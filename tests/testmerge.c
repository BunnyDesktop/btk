/* testmerge.c
 * Copyright (C) 2003 James Henstridge
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

#include "config.h"

#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <btk/btk.h>

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1 
#endif

struct { const gchar *filename; guint merge_id; } merge_ids[] = {
  { "merge-1.ui", 0 },
  { "merge-2.ui", 0 },
  { "merge-3.ui", 0 }
};

static void
dump_tree (BtkWidget    *button, 
	   BtkUIManager *merge)
{
  gchar *dump;

  dump = btk_ui_manager_get_ui (merge);
  g_message ("%s", dump);
  g_free (dump);
}

static void
dump_accels (void)
{
  btk_accel_map_save_fd (STDOUT_FILENO);
}

static void
print_toplevel (BtkWidget *widget, gpointer user_data)
{
  g_print ("%s\n", B_OBJECT_TYPE_NAME (widget));
}

static void
dump_toplevels (BtkWidget    *button, 
		BtkUIManager *merge)
{
  GSList *toplevels;

  toplevels = btk_ui_manager_get_toplevels (merge, 
					    BTK_UI_MANAGER_MENUBAR |
					    BTK_UI_MANAGER_TOOLBAR |
					    BTK_UI_MANAGER_POPUP);

  b_slist_foreach (toplevels, (GFunc) print_toplevel, NULL);
  b_slist_free (toplevels);
}

static void
toggle_tearoffs (BtkWidget    *button, 
		 BtkUIManager *merge)
{
  gboolean add_tearoffs;

  add_tearoffs = btk_ui_manager_get_add_tearoffs (merge);
  
  btk_ui_manager_set_add_tearoffs (merge, !add_tearoffs);
}

static gint
delayed_toggle_dynamic (BtkUIManager *merge)
{
  BtkAction *dyn;
  static BtkActionGroup *dynamic = NULL;
  static guint merge_id = 0;

  if (!dynamic)
    {
      dynamic = btk_action_group_new ("dynamic");
      btk_ui_manager_insert_action_group (merge, dynamic, 0);
      dyn = g_object_new (BTK_TYPE_ACTION,
			  "name", "dyn1",
			  "label", "Dynamic action 1",
			  "stock_id", BTK_STOCK_COPY,
			  NULL);
      btk_action_group_add_action (dynamic, dyn);
      dyn = g_object_new (BTK_TYPE_ACTION,
			  "name", "dyn2",
			  "label", "Dynamic action 2",
			  "stock_id", BTK_STOCK_EXECUTE,
			  NULL);
      btk_action_group_add_action (dynamic, dyn);
    }
  
  if (merge_id == 0)
    {
      merge_id = btk_ui_manager_new_merge_id (merge);
      btk_ui_manager_add_ui (merge, merge_id, "/toolbar1/ToolbarPlaceholder", 
			     "dyn1", "dyn1", 0, 0);
      btk_ui_manager_add_ui (merge, merge_id, "/toolbar1/ToolbarPlaceholder", 
			     "dynsep", NULL, BTK_UI_MANAGER_SEPARATOR, 0);
      btk_ui_manager_add_ui (merge, merge_id, "/toolbar1/ToolbarPlaceholder", 
			     "dyn2", "dyn2", 0, 0);

      btk_ui_manager_add_ui (merge, merge_id, "/menubar/EditMenu", 
			     "dyn1menu", "dyn1", BTK_UI_MANAGER_MENU, 0);
      btk_ui_manager_add_ui (merge, merge_id, "/menubar/EditMenu/dyn1menu", 
			     "dyn1", "dyn1", BTK_UI_MANAGER_MENUITEM, 0);
      btk_ui_manager_add_ui (merge, merge_id, "/menubar/EditMenu/dyn1menu/dyn1", 
			     "dyn2", "dyn2", BTK_UI_MANAGER_AUTO, FALSE);
    }
  else 
    {
      btk_ui_manager_remove_ui (merge, merge_id);
      merge_id = 0;
    }

  return FALSE;
}

static void
toggle_dynamic (BtkWidget    *button, 
		BtkUIManager *merge)
{
  bdk_threads_add_timeout (2000, (GSourceFunc)delayed_toggle_dynamic, merge);
}

static void
activate_action (BtkAction *action)
{
  const gchar *name = btk_action_get_name (action);
  const gchar *typename = B_OBJECT_TYPE_NAME (action);

  g_message ("Action %s (type=%s) activated", name, typename);
}

static void
toggle_action (BtkAction *action)
{
  const gchar *name = btk_action_get_name (action);
  const gchar *typename = B_OBJECT_TYPE_NAME (action);

  g_message ("ToggleAction %s (type=%s) toggled (active=%d)", name, typename,
	     btk_toggle_action_get_active (BTK_TOGGLE_ACTION (action)));
}


static void
radio_action_changed (BtkAction *action, BtkRadioAction *current)
{
  g_message ("RadioAction %s (type=%s) activated (active=%d) (value %d)", 
	     btk_action_get_name (BTK_ACTION (current)), 
	     B_OBJECT_TYPE_NAME (BTK_ACTION (current)),
	     btk_toggle_action_get_active (BTK_TOGGLE_ACTION (current)),
	     btk_radio_action_get_current_value (current));
}

static BtkActionEntry entries[] = {
  { "FileMenuAction", NULL, "_File" },
  { "EditMenuAction", NULL, "_Edit" },
  { "HelpMenuAction", NULL, "_Help" },
  { "JustifyMenuAction", NULL, "_Justify" },
  { "EmptyMenu1Action", NULL, "Empty 1" },
  { "EmptyMenu2Action", NULL, "Empty 2" },
  { "Test", NULL, "Test" },

  { "QuitAction",  BTK_STOCK_QUIT,  NULL,     "<control>q", "Quit", G_CALLBACK (btk_main_quit) },
  { "NewAction",   BTK_STOCK_NEW,   NULL,     "<control>n", "Create something", G_CALLBACK (activate_action) },
  { "New2Action",  BTK_STOCK_NEW,   NULL,     "<control>m", "Create something else", G_CALLBACK (activate_action) },
  { "OpenAction",  BTK_STOCK_OPEN,  NULL,     NULL,         "Open it", G_CALLBACK (activate_action) },
  { "CutAction",   BTK_STOCK_CUT,   NULL,     "<control>x", "Knive", G_CALLBACK (activate_action) },
  { "CopyAction",  BTK_STOCK_COPY,  NULL,     "<control>c", "Copy", G_CALLBACK (activate_action) },
  { "PasteAction", BTK_STOCK_PASTE, NULL,     "<control>v", "Paste", G_CALLBACK (activate_action) },
  { "AboutAction", NULL,            "_About", NULL,         "About", G_CALLBACK (activate_action) },
};
static guint n_entries = G_N_ELEMENTS (entries);

static BtkToggleActionEntry toggle_entries[] = {
  { "BoldAction",  BTK_STOCK_BOLD,  "_Bold",  "<control>b", "Make it bold", G_CALLBACK (toggle_action), 
    TRUE },
};
static guint n_toggle_entries = G_N_ELEMENTS (toggle_entries);

enum {
  JUSTIFY_LEFT,
  JUSTIFY_CENTER,
  JUSTIFY_RIGHT,
  JUSTIFY_FILL
};

static BtkRadioActionEntry radio_entries[] = {
  { "justify-left", BTK_STOCK_JUSTIFY_LEFT, NULL, "<control>L", 
    "Left justify the text", JUSTIFY_LEFT },
  { "justify-center", BTK_STOCK_JUSTIFY_CENTER, NULL, "<super>E",
    "Center justify the text", JUSTIFY_CENTER },
  { "justify-right", BTK_STOCK_JUSTIFY_RIGHT, NULL, "<hyper>R",
    "Right justify the text", JUSTIFY_RIGHT },
  { "justify-fill", BTK_STOCK_JUSTIFY_FILL, NULL, "<super><hyper>J",
    "Fill justify the text", JUSTIFY_FILL },
};
static guint n_radio_entries = G_N_ELEMENTS (radio_entries);

static void
add_widget (BtkUIManager *merge, 
	    BtkWidget    *widget, 
	    BtkBox       *box)
{
  BtkWidget *handle_box;

  if (BTK_IS_TOOLBAR (widget))
    {
      handle_box = btk_handle_box_new ();
      btk_widget_show (handle_box);
      btk_container_add (BTK_CONTAINER (handle_box), widget);
      btk_box_pack_start (box, handle_box, FALSE, FALSE, 0);
      g_signal_connect_swapped (widget, "destroy", 
				G_CALLBACK (btk_widget_destroy), handle_box);
    }
  else
    btk_box_pack_start (box, widget, FALSE, FALSE, 0);
    
  btk_widget_show (widget);
}

static void
toggle_merge (BtkWidget    *button, 
	      BtkUIManager *merge)
{
  gint mergenum;

  mergenum = GPOINTER_TO_INT (g_object_get_data (B_OBJECT (button), "mergenum"));

  if (btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (button)))
    {
      GError *err = NULL;

      g_message ("merging %s", merge_ids[mergenum].filename);
      merge_ids[mergenum].merge_id =
	btk_ui_manager_add_ui_from_file (merge, merge_ids[mergenum].filename, &err);
      if (err != NULL)
	{
	  BtkWidget *dialog;

	  dialog = btk_message_dialog_new (BTK_WINDOW (btk_widget_get_toplevel (button)),
					   0, BTK_MESSAGE_WARNING, BTK_BUTTONS_OK,
					   "could not merge %s: %s", merge_ids[mergenum].filename,
					   err->message);

	  g_signal_connect (dialog, "response", G_CALLBACK (btk_object_destroy), NULL);
	  btk_widget_show (dialog);

	  g_clear_error (&err);
	}
    }
  else
    {
      g_message ("unmerging %s (merge_id=%u)", merge_ids[mergenum].filename,
		 merge_ids[mergenum].merge_id);
      btk_ui_manager_remove_ui (merge, merge_ids[mergenum].merge_id);
    }
}

static void  
set_name_func (BtkTreeViewColumn *tree_column,
	       BtkCellRenderer   *cell,
	       BtkTreeModel      *tree_model,
	       BtkTreeIter       *iter,
	       gpointer           data)
{
  BtkAction *action;
  char *name;
  
  btk_tree_model_get (tree_model, iter, 0, &action, -1);
  g_object_get (action, "name", &name, NULL);
  g_object_set (cell, "text", name, NULL);
  g_free (name);
  g_object_unref (action);
}

static void
set_sensitive_func (BtkTreeViewColumn *tree_column,
		    BtkCellRenderer   *cell,
		    BtkTreeModel      *tree_model,
		    BtkTreeIter       *iter,
		    gpointer           data)
{
  BtkAction *action;
  gboolean sensitive;
  
  btk_tree_model_get (tree_model, iter, 0, &action, -1);
  g_object_get (action, "sensitive", &sensitive, NULL);
  g_object_set (cell, "active", sensitive, NULL);
  g_object_unref (action);
}


static void
set_visible_func (BtkTreeViewColumn *tree_column,
		  BtkCellRenderer   *cell,
		  BtkTreeModel      *tree_model,
		  BtkTreeIter       *iter,
		  gpointer           data)
{
  BtkAction *action;
  gboolean visible;
  
  btk_tree_model_get (tree_model, iter, 0, &action, -1);
  g_object_get (action, "visible", &visible, NULL);
  g_object_set (cell, "active", visible, NULL);
  g_object_unref (action);
}

static void
sensitivity_toggled (BtkCellRendererToggle *cell, 
		     const gchar           *path_str,
		     BtkTreeModel          *model)
{
  BtkTreePath *path;
  BtkTreeIter iter;
  BtkAction *action;
  gboolean sensitive;

  path = btk_tree_path_new_from_string (path_str);
  btk_tree_model_get_iter (model, &iter, path);

  btk_tree_model_get (model, &iter, 0, &action, -1);
  g_object_get (action, "sensitive", &sensitive, NULL);
  g_object_set (action, "sensitive", !sensitive, NULL);
  btk_tree_model_row_changed (model, path, &iter);
  btk_tree_path_free (path);
}

static void
visibility_toggled (BtkCellRendererToggle *cell, 
		    const gchar           *path_str, 
		    BtkTreeModel          *model)
{
  BtkTreePath *path;
  BtkTreeIter iter;
  BtkAction *action;
  gboolean visible;

  path = btk_tree_path_new_from_string (path_str);
  btk_tree_model_get_iter (model, &iter, path);

  btk_tree_model_get (model, &iter, 0, &action, -1);
  g_object_get (action, "visible", &visible, NULL);
  g_object_set (action, "visible", !visible, NULL);
  btk_tree_model_row_changed (model, path, &iter);
  btk_tree_path_free (path);
}

static gint
iter_compare_func (BtkTreeModel *model, 
		   BtkTreeIter  *a, 
		   BtkTreeIter  *b,
		   gpointer      user_data)
{
  BValue a_value = { 0, }, b_value = { 0, };
  BtkAction *a_action, *b_action;
  const gchar *a_name, *b_name;
  gint retval = 0;

  btk_tree_model_get_value (model, a, 0, &a_value);
  btk_tree_model_get_value (model, b, 0, &b_value);
  a_action = BTK_ACTION (b_value_get_object (&a_value));
  b_action = BTK_ACTION (b_value_get_object (&b_value));

  a_name = btk_action_get_name (a_action);
  b_name = btk_action_get_name (b_action);
  if (a_name == NULL && b_name == NULL) 
    retval = 0;
  else if (a_name == NULL)
    retval = -1;
  else if (b_name == NULL) 
    retval = 1;
  else 
    retval = strcmp (a_name, b_name);

  b_value_unset (&b_value);
  b_value_unset (&a_value);

  return retval;
}

static BtkWidget *
create_tree_view (BtkUIManager *merge)
{
  BtkWidget *tree_view, *sw;
  BtkListStore *store;
  GList *p;
  BtkCellRenderer *cell;
  
  store = btk_list_store_new (1, BTK_TYPE_ACTION);
  btk_tree_sortable_set_sort_func (BTK_TREE_SORTABLE (store), 0,
				   iter_compare_func, NULL, NULL);
  btk_tree_sortable_set_sort_column_id (BTK_TREE_SORTABLE (store), 0,
					BTK_SORT_ASCENDING);
  
  for (p = btk_ui_manager_get_action_groups (merge); p; p = p->next)
    {
      GList *actions, *l;

      actions = btk_action_group_list_actions (p->data);

      for (l = actions; l; l = l->next)
	{
	  BtkTreeIter iter;

	  btk_list_store_append (store, &iter);
	  btk_list_store_set (store, &iter, 0, l->data, -1);
	}

      g_list_free (actions);
    }
  
  tree_view = btk_tree_view_new_with_model (BTK_TREE_MODEL (store));
  g_object_unref (store);

  btk_tree_view_insert_column_with_data_func (BTK_TREE_VIEW (tree_view),
					      -1, "Action",
					      btk_cell_renderer_text_new (),
					      set_name_func, NULL, NULL);

  btk_tree_view_column_set_sort_column_id (btk_tree_view_get_column (BTK_TREE_VIEW (tree_view), 0), 0);

  cell = btk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (sensitivity_toggled), store);
  btk_tree_view_insert_column_with_data_func (BTK_TREE_VIEW (tree_view),
					      -1, "Sensitive",
					      cell,
					      set_sensitive_func, NULL, NULL);

  cell = btk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (visibility_toggled), store);
  btk_tree_view_insert_column_with_data_func (BTK_TREE_VIEW (tree_view),
					      -1, "Visible",
					      cell,
					      set_visible_func, NULL, NULL);

  sw = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (sw),
				  BTK_POLICY_NEVER, BTK_POLICY_AUTOMATIC);
  btk_container_add (BTK_CONTAINER (sw), tree_view);
  
  return sw;
}

static gboolean
area_press (BtkWidget      *drawing_area,
	    BdkEventButton *event,
	    BtkUIManager   *merge)
{
  btk_widget_grab_focus (drawing_area);

  if (event->button == 3 &&
      event->type == BDK_BUTTON_PRESS)
    {
      BtkWidget *menu = btk_ui_manager_get_widget (merge, "/FileMenu");
      
      if (BTK_IS_MENU (menu)) 
	{
	  btk_menu_popup (BTK_MENU (menu), NULL, NULL,
			  NULL, drawing_area,
			  3, event->time);
	  return TRUE;
	}
    }

  return FALSE;
  
}

static void
activate_path (BtkWidget      *button,
	       BtkUIManager   *merge)
{
  BtkAction *action = btk_ui_manager_get_action (merge, 
						 "/menubar/HelpMenu/About");
  if (action)
    btk_action_activate (action);
  else 
    g_message ("no action found");
}

typedef struct _ActionStatus ActionStatus;

struct _ActionStatus {
  BtkAction *action;
  BtkWidget *statusbar;
};

static void
action_status_destroy (gpointer data)
{
  ActionStatus *action_status = data;

  g_object_unref (action_status->action);
  g_object_unref (action_status->statusbar);

  g_free (action_status);
}

static void
set_tip (BtkWidget *widget)
{
  ActionStatus *data;
  gchar *tooltip;
  
  data = g_object_get_data (B_OBJECT (widget), "action-status");
  
  if (data) 
    {
      g_object_get (data->action, "tooltip", &tooltip, NULL);
      
      btk_statusbar_push (BTK_STATUSBAR (data->statusbar), 0, 
			  tooltip ? tooltip : "");
      
      g_free (tooltip);
    }
}

static void
unset_tip (BtkWidget *widget)
{
  ActionStatus *data;

  data = g_object_get_data (B_OBJECT (widget), "action-status");

  if (data)
    btk_statusbar_pop (BTK_STATUSBAR (data->statusbar), 0);
}
		    
static void
connect_proxy (BtkUIManager *merge,
	       BtkAction    *action,
	       BtkWidget    *proxy,
	       BtkWidget    *statusbar)
{
  if (BTK_IS_MENU_ITEM (proxy)) 
    {
      ActionStatus *data;

      data = g_object_get_data (B_OBJECT (proxy), "action-status");
      if (data)
	{
	  g_object_unref (data->action);
	  g_object_unref (data->statusbar);

	  data->action = g_object_ref (action);
	  data->statusbar = g_object_ref (statusbar);
	}
      else
	{
	  data = g_new0 (ActionStatus, 1);

	  data->action = g_object_ref (action);
	  data->statusbar = g_object_ref (statusbar);

	  g_object_set_data_full (B_OBJECT (proxy), "action-status", 
				  data, action_status_destroy);
	  
	  g_signal_connect (proxy, "select",  G_CALLBACK (set_tip), NULL);
	  g_signal_connect (proxy, "deselect", G_CALLBACK (unset_tip), NULL);
	}
    }
}

int
main (int argc, char **argv)
{
  BtkActionGroup *action_group;
  BtkAction *action;
  BtkUIManager *merge;
  BtkWidget *window, *table, *frame, *menu_box, *vbox, *view;
  BtkWidget *button, *area, *statusbar;
  gint i;
  
  btk_init (&argc, &argv);

  action_group = btk_action_group_new ("TestActions");
  btk_action_group_add_actions (action_group, 
				entries, n_entries, 
				NULL);
  action = btk_action_group_get_action (action_group, "EmptyMenu1Action");
  g_object_set (action, "hide_if_empty", FALSE, NULL);
  action = btk_action_group_get_action (action_group, "EmptyMenu2Action");
  g_object_set (action, "hide_if_empty", TRUE, NULL);
  btk_action_group_add_toggle_actions (action_group, 
				       toggle_entries, n_toggle_entries, 
				       NULL);
  btk_action_group_add_radio_actions (action_group, 
				      radio_entries, n_radio_entries, 
				      JUSTIFY_RIGHT,
				      G_CALLBACK (radio_action_changed), NULL);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_default_size (BTK_WINDOW (window), -1, 400);
  g_signal_connect (window, "destroy", G_CALLBACK (btk_main_quit), NULL);

  table = btk_table_new (2, 2, FALSE);
  btk_table_set_row_spacings (BTK_TABLE (table), 2);
  btk_table_set_col_spacings (BTK_TABLE (table), 2);
  btk_container_set_border_width (BTK_CONTAINER (table), 2);
  btk_container_add (BTK_CONTAINER (window), table);

  frame = btk_frame_new ("Menus and Toolbars");
  btk_table_attach (BTK_TABLE (table), frame, 0,2, 1,2,
		    BTK_FILL|BTK_EXPAND, BTK_FILL, 0, 0);
  
  menu_box = btk_vbox_new (FALSE, 0);
  btk_container_set_border_width (BTK_CONTAINER (menu_box), 2);
  btk_container_add (BTK_CONTAINER (frame), menu_box);

  statusbar = btk_statusbar_new ();
  btk_box_pack_end (BTK_BOX (menu_box), statusbar, FALSE, FALSE, 0);
    
  area = btk_drawing_area_new ();
  btk_widget_set_events (area, BDK_BUTTON_PRESS_MASK);
  btk_widget_set_size_request (area, -1, 40);
  btk_box_pack_end (BTK_BOX (menu_box), area, FALSE, FALSE, 0);
  btk_widget_show (area);

  button = btk_button_new ();
  btk_box_pack_end (BTK_BOX (menu_box), button, FALSE, FALSE, 0);
  btk_activatable_set_related_action (BTK_ACTIVATABLE (button),
			    btk_action_group_get_action (action_group, "AboutAction"));

  btk_widget_show (button);

  button = btk_check_button_new ();
  btk_box_pack_end (BTK_BOX (menu_box), button, FALSE, FALSE, 0);
  btk_activatable_set_related_action (BTK_ACTIVATABLE (button),
			    btk_action_group_get_action (action_group, "BoldAction"));
  btk_widget_show (button);

  merge = btk_ui_manager_new ();

  g_signal_connect (merge, "connect-proxy", G_CALLBACK (connect_proxy), statusbar);
  g_signal_connect (area, "button_press_event", G_CALLBACK (area_press), merge);

  btk_ui_manager_insert_action_group (merge, action_group, 0);
  g_signal_connect (merge, "add_widget", G_CALLBACK (add_widget), menu_box);

  btk_window_add_accel_group (BTK_WINDOW (window), 
			      btk_ui_manager_get_accel_group (merge));
  
  frame = btk_frame_new ("UI Files");
  btk_table_attach (BTK_TABLE (table), frame, 0,1, 0,1,
		    BTK_FILL, BTK_FILL|BTK_EXPAND, 0, 0);

  vbox = btk_vbox_new (FALSE, 2);
  btk_container_set_border_width (BTK_CONTAINER (vbox), 2);
  btk_container_add (BTK_CONTAINER (frame), vbox);

  for (i = 0; i < G_N_ELEMENTS (merge_ids); i++)
    {
      button = btk_check_button_new_with_label (merge_ids[i].filename);
      g_object_set_data (B_OBJECT (button), "mergenum", GINT_TO_POINTER (i));
      g_signal_connect (button, "toggled", G_CALLBACK (toggle_merge), merge);
      btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (button), TRUE);
    }

  button = btk_check_button_new_with_label ("Tearoffs");
  g_signal_connect (button, "clicked", G_CALLBACK (toggle_tearoffs), merge);
  btk_box_pack_end (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = btk_check_button_new_with_label ("Dynamic");
  g_signal_connect (button, "clicked", G_CALLBACK (toggle_dynamic), merge);
  btk_box_pack_end (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Activate path");
  g_signal_connect (button, "clicked", G_CALLBACK (activate_path), merge);
  btk_box_pack_end (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Dump Tree");
  g_signal_connect (button, "clicked", G_CALLBACK (dump_tree), merge);
  btk_box_pack_end (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Dump Toplevels");
  g_signal_connect (button, "clicked", G_CALLBACK (dump_toplevels), merge);
  btk_box_pack_end (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Dump Accels");
  g_signal_connect (button, "clicked", G_CALLBACK (dump_accels), NULL);
  btk_box_pack_end (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  view = create_tree_view (merge);
  btk_table_attach (BTK_TABLE (table), view, 1,2, 0,1,
		    BTK_FILL|BTK_EXPAND, BTK_FILL|BTK_EXPAND, 0, 0);

  btk_widget_show_all (window);
  btk_main ();

#ifdef DEBUG_UI_MANAGER
  {
    GList *action;
    
    g_print ("\n> before unreffing the ui manager <\n");
    for (action = btk_action_group_list_actions (action_group);
	 action; 
	 action = action->next)
      {
	BtkAction *a = action->data;
	g_print ("  action %s ref count %d\n", 
		 btk_action_get_name (a), B_OBJECT (a)->ref_count);
      }
  }
#endif

  g_object_unref (merge);

#ifdef DEBUG_UI_MANAGER
  {
    GList *action;

    g_print ("\n> after unreffing the ui manager <\n");
    for (action = btk_action_group_list_actions (action_group);
	 action; 
	 action = action->next)
      {
	BtkAction *a = action->data;
	g_print ("  action %s ref count %d\n", 
		 btk_action_get_name (a), B_OBJECT (a)->ref_count);
      }
  }
#endif

  g_object_unref (action_group);

  return 0;
}
