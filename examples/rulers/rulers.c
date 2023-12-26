
#include <btk/btk.h>

#define EVENT_METHOD(i, x) BTK_WIDGET_GET_CLASS(i)->x

#define XSIZE  600
#define YSIZE  400

/* This routine gets control when the close button is clicked */
static bboolean close_application( BtkWidget *widget,
                                   BdkEvent  *event,
                                   bpointer   data )
{
    btk_main_quit ();
    return FALSE;
}

/* The main routine */
int main( int   argc,
          char *argv[] ) {
    BtkWidget *window, *table, *area, *hrule, *vrule;

    /* Initialize BTK and create the main window */
    btk_init (&argc, &argv);

    window = btk_window_new (BTK_WINDOW_TOPLEVEL);
    g_signal_connect (window, "delete-event",
                      G_CALLBACK (close_application), NULL);
    btk_container_set_border_width (BTK_CONTAINER (window), 10);

    /* Create a table for placing the ruler and the drawing area */
    table = btk_table_new (3, 2, FALSE);
    btk_container_add (BTK_CONTAINER (window), table);

    area = btk_drawing_area_new ();
    btk_widget_set_size_request (BTK_WIDGET (area), XSIZE, YSIZE);
    btk_table_attach (BTK_TABLE (table), area, 1, 2, 1, 2,
                      BTK_EXPAND|BTK_FILL, BTK_FILL, 0, 0);
    btk_widget_set_events (area, BDK_POINTER_MOTION_MASK |
                                 BDK_POINTER_MOTION_HINT_MASK);

    /* The horizontal ruler goes on top. As the mouse moves across the
     * drawing area, a motion_notify_event is passed to the
     * appropriate event handler for the ruler. */
    hrule = btk_hruler_new ();
    btk_ruler_set_metric (BTK_RULER (hrule), BTK_PIXELS);
    btk_ruler_set_range (BTK_RULER (hrule), 7, 13, 0, 20);
    g_signal_connect_swapped (area, "motion-notify-event",
                              G_CALLBACK (EVENT_METHOD (hrule,
                                                        motion_notify_event)),
                              hrule);
    btk_table_attach (BTK_TABLE (table), hrule, 1, 2, 0, 1,
                      BTK_EXPAND|BTK_SHRINK|BTK_FILL, BTK_FILL, 0, 0);

    /* The vertical ruler goes on the left. As the mouse moves across
     * the drawing area, a motion_notify_event is passed to the
     * appropriate event handler for the ruler. */
    vrule = btk_vruler_new ();
    btk_ruler_set_metric (BTK_RULER (vrule), BTK_PIXELS);
    btk_ruler_set_range (BTK_RULER (vrule), 0, YSIZE, 10, YSIZE );
    g_signal_connect_swapped (area, "motion-notify-event",
                              G_CALLBACK (EVENT_METHOD (vrule,
                                                        motion_notify_event)),
                              vrule);
    btk_table_attach (BTK_TABLE (table), vrule, 0, 1, 1, 2,
                      BTK_FILL, BTK_EXPAND|BTK_SHRINK|BTK_FILL, 0, 0);

    /* Now show everything */
    btk_widget_show (area);
    btk_widget_show (hrule);
    btk_widget_show (vrule);
    btk_widget_show (table);
    btk_widget_show (window);
    btk_main ();

    return 0;
}
