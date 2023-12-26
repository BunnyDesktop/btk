/* btktreesortable.c
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
#include "btktreesortable.h"
#include "btkmarshalers.h"
#include "btkintl.h"
#include "btkalias.h"

static void btk_tree_sortable_base_init (bpointer g_class);

GType
btk_tree_sortable_get_type (void)
{
  static GType tree_sortable_type = 0;

  if (! tree_sortable_type)
    {
      const GTypeInfo tree_sortable_info =
      {
	sizeof (BtkTreeSortableIface), /* class_size */
	btk_tree_sortable_base_init,   /* base_init */
	NULL,		/* base_finalize */
	NULL,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	0,
	0,
	NULL
      };

      tree_sortable_type =
	g_type_register_static (B_TYPE_INTERFACE, I_("BtkTreeSortable"),
				&tree_sortable_info, 0);

      g_type_interface_add_prerequisite (tree_sortable_type, BTK_TYPE_TREE_MODEL);
    }

  return tree_sortable_type;
}

static void
btk_tree_sortable_base_init (bpointer g_class)
{
  static bboolean initialized = FALSE;

  if (! initialized)
    {
      /**
       * BtkTreeSortable::sort-column-changed:
       * @sortable: the object on which the signal is emitted
       *
       * The ::sort-column-changed signal is emitted when the sort column
       * or sort order of @sortable is changed. The signal is emitted before
       * the contents of @sortable are resorted.
       */
      g_signal_new (I_("sort-column-changed"),
                    BTK_TYPE_TREE_SORTABLE,
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (BtkTreeSortableIface, sort_column_changed),
                    NULL, NULL,
                    _btk_marshal_VOID__VOID,
                    B_TYPE_NONE, 0);
      initialized = TRUE;
    }
}

/**
 * btk_tree_sortable_sort_column_changed:
 * @sortable: A #BtkTreeSortable
 * 
 * Emits a #BtkTreeSortable::sort-column-changed signal on @sortable.
 */
void
btk_tree_sortable_sort_column_changed (BtkTreeSortable *sortable)
{
  g_return_if_fail (BTK_IS_TREE_SORTABLE (sortable));

  g_signal_emit_by_name (sortable, "sort-column-changed");
}

/**
 * btk_tree_sortable_get_sort_column_id:
 * @sortable: A #BtkTreeSortable
 * @sort_column_id: The sort column id to be filled in
 * @order: The #BtkSortType to be filled in
 * 
 * Fills in @sort_column_id and @order with the current sort column and the
 * order. It returns %TRUE unless the @sort_column_id is 
 * %BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID or 
 * %BTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID.
 * 
 * Return value: %TRUE if the sort column is not one of the special sort
 *   column ids.
 **/
bboolean
btk_tree_sortable_get_sort_column_id (BtkTreeSortable  *sortable,
				      bint             *sort_column_id,
				      BtkSortType      *order)
{
  BtkTreeSortableIface *iface;

  g_return_val_if_fail (BTK_IS_TREE_SORTABLE (sortable), FALSE);

  iface = BTK_TREE_SORTABLE_GET_IFACE (sortable);

  g_return_val_if_fail (iface != NULL, FALSE);
  g_return_val_if_fail (iface->get_sort_column_id != NULL, FALSE);

  return (* iface->get_sort_column_id) (sortable, sort_column_id, order);
}

/**
 * btk_tree_sortable_set_sort_column_id:
 * @sortable: A #BtkTreeSortable
 * @sort_column_id: the sort column id to set
 * @order: The sort order of the column
 * 
 * Sets the current sort column to be @sort_column_id. The @sortable will
 * resort itself to reflect this change, after emitting a
 * #BtkTreeSortable::sort-column-changed signal. @sort_column_id may either be
 * a regular column id, or one of the following special values:
 * <variablelist>
 * <varlistentry>
 *   <term>%BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID</term>
 *   <listitem>the default sort function will be used, if it is set</listitem>
 * </varlistentry>
 * <varlistentry>
 *   <term>%BTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID</term>
 *   <listitem>no sorting will occur</listitem>
 * </varlistentry>
 * </variablelist>
 */
void
btk_tree_sortable_set_sort_column_id (BtkTreeSortable  *sortable,
				      bint              sort_column_id,
				      BtkSortType       order)
{
  BtkTreeSortableIface *iface;

  g_return_if_fail (BTK_IS_TREE_SORTABLE (sortable));

  iface = BTK_TREE_SORTABLE_GET_IFACE (sortable);

  g_return_if_fail (iface != NULL);
  g_return_if_fail (iface->set_sort_column_id != NULL);
  
  (* iface->set_sort_column_id) (sortable, sort_column_id, order);
}

/**
 * btk_tree_sortable_set_sort_func:
 * @sortable: A #BtkTreeSortable
 * @sort_column_id: the sort column id to set the function for
 * @sort_func: The comparison function
 * @user_data: (allow-none): User data to pass to @sort_func, or %NULL
 * @destroy: (allow-none): Destroy notifier of @user_data, or %NULL
 * 
 * Sets the comparison function used when sorting to be @sort_func. If the
 * current sort column id of @sortable is the same as @sort_column_id, then 
 * the model will sort using this function.
 */
void
btk_tree_sortable_set_sort_func (BtkTreeSortable        *sortable,
				 bint                    sort_column_id,
				 BtkTreeIterCompareFunc  sort_func,
				 bpointer                user_data,
				 GDestroyNotify          destroy)
{
  BtkTreeSortableIface *iface;

  g_return_if_fail (BTK_IS_TREE_SORTABLE (sortable));
  g_return_if_fail (sort_func != NULL);

  iface = BTK_TREE_SORTABLE_GET_IFACE (sortable);

  g_return_if_fail (iface != NULL);
  g_return_if_fail (iface->set_sort_func != NULL);
  g_return_if_fail (sort_column_id >= 0);

  (* iface->set_sort_func) (sortable, sort_column_id, sort_func, user_data, destroy);
}

/**
 * btk_tree_sortable_set_default_sort_func:
 * @sortable: A #BtkTreeSortable
 * @sort_func: The comparison function
 * @user_data: (allow-none): User data to pass to @sort_func, or %NULL
 * @destroy: (allow-none): Destroy notifier of @user_data, or %NULL
 * 
 * Sets the default comparison function used when sorting to be @sort_func.  
 * If the current sort column id of @sortable is
 * %BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, then the model will sort using 
 * this function.
 *
 * If @sort_func is %NULL, then there will be no default comparison function.
 * This means that once the model  has been sorted, it can't go back to the
 * default state. In this case, when the current sort column id of @sortable 
 * is %BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, the model will be unsorted.
 */
void
btk_tree_sortable_set_default_sort_func (BtkTreeSortable        *sortable,
					 BtkTreeIterCompareFunc  sort_func,
					 bpointer                user_data,
					 GDestroyNotify          destroy)
{
  BtkTreeSortableIface *iface;

  g_return_if_fail (BTK_IS_TREE_SORTABLE (sortable));

  iface = BTK_TREE_SORTABLE_GET_IFACE (sortable);

  g_return_if_fail (iface != NULL);
  g_return_if_fail (iface->set_default_sort_func != NULL);
  
  (* iface->set_default_sort_func) (sortable, sort_func, user_data, destroy);
}

/**
 * btk_tree_sortable_has_default_sort_func:
 * @sortable: A #BtkTreeSortable
 * 
 * Returns %TRUE if the model has a default sort function. This is used
 * primarily by BtkTreeViewColumns in order to determine if a model can 
 * go back to the default state, or not.
 * 
 * Return value: %TRUE, if the model has a default sort function
 */
bboolean
btk_tree_sortable_has_default_sort_func (BtkTreeSortable *sortable)
{
  BtkTreeSortableIface *iface;

  g_return_val_if_fail (BTK_IS_TREE_SORTABLE (sortable), FALSE);

  iface = BTK_TREE_SORTABLE_GET_IFACE (sortable);

  g_return_val_if_fail (iface != NULL, FALSE);
  g_return_val_if_fail (iface->has_default_sort_func != NULL, FALSE);
  
  return (* iface->has_default_sort_func) (sortable);
}

#define __BTK_TREE_SORTABLE_C__
#include "btkaliasdef.c"
