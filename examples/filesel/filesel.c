
#include <btk/btk.h>

/* Get the selected filename and print it to the console */
static void file_ok_sel( BtkWidget        *w,
                         BtkFileSelection *fs )
{
    g_print ("%s\n", btk_file_selection_get_filename (BTK_FILE_SELECTION (fs)));
}

int main( int   argc,
          char *argv[] )
{
    BtkWidget *filew;
    
    btk_init (&argc, &argv);
    
    /* Create a new file selection widget */
    filew = btk_file_selection_new ("File selection");
    
    g_signal_connect (G_OBJECT (filew), "destroy",
	              G_CALLBACK (btk_main_quit), NULL);
    /* Connect the ok_button to file_ok_sel function */
    g_signal_connect (G_OBJECT (BTK_FILE_SELECTION (filew)->ok_button),
		      "clicked", G_CALLBACK (file_ok_sel), (gpointer) filew);
    
    /* Connect the cancel_button to destroy the widget */
    g_signal_connect_swapped (G_OBJECT (BTK_FILE_SELECTION (filew)->cancel_button),
	                      "clicked", G_CALLBACK (btk_widget_destroy),
			      G_OBJECT (filew));
    
    /* Lets set the filename, as if this were a save dialog, and we are giving
     a default filename */
    btk_file_selection_set_filename (BTK_FILE_SELECTION(filew), 
				     "penguin.png");
    
    btk_widget_show (filew);
    btk_main ();
    return 0;
}
