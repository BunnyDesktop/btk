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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the BTK+ Team and others 1997-2003.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BTK_MESSAGE_DIALOG_H__
#define __BTK_MESSAGE_DIALOG_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkdialog.h>
#include <btk/btkenums.h>

G_BEGIN_DECLS


#define BTK_TYPE_MESSAGE_DIALOG                  (btk_message_dialog_get_type ())
#define BTK_MESSAGE_DIALOG(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_MESSAGE_DIALOG, BtkMessageDialog))
#define BTK_MESSAGE_DIALOG_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_MESSAGE_DIALOG, BtkMessageDialogClass))
#define BTK_IS_MESSAGE_DIALOG(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_MESSAGE_DIALOG))
#define BTK_IS_MESSAGE_DIALOG_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_MESSAGE_DIALOG))
#define BTK_MESSAGE_DIALOG_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_MESSAGE_DIALOG, BtkMessageDialogClass))

typedef struct _BtkMessageDialog        BtkMessageDialog;
typedef struct _BtkMessageDialogClass   BtkMessageDialogClass;

struct _BtkMessageDialog
{
  /*< private >*/
  
  BtkDialog parent_instance;
  
  BtkWidget *GSEAL (image);
  BtkWidget *GSEAL (label);
};

struct _BtkMessageDialogClass
{
  BtkDialogClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

/**
 * BtkButtonsType:
 * @BTK_BUTTONS_NONE: no buttons at all
 * @BTK_BUTTONS_OK: an OK button
 * @BTK_BUTTONS_CLOSE: a Close button
 * @BTK_BUTTONS_CANCEL: a Cancel button
 * @BTK_BUTTONS_YES_NO: Yes and No buttons
 * @BTK_BUTTONS_OK_CANCEL: OK and Cancel buttons
 *
 * Prebuilt sets of buttons for the dialog. If
 * none of these choices are appropriate, simply use %BTK_BUTTONS_NONE
 * then call btk_dialog_add_buttons().
 * <note>
 *  Please note that %BTK_BUTTONS_OK, %BTK_BUTTONS_YES_NO
 *  and %BTK_BUTTONS_OK_CANCEL are discouraged by the
 *  <ulink url="http://library.bunny.org/devel/hig-book/stable/">BUNNY HIG</ulink>.
 * </note>
 */
typedef enum
{
  BTK_BUTTONS_NONE,
  BTK_BUTTONS_OK,
  BTK_BUTTONS_CLOSE,
  BTK_BUTTONS_CANCEL,
  BTK_BUTTONS_YES_NO,
  BTK_BUTTONS_OK_CANCEL
} BtkButtonsType;

GType      btk_message_dialog_get_type (void) G_GNUC_CONST;

BtkWidget* btk_message_dialog_new      (BtkWindow      *parent,
                                        BtkDialogFlags  flags,
                                        BtkMessageType  type,
                                        BtkButtonsType  buttons,
                                        const gchar    *message_format,
                                        ...) G_GNUC_PRINTF (5, 6);

BtkWidget* btk_message_dialog_new_with_markup   (BtkWindow      *parent,
                                                 BtkDialogFlags  flags,
                                                 BtkMessageType  type,
                                                 BtkButtonsType  buttons,
                                                 const gchar    *message_format,
                                                 ...) G_GNUC_PRINTF (5, 6);

void       btk_message_dialog_set_image    (BtkMessageDialog *dialog,
					    BtkWidget        *image);

BtkWidget * btk_message_dialog_get_image   (BtkMessageDialog *dialog);

void       btk_message_dialog_set_markup  (BtkMessageDialog *message_dialog,
                                           const gchar      *str);

void       btk_message_dialog_format_secondary_text (BtkMessageDialog *message_dialog,
                                                     const gchar      *message_format,
                                                     ...) G_GNUC_PRINTF (2, 3);

void       btk_message_dialog_format_secondary_markup (BtkMessageDialog *message_dialog,
                                                       const gchar      *message_format,
                                                       ...) G_GNUC_PRINTF (2, 3);

BtkWidget *btk_message_dialog_get_message_area (BtkMessageDialog *message_dialog);

G_END_DECLS

#endif /* __BTK_MESSAGE_DIALOG_H__ */
