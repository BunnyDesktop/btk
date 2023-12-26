
/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

#include <stdlib.h>
#include <btk/btk.h>

/* Backing pixmap for drawing area */
static BdkPixmap *pixmap = NULL;

/* Create a new backing pixmap of the appropriate size */
static gboolean configure_event( BtkWidget         *widget,
                                 BdkEventConfigure *event )
{
  if (pixmap)
    g_object_unref (pixmap);

  pixmap = bdk_pixmap_new (widget->window,
			   widget->allocation.width,
			   widget->allocation.height,
			   -1);
  bdk_draw_rectangle (pixmap,
		      widget->style->white_gc,
		      TRUE,
		      0, 0,
		      widget->allocation.width,
		      widget->allocation.height);

  return TRUE;
}

/* Redraw the screen from the backing pixmap */
static gboolean expose_event( BtkWidget      *widget,
                              BdkEventExpose *event )
{
  bdk_draw_drawable (widget->window,
		     widget->style->fg_gc[btk_widget_get_state (widget)],
		     pixmap,
		     event->area.x, event->area.y,
		     event->area.x, event->area.y,
		     event->area.width, event->area.height);

  return FALSE;
}

/* Draw a rectangle on the screen */
static void draw_brush( BtkWidget *widget,
                        gdouble    x,
                        gdouble    y)
{
  BdkRectangle update_rect;

  update_rect.x = x - 5;
  update_rect.y = y - 5;
  update_rect.width = 10;
  update_rect.height = 10;
  bdk_draw_rectangle (pixmap,
		      widget->style->black_gc,
		      TRUE,
		      update_rect.x, update_rect.y,
		      update_rect.width, update_rect.height);
  btk_widget_queue_draw_area (widget, 
		              update_rect.x, update_rect.y,
		              update_rect.width, update_rect.height);
}

static gboolean button_press_event( BtkWidget      *widget,
                                    BdkEventButton *event )
{
  if (event->button == 1 && pixmap != NULL)
    draw_brush (widget, event->x, event->y);

  return TRUE;
}

static gboolean motion_notify_event( BtkWidget *widget,
                                     BdkEventMotion *event )
{
  int x, y;
  BdkModifierType state;

  if (event->is_hint)
    bdk_window_get_pointer (event->window, &x, &y, &state);
  else
    {
      x = event->x;
      y = event->y;
      state = event->state;
    }
    
  if (state & BDK_BUTTON1_MASK && pixmap != NULL)
    draw_brush (widget, x, y);
  
  return TRUE;
}

void quit ()
{
  exit (0);
}

int main( int   argc, 
          char *argv[] )
{
  BtkWidget *window;
  BtkWidget *drawing_area;
  BtkWidget *vbox;

  BtkWidget *button;

  btk_init (&argc, &argv);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_widget_set_name (window, "Test Input");

  vbox = btk_vbox_new (FALSE, 0);
  btk_container_add (BTK_CONTAINER (window), vbox);
  btk_widget_show (vbox);

  g_signal_connect (window, "destroy",
                    G_CALLBACK (quit), NULL);

  /* Create the drawing area */

  drawing_area = btk_drawing_area_new ();
  btk_widget_set_size_request (BTK_WIDGET (drawing_area), 200, 200);
  btk_box_pack_start (BTK_BOX (vbox), drawing_area, TRUE, TRUE, 0);

  btk_widget_show (drawing_area);

  /* Signals used to handle backing pixmap */

  g_signal_connect (drawing_area, "expose-event",
		    G_CALLBACK (expose_event), NULL);
  g_signal_connect (drawing_area,"configure-event",
		    G_CALLBACK (configure_event), NULL);

  /* Event signals */

  g_signal_connect (drawing_area, "motion-notify-event",
		    G_CALLBACK (motion_notify_event), NULL);
  g_signal_connect (drawing_area, "button-press-event",
		    G_CALLBACK (button_press_event), NULL);

  btk_widget_set_events (drawing_area, BDK_EXPOSURE_MASK
			 | BDK_LEAVE_NOTIFY_MASK
			 | BDK_BUTTON_PRESS_MASK
			 | BDK_POINTER_MOTION_MASK
			 | BDK_POINTER_MOTION_HINT_MASK);

  /* .. And a quit button */
  button = btk_button_new_with_label ("Quit");
  btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  g_signal_connect_swapped (button, "clicked",
			    G_CALLBACK (btk_widget_destroy),
			    window);
  btk_widget_show (button);

  btk_widget_show (window);

  btk_main ();

  return 0;
}
