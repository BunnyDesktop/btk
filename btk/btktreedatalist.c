/* btktreedatalist.c
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
 * 
 * This file contains code shared between BtkTreeStore and BtkListStore.  Please
 * do not use it.
 */

#include "config.h"
#include "btktreedatalist.h"
#include "btkalias.h"
#include <string.h>

/* node allocation
 */
BtkTreeDataList *
_btk_tree_data_list_alloc (void)
{
  BtkTreeDataList *list;

  list = g_slice_new0 (BtkTreeDataList);

  return list;
}

void
_btk_tree_data_list_free (BtkTreeDataList *list,
			  GType           *column_headers)
{
  BtkTreeDataList *tmp, *next;
  bint i = 0;

  tmp = list;

  while (tmp)
    {
      next = tmp->next;
      if (g_type_is_a (column_headers [i], B_TYPE_STRING))
	g_free ((bchar *) tmp->data.v_pointer);
      else if (g_type_is_a (column_headers [i], B_TYPE_OBJECT) && tmp->data.v_pointer != NULL)
	g_object_unref (tmp->data.v_pointer);
      else if (g_type_is_a (column_headers [i], B_TYPE_BOXED) && tmp->data.v_pointer != NULL)
	g_boxed_free (column_headers [i], (bpointer) tmp->data.v_pointer);

      g_slice_free (BtkTreeDataList, tmp);
      i++;
      tmp = next;
    }
}

bboolean
_btk_tree_data_list_check_type (GType type)
{
  bint i = 0;
  static const GType type_list[] =
  {
    B_TYPE_BOOLEAN,
    B_TYPE_CHAR,
    B_TYPE_UCHAR,
    B_TYPE_INT,
    B_TYPE_UINT,
    B_TYPE_LONG,
    B_TYPE_ULONG,
    B_TYPE_INT64,
    B_TYPE_UINT64,
    B_TYPE_ENUM,
    B_TYPE_FLAGS,
    B_TYPE_FLOAT,
    B_TYPE_DOUBLE,
    B_TYPE_STRING,
    B_TYPE_POINTER,
    B_TYPE_BOXED,
    B_TYPE_OBJECT,
    B_TYPE_INVALID
  };

  if (! B_TYPE_IS_VALUE_TYPE (type))
    return FALSE;


  while (type_list[i] != B_TYPE_INVALID)
    {
      if (g_type_is_a (type, type_list[i]))
	return TRUE;
      i++;
    }
  return FALSE;
}

static inline GType
get_fundamental_type (GType type)
{
  GType result;

  result = B_TYPE_FUNDAMENTAL (type);

  if (result == B_TYPE_INTERFACE)
    {
      if (g_type_is_a (type, B_TYPE_OBJECT))
	result = B_TYPE_OBJECT;
    }

  return result;
}
void
_btk_tree_data_list_node_to_value (BtkTreeDataList *list,
				   GType            type,
				   BValue          *value)
{
  b_value_init (value, type);

  switch (get_fundamental_type (type))
    {
    case B_TYPE_BOOLEAN:
      b_value_set_boolean (value, (bboolean) list->data.v_int);
      break;
    case B_TYPE_CHAR:
      b_value_set_char (value, (bchar) list->data.v_char);
      break;
    case B_TYPE_UCHAR:
      b_value_set_uchar (value, (buchar) list->data.v_uchar);
      break;
    case B_TYPE_INT:
      b_value_set_int (value, (bint) list->data.v_int);
      break;
    case B_TYPE_UINT:
      b_value_set_uint (value, (buint) list->data.v_uint);
      break;
    case B_TYPE_LONG:
      b_value_set_long (value, list->data.v_long);
      break;
    case B_TYPE_ULONG:
      b_value_set_ulong (value, list->data.v_ulong);
      break;
    case B_TYPE_INT64:
      b_value_set_int64 (value, list->data.v_int64);
      break;
    case B_TYPE_UINT64:
      b_value_set_uint64 (value, list->data.v_uint64);
      break;
    case B_TYPE_ENUM:
      b_value_set_enum (value, list->data.v_int);
      break;
    case B_TYPE_FLAGS:
      b_value_set_flags (value, list->data.v_uint);
      break;
    case B_TYPE_FLOAT:
      b_value_set_float (value, (bfloat) list->data.v_float);
      break;
    case B_TYPE_DOUBLE:
      b_value_set_double (value, (bdouble) list->data.v_double);
      break;
    case B_TYPE_STRING:
      b_value_set_string (value, (bchar *) list->data.v_pointer);
      break;
    case B_TYPE_POINTER:
      b_value_set_pointer (value, (bpointer) list->data.v_pointer);
      break;
    case B_TYPE_BOXED:
      b_value_set_boxed (value, (bpointer) list->data.v_pointer);
      break;
    case B_TYPE_OBJECT:
      b_value_set_object (value, (BObject *) list->data.v_pointer);
      break;
    default:
      g_warning ("%s: Unsupported type (%s) retrieved.", B_STRLOC, g_type_name (value->g_type));
      break;
    }
}

void
_btk_tree_data_list_value_to_node (BtkTreeDataList *list,
				   BValue          *value)
{
  switch (get_fundamental_type (G_VALUE_TYPE (value)))
    {
    case B_TYPE_BOOLEAN:
      list->data.v_int = b_value_get_boolean (value);
      break;
    case B_TYPE_CHAR:
      list->data.v_char = b_value_get_char (value);
      break;
    case B_TYPE_UCHAR:
      list->data.v_uchar = b_value_get_uchar (value);
      break;
    case B_TYPE_INT:
      list->data.v_int = b_value_get_int (value);
      break;
    case B_TYPE_UINT:
      list->data.v_uint = b_value_get_uint (value);
      break;
    case B_TYPE_LONG:
      list->data.v_long = b_value_get_long (value);
      break;
    case B_TYPE_ULONG:
      list->data.v_ulong = b_value_get_ulong (value);
      break;
    case B_TYPE_INT64:
      list->data.v_int64 = b_value_get_int64 (value);
      break;
    case B_TYPE_UINT64:
      list->data.v_uint64 = b_value_get_uint64 (value);
      break;
    case B_TYPE_ENUM:
      list->data.v_int = b_value_get_enum (value);
      break;
    case B_TYPE_FLAGS:
      list->data.v_uint = b_value_get_flags (value);
      break;
    case B_TYPE_POINTER:
      list->data.v_pointer = b_value_get_pointer (value);
      break;
    case B_TYPE_FLOAT:
      list->data.v_float = b_value_get_float (value);
      break;
    case B_TYPE_DOUBLE:
      list->data.v_double = b_value_get_double (value);
      break;
    case B_TYPE_STRING:
      g_free (list->data.v_pointer);
      list->data.v_pointer = b_value_dup_string (value);
      break;
    case B_TYPE_OBJECT:
      if (list->data.v_pointer)
	g_object_unref (list->data.v_pointer);
      list->data.v_pointer = b_value_dup_object (value);
      break;
    case B_TYPE_BOXED:
      if (list->data.v_pointer)
	g_boxed_free (G_VALUE_TYPE (value), list->data.v_pointer);
      list->data.v_pointer = b_value_dup_boxed (value);
      break;
    default:
      g_warning ("%s: Unsupported type (%s) stored.", B_STRLOC, g_type_name (G_VALUE_TYPE (value)));
      break;
    }
}

BtkTreeDataList *
_btk_tree_data_list_node_copy (BtkTreeDataList *list,
                               GType            type)
{
  BtkTreeDataList *new_list;

  g_return_val_if_fail (list != NULL, NULL);
  
  new_list = _btk_tree_data_list_alloc ();
  new_list->next = NULL;

  switch (get_fundamental_type (type))
    {
    case B_TYPE_BOOLEAN:
    case B_TYPE_CHAR:
    case B_TYPE_UCHAR:
    case B_TYPE_INT:
    case B_TYPE_UINT:
    case B_TYPE_LONG:
    case B_TYPE_ULONG:
    case B_TYPE_INT64:
    case B_TYPE_UINT64:
    case B_TYPE_ENUM:
    case B_TYPE_FLAGS:
    case B_TYPE_POINTER:
    case B_TYPE_FLOAT:
    case B_TYPE_DOUBLE:
      new_list->data = list->data;
      break;
    case B_TYPE_STRING:
      new_list->data.v_pointer = g_strdup (list->data.v_pointer);
      break;
    case B_TYPE_OBJECT:
    case B_TYPE_INTERFACE:
      new_list->data.v_pointer = list->data.v_pointer;
      if (new_list->data.v_pointer)
	g_object_ref (new_list->data.v_pointer);
      break;
    case B_TYPE_BOXED:
      if (list->data.v_pointer)
	new_list->data.v_pointer = g_boxed_copy (type, list->data.v_pointer);
      else
	new_list->data.v_pointer = NULL;
      break;
    default:
      g_warning ("Unsupported node type (%s) copied.", g_type_name (type));
      break;
    }

  return new_list;
}

bint
_btk_tree_data_list_compare_func (BtkTreeModel *model,
				  BtkTreeIter  *a,
				  BtkTreeIter  *b,
				  bpointer      user_data)
{
  bint column = BPOINTER_TO_INT (user_data);
  GType type = btk_tree_model_get_column_type (model, column);
  BValue a_value = {0, };
  BValue b_value = {0, };
  bint retval;
  const bchar *stra, *strb;

  btk_tree_model_get_value (model, a, column, &a_value);
  btk_tree_model_get_value (model, b, column, &b_value);

  switch (get_fundamental_type (type))
    {
    case B_TYPE_BOOLEAN:
      if (b_value_get_boolean (&a_value) < b_value_get_boolean (&b_value))
	retval = -1;
      else if (b_value_get_boolean (&a_value) == b_value_get_boolean (&b_value))
	retval = 0;
      else
	retval = 1;
      break;
    case B_TYPE_CHAR:
      if (b_value_get_char (&a_value) < b_value_get_char (&b_value))
	retval = -1;
      else if (b_value_get_char (&a_value) == b_value_get_char (&b_value))
	retval = 0;
      else
	retval = 1;
      break;
    case B_TYPE_UCHAR:
      if (b_value_get_uchar (&a_value) < b_value_get_uchar (&b_value))
	retval = -1;
      else if (b_value_get_uchar (&a_value) == b_value_get_uchar (&b_value))
	retval = 0;
      else
	retval = 1;
      break;
    case B_TYPE_INT:
      if (b_value_get_int (&a_value) < b_value_get_int (&b_value))
	retval = -1;
      else if (b_value_get_int (&a_value) == b_value_get_int (&b_value))
	retval = 0;
      else
	retval = 1;
      break;
    case B_TYPE_UINT:
      if (b_value_get_uint (&a_value) < b_value_get_uint (&b_value))
	retval = -1;
      else if (b_value_get_uint (&a_value) == b_value_get_uint (&b_value))
	retval = 0;
      else
	retval = 1;
      break;
    case B_TYPE_LONG:
      if (b_value_get_long (&a_value) < b_value_get_long (&b_value))
	retval = -1;
      else if (b_value_get_long (&a_value) == b_value_get_long (&b_value))
	retval = 0;
      else
	retval = 1;
      break;
    case B_TYPE_ULONG:
      if (b_value_get_ulong (&a_value) < b_value_get_ulong (&b_value))
	retval = -1;
      else if (b_value_get_ulong (&a_value) == b_value_get_ulong (&b_value))
	retval = 0;
      else
	retval = 1;
      break;
    case B_TYPE_INT64:
      if (b_value_get_int64 (&a_value) < b_value_get_int64 (&b_value))
	retval = -1;
      else if (b_value_get_int64 (&a_value) == b_value_get_int64 (&b_value))
	retval = 0;
      else
	retval = 1;
      break;
    case B_TYPE_UINT64:
      if (b_value_get_uint64 (&a_value) < b_value_get_uint64 (&b_value))
	retval = -1;
      else if (b_value_get_uint64 (&a_value) == b_value_get_uint64 (&b_value))
	retval = 0;
      else
	retval = 1;
      break;
    case B_TYPE_ENUM:
      /* this is somewhat bogus. */
      if (b_value_get_enum (&a_value) < b_value_get_enum (&b_value))
	retval = -1;
      else if (b_value_get_enum (&a_value) == b_value_get_enum (&b_value))
	retval = 0;
      else
	retval = 1;
      break;
    case B_TYPE_FLAGS:
      /* this is even more bogus. */
      if (b_value_get_flags (&a_value) < b_value_get_flags (&b_value))
	retval = -1;
      else if (b_value_get_flags (&a_value) == b_value_get_flags (&b_value))
	retval = 0;
      else
	retval = 1;
      break;
    case B_TYPE_FLOAT:
      if (b_value_get_float (&a_value) < b_value_get_float (&b_value))
	retval = -1;
      else if (b_value_get_float (&a_value) == b_value_get_float (&b_value))
	retval = 0;
      else
	retval = 1;
      break;
    case B_TYPE_DOUBLE:
      if (b_value_get_double (&a_value) < b_value_get_double (&b_value))
	retval = -1;
      else if (b_value_get_double (&a_value) == b_value_get_double (&b_value))
	retval = 0;
      else
	retval = 1;
      break;
    case B_TYPE_STRING:
      stra = b_value_get_string (&a_value);
      strb = b_value_get_string (&b_value);
      if (stra == NULL) stra = "";
      if (strb == NULL) strb = "";
      retval = g_utf8_collate (stra, strb);
      break;
    case B_TYPE_POINTER:
    case B_TYPE_BOXED:
    case B_TYPE_OBJECT:
    default:
      g_warning ("Attempting to sort on invalid type %s\n", g_type_name (type));
      retval = FALSE;
      break;
    }

  b_value_unset (&a_value);
  b_value_unset (&b_value);

  return retval;
}


GList *
_btk_tree_data_list_header_new (bint   n_columns,
				GType *types)
{
  GList *retval = NULL;

  bint i;

  for (i = 0; i < n_columns; i ++)
    {
      BtkTreeDataSortHeader *header;

      header = g_slice_new (BtkTreeDataSortHeader);

      retval = g_list_prepend (retval, header);
      header->sort_column_id = i;
      header->func = _btk_tree_data_list_compare_func;
      header->destroy = NULL;
      header->data = BINT_TO_POINTER (i);
    }
  return g_list_reverse (retval);
}

void
_btk_tree_data_list_header_free (GList *list)
{
  GList *tmp;

  for (tmp = list; tmp; tmp = tmp->next)
    {
      BtkTreeDataSortHeader *header = (BtkTreeDataSortHeader *) tmp->data;

      if (header->destroy)
	{
	  GDestroyNotify d = header->destroy;

	  header->destroy = NULL;
	  d (header->data);
	}

      g_slice_free (BtkTreeDataSortHeader, header);
    }
  g_list_free (list);
}

BtkTreeDataSortHeader *
_btk_tree_data_list_get_header (GList   *header_list,
				bint     sort_column_id)
{
  BtkTreeDataSortHeader *header = NULL;

  for (; header_list; header_list = header_list->next)
    {
      header = (BtkTreeDataSortHeader*) header_list->data;
      if (header->sort_column_id == sort_column_id)
	return header;
    }
  return NULL;
}


GList *
_btk_tree_data_list_set_header (GList                  *header_list,
				bint                    sort_column_id,
				BtkTreeIterCompareFunc  func,
				bpointer                data,
				GDestroyNotify          destroy)
{
  GList *list = header_list;
  BtkTreeDataSortHeader *header = NULL;

  for (; list; list = list->next)
    {
      header = (BtkTreeDataSortHeader*) list->data;
      if (header->sort_column_id == sort_column_id)
	break;
      header = NULL;

      if (list->next == NULL)
	break;
    }
  
  if (header == NULL)
    {
      header = g_slice_new0 (BtkTreeDataSortHeader);
      header->sort_column_id = sort_column_id;
      if (list)
	list = g_list_append (list, header);
      else
	header_list = g_list_append (header_list, header);
    }

  if (header->destroy)
    {
      GDestroyNotify d = header->destroy;
      
      header->destroy = NULL;
      d (header->data);
    }
  
  header->func = func;
  header->data = data;
  header->destroy = destroy;

  return header_list;
}
