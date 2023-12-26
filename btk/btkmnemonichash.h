/* btkmnemonichash.h: Sets of mnemonics with cycling
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

#ifndef __BTK_MNEMONIC_HASH_H__
#define __BTK_MNEMONIC_HASH_H__

#include <bdk/bdk.h>
#include <btk/btkwidget.h>

G_BEGIN_DECLS

typedef struct _BtkMnemnonicHash BtkMnemonicHash;

typedef void (*BtkMnemonicHashForeach) (guint      keyval,
					GSList    *targets,
					gpointer   data);

BtkMnemonicHash *_btk_mnemonic_hash_new      (void);
void             _btk_mnemonic_hash_free     (BtkMnemonicHash        *mnemonic_hash);
void             _btk_mnemonic_hash_add      (BtkMnemonicHash        *mnemonic_hash,
					      guint                   keyval,
					      BtkWidget              *target);
void             _btk_mnemonic_hash_remove   (BtkMnemonicHash        *mnemonic_hash,
					      guint                   keyval,
					      BtkWidget              *target);
gboolean         _btk_mnemonic_hash_activate (BtkMnemonicHash        *mnemonic_hash,
					      guint                   keyval);
GSList *         _btk_mnemonic_hash_lookup   (BtkMnemonicHash        *mnemonic_hash,
					      guint                   keyval);
void             _btk_mnemonic_hash_foreach  (BtkMnemonicHash        *mnemonic_hash,
					      BtkMnemonicHashForeach  func,
					      gpointer                func_data);

G_END_DECLS

#endif /* __BTK_MNEMONIC_HASH_H__ */
