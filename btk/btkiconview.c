/* btkiconview.c
 * Copyright (C) 2002, 2004  Anders Carlsson <andersca@gnu.org>
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

#include <batk/batk.h>

#include <bdk/bdkkeysyms.h>

#include "btkiconview.h"
#include "btkcelllayout.h"
#include "btkcellrenderer.h"
#include "btkcellrenderertext.h"
#include "btkcellrendererpixbuf.h"
#include "btkmarshalers.h"
#include "btkbindings.h"
#include "btkdnd.h"
#include "btkmain.h"
#include "btkintl.h"
#include "btkaccessible.h"
#include "btkwindow.h"
#include "btkentry.h"
#include "btkcombobox.h"
#include "btktextbuffer.h"
#include "btktreednd.h"
#include "btkprivate.h"
#include "btkalias.h"

#undef DEBUG_ICON_VIEW

#define SCROLL_EDGE_SIZE 15

#define BTK_ICON_VIEW_GET_PRIVATE(obj) (B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_ICON_VIEW, BtkIconViewPrivate))

typedef struct _BtkIconViewItem BtkIconViewItem;
struct _BtkIconViewItem
{
  BtkTreeIter iter;
  bint index;
  
  bint row, col;

  /* Bounding box */
  bint x, y, width, height;

  /* Individual cells.
   * box[i] is the actual area occupied by cell i,
   * before, after are used to calculate the cell 
   * area relative to the box. 
   * See btk_icon_view_get_cell_area().
   */
  bint n_cells;
  BdkRectangle *box;
  bint *before;
  bint *after;

  buint selected : 1;
  buint selected_before_rubberbanding : 1;

};

typedef struct _BtkIconViewCellInfo BtkIconViewCellInfo;
struct _BtkIconViewCellInfo
{
  BtkCellRenderer *cell;

  buint expand : 1;
  buint pack : 1;
  buint editing : 1;

  bint position;

  GSList *attributes;

  BtkCellLayoutDataFunc func;
  bpointer func_data;
  GDestroyNotify destroy;
};

typedef struct _BtkIconViewChild BtkIconViewChild;
struct _BtkIconViewChild
{
  BtkWidget *widget;
  BtkIconViewItem *item;
  bint cell;
};

struct _BtkIconViewPrivate
{
  bint width, height;

  BtkSelectionMode selection_mode;

  BdkWindow *bin_window;

  GList *children;

  BtkTreeModel *model;
  
  GList *items;
  
  BtkAdjustment *hadjustment;
  BtkAdjustment *vadjustment;

  buint layout_idle_id;
  
  bboolean doing_rubberband;
  bint rubberband_x1, rubberband_y1;
  bint rubberband_x2, rubberband_y2;

  buint scroll_timeout_id;
  bint scroll_value_diff;
  bint event_last_x, event_last_y;

  BtkIconViewItem *anchor_item;
  BtkIconViewItem *cursor_item;
  BtkIconViewItem *edited_item;
  BtkCellEditable *editable;

  BtkIconViewItem *last_single_clicked;

  GList *cell_list;
  buint n_cells;

  bint cursor_cell;

  BtkOrientation item_orientation;

  bint columns;
  bint item_width;
  bint spacing;
  bint row_spacing;
  bint column_spacing;
  bint margin;
  bint item_padding;

  bint text_column;
  bint markup_column;
  bint pixbuf_column;

  bint pixbuf_cell;
  bint text_cell;

  bint tooltip_column;

  /* Drag-and-drop. */
  BdkModifierType start_button_mask;
  bint pressed_button;
  bint press_start_x;
  bint press_start_y;

  BdkDragAction source_actions;
  BdkDragAction dest_actions;

  BtkTreeRowReference *dest_item;
  BtkIconViewDropPosition dest_pos;

  /* scroll to */
  BtkTreeRowReference *scroll_to_path;
  bfloat scroll_to_row_align;
  bfloat scroll_to_col_align;
  buint scroll_to_use_align : 1;

  buint source_set : 1;
  buint dest_set : 1;
  buint reorderable : 1;
  buint empty_view_drop :1;

  buint modify_selection_pressed : 1;
  buint extend_selection_pressed : 1;

  buint draw_focus : 1;
};

/* Signals */
enum
{
  ITEM_ACTIVATED,
  SELECTION_CHANGED,
  SELECT_ALL,
  UNSELECT_ALL,
  SELECT_CURSOR_ITEM,
  TOGGLE_CURSOR_ITEM,
  MOVE_CURSOR,
  ACTIVATE_CURSOR_ITEM,
  LAST_SIGNAL
};

/* Properties */
enum
{
  PROP_0,
  PROP_PIXBUF_COLUMN,
  PROP_TEXT_COLUMN,
  PROP_MARKUP_COLUMN,  
  PROP_SELECTION_MODE,
  PROP_ORIENTATION,
  PROP_ITEM_ORIENTATION,
  PROP_MODEL,
  PROP_COLUMNS,
  PROP_ITEM_WIDTH,
  PROP_SPACING,
  PROP_ROW_SPACING,
  PROP_COLUMN_SPACING,
  PROP_MARGIN,
  PROP_REORDERABLE,
  PROP_TOOLTIP_COLUMN,
  PROP_ITEM_PADDING
};

/* BObject vfuncs */
static void             btk_icon_view_cell_layout_init          (BtkCellLayoutIface *iface);
static void             btk_icon_view_finalize                  (BObject            *object);
static void             btk_icon_view_set_property              (BObject            *object,
								 buint               prop_id,
								 const BValue       *value,
								 BParamSpec         *pspec);
static void             btk_icon_view_get_property              (BObject            *object,
								 buint               prop_id,
								 BValue             *value,
								 BParamSpec         *pspec);

/* BtkObject vfuncs */
static void             btk_icon_view_destroy                   (BtkObject          *object);

/* BtkWidget vfuncs */
static void             btk_icon_view_realize                   (BtkWidget          *widget);
static void             btk_icon_view_unrealize                 (BtkWidget          *widget);
static void             btk_icon_view_style_set                 (BtkWidget        *widget,
						                 BtkStyle         *previous_style);
static void             btk_icon_view_state_changed             (BtkWidget        *widget,
			                                         BtkStateType      previous_state);
static void             btk_icon_view_size_request              (BtkWidget          *widget,
								 BtkRequisition     *requisition);
static void             btk_icon_view_size_allocate             (BtkWidget          *widget,
								 BtkAllocation      *allocation);
static bboolean         btk_icon_view_expose                    (BtkWidget          *widget,
								 BdkEventExpose     *expose);
static bboolean         btk_icon_view_motion                    (BtkWidget          *widget,
								 BdkEventMotion     *event);
static bboolean         btk_icon_view_button_press              (BtkWidget          *widget,
								 BdkEventButton     *event);
static bboolean         btk_icon_view_button_release            (BtkWidget          *widget,
								 BdkEventButton     *event);
static bboolean         btk_icon_view_key_press                 (BtkWidget          *widget,
								 BdkEventKey        *event);
static bboolean         btk_icon_view_key_release               (BtkWidget          *widget,
								 BdkEventKey        *event);
static BatkObject       *btk_icon_view_get_accessible            (BtkWidget          *widget);


/* BtkContainer vfuncs */
static void             btk_icon_view_remove                    (BtkContainer       *container,
								 BtkWidget          *widget);
static void             btk_icon_view_forall                    (BtkContainer       *container,
								 bboolean            include_internals,
								 BtkCallback         callback,
								 bpointer            callback_data);

/* BtkIconView vfuncs */
static void             btk_icon_view_set_adjustments           (BtkIconView        *icon_view,
								 BtkAdjustment      *hadj,
								 BtkAdjustment      *vadj);
static void             btk_icon_view_real_select_all           (BtkIconView        *icon_view);
static void             btk_icon_view_real_unselect_all         (BtkIconView        *icon_view);
static void             btk_icon_view_real_select_cursor_item   (BtkIconView        *icon_view);
static void             btk_icon_view_real_toggle_cursor_item   (BtkIconView        *icon_view);
static bboolean         btk_icon_view_real_activate_cursor_item (BtkIconView        *icon_view);

 /* Internal functions */
static void                 btk_icon_view_adjustment_changed             (BtkAdjustment          *adjustment,
									  BtkIconView            *icon_view);
static void                 btk_icon_view_layout                         (BtkIconView            *icon_view);
static void                 btk_icon_view_paint_item                     (BtkIconView            *icon_view,
									  bairo_t *cr,

									  BtkIconViewItem        *item,
									  BdkRectangle           *area,
									  BdkDrawable *drawable,
									  bint         x,
									  bint         y,
									  bboolean     draw_focus);
static void                 btk_icon_view_paint_rubberband               (BtkIconView            *icon_view,
								          bairo_t *cr,
									  BdkRectangle           *area);
static void                 btk_icon_view_queue_draw_path                (BtkIconView *icon_view,
									  BtkTreePath *path);
static void                 btk_icon_view_queue_draw_item                (BtkIconView            *icon_view,
									  BtkIconViewItem        *item);
static void                 btk_icon_view_queue_layout                   (BtkIconView            *icon_view);
static void                 btk_icon_view_set_cursor_item                (BtkIconView            *icon_view,
									  BtkIconViewItem        *item,
									  bint                    cursor_cell);
static void                 btk_icon_view_start_rubberbanding            (BtkIconView            *icon_view,
									  bint                    x,
									  bint                    y);
static void                 btk_icon_view_stop_rubberbanding             (BtkIconView            *icon_view);
static void                 btk_icon_view_update_rubberband_selection    (BtkIconView            *icon_view);
static bboolean             btk_icon_view_item_hit_test                  (BtkIconView            *icon_view,
									  BtkIconViewItem        *item,
									  bint                    x,
									  bint                    y,
									  bint                    width,
									  bint                    height);
static bboolean             btk_icon_view_unselect_all_internal          (BtkIconView            *icon_view);
static void                 btk_icon_view_calculate_item_size            (BtkIconView            *icon_view,
									  BtkIconViewItem        *item);
static void                 btk_icon_view_calculate_item_size2           (BtkIconView            *icon_view,
									  BtkIconViewItem        *item,
									  bint                   *max_height);
static void                 btk_icon_view_update_rubberband              (bpointer                data);
static void                 btk_icon_view_item_invalidate_size           (BtkIconViewItem        *item);
static void                 btk_icon_view_invalidate_sizes               (BtkIconView            *icon_view);
static void                 btk_icon_view_add_move_binding               (BtkBindingSet          *binding_set,
									  buint                   keyval,
									  buint                   modmask,
									  BtkMovementStep         step,
									  bint                    count);
static bboolean             btk_icon_view_real_move_cursor               (BtkIconView            *icon_view,
									  BtkMovementStep         step,
									  bint                    count);
static void                 btk_icon_view_move_cursor_up_down            (BtkIconView            *icon_view,
									  bint                    count);
static void                 btk_icon_view_move_cursor_page_up_down       (BtkIconView            *icon_view,
									  bint                    count);
static void                 btk_icon_view_move_cursor_left_right         (BtkIconView            *icon_view,
									  bint                    count);
static void                 btk_icon_view_move_cursor_start_end          (BtkIconView            *icon_view,
									  bint                    count);
static void                 btk_icon_view_scroll_to_item                 (BtkIconView            *icon_view,
									  BtkIconViewItem        *item);
static void                 btk_icon_view_select_item                    (BtkIconView            *icon_view,
									  BtkIconViewItem        *item);
static void                 btk_icon_view_unselect_item                  (BtkIconView            *icon_view,
									  BtkIconViewItem        *item);
static bboolean             btk_icon_view_select_all_between             (BtkIconView            *icon_view,
									  BtkIconViewItem        *anchor,
									  BtkIconViewItem        *cursor);
static BtkIconViewItem *    btk_icon_view_get_item_at_coords             (BtkIconView            *icon_view,
									  bint                    x,
									  bint                    y,
									  bboolean                only_in_cell,
									  BtkIconViewCellInfo   **cell_at_pos);
static void                 btk_icon_view_get_cell_area                  (BtkIconView            *icon_view,
									  BtkIconViewItem        *item,
									  BtkIconViewCellInfo    *cell_info,
									  BdkRectangle           *cell_area);
static void                 btk_icon_view_get_cell_box                   (BtkIconView            *icon_view,
									  BtkIconViewItem        *item,
									  BtkIconViewCellInfo    *info,
									  BdkRectangle           *box);
static BtkIconViewCellInfo *btk_icon_view_get_cell_info                  (BtkIconView            *icon_view,
									  BtkCellRenderer        *renderer);
static void                 btk_icon_view_set_cell_data                  (BtkIconView            *icon_view,
									  BtkIconViewItem        *item);
static void                 btk_icon_view_cell_layout_pack_start         (BtkCellLayout          *layout,
									  BtkCellRenderer        *renderer,
									  bboolean                expand);
static void                 btk_icon_view_cell_layout_pack_end           (BtkCellLayout          *layout,
									  BtkCellRenderer        *renderer,
									  bboolean                expand);
static void                 btk_icon_view_cell_layout_add_attribute      (BtkCellLayout          *layout,
									  BtkCellRenderer        *renderer,
									  const bchar            *attribute,
									  bint                    column);
static void                 btk_icon_view_cell_layout_clear              (BtkCellLayout          *layout);
static void                 btk_icon_view_cell_layout_clear_attributes   (BtkCellLayout          *layout,
									  BtkCellRenderer        *renderer);
static void                 btk_icon_view_cell_layout_set_cell_data_func (BtkCellLayout          *layout,
									  BtkCellRenderer        *cell,
									  BtkCellLayoutDataFunc   func,
									  bpointer                func_data,
									  GDestroyNotify          destroy);
static void                 btk_icon_view_cell_layout_reorder            (BtkCellLayout          *layout,
									  BtkCellRenderer        *cell,
									  bint                    position);
static GList *              btk_icon_view_cell_layout_get_cells          (BtkCellLayout          *layout);

static void                 btk_icon_view_item_activate_cell             (BtkIconView            *icon_view,
									  BtkIconViewItem        *item,
									  BtkIconViewCellInfo    *cell_info,
									  BdkEvent               *event);
static void                 btk_icon_view_item_selected_changed          (BtkIconView            *icon_view,
		                                                          BtkIconViewItem        *item);
static void                 btk_icon_view_put                            (BtkIconView            *icon_view,
									  BtkWidget              *widget,
									  BtkIconViewItem        *item,
									  bint                    cell);
static void                 btk_icon_view_remove_widget                  (BtkCellEditable        *editable,
									  BtkIconView            *icon_view);
static void                 btk_icon_view_start_editing                  (BtkIconView            *icon_view,
									  BtkIconViewItem        *item,
									  BtkIconViewCellInfo    *cell_info,
									  BdkEvent               *event);
static void                 btk_icon_view_stop_editing                   (BtkIconView            *icon_view,
									  bboolean                cancel_editing);

/* Source side drag signals */
static void btk_icon_view_drag_begin       (BtkWidget        *widget,
                                            BdkDragContext   *context);
static void btk_icon_view_drag_end         (BtkWidget        *widget,
                                            BdkDragContext   *context);
static void btk_icon_view_drag_data_get    (BtkWidget        *widget,
                                            BdkDragContext   *context,
                                            BtkSelectionData *selection_data,
                                            buint             info,
                                            buint             time);
static void btk_icon_view_drag_data_delete (BtkWidget        *widget,
                                            BdkDragContext   *context);

/* Target side drag signals */
static void     btk_icon_view_drag_leave         (BtkWidget        *widget,
                                                  BdkDragContext   *context,
                                                  buint             time);
static bboolean btk_icon_view_drag_motion        (BtkWidget        *widget,
                                                  BdkDragContext   *context,
                                                  bint              x,
                                                  bint              y,
                                                  buint             time);
static bboolean btk_icon_view_drag_drop          (BtkWidget        *widget,
                                                  BdkDragContext   *context,
                                                  bint              x,
                                                  bint              y,
                                                  buint             time);
static void     btk_icon_view_drag_data_received (BtkWidget        *widget,
                                                  BdkDragContext   *context,
                                                  bint              x,
                                                  bint              y,
                                                  BtkSelectionData *selection_data,
                                                  buint             info,
                                                  buint             time);
static bboolean btk_icon_view_maybe_begin_drag   (BtkIconView             *icon_view,
					   	  BdkEventMotion          *event);

static void     remove_scroll_timeout            (BtkIconView *icon_view);

static void     adjust_wrap_width                (BtkIconView     *icon_view,
						  BtkIconViewItem *item);

/* BtkBuildable */
static BtkBuildableIface *parent_buildable_iface;
static void     btk_icon_view_buildable_init             (BtkBuildableIface *iface);
static bboolean btk_icon_view_buildable_custom_tag_start (BtkBuildable  *buildable,
							  BtkBuilder    *builder,
							  BObject       *child,
							  const bchar   *tagname,
							  GMarkupParser *parser,
							  bpointer      *data);
static void     btk_icon_view_buildable_custom_tag_end   (BtkBuildable  *buildable,
							  BtkBuilder    *builder,
							  BObject       *child,
							  const bchar   *tagname,
							  bpointer      *data);

static buint icon_view_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (BtkIconView, btk_icon_view, BTK_TYPE_CONTAINER,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_CELL_LAYOUT,
						btk_icon_view_cell_layout_init)
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
						btk_icon_view_buildable_init))

static void
btk_icon_view_class_init (BtkIconViewClass *klass)
{
  BObjectClass *bobject_class;
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;
  BtkContainerClass *container_class;
  BtkBindingSet *binding_set;
  
  binding_set = btk_binding_set_by_class (klass);

  g_type_class_add_private (klass, sizeof (BtkIconViewPrivate));

  bobject_class = (BObjectClass *) klass;
  object_class = (BtkObjectClass *) klass;
  widget_class = (BtkWidgetClass *) klass;
  container_class = (BtkContainerClass *) klass;

  bobject_class->finalize = btk_icon_view_finalize;
  bobject_class->set_property = btk_icon_view_set_property;
  bobject_class->get_property = btk_icon_view_get_property;

  object_class->destroy = btk_icon_view_destroy;
  
  widget_class->realize = btk_icon_view_realize;
  widget_class->unrealize = btk_icon_view_unrealize;
  widget_class->style_set = btk_icon_view_style_set;
  widget_class->get_accessible = btk_icon_view_get_accessible;
  widget_class->size_request = btk_icon_view_size_request;
  widget_class->size_allocate = btk_icon_view_size_allocate;
  widget_class->expose_event = btk_icon_view_expose;
  widget_class->motion_notify_event = btk_icon_view_motion;
  widget_class->button_press_event = btk_icon_view_button_press;
  widget_class->button_release_event = btk_icon_view_button_release;
  widget_class->key_press_event = btk_icon_view_key_press;
  widget_class->key_release_event = btk_icon_view_key_release;
  widget_class->drag_begin = btk_icon_view_drag_begin;
  widget_class->drag_end = btk_icon_view_drag_end;
  widget_class->drag_data_get = btk_icon_view_drag_data_get;
  widget_class->drag_data_delete = btk_icon_view_drag_data_delete;
  widget_class->drag_leave = btk_icon_view_drag_leave;
  widget_class->drag_motion = btk_icon_view_drag_motion;
  widget_class->drag_drop = btk_icon_view_drag_drop;
  widget_class->drag_data_received = btk_icon_view_drag_data_received;
  widget_class->state_changed = btk_icon_view_state_changed;

  container_class->remove = btk_icon_view_remove;
  container_class->forall = btk_icon_view_forall;

  klass->set_scroll_adjustments = btk_icon_view_set_adjustments;
  klass->select_all = btk_icon_view_real_select_all;
  klass->unselect_all = btk_icon_view_real_unselect_all;
  klass->select_cursor_item = btk_icon_view_real_select_cursor_item;
  klass->toggle_cursor_item = btk_icon_view_real_toggle_cursor_item;
  klass->activate_cursor_item = btk_icon_view_real_activate_cursor_item;  
  klass->move_cursor = btk_icon_view_real_move_cursor;
  
  /* Properties */
  /**
   * BtkIconView:selection-mode:
   * 
   * The ::selection-mode property specifies the selection mode of
   * icon view. If the mode is #BTK_SELECTION_MULTIPLE, rubberband selection
   * is enabled, for the other modes, only keyboard selection is possible.
   *
   * Since: 2.6
   */
  g_object_class_install_property (bobject_class,
				   PROP_SELECTION_MODE,
				   g_param_spec_enum ("selection-mode",
						      P_("Selection mode"),
						      P_("The selection mode"),
						      BTK_TYPE_SELECTION_MODE,
						      BTK_SELECTION_SINGLE,
						      BTK_PARAM_READWRITE));

  /**
   * BtkIconView:pixbuf-column:
   *
   * The ::pixbuf-column property contains the number of the model column
   * containing the pixbufs which are displayed. The pixbuf column must be 
   * of type #BDK_TYPE_PIXBUF. Setting this property to -1 turns off the
   * display of pixbufs.
   *
   * Since: 2.6
   */
  g_object_class_install_property (bobject_class,
				   PROP_PIXBUF_COLUMN,
				   g_param_spec_int ("pixbuf-column",
						     P_("Pixbuf column"),
						     P_("Model column used to retrieve the icon pixbuf from"),
						     -1, B_MAXINT, -1,
						     BTK_PARAM_READWRITE));

  /**
   * BtkIconView:text-column:
   *
   * The ::text-column property contains the number of the model column
   * containing the texts which are displayed. The text column must be 
   * of type #B_TYPE_STRING. If this property and the :markup-column 
   * property are both set to -1, no texts are displayed.   
   *
   * Since: 2.6
   */
  g_object_class_install_property (bobject_class,
				   PROP_TEXT_COLUMN,
				   g_param_spec_int ("text-column",
						     P_("Text column"),
						     P_("Model column used to retrieve the text from"),
						     -1, B_MAXINT, -1,
						     BTK_PARAM_READWRITE));

  
  /**
   * BtkIconView:markup-column:
   *
   * The ::markup-column property contains the number of the model column
   * containing markup information to be displayed. The markup column must be 
   * of type #B_TYPE_STRING. If this property and the :text-column property 
   * are both set to column numbers, it overrides the text column.
   * If both are set to -1, no texts are displayed.   
   *
   * Since: 2.6
   */
  g_object_class_install_property (bobject_class,
				   PROP_MARKUP_COLUMN,
				   g_param_spec_int ("markup-column",
						     P_("Markup column"),
						     P_("Model column used to retrieve the text if using Bango markup"),
						     -1, B_MAXINT, -1,
						     BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_MODEL,
                                   g_param_spec_object ("model",
							P_("Icon View Model"),
							P_("The model for the icon view"),
							BTK_TYPE_TREE_MODEL,
							BTK_PARAM_READWRITE));
  
  /**
   * BtkIconView:columns:
   *
   * The columns property contains the number of the columns in which the
   * items should be displayed. If it is -1, the number of columns will
   * be chosen automatically to fill the available area.
   *
   * Since: 2.6
   */
  g_object_class_install_property (bobject_class,
				   PROP_COLUMNS,
				   g_param_spec_int ("columns",
						     P_("Number of columns"),
						     P_("Number of columns to display"),
						     -1, B_MAXINT, -1,
						     BTK_PARAM_READWRITE));
  

  /**
   * BtkIconView:item-width:
   *
   * The item-width property specifies the width to use for each item. 
   * If it is set to -1, the icon view will automatically determine a 
   * suitable item size.
   *
   * Since: 2.6
   */
  g_object_class_install_property (bobject_class,
				   PROP_ITEM_WIDTH,
				   g_param_spec_int ("item-width",
						     P_("Width for each item"),
						     P_("The width used for each item"),
						     -1, B_MAXINT, -1,
						     BTK_PARAM_READWRITE));  

  /**
   * BtkIconView:spacing:
   *
   * The spacing property specifies the space which is inserted between
   * the cells (i.e. the icon and the text) of an item.
   *
   * Since: 2.6
   */
  g_object_class_install_property (bobject_class,
                                   PROP_SPACING,
                                   g_param_spec_int ("spacing",
						     P_("Spacing"),
						     P_("Space which is inserted between cells of an item"),
						     0, B_MAXINT, 0,
						     BTK_PARAM_READWRITE));

  /**
   * BtkIconView:row-spacing:
   *
   * The row-spacing property specifies the space which is inserted between
   * the rows of the icon view.
   *
   * Since: 2.6
   */
  g_object_class_install_property (bobject_class,
                                   PROP_ROW_SPACING,
                                   g_param_spec_int ("row-spacing",
						     P_("Row Spacing"),
						     P_("Space which is inserted between grid rows"),
						     0, B_MAXINT, 6,
						     BTK_PARAM_READWRITE));

  /**
   * BtkIconView:column-spacing:
   *
   * The column-spacing property specifies the space which is inserted between
   * the columns of the icon view.
   *
   * Since: 2.6
   */
  g_object_class_install_property (bobject_class,
                                   PROP_COLUMN_SPACING,
                                   g_param_spec_int ("column-spacing",
						     P_("Column Spacing"),
						     P_("Space which is inserted between grid columns"),
						     0, B_MAXINT, 6,
						     BTK_PARAM_READWRITE));

  /**
   * BtkIconView:margin:
   *
   * The margin property specifies the space which is inserted 
   * at the edges of the icon view.
   *
   * Since: 2.6
   */
  g_object_class_install_property (bobject_class,
                                   PROP_MARGIN,
                                   g_param_spec_int ("margin",
						     P_("Margin"),
						     P_("Space which is inserted at the edges of the icon view"),
						     0, B_MAXINT, 6,
						     BTK_PARAM_READWRITE));

  /**
   * BtkIconView:orientation:
   *
   * The orientation property specifies how the cells (i.e. the icon and 
   * the text) of the item are positioned relative to each other.
   *
   * Since: 2.6
   *
   * Deprecated: 2.22: Use the #BtkIconView::item-orientation property
   */
  g_object_class_install_property (bobject_class,
				   PROP_ORIENTATION,
				   g_param_spec_enum ("orientation",
						      P_("Orientation"),
						      P_("How the text and icon of each item are positioned relative to each other"),
						      BTK_TYPE_ORIENTATION,
						      BTK_ORIENTATION_VERTICAL,
						      BTK_PARAM_READWRITE | G_PARAM_DEPRECATED));

  /**
   * BtkIconView:item-orientation:
   *
   * The item-orientation property specifies how the cells (i.e. the icon and
   * the text) of the item are positioned relative to each other.
   *
   * Since: 2.22
   */
  g_object_class_install_property (bobject_class,
                                   PROP_ITEM_ORIENTATION,
                                   g_param_spec_enum ("item-orientation",
                                                      P_("Item Orientation"),
                                                      P_("How the text and icon of each item are positioned relative to each other"),
                                                      BTK_TYPE_ORIENTATION,
                                                      BTK_ORIENTATION_VERTICAL,
                                                      BTK_PARAM_READWRITE));

  /**
   * BtkIconView:reorderable:
   *
   * The reorderable property specifies if the items can be reordered
   * by DND.
   *
   * Since: 2.8
   */
  g_object_class_install_property (bobject_class,
                                   PROP_REORDERABLE,
                                   g_param_spec_boolean ("reorderable",
							 P_("Reorderable"),
							 P_("View is reorderable"),
							 FALSE,
							 G_PARAM_READWRITE));

    g_object_class_install_property (bobject_class,
                                     PROP_TOOLTIP_COLUMN,
                                     g_param_spec_int ("tooltip-column",
                                                       P_("Tooltip Column"),
                                                       P_("The column in the model containing the tooltip texts for the items"),
                                                       -1,
                                                       B_MAXINT,
                                                       -1,
                                                       BTK_PARAM_READWRITE));

  /**
   * BtkIconView:item-padding:
   *
   * The item-padding property specifies the padding around each
   * of the icon view's item.
   *
   * Since: 2.18
   */
  g_object_class_install_property (bobject_class,
                                   PROP_ITEM_PADDING,
                                   g_param_spec_int ("item-padding",
						     P_("Item Padding"),
						     P_("Padding around icon view items"),
						     0, B_MAXINT, 6,
						     BTK_PARAM_READWRITE));



  /* Style properties */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boxed ("selection-box-color",
                                                               P_("Selection Box Color"),
                                                               P_("Color of the selection box"),
                                                               BDK_TYPE_COLOR,
                                                               BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_uchar ("selection-box-alpha",
                                                               P_("Selection Box Alpha"),
                                                               P_("Opacity of the selection box"),
                                                               0, 0xff,
                                                               0x40,
                                                               BTK_PARAM_READABLE));

  /* Signals */
  /**
   * BtkIconView::set-scroll-adjustments
   * @horizontal: the horizontal #BtkAdjustment
   * @vertical: the vertical #BtkAdjustment
   *
   * Set the scroll adjustments for the icon view. Usually scrolled containers
   * like #BtkScrolledWindow will emit this signal to connect two instances
   * of #BtkScrollbar to the scroll directions of the #BtkIconView.
   */
  widget_class->set_scroll_adjustments_signal =
    g_signal_new (I_("set-scroll-adjustments"),
		  B_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkIconViewClass, set_scroll_adjustments),
		  NULL, NULL, 
		  _btk_marshal_VOID__OBJECT_OBJECT,
		  B_TYPE_NONE, 2,
		  BTK_TYPE_ADJUSTMENT, BTK_TYPE_ADJUSTMENT);

  /**
   * BtkIconView::item-activated:
   * @iconview: the object on which the signal is emitted
   * @path: the #BtkTreePath for the activated item
   *
   * The ::item-activated signal is emitted when the method
   * btk_icon_view_item_activated() is called or the user double 
   * clicks an item. It is also emitted when a non-editable item
   * is selected and one of the keys: Space, Return or Enter is
   * pressed.
   */
  icon_view_signals[ITEM_ACTIVATED] =
    g_signal_new (I_("item-activated"),
		  B_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkIconViewClass, item_activated),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__BOXED,
		  B_TYPE_NONE, 1,
		  BTK_TYPE_TREE_PATH);

  /**
   * BtkIconView::selection-changed:
   * @iconview: the object on which the signal is emitted
   *
   * The ::selection-changed signal is emitted when the selection
   * (i.e. the set of selected items) changes.
   */
  icon_view_signals[SELECTION_CHANGED] =
    g_signal_new (I_("selection-changed"),
		  B_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkIconViewClass, selection_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
  
  /**
   * BtkIconView::select-all:
   * @iconview: the object on which the signal is emitted
   *
   * A <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted when the user selects all items.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control selection
   * programmatically.
   * 
   * The default binding for this signal is Ctrl-a.
   */
  icon_view_signals[SELECT_ALL] =
    g_signal_new (I_("select-all"),
		  B_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkIconViewClass, select_all),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
  
  /**
   * BtkIconView::unselect-all:
   * @iconview: the object on which the signal is emitted
   *
   * A <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted when the user unselects all items.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control selection
   * programmatically.
   * 
   * The default binding for this signal is Ctrl-Shift-a. 
   */
  icon_view_signals[UNSELECT_ALL] =
    g_signal_new (I_("unselect-all"),
		  B_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkIconViewClass, unselect_all),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);

  /**
   * BtkIconView::select-cursor-item:
   * @iconview: the object on which the signal is emitted
   *
   * A <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted when the user selects the item that is currently
   * focused.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control selection
   * programmatically.
   * 
   * There is no default binding for this signal.
   */
  icon_view_signals[SELECT_CURSOR_ITEM] =
    g_signal_new (I_("select-cursor-item"),
		  B_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkIconViewClass, select_cursor_item),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);

  /**
   * BtkIconView::toggle-cursor-item:
   * @iconview: the object on which the signal is emitted
   *
   * A <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted when the user toggles whether the currently
   * focused item is selected or not. The exact effect of this 
   * depend on the selection mode.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control selection
   * programmatically.
   * 
   * There is no default binding for this signal is Ctrl-Space.
   */
  icon_view_signals[TOGGLE_CURSOR_ITEM] =
    g_signal_new (I_("toggle-cursor-item"),
		  B_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkIconViewClass, toggle_cursor_item),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);

  /**
   * BtkIconView::activate-cursor-item:
   * @iconview: the object on which the signal is emitted
   *
   * A <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted when the user activates the currently 
   * focused item. 
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control activation
   * programmatically.
   * 
   * The default bindings for this signal are Space, Return and Enter.
   */
  icon_view_signals[ACTIVATE_CURSOR_ITEM] =
    g_signal_new (I_("activate-cursor-item"),
		  B_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkIconViewClass, activate_cursor_item),
		  NULL, NULL,
		  _btk_marshal_BOOLEAN__VOID,
		  B_TYPE_BOOLEAN, 0);
  
  /**
   * BtkIconView::move-cursor:
   * @iconview: the object which received the signal
   * @step: the granularity of the move, as a #BtkMovementStep
   * @count: the number of @step units to move
   *
   * The ::move-cursor signal is a
   * <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted when the user initiates a cursor movement.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control the cursor
   * programmatically.
   *
   * The default bindings for this signal include
   * <itemizedlist>
   * <listitem>Arrow keys which move by individual steps</listitem>
   * <listitem>Home/End keys which move to the first/last item</listitem>
   * <listitem>PageUp/PageDown which move by "pages"</listitem>
   * </itemizedlist>
   *
   * All of these will extend the selection when combined with
   * the Shift modifier.
   */
  icon_view_signals[MOVE_CURSOR] =
    g_signal_new (I_("move-cursor"),
		  B_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkIconViewClass, move_cursor),
		  NULL, NULL,
		  _btk_marshal_BOOLEAN__ENUM_INT,
		  B_TYPE_BOOLEAN, 2,
		  BTK_TYPE_MOVEMENT_STEP,
		  B_TYPE_INT);

  /* Key bindings */
  btk_binding_entry_add_signal (binding_set, BDK_a, BDK_CONTROL_MASK, 
				"select-all", 0);
  btk_binding_entry_add_signal (binding_set, BDK_a, BDK_CONTROL_MASK | BDK_SHIFT_MASK, 
				"unselect-all", 0);
  btk_binding_entry_add_signal (binding_set, BDK_space, BDK_CONTROL_MASK, 
				"toggle-cursor-item", 0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Space, BDK_CONTROL_MASK,
				"toggle-cursor-item", 0);

  btk_binding_entry_add_signal (binding_set, BDK_space, 0, 
				"activate-cursor-item", 0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Space, 0,
				"activate-cursor-item", 0);
  btk_binding_entry_add_signal (binding_set, BDK_Return, 0, 
				"activate-cursor-item", 0);
  btk_binding_entry_add_signal (binding_set, BDK_ISO_Enter, 0, 
				"activate-cursor-item", 0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Enter, 0, 
				"activate-cursor-item", 0);

  btk_icon_view_add_move_binding (binding_set, BDK_Up, 0,
				  BTK_MOVEMENT_DISPLAY_LINES, -1);
  btk_icon_view_add_move_binding (binding_set, BDK_KP_Up, 0,
				  BTK_MOVEMENT_DISPLAY_LINES, -1);

  btk_icon_view_add_move_binding (binding_set, BDK_Down, 0,
				  BTK_MOVEMENT_DISPLAY_LINES, 1);
  btk_icon_view_add_move_binding (binding_set, BDK_KP_Down, 0,
				  BTK_MOVEMENT_DISPLAY_LINES, 1);

  btk_icon_view_add_move_binding (binding_set, BDK_p, BDK_CONTROL_MASK,
				  BTK_MOVEMENT_DISPLAY_LINES, -1);

  btk_icon_view_add_move_binding (binding_set, BDK_n, BDK_CONTROL_MASK,
				  BTK_MOVEMENT_DISPLAY_LINES, 1);

  btk_icon_view_add_move_binding (binding_set, BDK_Home, 0,
				  BTK_MOVEMENT_BUFFER_ENDS, -1);
  btk_icon_view_add_move_binding (binding_set, BDK_KP_Home, 0,
				  BTK_MOVEMENT_BUFFER_ENDS, -1);

  btk_icon_view_add_move_binding (binding_set, BDK_End, 0,
				  BTK_MOVEMENT_BUFFER_ENDS, 1);
  btk_icon_view_add_move_binding (binding_set, BDK_KP_End, 0,
				  BTK_MOVEMENT_BUFFER_ENDS, 1);

  btk_icon_view_add_move_binding (binding_set, BDK_Page_Up, 0,
				  BTK_MOVEMENT_PAGES, -1);
  btk_icon_view_add_move_binding (binding_set, BDK_KP_Page_Up, 0,
				  BTK_MOVEMENT_PAGES, -1);

  btk_icon_view_add_move_binding (binding_set, BDK_Page_Down, 0,
				  BTK_MOVEMENT_PAGES, 1);
  btk_icon_view_add_move_binding (binding_set, BDK_KP_Page_Down, 0,
				  BTK_MOVEMENT_PAGES, 1);

  btk_icon_view_add_move_binding (binding_set, BDK_Right, 0, 
				  BTK_MOVEMENT_VISUAL_POSITIONS, 1);
  btk_icon_view_add_move_binding (binding_set, BDK_Left, 0, 
				  BTK_MOVEMENT_VISUAL_POSITIONS, -1);

  btk_icon_view_add_move_binding (binding_set, BDK_KP_Right, 0, 
				  BTK_MOVEMENT_VISUAL_POSITIONS, 1);
  btk_icon_view_add_move_binding (binding_set, BDK_KP_Left, 0, 
				  BTK_MOVEMENT_VISUAL_POSITIONS, -1);
}

static void
btk_icon_view_buildable_init (BtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = _btk_cell_layout_buildable_add_child;
  iface->custom_tag_start = btk_icon_view_buildable_custom_tag_start;
  iface->custom_tag_end = btk_icon_view_buildable_custom_tag_end;
}

static void
btk_icon_view_cell_layout_init (BtkCellLayoutIface *iface)
{
  iface->pack_start = btk_icon_view_cell_layout_pack_start;
  iface->pack_end = btk_icon_view_cell_layout_pack_end;
  iface->clear = btk_icon_view_cell_layout_clear;
  iface->add_attribute = btk_icon_view_cell_layout_add_attribute;
  iface->set_cell_data_func = btk_icon_view_cell_layout_set_cell_data_func;
  iface->clear_attributes = btk_icon_view_cell_layout_clear_attributes;
  iface->reorder = btk_icon_view_cell_layout_reorder;
  iface->get_cells = btk_icon_view_cell_layout_get_cells;
}

static void
btk_icon_view_init (BtkIconView *icon_view)
{
  icon_view->priv = BTK_ICON_VIEW_GET_PRIVATE (icon_view);
  
  icon_view->priv->width = 0;
  icon_view->priv->height = 0;
  icon_view->priv->selection_mode = BTK_SELECTION_SINGLE;
  icon_view->priv->pressed_button = -1;
  icon_view->priv->press_start_x = -1;
  icon_view->priv->press_start_y = -1;
  icon_view->priv->text_column = -1;
  icon_view->priv->markup_column = -1;  
  icon_view->priv->pixbuf_column = -1;
  icon_view->priv->text_cell = -1;
  icon_view->priv->pixbuf_cell = -1;  
  icon_view->priv->tooltip_column = -1;  

  btk_widget_set_can_focus (BTK_WIDGET (icon_view), TRUE);
  
  btk_icon_view_set_adjustments (icon_view, NULL, NULL);

  icon_view->priv->cell_list = NULL;
  icon_view->priv->n_cells = 0;
  icon_view->priv->cursor_cell = -1;

  icon_view->priv->item_orientation = BTK_ORIENTATION_VERTICAL;

  icon_view->priv->columns = -1;
  icon_view->priv->item_width = -1;
  icon_view->priv->spacing = 0;
  icon_view->priv->row_spacing = 6;
  icon_view->priv->column_spacing = 6;
  icon_view->priv->margin = 6;
  icon_view->priv->item_padding = 6;

  icon_view->priv->draw_focus = TRUE;
}

static void
btk_icon_view_destroy (BtkObject *object)
{
  BtkIconView *icon_view;

  icon_view = BTK_ICON_VIEW (object);
  
  btk_icon_view_stop_editing (icon_view, TRUE);

  btk_icon_view_set_model (icon_view, NULL);
  
  if (icon_view->priv->layout_idle_id != 0)
    {
      g_source_remove (icon_view->priv->layout_idle_id);
      icon_view->priv->layout_idle_id = 0;
    }

  if (icon_view->priv->scroll_to_path != NULL)
    {
      btk_tree_row_reference_free (icon_view->priv->scroll_to_path);
      icon_view->priv->scroll_to_path = NULL;
    }

  remove_scroll_timeout (icon_view);

  if (icon_view->priv->hadjustment != NULL)
    {
      g_object_unref (icon_view->priv->hadjustment);
      icon_view->priv->hadjustment = NULL;
    }

  if (icon_view->priv->vadjustment != NULL)
    {
      g_object_unref (icon_view->priv->vadjustment);
      icon_view->priv->vadjustment = NULL;
    }
  
  BTK_OBJECT_CLASS (btk_icon_view_parent_class)->destroy (object);
}

/* BObject methods */
static void
btk_icon_view_finalize (BObject *object)
{
  btk_icon_view_cell_layout_clear (BTK_CELL_LAYOUT (object));

  B_OBJECT_CLASS (btk_icon_view_parent_class)->finalize (object);
}


static void
btk_icon_view_set_property (BObject      *object,
			    buint         prop_id,
			    const BValue *value,
			    BParamSpec   *pspec)
{
  BtkIconView *icon_view;

  icon_view = BTK_ICON_VIEW (object);

  switch (prop_id)
    {
    case PROP_SELECTION_MODE:
      btk_icon_view_set_selection_mode (icon_view, b_value_get_enum (value));
      break;
    case PROP_PIXBUF_COLUMN:
      btk_icon_view_set_pixbuf_column (icon_view, b_value_get_int (value));
      break;
    case PROP_TEXT_COLUMN:
      btk_icon_view_set_text_column (icon_view, b_value_get_int (value));
      break;
    case PROP_MARKUP_COLUMN:
      btk_icon_view_set_markup_column (icon_view, b_value_get_int (value));
      break;
    case PROP_MODEL:
      btk_icon_view_set_model (icon_view, b_value_get_object (value));
      break;
    case PROP_ORIENTATION:
    case PROP_ITEM_ORIENTATION:
      btk_icon_view_set_item_orientation (icon_view, b_value_get_enum (value));
      break;
    case PROP_COLUMNS:
      btk_icon_view_set_columns (icon_view, b_value_get_int (value));
      break;
    case PROP_ITEM_WIDTH:
      btk_icon_view_set_item_width (icon_view, b_value_get_int (value));
      break;
    case PROP_SPACING:
      btk_icon_view_set_spacing (icon_view, b_value_get_int (value));
      break;
    case PROP_ROW_SPACING:
      btk_icon_view_set_row_spacing (icon_view, b_value_get_int (value));
      break;
    case PROP_COLUMN_SPACING:
      btk_icon_view_set_column_spacing (icon_view, b_value_get_int (value));
      break;
    case PROP_MARGIN:
      btk_icon_view_set_margin (icon_view, b_value_get_int (value));
      break;
    case PROP_REORDERABLE:
      btk_icon_view_set_reorderable (icon_view, b_value_get_boolean (value));
      break;
      
    case PROP_TOOLTIP_COLUMN:
      btk_icon_view_set_tooltip_column (icon_view, b_value_get_int (value));
      break;

    case PROP_ITEM_PADDING:
      btk_icon_view_set_item_padding (icon_view, b_value_get_int (value));
      break;

    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_icon_view_get_property (BObject      *object,
			    buint         prop_id,
			    BValue       *value,
			    BParamSpec   *pspec)
{
  BtkIconView *icon_view;

  icon_view = BTK_ICON_VIEW (object);

  switch (prop_id)
    {
    case PROP_SELECTION_MODE:
      b_value_set_enum (value, icon_view->priv->selection_mode);
      break;
    case PROP_PIXBUF_COLUMN:
      b_value_set_int (value, icon_view->priv->pixbuf_column);
      break;
    case PROP_TEXT_COLUMN:
      b_value_set_int (value, icon_view->priv->text_column);
      break;
    case PROP_MARKUP_COLUMN:
      b_value_set_int (value, icon_view->priv->markup_column);
      break;
    case PROP_MODEL:
      b_value_set_object (value, icon_view->priv->model);
      break;
    case PROP_ORIENTATION:
    case PROP_ITEM_ORIENTATION:
      b_value_set_enum (value, icon_view->priv->item_orientation);
      break;
    case PROP_COLUMNS:
      b_value_set_int (value, icon_view->priv->columns);
      break;
    case PROP_ITEM_WIDTH:
      b_value_set_int (value, icon_view->priv->item_width);
      break;
    case PROP_SPACING:
      b_value_set_int (value, icon_view->priv->spacing);
      break;
    case PROP_ROW_SPACING:
      b_value_set_int (value, icon_view->priv->row_spacing);
      break;
    case PROP_COLUMN_SPACING:
      b_value_set_int (value, icon_view->priv->column_spacing);
      break;
    case PROP_MARGIN:
      b_value_set_int (value, icon_view->priv->margin);
      break;
    case PROP_REORDERABLE:
      b_value_set_boolean (value, icon_view->priv->reorderable);
      break;
    case PROP_TOOLTIP_COLUMN:
      b_value_set_int (value, icon_view->priv->tooltip_column);
      break;

    case PROP_ITEM_PADDING:
      b_value_set_int (value, icon_view->priv->item_padding);
      break;

    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* BtkWidget signals */
static void
btk_icon_view_realize (BtkWidget *widget)
{
  BtkIconView *icon_view;
  BdkWindowAttr attributes;
  bint attributes_mask;
  
  icon_view = BTK_ICON_VIEW (widget);

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
  bdk_window_set_back_pixmap (widget->window, NULL, FALSE);
  bdk_window_set_user_data (widget->window, widget);

  /* Make the window for the icon view */
  attributes.x = 0;
  attributes.y = 0;
  attributes.width = MAX (icon_view->priv->width, widget->allocation.width);
  attributes.height = MAX (icon_view->priv->height, widget->allocation.height);
  attributes.event_mask = (BDK_EXPOSURE_MASK |
			   BDK_SCROLL_MASK |
			   BDK_POINTER_MOTION_MASK |
			   BDK_BUTTON_PRESS_MASK |
			   BDK_BUTTON_RELEASE_MASK |
			   BDK_KEY_PRESS_MASK |
			   BDK_KEY_RELEASE_MASK) |
    btk_widget_get_events (widget);
  
  icon_view->priv->bin_window = bdk_window_new (widget->window,
						&attributes, attributes_mask);
  bdk_window_set_user_data (icon_view->priv->bin_window, widget);

  widget->style = btk_style_attach (widget->style, widget->window);
  bdk_window_set_background (icon_view->priv->bin_window, &widget->style->base[widget->state]);

  bdk_window_show (icon_view->priv->bin_window);
}

static void
btk_icon_view_unrealize (BtkWidget *widget)
{
  BtkIconView *icon_view;

  icon_view = BTK_ICON_VIEW (widget);

  bdk_window_set_user_data (icon_view->priv->bin_window, NULL);
  bdk_window_destroy (icon_view->priv->bin_window);
  icon_view->priv->bin_window = NULL;

  BTK_WIDGET_CLASS (btk_icon_view_parent_class)->unrealize (widget);
}

static void
btk_icon_view_state_changed (BtkWidget      *widget,
		 	     BtkStateType    previous_state)
{
  BtkIconView *icon_view = BTK_ICON_VIEW (widget);

  if (btk_widget_get_realized (widget))
    {
      bdk_window_set_background (widget->window, &widget->style->base[widget->state]);
      bdk_window_set_background (icon_view->priv->bin_window, &widget->style->base[widget->state]);
    }

  btk_widget_queue_draw (widget);
}

static void
btk_icon_view_style_set (BtkWidget *widget,
			 BtkStyle *previous_style)
{
  BtkIconView *icon_view = BTK_ICON_VIEW (widget);

  if (btk_widget_get_realized (widget))
    {
      bdk_window_set_background (widget->window, &widget->style->base[widget->state]);
      bdk_window_set_background (icon_view->priv->bin_window, &widget->style->base[widget->state]);
    }

  btk_widget_queue_resize (widget);
}

static void
btk_icon_view_size_request (BtkWidget      *widget,
			    BtkRequisition *requisition)
{
  BtkIconView *icon_view = BTK_ICON_VIEW (widget);
  GList *tmp_list;

  requisition->width = icon_view->priv->width;
  requisition->height = icon_view->priv->height;

  tmp_list = icon_view->priv->children;

  while (tmp_list)
    {
      BtkIconViewChild *child = tmp_list->data;
      BtkRequisition child_requisition;

      tmp_list = tmp_list->next;

      if (btk_widget_get_visible (child->widget))
        btk_widget_size_request (child->widget, &child_requisition);
    }  
}

static void
btk_icon_view_allocate_children (BtkIconView *icon_view)
{
  GList *list;

  for (list = icon_view->priv->children; list; list = list->next)
    {
      BtkIconViewChild *child = list->data;
      BtkAllocation allocation;

      /* totally ignore our child's requisition */
      if (child->cell < 0)
	{
	  allocation.x = child->item->x + icon_view->priv->item_padding;
	  allocation.y = child->item->y + icon_view->priv->item_padding;
	  allocation.width = child->item->width - icon_view->priv->item_padding * 2;
	  allocation.height = child->item->height - icon_view->priv->item_padding * 2;
	}
      else
	{
	  BdkRectangle *box = &child->item->box[child->cell];

	  allocation.x = box->x;
	  allocation.y = box->y;
	  allocation.width = box->width;
	  allocation.height = box->height;
	}

      btk_widget_size_allocate (child->widget, &allocation);
    }
}

static void
btk_icon_view_size_allocate (BtkWidget      *widget,
			     BtkAllocation  *allocation)
{
  BtkIconView *icon_view = BTK_ICON_VIEW (widget);

  BtkAdjustment *hadjustment, *vadjustment;

  widget->allocation = *allocation;
  
  if (btk_widget_get_realized (widget))
    {
      bdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);
      bdk_window_resize (icon_view->priv->bin_window,
			 MAX (icon_view->priv->width, allocation->width),
			 MAX (icon_view->priv->height, allocation->height));
    }

  btk_icon_view_layout (icon_view);
  
  btk_icon_view_allocate_children (icon_view);

  hadjustment = icon_view->priv->hadjustment;
  vadjustment = icon_view->priv->vadjustment;

  hadjustment->page_size = allocation->width;
  hadjustment->page_increment = allocation->width * 0.9;
  hadjustment->step_increment = allocation->width * 0.1;
  hadjustment->lower = 0;
  hadjustment->upper = MAX (allocation->width, icon_view->priv->width);

  if (hadjustment->value > hadjustment->upper - hadjustment->page_size)
    btk_adjustment_set_value (hadjustment, MAX (0, hadjustment->upper - hadjustment->page_size));

  vadjustment->page_size = allocation->height;
  vadjustment->page_increment = allocation->height * 0.9;
  vadjustment->step_increment = allocation->height * 0.1;
  vadjustment->lower = 0;
  vadjustment->upper = MAX (allocation->height, icon_view->priv->height);

  if (vadjustment->value > vadjustment->upper - vadjustment->page_size)
    btk_adjustment_set_value (vadjustment, MAX (0, vadjustment->upper - vadjustment->page_size));

  if (btk_widget_get_realized (widget) &&
      icon_view->priv->scroll_to_path)
    {
      BtkTreePath *path;
      path = btk_tree_row_reference_get_path (icon_view->priv->scroll_to_path);
      btk_tree_row_reference_free (icon_view->priv->scroll_to_path);
      icon_view->priv->scroll_to_path = NULL;

      btk_icon_view_scroll_to_path (icon_view, path,
				    icon_view->priv->scroll_to_use_align,
				    icon_view->priv->scroll_to_row_align,
				    icon_view->priv->scroll_to_col_align);
      btk_tree_path_free (path);
    }
  else
    {
      btk_adjustment_changed (hadjustment);
      btk_adjustment_changed (vadjustment);
    }
}

static bboolean
btk_icon_view_expose (BtkWidget *widget,
		      BdkEventExpose *expose)
{
  BtkIconView *icon_view;
  GList *icons;
  bairo_t *cr;
  BtkTreePath *path;
  bint dest_index;
  BtkIconViewDropPosition dest_pos;
  BtkIconViewItem *dest_item = NULL;

  icon_view = BTK_ICON_VIEW (widget);

  if (expose->window != icon_view->priv->bin_window)
    return FALSE;

  /* If a layout has been scheduled, do it now so that all
   * cell view items have valid sizes before we proceed. */
  if (icon_view->priv->layout_idle_id != 0)
    btk_icon_view_layout (icon_view);

  cr = bdk_bairo_create (icon_view->priv->bin_window);
  bairo_set_line_width (cr, 1.);

  btk_icon_view_get_drag_dest_item (icon_view, &path, &dest_pos);

  if (path)
    {
      dest_index = btk_tree_path_get_indices (path)[0];
      btk_tree_path_free (path);
    }
  else
    dest_index = -1;

  for (icons = icon_view->priv->items; icons; icons = icons->next) 
    {
      BtkIconViewItem *item = icons->data;
      BdkRectangle area;
      
      area.x = item->x;
      area.y = item->y;
      area.width = item->width;
      area.height = item->height;
	
      if (bdk_rebunnyion_rect_in (expose->rebunnyion, &area) == BDK_OVERLAP_RECTANGLE_OUT)
	continue;
      
      btk_icon_view_paint_item (icon_view, cr, item, &expose->area, 
				icon_view->priv->bin_window,
				item->x, item->y,
				icon_view->priv->draw_focus); 
 
      if (dest_index == item->index)
	dest_item = item;
    }

  if (dest_item)
    {
      switch (dest_pos)
	{
	case BTK_ICON_VIEW_DROP_INTO:
	  btk_paint_focus (widget->style,
			   icon_view->priv->bin_window,
			   btk_widget_get_state (widget),
			   NULL,
			   widget,
			   "iconview-drop-indicator",
			   dest_item->x, dest_item->y,
			   dest_item->width, dest_item->height);
	  break;
	case BTK_ICON_VIEW_DROP_ABOVE:
	  btk_paint_focus (widget->style,
			   icon_view->priv->bin_window,
			   btk_widget_get_state (widget),
			   NULL,
			   widget,
			   "iconview-drop-indicator",
			   dest_item->x, dest_item->y - 1,
			   dest_item->width, 2);
	  break;
	case BTK_ICON_VIEW_DROP_LEFT:
	  btk_paint_focus (widget->style,
			   icon_view->priv->bin_window,
			   btk_widget_get_state (widget),
			   NULL,
			   widget,
			   "iconview-drop-indicator",
			   dest_item->x - 1, dest_item->y,
			   2, dest_item->height);
	  break;
	case BTK_ICON_VIEW_DROP_BELOW:
	  btk_paint_focus (widget->style,
			   icon_view->priv->bin_window,
			   btk_widget_get_state (widget),
			   NULL,
			   widget,
			   "iconview-drop-indicator",
			   dest_item->x, dest_item->y + dest_item->height - 1,
			   dest_item->width, 2);
	  break;
	case BTK_ICON_VIEW_DROP_RIGHT:
	  btk_paint_focus (widget->style,
			   icon_view->priv->bin_window,
			   btk_widget_get_state (widget),
			   NULL,
			   widget,
			   "iconview-drop-indicator",
			   dest_item->x + dest_item->width - 1, dest_item->y,
			   2, dest_item->height);
	case BTK_ICON_VIEW_NO_DROP: ;
	  break;
	}
    }
  
  if (icon_view->priv->doing_rubberband)
    {
      BdkRectangle *rectangles;
      bint n_rectangles;
      
      bdk_rebunnyion_get_rectangles (expose->rebunnyion,
				 &rectangles,
				 &n_rectangles);
      
      while (n_rectangles--)
	btk_icon_view_paint_rubberband (icon_view, cr, &rectangles[n_rectangles]);

      g_free (rectangles);
    }

  bairo_destroy (cr);

  BTK_WIDGET_CLASS (btk_icon_view_parent_class)->expose_event (widget, expose);

  return TRUE;
}

static bboolean
rubberband_scroll_timeout (bpointer data)
{
  BtkIconView *icon_view;
  bdouble value;

  icon_view = data;

  value = MIN (icon_view->priv->vadjustment->value +
	       icon_view->priv->scroll_value_diff,
	       icon_view->priv->vadjustment->upper -
	       icon_view->priv->vadjustment->page_size);

  btk_adjustment_set_value (icon_view->priv->vadjustment, value);

  btk_icon_view_update_rubberband (icon_view);
  
  return TRUE;
}

static bboolean
btk_icon_view_motion (BtkWidget      *widget,
		      BdkEventMotion *event)
{
  BtkIconView *icon_view;
  bint abs_y;
  
  icon_view = BTK_ICON_VIEW (widget);

  btk_icon_view_maybe_begin_drag (icon_view, event);

  if (icon_view->priv->doing_rubberband)
    {
      btk_icon_view_update_rubberband (widget);
      
      abs_y = event->y - icon_view->priv->height *
	(icon_view->priv->vadjustment->value /
	 (icon_view->priv->vadjustment->upper -
	  icon_view->priv->vadjustment->lower));

      if (abs_y < 0 || abs_y > widget->allocation.height)
	{
	  if (abs_y < 0)
	    icon_view->priv->scroll_value_diff = abs_y;
	  else
	    icon_view->priv->scroll_value_diff = abs_y - widget->allocation.height;

	  icon_view->priv->event_last_x = event->x;
	  icon_view->priv->event_last_y = event->y;

	  if (icon_view->priv->scroll_timeout_id == 0)
	    icon_view->priv->scroll_timeout_id = bdk_threads_add_timeout (30, rubberband_scroll_timeout, 
								icon_view);
 	}
      else 
	remove_scroll_timeout (icon_view);
    }
  
  return TRUE;
}

static void
btk_icon_view_remove (BtkContainer *container,
		      BtkWidget    *widget)
{
  BtkIconView *icon_view;
  BtkIconViewChild *child = NULL;
  GList *tmp_list;

  icon_view = BTK_ICON_VIEW (container);
  
  tmp_list = icon_view->priv->children;
  while (tmp_list)
    {
      child = tmp_list->data;
      if (child->widget == widget)
	{
	  btk_widget_unparent (widget);

	  icon_view->priv->children = g_list_remove_link (icon_view->priv->children, tmp_list);
	  g_list_free_1 (tmp_list);
	  g_free (child);
	  return;
	}

      tmp_list = tmp_list->next;
    }
}

static void
btk_icon_view_forall (BtkContainer *container,
		      bboolean      include_internals,
		      BtkCallback   callback,
		      bpointer      callback_data)
{
  BtkIconView *icon_view;
  BtkIconViewChild *child = NULL;
  GList *tmp_list;

  icon_view = BTK_ICON_VIEW (container);

  tmp_list = icon_view->priv->children;
  while (tmp_list)
    {
      child = tmp_list->data;
      tmp_list = tmp_list->next;

      (* callback) (child->widget, callback_data);
    }
}

static void
btk_icon_view_item_activate_cell (BtkIconView         *icon_view, 
				  BtkIconViewItem     *item, 
				  BtkIconViewCellInfo *info,
				  BdkEvent            *event)
{
  BtkTreePath *path;  
  bchar *path_string;
  BdkRectangle cell_area;
  bboolean visible, mode;

  btk_icon_view_set_cell_data (icon_view, item);

  g_object_get (info->cell,
		"visible", &visible,
		"mode", &mode,
		NULL);

  if (visible && mode == BTK_CELL_RENDERER_MODE_ACTIVATABLE)
    {
      btk_icon_view_get_cell_area (icon_view, item, info, &cell_area);

      path = btk_tree_path_new_from_indices (item->index, -1);
      path_string = btk_tree_path_to_string (path);
      btk_tree_path_free (path);

      btk_cell_renderer_activate (info->cell, 
				  event, 
				  BTK_WIDGET (icon_view),
				  path_string, 
				  &cell_area, 
				  &cell_area, 
				  0);

      g_free (path_string);      
    }
}

static void 
btk_icon_view_item_selected_changed (BtkIconView      *icon_view,
                                     BtkIconViewItem  *item)
{
  BatkObject *obj;
  BatkObject *item_obj;

  obj = btk_widget_get_accessible (BTK_WIDGET (icon_view));
  if (obj != NULL)
    {
      item_obj = batk_object_ref_accessible_child (obj, item->index);
      if (item_obj != NULL)
        {
          batk_object_notify_state_change (item_obj, BATK_STATE_SELECTED, item->selected);
          g_object_unref (item_obj);
        }
    }
}

static void 
btk_icon_view_put (BtkIconView     *icon_view,
		   BtkWidget       *widget,
		   BtkIconViewItem *item,
		   bint             cell)
{
  BtkIconViewChild *child;
  
  child = g_new (BtkIconViewChild, 1);
  
  child->widget = widget;
  child->item = item;
  child->cell = cell;

  icon_view->priv->children = g_list_append (icon_view->priv->children, child);

  if (btk_widget_get_realized (BTK_WIDGET (icon_view)))
    btk_widget_set_parent_window (child->widget, icon_view->priv->bin_window);
  
  btk_widget_set_parent (widget, BTK_WIDGET (icon_view));
}

static void
btk_icon_view_remove_widget (BtkCellEditable *editable,
			     BtkIconView     *icon_view)
{
  GList *l;
  BtkIconViewItem *item;

  if (icon_view->priv->edited_item == NULL)
    return;

  item = icon_view->priv->edited_item;
  icon_view->priv->edited_item = NULL;
  icon_view->priv->editable = NULL;
  for (l = icon_view->priv->cell_list; l; l = l->next)
    {
      BtkIconViewCellInfo *info = l->data;

      info->editing = FALSE;
    }

  if (btk_widget_has_focus (BTK_WIDGET (editable)))
    btk_widget_grab_focus (BTK_WIDGET (icon_view));
  
  g_signal_handlers_disconnect_by_func (editable,
					btk_icon_view_remove_widget,
					icon_view);

  btk_container_remove (BTK_CONTAINER (icon_view),
			BTK_WIDGET (editable));  

  btk_icon_view_queue_draw_item (icon_view, item);
}


static void
btk_icon_view_start_editing (BtkIconView         *icon_view, 
			     BtkIconViewItem     *item, 
			     BtkIconViewCellInfo *info,
			     BdkEvent            *event)
{
  BtkTreePath *path;  
  bchar *path_string;
  BdkRectangle cell_area;
  bboolean visible, mode;
  BtkCellEditable *editable;

  btk_icon_view_set_cell_data (icon_view, item);

  g_object_get (info->cell,
		"visible", &visible,
		"mode", &mode,
		NULL);
  if (visible && mode == BTK_CELL_RENDERER_MODE_EDITABLE)
    {
      btk_icon_view_get_cell_area (icon_view, item, info, &cell_area);

      path = btk_tree_path_new_from_indices (item->index, -1);
      path_string = btk_tree_path_to_string (path);
      btk_tree_path_free (path);

      editable = btk_cell_renderer_start_editing (info->cell, 
						  event, 
						  BTK_WIDGET (icon_view),
						  path_string, 
						  &cell_area, 
						  &cell_area, 
						  0);
      g_free (path_string);      

      /* the rest corresponds to tree_view_real_start_editing... */
      icon_view->priv->edited_item = item;
      icon_view->priv->editable = editable;
      info->editing = TRUE;

      btk_icon_view_put (icon_view, BTK_WIDGET (editable), item, 
			 info->position);
      btk_cell_editable_start_editing (BTK_CELL_EDITABLE (editable), 
				       (BdkEvent *)event);
      btk_widget_grab_focus (BTK_WIDGET (editable));
      g_signal_connect (editable, "remove-widget",
			G_CALLBACK (btk_icon_view_remove_widget), 
			icon_view);

    }
}

static void
btk_icon_view_stop_editing (BtkIconView *icon_view,
			    bboolean     cancel_editing)
{
  BtkCellRenderer *cell = NULL;
  BtkIconViewItem *item;
  GList *l;

  if (icon_view->priv->edited_item == NULL)
    return;

  /*
   * This is very evil. We need to do this, because
   * btk_cell_editable_editing_done may trigger btk_icon_view_row_changed
   * later on. If btk_icon_view_row_changed notices
   * icon_view->priv->edited_item != NULL, it'll call
   * btk_icon_view_stop_editing again. Bad things will happen then.
   *
   * Please read that again if you intend to modify anything here.
   */

  item = icon_view->priv->edited_item;
  icon_view->priv->edited_item = NULL;

  for (l = icon_view->priv->cell_list; l; l = l->next)
    {
      BtkIconViewCellInfo *info = l->data;

      if (info->editing)
	{
	  cell = info->cell;
	  break;
	}
    }

  if (cell == NULL)
    return;

  btk_cell_renderer_stop_editing (cell, cancel_editing);
  if (!cancel_editing)
    btk_cell_editable_editing_done (icon_view->priv->editable);

  icon_view->priv->edited_item = item;

  btk_cell_editable_remove_widget (icon_view->priv->editable);
}

/**
 * btk_icon_view_set_cursor:
 * @icon_view: A #BtkIconView
 * @path: A #BtkTreePath
 * @cell: (allow-none): One of the cell renderers of @icon_view, or %NULL
 * @start_editing: %TRUE if the specified cell should start being edited.
 *
 * Sets the current keyboard focus to be at @path, and selects it.  This is
 * useful when you want to focus the user's attention on a particular item.
 * If @cell is not %NULL, then focus is given to the cell specified by 
 * it. Additionally, if @start_editing is %TRUE, then editing should be 
 * started in the specified cell.  
 *
 * This function is often followed by <literal>btk_widget_grab_focus 
 * (icon_view)</literal> in order to give keyboard focus to the widget.  
 * Please note that editing can only happen when the widget is realized.
 *
 * Since: 2.8
 **/
void
btk_icon_view_set_cursor (BtkIconView     *icon_view,
			  BtkTreePath     *path,
			  BtkCellRenderer *cell,
			  bboolean         start_editing)
{
  BtkIconViewItem *item = NULL;
  BtkIconViewCellInfo *info =  NULL;
  GList *l;
  bint i, cell_pos;

  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));
  g_return_if_fail (path != NULL);
  g_return_if_fail (cell == NULL || BTK_IS_CELL_RENDERER (cell));

  btk_icon_view_stop_editing (icon_view, TRUE);

  if (btk_tree_path_get_depth (path) == 1)
    item = g_list_nth_data (icon_view->priv->items,
			    btk_tree_path_get_indices(path)[0]);
  
  if (!item)
    return;

  cell_pos = -1;
  for (l = icon_view->priv->cell_list, i = 0; l; l = l->next, i++)
    {
      info = l->data;
      
      if (info->cell == cell)
	{
	  cell_pos = i;
	  break;
	}
	  
      info = NULL;
    }
  
  g_return_if_fail (cell == NULL || info != NULL);

  btk_icon_view_set_cursor_item (icon_view, item, cell_pos);
  btk_icon_view_scroll_to_path (icon_view, path, FALSE, 0.0, 0.0);
  
  if (info && start_editing)
    btk_icon_view_start_editing (icon_view, item, info, NULL);
}

/**
 * btk_icon_view_get_cursor:
 * @icon_view: A #BtkIconView
 * @path: (allow-none): Return location for the current cursor path, or %NULL
 * @cell: (allow-none): Return location the current focus cell, or %NULL
 *
 * Fills in @path and @cell with the current cursor path and cell. 
 * If the cursor isn't currently set, then *@path will be %NULL.  
 * If no cell currently has focus, then *@cell will be %NULL.
 *
 * The returned #BtkTreePath must be freed with btk_tree_path_free().
 *
 * Return value: %TRUE if the cursor is set.
 *
 * Since: 2.8
 **/
bboolean
btk_icon_view_get_cursor (BtkIconView      *icon_view,
			  BtkTreePath     **path,
			  BtkCellRenderer **cell)
{
  BtkIconViewItem *item;
  BtkIconViewCellInfo *info;

  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), FALSE);

  item = icon_view->priv->cursor_item;
  if (icon_view->priv->cursor_cell < 0)
    info = NULL;
  else
    info = g_list_nth_data (icon_view->priv->cell_list, 
			    icon_view->priv->cursor_cell);

  if (path != NULL)
    {
      if (item != NULL)
	*path = btk_tree_path_new_from_indices (item->index, -1);
      else
	*path = NULL;
    }

  if (cell != NULL)
    {
      if (info != NULL)
	*cell = info->cell;
      else 
	*cell = NULL;
    }

  return (item != NULL);
}

static bboolean
btk_icon_view_button_press (BtkWidget      *widget,
			    BdkEventButton *event)
{
  BtkIconView *icon_view;
  BtkIconViewItem *item;
  BtkIconViewCellInfo *info = NULL;
  bboolean dirty = FALSE;
  BtkCellRendererMode mode;
  bint cursor_cell = -1;

  icon_view = BTK_ICON_VIEW (widget);

  if (event->window != icon_view->priv->bin_window)
    return FALSE;

  if (!btk_widget_has_focus (widget))
    btk_widget_grab_focus (widget);

  if (event->button == 1 && event->type == BDK_BUTTON_PRESS)
    {
      item = btk_icon_view_get_item_at_coords (icon_view, 
					       event->x, event->y,
					       FALSE,
					       &info);    

      /*
       * We consider only the the cells' area as the item area if the
       * item is not selected, but if it *is* selected, the complete
       * selection rectangle is considered to be part of the item.
       */
      if (item != NULL && (info != NULL || item->selected))
	{
	  if (info != NULL)
	    {
	      g_object_get (info->cell, "mode", &mode, NULL);

	      if (mode == BTK_CELL_RENDERER_MODE_ACTIVATABLE ||
		  mode == BTK_CELL_RENDERER_MODE_EDITABLE)
		cursor_cell = g_list_index (icon_view->priv->cell_list, info);
	    }

	  btk_icon_view_scroll_to_item (icon_view, item);
	  
	  if (icon_view->priv->selection_mode == BTK_SELECTION_NONE)
	    {
	      btk_icon_view_set_cursor_item (icon_view, item, cursor_cell);
	    }
	  else if (icon_view->priv->selection_mode == BTK_SELECTION_MULTIPLE &&
		   (event->state & BTK_EXTEND_SELECTION_MOD_MASK))
	    {
	      btk_icon_view_unselect_all_internal (icon_view);

	      btk_icon_view_set_cursor_item (icon_view, item, cursor_cell);
	      if (!icon_view->priv->anchor_item)
		icon_view->priv->anchor_item = item;
	      else 
		btk_icon_view_select_all_between (icon_view,
						  icon_view->priv->anchor_item,
						  item);
	      dirty = TRUE;
	    }
	  else 
	    {
	      if ((icon_view->priv->selection_mode == BTK_SELECTION_MULTIPLE ||
		  ((icon_view->priv->selection_mode == BTK_SELECTION_SINGLE) && item->selected)) &&
		  (event->state & BTK_MODIFY_SELECTION_MOD_MASK))
		{
		  item->selected = !item->selected;
		  btk_icon_view_queue_draw_item (icon_view, item);
		  dirty = TRUE;
		}
	      else
		{
		  btk_icon_view_unselect_all_internal (icon_view);

		  item->selected = TRUE;
		  btk_icon_view_queue_draw_item (icon_view, item);
		  dirty = TRUE;
		}
	      btk_icon_view_set_cursor_item (icon_view, item, cursor_cell);
	      icon_view->priv->anchor_item = item;
	    }

	  /* Save press to possibly begin a drag */
	  if (icon_view->priv->pressed_button < 0)
	    {
	      icon_view->priv->pressed_button = event->button;
	      icon_view->priv->press_start_x = event->x;
	      icon_view->priv->press_start_y = event->y;
	    }

	  if (!icon_view->priv->last_single_clicked)
	    icon_view->priv->last_single_clicked = item;

	  /* cancel the current editing, if it exists */
	  btk_icon_view_stop_editing (icon_view, TRUE);

	  if (info != NULL)
	    {
	      if (mode == BTK_CELL_RENDERER_MODE_ACTIVATABLE)
		btk_icon_view_item_activate_cell (icon_view, item, info, 
					          (BdkEvent *)event);
	      else if (mode == BTK_CELL_RENDERER_MODE_EDITABLE)
		btk_icon_view_start_editing (icon_view, item, info, 
					     (BdkEvent *)event);
	    }
	}
      else
	{
	  if (icon_view->priv->selection_mode != BTK_SELECTION_BROWSE &&
	      !(event->state & BTK_MODIFY_SELECTION_MOD_MASK))
	    {
	      dirty = btk_icon_view_unselect_all_internal (icon_view);
	    }
	  
	  if (icon_view->priv->selection_mode == BTK_SELECTION_MULTIPLE)
	    btk_icon_view_start_rubberbanding (icon_view, event->x, event->y);
	}

      /* don't draw keyboard focus around an clicked-on item */
      icon_view->priv->draw_focus = FALSE;
    }

  if (event->button == 1 && event->type == BDK_2BUTTON_PRESS)
    {
      item = btk_icon_view_get_item_at_coords (icon_view,
					       event->x, event->y,
					       FALSE,
					       NULL);

      if (item && item == icon_view->priv->last_single_clicked)
	{
	  BtkTreePath *path;

	  path = btk_tree_path_new_from_indices (item->index, -1);
	  btk_icon_view_item_activated (icon_view, path);
	  btk_tree_path_free (path);
	}

      icon_view->priv->last_single_clicked = NULL;
      icon_view->priv->pressed_button = -1;
    }
  
  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);

  return event->button == 1;
}

static bboolean
btk_icon_view_button_release (BtkWidget      *widget,
			      BdkEventButton *event)
{
  BtkIconView *icon_view;

  icon_view = BTK_ICON_VIEW (widget);
  
  if (icon_view->priv->pressed_button == event->button)
    icon_view->priv->pressed_button = -1;

  btk_icon_view_stop_rubberbanding (icon_view);

  remove_scroll_timeout (icon_view);

  return TRUE;
}

static bboolean
btk_icon_view_key_press (BtkWidget      *widget,
			 BdkEventKey    *event)
{
  BtkIconView *icon_view = BTK_ICON_VIEW (widget);

  if (icon_view->priv->doing_rubberband)
    {
      if (event->keyval == BDK_Escape)
	btk_icon_view_stop_rubberbanding (icon_view);

      return TRUE;
    }

  return BTK_WIDGET_CLASS (btk_icon_view_parent_class)->key_press_event (widget, event);
}

static bboolean
btk_icon_view_key_release (BtkWidget      *widget,
			   BdkEventKey    *event)
{
  BtkIconView *icon_view = BTK_ICON_VIEW (widget);

  if (icon_view->priv->doing_rubberband)
    return TRUE;

  return BTK_WIDGET_CLASS (btk_icon_view_parent_class)->key_press_event (widget, event);
}

static void
btk_icon_view_update_rubberband (bpointer data)
{
  BtkIconView *icon_view;
  bint x, y;
  BdkRectangle old_area;
  BdkRectangle new_area;
  BdkRectangle common;
  BdkRebunnyion *invalid_rebunnyion;
  
  icon_view = BTK_ICON_VIEW (data);

  bdk_window_get_pointer (icon_view->priv->bin_window, &x, &y, NULL);

  x = MAX (x, 0);
  y = MAX (y, 0);

  old_area.x = MIN (icon_view->priv->rubberband_x1,
		    icon_view->priv->rubberband_x2);
  old_area.y = MIN (icon_view->priv->rubberband_y1,
		    icon_view->priv->rubberband_y2);
  old_area.width = ABS (icon_view->priv->rubberband_x2 -
			icon_view->priv->rubberband_x1) + 1;
  old_area.height = ABS (icon_view->priv->rubberband_y2 -
			 icon_view->priv->rubberband_y1) + 1;
  
  new_area.x = MIN (icon_view->priv->rubberband_x1, x);
  new_area.y = MIN (icon_view->priv->rubberband_y1, y);
  new_area.width = ABS (x - icon_view->priv->rubberband_x1) + 1;
  new_area.height = ABS (y - icon_view->priv->rubberband_y1) + 1;

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
  
  bdk_window_invalidate_rebunnyion (icon_view->priv->bin_window, invalid_rebunnyion, TRUE);
    
  bdk_rebunnyion_destroy (invalid_rebunnyion);

  icon_view->priv->rubberband_x2 = x;
  icon_view->priv->rubberband_y2 = y;  

  btk_icon_view_update_rubberband_selection (icon_view);
}

static void
btk_icon_view_start_rubberbanding (BtkIconView  *icon_view,
				   bint          x,
				   bint          y)
{
  GList *items;

  g_assert (!icon_view->priv->doing_rubberband);

  for (items = icon_view->priv->items; items; items = items->next)
    {
      BtkIconViewItem *item = items->data;

      item->selected_before_rubberbanding = item->selected;
    }
  
  icon_view->priv->rubberband_x1 = x;
  icon_view->priv->rubberband_y1 = y;
  icon_view->priv->rubberband_x2 = x;
  icon_view->priv->rubberband_y2 = y;

  icon_view->priv->doing_rubberband = TRUE;

  btk_grab_add (BTK_WIDGET (icon_view));
}

static void
btk_icon_view_stop_rubberbanding (BtkIconView *icon_view)
{
  if (!icon_view->priv->doing_rubberband)
    return;

  icon_view->priv->doing_rubberband = FALSE;

  btk_grab_remove (BTK_WIDGET (icon_view));
  
  btk_widget_queue_draw (BTK_WIDGET (icon_view));
}

static void
btk_icon_view_update_rubberband_selection (BtkIconView *icon_view)
{
  GList *items;
  bint x, y, width, height;
  bboolean dirty = FALSE;
  
  x = MIN (icon_view->priv->rubberband_x1,
	   icon_view->priv->rubberband_x2);
  y = MIN (icon_view->priv->rubberband_y1,
	   icon_view->priv->rubberband_y2);
  width = ABS (icon_view->priv->rubberband_x1 - 
	       icon_view->priv->rubberband_x2);
  height = ABS (icon_view->priv->rubberband_y1 - 
		icon_view->priv->rubberband_y2);
  
  for (items = icon_view->priv->items; items; items = items->next)
    {
      BtkIconViewItem *item = items->data;
      bboolean is_in;
      bboolean selected;
      
      is_in = btk_icon_view_item_hit_test (icon_view, item, 
					   x, y, width, height);

      selected = is_in ^ item->selected_before_rubberbanding;

      if (item->selected != selected)
	{
	  item->selected = selected;
	  dirty = TRUE;
	  btk_icon_view_queue_draw_item (icon_view, item);
	}
    }

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}

static bboolean
btk_icon_view_item_hit_test (BtkIconView      *icon_view,
			     BtkIconViewItem  *item,
			     bint              x,
			     bint              y,
			     bint              width,
			     bint              height)
{
  GList *l;
  BdkRectangle box;
 
  if (MIN (x + width, item->x + item->width) - MAX (x, item->x) <= 0 ||
      MIN (y + height, item->y + item->height) - MAX (y, item->y) <= 0)
    return FALSE;

  for (l = icon_view->priv->cell_list; l; l = l->next)
    {
      BtkIconViewCellInfo *info = (BtkIconViewCellInfo *)l->data;
      
      if (!info->cell->visible)
	continue;
      
      btk_icon_view_get_cell_box (icon_view, item, info, &box);

      if (MIN (x + width, box.x + box.width) - MAX (x, box.x) > 0 &&
	MIN (y + height, box.y + box.height) - MAX (y, box.y) > 0)
	return TRUE;
    }

  return FALSE;
}

static bboolean
btk_icon_view_unselect_all_internal (BtkIconView  *icon_view)
{
  bboolean dirty = FALSE;
  GList *items;

  if (icon_view->priv->selection_mode == BTK_SELECTION_NONE)
    return FALSE;

  for (items = icon_view->priv->items; items; items = items->next)
    {
      BtkIconViewItem *item = items->data;

      if (item->selected)
	{
	  item->selected = FALSE;
	  dirty = TRUE;
	  btk_icon_view_queue_draw_item (icon_view, item);
	  btk_icon_view_item_selected_changed (icon_view, item);
	}
    }

  return dirty;
}


/* BtkIconView signals */
static void
btk_icon_view_set_adjustments (BtkIconView   *icon_view,
			       BtkAdjustment *hadj,
			       BtkAdjustment *vadj)
{
  bboolean need_adjust = FALSE;

  if (hadj)
    g_return_if_fail (BTK_IS_ADJUSTMENT (hadj));
  else
    hadj = BTK_ADJUSTMENT (btk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  if (vadj)
    g_return_if_fail (BTK_IS_ADJUSTMENT (vadj));
  else
    vadj = BTK_ADJUSTMENT (btk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));

  if (icon_view->priv->hadjustment && (icon_view->priv->hadjustment != hadj))
    {
      g_signal_handlers_disconnect_matched (icon_view->priv->hadjustment, G_SIGNAL_MATCH_DATA,
					   0, 0, NULL, NULL, icon_view);
      g_object_unref (icon_view->priv->hadjustment);
    }

  if (icon_view->priv->vadjustment && (icon_view->priv->vadjustment != vadj))
    {
      g_signal_handlers_disconnect_matched (icon_view->priv->vadjustment, G_SIGNAL_MATCH_DATA,
					    0, 0, NULL, NULL, icon_view);
      g_object_unref (icon_view->priv->vadjustment);
    }

  if (icon_view->priv->hadjustment != hadj)
    {
      icon_view->priv->hadjustment = hadj;
      g_object_ref_sink (icon_view->priv->hadjustment);

      g_signal_connect (icon_view->priv->hadjustment, "value-changed",
			G_CALLBACK (btk_icon_view_adjustment_changed),
			icon_view);
      need_adjust = TRUE;
    }

  if (icon_view->priv->vadjustment != vadj)
    {
      icon_view->priv->vadjustment = vadj;
      g_object_ref_sink (icon_view->priv->vadjustment);

      g_signal_connect (icon_view->priv->vadjustment, "value-changed",
			G_CALLBACK (btk_icon_view_adjustment_changed),
			icon_view);
      need_adjust = TRUE;
    }

  if (need_adjust)
    btk_icon_view_adjustment_changed (NULL, icon_view);
}

static void
btk_icon_view_real_select_all (BtkIconView *icon_view)
{
  btk_icon_view_select_all (icon_view);
}

static void
btk_icon_view_real_unselect_all (BtkIconView *icon_view)
{
  btk_icon_view_unselect_all (icon_view);
}

static void
btk_icon_view_real_select_cursor_item (BtkIconView *icon_view)
{
  btk_icon_view_unselect_all (icon_view);
  
  if (icon_view->priv->cursor_item != NULL)
    btk_icon_view_select_item (icon_view, icon_view->priv->cursor_item);
}

static bboolean
btk_icon_view_real_activate_cursor_item (BtkIconView *icon_view)
{
  BtkTreePath *path;
  BtkCellRendererMode mode;
  BtkIconViewCellInfo *info = NULL;
  
  if (!icon_view->priv->cursor_item)
    return FALSE;

  info = g_list_nth_data (icon_view->priv->cell_list, 
			  icon_view->priv->cursor_cell);

  if (info) 
    {  
      g_object_get (info->cell, "mode", &mode, NULL);

      if (mode == BTK_CELL_RENDERER_MODE_ACTIVATABLE)
	{
	  btk_icon_view_item_activate_cell (icon_view, 
					    icon_view->priv->cursor_item, 
					    info, NULL);
	  return TRUE;
	}
      else if (mode == BTK_CELL_RENDERER_MODE_EDITABLE)
	{
	  btk_icon_view_start_editing (icon_view, 
				       icon_view->priv->cursor_item, 
				       info, NULL);
	  return TRUE;
	}
    }
  
  path = btk_tree_path_new_from_indices (icon_view->priv->cursor_item->index, -1);
  btk_icon_view_item_activated (icon_view, path);
  btk_tree_path_free (path);

  return TRUE;
}

static void
btk_icon_view_real_toggle_cursor_item (BtkIconView *icon_view)
{
  if (!icon_view->priv->cursor_item)
    return;

  switch (icon_view->priv->selection_mode)
    {
    case BTK_SELECTION_NONE:
      break;
    case BTK_SELECTION_BROWSE:
      btk_icon_view_select_item (icon_view, icon_view->priv->cursor_item);
      break;
    case BTK_SELECTION_SINGLE:
      if (icon_view->priv->cursor_item->selected)
	btk_icon_view_unselect_item (icon_view, icon_view->priv->cursor_item);
      else
	btk_icon_view_select_item (icon_view, icon_view->priv->cursor_item);
      break;
    case BTK_SELECTION_MULTIPLE:
      icon_view->priv->cursor_item->selected = !icon_view->priv->cursor_item->selected;
      g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0); 
      
      btk_icon_view_item_selected_changed (icon_view, icon_view->priv->cursor_item);      
      btk_icon_view_queue_draw_item (icon_view, icon_view->priv->cursor_item);
      break;
    }
}

/* Internal functions */
static void
btk_icon_view_adjustment_changed (BtkAdjustment *adjustment,
				  BtkIconView   *icon_view)
{
  if (btk_widget_get_realized (BTK_WIDGET (icon_view)))
    {
      bdk_window_move (icon_view->priv->bin_window,
		       - icon_view->priv->hadjustment->value,
		       - icon_view->priv->vadjustment->value);

      if (icon_view->priv->doing_rubberband)
	btk_icon_view_update_rubberband (BTK_WIDGET (icon_view));

      bdk_window_process_updates (icon_view->priv->bin_window, TRUE);
    }
}

static GList *
btk_icon_view_layout_single_row (BtkIconView *icon_view, 
				 GList       *first_item, 
				 bint         item_width,
				 bint         row,
				 bint        *y, 
				 bint        *maximum_width)
{
  bint focus_width;
  bint x, current_width;
  GList *items, *last_item;
  bint col;
  bint colspan;
  bint *max_height;
  bint i;
  bboolean rtl;

  rtl = btk_widget_get_direction (BTK_WIDGET (icon_view)) == BTK_TEXT_DIR_RTL;
  max_height = g_new0 (bint, icon_view->priv->n_cells);

  x = 0;
  col = 0;
  items = first_item;
  current_width = 0;

  btk_widget_style_get (BTK_WIDGET (icon_view),
			"focus-line-width", &focus_width,
			NULL);

  x += icon_view->priv->margin + focus_width;
  current_width += 2 * (icon_view->priv->margin + focus_width);

  items = first_item;
  while (items)
    {
      BtkIconViewItem *item = items->data;

      btk_icon_view_calculate_item_size (icon_view, item);
      colspan = 1 + (item->width - 1) / (item_width + icon_view->priv->column_spacing);

      item->width = colspan * item_width + (colspan - 1) * icon_view->priv->column_spacing;

      current_width += item->width;

      if (items != first_item)
	{
	  if ((icon_view->priv->columns <= 0 && current_width > BTK_WIDGET (icon_view)->allocation.width) ||
	      (icon_view->priv->columns > 0 && col >= icon_view->priv->columns))
	    break;
	}

      current_width += icon_view->priv->column_spacing + 2 * focus_width;

      item->y = *y + focus_width;
      item->x = x;

      x = current_width - (icon_view->priv->margin + focus_width); 

      for (i = 0; i < icon_view->priv->n_cells; i++)
	max_height[i] = MAX (max_height[i], item->box[i].height);
	      
      if (current_width > *maximum_width)
	*maximum_width = current_width;

      item->row = row;
      item->col = col;

      col += colspan;
      items = items->next;
    }

  last_item = items;

  /* Now go through the row again and align the icons */
  for (items = first_item; items != last_item; items = items->next)
    {
      BtkIconViewItem *item = items->data;

      if (rtl)
	{
	  item->x = *maximum_width - item->width - item->x;
	  item->col = col - 1 - item->col;
	}

      btk_icon_view_calculate_item_size2 (icon_view, item, max_height);

      /* We may want to readjust the new y coordinate. */
      if (item->y + item->height + focus_width + icon_view->priv->row_spacing > *y)
	*y = item->y + item->height + focus_width + icon_view->priv->row_spacing;
    }

  g_free (max_height);
  
  return last_item;
}

static void
btk_icon_view_set_adjustment_upper (BtkAdjustment *adj,
				    bdouble        upper)
{
  if (upper != adj->upper)
    {
      bdouble min = MAX (0.0, upper - adj->page_size);
      bboolean value_changed = FALSE;
      
      adj->upper = upper;

      if (adj->value > min)
	{
	  adj->value = min;
	  value_changed = TRUE;
	}
      
      btk_adjustment_changed (adj);
      
      if (value_changed)
	btk_adjustment_value_changed (adj);
    }
}

static void
btk_icon_view_layout (BtkIconView *icon_view)
{
  bint y = 0, maximum_width = 0;
  GList *icons;
  BtkWidget *widget;
  bint row;
  bint item_width;

  if (icon_view->priv->layout_idle_id != 0)
    {
      g_source_remove (icon_view->priv->layout_idle_id);
      icon_view->priv->layout_idle_id = 0;
    }
  
  if (icon_view->priv->model == NULL)
    return;

  widget = BTK_WIDGET (icon_view);

  item_width = icon_view->priv->item_width;

  if (item_width < 0)
    {
      for (icons = icon_view->priv->items; icons; icons = icons->next)
	{
	  BtkIconViewItem *item = icons->data;
	  btk_icon_view_calculate_item_size (icon_view, item);
	  item_width = MAX (item_width, item->width);
	}
    }


  icons = icon_view->priv->items;
  y += icon_view->priv->margin;
  row = 0;

  if (icons)
    {
      btk_icon_view_set_cell_data (icon_view, icons->data);
      adjust_wrap_width (icon_view, icons->data);
    }
  
  do
    {
      icons = btk_icon_view_layout_single_row (icon_view, icons, 
					       item_width, row,
					       &y, &maximum_width);
      row++;
    }
  while (icons != NULL);

  if (maximum_width != icon_view->priv->width)
    icon_view->priv->width = maximum_width;

  y += icon_view->priv->margin;
  
  if (y != icon_view->priv->height)
    icon_view->priv->height = y;

  btk_icon_view_set_adjustment_upper (icon_view->priv->hadjustment, 
				      icon_view->priv->width);
  btk_icon_view_set_adjustment_upper (icon_view->priv->vadjustment, 
				      icon_view->priv->height);

  if (icon_view->priv->width != widget->requisition.width ||
      icon_view->priv->height != widget->requisition.height)
    btk_widget_queue_resize_no_redraw (widget);

  if (btk_widget_get_realized (BTK_WIDGET (icon_view)))
    bdk_window_resize (icon_view->priv->bin_window,
		       MAX (icon_view->priv->width, widget->allocation.width),
		       MAX (icon_view->priv->height, widget->allocation.height));

  if (icon_view->priv->scroll_to_path)
    {
      BtkTreePath *path;

      path = btk_tree_row_reference_get_path (icon_view->priv->scroll_to_path);
      btk_tree_row_reference_free (icon_view->priv->scroll_to_path);
      icon_view->priv->scroll_to_path = NULL;
      
      btk_icon_view_scroll_to_path (icon_view, path,
				    icon_view->priv->scroll_to_use_align,
				    icon_view->priv->scroll_to_row_align,
				    icon_view->priv->scroll_to_col_align);
      btk_tree_path_free (path);
    }
  
  btk_widget_queue_draw (widget);
}

static void 
btk_icon_view_get_cell_area (BtkIconView         *icon_view,
			     BtkIconViewItem     *item,
			     BtkIconViewCellInfo *info,
			     BdkRectangle        *cell_area)
{
  g_return_if_fail (info->position < item->n_cells);

  if (icon_view->priv->item_orientation == BTK_ORIENTATION_HORIZONTAL)
    {
      cell_area->x = item->box[info->position].x - item->before[info->position];
      cell_area->y = item->y + icon_view->priv->item_padding;
      cell_area->width = item->box[info->position].width + 
	item->before[info->position] + item->after[info->position];
      cell_area->height = item->height - icon_view->priv->item_padding * 2;
    }
  else
    {
      cell_area->x = item->x + icon_view->priv->item_padding;
      cell_area->y = item->box[info->position].y - item->before[info->position];
      cell_area->width = item->width - icon_view->priv->item_padding * 2;
      cell_area->height = item->box[info->position].height + 
	item->before[info->position] + item->after[info->position];
    }
}

static void 
btk_icon_view_get_cell_box (BtkIconView         *icon_view,
			    BtkIconViewItem     *item,
			    BtkIconViewCellInfo *info,
			    BdkRectangle        *box)
{
  g_return_if_fail (info->position < item->n_cells);

  *box = item->box[info->position];
}

/* try to guess a reasonable wrap width for an implicit text cell renderer
 */
static void
adjust_wrap_width (BtkIconView     *icon_view,
		   BtkIconViewItem *item)
{
  BtkIconViewCellInfo *text_info;
  BtkIconViewCellInfo *pixbuf_info;
  bint pixbuf_width, wrap_width;
      
  if (icon_view->priv->text_cell != -1 &&
      icon_view->priv->pixbuf_cell != -1)
    {
      bint item_width;

      text_info = g_list_nth_data (icon_view->priv->cell_list,
				   icon_view->priv->text_cell);
      pixbuf_info = g_list_nth_data (icon_view->priv->cell_list,
				     icon_view->priv->pixbuf_cell);
      
      btk_cell_renderer_get_size (pixbuf_info->cell, 
				  BTK_WIDGET (icon_view), 
				  NULL, NULL, NULL,
				  &pixbuf_width, 
				  NULL);
	  

      if (icon_view->priv->item_width > 0)
	item_width = icon_view->priv->item_width;
      else
	item_width = item->width;

      if (icon_view->priv->item_orientation == BTK_ORIENTATION_VERTICAL)
        wrap_width = item_width;
      else {
        if (item->width == -1 && item_width <= 0)
          wrap_width = MAX (2 * pixbuf_width, 50);
        else
          wrap_width = item_width - pixbuf_width - icon_view->priv->spacing;
        }

      wrap_width -= icon_view->priv->item_padding * 2;

      g_object_set (text_info->cell, "wrap-width", wrap_width, NULL);
      g_object_set (text_info->cell, "width", wrap_width, NULL);
    }
}

static void
btk_icon_view_calculate_item_size (BtkIconView     *icon_view,
				   BtkIconViewItem *item)
{
  bint spacing;
  GList *l;

  if (item->width != -1 && item->height != -1) 
    return;

  if (item->n_cells != icon_view->priv->n_cells)
    {
      g_free (item->before);
      g_free (item->after);
      g_free (item->box);
      
      item->before = g_new0 (bint, icon_view->priv->n_cells);
      item->after = g_new0 (bint, icon_view->priv->n_cells);
      item->box = g_new0 (BdkRectangle, icon_view->priv->n_cells);

      item->n_cells = icon_view->priv->n_cells;
    }

  btk_icon_view_set_cell_data (icon_view, item);

  spacing = icon_view->priv->spacing;

  item->width = 0;
  item->height = 0;
  for (l = icon_view->priv->cell_list; l; l = l->next)
    {
      BtkIconViewCellInfo *info = (BtkIconViewCellInfo *)l->data;
      
      if (!info->cell->visible)
	continue;
      
      btk_cell_renderer_get_size (info->cell, BTK_WIDGET (icon_view), 
				  NULL, NULL, NULL,
				  &item->box[info->position].width, 
				  &item->box[info->position].height);

      if (icon_view->priv->item_orientation == BTK_ORIENTATION_HORIZONTAL)
	{
	  item->width += item->box[info->position].width 
	    + (info->position > 0 ? spacing : 0);
	  item->height = MAX (item->height, item->box[info->position].height);
	}
      else
	{
	  item->width = MAX (item->width, item->box[info->position].width);
	  item->height += item->box[info->position].height + (info->position > 0 ? spacing : 0);
	}
    }

  item->width += icon_view->priv->item_padding * 2;
  item->height += icon_view->priv->item_padding * 2;
}

static void
btk_icon_view_calculate_item_size2 (BtkIconView     *icon_view,
				    BtkIconViewItem *item,
				    bint            *max_height)
{
  BdkRectangle cell_area;
  bint spacing;
  GList *l;
  bint i, k;
  bboolean rtl;

  rtl = btk_widget_get_direction (BTK_WIDGET (icon_view)) == BTK_TEXT_DIR_RTL;

  btk_icon_view_set_cell_data (icon_view, item);

  spacing = icon_view->priv->spacing;

  item->height = 0;
  for (i = 0; i < icon_view->priv->n_cells; i++)
    {
      if (icon_view->priv->item_orientation == BTK_ORIENTATION_HORIZONTAL)
	item->height = MAX (item->height, max_height[i]);
      else
	item->height += max_height[i] + (i > 0 ? spacing : 0);
    }

  cell_area.x = item->x + icon_view->priv->item_padding;
  cell_area.y = item->y + icon_view->priv->item_padding;
      
  for (k = 0; k < 2; k++)
    for (l = icon_view->priv->cell_list, i = 0; l; l = l->next, i++)
      {
	BtkIconViewCellInfo *info = (BtkIconViewCellInfo *)l->data;
	
	if (info->pack == (k ? BTK_PACK_START : BTK_PACK_END))
	  continue;

	if (!info->cell->visible)
	  continue;

	if (icon_view->priv->item_orientation == BTK_ORIENTATION_HORIZONTAL)
	  {
            /* We should not subtract icon_view->priv->item_padding from item->height,
             * because item->height is recalculated above using
             * max_height which does not contain item padding.
             */
	    cell_area.width = item->box[info->position].width;
	    cell_area.height = item->height;
	  }
	else
	  {
            /* item->width is not recalculated and thus needs to be
             * corrected for the padding.
             */
	    cell_area.width = item->width - 2 * icon_view->priv->item_padding;
	    cell_area.height = max_height[i];
	  }
	
	btk_cell_renderer_get_size (info->cell, BTK_WIDGET (icon_view), 
				    &cell_area,
				    &item->box[info->position].x, &item->box[info->position].y,
				    &item->box[info->position].width, &item->box[info->position].height);
	
	item->box[info->position].x += cell_area.x;
	item->box[info->position].y += cell_area.y;
	if (icon_view->priv->item_orientation == BTK_ORIENTATION_HORIZONTAL)
	  {
	    item->before[info->position] = item->box[info->position].x - cell_area.x;
	    item->after[info->position] = cell_area.width - item->box[info->position].width - item->before[info->position];
	    cell_area.x += cell_area.width + spacing;
	  }
	else
	  {
	    if (item->box[info->position].width > item->width - icon_view->priv->item_padding * 2)
	      {
		item->width = item->box[info->position].width + icon_view->priv->item_padding * 2;
		cell_area.width = item->width;
	      }
	    item->before[info->position] = item->box[info->position].y - cell_area.y;
	    item->after[info->position] = cell_area.height - item->box[info->position].height - item->before[info->position];
	    cell_area.y += cell_area.height + spacing;
	  }
      }
  
  if (rtl && icon_view->priv->item_orientation == BTK_ORIENTATION_HORIZONTAL)
    {
      for (i = 0; i < icon_view->priv->n_cells; i++)
	{
	  item->box[i].x = item->x + item->width - 
	    (item->box[i].x + item->box[i].width - item->x);
	}      
    }

  item->height += icon_view->priv->item_padding * 2;
}

static void
btk_icon_view_invalidate_sizes (BtkIconView *icon_view)
{
  g_list_foreach (icon_view->priv->items,
		  (GFunc)btk_icon_view_item_invalidate_size, NULL);
}

static void
btk_icon_view_item_invalidate_size (BtkIconViewItem *item)
{
  item->width = -1;
  item->height = -1;
}

static void
btk_icon_view_paint_item (BtkIconView     *icon_view,
			  bairo_t         *cr,
			  BtkIconViewItem *item,
			  BdkRectangle    *area,
			  BdkDrawable     *drawable,
			  bint             x,
			  bint             y,
			  bboolean         draw_focus)
{
  bint focus_width;
  bint padding;
  BdkRectangle cell_area, box;
  GList *l;
  bint i;
  BtkStateType state;
  BtkCellRendererState flags;
      
  if (icon_view->priv->model == NULL)
    return;
  
  btk_icon_view_set_cell_data (icon_view, item);

  btk_widget_style_get (BTK_WIDGET (icon_view),
			"focus-line-width", &focus_width,
			NULL);
  
  padding = focus_width; 
  
  if (item->selected)
    {
      flags = BTK_CELL_RENDERER_SELECTED;
      if (btk_widget_has_focus (BTK_WIDGET (icon_view)))
	state = BTK_STATE_SELECTED;
      else
	state = BTK_STATE_ACTIVE;
    }
  else
    {
      flags = 0;
      state = BTK_STATE_NORMAL;
    }
  
#ifdef DEBUG_ICON_VIEW
  bdk_draw_rectangle (drawable,
		      BTK_WIDGET (icon_view)->style->black_gc,
		      FALSE,
		      x, y, 
		      item->width, item->height);
#endif

  if (item->selected)
    {
      btk_paint_flat_box (BTK_WIDGET (icon_view)->style,
			  (BdkWindow *) drawable,
			  BTK_STATE_SELECTED,
			  BTK_SHADOW_NONE,
			  area,
			  BTK_WIDGET (icon_view),
			  "icon_view_item",
			  x, y,
			  item->width, item->height);
    }
  
  for (l = icon_view->priv->cell_list; l; l = l->next)
    {
      BtkIconViewCellInfo *info = (BtkIconViewCellInfo *)l->data;
      
      if (!info->cell->visible)
	continue;
      
      btk_icon_view_get_cell_area (icon_view, item, info, &cell_area);
      
#ifdef DEBUG_ICON_VIEW
      bdk_draw_rectangle (drawable,
			  BTK_WIDGET (icon_view)->style->black_gc,
			  FALSE,
			  x - item->x + cell_area.x, 
			  y - item->y + cell_area.y, 
			  cell_area.width, cell_area.height);

      btk_icon_view_get_cell_box (icon_view, item, info, &box);
	  
      bdk_draw_rectangle (drawable,
			  BTK_WIDGET (icon_view)->style->black_gc,
			  FALSE,
			  x - item->x + box.x, 
			  y - item->y + box.y, 
			  box.width, box.height);
#endif

      cell_area.x = x - item->x + cell_area.x;
      cell_area.y = y - item->y + cell_area.y;

      btk_cell_renderer_render (info->cell,
				drawable,
				BTK_WIDGET (icon_view),
				&cell_area, &cell_area, area, flags);
    }

  if (draw_focus &&
      btk_widget_has_focus (BTK_WIDGET (icon_view)) &&
      item == icon_view->priv->cursor_item)
    {
      for (l = icon_view->priv->cell_list, i = 0; l; l = l->next, i++)
        {
          BtkIconViewCellInfo *info = (BtkIconViewCellInfo *)l->data;

          if (!info->cell->visible)
            continue;

          /* If found a editable/activatable cell, draw focus on it. */
          if (icon_view->priv->cursor_cell < 0 &&
              info->cell->mode != BTK_CELL_RENDERER_MODE_INERT)
            icon_view->priv->cursor_cell = i;

          btk_icon_view_get_cell_box (icon_view, item, info, &box);

          if (i == icon_view->priv->cursor_cell)
            {
              btk_paint_focus (BTK_WIDGET (icon_view)->style,
                               drawable,
                               BTK_STATE_NORMAL,
                               area,
                               BTK_WIDGET (icon_view),
                               "icon_view",
                               x - item->x + box.x - padding,
                               y - item->y + box.y - padding,
                               box.width + 2 * padding,
                               box.height + 2 * padding);
              break;
            }
        }

      /* If there are no editable/activatable cells, draw focus 
       * around the whole item.
       */
      if (icon_view->priv->cursor_cell < 0)
        btk_paint_focus (BTK_WIDGET (icon_view)->style,
                         drawable,
                         BTK_STATE_NORMAL,
                         area,
                         BTK_WIDGET (icon_view),
                         "icon_view",
                         x - padding,
                         y - padding,
                         item->width + 2 * padding,
                         item->height + 2 * padding);
    }
}

static void
btk_icon_view_paint_rubberband (BtkIconView     *icon_view,
				bairo_t         *cr,
				BdkRectangle    *area)
{
  BdkRectangle rect;
  BdkRectangle rubber_rect;
  BdkColor *fill_color_bdk;
  buchar fill_color_alpha;

  rubber_rect.x = MIN (icon_view->priv->rubberband_x1, icon_view->priv->rubberband_x2);
  rubber_rect.y = MIN (icon_view->priv->rubberband_y1, icon_view->priv->rubberband_y2);
  rubber_rect.width = ABS (icon_view->priv->rubberband_x1 - icon_view->priv->rubberband_x2) + 1;
  rubber_rect.height = ABS (icon_view->priv->rubberband_y1 - icon_view->priv->rubberband_y2) + 1;

  if (!bdk_rectangle_intersect (&rubber_rect, area, &rect))
    return;

  btk_widget_style_get (BTK_WIDGET (icon_view),
                        "selection-box-color", &fill_color_bdk,
                        "selection-box-alpha", &fill_color_alpha,
                        NULL);

  if (!fill_color_bdk)
    fill_color_bdk = bdk_color_copy (&BTK_WIDGET (icon_view)->style->base[BTK_STATE_SELECTED]);

  bairo_set_source_rgba (cr,
			 fill_color_bdk->red / 65535.,
			 fill_color_bdk->green / 65535.,
			 fill_color_bdk->blue / 65535.,
			 fill_color_alpha / 255.);

  bairo_save (cr);
  bdk_bairo_rectangle (cr, &rect);
  bairo_clip (cr);
  bairo_paint (cr);

  /* Draw the border without alpha */
  bairo_set_source_rgb (cr,
			fill_color_bdk->red / 65535.,
			fill_color_bdk->green / 65535.,
			fill_color_bdk->blue / 65535.);
  bairo_rectangle (cr, 
		   rubber_rect.x + 0.5, rubber_rect.y + 0.5,
		   rubber_rect.width - 1, rubber_rect.height - 1);
  bairo_stroke (cr);
  bairo_restore (cr);

  bdk_color_free (fill_color_bdk);
}

static void
btk_icon_view_queue_draw_path (BtkIconView *icon_view,
			       BtkTreePath *path)
{
  GList *l;
  bint index;

  index = btk_tree_path_get_indices (path)[0];

  for (l = icon_view->priv->items; l; l = l->next) 
    {
      BtkIconViewItem *item = l->data;

      if (item->index == index)
	{
	  btk_icon_view_queue_draw_item (icon_view, item);
	  break;
	}
    }
}

static void
btk_icon_view_queue_draw_item (BtkIconView     *icon_view,
			       BtkIconViewItem *item)
{
  bint focus_width;
  BdkRectangle rect;

  btk_widget_style_get (BTK_WIDGET (icon_view),
			"focus-line-width", &focus_width,
			NULL);

  rect.x = item->x - focus_width;
  rect.y = item->y - focus_width;
  rect.width = item->width + 2 * focus_width;
  rect.height = item->height + 2 * focus_width;

  if (icon_view->priv->bin_window)
    bdk_window_invalidate_rect (icon_view->priv->bin_window, &rect, TRUE);
}

static bboolean
layout_callback (bpointer user_data)
{
  BtkIconView *icon_view;

  icon_view = BTK_ICON_VIEW (user_data);
  
  icon_view->priv->layout_idle_id = 0;

  btk_icon_view_layout (icon_view);
  
  return FALSE;
}

static void
btk_icon_view_queue_layout (BtkIconView *icon_view)
{
  if (icon_view->priv->layout_idle_id != 0)
    return;

  icon_view->priv->layout_idle_id = bdk_threads_add_idle (layout_callback, icon_view);
}

static void
btk_icon_view_set_cursor_item (BtkIconView     *icon_view,
			       BtkIconViewItem *item,
			       bint             cursor_cell)
{
  BatkObject *obj;
  BatkObject *item_obj;
  BatkObject *cursor_item_obj;

  if (icon_view->priv->cursor_item == item &&
      (cursor_cell < 0 || cursor_cell == icon_view->priv->cursor_cell))
    return;

  obj = btk_widget_get_accessible (BTK_WIDGET (icon_view));
  if (icon_view->priv->cursor_item != NULL)
    {
      btk_icon_view_queue_draw_item (icon_view, icon_view->priv->cursor_item);
      if (obj != NULL)
        {
          cursor_item_obj = batk_object_ref_accessible_child (obj, icon_view->priv->cursor_item->index);
          if (cursor_item_obj != NULL)
            batk_object_notify_state_change (cursor_item_obj, BATK_STATE_FOCUSED, FALSE);
        }
    }
  icon_view->priv->cursor_item = item;
  if (cursor_cell >= 0)
    icon_view->priv->cursor_cell = cursor_cell;

  btk_icon_view_queue_draw_item (icon_view, item);
  
  /* Notify that accessible focus object has changed */
  item_obj = batk_object_ref_accessible_child (obj, item->index);

  if (item_obj != NULL)
    {
      batk_focus_tracker_notify (item_obj);
      batk_object_notify_state_change (item_obj, BATK_STATE_FOCUSED, TRUE);
      g_object_unref (item_obj); 
    }
}


static BtkIconViewItem *
btk_icon_view_item_new (void)
{
  BtkIconViewItem *item;

  item = g_new0 (BtkIconViewItem, 1);

  item->width = -1;
  item->height = -1;
  
  return item;
}

static void
btk_icon_view_item_free (BtkIconViewItem *item)
{
  g_return_if_fail (item != NULL);

  g_free (item->before);
  g_free (item->after);
  g_free (item->box);

  g_free (item);
}


static BtkIconViewItem *
btk_icon_view_get_item_at_coords (BtkIconView          *icon_view,
				  bint                  x,
				  bint                  y,
				  bboolean              only_in_cell,
				  BtkIconViewCellInfo **cell_at_pos)
{
  GList *items, *l;
  BdkRectangle box;

  if (cell_at_pos)
    *cell_at_pos = NULL;

  for (items = icon_view->priv->items; items; items = items->next)
    {
      BtkIconViewItem *item = items->data;

      if (x >= item->x - icon_view->priv->column_spacing/2 && x <= item->x + item->width + icon_view->priv->column_spacing/2 &&
	  y >= item->y - icon_view->priv->row_spacing/2 && y <= item->y + item->height + icon_view->priv->row_spacing/2)
	{
	  if (only_in_cell || cell_at_pos)
	    {
	      btk_icon_view_set_cell_data (icon_view, item);

	      for (l = icon_view->priv->cell_list; l; l = l->next)
		{
		  BtkIconViewCellInfo *info = (BtkIconViewCellInfo *)l->data;

		  if (!info->cell->visible)
		    continue;

		  btk_icon_view_get_cell_box (icon_view, item, info, &box);

		  if ((x >= box.x && x <= box.x + box.width &&
		       y >= box.y && y <= box.y + box.height) ||
		      (x >= box.x  &&
		       x <= box.x + box.width &&
		       y >= box.y &&
		       y <= box.y + box.height))
		    {
		      if (cell_at_pos)
			*cell_at_pos = info;

		      return item;
		    }
		}

	      if (only_in_cell)
		return NULL;
	    }

	  return item;
	}
    }

  return NULL;
}

static void
btk_icon_view_select_item (BtkIconView      *icon_view,
			   BtkIconViewItem  *item)
{
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));
  g_return_if_fail (item != NULL);

  if (item->selected)
    return;
  
  if (icon_view->priv->selection_mode == BTK_SELECTION_NONE)
    return;
  else if (icon_view->priv->selection_mode != BTK_SELECTION_MULTIPLE)
    btk_icon_view_unselect_all_internal (icon_view);

  item->selected = TRUE;

  btk_icon_view_item_selected_changed (icon_view, item);
  g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);

  btk_icon_view_queue_draw_item (icon_view, item);
}


static void
btk_icon_view_unselect_item (BtkIconView      *icon_view,
			     BtkIconViewItem  *item)
{
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));
  g_return_if_fail (item != NULL);

  if (!item->selected)
    return;
  
  if (icon_view->priv->selection_mode == BTK_SELECTION_NONE ||
      icon_view->priv->selection_mode == BTK_SELECTION_BROWSE)
    return;
  
  item->selected = FALSE;

  btk_icon_view_item_selected_changed (icon_view, item);
  g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);

  btk_icon_view_queue_draw_item (icon_view, item);
}

static void
verify_items (BtkIconView *icon_view)
{
  GList *items;
  int i = 0;

  for (items = icon_view->priv->items; items; items = items->next)
    {
      BtkIconViewItem *item = items->data;

      if (item->index != i)
	g_error ("List item does not match its index: "
		 "item index %d and list index %d\n", item->index, i);

      i++;
    }
}

static void
btk_icon_view_row_changed (BtkTreeModel *model,
			   BtkTreePath  *path,
			   BtkTreeIter  *iter,
			   bpointer      data)
{
  BtkIconViewItem *item;
  bint index;
  BtkIconView *icon_view;

  icon_view = BTK_ICON_VIEW (data);

  btk_icon_view_stop_editing (icon_view, TRUE);
  
  index = btk_tree_path_get_indices(path)[0];
  item = g_list_nth_data (icon_view->priv->items, index);

  btk_icon_view_item_invalidate_size (item);
  btk_icon_view_queue_layout (icon_view);

  verify_items (icon_view);
}

static void
btk_icon_view_row_inserted (BtkTreeModel *model,
			    BtkTreePath  *path,
			    BtkTreeIter  *iter,
			    bpointer      data)
{
  bint index;
  BtkIconViewItem *item;
  bboolean iters_persist;
  BtkIconView *icon_view;
  GList *list;
  
  icon_view = BTK_ICON_VIEW (data);

  iters_persist = btk_tree_model_get_flags (icon_view->priv->model) & BTK_TREE_MODEL_ITERS_PERSIST;
  
  index = btk_tree_path_get_indices(path)[0];

  item = btk_icon_view_item_new ();

  if (iters_persist)
    item->iter = *iter;

  item->index = index;

  /* FIXME: We can be more efficient here,
     we can store a tail pointer and use that when
     appending (which is a rather common operation)
  */
  icon_view->priv->items = g_list_insert (icon_view->priv->items,
					 item, index);
  
  list = g_list_nth (icon_view->priv->items, index + 1);
  for (; list; list = list->next)
    {
      item = list->data;

      item->index++;
    }
    
  verify_items (icon_view);

  btk_icon_view_queue_layout (icon_view);
}

static void
btk_icon_view_row_deleted (BtkTreeModel *model,
			   BtkTreePath  *path,
			   bpointer      data)
{
  bint index;
  BtkIconView *icon_view;
  BtkIconViewItem *item;
  GList *list, *next;
  bboolean emit = FALSE;
  
  icon_view = BTK_ICON_VIEW (data);

  index = btk_tree_path_get_indices(path)[0];

  list = g_list_nth (icon_view->priv->items, index);
  item = list->data;

  btk_icon_view_stop_editing (icon_view, TRUE);

  if (item == icon_view->priv->anchor_item)
    icon_view->priv->anchor_item = NULL;

  if (item == icon_view->priv->cursor_item)
    icon_view->priv->cursor_item = NULL;

  if (item->selected)
    emit = TRUE;
  
  btk_icon_view_item_free (item);

  for (next = list->next; next; next = next->next)
    {
      item = next->data;

      item->index--;
    }
  
  icon_view->priv->items = g_list_delete_link (icon_view->priv->items, list);

  verify_items (icon_view);  
  
  btk_icon_view_queue_layout (icon_view);

  if (emit)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}

static void
btk_icon_view_rows_reordered (BtkTreeModel *model,
			      BtkTreePath  *parent,
			      BtkTreeIter  *iter,
			      bint         *new_order,
			      bpointer      data)
{
  int i;
  int length;
  BtkIconView *icon_view;
  GList *items = NULL, *list;
  BtkIconViewItem **item_array;
  bint *order;
  
  icon_view = BTK_ICON_VIEW (data);

  btk_icon_view_stop_editing (icon_view, TRUE);

  length = btk_tree_model_iter_n_children (model, NULL);

  order = g_new (bint, length);
  for (i = 0; i < length; i++)
    order [new_order[i]] = i;

  item_array = g_new (BtkIconViewItem *, length);
  for (i = 0, list = icon_view->priv->items; list != NULL; list = list->next, i++)
    item_array[order[i]] = list->data;
  g_free (order);

  for (i = length - 1; i >= 0; i--)
    {
      item_array[i]->index = i;
      items = g_list_prepend (items, item_array[i]);
    }
  
  g_free (item_array);
  g_list_free (icon_view->priv->items);
  icon_view->priv->items = items;

  btk_icon_view_queue_layout (icon_view);

  verify_items (icon_view);  
}

static void
btk_icon_view_build_items (BtkIconView *icon_view)
{
  BtkTreeIter iter;
  int i;
  bboolean iters_persist;
  GList *items = NULL;

  iters_persist = btk_tree_model_get_flags (icon_view->priv->model) & BTK_TREE_MODEL_ITERS_PERSIST;
  
  if (!btk_tree_model_get_iter_first (icon_view->priv->model,
				      &iter))
    return;

  i = 0;
  
  do
    {
      BtkIconViewItem *item = btk_icon_view_item_new ();

      if (iters_persist)
	item->iter = iter;

      item->index = i;
      
      i++;

      items = g_list_prepend (items, item);
      
    } while (btk_tree_model_iter_next (icon_view->priv->model, &iter));

  icon_view->priv->items = g_list_reverse (items);
}

static void
btk_icon_view_add_move_binding (BtkBindingSet  *binding_set,
				buint           keyval,
				buint           modmask,
				BtkMovementStep step,
				bint            count)
{
  
  btk_binding_entry_add_signal (binding_set, keyval, modmask,
                                I_("move-cursor"), 2,
                                B_TYPE_ENUM, step,
                                B_TYPE_INT, count);

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

static bboolean
btk_icon_view_real_move_cursor (BtkIconView     *icon_view,
				BtkMovementStep  step,
				bint             count)
{
  BdkModifierType state;

  g_return_val_if_fail (BTK_ICON_VIEW (icon_view), FALSE);
  g_return_val_if_fail (step == BTK_MOVEMENT_LOGICAL_POSITIONS ||
			step == BTK_MOVEMENT_VISUAL_POSITIONS ||
			step == BTK_MOVEMENT_DISPLAY_LINES ||
			step == BTK_MOVEMENT_PAGES ||
			step == BTK_MOVEMENT_BUFFER_ENDS, FALSE);

  if (!btk_widget_has_focus (BTK_WIDGET (icon_view)))
    return FALSE;

  btk_icon_view_stop_editing (icon_view, FALSE);
  btk_widget_grab_focus (BTK_WIDGET (icon_view));

  if (btk_get_current_event_state (&state))
    {
      if ((state & BTK_MODIFY_SELECTION_MOD_MASK) == BTK_MODIFY_SELECTION_MOD_MASK)
        icon_view->priv->modify_selection_pressed = TRUE;
      if ((state & BTK_EXTEND_SELECTION_MOD_MASK) == BTK_EXTEND_SELECTION_MOD_MASK)
        icon_view->priv->extend_selection_pressed = TRUE;
    }
  /* else we assume not pressed */

  switch (step)
    {
    case BTK_MOVEMENT_LOGICAL_POSITIONS:
    case BTK_MOVEMENT_VISUAL_POSITIONS:
      btk_icon_view_move_cursor_left_right (icon_view, count);
      break;
    case BTK_MOVEMENT_DISPLAY_LINES:
      btk_icon_view_move_cursor_up_down (icon_view, count);
      break;
    case BTK_MOVEMENT_PAGES:
      btk_icon_view_move_cursor_page_up_down (icon_view, count);
      break;
    case BTK_MOVEMENT_BUFFER_ENDS:
      btk_icon_view_move_cursor_start_end (icon_view, count);
      break;
    default:
      g_assert_not_reached ();
    }

  icon_view->priv->modify_selection_pressed = FALSE;
  icon_view->priv->extend_selection_pressed = FALSE;

  icon_view->priv->draw_focus = TRUE;

  return TRUE;
}

static BtkIconViewItem *
find_item (BtkIconView     *icon_view,
	   BtkIconViewItem *current,
	   bint             row_ofs,
	   bint             col_ofs)
{
  bint row, col;
  GList *items;
  BtkIconViewItem *item;

  /* FIXME: this could be more efficient 
   */
  row = current->row + row_ofs;
  col = current->col + col_ofs;

  for (items = icon_view->priv->items; items; items = items->next)
    {
      item = items->data;
      if (item->row == row && item->col == col)
	return item;
    }
  
  return NULL;
}

static bint
find_cell (BtkIconView     *icon_view,
	   BtkIconViewItem *item,
	   bint             cell,
	   BtkOrientation   orientation,
	   bint             step,
	   bint            *count)
{
  bint n_focusable;
  bint *focusable;
  bint current;
  bint i, k;
  GList *l;

  if (icon_view->priv->item_orientation != orientation)
    return cell;

  btk_icon_view_set_cell_data (icon_view, item);

  focusable = g_new0 (bint, icon_view->priv->n_cells);
  n_focusable = 0;

  current = 0;
  for (k = 0; k < 2; k++)
    for (l = icon_view->priv->cell_list, i = 0; l; l = l->next, i++)
      {
	BtkIconViewCellInfo *info = (BtkIconViewCellInfo *)l->data;
	
	if (info->pack == (k ? BTK_PACK_START : BTK_PACK_END))
	  continue;
	
	if (!info->cell->visible)
	  continue;

	if (info->cell->mode != BTK_CELL_RENDERER_MODE_INERT)
	  {
	    if (cell == i)
	      current = n_focusable;

	    focusable[n_focusable] = i;

	    n_focusable++;
	  }
      }
  
  if (n_focusable == 0)
    {
      g_free (focusable);
      return -1;
    }

  if (cell < 0)
    {
      current = step > 0 ? 0 : n_focusable - 1;
      cell = focusable[current];
    }

  if (current + *count < 0)
    {
      cell = -1;
      *count = current + *count;
    }
  else if (current + *count > n_focusable - 1)
    {
      cell = -1;
      *count = current + *count - (n_focusable - 1);
    }
  else
    {
      cell = focusable[current + *count];
      *count = 0;
    }
  
  g_free (focusable);
  
  return cell;
}

static BtkIconViewItem *
find_item_page_up_down (BtkIconView     *icon_view,
			BtkIconViewItem *current,
			bint             count)
{
  GList *item, *next;
  bint y, col;
  
  col = current->col;
  y = current->y + count * icon_view->priv->vadjustment->page_size;

  item = g_list_find (icon_view->priv->items, current);
  if (count > 0)
    {
      while (item)
	{
	  for (next = item->next; next; next = next->next)
	    {
	      if (((BtkIconViewItem *)next->data)->col == col)
		break;
	    }
	  if (!next || ((BtkIconViewItem *)next->data)->y > y)
	    break;

	  item = next;
	}
    }
  else 
    {
      while (item)
	{
	  for (next = item->prev; next; next = next->prev)
	    {
	      if (((BtkIconViewItem *)next->data)->col == col)
		break;
	    }
	  if (!next || ((BtkIconViewItem *)next->data)->y < y)
	    break;

	  item = next;
	}
    }

  if (item)
    return item->data;

  return NULL;
}

static bboolean
btk_icon_view_select_all_between (BtkIconView     *icon_view,
				  BtkIconViewItem *anchor,
				  BtkIconViewItem *cursor)
{
  GList *items;
  BtkIconViewItem *item;
  bint row1, row2, col1, col2;
  bboolean dirty = FALSE;
  
  if (anchor->row < cursor->row)
    {
      row1 = anchor->row;
      row2 = cursor->row;
    }
  else
    {
      row1 = cursor->row;
      row2 = anchor->row;
    }

  if (anchor->col < cursor->col)
    {
      col1 = anchor->col;
      col2 = cursor->col;
    }
  else
    {
      col1 = cursor->col;
      col2 = anchor->col;
    }

  for (items = icon_view->priv->items; items; items = items->next)
    {
      item = items->data;

      if (row1 <= item->row && item->row <= row2 &&
	  col1 <= item->col && item->col <= col2)
	{
	  if (!item->selected)
	    {
	      dirty = TRUE;
	      item->selected = TRUE;
	      btk_icon_view_item_selected_changed (icon_view, item);
	    }
	  btk_icon_view_queue_draw_item (icon_view, item);
	}
    }

  return dirty;
}

static void 
btk_icon_view_move_cursor_up_down (BtkIconView *icon_view,
				   bint         count)
{
  BtkIconViewItem *item;
  bint cell;
  bboolean dirty = FALSE;
  bint step;
  BtkDirectionType direction;
  
  if (!btk_widget_has_focus (BTK_WIDGET (icon_view)))
    return;

  direction = count < 0 ? BTK_DIR_UP : BTK_DIR_DOWN;

  if (!icon_view->priv->cursor_item)
    {
      GList *list;

      if (count > 0)
	list = icon_view->priv->items;
      else
	list = g_list_last (icon_view->priv->items);

      item = list ? list->data : NULL;
      cell = -1;
    }
  else
    {
      item = icon_view->priv->cursor_item;
      cell = icon_view->priv->cursor_cell;
      step = count > 0 ? 1 : -1;      
      while (item)
	{
	  cell = find_cell (icon_view, item, cell,
			    BTK_ORIENTATION_VERTICAL, 
			    step, &count);
	  if (count == 0)
	    break;

	  item = find_item (icon_view, item, step, 0);
	  count = count - step;
	}
    }

  if (!item)
    {
      if (!btk_widget_keynav_failed (BTK_WIDGET (icon_view), direction))
        {
          BtkWidget *toplevel = btk_widget_get_toplevel (BTK_WIDGET (icon_view));
          if (toplevel)
            btk_widget_child_focus (toplevel,
                                    direction == BTK_DIR_UP ?
                                    BTK_DIR_TAB_BACKWARD :
                                    BTK_DIR_TAB_FORWARD);
        }

      return;
    }

  if (icon_view->priv->modify_selection_pressed ||
      !icon_view->priv->extend_selection_pressed ||
      !icon_view->priv->anchor_item ||
      icon_view->priv->selection_mode != BTK_SELECTION_MULTIPLE)
    icon_view->priv->anchor_item = item;

  btk_icon_view_set_cursor_item (icon_view, item, cell);

  if (!icon_view->priv->modify_selection_pressed &&
      icon_view->priv->selection_mode != BTK_SELECTION_NONE)
    {
      dirty = btk_icon_view_unselect_all_internal (icon_view);
      dirty = btk_icon_view_select_all_between (icon_view, 
						icon_view->priv->anchor_item,
						item) || dirty;
    }

  btk_icon_view_scroll_to_item (icon_view, item);

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}

static void 
btk_icon_view_move_cursor_page_up_down (BtkIconView *icon_view,
					bint         count)
{
  BtkIconViewItem *item;
  bboolean dirty = FALSE;
  
  if (!btk_widget_has_focus (BTK_WIDGET (icon_view)))
    return;
  
  if (!icon_view->priv->cursor_item)
    {
      GList *list;

      if (count > 0)
	list = icon_view->priv->items;
      else
	list = g_list_last (icon_view->priv->items);

      item = list ? list->data : NULL;
    }
  else
    item = find_item_page_up_down (icon_view, 
				   icon_view->priv->cursor_item,
				   count);

  if (item == icon_view->priv->cursor_item)
    btk_widget_error_bell (BTK_WIDGET (icon_view));

  if (!item)
    return;

  if (icon_view->priv->modify_selection_pressed ||
      !icon_view->priv->extend_selection_pressed ||
      !icon_view->priv->anchor_item ||
      icon_view->priv->selection_mode != BTK_SELECTION_MULTIPLE)
    icon_view->priv->anchor_item = item;

  btk_icon_view_set_cursor_item (icon_view, item, -1);

  if (!icon_view->priv->modify_selection_pressed &&
      icon_view->priv->selection_mode != BTK_SELECTION_NONE)
    {
      dirty = btk_icon_view_unselect_all_internal (icon_view);
      dirty = btk_icon_view_select_all_between (icon_view, 
						icon_view->priv->anchor_item,
						item) || dirty;
    }

  btk_icon_view_scroll_to_item (icon_view, item);

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);  
}

static void 
btk_icon_view_move_cursor_left_right (BtkIconView *icon_view,
				      bint         count)
{
  BtkIconViewItem *item;
  bint cell = -1;
  bboolean dirty = FALSE;
  bint step;
  BtkDirectionType direction;

  if (!btk_widget_has_focus (BTK_WIDGET (icon_view)))
    return;

  direction = count < 0 ? BTK_DIR_LEFT : BTK_DIR_RIGHT;

  if (!icon_view->priv->cursor_item)
    {
      GList *list;

      if (count > 0)
	list = icon_view->priv->items;
      else
	list = g_list_last (icon_view->priv->items);

      item = list ? list->data : NULL;
    }
  else
    {
      item = icon_view->priv->cursor_item;
      cell = icon_view->priv->cursor_cell;
      step = count > 0 ? 1 : -1;
      while (item)
	{
	  cell = find_cell (icon_view, item, cell,
			    BTK_ORIENTATION_HORIZONTAL, 
			    step, &count);
	  if (count == 0)
	    break;
	  
	  item = find_item (icon_view, item, 0, step);
	  count = count - step;
	}
    }

  if (!item)
    {
      if (!btk_widget_keynav_failed (BTK_WIDGET (icon_view), direction))
        {
          BtkWidget *toplevel = btk_widget_get_toplevel (BTK_WIDGET (icon_view));
          if (toplevel)
            btk_widget_child_focus (toplevel,
                                    direction == BTK_DIR_LEFT ?
                                    BTK_DIR_TAB_BACKWARD :
                                    BTK_DIR_TAB_FORWARD);
        }

      return;
    }

  if (icon_view->priv->modify_selection_pressed ||
      !icon_view->priv->extend_selection_pressed ||
      !icon_view->priv->anchor_item ||
      icon_view->priv->selection_mode != BTK_SELECTION_MULTIPLE)
    icon_view->priv->anchor_item = item;

  btk_icon_view_set_cursor_item (icon_view, item, cell);

  if (!icon_view->priv->modify_selection_pressed &&
      icon_view->priv->selection_mode != BTK_SELECTION_NONE)
    {
      dirty = btk_icon_view_unselect_all_internal (icon_view);
      dirty = btk_icon_view_select_all_between (icon_view, 
						icon_view->priv->anchor_item,
						item) || dirty;
    }

  btk_icon_view_scroll_to_item (icon_view, item);

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}

static void 
btk_icon_view_move_cursor_start_end (BtkIconView *icon_view,
				     bint         count)
{
  BtkIconViewItem *item;
  GList *list;
  bboolean dirty = FALSE;
  
  if (!btk_widget_has_focus (BTK_WIDGET (icon_view)))
    return;
  
  if (count < 0)
    list = icon_view->priv->items;
  else
    list = g_list_last (icon_view->priv->items);
  
  item = list ? list->data : NULL;

  if (item == icon_view->priv->cursor_item)
    btk_widget_error_bell (BTK_WIDGET (icon_view));

  if (!item)
    return;

  if (icon_view->priv->modify_selection_pressed ||
      !icon_view->priv->extend_selection_pressed ||
      !icon_view->priv->anchor_item ||
      icon_view->priv->selection_mode != BTK_SELECTION_MULTIPLE)
    icon_view->priv->anchor_item = item;

  btk_icon_view_set_cursor_item (icon_view, item, -1);

  if (!icon_view->priv->modify_selection_pressed &&
      icon_view->priv->selection_mode != BTK_SELECTION_NONE)
    {
      dirty = btk_icon_view_unselect_all_internal (icon_view);
      dirty = btk_icon_view_select_all_between (icon_view, 
						icon_view->priv->anchor_item,
						item) || dirty;
    }

  btk_icon_view_scroll_to_item (icon_view, item);

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}

/**
 * btk_icon_view_scroll_to_path:
 * @icon_view: A #BtkIconView.
 * @path: The path of the item to move to.
 * @use_align: whether to use alignment arguments, or %FALSE.
 * @row_align: The vertical alignment of the item specified by @path.
 * @col_align: The horizontal alignment of the item specified by @path.
 *
 * Moves the alignments of @icon_view to the position specified by @path.  
 * @row_align determines where the row is placed, and @col_align determines 
 * where @column is placed.  Both are expected to be between 0.0 and 1.0. 
 * 0.0 means left/top alignment, 1.0 means right/bottom alignment, 0.5 means 
 * center.
 *
 * If @use_align is %FALSE, then the alignment arguments are ignored, and the
 * tree does the minimum amount of work to scroll the item onto the screen.
 * This means that the item will be scrolled to the edge closest to its current
 * position.  If the item is currently visible on the screen, nothing is done.
 *
 * This function only works if the model is set, and @path is a valid row on 
 * the model. If the model changes before the @icon_view is realized, the 
 * centered path will be modified to reflect this change.
 *
 * Since: 2.8
 **/
void
btk_icon_view_scroll_to_path (BtkIconView *icon_view,
			      BtkTreePath *path,
			      bboolean     use_align,
			      bfloat       row_align,
			      bfloat       col_align)
{
  BtkIconViewItem *item = NULL;

  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));
  g_return_if_fail (path != NULL);
  g_return_if_fail (row_align >= 0.0 && row_align <= 1.0);
  g_return_if_fail (col_align >= 0.0 && col_align <= 1.0);

  if (btk_tree_path_get_depth (path) > 0)
    item = g_list_nth_data (icon_view->priv->items,
			    btk_tree_path_get_indices(path)[0]);
  
  if (!item || item->width < 0 ||
      !btk_widget_get_realized (BTK_WIDGET (icon_view)))
    {
      if (icon_view->priv->scroll_to_path)
	btk_tree_row_reference_free (icon_view->priv->scroll_to_path);

      icon_view->priv->scroll_to_path = NULL;

      if (path)
	icon_view->priv->scroll_to_path = btk_tree_row_reference_new_proxy (B_OBJECT (icon_view), icon_view->priv->model, path);

      icon_view->priv->scroll_to_use_align = use_align;
      icon_view->priv->scroll_to_row_align = row_align;
      icon_view->priv->scroll_to_col_align = col_align;

      return;
    }

  if (use_align)
    {
      bint x, y;
      bint focus_width;
      bfloat offset, value;

      btk_widget_style_get (BTK_WIDGET (icon_view),
			    "focus-line-width", &focus_width,
			    NULL);
      
      bdk_window_get_position (icon_view->priv->bin_window, &x, &y);
      
      offset =  y + item->y - focus_width - 
	row_align * (BTK_WIDGET (icon_view)->allocation.height - item->height);
      value = CLAMP (icon_view->priv->vadjustment->value + offset, 
		     icon_view->priv->vadjustment->lower,
		     icon_view->priv->vadjustment->upper - icon_view->priv->vadjustment->page_size);
      btk_adjustment_set_value (icon_view->priv->vadjustment, value);

      offset = x + item->x - focus_width - 
	col_align * (BTK_WIDGET (icon_view)->allocation.width - item->width);
      value = CLAMP (icon_view->priv->hadjustment->value + offset, 
		     icon_view->priv->hadjustment->lower,
		     icon_view->priv->hadjustment->upper - icon_view->priv->hadjustment->page_size);
      btk_adjustment_set_value (icon_view->priv->hadjustment, value);

      btk_adjustment_changed (icon_view->priv->hadjustment);
      btk_adjustment_changed (icon_view->priv->vadjustment);
    }
  else
    btk_icon_view_scroll_to_item (icon_view, item);
}


static void     
btk_icon_view_scroll_to_item (BtkIconView     *icon_view, 
			      BtkIconViewItem *item)
{
  bint x, y, width, height;
  bint focus_width;

  btk_widget_style_get (BTK_WIDGET (icon_view),
			"focus-line-width", &focus_width,
			NULL);

  width = bdk_window_get_width (icon_view->priv->bin_window);
  height = bdk_window_get_height (icon_view->priv->bin_window);
  bdk_window_get_position (icon_view->priv->bin_window, &x, &y);
  
  if (y + item->y - focus_width < 0)
    btk_adjustment_set_value (icon_view->priv->vadjustment, 
			      icon_view->priv->vadjustment->value + y + item->y - focus_width);
  else if (y + item->y + item->height + focus_width > BTK_WIDGET (icon_view)->allocation.height)
    btk_adjustment_set_value (icon_view->priv->vadjustment, 
			      icon_view->priv->vadjustment->value + y + item->y + item->height 
			      + focus_width - BTK_WIDGET (icon_view)->allocation.height);

  if (x + item->x - focus_width < 0)
    btk_adjustment_set_value (icon_view->priv->hadjustment, 
			      icon_view->priv->hadjustment->value + x + item->x - focus_width);
  else if (x + item->x + item->width + focus_width > BTK_WIDGET (icon_view)->allocation.width)
    btk_adjustment_set_value (icon_view->priv->hadjustment, 
			      icon_view->priv->hadjustment->value + x + item->x + item->width 
			      + focus_width - BTK_WIDGET (icon_view)->allocation.width);

  btk_adjustment_changed (icon_view->priv->hadjustment);
  btk_adjustment_changed (icon_view->priv->vadjustment);
}

/* BtkCellLayout implementation */
static BtkIconViewCellInfo *
btk_icon_view_get_cell_info (BtkIconView     *icon_view,
                             BtkCellRenderer *renderer)
{
  GList *i;

  for (i = icon_view->priv->cell_list; i; i = i->next)
    {
      BtkIconViewCellInfo *info = (BtkIconViewCellInfo *)i->data;

      if (info->cell == renderer)
        return info;
    }

  return NULL;
}

static void
btk_icon_view_set_cell_data (BtkIconView     *icon_view, 
			     BtkIconViewItem *item)
{
  GList *i;
  bboolean iters_persist;
  BtkTreeIter iter;
  
  iters_persist = btk_tree_model_get_flags (icon_view->priv->model) & BTK_TREE_MODEL_ITERS_PERSIST;
  
  if (!iters_persist)
    {
      BtkTreePath *path;

      path = btk_tree_path_new_from_indices (item->index, -1);
      if (!btk_tree_model_get_iter (icon_view->priv->model, &iter, path))
        return;
      btk_tree_path_free (path);
    }
  else
    iter = item->iter;
  
  for (i = icon_view->priv->cell_list; i; i = i->next)
    {
      GSList *j;
      BtkIconViewCellInfo *info = i->data;

      g_object_freeze_notify (B_OBJECT (info->cell));

      for (j = info->attributes; j && j->next; j = j->next->next)
        {
          bchar *property = j->data;
          bint column = BPOINTER_TO_INT (j->next->data);
          BValue value = {0, };

          btk_tree_model_get_value (icon_view->priv->model, &iter,
                                    column, &value);
          g_object_set_property (B_OBJECT (info->cell),
                                 property, &value);
          b_value_unset (&value);
        }

      if (info->func)
	(* info->func) (BTK_CELL_LAYOUT (icon_view),
			info->cell,
			icon_view->priv->model,
			&iter,
			info->func_data);
      
      g_object_thaw_notify (B_OBJECT (info->cell));
    }  
}

static void 
free_cell_attributes (BtkIconViewCellInfo *info)
{ 
  GSList *list;

  list = info->attributes;
  while (list && list->next)
    {
      g_free (list->data);
      list = list->next->next;
    }
  
  b_slist_free (info->attributes);
  info->attributes = NULL;
}

static void
free_cell_info (BtkIconViewCellInfo *info)
{
  free_cell_attributes (info);

  g_object_unref (info->cell);
  
  if (info->destroy)
    (* info->destroy) (info->func_data);

  g_free (info);
}

static void
btk_icon_view_cell_layout_pack_start (BtkCellLayout   *layout,
                                      BtkCellRenderer *renderer,
                                      bboolean         expand)
{
  BtkIconViewCellInfo *info;
  BtkIconView *icon_view = BTK_ICON_VIEW (layout);

  g_return_if_fail (BTK_IS_CELL_RENDERER (renderer));
  g_return_if_fail (!btk_icon_view_get_cell_info (icon_view, renderer));

  g_object_ref_sink (renderer);

  info = g_new0 (BtkIconViewCellInfo, 1);
  info->cell = renderer;
  info->expand = expand ? TRUE : FALSE;
  info->pack = BTK_PACK_START;
  info->position = icon_view->priv->n_cells;
  
  icon_view->priv->cell_list = g_list_append (icon_view->priv->cell_list, info);
  icon_view->priv->n_cells++;
}

static void
btk_icon_view_cell_layout_pack_end (BtkCellLayout   *layout,
                                    BtkCellRenderer *renderer,
                                    bboolean         expand)
{
  BtkIconViewCellInfo *info;
  BtkIconView *icon_view = BTK_ICON_VIEW (layout);

  g_return_if_fail (BTK_IS_CELL_RENDERER (renderer));
  g_return_if_fail (!btk_icon_view_get_cell_info (icon_view, renderer));

  g_object_ref_sink (renderer);

  info = g_new0 (BtkIconViewCellInfo, 1);
  info->cell = renderer;
  info->expand = expand ? TRUE : FALSE;
  info->pack = BTK_PACK_END;
  info->position = icon_view->priv->n_cells;

  icon_view->priv->cell_list = g_list_append (icon_view->priv->cell_list, info);
  icon_view->priv->n_cells++;
}

static void
btk_icon_view_cell_layout_add_attribute (BtkCellLayout   *layout,
                                         BtkCellRenderer *renderer,
                                         const bchar     *attribute,
                                         bint             column)
{
  BtkIconViewCellInfo *info;
  BtkIconView *icon_view = BTK_ICON_VIEW (layout);

  info = btk_icon_view_get_cell_info (icon_view, renderer);
  g_return_if_fail (info != NULL);

  info->attributes = b_slist_prepend (info->attributes,
                                      BINT_TO_POINTER (column));
  info->attributes = b_slist_prepend (info->attributes,
                                      g_strdup (attribute));
}

static void
btk_icon_view_cell_layout_clear (BtkCellLayout *layout)
{
  BtkIconView *icon_view = BTK_ICON_VIEW (layout);

  while (icon_view->priv->cell_list)
    {
      BtkIconViewCellInfo *info = (BtkIconViewCellInfo *)icon_view->priv->cell_list->data;
      free_cell_info (info);
      icon_view->priv->cell_list = g_list_delete_link (icon_view->priv->cell_list, 
						       icon_view->priv->cell_list);
    }

  icon_view->priv->n_cells = 0;
}

static void
btk_icon_view_cell_layout_set_cell_data_func (BtkCellLayout         *layout,
                                              BtkCellRenderer       *cell,
                                              BtkCellLayoutDataFunc  func,
                                              bpointer               func_data,
                                              GDestroyNotify         destroy)
{
  BtkIconViewCellInfo *info;
  BtkIconView *icon_view = BTK_ICON_VIEW (layout);

  info = btk_icon_view_get_cell_info (icon_view, cell);
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
}

static void
btk_icon_view_cell_layout_clear_attributes (BtkCellLayout   *layout,
                                            BtkCellRenderer *renderer)
{
  BtkIconViewCellInfo *info;

  info = btk_icon_view_get_cell_info (BTK_ICON_VIEW (layout), renderer);
  if (info != NULL)
    free_cell_attributes (info);
}

static void
btk_icon_view_cell_layout_reorder (BtkCellLayout   *layout,
                                   BtkCellRenderer *cell,
                                   bint             position)
{
  BtkIconView *icon_view;
  GList *link, *l;
  BtkIconViewCellInfo *info;
  bint i;

  icon_view = BTK_ICON_VIEW (layout);

  g_return_if_fail (BTK_IS_CELL_RENDERER (cell));

  info = btk_icon_view_get_cell_info (icon_view, cell);

  g_return_if_fail (info != NULL);
  g_return_if_fail (position >= 0);

  link = g_list_find (icon_view->priv->cell_list, info);

  g_return_if_fail (link != NULL);

  icon_view->priv->cell_list = g_list_delete_link (icon_view->priv->cell_list,
                                                   link);
  icon_view->priv->cell_list = g_list_insert (icon_view->priv->cell_list,
                                             info, position);

  for (l = icon_view->priv->cell_list, i = 0; l; l = l->next, i++)
    {
      info = l->data;

      info->position = i;
    }

  btk_widget_queue_draw (BTK_WIDGET (icon_view));
}

static GList *
btk_icon_view_cell_layout_get_cells (BtkCellLayout *layout)
{
  BtkIconView *icon_view = (BtkIconView *)layout;
  GList *retval = NULL, *l;

  for (l = icon_view->priv->cell_list; l; l = l->next)
    {
      BtkIconViewCellInfo *info = (BtkIconViewCellInfo *)l->data;

      retval = g_list_prepend (retval, info->cell);
    }

  return g_list_reverse (retval);
}

/* Public API */


/**
 * btk_icon_view_new:
 * 
 * Creates a new #BtkIconView widget
 * 
 * Return value: A newly created #BtkIconView widget
 *
 * Since: 2.6
 **/
BtkWidget *
btk_icon_view_new (void)
{
  return g_object_new (BTK_TYPE_ICON_VIEW, NULL);
}

/**
 * btk_icon_view_new_with_model:
 * @model: The model.
 * 
 * Creates a new #BtkIconView widget with the model @model.
 * 
 * Return value: A newly created #BtkIconView widget.
 *
 * Since: 2.6 
 **/
BtkWidget *
btk_icon_view_new_with_model (BtkTreeModel *model)
{
  return g_object_new (BTK_TYPE_ICON_VIEW, "model", model, NULL);
}

/**
 * btk_icon_view_convert_widget_to_bin_window_coords:
 * @icon_view: a #BtkIconView 
 * @wx: X coordinate relative to the widget
 * @wy: Y coordinate relative to the widget
 * @bx: (out): return location for bin_window X coordinate
 * @by: (out): return location for bin_window Y coordinate
 * 
 * Converts widget coordinates to coordinates for the bin_window,
 * as expected by e.g. btk_icon_view_get_path_at_pos(). 
 *
 * Since: 2.12
 */
void
btk_icon_view_convert_widget_to_bin_window_coords (BtkIconView *icon_view,
                                                   bint         wx,
                                                   bint         wy, 
                                                   bint        *bx,
                                                   bint        *by)
{
  bint x, y;

  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->bin_window) 
    bdk_window_get_position (icon_view->priv->bin_window, &x, &y);
  else
    x = y = 0;
 
  if (bx)
    *bx = wx - x;
  if (by)
    *by = wy - y;
}

/**
 * btk_icon_view_get_path_at_pos:
 * @icon_view: A #BtkIconView.
 * @x: The x position to be identified
 * @y: The y position to be identified
 * 
 * Finds the path at the point (@x, @y), relative to bin_window coordinates.
 * See btk_icon_view_get_item_at_pos(), if you are also interested in
 * the cell at the specified position. 
 * See btk_icon_view_convert_widget_to_bin_window_coords() for converting
 * widget coordinates to bin_window coordinates.
 * 
 * Return value: The #BtkTreePath corresponding to the icon or %NULL
 * if no icon exists at that position.
 *
 * Since: 2.6 
 **/
BtkTreePath *
btk_icon_view_get_path_at_pos (BtkIconView *icon_view,
			       bint         x,
			       bint         y)
{
  BtkIconViewItem *item;
  BtkTreePath *path;
  
  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), NULL);

  item = btk_icon_view_get_item_at_coords (icon_view, x, y, TRUE, NULL);

  if (!item)
    return NULL;

  path = btk_tree_path_new_from_indices (item->index, -1);

  return path;
}

/**
 * btk_icon_view_get_item_at_pos:
 * @icon_view: A #BtkIconView.
 * @x: The x position to be identified
 * @y: The y position to be identified
 * @path: (allow-none): Return location for the path, or %NULL
 * @cell: Return location for the renderer responsible for the cell
 *   at (@x, @y), or %NULL
 * 
 * Finds the path at the point (@x, @y), relative to bin_window coordinates.
 * In contrast to btk_icon_view_get_path_at_pos(), this function also 
 * obtains the cell at the specified position. The returned path should
 * be freed with btk_tree_path_free().
 * See btk_icon_view_convert_widget_to_bin_window_coords() for converting
 * widget coordinates to bin_window coordinates.
 * 
 * Return value: %TRUE if an item exists at the specified position
 *
 * Since: 2.8
 **/
bboolean 
btk_icon_view_get_item_at_pos (BtkIconView      *icon_view,
			       bint              x,
			       bint              y,
			       BtkTreePath     **path,
			       BtkCellRenderer **cell)
{
  BtkIconViewItem *item;
  BtkIconViewCellInfo *info;
  
  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), FALSE);

  item = btk_icon_view_get_item_at_coords (icon_view, x, y, TRUE, &info);

  if (path != NULL)
    {
      if (item != NULL)
	*path = btk_tree_path_new_from_indices (item->index, -1);
      else
	*path = NULL;
    }

  if (cell != NULL)
    {
      if (info != NULL)
	*cell = info->cell;
      else 
	*cell = NULL;
    }

  return (item != NULL);
}

/**
 * btk_icon_view_set_tooltip_item:
 * @icon_view: a #BtkIconView
 * @tooltip: a #BtkTooltip
 * @path: a #BtkTreePath
 * 
 * Sets the tip area of @tooltip to be the area covered by the item at @path.
 * See also btk_icon_view_set_tooltip_column() for a simpler alternative.
 * See also btk_tooltip_set_tip_area().
 * 
 * Since: 2.12
 */
void 
btk_icon_view_set_tooltip_item (BtkIconView     *icon_view,
                                BtkTooltip      *tooltip,
                                BtkTreePath     *path)
{
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));
  g_return_if_fail (BTK_IS_TOOLTIP (tooltip));

  btk_icon_view_set_tooltip_cell (icon_view, tooltip, path, NULL);
}

/**
 * btk_icon_view_set_tooltip_cell:
 * @icon_view: a #BtkIconView
 * @tooltip: a #BtkTooltip
 * @path: a #BtkTreePath
 * @cell: (allow-none): a #BtkCellRenderer or %NULL
 *
 * Sets the tip area of @tooltip to the area which @cell occupies in
 * the item pointed to by @path. See also btk_tooltip_set_tip_area().
 *
 * See also btk_icon_view_set_tooltip_column() for a simpler alternative.
 *
 * Since: 2.12
 */
void
btk_icon_view_set_tooltip_cell (BtkIconView     *icon_view,
                                BtkTooltip      *tooltip,
                                BtkTreePath     *path,
                                BtkCellRenderer *cell)
{
  BdkRectangle rect;
  BtkIconViewItem *item = NULL;
  BtkIconViewCellInfo *info = NULL;
  bint x, y;
 
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));
  g_return_if_fail (BTK_IS_TOOLTIP (tooltip));
  g_return_if_fail (cell == NULL || BTK_IS_CELL_RENDERER (cell));

  if (btk_tree_path_get_depth (path) > 0)
    item = g_list_nth_data (icon_view->priv->items,
                            btk_tree_path_get_indices(path)[0]);
 
  if (!item)
    return;

  if (cell)
    {
      info = btk_icon_view_get_cell_info (icon_view, cell);
      btk_icon_view_get_cell_area (icon_view, item, info, &rect);
    }
  else
    {
      rect.x = item->x;
      rect.y = item->y;
      rect.width = item->width;
      rect.height = item->height;
    }
  
  if (icon_view->priv->bin_window)
    {
      bdk_window_get_position (icon_view->priv->bin_window, &x, &y);
      rect.x += x;
      rect.y += y; 
    }

  btk_tooltip_set_tip_area (tooltip, &rect); 
}


/**
 * btk_icon_view_get_tooltip_context:
 * @icon_view: an #BtkIconView
 * @x: (inout): the x coordinate (relative to widget coordinates)
 * @y: (inout): the y coordinate (relative to widget coordinates)
 * @keyboard_tip: whether this is a keyboard tooltip or not
 * @model: (out) (allow-none): a pointer to receive a #BtkTreeModel or %NULL
 * @path: (out) (allow-none): a pointer to receive a #BtkTreePath or %NULL
 * @iter: (out) (allow-none): a pointer to receive a #BtkTreeIter or %NULL
 *
 * This function is supposed to be used in a #BtkWidget::query-tooltip
 * signal handler for #BtkIconView.  The @x, @y and @keyboard_tip values
 * which are received in the signal handler, should be passed to this
 * function without modification.
 *
 * The return value indicates whether there is an icon view item at the given
 * coordinates (%TRUE) or not (%FALSE) for mouse tooltips. For keyboard
 * tooltips the item returned will be the cursor item. When %TRUE, then any of
 * @model, @path and @iter which have been provided will be set to point to
 * that row and the corresponding model. @x and @y will always be converted
 * to be relative to @icon_view's bin_window if @keyboard_tooltip is %FALSE.
 *
 * Return value: whether or not the given tooltip context points to a item
 *
 * Since: 2.12
 */
bboolean
btk_icon_view_get_tooltip_context (BtkIconView   *icon_view,
                                   bint          *x,
                                   bint          *y,
                                   bboolean       keyboard_tip,
                                   BtkTreeModel **model,
                                   BtkTreePath  **path,
                                   BtkTreeIter   *iter)
{
  BtkTreePath *tmppath = NULL;

  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), FALSE);
  g_return_val_if_fail (x != NULL, FALSE);
  g_return_val_if_fail (y != NULL, FALSE);

  if (keyboard_tip)
    {
      btk_icon_view_get_cursor (icon_view, &tmppath, NULL);

      if (!tmppath)
        return FALSE;
    }
  else
    {
      btk_icon_view_convert_widget_to_bin_window_coords (icon_view, *x, *y,
                                                         x, y);

      if (!btk_icon_view_get_item_at_pos (icon_view, *x, *y, &tmppath, NULL))
        return FALSE;
    }

  if (model)
    *model = btk_icon_view_get_model (icon_view);

  if (iter)
    btk_tree_model_get_iter (btk_icon_view_get_model (icon_view),
                             iter, tmppath);

  if (path)
    *path = tmppath;
  else
    btk_tree_path_free (tmppath);

  return TRUE;
}

static bboolean
btk_icon_view_set_tooltip_query_cb (BtkWidget  *widget,
                                    bint        x,
                                    bint        y,
                                    bboolean    keyboard_tip,
                                    BtkTooltip *tooltip,
                                    bpointer    data)
{
  bchar *str;
  BtkTreeIter iter;
  BtkTreePath *path;
  BtkTreeModel *model;
  BtkIconView *icon_view = BTK_ICON_VIEW (widget);

  if (!btk_icon_view_get_tooltip_context (BTK_ICON_VIEW (widget),
                                          &x, &y,
                                          keyboard_tip,
                                          &model, &path, &iter))
    return FALSE;

  btk_tree_model_get (model, &iter, icon_view->priv->tooltip_column, &str, -1);

  if (!str)
    {
      btk_tree_path_free (path);
      return FALSE;
    }

  btk_tooltip_set_markup (tooltip, str);
  btk_icon_view_set_tooltip_item (icon_view, tooltip, path);

  btk_tree_path_free (path);
  g_free (str);

  return TRUE;
}


/**
 * btk_icon_view_set_tooltip_column:
 * @icon_view: a #BtkIconView
 * @column: an integer, which is a valid column number for @icon_view's model
 *
 * If you only plan to have simple (text-only) tooltips on full items, you
 * can use this function to have #BtkIconView handle these automatically
 * for you. @column should be set to the column in @icon_view's model
 * containing the tooltip texts, or -1 to disable this feature.
 *
 * When enabled, #BtkWidget::has-tooltip will be set to %TRUE and
 * @icon_view will connect a #BtkWidget::query-tooltip signal handler.
 *
 * Since: 2.12
 */
void
btk_icon_view_set_tooltip_column (BtkIconView *icon_view,
                                  bint         column)
{
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));

  if (column == icon_view->priv->tooltip_column)
    return;

  if (column == -1)
    {
      g_signal_handlers_disconnect_by_func (icon_view,
                                            btk_icon_view_set_tooltip_query_cb,
                                            NULL);
      btk_widget_set_has_tooltip (BTK_WIDGET (icon_view), FALSE);
    }
  else
    {
      if (icon_view->priv->tooltip_column == -1)
        {
          g_signal_connect (icon_view, "query-tooltip",
                            G_CALLBACK (btk_icon_view_set_tooltip_query_cb), NULL);
          btk_widget_set_has_tooltip (BTK_WIDGET (icon_view), TRUE);
        }
    }

  icon_view->priv->tooltip_column = column;
  g_object_notify (B_OBJECT (icon_view), "tooltip-column");
}

/** 
 * btk_icon_view_get_tooltip_column:
 * @icon_view: a #BtkIconView
 *
 * Returns the column of @icon_view's model which is being used for
 * displaying tooltips on @icon_view's rows.
 *
 * Return value: the index of the tooltip column that is currently being
 * used, or -1 if this is disabled.
 *
 * Since: 2.12
 */
bint
btk_icon_view_get_tooltip_column (BtkIconView *icon_view)
{
  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), 0);

  return icon_view->priv->tooltip_column;
}

/**
 * btk_icon_view_get_visible_range:
 * @icon_view: A #BtkIconView
 * @start_path: (allow-none): Return location for start of rebunnyion, or %NULL
 * @end_path: (allow-none): Return location for end of rebunnyion, or %NULL
 * 
 * Sets @start_path and @end_path to be the first and last visible path. 
 * Note that there may be invisible paths in between.
 * 
 * Both paths should be freed with btk_tree_path_free() after use.
 * 
 * Return value: %TRUE, if valid paths were placed in @start_path and @end_path
 *
 * Since: 2.8
 **/
bboolean
btk_icon_view_get_visible_range (BtkIconView  *icon_view,
				 BtkTreePath **start_path,
				 BtkTreePath **end_path)
{
  bint start_index = -1;
  bint end_index = -1;
  GList *icons;

  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), FALSE);

  if (icon_view->priv->hadjustment == NULL ||
      icon_view->priv->vadjustment == NULL)
    return FALSE;

  if (start_path == NULL && end_path == NULL)
    return FALSE;
  
  for (icons = icon_view->priv->items; icons; icons = icons->next) 
    {
      BtkIconViewItem *item = icons->data;

      if ((item->x + item->width >= (int)icon_view->priv->hadjustment->value) &&
	  (item->y + item->height >= (int)icon_view->priv->vadjustment->value) &&
	  (item->x <= (int) (icon_view->priv->hadjustment->value + icon_view->priv->hadjustment->page_size)) &&
	  (item->y <= (int) (icon_view->priv->vadjustment->value + icon_view->priv->vadjustment->page_size)))
	{
	  if (start_index == -1)
	    start_index = item->index;
	  end_index = item->index;
	}
    }

  if (start_path && start_index != -1)
    *start_path = btk_tree_path_new_from_indices (start_index, -1);
  if (end_path && end_index != -1)
    *end_path = btk_tree_path_new_from_indices (end_index, -1);
  
  return start_index != -1;
}

/**
 * btk_icon_view_selected_foreach:
 * @icon_view: A #BtkIconView.
 * @func: (scope call): The function to call for each selected icon.
 * @data: User data to pass to the function.
 * 
 * Calls a function for each selected icon. Note that the model or
 * selection cannot be modified from within this function.
 *
 * Since: 2.6 
 **/
void
btk_icon_view_selected_foreach (BtkIconView           *icon_view,
				BtkIconViewForeachFunc func,
				bpointer               data)
{
  GList *list;
  
  for (list = icon_view->priv->items; list; list = list->next)
    {
      BtkIconViewItem *item = list->data;
      BtkTreePath *path = btk_tree_path_new_from_indices (item->index, -1);

      if (item->selected)
	(* func) (icon_view, path, data);

      btk_tree_path_free (path);
    }
}

/**
 * btk_icon_view_set_selection_mode:
 * @icon_view: A #BtkIconView.
 * @mode: The selection mode
 * 
 * Sets the selection mode of the @icon_view.
 *
 * Since: 2.6 
 **/
void
btk_icon_view_set_selection_mode (BtkIconView      *icon_view,
				  BtkSelectionMode  mode)
{
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));

  if (mode == icon_view->priv->selection_mode)
    return;
  
  if (mode == BTK_SELECTION_NONE ||
      icon_view->priv->selection_mode == BTK_SELECTION_MULTIPLE)
    btk_icon_view_unselect_all (icon_view);
  
  icon_view->priv->selection_mode = mode;

  g_object_notify (B_OBJECT (icon_view), "selection-mode");
}

/**
 * btk_icon_view_get_selection_mode:
 * @icon_view: A #BtkIconView.
 * 
 * Gets the selection mode of the @icon_view.
 *
 * Return value: the current selection mode
 *
 * Since: 2.6 
 **/
BtkSelectionMode
btk_icon_view_get_selection_mode (BtkIconView *icon_view)
{
  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), BTK_SELECTION_SINGLE);

  return icon_view->priv->selection_mode;
}

/**
 * btk_icon_view_set_model:
 * @icon_view: A #BtkIconView.
 * @model: (allow-none): The model.
 *
 * Sets the model for a #BtkIconView.
 * If the @icon_view already has a model set, it will remove
 * it before setting the new model.  If @model is %NULL, then
 * it will unset the old model.
 *
 * Since: 2.6 
 **/
void
btk_icon_view_set_model (BtkIconView *icon_view,
			 BtkTreeModel *model)
{
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));
  g_return_if_fail (model == NULL || BTK_IS_TREE_MODEL (model));
  
  if (icon_view->priv->model == model)
    return;

  if (icon_view->priv->scroll_to_path)
    {
      btk_tree_row_reference_free (icon_view->priv->scroll_to_path);
      icon_view->priv->scroll_to_path = NULL;
    }

  btk_icon_view_stop_editing (icon_view, TRUE);

  if (model)
    {
      GType column_type;
      
      g_return_if_fail (btk_tree_model_get_flags (model) & BTK_TREE_MODEL_LIST_ONLY);

      if (icon_view->priv->pixbuf_column != -1)
	{
	  column_type = btk_tree_model_get_column_type (model,
							icon_view->priv->pixbuf_column);	  

	  g_return_if_fail (column_type == BDK_TYPE_PIXBUF);
	}

      if (icon_view->priv->text_column != -1)
	{
	  column_type = btk_tree_model_get_column_type (model,
							icon_view->priv->text_column);	  

	  g_return_if_fail (column_type == B_TYPE_STRING);
	}

      if (icon_view->priv->markup_column != -1)
	{
	  column_type = btk_tree_model_get_column_type (model,
							icon_view->priv->markup_column);	  

	  g_return_if_fail (column_type == B_TYPE_STRING);
	}
      
    }
  
  if (icon_view->priv->model)
    {
      g_signal_handlers_disconnect_by_func (icon_view->priv->model,
					    btk_icon_view_row_changed,
					    icon_view);
      g_signal_handlers_disconnect_by_func (icon_view->priv->model,
					    btk_icon_view_row_inserted,
					    icon_view);
      g_signal_handlers_disconnect_by_func (icon_view->priv->model,
					    btk_icon_view_row_deleted,
					    icon_view);
      g_signal_handlers_disconnect_by_func (icon_view->priv->model,
					    btk_icon_view_rows_reordered,
					    icon_view);

      g_object_unref (icon_view->priv->model);
      
      g_list_foreach (icon_view->priv->items, (GFunc)btk_icon_view_item_free, NULL);
      g_list_free (icon_view->priv->items);
      icon_view->priv->items = NULL;
      icon_view->priv->anchor_item = NULL;
      icon_view->priv->cursor_item = NULL;
      icon_view->priv->last_single_clicked = NULL;
      icon_view->priv->width = 0;
      icon_view->priv->height = 0;
    }

  icon_view->priv->model = model;

  if (icon_view->priv->model)
    {
      g_object_ref (icon_view->priv->model);
      g_signal_connect (icon_view->priv->model,
			"row-changed",
			G_CALLBACK (btk_icon_view_row_changed),
			icon_view);
      g_signal_connect (icon_view->priv->model,
			"row-inserted",
			G_CALLBACK (btk_icon_view_row_inserted),
			icon_view);
      g_signal_connect (icon_view->priv->model,
			"row-deleted",
			G_CALLBACK (btk_icon_view_row_deleted),
			icon_view);
      g_signal_connect (icon_view->priv->model,
			"rows-reordered",
			G_CALLBACK (btk_icon_view_rows_reordered),
			icon_view);

      btk_icon_view_build_items (icon_view);

      btk_icon_view_queue_layout (icon_view);
    }

  g_object_notify (B_OBJECT (icon_view), "model");  

  if (btk_widget_get_realized (BTK_WIDGET (icon_view)))
    btk_widget_queue_resize (BTK_WIDGET (icon_view));
}

/**
 * btk_icon_view_get_model:
 * @icon_view: a #BtkIconView
 *
 * Returns the model the #BtkIconView is based on.  Returns %NULL if the
 * model is unset.
 *
 * Return value: (transfer none): A #BtkTreeModel, or %NULL if none is
 *     currently being used.
 *
 * Since: 2.6 
 **/
BtkTreeModel *
btk_icon_view_get_model (BtkIconView *icon_view)
{
  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), NULL);

  return icon_view->priv->model;
}

static void
update_text_cell (BtkIconView *icon_view)
{
  BtkIconViewCellInfo *info;
  GList *l;
  bint i;
	  
  if (icon_view->priv->text_column == -1 &&
      icon_view->priv->markup_column == -1)
    {
      if (icon_view->priv->text_cell != -1)
	{
	  if (icon_view->priv->pixbuf_cell > icon_view->priv->text_cell)
	    icon_view->priv->pixbuf_cell--;

	  info = g_list_nth_data (icon_view->priv->cell_list, 
				  icon_view->priv->text_cell);
	  
	  icon_view->priv->cell_list = g_list_remove (icon_view->priv->cell_list, info);
	  
	  free_cell_info (info);
	  
	  icon_view->priv->n_cells--;
	  icon_view->priv->text_cell = -1;
	}
    }
  else 
    {
      if (icon_view->priv->text_cell == -1)
	{
	  BtkCellRenderer *cell = btk_cell_renderer_text_new ();
	  btk_cell_layout_pack_end (BTK_CELL_LAYOUT (icon_view), cell, FALSE);
	  for (l = icon_view->priv->cell_list, i = 0; l; l = l->next, i++)
	    {
	      info = l->data;
	      if (info->cell == cell)
		{
		  icon_view->priv->text_cell = i;
		  break;
		}
	    }
	}
      
      info = g_list_nth_data (icon_view->priv->cell_list,
			      icon_view->priv->text_cell);

      if (icon_view->priv->markup_column != -1)
	btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (icon_view),
					info->cell, 
					"markup", icon_view->priv->markup_column, 
					NULL);
      else
	btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (icon_view),
					info->cell, 
					"text", icon_view->priv->text_column, 
					NULL);

      if (icon_view->priv->item_orientation == BTK_ORIENTATION_VERTICAL)
	g_object_set (info->cell,
                      "alignment", BANGO_ALIGN_CENTER,
		      "wrap-mode", BANGO_WRAP_WORD_CHAR,
		      "xalign", 0.0,
		      "yalign", 0.0,
		      NULL);
      else
	g_object_set (info->cell,
                      "alignment", BANGO_ALIGN_LEFT,
		      "wrap-mode", BANGO_WRAP_WORD_CHAR,
		      "xalign", 0.0,
		      "yalign", 0.0,
		      NULL);
    }
}

static void
update_pixbuf_cell (BtkIconView *icon_view)
{
  BtkIconViewCellInfo *info;
  GList *l;
  bint i;

  if (icon_view->priv->pixbuf_column == -1)
    {
      if (icon_view->priv->pixbuf_cell != -1)
	{
	  if (icon_view->priv->text_cell > icon_view->priv->pixbuf_cell)
	    icon_view->priv->text_cell--;

	  info = g_list_nth_data (icon_view->priv->cell_list, 
				  icon_view->priv->pixbuf_cell);
	  
	  icon_view->priv->cell_list = g_list_remove (icon_view->priv->cell_list, info);
	  
	  free_cell_info (info);
	  
	  icon_view->priv->n_cells--;
	  icon_view->priv->pixbuf_cell = -1;
	}
    }
  else 
    {
      if (icon_view->priv->pixbuf_cell == -1)
	{
	  BtkCellRenderer *cell = btk_cell_renderer_pixbuf_new ();
	  
	  btk_cell_layout_pack_start (BTK_CELL_LAYOUT (icon_view), cell, FALSE);
	  for (l = icon_view->priv->cell_list, i = 0; l; l = l->next, i++)
	    {
	      info = l->data;
	      if (info->cell == cell)
		{
		  icon_view->priv->pixbuf_cell = i;
		  break;
		}
	    }
	}
      
	info = g_list_nth_data (icon_view->priv->cell_list, 
				icon_view->priv->pixbuf_cell);
	
	btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (icon_view),
					info->cell, 
					"pixbuf", icon_view->priv->pixbuf_column, 
					NULL);

	if (icon_view->priv->item_orientation == BTK_ORIENTATION_VERTICAL)
	  g_object_set (info->cell,
			"xalign", 0.5,
			"yalign", 1.0,
			NULL);
	else
	  g_object_set (info->cell,
			"xalign", 0.0,
			"yalign", 0.0,
			NULL);
    }
}

/**
 * btk_icon_view_set_text_column:
 * @icon_view: A #BtkIconView.
 * @column: A column in the currently used model, or -1 to display no text
 * 
 * Sets the column with text for @icon_view to be @column. The text
 * column must be of type #B_TYPE_STRING.
 *
 * Since: 2.6 
 **/
void
btk_icon_view_set_text_column (BtkIconView *icon_view,
			       bint          column)
{
  if (column == icon_view->priv->text_column)
    return;
  
  if (column == -1)
    icon_view->priv->text_column = -1;
  else
    {
      if (icon_view->priv->model != NULL)
	{
	  GType column_type;
	  
	  column_type = btk_tree_model_get_column_type (icon_view->priv->model, column);

	  g_return_if_fail (column_type == B_TYPE_STRING);
	}
      
      icon_view->priv->text_column = column;
    }

  btk_icon_view_stop_editing (icon_view, TRUE);

  update_text_cell (icon_view);

  btk_icon_view_invalidate_sizes (icon_view);
  btk_icon_view_queue_layout (icon_view);
  
  g_object_notify (B_OBJECT (icon_view), "text-column");
}

/**
 * btk_icon_view_get_text_column:
 * @icon_view: A #BtkIconView.
 *
 * Returns the column with text for @icon_view.
 *
 * Returns: the text column, or -1 if it's unset.
 *
 * Since: 2.6
 */
bint
btk_icon_view_get_text_column (BtkIconView  *icon_view)
{
  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->text_column;
}

/**
 * btk_icon_view_set_markup_column:
 * @icon_view: A #BtkIconView.
 * @column: A column in the currently used model, or -1 to display no text
 * 
 * Sets the column with markup information for @icon_view to be
 * @column. The markup column must be of type #B_TYPE_STRING.
 * If the markup column is set to something, it overrides
 * the text column set by btk_icon_view_set_text_column().
 *
 * Since: 2.6
 **/
void
btk_icon_view_set_markup_column (BtkIconView *icon_view,
				 bint         column)
{
  if (column == icon_view->priv->markup_column)
    return;
  
  if (column == -1)
    icon_view->priv->markup_column = -1;
  else
    {
      if (icon_view->priv->model != NULL)
	{
	  GType column_type;
	  
	  column_type = btk_tree_model_get_column_type (icon_view->priv->model, column);

	  g_return_if_fail (column_type == B_TYPE_STRING);
	}
      
      icon_view->priv->markup_column = column;
    }

  btk_icon_view_stop_editing (icon_view, TRUE);

  update_text_cell (icon_view);

  btk_icon_view_invalidate_sizes (icon_view);
  btk_icon_view_queue_layout (icon_view);
  
  g_object_notify (B_OBJECT (icon_view), "markup-column");
}

/**
 * btk_icon_view_get_markup_column:
 * @icon_view: A #BtkIconView.
 *
 * Returns the column with markup text for @icon_view.
 *
 * Returns: the markup column, or -1 if it's unset.
 *
 * Since: 2.6
 */
bint
btk_icon_view_get_markup_column (BtkIconView  *icon_view)
{
  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->markup_column;
}

/**
 * btk_icon_view_set_pixbuf_column:
 * @icon_view: A #BtkIconView.
 * @column: A column in the currently used model, or -1 to disable
 * 
 * Sets the column with pixbufs for @icon_view to be @column. The pixbuf
 * column must be of type #BDK_TYPE_PIXBUF
 *
 * Since: 2.6 
 **/
void
btk_icon_view_set_pixbuf_column (BtkIconView *icon_view,
				 bint         column)
{
  if (column == icon_view->priv->pixbuf_column)
    return;
  
  if (column == -1)
    icon_view->priv->pixbuf_column = -1;
  else
    {
      if (icon_view->priv->model != NULL)
	{
	  GType column_type;
	  
	  column_type = btk_tree_model_get_column_type (icon_view->priv->model, column);

	  g_return_if_fail (column_type == BDK_TYPE_PIXBUF);
	}
      
      icon_view->priv->pixbuf_column = column;
    }

  btk_icon_view_stop_editing (icon_view, TRUE);

  update_pixbuf_cell (icon_view);

  btk_icon_view_invalidate_sizes (icon_view);
  btk_icon_view_queue_layout (icon_view);
  
  g_object_notify (B_OBJECT (icon_view), "pixbuf-column");
  
}

/**
 * btk_icon_view_get_pixbuf_column:
 * @icon_view: A #BtkIconView.
 *
 * Returns the column with pixbufs for @icon_view.
 *
 * Returns: the pixbuf column, or -1 if it's unset.
 *
 * Since: 2.6
 */
bint
btk_icon_view_get_pixbuf_column (BtkIconView  *icon_view)
{
  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->pixbuf_column;
}

/**
 * btk_icon_view_select_path:
 * @icon_view: A #BtkIconView.
 * @path: The #BtkTreePath to be selected.
 * 
 * Selects the row at @path.
 *
 * Since: 2.6
 **/
void
btk_icon_view_select_path (BtkIconView *icon_view,
			   BtkTreePath *path)
{
  BtkIconViewItem *item = NULL;

  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));
  g_return_if_fail (icon_view->priv->model != NULL);
  g_return_if_fail (path != NULL);

  if (btk_tree_path_get_depth (path) > 0)
    item = g_list_nth_data (icon_view->priv->items,
			    btk_tree_path_get_indices(path)[0]);

  if (item)
    btk_icon_view_select_item (icon_view, item);
}

/**
 * btk_icon_view_unselect_path:
 * @icon_view: A #BtkIconView.
 * @path: The #BtkTreePath to be unselected.
 * 
 * Unselects the row at @path.
 *
 * Since: 2.6
 **/
void
btk_icon_view_unselect_path (BtkIconView *icon_view,
			     BtkTreePath *path)
{
  BtkIconViewItem *item;
  
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));
  g_return_if_fail (icon_view->priv->model != NULL);
  g_return_if_fail (path != NULL);

  item = g_list_nth_data (icon_view->priv->items,
			  btk_tree_path_get_indices(path)[0]);

  if (!item)
    return;
  
  btk_icon_view_unselect_item (icon_view, item);
}

/**
 * btk_icon_view_get_selected_items:
 * @icon_view: A #BtkIconView.
 *
 * Creates a list of paths of all selected items. Additionally, if you are
 * planning on modifying the model after calling this function, you may
 * want to convert the returned list into a list of #BtkTreeRowReference<!-- -->s.
 * To do this, you can use btk_tree_row_reference_new().
 *
 * To free the return value, use:
 * |[
 * g_list_foreach (list, (GFunc)btk_tree_path_free, NULL);
 * g_list_free (list);
 * ]|
 *
 * Return value: (element-type BtkTreePath) (transfer full): A #GList containing a #BtkTreePath for each selected row.
 *
 * Since: 2.6
 **/
GList *
btk_icon_view_get_selected_items (BtkIconView *icon_view)
{
  GList *list;
  GList *selected = NULL;
  
  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), NULL);
  
  for (list = icon_view->priv->items; list != NULL; list = list->next)
    {
      BtkIconViewItem *item = list->data;

      if (item->selected)
	{
	  BtkTreePath *path = btk_tree_path_new_from_indices (item->index, -1);

	  selected = g_list_prepend (selected, path);
	}
    }

  return selected;
}

/**
 * btk_icon_view_select_all:
 * @icon_view: A #BtkIconView.
 * 
 * Selects all the icons. @icon_view must has its selection mode set
 * to #BTK_SELECTION_MULTIPLE.
 *
 * Since: 2.6
 **/
void
btk_icon_view_select_all (BtkIconView *icon_view)
{
  GList *items;
  bboolean dirty = FALSE;
  
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->selection_mode != BTK_SELECTION_MULTIPLE)
    return;

  for (items = icon_view->priv->items; items; items = items->next)
    {
      BtkIconViewItem *item = items->data;
      
      if (!item->selected)
	{
	  dirty = TRUE;
	  item->selected = TRUE;
	  btk_icon_view_queue_draw_item (icon_view, item);
	}
    }

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}

/**
 * btk_icon_view_unselect_all:
 * @icon_view: A #BtkIconView.
 * 
 * Unselects all the icons.
 *
 * Since: 2.6
 **/
void
btk_icon_view_unselect_all (BtkIconView *icon_view)
{
  bboolean dirty = FALSE;
  
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->selection_mode == BTK_SELECTION_BROWSE)
    return;

  dirty = btk_icon_view_unselect_all_internal (icon_view);

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}

/**
 * btk_icon_view_path_is_selected:
 * @icon_view: A #BtkIconView.
 * @path: A #BtkTreePath to check selection on.
 * 
 * Returns %TRUE if the icon pointed to by @path is currently
 * selected. If @path does not point to a valid location, %FALSE is returned.
 * 
 * Return value: %TRUE if @path is selected.
 *
 * Since: 2.6
 **/
bboolean
btk_icon_view_path_is_selected (BtkIconView *icon_view,
				BtkTreePath *path)
{
  BtkIconViewItem *item;
  
  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), FALSE);
  g_return_val_if_fail (icon_view->priv->model != NULL, FALSE);
  g_return_val_if_fail (path != NULL, FALSE);
  
  item = g_list_nth_data (icon_view->priv->items,
			  btk_tree_path_get_indices(path)[0]);

  if (!item)
    return FALSE;
  
  return item->selected;
}

/**
 * btk_icon_view_get_item_row:
 * @icon_view: a #BtkIconView
 * @path: the #BtkTreePath of the item
 *
 * Gets the row in which the item @path is currently
 * displayed. Row numbers start at 0.
 *
 * Returns: The row in which the item is displayed
 *
 * Since: 2.22
 */
bint
btk_icon_view_get_item_row (BtkIconView *icon_view,
                            BtkTreePath *path)
{
  BtkIconViewItem *item;

  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), FALSE);
  g_return_val_if_fail (icon_view->priv->model != NULL, FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  item = g_list_nth_data (icon_view->priv->items,
                          btk_tree_path_get_indices(path)[0]);

  if (!item)
    return -1;

  return item->row;
}

/**
 * btk_icon_view_get_item_column:
 * @icon_view: a #BtkIconView
 * @path: the #BtkTreePath of the item
 *
 * Gets the column in which the item @path is currently
 * displayed. Column numbers start at 0.
 *
 * Returns: The column in which the item is displayed
 *
 * Since: 2.22
 */
bint
btk_icon_view_get_item_column (BtkIconView *icon_view,
                               BtkTreePath *path)
{
  BtkIconViewItem *item;

  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), FALSE);
  g_return_val_if_fail (icon_view->priv->model != NULL, FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  item = g_list_nth_data (icon_view->priv->items,
                          btk_tree_path_get_indices(path)[0]);

  if (!item)
    return -1;

  return item->col;
}

/**
 * btk_icon_view_item_activated:
 * @icon_view: A #BtkIconView
 * @path: The #BtkTreePath to be activated
 * 
 * Activates the item determined by @path.
 *
 * Since: 2.6
 **/
void
btk_icon_view_item_activated (BtkIconView      *icon_view,
			      BtkTreePath      *path)
{
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));
  g_return_if_fail (path != NULL);
  
  g_signal_emit (icon_view, icon_view_signals[ITEM_ACTIVATED], 0, path);
}

/**
 * btk_icon_view_set_item_orientation:
 * @icon_view: a #BtkIconView
 * @orientation: the relative position of texts and icons 
 * 
 * Sets the ::item-orientation property which determines whether
 * the labels are drawn beside the icons instead of below.
 *
 * Since: 2.22
 **/
void 
btk_icon_view_set_item_orientation (BtkIconView    *icon_view,
                                    BtkOrientation  orientation)
{
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->item_orientation != orientation)
    {
      icon_view->priv->item_orientation = orientation;

      btk_icon_view_stop_editing (icon_view, TRUE);
      btk_icon_view_invalidate_sizes (icon_view);
      btk_icon_view_queue_layout (icon_view);

      update_text_cell (icon_view);
      update_pixbuf_cell (icon_view);
      
      g_object_notify (B_OBJECT (icon_view), "item-orientation");
      g_object_notify (B_OBJECT (icon_view), "orientation");
    }
}

/**
 * btk_icon_view_set_orientation:
 * @icon_view: a #BtkIconView
 * @orientation: the relative position of texts and icons 
 * 
 * Sets the ::orientation property which determines whether the labels 
 * are drawn beside the icons instead of below.
 *
 * Since: 2.6
 *
 * Deprecated: 2.22: Use btk_icon_view_set_item_orientation()
 **/
void 
btk_icon_view_set_orientation (BtkIconView    *icon_view,
                               BtkOrientation  orientation)
{
  btk_icon_view_set_item_orientation (icon_view, orientation);
}

/**
 * btk_icon_view_get_item_orientation:
 * @icon_view: a #BtkIconView
 *
 * Returns the value of the ::item-orientation property which determines
 * whether the labels are drawn beside the icons instead of below.
 *
 * Return value: the relative position of texts and icons
 *
 * Since: 2.22
 */
BtkOrientation
btk_icon_view_get_item_orientation (BtkIconView *icon_view)
{
  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view),
			BTK_ORIENTATION_VERTICAL);

  return icon_view->priv->item_orientation;
}

/**
 * btk_icon_view_get_orientation:
 * @icon_view: a #BtkIconView
 * 
 * Returns the value of the ::orientation property which determines 
 * whether the labels are drawn beside the icons instead of below. 
 * 
 * Return value: the relative position of texts and icons 
 *
 * Since: 2.6
 *
 * Deprecated: 2.22: Use btk_icon_view_get_item_orientation()
 **/
BtkOrientation
btk_icon_view_get_orientation (BtkIconView *icon_view)
{
  return btk_icon_view_get_item_orientation (icon_view);
}

/**
 * btk_icon_view_set_columns:
 * @icon_view: a #BtkIconView
 * @columns: the number of columns
 * 
 * Sets the ::columns property which determines in how
 * many columns the icons are arranged. If @columns is
 * -1, the number of columns will be chosen automatically 
 * to fill the available area. 
 *
 * Since: 2.6
 */
void 
btk_icon_view_set_columns (BtkIconView *icon_view,
			   bint         columns)
{
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));
  
  if (icon_view->priv->columns != columns)
    {
      icon_view->priv->columns = columns;

      btk_icon_view_stop_editing (icon_view, TRUE);
      btk_icon_view_queue_layout (icon_view);
      
      g_object_notify (B_OBJECT (icon_view), "columns");
    }  
}

/**
 * btk_icon_view_get_columns:
 * @icon_view: a #BtkIconView
 * 
 * Returns the value of the ::columns property.
 * 
 * Return value: the number of columns, or -1
 *
 * Since: 2.6
 */
bint
btk_icon_view_get_columns (BtkIconView *icon_view)
{
  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->columns;
}

/**
 * btk_icon_view_set_item_width:
 * @icon_view: a #BtkIconView
 * @item_width: the width for each item
 * 
 * Sets the ::item-width property which specifies the width 
 * to use for each item. If it is set to -1, the icon view will 
 * automatically determine a suitable item size.
 *
 * Since: 2.6
 */
void 
btk_icon_view_set_item_width (BtkIconView *icon_view,
			      bint         item_width)
{
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));
  
  if (icon_view->priv->item_width != item_width)
    {
      icon_view->priv->item_width = item_width;
      
      btk_icon_view_stop_editing (icon_view, TRUE);
      btk_icon_view_invalidate_sizes (icon_view);
      btk_icon_view_queue_layout (icon_view);
      
      update_text_cell (icon_view);

      g_object_notify (B_OBJECT (icon_view), "item-width");
    }  
}

/**
 * btk_icon_view_get_item_width:
 * @icon_view: a #BtkIconView
 * 
 * Returns the value of the ::item-width property.
 * 
 * Return value: the width of a single item, or -1
 *
 * Since: 2.6
 */
bint
btk_icon_view_get_item_width (BtkIconView *icon_view)
{
  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->item_width;
}


/**
 * btk_icon_view_set_spacing:
 * @icon_view: a #BtkIconView
 * @spacing: the spacing
 * 
 * Sets the ::spacing property which specifies the space 
 * which is inserted between the cells (i.e. the icon and 
 * the text) of an item.
 *
 * Since: 2.6
 */
void 
btk_icon_view_set_spacing (BtkIconView *icon_view,
			   bint         spacing)
{
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));
  
  if (icon_view->priv->spacing != spacing)
    {
      icon_view->priv->spacing = spacing;

      btk_icon_view_stop_editing (icon_view, TRUE);
      btk_icon_view_invalidate_sizes (icon_view);
      btk_icon_view_queue_layout (icon_view);
      
      g_object_notify (B_OBJECT (icon_view), "spacing");
    }  
}

/**
 * btk_icon_view_get_spacing:
 * @icon_view: a #BtkIconView
 * 
 * Returns the value of the ::spacing property.
 * 
 * Return value: the space between cells 
 *
 * Since: 2.6
 */
bint
btk_icon_view_get_spacing (BtkIconView *icon_view)
{
  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->spacing;
}

/**
 * btk_icon_view_set_row_spacing:
 * @icon_view: a #BtkIconView
 * @row_spacing: the row spacing
 * 
 * Sets the ::row-spacing property which specifies the space 
 * which is inserted between the rows of the icon view.
 *
 * Since: 2.6
 */
void 
btk_icon_view_set_row_spacing (BtkIconView *icon_view,
			       bint         row_spacing)
{
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));
  
  if (icon_view->priv->row_spacing != row_spacing)
    {
      icon_view->priv->row_spacing = row_spacing;

      btk_icon_view_stop_editing (icon_view, TRUE);
      btk_icon_view_invalidate_sizes (icon_view);
      btk_icon_view_queue_layout (icon_view);
      
      g_object_notify (B_OBJECT (icon_view), "row-spacing");
    }  
}

/**
 * btk_icon_view_get_row_spacing:
 * @icon_view: a #BtkIconView
 * 
 * Returns the value of the ::row-spacing property.
 * 
 * Return value: the space between rows
 *
 * Since: 2.6
 */
bint
btk_icon_view_get_row_spacing (BtkIconView *icon_view)
{
  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->row_spacing;
}

/**
 * btk_icon_view_set_column_spacing:
 * @icon_view: a #BtkIconView
 * @column_spacing: the column spacing
 * 
 * Sets the ::column-spacing property which specifies the space 
 * which is inserted between the columns of the icon view.
 *
 * Since: 2.6
 */
void 
btk_icon_view_set_column_spacing (BtkIconView *icon_view,
				  bint         column_spacing)
{
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));
  
  if (icon_view->priv->column_spacing != column_spacing)
    {
      icon_view->priv->column_spacing = column_spacing;

      btk_icon_view_stop_editing (icon_view, TRUE);
      btk_icon_view_invalidate_sizes (icon_view);
      btk_icon_view_queue_layout (icon_view);
      
      g_object_notify (B_OBJECT (icon_view), "column-spacing");
    }  
}

/**
 * btk_icon_view_get_column_spacing:
 * @icon_view: a #BtkIconView
 * 
 * Returns the value of the ::column-spacing property.
 * 
 * Return value: the space between columns
 *
 * Since: 2.6
 */
bint
btk_icon_view_get_column_spacing (BtkIconView *icon_view)
{
  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->column_spacing;
}

/**
 * btk_icon_view_set_margin:
 * @icon_view: a #BtkIconView
 * @margin: the margin
 * 
 * Sets the ::margin property which specifies the space 
 * which is inserted at the top, bottom, left and right 
 * of the icon view.
 *
 * Since: 2.6
 */
void 
btk_icon_view_set_margin (BtkIconView *icon_view,
			  bint         margin)
{
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));
  
  if (icon_view->priv->margin != margin)
    {
      icon_view->priv->margin = margin;

      btk_icon_view_stop_editing (icon_view, TRUE);
      btk_icon_view_invalidate_sizes (icon_view);
      btk_icon_view_queue_layout (icon_view);
      
      g_object_notify (B_OBJECT (icon_view), "margin");
    }  
}

/**
 * btk_icon_view_get_margin:
 * @icon_view: a #BtkIconView
 * 
 * Returns the value of the ::margin property.
 * 
 * Return value: the space at the borders 
 *
 * Since: 2.6
 */
bint
btk_icon_view_get_margin (BtkIconView *icon_view)
{
  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->margin;
}

/**
 * btk_icon_view_set_item_padding:
 * @icon_view: a #BtkIconView
 * @item_padding: the item padding
 *
 * Sets the #BtkIconView:item-padding property which specifies the padding
 * around each of the icon view's items.
 *
 * Since: 2.18
 */
void
btk_icon_view_set_item_padding (BtkIconView *icon_view,
				bint         item_padding)
{
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));
  
  if (icon_view->priv->item_padding != item_padding)
    {
      icon_view->priv->item_padding = item_padding;

      btk_icon_view_stop_editing (icon_view, TRUE);
      btk_icon_view_invalidate_sizes (icon_view);
      btk_icon_view_queue_layout (icon_view);
      
      g_object_notify (B_OBJECT (icon_view), "item-padding");
    }  
}

/**
 * btk_icon_view_get_item_padding:
 * @icon_view: a #BtkIconView
 * 
 * Returns the value of the ::item-padding property.
 * 
 * Return value: the padding around items
 *
 * Since: 2.18
 */
bint
btk_icon_view_get_item_padding (BtkIconView *icon_view)
{
  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), -1);

  return icon_view->priv->item_padding;
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
                     I_("btk-icon-view-status-pending"),
                     BINT_TO_POINTER (suggested_action));
}

static BdkDragAction
get_status_pending (BdkDragContext *context)
{
  return BPOINTER_TO_INT (g_object_get_data (B_OBJECT (context),
                                             "btk-icon-view-status-pending"));
}

static void
unset_reorderable (BtkIconView *icon_view)
{
  if (icon_view->priv->reorderable)
    {
      icon_view->priv->reorderable = FALSE;
      g_object_notify (B_OBJECT (icon_view), "reorderable");
    }
}

static void
set_source_row (BdkDragContext *context,
                BtkTreeModel   *model,
                BtkTreePath    *source_row)
{
  if (source_row)
    g_object_set_data_full (B_OBJECT (context),
			    I_("btk-icon-view-source-row"),
			    btk_tree_row_reference_new (model, source_row),
			    (GDestroyNotify) btk_tree_row_reference_free);
  else
    g_object_set_data_full (B_OBJECT (context),
			    I_("btk-icon-view-source-row"),
			    NULL, NULL);
}

static BtkTreePath*
get_source_row (BdkDragContext *context)
{
  BtkTreeRowReference *ref;

  ref = g_object_get_data (B_OBJECT (context), "btk-icon-view-source-row");

  if (ref)
    return btk_tree_row_reference_get_path (ref);
  else
    return NULL;
}

typedef struct
{
  BtkTreeRowReference *dest_row;
  bboolean             empty_view_drop;
  bboolean             drop_append_mode;
} DestRow;

static void
dest_row_free (bpointer data)
{
  DestRow *dr = (DestRow *)data;

  btk_tree_row_reference_free (dr->dest_row);
  g_free (dr);
}

static void
set_dest_row (BdkDragContext *context,
	      BtkTreeModel   *model,
	      BtkTreePath    *dest_row,
	      bboolean        empty_view_drop,
	      bboolean        drop_append_mode)
{
  DestRow *dr;

  if (!dest_row)
    {
      g_object_set_data_full (B_OBJECT (context),
			      I_("btk-icon-view-dest-row"),
			      NULL, NULL);
      return;
    }
  
  dr = g_new0 (DestRow, 1);
     
  dr->dest_row = btk_tree_row_reference_new (model, dest_row);
  dr->empty_view_drop = empty_view_drop;
  dr->drop_append_mode = drop_append_mode;
  g_object_set_data_full (B_OBJECT (context),
                          I_("btk-icon-view-dest-row"),
                          dr, (GDestroyNotify) dest_row_free);
}

static BtkTreePath*
get_dest_row (BdkDragContext *context)
{
  DestRow *dr;

  dr = g_object_get_data (B_OBJECT (context), "btk-icon-view-dest-row");

  if (dr)
    {
      BtkTreePath *path = NULL;
      
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

static bboolean
check_model_dnd (BtkTreeModel *model,
                 GType         required_iface,
                 const bchar  *signal)
{
  if (model == NULL || !B_TYPE_CHECK_INSTANCE_TYPE ((model), required_iface))
    {
      g_warning ("You must override the default '%s' handler "
                 "on BtkIconView when using models that don't support "
                 "the %s interface and enabling drag-and-drop. The simplest way to do this "
                 "is to connect to '%s' and call "
                 "g_signal_stop_emission_by_name() in your signal handler to prevent "
                 "the default handler from running. Look at the source code "
                 "for the default handler in btkiconview.c to get an idea what "
                 "your handler should do. (btkiconview.c is in the BTK+ source "
                 "code.) If you're using BTK+ from a language other than C, "
                 "there may be a more natural way to override default handlers, e.g. via derivation.",
                 signal, g_type_name (required_iface), signal);
      return FALSE;
    }
  else
    return TRUE;
}

static void
remove_scroll_timeout (BtkIconView *icon_view)
{
  if (icon_view->priv->scroll_timeout_id != 0)
    {
      g_source_remove (icon_view->priv->scroll_timeout_id);

      icon_view->priv->scroll_timeout_id = 0;
    }
}

static void
btk_icon_view_autoscroll (BtkIconView *icon_view)
{
  bint px, py, width, height;
  bint hoffset, voffset;
  bfloat value;

  bdk_window_get_pointer (BTK_WIDGET (icon_view)->window, &px, &py, NULL);
  bdk_window_get_geometry (BTK_WIDGET (icon_view)->window, NULL, NULL, &width, &height, NULL);

  /* see if we are near the edge. */
  voffset = py - 2 * SCROLL_EDGE_SIZE;
  if (voffset > 0)
    voffset = MAX (py - (height - 2 * SCROLL_EDGE_SIZE), 0);

  hoffset = px - 2 * SCROLL_EDGE_SIZE;
  if (hoffset > 0)
    hoffset = MAX (px - (width - 2 * SCROLL_EDGE_SIZE), 0);

  if (voffset != 0)
    {
      value = CLAMP (icon_view->priv->vadjustment->value + voffset, 
		     icon_view->priv->vadjustment->lower,
		     icon_view->priv->vadjustment->upper - icon_view->priv->vadjustment->page_size);
      btk_adjustment_set_value (icon_view->priv->vadjustment, value);
    }
  if (hoffset != 0)
    {
      value = CLAMP (icon_view->priv->hadjustment->value + hoffset, 
		     icon_view->priv->hadjustment->lower,
		     icon_view->priv->hadjustment->upper - icon_view->priv->hadjustment->page_size);
      btk_adjustment_set_value (icon_view->priv->hadjustment, value);
    }
}


static bboolean
drag_scroll_timeout (bpointer data)
{
  BtkIconView *icon_view = BTK_ICON_VIEW (data);

  btk_icon_view_autoscroll (icon_view);

  return TRUE;
}


static bboolean
set_destination (BtkIconView    *icon_view,
		 BdkDragContext *context,
		 bint            x,
		 bint            y,
		 BdkDragAction  *suggested_action,
		 BdkAtom        *target)
{
  BtkWidget *widget;
  BtkTreePath *path = NULL;
  BtkIconViewDropPosition pos;
  BtkIconViewDropPosition old_pos;
  BtkTreePath *old_dest_path = NULL;
  bboolean can_drop = FALSE;

  widget = BTK_WIDGET (icon_view);

  *suggested_action = 0;
  *target = BDK_NONE;

  if (!icon_view->priv->dest_set)
    {
      /* someone unset us as a drag dest, note that if
       * we return FALSE drag_leave isn't called
       */

      btk_icon_view_set_drag_dest_item (icon_view,
					NULL,
					BTK_ICON_VIEW_DROP_LEFT);

      remove_scroll_timeout (BTK_ICON_VIEW (widget));

      return FALSE; /* no longer a drop site */
    }

  *target = btk_drag_dest_find_target (widget, context,
                                       btk_drag_dest_get_target_list (widget));
  if (*target == BDK_NONE)
    return FALSE;

  if (!btk_icon_view_get_dest_item_at_pos (icon_view, x, y, &path, &pos)) 
    {
      bint n_children;
      BtkTreeModel *model;
      
      /* the row got dropped on empty space, let's setup a special case
       */

      if (path)
	btk_tree_path_free (path);

      model = btk_icon_view_get_model (icon_view);

      n_children = btk_tree_model_iter_n_children (model, NULL);
      if (n_children)
        {
          pos = BTK_ICON_VIEW_DROP_BELOW;
          path = btk_tree_path_new_from_indices (n_children - 1, -1);
        }
      else
        {
          pos = BTK_ICON_VIEW_DROP_ABOVE;
          path = btk_tree_path_new_from_indices (0, -1);
        }

      can_drop = TRUE;

      goto out;
    }

  g_assert (path);

  btk_icon_view_get_drag_dest_item (icon_view,
				    &old_dest_path,
				    &old_pos);
  
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

      btk_icon_view_set_drag_dest_item (BTK_ICON_VIEW (widget),
					path, pos);
    }
  else
    {
      /* can't drop here */
      btk_icon_view_set_drag_dest_item (BTK_ICON_VIEW (widget),
					NULL,
					BTK_ICON_VIEW_DROP_LEFT);
    }
  
  if (path)
    btk_tree_path_free (path);
  
  return TRUE;
}

static BtkTreePath*
get_logical_destination (BtkIconView *icon_view,
			 bboolean    *drop_append_mode)
{
  /* adjust path to point to the row the drop goes in front of */
  BtkTreePath *path = NULL;
  BtkIconViewDropPosition pos;
  
  *drop_append_mode = FALSE;

  btk_icon_view_get_drag_dest_item (icon_view, &path, &pos);

  if (path == NULL)
    return NULL;

  if (pos == BTK_ICON_VIEW_DROP_RIGHT || 
      pos == BTK_ICON_VIEW_DROP_BELOW)
    {
      BtkTreeIter iter;
      BtkTreeModel *model = icon_view->priv->model;

      if (!btk_tree_model_get_iter (model, &iter, path) ||
          !btk_tree_model_iter_next (model, &iter))
        *drop_append_mode = TRUE;
      else
        {
          *drop_append_mode = FALSE;
          btk_tree_path_next (path);
        }      
    }

  return path;
}

static bboolean
btk_icon_view_maybe_begin_drag (BtkIconView    *icon_view,
				BdkEventMotion *event)
{
  BtkWidget *widget = BTK_WIDGET (icon_view);
  BdkDragContext *context;
  BtkTreePath *path = NULL;
  bint button;
  BtkTreeModel *model;
  bboolean retval = FALSE;

  if (!icon_view->priv->source_set)
    goto out;

  if (icon_view->priv->pressed_button < 0)
    goto out;

  if (!btk_drag_check_threshold (BTK_WIDGET (icon_view),
                                 icon_view->priv->press_start_x,
                                 icon_view->priv->press_start_y,
                                 event->x, event->y))
    goto out;

  model = btk_icon_view_get_model (icon_view);

  if (model == NULL)
    goto out;

  button = icon_view->priv->pressed_button;
  icon_view->priv->pressed_button = -1;

  path = btk_icon_view_get_path_at_pos (icon_view,
					icon_view->priv->press_start_x,
					icon_view->priv->press_start_y);

  if (path == NULL)
    goto out;

  if (!BTK_IS_TREE_DRAG_SOURCE (model) ||
      !btk_tree_drag_source_row_draggable (BTK_TREE_DRAG_SOURCE (model),
					   path))
    goto out;

  /* FIXME Check whether we're a start button, if not return FALSE and
   * free path
   */

  /* Now we can begin the drag */
  
  retval = TRUE;

  context = btk_drag_begin (widget,
                            btk_drag_source_get_target_list (widget),
                            icon_view->priv->source_actions,
                            button,
                            (BdkEvent*)event);

  set_source_row (context, model, path);
  
 out:
  if (path)
    btk_tree_path_free (path);

  return retval;
}

/* Source side drag signals */
static void 
btk_icon_view_drag_begin (BtkWidget      *widget,
			  BdkDragContext *context)
{
  BtkIconView *icon_view;
  BtkIconViewItem *item;
  BdkPixmap *icon;
  bint x, y;
  BtkTreePath *path;

  icon_view = BTK_ICON_VIEW (widget);

  /* if the user uses a custom DnD impl, we don't set the icon here */
  if (!icon_view->priv->dest_set && !icon_view->priv->source_set)
    return;

  item = btk_icon_view_get_item_at_coords (icon_view,
					   icon_view->priv->press_start_x,
					   icon_view->priv->press_start_y,
					   TRUE,
					   NULL);

  g_return_if_fail (item != NULL);

  x = icon_view->priv->press_start_x - item->x + 1;
  y = icon_view->priv->press_start_y - item->y + 1;
  
  path = btk_tree_path_new_from_indices (item->index, -1);
  icon = btk_icon_view_create_drag_icon (icon_view, path);
  btk_tree_path_free (path);

  btk_drag_set_icon_pixmap (context, 
			    bdk_drawable_get_colormap (icon),
			    icon, 
			    NULL, 
			    x, y);

  g_object_unref (icon);
}

static void 
btk_icon_view_drag_end (BtkWidget      *widget,
			BdkDragContext *context)
{
  /* do nothing */
}

static void 
btk_icon_view_drag_data_get (BtkWidget        *widget,
			     BdkDragContext   *context,
			     BtkSelectionData *selection_data,
			     buint             info,
			     buint             time)
{
  BtkIconView *icon_view;
  BtkTreeModel *model;
  BtkTreePath *source_row;

  icon_view = BTK_ICON_VIEW (widget);
  model = btk_icon_view_get_model (icon_view);

  if (model == NULL)
    return;

  if (!icon_view->priv->source_set)
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
    btk_tree_set_row_drag_data (selection_data,
				model,
				source_row);

 done:
  btk_tree_path_free (source_row);
}

static void 
btk_icon_view_drag_data_delete (BtkWidget      *widget,
				BdkDragContext *context)
{
  BtkTreeModel *model;
  BtkIconView *icon_view;
  BtkTreePath *source_row;

  icon_view = BTK_ICON_VIEW (widget);
  model = btk_icon_view_get_model (icon_view);

  if (!check_model_dnd (model, BTK_TYPE_TREE_DRAG_SOURCE, "drag-data-delete"))
    return;

  if (!icon_view->priv->source_set)
    return;

  source_row = get_source_row (context);

  if (source_row == NULL)
    return;

  btk_tree_drag_source_drag_data_delete (BTK_TREE_DRAG_SOURCE (model),
                                         source_row);

  btk_tree_path_free (source_row);

  set_source_row (context, NULL, NULL);
}

/* Target side drag signals */
static void
btk_icon_view_drag_leave (BtkWidget      *widget,
			  BdkDragContext *context,
			  buint           time)
{
  BtkIconView *icon_view;

  icon_view = BTK_ICON_VIEW (widget);

  /* unset any highlight row */
  btk_icon_view_set_drag_dest_item (icon_view,
				    NULL,
				    BTK_ICON_VIEW_DROP_LEFT);

  remove_scroll_timeout (icon_view);
}

static bboolean 
btk_icon_view_drag_motion (BtkWidget      *widget,
			   BdkDragContext *context,
			   bint            x,
			   bint            y,
			   buint           time)
{
  BtkTreePath *path = NULL;
  BtkIconViewDropPosition pos;
  BtkIconView *icon_view;
  BdkDragAction suggested_action = 0;
  BdkAtom target;
  bboolean empty;

  icon_view = BTK_ICON_VIEW (widget);

  if (!set_destination (icon_view, context, x, y, &suggested_action, &target))
    return FALSE;

  btk_icon_view_get_drag_dest_item (icon_view, &path, &pos);

  /* we only know this *after* set_desination_row */
  empty = icon_view->priv->empty_view_drop;

  if (path == NULL && !empty)
    {
      /* Can't drop here. */
      bdk_drag_status (context, 0, time);
    }
  else
    {
      if (icon_view->priv->scroll_timeout_id == 0)
	{
	  icon_view->priv->scroll_timeout_id =
	    bdk_threads_add_timeout (50, drag_scroll_timeout, icon_view);
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

static bboolean 
btk_icon_view_drag_drop (BtkWidget      *widget,
			 BdkDragContext *context,
			 bint            x,
			 bint            y,
			 buint           time)
{
  BtkIconView *icon_view;
  BtkTreePath *path;
  BdkDragAction suggested_action = 0;
  BdkAtom target = BDK_NONE;
  BtkTreeModel *model;
  bboolean drop_append_mode;

  icon_view = BTK_ICON_VIEW (widget);
  model = btk_icon_view_get_model (icon_view);

  remove_scroll_timeout (BTK_ICON_VIEW (widget));

  if (!icon_view->priv->dest_set)
    return FALSE;

  if (!check_model_dnd (model, BTK_TYPE_TREE_DRAG_DEST, "drag-drop"))
    return FALSE;

  if (!set_destination (icon_view, context, x, y, &suggested_action, &target))
    return FALSE;
  
  path = get_logical_destination (icon_view, &drop_append_mode);

  if (target != BDK_NONE && path != NULL)
    {
      /* in case a motion had requested drag data, change things so we
       * treat drag data receives as a drop.
       */
      set_status_pending (context, 0);
      set_dest_row (context, model, path, 
		    icon_view->priv->empty_view_drop, drop_append_mode);
    }

  if (path)
    btk_tree_path_free (path);

  /* Unset this thing */
  btk_icon_view_set_drag_dest_item (icon_view, NULL, BTK_ICON_VIEW_DROP_LEFT);

  if (target != BDK_NONE)
    {
      btk_drag_get_data (widget, context, target, time);
      return TRUE;
    }
  else
    return FALSE;
}

static void
btk_icon_view_drag_data_received (BtkWidget        *widget,
				  BdkDragContext   *context,
				  bint              x,
				  bint              y,
				  BtkSelectionData *selection_data,
				  buint             info,
				  buint             time)
{
  BtkTreePath *path;
  bboolean accepted = FALSE;
  BtkTreeModel *model;
  BtkIconView *icon_view;
  BtkTreePath *dest_row;
  BdkDragAction suggested_action;
  bboolean drop_append_mode;
  
  icon_view = BTK_ICON_VIEW (widget);  
  model = btk_icon_view_get_model (icon_view);

  if (!check_model_dnd (model, BTK_TYPE_TREE_DRAG_DEST, "drag-data-received"))
    return;

  if (!icon_view->priv->dest_set)
    return;

  suggested_action = get_status_pending (context);

  if (suggested_action)
    {
      /* We are getting this data due to a request in drag_motion,
       * rather than due to a request in drag_drop, so we are just
       * supposed to call drag_status, not actually paste in the
       * data.
       */
      path = get_logical_destination (icon_view, &drop_append_mode);

      if (path == NULL)
        suggested_action = 0;

      if (suggested_action)
        {
	  if (!btk_tree_drag_dest_row_drop_possible (BTK_TREE_DRAG_DEST (model),
						     path,
						     selection_data))
	    suggested_action = 0;
        }

      bdk_drag_status (context, suggested_action, time);

      if (path)
        btk_tree_path_free (path);

      /* If you can't drop, remove user drop indicator until the next motion */
      if (suggested_action == 0)
        btk_icon_view_set_drag_dest_item (icon_view,
					  NULL,
					  BTK_ICON_VIEW_DROP_LEFT);
      return;
    }
  

  dest_row = get_dest_row (context);

  if (dest_row == NULL)
    return;

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

  btk_tree_path_free (dest_row);

  /* drop dest_row */
  set_dest_row (context, NULL, NULL, FALSE, FALSE);
}

/* Drag-and-Drop support */
/**
 * btk_icon_view_enable_model_drag_source:
 * @icon_view: a #BtkIconTreeView
 * @start_button_mask: Mask of allowed buttons to start drag
 * @targets: the table of targets that the drag will support
 * @n_targets: the number of items in @targets
 * @actions: the bitmask of possible actions for a drag from this
 *    widget
 *
 * Turns @icon_view into a drag source for automatic DND. Calling this
 * method sets #BtkIconView:reorderable to %FALSE.
 *
 * Since: 2.8
 **/
void
btk_icon_view_enable_model_drag_source (BtkIconView              *icon_view,
					BdkModifierType           start_button_mask,
					const BtkTargetEntry     *targets,
					bint                      n_targets,
					BdkDragAction             actions)
{
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));

  btk_drag_source_set (BTK_WIDGET (icon_view), 0, targets, n_targets, actions);

  icon_view->priv->start_button_mask = start_button_mask;
  icon_view->priv->source_actions = actions;

  icon_view->priv->source_set = TRUE;

  unset_reorderable (icon_view);
}

/**
 * btk_icon_view_enable_model_drag_dest:
 * @icon_view: a #BtkIconView
 * @targets: the table of targets that the drag will support
 * @n_targets: the number of items in @targets
 * @actions: the bitmask of possible actions for a drag to this
 *    widget
 *
 * Turns @icon_view into a drop destination for automatic DND. Calling this
 * method sets #BtkIconView:reorderable to %FALSE.
 *
 * Since: 2.8
 **/
void 
btk_icon_view_enable_model_drag_dest (BtkIconView          *icon_view,
				      const BtkTargetEntry *targets,
				      bint                  n_targets,
				      BdkDragAction         actions)
{
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));

  btk_drag_dest_set (BTK_WIDGET (icon_view), 0, targets, n_targets, actions);

  icon_view->priv->dest_actions = actions;

  icon_view->priv->dest_set = TRUE;

  unset_reorderable (icon_view);  
}

/**
 * btk_icon_view_unset_model_drag_source:
 * @icon_view: a #BtkIconView
 * 
 * Undoes the effect of btk_icon_view_enable_model_drag_source(). Calling this
 * method sets #BtkIconView:reorderable to %FALSE.
 *
 * Since: 2.8
 **/
void
btk_icon_view_unset_model_drag_source (BtkIconView *icon_view)
{
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->source_set)
    {
      btk_drag_source_unset (BTK_WIDGET (icon_view));
      icon_view->priv->source_set = FALSE;
    }

  unset_reorderable (icon_view);
}

/**
 * btk_icon_view_unset_model_drag_dest:
 * @icon_view: a #BtkIconView
 * 
 * Undoes the effect of btk_icon_view_enable_model_drag_dest(). Calling this
 * method sets #BtkIconView:reorderable to %FALSE.
 *
 * Since: 2.8
 **/
void
btk_icon_view_unset_model_drag_dest (BtkIconView *icon_view)
{
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->dest_set)
    {
      btk_drag_dest_unset (BTK_WIDGET (icon_view));
      icon_view->priv->dest_set = FALSE;
    }

  unset_reorderable (icon_view);
}

/* These are useful to implement your own custom stuff. */
/**
 * btk_icon_view_set_drag_dest_item:
 * @icon_view: a #BtkIconView
 * @path: (allow-none): The path of the item to highlight, or %NULL.
 * @pos: Specifies where to drop, relative to the item
 *
 * Sets the item that is highlighted for feedback.
 *
 * Since: 2.8
 */
void
btk_icon_view_set_drag_dest_item (BtkIconView              *icon_view,
				  BtkTreePath              *path,
				  BtkIconViewDropPosition   pos)
{
  /* Note; this function is exported to allow a custom DND
   * implementation, so it can't touch TreeViewDragInfo
   */

  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));

  if (icon_view->priv->dest_item)
    {
      BtkTreePath *current_path;
      current_path = btk_tree_row_reference_get_path (icon_view->priv->dest_item);
      btk_tree_row_reference_free (icon_view->priv->dest_item);
      icon_view->priv->dest_item = NULL;      

      btk_icon_view_queue_draw_path (icon_view, current_path);
      btk_tree_path_free (current_path);
    }
  
  /* special case a drop on an empty model */
  icon_view->priv->empty_view_drop = FALSE;
  if (pos == BTK_ICON_VIEW_DROP_ABOVE && path
      && btk_tree_path_get_depth (path) == 1
      && btk_tree_path_get_indices (path)[0] == 0)
    {
      bint n_children;

      n_children = btk_tree_model_iter_n_children (icon_view->priv->model,
                                                   NULL);

      if (n_children == 0)
        icon_view->priv->empty_view_drop = TRUE;
    }

  icon_view->priv->dest_pos = pos;

  if (path)
    {
      icon_view->priv->dest_item =
        btk_tree_row_reference_new_proxy (B_OBJECT (icon_view), 
					  icon_view->priv->model, path);
      
      btk_icon_view_queue_draw_path (icon_view, path);
    }
}

/**
 * btk_icon_view_get_drag_dest_item:
 * @icon_view: a #BtkIconView
 * @path: (allow-none): Return location for the path of the highlighted item, or %NULL.
 * @pos: (allow-none): Return location for the drop position, or %NULL
 * 
 * Gets information about the item that is highlighted for feedback.
 *
 * Since: 2.8
 **/
void
btk_icon_view_get_drag_dest_item (BtkIconView              *icon_view,
				  BtkTreePath             **path,
				  BtkIconViewDropPosition  *pos)
{
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));

  if (path)
    {
      if (icon_view->priv->dest_item)
        *path = btk_tree_row_reference_get_path (icon_view->priv->dest_item);
      else
	*path = NULL;
    }

  if (pos)
    *pos = icon_view->priv->dest_pos;
}

/**
 * btk_icon_view_get_dest_item_at_pos:
 * @icon_view: a #BtkIconView
 * @drag_x: the position to determine the destination item for
 * @drag_y: the position to determine the destination item for
 * @path: (allow-none): Return location for the path of the item, or %NULL.
 * @pos: (allow-none): Return location for the drop position, or %NULL
 * 
 * Determines the destination item for a given position.
 * 
 * Return value: whether there is an item at the given position.
 *
 * Since: 2.8
 **/
bboolean
btk_icon_view_get_dest_item_at_pos (BtkIconView              *icon_view,
				    bint                      drag_x,
				    bint                      drag_y,
				    BtkTreePath             **path,
				    BtkIconViewDropPosition  *pos)
{
  BtkIconViewItem *item;

  /* Note; this function is exported to allow a custom DND
   * implementation, so it can't touch TreeViewDragInfo
   */

  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), FALSE);
  g_return_val_if_fail (drag_x >= 0, FALSE);
  g_return_val_if_fail (drag_y >= 0, FALSE);
  g_return_val_if_fail (icon_view->priv->bin_window != NULL, FALSE);


  if (path)
    *path = NULL;

  item = btk_icon_view_get_item_at_coords (icon_view, 
					   drag_x + icon_view->priv->hadjustment->value, 
					   drag_y + icon_view->priv->vadjustment->value,
					   FALSE, NULL);

  if (item == NULL)
    return FALSE;

  if (path)
    *path = btk_tree_path_new_from_indices (item->index, -1);

  if (pos)
    {
      if (drag_x < item->x + item->width / 4)
	*pos = BTK_ICON_VIEW_DROP_LEFT;
      else if (drag_x > item->x + item->width * 3 / 4)
	*pos = BTK_ICON_VIEW_DROP_RIGHT;
      else if (drag_y < item->y + item->height / 4)
	*pos = BTK_ICON_VIEW_DROP_ABOVE;
      else if (drag_y > item->y + item->height * 3 / 4)
	*pos = BTK_ICON_VIEW_DROP_BELOW;
      else
	*pos = BTK_ICON_VIEW_DROP_INTO;
    }

  return TRUE;
}

/**
 * btk_icon_view_create_drag_icon:
 * @icon_view: a #BtkIconView
 * @path: a #BtkTreePath in @icon_view
 *
 * Creates a #BdkPixmap representation of the item at @path.
 * This image is used for a drag icon.
 *
 * Return value: (transfer full): a newly-allocated pixmap of the drag icon.
 * 
 * Since: 2.8
 **/
BdkPixmap *
btk_icon_view_create_drag_icon (BtkIconView *icon_view,
				BtkTreePath *path)
{
  BtkWidget *widget;
  bairo_t *cr;
  BdkPixmap *drawable;
  GList *l;
  bint index;
  BdkRectangle area;

  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), NULL);
  g_return_val_if_fail (path != NULL, NULL);

  widget = BTK_WIDGET (icon_view);

  if (!btk_widget_get_realized (widget))
    return NULL;

  index = btk_tree_path_get_indices (path)[0];

  for (l = icon_view->priv->items; l; l = l->next) 
    {
      BtkIconViewItem *item = l->data;
      
      if (index == item->index)
	{
	  drawable = bdk_pixmap_new (icon_view->priv->bin_window,
				     item->width + 2,
				     item->height + 2,
				     -1);

	  cr = bdk_bairo_create (drawable);
	  bairo_set_line_width (cr, 1.);

	  bdk_bairo_set_source_color
	    (cr, &widget->style->base[btk_widget_get_state (widget)]);
	  bairo_rectangle (cr, 0, 0, item->width + 2, item->height + 2);
	  bairo_fill (cr);

	  area.x = 0;
	  area.y = 0;
	  area.width = item->width;
	  area.height = item->height;

	  btk_icon_view_paint_item (icon_view, cr, item, &area, 
 				    drawable, 1, 1, FALSE); 

	  bairo_set_source_rgb (cr, 0.0, 0.0, 0.0); /* black */
	  bairo_rectangle (cr, 0.5, 0.5, item->width + 1, item->height + 1);
	  bairo_stroke (cr);

	  bairo_destroy (cr);

	  return drawable;
	}
    }
  
  return NULL;
}

/**
 * btk_icon_view_get_reorderable:
 * @icon_view: a #BtkIconView
 *
 * Retrieves whether the user can reorder the list via drag-and-drop. 
 * See btk_icon_view_set_reorderable().
 *
 * Return value: %TRUE if the list can be reordered.
 *
 * Since: 2.8
 **/
bboolean
btk_icon_view_get_reorderable (BtkIconView *icon_view)
{
  g_return_val_if_fail (BTK_IS_ICON_VIEW (icon_view), FALSE);

  return icon_view->priv->reorderable;
}

static const BtkTargetEntry item_targets[] = {
  { "BTK_TREE_MODEL_ROW", BTK_TARGET_SAME_WIDGET, 0 }
};


/**
 * btk_icon_view_set_reorderable:
 * @icon_view: A #BtkIconView.
 * @reorderable: %TRUE, if the list of items can be reordered.
 *
 * This function is a convenience function to allow you to reorder models that
 * support the #BtkTreeDragSourceIface and the #BtkTreeDragDestIface.  Both
 * #BtkTreeStore and #BtkListStore support these.  If @reorderable is %TRUE, then
 * the user can reorder the model by dragging and dropping rows.  The
 * developer can listen to these changes by connecting to the model's
 * row_inserted and row_deleted signals. The reordering is implemented by setting up
 * the icon view as a drag source and destination. Therefore, drag and
 * drop can not be used in a reorderable view for any other purpose.
 *
 * This function does not give you any degree of control over the order -- any
 * reordering is allowed.  If more control is needed, you should probably
 * handle drag and drop manually.
 *
 * Since: 2.8
 **/
void
btk_icon_view_set_reorderable (BtkIconView *icon_view,
			       bboolean     reorderable)
{
  g_return_if_fail (BTK_IS_ICON_VIEW (icon_view));

  reorderable = reorderable != FALSE;

  if (icon_view->priv->reorderable == reorderable)
    return;

  if (reorderable)
    {
      btk_icon_view_enable_model_drag_source (icon_view,
					      BDK_BUTTON1_MASK,
					      item_targets,
					      G_N_ELEMENTS (item_targets),
					      BDK_ACTION_MOVE);
      btk_icon_view_enable_model_drag_dest (icon_view,
					    item_targets,
					    G_N_ELEMENTS (item_targets),
					    BDK_ACTION_MOVE);
    }
  else
    {
      btk_icon_view_unset_model_drag_source (icon_view);
      btk_icon_view_unset_model_drag_dest (icon_view);
    }

  icon_view->priv->reorderable = reorderable;

  g_object_notify (B_OBJECT (icon_view), "reorderable");
}


/* Accessibility Support */

static bpointer accessible_parent_class;
static bpointer accessible_item_parent_class;
static GQuark accessible_private_data_quark = 0;

#define BTK_TYPE_ICON_VIEW_ITEM_ACCESSIBLE      (btk_icon_view_item_accessible_get_type ())
#define BTK_ICON_VIEW_ITEM_ACCESSIBLE(obj)      (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_ICON_VIEW_ITEM_ACCESSIBLE, BtkIconViewItemAccessible))
#define BTK_IS_ICON_VIEW_ITEM_ACCESSIBLE(obj)   (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_ICON_VIEW_ITEM_ACCESSIBLE))

static GType btk_icon_view_item_accessible_get_type (void);

enum {
    ACTION_ACTIVATE,
    LAST_ACTION
};

typedef struct
{
  BatkObject parent;

  BtkIconViewItem *item;

  BtkWidget *widget;

  BatkStateSet *state_set;

  bchar *text;

  BtkTextBuffer *text_buffer;

  bchar *action_descriptions[LAST_ACTION];
  bchar *image_description;
  buint action_idle_handler;
} BtkIconViewItemAccessible;

static const bchar *const btk_icon_view_item_accessible_action_names[] = 
{
  "activate",
  NULL
};

static const bchar *const btk_icon_view_item_accessible_action_descriptions[] =
{
  "Activate item",
  NULL
};
typedef struct _BtkIconViewItemAccessibleClass
{
  BatkObjectClass parent_class;

} BtkIconViewItemAccessibleClass;

static bboolean btk_icon_view_item_accessible_is_showing (BtkIconViewItemAccessible *item);

static bboolean
btk_icon_view_item_accessible_idle_do_action (bpointer data)
{
  BtkIconViewItemAccessible *item;
  BtkIconView *icon_view;
  BtkTreePath *path;

  item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (data);
  item->action_idle_handler = 0;

  if (item->widget != NULL)
    {
      icon_view = BTK_ICON_VIEW (item->widget);
      path = btk_tree_path_new_from_indices (item->item->index, -1);
      btk_icon_view_item_activated (icon_view, path);
      btk_tree_path_free (path);
    }

  return FALSE;
}

static bboolean
btk_icon_view_item_accessible_action_do_action (BatkAction *action,
                                                bint       i)
{
  BtkIconViewItemAccessible *item;

  if (i < 0 || i >= LAST_ACTION) 
    return FALSE;

  item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (action);

  if (!BTK_IS_ICON_VIEW (item->widget))
    return FALSE;

  if (batk_state_set_contains_state (item->state_set, BATK_STATE_DEFUNCT))
    return FALSE;

  switch (i)
    {
    case ACTION_ACTIVATE:
      if (!item->action_idle_handler)
        item->action_idle_handler = bdk_threads_add_idle (btk_icon_view_item_accessible_idle_do_action, item);
      break;
    default:
      g_assert_not_reached ();
      return FALSE;

    }        
  return TRUE;
}

static bint
btk_icon_view_item_accessible_action_get_n_actions (BatkAction *action)
{
        return LAST_ACTION;
}

static const bchar *
btk_icon_view_item_accessible_action_get_description (BatkAction *action,
                                                      bint       i)
{
  BtkIconViewItemAccessible *item;

  if (i < 0 || i >= LAST_ACTION) 
    return NULL;

  item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (action);

  if (item->action_descriptions[i])
    return item->action_descriptions[i];
  else
    return btk_icon_view_item_accessible_action_descriptions[i];
}

static const bchar *
btk_icon_view_item_accessible_action_get_name (BatkAction *action,
                                               bint       i)
{
  if (i < 0 || i >= LAST_ACTION) 
    return NULL;

  return btk_icon_view_item_accessible_action_names[i];
}

static bboolean
btk_icon_view_item_accessible_action_set_description (BatkAction   *action,
                                                      bint         i,
                                                      const bchar *description)
{
  BtkIconViewItemAccessible *item;

  if (i < 0 || i >= LAST_ACTION) 
    return FALSE;

  item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (action);

  g_free (item->action_descriptions[i]);

  item->action_descriptions[i] = g_strdup (description);

  return TRUE;
}

static void
batk_action_item_interface_init (BatkActionIface *iface)
{
  iface->do_action = btk_icon_view_item_accessible_action_do_action;
  iface->get_n_actions = btk_icon_view_item_accessible_action_get_n_actions;
  iface->get_description = btk_icon_view_item_accessible_action_get_description;
  iface->get_name = btk_icon_view_item_accessible_action_get_name;
  iface->set_description = btk_icon_view_item_accessible_action_set_description;
}

static const bchar *
btk_icon_view_item_accessible_image_get_image_description (BatkImage *image)
{
  BtkIconViewItemAccessible *item;

  item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (image);

  return item->image_description;
}

static bboolean
btk_icon_view_item_accessible_image_set_image_description (BatkImage    *image,
                                                           const bchar *description)
{
  BtkIconViewItemAccessible *item;

  item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (image);

  g_free (item->image_description);
  item->image_description = g_strdup (description);

  return TRUE;
}

static bboolean
get_pixbuf_box (BtkIconView     *icon_view,
		BtkIconViewItem *item,
		BdkRectangle    *box)
{
  GList *l;

  for (l = icon_view->priv->cell_list; l; l = l->next)
    {
      BtkIconViewCellInfo *info = l->data;
      
      if (BTK_IS_CELL_RENDERER_PIXBUF (info->cell))
	{
	  btk_icon_view_get_cell_box (icon_view, item, info, box);

	  return TRUE;
	}
    }

  return FALSE;
}

static bchar *
get_text (BtkIconView     *icon_view,
	  BtkIconViewItem *item)
{
  GList *l;
  bchar *text;

  for (l = icon_view->priv->cell_list; l; l = l->next)
    {
      BtkIconViewCellInfo *info = l->data;
      
      if (BTK_IS_CELL_RENDERER_TEXT (info->cell))
	{
	  g_object_get (info->cell, "text", &text, NULL);
	  
	  return text;
	}
    }

  return NULL;
}

static void
btk_icon_view_item_accessible_image_get_image_size (BatkImage *image,
                                                    bint     *width,
                                                    bint     *height)
{
  BtkIconViewItemAccessible *item;
  BdkRectangle box;

  item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (image);

  if (!BTK_IS_ICON_VIEW (item->widget))
    return;

  if (batk_state_set_contains_state (item->state_set, BATK_STATE_DEFUNCT))
    return;

  if (get_pixbuf_box (BTK_ICON_VIEW (item->widget), item->item, &box))
    {
      *width = box.width;
      *height = box.height;  
    }
}

static void
btk_icon_view_item_accessible_image_get_image_position (BatkImage    *image,
                                                        bint        *x,
                                                        bint        *y,
                                                        BatkCoordType coord_type)
{
  BtkIconViewItemAccessible *item;
  BdkRectangle box;

  item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (image);

  if (!BTK_IS_ICON_VIEW (item->widget))
    return;

  if (batk_state_set_contains_state (item->state_set, BATK_STATE_DEFUNCT))
    return;

  batk_component_get_position (BATK_COMPONENT (image), x, y, coord_type);

  if (get_pixbuf_box (BTK_ICON_VIEW (item->widget), item->item, &box))
    {
      *x+= box.x - item->item->x;
      *y+= box.y - item->item->y;
    }

}

static void
batk_image_item_interface_init (BatkImageIface *iface)
{
  iface->get_image_description = btk_icon_view_item_accessible_image_get_image_description;
  iface->set_image_description = btk_icon_view_item_accessible_image_set_image_description;
  iface->get_image_size = btk_icon_view_item_accessible_image_get_image_size;
  iface->get_image_position = btk_icon_view_item_accessible_image_get_image_position;
}

static bchar *
btk_icon_view_item_accessible_text_get_text (BatkText *text,
                                             bint     start_pos,
                                             bint     end_pos)
{
  BtkIconViewItemAccessible *item;
  BtkTextIter start, end;
  BtkTextBuffer *buffer;

  item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!BTK_IS_ICON_VIEW (item->widget))
    return NULL;

  if (batk_state_set_contains_state (item->state_set, BATK_STATE_DEFUNCT))
    return NULL;

  buffer = item->text_buffer;
  btk_text_buffer_get_iter_at_offset (buffer, &start, start_pos);
  if (end_pos < 0)
    btk_text_buffer_get_end_iter (buffer, &end);
  else
    btk_text_buffer_get_iter_at_offset (buffer, &end, end_pos);

  return btk_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static gunichar
btk_icon_view_item_accessible_text_get_character_at_offset (BatkText *text,
                                                            bint     offset)
{
  BtkIconViewItemAccessible *item;
  BtkTextIter start, end;
  BtkTextBuffer *buffer;
  bchar *string;
  gunichar unichar;

  item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!BTK_IS_ICON_VIEW (item->widget))
    return '\0';

  if (batk_state_set_contains_state (item->state_set, BATK_STATE_DEFUNCT))
    return '\0';

  buffer = item->text_buffer;
  if (offset >= btk_text_buffer_get_char_count (buffer))
    return '\0';

  btk_text_buffer_get_iter_at_offset (buffer, &start, offset);
  end = start;
  btk_text_iter_forward_char (&end);
  string = btk_text_buffer_get_slice (buffer, &start, &end, FALSE);
  unichar = g_utf8_get_char (string);
  g_free(string);

  return unichar;
}

#if 0
static void
get_bango_text_offsets (BangoLayout     *layout,
                        BtkTextBuffer   *buffer,
                        bint             function,
                        BatkTextBoundary  boundary_type,
                        bint             offset,
                        bint            *start_offset,
                        bint            *end_offset,
                        BtkTextIter     *start_iter,
                        BtkTextIter     *end_iter)
{
  BangoLayoutIter *iter;
  BangoLayoutLine *line, *prev_line = NULL, *prev_prev_line = NULL;
  bint index, start_index, end_index;
  const bchar *text;
  bboolean found = FALSE;

  text = bango_layout_get_text (layout);
  index = g_utf8_offset_to_pointer (text, offset) - text;
  iter = bango_layout_get_iter (layout);
  do
    {
      line = bango_layout_iter_get_line_readonly (iter);
      start_index = line->start_index;
      end_index = start_index + line->length;

      if (index >= start_index && index <= end_index)
        {
          /*
           * Found line for offset
           */
          switch (function)
            {
            case 0:
                  /*
                   * We want the previous line
                   */
              if (prev_line)
                {
                  switch (boundary_type)
                    {
                    case BATK_TEXT_BOUNDARY_LINE_START:
                      end_index = start_index;
                      start_index = prev_line->start_index;
                      break;
                    case BATK_TEXT_BOUNDARY_LINE_END:
                      if (prev_prev_line)
                        start_index = prev_prev_line->start_index + 
                                  prev_prev_line->length;
                      end_index = prev_line->start_index + prev_line->length;
                      break;
                    default:
                      g_assert_not_reached();
                    }
                }
              else
                start_index = end_index = 0;
              break;
            case 1:
              switch (boundary_type)
                {
                case BATK_TEXT_BOUNDARY_LINE_START:
                  if (bango_layout_iter_next_line (iter))
                    end_index = bango_layout_iter_get_line_readonly (iter)->start_index;
                  break;
                case BATK_TEXT_BOUNDARY_LINE_END:
                  if (prev_line)
                    start_index = prev_line->start_index + 
                                  prev_line->length;
                  break;
                default:
                  g_assert_not_reached();
                }
              break;
            case 2:
               /*
                * We want the next line
                */
              if (bango_layout_iter_next_line (iter))
                {
                  line = bango_layout_iter_get_line_readonly (iter);
                  switch (boundary_type)
                    {
                    case BATK_TEXT_BOUNDARY_LINE_START:
                      start_index = line->start_index;
                      if (bango_layout_iter_next_line (iter))
                        end_index = bango_layout_iter_get_line_readonly (iter)->start_index;
                      else
                        end_index = start_index + line->length;
                      break;
                    case BATK_TEXT_BOUNDARY_LINE_END:
                      start_index = end_index;
                      end_index = line->start_index + line->length;
                      break;
                    default:
                      g_assert_not_reached();
                    }
                }
              else
                start_index = end_index;
              break;
            }
          found = TRUE;
          break;
        }
      prev_prev_line = prev_line; 
      prev_line = line; 
    }
  while (bango_layout_iter_next_line (iter));

  if (!found)
    {
      start_index = prev_line->start_index + prev_line->length;
      end_index = start_index;
    }
  bango_layout_iter_free (iter);
  *start_offset = g_utf8_pointer_to_offset (text, text + start_index);
  *end_offset = g_utf8_pointer_to_offset (text, text + end_index);
 
  btk_text_buffer_get_iter_at_offset (buffer, start_iter, *start_offset);
  btk_text_buffer_get_iter_at_offset (buffer, end_iter, *end_offset);
}
#endif

static bchar*
btk_icon_view_item_accessible_text_get_text_before_offset (BatkText         *text,
                                                           bint            offset,
                                                           BatkTextBoundary boundary_type,
                                                           bint            *start_offset,
                                                           bint            *end_offset)
{
  BtkIconViewItemAccessible *item;
  BtkTextIter start, end;
  BtkTextBuffer *buffer;
#if 0
  BtkIconView *icon_view;
#endif

  item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!BTK_IS_ICON_VIEW (item->widget))
    return NULL;

  if (batk_state_set_contains_state (item->state_set, BATK_STATE_DEFUNCT))
    return NULL;

  buffer = item->text_buffer;

  if (!btk_text_buffer_get_char_count (buffer))
    {
      *start_offset = 0;
      *end_offset = 0;
      return g_strdup ("");
    }
  btk_text_buffer_get_iter_at_offset (buffer, &start, offset);
   
  end = start;

  switch (boundary_type)
    {
    case BATK_TEXT_BOUNDARY_CHAR:
      btk_text_iter_backward_char(&start);
      break;
    case BATK_TEXT_BOUNDARY_WORD_START:
      if (!btk_text_iter_starts_word (&start))
        btk_text_iter_backward_word_start (&start);
      end = start;
      btk_text_iter_backward_word_start(&start);
      break;
    case BATK_TEXT_BOUNDARY_WORD_END:
      if (btk_text_iter_inside_word (&start) &&
          !btk_text_iter_starts_word (&start))
        btk_text_iter_backward_word_start (&start);
      while (!btk_text_iter_ends_word (&start))
        {
          if (!btk_text_iter_backward_char (&start))
            break;
        }
      end = start;
      btk_text_iter_backward_word_start(&start);
      while (!btk_text_iter_ends_word (&start))
        {
          if (!btk_text_iter_backward_char (&start))
            break;
        }
      break;
    case BATK_TEXT_BOUNDARY_SENTENCE_START:
      if (!btk_text_iter_starts_sentence (&start))
        btk_text_iter_backward_sentence_start (&start);
      end = start;
      btk_text_iter_backward_sentence_start (&start);
      break;
    case BATK_TEXT_BOUNDARY_SENTENCE_END:
      if (btk_text_iter_inside_sentence (&start) &&
          !btk_text_iter_starts_sentence (&start))
        btk_text_iter_backward_sentence_start (&start);
      while (!btk_text_iter_ends_sentence (&start))
        {
          if (!btk_text_iter_backward_char (&start))
            break;
        }
      end = start;
      btk_text_iter_backward_sentence_start (&start);
      while (!btk_text_iter_ends_sentence (&start))
        {
          if (!btk_text_iter_backward_char (&start))
            break;
        }
      break;
   case BATK_TEXT_BOUNDARY_LINE_START:
   case BATK_TEXT_BOUNDARY_LINE_END:
#if 0
      icon_view = BTK_ICON_VIEW (item->widget);
      /* FIXME we probably have to use BailTextCell to salvage this */
      btk_icon_view_update_item_text (icon_view, item->item);
      get_bango_text_offsets (icon_view->priv->layout,
                              buffer,
                              0,
                              boundary_type,
                              offset,
                              start_offset,
                              end_offset,
                              &start,
                              &end);
#endif
      break;
    }

  *start_offset = btk_text_iter_get_offset (&start);
  *end_offset = btk_text_iter_get_offset (&end);

  return btk_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static bchar*
btk_icon_view_item_accessible_text_get_text_at_offset (BatkText         *text,
                                                       bint            offset,
                                                       BatkTextBoundary boundary_type,
                                                       bint            *start_offset,
                                                       bint            *end_offset)
{
  BtkIconViewItemAccessible *item;
  BtkTextIter start, end;
  BtkTextBuffer *buffer;
#if 0
  BtkIconView *icon_view;
#endif

  item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!BTK_IS_ICON_VIEW (item->widget))
    return NULL;

  if (batk_state_set_contains_state (item->state_set, BATK_STATE_DEFUNCT))
    return NULL;

  buffer = item->text_buffer;

  if (!btk_text_buffer_get_char_count (buffer))
    {
      *start_offset = 0;
      *end_offset = 0;
      return g_strdup ("");
    }
  btk_text_buffer_get_iter_at_offset (buffer, &start, offset);
   
  end = start;

  switch (boundary_type)
    {
    case BATK_TEXT_BOUNDARY_CHAR:
      btk_text_iter_forward_char (&end);
      break;
    case BATK_TEXT_BOUNDARY_WORD_START:
      if (!btk_text_iter_starts_word (&start))
        btk_text_iter_backward_word_start (&start);
      if (btk_text_iter_inside_word (&end))
        btk_text_iter_forward_word_end (&end);
      while (!btk_text_iter_starts_word (&end))
        {
          if (!btk_text_iter_forward_char (&end))
            break;
        }
      break;
    case BATK_TEXT_BOUNDARY_WORD_END:
      if (btk_text_iter_inside_word (&start) &&
          !btk_text_iter_starts_word (&start))
        btk_text_iter_backward_word_start (&start);
      while (!btk_text_iter_ends_word (&start))
        {
          if (!btk_text_iter_backward_char (&start))
            break;
        }
      btk_text_iter_forward_word_end (&end);
      break;
    case BATK_TEXT_BOUNDARY_SENTENCE_START:
      if (!btk_text_iter_starts_sentence (&start))
        btk_text_iter_backward_sentence_start (&start);
      if (btk_text_iter_inside_sentence (&end))
        btk_text_iter_forward_sentence_end (&end);
      while (!btk_text_iter_starts_sentence (&end))
        {
          if (!btk_text_iter_forward_char (&end))
            break;
        }
      break;
    case BATK_TEXT_BOUNDARY_SENTENCE_END:
      if (btk_text_iter_inside_sentence (&start) &&
          !btk_text_iter_starts_sentence (&start))
        btk_text_iter_backward_sentence_start (&start);
      while (!btk_text_iter_ends_sentence (&start))
        {
          if (!btk_text_iter_backward_char (&start))
            break;
        }
      btk_text_iter_forward_sentence_end (&end);
      break;
   case BATK_TEXT_BOUNDARY_LINE_START:
   case BATK_TEXT_BOUNDARY_LINE_END:
#if 0
      icon_view = BTK_ICON_VIEW (item->widget);
      /* FIXME we probably have to use BailTextCell to salvage this */
      btk_icon_view_update_item_text (icon_view, item->item);
      get_bango_text_offsets (icon_view->priv->layout,
                              buffer,
                              1,
                              boundary_type,
                              offset,
                              start_offset,
                              end_offset,
                              &start,
                              &end);
#endif
      break;
    }


  *start_offset = btk_text_iter_get_offset (&start);
  *end_offset = btk_text_iter_get_offset (&end);

  return btk_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static bchar*
btk_icon_view_item_accessible_text_get_text_after_offset (BatkText         *text,
                                                          bint            offset,
                                                          BatkTextBoundary boundary_type,
                                                          bint            *start_offset,
                                                          bint            *end_offset)
{
  BtkIconViewItemAccessible *item;
  BtkTextIter start, end;
  BtkTextBuffer *buffer;
#if 0
  BtkIconView *icon_view;
#endif

  item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!BTK_IS_ICON_VIEW (item->widget))
    return NULL;

  if (batk_state_set_contains_state (item->state_set, BATK_STATE_DEFUNCT))
    return NULL;

  buffer = item->text_buffer;

  if (!btk_text_buffer_get_char_count (buffer))
    {
      *start_offset = 0;
      *end_offset = 0;
      return g_strdup ("");
    }
  btk_text_buffer_get_iter_at_offset (buffer, &start, offset);
   
  end = start;

  switch (boundary_type)
    {
    case BATK_TEXT_BOUNDARY_CHAR:
      btk_text_iter_forward_char(&start);
      btk_text_iter_forward_chars(&end, 2);
      break;
    case BATK_TEXT_BOUNDARY_WORD_START:
      if (btk_text_iter_inside_word (&end))
        btk_text_iter_forward_word_end (&end);
      while (!btk_text_iter_starts_word (&end))
        {
          if (!btk_text_iter_forward_char (&end))
            break;
        }
      start = end;
      if (!btk_text_iter_is_end (&end))
        {
          btk_text_iter_forward_word_end (&end);
          while (!btk_text_iter_starts_word (&end))
            {
              if (!btk_text_iter_forward_char (&end))
                break;
            }
        }
      break;
    case BATK_TEXT_BOUNDARY_WORD_END:
      btk_text_iter_forward_word_end (&end);
      start = end;
      if (!btk_text_iter_is_end (&end))
        btk_text_iter_forward_word_end (&end);
      break;
    case BATK_TEXT_BOUNDARY_SENTENCE_START:
      if (btk_text_iter_inside_sentence (&end))
        btk_text_iter_forward_sentence_end (&end);
      while (!btk_text_iter_starts_sentence (&end))
        {
          if (!btk_text_iter_forward_char (&end))
            break;
        }
      start = end;
      if (!btk_text_iter_is_end (&end))
        {
          btk_text_iter_forward_sentence_end (&end);
          while (!btk_text_iter_starts_sentence (&end))
            {
              if (!btk_text_iter_forward_char (&end))
                break;
            }
        }
      break;
    case BATK_TEXT_BOUNDARY_SENTENCE_END:
      btk_text_iter_forward_sentence_end (&end);
      start = end;
      if (!btk_text_iter_is_end (&end))
        btk_text_iter_forward_sentence_end (&end);
      break;
   case BATK_TEXT_BOUNDARY_LINE_START:
   case BATK_TEXT_BOUNDARY_LINE_END:
#if 0
      icon_view = BTK_ICON_VIEW (item->widget);
      /* FIXME we probably have to use BailTextCell to salvage this */
      btk_icon_view_update_item_text (icon_view, item->item);
      get_bango_text_offsets (icon_view->priv->layout,
                              buffer,
                              2,
                              boundary_type,
                              offset,
                              start_offset,
                              end_offset,
                              &start,
                              &end);
#endif
      break;
    }
  *start_offset = btk_text_iter_get_offset (&start);
  *end_offset = btk_text_iter_get_offset (&end);

  return btk_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static bint
btk_icon_view_item_accessible_text_get_character_count (BatkText *text)
{
  BtkIconViewItemAccessible *item;

  item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!BTK_IS_ICON_VIEW (item->widget))
    return 0;

  if (batk_state_set_contains_state (item->state_set, BATK_STATE_DEFUNCT))
    return 0;

  return btk_text_buffer_get_char_count (item->text_buffer);
}

static void
btk_icon_view_item_accessible_text_get_character_extents (BatkText      *text,
                                                          bint         offset,
                                                          bint         *x,
                                                          bint         *y,
                                                          bint         *width,
                                                          bint         *height,
                                                          BatkCoordType coord_type)
{
  BtkIconViewItemAccessible *item;
#if 0
  BtkIconView *icon_view;
  BangoRectangle char_rect;
  const bchar *item_text;
  bint index;
#endif

  item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!BTK_IS_ICON_VIEW (item->widget))
    return;

  if (batk_state_set_contains_state (item->state_set, BATK_STATE_DEFUNCT))
    return;

#if 0
  icon_view = BTK_ICON_VIEW (item->widget);
      /* FIXME we probably have to use BailTextCell to salvage this */
  btk_icon_view_update_item_text (icon_view, item->item);
  item_text = bango_layout_get_text (icon_view->priv->layout);
  index = g_utf8_offset_to_pointer (item_text, offset) - item_text;
  bango_layout_index_to_pos (icon_view->priv->layout, index, &char_rect);

  batk_component_get_position (BATK_COMPONENT (text), x, y, coord_type);
  *x += item->item->layout_x - item->item->x + char_rect.x / BANGO_SCALE;
  /* Look at btk_icon_view_paint_item() to see where the text is. */
  *x -=  ((item->item->width - item->item->layout_width) / 2) + (MAX (item->item->pixbuf_width, icon_view->priv->item_width) - item->item->width) / 2,
  *y += item->item->layout_y - item->item->y + char_rect.y / BANGO_SCALE;
  *width = char_rect.width / BANGO_SCALE;
  *height = char_rect.height / BANGO_SCALE;
#endif
}

static bint
btk_icon_view_item_accessible_text_get_offset_at_point (BatkText      *text,
                                                        bint          x,
                                                        bint          y,
                                                        BatkCoordType coord_type)
{
  BtkIconViewItemAccessible *item;
  bint offset = 0;
#if 0
  BtkIconView *icon_view;
  const bchar *item_text;
  bint index;
  bint l_x, l_y;
#endif

  item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!BTK_IS_ICON_VIEW (item->widget))
    return -1;

  if (batk_state_set_contains_state (item->state_set, BATK_STATE_DEFUNCT))
    return -1;

#if 0
  icon_view = BTK_ICON_VIEW (item->widget);
      /* FIXME we probably have to use BailTextCell to salvage this */
  btk_icon_view_update_item_text (icon_view, item->item);
  batk_component_get_position (BATK_COMPONENT (text), &l_x, &l_y, coord_type);
  x -= l_x + item->item->layout_x - item->item->x;
  x +=  ((item->item->width - item->item->layout_width) / 2) + (MAX (item->item->pixbuf_width, icon_view->priv->item_width) - item->item->width) / 2,
  y -= l_y + item->item->layout_y - item->item->y;
  item_text = bango_layout_get_text (icon_view->priv->layout);
  if (!bango_layout_xy_to_index (icon_view->priv->layout, 
                                x * BANGO_SCALE,
                                y * BANGO_SCALE,
                                &index, NULL))
    {
      if (x < 0 || y < 0)
        index = 0;
      else
        index = -1;
    } 
  if (index == -1)
    offset = g_utf8_strlen (item_text, -1);
  else
    offset = g_utf8_pointer_to_offset (item_text, item_text + index);
#endif
  return offset;
}

static void
batk_text_item_interface_init (BatkTextIface *iface)
{
  iface->get_text = btk_icon_view_item_accessible_text_get_text;
  iface->get_character_at_offset = btk_icon_view_item_accessible_text_get_character_at_offset;
  iface->get_text_before_offset = btk_icon_view_item_accessible_text_get_text_before_offset;
  iface->get_text_at_offset = btk_icon_view_item_accessible_text_get_text_at_offset;
  iface->get_text_after_offset = btk_icon_view_item_accessible_text_get_text_after_offset;
  iface->get_character_count = btk_icon_view_item_accessible_text_get_character_count;
  iface->get_character_extents = btk_icon_view_item_accessible_text_get_character_extents;
  iface->get_offset_at_point = btk_icon_view_item_accessible_text_get_offset_at_point;
}

static void
btk_icon_view_item_accessible_get_extents (BatkComponent *component,
                                           bint         *x,
                                           bint         *y,
                                           bint         *width,
                                           bint         *height,
                                           BatkCoordType  coord_type)
{
  BtkIconViewItemAccessible *item;
  BatkObject *parent_obj;
  bint l_x, l_y;

  g_return_if_fail (BTK_IS_ICON_VIEW_ITEM_ACCESSIBLE (component));

  item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (component);
  if (!BTK_IS_WIDGET (item->widget))
    return;

  if (batk_state_set_contains_state (item->state_set, BATK_STATE_DEFUNCT))
    return;

  *width = item->item->width;
  *height = item->item->height;
  if (btk_icon_view_item_accessible_is_showing (item))
    {
      parent_obj = btk_widget_get_accessible (item->widget);
      batk_component_get_position (BATK_COMPONENT (parent_obj), &l_x, &l_y, coord_type);
      *x = l_x + item->item->x;
      *y = l_y + item->item->y;
    }
  else
    {
      *x = B_MININT;
      *y = B_MININT;
    }
}

static bboolean
btk_icon_view_item_accessible_grab_focus (BatkComponent *component)
{
  BtkIconViewItemAccessible *item;
  BtkWidget *toplevel;

  g_return_val_if_fail (BTK_IS_ICON_VIEW_ITEM_ACCESSIBLE (component), FALSE);

  item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (component);
  if (!BTK_IS_WIDGET (item->widget))
    return FALSE;

  btk_widget_grab_focus (item->widget);
  btk_icon_view_set_cursor_item (BTK_ICON_VIEW (item->widget), item->item, -1);
  toplevel = btk_widget_get_toplevel (BTK_WIDGET (item->widget));
  if (btk_widget_is_toplevel (toplevel))
    btk_window_present (BTK_WINDOW (toplevel));

  return TRUE;
}

static void
batk_component_item_interface_init (BatkComponentIface *iface)
{
  iface->get_extents = btk_icon_view_item_accessible_get_extents;
  iface->grab_focus = btk_icon_view_item_accessible_grab_focus;
}

static bboolean
btk_icon_view_item_accessible_add_state (BtkIconViewItemAccessible *item,
                                         BatkStateType               state_type,
                                         bboolean                   emit_signal)
{
  bboolean rc;

  rc = batk_state_set_add_state (item->state_set, state_type);
  /*
   * The signal should only be generated if the value changed,
   * not when the item is set up.  So states that are set
   * initially should pass FALSE as the emit_signal argument.
   */

  if (emit_signal)
    {
      batk_object_notify_state_change (BATK_OBJECT (item), state_type, TRUE);
      /* If state_type is BATK_STATE_VISIBLE, additional notification */
      if (state_type == BATK_STATE_VISIBLE)
        g_signal_emit_by_name (item, "visible-data-changed");
    }

  return rc;
}

static bboolean
btk_icon_view_item_accessible_remove_state (BtkIconViewItemAccessible *item,
                                            BatkStateType               state_type,
                                            bboolean                   emit_signal)
{
  if (batk_state_set_contains_state (item->state_set, state_type))
    {
      bboolean rc;

      rc = batk_state_set_remove_state (item->state_set, state_type);
      /*
       * The signal should only be generated if the value changed,
       * not when the item is set up.  So states that are set
       * initially should pass FALSE as the emit_signal argument.
       */

      if (emit_signal)
        {
          batk_object_notify_state_change (BATK_OBJECT (item), state_type, FALSE);
          /* If state_type is BATK_STATE_VISIBLE, additional notification */
          if (state_type == BATK_STATE_VISIBLE)
            g_signal_emit_by_name (item, "visible-data-changed");
        }

      return rc;
    }
  else
    return FALSE;
}

static bboolean
btk_icon_view_item_accessible_is_showing (BtkIconViewItemAccessible *item)
{
  BtkIconView *icon_view;
  BdkRectangle visible_rect;
  bboolean is_showing;

  /*
   * An item is considered "SHOWING" if any part of the item is in the
   * visible rectangle.
   */

  if (!BTK_IS_ICON_VIEW (item->widget))
    return FALSE;

  if (item->item == NULL)
    return FALSE;

  icon_view = BTK_ICON_VIEW (item->widget);
  visible_rect.x = 0;
  if (icon_view->priv->hadjustment)
    visible_rect.x += icon_view->priv->hadjustment->value;
  visible_rect.y = 0;
  if (icon_view->priv->hadjustment)
    visible_rect.y += icon_view->priv->vadjustment->value;
  visible_rect.width = item->widget->allocation.width;
  visible_rect.height = item->widget->allocation.height;

  if (((item->item->x + item->item->width) < visible_rect.x) ||
     ((item->item->y + item->item->height) < (visible_rect.y)) ||
     (item->item->x > (visible_rect.x + visible_rect.width)) ||
     (item->item->y > (visible_rect.y + visible_rect.height)))
    is_showing =  FALSE;
  else
    is_showing = TRUE;

  return is_showing;
}

static bboolean
btk_icon_view_item_accessible_set_visibility (BtkIconViewItemAccessible *item,
                                              bboolean                   emit_signal)
{
  if (btk_icon_view_item_accessible_is_showing (item))
    return btk_icon_view_item_accessible_add_state (item, BATK_STATE_SHOWING,
						    emit_signal);
  else
    return btk_icon_view_item_accessible_remove_state (item, BATK_STATE_SHOWING,
						       emit_signal);
}

static void
btk_icon_view_item_accessible_object_init (BtkIconViewItemAccessible *item)
{
  bint i;

  item->state_set = batk_state_set_new ();

  batk_state_set_add_state (item->state_set, BATK_STATE_ENABLED);
  batk_state_set_add_state (item->state_set, BATK_STATE_FOCUSABLE);
  batk_state_set_add_state (item->state_set, BATK_STATE_SENSITIVE);
  batk_state_set_add_state (item->state_set, BATK_STATE_SELECTABLE);
  batk_state_set_add_state (item->state_set, BATK_STATE_VISIBLE);

  for (i = 0; i < LAST_ACTION; i++)
    item->action_descriptions[i] = NULL;

  item->image_description = NULL;

  item->action_idle_handler = 0;
}

static void
btk_icon_view_item_accessible_finalize (BObject *object)
{
  BtkIconViewItemAccessible *item;
  bint i;

  g_return_if_fail (BTK_IS_ICON_VIEW_ITEM_ACCESSIBLE (object));

  item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (object);

  if (item->widget)
    g_object_remove_weak_pointer (B_OBJECT (item->widget), (bpointer) &item->widget);

  if (item->state_set)
    g_object_unref (item->state_set);

  if (item->text_buffer)
     g_object_unref (item->text_buffer);

  for (i = 0; i < LAST_ACTION; i++)
    g_free (item->action_descriptions[i]);

  g_free (item->image_description);

  if (item->action_idle_handler)
    {
      g_source_remove (item->action_idle_handler);
      item->action_idle_handler = 0;
    }

  B_OBJECT_CLASS (accessible_item_parent_class)->finalize (object);
}

static BatkObject*
btk_icon_view_item_accessible_get_parent (BatkObject *obj)
{
  BtkIconViewItemAccessible *item;

  g_return_val_if_fail (BTK_IS_ICON_VIEW_ITEM_ACCESSIBLE (obj), NULL);
  item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (obj);

  if (item->widget)
    return btk_widget_get_accessible (item->widget);
  else
    return NULL;
}

static bint
btk_icon_view_item_accessible_get_index_in_parent (BatkObject *obj)
{
  BtkIconViewItemAccessible *item;

  g_return_val_if_fail (BTK_IS_ICON_VIEW_ITEM_ACCESSIBLE (obj), 0);
  item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (obj);

  return item->item->index; 
}

static BatkStateSet *
btk_icon_view_item_accessible_ref_state_set (BatkObject *obj)
{
  BtkIconViewItemAccessible *item;
  BtkIconView *icon_view;

  item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (obj);
  g_return_val_if_fail (item->state_set, NULL);

  if (!item->widget)
    return NULL;

  icon_view = BTK_ICON_VIEW (item->widget);
  if (icon_view->priv->cursor_item == item->item)
    batk_state_set_add_state (item->state_set, BATK_STATE_FOCUSED);
  else
    batk_state_set_remove_state (item->state_set, BATK_STATE_FOCUSED);
  if (item->item->selected)
    batk_state_set_add_state (item->state_set, BATK_STATE_SELECTED);
  else
    batk_state_set_remove_state (item->state_set, BATK_STATE_SELECTED);

  return g_object_ref (item->state_set);
}

static void
btk_icon_view_item_accessible_class_init (BatkObjectClass *klass)
{
  BObjectClass *bobject_class;

  accessible_item_parent_class = g_type_class_peek_parent (klass);

  bobject_class = (BObjectClass *)klass;

  bobject_class->finalize = btk_icon_view_item_accessible_finalize;

  klass->get_index_in_parent = btk_icon_view_item_accessible_get_index_in_parent;
  klass->get_parent = btk_icon_view_item_accessible_get_parent;
  klass->ref_state_set = btk_icon_view_item_accessible_ref_state_set;
}

static GType
btk_icon_view_item_accessible_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      const GTypeInfo tinfo =
      {
        sizeof (BtkIconViewItemAccessibleClass),
        (GBaseInitFunc) NULL, /* base init */
        (GBaseFinalizeFunc) NULL, /* base finalize */
        (GClassInitFunc) btk_icon_view_item_accessible_class_init, /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL, /* class data */
        sizeof (BtkIconViewItemAccessible), /* instance size */
        0, /* nb preallocs */
        (GInstanceInitFunc) btk_icon_view_item_accessible_object_init, /* instance init */
        NULL /* value table */
      };

      const GInterfaceInfo batk_component_info =
      {
        (GInterfaceInitFunc) batk_component_item_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };
      const GInterfaceInfo batk_action_info =
      {
        (GInterfaceInitFunc) batk_action_item_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };
      const GInterfaceInfo batk_image_info =
      {
        (GInterfaceInitFunc) batk_image_item_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };
      const GInterfaceInfo batk_text_info =
      {
        (GInterfaceInitFunc) batk_text_item_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      type = g_type_register_static (BATK_TYPE_OBJECT,
                                     I_("BtkIconViewItemAccessible"), &tinfo, 0);
      g_type_add_interface_static (type, BATK_TYPE_COMPONENT,
                                   &batk_component_info);
      g_type_add_interface_static (type, BATK_TYPE_ACTION,
                                   &batk_action_info);
      g_type_add_interface_static (type, BATK_TYPE_IMAGE,
                                   &batk_image_info);
      g_type_add_interface_static (type, BATK_TYPE_TEXT,
                                   &batk_text_info);
    }

  return type;
}

#define BTK_TYPE_ICON_VIEW_ACCESSIBLE      (btk_icon_view_accessible_get_type ())
#define BTK_ICON_VIEW_ACCESSIBLE(obj)      (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_ICON_VIEW_ACCESSIBLE, BtkIconViewAccessible))
#define BTK_IS_ICON_VIEW_ACCESSIBLE(obj)   (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_ICON_VIEW_ACCESSIBLE))

static GType btk_icon_view_accessible_get_type (void);

typedef struct
{
   BatkObject parent;
} BtkIconViewAccessible;

typedef struct
{
  BatkObject *item;
  bint       index;
} BtkIconViewItemAccessibleInfo;

typedef struct
{
  GList *items;

  BtkAdjustment *old_hadj;
  BtkAdjustment *old_vadj;

  BtkTreeModel *model;

} BtkIconViewAccessiblePrivate;

static BtkIconViewAccessiblePrivate *
btk_icon_view_accessible_get_priv (BatkObject *accessible)
{
  return g_object_get_qdata (B_OBJECT (accessible),
                             accessible_private_data_quark);
}

static void
btk_icon_view_item_accessible_info_new (BatkObject *accessible,
                                        BatkObject *item,
                                        bint       index)
{
  BtkIconViewItemAccessibleInfo *info;
  BtkIconViewItemAccessibleInfo *tmp_info;
  BtkIconViewAccessiblePrivate *priv;
  GList *items;

  info = g_new (BtkIconViewItemAccessibleInfo, 1);
  info->item = item;
  info->index = index;

  priv = btk_icon_view_accessible_get_priv (accessible);
  items = priv->items;
  while (items)
    {
      tmp_info = items->data;
      if (tmp_info->index > index)
        break;
      items = items->next;
    }
  priv->items = g_list_insert_before (priv->items, items, info);
  priv->old_hadj = NULL;
  priv->old_vadj = NULL;
}

static bint
btk_icon_view_accessible_get_n_children (BatkObject *accessible)
{
  BtkIconView *icon_view;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (accessible)->widget;
  if (!widget)
      return 0;

  icon_view = BTK_ICON_VIEW (widget);

  return g_list_length (icon_view->priv->items);
}

static BatkObject *
btk_icon_view_accessible_find_child (BatkObject *accessible,
                                     bint       index)
{
  BtkIconViewAccessiblePrivate *priv;
  BtkIconViewItemAccessibleInfo *info;
  GList *items;

  priv = btk_icon_view_accessible_get_priv (accessible);
  items = priv->items;

  while (items)
    {
      info = items->data;
      if (info->index == index)
        return info->item;
      items = items->next; 
    }
  return NULL;
}

static BatkObject *
btk_icon_view_accessible_ref_child (BatkObject *accessible,
                                    bint       index)
{
  BtkIconView *icon_view;
  BtkWidget *widget;
  GList *icons;
  BatkObject *obj;
  BtkIconViewItemAccessible *a11y_item;

  widget = BTK_ACCESSIBLE (accessible)->widget;
  if (!widget)
    return NULL;

  icon_view = BTK_ICON_VIEW (widget);
  icons = g_list_nth (icon_view->priv->items, index);
  obj = NULL;
  if (icons)
    {
      BtkIconViewItem *item = icons->data;
   
      g_return_val_if_fail (item->index == index, NULL);
      obj = btk_icon_view_accessible_find_child (accessible, index);
      if (!obj)
        {
          bchar *text;

          obj = g_object_new (btk_icon_view_item_accessible_get_type (), NULL);
          btk_icon_view_item_accessible_info_new (accessible,
                                                  obj,
                                                  index);
          obj->role = BATK_ROLE_ICON;
          a11y_item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (obj);
          a11y_item->item = item;
          a11y_item->widget = widget;
          a11y_item->text_buffer = btk_text_buffer_new (NULL);

	  btk_icon_view_set_cell_data (icon_view, item);
          text = get_text (icon_view, item);
          if (text)
            {
              btk_text_buffer_set_text (a11y_item->text_buffer, text, -1);
              g_free (text);
            } 

          btk_icon_view_item_accessible_set_visibility (a11y_item, FALSE);
          g_object_add_weak_pointer (B_OBJECT (widget), (bpointer) &(a11y_item->widget));
       }
      g_object_ref (obj);
    }
  return obj;
}

static void
btk_icon_view_accessible_traverse_items (BtkIconViewAccessible *view,
                                         GList                 *list)
{
  BtkIconViewAccessiblePrivate *priv;
  BtkIconViewItemAccessibleInfo *info;
  BtkIconViewItemAccessible *item;
  GList *items;
  
  priv =  btk_icon_view_accessible_get_priv (BATK_OBJECT (view));
  if (priv->items)
    {
      BtkWidget *widget;
      bboolean act_on_item;

      widget = BTK_ACCESSIBLE (view)->widget;
      if (widget == NULL)
        return;

      items = priv->items;

      act_on_item = (list == NULL);

      while (items)
        {

          info = (BtkIconViewItemAccessibleInfo *)items->data;
          item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (info->item);

          if (act_on_item == FALSE && list == items)
            act_on_item = TRUE;

          if (act_on_item)
	    btk_icon_view_item_accessible_set_visibility (item, TRUE);

          items = items->next;
       }
   }
}

static void
btk_icon_view_accessible_adjustment_changed (BtkAdjustment *adjustment,
                                             BtkIconView   *icon_view)
{
  BatkObject *obj;
  BtkIconViewAccessible *view;

  /*
   * The scrollbars have changed
   */
  obj = btk_widget_get_accessible (BTK_WIDGET (icon_view));
  view = BTK_ICON_VIEW_ACCESSIBLE (obj);

  btk_icon_view_accessible_traverse_items (view, NULL);
}

static void
btk_icon_view_accessible_set_scroll_adjustments (BtkWidget      *widget,
                                                 BtkAdjustment *hadj,
                                                 BtkAdjustment *vadj)
{
  BatkObject *batk_obj;
  BtkIconViewAccessiblePrivate *priv;

  batk_obj = btk_widget_get_accessible (widget);
  priv = btk_icon_view_accessible_get_priv (batk_obj);

  if (priv->old_hadj != hadj)
    {
      if (priv->old_hadj)
        {
          g_object_remove_weak_pointer (B_OBJECT (priv->old_hadj),
                                        (bpointer *)&priv->old_hadj);
          
          g_signal_handlers_disconnect_by_func (priv->old_hadj,
                                                (bpointer) btk_icon_view_accessible_adjustment_changed,
                                                widget);
        }
      priv->old_hadj = hadj;
      if (priv->old_hadj)
        {
          g_object_add_weak_pointer (B_OBJECT (priv->old_hadj),
                                     (bpointer *)&priv->old_hadj);
          g_signal_connect (hadj,
                            "value-changed",
                            G_CALLBACK (btk_icon_view_accessible_adjustment_changed),
                            widget);
        }
    }
  if (priv->old_vadj != vadj)
    {
      if (priv->old_vadj)
        {
          g_object_remove_weak_pointer (B_OBJECT (priv->old_vadj),
                                        (bpointer *)&priv->old_vadj);
          
          g_signal_handlers_disconnect_by_func (priv->old_vadj,
                                                (bpointer) btk_icon_view_accessible_adjustment_changed,
                                                widget);
        }
      priv->old_vadj = vadj;
      if (priv->old_vadj)
        {
          g_object_add_weak_pointer (B_OBJECT (priv->old_vadj),
                                     (bpointer *)&priv->old_vadj);
          g_signal_connect (vadj,
                            "value-changed",
                            G_CALLBACK (btk_icon_view_accessible_adjustment_changed),
                            widget);
        }
    }
}

static void
btk_icon_view_accessible_model_row_changed (BtkTreeModel *tree_model,
                                            BtkTreePath  *path,
                                            BtkTreeIter  *iter,
                                            bpointer      user_data)
{
  BatkObject *batk_obj;
  bint index;
  BtkWidget *widget;
  BtkIconView *icon_view;
  BtkIconViewItem *item;
  BtkIconViewAccessible *a11y_view;
  BtkIconViewItemAccessible *a11y_item;
  const bchar *name;
  bchar *text;

  batk_obj = btk_widget_get_accessible (BTK_WIDGET (user_data));
  a11y_view = BTK_ICON_VIEW_ACCESSIBLE (batk_obj);
  index = btk_tree_path_get_indices(path)[0];
  a11y_item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (
      btk_icon_view_accessible_find_child (batk_obj, index));

  if (a11y_item)
    {
      widget = BTK_ACCESSIBLE (batk_obj)->widget;
      icon_view = BTK_ICON_VIEW (widget);
      item = a11y_item->item;

      name = batk_object_get_name (BATK_OBJECT (a11y_item));

      if (!name || strcmp (name, "") == 0)
        {
          btk_icon_view_set_cell_data (icon_view, item);
          text = get_text (icon_view, item);
          if (text)
            {
              btk_text_buffer_set_text (a11y_item->text_buffer, text, -1);
              g_free (text);
            }
        }
    }

  g_signal_emit_by_name (batk_obj, "visible-data-changed");

  return;
}

static void
btk_icon_view_accessible_model_row_inserted (BtkTreeModel *tree_model,
                                             BtkTreePath  *path,
                                             BtkTreeIter  *iter,
                                             bpointer     user_data)
{
  BtkIconViewAccessiblePrivate *priv;
  BtkIconViewItemAccessibleInfo *info;
  BtkIconViewAccessible *view;
  BtkIconViewItemAccessible *item;
  GList *items;
  GList *tmp_list;
  BatkObject *batk_obj;
  bint index;

  index = btk_tree_path_get_indices(path)[0];
  batk_obj = btk_widget_get_accessible (BTK_WIDGET (user_data));
  view = BTK_ICON_VIEW_ACCESSIBLE (batk_obj);
  priv = btk_icon_view_accessible_get_priv (batk_obj);

  items = priv->items;
  tmp_list = NULL;
  while (items)
    {
      info = items->data;
      item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (info->item);
      if (info->index != item->item->index)
        {
          if (info->index < index)
            g_warning ("Unexpected index value on insertion %d %d", index, info->index);
 
          if (tmp_list == NULL)
            tmp_list = items;
   
          info->index = item->item->index;
        }

      items = items->next;
    }
  btk_icon_view_accessible_traverse_items (view, tmp_list);
  g_signal_emit_by_name (batk_obj, "children-changed::add",
                         index, NULL, NULL);
  return;
}

static void
btk_icon_view_accessible_model_row_deleted (BtkTreeModel *tree_model,
                                            BtkTreePath  *path,
                                            bpointer     user_data)
{
  BtkIconViewAccessiblePrivate *priv;
  BtkIconViewItemAccessibleInfo *info;
  BtkIconViewAccessible *view;
  BtkIconViewItemAccessible *item;
  GList *items;
  GList *tmp_list;
  GList *deleted_item;
  BatkObject *batk_obj;
  bint index;

  index = btk_tree_path_get_indices(path)[0];
  batk_obj = btk_widget_get_accessible (BTK_WIDGET (user_data));
  view = BTK_ICON_VIEW_ACCESSIBLE (batk_obj);
  priv = btk_icon_view_accessible_get_priv (batk_obj);

  items = priv->items;
  tmp_list = NULL;
  deleted_item = NULL;
  info = NULL;
  while (items)
    {
      info = items->data;
      item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (info->item);
      if (info->index == index)
        {
          deleted_item = items;
        }
      if (info->index != item->item->index)
        {
          if (tmp_list == NULL)
            tmp_list = items;
            
          info->index = item->item->index;
        }

      items = items->next;
    }
  btk_icon_view_accessible_traverse_items (view, tmp_list);
  if (deleted_item)
    {
      info = deleted_item->data;
      btk_icon_view_item_accessible_add_state (BTK_ICON_VIEW_ITEM_ACCESSIBLE (info->item), BATK_STATE_DEFUNCT, TRUE);
      g_signal_emit_by_name (batk_obj, "children-changed::remove",
                             index, NULL, NULL);
      priv->items = g_list_remove_link (priv->items, deleted_item);
      g_free (info);
    }

  return;
}

static bint
btk_icon_view_accessible_item_compare (BtkIconViewItemAccessibleInfo *i1,
                                       BtkIconViewItemAccessibleInfo *i2)
{
  return i1->index - i2->index;
}

static void
btk_icon_view_accessible_model_rows_reordered (BtkTreeModel *tree_model,
                                               BtkTreePath  *path,
                                               BtkTreeIter  *iter,
                                               bint         *new_order,
                                               bpointer     user_data)
{
  BtkIconViewAccessiblePrivate *priv;
  BtkIconViewItemAccessibleInfo *info;
  BtkIconView *icon_view;
  BtkIconViewItemAccessible *item;
  GList *items;
  BatkObject *batk_obj;
  bint *order;
  bint length, i;

  batk_obj = btk_widget_get_accessible (BTK_WIDGET (user_data));
  icon_view = BTK_ICON_VIEW (user_data);
  priv = btk_icon_view_accessible_get_priv (batk_obj);

  length = btk_tree_model_iter_n_children (tree_model, NULL);

  order = g_new (bint, length);
  for (i = 0; i < length; i++)
    order [new_order[i]] = i;

  items = priv->items;
  while (items)
    {
      info = items->data;
      item = BTK_ICON_VIEW_ITEM_ACCESSIBLE (info->item);
      info->index = order[info->index];
      item->item = g_list_nth_data (icon_view->priv->items, info->index);
      items = items->next;
    }
  g_free (order);
  priv->items = g_list_sort (priv->items, 
                             (GCompareFunc)btk_icon_view_accessible_item_compare);

  return;
}

static void
btk_icon_view_accessible_disconnect_model_signals (BtkTreeModel *model,
                                                   BtkWidget *widget)
{
  BObject *obj;

  obj = B_OBJECT (model);
  g_signal_handlers_disconnect_by_func (obj, (bpointer) btk_icon_view_accessible_model_row_changed, widget);
  g_signal_handlers_disconnect_by_func (obj, (bpointer) btk_icon_view_accessible_model_row_inserted, widget);
  g_signal_handlers_disconnect_by_func (obj, (bpointer) btk_icon_view_accessible_model_row_deleted, widget);
  g_signal_handlers_disconnect_by_func (obj, (bpointer) btk_icon_view_accessible_model_rows_reordered, widget);
}

static void
btk_icon_view_accessible_connect_model_signals (BtkIconView *icon_view)
{
  BObject *obj;

  obj = B_OBJECT (icon_view->priv->model);
  g_signal_connect_data (obj, "row-changed",
                         (GCallback) btk_icon_view_accessible_model_row_changed,
                         icon_view, NULL, 0);
  g_signal_connect_data (obj, "row-inserted",
                         (GCallback) btk_icon_view_accessible_model_row_inserted, 
                         icon_view, NULL, G_CONNECT_AFTER);
  g_signal_connect_data (obj, "row-deleted",
                         (GCallback) btk_icon_view_accessible_model_row_deleted, 
                         icon_view, NULL, G_CONNECT_AFTER);
  g_signal_connect_data (obj, "rows-reordered",
                         (GCallback) btk_icon_view_accessible_model_rows_reordered, 
                         icon_view, NULL, G_CONNECT_AFTER);
}

static void
btk_icon_view_accessible_clear_cache (BtkIconViewAccessiblePrivate *priv)
{
  BtkIconViewItemAccessibleInfo *info;
  GList *items;

  items = priv->items;
  while (items)
    {
      info = (BtkIconViewItemAccessibleInfo *) items->data;
      g_object_unref (info->item);
      g_free (items->data);
      items = items->next;
    }
  g_list_free (priv->items);
  priv->items = NULL;
}

static void
btk_icon_view_accessible_notify_btk (BObject *obj,
                                     BParamSpec *pspec)
{
  BtkIconView *icon_view;
  BtkWidget *widget;
  BatkObject *batk_obj;
  BtkIconViewAccessiblePrivate *priv;

  if (strcmp (pspec->name, "model") == 0)
    {
      widget = BTK_WIDGET (obj); 
      batk_obj = btk_widget_get_accessible (widget);
      priv = btk_icon_view_accessible_get_priv (batk_obj);
      if (priv->model)
        {
          g_object_remove_weak_pointer (B_OBJECT (priv->model),
                                        (bpointer *)&priv->model);
          btk_icon_view_accessible_disconnect_model_signals (priv->model, widget);
        }
      btk_icon_view_accessible_clear_cache (priv);

      icon_view = BTK_ICON_VIEW (obj);
      priv->model = icon_view->priv->model;
      /* If there is no model the BtkIconView is probably being destroyed */
      if (priv->model)
        {
          g_object_add_weak_pointer (B_OBJECT (priv->model), (bpointer *)&priv->model);
          btk_icon_view_accessible_connect_model_signals (icon_view);
        }
    }

  return;
}

static void
btk_icon_view_accessible_initialize (BatkObject *accessible,
                                     bpointer   data)
{
  BtkIconViewAccessiblePrivate *priv;
  BtkIconView *icon_view;

  if (BATK_OBJECT_CLASS (accessible_parent_class)->initialize)
    BATK_OBJECT_CLASS (accessible_parent_class)->initialize (accessible, data);

  priv = g_new0 (BtkIconViewAccessiblePrivate, 1);
  g_object_set_qdata (B_OBJECT (accessible),
                      accessible_private_data_quark,
                      priv);

  icon_view = BTK_ICON_VIEW (data);
  if (icon_view->priv->hadjustment)
    {
      priv->old_hadj = icon_view->priv->hadjustment;
      g_object_add_weak_pointer (B_OBJECT (priv->old_hadj), (bpointer *)&priv->old_hadj);
      g_signal_connect (icon_view->priv->hadjustment,
                        "value-changed",
                        G_CALLBACK (btk_icon_view_accessible_adjustment_changed),
                        icon_view);
    } 
  if (icon_view->priv->vadjustment)
    {
      priv->old_vadj = icon_view->priv->vadjustment;
      g_object_add_weak_pointer (B_OBJECT (priv->old_vadj), (bpointer *)&priv->old_vadj);
      g_signal_connect (icon_view->priv->vadjustment,
                        "value-changed",
                        G_CALLBACK (btk_icon_view_accessible_adjustment_changed),
                        icon_view);
    }
  g_signal_connect_after (data,
                          "set-scroll-adjustments",
                          G_CALLBACK (btk_icon_view_accessible_set_scroll_adjustments),
                          NULL);
  g_signal_connect (data,
                    "notify",
                    G_CALLBACK (btk_icon_view_accessible_notify_btk),
                    NULL);

  priv->model = icon_view->priv->model;
  if (priv->model)
    {
      g_object_add_weak_pointer (B_OBJECT (priv->model), (bpointer *)&priv->model);
      btk_icon_view_accessible_connect_model_signals (icon_view);
    }
                          
  accessible->role = BATK_ROLE_LAYERED_PANE;
}

static void
btk_icon_view_accessible_finalize (BObject *object)
{
  BtkIconViewAccessiblePrivate *priv;

  priv = btk_icon_view_accessible_get_priv (BATK_OBJECT (object));
  btk_icon_view_accessible_clear_cache (priv);

  g_free (priv);

  B_OBJECT_CLASS (accessible_parent_class)->finalize (object);
}

static void
btk_icon_view_accessible_destroyed (BtkWidget *widget,
                                    BtkAccessible *accessible)
{
  BatkObject *batk_obj;
  BtkIconViewAccessiblePrivate *priv;

  batk_obj = BATK_OBJECT (accessible);
  priv = btk_icon_view_accessible_get_priv (batk_obj);
  if (priv->old_hadj)
    {
      g_object_remove_weak_pointer (B_OBJECT (priv->old_hadj),
                                    (bpointer *)&priv->old_hadj);
          
      g_signal_handlers_disconnect_by_func (priv->old_hadj,
                                            (bpointer) btk_icon_view_accessible_adjustment_changed,
                                            widget);
      priv->old_hadj = NULL;
    }
  if (priv->old_vadj)
    {
      g_object_remove_weak_pointer (B_OBJECT (priv->old_vadj),
                                    (bpointer *)&priv->old_vadj);
          
      g_signal_handlers_disconnect_by_func (priv->old_vadj,
                                            (bpointer) btk_icon_view_accessible_adjustment_changed,
                                            widget);
      priv->old_vadj = NULL;
    }
}

static void
btk_icon_view_accessible_connect_widget_destroyed (BtkAccessible *accessible)
{
  if (accessible->widget)
    {
      g_signal_connect_after (accessible->widget,
                              "destroy",
                              G_CALLBACK (btk_icon_view_accessible_destroyed),
                              accessible);
    }
  BTK_ACCESSIBLE_CLASS (accessible_parent_class)->connect_widget_destroyed (accessible);
}

static void
btk_icon_view_accessible_class_init (BatkObjectClass *klass)
{
  BObjectClass *bobject_class;
  BtkAccessibleClass *accessible_class;

  accessible_parent_class = g_type_class_peek_parent (klass);

  bobject_class = (BObjectClass *)klass;
  accessible_class = (BtkAccessibleClass *)klass;

  bobject_class->finalize = btk_icon_view_accessible_finalize;

  klass->get_n_children = btk_icon_view_accessible_get_n_children;
  klass->ref_child = btk_icon_view_accessible_ref_child;
  klass->initialize = btk_icon_view_accessible_initialize;

  accessible_class->connect_widget_destroyed = btk_icon_view_accessible_connect_widget_destroyed;

  accessible_private_data_quark = g_quark_from_static_string ("icon_view-accessible-private-data");
}

static BatkObject*
btk_icon_view_accessible_ref_accessible_at_point (BatkComponent *component,
                                                  bint          x,
                                                  bint          y,
                                                  BatkCoordType  coord_type)
{
  BtkWidget *widget;
  BtkIconView *icon_view;
  BtkIconViewItem *item;
  bint x_pos, y_pos;

  widget = BTK_ACCESSIBLE (component)->widget;
  if (widget == NULL)
  /* State is defunct */
    return NULL;

  icon_view = BTK_ICON_VIEW (widget);
  batk_component_get_extents (component, &x_pos, &y_pos, NULL, NULL, coord_type);
  item = btk_icon_view_get_item_at_coords (icon_view, x - x_pos, y - y_pos, TRUE, NULL);
  if (item)
    return btk_icon_view_accessible_ref_child (BATK_OBJECT (component), item->index);

  return NULL;
}

static void
batk_component_interface_init (BatkComponentIface *iface)
{
  iface->ref_accessible_at_point = btk_icon_view_accessible_ref_accessible_at_point;
}

static bboolean
btk_icon_view_accessible_add_selection (BatkSelection *selection,
                                        bint i)
{
  BtkWidget *widget;
  BtkIconView *icon_view;
  BtkIconViewItem *item;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    return FALSE;

  icon_view = BTK_ICON_VIEW (widget);

  item = g_list_nth_data (icon_view->priv->items, i);

  if (!item)
    return FALSE;

  btk_icon_view_select_item (icon_view, item);

  return TRUE;
}

static bboolean
btk_icon_view_accessible_clear_selection (BatkSelection *selection)
{
  BtkWidget *widget;
  BtkIconView *icon_view;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    return FALSE;

  icon_view = BTK_ICON_VIEW (widget);
  btk_icon_view_unselect_all (icon_view);

  return TRUE;
}

static BatkObject*
btk_icon_view_accessible_ref_selection (BatkSelection *selection,
                                        bint          i)
{
  GList *l;
  BtkWidget *widget;
  BtkIconView *icon_view;
  BtkIconViewItem *item;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    return NULL;

  icon_view = BTK_ICON_VIEW (widget);

  l = icon_view->priv->items;
  while (l)
    {
      item = l->data;
      if (item->selected)
        {
          if (i == 0)
	    return batk_object_ref_accessible_child (btk_widget_get_accessible (widget), item->index);
          else
            i--;
        }
      l = l->next;
    }

  return NULL;
}

static bint
btk_icon_view_accessible_get_selection_count (BatkSelection *selection)
{
  BtkWidget *widget;
  BtkIconView *icon_view;
  BtkIconViewItem *item;
  GList *l;
  bint count;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    return 0;

  icon_view = BTK_ICON_VIEW (widget);

  l = icon_view->priv->items;
  count = 0;
  while (l)
    {
      item = l->data;

      if (item->selected)
	count++;

      l = l->next;
    }

  return count;
}

static bboolean
btk_icon_view_accessible_is_child_selected (BatkSelection *selection,
                                            bint          i)
{
  BtkWidget *widget;
  BtkIconView *icon_view;
  BtkIconViewItem *item;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    return FALSE;

  icon_view = BTK_ICON_VIEW (widget);

  item = g_list_nth_data (icon_view->priv->items, i);
  if (!item)
    return FALSE;

  return item->selected;
}

static bboolean
btk_icon_view_accessible_remove_selection (BatkSelection *selection,
                                           bint          i)
{
  BtkWidget *widget;
  BtkIconView *icon_view;
  BtkIconViewItem *item;
  GList *l;
  bint count;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    return FALSE;

  icon_view = BTK_ICON_VIEW (widget);
  l = icon_view->priv->items;
  count = 0;
  while (l)
    {
      item = l->data;
      if (item->selected)
        {
          if (count == i)
            {
              btk_icon_view_unselect_item (icon_view, item);
              return TRUE;
            }
          count++;
        }
      l = l->next;
    }

  return FALSE;
}
 
static bboolean
btk_icon_view_accessible_select_all_selection (BatkSelection *selection)
{
  BtkWidget *widget;
  BtkIconView *icon_view;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    return FALSE;

  icon_view = BTK_ICON_VIEW (widget);
  btk_icon_view_select_all (icon_view);
  return TRUE;
}

static void
btk_icon_view_accessible_selection_interface_init (BatkSelectionIface *iface)
{
  iface->add_selection = btk_icon_view_accessible_add_selection;
  iface->clear_selection = btk_icon_view_accessible_clear_selection;
  iface->ref_selection = btk_icon_view_accessible_ref_selection;
  iface->get_selection_count = btk_icon_view_accessible_get_selection_count;
  iface->is_child_selected = btk_icon_view_accessible_is_child_selected;
  iface->remove_selection = btk_icon_view_accessible_remove_selection;
  iface->select_all_selection = btk_icon_view_accessible_select_all_selection;
}

static GType
btk_icon_view_accessible_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      GTypeInfo tinfo =
      {
        0, /* class size */
        (GBaseInitFunc) NULL, /* base init */
        (GBaseFinalizeFunc) NULL, /* base finalize */
        (GClassInitFunc) btk_icon_view_accessible_class_init,
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL, /* class data */
        0, /* instance size */
        0, /* nb preallocs */
        (GInstanceInitFunc) NULL, /* instance init */
        NULL /* value table */
      };
      const GInterfaceInfo batk_component_info =
      {
        (GInterfaceInitFunc) batk_component_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };
      const GInterfaceInfo batk_selection_info =
      {
        (GInterfaceInitFunc) btk_icon_view_accessible_selection_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      /*
       * Figure out the size of the class and instance
       * we are deriving from
       */
      BatkObjectFactory *factory;
      GType derived_type;
      GTypeQuery query;
      GType derived_batk_type;

      derived_type = g_type_parent (BTK_TYPE_ICON_VIEW);
      factory = batk_registry_get_factory (batk_get_default_registry (), 
                                          derived_type);
      derived_batk_type = batk_object_factory_get_accessible_type (factory);
      g_type_query (derived_batk_type, &query);
      tinfo.class_size = query.class_size;
      tinfo.instance_size = query.instance_size;
 
      type = g_type_register_static (derived_batk_type, 
                                     I_("BtkIconViewAccessible"), 
                                     &tinfo, 0);
      g_type_add_interface_static (type, BATK_TYPE_COMPONENT,
                                   &batk_component_info);
      g_type_add_interface_static (type, BATK_TYPE_SELECTION,
                                   &batk_selection_info);
    }
  return type;
}

static BatkObject *
btk_icon_view_accessible_new (BObject *obj)
{
  BatkObject *accessible;

  g_return_val_if_fail (BTK_IS_WIDGET (obj), NULL);

  accessible = g_object_new (btk_icon_view_accessible_get_type (), NULL);
  batk_object_initialize (accessible, obj);

  return accessible;
}

static GType
btk_icon_view_accessible_factory_get_accessible_type (void)
{
  return btk_icon_view_accessible_get_type ();
}

static BatkObject*
btk_icon_view_accessible_factory_create_accessible (BObject *obj)
{
  return btk_icon_view_accessible_new (obj);
}

static void
btk_icon_view_accessible_factory_class_init (BatkObjectFactoryClass *klass)
{
  klass->create_accessible = btk_icon_view_accessible_factory_create_accessible;
  klass->get_accessible_type = btk_icon_view_accessible_factory_get_accessible_type;
}

static GType
btk_icon_view_accessible_factory_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      const GTypeInfo tinfo =
      {
        sizeof (BatkObjectFactoryClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) btk_icon_view_accessible_factory_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (BatkObjectFactory),
        0,             /* n_preallocs */
        NULL, NULL
      };

      type = g_type_register_static (BATK_TYPE_OBJECT_FACTORY, 
                                    I_("BtkIconViewAccessibleFactory"),
                                    &tinfo, 0);
    }
  return type;
}


static BatkObject *
btk_icon_view_get_accessible (BtkWidget *widget)
{
  static bboolean first_time = TRUE;

  if (first_time)
    {
      BatkObjectFactory *factory;
      BatkRegistry *registry;
      GType derived_type; 
      GType derived_batk_type; 

      /*
       * Figure out whether accessibility is enabled by looking at the
       * type of the accessible object which would be created for
       * the parent type of BtkIconView.
       */
      derived_type = g_type_parent (BTK_TYPE_ICON_VIEW);

      registry = batk_get_default_registry ();
      factory = batk_registry_get_factory (registry,
                                          derived_type);
      derived_batk_type = batk_object_factory_get_accessible_type (factory);
      if (g_type_is_a (derived_batk_type, BTK_TYPE_ACCESSIBLE)) 
	batk_registry_set_factory_type (registry, 
				       BTK_TYPE_ICON_VIEW,
				       btk_icon_view_accessible_factory_get_type ());
      first_time = FALSE;
    } 
  return BTK_WIDGET_CLASS (btk_icon_view_parent_class)->get_accessible (widget);
}

static bboolean
btk_icon_view_buildable_custom_tag_start (BtkBuildable  *buildable,
					  BtkBuilder    *builder,
					  BObject       *child,
					  const bchar   *tagname,
					  GMarkupParser *parser,
					  bpointer      *data)
{
  if (parent_buildable_iface->custom_tag_start (buildable, builder, child,
						tagname, parser, data))
    return TRUE;

  return _btk_cell_layout_buildable_custom_tag_start (buildable, builder, child,
						      tagname, parser, data);
}

static void
btk_icon_view_buildable_custom_tag_end (BtkBuildable *buildable,
					BtkBuilder   *builder,
					BObject      *child,
					const bchar  *tagname,
					bpointer     *data)
{
  if (strcmp (tagname, "attributes") == 0)
    _btk_cell_layout_buildable_custom_tag_end (buildable, builder, child, tagname,
					       data);
  else
    parent_buildable_iface->custom_tag_end (buildable, builder, child, tagname,
					    data);
}



#define __BTK_ICON_VIEW_C__
#include "btkaliasdef.c"
