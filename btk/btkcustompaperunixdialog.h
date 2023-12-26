/* BtkCustomPaperUnixDialog
 * Copyright (C) 2006 Alexander Larsson <alexl@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __BTK_CUSTOM_PAPER_UNIX_DIALOG_H__
#define __BTK_CUSTOM_PAPER_UNIX_DIALOG_H__

#include <btk/btk.h>

B_BEGIN_DECLS

#define BTK_TYPE_CUSTOM_PAPER_UNIX_DIALOG                  (btk_custom_paper_unix_dialog_get_type ())
#define BTK_CUSTOM_PAPER_UNIX_DIALOG(obj)                  (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_CUSTOM_PAPER_UNIX_DIALOG, BtkCustomPaperUnixDialog))
#define BTK_CUSTOM_PAPER_UNIX_DIALOG_CLASS(klass)          (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_CUSTOM_PAPER_UNIX_DIALOG, BtkCustomPaperUnixDialogClass))
#define BTK_IS_CUSTOM_PAPER_UNIX_DIALOG(obj)               (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_CUSTOM_PAPER_UNIX_DIALOG))
#define BTK_IS_CUSTOM_PAPER_UNIX_DIALOG_CLASS(klass)       (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_CUSTOM_PAPER_UNIX_DIALOG))
#define BTK_CUSTOM_PAPER_UNIX_DIALOG_GET_CLASS(obj)        (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_CUSTOM_PAPER_UNIX_DIALOG, BtkCustomPaperUnixDialogClass))


typedef struct _BtkCustomPaperUnixDialog         BtkCustomPaperUnixDialog;
typedef struct _BtkCustomPaperUnixDialogClass    BtkCustomPaperUnixDialogClass;
typedef struct BtkCustomPaperUnixDialogPrivate   BtkCustomPaperUnixDialogPrivate;

struct _BtkCustomPaperUnixDialog
{
  BtkDialog parent_instance;

  BtkCustomPaperUnixDialogPrivate *GSEAL (priv);
};

struct _BtkCustomPaperUnixDialogClass
{
  BtkDialogClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
  void (*_btk_reserved5) (void);
  void (*_btk_reserved6) (void);
  void (*_btk_reserved7) (void);
};

GType             btk_custom_paper_unix_dialog_get_type           (void) B_GNUC_CONST;
BtkWidget *       _btk_custom_paper_unix_dialog_new                (BtkWindow   *parent,
								   const bchar *title);
BtkUnit           _btk_print_get_default_user_units                (void);
void              _btk_print_load_custom_papers                    (BtkListStore *store);
void              _btk_print_save_custom_papers                    (BtkListStore *store);


B_END_DECLS

#endif /* __BTK_CUSTOM_PAPER_UNIX_DIALOG_H__ */
