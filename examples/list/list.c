
#include "config.h"
#include <btk/btk.h>
#include <stdio.h>

/* This is our data identification string to store
 * data in list items
 */
const gchar *list_item_data_key="list_item_data";


/* prototypes for signal handler that we are going to connect
 * to the List widget
 */
static void  sigh_print_selection( BtkWidget *btklist,
                                   gpointer   func_data);

static void  sigh_button_event( BtkWidget      *btklist,
                                BdkEventButton *event,
                                BtkWidget      *frame );


/* Main function to set up the user interface */

gint main( int    argc,
           gchar *argv[] )
{
    BtkWidget *separator;
    BtkWidget *window;
    BtkWidget *vbox;
    BtkWidget *scrolled_window;
    BtkWidget *frame;
    BtkWidget *btklist;
    BtkWidget *button;
    BtkWidget *list_item;
    GList *dlist;
    guint i;
    gchar buffer[64];


    /* Initialize BTK (and subsequently BDK) */

    btk_init (&argc, &argv);


    /* Create a window to put all the widgets in
     * connect btk_main_quit() to the "destroy" event of
     * the window to handle window manager close-window-events
     */
    window = btk_window_new (BTK_WINDOW_TOPLEVEL);
    btk_window_set_title (BTK_WINDOW (window), "BtkList Example");
    g_signal_connect (window, "destroy",
		      G_CALLBACK (btk_main_quit),
		      NULL);


    /* Inside the window we need a box to arrange the widgets
     * vertically */
    vbox=btk_vbox_new (FALSE, 5);
    btk_container_set_border_width (BTK_CONTAINER (vbox), 5);
    btk_container_add (BTK_CONTAINER (window), vbox);
    btk_widget_show (vbox);

    /* This is the scrolled window to put the List widget inside */
    scrolled_window = btk_scrolled_window_new (NULL, NULL);
    btk_widget_set_size_request (scrolled_window, 250, 150);
    btk_container_add (BTK_CONTAINER (vbox), scrolled_window);
    btk_widget_show (scrolled_window);

    /* Create thekList widget.
     * Connect the sigh_print_selection() signal handler
     * function to the "selection_changed" signal of the List
     * to print out the selected items each time the selection
     * has changed */
    btklist=btk_list_new ();
    btk_scrolled_window_add_with_viewport (BTK_SCROLLED_WINDOW (scrolled_window),
                                           btklist);
    btk_widget_show (btklist);
    g_signal_connect (btklist, "selection-changed",
                      G_CALLBACK (sigh_print_selection),
                      NULL);

    /* We create a "Prison" to put a list item in ;) */
    frame=btk_frame_new ("Prison");
    btk_widget_set_size_request (frame, 200, 50);
    btk_container_set_border_width (BTK_CONTAINER (frame), 5);
    btk_frame_set_shadow_type (BTK_FRAME (frame), BTK_SHADOW_OUT);
    btk_container_add (BTK_CONTAINER (vbox), frame);
    btk_widget_show (frame);

    /* Connect the sigh_button_event() signal handler to the List
     * which will handle the "arresting" of list items
     */
    g_signal_connect (btklist, "button-release-event",
                      G_CALLBACK (sigh_button_event),
                      frame);

    /* Create a separator */
    separator=btk_hseparator_new ();
    btk_container_add (BTK_CONTAINER (vbox), separator);
    btk_widget_show (separator);

    /* Finally create a button and connect its "clicked" signal
     * to the destruction of the window */
    button=btk_button_new_with_label ("Close");
    btk_container_add (BTK_CONTAINER (vbox), button);
    btk_widget_show (button);
    g_signal_connect_swapped (button, "clicked",
                              G_CALLBACK (btk_widget_destroy),
                              window);


    /* Now we create 5 list items, each having its own
     * label and add them to the List using btk_container_add()
     * Also we query the text string from the label and
     * associate it with the list_item_data_key for each list item
     */
    for (i = 0; i < 5; i++) {
	BtkWidget       *label;
	gchar           *string;

	sprintf(buffer, "ListItemContainer with Label #%d", i);
	label=btk_label_new (buffer);
	list_item=btk_list_item_new ();
	btk_container_add (BTK_CONTAINER (list_item), label);
	btk_widget_show (label);
	btk_container_add (BTK_CONTAINER (btklist), list_item);
	btk_widget_show (list_item);
	btk_label_get (BTK_LABEL (label), &string);
	g_object_set_data (B_OBJECT (list_item), list_item_data_key, string);
    }
    /* Here, we are creating another 5 labels, this time
     * we use btk_list_item_new_with_label() for the creation
     * we can't query the text string from the label because
     * we don't have the labels pointer and therefore
     * we just associate the list_item_data_key of each
     * list item with the same text string.
     * For adding of the list items we put them all into a doubly
     * linked list (GList), and then add them by a single call to
     * btk_list_append_items().
     * Because we use g_list_prepend() to put the items into the
     * doubly linked list, their order will be descending (instead
     * of ascending when using g_list_append())
     */
    dlist = NULL;
    for (; i < 10; i++) {
	sprintf(buffer, "List Item with Label %d", i);
	list_item = btk_list_item_new_with_label (buffer);
	dlist = g_list_prepend (dlist, list_item);
	btk_widget_show (list_item);
	g_object_set_data (B_OBJECT (list_item),
                           list_item_data_key,
                           "ListItem with integrated Label");
    }
    btk_list_append_items (BTK_LIST (btklist), dlist);

    /* Finally we want to see the window, don't we? ;) */
    btk_widget_show (window);

    /* Fire up the main event loop of btk */
    btk_main ();

    /* We get here after btk_main_quit() has been called which
     * happens if the main window gets destroyed
     */
    return 0;
}

/* This is the signal handler that got connected to button
 * press/release events of the List
 */
void sigh_button_event( BtkWidget      *btklist,
                        BdkEventButton *event,
                        BtkWidget      *frame )
{
    /* We only do something if the third (rightmost mouse button
     * was released
     */
    if (event->type == BDK_BUTTON_RELEASE &&
	event->button == 3) {
	GList           *dlist, *free_list;
	BtkWidget       *new_prisoner;

	/* Fetch the currently selected list item which
	 * will be our next prisoner ;)
	 */
	dlist = BTK_LIST (btklist)->selection;
	if (dlist)
		new_prisoner = BTK_WIDGET (dlist->data);
	else
		new_prisoner = NULL;

	/* Look for already imprisoned list items, we
	 * will put them back into the list.
	 * Remember to free the doubly linked list that
	 * btk_container_children() returns
	 */
	dlist = btk_container_children (BTK_CONTAINER (frame));
	free_list = dlist;
	while (dlist) {
	    BtkWidget       *list_item;

	    list_item = dlist->data;

	    btk_widget_reparent (list_item, btklist);

	    dlist = dlist->next;
	}
	g_list_free (free_list);

	/* If we have a new prisoner, remove him from the
	 * List and put him into the frame "Prison".
	 * We need to unselect the item first.
	 */
	if (new_prisoner) {
	    GList   static_dlist;

	    static_dlist.data = new_prisoner;
	    static_dlist.next = NULL;
	    static_dlist.prev = NULL;

	    btk_list_unselect_child (BTK_LIST (btklist),
				     new_prisoner);
	    btk_widget_reparent (new_prisoner, frame);
	}
    }
}

/* This is the signal handler that gets called if List
 * emits the "selection_changed" signal
 */
void sigh_print_selection( BtkWidget *btklist,
                           gpointer   func_data )
{
    GList   *dlist;

    /* Fetch the doubly linked list of selected items
     * of the List, remember to treat this as read-only!
     */
    dlist = BTK_LIST (btklist)->selection;

    /* If there are no selected items there is nothing more
     * to do than just telling the user so
     */
    if (!dlist) {
	g_print ("Selection cleared\n");
	return;
    }
    /* Ok, we got a selection and so we print it
     */
    g_print ("The selection is a ");

    /* Get the list item from the doubly linked list
     * and then query the data associated with list_item_data_key.
     * We then just print it */
    while (dlist) {
	const gchar *item_data_string;

	item_data_string = g_object_get_data (B_OBJECT (dlist->data),
	 				      list_item_data_key);
	g_print("%s ", item_data_string);

	dlist = dlist->next;
    }
    g_print ("\n");
}
