/* btkcombobox.c
 * Copyright (C) 2002, 2003  Kristian Rietveld <kris@btk.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include "btkcombobox.h"

#include "btkarrow.h"
#include "btkbindings.h"
#include "btkcelllayout.h"
#include "btkcellrenderertext.h"
#include "btkcellview.h"
#include "btkeventbox.h"
#include "btkframe.h"
#include "btkhbox.h"
#include "btkliststore.h"
#include "btkmain.h"
#include "btkmenu.h"
#include "btkscrolledwindow.h"
#include "btkseparatormenuitem.h"
#include "btktearoffmenuitem.h"
#include "btktogglebutton.h"
#include "btktreeselection.h"
#include "btkvseparator.h"
#include "btkwindow.h"
#include "btkprivate.h"

#include <bdk/bdkkeysyms.h>

#include <bobject/gvaluecollector.h>

#include <string.h>
#include <stdarg.h>

#include "btkmarshalers.h"
#include "btkintl.h"

#include "btktreeprivate.h"
#include "btkalias.h"

/**
 * SECTION:btkcombobox
 * @Short_description: A widget used to choose from a list of items
 * @Title: BtkComboBox
 * @See_also: #BtkComboBoxText, #BtkTreeModel, #BtkCellRenderer
 *
 * A BtkComboBox is a widget that allows the user to choose from a list of
 * valid choices. The BtkComboBox displays the selected choice. When
 * activated, the BtkComboBox displays a popup which allows the user to
 * make a new choice. The style in which the selected value is displayed,
 * and the style of the popup is determined by the current theme. It may
 * be similar to a Windows-style combo box.
 *
 * The BtkComboBox uses the model-view pattern; the list of valid choices
 * is specified in the form of a tree model, and the display of the choices
 * can be adapted to the data in the model by using cell renderers, as you
 * would in a tree view. This is possible since BtkComboBox implements the
 * #BtkCellLayout interface. The tree model holding the valid choices is
 * not restricted to a flat list, it can be a real tree, and the popup will
 * reflect the tree structure.
 *
 * To allow the user to enter values not in the model, the 'has-entry'
 * property allows the BtkComboBox to contain a #BtkEntry. This entry
 * can be accessed by calling btk_bin_get_child() on the combo box.
 *
 * For a simple list of textual choices, the model-view API of BtkComboBox
 * can be a bit overwhelming. In this case, #BtkComboBoxText offers a
 * simple alternative. Both BtkComboBox and #BtkComboBoxText can contain
 * an entry.
 */

/* WELCOME, to THE house of evil code */

typedef struct _ComboCellInfo ComboCellInfo;
struct _ComboCellInfo
{
  BtkCellRenderer *cell;
  GSList *attributes;

  BtkCellLayoutDataFunc func;
  gpointer func_data;
  GDestroyNotify destroy;

  guint expand : 1;
  guint pack : 1;
};

#define BTK_COMBO_BOX_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_COMBO_BOX, BtkComboBoxPrivate))

struct _BtkComboBoxPrivate
{
  BtkTreeModel *model;

  gint col_column;
  gint row_column;

  gint wrap_width;
  BtkShadowType shadow_type;

  gint active; /* Only temporary */
  BtkTreeRowReference *active_row;

  BtkWidget *tree_view;
  BtkTreeViewColumn *column;

  BtkWidget *cell_view;
  BtkWidget *cell_view_frame;

  BtkWidget *button;
  BtkWidget *box;
  BtkWidget *arrow;
  BtkWidget *separator;

  BtkWidget *popup_widget;
  BtkWidget *popup_window;
  BtkWidget *scrolled_window;

  guint inserted_id;
  guint deleted_id;
  guint reordered_id;
  guint changed_id;
  guint popup_idle_id;
  guint activate_button;
  guint32 activate_time;
  guint scroll_timer;
  guint resize_idle_id;

  gint width;
  gint height;

  /* For "has-entry" specific behavior we track
   * an automated cell renderer and text column */
  gint  text_column;
  BtkCellRenderer *text_renderer;

  GSList *cells;

  guint popup_in_progress : 1;
  guint popup_shown : 1;
  guint add_tearoffs : 1;
  guint has_frame : 1;
  guint is_cell_renderer : 1;
  guint editing_canceled : 1;
  guint auto_scroll : 1;
  guint focus_on_click : 1;
  guint button_sensitivity : 2;
  guint has_entry : 1;

  BtkTreeViewRowSeparatorFunc row_separator_func;
  gpointer                    row_separator_data;
  GDestroyNotify              row_separator_destroy;

  gchar *tearoff_title;
};

/* While debugging this evil code, I have learned that
 * there are actually 4 modes to this widget, which can
 * be characterized as follows
 * 
 * 1) menu mode, no child added
 *
 * tree_view -> NULL
 * cell_view -> BtkCellView, regular child
 * cell_view_frame -> NULL
 * button -> BtkToggleButton set_parent to combo
 * arrow -> BtkArrow set_parent to button
 * separator -> BtkVSepator set_parent to button
 * popup_widget -> BtkMenu
 * popup_window -> NULL
 * scrolled_window -> NULL
 *
 * 2) menu mode, child added
 * 
 * tree_view -> NULL
 * cell_view -> NULL 
 * cell_view_frame -> NULL
 * button -> BtkToggleButton set_parent to combo
 * arrow -> BtkArrow, child of button
 * separator -> NULL
 * popup_widget -> BtkMenu
 * popup_window -> NULL
 * scrolled_window -> NULL
 *
 * 3) list mode, no child added
 * 
 * tree_view -> BtkTreeView, child of scrolled_window
 * cell_view -> BtkCellView, regular child
 * cell_view_frame -> BtkFrame, set parent to combo
 * button -> BtkToggleButton, set_parent to combo
 * arrow -> BtkArrow, child of button
 * separator -> NULL
 * popup_widget -> tree_view
 * popup_window -> BtkWindow
 * scrolled_window -> BtkScrolledWindow, child of popup_window
 *
 * 4) list mode, child added
 *
 * tree_view -> BtkTreeView, child of scrolled_window
 * cell_view -> NULL
 * cell_view_frame -> NULL
 * button -> BtkToggleButton, set_parent to combo
 * arrow -> BtkArrow, child of button
 * separator -> NULL
 * popup_widget -> tree_view
 * popup_window -> BtkWindow
 * scrolled_window -> BtkScrolledWindow, child of popup_window
 * 
 */

enum {
  CHANGED,
  MOVE_ACTIVE,
  POPUP,
  POPDOWN,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_MODEL,
  PROP_WRAP_WIDTH,
  PROP_ROW_SPAN_COLUMN,
  PROP_COLUMN_SPAN_COLUMN,
  PROP_ACTIVE,
  PROP_ADD_TEAROFFS,
  PROP_TEAROFF_TITLE,
  PROP_HAS_FRAME,
  PROP_FOCUS_ON_CLICK,
  PROP_POPUP_SHOWN,
  PROP_BUTTON_SENSITIVITY,
  PROP_EDITING_CANCELED,
  PROP_HAS_ENTRY,
  PROP_ENTRY_TEXT_COLUMN
};

static guint combo_box_signals[LAST_SIGNAL] = {0,};

#define BONUS_PADDING 4
#define SCROLL_TIME  100

/* common */

static void     btk_combo_box_cell_layout_init     (BtkCellLayoutIface *iface);
static void     btk_combo_box_cell_editable_init   (BtkCellEditableIface *iface);
static GObject *btk_combo_box_constructor          (GType                  type,
						    guint                  n_construct_properties,
						    GObjectConstructParam *construct_properties);
static void     btk_combo_box_dispose              (GObject          *object);
static void     btk_combo_box_finalize             (GObject          *object);
static void     btk_combo_box_destroy              (BtkObject        *object);

static void     btk_combo_box_set_property         (GObject         *object,
                                                    guint            prop_id,
                                                    const GValue    *value,
                                                    GParamSpec      *spec);
static void     btk_combo_box_get_property         (GObject         *object,
                                                    guint            prop_id,
                                                    GValue          *value,
                                                    GParamSpec      *spec);

static void     btk_combo_box_state_changed        (BtkWidget        *widget,
			                            BtkStateType      previous);
static void     btk_combo_box_grab_focus           (BtkWidget       *widget);
static void     btk_combo_box_style_set            (BtkWidget       *widget,
                                                    BtkStyle        *previous);
static void     btk_combo_box_button_toggled       (BtkWidget       *widget,
                                                    gpointer         data);
static void     btk_combo_box_button_state_changed (BtkWidget       *widget,
			                            BtkStateType     previous,
						    gpointer         data);
static void     btk_combo_box_add                  (BtkContainer    *container,
                                                    BtkWidget       *widget);
static void     btk_combo_box_remove               (BtkContainer    *container,
                                                    BtkWidget       *widget);

static ComboCellInfo *btk_combo_box_get_cell_info  (BtkComboBox      *combo_box,
                                                    BtkCellRenderer  *cell);

static void     btk_combo_box_menu_show            (BtkWidget        *menu,
                                                    gpointer          user_data);
static void     btk_combo_box_menu_hide            (BtkWidget        *menu,
                                                    gpointer          user_data);

static void     btk_combo_box_set_popup_widget     (BtkComboBox      *combo_box,
                                                    BtkWidget        *popup);
static void     btk_combo_box_menu_position_below  (BtkMenu          *menu,
                                                    gint             *x,
                                                    gint             *y,
                                                    gint             *push_in,
                                                    gpointer          user_data);
static void     btk_combo_box_menu_position_over   (BtkMenu          *menu,
                                                    gint             *x,
                                                    gint             *y,
                                                    gint             *push_in,
                                                    gpointer          user_data);
static void     btk_combo_box_menu_position        (BtkMenu          *menu,
                                                    gint             *x,
                                                    gint             *y,
                                                    gint             *push_in,
                                                    gpointer          user_data);

static gint     btk_combo_box_calc_requested_width (BtkComboBox      *combo_box,
                                                    BtkTreePath      *path);
static void     btk_combo_box_remeasure            (BtkComboBox      *combo_box);

static void     btk_combo_box_unset_model          (BtkComboBox      *combo_box);

static void     btk_combo_box_size_request         (BtkWidget        *widget,
                                                    BtkRequisition   *requisition);
static void     btk_combo_box_size_allocate        (BtkWidget        *widget,
                                                    BtkAllocation    *allocation);
static void     btk_combo_box_forall               (BtkContainer     *container,
                                                    gboolean          include_internals,
                                                    BtkCallback       callback,
                                                    gpointer          callback_data);
static gboolean btk_combo_box_expose_event         (BtkWidget        *widget,
                                                    BdkEventExpose   *event);
static gboolean btk_combo_box_scroll_event         (BtkWidget        *widget,
                                                    BdkEventScroll   *event);
static void     btk_combo_box_set_active_internal  (BtkComboBox      *combo_box,
						    BtkTreePath      *path);

static void     btk_combo_box_check_appearance     (BtkComboBox      *combo_box);
static gchar *  btk_combo_box_real_get_active_text (BtkComboBox      *combo_box);
static void     btk_combo_box_real_move_active     (BtkComboBox      *combo_box,
                                                    BtkScrollType     scroll);
static void     btk_combo_box_real_popup           (BtkComboBox      *combo_box);
static gboolean btk_combo_box_real_popdown         (BtkComboBox      *combo_box);

/* listening to the model */
static void     btk_combo_box_model_row_inserted   (BtkTreeModel     *model,
						    BtkTreePath      *path,
						    BtkTreeIter      *iter,
						    gpointer          user_data);
static void     btk_combo_box_model_row_deleted    (BtkTreeModel     *model,
						    BtkTreePath      *path,
						    gpointer          user_data);
static void     btk_combo_box_model_rows_reordered (BtkTreeModel     *model,
						    BtkTreePath      *path,
						    BtkTreeIter      *iter,
						    gint             *new_order,
						    gpointer          user_data);
static void     btk_combo_box_model_row_changed    (BtkTreeModel     *model,
						    BtkTreePath      *path,
						    BtkTreeIter      *iter,
						    gpointer          data);
static void     btk_combo_box_model_row_expanded   (BtkTreeModel     *model,
						    BtkTreePath      *path,
						    BtkTreeIter      *iter,
						    gpointer          data);

/* list */
static void     btk_combo_box_list_position        (BtkComboBox      *combo_box, 
						    gint             *x, 
						    gint             *y, 
						    gint             *width,
						    gint             *height);
static void     btk_combo_box_list_setup           (BtkComboBox      *combo_box);
static void     btk_combo_box_list_destroy         (BtkComboBox      *combo_box);

static gboolean btk_combo_box_list_button_released (BtkWidget        *widget,
                                                    BdkEventButton   *event,
                                                    gpointer          data);
static gboolean btk_combo_box_list_key_press       (BtkWidget        *widget,
                                                    BdkEventKey      *event,
                                                    gpointer          data);
static gboolean btk_combo_box_list_enter_notify    (BtkWidget        *widget,
                                                    BdkEventCrossing *event,
                                                    gpointer          data);
static void     btk_combo_box_list_auto_scroll     (BtkComboBox   *combo,
						    gint           x,
						    gint           y);
static gboolean btk_combo_box_list_scroll_timeout  (BtkComboBox   *combo);
static gboolean btk_combo_box_list_button_pressed  (BtkWidget        *widget,
                                                    BdkEventButton   *event,
                                                    gpointer          data);

static gboolean btk_combo_box_list_select_func     (BtkTreeSelection *selection,
						    BtkTreeModel     *model,
						    BtkTreePath      *path,
						    gboolean          path_currently_selected,
						    gpointer          data);

static void     btk_combo_box_list_row_changed     (BtkTreeModel     *model,
                                                    BtkTreePath      *path,
                                                    BtkTreeIter      *iter,
                                                    gpointer          data);
static void     btk_combo_box_list_popup_resize    (BtkComboBox      *combo_box);

/* menu */
static void     btk_combo_box_menu_setup           (BtkComboBox      *combo_box,
                                                    gboolean          add_children);
static void     btk_combo_box_menu_fill            (BtkComboBox      *combo_box);
static void     btk_combo_box_menu_fill_level      (BtkComboBox      *combo_box,
						    BtkWidget        *menu,
						    BtkTreeIter      *iter);
static void     btk_combo_box_update_title         (BtkComboBox      *combo_box);
static void     btk_combo_box_menu_destroy         (BtkComboBox      *combo_box);

static void     btk_combo_box_relayout_item        (BtkComboBox      *combo_box,
						    BtkWidget        *item,
                                                    BtkTreeIter      *iter,
						    BtkWidget        *last);
static void     btk_combo_box_relayout             (BtkComboBox      *combo_box);

static gboolean btk_combo_box_menu_button_press    (BtkWidget        *widget,
                                                    BdkEventButton   *event,
                                                    gpointer          user_data);
static void     btk_combo_box_menu_item_activate   (BtkWidget        *item,
                                                    gpointer          user_data);

static void     btk_combo_box_update_sensitivity   (BtkComboBox      *combo_box);
static void     btk_combo_box_menu_row_inserted    (BtkTreeModel     *model,
                                                    BtkTreePath      *path,
                                                    BtkTreeIter      *iter,
                                                    gpointer          user_data);
static void     btk_combo_box_menu_row_deleted     (BtkTreeModel     *model,
                                                    BtkTreePath      *path,
                                                    gpointer          user_data);
static void     btk_combo_box_menu_rows_reordered  (BtkTreeModel     *model,
						    BtkTreePath      *path,
						    BtkTreeIter      *iter,
						    gint             *new_order,
						    gpointer          user_data);
static void     btk_combo_box_menu_row_changed     (BtkTreeModel     *model,
                                                    BtkTreePath      *path,
                                                    BtkTreeIter      *iter,
                                                    gpointer          data);
static gboolean btk_combo_box_menu_key_press       (BtkWidget        *widget,
						    BdkEventKey      *event,
						    gpointer          data);
static void     btk_combo_box_menu_popup           (BtkComboBox      *combo_box,
						    guint             button, 
						    guint32           activate_time);
static BtkWidget *btk_cell_view_menu_item_new      (BtkComboBox      *combo_box,
						    BtkTreeModel     *model,
						    BtkTreeIter      *iter);

/* cell layout */
static void     btk_combo_box_cell_layout_pack_start         (BtkCellLayout         *layout,
                                                              BtkCellRenderer       *cell,
                                                              gboolean               expand);
static void     btk_combo_box_cell_layout_pack_end           (BtkCellLayout         *layout,
                                                              BtkCellRenderer       *cell,
                                                              gboolean               expand);
static GList   *btk_combo_box_cell_layout_get_cells          (BtkCellLayout         *layout);
static void     btk_combo_box_cell_layout_clear              (BtkCellLayout         *layout);
static void     btk_combo_box_cell_layout_add_attribute      (BtkCellLayout         *layout,
                                                              BtkCellRenderer       *cell,
                                                              const gchar           *attribute,
                                                              gint                   column);
static void     btk_combo_box_cell_layout_set_cell_data_func (BtkCellLayout         *layout,
                                                              BtkCellRenderer       *cell,
                                                              BtkCellLayoutDataFunc  func,
                                                              gpointer               func_data,
                                                              GDestroyNotify         destroy);
static void     btk_combo_box_cell_layout_clear_attributes   (BtkCellLayout         *layout,
                                                              BtkCellRenderer       *cell);
static void     btk_combo_box_cell_layout_reorder            (BtkCellLayout         *layout,
                                                              BtkCellRenderer       *cell,
                                                              gint                   position);
static gboolean btk_combo_box_mnemonic_activate              (BtkWidget    *widget,
							      gboolean      group_cycling);

static void     btk_combo_box_sync_cells                     (BtkComboBox   *combo_box,
					                      BtkCellLayout *cell_layout);
static void     combo_cell_data_func                         (BtkCellLayout   *cell_layout,
							      BtkCellRenderer *cell,
							      BtkTreeModel    *tree_model,
							      BtkTreeIter     *iter,
							      gpointer         data);
static void     btk_combo_box_child_show                     (BtkWidget       *widget,
							      BtkComboBox     *combo_box);
static void     btk_combo_box_child_hide                     (BtkWidget       *widget,
							      BtkComboBox     *combo_box);

/* BtkComboBox:has-entry callbacks */
static void     btk_combo_box_entry_contents_changed         (BtkEntry        *entry,
							      gpointer         user_data);
static void     btk_combo_box_entry_active_changed           (BtkComboBox     *combo_box,
							      gpointer         user_data);


/* BtkBuildable method implementation */
static BtkBuildableIface *parent_buildable_iface;

static void     btk_combo_box_buildable_init                 (BtkBuildableIface *iface);
static gboolean btk_combo_box_buildable_custom_tag_start     (BtkBuildable  *buildable,
							      BtkBuilder    *builder,
							      GObject       *child,
							      const gchar   *tagname,
							      GMarkupParser *parser,
							      gpointer      *data);
static void     btk_combo_box_buildable_custom_tag_end       (BtkBuildable  *buildable,
							      BtkBuilder    *builder,
							      GObject       *child,
							      const gchar   *tagname,
							      gpointer      *data);
static GObject *btk_combo_box_buildable_get_internal_child   (BtkBuildable *buildable,
							      BtkBuilder   *builder,
							      const gchar  *childname);


/* BtkCellEditable method implementations */
static void btk_combo_box_start_editing (BtkCellEditable *cell_editable,
					 BdkEvent        *event);


G_DEFINE_TYPE_WITH_CODE (BtkComboBox, btk_combo_box, BTK_TYPE_BIN,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_CELL_LAYOUT,
						btk_combo_box_cell_layout_init)
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_CELL_EDITABLE,
						btk_combo_box_cell_editable_init)
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
						btk_combo_box_buildable_init))


/* common */
static void
btk_combo_box_class_init (BtkComboBoxClass *klass)
{
  GObjectClass *object_class;
  BtkObjectClass *btk_object_class;
  BtkContainerClass *container_class;
  BtkWidgetClass *widget_class;
  BtkBindingSet *binding_set;

  klass->get_active_text = btk_combo_box_real_get_active_text;

  container_class = (BtkContainerClass *)klass;
  container_class->forall = btk_combo_box_forall;
  container_class->add = btk_combo_box_add;
  container_class->remove = btk_combo_box_remove;

  widget_class = (BtkWidgetClass *)klass;
  widget_class->size_allocate = btk_combo_box_size_allocate;
  widget_class->size_request = btk_combo_box_size_request;
  widget_class->expose_event = btk_combo_box_expose_event;
  widget_class->scroll_event = btk_combo_box_scroll_event;
  widget_class->mnemonic_activate = btk_combo_box_mnemonic_activate;
  widget_class->grab_focus = btk_combo_box_grab_focus;
  widget_class->style_set = btk_combo_box_style_set;
  widget_class->state_changed = btk_combo_box_state_changed;

  btk_object_class = (BtkObjectClass *)klass;
  btk_object_class->destroy = btk_combo_box_destroy;

  object_class = (GObjectClass *)klass;
  object_class->constructor = btk_combo_box_constructor;
  object_class->dispose = btk_combo_box_dispose;
  object_class->finalize = btk_combo_box_finalize;
  object_class->set_property = btk_combo_box_set_property;
  object_class->get_property = btk_combo_box_get_property;

  /* signals */
  /**
   * BtkComboBox::changed:
   * @widget: the object which received the signal
   * 
   * The changed signal is emitted when the active
   * item is changed. The can be due to the user selecting
   * a different item from the list, or due to a 
   * call to btk_combo_box_set_active_iter().
   * It will also be emitted while typing into a BtkComboBoxEntry, 
   * as well as when selecting an item from the BtkComboBoxEntry's list.
   *
   * Since: 2.4
   */
  combo_box_signals[CHANGED] =
    g_signal_new (I_("changed"),
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkComboBoxClass, changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  /**
   * BtkComboBox::move-active:
   * @widget: the object that received the signal
   * @scroll_type: a #BtkScrollType
   *
   * The ::move-active signal is a 
   * <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted to move the active selection.
   *
   * Since: 2.12
   */
  combo_box_signals[MOVE_ACTIVE] =
    g_signal_new_class_handler (I_("move-active"),
                                G_OBJECT_CLASS_TYPE (klass),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (btk_combo_box_real_move_active),
                                NULL, NULL,
                                g_cclosure_marshal_VOID__ENUM,
                                G_TYPE_NONE, 1,
                                BTK_TYPE_SCROLL_TYPE);

  /**
   * BtkComboBox::popup:
   * @widget: the object that received the signal
   *
   * The ::popup signal is a 
   * <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted to popup the combo box list.
   *
   * The default binding for this signal is Alt+Down.
   *
   * Since: 2.12
   */
  combo_box_signals[POPUP] =
    g_signal_new_class_handler (I_("popup"),
                                G_OBJECT_CLASS_TYPE (klass),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (btk_combo_box_real_popup),
                                NULL, NULL,
                                g_cclosure_marshal_VOID__VOID,
                                G_TYPE_NONE, 0);
  /**
   * BtkComboBox::popdown:
   * @button: the object which received the signal
   *
   * The ::popdown signal is a 
   * <link linkend="keybinding-signals">keybinding signal</link> 
   * which gets emitted to popdown the combo box list.
   *
   * The default bindings for this signal are Alt+Up and Escape.
   *
   * Since: 2.12
   */
  combo_box_signals[POPDOWN] =
    g_signal_new_class_handler (I_("popdown"),
                                G_OBJECT_CLASS_TYPE (klass),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (btk_combo_box_real_popdown),
                                NULL, NULL,
                                _btk_marshal_BOOLEAN__VOID,
                                G_TYPE_BOOLEAN, 0);

  /* key bindings */
  binding_set = btk_binding_set_by_class (widget_class);

  btk_binding_entry_add_signal (binding_set, BDK_Down, BDK_MOD1_MASK,
				"popup", 0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Down, BDK_MOD1_MASK,
				"popup", 0);

  btk_binding_entry_add_signal (binding_set, BDK_Up, BDK_MOD1_MASK,
				"popdown", 0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Up, BDK_MOD1_MASK,
				"popdown", 0);
  btk_binding_entry_add_signal (binding_set, BDK_Escape, 0,
				"popdown", 0);

  btk_binding_entry_add_signal (binding_set, BDK_Up, 0,
				"move-active", 1,
				BTK_TYPE_SCROLL_TYPE, BTK_SCROLL_STEP_UP);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Up, 0,
				"move-active", 1,
				BTK_TYPE_SCROLL_TYPE, BTK_SCROLL_STEP_UP);
  btk_binding_entry_add_signal (binding_set, BDK_Page_Up, 0,
				"move-active", 1,
				BTK_TYPE_SCROLL_TYPE, BTK_SCROLL_PAGE_UP);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Page_Up, 0,
				"move-active", 1,
				BTK_TYPE_SCROLL_TYPE, BTK_SCROLL_PAGE_UP);
  btk_binding_entry_add_signal (binding_set, BDK_Home, 0,
				"move-active", 1,
				BTK_TYPE_SCROLL_TYPE, BTK_SCROLL_START);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Home, 0,
				"move-active", 1,
				BTK_TYPE_SCROLL_TYPE, BTK_SCROLL_START);

  btk_binding_entry_add_signal (binding_set, BDK_Down, 0,
				"move-active", 1,
				BTK_TYPE_SCROLL_TYPE, BTK_SCROLL_STEP_DOWN);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Down, 0,
				"move-active", 1,
				BTK_TYPE_SCROLL_TYPE, BTK_SCROLL_STEP_DOWN);
  btk_binding_entry_add_signal (binding_set, BDK_Page_Down, 0,
				"move-active", 1,
				BTK_TYPE_SCROLL_TYPE, BTK_SCROLL_PAGE_DOWN);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Page_Down, 0,
				"move-active", 1,
				BTK_TYPE_SCROLL_TYPE, BTK_SCROLL_PAGE_DOWN);
  btk_binding_entry_add_signal (binding_set, BDK_End, 0,
				"move-active", 1,
				BTK_TYPE_SCROLL_TYPE, BTK_SCROLL_END);
  btk_binding_entry_add_signal (binding_set, BDK_KP_End, 0,
				"move-active", 1,
				BTK_TYPE_SCROLL_TYPE, BTK_SCROLL_END);

  /* properties */
  g_object_class_override_property (object_class,
                                    PROP_EDITING_CANCELED,
                                    "editing-canceled");

  /**
   * BtkComboBox:model:
   *
   * The model from which the combo box takes the values shown
   * in the list. 
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_MODEL,
                                   g_param_spec_object ("model",
                                                        P_("ComboBox model"),
                                                        P_("The model for the combo box"),
                                                        BTK_TYPE_TREE_MODEL,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkComboBox:wrap-width:
   *
   * If wrap-width is set to a positive value, items in the popup will be laid
   * out along multiple columns, starting a new row on reaching the wrap width.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_WRAP_WIDTH,
                                   g_param_spec_int ("wrap-width",
                                                     P_("Wrap width"),
                                                     P_("Wrap width for laying out the items in a grid"),
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     BTK_PARAM_READWRITE));


  /**
   * BtkComboBox:row-span-column:
   *
   * If this is set to a non-negative value, it must be the index of a column
   * of type %G_TYPE_INT in the model. The value in that column for each item
   * will determine how many rows that item will span in the popup. Therefore,
   * values in this column must be greater than zero.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_ROW_SPAN_COLUMN,
                                   g_param_spec_int ("row-span-column",
                                                     P_("Row span column"),
                                                     P_("TreeModel column containing the row span values"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     BTK_PARAM_READWRITE));


  /**
   * BtkComboBox:column-span-column:
   *
   * If this is set to a non-negative value, it must be the index of a column
   * of type %G_TYPE_INT in the model. The value in that column for each item
   * will determine how many columns that item will span in the popup.
   * Therefore, values in this column must be greater than zero, and the sum of
   * an item’s column position + span should not exceed #BtkComboBox:wrap-width.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_COLUMN_SPAN_COLUMN,
                                   g_param_spec_int ("column-span-column",
                                                     P_("Column span column"),
                                                     P_("TreeModel column containing the column span values"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     BTK_PARAM_READWRITE));


  /**
   * BtkComboBox:active:
   *
   * The item which is currently active. If the model is a non-flat treemodel,
   * and the active item is not an immediate child of the root of the tree,
   * this property has the value 
   * <literal>btk_tree_path_get_indices (path)[0]</literal>,
   * where <literal>path</literal> is the #BtkTreePath of the active item.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_ACTIVE,
                                   g_param_spec_int ("active",
                                                     P_("Active item"),
                                                     P_("The item which is currently active"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     BTK_PARAM_READWRITE));

  /**
   * BtkComboBox:add-tearoffs:
   *
   * The add-tearoffs property controls whether generated menus 
   * have tearoff menu items. 
   *
   * Note that this only affects menu style combo boxes.
   *
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
                                   PROP_ADD_TEAROFFS,
				   g_param_spec_boolean ("add-tearoffs",
							 P_("Add tearoffs to menus"),
							 P_("Whether dropdowns should have a tearoff menu item"),
							 FALSE,
							 BTK_PARAM_READWRITE));
  
  /**
   * BtkComboBox:has-frame:
   *
   * The has-frame property controls whether a frame
   * is drawn around the entry.
   *
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
                                   PROP_HAS_FRAME,
				   g_param_spec_boolean ("has-frame",
							 P_("Has Frame"),
							 P_("Whether the combo box draws a frame around the child"),
							 TRUE,
							 BTK_PARAM_READWRITE));
  
  g_object_class_install_property (object_class,
                                   PROP_FOCUS_ON_CLICK,
                                   g_param_spec_boolean ("focus-on-click",
							 P_("Focus on click"),
							 P_("Whether the combo box grabs focus when it is clicked with the mouse"),
							 TRUE,
							 BTK_PARAM_READWRITE));

  /**
   * BtkComboBox:tearoff-title:
   *
   * A title that may be displayed by the window manager 
   * when the popup is torn-off.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class,
                                   PROP_TEAROFF_TITLE,
                                   g_param_spec_string ("tearoff-title",
                                                        P_("Tearoff Title"),
                                                        P_("A title that may be displayed by the window manager when the popup is torn-off"),
                                                        NULL,
                                                        BTK_PARAM_READWRITE));


  /**
   * BtkComboBox:popup-shown:
   *
   * Whether the combo boxes dropdown is popped up. 
   * Note that this property is mainly useful, because
   * it allows you to connect to notify::popup-shown.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class,
                                   PROP_POPUP_SHOWN,
                                   g_param_spec_boolean ("popup-shown",
                                                         P_("Popup shown"),
                                                         P_("Whether the combo's dropdown is shown"),
                                                         FALSE,
                                                         BTK_PARAM_READABLE));


   /**
    * BtkComboBox:button-sensitivity:
    *
    * Whether the dropdown button is sensitive when
    * the model is empty.
    *
    * Since: 2.14
    */
   g_object_class_install_property (object_class,
                                    PROP_BUTTON_SENSITIVITY,
                                    g_param_spec_enum ("button-sensitivity",
                                                       P_("Button Sensitivity"),
                                                       P_("Whether the dropdown button is sensitive when the model is empty"),
                                                       BTK_TYPE_SENSITIVITY_TYPE,
                                                       BTK_SENSITIVITY_AUTO,
                                                       BTK_PARAM_READWRITE));

   /**
    * BtkComboBox:has-entry:
    *
    * Whether the combo box has an entry.
    *
    * Since: 2.24
    */
   g_object_class_install_property (object_class,
                                    PROP_HAS_ENTRY,
                                    g_param_spec_boolean ("has-entry",
							  P_("Has Entry"),
							  P_("Whether combo box has an entry"),
							  FALSE,
							  BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

   /**
    * BtkComboBox:entry-text-column:
    *
    * The column in the combo box's model to associate with strings from the entry
    * if the combo was created with #BtkComboBox:has-entry = %TRUE.
    *
    * Since: 2.24
    */
   g_object_class_install_property (object_class,
                                    PROP_ENTRY_TEXT_COLUMN,
                                    g_param_spec_int ("entry-text-column",
						      P_("Entry Text Column"),
						      P_("The column in the combo box's model to associate "
							 "with strings from the entry if the combo was "
							 "created with #BtkComboBox:has-entry = %TRUE"),
						      -1, G_MAXINT, -1,
						      BTK_PARAM_READWRITE));

  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boolean ("appears-as-list",
                                                                 P_("Appears as list"),
                                                                 P_("Whether dropdowns should look like lists rather than menus"),
                                                                 FALSE,
                                                                 BTK_PARAM_READABLE));

  /**
   * BtkComboBox:arrow-size:
   *
   * Sets the minimum size of the arrow in the combo box.  Note
   * that the arrow size is coupled to the font size, so in case
   * a larger font is used, the arrow will be larger than set
   * by arrow size.
   *
   * Since: 2.12
   */
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("arrow-size",
							     P_("Arrow Size"),
							     P_("The minimum size of the arrow in the combo box"),
							     0,
							     G_MAXINT,
							     15,
							     BTK_PARAM_READABLE));

  /**
   * BtkComboBox:shadow-type:
   *
   * Which kind of shadow to draw around the combo box.
   *
   * Since: 2.12
   */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("shadow-type",
                                                              P_("Shadow type"),
                                                              P_("Which kind of shadow to draw around the combo box"),
                                                              BTK_TYPE_SHADOW_TYPE,
                                                              BTK_SHADOW_NONE,
                                                              BTK_PARAM_READABLE));

  g_type_class_add_private (object_class, sizeof (BtkComboBoxPrivate));
}

static void
btk_combo_box_buildable_init (BtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = _btk_cell_layout_buildable_add_child;
  iface->custom_tag_start = btk_combo_box_buildable_custom_tag_start;
  iface->custom_tag_end = btk_combo_box_buildable_custom_tag_end;
  iface->get_internal_child = btk_combo_box_buildable_get_internal_child;
}

static void
btk_combo_box_cell_layout_init (BtkCellLayoutIface *iface)
{
  iface->pack_start = btk_combo_box_cell_layout_pack_start;
  iface->pack_end = btk_combo_box_cell_layout_pack_end;
  iface->get_cells = btk_combo_box_cell_layout_get_cells;
  iface->clear = btk_combo_box_cell_layout_clear;
  iface->add_attribute = btk_combo_box_cell_layout_add_attribute;
  iface->set_cell_data_func = btk_combo_box_cell_layout_set_cell_data_func;
  iface->clear_attributes = btk_combo_box_cell_layout_clear_attributes;
  iface->reorder = btk_combo_box_cell_layout_reorder;
}

static void
btk_combo_box_cell_editable_init (BtkCellEditableIface *iface)
{
  iface->start_editing = btk_combo_box_start_editing;
}

static void
btk_combo_box_init (BtkComboBox *combo_box)
{
  BtkComboBoxPrivate *priv = BTK_COMBO_BOX_GET_PRIVATE (combo_box);

  priv->cell_view = btk_cell_view_new ();
  btk_widget_set_parent (priv->cell_view, BTK_WIDGET (combo_box));
  BTK_BIN (combo_box)->child = priv->cell_view;
  btk_widget_show (priv->cell_view);

  priv->width = 0;
  priv->height = 0;
  priv->wrap_width = 0;

  priv->active = -1;
  priv->active_row = NULL;
  priv->col_column = -1;
  priv->row_column = -1;

  priv->popup_shown = FALSE;
  priv->add_tearoffs = FALSE;
  priv->has_frame = TRUE;
  priv->is_cell_renderer = FALSE;
  priv->editing_canceled = FALSE;
  priv->auto_scroll = FALSE;
  priv->focus_on_click = TRUE;
  priv->button_sensitivity = BTK_SENSITIVITY_AUTO;
  priv->has_entry = FALSE;

  priv->text_column = -1;
  priv->text_renderer = NULL;

  combo_box->priv = priv;

  btk_combo_box_check_appearance (combo_box);
}

static void
btk_combo_box_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      btk_combo_box_set_model (combo_box, g_value_get_object (value));
      break;

    case PROP_WRAP_WIDTH:
      btk_combo_box_set_wrap_width (combo_box, g_value_get_int (value));
      break;

    case PROP_ROW_SPAN_COLUMN:
      btk_combo_box_set_row_span_column (combo_box, g_value_get_int (value));
      break;

    case PROP_COLUMN_SPAN_COLUMN:
      btk_combo_box_set_column_span_column (combo_box, g_value_get_int (value));
      break;

    case PROP_ACTIVE:
      btk_combo_box_set_active (combo_box, g_value_get_int (value));
      break;

    case PROP_ADD_TEAROFFS:
      btk_combo_box_set_add_tearoffs (combo_box, g_value_get_boolean (value));
      break;

    case PROP_HAS_FRAME:
      combo_box->priv->has_frame = g_value_get_boolean (value);

      if (combo_box->priv->has_entry)
        {
          BtkWidget *child;

          child = btk_bin_get_child (BTK_BIN (combo_box));

          btk_entry_set_has_frame (BTK_ENTRY (child),
                                   combo_box->priv->has_frame);
        }

      break;

    case PROP_FOCUS_ON_CLICK:
      btk_combo_box_set_focus_on_click (combo_box,
                                        g_value_get_boolean (value));
      break;

    case PROP_TEAROFF_TITLE:
      btk_combo_box_set_title (combo_box, g_value_get_string (value));
      break;

    case PROP_POPUP_SHOWN:
      if (g_value_get_boolean (value))
        btk_combo_box_popup (combo_box);
      else
        btk_combo_box_popdown (combo_box);
      break;

    case PROP_BUTTON_SENSITIVITY:
      btk_combo_box_set_button_sensitivity (combo_box,
                                            g_value_get_enum (value));
      break;

    case PROP_EDITING_CANCELED:
      combo_box->priv->editing_canceled = g_value_get_boolean (value);
      break;

    case PROP_HAS_ENTRY:
      combo_box->priv->has_entry = g_value_get_boolean (value);
      break;

    case PROP_ENTRY_TEXT_COLUMN:
      btk_combo_box_set_entry_text_column (combo_box, g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_combo_box_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (object);
  BtkComboBoxPrivate *priv = BTK_COMBO_BOX_GET_PRIVATE (combo_box);

  switch (prop_id)
    {
      case PROP_MODEL:
        g_value_set_object (value, combo_box->priv->model);
        break;

      case PROP_WRAP_WIDTH:
        g_value_set_int (value, combo_box->priv->wrap_width);
        break;

      case PROP_ROW_SPAN_COLUMN:
        g_value_set_int (value, combo_box->priv->row_column);
        break;

      case PROP_COLUMN_SPAN_COLUMN:
        g_value_set_int (value, combo_box->priv->col_column);
        break;

      case PROP_ACTIVE:
        g_value_set_int (value, btk_combo_box_get_active (combo_box));
        break;

      case PROP_ADD_TEAROFFS:
        g_value_set_boolean (value, btk_combo_box_get_add_tearoffs (combo_box));
        break;

      case PROP_HAS_FRAME:
        g_value_set_boolean (value, combo_box->priv->has_frame);
        break;

      case PROP_FOCUS_ON_CLICK:
        g_value_set_boolean (value, combo_box->priv->focus_on_click);
        break;

      case PROP_TEAROFF_TITLE:
        g_value_set_string (value, btk_combo_box_get_title (combo_box));
        break;

      case PROP_POPUP_SHOWN:
        g_value_set_boolean (value, combo_box->priv->popup_shown);
        break;

      case PROP_BUTTON_SENSITIVITY:
        g_value_set_enum (value, combo_box->priv->button_sensitivity);
        break;

      case PROP_EDITING_CANCELED:
        g_value_set_boolean (value, priv->editing_canceled);
        break;

      case PROP_HAS_ENTRY:
	g_value_set_boolean (value, priv->has_entry);
	break;

      case PROP_ENTRY_TEXT_COLUMN:
	g_value_set_int (value, priv->text_column);
	break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
btk_combo_box_state_changed (BtkWidget    *widget,
			     BtkStateType  previous)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (widget);
  BtkComboBoxPrivate *priv = combo_box->priv;

  if (btk_widget_get_realized (widget))
    {
      if (priv->tree_view && priv->cell_view)
	btk_cell_view_set_background_color (BTK_CELL_VIEW (priv->cell_view), 
					    &widget->style->base[btk_widget_get_state (widget)]);
    }

  btk_widget_queue_draw (widget);
}

static void
btk_combo_box_button_state_changed (BtkWidget    *widget,
				    BtkStateType  previous,
				    gpointer      data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (data);
  BtkComboBoxPrivate *priv = combo_box->priv;

  if (btk_widget_get_realized (widget))
    {
      if (!priv->tree_view && priv->cell_view)
	{
	  if ((btk_widget_get_state (widget) == BTK_STATE_INSENSITIVE) !=
	      (btk_widget_get_state (priv->cell_view) == BTK_STATE_INSENSITIVE))
	    btk_widget_set_sensitive (priv->cell_view, btk_widget_get_sensitive (widget));
	  
	  btk_widget_set_state (priv->cell_view, 
				btk_widget_get_state (widget));
	}
    }

  btk_widget_queue_draw (widget);
}

static void
btk_combo_box_check_appearance (BtkComboBox *combo_box)
{
  BtkComboBoxPrivate *priv = combo_box->priv;
  gboolean appears_as_list;

  /* if wrap_width > 0, then we are in grid-mode and forced to use
   * unix style
   */
  if (priv->wrap_width)
    appears_as_list = FALSE;
  else
    btk_widget_style_get (BTK_WIDGET (combo_box),
			  "appears-as-list", &appears_as_list,
			  NULL);

  if (appears_as_list)
    {
      /* Destroy all the menu mode widgets, if they exist. */
      if (BTK_IS_MENU (priv->popup_widget))
	btk_combo_box_menu_destroy (combo_box);

      /* Create the list mode widgets, if they don't already exist. */
      if (!BTK_IS_TREE_VIEW (priv->tree_view))
	btk_combo_box_list_setup (combo_box);
    }
  else
    {
      /* Destroy all the list mode widgets, if they exist. */
      if (BTK_IS_TREE_VIEW (priv->tree_view))
	btk_combo_box_list_destroy (combo_box);

      /* Create the menu mode widgets, if they don't already exist. */
      if (!BTK_IS_MENU (priv->popup_widget))
	btk_combo_box_menu_setup (combo_box, TRUE);
    }

  btk_widget_style_get (BTK_WIDGET (combo_box),
			"shadow-type", &priv->shadow_type,
			NULL);
}

static void
btk_combo_box_style_set (BtkWidget *widget,
                         BtkStyle  *previous)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (widget);
  BtkComboBoxPrivate *priv = combo_box->priv;

  btk_combo_box_check_appearance (combo_box);

  if (priv->tree_view && priv->cell_view)
    btk_cell_view_set_background_color (BTK_CELL_VIEW (priv->cell_view), 
					&widget->style->base[btk_widget_get_state (widget)]);

  if (BTK_IS_ENTRY (BTK_BIN (combo_box)->child))
    g_object_set (BTK_BIN (combo_box)->child, "shadow-type",
                  BTK_SHADOW_NONE == priv->shadow_type ?
                  BTK_SHADOW_IN : BTK_SHADOW_NONE, NULL);
}

static void
btk_combo_box_button_toggled (BtkWidget *widget,
                              gpointer   data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (data);

  if (btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (widget)))
    {
      if (!combo_box->priv->popup_in_progress)
        btk_combo_box_popup (combo_box);
    }
  else
    btk_combo_box_popdown (combo_box);
}

static void
btk_combo_box_add (BtkContainer *container,
                   BtkWidget    *widget)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (container);
  BtkComboBoxPrivate *priv = combo_box->priv;

  if (priv->has_entry && !BTK_IS_ENTRY (widget))
    {
      g_warning ("Attempting to add a widget with type %s to a BtkComboBox that needs an entry "
		 "(need an instance of BtkEntry or of a subclass)",
                 G_OBJECT_TYPE_NAME (widget));
      return;
    }

  if (priv->cell_view &&
      btk_widget_get_parent (priv->cell_view))
    {
      btk_widget_unparent (priv->cell_view);
      BTK_BIN (container)->child = NULL;
      btk_widget_queue_resize (BTK_WIDGET (container));
    }
  
  btk_widget_set_parent (widget, BTK_WIDGET (container));
  BTK_BIN (container)->child = widget;

  if (priv->cell_view &&
      widget != priv->cell_view)
    {
      /* since the cell_view was unparented, it's gone now */
      priv->cell_view = NULL;

      if (!priv->tree_view && priv->separator)
        {
	  btk_container_remove (BTK_CONTAINER (priv->separator->parent),
				priv->separator);
	  priv->separator = NULL;

          btk_widget_queue_resize (BTK_WIDGET (container));
        }
      else if (priv->cell_view_frame)
        {
          btk_widget_unparent (priv->cell_view_frame);
          priv->cell_view_frame = NULL;
          priv->box = NULL;
        }
    }

  if (priv->has_entry)
    {
      /* this flag is a hack to tell the entry to fill its allocation.
       */
      BTK_ENTRY (widget)->is_cell_renderer = TRUE;

      g_signal_connect (widget, "changed",
			G_CALLBACK (btk_combo_box_entry_contents_changed),
			combo_box);

      btk_entry_set_has_frame (BTK_ENTRY (widget), priv->has_frame);
    }
}

static void
btk_combo_box_remove (BtkContainer *container,
		      BtkWidget    *widget)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (container);
  BtkComboBoxPrivate *priv = combo_box->priv;
  BtkTreePath *path;
  gboolean appears_as_list;

  if (priv->has_entry)
    {
      BtkWidget *child_widget;

      child_widget = btk_bin_get_child (BTK_BIN (container));
      if (widget && widget == child_widget)
	{
	  g_signal_handlers_disconnect_by_func (widget,
						btk_combo_box_entry_contents_changed,
						container);
	  BTK_ENTRY (widget)->is_cell_renderer = FALSE;
	}
    }

  if (widget == priv->cell_view)
    priv->cell_view = NULL;

  btk_widget_unparent (widget);
  BTK_BIN (container)->child = NULL;

  if (BTK_OBJECT_FLAGS (combo_box) & BTK_IN_DESTRUCTION)
    return;

  btk_widget_queue_resize (BTK_WIDGET (container));

  if (!priv->tree_view)
    appears_as_list = FALSE;
  else
    appears_as_list = TRUE;
  
  if (appears_as_list)
    btk_combo_box_list_destroy (combo_box);
  else if (BTK_IS_MENU (priv->popup_widget))
    {
      btk_combo_box_menu_destroy (combo_box);
      btk_menu_detach (BTK_MENU (priv->popup_widget));
      priv->popup_widget = NULL;
    }

  if (!priv->cell_view)
    {
      priv->cell_view = btk_cell_view_new ();
      btk_widget_set_parent (priv->cell_view, BTK_WIDGET (container));
      BTK_BIN (container)->child = priv->cell_view;
      
      btk_widget_show (priv->cell_view);
      btk_cell_view_set_model (BTK_CELL_VIEW (priv->cell_view),
			       priv->model);
      btk_combo_box_sync_cells (combo_box, BTK_CELL_LAYOUT (priv->cell_view));
    }


  if (appears_as_list)
    btk_combo_box_list_setup (combo_box); 
  else
    btk_combo_box_menu_setup (combo_box, TRUE);

  if (btk_tree_row_reference_valid (priv->active_row))
    {
      path = btk_tree_row_reference_get_path (priv->active_row);
      btk_combo_box_set_active_internal (combo_box, path);
      btk_tree_path_free (path);
    }
  else
    btk_combo_box_set_active_internal (combo_box, NULL);
}

static ComboCellInfo *
btk_combo_box_get_cell_info (BtkComboBox     *combo_box,
                             BtkCellRenderer *cell)
{
  GSList *i;

  for (i = combo_box->priv->cells; i; i = i->next)
    {
      ComboCellInfo *info = (ComboCellInfo *)i->data;

      if (info && info->cell == cell)
        return info;
    }

  return NULL;
}

static void
btk_combo_box_menu_show (BtkWidget *menu,
                         gpointer   user_data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (user_data);
  BtkComboBoxPrivate *priv = combo_box->priv;

  btk_combo_box_child_show (menu, user_data);

  priv->popup_in_progress = TRUE;
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (priv->button),
                                TRUE);
  priv->popup_in_progress = FALSE;
}

static void
btk_combo_box_menu_hide (BtkWidget *menu,
                         gpointer   user_data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (user_data);

  btk_combo_box_child_hide (menu,user_data);

  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (combo_box->priv->button),
                                FALSE);
}

static void
btk_combo_box_detacher (BtkWidget *widget,
			BtkMenu	  *menu)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (widget);
  BtkComboBoxPrivate *priv = combo_box->priv;

  g_return_if_fail (priv->popup_widget == (BtkWidget *) menu);

  g_signal_handlers_disconnect_by_func (menu->toplevel,
					btk_combo_box_menu_show,
					combo_box);
  g_signal_handlers_disconnect_by_func (menu->toplevel,
					btk_combo_box_menu_hide,
					combo_box);
  
  priv->popup_widget = NULL;
}

static gboolean
btk_combo_box_grab_broken_event (BtkWidget          *widget,
                                 BdkEventGrabBroken *event,
                                 gpointer            user_data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (user_data);

  if (event->grab_window == NULL)
    btk_combo_box_popdown (combo_box);

  return TRUE;
}

static void
btk_combo_box_set_popup_widget (BtkComboBox *combo_box,
                                BtkWidget   *popup)
{
  BtkComboBoxPrivate *priv = combo_box->priv;

  if (BTK_IS_MENU (priv->popup_widget))
    {
      btk_menu_detach (BTK_MENU (priv->popup_widget));
      priv->popup_widget = NULL;
    }
  else if (priv->popup_widget)
    {
      btk_container_remove (BTK_CONTAINER (priv->scrolled_window),
                            priv->popup_widget);
      g_object_unref (priv->popup_widget);
      priv->popup_widget = NULL;
    }

  if (BTK_IS_MENU (popup))
    {
      if (priv->popup_window)
        {
          btk_widget_destroy (priv->popup_window);
          priv->popup_window = NULL;
        }

      priv->popup_widget = popup;

      /* 
       * Note that we connect to show/hide on the toplevel, not the
       * menu itself, since the menu is not shown/hidden when it is
       * popped up while torn-off.
       */
      g_signal_connect (BTK_MENU (popup)->toplevel, "show",
                        G_CALLBACK (btk_combo_box_menu_show), combo_box);
      g_signal_connect (BTK_MENU (popup)->toplevel, "hide",
                        G_CALLBACK (btk_combo_box_menu_hide), combo_box);

      btk_menu_attach_to_widget (BTK_MENU (popup),
				 BTK_WIDGET (combo_box),
				 btk_combo_box_detacher);
    }
  else
    {
      if (!priv->popup_window)
        {
          priv->popup_window = btk_window_new (BTK_WINDOW_POPUP);
          btk_widget_set_name (priv->popup_window, "btk-combobox-popup-window");

	  btk_window_set_type_hint (BTK_WINDOW (priv->popup_window),
				    BDK_WINDOW_TYPE_HINT_COMBO);

          g_signal_connect (priv->popup_window, "show",
                            G_CALLBACK (btk_combo_box_child_show),
                            combo_box);
          g_signal_connect (priv->popup_window, "hide",
                            G_CALLBACK (btk_combo_box_child_hide),
                            combo_box);
          g_signal_connect (priv->popup_window, "grab-broken-event",
                            G_CALLBACK (btk_combo_box_grab_broken_event),
                            combo_box);
  	  
	  btk_window_set_resizable (BTK_WINDOW (priv->popup_window), FALSE);

	  priv->scrolled_window = btk_scrolled_window_new (NULL, NULL);
	  
	  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (priv->scrolled_window),
					  BTK_POLICY_NEVER,
					  BTK_POLICY_NEVER);
	  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (priv->scrolled_window),
					       BTK_SHADOW_IN);

          btk_widget_show (priv->scrolled_window);
	  
	  btk_container_add (BTK_CONTAINER (priv->popup_window),
			     priv->scrolled_window);
        }

      btk_container_add (BTK_CONTAINER (priv->scrolled_window),
                         popup);

      btk_widget_show (popup);
      g_object_ref (popup);
      priv->popup_widget = popup;
    }
}

static void
btk_combo_box_menu_position_below (BtkMenu  *menu,
				   gint     *x,
				   gint     *y,
				   gint     *push_in,
				   gpointer  user_data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (user_data);
  gint sx, sy;
  BtkWidget *child;
  BtkRequisition req;
  BdkScreen *screen;
  gint monitor_num;
  BdkRectangle monitor;
  
  /* FIXME: is using the size request here broken? */
  child = BTK_BIN (combo_box)->child;

  sx = sy = 0;

  if (!btk_widget_get_has_window (child))
    {
      sx += child->allocation.x;
      sy += child->allocation.y;
    }

  bdk_window_get_root_coords (child->window, sx, sy, &sx, &sy);

  if (BTK_SHADOW_NONE != combo_box->priv->shadow_type)
    sx -= BTK_WIDGET (combo_box)->style->xthickness;

  btk_widget_size_request (BTK_WIDGET (menu), &req);

  if (btk_widget_get_direction (BTK_WIDGET (combo_box)) == BTK_TEXT_DIR_LTR)
    *x = sx;
  else
    *x = sx + child->allocation.width - req.width;
  *y = sy;

  screen = btk_widget_get_screen (BTK_WIDGET (combo_box));
  monitor_num = bdk_screen_get_monitor_at_window (screen, 
						  BTK_WIDGET (combo_box)->window);
  bdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);
  
  if (*x < monitor.x)
    *x = monitor.x;
  else if (*x + req.width > monitor.x + monitor.width)
    *x = monitor.x + monitor.width - req.width;
  
  if (monitor.y + monitor.height - *y - child->allocation.height >= req.height)
    *y += child->allocation.height;
  else if (*y - monitor.y >= req.height)
    *y -= req.height;
  else if (monitor.y + monitor.height - *y - child->allocation.height > *y - monitor.y) 
    *y += child->allocation.height;
  else
    *y -= req.height;

   *push_in = FALSE;
}

static void
btk_combo_box_menu_position_over (BtkMenu  *menu,
				  gint     *x,
				  gint     *y,
				  gboolean *push_in,
				  gpointer  user_data)
{
  BtkComboBox *combo_box;
  BtkWidget *active;
  BtkWidget *child;
  BtkWidget *widget;
  BtkRequisition requisition;
  GList *children;
  gint screen_width;
  gint menu_xpos;
  gint menu_ypos;
  gint menu_width;

  combo_box = BTK_COMBO_BOX (user_data);
  widget = BTK_WIDGET (combo_box);

  btk_widget_get_child_requisition (BTK_WIDGET (menu), &requisition);
  menu_width = requisition.width;

  active = btk_menu_get_active (BTK_MENU (combo_box->priv->popup_widget));

  menu_xpos = widget->allocation.x;
  menu_ypos = widget->allocation.y + widget->allocation.height / 2 - 2;

  if (active != NULL)
    {
      btk_widget_get_child_requisition (active, &requisition);
      menu_ypos -= requisition.height / 2;
    }

  children = BTK_MENU_SHELL (combo_box->priv->popup_widget)->children;
  while (children)
    {
      child = children->data;

      if (active == child)
	break;

      if (btk_widget_get_visible (child))
	{
	  btk_widget_get_child_requisition (child, &requisition);
	  menu_ypos -= requisition.height;
	}

      children = children->next;
    }

  if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL)
    menu_xpos = menu_xpos + widget->allocation.width - menu_width;

  bdk_window_get_root_coords (widget->window, menu_xpos, menu_ypos,
			      &menu_xpos, &menu_ypos);

  /* Clamp the position on screen */
  screen_width = bdk_screen_get_width (btk_widget_get_screen (widget));
  
  if (menu_xpos < 0)
    menu_xpos = 0;
  else if ((menu_xpos + menu_width) > screen_width)
    menu_xpos -= ((menu_xpos + menu_width) - screen_width);

  *x = menu_xpos;
  *y = menu_ypos;

  *push_in = TRUE;
}

static void
btk_combo_box_menu_position (BtkMenu  *menu,
			     gint     *x,
			     gint     *y,
			     gint     *push_in,
			     gpointer  user_data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (user_data);
  BtkComboBoxPrivate *priv = combo_box->priv;
  BtkWidget *menu_item;


  if (priv->wrap_width > 0 || priv->cell_view == NULL)	
    btk_combo_box_menu_position_below (menu, x, y, push_in, user_data);
  else
    {
      /* FIXME handle nested menus better */
      menu_item = btk_menu_get_active (BTK_MENU (priv->popup_widget));
      if (menu_item)
	btk_menu_shell_select_item (BTK_MENU_SHELL (priv->popup_widget), 
				    menu_item);

      btk_combo_box_menu_position_over (menu, x, y, push_in, user_data);
    }

  if (!btk_widget_get_visible (BTK_MENU (priv->popup_widget)->toplevel))
    btk_window_set_type_hint (BTK_WINDOW (BTK_MENU (priv->popup_widget)->toplevel),
                              BDK_WINDOW_TYPE_HINT_COMBO);
}

static void
btk_combo_box_list_position (BtkComboBox *combo_box, 
			     gint        *x, 
			     gint        *y, 
			     gint        *width,
			     gint        *height)
{
  BtkComboBoxPrivate *priv = combo_box->priv;
  BdkScreen *screen;
  gint monitor_num;
  BdkRectangle monitor;
  BtkRequisition popup_req;
  BtkPolicyType hpolicy, vpolicy;
  
  /* under windows, the drop down list is as wide as the combo box itself.
     see bug #340204 */
  BtkWidget *sample = BTK_WIDGET (combo_box);

  *x = *y = 0;

  if (!btk_widget_get_has_window (sample))
    {
      *x += sample->allocation.x;
      *y += sample->allocation.y;
    }
  
  bdk_window_get_root_coords (sample->window, *x, *y, x, y);

  *width = sample->allocation.width;

  hpolicy = vpolicy = BTK_POLICY_NEVER;
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (priv->scrolled_window),
				  hpolicy, vpolicy);
  btk_widget_size_request (priv->scrolled_window, &popup_req);

  if (popup_req.width > *width)
    {
      hpolicy = BTK_POLICY_ALWAYS;
      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (priv->scrolled_window),
				      hpolicy, vpolicy);
      btk_widget_size_request (priv->scrolled_window, &popup_req);
    }

  *height = popup_req.height;

  screen = btk_widget_get_screen (BTK_WIDGET (combo_box));
  monitor_num = bdk_screen_get_monitor_at_window (screen, 
						  BTK_WIDGET (combo_box)->window);
  bdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

  if (*x < monitor.x)
    *x = monitor.x;
  else if (*x + *width > monitor.x + monitor.width)
    *x = monitor.x + monitor.width - *width;
  
  if (*y + sample->allocation.height + *height <= monitor.y + monitor.height)
    *y += sample->allocation.height;
  else if (*y - *height >= monitor.y)
    *y -= *height;
  else if (monitor.y + monitor.height - (*y + sample->allocation.height) > *y - monitor.y)
    {
      *y += sample->allocation.height;
      *height = monitor.y + monitor.height - *y;
    }
  else 
    {
      *height = *y - monitor.y;
      *y = monitor.y;
    }

  if (popup_req.height > *height)
    {
      vpolicy = BTK_POLICY_ALWAYS;
      
      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (priv->scrolled_window),
				      hpolicy, vpolicy);
    }
} 

static gboolean
cell_view_is_sensitive (BtkCellView *cell_view)
{
  GList *cells, *list;
  gboolean sensitive;
  
  cells = btk_cell_layout_get_cells (BTK_CELL_LAYOUT (cell_view));

  sensitive = FALSE;
  for (list = cells; list; list = list->next)
    {
      g_object_get (list->data, "sensitive", &sensitive, NULL);
      
      if (sensitive)
	break;
    }
  g_list_free (cells);

  return sensitive;
}

static gboolean
tree_column_row_is_sensitive (BtkComboBox *combo_box,
			      BtkTreeIter *iter)
{
  BtkComboBoxPrivate *priv = combo_box->priv;
  GList *cells, *list;
  gboolean sensitive;

  if (!priv->column)
    return TRUE;

  if (priv->row_separator_func)
    {
      if (priv->row_separator_func (priv->model, iter,
                                    priv->row_separator_data))
	return FALSE;
    }

  btk_tree_view_column_cell_set_cell_data (priv->column,
					   priv->model,
					   iter, FALSE, FALSE);

  cells = btk_cell_layout_get_cells (BTK_CELL_LAYOUT (priv->column));

  sensitive = FALSE;
  for (list = cells; list; list = list->next)
    {
      g_object_get (list->data, "sensitive", &sensitive, NULL);
      
      if (sensitive)
	break;
    }
  g_list_free (cells);

  return sensitive;
}

static void
update_menu_sensitivity (BtkComboBox *combo_box,
			 BtkWidget   *menu)
{
  BtkComboBoxPrivate *priv = combo_box->priv;
  GList *children, *child;
  BtkWidget *item, *submenu, *separator;
  BtkWidget *cell_view;
  gboolean sensitive;

  if (!priv->model)
    return;

  children = btk_container_get_children (BTK_CONTAINER (menu));

  for (child = children; child; child = child->next)
    {
      item = BTK_WIDGET (child->data);
      cell_view = BTK_BIN (item)->child;

      if (!BTK_IS_CELL_VIEW (cell_view))
	continue;
      
      submenu = btk_menu_item_get_submenu (BTK_MENU_ITEM (item));
      if (submenu != NULL)
	{
	  btk_widget_set_sensitive (item, TRUE);
	  update_menu_sensitivity (combo_box, submenu);
	}
      else
	{
	  sensitive = cell_view_is_sensitive (BTK_CELL_VIEW (cell_view));

	  if (menu != priv->popup_widget && child == children)
	    {
	      separator = BTK_WIDGET (child->next->data);
	      g_object_set (item, "visible", sensitive, NULL);
	      g_object_set (separator, "visible", sensitive, NULL);
	    }
	  else
	    btk_widget_set_sensitive (item, sensitive);
	}
    }

  g_list_free (children);
}

static void 
btk_combo_box_menu_popup (BtkComboBox *combo_box,
			  guint        button, 
			  guint32      activate_time)
{
  BtkComboBoxPrivate *priv = combo_box->priv;
  BtkTreePath *path;
  gint active_item;
  BtkRequisition requisition;
  gint width;
  
  update_menu_sensitivity (combo_box, priv->popup_widget);

  active_item = -1;
  if (btk_tree_row_reference_valid (priv->active_row))
    {
      path = btk_tree_row_reference_get_path (priv->active_row);
      active_item = btk_tree_path_get_indices (path)[0];
      btk_tree_path_free (path);
      
      if (priv->add_tearoffs)
	active_item++;
    }

  /* FIXME handle nested menus better */
  btk_menu_set_active (BTK_MENU (priv->popup_widget), active_item);
  
  if (priv->wrap_width == 0)
    {
      width = BTK_WIDGET (combo_box)->allocation.width;
      btk_widget_set_size_request (priv->popup_widget, -1, -1);
      btk_widget_size_request (priv->popup_widget, &requisition);
      
      btk_widget_set_size_request (priv->popup_widget,
				   MAX (width, requisition.width), -1);
    }
  
  btk_menu_popup (BTK_MENU (priv->popup_widget),
		  NULL, NULL,
		  btk_combo_box_menu_position, combo_box,
		  button, activate_time);
}

static gboolean
popup_grab_on_window (BdkWindow *window,
		      guint32    activate_time,
		      gboolean   grab_keyboard)
{
  if ((bdk_pointer_grab (window, TRUE,
			 BDK_BUTTON_PRESS_MASK | BDK_BUTTON_RELEASE_MASK |
			 BDK_POINTER_MOTION_MASK,
			 NULL, NULL, activate_time) == 0))
    {
      if (!grab_keyboard ||
	  bdk_keyboard_grab (window, TRUE,
			     activate_time) == 0)
	return TRUE;
      else
	{
	  bdk_display_pointer_ungrab (bdk_window_get_display (window),
				      activate_time);
	  return FALSE;
	}
    }

  return FALSE;
}

/**
 * btk_combo_box_popup:
 * @combo_box: a #BtkComboBox
 * 
 * Pops up the menu or dropdown list of @combo_box. 
 *
 * This function is mostly intended for use by accessibility technologies;
 * applications should have little use for it.
 *
 * Since: 2.4
 */
void
btk_combo_box_popup (BtkComboBox *combo_box)
{
  g_return_if_fail (BTK_IS_COMBO_BOX (combo_box));

  g_signal_emit (combo_box, combo_box_signals[POPUP], 0);
}

static void
btk_combo_box_real_popup (BtkComboBox *combo_box)
{
  BtkComboBoxPrivate *priv = combo_box->priv;
  gint x, y, width, height;
  BtkTreePath *path = NULL, *ppath;
  BtkWidget *toplevel;

  if (!btk_widget_get_realized (BTK_WIDGET (combo_box)))
    return;

  if (btk_widget_get_mapped (priv->popup_widget))
    return;

  if (BTK_IS_MENU (priv->popup_widget))
    {
      btk_combo_box_menu_popup (combo_box, 
                                priv->activate_button,
                                priv->activate_time);
      return;
    }

  toplevel = btk_widget_get_toplevel (BTK_WIDGET (combo_box));
  if (BTK_IS_WINDOW (toplevel))
    {
      btk_window_group_add_window (btk_window_get_group (BTK_WINDOW (toplevel)),
                                   BTK_WINDOW (priv->popup_window));
      btk_window_set_transient_for (BTK_WINDOW (priv->popup_window),
                                    BTK_WINDOW (toplevel));
    }

  btk_widget_show_all (priv->scrolled_window);
  btk_combo_box_list_position (combo_box, &x, &y, &width, &height);
  
  btk_widget_set_size_request (priv->popup_window, width, height);  
  btk_window_move (BTK_WINDOW (priv->popup_window), x, y);

  if (btk_tree_row_reference_valid (priv->active_row))
    {
      path = btk_tree_row_reference_get_path (priv->active_row);
      ppath = btk_tree_path_copy (path);
      if (btk_tree_path_up (ppath))
	btk_tree_view_expand_to_path (BTK_TREE_VIEW (priv->tree_view),
				      ppath);
      btk_tree_path_free (ppath);
    }
  btk_tree_view_set_hover_expand (BTK_TREE_VIEW (priv->tree_view), 
				  TRUE);
  
  /* popup */
  btk_window_set_screen (BTK_WINDOW (priv->popup_window),
                         btk_widget_get_screen (BTK_WIDGET (combo_box)));
  btk_widget_show (priv->popup_window);

  if (path)
    {
      btk_tree_view_set_cursor (BTK_TREE_VIEW (priv->tree_view),
				path, NULL, FALSE);
      btk_tree_path_free (path);
    }

  btk_widget_grab_focus (priv->popup_window);
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (priv->button),
                                TRUE);

  if (!btk_widget_has_focus (priv->tree_view))
    btk_widget_grab_focus (priv->tree_view);

  if (!popup_grab_on_window (priv->popup_window->window,
			     BDK_CURRENT_TIME, TRUE))
    {
      btk_widget_hide (priv->popup_window);
      return;
    }

  btk_grab_add (priv->popup_window);
}

static gboolean
btk_combo_box_real_popdown (BtkComboBox *combo_box)
{
  if (combo_box->priv->popup_shown)
    {
      btk_combo_box_popdown (combo_box);
      return TRUE;
    }

  return FALSE;
}

/**
 * btk_combo_box_popdown:
 * @combo_box: a #BtkComboBox
 * 
 * Hides the menu or dropdown list of @combo_box.
 *
 * This function is mostly intended for use by accessibility technologies;
 * applications should have little use for it.
 *
 * Since: 2.4
 */
void
btk_combo_box_popdown (BtkComboBox *combo_box)
{
  BtkComboBoxPrivate *priv = combo_box->priv;
  BdkDisplay *display;

  g_return_if_fail (BTK_IS_COMBO_BOX (combo_box));

  if (BTK_IS_MENU (priv->popup_widget))
    {
      btk_menu_popdown (BTK_MENU (priv->popup_widget));
      return;
    }

  if (!btk_widget_get_realized (BTK_WIDGET (combo_box)))
    return;

  btk_grab_remove (priv->popup_window);

  display = btk_widget_get_display (BTK_WIDGET (combo_box));
  bdk_display_pointer_ungrab (display, BDK_CURRENT_TIME);
  bdk_display_keyboard_ungrab (display, BDK_CURRENT_TIME);

  btk_widget_hide_all (priv->popup_window);
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (priv->button),
                                FALSE);
}

static gint
btk_combo_box_calc_requested_width (BtkComboBox *combo_box,
                                    BtkTreePath *path)
{
  BtkComboBoxPrivate *priv = combo_box->priv;
  gint padding;
  BtkRequisition req;

  if (priv->cell_view)
    btk_widget_style_get (priv->cell_view,
                          "focus-line-width", &padding,
                          NULL);
  else
    padding = 0;

  /* add some pixels for good measure */
  padding += BONUS_PADDING;

  if (priv->cell_view)
    btk_cell_view_get_size_of_row (BTK_CELL_VIEW (priv->cell_view),
                                   path, &req);
  else
    req.width = 0;

  return req.width + padding;
}

static void
btk_combo_box_remeasure (BtkComboBox *combo_box)
{
  BtkComboBoxPrivate *priv = combo_box->priv;
  BtkTreeIter iter;
  BtkTreePath *path;

  if (!priv->model ||
      !btk_tree_model_get_iter_first (priv->model, &iter))
    return;

  priv->width = 0;
  priv->height = 0;

  path = btk_tree_path_new_from_indices (0, -1);

  do
    {
      BtkRequisition req;

      if (priv->cell_view)
	btk_cell_view_get_size_of_row (BTK_CELL_VIEW (priv->cell_view), 
                                       path, &req);
      else
        {
          req.width = 0;
          req.height = 0;
        }

      priv->width = MAX (priv->width, req.width);
      priv->height = MAX (priv->height, req.height);

      btk_tree_path_next (path);
    }
  while (btk_tree_model_iter_next (priv->model, &iter));

  btk_tree_path_free (path);
}

static void
btk_combo_box_size_request (BtkWidget      *widget,
                            BtkRequisition *requisition)
{
  gint width, height;
  gint focus_width, focus_pad;
  gint font_size;
  gint arrow_size;
  BtkRequisition bin_req;
  BangoContext *context;
  BangoFontMetrics *metrics;
  BangoFontDescription *font_desc;

  BtkComboBox *combo_box = BTK_COMBO_BOX (widget);
  BtkComboBoxPrivate *priv = combo_box->priv;
 
  /* common */
  btk_widget_size_request (BTK_BIN (widget)->child, &bin_req);
  btk_combo_box_remeasure (combo_box);
  bin_req.width = MAX (bin_req.width, priv->width);
  bin_req.height = MAX (bin_req.height, priv->height);

  btk_widget_style_get (BTK_WIDGET (widget),
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			"arrow-size", &arrow_size,
			NULL);

  font_desc = BTK_BIN (widget)->child->style->font_desc;
  context = btk_widget_get_bango_context (widget);
  metrics = bango_context_get_metrics (context, font_desc,
				       bango_context_get_language (context));
  font_size = BANGO_PIXELS (bango_font_metrics_get_ascent (metrics) +
			    bango_font_metrics_get_descent (metrics));
  bango_font_metrics_unref (metrics);

  arrow_size = MAX (arrow_size, font_size);

  btk_widget_set_size_request (priv->arrow, arrow_size, arrow_size);

  if (!priv->tree_view)
    {
      /* menu mode */

      if (priv->cell_view)
        {
          BtkRequisition button_req, sep_req, arrow_req;
          gint border_width, xthickness, ythickness;

          btk_widget_size_request (priv->button, &button_req);
	  border_width = BTK_CONTAINER (combo_box)->border_width;
          xthickness = priv->button->style->xthickness;
          ythickness = priv->button->style->ythickness;

          bin_req.width = MAX (bin_req.width, priv->width);
          bin_req.height = MAX (bin_req.height, priv->height);

          btk_widget_size_request (priv->separator, &sep_req);
          btk_widget_size_request (priv->arrow, &arrow_req);

          height = MAX (sep_req.height, arrow_req.height);
          height = MAX (height, bin_req.height);

          width = bin_req.width + sep_req.width + arrow_req.width;

          height += 2*(border_width + ythickness + focus_width + focus_pad);
          width  += 2*(border_width + xthickness + focus_width + focus_pad);

          requisition->width = width;
          requisition->height = height;
        }
      else
        {
          BtkRequisition but_req;

          btk_widget_size_request (priv->button, &but_req);

          requisition->width = bin_req.width + but_req.width;
          requisition->height = MAX (bin_req.height, but_req.height);
        }
    }
  else
    {
      /* list mode */
      BtkRequisition button_req, frame_req;

      /* sample + frame */
      *requisition = bin_req;

      requisition->width += 2 * focus_width;
      
      if (priv->cell_view_frame)
        {
	  btk_widget_size_request (priv->cell_view_frame, &frame_req);
	  if (priv->has_frame)
	    {
	      requisition->width += 2 *
		(BTK_CONTAINER (priv->cell_view_frame)->border_width +
		 BTK_WIDGET (priv->cell_view_frame)->style->xthickness);
	      requisition->height += 2 *
		(BTK_CONTAINER (priv->cell_view_frame)->border_width +
		 BTK_WIDGET (priv->cell_view_frame)->style->ythickness);
	    }
        }

      /* the button */
      btk_widget_size_request (priv->button, &button_req);

      requisition->height = MAX (requisition->height, button_req.height);
      requisition->width += button_req.width;
    }

  if (BTK_SHADOW_NONE != priv->shadow_type)
    {
      requisition->height += 2 * widget->style->ythickness;
      requisition->width += 2 * widget->style->xthickness;
    }
}

#define BTK_COMBO_BOX_SIZE_ALLOCATE_BUTTON 					\
  btk_widget_size_request (combo_box->priv->button, &req); 			\
  										\
  if (is_rtl) 									\
    child.x = allocation->x + shadow_width;					\
  else										\
    child.x = allocation->x + allocation->width - req.width - shadow_width;	\
    										\
  child.y = allocation->y + shadow_height;					\
  child.width = req.width;							\
  child.height = allocation->height - 2 * shadow_height;			\
  child.width = MAX (1, child.width);						\
  child.height = MAX (1, child.height);						\
  										\
  btk_widget_size_allocate (combo_box->priv->button, &child);

static void
btk_combo_box_size_allocate (BtkWidget     *widget,
                             BtkAllocation *allocation)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (widget);
  BtkComboBoxPrivate *priv = combo_box->priv;
  gint shadow_width, shadow_height;
  gint focus_width, focus_pad;
  BtkAllocation child;
  BtkRequisition req;
  gboolean is_rtl = btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL;

  widget->allocation = *allocation;

  btk_widget_style_get (BTK_WIDGET (widget),
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			NULL);

  if (BTK_SHADOW_NONE != priv->shadow_type)
    {
      shadow_width = widget->style->xthickness;
      shadow_height = widget->style->ythickness;
    }
  else
    {
      shadow_width = 0;
      shadow_height = 0;
    }

  if (!priv->tree_view)
    {
      if (priv->cell_view)
        {
          gint border_width, xthickness, ythickness;
          gint width;

          /* menu mode */
          allocation->x += shadow_width;
          allocation->y += shadow_height;
          allocation->width -= 2 * shadow_width;
          allocation->height -= 2 * shadow_height;

          btk_widget_size_allocate (priv->button, allocation);

          /* set some things ready */
          border_width = BTK_CONTAINER (priv->button)->border_width;
          xthickness = priv->button->style->xthickness;
          ythickness = priv->button->style->ythickness;

          child.x = allocation->x;
          child.y = allocation->y;
	  width = allocation->width;
	  child.height = allocation->height;

	  if (!priv->is_cell_renderer)
	    {
	      child.x += border_width + xthickness + focus_width + focus_pad;
	      child.y += border_width + ythickness + focus_width + focus_pad;
	      width -= 2 * (child.x - allocation->x);
	      child.height -= 2 * (child.y - allocation->y);
	    }


          /* handle the children */
          btk_widget_size_request (priv->arrow, &req);
          child.width = req.width;
          if (!is_rtl)
            child.x += width - req.width;
	  child.width = MAX (1, child.width);
	  child.height = MAX (1, child.height);
          btk_widget_size_allocate (priv->arrow, &child);
          if (is_rtl)
            child.x += req.width;
          btk_widget_size_request (priv->separator, &req);
          child.width = req.width;
          if (!is_rtl)
            child.x -= req.width;
	  child.width = MAX (1, child.width);
	  child.height = MAX (1, child.height);
          btk_widget_size_allocate (priv->separator, &child);

          if (is_rtl)
            {
              child.x += req.width;
              child.width = allocation->x + allocation->width 
                - (border_width + xthickness + focus_width + focus_pad) 
		- child.x;
            }
          else 
            {
              child.width = child.x;
              child.x = allocation->x 
		+ border_width + xthickness + focus_width + focus_pad;
              child.width -= child.x;
            }

          if (btk_widget_get_visible (priv->popup_widget))
            {
              gint width;
              BtkRequisition requisition;

              /* Warning here, without the check in the position func */
              btk_menu_reposition (BTK_MENU (priv->popup_widget));
              if (priv->wrap_width == 0)
                {
                  width = BTK_WIDGET (combo_box)->allocation.width;
                  btk_widget_set_size_request (priv->popup_widget, -1, -1);
                  btk_widget_size_request (priv->popup_widget, &requisition);
                  btk_widget_set_size_request (priv->popup_widget,
                    MAX (width, requisition.width), -1);
               }
            }

	  child.width = MAX (1, child.width);
	  child.height = MAX (1, child.height);
          btk_widget_size_allocate (BTK_BIN (widget)->child, &child);
        }
      else
        {
          BTK_COMBO_BOX_SIZE_ALLOCATE_BUTTON

          if (is_rtl)
            child.x = allocation->x + req.width + shadow_width;
          else
            child.x = allocation->x + shadow_width;
          child.y = allocation->y + shadow_height;
          child.width = allocation->width - req.width - 2 * shadow_width;
	  child.width = MAX (1, child.width);
	  child.height = MAX (1, child.height);
          btk_widget_size_allocate (BTK_BIN (widget)->child, &child);
        }
    }
  else
    {
      /* list mode */

      /* Combobox thickness + border-width */
      int delta_x = shadow_width + BTK_CONTAINER (widget)->border_width;
      int delta_y = shadow_height + BTK_CONTAINER (widget)->border_width;

      /* button */
      BTK_COMBO_BOX_SIZE_ALLOCATE_BUTTON

      /* frame */
      if (is_rtl)
        child.x = allocation->x + req.width;
      else
        child.x = allocation->x;

      child.y = allocation->y;
      child.width = allocation->width - req.width;
      child.height = allocation->height;

      if (priv->cell_view_frame)
        {
          child.x += delta_x;
          child.y += delta_y;
          child.width = MAX (1, child.width - delta_x * 2);
          child.height = MAX (1, child.height - delta_y * 2);
          btk_widget_size_allocate (priv->cell_view_frame, &child);

          /* the sample */
          if (priv->has_frame)
            {
              delta_x = BTK_CONTAINER (priv->cell_view_frame)->border_width +
                BTK_WIDGET (priv->cell_view_frame)->style->xthickness;
              delta_y = BTK_CONTAINER (priv->cell_view_frame)->border_width +
                BTK_WIDGET (priv->cell_view_frame)->style->ythickness;

              child.x += delta_x;
              child.y += delta_y;
              child.width -= delta_x * 2;
              child.height -= delta_y * 2;
            }
        }
      else
        {
          child.x += delta_x;
          child.y += delta_y;
          child.width -= delta_x * 2;
          child.height -= delta_y * 2;
        }

      if (btk_widget_get_visible (priv->popup_window))
        {
          gint x, y, width, height;
          btk_combo_box_list_position (combo_box, &x, &y, &width, &height);
          btk_window_move (BTK_WINDOW (priv->popup_window), x, y);
          btk_widget_set_size_request (priv->popup_window, width, height);
        }

      
      child.width = MAX (1, child.width);
      child.height = MAX (1, child.height);
      
      btk_widget_size_allocate (BTK_BIN (combo_box)->child, &child);
    }
}

#undef BTK_COMBO_BOX_ALLOCATE_BUTTON

static void
btk_combo_box_unset_model (BtkComboBox *combo_box)
{
  BtkComboBoxPrivate *priv = combo_box->priv;

  if (priv->model)
    {
      g_signal_handler_disconnect (priv->model,
				   priv->inserted_id);
      g_signal_handler_disconnect (priv->model,
				   priv->deleted_id);
      g_signal_handler_disconnect (priv->model,
				   priv->reordered_id);
      g_signal_handler_disconnect (priv->model,
				   priv->changed_id);
    }

  /* menu mode */
  if (!priv->tree_view)
    {
      if (priv->popup_widget)
        btk_container_foreach (BTK_CONTAINER (priv->popup_widget),
                               (BtkCallback)btk_widget_destroy, NULL);
    }

  if (priv->model)
    {
      g_object_unref (priv->model);
      priv->model = NULL;
    }

  if (priv->active_row)
    {
      btk_tree_row_reference_free (priv->active_row);
      priv->active_row = NULL;
    }

  if (priv->cell_view)
    btk_cell_view_set_model (BTK_CELL_VIEW (priv->cell_view), NULL);
}

static void
btk_combo_box_forall (BtkContainer *container,
                      gboolean      include_internals,
                      BtkCallback   callback,
                      gpointer      callback_data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (container);
  BtkComboBoxPrivate *priv = combo_box->priv;

  if (include_internals)
    {
      if (priv->button)
	(* callback) (priv->button, callback_data);
      if (priv->cell_view_frame)
	(* callback) (priv->cell_view_frame, callback_data);
    }

  if (BTK_BIN (container)->child)
    (* callback) (BTK_BIN (container)->child, callback_data);
}

static void 
btk_combo_box_child_show (BtkWidget *widget,
                          BtkComboBox *combo_box)
{
  BtkComboBoxPrivate *priv = combo_box->priv;

  priv->popup_shown = TRUE;
  g_object_notify (G_OBJECT (combo_box), "popup-shown");
}

static void 
btk_combo_box_child_hide (BtkWidget *widget,
                          BtkComboBox *combo_box)
{
  BtkComboBoxPrivate *priv = combo_box->priv;

  priv->popup_shown = FALSE;
  g_object_notify (G_OBJECT (combo_box), "popup-shown");
}

static gboolean
btk_combo_box_expose_event (BtkWidget      *widget,
                            BdkEventExpose *event)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (widget);
  BtkComboBoxPrivate *priv = combo_box->priv;

  if (btk_widget_is_drawable (widget) &&
      BTK_SHADOW_NONE != priv->shadow_type)
    {
      btk_paint_shadow (widget->style, widget->window,
                        BTK_STATE_NORMAL, priv->shadow_type,
                        NULL, widget, "combobox",
                        widget->allocation.x, widget->allocation.y,
                        widget->allocation.width, widget->allocation.height);
    }

  btk_container_propagate_expose (BTK_CONTAINER (widget),
				  priv->button, event);

  if (priv->tree_view && priv->cell_view_frame)
    {
      btk_container_propagate_expose (BTK_CONTAINER (widget),
				      priv->cell_view_frame, event);
    }

  btk_container_propagate_expose (BTK_CONTAINER (widget),
                                  BTK_BIN (widget)->child, event);

  return FALSE;
}

typedef struct {
  BtkComboBox *combo;
  BtkTreePath *path;
  BtkTreeIter iter;
  gboolean found;
  gboolean set;
  gboolean visible;
} SearchData;

static gboolean
path_visible (BtkTreeView *view,
	      BtkTreePath *path)
{
  BtkRBTree *tree;
  BtkRBNode *node;
  
  /* Note that we rely on the fact that collapsed rows don't have nodes 
   */
  return _btk_tree_view_find_node (view, path, &tree, &node);
}

static gboolean
tree_next_func (BtkTreeModel *model,
		BtkTreePath  *path,
		BtkTreeIter  *iter,
		gpointer      data)
{
  SearchData *search_data = (SearchData *)data;

  if (search_data->found) 
    {
      if (!tree_column_row_is_sensitive (search_data->combo, iter))
	return FALSE;
      
      if (search_data->visible &&
	  !path_visible (BTK_TREE_VIEW (search_data->combo->priv->tree_view), path))
	return FALSE;

      search_data->set = TRUE;
      search_data->iter = *iter;

      return TRUE;
    }
 
  if (btk_tree_path_compare (path, search_data->path) == 0)
    search_data->found = TRUE;
  
  return FALSE;
}

static gboolean
tree_next (BtkComboBox  *combo,
	   BtkTreeModel *model,
	   BtkTreeIter  *iter,
	   BtkTreeIter  *next,
	   gboolean      visible)
{
  SearchData search_data;

  search_data.combo = combo;
  search_data.path = btk_tree_model_get_path (model, iter);
  search_data.visible = visible;
  search_data.found = FALSE;
  search_data.set = FALSE;

  btk_tree_model_foreach (model, tree_next_func, &search_data);
  
  *next = search_data.iter;

  btk_tree_path_free (search_data.path);

  return search_data.set;
}

static gboolean
tree_prev_func (BtkTreeModel *model,
		BtkTreePath  *path,
		BtkTreeIter  *iter,
		gpointer      data)
{
  SearchData *search_data = (SearchData *)data;

  if (btk_tree_path_compare (path, search_data->path) == 0)
    {
      search_data->found = TRUE;
      return TRUE;
    }
  
  if (!tree_column_row_is_sensitive (search_data->combo, iter))
    return FALSE;
      
  if (search_data->visible &&
      !path_visible (BTK_TREE_VIEW (search_data->combo->priv->tree_view), path))
    return FALSE; 
  
  search_data->set = TRUE;
  search_data->iter = *iter;
  
  return FALSE; 
}

static gboolean
tree_prev (BtkComboBox  *combo,
	   BtkTreeModel *model,
	   BtkTreeIter  *iter,
	   BtkTreeIter  *prev,
	   gboolean      visible)
{
  SearchData search_data;

  search_data.combo = combo;
  search_data.path = btk_tree_model_get_path (model, iter);
  search_data.visible = visible;
  search_data.found = FALSE;
  search_data.set = FALSE;

  btk_tree_model_foreach (model, tree_prev_func, &search_data);
  
  *prev = search_data.iter;

  btk_tree_path_free (search_data.path);

  return search_data.set;
}

static gboolean
tree_last_func (BtkTreeModel *model,
		BtkTreePath  *path,
		BtkTreeIter  *iter,
		gpointer      data)
{
  SearchData *search_data = (SearchData *)data;

  if (!tree_column_row_is_sensitive (search_data->combo, iter))
    return FALSE;
      
  /* Note that we rely on the fact that collapsed rows don't have nodes 
   */
  if (search_data->visible &&
      !path_visible (BTK_TREE_VIEW (search_data->combo->priv->tree_view), path))
    return FALSE; 
  
  search_data->set = TRUE;
  search_data->iter = *iter;
  
  return FALSE; 
}

static gboolean
tree_last (BtkComboBox  *combo,
	   BtkTreeModel *model,
	   BtkTreeIter  *last,
	   gboolean      visible)
{
  SearchData search_data;

  search_data.combo = combo;
  search_data.visible = visible;
  search_data.set = FALSE;

  btk_tree_model_foreach (model, tree_last_func, &search_data);
  
  *last = search_data.iter;

  return search_data.set;  
}


static gboolean
tree_first_func (BtkTreeModel *model,
		 BtkTreePath  *path,
		 BtkTreeIter  *iter,
		 gpointer      data)
{
  SearchData *search_data = (SearchData *)data;

  if (!tree_column_row_is_sensitive (search_data->combo, iter))
    return FALSE;
  
  if (search_data->visible &&
      !path_visible (BTK_TREE_VIEW (search_data->combo->priv->tree_view), path))
    return FALSE;
  
  search_data->set = TRUE;
  search_data->iter = *iter;
  
  return TRUE;
}

static gboolean
tree_first (BtkComboBox  *combo,
	    BtkTreeModel *model,
	    BtkTreeIter  *first,
	    gboolean      visible)
{
  SearchData search_data;
  
  search_data.combo = combo;
  search_data.visible = visible;
  search_data.set = FALSE;

  btk_tree_model_foreach (model, tree_first_func, &search_data);
  
  *first = search_data.iter;

  return search_data.set;  
}

static gboolean
btk_combo_box_scroll_event (BtkWidget          *widget,
                            BdkEventScroll     *event)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (widget);
  gboolean found;
  BtkTreeIter iter;
  BtkTreeIter new_iter;

  if (!btk_combo_box_get_active_iter (combo_box, &iter))
    return TRUE;
  
  if (event->direction == BDK_SCROLL_UP)
    found = tree_prev (combo_box, combo_box->priv->model, 
		       &iter, &new_iter, FALSE);
  else
    found = tree_next (combo_box, combo_box->priv->model, 
		       &iter, &new_iter, FALSE);
  
  if (found)
    btk_combo_box_set_active_iter (combo_box, &new_iter);

  return TRUE;
}

/*
 * menu style
 */

static void
btk_combo_box_sync_cells (BtkComboBox   *combo_box,
			  BtkCellLayout *cell_layout)
{
  BtkComboBoxPrivate *priv = combo_box->priv;
  GSList *k;

  for (k = priv->cells; k; k = k->next)
    {
      GSList *j;
      ComboCellInfo *info = (ComboCellInfo *)k->data;

      if (info->pack == BTK_PACK_START)
        btk_cell_layout_pack_start (cell_layout,
                                    info->cell, info->expand);
      else if (info->pack == BTK_PACK_END)
        btk_cell_layout_pack_end (cell_layout,
                                  info->cell, info->expand);

      btk_cell_layout_set_cell_data_func (cell_layout,
                                          info->cell,
                                          combo_cell_data_func, info, NULL);

      for (j = info->attributes; j; j = j->next->next)
        {
          btk_cell_layout_add_attribute (cell_layout,
                                         info->cell,
                                         j->data,
                                         GPOINTER_TO_INT (j->next->data));
        }
    }
}

static void
btk_combo_box_menu_setup (BtkComboBox *combo_box,
                          gboolean     add_children)
{
  BtkComboBoxPrivate *priv = combo_box->priv;
  BtkWidget *menu;

  if (priv->cell_view)
    {
      priv->button = btk_toggle_button_new ();
      btk_button_set_focus_on_click (BTK_BUTTON (priv->button),
				     priv->focus_on_click);

      g_signal_connect (priv->button, "toggled",
                        G_CALLBACK (btk_combo_box_button_toggled), combo_box);
      btk_widget_set_parent (priv->button,
                             BTK_BIN (combo_box)->child->parent);

      priv->box = btk_hbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (priv->button), priv->box);

      priv->separator = btk_vseparator_new ();
      btk_container_add (BTK_CONTAINER (priv->box), priv->separator);

      priv->arrow = btk_arrow_new (BTK_ARROW_DOWN, BTK_SHADOW_NONE);
      btk_container_add (BTK_CONTAINER (priv->box), priv->arrow);

      btk_widget_show_all (priv->button);
    }
  else
    {
      priv->button = btk_toggle_button_new ();
      btk_button_set_focus_on_click (BTK_BUTTON (priv->button),
				     priv->focus_on_click);

      g_signal_connect (priv->button, "toggled",
                        G_CALLBACK (btk_combo_box_button_toggled), combo_box);
      btk_widget_set_parent (priv->button,
                             BTK_BIN (combo_box)->child->parent);

      priv->arrow = btk_arrow_new (BTK_ARROW_DOWN, BTK_SHADOW_NONE);
      btk_container_add (BTK_CONTAINER (priv->button), priv->arrow);
      btk_widget_show_all (priv->button);
    }

  g_signal_connect (priv->button, "button-press-event",
                    G_CALLBACK (btk_combo_box_menu_button_press),
                    combo_box);
  g_signal_connect (priv->button, "state-changed",
		    G_CALLBACK (btk_combo_box_button_state_changed), 
		    combo_box);

  /* create our funky menu */
  menu = btk_menu_new ();
  btk_widget_set_name (menu, "btk-combobox-popup-menu");
  btk_menu_set_reserve_toggle_size (BTK_MENU (menu), FALSE);
  
  g_signal_connect (menu, "key-press-event",
		    G_CALLBACK (btk_combo_box_menu_key_press), combo_box);
  btk_combo_box_set_popup_widget (combo_box, menu);

  /* add items */
  if (add_children)
    btk_combo_box_menu_fill (combo_box);

  /* the column is needed in tree_column_row_is_sensitive() */
  priv->column = btk_tree_view_column_new ();
  g_object_ref_sink (priv->column);
  btk_combo_box_sync_cells (combo_box, BTK_CELL_LAYOUT (priv->column));

  btk_combo_box_update_title (combo_box);
  btk_combo_box_update_sensitivity (combo_box);
}

static void
btk_combo_box_menu_fill (BtkComboBox *combo_box)
{
  BtkComboBoxPrivate *priv = combo_box->priv;
  BtkWidget *menu;

  if (!priv->model)
    return;

  menu = priv->popup_widget;

  if (priv->add_tearoffs)
    {
      BtkWidget *tearoff = btk_tearoff_menu_item_new ();

      btk_widget_show (tearoff);
      
      if (priv->wrap_width)
	btk_menu_attach (BTK_MENU (menu), tearoff, 0, priv->wrap_width, 0, 1);
      else
	btk_menu_shell_append (BTK_MENU_SHELL (menu), tearoff);
    }
  
  btk_combo_box_menu_fill_level (combo_box, menu, NULL);
}

static BtkWidget *
btk_cell_view_menu_item_new (BtkComboBox  *combo_box,
			     BtkTreeModel *model,
			     BtkTreeIter  *iter)
{
  BtkWidget *cell_view; 
  BtkWidget *item;
  BtkTreePath *path;
  BtkRequisition req;

  cell_view = btk_cell_view_new ();
  item = btk_menu_item_new ();
  btk_container_add (BTK_CONTAINER (item), cell_view);

  btk_cell_view_set_model (BTK_CELL_VIEW (cell_view), model);	  
  path = btk_tree_model_get_path (model, iter);
  btk_cell_view_set_displayed_row (BTK_CELL_VIEW (cell_view), path);
  btk_tree_path_free (path);

  btk_combo_box_sync_cells (combo_box, BTK_CELL_LAYOUT (cell_view));
  btk_widget_size_request (cell_view, &req);
  btk_widget_show (cell_view);
  
  return item;
}
 
static void
btk_combo_box_menu_fill_level (BtkComboBox *combo_box,
			       BtkWidget   *menu,
			       BtkTreeIter *parent)
{
  BtkComboBoxPrivate *priv = combo_box->priv;
  BtkTreeModel *model = priv->model;
  BtkWidget *item, *submenu, *subitem, *separator;
  BtkTreeIter iter;
  gboolean is_separator;
  gint i, n_children;
  BtkWidget *last;
  BtkTreePath *path;
  
  n_children = btk_tree_model_iter_n_children (model, parent);
  
  last = NULL;
  for (i = 0; i < n_children; i++)
    {
      btk_tree_model_iter_nth_child (model, &iter, parent, i);

      if (priv->row_separator_func)
	is_separator = priv->row_separator_func (priv->model, &iter,
                                                 priv->row_separator_data);
      else
	is_separator = FALSE;
      
      if (is_separator)
	{
	  item = btk_separator_menu_item_new ();
	  path = btk_tree_model_get_path (model, &iter);
	  g_object_set_data_full (G_OBJECT (item),
				  I_("btk-combo-box-item-path"),
				  btk_tree_row_reference_new (model, path),
				  (GDestroyNotify)btk_tree_row_reference_free);
	  btk_tree_path_free (path);
	}
      else
	{
	  item = btk_cell_view_menu_item_new (combo_box, model, &iter);
	  if (btk_tree_model_iter_has_child (model, &iter))
	    {
	      submenu = btk_menu_new ();
              btk_menu_set_reserve_toggle_size (BTK_MENU (submenu), FALSE);
	      btk_widget_show (submenu);
	      btk_menu_item_set_submenu (BTK_MENU_ITEM (item), submenu);
	      
	      /* Ugly - since menus can only activate leafs, we have to
	       * duplicate the item inside the submenu.
	       */
	      subitem = btk_cell_view_menu_item_new (combo_box, model, &iter);
	      separator = btk_separator_menu_item_new ();
	      btk_widget_show (subitem);
	      btk_widget_show (separator);
	      g_signal_connect (subitem, "activate",
				G_CALLBACK (btk_combo_box_menu_item_activate),
				combo_box);
	      btk_menu_shell_append (BTK_MENU_SHELL (submenu), subitem);
	      btk_menu_shell_append (BTK_MENU_SHELL (submenu), separator);
	      
	      btk_combo_box_menu_fill_level (combo_box, submenu, &iter);
	    }
	  g_signal_connect (item, "activate",
			    G_CALLBACK (btk_combo_box_menu_item_activate),
			    combo_box);
	}
      
      btk_menu_shell_append (BTK_MENU_SHELL (menu), item);
      if (priv->wrap_width && menu == priv->popup_widget)
        btk_combo_box_relayout_item (combo_box, item, &iter, last);
      btk_widget_show (item);
      
      last = item;
    }
}

static void
btk_combo_box_menu_destroy (BtkComboBox *combo_box)
{
  BtkComboBoxPrivate *priv = combo_box->priv;

  g_signal_handlers_disconnect_by_func (priv->button,
                                        btk_combo_box_button_toggled,
                                        combo_box);
  g_signal_handlers_disconnect_by_func (priv->button,
                                        btk_combo_box_menu_button_press,
                                        combo_box);
  g_signal_handlers_disconnect_by_func (priv->button,
                                        btk_combo_box_button_state_changed,
                                        combo_box);
  g_signal_handlers_disconnect_by_data (priv->popup_widget, combo_box);

  /* unparent will remove our latest ref */
  btk_widget_unparent (priv->button);
  
  priv->box = NULL;
  priv->button = NULL;
  priv->arrow = NULL;
  priv->separator = NULL;

  g_object_unref (priv->column);
  priv->column = NULL;

  /* changing the popup window will unref the menu and the children */
}

/*
 * grid
 */

static gboolean
menu_occupied (BtkMenu   *menu,
               guint      left_attach,
               guint      right_attach,
               guint      top_attach,
               guint      bottom_attach)
{
  GList *i;

  for (i = BTK_MENU_SHELL (menu)->children; i; i = i->next)
    {
      guint l, r, b, t;

      btk_container_child_get (BTK_CONTAINER (menu), 
			       i->data,
                               "left-attach", &l,
                               "right-attach", &r,
                               "bottom-attach", &b,
                               "top-attach", &t,
                               NULL);

      /* look if this item intersects with the given coordinates */
      if (right_attach > l && left_attach < r && bottom_attach > t && top_attach < b)
	return TRUE;
    }

  return FALSE;
}

static void
btk_combo_box_relayout_item (BtkComboBox *combo_box,
			     BtkWidget   *item,
                             BtkTreeIter *iter,
			     BtkWidget   *last)
{
  BtkComboBoxPrivate *priv = combo_box->priv;
  gint current_col = 0, current_row = 0;
  gint rows = 1, cols = 1;
  BtkWidget *menu = priv->popup_widget;

  if (!BTK_IS_MENU_SHELL (menu))
    return;
  
  if (priv->col_column == -1 &&
      priv->row_column == -1 &&
      last)
    {
      btk_container_child_get (BTK_CONTAINER (menu), 
			       last,
			       "right-attach", &current_col,
			       "top-attach", &current_row,
			       NULL);
      if (current_col + cols > priv->wrap_width)
	{
	  current_col = 0;
	  current_row++;
	}
    }
  else
    {
      if (priv->col_column != -1)
	btk_tree_model_get (priv->model, iter,
			    priv->col_column, &cols,
			    -1);
      if (priv->row_column != -1)
	btk_tree_model_get (priv->model, iter,
			    priv->row_column, &rows,
			    -1);

      while (1)
	{
	  if (current_col + cols > priv->wrap_width)
	    {
	      current_col = 0;
	      current_row++;
	    }
	  
	  if (!menu_occupied (BTK_MENU (menu), 
			      current_col, current_col + cols,
			      current_row, current_row + rows))
	    break;
	  
	  current_col++;
	}
    }

  /* set attach props */
  btk_menu_attach (BTK_MENU (menu), item,
                   current_col, current_col + cols,
                   current_row, current_row + rows);
}

static void
btk_combo_box_relayout (BtkComboBox *combo_box)
{
  GList *list, *j;
  BtkWidget *menu;

  menu = combo_box->priv->popup_widget;
  
  /* do nothing unless we are in menu style and realized */
  if (combo_box->priv->tree_view || !BTK_IS_MENU_SHELL (menu))
    return;
  
  list = btk_container_get_children (BTK_CONTAINER (menu));
  
  for (j = g_list_last (list); j; j = j->prev)
    btk_container_remove (BTK_CONTAINER (menu), j->data);
  
  btk_combo_box_menu_fill (combo_box);

  g_list_free (list);
}

/* callbacks */
static gboolean
btk_combo_box_menu_button_press (BtkWidget      *widget,
                                 BdkEventButton *event,
                                 gpointer        user_data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (user_data);
  BtkComboBoxPrivate *priv = combo_box->priv;

  if (BTK_IS_MENU (priv->popup_widget) &&
      event->type == BDK_BUTTON_PRESS && event->button == 1)
    {
      if (priv->focus_on_click && 
	  !btk_widget_has_focus (priv->button))
	btk_widget_grab_focus (priv->button);

      btk_combo_box_menu_popup (combo_box, event->button, event->time);

      return TRUE;
    }

  return FALSE;
}

static void
btk_combo_box_menu_item_activate (BtkWidget *item,
                                  gpointer   user_data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (user_data);
  BtkWidget *cell_view;
  BtkTreePath *path;
  BtkTreeIter iter;

  cell_view = BTK_BIN (item)->child;

  g_return_if_fail (BTK_IS_CELL_VIEW (cell_view));

  path = btk_cell_view_get_displayed_row (BTK_CELL_VIEW (cell_view));

  if (btk_tree_model_get_iter (combo_box->priv->model, &iter, path))
  {
    if (btk_menu_item_get_submenu (BTK_MENU_ITEM (item)) == NULL)
      btk_combo_box_set_active_iter (combo_box, &iter);
  }

  btk_tree_path_free (path);

  g_object_set (combo_box,
                "editing-canceled", FALSE,
                NULL);
}

static void
btk_combo_box_update_sensitivity (BtkComboBox *combo_box)
{
  BtkTreeIter iter;
  gboolean sensitive = TRUE; /* fool code checkers */

  if (!combo_box->priv->button)
    return;

  switch (combo_box->priv->button_sensitivity)
    {
      case BTK_SENSITIVITY_ON:
        sensitive = TRUE;
        break;
      case BTK_SENSITIVITY_OFF:
        sensitive = FALSE;
        break;
      case BTK_SENSITIVITY_AUTO:
        sensitive = combo_box->priv->model &&
                    btk_tree_model_get_iter_first (combo_box->priv->model, &iter);
        break;
      default:
        g_assert_not_reached ();
        break;
    }

  btk_widget_set_sensitive (combo_box->priv->button, sensitive);

  /* In list-mode, we also need to update sensitivity of the event box */
  if (BTK_IS_TREE_VIEW (combo_box->priv->tree_view)
      && combo_box->priv->cell_view)
    btk_widget_set_sensitive (combo_box->priv->box, sensitive);
}

static void
btk_combo_box_model_row_inserted (BtkTreeModel     *model,
				  BtkTreePath      *path,
				  BtkTreeIter      *iter,
				  gpointer          user_data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (user_data);

  if (combo_box->priv->tree_view)
    btk_combo_box_list_popup_resize (combo_box);
  else
    btk_combo_box_menu_row_inserted (model, path, iter, user_data);

  btk_combo_box_update_sensitivity (combo_box);
}

static void
btk_combo_box_model_row_deleted (BtkTreeModel     *model,
				 BtkTreePath      *path,
				 gpointer          user_data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (user_data);
  BtkComboBoxPrivate *priv = combo_box->priv;

  if (!btk_tree_row_reference_valid (priv->active_row))
    {
      if (priv->cell_view)
	btk_cell_view_set_displayed_row (BTK_CELL_VIEW (priv->cell_view), NULL);
      g_signal_emit (combo_box, combo_box_signals[CHANGED], 0);
    }
  
  if (priv->tree_view)
    btk_combo_box_list_popup_resize (combo_box);
  else
    btk_combo_box_menu_row_deleted (model, path, user_data);  

  btk_combo_box_update_sensitivity (combo_box);
}

static void
btk_combo_box_model_rows_reordered (BtkTreeModel    *model,
				    BtkTreePath     *path,
				    BtkTreeIter     *iter,
				    gint            *new_order,
				    gpointer         user_data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (user_data);

  btk_tree_row_reference_reordered (G_OBJECT (user_data), path, iter, new_order);

  if (!combo_box->priv->tree_view)
    btk_combo_box_menu_rows_reordered (model, path, iter, new_order, user_data);
}
						    
static void
btk_combo_box_model_row_changed (BtkTreeModel     *model,
				 BtkTreePath      *path,
				 BtkTreeIter      *iter,
				 gpointer          user_data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (user_data);
  BtkComboBoxPrivate *priv = combo_box->priv;
  BtkTreePath *active_path;

  /* FIXME this belongs to BtkCellView */
  if (btk_tree_row_reference_valid (priv->active_row))
    {
      active_path = btk_tree_row_reference_get_path (priv->active_row);
      if (btk_tree_path_compare (path, active_path) == 0 &&
	  priv->cell_view)
	btk_widget_queue_resize (BTK_WIDGET (priv->cell_view));
      btk_tree_path_free (active_path);
    }
      
  if (priv->tree_view)
    btk_combo_box_list_row_changed (model, path, iter, user_data);
  else
    btk_combo_box_menu_row_changed (model, path, iter, user_data);
}

static gboolean
list_popup_resize_idle (gpointer user_data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (user_data);
  BtkComboBoxPrivate *priv = combo_box->priv;
  gint x, y, width, height;

  if (priv->tree_view && btk_widget_get_mapped (priv->popup_window))
    {
      btk_combo_box_list_position (combo_box, &x, &y, &width, &height);
  
      btk_widget_set_size_request (priv->popup_window, width, height);
      btk_window_move (BTK_WINDOW (priv->popup_window), x, y);
    }

  priv->resize_idle_id = 0;

  return FALSE;
}

static void
btk_combo_box_list_popup_resize (BtkComboBox *combo_box)
{
  BtkComboBoxPrivate *priv = combo_box->priv;

  if (!priv->resize_idle_id)
    priv->resize_idle_id = 
      bdk_threads_add_idle (list_popup_resize_idle, combo_box);
}

static void
btk_combo_box_model_row_expanded (BtkTreeModel     *model,
				  BtkTreePath      *path,
				  BtkTreeIter      *iter,
				  gpointer          user_data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (user_data);
  
  btk_combo_box_list_popup_resize (combo_box);
}


static BtkWidget *
find_menu_by_path (BtkWidget   *menu,
		   BtkTreePath *path,
		   gboolean     skip_first)
{
  GList *i, *list;
  BtkWidget *item;
  BtkWidget *submenu;    
  BtkTreeRowReference *mref;
  BtkTreePath *mpath;
  gboolean skip;

  list = btk_container_get_children (BTK_CONTAINER (menu));
  skip = skip_first;
  item = NULL;
  for (i = list; i; i = i->next)
    {
      if (BTK_IS_SEPARATOR_MENU_ITEM (i->data))
	{
	  mref = g_object_get_data (G_OBJECT (i->data), "btk-combo-box-item-path");
	  if (!mref)
	    continue;
	  else if (!btk_tree_row_reference_valid (mref))
	    mpath = NULL;
	  else
	    mpath = btk_tree_row_reference_get_path (mref);
	}
      else if (BTK_IS_CELL_VIEW (BTK_BIN (i->data)->child))
	{
	  if (skip)
	    {
	      skip = FALSE;
	      continue;
	    }

	  mpath = btk_cell_view_get_displayed_row (BTK_CELL_VIEW (BTK_BIN (i->data)->child));
	}
      else 
	continue;

      /* this case is necessary, since the row reference of
       * the cell view may already be updated after a deletion
       */
      if (!mpath)
	{
	  item = i->data;
	  break;
	}
      if (btk_tree_path_compare (mpath, path) == 0)
	{
	  btk_tree_path_free (mpath);
	  item = i->data;
	  break;
	}
      if (btk_tree_path_is_ancestor (mpath, path))
	{
	  submenu = btk_menu_item_get_submenu (BTK_MENU_ITEM (i->data));
	  if (submenu != NULL)
	    {
	      btk_tree_path_free (mpath);
	      item = find_menu_by_path (submenu, path, TRUE);
	      break;
	    }
	}
      btk_tree_path_free (mpath);
    }
  
  g_list_free (list);  

  return item;
}

#if 0
static void
dump_menu_tree (BtkWidget   *menu, 
		gint         level)
{
  GList *i, *list;
  BtkWidget *submenu;    
  BtkTreePath *path;

  list = btk_container_get_children (BTK_CONTAINER (menu));
  for (i = list; i; i = i->next)
    {
      if (BTK_IS_CELL_VIEW (BTK_BIN (i->data)->child))
	{
	  path = btk_cell_view_get_displayed_row (BTK_CELL_VIEW (BTK_BIN (i->data)->child));
	  g_print ("%*s%s\n", 2 * level, " ", btk_tree_path_to_string (path));
	  btk_tree_path_free (path);

	  submenu = btk_menu_item_get_submenu (BTK_MENU_ITEM (i->data));
	  if (submenu != NULL)
	    dump_menu_tree (submenu, level + 1);
	}
    }
  
  g_list_free (list);  
}
#endif

static void
btk_combo_box_menu_row_inserted (BtkTreeModel *model,
                                 BtkTreePath  *path,
                                 BtkTreeIter  *iter,
                                 gpointer      user_data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (user_data);
  BtkComboBoxPrivate *priv = combo_box->priv;
  BtkWidget *parent;
  BtkWidget *item, *menu, *separator;
  BtkTreePath *ppath;
  BtkTreeIter piter;
  gint depth, pos;
  gboolean is_separator;

  if (!priv->popup_widget)
    return;

  depth = btk_tree_path_get_depth (path);
  pos = btk_tree_path_get_indices (path)[depth - 1];
  if (depth > 1)
    {
      ppath = btk_tree_path_copy (path);
      btk_tree_path_up (ppath);
      parent = find_menu_by_path (priv->popup_widget, ppath, FALSE);
      btk_tree_path_free (ppath);

      menu = btk_menu_item_get_submenu (BTK_MENU_ITEM (parent));
      if (!menu)
	{
	  menu = btk_menu_new ();
          btk_menu_set_reserve_toggle_size (BTK_MENU (menu), FALSE);
	  btk_widget_show (menu);
	  btk_menu_item_set_submenu (BTK_MENU_ITEM (parent), menu);
	  
	  /* Ugly - since menus can only activate leaves, we have to
	   * duplicate the item inside the submenu.
	   */
	  btk_tree_model_iter_parent (model, &piter, iter);
	  item = btk_cell_view_menu_item_new (combo_box, model, &piter);
	  separator = btk_separator_menu_item_new ();
	  g_signal_connect (item, "activate",
			    G_CALLBACK (btk_combo_box_menu_item_activate),
			    combo_box);
	  btk_menu_shell_append (BTK_MENU_SHELL (menu), item);
	  btk_menu_shell_append (BTK_MENU_SHELL (menu), separator);
	  if (cell_view_is_sensitive (BTK_CELL_VIEW (BTK_BIN (item)->child)))
	    {
	      btk_widget_show (item);
	      btk_widget_show (separator);
	    }
	}
      pos += 2;
    }
  else
    {
      menu = priv->popup_widget;
      if (priv->add_tearoffs)
	pos += 1;
    }
  
  if (priv->row_separator_func)
    is_separator = priv->row_separator_func (model, iter,
                                             priv->row_separator_data);
  else
    is_separator = FALSE;

  if (is_separator)
    {
      item = btk_separator_menu_item_new ();
      g_object_set_data_full (G_OBJECT (item),
			      I_("btk-combo-box-item-path"),
			      btk_tree_row_reference_new (model, path),
			      (GDestroyNotify)btk_tree_row_reference_free);
    }
  else
    {
      item = btk_cell_view_menu_item_new (combo_box, model, iter);
      
      g_signal_connect (item, "activate",
			G_CALLBACK (btk_combo_box_menu_item_activate),
			combo_box);
    }

  btk_widget_show (item);
  btk_menu_shell_insert (BTK_MENU_SHELL (menu), item, pos);
}

static void
btk_combo_box_menu_row_deleted (BtkTreeModel *model,
                                BtkTreePath  *path,
                                gpointer      user_data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (user_data);
  BtkComboBoxPrivate *priv = combo_box->priv;
  BtkWidget *menu;
  BtkWidget *item;

  if (!priv->popup_widget)
    return;

  item = find_menu_by_path (priv->popup_widget, path, FALSE);
  menu = btk_widget_get_parent (item);
  btk_container_remove (BTK_CONTAINER (menu), item);

  if (btk_tree_path_get_depth (path) > 1)
    {
      BtkTreePath *parent_path;
      BtkTreeIter iter;
      BtkWidget *parent;

      parent_path = btk_tree_path_copy (path);
      btk_tree_path_up (parent_path);
      btk_tree_model_get_iter (model, &iter, parent_path);

      if (!btk_tree_model_iter_has_child (model, &iter))
	{
	  parent = find_menu_by_path (priv->popup_widget, 
				      parent_path, FALSE);
	  btk_menu_item_set_submenu (BTK_MENU_ITEM (parent), NULL);
	}
    }
}

static void
btk_combo_box_menu_rows_reordered  (BtkTreeModel     *model,
				    BtkTreePath      *path,
				    BtkTreeIter      *iter,
	      			    gint             *new_order,
				    gpointer          user_data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (user_data);

  btk_combo_box_relayout (combo_box);
}
				    
static void
btk_combo_box_menu_row_changed (BtkTreeModel *model,
                                BtkTreePath  *path,
                                BtkTreeIter  *iter,
                                gpointer      user_data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (user_data);
  BtkComboBoxPrivate *priv = combo_box->priv;
  BtkWidget *item;
  gint width;
  gboolean is_separator;

  if (!priv->popup_widget)
    return;

  item = find_menu_by_path (priv->popup_widget, path, FALSE);

  if (priv->row_separator_func)
    is_separator = priv->row_separator_func (model, iter,
                                             priv->row_separator_data);
  else
    is_separator = FALSE;

  if (is_separator != BTK_IS_SEPARATOR_MENU_ITEM (item))
    {
      btk_combo_box_menu_row_deleted (model, path, combo_box);
      btk_combo_box_menu_row_inserted (model, path, iter, combo_box);
    }

  if (priv->wrap_width && item->parent == priv->popup_widget)
    {
      BtkWidget *pitem = NULL;
      BtkTreePath *prev;

      prev = btk_tree_path_copy (path);

      if (btk_tree_path_prev (prev))
        pitem = find_menu_by_path (priv->popup_widget, prev, FALSE);

      btk_tree_path_free (prev);

      /* unattach item so btk_combo_box_relayout_item() won't spuriously
         move it */
      btk_container_child_set (BTK_CONTAINER (priv->popup_widget),
                               item, 
			       "left-attach", -1, 
			       "right-attach", -1,
                               "top-attach", -1, 
			       "bottom-attach", -1, 
			       NULL);

      btk_combo_box_relayout_item (combo_box, item, iter, pitem);
    }

  width = btk_combo_box_calc_requested_width (combo_box, path);

  if (width > priv->width)
    {
      if (priv->cell_view)
	{
	  btk_widget_set_size_request (priv->cell_view, width, -1);
	  btk_widget_queue_resize (priv->cell_view);
	}
      priv->width = width;
    }
}

/*
 * list style
 */

static void
btk_combo_box_list_setup (BtkComboBox *combo_box)
{
  BtkComboBoxPrivate *priv = combo_box->priv;
  BtkTreeSelection *sel;
  BtkStyle *style;
  BtkWidget *widget = BTK_WIDGET (combo_box);

  priv->button = btk_toggle_button_new ();
  btk_widget_set_parent (priv->button,
                         BTK_BIN (combo_box)->child->parent);
  g_signal_connect (priv->button, "button-press-event",
                    G_CALLBACK (btk_combo_box_list_button_pressed), combo_box);
  g_signal_connect (priv->button, "toggled",
                    G_CALLBACK (btk_combo_box_button_toggled), combo_box);

  priv->arrow = btk_arrow_new (BTK_ARROW_DOWN, BTK_SHADOW_NONE);
  btk_container_add (BTK_CONTAINER (priv->button), priv->arrow);
  priv->separator = NULL;
  btk_widget_show_all (priv->button);

  if (priv->cell_view)
    {
      style = btk_widget_get_style (widget);
      btk_cell_view_set_background_color (BTK_CELL_VIEW (priv->cell_view),
                                          &style->base[btk_widget_get_state (widget)]);

      priv->box = btk_event_box_new ();
      btk_event_box_set_visible_window (BTK_EVENT_BOX (priv->box), 
					FALSE);

      if (priv->has_frame)
	{
	  priv->cell_view_frame = btk_frame_new (NULL);
	  btk_frame_set_shadow_type (BTK_FRAME (priv->cell_view_frame),
				     BTK_SHADOW_IN);
	}
      else 
	{
	  combo_box->priv->cell_view_frame = btk_event_box_new ();
	  btk_event_box_set_visible_window (BTK_EVENT_BOX (combo_box->priv->cell_view_frame), 
					    FALSE);
	}
      
      btk_widget_set_parent (priv->cell_view_frame,
			     BTK_BIN (combo_box)->child->parent);
      btk_container_add (BTK_CONTAINER (priv->cell_view_frame), priv->box);
      btk_widget_show_all (priv->cell_view_frame);

      g_signal_connect (priv->box, "button-press-event",
			G_CALLBACK (btk_combo_box_list_button_pressed), 
			combo_box);
    }

  priv->tree_view = btk_tree_view_new ();
  sel = btk_tree_view_get_selection (BTK_TREE_VIEW (priv->tree_view));
  btk_tree_selection_set_mode (sel, BTK_SELECTION_BROWSE);
  btk_tree_selection_set_select_function (sel,
					  btk_combo_box_list_select_func,
					  NULL, NULL);
  btk_tree_view_set_headers_visible (BTK_TREE_VIEW (priv->tree_view),
                                     FALSE);
  btk_tree_view_set_hover_selection (BTK_TREE_VIEW (priv->tree_view),
				     TRUE);
  if (priv->row_separator_func)
    btk_tree_view_set_row_separator_func (BTK_TREE_VIEW (priv->tree_view), 
					  priv->row_separator_func, 
					  priv->row_separator_data, 
					  NULL);
  if (priv->model)
    btk_tree_view_set_model (BTK_TREE_VIEW (priv->tree_view), priv->model);
    
  priv->column = btk_tree_view_column_new ();
  btk_tree_view_append_column (BTK_TREE_VIEW (priv->tree_view), priv->column);

  /* sync up */
  btk_combo_box_sync_cells (combo_box, 
			    BTK_CELL_LAYOUT (priv->column));

  if (btk_tree_row_reference_valid (priv->active_row))
    {
      BtkTreePath *path;

      path = btk_tree_row_reference_get_path (priv->active_row);
      btk_tree_view_set_cursor (BTK_TREE_VIEW (priv->tree_view),
				path, NULL, FALSE);
      btk_tree_path_free (path);
    }

  /* set sample/popup widgets */
  btk_combo_box_set_popup_widget (combo_box, priv->tree_view);

  g_signal_connect (priv->tree_view, "key-press-event",
                    G_CALLBACK (btk_combo_box_list_key_press),
                    combo_box);
  g_signal_connect (priv->tree_view, "enter-notify-event",
                    G_CALLBACK (btk_combo_box_list_enter_notify),
                    combo_box);
  g_signal_connect (priv->tree_view, "row-expanded",
		    G_CALLBACK (btk_combo_box_model_row_expanded),
		    combo_box);
  g_signal_connect (priv->tree_view, "row-collapsed",
		    G_CALLBACK (btk_combo_box_model_row_expanded),
		    combo_box);
  g_signal_connect (priv->popup_window, "button-press-event",
                    G_CALLBACK (btk_combo_box_list_button_pressed),
                    combo_box);
  g_signal_connect (priv->popup_window, "button-release-event",
                    G_CALLBACK (btk_combo_box_list_button_released),
                    combo_box);

  btk_widget_show (priv->tree_view);

  btk_combo_box_update_sensitivity (combo_box);
}

static void
btk_combo_box_list_destroy (BtkComboBox *combo_box)
{
  BtkComboBoxPrivate *priv = combo_box->priv;

  /* disconnect signals */
  g_signal_handlers_disconnect_by_data (priv->tree_view, combo_box);
  g_signal_handlers_disconnect_by_func (priv->button,
                                        btk_combo_box_list_button_pressed,
                                        combo_box);
  g_signal_handlers_disconnect_by_data (priv->popup_window, combo_box);
  
  if (priv->box)
    g_signal_handlers_disconnect_matched (priv->box,
					  G_SIGNAL_MATCH_DATA,
					  0, 0, NULL,
					  btk_combo_box_list_button_pressed,
					  NULL);

  /* destroy things (unparent will kill the latest ref from us)
   * last unref on button will destroy the arrow
   */
  btk_widget_unparent (priv->button);
  priv->button = NULL;
  priv->arrow = NULL;

  if (priv->cell_view)
    {
      g_object_set (priv->cell_view,
                    "background-set", FALSE,
                    NULL);
    }

  if (priv->cell_view_frame)
    {
      btk_widget_unparent (priv->cell_view_frame);
      priv->cell_view_frame = NULL;
      priv->box = NULL;
    }

  if (priv->scroll_timer)
    {
      g_source_remove (priv->scroll_timer);
      priv->scroll_timer = 0;
    }

  if (priv->resize_idle_id)
    {
      g_source_remove (priv->resize_idle_id);
      priv->resize_idle_id = 0;
    }

  btk_widget_destroy (priv->tree_view);

  priv->tree_view = NULL;
  if (priv->popup_widget)
    {
      g_object_unref (priv->popup_widget);
      priv->popup_widget = NULL;
    }
}

/* callbacks */

static gboolean
btk_combo_box_list_button_pressed (BtkWidget      *widget,
                                   BdkEventButton *event,
                                   gpointer        data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (data);
  BtkComboBoxPrivate *priv = combo_box->priv;

  BtkWidget *ewidget = btk_get_event_widget ((BdkEvent *)event);

  if (ewidget == priv->popup_window)
    return TRUE;

  if ((ewidget != priv->button && ewidget != priv->box) ||
      btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (priv->button)))
    return FALSE;

  if (priv->focus_on_click && 
      !btk_widget_has_focus (priv->button))
    btk_widget_grab_focus (priv->button);

  btk_combo_box_popup (combo_box);

  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (priv->button), TRUE);

  priv->auto_scroll = FALSE;
  if (priv->scroll_timer == 0)
    priv->scroll_timer = bdk_threads_add_timeout (SCROLL_TIME, 
						  (GSourceFunc) btk_combo_box_list_scroll_timeout, 
						   combo_box);

  priv->popup_in_progress = TRUE;

  return TRUE;
}

static gboolean
btk_combo_box_list_button_released (BtkWidget      *widget,
                                    BdkEventButton *event,
                                    gpointer        data)
{
  gboolean ret;
  BtkTreePath *path = NULL;
  BtkTreeIter iter;

  BtkComboBox *combo_box = BTK_COMBO_BOX (data);
  BtkComboBoxPrivate *priv = combo_box->priv;

  gboolean popup_in_progress = FALSE;

  BtkWidget *ewidget = btk_get_event_widget ((BdkEvent *)event);

  if (priv->popup_in_progress)
    {
      popup_in_progress = TRUE;
      priv->popup_in_progress = FALSE;
    }

  btk_tree_view_set_hover_expand (BTK_TREE_VIEW (priv->tree_view), 
				  FALSE);
  if (priv->scroll_timer)
    {
      g_source_remove (priv->scroll_timer);
      priv->scroll_timer = 0;
    }

  if (ewidget != priv->tree_view)
    {
      if ((ewidget == priv->button || 
	   ewidget == priv->box) &&
          !popup_in_progress &&
          btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (priv->button)))
        {
          btk_combo_box_popdown (combo_box);
          return TRUE;
        }

      /* released outside treeview */
      if (ewidget != priv->button && ewidget != priv->box)
        {
          btk_combo_box_popdown (combo_box);

          return TRUE;
        }

      return FALSE;
    }

  /* select something cool */
  ret = btk_tree_view_get_path_at_pos (BTK_TREE_VIEW (priv->tree_view),
                                       event->x, event->y,
                                       &path,
                                       NULL, NULL, NULL);

  if (!ret)
    return TRUE; /* clicked outside window? */

  btk_tree_model_get_iter (priv->model, &iter, path);

  /* Use iter before popdown, as mis-users like BtkFileChooserButton alter the
   * model during notify::popped-up, which means the iterator becomes invalid.
   */
  if (tree_column_row_is_sensitive (combo_box, &iter))
    btk_combo_box_set_active_internal (combo_box, path);

  btk_tree_path_free (path);

  btk_combo_box_popdown (combo_box);

  return TRUE;
}

static gboolean
btk_combo_box_menu_key_press (BtkWidget   *widget,
			      BdkEventKey *event,
			      gpointer     data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (data);

  if (!btk_bindings_activate_event (BTK_OBJECT (widget), event))
    {
      /* The menu hasn't managed the
       * event, forward it to the combobox
       */
      btk_bindings_activate_event (BTK_OBJECT (combo_box), event);
    }

  return TRUE;
}

static gboolean
btk_combo_box_list_key_press (BtkWidget   *widget,
                              BdkEventKey *event,
                              gpointer     data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (data);
  BtkTreeIter iter;

  if (event->keyval == BDK_Return || event->keyval == BDK_ISO_Enter || event->keyval == BDK_KP_Enter ||
      event->keyval == BDK_space || event->keyval == BDK_KP_Space) 
  {
    BtkTreeModel *model = NULL;
    
    btk_combo_box_popdown (combo_box);
    
    if (combo_box->priv->model)
      {
	BtkTreeSelection *sel;

	sel = btk_tree_view_get_selection (BTK_TREE_VIEW (combo_box->priv->tree_view));

	if (btk_tree_selection_get_selected (sel, &model, &iter))
	  btk_combo_box_set_active_iter (combo_box, &iter);
      }
    
    return TRUE;
  }

  if (!btk_bindings_activate_event (BTK_OBJECT (widget), event))
    {
      /* The list hasn't managed the
       * event, forward it to the combobox
       */
      btk_bindings_activate_event (BTK_OBJECT (combo_box), event);
    }

  return TRUE;
}

static void
btk_combo_box_list_auto_scroll (BtkComboBox *combo_box,
				gint         x, 
				gint         y)
{
  BtkWidget *tree_view = combo_box->priv->tree_view;
  BtkAdjustment *adj;
  gdouble value;

  adj = btk_scrolled_window_get_hadjustment (BTK_SCROLLED_WINDOW (combo_box->priv->scrolled_window));
  if (adj && adj->upper - adj->lower > adj->page_size)
    {
      if (x <= tree_view->allocation.x && 
	  adj->lower < adj->value)
	{
	  value = adj->value - (tree_view->allocation.x - x + 1);
	  btk_adjustment_set_value (adj, CLAMP (value, adj->lower, adj->upper - adj->page_size));
	}
      else if (x >= tree_view->allocation.x + tree_view->allocation.width &&
	       adj->upper - adj->page_size > adj->value)
	{
	  value = adj->value + (x - tree_view->allocation.x - tree_view->allocation.width + 1);
	  btk_adjustment_set_value (adj, CLAMP (value, 0.0, adj->upper - adj->page_size));
	}
    }

  adj = btk_scrolled_window_get_vadjustment (BTK_SCROLLED_WINDOW (combo_box->priv->scrolled_window));
  if (adj && adj->upper - adj->lower > adj->page_size)
    {
      if (y <= tree_view->allocation.y && 
	  adj->lower < adj->value)
	{
	  value = adj->value - (tree_view->allocation.y - y + 1);
	  btk_adjustment_set_value (adj, CLAMP (value, adj->lower, adj->upper - adj->page_size));
	}
      else if (y >= tree_view->allocation.height &&
	       adj->upper - adj->page_size > adj->value)
	{
	  value = adj->value + (y - tree_view->allocation.height + 1);
	  btk_adjustment_set_value (adj, CLAMP (value, 0.0, adj->upper - adj->page_size));
	}
    }
}

static gboolean
btk_combo_box_list_scroll_timeout (BtkComboBox *combo_box)
{
  BtkComboBoxPrivate *priv = combo_box->priv;
  gint x, y;

  if (priv->auto_scroll)
    {
      bdk_window_get_pointer (priv->tree_view->window, &x, &y, NULL);
      btk_combo_box_list_auto_scroll (combo_box, x, y);
    }

  return TRUE;
}

static gboolean 
btk_combo_box_list_enter_notify (BtkWidget        *widget,
				 BdkEventCrossing *event,
				 gpointer          data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (data);

  combo_box->priv->auto_scroll = TRUE;

  return TRUE;
}

static gboolean
btk_combo_box_list_select_func (BtkTreeSelection *selection,
				BtkTreeModel     *model,
				BtkTreePath      *path,
				gboolean          path_currently_selected,
				gpointer          data)
{
  GList *list;
  gboolean sensitive = FALSE;

  for (list = selection->tree_view->priv->columns; list && !sensitive; list = list->next)
    {
      GList *cells, *cell;
      gboolean cell_sensitive, cell_visible;
      BtkTreeIter iter;
      BtkTreeViewColumn *column = BTK_TREE_VIEW_COLUMN (list->data);

      if (!column->visible)
	continue;

      btk_tree_model_get_iter (model, &iter, path);
      btk_tree_view_column_cell_set_cell_data (column, model, &iter,
					       FALSE, FALSE);

      cell = cells = btk_cell_layout_get_cells (BTK_CELL_LAYOUT (column));
      while (cell)
        {
	  g_object_get (cell->data,
			"sensitive", &cell_sensitive,
			"visible", &cell_visible,
			NULL);

	  if (cell_visible && cell_sensitive)
	    break;

	  cell = cell->next;
	}
      g_list_free (cells);

      sensitive = cell_sensitive;
    }

  return sensitive;
}

static void
btk_combo_box_list_row_changed (BtkTreeModel *model,
                                BtkTreePath  *path,
                                BtkTreeIter  *iter,
                                gpointer      data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (data);
  BtkComboBoxPrivate *priv = combo_box->priv;
  gint width;

  width = btk_combo_box_calc_requested_width (combo_box, path);

  if (width > priv->width)
    {
      if (priv->cell_view) 
	{
	  btk_widget_set_size_request (priv->cell_view, width, -1);
	  btk_widget_queue_resize (priv->cell_view);
	}
      priv->width = width;
    }
}

/*
 * BtkCellLayout implementation
 */

static void
pack_start_recurse (BtkWidget       *menu,
		    BtkCellRenderer *cell,
		    gboolean         expand)
{
  GList *i, *list;
  BtkWidget *submenu;    
  
  list = btk_container_get_children (BTK_CONTAINER (menu));
  for (i = list; i; i = i->next)
    {
      if (BTK_IS_CELL_LAYOUT (BTK_BIN (i->data)->child))
	btk_cell_layout_pack_start (BTK_CELL_LAYOUT (BTK_BIN (i->data)->child), 
				    cell, expand);

      submenu = btk_menu_item_get_submenu (BTK_MENU_ITEM (i->data));
      if (submenu != NULL)
	pack_start_recurse (submenu, cell, expand);
    }

  g_list_free (list);
}

static void
btk_combo_box_cell_layout_pack_start (BtkCellLayout   *layout,
                                      BtkCellRenderer *cell,
                                      gboolean         expand)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (layout);
  ComboCellInfo *info;
  BtkComboBoxPrivate *priv;

  priv = combo_box->priv;

  g_object_ref_sink (cell);

  info = g_slice_new0 (ComboCellInfo);
  info->cell = cell;
  info->expand = expand;
  info->pack = BTK_PACK_START;

  priv->cells = g_slist_append (priv->cells, info);

  if (priv->cell_view)
    btk_cell_layout_pack_start (BTK_CELL_LAYOUT (priv->cell_view),
                                cell, expand);

  if (priv->column)
    btk_tree_view_column_pack_start (priv->column, cell, expand);

  if (BTK_IS_MENU (priv->popup_widget))
    pack_start_recurse (priv->popup_widget, cell, expand);
}

static void
pack_end_recurse (BtkWidget       *menu,
		  BtkCellRenderer *cell,
		  gboolean         expand)
{
  GList *i, *list;
  BtkWidget *submenu;    
  
  list = btk_container_get_children (BTK_CONTAINER (menu));
  for (i = list; i; i = i->next)
    {
      if (BTK_IS_CELL_LAYOUT (BTK_BIN (i->data)->child))
	btk_cell_layout_pack_end (BTK_CELL_LAYOUT (BTK_BIN (i->data)->child), 
				  cell, expand);

      submenu = btk_menu_item_get_submenu (BTK_MENU_ITEM (i->data));
      if (submenu != NULL)
	pack_end_recurse (submenu, cell, expand);
    }

  g_list_free (list);
}

static void
btk_combo_box_cell_layout_pack_end (BtkCellLayout   *layout,
                                    BtkCellRenderer *cell,
                                    gboolean         expand)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (layout);
  ComboCellInfo *info;
  BtkComboBoxPrivate *priv;

  priv = combo_box->priv;

  g_object_ref_sink (cell);

  info = g_slice_new0 (ComboCellInfo);
  info->cell = cell;
  info->expand = expand;
  info->pack = BTK_PACK_END;

  priv->cells = g_slist_append (priv->cells, info);

  if (priv->cell_view)
    btk_cell_layout_pack_end (BTK_CELL_LAYOUT (priv->cell_view),
                              cell, expand);

  if (priv->column)
    btk_tree_view_column_pack_end (priv->column, cell, expand);

  if (BTK_IS_MENU (priv->popup_widget))
    pack_end_recurse (priv->popup_widget, cell, expand);
}

static GList *
btk_combo_box_cell_layout_get_cells (BtkCellLayout *layout)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (layout);
  GSList *list;
  GList *retval = NULL;

  for (list = combo_box->priv->cells; list; list = list->next)
    {
      ComboCellInfo *info = (ComboCellInfo *)list->data;

      retval = g_list_prepend (retval, info->cell);
    }

  return g_list_reverse (retval);
}

static void
clear_recurse (BtkWidget *menu)
{
  GList *i, *list;
  BtkWidget *submenu;    
  
  list = btk_container_get_children (BTK_CONTAINER (menu));
  for (i = list; i; i = i->next)
    {
      if (BTK_IS_CELL_LAYOUT (BTK_BIN (i->data)->child))
	btk_cell_layout_clear (BTK_CELL_LAYOUT (BTK_BIN (i->data)->child)); 

      submenu = btk_menu_item_get_submenu (BTK_MENU_ITEM (i->data));
      if (submenu != NULL)
	clear_recurse (submenu);
    }

  g_list_free (list);
}

static void
btk_combo_box_cell_layout_clear (BtkCellLayout *layout)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (layout);
  BtkComboBoxPrivate *priv = combo_box->priv;
  GSList *i;
  
  if (priv->cell_view)
    btk_cell_layout_clear (BTK_CELL_LAYOUT (priv->cell_view));

  if (priv->column)
    btk_tree_view_column_clear (priv->column);

  for (i = priv->cells; i; i = i->next)
    {
      ComboCellInfo *info = (ComboCellInfo *)i->data;

      btk_combo_box_cell_layout_clear_attributes (layout, info->cell);
      g_object_unref (info->cell);
      g_slice_free (ComboCellInfo, info);
      i->data = NULL;
    }
  g_slist_free (priv->cells);
  priv->cells = NULL;

  if (BTK_IS_MENU (priv->popup_widget))
    clear_recurse (priv->popup_widget);
}

static void
add_attribute_recurse (BtkWidget       *menu,
		       BtkCellRenderer *cell,
		       const gchar     *attribute,
		       gint             column)
{
  GList *i, *list;
  BtkWidget *submenu;    
  
  list = btk_container_get_children (BTK_CONTAINER (menu));
  for (i = list; i; i = i->next)
    {
      if (BTK_IS_CELL_LAYOUT (BTK_BIN (i->data)->child))
	btk_cell_layout_add_attribute (BTK_CELL_LAYOUT (BTK_BIN (i->data)->child),
				       cell, attribute, column); 

      submenu = btk_menu_item_get_submenu (BTK_MENU_ITEM (i->data));
      if (submenu != NULL)
	add_attribute_recurse (submenu, cell, attribute, column);
    }

  g_list_free (list);
}
		       
static void
btk_combo_box_cell_layout_add_attribute (BtkCellLayout   *layout,
                                         BtkCellRenderer *cell,
                                         const gchar     *attribute,
                                         gint             column)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (layout);
  ComboCellInfo *info;

  info = btk_combo_box_get_cell_info (combo_box, cell);
  g_return_if_fail (info != NULL);

  info->attributes = g_slist_prepend (info->attributes,
                                      GINT_TO_POINTER (column));
  info->attributes = g_slist_prepend (info->attributes,
                                      g_strdup (attribute));

  if (combo_box->priv->cell_view)
    btk_cell_layout_add_attribute (BTK_CELL_LAYOUT (combo_box->priv->cell_view),
                                   cell, attribute, column);

  if (combo_box->priv->column)
    btk_cell_layout_add_attribute (BTK_CELL_LAYOUT (combo_box->priv->column),
                                   cell, attribute, column);

  if (BTK_IS_MENU (combo_box->priv->popup_widget))
    add_attribute_recurse (combo_box->priv->popup_widget, cell, attribute, column);
  btk_widget_queue_resize (BTK_WIDGET (combo_box));
}

static void
combo_cell_data_func (BtkCellLayout   *cell_layout,
		      BtkCellRenderer *cell,
		      BtkTreeModel    *tree_model,
		      BtkTreeIter     *iter,
		      gpointer         data)
{
  ComboCellInfo *info = (ComboCellInfo *)data;
  BtkWidget *parent = NULL;
  
  if (!info->func)
    return;

  info->func (cell_layout, cell, tree_model, iter, info->func_data);

  if (BTK_IS_WIDGET (cell_layout))
    parent = btk_widget_get_parent (BTK_WIDGET (cell_layout));
  
  if (BTK_IS_MENU_ITEM (parent) && 
      btk_menu_item_get_submenu (BTK_MENU_ITEM (parent)))
    g_object_set (cell, "sensitive", TRUE, NULL);
}


static void 
set_cell_data_func_recurse (BtkWidget       *menu,
			    BtkCellRenderer *cell,
			    ComboCellInfo   *info)
{
  GList *i, *list;
  BtkWidget *submenu;    
  BtkWidget *cell_view;
  
 list = btk_container_get_children (BTK_CONTAINER (menu));
  for (i = list; i; i = i->next)
    {
      cell_view = BTK_BIN (i->data)->child;
      if (BTK_IS_CELL_LAYOUT (cell_view))
	{
	  /* Override sensitivity for inner nodes; we don't
	   * want menuitems with submenus to appear insensitive 
	   */ 
	  btk_cell_layout_set_cell_data_func (BTK_CELL_LAYOUT (cell_view), 
					      cell, 
					      combo_cell_data_func, 
					      info, NULL); 
	  submenu = btk_menu_item_get_submenu (BTK_MENU_ITEM (i->data));
	  if (submenu != NULL)
	    set_cell_data_func_recurse (submenu, cell, info);
	}
    }

  g_list_free (list);
}

static void
btk_combo_box_cell_layout_set_cell_data_func (BtkCellLayout         *layout,
                                              BtkCellRenderer       *cell,
                                              BtkCellLayoutDataFunc  func,
                                              gpointer               func_data,
                                              GDestroyNotify         destroy)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (layout);
  BtkComboBoxPrivate *priv = combo_box->priv;
  ComboCellInfo *info;

  info = btk_combo_box_get_cell_info (combo_box, cell);
  g_return_if_fail (info != NULL);
  
  if (info->destroy)
    {
      GDestroyNotify d = info->destroy;

      info->destroy = NULL;
      d (info->func_data);
    }

  info->func = func;
  info->func_data = func_data;
  info->destroy = destroy;

  if (priv->cell_view)
    btk_cell_layout_set_cell_data_func (BTK_CELL_LAYOUT (priv->cell_view), cell, func, func_data, NULL);

  if (priv->column)
    btk_cell_layout_set_cell_data_func (BTK_CELL_LAYOUT (priv->column), cell, func, func_data, NULL);

  if (BTK_IS_MENU (priv->popup_widget))
    set_cell_data_func_recurse (priv->popup_widget, cell, info);

  btk_widget_queue_resize (BTK_WIDGET (combo_box));
}

static void 
clear_attributes_recurse (BtkWidget       *menu,
			  BtkCellRenderer *cell)
{
  GList *i, *list;
  BtkWidget *submenu;    
  
  list = btk_container_get_children (BTK_CONTAINER (menu));
  for (i = list; i; i = i->next)
    {
      if (BTK_IS_CELL_LAYOUT (BTK_BIN (i->data)->child))
	btk_cell_layout_clear_attributes (BTK_CELL_LAYOUT (BTK_BIN (i->data)->child),
					    cell); 
      
      submenu = btk_menu_item_get_submenu (BTK_MENU_ITEM (i->data));
      if (submenu != NULL)
	clear_attributes_recurse (submenu, cell);
    }

  g_list_free (list);
}

static void
btk_combo_box_cell_layout_clear_attributes (BtkCellLayout   *layout,
                                            BtkCellRenderer *cell)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (layout);
  BtkComboBoxPrivate *priv;
  ComboCellInfo *info;
  GSList *list;

  priv = combo_box->priv;

  info = btk_combo_box_get_cell_info (combo_box, cell);
  g_return_if_fail (info != NULL);

  list = info->attributes;
  while (list && list->next)
    {
      g_free (list->data);
      list = list->next->next;
    }
  g_slist_free (info->attributes);
  info->attributes = NULL;

  if (priv->cell_view)
    btk_cell_layout_clear_attributes (BTK_CELL_LAYOUT (priv->cell_view), cell);

  if (priv->column)
    btk_cell_layout_clear_attributes (BTK_CELL_LAYOUT (priv->column), cell);

  if (BTK_IS_MENU (priv->popup_widget))
    clear_attributes_recurse (priv->popup_widget, cell);

  btk_widget_queue_resize (BTK_WIDGET (combo_box));
}

static void 
reorder_recurse (BtkWidget             *menu,
		 BtkCellRenderer       *cell,
		 gint                   position)
{
  GList *i, *list;
  BtkWidget *submenu;    
  
  list = btk_container_get_children (BTK_CONTAINER (menu));
  for (i = list; i; i = i->next)
    {
      if (BTK_IS_CELL_LAYOUT (BTK_BIN (i->data)->child))
	btk_cell_layout_reorder (BTK_CELL_LAYOUT (BTK_BIN (i->data)->child),
				 cell, position); 
      
      submenu = btk_menu_item_get_submenu (BTK_MENU_ITEM (i->data));
      if (submenu != NULL)
	reorder_recurse (submenu, cell, position);
    }

  g_list_free (list);
}

static void
btk_combo_box_cell_layout_reorder (BtkCellLayout   *layout,
                                   BtkCellRenderer *cell,
                                   gint             position)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (layout);
  BtkComboBoxPrivate *priv;
  ComboCellInfo *info;
  GSList *link;

  priv = combo_box->priv;

  info = btk_combo_box_get_cell_info (combo_box, cell);

  g_return_if_fail (info != NULL);
  g_return_if_fail (position >= 0);

  link = g_slist_find (priv->cells, info);

  g_return_if_fail (link != NULL);

  priv->cells = g_slist_delete_link (priv->cells, link);
  priv->cells = g_slist_insert (priv->cells, info, position);

  if (priv->cell_view)
    btk_cell_layout_reorder (BTK_CELL_LAYOUT (priv->cell_view),
                             cell, position);

  if (priv->column)
    btk_cell_layout_reorder (BTK_CELL_LAYOUT (priv->column),
                             cell, position);

  if (BTK_IS_MENU (priv->popup_widget))
    reorder_recurse (priv->popup_widget, cell, position);

  btk_widget_queue_draw (BTK_WIDGET (combo_box));
}

/*
 * public API
 */

/**
 * btk_combo_box_new:
 *
 * Creates a new empty #BtkComboBox.
 *
 * Return value: A new #BtkComboBox.
 *
 * Since: 2.4
 */
BtkWidget *
btk_combo_box_new (void)
{
  return g_object_new (BTK_TYPE_COMBO_BOX, NULL);
}

/**
 * btk_combo_box_new_with_entry:
 *
 * Creates a new empty #BtkComboBox with an entry.
 *
 * Return value: A new #BtkComboBox.
 *
 * Since: 2.24
 */
BtkWidget *
btk_combo_box_new_with_entry (void)
{
  return g_object_new (BTK_TYPE_COMBO_BOX, "has-entry", TRUE, NULL);
}

/**
 * btk_combo_box_new_with_model:
 * @model: A #BtkTreeModel.
 *
 * Creates a new #BtkComboBox with the model initialized to @model.
 *
 * Return value: A new #BtkComboBox.
 *
 * Since: 2.4
 */
BtkWidget *
btk_combo_box_new_with_model (BtkTreeModel *model)
{
  BtkComboBox *combo_box;

  g_return_val_if_fail (BTK_IS_TREE_MODEL (model), NULL);

  combo_box = g_object_new (BTK_TYPE_COMBO_BOX, "model", model, NULL);

  return BTK_WIDGET (combo_box);
}

/**
 * btk_combo_box_new_with_model_and_entry:
 *
 * Creates a new empty #BtkComboBox with an entry
 * and with the model initialized to @model.
 *
 * Return value: A new #BtkComboBox
 *
 * Since: 2.24
 */
BtkWidget *
btk_combo_box_new_with_model_and_entry (BtkTreeModel *model)
{
  return g_object_new (BTK_TYPE_COMBO_BOX,
                       "has-entry", TRUE,
                       "model", model,
                       NULL);
}

/**
 * btk_combo_box_get_wrap_width:
 * @combo_box: A #BtkComboBox
 *
 * Returns the wrap width which is used to determine the number of columns 
 * for the popup menu. If the wrap width is larger than 1, the combo box 
 * is in table mode.
 *
 * Returns: the wrap width.
 *
 * Since: 2.6
 */
gint
btk_combo_box_get_wrap_width (BtkComboBox *combo_box)
{
  g_return_val_if_fail (BTK_IS_COMBO_BOX (combo_box), -1);

  return combo_box->priv->wrap_width;
}

/**
 * btk_combo_box_set_wrap_width:
 * @combo_box: A #BtkComboBox
 * @width: Preferred number of columns
 *
 * Sets the wrap width of @combo_box to be @width. The wrap width is basically
 * the preferred number of columns when you want the popup to be layed out
 * in a table.
 *
 * Since: 2.4
 */
void
btk_combo_box_set_wrap_width (BtkComboBox *combo_box,
                              gint         width)
{
  BtkComboBoxPrivate *priv;

  g_return_if_fail (BTK_IS_COMBO_BOX (combo_box));
  g_return_if_fail (width >= 0);

  priv = combo_box->priv;

  if (width != priv->wrap_width)
    {
      priv->wrap_width = width;

      btk_combo_box_check_appearance (combo_box);
      btk_combo_box_relayout (combo_box);
      
      g_object_notify (G_OBJECT (combo_box), "wrap-width");
    }
}

/**
 * btk_combo_box_get_row_span_column:
 * @combo_box: A #BtkComboBox
 *
 * Returns the column with row span information for @combo_box.
 *
 * Returns: the row span column.
 *
 * Since: 2.6
 */
gint
btk_combo_box_get_row_span_column (BtkComboBox *combo_box)
{
  g_return_val_if_fail (BTK_IS_COMBO_BOX (combo_box), -1);

  return combo_box->priv->row_column;
}

/**
 * btk_combo_box_set_row_span_column:
 * @combo_box: A #BtkComboBox.
 * @row_span: A column in the model passed during construction.
 *
 * Sets the column with row span information for @combo_box to be @row_span.
 * The row span column contains integers which indicate how many rows
 * an item should span.
 *
 * Since: 2.4
 */
void
btk_combo_box_set_row_span_column (BtkComboBox *combo_box,
                                   gint         row_span)
{
  BtkComboBoxPrivate *priv;
  gint col;

  g_return_if_fail (BTK_IS_COMBO_BOX (combo_box));

  priv = combo_box->priv;

  col = btk_tree_model_get_n_columns (priv->model);
  g_return_if_fail (row_span >= -1 && row_span < col);

  if (row_span != priv->row_column)
    {
      priv->row_column = row_span;
      
      btk_combo_box_relayout (combo_box);
 
      g_object_notify (G_OBJECT (combo_box), "row-span-column");
    }
}

/**
 * btk_combo_box_get_column_span_column:
 * @combo_box: A #BtkComboBox
 *
 * Returns the column with column span information for @combo_box.
 *
 * Returns: the column span column.
 *
 * Since: 2.6
 */
gint
btk_combo_box_get_column_span_column (BtkComboBox *combo_box)
{
  g_return_val_if_fail (BTK_IS_COMBO_BOX (combo_box), -1);

  return combo_box->priv->col_column;
}

/**
 * btk_combo_box_set_column_span_column:
 * @combo_box: A #BtkComboBox
 * @column_span: A column in the model passed during construction
 *
 * Sets the column with column span information for @combo_box to be
 * @column_span. The column span column contains integers which indicate
 * how many columns an item should span.
 *
 * Since: 2.4
 */
void
btk_combo_box_set_column_span_column (BtkComboBox *combo_box,
                                      gint         column_span)
{
  BtkComboBoxPrivate *priv;
  gint col;

  g_return_if_fail (BTK_IS_COMBO_BOX (combo_box));

  priv = combo_box->priv;

  col = btk_tree_model_get_n_columns (priv->model);
  g_return_if_fail (column_span >= -1 && column_span < col);

  if (column_span != priv->col_column)
    {
      priv->col_column = column_span;
      
      btk_combo_box_relayout (combo_box);

      g_object_notify (G_OBJECT (combo_box), "column-span-column");
    }
}

/**
 * btk_combo_box_get_active:
 * @combo_box: A #BtkComboBox
 *
 * Returns the index of the currently active item, or -1 if there's no
 * active item. If the model is a non-flat treemodel, and the active item 
 * is not an immediate child of the root of the tree, this function returns 
 * <literal>btk_tree_path_get_indices (path)[0]</literal>, where 
 * <literal>path</literal> is the #BtkTreePath of the active item.
 *
 * Return value: An integer which is the index of the currently active item, 
 *     or -1 if there's no active item.
 *
 * Since: 2.4
 */
gint
btk_combo_box_get_active (BtkComboBox *combo_box)
{
  BtkComboBoxPrivate *priv;
  gint result;

  g_return_val_if_fail (BTK_IS_COMBO_BOX (combo_box), 0);

  priv = combo_box->priv;

  if (btk_tree_row_reference_valid (priv->active_row))
    {
      BtkTreePath *path;

      path = btk_tree_row_reference_get_path (priv->active_row);      
      result = btk_tree_path_get_indices (path)[0];
      btk_tree_path_free (path);
    }
  else
    result = -1;

  return result;
}

/**
 * btk_combo_box_set_active:
 * @combo_box: A #BtkComboBox
 * @index_: An index in the model passed during construction, or -1 to have
 * no active item
 *
 * Sets the active item of @combo_box to be the item at @index.
 *
 * Since: 2.4
 */
void
btk_combo_box_set_active (BtkComboBox *combo_box,
                          gint         index_)
{
  BtkTreePath *path = NULL;
  g_return_if_fail (BTK_IS_COMBO_BOX (combo_box));
  g_return_if_fail (index_ >= -1);

  if (combo_box->priv->model == NULL)
    {
      /* Save index, in case the model is set after the index */
      combo_box->priv->active = index_;
      if (index_ != -1)
        return;
    }

  if (index_ != -1)
    path = btk_tree_path_new_from_indices (index_, -1);
   
  btk_combo_box_set_active_internal (combo_box, path);

  if (path)
    btk_tree_path_free (path);
}

static void
btk_combo_box_set_active_internal (BtkComboBox *combo_box,
				   BtkTreePath *path)
{
  BtkComboBoxPrivate *priv = combo_box->priv;
  BtkTreePath *active_path;
  gint path_cmp;

  /* Remember whether the initially active row is valid. */
  gboolean is_valid_row_reference = btk_tree_row_reference_valid (priv->active_row);

  if (path && is_valid_row_reference)
    {
      active_path = btk_tree_row_reference_get_path (priv->active_row);
      path_cmp = btk_tree_path_compare (path, active_path);
      btk_tree_path_free (active_path);
      if (path_cmp == 0)
	return;
    }

  if (priv->active_row)
    {
      btk_tree_row_reference_free (priv->active_row);
      priv->active_row = NULL;
    }
  
  if (!path)
    {
      if (priv->tree_view)
        btk_tree_selection_unselect_all (btk_tree_view_get_selection (BTK_TREE_VIEW (priv->tree_view)));
      else
        {
          BtkMenu *menu = BTK_MENU (priv->popup_widget);

          if (BTK_IS_MENU (menu))
            btk_menu_set_active (menu, -1);
        }

      if (priv->cell_view)
        btk_cell_view_set_displayed_row (BTK_CELL_VIEW (priv->cell_view), NULL);

      /*
       *  Do not emit a "changed" signal when an already invalid selection was
       *  now set to invalid.
       */
      if (!is_valid_row_reference)
        return;
    }
  else
    {
      priv->active_row = 
	btk_tree_row_reference_new (priv->model, path);

      if (priv->tree_view)
	{
	  btk_tree_view_set_cursor (BTK_TREE_VIEW (priv->tree_view), 
				    path, NULL, FALSE);
	}
      else if (BTK_IS_MENU (priv->popup_widget))
        {
	  /* FIXME handle nested menus better */
	  btk_menu_set_active (BTK_MENU (priv->popup_widget), 
			       btk_tree_path_get_indices (path)[0]);
        }

      if (priv->cell_view)
	btk_cell_view_set_displayed_row (BTK_CELL_VIEW (priv->cell_view), 
					 path);
    }

  g_signal_emit (combo_box, combo_box_signals[CHANGED], 0);
  g_object_notify (G_OBJECT (combo_box), "active");
}


/**
 * btk_combo_box_get_active_iter:
 * @combo_box: A #BtkComboBox
 * @iter: (out): A #BtkTreeIter
 *
 * Sets @iter to point to the currently active item, if any item is active.
 * Otherwise, @iter is left unchanged.
 *
 * Returns: %TRUE if @iter was set, %FALSE otherwise
 *
 * Since: 2.4
 */
gboolean
btk_combo_box_get_active_iter (BtkComboBox     *combo_box,
                               BtkTreeIter     *iter)
{
  BtkTreePath *path;
  gboolean result;

  g_return_val_if_fail (BTK_IS_COMBO_BOX (combo_box), FALSE);

  if (!btk_tree_row_reference_valid (combo_box->priv->active_row))
    return FALSE;

  path = btk_tree_row_reference_get_path (combo_box->priv->active_row);
  result = btk_tree_model_get_iter (combo_box->priv->model, iter, path);
  btk_tree_path_free (path);

  return result;
}

/**
 * btk_combo_box_set_active_iter:
 * @combo_box: A #BtkComboBox
 * @iter: (allow-none): The #BtkTreeIter, or %NULL
 * 
 * Sets the current active item to be the one referenced by @iter, or
 * unsets the active item if @iter is %NULL.
 * 
 * Since: 2.4
 */
void
btk_combo_box_set_active_iter (BtkComboBox     *combo_box,
                               BtkTreeIter     *iter)
{
  BtkTreePath *path = NULL;

  g_return_if_fail (BTK_IS_COMBO_BOX (combo_box));

  if (iter)
    path = btk_tree_model_get_path (btk_combo_box_get_model (combo_box), iter);

  btk_combo_box_set_active_internal (combo_box, path);
  btk_tree_path_free (path);
}

/**
 * btk_combo_box_set_model:
 * @combo_box: A #BtkComboBox
 * @model: (allow-none): A #BtkTreeModel
 *
 * Sets the model used by @combo_box to be @model. Will unset a previously set
 * model (if applicable). If model is %NULL, then it will unset the model.
 *
 * Note that this function does not clear the cell renderers, you have to 
 * call btk_cell_layout_clear() yourself if you need to set up different 
 * cell renderers for the new model.
 *
 * Since: 2.4
 */
void
btk_combo_box_set_model (BtkComboBox  *combo_box,
                         BtkTreeModel *model)
{
  g_return_if_fail (BTK_IS_COMBO_BOX (combo_box));
  g_return_if_fail (model == NULL || BTK_IS_TREE_MODEL (model));

  if (model == combo_box->priv->model)
    return;
  
  btk_combo_box_unset_model (combo_box);

  if (model == NULL)
    goto out;

  combo_box->priv->model = model;
  g_object_ref (combo_box->priv->model);

  combo_box->priv->inserted_id =
    g_signal_connect (combo_box->priv->model, "row-inserted",
		      G_CALLBACK (btk_combo_box_model_row_inserted),
		      combo_box);
  combo_box->priv->deleted_id =
    g_signal_connect (combo_box->priv->model, "row-deleted",
		      G_CALLBACK (btk_combo_box_model_row_deleted),
		      combo_box);
  combo_box->priv->reordered_id =
    g_signal_connect (combo_box->priv->model, "rows-reordered",
		      G_CALLBACK (btk_combo_box_model_rows_reordered),
		      combo_box);
  combo_box->priv->changed_id =
    g_signal_connect (combo_box->priv->model, "row-changed",
		      G_CALLBACK (btk_combo_box_model_row_changed),
		      combo_box);
      
  if (combo_box->priv->tree_view)
    {
      /* list mode */
      btk_tree_view_set_model (BTK_TREE_VIEW (combo_box->priv->tree_view),
                               combo_box->priv->model);
      btk_combo_box_list_popup_resize (combo_box);
    }
  else
    {
      /* menu mode */
      if (combo_box->priv->popup_widget)
	btk_combo_box_menu_fill (combo_box);

    }

  if (combo_box->priv->cell_view)
    btk_cell_view_set_model (BTK_CELL_VIEW (combo_box->priv->cell_view),
                             combo_box->priv->model);

  if (combo_box->priv->active != -1)
    {
      /* If an index was set in advance, apply it now */
      btk_combo_box_set_active (combo_box, combo_box->priv->active);
      combo_box->priv->active = -1;
    }

out:
  btk_combo_box_update_sensitivity (combo_box);

  g_object_notify (G_OBJECT (combo_box), "model");
}

/**
 * btk_combo_box_get_model:
 * @combo_box: A #BtkComboBox
 *
 * Returns the #BtkTreeModel which is acting as data source for @combo_box.
 *
 * Return value: (transfer none): A #BtkTreeModel which was passed
 *     during construction.
 *
 * Since: 2.4
 */
BtkTreeModel *
btk_combo_box_get_model (BtkComboBox *combo_box)
{
  g_return_val_if_fail (BTK_IS_COMBO_BOX (combo_box), NULL);

  return combo_box->priv->model;
}


/* convenience API for simple text combos */

/**
 * btk_combo_box_new_text:
 *
 * Convenience function which constructs a new text combo box, which is a
 * #BtkComboBox just displaying strings. If you use this function to create
 * a text combo box, you should only manipulate its data source with the
 * following convenience functions: btk_combo_box_append_text(),
 * btk_combo_box_insert_text(), btk_combo_box_prepend_text() and
 * btk_combo_box_remove_text().
 *
 * Return value: (transfer none): A new text combo box.
 *
 * Since: 2.4
 *
 * Deprecated: 2.24: Use #BtkComboBoxText
 */
BtkWidget *
btk_combo_box_new_text (void)
{
  BtkWidget *combo_box;
  BtkCellRenderer *cell;
  BtkListStore *store;

  store = btk_list_store_new (1, G_TYPE_STRING);
  combo_box = btk_combo_box_new_with_model (BTK_TREE_MODEL (store));
  g_object_unref (store);

  cell = btk_cell_renderer_text_new ();
  btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combo_box), cell, TRUE);
  btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (combo_box), cell,
                                  "text", 0,
                                  NULL);

  return combo_box;
}

/**
 * btk_combo_box_append_text:
 * @combo_box: A #BtkComboBox constructed using btk_combo_box_new_text()
 * @text: A string
 *
 * Appends @string to the list of strings stored in @combo_box. Note that
 * you can only use this function with combo boxes constructed with
 * btk_combo_box_new_text().
 *
 * Since: 2.4
 *
 * Deprecated: 2.24: Use #BtkComboBoxText
 */
void
btk_combo_box_append_text (BtkComboBox *combo_box,
                           const gchar *text)
{
  BtkTreeIter iter;
  BtkListStore *store;

  g_return_if_fail (BTK_IS_COMBO_BOX (combo_box));
  g_return_if_fail (BTK_IS_LIST_STORE (combo_box->priv->model));
  g_return_if_fail (btk_tree_model_get_column_type (combo_box->priv->model, 0)
		    == G_TYPE_STRING);
  g_return_if_fail (text != NULL);

  store = BTK_LIST_STORE (combo_box->priv->model);

  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, text, -1);
}

/**
 * btk_combo_box_insert_text:
 * @combo_box: A #BtkComboBox constructed using btk_combo_box_new_text()
 * @position: An index to insert @text
 * @text: A string
 *
 * Inserts @string at @position in the list of strings stored in @combo_box.
 * Note that you can only use this function with combo boxes constructed
 * with btk_combo_box_new_text().
 *
 * Since: 2.4
 *
 * Deprecated: 2.24: Use #BtkComboBoxText
 */
void
btk_combo_box_insert_text (BtkComboBox *combo_box,
                           gint         position,
                           const gchar *text)
{
  BtkTreeIter iter;
  BtkListStore *store;

  g_return_if_fail (BTK_IS_COMBO_BOX (combo_box));
  g_return_if_fail (BTK_IS_LIST_STORE (combo_box->priv->model));
  g_return_if_fail (position >= 0);
  g_return_if_fail (btk_tree_model_get_column_type (combo_box->priv->model, 0)
		    == G_TYPE_STRING);
  g_return_if_fail (text != NULL);

  store = BTK_LIST_STORE (combo_box->priv->model);

  btk_list_store_insert (store, &iter, position);
  btk_list_store_set (store, &iter, 0, text, -1);
}

/**
 * btk_combo_box_prepend_text:
 * @combo_box: A #BtkComboBox constructed with btk_combo_box_new_text()
 * @text: A string
 *
 * Prepends @string to the list of strings stored in @combo_box. Note that
 * you can only use this function with combo boxes constructed with
 * btk_combo_box_new_text().
 *
 * Since: 2.4
 *
 * Deprecated: 2.24: Use #BtkComboBoxText
 */
void
btk_combo_box_prepend_text (BtkComboBox *combo_box,
                            const gchar *text)
{
  BtkTreeIter iter;
  BtkListStore *store;

  g_return_if_fail (BTK_IS_COMBO_BOX (combo_box));
  g_return_if_fail (BTK_IS_LIST_STORE (combo_box->priv->model));
  g_return_if_fail (btk_tree_model_get_column_type (combo_box->priv->model, 0)
		    == G_TYPE_STRING);
  g_return_if_fail (text != NULL);

  store = BTK_LIST_STORE (combo_box->priv->model);

  btk_list_store_prepend (store, &iter);
  btk_list_store_set (store, &iter, 0, text, -1);
}

/**
 * btk_combo_box_remove_text:
 * @combo_box: A #BtkComboBox constructed with btk_combo_box_new_text()
 * @position: Index of the item to remove
 *
 * Removes the string at @position from @combo_box. Note that you can only use
 * this function with combo boxes constructed with btk_combo_box_new_text().
 *
 * Since: 2.4
 *
 * Deprecated: 2.24: Use #BtkComboBoxText
 */
void
btk_combo_box_remove_text (BtkComboBox *combo_box,
                           gint         position)
{
  BtkTreeIter iter;
  BtkListStore *store;

  g_return_if_fail (BTK_IS_COMBO_BOX (combo_box));
  g_return_if_fail (BTK_IS_LIST_STORE (combo_box->priv->model));
  g_return_if_fail (btk_tree_model_get_column_type (combo_box->priv->model, 0)
		    == G_TYPE_STRING);
  g_return_if_fail (position >= 0);

  store = BTK_LIST_STORE (combo_box->priv->model);

  if (btk_tree_model_iter_nth_child (combo_box->priv->model, &iter,
                                     NULL, position))
    btk_list_store_remove (store, &iter);
}

/**
 * btk_combo_box_get_active_text:
 * @combo_box: A #BtkComboBox constructed with btk_combo_box_new_text()
 *
 * Returns the currently active string in @combo_box or %NULL if none
 * is selected. Note that you can only use this function with combo
 * boxes constructed with btk_combo_box_new_text() and with
 * #BtkComboBoxEntry<!-- -->s.
 *
 * Returns: a newly allocated string containing the currently active text.
 *     Must be freed with g_free().
 *
 * Since: 2.6
 *
 * Deprecated: 2.24: If you used this with a #BtkComboBox constructed with 
 * btk_combo_box_new_text() then you should now use #BtkComboBoxText and 
 * btk_combo_box_text_get_active_text() instead. Or if you used this with a
 * #BtkComboBoxEntry then you should now use #BtkComboBox with 
 * #BtkComboBox:has-entry as %TRUE and use
 * btk_entry_get_text (BTK_ENTRY (btk_bin_get_child (BTK_BIN (combobox))).
 */
gchar *
btk_combo_box_get_active_text (BtkComboBox *combo_box)
{
  BtkComboBoxClass *class;

  g_return_val_if_fail (BTK_IS_COMBO_BOX (combo_box), NULL);

  class = BTK_COMBO_BOX_GET_CLASS (combo_box);

  if (class->get_active_text)
    return class->get_active_text (combo_box);

  return NULL;
}

static gchar *
btk_combo_box_real_get_active_text (BtkComboBox *combo_box)
{
  BtkTreeIter iter;
  gchar *text = NULL;

  if (combo_box->priv->has_entry)
    {
      BtkBin *combo = BTK_BIN (combo_box);
      BtkWidget *child;

      child = btk_bin_get_child (combo);
      if (child)
	return g_strdup (btk_entry_get_text (BTK_ENTRY (child)));

      return NULL;
    }
  else
    {
      g_return_val_if_fail (BTK_IS_LIST_STORE (combo_box->priv->model), NULL);
      g_return_val_if_fail (btk_tree_model_get_column_type (combo_box->priv->model, 0)
			    == G_TYPE_STRING, NULL);

      if (btk_combo_box_get_active_iter (combo_box, &iter))
        btk_tree_model_get (combo_box->priv->model, &iter,
			    0, &text, -1);

      return text;
    }
}

static void
btk_combo_box_real_move_active (BtkComboBox   *combo_box,
                                BtkScrollType  scroll)
{
  BtkTreeIter iter;
  BtkTreeIter new_iter;
  gboolean    active_iter;
  gboolean    found;

  if (!combo_box->priv->model)
    {
      btk_widget_error_bell (BTK_WIDGET (combo_box));
      return;
    }

  active_iter = btk_combo_box_get_active_iter (combo_box, &iter);

  switch (scroll)
    {
    case BTK_SCROLL_STEP_BACKWARD:
    case BTK_SCROLL_STEP_UP:
    case BTK_SCROLL_STEP_LEFT:
      if (active_iter)
        {
	  found = tree_prev (combo_box, combo_box->priv->model,
			     &iter, &new_iter, FALSE);
	  break;
        }
      /* else fall through */

    case BTK_SCROLL_PAGE_FORWARD:
    case BTK_SCROLL_PAGE_DOWN:
    case BTK_SCROLL_PAGE_RIGHT:
    case BTK_SCROLL_END:
      found = tree_last (combo_box, combo_box->priv->model, &new_iter, FALSE);
      break;

    case BTK_SCROLL_STEP_FORWARD:
    case BTK_SCROLL_STEP_DOWN:
    case BTK_SCROLL_STEP_RIGHT:
      if (active_iter)
        {
	  found = tree_next (combo_box, combo_box->priv->model,
			     &iter, &new_iter, FALSE);
          break;
        }
      /* else fall through */

    case BTK_SCROLL_PAGE_BACKWARD:
    case BTK_SCROLL_PAGE_UP:
    case BTK_SCROLL_PAGE_LEFT:
    case BTK_SCROLL_START:
      found = tree_first (combo_box, combo_box->priv->model, &new_iter, FALSE);
      break;

    default:
      return;
    }

  if (found && active_iter)
    {
      BtkTreePath *old_path;
      BtkTreePath *new_path;

      old_path = btk_tree_model_get_path (combo_box->priv->model, &iter);
      new_path = btk_tree_model_get_path (combo_box->priv->model, &new_iter);

      if (btk_tree_path_compare (old_path, new_path) == 0)
        found = FALSE;

      btk_tree_path_free (old_path);
      btk_tree_path_free (new_path);
    }

  if (found)
    {
      btk_combo_box_set_active_iter (combo_box, &new_iter);
    }
  else
    {
      btk_widget_error_bell (BTK_WIDGET (combo_box));
    }
}

static gboolean
btk_combo_box_mnemonic_activate (BtkWidget *widget,
				 gboolean   group_cycling)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (widget);

  if (combo_box->priv->has_entry)
    {
      BtkWidget* child;

      child = btk_bin_get_child (BTK_BIN (combo_box));
      if (child)
	btk_widget_grab_focus (child);
    }
  else
    btk_widget_grab_focus (combo_box->priv->button);

  return TRUE;
}

static void
btk_combo_box_grab_focus (BtkWidget *widget)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (widget);

  if (combo_box->priv->has_entry)
    {
      BtkWidget *child;

      child = btk_bin_get_child (BTK_BIN (combo_box));
      if (child)
	btk_widget_grab_focus (child);
    }
  else
    btk_widget_grab_focus (combo_box->priv->button);
}

static void
btk_combo_box_destroy (BtkObject *object)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (object);

  if (combo_box->priv->popup_idle_id > 0)
    {
      g_source_remove (combo_box->priv->popup_idle_id);
      combo_box->priv->popup_idle_id = 0;
    }

  btk_combo_box_popdown (combo_box);

  if (combo_box->priv->row_separator_destroy)
    combo_box->priv->row_separator_destroy (combo_box->priv->row_separator_data);

  combo_box->priv->row_separator_func = NULL;
  combo_box->priv->row_separator_data = NULL;
  combo_box->priv->row_separator_destroy = NULL;

  BTK_OBJECT_CLASS (btk_combo_box_parent_class)->destroy (object);
  combo_box->priv->cell_view = NULL;
}

static void
btk_combo_box_entry_contents_changed (BtkEntry *entry,
                                      gpointer  user_data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (user_data);

  /*
   *  Fixes regression reported in bug #574059. The old functionality relied on
   *  bug #572478.  As a bugfix, we now emit the "changed" signal ourselves
   *  when the selection was already set to -1.
   */
  if (btk_combo_box_get_active(combo_box) == -1)
    g_signal_emit_by_name (combo_box, "changed");
  else
    btk_combo_box_set_active (combo_box, -1);
}

static void
btk_combo_box_entry_active_changed (BtkComboBox *combo_box,
                                    gpointer     user_data)
{
  BtkComboBoxPrivate *priv = combo_box->priv;
  BtkTreeModel *model;
  BtkTreeIter iter;

  if (btk_combo_box_get_active_iter (combo_box, &iter))
    {
      BtkEntry *entry = BTK_ENTRY (btk_bin_get_child (BTK_BIN (combo_box)));

      if (entry)
	{
          GValue value = {0,};

	  g_signal_handlers_block_by_func (entry,
					   btk_combo_box_entry_contents_changed,
					   combo_box);

	  model = btk_combo_box_get_model (combo_box);

          btk_tree_model_get_value (model, &iter,
                                    priv->text_column, &value);
          g_object_set_property (G_OBJECT (entry), "text", &value);
          g_value_unset (&value);

	  g_signal_handlers_unblock_by_func (entry,
					     btk_combo_box_entry_contents_changed,
					     combo_box);
	}
    }
}

static GObject *
btk_combo_box_constructor (GType                  type,
			   guint                  n_construct_properties,
			   GObjectConstructParam *construct_properties)
{
  GObject            *object;
  BtkComboBox        *combo_box;
  BtkComboBoxPrivate *priv;

  object = G_OBJECT_CLASS (btk_combo_box_parent_class)->constructor
    (type, n_construct_properties, construct_properties);

  combo_box = BTK_COMBO_BOX (object);
  priv      = combo_box->priv;

  if (priv->has_entry)
    {
      BtkWidget *entry;

      entry = btk_entry_new ();
      btk_widget_show (entry);
      btk_container_add (BTK_CONTAINER (combo_box), entry);

      priv->text_renderer = btk_cell_renderer_text_new ();
      btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combo_box),
				  priv->text_renderer, TRUE);

      btk_combo_box_set_active (BTK_COMBO_BOX (combo_box), -1);

      g_signal_connect (combo_box, "changed",
			G_CALLBACK (btk_combo_box_entry_active_changed), NULL);
    }

  return object;
}


static void
btk_combo_box_dispose(GObject* object)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (object);

  if (BTK_IS_MENU (combo_box->priv->popup_widget))
    {
      btk_combo_box_menu_destroy (combo_box);
      btk_menu_detach (BTK_MENU (combo_box->priv->popup_widget));
      combo_box->priv->popup_widget = NULL;
    }

  G_OBJECT_CLASS (btk_combo_box_parent_class)->dispose (object);
}

static void
btk_combo_box_finalize (GObject *object)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (object);
  GSList *i;
  
  if (BTK_IS_TREE_VIEW (combo_box->priv->tree_view))
    btk_combo_box_list_destroy (combo_box);

  if (combo_box->priv->popup_window)
    btk_widget_destroy (combo_box->priv->popup_window);

  btk_combo_box_unset_model (combo_box);

  for (i = combo_box->priv->cells; i; i = i->next)
    {
      ComboCellInfo *info = (ComboCellInfo *)i->data;
      GSList *list = info->attributes;

      if (info->destroy)
	info->destroy (info->func_data);

      while (list && list->next)
	{
	  g_free (list->data);
	  list = list->next->next;
	}
      g_slist_free (info->attributes);

      g_object_unref (info->cell);
      g_slice_free (ComboCellInfo, info);
    }
   g_slist_free (combo_box->priv->cells);

   g_free (combo_box->priv->tearoff_title);

   G_OBJECT_CLASS (btk_combo_box_parent_class)->finalize (object);
}


static gboolean
btk_cell_editable_key_press (BtkWidget   *widget,
			     BdkEventKey *event,
			     gpointer     data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (data);

  if (event->keyval == BDK_Escape)
    {
      g_object_set (combo_box,
                    "editing-canceled", TRUE,
                    NULL);
      btk_cell_editable_editing_done (BTK_CELL_EDITABLE (combo_box));
      btk_cell_editable_remove_widget (BTK_CELL_EDITABLE (combo_box));
      
      return TRUE;
    }
  else if (event->keyval == BDK_Return ||
           event->keyval == BDK_ISO_Enter ||
           event->keyval == BDK_KP_Enter)
    {
      btk_cell_editable_editing_done (BTK_CELL_EDITABLE (combo_box));
      btk_cell_editable_remove_widget (BTK_CELL_EDITABLE (combo_box));
      
      return TRUE;
    }

  return FALSE;
}

static gboolean
popdown_idle (gpointer data)
{
  BtkComboBox *combo_box;

  combo_box = BTK_COMBO_BOX (data);
  
  btk_cell_editable_editing_done (BTK_CELL_EDITABLE (combo_box));
  btk_cell_editable_remove_widget (BTK_CELL_EDITABLE (combo_box));

  g_object_unref (combo_box);

  return FALSE;
}

static void
popdown_handler (BtkWidget *widget,
		 gpointer   data)
{
  bdk_threads_add_idle (popdown_idle, g_object_ref (data));
}

static gboolean
popup_idle (gpointer data)
{
  BtkComboBox *combo_box;

  combo_box = BTK_COMBO_BOX (data);

  if (BTK_IS_MENU (combo_box->priv->popup_widget) &&
      combo_box->priv->cell_view)
    g_signal_connect_object (combo_box->priv->popup_widget,
			     "unmap", G_CALLBACK (popdown_handler),
			     combo_box, 0);
  
  /* we unset this if a menu item is activated */
  g_object_set (combo_box,
                "editing-canceled", TRUE,
                NULL);
  btk_combo_box_popup (combo_box);

  combo_box->priv->popup_idle_id = 0;
  combo_box->priv->activate_button = 0;
  combo_box->priv->activate_time = 0;

  return FALSE;
}

static void
btk_combo_box_start_editing (BtkCellEditable *cell_editable,
			     BdkEvent        *event)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (cell_editable);

  combo_box->priv->is_cell_renderer = TRUE;

  if (combo_box->priv->cell_view)
    {
      g_signal_connect_object (combo_box->priv->button, "key-press-event",
			       G_CALLBACK (btk_cell_editable_key_press), 
			       cell_editable, 0);  

      btk_widget_grab_focus (combo_box->priv->button);
    }
  else
    {
      g_signal_connect_object (BTK_BIN (combo_box)->child, "key-press-event",
			       G_CALLBACK (btk_cell_editable_key_press), 
			       cell_editable, 0);  

      btk_widget_grab_focus (BTK_WIDGET (BTK_BIN (combo_box)->child));
      btk_widget_set_can_focus (combo_box->priv->button, FALSE);
    }

  /* we do the immediate popup only for the optionmenu-like 
   * appearance 
   */  
  if (combo_box->priv->is_cell_renderer && 
      combo_box->priv->cell_view && !combo_box->priv->tree_view)
    {
      if (event && event->type == BDK_BUTTON_PRESS)
        {
          BdkEventButton *event_button = (BdkEventButton *)event;

          combo_box->priv->activate_button = event_button->button;
          combo_box->priv->activate_time = event_button->time;
        }

      combo_box->priv->popup_idle_id = 
          bdk_threads_add_idle (popup_idle, combo_box);
    }
}


/**
 * btk_combo_box_get_add_tearoffs:
 * @combo_box: a #BtkComboBox
 * 
 * Gets the current value of the :add-tearoffs property.
 * 
 * Return value: the current value of the :add-tearoffs property.
 */
gboolean
btk_combo_box_get_add_tearoffs (BtkComboBox *combo_box)
{
  g_return_val_if_fail (BTK_IS_COMBO_BOX (combo_box), FALSE);

  return combo_box->priv->add_tearoffs;
}

/**
 * btk_combo_box_set_add_tearoffs:
 * @combo_box: a #BtkComboBox 
 * @add_tearoffs: %TRUE to add tearoff menu items
 *  
 * Sets whether the popup menu should have a tearoff 
 * menu item.
 *
 * Since: 2.6
 */
void
btk_combo_box_set_add_tearoffs (BtkComboBox *combo_box,
				gboolean     add_tearoffs)
{
  g_return_if_fail (BTK_IS_COMBO_BOX (combo_box));

  add_tearoffs = add_tearoffs != FALSE;

  if (combo_box->priv->add_tearoffs != add_tearoffs)
    {
      combo_box->priv->add_tearoffs = add_tearoffs;
      btk_combo_box_check_appearance (combo_box);
      btk_combo_box_relayout (combo_box);
      g_object_notify (G_OBJECT (combo_box), "add-tearoffs");
    }
}

/**
 * btk_combo_box_get_title:
 * @combo_box: a #BtkComboBox
 *
 * Gets the current title of the menu in tearoff mode. See
 * btk_combo_box_set_add_tearoffs().
 *
 * Returns: the menu's title in tearoff mode. This is an internal copy of the
 * string which must not be freed.
 *
 * Since: 2.10
 */
const gchar*
btk_combo_box_get_title (BtkComboBox *combo_box)
{
  g_return_val_if_fail (BTK_IS_COMBO_BOX (combo_box), NULL);
  
  return combo_box->priv->tearoff_title;
}

static void
btk_combo_box_update_title (BtkComboBox *combo_box)
{
  btk_combo_box_check_appearance (combo_box);
  
  if (combo_box->priv->popup_widget && 
      BTK_IS_MENU (combo_box->priv->popup_widget))
    btk_menu_set_title (BTK_MENU (combo_box->priv->popup_widget), 
			combo_box->priv->tearoff_title);
}

/**
 * btk_combo_box_set_title:
 * @combo_box: a #BtkComboBox 
 * @title: a title for the menu in tearoff mode
 *  
 * Sets the menu's title in tearoff mode.
 *
 * Since: 2.10
 */
void
btk_combo_box_set_title (BtkComboBox *combo_box,
			 const gchar *title)
{
  BtkComboBoxPrivate *priv;

  g_return_if_fail (BTK_IS_COMBO_BOX (combo_box));

  priv = combo_box->priv;

  if (strcmp (title ? title : "", 
	      priv->tearoff_title ? priv->tearoff_title : "") != 0)
    {
      g_free (priv->tearoff_title);
      priv->tearoff_title = g_strdup (title);

      btk_combo_box_update_title (combo_box);

      g_object_notify (G_OBJECT (combo_box), "tearoff-title");
    }
}

/**
 * btk_combo_box_get_popup_accessible:
 * @combo_box: a #BtkComboBox
 *
 * Gets the accessible object corresponding to the combo box's popup.
 *
 * This function is mostly intended for use by accessibility technologies;
 * applications should have little use for it.
 *
 * Returns: (transfer none): the accessible object corresponding
 *     to the combo box's popup.
 *
 * Since: 2.6
 */
BatkObject*
btk_combo_box_get_popup_accessible (BtkComboBox *combo_box)
{
  BatkObject *batk_obj;

  g_return_val_if_fail (BTK_IS_COMBO_BOX (combo_box), NULL);

  if (combo_box->priv->popup_widget)
    {
      batk_obj = btk_widget_get_accessible (combo_box->priv->popup_widget);
      return batk_obj;
    }

  return NULL;
}

/**
 * btk_combo_box_get_row_separator_func:
 * @combo_box: a #BtkComboBox
 * 
 * Returns the current row separator function.
 * 
 * Return value: the current row separator function.
 *
 * Since: 2.6
 */
BtkTreeViewRowSeparatorFunc 
btk_combo_box_get_row_separator_func (BtkComboBox *combo_box)
{
  g_return_val_if_fail (BTK_IS_COMBO_BOX (combo_box), NULL);

  return combo_box->priv->row_separator_func;
}

/**
 * btk_combo_box_set_row_separator_func:
 * @combo_box: a #BtkComboBox
 * @func: a #BtkTreeViewRowSeparatorFunc
 * @data: (allow-none): user data to pass to @func, or %NULL
 * @destroy: (allow-none): destroy notifier for @data, or %NULL
 * 
 * Sets the row separator function, which is used to determine
 * whether a row should be drawn as a separator. If the row separator
 * function is %NULL, no separators are drawn. This is the default value.
 *
 * Since: 2.6
 */
void
btk_combo_box_set_row_separator_func (BtkComboBox                 *combo_box,
				      BtkTreeViewRowSeparatorFunc  func,
				      gpointer                     data,
				      GDestroyNotify               destroy)
{
  g_return_if_fail (BTK_IS_COMBO_BOX (combo_box));

  if (combo_box->priv->row_separator_destroy)
    combo_box->priv->row_separator_destroy (combo_box->priv->row_separator_data);

  combo_box->priv->row_separator_func = func;
  combo_box->priv->row_separator_data = data;
  combo_box->priv->row_separator_destroy = destroy;

  if (combo_box->priv->tree_view)
    btk_tree_view_set_row_separator_func (BTK_TREE_VIEW (combo_box->priv->tree_view), 
					  func, data, NULL);

  btk_combo_box_relayout (combo_box);

  btk_widget_queue_draw (BTK_WIDGET (combo_box));
}

/**
 * btk_combo_box_set_button_sensitivity:
 * @combo_box: a #BtkComboBox
 * @sensitivity: specify the sensitivity of the dropdown button
 *
 * Sets whether the dropdown button of the combo box should be
 * always sensitive (%BTK_SENSITIVITY_ON), never sensitive (%BTK_SENSITIVITY_OFF)
 * or only if there is at least one item to display (%BTK_SENSITIVITY_AUTO).
 *
 * Since: 2.14
 **/
void
btk_combo_box_set_button_sensitivity (BtkComboBox        *combo_box,
                                      BtkSensitivityType  sensitivity)
{
  g_return_if_fail (BTK_IS_COMBO_BOX (combo_box));

  if (combo_box->priv->button_sensitivity != sensitivity)
    {
      combo_box->priv->button_sensitivity = sensitivity;
      btk_combo_box_update_sensitivity (combo_box);

      g_object_notify (G_OBJECT (combo_box), "button-sensitivity");
    }
}

/**
 * btk_combo_box_get_button_sensitivity:
 * @combo_box: a #BtkComboBox
 *
 * Returns whether the combo box sets the dropdown button
 * sensitive or not when there are no items in the model.
 *
 * Return Value: %BTK_SENSITIVITY_ON if the dropdown button
 *    is sensitive when the model is empty, %BTK_SENSITIVITY_OFF
 *    if the button is always insensitive or
 *    %BTK_SENSITIVITY_AUTO if it is only sensitive as long as
 *    the model has one item to be selected.
 *
 * Since: 2.14
 **/
BtkSensitivityType
btk_combo_box_get_button_sensitivity (BtkComboBox *combo_box)
{
  g_return_val_if_fail (BTK_IS_COMBO_BOX (combo_box), FALSE);

  return combo_box->priv->button_sensitivity;
}


/**
 * btk_combo_box_get_has_entry:
 * @combo_box: a #BtkComboBox
 *
 * Returns whether the combo box has an entry.
 *
 * Return Value: whether there is an entry in @combo_box.
 *
 * Since: 2.24
 **/
gboolean
btk_combo_box_get_has_entry (BtkComboBox *combo_box)
{
  g_return_val_if_fail (BTK_IS_COMBO_BOX (combo_box), FALSE);

  return combo_box->priv->has_entry;
}

/**
 * btk_combo_box_set_entry_text_column:
 * @combo_box: A #BtkComboBox
 * @text_column: A column in @model to get the strings from for
 *   the internal entry
 *
 * Sets the model column which @combo_box should use to get strings from
 * to be @text_column. The column @text_column in the model of @combo_box
 * must be of type %G_TYPE_STRING.
 *
 * This is only relevant if @combo_box has been created with
 * #BtkComboBox:has-entry as %TRUE.
 *
 * Since: 2.24
 */
void
btk_combo_box_set_entry_text_column (BtkComboBox *combo_box,
				     gint         text_column)
{
  BtkComboBoxPrivate *priv = combo_box->priv;
  BtkTreeModel *model;

  g_return_if_fail (BTK_IS_COMBO_BOX (combo_box));

  model = btk_combo_box_get_model (combo_box);

  g_return_if_fail (text_column >= 0);
  g_return_if_fail (model == NULL || text_column < btk_tree_model_get_n_columns (model));

  priv->text_column = text_column;

  if (priv->text_renderer)
    btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (combo_box),
                                    priv->text_renderer,
                                    "text", text_column,
                                    NULL);
}

/**
 * btk_combo_box_get_entry_text_column:
 * @combo_box: A #BtkComboBox.
 *
 * Returns the column which @combo_box is using to get the strings
 * from to display in the internal entry.
 *
 * Return value: A column in the data source model of @combo_box.
 *
 * Since: 2.24
 */
gint
btk_combo_box_get_entry_text_column (BtkComboBox *combo_box)
{
  g_return_val_if_fail (BTK_IS_COMBO_BOX (combo_box), 0);

  return combo_box->priv->text_column;
}

/**
 * btk_combo_box_set_focus_on_click:
 * @combo: a #BtkComboBox
 * @focus_on_click: whether the combo box grabs focus when clicked 
 *    with the mouse
 * 
 * Sets whether the combo box will grab focus when it is clicked with 
 * the mouse. Making mouse clicks not grab focus is useful in places 
 * like toolbars where you don't want the keyboard focus removed from 
 * the main area of the application.
 *
 * Since: 2.6
 */
void
btk_combo_box_set_focus_on_click (BtkComboBox *combo_box,
				  gboolean     focus_on_click)
{
  g_return_if_fail (BTK_IS_COMBO_BOX (combo_box));
  
  focus_on_click = focus_on_click != FALSE;

  if (combo_box->priv->focus_on_click != focus_on_click)
    {
      combo_box->priv->focus_on_click = focus_on_click;

      if (combo_box->priv->button)
	btk_button_set_focus_on_click (BTK_BUTTON (combo_box->priv->button),
				       focus_on_click);
      
      g_object_notify (G_OBJECT (combo_box), "focus-on-click");
    }
}

/**
 * btk_combo_box_get_focus_on_click:
 * @combo: a #BtkComboBox
 * 
 * Returns whether the combo box grabs focus when it is clicked 
 * with the mouse. See btk_combo_box_set_focus_on_click().
 *
 * Return value: %TRUE if the combo box grabs focus when it is 
 *     clicked with the mouse.
 *
 * Since: 2.6
 */
gboolean
btk_combo_box_get_focus_on_click (BtkComboBox *combo_box)
{
  g_return_val_if_fail (BTK_IS_COMBO_BOX (combo_box), FALSE);
  
  return combo_box->priv->focus_on_click;
}


static gboolean
btk_combo_box_buildable_custom_tag_start (BtkBuildable  *buildable,
					  BtkBuilder    *builder,
					  GObject       *child,
					  const gchar   *tagname,
					  GMarkupParser *parser,
					  gpointer      *data)
{
  if (parent_buildable_iface->custom_tag_start (buildable, builder, child,
						tagname, parser, data))
    return TRUE;

  return _btk_cell_layout_buildable_custom_tag_start (buildable, builder, child,
						      tagname, parser, data);
}

static void
btk_combo_box_buildable_custom_tag_end (BtkBuildable *buildable,
					BtkBuilder   *builder,
					GObject      *child,
					const gchar  *tagname,
					gpointer     *data)
{
  if (strcmp (tagname, "attributes") == 0)
    _btk_cell_layout_buildable_custom_tag_end (buildable, builder, child, tagname,
					       data);
  else
    parent_buildable_iface->custom_tag_end (buildable, builder, child, tagname,
					    data);
}

static GObject *
btk_combo_box_buildable_get_internal_child (BtkBuildable *buildable,
					    BtkBuilder   *builder,
					    const gchar  *childname)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (buildable);

  if (combo_box->priv->has_entry && strcmp (childname, "entry") == 0)
    return G_OBJECT (btk_bin_get_child (BTK_BIN (buildable)));

  return parent_buildable_iface->get_internal_child (buildable, builder, childname);
}

#define __BTK_COMBO_BOX_C__
#include "btkaliasdef.c"
