#include <btk/btk.h>
#include <stdio.h>
#include "prop-editor.h"

static bboolean
delete_event_cb (BtkWidget *editor,
                 bint       response,
                 bpointer   user_data)
{
  btk_widget_hide (editor);

  return TRUE;
}

static void
properties_cb (BtkWidget *button,
               BObject   *entry)
{
  BtkWidget *editor;

  editor = g_object_get_data (entry, "properties-dialog");

  if (editor == NULL)
    {
      editor = create_prop_editor (B_OBJECT (entry), B_TYPE_INVALID);
      btk_container_set_border_width (BTK_CONTAINER (editor), 12);
      btk_window_set_transient_for (BTK_WINDOW (editor),
                                    BTK_WINDOW (btk_widget_get_toplevel (button)));
      g_signal_connect (editor, "delete-event", G_CALLBACK (delete_event_cb), NULL);
      g_object_set_data (entry, "properties-dialog", editor);
    }

  btk_window_present (BTK_WINDOW (editor));
}

static void
clear_pressed (BtkEntry *entry, bint icon, BdkEvent *event, bpointer data)
{
   if (icon == BTK_ENTRY_ICON_SECONDARY)
     btk_entry_set_text (entry, "");
}

static void
drag_begin_cb (BtkWidget      *widget,
               BdkDragContext *context,
               bpointer        user_data)
{
  bint pos;

  pos = btk_entry_get_current_icon_drag_source (BTK_ENTRY (widget));
  if (pos != -1)
    btk_drag_set_icon_stock (context, BTK_STOCK_INFO, 2, 2);

  g_print ("drag begin %d\n", pos);
}

static void
drag_data_get_cb (BtkWidget        *widget,
                  BdkDragContext   *context,
                  BtkSelectionData *data,
                  buint             info,
                  buint             time,
                  bpointer          user_data)
{
  bint pos;

  pos = btk_entry_get_current_icon_drag_source (BTK_ENTRY (widget));

  if (pos == BTK_ENTRY_ICON_PRIMARY)
    {
#if 0
      bint start, end;
      
      if (btk_editable_get_selection_bounds (BTK_EDITABLE (widget), &start, &end))
        {
          bchar *str;
          
          str = btk_editable_get_chars (BTK_EDITABLE (widget), start, end);
          btk_selection_data_set_text (data, str, -1);
          g_free (str);
        }
#else
      btk_selection_data_set_text (data, "XXX", -1);
#endif
    }
}

int
main (int argc, char **argv)
{
  BtkWidget *window;
  BtkWidget *table;
  BtkWidget *label;
  BtkWidget *entry;
  BtkWidget *button;
  GIcon *icon;
  BtkTargetList *tlist;

  btk_init (&argc, &argv);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_title (BTK_WINDOW (window), "Btk Entry Icons Test");
  btk_container_set_border_width (BTK_CONTAINER (window), 12);

  g_signal_connect (B_OBJECT (window), "destroy",
		    G_CALLBACK (btk_main_quit), NULL);

  table = btk_table_new (2, 4, FALSE);
  btk_container_add (BTK_CONTAINER (window), table);
  btk_table_set_row_spacings (BTK_TABLE (table), 6);
  btk_table_set_col_spacings (BTK_TABLE (table), 6);

  /*
   * Open File - Sets the icon using a GIcon
   */
  label = btk_label_new ("Open File:");
  btk_table_attach (BTK_TABLE (table), label, 0, 1, 0, 1,
		    BTK_FILL, BTK_FILL, 0, 0);
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);

  entry = btk_entry_new ();
  btk_table_attach (BTK_TABLE (table), entry, 1, 2, 0, 1,
		    BTK_FILL | BTK_EXPAND, BTK_FILL, 0, 0);

  icon = g_themed_icon_new ("folder");
  g_themed_icon_append_name (G_THEMED_ICON (icon), "btk-directory");

  btk_entry_set_icon_from_gicon (BTK_ENTRY (entry),
				 BTK_ENTRY_ICON_PRIMARY,
				 icon);
  btk_entry_set_icon_sensitive (BTK_ENTRY (entry),
			        BTK_ENTRY_ICON_PRIMARY,
				FALSE);

  btk_entry_set_icon_tooltip_text (BTK_ENTRY (entry),
				   BTK_ENTRY_ICON_PRIMARY,
				   "Open a file");

  button = btk_button_new_with_label ("Properties");
  btk_table_attach (BTK_TABLE (table), button, 2, 3, 0, 1,
		    BTK_FILL, BTK_FILL, 0, 0);
  g_signal_connect (button, "clicked", 
                    G_CALLBACK (properties_cb), entry);                    

  
  /*
   * Save File - sets the icon using a stock id.
   */
  label = btk_label_new ("Save File:");
  btk_table_attach (BTK_TABLE (table), label, 0, 1, 1, 2,
		    BTK_FILL, BTK_FILL, 0, 0);
  btk_misc_set_alignment (BTK_MISC(label), 0.0, 0.5);

  entry = btk_entry_new ();
  btk_table_attach (BTK_TABLE (table), entry, 1, 2, 1, 2,
		    BTK_FILL | BTK_EXPAND, BTK_FILL, 0, 0);
  btk_entry_set_text (BTK_ENTRY (entry), "‏Right-to-left");
  btk_widget_set_direction (entry, BTK_TEXT_DIR_RTL);
  
  btk_entry_set_icon_from_stock (BTK_ENTRY (entry),
				 BTK_ENTRY_ICON_PRIMARY,
				 BTK_STOCK_SAVE);
  btk_entry_set_icon_tooltip_text (BTK_ENTRY (entry),
				   BTK_ENTRY_ICON_PRIMARY,
				   "Save a file");
  tlist = btk_target_list_new (NULL, 0);
  btk_target_list_add_text_targets (tlist, 0);
  btk_entry_set_icon_drag_source (BTK_ENTRY (entry),
                                  BTK_ENTRY_ICON_PRIMARY,
                                  tlist, BDK_ACTION_COPY); 
  g_signal_connect_after (entry, "drag-begin", 
                          G_CALLBACK (drag_begin_cb), NULL);
  g_signal_connect (entry, "drag-data-get", 
                    G_CALLBACK (drag_data_get_cb), NULL);
  btk_target_list_unref (tlist);

  button = btk_button_new_with_label ("Properties");
  btk_table_attach (BTK_TABLE (table), button, 2, 3, 1, 2,
		    BTK_FILL, BTK_FILL, 0, 0);
  g_signal_connect (button, "clicked", 
                    G_CALLBACK (properties_cb), entry);                    

  /*
   * Search - Uses a helper function
   */
  label = btk_label_new ("Search:");
  btk_table_attach (BTK_TABLE (table), label, 0, 1, 2, 3,
		    BTK_FILL, BTK_FILL, 0, 0);
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);

  entry = btk_entry_new ();
  btk_table_attach (BTK_TABLE (table), entry, 1, 2, 2, 3,
		    BTK_FILL | BTK_EXPAND, BTK_FILL, 0, 0);

  btk_entry_set_icon_from_stock (BTK_ENTRY (entry),
				 BTK_ENTRY_ICON_PRIMARY,
				 BTK_STOCK_FIND);

  btk_entry_set_icon_from_stock (BTK_ENTRY (entry),
				 BTK_ENTRY_ICON_SECONDARY,
				 BTK_STOCK_CLEAR);

  g_signal_connect (entry, "icon-press", G_CALLBACK (clear_pressed), NULL);

  button = btk_button_new_with_label ("Properties");
  btk_table_attach (BTK_TABLE (table), button, 2, 3, 2, 3,
		    BTK_FILL, BTK_FILL, 0, 0);
  g_signal_connect (button, "clicked", 
                    G_CALLBACK (properties_cb), entry);                    

  /*
   * Password - Sets the icon using a stock id
   */
  label = btk_label_new ("Password:");
  btk_table_attach (BTK_TABLE (table), label, 0, 1, 3, 4,
		    BTK_FILL, BTK_FILL, 0, 0);
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);

  entry = btk_entry_new ();
  btk_table_attach (BTK_TABLE (table), entry, 1, 2, 3, 4,
		    BTK_FILL | BTK_EXPAND, BTK_FILL, 0, 0);
  btk_entry_set_visibility (BTK_ENTRY (entry), FALSE);

  btk_entry_set_icon_from_stock (BTK_ENTRY (entry),
				 BTK_ENTRY_ICON_PRIMARY,
				 BTK_STOCK_DIALOG_AUTHENTICATION);

  btk_entry_set_icon_activatable (BTK_ENTRY (entry),
				  BTK_ENTRY_ICON_PRIMARY,
				  FALSE);

  button = btk_button_new_with_label ("Properties");
  btk_table_attach (BTK_TABLE (table), button, 2, 3, 3, 4,
		    BTK_FILL, BTK_FILL, 0, 0);
  g_signal_connect (button, "clicked", 
                    G_CALLBACK (properties_cb), entry);                    

  /* Name - Does not set any icons. */
  label = btk_label_new ("Name:");
  btk_table_attach (BTK_TABLE (table), label, 0, 1, 4, 5,
		    BTK_FILL, BTK_FILL, 0, 0);
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);

  entry = btk_entry_new ();
  btk_table_attach (BTK_TABLE (table), entry, 1, 2, 4, 5,
		    BTK_FILL | BTK_EXPAND, BTK_FILL, 0, 0);

  button = btk_button_new_with_label ("Properties");
  btk_table_attach (BTK_TABLE (table), button, 2, 3, 4, 5,
		    BTK_FILL, BTK_FILL, 0, 0);
  g_signal_connect (button, "clicked", 
                    G_CALLBACK (properties_cb), entry);                    

  btk_widget_show_all (window);

  btk_main();

  return 0;
}
