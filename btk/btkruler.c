/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
#include <string.h>

#undef BDK_DISABLE_DEPRECATED /* We need bdk_drawable_get_size() */
#undef BTK_DISABLE_DEPRECATED

#include "btkorientable.h"
#include "btkruler.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"


#define RULER_WIDTH           14
#define MINIMUM_INCR          5
#define MAXIMUM_SUBDIVIDE     5
#define MAXIMUM_SCALES        10

#define ROUND(x) ((int) ((x) + 0.5))

enum {
  PROP_0,
  PROP_ORIENTATION,
  PROP_LOWER,
  PROP_UPPER,
  PROP_POSITION,
  PROP_MAX_SIZE,
  PROP_METRIC
};

typedef struct _BtkRulerPrivate BtkRulerPrivate;

struct _BtkRulerPrivate
{
  BtkOrientation orientation;
};

#define BTK_RULER_GET_PRIVATE(obj) (B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_RULER, BtkRulerPrivate))


static void     btk_ruler_set_property    (BObject        *object,
                                           guint            prop_id,
                                           const BValue   *value,
                                           BParamSpec     *pspec);
static void     btk_ruler_get_property    (BObject        *object,
                                           guint           prop_id,
                                           BValue         *value,
                                           BParamSpec     *pspec);
static void     btk_ruler_realize         (BtkWidget      *widget);
static void     btk_ruler_unrealize       (BtkWidget      *widget);
static void     btk_ruler_size_request    (BtkWidget      *widget,
                                           BtkRequisition *requisition);
static void     btk_ruler_size_allocate   (BtkWidget      *widget,
                                           BtkAllocation  *allocation);
static gboolean btk_ruler_motion_notify   (BtkWidget      *widget,
                                           BdkEventMotion *event);
static gboolean btk_ruler_expose          (BtkWidget      *widget,
                                           BdkEventExpose *event);
static void     btk_ruler_make_pixmap     (BtkRuler       *ruler);
static void     btk_ruler_real_draw_ticks (BtkRuler       *ruler);
static void     btk_ruler_real_draw_pos   (BtkRuler       *ruler);


static const BtkRulerMetric ruler_metrics[] =
{
  { "Pixel", "Pi", 1.0, { 1, 2, 5, 10, 25, 50, 100, 250, 500, 1000 }, { 1, 5, 10, 50, 100 }},
  { "Inches", "In", 72.0, { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512 }, { 1, 2, 4, 8, 16 }},
  { "Centimeters", "Cn", 28.35, { 1, 2, 5, 10, 25, 50, 100, 250, 500, 1000 }, { 1, 5, 10, 50, 100 }},
};


G_DEFINE_TYPE_WITH_CODE (BtkRuler, btk_ruler, BTK_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BTK_TYPE_ORIENTABLE,
                                                NULL))


static void
btk_ruler_class_init (BtkRulerClass *class)
{
  BObjectClass   *bobject_class = B_OBJECT_CLASS (class);
  BtkWidgetClass *widget_class  = BTK_WIDGET_CLASS (class);

  bobject_class->set_property = btk_ruler_set_property;
  bobject_class->get_property = btk_ruler_get_property;

  widget_class->realize = btk_ruler_realize;
  widget_class->unrealize = btk_ruler_unrealize;
  widget_class->size_request = btk_ruler_size_request;
  widget_class->size_allocate = btk_ruler_size_allocate;
  widget_class->motion_notify_event = btk_ruler_motion_notify;
  widget_class->expose_event = btk_ruler_expose;

  class->draw_ticks = btk_ruler_real_draw_ticks;
  class->draw_pos = btk_ruler_real_draw_pos;

  g_object_class_override_property (bobject_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  g_object_class_install_property (bobject_class,
                                   PROP_LOWER,
                                   g_param_spec_double ("lower",
							P_("Lower"),
							P_("Lower limit of ruler"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE,
							0.0,
							BTK_PARAM_READWRITE));  

  g_object_class_install_property (bobject_class,
                                   PROP_UPPER,
                                   g_param_spec_double ("upper",
							P_("Upper"),
							P_("Upper limit of ruler"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE,
							0.0,
							BTK_PARAM_READWRITE));  

  g_object_class_install_property (bobject_class,
                                   PROP_POSITION,
                                   g_param_spec_double ("position",
							P_("Position"),
							P_("Position of mark on the ruler"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE,
							0.0,
							BTK_PARAM_READWRITE));  

  g_object_class_install_property (bobject_class,
                                   PROP_MAX_SIZE,
                                   g_param_spec_double ("max-size",
							P_("Max Size"),
							P_("Maximum size of the ruler"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE,
							0.0,
							BTK_PARAM_READWRITE));  
  /**
   * BtkRuler:metric:
   *
   * The metric used for the ruler.
   *
   * Since: 2.8
   */
  g_object_class_install_property (bobject_class,
                                   PROP_METRIC,
                                   g_param_spec_enum ("metric",
						      P_("Metric"),
						      P_("The metric used for the ruler"),
						      BTK_TYPE_METRIC_TYPE, 
						      BTK_PIXELS,
						      BTK_PARAM_READWRITE));  

  g_type_class_add_private (bobject_class, sizeof (BtkRulerPrivate));
}

static void
btk_ruler_init (BtkRuler *ruler)
{
  BtkWidget *widget = BTK_WIDGET (ruler);
  BtkRulerPrivate *private = BTK_RULER_GET_PRIVATE (ruler);

  private->orientation = BTK_ORIENTATION_HORIZONTAL;

  widget->requisition.width  = widget->style->xthickness * 2 + 1;
  widget->requisition.height = widget->style->ythickness * 2 + RULER_WIDTH;

  ruler->backing_store = NULL;
  ruler->xsrc = 0;
  ruler->ysrc = 0;
  ruler->slider_size = 0;
  ruler->lower = 0;
  ruler->upper = 0;
  ruler->position = 0;
  ruler->max_size = 0;

  btk_ruler_set_metric (ruler, BTK_PIXELS);
}

static void
btk_ruler_set_property (BObject      *object,
 			guint         prop_id,
			const BValue *value,
			BParamSpec   *pspec)
{
  BtkRuler *ruler = BTK_RULER (object);
  BtkRulerPrivate *private = BTK_RULER_GET_PRIVATE (ruler);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      private->orientation = b_value_get_enum (value);
      btk_widget_queue_resize (BTK_WIDGET (ruler));
      break;
    case PROP_LOWER:
      btk_ruler_set_range (ruler, b_value_get_double (value), ruler->upper,
			   ruler->position, ruler->max_size);
      break;
    case PROP_UPPER:
      btk_ruler_set_range (ruler, ruler->lower, b_value_get_double (value),
			   ruler->position, ruler->max_size);
      break;
    case PROP_POSITION:
      btk_ruler_set_range (ruler, ruler->lower, ruler->upper,
			   b_value_get_double (value), ruler->max_size);
      break;
    case PROP_MAX_SIZE:
      btk_ruler_set_range (ruler, ruler->lower, ruler->upper,
			   ruler->position,  b_value_get_double (value));
      break;
    case PROP_METRIC:
      btk_ruler_set_metric (ruler, b_value_get_enum (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_ruler_get_property (BObject      *object,
			guint         prop_id,
			BValue       *value,
			BParamSpec   *pspec)
{
  BtkRuler *ruler = BTK_RULER (object);
  BtkRulerPrivate *private = BTK_RULER_GET_PRIVATE (ruler);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      b_value_set_enum (value, private->orientation);
      break;
    case PROP_LOWER:
      b_value_set_double (value, ruler->lower);
      break;
    case PROP_UPPER:
      b_value_set_double (value, ruler->upper);
      break;
    case PROP_POSITION:
      b_value_set_double (value, ruler->position);
      break;
    case PROP_MAX_SIZE:
      b_value_set_double (value, ruler->max_size);
      break;
    case PROP_METRIC:
      b_value_set_enum (value, btk_ruler_get_metric (ruler));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

void
btk_ruler_set_metric (BtkRuler      *ruler,
		      BtkMetricType  metric)
{
  g_return_if_fail (BTK_IS_RULER (ruler));

  ruler->metric = (BtkRulerMetric *) &ruler_metrics[metric];

  if (btk_widget_is_drawable (BTK_WIDGET (ruler)))
    btk_widget_queue_draw (BTK_WIDGET (ruler));

  g_object_notify (B_OBJECT (ruler), "metric");
}

/**
 * btk_ruler_get_metric:
 * @ruler: a #BtkRuler
 *
 * Gets the units used for a #BtkRuler. See btk_ruler_set_metric().
 *
 * Return value: the units currently used for @ruler
 *
 * @Deprecated: 2.24: #BtkRuler has been removed from BTK 3 for being
 *              unmaintained and too specialized. There is no replacement.
 **/
BtkMetricType
btk_ruler_get_metric (BtkRuler *ruler)
{
  gint i;

  g_return_val_if_fail (BTK_IS_RULER (ruler), 0);

  for (i = 0; i < G_N_ELEMENTS (ruler_metrics); i++)
    if (ruler->metric == &ruler_metrics[i])
      return i;

  g_assert_not_reached ();

  return 0;
}

/**
 * btk_ruler_set_range:
 * @ruler: the btkruler
 * @lower: the lower limit of the ruler
 * @upper: the upper limit of the ruler
 * @position: the mark on the ruler
 * @max_size: the maximum size of the ruler used when calculating the space to
 * leave for the text
 *
 * This sets the range of the ruler. 
 *
 * @Deprecated: 2.24: #BtkRuler has been removed from BTK 3 for being
 *              unmaintained and too specialized. There is no replacement.
 */
void
btk_ruler_set_range (BtkRuler *ruler,
		     gdouble   lower,
		     gdouble   upper,
		     gdouble   position,
		     gdouble   max_size)
{
  g_return_if_fail (BTK_IS_RULER (ruler));

  g_object_freeze_notify (B_OBJECT (ruler));
  if (ruler->lower != lower)
    {
      ruler->lower = lower;
      g_object_notify (B_OBJECT (ruler), "lower");
    }
  if (ruler->upper != upper)
    {
      ruler->upper = upper;
      g_object_notify (B_OBJECT (ruler), "upper");
    }
  if (ruler->position != position)
    {
      ruler->position = position;
      g_object_notify (B_OBJECT (ruler), "position");
    }
  if (ruler->max_size != max_size)
    {
      ruler->max_size = max_size;
      g_object_notify (B_OBJECT (ruler), "max-size");
    }
  g_object_thaw_notify (B_OBJECT (ruler));

  if (btk_widget_is_drawable (BTK_WIDGET (ruler)))
    btk_widget_queue_draw (BTK_WIDGET (ruler));
}

/**
 * btk_ruler_get_range:
 * @ruler: a #BtkRuler
 * @lower: (allow-none): location to store lower limit of the ruler, or %NULL
 * @upper: (allow-none): location to store upper limit of the ruler, or %NULL
 * @position: (allow-none): location to store the current position of the mark on the ruler, or %NULL
 * @max_size: location to store the maximum size of the ruler used when calculating
 *            the space to leave for the text, or %NULL.
 *
 * Retrieves values indicating the range and current position of a #BtkRuler.
 * See btk_ruler_set_range().
 *
 * @Deprecated: 2.24: #BtkRuler has been removed from BTK 3 for being
 *              unmaintained and too specialized. There is no replacement.
 **/
void
btk_ruler_get_range (BtkRuler *ruler,
		     gdouble  *lower,
		     gdouble  *upper,
		     gdouble  *position,
		     gdouble  *max_size)
{
  g_return_if_fail (BTK_IS_RULER (ruler));

  if (lower)
    *lower = ruler->lower;
  if (upper)
    *upper = ruler->upper;
  if (position)
    *position = ruler->position;
  if (max_size)
    *max_size = ruler->max_size;
}

void
btk_ruler_draw_ticks (BtkRuler *ruler)
{
  g_return_if_fail (BTK_IS_RULER (ruler));

  if (BTK_RULER_GET_CLASS (ruler)->draw_ticks)
    BTK_RULER_GET_CLASS (ruler)->draw_ticks (ruler);
}

void
btk_ruler_draw_pos (BtkRuler *ruler)
{
  g_return_if_fail (BTK_IS_RULER (ruler));

  if (BTK_RULER_GET_CLASS (ruler)->draw_pos)
     BTK_RULER_GET_CLASS (ruler)->draw_pos (ruler);
}


static void
btk_ruler_realize (BtkWidget *widget)
{
  BtkRuler *ruler;
  BdkWindowAttr attributes;
  gint attributes_mask;

  ruler = BTK_RULER (widget);

  btk_widget_set_realized (widget, TRUE);

  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  attributes.event_mask = btk_widget_get_events (widget);
  attributes.event_mask |= (BDK_EXPOSURE_MASK |
			    BDK_POINTER_MOTION_MASK |
			    BDK_POINTER_MOTION_HINT_MASK);

  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;

  widget->window = bdk_window_new (btk_widget_get_parent_window (widget), &attributes, attributes_mask);
  bdk_window_set_user_data (widget->window, ruler);

  widget->style = btk_style_attach (widget->style, widget->window);
  btk_style_set_background (widget->style, widget->window, BTK_STATE_ACTIVE);

  btk_ruler_make_pixmap (ruler);
}

static void
btk_ruler_unrealize (BtkWidget *widget)
{
  BtkRuler *ruler = BTK_RULER (widget);

  if (ruler->backing_store)
    {
      g_object_unref (ruler->backing_store);
      ruler->backing_store = NULL;
    }

  BTK_WIDGET_CLASS (btk_ruler_parent_class)->unrealize (widget);
}

static void
btk_ruler_size_request (BtkWidget      *widget,
                        BtkRequisition *requisition)
{
  BtkRulerPrivate *private = BTK_RULER_GET_PRIVATE (widget);

  if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
    {
      requisition->width  = widget->style->xthickness * 2 + 1;
      requisition->height = widget->style->ythickness * 2 + RULER_WIDTH;
    }
  else
    {
      requisition->width  = widget->style->xthickness * 2 + RULER_WIDTH;
      requisition->height = widget->style->ythickness * 2 + 1;
    }
}

static void
btk_ruler_size_allocate (BtkWidget     *widget,
			 BtkAllocation *allocation)
{
  BtkRuler *ruler = BTK_RULER (widget);

  widget->allocation = *allocation;

  if (btk_widget_get_realized (widget))
    {
      bdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);

      btk_ruler_make_pixmap (ruler);
    }
}

static gboolean
btk_ruler_motion_notify (BtkWidget      *widget,
                         BdkEventMotion *event)
{
  BtkRuler *ruler = BTK_RULER (widget);
  BtkRulerPrivate *private = BTK_RULER_GET_PRIVATE (widget);
  gint x;
  gint y;

  bdk_event_request_motions (event);
  x = event->x;
  y = event->y;

  if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
    ruler->position = ruler->lower + ((ruler->upper - ruler->lower) * x) / widget->allocation.width;
  else
    ruler->position = ruler->lower + ((ruler->upper - ruler->lower) * y) / widget->allocation.height;

  g_object_notify (B_OBJECT (ruler), "position");

  /*  Make sure the ruler has been allocated already  */
  if (ruler->backing_store != NULL)
    btk_ruler_draw_pos (ruler);

  return FALSE;
}

static gboolean
btk_ruler_expose (BtkWidget      *widget,
		  BdkEventExpose *event)
{
  if (btk_widget_is_drawable (widget))
    {
      BtkRuler *ruler = BTK_RULER (widget);
      bairo_t *cr;

      btk_ruler_draw_ticks (ruler);
      
      cr = bdk_bairo_create (widget->window);
      bdk_bairo_set_source_pixmap (cr, ruler->backing_store, 0, 0);
      bdk_bairo_rectangle (cr, &event->area);
      bairo_fill (cr);
      bairo_destroy (cr);
      
      btk_ruler_draw_pos (ruler);
    }

  return FALSE;
}

static void
btk_ruler_make_pixmap (BtkRuler *ruler)
{
  BtkWidget *widget;
  gint width;
  gint height;

  widget = BTK_WIDGET (ruler);

  if (ruler->backing_store)
    {
      bdk_drawable_get_size (ruler->backing_store, &width, &height);
      if ((width == widget->allocation.width) &&
	  (height == widget->allocation.height))
	return;

      g_object_unref (ruler->backing_store);
    }

  ruler->backing_store = bdk_pixmap_new (widget->window,
					 widget->allocation.width,
					 widget->allocation.height,
					 -1);

  ruler->xsrc = 0;
  ruler->ysrc = 0;
}

static void
btk_ruler_real_draw_ticks (BtkRuler *ruler)
{
  BtkWidget *widget = BTK_WIDGET (ruler);
  BtkRulerPrivate *private = BTK_RULER_GET_PRIVATE (ruler);
  bairo_t *cr;
  gint i, j;
  gint width, height;
  gint xthickness;
  gint ythickness;
  gint length, ideal_length;
  gdouble lower, upper;		/* Upper and lower limits, in ruler units */
  gdouble increment;		/* Number of pixels per unit */
  gint scale;			/* Number of units per major unit */
  gdouble subd_incr;
  gdouble start, end, cur;
  gchar unit_str[32];
  gint digit_height;
  gint digit_offset;
  gint text_width;
  gint text_height;
  gint pos;
  BangoLayout *layout;
  BangoRectangle logical_rect, ink_rect;

  if (!btk_widget_is_drawable (widget))
    return;

  xthickness = widget->style->xthickness;
  ythickness = widget->style->ythickness;

  layout = btk_widget_create_bango_layout (widget, "012456789");
  bango_layout_get_extents (layout, &ink_rect, &logical_rect);

  digit_height = BANGO_PIXELS (ink_rect.height) + 2;
  digit_offset = ink_rect.y;

  if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
    {
      width = widget->allocation.width;
      height = widget->allocation.height - ythickness * 2;
    }
  else
    {
      width = widget->allocation.height;
      height = widget->allocation.width - ythickness * 2;
    }

#define DETAILE(private) (private->orientation == BTK_ORIENTATION_HORIZONTAL ? "hruler" : "vruler");

  btk_paint_box (widget->style, ruler->backing_store,
		 BTK_STATE_NORMAL, BTK_SHADOW_OUT,
		 NULL, widget,
                 private->orientation == BTK_ORIENTATION_HORIZONTAL ?
                 "hruler" : "vruler",
		 0, 0,
		 widget->allocation.width, widget->allocation.height);

  cr = bdk_bairo_create (ruler->backing_store);
  bdk_bairo_set_source_color (cr, &widget->style->fg[widget->state]);

  if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
    {
      bairo_rectangle (cr,
                       xthickness,
                       height + ythickness,
                       widget->allocation.width - 2 * xthickness,
                       1);
    }
  else
    {
      bairo_rectangle (cr,
                       height + xthickness,
                       ythickness,
                       1,
                       widget->allocation.height - 2 * ythickness);
    }

  upper = ruler->upper / ruler->metric->pixels_per_unit;
  lower = ruler->lower / ruler->metric->pixels_per_unit;

  if ((upper - lower) == 0)
    goto out;

  increment = (gdouble) width / (upper - lower);

  /* determine the scale H
   *  We calculate the text size as for the vruler instead of using
   *  text_width = bdk_string_width(font, unit_str), so that the result
   *  for the scale looks consistent with an accompanying vruler
   */
  /* determine the scale V
   *   use the maximum extents of the ruler to determine the largest
   *   possible number to be displayed.  Calculate the height in pixels
   *   of this displayed text. Use this height to find a scale which
   *   leaves sufficient room for drawing the ruler.
   */
  scale = ceil (ruler->max_size / ruler->metric->pixels_per_unit);
  g_snprintf (unit_str, sizeof (unit_str), "%d", scale);

  if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
    {
      text_width = strlen (unit_str) * digit_height + 1;

      for (scale = 0; scale < MAXIMUM_SCALES; scale++)
        if (ruler->metric->ruler_scale[scale] * fabs(increment) > 2 * text_width)
          break;
    }
  else
    {
      text_height = strlen (unit_str) * digit_height + 1;

      for (scale = 0; scale < MAXIMUM_SCALES; scale++)
        if (ruler->metric->ruler_scale[scale] * fabs(increment) > 2 * text_height)
          break;
    }

  if (scale == MAXIMUM_SCALES)
    scale = MAXIMUM_SCALES - 1;

  /* drawing starts here */
  length = 0;
  for (i = MAXIMUM_SUBDIVIDE - 1; i >= 0; i--)
    {
      subd_incr = (gdouble) ruler->metric->ruler_scale[scale] /
	          (gdouble) ruler->metric->subdivide[i];
      if (subd_incr * fabs(increment) <= MINIMUM_INCR)
	continue;

      /* Calculate the length of the tickmarks. Make sure that
       * this length increases for each set of ticks
       */
      ideal_length = height / (i + 1) - 1;
      if (ideal_length > ++length)
	length = ideal_length;

      if (lower < upper)
	{
	  start = floor (lower / subd_incr) * subd_incr;
	  end   = ceil  (upper / subd_incr) * subd_incr;
	}
      else
	{
	  start = floor (upper / subd_incr) * subd_incr;
	  end   = ceil  (lower / subd_incr) * subd_incr;
	}

      for (cur = start; cur <= end; cur += subd_incr)
	{
	  pos = ROUND ((cur - lower) * increment);

          if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
            {
              bairo_rectangle (cr,
                               pos, height + ythickness - length,
                               1,   length);
            }
          else
            {
              bairo_rectangle (cr,
                               height + xthickness - length, pos,
                               length,                       1);
            }

	  /* draw label */
	  if (i == 0)
	    {
	      g_snprintf (unit_str, sizeof (unit_str), "%d", (int) cur);

              if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
                {
                  bango_layout_set_text (layout, unit_str, -1);
                  bango_layout_get_extents (layout, &logical_rect, NULL);

                  btk_paint_layout (widget->style,
                                    ruler->backing_store,
                                    btk_widget_get_state (widget),
                                    FALSE,
                                    NULL,
                                    widget,
                                    "hruler",
                                    pos + 2, ythickness + BANGO_PIXELS (logical_rect.y - digit_offset),
                                    layout);
                }
              else
                {
                  for (j = 0; j < (int) strlen (unit_str); j++)
                    {
                      bango_layout_set_text (layout, unit_str + j, 1);
                      bango_layout_get_extents (layout, NULL, &logical_rect);

                      btk_paint_layout (widget->style,
                                        ruler->backing_store,
                                        btk_widget_get_state (widget),
                                        FALSE,
                                        NULL,
                                        widget,
                                        "vruler",
                                        xthickness + 1,
                                        pos + digit_height * j + 2 + BANGO_PIXELS (logical_rect.y - digit_offset),
                                        layout);
                    }
                }
	    }
	}
    }

  bairo_fill (cr);
out:
  bairo_destroy (cr);

  g_object_unref (layout);
}

static void
btk_ruler_real_draw_pos (BtkRuler *ruler)
{
  BtkWidget *widget = BTK_WIDGET (ruler);
  BtkRulerPrivate *private = BTK_RULER_GET_PRIVATE (ruler);
  gint x, y;
  gint width, height;
  gint bs_width, bs_height;
  gint xthickness;
  gint ythickness;
  gdouble increment;

  if (btk_widget_is_drawable (widget))
    {
      xthickness = widget->style->xthickness;
      ythickness = widget->style->ythickness;
      width = widget->allocation.width;
      height = widget->allocation.height;

      if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
        {
          height -= ythickness * 2;

          bs_width = height / 2 + 2;
          bs_width |= 1;  /* make sure it's odd */
          bs_height = bs_width / 2 + 1;
        }
      else
        {
          width -= xthickness * 2;

          bs_height = width / 2 + 2;
          bs_height |= 1;  /* make sure it's odd */
          bs_width = bs_height / 2 + 1;
        }

      if ((bs_width > 0) && (bs_height > 0))
	{
	  bairo_t *cr = bdk_bairo_create (widget->window);

	  /*  If a backing store exists, restore the ruler  */
	  if (ruler->backing_store)
            {
              bairo_t *cr = bdk_bairo_create (widget->window);

              bdk_bairo_set_source_pixmap (cr, ruler->backing_store, 0, 0);
              bairo_rectangle (cr, ruler->xsrc, ruler->ysrc, bs_width, bs_height);
              bairo_fill (cr);

              bairo_destroy (cr);
            }

          if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
            {
              increment = (gdouble) width / (ruler->upper - ruler->lower);

              x = ROUND ((ruler->position - ruler->lower) * increment) + (xthickness - bs_width) / 2 - 1;
              y = (height + bs_height) / 2 + ythickness;
            }
          else
            {
              increment = (gdouble) height / (ruler->upper - ruler->lower);

              x = (width + bs_width) / 2 + xthickness;
              y = ROUND ((ruler->position - ruler->lower) * increment) + (ythickness - bs_height) / 2 - 1;
            }

	  bdk_bairo_set_source_color (cr, &widget->style->fg[widget->state]);

	  bairo_move_to (cr, x, y);

          if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
            {
              bairo_line_to (cr, x + bs_width / 2.0, y + bs_height);
              bairo_line_to (cr, x + bs_width,       y);
            }
          else
            {
              bairo_line_to (cr, x + bs_width, y + bs_height / 2.0);
              bairo_line_to (cr, x,            y + bs_height);
            }

	  bairo_fill (cr);

	  bairo_destroy (cr);

	  ruler->xsrc = x;
	  ruler->ysrc = y;
	}
    }
}

#define __BTK_RULER_C__
#include "btkaliasdef.c"
