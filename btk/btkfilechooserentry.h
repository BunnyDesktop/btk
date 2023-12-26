/* BTK - The GIMP Toolkit
 * btkfilechooserentry.h: Entry with filename completion
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

#ifndef __BTK_FILE_CHOOSER_ENTRY_H__
#define __BTK_FILE_CHOOSER_ENTRY_H__

#include "btkfilesystem.h"
#include "btkfilechooser.h"

G_BEGIN_DECLS

#define BTK_TYPE_FILE_CHOOSER_ENTRY    (_btk_file_chooser_entry_get_type ())
#define BTK_FILE_CHOOSER_ENTRY(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_FILE_CHOOSER_ENTRY, BtkFileChooserEntry))
#define BTK_IS_FILE_CHOOSER_ENTRY(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_FILE_CHOOSER_ENTRY))

typedef struct _BtkFileChooserEntry      BtkFileChooserEntry;

GType              _btk_file_chooser_entry_get_type           (void) G_GNUC_CONST;
BtkWidget *        _btk_file_chooser_entry_new                (gboolean             eat_tab);
void               _btk_file_chooser_entry_set_action         (BtkFileChooserEntry *chooser_entry,
							       BtkFileChooserAction action);
BtkFileChooserAction _btk_file_chooser_entry_get_action       (BtkFileChooserEntry *chooser_entry);
void               _btk_file_chooser_entry_set_base_folder    (BtkFileChooserEntry *chooser_entry,
							       GFile               *folder);
GFile *            _btk_file_chooser_entry_get_current_folder (BtkFileChooserEntry *chooser_entry);
const gchar *      _btk_file_chooser_entry_get_file_part      (BtkFileChooserEntry *chooser_entry);
gboolean           _btk_file_chooser_entry_get_is_folder      (BtkFileChooserEntry *chooser_entry,
							       GFile               *file);
void               _btk_file_chooser_entry_select_filename    (BtkFileChooserEntry *chooser_entry);
void               _btk_file_chooser_entry_set_local_only     (BtkFileChooserEntry *chooser_entry,
                                                               gboolean             local_only);
gboolean           _btk_file_chooser_entry_get_local_only     (BtkFileChooserEntry *chooser_entry);

G_END_DECLS

#endif /* __BTK_FILE_CHOOSER_ENTRY_H__ */
