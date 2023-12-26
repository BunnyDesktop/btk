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

#undef BTK_DISABLE_DEPRECATED

#include "config.h"
#include <stdio.h>
#include "btk/btk.h"

/* Backing pixmap for drawing area */

static BdkPixmap *pixmap = NULL;

/* Information about cursor */

static bint cursor_proximity = TRUE;
static bdouble cursor_x;
static bdouble cursor_y;

/* Unique ID of current device */
static BdkDevice *current_device;

/* Erase the old cursor, and/or draw a new one, if necessary */
static void
update_cursor (BtkWidget *widget,  bdouble x, bdouble y)
{
  static bint cursor_present = 0;
  bint state = !current_device->has_cursor && cursor_proximity;

  if (pixmap != NULL)
    {
      bairo_t *cr = bdk_bairo_create (widget->window);

      if (cursor_present && (cursor_present != state ||
			     x != cursor_x || y != cursor_y))
	{
          bdk_bairo_set_source_pixmap (cr, pixmap, 0, 0);
          bairo_rectangle (cr, cursor_x - 5, cursor_y - 5, 10, 10);
          bairo_fill (cr);
	}

      cursor_present = state;
      cursor_x = x;
      cursor_y = y;

      if (cursor_present)
	{
          bairo_set_source_rgb (cr, 0, 0, 0);
          bairo_rectangle (cr, 
                           cursor_x - 5, cursor_y -5,
			   10, 10);
          bairo_fill (cr);
	}

      bairo_destroy (cr);
    }
}

/* Create a new backing pixmap of the appropriate size */
static bint
configure_event (BtkWidget *widget, BdkEventConfigure *event)
{
  bairo_t *cr;

  if (pixmap)
    g_object_unref (pixmap);
  pixmap = bdk_pixmap_new(widget->window,
			  widget->allocation.width,
			  widget->allocation.height,
			  -1);
  cr = bdk_bairo_create (pixmap);

  bairo_set_source_rgb (cr, 1, 1, 1);
  bairo_paint (cr);

  bairo_destroy (cr);

  return TRUE;
}

/* Refill the screen from the backing pixmap */
static bint
expose_event (BtkWidget *widget, BdkEventExpose *event)
{
  bairo_t *cr = bdk_bairo_create (widget->window);

  bdk_bairo_set_source_pixmap (cr, pixmap, 0, 0);
  bdk_bairo_rectangle (cr, &event->area);
  bairo_fill (cr);

  bairo_destroy (cr);

  return FALSE;
}

/* Draw a rectangle on the screen, size depending on pressure,
   and color on the type of device */
static void
draw_brush (BtkWidget *widget, BdkInputSource source,
	    bdouble x, bdouble y, bdouble pressure)
{
  BdkColor color;
  BdkRectangle update_rect;
  bairo_t *cr;

  switch (source)
    {
    case BDK_SOURCE_MOUSE:
      color = widget->style->dark[btk_widget_get_state (widget)];
      break;
    case BDK_SOURCE_PEN:
      color.red = color.green = color.blue = 0;
      break;
    case BDK_SOURCE_ERASER:
      color.red = color.green = color.blue = 65535;
      break;
    default:
      color = widget->style->light[btk_widget_get_state (widget)];
    }

  update_rect.x = x - 10 * pressure;
  update_rect.y = y - 10 * pressure;
  update_rect.width = 20 * pressure;
  update_rect.height = 20 * pressure;

  cr = bdk_bairo_create (pixmap);
  bdk_bairo_set_source_color (cr, &color);
  bdk_bairo_rectangle (cr, &update_rect);
  bairo_fill (cr);
  bairo_destroy (cr);

  btk_widget_queue_draw_area (widget,
			      update_rect.x, update_rect.y,
			      update_rect.width, update_rect.height);
  bdk_window_process_updates (widget->window, TRUE);
}

static buint32 motion_time;

static void
print_axes (BdkDevice *device, bdouble *axes)
{
  int i;
  
  if (axes)
    {
      g_print ("%s ", device->name);
      
      for (i=0; i<device->num_axes; i++)
	g_print ("%g ", axes[i]);

      g_print ("\n");
    }
}

static bint
button_press_event (BtkWidget *widget, BdkEventButton *event)
{
  current_device = event->device;
  cursor_proximity = TRUE;

  if (event->button == 1 && pixmap != NULL)
    {
      bdouble pressure = 0.5;

      print_axes (event->device, event->axes);
      bdk_event_get_axis ((BdkEvent *)event, BDK_AXIS_PRESSURE, &pressure);
      draw_brush (widget, event->device->source, event->x, event->y, pressure);
      
      motion_time = event->time;
    }

  update_cursor (widget, event->x, event->y);

  return TRUE;
}

static bint
key_press_event (BtkWidget *widget, BdkEventKey *event)
{
  if ((event->keyval >= 0x20) && (event->keyval <= 0xFF))
    printf("I got a %c\n", event->keyval);
  else
    printf("I got some other key\n");

  return TRUE;
}

static bint
motion_notify_event (BtkWidget *widget, BdkEventMotion *event)
{
  BdkTimeCoord **events;
  int n_events;
  int i;

  current_device = event->device;
  cursor_proximity = TRUE;

  if (event->state & BDK_BUTTON1_MASK && pixmap != NULL)
    {
      if (bdk_device_get_history (event->device, event->window, 
				  motion_time, event->time,
				  &events, &n_events))
	{
	  for (i=0; i<n_events; i++)
	    {
	      double x = 0, y = 0, pressure = 0.5;

	      bdk_device_get_axis (event->device, events[i]->axes, BDK_AXIS_X, &x);
	      bdk_device_get_axis (event->device, events[i]->axes, BDK_AXIS_Y, &y);
	      bdk_device_get_axis (event->device, events[i]->axes, BDK_AXIS_PRESSURE, &pressure);
	      draw_brush (widget,  event->device->source, x, y, pressure);

	      print_axes (event->device, events[i]->axes);
	    }
	  bdk_device_free_history (events, n_events);
	}
      else
	{
	  double pressure = 0.5;

	  bdk_event_get_axis ((BdkEvent *)event, BDK_AXIS_PRESSURE, &pressure);

	  draw_brush (widget,  event->device->source, event->x, event->y, pressure);
	}
      motion_time = event->time;
    }

  if (event->is_hint)
    bdk_device_get_state (event->device, event->window, NULL, NULL);

  print_axes (event->device, event->axes);
  update_cursor (widget, event->x, event->y);

  return TRUE;
}

/* We track the next two events to know when we need to draw a
   cursor */

static bint
proximity_out_event (BtkWidget *widget, BdkEventProximity *event)
{
  cursor_proximity = FALSE;
  update_cursor (widget, cursor_x, cursor_y);
  return TRUE;
}

static bint
leave_notify_event (BtkWidget *widget, BdkEventCrossing *event)
{
  cursor_proximity = FALSE;
  update_cursor (widget, cursor_x, cursor_y);
  return TRUE;
}

void
input_dialog_destroy (BtkWidget *w, bpointer data)
{
  *((BtkWidget **)data) = NULL;
}

void
create_input_dialog (void)
{
  static BtkWidget *inputd = NULL;

  if (!inputd)
    {
      inputd = btk_input_dialog_new ();

      g_signal_connect (inputd, "destroy",
			G_CALLBACK (input_dialog_destroy), &inputd);
      g_signal_connect_swapped (BTK_INPUT_DIALOG (inputd)->close_button,
			        "clicked",
			        G_CALLBACK (btk_widget_hide),
			        inputd);
      btk_widget_hide (BTK_INPUT_DIALOG (inputd)->save_button);

      btk_widget_show (inputd);
    }
  else
    {
      if (!btk_widget_get_mapped(inputd))
	btk_widget_show(inputd);
      else
	bdk_window_raise(inputd->window);
    }
}

void
quit (void)
{
  btk_main_quit ();
}

int
main (int argc, char *argv[])
{
  BtkWidget *window;
  BtkWidget *drawing_area;
  BtkWidget *vbox;

  BtkWidget *button;

  btk_init (&argc, &argv);

  current_device = bdk_device_get_core_pointer ();

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_widget_set_name (window, "Test Input");

  vbox = btk_vbox_new (FALSE, 0);
  btk_container_add (BTK_CONTAINER (window), vbox);
  btk_widget_show (vbox);

  g_signal_connect (window, "destroy",
		    G_CALLBACK (quit), NULL);

  /* Create the drawing area */

  drawing_area = btk_drawing_area_new ();
  btk_widget_set_size_request (drawing_area, 200, 200);
  btk_box_pack_start (BTK_BOX (vbox), drawing_area, TRUE, TRUE, 0);

  btk_widget_show (drawing_area);

  /* Signals used to handle backing pixmap */

  g_signal_connect (drawing_area, "expose_event",
		    G_CALLBACK (expose_event), NULL);
  g_signal_connect (drawing_area, "configure_event",
		    G_CALLBACK (configure_event), NULL);

  /* Event signals */

  g_signal_connect (drawing_area, "motion_notify_event",
		    G_CALLBACK (motion_notify_event), NULL);
  g_signal_connect (drawing_area, "button_press_event",
		    G_CALLBACK (button_press_event), NULL);
  g_signal_connect (drawing_area, "key_press_event",
		    G_CALLBACK (key_press_event), NULL);

  g_signal_connect (drawing_area, "leave_notify_event",
		    G_CALLBACK (leave_notify_event), NULL);
  g_signal_connect (drawing_area, "proximity_out_event",
		    G_CALLBACK (proximity_out_event), NULL);

  btk_widget_set_events (drawing_area, BDK_EXPOSURE_MASK
			 | BDK_LEAVE_NOTIFY_MASK
			 | BDK_BUTTON_PRESS_MASK
			 | BDK_KEY_PRESS_MASK
			 | BDK_POINTER_MOTION_MASK
			 | BDK_POINTER_MOTION_HINT_MASK
			 | BDK_PROXIMITY_OUT_MASK);

  /* The following call enables tracking and processing of extension
     events for the drawing area */
  btk_widget_set_extension_events (drawing_area, BDK_EXTENSION_EVENTS_ALL);

  btk_widget_set_can_focus (drawing_area, TRUE);
  btk_widget_grab_focus (drawing_area);

  /* .. And create some buttons */
  button = btk_button_new_with_label ("Input Dialog");
  btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  g_signal_connect (button, "clicked",
		    G_CALLBACK (create_input_dialog), NULL);
  btk_widget_show (button);

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
