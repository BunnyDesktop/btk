/* BTK - The GIMP Toolkit
 * Copyright (C) 1998, 2001 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"
#include <string.h>
#include <stdlib.h>

#include "btkaccelgroup.h"
#include "btkaccellabel.h" /* For _btk_accel_label_class_get_accelerator_label */
#include "btkaccelmap.h"
#include "btkintl.h"
#include "btkmain.h"		/* For _btk_boolean_handled_accumulator */
#include "bdk/bdkkeysyms.h"
#include "btkmarshalers.h"
#include "btkprivate.h"
#include "btkalias.h"

/**
 * SECTION:btkaccelgroup
 * @Short_description: Groups of global keyboard accelerators for an entire BtkWindow
 * @Title: Accelerator Groups
 * @See_also:btk_window_add_accel_group(), btk_accel_map_change_entry(),
 * btk_item_factory_new(), btk_label_new_with_mnemonic()
 * 
 * A #BtkAccelGroup represents a group of keyboard accelerators,
 * typically attached to a toplevel #BtkWindow (with
 * btk_window_add_accel_group()). Usually you won't need to create a
 * #BtkAccelGroup directly; instead, when using #BtkItemFactory, BTK+
 * automatically sets up the accelerators for your menus in the item
 * factory's #BtkAccelGroup.
 * 
 * 
 * Note that <firstterm>accelerators</firstterm> are different from
 * <firstterm>mnemonics</firstterm>. Accelerators are shortcuts for
 * activating a menu item; they appear alongside the menu item they're a
 * shortcut for. For example "Ctrl+Q" might appear alongside the "Quit"
 * menu item. Mnemonics are shortcuts for GUI elements such as text
 * entries or buttons; they appear as underlined characters. See
 * btk_label_new_with_mnemonic(). Menu items can have both accelerators
 * and mnemonics, of course.
 */


/* --- prototypes --- */
static void btk_accel_group_finalize     (BObject    *object);
static void btk_accel_group_get_property (BObject    *object,
                                          guint       param_id,
                                          BValue     *value,
                                          BParamSpec *pspec);
static void accel_closure_invalidate     (gpointer    data,
                                          GClosure   *closure);


/* --- variables --- */
static guint  signal_accel_activate      = 0;
static guint  signal_accel_changed       = 0;
static guint  quark_acceleratable_groups = 0;
static guint  default_accel_mod_mask     = (BDK_SHIFT_MASK   |
                                            BDK_CONTROL_MASK |
                                            BDK_MOD1_MASK    |
                                            BDK_SUPER_MASK   |
                                            BDK_HYPER_MASK   |
                                            BDK_META_MASK);


enum {
  PROP_0,
  PROP_IS_LOCKED,
  PROP_MODIFIER_MASK,
};

G_DEFINE_TYPE (BtkAccelGroup, btk_accel_group, B_TYPE_OBJECT)

/* --- functions --- */
static void
btk_accel_group_class_init (BtkAccelGroupClass *class)
{
  BObjectClass *object_class = B_OBJECT_CLASS (class);

  quark_acceleratable_groups = g_quark_from_static_string ("btk-acceleratable-accel-groups");

  object_class->finalize = btk_accel_group_finalize;
  object_class->get_property = btk_accel_group_get_property;

  class->accel_changed = NULL;

  g_object_class_install_property (object_class,
                                   PROP_IS_LOCKED,
                                   g_param_spec_boolean ("is-locked",
                                                         "Is locked",
                                                         "Is the accel group locked",
                                                         FALSE,
                                                         G_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_MODIFIER_MASK,
                                   g_param_spec_flags ("modifier-mask",
                                                       "Modifier Mask",
                                                       "Modifier Mask",
                                                       BDK_TYPE_MODIFIER_TYPE,
                                                       default_accel_mod_mask,
                                                       G_PARAM_READABLE));

  /**
   * BtkAccelGroup::accel-activate:
   * @accel_group: the #BtkAccelGroup which received the signal
   * @acceleratable: the object on which the accelerator was activated
   * @keyval: the accelerator keyval
   * @modifier: the modifier combination of the accelerator
   *
   * The accel-activate signal is an implementation detail of
   * #BtkAccelGroup and not meant to be used by applications.
   * 
   * Returns: %TRUE if the accelerator was activated
   */
  signal_accel_activate =
    g_signal_new (I_("accel-activate"),
		  B_OBJECT_CLASS_TYPE (class),
		  G_SIGNAL_DETAILED,
		  0,
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__OBJECT_UINT_FLAGS,
		  B_TYPE_BOOLEAN, 3,
		  B_TYPE_OBJECT,
		  B_TYPE_UINT,
		  BDK_TYPE_MODIFIER_TYPE);
  /**
   * BtkAccelGroup::accel-changed:
   * @accel_group: the #BtkAccelGroup which received the signal
   * @keyval: the accelerator keyval
   * @modifier: the modifier combination of the accelerator
   * @accel_closure: the #GClosure of the accelerator
   *
   * The accel-changed signal is emitted when a #BtkAccelGroupEntry
   * is added to or removed from the accel group. 
   *
   * Widgets like #BtkAccelLabel which display an associated 
   * accelerator should connect to this signal, and rebuild 
   * their visual representation if the @accel_closure is theirs.
   */
  signal_accel_changed =
    g_signal_new (I_("accel-changed"),
		  B_OBJECT_CLASS_TYPE (class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
		  G_STRUCT_OFFSET (BtkAccelGroupClass, accel_changed),
		  NULL, NULL,
		  _btk_marshal_VOID__UINT_FLAGS_BOXED,
		  B_TYPE_NONE, 3,
		  B_TYPE_UINT,
		  BDK_TYPE_MODIFIER_TYPE,
		  B_TYPE_CLOSURE);
}

static void
btk_accel_group_finalize (BObject *object)
{
  BtkAccelGroup *accel_group = BTK_ACCEL_GROUP (object);
  guint i;
  
  for (i = 0; i < accel_group->n_accels; i++)
    {
      BtkAccelGroupEntry *entry = &accel_group->priv_accels[i];

      if (entry->accel_path_quark)
	{
	  const gchar *accel_path = g_quark_to_string (entry->accel_path_quark);

	  _btk_accel_map_remove_group (accel_path, accel_group);
	}
      g_closure_remove_invalidate_notifier (entry->closure, accel_group, accel_closure_invalidate);

      /* remove quick_accel_add() refcount */
      g_closure_unref (entry->closure);
    }

  g_free (accel_group->priv_accels);

  B_OBJECT_CLASS (btk_accel_group_parent_class)->finalize (object);
}

static void
btk_accel_group_get_property (BObject    *object,
                              guint       param_id,
                              BValue     *value,
                              BParamSpec *pspec)
{
  BtkAccelGroup *accel_group = BTK_ACCEL_GROUP (object);

  switch (param_id)
    {
    case PROP_IS_LOCKED:
      b_value_set_boolean (value, accel_group->lock_count > 0);
      break;
    case PROP_MODIFIER_MASK:
      b_value_set_flags (value, accel_group->modifier_mask);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
btk_accel_group_init (BtkAccelGroup *accel_group)
{
  accel_group->lock_count = 0;
  accel_group->modifier_mask = btk_accelerator_get_default_mod_mask ();
  accel_group->acceleratables = NULL;
  accel_group->n_accels = 0;
  accel_group->priv_accels = NULL;
}

/**
 * btk_accel_group_new:
 * @returns: a new #BtkAccelGroup object
 * 
 * Creates a new #BtkAccelGroup. 
 */
BtkAccelGroup*
btk_accel_group_new (void)
{
  return g_object_new (BTK_TYPE_ACCEL_GROUP, NULL);
}

/**
 * btk_accel_group_get_is_locked:
 * @accel_group: a #BtkAccelGroup
 *
 * Locks are added and removed using btk_accel_group_lock() and
 * btk_accel_group_unlock().
 *
 * Returns: %TRUE if there are 1 or more locks on the @accel_group,
 * %FALSE otherwise.
 *
 * Since: 2.14
 */
gboolean
btk_accel_group_get_is_locked (BtkAccelGroup *accel_group)
{
  g_return_val_if_fail (BTK_IS_ACCEL_GROUP (accel_group), FALSE);

  return accel_group->lock_count > 0;
}

/**
 * btk_accel_group_get_modifier_mask:
 * @accel_group: a #BtkAccelGroup
 *
 * Gets a #BdkModifierType representing the mask for this
 * @accel_group. For example, #BDK_CONTROL_MASK, #BDK_SHIFT_MASK, etc.
 *
 * Returns: the modifier mask for this accel group.
 *
 * Since: 2.14
 */
BdkModifierType
btk_accel_group_get_modifier_mask (BtkAccelGroup *accel_group)
{
  g_return_val_if_fail (BTK_IS_ACCEL_GROUP (accel_group), 0);

  return accel_group->modifier_mask;
}

static void
accel_group_weak_ref_detach (GSList  *free_list,
			     BObject *stale_object)
{
  GSList *slist;
  
  for (slist = free_list; slist; slist = slist->next)
    {
      BtkAccelGroup *accel_group;
      
      accel_group = slist->data;
      accel_group->acceleratables = b_slist_remove (accel_group->acceleratables, stale_object);
      g_object_unref (accel_group);
    }
  b_slist_free (free_list);
  g_object_set_qdata (stale_object, quark_acceleratable_groups, NULL);
}

void
_btk_accel_group_attach (BtkAccelGroup *accel_group,
			 BObject       *object)
{
  GSList *slist;
  
  g_return_if_fail (BTK_IS_ACCEL_GROUP (accel_group));
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (b_slist_find (accel_group->acceleratables, object) == NULL);
  
  g_object_ref (accel_group);
  accel_group->acceleratables = b_slist_prepend (accel_group->acceleratables, object);
  slist = g_object_get_qdata (object, quark_acceleratable_groups);
  if (slist)
    g_object_weak_unref (object,
			 (GWeakNotify) accel_group_weak_ref_detach,
			 slist);
  slist = b_slist_prepend (slist, accel_group);
  g_object_set_qdata (object, quark_acceleratable_groups, slist);
  g_object_weak_ref (object,
		     (GWeakNotify) accel_group_weak_ref_detach,
		     slist);
}

void
_btk_accel_group_detach (BtkAccelGroup *accel_group,
			 BObject       *object)
{
  GSList *slist;
  
  g_return_if_fail (BTK_IS_ACCEL_GROUP (accel_group));
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (b_slist_find (accel_group->acceleratables, object) != NULL);
  
  accel_group->acceleratables = b_slist_remove (accel_group->acceleratables, object);
  slist = g_object_get_qdata (object, quark_acceleratable_groups);
  g_object_weak_unref (object,
		       (GWeakNotify) accel_group_weak_ref_detach,
		       slist);
  slist = b_slist_remove (slist, accel_group);
  g_object_set_qdata (object, quark_acceleratable_groups, slist);
  if (slist)
    g_object_weak_ref (object,
		       (GWeakNotify) accel_group_weak_ref_detach,
		       slist);
  g_object_unref (accel_group);
}

/**
 * btk_accel_groups_from_object:
 * @object:        a #BObject, usually a #BtkWindow
 *
 * Gets a list of all accel groups which are attached to @object.
 *
 * Returns: (element-type BtkAccelGroup) (transfer none): a list of all accel groups which are attached to @object
 */
GSList*
btk_accel_groups_from_object (BObject *object)
{
  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  
  return g_object_get_qdata (object, quark_acceleratable_groups);
}

/**
 * btk_accel_group_find:
 * @accel_group: a #BtkAccelGroup
 * @find_func: a function to filter the entries of @accel_group with
 * @data: data to pass to @find_func
 * @returns: (transfer none): the key of the first entry passing
 *    @find_func. The key is owned by BTK+ and must not be freed.
 *
 * Finds the first entry in an accelerator group for which 
 * @find_func returns %TRUE and returns its #BtkAccelKey.
 *
 */
BtkAccelKey*
btk_accel_group_find (BtkAccelGroup        *accel_group,
		      BtkAccelGroupFindFunc find_func,
		      gpointer              data)
{
  BtkAccelKey *key = NULL;
  guint i;

  g_return_val_if_fail (BTK_IS_ACCEL_GROUP (accel_group), NULL);
  g_return_val_if_fail (find_func != NULL, NULL);

  g_object_ref (accel_group);
  for (i = 0; i < accel_group->n_accels; i++)
    if (find_func (&accel_group->priv_accels[i].key,
		   accel_group->priv_accels[i].closure,
		   data))
      {
	key = &accel_group->priv_accels[i].key;
	break;
      }
  g_object_unref (accel_group);

  return key;
}

/**
 * btk_accel_group_lock:
 * @accel_group: a #BtkAccelGroup
 * 
 * Locks the given accelerator group.
 *
 * Locking an acelerator group prevents the accelerators contained
 * within it to be changed during runtime. Refer to
 * btk_accel_map_change_entry() about runtime accelerator changes.
 *
 * If called more than once, @accel_group remains locked until
 * btk_accel_group_unlock() has been called an equivalent number
 * of times.
 */
void
btk_accel_group_lock (BtkAccelGroup *accel_group)
{
  g_return_if_fail (BTK_IS_ACCEL_GROUP (accel_group));
  
  accel_group->lock_count += 1;

  if (accel_group->lock_count == 1) {
    /* State change from unlocked to locked */
    g_object_notify (B_OBJECT (accel_group), "is-locked");
  }
}

/**
 * btk_accel_group_unlock:
 * @accel_group: a #BtkAccelGroup
 * 
 * Undoes the last call to btk_accel_group_lock() on this @accel_group.
 */
void
btk_accel_group_unlock (BtkAccelGroup *accel_group)
{
  g_return_if_fail (BTK_IS_ACCEL_GROUP (accel_group));
  g_return_if_fail (accel_group->lock_count > 0);

  accel_group->lock_count -= 1;

  if (accel_group->lock_count < 1) {
    /* State change from locked to unlocked */
    g_object_notify (B_OBJECT (accel_group), "is-locked");
  }
}

static void
accel_closure_invalidate (gpointer  data,
			  GClosure *closure)
{
  BtkAccelGroup *accel_group = BTK_ACCEL_GROUP (data);

  btk_accel_group_disconnect (accel_group, closure);
}

static int
bsearch_compare_accels (const void *d1,
			const void *d2)
{
  const BtkAccelGroupEntry *entry1 = d1;
  const BtkAccelGroupEntry *entry2 = d2;

  if (entry1->key.accel_key == entry2->key.accel_key)
    return entry1->key.accel_mods < entry2->key.accel_mods ? -1 : entry1->key.accel_mods > entry2->key.accel_mods;
  else
    return entry1->key.accel_key < entry2->key.accel_key ? -1 : 1;
}

static void
quick_accel_add (BtkAccelGroup  *accel_group,
		 guint           accel_key,
		 BdkModifierType accel_mods,
		 BtkAccelFlags   accel_flags,
		 GClosure       *closure,
		 GQuark          path_quark)
{
  guint pos, i = accel_group->n_accels++;
  BtkAccelGroupEntry key;

  /* find position */
  key.key.accel_key = accel_key;
  key.key.accel_mods = accel_mods;
  for (pos = 0; pos < i; pos++)
    if (bsearch_compare_accels (&key, accel_group->priv_accels + pos) < 0)
      break;

  /* insert at position, ref closure */
  accel_group->priv_accels = g_renew (BtkAccelGroupEntry, accel_group->priv_accels, accel_group->n_accels);
  g_memmove (accel_group->priv_accels + pos + 1, accel_group->priv_accels + pos,
	     (i - pos) * sizeof (accel_group->priv_accels[0]));
  accel_group->priv_accels[pos].key.accel_key = accel_key;
  accel_group->priv_accels[pos].key.accel_mods = accel_mods;
  accel_group->priv_accels[pos].key.accel_flags = accel_flags;
  accel_group->priv_accels[pos].closure = g_closure_ref (closure);
  accel_group->priv_accels[pos].accel_path_quark = path_quark;
  g_closure_sink (closure);
  
  /* handle closure invalidation and reverse lookups */
  g_closure_add_invalidate_notifier (closure, accel_group, accel_closure_invalidate);

  /* get accel path notification */
  if (path_quark)
    _btk_accel_map_add_group (g_quark_to_string (path_quark), accel_group);

  /* connect and notify changed */
  if (accel_key)
    {
      gchar *accel_name = btk_accelerator_name (accel_key, accel_mods);
      GQuark accel_quark = g_quark_from_string (accel_name);

      g_free (accel_name);
      
      /* setup handler */
      g_signal_connect_closure_by_id (accel_group, signal_accel_activate, accel_quark, closure, FALSE);
      
      /* and notify */
      g_signal_emit (accel_group, signal_accel_changed, accel_quark, accel_key, accel_mods, closure);
    }
}

static void
quick_accel_remove (BtkAccelGroup      *accel_group,
                    guint               pos)
{
  GQuark accel_quark = 0;
  BtkAccelGroupEntry *entry = accel_group->priv_accels + pos;
  guint accel_key = entry->key.accel_key;
  BdkModifierType accel_mods = entry->key.accel_mods;
  GClosure *closure = entry->closure;

  /* quark for notification */
  if (accel_key)
    {
      gchar *accel_name = btk_accelerator_name (accel_key, accel_mods);

      accel_quark = g_quark_from_string (accel_name);
      g_free (accel_name);
    }

  /* clean up closure invalidate notification and disconnect */
  g_closure_remove_invalidate_notifier (entry->closure, accel_group, accel_closure_invalidate);
  if (accel_quark)
    g_signal_handlers_disconnect_matched (accel_group,
					  G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_CLOSURE,
					  signal_accel_activate, accel_quark,
					  closure, NULL, NULL);
  /* clean up accel path notification */
  if (entry->accel_path_quark)
    _btk_accel_map_remove_group (g_quark_to_string (entry->accel_path_quark), accel_group);

  /* physically remove */
  accel_group->n_accels -= 1;
  g_memmove (entry, entry + 1,
	     (accel_group->n_accels - pos) * sizeof (accel_group->priv_accels[0]));

  /* and notify */
  if (accel_quark)
    g_signal_emit (accel_group, signal_accel_changed, accel_quark, accel_key, accel_mods, closure);

  /* remove quick_accel_add() refcount */
  g_closure_unref (closure);
}

static BtkAccelGroupEntry*
quick_accel_find (BtkAccelGroup  *accel_group,
		  guint           accel_key,
		  BdkModifierType accel_mods,
		  guint		 *count_p)
{
  BtkAccelGroupEntry *entry;
  BtkAccelGroupEntry key;

  *count_p = 0;

  if (!accel_group->n_accels)
    return NULL;

  key.key.accel_key = accel_key;
  key.key.accel_mods = accel_mods;
  entry = bsearch (&key, accel_group->priv_accels, accel_group->n_accels,
		   sizeof (accel_group->priv_accels[0]), bsearch_compare_accels);
  
  if (!entry)
    return NULL;

  /* step back to the first member */
  for (; entry > accel_group->priv_accels; entry--)
    if (entry[-1].key.accel_key != accel_key ||
	entry[-1].key.accel_mods != accel_mods)
      break;
  /* count equal members */
  for (; entry + *count_p < accel_group->priv_accels + accel_group->n_accels; (*count_p)++)
    if (entry[*count_p].key.accel_key != accel_key ||
	entry[*count_p].key.accel_mods != accel_mods)
      break;
  return entry;
}

/**
 * btk_accel_group_connect:
 * @accel_group:      the accelerator group to install an accelerator in
 * @accel_key:        key value of the accelerator
 * @accel_mods:       modifier combination of the accelerator
 * @accel_flags:      a flag mask to configure this accelerator
 * @closure:          closure to be executed upon accelerator activation
 *
 * Installs an accelerator in this group. When @accel_group is being activated
 * in response to a call to btk_accel_groups_activate(), @closure will be
 * invoked if the @accel_key and @accel_mods from btk_accel_groups_activate()
 * match those of this connection.
 *
 * The signature used for the @closure is that of #BtkAccelGroupActivate.
 * 
 * Note that, due to implementation details, a single closure can only be
 * connected to one accelerator group.
 */
void
btk_accel_group_connect (BtkAccelGroup	*accel_group,
			 guint		 accel_key,
			 BdkModifierType accel_mods,
			 BtkAccelFlags	 accel_flags,
			 GClosure	*closure)
{
  g_return_if_fail (BTK_IS_ACCEL_GROUP (accel_group));
  g_return_if_fail (closure != NULL);
  g_return_if_fail (accel_key > 0);
  g_return_if_fail (btk_accel_group_from_accel_closure (closure) == NULL);

  g_object_ref (accel_group);
  if (!closure->is_invalid)
    quick_accel_add (accel_group,
		     bdk_keyval_to_lower (accel_key),
		     accel_mods, accel_flags, closure, 0);
  g_object_unref (accel_group);
}

/**
 * btk_accel_group_connect_by_path:
 * @accel_group:      the accelerator group to install an accelerator in
 * @accel_path:       path used for determining key and modifiers.
 * @closure:          closure to be executed upon accelerator activation
 *
 * Installs an accelerator in this group, using an accelerator path to look
 * up the appropriate key and modifiers (see btk_accel_map_add_entry()).
 * When @accel_group is being activated in response to a call to
 * btk_accel_groups_activate(), @closure will be invoked if the @accel_key and
 * @accel_mods from btk_accel_groups_activate() match the key and modifiers
 * for the path.
 *
 * The signature used for the @closure is that of #BtkAccelGroupActivate.
 * 
 * Note that @accel_path string will be stored in a #GQuark. Therefore, if you
 * pass a static string, you can save some memory by interning it first with 
 * g_intern_static_string().
 */
void
btk_accel_group_connect_by_path (BtkAccelGroup	*accel_group,
				 const gchar    *accel_path,
				 GClosure	*closure)
{
  guint accel_key = 0;
  BdkModifierType accel_mods = 0;
  BtkAccelKey key;

  g_return_if_fail (BTK_IS_ACCEL_GROUP (accel_group));
  g_return_if_fail (closure != NULL);
  g_return_if_fail (_btk_accel_path_is_valid (accel_path));

  if (closure->is_invalid)
    return;

  g_object_ref (accel_group);

  if (btk_accel_map_lookup_entry (accel_path, &key))
    {
      accel_key = bdk_keyval_to_lower (key.accel_key);
      accel_mods = key.accel_mods;
    }

  quick_accel_add (accel_group, accel_key, accel_mods, BTK_ACCEL_VISIBLE, closure,
		   g_quark_from_string (accel_path));

  g_object_unref (accel_group);
}

/**
 * btk_accel_group_disconnect:
 * @accel_group: the accelerator group to remove an accelerator from
 * @closure: (allow-none):     the closure to remove from this accelerator group, or %NULL
 *               to remove all closures
 * @returns:     %TRUE if the closure was found and got disconnected
 *
 * Removes an accelerator previously installed through
 * btk_accel_group_connect().
 *
 * Since 2.20 @closure can be %NULL.
 */
gboolean
btk_accel_group_disconnect (BtkAccelGroup *accel_group,
			    GClosure      *closure)
{
  guint i;

  g_return_val_if_fail (BTK_IS_ACCEL_GROUP (accel_group), FALSE);

  for (i = 0; i < accel_group->n_accels; i++)
    if (accel_group->priv_accels[i].closure == closure || !closure)
      {
	g_object_ref (accel_group);
	quick_accel_remove (accel_group, i);
	g_object_unref (accel_group);
	return TRUE;
      }
  return FALSE;
}

/**
 * btk_accel_group_disconnect_key:
 * @accel_group:      the accelerator group to install an accelerator in
 * @accel_key:        key value of the accelerator
 * @accel_mods:       modifier combination of the accelerator
 * @returns:          %TRUE if there was an accelerator which could be 
 *                    removed, %FALSE otherwise
 *
 * Removes an accelerator previously installed through
 * btk_accel_group_connect().
 */
gboolean
btk_accel_group_disconnect_key (BtkAccelGroup  *accel_group,
				guint	        accel_key,
				BdkModifierType accel_mods)
{
  BtkAccelGroupEntry *entries;
  GSList *slist, *clist = NULL;
  gboolean removed_one = FALSE;
  guint n;

  g_return_val_if_fail (BTK_IS_ACCEL_GROUP (accel_group), FALSE);

  g_object_ref (accel_group);
  
  accel_key = bdk_keyval_to_lower (accel_key);
  entries = quick_accel_find (accel_group, accel_key, accel_mods, &n);
  while (n--)
    {
      GClosure *closure = g_closure_ref (entries[n].closure);

      clist = b_slist_prepend (clist, closure);
    }

  for (slist = clist; slist; slist = slist->next)
    {
      GClosure *closure = slist->data;

      removed_one |= btk_accel_group_disconnect (accel_group, closure);
      g_closure_unref (closure);
    }
  b_slist_free (clist);

  g_object_unref (accel_group);

  return removed_one;
}

void
_btk_accel_group_reconnect (BtkAccelGroup *accel_group,
			    GQuark         accel_path_quark)
{
  GSList *slist, *clist = NULL;
  guint i;

  g_return_if_fail (BTK_IS_ACCEL_GROUP (accel_group));

  g_object_ref (accel_group);

  for (i = 0; i < accel_group->n_accels; i++)
    if (accel_group->priv_accels[i].accel_path_quark == accel_path_quark)
      {
	GClosure *closure = g_closure_ref (accel_group->priv_accels[i].closure);

	clist = b_slist_prepend (clist, closure);
      }

  for (slist = clist; slist; slist = slist->next)
    {
      GClosure *closure = slist->data;

      btk_accel_group_disconnect (accel_group, closure);
      btk_accel_group_connect_by_path (accel_group, g_quark_to_string (accel_path_quark), closure);
      g_closure_unref (closure);
    }
  b_slist_free (clist);

  g_object_unref (accel_group);
}

/**
 * btk_accel_group_query:
 * @accel_group:      the accelerator group to query
 * @accel_key:        key value of the accelerator
 * @accel_mods:       modifier combination of the accelerator
 * @n_entries: (allow-none):        location to return the number of entries found, or %NULL
 * @returns: (allow-none):          an array of @n_entries #BtkAccelGroupEntry elements, or %NULL. The array is owned by BTK+ and must not be freed. 
 *
 * Queries an accelerator group for all entries matching @accel_key and 
 * @accel_mods.
 */
BtkAccelGroupEntry*
btk_accel_group_query (BtkAccelGroup  *accel_group,
		       guint           accel_key,
		       BdkModifierType accel_mods,
		       guint          *n_entries)
{
  BtkAccelGroupEntry *entries;
  guint n;

  g_return_val_if_fail (BTK_IS_ACCEL_GROUP (accel_group), NULL);

  entries = quick_accel_find (accel_group, bdk_keyval_to_lower (accel_key), accel_mods, &n);

  if (n_entries)
    *n_entries = entries ? n : 0;

  return entries;
}

/**
 * btk_accel_group_from_accel_closure:
 * @closure: a #GClosure
 * @returns: (transfer none): the #BtkAccelGroup to which @closure
 *     is connected, or %NULL.
 *
 * Finds the #BtkAccelGroup to which @closure is connected; 
 * see btk_accel_group_connect().
 */
BtkAccelGroup*
btk_accel_group_from_accel_closure (GClosure *closure)
{
  guint i;

  g_return_val_if_fail (closure != NULL, NULL);

  /* a few remarks on what we do here. in general, we need a way to reverse lookup
   * accel_groups from closures that are being used in accel groups. this could
   * be done e.g via a hashtable. it is however cheaper (memory wise) to just
   * use the invalidation notifier on the closure itself (which we need to install
   * anyway), that contains the accel group as data which, besides needing to peek
   * a bit at closure internals, works just as good.
   */
  for (i = 0; i < G_CLOSURE_N_NOTIFIERS (closure); i++)
    if (closure->notifiers[i].notify == accel_closure_invalidate)
      return closure->notifiers[i].data;

  return NULL;
}

/**
 * btk_accel_group_activate:
 * @accel_group:   a #BtkAccelGroup
 * @accel_quark:   the quark for the accelerator name
 * @acceleratable: the #BObject, usually a #BtkWindow, on which
 *                 to activate the accelerator.
 * @accel_key:     accelerator keyval from a key event
 * @accel_mods:    keyboard state mask from a key event
 * 
 * Finds the first accelerator in @accel_group 
 * that matches @accel_key and @accel_mods, and
 * activates it.
 *
 * Returns: %TRUE if an accelerator was activated and handled this keypress
 */
gboolean
btk_accel_group_activate (BtkAccelGroup   *accel_group,
                          GQuark	   accel_quark,
                          BObject	  *acceleratable,
                          guint	           accel_key,
                          BdkModifierType  accel_mods)
{
  gboolean was_handled;

  g_return_val_if_fail (BTK_IS_ACCEL_GROUP (accel_group), FALSE);
  g_return_val_if_fail (G_IS_OBJECT (acceleratable), FALSE);
  
  was_handled = FALSE;
  g_signal_emit (accel_group, signal_accel_activate, accel_quark,
		 acceleratable, accel_key, accel_mods, &was_handled);

  return was_handled;
}

/**
 * btk_accel_groups_activate:
 * @object:        the #BObject, usually a #BtkWindow, on which
 *                 to activate the accelerator.
 * @accel_key:     accelerator keyval from a key event
 * @accel_mods:    keyboard state mask from a key event
 * 
 * Finds the first accelerator in any #BtkAccelGroup attached
 * to @object that matches @accel_key and @accel_mods, and
 * activates that accelerator.
 *
 * Returns: %TRUE if an accelerator was activated and handled this keypress
 */
gboolean
btk_accel_groups_activate (BObject	  *object,
			   guint	   accel_key,
			   BdkModifierType accel_mods)
{
  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);
  
  if (btk_accelerator_valid (accel_key, accel_mods))
    {
      gchar *accel_name;
      GQuark accel_quark;
      GSList *slist;

      accel_name = btk_accelerator_name (accel_key, (accel_mods & btk_accelerator_get_default_mod_mask ()));
      accel_quark = g_quark_from_string (accel_name);
      g_free (accel_name);
      
      for (slist = btk_accel_groups_from_object (object); slist; slist = slist->next)
	if (btk_accel_group_activate (slist->data, accel_quark, object, accel_key, accel_mods))
	  return TRUE;
    }
  
  return FALSE;
}

/**
 * btk_accelerator_valid:
 * @keyval:    a BDK keyval
 * @modifiers: modifier mask
 * @returns:   %TRUE if the accelerator is valid
 * 
 * Determines whether a given keyval and modifier mask constitute
 * a valid keyboard accelerator. For example, the #BDK_a keyval
 * plus #BDK_CONTROL_MASK is valid - this is a "Ctrl+a" accelerator.
 * But, you can't, for instance, use the #BDK_Control_L keyval
 * as an accelerator.
 */
gboolean
btk_accelerator_valid (guint		  keyval,
		       BdkModifierType	  modifiers)
{
  static const guint invalid_accelerator_vals[] = {
    BDK_Shift_L, BDK_Shift_R, BDK_Shift_Lock, BDK_Caps_Lock, BDK_ISO_Lock,
    BDK_Control_L, BDK_Control_R, BDK_Meta_L, BDK_Meta_R,
    BDK_Alt_L, BDK_Alt_R, BDK_Super_L, BDK_Super_R, BDK_Hyper_L, BDK_Hyper_R,
    BDK_ISO_Level3_Shift, BDK_ISO_Next_Group, BDK_ISO_Prev_Group,
    BDK_ISO_First_Group, BDK_ISO_Last_Group,
    BDK_Mode_switch, BDK_Num_Lock, BDK_Multi_key,
    BDK_Scroll_Lock, BDK_Sys_Req, 
    BDK_Tab, BDK_ISO_Left_Tab, BDK_KP_Tab,
    BDK_First_Virtual_Screen, BDK_Prev_Virtual_Screen,
    BDK_Next_Virtual_Screen, BDK_Last_Virtual_Screen,
    BDK_Terminate_Server, BDK_AudibleBell_Enable,
    0
  };
  static const guint invalid_unmodified_vals[] = {
    BDK_Up, BDK_Down, BDK_Left, BDK_Right,
    BDK_KP_Up, BDK_KP_Down, BDK_KP_Left, BDK_KP_Right,
    0
  };
  const guint *ac_val;

  modifiers &= BDK_MODIFIER_MASK;
    
  if (keyval <= 0xFF)
    return keyval >= 0x20;

  ac_val = invalid_accelerator_vals;
  while (*ac_val)
    {
      if (keyval == *ac_val++)
	return FALSE;
    }

  if (!modifiers)
    {
      ac_val = invalid_unmodified_vals;
      while (*ac_val)
	{
	  if (keyval == *ac_val++)
	    return FALSE;
	}
    }
  
  return TRUE;
}

static inline gboolean
is_alt (const gchar *string)
{
  return ((string[0] == '<') &&
	  (string[1] == 'a' || string[1] == 'A') &&
	  (string[2] == 'l' || string[2] == 'L') &&
	  (string[3] == 't' || string[3] == 'T') &&
	  (string[4] == '>'));
}

static inline gboolean
is_ctl (const gchar *string)
{
  return ((string[0] == '<') &&
	  (string[1] == 'c' || string[1] == 'C') &&
	  (string[2] == 't' || string[2] == 'T') &&
	  (string[3] == 'l' || string[3] == 'L') &&
	  (string[4] == '>'));
}

static inline gboolean
is_modx (const gchar *string)
{
  return ((string[0] == '<') &&
	  (string[1] == 'm' || string[1] == 'M') &&
	  (string[2] == 'o' || string[2] == 'O') &&
	  (string[3] == 'd' || string[3] == 'D') &&
	  (string[4] >= '1' && string[4] <= '5') &&
	  (string[5] == '>'));
}

static inline gboolean
is_ctrl (const gchar *string)
{
  return ((string[0] == '<') &&
	  (string[1] == 'c' || string[1] == 'C') &&
	  (string[2] == 't' || string[2] == 'T') &&
	  (string[3] == 'r' || string[3] == 'R') &&
	  (string[4] == 'l' || string[4] == 'L') &&
	  (string[5] == '>'));
}

static inline gboolean
is_shft (const gchar *string)
{
  return ((string[0] == '<') &&
	  (string[1] == 's' || string[1] == 'S') &&
	  (string[2] == 'h' || string[2] == 'H') &&
	  (string[3] == 'f' || string[3] == 'F') &&
	  (string[4] == 't' || string[4] == 'T') &&
	  (string[5] == '>'));
}

static inline gboolean
is_shift (const gchar *string)
{
  return ((string[0] == '<') &&
	  (string[1] == 's' || string[1] == 'S') &&
	  (string[2] == 'h' || string[2] == 'H') &&
	  (string[3] == 'i' || string[3] == 'I') &&
	  (string[4] == 'f' || string[4] == 'F') &&
	  (string[5] == 't' || string[5] == 'T') &&
	  (string[6] == '>'));
}

static inline gboolean
is_control (const gchar *string)
{
  return ((string[0] == '<') &&
	  (string[1] == 'c' || string[1] == 'C') &&
	  (string[2] == 'o' || string[2] == 'O') &&
	  (string[3] == 'n' || string[3] == 'N') &&
	  (string[4] == 't' || string[4] == 'T') &&
	  (string[5] == 'r' || string[5] == 'R') &&
	  (string[6] == 'o' || string[6] == 'O') &&
	  (string[7] == 'l' || string[7] == 'L') &&
	  (string[8] == '>'));
}

static inline gboolean
is_release (const gchar *string)
{
  return ((string[0] == '<') &&
	  (string[1] == 'r' || string[1] == 'R') &&
	  (string[2] == 'e' || string[2] == 'E') &&
	  (string[3] == 'l' || string[3] == 'L') &&
	  (string[4] == 'e' || string[4] == 'E') &&
	  (string[5] == 'a' || string[5] == 'A') &&
	  (string[6] == 's' || string[6] == 'S') &&
	  (string[7] == 'e' || string[7] == 'E') &&
	  (string[8] == '>'));
}

static inline gboolean
is_meta (const gchar *string)
{
  return ((string[0] == '<') &&
	  (string[1] == 'm' || string[1] == 'M') &&
	  (string[2] == 'e' || string[2] == 'E') &&
	  (string[3] == 't' || string[3] == 'T') &&
	  (string[4] == 'a' || string[4] == 'A') &&
	  (string[5] == '>'));
}

static inline gboolean
is_super (const gchar *string)
{
  return ((string[0] == '<') &&
	  (string[1] == 's' || string[1] == 'S') &&
	  (string[2] == 'u' || string[2] == 'U') &&
	  (string[3] == 'p' || string[3] == 'P') &&
	  (string[4] == 'e' || string[4] == 'E') &&
	  (string[5] == 'r' || string[5] == 'R') &&
	  (string[6] == '>'));
}

static inline gboolean
is_hyper (const gchar *string)
{
  return ((string[0] == '<') &&
	  (string[1] == 'h' || string[1] == 'H') &&
	  (string[2] == 'y' || string[2] == 'Y') &&
	  (string[3] == 'p' || string[3] == 'P') &&
	  (string[4] == 'e' || string[4] == 'E') &&
	  (string[5] == 'r' || string[5] == 'R') &&
	  (string[6] == '>'));
}

static inline gboolean
is_primary (const gchar *string)
{
  return ((string[0] == '<') &&
	  (string[1] == 'p' || string[1] == 'P') &&
	  (string[2] == 'r' || string[2] == 'R') &&
	  (string[3] == 'i' || string[3] == 'I') &&
	  (string[4] == 'm' || string[4] == 'M') &&
	  (string[5] == 'a' || string[5] == 'A') &&
	  (string[6] == 'r' || string[6] == 'R') &&
	  (string[7] == 'y' || string[7] == 'Y') &&
	  (string[8] == '>'));
}

/**
 * btk_accelerator_parse:
 * @accelerator:      string representing an accelerator
 * @accelerator_key: (out) (allow-none): return location for accelerator keyval
 * @accelerator_mods: (out) (allow-none): return location for accelerator modifier mask
 *
 * Parses a string representing an accelerator. The
 * format looks like "&lt;Control&gt;a" or "&lt;Shift&gt;&lt;Alt&gt;F1" or
 * "&lt;Release&gt;z" (the last one is for key release).
 * The parser is fairly liberal and allows lower or upper case,
 * and also abbreviations such as "&lt;Ctl&gt;" and "&lt;Ctrl&gt;".
 * Key names are parsed using bdk_keyval_from_name(). For character keys the
 * name is not the symbol, but the lowercase name, e.g. one would use
 * "&lt;Ctrl&gt;minus" instead of "&lt;Ctrl&gt;-".
 *
 * If the parse fails, @accelerator_key and @accelerator_mods will
 * be set to 0 (zero).
 */
void
btk_accelerator_parse (const gchar     *accelerator,
		       guint           *accelerator_key,
		       BdkModifierType *accelerator_mods)
{
  guint keyval;
  BdkModifierType mods;
  gint len;
  
  if (accelerator_key)
    *accelerator_key = 0;
  if (accelerator_mods)
    *accelerator_mods = 0;
  g_return_if_fail (accelerator != NULL);
  
  keyval = 0;
  mods = 0;
  len = strlen (accelerator);
  while (len)
    {
      if (*accelerator == '<')
	{
	  if (len >= 9 && is_release (accelerator))
	    {
	      accelerator += 9;
	      len -= 9;
	      mods |= BDK_RELEASE_MASK;
	    }
	  else if (len >= 9 && is_primary (accelerator))
	    {
	      accelerator += 9;
	      len -= 9;
	      mods |= BTK_DEFAULT_ACCEL_MOD_MASK_VIRTUAL;
	    }
	  else if (len >= 9 && is_control (accelerator))
	    {
	      accelerator += 9;
	      len -= 9;
	      mods |= BDK_CONTROL_MASK;
	    }
	  else if (len >= 7 && is_shift (accelerator))
	    {
	      accelerator += 7;
	      len -= 7;
	      mods |= BDK_SHIFT_MASK;
	    }
	  else if (len >= 6 && is_shft (accelerator))
	    {
	      accelerator += 6;
	      len -= 6;
	      mods |= BDK_SHIFT_MASK;
	    }
	  else if (len >= 6 && is_ctrl (accelerator))
	    {
	      accelerator += 6;
	      len -= 6;
	      mods |= BDK_CONTROL_MASK;
	    }
	  else if (len >= 6 && is_modx (accelerator))
	    {
	      static const guint mod_vals[] = {
		BDK_MOD1_MASK, BDK_MOD2_MASK, BDK_MOD3_MASK,
		BDK_MOD4_MASK, BDK_MOD5_MASK
	      };

	      len -= 6;
	      accelerator += 4;
	      mods |= mod_vals[*accelerator - '1'];
	      accelerator += 2;
	    }
	  else if (len >= 5 && is_ctl (accelerator))
	    {
	      accelerator += 5;
	      len -= 5;
	      mods |= BDK_CONTROL_MASK;
	    }
	  else if (len >= 5 && is_alt (accelerator))
	    {
	      accelerator += 5;
	      len -= 5;
	      mods |= BDK_MOD1_MASK;
	    }
          else if (len >= 6 && is_meta (accelerator))
	    {
	      accelerator += 6;
	      len -= 6;
	      mods |= BDK_META_MASK;
	    }
          else if (len >= 7 && is_hyper (accelerator))
	    {
	      accelerator += 7;
	      len -= 7;
	      mods |= BDK_HYPER_MASK;
	    }
          else if (len >= 7 && is_super (accelerator))
	    {
	      accelerator += 7;
	      len -= 7;
	      mods |= BDK_SUPER_MASK;
	    }
	  else
	    {
	      gchar last_ch;
	      
	      last_ch = *accelerator;
	      while (last_ch && last_ch != '>')
		{
		  last_ch = *accelerator;
		  accelerator += 1;
		  len -= 1;
		}
	    }
	}
      else
	{
	  keyval = bdk_keyval_from_name (accelerator);
	  accelerator += len;
	  len -= len;
	}
    }
  
  if (accelerator_key)
    *accelerator_key = bdk_keyval_to_lower (keyval);
  if (accelerator_mods)
    *accelerator_mods = mods;
}

/**
 * btk_accelerator_name:
 * @accelerator_key:  accelerator keyval
 * @accelerator_mods: accelerator modifier mask
 * 
 * Converts an accelerator keyval and modifier mask
 * into a string parseable by btk_accelerator_parse().
 * For example, if you pass in #BDK_q and #BDK_CONTROL_MASK,
 * this function returns "&lt;Control&gt;q". 
 *
 * If you need to display accelerators in the user interface,
 * see btk_accelerator_get_label().
 *
 * Returns: a newly-allocated accelerator name
 */
gchar*
btk_accelerator_name (guint           accelerator_key,
		      BdkModifierType accelerator_mods)
{
  static const gchar text_release[] = "<Release>";
  static const gchar text_primary[] = "<Primary>";
  static const gchar text_shift[] = "<Shift>";
  static const gchar text_control[] = "<Control>";
  static const gchar text_mod1[] = "<Alt>";
  static const gchar text_mod2[] = "<Mod2>";
  static const gchar text_mod3[] = "<Mod3>";
  static const gchar text_mod4[] = "<Mod4>";
  static const gchar text_mod5[] = "<Mod5>";
  static const gchar text_meta[] = "<Meta>";
  static const gchar text_super[] = "<Super>";
  static const gchar text_hyper[] = "<Hyper>";
  BdkModifierType saved_mods;
  guint l;
  gchar *keyval_name;
  gchar *accelerator;

  accelerator_mods &= BDK_MODIFIER_MASK;

  keyval_name = bdk_keyval_name (bdk_keyval_to_lower (accelerator_key));
  if (!keyval_name)
    keyval_name = "";

  saved_mods = accelerator_mods;
  l = 0;
  if (accelerator_mods & BDK_RELEASE_MASK)
    l += sizeof (text_release) - 1;
  if (accelerator_mods & BTK_DEFAULT_ACCEL_MOD_MASK_VIRTUAL)
    {
      l += sizeof (text_primary) - 1;
      accelerator_mods &= ~BTK_DEFAULT_ACCEL_MOD_MASK_VIRTUAL; /* consume the default accel */
    }
  if (accelerator_mods & BDK_SHIFT_MASK)
    l += sizeof (text_shift) - 1;
  if (accelerator_mods & BDK_CONTROL_MASK)
    l += sizeof (text_control) - 1;
  if (accelerator_mods & BDK_MOD1_MASK)
    l += sizeof (text_mod1) - 1;
  if (accelerator_mods & BDK_MOD2_MASK)
    l += sizeof (text_mod2) - 1;
  if (accelerator_mods & BDK_MOD3_MASK)
    l += sizeof (text_mod3) - 1;
  if (accelerator_mods & BDK_MOD4_MASK)
    l += sizeof (text_mod4) - 1;
  if (accelerator_mods & BDK_MOD5_MASK)
    l += sizeof (text_mod5) - 1;
  l += strlen (keyval_name);
  if (accelerator_mods & BDK_META_MASK)
    l += sizeof (text_meta) - 1;
  if (accelerator_mods & BDK_HYPER_MASK)
    l += sizeof (text_hyper) - 1;
  if (accelerator_mods & BDK_SUPER_MASK)
    l += sizeof (text_super) - 1;

  accelerator = g_new (gchar, l + 1);

  accelerator_mods = saved_mods;
  l = 0;
  accelerator[l] = 0;
  if (accelerator_mods & BDK_RELEASE_MASK)
    {
      strcpy (accelerator + l, text_release);
      l += sizeof (text_release) - 1;
    }
  if (accelerator_mods & BTK_DEFAULT_ACCEL_MOD_MASK_VIRTUAL)
    {
      strcpy (accelerator + l, text_primary);
      l += sizeof (text_primary) - 1;
      accelerator_mods &= ~BTK_DEFAULT_ACCEL_MOD_MASK_VIRTUAL; /* consume the default accel */
    }
  if (accelerator_mods & BDK_SHIFT_MASK)
    {
      strcpy (accelerator + l, text_shift);
      l += sizeof (text_shift) - 1;
    }
  if (accelerator_mods & BDK_CONTROL_MASK)
    {
      strcpy (accelerator + l, text_control);
      l += sizeof (text_control) - 1;
    }
  if (accelerator_mods & BDK_MOD1_MASK)
    {
      strcpy (accelerator + l, text_mod1);
      l += sizeof (text_mod1) - 1;
    }
  if (accelerator_mods & BDK_MOD2_MASK)
    {
      strcpy (accelerator + l, text_mod2);
      l += sizeof (text_mod2) - 1;
    }
  if (accelerator_mods & BDK_MOD3_MASK)
    {
      strcpy (accelerator + l, text_mod3);
      l += sizeof (text_mod3) - 1;
    }
  if (accelerator_mods & BDK_MOD4_MASK)
    {
      strcpy (accelerator + l, text_mod4);
      l += sizeof (text_mod4) - 1;
    }
  if (accelerator_mods & BDK_MOD5_MASK)
    {
      strcpy (accelerator + l, text_mod5);
      l += sizeof (text_mod5) - 1;
    }
  if (accelerator_mods & BDK_META_MASK)
    {
      strcpy (accelerator + l, text_meta);
      l += sizeof (text_meta) - 1;
    }
  if (accelerator_mods & BDK_HYPER_MASK)
    {
      strcpy (accelerator + l, text_hyper);
      l += sizeof (text_hyper) - 1;
    }
  if (accelerator_mods & BDK_SUPER_MASK)
    {
      strcpy (accelerator + l, text_super);
      l += sizeof (text_super) - 1;
    }
  strcpy (accelerator + l, keyval_name);

  return accelerator;
}

/**
 * btk_accelerator_get_label:
 * @accelerator_key:  accelerator keyval
 * @accelerator_mods: accelerator modifier mask
 * 
 * Converts an accelerator keyval and modifier mask into a string 
 * which can be used to represent the accelerator to the user. 
 *
 * Returns: a newly-allocated string representing the accelerator.
 *
 * Since: 2.6
 */
gchar*
btk_accelerator_get_label (guint           accelerator_key,
			   BdkModifierType accelerator_mods)
{
  BtkAccelLabelClass *klass;
  gchar *label;

  klass = g_type_class_ref (BTK_TYPE_ACCEL_LABEL);
  label = _btk_accel_label_class_get_accelerator_label (klass, 
							accelerator_key, 
							accelerator_mods);
  g_type_class_unref (klass); /* klass is kept alive since btk uses static types */

  return label;
}  

/**
 * btk_accelerator_set_default_mod_mask:
 * @default_mod_mask: accelerator modifier mask
 *
 * Sets the modifiers that will be considered significant for keyboard
 * accelerators. The default mod mask is #BDK_CONTROL_MASK |
 * #BDK_SHIFT_MASK | #BDK_MOD1_MASK | #BDK_SUPER_MASK | 
 * #BDK_HYPER_MASK | #BDK_META_MASK, that is, Control, Shift, Alt, 
 * Super, Hyper and Meta. Other modifiers will by default be ignored 
 * by #BtkAccelGroup.
 * You must include at least the three modifiers Control, Shift
 * and Alt in any value you pass to this function.
 *
 * The default mod mask should be changed on application startup,
 * before using any accelerator groups.
 */
void
btk_accelerator_set_default_mod_mask (BdkModifierType default_mod_mask)
{
  default_accel_mod_mask = (default_mod_mask & BDK_MODIFIER_MASK) |
    (BDK_CONTROL_MASK | BDK_SHIFT_MASK | BDK_MOD1_MASK);
}

/**
 * btk_accelerator_get_default_mod_mask:
 * @returns: the default accelerator modifier mask
 *
 * Gets the value set by btk_accelerator_set_default_mod_mask().
 */
guint
btk_accelerator_get_default_mod_mask (void)
{
  return default_accel_mod_mask;
}

#define __BTK_ACCEL_GROUP_C__
#include "btkaliasdef.c"
