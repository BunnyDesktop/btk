
#include <btk/btk.h>

/* This is a callback function. The data arguments are ignored
 * in this example. More on callbacks below. */
static void hello( BtkWidget *widget,
                   gpointer   data )
{
    g_print ("Hello World\n");
}

static gboolean delete_event( BtkWidget *widget,
                              BdkEvent  *event,
                              gpointer   data )
{
    /* If you return FALSE in the "delete_event" signal handler,
     * BTK will emit the "destroy" signal. Returning TRUE means
     * you don't want the window to be destroyed.
     * This is useful for popping up 'are you sure you want to quit?'
     * type dialogs. */

    g_print ("delete event occurred\n");

    /* Change TRUE to FALSE and the main window will be destroyed with
     * a "delete_event". */

    return TRUE;
}

/* Another callback */
static void destroy( BtkWidget *widget,
                     gpointer   data )
{
    btk_main_quit ();
}

int main( int   argc,
          char *argv[] )
{
    /* BtkWidget is the storage type for widgets */
    BtkWidget *window;
    BtkWidget *button;

    /* This is called in all BTK applications. Arguments are parsed
     * from the command line and are returned to the application. */
    btk_init (&argc, &argv);

    /* create a new window */
    window = btk_window_new (BTK_WINDOW_TOPLEVEL);

    /* When the window is given the "delete-event" signal (this is given
     * by the window manager, usually by the "close" option, or on the
     * titlebar), we ask it to call the delete_event () function
     * as defined above. The data passed to the callback
     * function is NULL and is ignored in the callback function. */
    g_signal_connect (window, "delete-event",
		      G_CALLBACK (delete_event), NULL);

    /* Here we connect the "destroy" event to a signal handler.
     * This event occurs when we call btk_widget_destroy() on the window,
     * or if we return FALSE in the "delete_event" callback. */
    g_signal_connect (window, "destroy",
		      G_CALLBACK (destroy), NULL);

    /* Sets the border width of the window. */
    btk_container_set_border_width (BTK_CONTAINER (window), 10);

    /* Creates a new button with the label "Hello World". */
    button = btk_button_new_with_label ("Hello World");

    /* When the button receives the "clicked" signal, it will call the
     * function hello() passing it NULL as its argument.  The hello()
     * function is defined above. */
    g_signal_connect (button, "clicked",
		      G_CALLBACK (hello), NULL);

    /* This will cause the window to be destroyed by calling
     * btk_widget_destroy(window) when "clicked".  Again, the destroy
     * signal could come from here, or the window manager. */
    g_signal_connect_swapped (button, "clicked",
			      G_CALLBACK (btk_widget_destroy),
                              window);

    /* This packs the button into the window (a btk container). */
    btk_container_add (BTK_CONTAINER (window), button);

    /* The final step is to display this newly created widget. */
    btk_widget_show (button);

    /* and the window */
    btk_widget_show (window);

    /* All BTK applications must have a btk_main(). Control ends here
     * and waits for an event to occur (like a key press or
     * mouse event). */
    btk_main ();

    return 0;
}
