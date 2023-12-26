/* btkcellrendererprogress.c
 * Copyright (C) 2002 Naba Kumar <kh_naba@users.sourceforge.net>
 * heavily modified by Jörgen Scheibengruber <mfcn@gmx.de>
 * heavily modified by Marco Pesenti Gritti <marco@bunny.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
 * Modified by the BTK+ Team and others 1997-2007.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"
#include <stdlib.h>

#include "btkcellrendererprogress.h"
#include "btkprogressbar.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

#define BTK_CELL_RENDERER_PROGRESS_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object),                        \
                                                                                     BTK_TYPE_CELL_RENDERER_PROGRESS, \
                                                                                     BtkCellRendererProgressPrivate))

enum
{
  PROP_0,
  PROP_VALUE,
  PROP_TEXT,
  PROP_PULSE,
  PROP_TEXT_XALIGN,
  PROP_TEXT_YALIGN,
  PROP_ORIENTATION
}; 

struct _BtkCellRendererProgressPrivate
{
  gint value;
  gchar *text;
  gchar *label;
  gint min_h;
  gint min_w;
  gint pulse;
  gint offset;
  gfloat text_xalign;
  gfloat text_yalign;
  BtkProgressBarOrientation orientation;
};

static void btk_cell_renderer_progress_finalize     (GObject                 *object);
static void btk_cell_renderer_progress_get_property (GObject                 *object,
						     guint                    param_id,
						     GValue                  *value,
						     GParamSpec              *pspec);
static void btk_cell_renderer_progress_set_property (GObject                 *object,
						     guint                    param_id,
						     const GValue            *value,
						     GParamSpec              *pspec);
static void btk_cell_renderer_progress_set_value    (BtkCellRendererProgress *cellprogress,
						     gint                     value);
static void btk_cell_renderer_progress_set_text     (BtkCellRendererProgress *cellprogress,
						     const gchar             *text);
static void btk_cell_renderer_progress_set_pulse    (BtkCellRendererProgress *cellprogress,
						     gint                     pulse);
static void compute_dimensions                      (BtkCellRenderer         *cell,
						     BtkWidget               *widget,
						     const gchar             *text,
						     gint                    *width,
						     gint                    *height);
static void btk_cell_renderer_progress_get_size     (BtkCellRenderer         *cell,
						     BtkWidget               *widget,
						     BdkRectangle            *cell_area,
						     gint                    *x_offset,
						     gint                    *y_offset,
						     gint                    *width,
						     gint                    *height);
static void btk_cell_renderer_progress_render       (BtkCellRenderer         *cell,
						     BdkWindow               *window,
						     BtkWidget               *widget,
						     BdkRectangle            *background_area,
						     BdkRectangle            *cell_area,
						     BdkRectangle            *expose_area,
						     guint                    flags);

     
G_DEFINE_TYPE (BtkCellRendererProgress, btk_cell_renderer_progress, BTK_TYPE_CELL_RENDERER)

static void
btk_cell_renderer_progress_class_init (BtkCellRendererProgressClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BtkCellRendererClass *cell_class = BTK_CELL_RENDERER_CLASS (klass);
  
  object_class->finalize = btk_cell_renderer_progress_finalize;
  object_class->get_property = btk_cell_renderer_progress_get_property;
  object_class->set_property = btk_cell_renderer_progress_set_property;
  
  cell_class->get_size = btk_cell_renderer_progress_get_size;
  cell_class->render = btk_cell_renderer_progress_render;
  
  /**
   * BtkCellRendererProgress:value:
   * 
   * The "value" property determines the percentage to which the
   * progress bar will be "filled in". 
   *
   * Since: 2.6
   **/
  g_object_class_install_property (object_class,
				   PROP_VALUE,
				   g_param_spec_int ("value",
						     P_("Value"),
						     P_("Value of the progress bar"),
						     0, 100, 0,
						     BTK_PARAM_READWRITE));

  /**
   * BtkCellRendererProgress:text:
   * 
   * The "text" property determines the label which will be drawn
   * over the progress bar. Setting this property to %NULL causes the default 
   * label to be displayed. Setting this property to an empty string causes 
   * no label to be displayed.
   *
   * Since: 2.6
   **/
  g_object_class_install_property (object_class,
				   PROP_TEXT,
				   g_param_spec_string ("text",
							P_("Text"),
							P_("Text on the progress bar"),
							NULL,
							BTK_PARAM_READWRITE));

  /**
   * BtkCellRendererProgress:pulse:
   * 
   * Setting this to a non-negative value causes the cell renderer to
   * enter "activity mode", where a block bounces back and forth to 
   * indicate that some progress is made, without specifying exactly how
   * much.
   *
   * Each increment of the property causes the block to move by a little 
   * bit.
   *
   * To indicate that the activity has not started yet, set the property
   * to zero. To indicate completion, set the property to %G_MAXINT.
   *
   * Since: 2.12
   */
  g_object_class_install_property (object_class,
                                   PROP_PULSE,
                                   g_param_spec_int ("pulse",
                                                     P_("Pulse"),
                                                     P_("Set this to positive values to indicate that some progress is made, but you don't know how much."),
                                                     -1, G_MAXINT, -1,
                                                     BTK_PARAM_READWRITE));

  /**
   * BtkCellRendererProgress:text-xalign:
   *
   * The "text-xalign" property controls the horizontal alignment of the
   * text in the progress bar.  Valid values range from 0 (left) to 1
   * (right).  Reserved for RTL layouts.
   *
   * Since: 2.12
   */
  g_object_class_install_property (object_class,
                                   PROP_TEXT_XALIGN,
                                   g_param_spec_float ("text-xalign",
                                                       P_("Text x alignment"),
                                                       P_("The horizontal text alignment, from 0 (left) to 1 (right). Reversed for RTL layouts."),
                                                       0.0, 1.0, 0.5,
                                                       BTK_PARAM_READWRITE));

  /**
   * BtkCellRendererProgress:text-yalign:
   *
   * The "text-yalign" property controls the vertical alignment of the
   * text in the progress bar.  Valid values range from 0 (top) to 1
   * (bottom).
   *
   * Since: 2.12
   */
  g_object_class_install_property (object_class,
                                   PROP_TEXT_YALIGN,
                                   g_param_spec_float ("text-yalign",
                                                       P_("Text y alignment"),
                                                       P_("The vertical text alignment, from 0 (top) to 1 (bottom)."),
                                                       0.0, 1.0, 0.5,
                                                       BTK_PARAM_READWRITE));

  /**
   * BtkCellRendererProgress:orientation:
   *
   * The "orientation" property controls the direction and growth
   * direction of the progress bar (left-to-right, right-to-left,
   * top-to-bottom or bottom-to-top).
   *
   * Since: 2.12
   */
  g_object_class_install_property (object_class,
                                   PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation",
                                                      P_("Orientation"),
                                                      P_("Orientation and growth direction of the progress bar"),
                                                      BTK_TYPE_PROGRESS_BAR_ORIENTATION,
                                                      BTK_PROGRESS_LEFT_TO_RIGHT,
                                                      BTK_PARAM_READWRITE));


  g_type_class_add_private (object_class, 
			    sizeof (BtkCellRendererProgressPrivate));
}

static void
btk_cell_renderer_progress_init (BtkCellRendererProgress *cellprogress)
{
  BtkCellRendererProgressPrivate *priv = BTK_CELL_RENDERER_PROGRESS_GET_PRIVATE (cellprogress);

  priv->value = 0;
  priv->text = NULL;
  priv->label = NULL;
  priv->min_w = -1;
  priv->min_h = -1;
  priv->pulse = -1;
  priv->offset = 0;

  priv->text_xalign = 0.5;
  priv->text_yalign = 0.5;

  priv->orientation = BTK_PROGRESS_LEFT_TO_RIGHT;

  cellprogress->priv = priv;
}


/**
 * btk_cell_renderer_progress_new:
 * 
 * Creates a new #BtkCellRendererProgress. 
 *
 * Return value: the new cell renderer
 *
 * Since: 2.6
 **/
BtkCellRenderer*
btk_cell_renderer_progress_new (void)
{
  return g_object_new (BTK_TYPE_CELL_RENDERER_PROGRESS, NULL);
}

static void
btk_cell_renderer_progress_finalize (GObject *object)
{
  BtkCellRendererProgress *cellprogress = BTK_CELL_RENDERER_PROGRESS (object);
  BtkCellRendererProgressPrivate *priv = cellprogress->priv;
  
  g_free (priv->text);
  g_free (priv->label);
  
  G_OBJECT_CLASS (btk_cell_renderer_progress_parent_class)->finalize (object);
}

static void
btk_cell_renderer_progress_get_property (GObject *object,
					 guint param_id,
					 GValue *value,
					 GParamSpec *pspec)
{
  BtkCellRendererProgress *cellprogress = BTK_CELL_RENDERER_PROGRESS (object);
  BtkCellRendererProgressPrivate *priv = cellprogress->priv;
  
  switch (param_id)
    {
    case PROP_VALUE:
      g_value_set_int (value, priv->value);
      break;
    case PROP_TEXT:
      g_value_set_string (value, priv->text);
      break;
    case PROP_PULSE:
      g_value_set_int (value, priv->pulse);
      break;
    case PROP_TEXT_XALIGN:
      g_value_set_float (value, priv->text_xalign);
      break;
    case PROP_TEXT_YALIGN:
      g_value_set_float (value, priv->text_yalign);
      break;
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }
}

static void
btk_cell_renderer_progress_set_property (GObject *object,
					 guint param_id,
					 const GValue *value,
					 GParamSpec   *pspec)
{
  BtkCellRendererProgress *cellprogress = BTK_CELL_RENDERER_PROGRESS (object);
  BtkCellRendererProgressPrivate *priv = cellprogress->priv;
  
  switch (param_id)
    {
    case PROP_VALUE:
      btk_cell_renderer_progress_set_value (cellprogress, 
					    g_value_get_int (value));
      break;
    case PROP_TEXT:
      btk_cell_renderer_progress_set_text (cellprogress,
					   g_value_get_string (value));
      break;
    case PROP_PULSE:
      btk_cell_renderer_progress_set_pulse (cellprogress, 
					    g_value_get_int (value));
      break;
    case PROP_TEXT_XALIGN:
      priv->text_xalign = g_value_get_float (value);
      break;
    case PROP_TEXT_YALIGN:
      priv->text_yalign = g_value_get_float (value);
      break;
    case PROP_ORIENTATION:
      priv->orientation = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }
}

static void
recompute_label (BtkCellRendererProgress *cellprogress)
{
  BtkCellRendererProgressPrivate *priv = cellprogress->priv;
  gchar *label;

  if (priv->text)
    label = g_strdup (priv->text);
  else if (priv->pulse < 0)
    label = g_strdup_printf (C_("progress bar label", "%d %%"), priv->value);
  else
    label = NULL;
 
  g_free (priv->label);
  priv->label = label;
}

static void
btk_cell_renderer_progress_set_value (BtkCellRendererProgress *cellprogress, 
				      gint                     value)
{
  cellprogress->priv->value = value;

  recompute_label (cellprogress);
}

static void
btk_cell_renderer_progress_set_text (BtkCellRendererProgress *cellprogress, 
				     const gchar             *text)
{
  gchar *new_text;

  new_text = g_strdup (text);
  g_free (cellprogress->priv->text);
  cellprogress->priv->text = new_text;

  recompute_label (cellprogress);
}

static void
btk_cell_renderer_progress_set_pulse (BtkCellRendererProgress *cellprogress, 
				      gint                     pulse)
{
   BtkCellRendererProgressPrivate *priv = cellprogress->priv;

   if (pulse != priv->pulse)
     {
       if (pulse <= 0)
         priv->offset = 0;
       else
         priv->offset = pulse;
     }

   priv->pulse = pulse;

   recompute_label (cellprogress);
}

static void
compute_dimensions (BtkCellRenderer *cell,
		    BtkWidget       *widget, 
		    const gchar     *text, 
		    gint            *width, 
		    gint            *height)
{
  BangoRectangle logical_rect;
  BangoLayout *layout;
  
  layout = btk_widget_create_bango_layout (widget, text);
  bango_layout_get_pixel_extents (layout, NULL, &logical_rect);
  
  if (width)
    *width = logical_rect.width + cell->xpad * 2;
  
  if (height)
    *height = logical_rect.height + cell->ypad * 2;

  g_object_unref (layout);
}

static void
btk_cell_renderer_progress_get_size (BtkCellRenderer *cell,
				     BtkWidget       *widget,
				     BdkRectangle    *cell_area,
				     gint            *x_offset,
				     gint            *y_offset,
				     gint            *width,
				     gint            *height)
{
  BtkCellRendererProgress *cellprogress = BTK_CELL_RENDERER_PROGRESS (cell);
  BtkCellRendererProgressPrivate *priv = cellprogress->priv;
  gint w, h;
  gchar *text;

  if (priv->min_w < 0)
    {
      text = g_strdup_printf (C_("progress bar label", "%d %%"), 100);
      compute_dimensions (cell, widget, text,
			  &priv->min_w,
			  &priv->min_h);
      g_free (text);
    }
  
  compute_dimensions (cell, widget, priv->label, &w, &h);
  
  if (width)
    *width = MAX (priv->min_w, w);
  
  if (height)
    *height = MIN (priv->min_h, h);

  /* FIXME: at the moment cell_area is only set when we are requesting
   * the size for drawing the focus rectangle. We now just return
   * the last size we used for drawing the progress bar, which will
   * work for now. Not a really nice solution though.
   */
  if (cell_area)
    {
      if (width)
        *width = cell_area->width;
      if (height)
        *height = cell_area->height;
    }

  if (x_offset) *x_offset = 0;
  if (y_offset) *y_offset = 0;
}

static inline gint
get_bar_size (gint pulse,
	      gint value,
	      gint full_size)
{
  gint bar_size;

  if (pulse < 0)
    bar_size = full_size * MAX (0, value) / 100;
  else if (pulse == 0)
    bar_size = 0;
  else if (pulse == G_MAXINT)
    bar_size = full_size;
  else
    bar_size = MAX (2, full_size / 5);

  return bar_size;
}

static inline gint
get_bar_position (gint     start,
		  gint     full_size,
		  gint     bar_size,
		  gint     pulse,
		  gint     offset,
		  gboolean is_rtl)
{
  gint position;

  if (pulse < 0 || pulse == 0 || pulse == G_MAXINT)
    {
      position = is_rtl ? (start + full_size - bar_size) : start;
    }
  else
    {
      position = (is_rtl ? offset + 12 : offset) % 24;
      if (position > 12)
	position = 24 - position;
      position = start + full_size * position / 15;
    }

  return position;
}

static void
btk_cell_renderer_progress_render (BtkCellRenderer *cell,
				   BdkWindow       *window,
				   BtkWidget       *widget,
				   BdkRectangle    *background_area,
				   BdkRectangle    *cell_area,
				   BdkRectangle    *expose_area,
				   guint            flags)
{
  BtkCellRendererProgress *cellprogress = BTK_CELL_RENDERER_PROGRESS (cell);
  BtkCellRendererProgressPrivate *priv= cellprogress->priv; 
  BangoLayout *layout;
  BangoRectangle logical_rect;
  gint x, y, w, h, x_pos, y_pos, bar_position, bar_size, start, full_size;
  BdkRectangle clip;
  gboolean is_rtl;

  is_rtl = btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL;
  
  x = cell_area->x + cell->xpad;
  y = cell_area->y + cell->ypad;
  w = cell_area->width - cell->xpad * 2;
  h = cell_area->height - cell->ypad * 2;

  /* FIXME: BtkProgressBar draws the box with "trough" detail,
   * but some engines don't paint anything with that detail for
   * non-BtkProgressBar widgets.
   */
  btk_paint_box (widget->style,
		 window,
		 BTK_STATE_NORMAL, BTK_SHADOW_IN, 
		 NULL, widget, NULL,
		 x, y, w, h);

  if (priv->orientation == BTK_PROGRESS_LEFT_TO_RIGHT
      || priv->orientation == BTK_PROGRESS_RIGHT_TO_LEFT)
    {
      clip.y = y;
      clip.height = h;

      start = x;
      full_size = w;

      bar_size = get_bar_size (priv->pulse, priv->value, full_size);

      if (priv->orientation == BTK_PROGRESS_LEFT_TO_RIGHT)
	bar_position = get_bar_position (start, full_size, bar_size,
					 priv->pulse, priv->offset, is_rtl);
      else
	bar_position = get_bar_position (start, full_size, bar_size,
					 priv->pulse, priv->offset, !is_rtl);

      clip.width = bar_size;
      clip.x = bar_position;
    }
  else
    {
      clip.x = x;
      clip.width = w;

      start = y;
      full_size = h;

      bar_size = get_bar_size (priv->pulse, priv->value, full_size);

      if (priv->orientation == BTK_PROGRESS_BOTTOM_TO_TOP)
	bar_position = get_bar_position (start, full_size, bar_size,
					 priv->pulse, priv->offset, TRUE);
      else
	bar_position = get_bar_position (start, full_size, bar_size,
					 priv->pulse, priv->offset, FALSE);

      clip.height = bar_size;
      clip.y = bar_position;
    }

  btk_paint_box (widget->style,
		 window,
		 BTK_STATE_SELECTED, BTK_SHADOW_OUT,
		 &clip, widget, "bar",
		 clip.x, clip.y,
		 clip.width, clip.height);

  if (priv->label)
    {
      gfloat text_xalign;

      layout = btk_widget_create_bango_layout (widget, priv->label);
      bango_layout_get_pixel_extents (layout, NULL, &logical_rect);

      if (btk_widget_get_direction (widget) != BTK_TEXT_DIR_LTR)
	text_xalign = 1.0 - priv->text_xalign;
      else
	text_xalign = priv->text_xalign;

      x_pos = x + widget->style->xthickness + text_xalign *
	(w - 2 * widget->style->xthickness - logical_rect.width);

      y_pos = y + widget->style->ythickness + priv->text_yalign *
	(h - 2 * widget->style->ythickness - logical_rect.height);
  
      btk_paint_layout (widget->style, window, 
	  	        BTK_STATE_SELECTED,
		        FALSE, &clip, widget, "progressbar",
		        x_pos, y_pos, 
		        layout);

      if (bar_position > start)
        {
	  if (priv->orientation == BTK_PROGRESS_LEFT_TO_RIGHT
	      || priv->orientation == BTK_PROGRESS_RIGHT_TO_LEFT)
	    {
	      clip.x = x;
	      clip.width = bar_position - x;
	    }
	  else
	    {
	      clip.y = y;
	      clip.height = bar_position - y;
	    }

          btk_paint_layout (widget->style, window, 
	  	            BTK_STATE_NORMAL,
		            FALSE, &clip, widget, "progressbar",
		            x_pos, y_pos,
		            layout);
        }

      if (bar_position + bar_size < start + full_size)
        {
	  if (priv->orientation == BTK_PROGRESS_LEFT_TO_RIGHT
	      || priv->orientation == BTK_PROGRESS_RIGHT_TO_LEFT)
	    {
	      clip.x = bar_position + bar_size;
	      clip.width = x + w - (bar_position + bar_size);
	    }
	  else
	    {
	      clip.y = bar_position + bar_size;
	      clip.height = y + h - (bar_position + bar_size);
	    }

          btk_paint_layout (widget->style, window, 
		            BTK_STATE_NORMAL,
		            FALSE, &clip, widget, "progressbar",
		            x_pos, y_pos,
		            layout);
        }

      g_object_unref (layout);
    }
}

#define __BTK_CELL_RENDERER_PROGRESS_C__
#include "btkaliasdef.c"
