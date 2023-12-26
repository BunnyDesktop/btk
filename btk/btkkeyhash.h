/* btkkeyhash.h: Keymap aware matching of key bindings
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

#ifndef __BTK_KEY_HASH_H__
#define __BTK_KEY_HASH_H__

#include <bdk/bdk.h>

B_BEGIN_DECLS

typedef struct _BtkKeyHash BtkKeyHash;

BtkKeyHash *_btk_key_hash_new           (BdkKeymap       *keymap,
					 GDestroyNotify   item_destroy_notify);
void        _btk_key_hash_add_entry     (BtkKeyHash      *key_hash,
					 guint            keyval,
					 BdkModifierType  modifiers,
					 gpointer         value);
void        _btk_key_hash_remove_entry  (BtkKeyHash      *key_hash,
					 gpointer         value);
GSList *    _btk_key_hash_lookup        (BtkKeyHash      *key_hash,
					 guint16          hardware_keycode,
					 BdkModifierType  state,
					 BdkModifierType  mask,
					 gint             group);
GSList *    _btk_key_hash_lookup_keyval (BtkKeyHash      *key_hash,
					 guint            keyval,
					 BdkModifierType  modifiers);
void        _btk_key_hash_free          (BtkKeyHash      *key_hash);

B_END_DECLS

#endif /* __BTK_KEY_HASH_H__ */
