/* Entry/Entry Completion
 *
 * BtkEntryCompletion provides a mechanism for adding support for
 * completion in BtkEntry.
 *
 */

#include <btk/btk.h>

static BtkWidget *window = NULL;

/* Creates a tree model containing the completions */
BtkTreeModel *
create_completion_model (void)
{
  BtkListStore *store;
  BtkTreeIter iter;
  
  store = btk_list_store_new (1, G_TYPE_STRING);

  /* Append one word */
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "BUNNY", -1);

  /* Append another word */
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "total", -1);

  /* And another word */
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "totally", -1);
  
  return BTK_TREE_MODEL (store);
}


BtkWidget *
do_entry_completion (BtkWidget *do_widget)
{
  BtkWidget *vbox;
  BtkWidget *label;
  BtkWidget *entry;
  BtkEntryCompletion *completion;
  BtkTreeModel *completion_model;
  
  if (!window)
  {
    window = btk_dialog_new_with_buttons ("BtkEntryCompletion",
					  BTK_WINDOW (do_widget),
					  0,
					  BTK_STOCK_CLOSE,
					  BTK_RESPONSE_NONE,
					  NULL);
    btk_window_set_resizable (BTK_WINDOW (window), FALSE);

    g_signal_connect (window, "response",
		      G_CALLBACK (btk_widget_destroy), NULL);
    g_signal_connect (window, "destroy",
		      G_CALLBACK (btk_widget_destroyed), &window);

    vbox = btk_vbox_new (FALSE, 5);
    btk_box_pack_start (BTK_BOX (btk_dialog_get_content_area (BTK_DIALOG (window))), vbox, TRUE, TRUE, 0);
    btk_container_set_border_width (BTK_CONTAINER (vbox), 5);

    label = btk_label_new (NULL);
    btk_label_set_markup (BTK_LABEL (label), "Completion demo, try writing <b>total</b> or <b>bunny</b> for example.");
    btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);

    /* Create our entry */
    entry = btk_entry_new ();
    btk_box_pack_start (BTK_BOX (vbox), entry, FALSE, FALSE, 0);

    /* Create the completion object */
    completion = btk_entry_completion_new ();

    /* Assign the completion to the entry */
    btk_entry_set_completion (BTK_ENTRY (entry), completion);
    g_object_unref (completion);
    
    /* Create a tree model and use it as the completion model */
    completion_model = create_completion_model ();
    btk_entry_completion_set_model (completion, completion_model);
    g_object_unref (completion_model);
    
    /* Use model column 0 as the text column */
    btk_entry_completion_set_text_column (completion, 0);
  }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);

  return window;
}


