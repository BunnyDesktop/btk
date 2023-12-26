
#include "config.h"
#include <btk/btk.h>

/* User clicked the "Add List" button. */
void button_add_clicked( gpointer data )
{
    int indx;
 
    /* Something silly to add to the list. 4 rows of 2 columns each */
    gchar *drink[4][2] = { { "Milk",    "3 Oz" },
                           { "Water",   "6 l" },
                           { "Carrots", "2" },
                           { "Snakes",  "55" } };

    /* Here we do the actual adding of the text. It's done once for
     * each row.
     */
    for (indx = 0; indx < 4; indx++)
	btk_clist_append ((BtkCList *)data, drink[indx]);

    return;
}

/* User clicked the "Clear List" button. */
void button_clear_clicked( gpointer data )
{
    /* Clear the list using btk_clist_clear. This is much faster than
     * calling btk_clist_remove once for each row.
     */
    btk_clist_clear ((BtkCList *)data);

    return;
}

/* The user clicked the "Hide/Show titles" button. */
void button_hide_show_clicked( gpointer data )
{
    /* Just a flag to remember the status. 0 = currently visible */
    static short int flag = 0;

    if (flag == 0)
    {
        /* Hide the titles and set the flag to 1 */
	btk_clist_column_titles_hide ((BtkCList *)data);
	flag++;
    }
    else
    {
        /* Show the titles and reset flag to 0 */
	btk_clist_column_titles_show ((BtkCList *)data);
	flag--;
    }

    return;
}

/* If we come here, then the user has selected a row in the list. */
void selection_made( BtkWidget      *clist,
                     gint            row,
                     gint            column,
		     BdkEventButton *event,
                     gpointer        data )
{
    gchar *text;

    /* Get the text that is stored in the selected row and column
     * which was clicked in. We will receive it as a pointer in the
     * argument text.
     */
    btk_clist_get_text (BTK_CLIST (clist), row, column, &text);

    /* Just prints some information about the selected row */
    g_print ("You selected row %d. More specifically you clicked in "
             "column %d, and the text in this cell is %s\n\n",
	     row, column, text);

    return;
}

int main( int    argc,
          gchar *argv[] )
{                                  
    BtkWidget *window;
    BtkWidget *vbox, *hbox;
    BtkWidget *scrolled_window, *clist;
    BtkWidget *button_add, *button_clear, *button_hide_show;    
    gchar *titles[2] = { "Ingredients", "Amount" };

    btk_init(&argc, &argv);
    
    window=btk_window_new (BTK_WINDOW_TOPLEVEL);
    btk_widget_set_size_request (BTK_WIDGET (window), 300, 150);

    btk_window_set_title (BTK_WINDOW (window), "BtkCList Example");
    g_signal_connect (G_OBJECT (window), "destroy",
                      G_CALLBACK (btk_main_quit),
                      NULL);
    
    vbox=btk_vbox_new (FALSE, 5);
    btk_container_set_border_width (BTK_CONTAINER (vbox), 5);
    btk_container_add (BTK_CONTAINER (window), vbox);
    btk_widget_show (vbox);
    
    /* Create a scrolled window to pack the CList widget into */
    scrolled_window = btk_scrolled_window_new (NULL, NULL);
    btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window),
                                    BTK_POLICY_AUTOMATIC, BTK_POLICY_ALWAYS);

    btk_box_pack_start (BTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
    btk_widget_show (scrolled_window);

    /* Create the CList. For this example we use 2 columns */
    clist = btk_clist_new_with_titles (2, titles);

    /* When a selection is made, we want to know about it. The callback
     * used is selection_made, and its code can be found further down */
    g_signal_connect (G_OBJECT (clist), "select_row",
                      G_CALLBACK (selection_made),
                      NULL);

    /* It isn't necessary to shadow the border, but it looks nice :) */
    btk_clist_set_shadow_type (BTK_CLIST (clist), BTK_SHADOW_OUT);

    /* What however is important, is that we set the column widths as
     * they will never be right otherwise. Note that the columns are
     * numbered from 0 and up (to 1 in this case).
     */
    btk_clist_set_column_width (BTK_CLIST (clist), 0, 150);

    /* Add the CList widget to the vertical box and show it. */
    btk_container_add (BTK_CONTAINER (scrolled_window), clist);
    btk_widget_show (clist);

    /* Create the buttons and add them to the window. See the button
     * tutorial for more examples and comments on this.
     */
    hbox = btk_hbox_new (FALSE, 0);
    btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, TRUE, 0);
    btk_widget_show (hbox);

    button_add = btk_button_new_with_label ("Add List");
    button_clear = btk_button_new_with_label ("Clear List");
    button_hide_show = btk_button_new_with_label ("Hide/Show titles");

    btk_box_pack_start (BTK_BOX (hbox), button_add, TRUE, TRUE, 0);
    btk_box_pack_start (BTK_BOX (hbox), button_clear, TRUE, TRUE, 0);
    btk_box_pack_start (BTK_BOX (hbox), button_hide_show, TRUE, TRUE, 0);

    /* Connect our callbacks to the three buttons */
    g_signal_connect_swapped (G_OBJECT (button_add), "clicked",
                              G_CALLBACK (button_add_clicked),
			      clist);
    g_signal_connect_swapped (G_OBJECT (button_clear), "clicked",
                              G_CALLBACK (button_clear_clicked),
                              clist);
    g_signal_connect_swapped (G_OBJECT (button_hide_show), "clicked",
                              G_CALLBACK (button_hide_show_clicked),
                              clist);

    btk_widget_show (button_add);
    btk_widget_show (button_clear);
    btk_widget_show (button_hide_show);

    /* The interface is completely set up so we show the window and
     * enter the btk_main loop.
     */
    btk_widget_show (window);

    btk_main();
    
    return 0;
}
