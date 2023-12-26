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

#include <string.h>

#undef BTK_DISABLE_DEPRECATED
#define __BTK_PROGRESS_BAR_C__

#include "btkprogressbar.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"


#define MIN_HORIZONTAL_BAR_WIDTH   150
#define MIN_HORIZONTAL_BAR_HEIGHT  20
#define MIN_VERTICAL_BAR_WIDTH     22
#define MIN_VERTICAL_BAR_HEIGHT    80

enum {
  PROP_0,

  /* Supported args */
  PROP_FRACTION,
  PROP_PULSE_STEP,
  PROP_ORIENTATION,
  PROP_TEXT,
  PROP_ELLIPSIZE,
  
  /* Deprecated args */
  PROP_ADJUSTMENT,
  PROP_BAR_STYLE,
  PROP_ACTIVITY_STEP,
  PROP_ACTIVITY_BLOCKS,
  PROP_DISCRETE_BLOCKS
};

static void btk_progress_bar_set_property  (BObject             *object,
					    guint                prop_id,
					    const BValue        *value,
					    BParamSpec          *pspec);
static void btk_progress_bar_get_property  (BObject             *object,
					    guint                prop_id,
					    BValue              *value,
					    BParamSpec          *pspec);
static gboolean btk_progress_bar_expose    (BtkWidget           *widget,
					    BdkEventExpose      *event);
static void btk_progress_bar_size_request  (BtkWidget           *widget,
					    BtkRequisition      *requisition);
static void btk_progress_bar_style_set     (BtkWidget           *widget,
					    BtkStyle            *previous);
static void btk_progress_bar_real_update   (BtkProgress         *progress);
static void btk_progress_bar_paint         (BtkProgress         *progress);
static void btk_progress_bar_act_mode_enter (BtkProgress        *progress);

static void btk_progress_bar_set_bar_style_internal       (BtkProgressBar *pbar,
							   BtkProgressBarStyle style);
static void btk_progress_bar_set_discrete_blocks_internal (BtkProgressBar *pbar,
							   guint           blocks);
static void btk_progress_bar_set_activity_step_internal   (BtkProgressBar *pbar,
							   guint           step);
static void btk_progress_bar_set_activity_blocks_internal (BtkProgressBar *pbar,
							   guint           blocks);


G_DEFINE_TYPE (BtkProgressBar, btk_progress_bar, BTK_TYPE_PROGRESS)

static void
btk_progress_bar_class_init (BtkProgressBarClass *class)
{
  BObjectClass *bobject_class;
  BtkWidgetClass *widget_class;
  BtkProgressClass *progress_class;
  
  bobject_class = B_OBJECT_CLASS (class);
  widget_class = (BtkWidgetClass *) class;
  progress_class = (BtkProgressClass *) class;

  bobject_class->set_property = btk_progress_bar_set_property;
  bobject_class->get_property = btk_progress_bar_get_property;
  
  widget_class->expose_event = btk_progress_bar_expose;
  widget_class->size_request = btk_progress_bar_size_request;
  widget_class->style_set = btk_progress_bar_style_set;

  progress_class->paint = btk_progress_bar_paint;
  progress_class->update = btk_progress_bar_real_update;
  progress_class->act_mode_enter = btk_progress_bar_act_mode_enter;

  g_object_class_install_property (bobject_class,
                                   PROP_ADJUSTMENT,
                                   g_param_spec_object ("adjustment",
                                                        P_("Adjustment"),
                                                        P_("The BtkAdjustment connected to the progress bar (Deprecated)"),
                                                        BTK_TYPE_ADJUSTMENT,
                                                        BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation",
						      P_("Orientation"),
						      P_("Orientation and growth direction of the progress bar"),
						      BTK_TYPE_PROGRESS_BAR_ORIENTATION,
						      BTK_PROGRESS_LEFT_TO_RIGHT,
						      BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_BAR_STYLE,
                                   g_param_spec_enum ("bar-style",
						      P_("Bar style"),
						      P_("Specifies the visual style of the bar in percentage mode (Deprecated)"),
						      BTK_TYPE_PROGRESS_BAR_STYLE,
						      BTK_PROGRESS_CONTINUOUS,
						      BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_ACTIVITY_STEP,
                                   g_param_spec_uint ("activity-step",
						      P_("Activity Step"),
						      P_("The increment used for each iteration in activity mode (Deprecated)"),
						      0, G_MAXUINT, 3,
						      BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_ACTIVITY_BLOCKS,
                                   g_param_spec_uint ("activity-blocks",
						      P_("Activity Blocks"),
						      P_("The number of blocks which can fit in the progress bar area in activity mode (Deprecated)"),
						      2, G_MAXUINT, 5,
						      BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_DISCRETE_BLOCKS,
                                   g_param_spec_uint ("discrete-blocks",
						      P_("Discrete Blocks"),
						      P_("The number of discrete blocks in a progress bar (when shown in the discrete style)"),
						      2, G_MAXUINT, 10,
						      BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
				   PROP_FRACTION,
				   g_param_spec_double ("fraction",
							P_("Fraction"),
							P_("The fraction of total work that has been completed"),
							0.0, 1.0, 0.0,
							BTK_PARAM_READWRITE));  
  
  g_object_class_install_property (bobject_class,
				   PROP_PULSE_STEP,
				   g_param_spec_double ("pulse-step",
							P_("Pulse Step"),
							P_("The fraction of total progress to move the bouncing block when pulsed"),
							0.0, 1.0, 0.1,
							BTK_PARAM_READWRITE));  
  
  g_object_class_install_property (bobject_class,
				   PROP_TEXT,
				   g_param_spec_string ("text",
							P_("Text"),
							P_("Text to be displayed in the progress bar"),
							NULL,
							BTK_PARAM_READWRITE));

  /**
   * BtkProgressBar:ellipsize:
   *
   * The preferred place to ellipsize the string, if the progressbar does 
   * not have enough room to display the entire string, specified as a 
   * #BangoEllisizeMode. 
   *
   * Note that setting this property to a value other than 
   * %BANGO_ELLIPSIZE_NONE has the side-effect that the progressbar requests 
   * only enough space to display the ellipsis "...". Another means to set a 
   * progressbar's width is btk_widget_set_size_request().
   *
   * Since: 2.6
   */
  g_object_class_install_property (bobject_class,
				   PROP_ELLIPSIZE,
                                   g_param_spec_enum ("ellipsize",
                                                      P_("Ellipsize"),
                                                      P_("The preferred place to ellipsize the string, if the progress bar "
                                                         "does not have enough room to display the entire string, if at all."),
						      BANGO_TYPE_ELLIPSIZE_MODE,
						      BANGO_ELLIPSIZE_NONE,
                                                      BTK_PARAM_READWRITE));
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("xspacing",
                                                             P_("XSpacing"),
                                                             P_("Extra spacing applied to the width of a progress bar."),
                                                             0, G_MAXINT, 7,
                                                             G_PARAM_READWRITE));
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("yspacing",
                                                             P_("YSpacing"),
                                                             P_("Extra spacing applied to the height of a progress bar."),
                                                             0, G_MAXINT, 7,
                                                             G_PARAM_READWRITE));

  /**
   * BtkProgressBar:min-horizontal-bar-width:
   *
   * The minimum horizontal width of the progress bar.
   *
   * Since: 2.14
   */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("min-horizontal-bar-width",
                                                             P_("Min horizontal bar width"),
                                                             P_("The minimum horizontal width of the progress bar"),
                                                             1, G_MAXINT, MIN_HORIZONTAL_BAR_WIDTH,
                                                             G_PARAM_READWRITE));
  /**
   * BtkProgressBar:min-horizontal-bar-height:
   *
   * Minimum horizontal height of the progress bar.
   *
   * Since: 2.14
   */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("min-horizontal-bar-height",
                                                             P_("Min horizontal bar height"),
                                                             P_("Minimum horizontal height of the progress bar"),
                                                             1, G_MAXINT, MIN_HORIZONTAL_BAR_HEIGHT,
                                                             G_PARAM_READWRITE));
  /**
   * BtkProgressBar:min-vertical-bar-width:
   *
   * The minimum vertical width of the progress bar.
   *
   * Since: 2.14
   */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("min-vertical-bar-width",
                                                             P_("Min vertical bar width"),
                                                             P_("The minimum vertical width of the progress bar"),
                                                             1, G_MAXINT, MIN_VERTICAL_BAR_WIDTH,
                                                             G_PARAM_READWRITE));
  /**
   * BtkProgressBar:min-vertical-bar-height:
   *
   * The minimum vertical height of the progress bar.
   *
   * Since: 2.14
   */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("min-vertical-bar-height",
                                                             P_("Min vertical bar height"),
                                                             P_("The minimum vertical height of the progress bar"),
                                                             1, G_MAXINT, MIN_VERTICAL_BAR_HEIGHT,
                                                             G_PARAM_READWRITE));
}

static void
btk_progress_bar_init (BtkProgressBar *pbar)
{
  pbar->bar_style = BTK_PROGRESS_CONTINUOUS;
  pbar->blocks = 10;
  pbar->in_block = -1;
  pbar->orientation = BTK_PROGRESS_LEFT_TO_RIGHT;
  pbar->pulse_fraction = 0.1;
  pbar->activity_pos = 0;
  pbar->activity_dir = 1;
  pbar->activity_step = 3;
  pbar->activity_blocks = 5;
  pbar->ellipsize = BANGO_ELLIPSIZE_NONE;
}

static void
btk_progress_bar_set_property (BObject      *object,
			       guint         prop_id,
			       const BValue *value,
			       BParamSpec   *pspec)
{
  BtkProgressBar *pbar;

  pbar = BTK_PROGRESS_BAR (object);

  switch (prop_id)
    {
    case PROP_ADJUSTMENT:
      btk_progress_set_adjustment (BTK_PROGRESS (pbar),
				   BTK_ADJUSTMENT (b_value_get_object (value)));
      break;
    case PROP_ORIENTATION:
      btk_progress_bar_set_orientation (pbar, b_value_get_enum (value));
      break;
    case PROP_BAR_STYLE:
      btk_progress_bar_set_bar_style_internal (pbar, b_value_get_enum (value));
      break;
    case PROP_ACTIVITY_STEP:
      btk_progress_bar_set_activity_step_internal (pbar, b_value_get_uint (value));
      break;
    case PROP_ACTIVITY_BLOCKS:
      btk_progress_bar_set_activity_blocks_internal (pbar, b_value_get_uint (value));
      break;
    case PROP_DISCRETE_BLOCKS:
      btk_progress_bar_set_discrete_blocks_internal (pbar, b_value_get_uint (value));
      break;
    case PROP_FRACTION:
      btk_progress_bar_set_fraction (pbar, b_value_get_double (value));
      break;
    case PROP_PULSE_STEP:
      btk_progress_bar_set_pulse_step (pbar, b_value_get_double (value));
      break;
    case PROP_TEXT:
      btk_progress_bar_set_text (pbar, b_value_get_string (value));
      break;
    case PROP_ELLIPSIZE:
      btk_progress_bar_set_ellipsize (pbar, b_value_get_enum (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_progress_bar_get_property (BObject      *object,
			       guint         prop_id,
			       BValue       *value,
			       BParamSpec   *pspec)
{
  BtkProgressBar *pbar;

  pbar = BTK_PROGRESS_BAR (object);

  switch (prop_id)
    {
    case PROP_ADJUSTMENT:
      b_value_set_object (value, BTK_PROGRESS (pbar)->adjustment);
      break;
    case PROP_ORIENTATION:
      b_value_set_enum (value, pbar->orientation);
      break;
    case PROP_BAR_STYLE:
      b_value_set_enum (value, pbar->bar_style);
      break;
    case PROP_ACTIVITY_STEP:
      b_value_set_uint (value, pbar->activity_step);
      break;
    case PROP_ACTIVITY_BLOCKS:
      b_value_set_uint (value, pbar->activity_blocks);
      break;
    case PROP_DISCRETE_BLOCKS:
      b_value_set_uint (value, pbar->blocks);
      break;
    case PROP_FRACTION:
      b_value_set_double (value, btk_progress_get_current_percentage (BTK_PROGRESS (pbar)));
      break;
    case PROP_PULSE_STEP:
      b_value_set_double (value, pbar->pulse_fraction);
      break;
    case PROP_TEXT:
      b_value_set_string (value, btk_progress_bar_get_text (pbar));
      break;
    case PROP_ELLIPSIZE:
      b_value_set_enum (value, pbar->ellipsize);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * btk_progress_bar_new:
 *
 * Creates a new #BtkProgressBar.
 *
 * Returns: a #BtkProgressBar.
 */
BtkWidget*
btk_progress_bar_new (void)
{
  BtkWidget *pbar;

  pbar = g_object_new (BTK_TYPE_PROGRESS_BAR, NULL);

  return pbar;
}

/**
 * btk_progress_bar_new_with_adjustment:
 * @adjustment: (allow-none):
 *
 * Creates a new #BtkProgressBar with an associated #BtkAdjustment.
 *
 * Returns: (transfer none): a #BtkProgressBar.
 */
BtkWidget*
btk_progress_bar_new_with_adjustment (BtkAdjustment *adjustment)
{
  BtkWidget *pbar;

  g_return_val_if_fail (BTK_IS_ADJUSTMENT (adjustment), NULL);

  pbar = g_object_new (BTK_TYPE_PROGRESS_BAR,
			 "adjustment", adjustment,
			 NULL);

  return pbar;
}

static void
btk_progress_bar_real_update (BtkProgress *progress)
{
  BtkProgressBar *pbar;
  BtkWidget *widget;

  g_return_if_fail (BTK_IS_PROGRESS (progress));

  pbar = BTK_PROGRESS_BAR (progress);
  widget = BTK_WIDGET (progress);
 
  if (pbar->bar_style == BTK_PROGRESS_CONTINUOUS ||
      BTK_PROGRESS (pbar)->activity_mode)
    {
      if (BTK_PROGRESS (pbar)->activity_mode)
	{
	  guint size;
          
	  /* advance the block */

	  if (pbar->orientation == BTK_PROGRESS_LEFT_TO_RIGHT ||
	      pbar->orientation == BTK_PROGRESS_RIGHT_TO_LEFT)
	    {
              /* Update our activity step. */
              
              pbar->activity_step = widget->allocation.width * pbar->pulse_fraction;
              
	      size = MAX (2, widget->allocation.width / pbar->activity_blocks);

	      if (pbar->activity_dir == 0)
		{
		  pbar->activity_pos += pbar->activity_step;
		  if (pbar->activity_pos + size >=
		      widget->allocation.width -
		      widget->style->xthickness)
		    {
		      pbar->activity_pos = MAX (0, widget->allocation.width -
			widget->style->xthickness - size);
		      pbar->activity_dir = 1;
		    }
		}
	      else
		{
		  pbar->activity_pos -= pbar->activity_step;
		  if (pbar->activity_pos <= widget->style->xthickness)
		    {
		      pbar->activity_pos = widget->style->xthickness;
		      pbar->activity_dir = 0;
		    }
		}
	    }
	  else
	    {
              /* Update our activity step. */
              
              pbar->activity_step = widget->allocation.height * pbar->pulse_fraction;
              
	      size = MAX (2, widget->allocation.height / pbar->activity_blocks);

	      if (pbar->activity_dir == 0)
		{
		  pbar->activity_pos += pbar->activity_step;
		  if (pbar->activity_pos + size >=
		      widget->allocation.height -
		      widget->style->ythickness)
		    {
		      pbar->activity_pos = MAX (0, widget->allocation.height -
			widget->style->ythickness - size);
		      pbar->activity_dir = 1;
		    }
		}
	      else
		{
		  pbar->activity_pos -= pbar->activity_step;
		  if (pbar->activity_pos <= widget->style->ythickness)
		    {
		      pbar->activity_pos = widget->style->ythickness;
		      pbar->activity_dir = 0;
		    }
		}
	    }
	}
      pbar->dirty = TRUE;
      btk_widget_queue_draw (BTK_WIDGET (progress));
    }
  else
    {
      gint in_block;
      
      in_block = -1 + (gint)(btk_progress_get_current_percentage (progress) *
			     (gdouble)pbar->blocks);
      
      if (pbar->in_block != in_block)
	{
	  pbar->in_block = in_block;
	  pbar->dirty = TRUE;
	  btk_widget_queue_draw (BTK_WIDGET (progress));
	}
    }
}

static gboolean
btk_progress_bar_expose (BtkWidget      *widget,
		     BdkEventExpose *event)
{
  BtkProgressBar *pbar;

  g_return_val_if_fail (BTK_IS_PROGRESS_BAR (widget), FALSE);

  pbar = BTK_PROGRESS_BAR (widget);

  if (pbar->dirty && btk_widget_is_drawable (widget))
    btk_progress_bar_paint (BTK_PROGRESS (pbar));

  return BTK_WIDGET_CLASS (btk_progress_bar_parent_class)->expose_event (widget, event);
}

static void
btk_progress_bar_size_request (BtkWidget      *widget,
			       BtkRequisition *requisition)
{
  BtkProgress *progress;
  BtkProgressBar *pbar;
  gchar *buf;
  BangoRectangle logical_rect;
  BangoLayout *layout;
  gint width, height;
  gint xspacing, yspacing;
  gint min_width, min_height;

  g_return_if_fail (BTK_IS_PROGRESS_BAR (widget));
  g_return_if_fail (requisition != NULL);

  btk_widget_style_get (widget,
                        "xspacing", &xspacing,
                        "yspacing", &yspacing,
                        NULL);

  progress = BTK_PROGRESS (widget);
  pbar = BTK_PROGRESS_BAR (widget);

  width = 2 * widget->style->xthickness + xspacing;
  height = 2 * widget->style->ythickness + yspacing;

  if (progress->show_text && pbar->bar_style != BTK_PROGRESS_DISCRETE)
    {
      if (!progress->adjustment)
	btk_progress_set_adjustment (progress, NULL);

      buf = btk_progress_get_text_from_value (progress, progress->adjustment->upper);

      layout = btk_widget_create_bango_layout (widget, buf);

      bango_layout_get_pixel_extents (layout, NULL, &logical_rect);
	  
      if (pbar->ellipsize)
	{
	  BangoContext *context;
	  BangoFontMetrics *metrics;
	  gint char_width;
	  
	  /* The minimum size for ellipsized text is ~ 3 chars */
	  context = bango_layout_get_context (layout);
	  metrics = bango_context_get_metrics (context, widget->style->font_desc, bango_context_get_language (context));
	  
	  char_width = bango_font_metrics_get_approximate_char_width (metrics);
	  bango_font_metrics_unref (metrics);
	  
	  width += BANGO_PIXELS (char_width) * 3;
	}
      else
	width += logical_rect.width;
      
      height += logical_rect.height;

      g_object_unref (layout);
      g_free (buf);
    }
  
  if (pbar->orientation == BTK_PROGRESS_LEFT_TO_RIGHT ||
      pbar->orientation == BTK_PROGRESS_RIGHT_TO_LEFT)
    btk_widget_style_get (widget,
			  "min-horizontal-bar-width", &min_width,
			  "min-horizontal-bar-height", &min_height,
			  NULL);
  else
    btk_widget_style_get (widget,
			  "min-vertical-bar-width", &min_width,
			  "min-vertical-bar-height", &min_height,
			  NULL);

  requisition->width = MAX (min_width, width);
  requisition->height = MAX (min_height, height);
}

static void
btk_progress_bar_style_set (BtkWidget      *widget,
    BtkStyle *previous)
{
  BtkProgressBar *pbar = BTK_PROGRESS_BAR (widget);

  pbar->dirty = TRUE;

  BTK_WIDGET_CLASS (btk_progress_bar_parent_class)->style_set (widget, previous);
}

static void
btk_progress_bar_act_mode_enter (BtkProgress *progress)
{
  BtkProgressBar *pbar;
  BtkWidget *widget;
  BtkProgressBarOrientation orientation;

  pbar = BTK_PROGRESS_BAR (progress);
  widget = BTK_WIDGET (progress);

  orientation = pbar->orientation;
  if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL) 
    {
      if (pbar->orientation == BTK_PROGRESS_LEFT_TO_RIGHT)
	orientation = BTK_PROGRESS_RIGHT_TO_LEFT;
      else if (pbar->orientation == BTK_PROGRESS_RIGHT_TO_LEFT)
	orientation = BTK_PROGRESS_LEFT_TO_RIGHT;
    }
  
  /* calculate start pos */

  if (orientation == BTK_PROGRESS_LEFT_TO_RIGHT ||
      orientation == BTK_PROGRESS_RIGHT_TO_LEFT)
    {
      if (orientation == BTK_PROGRESS_LEFT_TO_RIGHT)
	{
	  pbar->activity_pos = widget->style->xthickness;
	  pbar->activity_dir = 0;
	}
      else
	{
	  pbar->activity_pos = widget->allocation.width - 
	    widget->style->xthickness - (widget->allocation.height - 
		widget->style->ythickness * 2);
	  pbar->activity_dir = 1;
	}
    }
  else
    {
      if (orientation == BTK_PROGRESS_TOP_TO_BOTTOM)
	{
	  pbar->activity_pos = widget->style->ythickness;
	  pbar->activity_dir = 0;
	}
      else
	{
	  pbar->activity_pos = widget->allocation.height -
	    widget->style->ythickness - (widget->allocation.width - 
		widget->style->xthickness * 2);
	  pbar->activity_dir = 1;
	}
    }
}

static void
btk_progress_bar_get_activity (BtkProgressBar            *pbar,
			       BtkProgressBarOrientation  orientation,
			       gint                      *offset,
			       gint                      *amount)
{
  BtkWidget *widget = BTK_WIDGET (pbar);

  *offset = pbar->activity_pos;

  switch (orientation)
    {
    case BTK_PROGRESS_LEFT_TO_RIGHT:
    case BTK_PROGRESS_RIGHT_TO_LEFT:
      *amount = MAX (2, widget->allocation.width / pbar->activity_blocks);
      break;

    case BTK_PROGRESS_TOP_TO_BOTTOM:
    case BTK_PROGRESS_BOTTOM_TO_TOP:
      *amount = MAX (2, widget->allocation.height / pbar->activity_blocks);
      break;
    }
}

static void
btk_progress_bar_paint_activity (BtkProgressBar            *pbar,
				 BtkProgressBarOrientation  orientation)
{
  BtkWidget *widget = BTK_WIDGET (pbar);
  BtkProgress *progress = BTK_PROGRESS (pbar);
  BdkRectangle area;

  switch (orientation)
    {
    case BTK_PROGRESS_LEFT_TO_RIGHT:
    case BTK_PROGRESS_RIGHT_TO_LEFT:
      btk_progress_bar_get_activity (pbar, orientation, &area.x, &area.width);
      area.y = widget->style->ythickness;
      area.height = widget->allocation.height - 2 * widget->style->ythickness;
      break;

    case BTK_PROGRESS_TOP_TO_BOTTOM:
    case BTK_PROGRESS_BOTTOM_TO_TOP:
      btk_progress_bar_get_activity (pbar, orientation, &area.y, &area.height);
      area.x = widget->style->xthickness;
      area.width = widget->allocation.width - 2 * widget->style->xthickness;
      break;

    default:
      return;
      break;
    }

  btk_paint_box (widget->style,
		 progress->offscreen_pixmap,
		 BTK_STATE_PRELIGHT, BTK_SHADOW_OUT,
		 &area, widget, "bar",
		 area.x, area.y, area.width, area.height);
}

static void
btk_progress_bar_paint_continuous (BtkProgressBar            *pbar,
				   gint                       amount,
				   BtkProgressBarOrientation  orientation)
{
  BdkRectangle area;
  BtkWidget *widget = BTK_WIDGET (pbar);

  if (amount <= 0)
    return;

  switch (orientation)
    {
    case BTK_PROGRESS_LEFT_TO_RIGHT:
    case BTK_PROGRESS_RIGHT_TO_LEFT:
      area.width = amount;
      area.height = widget->allocation.height - widget->style->ythickness * 2;
      area.y = widget->style->ythickness;
      
      area.x = widget->style->xthickness;
      if (orientation == BTK_PROGRESS_RIGHT_TO_LEFT)
	area.x = widget->allocation.width - amount - area.x;
      break;
      
    case BTK_PROGRESS_TOP_TO_BOTTOM:
    case BTK_PROGRESS_BOTTOM_TO_TOP:
      area.width = widget->allocation.width - widget->style->xthickness * 2;
      area.height = amount;
      area.x = widget->style->xthickness;
      
      area.y = widget->style->ythickness;
      if (orientation == BTK_PROGRESS_BOTTOM_TO_TOP)
	area.y = widget->allocation.height - amount - area.y;
      break;
      
    default:
      return;
      break;
    }
  
  btk_paint_box (widget->style,
		 BTK_PROGRESS (pbar)->offscreen_pixmap,
		 BTK_STATE_PRELIGHT, BTK_SHADOW_OUT,
		 &area, widget, "bar",
		 area.x, area.y, area.width, area.height);
}

static void
btk_progress_bar_paint_discrete (BtkProgressBar            *pbar,
				 BtkProgressBarOrientation  orientation)
{
  BtkWidget *widget = BTK_WIDGET (pbar);
  gint i;

  for (i = 0; i <= pbar->in_block; i++)
    {
      BdkRectangle area;
      gint space;

      switch (orientation)
	{
	case BTK_PROGRESS_LEFT_TO_RIGHT:
	case BTK_PROGRESS_RIGHT_TO_LEFT:
	  space = widget->allocation.width - 2 * widget->style->xthickness;
	  
	  area.x = widget->style->xthickness + (i * space) / pbar->blocks;
	  area.y = widget->style->ythickness;
	  area.width = widget->style->xthickness + ((i + 1) * space) / pbar->blocks - area.x;
	  area.height = widget->allocation.height - 2 * widget->style->ythickness;

	  if (orientation == BTK_PROGRESS_RIGHT_TO_LEFT)
	    area.x = widget->allocation.width - area.width - area.x;
	  break;
	  
	case BTK_PROGRESS_TOP_TO_BOTTOM:
	case BTK_PROGRESS_BOTTOM_TO_TOP:
	  space = widget->allocation.height - 2 * widget->style->ythickness;
	  
	  area.x = widget->style->xthickness;
	  area.y = widget->style->ythickness + (i * space) / pbar->blocks;
	  area.width = widget->allocation.width - 2 * widget->style->xthickness;
	  area.height = widget->style->ythickness + ((i + 1) * space) / pbar->blocks - area.y;
	  
	  if (orientation == BTK_PROGRESS_BOTTOM_TO_TOP)
	    area.y = widget->allocation.height - area.height - area.y;
	  break;

	default:
	  return;
	  break;
	}
      
      btk_paint_box (widget->style,
		     BTK_PROGRESS (pbar)->offscreen_pixmap,
		     BTK_STATE_PRELIGHT, BTK_SHADOW_OUT,
		     &area, widget, "bar",
		     area.x, area.y, area.width, area.height);
    }
}

static void
btk_progress_bar_paint_text (BtkProgressBar            *pbar,
			     gint                       offset,
			     gint			amount,
			     BtkProgressBarOrientation  orientation)
{
  BtkProgress *progress = BTK_PROGRESS (pbar);
  BtkWidget *widget = BTK_WIDGET (pbar);
  gint x;
  gint y;
  gchar *buf;
  BdkRectangle rect;
  BangoLayout *layout;
  BangoRectangle logical_rect;
  BdkRectangle prelight_clip, start_clip, end_clip;
  gfloat text_xalign = progress->x_align;
  gfloat text_yalign = progress->y_align;

  if (btk_widget_get_direction (widget) != BTK_TEXT_DIR_LTR)
    text_xalign = 1.0 - text_xalign;

  buf = btk_progress_get_current_text (progress);
  
  layout = btk_widget_create_bango_layout (widget, buf);
  bango_layout_set_ellipsize (layout, pbar->ellipsize);
  if (pbar->ellipsize)
    bango_layout_set_width (layout, widget->allocation.width * BANGO_SCALE);

  bango_layout_get_pixel_extents (layout, NULL, &logical_rect);
  
  x = widget->style->xthickness + 1 + text_xalign *
      (widget->allocation.width - 2 * widget->style->xthickness -
       2 - logical_rect.width);

  y = widget->style->ythickness + 1 + text_yalign *
      (widget->allocation.height - 2 * widget->style->ythickness -
       2 - logical_rect.height);

  rect.x = widget->style->xthickness;
  rect.y = widget->style->ythickness;
  rect.width = widget->allocation.width - 2 * widget->style->xthickness;
  rect.height = widget->allocation.height - 2 * widget->style->ythickness;

  prelight_clip = start_clip = end_clip = rect;

  switch (orientation)
    {
    case BTK_PROGRESS_LEFT_TO_RIGHT:
      if (offset != -1)
	prelight_clip.x = offset;
      prelight_clip.width = amount;
      start_clip.width = prelight_clip.x - start_clip.x;
      end_clip.x = start_clip.x + start_clip.width + prelight_clip.width;
      end_clip.width -= prelight_clip.width + start_clip.width;
      break;
      
    case BTK_PROGRESS_RIGHT_TO_LEFT:
      if (offset != -1)
	prelight_clip.x = offset;
      else
	prelight_clip.x = rect.x + rect.width - amount;
      prelight_clip.width = amount;
      start_clip.width = prelight_clip.x - start_clip.x;
      end_clip.x = start_clip.x + start_clip.width + prelight_clip.width;
      end_clip.width -= prelight_clip.width + start_clip.width;
      break;
       
    case BTK_PROGRESS_TOP_TO_BOTTOM:
      if (offset != -1)
	prelight_clip.y = offset;
      prelight_clip.height = amount;
      start_clip.height = prelight_clip.y - start_clip.y;
      end_clip.y = start_clip.y + start_clip.height + prelight_clip.height;
      end_clip.height -= prelight_clip.height + start_clip.height;
      break;
      
    case BTK_PROGRESS_BOTTOM_TO_TOP:
      if (offset != -1)
	prelight_clip.y = offset;
      else
	prelight_clip.y = rect.y + rect.height - amount;
      prelight_clip.height = amount;
      start_clip.height = prelight_clip.y - start_clip.y;
      end_clip.y = start_clip.y + start_clip.height + prelight_clip.height;
      end_clip.height -= prelight_clip.height + start_clip.height;
      break;
    }

  if (start_clip.width > 0 && start_clip.height > 0)
    btk_paint_layout (widget->style,
		      progress->offscreen_pixmap,
		      BTK_STATE_NORMAL,
		      FALSE,
		      &start_clip,
		      widget,
		      "progressbar",
		      x, y,
		      layout);

  if (end_clip.width > 0 && end_clip.height > 0)
    btk_paint_layout (widget->style,
		      progress->offscreen_pixmap,
		      BTK_STATE_NORMAL,
		      FALSE,
		      &end_clip,
		      widget,
		      "progressbar",
		      x, y,
		      layout);

  btk_paint_layout (widget->style,
		    progress->offscreen_pixmap,
		    BTK_STATE_PRELIGHT,
		    FALSE,
		    &prelight_clip,
		    widget,
		    "progressbar",
		    x, y,
		    layout);

  g_object_unref (layout);
  g_free (buf);
}

static void
btk_progress_bar_paint (BtkProgress *progress)
{
  BtkProgressBar *pbar;
  BtkWidget *widget;

  BtkProgressBarOrientation orientation;

  g_return_if_fail (BTK_IS_PROGRESS_BAR (progress));

  pbar = BTK_PROGRESS_BAR (progress);
  widget = BTK_WIDGET (progress);

  orientation = pbar->orientation;
  if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL) 
    {
      if (pbar->orientation == BTK_PROGRESS_LEFT_TO_RIGHT)
	orientation = BTK_PROGRESS_RIGHT_TO_LEFT;
      else if (pbar->orientation == BTK_PROGRESS_RIGHT_TO_LEFT)
	orientation = BTK_PROGRESS_LEFT_TO_RIGHT;
    }
 
  if (progress->offscreen_pixmap)
    {
      btk_paint_box (widget->style,
		     progress->offscreen_pixmap,
		     BTK_STATE_NORMAL, BTK_SHADOW_IN, 
		     NULL, widget, "trough",
		     0, 0,
		     widget->allocation.width,
		     widget->allocation.height);
      
      if (progress->activity_mode)
	{
	  btk_progress_bar_paint_activity (pbar, orientation);

	  if (BTK_PROGRESS (pbar)->show_text)
	    {
	      gint offset;
	      gint amount;

	      btk_progress_bar_get_activity (pbar, orientation, &offset, &amount);
	      btk_progress_bar_paint_text (pbar, offset, amount, orientation);
	    }
	}
      else
	{
	  gint amount;
	  gint space;
	  
	  if (orientation == BTK_PROGRESS_LEFT_TO_RIGHT ||
	      orientation == BTK_PROGRESS_RIGHT_TO_LEFT)
	    space = widget->allocation.width - 2 * widget->style->xthickness;
	  else
	    space = widget->allocation.height - 2 * widget->style->ythickness;
	  
	  amount = space *
	    btk_progress_get_current_percentage (BTK_PROGRESS (pbar));
	  
	  if (pbar->bar_style == BTK_PROGRESS_CONTINUOUS)
	    {
	      btk_progress_bar_paint_continuous (pbar, amount, orientation);

	      if (BTK_PROGRESS (pbar)->show_text)
		btk_progress_bar_paint_text (pbar, -1, amount, orientation);
	    }
	  else
	    btk_progress_bar_paint_discrete (pbar, orientation);
	}

      pbar->dirty = FALSE;
    }
}

static void
btk_progress_bar_set_bar_style_internal (BtkProgressBar     *pbar,
					 BtkProgressBarStyle bar_style)
{
  g_return_if_fail (BTK_IS_PROGRESS_BAR (pbar));

  if (pbar->bar_style != bar_style)
    {
      pbar->bar_style = bar_style;

      if (btk_widget_is_drawable (BTK_WIDGET (pbar)))
	btk_widget_queue_resize (BTK_WIDGET (pbar));

      g_object_notify (B_OBJECT (pbar), "bar-style");
    }
}

static void
btk_progress_bar_set_discrete_blocks_internal (BtkProgressBar *pbar,
					       guint           blocks)
{
  g_return_if_fail (BTK_IS_PROGRESS_BAR (pbar));
  g_return_if_fail (blocks > 1);

  if (pbar->blocks != blocks)
    {
      pbar->blocks = blocks;

      if (btk_widget_is_drawable (BTK_WIDGET (pbar)))
	btk_widget_queue_resize (BTK_WIDGET (pbar));

      g_object_notify (B_OBJECT (pbar), "discrete-blocks");
    }
}

static void
btk_progress_bar_set_activity_step_internal (BtkProgressBar *pbar,
					     guint           step)
{
  g_return_if_fail (BTK_IS_PROGRESS_BAR (pbar));

  if (pbar->activity_step != step)
    {
      pbar->activity_step = step;
      g_object_notify (B_OBJECT (pbar), "activity-step");
    }
}

static void
btk_progress_bar_set_activity_blocks_internal (BtkProgressBar *pbar,
					       guint           blocks)
{
  g_return_if_fail (BTK_IS_PROGRESS_BAR (pbar));
  g_return_if_fail (blocks > 1);

  if (pbar->activity_blocks != blocks)
    {
      pbar->activity_blocks = blocks;
      g_object_notify (B_OBJECT (pbar), "activity-blocks");
    }
}

/*******************************************************************/

/**
 * btk_progress_bar_set_fraction:
 * @pbar: a #BtkProgressBar
 * @fraction: fraction of the task that's been completed
 * 
 * Causes the progress bar to "fill in" the given fraction
 * of the bar. The fraction should be between 0.0 and 1.0,
 * inclusive.
 * 
 **/
void
btk_progress_bar_set_fraction (BtkProgressBar *pbar,
                               gdouble         fraction)
{
  g_return_if_fail (BTK_IS_PROGRESS_BAR (pbar));

  /* If we know the percentage, we don't want activity mode. */
  btk_progress_set_activity_mode (BTK_PROGRESS (pbar), FALSE);
  
  /* We use the deprecated BtkProgress interface internally.
   * Once everything's been deprecated for a good long time,
   * we can clean up all this code.
   */
  btk_progress_set_percentage (BTK_PROGRESS (pbar), fraction);

  g_object_notify (B_OBJECT (pbar), "fraction");
}

/**
 * btk_progress_bar_pulse:
 * @pbar: a #BtkProgressBar
 * 
 * Indicates that some progress is made, but you don't know how much.
 * Causes the progress bar to enter "activity mode," where a block
 * bounces back and forth. Each call to btk_progress_bar_pulse()
 * causes the block to move by a little bit (the amount of movement
 * per pulse is determined by btk_progress_bar_set_pulse_step()).
 **/
void
btk_progress_bar_pulse (BtkProgressBar *pbar)
{  
  g_return_if_fail (BTK_IS_PROGRESS_BAR (pbar));

  /* If we don't know the percentage, we must want activity mode. */
  btk_progress_set_activity_mode (BTK_PROGRESS (pbar), TRUE);

  /* Sigh. */
  btk_progress_bar_real_update (BTK_PROGRESS (pbar));
}

/**
 * btk_progress_bar_set_text:
 * @pbar: a #BtkProgressBar
 * @text: (allow-none): a UTF-8 string, or %NULL 
 * 
 * Causes the given @text to appear superimposed on the progress bar.
 **/
void
btk_progress_bar_set_text (BtkProgressBar *pbar,
                           const gchar    *text)
{
  g_return_if_fail (BTK_IS_PROGRESS_BAR (pbar));
  
  btk_progress_set_show_text (BTK_PROGRESS (pbar), text && *text);
  btk_progress_set_format_string (BTK_PROGRESS (pbar), text);
  
  /* We don't support formats in this interface, but turn
   * them back on for NULL, which should put us back to
   * the initial state.
   */
  BTK_PROGRESS (pbar)->use_text_format = (text == NULL);
  
  g_object_notify (B_OBJECT (pbar), "text");
}

/**
 * btk_progress_bar_set_pulse_step:
 * @pbar: a #BtkProgressBar
 * @fraction: fraction between 0.0 and 1.0
 * 
 * Sets the fraction of total progress bar length to move the
 * bouncing block for each call to btk_progress_bar_pulse().
 **/
void
btk_progress_bar_set_pulse_step   (BtkProgressBar *pbar,
                                   gdouble         fraction)
{
  g_return_if_fail (BTK_IS_PROGRESS_BAR (pbar));
  
  pbar->pulse_fraction = fraction;

  g_object_notify (B_OBJECT (pbar), "pulse-step");
}

void
btk_progress_bar_update (BtkProgressBar *pbar,
			 gdouble         percentage)
{
  g_return_if_fail (BTK_IS_PROGRESS_BAR (pbar));

  /* Use of btk_progress_bar_update() is deprecated ! 
   * Use btk_progress_bar_set_percentage ()
   */   

  btk_progress_set_percentage (BTK_PROGRESS (pbar), percentage);
}

/**
 * btk_progress_bar_set_orientation:
 * @pbar: a #BtkProgressBar
 * @orientation: orientation of the progress bar
 * 
 * Causes the progress bar to switch to a different orientation
 * (left-to-right, right-to-left, top-to-bottom, or bottom-to-top). 
 **/
void
btk_progress_bar_set_orientation (BtkProgressBar           *pbar,
				  BtkProgressBarOrientation orientation)
{
  g_return_if_fail (BTK_IS_PROGRESS_BAR (pbar));

  if (pbar->orientation != orientation)
    {
      pbar->orientation = orientation;

      if (btk_widget_is_drawable (BTK_WIDGET (pbar)))
	btk_widget_queue_resize (BTK_WIDGET (pbar));

      g_object_notify (B_OBJECT (pbar), "orientation");
    }
}

/**
 * btk_progress_bar_get_text:
 * @pbar: a #BtkProgressBar
 * 
 * Retrieves the text displayed superimposed on the progress bar,
 * if any, otherwise %NULL. The return value is a reference
 * to the text, not a copy of it, so will become invalid
 * if you change the text in the progress bar.
 * 
 * Return value: text, or %NULL; this string is owned by the widget
 * and should not be modified or freed.
 **/
const gchar*
btk_progress_bar_get_text (BtkProgressBar *pbar)
{
  g_return_val_if_fail (BTK_IS_PROGRESS_BAR (pbar), NULL);

  if (BTK_PROGRESS (pbar)->use_text_format)
    return NULL;
  else
    return BTK_PROGRESS (pbar)->format;
}

/**
 * btk_progress_bar_get_fraction:
 * @pbar: a #BtkProgressBar
 * 
 * Returns the current fraction of the task that's been completed.
 * 
 * Return value: a fraction from 0.0 to 1.0
 **/
gdouble
btk_progress_bar_get_fraction (BtkProgressBar *pbar)
{
  g_return_val_if_fail (BTK_IS_PROGRESS_BAR (pbar), 0);

  return btk_progress_get_current_percentage (BTK_PROGRESS (pbar));
}

/**
 * btk_progress_bar_get_pulse_step:
 * @pbar: a #BtkProgressBar
 * 
 * Retrieves the pulse step set with btk_progress_bar_set_pulse_step()
 * 
 * Return value: a fraction from 0.0 to 1.0
 **/
gdouble
btk_progress_bar_get_pulse_step (BtkProgressBar *pbar)
{
  g_return_val_if_fail (BTK_IS_PROGRESS_BAR (pbar), 0);

  return pbar->pulse_fraction;
}

/**
 * btk_progress_bar_get_orientation:
 * @pbar: a #BtkProgressBar
 * 
 * Retrieves the current progress bar orientation.
 * 
 * Return value: orientation of the progress bar
 **/
BtkProgressBarOrientation
btk_progress_bar_get_orientation (BtkProgressBar *pbar)
{
  g_return_val_if_fail (BTK_IS_PROGRESS_BAR (pbar), 0);

  return pbar->orientation;
}

void
btk_progress_bar_set_bar_style (BtkProgressBar     *pbar,
				BtkProgressBarStyle bar_style)
{
  g_return_if_fail (BTK_IS_PROGRESS_BAR (pbar));

  btk_progress_bar_set_bar_style_internal (pbar, bar_style);
}

void
btk_progress_bar_set_discrete_blocks (BtkProgressBar *pbar,
				      guint           blocks)
{
  g_return_if_fail (BTK_IS_PROGRESS_BAR (pbar));
  g_return_if_fail (blocks > 1);

  btk_progress_bar_set_discrete_blocks_internal (pbar, blocks);
}

void
btk_progress_bar_set_activity_step (BtkProgressBar *pbar,
                                    guint           step)
{
  g_return_if_fail (BTK_IS_PROGRESS_BAR (pbar));

  btk_progress_bar_set_activity_step_internal (pbar, step);
}

void
btk_progress_bar_set_activity_blocks (BtkProgressBar *pbar,
				      guint           blocks)
{
  g_return_if_fail (BTK_IS_PROGRESS_BAR (pbar));
  g_return_if_fail (blocks > 1);

  btk_progress_bar_set_activity_blocks_internal (pbar, blocks);
}

/**
 * btk_progress_bar_set_ellipsize:
 * @pbar: a #BtkProgressBar
 * @mode: a #BangoEllipsizeMode
 *
 * Sets the mode used to ellipsize (add an ellipsis: "...") the text 
 * if there is not enough space to render the entire string.
 *
 * Since: 2.6
 **/
void
btk_progress_bar_set_ellipsize (BtkProgressBar     *pbar,
				BangoEllipsizeMode  mode)
{
  g_return_if_fail (BTK_IS_PROGRESS_BAR (pbar));
  g_return_if_fail (mode >= BANGO_ELLIPSIZE_NONE && 
		    mode <= BANGO_ELLIPSIZE_END);
  
  if ((BangoEllipsizeMode)pbar->ellipsize != mode)
    {
      pbar->ellipsize = mode;

      g_object_notify (B_OBJECT (pbar), "ellipsize");
      btk_widget_queue_resize (BTK_WIDGET (pbar));
    }
}

/**
 * btk_progress_bar_get_ellipsize:
 * @pbar: a #BtkProgressBar
 *
 * Returns the ellipsizing position of the progressbar. 
 * See btk_progress_bar_set_ellipsize().
 *
 * Return value: #BangoEllipsizeMode
 *
 * Since: 2.6
 **/
BangoEllipsizeMode 
btk_progress_bar_get_ellipsize (BtkProgressBar *pbar)
{
  g_return_val_if_fail (BTK_IS_PROGRESS_BAR (pbar), BANGO_ELLIPSIZE_NONE);

  return pbar->ellipsize;
}

#include "btkaliasdef.c"
