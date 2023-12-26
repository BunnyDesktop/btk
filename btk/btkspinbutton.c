/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * BtkSpinButton widget for BTK+
 * Copyright (C) 1998 Lars Hamann and Stefan Jeske
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <locale.h>
#include "bdk/bdkkeysyms.h"
#include "btkbindings.h"
#include "btkspinbutton.h"
#include "btkentryprivate.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btksettings.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

#define MIN_SPIN_BUTTON_WIDTH 30
#define MAX_TIMER_CALLS       5
#define EPSILON               1e-10
#define	MAX_DIGITS            20
#define MIN_ARROW_WIDTH       6

enum {
  PROP_0,
  PROP_ADJUSTMENT,
  PROP_CLIMB_RATE,
  PROP_DIGITS,
  PROP_SNAP_TO_TICKS,
  PROP_NUMERIC,
  PROP_WRAP,
  PROP_UPDATE_POLICY,
  PROP_VALUE
};

/* Signals */
enum
{
  INPUT,
  OUTPUT,
  VALUE_CHANGED,
  CHANGE_VALUE,
  WRAPPED,
  LAST_SIGNAL
};

static void btk_spin_button_editable_init  (BtkEditableClass   *iface);
static void btk_spin_button_finalize       (GObject            *object);
static void btk_spin_button_destroy        (BtkObject          *object);
static void btk_spin_button_set_property   (GObject         *object,
					    guint            prop_id,
					    const GValue    *value,
					    GParamSpec      *pspec);
static void btk_spin_button_get_property   (GObject         *object,
					    guint            prop_id,
					    GValue          *value,
					    GParamSpec      *pspec);
static void btk_spin_button_map            (BtkWidget          *widget);
static void btk_spin_button_unmap          (BtkWidget          *widget);
static void btk_spin_button_realize        (BtkWidget          *widget);
static void btk_spin_button_unrealize      (BtkWidget          *widget);
static void btk_spin_button_size_request   (BtkWidget          *widget,
					    BtkRequisition     *requisition);
static void btk_spin_button_size_allocate  (BtkWidget          *widget,
					    BtkAllocation      *allocation);
static gint btk_spin_button_expose         (BtkWidget          *widget,
					    BdkEventExpose     *event);
static gint btk_spin_button_button_press   (BtkWidget          *widget,
					    BdkEventButton     *event);
static gint btk_spin_button_button_release (BtkWidget          *widget,
					    BdkEventButton     *event);
static gint btk_spin_button_motion_notify  (BtkWidget          *widget,
					    BdkEventMotion     *event);
static gint btk_spin_button_enter_notify   (BtkWidget          *widget,
					    BdkEventCrossing   *event);
static gint btk_spin_button_leave_notify   (BtkWidget          *widget,
					    BdkEventCrossing   *event);
static gint btk_spin_button_focus_out      (BtkWidget          *widget,
					    BdkEventFocus      *event);
static void btk_spin_button_grab_notify    (BtkWidget          *widget,
					    gboolean            was_grabbed);
static void btk_spin_button_state_changed  (BtkWidget          *widget,
					    BtkStateType        previous_state);
static void btk_spin_button_style_set      (BtkWidget          *widget,
                                            BtkStyle           *previous_style);
static void btk_spin_button_draw_arrow     (BtkSpinButton      *spin_button, 
					    BdkRectangle       *area,
					    BtkArrowType        arrow_type);
static gboolean btk_spin_button_timer          (BtkSpinButton      *spin_button);
static void btk_spin_button_stop_spinning  (BtkSpinButton      *spin);
static void btk_spin_button_value_changed  (BtkAdjustment      *adjustment,
					    BtkSpinButton      *spin_button); 
static gint btk_spin_button_key_release    (BtkWidget          *widget,
					    BdkEventKey        *event);
static gint btk_spin_button_scroll         (BtkWidget          *widget,
					    BdkEventScroll     *event);
static void btk_spin_button_activate       (BtkEntry           *entry);
static void btk_spin_button_get_text_area_size (BtkEntry *entry,
						gint     *x,
						gint     *y,
						gint     *width,
						gint     *height);
static void btk_spin_button_snap           (BtkSpinButton      *spin_button,
					    gdouble             val);
static void btk_spin_button_insert_text    (BtkEditable        *editable,
					    const gchar        *new_text,
					    gint                new_text_length,
					    gint               *position);
static void btk_spin_button_real_spin      (BtkSpinButton      *spin_button,
					    gdouble             step);
static void btk_spin_button_real_change_value (BtkSpinButton   *spin,
					       BtkScrollType    scroll);

static gint btk_spin_button_default_input  (BtkSpinButton      *spin_button,
					    gdouble            *new_val);
static gint btk_spin_button_default_output (BtkSpinButton      *spin_button);

static gint spin_button_get_arrow_size     (BtkSpinButton      *spin_button);
static gint spin_button_get_shadow_type    (BtkSpinButton      *spin_button);

static guint spinbutton_signals[LAST_SIGNAL] = {0};

#define NO_ARROW 2

G_DEFINE_TYPE_WITH_CODE (BtkSpinButton, btk_spin_button, BTK_TYPE_ENTRY,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_EDITABLE,
						btk_spin_button_editable_init))

#define add_spin_binding(binding_set, keyval, mask, scroll)            \
  btk_binding_entry_add_signal (binding_set, keyval, mask,             \
                                "change_value", 1,                     \
                                BTK_TYPE_SCROLL_TYPE, scroll)

static void
btk_spin_button_class_init (BtkSpinButtonClass *class)
{
  GObjectClass     *bobject_class = G_OBJECT_CLASS (class);
  BtkObjectClass   *object_class = BTK_OBJECT_CLASS (class);
  BtkWidgetClass   *widget_class = BTK_WIDGET_CLASS (class);
  BtkEntryClass    *entry_class = BTK_ENTRY_CLASS (class);
  BtkBindingSet    *binding_set;

  bobject_class->finalize = btk_spin_button_finalize;

  bobject_class->set_property = btk_spin_button_set_property;
  bobject_class->get_property = btk_spin_button_get_property;

  object_class->destroy = btk_spin_button_destroy;

  widget_class->map = btk_spin_button_map;
  widget_class->unmap = btk_spin_button_unmap;
  widget_class->realize = btk_spin_button_realize;
  widget_class->unrealize = btk_spin_button_unrealize;
  widget_class->size_request = btk_spin_button_size_request;
  widget_class->size_allocate = btk_spin_button_size_allocate;
  widget_class->expose_event = btk_spin_button_expose;
  widget_class->scroll_event = btk_spin_button_scroll;
  widget_class->button_press_event = btk_spin_button_button_press;
  widget_class->button_release_event = btk_spin_button_button_release;
  widget_class->motion_notify_event = btk_spin_button_motion_notify;
  widget_class->key_release_event = btk_spin_button_key_release;
  widget_class->enter_notify_event = btk_spin_button_enter_notify;
  widget_class->leave_notify_event = btk_spin_button_leave_notify;
  widget_class->focus_out_event = btk_spin_button_focus_out;
  widget_class->grab_notify = btk_spin_button_grab_notify;
  widget_class->state_changed = btk_spin_button_state_changed;
  widget_class->style_set = btk_spin_button_style_set;

  entry_class->activate = btk_spin_button_activate;
  entry_class->get_text_area_size = btk_spin_button_get_text_area_size;

  class->input = NULL;
  class->output = NULL;
  class->change_value = btk_spin_button_real_change_value;

  g_object_class_install_property (bobject_class,
                                   PROP_ADJUSTMENT,
                                   g_param_spec_object ("adjustment",
                                                        P_("Adjustment"),
                                                        P_("The adjustment that holds the value of the spinbutton"),
                                                        BTK_TYPE_ADJUSTMENT,
                                                        BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_CLIMB_RATE,
                                   g_param_spec_double ("climb-rate",
							P_("Climb Rate"),
							P_("The acceleration rate when you hold down a button"),
							0.0,
							G_MAXDOUBLE,
							0.0,
							BTK_PARAM_READWRITE));  
  
  g_object_class_install_property (bobject_class,
                                   PROP_DIGITS,
                                   g_param_spec_uint ("digits",
						      P_("Digits"),
						      P_("The number of decimal places to display"),
						      0,
						      MAX_DIGITS,
						      0,
						      BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_SNAP_TO_TICKS,
                                   g_param_spec_boolean ("snap-to-ticks",
							 P_("Snap to Ticks"),
							 P_("Whether erroneous values are automatically changed to a spin button's nearest step increment"),
							 FALSE,
							 BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_NUMERIC,
                                   g_param_spec_boolean ("numeric",
							 P_("Numeric"),
							 P_("Whether non-numeric characters should be ignored"),
							 FALSE,
							 BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_WRAP,
                                   g_param_spec_boolean ("wrap",
							 P_("Wrap"),
							 P_("Whether a spin button should wrap upon reaching its limits"),
							 FALSE,
							 BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_UPDATE_POLICY,
                                   g_param_spec_enum ("update-policy",
						      P_("Update Policy"),
						      P_("Whether the spin button should update always, or only when the value is legal"),
						      BTK_TYPE_SPIN_BUTTON_UPDATE_POLICY,
						      BTK_UPDATE_ALWAYS,
						      BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_VALUE,
                                   g_param_spec_double ("value",
							P_("Value"),
							P_("Reads the current value, or sets a new value"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE,
							0.0,
							BTK_PARAM_READWRITE));  
  
  btk_widget_class_install_style_property_parser (widget_class,
						  g_param_spec_enum ("shadow-type", 
								     "Shadow Type", 
								     P_("Style of bevel around the spin button"),
								     BTK_TYPE_SHADOW_TYPE,
								     BTK_SHADOW_IN,
								     BTK_PARAM_READABLE),
						  btk_rc_property_parse_enum);
  spinbutton_signals[INPUT] =
    g_signal_new (I_("input"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkSpinButtonClass, input),
		  NULL, NULL,
		  _btk_marshal_INT__POINTER,
		  G_TYPE_INT, 1,
		  G_TYPE_POINTER);

  /**
   * BtkSpinButton::output:
   * @spin_button: the object which received the signal
   * 
   * The ::output signal can be used to change to formatting
   * of the value that is displayed in the spin buttons entry.
   * |[
   * /&ast; show leading zeros &ast;/
   * static gboolean
   * on_output (BtkSpinButton *spin,
   *            gpointer       data)
   * {
   *    BtkAdjustment *adj;
   *    gchar *text;
   *    int value;
   *    
   *    adj = btk_spin_button_get_adjustment (spin);
   *    value = (int)btk_adjustment_get_value (adj);
   *    text = g_strdup_printf ("%02d", value);
   *    btk_entry_set_text (BTK_ENTRY (spin), text);
   *    g_free (text);
   *    
   *    return TRUE;
   * }
   * ]|
   *
   * Returns: %TRUE if the value has been displayed.
   */
  spinbutton_signals[OUTPUT] =
    g_signal_new (I_("output"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkSpinButtonClass, output),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);

  spinbutton_signals[VALUE_CHANGED] =
    g_signal_new (I_("value-changed"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkSpinButtonClass, value_changed),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * BtkSpinButton::wrapped:
   * @spinbutton: the object which received the signal
   *
   * The wrapped signal is emitted right after the spinbutton wraps
   * from its maximum to minimum value or vice-versa.
   *
   * Since: 2.10
   */
  spinbutton_signals[WRAPPED] =
    g_signal_new (I_("wrapped"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkSpinButtonClass, wrapped),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /* Action signals */
  spinbutton_signals[CHANGE_VALUE] =
    g_signal_new (I_("change-value"),
                  G_TYPE_FROM_CLASS (bobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BtkSpinButtonClass, change_value),
                  NULL, NULL,
                  _btk_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  BTK_TYPE_SCROLL_TYPE);
  
  binding_set = btk_binding_set_by_class (class);
  
  add_spin_binding (binding_set, BDK_Up, 0, BTK_SCROLL_STEP_UP);
  add_spin_binding (binding_set, BDK_KP_Up, 0, BTK_SCROLL_STEP_UP);
  add_spin_binding (binding_set, BDK_Down, 0, BTK_SCROLL_STEP_DOWN);
  add_spin_binding (binding_set, BDK_KP_Down, 0, BTK_SCROLL_STEP_DOWN);
  add_spin_binding (binding_set, BDK_Page_Up, 0, BTK_SCROLL_PAGE_UP);
  add_spin_binding (binding_set, BDK_Page_Down, 0, BTK_SCROLL_PAGE_DOWN);
  add_spin_binding (binding_set, BDK_Page_Up, BDK_CONTROL_MASK, BTK_SCROLL_END);
  add_spin_binding (binding_set, BDK_Page_Down, BDK_CONTROL_MASK, BTK_SCROLL_START);
}

static void
btk_spin_button_editable_init (BtkEditableClass *iface)
{
  iface->insert_text = btk_spin_button_insert_text;
}

static void
btk_spin_button_set_property (GObject      *object,
			      guint         prop_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
  BtkSpinButton *spin_button = BTK_SPIN_BUTTON (object);

  switch (prop_id)
    {
      BtkAdjustment *adjustment;

    case PROP_ADJUSTMENT:
      adjustment = BTK_ADJUSTMENT (g_value_get_object (value));
      if (!adjustment)
	adjustment = (BtkAdjustment*) btk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
      btk_spin_button_set_adjustment (spin_button, adjustment);
      break;
    case PROP_CLIMB_RATE:
      btk_spin_button_configure (spin_button,
				 spin_button->adjustment,
				 g_value_get_double (value),
				 spin_button->digits);
      break;
    case PROP_DIGITS:
      btk_spin_button_configure (spin_button,
				 spin_button->adjustment,
				 spin_button->climb_rate,
				 g_value_get_uint (value));
      break;
    case PROP_SNAP_TO_TICKS:
      btk_spin_button_set_snap_to_ticks (spin_button, g_value_get_boolean (value));
      break;
    case PROP_NUMERIC:
      btk_spin_button_set_numeric (spin_button, g_value_get_boolean (value));
      break;
    case PROP_WRAP:
      btk_spin_button_set_wrap (spin_button, g_value_get_boolean (value));
      break;
    case PROP_UPDATE_POLICY:
      btk_spin_button_set_update_policy (spin_button, g_value_get_enum (value));
      break;
    case PROP_VALUE:
      btk_spin_button_set_value (spin_button, g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_spin_button_get_property (GObject      *object,
			      guint         prop_id,
			      GValue       *value,
			      GParamSpec   *pspec)
{
  BtkSpinButton *spin_button = BTK_SPIN_BUTTON (object);

  switch (prop_id)
    {
    case PROP_ADJUSTMENT:
      g_value_set_object (value, spin_button->adjustment);
      break;
    case PROP_CLIMB_RATE:
      g_value_set_double (value, spin_button->climb_rate);
      break;
    case PROP_DIGITS:
      g_value_set_uint (value, spin_button->digits);
      break;
    case PROP_SNAP_TO_TICKS:
      g_value_set_boolean (value, spin_button->snap_to_ticks);
      break;
    case PROP_NUMERIC:
      g_value_set_boolean (value, spin_button->numeric);
      break;
    case PROP_WRAP:
      g_value_set_boolean (value, spin_button->wrap);
      break;
    case PROP_UPDATE_POLICY:
      g_value_set_enum (value, spin_button->update_policy);
      break;
     case PROP_VALUE:
       g_value_set_double (value, spin_button->adjustment->value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_spin_button_init (BtkSpinButton *spin_button)
{
  spin_button->adjustment = NULL;
  spin_button->panel = NULL;
  spin_button->timer = 0;
  spin_button->climb_rate = 0.0;
  spin_button->timer_step = 0.0;
  spin_button->update_policy = BTK_UPDATE_ALWAYS;
  spin_button->in_child = NO_ARROW;
  spin_button->click_child = NO_ARROW;
  spin_button->button = 0;
  spin_button->need_timer = FALSE;
  spin_button->timer_calls = 0;
  spin_button->digits = 0;
  spin_button->numeric = FALSE;
  spin_button->wrap = FALSE;
  spin_button->snap_to_ticks = FALSE;

  btk_spin_button_set_adjustment (spin_button,
	  (BtkAdjustment*) btk_adjustment_new (0, 0, 0, 0, 0, 0));
}

static void
btk_spin_button_finalize (GObject *object)
{
  btk_spin_button_set_adjustment (BTK_SPIN_BUTTON (object), NULL);
  
  G_OBJECT_CLASS (btk_spin_button_parent_class)->finalize (object);
}

static void
btk_spin_button_destroy (BtkObject *object)
{
  btk_spin_button_stop_spinning (BTK_SPIN_BUTTON (object));
  
  BTK_OBJECT_CLASS (btk_spin_button_parent_class)->destroy (object);
}

static void
btk_spin_button_map (BtkWidget *widget)
{
  if (btk_widget_get_realized (widget) && !btk_widget_get_mapped (widget))
    {
      BTK_WIDGET_CLASS (btk_spin_button_parent_class)->map (widget);
      bdk_window_show (BTK_SPIN_BUTTON (widget)->panel);
    }
}

static void
btk_spin_button_unmap (BtkWidget *widget)
{
  if (btk_widget_get_mapped (widget))
    {
      btk_spin_button_stop_spinning (BTK_SPIN_BUTTON (widget));

      bdk_window_hide (BTK_SPIN_BUTTON (widget)->panel);
      BTK_WIDGET_CLASS (btk_spin_button_parent_class)->unmap (widget);
    }
}

static void
btk_spin_button_realize (BtkWidget *widget)
{
  BtkSpinButton *spin_button = BTK_SPIN_BUTTON (widget);
  BdkWindowAttr attributes;
  gint attributes_mask;
  gboolean return_val;
  gint arrow_size;

  arrow_size = spin_button_get_arrow_size (spin_button);

  btk_widget_set_events (widget, btk_widget_get_events (widget) |
			 BDK_KEY_RELEASE_MASK);
  BTK_WIDGET_CLASS (btk_spin_button_parent_class)->realize (widget);

  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  attributes.event_mask = btk_widget_get_events (widget);
  attributes.event_mask |= BDK_EXPOSURE_MASK | BDK_BUTTON_PRESS_MASK 
    | BDK_BUTTON_RELEASE_MASK | BDK_LEAVE_NOTIFY_MASK | BDK_ENTER_NOTIFY_MASK 
    | BDK_POINTER_MOTION_MASK | BDK_POINTER_MOTION_HINT_MASK;

  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;

  attributes.x = (widget->allocation.width - arrow_size -
		  2 * widget->style->xthickness);
  attributes.y = (widget->allocation.height -
					 widget->requisition.height) / 2;
  attributes.width = arrow_size + 2 * widget->style->xthickness;
  attributes.height = widget->requisition.height;
  
  spin_button->panel = bdk_window_new (widget->window, 
				       &attributes, attributes_mask);
  bdk_window_set_user_data (spin_button->panel, widget);

  btk_style_set_background (widget->style, spin_button->panel, BTK_STATE_NORMAL);

  return_val = FALSE;
  g_signal_emit (spin_button, spinbutton_signals[OUTPUT], 0, &return_val);

  /* If output wasn't processed explicitly by the method connected to the
   * 'output' signal; and if we don't have any explicit 'text' set initially,
   * fallback to the default output. */
  if (!return_val &&
      (spin_button->numeric || btk_entry_get_text (BTK_ENTRY (spin_button)) == NULL))
    btk_spin_button_default_output (spin_button);

  btk_widget_queue_resize (BTK_WIDGET (spin_button));
}

static void
btk_spin_button_unrealize (BtkWidget *widget)
{
  BtkSpinButton *spin = BTK_SPIN_BUTTON (widget);

  btk_spin_button_stop_spinning (BTK_SPIN_BUTTON (widget));

  BTK_WIDGET_CLASS (btk_spin_button_parent_class)->unrealize (widget);

  if (spin->panel)
    {
      bdk_window_set_user_data (spin->panel, NULL);
      bdk_window_destroy (spin->panel);
      spin->panel = NULL;
    }
}

static int
compute_double_length (double val, int digits)
{
  int a;
  int extra;

  a = 1;
  if (fabs (val) > 1.0)
    a = floor (log10 (fabs (val))) + 1;  

  extra = 0;
  
  /* The dot: */
  if (digits > 0)
    extra++;

  /* The sign: */
  if (val < 0)
    extra++;

  return a + digits + extra;
}

static void
btk_spin_button_size_request (BtkWidget      *widget,
			      BtkRequisition *requisition)
{
  BtkSpinButton *spin_button = BTK_SPIN_BUTTON (widget);
  BtkEntry *entry = BTK_ENTRY (widget);
  gint arrow_size;

  arrow_size = spin_button_get_arrow_size (spin_button);

  BTK_WIDGET_CLASS (btk_spin_button_parent_class)->size_request (widget, requisition);

  if (entry->width_chars < 0)
    {
      BangoContext *context;
      BangoFontMetrics *metrics;
      gint width;
      gint w;
      gint string_len;
      gint max_string_len;
      gint digit_width;
      gboolean interior_focus;
      gint focus_width;
      gint xborder, yborder;
      BtkBorder inner_border;

      btk_widget_style_get (widget,
			    "interior-focus", &interior_focus,
			    "focus-line-width", &focus_width,
			    NULL);

      context = btk_widget_get_bango_context (widget);
      metrics = bango_context_get_metrics (context,
					   widget->style->font_desc,
					   bango_context_get_language (context));

      digit_width = bango_font_metrics_get_approximate_digit_width (metrics);
      digit_width = BANGO_SCALE *
        ((digit_width + BANGO_SCALE - 1) / BANGO_SCALE);

      bango_font_metrics_unref (metrics);
      
      /* Get max of MIN_SPIN_BUTTON_WIDTH, size of upper, size of lower */
      
      width = MIN_SPIN_BUTTON_WIDTH;
      max_string_len = MAX (10, compute_double_length (1e9 * spin_button->adjustment->step_increment,
                                                       spin_button->digits));

      string_len = compute_double_length (spin_button->adjustment->upper,
                                          spin_button->digits);
      w = BANGO_PIXELS (MIN (string_len, max_string_len) * digit_width);
      width = MAX (width, w);
      string_len = compute_double_length (spin_button->adjustment->lower,
					  spin_button->digits);
      w = BANGO_PIXELS (MIN (string_len, max_string_len) * digit_width);
      width = MAX (width, w);
      
      _btk_entry_get_borders (entry, &xborder, &yborder);
      _btk_entry_effective_inner_border (entry, &inner_border);

      requisition->width = width + xborder * 2 + inner_border.left + inner_border.right;
    }

  requisition->width += arrow_size + 2 * widget->style->xthickness;
}

static void
btk_spin_button_size_allocate (BtkWidget     *widget,
			       BtkAllocation *allocation)
{
  BtkSpinButton *spin = BTK_SPIN_BUTTON (widget);
  BtkAllocation panel_allocation;
  gint arrow_size;
  gint panel_width;

  arrow_size = spin_button_get_arrow_size (spin);
  panel_width = arrow_size + 2 * widget->style->xthickness;
  
  widget->allocation = *allocation;
  
  if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL)
    panel_allocation.x = 0;
  else
    panel_allocation.x = allocation->width - panel_width;

  panel_allocation.width = panel_width;
  panel_allocation.height = MIN (widget->requisition.height, allocation->height);

  panel_allocation.y = 0;

  BTK_WIDGET_CLASS (btk_spin_button_parent_class)->size_allocate (widget, allocation);

  if (btk_widget_get_realized (widget))
    {
      bdk_window_move_resize (BTK_SPIN_BUTTON (widget)->panel, 
			      panel_allocation.x,
			      panel_allocation.y,
			      panel_allocation.width,
			      panel_allocation.height); 
    }

  btk_widget_queue_draw (BTK_WIDGET (spin));
}

static gint
btk_spin_button_expose (BtkWidget      *widget,
			BdkEventExpose *event)
{
  BtkSpinButton *spin = BTK_SPIN_BUTTON (widget);

  if (btk_widget_is_drawable (widget))
    {
      if (event->window == spin->panel)
	{
	  BtkShadowType shadow_type;

	  shadow_type = spin_button_get_shadow_type (spin);

	  if (shadow_type != BTK_SHADOW_NONE)
	    {
	      gint width, height;
              gboolean state_hint;
              BtkStateType state;

              btk_widget_style_get (widget, "state-hint", &state_hint, NULL);
              if (state_hint)
                state = btk_widget_has_focus (widget) ?
                  BTK_STATE_ACTIVE : btk_widget_get_state (widget);
              else
                state = BTK_STATE_NORMAL;

              width = bdk_window_get_width (spin->panel);
              height = bdk_window_get_height (spin->panel);

              if (btk_entry_get_has_frame (BTK_ENTRY (spin)))
                btk_paint_box (widget->style, spin->panel,
                               state, shadow_type,
                               &event->area, widget, "spinbutton",
                               0, 0, width, height);
	    }

	  btk_spin_button_draw_arrow (spin, &event->area, BTK_ARROW_UP);
	  btk_spin_button_draw_arrow (spin, &event->area, BTK_ARROW_DOWN);
	}
      else
        {
          if (event->window == widget->window)
            {
              gint text_x, text_y, text_width, text_height, slice_x;

              /* Since we reuse xthickness for the buttons panel on one side, and BtkEntry
               * always sizes its background to (allocation->width - 2 * xthickness), we
               * have to manually render the missing slice of the background on the panel
               * side.
               */
              BTK_ENTRY_GET_CLASS (spin)->get_text_area_size (BTK_ENTRY (spin),
                                                              &text_x, &text_y,
                                                              &text_width, &text_height);

              if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL)
                slice_x = text_x - widget->style->xthickness;
              else
                slice_x = text_x + text_width;

              btk_paint_flat_box (widget->style, widget->window,
                                  btk_widget_get_state (widget), BTK_SHADOW_NONE,
                                  &event->area, widget, "entry_bg",
                                  slice_x, text_y,
                                  widget->style->xthickness, text_height);
            }

          BTK_WIDGET_CLASS (btk_spin_button_parent_class)->expose_event (widget, event);
        }
    }
  
  return FALSE;
}

static gboolean
spin_button_at_limit (BtkSpinButton *spin_button,
                     BtkArrowType   arrow)
{
  BtkArrowType effective_arrow;

  if (spin_button->wrap)
    return FALSE;

  if (spin_button->adjustment->step_increment > 0)
    effective_arrow = arrow;
  else
    effective_arrow = arrow == BTK_ARROW_UP ? BTK_ARROW_DOWN : BTK_ARROW_UP; 
  
  if (effective_arrow == BTK_ARROW_UP &&
      (spin_button->adjustment->upper - spin_button->adjustment->value <= EPSILON))
    return TRUE;
  
  if (effective_arrow == BTK_ARROW_DOWN &&
      (spin_button->adjustment->value - spin_button->adjustment->lower <= EPSILON))
    return TRUE;
  
  return FALSE;
}

static void
btk_spin_button_draw_arrow (BtkSpinButton *spin_button, 
			    BdkRectangle  *area,
			    BtkArrowType   arrow_type)
{
  BtkStateType state_type;
  BtkShadowType shadow_type;
  BtkWidget *widget;
  gint x;
  gint y;
  gint height;
  gint width;
  gint h, w;

  g_return_if_fail (arrow_type == BTK_ARROW_UP || arrow_type == BTK_ARROW_DOWN);

  widget = BTK_WIDGET (spin_button);

  if (btk_widget_is_drawable (widget))
    {
      width = spin_button_get_arrow_size (spin_button) + 2 * widget->style->xthickness;

      if (arrow_type == BTK_ARROW_UP)
	{
	  x = 0;
	  y = 0;

	  height = widget->requisition.height / 2;
	}
      else
	{
	  x = 0;
	  y = widget->requisition.height / 2;

	  height = (widget->requisition.height + 1) / 2;
	}

      if (spin_button_at_limit (spin_button, arrow_type))
	{
	  shadow_type = BTK_SHADOW_OUT;
	  state_type = BTK_STATE_INSENSITIVE;
	}
      else
	{
	  if (spin_button->click_child == arrow_type)
	    {
	      state_type = BTK_STATE_ACTIVE;
	      shadow_type = BTK_SHADOW_IN;
	    }
	  else
	    {
	      if (spin_button->in_child == arrow_type &&
		  spin_button->click_child == NO_ARROW)
		{
		  state_type = BTK_STATE_PRELIGHT;
		}
	      else
		{
		  state_type = btk_widget_get_state (widget);
		}
	      
	      shadow_type = BTK_SHADOW_OUT;
	    }
	}
      
      btk_paint_box (widget->style, spin_button->panel,
		     state_type, shadow_type,
		     area, widget,
		     (arrow_type == BTK_ARROW_UP)? "spinbutton_up" : "spinbutton_down",
		     x, y, width, height);

      height = widget->requisition.height;

      if (arrow_type == BTK_ARROW_DOWN)
	{
	  y = height / 2;
	  height = height - y - 2;
	}
      else
	{
	  y = 2;
	  height = height / 2 - 2;
	}

      width -= 3;

      if (widget && btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL)
	x = 2;
      else
	x = 1;

      w = width / 2;
      w -= w % 2 - 1; /* force odd */
      h = (w + 1) / 2;
      
      x += (width - w) / 2;
      y += (height - h) / 2;
      
      height = h;
      width = w;

      btk_paint_arrow (widget->style, spin_button->panel,
		       state_type, shadow_type, 
		       area, widget, "spinbutton",
		       arrow_type, TRUE, 
		       x, y, width, height);
    }
}

static gint
btk_spin_button_enter_notify (BtkWidget        *widget,
			      BdkEventCrossing *event)
{
  BtkSpinButton *spin = BTK_SPIN_BUTTON (widget);

  if (event->window == spin->panel)
    {
      gint x;
      gint y;

      bdk_window_get_pointer (spin->panel, &x, &y, NULL);

      if (y <= widget->requisition.height / 2)
	spin->in_child = BTK_ARROW_UP;
      else
	spin->in_child = BTK_ARROW_DOWN;

      btk_widget_queue_draw (BTK_WIDGET (spin));
    }
 
  if (BTK_WIDGET_CLASS (btk_spin_button_parent_class)->enter_notify_event)
    return BTK_WIDGET_CLASS (btk_spin_button_parent_class)->enter_notify_event (widget, event);

  return FALSE;
}

static gint
btk_spin_button_leave_notify (BtkWidget        *widget,
			      BdkEventCrossing *event)
{
  BtkSpinButton *spin = BTK_SPIN_BUTTON (widget);

  spin->in_child = NO_ARROW;
  btk_widget_queue_draw (BTK_WIDGET (spin));
 
  if (BTK_WIDGET_CLASS (btk_spin_button_parent_class)->leave_notify_event)
    return BTK_WIDGET_CLASS (btk_spin_button_parent_class)->leave_notify_event (widget, event);

  return FALSE;
}

static gint
btk_spin_button_focus_out (BtkWidget     *widget,
			   BdkEventFocus *event)
{
  if (BTK_ENTRY (widget)->editable)
    btk_spin_button_update (BTK_SPIN_BUTTON (widget));

  return BTK_WIDGET_CLASS (btk_spin_button_parent_class)->focus_out_event (widget, event);
}

static void
btk_spin_button_grab_notify (BtkWidget *widget,
			     gboolean   was_grabbed)
{
  BtkSpinButton *spin = BTK_SPIN_BUTTON (widget);

  if (!was_grabbed)
    {
      btk_spin_button_stop_spinning (spin);
      btk_widget_queue_draw (BTK_WIDGET (spin));
    }
}

static void
btk_spin_button_state_changed (BtkWidget    *widget,
			       BtkStateType  previous_state)
{
  BtkSpinButton *spin = BTK_SPIN_BUTTON (widget);

  if (!btk_widget_is_sensitive (widget))
    {
      btk_spin_button_stop_spinning (spin);    
      btk_widget_queue_draw (BTK_WIDGET (spin));
    }
}

static void
btk_spin_button_style_set (BtkWidget *widget,
		           BtkStyle  *previous_style)
{
  BtkSpinButton *spin = BTK_SPIN_BUTTON (widget);

  if (previous_style && btk_widget_get_realized (widget))
    btk_style_set_background (widget->style, spin->panel, BTK_STATE_NORMAL);

  BTK_WIDGET_CLASS (btk_spin_button_parent_class)->style_set (widget, previous_style);
}


static gint
btk_spin_button_scroll (BtkWidget      *widget,
			BdkEventScroll *event)
{
  BtkSpinButton *spin = BTK_SPIN_BUTTON (widget);

  if (event->direction == BDK_SCROLL_UP)
    {
      if (!btk_widget_has_focus (widget))
	btk_widget_grab_focus (widget);
      btk_spin_button_real_spin (spin, spin->adjustment->step_increment);
    }
  else if (event->direction == BDK_SCROLL_DOWN)
    {
      if (!btk_widget_has_focus (widget))
	btk_widget_grab_focus (widget);
      btk_spin_button_real_spin (spin, -spin->adjustment->step_increment); 
    }
  else
    return FALSE;

  return TRUE;
}

static void
btk_spin_button_stop_spinning (BtkSpinButton *spin)
{
  if (spin->timer)
    {
      g_source_remove (spin->timer);
      spin->timer = 0;
      spin->timer_calls = 0;
      spin->need_timer = FALSE;
    }

  spin->button = 0;
  spin->timer = 0;
  spin->timer_step = spin->adjustment->step_increment;
  spin->timer_calls = 0;

  spin->click_child = NO_ARROW;
  spin->button = 0;
}

static void
start_spinning (BtkSpinButton *spin,
		BtkArrowType   click_child,
		gdouble        step)
{
  g_return_if_fail (click_child == BTK_ARROW_UP || click_child == BTK_ARROW_DOWN);
  
  spin->click_child = click_child;
  
  if (!spin->timer)
    {
      BtkSettings *settings = btk_widget_get_settings (BTK_WIDGET (spin));
      guint        timeout;

      g_object_get (settings, "btk-timeout-initial", &timeout, NULL);

      spin->timer_step = step;
      spin->need_timer = TRUE;
      spin->timer = bdk_threads_add_timeout (timeout,
				   (GSourceFunc) btk_spin_button_timer,
				   (gpointer) spin);
    }
  btk_spin_button_real_spin (spin, click_child == BTK_ARROW_UP ? step : -step);

  btk_widget_queue_draw (BTK_WIDGET (spin));
}

static gint
btk_spin_button_button_press (BtkWidget      *widget,
			      BdkEventButton *event)
{
  BtkSpinButton *spin = BTK_SPIN_BUTTON (widget);

  if (!spin->button)
    {
      if (event->window == spin->panel)
	{
	  if (!btk_widget_has_focus (widget))
	    btk_widget_grab_focus (widget);
	  spin->button = event->button;
	  
	  if (BTK_ENTRY (widget)->editable)
	    btk_spin_button_update (spin);
	  
	  if (event->y <= widget->requisition.height / 2)
	    {
	      if (event->button == 1)
		start_spinning (spin, BTK_ARROW_UP, spin->adjustment->step_increment);
	      else if (event->button == 2)
		start_spinning (spin, BTK_ARROW_UP, spin->adjustment->page_increment);
	      else
		spin->click_child = BTK_ARROW_UP;
	    }
	  else 
	    {
	      if (event->button == 1)
		start_spinning (spin, BTK_ARROW_DOWN, spin->adjustment->step_increment);
	      else if (event->button == 2)
		start_spinning (spin, BTK_ARROW_DOWN, spin->adjustment->page_increment);
	      else
		spin->click_child = BTK_ARROW_DOWN;
	    }
	  return TRUE;
	}
      else
	return BTK_WIDGET_CLASS (btk_spin_button_parent_class)->button_press_event (widget, event);
    }
  return FALSE;
}

static gint
btk_spin_button_button_release (BtkWidget      *widget,
				BdkEventButton *event)
{
  BtkSpinButton *spin = BTK_SPIN_BUTTON (widget);
  gint arrow_size;

  arrow_size = spin_button_get_arrow_size (spin);

  if (event->button == spin->button)
    {
      int click_child = spin->click_child;

      btk_spin_button_stop_spinning (spin);

      if (event->button == 3)
	{
	  if (event->y >= 0 && event->x >= 0 && 
	      event->y <= widget->requisition.height &&
	      event->x <= arrow_size + 2 * widget->style->xthickness)
	    {
	      if (click_child == BTK_ARROW_UP &&
		  event->y <= widget->requisition.height / 2)
		{
		  gdouble diff;

		  diff = spin->adjustment->upper - spin->adjustment->value;
		  if (diff > EPSILON)
		    btk_spin_button_real_spin (spin, diff);
		}
	      else if (click_child == BTK_ARROW_DOWN &&
		       event->y > widget->requisition.height / 2)
		{
		  gdouble diff;

		  diff = spin->adjustment->value - spin->adjustment->lower;
		  if (diff > EPSILON)
		    btk_spin_button_real_spin (spin, -diff);
		}
	    }
	}		  
      btk_widget_queue_draw (BTK_WIDGET (spin));

      return TRUE;
    }
  else
    return BTK_WIDGET_CLASS (btk_spin_button_parent_class)->button_release_event (widget, event);
}

static gint
btk_spin_button_motion_notify (BtkWidget      *widget,
			       BdkEventMotion *event)
{
  BtkSpinButton *spin = BTK_SPIN_BUTTON (widget);

  if (spin->button)
    return FALSE;

  if (event->window == spin->panel)
    {
      gint y = event->y;

      bdk_event_request_motions (event);
  
      if (y <= widget->requisition.height / 2 && 
	  spin->in_child == BTK_ARROW_DOWN)
	{
	  spin->in_child = BTK_ARROW_UP;
	  btk_widget_queue_draw (BTK_WIDGET (spin));
	}
      else if (y > widget->requisition.height / 2 && 
	  spin->in_child == BTK_ARROW_UP)
	{
	  spin->in_child = BTK_ARROW_DOWN;
	  btk_widget_queue_draw (BTK_WIDGET (spin));
	}
      
      return FALSE;
    }
	  
  return BTK_WIDGET_CLASS (btk_spin_button_parent_class)->motion_notify_event (widget, event);
}

static gint
btk_spin_button_timer (BtkSpinButton *spin_button)
{
  gboolean retval = FALSE;
  
  if (spin_button->timer)
    {
      if (spin_button->click_child == BTK_ARROW_UP)
	btk_spin_button_real_spin (spin_button,	spin_button->timer_step);
      else
	btk_spin_button_real_spin (spin_button,	-spin_button->timer_step);

      if (spin_button->need_timer)
	{
          BtkSettings *settings = btk_widget_get_settings (BTK_WIDGET (spin_button));
          guint        timeout;

          g_object_get (settings, "btk-timeout-repeat", &timeout, NULL);

	  spin_button->need_timer = FALSE;
	  spin_button->timer = bdk_threads_add_timeout (timeout,
					      (GSourceFunc) btk_spin_button_timer, 
					      (gpointer) spin_button);
	}
      else 
	{
	  if (spin_button->climb_rate > 0.0 && spin_button->timer_step 
	      < spin_button->adjustment->page_increment)
	    {
	      if (spin_button->timer_calls < MAX_TIMER_CALLS)
		spin_button->timer_calls++;
	      else 
		{
		  spin_button->timer_calls = 0;
		  spin_button->timer_step += spin_button->climb_rate;
		}
	    }
	  retval = TRUE;
	}
    }

  return retval;
}

static void
btk_spin_button_value_changed (BtkAdjustment *adjustment,
			       BtkSpinButton *spin_button)
{
  gboolean return_val;

  g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));

  return_val = FALSE;
  g_signal_emit (spin_button, spinbutton_signals[OUTPUT], 0, &return_val);
  if (return_val == FALSE)
    btk_spin_button_default_output (spin_button);

  g_signal_emit (spin_button, spinbutton_signals[VALUE_CHANGED], 0);

  btk_widget_queue_draw (BTK_WIDGET (spin_button));
  
  g_object_notify (G_OBJECT (spin_button), "value");
}

static void
btk_spin_button_real_change_value (BtkSpinButton *spin,
				   BtkScrollType  scroll)
{
  gdouble old_value;

  /* When the key binding is activated, there may be an outstanding
   * value, so we first have to commit what is currently written in
   * the spin buttons text entry. See #106574
   */
  btk_spin_button_update (spin);

  old_value = spin->adjustment->value;

  /* We don't test whether the entry is editable, since
   * this key binding conceptually corresponds to changing
   * the value with the buttons using the mouse, which
   * we allow for non-editable spin buttons.
   */
  switch (scroll)
    {
    case BTK_SCROLL_STEP_BACKWARD:
    case BTK_SCROLL_STEP_DOWN:
    case BTK_SCROLL_STEP_LEFT:
      btk_spin_button_real_spin (spin, -spin->timer_step);
      
      if (spin->climb_rate > 0.0 && spin->timer_step
	  < spin->adjustment->page_increment)
	{
	  if (spin->timer_calls < MAX_TIMER_CALLS)
	    spin->timer_calls++;
	  else 
	    {
	      spin->timer_calls = 0;
	      spin->timer_step += spin->climb_rate;
	    }
	}
      break;
      
    case BTK_SCROLL_STEP_FORWARD:
    case BTK_SCROLL_STEP_UP:
    case BTK_SCROLL_STEP_RIGHT:
      btk_spin_button_real_spin (spin, spin->timer_step);
      
      if (spin->climb_rate > 0.0 && spin->timer_step
	  < spin->adjustment->page_increment)
	{
	  if (spin->timer_calls < MAX_TIMER_CALLS)
	    spin->timer_calls++;
	  else 
	    {
	      spin->timer_calls = 0;
	      spin->timer_step += spin->climb_rate;
	    }
	}
      break;
      
    case BTK_SCROLL_PAGE_BACKWARD:
    case BTK_SCROLL_PAGE_DOWN:
    case BTK_SCROLL_PAGE_LEFT:
      btk_spin_button_real_spin (spin, -spin->adjustment->page_increment);
      break;
      
    case BTK_SCROLL_PAGE_FORWARD:
    case BTK_SCROLL_PAGE_UP:
    case BTK_SCROLL_PAGE_RIGHT:
      btk_spin_button_real_spin (spin, spin->adjustment->page_increment);
      break;
      
    case BTK_SCROLL_START:
      {
	gdouble diff = spin->adjustment->value - spin->adjustment->lower;
	if (diff > EPSILON)
	  btk_spin_button_real_spin (spin, -diff);
	break;
      }
      
    case BTK_SCROLL_END:
      {
	gdouble diff = spin->adjustment->upper - spin->adjustment->value;
	if (diff > EPSILON)
	  btk_spin_button_real_spin (spin, diff);
	break;
      }
      
    default:
      g_warning ("Invalid scroll type %d for BtkSpinButton::change-value", scroll);
      break;
    }
  
  btk_spin_button_update (spin);

  if (spin->adjustment->value == old_value)
    btk_widget_error_bell (BTK_WIDGET (spin));
}

static gint
btk_spin_button_key_release (BtkWidget   *widget,
			     BdkEventKey *event)
{
  BtkSpinButton *spin = BTK_SPIN_BUTTON (widget);

  /* We only get a release at the end of a key repeat run, so reset the timer_step */
  spin->timer_step = spin->adjustment->step_increment;
  spin->timer_calls = 0;
  
  return TRUE;
}

static void
btk_spin_button_snap (BtkSpinButton *spin_button,
		      gdouble        val)
{
  gdouble inc;
  gdouble tmp;

  inc = spin_button->adjustment->step_increment;
  if (inc == 0)
    return;
  
  tmp = (val - spin_button->adjustment->lower) / inc;
  if (tmp - floor (tmp) < ceil (tmp) - tmp)
    val = spin_button->adjustment->lower + floor (tmp) * inc;
  else
    val = spin_button->adjustment->lower + ceil (tmp) * inc;

  btk_spin_button_set_value (spin_button, val);
}

static void
btk_spin_button_activate (BtkEntry *entry)
{
  if (entry->editable)
    btk_spin_button_update (BTK_SPIN_BUTTON (entry));

  /* Chain up so that entry->activates_default is honored */
  BTK_ENTRY_CLASS (btk_spin_button_parent_class)->activate (entry);
}

static void
btk_spin_button_get_text_area_size (BtkEntry *entry,
				    gint     *x,
				    gint     *y,
				    gint     *width,
				    gint     *height)
{
  gint arrow_size;
  gint panel_width;

  BTK_ENTRY_CLASS (btk_spin_button_parent_class)->get_text_area_size (entry, x, y, width, height);

  arrow_size = spin_button_get_arrow_size (BTK_SPIN_BUTTON (entry));
  panel_width = arrow_size + 2 * BTK_WIDGET (entry)->style->xthickness;

  if (width)
    *width -= panel_width;

  if (btk_widget_get_direction (BTK_WIDGET (entry)) == BTK_TEXT_DIR_RTL && x)
    *x += panel_width;
}

static void
btk_spin_button_insert_text (BtkEditable *editable,
			     const gchar *new_text,
			     gint         new_text_length,
			     gint        *position)
{
  BtkEntry *entry = BTK_ENTRY (editable);
  BtkSpinButton *spin = BTK_SPIN_BUTTON (editable);
  BtkEditableClass *parent_editable_iface = g_type_interface_peek (btk_spin_button_parent_class, BTK_TYPE_EDITABLE);
 
  if (spin->numeric)
    {
      struct lconv *lc;
      gboolean sign;
      gint dotpos = -1;
      gint i;
      BdkWChar pos_sign;
      BdkWChar neg_sign;
      gint entry_length;
      const gchar *entry_text;

      entry_length = btk_entry_get_text_length (entry);
      entry_text = btk_entry_get_text (entry);

      lc = localeconv ();

      if (*(lc->negative_sign))
	neg_sign = *(lc->negative_sign);
      else 
	neg_sign = '-';

      if (*(lc->positive_sign))
	pos_sign = *(lc->positive_sign);
      else 
	pos_sign = '+';

#ifdef G_OS_WIN32
      /* Workaround for bug caused by some Windows application messing
       * up the positive sign of the current locale, more specifically
       * HKEY_CURRENT_USER\Control Panel\International\sPositiveSign.
       * See bug #330743 and for instance
       * http://www.msnewsgroups.net/group/microsoft.public.dotnet.languages.csharp/topic36024.aspx
       *
       * I don't know if the positive sign always gets bogusly set to
       * a digit when the above Registry value is corrupted as
       * described. (In my test case, it got set to "8", and in the
       * bug report above it presumably was set ot "0".) Probably it
       * might get set to almost anything? So how to distinguish a
       * bogus value from some correct one for some locale? That is
       * probably hard, but at least we should filter out the
       * digits...
       */
      if (pos_sign >= '0' && pos_sign <= '9')
	pos_sign = '+';
#endif

      for (sign=0, i=0; i<entry_length; i++)
	if ((entry_text[i] == neg_sign) ||
	    (entry_text[i] == pos_sign))
	  {
	    sign = 1;
	    break;
	  }

      if (sign && !(*position))
	return;

      for (dotpos=-1, i=0; i<entry_length; i++)
	if (entry_text[i] == *(lc->decimal_point))
	  {
	    dotpos = i;
	    break;
	  }

      if (dotpos > -1 && *position > dotpos &&
	  (gint)spin->digits - entry_length
	    + dotpos - new_text_length + 1 < 0)
	return;

      for (i = 0; i < new_text_length; i++)
	{
	  if (new_text[i] == neg_sign || new_text[i] == pos_sign)
	    {
	      if (sign || (*position) || i)
		return;
	      sign = TRUE;
	    }
	  else if (new_text[i] == *(lc->decimal_point))
	    {
	      if (!spin->digits || dotpos > -1 || 
 		  (new_text_length - 1 - i + entry_length
		    - *position > (gint)spin->digits)) 
		return;
	      dotpos = *position + i;
	    }
	  else if (new_text[i] < 0x30 || new_text[i] > 0x39)
	    return;
	}
    }

  parent_editable_iface->insert_text (editable, new_text,
				      new_text_length, position);
}

static void
btk_spin_button_real_spin (BtkSpinButton *spin_button,
			   gdouble        increment)
{
  BtkAdjustment *adj;
  gdouble new_value = 0.0;
  gboolean wrapped = FALSE;
  
  adj = spin_button->adjustment;

  new_value = adj->value + increment;

  if (increment > 0)
    {
      if (spin_button->wrap)
	{
	  if (fabs (adj->value - adj->upper) < EPSILON)
	    {
	      new_value = adj->lower;
	      wrapped = TRUE;
	    }
	  else if (new_value > adj->upper)
	    new_value = adj->upper;
	}
      else
	new_value = MIN (new_value, adj->upper);
    }
  else if (increment < 0) 
    {
      if (spin_button->wrap)
	{
	  if (fabs (adj->value - adj->lower) < EPSILON)
	    {
	      new_value = adj->upper;
	      wrapped = TRUE;
	    }
	  else if (new_value < adj->lower)
	    new_value = adj->lower;
	}
      else
	new_value = MAX (new_value, adj->lower);
    }

  if (fabs (new_value - adj->value) > EPSILON)
    btk_adjustment_set_value (adj, new_value);

  if (wrapped)
    g_signal_emit (spin_button, spinbutton_signals[WRAPPED], 0);

  btk_widget_queue_draw (BTK_WIDGET (spin_button));
}

static gint
btk_spin_button_default_input (BtkSpinButton *spin_button,
			       gdouble       *new_val)
{
  gchar *err = NULL;

  *new_val = g_strtod (btk_entry_get_text (BTK_ENTRY (spin_button)), &err);
  if (*err)
    return BTK_INPUT_ERROR;
  else
    return FALSE;
}

static gint
btk_spin_button_default_output (BtkSpinButton *spin_button)
{
  gchar *buf = g_strdup_printf ("%0.*f", spin_button->digits, spin_button->adjustment->value);

  if (strcmp (buf, btk_entry_get_text (BTK_ENTRY (spin_button))))
    btk_entry_set_text (BTK_ENTRY (spin_button), buf);
  g_free (buf);
  return FALSE;
}


/***********************************************************
 ***********************************************************
 ***                  Public interface                   ***
 ***********************************************************
 ***********************************************************/


/**
 * btk_spin_button_configure:
 * @spin_button: a #BtkSpinButton
 * @adjustment: (allow-none):  a #BtkAdjustment.
 * @climb_rate: the new climb rate.
 * @digits: the number of decimal places to display in the spin button.
 *
 * Changes the properties of an existing spin button. The adjustment, climb rate,
 * and number of decimal places are all changed accordingly, after this function call.
 */
void
btk_spin_button_configure (BtkSpinButton  *spin_button,
			   BtkAdjustment  *adjustment,
			   gdouble         climb_rate,
			   guint           digits)
{
  g_return_if_fail (BTK_IS_SPIN_BUTTON (spin_button));

  if (adjustment)
    btk_spin_button_set_adjustment (spin_button, adjustment);
  else
    adjustment = spin_button->adjustment;

  g_object_freeze_notify (G_OBJECT (spin_button));
  if (spin_button->digits != digits) 
    {
      spin_button->digits = digits;
      g_object_notify (G_OBJECT (spin_button), "digits");
    }

  if (spin_button->climb_rate != climb_rate)
    {
      spin_button->climb_rate = climb_rate;
      g_object_notify (G_OBJECT (spin_button), "climb-rate");
    }
  g_object_thaw_notify (G_OBJECT (spin_button));

  btk_adjustment_value_changed (adjustment);
}

BtkWidget *
btk_spin_button_new (BtkAdjustment *adjustment,
		     gdouble        climb_rate,
		     guint          digits)
{
  BtkSpinButton *spin;

  if (adjustment)
    g_return_val_if_fail (BTK_IS_ADJUSTMENT (adjustment), NULL);

  spin = g_object_new (BTK_TYPE_SPIN_BUTTON, NULL);

  btk_spin_button_configure (spin, adjustment, climb_rate, digits);

  return BTK_WIDGET (spin);
}

/**
 * btk_spin_button_new_with_range:
 * @min: Minimum allowable value
 * @max: Maximum allowable value
 * @step: Increment added or subtracted by spinning the widget
 * 
 * This is a convenience constructor that allows creation of a numeric 
 * #BtkSpinButton without manually creating an adjustment. The value is 
 * initially set to the minimum value and a page increment of 10 * @step
 * is the default. The precision of the spin button is equivalent to the 
 * precision of @step. 
 * 
 * Note that the way in which the precision is derived works best if @step 
 * is a power of ten. If the resulting precision is not suitable for your 
 * needs, use btk_spin_button_set_digits() to correct it.
 * 
 * Return value: The new spin button as a #BtkWidget.
 **/
BtkWidget *
btk_spin_button_new_with_range (gdouble min,
				gdouble max,
				gdouble step)
{
  BtkObject *adj;
  BtkSpinButton *spin;
  gint digits;

  g_return_val_if_fail (min <= max, NULL);
  g_return_val_if_fail (step != 0.0, NULL);

  spin = g_object_new (BTK_TYPE_SPIN_BUTTON, NULL);

  adj = btk_adjustment_new (min, min, max, step, 10 * step, 0);

  if (fabs (step) >= 1.0 || step == 0.0)
    digits = 0;
  else {
    digits = abs ((gint) floor (log10 (fabs (step))));
    if (digits > MAX_DIGITS)
      digits = MAX_DIGITS;
  }

  btk_spin_button_configure (spin, BTK_ADJUSTMENT (adj), step, digits);

  btk_spin_button_set_numeric (spin, TRUE);

  return BTK_WIDGET (spin);
}

static void
warn_nonzero_page_size (BtkAdjustment *adjustment)
{
  if (btk_adjustment_get_page_size (adjustment) != 0.0)
    g_warning ("BtkSpinButton: setting an adjustment with non-zero page size is deprecated");
}

/* Callback used when the spin button's adjustment changes.  We need to redraw
 * the arrows when the adjustment's range changes, and reevaluate our size request.
 */
static void
adjustment_changed_cb (BtkAdjustment *adjustment, gpointer data)
{
  BtkSpinButton *spin_button;

  spin_button = BTK_SPIN_BUTTON (data);

  spin_button->timer_step = spin_button->adjustment->step_increment;
  warn_nonzero_page_size (adjustment);
  btk_widget_queue_resize (BTK_WIDGET (spin_button));
}

/**
 * btk_spin_button_set_adjustment:
 * @spin_button: a #BtkSpinButton
 * @adjustment: a #BtkAdjustment to replace the existing adjustment
 * 
 * Replaces the #BtkAdjustment associated with @spin_button.
 **/
void
btk_spin_button_set_adjustment (BtkSpinButton *spin_button,
				BtkAdjustment *adjustment)
{
  g_return_if_fail (BTK_IS_SPIN_BUTTON (spin_button));

  if (spin_button->adjustment != adjustment)
    {
      if (spin_button->adjustment)
        {
	  g_signal_handlers_disconnect_by_func (spin_button->adjustment,
						btk_spin_button_value_changed,
						spin_button);
	  g_signal_handlers_disconnect_by_func (spin_button->adjustment,
						adjustment_changed_cb,
						spin_button);
	  g_object_unref (spin_button->adjustment);
        }
      spin_button->adjustment = adjustment;
      if (adjustment)
        {
	  g_object_ref_sink (adjustment);
	  g_signal_connect (adjustment, "value-changed",
			    G_CALLBACK (btk_spin_button_value_changed),
			    spin_button);
	  g_signal_connect (adjustment, "changed",
			    G_CALLBACK (adjustment_changed_cb),
			    spin_button);
	  spin_button->timer_step = spin_button->adjustment->step_increment;
          warn_nonzero_page_size (adjustment);
        }

      btk_widget_queue_resize (BTK_WIDGET (spin_button));
    }

  g_object_notify (G_OBJECT (spin_button), "adjustment");
}

/**
 * btk_spin_button_get_adjustment:
 * @spin_button: a #BtkSpinButton
 * 
 * Get the adjustment associated with a #BtkSpinButton
 * 
 * Return value: (transfer none): the #BtkAdjustment of @spin_button
 **/
BtkAdjustment *
btk_spin_button_get_adjustment (BtkSpinButton *spin_button)
{
  g_return_val_if_fail (BTK_IS_SPIN_BUTTON (spin_button), NULL);

  return spin_button->adjustment;
}

/**
 * btk_spin_button_set_digits:
 * @spin_button: a #BtkSpinButton
 * @digits: the number of digits after the decimal point to be displayed for the spin button's value
 * 
 * Set the precision to be displayed by @spin_button. Up to 20 digit precision
 * is allowed.
 **/
void
btk_spin_button_set_digits (BtkSpinButton *spin_button,
			    guint          digits)
{
  g_return_if_fail (BTK_IS_SPIN_BUTTON (spin_button));

  if (spin_button->digits != digits)
    {
      spin_button->digits = digits;
      btk_spin_button_value_changed (spin_button->adjustment, spin_button);
      g_object_notify (G_OBJECT (spin_button), "digits");
      
      /* since lower/upper may have changed */
      btk_widget_queue_resize (BTK_WIDGET (spin_button));
    }
}

/**
 * btk_spin_button_get_digits:
 * @spin_button: a #BtkSpinButton
 *
 * Fetches the precision of @spin_button. See btk_spin_button_set_digits().
 *
 * Returns: the current precision
 **/
guint
btk_spin_button_get_digits (BtkSpinButton *spin_button)
{
  g_return_val_if_fail (BTK_IS_SPIN_BUTTON (spin_button), 0);

  return spin_button->digits;
}

/**
 * btk_spin_button_set_increments:
 * @spin_button: a #BtkSpinButton
 * @step: increment applied for a button 1 press.
 * @page: increment applied for a button 2 press.
 * 
 * Sets the step and page increments for spin_button.  This affects how 
 * quickly the value changes when the spin button's arrows are activated.
 **/
void
btk_spin_button_set_increments (BtkSpinButton *spin_button,
				gdouble        step,
				gdouble        page)
{
  g_return_if_fail (BTK_IS_SPIN_BUTTON (spin_button));

  spin_button->adjustment->step_increment = step;
  spin_button->adjustment->page_increment = page;
}

/**
 * btk_spin_button_get_increments:
 * @spin_button: a #BtkSpinButton
 * @step: (out) (allow-none): location to store step increment, or %NULL
 * @page: (out) (allow-none): location to store page increment, or %NULL
 *
 * Gets the current step and page the increments used by @spin_button. See
 * btk_spin_button_set_increments().
 **/
void
btk_spin_button_get_increments (BtkSpinButton *spin_button,
				gdouble       *step,
				gdouble       *page)
{
  g_return_if_fail (BTK_IS_SPIN_BUTTON (spin_button));

  if (step)
    *step = spin_button->adjustment->step_increment;
  if (page)
    *page = spin_button->adjustment->page_increment;
}

/**
 * btk_spin_button_set_range:
 * @spin_button: a #BtkSpinButton
 * @min: minimum allowable value
 * @max: maximum allowable value
 * 
 * Sets the minimum and maximum allowable values for @spin_button
 **/
void
btk_spin_button_set_range (BtkSpinButton *spin_button,
			   gdouble        min,
			   gdouble        max)
{
  gdouble value;
  
  g_return_if_fail (BTK_IS_SPIN_BUTTON (spin_button));

  spin_button->adjustment->lower = min;
  spin_button->adjustment->upper = max;

  value = CLAMP (spin_button->adjustment->value,
                 spin_button->adjustment->lower,
                 (spin_button->adjustment->upper - spin_button->adjustment->page_size));

  if (value != spin_button->adjustment->value)
    btk_spin_button_set_value (spin_button, value);

  btk_adjustment_changed (spin_button->adjustment);
}

/**
 * btk_spin_button_get_range:
 * @spin_button: a #BtkSpinButton
 * @min: (out) (allow-none): location to store minimum allowed value, or %NULL
 * @max: (out) (allow-none): location to store maximum allowed value, or %NULL
 *
 * Gets the range allowed for @spin_button. See
 * btk_spin_button_set_range().
 **/
void
btk_spin_button_get_range (BtkSpinButton *spin_button,
			   gdouble       *min,
			   gdouble       *max)
{
  g_return_if_fail (BTK_IS_SPIN_BUTTON (spin_button));

  if (min)
    *min = spin_button->adjustment->lower;
  if (max)
    *max = spin_button->adjustment->upper;
}

/**
 * btk_spin_button_get_value:
 * @spin_button: a #BtkSpinButton
 * 
 * Get the value in the @spin_button.
 * 
 * Return value: the value of @spin_button
 **/
gdouble
btk_spin_button_get_value (BtkSpinButton *spin_button)
{
  g_return_val_if_fail (BTK_IS_SPIN_BUTTON (spin_button), 0.0);

  return spin_button->adjustment->value;
}

/**
 * btk_spin_button_get_value_as_int:
 * @spin_button: a #BtkSpinButton
 * 
 * Get the value @spin_button represented as an integer.
 * 
 * Return value: the value of @spin_button
 **/
gint
btk_spin_button_get_value_as_int (BtkSpinButton *spin_button)
{
  gdouble val;

  g_return_val_if_fail (BTK_IS_SPIN_BUTTON (spin_button), 0);

  val = spin_button->adjustment->value;
  if (val - floor (val) < ceil (val) - val)
    return floor (val);
  else
    return ceil (val);
}

/**
 * btk_spin_button_set_value:
 * @spin_button: a #BtkSpinButton
 * @value: the new value
 * 
 * Set the value of @spin_button.
 **/
void 
btk_spin_button_set_value (BtkSpinButton *spin_button, 
			   gdouble        value)
{
  g_return_if_fail (BTK_IS_SPIN_BUTTON (spin_button));

  if (fabs (value - spin_button->adjustment->value) > EPSILON)
    btk_adjustment_set_value (spin_button->adjustment, value);
  else
    {
      gint return_val = FALSE;
      g_signal_emit (spin_button, spinbutton_signals[OUTPUT], 0, &return_val);
      if (return_val == FALSE)
	btk_spin_button_default_output (spin_button);
    }
}

/**
 * btk_spin_button_set_update_policy:
 * @spin_button: a #BtkSpinButton 
 * @policy: a #BtkSpinButtonUpdatePolicy value
 * 
 * Sets the update behavior of a spin button. This determines whether the
 * spin button is always updated or only when a valid value is set.
 **/
void
btk_spin_button_set_update_policy (BtkSpinButton             *spin_button,
				   BtkSpinButtonUpdatePolicy  policy)
{
  g_return_if_fail (BTK_IS_SPIN_BUTTON (spin_button));

  if (spin_button->update_policy != policy)
    {
      spin_button->update_policy = policy;
      g_object_notify (G_OBJECT (spin_button), "update-policy");
    }
}

/**
 * btk_spin_button_get_update_policy:
 * @spin_button: a #BtkSpinButton
 *
 * Gets the update behavior of a spin button. See
 * btk_spin_button_set_update_policy().
 *
 * Return value: the current update policy
 **/
BtkSpinButtonUpdatePolicy
btk_spin_button_get_update_policy (BtkSpinButton *spin_button)
{
  g_return_val_if_fail (BTK_IS_SPIN_BUTTON (spin_button), BTK_UPDATE_ALWAYS);

  return spin_button->update_policy;
}

/**
 * btk_spin_button_set_numeric:
 * @spin_button: a #BtkSpinButton 
 * @numeric: flag indicating if only numeric entry is allowed. 
 * 
 * Sets the flag that determines if non-numeric text can be typed into
 * the spin button.
 **/
void
btk_spin_button_set_numeric (BtkSpinButton  *spin_button,
			     gboolean        numeric)
{
  g_return_if_fail (BTK_IS_SPIN_BUTTON (spin_button));

  numeric = numeric != FALSE;

  if (spin_button->numeric != numeric)
    {
       spin_button->numeric = numeric;
       g_object_notify (G_OBJECT (spin_button), "numeric");
    }
}

/**
 * btk_spin_button_get_numeric:
 * @spin_button: a #BtkSpinButton
 *
 * Returns whether non-numeric text can be typed into the spin button.
 * See btk_spin_button_set_numeric().
 *
 * Return value: %TRUE if only numeric text can be entered
 **/
gboolean
btk_spin_button_get_numeric (BtkSpinButton *spin_button)
{
  g_return_val_if_fail (BTK_IS_SPIN_BUTTON (spin_button), FALSE);

  return spin_button->numeric;
}

/**
 * btk_spin_button_set_wrap:
 * @spin_button: a #BtkSpinButton 
 * @wrap: a flag indicating if wrapping behavior is performed.
 * 
 * Sets the flag that determines if a spin button value wraps around to the
 * opposite limit when the upper or lower limit of the range is exceeded.
 **/
void
btk_spin_button_set_wrap (BtkSpinButton  *spin_button,
			  gboolean        wrap)
{
  g_return_if_fail (BTK_IS_SPIN_BUTTON (spin_button));

  wrap = wrap != FALSE; 

  if (spin_button->wrap != wrap)
    {
       spin_button->wrap = (wrap != 0);
  
       g_object_notify (G_OBJECT (spin_button), "wrap");
    }
}

/**
 * btk_spin_button_get_wrap:
 * @spin_button: a #BtkSpinButton
 *
 * Returns whether the spin button's value wraps around to the
 * opposite limit when the upper or lower limit of the range is
 * exceeded. See btk_spin_button_set_wrap().
 *
 * Return value: %TRUE if the spin button wraps around
 **/
gboolean
btk_spin_button_get_wrap (BtkSpinButton *spin_button)
{
  g_return_val_if_fail (BTK_IS_SPIN_BUTTON (spin_button), FALSE);

  return spin_button->wrap;
}

static gint
spin_button_get_arrow_size (BtkSpinButton *spin_button)
{
  gint size = bango_font_description_get_size (BTK_WIDGET (spin_button)->style->font_desc);
  gint arrow_size;

  arrow_size = MAX (BANGO_PIXELS (size), MIN_ARROW_WIDTH);

  return arrow_size - arrow_size % 2; /* force even */
}

/**
 * spin_button_get_shadow_type:
 * @spin_button: a #BtkSpinButton 
 * 
 * Convenience function to Get the shadow type from the underlying widget's
 * style.
 * 
 * Return value: the #BtkShadowType
 **/
static gint
spin_button_get_shadow_type (BtkSpinButton *spin_button)
{
  BtkShadowType rc_shadow_type;

  btk_widget_style_get (BTK_WIDGET (spin_button), "shadow-type", &rc_shadow_type, NULL);

  return rc_shadow_type;
}

/**
 * btk_spin_button_set_snap_to_ticks:
 * @spin_button: a #BtkSpinButton 
 * @snap_to_ticks: a flag indicating if invalid values should be corrected.
 * 
 * Sets the policy as to whether values are corrected to the nearest step 
 * increment when a spin button is activated after providing an invalid value.
 **/
void
btk_spin_button_set_snap_to_ticks (BtkSpinButton *spin_button,
				   gboolean       snap_to_ticks)
{
  guint new_val;

  g_return_if_fail (BTK_IS_SPIN_BUTTON (spin_button));

  new_val = (snap_to_ticks != 0);

  if (new_val != spin_button->snap_to_ticks)
    {
      spin_button->snap_to_ticks = new_val;
      if (new_val && BTK_ENTRY (spin_button)->editable)
	btk_spin_button_update (spin_button);
      
      g_object_notify (G_OBJECT (spin_button), "snap-to-ticks");
    }
}

/**
 * btk_spin_button_get_snap_to_ticks:
 * @spin_button: a #BtkSpinButton
 *
 * Returns whether the values are corrected to the nearest step. See
 * btk_spin_button_set_snap_to_ticks().
 *
 * Return value: %TRUE if values are snapped to the nearest step.
 **/
gboolean
btk_spin_button_get_snap_to_ticks (BtkSpinButton *spin_button)
{
  g_return_val_if_fail (BTK_IS_SPIN_BUTTON (spin_button), FALSE);

  return spin_button->snap_to_ticks;
}

/**
 * btk_spin_button_spin:
 * @spin_button: a #BtkSpinButton 
 * @direction: a #BtkSpinType indicating the direction to spin.
 * @increment: step increment to apply in the specified direction.
 * 
 * Increment or decrement a spin button's value in a specified direction
 * by a specified amount. 
 **/
void
btk_spin_button_spin (BtkSpinButton *spin_button,
		      BtkSpinType    direction,
		      gdouble        increment)
{
  BtkAdjustment *adj;
  gdouble diff;

  g_return_if_fail (BTK_IS_SPIN_BUTTON (spin_button));
  
  adj = spin_button->adjustment;

  /* for compatibility with the 1.0.x version of this function */
  if (increment != 0 && increment != adj->step_increment &&
      (direction == BTK_SPIN_STEP_FORWARD ||
       direction == BTK_SPIN_STEP_BACKWARD))
    {
      if (direction == BTK_SPIN_STEP_BACKWARD && increment > 0)
	increment = -increment;
      direction = BTK_SPIN_USER_DEFINED;
    }

  switch (direction)
    {
    case BTK_SPIN_STEP_FORWARD:

      btk_spin_button_real_spin (spin_button, adj->step_increment);
      break;

    case BTK_SPIN_STEP_BACKWARD:

      btk_spin_button_real_spin (spin_button, -adj->step_increment);
      break;

    case BTK_SPIN_PAGE_FORWARD:

      btk_spin_button_real_spin (spin_button, adj->page_increment);
      break;

    case BTK_SPIN_PAGE_BACKWARD:

      btk_spin_button_real_spin (spin_button, -adj->page_increment);
      break;

    case BTK_SPIN_HOME:

      diff = adj->value - adj->lower;
      if (diff > EPSILON)
	btk_spin_button_real_spin (spin_button, -diff);
      break;

    case BTK_SPIN_END:

      diff = adj->upper - adj->value;
      if (diff > EPSILON)
	btk_spin_button_real_spin (spin_button, diff);
      break;

    case BTK_SPIN_USER_DEFINED:

      if (increment != 0)
	btk_spin_button_real_spin (spin_button, increment);
      break;

    default:
      break;
    }
}

/**
 * btk_spin_button_update:
 * @spin_button: a #BtkSpinButton 
 * 
 * Manually force an update of the spin button.
 **/
void 
btk_spin_button_update (BtkSpinButton *spin_button)
{
  gdouble val;
  gint error = 0;
  gint return_val;

  g_return_if_fail (BTK_IS_SPIN_BUTTON (spin_button));

  return_val = FALSE;
  g_signal_emit (spin_button, spinbutton_signals[INPUT], 0, &val, &return_val);
  if (return_val == FALSE)
    {
      return_val = btk_spin_button_default_input (spin_button, &val);
      error = (return_val == BTK_INPUT_ERROR);
    }
  else if (return_val == BTK_INPUT_ERROR)
    error = 1;

  btk_widget_queue_draw (BTK_WIDGET (spin_button));

  if (spin_button->update_policy == BTK_UPDATE_ALWAYS)
    {
      if (val < spin_button->adjustment->lower)
	val = spin_button->adjustment->lower;
      else if (val > spin_button->adjustment->upper)
	val = spin_button->adjustment->upper;
    }
  else if ((spin_button->update_policy == BTK_UPDATE_IF_VALID) && 
	   (error ||
	   val < spin_button->adjustment->lower ||
	   val > spin_button->adjustment->upper))
    {
      btk_spin_button_value_changed (spin_button->adjustment, spin_button);
      return;
    }

  if (spin_button->snap_to_ticks)
    btk_spin_button_snap (spin_button, val);
  else
    btk_spin_button_set_value (spin_button, val);
}

#define __BTK_SPIN_BUTTON_C__
#include "btkaliasdef.c"
