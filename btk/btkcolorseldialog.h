/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BTK_COLOR_SELECTION_DIALOG_H__
#define __BTK_COLOR_SELECTION_DIALOG_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkdialog.h>
#include <btk/btkcolorsel.h>
#include <btk/btkvbox.h>

B_BEGIN_DECLS

#define BTK_TYPE_COLOR_SELECTION_DIALOG            (btk_color_selection_dialog_get_type ())
#define BTK_COLOR_SELECTION_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_COLOR_SELECTION_DIALOG, BtkColorSelectionDialog))
#define BTK_COLOR_SELECTION_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_COLOR_SELECTION_DIALOG, BtkColorSelectionDialogClass))
#define BTK_IS_COLOR_SELECTION_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_COLOR_SELECTION_DIALOG))
#define BTK_IS_COLOR_SELECTION_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_COLOR_SELECTION_DIALOG))
#define BTK_COLOR_SELECTION_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_COLOR_SELECTION_DIALOG, BtkColorSelectionDialogClass))


typedef struct _BtkColorSelectionDialog       BtkColorSelectionDialog;
typedef struct _BtkColorSelectionDialogClass  BtkColorSelectionDialogClass;


struct _BtkColorSelectionDialog
{
  BtkDialog parent_instance;

  BtkWidget *GSEAL (colorsel);
  BtkWidget *GSEAL (ok_button);
  BtkWidget *GSEAL (cancel_button);
  BtkWidget *GSEAL (help_button);
};

struct _BtkColorSelectionDialogClass
{
  BtkDialogClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


/* ColorSelectionDialog */
GType      btk_color_selection_dialog_get_type            (void) B_GNUC_CONST;
BtkWidget* btk_color_selection_dialog_new                 (const gchar *title);
BtkWidget* btk_color_selection_dialog_get_color_selection (BtkColorSelectionDialog *colorsel);


B_END_DECLS

#endif /* __BTK_COLOR_SELECTION_DIALOG_H__ */
