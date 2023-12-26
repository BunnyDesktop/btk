/* btktreemodelsort.h
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

#ifndef __BTK_TREE_MODEL_SORT_H__
#define __BTK_TREE_MODEL_SORT_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bdkconfig.h>
#include <btk/btktreemodel.h>
#include <btk/btktreesortable.h>

B_BEGIN_DECLS

#define BTK_TYPE_TREE_MODEL_SORT			(btk_tree_model_sort_get_type ())
#define BTK_TREE_MODEL_SORT(obj)			(B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TREE_MODEL_SORT, BtkTreeModelSort))
#define BTK_TREE_MODEL_SORT_CLASS(klass)		(B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TREE_MODEL_SORT, BtkTreeModelSortClass))
#define BTK_IS_TREE_MODEL_SORT(obj)			(B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TREE_MODEL_SORT))
#define BTK_IS_TREE_MODEL_SORT_CLASS(klass)		(B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TREE_MODEL_SORT))
#define BTK_TREE_MODEL_SORT_GET_CLASS(obj)		(B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TREE_MODEL_SORT, BtkTreeModelSortClass))

typedef struct _BtkTreeModelSort       BtkTreeModelSort;
typedef struct _BtkTreeModelSortClass  BtkTreeModelSortClass;

struct _BtkTreeModelSort
{
  BObject parent;

  /* < private > */
  bpointer GSEAL (root);
  bint GSEAL (stamp);
  buint GSEAL (child_flags);
  BtkTreeModel *GSEAL (child_model);
  bint GSEAL (zero_ref_count);

  /* sort information */
  GList *GSEAL (sort_list);
  bint GSEAL (sort_column_id);
  BtkSortType GSEAL (order);

  /* default sort */
  BtkTreeIterCompareFunc GSEAL (default_sort_func);
  bpointer GSEAL (default_sort_data);
  GDestroyNotify GSEAL (default_sort_destroy);

  /* signal ids */
  buint GSEAL (changed_id);
  buint GSEAL (inserted_id);
  buint GSEAL (has_child_toggled_id);
  buint GSEAL (deleted_id);
  buint GSEAL (reordered_id);
};

struct _BtkTreeModelSortClass
{
  BObjectClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType         btk_tree_model_sort_get_type                   (void) B_GNUC_CONST;
BtkTreeModel *btk_tree_model_sort_new_with_model             (BtkTreeModel     *child_model);

BtkTreeModel *btk_tree_model_sort_get_model                  (BtkTreeModelSort *tree_model);
BtkTreePath  *btk_tree_model_sort_convert_child_path_to_path (BtkTreeModelSort *tree_model_sort,
							      BtkTreePath      *child_path);
bboolean      btk_tree_model_sort_convert_child_iter_to_iter (BtkTreeModelSort *tree_model_sort,
							      BtkTreeIter      *sort_iter,
							      BtkTreeIter      *child_iter);
BtkTreePath  *btk_tree_model_sort_convert_path_to_child_path (BtkTreeModelSort *tree_model_sort,
							      BtkTreePath      *sorted_path);
void          btk_tree_model_sort_convert_iter_to_child_iter (BtkTreeModelSort *tree_model_sort,
							      BtkTreeIter      *child_iter,
							      BtkTreeIter      *sorted_iter);
void          btk_tree_model_sort_reset_default_sort_func    (BtkTreeModelSort *tree_model_sort);
void          btk_tree_model_sort_clear_cache                (BtkTreeModelSort *tree_model_sort);
bboolean      btk_tree_model_sort_iter_is_valid              (BtkTreeModelSort *tree_model_sort,
                                                              BtkTreeIter      *iter);


B_END_DECLS

#endif /* __BTK_TREE_MODEL_SORT_H__ */
