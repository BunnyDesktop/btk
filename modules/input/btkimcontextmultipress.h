/* Copyright (C) 2006 Openismus GmbH
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

#ifndef __BTK_IM_CONTEXT_MULTIPRESS_H__
#define __BTK_IM_CONTEXT_MULTIPRESS_H__

#include <btk/btk.h>

B_BEGIN_DECLS

#define BTK_TYPE_IM_CONTEXT_MULTIPRESS            (btk_im_context_multipress_get_type ())
#define BTK_IM_CONTEXT_MULTIPRESS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_IM_CONTEXT_MULTIPRESS, BtkImContextMultipress))
#define BTK_IM_CONTEXT_MULTIPRESS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_IM_CONTEXT_MULTIPRESS, BtkImContextMultipressClass))
#define BTK_IS_IM_CONTEXT_MULTIPRESS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_IM_CONTEXT_MULTIPRESS))
#define BTK_IS_IM_CONTEXT_MULTIPRESS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_IM_CONTEXT_MULTIPRESS))
#define BTK_IM_CONTEXT_MULTIPRESS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_IM_CONTEXT_MULTIPRESS, BtkImContextMultipressClass))

typedef struct _BtkImContextMultipress BtkImContextMultipress;

/* This input method allows multi-press character input, like that found on
 * mobile phones.
 *
 * This is based on BtkImContextSimple, which allows "compose" based on
 * sequences of characters.  But instead the character sequences are defined
 * by lists of characters for a key, so that repeated pressing of the same key
 * can cycle through the possible output characters, with automatic choosing
 * of the character after a time delay.
 */
struct _BtkImContextMultipress
{
  /*< private >*/
  BtkIMContext parent;

  /* Sequence information, loaded from the configuration file: */
  GHashTable* key_sequences;
  gsize dummy; /* ABI-preserving placeholder */

  /* The last character entered so far during a compose.
   * If this is NULL then we are not composing yet.
   */
  guint key_last_entered;
  
  /* The position of the compose in the possible sequence.
   *  For instance, this is 2 if aa has been pressed to show b (from abc0).
   */
  guint compose_count; 
  guint timeout_id;

  /* The character(s) that will be used if it the current character(s) is accepted: */
  const gchar *tentative_match;
};


typedef struct _BtkImContextMultipressClass  BtkImContextMultipressClass;

struct _BtkImContextMultipressClass
{
  BtkIMContextClass parent_class;
};

void btk_im_context_multipress_register_type (GTypeModule* type_module);
GType btk_im_context_multipress_get_type (void);
BtkIMContext *btk_im_context_multipress_new (void);

B_END_DECLS

#endif /* __BTK_IM_CONTEXT_MULTIPRESS_H__ */
