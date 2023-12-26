/* Entry/Search Entry
 *
 * BtkEntry allows to display icons and progress information.
 * This demo shows how to use these features in a search entry.
 */

#include <btk/btk.h>

static BtkWidget *window = NULL;
static BtkWidget *menu = NULL;
static BtkWidget *notebook = NULL;

static buint search_progress_id = 0;
static buint finish_search_id = 0;

static void
show_find_button (void)
{
  btk_notebook_set_current_page (BTK_NOTEBOOK (notebook), 0);
}

static void
show_cancel_button (void)
{
  btk_notebook_set_current_page (BTK_NOTEBOOK (notebook), 1);
}

static bboolean
search_progress (bpointer data)
{
  btk_entry_progress_pulse (BTK_ENTRY (data));

  return TRUE;
}

static void
search_progress_done (BtkEntry *entry)
{
  btk_entry_set_progress_fraction (entry, 0.0);
}

static bboolean
finish_search (BtkButton *button)
{
  show_find_button ();
  g_source_remove (search_progress_id);
  search_progress_id = 0;

  return FALSE;
}

static bboolean
start_search_feedback (bpointer data)
{
  search_progress_id = g_timeout_add_full (G_PRIORITY_DEFAULT, 100,
                                           (GSourceFunc)search_progress, data,
                                           (GDestroyNotify)search_progress_done);
  return FALSE;
}

static void
start_search (BtkButton *button,
              BtkEntry  *entry)
{
  show_cancel_button ();
  search_progress_id = g_timeout_add_seconds (1, (GSourceFunc)start_search_feedback, entry);
  finish_search_id = g_timeout_add_seconds (15, (GSourceFunc)finish_search, button);
}


static void
stop_search (BtkButton *button,
             bpointer   data)
{
  g_source_remove (finish_search_id);
  finish_search (button);
}

static void
clear_entry (BtkEntry *entry)
{
  btk_entry_set_text (entry, "");
}

static void
search_by_name (BtkWidget *item,
                BtkEntry  *entry)
{
  btk_entry_set_icon_from_stock (entry,
                                 BTK_ENTRY_ICON_PRIMARY,
                                 BTK_STOCK_FIND);
  btk_entry_set_icon_tooltip_text (entry,
                                   BTK_ENTRY_ICON_PRIMARY,
                                   "Search by name\n"
                                   "Click here to change the search type");
}

static void
search_by_description (BtkWidget *item,
                       BtkEntry  *entry)
{
  btk_entry_set_icon_from_stock (entry,
                                 BTK_ENTRY_ICON_PRIMARY,
                                 BTK_STOCK_EDIT);
  btk_entry_set_icon_tooltip_text (entry,
                                   BTK_ENTRY_ICON_PRIMARY,
                                   "Search by description\n"
                                   "Click here to change the search type");
}

static void
search_by_file (BtkWidget *item,
                BtkEntry  *entry)
{
  btk_entry_set_icon_from_stock (entry,
                                 BTK_ENTRY_ICON_PRIMARY,
                                 BTK_STOCK_OPEN);
  btk_entry_set_icon_tooltip_text (entry,
                                   BTK_ENTRY_ICON_PRIMARY,
                                   "Search by file name\n"
                                   "Click here to change the search type");
}

BtkWidget *
create_search_menu (BtkWidget *entry)
{
  BtkWidget *menu;
  BtkWidget *item;
  BtkWidget *image;

  menu = btk_menu_new ();

  item = btk_image_menu_item_new_with_mnemonic ("Search by _name");
  image = btk_image_new_from_stock (BTK_STOCK_FIND, BTK_ICON_SIZE_MENU);
  btk_image_menu_item_set_image (BTK_IMAGE_MENU_ITEM (item), image);
  btk_image_menu_item_set_always_show_image (BTK_IMAGE_MENU_ITEM (item), TRUE);
  g_signal_connect (item, "activate",
                    G_CALLBACK (search_by_name), entry);
  btk_menu_shell_append (BTK_MENU_SHELL (menu), item);

  item = btk_image_menu_item_new_with_mnemonic ("Search by _description");
  image = btk_image_new_from_stock (BTK_STOCK_EDIT, BTK_ICON_SIZE_MENU);
  btk_image_menu_item_set_image (BTK_IMAGE_MENU_ITEM (item), image);
  btk_image_menu_item_set_always_show_image (BTK_IMAGE_MENU_ITEM (item), TRUE);
  g_signal_connect (item, "activate",
                    G_CALLBACK (search_by_description), entry);
  btk_menu_shell_append (BTK_MENU_SHELL (menu), item);

  item = btk_image_menu_item_new_with_mnemonic ("Search by _file name");
  image = btk_image_new_from_stock (BTK_STOCK_OPEN, BTK_ICON_SIZE_MENU);
  btk_image_menu_item_set_image (BTK_IMAGE_MENU_ITEM (item), image);
  btk_image_menu_item_set_always_show_image (BTK_IMAGE_MENU_ITEM (item), TRUE);
  g_signal_connect (item, "activate",
                    G_CALLBACK (search_by_file), entry);
  btk_menu_shell_append (BTK_MENU_SHELL (menu), item);

  btk_widget_show_all (menu);

  return menu;
}

static void
icon_press_cb (BtkEntry       *entry,
               bint            position,
               BdkEventButton *event,
               bpointer        data)
{
  if (position == BTK_ENTRY_ICON_PRIMARY)
    btk_menu_popup (BTK_MENU (menu), NULL, NULL, NULL, NULL,
                    event->button, event->time);
  else
    clear_entry (entry);
}

static void
text_changed_cb (BtkEntry   *entry,
                 BParamSpec *pspec,
                 BtkWidget  *button)
{
  bboolean has_text;

  has_text = btk_entry_get_text_length (entry) > 0;
  btk_entry_set_icon_sensitive (entry,
                                BTK_ENTRY_ICON_SECONDARY,
                                has_text);
  btk_widget_set_sensitive (button, has_text);
}

static void
activate_cb (BtkEntry  *entry,
             BtkButton *button)
{
  if (search_progress_id != 0)
    return;

  start_search (button, entry);

}

static void
search_entry_destroyed (BtkWidget  *widget)
{
  if (finish_search_id != 0)
    g_source_remove (finish_search_id);

  if (search_progress_id != 0)
    g_source_remove (search_progress_id);

  window = NULL;
}

static void
entry_populate_popup (BtkEntry *entry,
                      BtkMenu  *menu,
                      bpointer user_data)
{
  BtkWidget *item;
  BtkWidget *search_menu;
  bboolean has_text;

  has_text = btk_entry_get_text_length (entry) > 0;

  item = btk_separator_menu_item_new ();
  btk_widget_show (item);
  btk_menu_shell_append (BTK_MENU_SHELL (menu), item);

  item = btk_menu_item_new_with_mnemonic ("C_lear");
  btk_widget_show (item);
  g_signal_connect_swapped (item, "activate",
                            G_CALLBACK (clear_entry), entry);
  btk_menu_shell_append (BTK_MENU_SHELL (menu), item);
  btk_widget_set_sensitive (item, has_text);

  search_menu = create_search_menu (BTK_WIDGET (entry));
  item = btk_menu_item_new_with_label ("Search by");
  btk_widget_show (item);
  btk_menu_item_set_submenu (BTK_MENU_ITEM (item), search_menu);
  btk_menu_shell_append (BTK_MENU_SHELL (menu), item);
}

BtkWidget *
do_search_entry (BtkWidget *do_widget)
{
  BtkWidget *vbox;
  BtkWidget *hbox;
  BtkWidget *label;
  BtkWidget *entry;
  BtkWidget *find_button;
  BtkWidget *cancel_button;

  if (!window)
    {
      window = btk_dialog_new_with_buttons ("Search Entry",
                                            BTK_WINDOW (do_widget),
                                            0,
                                            BTK_STOCK_CLOSE,
                                            BTK_RESPONSE_NONE,
                                            NULL);
      btk_window_set_resizable (BTK_WINDOW (window), FALSE);

      g_signal_connect (window, "response",
                        G_CALLBACK (btk_widget_destroy), NULL);
      g_signal_connect (window, "destroy",
                        G_CALLBACK (search_entry_destroyed), &window);

      vbox = btk_vbox_new (FALSE, 5);
      btk_box_pack_start (BTK_BOX (btk_dialog_get_content_area (BTK_DIALOG (window))), vbox, TRUE, TRUE, 0);
      btk_container_set_border_width (BTK_CONTAINER (vbox), 5);

      label = btk_label_new (NULL);
      btk_label_set_markup (BTK_LABEL (label), "Search entry demo");
      btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);

      hbox = btk_hbox_new (FALSE, 10);
      btk_box_pack_start (BTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      btk_container_set_border_width (BTK_CONTAINER (hbox), 0);

      /* Create our entry */
      entry = btk_entry_new ();
      btk_box_pack_start (BTK_BOX (hbox), entry, FALSE, FALSE, 0);

      /* Create the find and cancel buttons */
      notebook = btk_notebook_new ();
      btk_notebook_set_show_tabs (BTK_NOTEBOOK (notebook), FALSE);
      btk_notebook_set_show_border (BTK_NOTEBOOK (notebook), FALSE);
      btk_box_pack_start (BTK_BOX (hbox), notebook, FALSE, FALSE, 0);

      find_button = btk_button_new_with_label ("Find");
      g_signal_connect (find_button, "clicked",
                        G_CALLBACK (start_search), entry);
      btk_notebook_append_page (BTK_NOTEBOOK (notebook), find_button, NULL);
      btk_widget_show (find_button);

      cancel_button = btk_button_new_with_label ("Cancel");
      g_signal_connect (cancel_button, "clicked",
                        G_CALLBACK (stop_search), NULL);
      btk_notebook_append_page (BTK_NOTEBOOK (notebook), cancel_button, NULL);
      btk_widget_show (cancel_button);

      /* Set up the search icon */
      search_by_name (NULL, BTK_ENTRY (entry));

      /* Set up the clear icon */
      btk_entry_set_icon_from_stock (BTK_ENTRY (entry),
                                     BTK_ENTRY_ICON_SECONDARY,
                                     BTK_STOCK_CLEAR);
      text_changed_cb (BTK_ENTRY (entry), NULL, find_button);

      g_signal_connect (entry, "icon-press",
                        G_CALLBACK (icon_press_cb), NULL);
      g_signal_connect (entry, "notify::text",
                        G_CALLBACK (text_changed_cb), find_button);
      g_signal_connect (entry, "activate",
                        G_CALLBACK (activate_cb), NULL);

      /* Create the menu */
      menu = create_search_menu (entry);
      btk_menu_attach_to_widget (BTK_MENU (menu), entry, NULL);

      /* add accessible alternatives for icon functionality */
      g_signal_connect (entry, "populate-popup",
                        G_CALLBACK (entry_populate_popup), NULL);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    {
      btk_widget_destroy (menu);
      btk_widget_destroy (window);
      window = NULL;
    }

  return window;
}
