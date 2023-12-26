/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball, Josh MacDonald, 
 * Copyright (C) 1997-1998 Jay Painter <jpaint@serv.net><jpaint@gimp.org>  
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

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#undef BDK_DISABLE_DEPRECATED
#undef BTK_DISABLE_DEPRECATED
#define __BTK_CLIST_C__

#include <bdk/bdkkeysyms.h>

#include "btkmain.h"
#include "btkobject.h"
#include "btkctree.h"
#include "btkclist.h"
#include "btkbindings.h"
#include "btkdnd.h"
#include "btkmarshalers.h"
#include "btkintl.h"

#include "btkalias.h"

/* length of button_actions array */
#define MAX_BUTTON 5

/* the width of the column resize windows */
#define DRAG_WIDTH  6

/* minimum allowed width of a column */
#define COLUMN_MIN_WIDTH 5

/* this defigns the base grid spacing */
#define CELL_SPACING 1

/* added the horizontal space at the beginning and end of a row*/
#define COLUMN_INSET 3

/* used for auto-scrolling */
#define SCROLL_TIME  100

/* gives the top pixel of the given row in context of
 * the clist's voffset */
#define ROW_TOP_YPIXEL(clist, row) (((clist)->row_height * (row)) + \
				    (((row) + 1) * CELL_SPACING) + \
				    (clist)->voffset)

/* returns the row index from a y pixel location in the 
 * context of the clist's voffset */
#define ROW_FROM_YPIXEL(clist, y)  (((y) - (clist)->voffset) / \
				    ((clist)->row_height + CELL_SPACING))

/* gives the left pixel of the given column in context of
 * the clist's hoffset */
#define COLUMN_LEFT_XPIXEL(clist, colnum)  ((clist)->column[(colnum)].area.x + \
					    (clist)->hoffset)

/* returns the column index from a x pixel location in the 
 * context of the clist's hoffset */
static inline gint
COLUMN_FROM_XPIXEL (BtkCList * clist,
		    gint x)
{
  gint i, cx;

  for (i = 0; i < clist->columns; i++)
    if (clist->column[i].visible)
      {
	cx = clist->column[i].area.x + clist->hoffset;

	if (x >= (cx - (COLUMN_INSET + CELL_SPACING)) &&
	    x <= (cx + clist->column[i].area.width + COLUMN_INSET))
	  return i;
      }

  /* no match */
  return -1;
}

/* returns the top pixel of the given row in the context of
 * the list height */
#define ROW_TOP(clist, row)        (((clist)->row_height + CELL_SPACING) * (row))

/* returns the left pixel of the given column in the context of
 * the list width */
#define COLUMN_LEFT(clist, colnum) ((clist)->column[(colnum)].area.x)

/* returns the total height of the list */
#define LIST_HEIGHT(clist)         (((clist)->row_height * ((clist)->rows)) + \
				    (CELL_SPACING * ((clist)->rows + 1)))


/* returns the total width of the list */
static inline gint
LIST_WIDTH (BtkCList * clist) 
{
  gint last_column;

  for (last_column = clist->columns - 1;
       last_column >= 0 && !clist->column[last_column].visible; last_column--);

  if (last_column >= 0)
    return (clist->column[last_column].area.x +
	    clist->column[last_column].area.width +
	    COLUMN_INSET + CELL_SPACING);
  return 0;
}

/* returns the GList item for the nth row */
#define	ROW_ELEMENT(clist, row)	(((row) == (clist)->rows - 1) ? \
				 (clist)->row_list_end : \
				 g_list_nth ((clist)->row_list, (row)))


/* redraw the list if it's not frozen */
#define CLIST_UNFROZEN(clist)     (((BtkCList*) (clist))->freeze_count == 0)
#define	CLIST_REFRESH(clist)	B_STMT_START { \
  if (CLIST_UNFROZEN (clist)) \
    BTK_CLIST_GET_CLASS (clist)->refresh ((BtkCList*) (clist)); \
} B_STMT_END


/* Signals */
enum {
  SELECT_ROW,
  UNSELECT_ROW,
  ROW_MOVE,
  CLICK_COLUMN,
  RESIZE_COLUMN,
  TOGGLE_FOCUS_ROW,
  SELECT_ALL,
  UNSELECT_ALL,
  UNDO_SELECTION,
  START_SELECTION,
  END_SELECTION,
  TOGGLE_ADD_MODE,
  EXTEND_SELECTION,
  SCROLL_VERTICAL,
  SCROLL_HORIZONTAL,
  ABORT_COLUMN_RESIZE,
  LAST_SIGNAL
};

enum {
  SYNC_REMOVE,
  SYNC_INSERT
};

enum {
  ARG_0,
  ARG_N_COLUMNS,
  ARG_SHADOW_TYPE,
  ARG_SELECTION_MODE,
  ARG_ROW_HEIGHT,
  ARG_TITLES_ACTIVE,
  ARG_REORDERABLE,
  ARG_USE_DRAG_ICONS,
  ARG_SORT_TYPE
};

/* BtkCList Methods */
static void     btk_clist_class_init  (BtkCListClass         *klass);
static void     btk_clist_init        (BtkCList              *clist);
static GObject* btk_clist_constructor (GType                  type,
				       guint                  n_construct_properties,
				       GObjectConstructParam *construct_params);

/* BtkObject Methods */
static void btk_clist_destroy  (BtkObject *object);
static void btk_clist_finalize (GObject   *object);
static void btk_clist_set_arg  (BtkObject *object,
				BtkArg    *arg,
				guint      arg_id);
static void btk_clist_get_arg  (BtkObject *object,
				BtkArg    *arg,
				guint      arg_id);

/* BtkWidget Methods */
static void btk_clist_set_scroll_adjustments (BtkCList      *clist,
					      BtkAdjustment *hadjustment,
					      BtkAdjustment *vadjustment);
static void btk_clist_realize         (BtkWidget        *widget);
static void btk_clist_unrealize       (BtkWidget        *widget);
static void btk_clist_map             (BtkWidget        *widget);
static void btk_clist_unmap           (BtkWidget        *widget);
static gint btk_clist_expose          (BtkWidget        *widget,
			               BdkEventExpose   *event);
static gint btk_clist_button_press    (BtkWidget        *widget,
				       BdkEventButton   *event);
static gint btk_clist_button_release  (BtkWidget        *widget,
				       BdkEventButton   *event);
static gint btk_clist_motion          (BtkWidget        *widget, 
			               BdkEventMotion   *event);
static void btk_clist_size_request    (BtkWidget        *widget,
				       BtkRequisition   *requisition);
static void btk_clist_size_allocate   (BtkWidget        *widget,
				       BtkAllocation    *allocation);
static void btk_clist_draw_focus      (BtkWidget        *widget);
static gint btk_clist_focus_in        (BtkWidget        *widget,
				       BdkEventFocus    *event);
static gint btk_clist_focus_out       (BtkWidget        *widget,
				       BdkEventFocus    *event);
static gint btk_clist_focus           (BtkWidget        *widget,
				       BtkDirectionType  direction);
static void btk_clist_set_focus_child (BtkContainer     *container,
				       BtkWidget        *child);
static void btk_clist_style_set       (BtkWidget        *widget,
				       BtkStyle         *previous_style);
static void btk_clist_drag_begin      (BtkWidget        *widget,
				       BdkDragContext   *context);
static gint btk_clist_drag_motion     (BtkWidget        *widget,
				       BdkDragContext   *context,
				       gint              x,
				       gint              y,
				       guint             time);
static void btk_clist_drag_leave      (BtkWidget        *widget,
				       BdkDragContext   *context,
				       guint             time);
static void btk_clist_drag_end        (BtkWidget        *widget,
				       BdkDragContext   *context);
static gboolean btk_clist_drag_drop   (BtkWidget      *widget,
				       BdkDragContext *context,
				       gint            x,
				       gint            y,
				       guint           time);
static void btk_clist_drag_data_get   (BtkWidget        *widget,
				       BdkDragContext   *context,
				       BtkSelectionData *selection_data,
				       guint             info,
				       guint             time);
static void btk_clist_drag_data_received (BtkWidget        *widget,
					  BdkDragContext   *context,
					  gint              x,
					  gint              y,
					  BtkSelectionData *selection_data,
					  guint             info,
					  guint             time);

/* BtkContainer Methods */
static void btk_clist_forall          (BtkContainer  *container,
			               gboolean       include_internals,
			               BtkCallback    callback,
			               gpointer       callback_data);

/* Selection */
static void toggle_row                (BtkCList      *clist,
			               gint           row,
			               gint           column,
			               BdkEvent      *event);
static void real_select_row           (BtkCList      *clist,
			               gint           row,
			               gint           column,
			               BdkEvent      *event);
static void real_unselect_row         (BtkCList      *clist,
			               gint           row,
			               gint           column,
			               BdkEvent      *event);
static void update_extended_selection (BtkCList      *clist,
				       gint           row);
static GList *selection_find          (BtkCList      *clist,
			               gint           row_number,
			               GList         *row_list_element);
static void real_select_all           (BtkCList      *clist);
static void real_unselect_all         (BtkCList      *clist);
static void move_vertical             (BtkCList      *clist,
			               gint           row,
			               gfloat         align);
static void move_horizontal           (BtkCList      *clist,
			               gint           diff);
static void real_undo_selection       (BtkCList      *clist);
static void fake_unselect_all         (BtkCList      *clist,
			               gint           row);
static void fake_toggle_row           (BtkCList      *clist,
			               gint           row);
static void resync_selection          (BtkCList      *clist,
			               BdkEvent      *event);
static void sync_selection            (BtkCList      *clist,
	                               gint           row,
                                       gint           mode);
static void set_anchor                (BtkCList      *clist,
			               gboolean       add_mode,
			               gint           anchor,
			               gint           undo_anchor);
static void start_selection           (BtkCList      *clist);
static void end_selection             (BtkCList      *clist);
static void toggle_add_mode           (BtkCList      *clist);
static void toggle_focus_row          (BtkCList      *clist);
static void extend_selection          (BtkCList      *clist,
			               BtkScrollType  scroll_type,
			               gfloat         position,
			               gboolean       auto_start_selection);
static gint get_selection_info        (BtkCList       *clist,
				       gint            x,
				       gint            y,
				       gint           *row,
				       gint           *column);

/* Scrolling */
static void move_focus_row     (BtkCList      *clist,
			        BtkScrollType  scroll_type,
			        gfloat         position);
static void scroll_horizontal  (BtkCList      *clist,
			        BtkScrollType  scroll_type,
			        gfloat         position);
static void scroll_vertical    (BtkCList      *clist,
			        BtkScrollType  scroll_type,
			        gfloat         position);
static void move_horizontal    (BtkCList      *clist,
				gint           diff);
static void move_vertical      (BtkCList      *clist,
				gint           row,
				gfloat         align);
static gint horizontal_timeout (BtkCList      *clist);
static gint vertical_timeout   (BtkCList      *clist);
static void remove_grab        (BtkCList      *clist);


/* Resize Columns */
static void draw_xor_line             (BtkCList       *clist);
static gint new_column_width          (BtkCList       *clist,
			               gint            column,
			               gint           *x);
static void column_auto_resize        (BtkCList       *clist,
				       BtkCListRow    *clist_row,
				       gint            column,
				       gint            old_width);
static void real_resize_column        (BtkCList       *clist,
				       gint            column,
				       gint            width);
static void abort_column_resize       (BtkCList       *clist);
static void cell_size_request         (BtkCList       *clist,
			               BtkCListRow    *clist_row,
			               gint            column,
				       BtkRequisition *requisition);

/* Buttons */
static void column_button_create      (BtkCList       *clist,
				       gint            column);
static void column_button_clicked     (BtkWidget      *widget,
				       gpointer        data);

/* Adjustments */
static void adjust_adjustments        (BtkCList       *clist,
				       gboolean        block_resize);
static void vadjustment_changed       (BtkAdjustment  *adjustment,
				       gpointer        data);
static void vadjustment_value_changed (BtkAdjustment  *adjustment,
				       gpointer        data);
static void hadjustment_changed       (BtkAdjustment  *adjustment,
				       gpointer        data);
static void hadjustment_value_changed (BtkAdjustment  *adjustment,
				       gpointer        data);

/* Drawing */
static void get_cell_style   (BtkCList      *clist,
			      BtkCListRow   *clist_row,
			      gint           state,
			      gint           column,
			      BtkStyle     **style,
			      BdkGC        **fg_gc,
			      BdkGC        **bg_gc);
static gint draw_cell_pixmap (BdkWindow     *window,
			      BdkRectangle  *clip_rectangle,
			      BdkGC         *fg_gc,
			      BdkPixmap     *pixmap,
			      BdkBitmap     *mask,
			      gint           x,
			      gint           y,
			      gint           width,
			      gint           height);
static void draw_row         (BtkCList      *clist,
			      BdkRectangle  *area,
			      gint           row,
			      BtkCListRow   *clist_row);
static void draw_rows        (BtkCList      *clist,
			      BdkRectangle  *area);
static void clist_refresh    (BtkCList      *clist);
static void draw_drag_highlight (BtkCList        *clist,
				 BtkCListRow     *dest_row,
				 gint             dest_row_number,
				 BtkCListDragPos  drag_pos);
     
/* Size Allocation / Requisition */
static void size_allocate_title_buttons (BtkCList *clist);
static void size_allocate_columns       (BtkCList *clist,
					 gboolean  block_resize);
static gint list_requisition_width      (BtkCList *clist);

/* Memory Allocation/Distruction Routines */
static BtkCListColumn *columns_new (BtkCList      *clist);
static void column_title_new       (BtkCList      *clist,
			            gint           column,
			            const gchar   *title);
static void columns_delete         (BtkCList      *clist);
static BtkCListRow *row_new        (BtkCList      *clist);
static void row_delete             (BtkCList      *clist,
			            BtkCListRow   *clist_row);
static void set_cell_contents      (BtkCList      *clist,
			            BtkCListRow   *clist_row,
				    gint           column,
				    BtkCellType    type,
				    const gchar   *text,
				    guint8         spacing,
				    BdkPixmap     *pixmap,
				    BdkBitmap     *mask);
static gint real_insert_row        (BtkCList      *clist,
				    gint           row,
				    gchar         *text[]);
static void real_remove_row        (BtkCList      *clist,
				    gint           row);
static void real_clear             (BtkCList      *clist);

/* Sorting */
static gint default_compare        (BtkCList      *clist,
			            gconstpointer  row1,
			            gconstpointer  row2);
static void real_sort_list         (BtkCList      *clist);
static GList *btk_clist_merge      (BtkCList      *clist,
				    GList         *a,
				    GList         *b);
static GList *btk_clist_mergesort  (BtkCList      *clist,
				    GList         *list,
				    gint           num);
/* Misc */
static gboolean title_focus_in   (BtkCList *clist,
				  gint      dir);
static gboolean title_focus_move (BtkCList *clist,
				  gint      dir);

static void real_row_move             (BtkCList  *clist,
			               gint       source_row,
			               gint       dest_row);
static gint column_title_passive_func (BtkWidget *widget, 
				       BdkEvent  *event,
				       gpointer   data);
static void drag_dest_cell            (BtkCList         *clist,
				       gint              x,
				       gint              y,
				       BtkCListDestInfo *dest_info);



static BtkContainerClass *parent_class = NULL;
static guint clist_signals[LAST_SIGNAL] = {0};

static const BtkTargetEntry clist_target_table = { "btk-clist-drag-reorder", 0, 0};

BtkType
btk_clist_get_type (void)
{
  static BtkType clist_type = 0;

  if (!clist_type)
    {
      static const BtkTypeInfo clist_info =
      {
	"BtkCList",
	sizeof (BtkCList),
	sizeof (BtkCListClass),
	(BtkClassInitFunc) btk_clist_class_init,
	(BtkObjectInitFunc) btk_clist_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(BtkClassInitFunc) NULL,
      };

      I_("BtkCList");
      clist_type = btk_type_unique (BTK_TYPE_CONTAINER, &clist_info);
    }

  return clist_type;
}

static void
btk_clist_class_init (BtkCListClass *klass)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (klass);
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;
  BtkContainerClass *container_class;
  BtkBindingSet *binding_set;

  bobject_class->constructor = btk_clist_constructor;

  object_class = (BtkObjectClass *) klass;
  widget_class = (BtkWidgetClass *) klass;
  container_class = (BtkContainerClass *) klass;

  parent_class = btk_type_class (BTK_TYPE_CONTAINER);

  bobject_class->finalize = btk_clist_finalize;
  
  object_class->set_arg = btk_clist_set_arg;
  object_class->get_arg = btk_clist_get_arg;
  object_class->destroy = btk_clist_destroy;
  

  widget_class->realize = btk_clist_realize;
  widget_class->unrealize = btk_clist_unrealize;
  widget_class->map = btk_clist_map;
  widget_class->unmap = btk_clist_unmap;
  widget_class->button_press_event = btk_clist_button_press;
  widget_class->button_release_event = btk_clist_button_release;
  widget_class->motion_notify_event = btk_clist_motion;
  widget_class->expose_event = btk_clist_expose;
  widget_class->size_request = btk_clist_size_request;
  widget_class->size_allocate = btk_clist_size_allocate;
  widget_class->focus_in_event = btk_clist_focus_in;
  widget_class->focus_out_event = btk_clist_focus_out;
  widget_class->style_set = btk_clist_style_set;
  widget_class->drag_begin = btk_clist_drag_begin;
  widget_class->drag_end = btk_clist_drag_end;
  widget_class->drag_motion = btk_clist_drag_motion;
  widget_class->drag_leave = btk_clist_drag_leave;
  widget_class->drag_drop = btk_clist_drag_drop;
  widget_class->drag_data_get = btk_clist_drag_data_get;
  widget_class->drag_data_received = btk_clist_drag_data_received;
  widget_class->focus = btk_clist_focus;
  
  /* container_class->add = NULL; use the default BtkContainerClass warning */
  /* container_class->remove=NULL; use the default BtkContainerClass warning */

  container_class->forall = btk_clist_forall;
  container_class->set_focus_child = btk_clist_set_focus_child;

  klass->set_scroll_adjustments = btk_clist_set_scroll_adjustments;
  klass->refresh = clist_refresh;
  klass->select_row = real_select_row;
  klass->unselect_row = real_unselect_row;
  klass->row_move = real_row_move;
  klass->undo_selection = real_undo_selection;
  klass->resync_selection = resync_selection;
  klass->selection_find = selection_find;
  klass->click_column = NULL;
  klass->resize_column = real_resize_column;
  klass->draw_row = draw_row;
  klass->draw_drag_highlight = draw_drag_highlight;
  klass->insert_row = real_insert_row;
  klass->remove_row = real_remove_row;
  klass->clear = real_clear;
  klass->sort_list = real_sort_list;
  klass->select_all = real_select_all;
  klass->unselect_all = real_unselect_all;
  klass->fake_unselect_all = fake_unselect_all;
  klass->scroll_horizontal = scroll_horizontal;
  klass->scroll_vertical = scroll_vertical;
  klass->extend_selection = extend_selection;
  klass->toggle_focus_row = toggle_focus_row;
  klass->toggle_add_mode = toggle_add_mode;
  klass->start_selection = start_selection;
  klass->end_selection = end_selection;
  klass->abort_column_resize = abort_column_resize;
  klass->set_cell_contents = set_cell_contents;
  klass->cell_size_request = cell_size_request;

  btk_object_add_arg_type ("BtkCList::n-columns",
			   BTK_TYPE_UINT,
			   BTK_ARG_READWRITE | BTK_ARG_CONSTRUCT_ONLY | G_PARAM_STATIC_NAME,
			   ARG_N_COLUMNS);
  btk_object_add_arg_type ("BtkCList::shadow-type",
			   BTK_TYPE_SHADOW_TYPE,
			   BTK_ARG_READWRITE | G_PARAM_STATIC_NAME,
			   ARG_SHADOW_TYPE);
  btk_object_add_arg_type ("BtkCList::selection-mode",
			   BTK_TYPE_SELECTION_MODE,
			   BTK_ARG_READWRITE | G_PARAM_STATIC_NAME,
			   ARG_SELECTION_MODE);
  btk_object_add_arg_type ("BtkCList::row-height",
			   BTK_TYPE_UINT,
			   BTK_ARG_READWRITE | G_PARAM_STATIC_NAME,
			   ARG_ROW_HEIGHT);
  btk_object_add_arg_type ("BtkCList::reorderable",
			   BTK_TYPE_BOOL,
			   BTK_ARG_READWRITE | G_PARAM_STATIC_NAME,
			   ARG_REORDERABLE);
  btk_object_add_arg_type ("BtkCList::titles-active",
			   BTK_TYPE_BOOL,
			   BTK_ARG_READWRITE | G_PARAM_STATIC_NAME,
			   ARG_TITLES_ACTIVE);
  btk_object_add_arg_type ("BtkCList::use-drag-icons",
			   BTK_TYPE_BOOL,
			   BTK_ARG_READWRITE | G_PARAM_STATIC_NAME,
			   ARG_USE_DRAG_ICONS);
  btk_object_add_arg_type ("BtkCList::sort-type",
			   BTK_TYPE_SORT_TYPE,
			   BTK_ARG_READWRITE | G_PARAM_STATIC_NAME,
			   ARG_SORT_TYPE);  

  widget_class->set_scroll_adjustments_signal =
    btk_signal_new (I_("set-scroll-adjustments"),
		    BTK_RUN_LAST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkCListClass, set_scroll_adjustments),
		    _btk_marshal_VOID__OBJECT_OBJECT,
		    BTK_TYPE_NONE, 2, BTK_TYPE_ADJUSTMENT, BTK_TYPE_ADJUSTMENT);

  clist_signals[SELECT_ROW] =
    btk_signal_new (I_("select-row"),
		    BTK_RUN_FIRST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkCListClass, select_row),
		    _btk_marshal_VOID__INT_INT_BOXED,
		    BTK_TYPE_NONE, 3,
		    BTK_TYPE_INT,
		    BTK_TYPE_INT,
		    BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  clist_signals[UNSELECT_ROW] =
    btk_signal_new (I_("unselect-row"),
		    BTK_RUN_FIRST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkCListClass, unselect_row),
		    _btk_marshal_VOID__INT_INT_BOXED,
		    BTK_TYPE_NONE, 3, BTK_TYPE_INT,
		    BTK_TYPE_INT, BDK_TYPE_EVENT);
  clist_signals[ROW_MOVE] =
    btk_signal_new (I_("row-move"),
		    BTK_RUN_LAST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkCListClass, row_move),
		    _btk_marshal_VOID__INT_INT,
		    BTK_TYPE_NONE, 2, BTK_TYPE_INT, BTK_TYPE_INT);
  clist_signals[CLICK_COLUMN] =
    btk_signal_new (I_("click-column"),
		    BTK_RUN_FIRST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkCListClass, click_column),
		    _btk_marshal_VOID__INT,
		    BTK_TYPE_NONE, 1, BTK_TYPE_INT);
  clist_signals[RESIZE_COLUMN] =
    btk_signal_new (I_("resize-column"),
		    BTK_RUN_LAST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkCListClass, resize_column),
		    _btk_marshal_VOID__INT_INT,
		    BTK_TYPE_NONE, 2, BTK_TYPE_INT, BTK_TYPE_INT);

  clist_signals[TOGGLE_FOCUS_ROW] =
    btk_signal_new (I_("toggle-focus-row"),
                    BTK_RUN_LAST | BTK_RUN_ACTION,
                    BTK_CLASS_TYPE (object_class),
                    BTK_SIGNAL_OFFSET (BtkCListClass, toggle_focus_row),
                    _btk_marshal_VOID__VOID,
                    BTK_TYPE_NONE, 0);
  clist_signals[SELECT_ALL] =
    btk_signal_new (I_("select-all"),
                    BTK_RUN_LAST | BTK_RUN_ACTION,
                    BTK_CLASS_TYPE (object_class),
                    BTK_SIGNAL_OFFSET (BtkCListClass, select_all),
                    _btk_marshal_VOID__VOID,
                    BTK_TYPE_NONE, 0);
  clist_signals[UNSELECT_ALL] =
    btk_signal_new (I_("unselect-all"),
                    BTK_RUN_LAST | BTK_RUN_ACTION,
                    BTK_CLASS_TYPE (object_class),
                    BTK_SIGNAL_OFFSET (BtkCListClass, unselect_all),
                    _btk_marshal_VOID__VOID,
                    BTK_TYPE_NONE, 0);
  clist_signals[UNDO_SELECTION] =
    btk_signal_new (I_("undo-selection"),
		    BTK_RUN_LAST | BTK_RUN_ACTION,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkCListClass, undo_selection),
		    _btk_marshal_VOID__VOID,
		    BTK_TYPE_NONE, 0);
  clist_signals[START_SELECTION] =
    btk_signal_new (I_("start-selection"),
		    BTK_RUN_LAST | BTK_RUN_ACTION,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkCListClass, start_selection),
		    _btk_marshal_VOID__VOID,
		    BTK_TYPE_NONE, 0);
  clist_signals[END_SELECTION] =
    btk_signal_new (I_("end-selection"),
		    BTK_RUN_LAST | BTK_RUN_ACTION,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkCListClass, end_selection),
		    _btk_marshal_VOID__VOID,
		    BTK_TYPE_NONE, 0);
  clist_signals[TOGGLE_ADD_MODE] =
    btk_signal_new (I_("toggle-add-mode"),
		    BTK_RUN_LAST | BTK_RUN_ACTION,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkCListClass, toggle_add_mode),
		    _btk_marshal_VOID__VOID,
		    BTK_TYPE_NONE, 0);
  clist_signals[EXTEND_SELECTION] =
    btk_signal_new (I_("extend-selection"),
                    BTK_RUN_LAST | BTK_RUN_ACTION,
                    BTK_CLASS_TYPE (object_class),
                    BTK_SIGNAL_OFFSET (BtkCListClass, extend_selection),
                    _btk_marshal_VOID__ENUM_FLOAT_BOOLEAN,
                    BTK_TYPE_NONE, 3,
		    BTK_TYPE_SCROLL_TYPE, BTK_TYPE_FLOAT, BTK_TYPE_BOOL);
  clist_signals[SCROLL_VERTICAL] =
    btk_signal_new (I_("scroll-vertical"),
                    BTK_RUN_LAST | BTK_RUN_ACTION,
                    BTK_CLASS_TYPE (object_class),
                    BTK_SIGNAL_OFFSET (BtkCListClass, scroll_vertical),
                    _btk_marshal_VOID__ENUM_FLOAT,
                    BTK_TYPE_NONE, 2, BTK_TYPE_SCROLL_TYPE, BTK_TYPE_FLOAT);
  clist_signals[SCROLL_HORIZONTAL] =
    btk_signal_new (I_("scroll-horizontal"),
                    BTK_RUN_LAST | BTK_RUN_ACTION,
                    BTK_CLASS_TYPE (object_class),
                    BTK_SIGNAL_OFFSET (BtkCListClass, scroll_horizontal),
                    _btk_marshal_VOID__ENUM_FLOAT,
                    BTK_TYPE_NONE, 2, BTK_TYPE_SCROLL_TYPE, BTK_TYPE_FLOAT);
  clist_signals[ABORT_COLUMN_RESIZE] =
    btk_signal_new (I_("abort-column-resize"),
                    BTK_RUN_LAST | BTK_RUN_ACTION,
                    BTK_CLASS_TYPE (object_class),
                    BTK_SIGNAL_OFFSET (BtkCListClass, abort_column_resize),
                    _btk_marshal_VOID__VOID,
                    BTK_TYPE_NONE, 0);

  binding_set = btk_binding_set_by_class (klass);
  btk_binding_entry_add_signal (binding_set, BDK_Up, 0,
			        "scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_BACKWARD,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Up, 0,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_BACKWARD,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_Down, 0,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_FORWARD,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Down, 0,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_FORWARD,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_Page_Up, 0,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_PAGE_BACKWARD,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Page_Up, 0,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_PAGE_BACKWARD,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_Page_Down, 0,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_PAGE_FORWARD,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Page_Down, 0,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_PAGE_FORWARD,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_Home, BDK_CONTROL_MASK,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Home, BDK_CONTROL_MASK,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_End, BDK_CONTROL_MASK,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 1.0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_End, BDK_CONTROL_MASK,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 1.0);
  
  btk_binding_entry_add_signal (binding_set, BDK_Up, BDK_SHIFT_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_BACKWARD,
				BTK_TYPE_FLOAT, 0.0, BTK_TYPE_BOOL, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Up, BDK_SHIFT_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_BACKWARD,
				BTK_TYPE_FLOAT, 0.0, BTK_TYPE_BOOL, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_Down, BDK_SHIFT_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_FORWARD,
				BTK_TYPE_FLOAT, 0.0, BTK_TYPE_BOOL, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Down, BDK_SHIFT_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_FORWARD,
				BTK_TYPE_FLOAT, 0.0, BTK_TYPE_BOOL, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_Page_Up, BDK_SHIFT_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_PAGE_BACKWARD,
				BTK_TYPE_FLOAT, 0.0, BTK_TYPE_BOOL, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Page_Up, BDK_SHIFT_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_PAGE_BACKWARD,
				BTK_TYPE_FLOAT, 0.0, BTK_TYPE_BOOL, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_Page_Down, BDK_SHIFT_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_PAGE_FORWARD,
				BTK_TYPE_FLOAT, 0.0, BTK_TYPE_BOOL, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Page_Down, BDK_SHIFT_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_PAGE_FORWARD,
				BTK_TYPE_FLOAT, 0.0, BTK_TYPE_BOOL, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_Home,
				BDK_SHIFT_MASK | BDK_CONTROL_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 0.0, BTK_TYPE_BOOL, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Home,
                                BDK_SHIFT_MASK | BDK_CONTROL_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 0.0, BTK_TYPE_BOOL, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_End,
				BDK_SHIFT_MASK | BDK_CONTROL_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 1.0, BTK_TYPE_BOOL, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_End,
				BDK_SHIFT_MASK | BDK_CONTROL_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 1.0, BTK_TYPE_BOOL, TRUE);

  
  btk_binding_entry_add_signal (binding_set, BDK_Left, 0,
				"scroll-horizontal", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_BACKWARD,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Left, 0,
				"scroll-horizontal", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_BACKWARD,
				BTK_TYPE_FLOAT, 0.0);
  
  btk_binding_entry_add_signal (binding_set, BDK_Right, 0,
				"scroll-horizontal", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_FORWARD,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Right, 0,
				"scroll-horizontal", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_FORWARD,
				BTK_TYPE_FLOAT, 0.0);

  btk_binding_entry_add_signal (binding_set, BDK_Home, 0,
				"scroll-horizontal", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Home, 0,
				"scroll-horizontal", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 0.0);
  
  btk_binding_entry_add_signal (binding_set, BDK_End, 0,
				"scroll-horizontal", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 1.0);

  btk_binding_entry_add_signal (binding_set, BDK_KP_End, 0,
				"scroll-horizontal", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 1.0);
  
  btk_binding_entry_add_signal (binding_set, BDK_Escape, 0,
				"undo-selection", 0);
  btk_binding_entry_add_signal (binding_set, BDK_Escape, 0,
				"abort-column-resize", 0);
  btk_binding_entry_add_signal (binding_set, BDK_space, 0,
				"toggle-focus-row", 0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Space, 0,
				"toggle-focus-row", 0);  
  btk_binding_entry_add_signal (binding_set, BDK_space, BDK_CONTROL_MASK,
				"toggle-add-mode", 0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Space, BDK_CONTROL_MASK,
				"toggle-add-mode", 0);
  btk_binding_entry_add_signal (binding_set, BDK_slash, BDK_CONTROL_MASK,
				"select-all", 0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Divide, BDK_CONTROL_MASK,
				"select-all", 0);
  btk_binding_entry_add_signal (binding_set, '\\', BDK_CONTROL_MASK,
				"unselect-all", 0);
  btk_binding_entry_add_signal (binding_set, BDK_Shift_L,
				BDK_RELEASE_MASK | BDK_SHIFT_MASK,
				"end-selection", 0);
  btk_binding_entry_add_signal (binding_set, BDK_Shift_R,
				BDK_RELEASE_MASK | BDK_SHIFT_MASK,
				"end-selection", 0);
  btk_binding_entry_add_signal (binding_set, BDK_Shift_L,
				BDK_RELEASE_MASK | BDK_SHIFT_MASK |
				BDK_CONTROL_MASK,
				"end-selection", 0);
  btk_binding_entry_add_signal (binding_set, BDK_Shift_R,
				BDK_RELEASE_MASK | BDK_SHIFT_MASK |
				BDK_CONTROL_MASK,
				"end-selection", 0);
}

static void
btk_clist_set_arg (BtkObject      *object,
		   BtkArg         *arg,
		   guint           arg_id)
{
  BtkCList *clist;

  clist = BTK_CLIST (object);

  switch (arg_id)
    {
    case ARG_N_COLUMNS: /* only set at construction time */
      clist->columns = MAX (1, BTK_VALUE_UINT (*arg));
      break;
    case ARG_SHADOW_TYPE:
      btk_clist_set_shadow_type (clist, BTK_VALUE_ENUM (*arg));
      break;
    case ARG_SELECTION_MODE:
      btk_clist_set_selection_mode (clist, BTK_VALUE_ENUM (*arg));
      break;
    case ARG_ROW_HEIGHT:
      btk_clist_set_row_height (clist, BTK_VALUE_UINT (*arg));
      break;
    case ARG_REORDERABLE:
      btk_clist_set_reorderable (clist, BTK_VALUE_BOOL (*arg));
      break;
    case ARG_TITLES_ACTIVE:
      if (BTK_VALUE_BOOL (*arg))
	btk_clist_column_titles_active (clist);
      else
	btk_clist_column_titles_passive (clist);
      break;
    case ARG_USE_DRAG_ICONS:
      btk_clist_set_use_drag_icons (clist, BTK_VALUE_BOOL (*arg));
      break;
    case ARG_SORT_TYPE:
      btk_clist_set_sort_type (clist, BTK_VALUE_ENUM (*arg));
      break;
    }
}

static void
btk_clist_get_arg (BtkObject      *object,
		   BtkArg         *arg,
		   guint           arg_id)
{
  BtkCList *clist;

  clist = BTK_CLIST (object);

  switch (arg_id)
    {
      guint i;

    case ARG_N_COLUMNS:
      BTK_VALUE_UINT (*arg) = clist->columns;
      break;
    case ARG_SHADOW_TYPE:
      BTK_VALUE_ENUM (*arg) = clist->shadow_type;
      break;
    case ARG_SELECTION_MODE:
      BTK_VALUE_ENUM (*arg) = clist->selection_mode;
      break;
    case ARG_ROW_HEIGHT:
      BTK_VALUE_UINT (*arg) = BTK_CLIST_ROW_HEIGHT_SET(clist) ? clist->row_height : 0;
      break;
    case ARG_REORDERABLE:
      BTK_VALUE_BOOL (*arg) = BTK_CLIST_REORDERABLE (clist);
      break;
    case ARG_TITLES_ACTIVE:
      BTK_VALUE_BOOL (*arg) = TRUE;
      for (i = 0; i < clist->columns; i++)
	if (clist->column[i].button &&
	    !btk_widget_get_sensitive (clist->column[i].button))
	  {
	    BTK_VALUE_BOOL (*arg) = FALSE;
	    break;
	  }
      break;
    case ARG_USE_DRAG_ICONS:
      BTK_VALUE_BOOL (*arg) = BTK_CLIST_USE_DRAG_ICONS (clist);
      break;
    case ARG_SORT_TYPE:
      BTK_VALUE_ENUM (*arg) = clist->sort_type;
      break;
    default:
      arg->type = BTK_TYPE_INVALID;
      break;
    }
}

static void
btk_clist_init (BtkCList *clist)
{
  clist->flags = 0;

  btk_widget_set_has_window (BTK_WIDGET (clist), TRUE);
  btk_widget_set_can_focus (BTK_WIDGET (clist), TRUE);
  BTK_CLIST_SET_FLAG (clist, CLIST_DRAW_DRAG_LINE);
  BTK_CLIST_SET_FLAG (clist, CLIST_USE_DRAG_ICONS);

  clist->freeze_count = 0;

  clist->rows = 0;
  clist->row_height = 0;
  clist->row_list = NULL;
  clist->row_list_end = NULL;

  clist->columns = 0;

  clist->title_window = NULL;
  clist->column_title_area.x = 0;
  clist->column_title_area.y = 0;
  clist->column_title_area.width = 1;
  clist->column_title_area.height = 1;

  clist->clist_window = NULL;
  clist->clist_window_width = 1;
  clist->clist_window_height = 1;

  clist->hoffset = 0;
  clist->voffset = 0;

  clist->shadow_type = BTK_SHADOW_IN;
  clist->vadjustment = NULL;
  clist->hadjustment = NULL;

  clist->button_actions[0] = BTK_BUTTON_SELECTS | BTK_BUTTON_DRAGS;
  clist->button_actions[1] = BTK_BUTTON_IGNORED;
  clist->button_actions[2] = BTK_BUTTON_IGNORED;
  clist->button_actions[3] = BTK_BUTTON_IGNORED;
  clist->button_actions[4] = BTK_BUTTON_IGNORED;

  clist->cursor_drag = NULL;
  clist->xor_gc = NULL;
  clist->fg_gc = NULL;
  clist->bg_gc = NULL;
  clist->x_drag = 0;

  clist->selection_mode = BTK_SELECTION_SINGLE;
  clist->selection = NULL;
  clist->selection_end = NULL;
  clist->undo_selection = NULL;
  clist->undo_unselection = NULL;

  clist->focus_row = -1;
  clist->focus_header_column = -1;
  clist->undo_anchor = -1;

  clist->anchor = -1;
  clist->anchor_state = BTK_STATE_SELECTED;
  clist->drag_pos = -1;
  clist->htimer = 0;
  clist->vtimer = 0;

  clist->click_cell.row = -1;
  clist->click_cell.column = -1;

  clist->compare = default_compare;
  clist->sort_type = BTK_SORT_ASCENDING;
  clist->sort_column = 0;

  clist->drag_highlight_row = -1;
}

/* Constructor */
static GObject*
btk_clist_constructor (GType                  type,
		       guint                  n_construct_properties,
		       GObjectConstructParam *construct_properties)
{
  GObject *object = G_OBJECT_CLASS (parent_class)->constructor (type,
								n_construct_properties,
								construct_properties);
  BtkCList *clist = BTK_CLIST (object);
  
  /* allocate memory for columns */
  clist->column = columns_new (clist);
  
  /* there needs to be at least one column button 
   * because there is alot of code that will break if it
   * isn't there
   */
  column_button_create (clist, 0);
  
  return object;
}

/* BTKCLIST PUBLIC INTERFACE
 *   btk_clist_new
 *   btk_clist_new_with_titles
 *   btk_clist_set_hadjustment
 *   btk_clist_set_vadjustment
 *   btk_clist_get_hadjustment
 *   btk_clist_get_vadjustment
 *   btk_clist_set_shadow_type
 *   btk_clist_set_selection_mode
 *   btk_clist_freeze
 *   btk_clist_thaw
 */
BtkWidget*
btk_clist_new (gint columns)
{
  return btk_clist_new_with_titles (columns, NULL);
}
 
BtkWidget*
btk_clist_new_with_titles (gint   columns,
			   gchar *titles[])
{
  BtkCList *clist;

  clist = g_object_new (BTK_TYPE_CLIST,
			"n_columns", columns,
			NULL);
  if (titles)
    {
      guint i;

      for (i = 0; i < clist->columns; i++)
	btk_clist_set_column_title (clist, i, titles[i]);
      btk_clist_column_titles_show (clist);
    }
  else
    btk_clist_column_titles_hide (clist);
  
  return BTK_WIDGET (clist);
}

void
btk_clist_set_hadjustment (BtkCList      *clist,
			   BtkAdjustment *adjustment)
{
  BtkAdjustment *old_adjustment;

  g_return_if_fail (BTK_IS_CLIST (clist));
  if (adjustment)
    g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));
  
  if (clist->hadjustment == adjustment)
    return;
  
  old_adjustment = clist->hadjustment;

  if (clist->hadjustment)
    {
      btk_signal_disconnect_by_data (BTK_OBJECT (clist->hadjustment), clist);
      g_object_unref (clist->hadjustment);
    }

  clist->hadjustment = adjustment;

  if (clist->hadjustment)
    {
      g_object_ref_sink (clist->hadjustment);

      btk_signal_connect (BTK_OBJECT (clist->hadjustment), "changed",
			  G_CALLBACK (hadjustment_changed),
			  (gpointer) clist);
      btk_signal_connect (BTK_OBJECT (clist->hadjustment), "value-changed",
			  G_CALLBACK (hadjustment_value_changed),
			  (gpointer) clist);
    }

  if (!clist->hadjustment || !old_adjustment)
    btk_widget_queue_resize (BTK_WIDGET (clist));
}

BtkAdjustment *
btk_clist_get_hadjustment (BtkCList *clist)
{
  g_return_val_if_fail (BTK_IS_CLIST (clist), NULL);

  return clist->hadjustment;
}

void
btk_clist_set_vadjustment (BtkCList      *clist,
			   BtkAdjustment *adjustment)
{
  BtkAdjustment *old_adjustment;

  g_return_if_fail (BTK_IS_CLIST (clist));
  if (adjustment)
    g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));

  if (clist->vadjustment == adjustment)
    return;
  
  old_adjustment = clist->vadjustment;

  if (clist->vadjustment)
    {
      btk_signal_disconnect_by_data (BTK_OBJECT (clist->vadjustment), clist);
      g_object_unref (clist->vadjustment);
    }

  clist->vadjustment = adjustment;

  if (clist->vadjustment)
    {
      g_object_ref_sink (clist->vadjustment);

      btk_signal_connect (BTK_OBJECT (clist->vadjustment), "changed",
			  G_CALLBACK (vadjustment_changed),
			  (gpointer) clist);
      btk_signal_connect (BTK_OBJECT (clist->vadjustment), "value-changed",
			  G_CALLBACK (vadjustment_value_changed),
			  (gpointer) clist);
    }

  if (!clist->vadjustment || !old_adjustment)
    btk_widget_queue_resize (BTK_WIDGET (clist));
}

BtkAdjustment *
btk_clist_get_vadjustment (BtkCList *clist)
{
  g_return_val_if_fail (BTK_IS_CLIST (clist), NULL);

  return clist->vadjustment;
}

static void
btk_clist_set_scroll_adjustments (BtkCList      *clist,
				  BtkAdjustment *hadjustment,
				  BtkAdjustment *vadjustment)
{
  if (clist->hadjustment != hadjustment)
    btk_clist_set_hadjustment (clist, hadjustment);
  if (clist->vadjustment != vadjustment)
    btk_clist_set_vadjustment (clist, vadjustment);
}

void
btk_clist_set_shadow_type (BtkCList      *clist,
			   BtkShadowType  type)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  clist->shadow_type = type;

  if (btk_widget_get_visible (BTK_WIDGET (clist)))
    btk_widget_queue_resize (BTK_WIDGET (clist));
}

void
btk_clist_set_selection_mode (BtkCList         *clist,
			      BtkSelectionMode  mode)
{
  g_return_if_fail (BTK_IS_CLIST (clist));
  g_return_if_fail (mode != BTK_SELECTION_NONE);

  if (mode == clist->selection_mode)
    return;

  clist->selection_mode = mode;
  clist->anchor = -1;
  clist->anchor_state = BTK_STATE_SELECTED;
  clist->drag_pos = -1;
  clist->undo_anchor = clist->focus_row;

  g_list_free (clist->undo_selection);
  g_list_free (clist->undo_unselection);
  clist->undo_selection = NULL;
  clist->undo_unselection = NULL;

  switch (mode)
    {
    case BTK_SELECTION_MULTIPLE:
      return;
    case BTK_SELECTION_BROWSE:
    case BTK_SELECTION_SINGLE:
      btk_clist_unselect_all (clist);
      break;
    default:
      /* Someone set it by hand */
      g_assert_not_reached ();
    }
}

void
btk_clist_freeze (BtkCList *clist)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  clist->freeze_count++;
}

void
btk_clist_thaw (BtkCList *clist)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (clist->freeze_count)
    {
      clist->freeze_count--;
      CLIST_REFRESH (clist);
    }
}

/* PUBLIC COLUMN FUNCTIONS
 *   btk_clist_column_titles_show
 *   btk_clist_column_titles_hide
 *   btk_clist_column_title_active
 *   btk_clist_column_title_passive
 *   btk_clist_column_titles_active
 *   btk_clist_column_titles_passive
 *   btk_clist_set_column_title
 *   btk_clist_get_column_title
 *   btk_clist_set_column_widget
 *   btk_clist_set_column_justification
 *   btk_clist_set_column_visibility
 *   btk_clist_set_column_resizeable
 *   btk_clist_set_column_auto_resize
 *   btk_clist_optimal_column_width
 *   btk_clist_set_column_width
 *   btk_clist_set_column_min_width
 *   btk_clist_set_column_max_width
 */
void
btk_clist_column_titles_show (BtkCList *clist)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (!BTK_CLIST_SHOW_TITLES(clist))
    {
      BTK_CLIST_SET_FLAG (clist, CLIST_SHOW_TITLES);
      if (clist->title_window)
	bdk_window_show (clist->title_window);
      btk_widget_queue_resize (BTK_WIDGET (clist));
    }
}

void 
btk_clist_column_titles_hide (BtkCList *clist)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (BTK_CLIST_SHOW_TITLES(clist))
    {
      BTK_CLIST_UNSET_FLAG (clist, CLIST_SHOW_TITLES);
      if (clist->title_window)
	bdk_window_hide (clist->title_window);
      btk_widget_queue_resize (BTK_WIDGET (clist));
    }
}

void
btk_clist_column_title_active (BtkCList *clist,
			       gint      column)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (column < 0 || column >= clist->columns)
    return;
  if (!clist->column[column].button || !clist->column[column].button_passive)
    return;

  clist->column[column].button_passive = FALSE;

  btk_signal_disconnect_by_func (BTK_OBJECT (clist->column[column].button),
				 G_CALLBACK (column_title_passive_func),
				 NULL);

  btk_widget_set_can_focus (clist->column[column].button, TRUE);
  if (btk_widget_get_visible (BTK_WIDGET (clist)))
    btk_widget_queue_draw (clist->column[column].button);
}

void
btk_clist_column_title_passive (BtkCList *clist,
				gint      column)
{
  BtkButton *button;

  g_return_if_fail (BTK_IS_CLIST (clist));

  if (column < 0 || column >= clist->columns)
    return;
  if (!clist->column[column].button || clist->column[column].button_passive)
    return;

  button = BTK_BUTTON (clist->column[column].button);

  clist->column[column].button_passive = TRUE;

  if (button->button_down)
    g_signal_emit_by_name (button, "released");
  if (button->in_button)
    g_signal_emit_by_name (button, "leave");

  btk_signal_connect (BTK_OBJECT (clist->column[column].button), "event",
		      G_CALLBACK (column_title_passive_func),
                      NULL);

  btk_widget_set_can_focus (clist->column[column].button, FALSE);
  if (btk_widget_get_visible (BTK_WIDGET (clist)))
    btk_widget_queue_draw (clist->column[column].button);
}

void
btk_clist_column_titles_active (BtkCList *clist)
{
  gint i;

  g_return_if_fail (BTK_IS_CLIST (clist));

  for (i = 0; i < clist->columns; i++)
    btk_clist_column_title_active (clist, i);
}

void
btk_clist_column_titles_passive (BtkCList *clist)
{
  gint i;

  g_return_if_fail (BTK_IS_CLIST (clist));

  for (i = 0; i < clist->columns; i++)
    btk_clist_column_title_passive (clist, i);
}

void
btk_clist_set_column_title (BtkCList    *clist,
			    gint         column,
			    const gchar *title)
{
  gint new_button = 0;
  BtkWidget *old_widget;
  BtkWidget *alignment = NULL;
  BtkWidget *label;

  g_return_if_fail (BTK_IS_CLIST (clist));

  if (column < 0 || column >= clist->columns)
    return;

  /* if the column button doesn't currently exist,
   * it has to be created first */
  if (!clist->column[column].button)
    {
      column_button_create (clist, column);
      new_button = 1;
    }

  column_title_new (clist, column, title);

  /* remove and destroy the old widget */
  old_widget = BTK_BIN (clist->column[column].button)->child;
  if (old_widget)
    btk_container_remove (BTK_CONTAINER (clist->column[column].button), old_widget);

  /* create new alignment based no column justification */
  switch (clist->column[column].justification)
    {
    case BTK_JUSTIFY_LEFT:
      alignment = btk_alignment_new (0.0, 0.5, 0.0, 0.0);
      break;

    case BTK_JUSTIFY_RIGHT:
      alignment = btk_alignment_new (1.0, 0.5, 0.0, 0.0);
      break;

    case BTK_JUSTIFY_CENTER:
      alignment = btk_alignment_new (0.5, 0.5, 0.0, 0.0);
      break;

    case BTK_JUSTIFY_FILL:
      alignment = btk_alignment_new (0.5, 0.5, 0.0, 0.0);
      break;
    }

  btk_widget_push_composite_child ();
  label = btk_label_new (clist->column[column].title);
  btk_widget_pop_composite_child ();
  btk_container_add (BTK_CONTAINER (alignment), label);
  btk_container_add (BTK_CONTAINER (clist->column[column].button), alignment);
  btk_widget_show (label);
  btk_widget_show (alignment);

  /* if this button didn't previously exist, then the
   * column button positions have to be re-computed */
  if (btk_widget_get_visible (BTK_WIDGET (clist)) && new_button)
    size_allocate_title_buttons (clist);
}

gchar *
btk_clist_get_column_title (BtkCList *clist,
			    gint      column)
{
  g_return_val_if_fail (BTK_IS_CLIST (clist), NULL);

  if (column < 0 || column >= clist->columns)
    return NULL;

  return clist->column[column].title;
}

void
btk_clist_set_column_widget (BtkCList  *clist,
			     gint       column,
			     BtkWidget *widget)
{
  gint new_button = 0;
  BtkWidget *old_widget;

  g_return_if_fail (BTK_IS_CLIST (clist));

  if (column < 0 || column >= clist->columns)
    return;

  /* if the column button doesn't currently exist,
   * it has to be created first */
  if (!clist->column[column].button)
    {
      column_button_create (clist, column);
      new_button = 1;
    }

  column_title_new (clist, column, NULL);

  /* remove and destroy the old widget */
  old_widget = BTK_BIN (clist->column[column].button)->child;
  if (old_widget)
    btk_container_remove (BTK_CONTAINER (clist->column[column].button),
			  old_widget);

  /* add and show the widget */
  if (widget)
    {
      btk_container_add (BTK_CONTAINER (clist->column[column].button), widget);
      btk_widget_show (widget);
    }

  /* if this button didn't previously exist, then the
   * column button positions have to be re-computed */
  if (btk_widget_get_visible (BTK_WIDGET (clist)) && new_button)
    size_allocate_title_buttons (clist);
}

BtkWidget *
btk_clist_get_column_widget (BtkCList *clist,
			     gint      column)
{
  g_return_val_if_fail (BTK_IS_CLIST (clist), NULL);

  if (column < 0 || column >= clist->columns)
    return NULL;

  if (clist->column[column].button)
    return BTK_BIN (clist->column[column].button)->child;

  return NULL;
}

void
btk_clist_set_column_justification (BtkCList         *clist,
				    gint              column,
				    BtkJustification  justification)
{
  BtkWidget *alignment;

  g_return_if_fail (BTK_IS_CLIST (clist));

  if (column < 0 || column >= clist->columns)
    return;

  clist->column[column].justification = justification;

  /* change the alinment of the button title if it's not a
   * custom widget */
  if (clist->column[column].title)
    {
      alignment = BTK_BIN (clist->column[column].button)->child;

      switch (clist->column[column].justification)
	{
	case BTK_JUSTIFY_LEFT:
	  btk_alignment_set (BTK_ALIGNMENT (alignment), 0.0, 0.5, 0.0, 0.0);
	  break;

	case BTK_JUSTIFY_RIGHT:
	  btk_alignment_set (BTK_ALIGNMENT (alignment), 1.0, 0.5, 0.0, 0.0);
	  break;

	case BTK_JUSTIFY_CENTER:
	  btk_alignment_set (BTK_ALIGNMENT (alignment), 0.5, 0.5, 0.0, 0.0);
	  break;

	case BTK_JUSTIFY_FILL:
	  btk_alignment_set (BTK_ALIGNMENT (alignment), 0.5, 0.5, 0.0, 0.0);
	  break;

	default:
	  break;
	}
    }

  if (CLIST_UNFROZEN (clist))
    draw_rows (clist, NULL);
}

void
btk_clist_set_column_visibility (BtkCList *clist,
				 gint      column,
				 gboolean  visible)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (column < 0 || column >= clist->columns)
    return;
  if (clist->column[column].visible == visible)
    return;

  /* don't hide last visible column */
  if (!visible)
    {
      gint i;
      gint vis_columns = 0;

      for (i = 0, vis_columns = 0; i < clist->columns && vis_columns < 2; i++)
	if (clist->column[i].visible)
	  vis_columns++;

      if (vis_columns < 2)
	return;
    }

  clist->column[column].visible = visible;

  if (clist->column[column].button)
    {
      if (visible)
	btk_widget_show (clist->column[column].button);
      else
	btk_widget_hide (clist->column[column].button);
    }
  
  btk_widget_queue_resize (BTK_WIDGET(clist));
}

void
btk_clist_set_column_resizeable (BtkCList *clist,
				 gint      column,
				 gboolean  resizeable)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (column < 0 || column >= clist->columns)
    return;
  if (clist->column[column].resizeable == resizeable)
    return;

  clist->column[column].resizeable = resizeable;
  if (resizeable)
    clist->column[column].auto_resize = FALSE;

  if (btk_widget_get_visible (BTK_WIDGET (clist)))
    size_allocate_title_buttons (clist);
}

void
btk_clist_set_column_auto_resize (BtkCList *clist,
				  gint      column,
				  gboolean  auto_resize)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (column < 0 || column >= clist->columns)
    return;
  if (clist->column[column].auto_resize == auto_resize)
    return;

  clist->column[column].auto_resize = auto_resize;
  if (auto_resize)
    {
      clist->column[column].resizeable = FALSE;
      if (!BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
	{
	  gint width;

	  width = btk_clist_optimal_column_width (clist, column);
	  btk_clist_set_column_width (clist, column, width);
	}
    }

  if (btk_widget_get_visible (BTK_WIDGET (clist)))
    size_allocate_title_buttons (clist);
}

gint
btk_clist_columns_autosize (BtkCList *clist)
{
  gint i;
  gint width;

  g_return_val_if_fail (BTK_IS_CLIST (clist), 0);

  btk_clist_freeze (clist);
  width = 0;
  for (i = 0; i < clist->columns; i++)
    {
      btk_clist_set_column_width (clist, i,
				  btk_clist_optimal_column_width (clist, i));

      width += clist->column[i].width;
    }

  btk_clist_thaw (clist);
  return width;
}

gint
btk_clist_optimal_column_width (BtkCList *clist,
				gint      column)
{
  BtkRequisition requisition;
  GList *list;
  gint width;

  g_return_val_if_fail (BTK_CLIST (clist), 0);

  if (column < 0 || column >= clist->columns)
    return 0;

  if (BTK_CLIST_SHOW_TITLES(clist) && clist->column[column].button)
    width = (clist->column[column].button->requisition.width)
#if 0
	     (CELL_SPACING + (2 * COLUMN_INSET)))
#endif
		;
  else
    width = 0;

  for (list = clist->row_list; list; list = list->next)
    {
  BTK_CLIST_GET_CLASS (clist)->cell_size_request
	(clist, BTK_CLIST_ROW (list), column, &requisition);
      width = MAX (width, requisition.width);
    }

  return width;
}

void
btk_clist_set_column_width (BtkCList *clist,
			    gint      column,
			    gint      width)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (column < 0 || column >= clist->columns)
    return;

  btk_signal_emit (BTK_OBJECT (clist), clist_signals[RESIZE_COLUMN],
		   column, width);
}

void
btk_clist_set_column_min_width (BtkCList *clist,
				gint      column,
				gint      min_width)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (column < 0 || column >= clist->columns)
    return;
  if (clist->column[column].min_width == min_width)
    return;

  if (clist->column[column].max_width >= 0  &&
      clist->column[column].max_width < min_width)
    clist->column[column].min_width = clist->column[column].max_width;
  else
    clist->column[column].min_width = min_width;

  if (clist->column[column].area.width < clist->column[column].min_width)
    btk_clist_set_column_width (clist, column,clist->column[column].min_width);
}

void
btk_clist_set_column_max_width (BtkCList *clist,
				gint      column,
				gint      max_width)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (column < 0 || column >= clist->columns)
    return;
  if (clist->column[column].max_width == max_width)
    return;

  if (clist->column[column].min_width >= 0 && max_width >= 0 &&
      clist->column[column].min_width > max_width)
    clist->column[column].max_width = clist->column[column].min_width;
  else
    clist->column[column].max_width = max_width;
  
  if (clist->column[column].area.width > clist->column[column].max_width)
    btk_clist_set_column_width (clist, column,clist->column[column].max_width);
}

/* PRIVATE COLUMN FUNCTIONS
 *   column_auto_resize
 *   real_resize_column
 *   abort_column_resize
 *   size_allocate_title_buttons
 *   size_allocate_columns
 *   list_requisition_width
 *   new_column_width
 *   column_button_create
 *   column_button_clicked
 *   column_title_passive_func
 */
static void
column_auto_resize (BtkCList    *clist,
		    BtkCListRow *clist_row,
		    gint         column,
		    gint         old_width)
{
  /* resize column if needed for auto_resize */
  BtkRequisition requisition;

  if (!clist->column[column].auto_resize ||
      BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    return;

  if (clist_row)
    BTK_CLIST_GET_CLASS (clist)->cell_size_request (clist, clist_row,
						   column, &requisition);
  else
    requisition.width = 0;

  if (requisition.width > clist->column[column].width)
    btk_clist_set_column_width (clist, column, requisition.width);
  else if (requisition.width < old_width &&
	   old_width == clist->column[column].width)
    {
      GList *list;
      gint new_width = 0;

      /* run a "btk_clist_optimal_column_width" but break, if
       * the column doesn't shrink */
      if (BTK_CLIST_SHOW_TITLES(clist) && clist->column[column].button)
	new_width = (clist->column[column].button->requisition.width -
		     (CELL_SPACING + (2 * COLUMN_INSET)));
      else
	new_width = 0;

      for (list = clist->row_list; list; list = list->next)
	{
	  BTK_CLIST_GET_CLASS (clist)->cell_size_request
	    (clist, BTK_CLIST_ROW (list), column, &requisition);
	  new_width = MAX (new_width, requisition.width);
	  if (new_width == clist->column[column].width)
	    break;
	}
      if (new_width < clist->column[column].width)
	btk_clist_set_column_width
	  (clist, column, MAX (new_width, clist->column[column].min_width));
    }
}

static void
real_resize_column (BtkCList *clist,
		    gint      column,
		    gint      width)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (column < 0 || column >= clist->columns)
    return;
  
  if (width < MAX (COLUMN_MIN_WIDTH, clist->column[column].min_width))
    width = MAX (COLUMN_MIN_WIDTH, clist->column[column].min_width);
  if (clist->column[column].max_width >= 0 &&
      width > clist->column[column].max_width)
    width = clist->column[column].max_width;

  clist->column[column].width = width;
  clist->column[column].width_set = TRUE;

  /* FIXME: this is quite expensive to do if the widget hasn't
   *        been size_allocated yet, and pointless. Should
   *        a flag be kept
   */
  size_allocate_columns (clist, TRUE);
  size_allocate_title_buttons (clist);

  CLIST_REFRESH (clist);
}

static void
abort_column_resize (BtkCList *clist)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (!BTK_CLIST_IN_DRAG(clist))
    return;

  BTK_CLIST_UNSET_FLAG (clist, CLIST_IN_DRAG);
  btk_grab_remove (BTK_WIDGET (clist));
  bdk_display_pointer_ungrab (btk_widget_get_display (BTK_WIDGET (clist)),
			      BDK_CURRENT_TIME);
  clist->drag_pos = -1;

  if (clist->x_drag >= 0 && clist->x_drag <= clist->clist_window_width - 1)
    draw_xor_line (clist);

  if (BTK_CLIST_ADD_MODE(clist))
    {
      gint8 dashes[] = { 4, 4 };

      bdk_gc_set_line_attributes (clist->xor_gc, 1, BDK_LINE_ON_OFF_DASH, 0,0);
      bdk_gc_set_dashes (clist->xor_gc, 0, dashes, G_N_ELEMENTS (dashes));
    }
}

static void
size_allocate_title_buttons (BtkCList *clist)
{
  BtkAllocation button_allocation;
  gint last_column;
  gint last_button = 0;
  gint i;

  if (!btk_widget_get_realized (BTK_WIDGET (clist)))
    return;

  button_allocation.x = clist->hoffset;
  button_allocation.y = 0;
  button_allocation.width = 0;
  button_allocation.height = clist->column_title_area.height;

  /* find last visible column */
  for (last_column = clist->columns - 1; last_column >= 0; last_column--)
    if (clist->column[last_column].visible)
      break;

  for (i = 0; i < last_column; i++)
    {
      if (!clist->column[i].visible)
	{
	  last_button = i + 1;
	  bdk_window_hide (clist->column[i].window);
	  continue;
	}

      button_allocation.width += (clist->column[i].area.width +
				  CELL_SPACING + 2 * COLUMN_INSET);

      if (!clist->column[i + 1].button)
	{
	  bdk_window_hide (clist->column[i].window);
	  continue;
	}

      btk_widget_size_allocate (clist->column[last_button].button,
				&button_allocation);
      button_allocation.x += button_allocation.width;
      button_allocation.width = 0;

      if (clist->column[last_button].resizeable)
	{
	  bdk_window_show (clist->column[last_button].window);
	  bdk_window_move_resize (clist->column[last_button].window,
				  button_allocation.x - (DRAG_WIDTH / 2), 
				  0, DRAG_WIDTH,
				  clist->column_title_area.height);
	}
      else
	bdk_window_hide (clist->column[last_button].window);

      last_button = i + 1;
    }

  button_allocation.width += (clist->column[last_column].area.width +
			      2 * (CELL_SPACING + COLUMN_INSET));
  btk_widget_size_allocate (clist->column[last_button].button,
			    &button_allocation);

  if (clist->column[last_button].resizeable)
    {
      button_allocation.x += button_allocation.width;

      bdk_window_show (clist->column[last_button].window);
      bdk_window_move_resize (clist->column[last_button].window,
			      button_allocation.x - (DRAG_WIDTH / 2), 
			      0, DRAG_WIDTH, clist->column_title_area.height);
    }
  else
    bdk_window_hide (clist->column[last_button].window);
}

static void
size_allocate_columns (BtkCList *clist,
		       gboolean  block_resize)
{
  gint xoffset = CELL_SPACING + COLUMN_INSET;
  gint last_column;
  gint i;

  /* find last visible column and calculate correct column width */
  for (last_column = clist->columns - 1;
       last_column >= 0 && !clist->column[last_column].visible; last_column--);

  if (last_column < 0)
    return;

  for (i = 0; i <= last_column; i++)
    {
      if (!clist->column[i].visible)
	continue;
      clist->column[i].area.x = xoffset;
      if (clist->column[i].width_set)
	{
	  if (!block_resize && BTK_CLIST_SHOW_TITLES(clist) &&
	      clist->column[i].auto_resize && clist->column[i].button)
	    {
	      gint width;

	      width = (clist->column[i].button->requisition.width -
		       (CELL_SPACING + (2 * COLUMN_INSET)));

	      if (width > clist->column[i].width)
		btk_clist_set_column_width (clist, i, width);
	    }

	  clist->column[i].area.width = clist->column[i].width;
	  xoffset += clist->column[i].width + CELL_SPACING + (2* COLUMN_INSET);
	}
      else if (BTK_CLIST_SHOW_TITLES(clist) && clist->column[i].button)
	{
	  clist->column[i].area.width =
	    clist->column[i].button->requisition.width -
	    (CELL_SPACING + (2 * COLUMN_INSET));
	  xoffset += clist->column[i].button->requisition.width;
	}
    }

  clist->column[last_column].area.width = clist->column[last_column].area.width
    + MAX (0, clist->clist_window_width + COLUMN_INSET - xoffset);
}

static gint
list_requisition_width (BtkCList *clist) 
{
  gint width = CELL_SPACING;
  gint i;

  for (i = clist->columns - 1; i >= 0; i--)
    {
      if (!clist->column[i].visible)
	continue;

      if (clist->column[i].width_set)
	width += clist->column[i].width + CELL_SPACING + (2 * COLUMN_INSET);
      else if (BTK_CLIST_SHOW_TITLES(clist) && clist->column[i].button)
	width += clist->column[i].button->requisition.width;
    }

  return width;
}

/* this function returns the new width of the column being resized given
 * the column and x position of the cursor; the x cursor position is passed
 * in as a pointer and automagicly corrected if it's beyond min/max limits */
static gint
new_column_width (BtkCList *clist,
		  gint      column,
		  gint     *x)
{
  gint xthickness = BTK_WIDGET (clist)->style->xthickness;
  gint width;
  gint cx;
  gint dx;
  gint last_column;

  /* first translate the x position from widget->window
   * to clist->clist_window */
  cx = *x - xthickness;

  for (last_column = clist->columns - 1;
       last_column >= 0 && !clist->column[last_column].visible; last_column--);

  /* calculate new column width making sure it doesn't end up
   * less than the minimum width */
  dx = (COLUMN_LEFT_XPIXEL (clist, column) + COLUMN_INSET +
	(column < last_column) * CELL_SPACING);
  width = cx - dx;

  if (width < MAX (COLUMN_MIN_WIDTH, clist->column[column].min_width))
    {
      width = MAX (COLUMN_MIN_WIDTH, clist->column[column].min_width);
      cx = dx + width;
      *x = cx + xthickness;
    }
  else if (clist->column[column].max_width >= COLUMN_MIN_WIDTH &&
	   width > clist->column[column].max_width)
    {
      width = clist->column[column].max_width;
      cx = dx + clist->column[column].max_width;
      *x = cx + xthickness;
    }      

  if (cx < 0 || cx > clist->clist_window_width)
    *x = -1;

  return width;
}

static void
column_button_create (BtkCList *clist,
		      gint      column)
{
  BtkWidget *button;

  btk_widget_push_composite_child ();
  button = clist->column[column].button = btk_button_new ();
  btk_widget_pop_composite_child ();

  if (btk_widget_get_realized (BTK_WIDGET (clist)) && clist->title_window)
    btk_widget_set_parent_window (clist->column[column].button,
				  clist->title_window);
  btk_widget_set_parent (button, BTK_WIDGET (clist));

  btk_signal_connect (BTK_OBJECT (button), "clicked",
		      G_CALLBACK (column_button_clicked),
		      (gpointer) clist);
  btk_widget_show (button);
}

static void
column_button_clicked (BtkWidget *widget,
		       gpointer   data)
{
  gint i;
  BtkCList *clist;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (BTK_IS_CLIST (data));

  clist = BTK_CLIST (data);

  /* find the column who's button was pressed */
  for (i = 0; i < clist->columns; i++)
    if (clist->column[i].button == widget)
      break;

  btk_signal_emit (BTK_OBJECT (clist), clist_signals[CLICK_COLUMN], i);
}

static gint
column_title_passive_func (BtkWidget *widget, 
			   BdkEvent  *event,
			   gpointer   data)
{
  g_return_val_if_fail (event != NULL, FALSE);
  
  switch (event->type)
    {
    case BDK_MOTION_NOTIFY:
    case BDK_BUTTON_PRESS:
    case BDK_2BUTTON_PRESS:
    case BDK_3BUTTON_PRESS:
    case BDK_BUTTON_RELEASE:
    case BDK_ENTER_NOTIFY:
    case BDK_LEAVE_NOTIFY:
      return TRUE;
    default:
      break;
    }
  return FALSE;
}


/* PUBLIC CELL FUNCTIONS
 *   btk_clist_get_cell_type
 *   btk_clist_set_text
 *   btk_clist_get_text
 *   btk_clist_set_pixmap
 *   btk_clist_get_pixmap
 *   btk_clist_set_pixtext
 *   btk_clist_get_pixtext
 *   btk_clist_set_shift
 */
BtkCellType 
btk_clist_get_cell_type (BtkCList *clist,
			 gint      row,
			 gint      column)
{
  BtkCListRow *clist_row;

  g_return_val_if_fail (BTK_IS_CLIST (clist), -1);

  if (row < 0 || row >= clist->rows)
    return -1;
  if (column < 0 || column >= clist->columns)
    return -1;

  clist_row = ROW_ELEMENT (clist, row)->data;

  return clist_row->cell[column].type;
}

void
btk_clist_set_text (BtkCList    *clist,
		    gint         row,
		    gint         column,
		    const gchar *text)
{
  BtkCListRow *clist_row;

  g_return_if_fail (BTK_IS_CLIST (clist));

  if (row < 0 || row >= clist->rows)
    return;
  if (column < 0 || column >= clist->columns)
    return;

  clist_row = ROW_ELEMENT (clist, row)->data;

  /* if text is null, then the cell is empty */
  BTK_CLIST_GET_CLASS (clist)->set_cell_contents
    (clist, clist_row, column, BTK_CELL_TEXT, text, 0, NULL, NULL);

  /* redraw the list if it's not frozen */
  if (CLIST_UNFROZEN (clist))
    {
      if (btk_clist_row_is_visible (clist, row) != BTK_VISIBILITY_NONE)
	BTK_CLIST_GET_CLASS (clist)->draw_row (clist, NULL, row, clist_row);
    }
}

gint
btk_clist_get_text (BtkCList  *clist,
		    gint       row,
		    gint       column,
		    gchar    **text)
{
  BtkCListRow *clist_row;

  g_return_val_if_fail (BTK_IS_CLIST (clist), 0);

  if (row < 0 || row >= clist->rows)
    return 0;
  if (column < 0 || column >= clist->columns)
    return 0;

  clist_row = ROW_ELEMENT (clist, row)->data;

  if (clist_row->cell[column].type != BTK_CELL_TEXT)
    return 0;

  if (text)
    *text = BTK_CELL_TEXT (clist_row->cell[column])->text;

  return 1;
}

/**
 * btk_clist_set_pixmap:
 * @mask: (allow-none):
 */
void
btk_clist_set_pixmap (BtkCList  *clist,
		      gint       row,
		      gint       column,
		      BdkPixmap *pixmap,
		      BdkBitmap *mask)
{
  BtkCListRow *clist_row;

  g_return_if_fail (BTK_IS_CLIST (clist));

  if (row < 0 || row >= clist->rows)
    return;
  if (column < 0 || column >= clist->columns)
    return;

  clist_row = ROW_ELEMENT (clist, row)->data;
  
  g_object_ref (pixmap);
  
  if (mask) g_object_ref (mask);
  
  BTK_CLIST_GET_CLASS (clist)->set_cell_contents
    (clist, clist_row, column, BTK_CELL_PIXMAP, NULL, 0, pixmap, mask);

  /* redraw the list if it's not frozen */
  if (CLIST_UNFROZEN (clist))
    {
      if (btk_clist_row_is_visible (clist, row) != BTK_VISIBILITY_NONE)
	BTK_CLIST_GET_CLASS (clist)->draw_row (clist, NULL, row, clist_row);
    }
}

gint
btk_clist_get_pixmap (BtkCList   *clist,
		      gint        row,
		      gint        column,
		      BdkPixmap **pixmap,
		      BdkBitmap **mask)
{
  BtkCListRow *clist_row;

  g_return_val_if_fail (BTK_IS_CLIST (clist), 0);

  if (row < 0 || row >= clist->rows)
    return 0;
  if (column < 0 || column >= clist->columns)
    return 0;

  clist_row = ROW_ELEMENT (clist, row)->data;

  if (clist_row->cell[column].type != BTK_CELL_PIXMAP)
    return 0;

  if (pixmap)
  {
    *pixmap = BTK_CELL_PIXMAP (clist_row->cell[column])->pixmap;
    /* mask can be NULL */
    *mask = BTK_CELL_PIXMAP (clist_row->cell[column])->mask;
  }

  return 1;
}

void
btk_clist_set_pixtext (BtkCList    *clist,
		       gint         row,
		       gint         column,
		       const gchar *text,
		       guint8       spacing,
		       BdkPixmap   *pixmap,
		       BdkBitmap   *mask)
{
  BtkCListRow *clist_row;

  g_return_if_fail (BTK_IS_CLIST (clist));

  if (row < 0 || row >= clist->rows)
    return;
  if (column < 0 || column >= clist->columns)
    return;

  clist_row = ROW_ELEMENT (clist, row)->data;
  
  g_object_ref (pixmap);
  if (mask) g_object_ref (mask);
  BTK_CLIST_GET_CLASS (clist)->set_cell_contents
    (clist, clist_row, column, BTK_CELL_PIXTEXT, text, spacing, pixmap, mask);

  /* redraw the list if it's not frozen */
  if (CLIST_UNFROZEN (clist))
    {
      if (btk_clist_row_is_visible (clist, row) != BTK_VISIBILITY_NONE)
	BTK_CLIST_GET_CLASS (clist)->draw_row (clist, NULL, row, clist_row);
    }
}

gint
btk_clist_get_pixtext (BtkCList   *clist,
		       gint        row,
		       gint        column,
		       gchar     **text,
		       guint8     *spacing,
		       BdkPixmap **pixmap,
		       BdkBitmap **mask)
{
  BtkCListRow *clist_row;

  g_return_val_if_fail (BTK_IS_CLIST (clist), 0);

  if (row < 0 || row >= clist->rows)
    return 0;
  if (column < 0 || column >= clist->columns)
    return 0;

  clist_row = ROW_ELEMENT (clist, row)->data;

  if (clist_row->cell[column].type != BTK_CELL_PIXTEXT)
    return 0;

  if (text)
    *text = BTK_CELL_PIXTEXT (clist_row->cell[column])->text;
  if (spacing)
    *spacing = BTK_CELL_PIXTEXT (clist_row->cell[column])->spacing;
  if (pixmap)
    *pixmap = BTK_CELL_PIXTEXT (clist_row->cell[column])->pixmap;

  /* mask can be NULL */
  if (mask)
    *mask = BTK_CELL_PIXTEXT (clist_row->cell[column])->mask;

  return 1;
}

void
btk_clist_set_shift (BtkCList *clist,
		     gint      row,
		     gint      column,
		     gint      vertical,
		     gint      horizontal)
{
  BtkRequisition requisition = { 0 };
  BtkCListRow *clist_row;

  g_return_if_fail (BTK_IS_CLIST (clist));

  if (row < 0 || row >= clist->rows)
    return;
  if (column < 0 || column >= clist->columns)
    return;

  clist_row = ROW_ELEMENT (clist, row)->data;

  if (clist->column[column].auto_resize &&
      !BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    BTK_CLIST_GET_CLASS (clist)->cell_size_request (clist, clist_row,
						   column, &requisition);

  clist_row->cell[column].vertical = vertical;
  clist_row->cell[column].horizontal = horizontal;

  column_auto_resize (clist, clist_row, column, requisition.width);

  if (CLIST_UNFROZEN (clist) && btk_clist_row_is_visible (clist, row) != BTK_VISIBILITY_NONE)
    BTK_CLIST_GET_CLASS (clist)->draw_row (clist, NULL, row, clist_row);
}

/* PRIVATE CELL FUNCTIONS
 *   set_cell_contents
 *   cell_size_request
 */
static void
set_cell_contents (BtkCList    *clist,
		   BtkCListRow *clist_row,
		   gint         column,
		   BtkCellType  type,
		   const gchar *text,
		   guint8       spacing,
		   BdkPixmap   *pixmap,
		   BdkBitmap   *mask)
{
  BtkRequisition requisition;
  gchar *old_text = NULL;
  BdkPixmap *old_pixmap = NULL;
  BdkBitmap *old_mask = NULL;
  
  g_return_if_fail (BTK_IS_CLIST (clist));
  g_return_if_fail (clist_row != NULL);

  if (clist->column[column].auto_resize &&
      !BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    BTK_CLIST_GET_CLASS (clist)->cell_size_request (clist, clist_row,
						   column, &requisition);

  switch (clist_row->cell[column].type)
    {
    case BTK_CELL_EMPTY:
      break;
    case BTK_CELL_TEXT:
      old_text = BTK_CELL_TEXT (clist_row->cell[column])->text;
      break;
    case BTK_CELL_PIXMAP:
      old_pixmap = BTK_CELL_PIXMAP (clist_row->cell[column])->pixmap;
      old_mask = BTK_CELL_PIXMAP (clist_row->cell[column])->mask;
      break;
    case BTK_CELL_PIXTEXT:
      old_text = BTK_CELL_PIXTEXT (clist_row->cell[column])->text;
      old_pixmap = BTK_CELL_PIXTEXT (clist_row->cell[column])->pixmap;
      old_mask = BTK_CELL_PIXTEXT (clist_row->cell[column])->mask;
      break;
    case BTK_CELL_WIDGET:
      /* unimplemented */
      break;
    default:
      break;
    }

  clist_row->cell[column].type = BTK_CELL_EMPTY;

  /* Note that pixmap and mask were already ref'ed by the caller
   */
  switch (type)
    {
    case BTK_CELL_TEXT:
      if (text)
	{
	  clist_row->cell[column].type = BTK_CELL_TEXT;
	  BTK_CELL_TEXT (clist_row->cell[column])->text = g_strdup (text);
	}
      break;
    case BTK_CELL_PIXMAP:
      if (pixmap)
	{
	  clist_row->cell[column].type = BTK_CELL_PIXMAP;
	  BTK_CELL_PIXMAP (clist_row->cell[column])->pixmap = pixmap;
	  /* We set the mask even if it is NULL */
	  BTK_CELL_PIXMAP (clist_row->cell[column])->mask = mask;
	}
      break;
    case BTK_CELL_PIXTEXT:
      if (text && pixmap)
	{
	  clist_row->cell[column].type = BTK_CELL_PIXTEXT;
	  BTK_CELL_PIXTEXT (clist_row->cell[column])->text = g_strdup (text);
	  BTK_CELL_PIXTEXT (clist_row->cell[column])->spacing = spacing;
	  BTK_CELL_PIXTEXT (clist_row->cell[column])->pixmap = pixmap;
	  BTK_CELL_PIXTEXT (clist_row->cell[column])->mask = mask;
	}
      break;
    default:
      break;
    }

  if (clist->column[column].auto_resize &&
      !BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    column_auto_resize (clist, clist_row, column, requisition.width);

  g_free (old_text);
  if (old_pixmap)
    g_object_unref (old_pixmap);
  if (old_mask)
    g_object_unref (old_mask);
}

BangoLayout *
_btk_clist_create_cell_layout (BtkCList       *clist,
			       BtkCListRow    *clist_row,
			       gint            column)
{
  BangoLayout *layout;
  BtkStyle *style;
  BtkCell *cell;
  gchar *text;
  
  get_cell_style (clist, clist_row, BTK_STATE_NORMAL, column, &style,
		  NULL, NULL);


  cell = &clist_row->cell[column];
  switch (cell->type)
    {
    case BTK_CELL_TEXT:
    case BTK_CELL_PIXTEXT:
      text = ((cell->type == BTK_CELL_PIXTEXT) ?
	      BTK_CELL_PIXTEXT (*cell)->text :
	      BTK_CELL_TEXT (*cell)->text);

      if (!text)
	return NULL;
      
      layout = btk_widget_create_bango_layout (BTK_WIDGET (clist),
					       ((cell->type == BTK_CELL_PIXTEXT) ?
						BTK_CELL_PIXTEXT (*cell)->text :
						BTK_CELL_TEXT (*cell)->text));
      bango_layout_set_font_description (layout, style->font_desc);
      
      return layout;
      
    default:
      return NULL;
    }
}

static void
cell_size_request (BtkCList       *clist,
		   BtkCListRow    *clist_row,
		   gint            column,
		   BtkRequisition *requisition)
{
  gint width;
  gint height;
  BangoLayout *layout;
  BangoRectangle logical_rect;

  g_return_if_fail (BTK_IS_CLIST (clist));
  g_return_if_fail (requisition != NULL);

  layout = _btk_clist_create_cell_layout (clist, clist_row, column);
  if (layout)
    {
      bango_layout_get_pixel_extents (layout, NULL, &logical_rect);
      
      requisition->width = logical_rect.width;
      requisition->height = logical_rect.height;
      
      g_object_unref (layout);
    }
  else
    {
      requisition->width  = 0;
      requisition->height = 0;
    }

  if (layout && clist_row->cell[column].type == BTK_CELL_PIXTEXT)
    requisition->width += BTK_CELL_PIXTEXT (clist_row->cell[column])->spacing;

  switch (clist_row->cell[column].type)
    {
    case BTK_CELL_PIXTEXT:
      bdk_drawable_get_size (BTK_CELL_PIXTEXT (clist_row->cell[column])->pixmap,
                             &width, &height);
      requisition->width += width;
      requisition->height = MAX (requisition->height, height);      
      break;
    case BTK_CELL_PIXMAP:
      bdk_drawable_get_size (BTK_CELL_PIXMAP (clist_row->cell[column])->pixmap,
                             &width, &height);
      requisition->width += width;
      requisition->height = MAX (requisition->height, height);
      break;
      
    default:
      break;
    }

  requisition->width  += clist_row->cell[column].horizontal;
  requisition->height += clist_row->cell[column].vertical;
}

/* PUBLIC INSERT/REMOVE ROW FUNCTIONS
 *   btk_clist_prepend
 *   btk_clist_append
 *   btk_clist_insert
 *   btk_clist_remove
 *   btk_clist_clear
 */
gint
btk_clist_prepend (BtkCList    *clist,
		   gchar       *text[])
{
  g_return_val_if_fail (BTK_IS_CLIST (clist), -1);
  g_return_val_if_fail (text != NULL, -1);

  return BTK_CLIST_GET_CLASS (clist)->insert_row (clist, 0, text);
}

gint
btk_clist_append (BtkCList    *clist,
		  gchar       *text[])
{
  g_return_val_if_fail (BTK_IS_CLIST (clist), -1);
  g_return_val_if_fail (text != NULL, -1);

  return BTK_CLIST_GET_CLASS (clist)->insert_row (clist, clist->rows, text);
}

gint
btk_clist_insert (BtkCList    *clist,
		  gint         row,
		  gchar       *text[])
{
  g_return_val_if_fail (BTK_IS_CLIST (clist), -1);
  g_return_val_if_fail (text != NULL, -1);

  if (row < 0 || row > clist->rows)
    row = clist->rows;

  return BTK_CLIST_GET_CLASS (clist)->insert_row (clist, row, text);
}

void
btk_clist_remove (BtkCList *clist,
		  gint      row)
{
  BTK_CLIST_GET_CLASS (clist)->remove_row (clist, row);
}

void
btk_clist_clear (BtkCList *clist)
{
  g_return_if_fail (BTK_IS_CLIST (clist));
  
  BTK_CLIST_GET_CLASS (clist)->clear (clist);
}

/* PRIVATE INSERT/REMOVE ROW FUNCTIONS
 *   real_insert_row
 *   real_remove_row
 *   real_clear
 *   real_row_move
 */
static gint
real_insert_row (BtkCList *clist,
		 gint      row,
		 gchar    *text[])
{
  gint i;
  BtkCListRow *clist_row;

  g_return_val_if_fail (BTK_IS_CLIST (clist), -1);
  g_return_val_if_fail (text != NULL, -1);

  /* return if out of bounds */
  if (row < 0 || row > clist->rows)
    return -1;

  /* create the row */
  clist_row = row_new (clist);

  /* set the text in the row's columns */
  for (i = 0; i < clist->columns; i++)
    if (text[i])
      BTK_CLIST_GET_CLASS (clist)->set_cell_contents
	(clist, clist_row, i, BTK_CELL_TEXT, text[i], 0, NULL ,NULL);

  if (!clist->rows)
    {
      clist->row_list = g_list_append (clist->row_list, clist_row);
      clist->row_list_end = clist->row_list;
    }
  else
    {
      if (BTK_CLIST_AUTO_SORT(clist))   /* override insertion pos */
	{
	  GList *work;
	  
	  row = 0;
	  work = clist->row_list;
	  
	  if (clist->sort_type == BTK_SORT_ASCENDING)
	    {
	      while (row < clist->rows &&
		     clist->compare (clist, clist_row,
				     BTK_CLIST_ROW (work)) > 0)
		{
		  row++;
		  work = work->next;
		}
	    }
	  else
	    {
	      while (row < clist->rows &&
		     clist->compare (clist, clist_row,
				     BTK_CLIST_ROW (work)) < 0)
		{
		  row++;
		  work = work->next;
		}
	    }
	}
      
      /* reset the row end pointer if we're inserting at the end of the list */
      if (row == clist->rows)
	clist->row_list_end = (g_list_append (clist->row_list_end,
					      clist_row))->next;
      else
	clist->row_list = g_list_insert (clist->row_list, clist_row, row);

    }
  clist->rows++;

  if (row < ROW_FROM_YPIXEL (clist, 0))
    clist->voffset -= (clist->row_height + CELL_SPACING);

  /* syncronize the selection list */
  sync_selection (clist, row, SYNC_INSERT);

  if (clist->rows == 1)
    {
      clist->focus_row = 0;
      if (clist->selection_mode == BTK_SELECTION_BROWSE)
	btk_clist_select_row (clist, 0, -1);
    }

  /* redraw the list if it isn't frozen */
  if (CLIST_UNFROZEN (clist))
    {
      adjust_adjustments (clist, FALSE);

      if (btk_clist_row_is_visible (clist, row) != BTK_VISIBILITY_NONE)
	draw_rows (clist, NULL);
    }

  return row;
}

static void
real_remove_row (BtkCList *clist,
		 gint      row)
{
  gint was_visible;
  GList *list;
  BtkCListRow *clist_row;

  g_return_if_fail (BTK_IS_CLIST (clist));

  /* return if out of bounds */
  if (row < 0 || row > (clist->rows - 1))
    return;

  was_visible = (btk_clist_row_is_visible (clist, row) != BTK_VISIBILITY_NONE);

  /* get the row we're going to delete */
  list = ROW_ELEMENT (clist, row);
  g_assert (list != NULL);
  clist_row = list->data;

  /* if we're removing a selected row, we have to make sure
   * it's properly unselected, and then sync up the clist->selected
   * list to reflect the deincrimented indexies of rows after the
   * removal */
  if (clist_row->state == BTK_STATE_SELECTED)
    btk_signal_emit (BTK_OBJECT (clist), clist_signals[UNSELECT_ROW],
		     row, -1, NULL);

  sync_selection (clist, row, SYNC_REMOVE);

  /* reset the row end pointer if we're removing at the end of the list */
  clist->rows--;
  if (clist->row_list == list)
    clist->row_list = g_list_next (list);
  if (clist->row_list_end == list)
    clist->row_list_end = g_list_previous (list);
  list = g_list_remove (list, clist_row);

  if (row < ROW_FROM_YPIXEL (clist, 0))
    clist->voffset += clist->row_height + CELL_SPACING;

  if (clist->selection_mode == BTK_SELECTION_BROWSE && !clist->selection &&
      clist->focus_row >= 0)
    btk_signal_emit (BTK_OBJECT (clist), clist_signals[SELECT_ROW],
		     clist->focus_row, -1, NULL);

  /* toast the row */
  row_delete (clist, clist_row);

  /* redraw the row if it isn't frozen */
  if (CLIST_UNFROZEN (clist))
    {
      adjust_adjustments (clist, FALSE);

      if (was_visible)
	draw_rows (clist, NULL);
    }
}

static void
real_clear (BtkCList *clist)
{
  GList *list;
  GList *free_list;
  gint i;

  g_return_if_fail (BTK_IS_CLIST (clist));

  /* free up the selection list */
  g_list_free (clist->selection);
  g_list_free (clist->undo_selection);
  g_list_free (clist->undo_unselection);

  clist->selection = NULL;
  clist->selection_end = NULL;
  clist->undo_selection = NULL;
  clist->undo_unselection = NULL;
  clist->voffset = 0;
  clist->focus_row = -1;
  clist->anchor = -1;
  clist->undo_anchor = -1;
  clist->anchor_state = BTK_STATE_SELECTED;
  clist->drag_pos = -1;

  /* remove all the rows */
  BTK_CLIST_SET_FLAG (clist, CLIST_AUTO_RESIZE_BLOCKED);
  free_list = clist->row_list;
  clist->row_list = NULL;
  clist->row_list_end = NULL;
  clist->rows = 0;
  for (list = free_list; list; list = list->next)
    row_delete (clist, BTK_CLIST_ROW (list));
  g_list_free (free_list);
  BTK_CLIST_UNSET_FLAG (clist, CLIST_AUTO_RESIZE_BLOCKED);
  for (i = 0; i < clist->columns; i++)
    if (clist->column[i].auto_resize)
      {
	if (BTK_CLIST_SHOW_TITLES(clist) && clist->column[i].button)
	  btk_clist_set_column_width
	    (clist, i, (clist->column[i].button->requisition.width -
			(CELL_SPACING + (2 * COLUMN_INSET))));
	else
	  btk_clist_set_column_width (clist, i, 0);
      }
  /* zero-out the scrollbars */
  if (clist->vadjustment)
    {
      btk_adjustment_set_value (clist->vadjustment, 0.0);
      CLIST_REFRESH (clist);
    }
  else
    btk_widget_queue_resize (BTK_WIDGET (clist));
}

static void
real_row_move (BtkCList *clist,
	       gint      source_row,
	       gint      dest_row)
{
  BtkCListRow *clist_row;
  GList *list;
  gint first, last;
  gint d;

  g_return_if_fail (BTK_IS_CLIST (clist));

  if (BTK_CLIST_AUTO_SORT(clist))
    return;

  if (source_row < 0 || source_row >= clist->rows ||
      dest_row   < 0 || dest_row   >= clist->rows ||
      source_row == dest_row)
    return;

  btk_clist_freeze (clist);

  /* unlink source row */
  clist_row = ROW_ELEMENT (clist, source_row)->data;
  if (source_row == clist->rows - 1)
    clist->row_list_end = clist->row_list_end->prev;
  clist->row_list = g_list_remove (clist->row_list, clist_row);
  clist->rows--;

  /* relink source row */
  clist->row_list = g_list_insert (clist->row_list, clist_row, dest_row);
  if (dest_row == clist->rows)
    clist->row_list_end = clist->row_list_end->next;
  clist->rows++;

  /* sync selection */
  if (source_row > dest_row)
    {
      first = dest_row;
      last  = source_row;
      d = 1;
    }
  else
    {
      first = source_row;
      last  = dest_row;
      d = -1;
    }

  for (list = clist->selection; list; list = list->next)
    {
      if (list->data == GINT_TO_POINTER (source_row))
	list->data = GINT_TO_POINTER (dest_row);
      else if (first <= GPOINTER_TO_INT (list->data) &&
	       last >= GPOINTER_TO_INT (list->data))
	list->data = GINT_TO_POINTER (GPOINTER_TO_INT (list->data) + d);
    }
  
  if (clist->focus_row == source_row)
    clist->focus_row = dest_row;
  else if (clist->focus_row > first)
    clist->focus_row += d;

  btk_clist_thaw (clist);
}

/* PUBLIC ROW FUNCTIONS
 *   btk_clist_moveto
 *   btk_clist_set_row_height
 *   btk_clist_set_row_data
 *   btk_clist_set_row_data_full
 *   btk_clist_get_row_data
 *   btk_clist_find_row_from_data
 *   btk_clist_swap_rows
 *   btk_clist_row_move
 *   btk_clist_row_is_visible
 *   btk_clist_set_foreground
 *   btk_clist_set_background
 */
void
btk_clist_moveto (BtkCList *clist,
		  gint      row,
		  gint      column,
		  gfloat    row_align,
		  gfloat    col_align)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (row < -1 || row >= clist->rows)
    return;
  if (column < -1 || column >= clist->columns)
    return;

  row_align = CLAMP (row_align, 0, 1);
  col_align = CLAMP (col_align, 0, 1);

  /* adjust horizontal scrollbar */
  if (clist->hadjustment && column >= 0)
    {
      gint x;

      x = (COLUMN_LEFT (clist, column) - CELL_SPACING - COLUMN_INSET -
	   (col_align * (clist->clist_window_width - 2 * COLUMN_INSET -
			 CELL_SPACING - clist->column[column].area.width)));
      if (x < 0)
	btk_adjustment_set_value (clist->hadjustment, 0.0);
      else if (x > LIST_WIDTH (clist) - clist->clist_window_width)
	btk_adjustment_set_value 
	  (clist->hadjustment, LIST_WIDTH (clist) - clist->clist_window_width);
      else
	btk_adjustment_set_value (clist->hadjustment, x);
    }

  /* adjust vertical scrollbar */
  if (clist->vadjustment && row >= 0)
    move_vertical (clist, row, row_align);
}

void
btk_clist_set_row_height (BtkCList *clist,
			  guint     height)
{
  BtkWidget *widget;

  g_return_if_fail (BTK_IS_CLIST (clist));

  widget = BTK_WIDGET (clist);

  if (height > 0)
    {
      clist->row_height = height;
      BTK_CLIST_SET_FLAG (clist, CLIST_ROW_HEIGHT_SET);
    }
  else
    {
      BTK_CLIST_UNSET_FLAG (clist, CLIST_ROW_HEIGHT_SET);
      clist->row_height = 0;
    }

  if (widget->style->font_desc)
    {
      BangoContext *context = btk_widget_get_bango_context (widget);
      BangoFontMetrics *metrics;

      metrics = bango_context_get_metrics (context,
					   widget->style->font_desc,
					   bango_context_get_language (context));
      
      if (!BTK_CLIST_ROW_HEIGHT_SET(clist))
	{
	  clist->row_height = (bango_font_metrics_get_ascent (metrics) +
			       bango_font_metrics_get_descent (metrics));
	  clist->row_height = BANGO_PIXELS (clist->row_height);
	}

      bango_font_metrics_unref (metrics);
    }
      
  CLIST_REFRESH (clist);
}

void
btk_clist_set_row_data (BtkCList *clist,
			gint      row,
			gpointer  data)
{
  btk_clist_set_row_data_full (clist, row, data, NULL);
}

void
btk_clist_set_row_data_full (BtkCList       *clist,
			     gint            row,
			     gpointer        data,
			     GDestroyNotify  destroy)
{
  BtkCListRow *clist_row;

  g_return_if_fail (BTK_IS_CLIST (clist));

  if (row < 0 || row > (clist->rows - 1))
    return;

  clist_row = ROW_ELEMENT (clist, row)->data;

  if (clist_row->destroy)
    clist_row->destroy (clist_row->data);
  
  clist_row->data = data;
  clist_row->destroy = destroy;
}

gpointer
btk_clist_get_row_data (BtkCList *clist,
			gint      row)
{
  BtkCListRow *clist_row;

  g_return_val_if_fail (BTK_IS_CLIST (clist), NULL);

  if (row < 0 || row > (clist->rows - 1))
    return NULL;

  clist_row = ROW_ELEMENT (clist, row)->data;
  return clist_row->data;
}

gint
btk_clist_find_row_from_data (BtkCList *clist,
			      gpointer  data)
{
  GList *list;
  gint n;

  g_return_val_if_fail (BTK_IS_CLIST (clist), -1);

  for (n = 0, list = clist->row_list; list; n++, list = list->next)
    if (BTK_CLIST_ROW (list)->data == data)
      return n;

  return -1;
}

void 
btk_clist_swap_rows (BtkCList *clist,
		     gint      row1, 
		     gint      row2)
{
  gint first, last;

  g_return_if_fail (BTK_IS_CLIST (clist));
  g_return_if_fail (row1 != row2);

  if (BTK_CLIST_AUTO_SORT(clist))
    return;

  btk_clist_freeze (clist);

  first = MIN (row1, row2);
  last  = MAX (row1, row2);

  btk_clist_row_move (clist, last, first);
  btk_clist_row_move (clist, first + 1, last);
  
  btk_clist_thaw (clist);
}

void
btk_clist_row_move (BtkCList *clist,
		    gint      source_row,
		    gint      dest_row)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (BTK_CLIST_AUTO_SORT(clist))
    return;

  if (source_row < 0 || source_row >= clist->rows ||
      dest_row   < 0 || dest_row   >= clist->rows ||
      source_row == dest_row)
    return;

  btk_signal_emit (BTK_OBJECT (clist), clist_signals[ROW_MOVE],
		   source_row, dest_row);
}

BtkVisibility
btk_clist_row_is_visible (BtkCList *clist,
			  gint      row)
{
  gint top;

  g_return_val_if_fail (BTK_IS_CLIST (clist), 0);

  if (row < 0 || row >= clist->rows)
    return BTK_VISIBILITY_NONE;

  if (clist->row_height == 0)
    return BTK_VISIBILITY_NONE;

  if (row < ROW_FROM_YPIXEL (clist, 0))
    return BTK_VISIBILITY_NONE;

  if (row > ROW_FROM_YPIXEL (clist, clist->clist_window_height))
    return BTK_VISIBILITY_NONE;

  top = ROW_TOP_YPIXEL (clist, row);

  if ((top < 0)
      || ((top + clist->row_height) >= clist->clist_window_height))
    return BTK_VISIBILITY_PARTIAL;

  return BTK_VISIBILITY_FULL;
}

void
btk_clist_set_foreground (BtkCList       *clist,
			  gint            row,
			  const BdkColor *color)
{
  BtkCListRow *clist_row;

  g_return_if_fail (BTK_IS_CLIST (clist));

  if (row < 0 || row >= clist->rows)
    return;

  clist_row = ROW_ELEMENT (clist, row)->data;

  if (color)
    {
      clist_row->foreground = *color;
      clist_row->fg_set = TRUE;
      if (btk_widget_get_realized (BTK_WIDGET (clist)))
	bdk_colormap_alloc_color (btk_widget_get_colormap (BTK_WIDGET (clist)),
                                  &clist_row->foreground, FALSE, TRUE);
    }
  else
    clist_row->fg_set = FALSE;

  if (CLIST_UNFROZEN (clist) && btk_clist_row_is_visible (clist, row) != BTK_VISIBILITY_NONE)
    BTK_CLIST_GET_CLASS (clist)->draw_row (clist, NULL, row, clist_row);
}

void
btk_clist_set_background (BtkCList       *clist,
			  gint            row,
			  const BdkColor *color)
{
  BtkCListRow *clist_row;

  g_return_if_fail (BTK_IS_CLIST (clist));

  if (row < 0 || row >= clist->rows)
    return;

  clist_row = ROW_ELEMENT (clist, row)->data;

  if (color)
    {
      clist_row->background = *color;
      clist_row->bg_set = TRUE;
      if (btk_widget_get_realized (BTK_WIDGET (clist)))
	bdk_colormap_alloc_color (btk_widget_get_colormap (BTK_WIDGET (clist)),
                                  &clist_row->background, FALSE, TRUE);
    }
  else
    clist_row->bg_set = FALSE;

  if (CLIST_UNFROZEN (clist)
      && (btk_clist_row_is_visible (clist, row) != BTK_VISIBILITY_NONE))
    BTK_CLIST_GET_CLASS (clist)->draw_row (clist, NULL, row, clist_row);
}

/* PUBLIC ROW/CELL STYLE FUNCTIONS
 *   btk_clist_set_cell_style
 *   btk_clist_get_cell_style
 *   btk_clist_set_row_style
 *   btk_clist_get_row_style
 */
void
btk_clist_set_cell_style (BtkCList *clist,
			  gint      row,
			  gint      column,
			  BtkStyle *style)
{
  BtkRequisition requisition = { 0 };
  BtkCListRow *clist_row;

  g_return_if_fail (BTK_IS_CLIST (clist));

  if (row < 0 || row >= clist->rows)
    return;
  if (column < 0 || column >= clist->columns)
    return;

  clist_row = ROW_ELEMENT (clist, row)->data;

  if (clist_row->cell[column].style == style)
    return;

  if (clist->column[column].auto_resize &&
      !BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    BTK_CLIST_GET_CLASS (clist)->cell_size_request (clist, clist_row,
						   column, &requisition);

  if (clist_row->cell[column].style)
    {
      if (btk_widget_get_realized (BTK_WIDGET (clist)))
        btk_style_detach (clist_row->cell[column].style);
      g_object_unref (clist_row->cell[column].style);
    }

  clist_row->cell[column].style = style;

  if (clist_row->cell[column].style)
    {
      g_object_ref (clist_row->cell[column].style);
      
      if (btk_widget_get_realized (BTK_WIDGET (clist)))
        clist_row->cell[column].style =
	  btk_style_attach (clist_row->cell[column].style,
			    clist->clist_window);
    }

  column_auto_resize (clist, clist_row, column, requisition.width);

  /* redraw the list if it's not frozen */
  if (CLIST_UNFROZEN (clist))
    {
      if (btk_clist_row_is_visible (clist, row) != BTK_VISIBILITY_NONE)
	BTK_CLIST_GET_CLASS (clist)->draw_row (clist, NULL, row, clist_row);
    }
}

BtkStyle *
btk_clist_get_cell_style (BtkCList *clist,
			  gint      row,
			  gint      column)
{
  BtkCListRow *clist_row;

  g_return_val_if_fail (BTK_IS_CLIST (clist), NULL);

  if (row < 0 || row >= clist->rows || column < 0 || column >= clist->columns)
    return NULL;

  clist_row = ROW_ELEMENT (clist, row)->data;

  return clist_row->cell[column].style;
}

void
btk_clist_set_row_style (BtkCList *clist,
			 gint      row,
			 BtkStyle *style)
{
  BtkRequisition requisition;
  BtkCListRow *clist_row;
  gint *old_width;
  gint i;

  g_return_if_fail (BTK_IS_CLIST (clist));

  if (row < 0 || row >= clist->rows)
    return;

  clist_row = ROW_ELEMENT (clist, row)->data;

  if (clist_row->style == style)
    return;

  old_width = g_new (gint, clist->columns);

  if (!BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    {
      for (i = 0; i < clist->columns; i++)
	if (clist->column[i].auto_resize)
	  {
	    BTK_CLIST_GET_CLASS (clist)->cell_size_request (clist, clist_row,
							   i, &requisition);
	    old_width[i] = requisition.width;
	  }
    }

  if (clist_row->style)
    {
      if (btk_widget_get_realized (BTK_WIDGET (clist)))
        btk_style_detach (clist_row->style);
      g_object_unref (clist_row->style);
    }

  clist_row->style = style;

  if (clist_row->style)
    {
      g_object_ref (clist_row->style);
      
      if (btk_widget_get_realized (BTK_WIDGET (clist)))
        clist_row->style = btk_style_attach (clist_row->style,
					     clist->clist_window);
    }

  if (BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    for (i = 0; i < clist->columns; i++)
      column_auto_resize (clist, clist_row, i, old_width[i]);

  g_free (old_width);

  /* redraw the list if it's not frozen */
  if (CLIST_UNFROZEN (clist))
    {
      if (btk_clist_row_is_visible (clist, row) != BTK_VISIBILITY_NONE)
	BTK_CLIST_GET_CLASS (clist)->draw_row (clist, NULL, row, clist_row);
    }
}

BtkStyle *
btk_clist_get_row_style (BtkCList *clist,
			 gint      row)
{
  BtkCListRow *clist_row;

  g_return_val_if_fail (BTK_IS_CLIST (clist), NULL);

  if (row < 0 || row >= clist->rows)
    return NULL;

  clist_row = ROW_ELEMENT (clist, row)->data;

  return clist_row->style;
}

/* PUBLIC SELECTION FUNCTIONS
 *   btk_clist_set_selectable
 *   btk_clist_get_selectable
 *   btk_clist_select_row
 *   btk_clist_unselect_row
 *   btk_clist_select_all
 *   btk_clist_unselect_all
 *   btk_clist_undo_selection
 */
void
btk_clist_set_selectable (BtkCList *clist,
			  gint      row,
			  gboolean  selectable)
{
  BtkCListRow *clist_row;

  g_return_if_fail (BTK_IS_CLIST (clist));

  if (row < 0 || row >= clist->rows)
    return;

  clist_row = ROW_ELEMENT (clist, row)->data;

  if (selectable == clist_row->selectable)
    return;

  clist_row->selectable = selectable;

  if (!selectable && clist_row->state == BTK_STATE_SELECTED)
    {
      if (clist->anchor >= 0 &&
	  clist->selection_mode == BTK_SELECTION_MULTIPLE)
	{
	  clist->drag_button = 0;
	  remove_grab (clist);
	  BTK_CLIST_GET_CLASS (clist)->resync_selection (clist, NULL);
	}
      btk_signal_emit (BTK_OBJECT (clist), clist_signals[UNSELECT_ROW],
		       row, -1, NULL);
    }      
}

gboolean
btk_clist_get_selectable (BtkCList *clist,
			  gint      row)
{
  g_return_val_if_fail (BTK_IS_CLIST (clist), FALSE);

  if (row < 0 || row >= clist->rows)
    return FALSE;

  return BTK_CLIST_ROW (ROW_ELEMENT (clist, row))->selectable;
}

void
btk_clist_select_row (BtkCList *clist,
		      gint      row,
		      gint      column)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (row < 0 || row >= clist->rows)
    return;
  if (column < -1 || column >= clist->columns)
    return;

  btk_signal_emit (BTK_OBJECT (clist), clist_signals[SELECT_ROW],
		   row, column, NULL);
}

void
btk_clist_unselect_row (BtkCList *clist,
			gint      row,
			gint      column)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (row < 0 || row >= clist->rows)
    return;
  if (column < -1 || column >= clist->columns)
    return;

  btk_signal_emit (BTK_OBJECT (clist), clist_signals[UNSELECT_ROW],
		   row, column, NULL);
}

void
btk_clist_select_all (BtkCList *clist)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  BTK_CLIST_GET_CLASS (clist)->select_all (clist);
}

void
btk_clist_unselect_all (BtkCList *clist)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  BTK_CLIST_GET_CLASS (clist)->unselect_all (clist);
}

void
btk_clist_undo_selection (BtkCList *clist)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (clist->selection_mode == BTK_SELECTION_MULTIPLE &&
      (clist->undo_selection || clist->undo_unselection))
    btk_signal_emit (BTK_OBJECT (clist), clist_signals[UNDO_SELECTION]);
}

/* PRIVATE SELECTION FUNCTIONS
 *   selection_find
 *   toggle_row
 *   fake_toggle_row
 *   toggle_focus_row
 *   toggle_add_mode
 *   real_select_row
 *   real_unselect_row
 *   real_select_all
 *   real_unselect_all
 *   fake_unselect_all
 *   real_undo_selection
 *   set_anchor
 *   resync_selection
 *   update_extended_selection
 *   start_selection
 *   end_selection
 *   extend_selection
 *   sync_selection
 */
static GList *
selection_find (BtkCList *clist,
		gint      row_number,
		GList    *row_list_element)
{
  return g_list_find (clist->selection, GINT_TO_POINTER (row_number));
}

static void
toggle_row (BtkCList *clist,
	    gint      row,
	    gint      column,
	    BdkEvent *event)
{
  BtkCListRow *clist_row;

  switch (clist->selection_mode)
    {
    case BTK_SELECTION_MULTIPLE:
    case BTK_SELECTION_SINGLE:
      clist_row = ROW_ELEMENT (clist, row)->data;

      if (!clist_row)
	return;

      if (clist_row->state == BTK_STATE_SELECTED)
	{
	  btk_signal_emit (BTK_OBJECT (clist), clist_signals[UNSELECT_ROW],
			   row, column, event);
	  return;
	}
    case BTK_SELECTION_BROWSE:
      btk_signal_emit (BTK_OBJECT (clist), clist_signals[SELECT_ROW],
		       row, column, event);
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
fake_toggle_row (BtkCList *clist,
		 gint      row)
{
  GList *work;

  work = ROW_ELEMENT (clist, row);

  if (!work || !BTK_CLIST_ROW (work)->selectable)
    return;
  
  if (BTK_CLIST_ROW (work)->state == BTK_STATE_NORMAL)
    clist->anchor_state = BTK_CLIST_ROW (work)->state = BTK_STATE_SELECTED;
  else
    clist->anchor_state = BTK_CLIST_ROW (work)->state = BTK_STATE_NORMAL;
  
  if (CLIST_UNFROZEN (clist) &&
      btk_clist_row_is_visible (clist, row) != BTK_VISIBILITY_NONE)
    BTK_CLIST_GET_CLASS (clist)->draw_row (clist, NULL, row,
					  BTK_CLIST_ROW (work));
}

static gboolean
clist_has_grab (BtkCList *clist)
{
  return (BTK_WIDGET_HAS_GRAB (clist) &&
	  bdk_display_pointer_is_grabbed (btk_widget_get_display (BTK_WIDGET (clist))));
}

static void
toggle_focus_row (BtkCList *clist)
{
  g_return_if_fail (clist != 0);
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (clist_has_grab (clist) ||
      clist->focus_row < 0 || clist->focus_row >= clist->rows)
    return;

  switch (clist->selection_mode)
    {
    case  BTK_SELECTION_SINGLE:
      toggle_row (clist, clist->focus_row, 0, NULL);
      break;
    case BTK_SELECTION_MULTIPLE:
      g_list_free (clist->undo_selection);
      g_list_free (clist->undo_unselection);
      clist->undo_selection = NULL;
      clist->undo_unselection = NULL;

      clist->anchor = clist->focus_row;
      clist->drag_pos = clist->focus_row;
      clist->undo_anchor = clist->focus_row;
      
      if (BTK_CLIST_ADD_MODE(clist))
	fake_toggle_row (clist, clist->focus_row);
      else
	BTK_CLIST_GET_CLASS (clist)->fake_unselect_all (clist,clist->focus_row);

      BTK_CLIST_GET_CLASS (clist)->resync_selection (clist, NULL);
      break;
    default:
      break;
    }
}

static void
toggle_add_mode (BtkCList *clist)
{
  g_return_if_fail (clist != 0);
  g_return_if_fail (BTK_IS_CLIST (clist));
  
  if (clist_has_grab (clist) ||
      clist->selection_mode != BTK_SELECTION_MULTIPLE)
    return;

  btk_clist_draw_focus (BTK_WIDGET (clist));
  if (!BTK_CLIST_ADD_MODE(clist))
    {
      gint8 dashes[] = { 4, 4 };

      BTK_CLIST_SET_FLAG (clist, CLIST_ADD_MODE);
      bdk_gc_set_line_attributes (clist->xor_gc, 1,
				  BDK_LINE_ON_OFF_DASH, 0, 0);
      bdk_gc_set_dashes (clist->xor_gc, 0, dashes, G_N_ELEMENTS (dashes));
    }
  else
    {
      BTK_CLIST_UNSET_FLAG (clist, CLIST_ADD_MODE);
      bdk_gc_set_line_attributes (clist->xor_gc, 1, BDK_LINE_SOLID, 0, 0);
      clist->anchor_state = BTK_STATE_SELECTED;
    }
  btk_clist_draw_focus (BTK_WIDGET (clist));
}

static void
real_select_row (BtkCList *clist,
		 gint      row,
		 gint      column,
		 BdkEvent *event)
{
  BtkCListRow *clist_row;
  GList *list;
  gint sel_row;
  gboolean row_selected;

  g_return_if_fail (BTK_IS_CLIST (clist));

  if (row < 0 || row > (clist->rows - 1))
    return;

  switch (clist->selection_mode)
    {
    case BTK_SELECTION_SINGLE:
    case BTK_SELECTION_BROWSE:

      row_selected = FALSE;
      list = clist->selection;

      while (list)
	{
	  sel_row = GPOINTER_TO_INT (list->data);
	  list = list->next;

	  if (row == sel_row)
	    row_selected = TRUE;
	  else
	    btk_signal_emit (BTK_OBJECT (clist), clist_signals[UNSELECT_ROW], 
			     sel_row, column, event);
	}

      if (row_selected)
	return;
      
    default:
      break;
    }

  clist_row = ROW_ELEMENT (clist, row)->data;

  if (clist_row->state != BTK_STATE_NORMAL || !clist_row->selectable)
    return;

  clist_row->state = BTK_STATE_SELECTED;
  if (!clist->selection)
    {
      clist->selection = g_list_append (clist->selection,
					GINT_TO_POINTER (row));
      clist->selection_end = clist->selection;
    }
  else
    clist->selection_end = 
      g_list_append (clist->selection_end, GINT_TO_POINTER (row))->next;
  
  if (CLIST_UNFROZEN (clist)
      && (btk_clist_row_is_visible (clist, row) != BTK_VISIBILITY_NONE))
    BTK_CLIST_GET_CLASS (clist)->draw_row (clist, NULL, row, clist_row);
}

static void
real_unselect_row (BtkCList *clist,
		   gint      row,
		   gint      column,
		   BdkEvent *event)
{
  BtkCListRow *clist_row;

  g_return_if_fail (BTK_IS_CLIST (clist));

  if (row < 0 || row > (clist->rows - 1))
    return;

  clist_row = ROW_ELEMENT (clist, row)->data;

  if (clist_row->state == BTK_STATE_SELECTED)
    {
      clist_row->state = BTK_STATE_NORMAL;

      if (clist->selection_end && 
	  clist->selection_end->data == GINT_TO_POINTER (row))
	clist->selection_end = clist->selection_end->prev;

      clist->selection = g_list_remove (clist->selection,
					GINT_TO_POINTER (row));
      
      if (CLIST_UNFROZEN (clist)
	  && (btk_clist_row_is_visible (clist, row) != BTK_VISIBILITY_NONE))
	BTK_CLIST_GET_CLASS (clist)->draw_row (clist, NULL, row, clist_row);
    }
}

static void
real_select_all (BtkCList *clist)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (clist_has_grab (clist))
    return;

  switch (clist->selection_mode)
    {
    case BTK_SELECTION_SINGLE:
    case BTK_SELECTION_BROWSE:
      return;

    case BTK_SELECTION_MULTIPLE:
      g_list_free (clist->undo_selection);
      g_list_free (clist->undo_unselection);
      clist->undo_selection = NULL;
      clist->undo_unselection = NULL;
	  
      if (clist->rows &&
	  ((BtkCListRow *) (clist->row_list->data))->state !=
	  BTK_STATE_SELECTED)
	fake_toggle_row (clist, 0);

      clist->anchor_state =  BTK_STATE_SELECTED;
      clist->anchor = 0;
      clist->drag_pos = 0;
      clist->undo_anchor = clist->focus_row;
      update_extended_selection (clist, clist->rows);
      BTK_CLIST_GET_CLASS (clist)->resync_selection (clist, NULL);
      return;
    default:
      g_assert_not_reached ();
    }
}

static void
real_unselect_all (BtkCList *clist)
{
  GList *list;
  gint i;
 
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (clist_has_grab (clist))
    return;

  switch (clist->selection_mode)
    {
    case BTK_SELECTION_BROWSE:
      if (clist->focus_row >= 0)
	{
	  btk_signal_emit (BTK_OBJECT (clist),
			   clist_signals[SELECT_ROW],
			   clist->focus_row, -1, NULL);
	  return;
	}
      break;
    case BTK_SELECTION_MULTIPLE:
      g_list_free (clist->undo_selection);
      g_list_free (clist->undo_unselection);
      clist->undo_selection = NULL;
      clist->undo_unselection = NULL;

      clist->anchor = -1;
      clist->drag_pos = -1;
      clist->undo_anchor = clist->focus_row;
      break;
    default:
      break;
    }

  list = clist->selection;
  while (list)
    {
      i = GPOINTER_TO_INT (list->data);
      list = list->next;
      btk_signal_emit (BTK_OBJECT (clist),
		       clist_signals[UNSELECT_ROW], i, -1, NULL);
    }
}

static void
fake_unselect_all (BtkCList *clist,
		   gint      row)
{
  GList *list;
  GList *work;
  gint i;

  if (row >= 0 && (work = ROW_ELEMENT (clist, row)))
    {
      if (BTK_CLIST_ROW (work)->state == BTK_STATE_NORMAL &&
	  BTK_CLIST_ROW (work)->selectable)
	{
	  BTK_CLIST_ROW (work)->state = BTK_STATE_SELECTED;
	  
	  if (CLIST_UNFROZEN (clist) &&
	      btk_clist_row_is_visible (clist, row) != BTK_VISIBILITY_NONE)
	    BTK_CLIST_GET_CLASS (clist)->draw_row (clist, NULL, row,
						  BTK_CLIST_ROW (work));
	}  
    }

  clist->undo_selection = clist->selection;
  clist->selection = NULL;
  clist->selection_end = NULL;

  for (list = clist->undo_selection; list; list = list->next)
    {
      if ((i = GPOINTER_TO_INT (list->data)) == row ||
	  !(work = g_list_nth (clist->row_list, i)))
	continue;

      BTK_CLIST_ROW (work)->state = BTK_STATE_NORMAL;
      if (CLIST_UNFROZEN (clist) &&
	  btk_clist_row_is_visible (clist, i) != BTK_VISIBILITY_NONE)
	BTK_CLIST_GET_CLASS (clist)->draw_row (clist, NULL, i,
					      BTK_CLIST_ROW (work));
    }
}

static void
real_undo_selection (BtkCList *clist)
{
  GList *work;

  g_return_if_fail (BTK_IS_CLIST (clist));

  if (clist_has_grab (clist) ||
      clist->selection_mode != BTK_SELECTION_MULTIPLE)
    return;

  BTK_CLIST_GET_CLASS (clist)->resync_selection (clist, NULL);

  if (!(clist->undo_selection || clist->undo_unselection))
    {
      btk_clist_unselect_all (clist);
      return;
    }

  for (work = clist->undo_selection; work; work = work->next)
    btk_signal_emit (BTK_OBJECT (clist), clist_signals[SELECT_ROW],
		     GPOINTER_TO_INT (work->data), -1, NULL);

  for (work = clist->undo_unselection; work; work = work->next)
    {
      /* g_print ("unselect %d\n",GPOINTER_TO_INT (work->data)); */
      btk_signal_emit (BTK_OBJECT (clist), clist_signals[UNSELECT_ROW], 
		       GPOINTER_TO_INT (work->data), -1, NULL);
    }

  if (btk_widget_has_focus (BTK_WIDGET (clist)) && clist->focus_row != clist->undo_anchor)
    {
      btk_clist_draw_focus (BTK_WIDGET (clist));
      clist->focus_row = clist->undo_anchor;
      btk_clist_draw_focus (BTK_WIDGET (clist));
    }
  else
    clist->focus_row = clist->undo_anchor;
  
  clist->undo_anchor = -1;
 
  g_list_free (clist->undo_selection);
  g_list_free (clist->undo_unselection);
  clist->undo_selection = NULL;
  clist->undo_unselection = NULL;

  if (ROW_TOP_YPIXEL (clist, clist->focus_row) + clist->row_height >
      clist->clist_window_height)
    btk_clist_moveto (clist, clist->focus_row, -1, 1, 0);
  else if (ROW_TOP_YPIXEL (clist, clist->focus_row) < 0)
    btk_clist_moveto (clist, clist->focus_row, -1, 0, 0);
}

static void
set_anchor (BtkCList *clist,
	    gboolean  add_mode,
	    gint      anchor,
	    gint      undo_anchor)
{
  g_return_if_fail (BTK_IS_CLIST (clist));
  
  if (clist->selection_mode != BTK_SELECTION_MULTIPLE || clist->anchor >= 0)
    return;

  g_list_free (clist->undo_selection);
  g_list_free (clist->undo_unselection);
  clist->undo_selection = NULL;
  clist->undo_unselection = NULL;

  if (add_mode)
    fake_toggle_row (clist, anchor);
  else
    {
      BTK_CLIST_GET_CLASS (clist)->fake_unselect_all (clist, anchor);
      clist->anchor_state = BTK_STATE_SELECTED;
    }

  clist->anchor = anchor;
  clist->drag_pos = anchor;
  clist->undo_anchor = undo_anchor;
}

static void
resync_selection (BtkCList *clist,
		  BdkEvent *event)
{
  gint i;
  gint e;
  gint row;
  GList *list;
  BtkCListRow *clist_row;

  if (clist->selection_mode != BTK_SELECTION_MULTIPLE)
    return;

  if (clist->anchor < 0 || clist->drag_pos < 0)
    return;

  btk_clist_freeze (clist);

  i = MIN (clist->anchor, clist->drag_pos);
  e = MAX (clist->anchor, clist->drag_pos);

  if (clist->undo_selection)
    {
      list = clist->selection;
      clist->selection = clist->undo_selection;
      clist->selection_end = g_list_last (clist->selection);
      clist->undo_selection = list;
      list = clist->selection;
      while (list)
	{
	  row = GPOINTER_TO_INT (list->data);
	  list = list->next;
	  if (row < i || row > e)
	    {
	      clist_row = g_list_nth (clist->row_list, row)->data;
	      if (clist_row->selectable)
		{
		  clist_row->state = BTK_STATE_SELECTED;
		  btk_signal_emit (BTK_OBJECT (clist),
				   clist_signals[UNSELECT_ROW],
				   row, -1, event);
		  clist->undo_selection = g_list_prepend
		    (clist->undo_selection, GINT_TO_POINTER (row));
		}
	    }
	}
    }    

  if (clist->anchor < clist->drag_pos)
    {
      for (list = g_list_nth (clist->row_list, i); i <= e;
	   i++, list = list->next)
	if (BTK_CLIST_ROW (list)->selectable)
	  {
	    if (g_list_find (clist->selection, GINT_TO_POINTER(i)))
	      {
		if (BTK_CLIST_ROW (list)->state == BTK_STATE_NORMAL)
		  {
		    BTK_CLIST_ROW (list)->state = BTK_STATE_SELECTED;
		    btk_signal_emit (BTK_OBJECT (clist),
				     clist_signals[UNSELECT_ROW],
				     i, -1, event);
		    clist->undo_selection =
		      g_list_prepend (clist->undo_selection,
				      GINT_TO_POINTER (i));
		  }
	      }
	    else if (BTK_CLIST_ROW (list)->state == BTK_STATE_SELECTED)
	      {
		BTK_CLIST_ROW (list)->state = BTK_STATE_NORMAL;
		clist->undo_unselection =
		  g_list_prepend (clist->undo_unselection,
				  GINT_TO_POINTER (i));
	      }
	  }
    }
  else
    {
      for (list = g_list_nth (clist->row_list, e); i <= e;
	   e--, list = list->prev)
	if (BTK_CLIST_ROW (list)->selectable)
	  {
	    if (g_list_find (clist->selection, GINT_TO_POINTER(e)))
	      {
		if (BTK_CLIST_ROW (list)->state == BTK_STATE_NORMAL)
		  {
		    BTK_CLIST_ROW (list)->state = BTK_STATE_SELECTED;
		    btk_signal_emit (BTK_OBJECT (clist),
				     clist_signals[UNSELECT_ROW],
				     e, -1, event);
		    clist->undo_selection =
		      g_list_prepend (clist->undo_selection,
				      GINT_TO_POINTER (e));
		  }
	      }
	    else if (BTK_CLIST_ROW (list)->state == BTK_STATE_SELECTED)
	      {
		BTK_CLIST_ROW (list)->state = BTK_STATE_NORMAL;
		clist->undo_unselection =
		  g_list_prepend (clist->undo_unselection,
				  GINT_TO_POINTER (e));
	      }
	  }
    }
  
  clist->undo_unselection = g_list_reverse (clist->undo_unselection);
  for (list = clist->undo_unselection; list; list = list->next)
    btk_signal_emit (BTK_OBJECT (clist), clist_signals[SELECT_ROW],
		     GPOINTER_TO_INT (list->data), -1, event);

  clist->anchor = -1;
  clist->drag_pos = -1;

  btk_clist_thaw (clist);
}

static void
update_extended_selection (BtkCList *clist,
			   gint      row)
{
  gint i;
  GList *list;
  BdkRectangle area;
  gint s1 = -1;
  gint s2 = -1;
  gint e1 = -1;
  gint e2 = -1;
  gint y1 = clist->clist_window_height;
  gint y2 = clist->clist_window_height;
  gint h1 = 0;
  gint h2 = 0;
  gint top;

  if (clist->selection_mode != BTK_SELECTION_MULTIPLE || clist->anchor == -1)
    return;

  if (row < 0)
    row = 0;
  if (row >= clist->rows)
    row = clist->rows - 1;

  /* extending downwards */
  if (row > clist->drag_pos && clist->anchor <= clist->drag_pos)
    {
      s2 = clist->drag_pos + 1;
      e2 = row;
    }
  /* extending upwards */
  else if (row < clist->drag_pos && clist->anchor >= clist->drag_pos)
    {
      s2 = row;
      e2 = clist->drag_pos - 1;
    }
  else if (row < clist->drag_pos && clist->anchor < clist->drag_pos)
    {
      e1 = clist->drag_pos;
      /* row and drag_pos on different sides of anchor :
	 take back the selection between anchor and drag_pos,
         select between anchor and row */
      if (row < clist->anchor)
	{
	  s1 = clist->anchor + 1;
	  s2 = row;
	  e2 = clist->anchor - 1;
	}
      /* take back the selection between anchor and drag_pos */
      else
	s1 = row + 1;
    }
  else if (row > clist->drag_pos && clist->anchor > clist->drag_pos)
    {
      s1 = clist->drag_pos;
      /* row and drag_pos on different sides of anchor :
	 take back the selection between anchor and drag_pos,
         select between anchor and row */
      if (row > clist->anchor)
	{
	  e1 = clist->anchor - 1;
	  s2 = clist->anchor + 1;
	  e2 = row;
	}
      /* take back the selection between anchor and drag_pos */
      else
	e1 = row - 1;
    }

  clist->drag_pos = row;

  area.x = 0;
  area.width = clist->clist_window_width;

  /* restore the elements between s1 and e1 */
  if (s1 >= 0)
    {
      for (i = s1, list = g_list_nth (clist->row_list, i); i <= e1;
	   i++, list = list->next)
	if (BTK_CLIST_ROW (list)->selectable)
	  {
	    if (BTK_CLIST_GET_CLASS (clist)->selection_find (clist, i, list))
	      BTK_CLIST_ROW (list)->state = BTK_STATE_SELECTED;
	    else
	      BTK_CLIST_ROW (list)->state = BTK_STATE_NORMAL;
	  }

      top = ROW_TOP_YPIXEL (clist, clist->focus_row);

      if (top + clist->row_height <= 0)
	{
	  area.y = 0;
	  area.height = ROW_TOP_YPIXEL (clist, e1) + clist->row_height;
	  draw_rows (clist, &area);
	  btk_clist_moveto (clist, clist->focus_row, -1, 0, 0);
	}
      else if (top >= clist->clist_window_height)
	{
	  area.y = ROW_TOP_YPIXEL (clist, s1) - 1;
	  area.height = clist->clist_window_height - area.y;
	  draw_rows (clist, &area);
	  btk_clist_moveto (clist, clist->focus_row, -1, 1, 0);
	}
      else if (top < 0)
	btk_clist_moveto (clist, clist->focus_row, -1, 0, 0);
      else if (top + clist->row_height > clist->clist_window_height)
	btk_clist_moveto (clist, clist->focus_row, -1, 1, 0);

      y1 = ROW_TOP_YPIXEL (clist, s1) - 1;
      h1 = (e1 - s1 + 1) * (clist->row_height + CELL_SPACING);
    }

  /* extend the selection between s2 and e2 */
  if (s2 >= 0)
    {
      for (i = s2, list = g_list_nth (clist->row_list, i); i <= e2;
	   i++, list = list->next)
	if (BTK_CLIST_ROW (list)->selectable &&
	    BTK_CLIST_ROW (list)->state != clist->anchor_state)
	  BTK_CLIST_ROW (list)->state = clist->anchor_state;

      top = ROW_TOP_YPIXEL (clist, clist->focus_row);

      if (top + clist->row_height <= 0)
	{
	  area.y = 0;
	  area.height = ROW_TOP_YPIXEL (clist, e2) + clist->row_height;
	  draw_rows (clist, &area);
	  btk_clist_moveto (clist, clist->focus_row, -1, 0, 0);
	}
      else if (top >= clist->clist_window_height)
	{
	  area.y = ROW_TOP_YPIXEL (clist, s2) - 1;
	  area.height = clist->clist_window_height - area.y;
	  draw_rows (clist, &area);
	  btk_clist_moveto (clist, clist->focus_row, -1, 1, 0);
	}
      else if (top < 0)
	btk_clist_moveto (clist, clist->focus_row, -1, 0, 0);
      else if (top + clist->row_height > clist->clist_window_height)
	btk_clist_moveto (clist, clist->focus_row, -1, 1, 0);

      y2 = ROW_TOP_YPIXEL (clist, s2) - 1;
      h2 = (e2 - s2 + 1) * (clist->row_height + CELL_SPACING);
    }

  area.y = MAX (0, MIN (y1, y2));
  if (area.y > clist->clist_window_height)
    area.y = 0;
  area.height = MIN (clist->clist_window_height, h1 + h2);
  if (s1 >= 0 && s2 >= 0)
    area.height += (clist->row_height + CELL_SPACING);
  draw_rows (clist, &area);
}

static void
start_selection (BtkCList *clist)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (clist_has_grab (clist))
    return;

  set_anchor (clist, BTK_CLIST_ADD_MODE(clist), clist->focus_row,
	      clist->focus_row);
}

static void
end_selection (BtkCList *clist)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (bdk_display_pointer_is_grabbed (btk_widget_get_display (BTK_WIDGET (clist))) &&
      btk_widget_has_focus (BTK_WIDGET (clist)))
    return;

  BTK_CLIST_GET_CLASS (clist)->resync_selection (clist, NULL);
}

static void
extend_selection (BtkCList      *clist,
		  BtkScrollType  scroll_type,
		  gfloat         position,
		  gboolean       auto_start_selection)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (clist_has_grab (clist) ||
      clist->selection_mode != BTK_SELECTION_MULTIPLE)
    return;

  if (auto_start_selection)
    set_anchor (clist, BTK_CLIST_ADD_MODE(clist), clist->focus_row,
		clist->focus_row);
  else if (clist->anchor == -1)
    return;

  move_focus_row (clist, scroll_type, position);

  if (ROW_TOP_YPIXEL (clist, clist->focus_row) + clist->row_height >
      clist->clist_window_height)
    btk_clist_moveto (clist, clist->focus_row, -1, 1, 0);
  else if (ROW_TOP_YPIXEL (clist, clist->focus_row) < 0)
    btk_clist_moveto (clist, clist->focus_row, -1, 0, 0);

  update_extended_selection (clist, clist->focus_row);
}

static void
sync_selection (BtkCList *clist,
		gint      row,
		gint      mode)
{
  GList *list;
  gint d;

  if (mode == SYNC_INSERT)
    d = 1;
  else
    d = -1;
      
  if (clist->focus_row >= row)
    {
      if (d > 0 || clist->focus_row > row)
	clist->focus_row += d;
      if (clist->focus_row == -1 && clist->rows >= 1)
	clist->focus_row = 0;
      else if (d < 0 && clist->focus_row >= clist->rows - 1)
	clist->focus_row = clist->rows - 2;
      else if (clist->focus_row >= clist->rows)	/* Paranoia */
	clist->focus_row = clist->rows - 1;
    }

  BTK_CLIST_GET_CLASS (clist)->resync_selection (clist, NULL);

  g_list_free (clist->undo_selection);
  g_list_free (clist->undo_unselection);
  clist->undo_selection = NULL;
  clist->undo_unselection = NULL;

  clist->anchor = -1;
  clist->drag_pos = -1;
  clist->undo_anchor = clist->focus_row;

  list = clist->selection;

  while (list)
    {
      if (GPOINTER_TO_INT (list->data) >= row)
	list->data = ((gchar*) list->data) + d;
      list = list->next;
    }
}

/* BTKOBJECT
 *   btk_clist_destroy
 *   btk_clist_finalize
 */
static void
btk_clist_destroy (BtkObject *object)
{
  gint i;
  BtkCList *clist;

  g_return_if_fail (BTK_IS_CLIST (object));

  clist = BTK_CLIST (object);

  /* freeze the list */
  clist->freeze_count++;

  /* get rid of all the rows */
  btk_clist_clear (clist);

  /* Since we don't have a _remove method, unparent the children
   * instead of destroying them so the focus will be unset properly.
   * (For other containers, the _remove method takes care of the
   * unparent) The destroy will happen when the refcount drops
   * to zero.
   */

  /* unref adjustments */
  if (clist->hadjustment)
    {
      btk_signal_disconnect_by_data (BTK_OBJECT (clist->hadjustment), clist);
      g_object_unref (clist->hadjustment);
      clist->hadjustment = NULL;
    }
  if (clist->vadjustment)
    {
      btk_signal_disconnect_by_data (BTK_OBJECT (clist->vadjustment), clist);
      g_object_unref (clist->vadjustment);
      clist->vadjustment = NULL;
    }

  remove_grab (clist);

  /* destroy the column buttons */
  for (i = 0; i < clist->columns; i++)
    if (clist->column[i].button)
      {
	btk_widget_unparent (clist->column[i].button);
	clist->column[i].button = NULL;
      }

  BTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
btk_clist_finalize (GObject *object)
{
  BtkCList *clist;

  g_return_if_fail (BTK_IS_CLIST (object));

  clist = BTK_CLIST (object);

  columns_delete (clist);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* BTKWIDGET
 *   btk_clist_realize
 *   btk_clist_unrealize
 *   btk_clist_map
 *   btk_clist_unmap
 *   btk_clist_expose
 *   btk_clist_style_set
 *   btk_clist_button_press
 *   btk_clist_button_release
 *   btk_clist_motion
 *   btk_clist_size_request
 *   btk_clist_size_allocate
 */
static void
btk_clist_realize (BtkWidget *widget)
{
  BtkCList *clist;
  BdkWindowAttr attributes;
  BdkGCValues values;
  BtkCListRow *clist_row;
  GList *list;
  gint attributes_mask;
  gint border_width;
  gint i;
  gint j;

  g_return_if_fail (BTK_IS_CLIST (widget));

  clist = BTK_CLIST (widget);

  btk_widget_set_realized (widget, TRUE);

  border_width = BTK_CONTAINER (widget)->border_width;
  
  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x + border_width;
  attributes.y = widget->allocation.y + border_width;
  attributes.width = widget->allocation.width - border_width * 2;
  attributes.height = widget->allocation.height - border_width * 2;
  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  attributes.event_mask = btk_widget_get_events (widget);
  attributes.event_mask |= (BDK_EXPOSURE_MASK |
			    BDK_BUTTON_PRESS_MASK |
			    BDK_BUTTON_RELEASE_MASK |
			    BDK_KEY_RELEASE_MASK);
  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;

  /* main window */
  widget->window = bdk_window_new (btk_widget_get_parent_window (widget),
				   &attributes, attributes_mask);
  bdk_window_set_user_data (widget->window, clist);

  widget->style = btk_style_attach (widget->style, widget->window);

  btk_style_set_background (widget->style, widget->window, BTK_STATE_NORMAL);

  /* column-title window */

  attributes.x = clist->column_title_area.x;
  attributes.y = clist->column_title_area.y;
  attributes.width = clist->column_title_area.width;
  attributes.height = clist->column_title_area.height;
  
  clist->title_window = bdk_window_new (widget->window, &attributes,
					attributes_mask);
  bdk_window_set_user_data (clist->title_window, clist);

  btk_style_set_background (widget->style, clist->title_window,
			    BTK_STATE_NORMAL);
  bdk_window_show (clist->title_window);

  /* set things up so column buttons are drawn in title window */
  for (i = 0; i < clist->columns; i++)
    if (clist->column[i].button)
      btk_widget_set_parent_window (clist->column[i].button,
				    clist->title_window);

  /* clist-window */
  attributes.x = (clist->internal_allocation.x +
		  widget->style->xthickness);
  attributes.y = (clist->internal_allocation.y +
		  widget->style->ythickness +
		  clist->column_title_area.height);
  attributes.width = clist->clist_window_width;
  attributes.height = clist->clist_window_height;
  
  clist->clist_window = bdk_window_new (widget->window, &attributes,
					attributes_mask);
  bdk_window_set_user_data (clist->clist_window, clist);

  bdk_window_set_background (clist->clist_window,
			     &widget->style->base[BTK_STATE_NORMAL]);
  bdk_window_show (clist->clist_window);
  bdk_drawable_get_size (clist->clist_window, &clist->clist_window_width,
                         &clist->clist_window_height);

  /* create resize windows */
  attributes.wclass = BDK_INPUT_ONLY;
  attributes.event_mask = (BDK_BUTTON_PRESS_MASK |
			   BDK_BUTTON_RELEASE_MASK |
			   BDK_POINTER_MOTION_MASK |
			   BDK_POINTER_MOTION_HINT_MASK);
  attributes_mask = BDK_WA_CURSOR;
  attributes.cursor = bdk_cursor_new_for_display (btk_widget_get_display (widget),
						  BDK_SB_H_DOUBLE_ARROW);
  clist->cursor_drag = attributes.cursor;

  attributes.x =  LIST_WIDTH (clist) + 1;
  attributes.y = 0;
  attributes.width = 0;
  attributes.height = 0;

  for (i = 0; i < clist->columns; i++)
    {
      clist->column[i].window = bdk_window_new (clist->title_window,
						&attributes, attributes_mask);
      bdk_window_set_user_data (clist->column[i].window, clist);
    }

  /* This is slightly less efficient than creating them with the
   * right size to begin with, but easier
   */
  size_allocate_title_buttons (clist);

  /* GCs */
  clist->fg_gc = bdk_gc_new (widget->window);
  clist->bg_gc = bdk_gc_new (widget->window);
  
  /* We'll use this gc to do scrolling as well */
  bdk_gc_set_exposures (clist->fg_gc, TRUE);

  values.foreground = (widget->style->white.pixel==0 ?
		       widget->style->black:widget->style->white);
  values.function = BDK_XOR;
  values.subwindow_mode = BDK_INCLUDE_INFERIORS;
  clist->xor_gc = bdk_gc_new_with_values (widget->window,
					  &values,
					  BDK_GC_FOREGROUND |
					  BDK_GC_FUNCTION |
					  BDK_GC_SUBWINDOW);

  /* attach optional row/cell styles, allocate foreground/background colors */
  list = clist->row_list;
  for (i = 0; i < clist->rows; i++)
    {
      clist_row = list->data;
      list = list->next;

      if (clist_row->style)
	clist_row->style = btk_style_attach (clist_row->style,
					     clist->clist_window);

      if (clist_row->fg_set || clist_row->bg_set)
	{
	  BdkColormap *colormap;

	  colormap = btk_widget_get_colormap (widget);
	  if (clist_row->fg_set)
	    bdk_colormap_alloc_color (colormap, &clist_row->foreground,
                                      FALSE, TRUE);
	  if (clist_row->bg_set)
	    bdk_colormap_alloc_color (colormap, &clist_row->background,
                                      FALSE, TRUE);
	}
      
      for (j = 0; j < clist->columns; j++)
	if  (clist_row->cell[j].style)
	  clist_row->cell[j].style =
	    btk_style_attach (clist_row->cell[j].style, clist->clist_window);
    }
}

static void
btk_clist_unrealize (BtkWidget *widget)
{
  gint i;
  BtkCList *clist;

  g_return_if_fail (BTK_IS_CLIST (widget));

  clist = BTK_CLIST (widget);

  /* freeze the list */
  clist->freeze_count++;

  if (btk_widget_get_mapped (widget))
    btk_clist_unmap (widget);

  btk_widget_set_mapped (widget, FALSE);

  /* detach optional row/cell styles */
  if (btk_widget_get_realized (widget))
    {
      BtkCListRow *clist_row;
      GList *list;
      gint j;

      list = clist->row_list;
      for (i = 0; i < clist->rows; i++)
	{
	  clist_row = list->data;
	  list = list->next;

	  if (clist_row->style)
	    btk_style_detach (clist_row->style);
	  for (j = 0; j < clist->columns; j++)
	    if  (clist_row->cell[j].style)
	      btk_style_detach (clist_row->cell[j].style);
	}
    }

  bdk_cursor_unref (clist->cursor_drag);
  g_object_unref (clist->xor_gc);
  g_object_unref (clist->fg_gc);
  g_object_unref (clist->bg_gc);

  for (i = 0; i < clist->columns; i++)
    {
      if (clist->column[i].button)
	btk_widget_unrealize (clist->column[i].button);
      if (clist->column[i].window)
	{
	  bdk_window_set_user_data (clist->column[i].window, NULL);
	  bdk_window_destroy (clist->column[i].window);
	  clist->column[i].window = NULL;
	}
    }

  bdk_window_set_user_data (clist->clist_window, NULL);
  bdk_window_destroy (clist->clist_window);
  clist->clist_window = NULL;

  bdk_window_set_user_data (clist->title_window, NULL);
  bdk_window_destroy (clist->title_window);
  clist->title_window = NULL;

  clist->cursor_drag = NULL;
  clist->xor_gc = NULL;
  clist->fg_gc = NULL;
  clist->bg_gc = NULL;

  BTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
btk_clist_map (BtkWidget *widget)
{
  gint i;
  BtkCList *clist;

  g_return_if_fail (BTK_IS_CLIST (widget));

  clist = BTK_CLIST (widget);

  if (!btk_widget_get_mapped (widget))
    {
      btk_widget_set_mapped (widget, TRUE);

      /* map column buttons */
      for (i = 0; i < clist->columns; i++)
	{
	  if (clist->column[i].button &&
	      btk_widget_get_visible (clist->column[i].button) &&
	      !btk_widget_get_mapped (clist->column[i].button))
	    btk_widget_map (clist->column[i].button);
	}
      
      for (i = 0; i < clist->columns; i++)
	if (clist->column[i].window && clist->column[i].button)
	  {
	    bdk_window_raise (clist->column[i].window);
	    bdk_window_show (clist->column[i].window);
	  }

      bdk_window_show (clist->title_window);
      bdk_window_show (clist->clist_window);
      bdk_window_show (widget->window);

      /* unfreeze the list */
      clist->freeze_count = 0;
    }
}

static void
btk_clist_unmap (BtkWidget *widget)
{
  gint i;
  BtkCList *clist;

  g_return_if_fail (BTK_IS_CLIST (widget));

  clist = BTK_CLIST (widget);

  if (btk_widget_get_mapped (widget))
    {
      btk_widget_set_mapped (widget, FALSE);

      if (clist_has_grab (clist))
	{
	  remove_grab (clist);

	  BTK_CLIST_GET_CLASS (widget)->resync_selection (clist, NULL);

	  clist->click_cell.row = -1;
	  clist->click_cell.column = -1;
	  clist->drag_button = 0;

	  if (BTK_CLIST_IN_DRAG(clist))
	    {
	      gpointer drag_data;

	      BTK_CLIST_UNSET_FLAG (clist, CLIST_IN_DRAG);
	      drag_data = btk_object_get_data (BTK_OBJECT (clist),
					       "btk-site-data");
	      if (drag_data)
		btk_signal_handler_unblock_by_data (BTK_OBJECT (clist),
						    drag_data);
	    }
	}

      for (i = 0; i < clist->columns; i++)
	if (clist->column[i].window)
	  bdk_window_hide (clist->column[i].window);

      bdk_window_hide (clist->clist_window);
      bdk_window_hide (clist->title_window);
      bdk_window_hide (widget->window);

      /* unmap column buttons */
      for (i = 0; i < clist->columns; i++)
	if (clist->column[i].button &&
	    btk_widget_get_mapped (clist->column[i].button))
	  btk_widget_unmap (clist->column[i].button);

      /* freeze the list */
      clist->freeze_count++;
    }
}

static gint
btk_clist_expose (BtkWidget      *widget,
		  BdkEventExpose *event)
{
  BtkCList *clist;

  g_return_val_if_fail (BTK_IS_CLIST (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (BTK_WIDGET_DRAWABLE (widget))
    {
      clist = BTK_CLIST (widget);

      /* draw border */
      if (event->window == widget->window)
	btk_draw_shadow (widget->style, widget->window,
			 BTK_STATE_NORMAL, clist->shadow_type,
			 0, 0,
			 clist->clist_window_width +
			 (2 * widget->style->xthickness),
			 clist->clist_window_height +
			 (2 * widget->style->ythickness) +
			 clist->column_title_area.height);

      /* exposure events on the list */
      if (event->window == clist->clist_window)
	draw_rows (clist, &event->area);

      if (event->window == clist->clist_window &&
	  clist->drag_highlight_row >= 0)
	BTK_CLIST_GET_CLASS (clist)->draw_drag_highlight
	  (clist, g_list_nth (clist->row_list,
			      clist->drag_highlight_row)->data,
	   clist->drag_highlight_row, clist->drag_highlight_pos);

      if (event->window == clist->title_window)
	{
	  gint i;
	  
	  for (i = 0; i < clist->columns; i++)
	    {
	      if (clist->column[i].button)
		btk_container_propagate_expose (BTK_CONTAINER (clist),
						clist->column[i].button,
						event);
	    }
	}
    }

  return FALSE;
}

static void
btk_clist_style_set (BtkWidget *widget,
		     BtkStyle  *previous_style)
{
  BtkCList *clist = BTK_CLIST (widget);

  BTK_WIDGET_CLASS (parent_class)->style_set (widget, previous_style);

  if (btk_widget_get_realized (widget))
    {
      btk_style_set_background (widget->style, widget->window, widget->state);
      btk_style_set_background (widget->style, clist->title_window, BTK_STATE_SELECTED);
      bdk_window_set_background (clist->clist_window, &widget->style->base[BTK_STATE_NORMAL]);
    }

  /* Fill in data after widget has correct style */

  /* text properties */
  if (!BTK_CLIST_ROW_HEIGHT_SET(clist))
    /* Reset clist->row_height */
    btk_clist_set_row_height (clist, 0);

  /* Column widths */
  if (!BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    {
      gint width;
      gint i;

      for (i = 0; i < clist->columns; i++)
	if (clist->column[i].auto_resize)
	  {
	    width = btk_clist_optimal_column_width (clist, i);
	    if (width != clist->column[i].width)
	      btk_clist_set_column_width (clist, i, width);
	  }
    }
}

static gint
btk_clist_button_press (BtkWidget      *widget,
			BdkEventButton *event)
{
  gint i;
  BtkCList *clist;
  gint x;
  gint y;
  gint row;
  gint column;
  gint button_actions;

  g_return_val_if_fail (BTK_IS_CLIST (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  clist = BTK_CLIST (widget);

  button_actions = clist->button_actions[event->button - 1];

  if (button_actions == BTK_BUTTON_IGNORED)
    return FALSE;

  /* selections on the list */
  if (event->window == clist->clist_window)
    {
      x = event->x;
      y = event->y;

      if (get_selection_info (clist, x, y, &row, &column))
	{
	  gint old_row = clist->focus_row;

	  if (clist->focus_row == -1)
	    old_row = row;

	  if (event->type == BDK_BUTTON_PRESS)
	    {
	      BdkEventMask mask = ((1 << (4 + event->button)) |
				   BDK_POINTER_MOTION_HINT_MASK |
				   BDK_BUTTON_RELEASE_MASK);

	      if (bdk_pointer_grab (clist->clist_window, FALSE, mask,
				    NULL, NULL, event->time))
		return FALSE;
	      btk_grab_add (widget);

	      clist->click_cell.row = row;
	      clist->click_cell.column = column;
	      clist->drag_button = event->button;
	    }
	  else
	    {
	      clist->click_cell.row = -1;
	      clist->click_cell.column = -1;

	      clist->drag_button = 0;
	      remove_grab (clist);
	    }

	  if (button_actions & BTK_BUTTON_SELECTS)
	    {
	      if (BTK_CLIST_ADD_MODE(clist))
		{
		  BTK_CLIST_UNSET_FLAG (clist, CLIST_ADD_MODE);
		  if (btk_widget_has_focus (widget))
		    {
		      btk_clist_draw_focus (widget);
		      bdk_gc_set_line_attributes (clist->xor_gc, 1,
						  BDK_LINE_SOLID, 0, 0);
		      clist->focus_row = row;
		      btk_clist_draw_focus (widget);
		    }
		  else
		    {
		      bdk_gc_set_line_attributes (clist->xor_gc, 1,
						  BDK_LINE_SOLID, 0, 0);
		      clist->focus_row = row;
		    }
		}
	      else if (row != clist->focus_row)
		{
		  if (btk_widget_has_focus (widget))
		    {
		      btk_clist_draw_focus (widget);
		      clist->focus_row = row;
		      btk_clist_draw_focus (widget);
		    }
		  else
		    clist->focus_row = row;
		}
	    }

	  if (!btk_widget_has_focus (widget))
	    btk_widget_grab_focus (widget);

	  if (button_actions & BTK_BUTTON_SELECTS)
	    {
	      switch (clist->selection_mode)
		{
		case BTK_SELECTION_SINGLE:
		  if (event->type != BDK_BUTTON_PRESS)
		    {
		      btk_signal_emit (BTK_OBJECT (clist),
				       clist_signals[SELECT_ROW],
				       row, column, event);
		      clist->anchor = -1;
		    }
		  else
		    clist->anchor = row;
		  break;
		case BTK_SELECTION_BROWSE:
		  btk_signal_emit (BTK_OBJECT (clist),
				   clist_signals[SELECT_ROW],
				   row, column, event);
		  break;
		case BTK_SELECTION_MULTIPLE:
		  if (event->type != BDK_BUTTON_PRESS)
		    {
		      if (clist->anchor != -1)
			{
			  update_extended_selection (clist, clist->focus_row);
			  BTK_CLIST_GET_CLASS (clist)->resync_selection
			    (clist, (BdkEvent *) event);
			}
		      btk_signal_emit (BTK_OBJECT (clist),
				       clist_signals[SELECT_ROW],
				       row, column, event);
		      break;
		    }
	      
		  if (event->state & BDK_CONTROL_MASK)
		    {
		      if (event->state & BDK_SHIFT_MASK)
			{
			  if (clist->anchor < 0)
			    {
			      g_list_free (clist->undo_selection);
			      g_list_free (clist->undo_unselection);
			      clist->undo_selection = NULL;
			      clist->undo_unselection = NULL;
			      clist->anchor = old_row;
			      clist->drag_pos = old_row;
			      clist->undo_anchor = old_row;
			    }
			  update_extended_selection (clist, clist->focus_row);
			}
		      else
			{
			  if (clist->anchor == -1)
			    set_anchor (clist, TRUE, row, old_row);
			  else
			    update_extended_selection (clist,
						       clist->focus_row);
			}
		      break;
		    }

		  if (event->state & BDK_SHIFT_MASK)
		    {
		      set_anchor (clist, FALSE, old_row, old_row);
		      update_extended_selection (clist, clist->focus_row);
		      break;
		    }

		  if (clist->anchor == -1)
		    set_anchor (clist, FALSE, row, old_row);
		  else
		    update_extended_selection (clist, clist->focus_row);
		  break;
		default:
		  break;
		}
	    }
	}
      return TRUE;
    }

  /* press on resize windows */
  for (i = 0; i < clist->columns; i++)
    if (clist->column[i].resizeable && clist->column[i].window &&
	event->window == clist->column[i].window)
      {
	gpointer drag_data;

	if (bdk_pointer_grab (clist->column[i].window, FALSE,
			      BDK_POINTER_MOTION_HINT_MASK |
			      BDK_BUTTON1_MOTION_MASK |
			      BDK_BUTTON_RELEASE_MASK,
			      NULL, NULL, event->time))
	  return FALSE;

	btk_grab_add (widget);
	BTK_CLIST_SET_FLAG (clist, CLIST_IN_DRAG);

	/* block attached dnd signal handler */
	drag_data = btk_object_get_data (BTK_OBJECT (clist), "btk-site-data");
	if (drag_data)
	  btk_signal_handler_block_by_data (BTK_OBJECT (clist), drag_data);

	if (!btk_widget_has_focus (widget))
	  btk_widget_grab_focus (widget);

	clist->drag_pos = i;
	clist->x_drag = (COLUMN_LEFT_XPIXEL(clist, i) + COLUMN_INSET +
			 clist->column[i].area.width + CELL_SPACING);

	if (BTK_CLIST_ADD_MODE(clist))
	  bdk_gc_set_line_attributes (clist->xor_gc, 1, BDK_LINE_SOLID, 0, 0);
	draw_xor_line (clist);

        return TRUE;
      }

  return FALSE;
}

static gint
btk_clist_button_release (BtkWidget      *widget,
			  BdkEventButton *event)
{
  BtkCList *clist;
  gint button_actions;

  g_return_val_if_fail (BTK_IS_CLIST (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  clist = BTK_CLIST (widget);

  button_actions = clist->button_actions[event->button - 1];
  if (button_actions == BTK_BUTTON_IGNORED)
    return FALSE;

  /* release on resize windows */
  if (BTK_CLIST_IN_DRAG(clist))
    {
      gpointer drag_data;
      gint width;
      gint x;
      gint i;

      i = clist->drag_pos;
      clist->drag_pos = -1;

      /* unblock attached dnd signal handler */
      drag_data = btk_object_get_data (BTK_OBJECT (clist), "btk-site-data");
      if (drag_data)
	btk_signal_handler_unblock_by_data (BTK_OBJECT (clist), drag_data);

      BTK_CLIST_UNSET_FLAG (clist, CLIST_IN_DRAG);
      btk_widget_get_pointer (widget, &x, NULL);
      btk_grab_remove (widget);
      bdk_display_pointer_ungrab (btk_widget_get_display (widget), event->time);

      if (clist->x_drag >= 0)
	draw_xor_line (clist);

      if (BTK_CLIST_ADD_MODE(clist))
	{
	  gint8 dashes[] = { 4, 4 };

	  bdk_gc_set_line_attributes (clist->xor_gc, 1,
				      BDK_LINE_ON_OFF_DASH, 0, 0);
	  bdk_gc_set_dashes (clist->xor_gc, 0, dashes, G_N_ELEMENTS (dashes));
	}

      width = new_column_width (clist, i, &x);
      btk_clist_set_column_width (clist, i, width);

      return TRUE;
    }

  if (clist->drag_button == event->button)
    {
      gint row;
      gint column;

      clist->drag_button = 0;
      clist->click_cell.row = -1;
      clist->click_cell.column = -1;

      remove_grab (clist);

      if (button_actions & BTK_BUTTON_SELECTS)
	{
	  switch (clist->selection_mode)
	    {
	    case BTK_SELECTION_MULTIPLE:
	      if (!(event->state & BDK_SHIFT_MASK) ||
		  !BTK_WIDGET_CAN_FOCUS (widget) ||
		  event->x < 0 || event->x >= clist->clist_window_width ||
		  event->y < 0 || event->y >= clist->clist_window_height)
		BTK_CLIST_GET_CLASS (clist)->resync_selection
		  (clist, (BdkEvent *) event);
	      break;
	    case BTK_SELECTION_SINGLE:
	      if (get_selection_info (clist, event->x, event->y,
				      &row, &column))
		{
		  if (row >= 0 && row < clist->rows && clist->anchor == row)
		    toggle_row (clist, row, column, (BdkEvent *) event);
		}
	      clist->anchor = -1;
	      break;
	    default:
	      break;
	    }
	}

      return TRUE;
    }
  
  return FALSE;
}

static gint
btk_clist_motion (BtkWidget      *widget,
		  BdkEventMotion *event)
{
  BtkCList *clist;
  gint x;
  gint y;
  gint row;
  gint new_width;
  gint button_actions = 0;

  g_return_val_if_fail (BTK_IS_CLIST (widget), FALSE);

  clist = BTK_CLIST (widget);
  if (!clist_has_grab (clist))
    return FALSE;

  if (clist->drag_button > 0)
    button_actions = clist->button_actions[clist->drag_button - 1];

  if (BTK_CLIST_IN_DRAG(clist))
    {
      if (event->is_hint || event->window != widget->window)
	btk_widget_get_pointer (widget, &x, NULL);
      else
	x = event->x;
      
      new_width = new_column_width (clist, clist->drag_pos, &x);
      if (x != clist->x_drag)
	{
	  /* x_drag < 0 indicates that the xor line is already invisible */
	  if (clist->x_drag >= 0)
	    draw_xor_line (clist);

	  clist->x_drag = x;

	  if (clist->x_drag >= 0)
	    draw_xor_line (clist);
	}

      if (new_width <= MAX (COLUMN_MIN_WIDTH + 1,
			    clist->column[clist->drag_pos].min_width + 1))
	{
	  if (COLUMN_LEFT_XPIXEL (clist, clist->drag_pos) < 0 && x < 0)
	    btk_clist_moveto (clist, -1, clist->drag_pos, 0, 0);
	  return FALSE;
	}
      if (clist->column[clist->drag_pos].max_width >= COLUMN_MIN_WIDTH &&
	  new_width >= clist->column[clist->drag_pos].max_width)
	{
	  if (COLUMN_LEFT_XPIXEL (clist, clist->drag_pos) + new_width >
	      clist->clist_window_width && x < 0)
	    move_horizontal (clist,
			     COLUMN_LEFT_XPIXEL (clist, clist->drag_pos) +
			     new_width - clist->clist_window_width +
			     COLUMN_INSET + CELL_SPACING);
	  return FALSE;
	}
    }

  if (event->is_hint || event->window != clist->clist_window)
    bdk_window_get_pointer (clist->clist_window, &x, &y, NULL);
  else
    {
      x = event->x;
      y = event->y;
    }

  if (BTK_CLIST_REORDERABLE(clist) && button_actions & BTK_BUTTON_DRAGS)
    {
      /* delayed drag start */
      if (event->window == clist->clist_window &&
	  clist->click_cell.row >= 0 && clist->click_cell.column >= 0 &&
	  (y < 0 || y >= clist->clist_window_height ||
	   x < 0 || x >= clist->clist_window_width  ||
	   y < ROW_TOP_YPIXEL (clist, clist->click_cell.row) ||
	   y >= (ROW_TOP_YPIXEL (clist, clist->click_cell.row) +
		 clist->row_height) ||
	   x < COLUMN_LEFT_XPIXEL (clist, clist->click_cell.column) ||
	   x >= (COLUMN_LEFT_XPIXEL(clist, clist->click_cell.column) + 
		 clist->column[clist->click_cell.column].area.width)))
	{
	  BtkTargetList  *target_list;

	  target_list = btk_target_list_new (&clist_target_table, 1);
	  btk_drag_begin (widget, target_list, BDK_ACTION_MOVE,
			  clist->drag_button, (BdkEvent *)event);

	}
      return TRUE;
    }

  /* horizontal autoscrolling */
  if (clist->hadjustment && LIST_WIDTH (clist) > clist->clist_window_width &&
      (x < 0 || x >= clist->clist_window_width))
    {
      if (clist->htimer)
	return FALSE;

      clist->htimer = bdk_threads_add_timeout
	(SCROLL_TIME, (GSourceFunc) horizontal_timeout, clist);

      if (!((x < 0 && clist->hadjustment->value == 0) ||
	    (x >= clist->clist_window_width &&
	     clist->hadjustment->value ==
	     LIST_WIDTH (clist) - clist->clist_window_width)))
	{
	  if (x < 0)
	    move_horizontal (clist, -1 + (x/2));
	  else
	    move_horizontal (clist, 1 + (x - clist->clist_window_width) / 2);
	}
    }

  if (BTK_CLIST_IN_DRAG(clist))
    return FALSE;

  /* vertical autoscrolling */
  row = ROW_FROM_YPIXEL (clist, y);

  /* don't scroll on last pixel row if it's a cell spacing */
  if (y == clist->clist_window_height - 1 &&
      y == ROW_TOP_YPIXEL (clist, row-1) + clist->row_height)
    return FALSE;

  if (LIST_HEIGHT (clist) > clist->clist_window_height &&
      (y < 0 || y >= clist->clist_window_height))
    {
      if (clist->vtimer)
	return FALSE;

      clist->vtimer = bdk_threads_add_timeout (SCROLL_TIME,
				     (GSourceFunc) vertical_timeout, clist);

      if (clist->drag_button &&
	  ((y < 0 && clist->focus_row == 0) ||
	   (y >= clist->clist_window_height &&
	    clist->focus_row == clist->rows - 1)))
	return FALSE;
    }

  row = CLAMP (row, 0, clist->rows - 1);

  if (button_actions & BTK_BUTTON_SELECTS &
      !btk_object_get_data (BTK_OBJECT (widget), "btk-site-data"))
    {
      if (row == clist->focus_row)
	return FALSE;

      btk_clist_draw_focus (widget);
      clist->focus_row = row;
      btk_clist_draw_focus (widget);

      switch (clist->selection_mode)
	{
	case BTK_SELECTION_BROWSE:
	  btk_signal_emit (BTK_OBJECT (clist), clist_signals[SELECT_ROW],
			   clist->focus_row, -1, event);
	  break;
	case BTK_SELECTION_MULTIPLE:
	  update_extended_selection (clist, clist->focus_row);
	  break;
	default:
	  break;
	}
    }
  
  if (ROW_TOP_YPIXEL(clist, row) < 0)
    move_vertical (clist, row, 0);
  else if (ROW_TOP_YPIXEL(clist, row) + clist->row_height >
	   clist->clist_window_height)
    move_vertical (clist, row, 1);

  return FALSE;
}

static void
btk_clist_size_request (BtkWidget      *widget,
			BtkRequisition *requisition)
{
  BtkCList *clist;
  gint i;

  g_return_if_fail (BTK_IS_CLIST (widget));
  g_return_if_fail (requisition != NULL);

  clist = BTK_CLIST (widget);

  requisition->width = 0;
  requisition->height = 0;

  /* compute the size of the column title (title) area */
  clist->column_title_area.height = 0;
  if (BTK_CLIST_SHOW_TITLES(clist))
    for (i = 0; i < clist->columns; i++)
      if (clist->column[i].button)
	{
	  BtkRequisition child_requisition;
	  
	  btk_widget_size_request (clist->column[i].button,
				   &child_requisition);
	  clist->column_title_area.height =
	    MAX (clist->column_title_area.height,
		 child_requisition.height);
	}
  
  requisition->width += (widget->style->xthickness +
			 BTK_CONTAINER (widget)->border_width) * 2;
  requisition->height += (clist->column_title_area.height +
			  (widget->style->ythickness +
			   BTK_CONTAINER (widget)->border_width) * 2);

  /* if (!clist->hadjustment) */
  requisition->width += list_requisition_width (clist);
  /* if (!clist->vadjustment) */
  requisition->height += LIST_HEIGHT (clist);
}

static void
btk_clist_size_allocate (BtkWidget     *widget,
			 BtkAllocation *allocation)
{
  BtkCList *clist;
  BtkAllocation clist_allocation;
  gint border_width;

  g_return_if_fail (BTK_IS_CLIST (widget));
  g_return_if_fail (allocation != NULL);

  clist = BTK_CLIST (widget);
  widget->allocation = *allocation;
  border_width = BTK_CONTAINER (widget)->border_width;

  if (btk_widget_get_realized (widget))
    {
      bdk_window_move_resize (widget->window,
			      allocation->x + border_width,
			      allocation->y + border_width,
			      allocation->width - border_width * 2,
			      allocation->height - border_width * 2);
    }

  /* use internal allocation structure for all the math
   * because it's easier than always subtracting the container
   * border width */
  clist->internal_allocation.x = 0;
  clist->internal_allocation.y = 0;
  clist->internal_allocation.width = MAX (1, (gint)allocation->width -
					  border_width * 2);
  clist->internal_allocation.height = MAX (1, (gint)allocation->height -
					   border_width * 2);
	
  /* allocate clist window assuming no scrollbars */
  clist_allocation.x = (clist->internal_allocation.x +
			widget->style->xthickness);
  clist_allocation.y = (clist->internal_allocation.y +
			widget->style->ythickness +
			clist->column_title_area.height);
  clist_allocation.width = MAX (1, (gint)clist->internal_allocation.width - 
				(2 * (gint)widget->style->xthickness));
  clist_allocation.height = MAX (1, (gint)clist->internal_allocation.height -
				 (2 * (gint)widget->style->ythickness) -
				 (gint)clist->column_title_area.height);
  
  clist->clist_window_width = clist_allocation.width;
  clist->clist_window_height = clist_allocation.height;
  
  if (btk_widget_get_realized (widget))
    {
      bdk_window_move_resize (clist->clist_window,
			      clist_allocation.x,
			      clist_allocation.y,
			      clist_allocation.width,
			      clist_allocation.height);
    }
  
  /* position the window which holds the column title buttons */
  clist->column_title_area.x = widget->style->xthickness;
  clist->column_title_area.y = widget->style->ythickness;
  clist->column_title_area.width = clist_allocation.width;
  
  if (btk_widget_get_realized (widget))
    {
      bdk_window_move_resize (clist->title_window,
			      clist->column_title_area.x,
			      clist->column_title_area.y,
			      clist->column_title_area.width,
			      clist->column_title_area.height);
    }
  
  /* column button allocation */
  size_allocate_columns (clist, FALSE);
  size_allocate_title_buttons (clist);

  adjust_adjustments (clist, TRUE);
}

/* BTKCONTAINER
 *   btk_clist_forall
 */
static void
btk_clist_forall (BtkContainer *container,
		  gboolean      include_internals,
		  BtkCallback   callback,
		  gpointer      callback_data)
{
  BtkCList *clist;
  guint i;

  g_return_if_fail (BTK_IS_CLIST (container));
  g_return_if_fail (callback != NULL);

  if (!include_internals)
    return;

  clist = BTK_CLIST (container);
      
  /* callback for the column buttons */
  for (i = 0; i < clist->columns; i++)
    if (clist->column[i].button)
      (*callback) (clist->column[i].button, callback_data);
}

/* PRIVATE DRAWING FUNCTIONS
 *   get_cell_style
 *   draw_cell_pixmap
 *   draw_row
 *   draw_rows
 *   draw_xor_line
 *   clist_refresh
 */
static void
get_cell_style (BtkCList     *clist,
		BtkCListRow  *clist_row,
		gint          state,
		gint          column,
		BtkStyle    **style,
		BdkGC       **fg_gc,
		BdkGC       **bg_gc)
{
  gint fg_state;

  if ((state == BTK_STATE_NORMAL) &&
      (BTK_WIDGET (clist)->state == BTK_STATE_INSENSITIVE))
    fg_state = BTK_STATE_INSENSITIVE;
  else
    fg_state = state;

  if (clist_row->cell[column].style)
    {
      if (style)
	*style = clist_row->cell[column].style;
      if (fg_gc)
	*fg_gc = clist_row->cell[column].style->fg_gc[fg_state];
      if (bg_gc) {
	if (state == BTK_STATE_SELECTED)
	  *bg_gc = clist_row->cell[column].style->bg_gc[state];
	else
	  *bg_gc = clist_row->cell[column].style->base_gc[state];
      }
    }
  else if (clist_row->style)
    {
      if (style)
	*style = clist_row->style;
      if (fg_gc)
	*fg_gc = clist_row->style->fg_gc[fg_state];
      if (bg_gc) {
	if (state == BTK_STATE_SELECTED)
	  *bg_gc = clist_row->style->bg_gc[state];
	else
	  *bg_gc = clist_row->style->base_gc[state];
      }
    }
  else
    {
      if (style)
	*style = BTK_WIDGET (clist)->style;
      if (fg_gc)
	*fg_gc = BTK_WIDGET (clist)->style->fg_gc[fg_state];
      if (bg_gc) {
	if (state == BTK_STATE_SELECTED)
	  *bg_gc = BTK_WIDGET (clist)->style->bg_gc[state];
	else
	  *bg_gc = BTK_WIDGET (clist)->style->base_gc[state];
      }

      if (state != BTK_STATE_SELECTED)
	{
	  if (fg_gc && clist_row->fg_set)
	    *fg_gc = clist->fg_gc;
	  if (bg_gc && clist_row->bg_set)
	    *bg_gc = clist->bg_gc;
	}
    }
}

static gint
draw_cell_pixmap (BdkWindow    *window,
		  BdkRectangle *clip_rectangle,
		  BdkGC        *fg_gc,
		  BdkPixmap    *pixmap,
		  BdkBitmap    *mask,
		  gint          x,
		  gint          y,
		  gint          width,
		  gint          height)
{
  gint xsrc = 0;
  gint ysrc = 0;

  if (mask)
    {
      bdk_gc_set_clip_mask (fg_gc, mask);
      bdk_gc_set_clip_origin (fg_gc, x, y);
    }

  if (x < clip_rectangle->x)
    {
      xsrc = clip_rectangle->x - x;
      width -= xsrc;
      x = clip_rectangle->x;
    }
  if (x + width > clip_rectangle->x + clip_rectangle->width)
    width = clip_rectangle->x + clip_rectangle->width - x;

  if (y < clip_rectangle->y)
    {
      ysrc = clip_rectangle->y - y;
      height -= ysrc;
      y = clip_rectangle->y;
    }
  if (y + height > clip_rectangle->y + clip_rectangle->height)
    height = clip_rectangle->y + clip_rectangle->height - y;

  bdk_draw_drawable (window, fg_gc, pixmap, xsrc, ysrc, x, y, width, height);
  bdk_gc_set_clip_origin (fg_gc, 0, 0);
  if (mask)
    bdk_gc_set_clip_mask (fg_gc, NULL);

  return x + MAX (width, 0);
}

static void
draw_row (BtkCList     *clist,
	  BdkRectangle *area,
	  gint          row,
	  BtkCListRow  *clist_row)
{
  BtkWidget *widget;
  BdkRectangle *rect;
  BdkRectangle row_rectangle;
  BdkRectangle cell_rectangle;
  BdkRectangle clip_rectangle;
  BdkRectangle intersect_rectangle;
  gint last_column;
  gint state;
  gint i;

  g_return_if_fail (clist != NULL);

  /* bail now if we arn't drawable yet */
  if (!BTK_WIDGET_DRAWABLE (clist) || row < 0 || row >= clist->rows)
    return;

  widget = BTK_WIDGET (clist);

  /* if the function is passed the pointer to the row instead of null,
   * it avoids this expensive lookup */
  if (!clist_row)
    clist_row = ROW_ELEMENT (clist, row)->data;

  /* rectangle of the entire row */
  row_rectangle.x = 0;
  row_rectangle.y = ROW_TOP_YPIXEL (clist, row);
  row_rectangle.width = clist->clist_window_width;
  row_rectangle.height = clist->row_height;

  /* rectangle of the cell spacing above the row */
  cell_rectangle.x = 0;
  cell_rectangle.y = row_rectangle.y - CELL_SPACING;
  cell_rectangle.width = row_rectangle.width;
  cell_rectangle.height = CELL_SPACING;

  /* rectangle used to clip drawing operations, its y and height
   * positions only need to be set once, so we set them once here. 
   * the x and width are set withing the drawing loop below once per
   * column */
  clip_rectangle.y = row_rectangle.y;
  clip_rectangle.height = row_rectangle.height;

  if (clist_row->state == BTK_STATE_NORMAL)
    {
      if (clist_row->fg_set)
	bdk_gc_set_foreground (clist->fg_gc, &clist_row->foreground);
      if (clist_row->bg_set)
	bdk_gc_set_foreground (clist->bg_gc, &clist_row->background);
    }

  state = clist_row->state;

  /* draw the cell borders and background */
  if (area)
    {
      rect = &intersect_rectangle;
      if (bdk_rectangle_intersect (area, &cell_rectangle,
				   &intersect_rectangle))
	bdk_draw_rectangle (clist->clist_window,
			    widget->style->base_gc[BTK_STATE_NORMAL],
			    TRUE,
			    intersect_rectangle.x,
			    intersect_rectangle.y,
			    intersect_rectangle.width,
			    intersect_rectangle.height);

      /* the last row has to clear its bottom cell spacing too */
      if (clist_row == clist->row_list_end->data)
	{
	  cell_rectangle.y += clist->row_height + CELL_SPACING;

	  if (bdk_rectangle_intersect (area, &cell_rectangle,
				       &intersect_rectangle))
	    bdk_draw_rectangle (clist->clist_window,
				widget->style->base_gc[BTK_STATE_NORMAL],
				TRUE,
				intersect_rectangle.x,
				intersect_rectangle.y,
				intersect_rectangle.width,
				intersect_rectangle.height);
	}

      if (!bdk_rectangle_intersect (area, &row_rectangle,&intersect_rectangle))
	return;

    }
  else
    {
      rect = &clip_rectangle;
      bdk_draw_rectangle (clist->clist_window,
			  widget->style->base_gc[BTK_STATE_NORMAL],
			  TRUE,
			  cell_rectangle.x,
			  cell_rectangle.y,
			  cell_rectangle.width,
			  cell_rectangle.height);

      /* the last row has to clear its bottom cell spacing too */
      if (clist_row == clist->row_list_end->data)
	{
	  cell_rectangle.y += clist->row_height + CELL_SPACING;

	  bdk_draw_rectangle (clist->clist_window,
			      widget->style->base_gc[BTK_STATE_NORMAL],
			      TRUE,
			      cell_rectangle.x,
			      cell_rectangle.y,
			      cell_rectangle.width,
			      cell_rectangle.height);     
	}	  
    }
  
  for (last_column = clist->columns - 1;
       last_column >= 0 && !clist->column[last_column].visible; last_column--)
    ;

  /* iterate and draw all the columns (row cells) and draw their contents */
  for (i = 0; i < clist->columns; i++)
    {
      BtkStyle *style;
      BdkGC *fg_gc;
      BdkGC *bg_gc;
      BangoLayout *layout;
      BangoRectangle logical_rect;

      gint width;
      gint height;
      gint pixmap_width;
      gint offset = 0;

      if (!clist->column[i].visible)
	continue;

      get_cell_style (clist, clist_row, state, i, &style, &fg_gc, &bg_gc);

      clip_rectangle.x = clist->column[i].area.x + clist->hoffset;
      clip_rectangle.width = clist->column[i].area.width;

      /* calculate clipping rebunnyion clipping rebunnyion */
      clip_rectangle.x -= COLUMN_INSET + CELL_SPACING;
      clip_rectangle.width += (2 * COLUMN_INSET + CELL_SPACING +
			       (i == last_column) * CELL_SPACING);
      
      if (area && !bdk_rectangle_intersect (area, &clip_rectangle,
					    &intersect_rectangle))
	continue;

      bdk_draw_rectangle (clist->clist_window, bg_gc, TRUE,
			  rect->x, rect->y, rect->width, rect->height);

      clip_rectangle.x += COLUMN_INSET + CELL_SPACING;
      clip_rectangle.width -= (2 * COLUMN_INSET + CELL_SPACING +
			       (i == last_column) * CELL_SPACING);


      /* calculate real width for column justification */
      
      layout = _btk_clist_create_cell_layout (clist, clist_row, i);
      if (layout)
	{
	  bango_layout_get_pixel_extents (layout, NULL, &logical_rect);
	  width = logical_rect.width;
	}
      else
	width = 0;

      pixmap_width = 0;
      offset = 0;
      switch (clist_row->cell[i].type)
	{
	case BTK_CELL_PIXMAP:
	  bdk_drawable_get_size (BTK_CELL_PIXMAP (clist_row->cell[i])->pixmap,
                                 &pixmap_width, &height);
	  width += pixmap_width;
	  break;
	case BTK_CELL_PIXTEXT:
	  bdk_drawable_get_size (BTK_CELL_PIXTEXT (clist_row->cell[i])->pixmap,
                                 &pixmap_width, &height);
	  width += pixmap_width + BTK_CELL_PIXTEXT (clist_row->cell[i])->spacing;
	  break;
	default:
	  break;
	}

      switch (clist->column[i].justification)
	{
	case BTK_JUSTIFY_LEFT:
	  offset = clip_rectangle.x + clist_row->cell[i].horizontal;
	  break;
	case BTK_JUSTIFY_RIGHT:
	  offset = (clip_rectangle.x + clist_row->cell[i].horizontal +
		    clip_rectangle.width - width);
	  break;
	case BTK_JUSTIFY_CENTER:
	case BTK_JUSTIFY_FILL:
	  offset = (clip_rectangle.x + clist_row->cell[i].horizontal +
		    (clip_rectangle.width / 2) - (width / 2));
	  break;
	};

      /* Draw Text and/or Pixmap */
      switch (clist_row->cell[i].type)
	{
	case BTK_CELL_PIXMAP:
	  draw_cell_pixmap (clist->clist_window, &clip_rectangle, fg_gc,
			    BTK_CELL_PIXMAP (clist_row->cell[i])->pixmap,
			    BTK_CELL_PIXMAP (clist_row->cell[i])->mask,
			    offset,
			    clip_rectangle.y + clist_row->cell[i].vertical +
			    (clip_rectangle.height - height) / 2,
			    pixmap_width, height);
	  break;
	case BTK_CELL_PIXTEXT:
	  offset =
	    draw_cell_pixmap (clist->clist_window, &clip_rectangle, fg_gc,
			      BTK_CELL_PIXTEXT (clist_row->cell[i])->pixmap,
			      BTK_CELL_PIXTEXT (clist_row->cell[i])->mask,
			      offset,
			      clip_rectangle.y + clist_row->cell[i].vertical+
			      (clip_rectangle.height - height) / 2,
			      pixmap_width, height);
	  offset += BTK_CELL_PIXTEXT (clist_row->cell[i])->spacing;

	  /* Fall through */
	case BTK_CELL_TEXT:
	  if (layout)
	    {
	      gint row_center_offset = (clist->row_height - logical_rect.height - 1) / 2;

	      bdk_gc_set_clip_rectangle (fg_gc, &clip_rectangle);
	      bdk_draw_layout (clist->clist_window, fg_gc,
			       offset,
			       row_rectangle.y + row_center_offset + clist_row->cell[i].vertical,
			       layout);
              g_object_unref (layout);
	      bdk_gc_set_clip_rectangle (fg_gc, NULL);
	    }
	  break;
	default:
	  break;
	}
    }

  /* draw focus rectangle */
  if (clist->focus_row == row &&
      BTK_WIDGET_CAN_FOCUS (widget) && btk_widget_has_focus (widget))
    {
      if (!area)
	bdk_draw_rectangle (clist->clist_window, clist->xor_gc, FALSE,
			    row_rectangle.x, row_rectangle.y,
			    row_rectangle.width - 1, row_rectangle.height - 1);
      else if (bdk_rectangle_intersect (area, &row_rectangle,
					&intersect_rectangle))
	{
	  bdk_gc_set_clip_rectangle (clist->xor_gc, &intersect_rectangle);
	  bdk_draw_rectangle (clist->clist_window, clist->xor_gc, FALSE,
			      row_rectangle.x, row_rectangle.y,
			      row_rectangle.width - 1,
			      row_rectangle.height - 1);
	  bdk_gc_set_clip_rectangle (clist->xor_gc, NULL);
	}
    }
}

static void
draw_rows (BtkCList     *clist,
	   BdkRectangle *area)
{
  GList *list;
  BtkCListRow *clist_row;
  gint i;
  gint first_row;
  gint last_row;

  g_return_if_fail (BTK_IS_CLIST (clist));

  if (clist->row_height == 0 ||
      !BTK_WIDGET_DRAWABLE (clist))
    return;

  if (area)
    {
      first_row = ROW_FROM_YPIXEL (clist, area->y);
      last_row = ROW_FROM_YPIXEL (clist, area->y + area->height);
    }
  else
    {
      first_row = ROW_FROM_YPIXEL (clist, 0);
      last_row = ROW_FROM_YPIXEL (clist, clist->clist_window_height);
    }

  /* this is a small special case which exposes the bottom cell line
   * on the last row -- it might go away if I change the wall the cell
   * spacings are drawn
   */
  if (clist->rows == first_row)
    first_row--;

  list = ROW_ELEMENT (clist, first_row);
  i = first_row;
  while (list)
    {
      clist_row = list->data;
      list = list->next;

      if (i > last_row)
	return;

      BTK_CLIST_GET_CLASS (clist)->draw_row (clist, area, i, clist_row);
      i++;
    }

  if (!area)
    {
      int w, h, y;
      bdk_drawable_get_size (BDK_DRAWABLE (clist->clist_window), &w, &h);
      y = ROW_TOP_YPIXEL (clist, i);
      bdk_window_clear_area (clist->clist_window,
			     0, y,
			     w, h - y);
    }
}

static void                          
draw_xor_line (BtkCList *clist)
{
  BtkWidget *widget;

  g_return_if_fail (clist != NULL);

  widget = BTK_WIDGET (clist);

  bdk_draw_line (widget->window, clist->xor_gc,
                 clist->x_drag,
		 widget->style->ythickness,
                 clist->x_drag,
                 clist->column_title_area.height +
		 clist->clist_window_height + 1);
}

static void
clist_refresh (BtkCList *clist)
{
  g_return_if_fail (BTK_IS_CLIST (clist));
  
  if (CLIST_UNFROZEN (clist))
    { 
      adjust_adjustments (clist, FALSE);
      draw_rows (clist, NULL);
    }
}

/* get cell from coordinates
 *   get_selection_info
 *   btk_clist_get_selection_info
 */
static gint
get_selection_info (BtkCList *clist,
		    gint      x,
		    gint      y,
		    gint     *row,
		    gint     *column)
{
  gint trow, tcol;

  g_return_val_if_fail (BTK_IS_CLIST (clist), 0);

  /* bounds checking, return false if the user clicked 
   * on a blank area */
  trow = ROW_FROM_YPIXEL (clist, y);
  if (trow >= clist->rows)
    return 0;

  if (row)
    *row = trow;

  tcol = COLUMN_FROM_XPIXEL (clist, x);
  if (tcol >= clist->columns)
    return 0;

  if (column)
    *column = tcol;

  return 1;
}

gint
btk_clist_get_selection_info (BtkCList *clist, 
			      gint      x, 
			      gint      y, 
			      gint     *row, 
			      gint     *column)
{
  g_return_val_if_fail (BTK_IS_CLIST (clist), 0);
  return get_selection_info (clist, x, y, row, column);
}

/* PRIVATE ADJUSTMENT FUNCTIONS
 *   adjust_adjustments
 *   vadjustment_changed
 *   hadjustment_changed
 *   vadjustment_value_changed
 *   hadjustment_value_changed 
 *   check_exposures
 */
static void
adjust_adjustments (BtkCList *clist,
		    gboolean  block_resize)
{
  if (clist->vadjustment)
    {
      clist->vadjustment->page_size = clist->clist_window_height;
      clist->vadjustment->step_increment = clist->row_height;
      clist->vadjustment->page_increment =
	MAX (clist->vadjustment->page_size - clist->vadjustment->step_increment,
	     clist->vadjustment->page_size / 2);
      clist->vadjustment->lower = 0;
      clist->vadjustment->upper = LIST_HEIGHT (clist);

      if (clist->clist_window_height - clist->voffset > LIST_HEIGHT (clist) ||
	  (clist->voffset + (gint)clist->vadjustment->value) != 0)
	{
	  clist->vadjustment->value = MAX (0, (LIST_HEIGHT (clist) -
					       clist->clist_window_height));
	  btk_signal_emit_by_name (BTK_OBJECT (clist->vadjustment),
				   "value-changed");
	}
      btk_signal_emit_by_name (BTK_OBJECT (clist->vadjustment), "changed");
    }

  if (clist->hadjustment)
    {
      clist->hadjustment->page_size = clist->clist_window_width;
      clist->hadjustment->step_increment = 10;
      clist->hadjustment->page_increment =
	MAX (clist->hadjustment->page_size - clist->hadjustment->step_increment,
	     clist->hadjustment->page_size / 2);
      clist->hadjustment->lower = 0;
      clist->hadjustment->upper = LIST_WIDTH (clist);

      if (clist->clist_window_width - clist->hoffset > LIST_WIDTH (clist) ||
	  (clist->hoffset + (gint)clist->hadjustment->value) != 0)
	{
	  clist->hadjustment->value = MAX (0, (LIST_WIDTH (clist) -
					       clist->clist_window_width));
	  btk_signal_emit_by_name (BTK_OBJECT (clist->hadjustment),
				   "value-changed");
	}
      btk_signal_emit_by_name (BTK_OBJECT (clist->hadjustment), "changed");
    }

  if (!block_resize && (!clist->vadjustment || !clist->hadjustment))
    {
      BtkWidget *widget;
      BtkRequisition requisition;

      widget = BTK_WIDGET (clist);
      btk_widget_size_request (widget, &requisition);

      if ((!clist->hadjustment &&
	   requisition.width != widget->allocation.width) ||
	  (!clist->vadjustment &&
	   requisition.height != widget->allocation.height))
	btk_widget_queue_resize (widget);
    }
}

static void
vadjustment_changed (BtkAdjustment *adjustment,
		     gpointer       data)
{
  BtkCList *clist;

  g_return_if_fail (adjustment != NULL);
  g_return_if_fail (data != NULL);

  clist = BTK_CLIST (data);
}

static void
hadjustment_changed (BtkAdjustment *adjustment,
		     gpointer       data)
{
  BtkCList *clist;

  g_return_if_fail (adjustment != NULL);
  g_return_if_fail (data != NULL);

  clist = BTK_CLIST (data);
}

static void
vadjustment_value_changed (BtkAdjustment *adjustment,
			   gpointer       data)
{
  BtkCList *clist;
  gint dy, value;

  g_return_if_fail (adjustment != NULL);
  g_return_if_fail (BTK_IS_CLIST (data));

  clist = BTK_CLIST (data);

  if (adjustment != clist->vadjustment)
    return;

  value = -adjustment->value;
  dy = value - clist->voffset;
  clist->voffset = value;

  if (BTK_WIDGET_DRAWABLE (clist))
    {
      bdk_window_scroll (clist->clist_window, 0, dy);
      bdk_window_process_updates (clist->clist_window, FALSE);
    }
  
  return;
}

typedef struct
{
  BdkWindow *window;
  gint dx;
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
  
  if (!btk_widget_get_realized (widget))
    {
      if (btk_widget_get_visible (widget))
	{
	  BdkRectangle tmp_rectangle = widget->allocation;
	  tmp_rectangle.x += scroll_data->dx;
      
	  btk_widget_size_allocate (widget, &tmp_rectangle);
	}
    }
  else
    {
      if (ALLOCATION_WINDOW (widget) == scroll_data->window)
	{
	  widget->allocation.x += scroll_data->dx;

	  if (BTK_IS_CONTAINER (widget))
	    btk_container_forall (BTK_CONTAINER (widget),
				  adjust_allocation_recurse,
				  data);
	}
    }
}

static void
adjust_allocation (BtkWidget *widget,
		   gint       dx)
{
  ScrollData scroll_data;

  if (btk_widget_get_realized (widget))
    scroll_data.window = ALLOCATION_WINDOW (widget);
  else
    scroll_data.window = NULL;
    
  scroll_data.dx = dx;
  
  adjust_allocation_recurse (widget, &scroll_data);
}

static void
hadjustment_value_changed (BtkAdjustment *adjustment,
			   gpointer       data)
{
  BtkCList *clist;
  BtkContainer *container;
  BdkRectangle area;
  gint i;
  gint y = 0;
  gint value;
  gint dx;

  g_return_if_fail (adjustment != NULL);
  g_return_if_fail (BTK_IS_CLIST (data));

  clist = BTK_CLIST (data);
  container = BTK_CONTAINER (data);

  if (adjustment != clist->hadjustment)
    return;

  value = adjustment->value;

  dx = -value - clist->hoffset;

  if (btk_widget_get_realized (BTK_WIDGET (clist)))
    bdk_window_scroll (clist->title_window, dx, 0);

  /* adjust the column button's allocations */
  for (i = 0; i < clist->columns; i++)
    if (clist->column[i].button)
      adjust_allocation (clist->column[i].button, dx);

  clist->hoffset = -value;

  if (BTK_WIDGET_DRAWABLE (clist))
    {
      if (BTK_WIDGET_CAN_FOCUS(clist) && btk_widget_has_focus (BTK_WIDGET (clist)) &&
          !container->focus_child && BTK_CLIST_ADD_MODE(clist))
        {
          y = ROW_TOP_YPIXEL (clist, clist->focus_row);
      
          bdk_draw_rectangle (clist->clist_window, clist->xor_gc, FALSE, 0, y,
                              clist->clist_window_width - 1,
                              clist->row_height - 1);
        }
 
      bdk_window_scroll (clist->clist_window, dx, 0);
      bdk_window_process_updates (clist->clist_window, FALSE);

      if (BTK_WIDGET_CAN_FOCUS(clist) && btk_widget_has_focus (BTK_WIDGET (clist)) &&
          !container->focus_child)
        {
          if (BTK_CLIST_ADD_MODE(clist))
            {
              gint focus_row;
	  
              focus_row = clist->focus_row;
              clist->focus_row = -1;
              draw_rows (clist, &area);
              clist->focus_row = focus_row;
	  
              bdk_draw_rectangle (clist->clist_window, clist->xor_gc,
                                  FALSE, 0, y, clist->clist_window_width - 1,
                                  clist->row_height - 1);
              return;
            }
          else if (ABS(dx) < clist->clist_window_width - 1)
            {
              gint x0;
              gint x1;
	  
              if (dx > 0)
                {
                  x0 = clist->clist_window_width - 1;
                  x1 = dx;
                }
              else
                {
                  x0 = 0;
                  x1 = clist->clist_window_width - 1 + dx;
                }

              y = ROW_TOP_YPIXEL (clist, clist->focus_row);
              bdk_draw_line (clist->clist_window, clist->xor_gc,
                             x0, y + 1, x0, y + clist->row_height - 2);
              bdk_draw_line (clist->clist_window, clist->xor_gc,
                             x1, y + 1, x1, y + clist->row_height - 2);
            }
        }
    }
}

/* PRIVATE 
 * Memory Allocation/Distruction Routines for BtkCList stuctures
 *
 * functions:
 *   columns_new
 *   column_title_new
 *   columns_delete
 *   row_new
 *   row_delete
 */
static BtkCListColumn *
columns_new (BtkCList *clist)
{
  BtkCListColumn *column;
  gint i;

  column = g_new (BtkCListColumn, clist->columns);

  for (i = 0; i < clist->columns; i++)
    {
      column[i].area.x = 0;
      column[i].area.y = 0;
      column[i].area.width = 0;
      column[i].area.height = 0;
      column[i].title = NULL;
      column[i].button = NULL;
      column[i].window = NULL;
      column[i].width = 0;
      column[i].min_width = -1;
      column[i].max_width = -1;
      column[i].visible = TRUE;
      column[i].width_set = FALSE;
      column[i].resizeable = TRUE;
      column[i].auto_resize = FALSE;
      column[i].button_passive = FALSE;
      column[i].justification = BTK_JUSTIFY_LEFT;
    }

  return column;
}

static void
column_title_new (BtkCList    *clist,
		  gint         column,
		  const gchar *title)
{
  g_free (clist->column[column].title);

  clist->column[column].title = g_strdup (title);
}

static void
columns_delete (BtkCList *clist)
{
  gint i;

  for (i = 0; i < clist->columns; i++)
    g_free (clist->column[i].title);
      
  g_free (clist->column);
}

static BtkCListRow *
row_new (BtkCList *clist)
{
  int i;
  BtkCListRow *clist_row;

  clist_row = g_slice_new (BtkCListRow);
  clist_row->cell = g_slice_alloc (sizeof (BtkCell) * clist->columns);

  for (i = 0; i < clist->columns; i++)
    {
      clist_row->cell[i].type = BTK_CELL_EMPTY;
      clist_row->cell[i].vertical = 0;
      clist_row->cell[i].horizontal = 0;
      clist_row->cell[i].style = NULL;
    }

  clist_row->fg_set = FALSE;
  clist_row->bg_set = FALSE;
  clist_row->style = NULL;
  clist_row->selectable = TRUE;
  clist_row->state = BTK_STATE_NORMAL;
  clist_row->data = NULL;
  clist_row->destroy = NULL;

  return clist_row;
}

static void
row_delete (BtkCList    *clist,
	    BtkCListRow *clist_row)
{
  gint i;

  for (i = 0; i < clist->columns; i++)
    {
      BTK_CLIST_GET_CLASS (clist)->set_cell_contents
	(clist, clist_row, i, BTK_CELL_EMPTY, NULL, 0, NULL, NULL);
      if (clist_row->cell[i].style)
	{
	  if (btk_widget_get_realized (BTK_WIDGET (clist)))
	    btk_style_detach (clist_row->cell[i].style);
	  g_object_unref (clist_row->cell[i].style);
	}
    }

  if (clist_row->style)
    {
      if (btk_widget_get_realized (BTK_WIDGET (clist)))
        btk_style_detach (clist_row->style);
      g_object_unref (clist_row->style);
    }

  if (clist_row->destroy)
    clist_row->destroy (clist_row->data);

  g_slice_free1 (sizeof (BtkCell) * clist->columns, clist_row->cell);
  g_slice_free (BtkCListRow, clist_row);
}

/* FOCUS FUNCTIONS
 *   btk_clist_focus_content_area
 *   btk_clist_focus
 *   btk_clist_draw_focus
 *   btk_clist_focus_in
 *   btk_clist_focus_out
 *   title_focus
 */
static void
btk_clist_focus_content_area (BtkCList *clist)
{
  if (clist->focus_row < 0)
    {
      clist->focus_row = 0;
      
      if ((clist->selection_mode == BTK_SELECTION_BROWSE ||
	   clist->selection_mode == BTK_SELECTION_MULTIPLE) &&
	  !clist->selection)
	btk_signal_emit (BTK_OBJECT (clist),
			 clist_signals[SELECT_ROW],
			 clist->focus_row, -1, NULL);
    }
  btk_widget_grab_focus (BTK_WIDGET (clist));
}

static gboolean
btk_clist_focus (BtkWidget        *widget,
		 BtkDirectionType  direction)
{
  BtkCList *clist = BTK_CLIST (widget);
  BtkWidget *focus_child;
  gboolean is_current_focus;

  if (!btk_widget_is_sensitive (widget))
    return FALSE;

  focus_child = BTK_CONTAINER (widget)->focus_child;
  
  is_current_focus = btk_widget_is_focus (BTK_WIDGET (clist));
			  
  if (focus_child &&
      btk_widget_child_focus (focus_child, direction))
    return TRUE;
      
  switch (direction)
    {
    case BTK_DIR_LEFT:
    case BTK_DIR_RIGHT:
      if (focus_child)
	{
	  if (title_focus_move (clist, direction))
	    return TRUE;
	}
      else if (!is_current_focus)
	{
	  btk_clist_focus_content_area (clist);
	  return TRUE;
	}
      break;
    case BTK_DIR_DOWN:
    case BTK_DIR_TAB_FORWARD:
      if (!focus_child && !is_current_focus)
	{
	  if (title_focus_in (clist, direction))
	    return TRUE;
	}
      
      if (!is_current_focus && clist->rows)
	{
	  btk_clist_focus_content_area (clist);
	  return TRUE;
	}
      break;
    case BTK_DIR_UP:
    case BTK_DIR_TAB_BACKWARD:
      if (!focus_child && is_current_focus)
	{
	  if (title_focus_in (clist, direction))
	    return TRUE;
	}
      
      if (!is_current_focus && !focus_child && clist->rows)
	{
	  btk_clist_focus_content_area (clist);
	  return TRUE;
	}
      break;
    default:
      break;
    }

  return FALSE;
}

static void
btk_clist_set_focus_child (BtkContainer *container,
			   BtkWidget    *child)
{
  BtkCList *clist = BTK_CLIST (container);
  gint i;

  for (i = 0; i < clist->columns; i++)
    if (clist->column[i].button == child)
      clist->focus_header_column = i;
  
  parent_class->set_focus_child (container, child);
}

static void
btk_clist_draw_focus (BtkWidget *widget)
{
  BtkCList *clist;

  g_return_if_fail (BTK_IS_CLIST (widget));

  if (!BTK_WIDGET_DRAWABLE (widget) || !BTK_WIDGET_CAN_FOCUS (widget))
    return;

  clist = BTK_CLIST (widget);
  if (clist->focus_row >= 0)
    bdk_draw_rectangle (clist->clist_window, clist->xor_gc, FALSE,
			0, ROW_TOP_YPIXEL(clist, clist->focus_row),
			clist->clist_window_width - 1,
			clist->row_height - 1);
}

static gint
btk_clist_focus_in (BtkWidget     *widget,
		    BdkEventFocus *event)
{
  BtkCList *clist = BTK_CLIST (widget);

  if (clist->selection_mode == BTK_SELECTION_BROWSE &&
      clist->selection == NULL && clist->focus_row > -1)
    {
      GList *list;

      list = g_list_nth (clist->row_list, clist->focus_row);
      if (list && BTK_CLIST_ROW (list)->selectable)
	btk_signal_emit (BTK_OBJECT (clist), clist_signals[SELECT_ROW],
			 clist->focus_row, -1, event);
      else
	btk_clist_draw_focus (widget);
    }
  else
    btk_clist_draw_focus (widget);

  return FALSE;
}

static gint
btk_clist_focus_out (BtkWidget     *widget,
		     BdkEventFocus *event)
{
  BtkCList *clist = BTK_CLIST (widget);

  btk_clist_draw_focus (widget);
  
  BTK_CLIST_GET_CLASS (widget)->resync_selection (clist, (BdkEvent *) event);

  return FALSE;
}

static gboolean
focus_column (BtkCList *clist, gint column, gint dir)
{
  BtkWidget *child = clist->column[column].button;
  
  if (btk_widget_child_focus (child, dir))
    {
      return TRUE;
    }
  else if (BTK_WIDGET_CAN_FOCUS (child))
    {
      btk_widget_grab_focus (child);
      return TRUE;
    }

  return FALSE;
}

/* Focus moved onto the headers. Focus first focusable and visible child.
 * (FIXME: focus the last focused child if visible)
 */
static gboolean
title_focus_in (BtkCList *clist, gint dir)
{
  gint i;
  gint left, right;

  if (!BTK_CLIST_SHOW_TITLES (clist))
    return FALSE;

  /* Check last focused column */
  if (clist->focus_header_column != -1)
    {
      i = clist->focus_header_column;
      
      left = COLUMN_LEFT_XPIXEL (clist, i);
      right = left + clist->column[i].area.width;
      
      if (left >= 0 && right <= clist->clist_window_width)
	{
	  if (focus_column (clist, i, dir))
	    return TRUE;
	}
    }

  /* Check fully visible columns */
  for (i = 0 ; i < clist->columns ; i++)
    {
      left = COLUMN_LEFT_XPIXEL (clist, i);
      right = left + clist->column[i].area.width;
      
      if (left >= 0 && right <= clist->clist_window_width)
	{
	  if (focus_column (clist, i, dir))
	    return TRUE;
	}
    }

  /* Check partially visible columns */
  for (i = 0 ; i < clist->columns ; i++)
    {
      left = COLUMN_LEFT_XPIXEL (clist, i);
      right = left + clist->column[i].area.width;

      if ((left < 0 && right > 0) ||
	  (left < clist->clist_window_width && right > clist->clist_window_width))
	{
	  if (focus_column (clist, i, dir))
	    return TRUE;
	}
    }
      
  return FALSE;
}

/* Move the focus right or left within the title buttons, scrolling
 * as necessary to keep the focused child visible.
 */
static gboolean
title_focus_move (BtkCList *clist,
		  gint      dir)
{
  BtkWidget *focus_child;
  gboolean return_val = FALSE;
  gint d = 0;
  gint i = -1;
  gint j;

  if (!BTK_CLIST_SHOW_TITLES(clist))
    return FALSE;

  focus_child = BTK_CONTAINER (clist)->focus_child;
  g_assert (focus_child);

  /* Movement direction within headers
   */
  switch (dir)
    {
    case BTK_DIR_RIGHT:
      d = 1;
      break;
    case BTK_DIR_LEFT:
      d = -1;
      break;
    }
  
  for (i = 0; i < clist->columns; i++)
    if (clist->column[i].button == focus_child)
      break;
  
  g_assert (i != -1);		/* Have a starting column */
  
  j = i + d;
  while (!return_val && j >= 0 && j < clist->columns)
    {
      if (clist->column[j].button &&
	  btk_widget_get_visible (clist->column[j].button))
	{
	  if (focus_column (clist, j, dir))
	    {
	      return_val = TRUE;
	      break;
	    }
	}
      j += d;
    }

  /* If we didn't find it, wrap around and keep looking
   */
  if (!return_val)
    {
      j = d > 0 ? 0 : clist->columns - 1;

      while (!return_val && j != i)
	{
	  if (clist->column[j].button &&
	      btk_widget_get_visible (clist->column[j].button))
	    {
	      if (focus_column (clist, j, dir))
		{
		  return_val = TRUE;
		  break;
		}
	    }
	  j += d;
	}
    }

  /* Scroll horizontally so focused column is visible
   */
  if (return_val)
    {
      if (COLUMN_LEFT_XPIXEL (clist, j) < CELL_SPACING + COLUMN_INSET)
	btk_clist_moveto (clist, -1, j, 0, 0);
      else if (COLUMN_LEFT_XPIXEL(clist, j) + clist->column[j].area.width >
	       clist->clist_window_width)
	{
	  gint last_column;
	  
	  for (last_column = clist->columns - 1;
	       last_column >= 0 && !clist->column[last_column].visible; last_column--);

	  if (j == last_column)
	    btk_clist_moveto (clist, -1, j, 0, 0);
	  else
	    btk_clist_moveto (clist, -1, j, 0, 1);
	}
    }
  return TRUE;			/* Even if we didn't find a new one, we can keep the
				 * focus in the same place.
				 */
}

/* PRIVATE SCROLLING FUNCTIONS
 *   move_focus_row
 *   scroll_horizontal
 *   scroll_vertical
 *   move_horizontal
 *   move_vertical
 *   horizontal_timeout
 *   vertical_timeout
 *   remove_grab
 */
static void
move_focus_row (BtkCList      *clist,
		BtkScrollType  scroll_type,
		gfloat         position)
{
  BtkWidget *widget;

  g_return_if_fail (clist != 0);
  g_return_if_fail (BTK_IS_CLIST (clist));

  widget = BTK_WIDGET (clist);

  switch (scroll_type)
    {
    case BTK_SCROLL_STEP_UP:
    case BTK_SCROLL_STEP_BACKWARD:
      if (clist->focus_row <= 0)
	return;
      btk_clist_draw_focus (widget);
      clist->focus_row--;
      btk_clist_draw_focus (widget);
      break;

    case BTK_SCROLL_STEP_DOWN:
    case BTK_SCROLL_STEP_FORWARD:
      if (clist->focus_row >= clist->rows - 1)
	return;
      btk_clist_draw_focus (widget);
      clist->focus_row++;
      btk_clist_draw_focus (widget);
      break;
    case BTK_SCROLL_PAGE_UP:
    case BTK_SCROLL_PAGE_BACKWARD:
      if (clist->focus_row <= 0)
	return;
      btk_clist_draw_focus (widget);
      clist->focus_row = MAX (0, clist->focus_row -
			      (2 * clist->clist_window_height -
			       clist->row_height - CELL_SPACING) / 
			      (2 * (clist->row_height + CELL_SPACING)));
      btk_clist_draw_focus (widget);
      break;
    case BTK_SCROLL_PAGE_DOWN:
    case BTK_SCROLL_PAGE_FORWARD:
      if (clist->focus_row >= clist->rows - 1)
	return;
      btk_clist_draw_focus (widget);
      clist->focus_row = MIN (clist->rows - 1, clist->focus_row + 
			      (2 * clist->clist_window_height -
			       clist->row_height - CELL_SPACING) / 
			      (2 * (clist->row_height + CELL_SPACING)));
      btk_clist_draw_focus (widget);
      break;
    case BTK_SCROLL_JUMP:
      if (position >= 0 && position <= 1)
	{
	  btk_clist_draw_focus (widget);
	  clist->focus_row = position * (clist->rows - 1);
	  btk_clist_draw_focus (widget);
	}
      break;
    default:
      break;
    }
}

static void
scroll_horizontal (BtkCList      *clist,
		   BtkScrollType  scroll_type,
		   gfloat         position)
{
  gint column = 0;
  gint last_column;

  g_return_if_fail (clist != 0);
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (clist_has_grab (clist))
    return;

  for (last_column = clist->columns - 1;
       last_column >= 0 && !clist->column[last_column].visible; last_column--)
    ;

  switch (scroll_type)
    {
    case BTK_SCROLL_STEP_BACKWARD:
      column = COLUMN_FROM_XPIXEL (clist, 0);
      if (COLUMN_LEFT_XPIXEL (clist, column) - CELL_SPACING - COLUMN_INSET >= 0
	  && column > 0)
	column--;
      break;
    case BTK_SCROLL_STEP_FORWARD:
      column = COLUMN_FROM_XPIXEL (clist, clist->clist_window_width);
      if (column < 0)
	return;
      if (COLUMN_LEFT_XPIXEL (clist, column) +
	  clist->column[column].area.width +
	  CELL_SPACING + COLUMN_INSET - 1 <= clist->clist_window_width &&
	  column < last_column)
	column++;
      break;
    case BTK_SCROLL_PAGE_BACKWARD:
    case BTK_SCROLL_PAGE_FORWARD:
      return;
    case BTK_SCROLL_JUMP:
      if (position >= 0 && position <= 1)
	{
	  gint vis_columns = 0;
	  gint i;

	  for (i = 0; i <= last_column; i++)
 	    if (clist->column[i].visible)
	      vis_columns++;

	  column = position * vis_columns;

	  for (i = 0; i <= last_column && column > 0; i++)
	    if (clist->column[i].visible)
	      column--;

	  column = i;
	}
      else
	return;
      break;
    default:
      break;
    }

  if (COLUMN_LEFT_XPIXEL (clist, column) < CELL_SPACING + COLUMN_INSET)
    btk_clist_moveto (clist, -1, column, 0, 0);
  else if (COLUMN_LEFT_XPIXEL (clist, column) + CELL_SPACING + COLUMN_INSET - 1
	   + clist->column[column].area.width > clist->clist_window_width)
    {
      if (column == last_column)
	btk_clist_moveto (clist, -1, column, 0, 0);
      else
	btk_clist_moveto (clist, -1, column, 0, 1);
    }
}

static void
scroll_vertical (BtkCList      *clist,
		 BtkScrollType  scroll_type,
		 gfloat         position)
{
  gint old_focus_row;

  g_return_if_fail (BTK_IS_CLIST (clist));

  if (clist_has_grab (clist))
    return;

  switch (clist->selection_mode)
    {
    case BTK_SELECTION_MULTIPLE:
      if (clist->anchor >= 0)
	return;
    case BTK_SELECTION_BROWSE:

      old_focus_row = clist->focus_row;
      move_focus_row (clist, scroll_type, position);

      if (old_focus_row != clist->focus_row)
	{
	  if (clist->selection_mode == BTK_SELECTION_BROWSE)
	    btk_signal_emit (BTK_OBJECT (clist), clist_signals[UNSELECT_ROW],
			     old_focus_row, -1, NULL);
	  else if (!BTK_CLIST_ADD_MODE(clist))
	    {
	      btk_clist_unselect_all (clist);
	      clist->undo_anchor = old_focus_row;
	    }
	}

      switch (btk_clist_row_is_visible (clist, clist->focus_row))
	{
	case BTK_VISIBILITY_NONE:
	  if (old_focus_row != clist->focus_row &&
	      !(clist->selection_mode == BTK_SELECTION_MULTIPLE &&
		BTK_CLIST_ADD_MODE(clist)))
	    btk_signal_emit (BTK_OBJECT (clist), clist_signals[SELECT_ROW],
			     clist->focus_row, -1, NULL);
	  switch (scroll_type)
	    {
            case BTK_SCROLL_PAGE_UP:
            case BTK_SCROLL_STEP_UP:
	    case BTK_SCROLL_STEP_BACKWARD:
	    case BTK_SCROLL_PAGE_BACKWARD:
	      btk_clist_moveto (clist, clist->focus_row, -1, 0, 0);
	      break;
            case BTK_SCROLL_PAGE_DOWN:
            case BTK_SCROLL_STEP_DOWN:
	    case BTK_SCROLL_STEP_FORWARD:
	    case BTK_SCROLL_PAGE_FORWARD:
	      btk_clist_moveto (clist, clist->focus_row, -1, 1, 0);
	      break;
	    case BTK_SCROLL_JUMP:
	      btk_clist_moveto (clist, clist->focus_row, -1, 0.5, 0);
	      break;
	    default:
	      break;
	    }
	  break;
	case BTK_VISIBILITY_PARTIAL:
	  switch (scroll_type)
	    {
	    case BTK_SCROLL_STEP_BACKWARD:
	    case BTK_SCROLL_PAGE_BACKWARD:
	      btk_clist_moveto (clist, clist->focus_row, -1, 0, 0);
	      break;
	    case BTK_SCROLL_STEP_FORWARD:
	    case BTK_SCROLL_PAGE_FORWARD:
	      btk_clist_moveto (clist, clist->focus_row, -1, 1, 0);
	      break;
	    case BTK_SCROLL_JUMP:
	      btk_clist_moveto (clist, clist->focus_row, -1, 0.5, 0);
	      break;
	    default:
	      break;
	    }
	default:
	  if (old_focus_row != clist->focus_row &&
	      !(clist->selection_mode == BTK_SELECTION_MULTIPLE &&
		BTK_CLIST_ADD_MODE(clist)))
	    btk_signal_emit (BTK_OBJECT (clist), clist_signals[SELECT_ROW],
			     clist->focus_row, -1, NULL);
	  break;
	}
      break;
    default:
      move_focus_row (clist, scroll_type, position);

      if (ROW_TOP_YPIXEL (clist, clist->focus_row) + clist->row_height >
	  clist->clist_window_height)
	btk_clist_moveto (clist, clist->focus_row, -1, 1, 0);
      else if (ROW_TOP_YPIXEL (clist, clist->focus_row) < 0)
	btk_clist_moveto (clist, clist->focus_row, -1, 0, 0);
      break;
    }
}

static void
move_horizontal (BtkCList *clist,
		 gint      diff)
{
  gdouble value;

  if (!clist->hadjustment)
    return;

  value = CLAMP (clist->hadjustment->value + diff, 0.0,
		 clist->hadjustment->upper - clist->hadjustment->page_size);
  btk_adjustment_set_value (clist->hadjustment, value);
}

static void
move_vertical (BtkCList *clist,
	       gint      row,
	       gfloat    align)
{
  gdouble value;

  if (!clist->vadjustment)
    return;

  value = (ROW_TOP_YPIXEL (clist, row) - clist->voffset -
	   align * (clist->clist_window_height - clist->row_height) +
	   (2 * align - 1) * CELL_SPACING);

  if (value + clist->vadjustment->page_size > clist->vadjustment->upper)
    value = clist->vadjustment->upper - clist->vadjustment->page_size;

  btk_adjustment_set_value (clist->vadjustment, value);
}

static void
do_fake_motion (BtkWidget *widget)
{
  BdkEvent *event = bdk_event_new (BDK_MOTION_NOTIFY);

  event->motion.send_event = TRUE;

  btk_clist_motion (widget, (BdkEventMotion *)event);
  bdk_event_free (event);
}

static gint
horizontal_timeout (BtkCList *clist)
{
  clist->htimer = 0;
  do_fake_motion (BTK_WIDGET (clist));

  return FALSE;
}

static gint
vertical_timeout (BtkCList *clist)
{
  clist->vtimer = 0;
  do_fake_motion (BTK_WIDGET (clist));

  return FALSE;
}

static void
remove_grab (BtkCList *clist)
{
  BtkWidget *widget = BTK_WIDGET (clist);
  
  if (BTK_WIDGET_HAS_GRAB (clist))
    {
      BdkDisplay *display = btk_widget_get_display (widget);
      
      btk_grab_remove (widget);
      if (bdk_display_pointer_is_grabbed (display))
	bdk_display_pointer_ungrab (display, BDK_CURRENT_TIME);
    }

  if (clist->htimer)
    {
      g_source_remove (clist->htimer);
      clist->htimer = 0;
    }

  if (clist->vtimer)
    {
      g_source_remove (clist->vtimer);
      clist->vtimer = 0;
    }
}

/* PUBLIC SORTING FUNCTIONS
 * btk_clist_sort
 * btk_clist_set_compare_func
 * btk_clist_set_auto_sort
 * btk_clist_set_sort_type
 * btk_clist_set_sort_column
 */
void
btk_clist_sort (BtkCList *clist)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  BTK_CLIST_GET_CLASS (clist)->sort_list (clist);
}

void
btk_clist_set_compare_func (BtkCList            *clist,
			    BtkCListCompareFunc  cmp_func)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  clist->compare = (cmp_func) ? cmp_func : default_compare;
}

void       
btk_clist_set_auto_sort (BtkCList *clist,
			 gboolean  auto_sort)
{
  g_return_if_fail (BTK_IS_CLIST (clist));
  
  if (BTK_CLIST_AUTO_SORT(clist) && !auto_sort)
    BTK_CLIST_UNSET_FLAG (clist, CLIST_AUTO_SORT);
  else if (!BTK_CLIST_AUTO_SORT(clist) && auto_sort)
    {
      BTK_CLIST_SET_FLAG (clist, CLIST_AUTO_SORT);
      btk_clist_sort (clist);
    }
}

void       
btk_clist_set_sort_type (BtkCList    *clist,
			 BtkSortType  sort_type)
{
  g_return_if_fail (BTK_IS_CLIST (clist));
  
  clist->sort_type = sort_type;
}

void
btk_clist_set_sort_column (BtkCList *clist,
			   gint      column)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (column < 0 || column >= clist->columns)
    return;

  clist->sort_column = column;
}

/* PRIVATE SORTING FUNCTIONS
 *   default_compare
 *   real_sort_list
 *   btk_clist_merge
 *   btk_clist_mergesort
 */
static gint
default_compare (BtkCList      *clist,
		 gconstpointer  ptr1,
		 gconstpointer  ptr2)
{
  char *text1 = NULL;
  char *text2 = NULL;

  BtkCListRow *row1 = (BtkCListRow *) ptr1;
  BtkCListRow *row2 = (BtkCListRow *) ptr2;

  switch (row1->cell[clist->sort_column].type)
    {
    case BTK_CELL_TEXT:
      text1 = BTK_CELL_TEXT (row1->cell[clist->sort_column])->text;
      break;
    case BTK_CELL_PIXTEXT:
      text1 = BTK_CELL_PIXTEXT (row1->cell[clist->sort_column])->text;
      break;
    default:
      break;
    }
 
  switch (row2->cell[clist->sort_column].type)
    {
    case BTK_CELL_TEXT:
      text2 = BTK_CELL_TEXT (row2->cell[clist->sort_column])->text;
      break;
    case BTK_CELL_PIXTEXT:
      text2 = BTK_CELL_PIXTEXT (row2->cell[clist->sort_column])->text;
      break;
    default:
      break;
    }

  if (!text2)
    return (text1 != NULL);

  if (!text1)
    return -1;

  return strcmp (text1, text2);
}

static void
real_sort_list (BtkCList *clist)
{
  GList *list;
  GList *work;
  gint i;

  g_return_if_fail (BTK_IS_CLIST (clist));

  if (clist->rows <= 1)
    return;

  if (clist_has_grab (clist))
    return;

  btk_clist_freeze (clist);

  if (clist->anchor != -1 && clist->selection_mode == BTK_SELECTION_MULTIPLE)
    {
      BTK_CLIST_GET_CLASS (clist)->resync_selection (clist, NULL);
      g_list_free (clist->undo_selection);
      g_list_free (clist->undo_unselection);
      clist->undo_selection = NULL;
      clist->undo_unselection = NULL;
    }
   
  clist->row_list = btk_clist_mergesort (clist, clist->row_list, clist->rows);

  work = clist->selection;

  for (i = 0, list = clist->row_list; i < clist->rows; i++, list = list->next)
    {
      if (BTK_CLIST_ROW (list)->state == BTK_STATE_SELECTED)
	{
	  work->data = GINT_TO_POINTER (i);
	  work = work->next;
	}
      
      if (i == clist->rows - 1)
	clist->row_list_end = list;
    }

  btk_clist_thaw (clist);
}

static GList *
btk_clist_merge (BtkCList *clist,
		 GList    *a,         /* first list to merge */
		 GList    *b)         /* second list to merge */
{
  GList z = { 0 };                    /* auxiliary node */
  GList *c;
  gint cmp;

  c = &z;

  while (a || b)
    {
      if (a && !b)
	{
	  c->next = a;
	  a->prev = c;
	  c = a;
	  a = a->next;
	  break;
	}
      else if (!a && b)
	{
	  c->next = b;
	  b->prev = c;
	  c = b;
	  b = b->next;
	  break;
	}
      else /* a && b */
	{
	  cmp = clist->compare (clist, BTK_CLIST_ROW (a), BTK_CLIST_ROW (b));
	  if ((cmp >= 0 && clist->sort_type == BTK_SORT_DESCENDING) ||
	      (cmp <= 0 && clist->sort_type == BTK_SORT_ASCENDING) ||
	      (a && !b))
	    {
	      c->next = a;
	      a->prev = c;
	      c = a;
	      a = a->next;
	    }
	  else
	    {
	      c->next = b;
	      b->prev = c;
	      c = b;
	      b = b->next;
	    }
	}
    }

  z.next->prev = NULL;
  return z.next;
}

static GList *
btk_clist_mergesort (BtkCList *clist,
		     GList    *list,         /* the list to sort */
		     gint      num)          /* the list's length */
{
  GList *half;
  gint i;

  if (num <= 1)
    {
      return list;
    }
  else
    {
      /* move "half" to the middle */
      half = list;
      for (i = 0; i < num / 2; i++)
	half = half->next;

      /* cut the list in two */
      half->prev->next = NULL;
      half->prev = NULL;

      /* recursively sort both lists */
      return btk_clist_merge (clist,
		       btk_clist_mergesort (clist, list, num / 2),
		       btk_clist_mergesort (clist, half, num - num / 2));
    }
}

/************************/

static void
drag_source_info_destroy (gpointer data)
{
  BtkCListCellInfo *info = data;

  g_free (info);
}

static void
drag_dest_info_destroy (gpointer data)
{
  BtkCListDestInfo *info = data;

  g_free (info);
}

static void
drag_dest_cell (BtkCList         *clist,
		gint              x,
		gint              y,
		BtkCListDestInfo *dest_info)
{
  BtkWidget *widget;

  widget = BTK_WIDGET (clist);

  dest_info->insert_pos = BTK_CLIST_DRAG_NONE;

  y -= (BTK_CONTAINER (clist)->border_width +
	widget->style->ythickness +
	clist->column_title_area.height);

  dest_info->cell.row = ROW_FROM_YPIXEL (clist, y);
  if (dest_info->cell.row >= clist->rows)
    {
      dest_info->cell.row = clist->rows - 1;
      y = ROW_TOP_YPIXEL (clist, dest_info->cell.row) + clist->row_height;
    }
  if (dest_info->cell.row < -1)
    dest_info->cell.row = -1;
  
  x -= BTK_CONTAINER (widget)->border_width + widget->style->xthickness;

  dest_info->cell.column = COLUMN_FROM_XPIXEL (clist, x);

  if (dest_info->cell.row >= 0)
    {
      gint y_delta;
      gint h = 0;

      y_delta = y - ROW_TOP_YPIXEL (clist, dest_info->cell.row);
      
      if (BTK_CLIST_DRAW_DRAG_RECT(clist))
	{
	  dest_info->insert_pos = BTK_CLIST_DRAG_INTO;
	  h = clist->row_height / 4;
	}
      else if (BTK_CLIST_DRAW_DRAG_LINE(clist))
	{
	  dest_info->insert_pos = BTK_CLIST_DRAG_BEFORE;
	  h = clist->row_height / 2;
	}

      if (BTK_CLIST_DRAW_DRAG_LINE(clist))
	{
	  if (y_delta < h)
	    dest_info->insert_pos = BTK_CLIST_DRAG_BEFORE;
	  else if (clist->row_height - y_delta < h)
	    dest_info->insert_pos = BTK_CLIST_DRAG_AFTER;
	}
    }
}

static void
btk_clist_drag_begin (BtkWidget	     *widget,
		      BdkDragContext *context)
{
  BtkCList *clist;
  BtkCListCellInfo *info;

  g_return_if_fail (BTK_IS_CLIST (widget));
  g_return_if_fail (context != NULL);

  clist = BTK_CLIST (widget);

  clist->drag_button = 0;
  remove_grab (clist);

  switch (clist->selection_mode)
    {
    case BTK_SELECTION_MULTIPLE:
      update_extended_selection (clist, clist->focus_row);
      BTK_CLIST_GET_CLASS (clist)->resync_selection (clist, NULL);
      break;
    case BTK_SELECTION_SINGLE:
      clist->anchor = -1;
    case BTK_SELECTION_BROWSE:
      break;
    default:
      g_assert_not_reached ();
    }

  info = g_dataset_get_data (context, "btk-clist-drag-source");

  if (!info)
    {
      info = g_new (BtkCListCellInfo, 1);

      if (clist->click_cell.row < 0)
	clist->click_cell.row = 0;
      else if (clist->click_cell.row >= clist->rows)
	clist->click_cell.row = clist->rows - 1;
      info->row = clist->click_cell.row;
      info->column = clist->click_cell.column;

      g_dataset_set_data_full (context, "btk-clist-drag-source", info,
			       drag_source_info_destroy);
    }

  if (BTK_CLIST_USE_DRAG_ICONS (clist))
    btk_drag_set_icon_default (context);
}

static void
btk_clist_drag_end (BtkWidget	   *widget,
		    BdkDragContext *context)
{
  BtkCList *clist;

  g_return_if_fail (BTK_IS_CLIST (widget));
  g_return_if_fail (context != NULL);

  clist = BTK_CLIST (widget);

  clist->click_cell.row = -1;
  clist->click_cell.column = -1;
}

static void
btk_clist_drag_leave (BtkWidget      *widget,
		      BdkDragContext *context,
		      guint           time)
{
  BtkCList *clist;
  BtkCListDestInfo *dest_info;

  g_return_if_fail (BTK_IS_CLIST (widget));
  g_return_if_fail (context != NULL);

  clist = BTK_CLIST (widget);

  dest_info = g_dataset_get_data (context, "btk-clist-drag-dest");
  
  if (dest_info)
    {
      if (dest_info->cell.row >= 0 &&
	  BTK_CLIST_REORDERABLE(clist) &&
	  btk_drag_get_source_widget (context) == widget)
	{
	  GList *list;
	  BdkAtom atom = bdk_atom_intern_static_string ("btk-clist-drag-reorder");

	  list = context->targets;
	  while (list)
	    {
	      if (atom == BDK_POINTER_TO_ATOM (list->data))
		{
		  BTK_CLIST_GET_CLASS (clist)->draw_drag_highlight
		    (clist,
		     g_list_nth (clist->row_list, dest_info->cell.row)->data,
		     dest_info->cell.row, dest_info->insert_pos);
		  clist->drag_highlight_row = -1;
		  break;
		}
	      list = list->next;
	    }
	}
      g_dataset_remove_data (context, "btk-clist-drag-dest");
    }
}

static gint
btk_clist_drag_motion (BtkWidget      *widget,
		       BdkDragContext *context,
		       gint            x,
		       gint            y,
		       guint           time)
{
  BtkCList *clist;
  BtkCListDestInfo new_info;
  BtkCListDestInfo *dest_info;

  g_return_val_if_fail (BTK_IS_CLIST (widget), FALSE);

  clist = BTK_CLIST (widget);

  dest_info = g_dataset_get_data (context, "btk-clist-drag-dest");

  if (!dest_info)
    {
      dest_info = g_new (BtkCListDestInfo, 1);

      dest_info->insert_pos  = BTK_CLIST_DRAG_NONE;
      dest_info->cell.row    = -1;
      dest_info->cell.column = -1;

      g_dataset_set_data_full (context, "btk-clist-drag-dest", dest_info,
			       drag_dest_info_destroy);
    }

  drag_dest_cell (clist, x, y, &new_info);

  if (BTK_CLIST_REORDERABLE (clist))
    {
      GList *list;
      BdkAtom atom = bdk_atom_intern_static_string ("btk-clist-drag-reorder");

      list = context->targets;
      while (list)
	{
	  if (atom == BDK_POINTER_TO_ATOM (list->data))
	    break;
	  list = list->next;
	}

      if (list)
	{
	  if (btk_drag_get_source_widget (context) != widget ||
	      new_info.insert_pos == BTK_CLIST_DRAG_NONE ||
	      new_info.cell.row == clist->click_cell.row ||
	      (new_info.cell.row == clist->click_cell.row - 1 &&
	       new_info.insert_pos == BTK_CLIST_DRAG_AFTER) ||
	      (new_info.cell.row == clist->click_cell.row + 1 &&
	       new_info.insert_pos == BTK_CLIST_DRAG_BEFORE))
	    {
	      if (dest_info->cell.row < 0)
		{
		  bdk_drag_status (context, BDK_ACTION_DEFAULT, time);
		  return FALSE;
		}
	      return TRUE;
	    }
		
	  if (new_info.cell.row != dest_info->cell.row ||
	      (new_info.cell.row == dest_info->cell.row &&
	       dest_info->insert_pos != new_info.insert_pos))
	    {
	      if (dest_info->cell.row >= 0)
		BTK_CLIST_GET_CLASS (clist)->draw_drag_highlight
		  (clist, g_list_nth (clist->row_list,
				      dest_info->cell.row)->data,
		   dest_info->cell.row, dest_info->insert_pos);

	      dest_info->insert_pos  = new_info.insert_pos;
	      dest_info->cell.row    = new_info.cell.row;
	      dest_info->cell.column = new_info.cell.column;
	      
	      BTK_CLIST_GET_CLASS (clist)->draw_drag_highlight
		(clist, g_list_nth (clist->row_list,
				    dest_info->cell.row)->data,
		 dest_info->cell.row, dest_info->insert_pos);
	      
	      clist->drag_highlight_row = dest_info->cell.row;
	      clist->drag_highlight_pos = dest_info->insert_pos;

	      bdk_drag_status (context, context->suggested_action, time);
	    }
	  return TRUE;
	}
    }

  dest_info->insert_pos  = new_info.insert_pos;
  dest_info->cell.row    = new_info.cell.row;
  dest_info->cell.column = new_info.cell.column;
  return TRUE;
}

static gboolean
btk_clist_drag_drop (BtkWidget      *widget,
		     BdkDragContext *context,
		     gint            x,
		     gint            y,
		     guint           time)
{
  g_return_val_if_fail (BTK_IS_CLIST (widget), FALSE);
  g_return_val_if_fail (context != NULL, FALSE);

  if (BTK_CLIST_REORDERABLE (widget) &&
      btk_drag_get_source_widget (context) == widget)
    {
      GList *list;
      BdkAtom atom = bdk_atom_intern_static_string ("btk-clist-drag-reorder");

      list = context->targets;
      while (list)
	{
	  if (atom == BDK_POINTER_TO_ATOM (list->data))
	    return TRUE;
	  list = list->next;
	}
    }
  return FALSE;
}

static void
btk_clist_drag_data_received (BtkWidget        *widget,
			      BdkDragContext   *context,
			      gint              x,
			      gint              y,
			      BtkSelectionData *selection_data,
			      guint             info,
			      guint             time)
{
  BtkCList *clist;

  g_return_if_fail (BTK_IS_CLIST (widget));
  g_return_if_fail (context != NULL);
  g_return_if_fail (selection_data != NULL);

  clist = BTK_CLIST (widget);

  if (BTK_CLIST_REORDERABLE (clist) &&
      btk_drag_get_source_widget (context) == widget &&
      selection_data->target ==
      bdk_atom_intern_static_string ("btk-clist-drag-reorder") &&
      selection_data->format == 8 &&
      selection_data->length == sizeof (BtkCListCellInfo))
    {
      BtkCListCellInfo *source_info;

      source_info = (BtkCListCellInfo *)(selection_data->data);
      if (source_info)
	{
	  BtkCListDestInfo dest_info;

	  drag_dest_cell (clist, x, y, &dest_info);

	  if (dest_info.insert_pos == BTK_CLIST_DRAG_AFTER)
	    dest_info.cell.row++;
	  if (source_info->row < dest_info.cell.row)
	    dest_info.cell.row--;
	  if (dest_info.cell.row != source_info->row)
	    btk_clist_row_move (clist, source_info->row, dest_info.cell.row);

	  g_dataset_remove_data (context, "btk-clist-drag-dest");
	}
    }
}

static void  
btk_clist_drag_data_get (BtkWidget        *widget,
			 BdkDragContext   *context,
			 BtkSelectionData *selection_data,
			 guint             info,
			 guint             time)
{
  g_return_if_fail (BTK_IS_CLIST (widget));
  g_return_if_fail (context != NULL);
  g_return_if_fail (selection_data != NULL);

  if (selection_data->target == bdk_atom_intern_static_string ("btk-clist-drag-reorder"))
    {
      BtkCListCellInfo *info;

      info = g_dataset_get_data (context, "btk-clist-drag-source");

      if (info)
	{
	  BtkCListCellInfo ret_info;

	  ret_info.row = info->row;
	  ret_info.column = info->column;

	  btk_selection_data_set (selection_data, selection_data->target,
				  8, (guchar *) &ret_info,
				  sizeof (BtkCListCellInfo));
	}
    }
}

static void
draw_drag_highlight (BtkCList        *clist,
		     BtkCListRow     *dest_row,
		     gint             dest_row_number,
		     BtkCListDragPos  drag_pos)
{
  gint y;

  y = ROW_TOP_YPIXEL (clist, dest_row_number) - 1;

  switch (drag_pos)
    {
    case BTK_CLIST_DRAG_NONE:
      break;
    case BTK_CLIST_DRAG_AFTER:
      y += clist->row_height + 1;
    case BTK_CLIST_DRAG_BEFORE:
      bdk_draw_line (clist->clist_window, clist->xor_gc,
		     0, y, clist->clist_window_width, y);
      break;
    case BTK_CLIST_DRAG_INTO:
      bdk_draw_rectangle (clist->clist_window, clist->xor_gc, FALSE, 0, y,
			  clist->clist_window_width - 1, clist->row_height);
      break;
    }
}

void
btk_clist_set_reorderable (BtkCList *clist, 
			   gboolean  reorderable)
{
  BtkWidget *widget;

  g_return_if_fail (BTK_IS_CLIST (clist));

  if ((BTK_CLIST_REORDERABLE(clist) != 0) == reorderable)
    return;

  widget = BTK_WIDGET (clist);

  if (reorderable)
    {
      BTK_CLIST_SET_FLAG (clist, CLIST_REORDERABLE);
      btk_drag_dest_set (widget,
			 BTK_DEST_DEFAULT_MOTION | BTK_DEST_DEFAULT_DROP,
			 &clist_target_table, 1, BDK_ACTION_MOVE);
    }
  else
    {
      BTK_CLIST_UNSET_FLAG (clist, CLIST_REORDERABLE);
      btk_drag_dest_unset (BTK_WIDGET (clist));
    }
}

void
btk_clist_set_use_drag_icons (BtkCList *clist,
			      gboolean  use_icons)
{
  g_return_if_fail (BTK_IS_CLIST (clist));

  if (use_icons != 0)
    BTK_CLIST_SET_FLAG (clist, CLIST_USE_DRAG_ICONS);
  else
    BTK_CLIST_UNSET_FLAG (clist, CLIST_USE_DRAG_ICONS);
}

void
btk_clist_set_button_actions (BtkCList *clist,
			      guint     button,
			      guint8    button_actions)
{
  g_return_if_fail (BTK_IS_CLIST (clist));
  
  if (button < MAX_BUTTON)
    {
      if (bdk_display_pointer_is_grabbed (btk_widget_get_display (BTK_WIDGET (clist))) || 
	  BTK_WIDGET_HAS_GRAB (clist))
	{
	  remove_grab (clist);
	  clist->drag_button = 0;
	}

      BTK_CLIST_GET_CLASS (clist)->resync_selection (clist, NULL);

      clist->button_actions[button] = button_actions;
    }
}

#include "btkaliasdef.c"
