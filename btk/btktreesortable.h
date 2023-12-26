/* btktreesortable.h
 * Copyright (C) 2001  Red Hat, Inc.
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

#ifndef __BTK_TREE_SORTABLE_H__
#define __BTK_TREE_SORTABLE_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btktreemodel.h>
#include <btk/btktypeutils.h>


B_BEGIN_DECLS

#define BTK_TYPE_TREE_SORTABLE            (btk_tree_sortable_get_type ())
#define BTK_TREE_SORTABLE(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TREE_SORTABLE, BtkTreeSortable))
#define BTK_TREE_SORTABLE_CLASS(obj)      (B_TYPE_CHECK_CLASS_CAST ((obj), BTK_TYPE_TREE_SORTABLE, BtkTreeSortableIface))
#define BTK_IS_TREE_SORTABLE(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TREE_SORTABLE))
#define BTK_TREE_SORTABLE_GET_IFACE(obj)  (B_TYPE_INSTANCE_GET_INTERFACE ((obj), BTK_TYPE_TREE_SORTABLE, BtkTreeSortableIface))

enum {
  BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID = -1,
  BTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID = -2
};

typedef struct _BtkTreeSortable      BtkTreeSortable; /* Dummy typedef */
typedef struct _BtkTreeSortableIface BtkTreeSortableIface;

typedef gint (* BtkTreeIterCompareFunc) (BtkTreeModel *model,
					 BtkTreeIter  *a,
					 BtkTreeIter  *b,
					 gpointer      user_data);


struct _BtkTreeSortableIface
{
  GTypeInterface g_iface;

  /* signals */
  void     (* sort_column_changed)   (BtkTreeSortable        *sortable);

  /* virtual table */
  gboolean (* get_sort_column_id)    (BtkTreeSortable        *sortable,
				      gint                   *sort_column_id,
				      BtkSortType            *order);
  void     (* set_sort_column_id)    (BtkTreeSortable        *sortable,
				      gint                    sort_column_id,
				      BtkSortType             order);
  void     (* set_sort_func)         (BtkTreeSortable        *sortable,
				      gint                    sort_column_id,
				      BtkTreeIterCompareFunc  func,
				      gpointer                data,
				      GDestroyNotify          destroy);
  void     (* set_default_sort_func) (BtkTreeSortable        *sortable,
				      BtkTreeIterCompareFunc  func,
				      gpointer                data,
				      GDestroyNotify          destroy);
  gboolean (* has_default_sort_func) (BtkTreeSortable        *sortable);
};


GType    btk_tree_sortable_get_type              (void) B_GNUC_CONST;

void     btk_tree_sortable_sort_column_changed   (BtkTreeSortable        *sortable);
gboolean btk_tree_sortable_get_sort_column_id    (BtkTreeSortable        *sortable,
						  gint                   *sort_column_id,
						  BtkSortType            *order);
void     btk_tree_sortable_set_sort_column_id    (BtkTreeSortable        *sortable,
						  gint                    sort_column_id,
						  BtkSortType             order);
void     btk_tree_sortable_set_sort_func         (BtkTreeSortable        *sortable,
						  gint                    sort_column_id,
						  BtkTreeIterCompareFunc  sort_func,
						  gpointer                user_data,
						  GDestroyNotify          destroy);
void     btk_tree_sortable_set_default_sort_func (BtkTreeSortable        *sortable,
						  BtkTreeIterCompareFunc  sort_func,
						  gpointer                user_data,
						  GDestroyNotify          destroy);
gboolean btk_tree_sortable_has_default_sort_func (BtkTreeSortable        *sortable);

B_END_DECLS

#endif /* __BTK_TREE_SORTABLE_H__ */
