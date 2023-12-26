#include <btk/btk.h>

static bboolean
da_expose (BtkWidget *widget,
           BdkEventExpose *event,
           bpointer user_data)
{
  BtkOffscreenWindow *offscreen = (BtkOffscreenWindow *)user_data;
  BdkPixmap *pixmap;
  bairo_t *cr;

  if (btk_widget_is_drawable (widget))
    {
      pixmap = btk_offscreen_window_get_pixmap (offscreen);

      cr = bdk_bairo_create (widget->window);
      bdk_bairo_set_source_pixmap (cr, pixmap, 50, 50);
      bairo_paint (cr);
      bairo_destroy (cr);
    }

  return FALSE;
}

static bboolean
offscreen_damage (BtkWidget      *widget,
                  BdkEventExpose *event,
                  BtkWidget      *da)
{
  btk_widget_queue_draw (da);

  return TRUE;
}

static bboolean
da_button_press (BtkWidget *area, BdkEventButton *event, BtkWidget *button)
{
  btk_widget_set_size_request (button, 150, 60);
  return TRUE;
}

int
main (int argc, char **argv)
{
  BtkWidget *window;
  BtkWidget *button;
  BtkWidget *offscreen;
  BtkWidget *da;

  btk_init (&argc, &argv);

  offscreen = btk_offscreen_window_new ();

  button = btk_button_new_with_label ("Test");
  btk_widget_set_size_request (button, 50, 50);
  btk_container_add (BTK_CONTAINER (offscreen), button);
  btk_widget_show (button);

  btk_widget_show (offscreen);

  /* Queue exposures and ensure they are handled so
   * that the result is uptodate for the first
   * expose of the window. If you want to get further
   * changes, also track damage on the offscreen
   * as done above.
   */
  btk_widget_queue_draw (offscreen);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "delete-event",
                    G_CALLBACK (btk_main_quit), window);
  da = btk_drawing_area_new ();
  btk_container_add (BTK_CONTAINER (window), da);

  g_signal_connect (da,
                    "expose-event",
                    G_CALLBACK (da_expose),
                    offscreen);

  g_signal_connect (offscreen,
                    "damage-event",
                    G_CALLBACK (offscreen_damage),
                    da);

  btk_widget_add_events (da, BDK_BUTTON_PRESS_MASK);
  g_signal_connect (da, "button_press_event", G_CALLBACK (da_button_press),
                    button);

  btk_widget_show_all (window);

  btk_main();

  return 0;
}
