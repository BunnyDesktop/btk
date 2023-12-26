/* BTK - The GIMP Toolkit
 * btkfilesystem.h: Filesystem abstraction functions.
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

#ifndef __BTK_FILE_SYSTEM_H__
#define __BTK_FILE_SYSTEM_H__

#include <bunnyio/bunnyio.h>
#include <btk/btkwidget.h>	/* For icon handling */

G_BEGIN_DECLS

#define BTK_TYPE_FILE_SYSTEM         (_btk_file_system_get_type ())
#define BTK_FILE_SYSTEM(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BTK_TYPE_FILE_SYSTEM, BtkFileSystem))
#define BTK_FILE_SYSTEM_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST    ((c), BTK_TYPE_FILE_SYSTEM, BtkFileSystemClass))
#define BTK_IS_FILE_SYSTEM(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BTK_TYPE_FILE_SYSTEM))
#define BTK_IS_FILE_SYSTEM_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE    ((c), BTK_TYPE_FILE_SYSTEM))
#define BTK_FILE_SYSTEM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS  ((o), BTK_TYPE_FILE_SYSTEM, BtkFileSystemClass))

#define BTK_TYPE_FOLDER         (_btk_folder_get_type ())
#define BTK_FOLDER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BTK_TYPE_FOLDER, BtkFolder))
#define BTK_FOLDER_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST    ((c), BTK_TYPE_FOLDER, BtkFolderClass))
#define BTK_IS_FOLDER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BTK_TYPE_FOLDER))
#define BTK_IS_FOLDER_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE    ((c), BTK_TYPE_FOLDER))
#define BTK_FOLDER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS  ((o), BTK_TYPE_FOLDER, BtkFolderClass))

typedef struct BtkFileSystemClass BtkFileSystemClass;
typedef struct BtkFileSystem BtkFileSystem;
typedef struct BtkFolderClass BtkFolderClass;
typedef struct BtkFolder BtkFolder;
typedef struct BtkFileSystemVolume BtkFileSystemVolume; /* opaque struct */
typedef struct BtkFileSystemBookmark BtkFileSystemBookmark; /* opaque struct */

struct BtkFileSystemClass
{
  GObjectClass parent_class;

  void (*bookmarks_changed) (BtkFileSystem *file_system);
  void (*volumes_changed)   (BtkFileSystem *file_system);
};

struct BtkFileSystem
{
  GObject parent_object;
};

struct BtkFolderClass
{
  GObjectClass parent_class;

  void (*files_added)      (BtkFolder *folder,
			    GList     *paths);
  void (*files_removed)    (BtkFolder *folder,
			    GList     *paths);
  void (*files_changed)    (BtkFolder *folder,
			    GList     *paths);
  void (*finished_loading) (BtkFolder *folder);
  void (*deleted)          (BtkFolder *folder);
};

struct BtkFolder
{
  GObject parent_object;
};

typedef void (* BtkFileSystemGetFolderCallback)    (GCancellable        *cancellable,
						    BtkFolder           *folder,
						    const GError        *error,
						    gpointer             data);
typedef void (* BtkFileSystemGetInfoCallback)      (GCancellable        *cancellable,
						    GFileInfo           *file_info,
						    const GError        *error,
						    gpointer             data);
typedef void (* BtkFileSystemVolumeMountCallback)  (GCancellable        *cancellable,
						    BtkFileSystemVolume *volume,
						    const GError        *error,
						    gpointer             data);

/* BtkFileSystem methods */
GType           _btk_file_system_get_type     (void) G_GNUC_CONST;

BtkFileSystem * _btk_file_system_new          (void);

GSList *        _btk_file_system_list_volumes   (BtkFileSystem *file_system);
GSList *        _btk_file_system_list_bookmarks (BtkFileSystem *file_system);

GCancellable *  _btk_file_system_get_info               (BtkFileSystem                     *file_system,
							 GFile                             *file,
							 const gchar                       *attributes,
							 BtkFileSystemGetInfoCallback       callback,
							 gpointer                           data);
GCancellable *  _btk_file_system_mount_volume           (BtkFileSystem                     *file_system,
							 BtkFileSystemVolume               *volume,
							 GMountOperation                   *mount_operation,
							 BtkFileSystemVolumeMountCallback   callback,
							 gpointer                           data);
GCancellable *  _btk_file_system_mount_enclosing_volume (BtkFileSystem                     *file_system,
							 GFile                             *file,
							 GMountOperation                   *mount_operation,
							 BtkFileSystemVolumeMountCallback   callback,
							 gpointer                           data);

gboolean        _btk_file_system_insert_bookmark    (BtkFileSystem      *file_system,
						     GFile              *file,
						     gint                position,
						     GError            **error);
gboolean        _btk_file_system_remove_bookmark    (BtkFileSystem      *file_system,
						     GFile              *file,
						     GError            **error);

gchar *         _btk_file_system_get_bookmark_label (BtkFileSystem *file_system,
						     GFile         *file);
void            _btk_file_system_set_bookmark_label (BtkFileSystem *file_system,
						     GFile         *file,
						     const gchar   *label);

BtkFileSystemVolume * _btk_file_system_get_volume_for_file (BtkFileSystem       *file_system,
							    GFile               *file);

/* BtkFolder functions */
GSList *     _btk_folder_list_children (BtkFolder  *folder);
GFileInfo *  _btk_folder_get_info      (BtkFolder  *folder,
				        GFile      *file);

gboolean     _btk_folder_is_finished_loading (BtkFolder *folder);


/* BtkFileSystemVolume methods */
gchar *               _btk_file_system_volume_get_display_name (BtkFileSystemVolume *volume);
gboolean              _btk_file_system_volume_is_mounted       (BtkFileSystemVolume *volume);
GFile *               _btk_file_system_volume_get_root         (BtkFileSystemVolume *volume);
BdkPixbuf *           _btk_file_system_volume_render_icon      (BtkFileSystemVolume  *volume,
							        BtkWidget            *widget,
							        gint                  icon_size,
							        GError              **error);

BtkFileSystemVolume  *_btk_file_system_volume_ref              (BtkFileSystemVolume *volume);
void                  _btk_file_system_volume_unref            (BtkFileSystemVolume *volume);

/* BtkFileSystemBookmark methods */
void                   _btk_file_system_bookmark_free          (BtkFileSystemBookmark *bookmark);

/* GFileInfo helper functions */
BdkPixbuf *     _btk_file_info_render_icon (GFileInfo *info,
					    BtkWidget *widget,
					    gint       icon_size);

gboolean	_btk_file_info_consider_as_directory (GFileInfo *info);

/* GFile helper functions */
gboolean	_btk_file_has_native_path (GFile *file);

G_END_DECLS

#endif /* __BTK_FILE_SYSTEM_H__ */
