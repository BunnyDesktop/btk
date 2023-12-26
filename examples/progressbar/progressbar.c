
#include <btk/btk.h>

typedef struct _ProgressData {
  BtkWidget *window;
  BtkWidget *pbar;
  int timer;
  gboolean activity_mode;
} ProgressData;

/* Update the value of the progress bar so that we get
 * some movement */
static gboolean progress_timeout( gpointer data )
{
  ProgressData *pdata = (ProgressData *)data;
  gdouble new_val;
  
  if (pdata->activity_mode) 
    btk_progress_bar_pulse (BTK_PROGRESS_BAR (pdata->pbar));
  else 
    {
      /* Calculate the value of the progress bar using the
       * value range set in the adjustment object */
      
      new_val = btk_progress_bar_get_fraction (BTK_PROGRESS_BAR (pdata->pbar)) + 0.01;
      
      if (new_val > 1.0)
	new_val = 0.0;
      
      /* Set the new value */
      btk_progress_bar_set_fraction (BTK_PROGRESS_BAR (pdata->pbar), new_val);
    }
  
  /* As this is a timeout function, return TRUE so that it
   * continues to get called */
  return TRUE;
} 

/* Callback that toggles the text display within the progress bar trough */
static void toggle_show_text( BtkWidget    *widget,
                              ProgressData *pdata )
{
  const gchar *text;
  
  text = btk_progress_bar_get_text (BTK_PROGRESS_BAR (pdata->pbar));
  if (text && *text)
    btk_progress_bar_set_text (BTK_PROGRESS_BAR (pdata->pbar), "");
  else 
    btk_progress_bar_set_text (BTK_PROGRESS_BAR (pdata->pbar), "some text");
}

/* Callback that toggles the activity mode of the progress bar */
static void toggle_activity_mode( BtkWidget    *widget,
                                  ProgressData *pdata )
{
  pdata->activity_mode = !pdata->activity_mode;
  if (pdata->activity_mode) 
      btk_progress_bar_pulse (BTK_PROGRESS_BAR (pdata->pbar));
  else
      btk_progress_bar_set_fraction (BTK_PROGRESS_BAR (pdata->pbar), 0.0);
}

 
/* Callback that toggles the orientation of the progress bar */
static void toggle_orientation( BtkWidget    *widget,
                                ProgressData *pdata )
{
  switch (btk_progress_bar_get_orientation (BTK_PROGRESS_BAR (pdata->pbar))) {
  case BTK_PROGRESS_LEFT_TO_RIGHT:
    btk_progress_bar_set_orientation (BTK_PROGRESS_BAR (pdata->pbar), 
				      BTK_PROGRESS_RIGHT_TO_LEFT);
    break;
  case BTK_PROGRESS_RIGHT_TO_LEFT:
    btk_progress_bar_set_orientation (BTK_PROGRESS_BAR (pdata->pbar), 
				      BTK_PROGRESS_LEFT_TO_RIGHT);
    break;
  default:;
    /* do nothing */
  }
}

 
/* Clean up allocated memory and remove the timer */
static void destroy_progress( BtkWidget    *widget,
                              ProgressData *pdata)
{
    g_source_remove (pdata->timer);
    pdata->timer = 0;
    pdata->window = NULL;
    g_free (pdata);
    btk_main_quit ();
}

int main( int   argc,
          char *argv[])
{
    ProgressData *pdata;
    BtkWidget *align;
    BtkWidget *separator;
    BtkWidget *table;
    BtkWidget *button;
    BtkWidget *check;
    BtkWidget *vbox;

    btk_init (&argc, &argv);

    /* Allocate memory for the data that is passed to the callbacks */
    pdata = g_malloc (sizeof (ProgressData));
  
    pdata->window = btk_window_new (BTK_WINDOW_TOPLEVEL);
    btk_window_set_resizable (BTK_WINDOW (pdata->window), TRUE);

    g_signal_connect (G_OBJECT (pdata->window), "destroy",
	              G_CALLBACK (destroy_progress),
                      (gpointer) pdata);
    btk_window_set_title (BTK_WINDOW (pdata->window), "BtkProgressBar");
    btk_container_set_border_width (BTK_CONTAINER (pdata->window), 0);

    vbox = btk_vbox_new (FALSE, 5);
    btk_container_set_border_width (BTK_CONTAINER (vbox), 10);
    btk_container_add (BTK_CONTAINER (pdata->window), vbox);
    btk_widget_show (vbox);
  
    /* Create a centering alignment object */
    align = btk_alignment_new (0.5, 0.5, 0, 0);
    btk_box_pack_start (BTK_BOX (vbox), align, FALSE, FALSE, 5);
    btk_widget_show (align);

    /* Create the BtkProgressBar */
    pdata->pbar = btk_progress_bar_new ();
    pdata->activity_mode = FALSE;

    btk_container_add (BTK_CONTAINER (align), pdata->pbar);
    btk_widget_show (pdata->pbar);

    /* Add a timer callback to update the value of the progress bar */
    pdata->timer = bdk_threads_add_timeout (100, progress_timeout, pdata);

    separator = btk_hseparator_new ();
    btk_box_pack_start (BTK_BOX (vbox), separator, FALSE, FALSE, 0);
    btk_widget_show (separator);

    /* rows, columns, homogeneous */
    table = btk_table_new (2, 3, FALSE);
    btk_box_pack_start (BTK_BOX (vbox), table, FALSE, TRUE, 0);
    btk_widget_show (table);

    /* Add a check button to select displaying of the trough text */
    check = btk_check_button_new_with_label ("Show text");
    btk_table_attach (BTK_TABLE (table), check, 0, 1, 0, 1,
                      BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
		      5, 5);
    g_signal_connect (G_OBJECT (check), "clicked",
                      G_CALLBACK (toggle_show_text),
                      (gpointer) pdata);
    btk_widget_show (check);

    /* Add a check button to toggle activity mode */
    check = btk_check_button_new_with_label ("Activity mode");
    btk_table_attach (BTK_TABLE (table), check, 0, 1, 1, 2,
                      BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
                      5, 5);
    g_signal_connect (G_OBJECT (check), "clicked",
                      G_CALLBACK (toggle_activity_mode),
                      (gpointer) pdata);
    btk_widget_show (check);

    /* Add a check button to toggle orientation */
    check = btk_check_button_new_with_label ("Right to Left");
    btk_table_attach (BTK_TABLE (table), check, 0, 1, 2, 3,
                      BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
                      5, 5);
    g_signal_connect (G_OBJECT (check), "clicked",
                      G_CALLBACK (toggle_orientation),
                      (gpointer) pdata);
    btk_widget_show (check);

    /* Add a button to exit the program */
    button = btk_button_new_with_label ("close");
    g_signal_connect_swapped (G_OBJECT (button), "clicked",
                              G_CALLBACK (btk_widget_destroy),
                              G_OBJECT (pdata->window));
    btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);

    /* This makes it so the button is the default. */
    btk_widget_set_can_default (button, TRUE);

    /* This grabs this button to be the default button. Simply hitting
     * the "Enter" key will cause this button to activate. */
    btk_widget_grab_default (button);
    btk_widget_show (button);

    btk_widget_show (pdata->window);

    btk_main ();
    
    return 0;
}
