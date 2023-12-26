/* btkliststore.c
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
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
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <bobject/gvaluecollector.h>
#include "btktreemodel.h"
#include "btkliststore.h"
#include "btktreedatalist.h"
#include "btktreednd.h"
#include "btkintl.h"
#include "btkbuildable.h"
#include "btkbuilderprivate.h"
#include "btkalias.h"

#define BTK_LIST_STORE_IS_SORTED(list) (((BtkListStore*)(list))->sort_column_id != BTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID)
#define VALID_ITER(iter, list_store) ((iter)!= NULL && (iter)->user_data != NULL && list_store->stamp == (iter)->stamp && !g_sequence_iter_is_end ((iter)->user_data) && g_sequence_iter_get_sequence ((iter)->user_data) == list_store->seq)

static void         btk_list_store_tree_model_init (BtkTreeModelIface *iface);
static void         btk_list_store_drag_source_init(BtkTreeDragSourceIface *iface);
static void         btk_list_store_drag_dest_init  (BtkTreeDragDestIface   *iface);
static void         btk_list_store_sortable_init   (BtkTreeSortableIface   *iface);
static void         btk_list_store_buildable_init  (BtkBuildableIface      *iface);
static void         btk_list_store_finalize        (BObject           *object);
static BtkTreeModelFlags btk_list_store_get_flags  (BtkTreeModel      *tree_model);
static gint         btk_list_store_get_n_columns   (BtkTreeModel      *tree_model);
static GType        btk_list_store_get_column_type (BtkTreeModel      *tree_model,
						    gint               index);
static gboolean     btk_list_store_get_iter        (BtkTreeModel      *tree_model,
						    BtkTreeIter       *iter,
						    BtkTreePath       *path);
static BtkTreePath *btk_list_store_get_path        (BtkTreeModel      *tree_model,
						    BtkTreeIter       *iter);
static void         btk_list_store_get_value       (BtkTreeModel      *tree_model,
						    BtkTreeIter       *iter,
						    gint               column,
						    BValue            *value);
static gboolean     btk_list_store_iter_next       (BtkTreeModel      *tree_model,
						    BtkTreeIter       *iter);
static gboolean     btk_list_store_iter_children   (BtkTreeModel      *tree_model,
						    BtkTreeIter       *iter,
						    BtkTreeIter       *parent);
static gboolean     btk_list_store_iter_has_child  (BtkTreeModel      *tree_model,
						    BtkTreeIter       *iter);
static gint         btk_list_store_iter_n_children (BtkTreeModel      *tree_model,
						    BtkTreeIter       *iter);
static gboolean     btk_list_store_iter_nth_child  (BtkTreeModel      *tree_model,
						    BtkTreeIter       *iter,
						    BtkTreeIter       *parent,
						    gint               n);
static gboolean     btk_list_store_iter_parent     (BtkTreeModel      *tree_model,
						    BtkTreeIter       *iter,
						    BtkTreeIter       *child);


static void btk_list_store_set_n_columns   (BtkListStore *list_store,
					    gint          n_columns);
static void btk_list_store_set_column_type (BtkListStore *list_store,
					    gint          column,
					    GType         type);

static void btk_list_store_increment_stamp (BtkListStore *list_store);


/* Drag and Drop */
static gboolean real_btk_list_store_row_draggable (BtkTreeDragSource *drag_source,
                                                   BtkTreePath       *path);
static gboolean btk_list_store_drag_data_delete   (BtkTreeDragSource *drag_source,
                                                   BtkTreePath       *path);
static gboolean btk_list_store_drag_data_get      (BtkTreeDragSource *drag_source,
                                                   BtkTreePath       *path,
                                                   BtkSelectionData  *selection_data);
static gboolean btk_list_store_drag_data_received (BtkTreeDragDest   *drag_dest,
                                                   BtkTreePath       *dest,
                                                   BtkSelectionData  *selection_data);
static gboolean btk_list_store_row_drop_possible  (BtkTreeDragDest   *drag_dest,
                                                   BtkTreePath       *dest_path,
						   BtkSelectionData  *selection_data);


/* sortable */
static void     btk_list_store_sort                  (BtkListStore           *list_store);
static void     btk_list_store_sort_iter_changed     (BtkListStore           *list_store,
						      BtkTreeIter            *iter,
						      gint                    column);
static gboolean btk_list_store_get_sort_column_id    (BtkTreeSortable        *sortable,
						      gint                   *sort_column_id,
						      BtkSortType            *order);
static void     btk_list_store_set_sort_column_id    (BtkTreeSortable        *sortable,
						      gint                    sort_column_id,
						      BtkSortType             order);
static void     btk_list_store_set_sort_func         (BtkTreeSortable        *sortable,
						      gint                    sort_column_id,
						      BtkTreeIterCompareFunc  func,
						      gpointer                data,
						      GDestroyNotify          destroy);
static void     btk_list_store_set_default_sort_func (BtkTreeSortable        *sortable,
						      BtkTreeIterCompareFunc  func,
						      gpointer                data,
						      GDestroyNotify          destroy);
static gboolean btk_list_store_has_default_sort_func (BtkTreeSortable        *sortable);


/* buildable */
static gboolean btk_list_store_buildable_custom_tag_start (BtkBuildable  *buildable,
							   BtkBuilder    *builder,
							   BObject       *child,
							   const gchar   *tagname,
							   GMarkupParser *parser,
							   gpointer      *data);
static void     btk_list_store_buildable_custom_tag_end (BtkBuildable *buildable,
							 BtkBuilder   *builder,
							 BObject      *child,
							 const gchar  *tagname,
							 gpointer     *data);

G_DEFINE_TYPE_WITH_CODE (BtkListStore, btk_list_store, B_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_TREE_MODEL,
						btk_list_store_tree_model_init)
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_TREE_DRAG_SOURCE,
						btk_list_store_drag_source_init)
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_TREE_DRAG_DEST,
						btk_list_store_drag_dest_init)
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_TREE_SORTABLE,
						btk_list_store_sortable_init)
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
						btk_list_store_buildable_init))


static void
btk_list_store_class_init (BtkListStoreClass *class)
{
  BObjectClass *object_class;

  object_class = (BObjectClass*) class;

  object_class->finalize = btk_list_store_finalize;
}

static void
btk_list_store_tree_model_init (BtkTreeModelIface *iface)
{
  iface->get_flags = btk_list_store_get_flags;
  iface->get_n_columns = btk_list_store_get_n_columns;
  iface->get_column_type = btk_list_store_get_column_type;
  iface->get_iter = btk_list_store_get_iter;
  iface->get_path = btk_list_store_get_path;
  iface->get_value = btk_list_store_get_value;
  iface->iter_next = btk_list_store_iter_next;
  iface->iter_children = btk_list_store_iter_children;
  iface->iter_has_child = btk_list_store_iter_has_child;
  iface->iter_n_children = btk_list_store_iter_n_children;
  iface->iter_nth_child = btk_list_store_iter_nth_child;
  iface->iter_parent = btk_list_store_iter_parent;
}

static void
btk_list_store_drag_source_init (BtkTreeDragSourceIface *iface)
{
  iface->row_draggable = real_btk_list_store_row_draggable;
  iface->drag_data_delete = btk_list_store_drag_data_delete;
  iface->drag_data_get = btk_list_store_drag_data_get;
}

static void
btk_list_store_drag_dest_init (BtkTreeDragDestIface *iface)
{
  iface->drag_data_received = btk_list_store_drag_data_received;
  iface->row_drop_possible = btk_list_store_row_drop_possible;
}

static void
btk_list_store_sortable_init (BtkTreeSortableIface *iface)
{
  iface->get_sort_column_id = btk_list_store_get_sort_column_id;
  iface->set_sort_column_id = btk_list_store_set_sort_column_id;
  iface->set_sort_func = btk_list_store_set_sort_func;
  iface->set_default_sort_func = btk_list_store_set_default_sort_func;
  iface->has_default_sort_func = btk_list_store_has_default_sort_func;
}

void
btk_list_store_buildable_init (BtkBuildableIface *iface)
{
  iface->custom_tag_start = btk_list_store_buildable_custom_tag_start;
  iface->custom_tag_end = btk_list_store_buildable_custom_tag_end;
}

static void
btk_list_store_init (BtkListStore *list_store)
{
  list_store->seq = g_sequence_new (NULL);
  list_store->sort_list = NULL;
  list_store->stamp = g_random_int ();
  list_store->sort_column_id = BTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID;
  list_store->columns_dirty = FALSE;
  list_store->length = 0;
}

/**
 * btk_list_store_new:
 * @n_columns: number of columns in the list store
 * @Varargs: all #GType types for the columns, from first to last
 *
 * Creates a new list store as with @n_columns columns each of the types passed
 * in.  Note that only types derived from standard BObject fundamental types 
 * are supported. 
 *
 * As an example, <literal>btk_tree_store_new (3, B_TYPE_INT, B_TYPE_STRING,
 * BDK_TYPE_PIXBUF);</literal> will create a new #BtkListStore with three columns, of type
 * int, string and #BdkPixbuf respectively.
 *
 * Return value: a new #BtkListStore
 **/
BtkListStore *
btk_list_store_new (gint n_columns,
		    ...)
{
  BtkListStore *retval;
  va_list args;
  gint i;

  g_return_val_if_fail (n_columns > 0, NULL);

  retval = g_object_new (BTK_TYPE_LIST_STORE, NULL);
  btk_list_store_set_n_columns (retval, n_columns);

  va_start (args, n_columns);

  for (i = 0; i < n_columns; i++)
    {
      GType type = va_arg (args, GType);
      if (! _btk_tree_data_list_check_type (type))
	{
	  g_warning ("%s: Invalid type %s\n", B_STRLOC, g_type_name (type));
	  g_object_unref (retval);
	  return NULL;
	}

      btk_list_store_set_column_type (retval, i, type);
    }

  va_end (args);

  return retval;
}


/**
 * btk_list_store_newv:
 * @n_columns: number of columns in the list store
 * @types: (array length=n_columns): an array of #GType types for the columns, from first to last
 *
 * Non-vararg creation function.  Used primarily by language bindings.
 *
 * Return value: (transfer none): a new #BtkListStore
 **/
BtkListStore *
btk_list_store_newv (gint   n_columns,
		     GType *types)
{
  BtkListStore *retval;
  gint i;

  g_return_val_if_fail (n_columns > 0, NULL);

  retval = g_object_new (BTK_TYPE_LIST_STORE, NULL);
  btk_list_store_set_n_columns (retval, n_columns);

  for (i = 0; i < n_columns; i++)
    {
      if (! _btk_tree_data_list_check_type (types[i]))
	{
	  g_warning ("%s: Invalid type %s\n", B_STRLOC, g_type_name (types[i]));
	  g_object_unref (retval);
	  return NULL;
	}

      btk_list_store_set_column_type (retval, i, types[i]);
    }

  return retval;
}

/**
 * btk_list_store_set_column_types:
 * @list_store: A #BtkListStore
 * @n_columns: Number of columns for the list store
 * @types: (array length=n_columns): An array length n of #GTypes
 *
 * This function is meant primarily for #BObjects that inherit from #BtkListStore,
 * and should only be used when constructing a new #BtkListStore.  It will not
 * function after a row has been added, or a method on the #BtkTreeModel
 * interface is called.
 **/
void
btk_list_store_set_column_types (BtkListStore *list_store,
				 gint          n_columns,
				 GType        *types)
{
  gint i;

  g_return_if_fail (BTK_IS_LIST_STORE (list_store));
  g_return_if_fail (list_store->columns_dirty == 0);

  btk_list_store_set_n_columns (list_store, n_columns);
  for (i = 0; i < n_columns; i++)
    {
      if (! _btk_tree_data_list_check_type (types[i]))
	{
	  g_warning ("%s: Invalid type %s\n", B_STRLOC, g_type_name (types[i]));
	  continue;
	}
      btk_list_store_set_column_type (list_store, i, types[i]);
    }
}

static void
btk_list_store_set_n_columns (BtkListStore *list_store,
			      gint          n_columns)
{
  int i;

  if (list_store->n_columns == n_columns)
    return;

  list_store->column_headers = g_renew (GType, list_store->column_headers, n_columns);
  for (i = list_store->n_columns; i < n_columns; i++)
    list_store->column_headers[i] = B_TYPE_INVALID;
  list_store->n_columns = n_columns;

  if (list_store->sort_list)
    _btk_tree_data_list_header_free (list_store->sort_list);
  list_store->sort_list = _btk_tree_data_list_header_new (n_columns, list_store->column_headers);
}

static void
btk_list_store_set_column_type (BtkListStore *list_store,
				gint          column,
				GType         type)
{
  if (!_btk_tree_data_list_check_type (type))
    {
      g_warning ("%s: Invalid type %s\n", B_STRLOC, g_type_name (type));
      return;
    }

  list_store->column_headers[column] = type;
}

static void
btk_list_store_finalize (BObject *object)
{
  BtkListStore *list_store = BTK_LIST_STORE (object);

  g_sequence_foreach (list_store->seq,
		      (GFunc) _btk_tree_data_list_free, list_store->column_headers);

  g_sequence_free (list_store->seq);

  _btk_tree_data_list_header_free (list_store->sort_list);
  g_free (list_store->column_headers);
  
  if (list_store->default_sort_destroy)
    {
      GDestroyNotify d = list_store->default_sort_destroy;

      list_store->default_sort_destroy = NULL;
      d (list_store->default_sort_data);
      list_store->default_sort_data = NULL;
    }

  B_OBJECT_CLASS (btk_list_store_parent_class)->finalize (object);
}

/* Fulfill the BtkTreeModel requirements */
static BtkTreeModelFlags
btk_list_store_get_flags (BtkTreeModel *tree_model)
{
  return BTK_TREE_MODEL_ITERS_PERSIST | BTK_TREE_MODEL_LIST_ONLY;
}

static gint
btk_list_store_get_n_columns (BtkTreeModel *tree_model)
{
  BtkListStore *list_store = (BtkListStore *) tree_model;

  list_store->columns_dirty = TRUE;

  return list_store->n_columns;
}

static GType
btk_list_store_get_column_type (BtkTreeModel *tree_model,
				gint          index)
{
  BtkListStore *list_store = (BtkListStore *) tree_model;

  g_return_val_if_fail (index < BTK_LIST_STORE (tree_model)->n_columns, 
			B_TYPE_INVALID);

  list_store->columns_dirty = TRUE;

  return list_store->column_headers[index];
}

static gboolean
btk_list_store_get_iter (BtkTreeModel *tree_model,
			 BtkTreeIter  *iter,
			 BtkTreePath  *path)
{
  BtkListStore *list_store = (BtkListStore *) tree_model;
  GSequence *seq;
  gint i;

  list_store->columns_dirty = TRUE;

  seq = list_store->seq;
  
  i = btk_tree_path_get_indices (path)[0];

  if (i >= g_sequence_get_length (seq))
    return FALSE;

  iter->stamp = list_store->stamp;
  iter->user_data = g_sequence_get_iter_at_pos (seq, i);

  return TRUE;
}

static BtkTreePath *
btk_list_store_get_path (BtkTreeModel *tree_model,
			 BtkTreeIter  *iter)
{
  BtkTreePath *path;

  g_return_val_if_fail (iter->stamp == BTK_LIST_STORE (tree_model)->stamp, NULL);

  if (g_sequence_iter_is_end (iter->user_data))
    return NULL;
	
  path = btk_tree_path_new ();
  btk_tree_path_append_index (path, g_sequence_iter_get_position (iter->user_data));
  
  return path;
}

static void
btk_list_store_get_value (BtkTreeModel *tree_model,
			  BtkTreeIter  *iter,
			  gint          column,
			  BValue       *value)
{
  BtkListStore *list_store = (BtkListStore *) tree_model;
  BtkTreeDataList *list;
  gint tmp_column = column;

  g_return_if_fail (column < list_store->n_columns);
  g_return_if_fail (VALID_ITER (iter, list_store));
		    
  list = g_sequence_get (iter->user_data);

  while (tmp_column-- > 0 && list)
    list = list->next;

  if (list == NULL)
    b_value_init (value, list_store->column_headers[column]);
  else
    _btk_tree_data_list_node_to_value (list,
				       list_store->column_headers[column],
				       value);
}

static gboolean
btk_list_store_iter_next (BtkTreeModel  *tree_model,
			  BtkTreeIter   *iter)
{
  gboolean retval;

  g_return_val_if_fail (BTK_LIST_STORE (tree_model)->stamp == iter->stamp, FALSE);
  iter->user_data = g_sequence_iter_next (iter->user_data);

  retval = g_sequence_iter_is_end (iter->user_data);
  if (retval)
    iter->stamp = 0;

  return !retval;
}

static gboolean
btk_list_store_iter_children (BtkTreeModel *tree_model,
			      BtkTreeIter  *iter,
			      BtkTreeIter  *parent)
{
  BtkListStore *list_store = (BtkListStore *) tree_model;
  
  /* this is a list, nodes have no children */
  if (parent)
    {
      iter->stamp = 0;
      return FALSE;
    }

  if (g_sequence_get_length (list_store->seq) > 0)
    {
      iter->stamp = list_store->stamp;
      iter->user_data = g_sequence_get_begin_iter (list_store->seq);
      return TRUE;
    }
  else
    {
      iter->stamp = 0;
      return FALSE;
    }
}

static gboolean
btk_list_store_iter_has_child (BtkTreeModel *tree_model,
			       BtkTreeIter  *iter)
{
  return FALSE;
}

static gint
btk_list_store_iter_n_children (BtkTreeModel *tree_model,
				BtkTreeIter  *iter)
{
  BtkListStore *list_store = (BtkListStore *) tree_model;

  if (iter == NULL)
    return g_sequence_get_length (list_store->seq);

  g_return_val_if_fail (list_store->stamp == iter->stamp, -1);

  return 0;
}

static gboolean
btk_list_store_iter_nth_child (BtkTreeModel *tree_model,
			       BtkTreeIter  *iter,
			       BtkTreeIter  *parent,
			       gint          n)
{
  BtkListStore *list_store = (BtkListStore *) tree_model;
  GSequenceIter *child;

  iter->stamp = 0;

  if (parent)
    return FALSE;

  child = g_sequence_get_iter_at_pos (list_store->seq, n);

  if (g_sequence_iter_is_end (child))
    return FALSE;

  iter->stamp = list_store->stamp;
  iter->user_data = child;

  return TRUE;
}

static gboolean
btk_list_store_iter_parent (BtkTreeModel *tree_model,
			    BtkTreeIter  *iter,
			    BtkTreeIter  *child)
{
  iter->stamp = 0;
  return FALSE;
}

static gboolean
btk_list_store_real_set_value (BtkListStore *list_store,
			       BtkTreeIter  *iter,
			       gint          column,
			       BValue       *value,
			       gboolean      sort)
{
  BtkTreeDataList *list;
  BtkTreeDataList *prev;
  gint old_column = column;
  BValue real_value = {0, };
  gboolean converted = FALSE;
  gboolean retval = FALSE;

  if (! g_type_is_a (G_VALUE_TYPE (value), list_store->column_headers[column]))
    {
      if (! (b_value_type_compatible (G_VALUE_TYPE (value), list_store->column_headers[column]) &&
	     b_value_type_compatible (list_store->column_headers[column], G_VALUE_TYPE (value))))
	{
	  g_warning ("%s: Unable to convert from %s to %s\n",
		     B_STRLOC,
		     g_type_name (G_VALUE_TYPE (value)),
		     g_type_name (list_store->column_headers[column]));
	  return retval;
	}
      if (!b_value_transform (value, &real_value))
	{
	  g_warning ("%s: Unable to make conversion from %s to %s\n",
		     B_STRLOC,
		     g_type_name (G_VALUE_TYPE (value)),
		     g_type_name (list_store->column_headers[column]));
	  b_value_unset (&real_value);
	  return retval;
	}
      converted = TRUE;
    }

  prev = list = g_sequence_get (iter->user_data);

  while (list != NULL)
    {
      if (column == 0)
	{
	  if (converted)
	    _btk_tree_data_list_value_to_node (list, &real_value);
	  else
	    _btk_tree_data_list_value_to_node (list, value);
	  retval = TRUE;
	  if (converted)
	    b_value_unset (&real_value);
         if (sort && BTK_LIST_STORE_IS_SORTED (list_store))
            btk_list_store_sort_iter_changed (list_store, iter, old_column);
	  return retval;
	}

      column--;
      prev = list;
      list = list->next;
    }

  if (g_sequence_get (iter->user_data) == NULL)
    {
      list = _btk_tree_data_list_alloc();
      g_sequence_set (iter->user_data, list);
      list->next = NULL;
    }
  else
    {
      list = prev->next = _btk_tree_data_list_alloc ();
      list->next = NULL;
    }

  while (column != 0)
    {
      list->next = _btk_tree_data_list_alloc ();
      list = list->next;
      list->next = NULL;
      column --;
    }

  if (converted)
    _btk_tree_data_list_value_to_node (list, &real_value);
  else
    _btk_tree_data_list_value_to_node (list, value);

  retval = TRUE;
  if (converted)
    b_value_unset (&real_value);

  if (sort && BTK_LIST_STORE_IS_SORTED (list_store))
    btk_list_store_sort_iter_changed (list_store, iter, old_column);

  return retval;
}


/**
 * btk_list_store_set_value:
 * @list_store: A #BtkListStore
 * @iter: A valid #BtkTreeIter for the row being modified
 * @column: column number to modify
 * @value: new value for the cell
 *
 * Sets the data in the cell specified by @iter and @column.
 * The type of @value must be convertible to the type of the
 * column.
 *
 **/
void
btk_list_store_set_value (BtkListStore *list_store,
			  BtkTreeIter  *iter,
			  gint          column,
			  BValue       *value)
{
  g_return_if_fail (BTK_IS_LIST_STORE (list_store));
  g_return_if_fail (VALID_ITER (iter, list_store));
  g_return_if_fail (column >= 0 && column < list_store->n_columns);
  g_return_if_fail (G_IS_VALUE (value));

  if (btk_list_store_real_set_value (list_store, iter, column, value, TRUE))
    {
      BtkTreePath *path;

      path = btk_list_store_get_path (BTK_TREE_MODEL (list_store), iter);
      btk_tree_model_row_changed (BTK_TREE_MODEL (list_store), path, iter);
      btk_tree_path_free (path);
    }
}

static BtkTreeIterCompareFunc
btk_list_store_get_compare_func (BtkListStore *list_store)
{
  BtkTreeIterCompareFunc func = NULL;

  if (BTK_LIST_STORE_IS_SORTED (list_store))
    {
      if (list_store->sort_column_id != -1)
	{
	  BtkTreeDataSortHeader *header;
	  header = _btk_tree_data_list_get_header (list_store->sort_list,
						   list_store->sort_column_id);
	  g_return_val_if_fail (header != NULL, NULL);
	  g_return_val_if_fail (header->func != NULL, NULL);
	  func = header->func;
	}
      else
	{
	  func = list_store->default_sort_func;
	}
    }

  return func;
}

static void
btk_list_store_set_vector_internal (BtkListStore *list_store,
				    BtkTreeIter  *iter,
				    gboolean     *emit_signal,
				    gboolean     *maybe_need_sort,
				    gint         *columns,
				    BValue       *values,
				    gint          n_values)
{
  gint i;
  BtkTreeIterCompareFunc func = NULL;

  func = btk_list_store_get_compare_func (list_store);
  if (func != _btk_tree_data_list_compare_func)
    *maybe_need_sort = TRUE;

  for (i = 0; i < n_values; i++)
    {
      *emit_signal = btk_list_store_real_set_value (list_store, 
					       iter, 
					       columns[i],
					       &values[i],
					       FALSE) || *emit_signal;

      if (func == _btk_tree_data_list_compare_func &&
	  columns[i] == list_store->sort_column_id)
	*maybe_need_sort = TRUE;
    }
}

static void
btk_list_store_set_valist_internal (BtkListStore *list_store,
				    BtkTreeIter  *iter,
				    gboolean     *emit_signal,
				    gboolean     *maybe_need_sort,
				    va_list	  var_args)
{
  gint column;
  BtkTreeIterCompareFunc func = NULL;

  column = va_arg (var_args, gint);

  func = btk_list_store_get_compare_func (list_store);
  if (func != _btk_tree_data_list_compare_func)
    *maybe_need_sort = TRUE;

  while (column != -1)
    {
      BValue value = { 0, };
      gchar *error = NULL;

      if (column < 0 || column >= list_store->n_columns)
	{
	  g_warning ("%s: Invalid column number %d added to iter (remember to end your list of columns with a -1)", B_STRLOC, column);
	  break;
	}
      b_value_init (&value, list_store->column_headers[column]);

      G_VALUE_COLLECT (&value, var_args, 0, &error);
      if (error)
	{
	  g_warning ("%s: %s", B_STRLOC, error);
	  g_free (error);

 	  /* we purposely leak the value here, it might not be
	   * in a sane state if an error condition occoured
	   */
	  break;
	}

      /* FIXME: instead of calling this n times, refactor with above */
      *emit_signal = btk_list_store_real_set_value (list_store,
						    iter,
						    column,
						    &value,
						    FALSE) || *emit_signal;
      
      if (func == _btk_tree_data_list_compare_func &&
	  column == list_store->sort_column_id)
	*maybe_need_sort = TRUE;

      b_value_unset (&value);

      column = va_arg (var_args, gint);
    }
}

/**
 * btk_list_store_set_valuesv:
 * @list_store: A #BtkListStore
 * @iter: A valid #BtkTreeIter for the row being modified
 * @columns: (array length=n_values): an array of column numbers
 * @values: (array length=n_values): an array of BValues
 * @n_values: the length of the @columns and @values arrays
 *
 * A variant of btk_list_store_set_valist() which
 * takes the columns and values as two arrays, instead of
 * varargs. This function is mainly intended for 
 * language-bindings and in case the number of columns to
 * change is not known until run-time.
 *
 * Since: 2.12
 */
void
btk_list_store_set_valuesv (BtkListStore *list_store,
			    BtkTreeIter  *iter,
			    gint         *columns,
			    BValue       *values,
			    gint          n_values)
{
  gboolean emit_signal = FALSE;
  gboolean maybe_need_sort = FALSE;

  g_return_if_fail (BTK_IS_LIST_STORE (list_store));
  g_return_if_fail (VALID_ITER (iter, list_store));

  btk_list_store_set_vector_internal (list_store, iter,
				      &emit_signal,
				      &maybe_need_sort,
				      columns, values, n_values);

  if (maybe_need_sort && BTK_LIST_STORE_IS_SORTED (list_store))
    btk_list_store_sort_iter_changed (list_store, iter, list_store->sort_column_id);

  if (emit_signal)
    {
      BtkTreePath *path;

      path = btk_list_store_get_path (BTK_TREE_MODEL (list_store), iter);
      btk_tree_model_row_changed (BTK_TREE_MODEL (list_store), path, iter);
      btk_tree_path_free (path);
    }
}

/**
 * btk_list_store_set_valist:
 * @list_store: A #BtkListStore
 * @iter: A valid #BtkTreeIter for the row being modified
 * @var_args: va_list of column/value pairs
 *
 * See btk_list_store_set(); this version takes a va_list for use by language
 * bindings.
 *
 **/
void
btk_list_store_set_valist (BtkListStore *list_store,
                           BtkTreeIter  *iter,
                           va_list	 var_args)
{
  gboolean emit_signal = FALSE;
  gboolean maybe_need_sort = FALSE;

  g_return_if_fail (BTK_IS_LIST_STORE (list_store));
  g_return_if_fail (VALID_ITER (iter, list_store));

  btk_list_store_set_valist_internal (list_store, iter, 
				      &emit_signal, 
				      &maybe_need_sort,
				      var_args);

  if (maybe_need_sort && BTK_LIST_STORE_IS_SORTED (list_store))
    btk_list_store_sort_iter_changed (list_store, iter, list_store->sort_column_id);

  if (emit_signal)
    {
      BtkTreePath *path;

      path = btk_list_store_get_path (BTK_TREE_MODEL (list_store), iter);
      btk_tree_model_row_changed (BTK_TREE_MODEL (list_store), path, iter);
      btk_tree_path_free (path);
    }
}

/**
 * btk_list_store_set:
 * @list_store: a #BtkListStore
 * @iter: row iterator
 * @Varargs: pairs of column number and value, terminated with -1
 *
 * Sets the value of one or more cells in the row referenced by @iter.
 * The variable argument list should contain integer column numbers,
 * each column number followed by the value to be set.
 * The list is terminated by a -1. For example, to set column 0 with type
 * %B_TYPE_STRING to "Foo", you would write <literal>btk_list_store_set (store, iter,
 * 0, "Foo", -1)</literal>.
 *
 * The value will be referenced by the store if it is a %B_TYPE_OBJECT, and it
 * will be copied if it is a %B_TYPE_STRING or %B_TYPE_BOXED.
 **/
void
btk_list_store_set (BtkListStore *list_store,
		    BtkTreeIter  *iter,
		    ...)
{
  va_list var_args;

  va_start (var_args, iter);
  btk_list_store_set_valist (list_store, iter, var_args);
  va_end (var_args);
}

/**
 * btk_list_store_remove:
 * @list_store: A #BtkListStore
 * @iter: A valid #BtkTreeIter
 *
 * Removes the given row from the list store.  After being removed, 
 * @iter is set to be the next valid row, or invalidated if it pointed 
 * to the last row in @list_store.
 *
 * Return value: %TRUE if @iter is valid, %FALSE if not.
 **/
gboolean
btk_list_store_remove (BtkListStore *list_store,
		       BtkTreeIter  *iter)
{
  BtkTreePath *path;
  GSequenceIter *ptr, *next;

  g_return_val_if_fail (BTK_IS_LIST_STORE (list_store), FALSE);
  g_return_val_if_fail (VALID_ITER (iter, list_store), FALSE);

  path = btk_list_store_get_path (BTK_TREE_MODEL (list_store), iter);

  ptr = iter->user_data;
  next = g_sequence_iter_next (ptr);
  
  _btk_tree_data_list_free (g_sequence_get (ptr), list_store->column_headers);
  g_sequence_remove (iter->user_data);

  list_store->length--;
  
  btk_tree_model_row_deleted (BTK_TREE_MODEL (list_store), path);
  btk_tree_path_free (path);

  if (g_sequence_iter_is_end (next))
    {
      iter->stamp = 0;
      return FALSE;
    }
  else
    {
      iter->stamp = list_store->stamp;
      iter->user_data = next;
      return TRUE;
    }
}

/**
 * btk_list_store_insert:
 * @list_store: A #BtkListStore
 * @iter: (out): An unset #BtkTreeIter to set to the new row
 * @position: position to insert the new row
 *
 * Creates a new row at @position.  @iter will be changed to point to this new
 * row.  If @position is larger than the number of rows on the list, then the
 * new row will be appended to the list. The row will be empty after this
 * function is called.  To fill in values, you need to call 
 * btk_list_store_set() or btk_list_store_set_value().
 *
 **/
void
btk_list_store_insert (BtkListStore *list_store,
		       BtkTreeIter  *iter,
		       gint          position)
{
  BtkTreePath *path;
  GSequence *seq;
  GSequenceIter *ptr;
  gint length;

  g_return_if_fail (BTK_IS_LIST_STORE (list_store));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (position >= 0);

  list_store->columns_dirty = TRUE;

  seq = list_store->seq;

  length = g_sequence_get_length (seq);
  if (position > length)
    position = length;

  ptr = g_sequence_get_iter_at_pos (seq, position);
  ptr = g_sequence_insert_before (ptr, NULL);

  iter->stamp = list_store->stamp;
  iter->user_data = ptr;

  g_assert (VALID_ITER (iter, list_store));

  list_store->length++;
  
  path = btk_tree_path_new ();
  btk_tree_path_append_index (path, position);
  btk_tree_model_row_inserted (BTK_TREE_MODEL (list_store), path, iter);
  btk_tree_path_free (path);
}

/**
 * btk_list_store_insert_before:
 * @list_store: A #BtkListStore
 * @iter: (out): An unset #BtkTreeIter to set to the new row
 * @sibling: (allow-none): A valid #BtkTreeIter, or %NULL
 *
 * Inserts a new row before @sibling. If @sibling is %NULL, then the row will 
 * be appended to the end of the list. @iter will be changed to point to this 
 * new row. The row will be empty after this function is called. To fill in 
 * values, you need to call btk_list_store_set() or btk_list_store_set_value().
 *
 **/
void
btk_list_store_insert_before (BtkListStore *list_store,
			      BtkTreeIter  *iter,
			      BtkTreeIter  *sibling)
{
  GSequenceIter *after;
  
  g_return_if_fail (BTK_IS_LIST_STORE (list_store));
  g_return_if_fail (iter != NULL);
  if (sibling)
    g_return_if_fail (VALID_ITER (sibling, list_store));

  if (!sibling)
    after = g_sequence_get_end_iter (list_store->seq);
  else
    after = sibling->user_data;

  btk_list_store_insert (list_store, iter, g_sequence_iter_get_position (after));
}

/**
 * btk_list_store_insert_after:
 * @list_store: A #BtkListStore
 * @iter: (out): An unset #BtkTreeIter to set to the new row
 * @sibling: (allow-none): A valid #BtkTreeIter, or %NULL
 *
 * Inserts a new row after @sibling. If @sibling is %NULL, then the row will be
 * prepended to the beginning of the list. @iter will be changed to point to
 * this new row. The row will be empty after this function is called. To fill
 * in values, you need to call btk_list_store_set() or btk_list_store_set_value().
 *
 **/
void
btk_list_store_insert_after (BtkListStore *list_store,
			     BtkTreeIter  *iter,
			     BtkTreeIter  *sibling)
{
  GSequenceIter *after;

  g_return_if_fail (BTK_IS_LIST_STORE (list_store));
  g_return_if_fail (iter != NULL);
  if (sibling)
    g_return_if_fail (VALID_ITER (sibling, list_store));

  if (!sibling)
    after = g_sequence_get_begin_iter (list_store->seq);
  else
    after = g_sequence_iter_next (sibling->user_data);

  btk_list_store_insert (list_store, iter, g_sequence_iter_get_position (after));
}

/**
 * btk_list_store_prepend:
 * @list_store: A #BtkListStore
 * @iter: (out): An unset #BtkTreeIter to set to the prepend row
 *
 * Prepends a new row to @list_store. @iter will be changed to point to this new
 * row. The row will be empty after this function is called. To fill in
 * values, you need to call btk_list_store_set() or btk_list_store_set_value().
 *
 **/
void
btk_list_store_prepend (BtkListStore *list_store,
			BtkTreeIter  *iter)
{
  g_return_if_fail (BTK_IS_LIST_STORE (list_store));
  g_return_if_fail (iter != NULL);

  btk_list_store_insert (list_store, iter, 0);
}

/**
 * btk_list_store_append:
 * @list_store: A #BtkListStore
 * @iter: (out): An unset #BtkTreeIter to set to the appended row
 *
 * Appends a new row to @list_store.  @iter will be changed to point to this new
 * row.  The row will be empty after this function is called.  To fill in
 * values, you need to call btk_list_store_set() or btk_list_store_set_value().
 *
 **/
void
btk_list_store_append (BtkListStore *list_store,
		       BtkTreeIter  *iter)
{
  g_return_if_fail (BTK_IS_LIST_STORE (list_store));
  g_return_if_fail (iter != NULL);

  btk_list_store_insert (list_store, iter, g_sequence_get_length (list_store->seq));
}

static void
btk_list_store_increment_stamp (BtkListStore *list_store)
{
  do
    {
      list_store->stamp++;
    }
  while (list_store->stamp == 0);
}

/**
 * btk_list_store_clear:
 * @list_store: a #BtkListStore.
 *
 * Removes all rows from the list store.  
 *
 **/
void
btk_list_store_clear (BtkListStore *list_store)
{
  BtkTreeIter iter;
  g_return_if_fail (BTK_IS_LIST_STORE (list_store));

  while (g_sequence_get_length (list_store->seq) > 0)
    {
      iter.stamp = list_store->stamp;
      iter.user_data = g_sequence_get_begin_iter (list_store->seq);
      btk_list_store_remove (list_store, &iter);
    }

  btk_list_store_increment_stamp (list_store);
}

/**
 * btk_list_store_iter_is_valid:
 * @list_store: A #BtkListStore.
 * @iter: A #BtkTreeIter.
 *
 * <warning>This function is slow. Only use it for debugging and/or testing
 * purposes.</warning>
 *
 * Checks if the given iter is a valid iter for this #BtkListStore.
 *
 * Return value: %TRUE if the iter is valid, %FALSE if the iter is invalid.
 *
 * Since: 2.2
 **/
gboolean
btk_list_store_iter_is_valid (BtkListStore *list_store,
                              BtkTreeIter  *iter)
{
  g_return_val_if_fail (BTK_IS_LIST_STORE (list_store), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  if (!VALID_ITER (iter, list_store))
    return FALSE;

  if (g_sequence_iter_get_sequence (iter->user_data) != list_store->seq)
    return FALSE;

  return TRUE;
}

static gboolean real_btk_list_store_row_draggable (BtkTreeDragSource *drag_source,
                                                   BtkTreePath       *path)
{
  return TRUE;
}
  
static gboolean
btk_list_store_drag_data_delete (BtkTreeDragSource *drag_source,
                                 BtkTreePath       *path)
{
  BtkTreeIter iter;

  if (btk_list_store_get_iter (BTK_TREE_MODEL (drag_source),
                               &iter,
                               path))
    {
      btk_list_store_remove (BTK_LIST_STORE (drag_source), &iter);
      return TRUE;
    }
  return FALSE;
}

static gboolean
btk_list_store_drag_data_get (BtkTreeDragSource *drag_source,
                              BtkTreePath       *path,
                              BtkSelectionData  *selection_data)
{
  /* Note that we don't need to handle the BTK_TREE_MODEL_ROW
   * target, because the default handler does it for us, but
   * we do anyway for the convenience of someone maybe overriding the
   * default handler.
   */

  if (btk_tree_set_row_drag_data (selection_data,
				  BTK_TREE_MODEL (drag_source),
				  path))
    {
      return TRUE;
    }
  else
    {
      /* FIXME handle text targets at least. */
    }

  return FALSE;
}

static gboolean
btk_list_store_drag_data_received (BtkTreeDragDest   *drag_dest,
                                   BtkTreePath       *dest,
                                   BtkSelectionData  *selection_data)
{
  BtkTreeModel *tree_model;
  BtkListStore *list_store;
  BtkTreeModel *src_model = NULL;
  BtkTreePath *src_path = NULL;
  gboolean retval = FALSE;

  tree_model = BTK_TREE_MODEL (drag_dest);
  list_store = BTK_LIST_STORE (drag_dest);

  if (btk_tree_get_row_drag_data (selection_data,
				  &src_model,
				  &src_path) &&
      src_model == tree_model)
    {
      /* Copy the given row to a new position */
      BtkTreeIter src_iter;
      BtkTreeIter dest_iter;
      BtkTreePath *prev;

      if (!btk_list_store_get_iter (src_model,
                                    &src_iter,
                                    src_path))
        {
          goto out;
        }

      /* Get the path to insert _after_ (dest is the path to insert _before_) */
      prev = btk_tree_path_copy (dest);

      if (!btk_tree_path_prev (prev))
        {
          /* dest was the first spot in the list; which means we are supposed
           * to prepend.
           */
          btk_list_store_prepend (list_store, &dest_iter);

          retval = TRUE;
        }
      else
        {
          if (btk_list_store_get_iter (tree_model, &dest_iter, prev))
            {
              BtkTreeIter tmp_iter = dest_iter;

              btk_list_store_insert_after (list_store, &dest_iter, &tmp_iter);

              retval = TRUE;
            }
        }

      btk_tree_path_free (prev);

      /* If we succeeded in creating dest_iter, copy data from src
       */
      if (retval)
        {
          BtkTreeDataList *dl = g_sequence_get (src_iter.user_data);
          BtkTreeDataList *copy_head = NULL;
          BtkTreeDataList *copy_prev = NULL;
          BtkTreeDataList *copy_iter = NULL;
	  BtkTreePath *path;
          gint col;

          col = 0;
          while (dl)
            {
              copy_iter = _btk_tree_data_list_node_copy (dl,
                                                         list_store->column_headers[col]);

              if (copy_head == NULL)
                copy_head = copy_iter;

              if (copy_prev)
                copy_prev->next = copy_iter;

              copy_prev = copy_iter;

              dl = dl->next;
              ++col;
            }

	  dest_iter.stamp = list_store->stamp;
          g_sequence_set (dest_iter.user_data, copy_head);

	  path = btk_list_store_get_path (tree_model, &dest_iter);
	  btk_tree_model_row_changed (tree_model, path, &dest_iter);
	  btk_tree_path_free (path);
	}
    }
  else
    {
      /* FIXME maybe add some data targets eventually, or handle text
       * targets in the simple case.
       */
    }

 out:

  if (src_path)
    btk_tree_path_free (src_path);

  return retval;
}

static gboolean
btk_list_store_row_drop_possible (BtkTreeDragDest  *drag_dest,
                                  BtkTreePath      *dest_path,
				  BtkSelectionData *selection_data)
{
  gint *indices;
  BtkTreeModel *src_model = NULL;
  BtkTreePath *src_path = NULL;
  gboolean retval = FALSE;

  /* don't accept drops if the list has been sorted */
  if (BTK_LIST_STORE_IS_SORTED (drag_dest))
    return FALSE;

  if (!btk_tree_get_row_drag_data (selection_data,
				   &src_model,
				   &src_path))
    goto out;

  if (src_model != BTK_TREE_MODEL (drag_dest))
    goto out;

  if (btk_tree_path_get_depth (dest_path) != 1)
    goto out;

  /* can drop before any existing node, or before one past any existing. */

  indices = btk_tree_path_get_indices (dest_path);

  if (indices[0] <= g_sequence_get_length (BTK_LIST_STORE (drag_dest)->seq))
    retval = TRUE;

 out:
  if (src_path)
    btk_tree_path_free (src_path);
  
  return retval;
}

/* Sorting and reordering */

/* Reordering */
static gint
btk_list_store_reorder_func (GSequenceIter *a,
			     GSequenceIter *b,
			     gpointer       user_data)
{
  GHashTable *new_positions = user_data;
  gint apos = GPOINTER_TO_INT (g_hash_table_lookup (new_positions, a));
  gint bpos = GPOINTER_TO_INT (g_hash_table_lookup (new_positions, b));

  if (apos < bpos)
    return -1;
  if (apos > bpos)
    return 1;
  return 0;
}
  
/**
 * btk_list_store_reorder:
 * @store: A #BtkListStore.
 * @new_order: (array): an array of integers mapping the new position of each child
 *      to its old position before the re-ordering,
 *      i.e. @new_order<literal>[newpos] = oldpos</literal>.
 *
 * Reorders @store to follow the order indicated by @new_order. Note that
 * this function only works with unsorted stores.
 *
 * Since: 2.2
 **/
void
btk_list_store_reorder (BtkListStore *store,
			gint         *new_order)
{
  gint i;
  BtkTreePath *path;
  GHashTable *new_positions;
  GSequenceIter *ptr;
  gint *order;
  
  g_return_if_fail (BTK_IS_LIST_STORE (store));
  g_return_if_fail (!BTK_LIST_STORE_IS_SORTED (store));
  g_return_if_fail (new_order != NULL);

  order = g_new (gint, g_sequence_get_length (store->seq));
  for (i = 0; i < g_sequence_get_length (store->seq); i++)
    order[new_order[i]] = i;
  
  new_positions = g_hash_table_new (g_direct_hash, g_direct_equal);

  ptr = g_sequence_get_begin_iter (store->seq);
  i = 0;
  while (!g_sequence_iter_is_end (ptr))
    {
      g_hash_table_insert (new_positions, ptr, GINT_TO_POINTER (order[i++]));

      ptr = g_sequence_iter_next (ptr);
    }
  g_free (order);
  
  g_sequence_sort_iter (store->seq, btk_list_store_reorder_func, new_positions);

  g_hash_table_destroy (new_positions);
  
  /* emit signal */
  path = btk_tree_path_new ();
  btk_tree_model_rows_reordered (BTK_TREE_MODEL (store),
				 path, NULL, new_order);
  btk_tree_path_free (path);
}

static GHashTable *
save_positions (GSequence *seq)
{
  GHashTable *positions = g_hash_table_new (g_direct_hash, g_direct_equal);
  GSequenceIter *ptr;

  ptr = g_sequence_get_begin_iter (seq);
  while (!g_sequence_iter_is_end (ptr))
    {
      g_hash_table_insert (positions, ptr,
			   GINT_TO_POINTER (g_sequence_iter_get_position (ptr)));
      ptr = g_sequence_iter_next (ptr);
    }

  return positions;
}

static int *
generate_order (GSequence *seq,
		GHashTable *old_positions)
{
  GSequenceIter *ptr;
  int *order = g_new (int, g_sequence_get_length (seq));
  int i;

  i = 0;
  ptr = g_sequence_get_begin_iter (seq);
  while (!g_sequence_iter_is_end (ptr))
    {
      int old_pos = GPOINTER_TO_INT (g_hash_table_lookup (old_positions, ptr));
      order[i++] = old_pos;
      ptr = g_sequence_iter_next (ptr);
    }

  g_hash_table_destroy (old_positions);

  return order;
}

/**
 * btk_list_store_swap:
 * @store: A #BtkListStore.
 * @a: A #BtkTreeIter.
 * @b: Another #BtkTreeIter.
 *
 * Swaps @a and @b in @store. Note that this function only works with
 * unsorted stores.
 *
 * Since: 2.2
 **/
void
btk_list_store_swap (BtkListStore *store,
		     BtkTreeIter  *a,
		     BtkTreeIter  *b)
{
  GHashTable *old_positions;
  gint *order;
  BtkTreePath *path;

  g_return_if_fail (BTK_IS_LIST_STORE (store));
  g_return_if_fail (!BTK_LIST_STORE_IS_SORTED (store));
  g_return_if_fail (VALID_ITER (a, store));
  g_return_if_fail (VALID_ITER (b, store));

  if (a->user_data == b->user_data)
    return;

  old_positions = save_positions (store->seq);
  
  g_sequence_swap (a->user_data, b->user_data);

  order = generate_order (store->seq, old_positions);
  path = btk_tree_path_new ();
  
  btk_tree_model_rows_reordered (BTK_TREE_MODEL (store),
				 path, NULL, order);

  btk_tree_path_free (path);
  g_free (order);
}

static void
btk_list_store_move_to (BtkListStore *store,
			BtkTreeIter  *iter,
			gint	      new_pos)
{
  GHashTable *old_positions;
  BtkTreePath *path;
  gint *order;
  
  old_positions = save_positions (store->seq);
  
  g_sequence_move (iter->user_data, g_sequence_get_iter_at_pos (store->seq, new_pos));

  order = generate_order (store->seq, old_positions);
  
  path = btk_tree_path_new ();
  btk_tree_model_rows_reordered (BTK_TREE_MODEL (store),
				 path, NULL, order);
  btk_tree_path_free (path);
  g_free (order);
}

/**
 * btk_list_store_move_before:
 * @store: A #BtkListStore.
 * @iter: A #BtkTreeIter.
 * @position: (allow-none): A #BtkTreeIter, or %NULL.
 *
 * Moves @iter in @store to the position before @position. Note that this
 * function only works with unsorted stores. If @position is %NULL, @iter
 * will be moved to the end of the list.
 *
 * Since: 2.2
 **/
void
btk_list_store_move_before (BtkListStore *store,
                            BtkTreeIter  *iter,
			    BtkTreeIter  *position)
{
  gint pos;
  
  g_return_if_fail (BTK_IS_LIST_STORE (store));
  g_return_if_fail (!BTK_LIST_STORE_IS_SORTED (store));
  g_return_if_fail (VALID_ITER (iter, store));
  if (position)
    g_return_if_fail (VALID_ITER (position, store));

  if (position)
    pos = g_sequence_iter_get_position (position->user_data);
  else
    pos = -1;
  
  btk_list_store_move_to (store, iter, pos);
}

/**
 * btk_list_store_move_after:
 * @store: A #BtkListStore.
 * @iter: A #BtkTreeIter.
 * @position: (allow-none): A #BtkTreeIter or %NULL.
 *
 * Moves @iter in @store to the position after @position. Note that this
 * function only works with unsorted stores. If @position is %NULL, @iter
 * will be moved to the start of the list.
 *
 * Since: 2.2
 **/
void
btk_list_store_move_after (BtkListStore *store,
			   BtkTreeIter  *iter,
			   BtkTreeIter  *position)
{
  gint pos;
  
  g_return_if_fail (BTK_IS_LIST_STORE (store));
  g_return_if_fail (!BTK_LIST_STORE_IS_SORTED (store));
  g_return_if_fail (VALID_ITER (iter, store));
  if (position)
    g_return_if_fail (VALID_ITER (position, store));

  if (position)
    pos = g_sequence_iter_get_position (position->user_data) + 1;
  else
    pos = 0;
  
  btk_list_store_move_to (store, iter, pos);
}
    
/* Sorting */
static gint
btk_list_store_compare_func (GSequenceIter *a,
			     GSequenceIter *b,
			     gpointer      user_data)
{
  BtkListStore *list_store = user_data;
  BtkTreeIter iter_a;
  BtkTreeIter iter_b;
  gint retval;
  BtkTreeIterCompareFunc func;
  gpointer data;

  if (list_store->sort_column_id != -1)
    {
      BtkTreeDataSortHeader *header;

      header = _btk_tree_data_list_get_header (list_store->sort_list,
					       list_store->sort_column_id);
      g_return_val_if_fail (header != NULL, 0);
      g_return_val_if_fail (header->func != NULL, 0);

      func = header->func;
      data = header->data;
    }
  else
    {
      g_return_val_if_fail (list_store->default_sort_func != NULL, 0);
      func = list_store->default_sort_func;
      data = list_store->default_sort_data;
    }

  iter_a.stamp = list_store->stamp;
  iter_a.user_data = (gpointer)a;
  iter_b.stamp = list_store->stamp;
  iter_b.user_data = (gpointer)b;

  g_assert (VALID_ITER (&iter_a, list_store));
  g_assert (VALID_ITER (&iter_b, list_store));
  
  retval = (* func) (BTK_TREE_MODEL (list_store), &iter_a, &iter_b, data);

  if (list_store->order == BTK_SORT_DESCENDING)
    {
      if (retval > 0)
	retval = -1;
      else if (retval < 0)
	retval = 1;
    }

  return retval;
}

static void
btk_list_store_sort (BtkListStore *list_store)
{
  gint *new_order;
  BtkTreePath *path;
  GHashTable *old_positions;

  if (!BTK_LIST_STORE_IS_SORTED (list_store) ||
      g_sequence_get_length (list_store->seq) <= 1)
    return;

  old_positions = save_positions (list_store->seq);

  g_sequence_sort_iter (list_store->seq, btk_list_store_compare_func, list_store);

  /* Let the world know about our new order */
  new_order = generate_order (list_store->seq, old_positions);

  path = btk_tree_path_new ();
  btk_tree_model_rows_reordered (BTK_TREE_MODEL (list_store),
				 path, NULL, new_order);
  btk_tree_path_free (path);
  g_free (new_order);
}

static gboolean
iter_is_sorted (BtkListStore *list_store,
                BtkTreeIter  *iter)
{
  GSequenceIter *cmp;

  if (!g_sequence_iter_is_begin (iter->user_data))
    {
      cmp = g_sequence_iter_prev (iter->user_data);
      if (btk_list_store_compare_func (cmp, iter->user_data, list_store) > 0)
	return FALSE;
    }

  cmp = g_sequence_iter_next (iter->user_data);
  if (!g_sequence_iter_is_end (cmp))
    {
      if (btk_list_store_compare_func (iter->user_data, cmp, list_store) > 0)
	return FALSE;
    }
  
  return TRUE;
}

static void
btk_list_store_sort_iter_changed (BtkListStore *list_store,
				  BtkTreeIter  *iter,
				  gint          column)

{
  BtkTreePath *path;

  path = btk_list_store_get_path (BTK_TREE_MODEL (list_store), iter);
  btk_tree_model_row_changed (BTK_TREE_MODEL (list_store), path, iter);
  btk_tree_path_free (path);

  if (!iter_is_sorted (list_store, iter))
    {
      GHashTable *old_positions;
      gint *order;

      old_positions = save_positions (list_store->seq);
      g_sequence_sort_changed_iter (iter->user_data,
				    btk_list_store_compare_func,
				    list_store);
      order = generate_order (list_store->seq, old_positions);
      path = btk_tree_path_new ();
      btk_tree_model_rows_reordered (BTK_TREE_MODEL (list_store),
                                     path, NULL, order);
      btk_tree_path_free (path);
      g_free (order);
    }
}

static gboolean
btk_list_store_get_sort_column_id (BtkTreeSortable  *sortable,
				   gint             *sort_column_id,
				   BtkSortType      *order)
{
  BtkListStore *list_store = (BtkListStore *) sortable;

  if (sort_column_id)
    * sort_column_id = list_store->sort_column_id;
  if (order)
    * order = list_store->order;

  if (list_store->sort_column_id == BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID ||
      list_store->sort_column_id == BTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID)
    return FALSE;

  return TRUE;
}

static void
btk_list_store_set_sort_column_id (BtkTreeSortable  *sortable,
				   gint              sort_column_id,
				   BtkSortType       order)
{
  BtkListStore *list_store = (BtkListStore *) sortable;

  if ((list_store->sort_column_id == sort_column_id) &&
      (list_store->order == order))
    return;

  if (sort_column_id != BTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID)
    {
      if (sort_column_id != BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID)
	{
	  BtkTreeDataSortHeader *header = NULL;

	  header = _btk_tree_data_list_get_header (list_store->sort_list, 
						   sort_column_id);

	  /* We want to make sure that we have a function */
	  g_return_if_fail (header != NULL);
	  g_return_if_fail (header->func != NULL);
	}
      else
	{
	  g_return_if_fail (list_store->default_sort_func != NULL);
	}
    }


  list_store->sort_column_id = sort_column_id;
  list_store->order = order;

  btk_tree_sortable_sort_column_changed (sortable);

  btk_list_store_sort (list_store);
}

static void
btk_list_store_set_sort_func (BtkTreeSortable        *sortable,
			      gint                    sort_column_id,
			      BtkTreeIterCompareFunc  func,
			      gpointer                data,
			      GDestroyNotify          destroy)
{
  BtkListStore *list_store = (BtkListStore *) sortable;

  list_store->sort_list = _btk_tree_data_list_set_header (list_store->sort_list, 
							  sort_column_id, 
							  func, data, destroy);

  if (list_store->sort_column_id == sort_column_id)
    btk_list_store_sort (list_store);
}

static void
btk_list_store_set_default_sort_func (BtkTreeSortable        *sortable,
				      BtkTreeIterCompareFunc  func,
				      gpointer                data,
				      GDestroyNotify          destroy)
{
  BtkListStore *list_store = (BtkListStore *) sortable;

  if (list_store->default_sort_destroy)
    {
      GDestroyNotify d = list_store->default_sort_destroy;

      list_store->default_sort_destroy = NULL;
      d (list_store->default_sort_data);
    }

  list_store->default_sort_func = func;
  list_store->default_sort_data = data;
  list_store->default_sort_destroy = destroy;

  if (list_store->sort_column_id == BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID)
    btk_list_store_sort (list_store);
}

static gboolean
btk_list_store_has_default_sort_func (BtkTreeSortable *sortable)
{
  BtkListStore *list_store = (BtkListStore *) sortable;

  return (list_store->default_sort_func != NULL);
}


/**
 * btk_list_store_insert_with_values:
 * @list_store: A #BtkListStore
 * @iter: (out) (allow-none): An unset #BtkTreeIter to set to the new row, or %NULL.
 * @position: position to insert the new row
 * @Varargs: pairs of column number and value, terminated with -1
 *
 * Creates a new row at @position.  @iter will be changed to point to this new
 * row.  If @position is larger than the number of rows on the list, then the
 * new row will be appended to the list. The row will be filled with the 
 * values given to this function. 
 * 
 * Calling
 * <literal>btk_list_store_insert_with_values(list_store, iter, position...)</literal> 
 * has the same effect as calling 
 * |[
 * btk_list_store_insert (list_store, iter, position);
 * btk_list_store_set (list_store, iter, ...);
 * ]|
 * with the difference that the former will only emit a row_inserted signal,
 * while the latter will emit row_inserted, row_changed and, if the list store
 * is sorted, rows_reordered. Since emitting the rows_reordered signal
 * repeatedly can affect the performance of the program, 
 * btk_list_store_insert_with_values() should generally be preferred when
 * inserting rows in a sorted list store.
 *
 * Since: 2.6
 */
void
btk_list_store_insert_with_values (BtkListStore *list_store,
				   BtkTreeIter  *iter,
				   gint          position,
				   ...)
{
  BtkTreePath *path;
  GSequence *seq;
  GSequenceIter *ptr;
  BtkTreeIter tmp_iter;
  gint length;
  gboolean changed = FALSE;
  gboolean maybe_need_sort = FALSE;
  va_list var_args;

  /* FIXME: refactor to reduce overlap with btk_list_store_set() */
  g_return_if_fail (BTK_IS_LIST_STORE (list_store));

  if (!iter)
    iter = &tmp_iter;

  list_store->columns_dirty = TRUE;

  seq = list_store->seq;

  length = g_sequence_get_length (seq);
  if (position > length)
    position = length;

  ptr = g_sequence_get_iter_at_pos (seq, position);
  ptr = g_sequence_insert_before (ptr, NULL);

  iter->stamp = list_store->stamp;
  iter->user_data = ptr;

  g_assert (VALID_ITER (iter, list_store));

  list_store->length++;  

  va_start (var_args, position);
  btk_list_store_set_valist_internal (list_store, iter, 
				      &changed, &maybe_need_sort,
				      var_args);
  va_end (var_args);

  /* Don't emit rows_reordered here */
  if (maybe_need_sort && BTK_LIST_STORE_IS_SORTED (list_store))
    g_sequence_sort_changed_iter (iter->user_data,
				  btk_list_store_compare_func,
				  list_store);

  /* Just emit row_inserted */
  path = btk_list_store_get_path (BTK_TREE_MODEL (list_store), iter);
  btk_tree_model_row_inserted (BTK_TREE_MODEL (list_store), path, iter);
  btk_tree_path_free (path);
}


/**
 * btk_list_store_insert_with_valuesv:
 * @list_store: A #BtkListStore
 * @iter: (out) (allow-none): An unset #BtkTreeIter to set to the new row, or %NULL.
 * @position: position to insert the new row
 * @columns: (array length=n_values): an array of column numbers
 * @values: (array length=n_values): an array of BValues 
 * @n_values: the length of the @columns and @values arrays
 * 
 * A variant of btk_list_store_insert_with_values() which
 * takes the columns and values as two arrays, instead of
 * varargs. This function is mainly intended for 
 * language-bindings.
 *
 * Since: 2.6
 */
void
btk_list_store_insert_with_valuesv (BtkListStore *list_store,
				    BtkTreeIter  *iter,
				    gint          position,
				    gint         *columns, 
				    BValue       *values,
				    gint          n_values)
{
  BtkTreePath *path;
  GSequence *seq;
  GSequenceIter *ptr;
  BtkTreeIter tmp_iter;
  gint length;
  gboolean changed = FALSE;
  gboolean maybe_need_sort = FALSE;

  /* FIXME refactor to reduce overlap with 
   * btk_list_store_insert_with_values() 
   */
  g_return_if_fail (BTK_IS_LIST_STORE (list_store));

  if (!iter)
    iter = &tmp_iter;

  list_store->columns_dirty = TRUE;

  seq = list_store->seq;

  length = g_sequence_get_length (seq);
  if (position > length)
    position = length;

  ptr = g_sequence_get_iter_at_pos (seq, position);
  ptr = g_sequence_insert_before (ptr, NULL);

  iter->stamp = list_store->stamp;
  iter->user_data = ptr;

  g_assert (VALID_ITER (iter, list_store));

  list_store->length++;  

  btk_list_store_set_vector_internal (list_store, iter,
				      &changed, &maybe_need_sort,
				      columns, values, n_values);

  /* Don't emit rows_reordered here */
  if (maybe_need_sort && BTK_LIST_STORE_IS_SORTED (list_store))
    g_sequence_sort_changed_iter (iter->user_data,
				  btk_list_store_compare_func,
				  list_store);

  /* Just emit row_inserted */
  path = btk_list_store_get_path (BTK_TREE_MODEL (list_store), iter);
  btk_tree_model_row_inserted (BTK_TREE_MODEL (list_store), path, iter);
  btk_tree_path_free (path);
}

/* BtkBuildable custom tag implementation
 *
 * <columns>
 *   <column type="..."/>
 *   <column type="..."/>
 * </columns>
 */
typedef struct {
  gboolean translatable;
  gchar *context;
  int id;
} ColInfo;

typedef struct {
  BtkBuilder *builder;
  BObject *object;
  GSList *column_type_names;
  GType *column_types;
  BValue *values;
  gint *colids;
  ColInfo **columns;
  gint last_row;
  gint n_columns;
  gint row_column;
  GQuark error_quark;
  gboolean is_data;
  const gchar *domain;
} SubParserData;

static void
list_store_start_element (GMarkupParseContext *context,
			  const gchar         *element_name,
			  const gchar        **names,
			  const gchar        **values,
			  gpointer             user_data,
			  GError             **error)
{
  guint i;
  SubParserData *data = (SubParserData*)user_data;

  if (strcmp (element_name, "col") == 0)
    {
      int i, id = -1;
      gchar *context = NULL;
      gboolean translatable = FALSE;
      ColInfo *info;

      if (data->row_column >= data->n_columns)
        {
	  g_set_error (error, data->error_quark, 0,
	  	       "Too many columns, maximum is %d\n", data->n_columns - 1);
          return;
        }

      for (i = 0; names[i]; i++)
	if (strcmp (names[i], "id") == 0)
	  {
	    errno = 0;
	    id = atoi (values[i]);
	    if (errno)
              {
	        g_set_error (error, data->error_quark, 0,
		  	     "the id tag %s could not be converted to an integer",
			     values[i]);
                return;
              }
	    if (id < 0 || id >= data->n_columns)
              {
                g_set_error (error, data->error_quark, 0,
                             "id value %d out of range", id);
                return;
              }
	  }
	else if (strcmp (names[i], "translatable") == 0)
	  {
	    if (!_btk_builder_boolean_from_string (values[i], &translatable,
						   error))
	      return;
	  }
	else if (strcmp (names[i], "comments") == 0)
	  {
	    /* do nothing, comments are for translators */
	  }
	else if (strcmp (names[i], "context") == 0) 
	  {
	    context = g_strdup (values[i]);
	  }

      if (id == -1)
        {
	  g_set_error (error, data->error_quark, 0,
	  	       "<col> needs an id attribute");
          return;
        }
      
      info = g_slice_new0 (ColInfo);
      info->translatable = translatable;
      info->context = context;
      info->id = id;

      data->colids[data->row_column] = id;
      data->columns[data->row_column] = info;
      data->row_column++;
      data->is_data = TRUE;
    }
  else if (strcmp (element_name, "row") == 0)
    ;
  else if (strcmp (element_name, "column") == 0)
    for (i = 0; names[i]; i++)
      if (strcmp (names[i], "type") == 0)
	data->column_type_names = b_slist_prepend (data->column_type_names,
						   g_strdup (values[i]));
  else if (strcmp (element_name, "columns") == 0)
    ;
  else if (strcmp (element_name, "data") == 0)
    ;
  else
    g_set_error (error, data->error_quark, 0,
		 "Unknown start tag: %s", element_name);
}

static void
list_store_end_element (GMarkupParseContext *context,
			const gchar         *element_name,
			gpointer             user_data,
			GError             **error)
{
  SubParserData *data = (SubParserData*)user_data;

  g_assert (data->builder);
  
  if (strcmp (element_name, "row") == 0)
    {
      BtkTreeIter iter;
      int i;

      btk_list_store_insert_with_valuesv (BTK_LIST_STORE (data->object),
					  &iter,
					  data->last_row,
					  data->colids,
					  data->values,
					  data->row_column);
      for (i = 0; i < data->row_column; i++)
	{
	  ColInfo *info = data->columns[i];
	  g_free (info->context);
	  g_slice_free (ColInfo, info);
	  data->columns[i] = NULL;
	  b_value_unset (&data->values[i]);
	}
      g_free (data->values);
      data->values = g_new0 (BValue, data->n_columns);
      data->last_row++;
      data->row_column = 0;
    }
  else if (strcmp (element_name, "columns") == 0)
    {
      GType *column_types;
      GSList *l;
      int i;
      GType type;

      data->column_type_names = b_slist_reverse (data->column_type_names);
      column_types = g_new0 (GType, b_slist_length (data->column_type_names));

      for (l = data->column_type_names, i = 0; l; l = l->next, i++)
	{
	  type = btk_builder_get_type_from_name (data->builder, l->data);
	  if (type == B_TYPE_INVALID)
	    {
	      g_warning ("Unknown type %s specified in treemodel %s",
			 (const gchar*)l->data,
			 btk_buildable_get_name (BTK_BUILDABLE (data->object)));
	      continue;
	    }
	  column_types[i] = type;

	  g_free (l->data);
	}

      btk_list_store_set_column_types (BTK_LIST_STORE (data->object), i,
				       column_types);

      g_free (column_types);
    }
  else if (strcmp (element_name, "col") == 0)
    data->is_data = FALSE;
  else if (strcmp (element_name, "data") == 0)
    ;
  else if (strcmp (element_name, "column") == 0)
    ;
  else
    g_set_error (error, data->error_quark, 0,
		 "Unknown end tag: %s", element_name);
}

static void
list_store_text (GMarkupParseContext *context,
		 const gchar         *text,
		 gsize                text_len,
		 gpointer             user_data,
		 GError             **error)
{
  SubParserData *data = (SubParserData*)user_data;
  gint i;
  GError *tmp_error = NULL;
  gchar *string;
  ColInfo *info;
  
  if (!data->is_data)
    return;

  i = data->row_column - 1;
  info = data->columns[i];

  string = g_strndup (text, text_len);
  if (info->translatable && text_len)
    {
      gchar *translated;

      /* FIXME: This will not use the domain set in the .ui file,
       * since the parser is not telling the builder about the domain.
       * However, it will work for btk_builder_set_translation_domain() calls.
       */
      translated = _btk_builder_parser_translate (data->domain,
						  info->context,
						  string);
      g_free (string);
      string = translated;
    }

  if (!btk_builder_value_from_string_type (data->builder,
					   data->column_types[info->id],
					   string,
					   &data->values[i],
					   &tmp_error))
    {
      g_set_error (error,
		   tmp_error->domain,
		   tmp_error->code,
		   "Could not convert '%s' to type %s: %s\n",
		   text, g_type_name (data->column_types[info->id]),
		   tmp_error->message);
      g_error_free (tmp_error);
    }
  g_free (string);
}

static const GMarkupParser list_store_parser =
  {
    list_store_start_element,
    list_store_end_element,
    list_store_text
  };

static gboolean
btk_list_store_buildable_custom_tag_start (BtkBuildable  *buildable,
					   BtkBuilder    *builder,
					   BObject       *child,
					   const gchar   *tagname,
					   GMarkupParser *parser,
					   gpointer      *data)
{
  SubParserData *parser_data;

  if (child)
    return FALSE;

  if (strcmp (tagname, "columns") == 0)
    {

      parser_data = g_slice_new0 (SubParserData);
      parser_data->builder = builder;
      parser_data->object = B_OBJECT (buildable);
      parser_data->column_type_names = NULL;

      *parser = list_store_parser;
      *data = parser_data;
      return TRUE;
    }
  else if (strcmp (tagname, "data") == 0)
    {
      gint n_columns = btk_list_store_get_n_columns (BTK_TREE_MODEL (buildable));
      if (n_columns == 0)
	g_error ("Cannot append data to an empty model");

      parser_data = g_slice_new0 (SubParserData);
      parser_data->builder = builder;
      parser_data->object = B_OBJECT (buildable);
      parser_data->values = g_new0 (BValue, n_columns);
      parser_data->colids = g_new0 (gint, n_columns);
      parser_data->columns = g_new0 (ColInfo*, n_columns);
      parser_data->column_types = BTK_LIST_STORE (buildable)->column_headers;
      parser_data->n_columns = n_columns;
      parser_data->last_row = 0;
      parser_data->error_quark = g_quark_from_static_string ("BtkListStore");
      parser_data->domain = btk_builder_get_translation_domain (builder);
      
      *parser = list_store_parser;
      *data = parser_data;
      return TRUE;
    }
  else
    g_warning ("Unknown custom list store tag: %s", tagname);
  
  return FALSE;
}

static void
btk_list_store_buildable_custom_tag_end (BtkBuildable *buildable,
					 BtkBuilder   *builder,
					 BObject      *child,
					 const gchar  *tagname,
					 gpointer     *data)
{
  SubParserData *sub = (SubParserData*)data;
  
  if (strcmp (tagname, "columns") == 0)
    {
      b_slist_free (sub->column_type_names);
      g_slice_free (SubParserData, sub);
    }
  else if (strcmp (tagname, "data") == 0)
    {
      int i;
      for (i = 0; i < sub->n_columns; i++)
	{
	  ColInfo *info = sub->columns[i];
	  if (info)
	    {
	      g_free (info->context);
	      g_slice_free (ColInfo, info);
	    }
	}
      g_free (sub->colids);
      g_free (sub->columns);
      g_free (sub->values);
      g_slice_free (SubParserData, sub);
    }
  else
    g_warning ("Unknown custom list store tag: %s", tagname);
}

#define __BTK_LIST_STORE_C__
#include "btkaliasdef.c"
