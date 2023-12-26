
#include <stdlib.h>
#include <btk/btk.h>

int main( int argc,
          char *argv[] )
{
    BtkWidget *window;
    BtkWidget *event_box;
    BtkWidget *label;

    btk_init (&argc, &argv);

    window = btk_window_new (BTK_WINDOW_TOPLEVEL);

    btk_window_set_title (BTK_WINDOW (window), "Event Box");

    g_signal_connect (window, "destroy",
	              G_CALLBACK (exit), NULL);

    btk_container_set_border_width (BTK_CONTAINER (window), 10);

    /* Create an EventBox and add it to our toplevel window */

    event_box = btk_event_box_new ();
    btk_container_add (BTK_CONTAINER (window), event_box);
    btk_widget_show (event_box);

    /* Create a long label */

    label = btk_label_new ("Click here to quit, quit, quit, quit, quit");
    btk_container_add (BTK_CONTAINER (event_box), label);
    btk_widget_show (label);

    /* Clip it short. */
    btk_widget_set_size_request (label, 110, 20);

    /* And bind an action to it */
    btk_widget_set_events (event_box, BDK_BUTTON_PRESS_MASK);
    g_signal_connect (event_box, "button-press-event",
	              G_CALLBACK (exit), NULL);

    /* Yet one more thing you need an X window for ... */

    btk_widget_realize (event_box);
    bdk_window_set_cursor (event_box->window, bdk_cursor_new (BDK_HAND1));

    btk_widget_show (window);

    btk_main ();

    return 0;
}
