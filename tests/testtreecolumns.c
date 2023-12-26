/* testtreecolumns.c
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

/*
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 *
 * DO NOT!!! I REPEAT DO NOT!  EVER LOOK AT THIS CODE AS AN EXAMPLE OF WHAT YOUR
 * CODE SHOULD LOOK LIKE.
 *
 * IT IS VERY CONFUSING, AND IS MEANT TO TEST A LOT OF CODE IN THE TREE.  WHILE
 * IT IS ACTUALLY CORRECT CODE, IT IS NOT USEFUL.
 */

BtkWidget *left_tree_view;
BtkWidget *top_right_tree_view;
BtkWidget *bottom_right_tree_view;
BtkTreeModel *left_tree_model;
BtkTreeModel *top_right_tree_model;
BtkTreeModel *bottom_right_tree_model;
BtkWidget *sample_tree_view_top;
BtkWidget *sample_tree_view_bottom;

#define column_data "my_column_data"

static void move_row  (BtkTreeModel *src,
		       BtkTreeIter  *src_iter,
		       BtkTreeModel *dest,
		       BtkTreeIter  *dest_iter);

/* Kids, don't try this at home.  */

/* Small BtkTreeModel to model columns */
typedef struct _ViewColumnModel ViewColumnModel;
typedef struct _ViewColumnModelClass ViewColumnModelClass;

struct _ViewColumnModel
{
  GObject parent;
  BtkTreeView *view;
  GList *columns;
  gint stamp;
};

struct _ViewColumnModelClass
{
  GObjectClass parent_class;
};

static void view_column_model_init (ViewColumnModel *model)
{
  model->stamp = g_random_int ();
}

static gint
view_column_model_get_n_columns (BtkTreeModel *tree_model)
{
  return 2;
}

static GType
view_column_model_get_column_type (BtkTreeModel *tree_model,
				   gint          index)
{
  switch (index)
    {
    case 0:
      return G_TYPE_STRING;
    case 1:
      return BTK_TYPE_TREE_VIEW_COLUMN;
    default:
      return G_TYPE_INVALID;
    }
}

static gboolean
view_column_model_get_iter (BtkTreeModel *tree_model,
			    BtkTreeIter  *iter,
			    BtkTreePath  *path)

{
  ViewColumnModel *view_model = (ViewColumnModel *)tree_model;
  GList *list;
  gint i;

  g_return_val_if_fail (btk_tree_path_get_depth (path) > 0, FALSE);

  i = btk_tree_path_get_indices (path)[0];
  list = g_list_nth (view_model->columns, i);

  if (list == NULL)
    return FALSE;

  iter->stamp = view_model->stamp;
  iter->user_data = list;

  return TRUE;
}

static BtkTreePath *
view_column_model_get_path (BtkTreeModel *tree_model,
			    BtkTreeIter  *iter)
{
  ViewColumnModel *view_model = (ViewColumnModel *)tree_model;
  BtkTreePath *retval;
  GList *list;
  gint i = 0;

  g_return_val_if_fail (iter->stamp == view_model->stamp, NULL);

  for (list = view_model->columns; list; list = list->next)
    {
      if (list == (GList *)iter->user_data)
	break;
      i++;
    }
  if (list == NULL)
    return NULL;

  retval = btk_tree_path_new ();
  btk_tree_path_append_index (retval, i);
  return retval;
}

static void
view_column_model_get_value (BtkTreeModel *tree_model,
			     BtkTreeIter  *iter,
			     gint          column,
			     GValue       *value)
{
  ViewColumnModel *view_model = (ViewColumnModel *)tree_model;

  g_return_if_fail (column < 2);
  g_return_if_fail (view_model->stamp == iter->stamp);
  g_return_if_fail (iter->user_data != NULL);

  if (column == 0)
    {
      g_value_init (value, G_TYPE_STRING);
      g_value_set_string (value, btk_tree_view_column_get_title (BTK_TREE_VIEW_COLUMN (((GList *)iter->user_data)->data)));
    }
  else
    {
      g_value_init (value, BTK_TYPE_TREE_VIEW_COLUMN);
      g_value_set_object (value, ((GList *)iter->user_data)->data);
    }
}

static gboolean
view_column_model_iter_next (BtkTreeModel  *tree_model,
			     BtkTreeIter   *iter)
{
  ViewColumnModel *view_model = (ViewColumnModel *)tree_model;

  g_return_val_if_fail (view_model->stamp == iter->stamp, FALSE);
  g_return_val_if_fail (iter->user_data != NULL, FALSE);

  iter->user_data = ((GList *)iter->user_data)->next;
  return iter->user_data != NULL;
}

static gboolean
view_column_model_iter_children (BtkTreeModel *tree_model,
				 BtkTreeIter  *iter,
				 BtkTreeIter  *parent)
{
  ViewColumnModel *view_model = (ViewColumnModel *)tree_model;

  /* this is a list, nodes have no children */
  if (parent)
    return FALSE;

  /* but if parent == NULL we return the list itself as children of the
   * "root"
   */

  if (view_model->columns)
    {
      iter->stamp = view_model->stamp;
      iter->user_data = view_model->columns;
      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
view_column_model_iter_has_child (BtkTreeModel *tree_model,
				  BtkTreeIter  *iter)
{
  return FALSE;
}

static gint
view_column_model_iter_n_children (BtkTreeModel *tree_model,
				   BtkTreeIter  *iter)
{
  return g_list_length (((ViewColumnModel *)tree_model)->columns);
}

static gint
view_column_model_iter_nth_child (BtkTreeModel *tree_model,
 				  BtkTreeIter  *iter,
				  BtkTreeIter  *parent,
				  gint          n)
{
  ViewColumnModel *view_model = (ViewColumnModel *)tree_model;

  if (parent)
    return FALSE;

  iter->stamp = view_model->stamp;
  iter->user_data = g_list_nth ((GList *)view_model->columns, n);

  return (iter->user_data != NULL);
}

static gboolean
view_column_model_iter_parent (BtkTreeModel *tree_model,
			       BtkTreeIter  *iter,
			       BtkTreeIter  *child)
{
  return FALSE;
}

static void
view_column_model_tree_model_init (BtkTreeModelIface *iface)
{
  iface->get_n_columns = view_column_model_get_n_columns;
  iface->get_column_type = view_column_model_get_column_type;
  iface->get_iter = view_column_model_get_iter;
  iface->get_path = view_column_model_get_path;
  iface->get_value = view_column_model_get_value;
  iface->iter_next = view_column_model_iter_next;
  iface->iter_children = view_column_model_iter_children;
  iface->iter_has_child = view_column_model_iter_has_child;
  iface->iter_n_children = view_column_model_iter_n_children;
  iface->iter_nth_child = view_column_model_iter_nth_child;
  iface->iter_parent = view_column_model_iter_parent;
}

static gboolean
view_column_model_drag_data_get (BtkTreeDragSource   *drag_source,
				 BtkTreePath         *path,
				 BtkSelectionData    *selection_data)
{
  if (btk_tree_set_row_drag_data (selection_data,
				  BTK_TREE_MODEL (drag_source),
				  path))
    return TRUE;
  else
    return FALSE;
}

static gboolean
view_column_model_drag_data_delete (BtkTreeDragSource *drag_source,
				    BtkTreePath       *path)
{
  /* Nothing -- we handle moves on the dest side */
  
  return TRUE;
}

static gboolean
view_column_model_row_drop_possible (BtkTreeDragDest   *drag_dest,
				     BtkTreePath       *dest_path,
				     BtkSelectionData  *selection_data)
{
  BtkTreeModel *src_model;
  
  if (btk_tree_get_row_drag_data (selection_data,
				  &src_model,
				  NULL))
    {
      if (src_model == left_tree_model ||
	  src_model == top_right_tree_model ||
	  src_model == bottom_right_tree_model)
	return TRUE;
    }

  return FALSE;
}

static gboolean
view_column_model_drag_data_received (BtkTreeDragDest   *drag_dest,
				      BtkTreePath       *dest,
				      BtkSelectionData  *selection_data)
{
  BtkTreeModel *src_model;
  BtkTreePath *src_path = NULL;
  gboolean retval = FALSE;
  
  if (btk_tree_get_row_drag_data (selection_data,
				  &src_model,
				  &src_path))
    {
      BtkTreeIter src_iter;
      BtkTreeIter dest_iter;
      gboolean have_dest;

      /* We are a little lazy here, and assume if we can't convert dest
       * to an iter, we need to append. See btkliststore.c for a more
       * careful handling of this.
       */
      have_dest = btk_tree_model_get_iter (BTK_TREE_MODEL (drag_dest), &dest_iter, dest);

      if (btk_tree_model_get_iter (src_model, &src_iter, src_path))
	{
	  if (src_model == left_tree_model ||
	      src_model == top_right_tree_model ||
	      src_model == bottom_right_tree_model)
	    {
	      move_row (src_model, &src_iter, BTK_TREE_MODEL (drag_dest),
			have_dest ? &dest_iter : NULL);
	      retval = TRUE;
	    }
	}

      btk_tree_path_free (src_path);
    }
  
  return retval;
}

static void
view_column_model_drag_source_init (BtkTreeDragSourceIface *iface)
{
  iface->drag_data_get = view_column_model_drag_data_get;
  iface->drag_data_delete = view_column_model_drag_data_delete;
}

static void
view_column_model_drag_dest_init (BtkTreeDragDestIface *iface)
{
  iface->drag_data_received = view_column_model_drag_data_received;
  iface->row_drop_possible = view_column_model_row_drop_possible;
}

GType
view_column_model_get_type (void)
{
  static GType view_column_model_type = 0;

  if (!view_column_model_type)
    {
      const GTypeInfo view_column_model_info =
      {
	sizeof (BtkListStoreClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
        NULL,		/* class_init */
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (BtkListStore),
	0,
        (GInstanceInitFunc) view_column_model_init,
      };

      const GInterfaceInfo tree_model_info =
      {
	(GInterfaceInitFunc) view_column_model_tree_model_init,
	NULL,
	NULL
      };

      const GInterfaceInfo drag_source_info =
      {
	(GInterfaceInitFunc) view_column_model_drag_source_init,
	NULL,
	NULL
      };

      const GInterfaceInfo drag_dest_info =
      {
	(GInterfaceInitFunc) view_column_model_drag_dest_init,
	NULL,
	NULL
      };

      view_column_model_type = g_type_register_static (G_TYPE_OBJECT, "ViewModelColumn", &view_column_model_info, 0);
      g_type_add_interface_static (view_column_model_type,
				   BTK_TYPE_TREE_MODEL,
				   &tree_model_info);
      g_type_add_interface_static (view_column_model_type,
				   BTK_TYPE_TREE_DRAG_SOURCE,
				   &drag_source_info);
      g_type_add_interface_static (view_column_model_type,
				   BTK_TYPE_TREE_DRAG_DEST,
				   &drag_dest_info);
    }

  return view_column_model_type;
}

static void
update_columns (BtkTreeView *view, ViewColumnModel *view_model)
{
  GList *old_columns = view_model->columns;
  gint old_length, length;
  GList *a, *b;

  view_model->columns = btk_tree_view_get_columns (view_model->view);

  /* As the view tells us one change at a time, we can do this hack. */
  length = g_list_length (view_model->columns);
  old_length = g_list_length (old_columns);
  if (length != old_length)
    {
      BtkTreePath *path;
      gint i = 0;

      /* where are they different */
      for (a = old_columns, b = view_model->columns; a && b; a = a->next, b = b->next)
	{
	  if (a->data != b->data)
	    break;
	  i++;
	}
      path = btk_tree_path_new ();
      btk_tree_path_append_index (path, i);
      if (length < old_length)
	{
	  view_model->stamp++;
	  btk_tree_model_row_deleted (BTK_TREE_MODEL (view_model), path);
	}
      else
	{
	  BtkTreeIter iter;
	  iter.stamp = view_model->stamp;
	  iter.user_data = b;
	  btk_tree_model_row_inserted (BTK_TREE_MODEL (view_model), path, &iter);
	}
      btk_tree_path_free (path);
    }
  else
    {
      gint i;
      gint m = 0, n = 1;
      gint *new_order;
      BtkTreePath *path;

      new_order = g_new (int, length);
      a = old_columns; b = view_model->columns;

      while (a->data == b->data)
	{
	  a = a->next;
	  b = b->next;
	  if (a == NULL)
	    return;
	  m++;
	}

      if (a->next->data == b->data)
	{
	  b = b->next;
	  while (b->data != a->data)
	    {
	      b = b->next;
	      n++;
	    }
	  for (i = 0; i < m; i++)
	    new_order[i] = i;
	  for (i = m; i < m+n; i++)
	    new_order[i] = i+1;
	  new_order[i] = m;
	  for (i = m + n +1; i < length; i++)
	    new_order[i] = i;
	}
      else
	{
	  a = a->next;
	  while (a->data != b->data)
	    {
	      a = a->next;
	      n++;
	    }
	  for (i = 0; i < m; i++)
	    new_order[i] = i;
	  new_order[m] = m+n;
	  for (i = m+1; i < m + n+ 1; i++)
	    new_order[i] = i - 1;
	  for (i = m + n + 1; i < length; i++)
	    new_order[i] = i;
	}

      path = btk_tree_path_new ();
      btk_tree_model_rows_reordered (BTK_TREE_MODEL (view_model),
				     path,
				     NULL,
				     new_order);
      btk_tree_path_free (path);
      g_free (new_order);
    }
  if (old_columns)
    g_list_free (old_columns);
}

static BtkTreeModel *
view_column_model_new (BtkTreeView *view)
{
  BtkTreeModel *retval;

  retval = g_object_new (view_column_model_get_type (), NULL);
  ((ViewColumnModel *)retval)->view = view;
  ((ViewColumnModel *)retval)->columns = btk_tree_view_get_columns (view);

  g_signal_connect (view, "columns_changed", G_CALLBACK (update_columns), retval);

  return retval;
}

/* Back to sanity.
 */

static void
add_clicked (BtkWidget *button, gpointer data)
{
  static gint i = 0;

  BtkTreeIter iter;
  BtkTreeViewColumn *column;
  BtkTreeSelection *selection;
  BtkCellRenderer *cell;
  gchar *label = g_strdup_printf ("Column %d", i);

  cell = btk_cell_renderer_text_new ();
  column = btk_tree_view_column_new_with_attributes (label, cell, "text", 0, NULL);
  g_object_set_data_full (G_OBJECT (column), column_data, label, g_free);
  btk_tree_view_column_set_reorderable (column, TRUE);
  btk_tree_view_column_set_sizing (column, BTK_TREE_VIEW_COLUMN_GROW_ONLY);
  btk_tree_view_column_set_resizable (column, TRUE);
  btk_list_store_append (BTK_LIST_STORE (left_tree_model), &iter);
  btk_list_store_set (BTK_LIST_STORE (left_tree_model), &iter, 0, label, 1, column, -1);
  i++;

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (left_tree_view));
  btk_tree_selection_select_iter (selection, &iter);
}

static void
get_visible (BtkTreeViewColumn *tree_column,
	     BtkCellRenderer   *cell,
	     BtkTreeModel      *tree_model,
	     BtkTreeIter       *iter,
	     gpointer           data)
{
  BtkTreeViewColumn *column;

  btk_tree_model_get (tree_model, iter, 1, &column, -1);
  if (column)
    {
      btk_cell_renderer_toggle_set_active (BTK_CELL_RENDERER_TOGGLE (cell),
					   btk_tree_view_column_get_visible (column));
    }
}

static void
set_visible (BtkCellRendererToggle *cell,
	     gchar                 *path_str,
	     gpointer               data)
{
  BtkTreeView *tree_view = (BtkTreeView *) data;
  BtkTreeViewColumn *column;
  BtkTreeModel *model;
  BtkTreeIter iter;
  BtkTreePath *path = btk_tree_path_new_from_string (path_str);

  model = btk_tree_view_get_model (tree_view);

  btk_tree_model_get_iter (model, &iter, path);
  btk_tree_model_get (model, &iter, 1, &column, -1);

  if (column)
    {
      btk_tree_view_column_set_visible (column, ! btk_tree_view_column_get_visible (column));
      btk_tree_model_row_changed (model, path, &iter);
    }
  btk_tree_path_free (path);
}

static void
move_to_left (BtkTreeModel *src,
	      BtkTreeIter  *src_iter,
	      BtkTreeIter  *dest_iter)
{
  BtkTreeIter iter;
  BtkTreeViewColumn *column;
  BtkTreeSelection *selection;
  gchar *label;

  btk_tree_model_get (src, src_iter, 0, &label, 1, &column, -1);

  if (src == top_right_tree_model)
    btk_tree_view_remove_column (BTK_TREE_VIEW (sample_tree_view_top), column);
  else
    btk_tree_view_remove_column (BTK_TREE_VIEW (sample_tree_view_bottom), column);

  /*  btk_list_store_remove (BTK_LIST_STORE (btk_tree_view_get_model (BTK_TREE_VIEW (data))), &iter);*/

  /* Put it back on the left */
  if (dest_iter)
    btk_list_store_insert_before (BTK_LIST_STORE (left_tree_model),
				  &iter, dest_iter);
  else
    btk_list_store_append (BTK_LIST_STORE (left_tree_model), &iter);
  
  btk_list_store_set (BTK_LIST_STORE (left_tree_model), &iter, 0, label, 1, column, -1);
  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (left_tree_view));
  btk_tree_selection_select_iter (selection, &iter);

  g_free (label);
}

static void
move_to_right (BtkTreeIter  *src_iter,
	       BtkTreeModel *dest,
	       BtkTreeIter  *dest_iter)
{
  gchar *label;
  BtkTreeViewColumn *column;
  gint before = -1;

  btk_tree_model_get (BTK_TREE_MODEL (left_tree_model),
		      src_iter, 0, &label, 1, &column, -1);
  btk_list_store_remove (BTK_LIST_STORE (left_tree_model), src_iter);

  if (dest_iter)
    {
      BtkTreePath *path = btk_tree_model_get_path (dest, dest_iter);
      before = (btk_tree_path_get_indices (path))[0];
      btk_tree_path_free (path);
    }
  
  if (dest == top_right_tree_model)
    btk_tree_view_insert_column (BTK_TREE_VIEW (sample_tree_view_top), column, before);
  else
    btk_tree_view_insert_column (BTK_TREE_VIEW (sample_tree_view_bottom), column, before);

  g_free (label);
}

static void
move_up_or_down (BtkTreeModel *src,
		 BtkTreeIter  *src_iter,
		 BtkTreeModel *dest,
		 BtkTreeIter  *dest_iter)
{
  BtkTreeViewColumn *column;
  gchar *label;
  gint before = -1;
  
  btk_tree_model_get (src, src_iter, 0, &label, 1, &column, -1);

  if (dest_iter)
    {
      BtkTreePath *path = btk_tree_model_get_path (dest, dest_iter);
      before = (btk_tree_path_get_indices (path))[0];
      btk_tree_path_free (path);
    }
  
  if (src == top_right_tree_model)
    btk_tree_view_remove_column (BTK_TREE_VIEW (sample_tree_view_top), column);
  else
    btk_tree_view_remove_column (BTK_TREE_VIEW (sample_tree_view_bottom), column);

  if (dest == top_right_tree_model)
    btk_tree_view_insert_column (BTK_TREE_VIEW (sample_tree_view_top), column, before);
  else
    btk_tree_view_insert_column (BTK_TREE_VIEW (sample_tree_view_bottom), column, before);

  g_free (label);
}

static void
move_row  (BtkTreeModel *src,
	   BtkTreeIter  *src_iter,
	   BtkTreeModel *dest,
	   BtkTreeIter  *dest_iter)
{
  if (src == left_tree_model)
    move_to_right (src_iter, dest, dest_iter);
  else if (dest == left_tree_model)
    move_to_left (src, src_iter, dest_iter);
  else 
    move_up_or_down (src, src_iter, dest, dest_iter);
}

static void
add_left_clicked (BtkWidget *button,
		  gpointer data)
{
  BtkTreeIter iter;

  BtkTreeSelection *selection = btk_tree_view_get_selection (BTK_TREE_VIEW (data));

  btk_tree_selection_get_selected (selection, NULL, &iter);

  move_to_left (btk_tree_view_get_model (BTK_TREE_VIEW (data)), &iter, NULL);
}

static void
add_right_clicked (BtkWidget *button, gpointer data)
{
  BtkTreeIter iter;

  BtkTreeSelection *selection = btk_tree_view_get_selection (BTK_TREE_VIEW (left_tree_view));

  btk_tree_selection_get_selected (selection, NULL, &iter);

  move_to_right (&iter, btk_tree_view_get_model (BTK_TREE_VIEW (data)), NULL);
}

static void
selection_changed (BtkTreeSelection *selection, BtkWidget *button)
{
  if (btk_tree_selection_get_selected (selection, NULL, NULL))
    btk_widget_set_sensitive (button, TRUE);
  else
    btk_widget_set_sensitive (button, FALSE);
}

static BtkTargetEntry row_targets[] = {
  { "BTK_TREE_MODEL_ROW", BTK_TARGET_SAME_APP, 0}
};

int
main (int argc, char *argv[])
{
  BtkWidget *window;
  BtkWidget *hbox, *vbox;
  BtkWidget *vbox2, *bbox;
  BtkWidget *button;
  BtkTreeViewColumn *column;
  BtkCellRenderer *cell;
  BtkWidget *swindow;
  BtkTreeModel *sample_model;
  gint i;

  btk_init (&argc, &argv);

  /* First initialize all the models for signal purposes */
  left_tree_model = (BtkTreeModel *) btk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
  sample_model = (BtkTreeModel *) btk_list_store_new (1, G_TYPE_STRING);
  sample_tree_view_top = btk_tree_view_new_with_model (sample_model);
  sample_tree_view_bottom = btk_tree_view_new_with_model (sample_model);
  top_right_tree_model = (BtkTreeModel *) view_column_model_new (BTK_TREE_VIEW (sample_tree_view_top));
  bottom_right_tree_model = (BtkTreeModel *) view_column_model_new (BTK_TREE_VIEW (sample_tree_view_bottom));
  top_right_tree_view = btk_tree_view_new_with_model (top_right_tree_model);
  bottom_right_tree_view = btk_tree_view_new_with_model (bottom_right_tree_model);

  for (i = 0; i < 10; i++)
    {
      BtkTreeIter iter;
      gchar *string = g_strdup_printf ("%d", i);
      btk_list_store_append (BTK_LIST_STORE (sample_model), &iter);
      btk_list_store_set (BTK_LIST_STORE (sample_model), &iter, 0, string, -1);
      g_free (string);
    }

  /* Set up the test windows. */
  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy", G_CALLBACK (btk_main_quit), NULL); 
  btk_window_set_default_size (BTK_WINDOW (window), 300, 300);
  btk_window_set_title (BTK_WINDOW (window), "Top Window");
  swindow = btk_scrolled_window_new (NULL, NULL);
  btk_container_add (BTK_CONTAINER (window), swindow);
  btk_container_add (BTK_CONTAINER (swindow), sample_tree_view_top);
  btk_widget_show_all (window);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy", G_CALLBACK (btk_main_quit), NULL); 
  btk_window_set_default_size (BTK_WINDOW (window), 300, 300);
  btk_window_set_title (BTK_WINDOW (window), "Bottom Window");
  swindow = btk_scrolled_window_new (NULL, NULL);
  btk_container_add (BTK_CONTAINER (window), swindow);
  btk_container_add (BTK_CONTAINER (swindow), sample_tree_view_bottom);
  btk_widget_show_all (window);

  /* Set up the main window */
  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy", G_CALLBACK (btk_main_quit), NULL); 
  btk_window_set_default_size (BTK_WINDOW (window), 500, 300);
  vbox = btk_vbox_new (FALSE, 8);
  btk_container_set_border_width (BTK_CONTAINER (vbox), 8);
  btk_container_add (BTK_CONTAINER (window), vbox);

  hbox = btk_hbox_new (FALSE, 8);
  btk_box_pack_start (BTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  /* Left Pane */
  cell = btk_cell_renderer_text_new ();

  swindow = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (swindow), BTK_POLICY_AUTOMATIC, BTK_POLICY_AUTOMATIC);
  left_tree_view = btk_tree_view_new_with_model (left_tree_model);
  btk_container_add (BTK_CONTAINER (swindow), left_tree_view);
  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (left_tree_view), -1,
					       "Unattached Columns", cell, "text", 0, NULL);
  cell = btk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (set_visible), left_tree_view);
  column = btk_tree_view_column_new_with_attributes ("Visible", cell, NULL);
  btk_tree_view_append_column (BTK_TREE_VIEW (left_tree_view), column);

  btk_tree_view_column_set_cell_data_func (column, cell, get_visible, NULL, NULL);
  btk_box_pack_start (BTK_BOX (hbox), swindow, TRUE, TRUE, 0);

  /* Middle Pane */
  vbox2 = btk_vbox_new (FALSE, 8);
  btk_box_pack_start (BTK_BOX (hbox), vbox2, FALSE, FALSE, 0);
  
  bbox = btk_vbutton_box_new ();
  btk_button_box_set_layout (BTK_BUTTON_BOX (bbox), BTK_BUTTONBOX_SPREAD);
  btk_box_pack_start (BTK_BOX (vbox2), bbox, TRUE, TRUE, 0);

  button = btk_button_new_with_mnemonic ("<< (_Q)");
  btk_widget_set_sensitive (button, FALSE);
  g_signal_connect (button, "clicked", G_CALLBACK (add_left_clicked), top_right_tree_view);
  g_signal_connect (btk_tree_view_get_selection (BTK_TREE_VIEW (top_right_tree_view)),
                    "changed", G_CALLBACK (selection_changed), button);
  btk_box_pack_start (BTK_BOX (bbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_mnemonic (">> (_W)");
  btk_widget_set_sensitive (button, FALSE);
  g_signal_connect (button, "clicked", G_CALLBACK (add_right_clicked), top_right_tree_view);
  g_signal_connect (btk_tree_view_get_selection (BTK_TREE_VIEW (left_tree_view)),
                    "changed", G_CALLBACK (selection_changed), button);
  btk_box_pack_start (BTK_BOX (bbox), button, FALSE, FALSE, 0);

  bbox = btk_vbutton_box_new ();
  btk_button_box_set_layout (BTK_BUTTON_BOX (bbox), BTK_BUTTONBOX_SPREAD);
  btk_box_pack_start (BTK_BOX (vbox2), bbox, TRUE, TRUE, 0);

  button = btk_button_new_with_mnemonic ("<< (_E)");
  btk_widget_set_sensitive (button, FALSE);
  g_signal_connect (button, "clicked", G_CALLBACK (add_left_clicked), bottom_right_tree_view);
  g_signal_connect (btk_tree_view_get_selection (BTK_TREE_VIEW (bottom_right_tree_view)),
                    "changed", G_CALLBACK (selection_changed), button);
  btk_box_pack_start (BTK_BOX (bbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_mnemonic (">> (_R)");
  btk_widget_set_sensitive (button, FALSE);
  g_signal_connect (button, "clicked", G_CALLBACK (add_right_clicked), bottom_right_tree_view);
  g_signal_connect (btk_tree_view_get_selection (BTK_TREE_VIEW (left_tree_view)),
                    "changed", G_CALLBACK (selection_changed), button);
  btk_box_pack_start (BTK_BOX (bbox), button, FALSE, FALSE, 0);

  
  /* Right Pane */
  vbox2 = btk_vbox_new (FALSE, 8);
  btk_box_pack_start (BTK_BOX (hbox), vbox2, TRUE, TRUE, 0);

  swindow = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (swindow), BTK_POLICY_AUTOMATIC, BTK_POLICY_AUTOMATIC);
  btk_tree_view_set_headers_visible (BTK_TREE_VIEW (top_right_tree_view), FALSE);
  cell = btk_cell_renderer_text_new ();
  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (top_right_tree_view), -1,
					       NULL, cell, "text", 0, NULL);
  cell = btk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (set_visible), top_right_tree_view);
  column = btk_tree_view_column_new_with_attributes (NULL, cell, NULL);
  btk_tree_view_column_set_cell_data_func (column, cell, get_visible, NULL, NULL);
  btk_tree_view_append_column (BTK_TREE_VIEW (top_right_tree_view), column);

  btk_container_add (BTK_CONTAINER (swindow), top_right_tree_view);
  btk_box_pack_start (BTK_BOX (vbox2), swindow, TRUE, TRUE, 0);

  swindow = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (swindow), BTK_POLICY_AUTOMATIC, BTK_POLICY_AUTOMATIC);
  btk_tree_view_set_headers_visible (BTK_TREE_VIEW (bottom_right_tree_view), FALSE);
  cell = btk_cell_renderer_text_new ();
  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (bottom_right_tree_view), -1,
					       NULL, cell, "text", 0, NULL);
  cell = btk_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (set_visible), bottom_right_tree_view);
  column = btk_tree_view_column_new_with_attributes (NULL, cell, NULL);
  btk_tree_view_column_set_cell_data_func (column, cell, get_visible, NULL, NULL);
  btk_tree_view_append_column (BTK_TREE_VIEW (bottom_right_tree_view), column);
  btk_container_add (BTK_CONTAINER (swindow), bottom_right_tree_view);
  btk_box_pack_start (BTK_BOX (vbox2), swindow, TRUE, TRUE, 0);

  
  /* Drag and Drop */
  btk_tree_view_enable_model_drag_source (BTK_TREE_VIEW (left_tree_view),
					  BDK_BUTTON1_MASK,
					  row_targets,
					  G_N_ELEMENTS (row_targets),
					  BDK_ACTION_MOVE);
  btk_tree_view_enable_model_drag_dest (BTK_TREE_VIEW (left_tree_view),
					row_targets,
					G_N_ELEMENTS (row_targets),
					BDK_ACTION_MOVE);

  btk_tree_view_enable_model_drag_source (BTK_TREE_VIEW (top_right_tree_view),
					  BDK_BUTTON1_MASK,
					  row_targets,
					  G_N_ELEMENTS (row_targets),
					  BDK_ACTION_MOVE);
  btk_tree_view_enable_model_drag_dest (BTK_TREE_VIEW (top_right_tree_view),
					row_targets,
					G_N_ELEMENTS (row_targets),
					BDK_ACTION_MOVE);

  btk_tree_view_enable_model_drag_source (BTK_TREE_VIEW (bottom_right_tree_view),
					  BDK_BUTTON1_MASK,
					  row_targets,
					  G_N_ELEMENTS (row_targets),
					  BDK_ACTION_MOVE);
  btk_tree_view_enable_model_drag_dest (BTK_TREE_VIEW (bottom_right_tree_view),
					row_targets,
					G_N_ELEMENTS (row_targets),
					BDK_ACTION_MOVE);


  btk_box_pack_start (BTK_BOX (vbox), btk_hseparator_new (), FALSE, FALSE, 0);

  hbox = btk_hbox_new (FALSE, 8);
  btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  button = btk_button_new_with_mnemonic ("_Add new Column");
  g_signal_connect (button, "clicked", G_CALLBACK (add_clicked), left_tree_model);
  btk_box_pack_start (BTK_BOX (hbox), button, FALSE, FALSE, 0);

  btk_widget_show_all (window);
  btk_main ();

  return 0;
}
