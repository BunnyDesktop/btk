/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* BTK - The GIMP Toolkit
 * btkfilechooserdefault.c: Default implementation of BtkFileChooser
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

#include "config.h"

#include "bdk/bdkkeysyms.h"
#include "btkalignment.h"
#include "btkbindings.h"
#include "btkcelllayout.h"
#include "btkcellrendererpixbuf.h"
#include "btkcellrenderertext.h"
#include "btkcheckmenuitem.h"
#include "btkclipboard.h"
#include "btkcomboboxtext.h"
#include "btkentry.h"
#include "btkfilechooserprivate.h"
#include "btkfilechooserdefault.h"
#include "btkfilechooserdialog.h"
#include "btkfilechooserembed.h"
#include "btkfilechooserentry.h"
#include "btkfilechoosersettings.h"
#include "btkfilechooserutils.h"
#include "btkfilechooser.h"
#include "btkfilesystem.h"
#include "btkfilesystemmodel.h"
#include "btkframe.h"
#include "btkhbox.h"
#include "btkhpaned.h"
#include "btkiconfactory.h"
#include "btkicontheme.h"
#include "btkimage.h"
#include "btkimagemenuitem.h"
#include "btkinfobar.h"
#include "btklabel.h"
#include "btkmarshalers.h"
#include "btkmessagedialog.h"
#include "btkmountoperation.h"
#include "btkpathbar.h"
#include "btkprivate.h"
#include "btkradiobutton.h"
#include "btkrecentfilter.h"
#include "btkrecentmanager.h"
#include "btkscrolledwindow.h"
#include "btkseparatormenuitem.h"
#include "btksizegroup.h"
#include "btkstock.h"
#include "btktable.h"
#include "btktoolbar.h"
#include "btktoolbutton.h"
#include "btktooltip.h"
#include "btktreednd.h"
#include "btktreeprivate.h"
#include "btktreeselection.h"
#include "btkvbox.h"
#include "btkintl.h"

#include "btkalias.h"

#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <locale.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef G_OS_WIN32
#include <io.h>
#endif

/* Profiling stuff */
#undef PROFILE_FILE_CHOOSER
#ifdef PROFILE_FILE_CHOOSER


#ifndef F_OK 
#define F_OK 0
#endif

#define PROFILE_INDENT 4
static int profile_indent;

static void
profile_add_indent (int indent)
{
  profile_indent += indent;
  if (profile_indent < 0)
    g_error ("You screwed up your indentation");
}

void
_btk_file_chooser_profile_log (const char *func, int indent, const char *msg1, const char *msg2)
{
  char *str;

  if (indent < 0)
    profile_add_indent (indent);

  if (profile_indent == 0)
    str = g_strdup_printf ("MARK: %s %s %s", func ? func : "", msg1 ? msg1 : "", msg2 ? msg2 : "");
  else
    str = g_strdup_printf ("MARK: %*c %s %s %s", profile_indent - 1, ' ', func ? func : "", msg1 ? msg1 : "", msg2 ? msg2 : "");

  access (str, F_OK);
  g_free (str);

  if (indent > 0)
    profile_add_indent (indent);
}

#define profile_start(x, y) _btk_file_chooser_profile_log (B_STRFUNC, PROFILE_INDENT, x, y)
#define profile_end(x, y) _btk_file_chooser_profile_log (B_STRFUNC, -PROFILE_INDENT, x, y)
#define profile_msg(x, y) _btk_file_chooser_profile_log (NULL, 0, x, y)
#else
#define profile_start(x, y)
#define profile_end(x, y)
#define profile_msg(x, y)
#endif



typedef struct _BtkFileChooserDefaultClass BtkFileChooserDefaultClass;

#define BTK_FILE_CHOOSER_DEFAULT_CLASS(klass)     (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_FILE_CHOOSER_DEFAULT, BtkFileChooserDefaultClass))
#define BTK_IS_FILE_CHOOSER_DEFAULT_CLASS(klass)  (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_FILE_CHOOSER_DEFAULT))
#define BTK_FILE_CHOOSER_DEFAULT_GET_CLASS(obj)   (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_FILE_CHOOSER_DEFAULT, BtkFileChooserDefaultClass))

#define MAX_LOADING_TIME 500

#define DEFAULT_NEW_FOLDER_NAME _("Type name of new folder")

struct _BtkFileChooserDefaultClass
{
  BtkVBoxClass parent_class;
};

/* Signal IDs */
enum {
  LOCATION_POPUP,
  LOCATION_POPUP_ON_PASTE,
  UP_FOLDER,
  DOWN_FOLDER,
  HOME_FOLDER,
  DESKTOP_FOLDER,
  QUICK_BOOKMARK,
  LOCATION_TOGGLE_POPUP,
  SHOW_HIDDEN,
  SEARCH_SHORTCUT,
  RECENT_SHORTCUT,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/* Column numbers for the shortcuts tree.  Keep these in sync with shortcuts_model_create() */
enum {
  SHORTCUTS_COL_PIXBUF,
  SHORTCUTS_COL_NAME,
  SHORTCUTS_COL_DATA,
  SHORTCUTS_COL_TYPE,
  SHORTCUTS_COL_REMOVABLE,
  SHORTCUTS_COL_PIXBUF_VISIBLE,
  SHORTCUTS_COL_CANCELLABLE,
  SHORTCUTS_COL_NUM_COLUMNS
};

typedef enum {
  SHORTCUT_TYPE_FILE,
  SHORTCUT_TYPE_VOLUME,
  SHORTCUT_TYPE_SEPARATOR,
  SHORTCUT_TYPE_SEARCH,
  SHORTCUT_TYPE_RECENT
} ShortcutType;

#define MODEL_ATTRIBUTES "standard::name,standard::type,standard::display-name," \
                         "standard::is-hidden,standard::is-backup,standard::size," \
                         "standard::content-type,time::modified"
enum {
  /* the first 3 must be these due to settings caching sort column */
  MODEL_COL_NAME,
  MODEL_COL_SIZE,
  MODEL_COL_MTIME,
  MODEL_COL_FILE,
  MODEL_COL_NAME_COLLATED,
  MODEL_COL_IS_FOLDER,
  MODEL_COL_IS_SENSITIVE,
  MODEL_COL_PIXBUF,
  MODEL_COL_SIZE_TEXT,
  MODEL_COL_MTIME_TEXT,
  MODEL_COL_ELLIPSIZE,
  MODEL_COL_NUM_COLUMNS
};

/* This list of types is passed to _btk_file_system_model_new*() */
#define MODEL_COLUMN_TYPES					\
	MODEL_COL_NUM_COLUMNS,					\
	B_TYPE_STRING,		  /* MODEL_COL_NAME */		\
	B_TYPE_INT64,		  /* MODEL_COL_SIZE */		\
	B_TYPE_LONG,		  /* MODEL_COL_MTIME */		\
	B_TYPE_FILE,		  /* MODEL_COL_FILE */		\
	B_TYPE_STRING,		  /* MODEL_COL_NAME_COLLATED */	\
	B_TYPE_BOOLEAN,		  /* MODEL_COL_IS_FOLDER */	\
	B_TYPE_BOOLEAN,		  /* MODEL_COL_IS_SENSITIVE */	\
	BDK_TYPE_PIXBUF,	  /* MODEL_COL_PIXBUF */	\
	B_TYPE_STRING,		  /* MODEL_COL_SIZE_TEXT */	\
	B_TYPE_STRING,		  /* MODEL_COL_MTIME_TEXT */	\
	BANGO_TYPE_ELLIPSIZE_MODE /* MODEL_COL_ELLIPSIZE */

/* Identifiers for target types */
enum {
  BTK_TREE_MODEL_ROW,
};

/* Interesting places in the shortcuts bar */
typedef enum {
  SHORTCUTS_SEARCH,
  SHORTCUTS_RECENT,
  SHORTCUTS_CWD,
  SHORTCUTS_RECENT_SEPARATOR,
  SHORTCUTS_HOME,
  SHORTCUTS_DESKTOP,
  SHORTCUTS_VOLUMES,
  SHORTCUTS_SHORTCUTS,
  SHORTCUTS_BOOKMARKS_SEPARATOR,
  SHORTCUTS_BOOKMARKS,
  SHORTCUTS_CURRENT_FOLDER_SEPARATOR,
  SHORTCUTS_CURRENT_FOLDER
} ShortcutsIndex;

/* Icon size for if we can't get it from the theme */
#define FALLBACK_ICON_SIZE 16

#define PREVIEW_HBOX_SPACING 12
#define NUM_LINES 45
#define NUM_CHARS 60

static void btk_file_chooser_default_iface_init       (BtkFileChooserIface        *iface);
static void btk_file_chooser_embed_default_iface_init (BtkFileChooserEmbedIface   *iface);

static BObject* btk_file_chooser_default_constructor  (GType                  type,
						       guint                  n_construct_properties,
						       BObjectConstructParam *construct_params);
static void     btk_file_chooser_default_finalize     (BObject               *object);
static void     btk_file_chooser_default_set_property (BObject               *object,
						       guint                  prop_id,
						       const BValue          *value,
						       BParamSpec            *pspec);
static void     btk_file_chooser_default_get_property (BObject               *object,
						       guint                  prop_id,
						       BValue                *value,
						       BParamSpec            *pspec);
static void     btk_file_chooser_default_dispose      (BObject               *object);
static void     btk_file_chooser_default_show_all       (BtkWidget             *widget);
static void     btk_file_chooser_default_realize        (BtkWidget             *widget);
static void     btk_file_chooser_default_map            (BtkWidget             *widget);
static void     btk_file_chooser_default_hierarchy_changed (BtkWidget          *widget,
							    BtkWidget          *previous_toplevel);
static void     btk_file_chooser_default_style_set      (BtkWidget             *widget,
							 BtkStyle              *previous_style);
static void     btk_file_chooser_default_screen_changed (BtkWidget             *widget,
							 BdkScreen             *previous_screen);

static gboolean       btk_file_chooser_default_set_current_folder 	   (BtkFileChooser    *chooser,
									    GFile             *folder,
									    GError           **error);
static gboolean       btk_file_chooser_default_update_current_folder 	   (BtkFileChooser    *chooser,
									    GFile             *folder,
									    gboolean           keep_trail,
									    gboolean           clear_entry,
									    GError           **error);
static GFile *        btk_file_chooser_default_get_current_folder 	   (BtkFileChooser    *chooser);
static void           btk_file_chooser_default_set_current_name   	   (BtkFileChooser    *chooser,
									    const gchar       *name);
static gboolean       btk_file_chooser_default_select_file        	   (BtkFileChooser    *chooser,
									    GFile             *file,
									    GError           **error);
static void           btk_file_chooser_default_unselect_file      	   (BtkFileChooser    *chooser,
									    GFile             *file);
static void           btk_file_chooser_default_select_all         	   (BtkFileChooser    *chooser);
static void           btk_file_chooser_default_unselect_all       	   (BtkFileChooser    *chooser);
static GSList *       btk_file_chooser_default_get_files          	   (BtkFileChooser    *chooser);
static GFile *        btk_file_chooser_default_get_preview_file   	   (BtkFileChooser    *chooser);
static BtkFileSystem *btk_file_chooser_default_get_file_system    	   (BtkFileChooser    *chooser);
static void           btk_file_chooser_default_add_filter         	   (BtkFileChooser    *chooser,
									    BtkFileFilter     *filter);
static void           btk_file_chooser_default_remove_filter      	   (BtkFileChooser    *chooser,
									    BtkFileFilter     *filter);
static GSList *       btk_file_chooser_default_list_filters       	   (BtkFileChooser    *chooser);
static gboolean       btk_file_chooser_default_add_shortcut_folder    (BtkFileChooser    *chooser,
								       GFile             *file,
								       GError           **error);
static gboolean       btk_file_chooser_default_remove_shortcut_folder (BtkFileChooser    *chooser,
								       GFile             *file,
								       GError           **error);
static GSList *       btk_file_chooser_default_list_shortcut_folders  (BtkFileChooser    *chooser);

static void           btk_file_chooser_default_get_default_size       (BtkFileChooserEmbed *chooser_embed,
								       gint                *default_width,
								       gint                *default_height);
static gboolean       btk_file_chooser_default_should_respond         (BtkFileChooserEmbed *chooser_embed);
static void           btk_file_chooser_default_initial_focus          (BtkFileChooserEmbed *chooser_embed);

static void add_selection_to_recent_list (BtkFileChooserDefault *impl);

static void location_popup_handler  (BtkFileChooserDefault *impl,
				     const gchar           *path);
static void location_popup_on_paste_handler (BtkFileChooserDefault *impl);
static void location_toggle_popup_handler   (BtkFileChooserDefault *impl);
static void up_folder_handler       (BtkFileChooserDefault *impl);
static void down_folder_handler     (BtkFileChooserDefault *impl);
static void home_folder_handler     (BtkFileChooserDefault *impl);
static void desktop_folder_handler  (BtkFileChooserDefault *impl);
static void quick_bookmark_handler  (BtkFileChooserDefault *impl,
				     gint                   bookmark_index);
static void show_hidden_handler     (BtkFileChooserDefault *impl);
static void search_shortcut_handler (BtkFileChooserDefault *impl);
static void recent_shortcut_handler (BtkFileChooserDefault *impl);
static void update_appearance       (BtkFileChooserDefault *impl);

static void set_current_filter   (BtkFileChooserDefault *impl,
				  BtkFileFilter         *filter);
static void check_preview_change (BtkFileChooserDefault *impl);

static void filter_combo_changed       (BtkComboBox           *combo_box,
					BtkFileChooserDefault *impl);

static gboolean shortcuts_key_press_event_cb (BtkWidget             *widget,
					      BdkEventKey           *event,
					      BtkFileChooserDefault *impl);

static gboolean shortcuts_select_func   (BtkTreeSelection      *selection,
					 BtkTreeModel          *model,
					 BtkTreePath           *path,
					 gboolean               path_currently_selected,
					 gpointer               data);
static gboolean shortcuts_get_selected  (BtkFileChooserDefault *impl,
					 BtkTreeIter           *iter);
static void shortcuts_activate_iter (BtkFileChooserDefault *impl,
				     BtkTreeIter           *iter);
static int shortcuts_get_index (BtkFileChooserDefault *impl,
				ShortcutsIndex         where);
static int shortcut_find_position (BtkFileChooserDefault *impl,
				   GFile                 *file);

static void bookmarks_check_add_sensitivity (BtkFileChooserDefault *impl);

static gboolean list_select_func   (BtkTreeSelection      *selection,
				    BtkTreeModel          *model,
				    BtkTreePath           *path,
				    gboolean               path_currently_selected,
				    gpointer               data);

static void list_selection_changed     (BtkTreeSelection      *tree_selection,
					BtkFileChooserDefault *impl);
static void list_row_activated         (BtkTreeView           *tree_view,
					BtkTreePath           *path,
					BtkTreeViewColumn     *column,
					BtkFileChooserDefault *impl);

static void path_bar_clicked (BtkPathBar            *path_bar,
			      GFile                 *file,
			      GFile                 *child,
                              gboolean               child_is_hidden,
                              BtkFileChooserDefault *impl);

static void add_bookmark_button_clicked_cb    (BtkButton             *button,
					       BtkFileChooserDefault *impl);
static void remove_bookmark_button_clicked_cb (BtkButton             *button,
					       BtkFileChooserDefault *impl);

static void update_cell_renderer_attributes (BtkFileChooserDefault *impl);

static void load_remove_timer (BtkFileChooserDefault *impl, LoadState new_load_state);
static void browse_files_center_selected_row (BtkFileChooserDefault *impl);

static void location_button_toggled_cb (BtkToggleButton *toggle,
					BtkFileChooserDefault *impl);
static void location_switch_to_path_bar (BtkFileChooserDefault *impl);

static void stop_loading_and_clear_list_model (BtkFileChooserDefault *impl,
                                               gboolean remove_from_treeview);

static void     search_setup_widgets         (BtkFileChooserDefault *impl);
static void     search_stop_searching        (BtkFileChooserDefault *impl,
                                              gboolean               remove_query);
static void     search_clear_model           (BtkFileChooserDefault *impl, 
					      gboolean               remove_from_treeview);
static gboolean search_should_respond        (BtkFileChooserDefault *impl);
static GSList  *search_get_selected_files    (BtkFileChooserDefault *impl);
static void     search_entry_activate_cb     (BtkEntry              *entry, 
					      gpointer               data);
static void     settings_load                (BtkFileChooserDefault *impl);
static void     settings_save                (BtkFileChooserDefault *impl);

static void     recent_start_loading         (BtkFileChooserDefault *impl);
static void     recent_stop_loading          (BtkFileChooserDefault *impl);
static void     recent_clear_model           (BtkFileChooserDefault *impl,
                                              gboolean               remove_from_treeview);
static gboolean recent_should_respond        (BtkFileChooserDefault *impl);
static GSList * recent_get_selected_files    (BtkFileChooserDefault *impl);
static void     set_file_system_backend      (BtkFileChooserDefault *impl);
static void     unset_file_system_backend    (BtkFileChooserDefault *impl);





/* Drag and drop interface declarations */

typedef struct {
  BtkTreeModelFilter parent;

  BtkFileChooserDefault *impl;
} ShortcutsPaneModelFilter;

typedef struct {
  BtkTreeModelFilterClass parent_class;
} ShortcutsPaneModelFilterClass;

#define SHORTCUTS_PANE_MODEL_FILTER_TYPE (_shortcuts_pane_model_filter_get_type ())
#define SHORTCUTS_PANE_MODEL_FILTER(obj) (B_TYPE_CHECK_INSTANCE_CAST ((obj), SHORTCUTS_PANE_MODEL_FILTER_TYPE, ShortcutsPaneModelFilter))

static void shortcuts_pane_model_filter_drag_source_iface_init (BtkTreeDragSourceIface *iface);

G_DEFINE_TYPE_WITH_CODE (ShortcutsPaneModelFilter,
			 _shortcuts_pane_model_filter,
			 BTK_TYPE_TREE_MODEL_FILTER,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_TREE_DRAG_SOURCE,
						shortcuts_pane_model_filter_drag_source_iface_init))

static BtkTreeModel *shortcuts_pane_model_filter_new (BtkFileChooserDefault *impl,
						      BtkTreeModel          *child_model,
						      BtkTreePath           *root);



G_DEFINE_TYPE_WITH_CODE (BtkFileChooserDefault, _btk_file_chooser_default, BTK_TYPE_VBOX,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_FILE_CHOOSER,
						btk_file_chooser_default_iface_init)
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_FILE_CHOOSER_EMBED,
						btk_file_chooser_embed_default_iface_init));						


static void
add_normal_and_shifted_binding (BtkBindingSet  *binding_set,
				guint           keyval,
				BdkModifierType modifiers,
				const gchar    *signal_name)
{
  btk_binding_entry_add_signal (binding_set,
				keyval, modifiers,
				signal_name, 0);

  btk_binding_entry_add_signal (binding_set,
				keyval, modifiers | BDK_SHIFT_MASK,
				signal_name, 0);
}

static void
_btk_file_chooser_default_class_init (BtkFileChooserDefaultClass *class)
{
  static const guint quick_bookmark_keyvals[10] = {
    BDK_KEY_1, BDK_KEY_2, BDK_KEY_3, BDK_KEY_4, BDK_KEY_5, BDK_KEY_6, BDK_KEY_7, BDK_KEY_8, BDK_KEY_9, BDK_KEY_0
  };
  BObjectClass *bobject_class = B_OBJECT_CLASS (class);
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (class);
  BtkBindingSet *binding_set;
  int i;

  bobject_class->finalize = btk_file_chooser_default_finalize;
  bobject_class->constructor = btk_file_chooser_default_constructor;
  bobject_class->set_property = btk_file_chooser_default_set_property;
  bobject_class->get_property = btk_file_chooser_default_get_property;
  bobject_class->dispose = btk_file_chooser_default_dispose;

  widget_class->show_all = btk_file_chooser_default_show_all;
  widget_class->realize = btk_file_chooser_default_realize;
  widget_class->map = btk_file_chooser_default_map;
  widget_class->hierarchy_changed = btk_file_chooser_default_hierarchy_changed;
  widget_class->style_set = btk_file_chooser_default_style_set;
  widget_class->screen_changed = btk_file_chooser_default_screen_changed;

  signals[LOCATION_POPUP] =
    g_signal_new_class_handler (I_("location-popup"),
                                B_OBJECT_CLASS_TYPE (class),
                                G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                G_CALLBACK (location_popup_handler),
                                NULL, NULL,
                                _btk_marshal_VOID__STRING,
                                B_TYPE_NONE, 1, B_TYPE_STRING);

  signals[LOCATION_POPUP_ON_PASTE] =
    g_signal_new_class_handler (I_("location-popup-on-paste"),
                                B_OBJECT_CLASS_TYPE (class),
                                G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                G_CALLBACK (location_popup_on_paste_handler),
                                NULL, NULL,
                                _btk_marshal_VOID__VOID,
                                B_TYPE_NONE, 0);

  signals[LOCATION_TOGGLE_POPUP] =
    g_signal_new_class_handler (I_("location-toggle-popup"),
                                B_OBJECT_CLASS_TYPE (class),
                                G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                G_CALLBACK (location_toggle_popup_handler),
                                NULL, NULL,
                                _btk_marshal_VOID__VOID,
                                B_TYPE_NONE, 0);

  signals[UP_FOLDER] =
    g_signal_new_class_handler (I_("up-folder"),
                                B_OBJECT_CLASS_TYPE (class),
                                G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                G_CALLBACK (up_folder_handler),
                                NULL, NULL,
                                _btk_marshal_VOID__VOID,
                                B_TYPE_NONE, 0);

  signals[DOWN_FOLDER] =
    g_signal_new_class_handler (I_("down-folder"),
                                B_OBJECT_CLASS_TYPE (class),
                                G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                G_CALLBACK (down_folder_handler),
                                NULL, NULL,
                                _btk_marshal_VOID__VOID,
                                B_TYPE_NONE, 0);

  signals[HOME_FOLDER] =
    g_signal_new_class_handler (I_("home-folder"),
                                B_OBJECT_CLASS_TYPE (class),
                                G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                G_CALLBACK (home_folder_handler),
                                NULL, NULL,
                                _btk_marshal_VOID__VOID,
                                B_TYPE_NONE, 0);

  signals[DESKTOP_FOLDER] =
    g_signal_new_class_handler (I_("desktop-folder"),
                                B_OBJECT_CLASS_TYPE (class),
                                G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                G_CALLBACK (desktop_folder_handler),
                                NULL, NULL,
                                _btk_marshal_VOID__VOID,
                                B_TYPE_NONE, 0);

  signals[QUICK_BOOKMARK] =
    g_signal_new_class_handler (I_("quick-bookmark"),
                                B_OBJECT_CLASS_TYPE (class),
                                G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                G_CALLBACK (quick_bookmark_handler),
                                NULL, NULL,
                                _btk_marshal_VOID__INT,
                                B_TYPE_NONE, 1, B_TYPE_INT);

  signals[SHOW_HIDDEN] =
    g_signal_new_class_handler (I_("show-hidden"),
                                B_OBJECT_CLASS_TYPE (class),
                                G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                G_CALLBACK (show_hidden_handler),
                                NULL, NULL,
                                _btk_marshal_VOID__VOID,
                                B_TYPE_NONE, 0);

  signals[SEARCH_SHORTCUT] =
    g_signal_new_class_handler (I_("search-shortcut"),
                                B_OBJECT_CLASS_TYPE (class),
                                G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                G_CALLBACK (search_shortcut_handler),
                                NULL, NULL,
                                _btk_marshal_VOID__VOID,
                                B_TYPE_NONE, 0);

  signals[RECENT_SHORTCUT] =
    g_signal_new_class_handler (I_("recent-shortcut"),
                                B_OBJECT_CLASS_TYPE (class),
                                G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                G_CALLBACK (recent_shortcut_handler),
                                NULL, NULL,
                                _btk_marshal_VOID__VOID,
                                B_TYPE_NONE, 0);

  binding_set = btk_binding_set_by_class (class);

  btk_binding_entry_add_signal (binding_set,
				BDK_KEY_l, BDK_CONTROL_MASK,
				"location-toggle-popup",
				0);

  btk_binding_entry_add_signal (binding_set,
				BDK_KEY_slash, 0,
				"location-popup",
				1, B_TYPE_STRING, "/");
  btk_binding_entry_add_signal (binding_set,
				BDK_KEY_KP_Divide, 0,
				"location-popup",
				1, B_TYPE_STRING, "/");

#ifdef G_OS_UNIX
  btk_binding_entry_add_signal (binding_set,
				BDK_KEY_asciitilde, 0,
				"location-popup",
				1, B_TYPE_STRING, "~");
#endif

  btk_binding_entry_add_signal (binding_set,
				BDK_KEY_v, BDK_CONTROL_MASK,
				"location-popup-on-paste",
				0);
  btk_binding_entry_add_signal (binding_set,
		  		BDK_KEY_BackSpace, 0,
				"up-folder",
				0);

  add_normal_and_shifted_binding (binding_set,
				  BDK_KEY_Up, BDK_MOD1_MASK,
				  "up-folder");

  add_normal_and_shifted_binding (binding_set,
				  BDK_KEY_KP_Up, BDK_MOD1_MASK,
				  "up-folder");

  add_normal_and_shifted_binding (binding_set,
				  BDK_KEY_Down, BDK_MOD1_MASK,
				  "down-folder");
  add_normal_and_shifted_binding (binding_set,
				  BDK_KEY_KP_Down, BDK_MOD1_MASK,
				  "down-folder");

  btk_binding_entry_add_signal (binding_set,
				BDK_KEY_Home, BDK_MOD1_MASK,
				"home-folder",
				0);
  btk_binding_entry_add_signal (binding_set,
				BDK_KEY_KP_Home, BDK_MOD1_MASK,
				"home-folder",
				0);
  btk_binding_entry_add_signal (binding_set,
				BDK_KEY_d, BDK_MOD1_MASK,
				"desktop-folder",
				0);
  btk_binding_entry_add_signal (binding_set,
				BDK_KEY_h, BDK_CONTROL_MASK,
                                "show-hidden",
                                0);
  btk_binding_entry_add_signal (binding_set,
                                BDK_KEY_s, BDK_MOD1_MASK,
                                "search-shortcut",
                                0);
  btk_binding_entry_add_signal (binding_set,
                                BDK_KEY_r, BDK_MOD1_MASK,
                                "recent-shortcut",
                                0);

  for (i = 0; i < 10; i++)
    btk_binding_entry_add_signal (binding_set,
				  quick_bookmark_keyvals[i], BDK_MOD1_MASK,
				  "quick-bookmark",
				  1, B_TYPE_INT, i);

  _btk_file_chooser_install_properties (bobject_class);
}

static void
btk_file_chooser_default_iface_init (BtkFileChooserIface *iface)
{
  iface->select_file = btk_file_chooser_default_select_file;
  iface->unselect_file = btk_file_chooser_default_unselect_file;
  iface->select_all = btk_file_chooser_default_select_all;
  iface->unselect_all = btk_file_chooser_default_unselect_all;
  iface->get_files = btk_file_chooser_default_get_files;
  iface->get_preview_file = btk_file_chooser_default_get_preview_file;
  iface->get_file_system = btk_file_chooser_default_get_file_system;
  iface->set_current_folder = btk_file_chooser_default_set_current_folder;
  iface->get_current_folder = btk_file_chooser_default_get_current_folder;
  iface->set_current_name = btk_file_chooser_default_set_current_name;
  iface->add_filter = btk_file_chooser_default_add_filter;
  iface->remove_filter = btk_file_chooser_default_remove_filter;
  iface->list_filters = btk_file_chooser_default_list_filters;
  iface->add_shortcut_folder = btk_file_chooser_default_add_shortcut_folder;
  iface->remove_shortcut_folder = btk_file_chooser_default_remove_shortcut_folder;
  iface->list_shortcut_folders = btk_file_chooser_default_list_shortcut_folders;
}

static void
btk_file_chooser_embed_default_iface_init (BtkFileChooserEmbedIface *iface)
{
  iface->get_default_size = btk_file_chooser_default_get_default_size;
  iface->should_respond = btk_file_chooser_default_should_respond;
  iface->initial_focus = btk_file_chooser_default_initial_focus;
}

static void
_btk_file_chooser_default_init (BtkFileChooserDefault *impl)
{
  profile_start ("start", NULL);
#ifdef PROFILE_FILE_CHOOSER
  access ("MARK: *** CREATE FILE CHOOSER", F_OK);
#endif
  impl->local_only = TRUE;
  impl->preview_widget_active = TRUE;
  impl->use_preview_label = TRUE;
  impl->select_multiple = FALSE;
  impl->show_hidden = FALSE;
  impl->show_size_column = TRUE;
  impl->icon_size = FALLBACK_ICON_SIZE;
  impl->load_state = LOAD_EMPTY;
  impl->reload_state = RELOAD_EMPTY;
  impl->pending_select_files = NULL;
  impl->location_mode = LOCATION_MODE_PATH_BAR;
  impl->operation_mode = OPERATION_MODE_BROWSE;
  impl->sort_column = MODEL_COL_NAME;
  impl->sort_order = BTK_SORT_ASCENDING;
  impl->recent_manager = btk_recent_manager_get_default ();
  impl->create_folders = TRUE;

  btk_box_set_spacing (BTK_BOX (impl), 12);

  set_file_system_backend (impl);

  profile_end ("end", NULL);
}

/* Frees the data columns for the specified iter in the shortcuts model*/
static void
shortcuts_free_row_data (BtkFileChooserDefault *impl,
			 BtkTreeIter           *iter)
{
  gpointer col_data;
  ShortcutType shortcut_type;
  GCancellable *cancellable;

  btk_tree_model_get (BTK_TREE_MODEL (impl->shortcuts_model), iter,
		      SHORTCUTS_COL_DATA, &col_data,
		      SHORTCUTS_COL_TYPE, &shortcut_type,
		      SHORTCUTS_COL_CANCELLABLE, &cancellable,
		      -1);

  if (cancellable)
    g_cancellable_cancel (cancellable);

  if (!(shortcut_type == SHORTCUT_TYPE_FILE || 
	shortcut_type == SHORTCUT_TYPE_VOLUME) ||
      !col_data)
    return;

  if (shortcut_type == SHORTCUT_TYPE_VOLUME)
    {
      BtkFileSystemVolume *volume;

      volume = col_data;
      _btk_file_system_volume_unref (volume);
    }
  if (shortcut_type == SHORTCUT_TYPE_FILE)
    {
      GFile *file;

      file = col_data;
      g_object_unref (file);
    }
}

/* Frees all the data columns in the shortcuts model */
static void
shortcuts_free (BtkFileChooserDefault *impl)
{
  BtkTreeIter iter;

  if (!impl->shortcuts_model)
    return;

  if (btk_tree_model_get_iter_first (BTK_TREE_MODEL (impl->shortcuts_model), &iter))
    do
      {
	shortcuts_free_row_data (impl, &iter);
      }
    while (btk_tree_model_iter_next (BTK_TREE_MODEL (impl->shortcuts_model), &iter));

  g_object_unref (impl->shortcuts_model);
  impl->shortcuts_model = NULL;
}

static void
pending_select_files_free (BtkFileChooserDefault *impl)
{
  b_slist_foreach (impl->pending_select_files, (GFunc) g_object_unref, NULL);
  b_slist_free (impl->pending_select_files);
  impl->pending_select_files = NULL;
}

static void
pending_select_files_add (BtkFileChooserDefault *impl,
			  GFile                 *file)
{
  impl->pending_select_files =
    b_slist_prepend (impl->pending_select_files, g_object_ref (file));
}

static void
btk_file_chooser_default_finalize (BObject *object)
{
  BtkFileChooserDefault *impl = BTK_FILE_CHOOSER_DEFAULT (object);
  GSList *l;

  unset_file_system_backend (impl);

  if (impl->shortcuts_pane_filter_model)
    g_object_unref (impl->shortcuts_pane_filter_model);

  shortcuts_free (impl);

  g_free (impl->browse_files_last_selected_name);

  for (l = impl->filters; l; l = l->next)
    {
      BtkFileFilter *filter;

      filter = BTK_FILE_FILTER (l->data);
      g_object_unref (filter);
    }
  b_slist_free (impl->filters);

  if (impl->current_filter)
    g_object_unref (impl->current_filter);

  if (impl->current_volume_file)
    g_object_unref (impl->current_volume_file);

  if (impl->current_folder)
    g_object_unref (impl->current_folder);

  if (impl->preview_file)
    g_object_unref (impl->preview_file);

  if (impl->browse_path_bar_size_group)
    g_object_unref (impl->browse_path_bar_size_group);

  /* Free all the Models we have */
  stop_loading_and_clear_list_model (impl, FALSE);
  search_clear_model (impl, FALSE);
  recent_clear_model (impl, FALSE);

  /* stopping the load above should have cleared this */
  g_assert (impl->load_timeout_id == 0);

  g_free (impl->preview_display_name);

  g_free (impl->edited_new_text);

  B_OBJECT_CLASS (_btk_file_chooser_default_parent_class)->finalize (object);
}

/* Shows an error dialog set as transient for the specified window */
static void
error_message_with_parent (BtkWindow  *parent,
			   const char *msg,
			   const char *detail)
{
  BtkWidget *dialog;

  dialog = btk_message_dialog_new (parent,
				   BTK_DIALOG_MODAL | BTK_DIALOG_DESTROY_WITH_PARENT,
				   BTK_MESSAGE_ERROR,
				   BTK_BUTTONS_OK,
				   "%s",
				   msg);
  btk_message_dialog_format_secondary_text (BTK_MESSAGE_DIALOG (dialog),
					    "%s", detail);

  if (parent && btk_window_has_group (parent))
    btk_window_group_add_window (btk_window_get_group (parent),
                                 BTK_WINDOW (dialog));

  btk_dialog_run (BTK_DIALOG (dialog));
  btk_widget_destroy (dialog);
}

/* Returns a toplevel BtkWindow, or NULL if none */
static BtkWindow *
get_toplevel (BtkWidget *widget)
{
  BtkWidget *toplevel;

  toplevel = btk_widget_get_toplevel (widget);
  if (!btk_widget_is_toplevel (toplevel))
    return NULL;
  else
    return BTK_WINDOW (toplevel);
}

/* Shows an error dialog for the file chooser */
static void
error_message (BtkFileChooserDefault *impl,
	       const char            *msg,
	       const char            *detail)
{
  error_message_with_parent (get_toplevel (BTK_WIDGET (impl)), msg, detail);
}

/* Shows a simple error dialog relative to a path.  Frees the GError as well. */
static void
error_dialog (BtkFileChooserDefault *impl,
	      const char            *msg,
	      GFile                 *file,
	      GError                *error)
{
  if (error)
    {
      char *uri = NULL;
      char *text;

      if (file)
	uri = g_file_get_uri (file);
      text = g_strdup_printf (msg, uri);
      error_message (impl, text, error->message);
      g_free (text);
      g_free (uri);
      g_error_free (error);
    }
}

/* Displays an error message about not being able to get information for a file.
 * Frees the GError as well.
 */
static void
error_getting_info_dialog (BtkFileChooserDefault *impl,
			   GFile                 *file,
			   GError                *error)
{
  error_dialog (impl,
		_("Could not retrieve information about the file"),
		file, error);
}

/* Shows an error dialog about not being able to add a bookmark */
static void
error_adding_bookmark_dialog (BtkFileChooserDefault *impl,
			      GFile                 *file,
			      GError                *error)
{
  error_dialog (impl,
		_("Could not add a bookmark"),
		file, error);
}

/* Shows an error dialog about not being able to remove a bookmark */
static void
error_removing_bookmark_dialog (BtkFileChooserDefault *impl,
				GFile                 *file,
				GError                *error)
{
  error_dialog (impl,
		_("Could not remove bookmark"),
		file, error);
}

/* Shows an error dialog about not being able to create a folder */
static void
error_creating_folder_dialog (BtkFileChooserDefault *impl,
			      GFile                 *file,
			      GError                *error)
{
  error_dialog (impl, 
		_("The folder could not be created"), 
		file, error);
}

/* Shows an error about not being able to create a folder because a file with
 * the same name is already there.
 */
static void
error_creating_folder_over_existing_file_dialog (BtkFileChooserDefault *impl,
						 GFile                 *file,
						 GError                *error)
{
  error_dialog (impl,
		_("The folder could not be created, as a file with the same "
                  "name already exists.  Try using a different name for the "
                  "folder, or rename the file first."),
		file, error);
}

static void
error_with_file_under_nonfolder (BtkFileChooserDefault *impl,
				 GFile *parent_file)
{
  GError *error;

  error = NULL;
  g_set_error_literal (&error, G_IO_ERROR, G_IO_ERROR_NOT_DIRECTORY,
		       _("You need to choose a valid filename."));

  error_dialog (impl,
		_("Cannot create a file under %s as it is not a folder"),
		parent_file, error);
}

/* Shows an error about not being able to select a folder because a file with
 * the same name is already there.
 */
static void
error_selecting_folder_over_existing_file_dialog (BtkFileChooserDefault *impl,
						  GFile                 *file)
{
  error_dialog (impl,
		_("You may only select folders.  The item that you selected is not a folder; "
                  "try using a different item."),
		file, NULL);
}

/* Shows an error dialog about not being able to create a filename */
static void
error_building_filename_dialog (BtkFileChooserDefault *impl,
				GError                *error)
{
  error_dialog (impl, _("Invalid file name"), 
		NULL, error);
}

/* Shows an error dialog when we cannot switch to a folder */
static void
error_changing_folder_dialog (BtkFileChooserDefault *impl,
			      GFile                 *file,
			      GError                *error)
{
  error_dialog (impl, _("The folder contents could not be displayed"),
		file, error);
}

/* Changes folders, displaying an error dialog if this fails */
static gboolean
change_folder_and_display_error (BtkFileChooserDefault *impl,
				 GFile                 *file,
				 gboolean               clear_entry)
{
  GError *error;
  gboolean result;

  g_return_val_if_fail (G_IS_FILE (file), FALSE);

  /* We copy the path because of this case:
   *
   * list_row_activated()
   *   fetches path from model; path belongs to the model (*)
   *   calls change_folder_and_display_error()
   *     calls btk_file_chooser_set_current_folder_file()
   *       changing folders fails, sets model to NULL, thus freeing the path in (*)
   */

  error = NULL;
  result = btk_file_chooser_default_update_current_folder (BTK_FILE_CHOOSER (impl), file, TRUE, clear_entry, &error);

  if (!result)
    error_changing_folder_dialog (impl, file, error);

  return result;
}

static void
emit_default_size_changed (BtkFileChooserDefault *impl)
{
  profile_msg ("    emit default-size-changed start", NULL);
  g_signal_emit_by_name (impl, "default-size-changed");
  profile_msg ("    emit default-size-changed end", NULL);
}

static void
update_preview_widget_visibility (BtkFileChooserDefault *impl)
{
  if (impl->use_preview_label)
    {
      if (!impl->preview_label)
	{
	  impl->preview_label = btk_label_new (impl->preview_display_name);
	  btk_box_pack_start (BTK_BOX (impl->preview_box), impl->preview_label, FALSE, FALSE, 0);
	  btk_box_reorder_child (BTK_BOX (impl->preview_box), impl->preview_label, 0);
	  btk_label_set_ellipsize (BTK_LABEL (impl->preview_label), BANGO_ELLIPSIZE_MIDDLE);
	  btk_widget_show (impl->preview_label);
	}
    }
  else
    {
      if (impl->preview_label)
	{
	  btk_widget_destroy (impl->preview_label);
	  impl->preview_label = NULL;
	}
    }

  if (impl->preview_widget_active && impl->preview_widget)
    btk_widget_show (impl->preview_box);
  else
    btk_widget_hide (impl->preview_box);

  if (!btk_widget_get_mapped (BTK_WIDGET (impl)))
    emit_default_size_changed (impl);
}

static void
set_preview_widget (BtkFileChooserDefault *impl,
		    BtkWidget             *preview_widget)
{
  if (preview_widget == impl->preview_widget)
    return;

  if (impl->preview_widget)
    btk_container_remove (BTK_CONTAINER (impl->preview_box),
			  impl->preview_widget);

  impl->preview_widget = preview_widget;
  if (impl->preview_widget)
    {
      btk_widget_show (impl->preview_widget);
      btk_box_pack_start (BTK_BOX (impl->preview_box), impl->preview_widget, TRUE, TRUE, 0);
      btk_box_reorder_child (BTK_BOX (impl->preview_box),
			     impl->preview_widget,
			     (impl->use_preview_label && impl->preview_label) ? 1 : 0);
    }

  update_preview_widget_visibility (impl);
}

/* Renders a "Search" icon at an appropriate size for a tree view */
static BdkPixbuf *
render_search_icon (BtkFileChooserDefault *impl)
{
  return btk_widget_render_icon (BTK_WIDGET (impl), BTK_STOCK_FIND, BTK_ICON_SIZE_MENU, NULL);
}

static BdkPixbuf *
render_recent_icon (BtkFileChooserDefault *impl)
{
  BtkIconTheme *theme;
  BdkPixbuf *retval;

  if (btk_widget_has_screen (BTK_WIDGET (impl)))
    theme = btk_icon_theme_get_for_screen (btk_widget_get_screen (BTK_WIDGET (impl)));
  else
    theme = btk_icon_theme_get_default ();

  retval = btk_icon_theme_load_icon (theme, "document-open-recent",
                                     impl->icon_size, 0,
                                     NULL);

  /* fallback */
  if (!retval)
    retval = btk_widget_render_icon (BTK_WIDGET (impl), BTK_STOCK_FILE, BTK_ICON_SIZE_MENU, NULL);

  return retval;
}


/* Re-reads all the icons for the shortcuts, used when the theme changes */
struct ReloadIconsData
{
  BtkFileChooserDefault *impl;
  BtkTreeRowReference *row_ref;
};

static void
shortcuts_reload_icons_get_info_cb (GCancellable *cancellable,
				    GFileInfo    *info,
				    const GError *error,
				    gpointer      user_data)
{
  BdkPixbuf *pixbuf;
  BtkTreeIter iter;
  BtkTreePath *path;
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  struct ReloadIconsData *data = user_data;

  if (!b_slist_find (data->impl->reload_icon_cancellables, cancellable))
    goto out;

  data->impl->reload_icon_cancellables = b_slist_remove (data->impl->reload_icon_cancellables, cancellable);

  if (cancelled || error)
    goto out;

  pixbuf = _btk_file_info_render_icon (info, BTK_WIDGET (data->impl), data->impl->icon_size);

  path = btk_tree_row_reference_get_path (data->row_ref);
  if (path)
    {
      btk_tree_model_get_iter (BTK_TREE_MODEL (data->impl->shortcuts_model), &iter, path);
      btk_list_store_set (data->impl->shortcuts_model, &iter,
			  SHORTCUTS_COL_PIXBUF, pixbuf,
			  -1);
      btk_tree_path_free (path);
    }

  if (pixbuf)
    g_object_unref (pixbuf);

out:
  btk_tree_row_reference_free (data->row_ref);
  g_object_unref (data->impl);
  g_free (data);

  g_object_unref (cancellable);
}

static void
shortcuts_reload_icons (BtkFileChooserDefault *impl)
{
  GSList *l;
  BtkTreeIter iter;

  profile_start ("start", NULL);

  if (!btk_tree_model_get_iter_first (BTK_TREE_MODEL (impl->shortcuts_model), &iter))
    goto out;

  for (l = impl->reload_icon_cancellables; l; l = l->next)
    {
      GCancellable *cancellable = G_CANCELLABLE (l->data);
      g_cancellable_cancel (cancellable);
    }
  b_slist_free (impl->reload_icon_cancellables);
  impl->reload_icon_cancellables = NULL;

  do
    {
      gpointer data;
      ShortcutType shortcut_type;
      gboolean pixbuf_visible;
      BdkPixbuf *pixbuf;

      btk_tree_model_get (BTK_TREE_MODEL (impl->shortcuts_model), &iter,
			  SHORTCUTS_COL_DATA, &data,
			  SHORTCUTS_COL_TYPE, &shortcut_type,
			  SHORTCUTS_COL_PIXBUF_VISIBLE, &pixbuf_visible,
			  -1);

      pixbuf = NULL;
      if (pixbuf_visible)
        {
	  if (shortcut_type == SHORTCUT_TYPE_VOLUME)
	    {
	      BtkFileSystemVolume *volume;

	      volume = data;
	      pixbuf = _btk_file_system_volume_render_icon (volume, BTK_WIDGET (impl),
						 	    impl->icon_size, NULL);
	    }
	  else if (shortcut_type == SHORTCUT_TYPE_FILE)
            {
	      if (g_file_is_native (G_FILE (data)))
	        {
		  GFile *file;
	          struct ReloadIconsData *info;
	          BtkTreePath *tree_path;
		  GCancellable *cancellable;

	          file = data;

	          info = g_new0 (struct ReloadIconsData, 1);
	          info->impl = g_object_ref (impl);
	          tree_path = btk_tree_model_get_path (BTK_TREE_MODEL (impl->shortcuts_model), &iter);
	          info->row_ref = btk_tree_row_reference_new (BTK_TREE_MODEL (impl->shortcuts_model), tree_path);
	          btk_tree_path_free (tree_path);

	          cancellable = _btk_file_system_get_info (impl->file_system, file,
							   "standard::icon",
							   shortcuts_reload_icons_get_info_cb,
							   info);
	          impl->reload_icon_cancellables = b_slist_append (impl->reload_icon_cancellables, cancellable);
	        }
              else
	        {
	          BtkIconTheme *icon_theme;

	          /* Don't call get_info for remote paths to avoid latency and
	           * auth dialogs.
	           * If we switch to a better bookmarks file format (XBEL), we
	           * should use mime info to get a better icon.
	           */
	          icon_theme = btk_icon_theme_get_for_screen (btk_widget_get_screen (BTK_WIDGET (impl)));
	          pixbuf = btk_icon_theme_load_icon (icon_theme, "folder-remote", 
						     impl->icon_size, 0, NULL);
	        }
            }
	  else if (shortcut_type == SHORTCUT_TYPE_SEARCH)
	    {
	      pixbuf = render_search_icon (impl);
            }
	  else if (shortcut_type == SHORTCUT_TYPE_RECENT)
            {
              pixbuf = render_recent_icon (impl);
            }

          btk_list_store_set (impl->shortcuts_model, &iter,
  	   	              SHORTCUTS_COL_PIXBUF, pixbuf,
	                      -1);

          if (pixbuf)
            g_object_unref (pixbuf);

	}
    }
  while (btk_tree_model_iter_next (BTK_TREE_MODEL (impl->shortcuts_model),&iter));

 out:

  profile_end ("end", NULL);
}

static void 
shortcuts_find_folder (BtkFileChooserDefault *impl,
		       GFile                 *folder)
{
  BtkTreeSelection *selection;
  int pos;
  BtkTreePath *path;

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_shortcuts_tree_view));

  g_assert (folder != NULL);
  pos = shortcut_find_position (impl, folder);
  if (pos == -1)
    {
      btk_tree_selection_unselect_all (selection);
      return;
    }

  path = btk_tree_path_new_from_indices (pos, -1);
  btk_tree_selection_select_path (selection, path);
  btk_tree_path_free (path);
}

/* If a shortcut corresponds to the current folder, selects it */
static void
shortcuts_find_current_folder (BtkFileChooserDefault *impl)
{
  shortcuts_find_folder (impl, impl->current_folder);
}

/* Removes the specified number of rows from the shortcuts list */
static void
shortcuts_remove_rows (BtkFileChooserDefault *impl,
		       int                    start_row,
		       int                    n_rows)
{
  BtkTreePath *path;

  path = btk_tree_path_new_from_indices (start_row, -1);

  for (; n_rows; n_rows--)
    {
      BtkTreeIter iter;

      if (!btk_tree_model_get_iter (BTK_TREE_MODEL (impl->shortcuts_model), &iter, path))
	g_assert_not_reached ();

      shortcuts_free_row_data (impl, &iter);
      btk_list_store_remove (impl->shortcuts_model, &iter);
    }

  btk_tree_path_free (path);
}

static void
shortcuts_update_count (BtkFileChooserDefault *impl,
			ShortcutsIndex         type,
			gint                   value)
{
  switch (type)
    {
      case SHORTCUTS_CWD:
        if (value < 0)
	  impl->has_cwd = FALSE;
	else
	  impl->has_cwd = TRUE;
	break;
	
      case SHORTCUTS_HOME:
	if (value < 0)
	  impl->has_home = FALSE;
	else
	  impl->has_home = TRUE;
	break;

      case SHORTCUTS_DESKTOP:
	if (value < 0)
	  impl->has_desktop = FALSE;
	else
	  impl->has_desktop = TRUE;
	break;

      case SHORTCUTS_VOLUMES:
	impl->num_volumes += value;
	break;

      case SHORTCUTS_SHORTCUTS:
	impl->num_shortcuts += value;
	break;

      case SHORTCUTS_BOOKMARKS:
	impl->num_bookmarks += value;
	break;

      case SHORTCUTS_CURRENT_FOLDER:
	if (value < 0)
	  impl->shortcuts_current_folder_active = FALSE;
	else
	  impl->shortcuts_current_folder_active = TRUE;
	break;

      default:
	/* nothing */
	break;
    }
}

struct ShortcutsInsertRequest
{
  BtkFileChooserDefault *impl;
  GFile *file;
  int pos;
  char *label_copy;
  BtkTreeRowReference *row_ref;
  ShortcutsIndex type;
  gboolean name_only;
  gboolean removable;
};

static void
get_file_info_finished (GCancellable *cancellable,
			GFileInfo    *info,
			const GError *error,
			gpointer      data)
{
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  BdkPixbuf *pixbuf;
  BtkTreePath *path;
  BtkTreeIter iter;
  GCancellable *model_cancellable;
  struct ShortcutsInsertRequest *request = data;

  path = btk_tree_row_reference_get_path (request->row_ref);
  if (!path)
    /* Handle doesn't exist anymore in the model */
    goto out;

  btk_tree_model_get_iter (BTK_TREE_MODEL (request->impl->shortcuts_model),
			   &iter, path);
  btk_tree_path_free (path);

  /* validate cancellable, else goto out */
  btk_tree_model_get (BTK_TREE_MODEL (request->impl->shortcuts_model), &iter,
		      SHORTCUTS_COL_CANCELLABLE, &model_cancellable,
		      -1);
  if (cancellable != model_cancellable)
    goto out;

  /* set the cancellable to NULL in the model (we unref later on) */
  btk_list_store_set (request->impl->shortcuts_model, &iter,
		      SHORTCUTS_COL_CANCELLABLE, NULL,
		      -1);

  if (cancelled)
    goto out;

  if (!info)
    {
      shortcuts_free_row_data (request->impl, &iter);
      btk_list_store_remove (request->impl->shortcuts_model, &iter);
      shortcuts_update_count (request->impl, request->type, -1);

      if (request->type == SHORTCUTS_HOME)
        {
	  GFile *home;

	  home = g_file_new_for_path (g_get_home_dir ());
	  error_getting_info_dialog (request->impl, home, g_error_copy (error));
	  g_object_unref (home);
	}
      else if (request->type == SHORTCUTS_CURRENT_FOLDER)
        {
	  /* Remove the current folder separator */
	  gint separator_pos = shortcuts_get_index (request->impl, SHORTCUTS_CURRENT_FOLDER_SEPARATOR);
	  shortcuts_remove_rows (request->impl, separator_pos, 1);
        }

      goto out;
    }
  
  if (!request->label_copy)
    request->label_copy = g_strdup (g_file_info_get_display_name (info));
  pixbuf = _btk_file_info_render_icon (info, BTK_WIDGET (request->impl),
				       request->impl->icon_size);

  btk_list_store_set (request->impl->shortcuts_model, &iter,
		      SHORTCUTS_COL_PIXBUF, pixbuf,
		      SHORTCUTS_COL_PIXBUF_VISIBLE, TRUE,
		      SHORTCUTS_COL_NAME, request->label_copy,
		      SHORTCUTS_COL_TYPE, SHORTCUT_TYPE_FILE,
		      SHORTCUTS_COL_REMOVABLE, request->removable,
		      -1);

  if (request->impl->shortcuts_pane_filter_model)
    btk_tree_model_filter_refilter (BTK_TREE_MODEL_FILTER (request->impl->shortcuts_pane_filter_model));

  if (pixbuf)
    g_object_unref (pixbuf);

out:
  g_object_unref (request->impl);
  g_object_unref (request->file);
  btk_tree_row_reference_free (request->row_ref);
  g_free (request->label_copy);
  g_free (request);

  g_object_unref (cancellable);
}

/* FIXME: BtkFileSystem needs a function to split a remote path
 * into hostname and path components, or maybe just have a 
 * btk_file_system_path_get_display_name().
 *
 * This function is also used in btkfilechooserbutton.c
 */
gchar *
_btk_file_chooser_label_for_file (GFile *file)
{
  const gchar *path, *start, *end, *p;
  gchar *uri, *host, *label;

  uri = g_file_get_uri (file);

  start = strstr (uri, "://");
  if (start)
    {
      start += 3;
      path = strchr (start, '/');
      if (path)
        end = path;
      else
        {
          end = uri + strlen (uri);
          path = "/";
        }

      /* strip username */
      p = strchr (start, '@');
      if (p && p < end)
        start = p + 1;
  
      p = strchr (start, ':');
      if (p && p < end)
        end = p;
  
      host = g_strndup (start, end - start);
  
      /* Translators: the first string is a path and the second string 
       * is a hostname. Nautilus and the panel contain the same string 
       * to translate. 
       */
      label = g_strdup_printf (_("%1$s on %2$s"), path, host);
  
      g_free (host);
    }
  else
    {
      label = g_strdup (uri);
    }
  
  g_free (uri);

  return label;
}

/* Inserts a path in the shortcuts tree, making a copy of it; alternatively,
 * inserts a volume.  A position of -1 indicates the end of the tree.
 */
static void
shortcuts_insert_file (BtkFileChooserDefault *impl,
		       int                    pos,
		       ShortcutType           shortcut_type,
		       BtkFileSystemVolume   *volume,
		       GFile                 *file,
		       const char            *label,
		       gboolean               removable,
		       ShortcutsIndex         type)
{
  char *label_copy;
  BdkPixbuf *pixbuf = NULL;
  gpointer data = NULL;
  BtkTreeIter iter;
  BtkIconTheme *icon_theme;

  profile_start ("start shortcut", NULL);

  if (shortcut_type == SHORTCUT_TYPE_VOLUME)
    {
      data = volume;
      label_copy = _btk_file_system_volume_get_display_name (volume);
      pixbuf = _btk_file_system_volume_render_icon (volume, BTK_WIDGET (impl),
				 		    impl->icon_size, NULL);
    }
  else if (shortcut_type == SHORTCUT_TYPE_FILE)
    {
      if (g_file_is_native (file))
        {
          struct ShortcutsInsertRequest *request;
	  GCancellable *cancellable;
          BtkTreePath *p;

          request = g_new0 (struct ShortcutsInsertRequest, 1);
          request->impl = g_object_ref (impl);
          request->file = g_object_ref (file);
          request->name_only = TRUE;
          request->removable = removable;
          request->pos = pos;
          request->type = type;
          if (label)
	    request->label_copy = g_strdup (label);

          if (pos == -1)
	    btk_list_store_append (impl->shortcuts_model, &iter);
          else
	    btk_list_store_insert (impl->shortcuts_model, &iter, pos);

          p = btk_tree_model_get_path (BTK_TREE_MODEL (impl->shortcuts_model), &iter);
          request->row_ref = btk_tree_row_reference_new (BTK_TREE_MODEL (impl->shortcuts_model), p);
          btk_tree_path_free (p);

          cancellable = _btk_file_system_get_info (request->impl->file_system, request->file,
						   "standard::is-hidden,standard::is-backup,standard::display-name,standard::icon",
						   get_file_info_finished, request);

          btk_list_store_set (impl->shortcuts_model, &iter,
			      SHORTCUTS_COL_DATA, g_object_ref (file),
			      SHORTCUTS_COL_TYPE, SHORTCUT_TYPE_FILE,
			      SHORTCUTS_COL_CANCELLABLE, cancellable,
			      -1);

          shortcuts_update_count (impl, type, 1);

          return;
        }
      else
        {
          /* Don't call get_info for remote paths to avoid latency and
           * auth dialogs.
           */
          data = g_object_ref (file);
          if (label)
	    label_copy = g_strdup (label);
          else
	    label_copy = _btk_file_chooser_label_for_file (file);

          /* If we switch to a better bookmarks file format (XBEL), we
           * should use mime info to get a better icon.
           */
          icon_theme = btk_icon_theme_get_for_screen (btk_widget_get_screen (BTK_WIDGET (impl)));
          pixbuf = btk_icon_theme_load_icon (icon_theme, "folder-remote", 
					     impl->icon_size, 0, NULL);
        }
    }
   else
    {
      g_assert_not_reached ();

      return;
    }

  if (pos == -1)
    btk_list_store_append (impl->shortcuts_model, &iter);
  else
    btk_list_store_insert (impl->shortcuts_model, &iter, pos);

  shortcuts_update_count (impl, type, 1);

  btk_list_store_set (impl->shortcuts_model, &iter,
		      SHORTCUTS_COL_PIXBUF, pixbuf,
		      SHORTCUTS_COL_PIXBUF_VISIBLE, TRUE,
		      SHORTCUTS_COL_NAME, label_copy,
		      SHORTCUTS_COL_DATA, data,
		      SHORTCUTS_COL_TYPE, shortcut_type,
		      SHORTCUTS_COL_REMOVABLE, removable,
		      SHORTCUTS_COL_CANCELLABLE, NULL,
		      -1);

  if (impl->shortcuts_pane_filter_model)
    btk_tree_model_filter_refilter (BTK_TREE_MODEL_FILTER (impl->shortcuts_pane_filter_model));

  g_free (label_copy);

  if (pixbuf)
    g_object_unref (pixbuf);

  profile_end ("end", NULL);
}

static void
shortcuts_append_search (BtkFileChooserDefault *impl)
{
  BdkPixbuf *pixbuf;
  BtkTreeIter iter;

  pixbuf = render_search_icon (impl);

  btk_list_store_append (impl->shortcuts_model, &iter);
  btk_list_store_set (impl->shortcuts_model, &iter,
		      SHORTCUTS_COL_PIXBUF, pixbuf,
		      SHORTCUTS_COL_PIXBUF_VISIBLE, TRUE,
		      SHORTCUTS_COL_NAME, _("Search"),
		      SHORTCUTS_COL_DATA, NULL,
		      SHORTCUTS_COL_TYPE, SHORTCUT_TYPE_SEARCH,
		      SHORTCUTS_COL_REMOVABLE, FALSE,
		      -1);

  if (pixbuf)
    g_object_unref (pixbuf);

  impl->has_search = TRUE;
}

static void
shortcuts_append_recent (BtkFileChooserDefault *impl)
{
  BdkPixbuf *pixbuf;
  BtkTreeIter iter;

  pixbuf = render_recent_icon (impl);

  btk_list_store_append (impl->shortcuts_model, &iter);
  btk_list_store_set (impl->shortcuts_model, &iter,
                      SHORTCUTS_COL_PIXBUF, pixbuf,
                      SHORTCUTS_COL_PIXBUF_VISIBLE, TRUE,
                      SHORTCUTS_COL_NAME, _("Recently Used"),
                      SHORTCUTS_COL_DATA, NULL,
                      SHORTCUTS_COL_TYPE, SHORTCUT_TYPE_RECENT,
                      SHORTCUTS_COL_REMOVABLE, FALSE,
                      -1);
  
  if (pixbuf)
    g_object_unref (pixbuf);
}

/* Appends the current working directory to the shortuts panel, but only if it is not equal to $HOME.
 * This is so that the user can actually use the $CWD, for example, if running an application
 * from the shell.
 */
static void
shortcuts_append_cwd (BtkFileChooserDefault *impl)
{
  char *cwd;
  const char *home;
  GFile *cwd_file;
  GFile *home_file;

  impl->has_cwd = FALSE;

  cwd = g_get_current_dir ();
  if (cwd == NULL)
    return;

  home = g_get_home_dir ();
  if (home == NULL)
    {
      g_free (cwd);
      return;
    }

  cwd_file = g_file_new_for_path (cwd);
  home_file = g_file_new_for_path (home);

  if (!g_file_equal (cwd_file, home_file))
    {
      shortcuts_insert_file (impl, -1, SHORTCUT_TYPE_FILE, NULL, cwd_file, NULL, FALSE, SHORTCUTS_CWD);
      impl->has_cwd = TRUE;
    }

  g_object_unref (cwd_file);
  g_object_unref (home_file);
  g_free (cwd);
}

/* Appends an item for the user's home directory to the shortcuts model */
static void
shortcuts_append_home (BtkFileChooserDefault *impl)
{
  const char *home_path;
  GFile *home;

  profile_start ("start", NULL);

  home_path = g_get_home_dir ();
  if (home_path == NULL)
    {
      profile_end ("end - no home directory!?", NULL);
      return;
    }

  home = g_file_new_for_path (home_path);
  shortcuts_insert_file (impl, -1, SHORTCUT_TYPE_FILE, NULL, home, NULL, FALSE, SHORTCUTS_HOME);
  impl->has_home = TRUE;

  g_object_unref (home);

  profile_end ("end", NULL);
}

/* Appends the ~/Desktop directory to the shortcuts model */
static void
shortcuts_append_desktop (BtkFileChooserDefault *impl)
{
  const char *name;
  GFile *file;

  profile_start ("start", NULL);

  name = g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP);
  /* "To disable a directory, point it to the homedir."
   * See http://freedesktop.org/wiki/Software/xdg-user-dirs
   **/
  if (!g_strcmp0 (name, g_get_home_dir ()))
    {
      profile_end ("end", NULL);
      return;
    }

  file = g_file_new_for_path (name);
  shortcuts_insert_file (impl, -1, SHORTCUT_TYPE_FILE, NULL, file, _("Desktop"), FALSE, SHORTCUTS_DESKTOP);
  impl->has_desktop = TRUE;

  /* We do not actually pop up an error dialog if there is no desktop directory
   * because some people may really not want to have one.
   */

  g_object_unref (file);

  profile_end ("end", NULL);
}

/* Appends a list of GFile to the shortcuts model; returns how many were inserted */
static int
shortcuts_append_bookmarks (BtkFileChooserDefault *impl,
			    GSList                *bookmarks)
{
  int start_row;
  int num_inserted;
  gchar *label;

  profile_start ("start", NULL);

  start_row = shortcuts_get_index (impl, SHORTCUTS_BOOKMARKS_SEPARATOR) + 1;
  num_inserted = 0;

  for (; bookmarks; bookmarks = bookmarks->next)
    {
      GFile *file;

      file = bookmarks->data;

      if (impl->local_only && !_btk_file_has_native_path (file))
	continue;

      if (shortcut_find_position (impl, file) != -1)
        continue;

      label = _btk_file_system_get_bookmark_label (impl->file_system, file);

      shortcuts_insert_file (impl, start_row + num_inserted, SHORTCUT_TYPE_FILE, NULL, file, label, TRUE, SHORTCUTS_BOOKMARKS);
      g_free (label);

      num_inserted++;
    }

  profile_end ("end", NULL);

  return num_inserted;
}

/* Returns the index for the corresponding item in the shortcuts bar */
static int
shortcuts_get_index (BtkFileChooserDefault *impl,
		     ShortcutsIndex         where)
{
  int n;

  n = 0;
  
  if (where == SHORTCUTS_SEARCH)
    goto out;

  n += impl->has_search ? 1 : 0;

  if (where == SHORTCUTS_RECENT)
    goto out;

  n += 1; /* we always have the recently-used item */

  if (where == SHORTCUTS_CWD)
    goto out;

  n += impl->has_cwd ? 1 : 0;

  if (where == SHORTCUTS_RECENT_SEPARATOR)
    goto out;

  n += 1; /* we always have the separator after the recently-used item */

  if (where == SHORTCUTS_HOME)
    goto out;

  n += impl->has_home ? 1 : 0;

  if (where == SHORTCUTS_DESKTOP)
    goto out;

  n += impl->has_desktop ? 1 : 0;

  if (where == SHORTCUTS_VOLUMES)
    goto out;

  n += impl->num_volumes;

  if (where == SHORTCUTS_SHORTCUTS)
    goto out;

  n += impl->num_shortcuts;

  if (where == SHORTCUTS_BOOKMARKS_SEPARATOR)
    goto out;

  /* If there are no bookmarks there won't be a separator */
  n += (impl->num_bookmarks > 0) ? 1 : 0;

  if (where == SHORTCUTS_BOOKMARKS)
    goto out;

  n += impl->num_bookmarks;

  if (where == SHORTCUTS_CURRENT_FOLDER_SEPARATOR)
    goto out;

  n += 1;

  if (where == SHORTCUTS_CURRENT_FOLDER)
    goto out;

  g_assert_not_reached ();

 out:

  return n;
}

/* Adds all the file system volumes to the shortcuts model */
static void
shortcuts_add_volumes (BtkFileChooserDefault *impl)
{
  int start_row;
  GSList *list, *l;
  int n;
  gboolean old_changing_folders;

  profile_start ("start", NULL);

  old_changing_folders = impl->changing_folder;
  impl->changing_folder = TRUE;

  start_row = shortcuts_get_index (impl, SHORTCUTS_VOLUMES);
  shortcuts_remove_rows (impl, start_row, impl->num_volumes);
  impl->num_volumes = 0;

  list = _btk_file_system_list_volumes (impl->file_system);

  n = 0;

  for (l = list; l; l = l->next)
    {
      BtkFileSystemVolume *volume;

      volume = l->data;

      if (impl->local_only)
	{
	  if (_btk_file_system_volume_is_mounted (volume))
	    {
	      GFile *base_file;
              gboolean base_has_native_path = FALSE;

	      base_file = _btk_file_system_volume_get_root (volume);
              if (base_file != NULL)
                {
                  base_has_native_path = _btk_file_has_native_path (base_file);
                  g_object_unref (base_file);
                }

              if (!base_has_native_path)
                continue;
	    }
	}

      shortcuts_insert_file (impl,
                             start_row + n,
                             SHORTCUT_TYPE_VOLUME,
                             _btk_file_system_volume_ref (volume),
                             NULL,
                             NULL,
                             FALSE,
                             SHORTCUTS_VOLUMES);
      n++;
    }

  impl->num_volumes = n;
  b_slist_free (list);

  if (impl->shortcuts_pane_filter_model)
    btk_tree_model_filter_refilter (BTK_TREE_MODEL_FILTER (impl->shortcuts_pane_filter_model));

  impl->changing_folder = old_changing_folders;

  profile_end ("end", NULL);
}

/* Inserts a separator node in the shortcuts list */
static void
shortcuts_insert_separator (BtkFileChooserDefault *impl,
			    ShortcutsIndex         where)
{
  BtkTreeIter iter;

  g_assert (where == SHORTCUTS_RECENT_SEPARATOR || 
            where == SHORTCUTS_BOOKMARKS_SEPARATOR || 
	    where == SHORTCUTS_CURRENT_FOLDER_SEPARATOR);

  btk_list_store_insert (impl->shortcuts_model, &iter,
			 shortcuts_get_index (impl, where));
  btk_list_store_set (impl->shortcuts_model, &iter,
		      SHORTCUTS_COL_PIXBUF, NULL,
		      SHORTCUTS_COL_PIXBUF_VISIBLE, FALSE,
		      SHORTCUTS_COL_NAME, NULL,
		      SHORTCUTS_COL_DATA, NULL,
		      SHORTCUTS_COL_TYPE, SHORTCUT_TYPE_SEPARATOR,
		      -1);
}

/* Updates the list of bookmarks */
static void
shortcuts_add_bookmarks (BtkFileChooserDefault *impl)
{
  GSList *bookmarks;
  gboolean old_changing_folders;
  BtkTreeIter iter;
  GFile *list_selected = NULL;
  ShortcutType shortcut_type;
  gpointer col_data;

  profile_start ("start", NULL);
        
  old_changing_folders = impl->changing_folder;
  impl->changing_folder = TRUE;

  if (shortcuts_get_selected (impl, &iter))
    {
      btk_tree_model_get (BTK_TREE_MODEL (impl->shortcuts_model), 
			  &iter, 
			  SHORTCUTS_COL_DATA, &col_data,
			  SHORTCUTS_COL_TYPE, &shortcut_type,
			  -1);

      if (col_data && shortcut_type == SHORTCUT_TYPE_FILE)
	list_selected = g_object_ref (col_data);
    }

  if (impl->num_bookmarks > 0)
    shortcuts_remove_rows (impl,
			   shortcuts_get_index (impl, SHORTCUTS_BOOKMARKS_SEPARATOR),
			   impl->num_bookmarks + 1);

  impl->num_bookmarks = 0;
  shortcuts_insert_separator (impl, SHORTCUTS_BOOKMARKS_SEPARATOR);

  bookmarks = _btk_file_system_list_bookmarks (impl->file_system);
  shortcuts_append_bookmarks (impl, bookmarks);
  b_slist_foreach (bookmarks, (GFunc) g_object_unref, NULL);
  b_slist_free (bookmarks);

  if (impl->num_bookmarks == 0)
    shortcuts_remove_rows (impl, shortcuts_get_index (impl, SHORTCUTS_BOOKMARKS_SEPARATOR), 1);

  if (impl->shortcuts_pane_filter_model)
    btk_tree_model_filter_refilter (BTK_TREE_MODEL_FILTER (impl->shortcuts_pane_filter_model));

  if (list_selected)
    {
      shortcuts_find_folder (impl, list_selected);
      g_object_unref (list_selected);
    }

  impl->changing_folder = old_changing_folders;

  profile_end ("end", NULL);
}

/* Appends a separator and a row to the shortcuts list for the current folder */
static void
shortcuts_add_current_folder (BtkFileChooserDefault *impl)
{
  int pos;

  g_assert (!impl->shortcuts_current_folder_active);

  g_assert (impl->current_folder != NULL);

  pos = shortcut_find_position (impl, impl->current_folder);
  if (pos == -1)
    {
      BtkFileSystemVolume *volume;
      GFile *base_file;

      /* Separator */
      shortcuts_insert_separator (impl, SHORTCUTS_CURRENT_FOLDER_SEPARATOR);

      /* Item */
      pos = shortcuts_get_index (impl, SHORTCUTS_CURRENT_FOLDER);

      volume = _btk_file_system_get_volume_for_file (impl->file_system, impl->current_folder);
      if (volume)
        base_file = _btk_file_system_volume_get_root (volume);
      else
        base_file = NULL;

      if (base_file && g_file_equal (base_file, impl->current_folder))
        shortcuts_insert_file (impl, pos, SHORTCUT_TYPE_VOLUME, volume, NULL, NULL, FALSE, SHORTCUTS_CURRENT_FOLDER);
      else
        shortcuts_insert_file (impl, pos, SHORTCUT_TYPE_FILE, NULL, impl->current_folder, NULL, FALSE, SHORTCUTS_CURRENT_FOLDER);

      if (base_file)
        g_object_unref (base_file);
    }
}

/* Updates the current folder row in the shortcuts model */
static void
shortcuts_update_current_folder (BtkFileChooserDefault *impl)
{
  int pos;

  pos = shortcuts_get_index (impl, SHORTCUTS_CURRENT_FOLDER_SEPARATOR);

  if (impl->shortcuts_current_folder_active)
    {
      shortcuts_remove_rows (impl, pos, 2);
      impl->shortcuts_current_folder_active = FALSE;
    }

  shortcuts_add_current_folder (impl);
}

/* Filter function used for the shortcuts filter model */
static gboolean
shortcuts_pane_filter_cb (BtkTreeModel *model,
			  BtkTreeIter  *iter,
			  gpointer      data)
{
  BtkFileChooserDefault *impl;
  BtkTreePath *path;
  int pos;

  impl = BTK_FILE_CHOOSER_DEFAULT (data);

  path = btk_tree_model_get_path (model, iter);
  if (!path)
    return FALSE;

  pos = *btk_tree_path_get_indices (path);
  btk_tree_path_free (path);

  return (pos < shortcuts_get_index (impl, SHORTCUTS_CURRENT_FOLDER_SEPARATOR));
}

/* Creates the list model for shortcuts */
static void
shortcuts_model_create (BtkFileChooserDefault *impl)
{
  /* Keep this order in sync with the SHORCUTS_COL_* enum values */
  impl->shortcuts_model = btk_list_store_new (SHORTCUTS_COL_NUM_COLUMNS,
					      BDK_TYPE_PIXBUF,	/* pixbuf */
					      B_TYPE_STRING,	/* name */
					      B_TYPE_POINTER,	/* path or volume */
					      B_TYPE_INT,       /* ShortcutType */
					      B_TYPE_BOOLEAN,   /* removable */
					      B_TYPE_BOOLEAN,   /* pixbuf cell visibility */
					      B_TYPE_POINTER);  /* GCancellable */

  shortcuts_append_search (impl);

  if (impl->recent_manager)
    {
      shortcuts_append_recent (impl);
      shortcuts_insert_separator (impl, SHORTCUTS_RECENT_SEPARATOR);
    }
  
  if (impl->file_system)
    {
      shortcuts_append_cwd (impl);
      shortcuts_append_home (impl);
      shortcuts_append_desktop (impl);
      shortcuts_add_volumes (impl);
    }

  impl->shortcuts_pane_filter_model = shortcuts_pane_model_filter_new (impl,
							               BTK_TREE_MODEL (impl->shortcuts_model),
								       NULL);

  btk_tree_model_filter_set_visible_func (BTK_TREE_MODEL_FILTER (impl->shortcuts_pane_filter_model),
					  shortcuts_pane_filter_cb,
					  impl,
					  NULL);
}

/* Callback used when the "New Folder" button is clicked */
static void
new_folder_button_clicked (BtkButton             *button,
			   BtkFileChooserDefault *impl)
{
  BtkTreeIter iter;
  BtkTreePath *path;

  if (!impl->browse_files_model)
    return; /* FIXME: this sucks.  Disable the New Folder button or something. */

  /* Prevent button from being clicked twice */
  btk_widget_set_sensitive (impl->browse_new_folder_button, FALSE);

  _btk_file_system_model_add_editable (impl->browse_files_model, &iter);

  path = btk_tree_model_get_path (BTK_TREE_MODEL (impl->browse_files_model), &iter);
  btk_tree_view_scroll_to_cell (BTK_TREE_VIEW (impl->browse_files_tree_view),
				path, impl->list_name_column,
				FALSE, 0.0, 0.0);

  g_object_set (impl->list_name_renderer, "editable", TRUE, NULL);
  btk_tree_view_set_cursor (BTK_TREE_VIEW (impl->browse_files_tree_view),
			    path,
			    impl->list_name_column,
			    TRUE);

  btk_tree_path_free (path);
}

static GSource *
add_idle_while_impl_is_alive (BtkFileChooserDefault *impl, GCallback callback)
{
  GSource *source;

  source = g_idle_source_new ();
  g_source_set_closure (source,
			g_cclosure_new_object (callback, B_OBJECT (impl)));
  g_source_attach (source, NULL);

  return source;
}

/* Idle handler for creating a new folder after editing its name cell, or for
 * canceling the editing.
 */
static gboolean
edited_idle_cb (BtkFileChooserDefault *impl)
{
  BDK_THREADS_ENTER ();
  
  g_source_destroy (impl->edited_idle);
  impl->edited_idle = NULL;

  _btk_file_system_model_remove_editable (impl->browse_files_model);
  g_object_set (impl->list_name_renderer, "editable", FALSE, NULL);

  btk_widget_set_sensitive (impl->browse_new_folder_button, TRUE);

  if (impl->edited_new_text /* not cancelled? */
      && (strlen (impl->edited_new_text) != 0)
      && (strcmp (impl->edited_new_text, DEFAULT_NEW_FOLDER_NAME) != 0)) /* Don't create folder if name is empty or has not been edited */
    {
      GError *error = NULL;
      GFile *file;

      file = g_file_get_child_for_display_name (impl->current_folder,
						impl->edited_new_text,
						&error);
      if (file)
	{
	  GError *error = NULL;

	  if (g_file_make_directory (file, NULL, &error))
	    change_folder_and_display_error (impl, file, FALSE);
	  else
	    error_creating_folder_dialog (impl, file, error);

	  g_object_unref (file);
	}
      else
	error_creating_folder_dialog (impl, file, error);

      g_free (impl->edited_new_text);
      impl->edited_new_text = NULL;
    }

  BDK_THREADS_LEAVE ();

  return FALSE;
}

static void
queue_edited_idle (BtkFileChooserDefault *impl,
		   const gchar           *new_text)
{
  /* We create the folder in an idle handler so that we don't modify the tree
   * just now.
   */

  if (!impl->edited_idle)
    impl->edited_idle = add_idle_while_impl_is_alive (impl, G_CALLBACK (edited_idle_cb));

  g_free (impl->edited_new_text);
  impl->edited_new_text = g_strdup (new_text);
}

/* Callback used from the text cell renderer when the new folder is named */
static void
renderer_edited_cb (BtkCellRendererText   *cell_renderer_text,
		    const gchar           *path,
		    const gchar           *new_text,
		    BtkFileChooserDefault *impl)
{
  /* work around bug #154921 */
  g_object_set (cell_renderer_text, 
		"mode", BTK_CELL_RENDERER_MODE_INERT, NULL);
  queue_edited_idle (impl, new_text);
}

/* Callback used from the text cell renderer when the new folder edition gets
 * canceled.
 */
static void
renderer_editing_canceled_cb (BtkCellRendererText   *cell_renderer_text,
			      BtkFileChooserDefault *impl)
{
  /* work around bug #154921 */
  g_object_set (cell_renderer_text, 
		"mode", BTK_CELL_RENDERER_MODE_INERT, NULL);
  queue_edited_idle (impl, NULL);
}

/* Creates the widgets for the filter combo box */
static BtkWidget *
filter_create (BtkFileChooserDefault *impl)
{
  impl->filter_combo = btk_combo_box_text_new ();
  btk_combo_box_set_focus_on_click (BTK_COMBO_BOX (impl->filter_combo), FALSE);

  g_signal_connect (impl->filter_combo, "changed",
		    G_CALLBACK (filter_combo_changed), impl);

  btk_widget_set_tooltip_text (impl->filter_combo,
                               _("Select which types of files are shown"));

  return impl->filter_combo;
}

static BtkWidget *
toolbutton_new (BtkFileChooserDefault *impl,
                GIcon                 *icon,
                gboolean               sensitive,
                gboolean               show,
                GCallback              callback)
{
  BtkToolItem *item;
  BtkWidget *image;

  item = btk_tool_button_new (NULL, NULL);
  image = btk_image_new_from_gicon (icon, BTK_ICON_SIZE_SMALL_TOOLBAR);
  btk_widget_show (image);
  btk_tool_button_set_icon_widget (BTK_TOOL_BUTTON (item), image);

  btk_widget_set_sensitive (BTK_WIDGET (item), sensitive);
  g_signal_connect (item, "clicked", callback, impl);

  if (show)
    btk_widget_show (BTK_WIDGET (item));

  return BTK_WIDGET (item);
}

/* Looks for a path among the shortcuts; returns its index or -1 if it doesn't exist */
static int
shortcut_find_position (BtkFileChooserDefault *impl,
			GFile                 *file)
{
  BtkTreeIter iter;
  int i;
  int current_folder_separator_idx;

  if (!btk_tree_model_get_iter_first (BTK_TREE_MODEL (impl->shortcuts_model), &iter))
    return -1;

  current_folder_separator_idx = shortcuts_get_index (impl, SHORTCUTS_CURRENT_FOLDER_SEPARATOR);

#if 0
  /* FIXME: is this still needed? */
  if (current_folder_separator_idx >= impl->shortcuts_model->length)
    return -1;
#endif

  for (i = 0; i < current_folder_separator_idx; i++)
    {
      gpointer col_data;
      ShortcutType shortcut_type;

      btk_tree_model_get (BTK_TREE_MODEL (impl->shortcuts_model), &iter,
			  SHORTCUTS_COL_DATA, &col_data,
			  SHORTCUTS_COL_TYPE, &shortcut_type,
			  -1);

      if (col_data)
	{
	  if (shortcut_type == SHORTCUT_TYPE_VOLUME)
	    {
	      BtkFileSystemVolume *volume;
	      GFile *base_file;
	      gboolean exists;

	      volume = col_data;
	      base_file = _btk_file_system_volume_get_root (volume);

	      exists = base_file && g_file_equal (file, base_file);

	      if (base_file)
		g_object_unref (base_file);

	      if (exists)
		return i;
	    }
	  else if (shortcut_type == SHORTCUT_TYPE_FILE)
	    {
	      GFile *model_file;

	      model_file = col_data;

	      if (model_file && g_file_equal (model_file, file))
		return i;
	    }
	}

      if (i < current_folder_separator_idx - 1)
	{
	  if (!btk_tree_model_iter_next (BTK_TREE_MODEL (impl->shortcuts_model), &iter))
	    g_assert_not_reached ();
	}
    }

  return -1;
}

/* Tries to add a bookmark from a path name */
static gboolean
shortcuts_add_bookmark_from_file (BtkFileChooserDefault *impl,
				  GFile                 *file,
				  int                    pos)
{
  GError *error;

  g_return_val_if_fail (G_IS_FILE (file), FALSE);

  if (shortcut_find_position (impl, file) != -1)
    return FALSE;

  error = NULL;
  if (!_btk_file_system_insert_bookmark (impl->file_system, file, pos, &error))
    {
      error_adding_bookmark_dialog (impl, file, error);
      return FALSE;
    }

  return TRUE;
}

static void
add_bookmark_foreach_cb (BtkTreeModel *model,
			 BtkTreePath  *path,
			 BtkTreeIter  *iter,
			 gpointer      data)
{
  BtkFileChooserDefault *impl;
  GFile *file;

  impl = (BtkFileChooserDefault *) data;

  btk_tree_model_get (model, iter,
                      MODEL_COL_FILE, &file,
                      -1);

  shortcuts_add_bookmark_from_file (impl, file, -1);

  g_object_unref (file);
}

/* Adds a bookmark from the currently selected item in the file list */
static void
bookmarks_add_selected_folder (BtkFileChooserDefault *impl)
{
  BtkTreeSelection *selection;

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_files_tree_view));

  if (btk_tree_selection_count_selected_rows (selection) == 0)
    shortcuts_add_bookmark_from_file (impl, impl->current_folder, -1);
  else
    btk_tree_selection_selected_foreach (selection,
					 add_bookmark_foreach_cb,
					 impl);
}

/* Callback used when the "Add bookmark" button is clicked */
static void
add_bookmark_button_clicked_cb (BtkButton *button,
				BtkFileChooserDefault *impl)
{
  bookmarks_add_selected_folder (impl);
}

/* Returns TRUE plus an iter in the shortcuts_model if a row is selected;
 * returns FALSE if no shortcut is selected.
 */
static gboolean
shortcuts_get_selected (BtkFileChooserDefault *impl,
			BtkTreeIter           *iter)
{
  BtkTreeSelection *selection;
  BtkTreeIter parent_iter;

  if (!impl->browse_shortcuts_tree_view)
    return FALSE;

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_shortcuts_tree_view));

  if (!btk_tree_selection_get_selected (selection, NULL, &parent_iter))
    return FALSE;

  btk_tree_model_filter_convert_iter_to_child_iter (BTK_TREE_MODEL_FILTER (impl->shortcuts_pane_filter_model),
						    iter,
						    &parent_iter);
  return TRUE;
}

/* Removes the selected bookmarks */
static void
remove_selected_bookmarks (BtkFileChooserDefault *impl)
{
  BtkTreeIter iter;
  gpointer col_data;
  GFile *file;
  gboolean removable;
  GError *error;

  if (!shortcuts_get_selected (impl, &iter))
    return;

  btk_tree_model_get (BTK_TREE_MODEL (impl->shortcuts_model), &iter,
		      SHORTCUTS_COL_DATA, &col_data,
		      SHORTCUTS_COL_REMOVABLE, &removable,
		      -1);

  if (!removable)
    return;

  g_assert (col_data != NULL);

  file = col_data;

  error = NULL;
  if (!_btk_file_system_remove_bookmark (impl->file_system, file, &error))
    error_removing_bookmark_dialog (impl, file, error);
}

/* Callback used when the "Remove bookmark" button is clicked */
static void
remove_bookmark_button_clicked_cb (BtkButton *button,
				   BtkFileChooserDefault *impl)
{
  remove_selected_bookmarks (impl);
}

struct selection_check_closure {
  BtkFileChooserDefault *impl;
  int num_selected;
  gboolean all_files;
  gboolean all_folders;
};

/* Used from btk_tree_selection_selected_foreach() */
static void
selection_check_foreach_cb (BtkTreeModel *model,
			    BtkTreePath  *path,
			    BtkTreeIter  *iter,
			    gpointer      data)
{
  struct selection_check_closure *closure;
  gboolean is_folder;
  GFile *file;

  btk_tree_model_get (model, iter,
                      MODEL_COL_FILE, &file,
                      MODEL_COL_IS_FOLDER, &is_folder,
                      -1);

  if (file == NULL)
    return;

  g_object_unref (file);

  closure = data;
  closure->num_selected++;

  closure->all_folders = closure->all_folders && is_folder;
  closure->all_files = closure->all_files && !is_folder;
}

/* Checks whether the selected items in the file list are all files or all folders */
static void
selection_check (BtkFileChooserDefault *impl,
		 gint                  *num_selected,
		 gboolean              *all_files,
		 gboolean              *all_folders)
{
  struct selection_check_closure closure;
  BtkTreeSelection *selection;

  closure.impl = impl;
  closure.num_selected = 0;
  closure.all_files = TRUE;
  closure.all_folders = TRUE;

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_files_tree_view));
  btk_tree_selection_selected_foreach (selection,
				       selection_check_foreach_cb,
				       &closure);

  g_assert (closure.num_selected == 0 || !(closure.all_files && closure.all_folders));

  if (num_selected)
    *num_selected = closure.num_selected;

  if (all_files)
    *all_files = closure.all_files;

  if (all_folders)
    *all_folders = closure.all_folders;
}

struct get_selected_file_closure {
  BtkFileChooserDefault *impl;
  GFile *file;
};

static void
get_selected_file_foreach_cb (BtkTreeModel *model,
			      BtkTreePath  *path,
			      BtkTreeIter  *iter,
			      gpointer      data)
{
  struct get_selected_file_closure *closure = data;

  if (closure->file)
    {
      /* Just in case this function gets run more than once with a multiple selection; we only care about one file */
      g_object_unref (closure->file);
      closure->file = NULL;
    }

  btk_tree_model_get (model, iter,
                      MODEL_COL_FILE, &closure->file, /* this will give us a reffed file */
                      -1);
}

/* Returns a selected path from the file list */
static GFile *
get_selected_file (BtkFileChooserDefault *impl)
{
  struct get_selected_file_closure closure;
  BtkTreeSelection *selection;

  closure.impl = impl;
  closure.file = NULL;

  selection =  btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_files_tree_view));
  btk_tree_selection_selected_foreach (selection,
				       get_selected_file_foreach_cb,
				       &closure);

  return closure.file;
}

typedef struct {
  BtkFileChooserDefault *impl;
  gchar *tip;
} UpdateTooltipData;

static void 
update_tooltip (BtkTreeModel      *model,
		BtkTreePath       *path,
		BtkTreeIter       *iter,
		gpointer           data)
{
  UpdateTooltipData *udata = data;

  if (udata->tip == NULL)
    {
      gchar *display_name;

      btk_tree_model_get (model, iter,
                          MODEL_COL_NAME, &display_name,
                          -1);

      udata->tip = g_strdup_printf (_("Add the folder '%s' to the bookmarks"),
                                    display_name);
      g_free (display_name);
    }
}


/* Sensitize the "add bookmark" button if all the selected items are folders, or
 * if there are no selected items *and* the current folder is not in the
 * bookmarks list.  De-sensitize the button otherwise.
 */
static void
bookmarks_check_add_sensitivity (BtkFileChooserDefault *impl)
{
  gint num_selected;
  gboolean all_folders;
  gboolean active;
  gchar *tip;

  selection_check (impl, &num_selected, NULL, &all_folders);

  if (num_selected == 0)
    active = (impl->current_folder != NULL) && (shortcut_find_position (impl, impl->current_folder) == -1);
  else if (num_selected == 1)
    {
      GFile *file;

      file = get_selected_file (impl);
      active = file && all_folders && (shortcut_find_position (impl, file) == -1);
      if (file)
	g_object_unref (file);
    }
  else
    active = all_folders;

  btk_widget_set_sensitive (impl->browse_shortcuts_add_button, active);

  if (impl->browse_files_popup_menu_add_shortcut_item)
    btk_widget_set_sensitive (impl->browse_files_popup_menu_add_shortcut_item,
                              (num_selected == 0) ? FALSE : active);

  if (active)
    {
      if (num_selected == 0)
        tip = g_strdup_printf (_("Add the current folder to the bookmarks"));
      else if (num_selected > 1)
        tip = g_strdup_printf (_("Add the selected folders to the bookmarks"));
      else
        {
          BtkTreeSelection *selection;
          UpdateTooltipData data;

          selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_files_tree_view));
          data.impl = impl;
          data.tip = NULL;
          btk_tree_selection_selected_foreach (selection, update_tooltip, &data);
          tip = data.tip;
        }

      btk_widget_set_tooltip_text (impl->browse_shortcuts_add_button, tip);
      g_free (tip);
    }
}

/* Sets the sensitivity of the "remove bookmark" button depending on whether a
 * bookmark row is selected in the shortcuts tree.
 */
static void
bookmarks_check_remove_sensitivity (BtkFileChooserDefault *impl)
{
  BtkTreeIter iter;
  gboolean removable = FALSE;
  gboolean have_name = FALSE;
  gchar *name = NULL;
  
  if (shortcuts_get_selected (impl, &iter))
    {
      btk_tree_model_get (BTK_TREE_MODEL (impl->shortcuts_model), &iter,
                          SHORTCUTS_COL_REMOVABLE, &removable,
                          SHORTCUTS_COL_NAME, &name,
                          -1);
      btk_widget_set_sensitive (impl->browse_shortcuts_remove_button, removable);

      have_name = name != NULL && name[0] != '\0';
    }

  if (have_name)
    {
      char *tip;

      if (removable)
        tip = g_strdup_printf (_("Remove the bookmark '%s'"), name);
      else
        tip = g_strdup_printf (_("Bookmark '%s' cannot be removed"), name);

      btk_widget_set_tooltip_text (impl->browse_shortcuts_remove_button, tip);
      g_free (tip);
    }
  else
    btk_widget_set_tooltip_text (impl->browse_shortcuts_remove_button,
                                 _("Remove the selected bookmark"));
  g_free (name);
}

static void
shortcuts_check_popup_sensitivity (BtkFileChooserDefault *impl)
{
  BtkTreeIter iter;
  gboolean removable = FALSE;

  if (impl->browse_shortcuts_popup_menu == NULL)
    return;

  if (shortcuts_get_selected (impl, &iter))
    btk_tree_model_get (BTK_TREE_MODEL (impl->shortcuts_model), &iter,
			SHORTCUTS_COL_REMOVABLE, &removable,
			-1);

  btk_widget_set_sensitive (impl->browse_shortcuts_popup_menu_remove_item, removable);
  btk_widget_set_sensitive (impl->browse_shortcuts_popup_menu_rename_item, removable);
}

/* BtkWidget::drag-begin handler for the shortcuts list. */
static void
shortcuts_drag_begin_cb (BtkWidget             *widget,
			 BdkDragContext        *context,
			 BtkFileChooserDefault *impl)
{
#if 0
  impl->shortcuts_drag_context = g_object_ref (context);
#endif
}

#if 0
/* Removes the idle handler for outside drags */
static void
shortcuts_cancel_drag_outside_idle (BtkFileChooserDefault *impl)
{
  if (!impl->shortcuts_drag_outside_idle)
    return;

  g_source_destroy (impl->shortcuts_drag_outside_idle);
  impl->shortcuts_drag_outside_idle = NULL;
}
#endif

/* BtkWidget::drag-end handler for the shortcuts list. */
static void
shortcuts_drag_end_cb (BtkWidget             *widget,
		       BdkDragContext        *context,
		       BtkFileChooserDefault *impl)
{
#if 0
  g_object_unref (impl->shortcuts_drag_context);

  shortcuts_cancel_drag_outside_idle (impl);

  if (!impl->shortcuts_drag_outside)
    return;

  btk_button_clicked (BTK_BUTTON (impl->browse_shortcuts_remove_button));

  impl->shortcuts_drag_outside = FALSE;
#endif
}

/* BtkWidget::drag-data-delete handler for the shortcuts list. */
static void
shortcuts_drag_data_delete_cb (BtkWidget             *widget,
			       BdkDragContext        *context,
			       BtkFileChooserDefault *impl)
{
  g_signal_stop_emission_by_name (widget, "drag-data-delete");
}

/* BtkWidget::drag-leave handler for the shortcuts list.  We unhighlight the
 * drop position.
 */
static void
shortcuts_drag_leave_cb (BtkWidget             *widget,
			 BdkDragContext        *context,
			 guint                  time_,
			 BtkFileChooserDefault *impl)
{
#if 0
  if (btk_drag_get_source_widget (context) == widget && !impl->shortcuts_drag_outside_idle)
    {
      impl->shortcuts_drag_outside_idle = add_idle_while_impl_is_alive (impl, G_CALLBACK (shortcuts_drag_outside_idle_cb));
    }
#endif

  btk_tree_view_set_drag_dest_row (BTK_TREE_VIEW (impl->browse_shortcuts_tree_view),
				   NULL,
				   BTK_TREE_VIEW_DROP_BEFORE);

  g_signal_stop_emission_by_name (widget, "drag-leave");
}

/* Computes the appropriate row and position for dropping */
static void
shortcuts_compute_drop_position (BtkFileChooserDefault   *impl,
				 int                      x,
				 int                      y,
				 BtkTreePath            **path,
				 BtkTreeViewDropPosition *pos)
{
  BtkTreeView *tree_view;
  BtkTreeViewColumn *column;
  int cell_y;
  BdkRectangle cell;
  int row;
  int bookmarks_index;
  int header_height = 0;

  tree_view = BTK_TREE_VIEW (impl->browse_shortcuts_tree_view);

  if (btk_tree_view_get_headers_visible (tree_view))
    header_height = TREE_VIEW_HEADER_HEIGHT (tree_view);

  bookmarks_index = shortcuts_get_index (impl, SHORTCUTS_BOOKMARKS);

  if (!btk_tree_view_get_path_at_pos (tree_view,
                                      x,
				      y - header_height,
                                      path,
                                      &column,
                                      NULL,
                                      &cell_y))
    {
      row = bookmarks_index + impl->num_bookmarks - 1;
      *path = btk_tree_path_new_from_indices (row, -1);
      *pos = BTK_TREE_VIEW_DROP_AFTER;
      return;
    }

  row = *btk_tree_path_get_indices (*path);
  btk_tree_view_get_background_area (tree_view, *path, column, &cell);
  btk_tree_path_free (*path);

  if (row < bookmarks_index)
    {
      row = bookmarks_index;
      *pos = BTK_TREE_VIEW_DROP_BEFORE;
    }
  else if (row > bookmarks_index + impl->num_bookmarks - 1)
    {
      row = bookmarks_index + impl->num_bookmarks - 1;
      *pos = BTK_TREE_VIEW_DROP_AFTER;
    }
  else
    {
      if (cell_y < cell.height / 2)
	*pos = BTK_TREE_VIEW_DROP_BEFORE;
      else
	*pos = BTK_TREE_VIEW_DROP_AFTER;
    }

  *path = btk_tree_path_new_from_indices (row, -1);
}

/* BtkWidget::drag-motion handler for the shortcuts list.  We basically
 * implement the destination side of DnD by hand, due to limitations in
 * BtkTreeView's DnD API.
 */
static gboolean
shortcuts_drag_motion_cb (BtkWidget             *widget,
			  BdkDragContext        *context,
			  gint                   x,
			  gint                   y,
			  guint                  time_,
			  BtkFileChooserDefault *impl)
{
  BtkTreePath *path;
  BtkTreeViewDropPosition pos;
  BdkDragAction action;

#if 0
  if (btk_drag_get_source_widget (context) == widget)
    {
      shortcuts_cancel_drag_outside_idle (impl);

      if (impl->shortcuts_drag_outside)
	{
	  shortcuts_drag_set_delete_cursor (impl, FALSE);
	  impl->shortcuts_drag_outside = FALSE;
	}
    }
#endif

  if (bdk_drag_context_get_suggested_action (context) == BDK_ACTION_COPY ||
      (bdk_drag_context_get_actions (context) & BDK_ACTION_COPY) != 0)
    action = BDK_ACTION_COPY;
  else if (bdk_drag_context_get_suggested_action (context) == BDK_ACTION_MOVE ||
           (bdk_drag_context_get_actions (context) & BDK_ACTION_MOVE) != 0)
    action = BDK_ACTION_MOVE;
  else
    {
      action = 0;
      goto out;
    }

  shortcuts_compute_drop_position (impl, x, y, &path, &pos);
  btk_tree_view_set_drag_dest_row (BTK_TREE_VIEW (impl->browse_shortcuts_tree_view), path, pos);
  btk_tree_path_free (path);

 out:

  g_signal_stop_emission_by_name (widget, "drag-motion");

  if (action != 0)
    {
      bdk_drag_status (context, action, time_);
      return TRUE;
    }
  else
    return FALSE;
}

/* BtkWidget::drag-drop handler for the shortcuts list. */
static gboolean
shortcuts_drag_drop_cb (BtkWidget             *widget,
			BdkDragContext        *context,
			gint                   x,
			gint                   y,
			guint                  time_,
			BtkFileChooserDefault *impl)
{
#if 0
  shortcuts_cancel_drag_outside_idle (impl);
#endif

  g_signal_stop_emission_by_name (widget, "drag-drop");
  return TRUE;
}

/* Parses a "text/uri-list" string and inserts its URIs as bookmarks */
static void
shortcuts_drop_uris (BtkFileChooserDefault *impl,
		     BtkSelectionData      *selection_data,
		     int                    position)
{
  gchar **uris;
  gint i;

  uris = btk_selection_data_get_uris (selection_data);
  if (!uris)
    return;

  for (i = 0; uris[i]; i++)
    {
      char *uri;
      GFile *file;

      uri = uris[i];
      file = g_file_new_for_uri (uri);

      if (shortcuts_add_bookmark_from_file (impl, file, position))
	position++;

      g_object_unref (file);
    }

  g_strfreev (uris);
}

/* Reorders the selected bookmark to the specified position */
static void
shortcuts_reorder (BtkFileChooserDefault *impl,
                   BtkSelectionData      *selection_data,
		   int                    new_position)
{
  BtkTreeIter iter;
  BtkTreeIter filter_iter;
  gpointer col_data;
  ShortcutType shortcut_type;
  BtkTreePath *path;
  int old_position;
  int bookmarks_index;
  GFile *file;
  GError *error;
  gchar *name;
  BtkTreeModel *model;

  /* Get the selected path */

  if (!btk_tree_get_row_drag_data (selection_data, &model, &path))
    return;

  g_assert (model == impl->shortcuts_pane_filter_model);

  btk_tree_model_get_iter (model, &filter_iter, path);
  btk_tree_path_free (path);

  btk_tree_model_filter_convert_iter_to_child_iter (BTK_TREE_MODEL_FILTER (impl->shortcuts_pane_filter_model),
                                                    &iter,
						    &filter_iter);
  path = btk_tree_model_get_path (BTK_TREE_MODEL (impl->shortcuts_model), &iter);

  old_position = *btk_tree_path_get_indices (path);
  btk_tree_path_free (path);

  bookmarks_index = shortcuts_get_index (impl, SHORTCUTS_BOOKMARKS);
  old_position -= bookmarks_index;
  g_assert (old_position >= 0 && old_position < impl->num_bookmarks);

  btk_tree_model_get (BTK_TREE_MODEL (impl->shortcuts_model), &iter,
		      SHORTCUTS_COL_NAME, &name,
		      SHORTCUTS_COL_DATA, &col_data,
		      SHORTCUTS_COL_TYPE, &shortcut_type,
		      -1);
  g_assert (col_data != NULL);
  g_assert (shortcut_type == SHORTCUT_TYPE_FILE);
  
  file = col_data;
  g_object_ref (file); /* removal below will free file, so we need a new ref */

  /* Remove the path from the old position and insert it in the new one */

  if (new_position > old_position)
    new_position--;

  if (old_position == new_position)
    goto out;

  error = NULL;
  if (_btk_file_system_remove_bookmark (impl->file_system, file, &error))
    {
      shortcuts_add_bookmark_from_file (impl, file, new_position);
      _btk_file_system_set_bookmark_label (impl->file_system, file, name);
    }
  else
    error_adding_bookmark_dialog (impl, file, error);

 out:

  g_object_unref (file);
}

/* Callback used when we get the drag data for the bookmarks list.  We add the
 * received URIs as bookmarks if they are folders.
 */
static void
shortcuts_drag_data_received_cb (BtkWidget          *widget,
				 BdkDragContext     *context,
				 gint                x,
				 gint                y,
				 BtkSelectionData   *selection_data,
				 guint               info,
				 guint               time_,
				 gpointer            data)
{
  BtkFileChooserDefault *impl;
  BtkTreePath *tree_path;
  BtkTreeViewDropPosition tree_pos;
  BdkAtom target;
  int position;
  int bookmarks_index;

  impl = BTK_FILE_CHOOSER_DEFAULT (data);

  /* Compute position */

  bookmarks_index = shortcuts_get_index (impl, SHORTCUTS_BOOKMARKS);

  shortcuts_compute_drop_position (impl, x, y, &tree_path, &tree_pos);
  position = *btk_tree_path_get_indices (tree_path);
  btk_tree_path_free (tree_path);

  if (tree_pos == BTK_TREE_VIEW_DROP_AFTER)
    position++;

  g_assert (position >= bookmarks_index);
  position -= bookmarks_index;

  target = btk_selection_data_get_target (selection_data);

  if (btk_targets_include_uri (&target, 1))
    shortcuts_drop_uris (impl, selection_data, position);
  else if (target == bdk_atom_intern_static_string ("BTK_TREE_MODEL_ROW"))
    shortcuts_reorder (impl, selection_data, position);

  g_signal_stop_emission_by_name (widget, "drag-data-received");
}

/* Callback used to display a tooltip in the shortcuts tree */
static gboolean
shortcuts_query_tooltip_cb (BtkWidget             *widget,
			    gint                   x,
			    gint                   y,
			    gboolean               keyboard_mode,
			    BtkTooltip            *tooltip,
			    BtkFileChooserDefault *impl)
{
  BtkTreeModel *model;
  BtkTreeIter iter;

  if (btk_tree_view_get_tooltip_context (BTK_TREE_VIEW (widget),
					 &x, &y,
					 keyboard_mode,
					 &model,
					 NULL,
					 &iter))
    {
      gpointer col_data;
      ShortcutType shortcut_type;

      btk_tree_model_get (model, &iter,
			  SHORTCUTS_COL_DATA, &col_data,
			  SHORTCUTS_COL_TYPE, &shortcut_type,
			  -1);

      if (shortcut_type == SHORTCUT_TYPE_SEPARATOR)
	return FALSE;
      else if (shortcut_type == SHORTCUT_TYPE_VOLUME)
        return FALSE;
      else if (shortcut_type == SHORTCUT_TYPE_FILE)
	{
	  GFile *file;
	  char *parse_name;

	  file = G_FILE (col_data);
	  parse_name = g_file_get_parse_name (file);

	  btk_tooltip_set_text (tooltip, parse_name);

	  g_free (parse_name);

	  return TRUE;
	}
      else if (shortcut_type == SHORTCUT_TYPE_SEARCH)
	{
	  return FALSE;
	}
      else if (shortcut_type == SHORTCUT_TYPE_RECENT)
	{
	  return FALSE;
	}
    }

  return FALSE;
}


/* Callback used when the selection in the shortcuts tree changes */
static void
shortcuts_selection_changed_cb (BtkTreeSelection      *selection,
				BtkFileChooserDefault *impl)
{
  BtkTreeIter iter;
  BtkTreeIter child_iter;

  bookmarks_check_remove_sensitivity (impl);
  shortcuts_check_popup_sensitivity (impl);

  if (impl->changing_folder)
    return;

  if (btk_tree_selection_get_selected(selection, NULL, &iter))
    {
      btk_tree_model_filter_convert_iter_to_child_iter (BTK_TREE_MODEL_FILTER (impl->shortcuts_pane_filter_model),
							&child_iter,
							&iter);
      shortcuts_activate_iter (impl, &child_iter);
    }
}

static gboolean
shortcuts_row_separator_func (BtkTreeModel *model,
			      BtkTreeIter  *iter,
			      gpointer      data)
{
  ShortcutType shortcut_type;

  btk_tree_model_get (model, iter, SHORTCUTS_COL_TYPE, &shortcut_type, -1);
  
  return shortcut_type == SHORTCUT_TYPE_SEPARATOR;
}

static gboolean
shortcuts_key_press_event_after_cb (BtkWidget             *tree_view,
				    BdkEventKey           *event,
				    BtkFileChooserDefault *impl)
{
  BtkWidget *entry;

  /* don't screw up focus switching with Tab */
  if (event->keyval == BDK_KEY_Tab
      || event->keyval == BDK_KEY_KP_Tab
      || event->keyval == BDK_KEY_ISO_Left_Tab
      || event->length < 1)
    return FALSE;

  if (impl->location_entry)
    entry = impl->location_entry;
  else if (impl->search_entry)
    entry = impl->search_entry;
  else
    entry = NULL;

  if (entry)
    {
      btk_widget_grab_focus (entry);
      return btk_widget_event (entry, (BdkEvent *) event);
    }
  else
    return FALSE;
}

/* Callback used when the file list's popup menu is detached */
static void
shortcuts_popup_menu_detach_cb (BtkWidget *attach_widget,
				BtkMenu   *menu)
{
  BtkFileChooserDefault *impl;
  
  impl = g_object_get_data (B_OBJECT (attach_widget), "BtkFileChooserDefault");
  g_assert (BTK_IS_FILE_CHOOSER_DEFAULT (impl));

  impl->browse_shortcuts_popup_menu = NULL;
  impl->browse_shortcuts_popup_menu_remove_item = NULL;
  impl->browse_shortcuts_popup_menu_rename_item = NULL;
}

static void
remove_shortcut_cb (BtkMenuItem           *item,
		    BtkFileChooserDefault *impl)
{
  remove_selected_bookmarks (impl);
}

/* Rename the selected bookmark */
static void
rename_selected_bookmark (BtkFileChooserDefault *impl)
{
  BtkTreeIter iter;
  BtkTreePath *path;
  BtkTreeViewColumn *column;
  BtkCellRenderer *cell;
  GList *renderers;

  if (shortcuts_get_selected (impl, &iter))
    {
      path = btk_tree_model_get_path (BTK_TREE_MODEL (impl->shortcuts_model), &iter);
      column = btk_tree_view_get_column (BTK_TREE_VIEW (impl->browse_shortcuts_tree_view), 0);
      renderers = btk_cell_layout_get_cells (BTK_CELL_LAYOUT (column));
      cell = g_list_nth_data (renderers, 1);
      g_list_free (renderers);
      g_object_set (cell, "editable", TRUE, NULL);
      btk_tree_view_set_cursor_on_cell (BTK_TREE_VIEW (impl->browse_shortcuts_tree_view),
					path, column, cell, TRUE);
      btk_tree_path_free (path);
    }
}

static void
rename_shortcut_cb (BtkMenuItem           *item,
		    BtkFileChooserDefault *impl)
{
  rename_selected_bookmark (impl);
}

/* Constructs the popup menu for the file list if needed */
static void
shortcuts_build_popup_menu (BtkFileChooserDefault *impl)
{
  BtkWidget *item;

  if (impl->browse_shortcuts_popup_menu)
    return;

  impl->browse_shortcuts_popup_menu = btk_menu_new ();
  btk_menu_attach_to_widget (BTK_MENU (impl->browse_shortcuts_popup_menu),
			     impl->browse_shortcuts_tree_view,
			     shortcuts_popup_menu_detach_cb);

  item = btk_image_menu_item_new_with_label (_("Remove"));
  impl->browse_shortcuts_popup_menu_remove_item = item;
  btk_image_menu_item_set_image (BTK_IMAGE_MENU_ITEM (item),
				 btk_image_new_from_stock (BTK_STOCK_REMOVE, BTK_ICON_SIZE_MENU));
  g_signal_connect (item, "activate",
		    G_CALLBACK (remove_shortcut_cb), impl);
  btk_widget_show (item);
  btk_menu_shell_append (BTK_MENU_SHELL (impl->browse_shortcuts_popup_menu), item);

  item = btk_menu_item_new_with_label (_("Rename..."));
  impl->browse_shortcuts_popup_menu_rename_item = item;
  g_signal_connect (item, "activate",
		    G_CALLBACK (rename_shortcut_cb), impl);
  btk_widget_show (item);
  btk_menu_shell_append (BTK_MENU_SHELL (impl->browse_shortcuts_popup_menu), item);

  item = btk_separator_menu_item_new ();
  btk_widget_show (item);
  btk_menu_shell_append (BTK_MENU_SHELL (impl->browse_shortcuts_popup_menu), item);
}

static void
shortcuts_update_popup_menu (BtkFileChooserDefault *impl)
{
  shortcuts_build_popup_menu (impl);  
  shortcuts_check_popup_sensitivity (impl);
}

static void
popup_position_func (BtkMenu   *menu,
                     gint      *x,
                     gint      *y,
                     gboolean  *push_in,
                     gpointer	user_data);

static void
shortcuts_popup_menu (BtkFileChooserDefault *impl,
		      BdkEventButton        *event)
{
  shortcuts_update_popup_menu (impl);
  if (event)
    btk_menu_popup (BTK_MENU (impl->browse_shortcuts_popup_menu),
		    NULL, NULL, NULL, NULL,
		    event->button, event->time);
  else
    {
      btk_menu_popup (BTK_MENU (impl->browse_shortcuts_popup_menu),
		      NULL, NULL,
		      popup_position_func, impl->browse_shortcuts_tree_view,
		      0, BDK_CURRENT_TIME);
      btk_menu_shell_select_first (BTK_MENU_SHELL (impl->browse_shortcuts_popup_menu),
				   FALSE);
    }
}

/* Callback used for the BtkWidget::popup-menu signal of the shortcuts list */
static gboolean
shortcuts_popup_menu_cb (BtkWidget *widget,
			 BtkFileChooserDefault *impl)
{
  shortcuts_popup_menu (impl, NULL);
  return TRUE;
}

/* Callback used when a button is pressed on the shortcuts list.  
 * We trap button 3 to bring up a popup menu.
 */
static gboolean
shortcuts_button_press_event_cb (BtkWidget             *widget,
				 BdkEventButton        *event,
				 BtkFileChooserDefault *impl)
{
  static gboolean in_press = FALSE;
  gboolean handled;

  if (in_press)
    return FALSE;

  if (!_btk_button_event_triggers_context_menu (event))
    return FALSE;

  in_press = TRUE;
  handled = btk_widget_event (impl->browse_shortcuts_tree_view, (BdkEvent *) event);
  in_press = FALSE;

  if (!handled)
    return FALSE;

  shortcuts_popup_menu (impl, event);
  return TRUE;
}

static void
shortcuts_edited (BtkCellRenderer       *cell,
		  gchar                 *path_string,
		  gchar                 *new_text,
		  BtkFileChooserDefault *impl)
{
  BtkTreePath *path;
  BtkTreeIter iter;
  GFile *shortcut;

  g_object_set (cell, "editable", FALSE, NULL);

  path = btk_tree_path_new_from_string (path_string);
  if (!btk_tree_model_get_iter (BTK_TREE_MODEL (impl->shortcuts_model), &iter, path))
    g_assert_not_reached ();

  btk_tree_model_get (BTK_TREE_MODEL (impl->shortcuts_model), &iter,
		      SHORTCUTS_COL_DATA, &shortcut,
		      -1);
  btk_tree_path_free (path);
  
  _btk_file_system_set_bookmark_label (impl->file_system, shortcut, new_text);
}

static void
shortcuts_editing_canceled (BtkCellRenderer       *cell,
			    BtkFileChooserDefault *impl)
{
  g_object_set (cell, "editable", FALSE, NULL);
}

/* Creates the widgets for the shortcuts and bookmarks tree */
static BtkWidget *
shortcuts_list_create (BtkFileChooserDefault *impl)
{
  BtkWidget *swin;
  BtkTreeSelection *selection;
  BtkTreeViewColumn *column;
  BtkCellRenderer *renderer;

  /* Target types for dragging a row to/from the shortcuts list */
  const BtkTargetEntry tree_model_row_targets[] = {
    { "BTK_TREE_MODEL_ROW", BTK_TARGET_SAME_WIDGET, BTK_TREE_MODEL_ROW }
  };

  /* Scrolled window */

  swin = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (swin),
				  BTK_POLICY_NEVER, BTK_POLICY_AUTOMATIC);
  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (swin),
				       BTK_SHADOW_IN);
  btk_widget_show (swin);

  /* Tree */

  impl->browse_shortcuts_tree_view = btk_tree_view_new ();
  btk_tree_view_set_enable_search (BTK_TREE_VIEW (impl->browse_shortcuts_tree_view), FALSE);
#ifdef PROFILE_FILE_CHOOSER
  g_object_set_data (B_OBJECT (impl->browse_shortcuts_tree_view), "fmq-name", "shortcuts");
#endif

  /* Connect "after" to key-press-event on the shortcuts pane.  We want this action to be possible:
   *
   *   1. user brings up a SAVE dialog
   *   2. user clicks on a shortcut in the shortcuts pane
   *   3. user starts typing a filename
   *
   * Normally, the user's typing would be ignored, as the shortcuts treeview doesn't
   * support interactive search.  However, we'd rather focus the location entry
   * so that the user can type *there*.
   *
   * To preserve keyboard navigation in the shortcuts pane, we don't focus the
   * filename entry if one clicks on a shortcut; rather, we focus the entry only
   * if the user starts typing while the focus is in the shortcuts pane.
   */
  g_signal_connect_after (impl->browse_shortcuts_tree_view, "key-press-event",
			  G_CALLBACK (shortcuts_key_press_event_after_cb), impl);

  g_signal_connect (impl->browse_shortcuts_tree_view, "popup-menu",
		    G_CALLBACK (shortcuts_popup_menu_cb), impl);
  g_signal_connect (impl->browse_shortcuts_tree_view, "button-press-event",
		    G_CALLBACK (shortcuts_button_press_event_cb), impl);
  /* Accessible object name for the file chooser's shortcuts pane */
  batk_object_set_name (btk_widget_get_accessible (impl->browse_shortcuts_tree_view), _("Places"));

  btk_tree_view_set_model (BTK_TREE_VIEW (impl->browse_shortcuts_tree_view), impl->shortcuts_pane_filter_model);

  btk_tree_view_enable_model_drag_source (BTK_TREE_VIEW (impl->browse_shortcuts_tree_view),
					  BDK_BUTTON1_MASK,
					  tree_model_row_targets,
					  G_N_ELEMENTS (tree_model_row_targets),
					  BDK_ACTION_MOVE);

  btk_drag_dest_set (impl->browse_shortcuts_tree_view,
		     BTK_DEST_DEFAULT_ALL,
		     tree_model_row_targets,
		     G_N_ELEMENTS (tree_model_row_targets),
		     BDK_ACTION_COPY | BDK_ACTION_MOVE);
  btk_drag_dest_add_uri_targets (impl->browse_shortcuts_tree_view);

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_shortcuts_tree_view));
  btk_tree_selection_set_mode (selection, BTK_SELECTION_SINGLE);
  btk_tree_selection_set_select_function (selection,
					  shortcuts_select_func,
					  impl, NULL);

  g_signal_connect (selection, "changed",
		    G_CALLBACK (shortcuts_selection_changed_cb), impl);

  g_signal_connect (impl->browse_shortcuts_tree_view, "key-press-event",
		    G_CALLBACK (shortcuts_key_press_event_cb), impl);

  g_signal_connect (impl->browse_shortcuts_tree_view, "drag-begin",
		    G_CALLBACK (shortcuts_drag_begin_cb), impl);
  g_signal_connect (impl->browse_shortcuts_tree_view, "drag-end",
		    G_CALLBACK (shortcuts_drag_end_cb), impl);
  g_signal_connect (impl->browse_shortcuts_tree_view, "drag-data-delete",
		    G_CALLBACK (shortcuts_drag_data_delete_cb), impl);

  g_signal_connect (impl->browse_shortcuts_tree_view, "drag-leave",
		    G_CALLBACK (shortcuts_drag_leave_cb), impl);
  g_signal_connect (impl->browse_shortcuts_tree_view, "drag-motion",
		    G_CALLBACK (shortcuts_drag_motion_cb), impl);
  g_signal_connect (impl->browse_shortcuts_tree_view, "drag-drop",
		    G_CALLBACK (shortcuts_drag_drop_cb), impl);
  g_signal_connect (impl->browse_shortcuts_tree_view, "drag-data-received",
		    G_CALLBACK (shortcuts_drag_data_received_cb), impl);

  /* Support tooltips */
  btk_widget_set_has_tooltip (impl->browse_shortcuts_tree_view, TRUE);
  g_signal_connect (impl->browse_shortcuts_tree_view, "query-tooltip",
		    G_CALLBACK (shortcuts_query_tooltip_cb), impl);

  btk_container_add (BTK_CONTAINER (swin), impl->browse_shortcuts_tree_view);
  btk_widget_show (impl->browse_shortcuts_tree_view);

  /* Column */

  column = btk_tree_view_column_new ();
  /* Column header for the file chooser's shortcuts pane */
  btk_tree_view_column_set_title (column, _("_Places"));

  renderer = btk_cell_renderer_pixbuf_new ();
  btk_tree_view_column_pack_start (column, renderer, FALSE);
  btk_tree_view_column_set_attributes (column, renderer,
				       "pixbuf", SHORTCUTS_COL_PIXBUF,
				       "visible", SHORTCUTS_COL_PIXBUF_VISIBLE,
				       NULL);

  renderer = btk_cell_renderer_text_new ();
  g_object_set (renderer,
                "width-chars", 12,
                "ellipsize", BANGO_ELLIPSIZE_END,
                NULL);
  g_signal_connect (renderer, "edited",
		    G_CALLBACK (shortcuts_edited), impl);
  g_signal_connect (renderer, "editing-canceled",
		    G_CALLBACK (shortcuts_editing_canceled), impl);
  btk_tree_view_column_pack_start (column, renderer, TRUE);
  btk_tree_view_column_set_attributes (column, renderer,
				       "text", SHORTCUTS_COL_NAME,
				       NULL);

  btk_tree_view_set_row_separator_func (BTK_TREE_VIEW (impl->browse_shortcuts_tree_view),
					shortcuts_row_separator_func,
					NULL, NULL);

  btk_tree_view_append_column (BTK_TREE_VIEW (impl->browse_shortcuts_tree_view), column);

  return swin;
}

/* Creates the widgets for the shortcuts/bookmarks pane */
static BtkWidget *
shortcuts_pane_create (BtkFileChooserDefault *impl,
		       BtkSizeGroup          *size_group)
{
  BtkWidget *vbox;
  BtkWidget *toolbar;
  BtkWidget *widget;
  GIcon *icon;

  vbox = btk_vbox_new (FALSE, 0);
  btk_widget_show (vbox);

  /* Shortcuts tree */

  widget = shortcuts_list_create (impl);

  btk_size_group_add_widget (size_group, widget);

  btk_box_pack_start (BTK_BOX (vbox), widget, TRUE, TRUE, 0);

  /* Box for buttons */

  toolbar = btk_toolbar_new ();
  btk_toolbar_set_style (BTK_TOOLBAR (toolbar), BTK_TOOLBAR_ICONS);

  btk_toolbar_set_icon_size (BTK_TOOLBAR (toolbar), BTK_ICON_SIZE_MENU);

  btk_box_pack_start (BTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  btk_widget_show (toolbar);

  /* Add bookmark button */
  icon = g_themed_icon_new_with_default_fallbacks ("list-add-symbolic");
  impl->browse_shortcuts_add_button = toolbutton_new (impl,
                                                      icon,
                                                      FALSE,
                                                      TRUE,
                                                      G_CALLBACK (add_bookmark_button_clicked_cb));
  g_object_unref (icon);

  btk_toolbar_insert (BTK_TOOLBAR (toolbar), BTK_TOOL_ITEM (impl->browse_shortcuts_add_button), 0);
  btk_widget_set_tooltip_text (impl->browse_shortcuts_add_button,
                               _("Add the selected folder to the Bookmarks"));

  /* Remove bookmark button */
  icon = g_themed_icon_new_with_default_fallbacks ("list-remove-symbolic");
  impl->browse_shortcuts_remove_button = toolbutton_new (impl,
                                                         icon,
                                                         FALSE,
                                                         TRUE,
                                                         G_CALLBACK (remove_bookmark_button_clicked_cb));
  g_object_unref (icon);
  btk_toolbar_insert (BTK_TOOLBAR (toolbar), BTK_TOOL_ITEM (impl->browse_shortcuts_remove_button), 1);
  btk_widget_set_tooltip_text (impl->browse_shortcuts_remove_button,
                               _("Remove the selected bookmark"));

  return vbox;
}

static gboolean
key_is_left_or_right (BdkEventKey *event)
{
  guint modifiers;

  modifiers = btk_accelerator_get_default_mod_mask ();

  return ((event->keyval == BDK_KEY_Right
	   || event->keyval == BDK_KEY_KP_Right
	   || event->keyval == BDK_KEY_Left
	   || event->keyval == BDK_KEY_KP_Left)
	  && (event->state & modifiers) == 0);
}

/* Handles key press events on the file list, so that we can trap Enter to
 * activate the default button on our own.  Also, checks to see if '/' has been
 * pressed.
 */
static gboolean
browse_files_key_press_event_cb (BtkWidget   *widget,
				 BdkEventKey *event,
				 gpointer     data)
{
  BtkFileChooserDefault *impl;

  impl = (BtkFileChooserDefault *) data;

  if ((event->keyval == BDK_KEY_slash
       || event->keyval == BDK_KEY_KP_Divide
#ifdef G_OS_UNIX
       || event->keyval == BDK_KEY_asciitilde
#endif
       ) && !(event->state & BTK_NO_TEXT_INPUT_MOD_MASK))
    {
      location_popup_handler (impl, event->string);
      return TRUE;
    }

  if (key_is_left_or_right (event))
    {
      btk_widget_grab_focus (impl->browse_shortcuts_tree_view);
      return TRUE;
    }

  if ((event->keyval == BDK_KEY_Return
       || event->keyval == BDK_KEY_ISO_Enter
       || event->keyval == BDK_KEY_KP_Enter
       || event->keyval == BDK_KEY_space
       || event->keyval == BDK_KEY_KP_Space)
      && !(event->state & btk_accelerator_get_default_mod_mask ())
      && !(impl->action == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER ||
	   impl->action == BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER))
    {
      BtkWindow *window;

      window = get_toplevel (widget);
      if (window)
        {
          BtkWidget *default_widget, *focus_widget;

          default_widget = btk_window_get_default_widget (window);
          focus_widget = btk_window_get_focus (window);

          if (widget != default_widget &&
              !(widget == focus_widget && (!default_widget || !btk_widget_get_sensitive (default_widget))))
	    {
	      btk_window_activate_default (window);

	      return TRUE;
	    }
        }
    }

  return FALSE;
}

/* Callback used when the file list's popup menu is detached */
static void
popup_menu_detach_cb (BtkWidget *attach_widget,
		      BtkMenu   *menu)
{
  BtkFileChooserDefault *impl;

  impl = g_object_get_data (B_OBJECT (attach_widget), "BtkFileChooserDefault");
  g_assert (BTK_IS_FILE_CHOOSER_DEFAULT (impl));

  impl->browse_files_popup_menu = NULL;
  impl->browse_files_popup_menu_add_shortcut_item = NULL;
  impl->browse_files_popup_menu_hidden_files_item = NULL;
}

/* Callback used when the "Add to Bookmarks" menu item is activated */
static void
add_to_shortcuts_cb (BtkMenuItem           *item,
		     BtkFileChooserDefault *impl)
{
  bookmarks_add_selected_folder (impl);
}

/* Callback used when the "Show Hidden Files" menu item is toggled */
static void
show_hidden_toggled_cb (BtkCheckMenuItem      *item,
			BtkFileChooserDefault *impl)
{
  g_object_set (impl,
		"show-hidden", btk_check_menu_item_get_active (item),
		NULL);
}

/* Callback used when the "Show Size Column" menu item is toggled */
static void
show_size_column_toggled_cb (BtkCheckMenuItem *item,
                             BtkFileChooserDefault *impl)
{
  impl->show_size_column = btk_check_menu_item_get_active (item);

  btk_tree_view_column_set_visible (impl->list_size_column,
                                    impl->show_size_column);
}

/* Shows an error dialog about not being able to select a dragged file */
static void
error_selecting_dragged_file_dialog (BtkFileChooserDefault *impl,
				     GFile                 *file,
				     GError                *error)
{
  error_dialog (impl,
		_("Could not select file"),
		file, error);
}

static void
file_list_drag_data_select_uris (BtkFileChooserDefault  *impl,
				 gchar                 **uris)
{
  int i;
  char *uri;
  BtkFileChooser *chooser = BTK_FILE_CHOOSER (impl);

  for (i = 1; uris[i]; i++)
    {
      GFile *file;
      GError *error = NULL;

      uri = uris[i];
      file = g_file_new_for_uri (uri);

      btk_file_chooser_default_select_file (chooser, file, &error);
      if (error)
	error_selecting_dragged_file_dialog (impl, file, error);

      g_object_unref (file);
    }
}

struct FileListDragData
{
  BtkFileChooserDefault *impl;
  gchar **uris;
  GFile *file;
};

static void
file_list_drag_data_received_get_info_cb (GCancellable *cancellable,
					  GFileInfo    *info,
					  const GError *error,
					  gpointer      user_data)
{
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  struct FileListDragData *data = user_data;
  BtkFileChooser *chooser = BTK_FILE_CHOOSER (data->impl);

  if (cancellable != data->impl->file_list_drag_data_received_cancellable)
    goto out;

  data->impl->file_list_drag_data_received_cancellable = NULL;

  if (cancelled || error)
    goto out;

  if ((data->impl->action == BTK_FILE_CHOOSER_ACTION_OPEN ||
       data->impl->action == BTK_FILE_CHOOSER_ACTION_SAVE) &&
      data->uris[1] == 0 && !error && _btk_file_info_consider_as_directory (info))
    change_folder_and_display_error (data->impl, data->file, FALSE);
  else
    {
      GError *error = NULL;

      btk_file_chooser_default_unselect_all (chooser);
      btk_file_chooser_default_select_file (chooser, data->file, &error);
      if (error)
	error_selecting_dragged_file_dialog (data->impl, data->file, error);
      else
	browse_files_center_selected_row (data->impl);
    }

  if (data->impl->select_multiple)
    file_list_drag_data_select_uris (data->impl, data->uris);

out:
  g_object_unref (data->impl);
  g_strfreev (data->uris);
  g_object_unref (data->file);
  g_free (data);

  g_object_unref (cancellable);
}

static void
file_list_drag_data_received_cb (BtkWidget        *widget,
                                 BdkDragContext   *context,
                                 gint              x,
                                 gint              y,
                                 BtkSelectionData *selection_data,
                                 guint             info,
                                 guint             time_,
                                 gpointer          data)
{
  BtkFileChooserDefault *impl;
  gchar **uris;
  char *uri;
  GFile *file;

  impl = BTK_FILE_CHOOSER_DEFAULT (data);

  /* Allow only drags from other widgets; see bug #533891. */
  if (btk_drag_get_source_widget (context) == widget)
    {
      g_signal_stop_emission_by_name (widget, "drag-data-received");
      return;
    }

  /* Parse the text/uri-list string, navigate to the first one */
  uris = btk_selection_data_get_uris (selection_data);
  if (uris && uris[0])
    {
      struct FileListDragData *data;

      uri = uris[0];
      file = g_file_new_for_uri (uri);

      data = g_new0 (struct FileListDragData, 1);
      data->impl = g_object_ref (impl);
      data->uris = uris;
      data->file = file;

      if (impl->file_list_drag_data_received_cancellable)
        g_cancellable_cancel (impl->file_list_drag_data_received_cancellable);

      impl->file_list_drag_data_received_cancellable =
        _btk_file_system_get_info (impl->file_system, file,
                                   "standard::type",
                                   file_list_drag_data_received_get_info_cb,
                                   data);
    }

  g_signal_stop_emission_by_name (widget, "drag-data-received");
}

/* Don't do anything with the drag_drop signal */
static gboolean
file_list_drag_drop_cb (BtkWidget             *widget,
			BdkDragContext        *context,
			gint                   x,
			gint                   y,
			guint                  time_,
			BtkFileChooserDefault *impl)
{
  g_signal_stop_emission_by_name (widget, "drag-drop");
  return TRUE;
}

/* Disable the normal tree drag motion handler, it makes it look like you're
   dropping the dragged item onto a tree item */
static gboolean
file_list_drag_motion_cb (BtkWidget             *widget,
                          BdkDragContext        *context,
                          gint                   x,
                          gint                   y,
                          guint                  time_,
                          BtkFileChooserDefault *impl)
{
  g_signal_stop_emission_by_name (widget, "drag-motion");
  return TRUE;
}

/* Constructs the popup menu for the file list if needed */
static void
file_list_build_popup_menu (BtkFileChooserDefault *impl)
{
  BtkWidget *item;

  if (impl->browse_files_popup_menu)
    return;

  impl->browse_files_popup_menu = btk_menu_new ();
  btk_menu_attach_to_widget (BTK_MENU (impl->browse_files_popup_menu),
			     impl->browse_files_tree_view,
			     popup_menu_detach_cb);

  item = btk_image_menu_item_new_with_mnemonic (_("_Add to Bookmarks"));
  impl->browse_files_popup_menu_add_shortcut_item = item;
  btk_image_menu_item_set_image (BTK_IMAGE_MENU_ITEM (item),
				 btk_image_new_from_stock (BTK_STOCK_ADD, BTK_ICON_SIZE_MENU));
  g_signal_connect (item, "activate",
		    G_CALLBACK (add_to_shortcuts_cb), impl);
  btk_widget_show (item);
  btk_menu_shell_append (BTK_MENU_SHELL (impl->browse_files_popup_menu), item);

  item = btk_separator_menu_item_new ();
  btk_widget_show (item);
  btk_menu_shell_append (BTK_MENU_SHELL (impl->browse_files_popup_menu), item);

  item = btk_check_menu_item_new_with_mnemonic (_("Show _Hidden Files"));
  impl->browse_files_popup_menu_hidden_files_item = item;
  g_signal_connect (item, "toggled",
		    G_CALLBACK (show_hidden_toggled_cb), impl);
  btk_widget_show (item);
  btk_menu_shell_append (BTK_MENU_SHELL (impl->browse_files_popup_menu), item);

  item = btk_check_menu_item_new_with_mnemonic (_("Show _Size Column"));
  impl->browse_files_popup_menu_size_column_item = item;
  g_signal_connect (item, "toggled",
                    G_CALLBACK (show_size_column_toggled_cb), impl);
  btk_widget_show (item);
  btk_menu_shell_append (BTK_MENU_SHELL (impl->browse_files_popup_menu), item);

  bookmarks_check_add_sensitivity (impl);
}

/* Updates the popup menu for the file list, creating it if necessary */
static void
file_list_update_popup_menu (BtkFileChooserDefault *impl)
{
  file_list_build_popup_menu (impl);

  /* FIXME - handle OPERATION_MODE_SEARCH and OPERATION_MODE_RECENT */

  /* The sensitivity of the Add to Bookmarks item is set in
   * bookmarks_check_add_sensitivity()
   */

  /* 'Show Hidden Files' */
  g_signal_handlers_block_by_func (impl->browse_files_popup_menu_hidden_files_item,
				   G_CALLBACK (show_hidden_toggled_cb), impl);
  btk_check_menu_item_set_active (BTK_CHECK_MENU_ITEM (impl->browse_files_popup_menu_hidden_files_item),
				  impl->show_hidden);
  g_signal_handlers_unblock_by_func (impl->browse_files_popup_menu_hidden_files_item,
				     G_CALLBACK (show_hidden_toggled_cb), impl);

  /* 'Show Size Column' */
  g_signal_handlers_block_by_func (impl->browse_files_popup_menu_size_column_item,
				   G_CALLBACK (show_size_column_toggled_cb), impl);
  btk_check_menu_item_set_active (BTK_CHECK_MENU_ITEM (impl->browse_files_popup_menu_size_column_item),
				  impl->show_size_column);
  g_signal_handlers_unblock_by_func (impl->browse_files_popup_menu_size_column_item,
				     G_CALLBACK (show_size_column_toggled_cb), impl);
}

static void
popup_position_func (BtkMenu   *menu,
                     gint      *x,
                     gint      *y,
                     gboolean  *push_in,
                     gpointer	user_data)
{
  BtkWidget *widget = BTK_WIDGET (user_data);
  BdkScreen *screen = btk_widget_get_screen (widget);
  BtkRequisition req;
  gint monitor_num;
  BdkRectangle monitor;

  g_return_if_fail (btk_widget_get_realized (widget));

  bdk_window_get_origin (widget->window, x, y);

  btk_widget_size_request (BTK_WIDGET (menu), &req);

  *x += (widget->allocation.width - req.width) / 2;
  *y += (widget->allocation.height - req.height) / 2;

  monitor_num = bdk_screen_get_monitor_at_point (screen, *x, *y);
  btk_menu_set_monitor (menu, monitor_num);
  bdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

  *x = CLAMP (*x, monitor.x, monitor.x + MAX (0, monitor.width - req.width));
  *y = CLAMP (*y, monitor.y, monitor.y + MAX (0, monitor.height - req.height));

  *push_in = FALSE;
}

static void
file_list_popup_menu (BtkFileChooserDefault *impl,
		      BdkEventButton        *event)
{
  file_list_update_popup_menu (impl);
  if (event)
    btk_menu_popup (BTK_MENU (impl->browse_files_popup_menu),
		    NULL, NULL, NULL, NULL,
		    event->button, event->time);
  else
    {
      btk_menu_popup (BTK_MENU (impl->browse_files_popup_menu),
		      NULL, NULL,
		      popup_position_func, impl->browse_files_tree_view,
		      0, BDK_CURRENT_TIME);
      btk_menu_shell_select_first (BTK_MENU_SHELL (impl->browse_files_popup_menu),
				   FALSE);
    }

}

/* Callback used for the BtkWidget::popup-menu signal of the file list */
static gboolean
list_popup_menu_cb (BtkWidget *widget,
		    BtkFileChooserDefault *impl)
{
  file_list_popup_menu (impl, NULL);
  return TRUE;
}

/* Callback used when a button is pressed on the file list.  We trap button 3 to
 * bring up a popup menu.
 */
static gboolean
list_button_press_event_cb (BtkWidget             *widget,
			    BdkEventButton        *event,
			    BtkFileChooserDefault *impl)
{
  static gboolean in_press = FALSE;

  if (in_press)
    return FALSE;

  if (!_btk_button_event_triggers_context_menu (event))
    return FALSE;

  in_press = TRUE;
  btk_widget_event (impl->browse_files_tree_view, (BdkEvent *) event);
  in_press = FALSE;

  file_list_popup_menu (impl, event);
  return TRUE;
}

typedef struct {
  OperationMode operation_mode;
  gint general_column;
  gint model_column;
} ColumnMap;

/* Sets the sort column IDs for the file list; needs to be done whenever we
 * change the model on the treeview.
 */
static void
file_list_set_sort_column_ids (BtkFileChooserDefault *impl)
{
  btk_tree_view_column_set_sort_column_id (impl->list_name_column, MODEL_COL_NAME);
  btk_tree_view_column_set_sort_column_id (impl->list_mtime_column, MODEL_COL_MTIME);
  btk_tree_view_column_set_sort_column_id (impl->list_size_column, MODEL_COL_SIZE);
}

static gboolean
file_list_query_tooltip_cb (BtkWidget  *widget,
                            gint        x,
                            gint        y,
                            gboolean    keyboard_tip,
                            BtkTooltip *tooltip,
                            gpointer    user_data)
{
  BtkFileChooserDefault *impl = user_data;
  BtkTreeModel *model;
  BtkTreePath *path;
  BtkTreeIter iter;
  GFile *file;
  char *parse_name;

  if (impl->operation_mode == OPERATION_MODE_BROWSE)
    return FALSE;


  if (!btk_tree_view_get_tooltip_context (BTK_TREE_VIEW (impl->browse_files_tree_view),
                                          &x, &y,
                                          keyboard_tip,
                                          &model, &path, &iter))
    return FALSE;
                                       
  btk_tree_model_get (model, &iter,
                      MODEL_COL_FILE, &file,
                      -1);

  if (file == NULL)
    {
      btk_tree_path_free (path);
      return FALSE;
    }

  parse_name = g_file_get_parse_name (file);
  btk_tooltip_set_text (tooltip, parse_name);
  btk_tree_view_set_tooltip_row (BTK_TREE_VIEW (impl->browse_files_tree_view),
                                 tooltip,
                                 path);

  g_free (parse_name);
  g_object_unref (file);
  btk_tree_path_free (path);

  return TRUE;
}

static void
set_icon_cell_renderer_fixed_size (BtkFileChooserDefault *impl, BtkCellRenderer *renderer)
{
  gint xpad, ypad;

  btk_cell_renderer_get_padding (renderer, &xpad, &ypad);
  btk_cell_renderer_set_fixed_size (renderer, 
                                    xpad * 2 + impl->icon_size,
                                    ypad * 2 + impl->icon_size);
}

/* Creates the widgets for the file list */
static BtkWidget *
create_file_list (BtkFileChooserDefault *impl)
{
  BtkWidget *swin;
  BtkTreeSelection *selection;
  BtkTreeViewColumn *column;
  BtkCellRenderer *renderer;

  /* Scrolled window */
  swin = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (swin),
				  BTK_POLICY_AUTOMATIC, BTK_POLICY_ALWAYS);
  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (swin),
				       BTK_SHADOW_IN);

  /* Tree/list view */

  impl->browse_files_tree_view = btk_tree_view_new ();
#ifdef PROFILE_FILE_CHOOSER
  g_object_set_data (B_OBJECT (impl->browse_files_tree_view), "fmq-name", "file_list");
#endif
  g_object_set_data (B_OBJECT (impl->browse_files_tree_view), I_("BtkFileChooserDefault"), impl);
  batk_object_set_name (btk_widget_get_accessible (impl->browse_files_tree_view), _("Files"));

  btk_tree_view_set_rules_hint (BTK_TREE_VIEW (impl->browse_files_tree_view), TRUE);
  btk_container_add (BTK_CONTAINER (swin), impl->browse_files_tree_view);

  btk_drag_dest_set (impl->browse_files_tree_view,
                     BTK_DEST_DEFAULT_ALL,
                     NULL, 0,
                     BDK_ACTION_COPY | BDK_ACTION_MOVE);
  btk_drag_dest_add_uri_targets (impl->browse_files_tree_view);
  
  g_signal_connect (impl->browse_files_tree_view, "row-activated",
		    G_CALLBACK (list_row_activated), impl);
  g_signal_connect (impl->browse_files_tree_view, "key-press-event",
    		    G_CALLBACK (browse_files_key_press_event_cb), impl);
  g_signal_connect (impl->browse_files_tree_view, "popup-menu",
		    G_CALLBACK (list_popup_menu_cb), impl);
  g_signal_connect (impl->browse_files_tree_view, "button-press-event",
		    G_CALLBACK (list_button_press_event_cb), impl);

  g_signal_connect (impl->browse_files_tree_view, "drag-data-received",
                    G_CALLBACK (file_list_drag_data_received_cb), impl);
  g_signal_connect (impl->browse_files_tree_view, "drag-drop",
                    G_CALLBACK (file_list_drag_drop_cb), impl);
  g_signal_connect (impl->browse_files_tree_view, "drag-motion",
                    G_CALLBACK (file_list_drag_motion_cb), impl);

  g_object_set (impl->browse_files_tree_view, "has-tooltip", TRUE, NULL);
  g_signal_connect (impl->browse_files_tree_view, "query-tooltip",
                    G_CALLBACK (file_list_query_tooltip_cb), impl);

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_files_tree_view));
  btk_tree_selection_set_select_function (selection,
					  list_select_func,
					  impl, NULL);
  btk_tree_view_enable_model_drag_source (BTK_TREE_VIEW (impl->browse_files_tree_view),
					  BDK_BUTTON1_MASK,
					  NULL, 0,
					  BDK_ACTION_COPY | BDK_ACTION_MOVE);
  btk_drag_source_add_uri_targets (impl->browse_files_tree_view);

  g_signal_connect (selection, "changed",
		    G_CALLBACK (list_selection_changed), impl);

  /* Keep the column order in sync with update_cell_renderer_attributes() */

  /* Filename column */

  impl->list_name_column = btk_tree_view_column_new ();
  btk_tree_view_column_set_expand (impl->list_name_column, TRUE);
  btk_tree_view_column_set_resizable (impl->list_name_column, TRUE);
  btk_tree_view_column_set_title (impl->list_name_column, _("Name"));

  renderer = btk_cell_renderer_pixbuf_new ();
  /* We set a fixed size so that we get an empty slot even if no icons are loaded yet */
  set_icon_cell_renderer_fixed_size (impl, renderer);
  btk_tree_view_column_pack_start (impl->list_name_column, renderer, FALSE);

  impl->list_name_renderer = btk_cell_renderer_text_new ();
  g_object_set (impl->list_name_renderer,
		"ellipsize", BANGO_ELLIPSIZE_END,
		NULL);
  g_signal_connect (impl->list_name_renderer, "edited",
		    G_CALLBACK (renderer_edited_cb), impl);
  g_signal_connect (impl->list_name_renderer, "editing-canceled",
		    G_CALLBACK (renderer_editing_canceled_cb), impl);
  btk_tree_view_column_pack_start (impl->list_name_column, impl->list_name_renderer, TRUE);

  btk_tree_view_append_column (BTK_TREE_VIEW (impl->browse_files_tree_view), impl->list_name_column);

  /* Size column */

  column = btk_tree_view_column_new ();
  btk_tree_view_column_set_resizable (column, TRUE);
  btk_tree_view_column_set_title (column, _("Size"));

  renderer = btk_cell_renderer_text_new ();
  g_object_set (renderer, 
                "alignment", BANGO_ALIGN_RIGHT,
                NULL);
  btk_tree_view_column_pack_start (column, renderer, TRUE); /* bug: it doesn't expand */
  btk_tree_view_append_column (BTK_TREE_VIEW (impl->browse_files_tree_view), column);
  impl->list_size_column = column;

  /* Modification time column */

  column = btk_tree_view_column_new ();
  btk_tree_view_column_set_resizable (column, TRUE);
  btk_tree_view_column_set_title (column, _("Modified"));

  renderer = btk_cell_renderer_text_new ();
  btk_tree_view_column_pack_start (column, renderer, TRUE);
  btk_tree_view_append_column (BTK_TREE_VIEW (impl->browse_files_tree_view), column);
  impl->list_mtime_column = column;
  
  file_list_set_sort_column_ids (impl);
  update_cell_renderer_attributes (impl);

  btk_widget_show_all (swin);

  return swin;
}

/* Creates the widgets for the files/folders pane */
static BtkWidget *
file_pane_create (BtkFileChooserDefault *impl,
		  BtkSizeGroup          *size_group)
{
  BtkWidget *vbox;
  BtkWidget *hbox;
  BtkWidget *widget;

  vbox = btk_vbox_new (FALSE, 6);
  btk_widget_show (vbox);

  /* Box for lists and preview */

  hbox = btk_hbox_new (FALSE, PREVIEW_HBOX_SPACING);
  btk_box_pack_start (BTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  btk_widget_show (hbox);

  /* File list */

  widget = create_file_list (impl);
  btk_box_pack_start (BTK_BOX (hbox), widget, TRUE, TRUE, 0);
  btk_size_group_add_widget (size_group, widget);

  /* Preview */

  impl->preview_box = btk_vbox_new (FALSE, 12);
  btk_box_pack_start (BTK_BOX (hbox), impl->preview_box, FALSE, FALSE, 0);
  /* Don't show preview box initially */

  /* Filter combo */

  impl->filter_combo_hbox = btk_hbox_new (FALSE, 12);

  widget = filter_create (impl);

  btk_widget_show (widget);
  btk_box_pack_end (BTK_BOX (impl->filter_combo_hbox), widget, FALSE, FALSE, 0);

  btk_box_pack_end (BTK_BOX (vbox), impl->filter_combo_hbox, FALSE, FALSE, 0);

  return vbox;
}

static void
location_entry_create (BtkFileChooserDefault *impl)
{
  if (!impl->location_entry)
    impl->location_entry = _btk_file_chooser_entry_new (TRUE);

  _btk_file_chooser_entry_set_local_only (BTK_FILE_CHOOSER_ENTRY (impl->location_entry), impl->local_only);
  _btk_file_chooser_entry_set_action (BTK_FILE_CHOOSER_ENTRY (impl->location_entry), impl->action);
  btk_entry_set_width_chars (BTK_ENTRY (impl->location_entry), 45);
  btk_entry_set_activates_default (BTK_ENTRY (impl->location_entry), TRUE);
}

/* Creates the widgets specific to Save mode */
static void
save_widgets_create (BtkFileChooserDefault *impl)
{
  BtkWidget *vbox;
  BtkWidget *widget;

  if (impl->save_widgets != NULL)
    return;

  location_switch_to_path_bar (impl);

  vbox = btk_vbox_new (FALSE, 12);

  impl->save_widgets_table = btk_table_new (2, 2, FALSE);
  btk_box_pack_start (BTK_BOX (vbox), impl->save_widgets_table, FALSE, FALSE, 0);
  btk_widget_show (impl->save_widgets_table);
  btk_table_set_row_spacings (BTK_TABLE (impl->save_widgets_table), 12);
  btk_table_set_col_spacings (BTK_TABLE (impl->save_widgets_table), 12);

  /* Label */

  widget = btk_label_new_with_mnemonic (_("_Name:"));
  btk_misc_set_alignment (BTK_MISC (widget), 0.0, 0.5);
  btk_table_attach (BTK_TABLE (impl->save_widgets_table), widget,
		    0, 1, 0, 1,
		    BTK_FILL, BTK_FILL,
		    0, 0);
  btk_widget_show (widget);

  /* Location entry */

  location_entry_create (impl);
  btk_table_attach (BTK_TABLE (impl->save_widgets_table), impl->location_entry,
		    1, 2, 0, 1,
		    BTK_EXPAND | BTK_FILL, 0,
		    0, 0);
  btk_widget_show (impl->location_entry);
  btk_label_set_mnemonic_widget (BTK_LABEL (widget), impl->location_entry);

  /* Folder combo */
  impl->save_folder_label = btk_label_new (NULL);
  btk_misc_set_alignment (BTK_MISC (impl->save_folder_label), 0.0, 0.5);
  btk_table_attach (BTK_TABLE (impl->save_widgets_table), impl->save_folder_label,
		    0, 1, 1, 2,
		    BTK_FILL, BTK_FILL,
		    0, 0);
  btk_widget_show (impl->save_folder_label);

  impl->save_widgets = vbox;
  btk_box_pack_start (BTK_BOX (impl), impl->save_widgets, FALSE, FALSE, 0);
  btk_box_reorder_child (BTK_BOX (impl), impl->save_widgets, 0);
  btk_widget_show (impl->save_widgets);
}

/* Destroys the widgets specific to Save mode */
static void
save_widgets_destroy (BtkFileChooserDefault *impl)
{
  if (impl->save_widgets == NULL)
    return;

  btk_widget_destroy (impl->save_widgets);
  impl->save_widgets = NULL;
  impl->save_widgets_table = NULL;
  impl->location_entry = NULL;
  impl->save_folder_label = NULL;
}

/* Turns on the path bar widget.  Can be called even if we are already in that
 * mode.
 */
static void
location_switch_to_path_bar (BtkFileChooserDefault *impl)
{
  if (impl->location_entry)
    {
      btk_widget_destroy (impl->location_entry);
      impl->location_entry = NULL;
    }

  btk_widget_hide (impl->location_entry_box);
}

/* Turns on the location entry.  Can be called even if we are already in that
 * mode.
 */
static void
location_switch_to_filename_entry (BtkFileChooserDefault *impl)
{
  /* when in search or recent files mode, we are not showing the
   * location_entry_box container, so there's no point in switching
   * to it.
   */
  if (impl->operation_mode == OPERATION_MODE_SEARCH ||
      impl->operation_mode == OPERATION_MODE_RECENT)
    return;

  if (impl->location_entry)
    {
      btk_widget_destroy (impl->location_entry);
      impl->location_entry = NULL;
    }

  /* Box */

  btk_widget_show (impl->location_entry_box);

  /* Entry */

  location_entry_create (impl);
  btk_box_pack_start (BTK_BOX (impl->location_entry_box), impl->location_entry, TRUE, TRUE, 0);
  btk_label_set_mnemonic_widget (BTK_LABEL (impl->location_label), impl->location_entry);

  /* Configure the entry */

  _btk_file_chooser_entry_set_base_folder (BTK_FILE_CHOOSER_ENTRY (impl->location_entry), impl->current_folder);

  /* Done */

  btk_widget_show (impl->location_entry);
  btk_widget_grab_focus (impl->location_entry);
}

/* Sets a new location mode.  set_buttons determines whether the toggle button
 * for the mode will also be changed.
 */
static void
location_mode_set (BtkFileChooserDefault *impl,
		   LocationMode new_mode,
		   gboolean set_button)
{
  if (impl->action == BTK_FILE_CHOOSER_ACTION_OPEN ||
      impl->action == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)
    {
      BtkWindow *toplevel;
      BtkWidget *current_focus;
      gboolean button_active;
      gboolean switch_to_file_list;

      switch (new_mode)
	{
	case LOCATION_MODE_PATH_BAR:
	  button_active = FALSE;

	  /* The location_entry will disappear when we switch to path bar mode.  So,
	   * we'll focus the file list in that case, to avoid having a window with
	   * no focused widget.
	   */
	  toplevel = get_toplevel (BTK_WIDGET (impl));
	  switch_to_file_list = FALSE;
	  if (toplevel)
	    {
	      current_focus = btk_window_get_focus (toplevel);
	      if (!current_focus || current_focus == impl->location_entry)
		switch_to_file_list = TRUE;
	    }

	  location_switch_to_path_bar (impl);

	  if (switch_to_file_list)
	    btk_widget_grab_focus (impl->browse_files_tree_view);

	  break;

	case LOCATION_MODE_FILENAME_ENTRY:
	  button_active = TRUE;
	  location_switch_to_filename_entry (impl);
	  break;

	default:
	  g_assert_not_reached ();
	  return;
	}

      if (set_button)
	{
	  g_signal_handlers_block_by_func (impl->location_button,
					   G_CALLBACK (location_button_toggled_cb), impl);

	  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (impl->location_button), button_active);

	  g_signal_handlers_unblock_by_func (impl->location_button,
					     G_CALLBACK (location_button_toggled_cb), impl);
	}
    }

  impl->location_mode = new_mode;
}

static void
location_toggle_popup_handler (BtkFileChooserDefault *impl)
{
  /* when in search or recent files mode, we are not showing the
   * location_entry_box container, so there's no point in switching
   * to it.
   */
  if (impl->operation_mode == OPERATION_MODE_SEARCH ||
      impl->operation_mode == OPERATION_MODE_RECENT)
    return;

  /* If the file entry is not visible, show it.
   * If it is visible, turn it off only if it is focused.  Otherwise, switch to the entry.
   */
  if (impl->location_mode == LOCATION_MODE_PATH_BAR)
    {
      location_mode_set (impl, LOCATION_MODE_FILENAME_ENTRY, TRUE);
    }
  else if (impl->location_mode == LOCATION_MODE_FILENAME_ENTRY)
    {
      if (btk_widget_has_focus (impl->location_entry))
        {
          location_mode_set (impl, LOCATION_MODE_PATH_BAR, TRUE);
        }
      else
        {
          btk_widget_grab_focus (impl->location_entry);
        }
    }
}

/* Callback used when one of the location mode buttons is toggled */
static void
location_button_toggled_cb (BtkToggleButton *toggle,
			    BtkFileChooserDefault *impl)
{
  gboolean is_active;
  LocationMode new_mode;

  is_active = btk_toggle_button_get_active (toggle);

  if (is_active)
    {
      g_assert (impl->location_mode == LOCATION_MODE_PATH_BAR);
      new_mode = LOCATION_MODE_FILENAME_ENTRY;
    }
  else
    {
      g_assert (impl->location_mode == LOCATION_MODE_FILENAME_ENTRY);
      new_mode = LOCATION_MODE_PATH_BAR;
    }

  location_mode_set (impl, new_mode, FALSE);
}

/* Creates a toggle button for the location entry. */
static void
location_button_create (BtkFileChooserDefault *impl)
{
  BtkWidget *image;
  const char *str;

  image = btk_image_new_from_stock (BTK_STOCK_EDIT, BTK_ICON_SIZE_BUTTON);
  btk_widget_show (image);

  impl->location_button = g_object_new (BTK_TYPE_TOGGLE_BUTTON,
					"image", image,
					NULL);

  g_signal_connect (impl->location_button, "toggled",
		    G_CALLBACK (location_button_toggled_cb), impl);

  str = _("Type a file name");

  btk_widget_set_tooltip_text (impl->location_button, str);
  batk_object_set_name (btk_widget_get_accessible (impl->location_button), str);
}

typedef enum {
  PATH_BAR_FOLDER_PATH,
  PATH_BAR_SELECT_A_FOLDER,
  PATH_BAR_ERROR_NO_FILENAME,
  PATH_BAR_ERROR_NO_FOLDER,
  PATH_BAR_RECENTLY_USED,
  PATH_BAR_SEARCH
} PathBarMode;

/* Creates the info bar for informational messages or warnings, with its icon and label */
static void
info_bar_create (BtkFileChooserDefault *impl)
{
  BtkWidget *content_area;

  impl->browse_select_a_folder_info_bar = btk_info_bar_new ();
  impl->browse_select_a_folder_icon = btk_image_new_from_stock (BTK_STOCK_DIRECTORY, BTK_ICON_SIZE_MENU);
  impl->browse_select_a_folder_label = btk_label_new (NULL);

  content_area = btk_info_bar_get_content_area (BTK_INFO_BAR (impl->browse_select_a_folder_info_bar));

  btk_box_pack_start (BTK_BOX (content_area), impl->browse_select_a_folder_icon, FALSE, FALSE, 0);
  btk_box_pack_start (BTK_BOX (content_area), impl->browse_select_a_folder_label, FALSE, FALSE, 0);

  btk_widget_show (impl->browse_select_a_folder_icon);
  btk_widget_show (impl->browse_select_a_folder_label);
}

/* Sets the info bar to show the appropriate informational or warning message */
static void
info_bar_set (BtkFileChooserDefault *impl, PathBarMode mode)
{
  char *str;
  gboolean free_str;
  BtkMessageType message_type;

  free_str = FALSE;

  switch (mode)
    {
    case PATH_BAR_SELECT_A_FOLDER:
      str = g_strconcat ("<i>", _("Please select a folder below"), "</i>", NULL);
      free_str = TRUE;
      message_type = BTK_MESSAGE_OTHER;
      break;

    case PATH_BAR_ERROR_NO_FILENAME:
      str = _("Please type a file name");
      message_type = BTK_MESSAGE_WARNING;
      break;

    case PATH_BAR_ERROR_NO_FOLDER:
      str = _("Please select a folder below");
      message_type = BTK_MESSAGE_WARNING;
      break;

    default:
      g_assert_not_reached ();
      return;
    }

  btk_info_bar_set_message_type (BTK_INFO_BAR (impl->browse_select_a_folder_info_bar), message_type);
  btk_image_set_from_stock (BTK_IMAGE (impl->browse_select_a_folder_icon),
			    (message_type == BTK_MESSAGE_WARNING) ? BTK_STOCK_DIALOG_WARNING : BTK_STOCK_DIRECTORY,
			    BTK_ICON_SIZE_MENU);
  btk_label_set_markup (BTK_LABEL (impl->browse_select_a_folder_label), str);

  if (free_str)
    g_free (str);
}

/* Creates the icon and label used to show that the file chooser is in Search or Recently-used mode */
static void
special_mode_widgets_create (BtkFileChooserDefault *impl)
{
  impl->browse_special_mode_icon = btk_image_new ();
  btk_size_group_add_widget (impl->browse_path_bar_size_group, impl->browse_special_mode_icon);
  btk_box_pack_start (BTK_BOX (impl->browse_path_bar_hbox), impl->browse_special_mode_icon, FALSE, FALSE, 0);

  impl->browse_special_mode_label = btk_label_new (NULL);
  btk_size_group_add_widget (impl->browse_path_bar_size_group, impl->browse_special_mode_label);
  btk_box_pack_start (BTK_BOX (impl->browse_path_bar_hbox), impl->browse_special_mode_label, FALSE, FALSE, 0);
}

/* Creates the path bar's container and eveyrthing that goes in it: location button, pathbar, info bar, and Create Folder button */
static void
path_bar_widgets_create (BtkFileChooserDefault *impl)
{
  /* Location widgets - note browse_path_bar_hbox is packed in the right place until switch_path_bar() */
  impl->browse_path_bar_hbox = btk_hbox_new (FALSE, 12);
  btk_widget_show (impl->browse_path_bar_hbox);

  /* Size group that allows the path bar to be the same size between modes */
  impl->browse_path_bar_size_group = btk_size_group_new (BTK_SIZE_GROUP_VERTICAL);
  btk_size_group_set_ignore_hidden (impl->browse_path_bar_size_group, FALSE);

  /* Location button */
  location_button_create (impl);
  btk_size_group_add_widget (impl->browse_path_bar_size_group, impl->location_button);
  btk_box_pack_start (BTK_BOX (impl->browse_path_bar_hbox), impl->location_button, FALSE, FALSE, 0);

  /* Path bar */
  impl->browse_path_bar = g_object_new (BTK_TYPE_PATH_BAR, NULL);
  _btk_path_bar_set_file_system (BTK_PATH_BAR (impl->browse_path_bar), impl->file_system);
  g_signal_connect (impl->browse_path_bar, "path-clicked", G_CALLBACK (path_bar_clicked), impl);

  btk_size_group_add_widget (impl->browse_path_bar_size_group, impl->browse_path_bar);
  btk_box_pack_start (BTK_BOX (impl->browse_path_bar_hbox), impl->browse_path_bar, TRUE, TRUE, 0);

  /* Info bar */
  info_bar_create (impl);
  btk_size_group_add_widget (impl->browse_path_bar_size_group, impl->browse_select_a_folder_info_bar);
  btk_box_pack_start (BTK_BOX (impl->browse_path_bar_hbox), impl->browse_select_a_folder_info_bar, TRUE, TRUE, 0);

  /* Widgets for special modes (recently-used in Open mode, Search mode) */
  special_mode_widgets_create (impl);

  /* Create Folder */
  impl->browse_new_folder_button = btk_button_new_with_mnemonic (_("Create Fo_lder"));
  g_signal_connect (impl->browse_new_folder_button, "clicked",
		    G_CALLBACK (new_folder_button_clicked), impl);
  btk_size_group_add_widget (impl->browse_path_bar_size_group, impl->browse_new_folder_button);
  btk_box_pack_end (BTK_BOX (impl->browse_path_bar_hbox), impl->browse_new_folder_button, FALSE, FALSE, 0);
}

/* Sets the path bar's mode to show a label, the actual folder path, or a
 * warning message.  You may call this function with PATH_BAR_ERROR_* directly
 * if the pathbar is already showing the widgets you expect; otherwise, call
 * path_bar_update() instead to set the appropriate widgets automatically.
 */
static void
path_bar_set_mode (BtkFileChooserDefault *impl, PathBarMode mode)
{
  gboolean path_bar_visible		= FALSE;
  gboolean special_mode_widgets_visible = FALSE;
  gboolean info_bar_visible		= FALSE;
  gboolean create_folder_visible        = FALSE;

  char *tmp;

  switch (mode)
    {
    case PATH_BAR_FOLDER_PATH:
      path_bar_visible = TRUE;
      break;

    case PATH_BAR_SELECT_A_FOLDER:
    case PATH_BAR_ERROR_NO_FILENAME:
    case PATH_BAR_ERROR_NO_FOLDER:
      info_bar_set (impl, mode);
      info_bar_visible = TRUE;
      break;

    case PATH_BAR_RECENTLY_USED:
      btk_image_set_from_icon_name (BTK_IMAGE (impl->browse_special_mode_icon), "document-open-recent", BTK_ICON_SIZE_BUTTON);

      tmp = g_strdup_printf ("<b>%s</b>", _("Recently Used"));
      btk_label_set_markup (BTK_LABEL (impl->browse_special_mode_label), tmp);
      g_free (tmp);

      special_mode_widgets_visible = TRUE;
      break;

    case PATH_BAR_SEARCH:
      btk_image_set_from_stock (BTK_IMAGE (impl->browse_special_mode_icon), BTK_STOCK_FIND, BTK_ICON_SIZE_BUTTON);

      tmp = g_strdup_printf ("<b>%s</b>", _("Search:"));
      btk_label_set_markup (BTK_LABEL (impl->browse_special_mode_label), tmp);
      g_free (tmp);

      special_mode_widgets_visible = TRUE;
      break;

    default:
      g_assert_not_reached ();
    }

  btk_widget_set_visible (impl->browse_path_bar,			path_bar_visible);
  btk_widget_set_visible (impl->browse_special_mode_icon,		special_mode_widgets_visible);
  btk_widget_set_visible (impl->browse_special_mode_label,		special_mode_widgets_visible);
  btk_widget_set_visible (impl->browse_select_a_folder_info_bar,	info_bar_visible);

  if (path_bar_visible)
    {
      if (impl->create_folders
	  && impl->action != BTK_FILE_CHOOSER_ACTION_OPEN
	  && impl->operation_mode != OPERATION_MODE_RECENT)
	create_folder_visible = TRUE;
    }

  btk_widget_set_visible (impl->browse_new_folder_button,		create_folder_visible);
}

/* Creates the main hpaned with the widgets shared by Open and Save mode */
static void
browse_widgets_create (BtkFileChooserDefault *impl)
{
  BtkWidget *hpaned;
  BtkWidget *widget;
  BtkSizeGroup *size_group;

  impl->browse_widgets_box = btk_vbox_new (FALSE, 12);
  btk_box_pack_start (BTK_BOX (impl), impl->browse_widgets_box, TRUE, TRUE, 0);
  btk_widget_show (impl->browse_widgets_box);

  impl->browse_header_box = btk_vbox_new (FALSE, 12);
  btk_box_pack_start (BTK_BOX (impl->browse_widgets_box), impl->browse_header_box, FALSE, FALSE, 0);
  btk_widget_show (impl->browse_header_box);

  /* Path bar, info bar, and their respective machinery - the browse_path_bar_hbox will get packed elsewhere */
  path_bar_widgets_create (impl);

  /* Box for the location label and entry */

  impl->location_entry_box = btk_hbox_new (FALSE, 12);
  btk_box_pack_start (BTK_BOX (impl->browse_header_box), impl->location_entry_box, FALSE, FALSE, 0);

  impl->location_label = btk_label_new_with_mnemonic (_("_Location:"));
  btk_widget_show (impl->location_label);
  btk_box_pack_start (BTK_BOX (impl->location_entry_box), impl->location_label, FALSE, FALSE, 0);

  /* size group is used by the scrolled windows of the panes */
  size_group = btk_size_group_new (BTK_SIZE_GROUP_VERTICAL);

  /* Paned widget */
  hpaned = btk_hpaned_new ();
  btk_widget_show (hpaned);
  btk_box_pack_start (BTK_BOX (impl->browse_widgets_box), hpaned, TRUE, TRUE, 0);

  widget = shortcuts_pane_create (impl, size_group);
  btk_paned_pack1 (BTK_PANED (hpaned), widget, FALSE, FALSE);
  widget = file_pane_create (impl, size_group);
  btk_paned_pack2 (BTK_PANED (hpaned), widget, TRUE, FALSE);
  btk_paned_set_position (BTK_PANED (hpaned), 148);
  g_object_unref (size_group);
}

static BObject*
btk_file_chooser_default_constructor (GType                  type,
				      guint                  n_construct_properties,
				      BObjectConstructParam *construct_params)
{
  BtkFileChooserDefault *impl;
  BObject *object;

  profile_start ("start", NULL);

  object = B_OBJECT_CLASS (_btk_file_chooser_default_parent_class)->constructor (type,
										n_construct_properties,
										construct_params);
  impl = BTK_FILE_CHOOSER_DEFAULT (object);

  g_assert (impl->file_system);

  btk_widget_push_composite_child ();

  /* Shortcuts model */
  shortcuts_model_create (impl);

  /* The browse widgets */
  browse_widgets_create (impl);

  /* Alignment to hold extra widget */
  impl->extra_align = btk_alignment_new (0.0, 0.5, 1.0, 1.0);
  btk_box_pack_start (BTK_BOX (impl), impl->extra_align, FALSE, FALSE, 0);

  btk_widget_pop_composite_child ();
  update_appearance (impl);

  profile_end ("end", NULL);

  return object;
}

/* Sets the extra_widget by packing it in the appropriate place */
static void
set_extra_widget (BtkFileChooserDefault *impl,
		  BtkWidget             *extra_widget)
{
  if (extra_widget)
    {
      g_object_ref (extra_widget);
      /* FIXME: is this right ? */
      btk_widget_show (extra_widget);
    }

  if (impl->extra_widget)
    {
      btk_container_remove (BTK_CONTAINER (impl->extra_align), impl->extra_widget);
      g_object_unref (impl->extra_widget);
    }

  impl->extra_widget = extra_widget;
  if (impl->extra_widget)
    {
      btk_container_add (BTK_CONTAINER (impl->extra_align), impl->extra_widget);
      btk_widget_show (impl->extra_align);
    }
  else
    btk_widget_hide (impl->extra_align);
}

static void
set_local_only (BtkFileChooserDefault *impl,
		gboolean               local_only)
{
  if (local_only != impl->local_only)
    {
      impl->local_only = local_only;

      if (impl->location_entry)
	_btk_file_chooser_entry_set_local_only (BTK_FILE_CHOOSER_ENTRY (impl->location_entry), local_only);

      if (impl->shortcuts_model && impl->file_system)
	{
	  shortcuts_add_volumes (impl);
	  shortcuts_add_bookmarks (impl);
	}

      if (local_only && impl->current_folder &&
           !_btk_file_has_native_path (impl->current_folder))
	{
	  /* If we are pointing to a non-local folder, make an effort to change
	   * back to a local folder, but it's really up to the app to not cause
	   * such a situation, so we ignore errors.
	   */
	  const gchar *home = g_get_home_dir ();
	  GFile *home_file;

	  if (home == NULL)
	    return;

	  home_file = g_file_new_for_path (home);

	  btk_file_chooser_set_current_folder_file (BTK_FILE_CHOOSER (impl), home_file, NULL);

	  g_object_unref (home_file);
	}
    }
}

static void
volumes_bookmarks_changed_cb (BtkFileSystem         *file_system,
			      BtkFileChooserDefault *impl)
{
  shortcuts_add_volumes (impl);
  shortcuts_add_bookmarks (impl);

  bookmarks_check_add_sensitivity (impl);
  bookmarks_check_remove_sensitivity (impl);
  shortcuts_check_popup_sensitivity (impl);
}

/* Sets the file chooser to multiple selection mode */
static void
set_select_multiple (BtkFileChooserDefault *impl,
		     gboolean               select_multiple,
		     gboolean               property_notify)
{
  BtkTreeSelection *selection;
  BtkSelectionMode mode;

  if (select_multiple == impl->select_multiple)
    return;

  mode = select_multiple ? BTK_SELECTION_MULTIPLE : BTK_SELECTION_BROWSE;

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_files_tree_view));
  btk_tree_selection_set_mode (selection, mode);

  btk_tree_view_set_rubber_banding (BTK_TREE_VIEW (impl->browse_files_tree_view), select_multiple);

  impl->select_multiple = select_multiple;
  g_object_notify (B_OBJECT (impl), "select-multiple");

  check_preview_change (impl);
}

static void
set_file_system_backend (BtkFileChooserDefault *impl)
{
  profile_start ("start for backend", "default");

  impl->file_system = _btk_file_system_new ();

  g_signal_connect (impl->file_system, "volumes-changed",
		    G_CALLBACK (volumes_bookmarks_changed_cb), impl);
  g_signal_connect (impl->file_system, "bookmarks-changed",
		    G_CALLBACK (volumes_bookmarks_changed_cb), impl);

  profile_end ("end", NULL);
}

static void
unset_file_system_backend (BtkFileChooserDefault *impl)
{
  g_signal_handlers_disconnect_by_func (impl->file_system,
					G_CALLBACK (volumes_bookmarks_changed_cb), impl);

  g_object_unref (impl->file_system);

  impl->file_system = NULL;
}

/* Saves the widgets around the pathbar so they can be reparented later
 * in the correct place.  This function must be called paired with
 * restore_path_bar().
 */
static void
save_path_bar (BtkFileChooserDefault *impl)
{
  BtkWidget *parent;

  g_object_ref (impl->browse_path_bar_hbox);

  parent = btk_widget_get_parent (impl->browse_path_bar_hbox);
  if (parent)
    btk_container_remove (BTK_CONTAINER (parent), impl->browse_path_bar_hbox);
}

/* Reparents the path bar and the "Create folder" button to the right place:
 * Above the file list in Open mode, or to the right of the "Save in folder:"
 * label in Save mode.  The save_path_bar() function must be called before this
 * one.
 */
static void
restore_path_bar (BtkFileChooserDefault *impl)
{
  if (impl->action == BTK_FILE_CHOOSER_ACTION_OPEN
      || impl->action == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)
    {
      btk_box_pack_start (BTK_BOX (impl->browse_header_box), impl->browse_path_bar_hbox, FALSE, FALSE, 0);
      btk_box_reorder_child (BTK_BOX (impl->browse_header_box), impl->browse_path_bar_hbox, 0);
    }
  else if (impl->action == BTK_FILE_CHOOSER_ACTION_SAVE
	   || impl->action == BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER)
    {
      btk_table_attach (BTK_TABLE (impl->save_widgets_table), impl->browse_path_bar_hbox,
			1, 2, 1, 2,
			BTK_EXPAND | BTK_FILL, BTK_FILL,
			0, 0);
      btk_label_set_mnemonic_widget (BTK_LABEL (impl->save_folder_label), impl->browse_path_bar);
    }
  else
    g_assert_not_reached ();

  g_object_unref (impl->browse_path_bar_hbox);
}

/* Takes the folder stored in a row in the recent_model, and puts it in the pathbar */
static void
put_recent_folder_in_pathbar (BtkFileChooserDefault *impl, BtkTreeIter *iter)
{
  GFile *file;

  btk_tree_model_get (BTK_TREE_MODEL (impl->recent_model), iter,
		      MODEL_COL_FILE, &file,
		      -1);
  _btk_path_bar_set_file (BTK_PATH_BAR (impl->browse_path_bar), file, FALSE, NULL); /* NULL-GError */
  g_object_unref (file);
}

/* Sets the pathbar in the appropriate mode according to the current operation mode and action.  This is the central function for
 * dealing with the pathbar's widgets; as long as impl->action and impl->operation_mode are set correctly, then calling this
 * function will update all the pathbar's widgets.
 */
static void
path_bar_update (BtkFileChooserDefault *impl)
{
  PathBarMode mode;

  switch (impl->operation_mode)
    {
    case OPERATION_MODE_BROWSE:
      mode = PATH_BAR_FOLDER_PATH;
      break;

    case OPERATION_MODE_RECENT:
      if (impl->action == BTK_FILE_CHOOSER_ACTION_SAVE)
	{
	  BtkTreeSelection *selection;
	  gboolean have_selected;
	  BtkTreeIter iter;

	  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_files_tree_view));

	  /* Save mode means single-selection mode, so the following is valid */
	  have_selected = btk_tree_selection_get_selected (selection, NULL, &iter);

	  if (have_selected)
	    {
	      mode = PATH_BAR_FOLDER_PATH;
	      put_recent_folder_in_pathbar (impl, &iter);
	    }
	  else
	    mode = PATH_BAR_SELECT_A_FOLDER;
	}
      else
	mode = PATH_BAR_RECENTLY_USED;

      break;

    case OPERATION_MODE_SEARCH:
      mode = PATH_BAR_SEARCH;
      break;

    default:
      g_assert_not_reached ();
      return;
    }

  path_bar_set_mode (impl, mode);
}

static void
operation_mode_discard_search_widgets (BtkFileChooserDefault *impl)
{
  if (impl->search_hbox)
    {
      btk_widget_destroy (impl->search_hbox);

      impl->search_hbox = NULL;
      impl->search_entry = NULL;
    }
}

/* Stops running operations like populating the browse model, searches, and the recent-files model */
static void
operation_mode_stop (BtkFileChooserDefault *impl, OperationMode mode)
{
  switch (mode)
    {
    case OPERATION_MODE_BROWSE:
      stop_loading_and_clear_list_model (impl, TRUE);
      break;

    case OPERATION_MODE_SEARCH:
      search_stop_searching (impl, FALSE);
      search_clear_model (impl, TRUE);

      operation_mode_discard_search_widgets (impl);
      break;

    case OPERATION_MODE_RECENT:
      recent_stop_loading (impl);
      recent_clear_model (impl, TRUE);
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
operation_mode_set_browse (BtkFileChooserDefault *impl)
{
  path_bar_update (impl);

  if (impl->action == BTK_FILE_CHOOSER_ACTION_OPEN ||
      impl->action == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)
    {
      btk_widget_show (impl->location_button);
      location_mode_set (impl, impl->location_mode, TRUE);

      if (impl->location_mode == LOCATION_MODE_FILENAME_ENTRY)
	btk_widget_show (impl->location_entry_box);
    }
}

static void
operation_mode_set_search (BtkFileChooserDefault *impl)
{
  g_assert (impl->search_hbox == NULL);
  g_assert (impl->search_entry == NULL);
  g_assert (impl->search_model == NULL);

  search_setup_widgets (impl);
}

static void
operation_mode_set_recent (BtkFileChooserDefault *impl)
{
  path_bar_update (impl);

  /* Hide the location widgets temporarily */
  if (impl->action == BTK_FILE_CHOOSER_ACTION_OPEN ||
      impl->action == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)
    {
      btk_widget_hide (impl->location_button);
      btk_widget_hide (impl->location_entry_box);
    }

  recent_start_loading (impl);
}

/* Sometimes we need to frob the selection in the shortcuts list manually */
static void
shortcuts_select_item_without_activating (BtkFileChooserDefault *impl, int pos)
{
  BtkTreeSelection *selection;
  BtkTreePath *path;

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_shortcuts_tree_view));

  g_signal_handlers_block_by_func (selection, G_CALLBACK (shortcuts_selection_changed_cb), impl);

  path = btk_tree_path_new_from_indices (pos, -1);
  btk_tree_selection_select_path (selection, path);
  btk_tree_path_free (path);

  g_signal_handlers_unblock_by_func (selection, G_CALLBACK (shortcuts_selection_changed_cb), impl);
}

static void
operation_mode_set (BtkFileChooserDefault *impl, OperationMode mode)
{
  ShortcutsIndex shortcut_to_select;

  operation_mode_stop (impl, impl->operation_mode);

  impl->operation_mode = mode;

  switch (impl->operation_mode)
    {
    case OPERATION_MODE_BROWSE:
      operation_mode_set_browse (impl);
      shortcut_to_select = SHORTCUTS_CURRENT_FOLDER;
      break;

    case OPERATION_MODE_SEARCH:
      operation_mode_set_search (impl);
      shortcut_to_select = SHORTCUTS_SEARCH;
      break;

    case OPERATION_MODE_RECENT:
      operation_mode_set_recent (impl);
      shortcut_to_select = SHORTCUTS_RECENT;
      break;

    default:
      g_assert_not_reached ();
      return;
    }

  if (shortcut_to_select != SHORTCUTS_CURRENT_FOLDER)
    shortcuts_select_item_without_activating (impl, shortcuts_get_index (impl, shortcut_to_select));
}

/* This function is basically a do_all function.
 *
 * It sets the visibility on all the widgets based on the current state, and
 * moves the custom_widget if needed.
 */
static void
update_appearance (BtkFileChooserDefault *impl)
{
  save_path_bar (impl);

  if (impl->action == BTK_FILE_CHOOSER_ACTION_SAVE ||
      impl->action == BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER)
    {
      const char *text;

      btk_widget_hide (impl->location_button);
      save_widgets_create (impl);

      if (impl->action == BTK_FILE_CHOOSER_ACTION_SAVE)
	text = _("Save in _folder:");
      else
	text = _("Create in _folder:");

      btk_label_set_text_with_mnemonic (BTK_LABEL (impl->save_folder_label), text);

      if (impl->select_multiple)
	{
	  g_warning ("Save mode cannot be set in conjunction with multiple selection mode.  "
		     "Re-setting to single selection mode.");
	  set_select_multiple (impl, FALSE, TRUE);
	}
    }
  else if (impl->action == BTK_FILE_CHOOSER_ACTION_OPEN ||
	   impl->action == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)
    {
      btk_widget_show (impl->location_button);
      save_widgets_destroy (impl);
      location_mode_set (impl, impl->location_mode, TRUE);
    }

  if (impl->location_entry)
    _btk_file_chooser_entry_set_action (BTK_FILE_CHOOSER_ENTRY (impl->location_entry), impl->action);

  restore_path_bar (impl);
  path_bar_update (impl);

  /* This *is* needed; we need to redraw the file list because the "sensitivity"
   * of files may change depending whether we are in a file or folder-only mode.
   */
  btk_widget_queue_draw (impl->browse_files_tree_view);

  emit_default_size_changed (impl);
}

static void
btk_file_chooser_default_set_property (BObject      *object,
				       guint         prop_id,
				       const BValue *value,
				       BParamSpec   *pspec)

{
  BtkFileChooserDefault *impl = BTK_FILE_CHOOSER_DEFAULT (object);

  switch (prop_id)
    {
    case BTK_FILE_CHOOSER_PROP_ACTION:
      {
	BtkFileChooserAction action = b_value_get_enum (value);

	if (action != impl->action)
	  {
	    btk_file_chooser_default_unselect_all (BTK_FILE_CHOOSER (impl));
	    
	    if ((action == BTK_FILE_CHOOSER_ACTION_SAVE ||
                 action == BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER)
		&& impl->select_multiple)
	      {
		g_warning ("Tried to change the file chooser action to SAVE or CREATE_FOLDER, but "
			   "this is not allowed in multiple selection mode.  Resetting the file chooser "
			   "to single selection mode.");
		set_select_multiple (impl, FALSE, TRUE);
	      }
	    impl->action = action;
            update_cell_renderer_attributes (impl);
	    update_appearance (impl);
	    settings_load (impl);
	  }
      }
      break;

    case BTK_FILE_CHOOSER_PROP_FILE_SYSTEM_BACKEND:
      /* Ignore property */
      break;

    case BTK_FILE_CHOOSER_PROP_FILTER:
      set_current_filter (impl, b_value_get_object (value));
      break;

    case BTK_FILE_CHOOSER_PROP_LOCAL_ONLY:
      set_local_only (impl, b_value_get_boolean (value));
      break;

    case BTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET:
      set_preview_widget (impl, b_value_get_object (value));
      break;

    case BTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET_ACTIVE:
      impl->preview_widget_active = b_value_get_boolean (value);
      update_preview_widget_visibility (impl);
      break;

    case BTK_FILE_CHOOSER_PROP_USE_PREVIEW_LABEL:
      impl->use_preview_label = b_value_get_boolean (value);
      update_preview_widget_visibility (impl);
      break;

    case BTK_FILE_CHOOSER_PROP_EXTRA_WIDGET:
      set_extra_widget (impl, b_value_get_object (value));
      break;

    case BTK_FILE_CHOOSER_PROP_SELECT_MULTIPLE:
      {
	gboolean select_multiple = b_value_get_boolean (value);
	if ((impl->action == BTK_FILE_CHOOSER_ACTION_SAVE ||
             impl->action == BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER)
	    && select_multiple)
	  {
	    g_warning ("Tried to set the file chooser to multiple selection mode, but this is "
		       "not allowed in SAVE or CREATE_FOLDER modes.  Ignoring the change and "
		       "leaving the file chooser in single selection mode.");
	    return;
	  }

	set_select_multiple (impl, select_multiple, FALSE);
      }
      break;

    case BTK_FILE_CHOOSER_PROP_SHOW_HIDDEN:
      {
	gboolean show_hidden = b_value_get_boolean (value);
	if (show_hidden != impl->show_hidden)
	  {
	    impl->show_hidden = show_hidden;

	    if (impl->browse_files_model)
	      _btk_file_system_model_set_show_hidden (impl->browse_files_model, show_hidden);
	  }
      }
      break;

    case BTK_FILE_CHOOSER_PROP_DO_OVERWRITE_CONFIRMATION:
      {
	gboolean do_overwrite_confirmation = b_value_get_boolean (value);
	impl->do_overwrite_confirmation = do_overwrite_confirmation;
      }
      break;

    case BTK_FILE_CHOOSER_PROP_CREATE_FOLDERS:
      {
        gboolean create_folders = b_value_get_boolean (value);
        impl->create_folders = create_folders;
        update_appearance (impl);
      }
      break;

    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_file_chooser_default_get_property (BObject    *object,
				       guint       prop_id,
				       BValue     *value,
				       BParamSpec *pspec)
{
  BtkFileChooserDefault *impl = BTK_FILE_CHOOSER_DEFAULT (object);

  switch (prop_id)
    {
    case BTK_FILE_CHOOSER_PROP_ACTION:
      b_value_set_enum (value, impl->action);
      break;

    case BTK_FILE_CHOOSER_PROP_FILTER:
      b_value_set_object (value, impl->current_filter);
      break;

    case BTK_FILE_CHOOSER_PROP_LOCAL_ONLY:
      b_value_set_boolean (value, impl->local_only);
      break;

    case BTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET:
      b_value_set_object (value, impl->preview_widget);
      break;

    case BTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET_ACTIVE:
      b_value_set_boolean (value, impl->preview_widget_active);
      break;

    case BTK_FILE_CHOOSER_PROP_USE_PREVIEW_LABEL:
      b_value_set_boolean (value, impl->use_preview_label);
      break;

    case BTK_FILE_CHOOSER_PROP_EXTRA_WIDGET:
      b_value_set_object (value, impl->extra_widget);
      break;

    case BTK_FILE_CHOOSER_PROP_SELECT_MULTIPLE:
      b_value_set_boolean (value, impl->select_multiple);
      break;

    case BTK_FILE_CHOOSER_PROP_SHOW_HIDDEN:
      b_value_set_boolean (value, impl->show_hidden);
      break;

    case BTK_FILE_CHOOSER_PROP_DO_OVERWRITE_CONFIRMATION:
      b_value_set_boolean (value, impl->do_overwrite_confirmation);
      break;

    case BTK_FILE_CHOOSER_PROP_CREATE_FOLDERS:
      b_value_set_boolean (value, impl->create_folders);
      break;

    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* This cancels everything that may be going on in the background. */
static void
cancel_all_operations (BtkFileChooserDefault *impl)
{
  GSList *l;

  pending_select_files_free (impl);

  if (impl->reload_icon_cancellables)
    {
      for (l = impl->reload_icon_cancellables; l; l = l->next)
        {
	  GCancellable *cancellable = G_CANCELLABLE (l->data);
	  g_cancellable_cancel (cancellable);
        }
      b_slist_free (impl->reload_icon_cancellables);
      impl->reload_icon_cancellables = NULL;
    }

  if (impl->loading_shortcuts)
    {
      for (l = impl->loading_shortcuts; l; l = l->next)
        {
	  GCancellable *cancellable = G_CANCELLABLE (l->data);
	  g_cancellable_cancel (cancellable);
        }
      b_slist_free (impl->loading_shortcuts);
      impl->loading_shortcuts = NULL;
    }

  if (impl->file_list_drag_data_received_cancellable)
    {
      g_cancellable_cancel (impl->file_list_drag_data_received_cancellable);
      impl->file_list_drag_data_received_cancellable = NULL;
    }

  if (impl->update_current_folder_cancellable)
    {
      g_cancellable_cancel (impl->update_current_folder_cancellable);
      impl->update_current_folder_cancellable = NULL;
    }

  if (impl->should_respond_get_info_cancellable)
    {
      g_cancellable_cancel (impl->should_respond_get_info_cancellable);
      impl->should_respond_get_info_cancellable = NULL;
    }

  if (impl->file_exists_get_info_cancellable)
    {
      g_cancellable_cancel (impl->file_exists_get_info_cancellable);
      impl->file_exists_get_info_cancellable = NULL;
    }

  if (impl->update_from_entry_cancellable)
    {
      g_cancellable_cancel (impl->update_from_entry_cancellable);
      impl->update_from_entry_cancellable = NULL;
    }

  if (impl->shortcuts_activate_iter_cancellable)
    {
      g_cancellable_cancel (impl->shortcuts_activate_iter_cancellable);
      impl->shortcuts_activate_iter_cancellable = NULL;
    }

  search_stop_searching (impl, TRUE);
  recent_stop_loading (impl);
}

/* Removes the settings signal handler.  It's safe to call multiple times */
static void
remove_settings_signal (BtkFileChooserDefault *impl,
			BdkScreen             *screen)
{
  if (impl->settings_signal_id)
    {
      BtkSettings *settings;

      settings = btk_settings_get_for_screen (screen);
      g_signal_handler_disconnect (settings,
				   impl->settings_signal_id);
      impl->settings_signal_id = 0;
    }
}

static void
btk_file_chooser_default_dispose (BObject *object)
{
  BtkFileChooserDefault *impl = (BtkFileChooserDefault *) object;

  cancel_all_operations (impl);

  if (impl->extra_widget)
    {
      g_object_unref (impl->extra_widget);
      impl->extra_widget = NULL;
    }

  remove_settings_signal (impl, btk_widget_get_screen (BTK_WIDGET (impl)));

  B_OBJECT_CLASS (_btk_file_chooser_default_parent_class)->dispose (object);
}

/* We override show-all since we have internal widgets that
 * shouldn't be shown when you call show_all(), like the filter
 * combo box.
 */
static void
btk_file_chooser_default_show_all (BtkWidget *widget)
{
  BtkFileChooserDefault *impl = (BtkFileChooserDefault *) widget;

  btk_widget_show (widget);

  if (impl->extra_widget)
    btk_widget_show_all (impl->extra_widget);
}

/* Handler for BtkWindow::set-focus; this is where we save the last-focused
 * widget on our toplevel.  See btk_file_chooser_default_hierarchy_changed()
 */
static void
toplevel_set_focus_cb (BtkWindow             *window,
		       BtkWidget             *focus,
		       BtkFileChooserDefault *impl)
{
  impl->toplevel_last_focus_widget = btk_window_get_focus (window);
}

/* Callback used when the toplevel widget unmaps itself.  We don't do this in
 * our own ::unmap() handler because in BTK+2, child widgets don't get
 * unmapped.
 */
static void
toplevel_unmap_cb (BtkWidget *widget,
		   BtkFileChooserDefault *impl)
{
  settings_save (impl);

  cancel_all_operations (impl);
  impl->reload_state = RELOAD_EMPTY;
}

/* We monitor the focus widget on our toplevel to be able to know which widget
 * was last focused at the time our "should_respond" method gets called.
 */
static void
btk_file_chooser_default_hierarchy_changed (BtkWidget *widget,
					    BtkWidget *previous_toplevel)
{
  BtkFileChooserDefault *impl;
  BtkWidget *toplevel;

  impl = BTK_FILE_CHOOSER_DEFAULT (widget);
  toplevel = btk_widget_get_toplevel (widget);

  if (previous_toplevel)
    {
      if (impl->toplevel_set_focus_id != 0)
	{
	  g_signal_handler_disconnect (previous_toplevel, impl->toplevel_set_focus_id);
	  impl->toplevel_set_focus_id = 0;
	  impl->toplevel_last_focus_widget = NULL;
	}

      if (impl->toplevel_unmapped_id != 0)
	{
	  g_signal_handler_disconnect (previous_toplevel, impl->toplevel_unmapped_id);
	  impl->toplevel_unmapped_id = 0;
	}
    }

  if (btk_widget_is_toplevel (toplevel))
    {
      g_assert (impl->toplevel_set_focus_id == 0);
      impl->toplevel_set_focus_id = g_signal_connect (toplevel, "set-focus",
						      G_CALLBACK (toplevel_set_focus_cb), impl);
      impl->toplevel_last_focus_widget = btk_window_get_focus (BTK_WINDOW (toplevel));

      g_assert (impl->toplevel_unmapped_id == 0);
      impl->toplevel_unmapped_id = g_signal_connect (toplevel, "unmap",
						     G_CALLBACK (toplevel_unmap_cb), impl);
    }
}

/* Changes the icons wherever it is needed */
static void
change_icon_theme (BtkFileChooserDefault *impl)
{
  BtkSettings *settings;
  gint width, height;
  BtkCellRenderer *renderer;
  GList *cells;

  profile_start ("start", NULL);

  settings = btk_settings_get_for_screen (btk_widget_get_screen (BTK_WIDGET (impl)));

  if (btk_icon_size_lookup_for_settings (settings, BTK_ICON_SIZE_MENU, &width, &height))
    impl->icon_size = MAX (width, height);
  else
    impl->icon_size = FALLBACK_ICON_SIZE;

  shortcuts_reload_icons (impl);
  /* the first cell in the first column is the icon column, and we have a fixed size there */
  cells = btk_cell_layout_get_cells (BTK_CELL_LAYOUT (
        btk_tree_view_get_column (BTK_TREE_VIEW (impl->browse_files_tree_view), 0)));
  renderer = BTK_CELL_RENDERER (cells->data);
  set_icon_cell_renderer_fixed_size (impl, renderer);
  g_list_free (cells);
  if (impl->browse_files_model)
    _btk_file_system_model_clear_cache (impl->browse_files_model, MODEL_COL_PIXBUF);
  btk_widget_queue_resize (impl->browse_files_tree_view);

  profile_end ("end", NULL);
}

/* Callback used when a BtkSettings value changes */
static void
settings_notify_cb (BObject               *object,
		    BParamSpec            *pspec,
		    BtkFileChooserDefault *impl)
{
  const char *name;

  profile_start ("start", NULL);

  name = g_param_spec_get_name (pspec);

  if (strcmp (name, "btk-icon-theme-name") == 0 ||
      strcmp (name, "btk-icon-sizes") == 0)
    change_icon_theme (impl);

  profile_end ("end", NULL);
}

/* Installs a signal handler for BtkSettings so that we can monitor changes in
 * the icon theme.
 */
static void
check_icon_theme (BtkFileChooserDefault *impl)
{
  BtkSettings *settings;

  profile_start ("start", NULL);

  if (impl->settings_signal_id)
    {
      profile_end ("end", NULL);
      return;
    }

  if (btk_widget_has_screen (BTK_WIDGET (impl)))
    {
      settings = btk_settings_get_for_screen (btk_widget_get_screen (BTK_WIDGET (impl)));
      impl->settings_signal_id = g_signal_connect (settings, "notify",
						   G_CALLBACK (settings_notify_cb), impl);

      change_icon_theme (impl);
    }

  profile_end ("end", NULL);
}

static void
btk_file_chooser_default_style_set (BtkWidget *widget,
				    BtkStyle  *previous_style)
{
  BtkFileChooserDefault *impl;

  profile_start ("start", NULL);

  impl = BTK_FILE_CHOOSER_DEFAULT (widget);

  profile_msg ("    parent class style_set start", NULL);
  BTK_WIDGET_CLASS (_btk_file_chooser_default_parent_class)->style_set (widget, previous_style);
  profile_msg ("    parent class style_set end", NULL);

  if (btk_widget_has_screen (BTK_WIDGET (impl)))
    change_icon_theme (impl);

  emit_default_size_changed (impl);

  profile_end ("end", NULL);
}

static void
btk_file_chooser_default_screen_changed (BtkWidget *widget,
					 BdkScreen *previous_screen)
{
  BtkFileChooserDefault *impl;

  profile_start ("start", NULL);

  impl = BTK_FILE_CHOOSER_DEFAULT (widget);

  if (BTK_WIDGET_CLASS (_btk_file_chooser_default_parent_class)->screen_changed)
    BTK_WIDGET_CLASS (_btk_file_chooser_default_parent_class)->screen_changed (widget, previous_screen);

  remove_settings_signal (impl, previous_screen);
  check_icon_theme (impl);

  emit_default_size_changed (impl);

  profile_end ("end", NULL);
}

static void
set_sort_column (BtkFileChooserDefault *impl)
{
  BtkTreeSortable *sortable;

  sortable = BTK_TREE_SORTABLE (btk_tree_view_get_model (BTK_TREE_VIEW (impl->browse_files_tree_view)));
  /* can happen when we're still populating the model */
  if (sortable == NULL)
    return;

  btk_tree_sortable_set_sort_column_id (sortable,
                                        impl->sort_column,
                                        impl->sort_order);
}

static void
settings_load (BtkFileChooserDefault *impl)
{
  BtkFileChooserSettings *settings;
  LocationMode location_mode;
  gboolean show_hidden;
  gboolean show_size_column;
  gint sort_column;
  BtkSortType sort_order;
  StartupMode startup_mode;

  settings = _btk_file_chooser_settings_new ();

  location_mode = _btk_file_chooser_settings_get_location_mode (settings);
  show_hidden = _btk_file_chooser_settings_get_show_hidden (settings);
  show_size_column = _btk_file_chooser_settings_get_show_size_column (settings);
  sort_column = _btk_file_chooser_settings_get_sort_column (settings);
  sort_order = _btk_file_chooser_settings_get_sort_order (settings);
  startup_mode = _btk_file_chooser_settings_get_startup_mode (settings);

  g_object_unref (settings);

  location_mode_set (impl, location_mode, TRUE);

  btk_file_chooser_set_show_hidden (BTK_FILE_CHOOSER (impl), show_hidden);

  impl->show_size_column = show_size_column;
  btk_tree_view_column_set_visible (impl->list_size_column, show_size_column);

  impl->sort_column = sort_column;
  impl->sort_order = sort_order;
  /* We don't call set_sort_column() here as the models may not have been
   * created yet.  The individual functions that create and set the models will
   * call set_sort_column() themselves.
   */

  impl->startup_mode = startup_mode;
}

static void
save_dialog_geometry (BtkFileChooserDefault *impl, BtkFileChooserSettings *settings)
{
  BtkWindow *toplevel;
  int x, y, width, height;

  toplevel = get_toplevel (BTK_WIDGET (impl));

  if (!(toplevel && BTK_IS_FILE_CHOOSER_DIALOG (toplevel)))
    return;

  btk_window_get_position (toplevel, &x, &y);
  btk_window_get_size (toplevel, &width, &height);

  _btk_file_chooser_settings_set_geometry (settings, x, y, width, height);
}

static void
settings_save (BtkFileChooserDefault *impl)
{
  BtkFileChooserSettings *settings;

  settings = _btk_file_chooser_settings_new ();

  /* All the other state */

  _btk_file_chooser_settings_set_location_mode (settings, impl->location_mode);
  _btk_file_chooser_settings_set_show_hidden (settings, btk_file_chooser_get_show_hidden (BTK_FILE_CHOOSER (impl)));
  _btk_file_chooser_settings_set_show_size_column (settings, impl->show_size_column);
  _btk_file_chooser_settings_set_sort_column (settings, impl->sort_column);
  _btk_file_chooser_settings_set_sort_order (settings, impl->sort_order);
  _btk_file_chooser_settings_set_startup_mode (settings, impl->startup_mode);

  save_dialog_geometry (impl, settings);

  /* NULL GError */
  _btk_file_chooser_settings_save (settings, NULL);

  g_object_unref (settings);
}

/* BtkWidget::realize method */
static void
btk_file_chooser_default_realize (BtkWidget *widget)
{
  BtkFileChooserDefault *impl;

  impl = BTK_FILE_CHOOSER_DEFAULT (widget);

  BTK_WIDGET_CLASS (_btk_file_chooser_default_parent_class)->realize (widget);

  emit_default_size_changed (impl);
}

/* Changes the current folder to $CWD */
static void
switch_to_cwd (BtkFileChooserDefault *impl)
{
  char *current_working_dir;

  current_working_dir = g_get_current_dir ();
  btk_file_chooser_set_current_folder (BTK_FILE_CHOOSER (impl), current_working_dir);
  g_free (current_working_dir);
}

/* Sets the file chooser to showing Recent Files or $CWD, depending on the
 * user's settings.
 */
static void
set_startup_mode (BtkFileChooserDefault *impl)
{
  switch (impl->startup_mode)
    {
    case STARTUP_MODE_RECENT:
      recent_shortcut_handler (impl);
      break;

    case STARTUP_MODE_CWD:
      switch_to_cwd (impl);
      break;

    default:
      g_assert_not_reached ();
    }
}

/* BtkWidget::map method */
static void
btk_file_chooser_default_map (BtkWidget *widget)
{
  BtkFileChooserDefault *impl;

  profile_start ("start", NULL);

  impl = BTK_FILE_CHOOSER_DEFAULT (widget);

  BTK_WIDGET_CLASS (_btk_file_chooser_default_parent_class)->map (widget);

  settings_load (impl);

  if (impl->operation_mode == OPERATION_MODE_BROWSE)
    {
      switch (impl->reload_state)
        {
        case RELOAD_EMPTY:
	  set_startup_mode (impl);
          break;
        
        case RELOAD_HAS_FOLDER:
          /* Nothing; we are already loading or loaded, so we
           * don't need to reload
           */
          break;

        default:
          g_assert_not_reached ();
      }
    }

  volumes_bookmarks_changed_cb (impl->file_system, impl);

  profile_end ("end", NULL);
}

static void
install_list_model_filter (BtkFileChooserDefault *impl)
{
  _btk_file_system_model_set_filter (impl->browse_files_model,
                                     impl->current_filter);
}

#define COMPARE_DIRECTORIES										       \
  BtkFileChooserDefault *impl = user_data;								       \
  BtkFileSystemModel *fs_model = BTK_FILE_SYSTEM_MODEL (model);                                                \
  gboolean dir_a, dir_b;										       \
													       \
  dir_a = b_value_get_boolean (_btk_file_system_model_get_value (fs_model, a, MODEL_COL_IS_FOLDER));           \
  dir_b = b_value_get_boolean (_btk_file_system_model_get_value (fs_model, b, MODEL_COL_IS_FOLDER));           \
													       \
  if (dir_a != dir_b)											       \
    return impl->list_sort_ascending ? (dir_a ? -1 : 1) : (dir_a ? 1 : -1) /* Directories *always* go first */

/* Sort callback for the filename column */
static gint
name_sort_func (BtkTreeModel *model,
		BtkTreeIter  *a,
		BtkTreeIter  *b,
		gpointer      user_data)
{
  COMPARE_DIRECTORIES;
  else
    {
      const char *key_a, *key_b;
      gint result;

      key_a = b_value_get_string (_btk_file_system_model_get_value (fs_model, a, MODEL_COL_NAME_COLLATED));
      key_b = b_value_get_string (_btk_file_system_model_get_value (fs_model, b, MODEL_COL_NAME_COLLATED));

      if (key_a && key_b)
        result = strcmp (key_a, key_b);
      else if (key_a)
        result = 1;
      else if (key_b)
        result = -1;
      else
        result = 0;

      return result;
    }
}

/* Sort callback for the size column */
static gint
size_sort_func (BtkTreeModel *model,
		BtkTreeIter  *a,
		BtkTreeIter  *b,
		gpointer      user_data)
{
  COMPARE_DIRECTORIES;
  else
    {
      gint64 size_a, size_b;

      size_a = b_value_get_int64 (_btk_file_system_model_get_value (fs_model, a, MODEL_COL_SIZE));
      size_b = b_value_get_int64 (_btk_file_system_model_get_value (fs_model, b, MODEL_COL_SIZE));

      return size_a < size_b ? -1 : (size_a == size_b ? 0 : 1);
    }
}

/* Sort callback for the mtime column */
static gint
mtime_sort_func (BtkTreeModel *model,
		 BtkTreeIter  *a,
		 BtkTreeIter  *b,
		 gpointer      user_data)
{
  COMPARE_DIRECTORIES;
  else
    {
      glong ta, tb;

      ta = b_value_get_long (_btk_file_system_model_get_value (fs_model, a, MODEL_COL_MTIME));
      tb = b_value_get_long (_btk_file_system_model_get_value (fs_model, b, MODEL_COL_MTIME));

      return ta < tb ? -1 : (ta == tb ? 0 : 1);
    }
}

/* Callback used when the sort column changes.  We cache the sort order for use
 * in name_sort_func().
 */
static void
list_sort_column_changed_cb (BtkTreeSortable       *sortable,
			     BtkFileChooserDefault *impl)
{
  gint sort_column_id;
  BtkSortType sort_type;

  if (btk_tree_sortable_get_sort_column_id (sortable, &sort_column_id, &sort_type))
    {
      impl->list_sort_ascending = (sort_type == BTK_SORT_ASCENDING);
      impl->sort_column = sort_column_id;
      impl->sort_order = sort_type;
    }
}

static void
set_busy_cursor (BtkFileChooserDefault *impl,
		 gboolean               busy)
{
  BtkWidget *widget;
  BtkWindow *toplevel;
  BdkDisplay *display;
  BdkCursor *cursor;

  toplevel = get_toplevel (BTK_WIDGET (impl));
  widget = BTK_WIDGET (toplevel);
  if (!toplevel || !btk_widget_get_realized (widget))
    return;

  display = btk_widget_get_display (widget);

  if (busy)
    cursor = bdk_cursor_new_for_display (display, BDK_WATCH);
  else
    cursor = NULL;

  bdk_window_set_cursor (btk_widget_get_window (widget), cursor);
  bdk_display_flush (display);

  if (cursor)
    bdk_cursor_unref (cursor);
}

/* Creates a sort model to wrap the file system model and sets it on the tree view */
static void
load_set_model (BtkFileChooserDefault *impl)
{
  profile_start ("start", NULL);

  g_assert (impl->browse_files_model != NULL);

  profile_msg ("    btk_tree_view_set_model start", NULL);
  btk_tree_view_set_model (BTK_TREE_VIEW (impl->browse_files_tree_view),
			   BTK_TREE_MODEL (impl->browse_files_model));
  btk_tree_view_columns_autosize (BTK_TREE_VIEW (impl->browse_files_tree_view));
  btk_tree_view_set_search_column (BTK_TREE_VIEW (impl->browse_files_tree_view),
				   MODEL_COL_NAME);
  file_list_set_sort_column_ids (impl);
  set_sort_column (impl);
  profile_msg ("    btk_tree_view_set_model end", NULL);
  impl->list_sort_ascending = TRUE;

  profile_end ("end", NULL);
}

/* Timeout callback used when the loading timer expires */
static gboolean
load_timeout_cb (gpointer data)
{
  BtkFileChooserDefault *impl;

  profile_start ("start", NULL);

  impl = BTK_FILE_CHOOSER_DEFAULT (data);
  g_assert (impl->load_state == LOAD_PRELOAD);
  g_assert (impl->load_timeout_id != 0);
  g_assert (impl->browse_files_model != NULL);

  impl->load_timeout_id = 0;
  impl->load_state = LOAD_LOADING;

  load_set_model (impl);

  profile_end ("end", NULL);

  return FALSE;
}

/* Sets up a new load timer for the model and switches to the LOAD_PRELOAD state */
static void
load_setup_timer (BtkFileChooserDefault *impl)
{
  g_assert (impl->load_timeout_id == 0);
  g_assert (impl->load_state != LOAD_PRELOAD);

  impl->load_timeout_id = bdk_threads_add_timeout (MAX_LOADING_TIME, load_timeout_cb, impl);
  impl->load_state = LOAD_PRELOAD;
}

/* Removes the load timeout; changes the impl->load_state to the specified value. */
static void
load_remove_timer (BtkFileChooserDefault *impl, LoadState new_load_state)
{
  if (impl->load_timeout_id != 0)
    {
      g_assert (impl->load_state == LOAD_PRELOAD);

      g_source_remove (impl->load_timeout_id);
      impl->load_timeout_id = 0;
    }
  else
    g_assert (impl->load_state == LOAD_EMPTY ||
	      impl->load_state == LOAD_LOADING ||
	      impl->load_state == LOAD_FINISHED);

  g_assert (new_load_state == LOAD_EMPTY ||
	    new_load_state == LOAD_LOADING ||
	    new_load_state == LOAD_FINISHED);
  impl->load_state = new_load_state;
}

/* Selects the first row in the file list */
static void
browse_files_select_first_row (BtkFileChooserDefault *impl)
{
  BtkTreePath *path;
  BtkTreeIter dummy_iter;
  BtkTreeModel *tree_model;

  tree_model = btk_tree_view_get_model (BTK_TREE_VIEW (impl->browse_files_tree_view));

  if (!tree_model)
    return;

  path = btk_tree_path_new_from_indices (0, -1);

  /* If the list is empty, do nothing. */
  if (btk_tree_model_get_iter (tree_model, &dummy_iter, path))
      btk_tree_view_set_cursor (BTK_TREE_VIEW (impl->browse_files_tree_view), path, NULL, FALSE);

  btk_tree_path_free (path);
}

struct center_selected_row_closure {
  BtkFileChooserDefault *impl;
  gboolean already_centered;
};

/* Callback used from btk_tree_selection_selected_foreach(); centers the
 * selected row in the tree view.
 */
static void
center_selected_row_foreach_cb (BtkTreeModel      *model,
				BtkTreePath       *path,
				BtkTreeIter       *iter,
				gpointer           data)
{
  struct center_selected_row_closure *closure;

  closure = data;
  if (closure->already_centered)
    return;

  btk_tree_view_scroll_to_cell (BTK_TREE_VIEW (closure->impl->browse_files_tree_view), path, NULL, TRUE, 0.5, 0.0);
  closure->already_centered = TRUE;
}

/* Centers the selected row in the tree view */
static void
browse_files_center_selected_row (BtkFileChooserDefault *impl)
{
  struct center_selected_row_closure closure;
  BtkTreeSelection *selection;

  closure.impl = impl;
  closure.already_centered = FALSE;

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_files_tree_view));
  btk_tree_selection_selected_foreach (selection, center_selected_row_foreach_cb, &closure);
}

static gboolean
show_and_select_files (BtkFileChooserDefault *impl,
		       GSList                *files)
{
  BtkTreeSelection *selection;
  BtkFileSystemModel *fsmodel;
  gboolean enabled_hidden, removed_filters;
  gboolean selected_a_file;
  GSList *walk;

  g_assert (impl->load_state == LOAD_FINISHED);
  g_assert (impl->browse_files_model != NULL);

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_files_tree_view));
  fsmodel = BTK_FILE_SYSTEM_MODEL (btk_tree_view_get_model (BTK_TREE_VIEW (impl->browse_files_tree_view)));

  g_assert (fsmodel == impl->browse_files_model);

  enabled_hidden = impl->show_hidden;
  removed_filters = (impl->current_filter == NULL);

  selected_a_file = FALSE;

  for (walk = files; walk; walk = walk->next)
    {
      GFile *file = walk->data;
      BtkTreeIter iter;

      /* Is it a hidden file? */

      if (!_btk_file_system_model_get_iter_for_file (fsmodel, &iter, file))
        continue;

      if (!_btk_file_system_model_iter_is_visible (fsmodel, &iter))
        {
          GFileInfo *info = _btk_file_system_model_get_info (fsmodel, &iter);

          if (!enabled_hidden &&
              (g_file_info_get_is_hidden (info) ||
               g_file_info_get_is_backup (info)))
            {
              g_object_set (impl, "show-hidden", TRUE, NULL);
              enabled_hidden = TRUE;
            }
        }

      /* Is it a filtered file? */

      if (!_btk_file_system_model_get_iter_for_file (fsmodel, &iter, file))
        continue; /* re-get the iter as it may change when the model refilters */

      if (!_btk_file_system_model_iter_is_visible (fsmodel, &iter))
        {
	  /* Maybe we should have a way to ask the fsmodel if it had filtered a file */
	  if (!removed_filters)
	    {
	      set_current_filter (impl, NULL);
	      removed_filters = TRUE;
	    }
	}

      /* Okay, can we select the file now? */
          
      if (!_btk_file_system_model_get_iter_for_file (fsmodel, &iter, file))
        continue;

      if (_btk_file_system_model_iter_is_visible (fsmodel, &iter))
        {
          BtkTreePath *path;

          btk_tree_selection_select_iter (selection, &iter);

          path = btk_tree_model_get_path (BTK_TREE_MODEL (fsmodel), &iter);
          btk_tree_view_set_cursor (BTK_TREE_VIEW (impl->browse_files_tree_view),
                                    path, NULL, FALSE);
          btk_tree_path_free (path);

          selected_a_file = TRUE;
        }
    }

  browse_files_center_selected_row (impl);

  return selected_a_file;
}

/* Processes the pending operation when a folder is finished loading */
static void
pending_select_files_process (BtkFileChooserDefault *impl)
{
  g_assert (impl->load_state == LOAD_FINISHED);
  g_assert (impl->browse_files_model != NULL);

  if (impl->pending_select_files)
    {
      show_and_select_files (impl, impl->pending_select_files);
      pending_select_files_free (impl);
      browse_files_center_selected_row (impl);
    }
  else
    {
      /* We only select the first row if the chooser is actually mapped ---
       * selecting the first row is to help the user when he is interacting with
       * the chooser, but sometimes a chooser works not on behalf of the user,
       * but rather on behalf of something else like BtkFileChooserButton.  In
       * that case, the chooser's selection should be what the caller expects,
       * as the user can't see that something else got selected.  See bug #165264.
       */
      if (impl->action == BTK_FILE_CHOOSER_ACTION_OPEN &&
          btk_widget_get_mapped (BTK_WIDGET (impl)))
	browse_files_select_first_row (impl);
    }

  g_assert (impl->pending_select_files == NULL);
}

static void
show_error_on_reading_current_folder (BtkFileChooserDefault *impl, GError *error)
{
  GFileInfo *info;
  char *msg;

  info = g_file_query_info (impl->current_folder,
			    G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
			    G_FILE_QUERY_INFO_NONE,
			    NULL,
			    NULL);
  if (info)
    {
      msg = g_strdup_printf (_("Could not read the contents of %s"), g_file_info_get_display_name (info));
      g_object_unref (info);
    }
  else
    msg = g_strdup (_("Could not read the contents of the folder"));

  error_message (impl, msg, error->message);
  g_free (msg);
}

/* Callback used when the file system model finishes loading */
static void
browse_files_model_finished_loading_cb (BtkFileSystemModel    *model,
                                        GError                *error,
					BtkFileChooserDefault *impl)
{
  profile_start ("start", NULL);

  if (error)
    show_error_on_reading_current_folder (impl, error);

  if (impl->load_state == LOAD_PRELOAD)
    {
      load_remove_timer (impl, LOAD_FINISHED);
      load_set_model (impl);
    }
  else if (impl->load_state == LOAD_LOADING)
    {
      /* Nothing */
    }
  else
    {
      /* We can't g_assert_not_reached(), as something other than us may have
       *  initiated a folder reload.  See #165556.
       */
      profile_end ("end", NULL);
      return;
    }

  g_assert (impl->load_timeout_id == 0);

  impl->load_state = LOAD_FINISHED;

  pending_select_files_process (impl);
  set_busy_cursor (impl, FALSE);
#ifdef PROFILE_FILE_CHOOSER
  access ("MARK: *** FINISHED LOADING", F_OK);
#endif

  profile_end ("end", NULL);
}

static void
stop_loading_and_clear_list_model (BtkFileChooserDefault *impl,
                                   gboolean remove_from_treeview)
{
  load_remove_timer (impl, LOAD_EMPTY);
  
  if (impl->browse_files_model)
    {
      g_object_unref (impl->browse_files_model);
      impl->browse_files_model = NULL;
    }

  if (remove_from_treeview)
    btk_tree_view_set_model (BTK_TREE_VIEW (impl->browse_files_tree_view), NULL);
}

static char *
my_g_format_time_for_display (glong secs)
{
  GDate mtime, now;
  gint days_diff;
  struct tm tm_mtime;
  time_t time_mtime, time_now;
  const gchar *format;
  gchar *locale_format = NULL;
  gchar buf[256];
  char *date_str = NULL;
#ifdef G_OS_WIN32
  const char *locale, *dot = NULL;
  gint64 codepage = -1;
  char charset[20];
#endif

  time_mtime = secs;

#ifdef HAVE_LOCALTIME_R
  localtime_r ((time_t *) &time_mtime, &tm_mtime);
#else
  {
    struct tm *ptm = localtime ((time_t *) &time_mtime);

    if (!ptm)
      {
        g_warning ("ptm != NULL failed");
        
        return g_strdup (_("Unknown"));
      }
    else
      memcpy ((void *) &tm_mtime, (void *) ptm, sizeof (struct tm));
  }
#endif /* HAVE_LOCALTIME_R */

  g_date_set_time_t (&mtime, time_mtime);
  time_now = time (NULL);
  g_date_set_time_t (&now, time_now);

  days_diff = g_date_get_julian (&now) - g_date_get_julian (&mtime);

  /* Translators: %H means "hours" and %M means "minutes" */
  if (days_diff == 0)
    format = _("%H:%M");
  else if (days_diff == 1)
    format = _("Yesterday at %H:%M");
  else
    {
      if (days_diff > 1 && days_diff < 7)
        format = "%A"; /* Days from last week */
      else
        format = "%x"; /* Any other date */
    }

#ifdef G_OS_WIN32
  /* g_locale_from_utf8() returns a string in the system
   * code-page, which is not always the same as that used by the C
   * library. For instance when running a BTK+ program with
   * LANG=ko on an English version of Windows, the system
   * code-page is 1252, but the code-page used by the C library is
   * 949. (It's BTK+ itself that sets the C library locale when it
   * notices the LANG environment variable. See btkmain.c The
   * Microsoft C library doesn't look at any locale environment
   * variables.) We need to pass strftime() a string in the C
   * library's code-page. See bug #509885.
   */
  locale = setlocale (LC_ALL, NULL);
  if (locale != NULL)
    dot = strchr (locale, '.');
  if (dot != NULL)
    {
      codepage = g_ascii_strtoll (dot+1, NULL, 10);
      
      /* All codepages should fit in 16 bits AFAIK */
      if (codepage > 0 && codepage < 65536)
        {
          sprintf (charset, "CP%u", (guint) codepage);
          locale_format = g_convert (format, -1, charset, "UTF-8", NULL, NULL, NULL);
        }
    }
#else
  locale_format = g_locale_from_utf8 (format, -1, NULL, NULL, NULL);
#endif
  if (locale_format != NULL &&
      strftime (buf, sizeof (buf), locale_format, &tm_mtime) != 0)
    {
#ifdef G_OS_WIN32
      /* As above but in opposite direction... */
      if (codepage > 0 && codepage < 65536)
        date_str = g_convert (buf, -1, "UTF-8", charset, NULL, NULL, NULL);
#else
      date_str = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
#endif
    }

  if (date_str == NULL)
    date_str = g_strdup (_("Unknown"));

  g_free (locale_format);
  return date_str;
}

static void
copy_attribute (GFileInfo *to, GFileInfo *from, const char *attribute)
{
  GFileAttributeType type;
  gpointer value;

  if (g_file_info_get_attribute_data (from, attribute, &type, &value, NULL))
    g_file_info_set_attribute (to, attribute, type, value);
}

static void
file_system_model_got_thumbnail (BObject *object, GAsyncResult *res, gpointer data)
{
  BtkFileSystemModel *model = data; /* might be unreffed if operation was cancelled */
  GFile *file = G_FILE (object);
  GFileInfo *queried, *info;
  BtkTreeIter iter;

  queried = g_file_query_info_finish (file, res, NULL);
  if (queried == NULL)
    return;

  BDK_THREADS_ENTER ();

  /* now we know model is valid */

  /* file was deleted */
  if (!_btk_file_system_model_get_iter_for_file (model, &iter, file))
    {
      g_object_unref (queried);
      BDK_THREADS_LEAVE ();
      return;
    }

  info = g_file_info_dup (_btk_file_system_model_get_info (model, &iter));

  copy_attribute (info, queried, G_FILE_ATTRIBUTE_THUMBNAIL_PATH);
  copy_attribute (info, queried, G_FILE_ATTRIBUTE_THUMBNAILING_FAILED);
  copy_attribute (info, queried, G_FILE_ATTRIBUTE_STANDARD_ICON);

  _btk_file_system_model_update_file (model, file, info);

  g_object_unref (info);
  g_object_unref (queried);

  BDK_THREADS_LEAVE ();
}

static gboolean
file_system_model_set (BtkFileSystemModel *model,
                       GFile              *file,
                       GFileInfo          *info,
                       int                 column,
                       BValue             *value,
                       gpointer            data)
{
  BtkFileChooserDefault *impl = data;
 
  switch (column)
    {
    case MODEL_COL_FILE:
      b_value_set_object (value, file);
      break;
    case MODEL_COL_NAME:
      if (info == NULL)
        b_value_set_string (value, DEFAULT_NEW_FOLDER_NAME);
      else 
        b_value_set_string (value, g_file_info_get_display_name (info));
      break;
    case MODEL_COL_NAME_COLLATED:
      if (info == NULL)
        b_value_take_string (value, g_utf8_collate_key_for_filename (DEFAULT_NEW_FOLDER_NAME, -1));
      else 
        b_value_take_string (value, g_utf8_collate_key_for_filename (g_file_info_get_display_name (info), -1));
      break;
    case MODEL_COL_IS_FOLDER:
      b_value_set_boolean (value, info == NULL || _btk_file_info_consider_as_directory (info));
      break;
    case MODEL_COL_IS_SENSITIVE:
      if (info)
        {
          gboolean sensitive = TRUE;

          if (impl->action != BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER &&
              impl->action != BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER)
            {
              sensitive = TRUE;
            }
          else if (!_btk_file_info_consider_as_directory (info))
            {
              sensitive = FALSE;
            }
          else
            {
              BtkTreeIter iter;
              if (!_btk_file_system_model_get_iter_for_file (model, &iter, file))
                g_assert_not_reached ();
              sensitive = !_btk_file_system_model_iter_is_filtered_out (model, &iter);
            }

          b_value_set_boolean (value, sensitive);
        }
      else
        b_value_set_boolean (value, TRUE);
      break;
    case MODEL_COL_PIXBUF:
      if (info)
        {
          if (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_STANDARD_ICON))
            {
              b_value_take_object (value, _btk_file_info_render_icon (info, BTK_WIDGET (impl), impl->icon_size));
            }
          else
            {
              BtkTreeModel *tree_model;
              BtkTreePath *path, *start, *end;
              BtkTreeIter iter;

              if (impl->browse_files_tree_view == NULL ||
                  g_file_info_has_attribute (info, "filechooser::queried"))
                return FALSE;

              tree_model = btk_tree_view_get_model (BTK_TREE_VIEW (impl->browse_files_tree_view));
              if (tree_model != BTK_TREE_MODEL (model))
                return FALSE;

              if (!_btk_file_system_model_get_iter_for_file (model,
                                                             &iter,
                                                             file))
                g_assert_not_reached ();
              if (!btk_tree_view_get_visible_range (BTK_TREE_VIEW (impl->browse_files_tree_view), &start, &end))
                return FALSE;
              path = btk_tree_model_get_path (tree_model, &iter);
              if (btk_tree_path_compare (start, path) != 1 &&
                  btk_tree_path_compare (path, end) != 1)
                {
                  g_file_info_set_attribute_boolean (info, "filechooser::queried", TRUE);
                  g_file_query_info_async (file,
                                           G_FILE_ATTRIBUTE_THUMBNAIL_PATH ","
                                           G_FILE_ATTRIBUTE_THUMBNAILING_FAILED ","
                                           G_FILE_ATTRIBUTE_STANDARD_ICON,
                                           G_FILE_QUERY_INFO_NONE,
                                           G_PRIORITY_DEFAULT,
                                           _btk_file_system_model_get_cancellable (model),
                                           file_system_model_got_thumbnail,
                                           model);
                }
              btk_tree_path_free (path);
              btk_tree_path_free (start);
              btk_tree_path_free (end);
              return FALSE;
            }
        }
      else
        b_value_set_object (value, NULL);
      break;
    case MODEL_COL_SIZE:
      b_value_set_int64 (value, info ? g_file_info_get_size (info) : 0);
      break;
    case MODEL_COL_SIZE_TEXT:
      if (info == NULL || _btk_file_info_consider_as_directory (info))
        b_value_set_string (value, NULL);
      else
#if BUNNYLIB_CHECK_VERSION(2,30,0)
        b_value_take_string (value, g_format_size (g_file_info_get_size (info)));
#else
        b_value_take_string (value, g_format_size_for_display (g_file_info_get_size (info)));
#endif
      break;
    case MODEL_COL_MTIME:
    case MODEL_COL_MTIME_TEXT:
      {
        GTimeVal tv;
        if (info == NULL)
          break;
        g_file_info_get_modification_time (info, &tv);
        if (column == MODEL_COL_MTIME)
          b_value_set_long (value, tv.tv_sec);
        else if (tv.tv_sec == 0)
          b_value_set_static_string (value, _("Unknown"));
        else
          b_value_take_string (value, my_g_format_time_for_display (tv.tv_sec));
        break;
      }
    case MODEL_COL_ELLIPSIZE:
      b_value_set_enum (value, info ? BANGO_ELLIPSIZE_END : BANGO_ELLIPSIZE_NONE);
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  return TRUE;
}

/* Gets rid of the old list model and creates a new one for the current folder */
static gboolean
set_list_model (BtkFileChooserDefault *impl,
		GError               **error)
{
  g_assert (impl->current_folder != NULL);

  profile_start ("start", NULL);

  stop_loading_and_clear_list_model (impl, TRUE);

  set_busy_cursor (impl, TRUE);

  impl->browse_files_model = 
    _btk_file_system_model_new_for_directory (impl->current_folder,
					      MODEL_ATTRIBUTES,
					      file_system_model_set,
					      impl,
					      MODEL_COLUMN_TYPES);

  _btk_file_system_model_set_show_hidden (impl->browse_files_model, impl->show_hidden);

  profile_msg ("    set sort function", NULL);
  btk_tree_sortable_set_sort_func (BTK_TREE_SORTABLE (impl->browse_files_model), MODEL_COL_NAME, name_sort_func, impl, NULL);
  btk_tree_sortable_set_sort_func (BTK_TREE_SORTABLE (impl->browse_files_model), MODEL_COL_SIZE, size_sort_func, impl, NULL);
  btk_tree_sortable_set_sort_func (BTK_TREE_SORTABLE (impl->browse_files_model), MODEL_COL_MTIME, mtime_sort_func, impl, NULL);
  btk_tree_sortable_set_default_sort_func (BTK_TREE_SORTABLE (impl->browse_files_model), NULL, NULL, NULL);
  set_sort_column (impl);
  impl->list_sort_ascending = TRUE;
  g_signal_connect (impl->browse_files_model, "sort-column-changed",
		    G_CALLBACK (list_sort_column_changed_cb), impl);

  load_setup_timer (impl); /* This changes the state to LOAD_PRELOAD */

  g_signal_connect (impl->browse_files_model, "finished-loading",
		    G_CALLBACK (browse_files_model_finished_loading_cb), impl);

  install_list_model_filter (impl);

  profile_end ("end", NULL);

  return TRUE;
}

struct update_chooser_entry_selected_foreach_closure {
  int num_selected;
  BtkTreeIter first_selected_iter;
};

static gint
compare_utf8_filenames (const gchar *a,
			const gchar *b)
{
  gchar *a_folded, *b_folded;
  gint retval;

  a_folded = g_utf8_strdown (a, -1);
  b_folded = g_utf8_strdown (b, -1);

  retval = strcmp (a_folded, b_folded);

  g_free (a_folded);
  g_free (b_folded);

  return retval;
}

static void
update_chooser_entry_selected_foreach (BtkTreeModel *model,
				       BtkTreePath *path,
				       BtkTreeIter *iter,
				       gpointer data)
{
  struct update_chooser_entry_selected_foreach_closure *closure;

  closure = data;
  closure->num_selected++;

  if (closure->num_selected == 1)
    closure->first_selected_iter = *iter;
}

static void
update_chooser_entry (BtkFileChooserDefault *impl)
{
  BtkTreeSelection *selection;
  struct update_chooser_entry_selected_foreach_closure closure;

  /* no need to update the file chooser's entry if there's no entry */
  if (impl->operation_mode == OPERATION_MODE_SEARCH ||
      !impl->location_entry)
    return;

  if (!(impl->action == BTK_FILE_CHOOSER_ACTION_SAVE
        || impl->action == BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER
        || ((impl->action == BTK_FILE_CHOOSER_ACTION_OPEN
             || impl->action == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)
            && impl->location_mode == LOCATION_MODE_FILENAME_ENTRY)))
    return;

  g_assert (impl->location_entry != NULL);

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_files_tree_view));
  closure.num_selected = 0;
  btk_tree_selection_selected_foreach (selection, update_chooser_entry_selected_foreach, &closure);

  if (closure.num_selected == 0)
    {
      if (impl->operation_mode == OPERATION_MODE_RECENT)
	_btk_file_chooser_entry_set_base_folder (BTK_FILE_CHOOSER_ENTRY (impl->location_entry), NULL);
      else
	goto maybe_clear_entry;
    }
  else if (closure.num_selected == 1)
    {
      if (impl->operation_mode == OPERATION_MODE_BROWSE)
        {
          GFileInfo *info;
          gboolean change_entry;

          info = _btk_file_system_model_get_info (impl->browse_files_model, &closure.first_selected_iter);

          /* If the cursor moved to the row of the newly created folder,
           * retrieving info will return NULL.
           */
          if (!info)
            return;

          g_free (impl->browse_files_last_selected_name);
          impl->browse_files_last_selected_name =
            g_strdup (g_file_info_get_display_name (info));

          if (impl->action == BTK_FILE_CHOOSER_ACTION_OPEN ||
              impl->action == BTK_FILE_CHOOSER_ACTION_SAVE ||
              impl->action == BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER)
            {
              /* Don't change the name when clicking on a folder... */
              change_entry = (! _btk_file_info_consider_as_directory (info));
            }
          else
            change_entry = TRUE; /* ... unless we are in SELECT_FOLDER mode */

          if (change_entry)
            {
              btk_entry_set_text (BTK_ENTRY (impl->location_entry), impl->browse_files_last_selected_name);

              if (impl->action == BTK_FILE_CHOOSER_ACTION_SAVE)
                _btk_file_chooser_entry_select_filename (BTK_FILE_CHOOSER_ENTRY (impl->location_entry));
            }

          return;
        }
      else if (impl->operation_mode == OPERATION_MODE_RECENT
	       && impl->action == BTK_FILE_CHOOSER_ACTION_SAVE)
	{
	  GFile *folder;

	  /* Set the base folder on the name entry, so it will do completion relative to the correct recent-folder */

	  btk_tree_model_get (BTK_TREE_MODEL (impl->recent_model), &closure.first_selected_iter,
			      MODEL_COL_FILE, &folder,
			      -1);
	  _btk_file_chooser_entry_set_base_folder (BTK_FILE_CHOOSER_ENTRY (impl->location_entry), folder);
	  g_object_unref (folder);
	  return;
	}
    }
  else
    {
      g_assert (!(impl->action == BTK_FILE_CHOOSER_ACTION_SAVE ||
                  impl->action == BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER));

      /* Multiple selection, so just clear the entry. */
      g_free (impl->browse_files_last_selected_name);
      impl->browse_files_last_selected_name = NULL;

      btk_entry_set_text (BTK_ENTRY (impl->location_entry), "");
      return;
    }

 maybe_clear_entry:

  if ((impl->action == BTK_FILE_CHOOSER_ACTION_OPEN || impl->action == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)
      && impl->browse_files_last_selected_name)
    {
      const char *entry_text;
      int len;
      gboolean clear_entry;

      entry_text = btk_entry_get_text (BTK_ENTRY (impl->location_entry));
      len = strlen (entry_text);
      if (len != 0)
        {
          /* The file chooser entry may have appended a "/" to its text.
           * So take it out, and compare the result to the old selection.
           */
          if (entry_text[len - 1] == G_DIR_SEPARATOR)
            {
              gchar *tmp;

              tmp = g_strndup (entry_text, len - 1);
              clear_entry = (compare_utf8_filenames (impl->browse_files_last_selected_name, tmp) == 0);
              g_free (tmp);
            }
          else
            clear_entry = (compare_utf8_filenames (impl->browse_files_last_selected_name, entry_text) == 0);
        }
      else
        clear_entry = FALSE;

      if (clear_entry)
        btk_entry_set_text (BTK_ENTRY (impl->location_entry), "");
    }
}

static gboolean
btk_file_chooser_default_set_current_folder (BtkFileChooser  *chooser,
					     GFile           *file,
					     GError         **error)
{
  return btk_file_chooser_default_update_current_folder (chooser, file, FALSE, FALSE, error);
}


struct UpdateCurrentFolderData
{
  BtkFileChooserDefault *impl;
  GFile *file;
  gboolean keep_trail;
  gboolean clear_entry;
  GFile *original_file;
  GError *original_error;
};

static void
update_current_folder_mount_enclosing_volume_cb (GCancellable        *cancellable,
                                                 BtkFileSystemVolume *volume,
                                                 const GError        *error,
                                                 gpointer             user_data)
{
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  struct UpdateCurrentFolderData *data = user_data;
  BtkFileChooserDefault *impl = data->impl;

  if (cancellable != impl->update_current_folder_cancellable)
    goto out;

  impl->update_current_folder_cancellable = NULL;
  set_busy_cursor (impl, FALSE);

  if (cancelled)
    goto out;

  if (error)
    {
      error_changing_folder_dialog (data->impl, data->file, g_error_copy (error));
      impl->reload_state = RELOAD_EMPTY;
      goto out;
    }

  change_folder_and_display_error (impl, data->file, data->clear_entry);

out:
  g_object_unref (data->file);
  g_free (data);

  g_object_unref (cancellable);
}

static void
update_current_folder_get_info_cb (GCancellable *cancellable,
				   GFileInfo    *info,
				   const GError *error,
				   gpointer      user_data)
{
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  struct UpdateCurrentFolderData *data = user_data;
  BtkFileChooserDefault *impl = data->impl;

  if (cancellable != impl->update_current_folder_cancellable)
    goto out;

  impl->update_current_folder_cancellable = NULL;
  impl->reload_state = RELOAD_EMPTY;

  set_busy_cursor (impl, FALSE);

  if (cancelled)
    goto out;

  if (error)
    {
      GFile *parent_file;

      if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_MOUNTED))
        {
          GMountOperation *mount_operation;
          BtkWidget *toplevel;

          g_object_unref (cancellable);
          toplevel = btk_widget_get_toplevel (BTK_WIDGET (impl));

          mount_operation = btk_mount_operation_new (BTK_WINDOW (toplevel));

          set_busy_cursor (impl, TRUE);

          impl->update_current_folder_cancellable =
            _btk_file_system_mount_enclosing_volume (impl->file_system, data->file,
                                                     mount_operation,
                                                     update_current_folder_mount_enclosing_volume_cb,
                                                     data);

          return;
        }

      if (!data->original_file)
        {
	  data->original_file = g_object_ref (data->file);
	  data->original_error = g_error_copy (error);
	}

      parent_file = g_file_get_parent (data->file);

      /* get parent path and try to change the folder to that */
      if (parent_file)
        {
	  g_object_unref (data->file);
	  data->file = parent_file;

	  g_object_unref (cancellable);

	  /* restart the update current folder operation */
	  impl->reload_state = RELOAD_HAS_FOLDER;

	  impl->update_current_folder_cancellable =
	    _btk_file_system_get_info (impl->file_system, data->file,
				       "standard::type",
				       update_current_folder_get_info_cb,
				       data);

	  set_busy_cursor (impl, TRUE);

	  return;
	}
      else
        {
          /* Error and bail out, ignoring "not found" errors since they're useless:
           * they only happen when a program defaults to a folder that has been (re)moved.
           */
          if (!g_error_matches (data->original_error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
            error_changing_folder_dialog (impl, data->original_file, data->original_error);
          else
            g_error_free (data->original_error);

	  g_object_unref (data->original_file);

	  goto out;
	}
    }

  if (data->original_file)
    {
      /* Error and bail out, ignoring "not found" errors since they're useless:
       * they only happen when a program defaults to a folder that has been (re)moved.
       */
      if (!g_error_matches (data->original_error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
        error_changing_folder_dialog (impl, data->original_file, data->original_error);
      else
        g_error_free (data->original_error);

      g_object_unref (data->original_file);
    }

  if (! _btk_file_info_consider_as_directory (info))
    goto out;

  if (!_btk_path_bar_set_file (BTK_PATH_BAR (impl->browse_path_bar), data->file, data->keep_trail, NULL))
    goto out;

  if (impl->current_folder != data->file)
    {
      if (impl->current_folder)
	g_object_unref (impl->current_folder);

      impl->current_folder = g_object_ref (data->file);
    }

  impl->reload_state = RELOAD_HAS_FOLDER;

  /* Update the widgets that may trigger a folder change themselves.  */

  if (!impl->changing_folder)
    {
      impl->changing_folder = TRUE;

      shortcuts_update_current_folder (impl);

      impl->changing_folder = FALSE;
    }

  /* Set the folder on the save entry */

  if (impl->location_entry)
    {
      _btk_file_chooser_entry_set_base_folder (BTK_FILE_CHOOSER_ENTRY (impl->location_entry),
					       impl->current_folder);

      if (data->clear_entry)
        btk_entry_set_text (BTK_ENTRY (impl->location_entry), "");
    }

  /* Create a new list model.  This is slightly evil; we store the result value
   * but perform more actions rather than returning immediately even if it
   * generates an error.
   */
  set_list_model (impl, NULL);

  /* Refresh controls */

  shortcuts_find_current_folder (impl);

  g_signal_emit_by_name (impl, "current-folder-changed", 0);

  check_preview_change (impl);
  bookmarks_check_add_sensitivity (impl);

  g_signal_emit_by_name (impl, "selection-changed", 0);

out:
  g_object_unref (data->file);
  g_free (data);

  g_object_unref (cancellable);
}

static gboolean
btk_file_chooser_default_update_current_folder (BtkFileChooser    *chooser,
						GFile             *file,
						gboolean           keep_trail,
						gboolean           clear_entry,
						GError           **error)
{
  BtkFileChooserDefault *impl = BTK_FILE_CHOOSER_DEFAULT (chooser);
  struct UpdateCurrentFolderData *data;

  profile_start ("start", NULL);

  g_object_ref (file);

  operation_mode_set (impl, OPERATION_MODE_BROWSE);

  if (impl->local_only && !_btk_file_has_native_path (file))
    {
      g_set_error_literal (error,
                           BTK_FILE_CHOOSER_ERROR,
                           BTK_FILE_CHOOSER_ERROR_BAD_FILENAME,
                           _("Cannot change to folder because it is not local"));

      g_object_unref (file);
      profile_end ("end - not local", NULL);
      return FALSE;
    }

  if (impl->update_current_folder_cancellable)
    g_cancellable_cancel (impl->update_current_folder_cancellable);

  /* Test validity of path here.  */
  data = g_new0 (struct UpdateCurrentFolderData, 1);
  data->impl = impl;
  data->file = g_object_ref (file);
  data->keep_trail = keep_trail;
  data->clear_entry = clear_entry;

  impl->reload_state = RELOAD_HAS_FOLDER;

  impl->update_current_folder_cancellable =
    _btk_file_system_get_info (impl->file_system, file,
			       "standard::type",
			       update_current_folder_get_info_cb,
			       data);

  set_busy_cursor (impl, TRUE);
  g_object_unref (file);

  profile_end ("end", NULL);
  return TRUE;
}

static GFile *
btk_file_chooser_default_get_current_folder (BtkFileChooser *chooser)
{
  BtkFileChooserDefault *impl = BTK_FILE_CHOOSER_DEFAULT (chooser);

  if (impl->operation_mode == OPERATION_MODE_SEARCH ||
      impl->operation_mode == OPERATION_MODE_RECENT)
    return NULL;
 
  if (impl->current_folder)
    return g_object_ref (impl->current_folder);

  return NULL;
}

static void
btk_file_chooser_default_set_current_name (BtkFileChooser *chooser,
					   const gchar    *name)
{
  BtkFileChooserDefault *impl = BTK_FILE_CHOOSER_DEFAULT (chooser);

  g_return_if_fail (impl->action == BTK_FILE_CHOOSER_ACTION_SAVE ||
		    impl->action == BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER);

  pending_select_files_free (impl);
  btk_entry_set_text (BTK_ENTRY (impl->location_entry), name);
}

static gboolean
btk_file_chooser_default_select_file (BtkFileChooser  *chooser,
				      GFile           *file,
				      GError         **error)
{
  BtkFileChooserDefault *impl = BTK_FILE_CHOOSER_DEFAULT (chooser);
  GFile *parent_file;
  gboolean same_path;

  parent_file = g_file_get_parent (file);

  if (!parent_file)
    return btk_file_chooser_set_current_folder_file (chooser, file, error);

  if (impl->operation_mode == OPERATION_MODE_SEARCH ||
      impl->operation_mode == OPERATION_MODE_RECENT ||
      impl->load_state == LOAD_EMPTY)
    {
      same_path = FALSE;
    }
  else
    {
      g_assert (impl->current_folder != NULL);

      same_path = g_file_equal (parent_file, impl->current_folder);
    }

  if (same_path && impl->load_state == LOAD_FINISHED)
    {
      gboolean result;
      GSList files;

      files.data = (gpointer) file;
      files.next = NULL;

      result = show_and_select_files (impl, &files);
      g_object_unref (parent_file);
      return result;
    }

  pending_select_files_add (impl, file);

  if (!same_path)
    {
      gboolean result;

      result = btk_file_chooser_set_current_folder_file (chooser, parent_file, error);
      g_object_unref (parent_file);
      return result;
    }

  g_object_unref (parent_file);
  return TRUE;
}

static void
btk_file_chooser_default_unselect_file (BtkFileChooser *chooser,
					GFile          *file)
{
  BtkFileChooserDefault *impl = BTK_FILE_CHOOSER_DEFAULT (chooser);
  BtkTreeView *tree_view = BTK_TREE_VIEW (impl->browse_files_tree_view);
  BtkTreeIter iter;

  if (!impl->browse_files_model)
    return;

  if (!_btk_file_system_model_get_iter_for_file (impl->browse_files_model,
                                                 &iter,
                                                 file))
    return;

  btk_tree_selection_unselect_iter (btk_tree_view_get_selection (tree_view),
                                    &iter);
}

static gboolean
maybe_select (BtkTreeModel *model, 
	      BtkTreePath  *path, 
	      BtkTreeIter  *iter, 
	      gpointer     data)
{
  BtkFileChooserDefault *impl = BTK_FILE_CHOOSER_DEFAULT (data);
  BtkTreeSelection *selection;
  gboolean is_sensitive;
  gboolean is_folder;
  
  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_files_tree_view));
  
  btk_tree_model_get (model, iter,
                      MODEL_COL_IS_FOLDER, &is_folder,
                      MODEL_COL_IS_SENSITIVE, &is_sensitive,
                      -1);

  if (is_sensitive &&
      ((is_folder && impl->action == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER) ||
       (!is_folder && impl->action == BTK_FILE_CHOOSER_ACTION_OPEN)))
    btk_tree_selection_select_iter (selection, iter);
  else
    btk_tree_selection_unselect_iter (selection, iter);
    
  return FALSE;
}

static void
btk_file_chooser_default_select_all (BtkFileChooser *chooser)
{
  BtkFileChooserDefault *impl = BTK_FILE_CHOOSER_DEFAULT (chooser);

  if (impl->operation_mode == OPERATION_MODE_SEARCH ||
      impl->operation_mode == OPERATION_MODE_RECENT)
    {
      BtkTreeSelection *selection;
      
      selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_files_tree_view));
      btk_tree_selection_select_all (selection);
      return;
    }

  if (impl->select_multiple)
    btk_tree_model_foreach (BTK_TREE_MODEL (impl->browse_files_model), 
			    maybe_select, impl);
}

static void
btk_file_chooser_default_unselect_all (BtkFileChooser *chooser)
{
  BtkFileChooserDefault *impl = BTK_FILE_CHOOSER_DEFAULT (chooser);
  BtkTreeSelection *selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_files_tree_view));

  btk_tree_selection_unselect_all (selection);
  pending_select_files_free (impl);
}

/* Checks whether the filename entry for the Save modes contains a well-formed filename.
 *
 * is_well_formed_ret - whether what the user typed passes gkt_file_system_make_path()
 *
 * is_empty_ret - whether the file entry is totally empty
 *
 * is_file_part_empty_ret - whether the file part is empty (will be if user types "foobar/", and
 *                          the path will be "$cwd/foobar")
 */
static void
check_save_entry (BtkFileChooserDefault *impl,
		  GFile                **file_ret,
		  gboolean              *is_well_formed_ret,
		  gboolean              *is_empty_ret,
		  gboolean              *is_file_part_empty_ret,
		  gboolean              *is_folder)
{
  BtkFileChooserEntry *chooser_entry;
  GFile *current_folder;
  const char *file_part;
  GFile *file;
  GError *error;

  g_assert (impl->action == BTK_FILE_CHOOSER_ACTION_SAVE
	    || impl->action == BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER
	    || ((impl->action == BTK_FILE_CHOOSER_ACTION_OPEN
		 || impl->action == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)
		&& impl->location_mode == LOCATION_MODE_FILENAME_ENTRY));

  chooser_entry = BTK_FILE_CHOOSER_ENTRY (impl->location_entry);

  if (strlen (btk_entry_get_text (BTK_ENTRY (chooser_entry))) == 0)
    {
      *file_ret = NULL;
      *is_well_formed_ret = TRUE;
      *is_empty_ret = TRUE;
      *is_file_part_empty_ret = TRUE;
      *is_folder = FALSE;

      return;
    }

  *is_empty_ret = FALSE;

  current_folder = _btk_file_chooser_entry_get_current_folder (chooser_entry);
  if (!current_folder)
    {
      *file_ret = NULL;
      *is_well_formed_ret = FALSE;
      *is_file_part_empty_ret = FALSE;
      *is_folder = FALSE;

      return;
    }

  file_part = _btk_file_chooser_entry_get_file_part (chooser_entry);

  if (!file_part || file_part[0] == '\0')
    {
      *file_ret = current_folder;
      *is_well_formed_ret = TRUE;
      *is_file_part_empty_ret = TRUE;
      *is_folder = TRUE;

      return;
    }

  *is_file_part_empty_ret = FALSE;

  error = NULL;
  file = g_file_get_child_for_display_name (current_folder, file_part, &error);
  g_object_unref (current_folder);

  if (!file)
    {
      error_building_filename_dialog (impl, error);
      *file_ret = NULL;
      *is_well_formed_ret = FALSE;
      *is_folder = FALSE;

      return;
    }

  *file_ret = file;
  *is_well_formed_ret = TRUE;
  *is_folder = _btk_file_chooser_entry_get_is_folder (chooser_entry, file);
}

struct get_files_closure {
  BtkFileChooserDefault *impl;
  GSList *result;
  GFile *file_from_entry;
};

static void
get_files_foreach (BtkTreeModel *model,
		   BtkTreePath  *path,
		   BtkTreeIter  *iter,
		   gpointer      data)
{
  struct get_files_closure *info;
  GFile *file;
  BtkFileSystemModel *fs_model;

  info = data;
  fs_model = info->impl->browse_files_model;

  file = _btk_file_system_model_get_file (fs_model, iter);
  if (!file)
    return; /* We are on the editable row */

  if (!info->file_from_entry || !g_file_equal (info->file_from_entry, file))
    info->result = b_slist_prepend (info->result, g_object_ref (file));
}

static GSList *
btk_file_chooser_default_get_files (BtkFileChooser *chooser)
{
  BtkFileChooserDefault *impl = BTK_FILE_CHOOSER_DEFAULT (chooser);
  struct get_files_closure info;
  BtkWindow *toplevel;
  BtkWidget *current_focus;
  gboolean file_list_seen;

  info.impl = impl;
  info.result = NULL;
  info.file_from_entry = NULL;

  if (impl->operation_mode == OPERATION_MODE_SEARCH)
    return search_get_selected_files (impl);

  if (impl->operation_mode == OPERATION_MODE_RECENT)
    {
      if (impl->action == BTK_FILE_CHOOSER_ACTION_SAVE)
	{
	  file_list_seen = TRUE;
	  goto file_entry;
	}
      else
	return recent_get_selected_files (impl);
    }

  toplevel = get_toplevel (BTK_WIDGET (impl));
  if (toplevel)
    current_focus = btk_window_get_focus (toplevel);
  else
    current_focus = NULL;

  file_list_seen = FALSE;
  if (current_focus == impl->browse_files_tree_view)
    {
      BtkTreeSelection *selection;

    file_list:

      file_list_seen = TRUE;
      selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_files_tree_view));
      btk_tree_selection_selected_foreach (selection, get_files_foreach, &info);

      /* If there is no selection in the file list, we probably have this situation:
       *
       * 1. The user typed a filename in the SAVE filename entry ("foo.txt").
       * 2. He then double-clicked on a folder ("bar") in the file list
       *
       * So we want the selection to be "bar/foo.txt".  Jump to the case for the
       * filename entry to see if that is the case.
       */
      if (info.result == NULL && impl->location_entry)
	goto file_entry;
    }
  else if (impl->location_entry && current_focus == impl->location_entry)
    {
      gboolean is_well_formed, is_empty, is_file_part_empty, is_folder;

    file_entry:

      check_save_entry (impl, &info.file_from_entry, &is_well_formed, &is_empty, &is_file_part_empty, &is_folder);

      if (is_empty)
	goto out;

      if (!is_well_formed)
	return NULL;

      if (is_file_part_empty && impl->action == BTK_FILE_CHOOSER_ACTION_SAVE)
	{
	  g_object_unref (info.file_from_entry);
	  return NULL;
	}

      if (info.file_from_entry)
        info.result = b_slist_prepend (info.result, info.file_from_entry);
      else if (!file_list_seen) 
        goto file_list;
      else
        return NULL;
    }
  else if (impl->toplevel_last_focus_widget == impl->browse_files_tree_view)
    goto file_list;
  else if (impl->location_entry && impl->toplevel_last_focus_widget == impl->location_entry)
    goto file_entry;
  else
    {
      /* The focus is on a dialog's action area button or something else */
      if (impl->action == BTK_FILE_CHOOSER_ACTION_SAVE ||
          impl->action == BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER)
	goto file_entry;
      else
	goto file_list; 
    }

 out:

  /* If there's no folder selected, and we're in SELECT_FOLDER mode, then we
   * fall back to the current directory */
  if (impl->action == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER &&
      info.result == NULL)
    {
      GFile *current_folder;

      current_folder = btk_file_chooser_get_current_folder_file (chooser);

      if (current_folder)
        info.result = b_slist_prepend (info.result, current_folder);
    }

  return b_slist_reverse (info.result);
}

GFile *
btk_file_chooser_default_get_preview_file (BtkFileChooser *chooser)
{
  BtkFileChooserDefault *impl = BTK_FILE_CHOOSER_DEFAULT (chooser);

  if (impl->preview_file)
    return g_object_ref (impl->preview_file);
  else
    return NULL;
}

static BtkFileSystem *
btk_file_chooser_default_get_file_system (BtkFileChooser *chooser)
{
  BtkFileChooserDefault *impl = BTK_FILE_CHOOSER_DEFAULT (chooser);

  return impl->file_system;
}

/* Shows or hides the filter widgets */
static void
show_filters (BtkFileChooserDefault *impl,
	      gboolean               show)
{
  if (show)
    btk_widget_show (impl->filter_combo_hbox);
  else
    btk_widget_hide (impl->filter_combo_hbox);
}

static void
btk_file_chooser_default_add_filter (BtkFileChooser *chooser,
				     BtkFileFilter  *filter)
{
  BtkFileChooserDefault *impl = BTK_FILE_CHOOSER_DEFAULT (chooser);
  const gchar *name;

  if (b_slist_find (impl->filters, filter))
    {
      g_warning ("btk_file_chooser_add_filter() called on filter already in list\n");
      return;
    }

  g_object_ref_sink (filter);
  impl->filters = b_slist_append (impl->filters, filter);

  name = btk_file_filter_get_name (filter);
  if (!name)
    name = "Untitled filter";	/* Place-holder, doesn't need to be marked for translation */

  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (impl->filter_combo), name);

  if (!b_slist_find (impl->filters, impl->current_filter))
    set_current_filter (impl, filter);

  show_filters (impl, TRUE);
}

static void
btk_file_chooser_default_remove_filter (BtkFileChooser *chooser,
					BtkFileFilter  *filter)
{
  BtkFileChooserDefault *impl = BTK_FILE_CHOOSER_DEFAULT (chooser);
  BtkTreeModel *model;
  BtkTreeIter iter;
  gint filter_index;

  filter_index = b_slist_index (impl->filters, filter);

  if (filter_index < 0)
    {
      g_warning ("btk_file_chooser_remove_filter() called on filter not in list\n");
      return;
    }

  impl->filters = b_slist_remove (impl->filters, filter);

  if (filter == impl->current_filter)
    {
      if (impl->filters)
	set_current_filter (impl, impl->filters->data);
      else
	set_current_filter (impl, NULL);
    }

  /* Remove row from the combo box */
  model = btk_combo_box_get_model (BTK_COMBO_BOX (impl->filter_combo));
  if (!btk_tree_model_iter_nth_child  (model, &iter, NULL, filter_index))
    g_assert_not_reached ();

  btk_list_store_remove (BTK_LIST_STORE (model), &iter);

  g_object_unref (filter);

  if (!impl->filters)
    show_filters (impl, FALSE);
}

static GSList *
btk_file_chooser_default_list_filters (BtkFileChooser *chooser)
{
  BtkFileChooserDefault *impl = BTK_FILE_CHOOSER_DEFAULT (chooser);

  return b_slist_copy (impl->filters);
}

/* Returns the position in the shortcuts tree where the nth specified shortcut would appear */
static int
shortcuts_get_pos_for_shortcut_folder (BtkFileChooserDefault *impl,
				       int                    pos)
{
  return pos + shortcuts_get_index (impl, SHORTCUTS_SHORTCUTS);
}

struct AddShortcutData
{
  BtkFileChooserDefault *impl;
  GFile *file;
};

static void
add_shortcut_get_info_cb (GCancellable *cancellable,
			  GFileInfo    *info,
			  const GError *error,
			  gpointer      user_data)
{
  int pos;
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  struct AddShortcutData *data = user_data;

  if (!b_slist_find (data->impl->loading_shortcuts, cancellable))
    goto out;

  data->impl->loading_shortcuts = b_slist_remove (data->impl->loading_shortcuts, cancellable);

  if (cancelled || error || (! _btk_file_info_consider_as_directory (info)))
    goto out;

  pos = shortcuts_get_pos_for_shortcut_folder (data->impl, data->impl->num_shortcuts);

  shortcuts_insert_file (data->impl, pos, SHORTCUT_TYPE_FILE, NULL, data->file, NULL, FALSE, SHORTCUTS_SHORTCUTS);

  /* need to call shortcuts_add_bookmarks to flush out any duplicates bug #577806 */
  shortcuts_add_bookmarks (data->impl);

out:
  g_object_unref (data->impl);
  g_object_unref (data->file);
  g_free (data);

  g_object_unref (cancellable);
}

static gboolean
btk_file_chooser_default_add_shortcut_folder (BtkFileChooser  *chooser,
					      GFile           *file,
					      GError         **error)
{
  GCancellable *cancellable;
  BtkFileChooserDefault *impl = BTK_FILE_CHOOSER_DEFAULT (chooser);
  struct AddShortcutData *data;
  GSList *l;
  int pos;

  /* Avoid adding duplicates */
  pos = shortcut_find_position (impl, file);
  if (pos >= 0 && pos < shortcuts_get_index (impl, SHORTCUTS_BOOKMARKS_SEPARATOR))
    {
      gchar *uri;

      uri = g_file_get_uri (file);
      /* translators, "Shortcut" means "Bookmark" here */
      g_set_error (error,
		   BTK_FILE_CHOOSER_ERROR,
		   BTK_FILE_CHOOSER_ERROR_ALREADY_EXISTS,
		   _("Shortcut %s already exists"),
		   uri);
      g_free (uri);

      return FALSE;
    }

  for (l = impl->loading_shortcuts; l; l = l->next)
    {
      GCancellable *c = l->data;
      GFile *f;

      f = g_object_get_data (B_OBJECT (c), "add-shortcut-path-key");
      if (f && g_file_equal (file, f))
        {
	  gchar *uri;

	  uri = g_file_get_uri (file);
          g_set_error (error,
		       BTK_FILE_CHOOSER_ERROR,
		       BTK_FILE_CHOOSER_ERROR_ALREADY_EXISTS,
		       _("Shortcut %s already exists"),
		       uri);
          g_free (uri);

          return FALSE;
	}
    }

  data = g_new0 (struct AddShortcutData, 1);
  data->impl = g_object_ref (impl);
  data->file = g_object_ref (file);

  cancellable = _btk_file_system_get_info (impl->file_system, file,
					   "standard::type",
					   add_shortcut_get_info_cb, data);

  if (!cancellable)
    return FALSE;

  impl->loading_shortcuts = b_slist_append (impl->loading_shortcuts, cancellable);
  g_object_set_data (B_OBJECT (cancellable), "add-shortcut-path-key", data->file);

  return TRUE;
}

static gboolean
btk_file_chooser_default_remove_shortcut_folder (BtkFileChooser  *chooser,
						 GFile           *file,
						 GError         **error)
{
  BtkFileChooserDefault *impl = BTK_FILE_CHOOSER_DEFAULT (chooser);
  int pos;
  BtkTreeIter iter;
  GSList *l;
  char *uri;
  int i;

  for (l = impl->loading_shortcuts; l; l = l->next)
    {
      GCancellable *c = l->data;
      GFile *f;

      f = g_object_get_data (B_OBJECT (c), "add-shortcut-path-key");
      if (f && g_file_equal (file, f))
        {
	  impl->loading_shortcuts = b_slist_remove (impl->loading_shortcuts, c);
	  g_cancellable_cancel (c);
          return TRUE;
	}
    }

  if (impl->num_shortcuts == 0)
    goto out;

  pos = shortcuts_get_pos_for_shortcut_folder (impl, 0);
  if (!btk_tree_model_iter_nth_child (BTK_TREE_MODEL (impl->shortcuts_model), &iter, NULL, pos))
    g_assert_not_reached ();

  for (i = 0; i < impl->num_shortcuts; i++)
    {
      gpointer col_data;
      ShortcutType shortcut_type;
      GFile *shortcut;

      btk_tree_model_get (BTK_TREE_MODEL (impl->shortcuts_model), &iter,
			  SHORTCUTS_COL_DATA, &col_data,
			  SHORTCUTS_COL_TYPE, &shortcut_type,
			  -1);
      g_assert (col_data != NULL);
      g_assert (shortcut_type == SHORTCUT_TYPE_FILE);

      shortcut = col_data;
      if (g_file_equal (shortcut, file))
	{
	  shortcuts_remove_rows (impl, pos + i, 1);
	  impl->num_shortcuts--;
	  return TRUE;
	}

      if (!btk_tree_model_iter_next (BTK_TREE_MODEL (impl->shortcuts_model), &iter))
	g_assert_not_reached ();
    }

 out:

  uri = g_file_get_uri (file);
  /* translators, "Shortcut" means "Bookmark" here */
  g_set_error (error,
	       BTK_FILE_CHOOSER_ERROR,
	       BTK_FILE_CHOOSER_ERROR_NONEXISTENT,
	       _("Shortcut %s does not exist"),
	       uri);
  g_free (uri);

  return FALSE;
}

static GSList *
btk_file_chooser_default_list_shortcut_folders (BtkFileChooser *chooser)
{
  BtkFileChooserDefault *impl = BTK_FILE_CHOOSER_DEFAULT (chooser);
  int pos;
  BtkTreeIter iter;
  int i;
  GSList *list;

  if (impl->num_shortcuts == 0)
    return NULL;

  pos = shortcuts_get_pos_for_shortcut_folder (impl, 0);
  if (!btk_tree_model_iter_nth_child (BTK_TREE_MODEL (impl->shortcuts_model), &iter, NULL, pos))
    g_assert_not_reached ();

  list = NULL;

  for (i = 0; i < impl->num_shortcuts; i++)
    {
      gpointer col_data;
      ShortcutType shortcut_type;
      GFile *shortcut;

      btk_tree_model_get (BTK_TREE_MODEL (impl->shortcuts_model), &iter,
			  SHORTCUTS_COL_DATA, &col_data,
			  SHORTCUTS_COL_TYPE, &shortcut_type,
			  -1);
      g_assert (col_data != NULL);
      g_assert (shortcut_type == SHORTCUT_TYPE_FILE);

      shortcut = col_data;
      list = b_slist_prepend (list, g_object_ref (shortcut));

      if (i != impl->num_shortcuts - 1)
	{
	  if (!btk_tree_model_iter_next (BTK_TREE_MODEL (impl->shortcuts_model), &iter))
	    g_assert_not_reached ();
	}
    }

  return b_slist_reverse (list);
}

/* Guesses a size based upon font sizes */
static void
find_good_size_from_style (BtkWidget *widget,
			   gint      *width,
			   gint      *height)
{
  int font_size;
  BdkScreen *screen;
  double resolution;

  g_assert (widget->style != NULL);

  screen = btk_widget_get_screen (widget);
  if (screen)
    {
      resolution = bdk_screen_get_resolution (screen);
      if (resolution < 0.0) /* will be -1 if the resolution is not defined in the BdkScreen */
	resolution = 96.0;
    }
  else
    resolution = 96.0; /* wheeee */

  font_size = bango_font_description_get_size (widget->style->font_desc);
  font_size = BANGO_PIXELS (font_size) * resolution / 72.0;

  *width = font_size * NUM_CHARS;
  *height = font_size * NUM_LINES;
}

static void
btk_file_chooser_default_get_default_size (BtkFileChooserEmbed *chooser_embed,
					   gint                *default_width,
					   gint                *default_height)
{
  BtkFileChooserDefault *impl;
  BtkRequisition req;
  BtkFileChooserSettings *settings;
  int x, y, width, height;

  impl = BTK_FILE_CHOOSER_DEFAULT (chooser_embed);

  settings = _btk_file_chooser_settings_new ();
  _btk_file_chooser_settings_get_geometry (settings, &x, &y, &width, &height);
  g_object_unref (settings);

  if (x >= 0 && y >= 0 && width > 0 && height > 0)
    {
      *default_width = width;
      *default_height = height;
      return;
    }

  find_good_size_from_style (BTK_WIDGET (chooser_embed), default_width, default_height);

  if (impl->preview_widget_active &&
      impl->preview_widget &&
      btk_widget_get_visible (impl->preview_widget))
    {
      btk_widget_size_request (impl->preview_box, &req);
      *default_width += PREVIEW_HBOX_SPACING + req.width;
    }

  if (impl->extra_widget &&
      btk_widget_get_visible (impl->extra_widget))
    {
      btk_widget_size_request (impl->extra_align, &req);
      *default_height += btk_box_get_spacing (BTK_BOX (chooser_embed)) + req.height;
    }
}

struct switch_folder_closure {
  BtkFileChooserDefault *impl;
  GFile *file;
  int num_selected;
};

/* Used from btk_tree_selection_selected_foreach() in switch_to_selected_folder() */
static void
switch_folder_foreach_cb (BtkTreeModel      *model,
			  BtkTreePath       *path,
			  BtkTreeIter       *iter,
			  gpointer           data)
{
  struct switch_folder_closure *closure;

  closure = data;

  closure->file = _btk_file_system_model_get_file (closure->impl->browse_files_model, iter);
  closure->num_selected++;
}

/* Changes to the selected folder in the list view */
static void
switch_to_selected_folder (BtkFileChooserDefault *impl)
{
  BtkTreeSelection *selection;
  struct switch_folder_closure closure;

  /* We do this with foreach() rather than get_selected() as we may be in
   * multiple selection mode
   */

  closure.impl = impl;
  closure.file = NULL;
  closure.num_selected = 0;

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_files_tree_view));
  btk_tree_selection_selected_foreach (selection, switch_folder_foreach_cb, &closure);

  g_assert (closure.file && closure.num_selected == 1);

  change_folder_and_display_error (impl, closure.file, FALSE);
}

/* Gets the GFileInfo for the selected row in the file list; assumes single
 * selection mode.
 */
static GFileInfo *
get_selected_file_info_from_file_list (BtkFileChooserDefault *impl,
				       gboolean              *had_selection)
{
  BtkTreeSelection *selection;
  BtkTreeIter iter;
  GFileInfo *info;

  g_assert (!impl->select_multiple);
  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_files_tree_view));
  if (!btk_tree_selection_get_selected (selection, NULL, &iter))
    {
      *had_selection = FALSE;
      return NULL;
    }

  *had_selection = TRUE;

  info = _btk_file_system_model_get_info (impl->browse_files_model, &iter);
  return info;
}

/* Gets the display name of the selected file in the file list; assumes single
 * selection mode and that something is selected.
 */
static const gchar *
get_display_name_from_file_list (BtkFileChooserDefault *impl)
{
  GFileInfo *info;
  gboolean had_selection;

  info = get_selected_file_info_from_file_list (impl, &had_selection);
  g_assert (had_selection);
  g_assert (info != NULL);

  return g_file_info_get_display_name (info);
}

static void
add_custom_button_to_dialog (BtkDialog   *dialog,
			     const gchar *mnemonic_label,
			     const gchar *stock_id,
			     gint         response_id)
{
  BtkWidget *button;

  button = btk_button_new_with_mnemonic (mnemonic_label);
  btk_widget_set_can_default (button, TRUE);
  btk_button_set_image (BTK_BUTTON (button),
			btk_image_new_from_stock (stock_id, BTK_ICON_SIZE_BUTTON));
  btk_widget_show (button);

  btk_dialog_add_action_widget (BTK_DIALOG (dialog), button, response_id);
}

/* Presents an overwrite confirmation dialog; returns whether we should accept
 * the filename.
 */
static gboolean
confirm_dialog_should_accept_filename (BtkFileChooserDefault *impl,
				       const gchar           *file_part,
				       const gchar           *folder_display_name)
{
  BtkWindow *toplevel;
  BtkWidget *dialog;
  int response;

  toplevel = get_toplevel (BTK_WIDGET (impl));

  dialog = btk_message_dialog_new (toplevel,
				   BTK_DIALOG_MODAL | BTK_DIALOG_DESTROY_WITH_PARENT,
				   BTK_MESSAGE_QUESTION,
				   BTK_BUTTONS_NONE,
				   _("A file named \"%s\" already exists.  Do you want to replace it?"),
				   file_part);
  btk_message_dialog_format_secondary_text (BTK_MESSAGE_DIALOG (dialog),
					    _("The file already exists in \"%s\".  Replacing it will "
					      "overwrite its contents."),
					    folder_display_name);

  btk_dialog_add_button (BTK_DIALOG (dialog), BTK_STOCK_CANCEL, BTK_RESPONSE_CANCEL);
  add_custom_button_to_dialog (BTK_DIALOG (dialog), _("_Replace"),
                               BTK_STOCK_SAVE_AS, BTK_RESPONSE_ACCEPT);
  btk_dialog_set_alternative_button_order (BTK_DIALOG (dialog),
                                           BTK_RESPONSE_ACCEPT,
                                           BTK_RESPONSE_CANCEL,
                                           -1);
  btk_dialog_set_default_response (BTK_DIALOG (dialog), BTK_RESPONSE_ACCEPT);

  if (btk_window_has_group (toplevel))
    btk_window_group_add_window (btk_window_get_group (toplevel),
                                 BTK_WINDOW (dialog));

  response = btk_dialog_run (BTK_DIALOG (dialog));

  btk_widget_destroy (dialog);

  return (response == BTK_RESPONSE_ACCEPT);
}

struct GetDisplayNameData
{
  BtkFileChooserDefault *impl;
  gchar *file_part;
};

/* Every time we request a response explicitly, we need to save the selection to the recently-used list,
 * as requesting a response means, "the dialog is confirmed".
 */
static void
request_response_and_add_to_recent_list (BtkFileChooserDefault *impl)
{
  g_signal_emit_by_name (impl, "response-requested");
  add_selection_to_recent_list (impl);
}

static void
confirmation_confirm_get_info_cb (GCancellable *cancellable,
				  GFileInfo    *info,
				  const GError *error,
				  gpointer      user_data)
{
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  gboolean should_respond = FALSE;
  struct GetDisplayNameData *data = user_data;

  if (cancellable != data->impl->should_respond_get_info_cancellable)
    goto out;

  data->impl->should_respond_get_info_cancellable = NULL;

  if (cancelled)
    goto out;

  if (error)
    /* Huh?  Did the folder disappear?  Let the caller deal with it */
    should_respond = TRUE;
  else
    should_respond = confirm_dialog_should_accept_filename (data->impl, data->file_part, g_file_info_get_display_name (info));

  set_busy_cursor (data->impl, FALSE);
  if (should_respond)
    request_response_and_add_to_recent_list (data->impl);

out:
  g_object_unref (data->impl);
  g_free (data->file_part);
  g_free (data);

  g_object_unref (cancellable);
}

/* Does overwrite confirmation if appropriate, and returns whether the dialog
 * should respond.  Can get the file part from the file list or the save entry.
 */
static gboolean
should_respond_after_confirm_overwrite (BtkFileChooserDefault *impl,
					const gchar           *file_part,
					GFile                 *parent_file)
{
  BtkFileChooserConfirmation conf;

  if (!impl->do_overwrite_confirmation)
    return TRUE;

  conf = BTK_FILE_CHOOSER_CONFIRMATION_CONFIRM;

  g_signal_emit_by_name (impl, "confirm-overwrite", &conf);

  switch (conf)
    {
    case BTK_FILE_CHOOSER_CONFIRMATION_CONFIRM:
      {
	struct GetDisplayNameData *data;

	g_assert (file_part != NULL);

	data = g_new0 (struct GetDisplayNameData, 1);
	data->impl = g_object_ref (impl);
	data->file_part = g_strdup (file_part);

	if (impl->should_respond_get_info_cancellable)
	  g_cancellable_cancel (impl->should_respond_get_info_cancellable);

	impl->should_respond_get_info_cancellable =
	  _btk_file_system_get_info (impl->file_system, parent_file,
				     "standard::display-name",
				     confirmation_confirm_get_info_cb,
				     data);
	set_busy_cursor (data->impl, TRUE);
	return FALSE;
      }

    case BTK_FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME:
      return TRUE;

    case BTK_FILE_CHOOSER_CONFIRMATION_SELECT_AGAIN:
      return FALSE;

    default:
      g_assert_not_reached ();
      return FALSE;
    }
}

struct FileExistsData
{
  BtkFileChooserDefault *impl;
  gboolean file_exists_and_is_not_folder;
  GFile *parent_file;
  GFile *file;
};

static void
name_entry_get_parent_info_cb (GCancellable *cancellable,
			       GFileInfo    *info,
			       const GError *error,
			       gpointer      user_data)
{
  gboolean parent_is_folder;
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  struct FileExistsData *data = user_data;

  if (cancellable != data->impl->should_respond_get_info_cancellable)
    goto out;

  data->impl->should_respond_get_info_cancellable = NULL;

  set_busy_cursor (data->impl, FALSE);

  if (cancelled)
    goto out;

  if (!info)
    parent_is_folder = FALSE;
  else
    parent_is_folder = _btk_file_info_consider_as_directory (info);

  if (parent_is_folder)
    {
      if (data->impl->action == BTK_FILE_CHOOSER_ACTION_OPEN)
	{
	  request_response_and_add_to_recent_list (data->impl); /* even if the file doesn't exist, apps can make good use of that (e.g. Emacs) */
	}
      else if (data->impl->action == BTK_FILE_CHOOSER_ACTION_SAVE)
        {
          if (data->file_exists_and_is_not_folder)
	    {
	      gboolean retval;
	      char *file_part;

              /* Dup the string because the string may be modified
               * depending on what clients do in the confirm-overwrite
               * signal and this corrupts the pointer
               */
              file_part = g_strdup (_btk_file_chooser_entry_get_file_part (BTK_FILE_CHOOSER_ENTRY (data->impl->location_entry)));
	      retval = should_respond_after_confirm_overwrite (data->impl, file_part, data->parent_file);
              g_free (file_part);

	      if (retval)
		request_response_and_add_to_recent_list (data->impl);
	    }
	  else
	    request_response_and_add_to_recent_list (data->impl);
	}
      else if (data->impl->action == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER
	       || data->impl->action == BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER)
        {
	  GError *mkdir_error = NULL;

	  /* In both cases (SELECT_FOLDER and CREATE_FOLDER), if you type
	   * "/blah/nonexistent" you *will* want a folder created.
	   */

	  set_busy_cursor (data->impl, TRUE);
	  g_file_make_directory (data->file, NULL, &mkdir_error);
	  set_busy_cursor (data->impl, FALSE);

	  if (!mkdir_error)
	    request_response_and_add_to_recent_list (data->impl);
	  else
	    error_creating_folder_dialog (data->impl, data->file, mkdir_error);
        }
      else
	g_assert_not_reached ();
    }
  else
    {
      if (info)
	{
	  /* The parent exists, but it's not a folder!  Someone probably typed existing_file.txt/subfile.txt */
	  error_with_file_under_nonfolder (data->impl, data->parent_file);
	}
      else
	{
	  GError *error_copy;

	  /* The parent folder is not readable for some reason */

	  error_copy = g_error_copy (error);
	  error_changing_folder_dialog (data->impl, data->parent_file, error_copy);
	}
    }

out:
  g_object_unref (data->impl);
  g_object_unref (data->file);
  g_object_unref (data->parent_file);
  g_free (data);

  g_object_unref (cancellable);
}

static void
file_exists_get_info_cb (GCancellable *cancellable,
			 GFileInfo    *info,
			 const GError *error,
			 gpointer      user_data)
{
  gboolean data_ownership_taken = FALSE;
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  gboolean file_exists;
  gboolean is_folder;
  gboolean needs_parent_check = FALSE;
  struct FileExistsData *data = user_data;

  if (cancellable != data->impl->file_exists_get_info_cancellable)
    goto out;

  data->impl->file_exists_get_info_cancellable = NULL;

  set_busy_cursor (data->impl, FALSE);

  if (cancelled)
    goto out;

  file_exists = (info != NULL);
  is_folder = (file_exists && _btk_file_info_consider_as_directory (info));

  if (data->impl->action == BTK_FILE_CHOOSER_ACTION_OPEN)
    {
      if (is_folder)
	change_folder_and_display_error (data->impl, data->file, TRUE);
      else
	{
	  if (file_exists)
	    request_response_and_add_to_recent_list (data->impl); /* user typed an existing filename; we are done */
	  else
	    needs_parent_check = TRUE; /* file doesn't exist; see if its parent exists */
	}
    }
  else if (data->impl->action == BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER)
    {
      if (file_exists && !is_folder)
        {
          /* Oops, the user typed the name of an existing path which is not
           * a folder
           */
          error_creating_folder_over_existing_file_dialog (data->impl, data->file,
						           g_error_copy (error));
        }
      else
        {
          needs_parent_check = TRUE;
        }
    }
  else if (data->impl->action == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)
    {
      if (!file_exists)
        {
	  needs_parent_check = TRUE;
        }
      else
	{
	  if (is_folder)
	    {
	      /* User typed a folder; we are done */
	      request_response_and_add_to_recent_list (data->impl);
	    }
	  else
	    error_selecting_folder_over_existing_file_dialog (data->impl, data->file);
	}
    }
  else if (data->impl->action == BTK_FILE_CHOOSER_ACTION_SAVE)
    {
      if (is_folder)
	change_folder_and_display_error (data->impl, data->file, TRUE);
      else
	needs_parent_check = TRUE;
    }
  else
    {
      g_assert_not_reached();
    }

  if (needs_parent_check)
    {
      /* check that everything up to the last path component exists (i.e. the parent) */

      data->file_exists_and_is_not_folder = file_exists && !is_folder;
      data_ownership_taken = TRUE;

      if (data->impl->should_respond_get_info_cancellable)
        g_cancellable_cancel (data->impl->should_respond_get_info_cancellable);

      data->impl->should_respond_get_info_cancellable =
	_btk_file_system_get_info (data->impl->file_system,
				   data->parent_file,
				   "standard::type",
				   name_entry_get_parent_info_cb,
				   data);
      set_busy_cursor (data->impl, TRUE);
    }

out:
  if (!data_ownership_taken)
    {
      g_object_unref (data->impl);
      g_object_unref (data->file);
      g_object_unref (data->parent_file);
      g_free (data);
    }

  g_object_unref (cancellable);
}

static void
paste_text_received (BtkClipboard          *clipboard,
		     const gchar           *text,
		     BtkFileChooserDefault *impl)
{
  GFile *file;

  if (!text)
    return;

  file = g_file_new_for_uri (text);

  if (!btk_file_chooser_default_select_file (BTK_FILE_CHOOSER (impl), file, NULL))
    location_popup_handler (impl, text);

  g_object_unref (file);
}

/* Handler for the "location-popup-on-paste" keybinding signal */
static void
location_popup_on_paste_handler (BtkFileChooserDefault *impl)
{
  BtkClipboard *clipboard = btk_widget_get_clipboard (BTK_WIDGET (impl),
		  				      BDK_SELECTION_CLIPBOARD);
  btk_clipboard_request_text (clipboard,
		  	      (BtkClipboardTextReceivedFunc) paste_text_received,
			      impl);
}

/* Implementation for BtkFileChooserEmbed::should_respond() */
static void
add_selection_to_recent_list (BtkFileChooserDefault *impl)
{
  GSList *files;
  GSList *l;

  files = btk_file_chooser_default_get_files (BTK_FILE_CHOOSER (impl));

  for (l = files; l; l = l->next)
    {
      GFile *file = l->data;
      char *uri;

      uri = g_file_get_uri (file);
      if (uri)
	{
	  btk_recent_manager_add_item (impl->recent_manager, uri);
	  g_free (uri);
	}
    }

  b_slist_foreach (files, (GFunc) g_object_unref, NULL);
  b_slist_free (files);
}

static gboolean
btk_file_chooser_default_should_respond (BtkFileChooserEmbed *chooser_embed)
{
  BtkFileChooserDefault *impl;
  BtkWidget *toplevel;
  BtkWidget *current_focus;
  gboolean retval;

  impl = BTK_FILE_CHOOSER_DEFAULT (chooser_embed);

  toplevel = btk_widget_get_toplevel (BTK_WIDGET (impl));
  g_assert (BTK_IS_WINDOW (toplevel));

  retval = FALSE;

  current_focus = btk_window_get_focus (BTK_WINDOW (toplevel));

  if (current_focus == impl->browse_files_tree_view)
    {
      /* The following array encodes what we do based on the impl->action and the
       * number of files selected.
       */
      typedef enum {
	NOOP,			/* Do nothing (don't respond) */
	RESPOND,		/* Respond immediately */
	RESPOND_OR_SWITCH,      /* Respond immediately if the selected item is a file; switch to it if it is a folder */
	ALL_FILES,		/* Respond only if everything selected is a file */
	ALL_FOLDERS,		/* Respond only if everything selected is a folder */
	SAVE_ENTRY,		/* Go to the code for handling the save entry */
	NOT_REACHED		/* Sanity check */
      } ActionToTake;
      static const ActionToTake what_to_do[4][3] = {
	/*				  0 selected		1 selected		many selected */
	/* ACTION_OPEN */		{ NOOP,			RESPOND_OR_SWITCH,	ALL_FILES   },
	/* ACTION_SAVE */		{ SAVE_ENTRY,		RESPOND_OR_SWITCH,	NOT_REACHED },
	/* ACTION_SELECT_FOLDER */	{ RESPOND,		ALL_FOLDERS,		ALL_FOLDERS },
	/* ACTION_CREATE_FOLDER */	{ SAVE_ENTRY,		ALL_FOLDERS,		NOT_REACHED }
      };

      int num_selected;
      gboolean all_files, all_folders;
      int k;
      ActionToTake action;

    file_list:

      g_assert (impl->action >= BTK_FILE_CHOOSER_ACTION_OPEN && impl->action <= BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER);

      if (impl->operation_mode == OPERATION_MODE_SEARCH)
	{
	  retval = search_should_respond (impl);
	  goto out;
	}

      if (impl->operation_mode == OPERATION_MODE_RECENT)
	{
	  if (impl->action == BTK_FILE_CHOOSER_ACTION_SAVE)
	    goto save_entry;
	  else
	    {
	      retval = recent_should_respond (impl);
	      goto out;
	    }
	}

      selection_check (impl, &num_selected, &all_files, &all_folders);

      if (num_selected > 2)
	k = 2;
      else
	k = num_selected;

      action = what_to_do [impl->action] [k];

      switch (action)
	{
	case NOOP:
	  return FALSE;

	case RESPOND:
	  retval = TRUE;
	  goto out;

	case RESPOND_OR_SWITCH:
	  g_assert (num_selected == 1);

	  if (all_folders)
	    {
	      switch_to_selected_folder (impl);
	      return FALSE;
	    }
	  else if (impl->action == BTK_FILE_CHOOSER_ACTION_SAVE)
	    {
	      retval = should_respond_after_confirm_overwrite (impl,
							       get_display_name_from_file_list (impl),
							       impl->current_folder);
	      goto out;
	    }
	  else
	    {
	      retval = TRUE;
	      goto out;
	    }

	case ALL_FILES:
	  retval = all_files;
	  goto out;

	case ALL_FOLDERS:
	  retval = all_folders;
	  goto out;

	case SAVE_ENTRY:
	  goto save_entry;

	default:
	  g_assert_not_reached ();
	}
    }
  else if ((impl->location_entry != NULL) && (current_focus == impl->location_entry))
    {
      GFile *file;
      gboolean is_well_formed, is_empty, is_file_part_empty;
      gboolean is_folder;
      BtkFileChooserEntry *entry;
      GError *error;

    save_entry:

      g_assert (impl->action == BTK_FILE_CHOOSER_ACTION_SAVE
		|| impl->action == BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER
		|| ((impl->action == BTK_FILE_CHOOSER_ACTION_OPEN
		     || impl->action == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)
		    && impl->location_mode == LOCATION_MODE_FILENAME_ENTRY));

      entry = BTK_FILE_CHOOSER_ENTRY (impl->location_entry);
      check_save_entry (impl, &file, &is_well_formed, &is_empty, &is_file_part_empty, &is_folder);

      if (!is_well_formed)
	{
	  if (!is_empty
	      && impl->action == BTK_FILE_CHOOSER_ACTION_SAVE
	      && impl->operation_mode == OPERATION_MODE_RECENT)
	    {
	      path_bar_set_mode (impl, PATH_BAR_ERROR_NO_FOLDER);
#if 0
	      /* We'll #ifdef this out, as the fucking treeview selects its first row,
	       * thus changing our assumption that no selection is present - setting
	       * a selection causes the error message from path_bar_set_mode() to go away,
	       * but we want the user to see that message!
	       */
	      btk_widget_grab_focus (impl->browse_files_tree_view);
#endif
	    }
	  /* FIXME: else show an "invalid filename" error as the pathbar mode? */

	  return FALSE;
	}

      if (is_empty)
        {
          if (impl->action == BTK_FILE_CHOOSER_ACTION_SAVE
	      || impl->action == BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER)
	    {
	      path_bar_set_mode (impl, PATH_BAR_ERROR_NO_FILENAME);
	      btk_widget_grab_focus (impl->location_entry);
	      return FALSE;
	    }

          goto file_list;
        }

      g_assert (file != NULL);

      error = NULL;
      if (is_folder)
	{
	  if (impl->action == BTK_FILE_CHOOSER_ACTION_OPEN ||
	      impl->action == BTK_FILE_CHOOSER_ACTION_SAVE)
	    {
	      change_folder_and_display_error (impl, file, TRUE);
	    }
	  else if (impl->action == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER ||
		   impl->action == BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER)
	    {
	      /* The folder already exists, so we do not need to create it.
	       * Just respond to terminate the dialog.
	       */
	      retval = TRUE;
	    }
	  else
	    {
	      g_assert_not_reached ();
	    }
	}
      else
	{
	  struct FileExistsData *data;

	  /* We need to check whether file exists and whether it is a folder -
	   * the BtkFileChooserEntry *does* report is_folder==FALSE as a false
	   * negative (it doesn't know yet if your last path component is a
	   * folder).
	   */

	  data = g_new0 (struct FileExistsData, 1);
	  data->impl = g_object_ref (impl);
	  data->file = g_object_ref (file);
	  data->parent_file = _btk_file_chooser_entry_get_current_folder (entry);

	  if (impl->file_exists_get_info_cancellable)
	    g_cancellable_cancel (impl->file_exists_get_info_cancellable);

	  impl->file_exists_get_info_cancellable =
	    _btk_file_system_get_info (impl->file_system, file,
				       "standard::type",
				       file_exists_get_info_cb,
				       data);

	  set_busy_cursor (impl, TRUE);

	  if (error != NULL)
	    g_error_free (error);
	}

      g_object_unref (file);
    }
  else if (impl->toplevel_last_focus_widget == impl->browse_files_tree_view)
    {
      /* The focus is on a dialog's action area button, *and* the widget that
       * was focused immediately before it is the file list.  
       */
      goto file_list;
    }
  else if (impl->operation_mode == OPERATION_MODE_SEARCH && impl->toplevel_last_focus_widget == impl->search_entry)
    {
      search_entry_activate_cb (BTK_ENTRY (impl->search_entry), impl);
      return FALSE;
    }
  else if (impl->location_entry && impl->toplevel_last_focus_widget == impl->location_entry)
    {
      /* The focus is on a dialog's action area button, *and* the widget that
       * was focused immediately before it is the location entry.
       */
      goto save_entry;
    }
  else
    /* The focus is on a dialog's action area button or something else */
    if (impl->action == BTK_FILE_CHOOSER_ACTION_SAVE
	|| impl->action == BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER)
      goto save_entry;
    else
      goto file_list; 

 out:

  if (retval)
    add_selection_to_recent_list (impl);

  return retval;
}

/* Implementation for BtkFileChooserEmbed::initial_focus() */
static void
btk_file_chooser_default_initial_focus (BtkFileChooserEmbed *chooser_embed)
{
  BtkFileChooserDefault *impl;
  BtkWidget *widget;

  impl = BTK_FILE_CHOOSER_DEFAULT (chooser_embed);

  if (impl->action == BTK_FILE_CHOOSER_ACTION_OPEN ||
      impl->action == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)
    {
      if (impl->location_mode == LOCATION_MODE_PATH_BAR
	  || impl->operation_mode == OPERATION_MODE_RECENT)
	widget = impl->browse_files_tree_view;
      else
	widget = impl->location_entry;
    }
  else if (impl->action == BTK_FILE_CHOOSER_ACTION_SAVE ||
	   impl->action == BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER)
    widget = impl->location_entry;
  else
    {
      g_assert_not_reached ();
      widget = NULL;
    }

  g_assert (widget != NULL);
  btk_widget_grab_focus (widget);
}

/* Callback used from btk_tree_selection_selected_foreach(); gets the selected GFiles */
static void
search_selected_foreach_get_file_cb (BtkTreeModel *model,
				     BtkTreePath  *path,
				     BtkTreeIter  *iter,
				     gpointer      data)
{
  GSList **list;
  GFile *file;

  list = data;

  btk_tree_model_get (model, iter, MODEL_COL_FILE, &file, -1);
  *list = b_slist_prepend (*list, g_object_ref (file));
}

/* Constructs a list of the selected paths in search mode */
static GSList *
search_get_selected_files (BtkFileChooserDefault *impl)
{
  GSList *result;
  BtkTreeSelection *selection;

  result = NULL;

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_files_tree_view));
  btk_tree_selection_selected_foreach (selection, search_selected_foreach_get_file_cb, &result);
  result = b_slist_reverse (result);

  return result;
}

/* Called from ::should_respond().  We return whether there are selected files
 * in the search list.
 */
static gboolean
search_should_respond (BtkFileChooserDefault *impl)
{
  BtkTreeSelection *selection;

  g_assert (impl->operation_mode == OPERATION_MODE_SEARCH);

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_files_tree_view));
  return (btk_tree_selection_count_selected_rows (selection) != 0);
}

/* Adds one hit from the search engine to the search_model */
static void
search_add_hit (BtkFileChooserDefault *impl,
		gchar                 *uri)
{
  GFile *file;

  file = g_file_new_for_uri (uri);
  if (!file)
    return;

  if (!g_file_is_native (file))
    {
      g_object_unref (file);
      return;
    }

  _btk_file_system_model_add_and_query_file (impl->search_model,
                                             file,
                                             MODEL_ATTRIBUTES);

  g_object_unref (file);
}

/* Callback used from BtkSearchEngine when we get new hits */
static void
search_engine_hits_added_cb (BtkSearchEngine *engine,
			     GList           *hits,
			     gpointer         data)
{
  BtkFileChooserDefault *impl;
  GList *l;
  
  impl = BTK_FILE_CHOOSER_DEFAULT (data);

  for (l = hits; l; l = l->next)
    search_add_hit (impl, (gchar*)l->data);
}

/* Callback used from BtkSearchEngine when the query is done running */
static void
search_engine_finished_cb (BtkSearchEngine *engine,
			   gpointer         data)
{
  BtkFileChooserDefault *impl;
  
  impl = BTK_FILE_CHOOSER_DEFAULT (data);
  
#if 0
  /* EB: setting the model here will avoid loads of row events,
   * but it'll make the search look like blocked.
   */
  btk_tree_view_set_model (BTK_TREE_VIEW (impl->browse_files_tree_view),
                           BTK_TREE_MODEL (impl->search_model));
  file_list_set_sort_column_ids (impl);
#endif

  /* FMQ: if search was empty, say that we got no hits */
  set_busy_cursor (impl, FALSE);
}

/* Displays a generic error when we cannot create a BtkSearchEngine.  
 * It would be better if _btk_search_engine_new() gave us a GError 
 * with a better message, but it doesn't do that right now.
 */
static void
search_error_could_not_create_client (BtkFileChooserDefault *impl)
{
  error_message (impl,
		 _("Could not start the search process"),
		 _("The program was not able to create a connection to the indexer "
		   "daemon.  Please make sure it is running."));
}

static void
search_engine_error_cb (BtkSearchEngine *engine,
			const gchar     *message,
			gpointer         data)
{
  BtkFileChooserDefault *impl;
  
  impl = BTK_FILE_CHOOSER_DEFAULT (data);

  search_stop_searching (impl, TRUE);
  error_message (impl, _("Could not send the search request"), message);

  set_busy_cursor (impl, FALSE);
}

/* Frees the data in the search_model */
static void
search_clear_model (BtkFileChooserDefault *impl, 
		    gboolean               remove_from_treeview)
{
  if (!impl->search_model)
    return;

  g_object_unref (impl->search_model);
  impl->search_model = NULL;
  
  if (remove_from_treeview)
    btk_tree_view_set_model (BTK_TREE_VIEW (impl->browse_files_tree_view), NULL);
}

/* Stops any ongoing searches; does not touch the search_model */
static void
search_stop_searching (BtkFileChooserDefault *impl,
                       gboolean               remove_query)
{
  if (remove_query && impl->search_query)
    {
      g_object_unref (impl->search_query);
      impl->search_query = NULL;
    }
  
  if (impl->search_engine)
    {
      _btk_search_engine_stop (impl->search_engine);
      
      g_object_unref (impl->search_engine);
      impl->search_engine = NULL;
    }
}

/* Creates the search_model and puts it in the tree view */
static void
search_setup_model (BtkFileChooserDefault *impl)
{
  g_assert (impl->search_model == NULL);

  impl->search_model = _btk_file_system_model_new (file_system_model_set,
                                                   impl,
						   MODEL_COLUMN_TYPES);

  btk_tree_sortable_set_sort_func (BTK_TREE_SORTABLE (impl->search_model),
				   MODEL_COL_NAME,
				   name_sort_func,
				   impl, NULL);
  btk_tree_sortable_set_sort_func (BTK_TREE_SORTABLE (impl->search_model),
				   MODEL_COL_MTIME,
				   mtime_sort_func,
				   impl, NULL);
  btk_tree_sortable_set_sort_func (BTK_TREE_SORTABLE (impl->search_model),
				   MODEL_COL_SIZE,
				   size_sort_func,
				   impl, NULL);
  set_sort_column (impl);

  /* EB: setting the model here will make the hits list update feel
   * more "alive" than setting the model at the end of the search
   * run
   */
  btk_tree_view_set_model (BTK_TREE_VIEW (impl->browse_files_tree_view),
                           BTK_TREE_MODEL (impl->search_model));
  file_list_set_sort_column_ids (impl);
}

/* Creates a new query with the specified text and launches it */
static void
search_start_query (BtkFileChooserDefault *impl,
		    const gchar           *query_text)
{
  search_stop_searching (impl, FALSE);
  search_clear_model (impl, TRUE);
  search_setup_model (impl);
  set_busy_cursor (impl, TRUE);

  if (impl->search_engine == NULL)
    impl->search_engine = _btk_search_engine_new ();

  if (!impl->search_engine)
    {
      set_busy_cursor (impl, FALSE);
      search_error_could_not_create_client (impl); /* lame; we don't get an error code or anything */
      return;
    }

  if (!impl->search_query)
    {
      impl->search_query = _btk_query_new ();
      _btk_query_set_text (impl->search_query, query_text);
    }
  
  _btk_search_engine_set_query (impl->search_engine, impl->search_query);

  g_signal_connect (impl->search_engine, "hits-added",
		    G_CALLBACK (search_engine_hits_added_cb), impl);
  g_signal_connect (impl->search_engine, "finished",
		    G_CALLBACK (search_engine_finished_cb), impl);
  g_signal_connect (impl->search_engine, "error",
		    G_CALLBACK (search_engine_error_cb), impl);

  _btk_search_engine_start (impl->search_engine);
}

/* Callback used when the user presses Enter while typing on the search
 * entry; starts the query
 */
static void
search_entry_activate_cb (BtkEntry *entry,
			  gpointer data)
{
  BtkFileChooserDefault *impl;
  const char *text;

  impl = BTK_FILE_CHOOSER_DEFAULT (data);

  text = btk_entry_get_text (BTK_ENTRY (impl->search_entry));
  if (strlen (text) == 0)
    return;

  /* reset any existing query object */
  if (impl->search_query)
    {
      g_object_unref (impl->search_query);
      impl->search_query = NULL;
    }

  search_start_query (impl, text);
}

static gboolean
focus_entry_idle_cb (BtkFileChooserDefault *impl)
{
  BDK_THREADS_ENTER ();
  
  g_source_destroy (impl->focus_entry_idle);
  impl->focus_entry_idle = NULL;

  if (impl->search_entry)
    btk_widget_grab_focus (impl->search_entry);

  BDK_THREADS_LEAVE ();

  return FALSE;
}

static void
focus_search_entry_in_idle (BtkFileChooserDefault *impl)
{
  /* bgo#634558 - When the user clicks on the Search entry in the shortcuts
   * pane, we get a selection-changed signal and we set up the search widgets.
   * However, btk_tree_view_button_press() focuses the treeview *after* making
   * the change to the selection.  So, we need to re-focus the search entry
   * after the treeview has finished doing its work; we'll do that in an idle
   * handler.
   */

  if (!impl->focus_entry_idle)
    impl->focus_entry_idle = add_idle_while_impl_is_alive (impl, G_CALLBACK (focus_entry_idle_cb));
}

/* Hides the path bar and creates the search entry */
static void
search_setup_widgets (BtkFileChooserDefault *impl)
{
  impl->search_hbox = btk_hbox_new (FALSE, 12);

  path_bar_update (impl);

  impl->search_entry = btk_entry_new ();
  g_signal_connect (impl->search_entry, "activate",
		    G_CALLBACK (search_entry_activate_cb),
		    impl);
  btk_box_pack_start (BTK_BOX (impl->search_hbox), impl->search_entry, TRUE, TRUE, 0);

  /* if there already is a query, restart it */
  if (impl->search_query)
    {
      gchar *query = _btk_query_get_text (impl->search_query);

      if (query)
        {
          btk_entry_set_text (BTK_ENTRY (impl->search_entry), query);
          search_start_query (impl, query);

          g_free (query);
        }
      else
        {
          g_object_unref (impl->search_query);
          impl->search_query = NULL;
        }
    }

  /* Box for search widgets */
  btk_box_pack_start (BTK_BOX (impl->browse_path_bar_hbox), impl->search_hbox, TRUE, TRUE, 0);
  btk_widget_show_all (impl->search_hbox);
  btk_size_group_add_widget (BTK_SIZE_GROUP (impl->browse_path_bar_size_group), impl->search_hbox);

  /* Hide the location widgets temporarily */

  if (impl->action == BTK_FILE_CHOOSER_ACTION_OPEN ||
      impl->action == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)
    {
      btk_widget_hide (impl->location_button);
      btk_widget_hide (impl->location_entry_box);
    }

  focus_search_entry_in_idle (impl);

  /* FMQ: hide the filter combo? */
}

/*
 * Recent files support
 */

/* Frees the data in the recent_model */
static void
recent_clear_model (BtkFileChooserDefault *impl,
                    gboolean               remove_from_treeview)
{
  if (!impl->recent_model)
    return;

  if (remove_from_treeview)
    btk_tree_view_set_model (BTK_TREE_VIEW (impl->browse_files_tree_view), NULL);

  g_object_unref (impl->recent_model);
  impl->recent_model = NULL;
}

/* Stops any ongoing loading of the recent files list; does
 * not touch the recent_model
 */
static void
recent_stop_loading (BtkFileChooserDefault *impl)
{
  if (impl->load_recent_id)
    {
      g_source_remove (impl->load_recent_id);
      impl->load_recent_id = 0;
    }
}

static void
recent_setup_model (BtkFileChooserDefault *impl)
{
  g_assert (impl->recent_model == NULL);

  impl->recent_model = _btk_file_system_model_new (file_system_model_set,
                                                   impl,
						   MODEL_COLUMN_TYPES);

  _btk_file_system_model_set_filter (impl->recent_model,
                                     impl->current_filter);
  btk_tree_sortable_set_sort_func (BTK_TREE_SORTABLE (impl->recent_model),
				   MODEL_COL_NAME,
				   name_sort_func,
				   impl, NULL);
  btk_tree_sortable_set_sort_func (BTK_TREE_SORTABLE (impl->recent_model),
                                   MODEL_COL_SIZE,
                                   size_sort_func,
                                   impl, NULL);
  btk_tree_sortable_set_sort_func (BTK_TREE_SORTABLE (impl->recent_model),
                                   MODEL_COL_MTIME,
                                   mtime_sort_func,
                                   impl, NULL);
  set_sort_column (impl);
}

typedef struct
{
  BtkFileChooserDefault *impl;
  GList *items;
} RecentLoadData;

static void
recent_idle_cleanup (gpointer data)
{
  RecentLoadData *load_data = data;
  BtkFileChooserDefault *impl = load_data->impl;

  btk_tree_view_set_model (BTK_TREE_VIEW (impl->browse_files_tree_view),
                           BTK_TREE_MODEL (impl->recent_model));
  file_list_set_sort_column_ids (impl);
  btk_tree_sortable_set_sort_column_id (BTK_TREE_SORTABLE (impl->recent_model), MODEL_COL_MTIME, BTK_SORT_DESCENDING);

  set_busy_cursor (impl, FALSE);
  
  impl->load_recent_id = 0;
  
  g_free (load_data);
}

static gint
get_recent_files_limit (BtkWidget *widget)
{
  BtkSettings *settings;
  gint limit;

  if (btk_widget_has_screen (widget))
    settings = btk_settings_get_for_screen (btk_widget_get_screen (widget));
  else
    settings = btk_settings_get_default ();

  g_object_get (B_OBJECT (settings), "btk-recent-files-limit", &limit, NULL);

  return limit;
}

/* Populates the file system model with the BtkRecentInfo* items in the provided list; frees the items */
static void
populate_model_with_recent_items (BtkFileChooserDefault *impl, GList *items)
{
  gint limit;
  GList *l;
  int n;

  limit = get_recent_files_limit (BTK_WIDGET (impl));

  n = 0;

  for (l = items; l; l = l->next)
    {
      BtkRecentInfo *info = l->data;
      GFile *file;

      file = g_file_new_for_uri (btk_recent_info_get_uri (info));
      _btk_file_system_model_add_and_query_file (impl->recent_model,
                                                 file,
                                                 MODEL_ATTRIBUTES);
      g_object_unref (file);

      n++;
      if (limit != -1 && n >= limit)
	break;
    }
}

static void
populate_model_with_folders (BtkFileChooserDefault *impl, GList *items)
{
  GList *folders;
  GList *l;

  folders = _btk_file_chooser_extract_recent_folders (items);

  for (l = folders; l; l = l->next)
    {
      GFile *folder = l->data;

      _btk_file_system_model_add_and_query_file (impl->recent_model,
                                                 folder,
                                                 MODEL_ATTRIBUTES);
    }

  g_list_foreach (folders, (GFunc) g_object_unref, NULL);
  g_list_free (folders);
}

static gboolean
recent_idle_load (gpointer data)
{
  RecentLoadData *load_data = data;
  BtkFileChooserDefault *impl = load_data->impl;

  if (!impl->recent_manager)
    return FALSE;

  load_data->items = btk_recent_manager_get_items (impl->recent_manager);
  if (!load_data->items)
    return FALSE;

  if (impl->action == BTK_FILE_CHOOSER_ACTION_OPEN)
    populate_model_with_recent_items (impl, load_data->items);
  else
    populate_model_with_folders (impl, load_data->items);

  g_list_foreach (load_data->items, (GFunc) btk_recent_info_unref, NULL);
  g_list_free (load_data->items);
  load_data->items = NULL;

  return FALSE;
}

static void
recent_start_loading (BtkFileChooserDefault *impl)
{
  RecentLoadData *load_data;

  recent_stop_loading (impl);
  recent_clear_model (impl, TRUE);
  recent_setup_model (impl);
  set_busy_cursor (impl, TRUE);

  g_assert (impl->load_recent_id == 0);

  load_data = g_new (RecentLoadData, 1);
  load_data->impl = impl;
  load_data->items = NULL;

  /* begin lazy loading the recent files into the model */
  impl->load_recent_id = bdk_threads_add_idle_full (G_PRIORITY_HIGH_IDLE + 30,
                                                    recent_idle_load,
                                                    load_data,
                                                    recent_idle_cleanup);
}

static void
recent_selected_foreach_get_file_cb (BtkTreeModel *model,
				     BtkTreePath  *path,
				     BtkTreeIter  *iter,
				     gpointer      data)
{
  GSList **list;
  GFile *file;

  list = data;

  btk_tree_model_get (model, iter, MODEL_COL_FILE, &file, -1);
  *list = b_slist_prepend (*list, file);
}

/* Constructs a list of the selected paths in recent files mode */
static GSList *
recent_get_selected_files (BtkFileChooserDefault *impl)
{
  GSList *result;
  BtkTreeSelection *selection;

  result = NULL;

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_files_tree_view));
  btk_tree_selection_selected_foreach (selection, recent_selected_foreach_get_file_cb, &result);
  result = b_slist_reverse (result);

  return result;
}

/* Called from ::should_respond().  We return whether there are selected
 * files in the recent files list.
 */
static gboolean
recent_should_respond (BtkFileChooserDefault *impl)
{
  BtkTreeSelection *selection;

  g_assert (impl->operation_mode == OPERATION_MODE_RECENT);

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (impl->browse_files_tree_view));
  return (btk_tree_selection_count_selected_rows (selection) != 0);
}

static void
set_current_filter (BtkFileChooserDefault *impl,
		    BtkFileFilter         *filter)
{
  if (impl->current_filter != filter)
    {
      int filter_index;

      /* NULL filters are allowed to reset to non-filtered status
       */
      filter_index = b_slist_index (impl->filters, filter);
      if (impl->filters && filter && filter_index < 0)
	return;

      if (impl->current_filter)
	g_object_unref (impl->current_filter);
      impl->current_filter = filter;
      if (impl->current_filter)
	{
	  g_object_ref_sink (impl->current_filter);
	}

      if (impl->filters)
	btk_combo_box_set_active (BTK_COMBO_BOX (impl->filter_combo),
				  filter_index);

      if (impl->browse_files_model)
	install_list_model_filter (impl);

      if (impl->search_model)
        _btk_file_system_model_set_filter (impl->search_model, filter);

      if (impl->recent_model)
        _btk_file_system_model_set_filter (impl->recent_model, filter);

      g_object_notify (B_OBJECT (impl), "filter");
    }
}

static void
filter_combo_changed (BtkComboBox           *combo_box,
		      BtkFileChooserDefault *impl)
{
  gint new_index = btk_combo_box_get_active (combo_box);
  BtkFileFilter *new_filter = b_slist_nth_data (impl->filters, new_index);

  set_current_filter (impl, new_filter);
}

static void
check_preview_change (BtkFileChooserDefault *impl)
{
  BtkTreePath *cursor_path;
  GFile *new_file;
  char *new_display_name;
  BtkTreeModel *model;

  btk_tree_view_get_cursor (BTK_TREE_VIEW (impl->browse_files_tree_view), &cursor_path, NULL);
  model = btk_tree_view_get_model (BTK_TREE_VIEW (impl->browse_files_tree_view));
  if (cursor_path)
    {
      BtkTreeIter iter;

      btk_tree_model_get_iter (model, &iter, cursor_path);
      btk_tree_model_get (model, &iter,
                          MODEL_COL_FILE, &new_file,
                          MODEL_COL_NAME, &new_display_name,
                          -1);
      
      btk_tree_path_free (cursor_path);
    }
  else
    {
      new_file = NULL;
      new_display_name = NULL;
    }

  if (new_file != impl->preview_file &&
      !(new_file && impl->preview_file &&
	g_file_equal (new_file, impl->preview_file)))
    {
      if (impl->preview_file)
	{
	  g_object_unref (impl->preview_file);
	  g_free (impl->preview_display_name);
	}

      if (new_file)
	{
	  impl->preview_file = new_file;
	  impl->preview_display_name = new_display_name;
	}
      else
	{
	  impl->preview_file = NULL;
	  impl->preview_display_name = NULL;
          g_free (new_display_name);
	}

      if (impl->use_preview_label && impl->preview_label)
	btk_label_set_text (BTK_LABEL (impl->preview_label), impl->preview_display_name);

      g_signal_emit_by_name (impl, "update-preview");
    }
  else
    {
      if (new_file)
        g_object_unref (new_file);

      g_free (new_display_name);
    }
}

static void
shortcuts_activate_volume_mount_cb (GCancellable        *cancellable,
				    BtkFileSystemVolume *volume,
				    const GError        *error,
				    gpointer             data)
{
  GFile *file;
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  BtkFileChooserDefault *impl = data;

  if (cancellable != impl->shortcuts_activate_iter_cancellable)
    goto out;

  impl->shortcuts_activate_iter_cancellable = NULL;

  set_busy_cursor (impl, FALSE);

  if (cancelled)
    goto out;

  if (error)
    {
      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_FAILED_HANDLED))
        {
          char *msg, *name;

	  name = _btk_file_system_volume_get_display_name (volume);
          msg = g_strdup_printf (_("Could not mount %s"), name);

          error_message (impl, msg, error->message);

          g_free (msg);
	  g_free (name);
        }

      goto out;
    }

  file = _btk_file_system_volume_get_root (volume);
  if (file != NULL)
    {
      change_folder_and_display_error (impl, file, FALSE);
      g_object_unref (file);
    }

out:
  g_object_unref (impl);
  g_object_unref (cancellable);
}


/* Activates a volume by mounting it if necessary and then switching to its
 * base path.
 */
static void
shortcuts_activate_volume (BtkFileChooserDefault *impl,
			   BtkFileSystemVolume   *volume)
{
  GFile *file;

  operation_mode_set (impl, OPERATION_MODE_BROWSE);

  /* We ref the file chooser since volume_mount() may run a main loop, and the
   * user could close the file chooser window in the meantime.
   */
  g_object_ref (impl);

  if (!_btk_file_system_volume_is_mounted (volume))
    {
      GMountOperation *mount_op;

      set_busy_cursor (impl, TRUE);
   
      mount_op = btk_mount_operation_new (get_toplevel (BTK_WIDGET (impl)));
      impl->shortcuts_activate_iter_cancellable =
        _btk_file_system_mount_volume (impl->file_system, volume, mount_op,
				       shortcuts_activate_volume_mount_cb,
				       g_object_ref (impl));
      g_object_unref (mount_op);
    }
  else
    {
      file = _btk_file_system_volume_get_root (volume);
      if (file != NULL)
        {
          change_folder_and_display_error (impl, file, FALSE);
	  g_object_unref (file);
        }
    }

  g_object_unref (impl);
}

/* Opens the folder or volume at the specified iter in the shortcuts model */
struct ShortcutsActivateData
{
  BtkFileChooserDefault *impl;
  GFile *file;
};

static void
shortcuts_activate_get_info_cb (GCancellable *cancellable,
				GFileInfo    *info,
			        const GError *error,
			        gpointer      user_data)
{
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  struct ShortcutsActivateData *data = user_data;

  if (cancellable != data->impl->shortcuts_activate_iter_cancellable)
    goto out;

  data->impl->shortcuts_activate_iter_cancellable = NULL;

  if (cancelled)
    goto out;

  if (!error && _btk_file_info_consider_as_directory (info))
    change_folder_and_display_error (data->impl, data->file, FALSE);
  else
    btk_file_chooser_default_select_file (BTK_FILE_CHOOSER (data->impl),
                                          data->file,
                                          NULL);

out:
  g_object_unref (data->impl);
  g_object_unref (data->file);
  g_free (data);

  g_object_unref (cancellable);
}

static void
shortcuts_activate_mount_enclosing_volume (GCancellable        *cancellable,
					   BtkFileSystemVolume *volume,
					   const GError        *error,
					   gpointer             user_data)
{
  struct ShortcutsActivateData *data = user_data;

  if (error)
    {
      error_changing_folder_dialog (data->impl, data->file, g_error_copy (error));

      g_object_unref (data->impl);
      g_object_unref (data->file);
      g_free (data);

      return;
    }

  data->impl->shortcuts_activate_iter_cancellable =
    _btk_file_system_get_info (data->impl->file_system, data->file,
			       "standard::type",
			       shortcuts_activate_get_info_cb, data);

  if (volume)
    _btk_file_system_volume_unref (volume);
}

static void
shortcuts_activate_iter (BtkFileChooserDefault *impl,
			 BtkTreeIter           *iter)
{
  gpointer col_data;
  ShortcutType shortcut_type;

  /* In the Save modes, we want to preserve what the uesr typed in the filename
   * entry, so that he may choose another folder without erasing his typed name.
   */
  if (impl->location_entry
      && !(impl->action == BTK_FILE_CHOOSER_ACTION_SAVE
	   || impl->action == BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER))
    btk_entry_set_text (BTK_ENTRY (impl->location_entry), "");

  btk_tree_model_get (BTK_TREE_MODEL (impl->shortcuts_model), iter,
		      SHORTCUTS_COL_DATA, &col_data,
		      SHORTCUTS_COL_TYPE, &shortcut_type,
		      -1);

  if (impl->shortcuts_activate_iter_cancellable)
    {
      g_cancellable_cancel (impl->shortcuts_activate_iter_cancellable);
      impl->shortcuts_activate_iter_cancellable = NULL;
    }

  if (shortcut_type == SHORTCUT_TYPE_SEPARATOR)
    return;
  else if (shortcut_type == SHORTCUT_TYPE_VOLUME)
    {
      BtkFileSystemVolume *volume;

      volume = col_data;

      operation_mode_set (impl, OPERATION_MODE_BROWSE);

      shortcuts_activate_volume (impl, volume);
    }
  else if (shortcut_type == SHORTCUT_TYPE_FILE)
    {
      struct ShortcutsActivateData *data;
      BtkFileSystemVolume *volume;

      operation_mode_set (impl, OPERATION_MODE_BROWSE);

      volume = _btk_file_system_get_volume_for_file (impl->file_system, col_data);

      data = g_new0 (struct ShortcutsActivateData, 1);
      data->impl = g_object_ref (impl);
      data->file = g_object_ref (col_data);

      if (!volume || !_btk_file_system_volume_is_mounted (volume))
	{
	  GMountOperation *mount_operation;
	  BtkWidget *toplevel;

	  toplevel = btk_widget_get_toplevel (BTK_WIDGET (impl));

	  mount_operation = btk_mount_operation_new (BTK_WINDOW (toplevel));

	  impl->shortcuts_activate_iter_cancellable =
	    _btk_file_system_mount_enclosing_volume (impl->file_system, col_data,
						     mount_operation,
						     shortcuts_activate_mount_enclosing_volume,
						     data);
	}
      else
	{
	  impl->shortcuts_activate_iter_cancellable =
	    _btk_file_system_get_info (impl->file_system, data->file,
	 			       "standard::type",
				       shortcuts_activate_get_info_cb, data);
	}
    }
  else if (shortcut_type == SHORTCUT_TYPE_SEARCH)
    {
      operation_mode_set (impl, OPERATION_MODE_SEARCH);
    }
  else if (shortcut_type == SHORTCUT_TYPE_RECENT)
    {
      operation_mode_set (impl, OPERATION_MODE_RECENT);
    }
}

/* Handler for BtkWidget::key-press-event on the shortcuts list */
static gboolean
shortcuts_key_press_event_cb (BtkWidget             *widget,
			      BdkEventKey           *event,
			      BtkFileChooserDefault *impl)
{
  guint modifiers;

  modifiers = btk_accelerator_get_default_mod_mask ();

  if (key_is_left_or_right (event))
    {
      btk_widget_grab_focus (impl->browse_files_tree_view);
      return TRUE;
    }

  if ((event->keyval == BDK_KEY_BackSpace
      || event->keyval == BDK_KEY_Delete
      || event->keyval == BDK_KEY_KP_Delete)
      && (event->state & modifiers) == 0)
    {
      remove_selected_bookmarks (impl);
      return TRUE;
    }

  if ((event->keyval == BDK_KEY_F2)
      && (event->state & modifiers) == 0)
    {
      rename_selected_bookmark (impl);
      return TRUE;
    }

  return FALSE;
}

static gboolean
shortcuts_select_func  (BtkTreeSelection  *selection,
			BtkTreeModel      *model,
			BtkTreePath       *path,
			gboolean           path_currently_selected,
			gpointer           data)
{
  BtkFileChooserDefault *impl = data;
  BtkTreeIter filter_iter;
  ShortcutType shortcut_type;

  if (!btk_tree_model_get_iter (impl->shortcuts_pane_filter_model, &filter_iter, path))
    g_assert_not_reached ();

  btk_tree_model_get (impl->shortcuts_pane_filter_model, &filter_iter, SHORTCUTS_COL_TYPE, &shortcut_type, -1);

  return shortcut_type != SHORTCUT_TYPE_SEPARATOR;
}

static gboolean
list_select_func  (BtkTreeSelection  *selection,
		   BtkTreeModel      *model,
		   BtkTreePath       *path,
		   gboolean           path_currently_selected,
		   gpointer           data)
{
  BtkFileChooserDefault *impl = data;

  if (impl->action == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER ||
      impl->action == BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER)
    {
      BtkTreeIter iter;
      gboolean is_sensitive;
      gboolean is_folder;

      if (!btk_tree_model_get_iter (model, &iter, path))
        return FALSE;
      btk_tree_model_get (model, &iter,
                          MODEL_COL_IS_SENSITIVE, &is_sensitive,
                          MODEL_COL_IS_FOLDER, &is_folder,
                          -1);
      if (!is_sensitive || !is_folder)
        return FALSE;
    }
    
  return TRUE;
}

static void
list_selection_changed (BtkTreeSelection      *selection,
			BtkFileChooserDefault *impl)
{
  /* See if we are in the new folder editable row for Save mode */
  if (impl->operation_mode == OPERATION_MODE_BROWSE &&
      impl->action == BTK_FILE_CHOOSER_ACTION_SAVE)
    {
      GFileInfo *info;
      gboolean had_selection;

      info = get_selected_file_info_from_file_list (impl, &had_selection);
      if (!had_selection)
	goto out; /* normal processing */

      if (!info)
	return; /* We are on the editable row for New Folder */
    }

 out:

  if (impl->location_entry)
    update_chooser_entry (impl);

  path_bar_update (impl);

  check_preview_change (impl);
  bookmarks_check_add_sensitivity (impl);

  g_signal_emit_by_name (impl, "selection-changed", 0);
}

/* Callback used when a row in the file list is activated */
static void
list_row_activated (BtkTreeView           *tree_view,
		    BtkTreePath           *path,
		    BtkTreeViewColumn     *column,
		    BtkFileChooserDefault *impl)
{
  GFile *file;
  BtkTreeIter iter;
  BtkTreeModel *model;
  gboolean is_folder;
  gboolean is_sensitive;

  model = btk_tree_view_get_model (tree_view);

  if (!btk_tree_model_get_iter (model, &iter, path))
    return;

  btk_tree_model_get (model, &iter,
                      MODEL_COL_FILE, &file,
                      MODEL_COL_IS_FOLDER, &is_folder,
                      MODEL_COL_IS_SENSITIVE, &is_sensitive,
                      -1);
        
  if (is_sensitive && is_folder && file)
    {
      change_folder_and_display_error (impl, file, FALSE);
      return;
    }

  if (impl->action == BTK_FILE_CHOOSER_ACTION_OPEN ||
      impl->action == BTK_FILE_CHOOSER_ACTION_SAVE)
    g_signal_emit_by_name (impl, "file-activated");

  if (file)
    g_object_unref (file);
}

static void
path_bar_clicked (BtkPathBar            *path_bar,
		  GFile                 *file,
		  GFile                 *child_file,
		  gboolean               child_is_hidden,
		  BtkFileChooserDefault *impl)
{
  if (child_file)
    pending_select_files_add (impl, child_file);

  if (!change_folder_and_display_error (impl, file, FALSE))
    return;

  /* Say we have "/foo/bar/[.baz]" and the user clicks on "bar".  We should then
   * show hidden files so that ".baz" appears in the file list, as it will still
   * be shown in the path bar: "/foo/[bar]/.baz"
   */
  if (child_is_hidden)
    g_object_set (impl, "show-hidden", TRUE, NULL);
}

static void
update_cell_renderer_attributes (BtkFileChooserDefault *impl)
{
  BtkTreeViewColumn *column;
  BtkCellRenderer *renderer;
  GList *walk, *list;

  /* Keep the following column numbers in sync with create_file_list() */

  /* name */
  column = btk_tree_view_get_column (BTK_TREE_VIEW (impl->browse_files_tree_view), 0);
  list = btk_cell_layout_get_cells (BTK_CELL_LAYOUT (column));
  for (walk = list; walk; walk = walk->next)
    {
      renderer = walk->data;
      if (BTK_IS_CELL_RENDERER_PIXBUF (renderer))
        {
          btk_tree_view_column_set_attributes (column, renderer, 
                                               "pixbuf", MODEL_COL_PIXBUF,
                                               NULL);
        }
      else
        {
          btk_tree_view_column_set_attributes (column, renderer, 
                                               "text", MODEL_COL_NAME,
                                               "ellipsize", MODEL_COL_ELLIPSIZE,
                                               NULL);
        }

      btk_tree_view_column_add_attribute (column, renderer, "sensitive", MODEL_COL_IS_SENSITIVE);
    }
  g_list_free (list);

  /* size */
  column = btk_tree_view_get_column (BTK_TREE_VIEW (impl->browse_files_tree_view), 1);
  list = btk_cell_layout_get_cells (BTK_CELL_LAYOUT (column));
  renderer = list->data;
  btk_tree_view_column_set_attributes (column, renderer, 
                                       "text", MODEL_COL_SIZE_TEXT,
                                       NULL);

  btk_tree_view_column_add_attribute (column, renderer, "sensitive", MODEL_COL_IS_SENSITIVE);
  g_list_free (list);

  /* mtime */
  column = btk_tree_view_get_column (BTK_TREE_VIEW (impl->browse_files_tree_view), 2);
  list = btk_cell_layout_get_cells (BTK_CELL_LAYOUT (column));
  renderer = list->data;
  btk_tree_view_column_set_attributes (column, renderer, 
                                       "text", MODEL_COL_MTIME_TEXT,
                                       NULL);
  btk_tree_view_column_add_attribute (column, renderer, "sensitive", MODEL_COL_IS_SENSITIVE);
  g_list_free (list);
}

BtkWidget *
_btk_file_chooser_default_new (void)
{
  return g_object_new (BTK_TYPE_FILE_CHOOSER_DEFAULT, NULL);
}

static void
location_set_user_text (BtkFileChooserDefault *impl,
			const gchar           *path)
{
  btk_entry_set_text (BTK_ENTRY (impl->location_entry), path);
  btk_editable_set_position (BTK_EDITABLE (impl->location_entry), -1);
}

static void
location_popup_handler (BtkFileChooserDefault *impl,
			const gchar           *path)
{ 
  if (impl->operation_mode != OPERATION_MODE_BROWSE)
    {
      BtkWidget *widget_to_focus;

      operation_mode_set (impl, OPERATION_MODE_BROWSE);
      
      if (impl->current_folder)
        change_folder_and_display_error (impl, impl->current_folder, FALSE);

      if (impl->location_mode == LOCATION_MODE_PATH_BAR)
        widget_to_focus = impl->browse_files_tree_view;
      else
        widget_to_focus = impl->location_entry;

      btk_widget_grab_focus (widget_to_focus);
      return; 
    }
  
  if (impl->action == BTK_FILE_CHOOSER_ACTION_OPEN ||
      impl->action == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)
    {
      if (!path)
	return;

      location_mode_set (impl, LOCATION_MODE_FILENAME_ENTRY, TRUE);
      location_set_user_text (impl, path);
    }
  else if (impl->action == BTK_FILE_CHOOSER_ACTION_SAVE ||
	   impl->action == BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER)
    {
      btk_widget_grab_focus (impl->location_entry);
      if (path != NULL)
	location_set_user_text (impl, path);
    }
  else
    g_assert_not_reached ();
}

/* Handler for the "up-folder" keybinding signal */
static void
up_folder_handler (BtkFileChooserDefault *impl)
{
  _btk_path_bar_up (BTK_PATH_BAR (impl->browse_path_bar));
}

/* Handler for the "down-folder" keybinding signal */
static void
down_folder_handler (BtkFileChooserDefault *impl)
{
  _btk_path_bar_down (BTK_PATH_BAR (impl->browse_path_bar));
}

/* Switches to the shortcut in the specified index */
static void
switch_to_shortcut (BtkFileChooserDefault *impl,
		    int pos)
{
  BtkTreeIter iter;

  if (!btk_tree_model_iter_nth_child (BTK_TREE_MODEL (impl->shortcuts_model), &iter, NULL, pos))
    g_assert_not_reached ();

  shortcuts_activate_iter (impl, &iter);
}

/* Handler for the "home-folder" keybinding signal */
static void
home_folder_handler (BtkFileChooserDefault *impl)
{
  if (impl->has_home)
    switch_to_shortcut (impl, shortcuts_get_index (impl, SHORTCUTS_HOME));
}

/* Handler for the "desktop-folder" keybinding signal */
static void
desktop_folder_handler (BtkFileChooserDefault *impl)
{
  if (impl->has_desktop)
    switch_to_shortcut (impl, shortcuts_get_index (impl, SHORTCUTS_DESKTOP));
}

/* Handler for the "search-shortcut" keybinding signal */
static void
search_shortcut_handler (BtkFileChooserDefault *impl)
{
  if (impl->has_search)
    {
      switch_to_shortcut (impl, shortcuts_get_index (impl, SHORTCUTS_SEARCH));

      /* we want the entry widget to grab the focus the first
       * time, not the browse_files_tree_view widget.
       */
      if (impl->search_entry)
        btk_widget_grab_focus (impl->search_entry);
    }
}

/* Handler for the "recent-shortcut" keybinding signal */
static void
recent_shortcut_handler (BtkFileChooserDefault *impl)
{
  switch_to_shortcut (impl, shortcuts_get_index (impl, SHORTCUTS_RECENT));
}

static void
quick_bookmark_handler (BtkFileChooserDefault *impl,
			gint bookmark_index)
{
  int bookmark_pos;
  BtkTreePath *path;

  if (bookmark_index < 0 || bookmark_index >= impl->num_bookmarks)
    return;

  bookmark_pos = shortcuts_get_index (impl, SHORTCUTS_BOOKMARKS) + bookmark_index;

  path = btk_tree_path_new_from_indices (bookmark_pos, -1);
  btk_tree_view_scroll_to_cell (BTK_TREE_VIEW (impl->browse_shortcuts_tree_view),
				path, NULL,
				FALSE, 0.0, 0.0);
  btk_tree_path_free (path);

  switch_to_shortcut (impl, bookmark_pos);
}

static void
show_hidden_handler (BtkFileChooserDefault *impl)
{
  g_object_set (impl,
		"show-hidden", !impl->show_hidden,
		NULL);
}


/* Drag and drop interfaces */

static void
_shortcuts_pane_model_filter_class_init (ShortcutsPaneModelFilterClass *class)
{
}

static void
_shortcuts_pane_model_filter_init (ShortcutsPaneModelFilter *model)
{
  model->impl = NULL;
}

/* BtkTreeDragSource::row_draggable implementation for the shortcuts filter model */
static gboolean
shortcuts_pane_model_filter_row_draggable (BtkTreeDragSource *drag_source,
				           BtkTreePath       *path)
{
  ShortcutsPaneModelFilter *model;
  int pos;
  int bookmarks_pos;

  model = SHORTCUTS_PANE_MODEL_FILTER (drag_source);

  pos = *btk_tree_path_get_indices (path);
  bookmarks_pos = shortcuts_get_index (model->impl, SHORTCUTS_BOOKMARKS);

  return (pos >= bookmarks_pos && pos < bookmarks_pos + model->impl->num_bookmarks);
}

/* BtkTreeDragSource::drag_data_get implementation for the shortcuts
 * filter model
 */
static gboolean
shortcuts_pane_model_filter_drag_data_get (BtkTreeDragSource *drag_source,
                                           BtkTreePath       *path,
                                           BtkSelectionData  *selection_data)
{
  /* FIXME */

  return FALSE;
}

/* Fill the BtkTreeDragSourceIface vtable */
static void
shortcuts_pane_model_filter_drag_source_iface_init (BtkTreeDragSourceIface *iface)
{
  iface->row_draggable = shortcuts_pane_model_filter_row_draggable;
  iface->drag_data_get = shortcuts_pane_model_filter_drag_data_get;
}

#if 0
/* Fill the BtkTreeDragDestIface vtable */
static void
shortcuts_pane_model_filter_drag_dest_iface_init (BtkTreeDragDestIface *iface)
{
  iface->drag_data_received = shortcuts_pane_model_filter_drag_data_received;
  iface->row_drop_possible = shortcuts_pane_model_filter_row_drop_possible;
}
#endif

static BtkTreeModel *
shortcuts_pane_model_filter_new (BtkFileChooserDefault *impl,
			         BtkTreeModel          *child_model,
			         BtkTreePath           *root)
{
  ShortcutsPaneModelFilter *model;

  model = g_object_new (SHORTCUTS_PANE_MODEL_FILTER_TYPE,
			"child-model", child_model,
			"virtual-root", root,
			NULL);

  model->impl = impl;

  return BTK_TREE_MODEL (model);
}

