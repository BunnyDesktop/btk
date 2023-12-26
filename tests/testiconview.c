/* testiconview.c
 * Copyright (C) 2002  Anders Carlsson <andersca@gnu.org>
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

#include <btk/btk.h>
#include <sys/types.h>
#include <string.h>
#include "prop-editor.h"

#define NUMBER_OF_ITEMS   10
#define SOME_ITEMS       100
#define MANY_ITEMS     10000

static void
fill_model (BtkTreeModel *model)
{
  BdkPixbuf *pixbuf;
  int i;
  char *str, *str2;
  BtkTreeIter iter;
  BtkListStore *store = BTK_LIST_STORE (model);
  gint32 size;
  
  pixbuf = bdk_pixbuf_new_from_file ("bunny-textfile.png", NULL);

  i = 0;
  
  btk_list_store_prepend (store, &iter);

  btk_list_store_set (store, &iter,
		      0, pixbuf,
		      1, "Really really\nreally really loooooooooong item name",
		      2, 0,
		      3, "This is a <b>Test</b> of <i>markup</i>",
		      4, TRUE,
		      -1);

  while (i < NUMBER_OF_ITEMS - 1)
    {
      BdkPixbuf *pb;
      size = g_random_int_range (20, 70);
      pb = bdk_pixbuf_scale_simple (pixbuf, size, size, BDK_INTERP_NEAREST);

      str = g_strdup_printf ("Icon %d", i);
      str2 = g_strdup_printf ("Icon <b>%d</b>", i);	
      btk_list_store_prepend (store, &iter);
      btk_list_store_set (store, &iter,
			  0, pb,
			  1, str,
			  2, i,
			  3, str2,
			  4, TRUE,
			  -1);
      g_free (str);
      g_free (str2);
      i++;
    }
  
  //  btk_tree_sortable_set_sort_column_id (BTK_TREE_SORTABLE (store), 2, BTK_SORT_ASCENDING);
}

static BtkTreeModel *
create_model (void)
{
  BtkListStore *store;
  
  store = btk_list_store_new (5, BDK_TYPE_PIXBUF, B_TYPE_STRING, B_TYPE_INT, B_TYPE_STRING, B_TYPE_BOOLEAN);

  return BTK_TREE_MODEL (store);
}


static void
foreach_selected_remove (BtkWidget *button, BtkIconView *icon_list)
{
  BtkTreeIter iter;
  BtkTreeModel *model;

  GList *list, *selected;

  selected = btk_icon_view_get_selected_items (icon_list);
  model = btk_icon_view_get_model (icon_list);
  
  for (list = selected; list; list = list->next)
    {
      BtkTreePath *path = list->data;

      btk_tree_model_get_iter (model, &iter, path);
      btk_list_store_remove (BTK_LIST_STORE (model), &iter);
      
      btk_tree_path_free (path);
    } 
  
  g_list_free (selected);
}


static void
swap_rows (BtkWidget *button, BtkIconView *icon_list)
{
  BtkTreeIter iter, iter2;
  BtkTreeModel *model;

  model = btk_icon_view_get_model (icon_list);
  btk_tree_sortable_set_sort_column_id (BTK_TREE_SORTABLE (model), -2, BTK_SORT_ASCENDING);

  btk_tree_model_get_iter_first (model, &iter);
  iter2 = iter;
  btk_tree_model_iter_next (model, &iter2);
  btk_list_store_swap (BTK_LIST_STORE (model), &iter, &iter2);
}

static void
add_n_items (BtkIconView *icon_list, gint n)
{
  static gint count = NUMBER_OF_ITEMS;

  BtkTreeIter iter;
  BtkListStore *store;
  BdkPixbuf *pixbuf;
  gchar *str, *str2;
  gint i;

  store = BTK_LIST_STORE (btk_icon_view_get_model (icon_list));
  pixbuf = bdk_pixbuf_new_from_file ("bunny-textfile.png", NULL);


  for (i = 0; i < n; i++)
    {
      str = g_strdup_printf ("Icon %d", count);
      str2 = g_strdup_printf ("Icon <b>%d</b>", count);	
      btk_list_store_prepend (store, &iter);
      btk_list_store_set (store, &iter,
			  0, pixbuf,
			  1, str,
			  2, i,
			  3, str2,
			  -1);
      g_free (str);
      g_free (str2);
      count++;
    }
}

static void
add_some (BtkWidget *button, BtkIconView *icon_list)
{
  add_n_items (icon_list, SOME_ITEMS);
}

static void
add_many (BtkWidget *button, BtkIconView *icon_list)
{
  add_n_items (icon_list, MANY_ITEMS);
}

static void
add_large (BtkWidget *button, BtkIconView *icon_list)
{
  BtkListStore *store;
  BtkTreeIter iter;

  BdkPixbuf *pixbuf, *pb;
  gchar *str;

  store = BTK_LIST_STORE (btk_icon_view_get_model (icon_list));
  pixbuf = bdk_pixbuf_new_from_file ("bunny-textfile.png", NULL);

  pb = bdk_pixbuf_scale_simple (pixbuf, 
				2 * bdk_pixbuf_get_width (pixbuf),
				2 * bdk_pixbuf_get_height (pixbuf),
				BDK_INTERP_BILINEAR);

  str = g_strdup_printf ("Some really long text");
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter,
		      0, pb,
		      1, str,
		      2, 0,
		      3, str,
		      -1);
  g_object_unref (pb);
  g_free (str);
  
  pb = bdk_pixbuf_scale_simple (pixbuf, 
				3 * bdk_pixbuf_get_width (pixbuf),
				3 * bdk_pixbuf_get_height (pixbuf),
				BDK_INTERP_BILINEAR);

  str = g_strdup ("see how long text behaves when placed underneath "
		  "an oversized icon which would allow for long lines");
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter,
		      0, pb,
		      1, str,
		      2, 1,
		      3, str,
		      -1);
  g_object_unref (pb);
  g_free (str);

  pb = bdk_pixbuf_scale_simple (pixbuf, 
				3 * bdk_pixbuf_get_width (pixbuf),
				3 * bdk_pixbuf_get_height (pixbuf),
				BDK_INTERP_BILINEAR);

  str = g_strdup ("short text");
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter,
		      0, pb,
		      1, str,
		      2, 2,
		      3, str,
		      -1);
  g_object_unref (pb);
  g_free (str);

  g_object_unref (pixbuf);
}

static void
select_all (BtkWidget *button, BtkIconView *icon_list)
{
  btk_icon_view_select_all (icon_list);
}

static void
select_nonexisting (BtkWidget *button, BtkIconView *icon_list)
{  
  BtkTreePath *path = btk_tree_path_new_from_indices (999999, -1);
  btk_icon_view_select_path (icon_list, path);
  btk_tree_path_free (path);
}

static void
unselect_all (BtkWidget *button, BtkIconView *icon_list)
{
  btk_icon_view_unselect_all (icon_list);
}

static void
selection_changed (BtkIconView *icon_list)
{
  g_print ("Selection changed!\n");
}

typedef struct {
  BtkIconView     *icon_list;
  BtkTreePath     *path;
} ItemData;

static void
free_item_data (ItemData *data)
{
  btk_tree_path_free (data->path);
  g_free (data);
}

static void
item_activated (BtkIconView *icon_view,
		BtkTreePath *path)
{
  BtkTreeIter iter;
  BtkTreeModel *model;
  gchar *text;

  model = btk_icon_view_get_model (icon_view);
  btk_tree_model_get_iter (model, &iter, path);

  btk_tree_model_get (model, &iter, 1, &text, -1);
  g_print ("Item activated, text is %s\n", text);
  g_free (text);
  
}

static void
toggled (BtkCellRendererToggle *cell,
	 gchar                 *path_string,
	 gpointer               data)
{
  BtkTreeModel *model = BTK_TREE_MODEL (data);
  BtkTreeIter iter;
  BtkTreePath *path = btk_tree_path_new_from_string (path_string);
  gboolean value;

  btk_tree_model_get_iter (model, &iter, path);
  btk_tree_model_get (model, &iter, 4, &value, -1);

  value = !value;
  btk_list_store_set (BTK_LIST_STORE (model), &iter, 4, value, -1);

  btk_tree_path_free (path);
}

static void
edited (BtkCellRendererText *cell,
	gchar               *path_string,
	gchar               *new_text,
	gpointer             data)
{
  BtkTreeModel *model = BTK_TREE_MODEL (data);
  BtkTreeIter iter;
  BtkTreePath *path = btk_tree_path_new_from_string (path_string);

  btk_tree_model_get_iter (model, &iter, path);
  btk_list_store_set (BTK_LIST_STORE (model), &iter, 1, new_text, -1);

  btk_tree_path_free (path);
}

static void
item_cb (BtkWidget *menuitem,
	 ItemData  *data)
{
  item_activated (data->icon_list, data->path);
}

static void
do_popup_menu (BtkWidget      *icon_list, 
	       BdkEventButton *event)
{
  BtkIconView *icon_view = BTK_ICON_VIEW (icon_list); 
  BtkWidget *menu;
  BtkWidget *menuitem;
  BtkTreePath *path = NULL;
  int button, event_time;
  ItemData *data;
  GList *list;

  if (event)
    path = btk_icon_view_get_path_at_pos (icon_view, event->x, event->y);
  else
    {
      list = btk_icon_view_get_selected_items (icon_view);

      if (list)
        {
          path = (BtkTreePath*)list->data;
          g_list_foreach (list->next, (GFunc) btk_tree_path_free, NULL);
          g_list_free (list);
        }
    }

  if (!path)
    return;

  menu = btk_menu_new ();

  data = g_new0 (ItemData, 1);
  data->icon_list = icon_view;
  data->path = path;
  g_object_set_data_full (B_OBJECT (menu), "item-path", data, (GDestroyNotify)free_item_data);

  menuitem = btk_menu_item_new_with_label ("Activate");
  btk_widget_show (menuitem);
  btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
  g_signal_connect (menuitem, "activate", G_CALLBACK (item_cb), data);

  if (event)
    {
      button = event->button;
      event_time = event->time;
    }
  else
    {
      button = 0;
      event_time = btk_get_current_event_time ();
    }

  btk_menu_popup (BTK_MENU (menu), NULL, NULL, NULL, NULL, 
                  button, event_time);
}
	

static gboolean
button_press_event_handler (BtkWidget      *widget, 
			    BdkEventButton *event)
{
  /* Ignore double-clicks and triple-clicks */
  if (event->button == 3 && event->type == BDK_BUTTON_PRESS)
    {
      do_popup_menu (widget, event);
      return TRUE;
    }

  return FALSE;
}

static gboolean
popup_menu_handler (BtkWidget *widget)
{
  do_popup_menu (widget, NULL);
  return TRUE;
}

static const BtkTargetEntry item_targets[] = {
  { "BTK_TREE_MODEL_ROW", BTK_TARGET_SAME_APP, 0 }
};
	
gint
main (gint argc, gchar **argv)
{
  BtkWidget *paned, *tv;
  BtkWidget *window, *icon_list, *scrolled_window;
  BtkWidget *vbox, *bbox;
  BtkWidget *button;
  BtkWidget *prop_editor;
  BtkTreeModel *model;
  BtkCellRenderer *cell;
  BtkTreeViewColumn *tvc;
  
  btk_init (&argc, &argv);

  /* to test rtl layout, set RTL=1 in the environment */
  if (g_getenv ("RTL"))
    btk_widget_set_default_direction (BTK_TEXT_DIR_RTL);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_default_size (BTK_WINDOW (window), 700, 400);

  vbox = btk_vbox_new (FALSE, 0);
  btk_container_add (BTK_CONTAINER (window), vbox);

  paned = btk_hpaned_new ();
  btk_box_pack_start (BTK_BOX (vbox), paned, TRUE, TRUE, 0);

  icon_list = btk_icon_view_new ();
  btk_icon_view_set_selection_mode (BTK_ICON_VIEW (icon_list), BTK_SELECTION_MULTIPLE);

  tv = btk_tree_view_new ();
  tvc = btk_tree_view_column_new ();
  btk_tree_view_append_column (BTK_TREE_VIEW (tv), tvc);

  g_signal_connect_after (icon_list, "button_press_event",
			  G_CALLBACK (button_press_event_handler), NULL);
  g_signal_connect (icon_list, "selection_changed",
		    G_CALLBACK (selection_changed), NULL);
  g_signal_connect (icon_list, "popup_menu",
		    G_CALLBACK (popup_menu_handler), NULL);

  g_signal_connect (icon_list, "item_activated",
		    G_CALLBACK (item_activated), NULL);
  
  model = create_model ();
  btk_icon_view_set_model (BTK_ICON_VIEW (icon_list), model);
  btk_tree_view_set_model (BTK_TREE_VIEW (tv), model);
  fill_model (model);

#if 0

  btk_icon_view_set_pixbuf_column (BTK_ICON_VIEW (icon_list), 0);
  btk_icon_view_set_text_column (BTK_ICON_VIEW (icon_list), 1);

#else

  cell = btk_cell_renderer_toggle_new ();
  btk_cell_layout_pack_start (BTK_CELL_LAYOUT (icon_list), cell, FALSE);
  g_object_set (cell, "activatable", TRUE, NULL);
  btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (icon_list),
				  cell, "active", 4, NULL);
  g_signal_connect (cell, "toggled", G_CALLBACK (toggled), model);

  cell = btk_cell_renderer_pixbuf_new ();
  btk_cell_layout_pack_start (BTK_CELL_LAYOUT (icon_list), cell, FALSE);
  g_object_set (cell, 
		"follow-state", TRUE, 
		NULL);
  btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (icon_list),
				  cell, "pixbuf", 0, NULL);

  cell = btk_cell_renderer_text_new ();
  btk_cell_layout_pack_start (BTK_CELL_LAYOUT (icon_list), cell, FALSE);
  g_object_set (cell, 
		"editable", TRUE, 
		"xalign", 0.5,
		"wrap-mode", BANGO_WRAP_WORD_CHAR,
		"wrap-width", 100,
		NULL);
  btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (icon_list),
				  cell, "text", 1, NULL);
  g_signal_connect (cell, "edited", G_CALLBACK (edited), model);

  /* now the tree view... */
  cell = btk_cell_renderer_toggle_new ();
  btk_cell_layout_pack_start (BTK_CELL_LAYOUT (tvc), cell, FALSE);
  g_object_set (cell, "activatable", TRUE, NULL);
  btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (tvc),
				  cell, "active", 4, NULL);
  g_signal_connect (cell, "toggled", G_CALLBACK (toggled), model);

  cell = btk_cell_renderer_pixbuf_new ();
  btk_cell_layout_pack_start (BTK_CELL_LAYOUT (tvc), cell, FALSE);
  g_object_set (cell, 
		"follow-state", TRUE, 
		NULL);
  btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (tvc),
				  cell, "pixbuf", 0, NULL);

  cell = btk_cell_renderer_text_new ();
  btk_cell_layout_pack_start (BTK_CELL_LAYOUT (tvc), cell, FALSE);
  g_object_set (cell, "editable", TRUE, NULL);
  btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (tvc),
				  cell, "text", 1, NULL);
  g_signal_connect (cell, "edited", G_CALLBACK (edited), model);
#endif
  /* Allow DND between the icon view and the tree view */
  
  btk_icon_view_enable_model_drag_source (BTK_ICON_VIEW (icon_list),
					  BDK_BUTTON1_MASK,
					  item_targets,
					  G_N_ELEMENTS (item_targets),
					  BDK_ACTION_MOVE);
  btk_icon_view_enable_model_drag_dest (BTK_ICON_VIEW (icon_list),
					item_targets,
					G_N_ELEMENTS (item_targets),
					BDK_ACTION_MOVE);

  btk_tree_view_enable_model_drag_source (BTK_TREE_VIEW (tv),
					  BDK_BUTTON1_MASK,
					  item_targets,
					  G_N_ELEMENTS (item_targets),
					  BDK_ACTION_MOVE);
  btk_tree_view_enable_model_drag_dest (BTK_TREE_VIEW (tv),
					item_targets,
					G_N_ELEMENTS (item_targets),
					BDK_ACTION_MOVE);

			      
  prop_editor = create_prop_editor (B_OBJECT (icon_list), 0);
  btk_widget_show_all (prop_editor);
  
  scrolled_window = btk_scrolled_window_new (NULL, NULL);
  btk_container_add (BTK_CONTAINER (scrolled_window), icon_list);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window),
  				  BTK_POLICY_AUTOMATIC, BTK_POLICY_AUTOMATIC);

  btk_paned_add1 (BTK_PANED (paned), scrolled_window);

  scrolled_window = btk_scrolled_window_new (NULL, NULL);
  btk_container_add (BTK_CONTAINER (scrolled_window), tv);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window),
  				  BTK_POLICY_AUTOMATIC, BTK_POLICY_AUTOMATIC);

  btk_paned_add2 (BTK_PANED (paned), scrolled_window);

  bbox = btk_hbutton_box_new ();
  btk_button_box_set_layout (BTK_BUTTON_BOX (bbox), BTK_BUTTONBOX_START);
  btk_box_pack_start (BTK_BOX (vbox), bbox, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Add some");
  g_signal_connect (button, "clicked", G_CALLBACK (add_some), icon_list);
  btk_box_pack_start (BTK_BOX (bbox), button, TRUE, TRUE, 0);

  button = btk_button_new_with_label ("Add many");
  g_signal_connect (button, "clicked", G_CALLBACK (add_many), icon_list);
  btk_box_pack_start (BTK_BOX (bbox), button, TRUE, TRUE, 0);

  button = btk_button_new_with_label ("Add large");
  g_signal_connect (button, "clicked", G_CALLBACK (add_large), icon_list);
  btk_box_pack_start (BTK_BOX (bbox), button, TRUE, TRUE, 0);

  button = btk_button_new_with_label ("Remove selected");
  g_signal_connect (button, "clicked", G_CALLBACK (foreach_selected_remove), icon_list);
  btk_box_pack_start (BTK_BOX (bbox), button, TRUE, TRUE, 0);

  button = btk_button_new_with_label ("Swap");
  g_signal_connect (button, "clicked", G_CALLBACK (swap_rows), icon_list);
  btk_box_pack_start (BTK_BOX (bbox), button, TRUE, TRUE, 0);

  bbox = btk_hbutton_box_new ();
  btk_button_box_set_layout (BTK_BUTTON_BOX (bbox), BTK_BUTTONBOX_START);
  btk_box_pack_start (BTK_BOX (vbox), bbox, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Select all");
  g_signal_connect (button, "clicked", G_CALLBACK (select_all), icon_list);
  btk_box_pack_start (BTK_BOX (bbox), button, TRUE, TRUE, 0);

  button = btk_button_new_with_label ("Unselect all");
  g_signal_connect (button, "clicked", G_CALLBACK (unselect_all), icon_list);
  btk_box_pack_start (BTK_BOX (bbox), button, TRUE, TRUE, 0);

  button = btk_button_new_with_label ("Select nonexisting");
  g_signal_connect (button, "clicked", G_CALLBACK (select_nonexisting), icon_list);
  btk_box_pack_start (BTK_BOX (bbox), button, TRUE, TRUE, 0);

  icon_list = btk_icon_view_new ();

  scrolled_window = btk_scrolled_window_new (NULL, NULL);
  btk_container_add (BTK_CONTAINER (scrolled_window), icon_list);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window),
				  BTK_POLICY_AUTOMATIC, BTK_POLICY_AUTOMATIC);
  btk_paned_add2 (BTK_PANED (paned), scrolled_window);

  btk_widget_show_all (window);

  btk_main ();

  return 0;
}
