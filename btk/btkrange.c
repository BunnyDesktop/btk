/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 2001 Red Hat, Inc.
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
 * Modified by the BTK+ Team and others 1997-2004.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"

#include <stdio.h>
#include <math.h>

#undef BTK_DISABLE_DEPRECATED

#include <bdk/bdkkeysyms.h>
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkorientable.h"
#include "btkrange.h"
#include "btkscale.h"
#include "btkscrollbar.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

#define SCROLL_DELAY_FACTOR 5    /* Scroll repeat multiplier */
#define UPDATE_DELAY        300  /* Delay for queued update */

enum {
  PROP_0,
  PROP_ORIENTATION,
  PROP_UPDATE_POLICY,
  PROP_ADJUSTMENT,
  PROP_INVERTED,
  PROP_LOWER_STEPPER_SENSITIVITY,
  PROP_UPPER_STEPPER_SENSITIVITY,
  PROP_SHOW_FILL_LEVEL,
  PROP_RESTRICT_TO_FILL_LEVEL,
  PROP_FILL_LEVEL,
  PROP_ROUND_DIGITS
};

enum {
  VALUE_CHANGED,
  ADJUST_BOUNDS,
  MOVE_SLIDER,
  CHANGE_VALUE,
  LAST_SIGNAL
};

typedef enum {
  MOUSE_OUTSIDE,
  MOUSE_STEPPER_A,
  MOUSE_STEPPER_B,
  MOUSE_STEPPER_C,
  MOUSE_STEPPER_D,
  MOUSE_TROUGH,
  MOUSE_SLIDER,
  MOUSE_WIDGET /* inside widget but not in any of the above GUI elements */
} MouseLocation;

typedef enum {
  STEPPER_A,
  STEPPER_B,
  STEPPER_C,
  STEPPER_D
} Stepper;

#define BTK_RANGE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_RANGE, BtkRangeLayout))

struct _BtkRangeLayout
{
  /* These are in widget->window coordinates */
  BdkRectangle stepper_a;
  BdkRectangle stepper_b;
  BdkRectangle stepper_c;
  BdkRectangle stepper_d;
  /* The trough rectangle is the area the thumb can slide in, not the
   * entire range_rect
   */
  BdkRectangle trough;
  BdkRectangle slider;

  /* Layout-related state */

  MouseLocation mouse_location;
  /* last mouse coords we got, or -1 if mouse is outside the range */
  gint mouse_x;
  gint mouse_y;

  /* "grabbed" mouse location, OUTSIDE for no grab */
  MouseLocation grab_location;
  guint grab_button : 8; /* 0 if none */

  /* Stepper sensitivity */
  guint lower_sensitive : 1;
  guint upper_sensitive : 1;

  /* Fill level */
  guint show_fill_level : 1;
  guint restrict_to_fill_level : 1;

  BtkSensitivityType lower_sensitivity;
  BtkSensitivityType upper_sensitivity;
  guint repaint_id;

  gdouble fill_level;

  GQuark slider_detail_quark;
  GQuark stepper_detail_quark[4];

  gdouble *marks;
  gint *mark_pos;
  gint n_marks;
  gboolean recalc_marks;
};


static void btk_range_set_property   (GObject          *object,
                                      guint             prop_id,
                                      const GValue     *value,
                                      GParamSpec       *pspec);
static void btk_range_get_property   (GObject          *object,
                                      guint             prop_id,
                                      GValue           *value,
                                      GParamSpec       *pspec);
static void btk_range_destroy        (BtkObject        *object);
static void btk_range_size_request   (BtkWidget        *widget,
                                      BtkRequisition   *requisition);
static void btk_range_size_allocate  (BtkWidget        *widget,
                                      BtkAllocation    *allocation);
static void btk_range_realize        (BtkWidget        *widget);
static void btk_range_unrealize      (BtkWidget        *widget);
static void btk_range_map            (BtkWidget        *widget);
static void btk_range_unmap          (BtkWidget        *widget);
static gboolean btk_range_expose         (BtkWidget        *widget,
                                      BdkEventExpose   *event);
static gboolean btk_range_button_press   (BtkWidget        *widget,
                                      BdkEventButton   *event);
static gboolean btk_range_button_release (BtkWidget        *widget,
                                      BdkEventButton   *event);
static gboolean btk_range_motion_notify  (BtkWidget        *widget,
                                      BdkEventMotion   *event);
static gboolean btk_range_enter_notify   (BtkWidget        *widget,
                                      BdkEventCrossing *event);
static gboolean btk_range_leave_notify   (BtkWidget        *widget,
                                      BdkEventCrossing *event);
static gboolean btk_range_grab_broken (BtkWidget          *widget,
				       BdkEventGrabBroken *event);
static void btk_range_grab_notify    (BtkWidget          *widget,
				      gboolean            was_grabbed);
static void btk_range_state_changed  (BtkWidget          *widget,
				      BtkStateType        previous_state);
static gboolean btk_range_scroll_event   (BtkWidget        *widget,
                                      BdkEventScroll   *event);
static void btk_range_style_set      (BtkWidget        *widget,
                                      BtkStyle         *previous_style);
static void update_slider_position   (BtkRange	       *range,
				      gint              mouse_x,
				      gint              mouse_y);
static void stop_scrolling           (BtkRange         *range);

/* Range methods */

static void btk_range_move_slider              (BtkRange         *range,
                                                BtkScrollType     scroll);

/* Internals */
static gboolean      btk_range_scroll                   (BtkRange      *range,
                                                         BtkScrollType  scroll);
static gboolean      btk_range_update_mouse_location    (BtkRange      *range);
static void          btk_range_calc_layout              (BtkRange      *range,
							 gdouble	adjustment_value);
static void          btk_range_calc_marks               (BtkRange      *range);
static void          btk_range_get_props                (BtkRange      *range,
                                                         gint          *slider_width,
                                                         gint          *stepper_size,
                                                         gint          *focus_width,
                                                         gint          *trough_border,
                                                         gint          *stepper_spacing,
                                                         gboolean      *trough_under_steppers,
							 gint          *arrow_displacement_x,
							 gint	       *arrow_displacement_y);
static void          btk_range_calc_request             (BtkRange      *range,
                                                         gint           slider_width,
                                                         gint           stepper_size,
                                                         gint           focus_width,
                                                         gint           trough_border,
                                                         gint           stepper_spacing,
                                                         BdkRectangle  *range_rect,
                                                         BtkBorder     *border,
                                                         gint          *n_steppers_p,
                                                         gboolean      *has_steppers_ab,
                                                         gboolean      *has_steppers_cd,
                                                         gint          *slider_length_p);
static void          btk_range_adjustment_value_changed (BtkAdjustment *adjustment,
                                                         gpointer       data);
static void          btk_range_adjustment_changed       (BtkAdjustment *adjustment,
                                                         gpointer       data);
static void          btk_range_add_step_timer           (BtkRange      *range,
                                                         BtkScrollType  step);
static void          btk_range_remove_step_timer        (BtkRange      *range);
static void          btk_range_reset_update_timer       (BtkRange      *range);
static void          btk_range_remove_update_timer      (BtkRange      *range);
static BdkRectangle* get_area                           (BtkRange      *range,
                                                         MouseLocation  location);
static gboolean      btk_range_real_change_value        (BtkRange      *range,
                                                         BtkScrollType  scroll,
                                                         gdouble        value);
static void          btk_range_update_value             (BtkRange      *range);
static gboolean      btk_range_key_press                (BtkWidget     *range,
							 BdkEventKey   *event);


G_DEFINE_ABSTRACT_TYPE_WITH_CODE (BtkRange, btk_range, BTK_TYPE_WIDGET,
                                  G_IMPLEMENT_INTERFACE (BTK_TYPE_ORIENTABLE,
                                                         NULL))

static guint signals[LAST_SIGNAL];


static void
btk_range_class_init (BtkRangeClass *class)
{
  GObjectClass   *bobject_class;
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;

  bobject_class = G_OBJECT_CLASS (class);
  object_class = (BtkObjectClass*) class;
  widget_class = (BtkWidgetClass*) class;

  bobject_class->set_property = btk_range_set_property;
  bobject_class->get_property = btk_range_get_property;

  object_class->destroy = btk_range_destroy;

  widget_class->size_request = btk_range_size_request;
  widget_class->size_allocate = btk_range_size_allocate;
  widget_class->realize = btk_range_realize;
  widget_class->unrealize = btk_range_unrealize;  
  widget_class->map = btk_range_map;
  widget_class->unmap = btk_range_unmap;
  widget_class->expose_event = btk_range_expose;
  widget_class->button_press_event = btk_range_button_press;
  widget_class->button_release_event = btk_range_button_release;
  widget_class->motion_notify_event = btk_range_motion_notify;
  widget_class->scroll_event = btk_range_scroll_event;
  widget_class->enter_notify_event = btk_range_enter_notify;
  widget_class->leave_notify_event = btk_range_leave_notify;
  widget_class->grab_broken_event = btk_range_grab_broken;
  widget_class->grab_notify = btk_range_grab_notify;
  widget_class->state_changed = btk_range_state_changed;
  widget_class->style_set = btk_range_style_set;
  widget_class->key_press_event = btk_range_key_press;

  class->move_slider = btk_range_move_slider;
  class->change_value = btk_range_real_change_value;

  class->slider_detail = "slider";
  class->stepper_detail = "stepper";

  /**
   * BtkRange::value-changed:
   * @range: the #BtkRange
   *
   * Emitted when the range value changes.
   */
  signals[VALUE_CHANGED] =
    g_signal_new (I_("value-changed"),
                  G_TYPE_FROM_CLASS (bobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkRangeClass, value_changed),
                  NULL, NULL,
                  _btk_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  
  signals[ADJUST_BOUNDS] =
    g_signal_new (I_("adjust-bounds"),
                  G_TYPE_FROM_CLASS (bobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkRangeClass, adjust_bounds),
                  NULL, NULL,
                  _btk_marshal_VOID__DOUBLE,
                  G_TYPE_NONE, 1,
                  G_TYPE_DOUBLE);
  
  /**
   * BtkRange::move-slider:
   * @range: the #BtkRange
   * @step: how to move the slider
   *
   * Virtual function that moves the slider. Used for keybindings.
   */
  signals[MOVE_SLIDER] =
    g_signal_new (I_("move-slider"),
                  G_TYPE_FROM_CLASS (bobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BtkRangeClass, move_slider),
                  NULL, NULL,
                  _btk_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  BTK_TYPE_SCROLL_TYPE);

  /**
   * BtkRange::change-value:
   * @range: the range that received the signal
   * @scroll: the type of scroll action that was performed
   * @value: the new value resulting from the scroll action
   * @returns: %TRUE to prevent other handlers from being invoked for the
   * signal, %FALSE to propagate the signal further
   *
   * The ::change-value signal is emitted when a scroll action is
   * performed on a range.  It allows an application to determine the
   * type of scroll event that occurred and the resultant new value.
   * The application can handle the event itself and return %TRUE to
   * prevent further processing.  Or, by returning %FALSE, it can pass
   * the event to other handlers until the default BTK+ handler is
   * reached.
   *
   * The value parameter is unrounded.  An application that overrides
   * the ::change-value signal is responsible for clamping the value to
   * the desired number of decimal digits; the default BTK+ handler
   * clamps the value based on #BtkRange:round_digits.
   *
   * It is not possible to use delayed update policies in an overridden
   * ::change-value handler.
   *
   * Since: 2.6
   */
  signals[CHANGE_VALUE] =
    g_signal_new (I_("change-value"),
                  G_TYPE_FROM_CLASS (bobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkRangeClass, change_value),
                  _btk_boolean_handled_accumulator, NULL,
                  _btk_marshal_BOOLEAN__ENUM_DOUBLE,
                  G_TYPE_BOOLEAN, 2,
                  BTK_TYPE_SCROLL_TYPE,
                  G_TYPE_DOUBLE);

  g_object_class_override_property (bobject_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  g_object_class_install_property (bobject_class,
                                   PROP_UPDATE_POLICY,
                                   g_param_spec_enum ("update-policy",
						      P_("Update policy"),
						      P_("How the range should be updated on the screen"),
						      BTK_TYPE_UPDATE_TYPE,
						      BTK_UPDATE_CONTINUOUS,
						      BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_ADJUSTMENT,
                                   g_param_spec_object ("adjustment",
							P_("Adjustment"),
							P_("The BtkAdjustment that contains the current value of this range object"),
                                                        BTK_TYPE_ADJUSTMENT,
                                                        BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (bobject_class,
                                   PROP_INVERTED,
                                   g_param_spec_boolean ("inverted",
							P_("Inverted"),
							P_("Invert direction slider moves to increase range value"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_LOWER_STEPPER_SENSITIVITY,
                                   g_param_spec_enum ("lower-stepper-sensitivity",
						      P_("Lower stepper sensitivity"),
						      P_("The sensitivity policy for the stepper that points to the adjustment's lower side"),
						      BTK_TYPE_SENSITIVITY_TYPE,
						      BTK_SENSITIVITY_AUTO,
						      BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_UPPER_STEPPER_SENSITIVITY,
                                   g_param_spec_enum ("upper-stepper-sensitivity",
						      P_("Upper stepper sensitivity"),
						      P_("The sensitivity policy for the stepper that points to the adjustment's upper side"),
						      BTK_TYPE_SENSITIVITY_TYPE,
						      BTK_SENSITIVITY_AUTO,
						      BTK_PARAM_READWRITE));

  /**
   * BtkRange:show-fill-level:
   *
   * The show-fill-level property controls whether fill level indicator
   * graphics are displayed on the trough. See
   * btk_range_set_show_fill_level().
   *
   * Since: 2.12
   **/
  g_object_class_install_property (bobject_class,
                                   PROP_SHOW_FILL_LEVEL,
                                   g_param_spec_boolean ("show-fill-level",
                                                         P_("Show Fill Level"),
                                                         P_("Whether to display a fill level indicator graphics on trough."),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));

  /**
   * BtkRange:restrict-to-fill-level:
   *
   * The restrict-to-fill-level property controls whether slider
   * movement is restricted to an upper boundary set by the
   * fill level. See btk_range_set_restrict_to_fill_level().
   *
   * Since: 2.12
   **/
  g_object_class_install_property (bobject_class,
                                   PROP_RESTRICT_TO_FILL_LEVEL,
                                   g_param_spec_boolean ("restrict-to-fill-level",
                                                         P_("Restrict to Fill Level"),
                                                         P_("Whether to restrict the upper boundary to the fill level."),
                                                         TRUE,
                                                         BTK_PARAM_READWRITE));

  /**
   * BtkRange:fill-level:
   *
   * The fill level (e.g. prebuffering of a network stream).
   * See btk_range_set_fill_level().
   *
   * Since: 2.12
   **/
  g_object_class_install_property (bobject_class,
                                   PROP_FILL_LEVEL,
                                   g_param_spec_double ("fill-level",
							P_("Fill Level"),
							P_("The fill level."),
							-G_MAXDOUBLE,
							G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkRange:round-digits:
   *
   * The number of digits to round the value to when
   * it changes, or -1. See #BtkRange::change-value.
   *
   * Since: 2.24
   */
  g_object_class_install_property (bobject_class,
                                   PROP_ROUND_DIGITS,
                                   g_param_spec_int ("round-digits",
                                                     P_("Round Digits"),
                                                     P_("The number of digits to round the value to."),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     BTK_PARAM_READWRITE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("slider-width",
							     P_("Slider Width"),
							     P_("Width of scrollbar or scale thumb"),
							     0,
							     G_MAXINT,
							     14,
							     BTK_PARAM_READABLE));
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("trough-border",
                                                             P_("Trough Border"),
                                                             P_("Spacing between thumb/steppers and outer trough bevel"),
                                                             0,
                                                             G_MAXINT,
                                                             1,
                                                             BTK_PARAM_READABLE));
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("stepper-size",
							     P_("Stepper Size"),
							     P_("Length of step buttons at ends"),
							     0,
							     G_MAXINT,
							     14,
							     BTK_PARAM_READABLE));
  /**
   * BtkRange:stepper-spacing:
   *
   * The spacing between the stepper buttons and thumb. Note that
   * setting this value to anything > 0 will automatically set the
   * trough-under-steppers style property to %TRUE as well. Also,
   * stepper-spacing won't have any effect if there are no steppers.
   */
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("stepper-spacing",
							     P_("Stepper Spacing"),
							     P_("Spacing between step buttons and thumb"),
                                                             0,
							     G_MAXINT,
							     0,
							     BTK_PARAM_READABLE));
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("arrow-displacement-x",
							     P_("Arrow X Displacement"),
							     P_("How far in the x direction to move the arrow when the button is depressed"),
							     G_MININT,
							     G_MAXINT,
							     0,
							     BTK_PARAM_READABLE));
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("arrow-displacement-y",
							     P_("Arrow Y Displacement"),
							     P_("How far in the y direction to move the arrow when the button is depressed"),
							     G_MININT,
							     G_MAXINT,
							     0,
							     BTK_PARAM_READABLE));

  /**
   * BtkRange:activate-slider:
   *
   * When %TRUE, sliders will be drawn active and with shadow in
   * while they are dragged.
   *
   * Deprecated: 2.22: This style property will be removed in BTK+ 3
   */
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("activate-slider",
                                                                 P_("Draw slider ACTIVE during drag"),
							         P_("With this option set to TRUE, sliders will be drawn ACTIVE and with shadow IN while they are dragged"),
							         FALSE,
							         BTK_PARAM_READABLE));

  /**
   * BtkRange:trough-side-details:
   *
   * When %TRUE, the parts of the trough on the two sides of the 
   * slider are drawn with different details.
   *
   * Since: 2.10
   *
   * Deprecated: 2.22: This style property will be removed in BTK+ 3
   */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boolean ("trough-side-details",
                                                                 P_("Trough Side Details"),
                                                                 P_("When TRUE, the parts of the trough on the two sides of the slider are drawn with different details"),
                                                                 FALSE,
                                                                 BTK_PARAM_READABLE));

  /**
   * BtkRange:trough-under-steppers:
   *
   * Whether to draw the trough across the full length of the range or
   * to exclude the steppers and their spacing. Note that setting the
   * #BtkRange:stepper-spacing style property to any value > 0 will
   * automatically enable trough-under-steppers too.
   *
   * Since: 2.10
   */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boolean ("trough-under-steppers",
                                                                 P_("Trough Under Steppers"),
                                                                 P_("Whether to draw trough for full length of range or exclude the steppers and spacing"),
                                                                 TRUE,
                                                                 BTK_PARAM_READABLE));

  /**
   * BtkRange:arrow-scaling:
   *
   * The arrow size proportion relative to the scroll button size.
   *
   * Since: 2.14
   */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_float ("arrow-scaling",
							       P_("Arrow scaling"),
							       P_("Arrow scaling with regard to scroll button size"),
							       0.0, 1.0, 0.5,
							       BTK_PARAM_READABLE));

  /**
   * BtkRange:stepper-position-details:
   *
   * When %TRUE, the detail string for rendering the steppers will be
   * suffixed with information about the stepper position.
   *
   * Since: 2.22
   *
   * Deprecated: 2.22: This style property will be removed in BTK+ 3
   */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boolean ("stepper-position-details",
                                                                 P_("Stepper Position Details"),
                                                                 P_("When TRUE, the detail string for rendering the steppers is suffixed with position information"),
                                                                 FALSE,
                                                                 BTK_PARAM_READABLE));

  g_type_class_add_private (class, sizeof (BtkRangeLayout));
}

static void
btk_range_set_property (GObject      *object,
			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  BtkRange *range = BTK_RANGE (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      range->orientation = g_value_get_enum (value);

      range->layout->slider_detail_quark = 0;
      range->layout->stepper_detail_quark[0] = 0;
      range->layout->stepper_detail_quark[1] = 0;
      range->layout->stepper_detail_quark[2] = 0;
      range->layout->stepper_detail_quark[3] = 0;

      btk_widget_queue_resize (BTK_WIDGET (range));
      break;
    case PROP_UPDATE_POLICY:
      btk_range_set_update_policy (range, g_value_get_enum (value));
      break;
    case PROP_ADJUSTMENT:
      btk_range_set_adjustment (range, g_value_get_object (value));
      break;
    case PROP_INVERTED:
      btk_range_set_inverted (range, g_value_get_boolean (value));
      break;
    case PROP_LOWER_STEPPER_SENSITIVITY:
      btk_range_set_lower_stepper_sensitivity (range, g_value_get_enum (value));
      break;
    case PROP_UPPER_STEPPER_SENSITIVITY:
      btk_range_set_upper_stepper_sensitivity (range, g_value_get_enum (value));
      break;
    case PROP_SHOW_FILL_LEVEL:
      btk_range_set_show_fill_level (range, g_value_get_boolean (value));
      break;
    case PROP_RESTRICT_TO_FILL_LEVEL:
      btk_range_set_restrict_to_fill_level (range, g_value_get_boolean (value));
      break;
    case PROP_FILL_LEVEL:
      btk_range_set_fill_level (range, g_value_get_double (value));
      break;
    case PROP_ROUND_DIGITS:
      btk_range_set_round_digits (range, g_value_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_range_get_property (GObject      *object,
			guint         prop_id,
			GValue       *value,
			GParamSpec   *pspec)
{
  BtkRange *range = BTK_RANGE (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, range->orientation);
      break;
    case PROP_UPDATE_POLICY:
      g_value_set_enum (value, range->update_policy);
      break;
    case PROP_ADJUSTMENT:
      g_value_set_object (value, range->adjustment);
      break;
    case PROP_INVERTED:
      g_value_set_boolean (value, range->inverted);
      break;
    case PROP_LOWER_STEPPER_SENSITIVITY:
      g_value_set_enum (value, btk_range_get_lower_stepper_sensitivity (range));
      break;
    case PROP_UPPER_STEPPER_SENSITIVITY:
      g_value_set_enum (value, btk_range_get_upper_stepper_sensitivity (range));
      break;
    case PROP_SHOW_FILL_LEVEL:
      g_value_set_boolean (value, btk_range_get_show_fill_level (range));
      break;
    case PROP_RESTRICT_TO_FILL_LEVEL:
      g_value_set_boolean (value, btk_range_get_restrict_to_fill_level (range));
      break;
    case PROP_FILL_LEVEL:
      g_value_set_double (value, btk_range_get_fill_level (range));
      break;
    case PROP_ROUND_DIGITS:
      g_value_set_int (value, btk_range_get_round_digits (range));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_range_init (BtkRange *range)
{
  btk_widget_set_has_window (BTK_WIDGET (range), FALSE);

  range->orientation = BTK_ORIENTATION_HORIZONTAL;
  range->adjustment = NULL;
  range->update_policy = BTK_UPDATE_CONTINUOUS;
  range->inverted = FALSE;
  range->flippable = FALSE;
  range->min_slider_size = 1;
  range->has_stepper_a = FALSE;
  range->has_stepper_b = FALSE;
  range->has_stepper_c = FALSE;
  range->has_stepper_d = FALSE;
  range->need_recalc = TRUE;
  range->round_digits = -1;
  range->layout = BTK_RANGE_GET_PRIVATE (range);
  range->layout->mouse_location = MOUSE_OUTSIDE;
  range->layout->mouse_x = -1;
  range->layout->mouse_y = -1;
  range->layout->grab_location = MOUSE_OUTSIDE;
  range->layout->grab_button = 0;
  range->layout->lower_sensitivity = BTK_SENSITIVITY_AUTO;
  range->layout->upper_sensitivity = BTK_SENSITIVITY_AUTO;
  range->layout->lower_sensitive = TRUE;
  range->layout->upper_sensitive = TRUE;
  range->layout->show_fill_level = FALSE;
  range->layout->restrict_to_fill_level = TRUE;
  range->layout->fill_level = G_MAXDOUBLE;
  range->timer = NULL;  
}

/**
 * btk_range_get_adjustment:
 * @range: a #BtkRange
 * 
 * Get the #BtkAdjustment which is the "model" object for #BtkRange.
 * See btk_range_set_adjustment() for details.
 * The return value does not have a reference added, so should not
 * be unreferenced.
 * 
 * Return value: (transfer none): a #BtkAdjustment
 **/
BtkAdjustment*
btk_range_get_adjustment (BtkRange *range)
{
  g_return_val_if_fail (BTK_IS_RANGE (range), NULL);

  if (!range->adjustment)
    btk_range_set_adjustment (range, NULL);

  return range->adjustment;
}

/**
 * btk_range_set_update_policy:
 * @range: a #BtkRange
 * @policy: update policy
 *
 * Sets the update policy for the range. #BTK_UPDATE_CONTINUOUS means that
 * anytime the range slider is moved, the range value will change and the
 * value_changed signal will be emitted. #BTK_UPDATE_DELAYED means that
 * the value will be updated after a brief timeout where no slider motion
 * occurs, so updates are spaced by a short time rather than
 * continuous. #BTK_UPDATE_DISCONTINUOUS means that the value will only
 * be updated when the user releases the button and ends the slider
 * drag operation.
 *
 * Deprecated: 2.24: There is no replacement. If you require delayed
 *   updates, you need to code it yourself.
 **/
void
btk_range_set_update_policy (BtkRange      *range,
			     BtkUpdateType  policy)
{
  g_return_if_fail (BTK_IS_RANGE (range));

  if (range->update_policy != policy)
    {
      range->update_policy = policy;
      g_object_notify (G_OBJECT (range), "update-policy");
    }
}

/**
 * btk_range_get_update_policy:
 * @range: a #BtkRange
 *
 * Gets the update policy of @range. See btk_range_set_update_policy().
 *
 * Return value: the current update policy
 *
 * Deprecated: 2.24: There is no replacement. If you require delayed
 *   updates, you need to code it yourself.
 **/
BtkUpdateType
btk_range_get_update_policy (BtkRange *range)
{
  g_return_val_if_fail (BTK_IS_RANGE (range), BTK_UPDATE_CONTINUOUS);

  return range->update_policy;
}

/**
 * btk_range_set_adjustment:
 * @range: a #BtkRange
 * @adjustment: a #BtkAdjustment
 *
 * Sets the adjustment to be used as the "model" object for this range
 * widget. The adjustment indicates the current range value, the
 * minimum and maximum range values, the step/page increments used
 * for keybindings and scrolling, and the page size. The page size
 * is normally 0 for #BtkScale and nonzero for #BtkScrollbar, and
 * indicates the size of the visible area of the widget being scrolled.
 * The page size affects the size of the scrollbar slider.
 **/
void
btk_range_set_adjustment (BtkRange      *range,
			  BtkAdjustment *adjustment)
{
  g_return_if_fail (BTK_IS_RANGE (range));
  
  if (!adjustment)
    adjustment = (BtkAdjustment*) btk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  else
    g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));

  if (range->adjustment != adjustment)
    {
      if (range->adjustment)
	{
	  g_signal_handlers_disconnect_by_func (range->adjustment,
						btk_range_adjustment_changed,
						range);
	  g_signal_handlers_disconnect_by_func (range->adjustment,
						btk_range_adjustment_value_changed,
						range);
	  g_object_unref (range->adjustment);
	}

      range->adjustment = adjustment;
      g_object_ref_sink (adjustment);
      
      g_signal_connect (adjustment, "changed",
			G_CALLBACK (btk_range_adjustment_changed),
			range);
      g_signal_connect (adjustment, "value-changed",
			G_CALLBACK (btk_range_adjustment_value_changed),
			range);
      
      btk_range_adjustment_changed (adjustment, range);
      g_object_notify (G_OBJECT (range), "adjustment");
    }
}

/**
 * btk_range_set_inverted:
 * @range: a #BtkRange
 * @setting: %TRUE to invert the range
 *
 * Ranges normally move from lower to higher values as the
 * slider moves from top to bottom or left to right. Inverted
 * ranges have higher values at the top or on the right rather than
 * on the bottom or left.
 **/
void
btk_range_set_inverted (BtkRange *range,
                        gboolean  setting)
{
  g_return_if_fail (BTK_IS_RANGE (range));
  
  setting = setting != FALSE;

  if (setting != range->inverted)
    {
      range->inverted = setting;
      g_object_notify (G_OBJECT (range), "inverted");
      btk_widget_queue_resize (BTK_WIDGET (range));
    }
}

/**
 * btk_range_get_inverted:
 * @range: a #BtkRange
 * 
 * Gets the value set by btk_range_set_inverted().
 * 
 * Return value: %TRUE if the range is inverted
 **/
gboolean
btk_range_get_inverted (BtkRange *range)
{
  g_return_val_if_fail (BTK_IS_RANGE (range), FALSE);

  return range->inverted;
}

/**
 * btk_range_set_flippable:
 * @range: a #BtkRange
 * @flippable: %TRUE to make the range flippable
 *
 * If a range is flippable, it will switch its direction if it is
 * horizontal and its direction is %BTK_TEXT_DIR_RTL.
 *
 * See btk_widget_get_direction().
 *
 * Since: 2.18
 **/
void
btk_range_set_flippable (BtkRange *range,
                         gboolean  flippable)
{
  g_return_if_fail (BTK_IS_RANGE (range));

  flippable = flippable ? TRUE : FALSE;

  if (flippable != range->flippable)
    {
      range->flippable = flippable;

      btk_widget_queue_draw (BTK_WIDGET (range));
    }
}

/**
 * btk_range_get_flippable:
 * @range: a #BtkRange
 *
 * Gets the value set by btk_range_set_flippable().
 *
 * Return value: %TRUE if the range is flippable
 *
 * Since: 2.18
 **/
gboolean
btk_range_get_flippable (BtkRange *range)
{
  g_return_val_if_fail (BTK_IS_RANGE (range), FALSE);

  return range->flippable;
}

/**
 * btk_range_set_slider_size_fixed:
 * @range: a #BtkRange
 * @size_fixed: %TRUE to make the slider size constant
 *
 * Sets whether the range's slider has a fixed size, or a size that
 * depends on it's adjustment's page size.
 *
 * This function is useful mainly for #BtkRange subclasses.
 *
 * Since: 2.20
 **/
void
btk_range_set_slider_size_fixed (BtkRange *range,
                                 gboolean  size_fixed)
{
  g_return_if_fail (BTK_IS_RANGE (range));

  if (size_fixed != range->slider_size_fixed)
    {
      range->slider_size_fixed = size_fixed ? TRUE : FALSE;

      range->need_recalc = TRUE;
      btk_range_calc_layout (range, range->adjustment->value);
      btk_widget_queue_draw (BTK_WIDGET (range));
    }
}

/**
 * btk_range_get_slider_size_fixed:
 * @range: a #BtkRange
 *
 * This function is useful mainly for #BtkRange subclasses.
 *
 * See btk_range_set_slider_size_fixed().
 *
 * Return value: whether the range's slider has a fixed size.
 *
 * Since: 2.20
 **/
gboolean
btk_range_get_slider_size_fixed (BtkRange *range)
{
  g_return_val_if_fail (BTK_IS_RANGE (range), FALSE);

  return range->slider_size_fixed;
}

/**
 * btk_range_set_min_slider_size:
 * @range: a #BtkRange
 * @min_size: The slider's minimum size
 *
 * Sets the minimum size of the range's slider.
 *
 * This function is useful mainly for #BtkRange subclasses.
 *
 * Since: 2.20
 **/
void
btk_range_set_min_slider_size (BtkRange *range,
                               gboolean  min_size)
{
  g_return_if_fail (BTK_IS_RANGE (range));
  g_return_if_fail (min_size > 0);

  if (min_size != range->min_slider_size)
    {
      range->min_slider_size = min_size;

      range->need_recalc = TRUE;
      btk_range_calc_layout (range, range->adjustment->value);
      btk_widget_queue_draw (BTK_WIDGET (range));
    }
}

/**
 * btk_range_get_min_slider_size:
 * @range: a #BtkRange
 *
 * This function is useful mainly for #BtkRange subclasses.
 *
 * See btk_range_set_min_slider_size().
 *
 * Return value: The minimum size of the range's slider.
 *
 * Since: 2.20
 **/
gint
btk_range_get_min_slider_size (BtkRange *range)
{
  g_return_val_if_fail (BTK_IS_RANGE (range), FALSE);

  return range->min_slider_size;
}

/**
 * btk_range_get_range_rect:
 * @range: a #BtkRange
 * @range_rect: (out): return location for the range rectangle
 *
 * This function returns the area that contains the range's trough
 * and its steppers, in widget->window coordinates.
 *
 * This function is useful mainly for #BtkRange subclasses.
 *
 * Since: 2.20
 **/
void
btk_range_get_range_rect (BtkRange     *range,
                          BdkRectangle *range_rect)
{
  g_return_if_fail (BTK_IS_RANGE (range));
  g_return_if_fail (range_rect != NULL);

  btk_range_calc_layout (range, range->adjustment->value);

  *range_rect = range->range_rect;
}

/**
 * btk_range_get_slider_range:
 * @range: a #BtkRange
 * @slider_start: (out) (allow-none): return location for the slider's
 *     start, or %NULL
 * @slider_end: (out) (allow-none): return location for the slider's
 *     end, or %NULL
 *
 * This function returns sliders range along the long dimension,
 * in widget->window coordinates.
 *
 * This function is useful mainly for #BtkRange subclasses.
 *
 * Since: 2.20
 **/
void
btk_range_get_slider_range (BtkRange *range,
                            gint     *slider_start,
                            gint     *slider_end)
{
  g_return_if_fail (BTK_IS_RANGE (range));

  btk_range_calc_layout (range, range->adjustment->value);

  if (slider_start)
    *slider_start = range->slider_start;

  if (slider_end)
    *slider_end = range->slider_end;
}

/**
 * btk_range_set_lower_stepper_sensitivity:
 * @range:       a #BtkRange
 * @sensitivity: the lower stepper's sensitivity policy.
 *
 * Sets the sensitivity policy for the stepper that points to the
 * 'lower' end of the BtkRange's adjustment.
 *
 * Since: 2.10
 **/
void
btk_range_set_lower_stepper_sensitivity (BtkRange           *range,
                                         BtkSensitivityType  sensitivity)
{
  g_return_if_fail (BTK_IS_RANGE (range));

  if (range->layout->lower_sensitivity != sensitivity)
    {
      range->layout->lower_sensitivity = sensitivity;

      range->need_recalc = TRUE;
      btk_range_calc_layout (range, range->adjustment->value);
      btk_widget_queue_draw (BTK_WIDGET (range));

      g_object_notify (G_OBJECT (range), "lower-stepper-sensitivity");
    }
}

/**
 * btk_range_get_lower_stepper_sensitivity:
 * @range: a #BtkRange
 *
 * Gets the sensitivity policy for the stepper that points to the
 * 'lower' end of the BtkRange's adjustment.
 *
 * Return value: The lower stepper's sensitivity policy.
 *
 * Since: 2.10
 **/
BtkSensitivityType
btk_range_get_lower_stepper_sensitivity (BtkRange *range)
{
  g_return_val_if_fail (BTK_IS_RANGE (range), BTK_SENSITIVITY_AUTO);

  return range->layout->lower_sensitivity;
}

/**
 * btk_range_set_upper_stepper_sensitivity:
 * @range:       a #BtkRange
 * @sensitivity: the upper stepper's sensitivity policy.
 *
 * Sets the sensitivity policy for the stepper that points to the
 * 'upper' end of the BtkRange's adjustment.
 *
 * Since: 2.10
 **/
void
btk_range_set_upper_stepper_sensitivity (BtkRange           *range,
                                         BtkSensitivityType  sensitivity)
{
  g_return_if_fail (BTK_IS_RANGE (range));

  if (range->layout->upper_sensitivity != sensitivity)
    {
      range->layout->upper_sensitivity = sensitivity;

      range->need_recalc = TRUE;
      btk_range_calc_layout (range, range->adjustment->value);
      btk_widget_queue_draw (BTK_WIDGET (range));

      g_object_notify (G_OBJECT (range), "upper-stepper-sensitivity");
    }
}

/**
 * btk_range_get_upper_stepper_sensitivity:
 * @range: a #BtkRange
 *
 * Gets the sensitivity policy for the stepper that points to the
 * 'upper' end of the BtkRange's adjustment.
 *
 * Return value: The upper stepper's sensitivity policy.
 *
 * Since: 2.10
 **/
BtkSensitivityType
btk_range_get_upper_stepper_sensitivity (BtkRange *range)
{
  g_return_val_if_fail (BTK_IS_RANGE (range), BTK_SENSITIVITY_AUTO);

  return range->layout->upper_sensitivity;
}

/**
 * btk_range_set_increments:
 * @range: a #BtkRange
 * @step: step size
 * @page: page size
 *
 * Sets the step and page sizes for the range.
 * The step size is used when the user clicks the #BtkScrollbar
 * arrows or moves #BtkScale via arrow keys. The page size
 * is used for example when moving via Page Up or Page Down keys.
 **/
void
btk_range_set_increments (BtkRange *range,
                          gdouble   step,
                          gdouble   page)
{
  g_return_if_fail (BTK_IS_RANGE (range));

  range->adjustment->step_increment = step;
  range->adjustment->page_increment = page;

  btk_adjustment_changed (range->adjustment);
}

/**
 * btk_range_set_range:
 * @range: a #BtkRange
 * @min: minimum range value
 * @max: maximum range value
 * 
 * Sets the allowable values in the #BtkRange, and clamps the range
 * value to be between @min and @max. (If the range has a non-zero
 * page size, it is clamped between @min and @max - page-size.)
 **/
void
btk_range_set_range (BtkRange *range,
                     gdouble   min,
                     gdouble   max)
{
  gdouble value;
  
  g_return_if_fail (BTK_IS_RANGE (range));
  g_return_if_fail (min < max);
  
  range->adjustment->lower = min;
  range->adjustment->upper = max;

  value = range->adjustment->value;

  if (range->layout->restrict_to_fill_level)
    value = MIN (value, MAX (range->adjustment->lower,
                             range->layout->fill_level));

  value = CLAMP (value, range->adjustment->lower,
                 (range->adjustment->upper - range->adjustment->page_size));

  btk_adjustment_set_value (range->adjustment, value);
  btk_adjustment_changed (range->adjustment);
}

/**
 * btk_range_set_value:
 * @range: a #BtkRange
 * @value: new value of the range
 *
 * Sets the current value of the range; if the value is outside the
 * minimum or maximum range values, it will be clamped to fit inside
 * them. The range emits the #BtkRange::value-changed signal if the 
 * value changes.
 **/
void
btk_range_set_value (BtkRange *range,
                     gdouble   value)
{
  g_return_if_fail (BTK_IS_RANGE (range));

  if (range->layout->restrict_to_fill_level)
    value = MIN (value, MAX (range->adjustment->lower,
                             range->layout->fill_level));

  value = CLAMP (value, range->adjustment->lower,
                 (range->adjustment->upper - range->adjustment->page_size));

  btk_adjustment_set_value (range->adjustment, value);
}

/**
 * btk_range_get_value:
 * @range: a #BtkRange
 * 
 * Gets the current value of the range.
 * 
 * Return value: current value of the range.
 **/
gdouble
btk_range_get_value (BtkRange *range)
{
  g_return_val_if_fail (BTK_IS_RANGE (range), 0.0);

  return range->adjustment->value;
}

/**
 * btk_range_set_show_fill_level:
 * @range:           A #BtkRange
 * @show_fill_level: Whether a fill level indicator graphics is shown.
 *
 * Sets whether a graphical fill level is show on the trough. See
 * btk_range_set_fill_level() for a general description of the fill
 * level concept.
 *
 * Since: 2.12
 **/
void
btk_range_set_show_fill_level (BtkRange *range,
                               gboolean  show_fill_level)
{
  g_return_if_fail (BTK_IS_RANGE (range));

  show_fill_level = show_fill_level ? TRUE : FALSE;

  if (show_fill_level != range->layout->show_fill_level)
    {
      range->layout->show_fill_level = show_fill_level;
      g_object_notify (G_OBJECT (range), "show-fill-level");
      btk_widget_queue_draw (BTK_WIDGET (range));
    }
}

/**
 * btk_range_get_show_fill_level:
 * @range: A #BtkRange
 *
 * Gets whether the range displays the fill level graphically.
 *
 * Return value: %TRUE if @range shows the fill level.
 *
 * Since: 2.12
 **/
gboolean
btk_range_get_show_fill_level (BtkRange *range)
{
  g_return_val_if_fail (BTK_IS_RANGE (range), FALSE);

  return range->layout->show_fill_level;
}

/**
 * btk_range_set_restrict_to_fill_level:
 * @range:                  A #BtkRange
 * @restrict_to_fill_level: Whether the fill level restricts slider movement.
 *
 * Sets whether the slider is restricted to the fill level. See
 * btk_range_set_fill_level() for a general description of the fill
 * level concept.
 *
 * Since: 2.12
 **/
void
btk_range_set_restrict_to_fill_level (BtkRange *range,
                                      gboolean  restrict_to_fill_level)
{
  g_return_if_fail (BTK_IS_RANGE (range));

  restrict_to_fill_level = restrict_to_fill_level ? TRUE : FALSE;

  if (restrict_to_fill_level != range->layout->restrict_to_fill_level)
    {
      range->layout->restrict_to_fill_level = restrict_to_fill_level;
      g_object_notify (G_OBJECT (range), "restrict-to-fill-level");

      btk_range_set_value (range, btk_range_get_value (range));
    }
}

/**
 * btk_range_get_restrict_to_fill_level:
 * @range: A #BtkRange
 *
 * Gets whether the range is restricted to the fill level.
 *
 * Return value: %TRUE if @range is restricted to the fill level.
 *
 * Since: 2.12
 **/
gboolean
btk_range_get_restrict_to_fill_level (BtkRange *range)
{
  g_return_val_if_fail (BTK_IS_RANGE (range), FALSE);

  return range->layout->restrict_to_fill_level;
}

/**
 * btk_range_set_fill_level:
 * @range: a #BtkRange
 * @fill_level: the new position of the fill level indicator
 *
 * Set the new position of the fill level indicator.
 *
 * The "fill level" is probably best described by its most prominent
 * use case, which is an indicator for the amount of pre-buffering in
 * a streaming media player. In that use case, the value of the range
 * would indicate the current play position, and the fill level would
 * be the position up to which the file/stream has been downloaded.
 *
 * This amount of prebuffering can be displayed on the range's trough
 * and is themeable separately from the trough. To enable fill level
 * display, use btk_range_set_show_fill_level(). The range defaults
 * to not showing the fill level.
 *
 * Additionally, it's possible to restrict the range's slider position
 * to values which are smaller than the fill level. This is controller
 * by btk_range_set_restrict_to_fill_level() and is by default
 * enabled.
 *
 * Since: 2.12
 **/
void
btk_range_set_fill_level (BtkRange *range,
                          gdouble   fill_level)
{
  g_return_if_fail (BTK_IS_RANGE (range));

  if (fill_level != range->layout->fill_level)
    {
      range->layout->fill_level = fill_level;
      g_object_notify (G_OBJECT (range), "fill-level");

      if (range->layout->show_fill_level)
        btk_widget_queue_draw (BTK_WIDGET (range));

      if (range->layout->restrict_to_fill_level)
        btk_range_set_value (range, btk_range_get_value (range));
    }
}

/**
 * btk_range_get_fill_level:
 * @range: A #BtkRange
 *
 * Gets the current position of the fill level indicator.
 *
 * Return value: The current fill level
 *
 * Since: 2.12
 **/
gdouble
btk_range_get_fill_level (BtkRange *range)
{
  g_return_val_if_fail (BTK_IS_RANGE (range), 0.0);

  return range->layout->fill_level;
}

static gboolean
should_invert (BtkRange *range)
{  
  if (range->orientation == BTK_ORIENTATION_HORIZONTAL)
    return
      (range->inverted && !range->flippable) ||
      (range->inverted && range->flippable && btk_widget_get_direction (BTK_WIDGET (range)) == BTK_TEXT_DIR_LTR) ||
      (!range->inverted && range->flippable && btk_widget_get_direction (BTK_WIDGET (range)) == BTK_TEXT_DIR_RTL);
  else
    return range->inverted;
}

static void
btk_range_destroy (BtkObject *object)
{
  BtkRange *range = BTK_RANGE (object);

  btk_range_remove_step_timer (range);
  btk_range_remove_update_timer (range);

  if (range->layout->repaint_id)
    g_source_remove (range->layout->repaint_id);
  range->layout->repaint_id = 0;

  if (range->adjustment)
    {
      g_signal_handlers_disconnect_by_func (range->adjustment,
					    btk_range_adjustment_changed,
					    range);
      g_signal_handlers_disconnect_by_func (range->adjustment,
					    btk_range_adjustment_value_changed,
					    range);
      g_object_unref (range->adjustment);
      range->adjustment = NULL;
    }

  if (range->layout->n_marks)
    {
      g_free (range->layout->marks);
      range->layout->marks = NULL;
      g_free (range->layout->mark_pos);
      range->layout->mark_pos = NULL;
      range->layout->n_marks = 0;
    }

  BTK_OBJECT_CLASS (btk_range_parent_class)->destroy (object);
}

static void
btk_range_size_request (BtkWidget      *widget,
                        BtkRequisition *requisition)
{
  BtkRange *range;
  gint slider_width, stepper_size, focus_width, trough_border, stepper_spacing;
  BdkRectangle range_rect;
  BtkBorder border;
  
  range = BTK_RANGE (widget);
  
  btk_range_get_props (range,
                       &slider_width, &stepper_size,
                       &focus_width, &trough_border,
                       &stepper_spacing, NULL,
                       NULL, NULL);

  btk_range_calc_request (range, 
                          slider_width, stepper_size,
                          focus_width, trough_border, stepper_spacing,
                          &range_rect, &border, NULL, NULL, NULL, NULL);

  requisition->width = range_rect.width + border.left + border.right;
  requisition->height = range_rect.height + border.top + border.bottom;
}

static void
btk_range_size_allocate (BtkWidget     *widget,
                         BtkAllocation *allocation)
{
  BtkRange *range;

  range = BTK_RANGE (widget);

  widget->allocation = *allocation;
  
  range->layout->recalc_marks = TRUE;

  range->need_recalc = TRUE;
  btk_range_calc_layout (range, range->adjustment->value);

  if (btk_widget_get_realized (widget))
    bdk_window_move_resize (range->event_window,
			    widget->allocation.x,
			    widget->allocation.y,
			    widget->allocation.width,
			    widget->allocation.height);
}

static void
btk_range_realize (BtkWidget *widget)
{
  BtkRange *range;
  BdkWindowAttr attributes;
  gint attributes_mask;  

  range = BTK_RANGE (widget);

  btk_range_calc_layout (range, range->adjustment->value);
  
  btk_widget_set_realized (widget, TRUE);

  widget->window = btk_widget_get_parent_window (widget);
  g_object_ref (widget->window);
  
  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = BDK_INPUT_ONLY;
  attributes.event_mask = btk_widget_get_events (widget);
  attributes.event_mask |= (BDK_BUTTON_PRESS_MASK |
			    BDK_BUTTON_RELEASE_MASK |
			    BDK_ENTER_NOTIFY_MASK |
			    BDK_LEAVE_NOTIFY_MASK |
                            BDK_POINTER_MOTION_MASK |
                            BDK_POINTER_MOTION_HINT_MASK);

  attributes_mask = BDK_WA_X | BDK_WA_Y;

  range->event_window = bdk_window_new (btk_widget_get_parent_window (widget),
					&attributes, attributes_mask);
  bdk_window_set_user_data (range->event_window, range);

  widget->style = btk_style_attach (widget->style, widget->window);
}

static void
btk_range_unrealize (BtkWidget *widget)
{
  BtkRange *range = BTK_RANGE (widget);

  btk_range_remove_step_timer (range);
  btk_range_remove_update_timer (range);
  
  bdk_window_set_user_data (range->event_window, NULL);
  bdk_window_destroy (range->event_window);
  range->event_window = NULL;

  BTK_WIDGET_CLASS (btk_range_parent_class)->unrealize (widget);
}

static void
btk_range_map (BtkWidget *widget)
{
  BtkRange *range = BTK_RANGE (widget);
  
  bdk_window_show (range->event_window);

  BTK_WIDGET_CLASS (btk_range_parent_class)->map (widget);
}

static void
btk_range_unmap (BtkWidget *widget)
{
  BtkRange *range = BTK_RANGE (widget);
    
  stop_scrolling (range);

  bdk_window_hide (range->event_window);

  BTK_WIDGET_CLASS (btk_range_parent_class)->unmap (widget);
}

static const gchar *
btk_range_get_slider_detail (BtkRange *range)
{
  const gchar *slider_detail;

  if (range->layout->slider_detail_quark)
    return g_quark_to_string (range->layout->slider_detail_quark);

  slider_detail = BTK_RANGE_GET_CLASS (range)->slider_detail;

  if (slider_detail && slider_detail[0] == 'X')
    {
      gchar *detail = g_strdup (slider_detail);

      detail[0] = range->orientation == BTK_ORIENTATION_HORIZONTAL ? 'h' : 'v';

      range->layout->slider_detail_quark = g_quark_from_string (detail);

      g_free (detail);

      return g_quark_to_string (range->layout->slider_detail_quark);
    }

  return slider_detail;
}

static const gchar *
btk_range_get_stepper_detail (BtkRange *range,
                              Stepper   stepper)
{
  const gchar *stepper_detail;
  gboolean need_orientation;
  gboolean need_position;

  if (range->layout->stepper_detail_quark[stepper])
    return g_quark_to_string (range->layout->stepper_detail_quark[stepper]);

  stepper_detail = BTK_RANGE_GET_CLASS (range)->stepper_detail;

  need_orientation = stepper_detail && stepper_detail[0] == 'X';

  btk_widget_style_get (BTK_WIDGET (range),
                        "stepper-position-details", &need_position,
                        NULL);

  if (need_orientation || need_position)
    {
      gchar *detail;
      const gchar *position = NULL;

      if (need_position)
        {
          switch (stepper)
            {
            case STEPPER_A:
              position = "_start";
              break;
            case STEPPER_B:
              if (range->has_stepper_a)
                position = "_middle";
              else
                position = "_start";
              break;
            case STEPPER_C:
              if (range->has_stepper_d)
                position = "_middle";
              else
                position = "_end";
              break;
            case STEPPER_D:
              position = "_end";
              break;
            default:
              g_assert_not_reached ();
            }
        }

      detail = g_strconcat (stepper_detail, position, NULL);

      if (need_orientation)
        detail[0] = range->orientation == BTK_ORIENTATION_HORIZONTAL ? 'h' : 'v';

      range->layout->stepper_detail_quark[stepper] = g_quark_from_string (detail);

      g_free (detail);

      return g_quark_to_string (range->layout->stepper_detail_quark[stepper]);
    }

  return stepper_detail;
}

static void
draw_stepper (BtkRange     *range,
              Stepper       stepper,
              BtkArrowType  arrow_type,
              gboolean      clicked,
              gboolean      prelighted,
              BdkRectangle *area)
{
  BtkStateType state_type;
  BtkShadowType shadow_type;
  BdkRectangle intersection;
  BtkWidget *widget = BTK_WIDGET (range);
  gfloat arrow_scaling;
  BdkRectangle *rect;
  gint arrow_x;
  gint arrow_y;
  gint arrow_width;
  gint arrow_height;
  gboolean arrow_sensitive = TRUE;

  switch (stepper)
    {
    case STEPPER_A:
      rect = &range->layout->stepper_a;
      break;
    case STEPPER_B:
      rect = &range->layout->stepper_b;
      break;
    case STEPPER_C:
      rect = &range->layout->stepper_c;
      break;
    case STEPPER_D:
      rect = &range->layout->stepper_d;
      break;
    default:
      g_assert_not_reached ();
    };

  /* More to get the right clip rebunnyion than for efficiency */
  if (!bdk_rectangle_intersect (area, rect, &intersection))
    return;

  intersection.x += widget->allocation.x;
  intersection.y += widget->allocation.y;

  if ((!range->inverted && (arrow_type == BTK_ARROW_DOWN ||
                            arrow_type == BTK_ARROW_RIGHT)) ||
      (range->inverted  && (arrow_type == BTK_ARROW_UP ||
                            arrow_type == BTK_ARROW_LEFT)))
    {
      arrow_sensitive = range->layout->upper_sensitive;
    }
  else
    {
      arrow_sensitive = range->layout->lower_sensitive;
    }

  if (!btk_widget_is_sensitive (BTK_WIDGET (range)) || !arrow_sensitive)
    state_type = BTK_STATE_INSENSITIVE;
  else if (clicked)
    state_type = BTK_STATE_ACTIVE;
  else if (prelighted)
    state_type = BTK_STATE_PRELIGHT;
  else 
    state_type = BTK_STATE_NORMAL;

  if (clicked && arrow_sensitive)
    shadow_type = BTK_SHADOW_IN;
  else
    shadow_type = BTK_SHADOW_OUT;

  btk_paint_box (widget->style,
		 widget->window,
		 state_type, shadow_type,
		 &intersection, widget,
		 btk_range_get_stepper_detail (range, stepper),
		 widget->allocation.x + rect->x,
		 widget->allocation.y + rect->y,
		 rect->width,
		 rect->height);

  btk_widget_style_get (widget, "arrow-scaling", &arrow_scaling, NULL);

  arrow_width = rect->width * arrow_scaling;
  arrow_height = rect->height * arrow_scaling;
  arrow_x = widget->allocation.x + rect->x + (rect->width - arrow_width) / 2;
  arrow_y = widget->allocation.y + rect->y + (rect->height - arrow_height) / 2;

  if (clicked && arrow_sensitive)
    {
      gint arrow_displacement_x;
      gint arrow_displacement_y;

      btk_range_get_props (BTK_RANGE (widget),
                           NULL, NULL, NULL, NULL, NULL, NULL,
			   &arrow_displacement_x, &arrow_displacement_y);
      
      arrow_x += arrow_displacement_x;
      arrow_y += arrow_displacement_y;
    }
  
  btk_paint_arrow (widget->style,
                   widget->window,
                   state_type, shadow_type,
                   &intersection, widget,
                   btk_range_get_stepper_detail (range, stepper),
                   arrow_type,
                   TRUE,
		   arrow_x, arrow_y, arrow_width, arrow_height);
}

static gboolean
btk_range_expose (BtkWidget      *widget,
		  BdkEventExpose *event)
{
  BtkRange *range = BTK_RANGE (widget);
  gboolean sensitive;
  BtkStateType state;
  BtkShadowType shadow_type;
  BdkRectangle expose_area;	/* Relative to widget->allocation */
  BdkRectangle area;
  gint focus_line_width = 0;
  gint focus_padding = 0;
  gboolean touchscreen;

  g_object_get (btk_widget_get_settings (widget),
                "btk-touchscreen-mode", &touchscreen,
                NULL);
  if (btk_widget_get_can_focus (BTK_WIDGET (range)))
    btk_widget_style_get (BTK_WIDGET (range),
                          "focus-line-width", &focus_line_width,
                          "focus-padding", &focus_padding,
                          NULL);

  /* we're now exposing, so there's no need to force early repaints */
  if (range->layout->repaint_id)
    g_source_remove (range->layout->repaint_id);
  range->layout->repaint_id = 0;

  expose_area = event->area;
  expose_area.x -= widget->allocation.x;
  expose_area.y -= widget->allocation.y;
  
  btk_range_calc_marks (range);
  btk_range_calc_layout (range, range->adjustment->value);

  sensitive = btk_widget_is_sensitive (widget);

  /* Just to be confusing, we draw the trough for the whole
   * range rectangle, not the trough rectangle (the trough
   * rectangle is just for hit detection)
   */
  /* The bdk_rectangle_intersect is more to get the right
   * clip rebunnyion (limited to range_rect) than for efficiency
   */
  if (bdk_rectangle_intersect (&expose_area, &range->range_rect,
                               &area))
    {
      gint     x      = (widget->allocation.x + range->range_rect.x +
                         focus_line_width + focus_padding);
      gint     y      = (widget->allocation.y + range->range_rect.y +
                         focus_line_width + focus_padding);
      gint     width  = (range->range_rect.width -
                         2 * (focus_line_width + focus_padding));
      gint     height = (range->range_rect.height -
                         2 * (focus_line_width + focus_padding));
      gboolean trough_side_details;
      gboolean trough_under_steppers;
      gint     stepper_size;
      gint     stepper_spacing;

      area.x += widget->allocation.x;
      area.y += widget->allocation.y;

      btk_widget_style_get (BTK_WIDGET (range),
                            "trough-side-details",   &trough_side_details,
                            "trough-under-steppers", &trough_under_steppers,
                            "stepper-size",          &stepper_size,
                            "stepper-spacing",       &stepper_spacing,
                            NULL);

      if (stepper_spacing > 0)
        trough_under_steppers = FALSE;

      if (! trough_under_steppers)
        {
          gint offset  = 0;
          gint shorter = 0;

          if (range->has_stepper_a)
            offset += stepper_size;

          if (range->has_stepper_b)
            offset += stepper_size;

          shorter += offset;

          if (range->has_stepper_c)
            shorter += stepper_size;

          if (range->has_stepper_d)
            shorter += stepper_size;

          if (range->has_stepper_a || range->has_stepper_b)
            {
              offset  += stepper_spacing;
              shorter += stepper_spacing;
            }

          if (range->has_stepper_c || range->has_stepper_d)
            {
              shorter += stepper_spacing;
            }

          if (range->orientation == BTK_ORIENTATION_HORIZONTAL)
            {
              x     += offset;
              width -= shorter;
            }
          else
            {
              y      += offset;
              height -= shorter;
            }
	}

      if (! trough_side_details)
        {
          btk_paint_box (widget->style,
                         widget->window,
                         sensitive ? BTK_STATE_ACTIVE : BTK_STATE_INSENSITIVE,
                         BTK_SHADOW_IN,
                         &area, BTK_WIDGET(range), "trough",
                         x, y,
                         width, height);
        }
      else
        {
	  gint trough_change_pos_x = width;
	  gint trough_change_pos_y = height;

	  if (range->orientation == BTK_ORIENTATION_HORIZONTAL)
	    trough_change_pos_x = (range->layout->slider.x +
                                   range->layout->slider.width / 2 -
                                   (x - widget->allocation.x));
	  else
	    trough_change_pos_y = (range->layout->slider.y +
                                   range->layout->slider.height / 2 -
                                   (y - widget->allocation.y));

          btk_paint_box (widget->style,
                         widget->window,
                         sensitive ? BTK_STATE_ACTIVE : BTK_STATE_INSENSITIVE,
                         BTK_SHADOW_IN,
                         &area, BTK_WIDGET (range),
                         should_invert (range) ? "trough-upper" : "trough-lower",
                         x, y,
                         trough_change_pos_x, trough_change_pos_y);

	  if (range->orientation == BTK_ORIENTATION_HORIZONTAL)
	    trough_change_pos_y = 0;
	  else
	    trough_change_pos_x = 0;

          btk_paint_box (widget->style,
                         widget->window,
                         sensitive ? BTK_STATE_ACTIVE : BTK_STATE_INSENSITIVE,
                         BTK_SHADOW_IN,
                         &area, BTK_WIDGET (range),
                         should_invert (range) ? "trough-lower" : "trough-upper",
                         x + trough_change_pos_x, y + trough_change_pos_y,
                         width - trough_change_pos_x,
                         height - trough_change_pos_y);
        }

      if (range->layout->show_fill_level &&
          range->adjustment->upper - range->adjustment->page_size -
          range->adjustment->lower != 0)
	{
          gdouble  fill_level  = range->layout->fill_level;
	  gint     fill_x      = x;
	  gint     fill_y      = y;
	  gint     fill_width  = width;
	  gint     fill_height = height;
	  gchar   *fill_detail;

          fill_level = CLAMP (fill_level, range->adjustment->lower,
                              range->adjustment->upper -
                              range->adjustment->page_size);

	  if (range->orientation == BTK_ORIENTATION_HORIZONTAL)
	    {
	      fill_x     = widget->allocation.x + range->layout->trough.x;
	      fill_width = (range->layout->slider.width +
                            (fill_level - range->adjustment->lower) /
                            (range->adjustment->upper -
                             range->adjustment->lower -
                             range->adjustment->page_size) *
                            (range->layout->trough.width -
                             range->layout->slider.width));

              if (should_invert (range))
                fill_x += range->layout->trough.width - fill_width;
	    }
	  else
	    {
	      fill_y      = widget->allocation.y + range->layout->trough.y;
	      fill_height = (range->layout->slider.height +
                             (fill_level - range->adjustment->lower) /
                             (range->adjustment->upper -
                              range->adjustment->lower -
                              range->adjustment->page_size) *
                             (range->layout->trough.height -
                              range->layout->slider.height));

              if (should_invert (range))
                fill_y += range->layout->trough.height - fill_height;
	    }

	  if (fill_level < range->adjustment->upper - range->adjustment->page_size)
	    fill_detail = "trough-fill-level-full";
	  else
	    fill_detail = "trough-fill-level";

          btk_paint_box (widget->style,
                         widget->window,
                         sensitive ? BTK_STATE_ACTIVE : BTK_STATE_INSENSITIVE,
                         BTK_SHADOW_OUT,
                         &area, BTK_WIDGET (range), fill_detail,
                         fill_x, fill_y,
                         fill_width, fill_height);
	}

      if (sensitive && btk_widget_has_focus (widget))
        btk_paint_focus (widget->style, widget->window, btk_widget_get_state (widget),
                         &area, widget, "trough",
                         widget->allocation.x + range->range_rect.x,
                         widget->allocation.y + range->range_rect.y,
                         range->range_rect.width,
                         range->range_rect.height);
    }

  shadow_type = BTK_SHADOW_OUT;

  if (!sensitive)
    state = BTK_STATE_INSENSITIVE;
  else if (!touchscreen && range->layout->mouse_location == MOUSE_SLIDER)
    state = BTK_STATE_PRELIGHT;
  else
    state = BTK_STATE_NORMAL;

  if (range->layout->grab_location == MOUSE_SLIDER)
    {
      gboolean activate_slider;

      btk_widget_style_get (widget, "activate-slider", &activate_slider, NULL);

      if (activate_slider)
        {
          state = BTK_STATE_ACTIVE;
          shadow_type = BTK_SHADOW_IN;
        }
    }

  if (bdk_rectangle_intersect (&expose_area,
                               &range->layout->slider,
                               &area))
    {
      area.x += widget->allocation.x;
      area.y += widget->allocation.y;
      
      btk_paint_slider (widget->style,
                        widget->window,
                        state,
                        shadow_type,
                        &area,
                        widget,
                        btk_range_get_slider_detail (range),
                        widget->allocation.x + range->layout->slider.x,
                        widget->allocation.y + range->layout->slider.y,
                        range->layout->slider.width,
                        range->layout->slider.height,
                        range->orientation);
    }
  
  if (range->has_stepper_a)
    draw_stepper (range, STEPPER_A,
                  range->orientation == BTK_ORIENTATION_VERTICAL ? BTK_ARROW_UP : BTK_ARROW_LEFT,
                  range->layout->grab_location == MOUSE_STEPPER_A,
                  !touchscreen && range->layout->mouse_location == MOUSE_STEPPER_A,
                  &expose_area);

  if (range->has_stepper_b)
    draw_stepper (range, STEPPER_B,
                  range->orientation == BTK_ORIENTATION_VERTICAL ? BTK_ARROW_DOWN : BTK_ARROW_RIGHT,
                  range->layout->grab_location == MOUSE_STEPPER_B,
                  !touchscreen && range->layout->mouse_location == MOUSE_STEPPER_B,
                  &expose_area);

  if (range->has_stepper_c)
    draw_stepper (range, STEPPER_C,
                  range->orientation == BTK_ORIENTATION_VERTICAL ? BTK_ARROW_UP : BTK_ARROW_LEFT,
                  range->layout->grab_location == MOUSE_STEPPER_C,
                  !touchscreen && range->layout->mouse_location == MOUSE_STEPPER_C,
                  &expose_area);

  if (range->has_stepper_d)
    draw_stepper (range, STEPPER_D,
                  range->orientation == BTK_ORIENTATION_VERTICAL ? BTK_ARROW_DOWN : BTK_ARROW_RIGHT,
                  range->layout->grab_location == MOUSE_STEPPER_D,
                  !touchscreen && range->layout->mouse_location == MOUSE_STEPPER_D,
                  &expose_area);
  
  return FALSE;
}

static void
range_grab_add (BtkRange      *range,
                MouseLocation  location,
                gint           button)
{
  /* we don't actually btk_grab, since a button is down */

  btk_grab_add (BTK_WIDGET (range));
  
  range->layout->grab_location = location;
  range->layout->grab_button = button;
  
  if (btk_range_update_mouse_location (range))
    btk_widget_queue_draw (BTK_WIDGET (range));
}

static void
range_grab_remove (BtkRange *range)
{
  MouseLocation location;

  btk_grab_remove (BTK_WIDGET (range));
 
  location = range->layout->grab_location; 
  range->layout->grab_location = MOUSE_OUTSIDE;
  range->layout->grab_button = 0;

  if (btk_range_update_mouse_location (range) ||
      location != MOUSE_OUTSIDE)
    btk_widget_queue_draw (BTK_WIDGET (range));
}

static BtkScrollType
range_get_scroll_for_grab (BtkRange      *range)
{ 
  gboolean invert;

  invert = should_invert (range);
  switch (range->layout->grab_location)
    {
      /* Backward stepper */
    case MOUSE_STEPPER_A:
    case MOUSE_STEPPER_C:
      switch (range->layout->grab_button)
        {
        case 1:
          return invert ? BTK_SCROLL_STEP_FORWARD : BTK_SCROLL_STEP_BACKWARD;
          break;
        case 2:
          return invert ? BTK_SCROLL_PAGE_FORWARD : BTK_SCROLL_PAGE_BACKWARD;
          break;
        case 3:
          return invert ? BTK_SCROLL_END : BTK_SCROLL_START;
          break;
        }
      break;

      /* Forward stepper */
    case MOUSE_STEPPER_B:
    case MOUSE_STEPPER_D:
      switch (range->layout->grab_button)
        {
        case 1:
          return invert ? BTK_SCROLL_STEP_BACKWARD : BTK_SCROLL_STEP_FORWARD;
          break;
        case 2:
          return invert ? BTK_SCROLL_PAGE_BACKWARD : BTK_SCROLL_PAGE_FORWARD;
          break;
        case 3:
          return invert ? BTK_SCROLL_START : BTK_SCROLL_END;
          break;
       }
      break;

      /* In the trough */
    case MOUSE_TROUGH:
      {
        if (range->trough_click_forward)
	  return BTK_SCROLL_PAGE_FORWARD;
        else
	  return BTK_SCROLL_PAGE_BACKWARD;
      }
      break;

    case MOUSE_OUTSIDE:
    case MOUSE_SLIDER:
    case MOUSE_WIDGET:
      break;
    }

  return BTK_SCROLL_NONE;
}

static gdouble
coord_to_value (BtkRange *range,
                gint      coord)
{
  gdouble frac;
  gdouble value;
  gint    trough_length;
  gint    trough_start;
  gint    slider_length;
  gint    trough_border;
  gint    trough_under_steppers;

  if (range->orientation == BTK_ORIENTATION_VERTICAL)
    {
      trough_length = range->layout->trough.height;
      trough_start  = range->layout->trough.y;
      slider_length = range->layout->slider.height;
    }
  else
    {
      trough_length = range->layout->trough.width;
      trough_start  = range->layout->trough.x;
      slider_length = range->layout->slider.width;
    }

  btk_range_get_props (range, NULL, NULL, NULL, &trough_border, NULL,
                       &trough_under_steppers, NULL, NULL);

  if (! trough_under_steppers)
    {
      trough_start += trough_border;
      trough_length -= 2 * trough_border;
    }

  if (trough_length == slider_length)
    frac = 1.0;
  else
    frac = (MAX (0, coord - trough_start) /
            (gdouble) (trough_length - slider_length));

  if (should_invert (range))
    frac = 1.0 - frac;

  value = range->adjustment->lower + frac * (range->adjustment->upper -
                                             range->adjustment->lower -
                                             range->adjustment->page_size);

  return value;
}

static gboolean
btk_range_key_press (BtkWidget   *widget,
		     BdkEventKey *event)
{
  BtkRange *range = BTK_RANGE (widget);

  if (event->keyval == BDK_Escape &&
      range->layout->grab_location != MOUSE_OUTSIDE)
    {
      stop_scrolling (range);

      update_slider_position (range,
			      range->slide_initial_coordinate,
			      range->slide_initial_coordinate);

      return TRUE;
    }

  return BTK_WIDGET_CLASS (btk_range_parent_class)->key_press_event (widget, event);
}

static gint
btk_range_button_press (BtkWidget      *widget,
			BdkEventButton *event)
{
  BtkRange *range = BTK_RANGE (widget);
  gboolean primary_warps;
  gint page_increment_button, warp_button;
  
  if (!btk_widget_has_focus (widget))
    btk_widget_grab_focus (widget);

  /* ignore presses when we're already doing something else. */
  if (range->layout->grab_location != MOUSE_OUTSIDE)
    return FALSE;

  range->layout->mouse_x = event->x;
  range->layout->mouse_y = event->y;
  if (btk_range_update_mouse_location (range))
    btk_widget_queue_draw (widget);

  g_object_get (btk_widget_get_settings (widget),
                "btk-primary-button-warps-slider", &primary_warps,
                NULL);
  if (primary_warps)
    {
      warp_button = 1;
      page_increment_button = 3;
    }
  else
    {
      warp_button = 2;
      page_increment_button = 1;
    }

  if (range->layout->mouse_location == MOUSE_TROUGH  &&
      event->button == page_increment_button)
    {
      /* this button steps by page increment, as with button 2 on a stepper
       */
      BtkScrollType scroll;
      gdouble click_value;
      
      click_value = coord_to_value (range,
                                    range->orientation == BTK_ORIENTATION_VERTICAL ?
                                    event->y : event->x);
      
      range->trough_click_forward = click_value > range->adjustment->value;
      range_grab_add (range, MOUSE_TROUGH, event->button);
      
      scroll = range_get_scroll_for_grab (range);
      
      btk_range_add_step_timer (range, scroll);

      return TRUE;
    }
  else if ((range->layout->mouse_location == MOUSE_STEPPER_A ||
            range->layout->mouse_location == MOUSE_STEPPER_B ||
            range->layout->mouse_location == MOUSE_STEPPER_C ||
            range->layout->mouse_location == MOUSE_STEPPER_D) &&
           (event->button == 1 || event->button == 2 || event->button == 3))
    {
      BdkRectangle *stepper_area;
      BtkScrollType scroll;
      
      range_grab_add (range, range->layout->mouse_location, event->button);

      stepper_area = get_area (range, range->layout->mouse_location);
      btk_widget_queue_draw_area (widget,
                                  widget->allocation.x + stepper_area->x,
                                  widget->allocation.y + stepper_area->y,
                                  stepper_area->width,
                                  stepper_area->height);

      scroll = range_get_scroll_for_grab (range);
      if (scroll != BTK_SCROLL_NONE)
        btk_range_add_step_timer (range, scroll);
      
      return TRUE;
    }
  else if ((range->layout->mouse_location == MOUSE_TROUGH &&
            event->button == warp_button) ||
           range->layout->mouse_location == MOUSE_SLIDER)
    {
      gboolean need_value_update = FALSE;
      gboolean activate_slider;

      /* Any button can be used to drag the slider, but you can start
       * dragging the slider with a trough click using the warp button;
       * we warp the slider to mouse position, then begin the slider drag.
       */
      if (range->layout->mouse_location != MOUSE_SLIDER)
        {
          gdouble slider_low_value, slider_high_value, new_value;
          
          slider_high_value =
            coord_to_value (range,
                            range->orientation == BTK_ORIENTATION_VERTICAL ?
                            event->y : event->x);
          slider_low_value =
            coord_to_value (range,
                            range->orientation == BTK_ORIENTATION_VERTICAL ?
                            event->y - range->layout->slider.height :
                            event->x - range->layout->slider.width);
          
          /* compute new value for warped slider */
          new_value = slider_low_value + (slider_high_value - slider_low_value) / 2;

	  /* recalc slider, so we can set slide_initial_slider_position
           * properly
           */
	  range->need_recalc = TRUE;
          btk_range_calc_layout (range, new_value);

	  /* defer adjustment updates to update_slider_position() in order
	   * to keep pixel quantisation
	   */
	  need_value_update = TRUE;
        }
      
      if (range->orientation == BTK_ORIENTATION_VERTICAL)
        {
          range->slide_initial_slider_position = range->layout->slider.y;
          range->slide_initial_coordinate = event->y;
        }
      else
        {
          range->slide_initial_slider_position = range->layout->slider.x;
          range->slide_initial_coordinate = event->x;
        }

      range_grab_add (range, MOUSE_SLIDER, event->button);

      btk_widget_style_get (widget, "activate-slider", &activate_slider, NULL);

      /* force a redraw, if the active slider is drawn differently to the
       * prelight one
       */
      if (activate_slider)
        btk_widget_queue_draw (widget);

      if (need_value_update)
        update_slider_position (range, event->x, event->y);

      return TRUE;
    }
  
  return FALSE;
}

/* During a slide, move the slider as required given new mouse position */
static void
update_slider_position (BtkRange *range,
                        gint      mouse_x,
                        gint      mouse_y)
{
  gint delta;
  gint c;
  gdouble new_value;
  gboolean handled;
  gdouble next_value;
  gdouble mark_value;
  gdouble mark_delta;
  gint i;

  if (range->orientation == BTK_ORIENTATION_VERTICAL)
    delta = mouse_y - range->slide_initial_coordinate;
  else
    delta = mouse_x - range->slide_initial_coordinate;

  c = range->slide_initial_slider_position + delta;

  new_value = coord_to_value (range, c);
  next_value = coord_to_value (range, c + 1);
  mark_delta = fabs (next_value - new_value); 

  for (i = 0; i < range->layout->n_marks; i++)
    {
      mark_value = range->layout->marks[i];

      if (fabs (range->adjustment->value - mark_value) < 3 * mark_delta)
        {
          if (fabs (new_value - mark_value) < (range->slider_end - range->slider_start) * 0.5 * mark_delta)
            {
              new_value = mark_value;
              break;
            }
        }
    }  

  g_signal_emit (range, signals[CHANGE_VALUE], 0, BTK_SCROLL_JUMP, new_value,
                 &handled);
}

static void 
stop_scrolling (BtkRange *range)
{
  range_grab_remove (range);
  btk_range_remove_step_timer (range);
  /* Flush any pending discontinuous/delayed updates */
  btk_range_update_value (range);
}

static gboolean
btk_range_grab_broken (BtkWidget          *widget,
		       BdkEventGrabBroken *event)
{
  BtkRange *range = BTK_RANGE (widget);

  if (range->layout->grab_location != MOUSE_OUTSIDE)
    {
      if (range->layout->grab_location == MOUSE_SLIDER)
	update_slider_position (range, range->layout->mouse_x, range->layout->mouse_y);
      
      stop_scrolling (range);
      
      return TRUE;
    }
  
  return FALSE;
}

static gint
btk_range_button_release (BtkWidget      *widget,
			  BdkEventButton *event)
{
  BtkRange *range = BTK_RANGE (widget);

  if (event->window == range->event_window)
    {
      range->layout->mouse_x = event->x;
      range->layout->mouse_y = event->y;
    }
  else
    {
      bdk_window_get_pointer (range->event_window,
			      &range->layout->mouse_x,
			      &range->layout->mouse_y,
			      NULL);
    }
  
  if (range->layout->grab_button == event->button)
    {
      if (range->layout->grab_location == MOUSE_SLIDER)
        update_slider_position (range, range->layout->mouse_x, range->layout->mouse_y);

      stop_scrolling (range);
      
      return TRUE;
    }

  return FALSE;
}

/**
 * _btk_range_get_wheel_delta:
 * @range: a #BtkRange
 * @direction: A #BdkScrollDirection
 * 
 * Returns a good step value for the mouse wheel.
 * 
 * Return value: A good step value for the mouse wheel. 
 * 
 * Since: 2.4
 **/
gdouble
_btk_range_get_wheel_delta (BtkRange           *range,
			    BdkScrollDirection  direction)
{
  BtkAdjustment *adj = range->adjustment;
  gdouble delta;

  if (BTK_IS_SCROLLBAR (range))
    delta = pow (adj->page_size, 2.0 / 3.0);
  else
    delta = adj->step_increment * 2;
  
  if (direction == BDK_SCROLL_UP ||
      direction == BDK_SCROLL_LEFT)
    delta = - delta;
  
  if (range->inverted)
    delta = - delta;

  return delta;
}
      
static gboolean
btk_range_scroll_event (BtkWidget      *widget,
			BdkEventScroll *event)
{
  BtkRange *range = BTK_RANGE (widget);

  if (btk_widget_get_realized (widget))
    {
      BtkAdjustment *adj = BTK_RANGE (range)->adjustment;
      gdouble delta;
      gboolean handled;

      delta = _btk_range_get_wheel_delta (range, event->direction);

      g_signal_emit (range, signals[CHANGE_VALUE], 0,
                     BTK_SCROLL_JUMP, adj->value + delta,
                     &handled);
      
      /* Policy DELAYED makes sense with scroll events,
       * but DISCONTINUOUS doesn't, so we update immediately
       * for DISCONTINUOUS
       */
      if (range->update_policy == BTK_UPDATE_DISCONTINUOUS)
        btk_range_update_value (range);
    }

  return TRUE;
}

static gboolean
btk_range_motion_notify (BtkWidget      *widget,
			 BdkEventMotion *event)
{
  BtkRange *range;

  range = BTK_RANGE (widget);

  bdk_event_request_motions (event);
  
  range->layout->mouse_x = event->x;
  range->layout->mouse_y = event->y;

  if (btk_range_update_mouse_location (range))
    btk_widget_queue_draw (widget);

  if (range->layout->grab_location == MOUSE_SLIDER)
    update_slider_position (range, event->x, event->y);

  /* We handled the event if the mouse was in the range_rect */
  return range->layout->mouse_location != MOUSE_OUTSIDE;
}

static gboolean
btk_range_enter_notify (BtkWidget        *widget,
			BdkEventCrossing *event)
{
  BtkRange *range = BTK_RANGE (widget);

  range->layout->mouse_x = event->x;
  range->layout->mouse_y = event->y;

  if (btk_range_update_mouse_location (range))
    btk_widget_queue_draw (widget);
  
  return TRUE;
}

static gboolean
btk_range_leave_notify (BtkWidget        *widget,
			BdkEventCrossing *event)
{
  BtkRange *range = BTK_RANGE (widget);

  range->layout->mouse_x = -1;
  range->layout->mouse_y = -1;

  if (btk_range_update_mouse_location (range))
    btk_widget_queue_draw (widget);
  
  return TRUE;
}

static void
btk_range_grab_notify (BtkWidget *widget,
		       gboolean   was_grabbed)
{
  if (!was_grabbed)
    stop_scrolling (BTK_RANGE (widget));
}

static void
btk_range_state_changed (BtkWidget    *widget,
			 BtkStateType  previous_state)
{
  if (!btk_widget_is_sensitive (widget))
    stop_scrolling (BTK_RANGE (widget));
}

#define check_rectangle(rectangle1, rectangle2)              \
  {                                                          \
    if (rectangle1.x != rectangle2.x) return TRUE;           \
    if (rectangle1.y != rectangle2.y) return TRUE;           \
    if (rectangle1.width  != rectangle2.width)  return TRUE; \
    if (rectangle1.height != rectangle2.height) return TRUE; \
  }

static gboolean
layout_changed (BtkRangeLayout *layout1, 
		BtkRangeLayout *layout2)
{
  check_rectangle (layout1->slider, layout2->slider);
  check_rectangle (layout1->trough, layout2->trough);
  check_rectangle (layout1->stepper_a, layout2->stepper_a);
  check_rectangle (layout1->stepper_d, layout2->stepper_d);
  check_rectangle (layout1->stepper_b, layout2->stepper_b);
  check_rectangle (layout1->stepper_c, layout2->stepper_c);

  if (layout1->upper_sensitive != layout2->upper_sensitive) return TRUE;
  if (layout1->lower_sensitive != layout2->lower_sensitive) return TRUE;

  return FALSE;
}

static void
btk_range_adjustment_changed (BtkAdjustment *adjustment,
			      gpointer       data)
{
  BtkRange *range = BTK_RANGE (data);
  /* create a copy of the layout */
  BtkRangeLayout layout = *range->layout;

  range->layout->recalc_marks = TRUE;
  range->need_recalc = TRUE;
  btk_range_calc_layout (range, range->adjustment->value);
  
  /* now check whether the layout changed  */
  if (layout_changed (range->layout, &layout))
    btk_widget_queue_draw (BTK_WIDGET (range));

  /* Note that we don't round off to range->round_digits here.
   * that's because it's really broken to change a value
   * in response to a change signal on that value; round_digits
   * is therefore defined to be a filter on what the BtkRange
   * can input into the adjustment, not a filter that the BtkRange
   * will enforce on the adjustment.
   */
}

static gboolean
force_repaint (gpointer data)
{
  BtkRange *range = BTK_RANGE (data);

  range->layout->repaint_id = 0;
  if (btk_widget_is_drawable (BTK_WIDGET (range)))
    bdk_window_process_updates (BTK_WIDGET (range)->window, FALSE);

  return FALSE;
}

static void
btk_range_adjustment_value_changed (BtkAdjustment *adjustment,
				    gpointer       data)
{
  BtkRange *range = BTK_RANGE (data);
  /* create a copy of the layout */
  BtkRangeLayout layout = *range->layout;

  range->need_recalc = TRUE;
  btk_range_calc_layout (range, range->adjustment->value);
  
  /* now check whether the layout changed  */
  if (layout_changed (range->layout, &layout) ||
      (BTK_IS_SCALE (range) && BTK_SCALE (range)->draw_value))
    {
      btk_widget_queue_draw (BTK_WIDGET (range));
      /* setup a timer to ensure the range isn't lagging too much behind the scroll position */
      if (!range->layout->repaint_id)
        range->layout->repaint_id = bdk_threads_add_timeout_full (BDK_PRIORITY_EVENTS, 181, force_repaint, range, NULL);
    }
  
  /* Note that we don't round off to range->round_digits here.
   * that's because it's really broken to change a value
   * in response to a change signal on that value; round_digits
   * is therefore defined to be a filter on what the BtkRange
   * can input into the adjustment, not a filter that the BtkRange
   * will enforce on the adjustment.
   */

  g_signal_emit (range, signals[VALUE_CHANGED], 0);
}

static void
btk_range_style_set (BtkWidget *widget,
                     BtkStyle  *previous_style)
{
  BtkRange *range = BTK_RANGE (widget);

  range->need_recalc = TRUE;

  BTK_WIDGET_CLASS (btk_range_parent_class)->style_set (widget, previous_style);
}

static void
apply_marks (BtkRange *range, 
             gdouble   oldval,
             gdouble  *newval)
{
  gint i;
  gdouble mark;

  for (i = 0; i < range->layout->n_marks; i++)
    {
      mark = range->layout->marks[i];
      if ((oldval < mark && mark < *newval) ||
          (oldval > mark && mark > *newval))
        {
          *newval = mark;
          return;
        }
    }
}

static void
step_back (BtkRange *range)
{
  gdouble newval;
  gboolean handled;
  
  newval = range->adjustment->value - range->adjustment->step_increment;
  apply_marks (range, range->adjustment->value, &newval);
  g_signal_emit (range, signals[CHANGE_VALUE], 0,
                 BTK_SCROLL_STEP_BACKWARD, newval, &handled);
}

static void
step_forward (BtkRange *range)
{
  gdouble newval;
  gboolean handled;

  newval = range->adjustment->value + range->adjustment->step_increment;
  apply_marks (range, range->adjustment->value, &newval);
  g_signal_emit (range, signals[CHANGE_VALUE], 0,
                 BTK_SCROLL_STEP_FORWARD, newval, &handled);
}


static void
page_back (BtkRange *range)
{
  gdouble newval;
  gboolean handled;

  newval = range->adjustment->value - range->adjustment->page_increment;
  apply_marks (range, range->adjustment->value, &newval);
  g_signal_emit (range, signals[CHANGE_VALUE], 0,
                 BTK_SCROLL_PAGE_BACKWARD, newval, &handled);
}

static void
page_forward (BtkRange *range)
{
  gdouble newval;
  gboolean handled;

  newval = range->adjustment->value + range->adjustment->page_increment;
  apply_marks (range, range->adjustment->value, &newval);
  g_signal_emit (range, signals[CHANGE_VALUE], 0,
                 BTK_SCROLL_PAGE_FORWARD, newval, &handled);
}

static void
scroll_begin (BtkRange *range)
{
  gboolean handled;
  g_signal_emit (range, signals[CHANGE_VALUE], 0,
                 BTK_SCROLL_START, range->adjustment->lower,
                 &handled);
}

static void
scroll_end (BtkRange *range)
{
  gdouble newval;
  gboolean handled;

  newval = range->adjustment->upper - range->adjustment->page_size;
  g_signal_emit (range, signals[CHANGE_VALUE], 0, BTK_SCROLL_END, newval,
                 &handled);
}

static gboolean
btk_range_scroll (BtkRange     *range,
                  BtkScrollType scroll)
{
  gdouble old_value = range->adjustment->value;

  switch (scroll)
    {
    case BTK_SCROLL_STEP_LEFT:
      if (should_invert (range))
        step_forward (range);
      else
        step_back (range);
      break;
                    
    case BTK_SCROLL_STEP_UP:
      if (should_invert (range))
        step_forward (range);
      else
        step_back (range);
      break;

    case BTK_SCROLL_STEP_RIGHT:
      if (should_invert (range))
        step_back (range);
      else
        step_forward (range);
      break;
                    
    case BTK_SCROLL_STEP_DOWN:
      if (should_invert (range))
        step_back (range);
      else
        step_forward (range);
      break;
                  
    case BTK_SCROLL_STEP_BACKWARD:
      step_back (range);
      break;
                  
    case BTK_SCROLL_STEP_FORWARD:
      step_forward (range);
      break;

    case BTK_SCROLL_PAGE_LEFT:
      if (should_invert (range))
        page_forward (range);
      else
        page_back (range);
      break;
                    
    case BTK_SCROLL_PAGE_UP:
      if (should_invert (range))
        page_forward (range);
      else
        page_back (range);
      break;

    case BTK_SCROLL_PAGE_RIGHT:
      if (should_invert (range))
        page_back (range);
      else
        page_forward (range);
      break;
                    
    case BTK_SCROLL_PAGE_DOWN:
      if (should_invert (range))
        page_back (range);
      else
        page_forward (range);
      break;
                  
    case BTK_SCROLL_PAGE_BACKWARD:
      page_back (range);
      break;
                  
    case BTK_SCROLL_PAGE_FORWARD:
      page_forward (range);
      break;

    case BTK_SCROLL_START:
      scroll_begin (range);
      break;

    case BTK_SCROLL_END:
      scroll_end (range);
      break;

    case BTK_SCROLL_JUMP:
      /* Used by CList, range doesn't use it. */
      break;

    case BTK_SCROLL_NONE:
      break;
    }

  return range->adjustment->value != old_value;
}

static void
btk_range_move_slider (BtkRange     *range,
                       BtkScrollType scroll)
{
  gboolean cursor_only;

  g_object_get (btk_widget_get_settings (BTK_WIDGET (range)),
                "btk-keynav-cursor-only", &cursor_only,
                NULL);

  if (cursor_only)
    {
      BtkWidget *toplevel = btk_widget_get_toplevel (BTK_WIDGET (range));

      if (range->orientation == BTK_ORIENTATION_HORIZONTAL)
        {
          if (scroll == BTK_SCROLL_STEP_UP ||
              scroll == BTK_SCROLL_STEP_DOWN)
            {
              if (toplevel)
                btk_widget_child_focus (toplevel,
                                        scroll == BTK_SCROLL_STEP_UP ?
                                        BTK_DIR_UP : BTK_DIR_DOWN);
              return;
            }
        }
      else
        {
          if (scroll == BTK_SCROLL_STEP_LEFT ||
              scroll == BTK_SCROLL_STEP_RIGHT)
            {
              if (toplevel)
                btk_widget_child_focus (toplevel,
                                        scroll == BTK_SCROLL_STEP_LEFT ?
                                        BTK_DIR_LEFT : BTK_DIR_RIGHT);
              return;
            }
        }
    }

  if (! btk_range_scroll (range, scroll))
    btk_widget_error_bell (BTK_WIDGET (range));

  /* Policy DELAYED makes sense with key events,
   * but DISCONTINUOUS doesn't, so we update immediately
   * for DISCONTINUOUS
   */
  if (range->update_policy == BTK_UPDATE_DISCONTINUOUS)
    btk_range_update_value (range);
}

static void
btk_range_get_props (BtkRange  *range,
                     gint      *slider_width,
                     gint      *stepper_size,
                     gint      *focus_width,
                     gint      *trough_border,
                     gint      *stepper_spacing,
                     gboolean  *trough_under_steppers,
		     gint      *arrow_displacement_x,
		     gint      *arrow_displacement_y)
{
  BtkWidget *widget =  BTK_WIDGET (range);
  gint tmp_slider_width, tmp_stepper_size, tmp_focus_width, tmp_trough_border;
  gint tmp_stepper_spacing, tmp_trough_under_steppers;
  gint tmp_arrow_displacement_x, tmp_arrow_displacement_y;
  
  btk_widget_style_get (widget,
                        "slider-width", &tmp_slider_width,
                        "trough-border", &tmp_trough_border,
                        "stepper-size", &tmp_stepper_size,
                        "stepper-spacing", &tmp_stepper_spacing,
                        "trough-under-steppers", &tmp_trough_under_steppers,
			"arrow-displacement-x", &tmp_arrow_displacement_x,
			"arrow-displacement-y", &tmp_arrow_displacement_y,
                        NULL);

  if (tmp_stepper_spacing > 0)
    tmp_trough_under_steppers = FALSE;

  if (btk_widget_get_can_focus (BTK_WIDGET (range)))
    {
      gint focus_line_width;
      gint focus_padding;
      
      btk_widget_style_get (BTK_WIDGET (range),
			    "focus-line-width", &focus_line_width,
			    "focus-padding", &focus_padding,
			    NULL);

      tmp_focus_width = focus_line_width + focus_padding;
    }
  else
    {
      tmp_focus_width = 0;
    }
  
  if (slider_width)
    *slider_width = tmp_slider_width;

  if (focus_width)
    *focus_width = tmp_focus_width;

  if (trough_border)
    *trough_border = tmp_trough_border;

  if (stepper_size)
    *stepper_size = tmp_stepper_size;

  if (stepper_spacing)
    *stepper_spacing = tmp_stepper_spacing;

  if (trough_under_steppers)
    *trough_under_steppers = tmp_trough_under_steppers;

  if (arrow_displacement_x)
    *arrow_displacement_x = tmp_arrow_displacement_x;

  if (arrow_displacement_y)
    *arrow_displacement_y = tmp_arrow_displacement_y;
}

#define POINT_IN_RECT(xcoord, ycoord, rect) \
 ((xcoord) >= (rect).x &&                   \
  (xcoord) <  ((rect).x + (rect).width) &&  \
  (ycoord) >= (rect).y &&                   \
  (ycoord) <  ((rect).y + (rect).height))

/* Update mouse location, return TRUE if it changes */
static gboolean
btk_range_update_mouse_location (BtkRange *range)
{
  gint x, y;
  MouseLocation old;
  BtkWidget *widget;

  widget = BTK_WIDGET (range);
  
  old = range->layout->mouse_location;
  
  x = range->layout->mouse_x;
  y = range->layout->mouse_y;

  if (range->layout->grab_location != MOUSE_OUTSIDE)
    range->layout->mouse_location = range->layout->grab_location;
  else if (POINT_IN_RECT (x, y, range->layout->stepper_a))
    range->layout->mouse_location = MOUSE_STEPPER_A;
  else if (POINT_IN_RECT (x, y, range->layout->stepper_b))
    range->layout->mouse_location = MOUSE_STEPPER_B;
  else if (POINT_IN_RECT (x, y, range->layout->stepper_c))
    range->layout->mouse_location = MOUSE_STEPPER_C;
  else if (POINT_IN_RECT (x, y, range->layout->stepper_d))
    range->layout->mouse_location = MOUSE_STEPPER_D;
  else if (POINT_IN_RECT (x, y, range->layout->slider))
    range->layout->mouse_location = MOUSE_SLIDER;
  else if (POINT_IN_RECT (x, y, range->layout->trough))
    range->layout->mouse_location = MOUSE_TROUGH;
  else if (POINT_IN_RECT (x, y, widget->allocation))
    range->layout->mouse_location = MOUSE_WIDGET;
  else
    range->layout->mouse_location = MOUSE_OUTSIDE;

  return old != range->layout->mouse_location;
}

/* Clamp rect, border inside widget->allocation, such that we prefer
 * to take space from border not rect in all directions, and prefer to
 * give space to border over rect in one direction.
 */
static void
clamp_dimensions (BtkWidget    *widget,
                  BdkRectangle *rect,
                  BtkBorder    *border,
                  gboolean      border_expands_horizontally)
{
  gint extra, shortage;
  
  g_return_if_fail (rect->x == 0);
  g_return_if_fail (rect->y == 0);  
  g_return_if_fail (rect->width >= 0);
  g_return_if_fail (rect->height >= 0);

  /* Width */
  
  extra = widget->allocation.width - border->left - border->right - rect->width;
  if (extra > 0)
    {
      if (border_expands_horizontally)
        {
          border->left += extra / 2;
          border->right += extra / 2 + extra % 2;
        }
      else
        {
          rect->width += extra;
        }
    }
  
  /* See if we can fit rect, if not kill the border */
  shortage = rect->width - widget->allocation.width;
  if (shortage > 0)
    {
      rect->width = widget->allocation.width;
      /* lose the border */
      border->left = 0;
      border->right = 0;
    }
  else
    {
      /* See if we can fit rect with borders */
      shortage = rect->width + border->left + border->right -
        widget->allocation.width;
      if (shortage > 0)
        {
          /* Shrink borders */
          border->left -= shortage / 2;
          border->right -= shortage / 2 + shortage % 2;
        }
    }

  /* Height */
  
  extra = widget->allocation.height - border->top - border->bottom - rect->height;
  if (extra > 0)
    {
      if (border_expands_horizontally)
        {
          /* don't expand border vertically */
          rect->height += extra;
        }
      else
        {
          border->top += extra / 2;
          border->bottom += extra / 2 + extra % 2;
        }
    }
  
  /* See if we can fit rect, if not kill the border */
  shortage = rect->height - widget->allocation.height;
  if (shortage > 0)
    {
      rect->height = widget->allocation.height;
      /* lose the border */
      border->top = 0;
      border->bottom = 0;
    }
  else
    {
      /* See if we can fit rect with borders */
      shortage = rect->height + border->top + border->bottom -
        widget->allocation.height;
      if (shortage > 0)
        {
          /* Shrink borders */
          border->top -= shortage / 2;
          border->bottom -= shortage / 2 + shortage % 2;
        }
    }
}

static void
btk_range_calc_request (BtkRange      *range,
                        gint           slider_width,
                        gint           stepper_size,
                        gint           focus_width,
                        gint           trough_border,
                        gint           stepper_spacing,
                        BdkRectangle  *range_rect,
                        BtkBorder     *border,
                        gint          *n_steppers_p,
                        gboolean      *has_steppers_ab,
                        gboolean      *has_steppers_cd,
                        gint          *slider_length_p)
{
  gint slider_length;
  gint n_steppers;
  gint n_steppers_ab;
  gint n_steppers_cd;

  border->left = 0;
  border->right = 0;
  border->top = 0;
  border->bottom = 0;

  if (BTK_RANGE_GET_CLASS (range)->get_range_border)
    BTK_RANGE_GET_CLASS (range)->get_range_border (range, border);

  n_steppers_ab = 0;
  n_steppers_cd = 0;

  if (range->has_stepper_a)
    n_steppers_ab += 1;
  if (range->has_stepper_b)
    n_steppers_ab += 1;
  if (range->has_stepper_c)
    n_steppers_cd += 1;
  if (range->has_stepper_d)
    n_steppers_cd += 1;

  n_steppers = n_steppers_ab + n_steppers_cd;

  slider_length = range->min_slider_size;

  range_rect->x = 0;
  range_rect->y = 0;
  
  /* We never expand to fill available space in the small dimension
   * (i.e. vertical scrollbars are always a fixed width)
   */
  if (range->orientation == BTK_ORIENTATION_VERTICAL)
    {
      range_rect->width = (focus_width + trough_border) * 2 + slider_width;
      range_rect->height = stepper_size * n_steppers + (focus_width + trough_border) * 2 + slider_length;

      if (n_steppers_ab > 0)
        range_rect->height += stepper_spacing;

      if (n_steppers_cd > 0)
        range_rect->height += stepper_spacing;
    }
  else
    {
      range_rect->width = stepper_size * n_steppers + (focus_width + trough_border) * 2 + slider_length;
      range_rect->height = (focus_width + trough_border) * 2 + slider_width;

      if (n_steppers_ab > 0)
        range_rect->width += stepper_spacing;

      if (n_steppers_cd > 0)
        range_rect->width += stepper_spacing;
    }

  if (n_steppers_p)
    *n_steppers_p = n_steppers;

  if (has_steppers_ab)
    *has_steppers_ab = (n_steppers_ab > 0);

  if (has_steppers_cd)
    *has_steppers_cd = (n_steppers_cd > 0);

  if (slider_length_p)
    *slider_length_p = slider_length;
}

static void
btk_range_calc_layout (BtkRange *range,
		       gdouble   adjustment_value)
{
  gint slider_width, stepper_size, focus_width, trough_border, stepper_spacing;
  gint slider_length;
  BtkBorder border;
  gint n_steppers;
  gboolean has_steppers_ab;
  gboolean has_steppers_cd;
  gboolean trough_under_steppers;
  BdkRectangle range_rect;
  BtkRangeLayout *layout;
  BtkWidget *widget;
  
  if (!range->need_recalc)
    return;

  /* If we have a too-small allocation, we prefer the steppers over
   * the trough/slider, probably the steppers are a more useful
   * feature in small spaces.
   *
   * Also, we prefer to draw the range itself rather than the border
   * areas if there's a conflict, since the borders will be decoration
   * not controls. Though this depends on subclasses cooperating by
   * not drawing on range->range_rect.
   */

  widget = BTK_WIDGET (range);
  layout = range->layout;
  
  btk_range_get_props (range,
                       &slider_width, &stepper_size,
                       &focus_width, &trough_border,
                       &stepper_spacing, &trough_under_steppers,
		       NULL, NULL);

  btk_range_calc_request (range, 
                          slider_width, stepper_size,
                          focus_width, trough_border, stepper_spacing,
                          &range_rect, &border, &n_steppers,
                          &has_steppers_ab, &has_steppers_cd, &slider_length);
  
  /* We never expand to fill available space in the small dimension
   * (i.e. vertical scrollbars are always a fixed width)
   */
  if (range->orientation == BTK_ORIENTATION_VERTICAL)
    {
      clamp_dimensions (widget, &range_rect, &border, TRUE);
    }
  else
    {
      clamp_dimensions (widget, &range_rect, &border, FALSE);
    }
  
  range_rect.x = border.left;
  range_rect.y = border.top;
  
  range->range_rect = range_rect;
  
  if (range->orientation == BTK_ORIENTATION_VERTICAL)
    {
      gint stepper_width, stepper_height;

      /* Steppers are the width of the range, and stepper_size in
       * height, or if we don't have enough height, divided equally
       * among available space.
       */
      stepper_width = range_rect.width - focus_width * 2;

      if (trough_under_steppers)
        stepper_width -= trough_border * 2;

      if (stepper_width < 1)
        stepper_width = range_rect.width; /* screw the trough border */

      if (n_steppers == 0)
        stepper_height = 0; /* avoid divide by n_steppers */
      else
        stepper_height = MIN (stepper_size, (range_rect.height / n_steppers));

      /* Stepper A */
      
      layout->stepper_a.x = range_rect.x + focus_width + trough_border * trough_under_steppers;
      layout->stepper_a.y = range_rect.y + focus_width + trough_border * trough_under_steppers;

      if (range->has_stepper_a)
        {
          layout->stepper_a.width = stepper_width;
          layout->stepper_a.height = stepper_height;
        }
      else
        {
          layout->stepper_a.width = 0;
          layout->stepper_a.height = 0;
        }

      /* Stepper B */
      
      layout->stepper_b.x = layout->stepper_a.x;
      layout->stepper_b.y = layout->stepper_a.y + layout->stepper_a.height;

      if (range->has_stepper_b)
        {
          layout->stepper_b.width = stepper_width;
          layout->stepper_b.height = stepper_height;
        }
      else
        {
          layout->stepper_b.width = 0;
          layout->stepper_b.height = 0;
        }

      /* Stepper D */

      if (range->has_stepper_d)
        {
          layout->stepper_d.width = stepper_width;
          layout->stepper_d.height = stepper_height;
        }
      else
        {
          layout->stepper_d.width = 0;
          layout->stepper_d.height = 0;
        }
      
      layout->stepper_d.x = layout->stepper_a.x;
      layout->stepper_d.y = range_rect.y + range_rect.height - layout->stepper_d.height - focus_width - trough_border * trough_under_steppers;

      /* Stepper C */

      if (range->has_stepper_c)
        {
          layout->stepper_c.width = stepper_width;
          layout->stepper_c.height = stepper_height;
        }
      else
        {
          layout->stepper_c.width = 0;
          layout->stepper_c.height = 0;
        }
      
      layout->stepper_c.x = layout->stepper_a.x;
      layout->stepper_c.y = layout->stepper_d.y - layout->stepper_c.height;

      /* Now the trough is the remaining space between steppers B and C,
       * if any, minus spacing
       */
      layout->trough.x = range_rect.x;
      layout->trough.y = layout->stepper_b.y + layout->stepper_b.height + stepper_spacing * has_steppers_ab;
      layout->trough.width = range_rect.width;
      layout->trough.height = layout->stepper_c.y - layout->trough.y - stepper_spacing * has_steppers_cd;

      /* Slider fits into the trough, with stepper_spacing on either side,
       * and the size/position based on the adjustment or fixed, depending.
       */
      layout->slider.x = layout->trough.x + focus_width + trough_border;
      layout->slider.width = layout->trough.width - (focus_width + trough_border) * 2;

      /* Compute slider position/length */
      {
        gint y, bottom, top, height;
        
        top = layout->trough.y;
        bottom = layout->trough.y + layout->trough.height;

        if (! trough_under_steppers)
          {
            top += trough_border;
            bottom -= trough_border;
          }

        /* slider height is the fraction (page_size /
         * total_adjustment_range) times the trough height in pixels
         */

	if (range->adjustment->upper - range->adjustment->lower != 0)
	  height = ((bottom - top) * (range->adjustment->page_size /
				       (range->adjustment->upper - range->adjustment->lower)));
	else
	  height = range->min_slider_size;
        
        if (height < range->min_slider_size ||
            range->slider_size_fixed)
          height = range->min_slider_size;

        height = MIN (height, layout->trough.height);
        
        y = top;
        
	if (range->adjustment->upper - range->adjustment->lower - range->adjustment->page_size != 0)
	  y += (bottom - top - height) * ((adjustment_value - range->adjustment->lower) /
					  (range->adjustment->upper - range->adjustment->lower - range->adjustment->page_size));
        
        y = CLAMP (y, top, bottom);
        
        if (should_invert (range))
          y = bottom - (y - top + height);
        
        layout->slider.y = y;
        layout->slider.height = height;

        /* These are publically exported */
        range->slider_start = layout->slider.y;
        range->slider_end = layout->slider.y + layout->slider.height;
      }
    }
  else
    {
      gint stepper_width, stepper_height;

      /* Steppers are the height of the range, and stepper_size in
       * width, or if we don't have enough width, divided equally
       * among available space.
       */
      stepper_height = range_rect.height + focus_width * 2;

      if (trough_under_steppers)
        stepper_height -= trough_border * 2;

      if (stepper_height < 1)
        stepper_height = range_rect.height; /* screw the trough border */

      if (n_steppers == 0)
        stepper_width = 0; /* avoid divide by n_steppers */
      else
        stepper_width = MIN (stepper_size, (range_rect.width / n_steppers));

      /* Stepper A */
      
      layout->stepper_a.x = range_rect.x + focus_width + trough_border * trough_under_steppers;
      layout->stepper_a.y = range_rect.y + focus_width + trough_border * trough_under_steppers;

      if (range->has_stepper_a)
        {
          layout->stepper_a.width = stepper_width;
          layout->stepper_a.height = stepper_height;
        }
      else
        {
          layout->stepper_a.width = 0;
          layout->stepper_a.height = 0;
        }

      /* Stepper B */
      
      layout->stepper_b.x = layout->stepper_a.x + layout->stepper_a.width;
      layout->stepper_b.y = layout->stepper_a.y;

      if (range->has_stepper_b)
        {
          layout->stepper_b.width = stepper_width;
          layout->stepper_b.height = stepper_height;
        }
      else
        {
          layout->stepper_b.width = 0;
          layout->stepper_b.height = 0;
        }

      /* Stepper D */

      if (range->has_stepper_d)
        {
          layout->stepper_d.width = stepper_width;
          layout->stepper_d.height = stepper_height;
        }
      else
        {
          layout->stepper_d.width = 0;
          layout->stepper_d.height = 0;
        }

      layout->stepper_d.x = range_rect.x + range_rect.width - layout->stepper_d.width - focus_width - trough_border * trough_under_steppers;
      layout->stepper_d.y = layout->stepper_a.y;


      /* Stepper C */

      if (range->has_stepper_c)
        {
          layout->stepper_c.width = stepper_width;
          layout->stepper_c.height = stepper_height;
        }
      else
        {
          layout->stepper_c.width = 0;
          layout->stepper_c.height = 0;
        }
      
      layout->stepper_c.x = layout->stepper_d.x - layout->stepper_c.width;
      layout->stepper_c.y = layout->stepper_a.y;

      /* Now the trough is the remaining space between steppers B and C,
       * if any
       */
      layout->trough.x = layout->stepper_b.x + layout->stepper_b.width + stepper_spacing * has_steppers_ab;
      layout->trough.y = range_rect.y;

      layout->trough.width = layout->stepper_c.x - layout->trough.x - stepper_spacing * has_steppers_cd;
      layout->trough.height = range_rect.height;

      /* Slider fits into the trough, with stepper_spacing on either side,
       * and the size/position based on the adjustment or fixed, depending.
       */
      layout->slider.y = layout->trough.y + focus_width + trough_border;
      layout->slider.height = layout->trough.height - (focus_width + trough_border) * 2;

      /* Compute slider position/length */
      {
        gint x, left, right, width;
        
        left = layout->trough.x;
        right = layout->trough.x + layout->trough.width;

        if (! trough_under_steppers)
          {
            left += trough_border;
            right -= trough_border;
          }

        /* slider width is the fraction (page_size /
         * total_adjustment_range) times the trough width in pixels
         */
	
	if (range->adjustment->upper - range->adjustment->lower != 0)
	  width = ((right - left) * (range->adjustment->page_size /
                                   (range->adjustment->upper - range->adjustment->lower)));
	else
	  width = range->min_slider_size;
        
        if (width < range->min_slider_size ||
            range->slider_size_fixed)
          width = range->min_slider_size;
        
        width = MIN (width, layout->trough.width);
        
        x = left;
        
	if (range->adjustment->upper - range->adjustment->lower - range->adjustment->page_size != 0)
          x += (right - left - width) * ((adjustment_value - range->adjustment->lower) /
                                         (range->adjustment->upper - range->adjustment->lower - range->adjustment->page_size));
        
        x = CLAMP (x, left, right);
        
        if (should_invert (range))
          x = right - (x - left + width);
        
        layout->slider.x = x;
        layout->slider.width = width;

        /* These are publically exported */
        range->slider_start = layout->slider.x;
        range->slider_end = layout->slider.x + layout->slider.width;
      }
    }
  
  btk_range_update_mouse_location (range);

  switch (range->layout->upper_sensitivity)
    {
    case BTK_SENSITIVITY_AUTO:
      range->layout->upper_sensitive =
        (range->adjustment->value <
         (range->adjustment->upper - range->adjustment->page_size));
      break;

    case BTK_SENSITIVITY_ON:
      range->layout->upper_sensitive = TRUE;
      break;

    case BTK_SENSITIVITY_OFF:
      range->layout->upper_sensitive = FALSE;
      break;
    }

  switch (range->layout->lower_sensitivity)
    {
    case BTK_SENSITIVITY_AUTO:
      range->layout->lower_sensitive =
        (range->adjustment->value > range->adjustment->lower);
      break;

    case BTK_SENSITIVITY_ON:
      range->layout->lower_sensitive = TRUE;
      break;

    case BTK_SENSITIVITY_OFF:
      range->layout->lower_sensitive = FALSE;
      break;
    }
}

static BdkRectangle*
get_area (BtkRange     *range,
          MouseLocation location)
{
  switch (location)
    {
    case MOUSE_STEPPER_A:
      return &range->layout->stepper_a;
    case MOUSE_STEPPER_B:
      return &range->layout->stepper_b;
    case MOUSE_STEPPER_C:
      return &range->layout->stepper_c;
    case MOUSE_STEPPER_D:
      return &range->layout->stepper_d;
    case MOUSE_TROUGH:
      return &range->layout->trough;
    case MOUSE_SLIDER:
      return &range->layout->slider;
    case MOUSE_WIDGET:
    case MOUSE_OUTSIDE:
      break;
    }

  g_warning (G_STRLOC": bug");
  return NULL;
}

static void
btk_range_calc_marks (BtkRange *range)
{
  gint i;
  
  if (!range->layout->recalc_marks)
    return;

  range->layout->recalc_marks = FALSE;

  for (i = 0; i < range->layout->n_marks; i++)
    {
      range->need_recalc = TRUE;
      btk_range_calc_layout (range, range->layout->marks[i]);
      if (range->orientation == BTK_ORIENTATION_HORIZONTAL)
        range->layout->mark_pos[i] = range->layout->slider.x + range->layout->slider.width / 2;
      else
        range->layout->mark_pos[i] = range->layout->slider.y + range->layout->slider.height / 2;
    }

  range->need_recalc = TRUE;
}

static gboolean
btk_range_real_change_value (BtkRange     *range,
                             BtkScrollType scroll,
                             gdouble       value)
{
  /* potentially adjust the bounds _before we clamp */
  g_signal_emit (range, signals[ADJUST_BOUNDS], 0, value);

  if (range->layout->restrict_to_fill_level)
    value = MIN (value, MAX (range->adjustment->lower,
                             range->layout->fill_level));

  value = CLAMP (value, range->adjustment->lower,
                 (range->adjustment->upper - range->adjustment->page_size));

  if (range->round_digits >= 0)
    {
      gdouble power;
      gint i;

      i = range->round_digits;
      power = 1;
      while (i--)
        power *= 10;
      
      value = floor ((value * power) + 0.5) / power;
    }
  
  if (range->adjustment->value != value)
    {
      range->need_recalc = TRUE;

      btk_widget_queue_draw (BTK_WIDGET (range));
      
      switch (range->update_policy)
        {
        case BTK_UPDATE_CONTINUOUS:
          btk_adjustment_set_value (range->adjustment, value);
          break;

          /* Delayed means we update after a period of inactivity */
        case BTK_UPDATE_DELAYED:
          btk_range_reset_update_timer (range);
          /* FALL THRU */

          /* Discontinuous means we update on button release */
        case BTK_UPDATE_DISCONTINUOUS:
          /* don't emit value_changed signal */
          range->adjustment->value = value;
          range->update_pending = TRUE;
          break;
        }
    }
  return FALSE;
}

static void
btk_range_update_value (BtkRange *range)
{
  btk_range_remove_update_timer (range);
  
  if (range->update_pending)
    {
      btk_adjustment_value_changed (range->adjustment);

      range->update_pending = FALSE;
    }
}

struct _BtkRangeStepTimer
{
  guint timeout_id;
  BtkScrollType step;
};

static gboolean
second_timeout (gpointer data)
{
  BtkRange *range;

  range = BTK_RANGE (data);
  btk_range_scroll (range, range->timer->step);
  
  return TRUE;
}

static gboolean
initial_timeout (gpointer data)
{
  BtkRange    *range;
  BtkSettings *settings;
  guint        timeout;

  settings = btk_widget_get_settings (BTK_WIDGET (data));
  g_object_get (settings, "btk-timeout-repeat", &timeout, NULL);

  range = BTK_RANGE (data);
  range->timer->timeout_id = bdk_threads_add_timeout (timeout * SCROLL_DELAY_FACTOR,
                                            second_timeout,
                                            range);
  /* remove self */
  return FALSE;
}

static void
btk_range_add_step_timer (BtkRange      *range,
                          BtkScrollType  step)
{
  BtkSettings *settings;
  guint        timeout;

  g_return_if_fail (range->timer == NULL);
  g_return_if_fail (step != BTK_SCROLL_NONE);

  settings = btk_widget_get_settings (BTK_WIDGET (range));
  g_object_get (settings, "btk-timeout-initial", &timeout, NULL);

  range->timer = g_new (BtkRangeStepTimer, 1);

  range->timer->timeout_id = bdk_threads_add_timeout (timeout,
                                            initial_timeout,
                                            range);
  range->timer->step = step;

  btk_range_scroll (range, range->timer->step);
}

static void
btk_range_remove_step_timer (BtkRange *range)
{
  if (range->timer)
    {
      if (range->timer->timeout_id != 0)
        g_source_remove (range->timer->timeout_id);

      g_free (range->timer);

      range->timer = NULL;
    }
}

static gboolean
update_timeout (gpointer data)
{
  BtkRange *range;

  range = BTK_RANGE (data);
  btk_range_update_value (range);
  range->update_timeout_id = 0;

  /* self-remove */
  return FALSE;
}

static void
btk_range_reset_update_timer (BtkRange *range)
{
  btk_range_remove_update_timer (range);

  range->update_timeout_id = bdk_threads_add_timeout (UPDATE_DELAY,
                                            update_timeout,
                                            range);
}

static void
btk_range_remove_update_timer (BtkRange *range)
{
  if (range->update_timeout_id != 0)
    {
      g_source_remove (range->update_timeout_id);
      range->update_timeout_id = 0;
    }
}

void
_btk_range_set_stop_values (BtkRange *range,
                            gdouble  *values,
                            gint      n_values)
{
  gint i;

  g_free (range->layout->marks);
  range->layout->marks = g_new (gdouble, n_values);

  g_free (range->layout->mark_pos);
  range->layout->mark_pos = g_new (gint, n_values);

  range->layout->n_marks = n_values;

  for (i = 0; i < n_values; i++) 
    range->layout->marks[i] = values[i];

  range->layout->recalc_marks = TRUE;
}

gint
_btk_range_get_stop_positions (BtkRange  *range,
                               gint     **values)
{
  btk_range_calc_marks (range);

  if (values)
    *values = g_memdup (range->layout->mark_pos, range->layout->n_marks * sizeof (gint));

  return range->layout->n_marks;
}

/**
 * btk_range_set_round_digits:
 * @range: a #BtkRange
 * @round_digits: the precision in digits, or -1
 *
 * Sets the number of digits to round the value to when
 * it changes. See #BtkRange::change-value.
 *
 * Since: 2.24
 */
void
btk_range_set_round_digits (BtkRange *range,
                            gint      round_digits)
{
  g_return_if_fail (BTK_IS_RANGE (range));
  g_return_if_fail (round_digits >= -1);

  range->round_digits = round_digits;

  g_object_notify (G_OBJECT (range), "round-digits");
}

/**
 * btk_range_get_round_digits:
 * @range: a #BtkRange
 *
 * Gets the number of digits to round the value to when
 * it changes. See #BtkRange::change-value.
 *
 * Return value: the number of digits to round to
 *
 * Since: 2.24
 */
gint
btk_range_get_round_digits (BtkRange *range)
{
  g_return_val_if_fail (BTK_IS_RANGE (range), -1);

  return range->round_digits;
}


#define __BTK_RANGE_C__
#include "btkaliasdef.c"
