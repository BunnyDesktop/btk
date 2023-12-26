/* BtkPrintUnixDialog
 * Copyright (C) 2006 John (J5) Palmieri <johnp@redhat.com>
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

#ifndef __BTK_PRINT_UNIX_DIALOG_H__
#define __BTK_PRINT_UNIX_DIALOG_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_UNIX_PRINT_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btkunixprint.h> can be included directly."
#endif

#include <btk/btk.h>
#include <btk/btkprinter.h>
#include <btk/btkprintjob.h>

G_BEGIN_DECLS

#define BTK_TYPE_PRINT_UNIX_DIALOG                  (btk_print_unix_dialog_get_type ())
#define BTK_PRINT_UNIX_DIALOG(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PRINT_UNIX_DIALOG, BtkPrintUnixDialog))
#define BTK_PRINT_UNIX_DIALOG_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_PRINT_UNIX_DIALOG, BtkPrintUnixDialogClass))
#define BTK_IS_PRINT_UNIX_DIALOG(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PRINT_UNIX_DIALOG))
#define BTK_IS_PRINT_UNIX_DIALOG_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_PRINT_UNIX_DIALOG))
#define BTK_PRINT_UNIX_DIALOG_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_PRINT_UNIX_DIALOG, BtkPrintUnixDialogClass))


typedef struct _BtkPrintUnixDialog         BtkPrintUnixDialog;
typedef struct _BtkPrintUnixDialogClass    BtkPrintUnixDialogClass;
typedef struct BtkPrintUnixDialogPrivate   BtkPrintUnixDialogPrivate;

struct _BtkPrintUnixDialog
{
  BtkDialog parent_instance;

  BtkPrintUnixDialogPrivate *GSEAL (priv);
};

struct _BtkPrintUnixDialogClass
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

GType                btk_print_unix_dialog_get_type                (void) G_GNUC_CONST;
BtkWidget *          btk_print_unix_dialog_new                     (const gchar *title,
                                                                    BtkWindow   *parent);

void                 btk_print_unix_dialog_set_page_setup          (BtkPrintUnixDialog *dialog,
								    BtkPageSetup       *page_setup);
BtkPageSetup *       btk_print_unix_dialog_get_page_setup          (BtkPrintUnixDialog *dialog);
void                 btk_print_unix_dialog_set_current_page        (BtkPrintUnixDialog *dialog,
								    gint                current_page);
gint                 btk_print_unix_dialog_get_current_page        (BtkPrintUnixDialog *dialog);
void                 btk_print_unix_dialog_set_settings            (BtkPrintUnixDialog *dialog,
								    BtkPrintSettings   *settings);
BtkPrintSettings *   btk_print_unix_dialog_get_settings            (BtkPrintUnixDialog *dialog);
BtkPrinter *         btk_print_unix_dialog_get_selected_printer    (BtkPrintUnixDialog *dialog);
void                 btk_print_unix_dialog_add_custom_tab          (BtkPrintUnixDialog *dialog,
								    BtkWidget          *child,
								    BtkWidget          *tab_label);
void                 btk_print_unix_dialog_set_manual_capabilities (BtkPrintUnixDialog *dialog,
								    BtkPrintCapabilities capabilities);
BtkPrintCapabilities btk_print_unix_dialog_get_manual_capabilities (BtkPrintUnixDialog  *dialog);
void                 btk_print_unix_dialog_set_support_selection   (BtkPrintUnixDialog  *dialog,
								    gboolean             support_selection);
gboolean             btk_print_unix_dialog_get_support_selection   (BtkPrintUnixDialog  *dialog);
void                 btk_print_unix_dialog_set_has_selection       (BtkPrintUnixDialog  *dialog,
								    gboolean             has_selection);
gboolean             btk_print_unix_dialog_get_has_selection       (BtkPrintUnixDialog  *dialog);
void                 btk_print_unix_dialog_set_embed_page_setup    (BtkPrintUnixDialog *dialog,
								    gboolean            embed);
gboolean             btk_print_unix_dialog_get_embed_page_setup    (BtkPrintUnixDialog *dialog);
gboolean             btk_print_unix_dialog_get_page_setup_set      (BtkPrintUnixDialog *dialog);

G_END_DECLS

#endif /* __BTK_PRINT_UNIX_DIALOG_H__ */
