/* Clipboard
 *
 * BtkClipboard is used for clipboard handling. This demo shows how to
 * copy and paste text to and from the clipboard.
 *
 * It also shows how to transfer images via the clipboard or via
 * drag-and-drop, and how to make clipboard contents persist after
 * the application exits. Clipboard persistence requires a clipboard
 * manager to run.
 */

#include <btk/btk.h>
#include <string.h>

static BtkWidget *window = NULL;

void
copy_button_clicked (BtkWidget *button,
                     gpointer   user_data)
{
  BtkWidget *entry;
  BtkClipboard *clipboard;

  entry = BTK_WIDGET (user_data);

  /* Get the clipboard object */
  clipboard = btk_widget_get_clipboard (entry,
                                        BDK_SELECTION_CLIPBOARD);

  /* Set clipboard text */
  btk_clipboard_set_text (clipboard, btk_entry_get_text (BTK_ENTRY (entry)), -1);
}

void
paste_received (BtkClipboard *clipboard,
                const gchar  *text,
                gpointer      user_data)
{
  BtkWidget *entry;

  entry = BTK_WIDGET (user_data);

  /* Set the entry text */
  if(text)
    btk_entry_set_text (BTK_ENTRY (entry), text);
}

void
paste_button_clicked (BtkWidget *button,
                     gpointer   user_data)
{
  BtkWidget *entry;
  BtkClipboard *clipboard;

  entry = BTK_WIDGET (user_data);

  /* Get the clipboard object */
  clipboard = btk_widget_get_clipboard (entry,
                                        BDK_SELECTION_CLIPBOARD);

  /* Request the contents of the clipboard, contents_received will be
     called when we do get the contents.
   */
  btk_clipboard_request_text (clipboard,
                              paste_received, entry);
}

static BdkPixbuf *
get_image_pixbuf (BtkImage *image)
{
  gchar *stock_id;
  BtkIconSize size;

  switch (btk_image_get_storage_type (image))
    {
    case BTK_IMAGE_PIXBUF:
      return g_object_ref (btk_image_get_pixbuf (image));
    case BTK_IMAGE_STOCK:
      btk_image_get_stock (image, &stock_id, &size);
      return btk_widget_render_icon (BTK_WIDGET (image),
                                     stock_id, size, NULL);
    default:
      g_warning ("Image storage type %d not handled",
                 btk_image_get_storage_type (image));
      return NULL;
    }
}

static void
drag_begin (BtkWidget      *widget,
            BdkDragContext *context,
            gpointer        data)
{
  BdkPixbuf *pixbuf;

  pixbuf = get_image_pixbuf (BTK_IMAGE (data));
  btk_drag_set_icon_pixbuf (context, pixbuf, -2, -2);
  g_object_unref (pixbuf);
}

void
drag_data_get  (BtkWidget        *widget,
                BdkDragContext   *context,
                BtkSelectionData *selection_data,
                guint             info,
                guint             time,
                gpointer          data)
{
  BdkPixbuf *pixbuf;

  pixbuf = get_image_pixbuf (BTK_IMAGE (data));
  btk_selection_data_set_pixbuf (selection_data, pixbuf);
  g_object_unref (pixbuf);
}

static void
drag_data_received (BtkWidget        *widget,
                    BdkDragContext   *context,
                    gint              x,
                    gint              y,
                    BtkSelectionData *selection_data,
                    guint             info,
                    guint32           time,
                    gpointer          data)
{
  BdkPixbuf *pixbuf;

  if (btk_selection_data_get_length (selection_data) > 0)
    {
      pixbuf = btk_selection_data_get_pixbuf (selection_data);
      btk_image_set_from_pixbuf (BTK_IMAGE (data), pixbuf);
      g_object_unref (pixbuf);
    }
}

static void
copy_image (BtkMenuItem *item,
            gpointer     data)
{
  BtkClipboard *clipboard;
  BdkPixbuf *pixbuf;

  clipboard = btk_clipboard_get (BDK_SELECTION_CLIPBOARD);
  pixbuf = get_image_pixbuf (BTK_IMAGE (data));

  btk_clipboard_set_image (clipboard, pixbuf);
  g_object_unref (pixbuf);
}

static void
paste_image (BtkMenuItem *item,
             gpointer     data)
{
  BtkClipboard *clipboard;
  BdkPixbuf *pixbuf;

  clipboard = btk_clipboard_get (BDK_SELECTION_CLIPBOARD);
  pixbuf = btk_clipboard_wait_for_image (clipboard);

  if (pixbuf)
    {
      btk_image_set_from_pixbuf (BTK_IMAGE (data), pixbuf);
      g_object_unref (pixbuf);
    }
}

static gboolean
button_press (BtkWidget      *widget,
              BdkEventButton *button,
              gpointer        data)
{
  BtkWidget *menu;
  BtkWidget *item;

  if (button->button != 3)
    return FALSE;

  menu = btk_menu_new ();

  item = btk_image_menu_item_new_from_stock (BTK_STOCK_COPY, NULL);
  g_signal_connect (item, "activate", G_CALLBACK (copy_image), data);
  btk_widget_show (item);
  btk_menu_shell_append (BTK_MENU_SHELL (menu), item);

  item = btk_image_menu_item_new_from_stock (BTK_STOCK_PASTE, NULL);
  g_signal_connect (item, "activate", G_CALLBACK (paste_image), data);
  btk_widget_show (item);
  btk_menu_shell_append (BTK_MENU_SHELL (menu), item);

  btk_menu_popup (BTK_MENU (menu), NULL, NULL, NULL, NULL, 3, button->time);
  return TRUE;
}

BtkWidget *
do_clipboard (BtkWidget *do_widget)
{
  if (!window)
    {
      BtkWidget *vbox, *hbox;
      BtkWidget *label;
      BtkWidget *entry, *button;
      BtkWidget *ebox, *image;
      BtkClipboard *clipboard;

      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
                             btk_widget_get_screen (do_widget));
      btk_window_set_title (BTK_WINDOW (window), "Clipboard demo");

      g_signal_connect (window, "destroy",
                        G_CALLBACK (btk_widget_destroyed), &window);

      vbox = btk_vbox_new (FALSE, 0);
      btk_container_set_border_width (BTK_CONTAINER (vbox), 8);

      btk_container_add (BTK_CONTAINER (window), vbox);

      label = btk_label_new ("\"Copy\" will copy the text\nin the entry to the clipboard");

      btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);

      hbox = btk_hbox_new (FALSE, 4);
      btk_container_set_border_width (BTK_CONTAINER (hbox), 8);
      btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, FALSE, 0);

      /* Create the first entry */
      entry = btk_entry_new ();
      btk_box_pack_start (BTK_BOX (hbox), entry, TRUE, TRUE, 0);

      /* Create the button */
      button = btk_button_new_from_stock (BTK_STOCK_COPY);
      btk_box_pack_start (BTK_BOX (hbox), button, FALSE, FALSE, 0);
      g_signal_connect (button, "clicked",
                        G_CALLBACK (copy_button_clicked), entry);

      label = btk_label_new ("\"Paste\" will paste the text from the clipboard to the entry");
      btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);

      hbox = btk_hbox_new (FALSE, 4);
      btk_container_set_border_width (BTK_CONTAINER (hbox), 8);
      btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, FALSE, 0);

      /* Create the second entry */
      entry = btk_entry_new ();
      btk_box_pack_start (BTK_BOX (hbox), entry, TRUE, TRUE, 0);

      /* Create the button */
      button = btk_button_new_from_stock (BTK_STOCK_PASTE);
      btk_box_pack_start (BTK_BOX (hbox), button, FALSE, FALSE, 0);
      g_signal_connect (button, "clicked",
                        G_CALLBACK (paste_button_clicked), entry);

      label = btk_label_new ("Images can be transferred via the clipboard, too");
      btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);

      hbox = btk_hbox_new (FALSE, 4);
      btk_container_set_border_width (BTK_CONTAINER (hbox), 8);
      btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, FALSE, 0);

      /* Create the first image */
      image = btk_image_new_from_stock (BTK_STOCK_DIALOG_WARNING,
                                        BTK_ICON_SIZE_BUTTON);
      ebox = btk_event_box_new ();
      btk_container_add (BTK_CONTAINER (ebox), image);
      btk_container_add (BTK_CONTAINER (hbox), ebox);

      /* make ebox a drag source */
      btk_drag_source_set (ebox, BDK_BUTTON1_MASK, NULL, 0, BDK_ACTION_COPY);
      btk_drag_source_add_image_targets (ebox);
      g_signal_connect (ebox, "drag-begin",
                        G_CALLBACK (drag_begin), image);
      g_signal_connect (ebox, "drag-data-get",
                        G_CALLBACK (drag_data_get), image);

      /* accept drops on ebox */
      btk_drag_dest_set (ebox, BTK_DEST_DEFAULT_ALL,
                         NULL, 0, BDK_ACTION_COPY);
      btk_drag_dest_add_image_targets (ebox);
      g_signal_connect (ebox, "drag-data-received",
                        G_CALLBACK (drag_data_received), image);

      /* context menu on ebox */
      g_signal_connect (ebox, "button-press-event",
                        G_CALLBACK (button_press), image);

      /* Create the second image */
      image = btk_image_new_from_stock (BTK_STOCK_STOP,
                                        BTK_ICON_SIZE_BUTTON);
      ebox = btk_event_box_new ();
      btk_container_add (BTK_CONTAINER (ebox), image);
      btk_container_add (BTK_CONTAINER (hbox), ebox);

      /* make ebox a drag source */
      btk_drag_source_set (ebox, BDK_BUTTON1_MASK, NULL, 0, BDK_ACTION_COPY);
      btk_drag_source_add_image_targets (ebox);
      g_signal_connect (ebox, "drag-begin",
                        G_CALLBACK (drag_begin), image);
      g_signal_connect (ebox, "drag-data-get",
                        G_CALLBACK (drag_data_get), image);

      /* accept drops on ebox */
      btk_drag_dest_set (ebox, BTK_DEST_DEFAULT_ALL,
                         NULL, 0, BDK_ACTION_COPY);
      btk_drag_dest_add_image_targets (ebox);
      g_signal_connect (ebox, "drag-data-received",
                        G_CALLBACK (drag_data_received), image);

      /* context menu on ebox */
      g_signal_connect (ebox, "button-press-event",
                        G_CALLBACK (button_press), image);

      /* tell the clipboard manager to make the data persistent */
      clipboard = btk_clipboard_get (BDK_SELECTION_CLIPBOARD);
      btk_clipboard_set_can_store (clipboard, NULL, 0);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    {
      btk_widget_destroy (window);
      window = NULL;
    }

  return window;
}
