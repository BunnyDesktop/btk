/* testtreeedit.c
 * Copyright (C) 2001 Red Hat, Inc
 * Author: Jonathan Blandford
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
#include <btk/btk.h>

typedef struct {
  const bchar *string;
  bboolean is_editable;
  bint progress;
} ListEntry;

enum {
  STRING_COLUMN,
  IS_EDITABLE_COLUMN,
  PIXBUF_COLUMN,
  PROGRESS_COLUMN,
  NUM_COLUMNS
};

static ListEntry model_strings[] =
{
  {"A simple string", TRUE, 0 },
  {"Another string!", TRUE, 10 },
  {"Guess what, a third string. This one can't be edited", FALSE, 47 },
  {"And then a fourth string. Neither can this", FALSE, 48 },
  {"Multiline\nFun!", TRUE, 75 },
  { NULL }
};

static BtkTreeModel *
create_model (void)
{
  BtkTreeStore *model;
  BtkTreeIter iter;
  bint i;
  BdkPixbuf *foo;
  BtkWidget *blah;

  blah = btk_window_new (BTK_WINDOW_TOPLEVEL);
  foo = btk_widget_render_icon (blah, BTK_STOCK_NEW, BTK_ICON_SIZE_MENU, NULL);
  btk_widget_destroy (blah);
  
  model = btk_tree_store_new (NUM_COLUMNS,
			      B_TYPE_STRING,
			      B_TYPE_BOOLEAN,
			      BDK_TYPE_PIXBUF,
			      B_TYPE_INT);

  for (i = 0; model_strings[i].string != NULL; i++)
    {
      btk_tree_store_append (model, &iter, NULL);

      btk_tree_store_set (model, &iter,
			  STRING_COLUMN, model_strings[i].string,
			  IS_EDITABLE_COLUMN, model_strings[i].is_editable,
			  PIXBUF_COLUMN, foo,
			  PROGRESS_COLUMN, model_strings[i].progress,
			  -1);
    }
  
  return BTK_TREE_MODEL (model);
}

static void
toggled (BtkCellRendererToggle *cell,
	 bchar                 *path_string,
	 bpointer               data)
{
  BtkTreeModel *model = BTK_TREE_MODEL (data);
  BtkTreeIter iter;
  BtkTreePath *path = btk_tree_path_new_from_string (path_string);
  bboolean value;

  btk_tree_model_get_iter (model, &iter, path);
  btk_tree_model_get (model, &iter, IS_EDITABLE_COLUMN, &value, -1);

  value = !value;
  btk_tree_store_set (BTK_TREE_STORE (model), &iter, IS_EDITABLE_COLUMN, value, -1);

  btk_tree_path_free (path);
}

static void
edited (BtkCellRendererText *cell,
	bchar               *path_string,
	bchar               *new_text,
	bpointer             data)
{
  BtkTreeModel *model = BTK_TREE_MODEL (data);
  BtkTreeIter iter;
  BtkTreePath *path = btk_tree_path_new_from_string (path_string);

  btk_tree_model_get_iter (model, &iter, path);
  btk_tree_store_set (BTK_TREE_STORE (model), &iter, STRING_COLUMN, new_text, -1);

  btk_tree_path_free (path);
}

static bboolean
button_press_event (BtkWidget *widget, BdkEventButton *event, bpointer callback_data)
{
	/* Deselect if people click outside any row. */
	if (event->window == btk_tree_view_get_bin_window (BTK_TREE_VIEW (widget))
	    && !btk_tree_view_get_path_at_pos (BTK_TREE_VIEW (widget),
					       event->x, event->y, NULL, NULL, NULL, NULL)) {
		btk_tree_selection_unselect_all (btk_tree_view_get_selection (BTK_TREE_VIEW (widget)));
	}

	/* Let the default code run in any case; it won't reselect anything. */
	return FALSE;
}

bint
main (bint argc, bchar **argv)
{
  BtkWidget *window;
  BtkWidget *scrolled_window;
  BtkWidget *tree_view;
  BtkTreeModel *tree_model;
  BtkCellRenderer *renderer;
  BtkTreeViewColumn *column;
  
  btk_init (&argc, &argv);

  if (g_getenv ("RTL"))
    btk_widget_set_default_direction (BTK_TEXT_DIR_RTL);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_title (BTK_WINDOW (window), "BtkTreeView editing sample");
  g_signal_connect (window, "destroy", btk_main_quit, NULL);

  scrolled_window = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (scrolled_window), BTK_SHADOW_ETCHED_IN);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window), BTK_POLICY_AUTOMATIC, BTK_POLICY_AUTOMATIC);
  btk_container_add (BTK_CONTAINER (window), scrolled_window);

  tree_model = create_model ();
  tree_view = btk_tree_view_new_with_model (tree_model);
  g_signal_connect (tree_view, "button_press_event", G_CALLBACK (button_press_event), NULL);
  btk_tree_view_set_rules_hint (BTK_TREE_VIEW (tree_view), TRUE);
  btk_tree_view_set_headers_visible (BTK_TREE_VIEW (tree_view), TRUE);

  column = btk_tree_view_column_new ();
  btk_tree_view_column_set_title (column, "String");

  renderer = btk_cell_renderer_pixbuf_new ();
  btk_tree_view_column_pack_start (column, renderer, TRUE);
  btk_tree_view_column_set_attributes (column, renderer,
				       "pixbuf", PIXBUF_COLUMN, NULL);

  renderer = btk_cell_renderer_text_new ();
  btk_tree_view_column_pack_start (column, renderer, TRUE);
  btk_tree_view_column_set_attributes (column, renderer,
				       "text", STRING_COLUMN,
				       "editable", IS_EDITABLE_COLUMN,
				       NULL);
  g_signal_connect (renderer, "edited",
		    G_CALLBACK (edited), tree_model);
  renderer = btk_cell_renderer_text_new ();
  btk_tree_view_column_pack_start (column, renderer, TRUE);
  btk_tree_view_column_set_attributes (column, renderer,
		  		       "text", STRING_COLUMN,
				       "editable", IS_EDITABLE_COLUMN,
				       NULL);
  g_signal_connect (renderer, "edited",
		    G_CALLBACK (edited), tree_model);

  renderer = btk_cell_renderer_pixbuf_new ();
  btk_tree_view_column_pack_start (column, renderer, TRUE);
  btk_tree_view_column_set_attributes (column, renderer,
				       "pixbuf", PIXBUF_COLUMN, NULL);
  btk_tree_view_append_column (BTK_TREE_VIEW (tree_view), column);

  renderer = btk_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled",
		    G_CALLBACK (toggled), tree_model);
  
  g_object_set (renderer,
		"xalign", 0.0,
		NULL);
  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tree_view),
					       -1, "Editable",
					       renderer,
					       "active", IS_EDITABLE_COLUMN,
					       NULL);

  renderer = btk_cell_renderer_progress_new ();
  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tree_view),
					       -1, "Progress",
					       renderer,
					       "value", PROGRESS_COLUMN,
					       NULL);

  btk_container_add (BTK_CONTAINER (scrolled_window), tree_view);
  
  btk_window_set_default_size (BTK_WINDOW (window),
			       800, 175);

  btk_widget_show_all (window);
  btk_main ();

  return 0;
}
