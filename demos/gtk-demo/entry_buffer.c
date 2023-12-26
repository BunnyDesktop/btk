/* Entry/Entry Buffer
 *
 * BtkEntryBuffer provides the text content in a BtkEntry.
 *
 */

#include <btk/btk.h>

static BtkWidget *window = NULL;

BtkWidget *
do_entry_buffer (BtkWidget *do_widget)
{
  BtkWidget *vbox;
  BtkWidget *label;
  BtkWidget *entry;
  BtkEntryBuffer *buffer;

  if (!window)
  {
    window = btk_dialog_new_with_buttons ("BtkEntryBuffer",
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
    btk_label_set_markup (BTK_LABEL (label), "Entries share a buffer. Typing in one is reflected in the other.");
    btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);

    /* Create a buffer */
    buffer = btk_entry_buffer_new (NULL, 0);

    /* Create our first entry */
    entry = btk_entry_new_with_buffer (buffer);
    btk_box_pack_start (BTK_BOX (vbox), entry, FALSE, FALSE, 0);

    /* Create the second entry */
    entry = btk_entry_new_with_buffer (buffer);
    btk_entry_set_visibility (BTK_ENTRY (entry), FALSE);
    btk_box_pack_start (BTK_BOX (vbox), entry, FALSE, FALSE, 0);

    g_object_unref (buffer);
  }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);

  return window;
}


