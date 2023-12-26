
#include <stdio.h>
#include <btk/btk.h>

static bboolean button_press (BtkWidget *, BdkEvent *);
static void menuitem_response (bchar *);

int main( int   argc,
          char *argv[] )
{

    BtkWidget *window;
    BtkWidget *menu;
    BtkWidget *menu_bar;
    BtkWidget *root_menu;
    BtkWidget *menu_items;
    BtkWidget *vbox;
    BtkWidget *button;
    char buf[128];
    int i;

    btk_init (&argc, &argv);

    /* create a new window */
    window = btk_window_new (BTK_WINDOW_TOPLEVEL);
    btk_widget_set_size_request (BTK_WIDGET (window), 200, 100);
    btk_window_set_title (BTK_WINDOW (window), "BTK Menu Test");
    g_signal_connect (window, "delete-event",
                      G_CALLBACK (btk_main_quit), NULL);

    /* Init the menu-widget, and remember -- never
     * btk_show_widget() the menu widget!!
     * This is the menu that holds the menu items, the one that
     * will pop up when you click on the "Root Menu" in the app */
    menu = btk_menu_new ();

    /* Next we make a little loop that makes three menu-entries for "test-menu".
     * Notice the call to btk_menu_shell_append.  Here we are adding a list of
     * menu items to our menu.  Normally, we'd also catch the "clicked"
     * signal on each of the menu items and setup a callback for it,
     * but it's omitted here to save space. */

    for (i = 0; i < 3; i++)
        {
            /* Copy the names to the buf. */
            sprintf (buf, "Test-undermenu - %d", i);

            /* Create a new menu-item with a name... */
            menu_items = btk_menu_item_new_with_label (buf);

            /* ...and add it to the menu. */
            btk_menu_shell_append (BTK_MENU_SHELL (menu), menu_items);

	    /* Do something interesting when the menuitem is selected */
	    g_signal_connect_swapped (menu_items, "activate",
		                      G_CALLBACK (menuitem_response),
                                      (bpointer) g_strdup (buf));

            /* Show the widget */
            btk_widget_show (menu_items);
        }

    /* This is the root menu, and will be the label
     * displayed on the menu bar.  There won't be a signal handler attached,
     * as it only pops up the rest of the menu when pressed. */
    root_menu = btk_menu_item_new_with_label ("Root Menu");

    btk_widget_show (root_menu);

    /* Now we specify that we want our newly created "menu" to be the menu
     * for the "root menu" */
    btk_menu_item_set_submenu (BTK_MENU_ITEM (root_menu), menu);

    /* A vbox to put a menu and a button in: */
    vbox = btk_vbox_new (FALSE, 0);
    btk_container_add (BTK_CONTAINER (window), vbox);
    btk_widget_show (vbox);

    /* Create a menu-bar to hold the menus and add it to our main window */
    menu_bar = btk_menu_bar_new ();
    btk_box_pack_start (BTK_BOX (vbox), menu_bar, FALSE, FALSE, 2);
    btk_widget_show (menu_bar);

    /* Create a button to which to attach menu as a popup */
    button = btk_button_new_with_label ("press me");
    g_signal_connect_swapped (button, "event",
	                      G_CALLBACK (button_press),
                              menu);
    btk_box_pack_end (BTK_BOX (vbox), button, TRUE, TRUE, 2);
    btk_widget_show (button);

    /* And finally we append the menu-item to the menu-bar -- this is the
     * "root" menu-item I have been raving about =) */
    btk_menu_shell_append (BTK_MENU_SHELL (menu_bar), root_menu);

    /* always display the window as the last step so it all splashes on
     * the screen at once. */
    btk_widget_show (window);

    btk_main ();

    return 0;
}

/* Respond to a button-press by posting a menu passed in as widget.
 *
 * Note that the "widget" argument is the menu being posted, NOT
 * the button that was pressed.
 */

static bboolean button_press( BtkWidget *widget,
                              BdkEvent *event )
{

    if (event->type == BDK_BUTTON_PRESS) {
        BdkEventButton *bevent = (BdkEventButton *) event;
        btk_menu_popup (BTK_MENU (widget), NULL, NULL, NULL, NULL,
                        bevent->button, bevent->time);
        /* Tell calling code that we have handled this event; the buck
         * stops here. */
        return TRUE;
    }

    /* Tell calling code that we have not handled this event; pass it on. */
    return FALSE;
}


/* Print a string when a menu item is selected */

static void menuitem_response( bchar *string )
{
    printf ("%s\n", string);
}
