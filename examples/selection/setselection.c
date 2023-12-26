
#include <stdlib.h>
#include <btk/btk.h>
#include <time.h>
#include <string.h>

BtkWidget *selection_button;
BtkWidget *selection_widget;

/* Callback when the user toggles the selection */
static void selection_toggled( BtkWidget *widget,
                               bint      *have_selection )
{
  if (BTK_TOGGLE_BUTTON (widget)->active)
    {
      *have_selection = btk_selection_owner_set (selection_widget,
						 BDK_SELECTION_PRIMARY,
						 BDK_CURRENT_TIME);
      /* if claiming the selection failed, we return the button to
	 the out state */
      if (!*have_selection)
	btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (widget), FALSE);
    }
  else
    {
      if (*have_selection)
	{
	  /* Before clearing the selection by setting the owner to NULL,
	     we check if we are the actual owner */
	  if (bdk_selection_owner_get (BDK_SELECTION_PRIMARY) == widget->window)
	    btk_selection_owner_set (NULL, BDK_SELECTION_PRIMARY,
				     BDK_CURRENT_TIME);
	  *have_selection = FALSE;
	}
    }
}

/* Called when another application claims the selection */
static bboolean selection_clear( BtkWidget         *widget,
                                 BdkEventSelection *event,
                                 bint              *have_selection )
{
  *have_selection = FALSE;
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (selection_button), FALSE);

  return TRUE;
}

/* Supplies the current time as the selection. */
static void selection_handle( BtkWidget        *widget,
                              BtkSelectionData *selection_data,
                              buint             info,
                              buint             time_stamp,
                              bpointer          data )
{
  bchar *timestr;
  time_t current_time;

  current_time = time (NULL);
  timestr = asctime (localtime (&current_time));
  /* When we return a single string, it should not be null terminated.
     That will be done for us */

  btk_selection_data_set (selection_data, BDK_SELECTION_TYPE_STRING,
			  8, timestr, strlen (timestr));
}

int main( int   argc,
          char *argv[] )
{
  BtkWidget *window;

  static int have_selection = FALSE;

  btk_init (&argc, &argv);

  /* Create the toplevel window */

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_title (BTK_WINDOW (window), "Event Box");
  btk_container_set_border_width (BTK_CONTAINER (window), 10);

  g_signal_connect (window, "destroy",
		    G_CALLBACK (exit), NULL);

  /* Create a toggle button to act as the selection */

  selection_widget = btk_invisible_new ();
  selection_button = btk_toggle_button_new_with_label ("Claim Selection");
  btk_container_add (BTK_CONTAINER (window), selection_button);
  btk_widget_show (selection_button);

  g_signal_connect (selection_button, "toggled",
		    G_CALLBACK (selection_toggled), &have_selection);
  g_signal_connect (selection_widget, "selection-clear-event",
		    G_CALLBACK (selection_clear), &have_selection);

  btk_selection_add_target (selection_widget,
			    BDK_SELECTION_PRIMARY,
			    BDK_SELECTION_TYPE_STRING,
		            1);
  g_signal_connect (selection_widget, "selection-get",
		    G_CALLBACK (selection_handle), &have_selection);

  btk_widget_show (selection_button);
  btk_widget_show (window);

  btk_main ();

  return 0;
}
