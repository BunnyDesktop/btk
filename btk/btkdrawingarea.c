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
#include "btkdrawingarea.h"
#include "btkintl.h"
#include "btkalias.h"


static void btk_drawing_area_realize       (BtkWidget           *widget);
static void btk_drawing_area_size_allocate (BtkWidget           *widget,
					    BtkAllocation       *allocation);
static void btk_drawing_area_send_configure (BtkDrawingArea     *darea);

G_DEFINE_TYPE (BtkDrawingArea, btk_drawing_area, BTK_TYPE_WIDGET)

static void
btk_drawing_area_class_init (BtkDrawingAreaClass *class)
{
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (class);

  widget_class->realize = btk_drawing_area_realize;
  widget_class->size_allocate = btk_drawing_area_size_allocate;
}

static void
btk_drawing_area_init (BtkDrawingArea *darea)
{
  darea->draw_data = NULL;
}


BtkWidget*
btk_drawing_area_new (void)
{
  return g_object_new (BTK_TYPE_DRAWING_AREA, NULL);
}

void
btk_drawing_area_size (BtkDrawingArea *darea,
		       gint            width,
		       gint            height)
{
  g_return_if_fail (BTK_IS_DRAWING_AREA (darea));

  BTK_WIDGET (darea)->requisition.width = width;
  BTK_WIDGET (darea)->requisition.height = height;

  btk_widget_queue_resize (BTK_WIDGET (darea));
}

static void
btk_drawing_area_realize (BtkWidget *widget)
{
  BtkDrawingArea *darea = BTK_DRAWING_AREA (widget);
  BdkWindowAttr attributes;
  gint attributes_mask;

  if (!btk_widget_get_has_window (widget))
    {
      BTK_WIDGET_CLASS (btk_drawing_area_parent_class)->realize (widget);
    }
  else
    {
      btk_widget_set_realized (widget, TRUE);

      attributes.window_type = BDK_WINDOW_CHILD;
      attributes.x = widget->allocation.x;
      attributes.y = widget->allocation.y;
      attributes.width = widget->allocation.width;
      attributes.height = widget->allocation.height;
      attributes.wclass = BDK_INPUT_OUTPUT;
      attributes.visual = btk_widget_get_visual (widget);
      attributes.colormap = btk_widget_get_colormap (widget);
      attributes.event_mask = btk_widget_get_events (widget) | BDK_EXPOSURE_MASK;

      attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;

      widget->window = bdk_window_new (btk_widget_get_parent_window (widget),
                                       &attributes, attributes_mask);
      bdk_window_set_user_data (widget->window, darea);

      widget->style = btk_style_attach (widget->style, widget->window);
      btk_style_set_background (widget->style, widget->window, BTK_STATE_NORMAL);
    }

  btk_drawing_area_send_configure (BTK_DRAWING_AREA (widget));
}

static void
btk_drawing_area_size_allocate (BtkWidget     *widget,
				BtkAllocation *allocation)
{
  g_return_if_fail (BTK_IS_DRAWING_AREA (widget));
  g_return_if_fail (allocation != NULL);

  widget->allocation = *allocation;

  if (btk_widget_get_realized (widget))
    {
      if (btk_widget_get_has_window (widget))
        bdk_window_move_resize (widget->window,
                                allocation->x, allocation->y,
                                allocation->width, allocation->height);

      btk_drawing_area_send_configure (BTK_DRAWING_AREA (widget));
    }
}

static void
btk_drawing_area_send_configure (BtkDrawingArea *darea)
{
  BtkWidget *widget;
  BdkEvent *event = bdk_event_new (BDK_CONFIGURE);

  widget = BTK_WIDGET (darea);

  event->configure.window = g_object_ref (widget->window);
  event->configure.send_event = TRUE;
  event->configure.x = widget->allocation.x;
  event->configure.y = widget->allocation.y;
  event->configure.width = widget->allocation.width;
  event->configure.height = widget->allocation.height;
  
  btk_widget_event (widget, event);
  bdk_event_free (event);
}

#define __BTK_DRAWING_AREA_C__
#include "btkaliasdef.c"
