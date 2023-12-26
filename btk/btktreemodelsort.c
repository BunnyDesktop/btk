/* btktreemodelsort.c
 * Copyright (C) 2000,2001  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
 * Copyright (C) 2001,2002  Kristian Rietveld <kris@btk.org>
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

/* NOTE: There is a potential for confusion in this code as to whether an iter,
 * path or value refers to the BtkTreeModelSort model, or the child model being
 * sorted.  As a convention, variables referencing the child model will have an
 * s_ prefix before them (ie. s_iter, s_value, s_path);
 */

/* ITER FORMAT:
 *
 * iter->stamp = tree_model_sort->stamp
 * iter->user_data = SortLevel
 * iter->user_data2 = SortElt
 */

/* WARNING: this code is dangerous, can cause sleepless nights,
 * can cause your dog to die among other bad things
 *
 * we warned you and we're not liable for any head injuries.
 */

#include "config.h"
#include <string.h>

#include "btktreemodelsort.h"
#include "btktreesortable.h"
#include "btktreestore.h"
#include "btktreedatalist.h"
#include "btkintl.h"
#include "btkprivate.h"
#include "btktreednd.h"
#include "btkalias.h"

typedef struct _SortElt SortElt;
typedef struct _SortLevel SortLevel;
typedef struct _SortData SortData;
typedef struct _SortTuple SortTuple;

struct _SortElt
{
  BtkTreeIter  iter;
  SortLevel   *children;
  gint         offset;
  gint         ref_count;
  gint         zero_ref_count;
};

struct _SortLevel
{
  GArray    *array;
  gint       ref_count;
  gint       parent_elt_index;
  SortLevel *parent_level;
};

struct _SortData
{
  BtkTreeModelSort *tree_model_sort;
  BtkTreePath *parent_path;
  gint parent_path_depth;
  gint *parent_path_indices;
  BtkTreeIterCompareFunc sort_func;
  gpointer sort_data;
};

struct _SortTuple
{
  SortElt   *elt;
  gint       offset;
};

/* Properties */
enum {
  PROP_0,
  /* Construct args */
  PROP_MODEL
};



#define BTK_TREE_MODEL_SORT_CACHE_CHILD_ITERS(tree_model_sort) \
	(((BtkTreeModelSort *)tree_model_sort)->child_flags&BTK_TREE_MODEL_ITERS_PERSIST)
#define SORT_ELT(sort_elt) ((SortElt *)sort_elt)
#define SORT_LEVEL(sort_level) ((SortLevel *)sort_level)

#define SORT_LEVEL_PARENT_ELT(level) (&g_array_index (SORT_LEVEL ((level))->parent_level->array, SortElt, SORT_LEVEL ((level))->parent_elt_index))
#define SORT_LEVEL_ELT_INDEX(level, elt) (SORT_ELT ((elt)) - SORT_ELT (SORT_LEVEL ((level))->array->data))


#define GET_CHILD_ITER(tree_model_sort,ch_iter,so_iter) btk_tree_model_sort_convert_iter_to_child_iter((BtkTreeModelSort*)(tree_model_sort), (ch_iter), (so_iter));

#define NO_SORT_FUNC ((BtkTreeIterCompareFunc) 0x1)

#define VALID_ITER(iter, tree_model_sort) ((iter) != NULL && (iter)->user_data != NULL && (iter)->user_data2 != NULL && (tree_model_sort)->stamp == (iter)->stamp)

/* general (object/interface init, etc) */
static void btk_tree_model_sort_tree_model_init       (BtkTreeModelIface     *iface);
static void btk_tree_model_sort_tree_sortable_init    (BtkTreeSortableIface  *iface);
static void btk_tree_model_sort_drag_source_init      (BtkTreeDragSourceIface*iface);
static void btk_tree_model_sort_finalize              (GObject               *object);
static void btk_tree_model_sort_set_property          (GObject               *object,
						       guint                  prop_id,
						       const GValue          *value,
						       GParamSpec            *pspec);
static void btk_tree_model_sort_get_property          (GObject               *object,
						       guint                  prop_id,
						       GValue                *value,
						       GParamSpec            *pspec);

/* our signal handlers */
static void btk_tree_model_sort_row_changed           (BtkTreeModel          *model,
						       BtkTreePath           *start_path,
						       BtkTreeIter           *start_iter,
						       gpointer               data);
static void btk_tree_model_sort_row_inserted          (BtkTreeModel          *model,
						       BtkTreePath           *path,
						       BtkTreeIter           *iter,
						       gpointer               data);
static void btk_tree_model_sort_row_has_child_toggled (BtkTreeModel          *model,
						       BtkTreePath           *path,
						       BtkTreeIter           *iter,
						       gpointer               data);
static void btk_tree_model_sort_row_deleted           (BtkTreeModel          *model,
						       BtkTreePath           *path,
						       gpointer               data);
static void btk_tree_model_sort_rows_reordered        (BtkTreeModel          *s_model,
						       BtkTreePath           *s_path,
						       BtkTreeIter           *s_iter,
						       gint                  *new_order,
						       gpointer               data);

/* TreeModel interface */
static BtkTreeModelFlags btk_tree_model_sort_get_flags     (BtkTreeModel          *tree_model);
static gint         btk_tree_model_sort_get_n_columns      (BtkTreeModel          *tree_model);
static GType        btk_tree_model_sort_get_column_type    (BtkTreeModel          *tree_model,
                                                            gint                   index);
static gboolean     btk_tree_model_sort_get_iter           (BtkTreeModel          *tree_model,
                                                            BtkTreeIter           *iter,
                                                            BtkTreePath           *path);
static BtkTreePath *btk_tree_model_sort_get_path           (BtkTreeModel          *tree_model,
                                                            BtkTreeIter           *iter);
static void         btk_tree_model_sort_get_value          (BtkTreeModel          *tree_model,
                                                            BtkTreeIter           *iter,
                                                            gint                   column,
                                                            GValue                *value);
static gboolean     btk_tree_model_sort_iter_next          (BtkTreeModel          *tree_model,
                                                            BtkTreeIter           *iter);
static gboolean     btk_tree_model_sort_iter_children      (BtkTreeModel          *tree_model,
                                                            BtkTreeIter           *iter,
                                                            BtkTreeIter           *parent);
static gboolean     btk_tree_model_sort_iter_has_child     (BtkTreeModel          *tree_model,
                                                            BtkTreeIter           *iter);
static gint         btk_tree_model_sort_iter_n_children    (BtkTreeModel          *tree_model,
                                                            BtkTreeIter           *iter);
static gboolean     btk_tree_model_sort_iter_nth_child     (BtkTreeModel          *tree_model,
                                                            BtkTreeIter           *iter,
                                                            BtkTreeIter           *parent,
                                                            gint                   n);
static gboolean     btk_tree_model_sort_iter_parent        (BtkTreeModel          *tree_model,
                                                            BtkTreeIter           *iter,
                                                            BtkTreeIter           *child);
static void         btk_tree_model_sort_ref_node           (BtkTreeModel          *tree_model,
                                                            BtkTreeIter           *iter);
static void         btk_tree_model_sort_real_unref_node    (BtkTreeModel          *tree_model,
                                                            BtkTreeIter           *iter,
							    gboolean               propagate_unref);
static void         btk_tree_model_sort_unref_node         (BtkTreeModel          *tree_model,
                                                            BtkTreeIter           *iter);

/* TreeDragSource interface */
static gboolean     btk_tree_model_sort_row_draggable         (BtkTreeDragSource      *drag_source,
                                                               BtkTreePath            *path);
static gboolean     btk_tree_model_sort_drag_data_get         (BtkTreeDragSource      *drag_source,
                                                               BtkTreePath            *path,
							       BtkSelectionData       *selection_data);
static gboolean     btk_tree_model_sort_drag_data_delete      (BtkTreeDragSource      *drag_source,
                                                               BtkTreePath            *path);

/* TreeSortable interface */
static gboolean     btk_tree_model_sort_get_sort_column_id    (BtkTreeSortable        *sortable,
							       gint                   *sort_column_id,
							       BtkSortType            *order);
static void         btk_tree_model_sort_set_sort_column_id    (BtkTreeSortable        *sortable,
							       gint                    sort_column_id,
							       BtkSortType        order);
static void         btk_tree_model_sort_set_sort_func         (BtkTreeSortable        *sortable,
							       gint                    sort_column_id,
							       BtkTreeIterCompareFunc  func,
							       gpointer                data,
							       GDestroyNotify          destroy);
static void         btk_tree_model_sort_set_default_sort_func (BtkTreeSortable        *sortable,
							       BtkTreeIterCompareFunc  func,
							       gpointer                data,
							       GDestroyNotify          destroy);
static gboolean     btk_tree_model_sort_has_default_sort_func (BtkTreeSortable     *sortable);

/* Private functions (sort funcs, level handling and other utils) */
static void         btk_tree_model_sort_build_level       (BtkTreeModelSort *tree_model_sort,
							   SortLevel        *parent_level,
							   gint              parent_elt_index);
static void         btk_tree_model_sort_free_level        (BtkTreeModelSort *tree_model_sort,
							   SortLevel        *sort_level);
static void         btk_tree_model_sort_increment_stamp   (BtkTreeModelSort *tree_model_sort);
static void         btk_tree_model_sort_sort_level        (BtkTreeModelSort *tree_model_sort,
							   SortLevel        *level,
							   gboolean          recurse,
							   gboolean          emit_reordered);
static void         btk_tree_model_sort_sort              (BtkTreeModelSort *tree_model_sort);
static gint         btk_tree_model_sort_level_find_insert (BtkTreeModelSort *tree_model_sort,
							   SortLevel        *level,
							   BtkTreeIter      *iter,
							   gint             skip_index);
static gboolean     btk_tree_model_sort_insert_value      (BtkTreeModelSort *tree_model_sort,
							   SortLevel        *level,
							   BtkTreePath      *s_path,
							   BtkTreeIter      *s_iter);
static BtkTreePath *btk_tree_model_sort_elt_get_path      (SortLevel        *level,
							   SortElt          *elt);
static void         btk_tree_model_sort_set_model         (BtkTreeModelSort *tree_model_sort,
							   BtkTreeModel     *child_model);
static BtkTreePath *btk_real_tree_model_sort_convert_child_path_to_path (BtkTreeModelSort *tree_model_sort,
									 BtkTreePath      *child_path,
									 gboolean          build_levels);


G_DEFINE_TYPE_WITH_CODE (BtkTreeModelSort, btk_tree_model_sort, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_TREE_MODEL,
						btk_tree_model_sort_tree_model_init)
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_TREE_SORTABLE,
						btk_tree_model_sort_tree_sortable_init)
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_TREE_DRAG_SOURCE,
						btk_tree_model_sort_drag_source_init))

static void
btk_tree_model_sort_init (BtkTreeModelSort *tree_model_sort)
{
  tree_model_sort->sort_column_id = BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID;
  tree_model_sort->stamp = 0;
  tree_model_sort->zero_ref_count = 0;
  tree_model_sort->root = NULL;
  tree_model_sort->sort_list = NULL;
}

static void
btk_tree_model_sort_class_init (BtkTreeModelSortClass *class)
{
  GObjectClass *object_class;

  object_class = (GObjectClass *) class;

  object_class->set_property = btk_tree_model_sort_set_property;
  object_class->get_property = btk_tree_model_sort_get_property;

  object_class->finalize = btk_tree_model_sort_finalize;

  /* Properties */
  g_object_class_install_property (object_class,
                                   PROP_MODEL,
                                   g_param_spec_object ("model",
							P_("TreeModelSort Model"),
							P_("The model for the TreeModelSort to sort"),
							BTK_TYPE_TREE_MODEL,
							BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
btk_tree_model_sort_tree_model_init (BtkTreeModelIface *iface)
{
  iface->get_flags = btk_tree_model_sort_get_flags;
  iface->get_n_columns = btk_tree_model_sort_get_n_columns;
  iface->get_column_type = btk_tree_model_sort_get_column_type;
  iface->get_iter = btk_tree_model_sort_get_iter;
  iface->get_path = btk_tree_model_sort_get_path;
  iface->get_value = btk_tree_model_sort_get_value;
  iface->iter_next = btk_tree_model_sort_iter_next;
  iface->iter_children = btk_tree_model_sort_iter_children;
  iface->iter_has_child = btk_tree_model_sort_iter_has_child;
  iface->iter_n_children = btk_tree_model_sort_iter_n_children;
  iface->iter_nth_child = btk_tree_model_sort_iter_nth_child;
  iface->iter_parent = btk_tree_model_sort_iter_parent;
  iface->ref_node = btk_tree_model_sort_ref_node;
  iface->unref_node = btk_tree_model_sort_unref_node;
}

static void
btk_tree_model_sort_tree_sortable_init (BtkTreeSortableIface *iface)
{
  iface->get_sort_column_id = btk_tree_model_sort_get_sort_column_id;
  iface->set_sort_column_id = btk_tree_model_sort_set_sort_column_id;
  iface->set_sort_func = btk_tree_model_sort_set_sort_func;
  iface->set_default_sort_func = btk_tree_model_sort_set_default_sort_func;
  iface->has_default_sort_func = btk_tree_model_sort_has_default_sort_func;
}

static void
btk_tree_model_sort_drag_source_init (BtkTreeDragSourceIface *iface)
{
  iface->row_draggable = btk_tree_model_sort_row_draggable;
  iface->drag_data_delete = btk_tree_model_sort_drag_data_delete;
  iface->drag_data_get = btk_tree_model_sort_drag_data_get;
}

/**
 * btk_tree_model_sort_new_with_model:
 * @child_model: A #BtkTreeModel
 *
 * Creates a new #BtkTreeModel, with @child_model as the child model.
 *
 * Return value: (transfer full): A new #BtkTreeModel.
 */
BtkTreeModel *
btk_tree_model_sort_new_with_model (BtkTreeModel *child_model)
{
  BtkTreeModel *retval;

  g_return_val_if_fail (BTK_IS_TREE_MODEL (child_model), NULL);

  retval = g_object_new (btk_tree_model_sort_get_type (), NULL);

  btk_tree_model_sort_set_model (BTK_TREE_MODEL_SORT (retval), child_model);

  return retval;
}

/* GObject callbacks */
static void
btk_tree_model_sort_finalize (GObject *object)
{
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *) object;

  btk_tree_model_sort_set_model (tree_model_sort, NULL);

  if (tree_model_sort->root)
    btk_tree_model_sort_free_level (tree_model_sort, tree_model_sort->root);

  if (tree_model_sort->sort_list)
    {
      _btk_tree_data_list_header_free (tree_model_sort->sort_list);
      tree_model_sort->sort_list = NULL;
    }

  if (tree_model_sort->default_sort_destroy)
    {
      tree_model_sort->default_sort_destroy (tree_model_sort->default_sort_data);
      tree_model_sort->default_sort_destroy = NULL;
      tree_model_sort->default_sort_data = NULL;
    }


  /* must chain up */
  G_OBJECT_CLASS (btk_tree_model_sort_parent_class)->finalize (object);
}

static void
btk_tree_model_sort_set_property (GObject      *object,
				  guint         prop_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
  BtkTreeModelSort *tree_model_sort = BTK_TREE_MODEL_SORT (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      btk_tree_model_sort_set_model (tree_model_sort, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_tree_model_sort_get_property (GObject    *object,
				  guint       prop_id,
				  GValue     *value,
				  GParamSpec *pspec)
{
  BtkTreeModelSort *tree_model_sort = BTK_TREE_MODEL_SORT (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      g_value_set_object (value, btk_tree_model_sort_get_model(tree_model_sort));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_tree_model_sort_row_changed (BtkTreeModel *s_model,
				 BtkTreePath  *start_s_path,
				 BtkTreeIter  *start_s_iter,
				 gpointer      data)
{
  BtkTreeModelSort *tree_model_sort = BTK_TREE_MODEL_SORT (data);
  BtkTreePath *path = NULL;
  BtkTreeIter iter;
  BtkTreeIter tmpiter;

  SortElt tmp;
  SortElt *elt;
  SortLevel *level;

  gboolean free_s_path = FALSE;

  gint index = 0, old_index, i;

  g_return_if_fail (start_s_path != NULL || start_s_iter != NULL);

  if (!start_s_path)
    {
      free_s_path = TRUE;
      start_s_path = btk_tree_model_get_path (s_model, start_s_iter);
    }

  path = btk_real_tree_model_sort_convert_child_path_to_path (tree_model_sort,
							      start_s_path,
							      FALSE);
  if (!path)
    {
      if (free_s_path)
	btk_tree_path_free (start_s_path);
      return;
    }

  btk_tree_model_get_iter (BTK_TREE_MODEL (data), &iter, path);
  btk_tree_model_sort_ref_node (BTK_TREE_MODEL (data), &iter);

  level = iter.user_data;
  elt = iter.user_data2;

  if (level->array->len < 2 ||
      (tree_model_sort->sort_column_id == BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID &&
       tree_model_sort->default_sort_func == NO_SORT_FUNC))
    {
      if (free_s_path)
	btk_tree_path_free (start_s_path);

      btk_tree_model_row_changed (BTK_TREE_MODEL (data), path, &iter);
      btk_tree_model_sort_unref_node (BTK_TREE_MODEL (data), &iter);

      btk_tree_path_free (path);

      return;
    }
  
  if (!BTK_TREE_MODEL_SORT_CACHE_CHILD_ITERS (tree_model_sort))
    {
      btk_tree_model_get_iter (tree_model_sort->child_model,
			       &tmpiter, start_s_path);
    }

  old_index = elt - SORT_ELT (level->array->data);

  memcpy (&tmp, elt, sizeof (SortElt));

  if (BTK_TREE_MODEL_SORT_CACHE_CHILD_ITERS (tree_model_sort))
    index = btk_tree_model_sort_level_find_insert (tree_model_sort,
						   level,
						   &tmp.iter,
						   old_index);
  else
    index = btk_tree_model_sort_level_find_insert (tree_model_sort,
						   level,
						   &tmpiter,
						   old_index);

  if (index < old_index)
    {
      g_memmove (level->array->data + ((index + 1)*sizeof (SortElt)),
		 level->array->data + ((index)*sizeof (SortElt)),
		 (old_index - index)* sizeof(SortElt));
    }
  else if (index > old_index)
    {
      g_memmove (level->array->data + ((old_index)*sizeof (SortElt)),
		 level->array->data + ((old_index + 1)*sizeof (SortElt)),
		 (index - old_index)* sizeof(SortElt));
    }
  memcpy (level->array->data + ((index)*sizeof (SortElt)),
	  &tmp, sizeof (SortElt));

  for (i = 0; i < level->array->len; i++)
    if (g_array_index (level->array, SortElt, i).children)
      g_array_index (level->array, SortElt, i).children->parent_elt_index = i;

  btk_tree_path_up (path);
  btk_tree_path_append_index (path, index);

  btk_tree_model_sort_increment_stamp (tree_model_sort);

  /* if the item moved, then emit rows_reordered */
  if (old_index != index)
    {
      gint *new_order;
      gint j;

      BtkTreePath *tmppath;

      new_order = g_new (gint, level->array->len);

      for (j = 0; j < level->array->len; j++)
        {
	  if (index > old_index)
	    {
	      if (j == index)
		new_order[j] = old_index;
	      else if (j >= old_index && j < index)
		new_order[j] = j + 1;
	      else
		new_order[j] = j;
	    }
	  else if (index < old_index)
	    {
	      if (j == index)
		new_order[j] = old_index;
	      else if (j > index && j <= old_index)
		new_order[j] = j - 1;
	      else
		new_order[j] = j;
	    }
	  /* else? shouldn't really happen */
	}

      if (level->parent_elt_index >= 0)
        {
	  iter.stamp = tree_model_sort->stamp;
	  iter.user_data = level->parent_level;
	  iter.user_data2 = SORT_LEVEL_PARENT_ELT (level);

	  tmppath = btk_tree_model_get_path (BTK_TREE_MODEL (tree_model_sort), &iter);

	  btk_tree_model_rows_reordered (BTK_TREE_MODEL (tree_model_sort),
	                                 tmppath, &iter, new_order);
	}
      else
        {
	  /* toplevel */
	  tmppath = btk_tree_path_new ();

          btk_tree_model_rows_reordered (BTK_TREE_MODEL (tree_model_sort), tmppath,
	                                 NULL, new_order);
	}

      btk_tree_path_free (tmppath);
      g_free (new_order);
    }

  /* emit row_changed signal (at new location) */
  btk_tree_model_get_iter (BTK_TREE_MODEL (data), &iter, path);
  btk_tree_model_row_changed (BTK_TREE_MODEL (data), path, &iter);
  btk_tree_model_sort_unref_node (BTK_TREE_MODEL (data), &iter);

  btk_tree_path_free (path);
  if (free_s_path)
    btk_tree_path_free (start_s_path);
}

static void
btk_tree_model_sort_row_inserted (BtkTreeModel          *s_model,
				  BtkTreePath           *s_path,
				  BtkTreeIter           *s_iter,
				  gpointer               data)
{
  BtkTreeModelSort *tree_model_sort = BTK_TREE_MODEL_SORT (data);
  BtkTreePath *path;
  BtkTreeIter iter;
  BtkTreeIter real_s_iter;

  gint i = 0;

  gboolean free_s_path = FALSE;

  SortElt *elt;
  SortLevel *level;
  SortLevel *parent_level = NULL;

  parent_level = level = SORT_LEVEL (tree_model_sort->root);

  g_return_if_fail (s_path != NULL || s_iter != NULL);

  if (!s_path)
    {
      s_path = btk_tree_model_get_path (s_model, s_iter);
      free_s_path = TRUE;
    }

  if (!s_iter)
    btk_tree_model_get_iter (s_model, &real_s_iter, s_path);
  else
    real_s_iter = *s_iter;

  if (!tree_model_sort->root)
    {
      btk_tree_model_sort_build_level (tree_model_sort, NULL, -1);

      /* the build level already put the inserted iter in the level,
	 so no need to handle this signal anymore */

      goto done_and_submit;
    }

  /* find the parent level */
  while (i < btk_tree_path_get_depth (s_path) - 1)
    {
      gint j;

      if (!level)
	{
	  /* level not yet build, we won't cover this signal */
	  goto done;
	}

      if (level->array->len < btk_tree_path_get_indices (s_path)[i])
	{
	  g_warning ("%s: A node was inserted with a parent that's not in the tree.\n"
		     "This possibly means that a BtkTreeModel inserted a child node\n"
		     "before the parent was inserted.",
		     G_STRLOC);
	  goto done;
	}

      elt = NULL;
      for (j = 0; j < level->array->len; j++)
	if (g_array_index (level->array, SortElt, j).offset == btk_tree_path_get_indices (s_path)[i])
	  {
	    elt = &g_array_index (level->array, SortElt, j);
	    break;
	  }

      g_return_if_fail (elt != NULL);

      if (!elt->children)
	{
	  /* not covering this signal */
	  goto done;
	}

      level = elt->children;
      parent_level = level;
      i++;
    }

  if (!parent_level)
    goto done;

  if (level->ref_count == 0 && level != tree_model_sort->root)
    {
      btk_tree_model_sort_free_level (tree_model_sort, level);
      goto done;
    }

  if (!btk_tree_model_sort_insert_value (tree_model_sort,
					 parent_level,
					 s_path,
					 &real_s_iter))
    goto done;

 done_and_submit:
  path = btk_real_tree_model_sort_convert_child_path_to_path (tree_model_sort,
							      s_path,
							      FALSE);

  if (!path)
    return;

  btk_tree_model_sort_increment_stamp (tree_model_sort);

  btk_tree_model_get_iter (BTK_TREE_MODEL (data), &iter, path);
  btk_tree_model_row_inserted (BTK_TREE_MODEL (data), path, &iter);
  btk_tree_path_free (path);

 done:
  if (free_s_path)
    btk_tree_path_free (s_path);

  return;
}

static void
btk_tree_model_sort_row_has_child_toggled (BtkTreeModel *s_model,
					   BtkTreePath  *s_path,
					   BtkTreeIter  *s_iter,
					   gpointer      data)
{
  BtkTreeModelSort *tree_model_sort = BTK_TREE_MODEL_SORT (data);
  BtkTreePath *path;
  BtkTreeIter iter;

  g_return_if_fail (s_path != NULL && s_iter != NULL);

  path = btk_real_tree_model_sort_convert_child_path_to_path (tree_model_sort, s_path, FALSE);
  if (path == NULL)
    return;

  btk_tree_model_get_iter (BTK_TREE_MODEL (data), &iter, path);
  btk_tree_model_row_has_child_toggled (BTK_TREE_MODEL (data), path, &iter);

  btk_tree_path_free (path);
}

static void
btk_tree_model_sort_row_deleted (BtkTreeModel *s_model,
				 BtkTreePath  *s_path,
				 gpointer      data)
{
  BtkTreeModelSort *tree_model_sort = BTK_TREE_MODEL_SORT (data);
  BtkTreePath *path = NULL;
  SortElt *elt;
  SortLevel *level;
  BtkTreeIter iter;
  gint offset;
  gint i;

  g_return_if_fail (s_path != NULL);

  path = btk_real_tree_model_sort_convert_child_path_to_path (tree_model_sort, s_path, FALSE);
  if (path == NULL)
    return;

  btk_tree_model_get_iter (BTK_TREE_MODEL (data), &iter, path);

  level = SORT_LEVEL (iter.user_data);
  elt = SORT_ELT (iter.user_data2);
  offset = elt->offset;

  /* we _need_ to emit ::row_deleted before we start unreffing the node
   * itself. This is because of the row refs, which start unreffing nodes
   * when we emit ::row_deleted
   */
  btk_tree_model_row_deleted (BTK_TREE_MODEL (data), path);

  btk_tree_model_get_iter (BTK_TREE_MODEL (data), &iter, path);

  while (elt->ref_count > 0)
    btk_tree_model_sort_real_unref_node (BTK_TREE_MODEL (data), &iter, FALSE);

  if (level->ref_count == 0)
    {
      /* This will prune the level, so I can just emit the signal and 
       * not worry about cleaning this level up. 
       * Careful, root level is not cleaned up in increment stamp.
       */
      btk_tree_model_sort_increment_stamp (tree_model_sort);
      btk_tree_path_free (path);
      if (level == tree_model_sort->root)
	{
	  btk_tree_model_sort_free_level (tree_model_sort, 
					  tree_model_sort->root);
	  tree_model_sort->root = NULL;
	}
      return;
    }

  btk_tree_model_sort_increment_stamp (tree_model_sort);

  /* Remove the row */
  for (i = 0; i < level->array->len; i++)
    if (elt->offset == g_array_index (level->array, SortElt, i).offset)
      break;

  g_array_remove_index (level->array, i);

  /* update all offsets */
  for (i = 0; i < level->array->len; i++)
    {
      elt = & (g_array_index (level->array, SortElt, i));
      if (elt->offset > offset)
	elt->offset--;
      if (elt->children)
	elt->children->parent_elt_index = i;
    }

  btk_tree_path_free (path);
}

static void
btk_tree_model_sort_rows_reordered (BtkTreeModel *s_model,
				    BtkTreePath  *s_path,
				    BtkTreeIter  *s_iter,
				    gint         *new_order,
				    gpointer      data)
{
  SortElt *elt;
  SortLevel *level;
  BtkTreeIter iter;
  gint *tmp_array;
  int i, j;
  BtkTreePath *path;
  BtkTreeModelSort *tree_model_sort = BTK_TREE_MODEL_SORT (data);

  g_return_if_fail (new_order != NULL);

  if (s_path == NULL || btk_tree_path_get_depth (s_path) == 0)
    {
      if (tree_model_sort->root == NULL)
	return;
      path = btk_tree_path_new ();
      level = SORT_LEVEL (tree_model_sort->root);
    }
  else
    {
      path = btk_real_tree_model_sort_convert_child_path_to_path (tree_model_sort, s_path, FALSE);
      if (path == NULL)
	return;
      btk_tree_model_get_iter (BTK_TREE_MODEL (data), &iter, path);

      level = SORT_LEVEL (iter.user_data);
      elt = SORT_ELT (iter.user_data2);

      if (!elt->children)
	{
	  btk_tree_path_free (path);
	  return;
	}

      level = elt->children;
    }

  if (level->array->len < 2)
    {
      btk_tree_path_free (path);
      return;
    }

  tmp_array = g_new (int, level->array->len);
  for (i = 0; i < level->array->len; i++)
    {
      for (j = 0; j < level->array->len; j++)
	{
	  if (g_array_index (level->array, SortElt, i).offset == new_order[j])
	    tmp_array[i] = j;
	}
    }

  for (i = 0; i < level->array->len; i++)
    g_array_index (level->array, SortElt, i).offset = tmp_array[i];
  g_free (tmp_array);

  if (tree_model_sort->sort_column_id == BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID &&
      tree_model_sort->default_sort_func == NO_SORT_FUNC)
    {

      btk_tree_model_sort_sort_level (tree_model_sort, level,
				      FALSE, FALSE);
      btk_tree_model_sort_increment_stamp (tree_model_sort);

      if (btk_tree_path_get_depth (path))
	{
	  btk_tree_model_get_iter (BTK_TREE_MODEL (tree_model_sort),
				   &iter,
				   path);
	  btk_tree_model_rows_reordered (BTK_TREE_MODEL (tree_model_sort),
					 path, &iter, new_order);
	}
      else
	{
	  btk_tree_model_rows_reordered (BTK_TREE_MODEL (tree_model_sort),
					 path, NULL, new_order);
	}
    }

  btk_tree_path_free (path);
}

/* Fulfill our model requirements */
static BtkTreeModelFlags
btk_tree_model_sort_get_flags (BtkTreeModel *tree_model)
{
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *) tree_model;
  BtkTreeModelFlags flags;

  g_return_val_if_fail (tree_model_sort->child_model != NULL, 0);

  flags = btk_tree_model_get_flags (tree_model_sort->child_model);

  if ((flags & BTK_TREE_MODEL_LIST_ONLY) == BTK_TREE_MODEL_LIST_ONLY)
    return BTK_TREE_MODEL_LIST_ONLY;

  return 0;
}

static gint
btk_tree_model_sort_get_n_columns (BtkTreeModel *tree_model)
{
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *) tree_model;

  if (tree_model_sort->child_model == 0)
    return 0;

  return btk_tree_model_get_n_columns (tree_model_sort->child_model);
}

static GType
btk_tree_model_sort_get_column_type (BtkTreeModel *tree_model,
                                     gint          index)
{
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *) tree_model;

  g_return_val_if_fail (tree_model_sort->child_model != NULL, G_TYPE_INVALID);

  return btk_tree_model_get_column_type (tree_model_sort->child_model, index);
}

static gboolean
btk_tree_model_sort_get_iter (BtkTreeModel *tree_model,
			      BtkTreeIter  *iter,
			      BtkTreePath  *path)
{
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *) tree_model;
  gint *indices;
  SortLevel *level;
  gint depth, i;

  g_return_val_if_fail (tree_model_sort->child_model != NULL, FALSE);

  indices = btk_tree_path_get_indices (path);

  if (tree_model_sort->root == NULL)
    btk_tree_model_sort_build_level (tree_model_sort, NULL, -1);
  level = SORT_LEVEL (tree_model_sort->root);

  depth = btk_tree_path_get_depth (path);
  if (depth == 0)
    return FALSE;

  for (i = 0; i < depth - 1; i++)
    {
      if ((level == NULL) ||
	  (indices[i] >= level->array->len))
	return FALSE;

      if (g_array_index (level->array, SortElt, indices[i]).children == NULL)
	btk_tree_model_sort_build_level (tree_model_sort, level, indices[i]);
      level = g_array_index (level->array, SortElt, indices[i]).children;
    }

  if (!level || indices[i] >= level->array->len)
    {
      iter->stamp = 0;
      return FALSE;
    }

  iter->stamp = tree_model_sort->stamp;
  iter->user_data = level;
  iter->user_data2 = &g_array_index (level->array, SortElt, indices[depth - 1]);

  return TRUE;
}

static BtkTreePath *
btk_tree_model_sort_get_path (BtkTreeModel *tree_model,
			      BtkTreeIter  *iter)
{
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *) tree_model;
  BtkTreePath *retval;
  SortLevel *level;
  gint elt_index;

  g_return_val_if_fail (tree_model_sort->child_model != NULL, NULL);
  g_return_val_if_fail (tree_model_sort->stamp == iter->stamp, NULL);

  retval = btk_tree_path_new ();

  level = SORT_LEVEL (iter->user_data);
  elt_index = SORT_LEVEL_ELT_INDEX (level, iter->user_data2);

  while (level)
    {
      btk_tree_path_prepend_index (retval, elt_index);

      elt_index = level->parent_elt_index;
      level = level->parent_level;
    }

  return retval;
}

static void
btk_tree_model_sort_get_value (BtkTreeModel *tree_model,
			       BtkTreeIter  *iter,
			       gint          column,
			       GValue       *value)
{
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *) tree_model;
  BtkTreeIter child_iter;

  g_return_if_fail (tree_model_sort->child_model != NULL);
  g_return_if_fail (VALID_ITER (iter, tree_model_sort));

  GET_CHILD_ITER (tree_model_sort, &child_iter, iter);
  btk_tree_model_get_value (tree_model_sort->child_model,
			    &child_iter, column, value);
}

static gboolean
btk_tree_model_sort_iter_next (BtkTreeModel *tree_model,
			       BtkTreeIter  *iter)
{
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *) tree_model;
  SortLevel *level;
  SortElt *elt;

  g_return_val_if_fail (tree_model_sort->child_model != NULL, FALSE);
  g_return_val_if_fail (tree_model_sort->stamp == iter->stamp, FALSE);

  level = iter->user_data;
  elt = iter->user_data2;

  if (elt - (SortElt *)level->array->data >= level->array->len - 1)
    {
      iter->stamp = 0;
      return FALSE;
    }
  iter->user_data2 = elt + 1;

  return TRUE;
}

static gboolean
btk_tree_model_sort_iter_children (BtkTreeModel *tree_model,
				   BtkTreeIter  *iter,
				   BtkTreeIter  *parent)
{
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *) tree_model;
  SortLevel *level;

  iter->stamp = 0;
  g_return_val_if_fail (tree_model_sort->child_model != NULL, FALSE);
  if (parent) 
    g_return_val_if_fail (VALID_ITER (parent, tree_model_sort), FALSE);

  if (parent == NULL)
    {
      if (tree_model_sort->root == NULL)
	btk_tree_model_sort_build_level (tree_model_sort, NULL, -1);
      if (tree_model_sort->root == NULL)
	return FALSE;

      level = tree_model_sort->root;
      iter->stamp = tree_model_sort->stamp;
      iter->user_data = level;
      iter->user_data2 = level->array->data;
    }
  else
    {
      SortElt *elt;

      level = SORT_LEVEL (parent->user_data);
      elt = SORT_ELT (parent->user_data2);

      if (elt->children == NULL)
        btk_tree_model_sort_build_level (tree_model_sort, level,
                                         SORT_LEVEL_ELT_INDEX (level, elt));

      if (elt->children == NULL)
	return FALSE;

      iter->stamp = tree_model_sort->stamp;
      iter->user_data = elt->children;
      iter->user_data2 = elt->children->array->data;
    }

  return TRUE;
}

static gboolean
btk_tree_model_sort_iter_has_child (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter)
{
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *) tree_model;
  BtkTreeIter child_iter;

  g_return_val_if_fail (tree_model_sort->child_model != NULL, FALSE);
  g_return_val_if_fail (VALID_ITER (iter, tree_model_sort), FALSE);

  GET_CHILD_ITER (tree_model_sort, &child_iter, iter);

  return btk_tree_model_iter_has_child (tree_model_sort->child_model, &child_iter);
}

static gint
btk_tree_model_sort_iter_n_children (BtkTreeModel *tree_model,
				     BtkTreeIter  *iter)
{
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *) tree_model;
  BtkTreeIter child_iter;

  g_return_val_if_fail (tree_model_sort->child_model != NULL, 0);
  if (iter) 
    g_return_val_if_fail (VALID_ITER (iter, tree_model_sort), 0);

  if (iter == NULL)
    return btk_tree_model_iter_n_children (tree_model_sort->child_model, NULL);

  GET_CHILD_ITER (tree_model_sort, &child_iter, iter);

  return btk_tree_model_iter_n_children (tree_model_sort->child_model, &child_iter);
}

static gboolean
btk_tree_model_sort_iter_nth_child (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter,
				    BtkTreeIter  *parent,
				    gint          n)
{
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *) tree_model;
  SortLevel *level;
  /* We have this for the iter == parent case */
  BtkTreeIter children;

  if (parent) 
    g_return_val_if_fail (VALID_ITER (parent, tree_model_sort), FALSE);

  /* Use this instead of has_child to force us to build the level, if needed */
  if (btk_tree_model_sort_iter_children (tree_model, &children, parent) == FALSE)
    {
      iter->stamp = 0;
      return FALSE;
    }

  level = children.user_data;
  if (n >= level->array->len)
    {
      iter->stamp = 0;
      return FALSE;
    }

  iter->stamp = tree_model_sort->stamp;
  iter->user_data = level;
  iter->user_data2 = &g_array_index (level->array, SortElt, n);

  return TRUE;
}

static gboolean
btk_tree_model_sort_iter_parent (BtkTreeModel *tree_model,
				 BtkTreeIter  *iter,
				 BtkTreeIter  *child)
{ 
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *) tree_model;
  SortLevel *level;

  iter->stamp = 0;
  g_return_val_if_fail (tree_model_sort->child_model != NULL, FALSE);
  g_return_val_if_fail (VALID_ITER (child, tree_model_sort), FALSE);

  level = child->user_data;

  if (level->parent_level)
    {
      iter->stamp = tree_model_sort->stamp;
      iter->user_data = level->parent_level;
      iter->user_data2 = SORT_LEVEL_PARENT_ELT (level);

      return TRUE;
    }
  return FALSE;
}

static void
btk_tree_model_sort_ref_node (BtkTreeModel *tree_model,
			      BtkTreeIter  *iter)
{
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *) tree_model;
  BtkTreeIter child_iter;
  SortLevel *level, *parent_level;
  SortElt *elt;
  gint parent_elt_index;

  g_return_if_fail (tree_model_sort->child_model != NULL);
  g_return_if_fail (VALID_ITER (iter, tree_model_sort));

  GET_CHILD_ITER (tree_model_sort, &child_iter, iter);

  /* Reference the node in the child model */
  btk_tree_model_ref_node (tree_model_sort->child_model, &child_iter);

  /* Increase the reference count of this element and its level */
  level = iter->user_data;
  elt = iter->user_data2;

  elt->ref_count++;
  level->ref_count++;

  /* Increase the reference count of all parent elements */
  parent_level = level->parent_level;
  parent_elt_index = level->parent_elt_index;

  while (parent_level)
    {
      BtkTreeIter tmp_iter;

      tmp_iter.stamp = tree_model_sort->stamp;
      tmp_iter.user_data = parent_level;
      tmp_iter.user_data2 = &g_array_index (parent_level->array, SortElt, parent_elt_index);

      btk_tree_model_sort_ref_node (tree_model, &tmp_iter);

      parent_elt_index = parent_level->parent_elt_index;
      parent_level = parent_level->parent_level;
    }

  if (level->ref_count == 1)
    {
      SortLevel *parent_level = level->parent_level;
      gint parent_elt_index = level->parent_elt_index;

      /* We were at zero -- time to decrement the zero_ref_count val */
      while (parent_level)
        {
	  g_array_index (parent_level->array, SortElt, parent_elt_index).zero_ref_count--;

          parent_elt_index = parent_level->parent_elt_index;
	  parent_level = parent_level->parent_level;
	}

      if (tree_model_sort->root != level)
	tree_model_sort->zero_ref_count--;
    }
}

static void
btk_tree_model_sort_real_unref_node (BtkTreeModel *tree_model,
				     BtkTreeIter  *iter,
				     gboolean      propagate_unref)
{
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *) tree_model;
  SortLevel *level, *parent_level;
  SortElt *elt;
  gint parent_elt_index;

  g_return_if_fail (tree_model_sort->child_model != NULL);
  g_return_if_fail (VALID_ITER (iter, tree_model_sort));

  if (propagate_unref)
    {
      BtkTreeIter child_iter;

      GET_CHILD_ITER (tree_model_sort, &child_iter, iter);
      btk_tree_model_unref_node (tree_model_sort->child_model, &child_iter);
    }

  level = iter->user_data;
  elt = iter->user_data2;

  g_return_if_fail (elt->ref_count > 0);

  elt->ref_count--;
  level->ref_count--;

  /* Decrease the reference count of all parent elements */
  parent_level = level->parent_level;
  parent_elt_index = level->parent_elt_index;

  while (parent_level)
    {
      BtkTreeIter tmp_iter;

      tmp_iter.stamp = tree_model_sort->stamp;
      tmp_iter.user_data = parent_level;
      tmp_iter.user_data2 = &g_array_index (parent_level->array, SortElt, parent_elt_index);

      btk_tree_model_sort_real_unref_node (tree_model, &tmp_iter, FALSE);

      parent_elt_index = parent_level->parent_elt_index;
      parent_level = parent_level->parent_level;
    }

  if (level->ref_count == 0)
    {
      SortLevel *parent_level = level->parent_level;
      gint parent_elt_index = level->parent_elt_index;

      /* We are at zero -- time to increment the zero_ref_count val */
      while (parent_level)
	{
	  g_array_index (parent_level->array, SortElt, parent_elt_index).zero_ref_count++;

	  parent_elt_index = parent_level->parent_elt_index;
	  parent_level = parent_level->parent_level;
	}

      if (tree_model_sort->root != level)
	tree_model_sort->zero_ref_count++;
    }
}

static void
btk_tree_model_sort_unref_node (BtkTreeModel *tree_model,
				BtkTreeIter  *iter)
{
  btk_tree_model_sort_real_unref_node (tree_model, iter, TRUE);
}

/* Sortable interface */
static gboolean
btk_tree_model_sort_get_sort_column_id (BtkTreeSortable *sortable,
					gint            *sort_column_id,
					BtkSortType     *order)
{
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *)sortable;

  if (sort_column_id)
    *sort_column_id = tree_model_sort->sort_column_id;
  if (order)
    *order = tree_model_sort->order;

  if (tree_model_sort->sort_column_id == BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID ||
      tree_model_sort->sort_column_id == BTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID)
    return FALSE;
  
  return TRUE;
}

static void
btk_tree_model_sort_set_sort_column_id (BtkTreeSortable *sortable,
					gint             sort_column_id,
					BtkSortType      order)
{
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *)sortable;

  if (sort_column_id != BTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID)
    {
      if (sort_column_id != BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID)
        {
          BtkTreeDataSortHeader *header = NULL;

          header = _btk_tree_data_list_get_header (tree_model_sort->sort_list,
	  				           sort_column_id);

          /* we want to make sure that we have a function */
          g_return_if_fail (header != NULL);
          g_return_if_fail (header->func != NULL);
        }
      else
        g_return_if_fail (tree_model_sort->default_sort_func != NULL);

      if (tree_model_sort->sort_column_id == sort_column_id)
        {
          if (sort_column_id != BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID)
	    {
	      if (tree_model_sort->order == order)
	        return;
	    }
          else
	    return;
        }
    }

  tree_model_sort->sort_column_id = sort_column_id;
  tree_model_sort->order = order;

  btk_tree_sortable_sort_column_changed (sortable);

  btk_tree_model_sort_sort (tree_model_sort);
}

static void
btk_tree_model_sort_set_sort_func (BtkTreeSortable        *sortable,
				   gint                    sort_column_id,
				   BtkTreeIterCompareFunc  func,
				   gpointer                data,
				   GDestroyNotify          destroy)
{
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *) sortable;

  tree_model_sort->sort_list = _btk_tree_data_list_set_header (tree_model_sort->sort_list,
							       sort_column_id,
							       func, data, destroy);

  if (tree_model_sort->sort_column_id == sort_column_id)
    btk_tree_model_sort_sort (tree_model_sort);
}

static void
btk_tree_model_sort_set_default_sort_func (BtkTreeSortable        *sortable,
					   BtkTreeIterCompareFunc  func,
					   gpointer                data,
					   GDestroyNotify          destroy)
{
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *)sortable;

  if (tree_model_sort->default_sort_destroy)
    {
      GDestroyNotify d = tree_model_sort->default_sort_destroy;

      tree_model_sort->default_sort_destroy = NULL;
      d (tree_model_sort->default_sort_data);
    }

  tree_model_sort->default_sort_func = func;
  tree_model_sort->default_sort_data = data;
  tree_model_sort->default_sort_destroy = destroy;

  if (tree_model_sort->sort_column_id == BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID)
    btk_tree_model_sort_sort (tree_model_sort);
}

static gboolean
btk_tree_model_sort_has_default_sort_func (BtkTreeSortable *sortable)
{
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *)sortable;

  return (tree_model_sort->default_sort_func != NULL);
}

/* DragSource interface */
static gboolean
btk_tree_model_sort_row_draggable (BtkTreeDragSource *drag_source,
                                   BtkTreePath       *path)
{
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *)drag_source;
  BtkTreePath *child_path;
  gboolean draggable;

  child_path = btk_tree_model_sort_convert_path_to_child_path (tree_model_sort,
                                                               path);
  draggable = btk_tree_drag_source_row_draggable (BTK_TREE_DRAG_SOURCE (tree_model_sort->child_model), child_path);
  btk_tree_path_free (child_path);

  return draggable;
}

static gboolean
btk_tree_model_sort_drag_data_get (BtkTreeDragSource *drag_source,
                                   BtkTreePath       *path,
                                   BtkSelectionData  *selection_data)
{
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *)drag_source;
  BtkTreePath *child_path;
  gboolean gotten;

  child_path = btk_tree_model_sort_convert_path_to_child_path (tree_model_sort, path);
  gotten = btk_tree_drag_source_drag_data_get (BTK_TREE_DRAG_SOURCE (tree_model_sort->child_model), child_path, selection_data);
  btk_tree_path_free (child_path);

  return gotten;
}

static gboolean
btk_tree_model_sort_drag_data_delete (BtkTreeDragSource *drag_source,
                                      BtkTreePath       *path)
{
  BtkTreeModelSort *tree_model_sort = (BtkTreeModelSort *)drag_source;
  BtkTreePath *child_path;
  gboolean deleted;

  child_path = btk_tree_model_sort_convert_path_to_child_path (tree_model_sort, path);
  deleted = btk_tree_drag_source_drag_data_delete (BTK_TREE_DRAG_SOURCE (tree_model_sort->child_model), child_path);
  btk_tree_path_free (child_path);

  return deleted;
}

/* sorting code - private */
static gint
btk_tree_model_sort_compare_func (gconstpointer a,
				  gconstpointer b,
				  gpointer      user_data)
{
  SortData *data = (SortData *)user_data;
  BtkTreeModelSort *tree_model_sort = data->tree_model_sort;
  SortTuple *sa = (SortTuple *)a;
  SortTuple *sb = (SortTuple *)b;

  BtkTreeIter iter_a, iter_b;
  gint retval;

  /* shortcut, if we've the same offsets here, they should be equal */
  if (sa->offset == sb->offset)
    return 0;

  if (BTK_TREE_MODEL_SORT_CACHE_CHILD_ITERS (tree_model_sort))
    {
      iter_a = sa->elt->iter;
      iter_b = sb->elt->iter;
    }
  else
    {
      data->parent_path_indices [data->parent_path_depth-1] = sa->elt->offset;
      btk_tree_model_get_iter (BTK_TREE_MODEL (tree_model_sort->child_model), &iter_a, data->parent_path);
      data->parent_path_indices [data->parent_path_depth-1] = sb->elt->offset;
      btk_tree_model_get_iter (BTK_TREE_MODEL (tree_model_sort->child_model), &iter_b, data->parent_path);
    }

  retval = (* data->sort_func) (BTK_TREE_MODEL (tree_model_sort->child_model),
				&iter_a, &iter_b,
				data->sort_data);

  if (tree_model_sort->order == BTK_SORT_DESCENDING)
    {
      if (retval > 0)
	retval = -1;
      else if (retval < 0)
	retval = 1;
    }

  return retval;
}

static gint
btk_tree_model_sort_offset_compare_func (gconstpointer a,
					 gconstpointer b,
					 gpointer      user_data)
{
  gint retval;

  SortTuple *sa = (SortTuple *)a;
  SortTuple *sb = (SortTuple *)b;

  SortData *data = (SortData *)user_data;

  if (sa->elt->offset < sb->elt->offset)
    retval = -1;
  else if (sa->elt->offset > sb->elt->offset)
    retval = 1;
  else
    retval = 0;

  if (data->tree_model_sort->order == BTK_SORT_DESCENDING)
    {
      if (retval > 0)
	retval = -1;
      else if (retval < 0)
	retval = 1;
    }

  return retval;
}

static void
btk_tree_model_sort_sort_level (BtkTreeModelSort *tree_model_sort,
				SortLevel        *level,
				gboolean          recurse,
				gboolean          emit_reordered)
{
  gint i;
  gint ref_offset;
  GArray *sort_array;
  GArray *new_array;
  gint *new_order;

  BtkTreeIter iter;
  BtkTreePath *path;

  SortData data;

  g_return_if_fail (level != NULL);

  if (level->array->len < 1 && !((SortElt *)level->array->data)->children)
    return;

  iter.stamp = tree_model_sort->stamp;
  iter.user_data = level;
  iter.user_data2 = &g_array_index (level->array, SortElt, 0);

  btk_tree_model_sort_ref_node (BTK_TREE_MODEL (tree_model_sort), &iter);
  ref_offset = g_array_index (level->array, SortElt, 0).offset;

  /* Set up data */
  data.tree_model_sort = tree_model_sort;
  if (level->parent_elt_index >= 0)
    {
      data.parent_path = btk_tree_model_sort_elt_get_path (level->parent_level,
							   SORT_LEVEL_PARENT_ELT (level));
      btk_tree_path_append_index (data.parent_path, 0);
    }
  else
    {
      data.parent_path = btk_tree_path_new_first ();
    }
  data.parent_path_depth = btk_tree_path_get_depth (data.parent_path);
  data.parent_path_indices = btk_tree_path_get_indices (data.parent_path);

  /* make the array to be sorted */
  sort_array = g_array_sized_new (FALSE, FALSE, sizeof (SortTuple), level->array->len);
  for (i = 0; i < level->array->len; i++)
    {
      SortTuple tuple;

      tuple.elt = &g_array_index (level->array, SortElt, i);
      tuple.offset = i;

      g_array_append_val (sort_array, tuple);
    }

    if (tree_model_sort->sort_column_id != BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID)
      {
	BtkTreeDataSortHeader *header = NULL;

	header = _btk_tree_data_list_get_header (tree_model_sort->sort_list,
						 tree_model_sort->sort_column_id);

	g_return_if_fail (header != NULL);
	g_return_if_fail (header->func != NULL);

	data.sort_func = header->func;
	data.sort_data = header->data;
      }
    else
      {
	/* absolutely SHOULD NOT happen: */
	g_return_if_fail (tree_model_sort->default_sort_func != NULL);

	data.sort_func = tree_model_sort->default_sort_func;
	data.sort_data = tree_model_sort->default_sort_data;
      }

  if (data.sort_func == NO_SORT_FUNC)
    g_array_sort_with_data (sort_array,
			    btk_tree_model_sort_offset_compare_func,
			    &data);
  else
    g_array_sort_with_data (sort_array,
			    btk_tree_model_sort_compare_func,
			    &data);

  btk_tree_path_free (data.parent_path);

  new_array = g_array_sized_new (FALSE, FALSE, sizeof (SortElt), level->array->len);
  new_order = g_new (gint, level->array->len);

  for (i = 0; i < level->array->len; i++)
    {
      SortElt *elt;

      elt = g_array_index (sort_array, SortTuple, i).elt;
      new_order[i] = g_array_index (sort_array, SortTuple, i).offset;

      g_array_append_val (new_array, *elt);
      if (elt->children)
	elt->children->parent_elt_index = i;
    }

  g_array_free (level->array, TRUE);
  level->array = new_array;
  g_array_free (sort_array, TRUE);

  if (emit_reordered)
    {
      btk_tree_model_sort_increment_stamp (tree_model_sort);
      if (level->parent_elt_index >= 0)
	{
	  iter.stamp = tree_model_sort->stamp;
	  iter.user_data = level->parent_level;
	  iter.user_data2 = SORT_LEVEL_PARENT_ELT (level);

	  path = btk_tree_model_get_path (BTK_TREE_MODEL (tree_model_sort),
					  &iter);

	  btk_tree_model_rows_reordered (BTK_TREE_MODEL (tree_model_sort), path,
					 &iter, new_order);
	}
      else
	{
	  /* toplevel list */
	  path = btk_tree_path_new ();
	  btk_tree_model_rows_reordered (BTK_TREE_MODEL (tree_model_sort), path,
					 NULL, new_order);
	}

      btk_tree_path_free (path);
    }

  /* recurse, if possible */
  if (recurse)
    {
      for (i = 0; i < level->array->len; i++)
	{
	  SortElt *elt = &g_array_index (level->array, SortElt, i);

	  if (elt->children)
	    btk_tree_model_sort_sort_level (tree_model_sort,
					    elt->children,
					    TRUE, emit_reordered);
	}
    }

  g_free (new_order);

  /* get the iter we referenced at the beginning of this function and
   * unref it again
   */
  iter.stamp = tree_model_sort->stamp;
  iter.user_data = level;

  for (i = 0; i < level->array->len; i++)
    {
      if (g_array_index (level->array, SortElt, i).offset == ref_offset)
        {
	  iter.user_data2 = &g_array_index (level->array, SortElt, i);
	  break;
	}
    }

  btk_tree_model_sort_unref_node (BTK_TREE_MODEL (tree_model_sort), &iter);
}

static void
btk_tree_model_sort_sort (BtkTreeModelSort *tree_model_sort)
{
  if (tree_model_sort->sort_column_id == BTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID)
    return;

  if (!tree_model_sort->root)
    return;

  if (tree_model_sort->sort_column_id != BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID)
    {
      BtkTreeDataSortHeader *header = NULL;

      header = _btk_tree_data_list_get_header (tree_model_sort->sort_list,
					       tree_model_sort->sort_column_id);

      /* we want to make sure that we have a function */
      g_return_if_fail (header != NULL);
      g_return_if_fail (header->func != NULL);
    }
  else
    g_return_if_fail (tree_model_sort->default_sort_func != NULL);

  btk_tree_model_sort_sort_level (tree_model_sort, tree_model_sort->root,
				  TRUE, TRUE);
}

/* signal helpers */
static gint
btk_tree_model_sort_level_find_insert (BtkTreeModelSort *tree_model_sort,
				       SortLevel        *level,
				       BtkTreeIter      *iter,
				       gint             skip_index)
{
  gint start, middle, end;
  gint cmp;
  SortElt *tmp_elt;
  BtkTreeIter tmp_iter;

  BtkTreeIterCompareFunc func;
  gpointer data;

  if (tree_model_sort->sort_column_id != BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID)
    {
      BtkTreeDataSortHeader *header;
      
      header = _btk_tree_data_list_get_header (tree_model_sort->sort_list,
					       tree_model_sort->sort_column_id);
      
      g_return_val_if_fail (header != NULL, 0);
      
      func = header->func;
      data = header->data;
    }
  else
    {
      func = tree_model_sort->default_sort_func;
      data = tree_model_sort->default_sort_data;
      
      g_return_val_if_fail (func != NO_SORT_FUNC, 0);
    }

  g_return_val_if_fail (func != NULL, 0);

  start = 0;
  end = level->array->len;
  if (skip_index < 0)
    skip_index = end;
  else
    end--;

  if (start == end)
    return 0;
  
  while (start != end)
    {
      middle = (start + end) / 2;

      if (middle < skip_index)
	tmp_elt = &(g_array_index (level->array, SortElt, middle));
      else
	tmp_elt = &(g_array_index (level->array, SortElt, middle + 1));
  
      if (!BTK_TREE_MODEL_SORT_CACHE_CHILD_ITERS (tree_model_sort))
	{
	  BtkTreePath *path = btk_tree_model_sort_elt_get_path (level, tmp_elt);
	  btk_tree_model_get_iter (tree_model_sort->child_model,
				   &tmp_iter, path);
	  btk_tree_path_free (path);
	}
      else
	tmp_iter = tmp_elt->iter;
  
      if (tree_model_sort->order == BTK_SORT_ASCENDING)
	cmp = (* func) (BTK_TREE_MODEL (tree_model_sort->child_model),
			&tmp_iter, iter, data);
      else
	cmp = (* func) (BTK_TREE_MODEL (tree_model_sort->child_model),
			iter, &tmp_iter, data);

      if (cmp <= 0)
	start = middle + 1;
      else
	end = middle;
    }

  if (cmp <= 0)
    return middle + 1;
  else
    return middle;
}

static gboolean
btk_tree_model_sort_insert_value (BtkTreeModelSort *tree_model_sort,
				  SortLevel        *level,
				  BtkTreePath      *s_path,
				  BtkTreeIter      *s_iter)
{
  gint offset, index, i;

  SortElt elt;
  SortElt *tmp_elt;

  offset = btk_tree_path_get_indices (s_path)[btk_tree_path_get_depth (s_path) - 1];

  if (BTK_TREE_MODEL_SORT_CACHE_CHILD_ITERS (tree_model_sort))
    elt.iter = *s_iter;
  elt.offset = offset;
  elt.zero_ref_count = 0;
  elt.ref_count = 0;
  elt.children = NULL;

  /* update all larger offsets */
  tmp_elt = SORT_ELT (level->array->data);
  for (i = 0; i < level->array->len; i++, tmp_elt++)
    if (tmp_elt->offset >= offset)
      tmp_elt->offset++;

  if (tree_model_sort->sort_column_id == BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID &&
      tree_model_sort->default_sort_func == NO_SORT_FUNC)
    index = offset;
  else
    index = btk_tree_model_sort_level_find_insert (tree_model_sort,
                                                   level, s_iter,
                                                   -1);

  g_array_insert_vals (level->array, index, &elt, 1);
  tmp_elt = SORT_ELT (level->array->data);
  for (i = 0; i < level->array->len; i++, tmp_elt++)
    if (tmp_elt->children)
      tmp_elt->children->parent_elt_index = i;

  return TRUE;
}

/* sort elt stuff */
static BtkTreePath *
btk_tree_model_sort_elt_get_path (SortLevel *level,
				  SortElt *elt)
{
  SortLevel *walker = level;
  SortElt *walker2 = elt;
  BtkTreePath *path;

  g_return_val_if_fail (level != NULL, NULL);
  g_return_val_if_fail (elt != NULL, NULL);

  path = btk_tree_path_new ();

  while (walker)
    {
      btk_tree_path_prepend_index (path, walker2->offset);

      if (!walker->parent_level)
	break;

      walker2 = SORT_LEVEL_PARENT_ELT (walker);
      walker = walker->parent_level;
    }

  return path;
}

/**
 * btk_tree_model_sort_set_model:
 * @tree_model_sort: The #BtkTreeModelSort.
 * @child_model: (allow-none): A #BtkTreeModel, or %NULL.
 *
 * Sets the model of @tree_model_sort to be @model.  If @model is %NULL, 
 * then the old model is unset.  The sort function is unset as a result 
 * of this call. The model will be in an unsorted state until a sort 
 * function is set.
 **/
static void
btk_tree_model_sort_set_model (BtkTreeModelSort *tree_model_sort,
                               BtkTreeModel     *child_model)
{
  if (child_model)
    g_object_ref (child_model);

  if (tree_model_sort->child_model)
    {
      g_signal_handler_disconnect (tree_model_sort->child_model,
                                   tree_model_sort->changed_id);
      g_signal_handler_disconnect (tree_model_sort->child_model,
                                   tree_model_sort->inserted_id);
      g_signal_handler_disconnect (tree_model_sort->child_model,
                                   tree_model_sort->has_child_toggled_id);
      g_signal_handler_disconnect (tree_model_sort->child_model,
                                   tree_model_sort->deleted_id);
      g_signal_handler_disconnect (tree_model_sort->child_model,
				   tree_model_sort->reordered_id);

      /* reset our state */
      if (tree_model_sort->root)
	btk_tree_model_sort_free_level (tree_model_sort, tree_model_sort->root);
      tree_model_sort->root = NULL;
      _btk_tree_data_list_header_free (tree_model_sort->sort_list);
      tree_model_sort->sort_list = NULL;
      g_object_unref (tree_model_sort->child_model);
    }

  tree_model_sort->child_model = child_model;

  if (child_model)
    {
      GType *types;
      gint i, n_columns;

      tree_model_sort->changed_id =
        g_signal_connect (child_model, "row-changed",
                          G_CALLBACK (btk_tree_model_sort_row_changed),
                          tree_model_sort);
      tree_model_sort->inserted_id =
        g_signal_connect (child_model, "row-inserted",
                          G_CALLBACK (btk_tree_model_sort_row_inserted),
                          tree_model_sort);
      tree_model_sort->has_child_toggled_id =
        g_signal_connect (child_model, "row-has-child-toggled",
                          G_CALLBACK (btk_tree_model_sort_row_has_child_toggled),
                          tree_model_sort);
      tree_model_sort->deleted_id =
        g_signal_connect (child_model, "row-deleted",
                          G_CALLBACK (btk_tree_model_sort_row_deleted),
                          tree_model_sort);
      tree_model_sort->reordered_id =
	g_signal_connect (child_model, "rows-reordered",
			  G_CALLBACK (btk_tree_model_sort_rows_reordered),
			  tree_model_sort);

      tree_model_sort->child_flags = btk_tree_model_get_flags (child_model);
      n_columns = btk_tree_model_get_n_columns (child_model);

      types = g_new (GType, n_columns);
      for (i = 0; i < n_columns; i++)
        types[i] = btk_tree_model_get_column_type (child_model, i);

      tree_model_sort->sort_list = _btk_tree_data_list_header_new (n_columns, types);
      g_free (types);

      tree_model_sort->default_sort_func = NO_SORT_FUNC;
      tree_model_sort->stamp = g_random_int ();
    }
}

/**
 * btk_tree_model_sort_get_model:
 * @tree_model: a #BtkTreeModelSort
 *
 * Returns the model the #BtkTreeModelSort is sorting.
 *
 * Return value: (transfer none): the "child model" being sorted
 **/
BtkTreeModel *
btk_tree_model_sort_get_model (BtkTreeModelSort *tree_model)
{
  g_return_val_if_fail (BTK_IS_TREE_MODEL_SORT (tree_model), NULL);

  return tree_model->child_model;
}


static BtkTreePath *
btk_real_tree_model_sort_convert_child_path_to_path (BtkTreeModelSort *tree_model_sort,
						     BtkTreePath      *child_path,
						     gboolean          build_levels)
{
  gint *child_indices;
  BtkTreePath *retval;
  SortLevel *level;
  gint i;

  g_return_val_if_fail (tree_model_sort->child_model != NULL, NULL);
  g_return_val_if_fail (child_path != NULL, NULL);

  retval = btk_tree_path_new ();
  child_indices = btk_tree_path_get_indices (child_path);

  if (tree_model_sort->root == NULL && build_levels)
    btk_tree_model_sort_build_level (tree_model_sort, NULL, -1);
  level = SORT_LEVEL (tree_model_sort->root);

  for (i = 0; i < btk_tree_path_get_depth (child_path); i++)
    {
      gint j;
      gboolean found_child = FALSE;

      if (!level)
	{
	  btk_tree_path_free (retval);
	  return NULL;
	}

      if (child_indices[i] >= level->array->len)
	{
	  btk_tree_path_free (retval);
	  return NULL;
	}
      for (j = 0; j < level->array->len; j++)
	{
	  if ((g_array_index (level->array, SortElt, j)).offset == child_indices[i])
	    {
	      btk_tree_path_append_index (retval, j);
	      if (g_array_index (level->array, SortElt, j).children == NULL && build_levels)
		{
		  btk_tree_model_sort_build_level (tree_model_sort, level, j);
		}
	      level = g_array_index (level->array, SortElt, j).children;
	      found_child = TRUE;
	      break;
	    }
	}
      if (! found_child)
	{
	  btk_tree_path_free (retval);
	  return NULL;
	}
    }

  return retval;
}


/**
 * btk_tree_model_sort_convert_child_path_to_path:
 * @tree_model_sort: A #BtkTreeModelSort
 * @child_path: A #BtkTreePath to convert
 * 
 * Converts @child_path to a path relative to @tree_model_sort.  That is,
 * @child_path points to a path in the child model.  The returned path will
 * point to the same row in the sorted model.  If @child_path isn't a valid 
 * path on the child model, then %NULL is returned.
 * 
 * Return value: A newly allocated #BtkTreePath, or %NULL
 **/
BtkTreePath *
btk_tree_model_sort_convert_child_path_to_path (BtkTreeModelSort *tree_model_sort,
						BtkTreePath      *child_path)
{
  g_return_val_if_fail (BTK_IS_TREE_MODEL_SORT (tree_model_sort), NULL);
  g_return_val_if_fail (tree_model_sort->child_model != NULL, NULL);
  g_return_val_if_fail (child_path != NULL, NULL);

  return btk_real_tree_model_sort_convert_child_path_to_path (tree_model_sort, child_path, TRUE);
}

/**
 * btk_tree_model_sort_convert_child_iter_to_iter:
 * @tree_model_sort: A #BtkTreeModelSort
 * @sort_iter: (out): An uninitialized #BtkTreeIter.
 * @child_iter: A valid #BtkTreeIter pointing to a row on the child model
 * 
 * Sets @sort_iter to point to the row in @tree_model_sort that corresponds to
 * the row pointed at by @child_iter.  If @sort_iter was not set, %FALSE
 * is returned.  Note: a boolean is only returned since 2.14.
 *
 * Return value: %TRUE, if @sort_iter was set, i.e. if @sort_iter is a
 * valid iterator pointer to a visible row in the child model.
 **/
gboolean
btk_tree_model_sort_convert_child_iter_to_iter (BtkTreeModelSort *tree_model_sort,
						BtkTreeIter      *sort_iter,
						BtkTreeIter      *child_iter)
{
  gboolean ret;
  BtkTreePath *child_path, *path;

  g_return_val_if_fail (BTK_IS_TREE_MODEL_SORT (tree_model_sort), FALSE);
  g_return_val_if_fail (tree_model_sort->child_model != NULL, FALSE);
  g_return_val_if_fail (sort_iter != NULL, FALSE);
  g_return_val_if_fail (child_iter != NULL, FALSE);
  g_return_val_if_fail (sort_iter != child_iter, FALSE);

  sort_iter->stamp = 0;

  child_path = btk_tree_model_get_path (tree_model_sort->child_model, child_iter);
  g_return_val_if_fail (child_path != NULL, FALSE);

  path = btk_tree_model_sort_convert_child_path_to_path (tree_model_sort, child_path);
  btk_tree_path_free (child_path);

  if (!path)
    {
      g_warning ("%s: The conversion of the child path to a BtkTreeModel sort path failed", G_STRLOC);
      return FALSE;
    }

  ret = btk_tree_model_get_iter (BTK_TREE_MODEL (tree_model_sort),
                                 sort_iter, path);
  btk_tree_path_free (path);

  return ret;
}

/**
 * btk_tree_model_sort_convert_path_to_child_path:
 * @tree_model_sort: A #BtkTreeModelSort
 * @sorted_path: A #BtkTreePath to convert
 * 
 * Converts @sorted_path to a path on the child model of @tree_model_sort.  
 * That is, @sorted_path points to a location in @tree_model_sort.  The 
 * returned path will point to the same location in the model not being 
 * sorted.  If @sorted_path does not point to a location in the child model, 
 * %NULL is returned.
 * 
 * Return value: A newly allocated #BtkTreePath, or %NULL
 **/
BtkTreePath *
btk_tree_model_sort_convert_path_to_child_path (BtkTreeModelSort *tree_model_sort,
						BtkTreePath      *sorted_path)
{
  gint *sorted_indices;
  BtkTreePath *retval;
  SortLevel *level;
  gint i;

  g_return_val_if_fail (BTK_IS_TREE_MODEL_SORT (tree_model_sort), NULL);
  g_return_val_if_fail (tree_model_sort->child_model != NULL, NULL);
  g_return_val_if_fail (sorted_path != NULL, NULL);

  retval = btk_tree_path_new ();
  sorted_indices = btk_tree_path_get_indices (sorted_path);
  if (tree_model_sort->root == NULL)
    btk_tree_model_sort_build_level (tree_model_sort, NULL, -1);
  level = SORT_LEVEL (tree_model_sort->root);

  for (i = 0; i < btk_tree_path_get_depth (sorted_path); i++)
    {
      gint count = sorted_indices[i];

      if ((level == NULL) ||
	  (level->array->len <= count))
	{
	  btk_tree_path_free (retval);
	  return NULL;
	}

      if (g_array_index (level->array, SortElt, count).children == NULL)
	btk_tree_model_sort_build_level (tree_model_sort, level, count);

      if (level == NULL)
        {
	  btk_tree_path_free (retval);
	  break;
	}

      btk_tree_path_append_index (retval, g_array_index (level->array, SortElt, count).offset);
      level = g_array_index (level->array, SortElt, count).children;
    }
 
  return retval;
}

/**
 * btk_tree_model_sort_convert_iter_to_child_iter:
 * @tree_model_sort: A #BtkTreeModelSort
 * @child_iter: (out): An uninitialized #BtkTreeIter
 * @sorted_iter: A valid #BtkTreeIter pointing to a row on @tree_model_sort.
 * 
 * Sets @child_iter to point to the row pointed to by @sorted_iter.
 **/
void
btk_tree_model_sort_convert_iter_to_child_iter (BtkTreeModelSort *tree_model_sort,
						BtkTreeIter      *child_iter,
						BtkTreeIter      *sorted_iter)
{
  g_return_if_fail (BTK_IS_TREE_MODEL_SORT (tree_model_sort));
  g_return_if_fail (tree_model_sort->child_model != NULL);
  g_return_if_fail (child_iter != NULL);
  g_return_if_fail (VALID_ITER (sorted_iter, tree_model_sort));
  g_return_if_fail (sorted_iter != child_iter);

  if (BTK_TREE_MODEL_SORT_CACHE_CHILD_ITERS (tree_model_sort))
    {
      *child_iter = SORT_ELT (sorted_iter->user_data2)->iter;
    }
  else
    {
      BtkTreePath *path;

      path = btk_tree_model_sort_elt_get_path (sorted_iter->user_data,
					       sorted_iter->user_data2);
      btk_tree_model_get_iter (tree_model_sort->child_model, child_iter, path);
      btk_tree_path_free (path);
    }
}

static void
btk_tree_model_sort_build_level (BtkTreeModelSort *tree_model_sort,
				 SortLevel        *parent_level,
				 gint              parent_elt_index)
{
  BtkTreeIter iter;
  SortElt *parent_elt = NULL;
  SortLevel *new_level;
  gint length = 0;
  gint i;

  g_assert (tree_model_sort->child_model != NULL);

  if (parent_level == NULL)
    {
      if (btk_tree_model_get_iter_first (tree_model_sort->child_model, &iter) == FALSE)
	return;
      length = btk_tree_model_iter_n_children (tree_model_sort->child_model, NULL);
    }
  else
    {
      BtkTreeIter parent_iter;
      BtkTreeIter child_parent_iter;

      parent_elt = &g_array_index (parent_level->array, SortElt, parent_elt_index);

      parent_iter.stamp = tree_model_sort->stamp;
      parent_iter.user_data = parent_level;
      parent_iter.user_data2 = parent_elt;

      btk_tree_model_sort_convert_iter_to_child_iter (tree_model_sort,
						      &child_parent_iter,
						      &parent_iter);
      if (btk_tree_model_iter_children (tree_model_sort->child_model,
					&iter,
					&child_parent_iter) == FALSE)
	return;

      /* stamp may have changed */
      btk_tree_model_sort_convert_iter_to_child_iter (tree_model_sort,
						      &child_parent_iter,
						      &parent_iter);

      length = btk_tree_model_iter_n_children (tree_model_sort->child_model, &child_parent_iter);
    }

  g_return_if_fail (length > 0);

  new_level = g_new (SortLevel, 1);
  new_level->array = g_array_sized_new (FALSE, FALSE, sizeof (SortElt), length);
  new_level->ref_count = 0;
  new_level->parent_level = parent_level;
  new_level->parent_elt_index = parent_elt_index;

  if (parent_elt_index >= 0)
    parent_elt->children = new_level;
  else
    tree_model_sort->root = new_level;

  /* increase the count of zero ref_counts.*/
  while (parent_level)
    {
      g_array_index (parent_level->array, SortElt, parent_elt_index).zero_ref_count++;

      parent_elt_index = parent_level->parent_elt_index;
      parent_level = parent_level->parent_level;
    }

  if (new_level != tree_model_sort->root)
    tree_model_sort->zero_ref_count++;

  for (i = 0; i < length; i++)
    {
      SortElt sort_elt;
      sort_elt.offset = i;
      sort_elt.zero_ref_count = 0;
      sort_elt.ref_count = 0;
      sort_elt.children = NULL;

      if (BTK_TREE_MODEL_SORT_CACHE_CHILD_ITERS (tree_model_sort))
	{
	  sort_elt.iter = iter;
	  if (btk_tree_model_iter_next (tree_model_sort->child_model, &iter) == FALSE &&
	      i < length - 1)
	    {
	      if (parent_level)
	        {
	          BtkTreePath *level;
		  gchar *str;

		  level = btk_tree_model_sort_elt_get_path (parent_level,
							    parent_elt);
		  str = btk_tree_path_to_string (level);
		  btk_tree_path_free (level);

		  g_warning ("%s: There is a discrepancy between the sort model "
			     "and the child model.  The child model is "
			     "advertising a wrong length for level %s:.",
			     G_STRLOC, str);
		  g_free (str);
		}
	      else
	        {
		  g_warning ("%s: There is a discrepancy between the sort model "
			     "and the child model.  The child model is "
			     "advertising a wrong length for the root level.",
			     G_STRLOC);
		}

	      return;
	    }
	}
      g_array_append_val (new_level->array, sort_elt);
    }

  /* sort level */
  btk_tree_model_sort_sort_level (tree_model_sort, new_level,
				  FALSE, FALSE);
}

static void
btk_tree_model_sort_free_level (BtkTreeModelSort *tree_model_sort,
				SortLevel        *sort_level)
{
  gint i;

  g_assert (sort_level);

  for (i = 0; i < sort_level->array->len; i++)
    {
      if (g_array_index (sort_level->array, SortElt, i).children)
	btk_tree_model_sort_free_level (tree_model_sort,
					SORT_LEVEL (g_array_index (sort_level->array, SortElt, i).children));
    }

  if (sort_level->ref_count == 0)
    {
      SortLevel *parent_level = sort_level->parent_level;
      gint parent_elt_index = sort_level->parent_elt_index;

      while (parent_level)
        {
	  g_array_index (parent_level->array, SortElt, parent_elt_index).zero_ref_count--;

          parent_elt_index = parent_level->parent_elt_index;
	  parent_level = parent_level->parent_level;
	}

      if (sort_level != tree_model_sort->root)
	tree_model_sort->zero_ref_count--;
    }

  if (sort_level->parent_elt_index >= 0)
    SORT_LEVEL_PARENT_ELT (sort_level)->children = NULL;
  else
    tree_model_sort->root = NULL;

  g_array_free (sort_level->array, TRUE);
  sort_level->array = NULL;

  g_free (sort_level);
  sort_level = NULL;
}

static void
btk_tree_model_sort_increment_stamp (BtkTreeModelSort *tree_model_sort)
{
  do
    {
      tree_model_sort->stamp++;
    }
  while (tree_model_sort->stamp == 0);

  btk_tree_model_sort_clear_cache (tree_model_sort);
}

static void
btk_tree_model_sort_clear_cache_helper (BtkTreeModelSort *tree_model_sort,
					SortLevel        *level)
{
  gint i;

  g_assert (level != NULL);

  for (i = 0; i < level->array->len; i++)
    {
      if (g_array_index (level->array, SortElt, i).zero_ref_count > 0)
	btk_tree_model_sort_clear_cache_helper (tree_model_sort, g_array_index (level->array, SortElt, i).children);
    }

  if (level->ref_count == 0 && level != tree_model_sort->root)
    btk_tree_model_sort_free_level (tree_model_sort, level);
}

/**
 * btk_tree_model_sort_reset_default_sort_func:
 * @tree_model_sort: A #BtkTreeModelSort
 * 
 * This resets the default sort function to be in the 'unsorted' state.  That
 * is, it is in the same order as the child model. It will re-sort the model
 * to be in the same order as the child model only if the #BtkTreeModelSort
 * is in 'unsorted' state.
 **/
void
btk_tree_model_sort_reset_default_sort_func (BtkTreeModelSort *tree_model_sort)
{
  g_return_if_fail (BTK_IS_TREE_MODEL_SORT (tree_model_sort));

  if (tree_model_sort->default_sort_destroy)
    {
      GDestroyNotify d = tree_model_sort->default_sort_destroy;

      tree_model_sort->default_sort_destroy = NULL;
      d (tree_model_sort->default_sort_data);
    }

  tree_model_sort->default_sort_func = NO_SORT_FUNC;
  tree_model_sort->default_sort_data = NULL;
  tree_model_sort->default_sort_destroy = NULL;

  if (tree_model_sort->sort_column_id == BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID)
    btk_tree_model_sort_sort (tree_model_sort);
  tree_model_sort->sort_column_id = BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID;
}

/**
 * btk_tree_model_sort_clear_cache:
 * @tree_model_sort: A #BtkTreeModelSort
 * 
 * This function should almost never be called.  It clears the @tree_model_sort
 * of any cached iterators that haven't been reffed with
 * btk_tree_model_ref_node().  This might be useful if the child model being
 * sorted is static (and doesn't change often) and there has been a lot of
 * unreffed access to nodes.  As a side effect of this function, all unreffed
 * iters will be invalid.
 **/
void
btk_tree_model_sort_clear_cache (BtkTreeModelSort *tree_model_sort)
{
  g_return_if_fail (BTK_IS_TREE_MODEL_SORT (tree_model_sort));

  if (tree_model_sort->zero_ref_count > 0)
    btk_tree_model_sort_clear_cache_helper (tree_model_sort, (SortLevel *)tree_model_sort->root);
}

static gboolean
btk_tree_model_sort_iter_is_valid_helper (BtkTreeIter *iter,
					  SortLevel   *level)
{
  gint i;

  for (i = 0; i < level->array->len; i++)
    {
      SortElt *elt = &g_array_index (level->array, SortElt, i);

      if (iter->user_data == level && iter->user_data2 == elt)
	return TRUE;

      if (elt->children)
	if (btk_tree_model_sort_iter_is_valid_helper (iter, elt->children))
	  return TRUE;
    }

  return FALSE;
}

/**
 * btk_tree_model_sort_iter_is_valid:
 * @tree_model_sort: A #BtkTreeModelSort.
 * @iter: A #BtkTreeIter.
 *
 * <warning><para>
 * This function is slow. Only use it for debugging and/or testing purposes.
 * </para></warning>
 *
 * Checks if the given iter is a valid iter for this #BtkTreeModelSort.
 *
 * Return value: %TRUE if the iter is valid, %FALSE if the iter is invalid.
 *
 * Since: 2.2
 **/
gboolean
btk_tree_model_sort_iter_is_valid (BtkTreeModelSort *tree_model_sort,
                                   BtkTreeIter      *iter)
{
  g_return_val_if_fail (BTK_IS_TREE_MODEL_SORT (tree_model_sort), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  if (!VALID_ITER (iter, tree_model_sort))
    return FALSE;

  return btk_tree_model_sort_iter_is_valid_helper (iter,
						   tree_model_sort->root);
}

#define __BTK_TREE_MODEL_SORT_C__
#include "btkaliasdef.c"
