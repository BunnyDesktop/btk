
#include <btk/btk.h>

/* Our callback.
 * The data passed to this function is printed to stdout */
static void callback( BtkWidget *widget,
                      bpointer   data )
{
    g_print ("Hello again - %s was pressed\n", (char *) data);
}

/* This callback quits the program */
static bboolean delete_event( BtkWidget *widget,
                              BdkEvent  *event,
                              bpointer   data )
{
    btk_main_quit ();
    return FALSE;
}

int main( int   argc,
          char *argv[] )
{
    BtkWidget *window;
    BtkWidget *button;
    BtkWidget *table;

    btk_init (&argc, &argv);

    /* Create a new window */
    window = btk_window_new (BTK_WINDOW_TOPLEVEL);

    /* Set the window title */
    btk_window_set_title (BTK_WINDOW (window), "Table");

    /* Set a handler for delete_event that immediately
     * exits BTK. */
    g_signal_connect (window, "delete-event",
                      G_CALLBACK (delete_event), NULL);

    /* Sets the border width of the window. */
    btk_container_set_border_width (BTK_CONTAINER (window), 20);

    /* Create a 2x2 table */
    table = btk_table_new (2, 2, TRUE);

    /* Put the table in the main window */
    btk_container_add (BTK_CONTAINER (window), table);

    /* Create first button */
    button = btk_button_new_with_label ("button 1");

    /* When the button is clicked, we call the "callback" function
     * with a pointer to "button 1" as its argument */
    g_signal_connect (button, "clicked",
	              G_CALLBACK (callback), (bpointer) "button 1");


    /* Insert button 1 into the upper left quadrant of the table */
    btk_table_attach_defaults (BTK_TABLE (table), button, 0, 1, 0, 1);

    btk_widget_show (button);

    /* Create second button */

    button = btk_button_new_with_label ("button 2");

    /* When the button is clicked, we call the "callback" function
     * with a pointer to "button 2" as its argument */
    g_signal_connect (button, "clicked",
                      G_CALLBACK (callback), (bpointer) "button 2");
    /* Insert button 2 into the upper right quadrant of the table */
    btk_table_attach_defaults (BTK_TABLE (table), button, 1, 2, 0, 1);

    btk_widget_show (button);

    /* Create "Quit" button */
    button = btk_button_new_with_label ("Quit");

    /* When the button is clicked, we call the "delete_event" function
     * and the program exits */
    g_signal_connect (button, "clicked",
                      G_CALLBACK (delete_event), NULL);

    /* Insert the quit button into the both
     * lower quadrants of the table */
    btk_table_attach_defaults (BTK_TABLE (table), button, 0, 2, 1, 2);

    btk_widget_show (button);

    btk_widget_show (table);
    btk_widget_show (window);

    btk_main ();

    return 0;
}
