
#include <stdlib.h>
#include <btk/btk.h>

static void selection_received( BtkWidget        *widget,
                                BtkSelectionData *selection_data,
                                gpointer          data );

/* Signal handler invoked when user clicks on the "Get Targets" button */
static void get_targets( BtkWidget *widget,
                         gpointer data )
{
  static BdkAtom targets_atom = BDK_NONE;
  BtkWidget *window = (BtkWidget *)data;

  /* Get the atom corresponding to the string "TARGETS" */
  if (targets_atom == BDK_NONE)
    targets_atom = bdk_atom_intern ("TARGETS", FALSE);

  /* And request the "TARGETS" target for the primary selection */
  btk_selection_convert (window, BDK_SELECTION_PRIMARY, targets_atom,
			 BDK_CURRENT_TIME);
}

/* Signal handler called when the selections owner returns the data */
static void selection_received( BtkWidget        *widget,
                                BtkSelectionData *selection_data,
                                gpointer          data )
{
  BdkAtom *atoms;
  GList *item_list;
  int i;

  /* **** IMPORTANT **** Check to see if retrieval succeeded  */
  if (selection_data->length < 0)
    {
      g_print ("Selection retrieval failed\n");
      return;
    }
  /* Make sure we got the data in the expected form */
  if (selection_data->type != BDK_SELECTION_TYPE_ATOM)
    {
      g_print ("Selection \"TARGETS\" was not returned as atoms!\n");
      return;
    }

  /* Print out the atoms we received */
  atoms = (BdkAtom *)selection_data->data;

  item_list = NULL;
  for (i = 0; i < selection_data->length / sizeof(BdkAtom); i++)
    {
      char *name;
      name = bdk_atom_name (atoms[i]);
      if (name != NULL)
	g_print ("%s\n",name);
      else
	g_print ("(bad atom)\n");
    }

  return;
}

int main( int   argc,
          char *argv[] )
{
  BtkWidget *window;
  BtkWidget *button;

  btk_init (&argc, &argv);

  /* Create the toplevel window */

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_title (BTK_WINDOW (window), "Event Box");
  btk_container_set_border_width (BTK_CONTAINER (window), 10);

  g_signal_connect (window, "destroy",
	            G_CALLBACK (exit), NULL);

  /* Create a button the user can click to get targets */

  button = btk_button_new_with_label ("Get Targets");
  btk_container_add (BTK_CONTAINER (window), button);

  g_signal_connect (button, "clicked",
		    G_CALLBACK (get_targets), (gpointer) window);
  g_signal_connect (window, "selection-received",
		    G_CALLBACK (selection_received), NULL);

  btk_widget_show (button);
  btk_widget_show (window);

  btk_main ();

  return 0;
}
