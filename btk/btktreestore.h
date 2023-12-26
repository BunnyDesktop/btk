/* btktreestore.h
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

#ifndef __BTK_TREE_STORE_H__
#define __BTK_TREE_STORE_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bdkconfig.h>
#include <btk/btktreemodel.h>
#include <btk/btktreesortable.h>
#include <stdarg.h>


B_BEGIN_DECLS


#define BTK_TYPE_TREE_STORE			(btk_tree_store_get_type ())
#define BTK_TREE_STORE(obj)			(B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TREE_STORE, BtkTreeStore))
#define BTK_TREE_STORE_CLASS(klass)		(B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TREE_STORE, BtkTreeStoreClass))
#define BTK_IS_TREE_STORE(obj)			(B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TREE_STORE))
#define BTK_IS_TREE_STORE_CLASS(klass)		(B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TREE_STORE))
#define BTK_TREE_STORE_GET_CLASS(obj)		(B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TREE_STORE, BtkTreeStoreClass))

typedef struct _BtkTreeStore       BtkTreeStore;
typedef struct _BtkTreeStoreClass  BtkTreeStoreClass;

struct _BtkTreeStore
{
  BObject parent;

  gint GSEAL (stamp);
  gpointer GSEAL (root);
  gpointer GSEAL (last);
  gint GSEAL (n_columns);
  gint GSEAL (sort_column_id);
  GList *GSEAL (sort_list);
  BtkSortType GSEAL (order);
  GType *GSEAL (column_headers);
  BtkTreeIterCompareFunc GSEAL (default_sort_func);
  gpointer GSEAL (default_sort_data);
  GDestroyNotify GSEAL (default_sort_destroy);
  guint GSEAL (columns_dirty) : 1;
};

struct _BtkTreeStoreClass
{
  BObjectClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType         btk_tree_store_get_type         (void) B_GNUC_CONST;
BtkTreeStore *btk_tree_store_new              (gint          n_columns,
					       ...);
BtkTreeStore *btk_tree_store_newv             (gint          n_columns,
					       GType        *types);
void          btk_tree_store_set_column_types (BtkTreeStore *tree_store,
					       gint          n_columns,
					       GType        *types);

/* NOTE: use btk_tree_model_get to get values from a BtkTreeStore */

void          btk_tree_store_set_value        (BtkTreeStore *tree_store,
					       BtkTreeIter  *iter,
					       gint          column,
					       BValue       *value);
void          btk_tree_store_set              (BtkTreeStore *tree_store,
					       BtkTreeIter  *iter,
					       ...);
void          btk_tree_store_set_valuesv      (BtkTreeStore *tree_store,
					       BtkTreeIter  *iter,
					       gint         *columns,
					       BValue       *values,
					       gint          n_values);
void          btk_tree_store_set_valist       (BtkTreeStore *tree_store,
					       BtkTreeIter  *iter,
					       va_list       var_args);
gboolean      btk_tree_store_remove           (BtkTreeStore *tree_store,
					       BtkTreeIter  *iter);
void          btk_tree_store_insert           (BtkTreeStore *tree_store,
					       BtkTreeIter  *iter,
					       BtkTreeIter  *parent,
					       gint          position);
void          btk_tree_store_insert_before    (BtkTreeStore *tree_store,
					       BtkTreeIter  *iter,
					       BtkTreeIter  *parent,
					       BtkTreeIter  *sibling);
void          btk_tree_store_insert_after     (BtkTreeStore *tree_store,
					       BtkTreeIter  *iter,
					       BtkTreeIter  *parent,
					       BtkTreeIter  *sibling);
void          btk_tree_store_insert_with_values (BtkTreeStore *tree_store,
						 BtkTreeIter  *iter,
						 BtkTreeIter  *parent,
						 gint          position,
						 ...);
void          btk_tree_store_insert_with_valuesv (BtkTreeStore *tree_store,
						  BtkTreeIter  *iter,
						  BtkTreeIter  *parent,
						  gint          position,
						  gint         *columns,
						  BValue       *values,
						  gint          n_values);
void          btk_tree_store_prepend          (BtkTreeStore *tree_store,
					       BtkTreeIter  *iter,
					       BtkTreeIter  *parent);
void          btk_tree_store_append           (BtkTreeStore *tree_store,
					       BtkTreeIter  *iter,
					       BtkTreeIter  *parent);
gboolean      btk_tree_store_is_ancestor      (BtkTreeStore *tree_store,
					       BtkTreeIter  *iter,
					       BtkTreeIter  *descendant);
gint          btk_tree_store_iter_depth       (BtkTreeStore *tree_store,
					       BtkTreeIter  *iter);
void          btk_tree_store_clear            (BtkTreeStore *tree_store);
gboolean      btk_tree_store_iter_is_valid    (BtkTreeStore *tree_store,
                                               BtkTreeIter  *iter);
void          btk_tree_store_reorder          (BtkTreeStore *tree_store,
                                               BtkTreeIter  *parent,
                                               gint         *new_order);
void          btk_tree_store_swap             (BtkTreeStore *tree_store,
                                               BtkTreeIter  *a,
                                               BtkTreeIter  *b);
void          btk_tree_store_move_before      (BtkTreeStore *tree_store,
                                               BtkTreeIter  *iter,
                                               BtkTreeIter  *position);
void          btk_tree_store_move_after       (BtkTreeStore *tree_store,
                                               BtkTreeIter  *iter,
                                               BtkTreeIter  *position);


B_END_DECLS


#endif /* __BTK_TREE_STORE_H__ */
