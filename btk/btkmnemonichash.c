/* btkmnemonichash.c: Sets of mnemonics with cycling
 *
 * BTK - The GIMP Toolkit
 * Copyright (C) 2002, Red Hat Inc.
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

#include "btkmnemonichash.h"
#include "btkalias.h"

struct _BtkMnemnonicHash
{
  GHashTable *hash;
};


BtkMnemonicHash *
_btk_mnemonic_hash_new (void)
{
  BtkMnemonicHash *mnemonic_hash = g_new (BtkMnemonicHash, 1);

  mnemonic_hash->hash = g_hash_table_new (g_direct_hash, NULL);

  return mnemonic_hash;
}

static void
mnemonic_hash_free_foreach (bpointer	key,
			    bpointer	value,
			    bpointer	user)
{
  buint keyval = BPOINTER_TO_UINT (key);
  GSList *targets = value;

  bchar *name = btk_accelerator_name (keyval, 0);
      
  g_warning ("mnemonic \"%s\" wasn't removed for widget (%p)",
	     name, targets->data);
  g_free (name);
  
  b_slist_free (targets);
}

void
_btk_mnemonic_hash_free (BtkMnemonicHash *mnemonic_hash)
{
  g_hash_table_foreach (mnemonic_hash->hash,
			mnemonic_hash_free_foreach,
			NULL);

  g_hash_table_destroy (mnemonic_hash->hash);
  g_free (mnemonic_hash);
}

void
_btk_mnemonic_hash_add (BtkMnemonicHash *mnemonic_hash,
			buint            keyval,
			BtkWidget       *target)
{
  bpointer key = BUINT_TO_POINTER (keyval);
  GSList *targets, *new_targets;
  
  g_return_if_fail (BTK_IS_WIDGET (target));
  
  targets = g_hash_table_lookup (mnemonic_hash->hash, key);
  g_return_if_fail (b_slist_find (targets, target) == NULL);

  new_targets = b_slist_append (targets, target);
  if (new_targets != targets)
    g_hash_table_insert (mnemonic_hash->hash, key, new_targets);
}

void
_btk_mnemonic_hash_remove (BtkMnemonicHash *mnemonic_hash,
			   buint           keyval,
			   BtkWidget      *target)
{
  bpointer key = BUINT_TO_POINTER (keyval);
  GSList *targets, *new_targets;
  
  g_return_if_fail (BTK_IS_WIDGET (target));
  
  targets = g_hash_table_lookup (mnemonic_hash->hash, key);

  g_return_if_fail (targets && b_slist_find (targets, target) != NULL);

  new_targets = b_slist_remove (targets, target);
  if (new_targets != targets)
    {
      if (new_targets == NULL)
	g_hash_table_remove (mnemonic_hash->hash, key);
      else
	g_hash_table_insert (mnemonic_hash->hash, key, new_targets);
    }
}

bboolean
_btk_mnemonic_hash_activate (BtkMnemonicHash *mnemonic_hash,
			     buint            keyval)
{
  GSList *list, *targets;
  BtkWidget *widget, *chosen_widget;
  bboolean overloaded;

  targets = g_hash_table_lookup (mnemonic_hash->hash,
				 BUINT_TO_POINTER (keyval));
  if (!targets)
    return FALSE;
  
  overloaded = FALSE;
  chosen_widget = NULL;
  for (list = targets; list; list = list->next)
    {
      widget = BTK_WIDGET (list->data);
      
      if (btk_widget_is_sensitive (widget) &&
	  btk_widget_get_mapped (widget) &&
          widget->window &&
	  bdk_window_is_viewable (widget->window))
	{
	  if (chosen_widget)
	    {
	      overloaded = TRUE;
	      break;
	    }
	  else
	    chosen_widget = widget;
	}
    }

  if (chosen_widget)
    {
      /* For round robin we put the activated entry on
       * the end of the list after activation
       */
      targets = b_slist_remove (targets, chosen_widget);
      targets = b_slist_append (targets, chosen_widget);
      g_hash_table_insert (mnemonic_hash->hash,
			   BUINT_TO_POINTER (keyval),
			   targets);

      return btk_widget_mnemonic_activate (chosen_widget, overloaded);
    }
  return FALSE;
}

GSList *
_btk_mnemonic_hash_lookup (BtkMnemonicHash *mnemonic_hash,
			   buint            keyval)
{
  return g_hash_table_lookup (mnemonic_hash->hash, BUINT_TO_POINTER (keyval));
}

static void
mnemonic_hash_foreach_func (bpointer key,
			    bpointer value,
			    bpointer data)
{
  struct {
    BtkMnemonicHashForeach func;
    bpointer func_data;
  } *info = data;

  buint keyval = BPOINTER_TO_UINT (key);
  GSList *targets = value;
  
  (*info->func) (keyval, targets, info->func_data);
}

void
_btk_mnemonic_hash_foreach (BtkMnemonicHash       *mnemonic_hash,
			    BtkMnemonicHashForeach func,
			    bpointer               func_data)
{
  struct {
    BtkMnemonicHashForeach func;
    bpointer func_data;
  } info;
  
  info.func = func;
  info.func_data = func_data;

  g_hash_table_foreach (mnemonic_hash->hash,
			mnemonic_hash_foreach_func,
			&info);
}
