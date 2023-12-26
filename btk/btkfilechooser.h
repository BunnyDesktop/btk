/* BTK - The GIMP Toolkit
 * btkfilechooser.h: Abstract interface for file selector GUIs
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

#ifndef __BTK_FILE_CHOOSER_H__
#define __BTK_FILE_CHOOSER_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkfilefilter.h>
#include <btk/btkwidget.h>

B_BEGIN_DECLS

#define BTK_TYPE_FILE_CHOOSER             (btk_file_chooser_get_type ())
#define BTK_FILE_CHOOSER(obj)             (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_FILE_CHOOSER, BtkFileChooser))
#define BTK_IS_FILE_CHOOSER(obj)          (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_FILE_CHOOSER))

typedef struct _BtkFileChooser      BtkFileChooser;

/**
 * BtkFileChooserAction:
 * @BTK_FILE_CHOOSER_ACTION_OPEN: Indicates open mode.  The file chooser
 *  will only let the user pick an existing file.
 * @BTK_FILE_CHOOSER_ACTION_SAVE: Indicates save mode.  The file chooser
 *  will let the user pick an existing file, or type in a new
 *  filename.
 * @BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER: Indicates an Open mode for
 *  selecting folders.  The file chooser will let the user pick an
 *  existing folder.
 * @BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER: Indicates a mode for creating a
 *  new folder.  The file chooser will let the user name an existing or
 *  new folder.
 *
 * Describes whether a #BtkFileChooser is being used to open existing files
 * or to save to a possibly new file.
 */
typedef enum
{
  BTK_FILE_CHOOSER_ACTION_OPEN,
  BTK_FILE_CHOOSER_ACTION_SAVE,
  BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
  BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER
} BtkFileChooserAction;

/**
 * BtkFileChooserConfirmation:
 * @BTK_FILE_CHOOSER_CONFIRMATION_CONFIRM: The file chooser will present
 *  its stock dialog to confirm about overwriting an existing file.
 * @BTK_FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME: The file chooser will
 *  terminate and accept the user's choice of a file name.
 * @BTK_FILE_CHOOSER_CONFIRMATION_SELECT_AGAIN: The file chooser will
 *  continue running, so as to let the user select another file name.
 *
 * Used as a return value of handlers for the
 * #BtkFileChooser::confirm-overwrite signal of a #BtkFileChooser. This
 * value determines whether the file chooser will present the stock
 * confirmation dialog, accept the user's choice of a filename, or
 * let the user choose another filename.
 *
 * Since: 2.8
 */
typedef enum
{
  BTK_FILE_CHOOSER_CONFIRMATION_CONFIRM,
  BTK_FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME,
  BTK_FILE_CHOOSER_CONFIRMATION_SELECT_AGAIN
} BtkFileChooserConfirmation;

GType btk_file_chooser_get_type (void) B_GNUC_CONST;

/* GError enumeration for BtkFileChooser */
/**
 * BTK_FILE_CHOOSER_ERROR:
 *
 * Used to get the #GError quark for #BtkFileChooser errors.
 */
#define BTK_FILE_CHOOSER_ERROR (btk_file_chooser_error_quark ())

/**
 * BtkFileChooserError:
 * @BTK_FILE_CHOOSER_ERROR_NONEXISTENT: Indicates that a file does not exist.
 * @BTK_FILE_CHOOSER_ERROR_BAD_FILENAME: Indicates a malformed filename.
 * @BTK_FILE_CHOOSER_ERROR_ALREADY_EXISTS: Indicates a duplicate path (e.g. when
 *  adding a bookmark).
 * @BTK_FILE_CHOOSER_ERROR_INCOMPLETE_HOSTNAME: Indicates an incomplete hostname (e.g. "http://foo" without a slash after that).
 *
 * These identify the various errors that can occur while calling
 * #BtkFileChooser functions.
 */
typedef enum {
  BTK_FILE_CHOOSER_ERROR_NONEXISTENT,
  BTK_FILE_CHOOSER_ERROR_BAD_FILENAME,
  BTK_FILE_CHOOSER_ERROR_ALREADY_EXISTS,
  BTK_FILE_CHOOSER_ERROR_INCOMPLETE_HOSTNAME
} BtkFileChooserError;

GQuark btk_file_chooser_error_quark (void);

/* Configuration
 */
void                 btk_file_chooser_set_action          (BtkFileChooser       *chooser,
							   BtkFileChooserAction  action);
BtkFileChooserAction btk_file_chooser_get_action          (BtkFileChooser       *chooser);
void                 btk_file_chooser_set_local_only      (BtkFileChooser       *chooser,
							   gboolean              local_only);
gboolean             btk_file_chooser_get_local_only      (BtkFileChooser       *chooser);
void                 btk_file_chooser_set_select_multiple (BtkFileChooser       *chooser,
							   gboolean              select_multiple);
gboolean             btk_file_chooser_get_select_multiple (BtkFileChooser       *chooser);
void                 btk_file_chooser_set_show_hidden     (BtkFileChooser       *chooser,
							   gboolean              show_hidden);
gboolean             btk_file_chooser_get_show_hidden     (BtkFileChooser       *chooser);

void                 btk_file_chooser_set_do_overwrite_confirmation (BtkFileChooser *chooser,
								     gboolean        do_overwrite_confirmation);
gboolean             btk_file_chooser_get_do_overwrite_confirmation (BtkFileChooser *chooser);

void                 btk_file_chooser_set_create_folders  (BtkFileChooser       *chooser,
							  gboolean               create_folders);
gboolean             btk_file_chooser_get_create_folders (BtkFileChooser *chooser);

/* Suggested name for the Save-type actions
 */
void     btk_file_chooser_set_current_name   (BtkFileChooser *chooser,
					      const gchar    *name);

/* Filename manipulation
 */
#ifdef G_OS_WIN32
/* Reserve old names for DLL ABI backward compatibility */
#define btk_file_chooser_get_filename btk_file_chooser_get_filename_utf8
#define btk_file_chooser_set_filename btk_file_chooser_set_filename_utf8
#define btk_file_chooser_select_filename btk_file_chooser_select_filename_utf8
#define btk_file_chooser_unselect_filename btk_file_chooser_unselect_filename_utf8
#define btk_file_chooser_get_filenames btk_file_chooser_get_filenames_utf8
#define btk_file_chooser_set_current_folder btk_file_chooser_set_current_folder_utf8
#define btk_file_chooser_get_current_folder btk_file_chooser_get_current_folder_utf8
#define btk_file_chooser_get_preview_filename btk_file_chooser_get_preview_filename_utf8
#define btk_file_chooser_add_shortcut_folder btk_file_chooser_add_shortcut_folder_utf8
#define btk_file_chooser_remove_shortcut_folder btk_file_chooser_remove_shortcut_folder_utf8
#define btk_file_chooser_list_shortcut_folders btk_file_chooser_list_shortcut_folders_utf8
#endif

gchar *  btk_file_chooser_get_filename       (BtkFileChooser *chooser);
gboolean btk_file_chooser_set_filename       (BtkFileChooser *chooser,
					      const char     *filename);
gboolean btk_file_chooser_select_filename    (BtkFileChooser *chooser,
					      const char     *filename);
void     btk_file_chooser_unselect_filename  (BtkFileChooser *chooser,
					      const char     *filename);
void     btk_file_chooser_select_all         (BtkFileChooser *chooser);
void     btk_file_chooser_unselect_all       (BtkFileChooser *chooser);
GSList * btk_file_chooser_get_filenames      (BtkFileChooser *chooser);
gboolean btk_file_chooser_set_current_folder (BtkFileChooser *chooser,
					      const gchar    *filename);
gchar *  btk_file_chooser_get_current_folder (BtkFileChooser *chooser);


/* URI manipulation
 */
gchar *  btk_file_chooser_get_uri                (BtkFileChooser *chooser);
gboolean btk_file_chooser_set_uri                (BtkFileChooser *chooser,
						  const char     *uri);
gboolean btk_file_chooser_select_uri             (BtkFileChooser *chooser,
						  const char     *uri);
void     btk_file_chooser_unselect_uri           (BtkFileChooser *chooser,
						  const char     *uri);
GSList * btk_file_chooser_get_uris               (BtkFileChooser *chooser);
gboolean btk_file_chooser_set_current_folder_uri (BtkFileChooser *chooser,
						  const gchar    *uri);
gchar *  btk_file_chooser_get_current_folder_uri (BtkFileChooser *chooser);

/* GFile manipulation */
GFile *  btk_file_chooser_get_file                (BtkFileChooser  *chooser);
gboolean btk_file_chooser_set_file                (BtkFileChooser  *chooser,
                                                   GFile           *file,
                                                   GError         **error);
gboolean btk_file_chooser_select_file             (BtkFileChooser  *chooser,
                                                   GFile           *file,
                                                   GError         **error);
void     btk_file_chooser_unselect_file           (BtkFileChooser  *chooser,
                                                   GFile           *file);
GSList * btk_file_chooser_get_files               (BtkFileChooser  *chooser);
gboolean btk_file_chooser_set_current_folder_file (BtkFileChooser  *chooser,
                                                   GFile           *file,
                                                   GError         **error);
GFile *  btk_file_chooser_get_current_folder_file (BtkFileChooser  *chooser);

/* Preview widget
 */
void       btk_file_chooser_set_preview_widget        (BtkFileChooser *chooser,
						       BtkWidget      *preview_widget);
BtkWidget *btk_file_chooser_get_preview_widget        (BtkFileChooser *chooser);
void       btk_file_chooser_set_preview_widget_active (BtkFileChooser *chooser,
						       gboolean        active);
gboolean   btk_file_chooser_get_preview_widget_active (BtkFileChooser *chooser);
void       btk_file_chooser_set_use_preview_label     (BtkFileChooser *chooser,
						       gboolean        use_label);
gboolean   btk_file_chooser_get_use_preview_label     (BtkFileChooser *chooser);

char  *btk_file_chooser_get_preview_filename (BtkFileChooser *chooser);
char  *btk_file_chooser_get_preview_uri      (BtkFileChooser *chooser);
GFile *btk_file_chooser_get_preview_file     (BtkFileChooser *chooser);

/* Extra widget
 */
void       btk_file_chooser_set_extra_widget (BtkFileChooser *chooser,
					      BtkWidget      *extra_widget);
BtkWidget *btk_file_chooser_get_extra_widget (BtkFileChooser *chooser);

/* List of user selectable filters
 */
void    btk_file_chooser_add_filter    (BtkFileChooser *chooser,
					BtkFileFilter  *filter);
void    btk_file_chooser_remove_filter (BtkFileChooser *chooser,
					BtkFileFilter  *filter);
GSList *btk_file_chooser_list_filters  (BtkFileChooser *chooser);

/* Current filter
 */
void           btk_file_chooser_set_filter (BtkFileChooser *chooser,
					   BtkFileFilter  *filter);
BtkFileFilter *btk_file_chooser_get_filter (BtkFileChooser *chooser);

/* Per-application shortcut folders */

gboolean btk_file_chooser_add_shortcut_folder    (BtkFileChooser *chooser,
						  const char     *folder,
						  GError        **error);
gboolean btk_file_chooser_remove_shortcut_folder (BtkFileChooser *chooser,
						  const char     *folder,
						  GError        **error);
GSList *btk_file_chooser_list_shortcut_folders   (BtkFileChooser *chooser);

gboolean btk_file_chooser_add_shortcut_folder_uri    (BtkFileChooser *chooser,
						      const char     *uri,
						      GError        **error);
gboolean btk_file_chooser_remove_shortcut_folder_uri (BtkFileChooser *chooser,
						      const char     *uri,
						      GError        **error);
GSList *btk_file_chooser_list_shortcut_folder_uris   (BtkFileChooser *chooser);

B_END_DECLS

#endif /* __BTK_FILE_CHOOSER_H__ */
