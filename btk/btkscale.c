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
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#include "config.h"

#include <math.h>
#include <stdlib.h>

#include "bdk/bdkkeysyms.h"
#include "btkscale.h"
#include "btkiconfactory.h"
#include "btkicontheme.h"
#include "btkmarshalers.h"
#include "btkbindings.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkbuildable.h"
#include "btkbuilderprivate.h"
#include "btkalias.h"


#define	MAX_DIGITS	(64)	/* don't change this,
				 * a) you don't need to and
				 * b) you might cause buffer owerflows in
				 *    unrelated code portions otherwise
				 */

#define BTK_SCALE_GET_PRIVATE(obj) (B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_SCALE, BtkScalePrivate))

typedef struct _BtkScalePrivate BtkScalePrivate;

typedef struct _BtkScaleMark BtkScaleMark;

struct _BtkScaleMark
{
  gdouble          value;
  gchar           *markup;
  BtkPositionType  position;
};

struct _BtkScalePrivate
{
  BangoLayout *layout;
  GSList      *marks;
};

enum {
  PROP_0,
  PROP_DIGITS,
  PROP_DRAW_VALUE,
  PROP_VALUE_POS
};

enum {
  FORMAT_VALUE,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static void     btk_scale_set_property            (BObject        *object,
                                                   guint           prop_id,
                                                   const BValue   *value,
                                                   BParamSpec     *pspec);
static void     btk_scale_get_property            (BObject        *object,
                                                   guint           prop_id,
                                                   BValue         *value,
                                                   BParamSpec     *pspec);
static void     btk_scale_size_request            (BtkWidget      *widget,
                                                   BtkRequisition *requisition);
static void     btk_scale_style_set               (BtkWidget      *widget,
                                                   BtkStyle       *previous);
static void     btk_scale_get_range_border        (BtkRange       *range,
                                                   BtkBorder      *border);
static void     btk_scale_get_mark_label_size     (BtkScale        *scale,
                                                   BtkPositionType  position,
                                                   gint            *count1,
                                                   gint            *width1,
                                                   gint            *height1,
                                                   gint            *count2,
                                                   gint            *width2,
                                                   gint            *height2);
static void     btk_scale_finalize                (BObject        *object);
static void     btk_scale_screen_changed          (BtkWidget      *widget,
                                                   BdkScreen      *old_screen);
static gboolean btk_scale_expose                  (BtkWidget      *widget,
                                                   BdkEventExpose *event);
static void     btk_scale_real_get_layout_offsets (BtkScale       *scale,
                                                   gint           *x,
                                                   gint           *y);
static void     btk_scale_buildable_interface_init   (BtkBuildableIface *iface);
static gboolean btk_scale_buildable_custom_tag_start (BtkBuildable  *buildable,
                                                      BtkBuilder    *builder,
                                                      BObject       *child,
                                                      const gchar   *tagname,
                                                      GMarkupParser *parser,
                                                      gpointer      *data);
static void     btk_scale_buildable_custom_finished  (BtkBuildable  *buildable,
                                                      BtkBuilder    *builder,
                                                      BObject       *child,
                                                      const gchar   *tagname,
                                                      gpointer       user_data);


G_DEFINE_ABSTRACT_TYPE_WITH_CODE (BtkScale, btk_scale, BTK_TYPE_RANGE,
                                  G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
                                                         btk_scale_buildable_interface_init))


static gint
compare_marks (gconstpointer a, gconstpointer b, gpointer data)
{
  gboolean inverted = GPOINTER_TO_INT (data);
  const BtkScaleMark *ma, *mb;
  gint val;

  val = inverted ? -1 : 1;
  ma = a; mb = b;

  return (ma->value > mb->value) ? val : ((ma->value < mb->value) ? -val : 0);
}

static void
btk_scale_notify (BObject    *object,
                  BParamSpec *pspec)
{
  if (strcmp (pspec->name, "orientation") == 0)
    {
      BtkRange *range = BTK_RANGE (object);

      range->flippable = (range->orientation == BTK_ORIENTATION_HORIZONTAL);
    }
  else if (strcmp (pspec->name, "inverted") == 0)
    {
      BtkScalePrivate *priv = BTK_SCALE_GET_PRIVATE (object);
      BtkScaleMark *mark;
      GSList *m;
      gint i, n;
      gdouble *values;

      priv->marks = b_slist_sort_with_data (priv->marks,
                                            compare_marks,
                                            GINT_TO_POINTER (btk_range_get_inverted (BTK_RANGE (object))));

      n = b_slist_length (priv->marks);
      values = g_new (gdouble, n);
      for (m = priv->marks, i = 0; m; m = m->next, i++)
        {
          mark = m->data;
          values[i] = mark->value;
        }

      _btk_range_set_stop_values (BTK_RANGE (object), values, n);

      g_free (values);
    }

  if (B_OBJECT_CLASS (btk_scale_parent_class)->notify)
    B_OBJECT_CLASS (btk_scale_parent_class)->notify (object, pspec);
}


static gboolean
single_string_accumulator (GSignalInvocationHint *ihint,
                           BValue                *return_accu,
                           const BValue          *handler_return,
                           gpointer               dummy)
{
  gboolean continue_emission;
  const gchar *str;
  
  str = b_value_get_string (handler_return);
  b_value_set_string (return_accu, str);
  continue_emission = str == NULL;
  
  return continue_emission;
}


#define add_slider_binding(binding_set, keyval, mask, scroll)              \
  btk_binding_entry_add_signal (binding_set, keyval, mask,                 \
                                I_("move-slider"), 1, \
                                BTK_TYPE_SCROLL_TYPE, scroll)

static void
btk_scale_class_init (BtkScaleClass *class)
{
  BObjectClass   *bobject_class;
  BtkWidgetClass *widget_class;
  BtkRangeClass  *range_class;
  BtkBindingSet  *binding_set;
  
  bobject_class = B_OBJECT_CLASS (class);
  range_class = (BtkRangeClass*) class;
  widget_class = (BtkWidgetClass*) class;
  
  bobject_class->set_property = btk_scale_set_property;
  bobject_class->get_property = btk_scale_get_property;
  bobject_class->notify = btk_scale_notify;
  bobject_class->finalize = btk_scale_finalize;

  widget_class->style_set = btk_scale_style_set;
  widget_class->screen_changed = btk_scale_screen_changed;
  widget_class->expose_event = btk_scale_expose;
  widget_class->size_request = btk_scale_size_request;

  range_class->slider_detail = "Xscale";
  range_class->get_range_border = btk_scale_get_range_border;

  class->get_layout_offsets = btk_scale_real_get_layout_offsets;

  /**
   * BtkScale::format-value:
   * @scale: the object which received the signal
   * @value: the value to format
   *
   * Signal which allows you to change how the scale value is displayed.
   * Connect a signal handler which returns an allocated string representing 
   * @value. That string will then be used to display the scale's value.
   *
   * If no user-provided handlers are installed, the value will be displayed on
   * its own, rounded according to the value of the #BtkScale:digits property.
   *
   * Here's an example signal handler which displays a value 1.0 as
   * with "--&gt;1.0&lt;--".
   * |[
   * static gchar*
   * format_value_callback (BtkScale *scale,
   *                        gdouble   value)
   * {
   *   return g_strdup_printf ("--&gt;&percnt;0.*g&lt;--",
   *                           btk_scale_get_digits (scale), value);
   *  }
   * ]|
   *
   * Return value: allocated string representing @value
   */
  signals[FORMAT_VALUE] =
    g_signal_new (I_("format-value"),
                  B_TYPE_FROM_CLASS (bobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkScaleClass, format_value),
                  single_string_accumulator, NULL,
                  _btk_marshal_STRING__DOUBLE,
                  B_TYPE_STRING, 1,
                  B_TYPE_DOUBLE);

  g_object_class_install_property (bobject_class,
                                   PROP_DIGITS,
                                   g_param_spec_int ("digits",
						     P_("Digits"),
						     P_("The number of decimal places that are displayed in the value"),
						     -1,
						     MAX_DIGITS,
						     1,
						     BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_DRAW_VALUE,
                                   g_param_spec_boolean ("draw-value",
							 P_("Draw Value"),
							 P_("Whether the current value is displayed as a string next to the slider"),
							 TRUE,
							 BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_VALUE_POS,
                                   g_param_spec_enum ("value-pos",
						      P_("Value Position"),
						      P_("The position in which the current value is displayed"),
						      BTK_TYPE_POSITION_TYPE,
						      BTK_POS_TOP,
						      BTK_PARAM_READWRITE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("slider-length",
							     P_("Slider Length"),
							     P_("Length of scale's slider"),
							     0,
							     G_MAXINT,
							     31,
							     BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("value-spacing",
							     P_("Value spacing"),
							     P_("Space between value text and the slider/trough area"),
							     0,
							     G_MAXINT,
							     2,
							     BTK_PARAM_READABLE));
  
  /* All bindings (even arrow keys) are on both h/v scale, because
   * blind users etc. don't care about scale orientation.
   */
  
  binding_set = btk_binding_set_by_class (class);

  add_slider_binding (binding_set, BDK_Left, 0,
                      BTK_SCROLL_STEP_LEFT);

  add_slider_binding (binding_set, BDK_Left, BDK_CONTROL_MASK,
                      BTK_SCROLL_PAGE_LEFT);

  add_slider_binding (binding_set, BDK_KP_Left, 0,
                      BTK_SCROLL_STEP_LEFT);

  add_slider_binding (binding_set, BDK_KP_Left, BDK_CONTROL_MASK,
                      BTK_SCROLL_PAGE_LEFT);

  add_slider_binding (binding_set, BDK_Right, 0,
                      BTK_SCROLL_STEP_RIGHT);

  add_slider_binding (binding_set, BDK_Right, BDK_CONTROL_MASK,
                      BTK_SCROLL_PAGE_RIGHT);

  add_slider_binding (binding_set, BDK_KP_Right, 0,
                      BTK_SCROLL_STEP_RIGHT);

  add_slider_binding (binding_set, BDK_KP_Right, BDK_CONTROL_MASK,
                      BTK_SCROLL_PAGE_RIGHT);

  add_slider_binding (binding_set, BDK_Up, 0,
                      BTK_SCROLL_STEP_UP);

  add_slider_binding (binding_set, BDK_Up, BDK_CONTROL_MASK,
                      BTK_SCROLL_PAGE_UP);

  add_slider_binding (binding_set, BDK_KP_Up, 0,
                      BTK_SCROLL_STEP_UP);

  add_slider_binding (binding_set, BDK_KP_Up, BDK_CONTROL_MASK,
                      BTK_SCROLL_PAGE_UP);

  add_slider_binding (binding_set, BDK_Down, 0,
                      BTK_SCROLL_STEP_DOWN);

  add_slider_binding (binding_set, BDK_Down, BDK_CONTROL_MASK,
                      BTK_SCROLL_PAGE_DOWN);

  add_slider_binding (binding_set, BDK_KP_Down, 0,
                      BTK_SCROLL_STEP_DOWN);

  add_slider_binding (binding_set, BDK_KP_Down, BDK_CONTROL_MASK,
                      BTK_SCROLL_PAGE_DOWN);
   
  add_slider_binding (binding_set, BDK_Page_Up, BDK_CONTROL_MASK,
                      BTK_SCROLL_PAGE_LEFT);

  add_slider_binding (binding_set, BDK_KP_Page_Up, BDK_CONTROL_MASK,
                      BTK_SCROLL_PAGE_LEFT);  

  add_slider_binding (binding_set, BDK_Page_Up, 0,
                      BTK_SCROLL_PAGE_UP);

  add_slider_binding (binding_set, BDK_KP_Page_Up, 0,
                      BTK_SCROLL_PAGE_UP);
  
  add_slider_binding (binding_set, BDK_Page_Down, BDK_CONTROL_MASK,
                      BTK_SCROLL_PAGE_RIGHT);

  add_slider_binding (binding_set, BDK_KP_Page_Down, BDK_CONTROL_MASK,
                      BTK_SCROLL_PAGE_RIGHT);

  add_slider_binding (binding_set, BDK_Page_Down, 0,
                      BTK_SCROLL_PAGE_DOWN);

  add_slider_binding (binding_set, BDK_KP_Page_Down, 0,
                      BTK_SCROLL_PAGE_DOWN);

  /* Logical bindings (vs. visual bindings above) */

  add_slider_binding (binding_set, BDK_plus, 0,
                      BTK_SCROLL_STEP_FORWARD);  

  add_slider_binding (binding_set, BDK_minus, 0,
                      BTK_SCROLL_STEP_BACKWARD);  

  add_slider_binding (binding_set, BDK_plus, BDK_CONTROL_MASK,
                      BTK_SCROLL_PAGE_FORWARD);  

  add_slider_binding (binding_set, BDK_minus, BDK_CONTROL_MASK,
                      BTK_SCROLL_PAGE_BACKWARD);


  add_slider_binding (binding_set, BDK_KP_Add, 0,
                      BTK_SCROLL_STEP_FORWARD);  

  add_slider_binding (binding_set, BDK_KP_Subtract, 0,
                      BTK_SCROLL_STEP_BACKWARD);  

  add_slider_binding (binding_set, BDK_KP_Add, BDK_CONTROL_MASK,
                      BTK_SCROLL_PAGE_FORWARD);  

  add_slider_binding (binding_set, BDK_KP_Subtract, BDK_CONTROL_MASK,
                      BTK_SCROLL_PAGE_BACKWARD);
  
  
  add_slider_binding (binding_set, BDK_Home, 0,
                      BTK_SCROLL_START);

  add_slider_binding (binding_set, BDK_KP_Home, 0,
                      BTK_SCROLL_START);

  add_slider_binding (binding_set, BDK_End, 0,
                      BTK_SCROLL_END);

  add_slider_binding (binding_set, BDK_KP_End, 0,
                      BTK_SCROLL_END);

  g_type_class_add_private (bobject_class, sizeof (BtkScalePrivate));
}

static void
btk_scale_init (BtkScale *scale)
{
  BtkRange *range = BTK_RANGE (scale);

  btk_widget_set_can_focus (BTK_WIDGET (scale), TRUE);

  range->slider_size_fixed = TRUE;
  range->has_stepper_a = FALSE;
  range->has_stepper_b = FALSE;
  range->has_stepper_c = FALSE;
  range->has_stepper_d = FALSE;

  scale->draw_value = TRUE;
  scale->value_pos = BTK_POS_TOP;
  scale->digits = 1;
  range->round_digits = scale->digits;

  range->flippable = (range->orientation == BTK_ORIENTATION_HORIZONTAL);
}

static void
btk_scale_set_property (BObject      *object,
			guint         prop_id,
			const BValue *value,
			BParamSpec   *pspec)
{
  BtkScale *scale;

  scale = BTK_SCALE (object);

  switch (prop_id)
    {
    case PROP_DIGITS:
      btk_scale_set_digits (scale, b_value_get_int (value));
      break;
    case PROP_DRAW_VALUE:
      btk_scale_set_draw_value (scale, b_value_get_boolean (value));
      break;
    case PROP_VALUE_POS:
      btk_scale_set_value_pos (scale, b_value_get_enum (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_scale_get_property (BObject      *object,
			guint         prop_id,
			BValue       *value,
			BParamSpec   *pspec)
{
  BtkScale *scale;

  scale = BTK_SCALE (object);

  switch (prop_id)
    {
    case PROP_DIGITS:
      b_value_set_int (value, scale->digits);
      break;
    case PROP_DRAW_VALUE:
      b_value_set_boolean (value, scale->draw_value);
      break;
    case PROP_VALUE_POS:
      b_value_set_enum (value, scale->value_pos);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

#if 0
/**
 * btk_scale_new:
 * @orientation: the scale's orientation.
 * @adjustment: the #BtkAdjustment which sets the range of the scale, or
 *              %NULL to create a new adjustment.
 *
 * Creates a new #BtkScale.
 *
 * Return value: a new #BtkScale
 *
 * Since: 2.16
 **/
BtkWidget *
btk_scale_new (BtkOrientation  orientation,
               BtkAdjustment  *adjustment)
{
  g_return_val_if_fail (adjustment == NULL || BTK_IS_ADJUSTMENT (adjustment),
                        NULL);

  return g_object_new (BTK_TYPE_SCALE,
                       "orientation", orientation,
                       "adjustment",  adjustment,
                       NULL);
}

/**
 * btk_scale_new_with_range:
 * @orientation: the scale's orientation.
 * @min: minimum value
 * @max: maximum value
 * @step: step increment (tick size) used with keyboard shortcuts
 *
 * Creates a new scale widget with the given orientation that lets the
 * user input a number between @min and @max (including @min and @max)
 * with the increment @step.  @step must be nonzero; it's the distance
 * the slider moves when using the arrow keys to adjust the scale
 * value.
 *
 * Note that the way in which the precision is derived works best if @step
 * is a power of ten. If the resulting precision is not suitable for your
 * needs, use btk_scale_set_digits() to correct it.
 *
 * Return value: a new #BtkScale
 *
 * Since: 2.16
 **/
BtkWidget *
btk_scale_new_with_range (BtkOrientation orientation,
                          gdouble        min,
                          gdouble        max,
                          gdouble        step)
{
  BtkObject *adj;
  gint digits;

  g_return_val_if_fail (min < max, NULL);
  g_return_val_if_fail (step != 0.0, NULL);

  adj = btk_adjustment_new (min, min, max, step, 10 * step, 0);

  if (fabs (step) >= 1.0 || step == 0.0)
    {
      digits = 0;
    }
  else
    {
      digits = abs ((gint) floor (log10 (fabs (step))));
      if (digits > 5)
        digits = 5;
    }

  return g_object_new (BTK_TYPE_SCALE,
                       "orientation", orientation,
                       "adjustment",  adj,
                       "digits",      digits,
                       NULL);
}
#endif

/**
 * btk_scale_set_digits:
 * @scale: a #BtkScale
 * @digits: the number of decimal places to display, 
 *     e.g. use 1 to display 1.0, 2 to display 1.00, etc
 * 
 * Sets the number of decimal places that are displayed in the value. Also
 * causes the value of the adjustment to be rounded to this number of digits,
 * so the retrieved value matches the displayed one, if #BtkScale:draw-value is
 * %TRUE when the value changes. If you want to enforce rounding the value when
 * #BtkScale:draw-value is %FALSE, you can set #BtkRange:round-digits instead.
 *
 */
void
btk_scale_set_digits (BtkScale *scale,
		      gint      digits)
{
  BtkRange *range;
  
  g_return_if_fail (BTK_IS_SCALE (scale));

  range = BTK_RANGE (scale);
  
  digits = CLAMP (digits, -1, MAX_DIGITS);

  if (scale->digits != digits)
    {
      scale->digits = digits;
      if (scale->draw_value)
	range->round_digits = digits;
      
      _btk_scale_clear_layout (scale);
      btk_widget_queue_resize (BTK_WIDGET (scale));

      g_object_notify (B_OBJECT (scale), "digits");
    }
}

/**
 * btk_scale_get_digits:
 * @scale: a #BtkScale
 *
 * Gets the number of decimal places that are displayed in the value.
 *
 * Returns: the number of decimal places that are displayed
 */
gint
btk_scale_get_digits (BtkScale *scale)
{
  g_return_val_if_fail (BTK_IS_SCALE (scale), -1);

  return scale->digits;
}

/**
 * btk_scale_set_draw_value:
 * @scale: a #BtkScale
 * @draw_value: %TRUE to draw the value
 * 
 * Specifies whether the current value is displayed as a string next 
 * to the slider.
 */
void
btk_scale_set_draw_value (BtkScale *scale,
			  gboolean  draw_value)
{
  g_return_if_fail (BTK_IS_SCALE (scale));

  draw_value = draw_value != FALSE;

  if (scale->draw_value != draw_value)
    {
      scale->draw_value = draw_value;
      if (draw_value)
	BTK_RANGE (scale)->round_digits = scale->digits;
      else
	BTK_RANGE (scale)->round_digits = -1;

      _btk_scale_clear_layout (scale);

      btk_widget_queue_resize (BTK_WIDGET (scale));

      g_object_notify (B_OBJECT (scale), "draw-value");
    }
}

/**
 * btk_scale_get_draw_value:
 * @scale: a #BtkScale
 *
 * Returns whether the current value is displayed as a string 
 * next to the slider.
 *
 * Returns: whether the current value is displayed as a string
 */
gboolean
btk_scale_get_draw_value (BtkScale *scale)
{
  g_return_val_if_fail (BTK_IS_SCALE (scale), FALSE);

  return scale->draw_value;
}

/**
 * btk_scale_set_value_pos:
 * @scale: a #BtkScale
 * @pos: the position in which the current value is displayed
 * 
 * Sets the position in which the current value is displayed.
 */
void
btk_scale_set_value_pos (BtkScale        *scale,
			 BtkPositionType  pos)
{
  BtkWidget *widget;

  g_return_if_fail (BTK_IS_SCALE (scale));

  if (scale->value_pos != pos)
    {
      scale->value_pos = pos;
      widget = BTK_WIDGET (scale);

      _btk_scale_clear_layout (scale);
      if (btk_widget_get_visible (widget) && btk_widget_get_mapped (widget))
	btk_widget_queue_resize (widget);

      g_object_notify (B_OBJECT (scale), "value-pos");
    }
}

/**
 * btk_scale_get_value_pos:
 * @scale: a #BtkScale
 *
 * Gets the position in which the current value is displayed.
 *
 * Returns: the position in which the current value is displayed
 */
BtkPositionType
btk_scale_get_value_pos (BtkScale *scale)
{
  g_return_val_if_fail (BTK_IS_SCALE (scale), 0);

  return scale->value_pos;
}

static void
btk_scale_get_range_border (BtkRange  *range,
                            BtkBorder *border)
{
  BtkScalePrivate *priv;
  BtkWidget *widget;
  BtkScale *scale;
  gint w, h;
  
  widget = BTK_WIDGET (range);
  scale = BTK_SCALE (range);
  priv = BTK_SCALE_GET_PRIVATE (scale);

  _btk_scale_get_value_size (scale, &w, &h);

  border->left = 0;
  border->right = 0;
  border->top = 0;
  border->bottom = 0;

  if (scale->draw_value)
    {
      gint value_spacing;
      btk_widget_style_get (widget, "value-spacing", &value_spacing, NULL);

      switch (scale->value_pos)
        {
        case BTK_POS_LEFT:
          border->left += w + value_spacing;
          break;
        case BTK_POS_RIGHT:
          border->right += w + value_spacing;
          break;
        case BTK_POS_TOP:
          border->top += h + value_spacing;
          break;
        case BTK_POS_BOTTOM:
          border->bottom += h + value_spacing;
          break;
        }
    }

  if (priv->marks)
    {
      gint slider_width;
      gint value_spacing;
      gint n1, w1, h1, n2, w2, h2;
  
      btk_widget_style_get (widget, 
                            "slider-width", &slider_width,
                            "value-spacing", &value_spacing, 
                            NULL);


      if (BTK_RANGE (scale)->orientation == BTK_ORIENTATION_HORIZONTAL)
        {
          btk_scale_get_mark_label_size (scale, BTK_POS_TOP, &n1, &w1, &h1, &n2, &w2, &h2);
          if (n1 > 0)
            border->top += h1 + value_spacing + slider_width / 2;
          if (n2 > 0)
            border->bottom += h2 + value_spacing + slider_width / 2; 
        }
      else
        {
          btk_scale_get_mark_label_size (scale, BTK_POS_LEFT, &n1, &w1, &h1, &n2, &w2, &h2);
          if (n1 > 0)
            border->left += w1 + value_spacing + slider_width / 2;
          if (n2 > 0)
            border->right += w2 + value_spacing + slider_width / 2;
        }
    }
}

/* FIXME this could actually be static at the moment. */
void
_btk_scale_get_value_size (BtkScale *scale,
                           gint     *width,
                           gint     *height)
{
  BtkRange *range;

  g_return_if_fail (BTK_IS_SCALE (scale));

  if (scale->draw_value)
    {
      BangoLayout *layout;
      BangoRectangle logical_rect;
      gchar *txt;
      
      range = BTK_RANGE (scale);

      layout = btk_widget_create_bango_layout (BTK_WIDGET (scale), NULL);

      txt = _btk_scale_format_value (scale, range->adjustment->lower);
      bango_layout_set_text (layout, txt, -1);
      g_free (txt);
      
      bango_layout_get_pixel_extents (layout, NULL, &logical_rect);

      if (width)
	*width = logical_rect.width;
      if (height)
	*height = logical_rect.height;

      txt = _btk_scale_format_value (scale, range->adjustment->upper);
      bango_layout_set_text (layout, txt, -1);
      g_free (txt);
      
      bango_layout_get_pixel_extents (layout, NULL, &logical_rect);

      if (width)
	*width = MAX (*width, logical_rect.width);
      if (height)
	*height = MAX (*height, logical_rect.height);

      g_object_unref (layout);
    }
  else
    {
      if (width)
	*width = 0;
      if (height)
	*height = 0;
    }

}

static void
btk_scale_get_mark_label_size (BtkScale        *scale,
                               BtkPositionType  position,
                               gint            *count1,
                               gint            *width1,
                               gint            *height1,
                               gint            *count2,
                               gint            *width2,
                               gint            *height2)
{
  BtkScalePrivate *priv = BTK_SCALE_GET_PRIVATE (scale);
  BangoLayout *layout;
  BangoRectangle logical_rect;
  GSList *m;
  gint w, h;

  *count1 = *count2 = 0;
  *width1 = *width2 = 0;
  *height1 = *height2 = 0;

  layout = btk_widget_create_bango_layout (BTK_WIDGET (scale), NULL);

  for (m = priv->marks; m; m = m->next)
    {
      BtkScaleMark *mark = m->data;

      if (mark->markup)
        {
          bango_layout_set_markup (layout, mark->markup, -1);
          bango_layout_get_pixel_extents (layout, NULL, &logical_rect);

	  w = logical_rect.width;
	  h = logical_rect.height;
        }
      else
        {
          w = 0;
          h = 0;
        }

      if (mark->position == position)
        {
          (*count1)++;
          *width1 = MAX (*width1, w);
          *height1 = MAX (*height1, h);
        }
      else
        {
          (*count2)++;
          *width2 = MAX (*width2, w);
          *height2 = MAX (*height2, h);
        }
    }

  g_object_unref (layout);
}

static void
btk_scale_style_set (BtkWidget *widget,
                     BtkStyle  *previous)
{
  gint slider_length;
  BtkRange *range;

  range = BTK_RANGE (widget);
  
  btk_widget_style_get (widget,
                        "slider-length", &slider_length,
                        NULL);
  
  range->min_slider_size = slider_length;
  
  _btk_scale_clear_layout (BTK_SCALE (widget));

  BTK_WIDGET_CLASS (btk_scale_parent_class)->style_set (widget, previous);
}

static void
btk_scale_screen_changed (BtkWidget *widget,
                          BdkScreen *old_screen)
{
  _btk_scale_clear_layout (BTK_SCALE (widget));
}

static void
btk_scale_size_request (BtkWidget      *widget,
                        BtkRequisition *requisition)
{
  BtkRange *range = BTK_RANGE (widget);
  gint n1, w1, h1, n2, w2, h2;
  gint slider_length;

  BTK_WIDGET_CLASS (btk_scale_parent_class)->size_request (widget, requisition);
  
  btk_widget_style_get (widget, "slider-length", &slider_length, NULL);


  if (range->orientation == BTK_ORIENTATION_HORIZONTAL)
    {
      btk_scale_get_mark_label_size (BTK_SCALE (widget), BTK_POS_TOP, &n1, &w1, &h1, &n2, &w2, &h2);

      w1 = (n1 - 1) * w1 + MAX (w1, slider_length);
      w2 = (n2 - 1) * w2 + MAX (w2, slider_length);
      requisition->width = MAX (requisition->width, MAX (w1, w2));
    }
  else
    {
      btk_scale_get_mark_label_size (BTK_SCALE (widget), BTK_POS_LEFT, &n1, &w1, &h1, &n2, &w2, &h2);
      h1 = (n1 - 1) * h1 + MAX (h1, slider_length);
      h2 = (n2 - 1) * h1 + MAX (h2, slider_length);
      requisition->height = MAX (requisition->height, MAX (h1, h2));
    }
}

static gint
find_next_pos (BtkWidget      *widget,
               GSList          *list,
               gint            *marks,
               BtkPositionType  pos)
{
  GSList *m;
  gint i;

  for (m = list->next, i = 1; m; m = m->next, i++)
    {
      BtkScaleMark *mark = m->data;

      if (mark->position == pos)
        return marks[i];
    }
    
  if (BTK_RANGE(widget)->orientation == BTK_ORIENTATION_HORIZONTAL)
    return widget->allocation.width;
  else
    return widget->allocation.height;
}

static gboolean
btk_scale_expose (BtkWidget      *widget,
                  BdkEventExpose *event)
{
  BtkScale *scale = BTK_SCALE (widget);
  BtkScalePrivate *priv = BTK_SCALE_GET_PRIVATE (scale);
  BtkRange *range = BTK_RANGE (scale);
  BtkStateType state_type;
  gint *marks;
  gint focus_padding;
  gint slider_width;
  gint value_spacing;
  gint min_sep = 4;

  btk_widget_style_get (widget,
                        "focus-padding", &focus_padding,
                        "slider-width", &slider_width, 
                        "value-spacing", &value_spacing, 
                        NULL);

  /* We need to chain up _first_ so the various geometry members of
   * BtkRange struct are updated.
   */
  BTK_WIDGET_CLASS (btk_scale_parent_class)->expose_event (widget, event);

  state_type = BTK_STATE_NORMAL;
  if (!btk_widget_is_sensitive (widget))
    state_type = BTK_STATE_INSENSITIVE;

  if (priv->marks)
    {
      gint i;
      gint x1, x2, x3, y1, y2, y3;
      BangoLayout *layout;
      BangoRectangle logical_rect;
      GSList *m;
      gint min_pos_before, min_pos_after;
      gint min_pos, max_pos;

      _btk_range_get_stop_positions (range, &marks);

      layout = btk_widget_create_bango_layout (widget, NULL);

      if (range->orientation == BTK_ORIENTATION_HORIZONTAL)
        min_pos_before = min_pos_after = widget->allocation.x;
      else
        min_pos_before = min_pos_after = widget->allocation.y;
      for (m = priv->marks, i = 0; m; m = m->next, i++)
        {
          BtkScaleMark *mark = m->data;
    
          if (range->orientation == BTK_ORIENTATION_HORIZONTAL)
            {
              x1 = widget->allocation.x + marks[i];
              if (mark->position == BTK_POS_TOP)
                {
                  y1 = widget->allocation.y + range->range_rect.y;
                  y2 = y1 - slider_width / 2;
                  min_pos = min_pos_before;
                  max_pos = widget->allocation.x + find_next_pos (widget, m, marks + i, BTK_POS_TOP) - min_sep;
                }
              else
                {
                  y1 = widget->allocation.y + range->range_rect.y + range->range_rect.height;
                  y2 = y1 + slider_width / 2;
                  min_pos = min_pos_after;
                  max_pos = widget->allocation.x + find_next_pos (widget, m, marks + i, BTK_POS_BOTTOM) - min_sep;
                }

              btk_paint_vline (widget->style, widget->window, state_type,
                               NULL, widget, "scale-mark", y1, y2, x1);

              if (mark->markup)
                {
                  bango_layout_set_markup (layout, mark->markup, -1);
                  bango_layout_get_pixel_extents (layout, NULL, &logical_rect);

                  x3 = x1 - logical_rect.width / 2;
                  if (x3 < min_pos)
                    x3 = min_pos;
                  if (x3 + logical_rect.width > max_pos)
                        x3 = max_pos - logical_rect.width;
                  if (x3 < widget->allocation.x)
                     x3 = widget->allocation.x;
                  if (mark->position == BTK_POS_TOP)
                    {
                      y3 = y2 - value_spacing - logical_rect.height;
                      min_pos_before = x3 + logical_rect.width + min_sep;
                    }
                  else
                    {
                      y3 = y2 + value_spacing;
                      min_pos_after = x3 + logical_rect.width + min_sep;
                    }

                  btk_paint_layout (widget->style, widget->window, state_type,
                                    FALSE, NULL, widget, "scale-mark",
                                    x3, y3, layout);
                }
            }
          else
            {
              if (mark->position == BTK_POS_LEFT)
                {
                  x1 = widget->allocation.x + range->range_rect.x;
                  x2 = widget->allocation.x + range->range_rect.x - slider_width / 2;
                  min_pos = min_pos_before;
                  max_pos = widget->allocation.y + find_next_pos (widget, m, marks + i, BTK_POS_LEFT) - min_sep;
                }
              else
                {
                  x1 = widget->allocation.x + range->range_rect.x + range->range_rect.width;
                  x2 = widget->allocation.x + range->range_rect.x + range->range_rect.width + slider_width / 2;
                  min_pos = min_pos_after;
                  max_pos = widget->allocation.y + find_next_pos (widget, m, marks + i, BTK_POS_RIGHT) - min_sep;
                }
              y1 = widget->allocation.y + marks[i];

              btk_paint_hline (widget->style, widget->window, state_type,
                               NULL, widget, "range-mark", x1, x2, y1);

              if (mark->markup)
                {
                  bango_layout_set_markup (layout, mark->markup, -1);
                  bango_layout_get_pixel_extents (layout, NULL, &logical_rect);
              
                  y3 = y1 - logical_rect.height / 2;
                  if (y3 < min_pos)
                    y3 = min_pos;
                  if (y3 + logical_rect.height > max_pos)
                    y3 = max_pos - logical_rect.height;
                  if (y3 < widget->allocation.y)
                    y3 = widget->allocation.y;
                  if (mark->position == BTK_POS_LEFT)
                    {
                      x3 = x2 - value_spacing - logical_rect.width;
                      min_pos_before = y3 + logical_rect.height + min_sep;
                    }
                  else
                    {
                      x3 = x2 + value_spacing;
                      min_pos_after = y3 + logical_rect.height + min_sep;
                    }

                  btk_paint_layout (widget->style, widget->window, state_type,
                                    FALSE, NULL, widget, "scale-mark",
                                    x3, y3, layout);
                }
            }
        } 

      g_object_unref (layout);
      g_free (marks);
    }

  if (scale->draw_value)
    {
      BangoLayout *layout;
      gint x, y;

      layout = btk_scale_get_layout (scale);
      btk_scale_get_layout_offsets (scale, &x, &y);

      btk_paint_layout (widget->style,
                        widget->window,
                        state_type,
			FALSE,
                        NULL,
                        widget,
                        range->orientation == BTK_ORIENTATION_HORIZONTAL ?
                        "hscale" : "vscale",
                        x, y,
                        layout);

    }

  return FALSE;
}

static void
btk_scale_real_get_layout_offsets (BtkScale *scale,
                                   gint     *x,
                                   gint     *y)
{
  BtkWidget *widget = BTK_WIDGET (scale);
  BtkRange *range = BTK_RANGE (widget);
  BangoLayout *layout = btk_scale_get_layout (scale);
  BangoRectangle logical_rect;
  gint value_spacing;

  if (!layout)
    {
      *x = 0;
      *y = 0;

      return;
    }

  btk_widget_style_get (widget, "value-spacing", &value_spacing, NULL);

  bango_layout_get_pixel_extents (layout, NULL, &logical_rect);

  if (range->orientation == BTK_ORIENTATION_HORIZONTAL)
    {
      switch (scale->value_pos)
        {
        case BTK_POS_LEFT:
          *x = range->range_rect.x - value_spacing - logical_rect.width;
          *y = range->range_rect.y + (range->range_rect.height - logical_rect.height) / 2;
          break;

        case BTK_POS_RIGHT:
          *x = range->range_rect.x + range->range_rect.width + value_spacing;
          *y = range->range_rect.y + (range->range_rect.height - logical_rect.height) / 2;
          break;

        case BTK_POS_TOP:
          *x = range->slider_start +
            (range->slider_end - range->slider_start - logical_rect.width) / 2;
          *x = CLAMP (*x, 0, widget->allocation.width - logical_rect.width);
          *y = range->range_rect.y - logical_rect.height - value_spacing;
          break;

        case BTK_POS_BOTTOM:
          *x = range->slider_start +
            (range->slider_end - range->slider_start - logical_rect.width) / 2;
          *x = CLAMP (*x, 0, widget->allocation.width - logical_rect.width);
          *y = range->range_rect.y + range->range_rect.height + value_spacing;
          break;

        default:
          g_return_if_reached ();
          break;
        }
    }
  else
    {
      switch (scale->value_pos)
        {
        case BTK_POS_LEFT:
          *x = range->range_rect.x - logical_rect.width - value_spacing;
          *y = range->slider_start + (range->slider_end - range->slider_start - logical_rect.height) / 2;
          *y = CLAMP (*y, 0, widget->allocation.height - logical_rect.height);
          break;

        case BTK_POS_RIGHT:
          *x = range->range_rect.x + range->range_rect.width + value_spacing;
          *y = range->slider_start + (range->slider_end - range->slider_start - logical_rect.height) / 2;
          *y = CLAMP (*y, 0, widget->allocation.height - logical_rect.height);
          break;

        case BTK_POS_TOP:
          *x = range->range_rect.x + (range->range_rect.width - logical_rect.width) / 2;
          *y = range->range_rect.y - logical_rect.height - value_spacing;
          break;

        case BTK_POS_BOTTOM:
          *x = range->range_rect.x + (range->range_rect.width - logical_rect.width) / 2;
          *y = range->range_rect.y + range->range_rect.height + value_spacing;
          break;

        default:
          g_return_if_reached ();
        }
    }

  *x += widget->allocation.x;
  *y += widget->allocation.y;
}

/**
 * _btk_scale_format_value:
 * @scale: a #BtkScale
 * @value: adjustment value
 * 
 * Emits the #BtkScale::format-value signal.
 * 
 * Return value: formatted value
 */
gchar*
_btk_scale_format_value (BtkScale *scale,
                         gdouble   value)
{
  gchar *fmt = NULL;

  g_signal_emit (scale,
                 signals[FORMAT_VALUE],
                 0,
                 value,
                 &fmt);

  if (fmt)
    return fmt;
  else
    /* insert a LRM, to prevent -20 to come out as 20- in RTL locales */
    return g_strdup_printf ("\342\200\216%0.*f", scale->digits, value);
}

static void
btk_scale_finalize (BObject *object)
{
  BtkScale *scale = BTK_SCALE (object);

  _btk_scale_clear_layout (scale);
  btk_scale_clear_marks (scale);

  B_OBJECT_CLASS (btk_scale_parent_class)->finalize (object);
}

/**
 * btk_scale_get_layout:
 * @scale: A #BtkScale
 *
 * Gets the #BangoLayout used to display the scale. The returned
 * object is owned by the scale so does not need to be freed by
 * the caller.
 *
 * Return value: (transfer none): the #BangoLayout for this scale,
 *     or %NULL if the #BtkScale:draw-value property is %FALSE.
 *
 * Since: 2.4
 */
BangoLayout *
btk_scale_get_layout (BtkScale *scale)
{
  BtkScalePrivate *priv = BTK_SCALE_GET_PRIVATE (scale);
  gchar *txt;

  g_return_val_if_fail (BTK_IS_SCALE (scale), NULL);

  if (!priv->layout)
    {
      if (scale->draw_value)
	priv->layout = btk_widget_create_bango_layout (BTK_WIDGET (scale), NULL);
    }

  if (scale->draw_value) 
    {
      txt = _btk_scale_format_value (scale,
				     BTK_RANGE (scale)->adjustment->value);
      bango_layout_set_text (priv->layout, txt, -1);
      g_free (txt);
    }

  return priv->layout;
}

/**
 * btk_scale_get_layout_offsets:
 * @scale: a #BtkScale
 * @x: (out) (allow-none): location to store X offset of layout, or %NULL
 * @y: (out) (allow-none): location to store Y offset of layout, or %NULL
 *
 * Obtains the coordinates where the scale will draw the 
 * #BangoLayout representing the text in the scale. Remember
 * when using the #BangoLayout function you need to convert to
 * and from pixels using BANGO_PIXELS() or #BANGO_SCALE. 
 *
 * If the #BtkScale:draw-value property is %FALSE, the return 
 * values are undefined.
 *
 * Since: 2.4
 */
void 
btk_scale_get_layout_offsets (BtkScale *scale,
                              gint     *x,
                              gint     *y)
{
  gint local_x = 0; 
  gint local_y = 0;

  g_return_if_fail (BTK_IS_SCALE (scale));

  if (BTK_SCALE_GET_CLASS (scale)->get_layout_offsets)
    (BTK_SCALE_GET_CLASS (scale)->get_layout_offsets) (scale, &local_x, &local_y);

  if (x)
    *x = local_x;
  
  if (y)
    *y = local_y;
}

void
_btk_scale_clear_layout (BtkScale *scale)
{
  BtkScalePrivate *priv = BTK_SCALE_GET_PRIVATE (scale);

  g_return_if_fail (BTK_IS_SCALE (scale));

  if (priv->layout)
    {
      g_object_unref (priv->layout);
      priv->layout = NULL;
    }
}

static void
btk_scale_mark_free (BtkScaleMark *mark)
{
  g_free (mark->markup);
  g_free (mark);
}

/**
 * btk_scale_clear_marks:
 * @scale: a #BtkScale
 * 
 * Removes any marks that have been added with btk_scale_add_mark().
 *
 * Since: 2.16
 */
void
btk_scale_clear_marks (BtkScale *scale)
{
  BtkScalePrivate *priv = BTK_SCALE_GET_PRIVATE (scale);

  g_return_if_fail (BTK_IS_SCALE (scale));

  b_slist_foreach (priv->marks, (GFunc)btk_scale_mark_free, NULL);
  b_slist_free (priv->marks);
  priv->marks = NULL;

  _btk_range_set_stop_values (BTK_RANGE (scale), NULL, 0);

  btk_widget_queue_resize (BTK_WIDGET (scale));
}

/**
 * btk_scale_add_mark:
 * @scale: a #BtkScale
 * @value: the value at which the mark is placed, must be between 
 *   the lower and upper limits of the scales' adjustment
 * @position: where to draw the mark. For a horizontal scale, #BTK_POS_TOP
 *   is drawn above the scale, anything else below. For a vertical scale,
 *   #BTK_POS_LEFT is drawn to the left of the scale, anything else to the
 *   right.
 * @markup: (allow-none): Text to be shown at the mark, using <link linkend="BangoMarkupFormat">Bango markup</link>, or %NULL
 *
 *
 * Adds a mark at @value. 
 *
 * A mark is indicated visually by drawing a tick mark next to the scale, 
 * and BTK+ makes it easy for the user to position the scale exactly at the 
 * marks value.
 *
 * If @markup is not %NULL, text is shown next to the tick mark. 
 *
 * To remove marks from a scale, use btk_scale_clear_marks().
 *
 * Since: 2.16
 */
void
btk_scale_add_mark (BtkScale        *scale,
                    gdouble          value,
                    BtkPositionType  position,
                    const gchar     *markup)
{
  BtkScalePrivate *priv = BTK_SCALE_GET_PRIVATE (scale);
  BtkScaleMark *mark;
  GSList *m;
  gdouble *values;
  gint n, i;

  mark = g_new (BtkScaleMark, 1);
  mark->value = value;
  mark->markup = g_strdup (markup);
  mark->position = position;
 
  priv->marks = b_slist_insert_sorted_with_data (priv->marks, mark,
                                                 (GCompareFunc) compare_marks,
                                                 GINT_TO_POINTER (
                                                   btk_range_get_inverted (BTK_RANGE (scale)) 
                                                   ));

  n = b_slist_length (priv->marks);
  values = g_new (gdouble, n);
  for (m = priv->marks, i = 0; m; m = m->next, i++)
    {
      mark = m->data;
      values[i] = mark->value;
    }
  
  _btk_range_set_stop_values (BTK_RANGE (scale), values, n);

  g_free (values);

  btk_widget_queue_resize (BTK_WIDGET (scale));
}

static BtkBuildableIface *parent_buildable_iface;

static void
btk_scale_buildable_interface_init (BtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->custom_tag_start = btk_scale_buildable_custom_tag_start;
  iface->custom_finished = btk_scale_buildable_custom_finished;
}

typedef struct
{
  BtkScale *scale;
  BtkBuilder *builder;
  GSList *marks;
} MarksSubparserData;

typedef struct
{
  gdouble value;
  BtkPositionType position;
  GString *markup;
  gchar *context;
  gboolean translatable;
} MarkData;

static void
mark_data_free (MarkData *data)
{
  g_string_free (data->markup, TRUE);
  g_free (data->context);
  g_slice_free (MarkData, data);
}

static void
marks_start_element (GMarkupParseContext *context,
                     const gchar         *element_name,
                     const gchar        **names,
                     const gchar        **values,
                     gpointer             user_data,
                     GError             **error)
{
  MarksSubparserData *parser_data = (MarksSubparserData*)user_data;
  guint i;
  gint line_number, char_number;

  if (strcmp (element_name, "marks") == 0)
   ;
  else if (strcmp (element_name, "mark") == 0)
    {
      gdouble value = 0;
      gboolean has_value = FALSE;
      BtkPositionType position = BTK_POS_BOTTOM;
      const gchar *msg_context = NULL;
      gboolean translatable = FALSE;
      MarkData *mark;

      for (i = 0; names[i]; i++)
        {
          if (strcmp (names[i], "translatable") == 0)
            {
              if (!_btk_builder_boolean_from_string (values[i], &translatable, error))
                return;
            }
          else if (strcmp (names[i], "comments") == 0)
            {
              /* do nothing, comments are for translators */
            }
          else if (strcmp (names[i], "context") == 0)
            msg_context = values[i];
          else if (strcmp (names[i], "value") == 0)
            {
              BValue gvalue = { 0, };

              if (!btk_builder_value_from_string_type (parser_data->builder, B_TYPE_DOUBLE, values[i], &gvalue, error))
                return;

              value = b_value_get_double (&gvalue);
              has_value = TRUE;
            }
          else if (strcmp (names[i], "position") == 0)
            {
              BValue gvalue = { 0, };

              if (!btk_builder_value_from_string_type (parser_data->builder, BTK_TYPE_POSITION_TYPE, values[i], &gvalue, error))
                return;

              position = b_value_get_enum (&gvalue);
            }
          else
            {
              g_markup_parse_context_get_position (context,
                                                   &line_number,
                                                   &char_number);
              g_set_error (error,
                           BTK_BUILDER_ERROR,
                           BTK_BUILDER_ERROR_INVALID_ATTRIBUTE,
                           "%s:%d:%d '%s' is not a valid attribute of <%s>",
                           "<input>",
                           line_number, char_number, names[i], "mark");
              return;
            }
        }

      if (!has_value)
        {
          g_markup_parse_context_get_position (context,
                                               &line_number,
                                               &char_number);
          g_set_error (error,
                       BTK_BUILDER_ERROR,
                       BTK_BUILDER_ERROR_MISSING_ATTRIBUTE,
                       "%s:%d:%d <%s> requires attribute \"%s\"",
                       "<input>",
                       line_number, char_number, "mark",
                       "value");
          return;
        }

      mark = g_slice_new (MarkData);
      mark->value = value;
      mark->position = position;
      mark->markup = g_string_new ("");
      mark->context = g_strdup (msg_context);
      mark->translatable = translatable;

      parser_data->marks = b_slist_prepend (parser_data->marks, mark);
    }
  else
    {
      g_markup_parse_context_get_position (context,
                                           &line_number,
                                           &char_number);
      g_set_error (error,
                   BTK_BUILDER_ERROR,
                   BTK_BUILDER_ERROR_MISSING_ATTRIBUTE,
                   "%s:%d:%d unsupported tag for BtkScale: \"%s\"",
                   "<input>",
                   line_number, char_number, element_name);
      return;
    }
}

static void
marks_text (GMarkupParseContext  *context,
            const gchar          *text,
            gsize                 text_len,
            gpointer              user_data,
            GError              **error)
{
  MarksSubparserData *data = (MarksSubparserData*)user_data;

  if (strcmp (g_markup_parse_context_get_element (context), "mark") == 0)
    {
      MarkData *mark = data->marks->data;

      g_string_append_len (mark->markup, text, text_len);
    }
}

static const GMarkupParser marks_parser =
  {
    marks_start_element,
    NULL,
    marks_text,
  };


static gboolean
btk_scale_buildable_custom_tag_start (BtkBuildable  *buildable,
                                      BtkBuilder    *builder,
                                      BObject       *child,
                                      const gchar   *tagname,
                                      GMarkupParser *parser,
                                      gpointer      *data)
{
  MarksSubparserData *parser_data;

  if (child)
    return FALSE;

  if (strcmp (tagname, "marks") == 0)
    {
      parser_data = g_slice_new0 (MarksSubparserData);
      parser_data->scale = BTK_SCALE (buildable);
      parser_data->marks = NULL;

      *parser = marks_parser;
      *data = parser_data;
      return TRUE;
    }

  return parent_buildable_iface->custom_tag_start (buildable, builder, child,
                                                   tagname, parser, data);
}

static void
btk_scale_buildable_custom_finished (BtkBuildable *buildable,
                                     BtkBuilder   *builder,
                                     BObject      *child,
                                     const gchar  *tagname,
                                     gpointer      user_data)
{
  BtkScale *scale = BTK_SCALE (buildable);
  MarksSubparserData *marks_data;

  if (strcmp (tagname, "marks") == 0)
    {
      GSList *m;
      gchar *markup;

      marks_data = (MarksSubparserData *)user_data;

      for (m = marks_data->marks; m; m = m->next)
        {
          MarkData *mdata = m->data;

          if (mdata->translatable && mdata->markup->len)
            markup = _btk_builder_parser_translate (btk_builder_get_translation_domain (builder),
                                                    mdata->context,
                                                    mdata->markup->str);
          else
            markup = mdata->markup->str;

          btk_scale_add_mark (scale, mdata->value, mdata->position, markup);

          mark_data_free (mdata);
        }

      b_slist_free (marks_data->marks);
      g_slice_free (MarksSubparserData, marks_data);
    }
}

#define __BTK_SCALE_C__
#include "btkaliasdef.c"
