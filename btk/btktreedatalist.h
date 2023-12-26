/* btktreedatalist.h
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

#ifndef __BTK_TREE_DATA_LIST_H__
#define __BTK_TREE_DATA_LIST_H__

#include <btk/btk.h>

typedef struct _BtkTreeDataList BtkTreeDataList;
struct _BtkTreeDataList
{
  BtkTreeDataList *next;

  union {
    bint	   v_int;
    bint8          v_char;
    buint8         v_uchar;
    buint	   v_uint;
    blong	   v_long;
    bulong	   v_ulong;
    bint64	   v_int64;
    buint64        v_uint64;
    bfloat	   v_float;
    bdouble        v_double;
    bpointer	   v_pointer;
  } data;
};

typedef struct _BtkTreeDataSortHeader
{
  bint sort_column_id;
  BtkTreeIterCompareFunc func;
  bpointer data;
  GDestroyNotify destroy;
} BtkTreeDataSortHeader;

BtkTreeDataList *_btk_tree_data_list_alloc          (void);
void             _btk_tree_data_list_free           (BtkTreeDataList *list,
						     GType           *column_headers);
bboolean         _btk_tree_data_list_check_type     (GType            type);
void             _btk_tree_data_list_node_to_value  (BtkTreeDataList *list,
						     GType            type,
						     BValue          *value);
void             _btk_tree_data_list_value_to_node  (BtkTreeDataList *list,
						     BValue          *value);

BtkTreeDataList *_btk_tree_data_list_node_copy      (BtkTreeDataList *list,
                                                     GType            type);

/* Header code */
bint                   _btk_tree_data_list_compare_func (BtkTreeModel *model,
							 BtkTreeIter  *a,
							 BtkTreeIter  *b,
							 bpointer      user_data);
GList *                _btk_tree_data_list_header_new  (bint          n_columns,
							GType        *types);
void                   _btk_tree_data_list_header_free (GList        *header_list);
BtkTreeDataSortHeader *_btk_tree_data_list_get_header  (GList        *header_list,
							bint          sort_column_id);
GList                 *_btk_tree_data_list_set_header  (GList                  *header_list,
							bint                    sort_column_id,
							BtkTreeIterCompareFunc  func,
							bpointer                data,
							GDestroyNotify          destroy);

#endif /* __BTK_TREE_DATA_LIST_H__ */
