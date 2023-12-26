/* BTK - The GIMP Toolkit
 * btkfilechooserutils.h: Private utility functions useful for
 *                        implementing a BtkFileChooser interface
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

#ifndef __BTK_FILE_CHOOSER_UTILS_H__
#define __BTK_FILE_CHOOSER_UTILS_H__

#include "btkfilechooserprivate.h"

G_BEGIN_DECLS

#define BTK_FILE_CHOOSER_DELEGATE_QUARK	  (_btk_file_chooser_delegate_get_quark ())

typedef enum {
  BTK_FILE_CHOOSER_PROP_FIRST                  = 0x1000,
  BTK_FILE_CHOOSER_PROP_ACTION                 = BTK_FILE_CHOOSER_PROP_FIRST,
  BTK_FILE_CHOOSER_PROP_FILE_SYSTEM_BACKEND,
  BTK_FILE_CHOOSER_PROP_FILTER,
  BTK_FILE_CHOOSER_PROP_LOCAL_ONLY,
  BTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET, 
  BTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET_ACTIVE,
  BTK_FILE_CHOOSER_PROP_USE_PREVIEW_LABEL,
  BTK_FILE_CHOOSER_PROP_EXTRA_WIDGET,
  BTK_FILE_CHOOSER_PROP_SELECT_MULTIPLE,
  BTK_FILE_CHOOSER_PROP_SHOW_HIDDEN,
  BTK_FILE_CHOOSER_PROP_DO_OVERWRITE_CONFIRMATION,
  BTK_FILE_CHOOSER_PROP_CREATE_FOLDERS,
  BTK_FILE_CHOOSER_PROP_LAST                   = BTK_FILE_CHOOSER_PROP_CREATE_FOLDERS
} BtkFileChooserProp;

void _btk_file_chooser_install_properties (GObjectClass *klass);

void _btk_file_chooser_delegate_iface_init (BtkFileChooserIface *iface);
void _btk_file_chooser_set_delegate        (BtkFileChooser *receiver,
					    BtkFileChooser *delegate);

GQuark _btk_file_chooser_delegate_get_quark (void) G_GNUC_CONST;

GList *_btk_file_chooser_extract_recent_folders (GList *infos);

G_END_DECLS

#endif /* __BTK_FILE_CHOOSER_UTILS_H__ */
