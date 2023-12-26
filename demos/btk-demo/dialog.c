/* Dialog and Message Boxes
 *
 * Dialog widgets are used to pop up a transient window for user feedback.
 */

#include <btk/btk.h>

static BtkWidget *window = NULL;
static BtkWidget *entry1 = NULL;
static BtkWidget *entry2 = NULL;

static void
message_dialog_clicked (BtkButton *button,
			bpointer   user_data)
{
  BtkWidget *dialog;
  static bint i = 1;

  dialog = btk_message_dialog_new (BTK_WINDOW (window),
				   BTK_DIALOG_MODAL | BTK_DIALOG_DESTROY_WITH_PARENT,
				   BTK_MESSAGE_INFO,
				   BTK_BUTTONS_OK,
				   "This message box has been popped up the following\n"
				   "number of times:");
  btk_message_dialog_format_secondary_text (BTK_MESSAGE_DIALOG (dialog),
                                            "%d", i);
  btk_dialog_run (BTK_DIALOG (dialog));
  btk_widget_destroy (dialog);
  i++;
}

static void
interactive_dialog_clicked (BtkButton *button,
			    bpointer   user_data)
{
  BtkWidget *dialog;
  BtkWidget *hbox;
  BtkWidget *stock;
  BtkWidget *table;
  BtkWidget *local_entry1;
  BtkWidget *local_entry2;
  BtkWidget *label;
  bint response;

  dialog = btk_dialog_new_with_buttons ("Interactive Dialog",
					BTK_WINDOW (window),
					BTK_DIALOG_MODAL| BTK_DIALOG_DESTROY_WITH_PARENT,
					BTK_STOCK_OK,
					BTK_RESPONSE_OK,
                                        "_Non-stock Button",
                                        BTK_RESPONSE_CANCEL,
					NULL);

  hbox = btk_hbox_new (FALSE, 8);
  btk_container_set_border_width (BTK_CONTAINER (hbox), 8);
  btk_box_pack_start (BTK_BOX (btk_dialog_get_content_area (BTK_DIALOG (dialog))), hbox, FALSE, FALSE, 0);

  stock = btk_image_new_from_stock (BTK_STOCK_DIALOG_QUESTION, BTK_ICON_SIZE_DIALOG);
  btk_box_pack_start (BTK_BOX (hbox), stock, FALSE, FALSE, 0);

  table = btk_table_new (2, 2, FALSE);
  btk_table_set_row_spacings (BTK_TABLE (table), 4);
  btk_table_set_col_spacings (BTK_TABLE (table), 4);
  btk_box_pack_start (BTK_BOX (hbox), table, TRUE, TRUE, 0);
  label = btk_label_new_with_mnemonic ("_Entry 1");
  btk_table_attach_defaults (BTK_TABLE (table),
			     label,
			     0, 1, 0, 1);
  local_entry1 = btk_entry_new ();
  btk_entry_set_text (BTK_ENTRY (local_entry1), btk_entry_get_text (BTK_ENTRY (entry1)));
  btk_table_attach_defaults (BTK_TABLE (table), local_entry1, 1, 2, 0, 1);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), local_entry1);

  label = btk_label_new_with_mnemonic ("E_ntry 2");
  btk_table_attach_defaults (BTK_TABLE (table),
			     label,
			     0, 1, 1, 2);

  local_entry2 = btk_entry_new ();
  btk_entry_set_text (BTK_ENTRY (local_entry2), btk_entry_get_text (BTK_ENTRY (entry2)));
  btk_table_attach_defaults (BTK_TABLE (table), local_entry2, 1, 2, 1, 2);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), local_entry2);
  
  btk_widget_show_all (hbox);
  response = btk_dialog_run (BTK_DIALOG (dialog));

  if (response == BTK_RESPONSE_OK)
    {
      btk_entry_set_text (BTK_ENTRY (entry1), btk_entry_get_text (BTK_ENTRY (local_entry1)));
      btk_entry_set_text (BTK_ENTRY (entry2), btk_entry_get_text (BTK_ENTRY (local_entry2)));
    }

  btk_widget_destroy (dialog);
}

BtkWidget *
do_dialog (BtkWidget *do_widget)
{
  BtkWidget *frame;
  BtkWidget *vbox;
  BtkWidget *vbox2;
  BtkWidget *hbox;
  BtkWidget *button;
  BtkWidget *table;
  BtkWidget *label;
  
  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (do_widget));
      btk_window_set_title (BTK_WINDOW (window), "Dialogs");

      g_signal_connect (window, "destroy", G_CALLBACK (btk_widget_destroyed), &window);
      btk_container_set_border_width (BTK_CONTAINER (window), 8);

      frame = btk_frame_new ("Dialogs");
      btk_container_add (BTK_CONTAINER (window), frame);

      vbox = btk_vbox_new (FALSE, 8);
      btk_container_set_border_width (BTK_CONTAINER (vbox), 8);
      btk_container_add (BTK_CONTAINER (frame), vbox);

      /* Standard message dialog */
      hbox = btk_hbox_new (FALSE, 8);
      btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      button = btk_button_new_with_mnemonic ("_Message Dialog");
      g_signal_connect (button, "clicked",
			G_CALLBACK (message_dialog_clicked), NULL);
      btk_box_pack_start (BTK_BOX (hbox), button, FALSE, FALSE, 0);

      btk_box_pack_start (BTK_BOX (vbox), btk_hseparator_new (), FALSE, FALSE, 0);

      /* Interactive dialog*/
      hbox = btk_hbox_new (FALSE, 8);
      btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      vbox2 = btk_vbox_new (FALSE, 0);

      button = btk_button_new_with_mnemonic ("_Interactive Dialog");
      g_signal_connect (button, "clicked",
			G_CALLBACK (interactive_dialog_clicked), NULL);
      btk_box_pack_start (BTK_BOX (hbox), vbox2, FALSE, FALSE, 0);
      btk_box_pack_start (BTK_BOX (vbox2), button, FALSE, FALSE, 0);

      table = btk_table_new (2, 2, FALSE);
      btk_table_set_row_spacings (BTK_TABLE (table), 4);
      btk_table_set_col_spacings (BTK_TABLE (table), 4);
      btk_box_pack_start (BTK_BOX (hbox), table, FALSE, FALSE, 0);

      label = btk_label_new_with_mnemonic ("_Entry 1");
      btk_table_attach_defaults (BTK_TABLE (table),
				 label,
				 0, 1, 0, 1);

      entry1 = btk_entry_new ();
      btk_table_attach_defaults (BTK_TABLE (table), entry1, 1, 2, 0, 1);
      btk_label_set_mnemonic_widget (BTK_LABEL (label), entry1);

      label = btk_label_new_with_mnemonic ("E_ntry 2");
      
      btk_table_attach_defaults (BTK_TABLE (table),
				 label,
				 0, 1, 1, 2);

      entry2 = btk_entry_new ();
      btk_table_attach_defaults (BTK_TABLE (table), entry2, 1, 2, 1, 2);
      btk_label_set_mnemonic_widget (BTK_LABEL (label), entry2);
    }

  if (!btk_widget_get_visible (window))
    {
      btk_widget_show_all (window);
    }
  else
    {	 
      btk_widget_destroy (window);
      window = NULL;
    }

  return window;
}
