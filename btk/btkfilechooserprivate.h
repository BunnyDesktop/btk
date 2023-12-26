/* BTK - The GIMP Toolkit
 * btkfilechooserprivate.h: Interface definition for file selector GUIs
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

#ifndef __BTK_FILE_CHOOSER_PRIVATE_H__
#define __BTK_FILE_CHOOSER_PRIVATE_H__

#include "btkfilechooser.h"
#include "btkfilesystem.h"
#include "btkfilesystemmodel.h"
#include "btkliststore.h"
#include "btkrecentmanager.h"
#include "btksearchengine.h"
#include "btkquery.h"
#include "btksizegroup.h"
#include "btktreemodelsort.h"
#include "btktreestore.h"
#include "btktreeview.h"
#include "btkvbox.h"

G_BEGIN_DECLS

#define BTK_FILE_CHOOSER_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), BTK_TYPE_FILE_CHOOSER, BtkFileChooserIface))

typedef struct _BtkFileChooserIface BtkFileChooserIface;

struct _BtkFileChooserIface
{
  GTypeInterface base_iface;

  /* Methods
   */
  gboolean       (*set_current_folder) 	   (BtkFileChooser    *chooser,
					    GFile             *file,
					    GError           **error);
  GFile *        (*get_current_folder) 	   (BtkFileChooser    *chooser);
  void           (*set_current_name)   	   (BtkFileChooser    *chooser,
					    const gchar       *name);
  gboolean       (*select_file)        	   (BtkFileChooser    *chooser,
					    GFile             *file,
					    GError           **error);
  void           (*unselect_file)      	   (BtkFileChooser    *chooser,
					    GFile             *file);
  void           (*select_all)         	   (BtkFileChooser    *chooser);
  void           (*unselect_all)       	   (BtkFileChooser    *chooser);
  GSList *       (*get_files)          	   (BtkFileChooser    *chooser);
  GFile *        (*get_preview_file)   	   (BtkFileChooser    *chooser);
  BtkFileSystem *(*get_file_system)    	   (BtkFileChooser    *chooser);
  void           (*add_filter)         	   (BtkFileChooser    *chooser,
					    BtkFileFilter     *filter);
  void           (*remove_filter)      	   (BtkFileChooser    *chooser,
					    BtkFileFilter     *filter);
  GSList *       (*list_filters)       	   (BtkFileChooser    *chooser);
  gboolean       (*add_shortcut_folder)    (BtkFileChooser    *chooser,
					    GFile             *file,
					    GError           **error);
  gboolean       (*remove_shortcut_folder) (BtkFileChooser    *chooser,
					    GFile             *file,
					    GError           **error);
  GSList *       (*list_shortcut_folders)  (BtkFileChooser    *chooser);
  
  /* Signals
   */
  void (*current_folder_changed) (BtkFileChooser *chooser);
  void (*selection_changed)      (BtkFileChooser *chooser);
  void (*update_preview)         (BtkFileChooser *chooser);
  void (*file_activated)         (BtkFileChooser *chooser);
  BtkFileChooserConfirmation (*confirm_overwrite) (BtkFileChooser *chooser);
};

BtkFileSystem *_btk_file_chooser_get_file_system         (BtkFileChooser    *chooser);
gboolean       _btk_file_chooser_add_shortcut_folder     (BtkFileChooser    *chooser,
							  GFile             *folder,
							  GError           **error);
gboolean       _btk_file_chooser_remove_shortcut_folder  (BtkFileChooser    *chooser,
							  GFile             *folder,
							  GError           **error);
GSList *       _btk_file_chooser_list_shortcut_folder_files (BtkFileChooser *chooser);

/* BtkFileChooserDialog private */

struct _BtkFileChooserDialogPrivate
{
  BtkWidget *widget;
  
  char *file_system;

  /* for use with BtkFileChooserEmbed */
  gboolean response_requested;
};


/* BtkFileChooserWidget private */

struct _BtkFileChooserWidgetPrivate
{
  BtkWidget *impl;

  char *file_system;
};


/* BtkFileChooserDefault private */

typedef enum {
  LOAD_EMPTY,			/* There is no model */
  LOAD_PRELOAD,			/* Model is loading and a timer is running; model isn't inserted into the tree yet */
  LOAD_LOADING,			/* Timeout expired, model is inserted into the tree, but not fully loaded yet */
  LOAD_FINISHED			/* Model is fully loaded and inserted into the tree */
} LoadState;

typedef enum {
  RELOAD_EMPTY,			/* No folder has been set */
  RELOAD_HAS_FOLDER		/* We have a folder, although it may not be completely loaded yet; no need to reload */
} ReloadState;

typedef enum {
  LOCATION_MODE_PATH_BAR,
  LOCATION_MODE_FILENAME_ENTRY
} LocationMode;

typedef enum {
  OPERATION_MODE_BROWSE,
  OPERATION_MODE_SEARCH,
  OPERATION_MODE_RECENT
} OperationMode;

typedef enum {
  STARTUP_MODE_RECENT,
  STARTUP_MODE_CWD
} StartupMode;

struct _BtkFileChooserDefault
{
  BtkVBox parent_instance;

  BtkFileChooserAction action;

  BtkFileSystem *file_system;

  /* Save mode widgets */
  BtkWidget *save_widgets;
  BtkWidget *save_widgets_table;

  BtkWidget *save_folder_label;

  /* The file browsing widgets */
  BtkWidget *browse_widgets_box;
  BtkWidget *browse_header_box;
  BtkWidget *browse_shortcuts_tree_view;
  BtkWidget *browse_shortcuts_add_button;
  BtkWidget *browse_shortcuts_remove_button;
  BtkWidget *browse_shortcuts_popup_menu;
  BtkWidget *browse_shortcuts_popup_menu_remove_item;
  BtkWidget *browse_shortcuts_popup_menu_rename_item;
  BtkWidget *browse_files_tree_view;
  BtkWidget *browse_files_popup_menu;
  BtkWidget *browse_files_popup_menu_add_shortcut_item;
  BtkWidget *browse_files_popup_menu_hidden_files_item;
  BtkWidget *browse_files_popup_menu_size_column_item;
  BtkWidget *browse_new_folder_button;
  BtkWidget *browse_path_bar_hbox;
  BtkSizeGroup *browse_path_bar_size_group;
  BtkWidget *browse_path_bar;
  BtkWidget *browse_special_mode_icon;
  BtkWidget *browse_special_mode_label;
  BtkWidget *browse_select_a_folder_info_bar;
  BtkWidget *browse_select_a_folder_label;
  BtkWidget *browse_select_a_folder_icon;

  gulong toplevel_unmapped_id;

  BtkFileSystemModel *browse_files_model;
  char *browse_files_last_selected_name;

  StartupMode startup_mode;

  /* OPERATION_MODE_SEARCH */
  BtkWidget *search_hbox;
  BtkWidget *search_entry;
  BtkSearchEngine *search_engine;
  BtkQuery *search_query;
  BtkFileSystemModel *search_model;

  /* OPERATION_MODE_RECENT */
  BtkRecentManager *recent_manager;
  BtkFileSystemModel *recent_model;
  guint load_recent_id;

  BtkWidget *filter_combo_hbox;
  BtkWidget *filter_combo;
  BtkWidget *preview_box;
  BtkWidget *preview_label;
  BtkWidget *preview_widget;
  BtkWidget *extra_align;
  BtkWidget *extra_widget;

  BtkWidget *location_button;
  BtkWidget *location_entry_box;
  BtkWidget *location_label;
  BtkWidget *location_entry;
  LocationMode location_mode;

  BtkListStore *shortcuts_model;

  /* Filter for the shortcuts pane.  We filter out the "current folder" row and
   * the separator that we use for the "Save in folder" combo.
   */
  BtkTreeModel *shortcuts_pane_filter_model;
  
  /* Handles */
  GSList *loading_shortcuts;
  GSList *reload_icon_cancellables;
  GCancellable *file_list_drag_data_received_cancellable;
  GCancellable *update_current_folder_cancellable;
  GCancellable *should_respond_get_info_cancellable;
  GCancellable *file_exists_get_info_cancellable;
  GCancellable *update_from_entry_cancellable;
  GCancellable *shortcuts_activate_iter_cancellable;

  LoadState load_state;
  ReloadState reload_state;
  guint load_timeout_id;

  OperationMode operation_mode;

  GSList *pending_select_files;

  BtkFileFilter *current_filter;
  GSList *filters;

  int num_volumes;
  int num_shortcuts;
  int num_bookmarks;

  gulong volumes_changed_id;
  gulong bookmarks_changed_id;

  GFile *current_volume_file;
  GFile *current_folder;
  GFile *preview_file;
  char *preview_display_name;

  BtkTreeViewColumn *list_name_column;
  BtkCellRenderer *list_name_renderer;
  BtkTreeViewColumn *list_mtime_column;
  BtkTreeViewColumn *list_size_column;

  GSource *edited_idle;
  char *edited_new_text;

  gulong settings_signal_id;
  int icon_size;

  GSource *focus_entry_idle;

  gulong toplevel_set_focus_id;
  BtkWidget *toplevel_last_focus_widget;

  gint sort_column;
  BtkSortType sort_order;

#if 0
  BdkDragContext *shortcuts_drag_context;
  GSource *shortcuts_drag_outside_idle;
#endif

  /* Flags */

  guint local_only : 1;
  guint preview_widget_active : 1;
  guint use_preview_label : 1;
  guint select_multiple : 1;
  guint show_hidden : 1;
  guint do_overwrite_confirmation : 1;
  guint list_sort_ascending : 1;
  guint changing_folder : 1;
  guint shortcuts_current_folder_active : 1;
  guint has_cwd : 1;
  guint has_home : 1;
  guint has_desktop : 1;
  guint has_search : 1;
  guint show_size_column : 1;
  guint create_folders : 1;

#if 0
  guint shortcuts_drag_outside : 1;
#endif
};


G_END_DECLS

#endif /* __BTK_FILE_CHOOSER_PRIVATE_H__ */
