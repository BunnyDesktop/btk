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

#undef BDK_DISABLE_DEPRECATED
#undef BTK_DISABLE_DEPRECATED
#define __BTK_PROGRESS_C__

#include "btkprogress.h" 
#include "btkprivate.h" 
#include "btkintl.h"
#include "btkalias.h"

#define EPSILON  1e-5
#define DEFAULT_FORMAT "%P %%"

enum {
  PROP_0,
  PROP_ACTIVITY_MODE,
  PROP_SHOW_TEXT,
  PROP_TEXT_XALIGN,
  PROP_TEXT_YALIGN
};


static void btk_progress_set_property    (BObject          *object,
					  guint             prop_id,
					  const BValue     *value,
					  BParamSpec       *pspec);
static void btk_progress_get_property    (BObject          *object,
					  guint             prop_id,
					  BValue           *value,
					  BParamSpec       *pspec);
static void btk_progress_destroy         (BtkObject        *object);
static void btk_progress_finalize        (BObject          *object);
static void btk_progress_realize         (BtkWidget        *widget);
static gboolean btk_progress_expose      (BtkWidget        *widget,
				 	  BdkEventExpose   *event);
static void btk_progress_size_allocate   (BtkWidget        *widget,
				 	  BtkAllocation    *allocation);
static void btk_progress_create_pixmap   (BtkProgress      *progress);
static void btk_progress_value_changed   (BtkAdjustment    *adjustment,
					  BtkProgress      *progress);
static void btk_progress_changed         (BtkAdjustment    *adjustment,
					  BtkProgress      *progress);

G_DEFINE_ABSTRACT_TYPE (BtkProgress, btk_progress, BTK_TYPE_WIDGET)

static void
btk_progress_class_init (BtkProgressClass *class)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (class);
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;

  object_class = (BtkObjectClass *) class;
  widget_class = (BtkWidgetClass *) class;

  bobject_class->finalize = btk_progress_finalize;

  bobject_class->set_property = btk_progress_set_property;
  bobject_class->get_property = btk_progress_get_property;
  object_class->destroy = btk_progress_destroy;

  widget_class->realize = btk_progress_realize;
  widget_class->expose_event = btk_progress_expose;
  widget_class->size_allocate = btk_progress_size_allocate;

  /* to be overridden */
  class->paint = NULL;
  class->update = NULL;
  class->act_mode_enter = NULL;
  
  g_object_class_install_property (bobject_class,
                                   PROP_ACTIVITY_MODE,
                                   g_param_spec_boolean ("activity-mode",
							 P_("Activity mode"),
							 P_("If TRUE, the BtkProgress is in activity mode, meaning that it signals "
                                                            "something is happening, but not how much of the activity is finished. "
                                                            "This is used when you're doing something but don't know how long it will take."),
							 FALSE,
							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_SHOW_TEXT,
                                   g_param_spec_boolean ("show-text",
							 P_("Show text"),
							 P_("Whether the progress is shown as text."),
							 FALSE,
							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_TEXT_XALIGN,
				   g_param_spec_float ("text-xalign",
						       P_("Text x alignment"),
                                                       P_("The horizontal text alignment, from 0 (left) to 1 (right). Reversed for RTL layouts."),
						       0.0, 1.0, 0.5,
						       BTK_PARAM_READWRITE));  
  g_object_class_install_property (bobject_class,
				   PROP_TEXT_YALIGN,
				   g_param_spec_float ("text-yalign",
						       P_("Text y alignment"),
                                                       P_("The vertical text alignment, from 0 (top) to 1 (bottom)."),
						       0.0, 1.0, 0.5,
						       BTK_PARAM_READWRITE));
}

static void
btk_progress_set_property (BObject      *object,
			   guint         prop_id,
			   const BValue *value,
			   BParamSpec   *pspec)
{
  BtkProgress *progress;
  
  progress = BTK_PROGRESS (object);

  switch (prop_id)
    {
    case PROP_ACTIVITY_MODE:
      btk_progress_set_activity_mode (progress, b_value_get_boolean (value));
      break;
    case PROP_SHOW_TEXT:
      btk_progress_set_show_text (progress, b_value_get_boolean (value));
      break;
    case PROP_TEXT_XALIGN:
      btk_progress_set_text_alignment (progress,
				       b_value_get_float (value),
				       progress->y_align);
      break;
    case PROP_TEXT_YALIGN:
      btk_progress_set_text_alignment (progress,
				       progress->x_align,
				       b_value_get_float (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_progress_get_property (BObject      *object,
			   guint         prop_id,
			   BValue       *value,
			   BParamSpec   *pspec)
{
  BtkProgress *progress;
  
  progress = BTK_PROGRESS (object);

  switch (prop_id)
    {
    case PROP_ACTIVITY_MODE:
      b_value_set_boolean (value, (progress->activity_mode != FALSE));
      break;
    case PROP_SHOW_TEXT:
      b_value_set_boolean (value, (progress->show_text != FALSE));
      break;
    case PROP_TEXT_XALIGN:
      b_value_set_float (value, progress->x_align);
      break;
    case PROP_TEXT_YALIGN:
      b_value_set_float (value, progress->y_align);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_progress_init (BtkProgress *progress)
{
  progress->adjustment = NULL;
  progress->offscreen_pixmap = NULL;
  progress->format = g_strdup (DEFAULT_FORMAT);
  progress->x_align = 0.5;
  progress->y_align = 0.5;
  progress->show_text = FALSE;
  progress->activity_mode = FALSE;
  progress->use_text_format = TRUE;
}

static void
btk_progress_realize (BtkWidget *widget)
{
  BtkProgress *progress = BTK_PROGRESS (widget);
  BdkWindowAttr attributes;
  gint attributes_mask;

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
  attributes.event_mask |= BDK_EXPOSURE_MASK;

  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;

  widget->window = bdk_window_new (btk_widget_get_parent_window (widget),
				   &attributes, attributes_mask);
  bdk_window_set_user_data (widget->window, progress);

  widget->style = btk_style_attach (widget->style, widget->window);
  btk_style_set_background (widget->style, widget->window, BTK_STATE_ACTIVE);

  btk_progress_create_pixmap (progress);
}

static void
btk_progress_destroy (BtkObject *object)
{
  BtkProgress *progress = BTK_PROGRESS (object);

  if (progress->adjustment)
    {
      g_signal_handlers_disconnect_by_func (progress->adjustment,
					    btk_progress_value_changed,
					    progress);
      g_signal_handlers_disconnect_by_func (progress->adjustment,
					    btk_progress_changed,
					    progress);
      g_object_unref (progress->adjustment);
      progress->adjustment = NULL;
    }

  BTK_OBJECT_CLASS (btk_progress_parent_class)->destroy (object);
}

static void
btk_progress_finalize (BObject *object)
{
  BtkProgress *progress = BTK_PROGRESS (object);

  if (progress->offscreen_pixmap)
    g_object_unref (progress->offscreen_pixmap);

  g_free (progress->format);

  B_OBJECT_CLASS (btk_progress_parent_class)->finalize (object);
}

static gboolean
btk_progress_expose (BtkWidget      *widget,
		     BdkEventExpose *event)
{
  if (BTK_WIDGET_DRAWABLE (widget))
    bdk_draw_drawable (widget->window,
		       widget->style->black_gc,
		       BTK_PROGRESS (widget)->offscreen_pixmap,
		       event->area.x, event->area.y,
		       event->area.x, event->area.y,
		       event->area.width,
		       event->area.height);

  return FALSE;
}

static void
btk_progress_size_allocate (BtkWidget     *widget,
			    BtkAllocation *allocation)
{
  widget->allocation = *allocation;

  if (btk_widget_get_realized (widget))
    {
      bdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);

      btk_progress_create_pixmap (BTK_PROGRESS (widget));
    }
}

static void
btk_progress_create_pixmap (BtkProgress *progress)
{
  BtkWidget *widget;

  g_return_if_fail (BTK_IS_PROGRESS (progress));

  widget = BTK_WIDGET (progress);

  if (btk_widget_get_realized (widget))
    {
      if (progress->offscreen_pixmap)
	g_object_unref (progress->offscreen_pixmap);

      progress->offscreen_pixmap = bdk_pixmap_new (widget->window,
						   widget->allocation.width,
						   widget->allocation.height,
						   -1);

      /* clear the pixmap for transparent themes */
      btk_paint_flat_box (widget->style,
                          progress->offscreen_pixmap,
                          BTK_STATE_NORMAL, BTK_SHADOW_NONE,
                          NULL, widget, "trough", 0, 0, -1, -1);
      
      BTK_PROGRESS_GET_CLASS (progress)->paint (progress);
    }
}

static void
btk_progress_changed (BtkAdjustment *adjustment,
		      BtkProgress   *progress)
{
  /* A change in the value of adjustment->upper can change
   * the size request
   */
  if (progress->use_text_format && progress->show_text)
    btk_widget_queue_resize (BTK_WIDGET (progress));
  else
    BTK_PROGRESS_GET_CLASS (progress)->update (progress);
}

static void
btk_progress_value_changed (BtkAdjustment *adjustment,
			    BtkProgress   *progress)
{
  BTK_PROGRESS_GET_CLASS (progress)->update (progress);
}

static gchar *
btk_progress_build_string (BtkProgress *progress,
			   gdouble      value,
			   gdouble      percentage)
{
  gchar buf[256] = { 0 };
  gchar tmp[256] = { 0 };
  gchar *src;
  gchar *dest;
  gchar fmt[10];

  src = progress->format;

  /* This is the new supported version of this function */
  if (!progress->use_text_format)
    return g_strdup (src);

  /* And here's all the deprecated goo. */
  
  dest = buf;
 
  while (src && *src)
    {
      if (*src != '%')
	{
	  *dest = *src;
	  dest++;
	}
      else
	{
	  gchar c;
	  gint digits;

	  c = *(src + sizeof(gchar));
	  digits = 0;

	  if (c >= '0' && c <= '2')
	    {
	      digits = (gint) (c - '0');
	      src++;
	      c = *(src + sizeof(gchar));
	    }

	  switch (c)
	    {
	    case '%':
	      *dest = '%';
	      src++;
	      dest++;
	      break;
	    case 'p':
	    case 'P':
	      if (digits)
		{
		  g_snprintf (fmt, sizeof (fmt), "%%.%df", digits);
		  g_snprintf (tmp, sizeof (tmp), fmt, 100 * percentage);
		}
	      else
		g_snprintf (tmp, sizeof (tmp), "%.0f", 100 * percentage);
	      strcat (buf, tmp);
	      dest = &(buf[strlen (buf)]);
	      src++;
	      break;
	    case 'v':
	    case 'V':
	      if (digits)
		{
		  g_snprintf (fmt, sizeof (fmt), "%%.%df", digits);
		  g_snprintf (tmp, sizeof (tmp), fmt, value);
		}
	      else
		g_snprintf (tmp, sizeof (tmp), "%.0f", value);
	      strcat (buf, tmp);
	      dest = &(buf[strlen (buf)]);
	      src++;
	      break;
	    case 'l':
	    case 'L':
	      if (digits)
		{
		  g_snprintf (fmt, sizeof (fmt), "%%.%df", digits);
		  g_snprintf (tmp, sizeof (tmp), fmt, progress->adjustment->lower);
		}
	      else
		g_snprintf (tmp, sizeof (tmp), "%.0f", progress->adjustment->lower);
	      strcat (buf, tmp);
	      dest = &(buf[strlen (buf)]);
	      src++;
	      break;
	    case 'u':
	    case 'U':
	      if (digits)
		{
		  g_snprintf (fmt, sizeof (fmt), "%%.%df", digits);
		  g_snprintf (tmp, sizeof (tmp), fmt, progress->adjustment->upper);
		}
	      else
		g_snprintf (tmp, sizeof (tmp), "%.0f", progress->adjustment->upper);
	      strcat (buf, tmp);
	      dest = &(buf[strlen (buf)]);
	      src++;
	      break;
	    default:
	      break;
	    }
	}
      src++;
    }

  return g_strdup (buf);
}

/***************************************************************/

void
btk_progress_set_adjustment (BtkProgress   *progress,
			     BtkAdjustment *adjustment)
{
  g_return_if_fail (BTK_IS_PROGRESS (progress));
  if (adjustment)
    g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));
  else
    adjustment = (BtkAdjustment*) btk_adjustment_new (0, 0, 100, 0, 0, 0);

  if (progress->adjustment != adjustment)
    {
      if (progress->adjustment)
        {
	  g_signal_handlers_disconnect_by_func (progress->adjustment,
						btk_progress_changed,
						progress);
	  g_signal_handlers_disconnect_by_func (progress->adjustment,
						btk_progress_value_changed,
						progress);
          g_object_unref (progress->adjustment);
        }
      progress->adjustment = adjustment;
      if (adjustment)
        {
          g_object_ref_sink (adjustment);
          g_signal_connect (adjustment, "changed",
			    G_CALLBACK (btk_progress_changed),
			    progress);
          g_signal_connect (adjustment, "value-changed",
			    G_CALLBACK (btk_progress_value_changed),
			    progress);
        }

      btk_progress_changed (adjustment, progress);
    }
}

void
btk_progress_configure (BtkProgress *progress,
			gdouble      value,
			gdouble      min,
			gdouble      max)
{
  BtkAdjustment *adj;
  gboolean changed = FALSE;

  g_return_if_fail (BTK_IS_PROGRESS (progress));
  g_return_if_fail (min <= max);
  g_return_if_fail (value >= min && value <= max);

  if (!progress->adjustment)
    btk_progress_set_adjustment (progress, NULL);
  adj = progress->adjustment;

  if (fabs (adj->lower - min) > EPSILON || fabs (adj->upper - max) > EPSILON)
    changed = TRUE;

  adj->value = value;
  adj->lower = min;
  adj->upper = max;

  btk_adjustment_value_changed (adj);
  if (changed)
    btk_adjustment_changed (adj);
}

void
btk_progress_set_percentage (BtkProgress *progress,
			     gdouble      percentage)
{
  g_return_if_fail (BTK_IS_PROGRESS (progress));
  g_return_if_fail (percentage >= 0 && percentage <= 1.0);

  if (!progress->adjustment)
    btk_progress_set_adjustment (progress, NULL);
  btk_progress_set_value (progress, progress->adjustment->lower + percentage * 
		 (progress->adjustment->upper - progress->adjustment->lower));
}

gdouble
btk_progress_get_current_percentage (BtkProgress *progress)
{
  g_return_val_if_fail (BTK_IS_PROGRESS (progress), 0);

  if (!progress->adjustment)
    btk_progress_set_adjustment (progress, NULL);

  return btk_progress_get_percentage_from_value (progress, progress->adjustment->value);
}

gdouble
btk_progress_get_percentage_from_value (BtkProgress *progress,
					gdouble      value)
{
  g_return_val_if_fail (BTK_IS_PROGRESS (progress), 0);

  if (!progress->adjustment)
    btk_progress_set_adjustment (progress, NULL);

  if (progress->adjustment->lower < progress->adjustment->upper &&
      value >= progress->adjustment->lower &&
      value <= progress->adjustment->upper)
    return (value - progress->adjustment->lower) /
      (progress->adjustment->upper - progress->adjustment->lower);
  else
    return 0.0;
}

void
btk_progress_set_value (BtkProgress *progress,
			gdouble      value)
{
  g_return_if_fail (BTK_IS_PROGRESS (progress));

  if (!progress->adjustment)
    btk_progress_set_adjustment (progress, NULL);

  if (fabs (progress->adjustment->value - value) > EPSILON)
    btk_adjustment_set_value (progress->adjustment, value);
}

gdouble
btk_progress_get_value (BtkProgress *progress)
{
  g_return_val_if_fail (BTK_IS_PROGRESS (progress), 0);

  return progress->adjustment ? progress->adjustment->value : 0;
}

void
btk_progress_set_show_text (BtkProgress *progress,
			    gboolean     show_text)
{
  g_return_if_fail (BTK_IS_PROGRESS (progress));

  if (progress->show_text != show_text)
    {
      progress->show_text = show_text;

      btk_widget_queue_resize (BTK_WIDGET (progress));

      g_object_notify (B_OBJECT (progress), "show-text");
    }
}

void
btk_progress_set_text_alignment (BtkProgress *progress,
				 gfloat       x_align,
				 gfloat       y_align)
{
  g_return_if_fail (BTK_IS_PROGRESS (progress));
  g_return_if_fail (x_align >= 0.0 && x_align <= 1.0);
  g_return_if_fail (y_align >= 0.0 && y_align <= 1.0);

  if (progress->x_align != x_align || progress->y_align != y_align)
    {
      g_object_freeze_notify (B_OBJECT (progress));
      if (progress->x_align != x_align)
	{
	  progress->x_align = x_align;
	  g_object_notify (B_OBJECT (progress), "text-xalign");
	}

      if (progress->y_align != y_align)
	{
	  progress->y_align = y_align;
	  g_object_notify (B_OBJECT (progress), "text-yalign");
	}
      g_object_thaw_notify (B_OBJECT (progress));

      if (BTK_WIDGET_DRAWABLE (BTK_WIDGET (progress)))
	btk_widget_queue_resize (BTK_WIDGET (progress));
    }
}

void
btk_progress_set_format_string (BtkProgress *progress,
				const gchar *format)
{
  gchar *old_format;
  
  g_return_if_fail (BTK_IS_PROGRESS (progress));

  /* Turn on format, in case someone called
   * btk_progress_bar_set_text() and turned it off.
   */
  progress->use_text_format = TRUE;

  old_format = progress->format;

  if (!format)
    format = DEFAULT_FORMAT;

  progress->format = g_strdup (format);
  g_free (old_format);
  
  btk_widget_queue_resize (BTK_WIDGET (progress));
}

gchar *
btk_progress_get_current_text (BtkProgress *progress)
{
  g_return_val_if_fail (BTK_IS_PROGRESS (progress), NULL);

  if (!progress->adjustment)
    btk_progress_set_adjustment (progress, NULL);

  return btk_progress_build_string (progress, progress->adjustment->value,
				    btk_progress_get_current_percentage (progress));
}

gchar *
btk_progress_get_text_from_value (BtkProgress *progress,
				  gdouble      value)
{
  g_return_val_if_fail (BTK_IS_PROGRESS (progress), NULL);

  if (!progress->adjustment)
    btk_progress_set_adjustment (progress, NULL);

  return btk_progress_build_string (progress, value,
				    btk_progress_get_percentage_from_value (progress, value));
}

void
btk_progress_set_activity_mode (BtkProgress *progress,
				gboolean     activity_mode)
{
  g_return_if_fail (BTK_IS_PROGRESS (progress));

  if (progress->activity_mode != (activity_mode != FALSE))
    {
      progress->activity_mode = (activity_mode != FALSE);

      if (progress->activity_mode)
	BTK_PROGRESS_GET_CLASS (progress)->act_mode_enter (progress);

      if (BTK_WIDGET_DRAWABLE (BTK_WIDGET (progress)))
	btk_widget_queue_resize (BTK_WIDGET (progress));

      g_object_notify (B_OBJECT (progress), "activity-mode");
    }
}

#include "btkaliasdef.c"
