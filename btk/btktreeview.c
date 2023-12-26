/* btktreeview.c
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
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
#include <string.h>
#include <bdk/bdkkeysyms.h>

#include "btktreeview.h"
#include "btkrbtree.h"
#include "btktreednd.h"
#include "btktreeprivate.h"
#include "btkcellrenderer.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkbuildable.h"
#include "btkbutton.h"
#include "btkalignment.h"
#include "btklabel.h"
#include "btkhbox.h"
#include "btkvbox.h"
#include "btkarrow.h"
#include "btkintl.h"
#include "btkbindings.h"
#include "btkcontainer.h"
#include "btkentry.h"
#include "btkframe.h"
#include "btktreemodelsort.h"
#include "btktooltip.h"
#include "btkprivate.h"
#include "btkalias.h"

#define BTK_TREE_VIEW_PRIORITY_VALIDATE (BDK_PRIORITY_REDRAW + 5)
#define BTK_TREE_VIEW_PRIORITY_SCROLL_SYNC (BTK_TREE_VIEW_PRIORITY_VALIDATE + 2)
#define BTK_TREE_VIEW_TIME_MS_PER_IDLE 30
#define SCROLL_EDGE_SIZE 15
#define EXPANDER_EXTRA_PADDING 4
#define BTK_TREE_VIEW_SEARCH_DIALOG_TIMEOUT 5000
#define AUTO_EXPAND_TIMEOUT 500

/* The "background" areas of all rows/cells add up to cover the entire tree.
 * The background includes all inter-row and inter-cell spacing.
 * The "cell" areas are the cell_area passed in to btk_cell_renderer_render(),
 * i.e. just the cells, no spacing.
 */

#define BACKGROUND_HEIGHT(node) (BTK_RBNODE_GET_HEIGHT (node))
#define CELL_HEIGHT(node, separator) ((BACKGROUND_HEIGHT (node)) - (separator))

/* Translate from bin_window coordinates to rbtree (tree coordinates) and
 * vice versa.
 */
#define TREE_WINDOW_Y_TO_RBTREE_Y(tree_view,y) ((y) + tree_view->priv->dy)
#define RBTREE_Y_TO_TREE_WINDOW_Y(tree_view,y) ((y) - tree_view->priv->dy)

/* This is in bin_window coordinates */
#define BACKGROUND_FIRST_PIXEL(tree_view,tree,node) (RBTREE_Y_TO_TREE_WINDOW_Y (tree_view, _btk_rbtree_node_find_offset ((tree), (node))))
#define CELL_FIRST_PIXEL(tree_view,tree,node,separator) (BACKGROUND_FIRST_PIXEL (tree_view,tree,node) + separator/2)

#define ROW_HEIGHT(tree_view,height) \
  ((height > 0) ? (height) : (tree_view)->priv->expander_size)


typedef struct _BtkTreeViewChild BtkTreeViewChild;
struct _BtkTreeViewChild
{
  BtkWidget *widget;
  gint x;
  gint y;
  gint width;
  gint height;
};


typedef struct _TreeViewDragInfo TreeViewDragInfo;
struct _TreeViewDragInfo
{
  BdkModifierType start_button_mask;
  BtkTargetList *_unused_source_target_list;
  BdkDragAction source_actions;

  BtkTargetList *_unused_dest_target_list;

  guint source_set : 1;
  guint dest_set : 1;
};


/* Signals */
enum
{
  ROW_ACTIVATED,
  TEST_EXPAND_ROW,
  TEST_COLLAPSE_ROW,
  ROW_EXPANDED,
  ROW_COLLAPSED,
  COLUMNS_CHANGED,
  CURSOR_CHANGED,
  MOVE_CURSOR,
  SELECT_ALL,
  UNSELECT_ALL,
  SELECT_CURSOR_ROW,
  TOGGLE_CURSOR_ROW,
  EXPAND_COLLAPSE_CURSOR_ROW,
  SELECT_CURSOR_PARENT,
  START_INTERACTIVE_SEARCH,
  LAST_SIGNAL
};

/* Properties */
enum {
  PROP_0,
  PROP_MODEL,
  PROP_HADJUSTMENT,
  PROP_VADJUSTMENT,
  PROP_HEADERS_VISIBLE,
  PROP_HEADERS_CLICKABLE,
  PROP_EXPANDER_COLUMN,
  PROP_REORDERABLE,
  PROP_RULES_HINT,
  PROP_ENABLE_SEARCH,
  PROP_SEARCH_COLUMN,
  PROP_FIXED_HEIGHT_MODE,
  PROP_HOVER_SELECTION,
  PROP_HOVER_EXPAND,
  PROP_SHOW_EXPANDERS,
  PROP_LEVEL_INDENTATION,
  PROP_RUBBER_BANDING,
  PROP_ENABLE_GRID_LINES,
  PROP_ENABLE_TREE_LINES,
  PROP_TOOLTIP_COLUMN
};

/* object signals */
static void     btk_tree_view_finalize             (BObject          *object);
static void     btk_tree_view_set_property         (BObject         *object,
						    guint            prop_id,
						    const BValue    *value,
						    BParamSpec      *pspec);
static void     btk_tree_view_get_property         (BObject         *object,
						    guint            prop_id,
						    BValue          *value,
						    BParamSpec      *pspec);

/* btkobject signals */
static void     btk_tree_view_destroy              (BtkObject        *object);

/* btkwidget signals */
static void     btk_tree_view_realize              (BtkWidget        *widget);
static void     btk_tree_view_unrealize            (BtkWidget        *widget);
static void     btk_tree_view_map                  (BtkWidget        *widget);
static void     btk_tree_view_size_request         (BtkWidget        *widget,
						    BtkRequisition   *requisition);
static void     btk_tree_view_size_allocate        (BtkWidget        *widget,
						    BtkAllocation    *allocation);
static gboolean btk_tree_view_expose               (BtkWidget        *widget,
						    BdkEventExpose   *event);
static gboolean btk_tree_view_key_press            (BtkWidget        *widget,
						    BdkEventKey      *event);
static gboolean btk_tree_view_key_release          (BtkWidget        *widget,
						    BdkEventKey      *event);
static gboolean btk_tree_view_motion               (BtkWidget        *widget,
						    BdkEventMotion   *event);
static gboolean btk_tree_view_enter_notify         (BtkWidget        *widget,
						    BdkEventCrossing *event);
static gboolean btk_tree_view_leave_notify         (BtkWidget        *widget,
						    BdkEventCrossing *event);
static gboolean btk_tree_view_button_press         (BtkWidget        *widget,
						    BdkEventButton   *event);
static gboolean btk_tree_view_button_release       (BtkWidget        *widget,
						    BdkEventButton   *event);
static gboolean btk_tree_view_grab_broken          (BtkWidget          *widget,
						    BdkEventGrabBroken *event);
#if 0
static gboolean btk_tree_view_configure            (BtkWidget         *widget,
						    BdkEventConfigure *event);
#endif

static void     btk_tree_view_set_focus_child      (BtkContainer     *container,
						    BtkWidget        *child);
static gint     btk_tree_view_focus_out            (BtkWidget        *widget,
						    BdkEventFocus    *event);
static gint     btk_tree_view_focus                (BtkWidget        *widget,
						    BtkDirectionType  direction);
static void     btk_tree_view_grab_focus           (BtkWidget        *widget);
static void     btk_tree_view_style_set            (BtkWidget        *widget,
						    BtkStyle         *previous_style);
static void     btk_tree_view_grab_notify          (BtkWidget        *widget,
						    gboolean          was_grabbed);
static void     btk_tree_view_state_changed        (BtkWidget        *widget,
						    BtkStateType      previous_state);

/* container signals */
static void     btk_tree_view_remove               (BtkContainer     *container,
						    BtkWidget        *widget);
static void     btk_tree_view_forall               (BtkContainer     *container,
						    gboolean          include_internals,
						    BtkCallback       callback,
						    gpointer          callback_data);

/* Source side drag signals */
static void btk_tree_view_drag_begin       (BtkWidget        *widget,
                                            BdkDragContext   *context);
static void btk_tree_view_drag_end         (BtkWidget        *widget,
                                            BdkDragContext   *context);
static void btk_tree_view_drag_data_get    (BtkWidget        *widget,
                                            BdkDragContext   *context,
                                            BtkSelectionData *selection_data,
                                            guint             info,
                                            guint             time);
static void btk_tree_view_drag_data_delete (BtkWidget        *widget,
                                            BdkDragContext   *context);

/* Target side drag signals */
static void     btk_tree_view_drag_leave         (BtkWidget        *widget,
                                                  BdkDragContext   *context,
                                                  guint             time);
static gboolean btk_tree_view_drag_motion        (BtkWidget        *widget,
                                                  BdkDragContext   *context,
                                                  gint              x,
                                                  gint              y,
                                                  guint             time);
static gboolean btk_tree_view_drag_drop          (BtkWidget        *widget,
                                                  BdkDragContext   *context,
                                                  gint              x,
                                                  gint              y,
                                                  guint             time);
static void     btk_tree_view_drag_data_received (BtkWidget        *widget,
                                                  BdkDragContext   *context,
                                                  gint              x,
                                                  gint              y,
                                                  BtkSelectionData *selection_data,
                                                  guint             info,
                                                  guint             time);

/* tree_model signals */
static void btk_tree_view_set_adjustments                 (BtkTreeView     *tree_view,
							   BtkAdjustment   *hadj,
							   BtkAdjustment   *vadj);
static gboolean btk_tree_view_real_move_cursor            (BtkTreeView     *tree_view,
							   BtkMovementStep  step,
							   gint             count);
static gboolean btk_tree_view_real_select_all             (BtkTreeView     *tree_view);
static gboolean btk_tree_view_real_unselect_all           (BtkTreeView     *tree_view);
static gboolean btk_tree_view_real_select_cursor_row      (BtkTreeView     *tree_view,
							   gboolean         start_editing);
static gboolean btk_tree_view_real_toggle_cursor_row      (BtkTreeView     *tree_view);
static gboolean btk_tree_view_real_expand_collapse_cursor_row (BtkTreeView     *tree_view,
							       gboolean         logical,
							       gboolean         expand,
							       gboolean         open_all);
static gboolean btk_tree_view_real_select_cursor_parent   (BtkTreeView     *tree_view);
static void btk_tree_view_row_changed                     (BtkTreeModel    *model,
							   BtkTreePath     *path,
							   BtkTreeIter     *iter,
							   gpointer         data);
static void btk_tree_view_row_inserted                    (BtkTreeModel    *model,
							   BtkTreePath     *path,
							   BtkTreeIter     *iter,
							   gpointer         data);
static void btk_tree_view_row_has_child_toggled           (BtkTreeModel    *model,
							   BtkTreePath     *path,
							   BtkTreeIter     *iter,
							   gpointer         data);
static void btk_tree_view_row_deleted                     (BtkTreeModel    *model,
							   BtkTreePath     *path,
							   gpointer         data);
static void btk_tree_view_rows_reordered                  (BtkTreeModel    *model,
							   BtkTreePath     *parent,
							   BtkTreeIter     *iter,
							   gint            *new_order,
							   gpointer         data);

/* Incremental reflow */
static gboolean validate_row             (BtkTreeView *tree_view,
					  BtkRBTree   *tree,
					  BtkRBNode   *node,
					  BtkTreeIter *iter,
					  BtkTreePath *path);
static void     validate_visible_area    (BtkTreeView *tree_view);
static gboolean validate_rows_handler    (BtkTreeView *tree_view);
static gboolean do_validate_rows         (BtkTreeView *tree_view,
					  gboolean     size_request);
static gboolean validate_rows            (BtkTreeView *tree_view);
static gboolean presize_handler_callback (gpointer     data);
static void     install_presize_handler  (BtkTreeView *tree_view);
static void     install_scroll_sync_handler (BtkTreeView *tree_view);
static void     btk_tree_view_set_top_row   (BtkTreeView *tree_view,
					     BtkTreePath *path,
					     gint         offset);
static void	btk_tree_view_dy_to_top_row (BtkTreeView *tree_view);
static void     btk_tree_view_top_row_to_dy (BtkTreeView *tree_view);
static void     invalidate_empty_focus      (BtkTreeView *tree_view);

/* Internal functions */
static gboolean btk_tree_view_is_expander_column             (BtkTreeView        *tree_view,
							      BtkTreeViewColumn  *column);
static void     btk_tree_view_add_move_binding               (BtkBindingSet      *binding_set,
							      guint               keyval,
							      guint               modmask,
							      gboolean            add_shifted_binding,
							      BtkMovementStep     step,
							      gint                count);
static gint     btk_tree_view_unref_and_check_selection_tree (BtkTreeView        *tree_view,
							      BtkRBTree          *tree);
static void     btk_tree_view_queue_draw_path                (BtkTreeView        *tree_view,
							      BtkTreePath        *path,
							      const BdkRectangle *clip_rect);
static void     btk_tree_view_queue_draw_arrow               (BtkTreeView        *tree_view,
							      BtkRBTree          *tree,
							      BtkRBNode          *node,
							      const BdkRectangle *clip_rect);
static void     btk_tree_view_draw_arrow                     (BtkTreeView        *tree_view,
							      BtkRBTree          *tree,
							      BtkRBNode          *node,
							      gint                x,
							      gint                y);
static void     btk_tree_view_get_arrow_xrange               (BtkTreeView        *tree_view,
							      BtkRBTree          *tree,
							      gint               *x1,
							      gint               *x2);
static gint     btk_tree_view_new_column_width               (BtkTreeView        *tree_view,
							      gint                i,
							      gint               *x);
static void     btk_tree_view_adjustment_changed             (BtkAdjustment      *adjustment,
							      BtkTreeView        *tree_view);
static void     btk_tree_view_build_tree                     (BtkTreeView        *tree_view,
							      BtkRBTree          *tree,
							      BtkTreeIter        *iter,
							      gint                depth,
							      gboolean            recurse);
static void     btk_tree_view_clamp_node_visible             (BtkTreeView        *tree_view,
							      BtkRBTree          *tree,
							      BtkRBNode          *node);
static void     btk_tree_view_clamp_column_visible           (BtkTreeView        *tree_view,
							      BtkTreeViewColumn  *column,
							      gboolean            focus_to_cell);
static gboolean btk_tree_view_maybe_begin_dragging_row       (BtkTreeView        *tree_view,
							      BdkEventMotion     *event);
static void     btk_tree_view_focus_to_cursor                (BtkTreeView        *tree_view);
static void     btk_tree_view_move_cursor_up_down            (BtkTreeView        *tree_view,
							      gint                count);
static void     btk_tree_view_move_cursor_page_up_down       (BtkTreeView        *tree_view,
							      gint                count);
static void     btk_tree_view_move_cursor_left_right         (BtkTreeView        *tree_view,
							      gint                count);
static void     btk_tree_view_move_cursor_start_end          (BtkTreeView        *tree_view,
							      gint                count);
static gboolean btk_tree_view_real_collapse_row              (BtkTreeView        *tree_view,
							      BtkTreePath        *path,
							      BtkRBTree          *tree,
							      BtkRBNode          *node,
							      gboolean            animate);
static gboolean btk_tree_view_real_expand_row                (BtkTreeView        *tree_view,
							      BtkTreePath        *path,
							      BtkRBTree          *tree,
							      BtkRBNode          *node,
							      gboolean            open_all,
							      gboolean            animate);
static void     btk_tree_view_real_set_cursor                (BtkTreeView        *tree_view,
							      BtkTreePath        *path,
							      gboolean            clear_and_select,
							      gboolean            clamp_node);
static gboolean btk_tree_view_has_special_cell               (BtkTreeView        *tree_view);
static void     column_sizing_notify                         (BObject            *object,
                                                              BParamSpec         *pspec,
                                                              gpointer            data);
static gboolean expand_collapse_timeout                      (gpointer            data);
static void     add_expand_collapse_timeout                  (BtkTreeView        *tree_view,
                                                              BtkRBTree          *tree,
                                                              BtkRBNode          *node,
                                                              gboolean            expand);
static void     remove_expand_collapse_timeout               (BtkTreeView        *tree_view);
static void     cancel_arrow_animation                       (BtkTreeView        *tree_view);
static gboolean do_expand_collapse                           (BtkTreeView        *tree_view);
static void     btk_tree_view_stop_rubber_band               (BtkTreeView        *tree_view);
static void     update_prelight                              (BtkTreeView        *tree_view,
                                                              int                 x,
                                                              int                 y);

/* interactive search */
static void     btk_tree_view_ensure_interactive_directory (BtkTreeView *tree_view);
static void     btk_tree_view_search_dialog_hide     (BtkWidget        *search_dialog,
							 BtkTreeView      *tree_view);
static void     btk_tree_view_search_position_func      (BtkTreeView      *tree_view,
							 BtkWidget        *search_dialog,
							 gpointer          user_data);
static void     btk_tree_view_search_disable_popdown    (BtkEntry         *entry,
							 BtkMenu          *menu,
							 gpointer          data);
static void     btk_tree_view_search_preedit_changed    (BtkIMContext     *im_context,
							 BtkTreeView      *tree_view);
static void     btk_tree_view_search_activate           (BtkEntry         *entry,
							 BtkTreeView      *tree_view);
static gboolean btk_tree_view_real_search_enable_popdown(gpointer          data);
static void     btk_tree_view_search_enable_popdown     (BtkWidget        *widget,
							 gpointer          data);
static gboolean btk_tree_view_search_delete_event       (BtkWidget        *widget,
							 BdkEventAny      *event,
							 BtkTreeView      *tree_view);
static gboolean btk_tree_view_search_button_press_event (BtkWidget        *widget,
							 BdkEventButton   *event,
							 BtkTreeView      *tree_view);
static gboolean btk_tree_view_search_scroll_event       (BtkWidget        *entry,
							 BdkEventScroll   *event,
							 BtkTreeView      *tree_view);
static gboolean btk_tree_view_search_key_press_event    (BtkWidget        *entry,
							 BdkEventKey      *event,
							 BtkTreeView      *tree_view);
static gboolean btk_tree_view_search_move               (BtkWidget        *window,
							 BtkTreeView      *tree_view,
							 gboolean          up);
static gboolean btk_tree_view_search_equal_func         (BtkTreeModel     *model,
							 gint              column,
							 const gchar      *key,
							 BtkTreeIter      *iter,
							 gpointer          search_data);
static gboolean btk_tree_view_search_iter               (BtkTreeModel     *model,
							 BtkTreeSelection *selection,
							 BtkTreeIter      *iter,
							 const gchar      *text,
							 gint             *count,
							 gint              n);
static void     btk_tree_view_search_init               (BtkWidget        *entry,
							 BtkTreeView      *tree_view);
static void     btk_tree_view_put                       (BtkTreeView      *tree_view,
							 BtkWidget        *child_widget,
							 gint              x,
							 gint              y,
							 gint              width,
							 gint              height);
static gboolean btk_tree_view_start_editing             (BtkTreeView      *tree_view,
							 BtkTreePath      *cursor_path);
static void btk_tree_view_real_start_editing (BtkTreeView       *tree_view,
					      BtkTreeViewColumn *column,
					      BtkTreePath       *path,
					      BtkCellEditable   *cell_editable,
					      BdkRectangle      *cell_area,
					      BdkEvent          *event,
					      guint              flags);
static void btk_tree_view_stop_editing                  (BtkTreeView *tree_view,
							 gboolean     cancel_editing);
static gboolean btk_tree_view_real_start_interactive_search (BtkTreeView *tree_view,
							     gboolean     keybinding);
static gboolean btk_tree_view_start_interactive_search      (BtkTreeView *tree_view);
static BtkTreeViewColumn *btk_tree_view_get_drop_column (BtkTreeView       *tree_view,
							 BtkTreeViewColumn *column,
							 gint               drop_position);

/* BtkBuildable */
static void     btk_tree_view_buildable_add_child          (BtkBuildable      *tree_view,
							    BtkBuilder        *builder,
							    BObject           *child,
							    const gchar       *type);
static BObject *btk_tree_view_buildable_get_internal_child (BtkBuildable      *buildable,
							    BtkBuilder        *builder,
							    const gchar       *childname);
static void     btk_tree_view_buildable_init               (BtkBuildableIface *iface);


static gboolean scroll_row_timeout                   (gpointer     data);
static void     add_scroll_timeout                   (BtkTreeView *tree_view);
static void     remove_scroll_timeout                (BtkTreeView *tree_view);

static guint tree_view_signals [LAST_SIGNAL] = { 0 };



/* GType Methods
 */

G_DEFINE_TYPE_WITH_CODE (BtkTreeView, btk_tree_view, BTK_TYPE_CONTAINER,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
						btk_tree_view_buildable_init))

static void
btk_tree_view_class_init (BtkTreeViewClass *class)
{
  BObjectClass *o_class;
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;
  BtkContainerClass *container_class;
  BtkBindingSet *binding_set;

  binding_set = btk_binding_set_by_class (class);

  o_class = (BObjectClass *) class;
  object_class = (BtkObjectClass *) class;
  widget_class = (BtkWidgetClass *) class;
  container_class = (BtkContainerClass *) class;

  /* BObject signals */
  o_class->set_property = btk_tree_view_set_property;
  o_class->get_property = btk_tree_view_get_property;
  o_class->finalize = btk_tree_view_finalize;

  /* BtkObject signals */
  object_class->destroy = btk_tree_view_destroy;

  /* BtkWidget signals */
  widget_class->map = btk_tree_view_map;
  widget_class->realize = btk_tree_view_realize;
  widget_class->unrealize = btk_tree_view_unrealize;
  widget_class->size_request = btk_tree_view_size_request;
  widget_class->size_allocate = btk_tree_view_size_allocate;
  widget_class->button_press_event = btk_tree_view_button_press;
  widget_class->button_release_event = btk_tree_view_button_release;
  widget_class->grab_broken_event = btk_tree_view_grab_broken;
  /*widget_class->configure_event = btk_tree_view_configure;*/
  widget_class->motion_notify_event = btk_tree_view_motion;
  widget_class->expose_event = btk_tree_view_expose;
  widget_class->key_press_event = btk_tree_view_key_press;
  widget_class->key_release_event = btk_tree_view_key_release;
  widget_class->enter_notify_event = btk_tree_view_enter_notify;
  widget_class->leave_notify_event = btk_tree_view_leave_notify;
  widget_class->focus_out_event = btk_tree_view_focus_out;
  widget_class->drag_begin = btk_tree_view_drag_begin;
  widget_class->drag_end = btk_tree_view_drag_end;
  widget_class->drag_data_get = btk_tree_view_drag_data_get;
  widget_class->drag_data_delete = btk_tree_view_drag_data_delete;
  widget_class->drag_leave = btk_tree_view_drag_leave;
  widget_class->drag_motion = btk_tree_view_drag_motion;
  widget_class->drag_drop = btk_tree_view_drag_drop;
  widget_class->drag_data_received = btk_tree_view_drag_data_received;
  widget_class->focus = btk_tree_view_focus;
  widget_class->grab_focus = btk_tree_view_grab_focus;
  widget_class->style_set = btk_tree_view_style_set;
  widget_class->grab_notify = btk_tree_view_grab_notify;
  widget_class->state_changed = btk_tree_view_state_changed;

  /* BtkContainer signals */
  container_class->remove = btk_tree_view_remove;
  container_class->forall = btk_tree_view_forall;
  container_class->set_focus_child = btk_tree_view_set_focus_child;

  class->set_scroll_adjustments = btk_tree_view_set_adjustments;
  class->move_cursor = btk_tree_view_real_move_cursor;
  class->select_all = btk_tree_view_real_select_all;
  class->unselect_all = btk_tree_view_real_unselect_all;
  class->select_cursor_row = btk_tree_view_real_select_cursor_row;
  class->toggle_cursor_row = btk_tree_view_real_toggle_cursor_row;
  class->expand_collapse_cursor_row = btk_tree_view_real_expand_collapse_cursor_row;
  class->select_cursor_parent = btk_tree_view_real_select_cursor_parent;
  class->start_interactive_search = btk_tree_view_start_interactive_search;

  /* Properties */

  g_object_class_install_property (o_class,
                                   PROP_MODEL,
                                   g_param_spec_object ("model",
							P_("TreeView Model"),
							P_("The model for the tree view"),
							BTK_TYPE_TREE_MODEL,
							BTK_PARAM_READWRITE));

  g_object_class_install_property (o_class,
                                   PROP_HADJUSTMENT,
                                   g_param_spec_object ("hadjustment",
							P_("Horizontal Adjustment"),
                                                        P_("Horizontal Adjustment for the widget"),
                                                        BTK_TYPE_ADJUSTMENT,
                                                        BTK_PARAM_READWRITE));

  g_object_class_install_property (o_class,
                                   PROP_VADJUSTMENT,
                                   g_param_spec_object ("vadjustment",
							P_("Vertical Adjustment"),
                                                        P_("Vertical Adjustment for the widget"),
                                                        BTK_TYPE_ADJUSTMENT,
                                                        BTK_PARAM_READWRITE));

  g_object_class_install_property (o_class,
                                   PROP_HEADERS_VISIBLE,
                                   g_param_spec_boolean ("headers-visible",
							 P_("Headers Visible"),
							 P_("Show the column header buttons"),
							 TRUE,
							 BTK_PARAM_READWRITE));

  g_object_class_install_property (o_class,
                                   PROP_HEADERS_CLICKABLE,
                                   g_param_spec_boolean ("headers-clickable",
							 P_("Headers Clickable"),
							 P_("Column headers respond to click events"),
							 TRUE,
							 BTK_PARAM_READWRITE));

  g_object_class_install_property (o_class,
                                   PROP_EXPANDER_COLUMN,
                                   g_param_spec_object ("expander-column",
							P_("Expander Column"),
							P_("Set the column for the expander column"),
							BTK_TYPE_TREE_VIEW_COLUMN,
							BTK_PARAM_READWRITE));

  g_object_class_install_property (o_class,
                                   PROP_REORDERABLE,
                                   g_param_spec_boolean ("reorderable",
							 P_("Reorderable"),
							 P_("View is reorderable"),
							 FALSE,
							 BTK_PARAM_READWRITE));

  g_object_class_install_property (o_class,
                                   PROP_RULES_HINT,
                                   g_param_spec_boolean ("rules-hint",
							 P_("Rules Hint"),
							 P_("Set a hint to the theme engine to draw rows in alternating colors"),
							 FALSE,
							 BTK_PARAM_READWRITE));

    g_object_class_install_property (o_class,
				     PROP_ENABLE_SEARCH,
				     g_param_spec_boolean ("enable-search",
							   P_("Enable Search"),
							   P_("View allows user to search through columns interactively"),
							   TRUE,
							   BTK_PARAM_READWRITE));

    g_object_class_install_property (o_class,
				     PROP_SEARCH_COLUMN,
				     g_param_spec_int ("search-column",
						       P_("Search Column"),
						       P_("Model column to search through during interactive search"),
						       -1,
						       G_MAXINT,
						       -1,
						       BTK_PARAM_READWRITE));

    /**
     * BtkTreeView:fixed-height-mode:
     *
     * Setting the ::fixed-height-mode property to %TRUE speeds up 
     * #BtkTreeView by assuming that all rows have the same height. 
     * Only enable this option if all rows are the same height.  
     * Please see btk_tree_view_set_fixed_height_mode() for more 
     * information on this option.
     *
     * Since: 2.4
     **/
    g_object_class_install_property (o_class,
                                     PROP_FIXED_HEIGHT_MODE,
                                     g_param_spec_boolean ("fixed-height-mode",
                                                           P_("Fixed Height Mode"),
                                                           P_("Speeds up BtkTreeView by assuming that all rows have the same height"),
                                                           FALSE,
                                                           BTK_PARAM_READWRITE));
    
    /**
     * BtkTreeView:hover-selection:
     * 
     * Enables of disables the hover selection mode of @tree_view.
     * Hover selection makes the selected row follow the pointer.
     * Currently, this works only for the selection modes 
     * %BTK_SELECTION_SINGLE and %BTK_SELECTION_BROWSE.
     *
     * This mode is primarily intended for treeviews in popups, e.g.
     * in #BtkComboBox or #BtkEntryCompletion.
     *
     * Since: 2.6
     */
    g_object_class_install_property (o_class,
                                     PROP_HOVER_SELECTION,
                                     g_param_spec_boolean ("hover-selection",
                                                           P_("Hover Selection"),
                                                           P_("Whether the selection should follow the pointer"),
                                                           FALSE,
                                                           BTK_PARAM_READWRITE));

    /**
     * BtkTreeView:hover-expand:
     * 
     * Enables of disables the hover expansion mode of @tree_view.
     * Hover expansion makes rows expand or collapse if the pointer moves 
     * over them.
     *
     * This mode is primarily intended for treeviews in popups, e.g.
     * in #BtkComboBox or #BtkEntryCompletion.
     *
     * Since: 2.6
     */
    g_object_class_install_property (o_class,
                                     PROP_HOVER_EXPAND,
                                     g_param_spec_boolean ("hover-expand",
                                                           P_("Hover Expand"),
                                                           P_("Whether rows should be expanded/collapsed when the pointer moves over them"),
                                                           FALSE,
                                                           BTK_PARAM_READWRITE));

    /**
     * BtkTreeView:show-expanders:
     *
     * %TRUE if the view has expanders.
     *
     * Since: 2.12
     */
    g_object_class_install_property (o_class,
				     PROP_SHOW_EXPANDERS,
				     g_param_spec_boolean ("show-expanders",
							   P_("Show Expanders"),
							   P_("View has expanders"),
							   TRUE,
							   BTK_PARAM_READWRITE));

    /**
     * BtkTreeView:level-indentation:
     *
     * Extra indentation for each level.
     *
     * Since: 2.12
     */
    g_object_class_install_property (o_class,
				     PROP_LEVEL_INDENTATION,
				     g_param_spec_int ("level-indentation",
						       P_("Level Indentation"),
						       P_("Extra indentation for each level"),
						       0,
						       G_MAXINT,
						       0,
						       BTK_PARAM_READWRITE));

    g_object_class_install_property (o_class,
                                     PROP_RUBBER_BANDING,
                                     g_param_spec_boolean ("rubber-banding",
                                                           P_("Rubber Banding"),
                                                           P_("Whether to enable selection of multiple items by dragging the mouse pointer"),
                                                           FALSE,
                                                           BTK_PARAM_READWRITE));

    g_object_class_install_property (o_class,
                                     PROP_ENABLE_GRID_LINES,
                                     g_param_spec_enum ("enable-grid-lines",
							P_("Enable Grid Lines"),
							P_("Whether grid lines should be drawn in the tree view"),
							BTK_TYPE_TREE_VIEW_GRID_LINES,
							BTK_TREE_VIEW_GRID_LINES_NONE,
							BTK_PARAM_READWRITE));

    g_object_class_install_property (o_class,
                                     PROP_ENABLE_TREE_LINES,
                                     g_param_spec_boolean ("enable-tree-lines",
                                                           P_("Enable Tree Lines"),
                                                           P_("Whether tree lines should be drawn in the tree view"),
                                                           FALSE,
                                                           BTK_PARAM_READWRITE));

    g_object_class_install_property (o_class,
				     PROP_TOOLTIP_COLUMN,
				     g_param_spec_int ("tooltip-column",
						       P_("Tooltip Column"),
						       P_("The column in the model containing the tooltip texts for the rows"),
						       -1,
						       G_MAXINT,
						       -1,
						       BTK_PARAM_READWRITE));

  /* Style properties */
#define _TREE_VIEW_EXPANDER_SIZE 12
#define _TREE_VIEW_VERTICAL_SEPARATOR 2
#define _TREE_VIEW_HORIZONTAL_SEPARATOR 2

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("expander-size",
							     P_("Expander Size"),
							     P_("Size of the expander arrow"),
							     0,
							     G_MAXINT,
							     _TREE_VIEW_EXPANDER_SIZE,
							     BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("vertical-separator",
							     P_("Vertical Separator Width"),
							     P_("Vertical space between cells.  Must be an even number"),
							     0,
							     G_MAXINT,
							     _TREE_VIEW_VERTICAL_SEPARATOR,
							     BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("horizontal-separator",
							     P_("Horizontal Separator Width"),
							     P_("Horizontal space between cells.  Must be an even number"),
							     0,
							     G_MAXINT,
							     _TREE_VIEW_HORIZONTAL_SEPARATOR,
							     BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("allow-rules",
								 P_("Allow Rules"),
								 P_("Allow drawing of alternating color rows"),
								 TRUE,
								 BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("indent-expanders",
								 P_("Indent Expanders"),
								 P_("Make the expanders indented"),
								 TRUE,
								 BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boxed ("even-row-color",
                                                               P_("Even Row Color"),
                                                               P_("Color to use for even rows"),
							       BDK_TYPE_COLOR,
							       BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boxed ("odd-row-color",
                                                               P_("Odd Row Color"),
                                                               P_("Color to use for odd rows"),
							       BDK_TYPE_COLOR,
							       BTK_PARAM_READABLE));

  /**
   * BtkTreeView:row-ending-details:
   *
   * Enable extended row background themeing
   *
   * Deprecated: 2.22: This style property will be removed in BTK+ 3
   */
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("row-ending-details",
								 P_("Row Ending details"),
								 P_("Enable extended row background theming"),
								 FALSE,
								 BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("grid-line-width",
							     P_("Grid line width"),
							     P_("Width, in pixels, of the tree view grid lines"),
							     0, G_MAXINT, 1,
							     BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("tree-line-width",
							     P_("Tree line width"),
							     P_("Width, in pixels, of the tree view lines"),
							     0, G_MAXINT, 1,
							     BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_string ("grid-line-pattern",
								P_("Grid line pattern"),
								P_("Dash pattern used to draw the tree view grid lines"),
								"\1\1",
								BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_string ("tree-line-pattern",
								P_("Tree line pattern"),
								P_("Dash pattern used to draw the tree view lines"),
								"\1\1",
								BTK_PARAM_READABLE));

  /* Signals */
  /**
   * BtkTreeView::set-scroll-adjustments
   * @horizontal: the horizontal #BtkAdjustment
   * @vertical: the vertical #BtkAdjustment
   *
   * Set the scroll adjustments for the tree view. Usually scrolled containers
   * like #BtkScrolledWindow will emit this signal to connect two instances
   * of #BtkScrollbar to the scroll directions of the #BtkTreeView.
   */
  widget_class->set_scroll_adjustments_signal =
    g_signal_new (I_("set-scroll-adjustments"),
		  B_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkTreeViewClass, set_scroll_adjustments),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT_OBJECT,
		  B_TYPE_NONE, 2,
		  BTK_TYPE_ADJUSTMENT,
		  BTK_TYPE_ADJUSTMENT);

  /**
   * BtkTreeView::row-activated:
   * @tree_view: the object on which the signal is emitted
   * @path: the #BtkTreePath for the activated row
   * @column: the #BtkTreeViewColumn in which the activation occurred
   *
   * The "row-activated" signal is emitted when the method
   * btk_tree_view_row_activated() is called or the user double clicks 
   * a treeview row. It is also emitted when a non-editable row is 
   * selected and one of the keys: Space, Shift+Space, Return or 
   * Enter is pressed.
   * 
   * For selection handling refer to the <link linkend="TreeWidget">tree 
   * widget conceptual overview</link> as well as #BtkTreeSelection.
   */
  tree_view_signals[ROW_ACTIVATED] =
    g_signal_new (I_("row-activated"),
		  B_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkTreeViewClass, row_activated),
		  NULL, NULL,
		  _btk_marshal_VOID__BOXED_OBJECT,
		  B_TYPE_NONE, 2,
		  BTK_TYPE_TREE_PATH,
		  BTK_TYPE_TREE_VIEW_COLUMN);

  /**
   * BtkTreeView::test-expand-row:
   * @tree_view: the object on which the signal is emitted
   * @iter: the tree iter of the row to expand
   * @path: a tree path that points to the row 
   * 
   * The given row is about to be expanded (show its children nodes). Use this
   * signal if you need to control the expandability of individual rows.
   *
   * Returns: %FALSE to allow expansion, %TRUE to reject
   */
  tree_view_signals[TEST_EXPAND_ROW] =
    g_signal_new (I_("test-expand-row"),
		  B_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkTreeViewClass, test_expand_row),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED_BOXED,
		  B_TYPE_BOOLEAN, 2,
		  BTK_TYPE_TREE_ITER,
		  BTK_TYPE_TREE_PATH);

  /**
   * BtkTreeView::test-collapse-row:
   * @tree_view: the object on which the signal is emitted
   * @iter: the tree iter of the row to collapse
   * @path: a tree path that points to the row 
   * 
   * The given row is about to be collapsed (hide its children nodes). Use this
   * signal if you need to control the collapsibility of individual rows.
   *
   * Returns: %FALSE to allow collapsing, %TRUE to reject
   */
  tree_view_signals[TEST_COLLAPSE_ROW] =
    g_signal_new (I_("test-collapse-row"),
		  B_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkTreeViewClass, test_collapse_row),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED_BOXED,
		  B_TYPE_BOOLEAN, 2,
		  BTK_TYPE_TREE_ITER,
		  BTK_TYPE_TREE_PATH);

  /**
   * BtkTreeView::row-expanded:
   * @tree_view: the object on which the signal is emitted
   * @iter: the tree iter of the expanded row
   * @path: a tree path that points to the row 
   * 
   * The given row has been expanded (child nodes are shown).
   */
  tree_view_signals[ROW_EXPANDED] =
    g_signal_new (I_("row-expanded"),
		  B_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkTreeViewClass, row_expanded),
		  NULL, NULL,
		  _btk_marshal_VOID__BOXED_BOXED,
		  B_TYPE_NONE, 2,
		  BTK_TYPE_TREE_ITER,
		  BTK_TYPE_TREE_PATH);

  /**
   * BtkTreeView::row-collapsed:
   * @tree_view: the object on which the signal is emitted
   * @iter: the tree iter of the collapsed row
   * @path: a tree path that points to the row 
   * 
   * The given row has been collapsed (child nodes are hidden).
   */
  tree_view_signals[ROW_COLLAPSED] =
    g_signal_new (I_("row-collapsed"),
		  B_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkTreeViewClass, row_collapsed),
		  NULL, NULL,
		  _btk_marshal_VOID__BOXED_BOXED,
		  B_TYPE_NONE, 2,
		  BTK_TYPE_TREE_ITER,
		  BTK_TYPE_TREE_PATH);

  /**
   * BtkTreeView::columns-changed:
   * @tree_view: the object on which the signal is emitted 
   * 
   * The number of columns of the treeview has changed.
   */
  tree_view_signals[COLUMNS_CHANGED] =
    g_signal_new (I_("columns-changed"),
		  B_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkTreeViewClass, columns_changed),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);

  /**
   * BtkTreeView::cursor-changed:
   * @tree_view: the object on which the signal is emitted
   * 
   * The position of the cursor (focused cell) has changed.
   */
  tree_view_signals[CURSOR_CHANGED] =
    g_signal_new (I_("cursor-changed"),
		  B_TYPE_FROM_CLASS (o_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkTreeViewClass, cursor_changed),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);

  tree_view_signals[MOVE_CURSOR] =
    g_signal_new (I_("move-cursor"),
		  B_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkTreeViewClass, move_cursor),
		  NULL, NULL,
		  _btk_marshal_BOOLEAN__ENUM_INT,
		  B_TYPE_BOOLEAN, 2,
		  BTK_TYPE_MOVEMENT_STEP,
		  B_TYPE_INT);

  tree_view_signals[SELECT_ALL] =
    g_signal_new (I_("select-all"),
		  B_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkTreeViewClass, select_all),
		  NULL, NULL,
		  _btk_marshal_BOOLEAN__VOID,
		  B_TYPE_BOOLEAN, 0);

  tree_view_signals[UNSELECT_ALL] =
    g_signal_new (I_("unselect-all"),
		  B_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkTreeViewClass, unselect_all),
		  NULL, NULL,
		  _btk_marshal_BOOLEAN__VOID,
		  B_TYPE_BOOLEAN, 0);

  tree_view_signals[SELECT_CURSOR_ROW] =
    g_signal_new (I_("select-cursor-row"),
		  B_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkTreeViewClass, select_cursor_row),
		  NULL, NULL,
		  _btk_marshal_BOOLEAN__BOOLEAN,
		  B_TYPE_BOOLEAN, 1,
		  B_TYPE_BOOLEAN);

  tree_view_signals[TOGGLE_CURSOR_ROW] =
    g_signal_new (I_("toggle-cursor-row"),
		  B_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkTreeViewClass, toggle_cursor_row),
		  NULL, NULL,
		  _btk_marshal_BOOLEAN__VOID,
		  B_TYPE_BOOLEAN, 0);

  tree_view_signals[EXPAND_COLLAPSE_CURSOR_ROW] =
    g_signal_new (I_("expand-collapse-cursor-row"),
		  B_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkTreeViewClass, expand_collapse_cursor_row),
		  NULL, NULL,
		  _btk_marshal_BOOLEAN__BOOLEAN_BOOLEAN_BOOLEAN,
		  B_TYPE_BOOLEAN, 3,
		  B_TYPE_BOOLEAN,
		  B_TYPE_BOOLEAN,
		  B_TYPE_BOOLEAN);

  tree_view_signals[SELECT_CURSOR_PARENT] =
    g_signal_new (I_("select-cursor-parent"),
		  B_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkTreeViewClass, select_cursor_parent),
		  NULL, NULL,
		  _btk_marshal_BOOLEAN__VOID,
		  B_TYPE_BOOLEAN, 0);

  tree_view_signals[START_INTERACTIVE_SEARCH] =
    g_signal_new (I_("start-interactive-search"),
		  B_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkTreeViewClass, start_interactive_search),
		  NULL, NULL,
		  _btk_marshal_BOOLEAN__VOID,
		  B_TYPE_BOOLEAN, 0);

  /* Key bindings */
  btk_tree_view_add_move_binding (binding_set, BDK_Up, 0, TRUE,
				  BTK_MOVEMENT_DISPLAY_LINES, -1);
  btk_tree_view_add_move_binding (binding_set, BDK_KP_Up, 0, TRUE,
				  BTK_MOVEMENT_DISPLAY_LINES, -1);

  btk_tree_view_add_move_binding (binding_set, BDK_Down, 0, TRUE,
				  BTK_MOVEMENT_DISPLAY_LINES, 1);
  btk_tree_view_add_move_binding (binding_set, BDK_KP_Down, 0, TRUE,
				  BTK_MOVEMENT_DISPLAY_LINES, 1);

  btk_tree_view_add_move_binding (binding_set, BDK_p, BDK_CONTROL_MASK, FALSE,
				  BTK_MOVEMENT_DISPLAY_LINES, -1);

  btk_tree_view_add_move_binding (binding_set, BDK_n, BDK_CONTROL_MASK, FALSE,
				  BTK_MOVEMENT_DISPLAY_LINES, 1);

  btk_tree_view_add_move_binding (binding_set, BDK_Home, 0, TRUE,
				  BTK_MOVEMENT_BUFFER_ENDS, -1);
  btk_tree_view_add_move_binding (binding_set, BDK_KP_Home, 0, TRUE,
				  BTK_MOVEMENT_BUFFER_ENDS, -1);

  btk_tree_view_add_move_binding (binding_set, BDK_End, 0, TRUE,
				  BTK_MOVEMENT_BUFFER_ENDS, 1);
  btk_tree_view_add_move_binding (binding_set, BDK_KP_End, 0, TRUE,
				  BTK_MOVEMENT_BUFFER_ENDS, 1);

  btk_tree_view_add_move_binding (binding_set, BDK_Page_Up, 0, TRUE,
				  BTK_MOVEMENT_PAGES, -1);
  btk_tree_view_add_move_binding (binding_set, BDK_KP_Page_Up, 0, TRUE,
				  BTK_MOVEMENT_PAGES, -1);

  btk_tree_view_add_move_binding (binding_set, BDK_Page_Down, 0, TRUE,
				  BTK_MOVEMENT_PAGES, 1);
  btk_tree_view_add_move_binding (binding_set, BDK_KP_Page_Down, 0, TRUE,
				  BTK_MOVEMENT_PAGES, 1);


  btk_binding_entry_add_signal (binding_set, BDK_Right, 0, "move-cursor", 2,
				B_TYPE_ENUM, BTK_MOVEMENT_VISUAL_POSITIONS,
				B_TYPE_INT, 1);

  btk_binding_entry_add_signal (binding_set, BDK_Left, 0, "move-cursor", 2,
				B_TYPE_ENUM, BTK_MOVEMENT_VISUAL_POSITIONS,
				B_TYPE_INT, -1);

  btk_binding_entry_add_signal (binding_set, BDK_KP_Right, 0, "move-cursor", 2,
				B_TYPE_ENUM, BTK_MOVEMENT_VISUAL_POSITIONS,
				B_TYPE_INT, 1);

  btk_binding_entry_add_signal (binding_set, BDK_KP_Left, 0, "move-cursor", 2,
				B_TYPE_ENUM, BTK_MOVEMENT_VISUAL_POSITIONS,
				B_TYPE_INT, -1);

  btk_binding_entry_add_signal (binding_set, BDK_Right, BDK_CONTROL_MASK,
                                "move-cursor", 2,
				B_TYPE_ENUM, BTK_MOVEMENT_VISUAL_POSITIONS,
				B_TYPE_INT, 1);

  btk_binding_entry_add_signal (binding_set, BDK_Left, BDK_CONTROL_MASK,
                                "move-cursor", 2,
				B_TYPE_ENUM, BTK_MOVEMENT_VISUAL_POSITIONS,
				B_TYPE_INT, -1);

  btk_binding_entry_add_signal (binding_set, BDK_KP_Right, BDK_CONTROL_MASK,
                                "move-cursor", 2,
				B_TYPE_ENUM, BTK_MOVEMENT_VISUAL_POSITIONS,
				B_TYPE_INT, 1);

  btk_binding_entry_add_signal (binding_set, BDK_KP_Left, BDK_CONTROL_MASK,
                                "move-cursor", 2,
				B_TYPE_ENUM, BTK_MOVEMENT_VISUAL_POSITIONS,
				B_TYPE_INT, -1);

  btk_binding_entry_add_signal (binding_set, BDK_space, BDK_CONTROL_MASK, "toggle-cursor-row", 0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Space, BDK_CONTROL_MASK, "toggle-cursor-row", 0);

  btk_binding_entry_add_signal (binding_set, BDK_a, BDK_CONTROL_MASK, "select-all", 0);
  btk_binding_entry_add_signal (binding_set, BDK_slash, BDK_CONTROL_MASK, "select-all", 0);

  btk_binding_entry_add_signal (binding_set, BDK_A, BDK_CONTROL_MASK | BDK_SHIFT_MASK, "unselect-all", 0);
  btk_binding_entry_add_signal (binding_set, BDK_backslash, BDK_CONTROL_MASK, "unselect-all", 0);

  btk_binding_entry_add_signal (binding_set, BDK_space, BDK_SHIFT_MASK, "select-cursor-row", 1,
				B_TYPE_BOOLEAN, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Space, BDK_SHIFT_MASK, "select-cursor-row", 1,
				B_TYPE_BOOLEAN, TRUE);

  btk_binding_entry_add_signal (binding_set, BDK_space, 0, "select-cursor-row", 1,
				B_TYPE_BOOLEAN, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Space, 0, "select-cursor-row", 1,
				B_TYPE_BOOLEAN, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_Return, 0, "select-cursor-row", 1,
				B_TYPE_BOOLEAN, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_ISO_Enter, 0, "select-cursor-row", 1,
				B_TYPE_BOOLEAN, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Enter, 0, "select-cursor-row", 1,
				B_TYPE_BOOLEAN, TRUE);

  /* expand and collapse rows */
  btk_binding_entry_add_signal (binding_set, BDK_plus, 0, "expand-collapse-cursor-row", 3,
				B_TYPE_BOOLEAN, TRUE,
				B_TYPE_BOOLEAN, TRUE,
				B_TYPE_BOOLEAN, FALSE);

  btk_binding_entry_add_signal (binding_set, BDK_asterisk, 0,
                                "expand-collapse-cursor-row", 3,
                                B_TYPE_BOOLEAN, TRUE,
                                B_TYPE_BOOLEAN, TRUE,
                                B_TYPE_BOOLEAN, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Multiply, 0,
                                "expand-collapse-cursor-row", 3,
                                B_TYPE_BOOLEAN, TRUE,
                                B_TYPE_BOOLEAN, TRUE,
                                B_TYPE_BOOLEAN, TRUE);

  btk_binding_entry_add_signal (binding_set, BDK_slash, 0,
                                "expand-collapse-cursor-row", 3,
                                B_TYPE_BOOLEAN, TRUE,
                                B_TYPE_BOOLEAN, FALSE,
                                B_TYPE_BOOLEAN, FALSE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Divide, 0,
                                "expand-collapse-cursor-row", 3,
                                B_TYPE_BOOLEAN, TRUE,
                                B_TYPE_BOOLEAN, FALSE,
                                B_TYPE_BOOLEAN, FALSE);

  /* Not doable on US keyboards */
  btk_binding_entry_add_signal (binding_set, BDK_plus, BDK_SHIFT_MASK, "expand-collapse-cursor-row", 3,
				B_TYPE_BOOLEAN, TRUE,
				B_TYPE_BOOLEAN, TRUE,
				B_TYPE_BOOLEAN, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Add, 0, "expand-collapse-cursor-row", 3,
				B_TYPE_BOOLEAN, TRUE,
				B_TYPE_BOOLEAN, TRUE,
				B_TYPE_BOOLEAN, FALSE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Add, BDK_SHIFT_MASK, "expand-collapse-cursor-row", 3,
				B_TYPE_BOOLEAN, TRUE,
				B_TYPE_BOOLEAN, TRUE,
				B_TYPE_BOOLEAN, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Add, BDK_SHIFT_MASK, "expand-collapse-cursor-row", 3,
				B_TYPE_BOOLEAN, TRUE,
				B_TYPE_BOOLEAN, TRUE,
				B_TYPE_BOOLEAN, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_Right, BDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				B_TYPE_BOOLEAN, FALSE,
				B_TYPE_BOOLEAN, TRUE,
				B_TYPE_BOOLEAN, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Right, BDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				B_TYPE_BOOLEAN, FALSE,
				B_TYPE_BOOLEAN, TRUE,
				B_TYPE_BOOLEAN, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_Right,
                                BDK_CONTROL_MASK | BDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				B_TYPE_BOOLEAN, FALSE,
				B_TYPE_BOOLEAN, TRUE,
				B_TYPE_BOOLEAN, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Right,
                                BDK_CONTROL_MASK | BDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				B_TYPE_BOOLEAN, FALSE,
				B_TYPE_BOOLEAN, TRUE,
				B_TYPE_BOOLEAN, TRUE);

  btk_binding_entry_add_signal (binding_set, BDK_minus, 0, "expand-collapse-cursor-row", 3,
				B_TYPE_BOOLEAN, TRUE,
				B_TYPE_BOOLEAN, FALSE,
				B_TYPE_BOOLEAN, FALSE);
  btk_binding_entry_add_signal (binding_set, BDK_minus, BDK_SHIFT_MASK, "expand-collapse-cursor-row", 3,
				B_TYPE_BOOLEAN, TRUE,
				B_TYPE_BOOLEAN, FALSE,
				B_TYPE_BOOLEAN, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Subtract, 0, "expand-collapse-cursor-row", 3,
				B_TYPE_BOOLEAN, TRUE,
				B_TYPE_BOOLEAN, FALSE,
				B_TYPE_BOOLEAN, FALSE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Subtract, BDK_SHIFT_MASK, "expand-collapse-cursor-row", 3,
				B_TYPE_BOOLEAN, TRUE,
				B_TYPE_BOOLEAN, FALSE,
				B_TYPE_BOOLEAN, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_Left, BDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				B_TYPE_BOOLEAN, FALSE,
				B_TYPE_BOOLEAN, FALSE,
				B_TYPE_BOOLEAN, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Left, BDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				B_TYPE_BOOLEAN, FALSE,
				B_TYPE_BOOLEAN, FALSE,
				B_TYPE_BOOLEAN, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_Left,
                                BDK_CONTROL_MASK | BDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				B_TYPE_BOOLEAN, FALSE,
				B_TYPE_BOOLEAN, FALSE,
				B_TYPE_BOOLEAN, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Left,
                                BDK_CONTROL_MASK | BDK_SHIFT_MASK,
                                "expand-collapse-cursor-row", 3,
				B_TYPE_BOOLEAN, FALSE,
				B_TYPE_BOOLEAN, FALSE,
				B_TYPE_BOOLEAN, TRUE);

  btk_binding_entry_add_signal (binding_set, BDK_BackSpace, 0, "select-cursor-parent", 0);
  btk_binding_entry_add_signal (binding_set, BDK_BackSpace, BDK_CONTROL_MASK, "select-cursor-parent", 0);

  btk_binding_entry_add_signal (binding_set, BDK_f, BDK_CONTROL_MASK, "start-interactive-search", 0);

  btk_binding_entry_add_signal (binding_set, BDK_F, BDK_CONTROL_MASK, "start-interactive-search", 0);

  g_type_class_add_private (o_class, sizeof (BtkTreeViewPrivate));
}

static void
btk_tree_view_init (BtkTreeView *tree_view)
{
  tree_view->priv = B_TYPE_INSTANCE_GET_PRIVATE (tree_view, BTK_TYPE_TREE_VIEW, BtkTreeViewPrivate);

  btk_widget_set_can_focus (BTK_WIDGET (tree_view), TRUE);
  btk_widget_set_redraw_on_allocate (BTK_WIDGET (tree_view), FALSE);

  tree_view->priv->flags =  BTK_TREE_VIEW_SHOW_EXPANDERS
                            | BTK_TREE_VIEW_DRAW_KEYFOCUS
                            | BTK_TREE_VIEW_HEADERS_VISIBLE;

  /* We need some padding */
  tree_view->priv->dy = 0;
  tree_view->priv->cursor_offset = 0;
  tree_view->priv->n_columns = 0;
  tree_view->priv->header_height = 1;
  tree_view->priv->x_drag = 0;
  tree_view->priv->drag_pos = -1;
  tree_view->priv->header_has_focus = FALSE;
  tree_view->priv->pressed_button = -1;
  tree_view->priv->press_start_x = -1;
  tree_view->priv->press_start_y = -1;
  tree_view->priv->reorderable = FALSE;
  tree_view->priv->presize_handler_timer = 0;
  tree_view->priv->scroll_sync_timer = 0;
  tree_view->priv->fixed_height = -1;
  tree_view->priv->fixed_height_mode = FALSE;
  tree_view->priv->fixed_height_check = 0;
  btk_tree_view_set_adjustments (tree_view, NULL, NULL);
  tree_view->priv->selection = _btk_tree_selection_new_with_tree_view (tree_view);
  tree_view->priv->enable_search = TRUE;
  tree_view->priv->search_column = -1;
  tree_view->priv->search_position_func = btk_tree_view_search_position_func;
  tree_view->priv->search_equal_func = btk_tree_view_search_equal_func;
  tree_view->priv->search_custom_entry_set = FALSE;
  tree_view->priv->typeselect_flush_timeout = 0;
  tree_view->priv->init_hadjust_value = TRUE;    
  tree_view->priv->width = 0;
          
  tree_view->priv->hover_selection = FALSE;
  tree_view->priv->hover_expand = FALSE;

  tree_view->priv->level_indentation = 0;

  tree_view->priv->rubber_banding_enable = FALSE;

  tree_view->priv->grid_lines = BTK_TREE_VIEW_GRID_LINES_NONE;
  tree_view->priv->tree_lines_enabled = FALSE;

  tree_view->priv->tooltip_column = -1;

  tree_view->priv->post_validation_flag = FALSE;

  tree_view->priv->last_button_x = -1;
  tree_view->priv->last_button_y = -1;

  tree_view->priv->event_last_x = -10000;
  tree_view->priv->event_last_y = -10000;
}



/* BObject Methods
 */

static void
btk_tree_view_set_property (BObject         *object,
			    guint            prop_id,
			    const BValue    *value,
			    BParamSpec      *pspec)
{
  BtkTreeView *tree_view;

  tree_view = BTK_TREE_VIEW (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      btk_tree_view_set_model (tree_view, b_value_get_object (value));
      break;
    case PROP_HADJUSTMENT:
      btk_tree_view_set_hadjustment (tree_view, b_value_get_object (value));
      break;
    case PROP_VADJUSTMENT:
      btk_tree_view_set_vadjustment (tree_view, b_value_get_object (value));
      break;
    case PROP_HEADERS_VISIBLE:
      btk_tree_view_set_headers_visible (tree_view, b_value_get_boolean (value));
      break;
    case PROP_HEADERS_CLICKABLE:
      btk_tree_view_set_headers_clickable (tree_view, b_value_get_boolean (value));
      break;
    case PROP_EXPANDER_COLUMN:
      btk_tree_view_set_expander_column (tree_view, b_value_get_object (value));
      break;
    case PROP_REORDERABLE:
      btk_tree_view_set_reorderable (tree_view, b_value_get_boolean (value));
      break;
    case PROP_RULES_HINT:
      btk_tree_view_set_rules_hint (tree_view, b_value_get_boolean (value));
      break;
    case PROP_ENABLE_SEARCH:
      btk_tree_view_set_enable_search (tree_view, b_value_get_boolean (value));
      break;
    case PROP_SEARCH_COLUMN:
      btk_tree_view_set_search_column (tree_view, b_value_get_int (value));
      break;
    case PROP_FIXED_HEIGHT_MODE:
      btk_tree_view_set_fixed_height_mode (tree_view, b_value_get_boolean (value));
      break;
    case PROP_HOVER_SELECTION:
      tree_view->priv->hover_selection = b_value_get_boolean (value);
      break;
    case PROP_HOVER_EXPAND:
      tree_view->priv->hover_expand = b_value_get_boolean (value);
      break;
    case PROP_SHOW_EXPANDERS:
      btk_tree_view_set_show_expanders (tree_view, b_value_get_boolean (value));
      break;
    case PROP_LEVEL_INDENTATION:
      tree_view->priv->level_indentation = b_value_get_int (value);
      break;
    case PROP_RUBBER_BANDING:
      tree_view->priv->rubber_banding_enable = b_value_get_boolean (value);
      break;
    case PROP_ENABLE_GRID_LINES:
      btk_tree_view_set_grid_lines (tree_view, b_value_get_enum (value));
      break;
    case PROP_ENABLE_TREE_LINES:
      btk_tree_view_set_enable_tree_lines (tree_view, b_value_get_boolean (value));
      break;
    case PROP_TOOLTIP_COLUMN:
      btk_tree_view_set_tooltip_column (tree_view, b_value_get_int (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_tree_view_get_property (BObject    *object,
			    guint       prop_id,
			    BValue     *value,
			    BParamSpec *pspec)
{
  BtkTreeView *tree_view;

  tree_view = BTK_TREE_VIEW (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      b_value_set_object (value, tree_view->priv->model);
      break;
    case PROP_HADJUSTMENT:
      b_value_set_object (value, tree_view->priv->hadjustment);
      break;
    case PROP_VADJUSTMENT:
      b_value_set_object (value, tree_view->priv->vadjustment);
      break;
    case PROP_HEADERS_VISIBLE:
      b_value_set_boolean (value, btk_tree_view_get_headers_visible (tree_view));
      break;
    case PROP_HEADERS_CLICKABLE:
      b_value_set_boolean (value, btk_tree_view_get_headers_clickable (tree_view));
      break;
    case PROP_EXPANDER_COLUMN:
      b_value_set_object (value, tree_view->priv->expander_column);
      break;
    case PROP_REORDERABLE:
      b_value_set_boolean (value, tree_view->priv->reorderable);
      break;
    case PROP_RULES_HINT:
      b_value_set_boolean (value, tree_view->priv->has_rules);
      break;
    case PROP_ENABLE_SEARCH:
      b_value_set_boolean (value, tree_view->priv->enable_search);
      break;
    case PROP_SEARCH_COLUMN:
      b_value_set_int (value, tree_view->priv->search_column);
      break;
    case PROP_FIXED_HEIGHT_MODE:
      b_value_set_boolean (value, tree_view->priv->fixed_height_mode);
      break;
    case PROP_HOVER_SELECTION:
      b_value_set_boolean (value, tree_view->priv->hover_selection);
      break;
    case PROP_HOVER_EXPAND:
      b_value_set_boolean (value, tree_view->priv->hover_expand);
      break;
    case PROP_SHOW_EXPANDERS:
      b_value_set_boolean (value, BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_SHOW_EXPANDERS));
      break;
    case PROP_LEVEL_INDENTATION:
      b_value_set_int (value, tree_view->priv->level_indentation);
      break;
    case PROP_RUBBER_BANDING:
      b_value_set_boolean (value, tree_view->priv->rubber_banding_enable);
      break;
    case PROP_ENABLE_GRID_LINES:
      b_value_set_enum (value, tree_view->priv->grid_lines);
      break;
    case PROP_ENABLE_TREE_LINES:
      b_value_set_boolean (value, tree_view->priv->tree_lines_enabled);
      break;
    case PROP_TOOLTIP_COLUMN:
      b_value_set_int (value, tree_view->priv->tooltip_column);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_tree_view_finalize (BObject *object)
{
  B_OBJECT_CLASS (btk_tree_view_parent_class)->finalize (object);
}


static BtkBuildableIface *parent_buildable_iface;

static void
btk_tree_view_buildable_init (BtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = btk_tree_view_buildable_add_child;
  iface->get_internal_child = btk_tree_view_buildable_get_internal_child;
}

static void
btk_tree_view_buildable_add_child (BtkBuildable *tree_view,
				   BtkBuilder  *builder,
				   BObject     *child,
				   const gchar *type)
{
  btk_tree_view_append_column (BTK_TREE_VIEW (tree_view), BTK_TREE_VIEW_COLUMN (child));
}

static BObject *
btk_tree_view_buildable_get_internal_child (BtkBuildable      *buildable,
					    BtkBuilder        *builder,
					    const gchar       *childname)
{
    if (strcmp (childname, "selection") == 0)
      return B_OBJECT (BTK_TREE_VIEW (buildable)->priv->selection);
    
    return parent_buildable_iface->get_internal_child (buildable,
						       builder,
						       childname);
}

/* BtkObject Methods
 */

static void
btk_tree_view_free_rbtree (BtkTreeView *tree_view)
{
  _btk_rbtree_free (tree_view->priv->tree);
  
  tree_view->priv->tree = NULL;
  tree_view->priv->button_pressed_node = NULL;
  tree_view->priv->button_pressed_tree = NULL;
  tree_view->priv->prelight_tree = NULL;
  tree_view->priv->prelight_node = NULL;
  tree_view->priv->expanded_collapsed_node = NULL;
  tree_view->priv->expanded_collapsed_tree = NULL;
}

static void
btk_tree_view_destroy (BtkObject *object)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (object);
  GList *list;

  btk_tree_view_stop_editing (tree_view, TRUE);

  if (tree_view->priv->columns != NULL)
    {
      list = tree_view->priv->columns;
      while (list)
	{
	  BtkTreeViewColumn *column;
	  column = BTK_TREE_VIEW_COLUMN (list->data);
	  list = list->next;
	  btk_tree_view_remove_column (tree_view, column);
	}
      tree_view->priv->columns = NULL;
    }

  if (tree_view->priv->tree != NULL)
    {
      btk_tree_view_unref_and_check_selection_tree (tree_view, tree_view->priv->tree);

      btk_tree_view_free_rbtree (tree_view);
    }

  if (tree_view->priv->selection != NULL)
    {
      _btk_tree_selection_set_tree_view (tree_view->priv->selection, NULL);
      g_object_unref (tree_view->priv->selection);
      tree_view->priv->selection = NULL;
    }

  if (tree_view->priv->scroll_to_path != NULL)
    {
      btk_tree_row_reference_free (tree_view->priv->scroll_to_path);
      tree_view->priv->scroll_to_path = NULL;
    }

  if (tree_view->priv->drag_dest_row != NULL)
    {
      btk_tree_row_reference_free (tree_view->priv->drag_dest_row);
      tree_view->priv->drag_dest_row = NULL;
    }

  if (tree_view->priv->top_row != NULL)
    {
      btk_tree_row_reference_free (tree_view->priv->top_row);
      tree_view->priv->top_row = NULL;
    }

  if (tree_view->priv->column_drop_func_data &&
      tree_view->priv->column_drop_func_data_destroy)
    {
      tree_view->priv->column_drop_func_data_destroy (tree_view->priv->column_drop_func_data);
      tree_view->priv->column_drop_func_data = NULL;
    }

  if (tree_view->priv->destroy_count_destroy &&
      tree_view->priv->destroy_count_data)
    {
      tree_view->priv->destroy_count_destroy (tree_view->priv->destroy_count_data);
      tree_view->priv->destroy_count_data = NULL;
    }

  btk_tree_row_reference_free (tree_view->priv->cursor);
  tree_view->priv->cursor = NULL;

  btk_tree_row_reference_free (tree_view->priv->anchor);
  tree_view->priv->anchor = NULL;

  /* destroy interactive search dialog */
  if (tree_view->priv->search_window)
    {
      btk_widget_destroy (tree_view->priv->search_window);
      tree_view->priv->search_window = NULL;
      tree_view->priv->search_entry = NULL;
      if (tree_view->priv->typeselect_flush_timeout)
	{
	  g_source_remove (tree_view->priv->typeselect_flush_timeout);
	  tree_view->priv->typeselect_flush_timeout = 0;
	}
    }

  if (tree_view->priv->search_destroy && tree_view->priv->search_user_data)
    {
      tree_view->priv->search_destroy (tree_view->priv->search_user_data);
      tree_view->priv->search_user_data = NULL;
    }

  if (tree_view->priv->search_position_destroy && tree_view->priv->search_position_user_data)
    {
      tree_view->priv->search_position_destroy (tree_view->priv->search_position_user_data);
      tree_view->priv->search_position_user_data = NULL;
    }

  if (tree_view->priv->row_separator_destroy && tree_view->priv->row_separator_data)
    {
      tree_view->priv->row_separator_destroy (tree_view->priv->row_separator_data);
      tree_view->priv->row_separator_data = NULL;
    }
  
  btk_tree_view_set_model (tree_view, NULL);

  if (tree_view->priv->hadjustment)
    {
      g_object_unref (tree_view->priv->hadjustment);
      tree_view->priv->hadjustment = NULL;
    }
  if (tree_view->priv->vadjustment)
    {
      g_object_unref (tree_view->priv->vadjustment);
      tree_view->priv->vadjustment = NULL;
    }

  BTK_OBJECT_CLASS (btk_tree_view_parent_class)->destroy (object);
}



/* BtkWidget Methods
 */

/* BtkWidget::map helper */
static void
btk_tree_view_map_buttons (BtkTreeView *tree_view)
{
  GList *list;

  g_return_if_fail (btk_widget_get_mapped (BTK_WIDGET (tree_view)));

  if (BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_HEADERS_VISIBLE))
    {
      BtkTreeViewColumn *column;

      for (list = tree_view->priv->columns; list; list = list->next)
	{
	  column = list->data;
          if (btk_widget_get_visible (column->button) &&
              !btk_widget_get_mapped (column->button))
            btk_widget_map (column->button);
	}
      for (list = tree_view->priv->columns; list; list = list->next)
	{
	  column = list->data;
	  if (column->visible == FALSE)
	    continue;
	  if (column->resizable)
	    {
	      bdk_window_raise (column->window);
	      bdk_window_show (column->window);
	    }
	  else
	    bdk_window_hide (column->window);
	}
      bdk_window_show (tree_view->priv->header_window);
    }
}

static void
btk_tree_view_map (BtkWidget *widget)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (widget);
  GList *tmp_list;

  btk_widget_set_mapped (widget, TRUE);

  tmp_list = tree_view->priv->children;
  while (tmp_list)
    {
      BtkTreeViewChild *child = tmp_list->data;
      tmp_list = tmp_list->next;

      if (btk_widget_get_visible (child->widget))
	{
	  if (!btk_widget_get_mapped (child->widget))
	    btk_widget_map (child->widget);
	}
    }
  bdk_window_show (tree_view->priv->bin_window);

  btk_tree_view_map_buttons (tree_view);

  bdk_window_show (widget->window);
}

static void
btk_tree_view_realize (BtkWidget *widget)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (widget);
  GList *tmp_list;
  BdkWindowAttr attributes;
  gint attributes_mask;

  btk_widget_set_realized (widget, TRUE);

  /* Make the main, clipping window */
  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  attributes.event_mask = BDK_VISIBILITY_NOTIFY_MASK;

  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;

  widget->window = bdk_window_new (btk_widget_get_parent_window (widget),
				   &attributes, attributes_mask);
  bdk_window_set_user_data (widget->window, widget);

  /* Make the window for the tree */
  attributes.x = 0;
  attributes.y = TREE_VIEW_HEADER_HEIGHT (tree_view);
  attributes.width = MAX (tree_view->priv->width, widget->allocation.width);
  attributes.height = widget->allocation.height;
  attributes.event_mask = (BDK_EXPOSURE_MASK |
                           BDK_SCROLL_MASK |
                           BDK_POINTER_MOTION_MASK |
                           BDK_ENTER_NOTIFY_MASK |
                           BDK_LEAVE_NOTIFY_MASK |
                           BDK_BUTTON_PRESS_MASK |
                           BDK_BUTTON_RELEASE_MASK |
                           btk_widget_get_events (widget));

  tree_view->priv->bin_window = bdk_window_new (widget->window,
						&attributes, attributes_mask);
  bdk_window_set_user_data (tree_view->priv->bin_window, widget);

  /* Make the column header window */
  attributes.x = 0;
  attributes.y = 0;
  attributes.width = MAX (tree_view->priv->width, widget->allocation.width);
  attributes.height = tree_view->priv->header_height;
  attributes.event_mask = (BDK_EXPOSURE_MASK |
                           BDK_SCROLL_MASK |
                           BDK_ENTER_NOTIFY_MASK |
                           BDK_LEAVE_NOTIFY_MASK |
                           BDK_BUTTON_PRESS_MASK |
                           BDK_BUTTON_RELEASE_MASK |
                           BDK_KEY_PRESS_MASK |
                           BDK_KEY_RELEASE_MASK |
                           btk_widget_get_events (widget));

  tree_view->priv->header_window = bdk_window_new (widget->window,
						   &attributes, attributes_mask);
  bdk_window_set_user_data (tree_view->priv->header_window, widget);

  /* Add them all up. */
  widget->style = btk_style_attach (widget->style, widget->window);
  bdk_window_set_back_pixmap (widget->window, NULL, FALSE);
  bdk_window_set_background (tree_view->priv->bin_window, &widget->style->base[widget->state]);
  btk_style_set_background (widget->style, tree_view->priv->header_window, BTK_STATE_NORMAL);

  tmp_list = tree_view->priv->children;
  while (tmp_list)
    {
      BtkTreeViewChild *child = tmp_list->data;
      tmp_list = tmp_list->next;

      btk_widget_set_parent_window (child->widget, tree_view->priv->bin_window);
    }

  for (tmp_list = tree_view->priv->columns; tmp_list; tmp_list = tmp_list->next)
    _btk_tree_view_column_realize_button (BTK_TREE_VIEW_COLUMN (tmp_list->data));

  /* Need to call those here, since they create GCs */
  btk_tree_view_set_grid_lines (tree_view, tree_view->priv->grid_lines);
  btk_tree_view_set_enable_tree_lines (tree_view, tree_view->priv->tree_lines_enabled);

  install_presize_handler (tree_view); 
}

static void
btk_tree_view_unrealize (BtkWidget *widget)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (widget);
  BtkTreeViewPrivate *priv = tree_view->priv;
  GList *list;

  if (priv->scroll_timeout != 0)
    {
      g_source_remove (priv->scroll_timeout);
      priv->scroll_timeout = 0;
    }

  if (priv->auto_expand_timeout != 0)
    {
      g_source_remove (priv->auto_expand_timeout);
      priv->auto_expand_timeout = 0;
    }

  if (priv->open_dest_timeout != 0)
    {
      g_source_remove (priv->open_dest_timeout);
      priv->open_dest_timeout = 0;
    }

  remove_expand_collapse_timeout (tree_view);
  
  if (priv->presize_handler_timer != 0)
    {
      g_source_remove (priv->presize_handler_timer);
      priv->presize_handler_timer = 0;
    }

  if (priv->validate_rows_timer != 0)
    {
      g_source_remove (priv->validate_rows_timer);
      priv->validate_rows_timer = 0;
    }

  if (priv->scroll_sync_timer != 0)
    {
      g_source_remove (priv->scroll_sync_timer);
      priv->scroll_sync_timer = 0;
    }

  if (priv->typeselect_flush_timeout)
    {
      g_source_remove (priv->typeselect_flush_timeout);
      priv->typeselect_flush_timeout = 0;
    }
  
  for (list = priv->columns; list; list = list->next)
    _btk_tree_view_column_unrealize_button (BTK_TREE_VIEW_COLUMN (list->data));

  bdk_window_set_user_data (priv->bin_window, NULL);
  bdk_window_destroy (priv->bin_window);
  priv->bin_window = NULL;

  bdk_window_set_user_data (priv->header_window, NULL);
  bdk_window_destroy (priv->header_window);
  priv->header_window = NULL;

  if (priv->drag_window)
    {
      bdk_window_set_user_data (priv->drag_window, NULL);
      bdk_window_destroy (priv->drag_window);
      priv->drag_window = NULL;
    }

  if (priv->drag_highlight_window)
    {
      bdk_window_set_user_data (priv->drag_highlight_window, NULL);
      bdk_window_destroy (priv->drag_highlight_window);
      priv->drag_highlight_window = NULL;
    }

  BTK_WIDGET_CLASS (btk_tree_view_parent_class)->unrealize (widget);
}

/* BtkWidget::size_request helper */
static void
btk_tree_view_size_request_columns (BtkTreeView *tree_view)
{
  GList *list;

  tree_view->priv->header_height = 0;

  if (tree_view->priv->model)
    {
      for (list = tree_view->priv->columns; list; list = list->next)
        {
          BtkRequisition requisition;
          BtkTreeViewColumn *column = list->data;

	  if (column->button == NULL)
	    continue;

          column = list->data;
	  
          btk_widget_size_request (column->button, &requisition);
	  column->button_request = requisition.width;
          tree_view->priv->header_height = MAX (tree_view->priv->header_height, requisition.height);
        }
    }
}


/* Called only by ::size_request */
static void
btk_tree_view_update_size (BtkTreeView *tree_view)
{
  GList *list;
  BtkTreeViewColumn *column;
  gint i;

  if (tree_view->priv->model == NULL)
    {
      tree_view->priv->width = 0;
      tree_view->priv->prev_width = 0;                   
      tree_view->priv->height = 0;
      return;
    }

  tree_view->priv->prev_width = tree_view->priv->width;  
  tree_view->priv->width = 0;

  /* keep this in sync with size_allocate below */
  for (list = tree_view->priv->columns, i = 0; list; list = list->next, i++)
    {
      gint real_requested_width = 0;
      column = list->data;
      if (!column->visible)
	continue;

      if (column->use_resized_width)
	{
	  real_requested_width = column->resized_width;
	}
      else if (column->column_type == BTK_TREE_VIEW_COLUMN_FIXED)
	{
	  real_requested_width = column->fixed_width;
	}
      else if (BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_HEADERS_VISIBLE))
	{
	  real_requested_width = MAX (column->requested_width, column->button_request);
	}
      else
	{
	  real_requested_width = column->requested_width;
	}

      if (column->min_width != -1)
	real_requested_width = MAX (real_requested_width, column->min_width);
      if (column->max_width != -1)
	real_requested_width = MIN (real_requested_width, column->max_width);

      tree_view->priv->width += real_requested_width;
    }

  if (tree_view->priv->tree == NULL)
    tree_view->priv->height = 0;
  else
    tree_view->priv->height = tree_view->priv->tree->root->offset;
}

static void
btk_tree_view_size_request (BtkWidget      *widget,
			    BtkRequisition *requisition)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (widget);
  GList *tmp_list;

  /* we validate some rows initially just to make sure we have some size. 
   * In practice, with a lot of static lists, this should get a good width.
   */
  do_validate_rows (tree_view, FALSE);
  btk_tree_view_size_request_columns (tree_view);
  btk_tree_view_update_size (BTK_TREE_VIEW (widget));

  requisition->width = tree_view->priv->width;
  requisition->height = tree_view->priv->height + TREE_VIEW_HEADER_HEIGHT (tree_view);

  tmp_list = tree_view->priv->children;

  while (tmp_list)
    {
      BtkTreeViewChild *child = tmp_list->data;
      BtkRequisition child_requisition;

      tmp_list = tmp_list->next;

      if (btk_widget_get_visible (child->widget))
        btk_widget_size_request (child->widget, &child_requisition);
    }
}

static int
btk_tree_view_calculate_width_before_expander (BtkTreeView *tree_view)
{
  int width = 0;
  GList *list;
  gboolean rtl;

  rtl = (btk_widget_get_direction (BTK_WIDGET (tree_view)) == BTK_TEXT_DIR_RTL);
  for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
       list->data != tree_view->priv->expander_column;
       list = (rtl ? list->prev : list->next))
    {
      BtkTreeViewColumn *column = list->data;

      width += column->width;
    }

  return width;
}

static void
invalidate_column (BtkTreeView       *tree_view,
                   BtkTreeViewColumn *column)
{
  gint column_offset = 0;
  GList *list;
  BtkWidget *widget = BTK_WIDGET (tree_view);
  gboolean rtl;

  if (!btk_widget_get_realized (widget))
    return;

  rtl = (btk_widget_get_direction (BTK_WIDGET (tree_view)) == BTK_TEXT_DIR_RTL);
  for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
       list;
       list = (rtl ? list->prev : list->next))
    {
      BtkTreeViewColumn *tmpcolumn = list->data;
      if (tmpcolumn == column)
	{
	  BdkRectangle invalid_rect;
	  
	  invalid_rect.x = column_offset;
	  invalid_rect.y = 0;
	  invalid_rect.width = column->width;
	  invalid_rect.height = widget->allocation.height;
	  
	  bdk_window_invalidate_rect (widget->window, &invalid_rect, TRUE);
	  break;
	}
      
      column_offset += tmpcolumn->width;
    }
}

static void
invalidate_last_column (BtkTreeView *tree_view)
{
  GList *last_column;
  gboolean rtl;

  rtl = (btk_widget_get_direction (BTK_WIDGET (tree_view)) == BTK_TEXT_DIR_RTL);

  for (last_column = (rtl ? g_list_first (tree_view->priv->columns) : g_list_last (tree_view->priv->columns));
       last_column;
       last_column = (rtl ? last_column->next : last_column->prev))
    {
      if (BTK_TREE_VIEW_COLUMN (last_column->data)->visible)
        {
          invalidate_column (tree_view, last_column->data);
          return;
        }
    }
}

static gint
btk_tree_view_get_real_requested_width_from_column (BtkTreeView       *tree_view,
                                                    BtkTreeViewColumn *column)
{
  gint real_requested_width;

  if (column->use_resized_width)
    {
      real_requested_width = column->resized_width;
    }
  else if (column->column_type == BTK_TREE_VIEW_COLUMN_FIXED)
    {
      real_requested_width = column->fixed_width;
    }
  else if (BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_HEADERS_VISIBLE))
    {
      real_requested_width = MAX (column->requested_width, column->button_request);
    }
  else
    {
      real_requested_width = column->requested_width;
      if (real_requested_width < 0)
        real_requested_width = 0;
    }

  if (column->min_width != -1)
    real_requested_width = MAX (real_requested_width, column->min_width);
  if (column->max_width != -1)
    real_requested_width = MIN (real_requested_width, column->max_width);

  return real_requested_width;
}

/* BtkWidget::size_allocate helper */
static void
btk_tree_view_size_allocate_columns (BtkWidget *widget,
				     gboolean  *width_changed)
{
  BtkTreeView *tree_view;
  GList *list, *first_column, *last_column;
  BtkTreeViewColumn *column;
  BtkAllocation allocation;
  gint width = 0;
  gint extra, extra_per_column, extra_for_last;
  gint full_requested_width = 0;
  gint number_of_expand_columns = 0;
  gboolean column_changed = FALSE;
  gboolean rtl;
  gboolean update_expand;
  
  tree_view = BTK_TREE_VIEW (widget);

  for (last_column = g_list_last (tree_view->priv->columns);
       last_column && !(BTK_TREE_VIEW_COLUMN (last_column->data)->visible);
       last_column = last_column->prev)
    ;
  if (last_column == NULL)
    return;

  for (first_column = g_list_first (tree_view->priv->columns);
       first_column && !(BTK_TREE_VIEW_COLUMN (first_column->data)->visible);
       first_column = first_column->next)
    ;

  allocation.y = 0;
  allocation.height = tree_view->priv->header_height;

  rtl = (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL);

  /* find out how many extra space and expandable columns we have */
  for (list = tree_view->priv->columns; list != last_column->next; list = list->next)
    {
      column = (BtkTreeViewColumn *)list->data;

      if (!column->visible)
	continue;

      full_requested_width += btk_tree_view_get_real_requested_width_from_column (tree_view, column);

      if (column->expand)
	number_of_expand_columns++;
    }

  /* Only update the expand value if the width of the widget has changed,
   * or the number of expand columns has changed, or if there are no expand
   * columns, or if we didn't have an size-allocation yet after the
   * last validated node.
   */
  update_expand = (width_changed && *width_changed == TRUE)
      || number_of_expand_columns != tree_view->priv->last_number_of_expand_columns
      || number_of_expand_columns == 0
      || tree_view->priv->post_validation_flag == TRUE;

  tree_view->priv->post_validation_flag = FALSE;

  if (!update_expand)
    {
      extra = tree_view->priv->last_extra_space;
      extra_for_last = MAX (widget->allocation.width - full_requested_width - extra, 0);
    }
  else
    {
      extra = MAX (widget->allocation.width - full_requested_width, 0);
      extra_for_last = 0;

      tree_view->priv->last_extra_space = extra;
    }

  if (number_of_expand_columns > 0)
    extra_per_column = extra/number_of_expand_columns;
  else
    extra_per_column = 0;

  if (update_expand)
    {
      tree_view->priv->last_extra_space_per_column = extra_per_column;
      tree_view->priv->last_number_of_expand_columns = number_of_expand_columns;
    }

  for (list = (rtl ? last_column : first_column); 
       list != (rtl ? first_column->prev : last_column->next);
       list = (rtl ? list->prev : list->next)) 
    {
      gint real_requested_width = 0;
      gint old_width;

      column = list->data;
      old_width = column->width;

      if (!column->visible)
	continue;

      /* We need to handle the dragged button specially.
       */
      if (column == tree_view->priv->drag_column)
	{
	  BtkAllocation drag_allocation;
          drag_allocation.width = bdk_window_get_width (tree_view->priv->drag_window);
          drag_allocation.height = bdk_window_get_height (tree_view->priv->drag_window);
	  drag_allocation.x = 0;
	  drag_allocation.y = 0;
	  btk_widget_size_allocate (tree_view->priv->drag_column->button,
				    &drag_allocation);
	  width += drag_allocation.width;
	  continue;
	}

      real_requested_width = btk_tree_view_get_real_requested_width_from_column (tree_view, column);

      allocation.x = width;
      column->width = real_requested_width;

      if (column->expand)
	{
	  if (number_of_expand_columns == 1)
	    {
	      /* We add the remander to the last column as
	       * */
	      column->width += extra;
	    }
	  else
	    {
	      column->width += extra_per_column;
	      extra -= extra_per_column;
	      number_of_expand_columns --;
	    }
	}
      else if (number_of_expand_columns == 0 &&
	       list == last_column)
	{
	  column->width += extra;
	}

      /* In addition to expand, the last column can get even more
       * extra space so all available space is filled up.
       */
      if (extra_for_last > 0 && list == last_column)
	column->width += extra_for_last;

      g_object_notify (B_OBJECT (column), "width");

      allocation.width = column->width;
      width += column->width;

      if (column->width > old_width)
        column_changed = TRUE;

      btk_widget_size_allocate (column->button, &allocation);

      if (column->window)
	bdk_window_move_resize (column->window,
                                allocation.x + (rtl ? 0 : allocation.width) - TREE_VIEW_DRAG_WIDTH/2,
				allocation.y,
                                TREE_VIEW_DRAG_WIDTH, allocation.height);
    }

  /* We change the width here.  The user might have been resizing columns,
   * so the total width of the tree view changes.
   */
  tree_view->priv->width = width;
  if (width_changed)
    *width_changed = TRUE;

  if (column_changed)
    btk_widget_queue_draw (BTK_WIDGET (tree_view));
}


static void
btk_tree_view_size_allocate (BtkWidget     *widget,
			     BtkAllocation *allocation)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (widget);
  GList *tmp_list;
  gboolean width_changed = FALSE;
  gint old_width = widget->allocation.width;

  if (allocation->width != widget->allocation.width)
    width_changed = TRUE;

  widget->allocation = *allocation;

  tmp_list = tree_view->priv->children;

  while (tmp_list)
    {
      BtkAllocation allocation;

      BtkTreeViewChild *child = tmp_list->data;
      tmp_list = tmp_list->next;

      /* totally ignore our child's requisition */
      allocation.x = child->x;
      allocation.y = child->y;
      allocation.width = child->width;
      allocation.height = child->height;
      btk_widget_size_allocate (child->widget, &allocation);
    }

  /* We size-allocate the columns first because the width of the
   * tree view (used in updating the adjustments below) might change.
   */
  btk_tree_view_size_allocate_columns (widget, &width_changed);

  tree_view->priv->hadjustment->page_size = allocation->width;
  tree_view->priv->hadjustment->page_increment = allocation->width * 0.9;
  tree_view->priv->hadjustment->step_increment = allocation->width * 0.1;
  tree_view->priv->hadjustment->lower = 0;
  tree_view->priv->hadjustment->upper = MAX (tree_view->priv->hadjustment->page_size, tree_view->priv->width);

  if (btk_widget_get_direction(widget) == BTK_TEXT_DIR_RTL)   
    {
      if (allocation->width < tree_view->priv->width)
        {
	  if (tree_view->priv->init_hadjust_value)
	    {
	      tree_view->priv->hadjustment->value = MAX (tree_view->priv->width - allocation->width, 0);
	      tree_view->priv->init_hadjust_value = FALSE;
	    }
	  else if (allocation->width != old_width)
	    {
	      tree_view->priv->hadjustment->value = CLAMP (tree_view->priv->hadjustment->value - allocation->width + old_width, 0, tree_view->priv->width - allocation->width);
	    }
	  else
	    tree_view->priv->hadjustment->value = CLAMP (tree_view->priv->width - (tree_view->priv->prev_width - tree_view->priv->hadjustment->value), 0, tree_view->priv->width - allocation->width);
	}
      else
        {
	  tree_view->priv->hadjustment->value = 0;
	  tree_view->priv->init_hadjust_value = TRUE;
	}
    }
  else
    if (tree_view->priv->hadjustment->value + allocation->width > tree_view->priv->width)
      tree_view->priv->hadjustment->value = MAX (tree_view->priv->width - allocation->width, 0);

  btk_adjustment_changed (tree_view->priv->hadjustment);

  tree_view->priv->vadjustment->page_size = allocation->height - TREE_VIEW_HEADER_HEIGHT (tree_view);
  tree_view->priv->vadjustment->step_increment = tree_view->priv->vadjustment->page_size * 0.1;
  tree_view->priv->vadjustment->page_increment = tree_view->priv->vadjustment->page_size * 0.9;
  tree_view->priv->vadjustment->lower = 0;
  tree_view->priv->vadjustment->upper = MAX (tree_view->priv->vadjustment->page_size, tree_view->priv->height);

  btk_adjustment_changed (tree_view->priv->vadjustment);

  /* now the adjustments and window sizes are in sync, we can sync toprow/dy again */
  if (tree_view->priv->height <= tree_view->priv->vadjustment->page_size)
    btk_adjustment_set_value (BTK_ADJUSTMENT (tree_view->priv->vadjustment), 0);
  else if (tree_view->priv->vadjustment->value + tree_view->priv->vadjustment->page_size > tree_view->priv->height)
    btk_adjustment_set_value (BTK_ADJUSTMENT (tree_view->priv->vadjustment),
                              tree_view->priv->height - tree_view->priv->vadjustment->page_size);
  else if (btk_tree_row_reference_valid (tree_view->priv->top_row))
    btk_tree_view_top_row_to_dy (tree_view);
  else
    btk_tree_view_dy_to_top_row (tree_view);
  
  if (btk_widget_get_realized (widget))
    {
      bdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);
      bdk_window_move_resize (tree_view->priv->header_window,
			      - (gint) tree_view->priv->hadjustment->value,
			      0,
			      MAX (tree_view->priv->width, allocation->width),
			      tree_view->priv->header_height);
      bdk_window_move_resize (tree_view->priv->bin_window,
			      - (gint) tree_view->priv->hadjustment->value,
			      TREE_VIEW_HEADER_HEIGHT (tree_view),
			      MAX (tree_view->priv->width, allocation->width),
			      allocation->height - TREE_VIEW_HEADER_HEIGHT (tree_view));
    }

  if (tree_view->priv->tree == NULL)
    invalidate_empty_focus (tree_view);

  if (btk_widget_get_realized (widget))
    {
      gboolean has_expand_column = FALSE;
      for (tmp_list = tree_view->priv->columns; tmp_list; tmp_list = tmp_list->next)
	{
	  if (btk_tree_view_column_get_expand (BTK_TREE_VIEW_COLUMN (tmp_list->data)))
	    {
	      has_expand_column = TRUE;
	      break;
	    }
	}

      if (width_changed && tree_view->priv->expander_column)
        {
          /* Might seem awkward, but is the best heuristic I could come up
           * with.  Only if the width of the columns before the expander
           * changes, we will update the prelight status.  It is this
           * width that makes the expander move vertically.  Always updating
           * prelight status causes trouble with hover selections.
           */
          gint width_before_expander;

          width_before_expander = btk_tree_view_calculate_width_before_expander (tree_view);

          if (tree_view->priv->prev_width_before_expander
              != width_before_expander)
              update_prelight (tree_view,
                               tree_view->priv->event_last_x,
                               tree_view->priv->event_last_y);

          tree_view->priv->prev_width_before_expander = width_before_expander;
        }

      /* This little hack only works if we have an LTR locale, and no column has the  */
      if (width_changed)
	{
	  if (btk_widget_get_direction (BTK_WIDGET (tree_view)) == BTK_TEXT_DIR_LTR &&
	      ! has_expand_column)
	    invalidate_last_column (tree_view);
	  else
	    btk_widget_queue_draw (widget);
	}
    }
}

/* Grabs the focus and unsets the BTK_TREE_VIEW_DRAW_KEYFOCUS flag */
static void
grab_focus_and_unset_draw_keyfocus (BtkTreeView *tree_view)
{
  BtkWidget *widget = BTK_WIDGET (tree_view);

  if (btk_widget_get_can_focus (widget) && !btk_widget_has_focus (widget))
    btk_widget_grab_focus (widget);
  BTK_TREE_VIEW_UNSET_FLAG (tree_view, BTK_TREE_VIEW_DRAW_KEYFOCUS);
}

static inline gboolean
row_is_separator (BtkTreeView *tree_view,
		  BtkTreeIter *iter,
		  BtkTreePath *path)
{
  gboolean is_separator = FALSE;

  if (tree_view->priv->row_separator_func)
    {
      BtkTreeIter tmpiter;

      if (iter)
        tmpiter = *iter;
      else
        {
          if (!btk_tree_model_get_iter (tree_view->priv->model, &tmpiter, path))
            return FALSE;
        }

      is_separator = tree_view->priv->row_separator_func (tree_view->priv->model,
                                                          &tmpiter,
                                                          tree_view->priv->row_separator_data);
    }

  return is_separator;
}

static gboolean
btk_tree_view_button_press (BtkWidget      *widget,
			    BdkEventButton *event)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (widget);
  GList *list;
  BtkTreeViewColumn *column = NULL;
  gint i;
  BdkRectangle background_area;
  BdkRectangle cell_area;
  gint vertical_separator;
  gint horizontal_separator;
  gboolean path_is_selectable;
  gboolean rtl;

  rtl = (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL);
  btk_tree_view_stop_editing (tree_view, FALSE);
  btk_widget_style_get (widget,
			"vertical-separator", &vertical_separator,
			"horizontal-separator", &horizontal_separator,
			NULL);


  /* Because grab_focus can cause reentrancy, we delay grab_focus until after
   * we're done handling the button press.
   */

  if (event->window == tree_view->priv->bin_window)
    {
      BtkRBNode *node;
      BtkRBTree *tree;
      BtkTreePath *path;
      gchar *path_string;
      gint depth;
      gint new_y;
      gint y_offset;
      gint dval;
      gint pre_val, aft_val;
      BtkTreeViewColumn *column = NULL;
      BtkCellRenderer *focus_cell = NULL;
      gint column_handled_click = FALSE;
      gboolean row_double_click = FALSE;
      gboolean rtl;
      gboolean node_selected;

      /* Empty tree? */
      if (tree_view->priv->tree == NULL)
	{
	  grab_focus_and_unset_draw_keyfocus (tree_view);
	  return TRUE;
	}

      /* are we in an arrow? */
      if (tree_view->priv->prelight_node &&
          BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_ARROW_PRELIT) &&
	  TREE_VIEW_DRAW_EXPANDERS (tree_view))
	{
	  if (event->button == 1)
	    {
	      btk_grab_add (widget);
	      tree_view->priv->button_pressed_node = tree_view->priv->prelight_node;
	      tree_view->priv->button_pressed_tree = tree_view->priv->prelight_tree;
	      btk_tree_view_draw_arrow (BTK_TREE_VIEW (widget),
					tree_view->priv->prelight_tree,
					tree_view->priv->prelight_node,
					event->x,
					event->y);
	    }

	  grab_focus_and_unset_draw_keyfocus (tree_view);
	  return TRUE;
	}

      /* find the node that was clicked */
      new_y = TREE_WINDOW_Y_TO_RBTREE_Y(tree_view, event->y);
      if (new_y < 0)
	new_y = 0;
      y_offset = -_btk_rbtree_find_offset (tree_view->priv->tree, new_y, &tree, &node);

      if (node == NULL)
	{
	  /* We clicked in dead space */
	  grab_focus_and_unset_draw_keyfocus (tree_view);
	  return TRUE;
	}

      /* Get the path and the node */
      path = _btk_tree_view_find_path (tree_view, tree, node);
      path_is_selectable = !row_is_separator (tree_view, NULL, path);

      if (!path_is_selectable)
	{
	  btk_tree_path_free (path);
	  grab_focus_and_unset_draw_keyfocus (tree_view);
	  return TRUE;
	}

      depth = btk_tree_path_get_depth (path);
      background_area.y = y_offset + event->y;
      background_area.height = ROW_HEIGHT (tree_view, BTK_RBNODE_GET_HEIGHT (node));
      background_area.x = 0;


      /* Let the column have a chance at selecting it. */
      rtl = (btk_widget_get_direction (BTK_WIDGET (tree_view)) == BTK_TEXT_DIR_RTL);
      for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
	   list; list = (rtl ? list->prev : list->next))
	{
	  BtkTreeViewColumn *candidate = list->data;

	  if (!candidate->visible)
	    continue;

	  background_area.width = candidate->width;
	  if ((background_area.x > (gint) event->x) ||
	      (background_area.x + background_area.width <= (gint) event->x))
	    {
	      background_area.x += background_area.width;
	      continue;
	    }

	  /* we found the focus column */
	  column = candidate;
	  cell_area = background_area;
	  cell_area.width -= horizontal_separator;
	  cell_area.height -= vertical_separator;
	  cell_area.x += horizontal_separator/2;
	  cell_area.y += vertical_separator/2;
	  if (btk_tree_view_is_expander_column (tree_view, column))
	    {
	      if (!rtl)
		cell_area.x += (depth - 1) * tree_view->priv->level_indentation;
	      cell_area.width -= (depth - 1) * tree_view->priv->level_indentation;

              if (TREE_VIEW_DRAW_EXPANDERS (tree_view))
	        {
		  if (!rtl)
		    cell_area.x += depth * tree_view->priv->expander_size;
	          cell_area.width -= depth * tree_view->priv->expander_size;
		}
	    }
	  break;
	}

      if (column == NULL)
	{
	  btk_tree_path_free (path);
	  grab_focus_and_unset_draw_keyfocus (tree_view);
	  return FALSE;
	}

      tree_view->priv->focus_column = column;

      /* decide if we edit */
      if (event->type == BDK_BUTTON_PRESS && event->button == 1 &&
	  !(event->state & btk_accelerator_get_default_mod_mask ()))
	{
	  BtkTreePath *anchor;
	  BtkTreeIter iter;

	  btk_tree_model_get_iter (tree_view->priv->model, &iter, path);
	  btk_tree_view_column_cell_set_cell_data (column,
						   tree_view->priv->model,
						   &iter,
						   BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_PARENT),
						   node->children?TRUE:FALSE);

	  if (tree_view->priv->anchor)
	    anchor = btk_tree_row_reference_get_path (tree_view->priv->anchor);
	  else
	    anchor = NULL;

	  if ((anchor && !btk_tree_path_compare (anchor, path))
	      || !_btk_tree_view_column_has_editable_cell (column))
	    {
	      BtkCellEditable *cell_editable = NULL;

	      /* FIXME: get the right flags */
	      guint flags = 0;

	      path_string = btk_tree_path_to_string (path);

	      if (_btk_tree_view_column_cell_event (column,
						    &cell_editable,
						    (BdkEvent *)event,
						    path_string,
						    &background_area,
						    &cell_area, flags))
		{
		  if (cell_editable != NULL)
		    {
		      gint left, right;
		      BdkRectangle area;

		      area = cell_area;
		      _btk_tree_view_column_get_neighbor_sizes (column,	_btk_tree_view_column_get_edited_cell (column), &left, &right);

		      area.x += left;
		      area.width -= right + left;

		      btk_tree_view_real_start_editing (tree_view,
							column,
							path,
							cell_editable,
							&area,
							(BdkEvent *)event,
							flags);
		      g_free (path_string);
		      btk_tree_path_free (path);
		      btk_tree_path_free (anchor);
		      return TRUE;
		    }
		  column_handled_click = TRUE;
		}
	      g_free (path_string);
	    }
	  if (anchor)
	    btk_tree_path_free (anchor);
	}

      /* select */
      node_selected = BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SELECTED);
      pre_val = tree_view->priv->vadjustment->value;

      /* we only handle selection modifications on the first button press
       */
      if (event->type == BDK_BUTTON_PRESS)
        {
          if ((event->state & BTK_MODIFY_SELECTION_MOD_MASK) == BTK_MODIFY_SELECTION_MOD_MASK)
            tree_view->priv->modify_selection_pressed = TRUE;
          if ((event->state & BTK_EXTEND_SELECTION_MOD_MASK) == BTK_EXTEND_SELECTION_MOD_MASK)
            tree_view->priv->extend_selection_pressed = TRUE;

          focus_cell = _btk_tree_view_column_get_cell_at_pos (column, event->x - background_area.x);
          if (focus_cell)
            btk_tree_view_column_focus_cell (column, focus_cell);

          if (event->state & BTK_MODIFY_SELECTION_MOD_MASK)
            {
              btk_tree_view_real_set_cursor (tree_view, path, FALSE, TRUE);
              btk_tree_view_real_toggle_cursor_row (tree_view);
            }
          else if (event->state & BTK_EXTEND_SELECTION_MOD_MASK)
            {
              btk_tree_view_real_set_cursor (tree_view, path, FALSE, TRUE);
              btk_tree_view_real_select_cursor_row (tree_view, FALSE);
            }
          else
            {
              btk_tree_view_real_set_cursor (tree_view, path, TRUE, TRUE);
            }

          tree_view->priv->modify_selection_pressed = FALSE;
          tree_view->priv->extend_selection_pressed = FALSE;
        }

      /* the treeview may have been scrolled because of _set_cursor,
       * correct here
       */

      aft_val = tree_view->priv->vadjustment->value;
      dval = pre_val - aft_val;

      cell_area.y += dval;
      background_area.y += dval;

      /* Save press to possibly begin a drag
       */
      if (!column_handled_click &&
	  !tree_view->priv->in_grab &&
	  tree_view->priv->pressed_button < 0)
        {
          tree_view->priv->pressed_button = event->button;
          tree_view->priv->press_start_x = event->x;
          tree_view->priv->press_start_y = event->y;

	  if (tree_view->priv->rubber_banding_enable
	      && !node_selected
	      && tree_view->priv->selection->type == BTK_SELECTION_MULTIPLE)
	    {
	      tree_view->priv->press_start_y += tree_view->priv->dy;
	      tree_view->priv->rubber_band_x = event->x;
	      tree_view->priv->rubber_band_y = event->y + tree_view->priv->dy;
	      tree_view->priv->rubber_band_status = RUBBER_BAND_MAYBE_START;

	      if ((event->state & BTK_MODIFY_SELECTION_MOD_MASK) == BTK_MODIFY_SELECTION_MOD_MASK)
		tree_view->priv->rubber_band_modify = TRUE;
	      if ((event->state & BTK_EXTEND_SELECTION_MOD_MASK) == BTK_EXTEND_SELECTION_MOD_MASK)
		tree_view->priv->rubber_band_extend = TRUE;
	    }
        }

      /* Test if a double click happened on the same row. */
      if (event->button == 1 && event->type == BDK_BUTTON_PRESS)
        {
          int double_click_time, double_click_distance;

          g_object_get (btk_settings_get_default (),
                        "btk-double-click-time", &double_click_time,
                        "btk-double-click-distance", &double_click_distance,
                        NULL);

          /* Same conditions as _bdk_event_button_generate */
          if (tree_view->priv->last_button_x != -1 &&
              (event->time < tree_view->priv->last_button_time + double_click_time) &&
              (ABS (event->x - tree_view->priv->last_button_x) <= double_click_distance) &&
              (ABS (event->y - tree_view->priv->last_button_y) <= double_click_distance))
            {
              /* We do no longer compare paths of this row and the
               * row clicked previously.  We use the double click
               * distance to decide whether this is a valid click,
               * allowing the mouse to slightly move over another row.
               */
              row_double_click = TRUE;

              tree_view->priv->last_button_time = 0;
              tree_view->priv->last_button_x = -1;
              tree_view->priv->last_button_y = -1;
            }
          else
            {
              tree_view->priv->last_button_time = event->time;
              tree_view->priv->last_button_x = event->x;
              tree_view->priv->last_button_y = event->y;
            }
        }

      if (row_double_click)
	{
	  btk_grab_remove (widget);
	  btk_tree_view_row_activated (tree_view, path, column);

          if (tree_view->priv->pressed_button == event->button)
            tree_view->priv->pressed_button = -1;
	}

      btk_tree_path_free (path);

      /* If we activated the row through a double click we don't want to grab
       * focus back, as moving focus to another widget is pretty common.
       */
      if (!row_double_click)
	grab_focus_and_unset_draw_keyfocus (tree_view);

      return TRUE;
    }

  /* We didn't click in the window.  Let's check to see if we clicked on a column resize window.
   */
  for (i = 0, list = tree_view->priv->columns; list; list = list->next, i++)
    {
      column = list->data;
      if (event->window == column->window &&
	  column->resizable &&
	  column->window)
	{
	  gpointer drag_data;

	  if (event->type == BDK_2BUTTON_PRESS &&
	      btk_tree_view_column_get_sizing (column) != BTK_TREE_VIEW_COLUMN_AUTOSIZE)
	    {
	      column->use_resized_width = FALSE;
	      _btk_tree_view_column_autosize (tree_view, column);
	      return TRUE;
	    }

	  if (bdk_pointer_grab (column->window, FALSE,
				BDK_POINTER_MOTION_HINT_MASK |
				BDK_BUTTON1_MOTION_MASK |
				BDK_BUTTON_RELEASE_MASK,
				NULL, NULL, event->time))
	    return FALSE;

	  btk_grab_add (widget);
	  BTK_TREE_VIEW_SET_FLAG (tree_view, BTK_TREE_VIEW_IN_COLUMN_RESIZE);
	  column->resized_width = column->width - tree_view->priv->last_extra_space_per_column;

	  /* block attached dnd signal handler */
	  drag_data = g_object_get_data (B_OBJECT (widget), "btk-site-data");
	  if (drag_data)
	    g_signal_handlers_block_matched (widget,
					     G_SIGNAL_MATCH_DATA,
					     0, 0, NULL, NULL,
					     drag_data);

	  tree_view->priv->drag_pos = i;
	  tree_view->priv->x_drag = column->button->allocation.x + (rtl ? 0 : column->button->allocation.width);

	  if (!btk_widget_has_focus (widget))
	    btk_widget_grab_focus (widget);

	  return TRUE;
	}
    }
  return FALSE;
}

/* BtkWidget::button_release_event helper */
static gboolean
btk_tree_view_button_release_drag_column (BtkWidget      *widget,
					  BdkEventButton *event)
{
  BtkTreeView *tree_view;
  GList *l;
  gboolean rtl;

  tree_view = BTK_TREE_VIEW (widget);

  rtl = (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL);
  bdk_display_pointer_ungrab (btk_widget_get_display (widget), BDK_CURRENT_TIME);
  bdk_display_keyboard_ungrab (btk_widget_get_display (widget), BDK_CURRENT_TIME);

  /* Move the button back */
  g_object_ref (tree_view->priv->drag_column->button);
  btk_container_remove (BTK_CONTAINER (tree_view), tree_view->priv->drag_column->button);
  btk_widget_set_parent_window (tree_view->priv->drag_column->button, tree_view->priv->header_window);
  btk_widget_set_parent (tree_view->priv->drag_column->button, BTK_WIDGET (tree_view));
  g_object_unref (tree_view->priv->drag_column->button);
  btk_widget_queue_resize (widget);
  if (tree_view->priv->drag_column->resizable)
    {
      bdk_window_raise (tree_view->priv->drag_column->window);
      bdk_window_show (tree_view->priv->drag_column->window);
    }
  else
    bdk_window_hide (tree_view->priv->drag_column->window);

  btk_widget_grab_focus (tree_view->priv->drag_column->button);

  if (rtl)
    {
      if (tree_view->priv->cur_reorder &&
	  tree_view->priv->cur_reorder->right_column != tree_view->priv->drag_column)
	btk_tree_view_move_column_after (tree_view, tree_view->priv->drag_column,
					 tree_view->priv->cur_reorder->right_column);
    }
  else
    {
      if (tree_view->priv->cur_reorder &&
	  tree_view->priv->cur_reorder->left_column != tree_view->priv->drag_column)
	btk_tree_view_move_column_after (tree_view, tree_view->priv->drag_column,
					 tree_view->priv->cur_reorder->left_column);
    }
  tree_view->priv->drag_column = NULL;
  bdk_window_hide (tree_view->priv->drag_window);

  for (l = tree_view->priv->column_drag_info; l != NULL; l = l->next)
    g_slice_free (BtkTreeViewColumnReorder, l->data);
  g_list_free (tree_view->priv->column_drag_info);
  tree_view->priv->column_drag_info = NULL;
  tree_view->priv->cur_reorder = NULL;

  if (tree_view->priv->drag_highlight_window)
    bdk_window_hide (tree_view->priv->drag_highlight_window);

  /* Reset our flags */
  tree_view->priv->drag_column_window_state = DRAG_COLUMN_WINDOW_STATE_UNSET;
  BTK_TREE_VIEW_UNSET_FLAG (tree_view, BTK_TREE_VIEW_IN_COLUMN_DRAG);

  return TRUE;
}

/* BtkWidget::button_release_event helper */
static gboolean
btk_tree_view_button_release_column_resize (BtkWidget      *widget,
					    BdkEventButton *event)
{
  BtkTreeView *tree_view;
  gpointer drag_data;

  tree_view = BTK_TREE_VIEW (widget);

  tree_view->priv->drag_pos = -1;

  /* unblock attached dnd signal handler */
  drag_data = g_object_get_data (B_OBJECT (widget), "btk-site-data");
  if (drag_data)
    g_signal_handlers_unblock_matched (widget,
				       G_SIGNAL_MATCH_DATA,
				       0, 0, NULL, NULL,
				       drag_data);

  BTK_TREE_VIEW_UNSET_FLAG (tree_view, BTK_TREE_VIEW_IN_COLUMN_RESIZE);
  btk_grab_remove (widget);
  bdk_display_pointer_ungrab (bdk_window_get_display (event->window),
			      event->time);
  return TRUE;
}

static gboolean
btk_tree_view_button_release (BtkWidget      *widget,
			      BdkEventButton *event)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (widget);

  if (BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_IN_COLUMN_DRAG))
    return btk_tree_view_button_release_drag_column (widget, event);

  if (tree_view->priv->rubber_band_status)
    btk_tree_view_stop_rubber_band (tree_view);

  if (tree_view->priv->pressed_button == event->button)
    tree_view->priv->pressed_button = -1;

  if (BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_IN_COLUMN_RESIZE))
    return btk_tree_view_button_release_column_resize (widget, event);

  if (tree_view->priv->button_pressed_node == NULL)
    return FALSE;

  if (event->button == 1)
    {
      if (tree_view->priv->button_pressed_node == tree_view->priv->prelight_node &&
          BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_ARROW_PRELIT))
	{
	  BtkTreePath *path = NULL;

	  path = _btk_tree_view_find_path (tree_view,
					   tree_view->priv->button_pressed_tree,
					   tree_view->priv->button_pressed_node);
	  /* Actually activate the node */
	  if (tree_view->priv->button_pressed_node->children == NULL)
	    btk_tree_view_real_expand_row (tree_view, path,
					   tree_view->priv->button_pressed_tree,
					   tree_view->priv->button_pressed_node,
					   FALSE, TRUE);
	  else
	    btk_tree_view_real_collapse_row (BTK_TREE_VIEW (widget), path,
					     tree_view->priv->button_pressed_tree,
					     tree_view->priv->button_pressed_node, TRUE);
	  btk_tree_path_free (path);
	}

      tree_view->priv->button_pressed_tree = NULL;
      tree_view->priv->button_pressed_node = NULL;

      btk_grab_remove (widget);
    }

  return TRUE;
}

static gboolean
btk_tree_view_grab_broken (BtkWidget          *widget,
			   BdkEventGrabBroken *event)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (widget);

  if (BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_IN_COLUMN_DRAG))
    btk_tree_view_button_release_drag_column (widget, (BdkEventButton *)event);

  if (BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_IN_COLUMN_RESIZE))
    btk_tree_view_button_release_column_resize (widget, (BdkEventButton *)event);

  return TRUE;
}

#if 0
static gboolean
btk_tree_view_configure (BtkWidget *widget,
			 BdkEventConfigure *event)
{
  BtkTreeView *tree_view;

  tree_view = BTK_TREE_VIEW (widget);
  tree_view->priv->search_position_func (tree_view, tree_view->priv->search_window);

  return FALSE;
}
#endif

/* BtkWidget::motion_event function set.
 */

static gboolean
coords_are_over_arrow (BtkTreeView *tree_view,
                       BtkRBTree   *tree,
                       BtkRBNode   *node,
                       /* these are in bin window coords */
                       gint         x,
                       gint         y)
{
  BdkRectangle arrow;
  gint x2;

  if (!btk_widget_get_realized (BTK_WIDGET (tree_view)))
    return FALSE;

  if ((node->flags & BTK_RBNODE_IS_PARENT) == 0)
    return FALSE;

  arrow.y = BACKGROUND_FIRST_PIXEL (tree_view, tree, node);

  arrow.height = ROW_HEIGHT (tree_view, BACKGROUND_HEIGHT (node));

  btk_tree_view_get_arrow_xrange (tree_view, tree, &arrow.x, &x2);

  arrow.width = x2 - arrow.x;

  return (x >= arrow.x &&
          x < (arrow.x + arrow.width) &&
	  y >= arrow.y &&
	  y < (arrow.y + arrow.height));
}

static gboolean
auto_expand_timeout (gpointer data)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (data);
  BtkTreePath *path;

  if (tree_view->priv->prelight_node)
    {
      path = _btk_tree_view_find_path (tree_view,
				       tree_view->priv->prelight_tree,
				       tree_view->priv->prelight_node);   

      if (tree_view->priv->prelight_node->children)
	btk_tree_view_collapse_row (tree_view, path);
      else
	btk_tree_view_expand_row (tree_view, path, FALSE);

      btk_tree_path_free (path);
    }

  tree_view->priv->auto_expand_timeout = 0;

  return FALSE;
}

static void
remove_auto_expand_timeout (BtkTreeView *tree_view)
{
  if (tree_view->priv->auto_expand_timeout != 0)
    {
      g_source_remove (tree_view->priv->auto_expand_timeout);
      tree_view->priv->auto_expand_timeout = 0;
    }
}

static void
do_prelight (BtkTreeView *tree_view,
             BtkRBTree   *tree,
             BtkRBNode   *node,
	     /* these are in bin_window coords */
             gint         x,
             gint         y)
{
  if (tree_view->priv->prelight_tree == tree &&
      tree_view->priv->prelight_node == node)
    {
      /*  We are still on the same node,
	  but we might need to take care of the arrow  */

      if (tree && node && TREE_VIEW_DRAW_EXPANDERS (tree_view))
	{
	  gboolean over_arrow;
	  gboolean flag_set;

	  over_arrow = coords_are_over_arrow (tree_view, tree, node, x, y);
	  flag_set = BTK_TREE_VIEW_FLAG_SET (tree_view,
					     BTK_TREE_VIEW_ARROW_PRELIT);

	  if (over_arrow != flag_set)
	    {
	      if (over_arrow)
		BTK_TREE_VIEW_SET_FLAG (tree_view,
					BTK_TREE_VIEW_ARROW_PRELIT);
	      else
		BTK_TREE_VIEW_UNSET_FLAG (tree_view,
					  BTK_TREE_VIEW_ARROW_PRELIT);

	      btk_tree_view_draw_arrow (tree_view, tree, node, x, y);
	    }
	}

      return;
    }

  if (tree_view->priv->prelight_tree && tree_view->priv->prelight_node)
    {
      /*  Unprelight the old node and arrow  */

      BTK_RBNODE_UNSET_FLAG (tree_view->priv->prelight_node,
			     BTK_RBNODE_IS_PRELIT);

      if (BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_ARROW_PRELIT)
	  && TREE_VIEW_DRAW_EXPANDERS (tree_view))
	{
	  BTK_TREE_VIEW_UNSET_FLAG (tree_view, BTK_TREE_VIEW_ARROW_PRELIT);
	  
	  btk_tree_view_draw_arrow (tree_view,
				    tree_view->priv->prelight_tree,
				    tree_view->priv->prelight_node,
				    x,
				    y);
	}

      _btk_tree_view_queue_draw_node (tree_view,
				      tree_view->priv->prelight_tree,
				      tree_view->priv->prelight_node,
				      NULL);
    }


  if (tree_view->priv->hover_expand)
    remove_auto_expand_timeout (tree_view);

  /*  Set the new prelight values  */
  tree_view->priv->prelight_node = node;
  tree_view->priv->prelight_tree = tree;

  if (!node || !tree)
    return;

  /*  Prelight the new node and arrow  */

  if (TREE_VIEW_DRAW_EXPANDERS (tree_view)
      && coords_are_over_arrow (tree_view, tree, node, x, y))
    {
      BTK_TREE_VIEW_SET_FLAG (tree_view, BTK_TREE_VIEW_ARROW_PRELIT);

      btk_tree_view_draw_arrow (tree_view, tree, node, x, y);
    }

  BTK_RBNODE_SET_FLAG (node, BTK_RBNODE_IS_PRELIT);

  _btk_tree_view_queue_draw_node (tree_view, tree, node, NULL);

  if (tree_view->priv->hover_expand)
    {
      tree_view->priv->auto_expand_timeout = 
	bdk_threads_add_timeout (AUTO_EXPAND_TIMEOUT, auto_expand_timeout, tree_view);
    }
}

static void
prelight_or_select (BtkTreeView *tree_view,
		    BtkRBTree   *tree,
		    BtkRBNode   *node,
		    /* these are in bin_window coords */
		    gint         x,
		    gint         y)
{
  BtkSelectionMode mode = btk_tree_selection_get_mode (tree_view->priv->selection);
  
  if (tree_view->priv->hover_selection &&
      (mode == BTK_SELECTION_SINGLE || mode == BTK_SELECTION_BROWSE) &&
      !(tree_view->priv->edited_column &&
	tree_view->priv->edited_column->editable_widget))
    {
      if (node)
	{
	  if (!BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SELECTED))
	    {
	      BtkTreePath *path;
	      
	      path = _btk_tree_view_find_path (tree_view, tree, node);
	      btk_tree_selection_select_path (tree_view->priv->selection, path);
	      if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SELECTED))
		{
		  BTK_TREE_VIEW_UNSET_FLAG (tree_view, BTK_TREE_VIEW_DRAW_KEYFOCUS);
		  btk_tree_view_real_set_cursor (tree_view, path, FALSE, FALSE);
		}
	      btk_tree_path_free (path);
	    }
	}

      else if (mode == BTK_SELECTION_SINGLE)
	btk_tree_selection_unselect_all (tree_view->priv->selection);
    }

    do_prelight (tree_view, tree, node, x, y);
}

static void
ensure_unprelighted (BtkTreeView *tree_view)
{
  do_prelight (tree_view,
	       NULL, NULL,
	       -1000, -1000); /* coords not possibly over an arrow */

  g_assert (tree_view->priv->prelight_node == NULL);
}

static void
update_prelight (BtkTreeView *tree_view,
                 gint         x,
                 gint         y)
{
  int new_y;
  BtkRBTree *tree;
  BtkRBNode *node;

  if (tree_view->priv->tree == NULL)
    return;

  if (x == -10000)
    {
      ensure_unprelighted (tree_view);
      return;
    }

  new_y = TREE_WINDOW_Y_TO_RBTREE_Y (tree_view, y);
  if (new_y < 0)
    new_y = 0;

  _btk_rbtree_find_offset (tree_view->priv->tree,
                           new_y, &tree, &node);

  if (node)
    prelight_or_select (tree_view, tree, node, x, y);
}




/* Our motion arrow is either a box (in the case of the original spot)
 * or an arrow.  It is expander_size wide.
 */
/*
 * 11111111111111
 * 01111111111110
 * 00111111111100
 * 00011111111000
 * 00001111110000
 * 00000111100000
 * 00000111100000
 * 00000111100000
 * ~ ~ ~ ~ ~ ~ ~
 * 00000111100000
 * 00000111100000
 * 00000111100000
 * 00001111110000
 * 00011111111000
 * 00111111111100
 * 01111111111110
 * 11111111111111
 */

static void
btk_tree_view_motion_draw_column_motion_arrow (BtkTreeView *tree_view)
{
  BtkTreeViewColumnReorder *reorder = tree_view->priv->cur_reorder;
  BtkWidget *widget = BTK_WIDGET (tree_view);
  BdkBitmap *mask = NULL;
  gint x;
  gint y;
  gint width;
  gint height;
  gint arrow_type = DRAG_COLUMN_WINDOW_STATE_UNSET;
  BdkWindowAttr attributes;
  guint attributes_mask;
  bairo_t *cr;

  if (!reorder ||
      reorder->left_column == tree_view->priv->drag_column ||
      reorder->right_column == tree_view->priv->drag_column)
    arrow_type = DRAG_COLUMN_WINDOW_STATE_ORIGINAL;
  else if (reorder->left_column || reorder->right_column)
    {
      BdkRectangle visible_rect;
      btk_tree_view_get_visible_rect (tree_view, &visible_rect);
      if (reorder->left_column)
	x = reorder->left_column->button->allocation.x + reorder->left_column->button->allocation.width;
      else
	x = reorder->right_column->button->allocation.x;

      if (x < visible_rect.x)
	arrow_type = DRAG_COLUMN_WINDOW_STATE_ARROW_LEFT;
      else if (x > visible_rect.x + visible_rect.width)
	arrow_type = DRAG_COLUMN_WINDOW_STATE_ARROW_RIGHT;
      else
        arrow_type = DRAG_COLUMN_WINDOW_STATE_ARROW;
    }

  /* We want to draw the rectangle over the initial location. */
  if (arrow_type == DRAG_COLUMN_WINDOW_STATE_ORIGINAL)
    {
      if (tree_view->priv->drag_column_window_state != DRAG_COLUMN_WINDOW_STATE_ORIGINAL)
	{
	  if (tree_view->priv->drag_highlight_window)
	    {
	      bdk_window_set_user_data (tree_view->priv->drag_highlight_window,
					NULL);
	      bdk_window_destroy (tree_view->priv->drag_highlight_window);
	    }

	  attributes.window_type = BDK_WINDOW_CHILD;
	  attributes.wclass = BDK_INPUT_OUTPUT;
          attributes.x = tree_view->priv->drag_column_x;
          attributes.y = 0;
	  width = attributes.width = tree_view->priv->drag_column->button->allocation.width;
	  height = attributes.height = tree_view->priv->drag_column->button->allocation.height;
	  attributes.visual = btk_widget_get_visual (BTK_WIDGET (tree_view));
	  attributes.colormap = btk_widget_get_colormap (BTK_WIDGET (tree_view));
	  attributes.event_mask = BDK_VISIBILITY_NOTIFY_MASK | BDK_EXPOSURE_MASK | BDK_POINTER_MOTION_MASK;
	  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;
	  tree_view->priv->drag_highlight_window = bdk_window_new (tree_view->priv->header_window, &attributes, attributes_mask);
	  bdk_window_set_user_data (tree_view->priv->drag_highlight_window, BTK_WIDGET (tree_view));

	  mask = bdk_pixmap_new (tree_view->priv->drag_highlight_window, width, height, 1);
          cr = bdk_bairo_create (mask);

          bairo_set_operator (cr, BAIRO_OPERATOR_CLEAR);
          bairo_paint (cr);
          bairo_set_operator (cr, BAIRO_OPERATOR_SOURCE);
          bairo_rectangle (cr, 1, 1, width - 2, height - 2);
          bairo_stroke (cr);
          bairo_destroy (cr);

	  bdk_window_shape_combine_mask (tree_view->priv->drag_highlight_window,
					 mask, 0, 0);
	  if (mask) g_object_unref (mask);
	  tree_view->priv->drag_column_window_state = DRAG_COLUMN_WINDOW_STATE_ORIGINAL;
	}
    }
  else if (arrow_type == DRAG_COLUMN_WINDOW_STATE_ARROW)
    {
      width = tree_view->priv->expander_size;

      /* Get x, y, width, height of arrow */
      bdk_window_get_origin (tree_view->priv->header_window, &x, &y);
      if (reorder->left_column)
	{
	  x += reorder->left_column->button->allocation.x + reorder->left_column->button->allocation.width - width/2;
	  height = reorder->left_column->button->allocation.height;
	}
      else
	{
	  x += reorder->right_column->button->allocation.x - width/2;
	  height = reorder->right_column->button->allocation.height;
	}
      y -= tree_view->priv->expander_size/2; /* The arrow takes up only half the space */
      height += tree_view->priv->expander_size;

      /* Create the new window */
      if (tree_view->priv->drag_column_window_state != DRAG_COLUMN_WINDOW_STATE_ARROW)
	{
	  if (tree_view->priv->drag_highlight_window)
	    {
	      bdk_window_set_user_data (tree_view->priv->drag_highlight_window,
					NULL);
	      bdk_window_destroy (tree_view->priv->drag_highlight_window);
	    }

	  attributes.window_type = BDK_WINDOW_TEMP;
	  attributes.wclass = BDK_INPUT_OUTPUT;
	  attributes.visual = btk_widget_get_visual (BTK_WIDGET (tree_view));
	  attributes.colormap = btk_widget_get_colormap (BTK_WIDGET (tree_view));
	  attributes.event_mask = BDK_VISIBILITY_NOTIFY_MASK | BDK_EXPOSURE_MASK | BDK_POINTER_MOTION_MASK;
	  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;
          attributes.x = x;
          attributes.y = y;
	  attributes.width = width;
	  attributes.height = height;
	  tree_view->priv->drag_highlight_window = bdk_window_new (btk_widget_get_root_window (widget),
								   &attributes, attributes_mask);
	  bdk_window_set_user_data (tree_view->priv->drag_highlight_window, BTK_WIDGET (tree_view));

	  mask = bdk_pixmap_new (tree_view->priv->drag_highlight_window, width, height, 1);
          cr = bdk_bairo_create (mask);

          bairo_set_operator (cr, BAIRO_OPERATOR_CLEAR);
          bairo_paint (cr);
          bairo_set_operator (cr, BAIRO_OPERATOR_SOURCE);
          bairo_move_to (cr, 0, 0);
          bairo_line_to (cr, width, 0);
          bairo_line_to (cr, width / 2., width / 2);
          bairo_move_to (cr, 0, height);
          bairo_line_to (cr, width, height);
          bairo_line_to (cr, width / 2., height - width / 2.);
          bairo_fill (cr);

          bairo_destroy (cr);
	  bdk_window_shape_combine_mask (tree_view->priv->drag_highlight_window,
					 mask, 0, 0);
	  if (mask) g_object_unref (mask);
	}

      tree_view->priv->drag_column_window_state = DRAG_COLUMN_WINDOW_STATE_ARROW;
      bdk_window_move (tree_view->priv->drag_highlight_window, x, y);
    }
  else if (arrow_type == DRAG_COLUMN_WINDOW_STATE_ARROW_LEFT ||
	   arrow_type == DRAG_COLUMN_WINDOW_STATE_ARROW_RIGHT)
    {
      width = tree_view->priv->expander_size;

      /* Get x, y, width, height of arrow */
      width = width/2; /* remember, the arrow only takes half the available width */
      bdk_window_get_origin (widget->window, &x, &y);
      if (arrow_type == DRAG_COLUMN_WINDOW_STATE_ARROW_RIGHT)
	x += widget->allocation.width - width;

      if (reorder->left_column)
	height = reorder->left_column->button->allocation.height;
      else
	height = reorder->right_column->button->allocation.height;

      y -= tree_view->priv->expander_size;
      height += 2*tree_view->priv->expander_size;

      /* Create the new window */
      if (tree_view->priv->drag_column_window_state != DRAG_COLUMN_WINDOW_STATE_ARROW_LEFT &&
	  tree_view->priv->drag_column_window_state != DRAG_COLUMN_WINDOW_STATE_ARROW_RIGHT)
	{
	  if (tree_view->priv->drag_highlight_window)
	    {
	      bdk_window_set_user_data (tree_view->priv->drag_highlight_window,
					NULL);
	      bdk_window_destroy (tree_view->priv->drag_highlight_window);
	    }

	  attributes.window_type = BDK_WINDOW_TEMP;
	  attributes.wclass = BDK_INPUT_OUTPUT;
	  attributes.visual = btk_widget_get_visual (BTK_WIDGET (tree_view));
	  attributes.colormap = btk_widget_get_colormap (BTK_WIDGET (tree_view));
	  attributes.event_mask = BDK_VISIBILITY_NOTIFY_MASK | BDK_EXPOSURE_MASK | BDK_POINTER_MOTION_MASK;
	  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;
          attributes.x = x;
          attributes.y = y;
	  attributes.width = width;
	  attributes.height = height;
	  tree_view->priv->drag_highlight_window = bdk_window_new (NULL, &attributes, attributes_mask);
	  bdk_window_set_user_data (tree_view->priv->drag_highlight_window, BTK_WIDGET (tree_view));

	  mask = bdk_pixmap_new (tree_view->priv->drag_highlight_window, width, height, 1);
          cr = bdk_bairo_create (mask);

          bairo_set_operator (cr, BAIRO_OPERATOR_CLEAR);
          bairo_paint (cr);
          bairo_set_operator (cr, BAIRO_OPERATOR_SOURCE);
          /* mirror if we're on the left */
          if (arrow_type == DRAG_COLUMN_WINDOW_STATE_ARROW_LEFT)
            {
              bairo_translate (cr, width, 0);
              bairo_scale (cr, -1, 1);
            }
          bairo_move_to (cr, 0, 0);
          bairo_line_to (cr, width, width);
          bairo_line_to (cr, 0, tree_view->priv->expander_size);
          bairo_move_to (cr, 0, height);
          bairo_line_to (cr, width, height - width);
          bairo_line_to (cr, 0, height - tree_view->priv->expander_size);
          bairo_fill (cr);

          bairo_destroy (cr);
	  bdk_window_shape_combine_mask (tree_view->priv->drag_highlight_window,
					 mask, 0, 0);
	  if (mask) g_object_unref (mask);
	}

      tree_view->priv->drag_column_window_state = arrow_type;
      bdk_window_move (tree_view->priv->drag_highlight_window, x, y);
   }
  else
    {
      g_warning (B_STRLOC"Invalid BtkTreeViewColumnReorder struct");
      bdk_window_hide (tree_view->priv->drag_highlight_window);
      return;
    }

  bdk_window_show (tree_view->priv->drag_highlight_window);
  bdk_window_raise (tree_view->priv->drag_highlight_window);
}

static gboolean
btk_tree_view_motion_resize_column (BtkWidget      *widget,
				    BdkEventMotion *event)
{
  gint x;
  gint new_width;
  BtkTreeViewColumn *column;
  BtkTreeView *tree_view = BTK_TREE_VIEW (widget);

  column = btk_tree_view_get_column (tree_view, tree_view->priv->drag_pos);

  if (event->is_hint || event->window != widget->window)
    btk_widget_get_pointer (widget, &x, NULL);
  else
    x = event->x;

  if (tree_view->priv->hadjustment)
    x += tree_view->priv->hadjustment->value;

  new_width = btk_tree_view_new_column_width (tree_view,
					      tree_view->priv->drag_pos, &x);
  if (x != tree_view->priv->x_drag &&
      (new_width != column->fixed_width))
    {
      column->use_resized_width = TRUE;
      column->resized_width = new_width;
      if (column->expand)
	column->resized_width -= tree_view->priv->last_extra_space_per_column;
      btk_widget_queue_resize (widget);
    }

  return FALSE;
}


static void
btk_tree_view_update_current_reorder (BtkTreeView *tree_view)
{
  BtkTreeViewColumnReorder *reorder = NULL;
  GList *list;
  gint mouse_x;

  bdk_window_get_pointer (tree_view->priv->header_window, &mouse_x, NULL, NULL);
  for (list = tree_view->priv->column_drag_info; list; list = list->next)
    {
      reorder = (BtkTreeViewColumnReorder *) list->data;
      if (mouse_x >= reorder->left_align && mouse_x < reorder->right_align)
	break;
      reorder = NULL;
    }

  /*  if (reorder && reorder == tree_view->priv->cur_reorder)
      return;*/

  tree_view->priv->cur_reorder = reorder;
  btk_tree_view_motion_draw_column_motion_arrow (tree_view);
}

static void
btk_tree_view_vertical_autoscroll (BtkTreeView *tree_view)
{
  BdkRectangle visible_rect;
  gint y;
  gint offset;
  gfloat value;

  bdk_window_get_pointer (tree_view->priv->bin_window, NULL, &y, NULL);
  y += tree_view->priv->dy;

  btk_tree_view_get_visible_rect (tree_view, &visible_rect);

  /* see if we are near the edge. */
  offset = y - (visible_rect.y + 2 * SCROLL_EDGE_SIZE);
  if (offset > 0)
    {
      offset = y - (visible_rect.y + visible_rect.height - 2 * SCROLL_EDGE_SIZE);
      if (offset < 0)
	return;
    }

  value = CLAMP (tree_view->priv->vadjustment->value + offset, 0.0,
		 tree_view->priv->vadjustment->upper - tree_view->priv->vadjustment->page_size);
  btk_adjustment_set_value (tree_view->priv->vadjustment, value);
}

static gboolean
btk_tree_view_horizontal_autoscroll (BtkTreeView *tree_view)
{
  BdkRectangle visible_rect;
  gint x;
  gint offset;
  gfloat value;

  bdk_window_get_pointer (tree_view->priv->bin_window, &x, NULL, NULL);

  btk_tree_view_get_visible_rect (tree_view, &visible_rect);

  /* See if we are near the edge. */
  offset = x - (visible_rect.x + SCROLL_EDGE_SIZE);
  if (offset > 0)
    {
      offset = x - (visible_rect.x + visible_rect.width - SCROLL_EDGE_SIZE);
      if (offset < 0)
	return TRUE;
    }
  offset = offset/3;

  value = CLAMP (tree_view->priv->hadjustment->value + offset,
		 0.0, tree_view->priv->hadjustment->upper - tree_view->priv->hadjustment->page_size);
  btk_adjustment_set_value (tree_view->priv->hadjustment, value);

  return TRUE;

}

static gboolean
btk_tree_view_motion_drag_column (BtkWidget      *widget,
				  BdkEventMotion *event)
{
  BtkTreeView *tree_view = (BtkTreeView *) widget;
  BtkTreeViewColumn *column = tree_view->priv->drag_column;
  gint x, y;

  /* Sanity Check */
  if ((column == NULL) ||
      (event->window != tree_view->priv->drag_window))
    return FALSE;

  /* Handle moving the header */
  bdk_window_get_position (tree_view->priv->drag_window, &x, &y);
  x = CLAMP (x + (gint)event->x - column->drag_x, 0,
	     MAX (tree_view->priv->width, BTK_WIDGET (tree_view)->allocation.width) - column->button->allocation.width);
  bdk_window_move (tree_view->priv->drag_window, x, y);
  
  /* autoscroll, if needed */
  btk_tree_view_horizontal_autoscroll (tree_view);
  /* Update the current reorder position and arrow; */
  btk_tree_view_update_current_reorder (tree_view);

  return TRUE;
}

static void
btk_tree_view_stop_rubber_band (BtkTreeView *tree_view)
{
  remove_scroll_timeout (tree_view);
  btk_grab_remove (BTK_WIDGET (tree_view));

  if (tree_view->priv->rubber_band_status == RUBBER_BAND_ACTIVE)
    {
      BtkTreePath *tmp_path;

      btk_widget_queue_draw (BTK_WIDGET (tree_view));

      /* The anchor path should be set to the start path */
      tmp_path = _btk_tree_view_find_path (tree_view,
					   tree_view->priv->rubber_band_start_tree,
					   tree_view->priv->rubber_band_start_node);

      if (tree_view->priv->anchor)
	btk_tree_row_reference_free (tree_view->priv->anchor);

      tree_view->priv->anchor =
	btk_tree_row_reference_new_proxy (B_OBJECT (tree_view),
					  tree_view->priv->model,
					  tmp_path);

      btk_tree_path_free (tmp_path);

      /* ... and the cursor to the end path */
      tmp_path = _btk_tree_view_find_path (tree_view,
					   tree_view->priv->rubber_band_end_tree,
					   tree_view->priv->rubber_band_end_node);
      btk_tree_view_real_set_cursor (BTK_TREE_VIEW (tree_view), tmp_path, FALSE, FALSE);
      btk_tree_path_free (tmp_path);

      _btk_tree_selection_emit_changed (tree_view->priv->selection);
    }

  /* Clear status variables */
  tree_view->priv->rubber_band_status = RUBBER_BAND_OFF;
  tree_view->priv->rubber_band_extend = FALSE;
  tree_view->priv->rubber_band_modify = FALSE;

  tree_view->priv->rubber_band_start_node = NULL;
  tree_view->priv->rubber_band_start_tree = NULL;
  tree_view->priv->rubber_band_end_node = NULL;
  tree_view->priv->rubber_band_end_tree = NULL;
}

static void
btk_tree_view_update_rubber_band_selection_range (BtkTreeView *tree_view,
						 BtkRBTree   *start_tree,
						 BtkRBNode   *start_node,
						 BtkRBTree   *end_tree,
						 BtkRBNode   *end_node,
						 gboolean     select,
						 gboolean     skip_start,
						 gboolean     skip_end)
{
  if (start_node == end_node)
    return;

  /* We skip the first node and jump inside the loop */
  if (skip_start)
    goto skip_first;

  do
    {
      /* Small optimization by assuming insensitive nodes are never
       * selected.
       */
      if (!BTK_RBNODE_FLAG_SET (start_node, BTK_RBNODE_IS_SELECTED))
        {
	  BtkTreePath *path;
	  gboolean selectable;

	  path = _btk_tree_view_find_path (tree_view, start_tree, start_node);
	  selectable = _btk_tree_selection_row_is_selectable (tree_view->priv->selection, start_node, path);
	  btk_tree_path_free (path);

	  if (!selectable)
	    goto node_not_selectable;
	}

      if (select)
        {
	  if (tree_view->priv->rubber_band_extend)
            BTK_RBNODE_SET_FLAG (start_node, BTK_RBNODE_IS_SELECTED);
	  else if (tree_view->priv->rubber_band_modify)
	    {
	      /* Toggle the selection state */
	      if (BTK_RBNODE_FLAG_SET (start_node, BTK_RBNODE_IS_SELECTED))
		BTK_RBNODE_UNSET_FLAG (start_node, BTK_RBNODE_IS_SELECTED);
	      else
		BTK_RBNODE_SET_FLAG (start_node, BTK_RBNODE_IS_SELECTED);
	    }
	  else
	    BTK_RBNODE_SET_FLAG (start_node, BTK_RBNODE_IS_SELECTED);
	}
      else
        {
	  /* Mirror the above */
	  if (tree_view->priv->rubber_band_extend)
	    BTK_RBNODE_UNSET_FLAG (start_node, BTK_RBNODE_IS_SELECTED);
	  else if (tree_view->priv->rubber_band_modify)
	    {
	      /* Toggle the selection state */
	      if (BTK_RBNODE_FLAG_SET (start_node, BTK_RBNODE_IS_SELECTED))
		BTK_RBNODE_UNSET_FLAG (start_node, BTK_RBNODE_IS_SELECTED);
	      else
		BTK_RBNODE_SET_FLAG (start_node, BTK_RBNODE_IS_SELECTED);
	    }
	  else
	    BTK_RBNODE_UNSET_FLAG (start_node, BTK_RBNODE_IS_SELECTED);
	}

      _btk_tree_view_queue_draw_node (tree_view, start_tree, start_node, NULL);

node_not_selectable:
      if (start_node == end_node)
	break;

skip_first:

      if (start_node->children)
        {
	  start_tree = start_node->children;
	  start_node = start_tree->root;
	  while (start_node->left != start_tree->nil)
	    start_node = start_node->left;
	}
      else
        {
	  _btk_rbtree_next_full (start_tree, start_node, &start_tree, &start_node);

	  if (!start_tree)
	    /* Ran out of tree */
	    break;
	}

      if (skip_end && start_node == end_node)
	break;
    }
  while (TRUE);
}

static void
btk_tree_view_update_rubber_band_selection (BtkTreeView *tree_view)
{
  BtkRBTree *start_tree, *end_tree;
  BtkRBNode *start_node, *end_node;

  _btk_rbtree_find_offset (tree_view->priv->tree, MIN (tree_view->priv->press_start_y, tree_view->priv->rubber_band_y), &start_tree, &start_node);
  _btk_rbtree_find_offset (tree_view->priv->tree, MAX (tree_view->priv->press_start_y, tree_view->priv->rubber_band_y), &end_tree, &end_node);

  /* Handle the start area first */
  if (!tree_view->priv->rubber_band_start_node)
    {
      btk_tree_view_update_rubber_band_selection_range (tree_view,
						       start_tree,
						       start_node,
						       end_tree,
						       end_node,
						       TRUE,
						       FALSE,
						       FALSE);
    }
  else if (_btk_rbtree_node_find_offset (start_tree, start_node) <
           _btk_rbtree_node_find_offset (tree_view->priv->rubber_band_start_tree, tree_view->priv->rubber_band_start_node))
    {
      /* New node is above the old one; selection became bigger */
      btk_tree_view_update_rubber_band_selection_range (tree_view,
						       start_tree,
						       start_node,
						       tree_view->priv->rubber_band_start_tree,
						       tree_view->priv->rubber_band_start_node,
						       TRUE,
						       FALSE,
						       TRUE);
    }
  else if (_btk_rbtree_node_find_offset (start_tree, start_node) >
           _btk_rbtree_node_find_offset (tree_view->priv->rubber_band_start_tree, tree_view->priv->rubber_band_start_node))
    {
      /* New node is below the old one; selection became smaller */
      btk_tree_view_update_rubber_band_selection_range (tree_view,
						       tree_view->priv->rubber_band_start_tree,
						       tree_view->priv->rubber_band_start_node,
						       start_tree,
						       start_node,
						       FALSE,
						       FALSE,
						       TRUE);
    }

  tree_view->priv->rubber_band_start_tree = start_tree;
  tree_view->priv->rubber_band_start_node = start_node;

  /* Next, handle the end area */
  if (!tree_view->priv->rubber_band_end_node)
    {
      /* In the event this happens, start_node was also NULL; this case is
       * handled above.
       */
    }
  else if (!end_node)
    {
      /* Find the last node in the tree */
      _btk_rbtree_find_offset (tree_view->priv->tree, tree_view->priv->height - 1,
			       &end_tree, &end_node);

      /* Selection reached end of the tree */
      btk_tree_view_update_rubber_band_selection_range (tree_view,
						       tree_view->priv->rubber_band_end_tree,
						       tree_view->priv->rubber_band_end_node,
						       end_tree,
						       end_node,
						       TRUE,
						       TRUE,
						       FALSE);
    }
  else if (_btk_rbtree_node_find_offset (end_tree, end_node) >
           _btk_rbtree_node_find_offset (tree_view->priv->rubber_band_end_tree, tree_view->priv->rubber_band_end_node))
    {
      /* New node is below the old one; selection became bigger */
      btk_tree_view_update_rubber_band_selection_range (tree_view,
						       tree_view->priv->rubber_band_end_tree,
						       tree_view->priv->rubber_band_end_node,
						       end_tree,
						       end_node,
						       TRUE,
						       TRUE,
						       FALSE);
    }
  else if (_btk_rbtree_node_find_offset (end_tree, end_node) <
           _btk_rbtree_node_find_offset (tree_view->priv->rubber_band_end_tree, tree_view->priv->rubber_band_end_node))
    {
      /* New node is above the old one; selection became smaller */
      btk_tree_view_update_rubber_band_selection_range (tree_view,
						       end_tree,
						       end_node,
						       tree_view->priv->rubber_band_end_tree,
						       tree_view->priv->rubber_band_end_node,
						       FALSE,
						       TRUE,
						       FALSE);
    }

  tree_view->priv->rubber_band_end_tree = end_tree;
  tree_view->priv->rubber_band_end_node = end_node;
}

static void
btk_tree_view_update_rubber_band (BtkTreeView *tree_view)
{
  gint x, y;
  BdkRectangle old_area;
  BdkRectangle new_area;
  BdkRectangle common;
  BdkRebunnyion *invalid_rebunnyion;

  old_area.x = MIN (tree_view->priv->press_start_x, tree_view->priv->rubber_band_x);
  old_area.y = MIN (tree_view->priv->press_start_y, tree_view->priv->rubber_band_y) - tree_view->priv->dy;
  old_area.width = ABS (tree_view->priv->rubber_band_x - tree_view->priv->press_start_x) + 1;
  old_area.height = ABS (tree_view->priv->rubber_band_y - tree_view->priv->press_start_y) + 1;

  bdk_window_get_pointer (tree_view->priv->bin_window, &x, &y, NULL);

  x = MAX (x, 0);
  y = MAX (y, 0) + tree_view->priv->dy;

  new_area.x = MIN (tree_view->priv->press_start_x, x);
  new_area.y = MIN (tree_view->priv->press_start_y, y) - tree_view->priv->dy;
  new_area.width = ABS (x - tree_view->priv->press_start_x) + 1;
  new_area.height = ABS (y - tree_view->priv->press_start_y) + 1;

  invalid_rebunnyion = bdk_rebunnyion_rectangle (&old_area);
  bdk_rebunnyion_union_with_rect (invalid_rebunnyion, &new_area);

  bdk_rectangle_intersect (&old_area, &new_area, &common);
  if (common.width > 2 && common.height > 2)
    {
      BdkRebunnyion *common_rebunnyion;

      /* make sure the border is invalidated */
      common.x += 1;
      common.y += 1;
      common.width -= 2;
      common.height -= 2;

      common_rebunnyion = bdk_rebunnyion_rectangle (&common);

      bdk_rebunnyion_subtract (invalid_rebunnyion, common_rebunnyion);
      bdk_rebunnyion_destroy (common_rebunnyion);
    }

  bdk_window_invalidate_rebunnyion (tree_view->priv->bin_window, invalid_rebunnyion, TRUE);

  bdk_rebunnyion_destroy (invalid_rebunnyion);

  tree_view->priv->rubber_band_x = x;
  tree_view->priv->rubber_band_y = y;

  btk_tree_view_update_rubber_band_selection (tree_view);
}

static void
btk_tree_view_paint_rubber_band (BtkTreeView  *tree_view,
				BdkRectangle *area)
{
  bairo_t *cr;
  BdkRectangle rect;
  BdkRectangle rubber_rect;

  rubber_rect.x = MIN (tree_view->priv->press_start_x, tree_view->priv->rubber_band_x);
  rubber_rect.y = MIN (tree_view->priv->press_start_y, tree_view->priv->rubber_band_y) - tree_view->priv->dy;
  rubber_rect.width = ABS (tree_view->priv->press_start_x - tree_view->priv->rubber_band_x) + 1;
  rubber_rect.height = ABS (tree_view->priv->press_start_y - tree_view->priv->rubber_band_y) + 1;

  if (!bdk_rectangle_intersect (&rubber_rect, area, &rect))
    return;

  cr = bdk_bairo_create (tree_view->priv->bin_window);
  bairo_set_line_width (cr, 1.0);

  bairo_set_source_rgba (cr,
			 BTK_WIDGET (tree_view)->style->fg[BTK_STATE_NORMAL].red / 65535.,
			 BTK_WIDGET (tree_view)->style->fg[BTK_STATE_NORMAL].green / 65535.,
			 BTK_WIDGET (tree_view)->style->fg[BTK_STATE_NORMAL].blue / 65535.,
			 .25);

  bdk_bairo_rectangle (cr, &rect);
  bairo_clip (cr);
  bairo_paint (cr);

  bairo_set_source_rgb (cr,
			BTK_WIDGET (tree_view)->style->fg[BTK_STATE_NORMAL].red / 65535.,
			BTK_WIDGET (tree_view)->style->fg[BTK_STATE_NORMAL].green / 65535.,
			BTK_WIDGET (tree_view)->style->fg[BTK_STATE_NORMAL].blue / 65535.);

  bairo_rectangle (cr,
		   rubber_rect.x + 0.5, rubber_rect.y + 0.5,
		   rubber_rect.width - 1, rubber_rect.height - 1);
  bairo_stroke (cr);

  bairo_destroy (cr);
}

static gboolean
btk_tree_view_motion_bin_window (BtkWidget      *widget,
				 BdkEventMotion *event)
{
  BtkTreeView *tree_view;
  BtkRBTree *tree;
  BtkRBNode *node;
  gint new_y;

  tree_view = (BtkTreeView *) widget;

  if (tree_view->priv->tree == NULL)
    return FALSE;

  if (tree_view->priv->rubber_band_status == RUBBER_BAND_MAYBE_START)
    {
      btk_grab_add (BTK_WIDGET (tree_view));
      btk_tree_view_update_rubber_band (tree_view);

      tree_view->priv->rubber_band_status = RUBBER_BAND_ACTIVE;
    }
  else if (tree_view->priv->rubber_band_status == RUBBER_BAND_ACTIVE)
    {
      btk_tree_view_update_rubber_band (tree_view);

      add_scroll_timeout (tree_view);
    }

  /* only check for an initiated drag when a button is pressed */
  if (tree_view->priv->pressed_button >= 0
      && !tree_view->priv->rubber_band_status)
    btk_tree_view_maybe_begin_dragging_row (tree_view, event);

  new_y = TREE_WINDOW_Y_TO_RBTREE_Y(tree_view, event->y);
  if (new_y < 0)
    new_y = 0;

  _btk_rbtree_find_offset (tree_view->priv->tree, new_y, &tree, &node);

  /* If we are currently pressing down a button, we don't want to prelight anything else. */
  if ((tree_view->priv->button_pressed_node != NULL) &&
      (tree_view->priv->button_pressed_node != node))
    node = NULL;

  tree_view->priv->event_last_x = event->x;
  tree_view->priv->event_last_y = event->y;

  prelight_or_select (tree_view, tree, node, event->x, event->y);

  return TRUE;
}

static gboolean
btk_tree_view_motion (BtkWidget      *widget,
		      BdkEventMotion *event)
{
  BtkTreeView *tree_view;

  tree_view = (BtkTreeView *) widget;

  /* Resizing a column */
  if (BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_IN_COLUMN_RESIZE))
    return btk_tree_view_motion_resize_column (widget, event);

  /* Drag column */
  if (BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_IN_COLUMN_DRAG))
    return btk_tree_view_motion_drag_column (widget, event);

  /* Sanity check it */
  if (event->window == tree_view->priv->bin_window)
    return btk_tree_view_motion_bin_window (widget, event);

  return FALSE;
}

/* Invalidate the focus rectangle near the edge of the bin_window; used when
 * the tree is empty.
 */
static void
invalidate_empty_focus (BtkTreeView *tree_view)
{
  BdkRectangle area;

  if (!btk_widget_has_focus (BTK_WIDGET (tree_view)))
    return;

  area.x = 0;
  area.y = 0;
  area.width = bdk_window_get_width (tree_view->priv->bin_window);
  area.height = bdk_window_get_height (tree_view->priv->bin_window);
  bdk_window_invalidate_rect (tree_view->priv->bin_window, &area, FALSE);
}

/* Draws a focus rectangle near the edge of the bin_window; used when the tree
 * is empty.
 */
static void
draw_empty_focus (BtkTreeView *tree_view, BdkRectangle *clip_area)
{
  BtkWidget *widget = BTK_WIDGET (tree_view);
  gint w, h;

  if (!btk_widget_has_focus (widget))
    return;

  w = bdk_window_get_width (tree_view->priv->bin_window);
  h = bdk_window_get_height (tree_view->priv->bin_window);

  w -= 2;
  h -= 2;

  if (w > 0 && h > 0)
    btk_paint_focus (btk_widget_get_style (widget),
		     tree_view->priv->bin_window,
		     btk_widget_get_state (widget),
		     clip_area,
		     widget,
		     NULL,
		     1, 1, w, h);
}

typedef enum {
  BTK_TREE_VIEW_GRID_LINE,
  BTK_TREE_VIEW_TREE_LINE,
  BTK_TREE_VIEW_FOREGROUND_LINE
} BtkTreeViewLineType;

static void
btk_tree_view_draw_line (BtkTreeView         *tree_view,
                         BdkWindow           *window,
                         BtkTreeViewLineType  type,
                         int                  x1,
                         int                  y1,
                         int                  x2,
                         int                  y2)
{
  bairo_t *cr;

  cr = bdk_bairo_create (window);

  switch (type)
    {
    case BTK_TREE_VIEW_TREE_LINE:
      bairo_set_source_rgb (cr, 0, 0, 0);
      bairo_set_line_width (cr, tree_view->priv->tree_line_width);
      if (tree_view->priv->tree_line_dashes[0])
        bairo_set_dash (cr, 
                        tree_view->priv->tree_line_dashes,
                        2, 0.5);
      break;
    case BTK_TREE_VIEW_GRID_LINE:
      bairo_set_source_rgb (cr, 0, 0, 0);
      bairo_set_line_width (cr, tree_view->priv->grid_line_width);
      if (tree_view->priv->grid_line_dashes[0])
        bairo_set_dash (cr, 
                        tree_view->priv->grid_line_dashes,
                        2, 0.5);
      break;
    default:
      g_assert_not_reached ();
      /* fall through */
    case BTK_TREE_VIEW_FOREGROUND_LINE:
      bairo_set_line_width (cr, 1.0);
      bdk_bairo_set_source_color (cr,
          &BTK_WIDGET (tree_view)->style->fg[btk_widget_get_state (BTK_WIDGET (tree_view))]);
      break;
    }

  bairo_move_to (cr, x1 + 0.5, y1 + 0.5);
  bairo_line_to (cr, x2 + 0.5, y2 + 0.5);
  bairo_stroke (cr);

  bairo_destroy (cr);
}
                         
static void
btk_tree_view_draw_grid_lines (BtkTreeView    *tree_view,
			       BdkEventExpose *event,
			       gint            n_visible_columns)
{
  GList *list = tree_view->priv->columns;
  gint i = 0;
  gint current_x = 0;

  if (tree_view->priv->grid_lines != BTK_TREE_VIEW_GRID_LINES_VERTICAL
      && tree_view->priv->grid_lines != BTK_TREE_VIEW_GRID_LINES_BOTH)
    return;

  /* Only draw the lines for visible rows and columns */
  for (list = tree_view->priv->columns; list; list = list->next)
    {
      BtkTreeViewColumn *column = list->data;

      /* We don't want a line for the last column */
      if (i == n_visible_columns - 1)
	break;

      if (! column->visible)
	continue;

      i++;

      current_x += column->width;

      btk_tree_view_draw_line (tree_view, event->window,
                               BTK_TREE_VIEW_GRID_LINE,
                               current_x - 1, 0,
                               current_x - 1, tree_view->priv->height);
    }
}

/* Warning: Very scary function.
 * Modify at your own risk
 *
 * KEEP IN SYNC WITH btk_tree_view_create_row_drag_icon()!
 * FIXME: It's not...
 */
static gboolean
btk_tree_view_bin_expose (BtkWidget      *widget,
			  BdkEventExpose *event)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (widget);
  BtkTreePath *path;
  BtkRBTree *tree;
  GList *list;
  BtkRBNode *node;
  BtkRBNode *cursor = NULL;
  BtkRBTree *cursor_tree = NULL;
  BtkRBNode *drag_highlight = NULL;
  BtkRBTree *drag_highlight_tree = NULL;
  BtkTreeIter iter;
  gint new_y;
  gint y_offset, cell_offset;
  gint max_height;
  gint depth;
  BdkRectangle background_area;
  BdkRectangle cell_area;
  guint flags;
  gint highlight_x;
  gint expander_cell_width;
  gint bin_window_width;
  gint bin_window_height;
  BtkTreePath *cursor_path;
  BtkTreePath *drag_dest_path;
  GList *first_column, *last_column;
  gint vertical_separator;
  gint horizontal_separator;
  gint focus_line_width;
  gboolean allow_rules;
  gboolean has_special_cell;
  gboolean rtl;
  gint n_visible_columns;
  gint pointer_x, pointer_y;
  gint grid_line_width;
  gboolean got_pointer = FALSE;
  gboolean row_ending_details;
  gboolean draw_vgrid_lines, draw_hgrid_lines;

  rtl = (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL);

  btk_widget_style_get (widget,
			"horizontal-separator", &horizontal_separator,
			"vertical-separator", &vertical_separator,
			"allow-rules", &allow_rules,
			"focus-line-width", &focus_line_width,
			"row-ending-details", &row_ending_details,
			NULL);

  if (tree_view->priv->tree == NULL)
    {
      draw_empty_focus (tree_view, &event->area);
      return TRUE;
    }

  /* clip event->area to the visible area */
  if (event->area.height < 0)
    return TRUE;

  validate_visible_area (tree_view);

  new_y = TREE_WINDOW_Y_TO_RBTREE_Y (tree_view, event->area.y);

  if (new_y < 0)
    new_y = 0;
  y_offset = -_btk_rbtree_find_offset (tree_view->priv->tree, new_y, &tree, &node);
  bin_window_width = bdk_window_get_width (tree_view->priv->bin_window);
  bin_window_height = bdk_window_get_height (tree_view->priv->bin_window);

  if (tree_view->priv->height < bin_window_height)
    {
      btk_paint_flat_box (widget->style,
                          event->window,
                          widget->state,
                          BTK_SHADOW_NONE,
                          &event->area,
                          widget,
                          "cell_even",
                          0, tree_view->priv->height,
                          bin_window_width,
                          bin_window_height - tree_view->priv->height);
    }

  if (node == NULL)
    return TRUE;

  /* find the path for the node */
  path = _btk_tree_view_find_path ((BtkTreeView *)widget,
				   tree,
				   node);
  btk_tree_model_get_iter (tree_view->priv->model,
			   &iter,
			   path);
  depth = btk_tree_path_get_depth (path);
  btk_tree_path_free (path);
  
  cursor_path = NULL;
  drag_dest_path = NULL;

  if (tree_view->priv->cursor)
    cursor_path = btk_tree_row_reference_get_path (tree_view->priv->cursor);

  if (cursor_path)
    _btk_tree_view_find_node (tree_view, cursor_path,
                              &cursor_tree, &cursor);

  if (tree_view->priv->drag_dest_row)
    drag_dest_path = btk_tree_row_reference_get_path (tree_view->priv->drag_dest_row);

  if (drag_dest_path)
    _btk_tree_view_find_node (tree_view, drag_dest_path,
                              &drag_highlight_tree, &drag_highlight);

  draw_vgrid_lines =
    tree_view->priv->grid_lines == BTK_TREE_VIEW_GRID_LINES_VERTICAL
    || tree_view->priv->grid_lines == BTK_TREE_VIEW_GRID_LINES_BOTH;
  draw_hgrid_lines =
    tree_view->priv->grid_lines == BTK_TREE_VIEW_GRID_LINES_HORIZONTAL
    || tree_view->priv->grid_lines == BTK_TREE_VIEW_GRID_LINES_BOTH;

  if (draw_vgrid_lines || draw_hgrid_lines)
    btk_widget_style_get (widget, "grid-line-width", &grid_line_width, NULL);
  
  n_visible_columns = 0;
  for (list = tree_view->priv->columns; list; list = list->next)
    {
      if (! BTK_TREE_VIEW_COLUMN (list->data)->visible)
	continue;
      n_visible_columns ++;
    }

  /* Find the last column */
  for (last_column = g_list_last (tree_view->priv->columns);
       last_column && !(BTK_TREE_VIEW_COLUMN (last_column->data)->visible);
       last_column = last_column->prev)
    ;

  /* and the first */
  for (first_column = g_list_first (tree_view->priv->columns);
       first_column && !(BTK_TREE_VIEW_COLUMN (first_column->data)->visible);
       first_column = first_column->next)
    ;

  /* Actually process the expose event.  To do this, we want to
   * start at the first node of the event, and walk the tree in
   * order, drawing each successive node.
   */

  do
    {
      gboolean parity;
      gboolean is_separator = FALSE;
      gboolean is_first = FALSE;
      gboolean is_last = FALSE;
      
      is_separator = row_is_separator (tree_view, &iter, NULL);

      max_height = ROW_HEIGHT (tree_view, BACKGROUND_HEIGHT (node));

      cell_offset = 0;
      highlight_x = 0; /* should match x coord of first cell */
      expander_cell_width = 0;

      background_area.y = y_offset + event->area.y;
      background_area.height = max_height;

      flags = 0;

      if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_PRELIT))
	flags |= BTK_CELL_RENDERER_PRELIT;

      if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SELECTED))
        flags |= BTK_CELL_RENDERER_SELECTED;

      parity = _btk_rbtree_node_find_parity (tree, node);

      /* we *need* to set cell data on all cells before the call
       * to _has_special_cell, else _has_special_cell() does not
       * return a correct value.
       */
      for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
	   list;
	   list = (rtl ? list->prev : list->next))
        {
	  BtkTreeViewColumn *column = list->data;
	  btk_tree_view_column_cell_set_cell_data (column,
						   tree_view->priv->model,
						   &iter,
						   BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_PARENT),
						   node->children?TRUE:FALSE);
        }

      has_special_cell = btk_tree_view_has_special_cell (tree_view);

      for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
	   list;
	   list = (rtl ? list->prev : list->next))
	{
	  BtkTreeViewColumn *column = list->data;
	  const gchar *detail = NULL;
	  BtkStateType state;

	  if (!column->visible)
            continue;

	  if (cell_offset > event->area.x + event->area.width ||
	      cell_offset + column->width < event->area.x)
	    {
	      cell_offset += column->width;
	      continue;
	    }

          if (column->show_sort_indicator)
	    flags |= BTK_CELL_RENDERER_SORTED;
          else
            flags &= ~BTK_CELL_RENDERER_SORTED;

	  if (cursor == node)
            flags |= BTK_CELL_RENDERER_FOCUSED;
          else
            flags &= ~BTK_CELL_RENDERER_FOCUSED;

	  background_area.x = cell_offset;
	  background_area.width = column->width;

          cell_area = background_area;
          cell_area.y += vertical_separator / 2;
          cell_area.x += horizontal_separator / 2;
          cell_area.height -= vertical_separator;
	  cell_area.width -= horizontal_separator;

	  if (draw_vgrid_lines)
	    {
	      if (list == first_column)
	        {
		  cell_area.width -= grid_line_width / 2;
		}
	      else if (list == last_column)
	        {
		  cell_area.x += grid_line_width / 2;
		  cell_area.width -= grid_line_width / 2;
		}
	      else
	        {
	          cell_area.x += grid_line_width / 2;
	          cell_area.width -= grid_line_width;
		}
	    }

	  if (draw_hgrid_lines)
	    {
	      cell_area.y += grid_line_width / 2;
	      cell_area.height -= grid_line_width;
	    }

	  if (bdk_rebunnyion_rect_in (event->rebunnyion, &background_area) == BDK_OVERLAP_RECTANGLE_OUT)
	    {
	      cell_offset += column->width;
	      continue;
	    }

	  btk_tree_view_column_cell_set_cell_data (column,
						   tree_view->priv->model,
						   &iter,
						   BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_PARENT),
						   node->children?TRUE:FALSE);

          /* Select the detail for drawing the cell.  relevant
           * factors are parity, sortedness, and whether to
           * display rules.
           */
          if (allow_rules && tree_view->priv->has_rules)
            {
              if ((flags & BTK_CELL_RENDERER_SORTED) &&
		  n_visible_columns >= 3)
                {
                  if (parity)
                    detail = "cell_odd_ruled_sorted";
                  else
                    detail = "cell_even_ruled_sorted";
                }
              else
                {
                  if (parity)
                    detail = "cell_odd_ruled";
                  else
                    detail = "cell_even_ruled";
                }
            }
          else
            {
              if ((flags & BTK_CELL_RENDERER_SORTED) &&
		  n_visible_columns >= 3)
                {
                  if (parity)
                    detail = "cell_odd_sorted";
                  else
                    detail = "cell_even_sorted";
                }
              else
                {
                  if (parity)
                    detail = "cell_odd";
                  else
                    detail = "cell_even";
                }
            }

          g_assert (detail);

	  if (widget->state == BTK_STATE_INSENSITIVE)
	    state = BTK_STATE_INSENSITIVE;	    
	  else if (flags & BTK_CELL_RENDERER_SELECTED)
	    state = BTK_STATE_SELECTED;
	  else
	    state = BTK_STATE_NORMAL;

	  /* Draw background */
	  if (row_ending_details)
	    {
	      char new_detail[128];

	      is_first = (rtl ? !list->next : !list->prev);
	      is_last = (rtl ? !list->prev : !list->next);

	      /* (I don't like the snprintfs either, but couldn't find a
	       * less messy way).
	       */
	      if (is_first && is_last)
		g_snprintf (new_detail, 127, "%s", detail);
	      else if (is_first)
		g_snprintf (new_detail, 127, "%s_start", detail);
	      else if (is_last)
		g_snprintf (new_detail, 127, "%s_end", detail);
	      else
		g_snprintf (new_detail, 128, "%s_middle", detail);

	      btk_paint_flat_box (widget->style,
				  event->window,
				  state,
				  BTK_SHADOW_NONE,
				  &event->area,
				  widget,
				  new_detail,
				  background_area.x,
				  background_area.y,
				  background_area.width,
				  background_area.height);
	    }
	  else
	    {
	      btk_paint_flat_box (widget->style,
				  event->window,
				  state,
				  BTK_SHADOW_NONE,
				  &event->area,
				  widget,
				  detail,
				  background_area.x,
				  background_area.y,
				  background_area.width,
				  background_area.height);
	    }

	  if (btk_tree_view_is_expander_column (tree_view, column))
	    {
	      if (!rtl)
		cell_area.x += (depth - 1) * tree_view->priv->level_indentation;
	      cell_area.width -= (depth - 1) * tree_view->priv->level_indentation;

              if (TREE_VIEW_DRAW_EXPANDERS(tree_view))
	        {
	          if (!rtl)
		    cell_area.x += depth * tree_view->priv->expander_size;
		  cell_area.width -= depth * tree_view->priv->expander_size;
		}

              /* If we have an expander column, the highlight underline
               * starts with that column, so that it indicates which
               * level of the tree we're dropping at.
               */
              highlight_x = cell_area.x;
	      expander_cell_width = cell_area.width;

	      if (is_separator)
		btk_paint_hline (widget->style,
				 event->window,
				 state,
				 &cell_area,
				 widget,
				 NULL,
				 cell_area.x,
				 cell_area.x + cell_area.width,
				 cell_area.y + cell_area.height / 2);
	      else
		_btk_tree_view_column_cell_render (column,
						   event->window,
						   &background_area,
						   &cell_area,
						   &event->area,
						   flags);
	      if (TREE_VIEW_DRAW_EXPANDERS(tree_view)
		  && (node->flags & BTK_RBNODE_IS_PARENT) == BTK_RBNODE_IS_PARENT)
		{
		  if (!got_pointer)
		    {
		      bdk_window_get_pointer (tree_view->priv->bin_window, 
					      &pointer_x, &pointer_y, NULL);
		      got_pointer = TRUE;
		    }

		  btk_tree_view_draw_arrow (BTK_TREE_VIEW (widget),
					    tree,
					    node,
					    pointer_x, pointer_y);
		}
	    }
	  else
	    {
	      if (is_separator)
		btk_paint_hline (widget->style,
				 event->window,
				 state,
				 &cell_area,
				 widget,
				 NULL,
				 cell_area.x,
				 cell_area.x + cell_area.width,
				 cell_area.y + cell_area.height / 2);
	      else
		_btk_tree_view_column_cell_render (column,
						   event->window,
						   &background_area,
						   &cell_area,
						   &event->area,
						   flags);
	    }

	  if (draw_hgrid_lines)
	    {
	      if (background_area.y > 0)
                btk_tree_view_draw_line (tree_view, event->window,
                                         BTK_TREE_VIEW_GRID_LINE,
                                         background_area.x, background_area.y,
                                         background_area.x + background_area.width,
			                 background_area.y);

	      if (y_offset + max_height >= event->area.height)
                btk_tree_view_draw_line (tree_view, event->window,
                                         BTK_TREE_VIEW_GRID_LINE,
                                         background_area.x, background_area.y + max_height,
                                         background_area.x + background_area.width,
			                 background_area.y + max_height);
	    }

	  if (btk_tree_view_is_expander_column (tree_view, column) &&
	      tree_view->priv->tree_lines_enabled)
	    {
	      gint x = background_area.x;
	      gint mult = rtl ? -1 : 1;
	      gint y0 = background_area.y;
	      gint y1 = background_area.y + background_area.height/2;
	      gint y2 = background_area.y + background_area.height;

	      if (rtl)
		x += background_area.width - 1;

	      if ((node->flags & BTK_RBNODE_IS_PARENT) == BTK_RBNODE_IS_PARENT
		  && depth > 1)
	        {
                  btk_tree_view_draw_line (tree_view, event->window,
                                           BTK_TREE_VIEW_TREE_LINE,
                                           x + tree_view->priv->expander_size * (depth - 1.5) * mult,
                                           y1,
                                           x + tree_view->priv->expander_size * (depth - 1.1) * mult,
                                           y1);
	        }
	      else if (depth > 1)
	        {
                  btk_tree_view_draw_line (tree_view, event->window,
                                           BTK_TREE_VIEW_TREE_LINE,
                                           x + tree_view->priv->expander_size * (depth - 1.5) * mult,
                                           y1,
                                           x + tree_view->priv->expander_size * (depth - 0.5) * mult,
                                           y1);
		}

	      if (depth > 1)
	        {
		  gint i;
		  BtkRBNode *tmp_node;
		  BtkRBTree *tmp_tree;

	          if (!_btk_rbtree_next (tree, node))
                    btk_tree_view_draw_line (tree_view, event->window,
                                             BTK_TREE_VIEW_TREE_LINE,
                                             x + tree_view->priv->expander_size * (depth - 1.5) * mult,
                                             y0,
                                             x + tree_view->priv->expander_size * (depth - 1.5) * mult,
                                             y1);
		  else
                    btk_tree_view_draw_line (tree_view, event->window,
                                             BTK_TREE_VIEW_TREE_LINE,
                                             x + tree_view->priv->expander_size * (depth - 1.5) * mult,
                                             y0,
                                             x + tree_view->priv->expander_size * (depth - 1.5) * mult,
                                             y2);

		  tmp_node = tree->parent_node;
		  tmp_tree = tree->parent_tree;

		  for (i = depth - 2; i > 0; i--)
		    {
	              if (_btk_rbtree_next (tmp_tree, tmp_node))
                        btk_tree_view_draw_line (tree_view, event->window,
                                                 BTK_TREE_VIEW_TREE_LINE,
                                                 x + tree_view->priv->expander_size * (i - 0.5) * mult,
                                                 y0,
                                                 x + tree_view->priv->expander_size * (i - 0.5) * mult,
                                                 y2);

		      tmp_node = tmp_tree->parent_node;
		      tmp_tree = tmp_tree->parent_tree;
		    }
		}
	    }

	  if (node == cursor && has_special_cell &&
	      ((column == tree_view->priv->focus_column &&
		BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_DRAW_KEYFOCUS) &&
		btk_widget_has_focus (widget)) ||
	       (column == tree_view->priv->edited_column)))
	    {
	      _btk_tree_view_column_cell_draw_focus (column,
						     event->window,
						     &background_area,
						     &cell_area,
						     &event->area,
						     flags);
	    }

	  cell_offset += column->width;
	}

      if (node == drag_highlight)
        {
          /* Draw indicator for the drop
           */
          gint highlight_y = -1;
	  BtkRBTree *tree = NULL;
	  BtkRBNode *node = NULL;
	  gint width;

          switch (tree_view->priv->drag_dest_pos)
            {
            case BTK_TREE_VIEW_DROP_BEFORE:
              highlight_y = background_area.y - 1;
	      if (highlight_y < 0)
		      highlight_y = 0;
              break;

            case BTK_TREE_VIEW_DROP_AFTER:
              highlight_y = background_area.y + background_area.height - 1;
              break;

            case BTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
            case BTK_TREE_VIEW_DROP_INTO_OR_AFTER:
	      _btk_tree_view_find_node (tree_view, drag_dest_path, &tree, &node);

	      if (tree == NULL)
		break;
              width = bdk_window_get_width (tree_view->priv->bin_window);

	      if (row_ending_details)
		btk_paint_focus (widget->style,
			         tree_view->priv->bin_window,
				 btk_widget_get_state (widget),
				 &event->area,
				 widget,
				 (is_first
				  ? (is_last ? "treeview-drop-indicator" : "treeview-drop-indicator-left" )
				  : (is_last ? "treeview-drop-indicator-right" : "tree-view-drop-indicator-middle" )),
				 0, BACKGROUND_FIRST_PIXEL (tree_view, tree, node)
				 - focus_line_width / 2,
				 width, ROW_HEIGHT (tree_view, BACKGROUND_HEIGHT (node))
			       - focus_line_width + 1);
	      else
		btk_paint_focus (widget->style,
			         tree_view->priv->bin_window,
				 btk_widget_get_state (widget),
				 &event->area,
				 widget,
				 "treeview-drop-indicator",
				 0, BACKGROUND_FIRST_PIXEL (tree_view, tree, node)
				 - focus_line_width / 2,
				 width, ROW_HEIGHT (tree_view, BACKGROUND_HEIGHT (node))
				 - focus_line_width + 1);
              break;
            }

          if (highlight_y >= 0)
            {
              btk_tree_view_draw_line (tree_view, event->window,
                                       BTK_TREE_VIEW_FOREGROUND_LINE,
                                       rtl ? highlight_x + expander_cell_width : highlight_x,
                                       highlight_y,
                                       rtl ? 0 : bin_window_width,
                                       highlight_y);
            }
        }

      /* draw the big row-spanning focus rectangle, if needed */
      if (!has_special_cell && node == cursor &&
	  BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_DRAW_KEYFOCUS) &&
	  btk_widget_has_focus (widget))
        {
	  gint tmp_y, tmp_height;
	  gint width;
	  BtkStateType focus_rect_state;

	  focus_rect_state =
	    flags & BTK_CELL_RENDERER_SELECTED ? BTK_STATE_SELECTED :
	    (flags & BTK_CELL_RENDERER_PRELIT ? BTK_STATE_PRELIGHT :
	     (flags & BTK_CELL_RENDERER_INSENSITIVE ? BTK_STATE_INSENSITIVE :
	      BTK_STATE_NORMAL));

          width = bdk_window_get_width (tree_view->priv->bin_window);
	  
	  if (draw_hgrid_lines)
	    {
	      tmp_y = BACKGROUND_FIRST_PIXEL (tree_view, tree, node) + grid_line_width / 2;
	      tmp_height = ROW_HEIGHT (tree_view, BACKGROUND_HEIGHT (node)) - grid_line_width;
	    }
	  else
	    {
	      tmp_y = BACKGROUND_FIRST_PIXEL (tree_view, tree, node);
	      tmp_height = ROW_HEIGHT (tree_view, BACKGROUND_HEIGHT (node));
	    }

	  if (row_ending_details)
	    btk_paint_focus (widget->style,
			     tree_view->priv->bin_window,
			     focus_rect_state,
			     &event->area,
			     widget,
			     (is_first
			      ? (is_last ? "treeview" : "treeview-left" )
			      : (is_last ? "treeview-right" : "treeview-middle" )),
			     0, tmp_y,
			     width, tmp_height);
	  else
	    btk_paint_focus (widget->style,
			     tree_view->priv->bin_window,
			     focus_rect_state,
			     &event->area,
			     widget,
			     "treeview",
			     0, tmp_y,
			     width, tmp_height);
	}

      y_offset += max_height;
      if (node->children)
	{
	  BtkTreeIter parent = iter;
	  gboolean has_child;

	  tree = node->children;
	  node = tree->root;

          g_assert (node != tree->nil);

	  while (node->left != tree->nil)
	    node = node->left;
	  has_child = btk_tree_model_iter_children (tree_view->priv->model,
						    &iter,
						    &parent);
	  depth++;

	  /* Sanity Check! */
	  TREE_VIEW_INTERNAL_ASSERT (has_child, FALSE);
	}
      else
	{
	  gboolean done = FALSE;

	  do
	    {
	      node = _btk_rbtree_next (tree, node);
	      if (node != NULL)
		{
		  gboolean has_next = btk_tree_model_iter_next (tree_view->priv->model, &iter);
		  done = TRUE;

		  /* Sanity Check! */
		  TREE_VIEW_INTERNAL_ASSERT (has_next, FALSE);
		}
	      else
		{
		  BtkTreeIter parent_iter = iter;
		  gboolean has_parent;

		  node = tree->parent_node;
		  tree = tree->parent_tree;
		  if (tree == NULL)
		    /* we should go to done to free some memory */
		    goto done;
		  has_parent = btk_tree_model_iter_parent (tree_view->priv->model,
							   &iter,
							   &parent_iter);
		  depth--;

		  /* Sanity check */
		  TREE_VIEW_INTERNAL_ASSERT (has_parent, FALSE);
		}
	    }
	  while (!done);
	}
    }
  while (y_offset < event->area.height);

done:
  btk_tree_view_draw_grid_lines (tree_view, event, n_visible_columns);

 if (tree_view->priv->rubber_band_status == RUBBER_BAND_ACTIVE)
   {
     BdkRectangle *rectangles;
     gint n_rectangles;

     bdk_rebunnyion_get_rectangles (event->rebunnyion,
				&rectangles,
				&n_rectangles);

     while (n_rectangles--)
       btk_tree_view_paint_rubber_band (tree_view, &rectangles[n_rectangles]);

     g_free (rectangles);
   }

  if (cursor_path)
    btk_tree_path_free (cursor_path);

  if (drag_dest_path)
    btk_tree_path_free (drag_dest_path);

  return FALSE;
}

static gboolean
btk_tree_view_expose (BtkWidget      *widget,
		      BdkEventExpose *event)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (widget);

  if (event->window == tree_view->priv->bin_window)
    {
      gboolean retval;
      GList *tmp_list;

      retval = btk_tree_view_bin_expose (widget, event);

      /* We can't just chain up to Container::expose as it will try to send the
       * event to the headers, so we handle propagating it to our children
       * (eg. widgets being edited) ourselves.
       */
      tmp_list = tree_view->priv->children;
      while (tmp_list)
	{
	  BtkTreeViewChild *child = tmp_list->data;
	  tmp_list = tmp_list->next;

	  btk_container_propagate_expose (BTK_CONTAINER (tree_view), child->widget, event);
	}

      return retval;
    }

  else if (event->window == tree_view->priv->header_window)
    {
      GList *list;
      
      for (list = tree_view->priv->columns; list != NULL; list = list->next)
	{
	  BtkTreeViewColumn *column = list->data;

	  if (column == tree_view->priv->drag_column)
	    continue;

	  if (column->visible)
	    btk_container_propagate_expose (BTK_CONTAINER (tree_view),
					    column->button,
					    event);
	}
    }
  else if (event->window == tree_view->priv->drag_window)
    {
      btk_container_propagate_expose (BTK_CONTAINER (tree_view),
				      tree_view->priv->drag_column->button,
				      event);
    }
  return TRUE;
}

enum
{
  DROP_HOME,
  DROP_RIGHT,
  DROP_LEFT,
  DROP_END
};

/* returns 0x1 when no column has been found -- yes it's hackish */
static BtkTreeViewColumn *
btk_tree_view_get_drop_column (BtkTreeView       *tree_view,
			       BtkTreeViewColumn *column,
			       gint               drop_position)
{
  BtkTreeViewColumn *left_column = NULL;
  BtkTreeViewColumn *cur_column = NULL;
  GList *tmp_list;

  if (!column->reorderable)
    return (BtkTreeViewColumn *)0x1;

  switch (drop_position)
    {
      case DROP_HOME:
	/* find first column where we can drop */
	tmp_list = tree_view->priv->columns;
	if (column == BTK_TREE_VIEW_COLUMN (tmp_list->data))
	  return (BtkTreeViewColumn *)0x1;

	while (tmp_list)
	  {
	    g_assert (tmp_list);

	    cur_column = BTK_TREE_VIEW_COLUMN (tmp_list->data);
	    tmp_list = tmp_list->next;

	    if (left_column && left_column->visible == FALSE)
	      continue;

	    if (!tree_view->priv->column_drop_func)
	      return left_column;

	    if (!tree_view->priv->column_drop_func (tree_view, column, left_column, cur_column, tree_view->priv->column_drop_func_data))
	      {
		left_column = cur_column;
		continue;
	      }

	    return left_column;
	  }

	if (!tree_view->priv->column_drop_func)
	  return left_column;

	if (tree_view->priv->column_drop_func (tree_view, column, left_column, NULL, tree_view->priv->column_drop_func_data))
	  return left_column;
	else
	  return (BtkTreeViewColumn *)0x1;
	break;

      case DROP_RIGHT:
	/* find first column after column where we can drop */
	tmp_list = tree_view->priv->columns;

	for (; tmp_list; tmp_list = tmp_list->next)
	  if (BTK_TREE_VIEW_COLUMN (tmp_list->data) == column)
	    break;

	if (!tmp_list || !tmp_list->next)
	  return (BtkTreeViewColumn *)0x1;

	tmp_list = tmp_list->next;
	left_column = BTK_TREE_VIEW_COLUMN (tmp_list->data);
	tmp_list = tmp_list->next;

	while (tmp_list)
	  {
	    g_assert (tmp_list);

	    cur_column = BTK_TREE_VIEW_COLUMN (tmp_list->data);
	    tmp_list = tmp_list->next;

	    if (left_column && left_column->visible == FALSE)
	      {
		left_column = cur_column;
		if (tmp_list)
		  tmp_list = tmp_list->next;
	        continue;
	      }

	    if (!tree_view->priv->column_drop_func)
	      return left_column;

	    if (!tree_view->priv->column_drop_func (tree_view, column, left_column, cur_column, tree_view->priv->column_drop_func_data))
	      {
		left_column = cur_column;
		continue;
	      }

	    return left_column;
	  }

	if (!tree_view->priv->column_drop_func)
	  return left_column;

	if (tree_view->priv->column_drop_func (tree_view, column, left_column, NULL, tree_view->priv->column_drop_func_data))
	  return left_column;
	else
	  return (BtkTreeViewColumn *)0x1;
	break;

      case DROP_LEFT:
	/* find first column before column where we can drop */
	tmp_list = tree_view->priv->columns;

	for (; tmp_list; tmp_list = tmp_list->next)
	  if (BTK_TREE_VIEW_COLUMN (tmp_list->data) == column)
	    break;

	if (!tmp_list || !tmp_list->prev)
	  return (BtkTreeViewColumn *)0x1;

	tmp_list = tmp_list->prev;
	cur_column = BTK_TREE_VIEW_COLUMN (tmp_list->data);
	tmp_list = tmp_list->prev;

	while (tmp_list)
	  {
	    g_assert (tmp_list);

	    left_column = BTK_TREE_VIEW_COLUMN (tmp_list->data);

	    if (left_column && !left_column->visible)
	      {
		/*if (!tmp_list->prev)
		  return (BtkTreeViewColumn *)0x1;
		  */
/*
		cur_column = BTK_TREE_VIEW_COLUMN (tmp_list->prev->data);
		tmp_list = tmp_list->prev->prev;
		continue;*/

		cur_column = left_column;
		if (tmp_list)
		  tmp_list = tmp_list->prev;
		continue;
	      }

	    if (!tree_view->priv->column_drop_func)
	      return left_column;

	    if (tree_view->priv->column_drop_func (tree_view, column, left_column, cur_column, tree_view->priv->column_drop_func_data))
	      return left_column;

	    cur_column = left_column;
	    tmp_list = tmp_list->prev;
	  }

	if (!tree_view->priv->column_drop_func)
	  return NULL;

	if (tree_view->priv->column_drop_func (tree_view, column, NULL, cur_column, tree_view->priv->column_drop_func_data))
	  return NULL;
	else
	  return (BtkTreeViewColumn *)0x1;
	break;

      case DROP_END:
	/* same as DROP_HOME case, but doing it backwards */
	tmp_list = g_list_last (tree_view->priv->columns);
	cur_column = NULL;

	if (column == BTK_TREE_VIEW_COLUMN (tmp_list->data))
	  return (BtkTreeViewColumn *)0x1;

	while (tmp_list)
	  {
	    g_assert (tmp_list);

	    left_column = BTK_TREE_VIEW_COLUMN (tmp_list->data);

	    if (left_column && !left_column->visible)
	      {
		cur_column = left_column;
		tmp_list = tmp_list->prev;
	      }

	    if (!tree_view->priv->column_drop_func)
	      return left_column;

	    if (tree_view->priv->column_drop_func (tree_view, column, left_column, cur_column, tree_view->priv->column_drop_func_data))
	      return left_column;

	    cur_column = left_column;
	    tmp_list = tmp_list->prev;
	  }

	if (!tree_view->priv->column_drop_func)
	  return NULL;

	if (tree_view->priv->column_drop_func (tree_view, column, NULL, cur_column, tree_view->priv->column_drop_func_data))
	  return NULL;
	else
	  return (BtkTreeViewColumn *)0x1;
	break;
    }

  return (BtkTreeViewColumn *)0x1;
}

static gboolean
btk_tree_view_key_press (BtkWidget   *widget,
			 BdkEventKey *event)
{
  BtkTreeView *tree_view = (BtkTreeView *) widget;

  if (tree_view->priv->rubber_band_status)
    {
      if (event->keyval == BDK_Escape)
	btk_tree_view_stop_rubber_band (tree_view);

      return TRUE;
    }

  if (BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_IN_COLUMN_DRAG))
    {
      if (event->keyval == BDK_Escape)
	{
	  tree_view->priv->cur_reorder = NULL;
	  btk_tree_view_button_release_drag_column (widget, NULL);
	}
      return TRUE;
    }

  if (BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_HEADERS_VISIBLE))
    {
      GList *focus_column;
      gboolean rtl;

      rtl = (btk_widget_get_direction (BTK_WIDGET (tree_view)) == BTK_TEXT_DIR_RTL);

      for (focus_column = tree_view->priv->columns;
           focus_column;
           focus_column = focus_column->next)
        {
          BtkTreeViewColumn *column = BTK_TREE_VIEW_COLUMN (focus_column->data);

          if (btk_widget_has_focus (column->button))
            break;
        }

      if (focus_column &&
          (event->state & BDK_SHIFT_MASK) && (event->state & BDK_MOD1_MASK) &&
          (event->keyval == BDK_Left || event->keyval == BDK_KP_Left
           || event->keyval == BDK_Right || event->keyval == BDK_KP_Right))
        {
          BtkTreeViewColumn *column = BTK_TREE_VIEW_COLUMN (focus_column->data);

          if (!column->resizable)
            {
              btk_widget_error_bell (widget);
              return TRUE;
            }

          if (event->keyval == (rtl ? BDK_Right : BDK_Left)
              || event->keyval == (rtl ? BDK_KP_Right : BDK_KP_Left))
            {
              gint old_width = column->resized_width;

              column->resized_width = MAX (column->resized_width,
                                           column->width);
              column->resized_width -= 2;
              if (column->resized_width < 0)
                column->resized_width = 0;

              if (column->min_width == -1)
                column->resized_width = MAX (column->button->requisition.width,
                                             column->resized_width);
              else
                column->resized_width = MAX (column->min_width,
                                             column->resized_width);

              if (column->max_width != -1)
                column->resized_width = MIN (column->resized_width,
                                             column->max_width);

              column->use_resized_width = TRUE;

              if (column->resized_width != old_width)
                btk_widget_queue_resize (widget);
              else
                btk_widget_error_bell (widget);
            }
          else if (event->keyval == (rtl ? BDK_Left : BDK_Right)
                   || event->keyval == (rtl ? BDK_KP_Left : BDK_KP_Right))
            {
              gint old_width = column->resized_width;

              column->resized_width = MAX (column->resized_width,
                                           column->width);
              column->resized_width += 2;

              if (column->max_width != -1)
                column->resized_width = MIN (column->resized_width,
                                             column->max_width);

              column->use_resized_width = TRUE;

              if (column->resized_width != old_width)
                btk_widget_queue_resize (widget);
              else
                btk_widget_error_bell (widget);
            }

          return TRUE;
        }

      if (focus_column &&
          (event->state & BDK_MOD1_MASK) &&
          (event->keyval == BDK_Left || event->keyval == BDK_KP_Left
           || event->keyval == BDK_Right || event->keyval == BDK_KP_Right
           || event->keyval == BDK_Home || event->keyval == BDK_KP_Home
           || event->keyval == BDK_End || event->keyval == BDK_KP_End))
        {
          BtkTreeViewColumn *column = BTK_TREE_VIEW_COLUMN (focus_column->data);

          if (event->keyval == (rtl ? BDK_Right : BDK_Left)
              || event->keyval == (rtl ? BDK_KP_Right : BDK_KP_Left))
            {
              BtkTreeViewColumn *col;
              col = btk_tree_view_get_drop_column (tree_view, column, DROP_LEFT);
              if (col != (BtkTreeViewColumn *)0x1)
                btk_tree_view_move_column_after (tree_view, column, col);
              else
                btk_widget_error_bell (widget);
            }
          else if (event->keyval == (rtl ? BDK_Left : BDK_Right)
                   || event->keyval == (rtl ? BDK_KP_Left : BDK_KP_Right))
            {
              BtkTreeViewColumn *col;
              col = btk_tree_view_get_drop_column (tree_view, column, DROP_RIGHT);
              if (col != (BtkTreeViewColumn *)0x1)
                btk_tree_view_move_column_after (tree_view, column, col);
              else
                btk_widget_error_bell (widget);
            }
          else if (event->keyval == BDK_Home || event->keyval == BDK_KP_Home)
            {
              BtkTreeViewColumn *col;
              col = btk_tree_view_get_drop_column (tree_view, column, DROP_HOME);
              if (col != (BtkTreeViewColumn *)0x1)
                btk_tree_view_move_column_after (tree_view, column, col);
              else
                btk_widget_error_bell (widget);
            }
          else if (event->keyval == BDK_End || event->keyval == BDK_KP_End)
            {
              BtkTreeViewColumn *col;
              col = btk_tree_view_get_drop_column (tree_view, column, DROP_END);
              if (col != (BtkTreeViewColumn *)0x1)
                btk_tree_view_move_column_after (tree_view, column, col);
              else
                btk_widget_error_bell (widget);
            }

          return TRUE;
        }
    }

  /* Chain up to the parent class.  It handles the keybindings. */
  if (BTK_WIDGET_CLASS (btk_tree_view_parent_class)->key_press_event (widget, event))
    return TRUE;

  if (tree_view->priv->search_entry_avoid_unhandled_binding)
    {
      tree_view->priv->search_entry_avoid_unhandled_binding = FALSE;
      return FALSE;
    }

  /* We pass the event to the search_entry.  If its text changes, then we start
   * the typeahead find capabilities. */
  if (btk_widget_has_focus (BTK_WIDGET (tree_view))
      && tree_view->priv->enable_search
      && !tree_view->priv->search_custom_entry_set)
    {
      BdkEvent *new_event;
      char *old_text;
      const char *new_text;
      gboolean retval;
      BdkScreen *screen;
      gboolean text_modified;
      gulong popup_menu_id;

      btk_tree_view_ensure_interactive_directory (tree_view);

      /* Make a copy of the current text */
      old_text = g_strdup (btk_entry_get_text (BTK_ENTRY (tree_view->priv->search_entry)));
      new_event = bdk_event_copy ((BdkEvent *) event);
      g_object_unref (((BdkEventKey *) new_event)->window);
      ((BdkEventKey *) new_event)->window = g_object_ref (tree_view->priv->search_window->window);
      btk_widget_realize (tree_view->priv->search_window);

      popup_menu_id = g_signal_connect (tree_view->priv->search_entry, 
					"popup-menu", G_CALLBACK (btk_true),
                                        NULL);

      /* Move the entry off screen */
      screen = btk_widget_get_screen (BTK_WIDGET (tree_view));
      btk_window_move (BTK_WINDOW (tree_view->priv->search_window),
		       bdk_screen_get_width (screen) + 1,
		       bdk_screen_get_height (screen) + 1);
      btk_widget_show (tree_view->priv->search_window);

      /* Send the event to the window.  If the preedit_changed signal is emitted
       * during this event, we will set priv->imcontext_changed  */
      tree_view->priv->imcontext_changed = FALSE;
      retval = btk_widget_event (tree_view->priv->search_window, new_event);
      bdk_event_free (new_event);
      btk_widget_hide (tree_view->priv->search_window);

      g_signal_handler_disconnect (tree_view->priv->search_entry, 
				   popup_menu_id);

      /* We check to make sure that the entry tried to handle the text, and that
       * the text has changed.
       */
      new_text = btk_entry_get_text (BTK_ENTRY (tree_view->priv->search_entry));
      text_modified = strcmp (old_text, new_text) != 0;
      g_free (old_text);
      if (tree_view->priv->imcontext_changed ||    /* we're in a preedit */
	  (retval && text_modified))               /* ...or the text was modified */
	{
	  if (btk_tree_view_real_start_interactive_search (tree_view, FALSE))
	    {
	      btk_widget_grab_focus (BTK_WIDGET (tree_view));
	      return TRUE;
	    }
	  else
	    {
	      btk_entry_set_text (BTK_ENTRY (tree_view->priv->search_entry), "");
	      return FALSE;
	    }
	}
    }

  return FALSE;
}

static gboolean
btk_tree_view_key_release (BtkWidget   *widget,
			   BdkEventKey *event)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (widget);

  if (tree_view->priv->rubber_band_status)
    return TRUE;

  return BTK_WIDGET_CLASS (btk_tree_view_parent_class)->key_release_event (widget, event);
}

/* FIXME Is this function necessary? Can I get an enter_notify event
 * w/o either an expose event or a mouse motion event?
 */
static gboolean
btk_tree_view_enter_notify (BtkWidget        *widget,
			    BdkEventCrossing *event)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (widget);
  BtkRBTree *tree;
  BtkRBNode *node;
  gint new_y;

  /* Sanity check it */
  if (event->window != tree_view->priv->bin_window)
    return FALSE;

  if (tree_view->priv->tree == NULL)
    return FALSE;

  if (event->mode == BDK_CROSSING_GRAB ||
      event->mode == BDK_CROSSING_BTK_GRAB ||
      event->mode == BDK_CROSSING_BTK_UNGRAB ||
      event->mode == BDK_CROSSING_STATE_CHANGED)
    return TRUE;

  /* find the node internally */
  new_y = TREE_WINDOW_Y_TO_RBTREE_Y(tree_view, event->y);
  if (new_y < 0)
    new_y = 0;
  _btk_rbtree_find_offset (tree_view->priv->tree, new_y, &tree, &node);

  tree_view->priv->event_last_x = event->x;
  tree_view->priv->event_last_y = event->y;

  if ((tree_view->priv->button_pressed_node == NULL) ||
      (tree_view->priv->button_pressed_node == node))
    prelight_or_select (tree_view, tree, node, event->x, event->y);

  return TRUE;
}

static gboolean
btk_tree_view_leave_notify (BtkWidget        *widget,
			    BdkEventCrossing *event)
{
  BtkTreeView *tree_view;

  if (event->mode == BDK_CROSSING_GRAB)
    return TRUE;

  tree_view = BTK_TREE_VIEW (widget);

  if (tree_view->priv->prelight_node)
    _btk_tree_view_queue_draw_node (tree_view,
                                   tree_view->priv->prelight_tree,
                                   tree_view->priv->prelight_node,
                                   NULL);

  tree_view->priv->event_last_x = -10000;
  tree_view->priv->event_last_y = -10000;

  prelight_or_select (tree_view,
		      NULL, NULL,
		      -1000, -1000); /* coords not possibly over an arrow */

  return TRUE;
}


static gint
btk_tree_view_focus_out (BtkWidget     *widget,
			 BdkEventFocus *event)
{
  BtkTreeView *tree_view;

  tree_view = BTK_TREE_VIEW (widget);

  btk_widget_queue_draw (widget);

  /* destroy interactive search dialog */
  if (tree_view->priv->search_window)
    btk_tree_view_search_dialog_hide (tree_view->priv->search_window, tree_view);

  return FALSE;
}


/* Incremental Reflow
 */

static void
btk_tree_view_node_queue_redraw (BtkTreeView *tree_view,
				 BtkRBTree   *tree,
				 BtkRBNode   *node)
{
  gint y;

  y = _btk_rbtree_node_find_offset (tree, node)
    - tree_view->priv->vadjustment->value
    + TREE_VIEW_HEADER_HEIGHT (tree_view);

  btk_widget_queue_draw_area (BTK_WIDGET (tree_view),
			      0, y,
			      BTK_WIDGET (tree_view)->allocation.width,
			      BTK_RBNODE_GET_HEIGHT (node));
}

static gboolean
node_is_visible (BtkTreeView *tree_view,
		 BtkRBTree   *tree,
		 BtkRBNode   *node)
{
  int y;
  int height;

  y = _btk_rbtree_node_find_offset (tree, node);
  height = ROW_HEIGHT (tree_view, BTK_RBNODE_GET_HEIGHT (node));

  if (y >= tree_view->priv->vadjustment->value &&
      y + height <= (tree_view->priv->vadjustment->value
	             + tree_view->priv->vadjustment->page_size))
    return TRUE;

  return FALSE;
}

/* Returns TRUE if it updated the size
 */
static gboolean
validate_row (BtkTreeView *tree_view,
	      BtkRBTree   *tree,
	      BtkRBNode   *node,
	      BtkTreeIter *iter,
	      BtkTreePath *path)
{
  BtkTreeViewColumn *column;
  GList *list, *first_column, *last_column;
  gint height = 0;
  gint horizontal_separator;
  gint vertical_separator;
  gint focus_line_width;
  gint depth = btk_tree_path_get_depth (path);
  gboolean retval = FALSE;
  gboolean is_separator = FALSE;
  gboolean draw_vgrid_lines, draw_hgrid_lines;
  gint focus_pad;
  gint grid_line_width;
  gboolean wide_separators;
  gint separator_height;

  /* double check the row needs validating */
  if (! BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_INVALID) &&
      ! BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_COLUMN_INVALID))
    return FALSE;

  is_separator = row_is_separator (tree_view, iter, NULL);

  btk_widget_style_get (BTK_WIDGET (tree_view),
			"focus-padding", &focus_pad,
			"focus-line-width", &focus_line_width,
			"horizontal-separator", &horizontal_separator,
			"vertical-separator", &vertical_separator,
			"grid-line-width", &grid_line_width,
                        "wide-separators",  &wide_separators,
                        "separator-height", &separator_height,
			NULL);
  
  draw_vgrid_lines =
    tree_view->priv->grid_lines == BTK_TREE_VIEW_GRID_LINES_VERTICAL
    || tree_view->priv->grid_lines == BTK_TREE_VIEW_GRID_LINES_BOTH;
  draw_hgrid_lines =
    tree_view->priv->grid_lines == BTK_TREE_VIEW_GRID_LINES_HORIZONTAL
    || tree_view->priv->grid_lines == BTK_TREE_VIEW_GRID_LINES_BOTH;

  for (last_column = g_list_last (tree_view->priv->columns);
       last_column && !(BTK_TREE_VIEW_COLUMN (last_column->data)->visible);
       last_column = last_column->prev)
    ;

  for (first_column = g_list_first (tree_view->priv->columns);
       first_column && !(BTK_TREE_VIEW_COLUMN (first_column->data)->visible);
       first_column = first_column->next)
    ;

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      gint tmp_width;
      gint tmp_height;

      column = list->data;

      if (! column->visible)
	continue;

      if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_COLUMN_INVALID) && !column->dirty)
	continue;

      btk_tree_view_column_cell_set_cell_data (column, tree_view->priv->model, iter,
					       BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_PARENT),
					       node->children?TRUE:FALSE);
      btk_tree_view_column_cell_get_size (column,
					  NULL, NULL, NULL,
					  &tmp_width, &tmp_height);

      if (!is_separator)
	{
          tmp_height += vertical_separator;
	  height = MAX (height, tmp_height);
	  height = MAX (height, tree_view->priv->expander_size);
	}
      else
        {
          if (wide_separators)
            height = separator_height + 2 * focus_pad;
          else
            height = 2 + 2 * focus_pad;
        }

      if (btk_tree_view_is_expander_column (tree_view, column))
        {
	  tmp_width = tmp_width + horizontal_separator + (depth - 1) * tree_view->priv->level_indentation;

	  if (TREE_VIEW_DRAW_EXPANDERS (tree_view))
	    tmp_width += depth * tree_view->priv->expander_size;
	}
      else
	tmp_width = tmp_width + horizontal_separator;

      if (draw_vgrid_lines)
        {
	  if (list->data == first_column || list->data == last_column)
	    tmp_width += grid_line_width / 2.0;
	  else
	    tmp_width += grid_line_width;
	}

      if (tmp_width > column->requested_width)
	{
	  retval = TRUE;
	  column->requested_width = tmp_width;
	}
    }

  if (draw_hgrid_lines)
    height += grid_line_width;

  if (height != BTK_RBNODE_GET_HEIGHT (node))
    {
      retval = TRUE;
      _btk_rbtree_node_set_height (tree, node, height);
    }
  _btk_rbtree_node_mark_valid (tree, node);
  tree_view->priv->post_validation_flag = TRUE;

  return retval;
}


static void
validate_visible_area (BtkTreeView *tree_view)
{
  BtkTreePath *path = NULL;
  BtkTreePath *above_path = NULL;
  BtkTreeIter iter;
  BtkRBTree *tree = NULL;
  BtkRBNode *node = NULL;
  gboolean need_redraw = FALSE;
  gboolean size_changed = FALSE;
  gint total_height;
  gint area_above = 0;
  gint area_below = 0;

  if (tree_view->priv->tree == NULL)
    return;

  if (! BTK_RBNODE_FLAG_SET (tree_view->priv->tree->root, BTK_RBNODE_DESCENDANTS_INVALID) &&
      tree_view->priv->scroll_to_path == NULL)
    return;

  total_height = BTK_WIDGET (tree_view)->allocation.height - TREE_VIEW_HEADER_HEIGHT (tree_view);

  if (total_height == 0)
    return;

  /* First, we check to see if we need to scroll anywhere
   */
  if (tree_view->priv->scroll_to_path)
    {
      path = btk_tree_row_reference_get_path (tree_view->priv->scroll_to_path);
      if (path && !_btk_tree_view_find_node (tree_view, path, &tree, &node))
	{
          /* we are going to scroll, and will update dy */
	  btk_tree_model_get_iter (tree_view->priv->model, &iter, path);
	  if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_INVALID) ||
	      BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_COLUMN_INVALID))
	    {
	      _btk_tree_view_queue_draw_node (tree_view, tree, node, NULL);
	      if (validate_row (tree_view, tree, node, &iter, path))
		size_changed = TRUE;
	    }

	  if (tree_view->priv->scroll_to_use_align)
	    {
	      gint height = ROW_HEIGHT (tree_view, BTK_RBNODE_GET_HEIGHT (node));
	      area_above = (total_height - height) *
		tree_view->priv->scroll_to_row_align;
	      area_below = total_height - area_above - height;
	      area_above = MAX (area_above, 0);
	      area_below = MAX (area_below, 0);
	    }
	  else
	    {
	      /* two cases:
	       * 1) row not visible
	       * 2) row visible
	       */
	      gint dy;
	      gint height = ROW_HEIGHT (tree_view, BTK_RBNODE_GET_HEIGHT (node));

	      dy = _btk_rbtree_node_find_offset (tree, node);

	      if (dy >= tree_view->priv->vadjustment->value &&
		  dy + height <= (tree_view->priv->vadjustment->value
		                  + tree_view->priv->vadjustment->page_size))
	        {
		  /* row visible: keep the row at the same position */
		  area_above = dy - tree_view->priv->vadjustment->value;
		  area_below = (tree_view->priv->vadjustment->value +
		                tree_view->priv->vadjustment->page_size)
		               - dy - height;
		}
	      else
	        {
		  /* row not visible */
		  if (dy >= 0
		      && dy + height <= tree_view->priv->vadjustment->page_size)
		    {
		      /* row at the beginning -- fixed */
		      area_above = dy;
		      area_below = tree_view->priv->vadjustment->page_size
				   - area_above - height;
		    }
		  else if (dy >= (tree_view->priv->vadjustment->upper -
			          tree_view->priv->vadjustment->page_size))
		    {
		      /* row at the end -- fixed */
		      area_above = dy - (tree_view->priv->vadjustment->upper -
			           tree_view->priv->vadjustment->page_size);
                      area_below = tree_view->priv->vadjustment->page_size -
                                   area_above - height;

                      if (area_below < 0)
                        {
			  area_above = tree_view->priv->vadjustment->page_size - height;
                          area_below = 0;
                        }
		    }
		  else
		    {
		      /* row somewhere in the middle, bring it to the top
		       * of the view
		       */
		      area_above = 0;
		      area_below = total_height - height;
		    }
		}
	    }
	}
      else
	/* the scroll to isn't valid; ignore it.
	 */
	{
	  if (tree_view->priv->scroll_to_path && !path)
	    {
	      btk_tree_row_reference_free (tree_view->priv->scroll_to_path);
	      tree_view->priv->scroll_to_path = NULL;
	    }
	  if (path)
	    btk_tree_path_free (path);
	  path = NULL;
	}      
    }

  /* We didn't have a scroll_to set, so we just handle things normally
   */
  if (path == NULL)
    {
      gint offset;

      offset = _btk_rbtree_find_offset (tree_view->priv->tree,
					TREE_WINDOW_Y_TO_RBTREE_Y (tree_view, 0),
					&tree, &node);
      if (node == NULL)
	{
	  /* In this case, nothing has been validated */
	  path = btk_tree_path_new_first ();
	  _btk_tree_view_find_node (tree_view, path, &tree, &node);
	}
      else
	{
	  path = _btk_tree_view_find_path (tree_view, tree, node);
	  total_height += offset;
	}

      btk_tree_model_get_iter (tree_view->priv->model, &iter, path);

      if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_INVALID) ||
	  BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_COLUMN_INVALID))
	{
	  _btk_tree_view_queue_draw_node (tree_view, tree, node, NULL);
	  if (validate_row (tree_view, tree, node, &iter, path))
	    size_changed = TRUE;
	}
      area_above = 0;
      area_below = total_height - ROW_HEIGHT (tree_view, BTK_RBNODE_GET_HEIGHT (node));
    }

  above_path = btk_tree_path_copy (path);

  /* if we do not validate any row above the new top_row, we will make sure
   * that the row immediately above top_row has been validated. (if we do not
   * do this, _btk_rbtree_find_offset will find the row above top_row, because
   * when invalidated that row's height will be zero. and this will mess up
   * scrolling).
   */
  if (area_above == 0)
    {
      BtkRBTree *tmptree;
      BtkRBNode *tmpnode;

      _btk_tree_view_find_node (tree_view, above_path, &tmptree, &tmpnode);
      _btk_rbtree_prev_full (tmptree, tmpnode, &tmptree, &tmpnode);

      if (tmpnode)
        {
	  BtkTreePath *tmppath;
	  BtkTreeIter tmpiter;

	  tmppath = _btk_tree_view_find_path (tree_view, tmptree, tmpnode);
	  btk_tree_model_get_iter (tree_view->priv->model, &tmpiter, tmppath);

	  if (BTK_RBNODE_FLAG_SET (tmpnode, BTK_RBNODE_INVALID) ||
	      BTK_RBNODE_FLAG_SET (tmpnode, BTK_RBNODE_COLUMN_INVALID))
	    {
	      _btk_tree_view_queue_draw_node (tree_view, tmptree, tmpnode, NULL);
	      if (validate_row (tree_view, tmptree, tmpnode, &tmpiter, tmppath))
		size_changed = TRUE;
	    }

	  btk_tree_path_free (tmppath);
	}
    }

  /* Now, we walk forwards and backwards, measuring rows. Unfortunately,
   * backwards is much slower then forward, as there is no iter_prev function.
   * We go forwards first in case we run out of tree.  Then we go backwards to
   * fill out the top.
   */
  while (node && area_below > 0)
    {
      if (node->children)
	{
	  BtkTreeIter parent = iter;
	  gboolean has_child;

	  tree = node->children;
	  node = tree->root;

          g_assert (node != tree->nil);

	  while (node->left != tree->nil)
	    node = node->left;
	  has_child = btk_tree_model_iter_children (tree_view->priv->model,
						    &iter,
						    &parent);
	  TREE_VIEW_INTERNAL_ASSERT_VOID (has_child);
	  btk_tree_path_down (path);
	}
      else
	{
	  gboolean done = FALSE;
	  do
	    {
	      node = _btk_rbtree_next (tree, node);
	      if (node != NULL)
		{
		  gboolean has_next = btk_tree_model_iter_next (tree_view->priv->model, &iter);
		  done = TRUE;
		  btk_tree_path_next (path);

		  /* Sanity Check! */
		  TREE_VIEW_INTERNAL_ASSERT_VOID (has_next);
		}
	      else
		{
		  BtkTreeIter parent_iter = iter;
		  gboolean has_parent;

		  node = tree->parent_node;
		  tree = tree->parent_tree;
		  if (tree == NULL)
		    break;
		  has_parent = btk_tree_model_iter_parent (tree_view->priv->model,
							   &iter,
							   &parent_iter);
		  btk_tree_path_up (path);

		  /* Sanity check */
		  TREE_VIEW_INTERNAL_ASSERT_VOID (has_parent);
		}
	    }
	  while (!done);
	}

      if (!node)
        break;

      if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_INVALID) ||
	  BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_COLUMN_INVALID))
	{
	  _btk_tree_view_queue_draw_node (tree_view, tree, node, NULL);
	  if (validate_row (tree_view, tree, node, &iter, path))
	      size_changed = TRUE;
	}

      area_below -= ROW_HEIGHT (tree_view, BTK_RBNODE_GET_HEIGHT (node));
    }
  btk_tree_path_free (path);

  /* If we ran out of tree, and have extra area_below left, we need to add it
   * to area_above */
  if (area_below > 0)
    area_above += area_below;

  _btk_tree_view_find_node (tree_view, above_path, &tree, &node);

  /* We walk backwards */
  while (area_above > 0)
    {
      _btk_rbtree_prev_full (tree, node, &tree, &node);

      /* Always find the new path in the tree.  We cannot just assume
       * a btk_tree_path_prev() is enough here, as there might be children
       * in between this node and the previous sibling node.  If this
       * appears to be a performance hotspot in profiles, we can look into
       * intrigate logic for keeping path, node and iter in sync like
       * we do for forward walks.  (Which will be hard because of the lacking
       * iter_prev).
       */

      if (node == NULL)
	break;

      btk_tree_path_free (above_path);
      above_path = _btk_tree_view_find_path (tree_view, tree, node);

      btk_tree_model_get_iter (tree_view->priv->model, &iter, above_path);

      if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_INVALID) ||
	  BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_COLUMN_INVALID))
	{
	  _btk_tree_view_queue_draw_node (tree_view, tree, node, NULL);
	  if (validate_row (tree_view, tree, node, &iter, above_path))
	    size_changed = TRUE;
	}
      area_above -= ROW_HEIGHT (tree_view, BTK_RBNODE_GET_HEIGHT (node));
    }

  /* if we scrolled to a path, we need to set the dy here,
   * and sync the top row accordingly
   */
  if (tree_view->priv->scroll_to_path)
    {
      btk_tree_view_set_top_row (tree_view, above_path, -area_above);
      btk_tree_view_top_row_to_dy (tree_view);

      need_redraw = TRUE;
    }
  else if (tree_view->priv->height <= tree_view->priv->vadjustment->page_size)
    {
      /* when we are not scrolling, we should never set dy to something
       * else than zero. we update top_row to be in sync with dy = 0.
       */
      btk_adjustment_set_value (BTK_ADJUSTMENT (tree_view->priv->vadjustment), 0);
      btk_tree_view_dy_to_top_row (tree_view);
    }
  else if (tree_view->priv->vadjustment->value + tree_view->priv->vadjustment->page_size > tree_view->priv->height)
    {
      btk_adjustment_set_value (BTK_ADJUSTMENT (tree_view->priv->vadjustment), tree_view->priv->height - tree_view->priv->vadjustment->page_size);
      btk_tree_view_dy_to_top_row (tree_view);
    }
  else
    btk_tree_view_top_row_to_dy (tree_view);

  /* update width/height and queue a resize */
  if (size_changed)
    {
      BtkRequisition requisition;

      /* We temporarily guess a size, under the assumption that it will be the
       * same when we get our next size_allocate.  If we don't do this, we'll be
       * in an inconsistent state if we call top_row_to_dy. */

      btk_widget_size_request (BTK_WIDGET (tree_view), &requisition);
      tree_view->priv->hadjustment->upper = MAX (tree_view->priv->hadjustment->upper, (gfloat)requisition.width);
      tree_view->priv->vadjustment->upper = MAX (tree_view->priv->vadjustment->upper, (gfloat)requisition.height);
      btk_adjustment_changed (tree_view->priv->hadjustment);
      btk_adjustment_changed (tree_view->priv->vadjustment);
      btk_widget_queue_resize (BTK_WIDGET (tree_view));
    }

  if (tree_view->priv->scroll_to_path)
    {
      btk_tree_row_reference_free (tree_view->priv->scroll_to_path);
      tree_view->priv->scroll_to_path = NULL;
    }

  if (above_path)
    btk_tree_path_free (above_path);

  if (tree_view->priv->scroll_to_column)
    {
      tree_view->priv->scroll_to_column = NULL;
    }
  if (need_redraw)
    btk_widget_queue_draw (BTK_WIDGET (tree_view));
}

static void
initialize_fixed_height_mode (BtkTreeView *tree_view)
{
  if (!tree_view->priv->tree)
    return;

  if (tree_view->priv->fixed_height < 0)
    {
      BtkTreeIter iter;
      BtkTreePath *path;

      BtkRBTree *tree = NULL;
      BtkRBNode *node = NULL;

      tree = tree_view->priv->tree;
      node = tree->root;

      path = _btk_tree_view_find_path (tree_view, tree, node);
      btk_tree_model_get_iter (tree_view->priv->model, &iter, path);

      validate_row (tree_view, tree, node, &iter, path);

      btk_tree_path_free (path);

      tree_view->priv->fixed_height = ROW_HEIGHT (tree_view, BTK_RBNODE_GET_HEIGHT (node));
    }

   _btk_rbtree_set_fixed_height (tree_view->priv->tree,
                                 tree_view->priv->fixed_height, TRUE);
}

/* Our strategy for finding nodes to validate is a little convoluted.  We find
 * the left-most uninvalidated node.  We then try walking right, validating
 * nodes.  Once we find a valid node, we repeat the previous process of finding
 * the first invalid node.
 */

static gboolean
do_validate_rows (BtkTreeView *tree_view, gboolean queue_resize)
{
  BtkRBTree *tree = NULL;
  BtkRBNode *node = NULL;
  gboolean validated_area = FALSE;
  gint retval = TRUE;
  BtkTreePath *path = NULL;
  BtkTreeIter iter;
  GTimer *timer;
  gint i = 0;

  gint prev_height = -1;
  gboolean fixed_height = TRUE;

  g_assert (tree_view);

  if (tree_view->priv->tree == NULL)
      return FALSE;

  if (tree_view->priv->fixed_height_mode)
    {
      if (tree_view->priv->fixed_height < 0)
        initialize_fixed_height_mode (tree_view);

      return FALSE;
    }

  timer = g_timer_new ();
  g_timer_start (timer);

  do
    {
      if (! BTK_RBNODE_FLAG_SET (tree_view->priv->tree->root, BTK_RBNODE_DESCENDANTS_INVALID))
	{
	  retval = FALSE;
	  goto done;
	}

      if (path != NULL)
	{
	  node = _btk_rbtree_next (tree, node);
	  if (node != NULL)
	    {
	      TREE_VIEW_INTERNAL_ASSERT (btk_tree_model_iter_next (tree_view->priv->model, &iter), FALSE);
	      btk_tree_path_next (path);
	    }
	  else
	    {
	      btk_tree_path_free (path);
	      path = NULL;
	    }
	}

      if (path == NULL)
	{
	  tree = tree_view->priv->tree;
	  node = tree_view->priv->tree->root;

	  g_assert (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_DESCENDANTS_INVALID));

	  do
	    {
	      if (node->left != tree->nil &&
		  BTK_RBNODE_FLAG_SET (node->left, BTK_RBNODE_DESCENDANTS_INVALID))
		{
		  node = node->left;
		}
	      else if (node->right != tree->nil &&
		       BTK_RBNODE_FLAG_SET (node->right, BTK_RBNODE_DESCENDANTS_INVALID))
		{
		  node = node->right;
		}
	      else if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_INVALID) ||
		       BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_COLUMN_INVALID))
		{
		  break;
		}
	      else if (node->children != NULL)
		{
		  tree = node->children;
		  node = tree->root;
		}
	      else
		/* RBTree corruption!  All bad */
		g_assert_not_reached ();
	    }
	  while (TRUE);
	  path = _btk_tree_view_find_path (tree_view, tree, node);
	  btk_tree_model_get_iter (tree_view->priv->model, &iter, path);
	}

      validated_area = validate_row (tree_view, tree, node, &iter, path) ||
                       validated_area;

      if (!tree_view->priv->fixed_height_check)
        {
	  gint height;

	  height = ROW_HEIGHT (tree_view, BTK_RBNODE_GET_HEIGHT (node));
	  if (prev_height < 0)
	    prev_height = height;
	  else if (prev_height != height)
	    fixed_height = FALSE;
	}

      i++;
    }
  while (g_timer_elapsed (timer, NULL) < BTK_TREE_VIEW_TIME_MS_PER_IDLE / 1000.);

  if (!tree_view->priv->fixed_height_check)
   {
     if (fixed_height)
       _btk_rbtree_set_fixed_height (tree_view->priv->tree, prev_height, FALSE);

     tree_view->priv->fixed_height_check = 1;
   }
  
 done:
  if (validated_area)
    {
      BtkRequisition requisition;
      /* We temporarily guess a size, under the assumption that it will be the
       * same when we get our next size_allocate.  If we don't do this, we'll be
       * in an inconsistent state when we call top_row_to_dy. */

      btk_widget_size_request (BTK_WIDGET (tree_view), &requisition);
      tree_view->priv->hadjustment->upper = MAX (tree_view->priv->hadjustment->upper, (gfloat)requisition.width);
      tree_view->priv->vadjustment->upper = MAX (tree_view->priv->vadjustment->upper, (gfloat)requisition.height);
      btk_adjustment_changed (tree_view->priv->hadjustment);
      btk_adjustment_changed (tree_view->priv->vadjustment);

      if (queue_resize)
        btk_widget_queue_resize (BTK_WIDGET (tree_view));
    }

  if (path) btk_tree_path_free (path);
  g_timer_destroy (timer);

  return retval;
}

static gboolean
validate_rows (BtkTreeView *tree_view)
{
  gboolean retval;
  
  retval = do_validate_rows (tree_view, TRUE);
  
  if (! retval && tree_view->priv->validate_rows_timer)
    {
      g_source_remove (tree_view->priv->validate_rows_timer);
      tree_view->priv->validate_rows_timer = 0;
    }

  return retval;
}

static gboolean
validate_rows_handler (BtkTreeView *tree_view)
{
  gboolean retval;

  retval = do_validate_rows (tree_view, TRUE);
  if (! retval && tree_view->priv->validate_rows_timer)
    {
      g_source_remove (tree_view->priv->validate_rows_timer);
      tree_view->priv->validate_rows_timer = 0;
    }

  return retval;
}

static gboolean
do_presize_handler (BtkTreeView *tree_view)
{
  if (tree_view->priv->mark_rows_col_dirty)
    {
      if (tree_view->priv->tree)
	_btk_rbtree_column_invalid (tree_view->priv->tree);
      tree_view->priv->mark_rows_col_dirty = FALSE;
    }
  validate_visible_area (tree_view);
  tree_view->priv->presize_handler_timer = 0;

  if (tree_view->priv->fixed_height_mode)
    {
      BtkRequisition requisition;

      btk_widget_size_request (BTK_WIDGET (tree_view), &requisition);

      tree_view->priv->hadjustment->upper = MAX (tree_view->priv->hadjustment->upper, (gfloat)requisition.width);
      tree_view->priv->vadjustment->upper = MAX (tree_view->priv->vadjustment->upper, (gfloat)requisition.height);
      btk_adjustment_changed (tree_view->priv->hadjustment);
      btk_adjustment_changed (tree_view->priv->vadjustment);
      btk_widget_queue_resize (BTK_WIDGET (tree_view));
    }
		   
  return FALSE;
}

static gboolean
presize_handler_callback (gpointer data)
{
  do_presize_handler (BTK_TREE_VIEW (data));
		   
  return FALSE;
}

static void
install_presize_handler (BtkTreeView *tree_view)
{
  if (! btk_widget_get_realized (BTK_WIDGET (tree_view)))
    return;

  if (! tree_view->priv->presize_handler_timer)
    {
      tree_view->priv->presize_handler_timer =
	bdk_threads_add_idle_full (BTK_PRIORITY_RESIZE - 2, presize_handler_callback, tree_view, NULL);
    }
  if (! tree_view->priv->validate_rows_timer)
    {
      tree_view->priv->validate_rows_timer =
	bdk_threads_add_idle_full (BTK_TREE_VIEW_PRIORITY_VALIDATE, (GSourceFunc) validate_rows_handler, tree_view, NULL);
    }
}

static gboolean
scroll_sync_handler (BtkTreeView *tree_view)
{
  if (tree_view->priv->height <= tree_view->priv->vadjustment->page_size)
    btk_adjustment_set_value (BTK_ADJUSTMENT (tree_view->priv->vadjustment), 0);
  else if (btk_tree_row_reference_valid (tree_view->priv->top_row))
    btk_tree_view_top_row_to_dy (tree_view);
  else
    btk_tree_view_dy_to_top_row (tree_view);

  tree_view->priv->scroll_sync_timer = 0;

  return FALSE;
}

static void
install_scroll_sync_handler (BtkTreeView *tree_view)
{
  if (!btk_widget_get_realized (BTK_WIDGET (tree_view)))
    return;

  if (!tree_view->priv->scroll_sync_timer)
    {
      tree_view->priv->scroll_sync_timer =
	bdk_threads_add_idle_full (BTK_TREE_VIEW_PRIORITY_SCROLL_SYNC, (GSourceFunc) scroll_sync_handler, tree_view, NULL);
    }
}

static void
btk_tree_view_set_top_row (BtkTreeView *tree_view,
			   BtkTreePath *path,
			   gint         offset)
{
  btk_tree_row_reference_free (tree_view->priv->top_row);

  if (!path)
    {
      tree_view->priv->top_row = NULL;
      tree_view->priv->top_row_dy = 0;
    }
  else
    {
      tree_view->priv->top_row = btk_tree_row_reference_new_proxy (B_OBJECT (tree_view), tree_view->priv->model, path);
      tree_view->priv->top_row_dy = offset;
    }
}

/* Always call this iff dy is in the visible range.  If the tree is empty, then
 * it's set to be NULL, and top_row_dy is 0;
 */
static void
btk_tree_view_dy_to_top_row (BtkTreeView *tree_view)
{
  gint offset;
  BtkTreePath *path;
  BtkRBTree *tree;
  BtkRBNode *node;

  if (tree_view->priv->tree == NULL)
    {
      btk_tree_view_set_top_row (tree_view, NULL, 0);
    }
  else
    {
      offset = _btk_rbtree_find_offset (tree_view->priv->tree,
					tree_view->priv->dy,
					&tree, &node);

      if (tree == NULL)
        {
	  btk_tree_view_set_top_row (tree_view, NULL, 0);
	}
      else
        {
	  path = _btk_tree_view_find_path (tree_view, tree, node);
	  btk_tree_view_set_top_row (tree_view, path, offset);
	  btk_tree_path_free (path);
	}
    }
}

static void
btk_tree_view_top_row_to_dy (BtkTreeView *tree_view)
{
  BtkTreePath *path;
  BtkRBTree *tree;
  BtkRBNode *node;
  int new_dy;

  /* Avoid recursive calls */
  if (tree_view->priv->in_top_row_to_dy)
    return;

  if (tree_view->priv->top_row)
    path = btk_tree_row_reference_get_path (tree_view->priv->top_row);
  else
    path = NULL;

  if (!path)
    tree = NULL;
  else
    _btk_tree_view_find_node (tree_view, path, &tree, &node);

  if (path)
    btk_tree_path_free (path);

  if (tree == NULL)
    {
      /* keep dy and set new toprow */
      btk_tree_row_reference_free (tree_view->priv->top_row);
      tree_view->priv->top_row = NULL;
      tree_view->priv->top_row_dy = 0;
      /* DO NOT install the idle handler */
      btk_tree_view_dy_to_top_row (tree_view);
      return;
    }

  if (ROW_HEIGHT (tree_view, BACKGROUND_HEIGHT (node))
      < tree_view->priv->top_row_dy)
    {
      /* new top row -- do NOT install the idle handler */
      btk_tree_view_dy_to_top_row (tree_view);
      return;
    }

  new_dy = _btk_rbtree_node_find_offset (tree, node);
  new_dy += tree_view->priv->top_row_dy;

  if (new_dy + tree_view->priv->vadjustment->page_size > tree_view->priv->height)
    new_dy = tree_view->priv->height - tree_view->priv->vadjustment->page_size;

  new_dy = MAX (0, new_dy);

  tree_view->priv->in_top_row_to_dy = TRUE;
  btk_adjustment_set_value (tree_view->priv->vadjustment, (gdouble)new_dy);
  tree_view->priv->in_top_row_to_dy = FALSE;
}


void
_btk_tree_view_install_mark_rows_col_dirty (BtkTreeView *tree_view)
{
  tree_view->priv->mark_rows_col_dirty = TRUE;

  install_presize_handler (tree_view);
}

/*
 * This function works synchronously (due to the while (validate_rows...)
 * loop).
 *
 * There was a check for column_type != BTK_TREE_VIEW_COLUMN_AUTOSIZE
 * here. You now need to check that yourself.
 */
void
_btk_tree_view_column_autosize (BtkTreeView *tree_view,
			        BtkTreeViewColumn *column)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (column));

  _btk_tree_view_column_cell_set_dirty (column, FALSE);

  do_presize_handler (tree_view);
  while (validate_rows (tree_view));

  btk_widget_queue_resize (BTK_WIDGET (tree_view));
}

/* Drag-and-drop */

static void
set_source_row (BdkDragContext *context,
                BtkTreeModel   *model,
                BtkTreePath    *source_row)
{
  g_object_set_data_full (B_OBJECT (context),
                          I_("btk-tree-view-source-row"),
                          source_row ? btk_tree_row_reference_new (model, source_row) : NULL,
                          (GDestroyNotify) (source_row ? btk_tree_row_reference_free : NULL));
}

static BtkTreePath*
get_source_row (BdkDragContext *context)
{
  BtkTreeRowReference *ref =
    g_object_get_data (B_OBJECT (context), "btk-tree-view-source-row");

  if (ref)
    return btk_tree_row_reference_get_path (ref);
  else
    return NULL;
}

typedef struct
{
  BtkTreeRowReference *dest_row;
  guint                path_down_mode   : 1;
  guint                empty_view_drop  : 1;
  guint                drop_append_mode : 1;
}
DestRow;

static void
dest_row_free (gpointer data)
{
  DestRow *dr = (DestRow *)data;

  btk_tree_row_reference_free (dr->dest_row);
  g_slice_free (DestRow, dr);
}

static void
set_dest_row (BdkDragContext *context,
              BtkTreeModel   *model,
              BtkTreePath    *dest_row,
              gboolean        path_down_mode,
              gboolean        empty_view_drop,
              gboolean        drop_append_mode)
{
  DestRow *dr;

  if (!dest_row)
    {
      g_object_set_data_full (B_OBJECT (context), I_("btk-tree-view-dest-row"),
                              NULL, NULL);
      return;
    }

  dr = g_slice_new (DestRow);

  dr->dest_row = btk_tree_row_reference_new (model, dest_row);
  dr->path_down_mode = path_down_mode != FALSE;
  dr->empty_view_drop = empty_view_drop != FALSE;
  dr->drop_append_mode = drop_append_mode != FALSE;

  g_object_set_data_full (B_OBJECT (context), I_("btk-tree-view-dest-row"),
                          dr, (GDestroyNotify) dest_row_free);
}

static BtkTreePath*
get_dest_row (BdkDragContext *context,
              gboolean       *path_down_mode)
{
  DestRow *dr =
    g_object_get_data (B_OBJECT (context), "btk-tree-view-dest-row");

  if (dr)
    {
      BtkTreePath *path = NULL;

      if (path_down_mode)
        *path_down_mode = dr->path_down_mode;

      if (dr->dest_row)
        path = btk_tree_row_reference_get_path (dr->dest_row);
      else if (dr->empty_view_drop)
        path = btk_tree_path_new_from_indices (0, -1);
      else
        path = NULL;

      if (path && dr->drop_append_mode)
        btk_tree_path_next (path);

      return path;
    }
  else
    return NULL;
}

/* Get/set whether drag_motion requested the drag data and
 * drag_data_received should thus not actually insert the data,
 * since the data doesn't result from a drop.
 */
static void
set_status_pending (BdkDragContext *context,
                    BdkDragAction   suggested_action)
{
  g_object_set_data (B_OBJECT (context),
                     I_("btk-tree-view-status-pending"),
                     GINT_TO_POINTER (suggested_action));
}

static BdkDragAction
get_status_pending (BdkDragContext *context)
{
  return GPOINTER_TO_INT (g_object_get_data (B_OBJECT (context),
                                             "btk-tree-view-status-pending"));
}

static TreeViewDragInfo*
get_info (BtkTreeView *tree_view)
{
  return g_object_get_data (B_OBJECT (tree_view), "btk-tree-view-drag-info");
}

static void
destroy_info (TreeViewDragInfo *di)
{
  g_slice_free (TreeViewDragInfo, di);
}

static TreeViewDragInfo*
ensure_info (BtkTreeView *tree_view)
{
  TreeViewDragInfo *di;

  di = get_info (tree_view);

  if (di == NULL)
    {
      di = g_slice_new0 (TreeViewDragInfo);

      g_object_set_data_full (B_OBJECT (tree_view),
                              I_("btk-tree-view-drag-info"),
                              di,
                              (GDestroyNotify) destroy_info);
    }

  return di;
}

static void
remove_info (BtkTreeView *tree_view)
{
  g_object_set_data (B_OBJECT (tree_view), I_("btk-tree-view-drag-info"), NULL);
}

#if 0
static gint
drag_scan_timeout (gpointer data)
{
  BtkTreeView *tree_view;
  gint x, y;
  BdkModifierType state;
  BtkTreePath *path = NULL;
  BtkTreeViewColumn *column = NULL;
  BdkRectangle visible_rect;

  BDK_THREADS_ENTER ();

  tree_view = BTK_TREE_VIEW (data);

  bdk_window_get_pointer (tree_view->priv->bin_window,
                          &x, &y, &state);

  btk_tree_view_get_visible_rect (tree_view, &visible_rect);

  /* See if we are near the edge. */
  if ((x - visible_rect.x) < SCROLL_EDGE_SIZE ||
      (visible_rect.x + visible_rect.width - x) < SCROLL_EDGE_SIZE ||
      (y - visible_rect.y) < SCROLL_EDGE_SIZE ||
      (visible_rect.y + visible_rect.height - y) < SCROLL_EDGE_SIZE)
    {
      btk_tree_view_get_path_at_pos (tree_view,
                                     tree_view->priv->bin_window,
                                     x, y,
                                     &path,
                                     &column,
                                     NULL,
                                     NULL);

      if (path != NULL)
        {
          btk_tree_view_scroll_to_cell (tree_view,
                                        path,
                                        column,
					TRUE,
                                        0.5, 0.5);

          btk_tree_path_free (path);
        }
    }

  BDK_THREADS_LEAVE ();

  return TRUE;
}
#endif /* 0 */

static void
add_scroll_timeout (BtkTreeView *tree_view)
{
  if (tree_view->priv->scroll_timeout == 0)
    {
      tree_view->priv->scroll_timeout =
	bdk_threads_add_timeout (150, scroll_row_timeout, tree_view);
    }
}

static void
remove_scroll_timeout (BtkTreeView *tree_view)
{
  if (tree_view->priv->scroll_timeout != 0)
    {
      g_source_remove (tree_view->priv->scroll_timeout);
      tree_view->priv->scroll_timeout = 0;
    }
}

static gboolean
check_model_dnd (BtkTreeModel *model,
                 GType         required_iface,
                 const gchar  *signal)
{
  if (model == NULL || !B_TYPE_CHECK_INSTANCE_TYPE ((model), required_iface))
    {
      g_warning ("You must override the default '%s' handler "
                 "on BtkTreeView when using models that don't support "
                 "the %s interface and enabling drag-and-drop. The simplest way to do this "
                 "is to connect to '%s' and call "
                 "g_signal_stop_emission_by_name() in your signal handler to prevent "
                 "the default handler from running. Look at the source code "
                 "for the default handler in btktreeview.c to get an idea what "
                 "your handler should do. (btktreeview.c is in the BTK source "
                 "code.) If you're using BTK from a language other than C, "
                 "there may be a more natural way to override default handlers, e.g. via derivation.",
                 signal, g_type_name (required_iface), signal);
      return FALSE;
    }
  else
    return TRUE;
}

static void
remove_open_timeout (BtkTreeView *tree_view)
{
  if (tree_view->priv->open_dest_timeout != 0)
    {
      g_source_remove (tree_view->priv->open_dest_timeout);
      tree_view->priv->open_dest_timeout = 0;
    }
}


static gint
open_row_timeout (gpointer data)
{
  BtkTreeView *tree_view = data;
  BtkTreePath *dest_path = NULL;
  BtkTreeViewDropPosition pos;
  gboolean result = FALSE;

  btk_tree_view_get_drag_dest_row (tree_view,
                                   &dest_path,
                                   &pos);

  if (dest_path &&
      (pos == BTK_TREE_VIEW_DROP_INTO_OR_AFTER ||
       pos == BTK_TREE_VIEW_DROP_INTO_OR_BEFORE))
    {
      btk_tree_view_expand_row (tree_view, dest_path, FALSE);
      tree_view->priv->open_dest_timeout = 0;

      btk_tree_path_free (dest_path);
    }
  else
    {
      if (dest_path)
        btk_tree_path_free (dest_path);

      result = TRUE;
    }

  return result;
}

static gboolean
scroll_row_timeout (gpointer data)
{
  BtkTreeView *tree_view = data;

  btk_tree_view_vertical_autoscroll (tree_view);

  if (tree_view->priv->rubber_band_status == RUBBER_BAND_ACTIVE)
    btk_tree_view_update_rubber_band (tree_view);

  return TRUE;
}

/* Returns TRUE if event should not be propagated to parent widgets */
static gboolean
set_destination_row (BtkTreeView    *tree_view,
                     BdkDragContext *context,
                     /* coordinates relative to the widget */
                     gint            x,
                     gint            y,
                     BdkDragAction  *suggested_action,
                     BdkAtom        *target)
{
  BtkTreePath *path = NULL;
  BtkTreeViewDropPosition pos;
  BtkTreeViewDropPosition old_pos;
  TreeViewDragInfo *di;
  BtkWidget *widget;
  BtkTreePath *old_dest_path = NULL;
  gboolean can_drop = FALSE;

  *suggested_action = 0;
  *target = BDK_NONE;

  widget = BTK_WIDGET (tree_view);

  di = get_info (tree_view);

  if (di == NULL || y - TREE_VIEW_HEADER_HEIGHT (tree_view) < 0)
    {
      /* someone unset us as a drag dest, note that if
       * we return FALSE drag_leave isn't called
       */

      btk_tree_view_set_drag_dest_row (tree_view,
                                       NULL,
                                       BTK_TREE_VIEW_DROP_BEFORE);

      remove_scroll_timeout (BTK_TREE_VIEW (widget));
      remove_open_timeout (BTK_TREE_VIEW (widget));

      return FALSE; /* no longer a drop site */
    }

  *target = btk_drag_dest_find_target (widget, context,
                                       btk_drag_dest_get_target_list (widget));
  if (*target == BDK_NONE)
    {
      return FALSE;
    }

  if (!btk_tree_view_get_dest_row_at_pos (tree_view,
                                          x, y,
                                          &path,
                                          &pos))
    {
      gint n_children;
      BtkTreeModel *model;

      remove_open_timeout (tree_view);

      /* the row got dropped on empty space, let's setup a special case
       */

      if (path)
	btk_tree_path_free (path);

      model = btk_tree_view_get_model (tree_view);

      n_children = btk_tree_model_iter_n_children (model, NULL);
      if (n_children)
        {
          pos = BTK_TREE_VIEW_DROP_AFTER;
          path = btk_tree_path_new_from_indices (n_children - 1, -1);
        }
      else
        {
          pos = BTK_TREE_VIEW_DROP_BEFORE;
          path = btk_tree_path_new_from_indices (0, -1);
        }

      can_drop = TRUE;

      goto out;
    }

  g_assert (path);

  /* If we left the current row's "open" zone, unset the timeout for
   * opening the row
   */
  btk_tree_view_get_drag_dest_row (tree_view,
                                   &old_dest_path,
                                   &old_pos);

  if (old_dest_path &&
      (btk_tree_path_compare (path, old_dest_path) != 0 ||
       !(pos == BTK_TREE_VIEW_DROP_INTO_OR_AFTER ||
         pos == BTK_TREE_VIEW_DROP_INTO_OR_BEFORE)))
    remove_open_timeout (tree_view);

  if (old_dest_path)
    btk_tree_path_free (old_dest_path);

  if (TRUE /* FIXME if the location droppable predicate */)
    {
      can_drop = TRUE;
    }

out:
  if (can_drop)
    {
      BtkWidget *source_widget;

      *suggested_action = bdk_drag_context_get_suggested_action (context);
      source_widget = btk_drag_get_source_widget (context);

      if (source_widget == widget)
        {
          /* Default to MOVE, unless the user has
           * pressed ctrl or shift to affect available actions
           */
          if ((bdk_drag_context_get_actions (context) & BDK_ACTION_MOVE) != 0)
            *suggested_action = BDK_ACTION_MOVE;
        }

      btk_tree_view_set_drag_dest_row (BTK_TREE_VIEW (widget),
                                       path, pos);
    }
  else
    {
      /* can't drop here */
      remove_open_timeout (tree_view);

      btk_tree_view_set_drag_dest_row (BTK_TREE_VIEW (widget),
                                       NULL,
                                       BTK_TREE_VIEW_DROP_BEFORE);
    }

  if (path)
    btk_tree_path_free (path);

  return TRUE;
}

static BtkTreePath*
get_logical_dest_row (BtkTreeView *tree_view,
                      gboolean    *path_down_mode,
                      gboolean    *drop_append_mode)
{
  /* adjust path to point to the row the drop goes in front of */
  BtkTreePath *path = NULL;
  BtkTreeViewDropPosition pos;

  g_return_val_if_fail (path_down_mode != NULL, NULL);
  g_return_val_if_fail (drop_append_mode != NULL, NULL);

  *path_down_mode = FALSE;
  *drop_append_mode = 0;

  btk_tree_view_get_drag_dest_row (tree_view, &path, &pos);

  if (path == NULL)
    return NULL;

  if (pos == BTK_TREE_VIEW_DROP_BEFORE)
    ; /* do nothing */
  else if (pos == BTK_TREE_VIEW_DROP_INTO_OR_BEFORE ||
           pos == BTK_TREE_VIEW_DROP_INTO_OR_AFTER)
    *path_down_mode = TRUE;
  else
    {
      BtkTreeIter iter;
      BtkTreeModel *model = btk_tree_view_get_model (tree_view);

      g_assert (pos == BTK_TREE_VIEW_DROP_AFTER);

      if (!btk_tree_model_get_iter (model, &iter, path) ||
          !btk_tree_model_iter_next (model, &iter))
        *drop_append_mode = 1;
      else
        {
          *drop_append_mode = 0;
          btk_tree_path_next (path);
        }
    }

  return path;
}

static gboolean
btk_tree_view_maybe_begin_dragging_row (BtkTreeView      *tree_view,
                                        BdkEventMotion   *event)
{
  BtkWidget *widget = BTK_WIDGET (tree_view);
  BdkDragContext *context;
  TreeViewDragInfo *di;
  BtkTreePath *path = NULL;
  gint button;
  gint cell_x, cell_y;
  BtkTreeModel *model;
  gboolean retval = FALSE;

  di = get_info (tree_view);

  if (di == NULL || !di->source_set)
    goto out;

  if (tree_view->priv->pressed_button < 0)
    goto out;

  if (!btk_drag_check_threshold (widget,
                                 tree_view->priv->press_start_x,
                                 tree_view->priv->press_start_y,
                                 event->x, event->y))
    goto out;

  model = btk_tree_view_get_model (tree_view);

  if (model == NULL)
    goto out;

  button = tree_view->priv->pressed_button;
  tree_view->priv->pressed_button = -1;

  btk_tree_view_get_path_at_pos (tree_view,
                                 tree_view->priv->press_start_x,
                                 tree_view->priv->press_start_y,
                                 &path,
                                 NULL,
                                 &cell_x,
                                 &cell_y);

  if (path == NULL)
    goto out;

  if (!BTK_IS_TREE_DRAG_SOURCE (model) ||
      !btk_tree_drag_source_row_draggable (BTK_TREE_DRAG_SOURCE (model),
					   path))
    goto out;

  if (!(BDK_BUTTON1_MASK << (button - 1) & di->start_button_mask))
    goto out;

  /* Now we can begin the drag */

  retval = TRUE;

  context = btk_drag_begin (widget,
                            btk_drag_source_get_target_list (widget),
                            di->source_actions,
                            button,
                            (BdkEvent*)event);

  set_source_row (context, model, path);

 out:
  if (path)
    btk_tree_path_free (path);

  return retval;
}


static void
btk_tree_view_drag_begin (BtkWidget      *widget,
                          BdkDragContext *context)
{
  BtkTreeView *tree_view;
  BtkTreePath *path = NULL;
  gint cell_x, cell_y;
  BdkPixmap *row_pix;
  TreeViewDragInfo *di;

  tree_view = BTK_TREE_VIEW (widget);

  /* if the user uses a custom DND source impl, we don't set the icon here */
  di = get_info (tree_view);

  if (di == NULL || !di->source_set)
    return;

  btk_tree_view_get_path_at_pos (tree_view,
                                 tree_view->priv->press_start_x,
                                 tree_view->priv->press_start_y,
                                 &path,
                                 NULL,
                                 &cell_x,
                                 &cell_y);

  g_return_if_fail (path != NULL);

  row_pix = btk_tree_view_create_row_drag_icon (tree_view,
                                                path);

  btk_drag_set_icon_pixmap (context,
                            bdk_drawable_get_colormap (row_pix),
                            row_pix,
                            NULL,
                            /* the + 1 is for the black border in the icon */
                            tree_view->priv->press_start_x + 1,
                            cell_y + 1);

  g_object_unref (row_pix);
  btk_tree_path_free (path);
}

static void
btk_tree_view_drag_end (BtkWidget      *widget,
                        BdkDragContext *context)
{
  /* do nothing */
}

/* Default signal implementations for the drag signals */
static void
btk_tree_view_drag_data_get (BtkWidget        *widget,
                             BdkDragContext   *context,
                             BtkSelectionData *selection_data,
                             guint             info,
                             guint             time)
{
  BtkTreeView *tree_view;
  BtkTreeModel *model;
  TreeViewDragInfo *di;
  BtkTreePath *source_row;

  tree_view = BTK_TREE_VIEW (widget);

  model = btk_tree_view_get_model (tree_view);

  if (model == NULL)
    return;

  di = get_info (BTK_TREE_VIEW (widget));

  if (di == NULL)
    return;

  source_row = get_source_row (context);

  if (source_row == NULL)
    return;

  /* We can implement the BTK_TREE_MODEL_ROW target generically for
   * any model; for DragSource models there are some other targets
   * we also support.
   */

  if (BTK_IS_TREE_DRAG_SOURCE (model) &&
      btk_tree_drag_source_drag_data_get (BTK_TREE_DRAG_SOURCE (model),
                                          source_row,
                                          selection_data))
    goto done;

  /* If drag_data_get does nothing, try providing row data. */
  if (selection_data->target == bdk_atom_intern_static_string ("BTK_TREE_MODEL_ROW"))
    {
      btk_tree_set_row_drag_data (selection_data,
				  model,
				  source_row);
    }

 done:
  btk_tree_path_free (source_row);
}


static void
btk_tree_view_drag_data_delete (BtkWidget      *widget,
                                BdkDragContext *context)
{
  TreeViewDragInfo *di;
  BtkTreeModel *model;
  BtkTreeView *tree_view;
  BtkTreePath *source_row;

  tree_view = BTK_TREE_VIEW (widget);
  model = btk_tree_view_get_model (tree_view);

  if (!check_model_dnd (model, BTK_TYPE_TREE_DRAG_SOURCE, "drag_data_delete"))
    return;

  di = get_info (tree_view);

  if (di == NULL)
    return;

  source_row = get_source_row (context);

  if (source_row == NULL)
    return;

  btk_tree_drag_source_drag_data_delete (BTK_TREE_DRAG_SOURCE (model),
                                         source_row);

  btk_tree_path_free (source_row);

  set_source_row (context, NULL, NULL);
}

static void
btk_tree_view_drag_leave (BtkWidget      *widget,
                          BdkDragContext *context,
                          guint             time)
{
  /* unset any highlight row */
  btk_tree_view_set_drag_dest_row (BTK_TREE_VIEW (widget),
                                   NULL,
                                   BTK_TREE_VIEW_DROP_BEFORE);

  remove_scroll_timeout (BTK_TREE_VIEW (widget));
  remove_open_timeout (BTK_TREE_VIEW (widget));
}


static gboolean
btk_tree_view_drag_motion (BtkWidget        *widget,
                           BdkDragContext   *context,
			   /* coordinates relative to the widget */
                           gint              x,
                           gint              y,
                           guint             time)
{
  gboolean empty;
  BtkTreePath *path = NULL;
  BtkTreeViewDropPosition pos;
  BtkTreeView *tree_view;
  BdkDragAction suggested_action = 0;
  BdkAtom target;

  tree_view = BTK_TREE_VIEW (widget);

  if (!set_destination_row (tree_view, context, x, y, &suggested_action, &target))
    return FALSE;

  btk_tree_view_get_drag_dest_row (tree_view, &path, &pos);

  /* we only know this *after* set_desination_row */
  empty = tree_view->priv->empty_view_drop;

  if (path == NULL && !empty)
    {
      /* Can't drop here. */
      bdk_drag_status (context, 0, time);
    }
  else
    {
      if (tree_view->priv->open_dest_timeout == 0 &&
          (pos == BTK_TREE_VIEW_DROP_INTO_OR_AFTER ||
           pos == BTK_TREE_VIEW_DROP_INTO_OR_BEFORE))
        {
          tree_view->priv->open_dest_timeout =
            bdk_threads_add_timeout (AUTO_EXPAND_TIMEOUT, open_row_timeout, tree_view);
        }
      else
        {
	  add_scroll_timeout (tree_view);
	}

      if (target == bdk_atom_intern_static_string ("BTK_TREE_MODEL_ROW"))
        {
          /* Request data so we can use the source row when
           * determining whether to accept the drop
           */
          set_status_pending (context, suggested_action);
          btk_drag_get_data (widget, context, target, time);
        }
      else
        {
          set_status_pending (context, 0);
          bdk_drag_status (context, suggested_action, time);
        }
    }

  if (path)
    btk_tree_path_free (path);

  return TRUE;
}


static gboolean
btk_tree_view_drag_drop (BtkWidget        *widget,
                         BdkDragContext   *context,
			 /* coordinates relative to the widget */
                         gint              x,
                         gint              y,
                         guint             time)
{
  BtkTreeView *tree_view;
  BtkTreePath *path;
  BdkDragAction suggested_action = 0;
  BdkAtom target = BDK_NONE;
  TreeViewDragInfo *di;
  BtkTreeModel *model;
  gboolean path_down_mode;
  gboolean drop_append_mode;

  tree_view = BTK_TREE_VIEW (widget);

  model = btk_tree_view_get_model (tree_view);

  remove_scroll_timeout (BTK_TREE_VIEW (widget));
  remove_open_timeout (BTK_TREE_VIEW (widget));

  di = get_info (tree_view);

  if (di == NULL)
    return FALSE;

  if (!check_model_dnd (model, BTK_TYPE_TREE_DRAG_DEST, "drag_drop"))
    return FALSE;

  if (!set_destination_row (tree_view, context, x, y, &suggested_action, &target))
    return FALSE;

  path = get_logical_dest_row (tree_view, &path_down_mode, &drop_append_mode);

  if (target != BDK_NONE && path != NULL)
    {
      /* in case a motion had requested drag data, change things so we
       * treat drag data receives as a drop.
       */
      set_status_pending (context, 0);
      set_dest_row (context, model, path,
                    path_down_mode, tree_view->priv->empty_view_drop,
                    drop_append_mode);
    }

  if (path)
    btk_tree_path_free (path);

  /* Unset this thing */
  btk_tree_view_set_drag_dest_row (BTK_TREE_VIEW (widget),
                                   NULL,
                                   BTK_TREE_VIEW_DROP_BEFORE);

  if (target != BDK_NONE)
    {
      btk_drag_get_data (widget, context, target, time);
      return TRUE;
    }
  else
    return FALSE;
}

static void
btk_tree_view_drag_data_received (BtkWidget        *widget,
                                  BdkDragContext   *context,
				  /* coordinates relative to the widget */
                                  gint              x,
                                  gint              y,
                                  BtkSelectionData *selection_data,
                                  guint             info,
                                  guint             time)
{
  BtkTreePath *path;
  TreeViewDragInfo *di;
  gboolean accepted = FALSE;
  BtkTreeModel *model;
  BtkTreeView *tree_view;
  BtkTreePath *dest_row;
  BdkDragAction suggested_action;
  gboolean path_down_mode;
  gboolean drop_append_mode;

  tree_view = BTK_TREE_VIEW (widget);

  model = btk_tree_view_get_model (tree_view);

  if (!check_model_dnd (model, BTK_TYPE_TREE_DRAG_DEST, "drag_data_received"))
    return;

  di = get_info (tree_view);

  if (di == NULL)
    return;

  suggested_action = get_status_pending (context);

  if (suggested_action)
    {
      /* We are getting this data due to a request in drag_motion,
       * rather than due to a request in drag_drop, so we are just
       * supposed to call drag_status, not actually paste in the
       * data.
       */
      path = get_logical_dest_row (tree_view, &path_down_mode,
                                   &drop_append_mode);

      if (path == NULL)
        suggested_action = 0;
      else if (path_down_mode)
        btk_tree_path_down (path);

      if (suggested_action)
        {
	  if (!btk_tree_drag_dest_row_drop_possible (BTK_TREE_DRAG_DEST (model),
						     path,
						     selection_data))
            {
              if (path_down_mode)
                {
                  path_down_mode = FALSE;
                  btk_tree_path_up (path);

                  if (!btk_tree_drag_dest_row_drop_possible (BTK_TREE_DRAG_DEST (model),
                                                             path,
                                                             selection_data))
                    suggested_action = 0;
                }
              else
	        suggested_action = 0;
            }
        }

      bdk_drag_status (context, suggested_action, time);

      if (path)
        btk_tree_path_free (path);

      /* If you can't drop, remove user drop indicator until the next motion */
      if (suggested_action == 0)
        btk_tree_view_set_drag_dest_row (BTK_TREE_VIEW (widget),
                                         NULL,
                                         BTK_TREE_VIEW_DROP_BEFORE);

      return;
    }

  dest_row = get_dest_row (context, &path_down_mode);

  if (dest_row == NULL)
    return;

  if (selection_data->length >= 0)
    {
      if (path_down_mode)
        {
          btk_tree_path_down (dest_row);
          if (!btk_tree_drag_dest_row_drop_possible (BTK_TREE_DRAG_DEST (model),
                                                     dest_row, selection_data))
            btk_tree_path_up (dest_row);
        }
    }

  if (selection_data->length >= 0)
    {
      if (btk_tree_drag_dest_drag_data_received (BTK_TREE_DRAG_DEST (model),
                                                 dest_row,
                                                 selection_data))
        accepted = TRUE;
    }

  btk_drag_finish (context,
                   accepted,
                   (bdk_drag_context_get_selected_action (context) == BDK_ACTION_MOVE),
                   time);

  if (btk_tree_path_get_depth (dest_row) == 1
      && btk_tree_path_get_indices (dest_row)[0] == 0)
    {
      /* special special case drag to "0", scroll to first item */
      if (!tree_view->priv->scroll_to_path)
        btk_tree_view_scroll_to_cell (tree_view, dest_row, NULL, FALSE, 0.0, 0.0);
    }

  btk_tree_path_free (dest_row);

  /* drop dest_row */
  set_dest_row (context, NULL, NULL, FALSE, FALSE, FALSE);
}



/* BtkContainer Methods
 */


static void
btk_tree_view_remove (BtkContainer *container,
		      BtkWidget    *widget)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (container);
  BtkTreeViewChild *child = NULL;
  GList *tmp_list;

  tmp_list = tree_view->priv->children;
  while (tmp_list)
    {
      child = tmp_list->data;
      if (child->widget == widget)
	{
	  btk_widget_unparent (widget);

	  tree_view->priv->children = g_list_remove_link (tree_view->priv->children, tmp_list);
	  g_list_free_1 (tmp_list);
	  g_slice_free (BtkTreeViewChild, child);
	  return;
	}

      tmp_list = tmp_list->next;
    }

  tmp_list = tree_view->priv->columns;

  while (tmp_list)
    {
      BtkTreeViewColumn *column;
      column = tmp_list->data;
      if (column->button == widget)
	{
	  btk_widget_unparent (widget);
	  return;
	}
      tmp_list = tmp_list->next;
    }
}

static void
btk_tree_view_forall (BtkContainer *container,
		      gboolean      include_internals,
		      BtkCallback   callback,
		      gpointer      callback_data)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (container);
  BtkTreeViewChild *child = NULL;
  BtkTreeViewColumn *column;
  GList *tmp_list;

  tmp_list = tree_view->priv->children;
  while (tmp_list)
    {
      child = tmp_list->data;
      tmp_list = tmp_list->next;

      (* callback) (child->widget, callback_data);
    }
  if (include_internals == FALSE)
    return;

  for (tmp_list = tree_view->priv->columns; tmp_list; tmp_list = tmp_list->next)
    {
      column = tmp_list->data;

      if (column->button)
	(* callback) (column->button, callback_data);
    }
}

/* Returns TRUE if the treeview contains no "special" (editable or activatable)
 * cells. If so we draw one big row-spanning focus rectangle.
 */
static gboolean
btk_tree_view_has_special_cell (BtkTreeView *tree_view)
{
  GList *list;

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      if (!((BtkTreeViewColumn *)list->data)->visible)
	continue;
      if (_btk_tree_view_column_count_special_cells (list->data))
	return TRUE;
    }

  return FALSE;
}

static void
column_sizing_notify (BObject    *object,
                      BParamSpec *pspec,
                      gpointer    data)
{
  BtkTreeViewColumn *c = BTK_TREE_VIEW_COLUMN (object);

  if (btk_tree_view_column_get_sizing (c) != BTK_TREE_VIEW_COLUMN_FIXED)
    /* disable fixed height mode */
    g_object_set (data, "fixed-height-mode", FALSE, NULL);
}

/**
 * btk_tree_view_set_fixed_height_mode:
 * @tree_view: a #BtkTreeView 
 * @enable: %TRUE to enable fixed height mode
 * 
 * Enables or disables the fixed height mode of @tree_view. 
 * Fixed height mode speeds up #BtkTreeView by assuming that all 
 * rows have the same height. 
 * Only enable this option if all rows are the same height and all
 * columns are of type %BTK_TREE_VIEW_COLUMN_FIXED.
 *
 * Since: 2.6 
 **/
void
btk_tree_view_set_fixed_height_mode (BtkTreeView *tree_view,
                                     gboolean     enable)
{
  GList *l;
  
  enable = enable != FALSE;

  if (enable == tree_view->priv->fixed_height_mode)
    return;

  if (!enable)
    {
      tree_view->priv->fixed_height_mode = 0;
      tree_view->priv->fixed_height = -1;

      /* force a revalidation */
      install_presize_handler (tree_view);
    }
  else 
    {
      /* make sure all columns are of type FIXED */
      for (l = tree_view->priv->columns; l; l = l->next)
	{
	  BtkTreeViewColumn *c = l->data;
	  
	  g_return_if_fail (btk_tree_view_column_get_sizing (c) == BTK_TREE_VIEW_COLUMN_FIXED);
	}
      
      /* yes, we really have to do this is in a separate loop */
      for (l = tree_view->priv->columns; l; l = l->next)
	g_signal_connect (l->data, "notify::sizing",
			  G_CALLBACK (column_sizing_notify), tree_view);
      
      tree_view->priv->fixed_height_mode = 1;
      tree_view->priv->fixed_height = -1;
      
      if (tree_view->priv->tree)
	initialize_fixed_height_mode (tree_view);
    }

  g_object_notify (B_OBJECT (tree_view), "fixed-height-mode");
}

/**
 * btk_tree_view_get_fixed_height_mode:
 * @tree_view: a #BtkTreeView
 * 
 * Returns whether fixed height mode is turned on for @tree_view.
 * 
 * Return value: %TRUE if @tree_view is in fixed height mode
 * 
 * Since: 2.6
 **/
gboolean
btk_tree_view_get_fixed_height_mode (BtkTreeView *tree_view)
{
  return tree_view->priv->fixed_height_mode;
}

/* Returns TRUE if the focus is within the headers, after the focus operation is
 * done
 */
static gboolean
btk_tree_view_header_focus (BtkTreeView      *tree_view,
			    BtkDirectionType  dir,
			    gboolean          clamp_column_visible)
{
  BtkWidget *focus_child;

  GList *last_column, *first_column;
  GList *tmp_list;
  gboolean rtl;

  if (! BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_HEADERS_VISIBLE))
    return FALSE;

  focus_child = BTK_CONTAINER (tree_view)->focus_child;

  first_column = tree_view->priv->columns;
  while (first_column)
    {
      if (btk_widget_get_can_focus (BTK_TREE_VIEW_COLUMN (first_column->data)->button) &&
	  BTK_TREE_VIEW_COLUMN (first_column->data)->visible &&
	  (BTK_TREE_VIEW_COLUMN (first_column->data)->clickable ||
	   BTK_TREE_VIEW_COLUMN (first_column->data)->reorderable))
	break;
      first_column = first_column->next;
    }

  /* No headers are visible, or are focusable.  We can't focus in or out.
   */
  if (first_column == NULL)
    return FALSE;

  last_column = g_list_last (tree_view->priv->columns);
  while (last_column)
    {
      if (btk_widget_get_can_focus (BTK_TREE_VIEW_COLUMN (last_column->data)->button) &&
	  BTK_TREE_VIEW_COLUMN (last_column->data)->visible &&
	  (BTK_TREE_VIEW_COLUMN (last_column->data)->clickable ||
	   BTK_TREE_VIEW_COLUMN (last_column->data)->reorderable))
	break;
      last_column = last_column->prev;
    }


  rtl = (btk_widget_get_direction (BTK_WIDGET (tree_view)) == BTK_TEXT_DIR_RTL);

  switch (dir)
    {
    case BTK_DIR_TAB_BACKWARD:
    case BTK_DIR_TAB_FORWARD:
    case BTK_DIR_UP:
    case BTK_DIR_DOWN:
      if (focus_child == NULL)
	{
	  if (tree_view->priv->focus_column != NULL &&
              btk_widget_get_can_focus (tree_view->priv->focus_column->button))
	    focus_child = tree_view->priv->focus_column->button;
	  else
	    focus_child = BTK_TREE_VIEW_COLUMN (first_column->data)->button;
	  btk_widget_grab_focus (focus_child);
	  break;
	}
      return FALSE;

    case BTK_DIR_LEFT:
    case BTK_DIR_RIGHT:
      if (focus_child == NULL)
	{
	  if (tree_view->priv->focus_column != NULL)
	    focus_child = tree_view->priv->focus_column->button;
	  else if (dir == BTK_DIR_LEFT)
	    focus_child = BTK_TREE_VIEW_COLUMN (last_column->data)->button;
	  else
	    focus_child = BTK_TREE_VIEW_COLUMN (first_column->data)->button;
	  btk_widget_grab_focus (focus_child);
	  break;
	}

      if (btk_widget_child_focus (focus_child, dir))
	{
	  /* The focus moves inside the button. */
	  /* This is probably a great example of bad UI */
	  break;
	}

      /* We need to move the focus among the row of buttons. */
      for (tmp_list = tree_view->priv->columns; tmp_list; tmp_list = tmp_list->next)
	if (BTK_TREE_VIEW_COLUMN (tmp_list->data)->button == focus_child)
	  break;

      if ((tmp_list == first_column && dir == (rtl ? BTK_DIR_RIGHT : BTK_DIR_LEFT))
	  || (tmp_list == last_column && dir == (rtl ? BTK_DIR_LEFT : BTK_DIR_RIGHT)))
        {
	  btk_widget_error_bell (BTK_WIDGET (tree_view));
	  break;
	}

      while (tmp_list)
	{
	  BtkTreeViewColumn *column;

	  if (dir == (rtl ? BTK_DIR_LEFT : BTK_DIR_RIGHT))
	    tmp_list = tmp_list->next;
	  else
	    tmp_list = tmp_list->prev;

	  if (tmp_list == NULL)
	    {
	      g_warning ("Internal button not found");
	      break;
	    }
	  column = tmp_list->data;
	  if (column->button &&
	      column->visible &&
	      btk_widget_get_can_focus (column->button))
	    {
	      focus_child = column->button;
	      btk_widget_grab_focus (column->button);
	      break;
	    }
	}
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  /* if focus child is non-null, we assume it's been set to the current focus child
   */
  if (focus_child)
    {
      for (tmp_list = tree_view->priv->columns; tmp_list; tmp_list = tmp_list->next)
	if (BTK_TREE_VIEW_COLUMN (tmp_list->data)->button == focus_child)
	  {
	    tree_view->priv->focus_column = BTK_TREE_VIEW_COLUMN (tmp_list->data);
	    break;
	  }

      if (clamp_column_visible)
        {
	  btk_tree_view_clamp_column_visible (tree_view,
					      tree_view->priv->focus_column,
					      FALSE);
	}
    }

  return (focus_child != NULL);
}

/* This function returns in 'path' the first focusable path, if the given path
 * is already focusable, it's the returned one.
 */
static gboolean
search_first_focusable_path (BtkTreeView  *tree_view,
			     BtkTreePath **path,
			     gboolean      search_forward,
			     BtkRBTree   **new_tree,
			     BtkRBNode   **new_node)
{
  BtkRBTree *tree = NULL;
  BtkRBNode *node = NULL;

  if (!path || !*path)
    return FALSE;

  _btk_tree_view_find_node (tree_view, *path, &tree, &node);

  if (!tree || !node)
    return FALSE;

  while (node && row_is_separator (tree_view, NULL, *path))
    {
      if (search_forward)
	_btk_rbtree_next_full (tree, node, &tree, &node);
      else
	_btk_rbtree_prev_full (tree, node, &tree, &node);

      if (*path)
	btk_tree_path_free (*path);

      if (node)
	*path = _btk_tree_view_find_path (tree_view, tree, node);
      else
	*path = NULL;
    }

  if (new_tree)
    *new_tree = tree;

  if (new_node)
    *new_node = node;

  return (*path != NULL);
}

static gint
btk_tree_view_focus (BtkWidget        *widget,
		     BtkDirectionType  direction)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (widget);
  BtkContainer *container = BTK_CONTAINER (widget);
  BtkWidget *focus_child;

  if (!btk_widget_is_sensitive (widget) || !btk_widget_get_can_focus (widget))
    return FALSE;

  focus_child = container->focus_child;

  btk_tree_view_stop_editing (BTK_TREE_VIEW (widget), FALSE);
  /* Case 1.  Headers currently have focus. */
  if (focus_child)
    {
      switch (direction)
	{
	case BTK_DIR_LEFT:
	case BTK_DIR_RIGHT:
	  btk_tree_view_header_focus (tree_view, direction, TRUE);
	  return TRUE;
	case BTK_DIR_TAB_BACKWARD:
	case BTK_DIR_UP:
	  return FALSE;
	case BTK_DIR_TAB_FORWARD:
	case BTK_DIR_DOWN:
	  btk_widget_grab_focus (widget);
	  return TRUE;
	default:
	  g_assert_not_reached ();
	  return FALSE;
	}
    }

  /* Case 2. We don't have focus at all. */
  if (!btk_widget_has_focus (widget))
    {
      btk_widget_grab_focus (widget);
      return TRUE;
    }

  /* Case 3. We have focus already. */
  if (direction == BTK_DIR_TAB_BACKWARD)
    return (btk_tree_view_header_focus (tree_view, direction, FALSE));
  else if (direction == BTK_DIR_TAB_FORWARD)
    return FALSE;

  /* Other directions caught by the keybindings */
  btk_widget_grab_focus (widget);
  return TRUE;
}

static void
btk_tree_view_grab_focus (BtkWidget *widget)
{
  BTK_WIDGET_CLASS (btk_tree_view_parent_class)->grab_focus (widget);

  btk_tree_view_focus_to_cursor (BTK_TREE_VIEW (widget));
}

static void
btk_tree_view_style_set (BtkWidget *widget,
			 BtkStyle *previous_style)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (widget);
  GList *list;
  BtkTreeViewColumn *column;

  if (btk_widget_get_realized (widget))
    {
      bdk_window_set_back_pixmap (widget->window, NULL, FALSE);
      bdk_window_set_background (tree_view->priv->bin_window, &widget->style->base[widget->state]);
      btk_style_set_background (widget->style, tree_view->priv->header_window, BTK_STATE_NORMAL);

      btk_tree_view_set_grid_lines (tree_view, tree_view->priv->grid_lines);
      btk_tree_view_set_enable_tree_lines (tree_view, tree_view->priv->tree_lines_enabled);
    }

  btk_widget_style_get (widget,
			"expander-size", &tree_view->priv->expander_size,
			NULL);
  tree_view->priv->expander_size += EXPANDER_EXTRA_PADDING;

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      column = list->data;
      _btk_tree_view_column_cell_set_dirty (column, TRUE);
    }

  tree_view->priv->fixed_height = -1;
  _btk_rbtree_mark_invalid (tree_view->priv->tree);

  btk_widget_queue_resize (widget);
}


static void
btk_tree_view_set_focus_child (BtkContainer *container,
			       BtkWidget    *child)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (container);
  GList *list;

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      if (BTK_TREE_VIEW_COLUMN (list->data)->button == child)
	{
	  tree_view->priv->focus_column = BTK_TREE_VIEW_COLUMN (list->data);
	  break;
	}
    }

  BTK_CONTAINER_CLASS (btk_tree_view_parent_class)->set_focus_child (container, child);
}

static void
btk_tree_view_set_adjustments (BtkTreeView   *tree_view,
			       BtkAdjustment *hadj,
			       BtkAdjustment *vadj)
{
  gboolean need_adjust = FALSE;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  if (hadj)
    g_return_if_fail (BTK_IS_ADJUSTMENT (hadj));
  else
    hadj = BTK_ADJUSTMENT (btk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  if (vadj)
    g_return_if_fail (BTK_IS_ADJUSTMENT (vadj));
  else
    vadj = BTK_ADJUSTMENT (btk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));

  if (tree_view->priv->hadjustment && (tree_view->priv->hadjustment != hadj))
    {
      g_signal_handlers_disconnect_by_func (tree_view->priv->hadjustment,
					    btk_tree_view_adjustment_changed,
					    tree_view);
      g_object_unref (tree_view->priv->hadjustment);
    }

  if (tree_view->priv->vadjustment && (tree_view->priv->vadjustment != vadj))
    {
      g_signal_handlers_disconnect_by_func (tree_view->priv->vadjustment,
					    btk_tree_view_adjustment_changed,
					    tree_view);
      g_object_unref (tree_view->priv->vadjustment);
    }

  if (tree_view->priv->hadjustment != hadj)
    {
      tree_view->priv->hadjustment = hadj;
      g_object_ref_sink (tree_view->priv->hadjustment);

      g_signal_connect (tree_view->priv->hadjustment, "value-changed",
			G_CALLBACK (btk_tree_view_adjustment_changed),
			tree_view);
      need_adjust = TRUE;
    }

  if (tree_view->priv->vadjustment != vadj)
    {
      tree_view->priv->vadjustment = vadj;
      g_object_ref_sink (tree_view->priv->vadjustment);

      g_signal_connect (tree_view->priv->vadjustment, "value-changed",
			G_CALLBACK (btk_tree_view_adjustment_changed),
			tree_view);
      need_adjust = TRUE;
    }

  if (need_adjust)
    btk_tree_view_adjustment_changed (NULL, tree_view);
}


static gboolean
btk_tree_view_real_move_cursor (BtkTreeView       *tree_view,
				BtkMovementStep    step,
				gint               count)
{
  BdkModifierType state;

  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), FALSE);
  g_return_val_if_fail (step == BTK_MOVEMENT_LOGICAL_POSITIONS ||
			step == BTK_MOVEMENT_VISUAL_POSITIONS ||
			step == BTK_MOVEMENT_DISPLAY_LINES ||
			step == BTK_MOVEMENT_PAGES ||
			step == BTK_MOVEMENT_BUFFER_ENDS, FALSE);

  if (tree_view->priv->tree == NULL)
    return FALSE;
  if (!btk_widget_has_focus (BTK_WIDGET (tree_view)))
    return FALSE;

  btk_tree_view_stop_editing (tree_view, FALSE);
  BTK_TREE_VIEW_SET_FLAG (tree_view, BTK_TREE_VIEW_DRAW_KEYFOCUS);
  btk_widget_grab_focus (BTK_WIDGET (tree_view));

  if (btk_get_current_event_state (&state))
    {
      if ((state & BTK_MODIFY_SELECTION_MOD_MASK) == BTK_MODIFY_SELECTION_MOD_MASK)
        tree_view->priv->modify_selection_pressed = TRUE;
      if ((state & BTK_EXTEND_SELECTION_MOD_MASK) == BTK_EXTEND_SELECTION_MOD_MASK)
        tree_view->priv->extend_selection_pressed = TRUE;
    }
  /* else we assume not pressed */

  switch (step)
    {
      /* currently we make no distinction.  When we go bi-di, we need to */
    case BTK_MOVEMENT_LOGICAL_POSITIONS:
    case BTK_MOVEMENT_VISUAL_POSITIONS:
      btk_tree_view_move_cursor_left_right (tree_view, count);
      break;
    case BTK_MOVEMENT_DISPLAY_LINES:
      btk_tree_view_move_cursor_up_down (tree_view, count);
      break;
    case BTK_MOVEMENT_PAGES:
      btk_tree_view_move_cursor_page_up_down (tree_view, count);
      break;
    case BTK_MOVEMENT_BUFFER_ENDS:
      btk_tree_view_move_cursor_start_end (tree_view, count);
      break;
    default:
      g_assert_not_reached ();
    }

  tree_view->priv->modify_selection_pressed = FALSE;
  tree_view->priv->extend_selection_pressed = FALSE;

  return TRUE;
}

static void
btk_tree_view_put (BtkTreeView *tree_view,
		   BtkWidget   *child_widget,
		   /* in bin_window coordinates */
		   gint         x,
		   gint         y,
		   gint         width,
		   gint         height)
{
  BtkTreeViewChild *child;
  
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (BTK_IS_WIDGET (child_widget));

  child = g_slice_new (BtkTreeViewChild);

  child->widget = child_widget;
  child->x = x;
  child->y = y;
  child->width = width;
  child->height = height;

  tree_view->priv->children = g_list_append (tree_view->priv->children, child);

  if (btk_widget_get_realized (BTK_WIDGET (tree_view)))
    btk_widget_set_parent_window (child->widget, tree_view->priv->bin_window);
  
  btk_widget_set_parent (child_widget, BTK_WIDGET (tree_view));
}

void
_btk_tree_view_child_move_resize (BtkTreeView *tree_view,
				  BtkWidget   *widget,
				  /* in tree coordinates */
				  gint         x,
				  gint         y,
				  gint         width,
				  gint         height)
{
  BtkTreeViewChild *child = NULL;
  GList *list;
  BdkRectangle allocation;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (BTK_IS_WIDGET (widget));

  for (list = tree_view->priv->children; list; list = list->next)
    {
      if (((BtkTreeViewChild *)list->data)->widget == widget)
	{
	  child = list->data;
	  break;
	}
    }
  if (child == NULL)
    return;

  allocation.x = child->x = x;
  allocation.y = child->y = y;
  allocation.width = child->width = width;
  allocation.height = child->height = height;

  if (btk_widget_get_realized (widget))
    btk_widget_size_allocate (widget, &allocation);
}


/* TreeModel Callbacks
 */

static void
btk_tree_view_row_changed (BtkTreeModel *model,
			   BtkTreePath  *path,
			   BtkTreeIter  *iter,
			   gpointer      data)
{
  BtkTreeView *tree_view = (BtkTreeView *)data;
  BtkRBTree *tree;
  BtkRBNode *node;
  gboolean free_path = FALSE;
  GList *list;
  BtkTreePath *cursor_path;

  g_return_if_fail (path != NULL || iter != NULL);

  if (tree_view->priv->cursor != NULL)
    cursor_path = btk_tree_row_reference_get_path (tree_view->priv->cursor);
  else
    cursor_path = NULL;

  if (tree_view->priv->edited_column &&
      (cursor_path == NULL || btk_tree_path_compare (cursor_path, path) == 0))
    btk_tree_view_stop_editing (tree_view, TRUE);

  if (cursor_path != NULL)
    btk_tree_path_free (cursor_path);

  if (path == NULL)
    {
      path = btk_tree_model_get_path (model, iter);
      free_path = TRUE;
    }
  else if (iter == NULL)
    btk_tree_model_get_iter (model, iter, path);

  if (_btk_tree_view_find_node (tree_view,
				path,
				&tree,
				&node))
    /* We aren't actually showing the node */
    goto done;

  if (tree == NULL)
    goto done;

  if (tree_view->priv->fixed_height_mode
      && tree_view->priv->fixed_height >= 0)
    {
      _btk_rbtree_node_set_height (tree, node, tree_view->priv->fixed_height);
      if (btk_widget_get_realized (BTK_WIDGET (tree_view)))
	btk_tree_view_node_queue_redraw (tree_view, tree, node);
    }
  else
    {
      _btk_rbtree_node_mark_invalid (tree, node);
      for (list = tree_view->priv->columns; list; list = list->next)
        {
          BtkTreeViewColumn *column;

          column = list->data;
          if (! column->visible)
            continue;

          if (column->column_type == BTK_TREE_VIEW_COLUMN_AUTOSIZE)
            {
              _btk_tree_view_column_cell_set_dirty (column, TRUE);
            }
        }
    }

 done:
  if (!tree_view->priv->fixed_height_mode &&
      btk_widget_get_realized (BTK_WIDGET (tree_view)))
    install_presize_handler (tree_view);
  if (free_path)
    btk_tree_path_free (path);
}

static void
btk_tree_view_row_inserted (BtkTreeModel *model,
			    BtkTreePath  *path,
			    BtkTreeIter  *iter,
			    gpointer      data)
{
  BtkTreeView *tree_view = (BtkTreeView *) data;
  gint *indices;
  BtkRBTree *tmptree, *tree;
  BtkRBNode *tmpnode = NULL;
  gint depth;
  gint i = 0;
  gint height;
  gboolean free_path = FALSE;
  gboolean node_visible = TRUE;

  g_return_if_fail (path != NULL || iter != NULL);

  if (tree_view->priv->fixed_height_mode
      && tree_view->priv->fixed_height >= 0)
    height = tree_view->priv->fixed_height;
  else
    height = 0;

  if (path == NULL)
    {
      path = btk_tree_model_get_path (model, iter);
      free_path = TRUE;
    }
  else if (iter == NULL)
    btk_tree_model_get_iter (model, iter, path);

  if (tree_view->priv->tree == NULL)
    tree_view->priv->tree = _btk_rbtree_new ();

  tmptree = tree = tree_view->priv->tree;

  /* Update all row-references */
  btk_tree_row_reference_inserted (B_OBJECT (data), path);
  depth = btk_tree_path_get_depth (path);
  indices = btk_tree_path_get_indices (path);

  /* First, find the parent tree */
  while (i < depth - 1)
    {
      if (tmptree == NULL)
	{
	  /* We aren't showing the node */
	  node_visible = FALSE;
          goto done;
	}

      tmpnode = _btk_rbtree_find_count (tmptree, indices[i] + 1);
      if (tmpnode == NULL)
	{
	  g_warning ("A node was inserted with a parent that's not in the tree.\n" \
		     "This possibly means that a BtkTreeModel inserted a child node\n" \
		     "before the parent was inserted.");
          goto done;
	}
      else if (!BTK_RBNODE_FLAG_SET (tmpnode, BTK_RBNODE_IS_PARENT))
	{
          /* FIXME enforce correct behavior on model, probably */
	  /* In theory, the model should have emitted has_child_toggled here.  We
	   * try to catch it anyway, just to be safe, in case the model hasn't.
	   */
	  BtkTreePath *tmppath = _btk_tree_view_find_path (tree_view,
							   tree,
							   tmpnode);
	  btk_tree_view_row_has_child_toggled (model, tmppath, NULL, data);
	  btk_tree_path_free (tmppath);
          goto done;
	}

      tmptree = tmpnode->children;
      tree = tmptree;
      i++;
    }

  if (tree == NULL)
    {
      node_visible = FALSE;
      goto done;
    }

  /* ref the node */
  btk_tree_model_ref_node (tree_view->priv->model, iter);
  if (indices[depth - 1] == 0)
    {
      tmpnode = _btk_rbtree_find_count (tree, 1);
      tmpnode = _btk_rbtree_insert_before (tree, tmpnode, height, FALSE);
    }
  else
    {
      tmpnode = _btk_rbtree_find_count (tree, indices[depth - 1]);
      tmpnode = _btk_rbtree_insert_after (tree, tmpnode, height, FALSE);
    }

 done:
  if (height > 0)
    {
      if (tree)
        _btk_rbtree_node_mark_valid (tree, tmpnode);

      if (node_visible && node_is_visible (tree_view, tree, tmpnode))
	btk_widget_queue_resize (BTK_WIDGET (tree_view));
      else
	btk_widget_queue_resize_no_redraw (BTK_WIDGET (tree_view));
    }
  else
    install_presize_handler (tree_view);
  if (free_path)
    btk_tree_path_free (path);
}

static void
btk_tree_view_row_has_child_toggled (BtkTreeModel *model,
				     BtkTreePath  *path,
				     BtkTreeIter  *iter,
				     gpointer      data)
{
  BtkTreeView *tree_view = (BtkTreeView *)data;
  BtkTreeIter real_iter;
  gboolean has_child;
  BtkRBTree *tree;
  BtkRBNode *node;
  gboolean free_path = FALSE;

  g_return_if_fail (path != NULL || iter != NULL);

  if (iter)
    real_iter = *iter;

  if (path == NULL)
    {
      path = btk_tree_model_get_path (model, iter);
      free_path = TRUE;
    }
  else if (iter == NULL)
    btk_tree_model_get_iter (model, &real_iter, path);

  if (_btk_tree_view_find_node (tree_view,
				path,
				&tree,
				&node))
    /* We aren't actually showing the node */
    goto done;

  if (tree == NULL)
    goto done;

  has_child = btk_tree_model_iter_has_child (model, &real_iter);
  /* Sanity check.
   */
  if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_PARENT) == has_child)
    goto done;

  if (has_child)
    BTK_RBNODE_SET_FLAG (node, BTK_RBNODE_IS_PARENT);
  else
    BTK_RBNODE_UNSET_FLAG (node, BTK_RBNODE_IS_PARENT);

  if (has_child && BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_IS_LIST))
    {
      BTK_TREE_VIEW_UNSET_FLAG (tree_view, BTK_TREE_VIEW_IS_LIST);
      if (BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_SHOW_EXPANDERS))
	{
	  GList *list;

	  for (list = tree_view->priv->columns; list; list = list->next)
	    if (BTK_TREE_VIEW_COLUMN (list->data)->visible)
	      {
		BTK_TREE_VIEW_COLUMN (list->data)->dirty = TRUE;
		_btk_tree_view_column_cell_set_dirty (BTK_TREE_VIEW_COLUMN (list->data), TRUE);
		break;
	      }
	}
      btk_widget_queue_resize (BTK_WIDGET (tree_view));
    }
  else
    {
      _btk_tree_view_queue_draw_node (tree_view, tree, node, NULL);
    }

 done:
  if (free_path)
    btk_tree_path_free (path);
}

static void
count_children_helper (BtkRBTree *tree,
		       BtkRBNode *node,
		       gpointer   data)
{
  if (node->children)
    _btk_rbtree_traverse (node->children, node->children->root, G_POST_ORDER, count_children_helper, data);
  (*((gint *)data))++;
}

static void
check_selection_helper (BtkRBTree *tree,
                        BtkRBNode *node,
                        gpointer   data)
{
  gint *value = (gint *)data;

  *value = BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SELECTED);

  if (node->children && !*value)
    _btk_rbtree_traverse (node->children, node->children->root, G_POST_ORDER, check_selection_helper, data);
}

static void
btk_tree_view_row_deleted (BtkTreeModel *model,
			   BtkTreePath  *path,
			   gpointer      data)
{
  BtkTreeView *tree_view = (BtkTreeView *)data;
  BtkRBTree *tree;
  BtkRBNode *node;
  GList *list;
  gint selection_changed = FALSE;

  g_return_if_fail (path != NULL);

  btk_tree_row_reference_deleted (B_OBJECT (data), path);

  if (_btk_tree_view_find_node (tree_view, path, &tree, &node))
    return;

  if (tree == NULL)
    return;

  /* check if the selection has been changed */
  _btk_rbtree_traverse (tree, node, G_POST_ORDER,
                        check_selection_helper, &selection_changed);

  for (list = tree_view->priv->columns; list; list = list->next)
    if (((BtkTreeViewColumn *)list->data)->visible &&
	((BtkTreeViewColumn *)list->data)->column_type == BTK_TREE_VIEW_COLUMN_AUTOSIZE)
      _btk_tree_view_column_cell_set_dirty ((BtkTreeViewColumn *)list->data, TRUE);

  /* Ensure we don't have a dangling pointer to a dead node */
  ensure_unprelighted (tree_view);

  /* Cancel editting if we've started */
  btk_tree_view_stop_editing (tree_view, TRUE);

  /* If we have a node expanded/collapsed timeout, remove it */
  remove_expand_collapse_timeout (tree_view);

  if (tree_view->priv->destroy_count_func)
    {
      gint child_count = 0;
      if (node->children)
	_btk_rbtree_traverse (node->children, node->children->root, G_POST_ORDER, count_children_helper, &child_count);
      tree_view->priv->destroy_count_func (tree_view, path, child_count, tree_view->priv->destroy_count_data);
    }

  if (tree->root->count == 1)
    {
      if (tree_view->priv->tree == tree)
	tree_view->priv->tree = NULL;

      _btk_rbtree_remove (tree);
    }
  else
    {
      _btk_rbtree_remove_node (tree, node);
    }

  if (! btk_tree_row_reference_valid (tree_view->priv->top_row))
    {
      btk_tree_row_reference_free (tree_view->priv->top_row);
      tree_view->priv->top_row = NULL;
    }

  install_scroll_sync_handler (tree_view);

  btk_widget_queue_resize (BTK_WIDGET (tree_view));

  if (selection_changed)
    g_signal_emit_by_name (tree_view->priv->selection, "changed");
}

static void
btk_tree_view_rows_reordered (BtkTreeModel *model,
			      BtkTreePath  *parent,
			      BtkTreeIter  *iter,
			      gint         *new_order,
			      gpointer      data)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (data);
  BtkRBTree *tree;
  BtkRBNode *node;
  gint len;

  len = btk_tree_model_iter_n_children (model, iter);

  if (len < 2)
    return;

  btk_tree_row_reference_reordered (B_OBJECT (data),
				    parent,
				    iter,
				    new_order);

  if (_btk_tree_view_find_node (tree_view,
				parent,
				&tree,
				&node))
    return;

  /* We need to special case the parent path */
  if (tree == NULL)
    tree = tree_view->priv->tree;
  else
    tree = node->children;

  if (tree == NULL)
    return;

  if (tree_view->priv->edited_column)
    btk_tree_view_stop_editing (tree_view, TRUE);

  /* we need to be unprelighted */
  ensure_unprelighted (tree_view);

  /* clear the timeout */
  cancel_arrow_animation (tree_view);
  
  _btk_rbtree_reorder (tree, new_order, len);

  btk_widget_queue_draw (BTK_WIDGET (tree_view));

  btk_tree_view_dy_to_top_row (tree_view);
}


/* Internal tree functions
 */


static void
btk_tree_view_get_background_xrange (BtkTreeView       *tree_view,
                                     BtkRBTree         *tree,
                                     BtkTreeViewColumn *column,
                                     gint              *x1,
                                     gint              *x2)
{
  BtkTreeViewColumn *tmp_column = NULL;
  gint total_width;
  GList *list;
  gboolean rtl;

  if (x1)
    *x1 = 0;

  if (x2)
    *x2 = 0;

  rtl = (btk_widget_get_direction (BTK_WIDGET (tree_view)) == BTK_TEXT_DIR_RTL);

  total_width = 0;
  for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
       list;
       list = (rtl ? list->prev : list->next))
    {
      tmp_column = list->data;

      if (tmp_column == column)
        break;

      if (tmp_column->visible)
        total_width += tmp_column->width;
    }

  if (tmp_column != column)
    {
      g_warning (B_STRLOC": passed-in column isn't in the tree");
      return;
    }

  if (x1)
    *x1 = total_width;

  if (x2)
    {
      if (column->visible)
        *x2 = total_width + column->width;
      else
        *x2 = total_width; /* width of 0 */
    }
}
static void
btk_tree_view_get_arrow_xrange (BtkTreeView *tree_view,
				BtkRBTree   *tree,
                                gint        *x1,
                                gint        *x2)
{
  gint x_offset = 0;
  GList *list;
  BtkTreeViewColumn *tmp_column = NULL;
  gint total_width;
  gboolean indent_expanders;
  gboolean rtl;

  rtl = (btk_widget_get_direction (BTK_WIDGET (tree_view)) == BTK_TEXT_DIR_RTL);

  total_width = 0;
  for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
       list;
       list = (rtl ? list->prev : list->next))
    {
      tmp_column = list->data;

      if (btk_tree_view_is_expander_column (tree_view, tmp_column))
        {
	  if (rtl)
	    x_offset = total_width + tmp_column->width - tree_view->priv->expander_size;
	  else
	    x_offset = total_width;
          break;
        }

      if (tmp_column->visible)
        total_width += tmp_column->width;
    }

  btk_widget_style_get (BTK_WIDGET (tree_view),
			"indent-expanders", &indent_expanders,
			NULL);

  if (indent_expanders)
    {
      if (rtl)
	x_offset -= tree_view->priv->expander_size * _btk_rbtree_get_depth (tree);
      else
	x_offset += tree_view->priv->expander_size * _btk_rbtree_get_depth (tree);
    }

  *x1 = x_offset;
  
  if (tmp_column && tmp_column->visible)
    /* +1 because x2 isn't included in the range. */
    *x2 = *x1 + tree_view->priv->expander_size + 1;
  else
    *x2 = *x1;
}

static void
btk_tree_view_build_tree (BtkTreeView *tree_view,
			  BtkRBTree   *tree,
			  BtkTreeIter *iter,
			  gint         depth,
			  gboolean     recurse)
{
  BtkRBNode *temp = NULL;
  BtkTreePath *path = NULL;
  gboolean is_list = BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_IS_LIST);

  do
    {
      btk_tree_model_ref_node (tree_view->priv->model, iter);
      temp = _btk_rbtree_insert_after (tree, temp, 0, FALSE);

      if (tree_view->priv->fixed_height > 0)
        {
          if (BTK_RBNODE_FLAG_SET (temp, BTK_RBNODE_INVALID))
	    {
              _btk_rbtree_node_set_height (tree, temp, tree_view->priv->fixed_height);
	      _btk_rbtree_node_mark_valid (tree, temp);
	    }
        }

      if (is_list)
        continue;

      if (recurse)
	{
	  BtkTreeIter child;

	  if (!path)
	    path = btk_tree_model_get_path (tree_view->priv->model, iter);
	  else
	    btk_tree_path_next (path);

	  if (btk_tree_model_iter_children (tree_view->priv->model, &child, iter))
	    {
	      gboolean expand;

	      g_signal_emit (tree_view, tree_view_signals[TEST_EXPAND_ROW], 0, iter, path, &expand);

	      if (btk_tree_model_iter_has_child (tree_view->priv->model, iter)
		  && !expand)
	        {
	          temp->children = _btk_rbtree_new ();
	          temp->children->parent_tree = tree;
	          temp->children->parent_node = temp;
	          btk_tree_view_build_tree (tree_view, temp->children, &child, depth + 1, recurse);
		}
	    }
	}

      if (btk_tree_model_iter_has_child (tree_view->priv->model, iter))
	{
	  if ((temp->flags&BTK_RBNODE_IS_PARENT) != BTK_RBNODE_IS_PARENT)
	    temp->flags ^= BTK_RBNODE_IS_PARENT;
	}
    }
  while (btk_tree_model_iter_next (tree_view->priv->model, iter));

  if (path)
    btk_tree_path_free (path);
}

/* Make sure the node is visible vertically */
static void
btk_tree_view_clamp_node_visible (BtkTreeView *tree_view,
				  BtkRBTree   *tree,
				  BtkRBNode   *node)
{
  gint node_dy, height;
  BtkTreePath *path = NULL;

  if (!btk_widget_get_realized (BTK_WIDGET (tree_view)))
    return;

  /* just return if the node is visible, avoiding a costly expose */
  node_dy = _btk_rbtree_node_find_offset (tree, node);
  height = ROW_HEIGHT (tree_view, BTK_RBNODE_GET_HEIGHT (node));
  if (! BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_INVALID)
      && node_dy >= tree_view->priv->vadjustment->value
      && node_dy + height <= (tree_view->priv->vadjustment->value
                              + tree_view->priv->vadjustment->page_size))
    return;

  path = _btk_tree_view_find_path (tree_view, tree, node);
  if (path)
    {
      /* We process updates because we want to clear old selected items when we scroll.
       * if this is removed, we get a "selection streak" at the bottom. */
      bdk_window_process_updates (tree_view->priv->bin_window, TRUE);
      btk_tree_view_scroll_to_cell (tree_view, path, NULL, FALSE, 0.0, 0.0);
      btk_tree_path_free (path);
    }
}

static void
btk_tree_view_clamp_column_visible (BtkTreeView       *tree_view,
				    BtkTreeViewColumn *column,
				    gboolean           focus_to_cell)
{
  gint x, width;

  if (column == NULL)
    return;

  x = column->button->allocation.x;
  width = column->button->allocation.width;

  if (width > tree_view->priv->hadjustment->page_size)
    {
      /* The column is larger than the horizontal page size.  If the
       * column has cells which can be focussed individually, then we make
       * sure the cell which gets focus is fully visible (if even the
       * focus cell is bigger than the page size, we make sure the
       * left-hand side of the cell is visible).
       *
       * If the column does not have those so-called special cells, we
       * make sure the left-hand side of the column is visible.
       */

      if (focus_to_cell && btk_tree_view_has_special_cell (tree_view))
        {
	  BtkTreePath *cursor_path;
	  BdkRectangle background_area, cell_area, focus_area;

	  cursor_path = btk_tree_row_reference_get_path (tree_view->priv->cursor);

	  btk_tree_view_get_cell_area (tree_view,
				       cursor_path, column, &cell_area);
	  btk_tree_view_get_background_area (tree_view,
					     cursor_path, column,
					     &background_area);

	  btk_tree_path_free (cursor_path);

	  _btk_tree_view_column_get_focus_area (column,
						&background_area,
						&cell_area,
						&focus_area);

	  x = focus_area.x;
	  width = focus_area.width;

	  if (width < tree_view->priv->hadjustment->page_size)
	    {
	      if ((tree_view->priv->hadjustment->value + tree_view->priv->hadjustment->page_size) < (x + width))
		btk_adjustment_set_value (tree_view->priv->hadjustment,
					  x + width - tree_view->priv->hadjustment->page_size);
	      else if (tree_view->priv->hadjustment->value > x)
		btk_adjustment_set_value (tree_view->priv->hadjustment, x);
	    }
	}

      btk_adjustment_set_value (tree_view->priv->hadjustment,
				CLAMP (x,
				       tree_view->priv->hadjustment->lower,
				       tree_view->priv->hadjustment->upper
				       - tree_view->priv->hadjustment->page_size));
    }
  else
    {
      if ((tree_view->priv->hadjustment->value + tree_view->priv->hadjustment->page_size) < (x + width))
	  btk_adjustment_set_value (tree_view->priv->hadjustment,
				    x + width - tree_view->priv->hadjustment->page_size);
      else if (tree_view->priv->hadjustment->value > x)
	btk_adjustment_set_value (tree_view->priv->hadjustment, x);
  }
}

/* This function could be more efficient.  I'll optimize it if profiling seems
 * to imply that it is important */
BtkTreePath *
_btk_tree_view_find_path (BtkTreeView *tree_view,
			  BtkRBTree   *tree,
			  BtkRBNode   *node)
{
  BtkTreePath *path;
  BtkRBTree *tmp_tree;
  BtkRBNode *tmp_node, *last;
  gint count;

  path = btk_tree_path_new ();

  g_return_val_if_fail (node != NULL, path);
  g_return_val_if_fail (node != tree->nil, path);

  count = 1 + node->left->count;

  last = node;
  tmp_node = node->parent;
  tmp_tree = tree;
  while (tmp_tree)
    {
      while (tmp_node != tmp_tree->nil)
	{
	  if (tmp_node->right == last)
	    count += 1 + tmp_node->left->count;
	  last = tmp_node;
	  tmp_node = tmp_node->parent;
	}
      btk_tree_path_prepend_index (path, count - 1);
      last = tmp_tree->parent_node;
      tmp_tree = tmp_tree->parent_tree;
      if (last)
	{
	  count = 1 + last->left->count;
	  tmp_node = last->parent;
	}
    }
  return path;
}

/* Returns TRUE if we ran out of tree before finding the path.  If the path is
 * invalid (ie. points to a node that's not in the tree), *tree and *node are
 * both set to NULL.
 */
gboolean
_btk_tree_view_find_node (BtkTreeView  *tree_view,
			  BtkTreePath  *path,
			  BtkRBTree   **tree,
			  BtkRBNode   **node)
{
  BtkRBNode *tmpnode = NULL;
  BtkRBTree *tmptree = tree_view->priv->tree;
  gint *indices = btk_tree_path_get_indices (path);
  gint depth = btk_tree_path_get_depth (path);
  gint i = 0;

  *node = NULL;
  *tree = NULL;

  if (depth == 0 || tmptree == NULL)
    return FALSE;
  do
    {
      tmpnode = _btk_rbtree_find_count (tmptree, indices[i] + 1);
      ++i;
      if (tmpnode == NULL)
	{
	  *tree = NULL;
	  *node = NULL;
	  return FALSE;
	}
      if (i >= depth)
	{
	  *tree = tmptree;
	  *node = tmpnode;
	  return FALSE;
	}
      *tree = tmptree;
      *node = tmpnode;
      tmptree = tmpnode->children;
      if (tmptree == NULL)
	return TRUE;
    }
  while (1);
}

static gboolean
btk_tree_view_is_expander_column (BtkTreeView       *tree_view,
				  BtkTreeViewColumn *column)
{
  GList *list;

  if (BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_IS_LIST))
    return FALSE;

  if (tree_view->priv->expander_column != NULL)
    {
      if (tree_view->priv->expander_column == column)
	return TRUE;
      return FALSE;
    }
  else
    {
      for (list = tree_view->priv->columns;
	   list;
	   list = list->next)
	if (((BtkTreeViewColumn *)list->data)->visible)
	  break;
      if (list && list->data == column)
	return TRUE;
    }
  return FALSE;
}

static void
btk_tree_view_add_move_binding (BtkBindingSet  *binding_set,
				guint           keyval,
				guint           modmask,
				gboolean        add_shifted_binding,
				BtkMovementStep step,
				gint            count)
{
  
  btk_binding_entry_add_signal (binding_set, keyval, modmask,
                                "move-cursor", 2,
                                B_TYPE_ENUM, step,
                                B_TYPE_INT, count);

  if (add_shifted_binding)
    btk_binding_entry_add_signal (binding_set, keyval, BDK_SHIFT_MASK,
				  "move-cursor", 2,
				  B_TYPE_ENUM, step,
				  B_TYPE_INT, count);

  if ((modmask & BDK_CONTROL_MASK) == BDK_CONTROL_MASK)
   return;

  btk_binding_entry_add_signal (binding_set, keyval, BDK_CONTROL_MASK | BDK_SHIFT_MASK,
                                "move-cursor", 2,
                                B_TYPE_ENUM, step,
                                B_TYPE_INT, count);

  btk_binding_entry_add_signal (binding_set, keyval, BDK_CONTROL_MASK,
                                "move-cursor", 2,
                                B_TYPE_ENUM, step,
                                B_TYPE_INT, count);
}

static gint
btk_tree_view_unref_tree_helper (BtkTreeModel *model,
				 BtkTreeIter  *iter,
				 BtkRBTree    *tree,
				 BtkRBNode    *node)
{
  gint retval = FALSE;
  do
    {
      g_return_val_if_fail (node != NULL, FALSE);

      if (node->children)
	{
	  BtkTreeIter child;
	  BtkRBTree *new_tree;
	  BtkRBNode *new_node;

	  new_tree = node->children;
	  new_node = new_tree->root;

	  while (new_node && new_node->left != new_tree->nil)
	    new_node = new_node->left;

	  if (!btk_tree_model_iter_children (model, &child, iter))
	    return FALSE;

	  retval = retval || btk_tree_view_unref_tree_helper (model, &child, new_tree, new_node);
	}

      if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SELECTED))
	retval = TRUE;
      btk_tree_model_unref_node (model, iter);
      node = _btk_rbtree_next (tree, node);
    }
  while (btk_tree_model_iter_next (model, iter));

  return retval;
}

static gint
btk_tree_view_unref_and_check_selection_tree (BtkTreeView *tree_view,
					      BtkRBTree   *tree)
{
  BtkTreeIter iter;
  BtkTreePath *path;
  BtkRBNode *node;
  gint retval;

  if (!tree)
    return FALSE;

  node = tree->root;
  while (node && node->left != tree->nil)
    node = node->left;

  g_return_val_if_fail (node != NULL, FALSE);
  path = _btk_tree_view_find_path (tree_view, tree, node);
  btk_tree_model_get_iter (BTK_TREE_MODEL (tree_view->priv->model),
			   &iter, path);
  retval = btk_tree_view_unref_tree_helper (BTK_TREE_MODEL (tree_view->priv->model), &iter, tree, node);
  btk_tree_path_free (path);

  return retval;
}

static void
btk_tree_view_set_column_drag_info (BtkTreeView       *tree_view,
				    BtkTreeViewColumn *column)
{
  BtkTreeViewColumn *left_column;
  BtkTreeViewColumn *cur_column = NULL;
  BtkTreeViewColumnReorder *reorder;
  gboolean rtl;
  GList *tmp_list;
  gint left;

  /* We want to precalculate the motion list such that we know what column slots
   * are available.
   */
  left_column = NULL;
  rtl = (btk_widget_get_direction (BTK_WIDGET (tree_view)) == BTK_TEXT_DIR_RTL);

  /* First, identify all possible drop spots */
  if (rtl)
    tmp_list = g_list_last (tree_view->priv->columns);
  else
    tmp_list = g_list_first (tree_view->priv->columns);

  while (tmp_list)
    {
      cur_column = BTK_TREE_VIEW_COLUMN (tmp_list->data);
      tmp_list = rtl?g_list_previous (tmp_list):g_list_next (tmp_list);

      if (cur_column->visible == FALSE)
	continue;

      /* If it's not the column moving and func tells us to skip over the column, we continue. */
      if (left_column != column && cur_column != column &&
	  tree_view->priv->column_drop_func &&
	  ! tree_view->priv->column_drop_func (tree_view, column, left_column, cur_column, tree_view->priv->column_drop_func_data))
	{
	  left_column = cur_column;
	  continue;
	}
      reorder = g_slice_new0 (BtkTreeViewColumnReorder);
      reorder->left_column = left_column;
      left_column = reorder->right_column = cur_column;

      tree_view->priv->column_drag_info = g_list_append (tree_view->priv->column_drag_info, reorder);
    }

  /* Add the last one */
  if (tree_view->priv->column_drop_func == NULL ||
      ((left_column != column) &&
       tree_view->priv->column_drop_func (tree_view, column, left_column, NULL, tree_view->priv->column_drop_func_data)))
    {
      reorder = g_slice_new0 (BtkTreeViewColumnReorder);
      reorder->left_column = left_column;
      reorder->right_column = NULL;
      tree_view->priv->column_drag_info = g_list_append (tree_view->priv->column_drag_info, reorder);
    }

  /* We quickly check to see if it even makes sense to reorder columns. */
  /* If there is nothing that can be moved, then we return */

  if (tree_view->priv->column_drag_info == NULL)
    return;

  /* We know there are always 2 slots possbile, as you can always return column. */
  /* If that's all there is, return */
  if (tree_view->priv->column_drag_info->next == NULL || 
      (tree_view->priv->column_drag_info->next->next == NULL &&
       ((BtkTreeViewColumnReorder *)tree_view->priv->column_drag_info->data)->right_column == column &&
       ((BtkTreeViewColumnReorder *)tree_view->priv->column_drag_info->next->data)->left_column == column))
    {
      for (tmp_list = tree_view->priv->column_drag_info; tmp_list; tmp_list = tmp_list->next)
	g_slice_free (BtkTreeViewColumnReorder, tmp_list->data);
      g_list_free (tree_view->priv->column_drag_info);
      tree_view->priv->column_drag_info = NULL;
      return;
    }
  /* We fill in the ranges for the columns, now that we've isolated them */
  left = - TREE_VIEW_COLUMN_DRAG_DEAD_MULTIPLIER (tree_view);

  for (tmp_list = tree_view->priv->column_drag_info; tmp_list; tmp_list = tmp_list->next)
    {
      reorder = (BtkTreeViewColumnReorder *) tmp_list->data;

      reorder->left_align = left;
      if (tmp_list->next != NULL)
	{
	  g_assert (tmp_list->next->data);
	  left = reorder->right_align = (reorder->right_column->button->allocation.x +
					 reorder->right_column->button->allocation.width +
					 ((BtkTreeViewColumnReorder *)tmp_list->next->data)->left_column->button->allocation.x)/2;
	}
      else
	{
	  gint width;

          width = bdk_window_get_width (tree_view->priv->header_window);
	  reorder->right_align = width + TREE_VIEW_COLUMN_DRAG_DEAD_MULTIPLIER (tree_view);
	}
    }
}

void
_btk_tree_view_column_start_drag (BtkTreeView       *tree_view,
				  BtkTreeViewColumn *column)
{
  BdkEvent *send_event;
  BtkAllocation allocation;
  gint x, y, width, height;
  BdkScreen *screen = btk_widget_get_screen (BTK_WIDGET (tree_view));
  BdkDisplay *display = bdk_screen_get_display (screen);

  g_return_if_fail (tree_view->priv->column_drag_info == NULL);
  g_return_if_fail (tree_view->priv->cur_reorder == NULL);

  btk_tree_view_set_column_drag_info (tree_view, column);

  if (tree_view->priv->column_drag_info == NULL)
    return;

  if (tree_view->priv->drag_window == NULL)
    {
      BdkWindowAttr attributes;
      guint attributes_mask;

      attributes.window_type = BDK_WINDOW_CHILD;
      attributes.wclass = BDK_INPUT_OUTPUT;
      attributes.x = column->button->allocation.x;
      attributes.y = 0;
      attributes.width = column->button->allocation.width;
      attributes.height = column->button->allocation.height;
      attributes.visual = btk_widget_get_visual (BTK_WIDGET (tree_view));
      attributes.colormap = btk_widget_get_colormap (BTK_WIDGET (tree_view));
      attributes.event_mask = BDK_VISIBILITY_NOTIFY_MASK | BDK_EXPOSURE_MASK | BDK_POINTER_MOTION_MASK;
      attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;

      tree_view->priv->drag_window = bdk_window_new (tree_view->priv->bin_window,
						     &attributes,
						     attributes_mask);
      bdk_window_set_user_data (tree_view->priv->drag_window, BTK_WIDGET (tree_view));
    }

  bdk_display_pointer_ungrab (display, BDK_CURRENT_TIME);
  bdk_display_keyboard_ungrab (display, BDK_CURRENT_TIME);

  btk_grab_remove (column->button);

  send_event = bdk_event_new (BDK_LEAVE_NOTIFY);
  send_event->crossing.send_event = TRUE;
  send_event->crossing.window = g_object_ref (BTK_BUTTON (column->button)->event_window);
  send_event->crossing.subwindow = NULL;
  send_event->crossing.detail = BDK_NOTIFY_ANCESTOR;
  send_event->crossing.time = BDK_CURRENT_TIME;

  btk_propagate_event (column->button, send_event);
  bdk_event_free (send_event);

  send_event = bdk_event_new (BDK_BUTTON_RELEASE);
  send_event->button.window = g_object_ref (bdk_screen_get_root_window (screen));
  send_event->button.send_event = TRUE;
  send_event->button.time = BDK_CURRENT_TIME;
  send_event->button.x = -1;
  send_event->button.y = -1;
  send_event->button.axes = NULL;
  send_event->button.state = 0;
  send_event->button.button = 1;
  send_event->button.device = bdk_display_get_core_pointer (display);
  send_event->button.x_root = 0;
  send_event->button.y_root = 0;

  btk_propagate_event (column->button, send_event);
  bdk_event_free (send_event);

  /* Kids, don't try this at home */
  g_object_ref (column->button);
  btk_container_remove (BTK_CONTAINER (tree_view), column->button);
  btk_widget_set_parent_window (column->button, tree_view->priv->drag_window);
  btk_widget_set_parent (column->button, BTK_WIDGET (tree_view));
  g_object_unref (column->button);

  tree_view->priv->drag_column_x = column->button->allocation.x;
  allocation = column->button->allocation;
  allocation.x = 0;
  btk_widget_size_allocate (column->button, &allocation);
  btk_widget_set_parent_window (column->button, tree_view->priv->drag_window);

  tree_view->priv->drag_column = column;
  bdk_window_show (tree_view->priv->drag_window);

  bdk_window_get_origin (tree_view->priv->header_window, &x, &y);
  width = bdk_window_get_width (tree_view->priv->header_window);
  height = bdk_window_get_height (tree_view->priv->header_window);

  btk_widget_grab_focus (BTK_WIDGET (tree_view));
  while (btk_events_pending ())
    btk_main_iteration ();

  BTK_TREE_VIEW_SET_FLAG (tree_view, BTK_TREE_VIEW_IN_COLUMN_DRAG);
  bdk_pointer_grab (tree_view->priv->drag_window,
		    FALSE,
		    BDK_POINTER_MOTION_MASK|BDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, BDK_CURRENT_TIME);
  bdk_keyboard_grab (tree_view->priv->drag_window,
		     FALSE,
		     BDK_CURRENT_TIME);
}

static void
btk_tree_view_queue_draw_arrow (BtkTreeView        *tree_view,
				BtkRBTree          *tree,
				BtkRBNode          *node,
				const BdkRectangle *clip_rect)
{
  BdkRectangle rect;

  if (!btk_widget_get_realized (BTK_WIDGET (tree_view)))
    return;

  rect.x = 0;
  rect.width = MAX (tree_view->priv->expander_size, MAX (tree_view->priv->width, BTK_WIDGET (tree_view)->allocation.width));

  rect.y = BACKGROUND_FIRST_PIXEL (tree_view, tree, node);
  rect.height = ROW_HEIGHT (tree_view, BACKGROUND_HEIGHT (node));

  if (clip_rect)
    {
      BdkRectangle new_rect;

      bdk_rectangle_intersect (clip_rect, &rect, &new_rect);

      bdk_window_invalidate_rect (tree_view->priv->bin_window, &new_rect, TRUE);
    }
  else
    {
      bdk_window_invalidate_rect (tree_view->priv->bin_window, &rect, TRUE);
    }
}

void
_btk_tree_view_queue_draw_node (BtkTreeView        *tree_view,
				BtkRBTree          *tree,
				BtkRBNode          *node,
				const BdkRectangle *clip_rect)
{
  BdkRectangle rect;

  if (!btk_widget_get_realized (BTK_WIDGET (tree_view)))
    return;

  rect.x = 0;
  rect.width = MAX (tree_view->priv->width, BTK_WIDGET (tree_view)->allocation.width);

  rect.y = BACKGROUND_FIRST_PIXEL (tree_view, tree, node);
  rect.height = ROW_HEIGHT (tree_view, BACKGROUND_HEIGHT (node));

  if (clip_rect)
    {
      BdkRectangle new_rect;

      bdk_rectangle_intersect (clip_rect, &rect, &new_rect);

      bdk_window_invalidate_rect (tree_view->priv->bin_window, &new_rect, TRUE);
    }
  else
    {
      bdk_window_invalidate_rect (tree_view->priv->bin_window, &rect, TRUE);
    }
}

static void
btk_tree_view_queue_draw_path (BtkTreeView        *tree_view,
                               BtkTreePath        *path,
                               const BdkRectangle *clip_rect)
{
  BtkRBTree *tree = NULL;
  BtkRBNode *node = NULL;

  _btk_tree_view_find_node (tree_view, path, &tree, &node);

  if (tree)
    _btk_tree_view_queue_draw_node (tree_view, tree, node, clip_rect);
}

/* x and y are the mouse position
 */
static void
btk_tree_view_draw_arrow (BtkTreeView *tree_view,
                          BtkRBTree   *tree,
			  BtkRBNode   *node,
			  /* in bin_window coordinates */
			  gint         x,
			  gint         y)
{
  BdkRectangle area;
  BtkStateType state;
  BtkWidget *widget;
  gint x_offset = 0;
  gint x2;
  gint vertical_separator;
  gint expander_size;
  BtkExpanderStyle expander_style;

  widget = BTK_WIDGET (tree_view);

  btk_widget_style_get (widget,
			"vertical-separator", &vertical_separator,
			NULL);
  expander_size = tree_view->priv->expander_size - EXPANDER_EXTRA_PADDING;

  if (! BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_PARENT))
    return;

  btk_tree_view_get_arrow_xrange (tree_view, tree, &x_offset, &x2);

  area.x = x_offset;
  area.y = CELL_FIRST_PIXEL (tree_view, tree, node, vertical_separator);
  area.width = expander_size + 2;
  area.height = MAX (CELL_HEIGHT (node, vertical_separator), (expander_size - vertical_separator));

  if (btk_widget_get_state (widget) == BTK_STATE_INSENSITIVE)
    {
      state = BTK_STATE_INSENSITIVE;
    }
  else if (node == tree_view->priv->button_pressed_node)
    {
      if (x >= area.x && x <= (area.x + area.width) &&
	  y >= area.y && y <= (area.y + area.height))
        state = BTK_STATE_ACTIVE;
      else
        state = BTK_STATE_NORMAL;
    }
  else
    {
      if (node == tree_view->priv->prelight_node &&
	  BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_ARROW_PRELIT))
	state = BTK_STATE_PRELIGHT;
      else
	state = BTK_STATE_NORMAL;
    }

  if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SEMI_EXPANDED))
    expander_style = BTK_EXPANDER_SEMI_EXPANDED;
  else if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SEMI_COLLAPSED))
    expander_style = BTK_EXPANDER_SEMI_COLLAPSED;
  else if (node->children != NULL)
    expander_style = BTK_EXPANDER_EXPANDED;
  else
    expander_style = BTK_EXPANDER_COLLAPSED;

  btk_paint_expander (widget->style,
                      tree_view->priv->bin_window,
                      state,
                      &area,
                      widget,
                      "treeview",
		      area.x + area.width / 2,
		      area.y + area.height / 2,
		      expander_style);
}

static void
btk_tree_view_focus_to_cursor (BtkTreeView *tree_view)

{
  BtkTreePath *cursor_path;

  if ((tree_view->priv->tree == NULL) ||
      (! btk_widget_get_realized (BTK_WIDGET (tree_view))))
    return;

  cursor_path = NULL;
  if (tree_view->priv->cursor)
    cursor_path = btk_tree_row_reference_get_path (tree_view->priv->cursor);

  if (cursor_path == NULL)
    {
      /* Consult the selection before defaulting to the
       * first focusable element
       */
      GList *selected_rows;
      BtkTreeModel *model;
      BtkTreeSelection *selection;

      selection = btk_tree_view_get_selection (tree_view);
      selected_rows = btk_tree_selection_get_selected_rows (selection, &model);

      if (selected_rows)
	{
          cursor_path = btk_tree_path_copy((const BtkTreePath *)(selected_rows->data));
	  g_list_foreach (selected_rows, (GFunc)btk_tree_path_free, NULL);
	  g_list_free (selected_rows);
        }
      else
	{
	  cursor_path = btk_tree_path_new_first ();
	  search_first_focusable_path (tree_view, &cursor_path,
				       TRUE, NULL, NULL);
	}

      btk_tree_row_reference_free (tree_view->priv->cursor);
      tree_view->priv->cursor = NULL;

      if (cursor_path)
	{
	  if (tree_view->priv->selection->type == BTK_SELECTION_MULTIPLE)
	    btk_tree_view_real_set_cursor (tree_view, cursor_path, FALSE, FALSE);
	  else
	    btk_tree_view_real_set_cursor (tree_view, cursor_path, TRUE, FALSE);
	}
    }

  if (cursor_path)
    {
      BTK_TREE_VIEW_SET_FLAG (tree_view, BTK_TREE_VIEW_DRAW_KEYFOCUS);

      btk_tree_view_queue_draw_path (tree_view, cursor_path, NULL);
      btk_tree_path_free (cursor_path);

      if (tree_view->priv->focus_column == NULL)
	{
	  GList *list;
	  for (list = tree_view->priv->columns; list; list = list->next)
	    {
	      if (BTK_TREE_VIEW_COLUMN (list->data)->visible)
		{
		  tree_view->priv->focus_column = BTK_TREE_VIEW_COLUMN (list->data);
		  break;
		}
	    }
	}
    }
}

static void
btk_tree_view_move_cursor_up_down (BtkTreeView *tree_view,
				   gint         count)
{
  gint selection_count;
  BtkRBTree *cursor_tree = NULL;
  BtkRBNode *cursor_node = NULL;
  BtkRBTree *new_cursor_tree = NULL;
  BtkRBNode *new_cursor_node = NULL;
  BtkTreePath *cursor_path = NULL;
  gboolean grab_focus = TRUE;
  gboolean selectable;

  if (! btk_widget_has_focus (BTK_WIDGET (tree_view)))
    return;

  cursor_path = NULL;
  if (!btk_tree_row_reference_valid (tree_view->priv->cursor))
    /* FIXME: we lost the cursor; should we get the first? */
    return;

  cursor_path = btk_tree_row_reference_get_path (tree_view->priv->cursor);
  _btk_tree_view_find_node (tree_view, cursor_path,
			    &cursor_tree, &cursor_node);

  if (cursor_tree == NULL)
    /* FIXME: we lost the cursor; should we get the first? */
    return;

  selection_count = btk_tree_selection_count_selected_rows (tree_view->priv->selection);
  selectable = _btk_tree_selection_row_is_selectable (tree_view->priv->selection,
						      cursor_node,
						      cursor_path);

  if (selection_count == 0
      && tree_view->priv->selection->type != BTK_SELECTION_NONE
      && !tree_view->priv->modify_selection_pressed
      && selectable)
    {
      /* Don't move the cursor, but just select the current node */
      new_cursor_tree = cursor_tree;
      new_cursor_node = cursor_node;
    }
  else
    {
      if (count == -1)
	_btk_rbtree_prev_full (cursor_tree, cursor_node,
			       &new_cursor_tree, &new_cursor_node);
      else
	_btk_rbtree_next_full (cursor_tree, cursor_node,
			       &new_cursor_tree, &new_cursor_node);
    }

  btk_tree_path_free (cursor_path);

  if (new_cursor_node)
    {
      cursor_path = _btk_tree_view_find_path (tree_view,
					      new_cursor_tree, new_cursor_node);

      search_first_focusable_path (tree_view, &cursor_path,
				   (count != -1),
				   &new_cursor_tree,
				   &new_cursor_node);

      if (cursor_path)
	btk_tree_path_free (cursor_path);
    }

  /*
   * If the list has only one item and multi-selection is set then select
   * the row (if not yet selected).
   */
  if (tree_view->priv->selection->type == BTK_SELECTION_MULTIPLE &&
      new_cursor_node == NULL)
    {
      if (count == -1)
        _btk_rbtree_next_full (cursor_tree, cursor_node,
    			       &new_cursor_tree, &new_cursor_node);
      else
        _btk_rbtree_prev_full (cursor_tree, cursor_node,
			       &new_cursor_tree, &new_cursor_node);

      if (new_cursor_node == NULL
	  && !BTK_RBNODE_FLAG_SET (cursor_node, BTK_RBNODE_IS_SELECTED))
        {
          new_cursor_node = cursor_node;
          new_cursor_tree = cursor_tree;
        }
      else
        {
          new_cursor_node = NULL;
        }
    }

  if (new_cursor_node)
    {
      cursor_path = _btk_tree_view_find_path (tree_view, new_cursor_tree, new_cursor_node);
      btk_tree_view_real_set_cursor (tree_view, cursor_path, TRUE, TRUE);
      btk_tree_path_free (cursor_path);
    }
  else
    {
      btk_tree_view_clamp_node_visible (tree_view, cursor_tree, cursor_node);

      if (!tree_view->priv->extend_selection_pressed)
        {
          if (! btk_widget_keynav_failed (BTK_WIDGET (tree_view),
                                          count < 0 ?
                                          BTK_DIR_UP : BTK_DIR_DOWN))
            {
              BtkWidget *toplevel = btk_widget_get_toplevel (BTK_WIDGET (tree_view));

              if (toplevel)
                btk_widget_child_focus (toplevel,
                                        count < 0 ?
                                        BTK_DIR_TAB_BACKWARD :
                                        BTK_DIR_TAB_FORWARD);

              grab_focus = FALSE;
            }
        }
      else
        {
          btk_widget_error_bell (BTK_WIDGET (tree_view));
        }
    }

  if (grab_focus)
    btk_widget_grab_focus (BTK_WIDGET (tree_view));
}

static void
btk_tree_view_move_cursor_page_up_down (BtkTreeView *tree_view,
					gint         count)
{
  BtkRBTree *cursor_tree = NULL;
  BtkRBNode *cursor_node = NULL;
  BtkTreePath *old_cursor_path = NULL;
  BtkTreePath *cursor_path = NULL;
  BtkRBTree *start_cursor_tree = NULL;
  BtkRBNode *start_cursor_node = NULL;
  gint y;
  gint window_y;
  gint vertical_separator;

  if (!btk_widget_has_focus (BTK_WIDGET (tree_view)))
    return;

  if (btk_tree_row_reference_valid (tree_view->priv->cursor))
    old_cursor_path = btk_tree_row_reference_get_path (tree_view->priv->cursor);
  else
    /* This is sorta weird.  Focus in should give us a cursor */
    return;

  btk_widget_style_get (BTK_WIDGET (tree_view), "vertical-separator", &vertical_separator, NULL);
  _btk_tree_view_find_node (tree_view, old_cursor_path,
			    &cursor_tree, &cursor_node);

  if (cursor_tree == NULL)
    {
      /* FIXME: we lost the cursor.  Should we try to get one? */
      btk_tree_path_free (old_cursor_path);
      return;
    }
  g_return_if_fail (cursor_node != NULL);

  y = _btk_rbtree_node_find_offset (cursor_tree, cursor_node);
  window_y = RBTREE_Y_TO_TREE_WINDOW_Y (tree_view, y);
  y += tree_view->priv->cursor_offset;
  y += count * (int)tree_view->priv->vadjustment->page_increment;
  y = CLAMP (y, (gint)tree_view->priv->vadjustment->lower,  (gint)tree_view->priv->vadjustment->upper - vertical_separator);

  if (y >= tree_view->priv->height)
    y = tree_view->priv->height - 1;

  tree_view->priv->cursor_offset =
    _btk_rbtree_find_offset (tree_view->priv->tree, y,
			     &cursor_tree, &cursor_node);

  if (cursor_tree == NULL)
    {
      /* FIXME: we lost the cursor.  Should we try to get one? */
      btk_tree_path_free (old_cursor_path);
      return;
    }

  if (tree_view->priv->cursor_offset > BACKGROUND_HEIGHT (cursor_node))
    {
      _btk_rbtree_next_full (cursor_tree, cursor_node,
			     &cursor_tree, &cursor_node);
      tree_view->priv->cursor_offset -= BACKGROUND_HEIGHT (cursor_node);
    }

  y -= tree_view->priv->cursor_offset;
  cursor_path = _btk_tree_view_find_path (tree_view, cursor_tree, cursor_node);

  start_cursor_tree = cursor_tree;
  start_cursor_node = cursor_node;

  if (! search_first_focusable_path (tree_view, &cursor_path,
				     (count != -1),
				     &cursor_tree, &cursor_node))
    {
      /* It looks like we reached the end of the view without finding
       * a focusable row.  We will step backwards to find the last
       * focusable row.
       */
      cursor_tree = start_cursor_tree;
      cursor_node = start_cursor_node;
      cursor_path = _btk_tree_view_find_path (tree_view, cursor_tree, cursor_node);

      search_first_focusable_path (tree_view, &cursor_path,
				   (count == -1),
				   &cursor_tree, &cursor_node);
    }

  if (!cursor_path)
    goto cleanup;

  /* update y */
  y = _btk_rbtree_node_find_offset (cursor_tree, cursor_node);

  btk_tree_view_real_set_cursor (tree_view, cursor_path, TRUE, FALSE);

  y -= window_y;
  btk_tree_view_scroll_to_point (tree_view, -1, y);
  btk_tree_view_clamp_node_visible (tree_view, cursor_tree, cursor_node);
  _btk_tree_view_queue_draw_node (tree_view, cursor_tree, cursor_node, NULL);

  if (!btk_tree_path_compare (old_cursor_path, cursor_path))
    btk_widget_error_bell (BTK_WIDGET (tree_view));

  btk_widget_grab_focus (BTK_WIDGET (tree_view));

cleanup:
  btk_tree_path_free (old_cursor_path);
  btk_tree_path_free (cursor_path);
}

static void
btk_tree_view_move_cursor_left_right (BtkTreeView *tree_view,
				      gint         count)
{
  BtkRBTree *cursor_tree = NULL;
  BtkRBNode *cursor_node = NULL;
  BtkTreePath *cursor_path = NULL;
  BtkTreeViewColumn *column;
  BtkTreeIter iter;
  GList *list;
  gboolean found_column = FALSE;
  gboolean rtl;

  rtl = (btk_widget_get_direction (BTK_WIDGET (tree_view)) == BTK_TEXT_DIR_RTL);

  if (!btk_widget_has_focus (BTK_WIDGET (tree_view)))
    return;

  if (btk_tree_row_reference_valid (tree_view->priv->cursor))
    cursor_path = btk_tree_row_reference_get_path (tree_view->priv->cursor);
  else
    return;

  _btk_tree_view_find_node (tree_view, cursor_path, &cursor_tree, &cursor_node);
  if (cursor_tree == NULL)
    return;
  if (btk_tree_model_get_iter (tree_view->priv->model, &iter, cursor_path) == FALSE)
    {
      btk_tree_path_free (cursor_path);
      return;
    }
  btk_tree_path_free (cursor_path);

  list = rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns);
  if (tree_view->priv->focus_column)
    {
      for (; list; list = (rtl ? list->prev : list->next))
	{
	  if (list->data == tree_view->priv->focus_column)
	    break;
	}
    }

  while (list)
    {
      gboolean left, right;

      column = list->data;
      if (column->visible == FALSE)
	goto loop_end;

      btk_tree_view_column_cell_set_cell_data (column,
					       tree_view->priv->model,
					       &iter,
					       BTK_RBNODE_FLAG_SET (cursor_node, BTK_RBNODE_IS_PARENT),
					       cursor_node->children?TRUE:FALSE);

      if (rtl)
        {
	  right = list->prev ? TRUE : FALSE;
	  left = list->next ? TRUE : FALSE;
	}
      else
        {
	  left = list->prev ? TRUE : FALSE;
	  right = list->next ? TRUE : FALSE;
        }

      if (_btk_tree_view_column_cell_focus (column, count, left, right))
	{
	  tree_view->priv->focus_column = column;
	  found_column = TRUE;
	  break;
	}
    loop_end:
      if (count == 1)
	list = rtl ? list->prev : list->next;
      else
	list = rtl ? list->next : list->prev;
    }

  if (found_column)
    {
      if (!btk_tree_view_has_special_cell (tree_view))
	_btk_tree_view_queue_draw_node (tree_view,
				        cursor_tree,
				        cursor_node,
				        NULL);
      g_signal_emit (tree_view, tree_view_signals[CURSOR_CHANGED], 0);
      btk_widget_grab_focus (BTK_WIDGET (tree_view));
    }
  else
    {
      btk_widget_error_bell (BTK_WIDGET (tree_view));
    }

  btk_tree_view_clamp_column_visible (tree_view,
				      tree_view->priv->focus_column, TRUE);
}

static void
btk_tree_view_move_cursor_start_end (BtkTreeView *tree_view,
				     gint         count)
{
  BtkRBTree *cursor_tree;
  BtkRBNode *cursor_node;
  BtkTreePath *path;
  BtkTreePath *old_path;

  if (!btk_widget_has_focus (BTK_WIDGET (tree_view)))
    return;

  g_return_if_fail (tree_view->priv->tree != NULL);

  btk_tree_view_get_cursor (tree_view, &old_path, NULL);

  cursor_tree = tree_view->priv->tree;
  cursor_node = cursor_tree->root;

  if (count == -1)
    {
      while (cursor_node && cursor_node->left != cursor_tree->nil)
	cursor_node = cursor_node->left;

      /* Now go forward to find the first focusable row. */
      path = _btk_tree_view_find_path (tree_view, cursor_tree, cursor_node);
      search_first_focusable_path (tree_view, &path,
				   TRUE, &cursor_tree, &cursor_node);
    }
  else
    {
      do
	{
	  while (cursor_node && cursor_node->right != cursor_tree->nil)
	    cursor_node = cursor_node->right;
	  if (cursor_node->children == NULL)
	    break;

	  cursor_tree = cursor_node->children;
	  cursor_node = cursor_tree->root;
	}
      while (1);

      /* Now go backwards to find last focusable row. */
      path = _btk_tree_view_find_path (tree_view, cursor_tree, cursor_node);
      search_first_focusable_path (tree_view, &path,
				   FALSE, &cursor_tree, &cursor_node);
    }

  if (!path)
    goto cleanup;

  if (btk_tree_path_compare (old_path, path))
    {
      btk_tree_view_real_set_cursor (tree_view, path, TRUE, TRUE);
      btk_widget_grab_focus (BTK_WIDGET (tree_view));
    }
  else
    {
      btk_widget_error_bell (BTK_WIDGET (tree_view));
    }

cleanup:
  btk_tree_path_free (old_path);
  btk_tree_path_free (path);
}

static gboolean
btk_tree_view_real_select_all (BtkTreeView *tree_view)
{
  if (!btk_widget_has_focus (BTK_WIDGET (tree_view)))
    return FALSE;

  if (tree_view->priv->selection->type != BTK_SELECTION_MULTIPLE)
    return FALSE;

  btk_tree_selection_select_all (tree_view->priv->selection);

  return TRUE;
}

static gboolean
btk_tree_view_real_unselect_all (BtkTreeView *tree_view)
{
  if (!btk_widget_has_focus (BTK_WIDGET (tree_view)))
    return FALSE;

  if (tree_view->priv->selection->type != BTK_SELECTION_MULTIPLE)
    return FALSE;

  btk_tree_selection_unselect_all (tree_view->priv->selection);

  return TRUE;
}

static gboolean
btk_tree_view_real_select_cursor_row (BtkTreeView *tree_view,
				      gboolean     start_editing)
{
  BtkRBTree *new_tree = NULL;
  BtkRBNode *new_node = NULL;
  BtkRBTree *cursor_tree = NULL;
  BtkRBNode *cursor_node = NULL;
  BtkTreePath *cursor_path = NULL;
  BtkTreeSelectMode mode = 0;

  if (!btk_widget_has_focus (BTK_WIDGET (tree_view)))
    return FALSE;

  if (tree_view->priv->cursor)
    cursor_path = btk_tree_row_reference_get_path (tree_view->priv->cursor);

  if (cursor_path == NULL)
    return FALSE;

  _btk_tree_view_find_node (tree_view, cursor_path,
			    &cursor_tree, &cursor_node);

  if (cursor_tree == NULL)
    {
      btk_tree_path_free (cursor_path);
      return FALSE;
    }

  if (!tree_view->priv->extend_selection_pressed && start_editing &&
      tree_view->priv->focus_column)
    {
      if (btk_tree_view_start_editing (tree_view, cursor_path))
	{
	  btk_tree_path_free (cursor_path);
	  return TRUE;
	}
    }

  if (tree_view->priv->modify_selection_pressed)
    mode |= BTK_TREE_SELECT_MODE_TOGGLE;
  if (tree_view->priv->extend_selection_pressed)
    mode |= BTK_TREE_SELECT_MODE_EXTEND;

  _btk_tree_selection_internal_select_node (tree_view->priv->selection,
					    cursor_node,
					    cursor_tree,
					    cursor_path,
                                            mode,
					    FALSE);

  /* We bail out if the original (tree, node) don't exist anymore after
   * handling the selection-changed callback.  We do return TRUE because
   * the key press has been handled at this point.
   */
  _btk_tree_view_find_node (tree_view, cursor_path, &new_tree, &new_node);

  if (cursor_tree != new_tree || cursor_node != new_node)
    return FALSE;

  btk_tree_view_clamp_node_visible (tree_view, cursor_tree, cursor_node);

  btk_widget_grab_focus (BTK_WIDGET (tree_view));
  _btk_tree_view_queue_draw_node (tree_view, cursor_tree, cursor_node, NULL);

  if (!tree_view->priv->extend_selection_pressed)
    btk_tree_view_row_activated (tree_view, cursor_path,
                                 tree_view->priv->focus_column);
    
  btk_tree_path_free (cursor_path);

  return TRUE;
}

static gboolean
btk_tree_view_real_toggle_cursor_row (BtkTreeView *tree_view)
{
  BtkRBTree *new_tree = NULL;
  BtkRBNode *new_node = NULL;
  BtkRBTree *cursor_tree = NULL;
  BtkRBNode *cursor_node = NULL;
  BtkTreePath *cursor_path = NULL;

  if (!btk_widget_has_focus (BTK_WIDGET (tree_view)))
    return FALSE;

  cursor_path = NULL;
  if (tree_view->priv->cursor)
    cursor_path = btk_tree_row_reference_get_path (tree_view->priv->cursor);

  if (cursor_path == NULL)
    return FALSE;

  _btk_tree_view_find_node (tree_view, cursor_path,
			    &cursor_tree, &cursor_node);
  if (cursor_tree == NULL)
    {
      btk_tree_path_free (cursor_path);
      return FALSE;
    }

  _btk_tree_selection_internal_select_node (tree_view->priv->selection,
					    cursor_node,
					    cursor_tree,
					    cursor_path,
                                            BTK_TREE_SELECT_MODE_TOGGLE,
					    FALSE);

  /* We bail out if the original (tree, node) don't exist anymore after
   * handling the selection-changed callback.  We do return TRUE because
   * the key press has been handled at this point.
   */
  _btk_tree_view_find_node (tree_view, cursor_path, &new_tree, &new_node);

  if (cursor_tree != new_tree || cursor_node != new_node)
    return FALSE;

  btk_tree_view_clamp_node_visible (tree_view, cursor_tree, cursor_node);

  btk_widget_grab_focus (BTK_WIDGET (tree_view));
  btk_tree_view_queue_draw_path (tree_view, cursor_path, NULL);
  btk_tree_path_free (cursor_path);

  return TRUE;
}

static gboolean
btk_tree_view_real_expand_collapse_cursor_row (BtkTreeView *tree_view,
					       gboolean     logical,
					       gboolean     expand,
					       gboolean     open_all)
{
  BtkTreePath *cursor_path = NULL;
  BtkRBTree *tree;
  BtkRBNode *node;

  if (!btk_widget_has_focus (BTK_WIDGET (tree_view)))
    return FALSE;

  cursor_path = NULL;
  if (tree_view->priv->cursor)
    cursor_path = btk_tree_row_reference_get_path (tree_view->priv->cursor);

  if (cursor_path == NULL)
    return FALSE;

  if (_btk_tree_view_find_node (tree_view, cursor_path, &tree, &node))
    return FALSE;

  /* Don't handle the event if we aren't an expander */
  if (!((node->flags & BTK_RBNODE_IS_PARENT) == BTK_RBNODE_IS_PARENT))
    return FALSE;

  if (!logical
      && btk_widget_get_direction (BTK_WIDGET (tree_view)) == BTK_TEXT_DIR_RTL)
    expand = !expand;

  if (expand)
    btk_tree_view_real_expand_row (tree_view, cursor_path, tree, node, open_all, TRUE);
  else
    btk_tree_view_real_collapse_row (tree_view, cursor_path, tree, node, TRUE);

  btk_tree_path_free (cursor_path);

  return TRUE;
}

static gboolean
btk_tree_view_real_select_cursor_parent (BtkTreeView *tree_view)
{
  BtkRBTree *cursor_tree = NULL;
  BtkRBNode *cursor_node = NULL;
  BtkTreePath *cursor_path = NULL;
  BdkModifierType state;

  if (!btk_widget_has_focus (BTK_WIDGET (tree_view)))
    goto out;

  cursor_path = NULL;
  if (tree_view->priv->cursor)
    cursor_path = btk_tree_row_reference_get_path (tree_view->priv->cursor);

  if (cursor_path == NULL)
    goto out;

  _btk_tree_view_find_node (tree_view, cursor_path,
			    &cursor_tree, &cursor_node);
  if (cursor_tree == NULL)
    {
      btk_tree_path_free (cursor_path);
      goto out;
    }

  if (cursor_tree->parent_node)
    {
      btk_tree_view_queue_draw_path (tree_view, cursor_path, NULL);
      cursor_node = cursor_tree->parent_node;
      cursor_tree = cursor_tree->parent_tree;

      btk_tree_path_up (cursor_path);

      if (btk_get_current_event_state (&state))
	{
	  if ((state & BTK_MODIFY_SELECTION_MOD_MASK) == BTK_MODIFY_SELECTION_MOD_MASK)
	    tree_view->priv->modify_selection_pressed = TRUE;
	}

      btk_tree_view_real_set_cursor (tree_view, cursor_path, TRUE, FALSE);
      btk_tree_view_clamp_node_visible (tree_view, cursor_tree, cursor_node);

      btk_widget_grab_focus (BTK_WIDGET (tree_view));
      btk_tree_view_queue_draw_path (tree_view, cursor_path, NULL);
      btk_tree_path_free (cursor_path);

      tree_view->priv->modify_selection_pressed = FALSE;

      return TRUE;
    }

 out:

  tree_view->priv->search_entry_avoid_unhandled_binding = TRUE;
  return FALSE;
}

static gboolean
btk_tree_view_search_entry_flush_timeout (BtkTreeView *tree_view)
{
  btk_tree_view_search_dialog_hide (tree_view->priv->search_window, tree_view);
  tree_view->priv->typeselect_flush_timeout = 0;

  return FALSE;
}

/* Cut and paste from btkwindow.c */
static void
send_focus_change (BtkWidget *widget,
		   gboolean   in)
{
  BdkEvent *fevent = bdk_event_new (BDK_FOCUS_CHANGE);

  fevent->focus_change.type = BDK_FOCUS_CHANGE;
  fevent->focus_change.window = g_object_ref (btk_widget_get_window (widget));
  fevent->focus_change.in = in;

  btk_widget_send_focus_change (widget, fevent);

  bdk_event_free (fevent);
}

static void
btk_tree_view_ensure_interactive_directory (BtkTreeView *tree_view)
{
  BtkWidget *frame, *vbox, *toplevel;
  BdkScreen *screen;

  if (tree_view->priv->search_custom_entry_set)
    return;

  toplevel = btk_widget_get_toplevel (BTK_WIDGET (tree_view));
  screen = btk_widget_get_screen (BTK_WIDGET (tree_view));

   if (tree_view->priv->search_window != NULL)
     {
       if (BTK_WINDOW (toplevel)->group)
	 btk_window_group_add_window (BTK_WINDOW (toplevel)->group,
				      BTK_WINDOW (tree_view->priv->search_window));
       else if (BTK_WINDOW (tree_view->priv->search_window)->group)
	 btk_window_group_remove_window (BTK_WINDOW (tree_view->priv->search_window)->group,
					 BTK_WINDOW (tree_view->priv->search_window));
       btk_window_set_screen (BTK_WINDOW (tree_view->priv->search_window), screen);
       return;
     }
   
  tree_view->priv->search_window = btk_window_new (BTK_WINDOW_POPUP);
  btk_window_set_screen (BTK_WINDOW (tree_view->priv->search_window), screen);

  if (BTK_WINDOW (toplevel)->group)
    btk_window_group_add_window (BTK_WINDOW (toplevel)->group,
				 BTK_WINDOW (tree_view->priv->search_window));

  btk_window_set_type_hint (BTK_WINDOW (tree_view->priv->search_window),
			    BDK_WINDOW_TYPE_HINT_UTILITY);
  btk_window_set_modal (BTK_WINDOW (tree_view->priv->search_window), TRUE);
  g_signal_connect (tree_view->priv->search_window, "delete-event",
		    G_CALLBACK (btk_tree_view_search_delete_event),
		    tree_view);
  g_signal_connect (tree_view->priv->search_window, "key-press-event",
		    G_CALLBACK (btk_tree_view_search_key_press_event),
		    tree_view);
  g_signal_connect (tree_view->priv->search_window, "button-press-event",
		    G_CALLBACK (btk_tree_view_search_button_press_event),
		    tree_view);
  g_signal_connect (tree_view->priv->search_window, "scroll-event",
		    G_CALLBACK (btk_tree_view_search_scroll_event),
		    tree_view);

  frame = btk_frame_new (NULL);
  btk_frame_set_shadow_type (BTK_FRAME (frame), BTK_SHADOW_ETCHED_IN);
  btk_widget_show (frame);
  btk_container_add (BTK_CONTAINER (tree_view->priv->search_window), frame);

  vbox = btk_vbox_new (FALSE, 0);
  btk_widget_show (vbox);
  btk_container_add (BTK_CONTAINER (frame), vbox);
  btk_container_set_border_width (BTK_CONTAINER (vbox), 3);

  /* add entry */
  tree_view->priv->search_entry = btk_entry_new ();
  btk_widget_show (tree_view->priv->search_entry);
  g_signal_connect (tree_view->priv->search_entry, "populate-popup",
		    G_CALLBACK (btk_tree_view_search_disable_popdown),
		    tree_view);
  g_signal_connect (tree_view->priv->search_entry,
		    "activate", G_CALLBACK (btk_tree_view_search_activate),
		    tree_view);
  g_signal_connect (BTK_ENTRY (tree_view->priv->search_entry)->im_context,
		    "preedit-changed",
		    G_CALLBACK (btk_tree_view_search_preedit_changed),
		    tree_view);
  btk_container_add (BTK_CONTAINER (vbox),
		     tree_view->priv->search_entry);

  btk_widget_realize (tree_view->priv->search_entry);
}

/* Pops up the interactive search entry.  If keybinding is TRUE then the user
 * started this by typing the start_interactive_search keybinding.  Otherwise, it came from 
 */
static gboolean
btk_tree_view_real_start_interactive_search (BtkTreeView *tree_view,
					     gboolean     keybinding)
{
  /* We only start interactive search if we have focus or the columns
   * have focus.  If one of our children have focus, we don't want to
   * start the search.
   */
  GList *list;
  gboolean found_focus = FALSE;
  BtkWidgetClass *entry_parent_class;
  
  if (!tree_view->priv->enable_search && !keybinding)
    return FALSE;

  if (tree_view->priv->search_custom_entry_set)
    return FALSE;

  if (tree_view->priv->search_window != NULL &&
      btk_widget_get_visible (tree_view->priv->search_window))
    return TRUE;

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      BtkTreeViewColumn *column;

      column = list->data;
      if (! column->visible)
	continue;

      if (btk_widget_has_focus (column->button))
	{
	  found_focus = TRUE;
	  break;
	}
    }
  
  if (btk_widget_has_focus (BTK_WIDGET (tree_view)))
    found_focus = TRUE;

  if (!found_focus)
    return FALSE;

  if (tree_view->priv->search_column < 0)
    return FALSE;

  btk_tree_view_ensure_interactive_directory (tree_view);

  if (keybinding)
    btk_entry_set_text (BTK_ENTRY (tree_view->priv->search_entry), "");

  /* done, show it */
  tree_view->priv->search_position_func (tree_view, tree_view->priv->search_window, tree_view->priv->search_position_user_data);
  btk_widget_show (tree_view->priv->search_window);
  if (tree_view->priv->search_entry_changed_id == 0)
    {
      tree_view->priv->search_entry_changed_id =
	g_signal_connect (tree_view->priv->search_entry, "changed",
			  G_CALLBACK (btk_tree_view_search_init),
			  tree_view);
    }

  tree_view->priv->typeselect_flush_timeout =
    bdk_threads_add_timeout (BTK_TREE_VIEW_SEARCH_DIALOG_TIMEOUT,
		   (GSourceFunc) btk_tree_view_search_entry_flush_timeout,
		   tree_view);

  /* Grab focus will select all the text.  We don't want that to happen, so we
   * call the parent instance and bypass the selection change.  This is probably
   * really non-kosher. */
  entry_parent_class = g_type_class_peek_parent (BTK_ENTRY_GET_CLASS (tree_view->priv->search_entry));
  (entry_parent_class->grab_focus) (tree_view->priv->search_entry);

  /* send focus-in event */
  send_focus_change (tree_view->priv->search_entry, TRUE);

  /* search first matching iter */
  btk_tree_view_search_init (tree_view->priv->search_entry, tree_view);

  return TRUE;
}

static gboolean
btk_tree_view_start_interactive_search (BtkTreeView *tree_view)
{
  return btk_tree_view_real_start_interactive_search (tree_view, TRUE);
}

/* this function returns the new width of the column being resized given
 * the column and x position of the cursor; the x cursor position is passed
 * in as a pointer and automagicly corrected if it's beyond min/max limits
 */
static gint
btk_tree_view_new_column_width (BtkTreeView *tree_view,
				gint       i,
				gint      *x)
{
  BtkTreeViewColumn *column;
  gint width;
  gboolean rtl;

  /* first translate the x position from widget->window
   * to clist->clist_window
   */
  rtl = (btk_widget_get_direction (BTK_WIDGET (tree_view)) == BTK_TEXT_DIR_RTL);
  column = g_list_nth (tree_view->priv->columns, i)->data;
  width = rtl ? (column->button->allocation.x + column->button->allocation.width - *x) : (*x - column->button->allocation.x);
 
  /* Clamp down the value */
  if (column->min_width == -1)
    width = MAX (column->button->requisition.width,
		 width);
  else
    width = MAX (column->min_width,
		 width);
  if (column->max_width != -1)
    width = MIN (width, column->max_width);

  *x = rtl ? (column->button->allocation.x + column->button->allocation.width - width) : (column->button->allocation.x + width);
 
  return width;
}


/* FIXME this adjust_allocation is a big cut-and-paste from
 * BtkCList, needs to be some "official" way to do this
 * factored out.
 */
typedef struct
{
  BdkWindow *window;
  int dx;
  int dy;
} ScrollData;

/* The window to which widget->window is relative */
#define ALLOCATION_WINDOW(widget)		\
   (!btk_widget_get_has_window (widget) ?		\
    (widget)->window :                          \
     bdk_window_get_parent ((widget)->window))

static void
adjust_allocation_recurse (BtkWidget *widget,
			   gpointer   data)
{
  ScrollData *scroll_data = data;

  /* Need to really size allocate instead of just poking
   * into widget->allocation if the widget is not realized.
   * FIXME someone figure out why this was.
   */
  if (!btk_widget_get_realized (widget))
    {
      if (btk_widget_get_visible (widget))
	{
	  BdkRectangle tmp_rectangle = widget->allocation;
	  tmp_rectangle.x += scroll_data->dx;
          tmp_rectangle.y += scroll_data->dy;
          
	  btk_widget_size_allocate (widget, &tmp_rectangle);
	}
    }
  else
    {
      if (ALLOCATION_WINDOW (widget) == scroll_data->window)
	{
	  widget->allocation.x += scroll_data->dx;
          widget->allocation.y += scroll_data->dy;
          
	  if (BTK_IS_CONTAINER (widget))
	    btk_container_forall (BTK_CONTAINER (widget),
				  adjust_allocation_recurse,
				  data);
	}
    }
}

static void
adjust_allocation (BtkWidget *widget,
		   int        dx,
                   int        dy)
{
  ScrollData scroll_data;

  if (btk_widget_get_realized (widget))
    scroll_data.window = ALLOCATION_WINDOW (widget);
  else
    scroll_data.window = NULL;
    
  scroll_data.dx = dx;
  scroll_data.dy = dy;
  
  adjust_allocation_recurse (widget, &scroll_data);
}

/* Callbacks */
static void
btk_tree_view_adjustment_changed (BtkAdjustment *adjustment,
				  BtkTreeView   *tree_view)
{
  if (btk_widget_get_realized (BTK_WIDGET (tree_view)))
    {
      gint dy;
	
      bdk_window_move (tree_view->priv->bin_window,
		       - tree_view->priv->hadjustment->value,
		       TREE_VIEW_HEADER_HEIGHT (tree_view));
      bdk_window_move (tree_view->priv->header_window,
		       - tree_view->priv->hadjustment->value,
		       0);
      dy = tree_view->priv->dy - (int) tree_view->priv->vadjustment->value;
      if (dy)
	{
          update_prelight (tree_view,
                           tree_view->priv->event_last_x,
                           tree_view->priv->event_last_y - dy);

	  if (tree_view->priv->edited_column &&
              BTK_IS_WIDGET (tree_view->priv->edited_column->editable_widget))
	    {
	      GList *list;
	      BtkWidget *widget;
	      BtkTreeViewChild *child = NULL;

	      widget = BTK_WIDGET (tree_view->priv->edited_column->editable_widget);
	      adjust_allocation (widget, 0, dy); 
	      
	      for (list = tree_view->priv->children; list; list = list->next)
		{
		  child = (BtkTreeViewChild *)list->data;
		  if (child->widget == widget)
		    {
		      child->y += dy;
		      break;
		    }
		}
	    }
	}
      bdk_window_scroll (tree_view->priv->bin_window, 0, dy);

      if (tree_view->priv->dy != (int) tree_view->priv->vadjustment->value)
        {
          /* update our dy and top_row */
          tree_view->priv->dy = (int) tree_view->priv->vadjustment->value;

          if (!tree_view->priv->in_top_row_to_dy)
            btk_tree_view_dy_to_top_row (tree_view);
	}

      bdk_window_process_updates (tree_view->priv->header_window, TRUE);
      bdk_window_process_updates (tree_view->priv->bin_window, TRUE);
    }
}



/* Public methods
 */

/**
 * btk_tree_view_new:
 *
 * Creates a new #BtkTreeView widget.
 *
 * Return value: A newly created #BtkTreeView widget.
 **/
BtkWidget *
btk_tree_view_new (void)
{
  return g_object_new (BTK_TYPE_TREE_VIEW, NULL);
}

/**
 * btk_tree_view_new_with_model:
 * @model: the model.
 *
 * Creates a new #BtkTreeView widget with the model initialized to @model.
 *
 * Return value: A newly created #BtkTreeView widget.
 **/
BtkWidget *
btk_tree_view_new_with_model (BtkTreeModel *model)
{
  return g_object_new (BTK_TYPE_TREE_VIEW, "model", model, NULL);
}

/* Public Accessors
 */

/**
 * btk_tree_view_get_model:
 * @tree_view: a #BtkTreeView
 *
 * Returns the model the #BtkTreeView is based on.  Returns %NULL if the
 * model is unset.
 *
 * Return value: (transfer none): A #BtkTreeModel, or %NULL if none is currently being used.
 **/
BtkTreeModel *
btk_tree_view_get_model (BtkTreeView *tree_view)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), NULL);

  return tree_view->priv->model;
}

/**
 * btk_tree_view_set_model:
 * @tree_view: A #BtkTreeNode.
 * @model: (allow-none): The model.
 *
 * Sets the model for a #BtkTreeView.  If the @tree_view already has a model
 * set, it will remove it before setting the new model.  If @model is %NULL,
 * then it will unset the old model.
 **/
void
btk_tree_view_set_model (BtkTreeView  *tree_view,
			 BtkTreeModel *model)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (model == NULL || BTK_IS_TREE_MODEL (model));

  if (model == tree_view->priv->model)
    return;

  if (tree_view->priv->scroll_to_path)
    {
      btk_tree_row_reference_free (tree_view->priv->scroll_to_path);
      tree_view->priv->scroll_to_path = NULL;
    }

  if (tree_view->priv->rubber_band_status)
    btk_tree_view_stop_rubber_band (tree_view);

  if (tree_view->priv->model)
    {
      GList *tmplist = tree_view->priv->columns;

      btk_tree_view_unref_and_check_selection_tree (tree_view, tree_view->priv->tree);
      btk_tree_view_stop_editing (tree_view, TRUE);

      remove_expand_collapse_timeout (tree_view);

      g_signal_handlers_disconnect_by_func (tree_view->priv->model,
					    btk_tree_view_row_changed,
					    tree_view);
      g_signal_handlers_disconnect_by_func (tree_view->priv->model,
					    btk_tree_view_row_inserted,
					    tree_view);
      g_signal_handlers_disconnect_by_func (tree_view->priv->model,
					    btk_tree_view_row_has_child_toggled,
					    tree_view);
      g_signal_handlers_disconnect_by_func (tree_view->priv->model,
					    btk_tree_view_row_deleted,
					    tree_view);
      g_signal_handlers_disconnect_by_func (tree_view->priv->model,
					    btk_tree_view_rows_reordered,
					    tree_view);

      for (; tmplist; tmplist = tmplist->next)
	_btk_tree_view_column_unset_model (tmplist->data,
					   tree_view->priv->model);

      if (tree_view->priv->tree)
	btk_tree_view_free_rbtree (tree_view);

      btk_tree_row_reference_free (tree_view->priv->drag_dest_row);
      tree_view->priv->drag_dest_row = NULL;
      btk_tree_row_reference_free (tree_view->priv->cursor);
      tree_view->priv->cursor = NULL;
      btk_tree_row_reference_free (tree_view->priv->anchor);
      tree_view->priv->anchor = NULL;
      btk_tree_row_reference_free (tree_view->priv->top_row);
      tree_view->priv->top_row = NULL;
      btk_tree_row_reference_free (tree_view->priv->scroll_to_path);
      tree_view->priv->scroll_to_path = NULL;

      tree_view->priv->scroll_to_column = NULL;

      g_object_unref (tree_view->priv->model);

      tree_view->priv->search_column = -1;
      tree_view->priv->fixed_height_check = 0;
      tree_view->priv->fixed_height = -1;
      tree_view->priv->dy = tree_view->priv->top_row_dy = 0;
      tree_view->priv->last_button_x = -1;
      tree_view->priv->last_button_y = -1;
    }

  tree_view->priv->model = model;

  if (tree_view->priv->model)
    {
      gint i;
      BtkTreePath *path;
      BtkTreeIter iter;
      BtkTreeModelFlags flags;

      if (tree_view->priv->search_column == -1)
	{
	  for (i = 0; i < btk_tree_model_get_n_columns (model); i++)
	    {
	      GType type = btk_tree_model_get_column_type (model, i);

	      if (b_value_type_transformable (type, B_TYPE_STRING))
		{
		  tree_view->priv->search_column = i;
		  break;
		}
	    }
	}

      g_object_ref (tree_view->priv->model);
      g_signal_connect (tree_view->priv->model,
			"row-changed",
			G_CALLBACK (btk_tree_view_row_changed),
			tree_view);
      g_signal_connect (tree_view->priv->model,
			"row-inserted",
			G_CALLBACK (btk_tree_view_row_inserted),
			tree_view);
      g_signal_connect (tree_view->priv->model,
			"row-has-child-toggled",
			G_CALLBACK (btk_tree_view_row_has_child_toggled),
			tree_view);
      g_signal_connect (tree_view->priv->model,
			"row-deleted",
			G_CALLBACK (btk_tree_view_row_deleted),
			tree_view);
      g_signal_connect (tree_view->priv->model,
			"rows-reordered",
			G_CALLBACK (btk_tree_view_rows_reordered),
			tree_view);

      flags = btk_tree_model_get_flags (tree_view->priv->model);
      if ((flags & BTK_TREE_MODEL_LIST_ONLY) == BTK_TREE_MODEL_LIST_ONLY)
        BTK_TREE_VIEW_SET_FLAG (tree_view, BTK_TREE_VIEW_IS_LIST);
      else
        BTK_TREE_VIEW_UNSET_FLAG (tree_view, BTK_TREE_VIEW_IS_LIST);

      path = btk_tree_path_new_first ();
      if (btk_tree_model_get_iter (tree_view->priv->model, &iter, path))
	{
	  tree_view->priv->tree = _btk_rbtree_new ();
	  btk_tree_view_build_tree (tree_view, tree_view->priv->tree, &iter, 1, FALSE);
	}
      btk_tree_path_free (path);

      /*  FIXME: do I need to do this? btk_tree_view_create_buttons (tree_view); */
      install_presize_handler (tree_view);
    }

  g_object_notify (B_OBJECT (tree_view), "model");

  if (tree_view->priv->selection)
  _btk_tree_selection_emit_changed (tree_view->priv->selection);

  if (btk_widget_get_realized (BTK_WIDGET (tree_view)))
    btk_widget_queue_resize (BTK_WIDGET (tree_view));
}

/**
 * btk_tree_view_get_selection:
 * @tree_view: A #BtkTreeView.
 *
 * Gets the #BtkTreeSelection associated with @tree_view.
 *
 * Return value: (transfer none): A #BtkTreeSelection object.
 **/
BtkTreeSelection *
btk_tree_view_get_selection (BtkTreeView *tree_view)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), NULL);

  return tree_view->priv->selection;
}

/**
 * btk_tree_view_get_hadjustment:
 * @tree_view: A #BtkTreeView
 *
 * Gets the #BtkAdjustment currently being used for the horizontal aspect.
 *
 * Return value: (transfer none): A #BtkAdjustment object, or %NULL
 *     if none is currently being used.
 **/
BtkAdjustment *
btk_tree_view_get_hadjustment (BtkTreeView *tree_view)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), NULL);

  if (tree_view->priv->hadjustment == NULL)
    btk_tree_view_set_hadjustment (tree_view, NULL);

  return tree_view->priv->hadjustment;
}

/**
 * btk_tree_view_set_hadjustment:
 * @tree_view: A #BtkTreeView
 * @adjustment: (allow-none): The #BtkAdjustment to set, or %NULL
 *
 * Sets the #BtkAdjustment for the current horizontal aspect.
 **/
void
btk_tree_view_set_hadjustment (BtkTreeView   *tree_view,
			       BtkAdjustment *adjustment)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  btk_tree_view_set_adjustments (tree_view,
				 adjustment,
				 tree_view->priv->vadjustment);

  g_object_notify (B_OBJECT (tree_view), "hadjustment");
}

/**
 * btk_tree_view_get_vadjustment:
 * @tree_view: A #BtkTreeView
 *
 * Gets the #BtkAdjustment currently being used for the vertical aspect.
 *
 * Return value: (transfer none): A #BtkAdjustment object, or %NULL
 *     if none is currently being used.
 **/
BtkAdjustment *
btk_tree_view_get_vadjustment (BtkTreeView *tree_view)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), NULL);

  if (tree_view->priv->vadjustment == NULL)
    btk_tree_view_set_vadjustment (tree_view, NULL);

  return tree_view->priv->vadjustment;
}

/**
 * btk_tree_view_set_vadjustment:
 * @tree_view: A #BtkTreeView
 * @adjustment: (allow-none): The #BtkAdjustment to set, or %NULL
 *
 * Sets the #BtkAdjustment for the current vertical aspect.
 **/
void
btk_tree_view_set_vadjustment (BtkTreeView   *tree_view,
			       BtkAdjustment *adjustment)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  btk_tree_view_set_adjustments (tree_view,
				 tree_view->priv->hadjustment,
				 adjustment);

  g_object_notify (B_OBJECT (tree_view), "vadjustment");
}

/* Column and header operations */

/**
 * btk_tree_view_get_headers_visible:
 * @tree_view: A #BtkTreeView.
 *
 * Returns %TRUE if the headers on the @tree_view are visible.
 *
 * Return value: Whether the headers are visible or not.
 **/
gboolean
btk_tree_view_get_headers_visible (BtkTreeView *tree_view)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), FALSE);

  return BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_HEADERS_VISIBLE);
}

/**
 * btk_tree_view_set_headers_visible:
 * @tree_view: A #BtkTreeView.
 * @headers_visible: %TRUE if the headers are visible
 *
 * Sets the visibility state of the headers.
 **/
void
btk_tree_view_set_headers_visible (BtkTreeView *tree_view,
				   gboolean     headers_visible)
{
  gint x, y;
  GList *list;
  BtkTreeViewColumn *column;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  headers_visible = !! headers_visible;

  if (BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_HEADERS_VISIBLE) == headers_visible)
    return;

  if (headers_visible)
    BTK_TREE_VIEW_SET_FLAG (tree_view, BTK_TREE_VIEW_HEADERS_VISIBLE);
  else
    BTK_TREE_VIEW_UNSET_FLAG (tree_view, BTK_TREE_VIEW_HEADERS_VISIBLE);

  if (btk_widget_get_realized (BTK_WIDGET (tree_view)))
    {
      bdk_window_get_position (tree_view->priv->bin_window, &x, &y);
      if (headers_visible)
	{
	  bdk_window_move_resize (tree_view->priv->bin_window, x, y  + TREE_VIEW_HEADER_HEIGHT (tree_view), tree_view->priv->width, BTK_WIDGET (tree_view)->allocation.height -  + TREE_VIEW_HEADER_HEIGHT (tree_view));

          if (btk_widget_get_mapped (BTK_WIDGET (tree_view)))
            btk_tree_view_map_buttons (tree_view);
 	}
      else
	{
	  bdk_window_move_resize (tree_view->priv->bin_window, x, y, tree_view->priv->width, tree_view->priv->height);

	  for (list = tree_view->priv->columns; list; list = list->next)
	    {
	      column = list->data;
	      btk_widget_unmap (column->button);
	    }
	  bdk_window_hide (tree_view->priv->header_window);
	}
    }

  tree_view->priv->vadjustment->page_size = BTK_WIDGET (tree_view)->allocation.height - TREE_VIEW_HEADER_HEIGHT (tree_view);
  tree_view->priv->vadjustment->page_increment = (BTK_WIDGET (tree_view)->allocation.height - TREE_VIEW_HEADER_HEIGHT (tree_view)) / 2;
  tree_view->priv->vadjustment->lower = 0;
  tree_view->priv->vadjustment->upper = tree_view->priv->height;
  btk_adjustment_changed (tree_view->priv->vadjustment);

  btk_widget_queue_resize (BTK_WIDGET (tree_view));

  g_object_notify (B_OBJECT (tree_view), "headers-visible");
}

/**
 * btk_tree_view_columns_autosize:
 * @tree_view: A #BtkTreeView.
 *
 * Resizes all columns to their optimal width. Only works after the
 * treeview has been realized.
 **/
void
btk_tree_view_columns_autosize (BtkTreeView *tree_view)
{
  gboolean dirty = FALSE;
  GList *list;
  BtkTreeViewColumn *column;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      column = list->data;
      if (column->column_type == BTK_TREE_VIEW_COLUMN_AUTOSIZE)
	continue;
      _btk_tree_view_column_cell_set_dirty (column, TRUE);
      dirty = TRUE;
    }

  if (dirty)
    btk_widget_queue_resize (BTK_WIDGET (tree_view));
}

/**
 * btk_tree_view_set_headers_clickable:
 * @tree_view: A #BtkTreeView.
 * @setting: %TRUE if the columns are clickable.
 *
 * Allow the column title buttons to be clicked.
 **/
void
btk_tree_view_set_headers_clickable (BtkTreeView *tree_view,
				     gboolean   setting)
{
  GList *list;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  for (list = tree_view->priv->columns; list; list = list->next)
    btk_tree_view_column_set_clickable (BTK_TREE_VIEW_COLUMN (list->data), setting);

  g_object_notify (B_OBJECT (tree_view), "headers-clickable");
}


/**
 * btk_tree_view_get_headers_clickable:
 * @tree_view: A #BtkTreeView.
 *
 * Returns whether all header columns are clickable.
 *
 * Return value: %TRUE if all header columns are clickable, otherwise %FALSE
 *
 * Since: 2.10
 **/
gboolean 
btk_tree_view_get_headers_clickable (BtkTreeView *tree_view)
{
  GList *list;
  
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), FALSE);

  for (list = tree_view->priv->columns; list; list = list->next)
    if (!BTK_TREE_VIEW_COLUMN (list->data)->clickable)
      return FALSE;

  return TRUE;
}

/**
 * btk_tree_view_set_rules_hint
 * @tree_view: a #BtkTreeView
 * @setting: %TRUE if the tree requires reading across rows
 *
 * This function tells BTK+ that the user interface for your
 * application requires users to read across tree rows and associate
 * cells with one another. By default, BTK+ will then render the tree
 * with alternating row colors. Do <emphasis>not</emphasis> use it
 * just because you prefer the appearance of the ruled tree; that's a
 * question for the theme. Some themes will draw tree rows in
 * alternating colors even when rules are turned off, and users who
 * prefer that appearance all the time can choose those themes. You
 * should call this function only as a <emphasis>semantic</emphasis>
 * hint to the theme engine that your tree makes alternating colors
 * useful from a functional standpoint (since it has lots of columns,
 * generally).
 *
 **/
void
btk_tree_view_set_rules_hint (BtkTreeView  *tree_view,
                              gboolean      setting)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  setting = setting != FALSE;

  if (tree_view->priv->has_rules != setting)
    {
      tree_view->priv->has_rules = setting;
      btk_widget_queue_draw (BTK_WIDGET (tree_view));
    }

  g_object_notify (B_OBJECT (tree_view), "rules-hint");
}

/**
 * btk_tree_view_get_rules_hint
 * @tree_view: a #BtkTreeView
 *
 * Gets the setting set by btk_tree_view_set_rules_hint().
 *
 * Return value: %TRUE if rules are useful for the user of this tree
 **/
gboolean
btk_tree_view_get_rules_hint (BtkTreeView  *tree_view)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), FALSE);

  return tree_view->priv->has_rules;
}

/* Public Column functions
 */

/**
 * btk_tree_view_append_column:
 * @tree_view: A #BtkTreeView.
 * @column: The #BtkTreeViewColumn to add.
 *
 * Appends @column to the list of columns. If @tree_view has "fixed_height"
 * mode enabled, then @column must have its "sizing" property set to be
 * BTK_TREE_VIEW_COLUMN_FIXED.
 *
 * Return value: The number of columns in @tree_view after appending.
 **/
gint
btk_tree_view_append_column (BtkTreeView       *tree_view,
			     BtkTreeViewColumn *column)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), -1);
  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (column), -1);
  g_return_val_if_fail (column->tree_view == NULL, -1);

  return btk_tree_view_insert_column (tree_view, column, -1);
}


/**
 * btk_tree_view_remove_column:
 * @tree_view: A #BtkTreeView.
 * @column: The #BtkTreeViewColumn to remove.
 *
 * Removes @column from @tree_view.
 *
 * Return value: The number of columns in @tree_view after removing.
 **/
gint
btk_tree_view_remove_column (BtkTreeView       *tree_view,
                             BtkTreeViewColumn *column)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), -1);
  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (column), -1);
  g_return_val_if_fail (column->tree_view == BTK_WIDGET (tree_view), -1);

  if (tree_view->priv->focus_column == column)
    tree_view->priv->focus_column = NULL;

  if (tree_view->priv->edited_column == column)
    {
      btk_tree_view_stop_editing (tree_view, TRUE);

      /* no need to, but just to be sure ... */
      tree_view->priv->edited_column = NULL;
    }

  if (tree_view->priv->expander_column == column)
    tree_view->priv->expander_column = NULL;

  g_signal_handlers_disconnect_by_func (column,
                                        G_CALLBACK (column_sizing_notify),
                                        tree_view);

  _btk_tree_view_column_unset_tree_view (column);

  tree_view->priv->columns = g_list_remove (tree_view->priv->columns, column);
  tree_view->priv->n_columns--;

  if (btk_widget_get_realized (BTK_WIDGET (tree_view)))
    {
      GList *list;

      _btk_tree_view_column_unrealize_button (column);
      for (list = tree_view->priv->columns; list; list = list->next)
	{
	  BtkTreeViewColumn *tmp_column;

	  tmp_column = BTK_TREE_VIEW_COLUMN (list->data);
	  if (tmp_column->visible)
	    _btk_tree_view_column_cell_set_dirty (tmp_column, TRUE);
	}

      if (tree_view->priv->n_columns == 0 &&
	  btk_tree_view_get_headers_visible (tree_view))
	bdk_window_hide (tree_view->priv->header_window);

      btk_widget_queue_resize (BTK_WIDGET (tree_view));
    }

  g_object_unref (column);
  g_signal_emit (tree_view, tree_view_signals[COLUMNS_CHANGED], 0);

  return tree_view->priv->n_columns;
}

/**
 * btk_tree_view_insert_column:
 * @tree_view: A #BtkTreeView.
 * @column: The #BtkTreeViewColumn to be inserted.
 * @position: The position to insert @column in.
 *
 * This inserts the @column into the @tree_view at @position.  If @position is
 * -1, then the column is inserted at the end. If @tree_view has
 * "fixed_height" mode enabled, then @column must have its "sizing" property
 * set to be BTK_TREE_VIEW_COLUMN_FIXED.
 *
 * Return value: The number of columns in @tree_view after insertion.
 **/
gint
btk_tree_view_insert_column (BtkTreeView       *tree_view,
                             BtkTreeViewColumn *column,
                             gint               position)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), -1);
  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (column), -1);
  g_return_val_if_fail (column->tree_view == NULL, -1);

  if (tree_view->priv->fixed_height_mode)
    g_return_val_if_fail (btk_tree_view_column_get_sizing (column)
                          == BTK_TREE_VIEW_COLUMN_FIXED, -1);

  g_object_ref_sink (column);

  if (tree_view->priv->n_columns == 0 &&
      btk_widget_get_realized (BTK_WIDGET (tree_view)) &&
      btk_tree_view_get_headers_visible (tree_view))
    {
      bdk_window_show (tree_view->priv->header_window);
    }

  g_signal_connect (column, "notify::sizing",
                    G_CALLBACK (column_sizing_notify), tree_view);

  tree_view->priv->columns = g_list_insert (tree_view->priv->columns,
					    column, position);
  tree_view->priv->n_columns++;

  _btk_tree_view_column_set_tree_view (column, tree_view);

  if (btk_widget_get_realized (BTK_WIDGET (tree_view)))
    {
      GList *list;

      _btk_tree_view_column_realize_button (column);

      for (list = tree_view->priv->columns; list; list = list->next)
	{
	  column = BTK_TREE_VIEW_COLUMN (list->data);
	  if (column->visible)
	    _btk_tree_view_column_cell_set_dirty (column, TRUE);
	}
      btk_widget_queue_resize (BTK_WIDGET (tree_view));
    }

  g_signal_emit (tree_view, tree_view_signals[COLUMNS_CHANGED], 0);

  return tree_view->priv->n_columns;
}

/**
 * btk_tree_view_insert_column_with_attributes:
 * @tree_view: A #BtkTreeView
 * @position: The position to insert the new column in.
 * @title: The title to set the header to.
 * @cell: The #BtkCellRenderer.
 * @Varargs: A %NULL-terminated list of attributes.
 *
 * Creates a new #BtkTreeViewColumn and inserts it into the @tree_view at
 * @position.  If @position is -1, then the newly created column is inserted at
 * the end.  The column is initialized with the attributes given. If @tree_view
 * has "fixed_height" mode enabled, then the new column will have its sizing
 * property set to be BTK_TREE_VIEW_COLUMN_FIXED.
 *
 * Return value: The number of columns in @tree_view after insertion.
 **/
gint
btk_tree_view_insert_column_with_attributes (BtkTreeView     *tree_view,
					     gint             position,
					     const gchar     *title,
					     BtkCellRenderer *cell,
					     ...)
{
  BtkTreeViewColumn *column;
  gchar *attribute;
  va_list args;
  gint column_id;

  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), -1);

  column = btk_tree_view_column_new ();
  if (tree_view->priv->fixed_height_mode)
    btk_tree_view_column_set_sizing (column, BTK_TREE_VIEW_COLUMN_FIXED);

  btk_tree_view_column_set_title (column, title);
  btk_tree_view_column_pack_start (column, cell, TRUE);

  va_start (args, cell);

  attribute = va_arg (args, gchar *);

  while (attribute != NULL)
    {
      column_id = va_arg (args, gint);
      btk_tree_view_column_add_attribute (column, cell, attribute, column_id);
      attribute = va_arg (args, gchar *);
    }

  va_end (args);

  btk_tree_view_insert_column (tree_view, column, position);

  return tree_view->priv->n_columns;
}

/**
 * btk_tree_view_insert_column_with_data_func:
 * @tree_view: a #BtkTreeView
 * @position: Position to insert, -1 for append
 * @title: column title
 * @cell: cell renderer for column
 * @func: function to set attributes of cell renderer
 * @data: data for @func
 * @dnotify: destroy notifier for @data
 *
 * Convenience function that inserts a new column into the #BtkTreeView
 * with the given cell renderer and a #BtkCellDataFunc to set cell renderer
 * attributes (normally using data from the model). See also
 * btk_tree_view_column_set_cell_data_func(), btk_tree_view_column_pack_start().
 * If @tree_view has "fixed_height" mode enabled, then the new column will have its
 * "sizing" property set to be BTK_TREE_VIEW_COLUMN_FIXED.
 *
 * Return value: number of columns in the tree view post-insert
 **/
gint
btk_tree_view_insert_column_with_data_func  (BtkTreeView               *tree_view,
                                             gint                       position,
                                             const gchar               *title,
                                             BtkCellRenderer           *cell,
                                             BtkTreeCellDataFunc        func,
                                             gpointer                   data,
                                             GDestroyNotify             dnotify)
{
  BtkTreeViewColumn *column;

  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), -1);

  column = btk_tree_view_column_new ();
  if (tree_view->priv->fixed_height_mode)
    btk_tree_view_column_set_sizing (column, BTK_TREE_VIEW_COLUMN_FIXED);

  btk_tree_view_column_set_title (column, title);
  btk_tree_view_column_pack_start (column, cell, TRUE);
  btk_tree_view_column_set_cell_data_func (column, cell, func, data, dnotify);

  btk_tree_view_insert_column (tree_view, column, position);

  return tree_view->priv->n_columns;
}

/**
 * btk_tree_view_get_column:
 * @tree_view: A #BtkTreeView.
 * @n: The position of the column, counting from 0.
 *
 * Gets the #BtkTreeViewColumn at the given position in the #tree_view.
 *
 * Return value: (transfer none): The #BtkTreeViewColumn, or %NULL if the
 *     position is outside the range of columns.
 **/
BtkTreeViewColumn *
btk_tree_view_get_column (BtkTreeView *tree_view,
			  gint         n)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), NULL);

  if (n < 0 || n >= tree_view->priv->n_columns)
    return NULL;

  if (tree_view->priv->columns == NULL)
    return NULL;

  return BTK_TREE_VIEW_COLUMN (g_list_nth (tree_view->priv->columns, n)->data);
}

/**
 * btk_tree_view_get_columns:
 * @tree_view: A #BtkTreeView
 *
 * Returns a #GList of all the #BtkTreeViewColumn s currently in @tree_view.
 * The returned list must be freed with g_list_free ().
 *
 * Return value: (element-type BtkTreeViewColumn) (transfer container): A list of #BtkTreeViewColumn s
 **/
GList *
btk_tree_view_get_columns (BtkTreeView *tree_view)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), NULL);

  return g_list_copy (tree_view->priv->columns);
}

/**
 * btk_tree_view_move_column_after:
 * @tree_view: A #BtkTreeView
 * @column: The #BtkTreeViewColumn to be moved.
 * @base_column: (allow-none): The #BtkTreeViewColumn to be moved relative to, or %NULL.
 *
 * Moves @column to be after to @base_column.  If @base_column is %NULL, then
 * @column is placed in the first position.
 **/
void
btk_tree_view_move_column_after (BtkTreeView       *tree_view,
				 BtkTreeViewColumn *column,
				 BtkTreeViewColumn *base_column)
{
  GList *column_list_el, *base_el = NULL;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  column_list_el = g_list_find (tree_view->priv->columns, column);
  g_return_if_fail (column_list_el != NULL);

  if (base_column)
    {
      base_el = g_list_find (tree_view->priv->columns, base_column);
      g_return_if_fail (base_el != NULL);
    }

  if (column_list_el->prev == base_el)
    return;

  tree_view->priv->columns = g_list_remove_link (tree_view->priv->columns, column_list_el);
  if (base_el == NULL)
    {
      column_list_el->prev = NULL;
      column_list_el->next = tree_view->priv->columns;
      if (column_list_el->next)
	column_list_el->next->prev = column_list_el;
      tree_view->priv->columns = column_list_el;
    }
  else
    {
      column_list_el->prev = base_el;
      column_list_el->next = base_el->next;
      if (column_list_el->next)
	column_list_el->next->prev = column_list_el;
      base_el->next = column_list_el;
    }

  if (btk_widget_get_realized (BTK_WIDGET (tree_view)))
    {
      btk_widget_queue_resize (BTK_WIDGET (tree_view));
      btk_tree_view_size_allocate_columns (BTK_WIDGET (tree_view), NULL);
    }

  g_signal_emit (tree_view, tree_view_signals[COLUMNS_CHANGED], 0);
}

/**
 * btk_tree_view_set_expander_column:
 * @tree_view: A #BtkTreeView
 * @column: %NULL, or the column to draw the expander arrow at.
 *
 * Sets the column to draw the expander arrow at. It must be in @tree_view.  
 * If @column is %NULL, then the expander arrow is always at the first 
 * visible column.
 *
 * If you do not want expander arrow to appear in your tree, set the 
 * expander column to a hidden column.
 **/
void
btk_tree_view_set_expander_column (BtkTreeView       *tree_view,
                                   BtkTreeViewColumn *column)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (column == NULL || BTK_IS_TREE_VIEW_COLUMN (column));

  if (tree_view->priv->expander_column != column)
    {
      GList *list;

      if (column)
	{
	  /* Confirm that column is in tree_view */
	  for (list = tree_view->priv->columns; list; list = list->next)
	    if (list->data == column)
	      break;
	  g_return_if_fail (list != NULL);
	}

      tree_view->priv->expander_column = column;
      g_object_notify (B_OBJECT (tree_view), "expander-column");
    }
}

/**
 * btk_tree_view_get_expander_column:
 * @tree_view: A #BtkTreeView
 *
 * Returns the column that is the current expander column.
 * This column has the expander arrow drawn next to it.
 *
 * Return value: (transfer none): The expander column.
 **/
BtkTreeViewColumn *
btk_tree_view_get_expander_column (BtkTreeView *tree_view)
{
  GList *list;

  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), NULL);

  for (list = tree_view->priv->columns; list; list = list->next)
    if (btk_tree_view_is_expander_column (tree_view, BTK_TREE_VIEW_COLUMN (list->data)))
      return (BtkTreeViewColumn *) list->data;
  return NULL;
}


/**
 * btk_tree_view_set_column_drag_function:
 * @tree_view: A #BtkTreeView.
 * @func: (allow-none): A function to determine which columns are reorderable, or %NULL.
 * @user_data: (allow-none): User data to be passed to @func, or %NULL
 * @destroy: (allow-none): Destroy notifier for @user_data, or %NULL
 *
 * Sets a user function for determining where a column may be dropped when
 * dragged.  This function is called on every column pair in turn at the
 * beginning of a column drag to determine where a drop can take place.  The
 * arguments passed to @func are: the @tree_view, the #BtkTreeViewColumn being
 * dragged, the two #BtkTreeViewColumn s determining the drop spot, and
 * @user_data.  If either of the #BtkTreeViewColumn arguments for the drop spot
 * are %NULL, then they indicate an edge.  If @func is set to be %NULL, then
 * @tree_view reverts to the default behavior of allowing all columns to be
 * dropped everywhere.
 **/
void
btk_tree_view_set_column_drag_function (BtkTreeView               *tree_view,
					BtkTreeViewColumnDropFunc  func,
					gpointer                   user_data,
					GDestroyNotify             destroy)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  if (tree_view->priv->column_drop_func_data_destroy)
    tree_view->priv->column_drop_func_data_destroy (tree_view->priv->column_drop_func_data);

  tree_view->priv->column_drop_func = func;
  tree_view->priv->column_drop_func_data = user_data;
  tree_view->priv->column_drop_func_data_destroy = destroy;
}

/**
 * btk_tree_view_scroll_to_point:
 * @tree_view: a #BtkTreeView
 * @tree_x: X coordinate of new top-left pixel of visible area, or -1
 * @tree_y: Y coordinate of new top-left pixel of visible area, or -1
 *
 * Scrolls the tree view such that the top-left corner of the visible
 * area is @tree_x, @tree_y, where @tree_x and @tree_y are specified
 * in tree coordinates.  The @tree_view must be realized before
 * this function is called.  If it isn't, you probably want to be
 * using btk_tree_view_scroll_to_cell().
 *
 * If either @tree_x or @tree_y are -1, then that direction isn't scrolled.
 **/
void
btk_tree_view_scroll_to_point (BtkTreeView *tree_view,
                               gint         tree_x,
                               gint         tree_y)
{
  BtkAdjustment *hadj;
  BtkAdjustment *vadj;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (btk_widget_get_realized (BTK_WIDGET (tree_view)));

  hadj = tree_view->priv->hadjustment;
  vadj = tree_view->priv->vadjustment;

  if (tree_x != -1)
    btk_adjustment_set_value (hadj, CLAMP (tree_x, hadj->lower, hadj->upper - hadj->page_size));
  if (tree_y != -1)
    btk_adjustment_set_value (vadj, CLAMP (tree_y, vadj->lower, vadj->upper - vadj->page_size));
}

/**
 * btk_tree_view_scroll_to_cell:
 * @tree_view: A #BtkTreeView.
 * @path: (allow-none): The path of the row to move to, or %NULL.
 * @column: (allow-none): The #BtkTreeViewColumn to move horizontally to, or %NULL.
 * @use_align: whether to use alignment arguments, or %FALSE.
 * @row_align: The vertical alignment of the row specified by @path.
 * @col_align: The horizontal alignment of the column specified by @column.
 *
 * Moves the alignments of @tree_view to the position specified by @column and
 * @path.  If @column is %NULL, then no horizontal scrolling occurs.  Likewise,
 * if @path is %NULL no vertical scrolling occurs.  At a minimum, one of @column
 * or @path need to be non-%NULL.  @row_align determines where the row is
 * placed, and @col_align determines where @column is placed.  Both are expected
 * to be between 0.0 and 1.0. 0.0 means left/top alignment, 1.0 means
 * right/bottom alignment, 0.5 means center.
 *
 * If @use_align is %FALSE, then the alignment arguments are ignored, and the
 * tree does the minimum amount of work to scroll the cell onto the screen.
 * This means that the cell will be scrolled to the edge closest to its current
 * position.  If the cell is currently visible on the screen, nothing is done.
 *
 * This function only works if the model is set, and @path is a valid row on the
 * model.  If the model changes before the @tree_view is realized, the centered
 * path will be modified to reflect this change.
 **/
void
btk_tree_view_scroll_to_cell (BtkTreeView       *tree_view,
                              BtkTreePath       *path,
                              BtkTreeViewColumn *column,
			      gboolean           use_align,
                              gfloat             row_align,
                              gfloat             col_align)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (tree_view->priv->model != NULL);
  g_return_if_fail (tree_view->priv->tree != NULL);
  g_return_if_fail (row_align >= 0.0 && row_align <= 1.0);
  g_return_if_fail (col_align >= 0.0 && col_align <= 1.0);
  g_return_if_fail (path != NULL || column != NULL);

#if 0
  g_print ("btk_tree_view_scroll_to_cell:\npath: %s\ncolumn: %s\nuse_align: %d\nrow_align: %f\ncol_align: %f\n",
	   btk_tree_path_to_string (path), column?"non-null":"null", use_align, row_align, col_align);
#endif
  row_align = CLAMP (row_align, 0.0, 1.0);
  col_align = CLAMP (col_align, 0.0, 1.0);


  /* Note: Despite the benefits that come from having one code path for the
   * scrolling code, we short-circuit validate_visible_area's immplementation as
   * it is much slower than just going to the point.
   */
  if (!btk_widget_get_visible (BTK_WIDGET (tree_view)) ||
      !btk_widget_get_realized (BTK_WIDGET (tree_view)) ||
      BTK_WIDGET_ALLOC_NEEDED (tree_view) || 
      BTK_RBNODE_FLAG_SET (tree_view->priv->tree->root, BTK_RBNODE_DESCENDANTS_INVALID))
    {
      if (tree_view->priv->scroll_to_path)
	btk_tree_row_reference_free (tree_view->priv->scroll_to_path);

      tree_view->priv->scroll_to_path = NULL;
      tree_view->priv->scroll_to_column = NULL;

      if (path)
	tree_view->priv->scroll_to_path = btk_tree_row_reference_new_proxy (B_OBJECT (tree_view), tree_view->priv->model, path);
      if (column)
	tree_view->priv->scroll_to_column = column;
      tree_view->priv->scroll_to_use_align = use_align;
      tree_view->priv->scroll_to_row_align = row_align;
      tree_view->priv->scroll_to_col_align = col_align;

      install_presize_handler (tree_view);
    }
  else
    {
      BdkRectangle cell_rect;
      BdkRectangle vis_rect;
      gint dest_x, dest_y;

      btk_tree_view_get_background_area (tree_view, path, column, &cell_rect);
      btk_tree_view_get_visible_rect (tree_view, &vis_rect);

      cell_rect.y = TREE_WINDOW_Y_TO_RBTREE_Y (tree_view, cell_rect.y);

      dest_x = vis_rect.x;
      dest_y = vis_rect.y;

      if (column)
	{
	  if (use_align)
	    {
	      dest_x = cell_rect.x - ((vis_rect.width - cell_rect.width) * col_align);
	    }
	  else
	    {
	      if (cell_rect.x < vis_rect.x)
		dest_x = cell_rect.x;
	      if (cell_rect.x + cell_rect.width > vis_rect.x + vis_rect.width)
		dest_x = cell_rect.x + cell_rect.width - vis_rect.width;
	    }
	}

      if (path)
	{
	  if (use_align)
	    {
	      dest_y = cell_rect.y - ((vis_rect.height - cell_rect.height) * row_align);
	      dest_y = MAX (dest_y, 0);
	    }
	  else
	    {
	      if (cell_rect.y < vis_rect.y)
		dest_y = cell_rect.y;
	      if (cell_rect.y + cell_rect.height > vis_rect.y + vis_rect.height)
		dest_y = cell_rect.y + cell_rect.height - vis_rect.height;
	    }
	}

      btk_tree_view_scroll_to_point (tree_view, dest_x, dest_y);
    }
}

/**
 * btk_tree_view_row_activated:
 * @tree_view: A #BtkTreeView
 * @path: The #BtkTreePath to be activated.
 * @column: The #BtkTreeViewColumn to be activated.
 *
 * Activates the cell determined by @path and @column.
 **/
void
btk_tree_view_row_activated (BtkTreeView       *tree_view,
			     BtkTreePath       *path,
			     BtkTreeViewColumn *column)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  g_signal_emit (tree_view, tree_view_signals[ROW_ACTIVATED], 0, path, column);
}


static void
btk_tree_view_expand_all_emission_helper (BtkRBTree *tree,
                                          BtkRBNode *node,
                                          gpointer   data)
{
  BtkTreeView *tree_view = data;

  if ((node->flags & BTK_RBNODE_IS_PARENT) == BTK_RBNODE_IS_PARENT &&
      node->children)
    {
      BtkTreePath *path;
      BtkTreeIter iter;

      path = _btk_tree_view_find_path (tree_view, tree, node);
      btk_tree_model_get_iter (tree_view->priv->model, &iter, path);

      g_signal_emit (tree_view, tree_view_signals[ROW_EXPANDED], 0, &iter, path);

      btk_tree_path_free (path);
    }

  if (node->children)
    _btk_rbtree_traverse (node->children,
                          node->children->root,
                          G_PRE_ORDER,
                          btk_tree_view_expand_all_emission_helper,
                          tree_view);
}

/**
 * btk_tree_view_expand_all:
 * @tree_view: A #BtkTreeView.
 *
 * Recursively expands all nodes in the @tree_view.
 **/
void
btk_tree_view_expand_all (BtkTreeView *tree_view)
{
  BtkTreePath *path;
  BtkRBTree *tree;
  BtkRBNode *node;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  if (tree_view->priv->tree == NULL)
    return;

  path = btk_tree_path_new_first ();
  _btk_tree_view_find_node (tree_view, path, &tree, &node);

  while (node)
    {
      btk_tree_view_real_expand_row (tree_view, path, tree, node, TRUE, FALSE);
      node = _btk_rbtree_next (tree, node);
      btk_tree_path_next (path);
  }

  btk_tree_path_free (path);
}

/* Timeout to animate the expander during expands and collapses */
static gboolean
expand_collapse_timeout (gpointer data)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (data);
  gboolean retval = do_expand_collapse (data);

  if (! retval)
    remove_expand_collapse_timeout (tree_view);

  return retval;
}

static void
add_expand_collapse_timeout (BtkTreeView *tree_view,
                             BtkRBTree   *tree,
                             BtkRBNode   *node,
                             gboolean     expand)
{
  if (tree_view->priv->expand_collapse_timeout != 0)
    return;

  tree_view->priv->expand_collapse_timeout =
      bdk_threads_add_timeout (50, expand_collapse_timeout, tree_view);
  tree_view->priv->expanded_collapsed_tree = tree;
  tree_view->priv->expanded_collapsed_node = node;

  if (expand)
    BTK_RBNODE_SET_FLAG (node, BTK_RBNODE_IS_SEMI_COLLAPSED);
  else
    BTK_RBNODE_SET_FLAG (node, BTK_RBNODE_IS_SEMI_EXPANDED);
}

static void
remove_expand_collapse_timeout (BtkTreeView *tree_view)
{
  if (tree_view->priv->expand_collapse_timeout)
    {
      g_source_remove (tree_view->priv->expand_collapse_timeout);
      tree_view->priv->expand_collapse_timeout = 0;
    }

  if (tree_view->priv->expanded_collapsed_node != NULL)
    {
      BTK_RBNODE_UNSET_FLAG (tree_view->priv->expanded_collapsed_node, BTK_RBNODE_IS_SEMI_EXPANDED);
      BTK_RBNODE_UNSET_FLAG (tree_view->priv->expanded_collapsed_node, BTK_RBNODE_IS_SEMI_COLLAPSED);

      tree_view->priv->expanded_collapsed_node = NULL;
    }
}

static void
cancel_arrow_animation (BtkTreeView *tree_view)
{
  if (tree_view->priv->expand_collapse_timeout)
    {
      while (do_expand_collapse (tree_view));

      remove_expand_collapse_timeout (tree_view);
    }
}

static gboolean
do_expand_collapse (BtkTreeView *tree_view)
{
  BtkRBNode *node;
  BtkRBTree *tree;
  gboolean expanding;
  gboolean redraw;

  redraw = FALSE;
  expanding = TRUE;

  node = tree_view->priv->expanded_collapsed_node;
  tree = tree_view->priv->expanded_collapsed_tree;

  if (node->children == NULL)
    expanding = FALSE;

  if (expanding)
    {
      if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SEMI_COLLAPSED))
	{
	  BTK_RBNODE_UNSET_FLAG (node, BTK_RBNODE_IS_SEMI_COLLAPSED);
	  BTK_RBNODE_SET_FLAG (node, BTK_RBNODE_IS_SEMI_EXPANDED);

	  redraw = TRUE;

	}
      else if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SEMI_EXPANDED))
	{
	  BTK_RBNODE_UNSET_FLAG (node, BTK_RBNODE_IS_SEMI_EXPANDED);

	  redraw = TRUE;
	}
    }
  else
    {
      if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SEMI_EXPANDED))
	{
	  BTK_RBNODE_UNSET_FLAG (node, BTK_RBNODE_IS_SEMI_EXPANDED);
	  BTK_RBNODE_SET_FLAG (node, BTK_RBNODE_IS_SEMI_COLLAPSED);

	  redraw = TRUE;
	}
      else if (BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SEMI_COLLAPSED))
	{
	  BTK_RBNODE_UNSET_FLAG (node, BTK_RBNODE_IS_SEMI_COLLAPSED);

	  redraw = TRUE;

	}
    }

  if (redraw)
    {
      btk_tree_view_queue_draw_arrow (tree_view, tree, node, NULL);

      return TRUE;
    }

  return FALSE;
}

/**
 * btk_tree_view_collapse_all:
 * @tree_view: A #BtkTreeView.
 *
 * Recursively collapses all visible, expanded nodes in @tree_view.
 **/
void
btk_tree_view_collapse_all (BtkTreeView *tree_view)
{
  BtkRBTree *tree;
  BtkRBNode *node;
  BtkTreePath *path;
  gint *indices;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  if (tree_view->priv->tree == NULL)
    return;

  path = btk_tree_path_new ();
  btk_tree_path_down (path);
  indices = btk_tree_path_get_indices (path);

  tree = tree_view->priv->tree;
  node = tree->root;
  while (node && node->left != tree->nil)
    node = node->left;

  while (node)
    {
      if (node->children)
	btk_tree_view_real_collapse_row (tree_view, path, tree, node, FALSE);
      indices[0]++;
      node = _btk_rbtree_next (tree, node);
    }

  btk_tree_path_free (path);
}

/**
 * btk_tree_view_expand_to_path:
 * @tree_view: A #BtkTreeView.
 * @path: path to a row.
 *
 * Expands the row at @path. This will also expand all parent rows of
 * @path as necessary.
 *
 * Since: 2.2
 **/
void
btk_tree_view_expand_to_path (BtkTreeView *tree_view,
			      BtkTreePath *path)
{
  gint i, depth;
  gint *indices;
  BtkTreePath *tmp;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (path != NULL);

  depth = btk_tree_path_get_depth (path);
  indices = btk_tree_path_get_indices (path);

  tmp = btk_tree_path_new ();
  g_return_if_fail (tmp != NULL);

  for (i = 0; i < depth; i++)
    {
      btk_tree_path_append_index (tmp, indices[i]);
      btk_tree_view_expand_row (tree_view, tmp, FALSE);
    }

  btk_tree_path_free (tmp);
}

/* FIXME the bool return values for expand_row and collapse_row are
 * not analagous; they should be TRUE if the row had children and
 * was not already in the requested state.
 */


static gboolean
btk_tree_view_real_expand_row (BtkTreeView *tree_view,
			       BtkTreePath *path,
			       BtkRBTree   *tree,
			       BtkRBNode   *node,
			       gboolean     open_all,
			       gboolean     animate)
{
  BtkTreeIter iter;
  BtkTreeIter temp;
  gboolean expand;

  if (animate)
    g_object_get (btk_widget_get_settings (BTK_WIDGET (tree_view)),
                  "btk-enable-animations", &animate,
                  NULL);

  remove_auto_expand_timeout (tree_view);

  if (node->children && !open_all)
    return FALSE;

  if (! BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_PARENT))
    return FALSE;

  btk_tree_model_get_iter (tree_view->priv->model, &iter, path);
  if (! btk_tree_model_iter_has_child (tree_view->priv->model, &iter))
    return FALSE;


   if (node->children && open_all)
    {
      gboolean retval = FALSE;
      BtkTreePath *tmp_path = btk_tree_path_copy (path);

      btk_tree_path_append_index (tmp_path, 0);
      tree = node->children;
      node = tree->root;
      while (node->left != tree->nil)
	node = node->left;
      /* try to expand the children */
      do
        {
         gboolean t;
	 t = btk_tree_view_real_expand_row (tree_view, tmp_path, tree, node,
					    TRUE, animate);
         if (t)
           retval = TRUE;

         btk_tree_path_next (tmp_path);
	 node = _btk_rbtree_next (tree, node);
       }
      while (node != NULL);

      btk_tree_path_free (tmp_path);

      return retval;
    }

  g_signal_emit (tree_view, tree_view_signals[TEST_EXPAND_ROW], 0, &iter, path, &expand);

  if (!btk_tree_model_iter_has_child (tree_view->priv->model, &iter))
    return FALSE;

  if (expand)
    return FALSE;

  node->children = _btk_rbtree_new ();
  node->children->parent_tree = tree;
  node->children->parent_node = node;

  btk_tree_model_iter_children (tree_view->priv->model, &temp, &iter);

  btk_tree_view_build_tree (tree_view,
			    node->children,
			    &temp,
			    btk_tree_path_get_depth (path) + 1,
			    open_all);

  remove_expand_collapse_timeout (tree_view);

  if (animate)
    add_expand_collapse_timeout (tree_view, tree, node, TRUE);

  install_presize_handler (tree_view);

  g_signal_emit (tree_view, tree_view_signals[ROW_EXPANDED], 0, &iter, path);
  if (open_all && node->children)
    {
      _btk_rbtree_traverse (node->children,
                            node->children->root,
                            G_PRE_ORDER,
                            btk_tree_view_expand_all_emission_helper,
                            tree_view);
    }
  return TRUE;
}


/**
 * btk_tree_view_expand_row:
 * @tree_view: a #BtkTreeView
 * @path: path to a row
 * @open_all: whether to recursively expand, or just expand immediate children
 *
 * Opens the row so its children are visible.
 *
 * Return value: %TRUE if the row existed and had children
 **/
gboolean
btk_tree_view_expand_row (BtkTreeView *tree_view,
			  BtkTreePath *path,
			  gboolean     open_all)
{
  BtkRBTree *tree;
  BtkRBNode *node;

  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), FALSE);
  g_return_val_if_fail (tree_view->priv->model != NULL, FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  if (_btk_tree_view_find_node (tree_view,
				path,
				&tree,
				&node))
    return FALSE;

  if (tree != NULL)
    return btk_tree_view_real_expand_row (tree_view, path, tree, node, open_all, FALSE);
  else
    return FALSE;
}

static gboolean
btk_tree_view_real_collapse_row (BtkTreeView *tree_view,
				 BtkTreePath *path,
				 BtkRBTree   *tree,
				 BtkRBNode   *node,
				 gboolean     animate)
{
  BtkTreeIter iter;
  BtkTreeIter children;
  gboolean collapse;
  gint x, y;
  GList *list;
  BdkWindow *child, *parent;

  if (animate)
    g_object_get (btk_widget_get_settings (BTK_WIDGET (tree_view)),
                  "btk-enable-animations", &animate,
                  NULL);

  remove_auto_expand_timeout (tree_view);

  if (node->children == NULL)
    return FALSE;

  btk_tree_model_get_iter (tree_view->priv->model, &iter, path);

  g_signal_emit (tree_view, tree_view_signals[TEST_COLLAPSE_ROW], 0, &iter, path, &collapse);

  if (collapse)
    return FALSE;

  /* if the prelighted node is a child of us, we want to unprelight it.  We have
   * a chance to prelight the correct node below */

  if (tree_view->priv->prelight_tree)
    {
      BtkRBTree *parent_tree;
      BtkRBNode *parent_node;

      parent_tree = tree_view->priv->prelight_tree->parent_tree;
      parent_node = tree_view->priv->prelight_tree->parent_node;
      while (parent_tree)
	{
	  if (parent_tree == tree && parent_node == node)
	    {
	      ensure_unprelighted (tree_view);
	      break;
	    }
	  parent_node = parent_tree->parent_node;
	  parent_tree = parent_tree->parent_tree;
	}
    }

  TREE_VIEW_INTERNAL_ASSERT (btk_tree_model_iter_children (tree_view->priv->model, &children, &iter), FALSE);

  for (list = tree_view->priv->columns; list; list = list->next)
    {
      BtkTreeViewColumn *column = list->data;

      if (column->visible == FALSE)
	continue;
      if (btk_tree_view_column_get_sizing (column) == BTK_TREE_VIEW_COLUMN_AUTOSIZE)
	_btk_tree_view_column_cell_set_dirty (column, TRUE);
    }

  if (tree_view->priv->destroy_count_func)
    {
      BtkTreePath *child_path;
      gint child_count = 0;
      child_path = btk_tree_path_copy (path);
      btk_tree_path_down (child_path);
      if (node->children)
	_btk_rbtree_traverse (node->children, node->children->root, G_POST_ORDER, count_children_helper, &child_count);
      tree_view->priv->destroy_count_func (tree_view, child_path, child_count, tree_view->priv->destroy_count_data);
      btk_tree_path_free (child_path);
    }

  if (btk_tree_row_reference_valid (tree_view->priv->cursor))
    {
      BtkTreePath *cursor_path = btk_tree_row_reference_get_path (tree_view->priv->cursor);

      if (btk_tree_path_is_ancestor (path, cursor_path))
	{
	  btk_tree_row_reference_free (tree_view->priv->cursor);
	  tree_view->priv->cursor = btk_tree_row_reference_new_proxy (B_OBJECT (tree_view),
								      tree_view->priv->model,
								      path);
	}
      btk_tree_path_free (cursor_path);
    }

  if (btk_tree_row_reference_valid (tree_view->priv->anchor))
    {
      BtkTreePath *anchor_path = btk_tree_row_reference_get_path (tree_view->priv->anchor);
      if (btk_tree_path_is_ancestor (path, anchor_path))
	{
	  btk_tree_row_reference_free (tree_view->priv->anchor);
	  tree_view->priv->anchor = NULL;
	}
      btk_tree_path_free (anchor_path);
    }

  /* Stop a pending double click */
  tree_view->priv->last_button_x = -1;
  tree_view->priv->last_button_y = -1;

  remove_expand_collapse_timeout (tree_view);

  if (btk_tree_view_unref_and_check_selection_tree (tree_view, node->children))
    {
      _btk_rbtree_remove (node->children);
      g_signal_emit_by_name (tree_view->priv->selection, "changed");
    }
  else
    _btk_rbtree_remove (node->children);
  
  if (animate)
    add_expand_collapse_timeout (tree_view, tree, node, FALSE);
  
  if (btk_widget_get_mapped (BTK_WIDGET (tree_view)))
    {
      btk_widget_queue_resize (BTK_WIDGET (tree_view));
    }

  g_signal_emit (tree_view, tree_view_signals[ROW_COLLAPSED], 0, &iter, path);

  if (btk_widget_get_mapped (BTK_WIDGET (tree_view)))
    {
      /* now that we've collapsed all rows, we want to try to set the prelight
       * again. To do this, we fake a motion event and send it to ourselves. */

      child = tree_view->priv->bin_window;
      parent = bdk_window_get_parent (child);

      if (bdk_window_get_pointer (parent, &x, &y, NULL) == child)
	{
	  BdkEventMotion event;
	  gint child_x, child_y;

	  bdk_window_get_position (child, &child_x, &child_y);

	  event.window = tree_view->priv->bin_window;
	  event.x = x - child_x;
	  event.y = y - child_y;

	  /* despite the fact this isn't a real event, I'm almost positive it will
	   * never trigger a drag event.  maybe_drag is the only function that uses
	   * more than just event.x and event.y. */
	  btk_tree_view_motion_bin_window (BTK_WIDGET (tree_view), &event);
	}
    }

  return TRUE;
}

/**
 * btk_tree_view_collapse_row:
 * @tree_view: a #BtkTreeView
 * @path: path to a row in the @tree_view
 *
 * Collapses a row (hides its child rows, if they exist).
 *
 * Return value: %TRUE if the row was collapsed.
 **/
gboolean
btk_tree_view_collapse_row (BtkTreeView *tree_view,
			    BtkTreePath *path)
{
  BtkRBTree *tree;
  BtkRBNode *node;

  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), FALSE);
  g_return_val_if_fail (tree_view->priv->tree != NULL, FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  if (_btk_tree_view_find_node (tree_view,
				path,
				&tree,
				&node))
    return FALSE;

  if (tree == NULL || node->children == NULL)
    return FALSE;

  return btk_tree_view_real_collapse_row (tree_view, path, tree, node, FALSE);
}

static void
btk_tree_view_map_expanded_rows_helper (BtkTreeView            *tree_view,
					BtkRBTree              *tree,
					BtkTreePath            *path,
					BtkTreeViewMappingFunc  func,
					gpointer                user_data)
{
  BtkRBNode *node;

  if (tree == NULL || tree->root == NULL)
    return;

  node = tree->root;

  while (node && node->left != tree->nil)
    node = node->left;

  while (node)
    {
      if (node->children)
	{
	  (* func) (tree_view, path, user_data);
	  btk_tree_path_down (path);
	  btk_tree_view_map_expanded_rows_helper (tree_view, node->children, path, func, user_data);
	  btk_tree_path_up (path);
	}
      btk_tree_path_next (path);
      node = _btk_rbtree_next (tree, node);
    }
}

/**
 * btk_tree_view_map_expanded_rows:
 * @tree_view: A #BtkTreeView
 * @func: (scope call): A function to be called
 * @data: User data to be passed to the function.
 *
 * Calls @func on all expanded rows.
 **/
void
btk_tree_view_map_expanded_rows (BtkTreeView            *tree_view,
				 BtkTreeViewMappingFunc  func,
				 gpointer                user_data)
{
  BtkTreePath *path;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (func != NULL);

  path = btk_tree_path_new_first ();

  btk_tree_view_map_expanded_rows_helper (tree_view,
					  tree_view->priv->tree,
					  path, func, user_data);

  btk_tree_path_free (path);
}

/**
 * btk_tree_view_row_expanded:
 * @tree_view: A #BtkTreeView.
 * @path: A #BtkTreePath to test expansion state.
 *
 * Returns %TRUE if the node pointed to by @path is expanded in @tree_view.
 *
 * Return value: %TRUE if #path is expanded.
 **/
gboolean
btk_tree_view_row_expanded (BtkTreeView *tree_view,
			    BtkTreePath *path)
{
  BtkRBTree *tree;
  BtkRBNode *node;

  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  _btk_tree_view_find_node (tree_view, path, &tree, &node);

  if (node == NULL)
    return FALSE;

  return (node->children != NULL);
}

/**
 * btk_tree_view_get_reorderable:
 * @tree_view: a #BtkTreeView
 *
 * Retrieves whether the user can reorder the tree via drag-and-drop. See
 * btk_tree_view_set_reorderable().
 *
 * Return value: %TRUE if the tree can be reordered.
 **/
gboolean
btk_tree_view_get_reorderable (BtkTreeView *tree_view)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), FALSE);

  return tree_view->priv->reorderable;
}

/**
 * btk_tree_view_set_reorderable:
 * @tree_view: A #BtkTreeView.
 * @reorderable: %TRUE, if the tree can be reordered.
 *
 * This function is a convenience function to allow you to reorder
 * models that support the #BtkDragSourceIface and the
 * #BtkDragDestIface.  Both #BtkTreeStore and #BtkListStore support
 * these.  If @reorderable is %TRUE, then the user can reorder the
 * model by dragging and dropping rows. The developer can listen to
 * these changes by connecting to the model's row_inserted and
 * row_deleted signals. The reordering is implemented by setting up
 * the tree view as a drag source and destination. Therefore, drag and
 * drop can not be used in a reorderable view for any other purpose.
 *
 * This function does not give you any degree of control over the order -- any
 * reordering is allowed.  If more control is needed, you should probably
 * handle drag and drop manually.
 **/
void
btk_tree_view_set_reorderable (BtkTreeView *tree_view,
			       gboolean     reorderable)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  reorderable = reorderable != FALSE;

  if (tree_view->priv->reorderable == reorderable)
    return;

  if (reorderable)
    {
      const BtkTargetEntry row_targets[] = {
        { "BTK_TREE_MODEL_ROW", BTK_TARGET_SAME_WIDGET, 0 }
      };

      btk_tree_view_enable_model_drag_source (tree_view,
					      BDK_BUTTON1_MASK,
					      row_targets,
					      G_N_ELEMENTS (row_targets),
					      BDK_ACTION_MOVE);
      btk_tree_view_enable_model_drag_dest (tree_view,
					    row_targets,
					    G_N_ELEMENTS (row_targets),
					    BDK_ACTION_MOVE);
    }
  else
    {
      btk_tree_view_unset_rows_drag_source (tree_view);
      btk_tree_view_unset_rows_drag_dest (tree_view);
    }

  tree_view->priv->reorderable = reorderable;

  g_object_notify (B_OBJECT (tree_view), "reorderable");
}

static void
btk_tree_view_real_set_cursor (BtkTreeView     *tree_view,
			       BtkTreePath     *path,
			       gboolean         clear_and_select,
			       gboolean         clamp_node)
{
  BtkRBTree *tree = NULL;
  BtkRBNode *node = NULL;

  if (btk_tree_row_reference_valid (tree_view->priv->cursor))
    {
      BtkTreePath *cursor_path;
      cursor_path = btk_tree_row_reference_get_path (tree_view->priv->cursor);
      btk_tree_view_queue_draw_path (tree_view, cursor_path, NULL);
      btk_tree_path_free (cursor_path);
    }

  btk_tree_row_reference_free (tree_view->priv->cursor);
  tree_view->priv->cursor = NULL;

  /* One cannot set the cursor on a separator.   Also, if
   * _btk_tree_view_find_node returns TRUE, it ran out of tree
   * before finding the tree and node belonging to path.  The
   * path maps to a non-existing path and we will silently bail out.
   * We unset tree and node to avoid further processing.
   */
  if (!row_is_separator (tree_view, NULL, path)
      && _btk_tree_view_find_node (tree_view, path, &tree, &node) == FALSE)
    {
      tree_view->priv->cursor =
          btk_tree_row_reference_new_proxy (B_OBJECT (tree_view),
                                            tree_view->priv->model,
                                            path);
    }
  else
    {
      tree = NULL;
      node = NULL;
    }

  if (tree != NULL)
    {
      BtkRBTree *new_tree = NULL;
      BtkRBNode *new_node = NULL;

      if (clear_and_select && !tree_view->priv->modify_selection_pressed)
        {
          BtkTreeSelectMode mode = 0;

          if (tree_view->priv->modify_selection_pressed)
            mode |= BTK_TREE_SELECT_MODE_TOGGLE;
          if (tree_view->priv->extend_selection_pressed)
            mode |= BTK_TREE_SELECT_MODE_EXTEND;

          _btk_tree_selection_internal_select_node (tree_view->priv->selection,
                                                    node, tree, path, mode,
                                                    FALSE);
        }

      /* We have to re-find tree and node here again, somebody might have
       * cleared the node or the whole tree in the BtkTreeSelection::changed
       * callback. If the nodes differ we bail out here.
       */
      _btk_tree_view_find_node (tree_view, path, &new_tree, &new_node);

      if (tree != new_tree || node != new_node)
        return;

      if (clamp_node)
        {
	  btk_tree_view_clamp_node_visible (tree_view, tree, node);
	  _btk_tree_view_queue_draw_node (tree_view, tree, node, NULL);
	}
    }

  g_signal_emit (tree_view, tree_view_signals[CURSOR_CHANGED], 0);
}

/**
 * btk_tree_view_get_cursor:
 * @tree_view: A #BtkTreeView
 * @path: (out) (transfer full) (allow-none): A pointer to be filled with the current cursor path, or %NULL
 * @focus_column: (out) (transfer none) (allow-none): A pointer to be filled with the current focus column, or %NULL
 *
 * Fills in @path and @focus_column with the current path and focus column.  If
 * the cursor isn't currently set, then *@path will be %NULL.  If no column
 * currently has focus, then *@focus_column will be %NULL.
 *
 * The returned #BtkTreePath must be freed with btk_tree_path_free() when
 * you are done with it.
 **/
void
btk_tree_view_get_cursor (BtkTreeView        *tree_view,
			  BtkTreePath       **path,
			  BtkTreeViewColumn **focus_column)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  if (path)
    {
      if (btk_tree_row_reference_valid (tree_view->priv->cursor))
	*path = btk_tree_row_reference_get_path (tree_view->priv->cursor);
      else
	*path = NULL;
    }

  if (focus_column)
    {
      *focus_column = tree_view->priv->focus_column;
    }
}

/**
 * btk_tree_view_set_cursor:
 * @tree_view: A #BtkTreeView
 * @path: A #BtkTreePath
 * @focus_column: (allow-none): A #BtkTreeViewColumn, or %NULL
 * @start_editing: %TRUE if the specified cell should start being edited.
 *
 * Sets the current keyboard focus to be at @path, and selects it.  This is
 * useful when you want to focus the user's attention on a particular row.  If
 * @focus_column is not %NULL, then focus is given to the column specified by 
 * it. Additionally, if @focus_column is specified, and @start_editing is 
 * %TRUE, then editing should be started in the specified cell.  
 * This function is often followed by @btk_widget_grab_focus (@tree_view) 
 * in order to give keyboard focus to the widget.  Please note that editing 
 * can only happen when the widget is realized.
 *
 * If @path is invalid for @model, the current cursor (if any) will be unset
 * and the function will return without failing.
 **/
void
btk_tree_view_set_cursor (BtkTreeView       *tree_view,
			  BtkTreePath       *path,
			  BtkTreeViewColumn *focus_column,
			  gboolean           start_editing)
{
  btk_tree_view_set_cursor_on_cell (tree_view, path, focus_column,
				    NULL, start_editing);
}

/**
 * btk_tree_view_set_cursor_on_cell:
 * @tree_view: A #BtkTreeView
 * @path: A #BtkTreePath
 * @focus_column: (allow-none): A #BtkTreeViewColumn, or %NULL
 * @focus_cell: (allow-none): A #BtkCellRenderer, or %NULL
 * @start_editing: %TRUE if the specified cell should start being edited.
 *
 * Sets the current keyboard focus to be at @path, and selects it.  This is
 * useful when you want to focus the user's attention on a particular row.  If
 * @focus_column is not %NULL, then focus is given to the column specified by
 * it. If @focus_column and @focus_cell are not %NULL, and @focus_column
 * contains 2 or more editable or activatable cells, then focus is given to
 * the cell specified by @focus_cell. Additionally, if @focus_column is
 * specified, and @start_editing is %TRUE, then editing should be started in
 * the specified cell.  This function is often followed by
 * @btk_widget_grab_focus (@tree_view) in order to give keyboard focus to the
 * widget.  Please note that editing can only happen when the widget is
 * realized.
 *
 * If @path is invalid for @model, the current cursor (if any) will be unset
 * and the function will return without failing.
 *
 * Since: 2.2
 **/
void
btk_tree_view_set_cursor_on_cell (BtkTreeView       *tree_view,
				  BtkTreePath       *path,
				  BtkTreeViewColumn *focus_column,
				  BtkCellRenderer   *focus_cell,
				  gboolean           start_editing)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (path != NULL);
  g_return_if_fail (focus_column == NULL || BTK_IS_TREE_VIEW_COLUMN (focus_column));

  if (!tree_view->priv->model)
    return;

  if (focus_cell)
    {
      g_return_if_fail (focus_column);
      g_return_if_fail (BTK_IS_CELL_RENDERER (focus_cell));
    }

  /* cancel the current editing, if it exists */
  if (tree_view->priv->edited_column &&
      tree_view->priv->edited_column->editable_widget)
    btk_tree_view_stop_editing (tree_view, TRUE);

  btk_tree_view_real_set_cursor (tree_view, path, TRUE, TRUE);

  if (focus_column && focus_column->visible)
    {
      GList *list;
      gboolean column_in_tree = FALSE;

      for (list = tree_view->priv->columns; list; list = list->next)
	if (list->data == focus_column)
	  {
	    column_in_tree = TRUE;
	    break;
	  }
      g_return_if_fail (column_in_tree);
      tree_view->priv->focus_column = focus_column;
      if (focus_cell)
	btk_tree_view_column_focus_cell (focus_column, focus_cell);
      if (start_editing)
	btk_tree_view_start_editing (tree_view, path);
    }
}

/**
 * btk_tree_view_get_bin_window:
 * @tree_view: A #BtkTreeView
 *
 * Returns the window that @tree_view renders to.
 * This is used primarily to compare to <literal>event->window</literal>
 * to confirm that the event on @tree_view is on the right window.
 *
 * Return value: (transfer none): A #BdkWindow, or %NULL when @tree_view
 *     hasn't been realized yet
 **/
BdkWindow *
btk_tree_view_get_bin_window (BtkTreeView *tree_view)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), NULL);

  return tree_view->priv->bin_window;
}

/**
 * btk_tree_view_get_path_at_pos:
 * @tree_view: A #BtkTreeView.
 * @x: The x position to be identified (relative to bin_window).
 * @y: The y position to be identified (relative to bin_window).
 * @path: (out) (allow-none): A pointer to a #BtkTreePath pointer to be filled in, or %NULL
 * @column: (out) (transfer none) (allow-none): A pointer to a #BtkTreeViewColumn pointer to be filled in, or %NULL
 * @cell_x: (out) (allow-none): A pointer where the X coordinate relative to the cell can be placed, or %NULL
 * @cell_y: (out) (allow-none): A pointer where the Y coordinate relative to the cell can be placed, or %NULL
 *
 * Finds the path at the point (@x, @y), relative to bin_window coordinates
 * (please see btk_tree_view_get_bin_window()).
 * That is, @x and @y are relative to an events coordinates. @x and @y must
 * come from an event on the @tree_view only where <literal>event->window ==
 * btk_tree_view_get_bin_window (<!-- -->)</literal>. It is primarily for
 * things like popup menus. If @path is non-%NULL, then it will be filled
 * with the #BtkTreePath at that point.  This path should be freed with
 * btk_tree_path_free().  If @column is non-%NULL, then it will be filled
 * with the column at that point.  @cell_x and @cell_y return the coordinates
 * relative to the cell background (i.e. the @background_area passed to
 * btk_cell_renderer_render()).  This function is only meaningful if
 * @tree_view is realized.  Therefore this function will always return %FALSE
 * if @tree_view is not realized or does not have a model.
 *
 * For converting widget coordinates (eg. the ones you get from
 * BtkWidget::query-tooltip), please see
 * btk_tree_view_convert_widget_to_bin_window_coords().
 *
 * Return value: %TRUE if a row exists at that coordinate.
 **/
gboolean
btk_tree_view_get_path_at_pos (BtkTreeView        *tree_view,
			       gint                x,
			       gint                y,
			       BtkTreePath       **path,
			       BtkTreeViewColumn **column,
                               gint               *cell_x,
                               gint               *cell_y)
{
  BtkRBTree *tree;
  BtkRBNode *node;
  gint y_offset;

  g_return_val_if_fail (tree_view != NULL, FALSE);

  if (path)
    *path = NULL;
  if (column)
    *column = NULL;

  if (tree_view->priv->bin_window == NULL)
    return FALSE;

  if (tree_view->priv->tree == NULL)
    return FALSE;

  if (x > tree_view->priv->hadjustment->upper)
    return FALSE;

  if (x < 0 || y < 0)
    return FALSE;

  if (column || cell_x)
    {
      BtkTreeViewColumn *tmp_column;
      BtkTreeViewColumn *last_column = NULL;
      GList *list;
      gint remaining_x = x;
      gboolean found = FALSE;
      gboolean rtl;

      rtl = (btk_widget_get_direction (BTK_WIDGET (tree_view)) == BTK_TEXT_DIR_RTL);
      for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
	   list;
	   list = (rtl ? list->prev : list->next))
	{
	  tmp_column = list->data;

	  if (tmp_column->visible == FALSE)
	    continue;

	  last_column = tmp_column;
	  if (remaining_x <= tmp_column->width)
	    {
              found = TRUE;

              if (column)
                *column = tmp_column;

              if (cell_x)
                *cell_x = remaining_x;

	      break;
	    }
	  remaining_x -= tmp_column->width;
	}

      /* If found is FALSE and there is a last_column, then it the remainder
       * space is in that area
       */
      if (!found)
        {
	  if (last_column)
	    {
	      if (column)
		*column = last_column;
	      
	      if (cell_x)
		*cell_x = last_column->width + remaining_x;
	    }
	  else
	    {
	      return FALSE;
	    }
	}
    }

  y_offset = _btk_rbtree_find_offset (tree_view->priv->tree,
				      TREE_WINDOW_Y_TO_RBTREE_Y (tree_view, y),
				      &tree, &node);

  if (tree == NULL)
    return FALSE;

  if (cell_y)
    *cell_y = y_offset;

  if (path)
    *path = _btk_tree_view_find_path (tree_view, tree, node);

  return TRUE;
}


/**
 * btk_tree_view_get_cell_area:
 * @tree_view: a #BtkTreeView
 * @path: (allow-none): a #BtkTreePath for the row, or %NULL to get only horizontal coordinates
 * @column: (allow-none): a #BtkTreeViewColumn for the column, or %NULL to get only vertical coordinates
 * @rect: (out): rectangle to fill with cell rect
 *
 * Fills the bounding rectangle in bin_window coordinates for the cell at the
 * row specified by @path and the column specified by @column.  If @path is
 * %NULL, or points to a path not currently displayed, the @y and @height fields
 * of the rectangle will be filled with 0. If @column is %NULL, the @x and @width
 * fields will be filled with 0.  The sum of all cell rects does not cover the
 * entire tree; there are extra pixels in between rows, for example. The
 * returned rectangle is equivalent to the @cell_area passed to
 * btk_cell_renderer_render().  This function is only valid if @tree_view is
 * realized.
 **/
void
btk_tree_view_get_cell_area (BtkTreeView        *tree_view,
                             BtkTreePath        *path,
                             BtkTreeViewColumn  *column,
                             BdkRectangle       *rect)
{
  BtkRBTree *tree = NULL;
  BtkRBNode *node = NULL;
  gint vertical_separator;
  gint horizontal_separator;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (column == NULL || BTK_IS_TREE_VIEW_COLUMN (column));
  g_return_if_fail (rect != NULL);
  g_return_if_fail (!column || column->tree_view == (BtkWidget *) tree_view);
  g_return_if_fail (btk_widget_get_realized (BTK_WIDGET (tree_view)));

  btk_widget_style_get (BTK_WIDGET (tree_view),
			"vertical-separator", &vertical_separator,
			"horizontal-separator", &horizontal_separator,
			NULL);

  rect->x = 0;
  rect->y = 0;
  rect->width = 0;
  rect->height = 0;

  if (column)
    {
      rect->x = column->button->allocation.x + horizontal_separator/2;
      rect->width = column->button->allocation.width - horizontal_separator;
    }

  if (path)
    {
      gboolean ret = _btk_tree_view_find_node (tree_view, path, &tree, &node);

      /* Get vertical coords */
      if ((!ret && tree == NULL) || ret)
	return;

      rect->y = CELL_FIRST_PIXEL (tree_view, tree, node, vertical_separator);
      rect->height = MAX (CELL_HEIGHT (node, vertical_separator), tree_view->priv->expander_size - vertical_separator);

      if (column &&
	  btk_tree_view_is_expander_column (tree_view, column))
	{
	  gint depth = btk_tree_path_get_depth (path);
	  gboolean rtl;

	  rtl = btk_widget_get_direction (BTK_WIDGET (tree_view)) == BTK_TEXT_DIR_RTL;

	  if (!rtl)
	    rect->x += (depth - 1) * tree_view->priv->level_indentation;
	  rect->width -= (depth - 1) * tree_view->priv->level_indentation;

	  if (TREE_VIEW_DRAW_EXPANDERS (tree_view))
	    {
	      if (!rtl)
		rect->x += depth * tree_view->priv->expander_size;
	      rect->width -= depth * tree_view->priv->expander_size;
	    }

	  rect->width = MAX (rect->width, 0);
	}
    }
}

/**
 * btk_tree_view_get_background_area:
 * @tree_view: a #BtkTreeView
 * @path: (allow-none): a #BtkTreePath for the row, or %NULL to get only horizontal coordinates
 * @column: (allow-none): a #BtkTreeViewColumn for the column, or %NULL to get only vertical coordiantes
 * @rect: (out): rectangle to fill with cell background rect
 *
 * Fills the bounding rectangle in bin_window coordinates for the cell at the
 * row specified by @path and the column specified by @column.  If @path is
 * %NULL, or points to a node not found in the tree, the @y and @height fields of
 * the rectangle will be filled with 0. If @column is %NULL, the @x and @width
 * fields will be filled with 0.  The returned rectangle is equivalent to the
 * @background_area passed to btk_cell_renderer_render().  These background
 * areas tile to cover the entire bin window.  Contrast with the @cell_area,
 * returned by btk_tree_view_get_cell_area(), which returns only the cell
 * itself, excluding surrounding borders and the tree expander area.
 *
 **/
void
btk_tree_view_get_background_area (BtkTreeView        *tree_view,
                                   BtkTreePath        *path,
                                   BtkTreeViewColumn  *column,
                                   BdkRectangle       *rect)
{
  BtkRBTree *tree = NULL;
  BtkRBNode *node = NULL;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (column == NULL || BTK_IS_TREE_VIEW_COLUMN (column));
  g_return_if_fail (rect != NULL);

  rect->x = 0;
  rect->y = 0;
  rect->width = 0;
  rect->height = 0;

  if (path)
    {
      /* Get vertical coords */

      if (!_btk_tree_view_find_node (tree_view, path, &tree, &node) &&
	  tree == NULL)
	return;

      rect->y = BACKGROUND_FIRST_PIXEL (tree_view, tree, node);

      rect->height = ROW_HEIGHT (tree_view, BACKGROUND_HEIGHT (node));
    }

  if (column)
    {
      gint x2 = 0;

      btk_tree_view_get_background_xrange (tree_view, tree, column, &rect->x, &x2);
      rect->width = x2 - rect->x;
    }
}

/**
 * btk_tree_view_get_visible_rect:
 * @tree_view: a #BtkTreeView
 * @visible_rect: (out): rectangle to fill
 *
 * Fills @visible_rect with the currently-visible rebunnyion of the
 * buffer, in tree coordinates. Convert to bin_window coordinates with
 * btk_tree_view_convert_tree_to_bin_window_coords().
 * Tree coordinates start at 0,0 for row 0 of the tree, and cover the entire
 * scrollable area of the tree.
 **/
void
btk_tree_view_get_visible_rect (BtkTreeView  *tree_view,
                                BdkRectangle *visible_rect)
{
  BtkWidget *widget;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  widget = BTK_WIDGET (tree_view);

  if (visible_rect)
    {
      visible_rect->x = tree_view->priv->hadjustment->value;
      visible_rect->y = tree_view->priv->vadjustment->value;
      visible_rect->width = widget->allocation.width;
      visible_rect->height = widget->allocation.height - TREE_VIEW_HEADER_HEIGHT (tree_view);
    }
}

/**
 * btk_tree_view_widget_to_tree_coords:
 * @tree_view: a #BtkTreeView
 * @wx: X coordinate relative to bin_window
 * @wy: Y coordinate relative to bin_window
 * @tx: return location for tree X coordinate
 * @ty: return location for tree Y coordinate
 *
 * Converts bin_window coordinates to coordinates for the
 * tree (the full scrollable area of the tree).
 *
 * Deprecated: 2.12: Due to historial reasons the name of this function is
 * incorrect.  For converting coordinates relative to the widget to
 * bin_window coordinates, please see
 * btk_tree_view_convert_widget_to_bin_window_coords().
 *
 **/
void
btk_tree_view_widget_to_tree_coords (BtkTreeView *tree_view,
				      gint         wx,
				      gint         wy,
				      gint        *tx,
				      gint        *ty)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  if (tx)
    *tx = wx + tree_view->priv->hadjustment->value;
  if (ty)
    *ty = wy + tree_view->priv->dy;
}

/**
 * btk_tree_view_tree_to_widget_coords:
 * @tree_view: a #BtkTreeView
 * @tx: tree X coordinate
 * @ty: tree Y coordinate
 * @wx: return location for X coordinate relative to bin_window
 * @wy: return location for Y coordinate relative to bin_window
 *
 * Converts tree coordinates (coordinates in full scrollable area of the tree)
 * to bin_window coordinates.
 *
 * Deprecated: 2.12: Due to historial reasons the name of this function is
 * incorrect.  For converting bin_window coordinates to coordinates relative
 * to bin_window, please see
 * btk_tree_view_convert_bin_window_to_widget_coords().
 *
 **/
void
btk_tree_view_tree_to_widget_coords (BtkTreeView *tree_view,
                                     gint         tx,
                                     gint         ty,
                                     gint        *wx,
                                     gint        *wy)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  if (wx)
    *wx = tx - tree_view->priv->hadjustment->value;
  if (wy)
    *wy = ty - tree_view->priv->dy;
}


/**
 * btk_tree_view_convert_widget_to_tree_coords:
 * @tree_view: a #BtkTreeView
 * @wx: X coordinate relative to the widget
 * @wy: Y coordinate relative to the widget
 * @tx: (out): return location for tree X coordinate
 * @ty: (out): return location for tree Y coordinate
 *
 * Converts widget coordinates to coordinates for the
 * tree (the full scrollable area of the tree).
 *
 * Since: 2.12
 **/
void
btk_tree_view_convert_widget_to_tree_coords (BtkTreeView *tree_view,
                                             gint         wx,
                                             gint         wy,
                                             gint        *tx,
                                             gint        *ty)
{
  gint x, y;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  btk_tree_view_convert_widget_to_bin_window_coords (tree_view,
						     wx, wy,
						     &x, &y);
  btk_tree_view_convert_bin_window_to_tree_coords (tree_view,
						   x, y,
						   tx, ty);
}

/**
 * btk_tree_view_convert_tree_to_widget_coords:
 * @tree_view: a #BtkTreeView
 * @tx: X coordinate relative to the tree
 * @ty: Y coordinate relative to the tree
 * @wx: (out): return location for widget X coordinate
 * @wy: (out): return location for widget Y coordinate
 *
 * Converts tree coordinates (coordinates in full scrollable area of the tree)
 * to widget coordinates.
 *
 * Since: 2.12
 **/
void
btk_tree_view_convert_tree_to_widget_coords (BtkTreeView *tree_view,
                                             gint         tx,
                                             gint         ty,
                                             gint        *wx,
                                             gint        *wy)
{
  gint x, y;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  btk_tree_view_convert_tree_to_bin_window_coords (tree_view,
						   tx, ty,
						   &x, &y);
  btk_tree_view_convert_bin_window_to_widget_coords (tree_view,
						     x, y,
						     wx, wy);
}

/**
 * btk_tree_view_convert_widget_to_bin_window_coords:
 * @tree_view: a #BtkTreeView
 * @wx: X coordinate relative to the widget
 * @wy: Y coordinate relative to the widget
 * @bx: (out): return location for bin_window X coordinate
 * @by: (out): return location for bin_window Y coordinate
 *
 * Converts widget coordinates to coordinates for the bin_window
 * (see btk_tree_view_get_bin_window()).
 *
 * Since: 2.12
 **/
void
btk_tree_view_convert_widget_to_bin_window_coords (BtkTreeView *tree_view,
                                                   gint         wx,
                                                   gint         wy,
                                                   gint        *bx,
                                                   gint        *by)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  if (bx)
    *bx = wx + tree_view->priv->hadjustment->value;
  if (by)
    *by = wy - TREE_VIEW_HEADER_HEIGHT (tree_view);
}

/**
 * btk_tree_view_convert_bin_window_to_widget_coords:
 * @tree_view: a #BtkTreeView
 * @bx: bin_window X coordinate
 * @by: bin_window Y coordinate
 * @wx: (out): return location for widget X coordinate
 * @wy: (out): return location for widget Y coordinate
 *
 * Converts bin_window coordinates (see btk_tree_view_get_bin_window())
 * to widget relative coordinates.
 *
 * Since: 2.12
 **/
void
btk_tree_view_convert_bin_window_to_widget_coords (BtkTreeView *tree_view,
                                                   gint         bx,
                                                   gint         by,
                                                   gint        *wx,
                                                   gint        *wy)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  if (wx)
    *wx = bx - tree_view->priv->hadjustment->value;
  if (wy)
    *wy = by + TREE_VIEW_HEADER_HEIGHT (tree_view);
}

/**
 * btk_tree_view_convert_tree_to_bin_window_coords:
 * @tree_view: a #BtkTreeView
 * @tx: tree X coordinate
 * @ty: tree Y coordinate
 * @bx: (out): return location for X coordinate relative to bin_window
 * @by: (out): return location for Y coordinate relative to bin_window
 *
 * Converts tree coordinates (coordinates in full scrollable area of the tree)
 * to bin_window coordinates.
 *
 * Since: 2.12
 **/
void
btk_tree_view_convert_tree_to_bin_window_coords (BtkTreeView *tree_view,
                                                 gint         tx,
                                                 gint         ty,
                                                 gint        *bx,
                                                 gint        *by)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  if (bx)
    *bx = tx;
  if (by)
    *by = ty - tree_view->priv->dy;
}

/**
 * btk_tree_view_convert_bin_window_to_tree_coords:
 * @tree_view: a #BtkTreeView
 * @bx: X coordinate relative to bin_window
 * @by: Y coordinate relative to bin_window
 * @tx: (out): return location for tree X coordinate
 * @ty: (out): return location for tree Y coordinate
 *
 * Converts bin_window coordinates to coordinates for the
 * tree (the full scrollable area of the tree).
 *
 * Since: 2.12
 **/
void
btk_tree_view_convert_bin_window_to_tree_coords (BtkTreeView *tree_view,
                                                 gint         bx,
                                                 gint         by,
                                                 gint        *tx,
                                                 gint        *ty)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  if (tx)
    *tx = bx;
  if (ty)
    *ty = by + tree_view->priv->dy;
}



/**
 * btk_tree_view_get_visible_range:
 * @tree_view: A #BtkTreeView
 * @start_path: (out) (allow-none): Return location for start of rebunnyion,
 *              or %NULL.
 * @end_path: (out) (allow-none): Return location for end of rebunnyion, or %NULL.
 *
 * Sets @start_path and @end_path to be the first and last visible path.
 * Note that there may be invisible paths in between.
 *
 * The paths should be freed with btk_tree_path_free() after use.
 *
 * Returns: %TRUE, if valid paths were placed in @start_path and @end_path.
 *
 * Since: 2.8
 **/
gboolean
btk_tree_view_get_visible_range (BtkTreeView  *tree_view,
                                 BtkTreePath **start_path,
                                 BtkTreePath **end_path)
{
  BtkRBTree *tree;
  BtkRBNode *node;
  gboolean retval;
  
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), FALSE);

  if (!tree_view->priv->tree)
    return FALSE;

  retval = TRUE;

  if (start_path)
    {
      _btk_rbtree_find_offset (tree_view->priv->tree,
                               TREE_WINDOW_Y_TO_RBTREE_Y (tree_view, 0),
                               &tree, &node);
      if (node)
        *start_path = _btk_tree_view_find_path (tree_view, tree, node);
      else
        retval = FALSE;
    }

  if (end_path)
    {
      gint y;

      if (tree_view->priv->height < tree_view->priv->vadjustment->page_size)
        y = tree_view->priv->height - 1;
      else
        y = TREE_WINDOW_Y_TO_RBTREE_Y (tree_view, tree_view->priv->vadjustment->page_size) - 1;

      _btk_rbtree_find_offset (tree_view->priv->tree, y, &tree, &node);
      if (node)
        *end_path = _btk_tree_view_find_path (tree_view, tree, node);
      else
        retval = FALSE;
    }

  return retval;
}

static void
unset_reorderable (BtkTreeView *tree_view)
{
  if (tree_view->priv->reorderable)
    {
      tree_view->priv->reorderable = FALSE;
      g_object_notify (B_OBJECT (tree_view), "reorderable");
    }
}

/**
 * btk_tree_view_enable_model_drag_source:
 * @tree_view: a #BtkTreeView
 * @start_button_mask: Mask of allowed buttons to start drag
 * @targets: (array length=n_targets): the table of targets that the drag will support
 * @n_targets: the number of items in @targets
 * @actions: the bitmask of possible actions for a drag from this
 *    widget
 *
 * Turns @tree_view into a drag source for automatic DND. Calling this
 * method sets #BtkTreeView:reorderable to %FALSE.
 **/
void
btk_tree_view_enable_model_drag_source (BtkTreeView              *tree_view,
					BdkModifierType           start_button_mask,
					const BtkTargetEntry     *targets,
					gint                      n_targets,
					BdkDragAction             actions)
{
  TreeViewDragInfo *di;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  btk_drag_source_set (BTK_WIDGET (tree_view),
		       0,
		       targets,
		       n_targets,
		       actions);

  di = ensure_info (tree_view);

  di->start_button_mask = start_button_mask;
  di->source_actions = actions;
  di->source_set = TRUE;

  unset_reorderable (tree_view);
}

/**
 * btk_tree_view_enable_model_drag_dest:
 * @tree_view: a #BtkTreeView
 * @targets: (array length=n_targets): the table of targets that the drag will support
 * @n_targets: the number of items in @targets
 * @actions: the bitmask of possible actions for a drag from this
 *    widget
 * 
 * Turns @tree_view into a drop destination for automatic DND. Calling
 * this method sets #BtkTreeView:reorderable to %FALSE.
 **/
void
btk_tree_view_enable_model_drag_dest (BtkTreeView              *tree_view,
				      const BtkTargetEntry     *targets,
				      gint                      n_targets,
				      BdkDragAction             actions)
{
  TreeViewDragInfo *di;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  btk_drag_dest_set (BTK_WIDGET (tree_view),
                     0,
                     targets,
                     n_targets,
                     actions);

  di = ensure_info (tree_view);
  di->dest_set = TRUE;

  unset_reorderable (tree_view);
}

/**
 * btk_tree_view_unset_rows_drag_source:
 * @tree_view: a #BtkTreeView
 *
 * Undoes the effect of
 * btk_tree_view_enable_model_drag_source(). Calling this method sets
 * #BtkTreeView:reorderable to %FALSE.
 **/
void
btk_tree_view_unset_rows_drag_source (BtkTreeView *tree_view)
{
  TreeViewDragInfo *di;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  di = get_info (tree_view);

  if (di)
    {
      if (di->source_set)
        {
          btk_drag_source_unset (BTK_WIDGET (tree_view));
          di->source_set = FALSE;
        }

      if (!di->dest_set && !di->source_set)
        remove_info (tree_view);
    }
  
  unset_reorderable (tree_view);
}

/**
 * btk_tree_view_unset_rows_drag_dest:
 * @tree_view: a #BtkTreeView
 *
 * Undoes the effect of
 * btk_tree_view_enable_model_drag_dest(). Calling this method sets
 * #BtkTreeView:reorderable to %FALSE.
 **/
void
btk_tree_view_unset_rows_drag_dest (BtkTreeView *tree_view)
{
  TreeViewDragInfo *di;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  di = get_info (tree_view);

  if (di)
    {
      if (di->dest_set)
        {
          btk_drag_dest_unset (BTK_WIDGET (tree_view));
          di->dest_set = FALSE;
        }

      if (!di->dest_set && !di->source_set)
        remove_info (tree_view);
    }

  unset_reorderable (tree_view);
}

/**
 * btk_tree_view_set_drag_dest_row:
 * @tree_view: a #BtkTreeView
 * @path: (allow-none): The path of the row to highlight, or %NULL.
 * @pos: Specifies whether to drop before, after or into the row
 * 
 * Sets the row that is highlighted for feedback.
 **/
void
btk_tree_view_set_drag_dest_row (BtkTreeView            *tree_view,
                                 BtkTreePath            *path,
                                 BtkTreeViewDropPosition pos)
{
  BtkTreePath *current_dest;

  /* Note; this function is exported to allow a custom DND
   * implementation, so it can't touch TreeViewDragInfo
   */

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  current_dest = NULL;

  if (tree_view->priv->drag_dest_row)
    {
      current_dest = btk_tree_row_reference_get_path (tree_view->priv->drag_dest_row);
      btk_tree_row_reference_free (tree_view->priv->drag_dest_row);
    }

  /* special case a drop on an empty model */
  tree_view->priv->empty_view_drop = 0;

  if (pos == BTK_TREE_VIEW_DROP_BEFORE && path
      && btk_tree_path_get_depth (path) == 1
      && btk_tree_path_get_indices (path)[0] == 0)
    {
      gint n_children;

      n_children = btk_tree_model_iter_n_children (tree_view->priv->model,
                                                   NULL);

      if (!n_children)
        tree_view->priv->empty_view_drop = 1;
    }

  tree_view->priv->drag_dest_pos = pos;

  if (path)
    {
      tree_view->priv->drag_dest_row =
        btk_tree_row_reference_new_proxy (B_OBJECT (tree_view), tree_view->priv->model, path);
      btk_tree_view_queue_draw_path (tree_view, path, NULL);
    }
  else
    tree_view->priv->drag_dest_row = NULL;

  if (current_dest)
    {
      BtkRBTree *tree, *new_tree;
      BtkRBNode *node, *new_node;

      _btk_tree_view_find_node (tree_view, current_dest, &tree, &node);
      _btk_tree_view_queue_draw_node (tree_view, tree, node, NULL);

      if (tree && node)
	{
	  _btk_rbtree_next_full (tree, node, &new_tree, &new_node);
	  if (new_tree && new_node)
	    _btk_tree_view_queue_draw_node (tree_view, new_tree, new_node, NULL);

	  _btk_rbtree_prev_full (tree, node, &new_tree, &new_node);
	  if (new_tree && new_node)
	    _btk_tree_view_queue_draw_node (tree_view, new_tree, new_node, NULL);
	}
      btk_tree_path_free (current_dest);
    }
}

/**
 * btk_tree_view_get_drag_dest_row:
 * @tree_view: a #BtkTreeView
 * @path: (out) (allow-none): Return location for the path of the highlighted row, or %NULL.
 * @pos: (out) (allow-none): Return location for the drop position, or %NULL
 * 
 * Gets information about the row that is highlighted for feedback.
 **/
void
btk_tree_view_get_drag_dest_row (BtkTreeView              *tree_view,
                                 BtkTreePath             **path,
                                 BtkTreeViewDropPosition  *pos)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  if (path)
    {
      if (tree_view->priv->drag_dest_row)
        *path = btk_tree_row_reference_get_path (tree_view->priv->drag_dest_row);
      else
        {
          if (tree_view->priv->empty_view_drop)
            *path = btk_tree_path_new_from_indices (0, -1);
          else
            *path = NULL;
        }
    }

  if (pos)
    *pos = tree_view->priv->drag_dest_pos;
}

/**
 * btk_tree_view_get_dest_row_at_pos:
 * @tree_view: a #BtkTreeView
 * @drag_x: the position to determine the destination row for
 * @drag_y: the position to determine the destination row for
 * @path: (out) (allow-none): Return location for the path of the highlighted row, or %NULL.
 * @pos: (out) (allow-none): Return location for the drop position, or %NULL
 * 
 * Determines the destination row for a given position.  @drag_x and
 * @drag_y are expected to be in widget coordinates.  This function is only
 * meaningful if @tree_view is realized.  Therefore this function will always
 * return %FALSE if @tree_view is not realized or does not have a model.
 * 
 * Return value: whether there is a row at the given position, %TRUE if this
 * is indeed the case.
 **/
gboolean
btk_tree_view_get_dest_row_at_pos (BtkTreeView             *tree_view,
                                   gint                     drag_x,
                                   gint                     drag_y,
                                   BtkTreePath            **path,
                                   BtkTreeViewDropPosition *pos)
{
  gint cell_y;
  gint bin_x, bin_y;
  gdouble offset_into_row;
  gdouble third;
  BdkRectangle cell;
  BtkTreeViewColumn *column = NULL;
  BtkTreePath *tmp_path = NULL;

  /* Note; this function is exported to allow a custom DND
   * implementation, so it can't touch TreeViewDragInfo
   */

  g_return_val_if_fail (tree_view != NULL, FALSE);
  g_return_val_if_fail (drag_x >= 0, FALSE);
  g_return_val_if_fail (drag_y >= 0, FALSE);

  if (path)
    *path = NULL;

  if (tree_view->priv->bin_window == NULL)
    return FALSE;

  if (tree_view->priv->tree == NULL)
    return FALSE;

  /* If in the top third of a row, we drop before that row; if
   * in the bottom third, drop after that row; if in the middle,
   * and the row has children, drop into the row.
   */
  btk_tree_view_convert_widget_to_bin_window_coords (tree_view, drag_x, drag_y,
						     &bin_x, &bin_y);

  if (!btk_tree_view_get_path_at_pos (tree_view,
				      bin_x,
				      bin_y,
                                      &tmp_path,
                                      &column,
                                      NULL,
                                      &cell_y))
    return FALSE;

  btk_tree_view_get_background_area (tree_view, tmp_path, column,
                                     &cell);

  offset_into_row = cell_y;

  if (path)
    *path = tmp_path;
  else
    btk_tree_path_free (tmp_path);

  tmp_path = NULL;

  third = cell.height / 3.0;

  if (pos)
    {
      if (offset_into_row < third)
        {
          *pos = BTK_TREE_VIEW_DROP_BEFORE;
        }
      else if (offset_into_row < (cell.height / 2.0))
        {
          *pos = BTK_TREE_VIEW_DROP_INTO_OR_BEFORE;
        }
      else if (offset_into_row < third * 2.0)
        {
          *pos = BTK_TREE_VIEW_DROP_INTO_OR_AFTER;
        }
      else
        {
          *pos = BTK_TREE_VIEW_DROP_AFTER;
        }
    }

  return TRUE;
}



/* KEEP IN SYNC WITH BTK_TREE_VIEW_BIN_EXPOSE */
/**
 * btk_tree_view_create_row_drag_icon:
 * @tree_view: a #BtkTreeView
 * @path: a #BtkTreePath in @tree_view
 *
 * Creates a #BdkPixmap representation of the row at @path.
 * This image is used for a drag icon.
 *
 * Return value: (transfer none): a newly-allocated pixmap of the drag icon.
 **/
BdkPixmap *
btk_tree_view_create_row_drag_icon (BtkTreeView  *tree_view,
                                    BtkTreePath  *path)
{
  BtkTreeIter   iter;
  BtkRBTree    *tree;
  BtkRBNode    *node;
  gint cell_offset;
  GList *list;
  BdkRectangle background_area;
  BdkRectangle expose_area;
  BtkWidget *widget;
  gint depth;
  /* start drawing inside the black outline */
  gint x = 1, y = 1;
  BdkDrawable *drawable;
  gint bin_window_width;
  gboolean is_separator = FALSE;
  gboolean rtl;
  bairo_t *cr;

  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), NULL);
  g_return_val_if_fail (path != NULL, NULL);

  widget = BTK_WIDGET (tree_view);

  if (!btk_widget_get_realized (widget))
    return NULL;

  depth = btk_tree_path_get_depth (path);

  _btk_tree_view_find_node (tree_view,
                            path,
                            &tree,
                            &node);

  if (tree == NULL)
    return NULL;

  if (!btk_tree_model_get_iter (tree_view->priv->model,
                                &iter,
                                path))
    return NULL;
  
  is_separator = row_is_separator (tree_view, &iter, NULL);

  cell_offset = x;

  background_area.y = y;
  background_area.height = ROW_HEIGHT (tree_view, BACKGROUND_HEIGHT (node));

  bin_window_width = bdk_window_get_width (tree_view->priv->bin_window);

  drawable = bdk_pixmap_new (tree_view->priv->bin_window,
                             bin_window_width + 2,
                             background_area.height + 2,
                             -1);

  expose_area.x = 0;
  expose_area.y = 0;
  expose_area.width = bin_window_width + 2;
  expose_area.height = background_area.height + 2;

  cr = bdk_bairo_create (drawable);
  bdk_bairo_set_source_color (cr, &widget->style->base [btk_widget_get_state (widget)]);
  bairo_paint (cr);

  rtl = btk_widget_get_direction (BTK_WIDGET (tree_view)) == BTK_TEXT_DIR_RTL;

  for (list = (rtl ? g_list_last (tree_view->priv->columns) : g_list_first (tree_view->priv->columns));
      list;
      list = (rtl ? list->prev : list->next))
    {
      BtkTreeViewColumn *column = list->data;
      BdkRectangle cell_area;
      gint vertical_separator;

      if (!column->visible)
        continue;

      btk_tree_view_column_cell_set_cell_data (column, tree_view->priv->model, &iter,
					       BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_PARENT),
					       node->children?TRUE:FALSE);

      background_area.x = cell_offset;
      background_area.width = column->width;

      btk_widget_style_get (widget,
			    "vertical-separator", &vertical_separator,
			    NULL);

      cell_area = background_area;

      cell_area.y += vertical_separator / 2;
      cell_area.height -= vertical_separator;

      if (btk_tree_view_is_expander_column (tree_view, column))
        {
	  if (!rtl)
	    cell_area.x += (depth - 1) * tree_view->priv->level_indentation;
	  cell_area.width -= (depth - 1) * tree_view->priv->level_indentation;

          if (TREE_VIEW_DRAW_EXPANDERS(tree_view))
	    {
	      if (!rtl)
		cell_area.x += depth * tree_view->priv->expander_size;
	      cell_area.width -= depth * tree_view->priv->expander_size;
	    }
        }

      if (btk_tree_view_column_cell_is_visible (column))
	{
	  if (is_separator)
	    btk_paint_hline (widget->style,
			     drawable,
			     BTK_STATE_NORMAL,
			     &cell_area,
			     widget,
			     NULL,
			     cell_area.x,
			     cell_area.x + cell_area.width,
			     cell_area.y + cell_area.height / 2);
	  else
	    _btk_tree_view_column_cell_render (column,
					       drawable,
					       &background_area,
					       &cell_area,
					       &expose_area,
					       0);
	}
      cell_offset += column->width;
    }

  bairo_set_source_rgb (cr, 0, 0, 0);
  bairo_rectangle (cr, 
                   0.5, 0.5, 
                   bin_window_width + 1,
                   background_area.height + 1);
  bairo_set_line_width (cr, 1.0);
  bairo_stroke (cr);

  bairo_destroy (cr);

  return drawable;
}


/**
 * btk_tree_view_set_destroy_count_func:
 * @tree_view: A #BtkTreeView
 * @func: (allow-none): Function to be called when a view row is destroyed, or %NULL
 * @data: (allow-none): User data to be passed to @func, or %NULL
 * @destroy: (allow-none): Destroy notifier for @data, or %NULL
 *
 * This function should almost never be used.  It is meant for private use by
 * BATK for determining the number of visible children that are removed when the
 * user collapses a row, or a row is deleted.
 **/
void
btk_tree_view_set_destroy_count_func (BtkTreeView             *tree_view,
				      BtkTreeDestroyCountFunc  func,
				      gpointer                 data,
				      GDestroyNotify           destroy)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  if (tree_view->priv->destroy_count_destroy)
    tree_view->priv->destroy_count_destroy (tree_view->priv->destroy_count_data);

  tree_view->priv->destroy_count_func = func;
  tree_view->priv->destroy_count_data = data;
  tree_view->priv->destroy_count_destroy = destroy;
}


/*
 * Interactive search
 */

/**
 * btk_tree_view_set_enable_search:
 * @tree_view: A #BtkTreeView
 * @enable_search: %TRUE, if the user can search interactively
 *
 * If @enable_search is set, then the user can type in text to search through
 * the tree interactively (this is sometimes called "typeahead find").
 * 
 * Note that even if this is %FALSE, the user can still initiate a search 
 * using the "start-interactive-search" key binding.
 */
void
btk_tree_view_set_enable_search (BtkTreeView *tree_view,
				 gboolean     enable_search)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  enable_search = !!enable_search;
  
  if (tree_view->priv->enable_search != enable_search)
    {
       tree_view->priv->enable_search = enable_search;
       g_object_notify (B_OBJECT (tree_view), "enable-search");
    }
}

/**
 * btk_tree_view_get_enable_search:
 * @tree_view: A #BtkTreeView
 *
 * Returns whether or not the tree allows to start interactive searching 
 * by typing in text.
 *
 * Return value: whether or not to let the user search interactively
 */
gboolean
btk_tree_view_get_enable_search (BtkTreeView *tree_view)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), FALSE);

  return tree_view->priv->enable_search;
}


/**
 * btk_tree_view_get_search_column:
 * @tree_view: A #BtkTreeView
 *
 * Gets the column searched on by the interactive search code.
 *
 * Return value: the column the interactive search code searches in.
 */
gint
btk_tree_view_get_search_column (BtkTreeView *tree_view)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), -1);

  return (tree_view->priv->search_column);
}

/**
 * btk_tree_view_set_search_column:
 * @tree_view: A #BtkTreeView
 * @column: the column of the model to search in, or -1 to disable searching
 *
 * Sets @column as the column where the interactive search code should
 * search in for the current model. 
 * 
 * If the search column is set, users can use the "start-interactive-search"
 * key binding to bring up search popup. The enable-search property controls
 * whether simply typing text will also start an interactive search.
 *
 * Note that @column refers to a column of the current model. The search 
 * column is reset to -1 when the model is changed.
 */
void
btk_tree_view_set_search_column (BtkTreeView *tree_view,
				 gint         column)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (column >= -1);

  if (tree_view->priv->search_column == column)
    return;

  tree_view->priv->search_column = column;
  g_object_notify (B_OBJECT (tree_view), "search-column");
}

/**
 * btk_tree_view_get_search_equal_func:
 * @tree_view: A #BtkTreeView
 *
 * Returns the compare function currently in use.
 *
 * Return value: the currently used compare function for the search code.
 */

BtkTreeViewSearchEqualFunc
btk_tree_view_get_search_equal_func (BtkTreeView *tree_view)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), NULL);

  return tree_view->priv->search_equal_func;
}

/**
 * btk_tree_view_set_search_equal_func:
 * @tree_view: A #BtkTreeView
 * @search_equal_func: the compare function to use during the search
 * @search_user_data: (allow-none): user data to pass to @search_equal_func, or %NULL
 * @search_destroy: (allow-none): Destroy notifier for @search_user_data, or %NULL
 *
 * Sets the compare function for the interactive search capabilities; note
 * that somewhat like strcmp() returning 0 for equality
 * #BtkTreeViewSearchEqualFunc returns %FALSE on matches.
 **/
void
btk_tree_view_set_search_equal_func (BtkTreeView                *tree_view,
				     BtkTreeViewSearchEqualFunc  search_equal_func,
				     gpointer                    search_user_data,
				     GDestroyNotify              search_destroy)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (search_equal_func != NULL);

  if (tree_view->priv->search_destroy)
    tree_view->priv->search_destroy (tree_view->priv->search_user_data);

  tree_view->priv->search_equal_func = search_equal_func;
  tree_view->priv->search_user_data = search_user_data;
  tree_view->priv->search_destroy = search_destroy;
  if (tree_view->priv->search_equal_func == NULL)
    tree_view->priv->search_equal_func = btk_tree_view_search_equal_func;
}

/**
 * btk_tree_view_get_search_entry:
 * @tree_view: A #BtkTreeView
 *
 * Returns the #BtkEntry which is currently in use as interactive search
 * entry for @tree_view.  In case the built-in entry is being used, %NULL
 * will be returned.
 *
 * Return value: (transfer none): the entry currently in use as search entry.
 *
 * Since: 2.10
 */
BtkEntry *
btk_tree_view_get_search_entry (BtkTreeView *tree_view)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), NULL);

  if (tree_view->priv->search_custom_entry_set)
    return BTK_ENTRY (tree_view->priv->search_entry);

  return NULL;
}

/**
 * btk_tree_view_set_search_entry:
 * @tree_view: A #BtkTreeView
 * @entry: (allow-none): the entry the interactive search code of @tree_view should use or %NULL
 *
 * Sets the entry which the interactive search code will use for this
 * @tree_view.  This is useful when you want to provide a search entry
 * in our interface at all time at a fixed position.  Passing %NULL for
 * @entry will make the interactive search code use the built-in popup
 * entry again.
 *
 * Since: 2.10
 */
void
btk_tree_view_set_search_entry (BtkTreeView *tree_view,
				BtkEntry    *entry)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (entry == NULL || BTK_IS_ENTRY (entry));

  if (tree_view->priv->search_custom_entry_set)
    {
      if (tree_view->priv->search_entry_changed_id)
        {
	  g_signal_handler_disconnect (tree_view->priv->search_entry,
				       tree_view->priv->search_entry_changed_id);
	  tree_view->priv->search_entry_changed_id = 0;
	}
      g_signal_handlers_disconnect_by_func (tree_view->priv->search_entry,
					    G_CALLBACK (btk_tree_view_search_key_press_event),
					    tree_view);

      g_object_unref (tree_view->priv->search_entry);
    }
  else if (tree_view->priv->search_window)
    {
      btk_widget_destroy (tree_view->priv->search_window);

      tree_view->priv->search_window = NULL;
    }

  if (entry)
    {
      tree_view->priv->search_entry = g_object_ref (entry);
      tree_view->priv->search_custom_entry_set = TRUE;

      if (tree_view->priv->search_entry_changed_id == 0)
        {
          tree_view->priv->search_entry_changed_id =
	    g_signal_connect (tree_view->priv->search_entry, "changed",
			      G_CALLBACK (btk_tree_view_search_init),
			      tree_view);
	}
      
        g_signal_connect (tree_view->priv->search_entry, "key-press-event",
		          G_CALLBACK (btk_tree_view_search_key_press_event),
		          tree_view);

	btk_tree_view_search_init (tree_view->priv->search_entry, tree_view);
    }
  else
    {
      tree_view->priv->search_entry = NULL;
      tree_view->priv->search_custom_entry_set = FALSE;
    }
}

/**
 * btk_tree_view_set_search_position_func:
 * @tree_view: A #BtkTreeView
 * @func: (allow-none): the function to use to position the search dialog, or %NULL
 *    to use the default search position function
 * @data: (allow-none): user data to pass to @func, or %NULL
 * @destroy: (allow-none): Destroy notifier for @data, or %NULL
 *
 * Sets the function to use when positioning the search dialog.
 *
 * Since: 2.10
 **/
void
btk_tree_view_set_search_position_func (BtkTreeView                   *tree_view,
				        BtkTreeViewSearchPositionFunc  func,
				        gpointer                       user_data,
				        GDestroyNotify                 destroy)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  if (tree_view->priv->search_position_destroy)
    tree_view->priv->search_position_destroy (tree_view->priv->search_position_user_data);

  tree_view->priv->search_position_func = func;
  tree_view->priv->search_position_user_data = user_data;
  tree_view->priv->search_position_destroy = destroy;
  if (tree_view->priv->search_position_func == NULL)
    tree_view->priv->search_position_func = btk_tree_view_search_position_func;
}

/**
 * btk_tree_view_get_search_position_func:
 * @tree_view: A #BtkTreeView
 *
 * Returns the positioning function currently in use.
 *
 * Return value: the currently used function for positioning the search dialog.
 *
 * Since: 2.10
 */
BtkTreeViewSearchPositionFunc
btk_tree_view_get_search_position_func (BtkTreeView *tree_view)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), NULL);

  return tree_view->priv->search_position_func;
}


static void
btk_tree_view_search_dialog_hide (BtkWidget   *search_dialog,
				  BtkTreeView *tree_view)
{
  if (tree_view->priv->disable_popdown)
    return;

  if (tree_view->priv->search_entry_changed_id)
    {
      g_signal_handler_disconnect (tree_view->priv->search_entry,
				   tree_view->priv->search_entry_changed_id);
      tree_view->priv->search_entry_changed_id = 0;
    }
  if (tree_view->priv->typeselect_flush_timeout)
    {
      g_source_remove (tree_view->priv->typeselect_flush_timeout);
      tree_view->priv->typeselect_flush_timeout = 0;
    }
	
  if (btk_widget_get_visible (search_dialog))
    {
      /* send focus-in event */
      send_focus_change (BTK_WIDGET (tree_view->priv->search_entry), FALSE);
      btk_widget_hide (search_dialog);
      btk_entry_set_text (BTK_ENTRY (tree_view->priv->search_entry), "");
      send_focus_change (BTK_WIDGET (tree_view), TRUE);
    }
}

static void
btk_tree_view_search_position_func (BtkTreeView *tree_view,
				    BtkWidget   *search_dialog,
				    gpointer     user_data)
{
  gint x, y;
  gint tree_x, tree_y;
  gint tree_width, tree_height;
  BdkWindow *tree_window = BTK_WIDGET (tree_view)->window;
  BdkScreen *screen = bdk_window_get_screen (tree_window);
  BtkRequisition requisition;
  gint monitor_num;
  BdkRectangle monitor;

  monitor_num = bdk_screen_get_monitor_at_window (screen, tree_window);
  bdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

  btk_widget_realize (search_dialog);

  bdk_window_get_origin (tree_window, &tree_x, &tree_y);
  tree_width = bdk_window_get_width (tree_window);
  tree_height = bdk_window_get_height (tree_window);
  btk_widget_size_request (search_dialog, &requisition);

  if (tree_x + tree_width > bdk_screen_get_width (screen))
    x = bdk_screen_get_width (screen) - requisition.width;
  else if (tree_x + tree_width - requisition.width < 0)
    x = 0;
  else
    x = tree_x + tree_width - requisition.width;

  if (tree_y + tree_height + requisition.height > bdk_screen_get_height (screen))
    y = bdk_screen_get_height (screen) - requisition.height;
  else if (tree_y + tree_height < 0) /* isn't really possible ... */
    y = 0;
  else
    y = tree_y + tree_height;

  btk_window_move (BTK_WINDOW (search_dialog), x, y);
}

static void
btk_tree_view_search_disable_popdown (BtkEntry *entry,
				      BtkMenu  *menu,
				      gpointer  data)
{
  BtkTreeView *tree_view = (BtkTreeView *)data;

  tree_view->priv->disable_popdown = 1;
  g_signal_connect (menu, "hide",
		    G_CALLBACK (btk_tree_view_search_enable_popdown), data);
}

/* Because we're visible but offscreen, we just set a flag in the preedit
 * callback.
 */
static void
btk_tree_view_search_preedit_changed (BtkIMContext *im_context,
				      BtkTreeView  *tree_view)
{
  tree_view->priv->imcontext_changed = 1;
  if (tree_view->priv->typeselect_flush_timeout)
    {
      g_source_remove (tree_view->priv->typeselect_flush_timeout);
      tree_view->priv->typeselect_flush_timeout =
	bdk_threads_add_timeout (BTK_TREE_VIEW_SEARCH_DIALOG_TIMEOUT,
		       (GSourceFunc) btk_tree_view_search_entry_flush_timeout,
		       tree_view);
    }

}

static void
btk_tree_view_search_activate (BtkEntry    *entry,
			       BtkTreeView *tree_view)
{
  BtkTreePath *path;
  BtkRBNode *node;
  BtkRBTree *tree;

  btk_tree_view_search_dialog_hide (tree_view->priv->search_window,
				    tree_view);

  /* If we have a row selected and it's the cursor row, we activate
   * the row XXX */
  if (btk_tree_row_reference_valid (tree_view->priv->cursor))
    {
      path = btk_tree_row_reference_get_path (tree_view->priv->cursor);
      
      _btk_tree_view_find_node (tree_view, path, &tree, &node);
      
      if (node && BTK_RBNODE_FLAG_SET (node, BTK_RBNODE_IS_SELECTED))
	btk_tree_view_row_activated (tree_view, path, tree_view->priv->focus_column);
      
      btk_tree_path_free (path);
    }
}

static gboolean
btk_tree_view_real_search_enable_popdown (gpointer data)
{
  BtkTreeView *tree_view = (BtkTreeView *)data;

  tree_view->priv->disable_popdown = 0;

  return FALSE;
}

static void
btk_tree_view_search_enable_popdown (BtkWidget *widget,
				     gpointer   data)
{
  bdk_threads_add_timeout_full (G_PRIORITY_HIGH, 200, btk_tree_view_real_search_enable_popdown, g_object_ref (data), g_object_unref);
}

static gboolean
btk_tree_view_search_delete_event (BtkWidget *widget,
				   BdkEventAny *event,
				   BtkTreeView *tree_view)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  btk_tree_view_search_dialog_hide (widget, tree_view);

  return TRUE;
}

static gboolean
btk_tree_view_search_button_press_event (BtkWidget *widget,
					 BdkEventButton *event,
					 BtkTreeView *tree_view)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  btk_tree_view_search_dialog_hide (widget, tree_view);

  if (event->window == tree_view->priv->bin_window)
    btk_tree_view_button_press (BTK_WIDGET (tree_view), event);

  return TRUE;
}

static gboolean
btk_tree_view_search_scroll_event (BtkWidget *widget,
				   BdkEventScroll *event,
				   BtkTreeView *tree_view)
{
  gboolean retval = FALSE;

  if (event->direction == BDK_SCROLL_UP)
    {
      btk_tree_view_search_move (widget, tree_view, TRUE);
      retval = TRUE;
    }
  else if (event->direction == BDK_SCROLL_DOWN)
    {
      btk_tree_view_search_move (widget, tree_view, FALSE);
      retval = TRUE;
    }

  /* renew the flush timeout */
  if (retval && tree_view->priv->typeselect_flush_timeout
      && !tree_view->priv->search_custom_entry_set)
    {
      g_source_remove (tree_view->priv->typeselect_flush_timeout);
      tree_view->priv->typeselect_flush_timeout =
	bdk_threads_add_timeout (BTK_TREE_VIEW_SEARCH_DIALOG_TIMEOUT,
		       (GSourceFunc) btk_tree_view_search_entry_flush_timeout,
		       tree_view);
    }

  return retval;
}

static gboolean
btk_tree_view_search_key_press_event (BtkWidget *widget,
				      BdkEventKey *event,
				      BtkTreeView *tree_view)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), FALSE);

  /* close window and cancel the search */
  if (!tree_view->priv->search_custom_entry_set
      && (event->keyval == BDK_Escape ||
          event->keyval == BDK_Tab ||
	    event->keyval == BDK_KP_Tab ||
	    event->keyval == BDK_ISO_Left_Tab))
    {
      btk_tree_view_search_dialog_hide (widget, tree_view);
      return TRUE;
    }

  /* select previous matching iter */
  if (event->keyval == BDK_Up || event->keyval == BDK_KP_Up)
    {
      if (!btk_tree_view_search_move (widget, tree_view, TRUE))
        btk_widget_error_bell (widget);

      retval = TRUE;
    }

  if (((event->state & (BTK_DEFAULT_ACCEL_MOD_MASK | BDK_SHIFT_MASK)) == (BTK_DEFAULT_ACCEL_MOD_MASK | BDK_SHIFT_MASK))
      && (event->keyval == BDK_g || event->keyval == BDK_G))
    {
      if (!btk_tree_view_search_move (widget, tree_view, TRUE))
        btk_widget_error_bell (widget);

      retval = TRUE;
    }

  /* select next matching iter */
  if (event->keyval == BDK_Down || event->keyval == BDK_KP_Down)
    {
      if (!btk_tree_view_search_move (widget, tree_view, FALSE))
        btk_widget_error_bell (widget);

      retval = TRUE;
    }

  if (((event->state & (BTK_DEFAULT_ACCEL_MOD_MASK | BDK_SHIFT_MASK)) == BTK_DEFAULT_ACCEL_MOD_MASK)
      && (event->keyval == BDK_g || event->keyval == BDK_G))
    {
      if (!btk_tree_view_search_move (widget, tree_view, FALSE))
        btk_widget_error_bell (widget);

      retval = TRUE;
    }

  /* renew the flush timeout */
  if (retval && tree_view->priv->typeselect_flush_timeout
      && !tree_view->priv->search_custom_entry_set)
    {
      g_source_remove (tree_view->priv->typeselect_flush_timeout);
      tree_view->priv->typeselect_flush_timeout =
	bdk_threads_add_timeout (BTK_TREE_VIEW_SEARCH_DIALOG_TIMEOUT,
		       (GSourceFunc) btk_tree_view_search_entry_flush_timeout,
		       tree_view);
    }

  return retval;
}

/*  this function returns FALSE if there is a search string but
 *  nothing was found, and TRUE otherwise.
 */
static gboolean
btk_tree_view_search_move (BtkWidget   *window,
			   BtkTreeView *tree_view,
			   gboolean     up)
{
  gboolean ret;
  gint len;
  gint count = 0;
  const gchar *text;
  BtkTreeIter iter;
  BtkTreeModel *model;
  BtkTreeSelection *selection;

  text = btk_entry_get_text (BTK_ENTRY (tree_view->priv->search_entry));

  g_return_val_if_fail (text != NULL, FALSE);

  len = strlen (text);

  if (up && tree_view->priv->selected_iter == 1)
    return strlen (text) < 1;

  len = strlen (text);

  if (len < 1)
    return TRUE;

  model = btk_tree_view_get_model (tree_view);
  selection = btk_tree_view_get_selection (tree_view);

  /* search */
  btk_tree_selection_unselect_all (selection);
  if (!btk_tree_model_get_iter_first (model, &iter))
    return TRUE;

  ret = btk_tree_view_search_iter (model, selection, &iter, text,
				   &count, up?((tree_view->priv->selected_iter) - 1):((tree_view->priv->selected_iter + 1)));

  if (ret)
    {
      /* found */
      tree_view->priv->selected_iter += up?(-1):(1);
      return TRUE;
    }
  else
    {
      /* return to old iter */
      count = 0;
      btk_tree_model_get_iter_first (model, &iter);
      btk_tree_view_search_iter (model, selection,
				 &iter, text,
				 &count, tree_view->priv->selected_iter);
      return FALSE;
    }
}

static gboolean
btk_tree_view_search_equal_func (BtkTreeModel *model,
				 gint          column,
				 const gchar  *key,
				 BtkTreeIter  *iter,
				 gpointer      search_data)
{
  gboolean retval = TRUE;
  const gchar *str;
  gchar *normalized_string;
  gchar *normalized_key;
  gchar *case_normalized_string = NULL;
  gchar *case_normalized_key = NULL;
  BValue value = {0,};
  BValue transformed = {0,};

  btk_tree_model_get_value (model, iter, column, &value);

  b_value_init (&transformed, B_TYPE_STRING);

  if (!b_value_transform (&value, &transformed))
    {
      b_value_unset (&value);
      return TRUE;
    }

  b_value_unset (&value);

  str = b_value_get_string (&transformed);
  if (!str)
    {
      b_value_unset (&transformed);
      return TRUE;
    }

  normalized_string = g_utf8_normalize (str, -1, G_NORMALIZE_ALL);
  normalized_key = g_utf8_normalize (key, -1, G_NORMALIZE_ALL);

  if (normalized_string && normalized_key)
    {
      case_normalized_string = g_utf8_casefold (normalized_string, -1);
      case_normalized_key = g_utf8_casefold (normalized_key, -1);

      if (strncmp (case_normalized_key, case_normalized_string, strlen (case_normalized_key)) == 0)
        retval = FALSE;
    }

  b_value_unset (&transformed);
  g_free (normalized_key);
  g_free (normalized_string);
  g_free (case_normalized_key);
  g_free (case_normalized_string);

  return retval;
}

static gboolean
btk_tree_view_search_iter (BtkTreeModel     *model,
			   BtkTreeSelection *selection,
			   BtkTreeIter      *iter,
			   const gchar      *text,
			   gint             *count,
			   gint              n)
{
  BtkRBTree *tree = NULL;
  BtkRBNode *node = NULL;
  BtkTreePath *path;

  BtkTreeView *tree_view = btk_tree_selection_get_tree_view (selection);

  path = btk_tree_model_get_path (model, iter);
  _btk_tree_view_find_node (tree_view, path, &tree, &node);

  do
    {
      if (! tree_view->priv->search_equal_func (model, tree_view->priv->search_column, text, iter, tree_view->priv->search_user_data))
        {
          (*count)++;
          if (*count == n)
            {
              btk_tree_view_scroll_to_cell (tree_view, path, NULL,
					    TRUE, 0.5, 0.0);
              btk_tree_selection_select_iter (selection, iter);
              btk_tree_view_real_set_cursor (tree_view, path, FALSE, TRUE);

	      if (path)
		btk_tree_path_free (path);

              return TRUE;
            }
        }

      if (node->children)
	{
	  gboolean has_child;
	  BtkTreeIter tmp;

	  tree = node->children;
	  node = tree->root;

	  while (node->left != tree->nil)
	    node = node->left;

	  tmp = *iter;
	  has_child = btk_tree_model_iter_children (model, iter, &tmp);
	  btk_tree_path_down (path);

	  /* sanity check */
	  TREE_VIEW_INTERNAL_ASSERT (has_child, FALSE);
	}
      else
	{
	  gboolean done = FALSE;

	  do
	    {
	      node = _btk_rbtree_next (tree, node);

	      if (node)
		{
		  gboolean has_next;

		  has_next = btk_tree_model_iter_next (model, iter);

		  done = TRUE;
		  btk_tree_path_next (path);

		  /* sanity check */
		  TREE_VIEW_INTERNAL_ASSERT (has_next, FALSE);
		}
	      else
		{
		  gboolean has_parent;
		  BtkTreeIter tmp_iter = *iter;

		  node = tree->parent_node;
		  tree = tree->parent_tree;

		  if (!tree)
		    {
		      if (path)
			btk_tree_path_free (path);

		      /* we've run out of tree, done with this func */
		      return FALSE;
		    }

		  has_parent = btk_tree_model_iter_parent (model,
							   iter,
							   &tmp_iter);
		  btk_tree_path_up (path);

		  /* sanity check */
		  TREE_VIEW_INTERNAL_ASSERT (has_parent, FALSE);
		}
	    }
	  while (!done);
	}
    }
  while (1);

  return FALSE;
}

static void
btk_tree_view_search_init (BtkWidget   *entry,
			   BtkTreeView *tree_view)
{
  gint ret;
  gint count = 0;
  const gchar *text;
  BtkTreeIter iter;
  BtkTreeModel *model;
  BtkTreeSelection *selection;

  g_return_if_fail (BTK_IS_ENTRY (entry));
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  text = btk_entry_get_text (BTK_ENTRY (entry));

  model = btk_tree_view_get_model (tree_view);
  selection = btk_tree_view_get_selection (tree_view);

  /* search */
  btk_tree_selection_unselect_all (selection);
  if (tree_view->priv->typeselect_flush_timeout
      && !tree_view->priv->search_custom_entry_set)
    {
      g_source_remove (tree_view->priv->typeselect_flush_timeout);
      tree_view->priv->typeselect_flush_timeout =
	bdk_threads_add_timeout (BTK_TREE_VIEW_SEARCH_DIALOG_TIMEOUT,
		       (GSourceFunc) btk_tree_view_search_entry_flush_timeout,
		       tree_view);
    }

  if (*text == '\0')
    return;

  if (!btk_tree_model_get_iter_first (model, &iter))
    return;

  ret = btk_tree_view_search_iter (model, selection,
				   &iter, text,
				   &count, 1);

  if (ret)
    tree_view->priv->selected_iter = 1;
}

static void
btk_tree_view_remove_widget (BtkCellEditable *cell_editable,
			     BtkTreeView     *tree_view)
{
  if (tree_view->priv->edited_column == NULL)
    return;

  _btk_tree_view_column_stop_editing (tree_view->priv->edited_column);
  tree_view->priv->edited_column = NULL;

  if (btk_widget_has_focus (BTK_WIDGET (cell_editable)))
    btk_widget_grab_focus (BTK_WIDGET (tree_view));

  g_signal_handlers_disconnect_by_func (cell_editable,
					btk_tree_view_remove_widget,
					tree_view);

  btk_container_remove (BTK_CONTAINER (tree_view),
			BTK_WIDGET (cell_editable));  

  /* FIXME should only redraw a single node */
  btk_widget_queue_draw (BTK_WIDGET (tree_view));
}

static gboolean
btk_tree_view_start_editing (BtkTreeView *tree_view,
			     BtkTreePath *cursor_path)
{
  BtkTreeIter iter;
  BdkRectangle background_area;
  BdkRectangle cell_area;
  BtkCellEditable *editable_widget = NULL;
  gchar *path_string;
  guint flags = 0; /* can be 0, as the flags are primarily for rendering */
  gint retval = FALSE;
  BtkRBTree *cursor_tree;
  BtkRBNode *cursor_node;

  g_assert (tree_view->priv->focus_column);

  if (!btk_widget_get_realized (BTK_WIDGET (tree_view)))
    return FALSE;

  if (_btk_tree_view_find_node (tree_view, cursor_path, &cursor_tree, &cursor_node) ||
      cursor_node == NULL)
    return FALSE;

  path_string = btk_tree_path_to_string (cursor_path);
  btk_tree_model_get_iter (tree_view->priv->model, &iter, cursor_path);

  validate_row (tree_view, cursor_tree, cursor_node, &iter, cursor_path);

  btk_tree_view_column_cell_set_cell_data (tree_view->priv->focus_column,
					   tree_view->priv->model,
					   &iter,
					   BTK_RBNODE_FLAG_SET (cursor_node, BTK_RBNODE_IS_PARENT),
					   cursor_node->children?TRUE:FALSE);
  btk_tree_view_get_background_area (tree_view,
				     cursor_path,
				     tree_view->priv->focus_column,
				     &background_area);
  btk_tree_view_get_cell_area (tree_view,
			       cursor_path,
			       tree_view->priv->focus_column,
			       &cell_area);

  if (_btk_tree_view_column_cell_event (tree_view->priv->focus_column,
					&editable_widget,
					NULL,
					path_string,
					&background_area,
					&cell_area,
					flags))
    {
      retval = TRUE;
      if (editable_widget != NULL)
	{
	  gint left, right;
	  BdkRectangle area;
	  BtkCellRenderer *cell;

	  area = cell_area;
	  cell = _btk_tree_view_column_get_edited_cell (tree_view->priv->focus_column);

	  _btk_tree_view_column_get_neighbor_sizes (tree_view->priv->focus_column, cell, &left, &right);

	  area.x += left;
	  area.width -= right + left;

	  btk_tree_view_real_start_editing (tree_view,
					    tree_view->priv->focus_column,
					    cursor_path,
					    editable_widget,
					    &area,
					    NULL,
					    flags);
	}

    }
  g_free (path_string);
  return retval;
}

static void
btk_tree_view_real_start_editing (BtkTreeView       *tree_view,
				  BtkTreeViewColumn *column,
				  BtkTreePath       *path,
				  BtkCellEditable   *cell_editable,
				  BdkRectangle      *cell_area,
				  BdkEvent          *event,
				  guint              flags)
{
  gint pre_val = tree_view->priv->vadjustment->value;
  BtkRequisition requisition;

  tree_view->priv->edited_column = column;
  _btk_tree_view_column_start_editing (column, BTK_CELL_EDITABLE (cell_editable));

  btk_tree_view_real_set_cursor (tree_view, path, FALSE, TRUE);
  cell_area->y += pre_val - (int)tree_view->priv->vadjustment->value;

  btk_widget_size_request (BTK_WIDGET (cell_editable), &requisition);

  BTK_TREE_VIEW_SET_FLAG (tree_view, BTK_TREE_VIEW_DRAW_KEYFOCUS);

  if (requisition.height < cell_area->height)
    {
      gint diff = cell_area->height - requisition.height;
      btk_tree_view_put (tree_view,
			 BTK_WIDGET (cell_editable),
			 cell_area->x, cell_area->y + diff/2,
			 cell_area->width, requisition.height);
    }
  else
    {
      btk_tree_view_put (tree_view,
			 BTK_WIDGET (cell_editable),
			 cell_area->x, cell_area->y,
			 cell_area->width, cell_area->height);
    }

  btk_cell_editable_start_editing (BTK_CELL_EDITABLE (cell_editable),
				   (BdkEvent *)event);

  btk_widget_grab_focus (BTK_WIDGET (cell_editable));
  g_signal_connect (cell_editable, "remove-widget",
		    G_CALLBACK (btk_tree_view_remove_widget), tree_view);
}

static void
btk_tree_view_stop_editing (BtkTreeView *tree_view,
			    gboolean     cancel_editing)
{
  BtkTreeViewColumn *column;
  BtkCellRenderer *cell;

  if (tree_view->priv->edited_column == NULL)
    return;

  /*
   * This is very evil. We need to do this, because
   * btk_cell_editable_editing_done may trigger btk_tree_view_row_changed
   * later on. If btk_tree_view_row_changed notices
   * tree_view->priv->edited_column != NULL, it'll call
   * btk_tree_view_stop_editing again. Bad things will happen then.
   *
   * Please read that again if you intend to modify anything here.
   */

  column = tree_view->priv->edited_column;
  tree_view->priv->edited_column = NULL;

  cell = _btk_tree_view_column_get_edited_cell (column);
  btk_cell_renderer_stop_editing (cell, cancel_editing);

  if (!cancel_editing)
    btk_cell_editable_editing_done (column->editable_widget);

  tree_view->priv->edited_column = column;

  btk_cell_editable_remove_widget (column->editable_widget);
}


/**
 * btk_tree_view_set_hover_selection:
 * @tree_view: a #BtkTreeView
 * @hover: %TRUE to enable hover selection mode
 *
 * Enables of disables the hover selection mode of @tree_view.
 * Hover selection makes the selected row follow the pointer.
 * Currently, this works only for the selection modes 
 * %BTK_SELECTION_SINGLE and %BTK_SELECTION_BROWSE.
 * 
 * Since: 2.6
 **/
void     
btk_tree_view_set_hover_selection (BtkTreeView *tree_view,
				   gboolean     hover)
{
  hover = hover != FALSE;

  if (hover != tree_view->priv->hover_selection)
    {
      tree_view->priv->hover_selection = hover;

      g_object_notify (B_OBJECT (tree_view), "hover-selection");
    }
}

/**
 * btk_tree_view_get_hover_selection:
 * @tree_view: a #BtkTreeView
 * 
 * Returns whether hover selection mode is turned on for @tree_view.
 * 
 * Return value: %TRUE if @tree_view is in hover selection mode
 *
 * Since: 2.6 
 **/
gboolean 
btk_tree_view_get_hover_selection (BtkTreeView *tree_view)
{
  return tree_view->priv->hover_selection;
}

/**
 * btk_tree_view_set_hover_expand:
 * @tree_view: a #BtkTreeView
 * @expand: %TRUE to enable hover selection mode
 *
 * Enables of disables the hover expansion mode of @tree_view.
 * Hover expansion makes rows expand or collapse if the pointer 
 * moves over them.
 * 
 * Since: 2.6
 **/
void     
btk_tree_view_set_hover_expand (BtkTreeView *tree_view,
				gboolean     expand)
{
  expand = expand != FALSE;

  if (expand != tree_view->priv->hover_expand)
    {
      tree_view->priv->hover_expand = expand;

      g_object_notify (B_OBJECT (tree_view), "hover-expand");
    }
}

/**
 * btk_tree_view_get_hover_expand:
 * @tree_view: a #BtkTreeView
 * 
 * Returns whether hover expansion mode is turned on for @tree_view.
 * 
 * Return value: %TRUE if @tree_view is in hover expansion mode
 *
 * Since: 2.6 
 **/
gboolean 
btk_tree_view_get_hover_expand (BtkTreeView *tree_view)
{
  return tree_view->priv->hover_expand;
}

/**
 * btk_tree_view_set_rubber_banding:
 * @tree_view: a #BtkTreeView
 * @enable: %TRUE to enable rubber banding
 *
 * Enables or disables rubber banding in @tree_view.  If the selection mode
 * is #BTK_SELECTION_MULTIPLE, rubber banding will allow the user to select
 * multiple rows by dragging the mouse.
 * 
 * Since: 2.10
 **/
void
btk_tree_view_set_rubber_banding (BtkTreeView *tree_view,
				  gboolean     enable)
{
  enable = enable != FALSE;

  if (enable != tree_view->priv->rubber_banding_enable)
    {
      tree_view->priv->rubber_banding_enable = enable;

      g_object_notify (B_OBJECT (tree_view), "rubber-banding");
    }
}

/**
 * btk_tree_view_get_rubber_banding:
 * @tree_view: a #BtkTreeView
 * 
 * Returns whether rubber banding is turned on for @tree_view.  If the
 * selection mode is #BTK_SELECTION_MULTIPLE, rubber banding will allow the
 * user to select multiple rows by dragging the mouse.
 * 
 * Return value: %TRUE if rubber banding in @tree_view is enabled.
 *
 * Since: 2.10
 **/
gboolean
btk_tree_view_get_rubber_banding (BtkTreeView *tree_view)
{
  return tree_view->priv->rubber_banding_enable;
}

/**
 * btk_tree_view_is_rubber_banding_active:
 * @tree_view: a #BtkTreeView
 * 
 * Returns whether a rubber banding operation is currently being done
 * in @tree_view.
 *
 * Return value: %TRUE if a rubber banding operation is currently being
 * done in @tree_view.
 *
 * Since: 2.12
 **/
gboolean
btk_tree_view_is_rubber_banding_active (BtkTreeView *tree_view)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), FALSE);

  if (tree_view->priv->rubber_banding_enable
      && tree_view->priv->rubber_band_status == RUBBER_BAND_ACTIVE)
    return TRUE;

  return FALSE;
}

/**
 * btk_tree_view_get_row_separator_func:
 * @tree_view: a #BtkTreeView
 * 
 * Returns the current row separator function.
 * 
 * Return value: the current row separator function.
 *
 * Since: 2.6
 **/
BtkTreeViewRowSeparatorFunc 
btk_tree_view_get_row_separator_func (BtkTreeView *tree_view)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), NULL);

  return tree_view->priv->row_separator_func;
}

/**
 * btk_tree_view_set_row_separator_func:
 * @tree_view: a #BtkTreeView
 * @func: a #BtkTreeViewRowSeparatorFunc
 * @data: (allow-none): user data to pass to @func, or %NULL
 * @destroy: (allow-none): destroy notifier for @data, or %NULL
 * 
 * Sets the row separator function, which is used to determine
 * whether a row should be drawn as a separator. If the row separator
 * function is %NULL, no separators are drawn. This is the default value.
 *
 * Since: 2.6
 **/
void
btk_tree_view_set_row_separator_func (BtkTreeView                 *tree_view,
				      BtkTreeViewRowSeparatorFunc  func,
				      gpointer                     data,
				      GDestroyNotify               destroy)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  if (tree_view->priv->row_separator_destroy)
    tree_view->priv->row_separator_destroy (tree_view->priv->row_separator_data);

  tree_view->priv->row_separator_func = func;
  tree_view->priv->row_separator_data = data;
  tree_view->priv->row_separator_destroy = destroy;

  /* Have the tree recalculate heights */
  _btk_rbtree_mark_invalid (tree_view->priv->tree);
  btk_widget_queue_resize (BTK_WIDGET (tree_view));
}

  
static void
btk_tree_view_grab_notify (BtkWidget *widget,
			   gboolean   was_grabbed)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (widget);

  tree_view->priv->in_grab = !was_grabbed;

  if (!was_grabbed)
    {
      tree_view->priv->pressed_button = -1;

      if (tree_view->priv->rubber_band_status)
	btk_tree_view_stop_rubber_band (tree_view);
    }
}

static void
btk_tree_view_state_changed (BtkWidget      *widget,
		 	     BtkStateType    previous_state)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (widget);

  if (btk_widget_get_realized (widget))
    {
      bdk_window_set_back_pixmap (widget->window, NULL, FALSE);
      bdk_window_set_background (tree_view->priv->bin_window, &widget->style->base[widget->state]);
    }

  btk_widget_queue_draw (widget);
}

/**
 * btk_tree_view_get_grid_lines:
 * @tree_view: a #BtkTreeView
 *
 * Returns which grid lines are enabled in @tree_view.
 *
 * Return value: a #BtkTreeViewGridLines value indicating which grid lines
 * are enabled.
 *
 * Since: 2.10
 */
BtkTreeViewGridLines
btk_tree_view_get_grid_lines (BtkTreeView *tree_view)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), 0);

  return tree_view->priv->grid_lines;
}

/**
 * btk_tree_view_set_grid_lines:
 * @tree_view: a #BtkTreeView
 * @grid_lines: a #BtkTreeViewGridLines value indicating which grid lines to
 * enable.
 *
 * Sets which grid lines to draw in @tree_view.
 *
 * Since: 2.10
 */
void
btk_tree_view_set_grid_lines (BtkTreeView           *tree_view,
			      BtkTreeViewGridLines   grid_lines)
{
  BtkTreeViewPrivate *priv;
  BtkWidget *widget;
  BtkTreeViewGridLines old_grid_lines;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  priv = tree_view->priv;
  widget = BTK_WIDGET (tree_view);

  old_grid_lines = priv->grid_lines;
  priv->grid_lines = grid_lines;
  
  if (btk_widget_get_realized (widget))
    {
      if (grid_lines == BTK_TREE_VIEW_GRID_LINES_NONE &&
	  priv->grid_line_width)
	{
	  priv->grid_line_width = 0;
	}
      
      if (grid_lines != BTK_TREE_VIEW_GRID_LINES_NONE && 
	  !priv->grid_line_width)
	{
	  gint8 *dash_list;

	  btk_widget_style_get (widget,
				"grid-line-width", &priv->grid_line_width,
				"grid-line-pattern", (gchar *)&dash_list,
				NULL);
      
          if (dash_list)
            {
              priv->grid_line_dashes[0] = dash_list[0];
              if (dash_list[0])
                priv->grid_line_dashes[1] = dash_list[1];
	      
              g_free (dash_list);
            }
          else
            {
              priv->grid_line_dashes[0] = 1;
              priv->grid_line_dashes[1] = 1;
            }
	}      
    }

  if (old_grid_lines != grid_lines)
    {
      btk_widget_queue_draw (BTK_WIDGET (tree_view));
      
      g_object_notify (B_OBJECT (tree_view), "enable-grid-lines");
    }
}

/**
 * btk_tree_view_get_enable_tree_lines:
 * @tree_view: a #BtkTreeView.
 *
 * Returns whether or not tree lines are drawn in @tree_view.
 *
 * Return value: %TRUE if tree lines are drawn in @tree_view, %FALSE
 * otherwise.
 *
 * Since: 2.10
 */
gboolean
btk_tree_view_get_enable_tree_lines (BtkTreeView *tree_view)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), FALSE);

  return tree_view->priv->tree_lines_enabled;
}

/**
 * btk_tree_view_set_enable_tree_lines:
 * @tree_view: a #BtkTreeView
 * @enabled: %TRUE to enable tree line drawing, %FALSE otherwise.
 *
 * Sets whether to draw lines interconnecting the expanders in @tree_view.
 * This does not have any visible effects for lists.
 *
 * Since: 2.10
 */
void
btk_tree_view_set_enable_tree_lines (BtkTreeView *tree_view,
				     gboolean     enabled)
{
  BtkTreeViewPrivate *priv;
  BtkWidget *widget;
  gboolean was_enabled;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  enabled = enabled != FALSE;

  priv = tree_view->priv;
  widget = BTK_WIDGET (tree_view);

  was_enabled = priv->tree_lines_enabled;

  priv->tree_lines_enabled = enabled;

  if (btk_widget_get_realized (widget))
    {
      if (!enabled && priv->tree_line_width)
	{
          priv->tree_line_width = 0;
	}
      
      if (enabled && !priv->tree_line_width)
	{
	  gint8 *dash_list;
	  btk_widget_style_get (widget,
				"tree-line-width", &priv->tree_line_width,
				"tree-line-pattern", (gchar *)&dash_list,
				NULL);
	  
          if (dash_list)
            {
              priv->tree_line_dashes[0] = dash_list[0];
              if (dash_list[0])
                priv->tree_line_dashes[1] = dash_list[1];
	      
              g_free (dash_list);
            }
          else
            {
              priv->tree_line_dashes[0] = 1;
              priv->tree_line_dashes[1] = 1;
            }
	}
    }

  if (was_enabled != enabled)
    {
      btk_widget_queue_draw (BTK_WIDGET (tree_view));

      g_object_notify (B_OBJECT (tree_view), "enable-tree-lines");
    }
}


/**
 * btk_tree_view_set_show_expanders:
 * @tree_view: a #BtkTreeView
 * @enabled: %TRUE to enable expander drawing, %FALSE otherwise.
 *
 * Sets whether to draw and enable expanders and indent child rows in
 * @tree_view.  When disabled there will be no expanders visible in trees
 * and there will be no way to expand and collapse rows by default.  Also
 * note that hiding the expanders will disable the default indentation.  You
 * can set a custom indentation in this case using
 * btk_tree_view_set_level_indentation().
 * This does not have any visible effects for lists.
 *
 * Since: 2.12
 */
void
btk_tree_view_set_show_expanders (BtkTreeView *tree_view,
				  gboolean     enabled)
{
  gboolean was_enabled;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  enabled = enabled != FALSE;
  was_enabled = BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_SHOW_EXPANDERS);

  if (enabled)
    BTK_TREE_VIEW_SET_FLAG (tree_view, BTK_TREE_VIEW_SHOW_EXPANDERS);
  else
    BTK_TREE_VIEW_UNSET_FLAG (tree_view, BTK_TREE_VIEW_SHOW_EXPANDERS);

  if (enabled != was_enabled)
    btk_widget_queue_draw (BTK_WIDGET (tree_view));
}

/**
 * btk_tree_view_get_show_expanders:
 * @tree_view: a #BtkTreeView.
 *
 * Returns whether or not expanders are drawn in @tree_view.
 *
 * Return value: %TRUE if expanders are drawn in @tree_view, %FALSE
 * otherwise.
 *
 * Since: 2.12
 */
gboolean
btk_tree_view_get_show_expanders (BtkTreeView *tree_view)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), FALSE);

  return BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_SHOW_EXPANDERS);
}

/**
 * btk_tree_view_set_level_indentation:
 * @tree_view: a #BtkTreeView
 * @indentation: the amount, in pixels, of extra indentation in @tree_view.
 *
 * Sets the amount of extra indentation for child levels to use in @tree_view
 * in addition to the default indentation.  The value should be specified in
 * pixels, a value of 0 disables this feature and in this case only the default
 * indentation will be used.
 * This does not have any visible effects for lists.
 *
 * Since: 2.12
 */
void
btk_tree_view_set_level_indentation (BtkTreeView *tree_view,
				     gint         indentation)
{
  tree_view->priv->level_indentation = indentation;

  btk_widget_queue_draw (BTK_WIDGET (tree_view));
}

/**
 * btk_tree_view_get_level_indentation:
 * @tree_view: a #BtkTreeView.
 *
 * Returns the amount, in pixels, of extra indentation for child levels
 * in @tree_view.
 *
 * Return value: the amount of extra indentation for child levels in
 * @tree_view.  A return value of 0 means that this feature is disabled.
 *
 * Since: 2.12
 */
gint
btk_tree_view_get_level_indentation (BtkTreeView *tree_view)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), 0);

  return tree_view->priv->level_indentation;
}

/**
 * btk_tree_view_set_tooltip_row:
 * @tree_view: a #BtkTreeView
 * @tooltip: a #BtkTooltip
 * @path: a #BtkTreePath
 *
 * Sets the tip area of @tooltip to be the area covered by the row at @path.
 * See also btk_tree_view_set_tooltip_column() for a simpler alternative.
 * See also btk_tooltip_set_tip_area().
 *
 * Since: 2.12
 */
void
btk_tree_view_set_tooltip_row (BtkTreeView *tree_view,
			       BtkTooltip  *tooltip,
			       BtkTreePath *path)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (BTK_IS_TOOLTIP (tooltip));

  btk_tree_view_set_tooltip_cell (tree_view, tooltip, path, NULL, NULL);
}

/**
 * btk_tree_view_set_tooltip_cell:
 * @tree_view: a #BtkTreeView
 * @tooltip: a #BtkTooltip
 * @path: (allow-none): a #BtkTreePath or %NULL
 * @column: (allow-none): a #BtkTreeViewColumn or %NULL
 * @cell: (allow-none): a #BtkCellRenderer or %NULL
 *
 * Sets the tip area of @tooltip to the area @path, @column and @cell have
 * in common.  For example if @path is %NULL and @column is set, the tip
 * area will be set to the full area covered by @column.  See also
 * btk_tooltip_set_tip_area().
 *
 * Note that if @path is not specified and @cell is set and part of a column
 * containing the expander, the tooltip might not show and hide at the correct
 * position.  In such cases @path must be set to the current node under the
 * mouse cursor for this function to operate correctly.
 *
 * See also btk_tree_view_set_tooltip_column() for a simpler alternative.
 *
 * Since: 2.12
 */
void
btk_tree_view_set_tooltip_cell (BtkTreeView       *tree_view,
				BtkTooltip        *tooltip,
				BtkTreePath       *path,
				BtkTreeViewColumn *column,
				BtkCellRenderer   *cell)
{
  BdkRectangle rect;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (BTK_IS_TOOLTIP (tooltip));
  g_return_if_fail (column == NULL || BTK_IS_TREE_VIEW_COLUMN (column));
  g_return_if_fail (cell == NULL || BTK_IS_CELL_RENDERER (cell));

  /* Determine x values. */
  if (column && cell)
    {
      BdkRectangle tmp;
      gint start, width;

      /* We always pass in path here, whether it is NULL or not.
       * For cells in expander columns path must be specified so that
       * we can correctly account for the indentation.  This also means
       * that the tooltip is constrained vertically by the "Determine y
       * values" code below; this is not a real problem since cells actually
       * don't stretch vertically in constrast to columns.
       */
      btk_tree_view_get_cell_area (tree_view, path, column, &tmp);
      btk_tree_view_column_cell_get_position (column, cell, &start, &width);

      btk_tree_view_convert_bin_window_to_widget_coords (tree_view,
							 tmp.x + start, 0,
							 &rect.x, NULL);
      rect.width = width;
    }
  else if (column)
    {
      BdkRectangle tmp;

      btk_tree_view_get_background_area (tree_view, NULL, column, &tmp);
      btk_tree_view_convert_bin_window_to_widget_coords (tree_view,
							 tmp.x, 0,
							 &rect.x, NULL);
      rect.width = tmp.width;
    }
  else
    {
      rect.x = 0;
      rect.width = BTK_WIDGET (tree_view)->allocation.width;
    }

  /* Determine y values. */
  if (path)
    {
      BdkRectangle tmp;

      btk_tree_view_get_background_area (tree_view, path, NULL, &tmp);
      btk_tree_view_convert_bin_window_to_widget_coords (tree_view,
							 0, tmp.y,
							 NULL, &rect.y);
      rect.height = tmp.height;
    }
  else
    {
      rect.y = 0;
      rect.height = tree_view->priv->vadjustment->page_size;
    }

  btk_tooltip_set_tip_area (tooltip, &rect);
}

/**
 * btk_tree_view_get_tooltip_context:
 * @tree_view: a #BtkTreeView
 * @x: (inout): the x coordinate (relative to widget coordinates)
 * @y: (inout): the y coordinate (relative to widget coordinates)
 * @keyboard_tip: whether this is a keyboard tooltip or not
 * @model: (out) (allow-none): a pointer to receive a #BtkTreeModel or %NULL
 * @path: (out) (allow-none): a pointer to receive a #BtkTreePath or %NULL
 * @iter: (out) (allow-none): a pointer to receive a #BtkTreeIter or %NULL
 *
 * This function is supposed to be used in a #BtkWidget::query-tooltip
 * signal handler for #BtkTreeView.  The @x, @y and @keyboard_tip values
 * which are received in the signal handler, should be passed to this
 * function without modification.
 *
 * The return value indicates whether there is a tree view row at the given
 * coordinates (%TRUE) or not (%FALSE) for mouse tooltips.  For keyboard
 * tooltips the row returned will be the cursor row.  When %TRUE, then any of
 * @model, @path and @iter which have been provided will be set to point to
 * that row and the corresponding model.  @x and @y will always be converted
 * to be relative to @tree_view's bin_window if @keyboard_tooltip is %FALSE.
 *
 * Return value: whether or not the given tooltip context points to a row.
 *
 * Since: 2.12
 */
gboolean
btk_tree_view_get_tooltip_context (BtkTreeView   *tree_view,
				   gint          *x,
				   gint          *y,
				   gboolean       keyboard_tip,
				   BtkTreeModel **model,
				   BtkTreePath  **path,
				   BtkTreeIter   *iter)
{
  BtkTreePath *tmppath = NULL;

  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), FALSE);
  g_return_val_if_fail (x != NULL, FALSE);
  g_return_val_if_fail (y != NULL, FALSE);

  if (keyboard_tip)
    {
      btk_tree_view_get_cursor (tree_view, &tmppath, NULL);

      if (!tmppath)
	return FALSE;
    }
  else
    {
      btk_tree_view_convert_widget_to_bin_window_coords (tree_view, *x, *y,
							 x, y);

      if (!btk_tree_view_get_path_at_pos (tree_view, *x, *y,
					  &tmppath, NULL, NULL, NULL))
	return FALSE;
    }

  if (model)
    *model = btk_tree_view_get_model (tree_view);

  if (iter)
    btk_tree_model_get_iter (btk_tree_view_get_model (tree_view),
			     iter, tmppath);

  if (path)
    *path = tmppath;
  else
    btk_tree_path_free (tmppath);

  return TRUE;
}

static gboolean
btk_tree_view_set_tooltip_query_cb (BtkWidget  *widget,
				    gint        x,
				    gint        y,
				    gboolean    keyboard_tip,
				    BtkTooltip *tooltip,
				    gpointer    data)
{
  BValue value = { 0, };
  BValue transformed = { 0, };
  BtkTreeIter iter;
  BtkTreePath *path;
  BtkTreeModel *model;
  BtkTreeView *tree_view = BTK_TREE_VIEW (widget);

  if (!btk_tree_view_get_tooltip_context (BTK_TREE_VIEW (widget),
					  &x, &y,
					  keyboard_tip,
					  &model, &path, &iter))
    return FALSE;

  btk_tree_model_get_value (model, &iter,
                            tree_view->priv->tooltip_column, &value);

  b_value_init (&transformed, B_TYPE_STRING);

  if (!b_value_transform (&value, &transformed))
    {
      b_value_unset (&value);
      btk_tree_path_free (path);

      return FALSE;
    }

  b_value_unset (&value);

  if (!b_value_get_string (&transformed))
    {
      b_value_unset (&transformed);
      btk_tree_path_free (path);

      return FALSE;
    }

  btk_tooltip_set_markup (tooltip, b_value_get_string (&transformed));
  btk_tree_view_set_tooltip_row (tree_view, tooltip, path);

  btk_tree_path_free (path);
  b_value_unset (&transformed);

  return TRUE;
}

/**
 * btk_tree_view_set_tooltip_column:
 * @tree_view: a #BtkTreeView
 * @column: an integer, which is a valid column number for @tree_view's model
 *
 * If you only plan to have simple (text-only) tooltips on full rows, you
 * can use this function to have #BtkTreeView handle these automatically
 * for you. @column should be set to the column in @tree_view's model
 * containing the tooltip texts, or -1 to disable this feature.
 *
 * When enabled, #BtkWidget::has-tooltip will be set to %TRUE and
 * @tree_view will connect a #BtkWidget::query-tooltip signal handler.
 *
 * Note that the signal handler sets the text with btk_tooltip_set_markup(),
 * so &amp;, &lt;, etc have to be escaped in the text.
 *
 * Since: 2.12
 */
void
btk_tree_view_set_tooltip_column (BtkTreeView *tree_view,
			          gint         column)
{
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  if (column == tree_view->priv->tooltip_column)
    return;

  if (column == -1)
    {
      g_signal_handlers_disconnect_by_func (tree_view,
	  				    btk_tree_view_set_tooltip_query_cb,
					    NULL);
      btk_widget_set_has_tooltip (BTK_WIDGET (tree_view), FALSE);
    }
  else
    {
      if (tree_view->priv->tooltip_column == -1)
        {
          g_signal_connect (tree_view, "query-tooltip",
		            G_CALLBACK (btk_tree_view_set_tooltip_query_cb), NULL);
          btk_widget_set_has_tooltip (BTK_WIDGET (tree_view), TRUE);
        }
    }

  tree_view->priv->tooltip_column = column;
  g_object_notify (B_OBJECT (tree_view), "tooltip-column");
}

/**
 * btk_tree_view_get_tooltip_column:
 * @tree_view: a #BtkTreeView
 *
 * Returns the column of @tree_view's model which is being used for
 * displaying tooltips on @tree_view's rows.
 *
 * Return value: the index of the tooltip column that is currently being
 * used, or -1 if this is disabled.
 *
 * Since: 2.12
 */
gint
btk_tree_view_get_tooltip_column (BtkTreeView *tree_view)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW (tree_view), 0);

  return tree_view->priv->tooltip_column;
}

#define __BTK_TREE_VIEW_C__
#include "btkaliasdef.c"
