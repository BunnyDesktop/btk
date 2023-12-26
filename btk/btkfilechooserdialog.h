/* BTK - The GIMP Toolkit
 * btkfilechooserdialog.h: File selector dialog
 * Copyright (C) 2003, Red Hat, Inc.
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

#ifndef __BTK_FILE_CHOOSER_DIALOG_H__
#define __BTK_FILE_CHOOSER_DIALOG_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkdialog.h>
#include <btk/btkfilechooser.h>

B_BEGIN_DECLS

#define BTK_TYPE_FILE_CHOOSER_DIALOG             (btk_file_chooser_dialog_get_type ())
#define BTK_FILE_CHOOSER_DIALOG(obj)             (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_FILE_CHOOSER_DIALOG, BtkFileChooserDialog))
#define BTK_FILE_CHOOSER_DIALOG_CLASS(klass)     (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_FILE_CHOOSER_DIALOG, BtkFileChooserDialogClass))
#define BTK_IS_FILE_CHOOSER_DIALOG(obj)          (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_FILE_CHOOSER_DIALOG))
#define BTK_IS_FILE_CHOOSER_DIALOG_CLASS(klass)  (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_FILE_CHOOSER_DIALOG))
#define BTK_FILE_CHOOSER_DIALOG_GET_CLASS(obj)   (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_FILE_CHOOSER_DIALOG, BtkFileChooserDialogClass))

typedef struct _BtkFileChooserDialog        BtkFileChooserDialog;
typedef struct _BtkFileChooserDialogPrivate BtkFileChooserDialogPrivate;
typedef struct _BtkFileChooserDialogClass   BtkFileChooserDialogClass;

struct _BtkFileChooserDialog
{
  BtkDialog parent_instance;

  BtkFileChooserDialogPrivate *GSEAL (priv);
};

struct _BtkFileChooserDialogClass
{
  BtkDialogClass parent_class;
};

GType      btk_file_chooser_dialog_get_type         (void) B_GNUC_CONST;
BtkWidget *btk_file_chooser_dialog_new              (const bchar          *title,
						     BtkWindow            *parent,
						     BtkFileChooserAction  action,
						     const bchar          *first_button_text,
						     ...) B_GNUC_NULL_TERMINATED;

#ifndef BTK_DISABLE_DEPRECATED
BtkWidget *btk_file_chooser_dialog_new_with_backend (const bchar          *title,
						     BtkWindow            *parent,
						     BtkFileChooserAction  action,
						     const bchar          *backend,
						     const bchar          *first_button_text,
						     ...) B_GNUC_NULL_TERMINATED;
#endif /* BTK_DISABLE_DEPRECATED */

B_END_DECLS

#endif /* __BTK_FILE_CHOOSER_DIALOG_H__ */
