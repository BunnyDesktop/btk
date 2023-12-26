/* BTK - The GIMP Toolkit
 * btkprintbackend.h: Abstract printer backend interfaces
 * Copyright (C) 2006, Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <string.h>
#include <bunnylib.h>
#include <bmodule.h>

#include "btkprinteroptionset.h"
#include "btkalias.h"

/*****************************************
 *         BtkPrinterOptionSet    *
 *****************************************/

enum {
  CHANGED,
  LAST_SIGNAL
};

static buint signals[LAST_SIGNAL] = { 0 };

/* ugly side-effect of aliasing */
#undef btk_printer_option_set

G_DEFINE_TYPE (BtkPrinterOptionSet, btk_printer_option_set, B_TYPE_OBJECT)

static void
btk_printer_option_set_finalize (BObject *object)
{
  BtkPrinterOptionSet *set = BTK_PRINTER_OPTION_SET (object);

  g_hash_table_destroy (set->hash);
  g_ptr_array_foreach (set->array, (GFunc)g_object_unref, NULL);
  g_ptr_array_free (set->array, TRUE);
  
  B_OBJECT_CLASS (btk_printer_option_set_parent_class)->finalize (object);
}

static void
btk_printer_option_set_init (BtkPrinterOptionSet *set)
{
  set->array = g_ptr_array_new ();
  set->hash = g_hash_table_new (g_str_hash, g_str_equal);
}

static void
btk_printer_option_set_class_init (BtkPrinterOptionSetClass *class)
{
  BObjectClass *bobject_class = (BObjectClass *)class;

  bobject_class->finalize = btk_printer_option_set_finalize;

  signals[CHANGED] =
    g_signal_new ("changed",
		  B_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkPrinterOptionSetClass, changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
}


static void
emit_changed (BtkPrinterOptionSet *set)
{
  g_signal_emit (set, signals[CHANGED], 0);
}

BtkPrinterOptionSet *
btk_printer_option_set_new (void)
{
  return g_object_new (BTK_TYPE_PRINTER_OPTION_SET, NULL);
}

void
btk_printer_option_set_remove (BtkPrinterOptionSet *set,
			       BtkPrinterOption    *option)
{
  int i;
  
  for (i = 0; i < set->array->len; i++)
    {
      if (g_ptr_array_index (set->array, i) == option)
	{
	  g_ptr_array_remove_index (set->array, i);
	  g_hash_table_remove (set->hash, option->name);
	  g_signal_handlers_disconnect_by_func (option, emit_changed, set);

	  g_object_unref (option);
	  break;
	}
    }
}

void
btk_printer_option_set_add (BtkPrinterOptionSet *set,
			    BtkPrinterOption    *option)
{
  g_object_ref (option);
  
  if (btk_printer_option_set_lookup (set, option->name))
    btk_printer_option_set_remove (set, option);
    
  g_ptr_array_add (set->array, option);
  g_hash_table_insert (set->hash, option->name, option);
  g_signal_connect_object (option, "changed", G_CALLBACK (emit_changed), set, G_CONNECT_SWAPPED);
}

BtkPrinterOption *
btk_printer_option_set_lookup (BtkPrinterOptionSet *set,
			       const char          *name)
{
  bpointer ptr;

  ptr = g_hash_table_lookup (set->hash, name);

  return BTK_PRINTER_OPTION (ptr);
}

void
btk_printer_option_set_clear_conflicts (BtkPrinterOptionSet *set)
{
  btk_printer_option_set_foreach (set,
				  (BtkPrinterOptionSetFunc)btk_printer_option_clear_has_conflict,
				  NULL);
}

/**
 * btk_printer_option_set_get_groups:
 *
 * Return value: (element-type utf8) (transfer full):
 */
GList *
btk_printer_option_set_get_groups (BtkPrinterOptionSet *set)
{
  BtkPrinterOption *option;
  GList *list = NULL;
  int i;

  for (i = 0; i < set->array->len; i++)
    {
      option = g_ptr_array_index (set->array, i);

      if (g_list_find_custom (list, option->group, (GCompareFunc)g_strcmp0) == NULL)
	list = g_list_prepend (list, g_strdup (option->group));
    }

  return g_list_reverse (list);
}

void
btk_printer_option_set_foreach_in_group (BtkPrinterOptionSet     *set,
					 const char              *group,
					 BtkPrinterOptionSetFunc  func,
					 bpointer                 user_data)
{
  BtkPrinterOption *option;
  int i;

  for (i = 0; i < set->array->len; i++)
    {
      option = g_ptr_array_index (set->array, i);

      if (group == NULL || g_strcmp0 (group, option->group) == 0)
	func (option, user_data);
    }
}

void
btk_printer_option_set_foreach (BtkPrinterOptionSet *set,
				BtkPrinterOptionSetFunc func,
				bpointer user_data)
{
  btk_printer_option_set_foreach_in_group (set, NULL, func, user_data);
}


#define __BTK_PRINTER_OPTION_SET_C__
#include "btkaliasdef.c"
