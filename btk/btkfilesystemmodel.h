/* BTK - The GIMP Toolkit
 * btkfilesystemmodel.h: BtkTreeModel wrapping a BtkFileSystem
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

#ifndef __BTK_FILE_SYSTEM_MODEL_H__
#define __BTK_FILE_SYSTEM_MODEL_H__

#include <bunnyio/bunnyio.h>
#include <btk/btkfilefilter.h>
#include <btk/btktreemodel.h>

B_BEGIN_DECLS

#define BTK_TYPE_FILE_SYSTEM_MODEL             (_btk_file_system_model_get_type ())
#define BTK_FILE_SYSTEM_MODEL(obj)             (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_FILE_SYSTEM_MODEL, BtkFileSystemModel))
#define BTK_IS_FILE_SYSTEM_MODEL(obj)          (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_FILE_SYSTEM_MODEL))

typedef struct _BtkFileSystemModel      BtkFileSystemModel;

GType _btk_file_system_model_get_type (void) B_GNUC_CONST;

typedef gboolean (*BtkFileSystemModelGetValue)   (BtkFileSystemModel *model,
                                                  GFile              *file,
                                                  GFileInfo          *info,
                                                  int                 column,
                                                  BValue             *value,
                                                  gpointer            user_data);

BtkFileSystemModel *_btk_file_system_model_new              (BtkFileSystemModelGetValue get_func,
                                                             gpointer            get_data,
                                                             guint               n_columns,
                                                             ...);
BtkFileSystemModel *_btk_file_system_model_new_for_directory(GFile *             dir,
                                                             const gchar *       attributes,
                                                             BtkFileSystemModelGetValue get_func,
                                                             gpointer            get_data,
                                                             guint               n_columns,
                                                             ...);
GCancellable *      _btk_file_system_model_get_cancellable  (BtkFileSystemModel *model);
gboolean            _btk_file_system_model_iter_is_visible  (BtkFileSystemModel *model,
							     BtkTreeIter        *iter);
gboolean            _btk_file_system_model_iter_is_filtered_out (BtkFileSystemModel *model,
								 BtkTreeIter        *iter);
GFileInfo *         _btk_file_system_model_get_info         (BtkFileSystemModel *model,
							     BtkTreeIter        *iter);
gboolean            _btk_file_system_model_get_iter_for_file(BtkFileSystemModel *model,
							     BtkTreeIter        *iter,
							     GFile              *file);
GFile *             _btk_file_system_model_get_file         (BtkFileSystemModel *model,
							     BtkTreeIter        *iter);
const BValue *      _btk_file_system_model_get_value        (BtkFileSystemModel *model,
                                                             BtkTreeIter *       iter,
                                                             int                 column);

void                _btk_file_system_model_add_and_query_file (BtkFileSystemModel *model,
                                                             GFile              *file,
                                                             const char         *attributes);
void                _btk_file_system_model_update_file      (BtkFileSystemModel *model,
                                                             GFile              *file,
                                                             GFileInfo          *info);

void                _btk_file_system_model_set_show_hidden  (BtkFileSystemModel *model,
							     gboolean            show_hidden);
void                _btk_file_system_model_set_show_folders (BtkFileSystemModel *model,
							     gboolean            show_folders);
void                _btk_file_system_model_set_show_files   (BtkFileSystemModel *model,
							     gboolean            show_files);
void                _btk_file_system_model_set_filter_folders (BtkFileSystemModel *model,
							     gboolean            show_folders);
void                _btk_file_system_model_clear_cache      (BtkFileSystemModel *model,
                                                             int                 column);

void                _btk_file_system_model_set_filter       (BtkFileSystemModel *model,
                                                             BtkFileFilter      *filter);

void _btk_file_system_model_add_editable    (BtkFileSystemModel *model,
					     BtkTreeIter        *iter);
void _btk_file_system_model_remove_editable (BtkFileSystemModel *model);

B_END_DECLS

#endif /* __BTK_FILE_SYSTEM_MODEL_H__ */
