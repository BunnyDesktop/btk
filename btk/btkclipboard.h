/* BTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
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
 *
 * Global clipboard abstraction.
 */

#ifndef __BTK_CLIPBOARD_H__
#define __BTK_CLIPBOARD_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkselection.h>

B_BEGIN_DECLS

#define BTK_TYPE_CLIPBOARD            (btk_clipboard_get_type ())
#define BTK_CLIPBOARD(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_CLIPBOARD, BtkClipboard))
#define BTK_IS_CLIPBOARD(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_CLIPBOARD))

typedef void (* BtkClipboardReceivedFunc)         (BtkClipboard     *clipboard,
					           BtkSelectionData *selection_data,
					           bpointer          data);
typedef void (* BtkClipboardTextReceivedFunc)     (BtkClipboard     *clipboard,
					           const bchar      *text,
					           bpointer          data);
typedef void (* BtkClipboardRichTextReceivedFunc) (BtkClipboard     *clipboard,
                                                   BdkAtom           format,
					           const buint8     *text,
                                                   bsize             length,
					           bpointer          data);
typedef void (* BtkClipboardImageReceivedFunc)    (BtkClipboard     *clipboard,
						   BdkPixbuf        *pixbuf,
						   bpointer          data);
typedef void (* BtkClipboardURIReceivedFunc)      (BtkClipboard     *clipboard,
						   bchar           **uris,
						   bpointer          data);
typedef void (* BtkClipboardTargetsReceivedFunc)  (BtkClipboard     *clipboard,
					           BdkAtom          *atoms,
						   bint              n_atoms,
					           bpointer          data);

/* Should these functions have BtkClipboard *clipboard as the first argument?
 * right now for ClearFunc, you may have trouble determining _which_ clipboard
 * was cleared, if you reuse your ClearFunc for multiple clipboards.
 */
typedef void (* BtkClipboardGetFunc)          (BtkClipboard     *clipboard,
					       BtkSelectionData *selection_data,
					       buint             info,
					       bpointer          user_data_or_owner);
typedef void (* BtkClipboardClearFunc)        (BtkClipboard     *clipboard,
					       bpointer          user_data_or_owner);

GType         btk_clipboard_get_type (void) B_GNUC_CONST;

BtkClipboard *btk_clipboard_get_for_display (BdkDisplay   *display,
					     BdkAtom       selection);
#ifndef BDK_MULTIHEAD_SAFE
BtkClipboard *btk_clipboard_get             (BdkAtom       selection);
#endif

BdkDisplay   *btk_clipboard_get_display     (BtkClipboard *clipboard);


bboolean btk_clipboard_set_with_data  (BtkClipboard          *clipboard,
				       const BtkTargetEntry  *targets,
				       buint                  n_targets,
				       BtkClipboardGetFunc    get_func,
				       BtkClipboardClearFunc  clear_func,
				       bpointer               user_data);
bboolean btk_clipboard_set_with_owner (BtkClipboard          *clipboard,
				       const BtkTargetEntry  *targets,
				       buint                  n_targets,
				       BtkClipboardGetFunc    get_func,
				       BtkClipboardClearFunc  clear_func,
				       BObject               *owner);
BObject *btk_clipboard_get_owner      (BtkClipboard          *clipboard);
void     btk_clipboard_clear          (BtkClipboard          *clipboard);
void     btk_clipboard_set_text       (BtkClipboard          *clipboard,
				       const bchar           *text,
				       bint                   len);
void     btk_clipboard_set_image      (BtkClipboard          *clipboard,
				       BdkPixbuf             *pixbuf);

void btk_clipboard_request_contents  (BtkClipboard                     *clipboard,
                                      BdkAtom                           target,
                                      BtkClipboardReceivedFunc          callback,
                                      bpointer                          user_data);
void btk_clipboard_request_text      (BtkClipboard                     *clipboard,
                                      BtkClipboardTextReceivedFunc      callback,
                                      bpointer                          user_data);
void btk_clipboard_request_rich_text (BtkClipboard                     *clipboard,
                                      BtkTextBuffer                    *buffer,
                                      BtkClipboardRichTextReceivedFunc  callback,
                                      bpointer                          user_data);
void btk_clipboard_request_image     (BtkClipboard                     *clipboard,
                                      BtkClipboardImageReceivedFunc     callback,
                                      bpointer                          user_data);
void btk_clipboard_request_uris      (BtkClipboard                     *clipboard,
                                      BtkClipboardURIReceivedFunc       callback,
                                      bpointer                          user_data);
void btk_clipboard_request_targets   (BtkClipboard                     *clipboard,
                                      BtkClipboardTargetsReceivedFunc   callback,
                                      bpointer                          user_data);

BtkSelectionData *btk_clipboard_wait_for_contents  (BtkClipboard  *clipboard,
                                                    BdkAtom        target);
bchar *           btk_clipboard_wait_for_text      (BtkClipboard  *clipboard);
buint8 *          btk_clipboard_wait_for_rich_text (BtkClipboard  *clipboard,
                                                    BtkTextBuffer *buffer,
                                                    BdkAtom       *format,
                                                    bsize         *length);
BdkPixbuf *       btk_clipboard_wait_for_image     (BtkClipboard  *clipboard);
bchar **          btk_clipboard_wait_for_uris      (BtkClipboard  *clipboard);
bboolean          btk_clipboard_wait_for_targets   (BtkClipboard  *clipboard,
                                                    BdkAtom      **targets,
                                                    bint          *n_targets);

bboolean btk_clipboard_wait_is_text_available      (BtkClipboard  *clipboard);
bboolean btk_clipboard_wait_is_rich_text_available (BtkClipboard  *clipboard,
                                                    BtkTextBuffer *buffer);
bboolean btk_clipboard_wait_is_image_available     (BtkClipboard  *clipboard);
bboolean btk_clipboard_wait_is_uris_available      (BtkClipboard  *clipboard);
bboolean btk_clipboard_wait_is_target_available    (BtkClipboard  *clipboard,
                                                    BdkAtom        target);


void btk_clipboard_set_can_store (BtkClipboard         *clipboard,
				  const BtkTargetEntry *targets,
				  bint                  n_targets);

void btk_clipboard_store         (BtkClipboard   *clipboard);

/* private */
void     _btk_clipboard_handle_event    (BdkEventOwnerChange *event);

void     _btk_clipboard_store_all       (void);

B_END_DECLS

#endif /* __BTK_CLIPBOARD_H__ */
