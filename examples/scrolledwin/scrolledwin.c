
#include <stdio.h>
#include <btk/btk.h>

static void destroy( BtkWidget *widget,
                     gpointer   data )
{
    btk_main_quit ();
}

int main( int   argc,
          char *argv[] )
{
    static BtkWidget *window;
    BtkWidget *scrolled_window;
    BtkWidget *table;
    BtkWidget *button;
    char buffer[32];
    int i, j;
    
    btk_init (&argc, &argv);
    
    /* Create a new dialog window for the scrolled window to be
     * packed into.  */
    window = btk_dialog_new ();
    g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK (destroy), NULL);
    btk_window_set_title (BTK_WINDOW (window), "BtkScrolledWindow example");
    btk_container_set_border_width (BTK_CONTAINER (window), 0);
    btk_widget_set_size_request (window, 300, 300);
    
    /* create a new scrolled window. */
    scrolled_window = btk_scrolled_window_new (NULL, NULL);
    
    btk_container_set_border_width (BTK_CONTAINER (scrolled_window), 10);
    
    /* the policy is one of BTK_POLICY AUTOMATIC, or BTK_POLICY_ALWAYS.
     * BTK_POLICY_AUTOMATIC will automatically decide whether you need
     * scrollbars, whereas BTK_POLICY_ALWAYS will always leave the scrollbars
     * there.  The first one is the horizontal scrollbar, the second, 
     * the vertical. */
    btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window),
                                    BTK_POLICY_AUTOMATIC, BTK_POLICY_ALWAYS);
    /* The dialog window is created with a vbox packed into it. */								
    btk_box_pack_start (BTK_BOX (BTK_DIALOG(window)->vbox), scrolled_window, 
			TRUE, TRUE, 0);
    btk_widget_show (scrolled_window);
    
    /* create a table of 10 by 10 squares. */
    table = btk_table_new (10, 10, FALSE);
    
    /* set the spacing to 10 on x and 10 on y */
    btk_table_set_row_spacings (BTK_TABLE (table), 10);
    btk_table_set_col_spacings (BTK_TABLE (table), 10);
    
    /* pack the table into the scrolled window */
    btk_scrolled_window_add_with_viewport (
                   BTK_SCROLLED_WINDOW (scrolled_window), table);
    btk_widget_show (table);
    
    /* this simply creates a grid of toggle buttons on the table
     * to demonstrate the scrolled window. */
    for (i = 0; i < 10; i++)
       for (j = 0; j < 10; j++) {
          sprintf (buffer, "button (%d,%d)\n", i, j);
	  button = btk_toggle_button_new_with_label (buffer);
	  btk_table_attach_defaults (BTK_TABLE (table), button,
	                             i, i+1, j, j+1);
          btk_widget_show (button);
       }
    
    /* Add a "close" button to the bottom of the dialog */
    button = btk_button_new_with_label ("close");
    g_signal_connect_swapped (G_OBJECT (button), "clicked",
			      G_CALLBACK (btk_widget_destroy),
			      G_OBJECT (window));
    
    /* this makes it so the button is the default. */
    
    btk_widget_set_can_default (button, TRUE);
    btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->action_area), button, TRUE, TRUE, 0);
    
    /* This grabs this button to be the default button. Simply hitting
     * the "Enter" key will cause this button to activate. */
    btk_widget_grab_default (button);
    btk_widget_show (button);
    
    btk_widget_show (window);
    
    btk_main();
    
    return 0;
}
