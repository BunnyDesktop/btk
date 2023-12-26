/* Drawing Area
 *
 * BtkDrawingArea is a blank area where you can draw custom displays
 * of various kinds.
 *
 * This demo has two drawing areas. The checkerboard area shows
 * how you can just draw something; all you have to do is write
 * a signal handler for expose_event, as shown here.
 *
 * The "scribble" area is a bit more advanced, and shows how to handle
 * events such as button presses and mouse motion. Click the mouse
 * and drag in the scribble area to draw squiggles. Resize the window
 * to clear the area.
 */

#include <btk/btk.h>

static BtkWidget *window = NULL;
/* Pixmap for scribble area, to store current scribbles */
static bairo_surface_t *surface = NULL;

/* Create a new surface of the appropriate size to store our scribbles */
static bboolean
scribble_configure_event (BtkWidget         *widget,
                          BdkEventConfigure *event,
                          bpointer           data)
{
  bairo_t *cr;
  BtkAllocation allocation;
  
  btk_widget_get_allocation (widget, &allocation);

  if (surface)
    bairo_surface_destroy (surface);

  surface = bdk_window_create_similar_surface (btk_widget_get_window (widget),
                                               BAIRO_CONTENT_COLOR,
                                               allocation.width,
                                               allocation.height);

  /* Initialize the surface to white */
  cr = bairo_create (surface);

  bairo_set_source_rgb (cr, 1, 1, 1);
  bairo_paint (cr);

  bairo_destroy (cr);

  /* We've handled the configure event, no need for further processing. */
  return TRUE;
}

/* Redraw the screen from the surface */
static bboolean
scribble_expose_event (BtkWidget      *widget,
                       BdkEventExpose *event,
                       bpointer        data)
{
  bairo_t *cr;

  cr = bdk_bairo_create (btk_widget_get_window (widget));
  
  bairo_set_source_surface (cr, surface, 0, 0);
  bdk_bairo_rectangle (cr, &event->area);
  bairo_fill (cr);

  bairo_destroy (cr);

  return FALSE;
}

/* Draw a rectangle on the screen */
static void
draw_brush (BtkWidget *widget,
            bdouble    x,
            bdouble    y)
{
  BdkRectangle update_rect;
  bairo_t *cr;

  update_rect.x = x - 3;
  update_rect.y = y - 3;
  update_rect.width = 6;
  update_rect.height = 6;

  /* Paint to the surface, where we store our state */
  cr = bairo_create (surface);

  bdk_bairo_rectangle (cr, &update_rect);
  bairo_fill (cr);

  bairo_destroy (cr);

  /* Now invalidate the affected rebunnyion of the drawing area. */
  bdk_window_invalidate_rect (btk_widget_get_window (widget),
                              &update_rect,
                              FALSE);
}

static bboolean
scribble_button_press_event (BtkWidget      *widget,
                             BdkEventButton *event,
                             bpointer        data)
{
  if (surface == NULL)
    return FALSE; /* paranoia check, in case we haven't gotten a configure event */

  if (event->button == 1)
    draw_brush (widget, event->x, event->y);

  /* We've handled the event, stop processing */
  return TRUE;
}

static bboolean
scribble_motion_notify_event (BtkWidget      *widget,
                              BdkEventMotion *event,
                              bpointer        data)
{
  int x, y;
  BdkModifierType state;

  if (surface == NULL)
    return FALSE; /* paranoia check, in case we haven't gotten a configure event */

  /* This call is very important; it requests the next motion event.
   * If you don't call bdk_window_get_pointer() you'll only get
   * a single motion event. The reason is that we specified
   * BDK_POINTER_MOTION_HINT_MASK to btk_widget_set_events().
   * If we hadn't specified that, we could just use event->x, event->y
   * as the pointer location. But we'd also get deluged in events.
   * By requesting the next event as we handle the current one,
   * we avoid getting a huge number of events faster than we
   * can cope.
   */

  bdk_window_get_pointer (event->window, &x, &y, &state);

  if (state & BDK_BUTTON1_MASK)
    draw_brush (widget, x, y);

  /* We've handled it, stop processing */
  return TRUE;
}


static bboolean
checkerboard_expose (BtkWidget      *da,
                     BdkEventExpose *event,
                     bpointer        data)
{
  bint i, j, xcount, ycount;
  bairo_t *cr;
  BtkAllocation allocation;
  
  btk_widget_get_allocation (da, &allocation);

#define CHECK_SIZE 10
#define SPACING 2

  /* At the start of an expose handler, a clip rebunnyion of event->area
   * is set on the window, and event->area has been cleared to the
   * widget's background color. The docs for
   * bdk_window_begin_paint_rebunnyion() give more details on how this
   * works.
   */

  cr = bdk_bairo_create (btk_widget_get_window (da));
  bdk_bairo_rectangle (cr, &event->area);
  bairo_clip (cr);

  xcount = 0;
  i = SPACING;
  while (i < allocation.width)
    {
      j = SPACING;
      ycount = xcount % 2; /* start with even/odd depending on row */
      while (j < allocation.height)
        {
          if (ycount % 2)
            bairo_set_source_rgb (cr, 0.45777, 0, 0.45777);
          else
            bairo_set_source_rgb (cr, 1, 1, 1);

          /* If we're outside event->area, this will do nothing.
           * It might be mildly more efficient if we handled
           * the clipping ourselves, but again we're feeling lazy.
           */
          bairo_rectangle (cr, i, j, CHECK_SIZE, CHECK_SIZE);
          bairo_fill (cr);

          j += CHECK_SIZE + SPACING;
          ++ycount;
        }

      i += CHECK_SIZE + SPACING;
      ++xcount;
    }

  bairo_destroy (cr);

  /* return TRUE because we've handled this event, so no
   * further processing is required.
   */
  return TRUE;
}

static void
close_window (void)
{
  window = NULL;

  if (surface)
    bairo_surface_destroy (surface);
  surface = NULL;
}

BtkWidget *
do_drawingarea (BtkWidget *do_widget)
{
  BtkWidget *frame;
  BtkWidget *vbox;
  BtkWidget *da;
  BtkWidget *label;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
                             btk_widget_get_screen (do_widget));
      btk_window_set_title (BTK_WINDOW (window), "Drawing Area");

      g_signal_connect (window, "destroy", G_CALLBACK (close_window), NULL);

      btk_container_set_border_width (BTK_CONTAINER (window), 8);

      vbox = btk_vbox_new (FALSE, 8);
      btk_container_set_border_width (BTK_CONTAINER (vbox), 8);
      btk_container_add (BTK_CONTAINER (window), vbox);

      /*
       * Create the checkerboard area
       */

      label = btk_label_new (NULL);
      btk_label_set_markup (BTK_LABEL (label),
                            "<u>Checkerboard pattern</u>");
      btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);

      frame = btk_frame_new (NULL);
      btk_frame_set_shadow_type (BTK_FRAME (frame), BTK_SHADOW_IN);
      btk_box_pack_start (BTK_BOX (vbox), frame, TRUE, TRUE, 0);

      da = btk_drawing_area_new ();
      /* set a minimum size */
      btk_widget_set_size_request (da, 100, 100);

      btk_container_add (BTK_CONTAINER (frame), da);

      g_signal_connect (da, "expose-event",
                        G_CALLBACK (checkerboard_expose), NULL);

      /*
       * Create the scribble area
       */

      label = btk_label_new (NULL);
      btk_label_set_markup (BTK_LABEL (label),
                            "<u>Scribble area</u>");
      btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);

      frame = btk_frame_new (NULL);
      btk_frame_set_shadow_type (BTK_FRAME (frame), BTK_SHADOW_IN);
      btk_box_pack_start (BTK_BOX (vbox), frame, TRUE, TRUE, 0);

      da = btk_drawing_area_new ();
      /* set a minimum size */
      btk_widget_set_size_request (da, 100, 100);

      btk_container_add (BTK_CONTAINER (frame), da);

      /* Signals used to handle backing surface */

      g_signal_connect (da, "expose-event",
                        G_CALLBACK (scribble_expose_event), NULL);
      g_signal_connect (da,"configure-event",
                        G_CALLBACK (scribble_configure_event), NULL);

      /* Event signals */

      g_signal_connect (da, "motion-notify-event",
                        G_CALLBACK (scribble_motion_notify_event), NULL);
      g_signal_connect (da, "button-press-event",
                        G_CALLBACK (scribble_button_press_event), NULL);


      /* Ask to receive events the drawing area doesn't normally
       * subscribe to
       */
      btk_widget_set_events (da, btk_widget_get_events (da)
                             | BDK_LEAVE_NOTIFY_MASK
                             | BDK_BUTTON_PRESS_MASK
                             | BDK_POINTER_MOTION_MASK
                             | BDK_POINTER_MOTION_HINT_MASK);

    }

  if (!btk_widget_get_visible (window))
      btk_widget_show_all (window);
  else
      btk_widget_destroy (window);

  return window;
}
