
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

#include <btk/btk.h>

/* Backing pixmap for drawing area */
static BdkPixmap *pixmap = NULL;

/* Create a new backing pixmap of the appropriate size */
static bboolean
configure_event (BtkWidget *widget, BdkEventConfigure *event)
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
static bboolean
expose_event (BtkWidget *widget, BdkEventExpose *event)
{
  bdk_draw_drawable (widget->window,
                     widget->style->fg_gc[btk_widget_get_state (widget)],
                     pixmap,
                     event->area.x, event->area.y,
                     event->area.x, event->area.y,
                     event->area.width, event->area.height);

  return FALSE;
}

/* Draw a rectangle on the screen, size depending on pressure,
   and color on the type of device */
static void
draw_brush (BtkWidget *widget, BdkInputSource source,
            bdouble x, bdouble y, bdouble pressure)
{
  BdkGC *gc;
  BdkRectangle update_rect;

  switch (source)
    {
    case BDK_SOURCE_MOUSE:
      gc = widget->style->dark_gc[btk_widget_get_state (widget)];
      break;
    case BDK_SOURCE_PEN:
      gc = widget->style->black_gc;
      break;
    case BDK_SOURCE_ERASER:
      gc = widget->style->white_gc;
      break;
    default:
      gc = widget->style->light_gc[btk_widget_get_state (widget)];
    }

  update_rect.x = x - 10 * pressure;
  update_rect.y = y - 10 * pressure;
  update_rect.width = 20 * pressure;
  update_rect.height = 20 * pressure;
  bdk_draw_rectangle (pixmap, gc, TRUE,
                      update_rect.x, update_rect.y,
                      update_rect.width, update_rect.height);
  btk_widget_queue_draw_area (widget, 
                      update_rect.x, update_rect.y,
                      update_rect.width, update_rect.height);
}

static void
print_button_press (BdkDevice *device)
{
  g_print ("Button press on device '%s'\n", device->name);
}

static bboolean
button_press_event (BtkWidget *widget, BdkEventButton *event)
{
  print_button_press (event->device);
  
  if (event->button == 1 && pixmap != NULL) {
    bdouble pressure;
    bdk_event_get_axis ((BdkEvent *)event, BDK_AXIS_PRESSURE, &pressure);
    draw_brush (widget, event->device->source, event->x, event->y, pressure);
  }

  return TRUE;
}

static bboolean
motion_notify_event (BtkWidget *widget, BdkEventMotion *event)
{
  bdouble x, y;
  bdouble pressure;
  BdkModifierType state;

  if (event->is_hint) 
    {
      bdk_device_get_state (event->device, event->window, NULL, &state);
      bdk_event_get_axis ((BdkEvent *)event, BDK_AXIS_X, &x);
      bdk_event_get_axis ((BdkEvent *)event, BDK_AXIS_Y, &y);
      bdk_event_get_axis ((BdkEvent *)event, BDK_AXIS_PRESSURE, &pressure);
    }
  else
    {
      x = event->x;
      y = event->y;
      bdk_event_get_axis ((BdkEvent *)event, BDK_AXIS_PRESSURE, &pressure);
      state = event->state;
    }
    
  if (state & BDK_BUTTON1_MASK && pixmap != NULL)
    draw_brush (widget, event->device->source, x, y, pressure);
  
  return TRUE;
}

void
input_dialog_destroy (BtkWidget *w, bpointer data)
{
  *((BtkWidget **)data) = NULL;
}

void
create_input_dialog ()
{
  static BtkWidget *inputd = NULL;

  if (!inputd)
    {
      inputd = btk_input_dialog_new();

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
      if (!btk_widget_get_mapped (inputd))
        btk_widget_show (inputd);
      else
        bdk_window_raise (inputd->window);
    }
}

int
main (int argc, char *argv[])
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
                    G_CALLBACK (btk_main_quit), NULL);

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

  /* The following call enables tracking and processing of extension
     events for the drawing area */
  btk_widget_set_extension_events (drawing_area, BDK_EXTENSION_EVENTS_CURSOR);

  /* .. And some buttons */
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
