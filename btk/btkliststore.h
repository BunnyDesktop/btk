/* btkliststore.h
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

#ifndef __BTK_LIST_STORE_H__
#define __BTK_LIST_STORE_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bdkconfig.h>
#include <btk/btktreemodel.h>
#include <btk/btktreesortable.h>


G_BEGIN_DECLS


#define BTK_TYPE_LIST_STORE	       (btk_list_store_get_type ())
#define BTK_LIST_STORE(obj)	       (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_LIST_STORE, BtkListStore))
#define BTK_LIST_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_LIST_STORE, BtkListStoreClass))
#define BTK_IS_LIST_STORE(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_LIST_STORE))
#define BTK_IS_LIST_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_LIST_STORE))
#define BTK_LIST_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_LIST_STORE, BtkListStoreClass))

typedef struct _BtkListStore       BtkListStore;
typedef struct _BtkListStoreClass  BtkListStoreClass;

struct _BtkListStore
{
  GObject parent;

  /*< private >*/
  gint GSEAL (stamp);
  gpointer GSEAL (seq);		/* head of the list */
  gpointer GSEAL (_btk_reserved1);
  GList *GSEAL (sort_list);
  gint GSEAL (n_columns);
  gint GSEAL (sort_column_id);
  BtkSortType GSEAL (order);
  GType *GSEAL (column_headers);
  gint GSEAL (length);
  BtkTreeIterCompareFunc GSEAL (default_sort_func);
  gpointer GSEAL (default_sort_data);
  GDestroyNotify GSEAL (default_sort_destroy);
  guint GSEAL (columns_dirty) : 1;
};

struct _BtkListStoreClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType         btk_list_store_get_type         (void) G_GNUC_CONST;
BtkListStore *btk_list_store_new              (gint          n_columns,
					       ...);
BtkListStore *btk_list_store_newv             (gint          n_columns,
					       GType        *types);
void          btk_list_store_set_column_types (BtkListStore *list_store,
					       gint          n_columns,
					       GType        *types);

/* NOTE: use btk_tree_model_get to get values from a BtkListStore */

void          btk_list_store_set_value        (BtkListStore *list_store,
					       BtkTreeIter  *iter,
					       gint          column,
					       GValue       *value);
void          btk_list_store_set              (BtkListStore *list_store,
					       BtkTreeIter  *iter,
					       ...);
void          btk_list_store_set_valuesv      (BtkListStore *list_store,
					       BtkTreeIter  *iter,
					       gint         *columns,
					       GValue       *values,
					       gint          n_values);
void          btk_list_store_set_valist       (BtkListStore *list_store,
					       BtkTreeIter  *iter,
					       va_list       var_args);
gboolean      btk_list_store_remove           (BtkListStore *list_store,
					       BtkTreeIter  *iter);
void          btk_list_store_insert           (BtkListStore *list_store,
					       BtkTreeIter  *iter,
					       gint          position);
void          btk_list_store_insert_before    (BtkListStore *list_store,
					       BtkTreeIter  *iter,
					       BtkTreeIter  *sibling);
void          btk_list_store_insert_after     (BtkListStore *list_store,
					       BtkTreeIter  *iter,
					       BtkTreeIter  *sibling);
void          btk_list_store_insert_with_values  (BtkListStore *list_store,
						  BtkTreeIter  *iter,
						  gint          position,
						  ...);
void          btk_list_store_insert_with_valuesv (BtkListStore *list_store,
						  BtkTreeIter  *iter,
						  gint          position,
						  gint         *columns,
						  GValue       *values,
						  gint          n_values);
void          btk_list_store_prepend          (BtkListStore *list_store,
					       BtkTreeIter  *iter);
void          btk_list_store_append           (BtkListStore *list_store,
					       BtkTreeIter  *iter);
void          btk_list_store_clear            (BtkListStore *list_store);
gboolean      btk_list_store_iter_is_valid    (BtkListStore *list_store,
                                               BtkTreeIter  *iter);
void          btk_list_store_reorder          (BtkListStore *store,
                                               gint         *new_order);
void          btk_list_store_swap             (BtkListStore *store,
                                               BtkTreeIter  *a,
                                               BtkTreeIter  *b);
void          btk_list_store_move_after       (BtkListStore *store,
                                               BtkTreeIter  *iter,
                                               BtkTreeIter  *position);
void          btk_list_store_move_before      (BtkListStore *store,
                                               BtkTreeIter  *iter,
                                               BtkTreeIter  *position);


G_END_DECLS


#endif /* __BTK_LIST_STORE_H__ */
