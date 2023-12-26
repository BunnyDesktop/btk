
#include <btk/btk.h>

/* Our new improved callback.  The data passed to this function
 * is printed to stdout. */
static void callback (BtkWidget *widget,
                      gpointer   data)
{
    g_print ("Hello again - %s was pressed\n", (gchar *) data);
}

/* another callback */
static gboolean delete_event (BtkWidget *widget,
                              BdkEvent  *event,
                              gpointer   data)
{
    btk_main_quit ();
    return FALSE;
}

int main (int   argc,
          char *argv[])
{
    /* BtkWidget is the storage type for widgets */
    BtkWidget *window;
    BtkWidget *button;
    BtkWidget *box1;

    /* This is called in all BTK applications. Arguments are parsed
     * from the command line and are returned to the application. */
    btk_init (&argc, &argv);

    /* Create a new window */
    window = btk_window_new (BTK_WINDOW_TOPLEVEL);

    /* This is a new call, which just sets the title of our
     * new window to "Hello Buttons!" */
    btk_window_set_title (BTK_WINDOW (window), "Hello Buttons!");

    /* Here we just set a handler for delete_event that immediately
     * exits BTK. */
    g_signal_connect (window, "delete-event",
		      G_CALLBACK (delete_event), NULL);

    /* Sets the border width of the window. */
    btk_container_set_border_width (BTK_CONTAINER (window), 10);

    /* We create a box to pack widgets into.  This is described in detail
     * in the "packing" section. The box is not really visible, it
     * is just used as a tool to arrange widgets. */
    box1 = btk_hbox_new (FALSE, 0);

    /* Put the box into the main window. */
    btk_container_add (BTK_CONTAINER (window), box1);

    /* Creates a new button with the label "Button 1". */
    button = btk_button_new_with_label ("Button 1");

    /* Now when the button is clicked, we call the "callback" function
     * with a pointer to "button 1" as its argument */
    g_signal_connect (button, "clicked",
		      G_CALLBACK (callback), "button 1");

    /* Instead of btk_container_add, we pack this button into the invisible
     * box, which has been packed into the window. */
    btk_box_pack_start (BTK_BOX (box1), button, TRUE, TRUE, 0);

    /* Always remember this step, this tells BTK that our preparation for
     * this button is complete, and it can now be displayed. */
    btk_widget_show (button);

    /* Do these same steps again to create a second button */
    button = btk_button_new_with_label ("Button 2");

    /* Call the same callback function with a different argument,
     * passing a pointer to "button 2" instead. */
    g_signal_connect (button, "clicked",
		      G_CALLBACK (callback), "button 2");

    btk_box_pack_start (BTK_BOX (box1), button, TRUE, TRUE, 0);

    /* The order in which we show the buttons is not really important, but I
     * recommend showing the window last, so it all pops up at once. */
    btk_widget_show (button);

    btk_widget_show (box1);

    btk_widget_show (window);

    /* Rest in btk_main and wait for the fun to begin! */
    btk_main ();

    return 0;
}
