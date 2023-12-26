/* BDK - The GIMP Drawing Kit
 * Copyright (C) 2000 Red Hat, Inc.
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

#ifndef __BDK_KEYS_H__
#define __BDK_KEYS_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bdk/bdktypes.h>

G_BEGIN_DECLS

typedef struct _BdkKeymapKey BdkKeymapKey;

/* BdkKeymapKey is a hardware key that can be mapped to a keyval */
struct _BdkKeymapKey
{
  guint keycode;
  gint  group;
  gint  level;
};

/* A BdkKeymap defines the translation from keyboard state
 * (including a hardware key, a modifier mask, and active keyboard group)
 * to a keyval. This translation has two phases. The first phase is
 * to determine the effective keyboard group and level for the keyboard
 * state; the second phase is to look up the keycode/group/level triplet
 * in the keymap and see what keyval it corresponds to.
 */

typedef struct _BdkKeymap      BdkKeymap;
typedef struct _BdkKeymapClass BdkKeymapClass;

#define BDK_TYPE_KEYMAP              (bdk_keymap_get_type ())
#define BDK_KEYMAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_KEYMAP, BdkKeymap))
#define BDK_KEYMAP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_KEYMAP, BdkKeymapClass))
#define BDK_IS_KEYMAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_KEYMAP))
#define BDK_IS_KEYMAP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_KEYMAP))
#define BDK_KEYMAP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_KEYMAP, BdkKeymapClass))

struct _BdkKeymap
{
  GObject     parent_instance;
  BdkDisplay *GSEAL (display);
};

struct _BdkKeymapClass
{
  GObjectClass parent_class;

  void (*direction_changed) (BdkKeymap *keymap);
  void (*keys_changed)      (BdkKeymap *keymap);
  void (*state_changed)     (BdkKeymap *keymap);
};

GType bdk_keymap_get_type (void) G_GNUC_CONST;

#ifndef BDK_MULTIHEAD_SAFE
BdkKeymap* bdk_keymap_get_default     (void);
#endif
BdkKeymap* bdk_keymap_get_for_display (BdkDisplay *display);


guint          bdk_keymap_lookup_key               (BdkKeymap           *keymap,
						    const BdkKeymapKey  *key);
gboolean       bdk_keymap_translate_keyboard_state (BdkKeymap           *keymap,
						    guint                hardware_keycode,
						    BdkModifierType      state,
						    gint                 group,
						    guint               *keyval,
						    gint                *effective_group,
						    gint                *level,
						    BdkModifierType     *consumed_modifiers);
gboolean       bdk_keymap_get_entries_for_keyval   (BdkKeymap           *keymap,
						    guint                keyval,
						    BdkKeymapKey       **keys,
						    gint                *n_keys);
gboolean       bdk_keymap_get_entries_for_keycode  (BdkKeymap           *keymap,
						    guint                hardware_keycode,
						    BdkKeymapKey       **keys,
						    guint              **keyvals,
						    gint                *n_entries);
BangoDirection bdk_keymap_get_direction            (BdkKeymap           *keymap);
gboolean       bdk_keymap_have_bidi_layouts        (BdkKeymap           *keymap);
gboolean       bdk_keymap_get_caps_lock_state      (BdkKeymap           *keymap);
void           bdk_keymap_add_virtual_modifiers    (BdkKeymap           *keymap,
                                                    BdkModifierType     *state);
gboolean       bdk_keymap_map_virtual_modifiers    (BdkKeymap           *keymap,
                                                    BdkModifierType     *state);

/* Key values
 */
gchar*   bdk_keyval_name         (guint        keyval) G_GNUC_CONST;
guint    bdk_keyval_from_name    (const gchar *keyval_name);
void     bdk_keyval_convert_case (guint        symbol,
				  guint       *lower,
				  guint       *upper);
guint    bdk_keyval_to_upper     (guint        keyval) G_GNUC_CONST;
guint    bdk_keyval_to_lower     (guint        keyval) G_GNUC_CONST;
gboolean bdk_keyval_is_upper     (guint        keyval) G_GNUC_CONST;
gboolean bdk_keyval_is_lower     (guint        keyval) G_GNUC_CONST;

guint32  bdk_keyval_to_unicode   (guint        keyval) G_GNUC_CONST;
guint    bdk_unicode_to_keyval   (guint32      wc) G_GNUC_CONST;


G_END_DECLS

#endif /* __BDK_KEYS_H__ */
