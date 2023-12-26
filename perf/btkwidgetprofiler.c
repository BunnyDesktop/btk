#include "config.h"
#include <string.h>
#include "btkwidgetprofiler.h"
#include "marshalers.h"
#include "typebuiltins.h"

typedef enum {
  STATE_NOT_CREATED,
  STATE_INSTRUMENTED_NOT_MAPPED,
  STATE_INSTRUMENTED_MAPPED
} State;

struct _BtkWidgetProfilerPrivate {
  State state;

  BtkWidget *profiled_widget;
  BtkWidget *toplevel;

  int n_iterations;

  GTimer *timer;

  gulong toplevel_expose_event_id;
  gulong toplevel_property_notify_event_id;

  BdkAtom profiler_atom;

  guint profiling : 1;
};

G_DEFINE_TYPE (BtkWidgetProfiler, btk_widget_profiler, G_TYPE_OBJECT);

static void btk_widget_profiler_finalize (GObject *object);

enum {
  CREATE_WIDGET,
  REPORT,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static void
btk_widget_profiler_class_init (BtkWidgetProfilerClass *class)
{
  GObjectClass *object_class;

  object_class = (GObjectClass *) class;

  signals[CREATE_WIDGET] =
    g_signal_new ("create-widget",
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetProfilerClass, create_widget),
		  NULL, NULL,
		  _btk_marshal_OBJECT__VOID,
		  G_TYPE_OBJECT, 0);

  signals[REPORT] =
    g_signal_new ("report",
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkWidgetProfilerClass, report),
		  NULL, NULL,
		  _btk_marshal_VOID__ENUM_OBJECT_DOUBLE,
		  G_TYPE_NONE, 3,
		  BTK_TYPE_WIDGET_PROFILER_REPORT,
		  G_TYPE_OBJECT,
		  G_TYPE_DOUBLE);

  object_class->finalize = btk_widget_profiler_finalize;
}

static void
btk_widget_profiler_init (BtkWidgetProfiler *profiler)
{
  BtkWidgetProfilerPrivate *priv;

  priv = g_new0 (BtkWidgetProfilerPrivate, 1);
  profiler->priv = priv;

  priv->state = STATE_NOT_CREATED;
  priv->n_iterations = 1;

  priv->timer = g_timer_new ();

  priv->profiler_atom = bdk_atom_intern ("BtkWidgetProfiler", FALSE);
}

static void
reset_state (BtkWidgetProfiler *profiler)
{
  BtkWidgetProfilerPrivate *priv;

  priv = profiler->priv;

  if (priv->toplevel)
    {
      g_signal_handler_disconnect (priv->toplevel, priv->toplevel_expose_event_id);
      priv->toplevel_expose_event_id = 0;

      g_signal_handler_disconnect (priv->toplevel, priv->toplevel_property_notify_event_id);
      priv->toplevel_property_notify_event_id = 0;

      btk_widget_destroy (priv->toplevel);
      priv->toplevel = NULL;
      priv->profiled_widget = NULL;
    }

  priv->state = STATE_NOT_CREATED;
}

static void
btk_widget_profiler_finalize (GObject *object)
{
  BtkWidgetProfiler *profiler;
  BtkWidgetProfilerPrivate *priv;

  profiler = BTK_WIDGET_PROFILER (object);
  priv = profiler->priv;

  reset_state (profiler);
  g_timer_destroy (priv->timer);

  g_free (priv);

  G_OBJECT_CLASS (btk_widget_profiler_parent_class)->finalize (object);
}

BtkWidgetProfiler *
btk_widget_profiler_new (void)
{
  return g_object_new (BTK_TYPE_WIDGET_PROFILER, NULL);
}

void
btk_widget_profiler_set_num_iterations (BtkWidgetProfiler *profiler,
					gint               n_iterations)
{
  BtkWidgetProfilerPrivate *priv;

  g_return_if_fail (BTK_IS_WIDGET_PROFILER (profiler));
  g_return_if_fail (n_iterations > 0);

  priv = profiler->priv;
  priv->n_iterations = n_iterations;
}

static void
report (BtkWidgetProfiler      *profiler,
	BtkWidgetProfilerReport report,
	gdouble                 elapsed)
{
  BtkWidgetProfilerPrivate *priv;

  priv = profiler->priv;

  g_signal_emit (profiler, signals[REPORT], 0, report, priv->profiled_widget, elapsed);
}

static BtkWidget *
create_widget_via_emission (BtkWidgetProfiler *profiler)
{
  BtkWidget *widget;

  widget = NULL;
  g_signal_emit (profiler, signals[CREATE_WIDGET], 0, &widget);
  if (!widget)
    g_error ("The profiler emitted the \"create-widget\" signal but the signal handler returned no widget!");

  if (btk_widget_get_visible (widget) || btk_widget_get_mapped (widget))
    g_error ("The handler for \"create-widget\" must return an unmapped and unshown widget");

  return widget;
}

static gboolean
toplevel_property_notify_event_cb (BtkWidget *widget, BdkEventProperty *event, gpointer data)
{
  BtkWidgetProfiler *profiler;
  BtkWidgetProfilerPrivate *priv;
  gdouble elapsed;

  profiler = BTK_WIDGET_PROFILER (data);
  priv = profiler->priv;

  if (event->atom != priv->profiler_atom)
    return FALSE;

  /* Finish timing map/expose */

  elapsed = g_timer_elapsed (priv->timer, NULL);
  report (profiler, BTK_WIDGET_PROFILER_REPORT_EXPOSE, elapsed);

  btk_main_quit (); /* This will get us back to the end of profile_map_expose() */
  return TRUE;
}

static gboolean
toplevel_idle_after_expose_cb (gpointer data)
{
  BtkWidgetProfiler *profiler;
  BtkWidgetProfilerPrivate *priv;

  profiler = BTK_WIDGET_PROFILER (data);
  priv = profiler->priv;

  bdk_property_change (priv->toplevel->window,
		       priv->profiler_atom,
		       bdk_atom_intern ("STRING", FALSE),
		       8,
		       BDK_PROP_MODE_REPLACE,
		       (guchar *) "hello",
		       strlen ("hello"));

  return FALSE;
}

static gboolean
toplevel_expose_event_cb (BtkWidget *widget, BdkEventExpose *event, gpointer data)
{
  BtkWidgetProfiler *profiler;

  profiler = BTK_WIDGET_PROFILER (data);

  bdk_threads_add_idle_full (G_PRIORITY_HIGH, toplevel_idle_after_expose_cb, profiler, NULL);
  return FALSE;
}

static void
instrument_toplevel (BtkWidgetProfiler *profiler,
		     BtkWidget         *toplevel)
{
  BtkWidgetProfilerPrivate *priv;

  priv = profiler->priv;

  priv->toplevel_expose_event_id = g_signal_connect (toplevel, "expose-event",
						     G_CALLBACK (toplevel_expose_event_cb), profiler);

  btk_widget_add_events (toplevel, BDK_PROPERTY_CHANGE_MASK);
  priv->toplevel_property_notify_event_id = g_signal_connect (toplevel, "property-notify-event",
							      G_CALLBACK (toplevel_property_notify_event_cb), profiler);
}

static BtkWidget *
ensure_and_get_toplevel (BtkWidget *widget)
{
	BtkWidget *toplevel;
	BtkWidget *window;

	toplevel = btk_widget_get_toplevel (widget);
	if (btk_widget_is_toplevel (toplevel))
		return toplevel;

	g_assert (toplevel == widget); /* we don't want extraneous ancestors */

	window = btk_window_new (BTK_WINDOW_TOPLEVEL);
	btk_container_add (BTK_CONTAINER (window), widget);

	return window;
}

static BtkWidget *
get_instrumented_toplevel (BtkWidgetProfiler *profiler,
			   BtkWidget         *widget)
{
  BtkWidget *window;

  window = ensure_and_get_toplevel (widget);
  instrument_toplevel (profiler, window);

  return window;
}

static void
map_widget (BtkWidgetProfiler *profiler)
{
  BtkWidgetProfilerPrivate *priv;

  priv = profiler->priv;
  g_assert (priv->state == STATE_INSTRUMENTED_NOT_MAPPED);

  /* Time map.
   *
   * FIXME: we are really timing a show_all(); we don't really wait for all the "map_event" signals
   * to happen.  Should we rename BTK_WIDGET_PROFILER_REPORT_MAP report to something else?
   */

  btk_widget_show_all (priv->toplevel);
  priv->state = STATE_INSTRUMENTED_MAPPED;
}

static void
profile_map_expose (BtkWidgetProfiler *profiler)
{
  BtkWidgetProfilerPrivate *priv;
  gdouble elapsed;

  priv = profiler->priv;

  g_assert (priv->state == STATE_INSTRUMENTED_NOT_MAPPED);

  g_timer_reset (priv->timer);
  map_widget (profiler);
  elapsed = g_timer_elapsed (priv->timer, NULL);

  report (profiler, BTK_WIDGET_PROFILER_REPORT_MAP, elapsed);

  /* Time expose; this gets recorded in toplevel_property_notify_event_cb() */

  g_timer_reset (priv->timer);
  btk_main ();
}

static void
profile_destroy (BtkWidgetProfiler *profiler)
{
  BtkWidgetProfilerPrivate *priv;
  gdouble elapsed;

  priv = profiler->priv;

  g_assert (priv->state != STATE_NOT_CREATED);

  g_timer_reset (priv->timer);
  reset_state (profiler);
  elapsed = g_timer_elapsed (priv->timer, NULL);

  report (profiler, BTK_WIDGET_PROFILER_REPORT_DESTROY, elapsed);
}

static void
create_widget (BtkWidgetProfiler *profiler)
{
  BtkWidgetProfilerPrivate *priv;

  priv = profiler->priv;

  g_assert (priv->state == STATE_NOT_CREATED);

  priv->profiled_widget = create_widget_via_emission (profiler);
  priv->toplevel = get_instrumented_toplevel (profiler, priv->profiled_widget);

  priv->state = STATE_INSTRUMENTED_NOT_MAPPED;
}

/* The "boot time" of a widget is the time needed to
 *
 *   1. Create the widget
 *   2. Map it
 *   3. Expose it
 *   4. Destroy it.
 *
 * This runs a lot of interesting code:  instantiation, size requisition and
 * allocation, realization, mapping, exposing, destruction.
 */
static void
profile_boot (BtkWidgetProfiler *profiler)
{
  BtkWidgetProfilerPrivate *priv;
  gdouble elapsed;

  priv = profiler->priv;

  g_assert (priv->state == STATE_NOT_CREATED);

  /* Time creation */

  g_timer_reset (priv->timer);
  create_widget (profiler);
  elapsed = g_timer_elapsed (priv->timer, NULL);

  report (profiler, BTK_WIDGET_PROFILER_REPORT_CREATE, elapsed);

  /* Start timing map/expose */

  profile_map_expose (profiler);

  /* Profile destruction */

  profile_destroy (profiler);
}

/* To measure expose time, we trigger a full expose on the toplevel window.  We
 * do the same as xrefresh(1), i.e. we map and unmap a window to make the other
 * one expose.
 */
static void
profile_expose (BtkWidgetProfiler *profiler)
{
  BtkWidgetProfilerPrivate *priv;
  BdkWindow *window;
  BdkWindowAttr attr;
  int attr_mask;

  priv = profiler->priv;

  g_assert (priv->state == STATE_INSTRUMENTED_MAPPED);

  /* Time creation */

  attr.x = 0;
  attr.y = 0;
  attr.width = priv->toplevel->allocation.width;
  attr.height = priv->toplevel->allocation.width;
  attr.wclass = BDK_INPUT_OUTPUT;
  attr.window_type = BDK_WINDOW_CHILD;

  attr_mask = BDK_WA_X | BDK_WA_Y;

  window = bdk_window_new (priv->toplevel->window, &attr, attr_mask);
  bdk_window_set_back_pixmap (window, NULL, TRUE); /* avoid flicker */

  bdk_window_show (window);
  bdk_window_hide (window);
  bdk_window_destroy (window);

  /* Time expose; this gets recorded in toplevel_property_notify_event_cb() */

  g_timer_reset (priv->timer);
  btk_main ();
}

void
btk_widget_profiler_profile_boot (BtkWidgetProfiler *profiler)
{
  BtkWidgetProfilerPrivate *priv;
  int i, n;

  g_return_if_fail (BTK_IS_WIDGET_PROFILER (profiler));

  priv = profiler->priv;
  g_return_if_fail (!priv->profiling);

  reset_state (profiler);
  priv->profiling = TRUE;

  n = priv->n_iterations;
  for (i = 0; i < n; i++)
    profile_boot (profiler);

  priv->profiling = FALSE;
}

void
btk_widget_profiler_profile_expose (BtkWidgetProfiler *profiler)
{
  BtkWidgetProfilerPrivate *priv;
  int i, n;

  g_return_if_fail (BTK_IS_WIDGET_PROFILER (profiler));

  priv = profiler->priv;
  g_return_if_fail (!priv->profiling);

  reset_state (profiler);
  priv->profiling = TRUE;

  create_widget (profiler);
  map_widget (profiler);

  n = priv->n_iterations;
  for (i = 0; i < n; i++)
    profile_expose (profiler);

  priv->profiling = FALSE;

  reset_state (profiler);
}
