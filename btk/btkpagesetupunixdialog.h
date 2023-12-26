/* BtkPageSetupUnixDialog
 * Copyright (C) 2006 Alexander Larsson <alexl@redhat.com>
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

#ifndef __BTK_PAGE_SETUP_UNIX_DIALOG_H__
#define __BTK_PAGE_SETUP_UNIX_DIALOG_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_UNIX_PRINT_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btkunixprint.h> can be included directly."
#endif

#include <btk/btk.h>

B_BEGIN_DECLS

#define BTK_TYPE_PAGE_SETUP_UNIX_DIALOG                  (btk_page_setup_unix_dialog_get_type ())
#define BTK_PAGE_SETUP_UNIX_DIALOG(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PAGE_SETUP_UNIX_DIALOG, BtkPageSetupUnixDialog))
#define BTK_PAGE_SETUP_UNIX_DIALOG_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_PAGE_SETUP_UNIX_DIALOG, BtkPageSetupUnixDialogClass))
#define BTK_IS_PAGE_SETUP_UNIX_DIALOG(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PAGE_SETUP_UNIX_DIALOG))
#define BTK_IS_PAGE_SETUP_UNIX_DIALOG_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_PAGE_SETUP_UNIX_DIALOG))
#define BTK_PAGE_SETUP_UNIX_DIALOG_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_PAGE_SETUP_UNIX_DIALOG, BtkPageSetupUnixDialogClass))


typedef struct _BtkPageSetupUnixDialog         BtkPageSetupUnixDialog;
typedef struct _BtkPageSetupUnixDialogClass    BtkPageSetupUnixDialogClass;
typedef struct BtkPageSetupUnixDialogPrivate   BtkPageSetupUnixDialogPrivate;

struct _BtkPageSetupUnixDialog
{
  BtkDialog parent_instance;

  BtkPageSetupUnixDialogPrivate *GSEAL (priv);
};

struct _BtkPageSetupUnixDialogClass
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

GType 		  btk_page_setup_unix_dialog_get_type	        (void) B_GNUC_CONST;
BtkWidget *       btk_page_setup_unix_dialog_new                (const gchar            *title,
								 BtkWindow              *parent);
void              btk_page_setup_unix_dialog_set_page_setup     (BtkPageSetupUnixDialog *dialog,
								 BtkPageSetup           *page_setup);
BtkPageSetup *    btk_page_setup_unix_dialog_get_page_setup     (BtkPageSetupUnixDialog *dialog);
void              btk_page_setup_unix_dialog_set_print_settings (BtkPageSetupUnixDialog *dialog,
								 BtkPrintSettings       *print_settings);
BtkPrintSettings *btk_page_setup_unix_dialog_get_print_settings (BtkPageSetupUnixDialog *dialog);

B_END_DECLS

#endif /* __BTK_PAGE_SETUP_UNIX_DIALOG_H__ */
