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

B_BEGIN_DECLS

typedef struct _BtkMnemnonicHash BtkMnemonicHash;

typedef void (*BtkMnemonicHashForeach) (buint      keyval,
					GSList    *targets,
					bpointer   data);

BtkMnemonicHash *_btk_mnemonic_hash_new      (void);
void             _btk_mnemonic_hash_free     (BtkMnemonicHash        *mnemonic_hash);
void             _btk_mnemonic_hash_add      (BtkMnemonicHash        *mnemonic_hash,
					      buint                   keyval,
					      BtkWidget              *target);
void             _btk_mnemonic_hash_remove   (BtkMnemonicHash        *mnemonic_hash,
					      buint                   keyval,
					      BtkWidget              *target);
bboolean         _btk_mnemonic_hash_activate (BtkMnemonicHash        *mnemonic_hash,
					      buint                   keyval);
GSList *         _btk_mnemonic_hash_lookup   (BtkMnemonicHash        *mnemonic_hash,
					      buint                   keyval);
void             _btk_mnemonic_hash_foreach  (BtkMnemonicHash        *mnemonic_hash,
					      BtkMnemonicHashForeach  func,
					      bpointer                func_data);

B_END_DECLS

#endif /* __BTK_MNEMONIC_HASH_H__ */
