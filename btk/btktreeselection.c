/* btktreeselection.h
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
#include "btktreeselection.h"
#include "btktreeprivate.h"
#include "btkrbtree.h"
#include "btkmarshalers.h"
#include "btkintl.h"
#include "btkalias.h"

static void btk_tree_selection_finalize          (BObject               *object);
static gint btk_tree_selection_real_select_all   (BtkTreeSelection      *selection);
static gint btk_tree_selection_real_unselect_all (BtkTreeSelection      *selection);
static gint btk_tree_selection_real_select_node  (BtkTreeSelection      *selection,
						  BtkRBTree             *tree,
						  BtkRBNode             *node,
						  gboolean               select);

enum
{
  CHANGED,
  LAST_SIGNAL
};

static guint tree_selection_signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BtkTreeSelection, btk_tree_selection, B_TYPE_OBJECT)

static void
btk_tree_selection_class_init (BtkTreeSelectionClass *class)
{
  BObjectClass *object_class;

  object_class = (BObjectClass*) class;

  object_class->finalize = btk_tree_selection_finalize;
  class->changed = NULL;

  tree_selection_signals[CHANGED] =
    g_signal_new (I_("changed"),
		  B_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkTreeSelectionClass, changed),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
}

static void
btk_tree_selection_init (BtkTreeSelection *selection)
{
  selection->type = BTK_SELECTION_SINGLE;
}

static void
btk_tree_selection_finalize (BObject *object)
{
  BtkTreeSelection *selection = BTK_TREE_SELECTION (object);

  if (selection->destroy)
    {
      GDestroyNotify d = selection->destroy;

      selection->destroy = NULL;
      d (selection->user_data);
    }

  /* chain parent_class' handler */
  B_OBJECT_CLASS (btk_tree_selection_parent_class)->finalize (object);
}

/**
 * _btk_tree_selection_new:
 *
 * Creates a new #BtkTreeSelection object.  This function should not be invoked,
 * as each #BtkTreeView will create its own #BtkTreeSelection.
 *
 * Return value: A newly created #BtkTreeSelection object.
 **/
BtkTreeSelection*
_btk_tree_selection_new (void)
{
  BtkTreeSelection *selection;

  selection = g_object_new (BTK_TYPE_TREE_SELECTION, NULL);

  return selection;
}

/**
 * _btk_tree_selection_new_with_tree_view:
 * @tree_view: The #BtkTreeView.
 *
 * Creates a new #BtkTreeSelection object.  This function should not be invoked,
 * as each #BtkTreeView will create its own #BtkTreeSelection.
 *
 * Return value: A newly created #BtkTreeSelection object.
 **/
BtkTreeSelection*
_btk_tree_selection_new_with_tree_view (BtkTreeView *tree_view)
{
  BtkTreeSelection *selection;

  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), NULL);

  selection = _btk_tree_selection_new ();
  _btk_tree_selection_set_tree_view (selection, tree_view);

  return selection;
}

/**
 * _btk_tree_selection_set_tree_view:
 * @selection: A #BtkTreeSelection.
 * @tree_view: The #BtkTreeView.
 *
 * Sets the #BtkTreeView of @selection.  This function should not be invoked, as
 * it is used internally by #BtkTreeView.
 **/
void
_btk_tree_selection_set_tree_view (BtkTreeSelection *selection,
                                   BtkTreeView      *tree_view)
{
  g_return_if_fail (BTK_IS_TREE_SELECTION (selection));
  if (tree_view != NULL)
    g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  selection->tree_view = tree_view;
}

/**
 * btk_tree_selection_set_mode:
 * @selection: A #BtkTreeSelection.
 * @type: The selection mode
 *
 * Sets the selection mode of the @selection.  If the previous type was
 * #BTK_SELECTION_MULTIPLE, then the anchor is kept selected, if it was
 * previously selected.
 **/
void
btk_tree_selection_set_mode (BtkTreeSelection *selection,
			     BtkSelectionMode  type)
{
  BtkTreeSelectionFunc tmp_func;
  g_return_if_fail (BTK_IS_TREE_SELECTION (selection));

  if (selection->type == type)
    return;

  
  if (type == BTK_SELECTION_NONE)
    {
      /* We do this so that we unconditionally unset all rows
       */
      tmp_func = selection->user_func;
      selection->user_func = NULL;
      btk_tree_selection_unselect_all (selection);
      selection->user_func = tmp_func;

      btk_tree_row_reference_free (selection->tree_view->priv->anchor);
      selection->tree_view->priv->anchor = NULL;
    }
  else if (type == BTK_SELECTION_SINGLE ||
	   type == BTK_SELECTION_BROWSE)
    {
      BtkRBTree *tree = NULL;
      BtkRBNode *node = NULL;
      gint selected = FALSE;
      BtkTreePath *anchor_path = NULL;

      if (selection->tree_view->priv->anchor)
	{
          anchor_path = btk_tree_row_reference_get_path (selection->tree_view->priv->anchor);

          if (anchor_path)
            {
              _btk_tree_view_find_node (selection->tree_view,
                                        anchor_path,
                                        &tree,
                                        &node);

              if (node && BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SELECTED))
                selected = TRUE;
            }
	}

      /* We do this so that we unconditionally unset all rows
       */
      tmp_func = selection->user_func;
      selection->user_func = NULL;
      btk_tree_selection_unselect_all (selection);
      selection->user_func = tmp_func;

      if (node && selected)
	_btk_tree_selection_internal_select_node (selection,
						  node,
						  tree,
						  anchor_path,
                                                  0,
						  FALSE);
      if (anchor_path)
	btk_tree_path_free (anchor_path);
    }

  selection->type = type;
}

/**
 * btk_tree_selection_get_mode:
 * @selection: a #BtkTreeSelection
 *
 * Gets the selection mode for @selection. See
 * btk_tree_selection_set_mode().
 *
 * Return value: the current selection mode
 **/
BtkSelectionMode
btk_tree_selection_get_mode (BtkTreeSelection *selection)
{
  g_return_val_if_fail (BTK_IS_TREE_SELECTION (selection), BTK_SELECTION_SINGLE);

  return selection->type;
}

/**
 * btk_tree_selection_set_select_function:
 * @selection: A #BtkTreeSelection.
 * @func: The selection function.
 * @data: The selection function's data.
 * @destroy: The destroy function for user data.  May be NULL.
 *
 * Sets the selection function.  If set, this function is called before any node
 * is selected or unselected, giving some control over which nodes are selected.
 * The select function should return %TRUE if the state of the node may be toggled,
 * and %FALSE if the state of the node should be left unchanged.
 **/
void
btk_tree_selection_set_select_function (BtkTreeSelection     *selection,
					BtkTreeSelectionFunc  func,
					gpointer              data,
					GDestroyNotify        destroy)
{
  g_return_if_fail (BTK_IS_TREE_SELECTION (selection));
  g_return_if_fail (func != NULL);

  if (selection->destroy)
    {
      GDestroyNotify d = selection->destroy;

      selection->destroy = NULL;
      d (selection->user_data);
    }

  selection->user_func = func;
  selection->user_data = data;
  selection->destroy = destroy;
}

/**
 * btk_tree_selection_get_select_function: (skip)
 * @selection: A #BtkTreeSelection.
 *
 * Returns the current selection function.
 *
 * Return value: The function.
 *
 * Since: 2.14
 **/
BtkTreeSelectionFunc
btk_tree_selection_get_select_function (BtkTreeSelection *selection)
{
  g_return_val_if_fail (BTK_IS_TREE_SELECTION (selection), NULL);

  return selection->user_func;
}

/**
 * btk_tree_selection_get_user_data: (skip)
 * @selection: A #BtkTreeSelection.
 *
 * Returns the user data for the selection function.
 *
 * Return value: The user data.
 **/
gpointer
btk_tree_selection_get_user_data (BtkTreeSelection *selection)
{
  g_return_val_if_fail (BTK_IS_TREE_SELECTION (selection), NULL);

  return selection->user_data;
}

/**
 * btk_tree_selection_get_tree_view:
 * @selection: A #BtkTreeSelection
 * 
 * Returns the tree view associated with @selection.
 * 
 * Return value: (transfer none): A #BtkTreeView
 **/
BtkTreeView *
btk_tree_selection_get_tree_view (BtkTreeSelection *selection)
{
  g_return_val_if_fail (BTK_IS_TREE_SELECTION (selection), NULL);

  return selection->tree_view;
}

/**
 * btk_tree_selection_get_selected:
 * @selection: A #BtkTreeSelection.
 * @model: (out) (allow-none) (transfer none): A pointer to set to the #BtkTreeModel, or NULL.
 * @iter: (out) (allow-none): The #BtkTreeIter, or NULL.
 *
 * Sets @iter to the currently selected node if @selection is set to
 * #BTK_SELECTION_SINGLE or #BTK_SELECTION_BROWSE.  @iter may be NULL if you
 * just want to test if @selection has any selected nodes.  @model is filled
 * with the current model as a convenience.  This function will not work if you
 * use @selection is #BTK_SELECTION_MULTIPLE.
 *
 * Return value: TRUE, if there is a selected node.
 **/
gboolean
btk_tree_selection_get_selected (BtkTreeSelection  *selection,
				 BtkTreeModel     **model,
				 BtkTreeIter       *iter)
{
  BtkRBTree *tree;
  BtkRBNode *node;
  BtkTreePath *anchor_path;
  gboolean retval;
  gboolean found_node;

  g_return_val_if_fail (BTK_IS_TREE_SELECTION (selection), FALSE);
  g_return_val_if_fail (selection->type != BTK_SELECTION_MULTIPLE, FALSE);
  g_return_val_if_fail (selection->tree_view != NULL, FALSE);

  /* Clear the iter */
  if (iter)
    memset (iter, 0, sizeof (BtkTreeIter));

  if (model)
    *model = selection->tree_view->priv->model;

  if (selection->tree_view->priv->anchor == NULL)
    return FALSE;

  anchor_path = btk_tree_row_reference_get_path (selection->tree_view->priv->anchor);

  if (anchor_path == NULL)
    return FALSE;

  retval = FALSE;

  found_node = !_btk_tree_view_find_node (selection->tree_view,
                                          anchor_path,
                                          &tree,
                                          &node);

  if (found_node && BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SELECTED))
    {
      /* we only want to return the anchor if it exists in the rbtree and
       * is selected.
       */
      if (iter == NULL)
	retval = TRUE;
      else
        retval = btk_tree_model_get_iter (selection->tree_view->priv->model,
                                          iter,
                                          anchor_path);
    }
  else
    {
      /* We don't want to return the anchor if it isn't actually selected.
       */
      retval = FALSE;
    }

  btk_tree_path_free (anchor_path);

  return retval;
}

/**
 * btk_tree_selection_get_selected_rows:
 * @selection: A #BtkTreeSelection.
 * @model: (out) (allow-none) (transfer none): A pointer to set to the #BtkTreeModel, or %NULL.
 *
 * Creates a list of path of all selected rows. Additionally, if you are
 * planning on modifying the model after calling this function, you may
 * want to convert the returned list into a list of #BtkTreeRowReference<!-- -->s.
 * To do this, you can use btk_tree_row_reference_new().
 *
 * To free the return value, use:
 * |[
 * g_list_foreach (list, (GFunc) btk_tree_path_free, NULL);
 * g_list_free (list);
 * ]|
 *
 * Return value: (element-type BtkTreePath) (transfer full): A #GList containing a #BtkTreePath for each selected row.
 *
 * Since: 2.2
 **/
GList *
btk_tree_selection_get_selected_rows (BtkTreeSelection   *selection,
                                      BtkTreeModel      **model)
{
  GList *list = NULL;
  BtkRBTree *tree = NULL;
  BtkRBNode *node = NULL;
  BtkTreePath *path;

  g_return_val_if_fail (BTK_IS_TREE_SELECTION (selection), NULL);
  g_return_val_if_fail (selection->tree_view != NULL, NULL);

  if (model)
    *model = selection->tree_view->priv->model;

  if (selection->tree_view->priv->tree == NULL ||
      selection->tree_view->priv->tree->root == NULL)
    return NULL;

  if (selection->type == BTK_SELECTION_NONE)
    return NULL;
  else if (selection->type != BTK_SELECTION_MULTIPLE)
    {
      BtkTreeIter iter;

      if (btk_tree_selection_get_selected (selection, NULL, &iter))
        {
	  BtkTreePath *path;

	  path = btk_tree_model_get_path (selection->tree_view->priv->model, &iter);
	  list = g_list_append (list, path);

	  return list;
	}

      return NULL;
    }

  tree = selection->tree_view->priv->tree;
  node = selection->tree_view->priv->tree->root;

  while (node->left != tree->nil)
    node = node->left;
  path = btk_tree_path_new_first ();

  do
    {
      if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SELECTED))
	list = g_list_prepend (list, btk_tree_path_copy (path));

      if (node->children)
        {
	  tree = node->children;
	  node = tree->root;

	  while (node->left != tree->nil)
	    node = node->left;

	  btk_tree_path_append_index (path, 0);
	}
      else
        {
	  gboolean done = FALSE;

	  do
	    {
	      node = _btk_rbtree_next (tree, node);
	      if (node != NULL)
	        {
		  done = TRUE;
		  btk_tree_path_next (path);
		}
	      else
	        {
		  node = tree->parent_node;
		  tree = tree->parent_tree;

		  if (!tree)
		    {
		      btk_tree_path_free (path);

		      goto done; 
		    }

		  btk_tree_path_up (path);
		}
	    }
	  while (!done);
	}
    }
  while (TRUE);

  btk_tree_path_free (path);

 done:
  return g_list_reverse (list);
}

static void
btk_tree_selection_count_selected_rows_helper (BtkRBTree *tree,
					       BtkRBNode *node,
					       gpointer   data)
{
  gint *count = (gint *)data;

  if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SELECTED))
    (*count)++;

  if (node->children)
    _btk_rbtree_traverse (node->children, node->children->root,
			  G_PRE_ORDER,
			  btk_tree_selection_count_selected_rows_helper, data);
}

/**
 * btk_tree_selection_count_selected_rows:
 * @selection: A #BtkTreeSelection.
 *
 * Returns the number of rows that have been selected in @tree.
 *
 * Return value: The number of rows selected.
 * 
 * Since: 2.2
 **/
gint
btk_tree_selection_count_selected_rows (BtkTreeSelection *selection)
{
  gint count = 0;

  g_return_val_if_fail (BTK_IS_TREE_SELECTION (selection), 0);
  g_return_val_if_fail (selection->tree_view != NULL, 0);

  if (selection->tree_view->priv->tree == NULL ||
      selection->tree_view->priv->tree->root == NULL)
    return 0;

  if (selection->type == BTK_SELECTION_SINGLE ||
      selection->type == BTK_SELECTION_BROWSE)
    {
      if (btk_tree_selection_get_selected (selection, NULL, NULL))
	return 1;
      else
	return 0;
    }

  _btk_rbtree_traverse (selection->tree_view->priv->tree,
                        selection->tree_view->priv->tree->root,
			G_PRE_ORDER,
			btk_tree_selection_count_selected_rows_helper,
			&count);

  return count;
}

/* btk_tree_selection_selected_foreach helper */
static void
model_changed (gpointer data)
{
  gboolean *stop = (gboolean *)data;

  *stop = TRUE;
}

/**
 * btk_tree_selection_selected_foreach:
 * @selection: A #BtkTreeSelection.
 * @func: (scope call): The function to call for each selected node.
 * @data: user data to pass to the function.
 *
 * Calls a function for each selected node. Note that you cannot modify
 * the tree or selection from within this function. As a result,
 * btk_tree_selection_get_selected_rows() might be more useful.
 **/
void
btk_tree_selection_selected_foreach (BtkTreeSelection            *selection,
				     BtkTreeSelectionForeachFunc  func,
				     gpointer                     data)
{
  BtkTreePath *path;
  BtkRBTree *tree;
  BtkRBNode *node;
  BtkTreeIter iter;
  BtkTreeModel *model;

  gulong inserted_id, deleted_id, reordered_id, changed_id;
  gboolean stop = FALSE;

  g_return_if_fail (BTK_IS_TREE_SELECTION (selection));
  g_return_if_fail (selection->tree_view != NULL);

  if (func == NULL ||
      selection->tree_view->priv->tree == NULL ||
      selection->tree_view->priv->tree->root == NULL)
    return;

  if (selection->type == BTK_SELECTION_SINGLE ||
      selection->type == BTK_SELECTION_BROWSE)
    {
      if (btk_tree_row_reference_valid (selection->tree_view->priv->anchor))
	{
	  path = btk_tree_row_reference_get_path (selection->tree_view->priv->anchor);
	  btk_tree_model_get_iter (selection->tree_view->priv->model, &iter, path);
	  (* func) (selection->tree_view->priv->model, path, &iter, data);
	  btk_tree_path_free (path);
	}
      return;
    }

  tree = selection->tree_view->priv->tree;
  node = selection->tree_view->priv->tree->root;
  
  while (node->left != tree->nil)
    node = node->left;

  model = selection->tree_view->priv->model;
  g_object_ref (model);

  /* connect to signals to monitor changes in treemodel */
  inserted_id = g_signal_connect_swapped (model, "row-inserted",
					  G_CALLBACK (model_changed),
				          &stop);
  deleted_id = g_signal_connect_swapped (model, "row-deleted",
					 G_CALLBACK (model_changed),
				         &stop);
  reordered_id = g_signal_connect_swapped (model, "rows-reordered",
					   G_CALLBACK (model_changed),
				           &stop);
  changed_id = g_signal_connect_swapped (selection->tree_view, "notify::model",
					 G_CALLBACK (model_changed), 
					 &stop);

  /* find the node internally */
  path = btk_tree_path_new_first ();

  do
    {
      if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SELECTED))
        {
          btk_tree_model_get_iter (model, &iter, path);
	  (* func) (model, path, &iter, data);
        }

      if (stop)
	goto out;

      if (node->children)
	{
	  tree = node->children;
	  node = tree->root;

	  while (node->left != tree->nil)
	    node = node->left;

	  btk_tree_path_append_index (path, 0);
	}
      else
	{
	  gboolean done = FALSE;

	  do
	    {
	      node = _btk_rbtree_next (tree, node);
	      if (node != NULL)
		{
		  done = TRUE;
		  btk_tree_path_next (path);
		}
	      else
		{
		  node = tree->parent_node;
		  tree = tree->parent_tree;

		  if (tree == NULL)
		    {
		      /* we've run out of tree */
		      /* We're done with this function */

		      goto out;
		    }

		  btk_tree_path_up (path);
		}
	    }
	  while (!done);
	}
    }
  while (TRUE);

out:
  if (path)
    btk_tree_path_free (path);

  g_signal_handler_disconnect (model, inserted_id);
  g_signal_handler_disconnect (model, deleted_id);
  g_signal_handler_disconnect (model, reordered_id);
  g_signal_handler_disconnect (selection->tree_view, changed_id);
  g_object_unref (model);

  /* check if we have to spew a scary message */
  if (stop)
    g_warning ("The model has been modified from within btk_tree_selection_selected_foreach.\n"
	       "This function is for observing the selections of the tree only.  If\n"
	       "you are trying to get all selected items from the tree, try using\n"
	       "btk_tree_selection_get_selected_rows instead.\n");
}

/**
 * btk_tree_selection_select_path:
 * @selection: A #BtkTreeSelection.
 * @path: The #BtkTreePath to be selected.
 *
 * Select the row at @path.
 **/
void
btk_tree_selection_select_path (BtkTreeSelection *selection,
				BtkTreePath      *path)
{
  BtkRBNode *node;
  BtkRBTree *tree;
  gboolean ret;
  BtkTreeSelectMode mode = 0;

  g_return_if_fail (BTK_IS_TREE_SELECTION (selection));
  g_return_if_fail (selection->tree_view != NULL);
  g_return_if_fail (path != NULL);

  ret = _btk_tree_view_find_node (selection->tree_view,
				  path,
				  &tree,
				  &node);

  if (node == NULL || BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SELECTED) ||
      ret == TRUE)
    return;

  if (selection->type == BTK_SELECTION_MULTIPLE)
    mode = BTK_TREE_SELECT_MODE_TOGGLE;

  _btk_tree_selection_internal_select_node (selection,
					    node,
					    tree,
					    path,
                                            mode,
					    FALSE);
}

/**
 * btk_tree_selection_unselect_path:
 * @selection: A #BtkTreeSelection.
 * @path: The #BtkTreePath to be unselected.
 *
 * Unselects the row at @path.
 **/
void
btk_tree_selection_unselect_path (BtkTreeSelection *selection,
				  BtkTreePath      *path)
{
  BtkRBNode *node;
  BtkRBTree *tree;
  gboolean ret;

  g_return_if_fail (BTK_IS_TREE_SELECTION (selection));
  g_return_if_fail (selection->tree_view != NULL);
  g_return_if_fail (path != NULL);

  ret = _btk_tree_view_find_node (selection->tree_view,
				  path,
				  &tree,
				  &node);

  if (node == NULL || !BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SELECTED) ||
      ret == TRUE)
    return;

  _btk_tree_selection_internal_select_node (selection,
					    node,
					    tree,
					    path,
                                            BTK_TREE_SELECT_MODE_TOGGLE,
					    TRUE);
}

/**
 * btk_tree_selection_select_iter:
 * @selection: A #BtkTreeSelection.
 * @iter: The #BtkTreeIter to be selected.
 *
 * Selects the specified iterator.
 **/
void
btk_tree_selection_select_iter (BtkTreeSelection *selection,
				BtkTreeIter      *iter)
{
  BtkTreePath *path;

  g_return_if_fail (BTK_IS_TREE_SELECTION (selection));
  g_return_if_fail (selection->tree_view != NULL);
  g_return_if_fail (selection->tree_view->priv->model != NULL);
  g_return_if_fail (iter != NULL);

  path = btk_tree_model_get_path (selection->tree_view->priv->model,
				  iter);

  if (path == NULL)
    return;

  btk_tree_selection_select_path (selection, path);
  btk_tree_path_free (path);
}


/**
 * btk_tree_selection_unselect_iter:
 * @selection: A #BtkTreeSelection.
 * @iter: The #BtkTreeIter to be unselected.
 *
 * Unselects the specified iterator.
 **/
void
btk_tree_selection_unselect_iter (BtkTreeSelection *selection,
				  BtkTreeIter      *iter)
{
  BtkTreePath *path;

  g_return_if_fail (BTK_IS_TREE_SELECTION (selection));
  g_return_if_fail (selection->tree_view != NULL);
  g_return_if_fail (selection->tree_view->priv->model != NULL);
  g_return_if_fail (iter != NULL);

  path = btk_tree_model_get_path (selection->tree_view->priv->model,
				  iter);

  if (path == NULL)
    return;

  btk_tree_selection_unselect_path (selection, path);
  btk_tree_path_free (path);
}

/**
 * btk_tree_selection_path_is_selected:
 * @selection: A #BtkTreeSelection.
 * @path: A #BtkTreePath to check selection on.
 * 
 * Returns %TRUE if the row pointed to by @path is currently selected.  If @path
 * does not point to a valid location, %FALSE is returned
 * 
 * Return value: %TRUE if @path is selected.
 **/
gboolean
btk_tree_selection_path_is_selected (BtkTreeSelection *selection,
				     BtkTreePath      *path)
{
  BtkRBNode *node;
  BtkRBTree *tree;
  gboolean ret;

  g_return_val_if_fail (BTK_IS_TREE_SELECTION (selection), FALSE);
  g_return_val_if_fail (path != NULL, FALSE);
  g_return_val_if_fail (selection->tree_view != NULL, FALSE);

  if (selection->tree_view->priv->model == NULL)
    return FALSE;

  ret = _btk_tree_view_find_node (selection->tree_view,
				  path,
				  &tree,
				  &node);

  if ((node == NULL) || !BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SELECTED) ||
      ret == TRUE)
    return FALSE;

  return TRUE;
}

/**
 * btk_tree_selection_iter_is_selected:
 * @selection: A #BtkTreeSelection
 * @iter: A valid #BtkTreeIter
 * 
 * Returns %TRUE if the row at @iter is currently selected.
 * 
 * Return value: %TRUE, if @iter is selected
 **/
gboolean
btk_tree_selection_iter_is_selected (BtkTreeSelection *selection,
				     BtkTreeIter      *iter)
{
  BtkTreePath *path;
  gboolean retval;

  g_return_val_if_fail (BTK_IS_TREE_SELECTION (selection), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (selection->tree_view != NULL, FALSE);
  g_return_val_if_fail (selection->tree_view->priv->model != NULL, FALSE);

  path = btk_tree_model_get_path (selection->tree_view->priv->model, iter);
  if (path == NULL)
    return FALSE;

  retval = btk_tree_selection_path_is_selected (selection, path);
  btk_tree_path_free (path);

  return retval;
}


/* Wish I was in python, right now... */
struct _TempTuple {
  BtkTreeSelection *selection;
  gint dirty;
};

static void
select_all_helper (BtkRBTree  *tree,
		   BtkRBNode  *node,
		   gpointer    data)
{
  struct _TempTuple *tuple = data;

  if (node->children)
    _btk_rbtree_traverse (node->children,
			  node->children->root,
			  G_PRE_ORDER,
			  select_all_helper,
			  data);
  if (!BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SELECTED))
    {
      tuple->dirty = btk_tree_selection_real_select_node (tuple->selection, tree, node, TRUE) || tuple->dirty;
    }
}


/* We have a real_{un,}select_all function that doesn't emit the signal, so we
 * can use it in other places without fear of the signal being emitted.
 */
static gint
btk_tree_selection_real_select_all (BtkTreeSelection *selection)
{
  struct _TempTuple *tuple;

  if (selection->tree_view->priv->tree == NULL)
    return FALSE;

  /* Mark all nodes selected */
  tuple = g_new (struct _TempTuple, 1);
  tuple->selection = selection;
  tuple->dirty = FALSE;

  _btk_rbtree_traverse (selection->tree_view->priv->tree,
			selection->tree_view->priv->tree->root,
			G_PRE_ORDER,
			select_all_helper,
			tuple);
  if (tuple->dirty)
    {
      g_free (tuple);
      return TRUE;
    }
  g_free (tuple);
  return FALSE;
}

/**
 * btk_tree_selection_select_all:
 * @selection: A #BtkTreeSelection.
 *
 * Selects all the nodes. @selection must be set to #BTK_SELECTION_MULTIPLE
 * mode.
 **/
void
btk_tree_selection_select_all (BtkTreeSelection *selection)
{
  g_return_if_fail (BTK_IS_TREE_SELECTION (selection));
  g_return_if_fail (selection->tree_view != NULL);

  if (selection->tree_view->priv->tree == NULL || selection->tree_view->priv->model == NULL)
    return;

  g_return_if_fail (selection->type == BTK_SELECTION_MULTIPLE);

  if (btk_tree_selection_real_select_all (selection))
    g_signal_emit (selection, tree_selection_signals[CHANGED], 0);
}

static void
unselect_all_helper (BtkRBTree  *tree,
		     BtkRBNode  *node,
		     gpointer    data)
{
  struct _TempTuple *tuple = data;

  if (node->children)
    _btk_rbtree_traverse (node->children,
			  node->children->root,
			  G_PRE_ORDER,
			  unselect_all_helper,
			  data);
  if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SELECTED))
    {
      tuple->dirty = btk_tree_selection_real_select_node (tuple->selection, tree, node, FALSE) || tuple->dirty;
    }
}

static gboolean
btk_tree_selection_real_unselect_all (BtkTreeSelection *selection)
{
  struct _TempTuple *tuple;

  if (selection->type == BTK_SELECTION_SINGLE ||
      selection->type == BTK_SELECTION_BROWSE)
    {
      BtkRBTree *tree = NULL;
      BtkRBNode *node = NULL;
      BtkTreePath *anchor_path;

      if (selection->tree_view->priv->anchor == NULL)
	return FALSE;

      anchor_path = btk_tree_row_reference_get_path (selection->tree_view->priv->anchor);

      if (anchor_path == NULL)
        return FALSE;

      _btk_tree_view_find_node (selection->tree_view,
                                anchor_path,
				&tree,
				&node);

      btk_tree_path_free (anchor_path);

      if (tree == NULL)
        return FALSE;

      if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SELECTED))
	{
	  if (btk_tree_selection_real_select_node (selection, tree, node, FALSE))
	    {
	      btk_tree_row_reference_free (selection->tree_view->priv->anchor);
	      selection->tree_view->priv->anchor = NULL;
	      return TRUE;
	    }
	}
      return FALSE;
    }
  else
    {
      tuple = g_new (struct _TempTuple, 1);
      tuple->selection = selection;
      tuple->dirty = FALSE;

      _btk_rbtree_traverse (selection->tree_view->priv->tree,
                            selection->tree_view->priv->tree->root,
                            G_PRE_ORDER,
                            unselect_all_helper,
                            tuple);

      if (tuple->dirty)
        {
          g_free (tuple);
          return TRUE;
        }
      g_free (tuple);
      return FALSE;
    }
}

/**
 * btk_tree_selection_unselect_all:
 * @selection: A #BtkTreeSelection.
 *
 * Unselects all the nodes.
 **/
void
btk_tree_selection_unselect_all (BtkTreeSelection *selection)
{
  g_return_if_fail (BTK_IS_TREE_SELECTION (selection));
  g_return_if_fail (selection->tree_view != NULL);

  if (selection->tree_view->priv->tree == NULL || selection->tree_view->priv->model == NULL)
    return;
  
  if (btk_tree_selection_real_unselect_all (selection))
    g_signal_emit (selection, tree_selection_signals[CHANGED], 0);
}

enum
{
  RANGE_SELECT,
  RANGE_UNSELECT
};

static gint
btk_tree_selection_real_modify_range (BtkTreeSelection *selection,
                                      gint              mode,
				      BtkTreePath      *start_path,
				      BtkTreePath      *end_path)
{
  BtkRBNode *start_node, *end_node;
  BtkRBTree *start_tree, *end_tree;
  BtkTreePath *anchor_path = NULL;
  gboolean dirty = FALSE;

  switch (btk_tree_path_compare (start_path, end_path))
    {
    case 1:
      _btk_tree_view_find_node (selection->tree_view,
				end_path,
				&start_tree,
				&start_node);
      _btk_tree_view_find_node (selection->tree_view,
				start_path,
				&end_tree,
				&end_node);
      anchor_path = start_path;
      break;
    case 0:
      _btk_tree_view_find_node (selection->tree_view,
				start_path,
				&start_tree,
				&start_node);
      end_tree = start_tree;
      end_node = start_node;
      anchor_path = start_path;
      break;
    case -1:
      _btk_tree_view_find_node (selection->tree_view,
				start_path,
				&start_tree,
				&start_node);
      _btk_tree_view_find_node (selection->tree_view,
				end_path,
				&end_tree,
				&end_node);
      anchor_path = start_path;
      break;
    }

  g_return_val_if_fail (start_node != NULL, FALSE);
  g_return_val_if_fail (end_node != NULL, FALSE);

  if (anchor_path)
    {
      if (selection->tree_view->priv->anchor)
	btk_tree_row_reference_free (selection->tree_view->priv->anchor);

      selection->tree_view->priv->anchor =
	btk_tree_row_reference_new_proxy (B_OBJECT (selection->tree_view),
	                                  selection->tree_view->priv->model,
					  anchor_path);
    }

  do
    {
      dirty |= btk_tree_selection_real_select_node (selection, start_tree, start_node, (mode == RANGE_SELECT)?TRUE:FALSE);

      if (start_node == end_node)
	break;

      if (start_node->children)
	{
	  start_tree = start_node->children;
	  start_node = start_tree->root;
	  while (start_node->left != start_tree->nil)
	    start_node = start_node->left;
	}
      else
	{
	  _btk_rbtree_next_full (start_tree, start_node, &start_tree, &start_node);
	  if (start_tree == NULL)
	    {
	      /* we just ran out of tree.  That means someone passed in bogus values.
	       */
	      return dirty;
	    }
	}
    }
  while (TRUE);

  return dirty;
}

/**
 * btk_tree_selection_select_range:
 * @selection: A #BtkTreeSelection.
 * @start_path: The initial node of the range.
 * @end_path: The final node of the range.
 *
 * Selects a range of nodes, determined by @start_path and @end_path inclusive.
 * @selection must be set to #BTK_SELECTION_MULTIPLE mode. 
 **/
void
btk_tree_selection_select_range (BtkTreeSelection *selection,
				 BtkTreePath      *start_path,
				 BtkTreePath      *end_path)
{
  g_return_if_fail (BTK_IS_TREE_SELECTION (selection));
  g_return_if_fail (selection->tree_view != NULL);
  g_return_if_fail (selection->type == BTK_SELECTION_MULTIPLE);
  g_return_if_fail (selection->tree_view->priv->model != NULL);

  if (btk_tree_selection_real_modify_range (selection, RANGE_SELECT, start_path, end_path))
    g_signal_emit (selection, tree_selection_signals[CHANGED], 0);
}

/**
 * btk_tree_selection_unselect_range:
 * @selection: A #BtkTreeSelection.
 * @start_path: The initial node of the range.
 * @end_path: The initial node of the range.
 *
 * Unselects a range of nodes, determined by @start_path and @end_path
 * inclusive.
 *
 * Since: 2.2
 **/
void
btk_tree_selection_unselect_range (BtkTreeSelection *selection,
                                   BtkTreePath      *start_path,
				   BtkTreePath      *end_path)
{
  g_return_if_fail (BTK_IS_TREE_SELECTION (selection));
  g_return_if_fail (selection->tree_view != NULL);
  g_return_if_fail (selection->tree_view->priv->model != NULL);

  if (btk_tree_selection_real_modify_range (selection, RANGE_UNSELECT, start_path, end_path))
    g_signal_emit (selection, tree_selection_signals[CHANGED], 0);
}

gboolean
_btk_tree_selection_row_is_selectable (BtkTreeSelection *selection,
				       BtkRBNode        *node,
				       BtkTreePath      *path)
{
  BtkTreeIter iter;
  gboolean sensitive = FALSE;

  if (!btk_tree_model_get_iter (selection->tree_view->priv->model, &iter, path))
    sensitive = TRUE;

  if (!sensitive && selection->tree_view->priv->row_separator_func)
    {
      /* never allow separators to be selected */
      if ((* selection->tree_view->priv->row_separator_func) (selection->tree_view->priv->model,
							      &iter,
							      selection->tree_view->priv->row_separator_data))
	return FALSE;
    }

  if (selection->user_func)
    return (*selection->user_func) (selection, selection->tree_view->priv->model, path,
				    BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SELECTED),
				    selection->user_data);
  else
    return TRUE;
}


/* Called internally by btktreeview.c It handles actually selecting the tree.
 */

/*
 * docs about the 'override_browse_mode', we set this flag when we want to
 * unset select the node and override the select browse mode behaviour (that is
 * 'one node should *always* be selected').
 */
void
_btk_tree_selection_internal_select_node (BtkTreeSelection *selection,
					  BtkRBNode        *node,
					  BtkRBTree        *tree,
					  BtkTreePath      *path,
                                          BtkTreeSelectMode mode,
					  gboolean          override_browse_mode)
{
  gint flags;
  gint dirty = FALSE;
  BtkTreePath *anchor_path = NULL;

  if (selection->type == BTK_SELECTION_NONE)
    return;

  if (selection->tree_view->priv->anchor)
    anchor_path = btk_tree_row_reference_get_path (selection->tree_view->priv->anchor);

  if (selection->type == BTK_SELECTION_SINGLE ||
      selection->type == BTK_SELECTION_BROWSE)
    {
      /* just unselect */
      if (selection->type == BTK_SELECTION_BROWSE && override_browse_mode)
        {
	  dirty = btk_tree_selection_real_unselect_all (selection);
	}
      /* Did we try to select the same node again? */
      else if (selection->type == BTK_SELECTION_SINGLE &&
	       anchor_path && btk_tree_path_compare (path, anchor_path) == 0)
	{
	  if ((mode & BTK_TREE_SELECT_MODE_TOGGLE) == BTK_TREE_SELECT_MODE_TOGGLE)
	    {
	      dirty = btk_tree_selection_real_unselect_all (selection);
	    }
	}
      else
	{
	  if (anchor_path)
	    {
	      /* We only want to select the new node if we can unselect the old one,
	       * and we can select the new one. */
	      dirty = _btk_tree_selection_row_is_selectable (selection, node, path);

	      /* if dirty is FALSE, we weren't able to select the new one, otherwise, we try to
	       * unselect the new one
	       */
	      if (dirty)
		dirty = btk_tree_selection_real_unselect_all (selection);

	      /* if dirty is TRUE at this point, we successfully unselected the
	       * old one, and can then select the new one */
	      if (dirty)
		{
		  if (selection->tree_view->priv->anchor)
                    {
                      btk_tree_row_reference_free (selection->tree_view->priv->anchor);
                      selection->tree_view->priv->anchor = NULL;
                    }

		  if (btk_tree_selection_real_select_node (selection, tree, node, TRUE))
		    {
		      selection->tree_view->priv->anchor =
			btk_tree_row_reference_new_proxy (B_OBJECT (selection->tree_view), selection->tree_view->priv->model, path);
		    }
		}
	    }
	  else
	    {
	      if (btk_tree_selection_real_select_node (selection, tree, node, TRUE))
		{
		  dirty = TRUE;
		  if (selection->tree_view->priv->anchor)
		    btk_tree_row_reference_free (selection->tree_view->priv->anchor);

		  selection->tree_view->priv->anchor =
		    btk_tree_row_reference_new_proxy (B_OBJECT (selection->tree_view), selection->tree_view->priv->model, path);
		}
	    }
	}
    }
  else if (selection->type == BTK_SELECTION_MULTIPLE)
    {
      if ((mode & BTK_TREE_SELECT_MODE_EXTEND) == BTK_TREE_SELECT_MODE_EXTEND
          && (anchor_path == NULL))
	{
	  if (selection->tree_view->priv->anchor)
	    btk_tree_row_reference_free (selection->tree_view->priv->anchor);

	  selection->tree_view->priv->anchor =
	    btk_tree_row_reference_new_proxy (B_OBJECT (selection->tree_view), selection->tree_view->priv->model, path);
	  dirty = btk_tree_selection_real_select_node (selection, tree, node, TRUE);
	}
      else if ((mode & (BTK_TREE_SELECT_MODE_EXTEND | BTK_TREE_SELECT_MODE_TOGGLE)) == (BTK_TREE_SELECT_MODE_EXTEND | BTK_TREE_SELECT_MODE_TOGGLE))
	{
	  btk_tree_selection_select_range (selection,
					   anchor_path,
					   path);
	}
      else if ((mode & BTK_TREE_SELECT_MODE_TOGGLE) == BTK_TREE_SELECT_MODE_TOGGLE)
	{
	  flags = node->flags;
	  if (selection->tree_view->priv->anchor)
	    btk_tree_row_reference_free (selection->tree_view->priv->anchor);

	  selection->tree_view->priv->anchor =
	    btk_tree_row_reference_new_proxy (B_OBJECT (selection->tree_view), selection->tree_view->priv->model, path);

	  if ((flags & BTK_RBNODE_IS_SELECTED) == BTK_RBNODE_IS_SELECTED)
	    dirty |= btk_tree_selection_real_select_node (selection, tree, node, FALSE);
	  else
	    dirty |= btk_tree_selection_real_select_node (selection, tree, node, TRUE);
	}
      else if ((mode & BTK_TREE_SELECT_MODE_EXTEND) == BTK_TREE_SELECT_MODE_EXTEND)
	{
	  dirty = btk_tree_selection_real_unselect_all (selection);
	  dirty |= btk_tree_selection_real_modify_range (selection,
                                                         RANGE_SELECT,
							 anchor_path,
							 path);
	}
      else
	{
	  dirty = btk_tree_selection_real_unselect_all (selection);

	  if (selection->tree_view->priv->anchor)
	    btk_tree_row_reference_free (selection->tree_view->priv->anchor);

	  selection->tree_view->priv->anchor =
	    btk_tree_row_reference_new_proxy (B_OBJECT (selection->tree_view), selection->tree_view->priv->model, path);

	  dirty |= btk_tree_selection_real_select_node (selection, tree, node, TRUE);
	}
    }

  if (anchor_path)
    btk_tree_path_free (anchor_path);

  if (dirty)
    g_signal_emit (selection, tree_selection_signals[CHANGED], 0);  
}


void 
_btk_tree_selection_emit_changed (BtkTreeSelection *selection)
{
  g_signal_emit (selection, tree_selection_signals[CHANGED], 0);  
}

/* NOTE: Any {un,}selection ever done _MUST_ be done through this function!
 */

static gint
btk_tree_selection_real_select_node (BtkTreeSelection *selection,
				     BtkRBTree        *tree,
				     BtkRBNode        *node,
				     gboolean          select)
{
  gboolean toggle = FALSE;
  BtkTreePath *path = NULL;

  select = !! select;

  if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SELECTED) != select)
    {
      path = _btk_tree_view_find_path (selection->tree_view, tree, node);
      toggle = _btk_tree_selection_row_is_selectable (selection, node, path);
      btk_tree_path_free (path);
    }

  if (toggle)
    {
      node->flags ^= BTK_RBNODE_IS_SELECTED;

      _btk_tree_view_queue_draw_node (selection->tree_view, tree, node, NULL);
      
      return TRUE;
    }

  return FALSE;
}

#define __BTK_TREE_SELECTION_C__
#include "btkaliasdef.c"
