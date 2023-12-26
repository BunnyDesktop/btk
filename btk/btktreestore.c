/* btktreestore.c
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
#include <string.h>
#include <bobject/gvaluecollector.h>
#include "btktreemodel.h"
#include "btktreestore.h"
#include "btktreedatalist.h"
#include "btktreednd.h"
#include "btkbuildable.h"
#include "btkintl.h"
#include "btkalias.h"

#define G_NODE(node) ((GNode *)node)
#define BTK_TREE_STORE_IS_SORTED(tree) (((BtkTreeStore*)(tree))->sort_column_id != BTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID)
#define VALID_ITER(iter, tree_store) ((iter)!= NULL && (iter)->user_data != NULL && ((BtkTreeStore*)(tree_store))->stamp == (iter)->stamp)

static void         btk_tree_store_tree_model_init (BtkTreeModelIface *iface);
static void         btk_tree_store_drag_source_init(BtkTreeDragSourceIface *iface);
static void         btk_tree_store_drag_dest_init  (BtkTreeDragDestIface   *iface);
static void         btk_tree_store_sortable_init   (BtkTreeSortableIface   *iface);
static void         btk_tree_store_buildable_init  (BtkBuildableIface      *iface);
static void         btk_tree_store_finalize        (GObject           *object);
static BtkTreeModelFlags btk_tree_store_get_flags  (BtkTreeModel      *tree_model);
static gint         btk_tree_store_get_n_columns   (BtkTreeModel      *tree_model);
static GType        btk_tree_store_get_column_type (BtkTreeModel      *tree_model,
						    gint               index);
static gboolean     btk_tree_store_get_iter        (BtkTreeModel      *tree_model,
						    BtkTreeIter       *iter,
						    BtkTreePath       *path);
static BtkTreePath *btk_tree_store_get_path        (BtkTreeModel      *tree_model,
						    BtkTreeIter       *iter);
static void         btk_tree_store_get_value       (BtkTreeModel      *tree_model,
						    BtkTreeIter       *iter,
						    gint               column,
						    GValue            *value);
static gboolean     btk_tree_store_iter_next       (BtkTreeModel      *tree_model,
						    BtkTreeIter       *iter);
static gboolean     btk_tree_store_iter_children   (BtkTreeModel      *tree_model,
						    BtkTreeIter       *iter,
						    BtkTreeIter       *parent);
static gboolean     btk_tree_store_iter_has_child  (BtkTreeModel      *tree_model,
						    BtkTreeIter       *iter);
static gint         btk_tree_store_iter_n_children (BtkTreeModel      *tree_model,
						    BtkTreeIter       *iter);
static gboolean     btk_tree_store_iter_nth_child  (BtkTreeModel      *tree_model,
						    BtkTreeIter       *iter,
						    BtkTreeIter       *parent,
						    gint               n);
static gboolean     btk_tree_store_iter_parent     (BtkTreeModel      *tree_model,
						    BtkTreeIter       *iter,
						    BtkTreeIter       *child);


static void btk_tree_store_set_n_columns   (BtkTreeStore *tree_store,
					    gint          n_columns);
static void btk_tree_store_set_column_type (BtkTreeStore *tree_store,
					    gint          column,
					    GType         type);

static void btk_tree_store_increment_stamp (BtkTreeStore  *tree_store);


/* DND interfaces */
static gboolean real_btk_tree_store_row_draggable   (BtkTreeDragSource *drag_source,
						   BtkTreePath       *path);
static gboolean btk_tree_store_drag_data_delete   (BtkTreeDragSource *drag_source,
						   BtkTreePath       *path);
static gboolean btk_tree_store_drag_data_get      (BtkTreeDragSource *drag_source,
						   BtkTreePath       *path,
						   BtkSelectionData  *selection_data);
static gboolean btk_tree_store_drag_data_received (BtkTreeDragDest   *drag_dest,
						   BtkTreePath       *dest,
						   BtkSelectionData  *selection_data);
static gboolean btk_tree_store_row_drop_possible  (BtkTreeDragDest   *drag_dest,
						   BtkTreePath       *dest_path,
						   BtkSelectionData  *selection_data);

/* Sortable Interfaces */

static void     btk_tree_store_sort                    (BtkTreeStore           *tree_store);
static void     btk_tree_store_sort_iter_changed       (BtkTreeStore           *tree_store,
							BtkTreeIter            *iter,
							gint                    column,
							gboolean                emit_signal);
static gboolean btk_tree_store_get_sort_column_id      (BtkTreeSortable        *sortable,
							gint                   *sort_column_id,
							BtkSortType            *order);
static void     btk_tree_store_set_sort_column_id      (BtkTreeSortable        *sortable,
							gint                    sort_column_id,
							BtkSortType             order);
static void     btk_tree_store_set_sort_func           (BtkTreeSortable        *sortable,
							gint                    sort_column_id,
							BtkTreeIterCompareFunc  func,
							gpointer                data,
							GDestroyNotify          destroy);
static void     btk_tree_store_set_default_sort_func   (BtkTreeSortable        *sortable,
							BtkTreeIterCompareFunc  func,
							gpointer                data,
							GDestroyNotify          destroy);
static gboolean btk_tree_store_has_default_sort_func   (BtkTreeSortable        *sortable);


/* buildable */

static gboolean btk_tree_store_buildable_custom_tag_start (BtkBuildable  *buildable,
							   BtkBuilder    *builder,
							   GObject       *child,
							   const gchar   *tagname,
							   GMarkupParser *parser,
							   gpointer      *data);
static void     btk_tree_store_buildable_custom_finished (BtkBuildable 	 *buildable,
							  BtkBuilder   	 *builder,
							  GObject      	 *child,
							  const gchar  	 *tagname,
							  gpointer     	  user_data);

static void     validate_gnode                         (GNode *node);

static void     btk_tree_store_move                    (BtkTreeStore           *tree_store,
                                                        BtkTreeIter            *iter,
                                                        BtkTreeIter            *position,
                                                        gboolean                before);


static inline void
validate_tree (BtkTreeStore *tree_store)
{
  if (btk_debug_flags & BTK_DEBUG_TREE)
    {
      g_assert (G_NODE (tree_store->root)->parent == NULL);

      validate_gnode (G_NODE (tree_store->root));
    }
}

G_DEFINE_TYPE_WITH_CODE (BtkTreeStore, btk_tree_store, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_TREE_MODEL,
						btk_tree_store_tree_model_init)
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_TREE_DRAG_SOURCE,
						btk_tree_store_drag_source_init)
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_TREE_DRAG_DEST,
						btk_tree_store_drag_dest_init)
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_TREE_SORTABLE,
						btk_tree_store_sortable_init)
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
						btk_tree_store_buildable_init))

static void
btk_tree_store_class_init (BtkTreeStoreClass *class)
{
  GObjectClass *object_class;

  object_class = (GObjectClass *) class;

  object_class->finalize = btk_tree_store_finalize;
}

static void
btk_tree_store_tree_model_init (BtkTreeModelIface *iface)
{
  iface->get_flags = btk_tree_store_get_flags;
  iface->get_n_columns = btk_tree_store_get_n_columns;
  iface->get_column_type = btk_tree_store_get_column_type;
  iface->get_iter = btk_tree_store_get_iter;
  iface->get_path = btk_tree_store_get_path;
  iface->get_value = btk_tree_store_get_value;
  iface->iter_next = btk_tree_store_iter_next;
  iface->iter_children = btk_tree_store_iter_children;
  iface->iter_has_child = btk_tree_store_iter_has_child;
  iface->iter_n_children = btk_tree_store_iter_n_children;
  iface->iter_nth_child = btk_tree_store_iter_nth_child;
  iface->iter_parent = btk_tree_store_iter_parent;
}

static void
btk_tree_store_drag_source_init (BtkTreeDragSourceIface *iface)
{
  iface->row_draggable = real_btk_tree_store_row_draggable;
  iface->drag_data_delete = btk_tree_store_drag_data_delete;
  iface->drag_data_get = btk_tree_store_drag_data_get;
}

static void
btk_tree_store_drag_dest_init (BtkTreeDragDestIface *iface)
{
  iface->drag_data_received = btk_tree_store_drag_data_received;
  iface->row_drop_possible = btk_tree_store_row_drop_possible;
}

static void
btk_tree_store_sortable_init (BtkTreeSortableIface *iface)
{
  iface->get_sort_column_id = btk_tree_store_get_sort_column_id;
  iface->set_sort_column_id = btk_tree_store_set_sort_column_id;
  iface->set_sort_func = btk_tree_store_set_sort_func;
  iface->set_default_sort_func = btk_tree_store_set_default_sort_func;
  iface->has_default_sort_func = btk_tree_store_has_default_sort_func;
}

void
btk_tree_store_buildable_init (BtkBuildableIface *iface)
{
  iface->custom_tag_start = btk_tree_store_buildable_custom_tag_start;
  iface->custom_finished = btk_tree_store_buildable_custom_finished;
}

static void
btk_tree_store_init (BtkTreeStore *tree_store)
{
  tree_store->root = g_node_new (NULL);
  /* While the odds are against us getting 0...
   */
  do
    {
      tree_store->stamp = g_random_int ();
    }
  while (tree_store->stamp == 0);

  tree_store->sort_list = NULL;
  tree_store->sort_column_id = BTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID;
  tree_store->columns_dirty = FALSE;
}

/**
 * btk_tree_store_new:
 * @n_columns: number of columns in the tree store
 * @Varargs: all #GType types for the columns, from first to last
 *
 * Creates a new tree store as with @n_columns columns each of the types passed
 * in.  Note that only types derived from standard GObject fundamental types 
 * are supported. 
 *
 * As an example, <literal>btk_tree_store_new (3, G_TYPE_INT, G_TYPE_STRING,
 * BDK_TYPE_PIXBUF);</literal> will create a new #BtkTreeStore with three columns, of type
 * <type>int</type>, <type>string</type> and #BdkPixbuf respectively.
 *
 * Return value: a new #BtkTreeStore
 **/
BtkTreeStore *
btk_tree_store_new (gint n_columns,
			       ...)
{
  BtkTreeStore *retval;
  va_list args;
  gint i;

  g_return_val_if_fail (n_columns > 0, NULL);

  retval = g_object_new (BTK_TYPE_TREE_STORE, NULL);
  btk_tree_store_set_n_columns (retval, n_columns);

  va_start (args, n_columns);

  for (i = 0; i < n_columns; i++)
    {
      GType type = va_arg (args, GType);
      if (! _btk_tree_data_list_check_type (type))
	{
	  g_warning ("%s: Invalid type %s\n", B_STRLOC, g_type_name (type));
	  g_object_unref (retval);
          va_end (args);
	  return NULL;
	}
      btk_tree_store_set_column_type (retval, i, type);
    }
  va_end (args);

  return retval;
}
/**
 * btk_tree_store_newv:
 * @n_columns: number of columns in the tree store
 * @types: (array length=n_columns): an array of #GType types for the columns, from first to last
 *
 * Non vararg creation function.  Used primarily by language bindings.
 *
 * Return value: (transfer full): a new #BtkTreeStore
 **/
BtkTreeStore *
btk_tree_store_newv (gint   n_columns,
		     GType *types)
{
  BtkTreeStore *retval;
  gint i;

  g_return_val_if_fail (n_columns > 0, NULL);

  retval = g_object_new (BTK_TYPE_TREE_STORE, NULL);
  btk_tree_store_set_n_columns (retval, n_columns);

   for (i = 0; i < n_columns; i++)
    {
      if (! _btk_tree_data_list_check_type (types[i]))
	{
	  g_warning ("%s: Invalid type %s\n", B_STRLOC, g_type_name (types[i]));
	  g_object_unref (retval);
	  return NULL;
	}
      btk_tree_store_set_column_type (retval, i, types[i]);
    }

  return retval;
}


/**
 * btk_tree_store_set_column_types:
 * @tree_store: A #BtkTreeStore
 * @n_columns: Number of columns for the tree store
 * @types: (array length=n_columns): An array of #GType types, one for each column
 * 
 * This function is meant primarily for #GObjects that inherit from 
 * #BtkTreeStore, and should only be used when constructing a new 
 * #BtkTreeStore.  It will not function after a row has been added, 
 * or a method on the #BtkTreeModel interface is called.
 **/
void
btk_tree_store_set_column_types (BtkTreeStore *tree_store,
				 gint          n_columns,
				 GType        *types)
{
  gint i;

  g_return_if_fail (BTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (tree_store->columns_dirty == 0);

  btk_tree_store_set_n_columns (tree_store, n_columns);
   for (i = 0; i < n_columns; i++)
    {
      if (! _btk_tree_data_list_check_type (types[i]))
	{
	  g_warning ("%s: Invalid type %s\n", B_STRLOC, g_type_name (types[i]));
	  continue;
	}
      btk_tree_store_set_column_type (tree_store, i, types[i]);
    }
}

static void
btk_tree_store_set_n_columns (BtkTreeStore *tree_store,
			      gint          n_columns)
{
  int i;

  if (tree_store->n_columns == n_columns)
    return;

  tree_store->column_headers = g_renew (GType, tree_store->column_headers, n_columns);
  for (i = tree_store->n_columns; i < n_columns; i++)
    tree_store->column_headers[i] = G_TYPE_INVALID;
  tree_store->n_columns = n_columns;

  if (tree_store->sort_list)
    _btk_tree_data_list_header_free (tree_store->sort_list);

  tree_store->sort_list = _btk_tree_data_list_header_new (n_columns, tree_store->column_headers);
}

/**
 * btk_tree_store_set_column_type:
 * @tree_store: a #BtkTreeStore
 * @column: column number
 * @type: type of the data to be stored in @column
 *
 * Supported types include: %G_TYPE_UINT, %G_TYPE_INT, %G_TYPE_UCHAR,
 * %G_TYPE_CHAR, %G_TYPE_BOOLEAN, %G_TYPE_POINTER, %G_TYPE_FLOAT,
 * %G_TYPE_DOUBLE, %G_TYPE_STRING, %G_TYPE_OBJECT, and %G_TYPE_BOXED, along with
 * subclasses of those types such as %BDK_TYPE_PIXBUF.
 *
 **/
static void
btk_tree_store_set_column_type (BtkTreeStore *tree_store,
				gint          column,
				GType         type)
{
  if (!_btk_tree_data_list_check_type (type))
    {
      g_warning ("%s: Invalid type %s\n", B_STRLOC, g_type_name (type));
      return;
    }
  tree_store->column_headers[column] = type;
}

static gboolean
node_free (GNode *node, gpointer data)
{
  if (node->data)
    _btk_tree_data_list_free (node->data, (GType*)data);
  node->data = NULL;

  return FALSE;
}

static void
btk_tree_store_finalize (GObject *object)
{
  BtkTreeStore *tree_store = BTK_TREE_STORE (object);

  g_node_traverse (tree_store->root, G_POST_ORDER, G_TRAVERSE_ALL, -1,
		   node_free, tree_store->column_headers);
  g_node_destroy (tree_store->root);
  _btk_tree_data_list_header_free (tree_store->sort_list);
  g_free (tree_store->column_headers);

  if (tree_store->default_sort_destroy)
    {
      GDestroyNotify d = tree_store->default_sort_destroy;

      tree_store->default_sort_destroy = NULL;
      d (tree_store->default_sort_data);
      tree_store->default_sort_data = NULL;
    }

  /* must chain up */
  G_OBJECT_CLASS (btk_tree_store_parent_class)->finalize (object);
}

/* fulfill the BtkTreeModel requirements */
/* NOTE: BtkTreeStore::root is a GNode, that acts as the parent node.  However,
 * it is not visible to the tree or to the user., and the path "0" refers to the
 * first child of BtkTreeStore::root.
 */


static BtkTreeModelFlags
btk_tree_store_get_flags (BtkTreeModel *tree_model)
{
  return BTK_TREE_MODEL_ITERS_PERSIST;
}

static gint
btk_tree_store_get_n_columns (BtkTreeModel *tree_model)
{
  BtkTreeStore *tree_store = (BtkTreeStore *) tree_model;

  tree_store->columns_dirty = TRUE;

  return tree_store->n_columns;
}

static GType
btk_tree_store_get_column_type (BtkTreeModel *tree_model,
				gint          index)
{
  BtkTreeStore *tree_store = (BtkTreeStore *) tree_model;

  g_return_val_if_fail (index < tree_store->n_columns, G_TYPE_INVALID);

  tree_store->columns_dirty = TRUE;

  return tree_store->column_headers[index];
}

static gboolean
btk_tree_store_get_iter (BtkTreeModel *tree_model,
			 BtkTreeIter  *iter,
			 BtkTreePath  *path)
{
  BtkTreeStore *tree_store = (BtkTreeStore *) tree_model;
  BtkTreeIter parent;
  gint *indices;
  gint depth, i;

  tree_store->columns_dirty = TRUE;

  indices = btk_tree_path_get_indices (path);
  depth = btk_tree_path_get_depth (path);

  g_return_val_if_fail (depth > 0, FALSE);

  parent.stamp = tree_store->stamp;
  parent.user_data = tree_store->root;

  if (!btk_tree_store_iter_nth_child (tree_model, iter, &parent, indices[0]))
    return FALSE;

  for (i = 1; i < depth; i++)
    {
      parent = *iter;
      if (!btk_tree_store_iter_nth_child (tree_model, iter, &parent, indices[i]))
	return FALSE;
    }

  return TRUE;
}

static BtkTreePath *
btk_tree_store_get_path (BtkTreeModel *tree_model,
			 BtkTreeIter  *iter)
{
  BtkTreeStore *tree_store = (BtkTreeStore *) tree_model;
  BtkTreePath *retval;
  GNode *tmp_node;
  gint i = 0;

  g_return_val_if_fail (iter->user_data != NULL, NULL);
  g_return_val_if_fail (iter->stamp == tree_store->stamp, NULL);

  validate_tree (tree_store);

  if (G_NODE (iter->user_data)->parent == NULL &&
      G_NODE (iter->user_data) == tree_store->root)
    return btk_tree_path_new ();
  g_assert (G_NODE (iter->user_data)->parent != NULL);

  if (G_NODE (iter->user_data)->parent == G_NODE (tree_store->root))
    {
      retval = btk_tree_path_new ();
      tmp_node = G_NODE (tree_store->root)->children;
    }
  else
    {
      BtkTreeIter tmp_iter = *iter;

      tmp_iter.user_data = G_NODE (iter->user_data)->parent;

      retval = btk_tree_store_get_path (tree_model, &tmp_iter);
      tmp_node = G_NODE (iter->user_data)->parent->children;
    }

  if (retval == NULL)
    return NULL;

  if (tmp_node == NULL)
    {
      btk_tree_path_free (retval);
      return NULL;
    }

  for (; tmp_node; tmp_node = tmp_node->next)
    {
      if (tmp_node == G_NODE (iter->user_data))
	break;
      i++;
    }

  if (tmp_node == NULL)
    {
      /* We couldn't find node, meaning it's prolly not ours */
      /* Perhaps I should do a g_return_if_fail here. */
      btk_tree_path_free (retval);
      return NULL;
    }

  btk_tree_path_append_index (retval, i);

  return retval;
}


static void
btk_tree_store_get_value (BtkTreeModel *tree_model,
			  BtkTreeIter  *iter,
			  gint          column,
			  GValue       *value)
{
  BtkTreeStore *tree_store = (BtkTreeStore *) tree_model;
  BtkTreeDataList *list;
  gint tmp_column = column;

  g_return_if_fail (column < tree_store->n_columns);
  g_return_if_fail (VALID_ITER (iter, tree_store));

  list = G_NODE (iter->user_data)->data;

  while (tmp_column-- > 0 && list)
    list = list->next;

  if (list)
    {
      _btk_tree_data_list_node_to_value (list,
					 tree_store->column_headers[column],
					 value);
    }
  else
    {
      /* We want to return an initialized but empty (default) value */
      g_value_init (value, tree_store->column_headers[column]);
    }
}

static gboolean
btk_tree_store_iter_next (BtkTreeModel  *tree_model,
			  BtkTreeIter   *iter)
{
  g_return_val_if_fail (iter->user_data != NULL, FALSE);
  g_return_val_if_fail (iter->stamp == BTK_TREE_STORE (tree_model)->stamp, FALSE);

  if (G_NODE (iter->user_data)->next)
    {
      iter->user_data = G_NODE (iter->user_data)->next;
      return TRUE;
    }
  else
    {
      iter->stamp = 0;
      return FALSE;
    }
}

static gboolean
btk_tree_store_iter_children (BtkTreeModel *tree_model,
			      BtkTreeIter  *iter,
			      BtkTreeIter  *parent)
{
  BtkTreeStore *tree_store = (BtkTreeStore *) tree_model;
  GNode *children;

  if (parent)
    g_return_val_if_fail (VALID_ITER (parent, tree_store), FALSE);

  if (parent)
    children = G_NODE (parent->user_data)->children;
  else
    children = G_NODE (tree_store->root)->children;

  if (children)
    {
      iter->stamp = tree_store->stamp;
      iter->user_data = children;
      return TRUE;
    }
  else
    {
      iter->stamp = 0;
      return FALSE;
    }
}

static gboolean
btk_tree_store_iter_has_child (BtkTreeModel *tree_model,
			       BtkTreeIter  *iter)
{
  g_return_val_if_fail (iter->user_data != NULL, FALSE);
  g_return_val_if_fail (VALID_ITER (iter, tree_model), FALSE);

  return G_NODE (iter->user_data)->children != NULL;
}

static gint
btk_tree_store_iter_n_children (BtkTreeModel *tree_model,
				BtkTreeIter  *iter)
{
  GNode *node;
  gint i = 0;

  g_return_val_if_fail (iter == NULL || iter->user_data != NULL, 0);

  if (iter == NULL)
    node = G_NODE (BTK_TREE_STORE (tree_model)->root)->children;
  else
    node = G_NODE (iter->user_data)->children;

  while (node)
    {
      i++;
      node = node->next;
    }

  return i;
}

static gboolean
btk_tree_store_iter_nth_child (BtkTreeModel *tree_model,
			       BtkTreeIter  *iter,
			       BtkTreeIter  *parent,
			       gint          n)
{
  BtkTreeStore *tree_store = (BtkTreeStore *) tree_model;
  GNode *parent_node;
  GNode *child;

  g_return_val_if_fail (parent == NULL || parent->user_data != NULL, FALSE);

  if (parent == NULL)
    parent_node = tree_store->root;
  else
    parent_node = parent->user_data;

  child = g_node_nth_child (parent_node, n);

  if (child)
    {
      iter->user_data = child;
      iter->stamp = tree_store->stamp;
      return TRUE;
    }
  else
    {
      iter->stamp = 0;
      return FALSE;
    }
}

static gboolean
btk_tree_store_iter_parent (BtkTreeModel *tree_model,
			    BtkTreeIter  *iter,
			    BtkTreeIter  *child)
{
  BtkTreeStore *tree_store = (BtkTreeStore *) tree_model;
  GNode *parent;

  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (VALID_ITER (child, tree_store), FALSE);

  parent = G_NODE (child->user_data)->parent;

  g_assert (parent != NULL);

  if (parent != tree_store->root)
    {
      iter->user_data = parent;
      iter->stamp = tree_store->stamp;
      return TRUE;
    }
  else
    {
      iter->stamp = 0;
      return FALSE;
    }
}


/* Does not emit a signal */
static gboolean
btk_tree_store_real_set_value (BtkTreeStore *tree_store,
			       BtkTreeIter  *iter,
			       gint          column,
			       GValue       *value,
			       gboolean      sort)
{
  BtkTreeDataList *list;
  BtkTreeDataList *prev;
  gint old_column = column;
  GValue real_value = {0, };
  gboolean converted = FALSE;
  gboolean retval = FALSE;

  if (! g_type_is_a (G_VALUE_TYPE (value), tree_store->column_headers[column]))
    {
      if (! (g_value_type_compatible (G_VALUE_TYPE (value), tree_store->column_headers[column]) &&
	     g_value_type_compatible (tree_store->column_headers[column], G_VALUE_TYPE (value))))
	{
	  g_warning ("%s: Unable to convert from %s to %s\n",
		     B_STRLOC,
		     g_type_name (G_VALUE_TYPE (value)),
		     g_type_name (tree_store->column_headers[column]));
	  return retval;
	}
      if (!g_value_transform (value, &real_value))
	{
	  g_warning ("%s: Unable to make conversion from %s to %s\n",
		     B_STRLOC,
		     g_type_name (G_VALUE_TYPE (value)),
		     g_type_name (tree_store->column_headers[column]));
	  g_value_unset (&real_value);
	  return retval;
	}
      converted = TRUE;
    }

  prev = list = G_NODE (iter->user_data)->data;

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
	    g_value_unset (&real_value);
          if (sort && BTK_TREE_STORE_IS_SORTED (tree_store))
            btk_tree_store_sort_iter_changed (tree_store, iter, old_column, TRUE);
	  return retval;
	}

      column--;
      prev = list;
      list = list->next;
    }

  if (G_NODE (iter->user_data)->data == NULL)
    {
      G_NODE (iter->user_data)->data = list = _btk_tree_data_list_alloc ();
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
    g_value_unset (&real_value);

  if (sort && BTK_TREE_STORE_IS_SORTED (tree_store))
    btk_tree_store_sort_iter_changed (tree_store, iter, old_column, TRUE);

  return retval;
}

/**
 * btk_tree_store_set_value:
 * @tree_store: a #BtkTreeStore
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
btk_tree_store_set_value (BtkTreeStore *tree_store,
			  BtkTreeIter  *iter,
			  gint          column,
			  GValue       *value)
{
  g_return_if_fail (BTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (VALID_ITER (iter, tree_store));
  g_return_if_fail (column >= 0 && column < tree_store->n_columns);
  g_return_if_fail (G_IS_VALUE (value));

  if (btk_tree_store_real_set_value (tree_store, iter, column, value, TRUE))
    {
      BtkTreePath *path;

      path = btk_tree_store_get_path (BTK_TREE_MODEL (tree_store), iter);
      btk_tree_model_row_changed (BTK_TREE_MODEL (tree_store), path, iter);
      btk_tree_path_free (path);
    }
}

static BtkTreeIterCompareFunc
btk_tree_store_get_compare_func (BtkTreeStore *tree_store)
{
  BtkTreeIterCompareFunc func = NULL;

  if (BTK_TREE_STORE_IS_SORTED (tree_store))
    {
      if (tree_store->sort_column_id != -1)
	{
	  BtkTreeDataSortHeader *header;
	  header = _btk_tree_data_list_get_header (tree_store->sort_list,
						   tree_store->sort_column_id);
	  g_return_val_if_fail (header != NULL, NULL);
	  g_return_val_if_fail (header->func != NULL, NULL);
	  func = header->func;
	}
      else
	{
	  func = tree_store->default_sort_func;
	}
    }

  return func;
}

static void
btk_tree_store_set_vector_internal (BtkTreeStore *tree_store,
				    BtkTreeIter  *iter,
				    gboolean     *emit_signal,
				    gboolean     *maybe_need_sort,
				    gint         *columns,
				    GValue       *values,
				    gint          n_values)
{
  gint i;
  BtkTreeIterCompareFunc func = NULL;

  func = btk_tree_store_get_compare_func (tree_store);
  if (func != _btk_tree_data_list_compare_func)
    *maybe_need_sort = TRUE;

  for (i = 0; i < n_values; i++)
    {
      *emit_signal = btk_tree_store_real_set_value (tree_store, iter,
						    columns[i], &values[i],
						    FALSE) || *emit_signal;

      if (func == _btk_tree_data_list_compare_func &&
	  columns[i] == tree_store->sort_column_id)
	*maybe_need_sort = TRUE;
    }
}

static void
btk_tree_store_set_valist_internal (BtkTreeStore *tree_store,
                                    BtkTreeIter  *iter,
                                    gboolean     *emit_signal,
                                    gboolean     *maybe_need_sort,
                                    va_list       var_args)
{
  gint column;
  BtkTreeIterCompareFunc func = NULL;

  column = va_arg (var_args, gint);

  func = btk_tree_store_get_compare_func (tree_store);
  if (func != _btk_tree_data_list_compare_func)
    *maybe_need_sort = TRUE;

  while (column != -1)
    {
      GValue value = { 0, };
      gchar *error = NULL;

      if (column < 0 || column >= tree_store->n_columns)
	{
	  g_warning ("%s: Invalid column number %d added to iter (remember to end your list of columns with a -1)", B_STRLOC, column);
	  break;
	}
      g_value_init (&value, tree_store->column_headers[column]);

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

      *emit_signal = btk_tree_store_real_set_value (tree_store,
						    iter,
						    column,
						    &value,
						    FALSE) || *emit_signal;

      if (func == _btk_tree_data_list_compare_func &&
	  column == tree_store->sort_column_id)
	*maybe_need_sort = TRUE;

      g_value_unset (&value);

      column = va_arg (var_args, gint);
    }
}

/**
 * btk_tree_store_set_valuesv:
 * @tree_store: A #BtkTreeStore
 * @iter: A valid #BtkTreeIter for the row being modified
 * @columns: (array length=n_values): an array of column numbers
 * @values: (array length=n_values): an array of GValues
 * @n_values: the length of the @columns and @values arrays
 *
 * A variant of btk_tree_store_set_valist() which takes
 * the columns and values as two arrays, instead of varargs.  This
 * function is mainly intended for language bindings or in case
 * the number of columns to change is not known until run-time.
 *
 * Since: 2.12
 **/
void
btk_tree_store_set_valuesv (BtkTreeStore *tree_store,
			    BtkTreeIter  *iter,
			    gint         *columns,
			    GValue       *values,
			    gint          n_values)
{
  gboolean emit_signal = FALSE;
  gboolean maybe_need_sort = FALSE;

  g_return_if_fail (BTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (VALID_ITER (iter, tree_store));

  btk_tree_store_set_vector_internal (tree_store, iter,
				      &emit_signal,
				      &maybe_need_sort,
				      columns, values, n_values);

  if (maybe_need_sort && BTK_TREE_STORE_IS_SORTED (tree_store))
    btk_tree_store_sort_iter_changed (tree_store, iter, tree_store->sort_column_id, TRUE);

  if (emit_signal)
    {
      BtkTreePath *path;

      path = btk_tree_store_get_path (BTK_TREE_MODEL (tree_store), iter);
      btk_tree_model_row_changed (BTK_TREE_MODEL (tree_store), path, iter);
      btk_tree_path_free (path);
    }
}

/**
 * btk_tree_store_set_valist:
 * @tree_store: A #BtkTreeStore
 * @iter: A valid #BtkTreeIter for the row being modified
 * @var_args: <type>va_list</type> of column/value pairs
 *
 * See btk_tree_store_set(); this version takes a <type>va_list</type> for
 * use by language bindings.
 *
 **/
void
btk_tree_store_set_valist (BtkTreeStore *tree_store,
                           BtkTreeIter  *iter,
                           va_list       var_args)
{
  gboolean emit_signal = FALSE;
  gboolean maybe_need_sort = FALSE;

  g_return_if_fail (BTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (VALID_ITER (iter, tree_store));

  btk_tree_store_set_valist_internal (tree_store, iter,
				      &emit_signal,
				      &maybe_need_sort,
				      var_args);

  if (maybe_need_sort && BTK_TREE_STORE_IS_SORTED (tree_store))
    btk_tree_store_sort_iter_changed (tree_store, iter, tree_store->sort_column_id, TRUE);

  if (emit_signal)
    {
      BtkTreePath *path;

      path = btk_tree_store_get_path (BTK_TREE_MODEL (tree_store), iter);
      btk_tree_model_row_changed (BTK_TREE_MODEL (tree_store), path, iter);
      btk_tree_path_free (path);
    }
}

/**
 * btk_tree_store_set:
 * @tree_store: A #BtkTreeStore
 * @iter: A valid #BtkTreeIter for the row being modified
 * @Varargs: pairs of column number and value, terminated with -1
 *
 * Sets the value of one or more cells in the row referenced by @iter.
 * The variable argument list should contain integer column numbers,
 * each column number followed by the value to be set. 
 * The list is terminated by a -1. For example, to set column 0 with type
 * %G_TYPE_STRING to "Foo", you would write 
 * <literal>btk_tree_store_set (store, iter, 0, "Foo", -1)</literal>.
 *
 * The value will be referenced by the store if it is a %G_TYPE_OBJECT, and it
 * will be copied if it is a %G_TYPE_STRING or %G_TYPE_BOXED.
 **/
void
btk_tree_store_set (BtkTreeStore *tree_store,
		    BtkTreeIter  *iter,
		    ...)
{
  va_list var_args;

  va_start (var_args, iter);
  btk_tree_store_set_valist (tree_store, iter, var_args);
  va_end (var_args);
}

/**
 * btk_tree_store_remove:
 * @tree_store: A #BtkTreeStore
 * @iter: A valid #BtkTreeIter
 * 
 * Removes @iter from @tree_store.  After being removed, @iter is set to the
 * next valid row at that level, or invalidated if it previously pointed to the
 * last one.
 *
 * Return value: %TRUE if @iter is still valid, %FALSE if not.
 **/
gboolean
btk_tree_store_remove (BtkTreeStore *tree_store,
		       BtkTreeIter  *iter)
{
  BtkTreePath *path;
  BtkTreeIter new_iter = {0,};
  GNode *parent;
  GNode *next_node;

  g_return_val_if_fail (BTK_IS_TREE_STORE (tree_store), FALSE);
  g_return_val_if_fail (VALID_ITER (iter, tree_store), FALSE);

  parent = G_NODE (iter->user_data)->parent;

  g_assert (parent != NULL);
  next_node = G_NODE (iter->user_data)->next;

  if (G_NODE (iter->user_data)->data)
    g_node_traverse (G_NODE (iter->user_data), G_POST_ORDER, G_TRAVERSE_ALL,
		     -1, node_free, tree_store->column_headers);

  path = btk_tree_store_get_path (BTK_TREE_MODEL (tree_store), iter);
  g_node_destroy (G_NODE (iter->user_data));

  btk_tree_model_row_deleted (BTK_TREE_MODEL (tree_store), path);

  if (parent != G_NODE (tree_store->root))
    {
      /* child_toggled */
      if (parent->children == NULL)
	{
	  btk_tree_path_up (path);

	  new_iter.stamp = tree_store->stamp;
	  new_iter.user_data = parent;
	  btk_tree_model_row_has_child_toggled (BTK_TREE_MODEL (tree_store), path, &new_iter);
	}
    }
  btk_tree_path_free (path);

  /* revalidate iter */
  if (next_node != NULL)
    {
      iter->stamp = tree_store->stamp;
      iter->user_data = next_node;
      return TRUE;
    }
  else
    {
      iter->stamp = 0;
      iter->user_data = NULL;
    }

  return FALSE;
}

/**
 * btk_tree_store_insert:
 * @tree_store: A #BtkTreeStore
 * @iter: (out): An unset #BtkTreeIter to set to the new row
 * @parent: (allow-none): A valid #BtkTreeIter, or %NULL
 * @position: position to insert the new row
 *
 * Creates a new row at @position.  If parent is non-%NULL, then the row will be
 * made a child of @parent.  Otherwise, the row will be created at the toplevel.
 * If @position is larger than the number of rows at that level, then the new
 * row will be inserted to the end of the list.  @iter will be changed to point
 * to this new row.  The row will be empty after this function is called.  To
 * fill in values, you need to call btk_tree_store_set() or
 * btk_tree_store_set_value().
 *
 **/
void
btk_tree_store_insert (BtkTreeStore *tree_store,
		       BtkTreeIter  *iter,
		       BtkTreeIter  *parent,
		       gint          position)
{
  BtkTreePath *path;
  GNode *parent_node;
  GNode *new_node;

  g_return_if_fail (BTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (iter != NULL);
  if (parent)
    g_return_if_fail (VALID_ITER (parent, tree_store));

  if (parent)
    parent_node = parent->user_data;
  else
    parent_node = tree_store->root;

  tree_store->columns_dirty = TRUE;

  new_node = g_node_new (NULL);

  iter->stamp = tree_store->stamp;
  iter->user_data = new_node;
  g_node_insert (parent_node, position, new_node);

  path = btk_tree_store_get_path (BTK_TREE_MODEL (tree_store), iter);
  btk_tree_model_row_inserted (BTK_TREE_MODEL (tree_store), path, iter);

  if (parent_node != tree_store->root)
    {
      if (new_node->prev == NULL && new_node->next == NULL)
        {
          btk_tree_path_up (path);
          btk_tree_model_row_has_child_toggled (BTK_TREE_MODEL (tree_store), path, parent);
        }
    }

  btk_tree_path_free (path);

  validate_tree ((BtkTreeStore*)tree_store);
}

/**
 * btk_tree_store_insert_before:
 * @tree_store: A #BtkTreeStore
 * @iter: (out): An unset #BtkTreeIter to set to the new row
 * @parent: (allow-none): A valid #BtkTreeIter, or %NULL
 * @sibling: (allow-none): A valid #BtkTreeIter, or %NULL
 *
 * Inserts a new row before @sibling.  If @sibling is %NULL, then the row will
 * be appended to @parent 's children.  If @parent and @sibling are %NULL, then
 * the row will be appended to the toplevel.  If both @sibling and @parent are
 * set, then @parent must be the parent of @sibling.  When @sibling is set,
 * @parent is optional.
 *
 * @iter will be changed to point to this new row.  The row will be empty after
 * this function is called.  To fill in values, you need to call
 * btk_tree_store_set() or btk_tree_store_set_value().
 *
 **/
void
btk_tree_store_insert_before (BtkTreeStore *tree_store,
			      BtkTreeIter  *iter,
			      BtkTreeIter  *parent,
			      BtkTreeIter  *sibling)
{
  BtkTreePath *path;
  GNode *parent_node = NULL;
  GNode *new_node;

  g_return_if_fail (BTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (iter != NULL);
  if (parent != NULL)
    g_return_if_fail (VALID_ITER (parent, tree_store));
  if (sibling != NULL)
    g_return_if_fail (VALID_ITER (sibling, tree_store));

  if (parent == NULL && sibling == NULL)
    parent_node = tree_store->root;
  else if (parent == NULL)
    parent_node = G_NODE (sibling->user_data)->parent;
  else if (sibling == NULL)
    parent_node = G_NODE (parent->user_data);
  else
    {
      g_return_if_fail (G_NODE (sibling->user_data)->parent == G_NODE (parent->user_data));
      parent_node = G_NODE (parent->user_data);
    }

  tree_store->columns_dirty = TRUE;

  new_node = g_node_new (NULL);

  g_node_insert_before (parent_node,
			sibling ? G_NODE (sibling->user_data) : NULL,
                        new_node);

  iter->stamp = tree_store->stamp;
  iter->user_data = new_node;

  path = btk_tree_store_get_path (BTK_TREE_MODEL (tree_store), iter);
  btk_tree_model_row_inserted (BTK_TREE_MODEL (tree_store), path, iter);

  if (parent_node != tree_store->root)
    {
      if (new_node->prev == NULL && new_node->next == NULL)
        {
          BtkTreeIter parent_iter;

          parent_iter.stamp = tree_store->stamp;
          parent_iter.user_data = parent_node;

          btk_tree_path_up (path);
          btk_tree_model_row_has_child_toggled (BTK_TREE_MODEL (tree_store), path, &parent_iter);
        }
    }

  btk_tree_path_free (path);

  validate_tree (tree_store);
}

/**
 * btk_tree_store_insert_after:
 * @tree_store: A #BtkTreeStore
 * @iter: (out): An unset #BtkTreeIter to set to the new row
 * @parent: (allow-none): A valid #BtkTreeIter, or %NULL
 * @sibling: (allow-none): A valid #BtkTreeIter, or %NULL
 *
 * Inserts a new row after @sibling.  If @sibling is %NULL, then the row will be
 * prepended to @parent 's children.  If @parent and @sibling are %NULL, then
 * the row will be prepended to the toplevel.  If both @sibling and @parent are
 * set, then @parent must be the parent of @sibling.  When @sibling is set,
 * @parent is optional.
 *
 * @iter will be changed to point to this new row.  The row will be empty after
 * this function is called.  To fill in values, you need to call
 * btk_tree_store_set() or btk_tree_store_set_value().
 *
 **/
void
btk_tree_store_insert_after (BtkTreeStore *tree_store,
			     BtkTreeIter  *iter,
			     BtkTreeIter  *parent,
			     BtkTreeIter  *sibling)
{
  BtkTreePath *path;
  GNode *parent_node;
  GNode *new_node;

  g_return_if_fail (BTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (iter != NULL);
  if (parent != NULL)
    g_return_if_fail (VALID_ITER (parent, tree_store));
  if (sibling != NULL)
    g_return_if_fail (VALID_ITER (sibling, tree_store));

  if (parent == NULL && sibling == NULL)
    parent_node = tree_store->root;
  else if (parent == NULL)
    parent_node = G_NODE (sibling->user_data)->parent;
  else if (sibling == NULL)
    parent_node = G_NODE (parent->user_data);
  else
    {
      g_return_if_fail (G_NODE (sibling->user_data)->parent ==
                        G_NODE (parent->user_data));
      parent_node = G_NODE (parent->user_data);
    }

  tree_store->columns_dirty = TRUE;

  new_node = g_node_new (NULL);

  g_node_insert_after (parent_node,
		       sibling ? G_NODE (sibling->user_data) : NULL,
                       new_node);

  iter->stamp = tree_store->stamp;
  iter->user_data = new_node;

  path = btk_tree_store_get_path (BTK_TREE_MODEL (tree_store), iter);
  btk_tree_model_row_inserted (BTK_TREE_MODEL (tree_store), path, iter);

  if (parent_node != tree_store->root)
    {
      if (new_node->prev == NULL && new_node->next == NULL)
        {
          BtkTreeIter parent_iter;

          parent_iter.stamp = tree_store->stamp;
          parent_iter.user_data = parent_node;

          btk_tree_path_up (path);
          btk_tree_model_row_has_child_toggled (BTK_TREE_MODEL (tree_store), path, &parent_iter);
        }
    }

  btk_tree_path_free (path);

  validate_tree (tree_store);
}

/**
 * btk_tree_store_insert_with_values:
 * @tree_store: A #BtkTreeStore
 * @iter: (out) (allow-none): An unset #BtkTreeIter to set the new row, or %NULL.
 * @parent: (allow-none): A valid #BtkTreeIter, or %NULL
 * @position: position to insert the new row
 * @Varargs: pairs of column number and value, terminated with -1
 *
 * Creates a new row at @position.  @iter will be changed to point to this
 * new row.  If @position is larger than the number of rows on the list, then
 * the new row will be appended to the list.  The row will be filled with
 * the values given to this function.
 *
 * Calling
 * <literal>btk_tree_store_insert_with_values (tree_store, iter, position, ...)</literal>
 * has the same effect as calling
 * |[
 * btk_tree_store_insert (tree_store, iter, position);
 * btk_tree_store_set (tree_store, iter, ...);
 * ]|
 * with the different that the former will only emit a row_inserted signal,
 * while the latter will emit row_inserted, row_changed and if the tree store
 * is sorted, rows_reordered.  Since emitting the rows_reordered signal
 * repeatedly can affect the performance of the program,
 * btk_tree_store_insert_with_values() should generally be preferred when
 * inserting rows in a sorted tree store.
 *
 * Since: 2.10
 */
void
btk_tree_store_insert_with_values (BtkTreeStore *tree_store,
				   BtkTreeIter  *iter,
				   BtkTreeIter  *parent,
				   gint          position,
				   ...)
{
  BtkTreePath *path;
  GNode *parent_node;
  GNode *new_node;
  BtkTreeIter tmp_iter;
  va_list var_args;
  gboolean changed = FALSE;
  gboolean maybe_need_sort = FALSE;

  g_return_if_fail (BTK_IS_TREE_STORE (tree_store));

  if (!iter)
    iter = &tmp_iter;

  if (parent)
    g_return_if_fail (VALID_ITER (parent, tree_store));

  if (parent)
    parent_node = parent->user_data;
  else
    parent_node = tree_store->root;

  tree_store->columns_dirty = TRUE;

  new_node = g_node_new (NULL);

  iter->stamp = tree_store->stamp;
  iter->user_data = new_node;
  g_node_insert (parent_node, position, new_node);

  va_start (var_args, position);
  btk_tree_store_set_valist_internal (tree_store, iter,
				      &changed, &maybe_need_sort,
				      var_args);
  va_end (var_args);

  if (maybe_need_sort && BTK_TREE_STORE_IS_SORTED (tree_store))
    btk_tree_store_sort_iter_changed (tree_store, iter, tree_store->sort_column_id, FALSE);

  path = btk_tree_store_get_path (BTK_TREE_MODEL (tree_store), iter);
  btk_tree_model_row_inserted (BTK_TREE_MODEL (tree_store), path, iter);

  if (parent_node != tree_store->root)
    {
      if (new_node->prev == NULL && new_node->next == NULL)
        {
	  btk_tree_path_up (path);
	  btk_tree_model_row_has_child_toggled (BTK_TREE_MODEL (tree_store), path, parent);
	}
    }

  btk_tree_path_free (path);

  validate_tree ((BtkTreeStore *)tree_store);
}

/**
 * btk_tree_store_insert_with_valuesv:
 * @tree_store: A #BtkTreeStore
 * @iter: (out) (allow-none): An unset #BtkTreeIter to set the new row, or %NULL.
 * @parent: (allow-none): A valid #BtkTreeIter, or %NULL
 * @position: position to insert the new row
 * @columns: (array length=n_values): an array of column numbers
 * @values: (array length=n_values): an array of GValues
 * @n_values: the length of the @columns and @values arrays
 *
 * A variant of btk_tree_store_insert_with_values() which takes
 * the columns and values as two arrays, instead of varargs.  This
 * function is mainly intended for language bindings.
 *
 * Since: 2.10
 */
void
btk_tree_store_insert_with_valuesv (BtkTreeStore *tree_store,
				    BtkTreeIter  *iter,
				    BtkTreeIter  *parent,
				    gint          position,
				    gint         *columns,
				    GValue       *values,
				    gint          n_values)
{
  BtkTreePath *path;
  GNode *parent_node;
  GNode *new_node;
  BtkTreeIter tmp_iter;
  gboolean changed = FALSE;
  gboolean maybe_need_sort = FALSE;

  g_return_if_fail (BTK_IS_TREE_STORE (tree_store));

  if (!iter)
    iter = &tmp_iter;

  if (parent)
    g_return_if_fail (VALID_ITER (parent, tree_store));

  if (parent)
    parent_node = parent->user_data;
  else
    parent_node = tree_store->root;

  tree_store->columns_dirty = TRUE;

  new_node = g_node_new (NULL);

  iter->stamp = tree_store->stamp;
  iter->user_data = new_node;
  g_node_insert (parent_node, position, new_node);

  btk_tree_store_set_vector_internal (tree_store, iter,
				      &changed, &maybe_need_sort,
				      columns, values, n_values);

  if (maybe_need_sort && BTK_TREE_STORE_IS_SORTED (tree_store))
    btk_tree_store_sort_iter_changed (tree_store, iter, tree_store->sort_column_id, FALSE);

  path = btk_tree_store_get_path (BTK_TREE_MODEL (tree_store), iter);
  btk_tree_model_row_inserted (BTK_TREE_MODEL (tree_store), path, iter);

  if (parent_node != tree_store->root)
    {
      if (new_node->prev == NULL && new_node->next == NULL)
        {
	  btk_tree_path_up (path);
	  btk_tree_model_row_has_child_toggled (BTK_TREE_MODEL (tree_store), path, parent);
	}
    }

  btk_tree_path_free (path);

  validate_tree ((BtkTreeStore *)tree_store);
}

/**
 * btk_tree_store_prepend:
 * @tree_store: A #BtkTreeStore
 * @iter: (out): An unset #BtkTreeIter to set to the prepended row
 * @parent: (allow-none): A valid #BtkTreeIter, or %NULL
 * 
 * Prepends a new row to @tree_store.  If @parent is non-%NULL, then it will prepend
 * the new row before the first child of @parent, otherwise it will prepend a row
 * to the top level.  @iter will be changed to point to this new row.  The row
 * will be empty after this function is called.  To fill in values, you need to
 * call btk_tree_store_set() or btk_tree_store_set_value().
 **/
void
btk_tree_store_prepend (BtkTreeStore *tree_store,
			BtkTreeIter  *iter,
			BtkTreeIter  *parent)
{
  GNode *parent_node;

  g_return_if_fail (BTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (iter != NULL);
  if (parent != NULL)
    g_return_if_fail (VALID_ITER (parent, tree_store));

  tree_store->columns_dirty = TRUE;

  if (parent == NULL)
    parent_node = tree_store->root;
  else
    parent_node = parent->user_data;

  if (parent_node->children == NULL)
    {
      BtkTreePath *path;
      
      iter->stamp = tree_store->stamp;
      iter->user_data = g_node_new (NULL);

      g_node_prepend (parent_node, G_NODE (iter->user_data));

      path = btk_tree_store_get_path (BTK_TREE_MODEL (tree_store), iter);
      btk_tree_model_row_inserted (BTK_TREE_MODEL (tree_store), path, iter);

      if (parent_node != tree_store->root)
	{
	  btk_tree_path_up (path);
	  btk_tree_model_row_has_child_toggled (BTK_TREE_MODEL (tree_store), path, parent);
	}
      btk_tree_path_free (path);
    }
  else
    {
      btk_tree_store_insert_after (tree_store, iter, parent, NULL);
    }

  validate_tree (tree_store);
}

/**
 * btk_tree_store_append:
 * @tree_store: A #BtkTreeStore
 * @iter: (out): An unset #BtkTreeIter to set to the appended row
 * @parent: (allow-none): A valid #BtkTreeIter, or %NULL
 * 
 * Appends a new row to @tree_store.  If @parent is non-%NULL, then it will append the
 * new row after the last child of @parent, otherwise it will append a row to
 * the top level.  @iter will be changed to point to this new row.  The row will
 * be empty after this function is called.  To fill in values, you need to call
 * btk_tree_store_set() or btk_tree_store_set_value().
 **/
void
btk_tree_store_append (BtkTreeStore *tree_store,
		       BtkTreeIter  *iter,
		       BtkTreeIter  *parent)
{
  GNode *parent_node;

  g_return_if_fail (BTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (iter != NULL);
  if (parent != NULL)
    g_return_if_fail (VALID_ITER (parent, tree_store));

  if (parent == NULL)
    parent_node = tree_store->root;
  else
    parent_node = parent->user_data;

  tree_store->columns_dirty = TRUE;

  if (parent_node->children == NULL)
    {
      BtkTreePath *path;

      iter->stamp = tree_store->stamp;
      iter->user_data = g_node_new (NULL);

      g_node_append (parent_node, G_NODE (iter->user_data));

      path = btk_tree_store_get_path (BTK_TREE_MODEL (tree_store), iter);
      btk_tree_model_row_inserted (BTK_TREE_MODEL (tree_store), path, iter);

      if (parent_node != tree_store->root)
	{
	  btk_tree_path_up (path);
	  btk_tree_model_row_has_child_toggled (BTK_TREE_MODEL (tree_store), path, parent);
	}
      btk_tree_path_free (path);
    }
  else
    {
      btk_tree_store_insert_before (tree_store, iter, parent, NULL);
    }

  validate_tree (tree_store);
}

/**
 * btk_tree_store_is_ancestor:
 * @tree_store: A #BtkTreeStore
 * @iter: A valid #BtkTreeIter
 * @descendant: A valid #BtkTreeIter
 * 
 * Returns %TRUE if @iter is an ancestor of @descendant.  That is, @iter is the
 * parent (or grandparent or great-grandparent) of @descendant.
 * 
 * Return value: %TRUE, if @iter is an ancestor of @descendant
 **/
gboolean
btk_tree_store_is_ancestor (BtkTreeStore *tree_store,
			    BtkTreeIter  *iter,
			    BtkTreeIter  *descendant)
{
  g_return_val_if_fail (BTK_IS_TREE_STORE (tree_store), FALSE);
  g_return_val_if_fail (VALID_ITER (iter, tree_store), FALSE);
  g_return_val_if_fail (VALID_ITER (descendant, tree_store), FALSE);

  return g_node_is_ancestor (G_NODE (iter->user_data),
			     G_NODE (descendant->user_data));
}


/**
 * btk_tree_store_iter_depth:
 * @tree_store: A #BtkTreeStore
 * @iter: A valid #BtkTreeIter
 * 
 * Returns the depth of @iter.  This will be 0 for anything on the root level, 1
 * for anything down a level, etc.
 * 
 * Return value: The depth of @iter
 **/
gint
btk_tree_store_iter_depth (BtkTreeStore *tree_store,
			   BtkTreeIter  *iter)
{
  g_return_val_if_fail (BTK_IS_TREE_STORE (tree_store), 0);
  g_return_val_if_fail (VALID_ITER (iter, tree_store), 0);

  return g_node_depth (G_NODE (iter->user_data)) - 2;
}

/* simple ripoff from g_node_traverse_post_order */
static gboolean
btk_tree_store_clear_traverse (GNode        *node,
			       BtkTreeStore *store)
{
  BtkTreeIter iter;

  if (node->children)
    {
      GNode *child;

      child = node->children;
      while (child)
        {
	  register GNode *current;

	  current = child;
	  child = current->next;
	  if (btk_tree_store_clear_traverse (current, store))
	    return TRUE;
	}

      if (node->parent)
        {
	  iter.stamp = store->stamp;
	  iter.user_data = node;

	  btk_tree_store_remove (store, &iter);
	}
    }
  else if (node->parent)
    {
      iter.stamp = store->stamp;
      iter.user_data = node;

      btk_tree_store_remove (store, &iter);
    }

  return FALSE;
}

static void
btk_tree_store_increment_stamp (BtkTreeStore *tree_store)
{
  do
    {
      tree_store->stamp++;
    }
  while (tree_store->stamp == 0);
}

/**
 * btk_tree_store_clear:
 * @tree_store: a #BtkTreeStore
 * 
 * Removes all rows from @tree_store
 **/
void
btk_tree_store_clear (BtkTreeStore *tree_store)
{
  g_return_if_fail (BTK_IS_TREE_STORE (tree_store));

  btk_tree_store_clear_traverse (tree_store->root, tree_store);
  btk_tree_store_increment_stamp (tree_store);
}

static gboolean
btk_tree_store_iter_is_valid_helper (BtkTreeIter *iter,
				     GNode       *first)
{
  GNode *node;

  node = first;

  do
    {
      if (node == iter->user_data)
	return TRUE;

      if (node->children)
	if (btk_tree_store_iter_is_valid_helper (iter, node->children))
	  return TRUE;

      node = node->next;
    }
  while (node);

  return FALSE;
}

/**
 * btk_tree_store_iter_is_valid:
 * @tree_store: A #BtkTreeStore.
 * @iter: A #BtkTreeIter.
 *
 * WARNING: This function is slow. Only use it for debugging and/or testing
 * purposes.
 *
 * Checks if the given iter is a valid iter for this #BtkTreeStore.
 *
 * Return value: %TRUE if the iter is valid, %FALSE if the iter is invalid.
 *
 * Since: 2.2
 **/
gboolean
btk_tree_store_iter_is_valid (BtkTreeStore *tree_store,
                              BtkTreeIter  *iter)
{
  g_return_val_if_fail (BTK_IS_TREE_STORE (tree_store), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  if (!VALID_ITER (iter, tree_store))
    return FALSE;

  return btk_tree_store_iter_is_valid_helper (iter, tree_store->root);
}

/* DND */


static gboolean real_btk_tree_store_row_draggable (BtkTreeDragSource *drag_source,
                                                   BtkTreePath       *path)
{
  return TRUE;
}
               
static gboolean
btk_tree_store_drag_data_delete (BtkTreeDragSource *drag_source,
                                 BtkTreePath       *path)
{
  BtkTreeIter iter;

  if (btk_tree_store_get_iter (BTK_TREE_MODEL (drag_source),
                               &iter,
                               path))
    {
      btk_tree_store_remove (BTK_TREE_STORE (drag_source),
                             &iter);
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

static gboolean
btk_tree_store_drag_data_get (BtkTreeDragSource *drag_source,
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

static void
copy_node_data (BtkTreeStore *tree_store,
                BtkTreeIter  *src_iter,
                BtkTreeIter  *dest_iter)
{
  BtkTreeDataList *dl = G_NODE (src_iter->user_data)->data;
  BtkTreeDataList *copy_head = NULL;
  BtkTreeDataList *copy_prev = NULL;
  BtkTreeDataList *copy_iter = NULL;
  BtkTreePath *path;
  gint col;

  col = 0;
  while (dl)
    {
      copy_iter = _btk_tree_data_list_node_copy (dl, tree_store->column_headers[col]);

      if (copy_head == NULL)
        copy_head = copy_iter;

      if (copy_prev)
        copy_prev->next = copy_iter;

      copy_prev = copy_iter;

      dl = dl->next;
      ++col;
    }

  G_NODE (dest_iter->user_data)->data = copy_head;

  path = btk_tree_store_get_path (BTK_TREE_MODEL (tree_store), dest_iter);
  btk_tree_model_row_changed (BTK_TREE_MODEL (tree_store), path, dest_iter);
  btk_tree_path_free (path);
}

static void
recursive_node_copy (BtkTreeStore *tree_store,
                     BtkTreeIter  *src_iter,
                     BtkTreeIter  *dest_iter)
{
  BtkTreeIter child;
  BtkTreeModel *model;

  model = BTK_TREE_MODEL (tree_store);

  copy_node_data (tree_store, src_iter, dest_iter);

  if (btk_tree_store_iter_children (model,
                                    &child,
                                    src_iter))
    {
      /* Need to create children and recurse. Note our
       * dependence on persistent iterators here.
       */
      do
        {
          BtkTreeIter copy;

          /* Gee, a really slow algorithm... ;-) FIXME */
          btk_tree_store_append (tree_store,
                                 &copy,
                                 dest_iter);

          recursive_node_copy (tree_store, &child, &copy);
        }
      while (btk_tree_store_iter_next (model, &child));
    }
}

static gboolean
btk_tree_store_drag_data_received (BtkTreeDragDest   *drag_dest,
                                   BtkTreePath       *dest,
                                   BtkSelectionData  *selection_data)
{
  BtkTreeModel *tree_model;
  BtkTreeStore *tree_store;
  BtkTreeModel *src_model = NULL;
  BtkTreePath *src_path = NULL;
  gboolean retval = FALSE;

  tree_model = BTK_TREE_MODEL (drag_dest);
  tree_store = BTK_TREE_STORE (drag_dest);

  validate_tree (tree_store);

  if (btk_tree_get_row_drag_data (selection_data,
				  &src_model,
				  &src_path) &&
      src_model == tree_model)
    {
      /* Copy the given row to a new position */
      BtkTreeIter src_iter;
      BtkTreeIter dest_iter;
      BtkTreePath *prev;

      if (!btk_tree_store_get_iter (src_model,
                                    &src_iter,
                                    src_path))
        {
          goto out;
        }

      /* Get the path to insert _after_ (dest is the path to insert _before_) */
      prev = btk_tree_path_copy (dest);

      if (!btk_tree_path_prev (prev))
        {
          BtkTreeIter dest_parent;
          BtkTreePath *parent;
          BtkTreeIter *dest_parent_p;

          /* dest was the first spot at the current depth; which means
           * we are supposed to prepend.
           */

          /* Get the parent, NULL if parent is the root */
          dest_parent_p = NULL;
          parent = btk_tree_path_copy (dest);
          if (btk_tree_path_up (parent) &&
	      btk_tree_path_get_depth (parent) > 0)
            {
              btk_tree_store_get_iter (tree_model,
                                       &dest_parent,
                                       parent);
              dest_parent_p = &dest_parent;
            }
          btk_tree_path_free (parent);
          parent = NULL;

          btk_tree_store_prepend (tree_store,
                                  &dest_iter,
                                  dest_parent_p);

          retval = TRUE;
        }
      else
        {
          if (btk_tree_store_get_iter (tree_model, &dest_iter, prev))
            {
              BtkTreeIter tmp_iter = dest_iter;

              btk_tree_store_insert_after (tree_store, &dest_iter, NULL,
                                           &tmp_iter);

              retval = TRUE;
            }
        }

      btk_tree_path_free (prev);

      /* If we succeeded in creating dest_iter, walk src_iter tree branch,
       * duplicating it below dest_iter.
       */

      if (retval)
        {
          recursive_node_copy (tree_store,
                               &src_iter,
                               &dest_iter);
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
btk_tree_store_row_drop_possible (BtkTreeDragDest  *drag_dest,
                                  BtkTreePath      *dest_path,
				  BtkSelectionData *selection_data)
{
  BtkTreeModel *src_model = NULL;
  BtkTreePath *src_path = NULL;
  BtkTreePath *tmp = NULL;
  gboolean retval = FALSE;
  
  /* don't accept drops if the tree has been sorted */
  if (BTK_TREE_STORE_IS_SORTED (drag_dest))
    return FALSE;

  if (!btk_tree_get_row_drag_data (selection_data,
				   &src_model,
				   &src_path))
    goto out;
    
  /* can only drag to ourselves */
  if (src_model != BTK_TREE_MODEL (drag_dest))
    goto out;

  /* Can't drop into ourself. */
  if (btk_tree_path_is_ancestor (src_path,
                                 dest_path))
    goto out;

  /* Can't drop if dest_path's parent doesn't exist */
  {
    BtkTreeIter iter;

    if (btk_tree_path_get_depth (dest_path) > 1)
      {
	tmp = btk_tree_path_copy (dest_path);
	btk_tree_path_up (tmp);
	
	if (!btk_tree_store_get_iter (BTK_TREE_MODEL (drag_dest),
				      &iter, tmp))
	  goto out;
      }
  }
  
  /* Can otherwise drop anywhere. */
  retval = TRUE;

 out:

  if (src_path)
    btk_tree_path_free (src_path);
  if (tmp)
    btk_tree_path_free (tmp);

  return retval;
}

/* Sorting and reordering */
typedef struct _SortTuple
{
  gint offset;
  GNode *node;
} SortTuple;

/* Reordering */
static gint
btk_tree_store_reorder_func (gconstpointer a,
			     gconstpointer b,
			     gpointer      user_data)
{
  SortTuple *a_reorder;
  SortTuple *b_reorder;

  a_reorder = (SortTuple *)a;
  b_reorder = (SortTuple *)b;

  if (a_reorder->offset < b_reorder->offset)
    return -1;
  if (a_reorder->offset > b_reorder->offset)
    return 1;

  return 0;
}

/**
 * btk_tree_store_reorder:
 * @tree_store: A #BtkTreeStore.
 * @parent: A #BtkTreeIter.
 * @new_order: (array): an array of integers mapping the new position of each child
 *      to its old position before the re-ordering,
 *      i.e. @new_order<literal>[newpos] = oldpos</literal>.
 *
 * Reorders the children of @parent in @tree_store to follow the order
 * indicated by @new_order. Note that this function only works with
 * unsorted stores.
 *
 * Since: 2.2
 **/
void
btk_tree_store_reorder (BtkTreeStore *tree_store,
			BtkTreeIter  *parent,
			gint         *new_order)
{
  gint i, length = 0;
  GNode *level, *node;
  BtkTreePath *path;
  SortTuple *sort_array;

  g_return_if_fail (BTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (!BTK_TREE_STORE_IS_SORTED (tree_store));
  g_return_if_fail (parent == NULL || VALID_ITER (parent, tree_store));
  g_return_if_fail (new_order != NULL);

  if (!parent)
    level = G_NODE (tree_store->root)->children;
  else
    level = G_NODE (parent->user_data)->children;

  /* count nodes */
  node = level;
  while (node)
    {
      length++;
      node = node->next;
    }

  /* set up sortarray */
  sort_array = g_new (SortTuple, length);

  node = level;
  for (i = 0; i < length; i++)
    {
      sort_array[new_order[i]].offset = i;
      sort_array[i].node = node;

      node = node->next;
    }

  g_qsort_with_data (sort_array,
		     length,
		     sizeof (SortTuple),
		     btk_tree_store_reorder_func,
		     NULL);

  /* fix up level */
  for (i = 0; i < length - 1; i++)
    {
      sort_array[i].node->next = sort_array[i+1].node;
      sort_array[i+1].node->prev = sort_array[i].node;
    }

  sort_array[length-1].node->next = NULL;
  sort_array[0].node->prev = NULL;
  if (parent)
    G_NODE (parent->user_data)->children = sort_array[0].node;
  else
    G_NODE (tree_store->root)->children = sort_array[0].node;

  /* emit signal */
  if (parent)
    path = btk_tree_store_get_path (BTK_TREE_MODEL (tree_store), parent);
  else
    path = btk_tree_path_new ();
  btk_tree_model_rows_reordered (BTK_TREE_MODEL (tree_store), path,
				 parent, new_order);
  btk_tree_path_free (path);
  g_free (sort_array);
}

/**
 * btk_tree_store_swap:
 * @tree_store: A #BtkTreeStore.
 * @a: A #BtkTreeIter.
 * @b: Another #BtkTreeIter.
 *
 * Swaps @a and @b in the same level of @tree_store. Note that this function
 * only works with unsorted stores.
 *
 * Since: 2.2
 **/
void
btk_tree_store_swap (BtkTreeStore *tree_store,
		     BtkTreeIter  *a,
		     BtkTreeIter  *b)
{
  GNode *tmp, *node_a, *node_b, *parent_node;
  GNode *a_prev, *a_next, *b_prev, *b_next;
  gint i, a_count, b_count, length, *order;
  BtkTreePath *path_a, *path_b;
  BtkTreeIter parent;

  g_return_if_fail (BTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (VALID_ITER (a, tree_store));
  g_return_if_fail (VALID_ITER (b, tree_store));

  node_a = G_NODE (a->user_data);
  node_b = G_NODE (b->user_data);

  /* basic sanity checking */
  if (node_a == node_b)
    return;

  path_a = btk_tree_store_get_path (BTK_TREE_MODEL (tree_store), a);
  path_b = btk_tree_store_get_path (BTK_TREE_MODEL (tree_store), b);

  g_return_if_fail (path_a && path_b);

  btk_tree_path_up (path_a);
  btk_tree_path_up (path_b);

  if (btk_tree_path_get_depth (path_a) == 0
      || btk_tree_path_get_depth (path_b) == 0)
    {
      if (btk_tree_path_get_depth (path_a) != btk_tree_path_get_depth (path_b))
        {
          btk_tree_path_free (path_a);
          btk_tree_path_free (path_b);
                                                                                
          g_warning ("Given children are not in the same level\n");
          return;
        }
      parent_node = G_NODE (tree_store->root);
    }
  else
    {
      if (btk_tree_path_compare (path_a, path_b))
        {
          btk_tree_path_free (path_a);
          btk_tree_path_free (path_b);
                                                                                
          g_warning ("Given children don't have a common parent\n");
          return;
        }
      btk_tree_store_get_iter (BTK_TREE_MODEL (tree_store), &parent,
                               path_a);
      parent_node = G_NODE (parent.user_data);
    }
  btk_tree_path_free (path_b);

  /* old links which we have to keep around */
  a_prev = node_a->prev;
  a_next = node_a->next;

  b_prev = node_b->prev;
  b_next = node_b->next;

  /* fix up links if the nodes are next to eachother */
  if (a_prev == node_b)
    a_prev = node_a;
  if (a_next == node_b)
    a_next = node_a;

  if (b_prev == node_a)
    b_prev = node_b;
  if (b_next == node_a)
    b_next = node_b;

  /* counting nodes */
  tmp = parent_node->children;
  i = a_count = b_count = 0;
  while (tmp)
    {
      if (tmp == node_a)
	a_count = i;
      if (tmp == node_b)
	b_count = i;

      tmp = tmp->next;
      i++;
    }
  length = i;

  /* hacking the tree */
  if (!a_prev)
    parent_node->children = node_b;
  else
    a_prev->next = node_b;

  if (a_next)
    a_next->prev = node_b;

  if (!b_prev)
    parent_node->children = node_a;
  else
    b_prev->next = node_a;

  if (b_next)
    b_next->prev = node_a;

  node_a->prev = b_prev;
  node_a->next = b_next;

  node_b->prev = a_prev;
  node_b->next = a_next;

  /* emit signal */
  order = g_new (gint, length);
  for (i = 0; i < length; i++)
    if (i == a_count)
      order[i] = b_count;
    else if (i == b_count)
      order[i] = a_count;
    else
      order[i] = i;

  btk_tree_model_rows_reordered (BTK_TREE_MODEL (tree_store), path_a,
				 parent_node == tree_store->root 
				 ? NULL : &parent, order);
  btk_tree_path_free (path_a);
  g_free (order);
}

/* WARNING: this function is *incredibly* fragile. Please smashtest after
 * making changes here.
 *	-Kris
 */
static void
btk_tree_store_move (BtkTreeStore *tree_store,
                     BtkTreeIter  *iter,
		     BtkTreeIter  *position,
		     gboolean      before)
{
  GNode *parent, *node, *a, *b, *tmp, *tmp_a, *tmp_b;
  gint old_pos, new_pos, length, i, *order;
  BtkTreePath *path = NULL, *tmppath, *pos_path = NULL;
  BtkTreeIter parent_iter, dst_a, dst_b;
  gint depth = 0;
  gboolean handle_b = TRUE;

  g_return_if_fail (BTK_IS_TREE_STORE (tree_store));
  g_return_if_fail (!BTK_TREE_STORE_IS_SORTED (tree_store));
  g_return_if_fail (VALID_ITER (iter, tree_store));
  if (position)
    g_return_if_fail (VALID_ITER (position, tree_store));

  a = b = NULL;

  /* sanity checks */
  if (position)
    {
      path = btk_tree_store_get_path (BTK_TREE_MODEL (tree_store), iter);
      pos_path = btk_tree_store_get_path (BTK_TREE_MODEL (tree_store),
	                                  position);

      /* if before:
       *   moving the iter before path or "path + 1" doesn't make sense
       * else
       *   moving the iter before path or "path - 1" doesn't make sense
       */
      if (!btk_tree_path_compare (path, pos_path))
	goto free_paths_and_out;

      if (before)
        btk_tree_path_next (path);
      else
        btk_tree_path_prev (path);

      if (!btk_tree_path_compare (path, pos_path))
	goto free_paths_and_out;

      if (before)
        btk_tree_path_prev (path);
      else
        btk_tree_path_next (path);

      if (btk_tree_path_get_depth (path) != btk_tree_path_get_depth (pos_path))
        {
          g_warning ("Given children are not in the same level\n");

	  goto free_paths_and_out;
        }

      tmppath = btk_tree_path_copy (pos_path);
      btk_tree_path_up (path);
      btk_tree_path_up (tmppath);

      if (btk_tree_path_get_depth (path) > 0 &&
	  btk_tree_path_compare (path, tmppath))
        {
          g_warning ("Given children are not in the same level\n");

          btk_tree_path_free (tmppath);
	  goto free_paths_and_out;
        }

      btk_tree_path_free (tmppath);
    }

  if (!path)
    {
      path = btk_tree_store_get_path (BTK_TREE_MODEL (tree_store), iter);
      btk_tree_path_up (path);
    }

  depth = btk_tree_path_get_depth (path);

  if (depth)
    {
      btk_tree_store_get_iter (BTK_TREE_MODEL (tree_store), 
			       &parent_iter, path);

      parent = G_NODE (parent_iter.user_data);
    }
  else
    parent = G_NODE (tree_store->root);

  /* yes, I know that this can be done shorter, but I'm doing it this way
   * so the code is also maintainable
   */

  if (before && position)
    {
      b = G_NODE (position->user_data);

      if (btk_tree_path_get_indices (pos_path)[btk_tree_path_get_depth (pos_path) - 1] > 0)
        {
          btk_tree_path_prev (pos_path);
          if (btk_tree_store_get_iter (BTK_TREE_MODEL (tree_store), 
				       &dst_a, pos_path))
            a = G_NODE (dst_a.user_data);
          else
            a = NULL;
          btk_tree_path_next (pos_path);
	}

      /* if b is NULL, a is NULL too -- we are at the beginning of the list
       * yes and we leak memory here ...
       */
      g_return_if_fail (b);
    }
  else if (before && !position)
    {
      /* move before without position is appending */
      a = NULL;
      b = NULL;
    }
  else /* !before */
    {
      if (position)
        a = G_NODE (position->user_data);
      else
        a = NULL;

      if (position)
        {
          btk_tree_path_next (pos_path);
          if (btk_tree_store_get_iter (BTK_TREE_MODEL (tree_store), &dst_b, pos_path))
             b = G_NODE (dst_b.user_data);
          else
             b = NULL;
          btk_tree_path_prev (pos_path);
	}
      else
        {
	  /* move after without position is prepending */
	  if (depth)
	    btk_tree_store_iter_children (BTK_TREE_MODEL (tree_store), &dst_b,
	                                  &parent_iter);
	  else
	    btk_tree_store_iter_children (BTK_TREE_MODEL (tree_store), &dst_b,
		                          NULL);

	  b = G_NODE (dst_b.user_data);
	}

      /* if a is NULL, b is NULL too -- we are at the end of the list
       * yes and we leak memory here ...
       */
      if (position)
        g_return_if_fail (a);
    }

  /* counting nodes */
  tmp = parent->children;

  length = old_pos = 0;
  while (tmp)
    {
      if (tmp == iter->user_data)
	old_pos = length;

      tmp = tmp->next;
      length++;
    }

  /* remove node from list */
  node = G_NODE (iter->user_data);
  tmp_a = node->prev;
  tmp_b = node->next;

  if (tmp_a)
    tmp_a->next = tmp_b;
  else
    parent->children = tmp_b;

  if (tmp_b)
    tmp_b->prev = tmp_a;

  /* and reinsert the node */
  if (a)
    {
      tmp = a->next;

      a->next = node;
      node->next = tmp;
      node->prev = a;
    }
  else if (!a && !before)
    {
      tmp = parent->children;

      node->prev = NULL;
      parent->children = node;

      node->next = tmp;
      if (tmp) 
	tmp->prev = node;

      handle_b = FALSE;
    }
  else if (!a && before)
    {
      if (!position)
        {
          node->parent = NULL;
          node->next = node->prev = NULL;

          /* before with sibling = NULL appends */
          g_node_insert_before (parent, NULL, node);
	}
      else
        {
	  node->parent = NULL;
	  node->next = node->prev = NULL;

	  /* after with sibling = NULL prepends */
	  g_node_insert_after (parent, NULL, node);
	}

      handle_b = FALSE;
    }

  if (handle_b)
    {
      if (b)
        {
          tmp = b->prev;

          b->prev = node;
          node->prev = tmp;
          node->next = b;
        }
      else if (!(!a && before)) /* !a && before is completely handled above */
        node->next = NULL;
    }

  /* emit signal */
  if (position)
    new_pos = btk_tree_path_get_indices (pos_path)[btk_tree_path_get_depth (pos_path)-1];
  else if (before)
    {
      if (depth)
        new_pos = btk_tree_store_iter_n_children (BTK_TREE_MODEL (tree_store),
	                                          &parent_iter) - 1;
      else
	new_pos = btk_tree_store_iter_n_children (BTK_TREE_MODEL (tree_store),
	                                          NULL) - 1;
    }
  else
    new_pos = 0;

  if (new_pos > old_pos)
    {
      if (before && position)
        new_pos--;
    }
  else
    {
      if (!before && position)
        new_pos++;
    }

  order = g_new (gint, length);
  if (new_pos > old_pos)
    {
      for (i = 0; i < length; i++)
        if (i < old_pos)
          order[i] = i;
        else if (i >= old_pos && i < new_pos)
          order[i] = i + 1;
        else if (i == new_pos)
          order[i] = old_pos;
        else
	  order[i] = i;
    }
  else
    {
      for (i = 0; i < length; i++)
        if (i == new_pos)
	  order[i] = old_pos;
        else if (i > new_pos && i <= old_pos)
	  order[i] = i - 1;
	else
	  order[i] = i;
    }

  if (depth)
    {
      tmppath = btk_tree_store_get_path (BTK_TREE_MODEL (tree_store), 
					 &parent_iter);
      btk_tree_model_rows_reordered (BTK_TREE_MODEL (tree_store),
				     tmppath, &parent_iter, order);
    }
  else
    {
      tmppath = btk_tree_path_new ();
      btk_tree_model_rows_reordered (BTK_TREE_MODEL (tree_store),
				     tmppath, NULL, order);
    }

  btk_tree_path_free (tmppath);
  btk_tree_path_free (path);
  if (position)
    btk_tree_path_free (pos_path);
  g_free (order);

  return;

free_paths_and_out:
  btk_tree_path_free (path);
  btk_tree_path_free (pos_path);
}

/**
 * btk_tree_store_move_before:
 * @tree_store: A #BtkTreeStore.
 * @iter: A #BtkTreeIter.
 * @position: (allow-none): A #BtkTreeIter or %NULL.
 *
 * Moves @iter in @tree_store to the position before @position. @iter and
 * @position should be in the same level. Note that this function only
 * works with unsorted stores. If @position is %NULL, @iter will be
 * moved to the end of the level.
 *
 * Since: 2.2
 **/
void
btk_tree_store_move_before (BtkTreeStore *tree_store,
                            BtkTreeIter  *iter,
			    BtkTreeIter  *position)
{
  btk_tree_store_move (tree_store, iter, position, TRUE);
}

/**
 * btk_tree_store_move_after:
 * @tree_store: A #BtkTreeStore.
 * @iter: A #BtkTreeIter.
 * @position: (allow-none): A #BtkTreeIter.
 *
 * Moves @iter in @tree_store to the position after @position. @iter and
 * @position should be in the same level. Note that this function only
 * works with unsorted stores. If @position is %NULL, @iter will be moved
 * to the start of the level.
 *
 * Since: 2.2
 **/
void
btk_tree_store_move_after (BtkTreeStore *tree_store,
                           BtkTreeIter  *iter,
			   BtkTreeIter  *position)
{
  btk_tree_store_move (tree_store, iter, position, FALSE);
}

/* Sorting */
static gint
btk_tree_store_compare_func (gconstpointer a,
			     gconstpointer b,
			     gpointer      user_data)
{
  BtkTreeStore *tree_store = user_data;
  GNode *node_a;
  GNode *node_b;
  BtkTreeIterCompareFunc func;
  gpointer data;

  BtkTreeIter iter_a;
  BtkTreeIter iter_b;
  gint retval;

  if (tree_store->sort_column_id != -1)
    {
      BtkTreeDataSortHeader *header;

      header = _btk_tree_data_list_get_header (tree_store->sort_list,
					       tree_store->sort_column_id);
      g_return_val_if_fail (header != NULL, 0);
      g_return_val_if_fail (header->func != NULL, 0);

      func = header->func;
      data = header->data;
    }
  else
    {
      g_return_val_if_fail (tree_store->default_sort_func != NULL, 0);
      func = tree_store->default_sort_func;
      data = tree_store->default_sort_data;
    }

  node_a = ((SortTuple *) a)->node;
  node_b = ((SortTuple *) b)->node;

  iter_a.stamp = tree_store->stamp;
  iter_a.user_data = node_a;
  iter_b.stamp = tree_store->stamp;
  iter_b.user_data = node_b;

  retval = (* func) (BTK_TREE_MODEL (user_data), &iter_a, &iter_b, data);

  if (tree_store->order == BTK_SORT_DESCENDING)
    {
      if (retval > 0)
	retval = -1;
      else if (retval < 0)
	retval = 1;
    }
  return retval;
}

static void
btk_tree_store_sort_helper (BtkTreeStore *tree_store,
			    GNode        *parent,
			    gboolean      recurse)
{
  BtkTreeIter iter;
  GArray *sort_array;
  GNode *node;
  GNode *tmp_node;
  gint list_length;
  gint i;
  gint *new_order;
  BtkTreePath *path;

  node = parent->children;
  if (node == NULL || node->next == NULL)
    {
      if (recurse && node && node->children)
        btk_tree_store_sort_helper (tree_store, node, TRUE);

      return;
    }

  list_length = 0;
  for (tmp_node = node; tmp_node; tmp_node = tmp_node->next)
    list_length++;

  sort_array = g_array_sized_new (FALSE, FALSE, sizeof (SortTuple), list_length);

  i = 0;
  for (tmp_node = node; tmp_node; tmp_node = tmp_node->next)
    {
      SortTuple tuple;

      tuple.offset = i;
      tuple.node = tmp_node;
      g_array_append_val (sort_array, tuple);
      i++;
    }

  /* Sort the array */
  g_array_sort_with_data (sort_array, btk_tree_store_compare_func, tree_store);

  for (i = 0; i < list_length - 1; i++)
    {
      g_array_index (sort_array, SortTuple, i).node->next =
	g_array_index (sort_array, SortTuple, i + 1).node;
      g_array_index (sort_array, SortTuple, i + 1).node->prev =
	g_array_index (sort_array, SortTuple, i).node;
    }
  g_array_index (sort_array, SortTuple, list_length - 1).node->next = NULL;
  g_array_index (sort_array, SortTuple, 0).node->prev = NULL;
  parent->children = g_array_index (sort_array, SortTuple, 0).node;

  /* Let the world know about our new order */
  new_order = g_new (gint, list_length);
  for (i = 0; i < list_length; i++)
    new_order[i] = g_array_index (sort_array, SortTuple, i).offset;

  iter.stamp = tree_store->stamp;
  iter.user_data = parent;
  path = btk_tree_store_get_path (BTK_TREE_MODEL (tree_store), &iter);
  btk_tree_model_rows_reordered (BTK_TREE_MODEL (tree_store),
				 path, &iter, new_order);
  btk_tree_path_free (path);
  g_free (new_order);
  g_array_free (sort_array, TRUE);

  if (recurse)
    {
      for (tmp_node = parent->children; tmp_node; tmp_node = tmp_node->next)
	{
	  if (tmp_node->children)
	    btk_tree_store_sort_helper (tree_store, tmp_node, TRUE);
	}
    }
}

static void
btk_tree_store_sort (BtkTreeStore *tree_store)
{
  if (!BTK_TREE_STORE_IS_SORTED (tree_store))
    return;

  if (tree_store->sort_column_id != -1)
    {
      BtkTreeDataSortHeader *header = NULL;

      header = _btk_tree_data_list_get_header (tree_store->sort_list, 
					       tree_store->sort_column_id);

      /* We want to make sure that we have a function */
      g_return_if_fail (header != NULL);
      g_return_if_fail (header->func != NULL);
    }
  else
    {
      g_return_if_fail (tree_store->default_sort_func != NULL);
    }

  btk_tree_store_sort_helper (tree_store, G_NODE (tree_store->root), TRUE);
}

static void
btk_tree_store_sort_iter_changed (BtkTreeStore *tree_store,
				  BtkTreeIter  *iter,
				  gint          column,
				  gboolean      emit_signal)
{
  GNode *prev = NULL;
  GNode *next = NULL;
  GNode *node;
  BtkTreePath *tmp_path;
  BtkTreeIter tmp_iter;
  gint cmp_a = 0;
  gint cmp_b = 0;
  gint i;
  gint old_location;
  gint new_location;
  gint *new_order;
  gint length;
  BtkTreeIterCompareFunc func;
  gpointer data;

  g_return_if_fail (G_NODE (iter->user_data)->parent != NULL);

  tmp_iter.stamp = tree_store->stamp;
  if (tree_store->sort_column_id != -1)
    {
      BtkTreeDataSortHeader *header;
      header = _btk_tree_data_list_get_header (tree_store->sort_list,
					       tree_store->sort_column_id);
      g_return_if_fail (header != NULL);
      g_return_if_fail (header->func != NULL);
      func = header->func;
      data = header->data;
    }
  else
    {
      g_return_if_fail (tree_store->default_sort_func != NULL);
      func = tree_store->default_sort_func;
      data = tree_store->default_sort_data;
    }

  /* If it's the built in function, we don't sort. */
  if (func == _btk_tree_data_list_compare_func &&
      tree_store->sort_column_id != column)
    return;

  old_location = 0;
  node = G_NODE (iter->user_data)->parent->children;
  /* First we find the iter, its prev, and its next */
  while (node)
    {
      if (node == G_NODE (iter->user_data))
	break;
      old_location++;
      node = node->next;
    }
  g_assert (node != NULL);

  prev = node->prev;
  next = node->next;

  /* Check the common case, where we don't need to sort it moved. */
  if (prev != NULL)
    {
      tmp_iter.user_data = prev;
      cmp_a = (* func) (BTK_TREE_MODEL (tree_store), &tmp_iter, iter, data);
    }

  if (next != NULL)
    {
      tmp_iter.user_data = next;
      cmp_b = (* func) (BTK_TREE_MODEL (tree_store), iter, &tmp_iter, data);
    }

  if (tree_store->order == BTK_SORT_DESCENDING)
    {
      if (cmp_a < 0)
	cmp_a = 1;
      else if (cmp_a > 0)
	cmp_a = -1;

      if (cmp_b < 0)
	cmp_b = 1;
      else if (cmp_b > 0)
	cmp_b = -1;
    }

  if (prev == NULL && cmp_b <= 0)
    return;
  else if (next == NULL && cmp_a <= 0)
    return;
  else if (prev != NULL && next != NULL &&
	   cmp_a <= 0 && cmp_b <= 0)
    return;

  /* We actually need to sort it */
  /* First, remove the old link. */

  if (prev)
    prev->next = next;
  else
    node->parent->children = next;

  if (next)
    next->prev = prev;

  node->prev = NULL;
  node->next = NULL;

  /* FIXME: as an optimization, we can potentially start at next */
  prev = NULL;
  node = node->parent->children;
  new_location = 0;
  tmp_iter.user_data = node;
  if (tree_store->order == BTK_SORT_DESCENDING)
    cmp_a = (* func) (BTK_TREE_MODEL (tree_store), &tmp_iter, iter, data);
  else
    cmp_a = (* func) (BTK_TREE_MODEL (tree_store), iter, &tmp_iter, data);

  while ((node->next) && (cmp_a > 0))
    {
      prev = node;
      node = node->next;
      new_location++;
      tmp_iter.user_data = node;
      if (tree_store->order == BTK_SORT_DESCENDING)
	cmp_a = (* func) (BTK_TREE_MODEL (tree_store), &tmp_iter, iter, data);
      else
	cmp_a = (* func) (BTK_TREE_MODEL (tree_store), iter, &tmp_iter, data);
    }

  if ((!node->next) && (cmp_a > 0))
    {
      new_location++;
      node->next = G_NODE (iter->user_data);
      node->next->prev = node;
    }
  else if (prev)
    {
      prev->next = G_NODE (iter->user_data);
      prev->next->prev = prev;
      G_NODE (iter->user_data)->next = node;
      G_NODE (iter->user_data)->next->prev = G_NODE (iter->user_data);
    }
  else
    {
      G_NODE (iter->user_data)->next = G_NODE (iter->user_data)->parent->children;
      G_NODE (iter->user_data)->next->prev = G_NODE (iter->user_data);
      G_NODE (iter->user_data)->parent->children = G_NODE (iter->user_data);
    }

  if (!emit_signal)
    return;

  /* Emit the reordered signal. */
  length = g_node_n_children (node->parent);
  new_order = g_new (int, length);
  if (old_location < new_location)
    for (i = 0; i < length; i++)
      {
	if (i < old_location ||
	    i > new_location)
	  new_order[i] = i;
	else if (i >= old_location &&
		 i < new_location)
	  new_order[i] = i + 1;
	else if (i == new_location)
	  new_order[i] = old_location;
      }
  else
    for (i = 0; i < length; i++)
      {
	if (i < new_location ||
	    i > old_location)
	  new_order[i] = i;
	else if (i > new_location &&
		 i <= old_location)
	  new_order[i] = i - 1;
	else if (i == new_location)
	  new_order[i] = old_location;
      }

  tmp_iter.user_data = node->parent;
  tmp_path = btk_tree_store_get_path (BTK_TREE_MODEL (tree_store), &tmp_iter);

  btk_tree_model_rows_reordered (BTK_TREE_MODEL (tree_store),
				 tmp_path, &tmp_iter,
				 new_order);

  btk_tree_path_free (tmp_path);
  g_free (new_order);
}


static gboolean
btk_tree_store_get_sort_column_id (BtkTreeSortable  *sortable,
				   gint             *sort_column_id,
				   BtkSortType      *order)
{
  BtkTreeStore *tree_store = (BtkTreeStore *) sortable;

  if (sort_column_id)
    * sort_column_id = tree_store->sort_column_id;
  if (order)
    * order = tree_store->order;

  if (tree_store->sort_column_id == BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID ||
      tree_store->sort_column_id == BTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID)
    return FALSE;

  return TRUE;
}

static void
btk_tree_store_set_sort_column_id (BtkTreeSortable  *sortable,
				   gint              sort_column_id,
				   BtkSortType       order)
{
  BtkTreeStore *tree_store = (BtkTreeStore *) sortable;

  
  if ((tree_store->sort_column_id == sort_column_id) &&
      (tree_store->order == order))
    return;

  if (sort_column_id != BTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID)
    {
      if (sort_column_id != BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID)
	{
	  BtkTreeDataSortHeader *header = NULL;

	  header = _btk_tree_data_list_get_header (tree_store->sort_list, 
						   sort_column_id);

	  /* We want to make sure that we have a function */
	  g_return_if_fail (header != NULL);
	  g_return_if_fail (header->func != NULL);
	}
      else
	{
	  g_return_if_fail (tree_store->default_sort_func != NULL);
	}
    }

  tree_store->sort_column_id = sort_column_id;
  tree_store->order = order;

  btk_tree_sortable_sort_column_changed (sortable);

  btk_tree_store_sort (tree_store);
}

static void
btk_tree_store_set_sort_func (BtkTreeSortable        *sortable,
			      gint                    sort_column_id,
			      BtkTreeIterCompareFunc  func,
			      gpointer                data,
			      GDestroyNotify          destroy)
{
  BtkTreeStore *tree_store = (BtkTreeStore *) sortable;

  tree_store->sort_list = _btk_tree_data_list_set_header (tree_store->sort_list, 
							  sort_column_id, 
							  func, data, destroy);

  if (tree_store->sort_column_id == sort_column_id)
    btk_tree_store_sort (tree_store);
}

static void
btk_tree_store_set_default_sort_func (BtkTreeSortable        *sortable,
				      BtkTreeIterCompareFunc  func,
				      gpointer                data,
				      GDestroyNotify          destroy)
{
  BtkTreeStore *tree_store = (BtkTreeStore *) sortable;

  if (tree_store->default_sort_destroy)
    {
      GDestroyNotify d = tree_store->default_sort_destroy;

      tree_store->default_sort_destroy = NULL;
      d (tree_store->default_sort_data);
    }

  tree_store->default_sort_func = func;
  tree_store->default_sort_data = data;
  tree_store->default_sort_destroy = destroy;

  if (tree_store->sort_column_id == BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID)
    btk_tree_store_sort (tree_store);
}

static gboolean
btk_tree_store_has_default_sort_func (BtkTreeSortable *sortable)
{
  BtkTreeStore *tree_store = (BtkTreeStore *) sortable;

  return (tree_store->default_sort_func != NULL);
}

static void
validate_gnode (GNode* node)
{
  GNode *iter;

  iter = node->children;
  while (iter != NULL)
    {
      g_assert (iter->parent == node);
      if (iter->prev)
        g_assert (iter->prev->next == iter);
      validate_gnode (iter);
      iter = iter->next;
    }
}

/* BtkBuildable custom tag implementation
 *
 * <columns>
 *   <column type="..."/>
 *   <column type="..."/>
 * </columns>
 */
typedef struct {
  BtkBuilder *builder;
  GObject *object;
  GSList *items;
} GSListSubParserData;

static void
tree_model_start_element (GMarkupParseContext *context,
			  const gchar         *element_name,
			  const gchar        **names,
			  const gchar        **values,
			  gpointer            user_data,
			  GError            **error)
{
  guint i;
  GSListSubParserData *data = (GSListSubParserData*)user_data;

  for (i = 0; names[i]; i++)
    {
      if (strcmp (names[i], "type") == 0)
	data->items = g_slist_prepend (data->items, g_strdup (values[i]));
    }
}

static void
tree_model_end_element (GMarkupParseContext *context,
			const gchar         *element_name,
			gpointer             user_data,
			GError             **error)
{
  GSListSubParserData *data = (GSListSubParserData*)user_data;

  g_assert(data->builder);

  if (strcmp (element_name, "columns") == 0)
    {
      GSList *l;
      GType *types;
      int i;
      GType type;

      data = (GSListSubParserData*)user_data;
      data->items = g_slist_reverse (data->items);
      types = g_new0 (GType, g_slist_length (data->items));

      for (l = data->items, i = 0; l; l = l->next, i++)
        {
          type = btk_builder_get_type_from_name (data->builder, l->data);
          if (type == G_TYPE_INVALID)
            {
              g_warning ("Unknown type %s specified in treemodel %s",
                         (const gchar*)l->data,
                         btk_buildable_get_name (BTK_BUILDABLE (data->object)));
              continue;
            }
          types[i] = type;

          g_free (l->data);
        }

      btk_tree_store_set_column_types (BTK_TREE_STORE (data->object), i, types);

      g_free (types);
    }
}

static const GMarkupParser tree_model_parser =
  {
    tree_model_start_element,
    tree_model_end_element
  };


static gboolean
btk_tree_store_buildable_custom_tag_start (BtkBuildable  *buildable,
					   BtkBuilder    *builder,
					   GObject       *child,
					   const gchar   *tagname,
					   GMarkupParser *parser,
					   gpointer      *data)
{
  GSListSubParserData *parser_data;

  if (child)
    return FALSE;

  if (strcmp (tagname, "columns") == 0)
    {
      parser_data = g_slice_new0 (GSListSubParserData);
      parser_data->builder = builder;
      parser_data->items = NULL;
      parser_data->object = G_OBJECT (buildable);

      *parser = tree_model_parser;
      *data = parser_data;
      return TRUE;
    }

  return FALSE;
}

static void
btk_tree_store_buildable_custom_finished (BtkBuildable *buildable,
					  BtkBuilder   *builder,
					  GObject      *child,
					  const gchar  *tagname,
					  gpointer      user_data)
{
  GSListSubParserData *data;

  if (strcmp (tagname, "columns"))
    return;

  data = (GSListSubParserData*)user_data;

  g_slist_free (data->items);
  g_slice_free (GSListSubParserData, data);
}

#define __BTK_TREE_STORE_C__
#include "btkaliasdef.c"
