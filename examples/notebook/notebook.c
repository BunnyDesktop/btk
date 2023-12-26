
#include <stdio.h>
#include <btk/btk.h>

/* This function rotates the position of the tabs */
static void rotate_book( BtkButton   *button,
                         BtkNotebook *notebook )
{
    btk_notebook_set_tab_pos (notebook, (notebook->tab_pos + 1) % 4);
}

/* Add/Remove the page tabs and the borders */
static void tabsborder_book( BtkButton   *button,
                             BtkNotebook *notebook )
{
    gint tval = FALSE;
    gint bval = FALSE;
    if (notebook->show_tabs == 0)
	    tval = TRUE;
    if (notebook->show_border == 0)
	    bval = TRUE;

    btk_notebook_set_show_tabs (notebook, tval);
    btk_notebook_set_show_border (notebook, bval);
}

/* Remove a page from the notebook */
static void remove_book( BtkButton   *button,
                         BtkNotebook *notebook )
{
    gint page;

    page = btk_notebook_get_current_page (notebook);
    btk_notebook_remove_page (notebook, page);
    /* Need to refresh the widget --
     This forces the widget to redraw itself. */
    btk_widget_queue_draw (BTK_WIDGET (notebook));
}

static gboolean delete( BtkWidget *widget,
                        BtkWidget *event,
                        gpointer   data )
{
    btk_main_quit ();
    return FALSE;
}

int main( int argc,
          char *argv[] )
{
    BtkWidget *window;
    BtkWidget *button;
    BtkWidget *table;
    BtkWidget *notebook;
    BtkWidget *frame;
    BtkWidget *label;
    BtkWidget *checkbutton;
    int i;
    char bufferf[32];
    char bufferl[32];

    btk_init (&argc, &argv);

    window = btk_window_new (BTK_WINDOW_TOPLEVEL);

    g_signal_connect (window, "delete-event",
	              G_CALLBACK (delete), NULL);

    btk_container_set_border_width (BTK_CONTAINER (window), 10);

    table = btk_table_new (3, 6, FALSE);
    btk_container_add (BTK_CONTAINER (window), table);

    /* Create a new notebook, place the position of the tabs */
    notebook = btk_notebook_new ();
    btk_notebook_set_tab_pos (BTK_NOTEBOOK (notebook), BTK_POS_TOP);
    btk_table_attach_defaults (BTK_TABLE (table), notebook, 0, 6, 0, 1);
    btk_widget_show (notebook);

    /* Let's append a bunch of pages to the notebook */
    for (i = 0; i < 5; i++) {
	sprintf(bufferf, "Append Frame %d", i + 1);
	sprintf(bufferl, "Page %d", i + 1);

	frame = btk_frame_new (bufferf);
	btk_container_set_border_width (BTK_CONTAINER (frame), 10);
	btk_widget_set_size_request (frame, 100, 75);
	btk_widget_show (frame);

	label = btk_label_new (bufferf);
	btk_container_add (BTK_CONTAINER (frame), label);
	btk_widget_show (label);

	label = btk_label_new (bufferl);
	btk_notebook_append_page (BTK_NOTEBOOK (notebook), frame, label);
    }

    /* Now let's add a page to a specific spot */
    checkbutton = btk_check_button_new_with_label ("Check me please!");
    btk_widget_set_size_request (checkbutton, 100, 75);
    btk_widget_show (checkbutton);

    label = btk_label_new ("Add page");
    btk_notebook_insert_page (BTK_NOTEBOOK (notebook), checkbutton, label, 2);

    /* Now finally let's prepend pages to the notebook */
    for (i = 0; i < 5; i++) {
	sprintf (bufferf, "Prepend Frame %d", i + 1);
	sprintf (bufferl, "PPage %d", i + 1);

	frame = btk_frame_new (bufferf);
	btk_container_set_border_width (BTK_CONTAINER (frame), 10);
	btk_widget_set_size_request (frame, 100, 75);
	btk_widget_show (frame);

	label = btk_label_new (bufferf);
	btk_container_add (BTK_CONTAINER (frame), label);
	btk_widget_show (label);

	label = btk_label_new (bufferl);
	btk_notebook_prepend_page (BTK_NOTEBOOK (notebook), frame, label);
    }

    /* Set what page to start at (page 4) */
    btk_notebook_set_current_page (BTK_NOTEBOOK (notebook), 3);

    /* Create a bunch of buttons */
    button = btk_button_new_with_label ("close");
    g_signal_connect_swapped (button, "clicked",
			      G_CALLBACK (delete), NULL);
    btk_table_attach_defaults (BTK_TABLE (table), button, 0, 1, 1, 2);
    btk_widget_show (button);

    button = btk_button_new_with_label ("next page");
    g_signal_connect_swapped (button, "clicked",
			      G_CALLBACK (btk_notebook_next_page),
			      notebook);
    btk_table_attach_defaults (BTK_TABLE (table), button, 1, 2, 1, 2);
    btk_widget_show (button);

    button = btk_button_new_with_label ("prev page");
    g_signal_connect_swapped (button, "clicked",
			      G_CALLBACK (btk_notebook_prev_page),
			      notebook);
    btk_table_attach_defaults (BTK_TABLE (table), button, 2, 3, 1, 2);
    btk_widget_show (button);

    button = btk_button_new_with_label ("tab position");
    g_signal_connect (button, "clicked",
                      G_CALLBACK (rotate_book),
	              notebook);
    btk_table_attach_defaults (BTK_TABLE (table), button, 3, 4, 1, 2);
    btk_widget_show (button);

    button = btk_button_new_with_label ("tabs/border on/off");
    g_signal_connect (button, "clicked",
                      G_CALLBACK (tabsborder_book),
                      notebook);
    btk_table_attach_defaults (BTK_TABLE (table), button, 4, 5, 1, 2);
    btk_widget_show (button);

    button = btk_button_new_with_label ("remove page");
    g_signal_connect (button, "clicked",
                      G_CALLBACK (remove_book),
                      notebook);
    btk_table_attach_defaults (BTK_TABLE (table), button, 5, 6, 1, 2);
    btk_widget_show (button);

    btk_widget_show (table);
    btk_widget_show (window);

    btk_main ();

    return 0;
}
