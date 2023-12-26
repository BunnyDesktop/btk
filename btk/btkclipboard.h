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

G_BEGIN_DECLS

#define BTK_TYPE_CLIPBOARD            (btk_clipboard_get_type ())
#define BTK_CLIPBOARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_CLIPBOARD, BtkClipboard))
#define BTK_IS_CLIPBOARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_CLIPBOARD))

typedef void (* BtkClipboardReceivedFunc)         (BtkClipboard     *clipboard,
					           BtkSelectionData *selection_data,
					           gpointer          data);
typedef void (* BtkClipboardTextReceivedFunc)     (BtkClipboard     *clipboard,
					           const gchar      *text,
					           gpointer          data);
typedef void (* BtkClipboardRichTextReceivedFunc) (BtkClipboard     *clipboard,
                                                   BdkAtom           format,
					           const guint8     *text,
                                                   gsize             length,
					           gpointer          data);
typedef void (* BtkClipboardImageReceivedFunc)    (BtkClipboard     *clipboard,
						   BdkPixbuf        *pixbuf,
						   gpointer          data);
typedef void (* BtkClipboardURIReceivedFunc)      (BtkClipboard     *clipboard,
						   gchar           **uris,
						   gpointer          data);
typedef void (* BtkClipboardTargetsReceivedFunc)  (BtkClipboard     *clipboard,
					           BdkAtom          *atoms,
						   gint              n_atoms,
					           gpointer          data);

/* Should these functions have BtkClipboard *clipboard as the first argument?
 * right now for ClearFunc, you may have trouble determining _which_ clipboard
 * was cleared, if you reuse your ClearFunc for multiple clipboards.
 */
typedef void (* BtkClipboardGetFunc)          (BtkClipboard     *clipboard,
					       BtkSelectionData *selection_data,
					       guint             info,
					       gpointer          user_data_or_owner);
typedef void (* BtkClipboardClearFunc)        (BtkClipboard     *clipboard,
					       gpointer          user_data_or_owner);

GType         btk_clipboard_get_type (void) G_GNUC_CONST;

BtkClipboard *btk_clipboard_get_for_display (BdkDisplay   *display,
					     BdkAtom       selection);
#ifndef BDK_MULTIHEAD_SAFE
BtkClipboard *btk_clipboard_get             (BdkAtom       selection);
#endif

BdkDisplay   *btk_clipboard_get_display     (BtkClipboard *clipboard);


gboolean btk_clipboard_set_with_data  (BtkClipboard          *clipboard,
				       const BtkTargetEntry  *targets,
				       guint                  n_targets,
				       BtkClipboardGetFunc    get_func,
				       BtkClipboardClearFunc  clear_func,
				       gpointer               user_data);
gboolean btk_clipboard_set_with_owner (BtkClipboard          *clipboard,
				       const BtkTargetEntry  *targets,
				       guint                  n_targets,
				       BtkClipboardGetFunc    get_func,
				       BtkClipboardClearFunc  clear_func,
				       GObject               *owner);
GObject *btk_clipboard_get_owner      (BtkClipboard          *clipboard);
void     btk_clipboard_clear          (BtkClipboard          *clipboard);
void     btk_clipboard_set_text       (BtkClipboard          *clipboard,
				       const gchar           *text,
				       gint                   len);
void     btk_clipboard_set_image      (BtkClipboard          *clipboard,
				       BdkPixbuf             *pixbuf);

void btk_clipboard_request_contents  (BtkClipboard                     *clipboard,
                                      BdkAtom                           target,
                                      BtkClipboardReceivedFunc          callback,
                                      gpointer                          user_data);
void btk_clipboard_request_text      (BtkClipboard                     *clipboard,
                                      BtkClipboardTextReceivedFunc      callback,
                                      gpointer                          user_data);
void btk_clipboard_request_rich_text (BtkClipboard                     *clipboard,
                                      BtkTextBuffer                    *buffer,
                                      BtkClipboardRichTextReceivedFunc  callback,
                                      gpointer                          user_data);
void btk_clipboard_request_image     (BtkClipboard                     *clipboard,
                                      BtkClipboardImageReceivedFunc     callback,
                                      gpointer                          user_data);
void btk_clipboard_request_uris      (BtkClipboard                     *clipboard,
                                      BtkClipboardURIReceivedFunc       callback,
                                      gpointer                          user_data);
void btk_clipboard_request_targets   (BtkClipboard                     *clipboard,
                                      BtkClipboardTargetsReceivedFunc   callback,
                                      gpointer                          user_data);

BtkSelectionData *btk_clipboard_wait_for_contents  (BtkClipboard  *clipboard,
                                                    BdkAtom        target);
gchar *           btk_clipboard_wait_for_text      (BtkClipboard  *clipboard);
guint8 *          btk_clipboard_wait_for_rich_text (BtkClipboard  *clipboard,
                                                    BtkTextBuffer *buffer,
                                                    BdkAtom       *format,
                                                    gsize         *length);
BdkPixbuf *       btk_clipboard_wait_for_image     (BtkClipboard  *clipboard);
gchar **          btk_clipboard_wait_for_uris      (BtkClipboard  *clipboard);
gboolean          btk_clipboard_wait_for_targets   (BtkClipboard  *clipboard,
                                                    BdkAtom      **targets,
                                                    gint          *n_targets);

gboolean btk_clipboard_wait_is_text_available      (BtkClipboard  *clipboard);
gboolean btk_clipboard_wait_is_rich_text_available (BtkClipboard  *clipboard,
                                                    BtkTextBuffer *buffer);
gboolean btk_clipboard_wait_is_image_available     (BtkClipboard  *clipboard);
gboolean btk_clipboard_wait_is_uris_available      (BtkClipboard  *clipboard);
gboolean btk_clipboard_wait_is_target_available    (BtkClipboard  *clipboard,
                                                    BdkAtom        target);


void btk_clipboard_set_can_store (BtkClipboard         *clipboard,
				  const BtkTargetEntry *targets,
				  gint                  n_targets);

void btk_clipboard_store         (BtkClipboard   *clipboard);

/* private */
void     _btk_clipboard_handle_event    (BdkEventOwnerChange *event);

void     _btk_clipboard_store_all       (void);

G_END_DECLS

#endif /* __BTK_CLIPBOARD_H__ */
