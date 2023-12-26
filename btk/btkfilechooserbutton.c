/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */

/* BTK+: btkfilechooserbutton.c
 *
 * Copyright (c) 2004 James M. Cape <jcape@ignore-your.tv>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>

#include "btkintl.h"
#include "btkbutton.h"
#include "btkcelllayout.h"
#include "btkcellrenderertext.h"
#include "btkcellrendererpixbuf.h"
#include "btkcombobox.h"
#include "btkdnd.h"
#include "btkicontheme.h"
#include "btkiconfactory.h"
#include "btkimage.h"
#include "btklabel.h"
#include "btkliststore.h"
#include "btkstock.h"
#include "btktreemodelfilter.h"
#include "btkvseparator.h"
#include "btkfilechooserdialog.h"
#include "btkfilechooserprivate.h"
#include "btkfilechooserutils.h"
#include "btkmarshalers.h"

#include "btkfilechooserbutton.h"

#include "btkprivate.h"
#include "btkalias.h"

/* **************** *
 *  Private Macros  *
 * **************** */

#define BTK_FILE_CHOOSER_BUTTON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), BTK_TYPE_FILE_CHOOSER_BUTTON, BtkFileChooserButtonPrivate))

#define DEFAULT_TITLE		N_("Select a File")
#define DESKTOP_DISPLAY_NAME	N_("Desktop")
#define FALLBACK_DISPLAY_NAME	N_("(None)") /* this string is used in btk+/btk/tests/filechooser.c - change it there if you change it here */
#define FALLBACK_ICON_NAME	"stock_unknown"
#define FALLBACK_ICON_SIZE	16


/* ********************** *
 *  Private Enumerations  *
 * ********************** */

/* Property IDs */
enum
{
  PROP_0,

  PROP_DIALOG,
  PROP_FOCUS_ON_CLICK,
  PROP_TITLE,
  PROP_WIDTH_CHARS
};

/* Signals */
enum
{
  FILE_SET,
  LAST_SIGNAL
};

/* TreeModel Columns */
enum
{
  ICON_COLUMN,
  DISPLAY_NAME_COLUMN,
  TYPE_COLUMN,
  DATA_COLUMN,
  IS_FOLDER_COLUMN,
  CANCELLABLE_COLUMN,
  NUM_COLUMNS
};

/* TreeModel Row Types */
typedef enum
{
  ROW_TYPE_SPECIAL,
  ROW_TYPE_VOLUME,
  ROW_TYPE_SHORTCUT,
  ROW_TYPE_BOOKMARK_SEPARATOR,
  ROW_TYPE_BOOKMARK,
  ROW_TYPE_CURRENT_FOLDER_SEPARATOR,
  ROW_TYPE_CURRENT_FOLDER,
  ROW_TYPE_OTHER_SEPARATOR,
  ROW_TYPE_OTHER,
  ROW_TYPE_EMPTY_SELECTION,

  ROW_TYPE_INVALID = -1
}
RowType;


/* ******************** *
 *  Private Structures  *
 * ******************** */

struct _BtkFileChooserButtonPrivate
{
  BtkWidget *dialog;
  BtkWidget *button;
  BtkWidget *image;
  BtkWidget *label;
  BtkWidget *combo_box;
  BtkCellRenderer *icon_cell;
  BtkCellRenderer *name_cell;

  BtkTreeModel *model;
  BtkTreeModel *filter_model;

  BtkFileSystem *fs;
  GFile *selection_while_inactive;
  GFile *current_folder_while_inactive;

  gulong combo_box_changed_id;
  gulong fs_volumes_changed_id;
  gulong fs_bookmarks_changed_id;

  GCancellable *dnd_select_folder_cancellable;
  GCancellable *update_button_cancellable;
  GSList *change_icon_theme_cancellables;

  gint icon_size;

  guint8 n_special;
  guint8 n_volumes;
  guint8 n_shortcuts;
  guint8 n_bookmarks;
  guint  has_bookmark_separator       : 1;
  guint  has_current_folder_separator : 1;
  guint  has_current_folder           : 1;
  guint  has_other_separator          : 1;

  /* Used for hiding/showing the dialog when the button is hidden */
  guint  active                       : 1;

  guint  focus_on_click               : 1;

  /* Whether the next async callback from BUNNYIO should emit the "selection-changed" signal */
  guint  is_changing_selection        : 1;
};


/* ************* *
 *  DnD Support  *
 * ************* */

enum
{
  TEXT_PLAIN,
  TEXT_URI_LIST
};


/* ********************* *
 *  Function Prototypes  *
 * ********************* */

/* BtkFileChooserIface Functions */
static void     btk_file_chooser_button_file_chooser_iface_init (BtkFileChooserIface *iface);
static gboolean btk_file_chooser_button_set_current_folder (BtkFileChooser    *chooser,
							    GFile             *file,
							    GError           **error);
static GFile *btk_file_chooser_button_get_current_folder (BtkFileChooser    *chooser);
static gboolean btk_file_chooser_button_select_file (BtkFileChooser *chooser,
						     GFile          *file,
						     GError        **error);
static void btk_file_chooser_button_unselect_file (BtkFileChooser *chooser,
						   GFile          *file);
static void btk_file_chooser_button_unselect_all (BtkFileChooser *chooser);
static GSList *btk_file_chooser_button_get_files (BtkFileChooser *chooser);
static gboolean btk_file_chooser_button_add_shortcut_folder     (BtkFileChooser      *chooser,
								 GFile               *file,
								 GError             **error);
static gboolean btk_file_chooser_button_remove_shortcut_folder  (BtkFileChooser      *chooser,
								 GFile               *file,
								 GError             **error);

/* GObject Functions */
static GObject *btk_file_chooser_button_constructor        (GType             type,
							    guint             n_params,
							    GObjectConstructParam *params);
static void     btk_file_chooser_button_set_property       (GObject          *object,
							    guint             param_id,
							    const GValue     *value,
							    GParamSpec       *pspec);
static void     btk_file_chooser_button_get_property       (GObject          *object,
							    guint             param_id,
							    GValue           *value,
							    GParamSpec       *pspec);
static void     btk_file_chooser_button_finalize           (GObject          *object);

/* BtkObject Functions */
static void     btk_file_chooser_button_destroy            (BtkObject        *object);

/* BtkWidget Functions */
static void     btk_file_chooser_button_drag_data_received (BtkWidget        *widget,
							    BdkDragContext   *context,
							    gint              x,
							    gint              y,
							    BtkSelectionData *data,
							    guint             type,
							    guint             drag_time);
static void     btk_file_chooser_button_show_all           (BtkWidget        *widget);
static void     btk_file_chooser_button_hide_all           (BtkWidget        *widget);
static void     btk_file_chooser_button_show               (BtkWidget        *widget);
static void     btk_file_chooser_button_hide               (BtkWidget        *widget);
static void     btk_file_chooser_button_map                (BtkWidget        *widget);
static gboolean btk_file_chooser_button_mnemonic_activate  (BtkWidget        *widget,
							    gboolean          group_cycling);
static void     btk_file_chooser_button_style_set          (BtkWidget        *widget,
							    BtkStyle         *old_style);
static void     btk_file_chooser_button_screen_changed     (BtkWidget        *widget,
							    BdkScreen        *old_screen);

/* Utility Functions */
static BtkIconTheme *get_icon_theme               (BtkWidget            *widget);
static void          set_info_for_file_at_iter         (BtkFileChooserButton *fs,
							GFile                *file,
							BtkTreeIter          *iter);

static gint          model_get_type_position      (BtkFileChooserButton *button,
						   RowType               row_type);
static void          model_free_row_data          (BtkFileChooserButton *button,
						   BtkTreeIter          *iter);
static void          model_add_special            (BtkFileChooserButton *button);
static void          model_add_other              (BtkFileChooserButton *button);
static void          model_add_empty_selection    (BtkFileChooserButton *button);
static void          model_add_volumes            (BtkFileChooserButton *button,
						   GSList               *volumes);
static void          model_add_bookmarks          (BtkFileChooserButton *button,
						   GSList               *bookmarks);
static void          model_update_current_folder  (BtkFileChooserButton *button,
						   GFile                *file);
static void          model_remove_rows            (BtkFileChooserButton *button,
						   gint                  pos,
						   gint                  n_rows);

static gboolean      filter_model_visible_func    (BtkTreeModel         *model,
						   BtkTreeIter          *iter,
						   gpointer              user_data);

static gboolean      combo_box_row_separator_func (BtkTreeModel         *model,
						   BtkTreeIter          *iter,
						   gpointer              user_data);
static void          name_cell_data_func          (BtkCellLayout        *layout,
						   BtkCellRenderer      *cell,
						   BtkTreeModel         *model,
						   BtkTreeIter          *iter,
						   gpointer              user_data);
static void          open_dialog                  (BtkFileChooserButton *button);
static void          update_combo_box             (BtkFileChooserButton *button);
static void          update_label_and_image       (BtkFileChooserButton *button);

/* Child Object Callbacks */
static void     fs_volumes_changed_cb            (BtkFileSystem  *fs,
						  gpointer        user_data);
static void     fs_bookmarks_changed_cb          (BtkFileSystem  *fs,
						  gpointer        user_data);

static void     combo_box_changed_cb             (BtkComboBox    *combo_box,
						  gpointer        user_data);
static void     combo_box_notify_popup_shown_cb  (GObject        *object,
						  GParamSpec     *pspec,
						  gpointer        user_data);

static void     button_clicked_cb                (BtkButton      *real_button,
						  gpointer        user_data);

static void     dialog_update_preview_cb         (BtkFileChooser *dialog,
						  gpointer        user_data);
static void     dialog_notify_cb                 (GObject        *dialog,
						  GParamSpec     *pspec,
						  gpointer        user_data);
static gboolean dialog_delete_event_cb           (BtkWidget      *dialog,
						  BdkEvent       *event,
						  gpointer        user_data);
static void     dialog_response_cb               (BtkDialog      *dialog,
						  gint            response,
						  gpointer        user_data);

static guint file_chooser_button_signals[LAST_SIGNAL] = { 0 };

/* ******************* *
 *  GType Declaration  *
 * ******************* */

G_DEFINE_TYPE_WITH_CODE (BtkFileChooserButton, btk_file_chooser_button, BTK_TYPE_HBOX, { \
    G_IMPLEMENT_INTERFACE (BTK_TYPE_FILE_CHOOSER, btk_file_chooser_button_file_chooser_iface_init) \
})


/* ***************** *
 *  GType Functions  *
 * ***************** */

static void
btk_file_chooser_button_class_init (BtkFileChooserButtonClass * class)
{
  GObjectClass *bobject_class;
  BtkObjectClass *btkobject_class;
  BtkWidgetClass *widget_class;

  bobject_class = G_OBJECT_CLASS (class);
  btkobject_class = BTK_OBJECT_CLASS (class);
  widget_class = BTK_WIDGET_CLASS (class);

  bobject_class->constructor = btk_file_chooser_button_constructor;
  bobject_class->set_property = btk_file_chooser_button_set_property;
  bobject_class->get_property = btk_file_chooser_button_get_property;
  bobject_class->finalize = btk_file_chooser_button_finalize;

  btkobject_class->destroy = btk_file_chooser_button_destroy;

  widget_class->drag_data_received = btk_file_chooser_button_drag_data_received;
  widget_class->show_all = btk_file_chooser_button_show_all;
  widget_class->hide_all = btk_file_chooser_button_hide_all;
  widget_class->show = btk_file_chooser_button_show;
  widget_class->hide = btk_file_chooser_button_hide;
  widget_class->map = btk_file_chooser_button_map;
  widget_class->style_set = btk_file_chooser_button_style_set;
  widget_class->screen_changed = btk_file_chooser_button_screen_changed;
  widget_class->mnemonic_activate = btk_file_chooser_button_mnemonic_activate;

  /**
   * BtkFileChooserButton::file-set:
   * @widget: the object which received the signal.
   *
   * The ::file-set signal is emitted when the user selects a file.
   *
   * Note that this signal is only emitted when the <emphasis>user</emphasis>
   * changes the file.
   *
   * Since: 2.12
   */
  file_chooser_button_signals[FILE_SET] =
    g_signal_new (I_("file-set"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkFileChooserButtonClass, file_set),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * BtkFileChooserButton:dialog:
   *
   * Instance of the #BtkFileChooserDialog associated with the button.
   *
   * Since: 2.6
   */
  g_object_class_install_property (bobject_class, PROP_DIALOG,
				   g_param_spec_object ("dialog",
							P_("Dialog"),
							P_("The file chooser dialog to use."),
							BTK_TYPE_FILE_CHOOSER,
							(BTK_PARAM_WRITABLE |
							 G_PARAM_CONSTRUCT_ONLY)));

  /**
   * BtkFileChooserButton:focus-on-click:
   *
   * Whether the #BtkFileChooserButton button grabs focus when it is clicked
   * with the mouse.
   *
   * Since: 2.10
   */
  g_object_class_install_property (bobject_class,
                                   PROP_FOCUS_ON_CLICK,
                                   g_param_spec_boolean ("focus-on-click",
							 P_("Focus on click"),
							 P_("Whether the button grabs focus when it is clicked with the mouse"),
							 TRUE,
							 BTK_PARAM_READWRITE));

  /**
   * BtkFileChooserButton:title:
   *
   * Title to put on the #BtkFileChooserDialog associated with the button.
   *
   * Since: 2.6
   */
  g_object_class_install_property (bobject_class, PROP_TITLE,
				   g_param_spec_string ("title",
							P_("Title"),
							P_("The title of the file chooser dialog."),
							_(DEFAULT_TITLE),
							BTK_PARAM_READWRITE));

  /**
   * BtkFileChooserButton:width-chars:
   *
   * The width of the entry and label inside the button, in characters.
   *
   * Since: 2.6
   */
  g_object_class_install_property (bobject_class, PROP_WIDTH_CHARS,
				   g_param_spec_int ("width-chars",
						     P_("Width In Characters"),
						     P_("The desired width of the button widget, in characters."),
						     -1, G_MAXINT, -1,
						     BTK_PARAM_READWRITE));

  _btk_file_chooser_install_properties (bobject_class);

  g_type_class_add_private (class, sizeof (BtkFileChooserButtonPrivate));
}

static void
btk_file_chooser_button_init (BtkFileChooserButton *button)
{
  BtkFileChooserButtonPrivate *priv;
  BtkWidget *box, *image, *sep;
  BtkTargetList *target_list;

  priv = button->priv = BTK_FILE_CHOOSER_BUTTON_GET_PRIVATE (button);

  priv->icon_size = FALLBACK_ICON_SIZE;
  priv->focus_on_click = TRUE;

  btk_widget_push_composite_child ();

  /* Button */
  priv->button = btk_button_new ();
  g_signal_connect (priv->button, "clicked", G_CALLBACK (button_clicked_cb),
		    button);
  btk_container_add (BTK_CONTAINER (button), priv->button);
  btk_widget_show (priv->button);

  box = btk_hbox_new (FALSE, 4);
  btk_container_add (BTK_CONTAINER (priv->button), box);
  btk_widget_show (box);

  priv->image = btk_image_new ();
  btk_box_pack_start (BTK_BOX (box), priv->image, FALSE, FALSE, 0);
  btk_widget_show (priv->image);

  priv->label = btk_label_new (_(FALLBACK_DISPLAY_NAME));
  btk_label_set_ellipsize (BTK_LABEL (priv->label), BANGO_ELLIPSIZE_END);
  btk_misc_set_alignment (BTK_MISC (priv->label), 0.0, 0.5);
  btk_box_pack_start (BTK_BOX (box), priv->label, TRUE, TRUE, 0);
  //btk_container_add (BTK_CONTAINER (box), priv->label);
  btk_widget_show (priv->label);

  sep = btk_vseparator_new ();
  btk_box_pack_start (BTK_BOX (box), sep, FALSE, FALSE, 0);
  btk_widget_show (sep);

  image = btk_image_new_from_stock (BTK_STOCK_OPEN,
				    BTK_ICON_SIZE_MENU);
  btk_box_pack_start (BTK_BOX (box), image, FALSE, FALSE, 0);
  btk_widget_show (image);

  /* Combo Box */
  /* Keep in sync with columns enum, line 88 */
  priv->model =
    BTK_TREE_MODEL (btk_list_store_new (NUM_COLUMNS,
					BDK_TYPE_PIXBUF, /* ICON_COLUMN */
					G_TYPE_STRING,	 /* DISPLAY_NAME_COLUMN */
					G_TYPE_CHAR,	 /* TYPE_COLUMN */
					G_TYPE_POINTER	 /* DATA_COLUMN (Volume || Path) */,
					G_TYPE_BOOLEAN   /* IS_FOLDER_COLUMN */,
					G_TYPE_POINTER	 /* CANCELLABLE_COLUMN */));

  priv->combo_box = btk_combo_box_new ();
  priv->combo_box_changed_id = g_signal_connect (priv->combo_box, "changed",
						 G_CALLBACK (combo_box_changed_cb), button);

  g_signal_connect (priv->combo_box, "notify::popup-shown",
		    G_CALLBACK (combo_box_notify_popup_shown_cb), button);

  btk_container_add (BTK_CONTAINER (button), priv->combo_box);

  priv->icon_cell = btk_cell_renderer_pixbuf_new ();
  btk_cell_layout_pack_start (BTK_CELL_LAYOUT (priv->combo_box),
			      priv->icon_cell, FALSE);
  btk_cell_layout_add_attribute (BTK_CELL_LAYOUT (priv->combo_box),
				 priv->icon_cell, "pixbuf", ICON_COLUMN);

  priv->name_cell = btk_cell_renderer_text_new ();
  btk_cell_layout_pack_start (BTK_CELL_LAYOUT (priv->combo_box),
			      priv->name_cell, TRUE);
  btk_cell_layout_add_attribute (BTK_CELL_LAYOUT (priv->combo_box),
				 priv->name_cell, "text", DISPLAY_NAME_COLUMN);
  btk_cell_layout_set_cell_data_func (BTK_CELL_LAYOUT (priv->combo_box),
				      priv->name_cell, name_cell_data_func,
				      NULL, NULL);

  btk_widget_pop_composite_child ();

  /* DnD */
  btk_drag_dest_set (BTK_WIDGET (button),
                     (BTK_DEST_DEFAULT_ALL),
		     NULL, 0,
		     BDK_ACTION_COPY);
  target_list = btk_target_list_new (NULL, 0);
  btk_target_list_add_uri_targets (target_list, TEXT_URI_LIST);
  btk_target_list_add_text_targets (target_list, TEXT_PLAIN);
  btk_drag_dest_set_target_list (BTK_WIDGET (button), target_list);
  btk_target_list_unref (target_list);
}


/* ******************************* *
 *  BtkFileChooserIface Functions  *
 * ******************************* */
static void
btk_file_chooser_button_file_chooser_iface_init (BtkFileChooserIface *iface)
{
  _btk_file_chooser_delegate_iface_init (iface);

  iface->set_current_folder = btk_file_chooser_button_set_current_folder;
  iface->get_current_folder = btk_file_chooser_button_get_current_folder;
  iface->select_file = btk_file_chooser_button_select_file;
  iface->unselect_file = btk_file_chooser_button_unselect_file;
  iface->unselect_all = btk_file_chooser_button_unselect_all;
  iface->get_files = btk_file_chooser_button_get_files;
  iface->add_shortcut_folder = btk_file_chooser_button_add_shortcut_folder;
  iface->remove_shortcut_folder = btk_file_chooser_button_remove_shortcut_folder;
}

static void
emit_selection_changed_if_changing_selection (BtkFileChooserButton *button)
{
  BtkFileChooserButtonPrivate *priv = button->priv;

  if (priv->is_changing_selection)
    {
      priv->is_changing_selection = FALSE;
      g_signal_emit_by_name (button, "selection-changed");
    }
}

static gboolean
btk_file_chooser_button_set_current_folder (BtkFileChooser    *chooser,
					    GFile             *file,
					    GError           **error)
{
  BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (chooser);
  BtkFileChooserButtonPrivate *priv = button->priv;

  if (priv->current_folder_while_inactive)
    g_object_unref (priv->current_folder_while_inactive);

  priv->current_folder_while_inactive = g_object_ref (file);

  update_combo_box (button);

  g_signal_emit_by_name (button, "current-folder-changed");

  if (priv->active)
    btk_file_chooser_set_current_folder_file (BTK_FILE_CHOOSER (priv->dialog), file, NULL);

  return TRUE;
}

static GFile *
btk_file_chooser_button_get_current_folder (BtkFileChooser *chooser)
{
  BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (chooser);
  BtkFileChooserButtonPrivate *priv = button->priv;

  if (priv->current_folder_while_inactive)
    return g_object_ref (priv->current_folder_while_inactive);
  else
    return NULL;
}

static gboolean
btk_file_chooser_button_select_file (BtkFileChooser *chooser,
				     GFile          *file,
				     GError        **error)
{
  BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (chooser);
  BtkFileChooserButtonPrivate *priv = button->priv;

  if (priv->selection_while_inactive)
    g_object_unref (priv->selection_while_inactive);

  priv->selection_while_inactive = g_object_ref (file);

  priv->is_changing_selection = TRUE;

  update_label_and_image (button);
  update_combo_box (button);

  if (priv->active)
    btk_file_chooser_select_file (BTK_FILE_CHOOSER (priv->dialog), file, NULL);

  return TRUE;
}

static void
unselect_current_file (BtkFileChooserButton *button)
{
  BtkFileChooserButtonPrivate *priv = button->priv;

  if (priv->selection_while_inactive)
    {
      g_object_unref (priv->selection_while_inactive);
      priv->selection_while_inactive = NULL;
    }

  priv->is_changing_selection = TRUE;

  update_label_and_image (button);
  update_combo_box (button);
}

static void
btk_file_chooser_button_unselect_file (BtkFileChooser *chooser,
				       GFile          *file)
{
  BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (chooser);
  BtkFileChooserButtonPrivate *priv = button->priv;

  if (g_file_equal (priv->selection_while_inactive, file))
    unselect_current_file (button);

  if (priv->active)
    btk_file_chooser_unselect_file (BTK_FILE_CHOOSER (priv->dialog), file);
}

static void
btk_file_chooser_button_unselect_all (BtkFileChooser *chooser)
{
  BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (chooser);
  BtkFileChooserButtonPrivate *priv = button->priv;

  unselect_current_file (button);

  if (priv->active)
    btk_file_chooser_unselect_all (BTK_FILE_CHOOSER (priv->dialog));
}

static GFile *
get_selected_file (BtkFileChooserButton *button)
{
  BtkFileChooserButtonPrivate *priv = button->priv;
  GFile *retval;

  retval = NULL;

  if (priv->selection_while_inactive)
    retval = priv->selection_while_inactive;
  else if (btk_file_chooser_get_action (BTK_FILE_CHOOSER (priv->dialog)) == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)
    {
      /* If there is no "real" selection in SELECT_FOLDER mode, then we'll just return
       * the current folder, since that is what BtkFileChooserDefault would do.
       */
      if (priv->current_folder_while_inactive)
	retval = priv->current_folder_while_inactive;
    }

  if (retval)
    return g_object_ref (retval);
  else
    return NULL;
}

static GSList *
btk_file_chooser_button_get_files (BtkFileChooser *chooser)
{
  BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (chooser);
  GFile *file;

  file = get_selected_file (button);
  if (file)
    return g_slist_prepend (NULL, file);
  else
    return NULL;
}

static gboolean
btk_file_chooser_button_add_shortcut_folder (BtkFileChooser  *chooser,
					     GFile           *file,
					     GError         **error)
{
  BtkFileChooser *delegate;
  gboolean retval;

  delegate = g_object_get_qdata (G_OBJECT (chooser),
				 BTK_FILE_CHOOSER_DELEGATE_QUARK);
  retval = _btk_file_chooser_add_shortcut_folder (delegate, file, error);

  if (retval)
    {
      BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (chooser);
      BtkFileChooserButtonPrivate *priv = button->priv;
      BtkTreeIter iter;
      gint pos;

      pos = model_get_type_position (button, ROW_TYPE_SHORTCUT);
      pos += priv->n_shortcuts;

      btk_list_store_insert (BTK_LIST_STORE (priv->model), &iter, pos);
      btk_list_store_set (BTK_LIST_STORE (priv->model), &iter,
			  ICON_COLUMN, NULL,
			  DISPLAY_NAME_COLUMN, _(FALLBACK_DISPLAY_NAME),
			  TYPE_COLUMN, ROW_TYPE_SHORTCUT,
			  DATA_COLUMN, g_object_ref (file),
			  IS_FOLDER_COLUMN, FALSE,
			  -1);
      set_info_for_file_at_iter (button, file, &iter);
      priv->n_shortcuts++;

      btk_tree_model_filter_refilter (BTK_TREE_MODEL_FILTER (priv->filter_model));
    }

  return retval;
}

static gboolean
btk_file_chooser_button_remove_shortcut_folder (BtkFileChooser  *chooser,
						GFile           *file,
						GError         **error)
{
  BtkFileChooser *delegate;
  gboolean retval;

  delegate = g_object_get_qdata (G_OBJECT (chooser),
				 BTK_FILE_CHOOSER_DELEGATE_QUARK);

  retval = _btk_file_chooser_remove_shortcut_folder (delegate, file, error);

  if (retval)
    {
      BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (chooser);
      BtkFileChooserButtonPrivate *priv = button->priv;
      BtkTreeIter iter;
      gint pos;
      gchar type;

      pos = model_get_type_position (button, ROW_TYPE_SHORTCUT);
      btk_tree_model_iter_nth_child (priv->model, &iter, NULL, pos);

      do
	{
	  gpointer data;

	  btk_tree_model_get (priv->model, &iter,
			      TYPE_COLUMN, &type,
			      DATA_COLUMN, &data,
			      -1);

	  if (type == ROW_TYPE_SHORTCUT &&
	      data && g_file_equal (data, file))
	    {
	      model_free_row_data (BTK_FILE_CHOOSER_BUTTON (chooser), &iter);
	      btk_list_store_remove (BTK_LIST_STORE (priv->model), &iter);
	      priv->n_shortcuts--;
	      btk_tree_model_filter_refilter (BTK_TREE_MODEL_FILTER (priv->filter_model));
	      update_combo_box (BTK_FILE_CHOOSER_BUTTON (chooser));
	      break;
	    }
	}
      while (type == ROW_TYPE_SHORTCUT &&
	     btk_tree_model_iter_next (priv->model, &iter));
    }

  return retval;
}


/* ******************* *
 *  GObject Functions  *
 * ******************* */

static GObject *
btk_file_chooser_button_constructor (GType                  type,
				     guint                  n_params,
				     GObjectConstructParam *params)
{
  GObject *object;
  BtkFileChooserButton *button;
  BtkFileChooserButtonPrivate *priv;
  GSList *list;

  object = G_OBJECT_CLASS (btk_file_chooser_button_parent_class)->constructor (type,
									       n_params,
									       params);
  button = BTK_FILE_CHOOSER_BUTTON (object);
  priv = button->priv;

  if (!priv->dialog)
    {
      priv->dialog = btk_file_chooser_dialog_new (NULL, NULL,
						  BTK_FILE_CHOOSER_ACTION_OPEN,
						  BTK_STOCK_CANCEL,
						  BTK_RESPONSE_CANCEL,
						  BTK_STOCK_OPEN,
						  BTK_RESPONSE_ACCEPT,
						  NULL);

      btk_dialog_set_default_response (BTK_DIALOG (priv->dialog),
				       BTK_RESPONSE_ACCEPT);
      btk_dialog_set_alternative_button_order (BTK_DIALOG (priv->dialog),
					       BTK_RESPONSE_ACCEPT,
					       BTK_RESPONSE_CANCEL,
					       -1);

      btk_file_chooser_button_set_title (button, _(DEFAULT_TITLE));
    }
  else if (!btk_window_get_title (BTK_WINDOW (priv->dialog)))
    {
      btk_file_chooser_button_set_title (button, _(DEFAULT_TITLE));
    }

  g_signal_connect (priv->dialog, "delete-event",
		    G_CALLBACK (dialog_delete_event_cb), object);
  g_signal_connect (priv->dialog, "response",
		    G_CALLBACK (dialog_response_cb), object);

  /* This is used, instead of the standard delegate, to ensure that signals are only
   * delegated when the OK button is pressed. */
  g_object_set_qdata (object, BTK_FILE_CHOOSER_DELEGATE_QUARK, priv->dialog);

  g_signal_connect (priv->dialog, "update-preview",
		    G_CALLBACK (dialog_update_preview_cb), object);
  g_signal_connect (priv->dialog, "notify",
		    G_CALLBACK (dialog_notify_cb), object);
  g_object_add_weak_pointer (G_OBJECT (priv->dialog),
			     (gpointer) (&priv->dialog));

  priv->fs =
    g_object_ref (_btk_file_chooser_get_file_system (BTK_FILE_CHOOSER (priv->dialog)));

  model_add_special (button);

  list = _btk_file_system_list_volumes (priv->fs);
  model_add_volumes (button, list);
  g_slist_free (list);

  list = _btk_file_system_list_bookmarks (priv->fs);
  model_add_bookmarks (button, list);
  g_slist_foreach (list, (GFunc) g_object_unref, NULL);
  g_slist_free (list);

  model_add_other (button);

  model_add_empty_selection (button);

  priv->filter_model = btk_tree_model_filter_new (priv->model, NULL);
  btk_tree_model_filter_set_visible_func (BTK_TREE_MODEL_FILTER (priv->filter_model),
					  filter_model_visible_func,
					  object, NULL);

  btk_combo_box_set_model (BTK_COMBO_BOX (priv->combo_box), priv->filter_model);
  btk_combo_box_set_row_separator_func (BTK_COMBO_BOX (priv->combo_box),
					combo_box_row_separator_func,
					NULL, NULL);

  /* set up the action for a user-provided dialog, this also updates
   * the label, image and combobox
   */
  g_object_set (object,
		"action", btk_file_chooser_get_action (BTK_FILE_CHOOSER (priv->dialog)),
		NULL);

  priv->fs_volumes_changed_id =
    g_signal_connect (priv->fs, "volumes-changed",
		      G_CALLBACK (fs_volumes_changed_cb), object);
  priv->fs_bookmarks_changed_id =
    g_signal_connect (priv->fs, "bookmarks-changed",
		      G_CALLBACK (fs_bookmarks_changed_cb), object);

  update_label_and_image (button);
  update_combo_box (button);

  return object;
}

static void
btk_file_chooser_button_set_property (GObject      *object,
				      guint         param_id,
				      const GValue *value,
				      GParamSpec   *pspec)
{
  BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (object);
  BtkFileChooserButtonPrivate *priv = button->priv;

  switch (param_id)
    {
    case PROP_DIALOG:
      /* Construct-only */
      priv->dialog = g_value_get_object (value);
      break;
    case PROP_FOCUS_ON_CLICK:
      btk_file_chooser_button_set_focus_on_click (button, g_value_get_boolean (value));
      break;
    case PROP_WIDTH_CHARS:
      btk_file_chooser_button_set_width_chars (BTK_FILE_CHOOSER_BUTTON (object),
					       g_value_get_int (value));
      break;
    case BTK_FILE_CHOOSER_PROP_ACTION:
      switch (g_value_get_enum (value))
	{
	case BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER:
	case BTK_FILE_CHOOSER_ACTION_SAVE:
	  {
	    GEnumClass *eclass;
	    GEnumValue *eval;

	    eclass = g_type_class_peek (BTK_TYPE_FILE_CHOOSER_ACTION);
	    eval = g_enum_get_value (eclass, g_value_get_enum (value));
	    g_warning ("%s: Choosers of type `%s' do not support `%s'.",
		       B_STRFUNC, G_OBJECT_TYPE_NAME (object), eval->value_name);

	    g_value_set_enum ((GValue *) value, BTK_FILE_CHOOSER_ACTION_OPEN);
	  }
	  break;
	}

      g_object_set_property (G_OBJECT (priv->dialog), pspec->name, value);
      update_label_and_image (BTK_FILE_CHOOSER_BUTTON (object));
      update_combo_box (BTK_FILE_CHOOSER_BUTTON (object));

      switch (g_value_get_enum (value))
	{
	case BTK_FILE_CHOOSER_ACTION_OPEN:
	  btk_widget_hide (priv->combo_box);
	  btk_widget_show (priv->button);
	  break;
	case BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER:
	  btk_widget_hide (priv->button);
	  btk_widget_show (priv->combo_box);
	  break;
	default:
	  g_assert_not_reached ();
	  break;
	}
      break;

    case PROP_TITLE:
    case BTK_FILE_CHOOSER_PROP_FILTER:
    case BTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET:
    case BTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET_ACTIVE:
    case BTK_FILE_CHOOSER_PROP_USE_PREVIEW_LABEL:
    case BTK_FILE_CHOOSER_PROP_EXTRA_WIDGET:
    case BTK_FILE_CHOOSER_PROP_SHOW_HIDDEN:
    case BTK_FILE_CHOOSER_PROP_DO_OVERWRITE_CONFIRMATION:
    case BTK_FILE_CHOOSER_PROP_CREATE_FOLDERS:
      g_object_set_property (G_OBJECT (priv->dialog), pspec->name, value);
      break;

    case BTK_FILE_CHOOSER_PROP_LOCAL_ONLY:
      g_object_set_property (G_OBJECT (priv->dialog), pspec->name, value);
      fs_volumes_changed_cb (priv->fs, button);
      fs_bookmarks_changed_cb (priv->fs, button);
      break;

    case BTK_FILE_CHOOSER_PROP_FILE_SYSTEM_BACKEND:
      /* Ignore property */
      break;

    case BTK_FILE_CHOOSER_PROP_SELECT_MULTIPLE:
      g_warning ("%s: Choosers of type `%s` do not support selecting multiple files.",
		 B_STRFUNC, G_OBJECT_TYPE_NAME (object));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
btk_file_chooser_button_get_property (GObject    *object,
				      guint       param_id,
				      GValue     *value,
				      GParamSpec *pspec)
{
  BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (object);
  BtkFileChooserButtonPrivate *priv = button->priv;

  switch (param_id)
    {
    case PROP_WIDTH_CHARS:
      g_value_set_int (value,
		       btk_label_get_width_chars (BTK_LABEL (priv->label)));
      break;
    case PROP_FOCUS_ON_CLICK:
      g_value_set_boolean (value,
                           btk_file_chooser_button_get_focus_on_click (button));
      break;

    case PROP_TITLE:
    case BTK_FILE_CHOOSER_PROP_ACTION:
    case BTK_FILE_CHOOSER_PROP_FILE_SYSTEM_BACKEND:
    case BTK_FILE_CHOOSER_PROP_FILTER:
    case BTK_FILE_CHOOSER_PROP_LOCAL_ONLY:
    case BTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET:
    case BTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET_ACTIVE:
    case BTK_FILE_CHOOSER_PROP_USE_PREVIEW_LABEL:
    case BTK_FILE_CHOOSER_PROP_EXTRA_WIDGET:
    case BTK_FILE_CHOOSER_PROP_SELECT_MULTIPLE:
    case BTK_FILE_CHOOSER_PROP_SHOW_HIDDEN:
    case BTK_FILE_CHOOSER_PROP_DO_OVERWRITE_CONFIRMATION:
    case BTK_FILE_CHOOSER_PROP_CREATE_FOLDERS:
      g_object_get_property (G_OBJECT (priv->dialog), pspec->name, value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
btk_file_chooser_button_finalize (GObject *object)
{
  BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (object);
  BtkFileChooserButtonPrivate *priv = button->priv;

  if (priv->selection_while_inactive)
    g_object_unref (priv->selection_while_inactive);

  if (priv->current_folder_while_inactive)
    g_object_unref (priv->current_folder_while_inactive);

  G_OBJECT_CLASS (btk_file_chooser_button_parent_class)->finalize (object);
}

/* ********************* *
 *  BtkObject Functions  *
 * ********************* */

static void
btk_file_chooser_button_destroy (BtkObject *object)
{
  BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (object);
  BtkFileChooserButtonPrivate *priv = button->priv;
  BtkTreeIter iter;
  GSList *l;

  if (priv->dialog != NULL)
    {
      btk_widget_destroy (priv->dialog);
      priv->dialog = NULL;
    }

  if (priv->model && btk_tree_model_get_iter_first (priv->model, &iter)) do
    {
      model_free_row_data (button, &iter);
    }
  while (btk_tree_model_iter_next (priv->model, &iter));

  if (priv->dnd_select_folder_cancellable)
    {
      g_cancellable_cancel (priv->dnd_select_folder_cancellable);
      priv->dnd_select_folder_cancellable = NULL;
    }

  if (priv->update_button_cancellable)
    {
      g_cancellable_cancel (priv->update_button_cancellable);
      priv->update_button_cancellable = NULL;
    }

  if (priv->change_icon_theme_cancellables)
    {
      for (l = priv->change_icon_theme_cancellables; l; l = l->next)
        {
	  GCancellable *cancellable = G_CANCELLABLE (l->data);
	  g_cancellable_cancel (cancellable);
        }
      g_slist_free (priv->change_icon_theme_cancellables);
      priv->change_icon_theme_cancellables = NULL;
    }

  if (priv->model)
    {
      g_object_unref (priv->model);
      priv->model = NULL;
    }

  if (priv->filter_model)
    {
      g_object_unref (priv->filter_model);
      priv->filter_model = NULL;
    }

  if (priv->fs)
    {
      g_signal_handler_disconnect (priv->fs, priv->fs_volumes_changed_id);
      g_signal_handler_disconnect (priv->fs, priv->fs_bookmarks_changed_id);
      g_object_unref (priv->fs);
      priv->fs = NULL;
    }

  BTK_OBJECT_CLASS (btk_file_chooser_button_parent_class)->destroy (object);
}


/* ********************* *
 *  BtkWidget Functions  *
 * ********************* */

struct DndSelectFolderData
{
  BtkFileSystem *file_system;
  BtkFileChooserButton *button;
  BtkFileChooserAction action;
  GFile *file;
  gchar **uris;
  guint i;
  gboolean selected;
};

static void
dnd_select_folder_get_info_cb (GCancellable *cancellable,
			       GFileInfo    *info,
			       const GError *error,
			       gpointer      user_data)
{
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  struct DndSelectFolderData *data = user_data;

  if (cancellable != data->button->priv->dnd_select_folder_cancellable)
    {
      g_object_unref (data->button);
      g_object_unref (data->file);
      g_strfreev (data->uris);
      g_free (data);

      g_object_unref (cancellable);
      return;
    }

  data->button->priv->dnd_select_folder_cancellable = NULL;

  if (!cancelled && !error && info != NULL)
    {
      gboolean is_folder;

      is_folder = _btk_file_info_consider_as_directory (info);

      data->selected =
	(((data->action == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER && is_folder) ||
	  (data->action == BTK_FILE_CHOOSER_ACTION_OPEN && !is_folder)) &&
	 btk_file_chooser_select_file (BTK_FILE_CHOOSER (data->button->priv->dialog),
				       data->file, NULL));
    }
  else
    data->selected = FALSE;

  if (data->selected || data->uris[++data->i] == NULL)
    {
      g_signal_emit (data->button, file_chooser_button_signals[FILE_SET], 0);

      g_object_unref (data->button);
      g_object_unref (data->file);
      g_strfreev (data->uris);
      g_free (data);

      g_object_unref (cancellable);
      return;
    }

  if (data->file)
    g_object_unref (data->file);

  data->file = g_file_new_for_uri (data->uris[data->i]);

  data->button->priv->dnd_select_folder_cancellable =
    _btk_file_system_get_info (data->file_system, data->file,
			       "standard::type",
			       dnd_select_folder_get_info_cb, user_data);

  g_object_unref (cancellable);
}

static void
btk_file_chooser_button_drag_data_received (BtkWidget	     *widget,
					    BdkDragContext   *context,
					    gint	      x,
					    gint	      y,
					    BtkSelectionData *data,
					    guint	      type,
					    guint	      drag_time)
{
  BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (widget);
  BtkFileChooserButtonPrivate *priv = button->priv;
  GFile *file;
  gchar *text;

  if (BTK_WIDGET_CLASS (btk_file_chooser_button_parent_class)->drag_data_received != NULL)
    BTK_WIDGET_CLASS (btk_file_chooser_button_parent_class)->drag_data_received (widget,
										 context,
										 x, y,
										 data, type,
										 drag_time);

  if (widget == NULL || context == NULL || data == NULL || btk_selection_data_get_length (data) < 0)
    return;

  switch (type)
    {
    case TEXT_URI_LIST:
      {
	gchar **uris;
	struct DndSelectFolderData *info;

	uris = btk_selection_data_get_uris (data);

	if (uris == NULL)
	  break;

	info = g_new0 (struct DndSelectFolderData, 1);
	info->button = g_object_ref (button);
	info->i = 0;
	info->uris = uris;
	info->selected = FALSE;
	info->file_system = priv->fs;
	g_object_get (priv->dialog, "action", &info->action, NULL);

	info->file = g_file_new_for_uri (info->uris[info->i]);

	if (priv->dnd_select_folder_cancellable)
	  g_cancellable_cancel (priv->dnd_select_folder_cancellable);

	priv->dnd_select_folder_cancellable =
	  _btk_file_system_get_info (priv->fs, info->file,
				     "standard::type",
				     dnd_select_folder_get_info_cb, info);
      }
      break;

    case TEXT_PLAIN:
      text = (char*) btk_selection_data_get_text (data);
      file = g_file_new_for_uri (text);
      btk_file_chooser_select_file (BTK_FILE_CHOOSER (priv->dialog), file,
				    NULL);
      g_object_unref (file);
      g_free (text);
      g_signal_emit (button, file_chooser_button_signals[FILE_SET], 0);
      break;

    default:
      break;
    }

  btk_drag_finish (context, TRUE, FALSE, drag_time);
}

static void
btk_file_chooser_button_show_all (BtkWidget *widget)
{
  btk_widget_show (widget);
}

static void
btk_file_chooser_button_hide_all (BtkWidget *widget)
{
  btk_widget_hide (widget);
}

static void
btk_file_chooser_button_show (BtkWidget *widget)
{
  BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (widget);
  BtkFileChooserButtonPrivate *priv = button->priv;

  if (BTK_WIDGET_CLASS (btk_file_chooser_button_parent_class)->show)
    BTK_WIDGET_CLASS (btk_file_chooser_button_parent_class)->show (widget);

  if (priv->active)
    open_dialog (BTK_FILE_CHOOSER_BUTTON (widget));
}

static void
btk_file_chooser_button_hide (BtkWidget *widget)
{
  BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (widget);
  BtkFileChooserButtonPrivate *priv = button->priv;

  btk_widget_hide (priv->dialog);

  if (BTK_WIDGET_CLASS (btk_file_chooser_button_parent_class)->hide)
    BTK_WIDGET_CLASS (btk_file_chooser_button_parent_class)->hide (widget);
}

static void
btk_file_chooser_button_map (BtkWidget *widget)
{
  BTK_WIDGET_CLASS (btk_file_chooser_button_parent_class)->map (widget);
}

static gboolean
btk_file_chooser_button_mnemonic_activate (BtkWidget *widget,
					   gboolean   group_cycling)
{
  BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (widget);
  BtkFileChooserButtonPrivate *priv = button->priv;

  switch (btk_file_chooser_get_action (BTK_FILE_CHOOSER (priv->dialog)))
    {
    case BTK_FILE_CHOOSER_ACTION_OPEN:
      btk_widget_grab_focus (priv->button);
      break;
    case BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER:
      return btk_widget_mnemonic_activate (priv->combo_box, group_cycling);
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  return TRUE;
}

/* Changes the icons wherever it is needed */
struct ChangeIconThemeData
{
  BtkFileChooserButton *button;
  BtkTreeRowReference *row_ref;
};

static void
change_icon_theme_get_info_cb (GCancellable *cancellable,
			       GFileInfo    *info,
			       const GError *error,
			       gpointer      user_data)
{
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  BdkPixbuf *pixbuf;
  struct ChangeIconThemeData *data = user_data;

  if (!g_slist_find (data->button->priv->change_icon_theme_cancellables, cancellable))
    goto out;

  data->button->priv->change_icon_theme_cancellables =
    g_slist_remove (data->button->priv->change_icon_theme_cancellables, cancellable);

  if (cancelled || error)
    goto out;

  pixbuf = _btk_file_info_render_icon (info, BTK_WIDGET (data->button), data->button->priv->icon_size);

  if (pixbuf)
    {
      gint width = 0;
      BtkTreeIter iter;
      BtkTreePath *path;

      width = MAX (width, bdk_pixbuf_get_width (pixbuf));

      path = btk_tree_row_reference_get_path (data->row_ref);
      if (path)
        {
          btk_tree_model_get_iter (data->button->priv->model, &iter, path);
          btk_tree_path_free (path);

          btk_list_store_set (BTK_LIST_STORE (data->button->priv->model), &iter,
	  		      ICON_COLUMN, pixbuf,
			      -1);

          g_object_set (data->button->priv->icon_cell,
		        "width", width,
		        NULL);
        }
      g_object_unref (pixbuf);
    }

out:
  g_object_unref (data->button);
  btk_tree_row_reference_free (data->row_ref);
  g_free (data);

  g_object_unref (cancellable);
}

static void
change_icon_theme (BtkFileChooserButton *button)
{
  BtkFileChooserButtonPrivate *priv = button->priv;
  BtkSettings *settings;
  BtkIconTheme *theme;
  BtkTreeIter iter;
  GSList *l;
  gint width = 0, height = 0;

  for (l = button->priv->change_icon_theme_cancellables; l; l = l->next)
    {
      GCancellable *cancellable = G_CANCELLABLE (l->data);
      g_cancellable_cancel (cancellable);
    }
  g_slist_free (button->priv->change_icon_theme_cancellables);
  button->priv->change_icon_theme_cancellables = NULL;

  settings = btk_settings_get_for_screen (btk_widget_get_screen (BTK_WIDGET (button)));

  if (btk_icon_size_lookup_for_settings (settings, BTK_ICON_SIZE_MENU,
					 &width, &height))
    priv->icon_size = MAX (width, height);
  else
    priv->icon_size = FALLBACK_ICON_SIZE;

  update_label_and_image (button);

  btk_tree_model_get_iter_first (priv->model, &iter);

  theme = get_icon_theme (BTK_WIDGET (button));

  do
    {
      BdkPixbuf *pixbuf;
      gchar type;
      gpointer data;

      type = ROW_TYPE_INVALID;
      btk_tree_model_get (priv->model, &iter,
			  TYPE_COLUMN, &type,
			  DATA_COLUMN, &data,
			  -1);

      switch (type)
	{
	case ROW_TYPE_SPECIAL:
	case ROW_TYPE_SHORTCUT:
	case ROW_TYPE_BOOKMARK:
	case ROW_TYPE_CURRENT_FOLDER:
	  if (data)
	    {
	      if (g_file_is_native (G_FILE (data)))
		{
		  BtkTreePath *path;
		  GCancellable *cancellable;
		  struct ChangeIconThemeData *info;

		  info = g_new0 (struct ChangeIconThemeData, 1);
		  info->button = g_object_ref (button);
		  path = btk_tree_model_get_path (priv->model, &iter);
		  info->row_ref = btk_tree_row_reference_new (priv->model, path);
		  btk_tree_path_free (path);

		  cancellable =
		    _btk_file_system_get_info (priv->fs, data,
					       "standard::icon",
					       change_icon_theme_get_info_cb,
					       info);
		  button->priv->change_icon_theme_cancellables =
		    g_slist_append (button->priv->change_icon_theme_cancellables, cancellable);
		  pixbuf = NULL;
		}
	      else
		/* Don't call get_info for remote paths to avoid latency and
		 * auth dialogs.
		 * If we switch to a better bookmarks file format (XBEL), we
		 * should use mime info to get a better icon.
		 */
		pixbuf = btk_icon_theme_load_icon (theme, "folder-remote",
						   priv->icon_size, 0, NULL);
	    }
	  else
	    pixbuf = btk_icon_theme_load_icon (theme, FALLBACK_ICON_NAME,
					       priv->icon_size, 0, NULL);
	  break;
	case ROW_TYPE_VOLUME:
	  if (data)
	    pixbuf = _btk_file_system_volume_render_icon (data,
							  BTK_WIDGET (button),
							  priv->icon_size,
							  NULL);
	  else
	    pixbuf = btk_icon_theme_load_icon (theme, FALLBACK_ICON_NAME,
					       priv->icon_size, 0, NULL);
	  break;
	default:
	  continue;
	  break;
	}

      if (pixbuf)
	width = MAX (width, bdk_pixbuf_get_width (pixbuf));

      btk_list_store_set (BTK_LIST_STORE (priv->model), &iter,
			  ICON_COLUMN, pixbuf,
			  -1);

      if (pixbuf)
	g_object_unref (pixbuf);
    }
  while (btk_tree_model_iter_next (priv->model, &iter));

  g_object_set (button->priv->icon_cell,
		"width", width,
		NULL);
}

static void
btk_file_chooser_button_style_set (BtkWidget *widget,
				   BtkStyle  *old_style)
{
  BTK_WIDGET_CLASS (btk_file_chooser_button_parent_class)->style_set (widget,
								      old_style);

  if (btk_widget_has_screen (widget))
    change_icon_theme (BTK_FILE_CHOOSER_BUTTON (widget));
}

static void
btk_file_chooser_button_screen_changed (BtkWidget *widget,
					BdkScreen *old_screen)
{
  if (BTK_WIDGET_CLASS (btk_file_chooser_button_parent_class)->screen_changed)
    BTK_WIDGET_CLASS (btk_file_chooser_button_parent_class)->screen_changed (widget,
									     old_screen);

  change_icon_theme (BTK_FILE_CHOOSER_BUTTON (widget));
}


/* ******************* *
 *  Utility Functions  *
 * ******************* */

/* General */
static BtkIconTheme *
get_icon_theme (BtkWidget *widget)
{
  if (btk_widget_has_screen (widget))
    return btk_icon_theme_get_for_screen (btk_widget_get_screen (widget));

  return btk_icon_theme_get_default ();
}


struct SetDisplayNameData
{
  BtkFileChooserButton *button;
  char *label;
  BtkTreeRowReference *row_ref;
};

static void
set_info_get_info_cb (GCancellable *cancellable,
		      GFileInfo    *info,
		      const GError *error,
		      gpointer      callback_data)
{
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  BdkPixbuf *pixbuf;
  BtkTreePath *path;
  BtkTreeIter iter;
  GCancellable *model_cancellable;
  struct SetDisplayNameData *data = callback_data;
  gboolean is_folder;

  if (!data->button->priv->model)
    /* button got destroyed */
    goto out;

  path = btk_tree_row_reference_get_path (data->row_ref);
  if (!path)
    /* Cancellable doesn't exist anymore in the model */
    goto out;

  btk_tree_model_get_iter (data->button->priv->model, &iter, path);
  btk_tree_path_free (path);

  /* Validate the cancellable */
  btk_tree_model_get (data->button->priv->model, &iter,
		      CANCELLABLE_COLUMN, &model_cancellable,
		      -1);
  if (cancellable != model_cancellable)
    goto out;

  btk_list_store_set (BTK_LIST_STORE (data->button->priv->model), &iter,
		      CANCELLABLE_COLUMN, NULL,
		      -1);

  if (cancelled || error)
    /* There was an error, leave the fallback name in there */
    goto out;

  pixbuf = _btk_file_info_render_icon (info, BTK_WIDGET (data->button), data->button->priv->icon_size);

  if (!data->label)
    data->label = g_strdup (g_file_info_get_display_name (info));

  is_folder = _btk_file_info_consider_as_directory (info);

  btk_list_store_set (BTK_LIST_STORE (data->button->priv->model), &iter,
		      ICON_COLUMN, pixbuf,
		      DISPLAY_NAME_COLUMN, data->label,
		      IS_FOLDER_COLUMN, is_folder,
		      -1);

  if (pixbuf)
    g_object_unref (pixbuf);

out:
  g_object_unref (data->button);
  g_free (data->label);
  btk_tree_row_reference_free (data->row_ref);
  g_free (data);

  g_object_unref (cancellable);
}

static void
set_info_for_file_at_iter (BtkFileChooserButton *button,
			   GFile                *file,
			   BtkTreeIter          *iter)
{
  struct SetDisplayNameData *data;
  BtkTreePath *tree_path;
  GCancellable *cancellable;

  data = g_new0 (struct SetDisplayNameData, 1);
  data->button = g_object_ref (button);
  data->label = _btk_file_system_get_bookmark_label (button->priv->fs, file);

  tree_path = btk_tree_model_get_path (button->priv->model, iter);
  data->row_ref = btk_tree_row_reference_new (button->priv->model, tree_path);
  btk_tree_path_free (tree_path);

  cancellable = _btk_file_system_get_info (button->priv->fs, file,
					   "standard::type,standard::icon,standard::display-name",
					   set_info_get_info_cb, data);

  btk_list_store_set (BTK_LIST_STORE (button->priv->model), iter,
		      CANCELLABLE_COLUMN, cancellable,
		      -1);
}

/* Shortcuts Model */
static gint
model_get_type_position (BtkFileChooserButton *button,
			 RowType               row_type)
{
  gint retval = 0;

  if (row_type == ROW_TYPE_SPECIAL)
    return retval;

  retval += button->priv->n_special;

  if (row_type == ROW_TYPE_VOLUME)
    return retval;

  retval += button->priv->n_volumes;

  if (row_type == ROW_TYPE_SHORTCUT)
    return retval;

  retval += button->priv->n_shortcuts;

  if (row_type == ROW_TYPE_BOOKMARK_SEPARATOR)
    return retval;

  retval += button->priv->has_bookmark_separator;

  if (row_type == ROW_TYPE_BOOKMARK)
    return retval;

  retval += button->priv->n_bookmarks;

  if (row_type == ROW_TYPE_CURRENT_FOLDER_SEPARATOR)
    return retval;

  retval += button->priv->has_current_folder_separator;

  if (row_type == ROW_TYPE_CURRENT_FOLDER)
    return retval;

  retval += button->priv->has_current_folder;

  if (row_type == ROW_TYPE_OTHER_SEPARATOR)
    return retval;

  retval += button->priv->has_other_separator;

  if (row_type == ROW_TYPE_OTHER)
    return retval;

  retval++;

  if (row_type == ROW_TYPE_EMPTY_SELECTION)
    return retval;

  g_assert_not_reached ();
  return -1;
}

static void
model_free_row_data (BtkFileChooserButton *button,
		     BtkTreeIter          *iter)
{
  gchar type;
  gpointer data;
  GCancellable *cancellable;

  btk_tree_model_get (button->priv->model, iter,
		      TYPE_COLUMN, &type,
		      DATA_COLUMN, &data,
		      CANCELLABLE_COLUMN, &cancellable,
		      -1);

  if (cancellable)
    g_cancellable_cancel (cancellable);

  switch (type)
    {
    case ROW_TYPE_SPECIAL:
    case ROW_TYPE_SHORTCUT:
    case ROW_TYPE_BOOKMARK:
    case ROW_TYPE_CURRENT_FOLDER:
      g_object_unref (data);
      break;
    case ROW_TYPE_VOLUME:
      _btk_file_system_volume_unref (data);
      break;
    default:
      break;
    }
}

static void
model_add_special_get_info_cb (GCancellable *cancellable,
			       GFileInfo    *info,
			       const GError *error,
			       gpointer      user_data)
{
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  BtkTreeIter iter;
  BtkTreePath *path;
  BdkPixbuf *pixbuf;
  GCancellable *model_cancellable;
  struct ChangeIconThemeData *data = user_data;
  gchar *name;

  if (!data->button->priv->model)
    /* button got destroyed */
    goto out;

  path = btk_tree_row_reference_get_path (data->row_ref);
  if (!path)
    /* Cancellable doesn't exist anymore in the model */
    goto out;

  btk_tree_model_get_iter (data->button->priv->model, &iter, path);
  btk_tree_path_free (path);

  btk_tree_model_get (data->button->priv->model, &iter,
		      CANCELLABLE_COLUMN, &model_cancellable,
		      -1);
  if (cancellable != model_cancellable)
    goto out;

  btk_list_store_set (BTK_LIST_STORE (data->button->priv->model), &iter,
		      CANCELLABLE_COLUMN, NULL,
		      -1);

  if (cancelled || error)
    goto out;

  pixbuf = _btk_file_info_render_icon (info, BTK_WIDGET (data->button), data->button->priv->icon_size);

  if (pixbuf)
    {
      btk_list_store_set (BTK_LIST_STORE (data->button->priv->model), &iter,
			  ICON_COLUMN, pixbuf,
			  -1);
      g_object_unref (pixbuf);
    }

  btk_tree_model_get (data->button->priv->model, &iter,
                      DISPLAY_NAME_COLUMN, &name,
                      -1);
  if (!name)
    btk_list_store_set (BTK_LIST_STORE (data->button->priv->model), &iter,
  		        DISPLAY_NAME_COLUMN, g_file_info_get_display_name (info),
		        -1);
  g_free (name);

out:
  g_object_unref (data->button);
  btk_tree_row_reference_free (data->row_ref);
  g_free (data);

  g_object_unref (cancellable);
}

static void
model_add_special (BtkFileChooserButton *button)
{
  const gchar *homedir;
  const gchar *desktopdir;
  BtkListStore *store;
  BtkTreeIter iter;
  GFile *file;
  gint pos;

  store = BTK_LIST_STORE (button->priv->model);
  pos = model_get_type_position (button, ROW_TYPE_SPECIAL);

  homedir = g_get_home_dir ();

  if (homedir)
    {
      BtkTreePath *tree_path;
      GCancellable *cancellable;
      struct ChangeIconThemeData *info;

      file = g_file_new_for_path (homedir);
      btk_list_store_insert (store, &iter, pos);
      pos++;

      info = g_new0 (struct ChangeIconThemeData, 1);
      info->button = g_object_ref (button);
      tree_path = btk_tree_model_get_path (BTK_TREE_MODEL (store), &iter);
      info->row_ref = btk_tree_row_reference_new (BTK_TREE_MODEL (store),
						  tree_path);
      btk_tree_path_free (tree_path);

      cancellable = _btk_file_system_get_info (button->priv->fs, file,
					       "standard::icon,standard::display-name",
					       model_add_special_get_info_cb, info);

      btk_list_store_set (store, &iter,
			  ICON_COLUMN, NULL,
			  DISPLAY_NAME_COLUMN, NULL,
			  TYPE_COLUMN, ROW_TYPE_SPECIAL,
			  DATA_COLUMN, file,
			  IS_FOLDER_COLUMN, TRUE,
			  CANCELLABLE_COLUMN, cancellable,
			  -1);

      button->priv->n_special++;
    }

  desktopdir = g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP);

  /* "To disable a directory, point it to the homedir."
   * See http://freedesktop.org/wiki/Software/xdg-user-dirs
   **/
  if (g_strcmp0 (desktopdir, g_get_home_dir ()) != 0)
    {
      BtkTreePath *tree_path;
      GCancellable *cancellable;
      struct ChangeIconThemeData *info;

      file = g_file_new_for_path (desktopdir);
      btk_list_store_insert (store, &iter, pos);
      pos++;

      info = g_new0 (struct ChangeIconThemeData, 1);
      info->button = g_object_ref (button);
      tree_path = btk_tree_model_get_path (BTK_TREE_MODEL (store), &iter);
      info->row_ref = btk_tree_row_reference_new (BTK_TREE_MODEL (store),
						  tree_path);
      btk_tree_path_free (tree_path);

      cancellable = _btk_file_system_get_info (button->priv->fs, file,
					       "standard::icon,standard::display-name",
					       model_add_special_get_info_cb, info);

      btk_list_store_set (store, &iter,
			  TYPE_COLUMN, ROW_TYPE_SPECIAL,
			  ICON_COLUMN, NULL,
			  DISPLAY_NAME_COLUMN, _(DESKTOP_DISPLAY_NAME),
			  DATA_COLUMN, file,
			  IS_FOLDER_COLUMN, TRUE,
			  CANCELLABLE_COLUMN, cancellable,
			  -1);

      button->priv->n_special++;
    }
}

static void
model_add_volumes (BtkFileChooserButton *button,
		   GSList               *volumes)
{
  BtkListStore *store;
  gint pos;
  gboolean local_only;
  GSList *l;
  
  if (!volumes)
    return;

  store = BTK_LIST_STORE (button->priv->model);
  pos = model_get_type_position (button, ROW_TYPE_VOLUME);
  local_only = btk_file_chooser_get_local_only (BTK_FILE_CHOOSER (button->priv->dialog));

  for (l = volumes; l; l = l->next)
    {
      BtkFileSystemVolume *volume;
      BtkTreeIter iter;
      BdkPixbuf *pixbuf;
      gchar *display_name;

      volume = l->data;

      if (local_only)
        {
          if (_btk_file_system_volume_is_mounted (volume))
            {
              GFile *base_file;

              base_file = _btk_file_system_volume_get_root (volume);
              if (base_file != NULL)
                {
                  if (!_btk_file_has_native_path (base_file))
                    {
                      g_object_unref (base_file);
                      continue;
                    }
                  else
                    g_object_unref (base_file);
                }
            }
        }

      pixbuf = _btk_file_system_volume_render_icon (volume,
                                                    BTK_WIDGET (button),
                                                    button->priv->icon_size,
                                                    NULL);
      display_name = _btk_file_system_volume_get_display_name (volume);

      btk_list_store_insert (store, &iter, pos);
      btk_list_store_set (store, &iter,
                          ICON_COLUMN, pixbuf,
                          DISPLAY_NAME_COLUMN, display_name,
                          TYPE_COLUMN, ROW_TYPE_VOLUME,
                          DATA_COLUMN, _btk_file_system_volume_ref (volume),
                          IS_FOLDER_COLUMN, TRUE,
                          -1);

      if (pixbuf)
        g_object_unref (pixbuf);
      g_free (display_name);

      button->priv->n_volumes++;
      pos++;
    }
}

extern gchar * _btk_file_chooser_label_for_file (GFile *file);

static void
model_add_bookmarks (BtkFileChooserButton *button,
		     GSList               *bookmarks)
{
  BtkListStore *store;
  BtkTreeIter iter;
  gint pos;
  gboolean local_only;
  GSList *l;

  if (!bookmarks)
    return;

  store = BTK_LIST_STORE (button->priv->model);
  pos = model_get_type_position (button, ROW_TYPE_BOOKMARK);
  local_only = btk_file_chooser_get_local_only (BTK_FILE_CHOOSER (button->priv->dialog));

  for (l = bookmarks; l; l = l->next)
    {
      GFile *file;

      file = l->data;

      if (_btk_file_has_native_path (file))
	{
	  btk_list_store_insert (store, &iter, pos);
	  btk_list_store_set (store, &iter,
			      ICON_COLUMN, NULL,
			      DISPLAY_NAME_COLUMN, _(FALLBACK_DISPLAY_NAME),
			      TYPE_COLUMN, ROW_TYPE_BOOKMARK,
			      DATA_COLUMN, g_object_ref (file),
			      IS_FOLDER_COLUMN, FALSE,
			      -1);
	  set_info_for_file_at_iter (button, file, &iter);
	}
      else
	{
	  gchar *label;
	  BtkIconTheme *icon_theme;
	  BdkPixbuf *pixbuf;

	  if (local_only)
	    continue;

	  /* Don't call get_info for remote paths to avoid latency and
	   * auth dialogs.
	   * If we switch to a better bookmarks file format (XBEL), we
	   * should use mime info to get a better icon.
	   */
	  label = _btk_file_system_get_bookmark_label (button->priv->fs, file);
	  if (!label)
	    label = _btk_file_chooser_label_for_file (file);

	  icon_theme = btk_icon_theme_get_for_screen (btk_widget_get_screen (BTK_WIDGET (button)));
	  pixbuf = btk_icon_theme_load_icon (icon_theme, "folder-remote",
					     button->priv->icon_size, 0, NULL);

	  btk_list_store_insert (store, &iter, pos);
	  btk_list_store_set (store, &iter,
			      ICON_COLUMN, pixbuf,
			      DISPLAY_NAME_COLUMN, label,
			      TYPE_COLUMN, ROW_TYPE_BOOKMARK,
			      DATA_COLUMN, g_object_ref (file),
			      IS_FOLDER_COLUMN, TRUE,
			      -1);

	  g_free (label);
	  g_object_unref (pixbuf);
	}

      button->priv->n_bookmarks++;
      pos++;
    }

  if (button->priv->n_bookmarks > 0 &&
      !button->priv->has_bookmark_separator)
    {
      pos = model_get_type_position (button, ROW_TYPE_BOOKMARK_SEPARATOR);

      btk_list_store_insert (store, &iter, pos);
      btk_list_store_set (store, &iter,
			  ICON_COLUMN, NULL,
			  DISPLAY_NAME_COLUMN, NULL,
			  TYPE_COLUMN, ROW_TYPE_BOOKMARK_SEPARATOR,
			  DATA_COLUMN, NULL,
			  IS_FOLDER_COLUMN, FALSE,
			  -1);
      button->priv->has_bookmark_separator = TRUE;
    }
}

static void
model_update_current_folder (BtkFileChooserButton *button,
			     GFile                *file)
{
  BtkListStore *store;
  BtkTreeIter iter;
  gint pos;

  if (!file)
    return;

  store = BTK_LIST_STORE (button->priv->model);

  if (!button->priv->has_current_folder_separator)
    {
      pos = model_get_type_position (button, ROW_TYPE_CURRENT_FOLDER_SEPARATOR);
      btk_list_store_insert (store, &iter, pos);
      btk_list_store_set (store, &iter,
			  ICON_COLUMN, NULL,
			  DISPLAY_NAME_COLUMN, NULL,
			  TYPE_COLUMN, ROW_TYPE_CURRENT_FOLDER_SEPARATOR,
			  DATA_COLUMN, NULL,
			  IS_FOLDER_COLUMN, FALSE,
			  -1);
      button->priv->has_current_folder_separator = TRUE;
    }

  pos = model_get_type_position (button, ROW_TYPE_CURRENT_FOLDER);
  if (!button->priv->has_current_folder)
    {
      btk_list_store_insert (store, &iter, pos);
      button->priv->has_current_folder = TRUE;
    }
  else
    {
      btk_tree_model_iter_nth_child (button->priv->model, &iter, NULL, pos);
      model_free_row_data (button, &iter);
    }

  if (g_file_is_native (file))
    {
      btk_list_store_set (store, &iter,
			  ICON_COLUMN, NULL,
			  DISPLAY_NAME_COLUMN, _(FALLBACK_DISPLAY_NAME),
			  TYPE_COLUMN, ROW_TYPE_CURRENT_FOLDER,
			  DATA_COLUMN, g_object_ref (file),
			  IS_FOLDER_COLUMN, FALSE,
			  -1);
      set_info_for_file_at_iter (button, file, &iter);
    }
  else
    {
      gchar *label;
      BtkIconTheme *icon_theme;
      BdkPixbuf *pixbuf;

      /* Don't call get_info for remote paths to avoid latency and
       * auth dialogs.
       * If we switch to a better bookmarks file format (XBEL), we
       * should use mime info to get a better icon.
       */
      label = _btk_file_system_get_bookmark_label (button->priv->fs, file);
      if (!label)
	label = _btk_file_chooser_label_for_file (file);

      icon_theme = btk_icon_theme_get_for_screen (btk_widget_get_screen (BTK_WIDGET (button)));

      if (g_file_is_native (file))
	  pixbuf = btk_icon_theme_load_icon (icon_theme, "folder",
					     button->priv->icon_size, 0, NULL);
      else
	  pixbuf = btk_icon_theme_load_icon (icon_theme, "folder-remote",
					     button->priv->icon_size, 0, NULL);

      btk_list_store_set (store, &iter,
			  ICON_COLUMN, pixbuf,
			  DISPLAY_NAME_COLUMN, label,
			  TYPE_COLUMN, ROW_TYPE_CURRENT_FOLDER,
			  DATA_COLUMN, g_object_ref (file),
			  IS_FOLDER_COLUMN, TRUE,
			  -1);

      g_free (label);
      g_object_unref (pixbuf);
    }
}

static void
model_add_other (BtkFileChooserButton *button)
{
  BtkListStore *store;
  BtkTreeIter iter;
  gint pos;

  store = BTK_LIST_STORE (button->priv->model);
  pos = model_get_type_position (button, ROW_TYPE_OTHER_SEPARATOR);

  btk_list_store_insert (store, &iter, pos);
  btk_list_store_set (store, &iter,
		      ICON_COLUMN, NULL,
		      DISPLAY_NAME_COLUMN, NULL,
		      TYPE_COLUMN, ROW_TYPE_OTHER_SEPARATOR,
		      DATA_COLUMN, NULL,
		      IS_FOLDER_COLUMN, FALSE,
		      -1);
  button->priv->has_other_separator = TRUE;
  pos++;

  btk_list_store_insert (store, &iter, pos);
  btk_list_store_set (store, &iter,
		      ICON_COLUMN, NULL,
		      DISPLAY_NAME_COLUMN, _("Other..."),
		      TYPE_COLUMN, ROW_TYPE_OTHER,
		      DATA_COLUMN, NULL,
		      IS_FOLDER_COLUMN, FALSE,
		      -1);
}

static void
model_add_empty_selection (BtkFileChooserButton *button)
{
  BtkListStore *store;
  BtkTreeIter iter;
  gint pos;

  store = BTK_LIST_STORE (button->priv->model);
  pos = model_get_type_position (button, ROW_TYPE_EMPTY_SELECTION);

  btk_list_store_insert (store, &iter, pos);
  btk_list_store_set (store, &iter,
		      ICON_COLUMN, NULL,
		      DISPLAY_NAME_COLUMN, _(FALLBACK_DISPLAY_NAME),
		      TYPE_COLUMN, ROW_TYPE_EMPTY_SELECTION,
		      DATA_COLUMN, NULL,
		      IS_FOLDER_COLUMN, FALSE,
		      -1);
}

static void
model_remove_rows (BtkFileChooserButton *button,
		   gint                  pos,
		   gint                  n_rows)
{
  BtkListStore *store;

  if (!n_rows)
    return;

  store = BTK_LIST_STORE (button->priv->model);

  do
    {
      BtkTreeIter iter;

      if (!btk_tree_model_iter_nth_child (button->priv->model, &iter, NULL, pos))
	g_assert_not_reached ();

      model_free_row_data (button, &iter);
      btk_list_store_remove (store, &iter);
      n_rows--;
    }
  while (n_rows);
}

/* Filter Model */
static gboolean
test_if_file_is_visible (BtkFileSystem *fs,
			 GFile         *file,
			 gboolean       local_only,
			 gboolean       is_folder)
{
  if (!file)
    return FALSE;

  if (local_only && !_btk_file_has_native_path (file))
    return FALSE;

  if (!is_folder)
    return FALSE;

  return TRUE;
}

static gboolean
filter_model_visible_func (BtkTreeModel *model,
			   BtkTreeIter  *iter,
			   gpointer      user_data)
{
  BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (user_data);
  BtkFileChooserButtonPrivate *priv = button->priv;
  gchar type;
  gpointer data;
  gboolean local_only, retval, is_folder;

  type = ROW_TYPE_INVALID;
  data = NULL;
  local_only = btk_file_chooser_get_local_only (BTK_FILE_CHOOSER (priv->dialog));

  btk_tree_model_get (model, iter,
		      TYPE_COLUMN, &type,
		      DATA_COLUMN, &data,
		      IS_FOLDER_COLUMN, &is_folder,
		      -1);

  switch (type)
    {
    case ROW_TYPE_CURRENT_FOLDER:
      retval = TRUE;
      break;
    case ROW_TYPE_SPECIAL:
    case ROW_TYPE_SHORTCUT:
    case ROW_TYPE_BOOKMARK:
      retval = test_if_file_is_visible (priv->fs, data, local_only, is_folder);
      break;
    case ROW_TYPE_VOLUME:
      {
	retval = TRUE;
	if (local_only)
	  {
	    if (_btk_file_system_volume_is_mounted (data))
	      {
		GFile *base_file;

		base_file = _btk_file_system_volume_get_root (data);

		if (base_file)
		  {
		    if (!_btk_file_has_native_path (base_file))
		      retval = FALSE;
                    g_object_unref (base_file);
		  }
		else
		  retval = FALSE;
	      }
	  }
      }
      break;
    case ROW_TYPE_EMPTY_SELECTION:
      {
	gboolean popup_shown;

	g_object_get (priv->combo_box,
		      "popup-shown", &popup_shown,
		      NULL);

	if (popup_shown)
	  retval = FALSE;
	else
	  {
	    GFile *selected;

	    /* When the combo box is not popped up... */

	    selected = get_selected_file (button);
	    if (selected)
	      retval = FALSE; /* ... nonempty selection means the ROW_TYPE_EMPTY_SELECTION is *not* visible... */
	    else
	      retval = TRUE;  /* ... and empty selection means the ROW_TYPE_EMPTY_SELECTION *is* visible */

	    if (selected)
	      g_object_unref (selected);
	  }

	break;
      }
    default:
      retval = TRUE;
      break;
    }

  return retval;
}

/* Combo Box */
static void
name_cell_data_func (BtkCellLayout   *layout,
		     BtkCellRenderer *cell,
		     BtkTreeModel    *model,
		     BtkTreeIter     *iter,
		     gpointer         user_data)
{
  gchar type;

  type = 0;
  btk_tree_model_get (model, iter,
		      TYPE_COLUMN, &type,
		      -1);

  if (type == ROW_TYPE_CURRENT_FOLDER)
    g_object_set (cell, "ellipsize", BANGO_ELLIPSIZE_END, NULL);
  else
    g_object_set (cell, "ellipsize", BANGO_ELLIPSIZE_NONE, NULL);
}

static gboolean
combo_box_row_separator_func (BtkTreeModel *model,
			      BtkTreeIter  *iter,
			      gpointer      user_data)
{
  gchar type = ROW_TYPE_INVALID;

  btk_tree_model_get (model, iter, TYPE_COLUMN, &type, -1);

  return (type == ROW_TYPE_BOOKMARK_SEPARATOR ||
	  type == ROW_TYPE_CURRENT_FOLDER_SEPARATOR ||
	  type == ROW_TYPE_OTHER_SEPARATOR);
}

static void
select_combo_box_row_no_notify (BtkFileChooserButton *button, int pos)
{
  BtkFileChooserButtonPrivate *priv = button->priv;
  BtkTreeIter iter, filter_iter;

  btk_tree_model_iter_nth_child (priv->model, &iter, NULL, pos);
  btk_tree_model_filter_convert_child_iter_to_iter (BTK_TREE_MODEL_FILTER (priv->filter_model),
						    &filter_iter, &iter);

  g_signal_handler_block (priv->combo_box, priv->combo_box_changed_id);
  btk_combo_box_set_active_iter (BTK_COMBO_BOX (priv->combo_box), &filter_iter);
  g_signal_handler_unblock (priv->combo_box, priv->combo_box_changed_id);
}

static void
update_combo_box (BtkFileChooserButton *button)
{
  BtkFileChooserButtonPrivate *priv = button->priv;
  GFile *file;
  BtkTreeIter iter;
  gboolean row_found;

  file = get_selected_file (button);

  row_found = FALSE;

  btk_tree_model_get_iter_first (priv->filter_model, &iter);

  do
    {
      gchar type;
      gpointer data;

      type = ROW_TYPE_INVALID;
      data = NULL;

      btk_tree_model_get (priv->filter_model, &iter,
			  TYPE_COLUMN, &type,
			  DATA_COLUMN, &data,
			  -1);

      switch (type)
	{
	case ROW_TYPE_SPECIAL:
	case ROW_TYPE_SHORTCUT:
	case ROW_TYPE_BOOKMARK:
	case ROW_TYPE_CURRENT_FOLDER:
	  row_found = (file && g_file_equal (data, file));
	  break;
	case ROW_TYPE_VOLUME:
	  {
	    GFile *base_file;

	    base_file = _btk_file_system_volume_get_root (data);
            if (base_file)
              {
	        row_found = (file && g_file_equal (base_file, file));
		g_object_unref (base_file);
              }
	  }
	  break;
	default:
	  row_found = FALSE;
	  break;
	}

      if (row_found)
	{
	  g_signal_handler_block (priv->combo_box, priv->combo_box_changed_id);
	  btk_combo_box_set_active_iter (BTK_COMBO_BOX (priv->combo_box),
					 &iter);
	  g_signal_handler_unblock (priv->combo_box,
				    priv->combo_box_changed_id);
	}
    }
  while (!row_found && btk_tree_model_iter_next (priv->filter_model, &iter));

  if (!row_found)
    {
      gint pos;

      /* If it hasn't been found already, update & select the current-folder row. */
      if (file)
	{
	  model_update_current_folder (button, file);
	  pos = model_get_type_position (button, ROW_TYPE_CURRENT_FOLDER);
	}
      else
	{
	  /* No selection; switch to that row */

	  pos = model_get_type_position (button, ROW_TYPE_EMPTY_SELECTION);
	}

      btk_tree_model_filter_refilter (BTK_TREE_MODEL_FILTER (priv->filter_model));

      select_combo_box_row_no_notify (button, pos);
    }

  if (file)
    g_object_unref (file);
}

/* Button */
static void
update_label_get_info_cb (GCancellable *cancellable,
			  GFileInfo    *info,
			  const GError *error,
			  gpointer      data)
{
  gboolean cancelled = g_cancellable_is_cancelled (cancellable);
  BdkPixbuf *pixbuf;
  BtkFileChooserButton *button = data;
  BtkFileChooserButtonPrivate *priv = button->priv;

  if (cancellable != priv->update_button_cancellable)
    goto out;

  priv->update_button_cancellable = NULL;

  if (cancelled || error)
    goto out;

  btk_label_set_text (BTK_LABEL (priv->label), g_file_info_get_display_name (info));

  pixbuf = _btk_file_info_render_icon (info, BTK_WIDGET (priv->image), priv->icon_size);

  if (!pixbuf)
    pixbuf = btk_icon_theme_load_icon (get_icon_theme (BTK_WIDGET (priv->image)),
				       FALLBACK_ICON_NAME,
				       priv->icon_size, 0, NULL);

  btk_image_set_from_pixbuf (BTK_IMAGE (priv->image), pixbuf);
  if (pixbuf)
    g_object_unref (pixbuf);

out:
  emit_selection_changed_if_changing_selection (button);

  g_object_unref (button);
  g_object_unref (cancellable);
}

static void
update_label_and_image (BtkFileChooserButton *button)
{
  BtkFileChooserButtonPrivate *priv = button->priv;
  gchar *label_text;
  GFile *file;
  gboolean done_changing_selection;

  file = get_selected_file (button);

  label_text = NULL;
  done_changing_selection = FALSE;

  if (priv->update_button_cancellable)
    {
      g_cancellable_cancel (priv->update_button_cancellable);
      priv->update_button_cancellable = NULL;
    }

  if (file)
    {
      BtkFileSystemVolume *volume = NULL;

      volume = _btk_file_system_get_volume_for_file (priv->fs, file);
      if (volume)
        {
          GFile *base_file;

          base_file = _btk_file_system_volume_get_root (volume);
          if (base_file && g_file_equal (base_file, file))
            {
              BdkPixbuf *pixbuf;

              label_text = _btk_file_system_volume_get_display_name (volume);
              pixbuf = _btk_file_system_volume_render_icon (volume,
                                                            BTK_WIDGET (button),
                                                            priv->icon_size,
                                                            NULL);
              btk_image_set_from_pixbuf (BTK_IMAGE (priv->image), pixbuf);
              if (pixbuf)
                g_object_unref (pixbuf);
            }

          if (base_file)
            g_object_unref (base_file);

          _btk_file_system_volume_unref (volume);

          if (label_text)
	    {
	      done_changing_selection = TRUE;
	      goto out;
	    }
        }

      if (g_file_is_native (file))
        {
          priv->update_button_cancellable =
            _btk_file_system_get_info (priv->fs, file,
                                       "standard::icon,standard::display-name",
                                       update_label_get_info_cb,
                                       g_object_ref (button));
        }
      else
        {
          BdkPixbuf *pixbuf;

          label_text = _btk_file_system_get_bookmark_label (button->priv->fs, file);
          pixbuf = btk_icon_theme_load_icon (get_icon_theme (BTK_WIDGET (priv->image)),
                                             "text-x-generic",
                                             priv->icon_size, 0, NULL);
          btk_image_set_from_pixbuf (BTK_IMAGE (priv->image), pixbuf);
          if (pixbuf)
            g_object_unref (pixbuf);

	  done_changing_selection = TRUE;
        }
    }
  else
    {
      /* We know the selection is empty */
      done_changing_selection = TRUE;
    }

out:

  if (file)
    g_object_unref (file);

  if (label_text)
    {
      btk_label_set_text (BTK_LABEL (priv->label), label_text);
      g_free (label_text);
    }
  else
    {
      btk_label_set_text (BTK_LABEL (priv->label), _(FALLBACK_DISPLAY_NAME));
      btk_image_set_from_pixbuf (BTK_IMAGE (priv->image), NULL);
    }

  if (done_changing_selection)
    emit_selection_changed_if_changing_selection (button);
}


/* ************************ *
 *  Child Object Callbacks  *
 * ************************ */

/* File System */
static void
fs_volumes_changed_cb (BtkFileSystem *fs,
		       gpointer       user_data)
{
  BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (user_data);
  BtkFileChooserButtonPrivate *priv = button->priv;
  GSList *volumes;

  model_remove_rows (user_data,
		     model_get_type_position (user_data, ROW_TYPE_VOLUME),
		     priv->n_volumes);

  priv->n_volumes = 0;

  volumes = _btk_file_system_list_volumes (fs);
  model_add_volumes (user_data, volumes);
  g_slist_free (volumes);

  btk_tree_model_filter_refilter (BTK_TREE_MODEL_FILTER (priv->filter_model));

  update_label_and_image (user_data);
  update_combo_box (user_data);
}

static void
fs_bookmarks_changed_cb (BtkFileSystem *fs,
			 gpointer       user_data)
{
  BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (user_data);
  BtkFileChooserButtonPrivate *priv = button->priv;
  GSList *bookmarks;

  bookmarks = _btk_file_system_list_bookmarks (fs);
  model_remove_rows (user_data,
		     model_get_type_position (user_data,
					      ROW_TYPE_BOOKMARK_SEPARATOR),
		     (priv->n_bookmarks + priv->has_bookmark_separator));
  priv->has_bookmark_separator = FALSE;
  priv->n_bookmarks = 0;
  model_add_bookmarks (user_data, bookmarks);
  g_slist_foreach (bookmarks, (GFunc) g_object_unref, NULL);
  g_slist_free (bookmarks);

  btk_tree_model_filter_refilter (BTK_TREE_MODEL_FILTER (priv->filter_model));

  update_label_and_image (user_data);
  update_combo_box (user_data);
}

static void
save_inactive_state (BtkFileChooserButton *button)
{
  BtkFileChooserButtonPrivate *priv = button->priv;

  if (priv->current_folder_while_inactive)
    g_object_unref (priv->current_folder_while_inactive);

  if (priv->selection_while_inactive)
    g_object_unref (priv->selection_while_inactive);

  priv->current_folder_while_inactive = btk_file_chooser_get_current_folder_file (BTK_FILE_CHOOSER (priv->dialog));
  priv->selection_while_inactive = btk_file_chooser_get_file (BTK_FILE_CHOOSER (priv->dialog));
}

static void
restore_inactive_state (BtkFileChooserButton *button)
{
  BtkFileChooserButtonPrivate *priv = button->priv;

  if (priv->current_folder_while_inactive)
    btk_file_chooser_set_current_folder_file (BTK_FILE_CHOOSER (priv->dialog), priv->current_folder_while_inactive, NULL);

  if (priv->selection_while_inactive)
    btk_file_chooser_select_file (BTK_FILE_CHOOSER (priv->dialog), priv->selection_while_inactive, NULL);
  else
    btk_file_chooser_unselect_all (BTK_FILE_CHOOSER (priv->dialog));
}

/* Dialog */
static void
open_dialog (BtkFileChooserButton *button)
{
  BtkFileChooserButtonPrivate *priv = button->priv;

  /* Setup the dialog parent to be chooser button's toplevel, and be modal
     as needed. */
  if (!btk_widget_get_visible (priv->dialog))
    {
      BtkWidget *toplevel;

      toplevel = btk_widget_get_toplevel (BTK_WIDGET (button));

      if (btk_widget_is_toplevel (toplevel) && BTK_IS_WINDOW (toplevel))
        {
          if (BTK_WINDOW (toplevel) != btk_window_get_transient_for (BTK_WINDOW (priv->dialog)))
 	    btk_window_set_transient_for (BTK_WINDOW (priv->dialog),
					  BTK_WINDOW (toplevel));

	  btk_window_set_modal (BTK_WINDOW (priv->dialog),
				btk_window_get_modal (BTK_WINDOW (toplevel)));
	}
    }

  if (!priv->active)
    {
      restore_inactive_state (button);
      priv->active = TRUE;
    }

  btk_widget_set_sensitive (priv->combo_box, FALSE);
  btk_window_present (BTK_WINDOW (priv->dialog));
}

/* Combo Box */
static void
combo_box_changed_cb (BtkComboBox *combo_box,
		      gpointer     user_data)
{
  BtkTreeIter iter;

  if (btk_combo_box_get_active_iter (combo_box, &iter))
    {
      BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (user_data);
      BtkFileChooserButtonPrivate *priv = button->priv;
      gchar type;
      gpointer data;

      type = ROW_TYPE_INVALID;
      data = NULL;

      btk_tree_model_get (priv->filter_model, &iter,
			  TYPE_COLUMN, &type,
			  DATA_COLUMN, &data,
			  -1);

      switch (type)
	{
	case ROW_TYPE_SPECIAL:
	case ROW_TYPE_SHORTCUT:
	case ROW_TYPE_BOOKMARK:
	case ROW_TYPE_CURRENT_FOLDER:
	  if (data)
	    btk_file_chooser_button_select_file (BTK_FILE_CHOOSER (button), data, NULL);
	  break;
	case ROW_TYPE_VOLUME:
	  {
	    GFile *base_file;

	    base_file = _btk_file_system_volume_get_root (data);
	    if (base_file)
	      {
		btk_file_chooser_button_select_file (BTK_FILE_CHOOSER (button), base_file, NULL);
		g_object_unref (base_file);
	      }
	  }
	  break;
	case ROW_TYPE_OTHER:
	  open_dialog (user_data);
	  break;
	default:
	  break;
	}
    }
}

/* Calback for the "notify::popup-shown" signal on the combo box.
 * When the combo is popped up, we don't want the ROW_TYPE_EMPTY_SELECTION to be visible
 * at all; otherwise we would be showing a "(None)" item in the combo box's popup.
 *
 * However, when the combo box is *not* popped up, we want the empty-selection row
 * to be visible depending on the selection.
 *
 * Since all that is done through the filter_model_visible_func(), this means
 * that we need to refilter the model when the combo box pops up - hence the
 * present signal handler.
 */
static void
combo_box_notify_popup_shown_cb (GObject    *object,
				 GParamSpec *pspec,
				 gpointer    user_data)
{
  BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (user_data);
  BtkFileChooserButtonPrivate *priv = button->priv;
  gboolean popup_shown;

  g_object_get (priv->combo_box,
		"popup-shown", &popup_shown,
		NULL);

  /* Indicate that the ROW_TYPE_EMPTY_SELECTION will change visibility... */
  btk_tree_model_filter_refilter (BTK_TREE_MODEL_FILTER (priv->filter_model));

  /* If the combo box popup got dismissed, go back to showing the ROW_TYPE_EMPTY_SELECTION if needed */
  if (!popup_shown)
    {
      GFile *selected = get_selected_file (button);

      if (!selected)
	{
	  int pos;

	  pos = model_get_type_position (button, ROW_TYPE_EMPTY_SELECTION);
	  select_combo_box_row_no_notify (button, pos);
	}
      else
	g_object_unref (selected);
    }
}

/* Button */
static void
button_clicked_cb (BtkButton *real_button,
		   gpointer   user_data)
{
  open_dialog (user_data);
}

/* Dialog */

static void
dialog_update_preview_cb (BtkFileChooser *dialog,
			  gpointer        user_data)
{
  g_signal_emit_by_name (user_data, "update-preview");
}

static void
dialog_notify_cb (GObject    *dialog,
		  GParamSpec *pspec,
		  gpointer    user_data)
{
  gpointer iface;

  iface = g_type_interface_peek (g_type_class_peek (G_OBJECT_TYPE (dialog)),
				 BTK_TYPE_FILE_CHOOSER);
  if (g_object_interface_find_property (iface, pspec->name))
    g_object_notify (user_data, pspec->name);

  if (g_ascii_strcasecmp (pspec->name, "local-only") == 0)
    {
      BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (user_data);
      BtkFileChooserButtonPrivate *priv = button->priv;

      if (priv->has_current_folder)
	{
	  BtkTreeIter iter;
	  gint pos;
	  gpointer data;

	  pos = model_get_type_position (user_data,
					 ROW_TYPE_CURRENT_FOLDER);
	  btk_tree_model_iter_nth_child (priv->model, &iter, NULL, pos);

	  data = NULL;
	  btk_tree_model_get (priv->model, &iter, DATA_COLUMN, &data, -1);

	  /* If the path isn't local but we're in local-only mode now, remove
	   * the custom-folder row */
	  if (data && !_btk_file_has_native_path (G_FILE (data)) &&
	      btk_file_chooser_get_local_only (BTK_FILE_CHOOSER (priv->dialog)))
	    {
	      pos--;
	      model_remove_rows (user_data, pos, 2);
	    }
	}

      btk_tree_model_filter_refilter (BTK_TREE_MODEL_FILTER (priv->filter_model));
      update_combo_box (user_data);
    }
}

static gboolean
dialog_delete_event_cb (BtkWidget *dialog,
			BdkEvent  *event,
		        gpointer   user_data)
{
  g_signal_emit_by_name (dialog, "response", BTK_RESPONSE_DELETE_EVENT);

  return TRUE;
}

static void
dialog_response_cb (BtkDialog *dialog,
		    gint       response,
		    gpointer   user_data)
{
  BtkFileChooserButton *button = BTK_FILE_CHOOSER_BUTTON (user_data);
  BtkFileChooserButtonPrivate *priv = button->priv;

  if (response == BTK_RESPONSE_ACCEPT ||
      response == BTK_RESPONSE_OK)
    {
      save_inactive_state (button);

      g_signal_emit_by_name (button, "current-folder-changed");
      g_signal_emit_by_name (button, "selection-changed");
    }
  else
    {
      restore_inactive_state (button);
    }

  if (priv->active)
    priv->active = FALSE;

  update_label_and_image (button);
  update_combo_box (button);

  btk_widget_set_sensitive (priv->combo_box, TRUE);
  btk_widget_hide (priv->dialog);

  if (response == BTK_RESPONSE_ACCEPT ||
      response == BTK_RESPONSE_OK)
    g_signal_emit (button, file_chooser_button_signals[FILE_SET], 0);
}


/* ************************************************************************** *
 *  Public API                                                                *
 * ************************************************************************** */

/**
 * btk_file_chooser_button_new:
 * @title: the title of the browse dialog.
 * @action: the open mode for the widget.
 *
 * Creates a new file-selecting button widget.
 *
 * Returns: a new button widget.
 *
 * Since: 2.6
 **/
BtkWidget *
btk_file_chooser_button_new (const gchar          *title,
			     BtkFileChooserAction  action)
{
  g_return_val_if_fail (action == BTK_FILE_CHOOSER_ACTION_OPEN ||
			action == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, NULL);

  return g_object_new (BTK_TYPE_FILE_CHOOSER_BUTTON,
		       "action", action,
		       "title", (title ? title : _(DEFAULT_TITLE)),
		       NULL);
}

/**
 * btk_file_chooser_button_new_with_backend:
 * @title: the title of the browse dialog.
 * @action: the open mode for the widget.
 * @backend: the name of the #BtkFileSystem backend to use.
 * 
 * Creates a new file-selecting button widget using @backend.
 * 
 * Returns: a new button widget.
 * 
 * Since: 2.6
 * Deprecated: 2.14: Use btk_file_chooser_button_new() instead.
 **/
BtkWidget *
btk_file_chooser_button_new_with_backend (const gchar          *title,
					  BtkFileChooserAction  action,
					  const gchar          *backend)
{
  g_return_val_if_fail (action == BTK_FILE_CHOOSER_ACTION_OPEN ||
			action == BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, NULL);

  return g_object_new (BTK_TYPE_FILE_CHOOSER_BUTTON,
		       "action", action,
		       "title", (title ? title : _(DEFAULT_TITLE)),
		       NULL);
}

/**
 * btk_file_chooser_button_new_with_dialog:
 * @dialog: the widget to use as dialog
 *
 * Creates a #BtkFileChooserButton widget which uses @dialog as its
 * file-picking window.
 *
 * Note that @dialog must be a #BtkDialog (or subclass) which
 * implements the #BtkFileChooser interface and must not have
 * %BTK_DIALOG_DESTROY_WITH_PARENT set.
 *
 * Also note that the dialog needs to have its confirmative button
 * added with response %BTK_RESPONSE_ACCEPT or %BTK_RESPONSE_OK in
 * order for the button to take over the file selected in the dialog.
 *
 * Returns: a new button widget.
 *
 * Since: 2.6
 **/
BtkWidget *
btk_file_chooser_button_new_with_dialog (BtkWidget *dialog)
{
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (dialog) && BTK_IS_DIALOG (dialog), NULL);

  return g_object_new (BTK_TYPE_FILE_CHOOSER_BUTTON,
		       "dialog", dialog,
		       NULL);
}

/**
 * btk_file_chooser_button_set_title:
 * @button: the button widget to modify.
 * @title: the new browse dialog title.
 *
 * Modifies the @title of the browse dialog used by @button.
 *
 * Since: 2.6
 **/
void
btk_file_chooser_button_set_title (BtkFileChooserButton *button,
				   const gchar          *title)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER_BUTTON (button));

  btk_window_set_title (BTK_WINDOW (button->priv->dialog), title);
  g_object_notify (G_OBJECT (button), "title");
}

/**
 * btk_file_chooser_button_get_title:
 * @button: the button widget to examine.
 *
 * Retrieves the title of the browse dialog used by @button. The returned value
 * should not be modified or freed.
 *
 * Returns: a pointer to the browse dialog's title.
 *
 * Since: 2.6
 **/
const gchar *
btk_file_chooser_button_get_title (BtkFileChooserButton *button)
{
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER_BUTTON (button), NULL);

  return btk_window_get_title (BTK_WINDOW (button->priv->dialog));
}

/**
 * btk_file_chooser_button_get_width_chars:
 * @button: the button widget to examine.
 *
 * Retrieves the width in characters of the @button widget's entry and/or label.
 *
 * Returns: an integer width (in characters) that the button will use to size itself.
 *
 * Since: 2.6
 **/
gint
btk_file_chooser_button_get_width_chars (BtkFileChooserButton *button)
{
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER_BUTTON (button), -1);

  return btk_label_get_width_chars (BTK_LABEL (button->priv->label));
}

/**
 * btk_file_chooser_button_set_width_chars:
 * @button: the button widget to examine.
 * @n_chars: the new width, in characters.
 *
 * Sets the width (in characters) that @button will use to @n_chars.
 *
 * Since: 2.6
 **/
void
btk_file_chooser_button_set_width_chars (BtkFileChooserButton *button,
					 gint                  n_chars)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER_BUTTON (button));

  btk_label_set_width_chars (BTK_LABEL (button->priv->label), n_chars);
  g_object_notify (G_OBJECT (button), "width-chars");
}

/**
 * btk_file_chooser_button_set_focus_on_click:
 * @button: a #BtkFileChooserButton
 * @focus_on_click: whether the button grabs focus when clicked with the mouse
 *
 * Sets whether the button will grab focus when it is clicked with the mouse.
 * Making mouse clicks not grab focus is useful in places like toolbars where
 * you don't want the keyboard focus removed from the main area of the
 * application.
 *
 * Since: 2.10
 **/
void
btk_file_chooser_button_set_focus_on_click (BtkFileChooserButton *button,
					    gboolean              focus_on_click)
{
  BtkFileChooserButtonPrivate *priv;

  g_return_if_fail (BTK_IS_FILE_CHOOSER_BUTTON (button));

  priv = button->priv;

  focus_on_click = focus_on_click != FALSE;

  if (priv->focus_on_click != focus_on_click)
    {
      priv->focus_on_click = focus_on_click;
      btk_button_set_focus_on_click (BTK_BUTTON (priv->button), focus_on_click);
      btk_combo_box_set_focus_on_click (BTK_COMBO_BOX (priv->combo_box), focus_on_click);

      g_object_notify (G_OBJECT (button), "focus-on-click");
    }
}

/**
 * btk_file_chooser_button_get_focus_on_click:
 * @button: a #BtkFileChooserButton
 *
 * Returns whether the button grabs focus when it is clicked with the mouse.
 * See btk_file_chooser_button_set_focus_on_click().
 *
 * Return value: %TRUE if the button grabs focus when it is clicked with
 *               the mouse.
 *
 * Since: 2.10
 **/
gboolean
btk_file_chooser_button_get_focus_on_click (BtkFileChooserButton *button)
{
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER_BUTTON (button), FALSE);

  return button->priv->focus_on_click;
}

#define __BTK_FILE_CHOOSER_BUTTON_C__
#include "btkaliasdef.c"
