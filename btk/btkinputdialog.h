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

/*
 * NOTE this widget is considered too specialized/little-used for
 * BTK+, and will in the future be moved to some other package.  If
 * your application needs this widget, feel free to use it, as the
 * widget does work and is useful in some applications; it's just not
 * of general interest. However, we are not accepting new features for
 * the widget, and it will eventually move out of the BTK+
 * distribution.
 */

#ifndef BTK_DISABLE_DEPRECATED

#ifndef __BTK_INPUTDIALOG_H__
#define __BTK_INPUTDIALOG_H__


#include <btk/btkdialog.h>


G_BEGIN_DECLS

#define BTK_TYPE_INPUT_DIALOG              (btk_input_dialog_get_type ())
#define BTK_INPUT_DIALOG(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_INPUT_DIALOG, BtkInputDialog))
#define BTK_INPUT_DIALOG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_INPUT_DIALOG, BtkInputDialogClass))
#define BTK_IS_INPUT_DIALOG(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_INPUT_DIALOG))
#define BTK_IS_INPUT_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_INPUT_DIALOG))
#define BTK_INPUT_DIALOG_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_INPUT_DIALOG, BtkInputDialogClass))


typedef struct _BtkInputDialog       BtkInputDialog;
typedef struct _BtkInputDialogClass  BtkInputDialogClass;

struct _BtkInputDialog
{
  BtkDialog dialog;

  BtkWidget *GSEAL (axis_list);
  BtkWidget *GSEAL (axis_listbox);
  BtkWidget *GSEAL (mode_optionmenu);

  BtkWidget *GSEAL (close_button);
  BtkWidget *GSEAL (save_button);

  BtkWidget *GSEAL (axis_items[BDK_AXIS_LAST]);
  BdkDevice *GSEAL (current_device);

  BtkWidget *GSEAL (keys_list);
  BtkWidget *GSEAL (keys_listbox);
};

struct _BtkInputDialogClass
{
  BtkDialogClass parent_class;

  void (* enable_device)               (BtkInputDialog    *inputd,
					BdkDevice         *device);
  void (* disable_device)              (BtkInputDialog    *inputd,
					BdkDevice         *device);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

GType      btk_input_dialog_get_type     (void) G_GNUC_CONST;
BtkWidget* btk_input_dialog_new          (void);

G_END_DECLS

#endif /* __BTK_INPUTDIALOG_H__ */

#endif /* BTK_DISABLE_DEPRECATED */
