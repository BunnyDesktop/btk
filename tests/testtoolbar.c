/* testtoolbar.c
 *
 * Copyright (C) 2002 Anders Carlsson <andersca@codefactory.se>
 * Copyright (C) 2002 James Henstridge <james@daa.com.au>
 * Copyright (C) 2003 Soeren Sandmann <sandmann@daimi.au.dk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#undef BTK_DISABLE_DEPRECATED
#include "config.h"
#include <btk/btk.h>
#include "prop-editor.h"

static void
reload_clicked (BtkWidget *widget)
{
  static BdkAtom atom_rcfiles = BDK_NONE;

  BdkEventClient sev;
  int i;
  
  if (!atom_rcfiles)
    atom_rcfiles = bdk_atom_intern("_BTK_READ_RCFILES", FALSE);

  for(i = 0; i < 5; i++)
    sev.data.l[i] = 0;
  sev.data_format = 32;
  sev.message_type = atom_rcfiles;
  bdk_event_send_clientmessage_toall ((BdkEvent *) &sev);
}

static void
change_orientation (BtkWidget *button, BtkWidget *toolbar)
{
  BtkWidget *table;
  BtkOrientation orientation;

  table = btk_widget_get_parent (toolbar);
  if (btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (button)))
    orientation = BTK_ORIENTATION_VERTICAL;
  else
    orientation = BTK_ORIENTATION_HORIZONTAL;

  g_object_ref (toolbar);
  btk_container_remove (BTK_CONTAINER (table), toolbar);
  btk_toolbar_set_orientation (BTK_TOOLBAR (toolbar), orientation);
  if (orientation == BTK_ORIENTATION_HORIZONTAL)
    {
      btk_table_attach (BTK_TABLE (table), toolbar,
			0,2, 0,1, BTK_FILL|BTK_EXPAND, BTK_FILL, 0, 0);
    }
  else
    {
      btk_table_attach (BTK_TABLE (table), toolbar,
			0,1, 0,4, BTK_FILL, BTK_FILL|BTK_EXPAND, 0, 0);
    }
  g_object_unref (toolbar);
}

static void
change_show_arrow (BtkWidget *button, BtkWidget *toolbar)
{
  btk_toolbar_set_show_arrow (BTK_TOOLBAR (toolbar),
		btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (button)));
}

static void
set_toolbar_style_toggled (BtkCheckButton *button, BtkToolbar *toolbar)
{
  BtkWidget *option_menu;
  int style;
  
  option_menu = g_object_get_data (B_OBJECT (button), "option-menu");

  if (btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (button)))
    {
      style = btk_option_menu_get_history (BTK_OPTION_MENU (option_menu));

      btk_toolbar_set_style (toolbar, style);
      btk_widget_set_sensitive (option_menu, TRUE);
    }
  else
    {
      btk_toolbar_unset_style (toolbar);
      btk_widget_set_sensitive (option_menu, FALSE);
    }
}

static void
change_toolbar_style (BtkWidget *option_menu, BtkWidget *toolbar)
{
  BtkToolbarStyle style;

  style = btk_option_menu_get_history (BTK_OPTION_MENU (option_menu));
  btk_toolbar_set_style (BTK_TOOLBAR (toolbar), style);
}

static void
set_visible_func(BtkTreeViewColumn *tree_column, BtkCellRenderer *cell,
		 BtkTreeModel *model, BtkTreeIter *iter, bpointer data)
{
  BtkToolItem *tool_item;
  bboolean visible;

  btk_tree_model_get (model, iter, 0, &tool_item, -1);

  g_object_get (tool_item, "visible", &visible, NULL);
  g_object_set (cell, "active", visible, NULL);
  g_object_unref (tool_item);
}

static void
visibile_toggled(BtkCellRendererToggle *cell, const bchar *path_str,
		 BtkTreeModel *model)
{
  BtkTreePath *path;
  BtkTreeIter iter;
  BtkToolItem *tool_item;
  bboolean visible;

  path = btk_tree_path_new_from_string (path_str);
  btk_tree_model_get_iter (model, &iter, path);

  btk_tree_model_get (model, &iter, 0, &tool_item, -1);
  g_object_get (tool_item, "visible", &visible, NULL);
  g_object_set (tool_item, "visible", !visible, NULL);
  g_object_unref (tool_item);

  btk_tree_model_row_changed (model, path, &iter);
  btk_tree_path_free (path);
}

static void
set_expand_func(BtkTreeViewColumn *tree_column, BtkCellRenderer *cell,
		BtkTreeModel *model, BtkTreeIter *iter, bpointer data)
{
  BtkToolItem *tool_item;

  btk_tree_model_get (model, iter, 0, &tool_item, -1);

  g_object_set (cell, "active", btk_tool_item_get_expand (tool_item), NULL);
  g_object_unref (tool_item);
}

static void
expand_toggled(BtkCellRendererToggle *cell, const bchar *path_str,
	       BtkTreeModel *model)
{
  BtkTreePath *path;
  BtkTreeIter iter;
  BtkToolItem *tool_item;

  path = btk_tree_path_new_from_string (path_str);
  btk_tree_model_get_iter (model, &iter, path);

  btk_tree_model_get (model, &iter, 0, &tool_item, -1);
  btk_tool_item_set_expand (tool_item, !btk_tool_item_get_expand (tool_item));
  g_object_unref (tool_item);

  btk_tree_model_row_changed (model, path, &iter);
  btk_tree_path_free (path);
}

static void
set_homogeneous_func(BtkTreeViewColumn *tree_column, BtkCellRenderer *cell,
		     BtkTreeModel *model, BtkTreeIter *iter, bpointer data)
{
  BtkToolItem *tool_item;

  btk_tree_model_get (model, iter, 0, &tool_item, -1);

  g_object_set (cell, "active", btk_tool_item_get_homogeneous (tool_item), NULL);
  g_object_unref (tool_item);
}

static void
homogeneous_toggled(BtkCellRendererToggle *cell, const bchar *path_str,
		    BtkTreeModel *model)
{
  BtkTreePath *path;
  BtkTreeIter iter;
  BtkToolItem *tool_item;

  path = btk_tree_path_new_from_string (path_str);
  btk_tree_model_get_iter (model, &iter, path);

  btk_tree_model_get (model, &iter, 0, &tool_item, -1);
  btk_tool_item_set_homogeneous (tool_item, !btk_tool_item_get_homogeneous (tool_item));
  g_object_unref (tool_item);

  btk_tree_model_row_changed (model, path, &iter);
  btk_tree_path_free (path);
}


static void
set_important_func(BtkTreeViewColumn *tree_column, BtkCellRenderer *cell,
		   BtkTreeModel *model, BtkTreeIter *iter, bpointer data)
{
  BtkToolItem *tool_item;

  btk_tree_model_get (model, iter, 0, &tool_item, -1);

  g_object_set (cell, "active", btk_tool_item_get_is_important (tool_item), NULL);
  g_object_unref (tool_item);
}

static void
important_toggled(BtkCellRendererToggle *cell, const bchar *path_str,
		  BtkTreeModel *model)
{
  BtkTreePath *path;
  BtkTreeIter iter;
  BtkToolItem *tool_item;

  path = btk_tree_path_new_from_string (path_str);
  btk_tree_model_get_iter (model, &iter, path);

  btk_tree_model_get (model, &iter, 0, &tool_item, -1);
  btk_tool_item_set_is_important (tool_item, !btk_tool_item_get_is_important (tool_item));
  g_object_unref (tool_item);

  btk_tree_model_row_changed (model, path, &iter);
  btk_tree_path_free (path);
}

static BtkListStore *
create_items_list (BtkWidget **tree_view_p)
{
  BtkWidget *tree_view;
  BtkListStore *list_store;
  BtkCellRenderer *cell;
  
  list_store = btk_list_store_new (2, BTK_TYPE_TOOL_ITEM, B_TYPE_STRING);
  
  tree_view = btk_tree_view_new_with_model (BTK_TREE_MODEL (list_store));

  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tree_view),
					       -1, "Tool Item",
					       btk_cell_renderer_text_new (),
					       "text", 1, NULL);

  cell = btk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (visibile_toggled),
		    list_store);
  btk_tree_view_insert_column_with_data_func (BTK_TREE_VIEW (tree_view),
					      -1, "Visible",
					      cell,
					      set_visible_func, NULL, NULL);

  cell = btk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (expand_toggled),
		    list_store);
  btk_tree_view_insert_column_with_data_func (BTK_TREE_VIEW (tree_view),
					      -1, "Expand",
					      cell,
					      set_expand_func, NULL, NULL);

  cell = btk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (homogeneous_toggled),
		    list_store);
  btk_tree_view_insert_column_with_data_func (BTK_TREE_VIEW (tree_view),
					      -1, "Homogeneous",
					      cell,
					      set_homogeneous_func, NULL,NULL);

  cell = btk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (important_toggled),
		    list_store);
  btk_tree_view_insert_column_with_data_func (BTK_TREE_VIEW (tree_view),
					      -1, "Important",
					      cell,
					      set_important_func, NULL,NULL);

  g_object_unref (list_store);

  *tree_view_p = tree_view;

  return list_store;
}

static void
add_item_to_list (BtkListStore *store, BtkToolItem *item, const bchar *text)
{
  BtkTreeIter iter;

  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter,
		      0, item,
		      1, text,
		      -1);
  
}

static void
bold_toggled (BtkToggleToolButton *button)
{
  g_message ("Bold toggled (active=%d)",
	     btk_toggle_tool_button_get_active (button));
}

static void
set_icon_size_toggled (BtkCheckButton *button, BtkToolbar *toolbar)
{
  BtkWidget *option_menu;
  int icon_size;
  
  option_menu = g_object_get_data (B_OBJECT (button), "option-menu");

  if (btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (button)))
    {
      icon_size = btk_option_menu_get_history (BTK_OPTION_MENU (option_menu));
      icon_size += BTK_ICON_SIZE_SMALL_TOOLBAR;

      btk_toolbar_set_icon_size (toolbar, icon_size);
      btk_widget_set_sensitive (option_menu, TRUE);
    }
  else
    {
      btk_toolbar_unset_icon_size (toolbar);
      btk_widget_set_sensitive (option_menu, FALSE);
    }
}

static void
icon_size_history_changed (BtkOptionMenu *menu, BtkToolbar *toolbar)
{
  int icon_size;

  icon_size = btk_option_menu_get_history (menu);
  icon_size += BTK_ICON_SIZE_SMALL_TOOLBAR;

  btk_toolbar_set_icon_size (toolbar, icon_size);
}

static bboolean
toolbar_drag_drop (BtkWidget *widget, BdkDragContext *context,
		   bint x, bint y, buint time, BtkWidget *label)
{
  bchar buf[32];

  g_snprintf(buf, sizeof(buf), "%d",
	     btk_toolbar_get_drop_index (BTK_TOOLBAR (widget), x, y));
  btk_label_set_label (BTK_LABEL (label), buf);

  return TRUE;
}

static BtkTargetEntry target_table[] = {
  { "application/x-toolbar-item", 0, 0 }
};

static BtkWidget *
make_prop_editor (BObject *object)
{
  BtkWidget *prop_editor = create_prop_editor (object, 0);
  btk_widget_show (prop_editor);
  return prop_editor;
}

static void
rtl_toggled (BtkCheckButton *check)
{
  if (btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (check)))
    btk_widget_set_default_direction (BTK_TEXT_DIR_RTL);
  else
    btk_widget_set_default_direction (BTK_TEXT_DIR_LTR);
}

typedef struct
{
  int x;
  int y;
} MenuPositionData;

static void
position_function (BtkMenu *menu, bint *x, bint *y, bboolean *push_in, bpointer user_data)
{
  /* Do not do this in your own code */

  MenuPositionData *position_data = user_data;

  if (x)
    *x = position_data->x;

  if (y)
    *y = position_data->y;

  if (push_in)
    *push_in = FALSE;
}

static bboolean
popup_context_menu (BtkToolbar *toolbar, bint x, bint y, bint button_number)
{
  MenuPositionData position_data;
  
  BtkMenu *menu = BTK_MENU (btk_menu_new ());
  int i;

  for (i = 0; i < 5; i++)
    {
      BtkWidget *item;
      bchar *label = g_strdup_printf ("Item _%d", i);
      item = btk_menu_item_new_with_mnemonic (label);
      btk_menu_shell_append (BTK_MENU_SHELL (menu), item);
    }
  btk_widget_show_all (BTK_WIDGET (menu));

  if (button_number != -1)
    {
      position_data.x = x;
      position_data.y = y;
      
      btk_menu_popup (menu, NULL, NULL, position_function,
		      &position_data, button_number, btk_get_current_event_time());
    }
  else
    btk_menu_popup (menu, NULL, NULL, NULL, NULL, 0, btk_get_current_event_time());

  return TRUE;
}

static BtkToolItem *drag_item = NULL;

static bboolean
toolbar_drag_motion (BtkToolbar     *toolbar,
		     BdkDragContext *context,
		     bint            x,
		     bint            y,
		     buint           time,
		     bpointer        null)
{
  bint index;
  
  if (!drag_item)
    {
      drag_item = btk_tool_button_new (NULL, "A quite long button");
      btk_object_sink (BTK_OBJECT (g_object_ref (drag_item)));
    }
  
  bdk_drag_status (context, BDK_ACTION_MOVE, time);

  index = btk_toolbar_get_drop_index (toolbar, x, y);
  
  btk_toolbar_set_drop_highlight_item (toolbar, drag_item, index);
  
  return TRUE;
}

static void
toolbar_drag_leave (BtkToolbar     *toolbar,
		    BdkDragContext *context,
		    buint           time,
		    bpointer	    null)
{
  if (drag_item)
    {
      g_object_unref (drag_item);
      drag_item = NULL;
    }
  
  btk_toolbar_set_drop_highlight_item (toolbar, NULL, 0);
}

static bboolean
timeout_cb (BtkWidget *widget)
{
  static bboolean sensitive = TRUE;
  
  sensitive = !sensitive;
  
  btk_widget_set_sensitive (widget, sensitive);
  
  return TRUE;
}

static bboolean
timeout_cb1 (BtkWidget *widget)
{
	static bboolean sensitive = TRUE;
	sensitive = !sensitive;
	btk_widget_set_sensitive (widget, sensitive);
	return TRUE;
}

bint
main (bint argc, bchar **argv)
{
  BtkWidget *window, *toolbar, *table, *treeview, *scrolled_window;
  BtkWidget *hbox, *hbox1, *hbox2, *checkbox, *option_menu, *menu;
  bint i;
  static const bchar *toolbar_styles[] = { "icons", "text", "both (vertical)",
					   "both (horizontal)" };
  BtkToolItem *item;
  BtkListStore *store;
  BtkWidget *image;
  BtkWidget *menuitem;
  BtkWidget *button;
  BtkWidget *label;
  GIcon *gicon;
  GSList *group;
  
  btk_init (&argc, &argv);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);

  g_signal_connect (window, "destroy", G_CALLBACK(btk_main_quit), NULL);

  table = btk_table_new (4, 2, FALSE);
  btk_container_add (BTK_CONTAINER (window), table);

  toolbar = btk_toolbar_new ();
  btk_table_attach (BTK_TABLE (table), toolbar,
		    0,2, 0,1, BTK_FILL|BTK_EXPAND, BTK_FILL, 0, 0);

  hbox1 = btk_hbox_new (FALSE, 3);
  btk_container_set_border_width (BTK_CONTAINER (hbox1), 5);
  btk_table_attach (BTK_TABLE (table), hbox1,
		    1,2, 1,2, BTK_FILL|BTK_EXPAND, BTK_FILL, 0, 0);

  hbox2 = btk_hbox_new (FALSE, 2);
  btk_container_set_border_width (BTK_CONTAINER (hbox2), 5);
  btk_table_attach (BTK_TABLE (table), hbox2,
		    1,2, 2,3, BTK_FILL|BTK_EXPAND, BTK_FILL, 0, 0);

  checkbox = btk_check_button_new_with_mnemonic("_Vertical");
  btk_box_pack_start (BTK_BOX (hbox1), checkbox, FALSE, FALSE, 0);
  g_signal_connect (checkbox, "toggled",
		    G_CALLBACK (change_orientation), toolbar);

  checkbox = btk_check_button_new_with_mnemonic("_Show Arrow");
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (checkbox), TRUE);
  btk_box_pack_start (BTK_BOX (hbox1), checkbox, FALSE, FALSE, 0);
  g_signal_connect (checkbox, "toggled",
		    G_CALLBACK (change_show_arrow), toolbar);

  checkbox = btk_check_button_new_with_mnemonic("_Set Toolbar Style:");
  g_signal_connect (checkbox, "toggled", G_CALLBACK (set_toolbar_style_toggled), toolbar);
  btk_box_pack_start (BTK_BOX (hbox1), checkbox, FALSE, FALSE, 0);

  option_menu = btk_combo_box_text_new ();
  btk_widget_set_sensitive (option_menu, FALSE);  
  g_object_set_data (B_OBJECT (checkbox), "option-menu", option_menu);
  
  menu = btk_menu_new();
  for (i = 0; i < G_N_ELEMENTS (toolbar_styles); i++)
    btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (option_menu), toolbar_styles[i]);
  btk_combo_box_set_active (BTK_COMBO_BOX (option_menu),
                            btk_toolbar_get_style (BTK_TOOLBAR (toolbar)));
  btk_box_pack_start (BTK_BOX (hbox2), option_menu, FALSE, FALSE, 0);
  g_signal_connect (option_menu, "changed",
		    G_CALLBACK (change_toolbar_style), toolbar);

  checkbox = btk_check_button_new_with_mnemonic("_Set Icon Size:"); 
  g_signal_connect (checkbox, "toggled", G_CALLBACK (set_icon_size_toggled), toolbar);
  btk_box_pack_start (BTK_BOX (hbox2), checkbox, FALSE, FALSE, 0);

  option_menu = btk_combo_box_text_new ();
  g_object_set_data (B_OBJECT (checkbox), "option-menu", option_menu);
  btk_widget_set_sensitive (option_menu, FALSE);
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (option_menu), "small toolbar");
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (option_menu), "large toolbar");

  btk_box_pack_start (BTK_BOX (hbox2), option_menu, FALSE, FALSE, 0);
  g_signal_connect (option_menu, "changed",
		    G_CALLBACK (icon_size_history_changed), toolbar);
  
  scrolled_window = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window),
				  BTK_POLICY_AUTOMATIC, BTK_POLICY_AUTOMATIC);
  btk_table_attach (BTK_TABLE (table), scrolled_window,
		    1,2, 3,4, BTK_FILL|BTK_EXPAND, BTK_FILL|BTK_EXPAND, 0, 0);

  store = create_items_list (&treeview);
  btk_container_add (BTK_CONTAINER (scrolled_window), treeview);
  
  item = btk_tool_button_new_from_stock (BTK_STOCK_NEW);
  btk_tool_button_set_label (BTK_TOOL_BUTTON (item), "Custom label");
  btk_tool_button_set_label (BTK_TOOL_BUTTON (item), NULL);
  add_item_to_list (store, item, "New");
  btk_toolbar_insert (BTK_TOOLBAR (toolbar), item, -1);
  bdk_threads_add_timeout (3000, (GSourceFunc) timeout_cb, item);
  btk_tool_item_set_expand (item, TRUE);

  menu = btk_menu_new ();
  for (i = 0; i < 20; i++)
    {
      char *text;
      text = g_strdup_printf ("Menuitem %d", i);
      menuitem = btk_menu_item_new_with_label (text);
      g_free (text);
      btk_widget_show (menuitem);
      btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
    }

  item = btk_menu_tool_button_new_from_stock (BTK_STOCK_OPEN);
  btk_menu_tool_button_set_menu (BTK_MENU_TOOL_BUTTON (item), menu);
  add_item_to_list (store, item, "Open");
  btk_toolbar_insert (BTK_TOOLBAR (toolbar), item, -1);
  bdk_threads_add_timeout (3000, (GSourceFunc) timeout_cb1, item);
 
  menu = btk_menu_new ();
  for (i = 0; i < 20; i++)
    {
      char *text;
      text = g_strdup_printf ("A%d", i);
      menuitem = btk_menu_item_new_with_label (text);
      g_free (text);
      btk_widget_show (menuitem);
      btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
    }

  item = btk_menu_tool_button_new_from_stock (BTK_STOCK_GO_BACK);
  btk_menu_tool_button_set_menu (BTK_MENU_TOOL_BUTTON (item), menu);
  add_item_to_list (store, item, "BackWithHistory");
  btk_toolbar_insert (BTK_TOOLBAR (toolbar), item, -1);
 
  item = btk_separator_tool_item_new ();
  add_item_to_list (store, item, "-----");    
  btk_toolbar_insert (BTK_TOOLBAR (toolbar), item, -1);
  
  item = btk_tool_button_new_from_stock (BTK_STOCK_REFRESH);
  add_item_to_list (store, item, "Refresh");
  g_signal_connect (item, "clicked", G_CALLBACK (reload_clicked), NULL);
  btk_toolbar_insert (BTK_TOOLBAR (toolbar), item, -1);

  image = btk_image_new_from_stock (BTK_STOCK_DIALOG_WARNING, BTK_ICON_SIZE_DIALOG);
  item = btk_tool_item_new ();
  btk_widget_show (image);
  btk_container_add (BTK_CONTAINER (item), image);
  add_item_to_list (store, item, "(Custom Item)");    
  btk_toolbar_insert (BTK_TOOLBAR (toolbar), item, -1);
  
  item = btk_tool_button_new_from_stock (BTK_STOCK_GO_BACK);
  add_item_to_list (store, item, "Back");    
  btk_toolbar_insert (BTK_TOOLBAR (toolbar), item, -1);

  item = btk_separator_tool_item_new ();
  add_item_to_list (store, item, "-----");  
  btk_toolbar_insert (BTK_TOOLBAR (toolbar), item, -1);
  
  item = btk_tool_button_new_from_stock (BTK_STOCK_GO_FORWARD);
  add_item_to_list (store, item, "Forward");  
  btk_toolbar_insert (BTK_TOOLBAR (toolbar), item, -1);

  item = btk_toggle_tool_button_new_from_stock (BTK_STOCK_BOLD);
  g_signal_connect (item, "toggled", G_CALLBACK (bold_toggled), NULL);
  add_item_to_list (store, item, "Bold");  
  btk_toolbar_insert (BTK_TOOLBAR (toolbar), item, -1);
  btk_widget_set_sensitive (BTK_WIDGET (item), FALSE);

  item = btk_separator_tool_item_new ();
  add_item_to_list (store, item, "-----");  
  btk_toolbar_insert (BTK_TOOLBAR (toolbar), item, -1);
  btk_tool_item_set_expand (item, TRUE);
  btk_separator_tool_item_set_draw (BTK_SEPARATOR_TOOL_ITEM (item), FALSE);
  g_assert (btk_toolbar_get_nth_item (BTK_TOOLBAR (toolbar), 0) != 0);
  
  item = btk_radio_tool_button_new_from_stock (NULL, BTK_STOCK_JUSTIFY_LEFT);
  group = btk_radio_tool_button_get_group (BTK_RADIO_TOOL_BUTTON (item));
  add_item_to_list (store, item, "Left");
  btk_toolbar_insert (BTK_TOOLBAR (toolbar), item, -1);
  
  
  item = btk_radio_tool_button_new_from_stock (group, BTK_STOCK_JUSTIFY_CENTER);
  make_prop_editor (B_OBJECT (item));

  group = btk_radio_tool_button_get_group (BTK_RADIO_TOOL_BUTTON (item));
  add_item_to_list (store, item, "Center");
  btk_toolbar_insert (BTK_TOOLBAR (toolbar), item, -1);

  item = btk_radio_tool_button_new_from_stock (group, BTK_STOCK_JUSTIFY_RIGHT);
  add_item_to_list (store, item, "Right");
  btk_toolbar_insert (BTK_TOOLBAR (toolbar), item, -1);

  item = btk_tool_button_new (btk_image_new_from_file ("apple-red.png"), "_Apple");
  add_item_to_list (store, item, "Apple");
  btk_toolbar_insert (BTK_TOOLBAR (toolbar), item, -1);
  btk_tool_button_set_use_underline (BTK_TOOL_BUTTON (item), TRUE);

  gicon = g_content_type_get_icon ("video/ogg");
  image = btk_image_new_from_gicon (gicon, BTK_ICON_SIZE_LARGE_TOOLBAR);
  g_object_unref (gicon);
  item = btk_tool_button_new (image, "Video");
  add_item_to_list (store, item, "Video");
  btk_toolbar_insert (BTK_TOOLBAR (toolbar), item, -1);

  image = btk_image_new_from_icon_name ("utility-terminal", BTK_ICON_SIZE_LARGE_TOOLBAR);
  item = btk_tool_button_new (image, "Terminal");
  add_item_to_list (store, item, "Terminal");
  btk_toolbar_insert (BTK_TOOLBAR (toolbar), item, -1);

  hbox = btk_hbox_new (FALSE, 5);
  btk_container_set_border_width (BTK_CONTAINER (hbox), 5);
  btk_table_attach (BTK_TABLE (table), hbox,
		    1,2, 4,5, BTK_FILL|BTK_EXPAND, BTK_FILL, 0, 0);

  button = btk_button_new_with_label ("Drag me to the toolbar");
  btk_box_pack_start (BTK_BOX (hbox), button, FALSE, FALSE, 0);

  label = btk_label_new ("Drop index:");
  btk_box_pack_start (BTK_BOX (hbox), label, FALSE, FALSE, 0);

  label = btk_label_new ("");
  btk_box_pack_start (BTK_BOX (hbox), label, FALSE, FALSE, 0);

  checkbox = btk_check_button_new_with_mnemonic("_Right to left");
  if (btk_widget_get_default_direction () == BTK_TEXT_DIR_RTL)
    btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (checkbox), TRUE);
  else
    btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (checkbox), FALSE);
  g_signal_connect (checkbox, "toggled", G_CALLBACK (rtl_toggled), NULL);

  btk_box_pack_end (BTK_BOX (hbox), checkbox, FALSE, FALSE, 0);
  
  btk_drag_source_set (button, BDK_BUTTON1_MASK,
		       target_table, G_N_ELEMENTS (target_table),
		       BDK_ACTION_MOVE);
  btk_drag_dest_set (toolbar, BTK_DEST_DEFAULT_DROP,
		     target_table, G_N_ELEMENTS (target_table),
		     BDK_ACTION_MOVE);
  g_signal_connect (toolbar, "drag_motion",
		    G_CALLBACK (toolbar_drag_motion), NULL);
  g_signal_connect (toolbar, "drag_leave",
		    G_CALLBACK (toolbar_drag_leave), NULL);
  g_signal_connect (toolbar, "drag_drop",
		    G_CALLBACK (toolbar_drag_drop), label);

  btk_widget_show_all (window);

  make_prop_editor (B_OBJECT (toolbar));

  g_signal_connect (window, "delete_event", G_CALLBACK (btk_main_quit), NULL);
  
  g_signal_connect (toolbar, "popup_context_menu", G_CALLBACK (popup_context_menu), NULL);
  
  btk_main ();
  
  return 0;
}
