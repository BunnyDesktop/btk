/* BTK - The GIMP Toolkit
 *
 * Copyright (C) 2007 John Stowers, Neil Jagdish Patel.
 * Copyright (C) 2009 Bastien Nocera, David Zeuthen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 *
 * Code adapted from egg-spinner
 * by Christian Hergert <christian.hergert@gmail.com>
 */

/*
 * Modified by the BTK+ Team and others 2007.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#include "config.h"

#include "btkintl.h"
#include "btkaccessible.h"
#include "btkimage.h"
#include "btkspinner.h"
#include "btkstyle.h"
#include "btkalias.h"


/**
 * SECTION:btkspinner
 * @Short_description: Show a spinner animation
 * @Title: BtkSpinner
 * @See_also: #BtkCellRendererSpinner, #BtkProgressBar
 *
 * A BtkSpinner widget displays an icon-size spinning animation.
 * It is often used as an alternative to a #BtkProgressBar for
 * displaying indefinite activity, instead of actual progress.
 *
 * To start the animation, use btk_spinner_start(), to stop it
 * use btk_spinner_stop().
 */


#define BTK_SPINNER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_SPINNER, BtkSpinnerPrivate))

G_DEFINE_TYPE (BtkSpinner, btk_spinner, BTK_TYPE_DRAWING_AREA);

enum {
  PROP_0,
  PROP_ACTIVE
};

struct _BtkSpinnerPrivate
{
  guint current;
  guint num_steps;
  guint cycle_duration;
  gboolean active;
  guint timeout;
};

static void btk_spinner_class_init     (BtkSpinnerClass *klass);
static void btk_spinner_init           (BtkSpinner      *spinner);
static void btk_spinner_dispose        (GObject         *bobject);
static void btk_spinner_realize        (BtkWidget       *widget);
static void btk_spinner_unrealize      (BtkWidget       *widget);
static gboolean btk_spinner_expose     (BtkWidget       *widget,
                                        BdkEventExpose  *event);
static void btk_spinner_screen_changed (BtkWidget       *widget,
                                        BdkScreen       *old_screen);
static void btk_spinner_style_set      (BtkWidget       *widget,
                                        BtkStyle        *prev_style);
static void btk_spinner_get_property   (GObject         *object,
                                        guint            param_id,
                                        GValue          *value,
                                        GParamSpec      *pspec);
static void btk_spinner_set_property   (GObject         *object,
                                        guint            param_id,
                                        const GValue    *value,
                                        GParamSpec      *pspec);
static void btk_spinner_set_active     (BtkSpinner      *spinner,
                                        gboolean         active);
static BatkObject *btk_spinner_get_accessible      (BtkWidget *widget);
static GType      btk_spinner_accessible_get_type (void);

static void
btk_spinner_class_init (BtkSpinnerClass *klass)
{
  GObjectClass *bobject_class;
  BtkWidgetClass *widget_class;

  bobject_class = G_OBJECT_CLASS(klass);
  g_type_class_add_private (bobject_class, sizeof (BtkSpinnerPrivate));
  bobject_class->dispose = btk_spinner_dispose;
  bobject_class->get_property = btk_spinner_get_property;
  bobject_class->set_property = btk_spinner_set_property;

  widget_class = BTK_WIDGET_CLASS(klass);
  widget_class->expose_event = btk_spinner_expose;
  widget_class->realize = btk_spinner_realize;
  widget_class->unrealize = btk_spinner_unrealize;
  widget_class->screen_changed = btk_spinner_screen_changed;
  widget_class->style_set = btk_spinner_style_set;
  widget_class->get_accessible = btk_spinner_get_accessible;

  /* BtkSpinner:active:
   *
   * Whether the spinner is active
   *
   * Since: 2.20
   */
  g_object_class_install_property (bobject_class,
                                   PROP_ACTIVE,
                                   g_param_spec_boolean ("active",
                                                         P_("Active"),
                                                         P_("Whether the spinner is active"),
                                                         FALSE,
                                                         G_PARAM_READWRITE));
  /**
   * BtkSpinner:num-steps:
   *
   * The number of steps for the spinner to complete a full loop.
   * The animation will complete a full cycle in one second by default
   * (see the #BtkSpinner:cycle-duration style property).
   *
   * Since: 2.20
   */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_uint ("num-steps",
                                                             P_("Number of steps"),
                                                             P_("The number of steps for the spinner to complete a full loop. The animation will complete a full cycle in one second by default (see #BtkSpinner:cycle-duration)."),
                                                             1,
                                                             G_MAXUINT,
                                                             12,
                                                             G_PARAM_READABLE));

  /**
   * BtkSpinner:cycle-duration:
   *
   * The duration in milliseconds for the spinner to complete a full cycle.
   *
   * Since: 2.20
   */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_uint ("cycle-duration",
                                                             P_("Animation duration"),
                                                             P_("The length of time in milliseconds for the spinner to complete a full loop"),
                                                             500,
                                                             G_MAXUINT,
                                                             1000,
                                                             G_PARAM_READABLE));
}

static void
btk_spinner_get_property (GObject    *object,
                          guint       param_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  BtkSpinnerPrivate *priv;

  priv = BTK_SPINNER (object)->priv;

  switch (param_id)
    {
      case PROP_ACTIVE:
        g_value_set_boolean (value, priv->active);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }
}

static void
btk_spinner_set_property (GObject      *object,
                          guint         param_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  switch (param_id)
    {
      case PROP_ACTIVE:
        btk_spinner_set_active (BTK_SPINNER (object), g_value_get_boolean (value));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }
}

static void
btk_spinner_init (BtkSpinner *spinner)
{
  BtkSpinnerPrivate *priv;

  priv = BTK_SPINNER_GET_PRIVATE (spinner);
  priv->current = 0;
  priv->timeout = 0;

  spinner->priv = priv;

  btk_widget_set_has_window (BTK_WIDGET (spinner), FALSE);
}

static gboolean
btk_spinner_expose (BtkWidget      *widget,
                    BdkEventExpose *event)
{
  BtkStateType state_type;
  BtkSpinnerPrivate *priv;
  int width, height;

  priv = BTK_SPINNER (widget)->priv;

  width = widget->allocation.width;
  height = widget->allocation.height;

  if ((width < 12) || (height <12))
    btk_widget_set_size_request (widget, 12, 12);

  state_type = BTK_STATE_NORMAL;
  if (!btk_widget_is_sensitive (widget))
   state_type = BTK_STATE_INSENSITIVE;

  btk_paint_spinner (widget->style,
                     widget->window,
                     state_type,
                     &event->area,
                     widget,
                     "spinner",
                     priv->current,
                     event->area.x, event->area.y,
                     event->area.width, event->area.height);

  return FALSE;
}

static gboolean
btk_spinner_timeout (gpointer data)
{
  BtkSpinnerPrivate *priv;

  priv = BTK_SPINNER (data)->priv;

  if (priv->current + 1 >= priv->num_steps)
    priv->current = 0;
  else
    priv->current++;

  btk_widget_queue_draw (BTK_WIDGET (data));

  return TRUE;
}

static void
btk_spinner_add_timeout (BtkSpinner *spinner)
{
  BtkSpinnerPrivate *priv;

  priv = spinner->priv;

  priv->timeout = bdk_threads_add_timeout ((guint) priv->cycle_duration / priv->num_steps, btk_spinner_timeout, spinner);
}

static void
btk_spinner_remove_timeout (BtkSpinner *spinner)
{
  BtkSpinnerPrivate *priv;

  priv = spinner->priv;

  g_source_remove (priv->timeout);
  priv->timeout = 0;
}

static void
btk_spinner_realize (BtkWidget *widget)
{
  BtkSpinnerPrivate *priv;

  priv = BTK_SPINNER (widget)->priv;

  BTK_WIDGET_CLASS (btk_spinner_parent_class)->realize (widget);

  if (priv->active)
    btk_spinner_add_timeout (BTK_SPINNER (widget));
}

static void
btk_spinner_unrealize (BtkWidget *widget)
{
  BtkSpinnerPrivate *priv;

  priv = BTK_SPINNER (widget)->priv;

  if (priv->timeout != 0)
    {
      btk_spinner_remove_timeout (BTK_SPINNER (widget));
    }

  BTK_WIDGET_CLASS (btk_spinner_parent_class)->unrealize (widget);
}

static void
btk_spinner_screen_changed (BtkWidget* widget, BdkScreen* old_screen)
{
  BtkSpinner *spinner;
  BdkScreen* new_screen;
  BdkColormap* colormap;

  spinner = BTK_SPINNER (widget);

  new_screen = btk_widget_get_screen (widget);
  colormap = bdk_screen_get_rgba_colormap (new_screen);

  if (!colormap)
    {
      colormap = bdk_screen_get_rgb_colormap (new_screen);
    }

  btk_widget_set_colormap (widget, colormap);
}

static void
btk_spinner_style_set (BtkWidget *widget,
                       BtkStyle  *prev_style)
{
  BtkSpinnerPrivate *priv;

  priv = BTK_SPINNER (widget)->priv;

  btk_widget_style_get (BTK_WIDGET (widget),
                        "num-steps", &(priv->num_steps),
                        "cycle-duration", &(priv->cycle_duration),
                        NULL);

  if (priv->current > priv->num_steps)
    priv->current = 0;
}

static void
btk_spinner_dispose (GObject *bobject)
{
  BtkSpinnerPrivate *priv;

  priv = BTK_SPINNER (bobject)->priv;

  if (priv->timeout != 0)
    {
      btk_spinner_remove_timeout (BTK_SPINNER (bobject));
    }

  G_OBJECT_CLASS (btk_spinner_parent_class)->dispose (bobject);
}

static void
btk_spinner_set_active (BtkSpinner *spinner, gboolean active)
{
  BtkSpinnerPrivate *priv;

  active = active != FALSE;

  priv = BTK_SPINNER (spinner)->priv;

  if (priv->active != active)
    {
      priv->active = active;
      g_object_notify (G_OBJECT (spinner), "active");

      if (active && btk_widget_get_realized (BTK_WIDGET (spinner)) && priv->timeout == 0)
        {
          btk_spinner_add_timeout (spinner);
        }
      else if (!active && priv->timeout != 0)
        {
          btk_spinner_remove_timeout (spinner);
        }
    }
}

static GType
btk_spinner_accessible_factory_get_accessible_type (void)
{
  return btk_spinner_accessible_get_type ();
}

static BatkObject *
btk_spinner_accessible_new (GObject *obj)
{
  BatkObject *accessible;

  g_return_val_if_fail (BTK_IS_WIDGET (obj), NULL);

  accessible = g_object_new (btk_spinner_accessible_get_type (), NULL);
  batk_object_initialize (accessible, obj);

  return accessible;
}

static BatkObject*
btk_spinner_accessible_factory_create_accessible (GObject *obj)
{
  return btk_spinner_accessible_new (obj);
}

static void
btk_spinner_accessible_factory_class_init (BatkObjectFactoryClass *klass)
{
  klass->create_accessible = btk_spinner_accessible_factory_create_accessible;
  klass->get_accessible_type = btk_spinner_accessible_factory_get_accessible_type;
}

static GType
btk_spinner_accessible_factory_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      const GTypeInfo tinfo =
      {
        sizeof (BatkObjectFactoryClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) btk_spinner_accessible_factory_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (BatkObjectFactory),
        0,             /* n_preallocs */
        NULL, NULL
      };

      type = g_type_register_static (BATK_TYPE_OBJECT_FACTORY,
                                    I_("BtkSpinnerAccessibleFactory"),
                                    &tinfo, 0);
    }
  return type;
}

static BatkObjectClass *a11y_parent_class = NULL;

static void
btk_spinner_accessible_initialize (BatkObject *accessible,
                                   gpointer   widget)
{
  batk_object_set_name (accessible, C_("throbbing progress animation widget", "Spinner"));
  batk_object_set_description (accessible, _("Provides visual indication of progress"));

  a11y_parent_class->initialize (accessible, widget);
}

static void
btk_spinner_accessible_class_init (BatkObjectClass *klass)
{
  a11y_parent_class = g_type_class_peek_parent (klass);

  klass->initialize = btk_spinner_accessible_initialize;
}

static void
btk_spinner_accessible_image_get_size (BatkImage *image,
                                       gint     *width,
                                       gint     *height)
{
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (image)->widget;
  if (!widget)
    {
      *width = *height = 0;
    }
  else
    {
      *width = widget->allocation.width;
      *height = widget->allocation.height;
    }
}

static void
btk_spinner_accessible_image_interface_init (BatkImageIface *iface)
{
  iface->get_image_size = btk_spinner_accessible_image_get_size;
}

static GType
btk_spinner_accessible_get_type (void)
{
  static GType type = 0;

  /* Action interface
     Name etc. ... */
  if (B_UNLIKELY (type == 0))
    {
      const GInterfaceInfo batk_image_info = {
              (GInterfaceInitFunc) btk_spinner_accessible_image_interface_init,
              (GInterfaceFinalizeFunc) NULL,
              NULL
      };
      GType parent_batk_type;
      GTypeInfo tinfo = { 0 };
      GTypeQuery query;
      BatkObjectFactory *factory;

      if ((type = g_type_from_name ("BtkSpinnerAccessible")))
        return type;

      factory = batk_registry_get_factory (batk_get_default_registry (),
                                          BTK_TYPE_IMAGE);
      if (!factory)
        return G_TYPE_INVALID;

      parent_batk_type = batk_object_factory_get_accessible_type (factory);
      if (!parent_batk_type)
        return G_TYPE_INVALID;

      /*
       * Figure out the size of the class and instance
       * we are deriving from
       */
      g_type_query (parent_batk_type, &query);

      tinfo.class_init = (GClassInitFunc) btk_spinner_accessible_class_init;
      tinfo.class_size    = query.class_size;
      tinfo.instance_size = query.instance_size;

      /* Register the type */
      type = g_type_register_static (parent_batk_type,
                                     "BtkSpinnerAccessible",
                                     &tinfo, 0);

      g_type_add_interface_static (type, BATK_TYPE_IMAGE,
                                   &batk_image_info);
    }

  return type;
}

static BatkObject *
btk_spinner_get_accessible (BtkWidget *widget)
{
  static gboolean first_time = TRUE;

  if (first_time)
    {
      BatkObjectFactory *factory;
      BatkRegistry *registry;
      GType derived_type;
      GType derived_batk_type;

      /*
       * Figure out whether accessibility is enabled by looking at the
       * type of the accessible object which would be created for
       * the parent type of BtkSpinner.
       */
      derived_type = g_type_parent (BTK_TYPE_SPINNER);

      registry = batk_get_default_registry ();
      factory = batk_registry_get_factory (registry,
                                          derived_type);
      derived_batk_type = batk_object_factory_get_accessible_type (factory);
      if (g_type_is_a (derived_batk_type, BTK_TYPE_ACCESSIBLE))
        batk_registry_set_factory_type (registry,
                                       BTK_TYPE_SPINNER,
                                       btk_spinner_accessible_factory_get_type ());
      first_time = FALSE;
    }
  return BTK_WIDGET_CLASS (btk_spinner_parent_class)->get_accessible (widget);
}

/**
 * btk_spinner_new:
 *
 * Returns a new spinner widget. Not yet started.
 *
 * Return value: a new #BtkSpinner
 *
 * Since: 2.20
 */
BtkWidget *
btk_spinner_new (void)
{
  return g_object_new (BTK_TYPE_SPINNER, NULL);
}

/**
 * btk_spinner_start:
 * @spinner: a #BtkSpinner
 *
 * Starts the animation of the spinner.
 *
 * Since: 2.20
 */
void
btk_spinner_start (BtkSpinner *spinner)
{
  g_return_if_fail (BTK_IS_SPINNER (spinner));

  btk_spinner_set_active (spinner, TRUE);
}

/**
 * btk_spinner_stop:
 * @spinner: a #BtkSpinner
 *
 * Stops the animation of the spinner.
 *
 * Since: 2.20
 */
void
btk_spinner_stop (BtkSpinner *spinner)
{
  g_return_if_fail (BTK_IS_SPINNER (spinner));

  btk_spinner_set_active (spinner, FALSE);
}

#define __BTK_SPINNER_C__
#include "btkaliasdef.c"
