/* btktreemodel.h
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

#ifndef __BTK_TREE_MODEL_H__
#define __BTK_TREE_MODEL_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bunnylib-object.h>

/* Not needed, retained for compatibility -Yosh */
#include <btk/btkobject.h>

B_BEGIN_DECLS

#define BTK_TYPE_TREE_MODEL            (btk_tree_model_get_type ())
#define BTK_TREE_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TREE_MODEL, BtkTreeModel))
#define BTK_IS_TREE_MODEL(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TREE_MODEL))
#define BTK_TREE_MODEL_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BTK_TYPE_TREE_MODEL, BtkTreeModelIface))

#define BTK_TYPE_TREE_ITER             (btk_tree_iter_get_type ())
#define BTK_TYPE_TREE_PATH             (btk_tree_path_get_type ())
#define BTK_TYPE_TREE_ROW_REFERENCE    (btk_tree_row_reference_get_type ())

typedef struct _BtkTreeIter         BtkTreeIter;
typedef struct _BtkTreePath         BtkTreePath;
typedef struct _BtkTreeRowReference BtkTreeRowReference;
typedef struct _BtkTreeModel        BtkTreeModel; /* Dummy typedef */
typedef struct _BtkTreeModelIface   BtkTreeModelIface;
typedef gboolean (* BtkTreeModelForeachFunc) (BtkTreeModel *model, BtkTreePath *path, BtkTreeIter *iter, gpointer data);


typedef enum
{
  BTK_TREE_MODEL_ITERS_PERSIST = 1 << 0,
  BTK_TREE_MODEL_LIST_ONLY = 1 << 1
} BtkTreeModelFlags;

struct _BtkTreeIter
{
  gint stamp;
  gpointer user_data;
  gpointer user_data2;
  gpointer user_data3;
};

struct _BtkTreeModelIface
{
  GTypeInterface g_iface;

  /* Signals */
  void         (* row_changed)           (BtkTreeModel *tree_model,
					  BtkTreePath  *path,
					  BtkTreeIter  *iter);
  void         (* row_inserted)          (BtkTreeModel *tree_model,
					  BtkTreePath  *path,
					  BtkTreeIter  *iter);
  void         (* row_has_child_toggled) (BtkTreeModel *tree_model,
					  BtkTreePath  *path,
					  BtkTreeIter  *iter);
  void         (* row_deleted)           (BtkTreeModel *tree_model,
					  BtkTreePath  *path);
  void         (* rows_reordered)        (BtkTreeModel *tree_model,
					  BtkTreePath  *path,
					  BtkTreeIter  *iter,
					  gint         *new_order);

  /* Virtual Table */
  BtkTreeModelFlags (* get_flags)  (BtkTreeModel *tree_model);

  gint         (* get_n_columns)   (BtkTreeModel *tree_model);
  GType        (* get_column_type) (BtkTreeModel *tree_model,
				    gint          index_);
  gboolean     (* get_iter)        (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter,
				    BtkTreePath  *path);
  BtkTreePath *(* get_path)        (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter);
  void         (* get_value)       (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter,
				    gint          column,
				    GValue       *value);
  gboolean     (* iter_next)       (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter);
  gboolean     (* iter_children)   (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter,
				    BtkTreeIter  *parent);
  gboolean     (* iter_has_child)  (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter);
  gint         (* iter_n_children) (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter);
  gboolean     (* iter_nth_child)  (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter,
				    BtkTreeIter  *parent,
				    gint          n);
  gboolean     (* iter_parent)     (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter,
				    BtkTreeIter  *child);
  void         (* ref_node)        (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter);
  void         (* unref_node)      (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter);
};


/* BtkTreePath operations */
BtkTreePath *btk_tree_path_new              (void);
BtkTreePath *btk_tree_path_new_from_string  (const gchar       *path);
BtkTreePath *btk_tree_path_new_from_indices (gint               first_index,
					     ...);
gchar       *btk_tree_path_to_string        (BtkTreePath       *path);
BtkTreePath *btk_tree_path_new_first        (void);
void         btk_tree_path_append_index     (BtkTreePath       *path,
					     gint               index_);
void         btk_tree_path_prepend_index    (BtkTreePath       *path,
					     gint               index_);
gint         btk_tree_path_get_depth        (BtkTreePath       *path);
gint        *btk_tree_path_get_indices      (BtkTreePath       *path);
gint        *btk_tree_path_get_indices_with_depth (BtkTreePath *path,
                                                   gint        *depth);
void         btk_tree_path_free             (BtkTreePath       *path);
BtkTreePath *btk_tree_path_copy             (const BtkTreePath *path);
GType        btk_tree_path_get_type         (void) B_GNUC_CONST;
gint         btk_tree_path_compare          (const BtkTreePath *a,
					     const BtkTreePath *b);
void         btk_tree_path_next             (BtkTreePath       *path);
gboolean     btk_tree_path_prev             (BtkTreePath       *path);
gboolean     btk_tree_path_up               (BtkTreePath       *path);
void         btk_tree_path_down             (BtkTreePath       *path);

gboolean     btk_tree_path_is_ancestor      (BtkTreePath       *path,
                                             BtkTreePath       *descendant);
gboolean     btk_tree_path_is_descendant    (BtkTreePath       *path,
                                             BtkTreePath       *ancestor);

#ifndef BTK_DISABLE_DEPRECATED
#define btk_tree_path_new_root() btk_tree_path_new_first()
#endif /* !BTK_DISABLE_DEPRECATED */

/* Row reference (an object that tracks model changes so it refers to the same
 * row always; a path refers to a position, not a fixed row).  You almost always
 * want to call btk_tree_row_reference_new.
 */

GType                btk_tree_row_reference_get_type (void) B_GNUC_CONST;
BtkTreeRowReference *btk_tree_row_reference_new       (BtkTreeModel        *model,
						       BtkTreePath         *path);
BtkTreeRowReference *btk_tree_row_reference_new_proxy (GObject             *proxy,
						       BtkTreeModel        *model,
						       BtkTreePath         *path);
BtkTreePath         *btk_tree_row_reference_get_path  (BtkTreeRowReference *reference);
BtkTreeModel        *btk_tree_row_reference_get_model (BtkTreeRowReference *reference);
gboolean             btk_tree_row_reference_valid     (BtkTreeRowReference *reference);
BtkTreeRowReference *btk_tree_row_reference_copy      (BtkTreeRowReference *reference);
void                 btk_tree_row_reference_free      (BtkTreeRowReference *reference);
/* These two functions are only needed if you created the row reference with a
 * proxy object */
void                 btk_tree_row_reference_inserted  (GObject     *proxy,
						       BtkTreePath *path);
void                 btk_tree_row_reference_deleted   (GObject     *proxy,
						       BtkTreePath *path);
void                 btk_tree_row_reference_reordered (GObject     *proxy,
						       BtkTreePath *path,
						       BtkTreeIter *iter,
						       gint        *new_order);

/* BtkTreeIter operations */
BtkTreeIter *     btk_tree_iter_copy             (BtkTreeIter  *iter);
void              btk_tree_iter_free             (BtkTreeIter  *iter);
GType             btk_tree_iter_get_type         (void) B_GNUC_CONST;

GType             btk_tree_model_get_type        (void) B_GNUC_CONST;
BtkTreeModelFlags btk_tree_model_get_flags       (BtkTreeModel *tree_model);
gint              btk_tree_model_get_n_columns   (BtkTreeModel *tree_model);
GType             btk_tree_model_get_column_type (BtkTreeModel *tree_model,
						  gint          index_);


/* Iterator movement */
gboolean          btk_tree_model_get_iter        (BtkTreeModel *tree_model,
						  BtkTreeIter  *iter,
						  BtkTreePath  *path);
gboolean          btk_tree_model_get_iter_from_string (BtkTreeModel *tree_model,
						       BtkTreeIter  *iter,
						       const gchar  *path_string);
gchar *           btk_tree_model_get_string_from_iter (BtkTreeModel *tree_model,
                                                       BtkTreeIter  *iter);
gboolean          btk_tree_model_get_iter_first  (BtkTreeModel *tree_model,
						  BtkTreeIter  *iter);
BtkTreePath *     btk_tree_model_get_path        (BtkTreeModel *tree_model,
						  BtkTreeIter  *iter);
void              btk_tree_model_get_value       (BtkTreeModel *tree_model,
						  BtkTreeIter  *iter,
						  gint          column,
						  GValue       *value);
gboolean          btk_tree_model_iter_next       (BtkTreeModel *tree_model,
						  BtkTreeIter  *iter);
gboolean          btk_tree_model_iter_children   (BtkTreeModel *tree_model,
						  BtkTreeIter  *iter,
						  BtkTreeIter  *parent);
gboolean          btk_tree_model_iter_has_child  (BtkTreeModel *tree_model,
						  BtkTreeIter  *iter);
gint              btk_tree_model_iter_n_children (BtkTreeModel *tree_model,
						  BtkTreeIter  *iter);
gboolean          btk_tree_model_iter_nth_child  (BtkTreeModel *tree_model,
						  BtkTreeIter  *iter,
						  BtkTreeIter  *parent,
						  gint          n);
gboolean          btk_tree_model_iter_parent     (BtkTreeModel *tree_model,
						  BtkTreeIter  *iter,
						  BtkTreeIter  *child);
void              btk_tree_model_ref_node        (BtkTreeModel *tree_model,
						  BtkTreeIter  *iter);
void              btk_tree_model_unref_node      (BtkTreeModel *tree_model,
						  BtkTreeIter  *iter);
void              btk_tree_model_get             (BtkTreeModel *tree_model,
						  BtkTreeIter  *iter,
						  ...);
void              btk_tree_model_get_valist      (BtkTreeModel *tree_model,
						  BtkTreeIter  *iter,
						  va_list       var_args);


void              btk_tree_model_foreach         (BtkTreeModel            *model,
						  BtkTreeModelForeachFunc  func,
						  gpointer                 user_data);


#ifndef BTK_DISABLE_DEPRECATED
#define btk_tree_model_get_iter_root(tree_model, iter) btk_tree_model_get_iter_first(tree_model, iter)
#endif /* !BTK_DISABLE_DEPRECATED */

/* Signals */
void btk_tree_model_row_changed           (BtkTreeModel *tree_model,
					   BtkTreePath  *path,
					   BtkTreeIter  *iter);
void btk_tree_model_row_inserted          (BtkTreeModel *tree_model,
					   BtkTreePath  *path,
					   BtkTreeIter  *iter);
void btk_tree_model_row_has_child_toggled (BtkTreeModel *tree_model,
					   BtkTreePath  *path,
					   BtkTreeIter  *iter);
void btk_tree_model_row_deleted           (BtkTreeModel *tree_model,
					   BtkTreePath  *path);
void btk_tree_model_rows_reordered        (BtkTreeModel *tree_model,
					   BtkTreePath  *path,
					   BtkTreeIter  *iter,
					   gint         *new_order);

B_END_DECLS

#endif /* __BTK_TREE_MODEL_H__ */
