/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball, Josh MacDonald, 
 * Copyright (C) 1997-1998 Jay Painter <jpaint@serv.net><jpaint@gimp.org>  
 *
 * BtkCTree widget for BTK+
 * Copyright (C) 1998 Lars Hamann and Stefan Jeske
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

#undef BDK_DISABLE_DEPRECATED
#undef BTK_DISABLE_DEPRECATED
#define __BTK_CTREE_C__

#include "btkctree.h"
#include "btkbindings.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkdnd.h"
#include "btkintl.h"
#include <bdk/bdkkeysyms.h>

#include "btkalias.h"

#define PM_SIZE                    8
#define TAB_SIZE                   (PM_SIZE + 6)
#define CELL_SPACING               1
#define CLIST_OPTIMUM_SIZE         64
#define COLUMN_INSET               3
#define DRAG_WIDTH                 6

#define ROW_TOP_YPIXEL(clist, row) (((clist)->row_height * (row)) + \
				    (((row) + 1) * CELL_SPACING) + \
				    (clist)->voffset)
#define ROW_FROM_YPIXEL(clist, y)  (((y) - (clist)->voffset) / \
                                    ((clist)->row_height + CELL_SPACING))
#define COLUMN_LEFT_XPIXEL(clist, col)  ((clist)->column[(col)].area.x \
                                    + (clist)->hoffset)
#define COLUMN_LEFT(clist, column) ((clist)->column[(column)].area.x)

static inline bint
COLUMN_FROM_XPIXEL (BtkCList * clist,
		    bint x)
{
  bint i, cx;

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

#define CLIST_UNFROZEN(clist)     (((BtkCList*) (clist))->freeze_count == 0)
#define CLIST_REFRESH(clist)    B_STMT_START { \
  if (CLIST_UNFROZEN (clist)) \
    BTK_CLIST_GET_CLASS (clist)->refresh ((BtkCList*) (clist)); \
} B_STMT_END


enum {
  ARG_0,
  ARG_N_COLUMNS,
  ARG_TREE_COLUMN,
  ARG_INDENT,
  ARG_SPACING,
  ARG_SHOW_STUB,
  ARG_LINE_STYLE,
  ARG_EXPANDER_STYLE
};


static void     btk_ctree_class_init    (BtkCTreeClass         *klass);
static void     btk_ctree_init          (BtkCTree              *ctree);
static BObject* btk_ctree_constructor   (GType                  type,
				         buint                  n_construct_properties,
				         BObjectConstructParam *construct_params);
static void btk_ctree_set_arg		(BtkObject      *object,
					 BtkArg         *arg,
					 buint           arg_id);
static void btk_ctree_get_arg      	(BtkObject      *object,
					 BtkArg         *arg,
					 buint           arg_id);
static void btk_ctree_realize           (BtkWidget      *widget);
static void btk_ctree_unrealize         (BtkWidget      *widget);
static bint btk_ctree_button_press      (BtkWidget      *widget,
					 BdkEventButton *event);
static void ctree_attach_styles         (BtkCTree       *ctree,
					 BtkCTreeNode   *node,
					 bpointer        data);
static void ctree_detach_styles         (BtkCTree       *ctree,
					 BtkCTreeNode   *node, 
					 bpointer        data);
static bint draw_cell_pixmap            (BdkWindow      *window,
					 BdkRectangle   *clip_rectangle,
					 BdkGC          *fg_gc,
					 BdkPixmap      *pixmap,
					 BdkBitmap      *mask,
					 bint            x,
					 bint            y,
					 bint            width,
					 bint            height);
static void get_cell_style              (BtkCList       *clist,
					 BtkCListRow    *clist_row,
					 bint            state,
					 bint            column,
					 BtkStyle      **style,
					 BdkGC         **fg_gc,
					 BdkGC         **bg_gc);
static bint btk_ctree_draw_expander     (BtkCTree       *ctree,
					 BtkCTreeRow    *ctree_row,
					 BtkStyle       *style,
					 BdkRectangle   *clip_rectangle,
					 bint            x);
static bint btk_ctree_draw_lines        (BtkCTree       *ctree,
					 BtkCTreeRow    *ctree_row,
					 bint            row,
					 bint            column,
					 bint            state,
					 BdkRectangle   *clip_rectangle,
					 BdkRectangle   *cell_rectangle,
					 BdkRectangle   *crect,
					 BdkRectangle   *area,
					 BtkStyle       *style);
static void draw_row                    (BtkCList       *clist,
					 BdkRectangle   *area,
					 bint            row,
					 BtkCListRow    *clist_row);
static void draw_drag_highlight         (BtkCList        *clist,
					 BtkCListRow     *dest_row,
					 bint             dest_row_number,
					 BtkCListDragPos  drag_pos);
static void tree_draw_node              (BtkCTree      *ctree,
					 BtkCTreeNode  *node);
static void set_cell_contents           (BtkCList      *clist,
					 BtkCListRow   *clist_row,
					 bint           column,
					 BtkCellType    type,
					 const bchar   *text,
					 buint8         spacing,
					 BdkPixmap     *pixmap,
					 BdkBitmap     *mask);
static void set_node_info               (BtkCTree      *ctree,
					 BtkCTreeNode  *node,
					 const bchar   *text,
					 buint8         spacing,
					 BdkPixmap     *pixmap_closed,
					 BdkBitmap     *mask_closed,
					 BdkPixmap     *pixmap_opened,
					 BdkBitmap     *mask_opened,
					 bboolean       is_leaf,
					 bboolean       expanded);
static BtkCTreeRow *row_new             (BtkCTree      *ctree);
static void row_delete                  (BtkCTree      *ctree,
				 	 BtkCTreeRow   *ctree_row);
static void tree_delete                 (BtkCTree      *ctree, 
					 BtkCTreeNode  *node, 
					 bpointer       data);
static void tree_delete_row             (BtkCTree      *ctree, 
					 BtkCTreeNode  *node, 
					 bpointer       data);
static void real_clear                  (BtkCList      *clist);
static void tree_update_level           (BtkCTree      *ctree, 
					 BtkCTreeNode  *node, 
					 bpointer       data);
static void tree_select                 (BtkCTree      *ctree, 
					 BtkCTreeNode  *node, 
					 bpointer       data);
static void tree_unselect               (BtkCTree      *ctree, 
					 BtkCTreeNode  *node, 
				         bpointer       data);
static void real_select_all             (BtkCList      *clist);
static void real_unselect_all           (BtkCList      *clist);
static void tree_expand                 (BtkCTree      *ctree, 
					 BtkCTreeNode  *node,
					 bpointer       data);
static void tree_collapse               (BtkCTree      *ctree, 
					 BtkCTreeNode  *node,
					 bpointer       data);
static void tree_collapse_to_depth      (BtkCTree      *ctree, 
					 BtkCTreeNode  *node, 
					 bint           depth);
static void tree_toggle_expansion       (BtkCTree      *ctree,
					 BtkCTreeNode  *node,
					 bpointer       data);
static void change_focus_row_expansion  (BtkCTree      *ctree,
				         BtkCTreeExpansionType expansion);
static void real_select_row             (BtkCList      *clist,
					 bint           row,
					 bint           column,
					 BdkEvent      *event);
static void real_unselect_row           (BtkCList      *clist,
					 bint           row,
					 bint           column,
					 BdkEvent      *event);
static void real_tree_select            (BtkCTree      *ctree,
					 BtkCTreeNode  *node,
					 bint           column);
static void real_tree_unselect          (BtkCTree      *ctree,
					 BtkCTreeNode  *node,
					 bint           column);
static void real_tree_expand            (BtkCTree      *ctree,
					 BtkCTreeNode  *node);
static void real_tree_collapse          (BtkCTree      *ctree,
					 BtkCTreeNode  *node);
static void real_tree_move              (BtkCTree      *ctree,
					 BtkCTreeNode  *node,
					 BtkCTreeNode  *new_parent, 
					 BtkCTreeNode  *new_sibling);
static void real_row_move               (BtkCList      *clist,
					 bint           source_row,
					 bint           dest_row);
static void btk_ctree_link              (BtkCTree      *ctree,
					 BtkCTreeNode  *node,
					 BtkCTreeNode  *parent,
					 BtkCTreeNode  *sibling,
					 bboolean       update_focus_row);
static void btk_ctree_unlink            (BtkCTree      *ctree, 
					 BtkCTreeNode  *node,
					 bboolean       update_focus_row);
static BtkCTreeNode * btk_ctree_last_visible (BtkCTree     *ctree,
					      BtkCTreeNode *node);
static bboolean ctree_is_hot_spot       (BtkCTree      *ctree, 
					 BtkCTreeNode  *node,
					 bint           row, 
					 bint           x, 
					 bint           y);
static void tree_sort                   (BtkCTree      *ctree,
					 BtkCTreeNode  *node,
					 bpointer       data);
static void fake_unselect_all           (BtkCList      *clist,
					 bint           row);
static GList * selection_find           (BtkCList      *clist,
					 bint           row_number,
					 GList         *row_list_element);
static void resync_selection            (BtkCList      *clist,
					 BdkEvent      *event);
static void real_undo_selection         (BtkCList      *clist);
static void select_row_recursive        (BtkCTree      *ctree, 
					 BtkCTreeNode  *node, 
					 bpointer       data);
static bint real_insert_row             (BtkCList      *clist,
					 bint           row,
					 bchar         *text[]);
static void real_remove_row             (BtkCList      *clist,
					 bint           row);
static void real_sort_list              (BtkCList      *clist);
static void cell_size_request           (BtkCList       *clist,
					 BtkCListRow    *clist_row,
					 bint            column,
					 BtkRequisition *requisition);
static void column_auto_resize          (BtkCList       *clist,
					 BtkCListRow    *clist_row,
					 bint            column,
					 bint            old_width);
static void auto_resize_columns         (BtkCList       *clist);


static bboolean check_drag               (BtkCTree         *ctree,
			                  BtkCTreeNode     *drag_source,
					  BtkCTreeNode     *drag_target,
					  BtkCListDragPos   insert_pos);
static void btk_ctree_drag_begin         (BtkWidget        *widget,
					  BdkDragContext   *context);
static bint btk_ctree_drag_motion        (BtkWidget        *widget,
					  BdkDragContext   *context,
					  bint              x,
					  bint              y,
					  buint             time);
static void btk_ctree_drag_data_received (BtkWidget        *widget,
					  BdkDragContext   *context,
					  bint              x,
					  bint              y,
					  BtkSelectionData *selection_data,
					  buint             info,
					  buint32           time);
static void remove_grab                  (BtkCList         *clist);
static void drag_dest_cell               (BtkCList         *clist,
					  bint              x,
					  bint              y,
					  BtkCListDestInfo *dest_info);


enum
{
  TREE_SELECT_ROW,
  TREE_UNSELECT_ROW,
  TREE_EXPAND,
  TREE_COLLAPSE,
  TREE_MOVE,
  CHANGE_FOCUS_ROW_EXPANSION,
  LAST_SIGNAL
};

static BtkCListClass *parent_class = NULL;
static BtkContainerClass *container_class = NULL;
static buint ctree_signals[LAST_SIGNAL] = {0};


BtkType
btk_ctree_get_type (void)
{
  static BtkType ctree_type = 0;

  if (!ctree_type)
    {
      static const BtkTypeInfo ctree_info =
      {
	"BtkCTree",
	sizeof (BtkCTree),
	sizeof (BtkCTreeClass),
	(BtkClassInitFunc) btk_ctree_class_init,
	(BtkObjectInitFunc) btk_ctree_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (BtkClassInitFunc) NULL,
      };

      I_("BtkCTree");
      ctree_type = btk_type_unique (BTK_TYPE_CLIST, &ctree_info);
    }

  return ctree_type;
}

static void
btk_ctree_class_init (BtkCTreeClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;
  BtkCListClass *clist_class;
  BtkBindingSet *binding_set;

  bobject_class->constructor = btk_ctree_constructor;

  object_class = (BtkObjectClass *) klass;
  widget_class = (BtkWidgetClass *) klass;
  container_class = (BtkContainerClass *) klass;
  clist_class = (BtkCListClass *) klass;

  parent_class = btk_type_class (BTK_TYPE_CLIST);
  container_class = btk_type_class (BTK_TYPE_CONTAINER);

  object_class->set_arg = btk_ctree_set_arg;
  object_class->get_arg = btk_ctree_get_arg;

  widget_class->realize = btk_ctree_realize;
  widget_class->unrealize = btk_ctree_unrealize;
  widget_class->button_press_event = btk_ctree_button_press;

  widget_class->drag_begin = btk_ctree_drag_begin;
  widget_class->drag_motion = btk_ctree_drag_motion;
  widget_class->drag_data_received = btk_ctree_drag_data_received;

  clist_class->select_row = real_select_row;
  clist_class->unselect_row = real_unselect_row;
  clist_class->row_move = real_row_move;
  clist_class->undo_selection = real_undo_selection;
  clist_class->resync_selection = resync_selection;
  clist_class->selection_find = selection_find;
  clist_class->click_column = NULL;
  clist_class->draw_row = draw_row;
  clist_class->draw_drag_highlight = draw_drag_highlight;
  clist_class->clear = real_clear;
  clist_class->select_all = real_select_all;
  clist_class->unselect_all = real_unselect_all;
  clist_class->fake_unselect_all = fake_unselect_all;
  clist_class->insert_row = real_insert_row;
  clist_class->remove_row = real_remove_row;
  clist_class->sort_list = real_sort_list;
  clist_class->set_cell_contents = set_cell_contents;
  clist_class->cell_size_request = cell_size_request;

  klass->tree_select_row = real_tree_select;
  klass->tree_unselect_row = real_tree_unselect;
  klass->tree_expand = real_tree_expand;
  klass->tree_collapse = real_tree_collapse;
  klass->tree_move = real_tree_move;
  klass->change_focus_row_expansion = change_focus_row_expansion;

  btk_object_add_arg_type ("BtkCTree::n-columns", /* overrides BtkCList::n_columns!! */
			   BTK_TYPE_UINT,
			   BTK_ARG_READWRITE | BTK_ARG_CONSTRUCT_ONLY | G_PARAM_STATIC_NAME,
			   ARG_N_COLUMNS);
  btk_object_add_arg_type ("BtkCTree::tree-column",
			   BTK_TYPE_UINT,
			   BTK_ARG_READWRITE | BTK_ARG_CONSTRUCT_ONLY | G_PARAM_STATIC_NAME,
			   ARG_TREE_COLUMN);
  btk_object_add_arg_type ("BtkCTree::indent",
			   BTK_TYPE_UINT,
			   BTK_ARG_READWRITE | G_PARAM_STATIC_NAME,
			   ARG_INDENT);
  btk_object_add_arg_type ("BtkCTree::spacing",
			   BTK_TYPE_UINT,
			   BTK_ARG_READWRITE | G_PARAM_STATIC_NAME,
			   ARG_SPACING);
  btk_object_add_arg_type ("BtkCTree::show-stub",
			   BTK_TYPE_BOOL,
			   BTK_ARG_READWRITE | G_PARAM_STATIC_NAME,
			   ARG_SHOW_STUB);
  btk_object_add_arg_type ("BtkCTree::line-style",
			   BTK_TYPE_CTREE_LINE_STYLE,
			   BTK_ARG_READWRITE | G_PARAM_STATIC_NAME,
			   ARG_LINE_STYLE);
  btk_object_add_arg_type ("BtkCTree::expander-style",
			   BTK_TYPE_CTREE_EXPANDER_STYLE,
			   BTK_ARG_READWRITE | G_PARAM_STATIC_NAME,
			   ARG_EXPANDER_STYLE);

  ctree_signals[TREE_SELECT_ROW] =
    btk_signal_new (I_("tree-select-row"),
		    BTK_RUN_FIRST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkCTreeClass, tree_select_row),
		    _btk_marshal_VOID__POINTER_INT,
		    BTK_TYPE_NONE, 2,
		    BTK_TYPE_CTREE_NODE,
		    BTK_TYPE_INT);
  ctree_signals[TREE_UNSELECT_ROW] =
    btk_signal_new (I_("tree-unselect-row"),
		    BTK_RUN_FIRST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkCTreeClass, tree_unselect_row),
		    _btk_marshal_VOID__POINTER_INT,
		    BTK_TYPE_NONE, 2,
		    BTK_TYPE_CTREE_NODE,
		    BTK_TYPE_INT);
  ctree_signals[TREE_EXPAND] =
    btk_signal_new (I_("tree-expand"),
		    BTK_RUN_LAST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkCTreeClass, tree_expand),
		    _btk_marshal_VOID__POINTER,
		    BTK_TYPE_NONE, 1,
		    BTK_TYPE_CTREE_NODE);
  ctree_signals[TREE_COLLAPSE] =
    btk_signal_new (I_("tree-collapse"),
		    BTK_RUN_LAST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkCTreeClass, tree_collapse),
		    _btk_marshal_VOID__POINTER,
		    BTK_TYPE_NONE, 1,
		    BTK_TYPE_CTREE_NODE);
  ctree_signals[TREE_MOVE] =
    btk_signal_new (I_("tree-move"),
		    BTK_RUN_LAST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkCTreeClass, tree_move),
		    _btk_marshal_VOID__POINTER_POINTER_POINTER,
		    BTK_TYPE_NONE, 3,
		    BTK_TYPE_CTREE_NODE,
		    BTK_TYPE_CTREE_NODE,
		    BTK_TYPE_CTREE_NODE);
  ctree_signals[CHANGE_FOCUS_ROW_EXPANSION] =
    btk_signal_new (I_("change-focus-row-expansion"),
		    BTK_RUN_LAST | BTK_RUN_ACTION,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkCTreeClass,
				       change_focus_row_expansion),
		    _btk_marshal_VOID__ENUM,
		    BTK_TYPE_NONE, 1, BTK_TYPE_CTREE_EXPANSION_TYPE);

  binding_set = btk_binding_set_by_class (klass);
  btk_binding_entry_add_signal (binding_set,
				BDK_plus, 0,
				"change-focus-row-expansion", 1,
				BTK_TYPE_ENUM, BTK_CTREE_EXPANSION_EXPAND);
  btk_binding_entry_add_signal (binding_set,
				BDK_plus, BDK_CONTROL_MASK,
				"change-focus-row-expansion", 1,
				BTK_TYPE_ENUM, BTK_CTREE_EXPANSION_EXPAND_RECURSIVE);

  btk_binding_entry_add_signal (binding_set,
				BDK_KP_Add, 0,
				"change-focus-row-expansion", 1,
				BTK_TYPE_ENUM, BTK_CTREE_EXPANSION_EXPAND);
  btk_binding_entry_add_signal (binding_set,
				BDK_KP_Add, BDK_CONTROL_MASK,
				"change-focus-row-expansion", 1,
				BTK_TYPE_ENUM, BTK_CTREE_EXPANSION_EXPAND_RECURSIVE);
  
  btk_binding_entry_add_signal (binding_set,
				BDK_minus, 0,
				"change-focus-row-expansion", 1,
				BTK_TYPE_ENUM, BTK_CTREE_EXPANSION_COLLAPSE);
  btk_binding_entry_add_signal (binding_set,
                                BDK_minus, BDK_CONTROL_MASK,
				"change-focus-row-expansion", 1,
				BTK_TYPE_ENUM,
				BTK_CTREE_EXPANSION_COLLAPSE_RECURSIVE);
  btk_binding_entry_add_signal (binding_set,
				BDK_KP_Subtract, 0,
				"change-focus-row-expansion", 1,
				BTK_TYPE_ENUM, BTK_CTREE_EXPANSION_COLLAPSE);
  btk_binding_entry_add_signal (binding_set,
				BDK_KP_Subtract, BDK_CONTROL_MASK,
				"change-focus-row-expansion", 1,
				BTK_TYPE_ENUM,
				BTK_CTREE_EXPANSION_COLLAPSE_RECURSIVE);
  btk_binding_entry_add_signal (binding_set,
				BDK_equal, 0,
				"change-focus-row-expansion", 1,
				BTK_TYPE_ENUM, BTK_CTREE_EXPANSION_TOGGLE);
  btk_binding_entry_add_signal (binding_set,
				BDK_KP_Equal, 0,
				"change-focus-row-expansion", 1,
				BTK_TYPE_ENUM, BTK_CTREE_EXPANSION_TOGGLE);
  btk_binding_entry_add_signal (binding_set,
				BDK_KP_Multiply, 0,
				"change-focus-row-expansion", 1,
				BTK_TYPE_ENUM, BTK_CTREE_EXPANSION_TOGGLE);
  btk_binding_entry_add_signal (binding_set,
				BDK_asterisk, 0,
				"change-focus-row-expansion", 1,
				BTK_TYPE_ENUM, BTK_CTREE_EXPANSION_TOGGLE);
  btk_binding_entry_add_signal (binding_set,
				BDK_KP_Multiply, BDK_CONTROL_MASK,
				"change-focus-row-expansion", 1,
				BTK_TYPE_ENUM,
				BTK_CTREE_EXPANSION_TOGGLE_RECURSIVE);
  btk_binding_entry_add_signal (binding_set,
				BDK_asterisk, BDK_CONTROL_MASK,
				"change-focus-row-expansion", 1,
				BTK_TYPE_ENUM,
				BTK_CTREE_EXPANSION_TOGGLE_RECURSIVE);  
}

static void
btk_ctree_set_arg (BtkObject      *object,
		   BtkArg         *arg,
		   buint           arg_id)
{
  BtkCTree *ctree;
  BtkCList *clist;

  ctree = BTK_CTREE (object);
  clist = BTK_CLIST (ctree);

  switch (arg_id)
    {
    case ARG_N_COLUMNS: /* construct-only arg, only set at construction time */
      clist->columns = MAX (1, BTK_VALUE_UINT (*arg));
      ctree->tree_column = CLAMP (ctree->tree_column, 0, clist->columns);
      break;
    case ARG_TREE_COLUMN: /* construct-only arg, only set at construction time */
      ctree->tree_column = BTK_VALUE_UINT (*arg);
      ctree->tree_column = CLAMP (ctree->tree_column, 0, clist->columns);
      break;
    case ARG_INDENT:
      btk_ctree_set_indent (ctree, BTK_VALUE_UINT (*arg));
      break;
    case ARG_SPACING:
      btk_ctree_set_spacing (ctree, BTK_VALUE_UINT (*arg));
      break;
    case ARG_SHOW_STUB:
      btk_ctree_set_show_stub (ctree, BTK_VALUE_BOOL (*arg));
      break;
    case ARG_LINE_STYLE:
      btk_ctree_set_line_style (ctree, BTK_VALUE_ENUM (*arg));
      break;
    case ARG_EXPANDER_STYLE:
      btk_ctree_set_expander_style (ctree, BTK_VALUE_ENUM (*arg));
      break;
    default:
      break;
    }
}

static void
btk_ctree_get_arg (BtkObject      *object,
		   BtkArg         *arg,
		   buint           arg_id)
{
  BtkCTree *ctree;

  ctree = BTK_CTREE (object);

  switch (arg_id)
    {
    case ARG_N_COLUMNS:
      BTK_VALUE_UINT (*arg) = BTK_CLIST (ctree)->columns;
      break;
    case ARG_TREE_COLUMN:
      BTK_VALUE_UINT (*arg) = ctree->tree_column;
      break;
    case ARG_INDENT:
      BTK_VALUE_UINT (*arg) = ctree->tree_indent;
      break;
    case ARG_SPACING:
      BTK_VALUE_UINT (*arg) = ctree->tree_spacing;
      break;
    case ARG_SHOW_STUB:
      BTK_VALUE_BOOL (*arg) = ctree->show_stub;
      break;
    case ARG_LINE_STYLE:
      BTK_VALUE_ENUM (*arg) = ctree->line_style;
      break;
    case ARG_EXPANDER_STYLE:
      BTK_VALUE_ENUM (*arg) = ctree->expander_style;
      break;
    default:
      arg->type = BTK_TYPE_INVALID;
      break;
    }
}

static void
btk_ctree_init (BtkCTree *ctree)
{
  BtkCList *clist;

  BTK_CLIST_SET_FLAG (ctree, CLIST_DRAW_DRAG_RECT);
  BTK_CLIST_SET_FLAG (ctree, CLIST_DRAW_DRAG_LINE);

  clist = BTK_CLIST (ctree);

  ctree->tree_indent    = 20;
  ctree->tree_spacing   = 5;
  ctree->tree_column    = 0;
  ctree->line_style     = BTK_CTREE_LINES_SOLID;
  ctree->expander_style = BTK_CTREE_EXPANDER_SQUARE;
  ctree->drag_compare   = NULL;
  ctree->show_stub      = TRUE;

  clist->button_actions[0] |= BTK_BUTTON_EXPANDS;
}

static void
ctree_attach_styles (BtkCTree     *ctree,
		     BtkCTreeNode *node,
		     bpointer      data)
{
  BtkCList *clist;
  bint i;

  clist = BTK_CLIST (ctree);

  if (BTK_CTREE_ROW (node)->row.style)
    BTK_CTREE_ROW (node)->row.style =
      btk_style_attach (BTK_CTREE_ROW (node)->row.style, clist->clist_window);

  if (BTK_CTREE_ROW (node)->row.fg_set || BTK_CTREE_ROW (node)->row.bg_set)
    {
      BdkColormap *colormap;

      colormap = btk_widget_get_colormap (BTK_WIDGET (ctree));
      if (BTK_CTREE_ROW (node)->row.fg_set)
	bdk_colormap_alloc_color (colormap,
                                  &(BTK_CTREE_ROW (node)->row.foreground),
                                  FALSE, TRUE);
      if (BTK_CTREE_ROW (node)->row.bg_set)
	bdk_colormap_alloc_color (colormap,
                                  &(BTK_CTREE_ROW (node)->row.background),
                                  FALSE, TRUE);
    }

  for (i = 0; i < clist->columns; i++)
    if  (BTK_CTREE_ROW (node)->row.cell[i].style)
      BTK_CTREE_ROW (node)->row.cell[i].style =
	btk_style_attach (BTK_CTREE_ROW (node)->row.cell[i].style,
			  clist->clist_window);
}

static void
ctree_detach_styles (BtkCTree     *ctree,
		     BtkCTreeNode *node,
		     bpointer      data)
{
  BtkCList *clist;
  bint i;

  clist = BTK_CLIST (ctree);

  if (BTK_CTREE_ROW (node)->row.style)
    btk_style_detach (BTK_CTREE_ROW (node)->row.style);
  for (i = 0; i < clist->columns; i++)
    if  (BTK_CTREE_ROW (node)->row.cell[i].style)
      btk_style_detach (BTK_CTREE_ROW (node)->row.cell[i].style);
}

static void
btk_ctree_realize (BtkWidget *widget)
{
  BtkCTree *ctree;
  BtkCList *clist;
  BdkGCValues values;
  BtkCTreeNode *node;
  BtkCTreeNode *child;
  bint i;

  g_return_if_fail (BTK_IS_CTREE (widget));

  BTK_WIDGET_CLASS (parent_class)->realize (widget);

  ctree = BTK_CTREE (widget);
  clist = BTK_CLIST (widget);

  node = BTK_CTREE_NODE (clist->row_list);
  for (i = 0; i < clist->rows; i++)
    {
      if (BTK_CTREE_ROW (node)->children && !BTK_CTREE_ROW (node)->expanded)
	for (child = BTK_CTREE_ROW (node)->children; child;
	     child = BTK_CTREE_ROW (child)->sibling)
	  btk_ctree_pre_recursive (ctree, child, ctree_attach_styles, NULL);
      node = BTK_CTREE_NODE_NEXT (node);
    }

  values.foreground = widget->style->fg[BTK_STATE_NORMAL];
  values.background = widget->style->base[BTK_STATE_NORMAL];
  values.subwindow_mode = BDK_INCLUDE_INFERIORS;
  values.line_style = BDK_LINE_SOLID;
  ctree->lines_gc = bdk_gc_new_with_values (BTK_CLIST(widget)->clist_window, 
					    &values,
					    BDK_GC_FOREGROUND |
					    BDK_GC_BACKGROUND |
					    BDK_GC_SUBWINDOW |
					    BDK_GC_LINE_STYLE);

  if (ctree->line_style == BTK_CTREE_LINES_DOTTED)
    {
      bint8 dashes[] = { 1, 1 };

      bdk_gc_set_line_attributes (ctree->lines_gc, 1, 
				  BDK_LINE_ON_OFF_DASH, BDK_CAP_BUTT, BDK_JOIN_MITER);
      bdk_gc_set_dashes (ctree->lines_gc, 0, dashes, G_N_ELEMENTS (dashes));
    }
}

static void
btk_ctree_unrealize (BtkWidget *widget)
{
  BtkCTree *ctree;
  BtkCList *clist;

  g_return_if_fail (BTK_IS_CTREE (widget));

  BTK_WIDGET_CLASS (parent_class)->unrealize (widget);

  ctree = BTK_CTREE (widget);
  clist = BTK_CLIST (widget);

  if (btk_widget_get_realized (widget))
    {
      BtkCTreeNode *node;
      BtkCTreeNode *child;
      bint i;

      node = BTK_CTREE_NODE (clist->row_list);
      for (i = 0; i < clist->rows; i++)
	{
	  if (BTK_CTREE_ROW (node)->children &&
	      !BTK_CTREE_ROW (node)->expanded)
	    for (child = BTK_CTREE_ROW (node)->children; child;
		 child = BTK_CTREE_ROW (child)->sibling)
	      btk_ctree_pre_recursive(ctree, child, ctree_detach_styles, NULL);
	  node = BTK_CTREE_NODE_NEXT (node);
	}
    }

  g_object_unref (ctree->lines_gc);
}

static bint
btk_ctree_button_press (BtkWidget      *widget,
			BdkEventButton *event)
{
  BtkCTree *ctree;
  BtkCList *clist;
  bint button_actions;

  g_return_val_if_fail (BTK_IS_CTREE (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  ctree = BTK_CTREE (widget);
  clist = BTK_CLIST (widget);

  button_actions = clist->button_actions[event->button - 1];

  if (button_actions == BTK_BUTTON_IGNORED)
    return FALSE;

  if (event->window == clist->clist_window)
    {
      BtkCTreeNode *work;
      bint x;
      bint y;
      bint row;
      bint column;

      x = event->x;
      y = event->y;

      if (!btk_clist_get_selection_info (clist, x, y, &row, &column))
	return FALSE;

      work = BTK_CTREE_NODE (g_list_nth (clist->row_list, row));
	  
      if (button_actions & BTK_BUTTON_EXPANDS &&
	  (BTK_CTREE_ROW (work)->children && !BTK_CTREE_ROW (work)->is_leaf  &&
	   (event->type == BDK_2BUTTON_PRESS ||
	    ctree_is_hot_spot (ctree, work, row, x, y))))
	{
	  if (BTK_CTREE_ROW (work)->expanded)
	    btk_ctree_collapse (ctree, work);
	  else
	    btk_ctree_expand (ctree, work);

	  return TRUE;
	}
    }
  
  return BTK_WIDGET_CLASS (parent_class)->button_press_event (widget, event);
}

static void
draw_drag_highlight (BtkCList        *clist,
		     BtkCListRow     *dest_row,
		     bint             dest_row_number,
		     BtkCListDragPos  drag_pos)
{
  BtkCTree *ctree;
  BdkPoint points[4];
  bint level;
  bint i;
  bint y = 0;

  g_return_if_fail (BTK_IS_CTREE (clist));

  ctree = BTK_CTREE (clist);

  level = ((BtkCTreeRow *)(dest_row))->level;

  y = ROW_TOP_YPIXEL (clist, dest_row_number) - 1;

  switch (drag_pos)
    {
    case BTK_CLIST_DRAG_NONE:
      break;
    case BTK_CLIST_DRAG_AFTER:
      y += clist->row_height + 1;
    case BTK_CLIST_DRAG_BEFORE:
      
      if (clist->column[ctree->tree_column].visible)
	switch (clist->column[ctree->tree_column].justification)
	  {
	  case BTK_JUSTIFY_CENTER:
	  case BTK_JUSTIFY_FILL:
	  case BTK_JUSTIFY_LEFT:
	    if (ctree->tree_column > 0)
	      bdk_draw_line (clist->clist_window, clist->xor_gc, 
			     COLUMN_LEFT_XPIXEL(clist, 0), y,
			     COLUMN_LEFT_XPIXEL(clist, ctree->tree_column - 1)+
			     clist->column[ctree->tree_column - 1].area.width,
			     y);

	    bdk_draw_line (clist->clist_window, clist->xor_gc, 
			   COLUMN_LEFT_XPIXEL(clist, ctree->tree_column) + 
			   ctree->tree_indent * level -
			   (ctree->tree_indent - PM_SIZE) / 2, y,
			   BTK_WIDGET (ctree)->allocation.width, y);
	    break;
	  case BTK_JUSTIFY_RIGHT:
	    if (ctree->tree_column < clist->columns - 1)
	      bdk_draw_line (clist->clist_window, clist->xor_gc, 
			     COLUMN_LEFT_XPIXEL(clist, ctree->tree_column + 1),
			     y,
			     COLUMN_LEFT_XPIXEL(clist, clist->columns - 1) +
			     clist->column[clist->columns - 1].area.width, y);
      
	    bdk_draw_line (clist->clist_window, clist->xor_gc, 
			   0, y, COLUMN_LEFT_XPIXEL(clist, ctree->tree_column)
			   + clist->column[ctree->tree_column].area.width -
			   ctree->tree_indent * level +
			   (ctree->tree_indent - PM_SIZE) / 2, y);
	    break;
	  }
      else
	bdk_draw_line (clist->clist_window, clist->xor_gc, 
		       0, y, clist->clist_window_width, y);
      break;
    case BTK_CLIST_DRAG_INTO:
      y = ROW_TOP_YPIXEL (clist, dest_row_number) + clist->row_height;

      if (clist->column[ctree->tree_column].visible)
	switch (clist->column[ctree->tree_column].justification)
	  {
	  case BTK_JUSTIFY_CENTER:
	  case BTK_JUSTIFY_FILL:
	  case BTK_JUSTIFY_LEFT:
	    points[0].x =  COLUMN_LEFT_XPIXEL(clist, ctree->tree_column) + 
	      ctree->tree_indent * level - (ctree->tree_indent - PM_SIZE) / 2;
	    points[0].y = y;
	    points[3].x = points[0].x;
	    points[3].y = y - clist->row_height - 1;
	    points[1].x = clist->clist_window_width - 1;
	    points[1].y = points[0].y;
	    points[2].x = points[1].x;
	    points[2].y = points[3].y;

	    for (i = 0; i < 3; i++)
	      bdk_draw_line (clist->clist_window, clist->xor_gc,
			     points[i].x, points[i].y,
			     points[i+1].x, points[i+1].y);

	    if (ctree->tree_column > 0)
	      {
		points[0].x = COLUMN_LEFT_XPIXEL(clist,
						 ctree->tree_column - 1) +
		  clist->column[ctree->tree_column - 1].area.width ;
		points[0].y = y;
		points[3].x = points[0].x;
		points[3].y = y - clist->row_height - 1;
		points[1].x = 0;
		points[1].y = points[0].y;
		points[2].x = 0;
		points[2].y = points[3].y;

		for (i = 0; i < 3; i++)
		  bdk_draw_line (clist->clist_window, clist->xor_gc,
				 points[i].x, points[i].y, points[i+1].x, 
				 points[i+1].y);
	      }
	    break;
	  case BTK_JUSTIFY_RIGHT:
	    points[0].x =  COLUMN_LEFT_XPIXEL(clist, ctree->tree_column) - 
	      ctree->tree_indent * level + (ctree->tree_indent - PM_SIZE) / 2 +
	      clist->column[ctree->tree_column].area.width;
	    points[0].y = y;
	    points[3].x = points[0].x;
	    points[3].y = y - clist->row_height - 1;
	    points[1].x = 0;
	    points[1].y = points[0].y;
	    points[2].x = 0;
	    points[2].y = points[3].y;

	    for (i = 0; i < 3; i++)
	      bdk_draw_line (clist->clist_window, clist->xor_gc,
			     points[i].x, points[i].y,
			     points[i+1].x, points[i+1].y);

	    if (ctree->tree_column < clist->columns - 1)
	      {
		points[0].x = COLUMN_LEFT_XPIXEL(clist, ctree->tree_column +1);
		points[0].y = y;
		points[3].x = points[0].x;
		points[3].y = y - clist->row_height - 1;
		points[1].x = clist->clist_window_width - 1;
		points[1].y = points[0].y;
		points[2].x = points[1].x;
		points[2].y = points[3].y;

		for (i = 0; i < 3; i++)
		  bdk_draw_line (clist->clist_window, clist->xor_gc,
				 points[i].x, points[i].y,
				 points[i+1].x, points[i+1].y);
	      }
	    break;
	  }
      else
	bdk_draw_rectangle (clist->clist_window, clist->xor_gc, FALSE,
			    0, y - clist->row_height,
			    clist->clist_window_width - 1, clist->row_height);
      break;
    }
}

static bint
draw_cell_pixmap (BdkWindow    *window,
		  BdkRectangle *clip_rectangle,
		  BdkGC        *fg_gc,
		  BdkPixmap    *pixmap,
		  BdkBitmap    *mask,
		  bint          x,
		  bint          y,
		  bint          width,
		  bint          height)
{
  bint xsrc = 0;
  bint ysrc = 0;

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

  if (width > 0 && height > 0)
    bdk_draw_drawable (window, fg_gc, pixmap, xsrc, ysrc, x, y, width, height);

  if (mask)
    {
      bdk_gc_set_clip_rectangle (fg_gc, NULL);
      bdk_gc_set_clip_origin (fg_gc, 0, 0);
    }

  return x + MAX (width, 0);
}

static void
get_cell_style (BtkCList     *clist,
		BtkCListRow  *clist_row,
		bint          state,
		bint          column,
		BtkStyle    **style,
		BdkGC       **fg_gc,
		BdkGC       **bg_gc)
{
  bint fg_state;

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

static bint
btk_ctree_draw_expander (BtkCTree     *ctree,
			 BtkCTreeRow  *ctree_row,
			 BtkStyle     *style,
			 BdkRectangle *clip_rectangle,
			 bint          x)
{
  BtkCList *clist;
  BdkPoint points[3];
  bint justification_factor;
  bint y;

 if (ctree->expander_style == BTK_CTREE_EXPANDER_NONE)
   return x;

  clist = BTK_CLIST (ctree);
  if (clist->column[ctree->tree_column].justification == BTK_JUSTIFY_RIGHT)
    justification_factor = -1;
  else
    justification_factor = 1;
  y = (clip_rectangle->y + (clip_rectangle->height - PM_SIZE) / 2 -
       (clip_rectangle->height + 1) % 2);

  if (!ctree_row->children)
    {
      switch (ctree->expander_style)
	{
	case BTK_CTREE_EXPANDER_NONE:
	  return x;
	case BTK_CTREE_EXPANDER_TRIANGLE:
	  return x + justification_factor * (PM_SIZE + 3);
	case BTK_CTREE_EXPANDER_SQUARE:
	case BTK_CTREE_EXPANDER_CIRCULAR:
	  return x + justification_factor * (PM_SIZE + 1);
	}
    }

  bdk_gc_set_clip_rectangle (style->fg_gc[BTK_STATE_NORMAL], clip_rectangle);
  bdk_gc_set_clip_rectangle (style->base_gc[BTK_STATE_NORMAL], clip_rectangle);

  switch (ctree->expander_style)
    {
    case BTK_CTREE_EXPANDER_NONE:
      break;
    case BTK_CTREE_EXPANDER_TRIANGLE:
      if (ctree_row->expanded)
	{
	  points[0].x = x;
	  points[0].y = y + (PM_SIZE + 2) / 6;
	  points[1].x = points[0].x + justification_factor * (PM_SIZE + 2);
	  points[1].y = points[0].y;
	  points[2].x = (points[0].x +
			 justification_factor * (PM_SIZE + 2) / 2);
	  points[2].y = y + 2 * (PM_SIZE + 2) / 3;
	}
      else
	{
	  points[0].x = x + justification_factor * ((PM_SIZE + 2) / 6 + 2);
	  points[0].y = y - 1;
	  points[1].x = points[0].x;
	  points[1].y = points[0].y + (PM_SIZE + 2);
	  points[2].x = (points[0].x +
			 justification_factor * (2 * (PM_SIZE + 2) / 3 - 1));
	  points[2].y = points[0].y + (PM_SIZE + 2) / 2;
	}

      bdk_draw_polygon (clist->clist_window, style->base_gc[BTK_STATE_NORMAL],
			TRUE, points, 3);
      bdk_draw_polygon (clist->clist_window, style->fg_gc[BTK_STATE_NORMAL],
			FALSE, points, 3);

      x += justification_factor * (PM_SIZE + 3);
      break;
    case BTK_CTREE_EXPANDER_SQUARE:
    case BTK_CTREE_EXPANDER_CIRCULAR:
      if (justification_factor == -1)
	x += justification_factor * (PM_SIZE + 1);

      if (ctree->expander_style == BTK_CTREE_EXPANDER_CIRCULAR)
	{
	  bdk_draw_arc (clist->clist_window, style->base_gc[BTK_STATE_NORMAL],
			TRUE, x, y, PM_SIZE, PM_SIZE, 0, 360 * 64);
	  bdk_draw_arc (clist->clist_window, style->fg_gc[BTK_STATE_NORMAL],
			FALSE, x, y, PM_SIZE, PM_SIZE, 0, 360 * 64);
	}
      else
	{
	  bdk_draw_rectangle (clist->clist_window,
			      style->base_gc[BTK_STATE_NORMAL], TRUE,
			      x, y, PM_SIZE, PM_SIZE);
	  bdk_draw_rectangle (clist->clist_window,
			      style->fg_gc[BTK_STATE_NORMAL], FALSE,
			      x, y, PM_SIZE, PM_SIZE);
	}

      bdk_draw_line (clist->clist_window, style->fg_gc[BTK_STATE_NORMAL], 
		     x + 2, y + PM_SIZE / 2, x + PM_SIZE - 2, y + PM_SIZE / 2);

      if (!ctree_row->expanded)
	bdk_draw_line (clist->clist_window, style->fg_gc[BTK_STATE_NORMAL],
		       x + PM_SIZE / 2, y + 2,
		       x + PM_SIZE / 2, y + PM_SIZE - 2);

      if (justification_factor == 1)
	x += justification_factor * (PM_SIZE + 1);
      break;
    }

  bdk_gc_set_clip_rectangle (style->fg_gc[BTK_STATE_NORMAL], NULL);
  bdk_gc_set_clip_rectangle (style->base_gc[BTK_STATE_NORMAL], NULL);

  return x;
}


static bint
btk_ctree_draw_lines (BtkCTree     *ctree,
		      BtkCTreeRow  *ctree_row,
		      bint          row,
		      bint          column,
		      bint          state,
		      BdkRectangle *clip_rectangle,
		      BdkRectangle *cell_rectangle,
		      BdkRectangle *crect,
		      BdkRectangle *area,
		      BtkStyle     *style)
{
  BtkCList *clist;
  BtkCTreeNode *node;
  BtkCTreeNode *parent;
  BdkRectangle tree_rectangle;
  BdkRectangle tc_rectangle;
  BdkGC *bg_gc;
  bint offset;
  bint offset_x;
  bint offset_y;
  bint xcenter;
  bint ycenter;
  bint next_level;
  bint column_right;
  bint column_left;
  bint justify_right;
  bint justification_factor;
  
  clist = BTK_CLIST (ctree);
  ycenter = clip_rectangle->y + (clip_rectangle->height / 2);
  justify_right = (clist->column[column].justification == BTK_JUSTIFY_RIGHT);

  if (justify_right)
    {
      offset = (clip_rectangle->x + clip_rectangle->width - 1 -
		ctree->tree_indent * (ctree_row->level - 1));
      justification_factor = -1;
    }
  else
    {
      offset = clip_rectangle->x + ctree->tree_indent * (ctree_row->level - 1);
      justification_factor = 1;
    }

  switch (ctree->line_style)
    {
    case BTK_CTREE_LINES_NONE:
      break;
    case BTK_CTREE_LINES_TABBED:
      xcenter = offset + justification_factor * TAB_SIZE;

      column_right = (COLUMN_LEFT_XPIXEL (clist, ctree->tree_column) +
		      clist->column[ctree->tree_column].area.width +
		      COLUMN_INSET);
      column_left = (COLUMN_LEFT_XPIXEL (clist, ctree->tree_column) -
		     COLUMN_INSET - CELL_SPACING);

      if (area)
	{
	  tree_rectangle.y = crect->y;
	  tree_rectangle.height = crect->height;

	  if (justify_right)
	    {
	      tree_rectangle.x = xcenter;
	      tree_rectangle.width = column_right - xcenter;
	    }
	  else
	    {
	      tree_rectangle.x = column_left;
	      tree_rectangle.width = xcenter - column_left;
	    }

	  if (!bdk_rectangle_intersect (area, &tree_rectangle, &tc_rectangle))
	    {
	      offset += justification_factor * 3;
	      break;
	    }
	}

      bdk_gc_set_clip_rectangle (ctree->lines_gc, crect);

      next_level = ctree_row->level;

      if (!ctree_row->sibling || (ctree_row->children && ctree_row->expanded))
	{
	  node = btk_ctree_find_node_ptr (ctree, ctree_row);
	  if (BTK_CTREE_NODE_NEXT (node))
	    next_level = BTK_CTREE_ROW (BTK_CTREE_NODE_NEXT (node))->level;
	  else
	    next_level = 0;
	}

      if (ctree->tree_indent > 0)
	{
	  node = ctree_row->parent;
	  while (node)
	    {
	      xcenter -= (justification_factor * ctree->tree_indent);

	      if ((justify_right && xcenter < column_left) ||
		  (!justify_right && xcenter > column_right))
		{
		  node = BTK_CTREE_ROW (node)->parent;
		  continue;
		}

	      tree_rectangle.y = cell_rectangle->y;
	      tree_rectangle.height = cell_rectangle->height;
	      if (justify_right)
		{
		  tree_rectangle.x = MAX (xcenter - ctree->tree_indent + 1,
					  column_left);
		  tree_rectangle.width = MIN (xcenter - column_left,
					      ctree->tree_indent);
		}
	      else
		{
		  tree_rectangle.x = xcenter;
		  tree_rectangle.width = MIN (column_right - xcenter,
					      ctree->tree_indent);
		}

	      if (!area || bdk_rectangle_intersect (area, &tree_rectangle,
						    &tc_rectangle))
		{
		  get_cell_style (clist, &BTK_CTREE_ROW (node)->row,
				  state, column, NULL, NULL, &bg_gc);

		  if (bg_gc == clist->bg_gc)
		    bdk_gc_set_foreground
		      (clist->bg_gc, &BTK_CTREE_ROW (node)->row.background);

		  if (!area)
		    bdk_draw_rectangle (clist->clist_window, bg_gc, TRUE,
					tree_rectangle.x,
					tree_rectangle.y,
					tree_rectangle.width,
					tree_rectangle.height);
		  else 
		    bdk_draw_rectangle (clist->clist_window, bg_gc, TRUE,
					tc_rectangle.x,
					tc_rectangle.y,
					tc_rectangle.width,
					tc_rectangle.height);
		}
	      if (next_level > BTK_CTREE_ROW (node)->level)
		bdk_draw_line (clist->clist_window, ctree->lines_gc,
			       xcenter, crect->y,
			       xcenter, crect->y + crect->height);
	      else
		{
		  bint width;

		  offset_x = MIN (ctree->tree_indent, 2 * TAB_SIZE);
		  width = offset_x / 2 + offset_x % 2;

		  parent = BTK_CTREE_ROW (node)->parent;

		  tree_rectangle.y = ycenter;
		  tree_rectangle.height = (cell_rectangle->y - ycenter +
					   cell_rectangle->height);

		  if (justify_right)
		    {
		      tree_rectangle.x = MAX(xcenter + 1 - width, column_left);
		      tree_rectangle.width = MIN (xcenter + 1 - column_left,
						  width);
		    }
		  else
		    {
		      tree_rectangle.x = xcenter;
		      tree_rectangle.width = MIN (column_right - xcenter,
						  width);
		    }

		  if (!area ||
		      bdk_rectangle_intersect (area, &tree_rectangle,
					       &tc_rectangle))
		    {
		      if (parent)
			{
			  get_cell_style (clist, &BTK_CTREE_ROW (parent)->row,
					  state, column, NULL, NULL, &bg_gc);
			  if (bg_gc == clist->bg_gc)
			    bdk_gc_set_foreground
			      (clist->bg_gc,
			       &BTK_CTREE_ROW (parent)->row.background);
			}
		      else if (state == BTK_STATE_SELECTED)
			bg_gc = style->base_gc[state];
		      else
			bg_gc = BTK_WIDGET (clist)->style->base_gc[state];

		      if (!area)
			bdk_draw_rectangle (clist->clist_window, bg_gc, TRUE,
					    tree_rectangle.x,
					    tree_rectangle.y,
					    tree_rectangle.width,
					    tree_rectangle.height);
		      else
			bdk_draw_rectangle (clist->clist_window,
					    bg_gc, TRUE,
					    tc_rectangle.x,
					    tc_rectangle.y,
					    tc_rectangle.width,
					    tc_rectangle.height);
		    }

		  get_cell_style (clist, &BTK_CTREE_ROW (node)->row,
				  state, column, NULL, NULL, &bg_gc);
		  if (bg_gc == clist->bg_gc)
		    bdk_gc_set_foreground
		      (clist->bg_gc, &BTK_CTREE_ROW (node)->row.background);

		  bdk_gc_set_clip_rectangle (bg_gc, crect);
		  bdk_draw_arc (clist->clist_window, bg_gc, TRUE,
				xcenter - (justify_right * offset_x),
				cell_rectangle->y,
				offset_x, clist->row_height,
				(180 + (justify_right * 90)) * 64, 90 * 64);
		  bdk_gc_set_clip_rectangle (bg_gc, NULL);

		  bdk_draw_line (clist->clist_window, ctree->lines_gc, 
				 xcenter, cell_rectangle->y, xcenter, ycenter);

		  if (justify_right)
		    bdk_draw_arc (clist->clist_window, ctree->lines_gc, FALSE,
				  xcenter - offset_x, cell_rectangle->y,
				  offset_x, clist->row_height,
				  270 * 64, 90 * 64);
		  else
		    bdk_draw_arc (clist->clist_window, ctree->lines_gc, FALSE,
				  xcenter, cell_rectangle->y,
				  offset_x, clist->row_height,
				  180 * 64, 90 * 64);
		}
	      node = BTK_CTREE_ROW (node)->parent;
	    }
	}

      if (state != BTK_STATE_SELECTED)
	{
	  tree_rectangle.y = clip_rectangle->y;
	  tree_rectangle.height = clip_rectangle->height;
	  tree_rectangle.width = COLUMN_INSET + CELL_SPACING +
	    MIN (clist->column[ctree->tree_column].area.width + COLUMN_INSET,
		 TAB_SIZE);

	  if (justify_right)
	    tree_rectangle.x = MAX (xcenter + 1, column_left);
	  else
	    tree_rectangle.x = column_left;

	  if (!area)
	    bdk_draw_rectangle (clist->clist_window,
				BTK_WIDGET
				(ctree)->style->base_gc[BTK_STATE_NORMAL],
				TRUE,
				tree_rectangle.x,
				tree_rectangle.y,
				tree_rectangle.width,
				tree_rectangle.height);
	  else if (bdk_rectangle_intersect (area, &tree_rectangle,
					    &tc_rectangle))
	    bdk_draw_rectangle (clist->clist_window,
				BTK_WIDGET
				(ctree)->style->base_gc[BTK_STATE_NORMAL],
				TRUE,
				tc_rectangle.x,
				tc_rectangle.y,
				tc_rectangle.width,
				tc_rectangle.height);
	}

      xcenter = offset + (justification_factor * ctree->tree_indent / 2);

      get_cell_style (clist, &ctree_row->row, state, column, NULL, NULL,
		      &bg_gc);
      if (bg_gc == clist->bg_gc)
	bdk_gc_set_foreground (clist->bg_gc, &ctree_row->row.background);

      bdk_gc_set_clip_rectangle (bg_gc, crect);
      if (ctree_row->is_leaf)
	{
	  BdkPoint points[6];

	  points[0].x = offset + justification_factor * TAB_SIZE;
	  points[0].y = cell_rectangle->y;

	  points[1].x = points[0].x - justification_factor * 4;
	  points[1].y = points[0].y;

	  points[2].x = points[1].x - justification_factor * 2;
	  points[2].y = points[1].y + 3;

	  points[3].x = points[2].x;
	  points[3].y = points[2].y + clist->row_height - 5;

	  points[4].x = points[3].x + justification_factor * 2;
	  points[4].y = points[3].y + 3;

	  points[5].x = points[4].x + justification_factor * 4;
	  points[5].y = points[4].y;

	  bdk_draw_polygon (clist->clist_window, bg_gc, TRUE, points, 6);
	  bdk_draw_lines (clist->clist_window, ctree->lines_gc, points, 6);
	}
      else 
	{
	  bdk_draw_arc (clist->clist_window, bg_gc, TRUE,
			offset - (justify_right * 2 * TAB_SIZE),
			cell_rectangle->y,
			2 * TAB_SIZE, clist->row_height,
			(90 + (180 * justify_right)) * 64, 180 * 64);
	  bdk_draw_arc (clist->clist_window, ctree->lines_gc, FALSE,
			offset - (justify_right * 2 * TAB_SIZE),
			cell_rectangle->y,
			2 * TAB_SIZE, clist->row_height,
			(90 + (180 * justify_right)) * 64, 180 * 64);
	}
      bdk_gc_set_clip_rectangle (bg_gc, NULL);
      bdk_gc_set_clip_rectangle (ctree->lines_gc, NULL);

      offset += justification_factor * 3;
      break;
    default:
      xcenter = offset + justification_factor * PM_SIZE / 2;

      if (area)
	{
	  tree_rectangle.y = crect->y;
	  tree_rectangle.height = crect->height;

	  if (justify_right)
	    {
	      tree_rectangle.x = xcenter - PM_SIZE / 2 - 2;
	      tree_rectangle.width = (clip_rectangle->x +
				      clip_rectangle->width -tree_rectangle.x);
	    }
	  else
	    {
	      tree_rectangle.x = clip_rectangle->x + PM_SIZE / 2;
	      tree_rectangle.width = (xcenter + PM_SIZE / 2 + 2 -
				      clip_rectangle->x);
	    }

	  if (!bdk_rectangle_intersect (area, &tree_rectangle, &tc_rectangle))
	    break;
	}

      offset_x = 1;
      offset_y = 0;
      if (ctree->line_style == BTK_CTREE_LINES_DOTTED)
	{
	  offset_x += abs((clip_rectangle->x + clist->hoffset) % 2);
	  offset_y  = abs((cell_rectangle->y + clist->voffset) % 2);
	}

      clip_rectangle->y--;
      clip_rectangle->height++;
      bdk_gc_set_clip_rectangle (ctree->lines_gc, clip_rectangle);
      bdk_draw_line (clist->clist_window, ctree->lines_gc,
		     xcenter,
		     (ctree->show_stub || clist->row_list->data != ctree_row) ?
		     cell_rectangle->y + offset_y : ycenter,
		     xcenter,
		     (ctree_row->sibling) ? crect->y +crect->height : ycenter);

      bdk_draw_line (clist->clist_window, ctree->lines_gc,
		     xcenter + (justification_factor * offset_x), ycenter,
		     xcenter + (justification_factor * (PM_SIZE / 2 + 2)),
		     ycenter);

      node = ctree_row->parent;
      while (node)
	{
	  xcenter -= (justification_factor * ctree->tree_indent);

	  if (BTK_CTREE_ROW (node)->sibling)
	    bdk_draw_line (clist->clist_window, ctree->lines_gc, 
			   xcenter, cell_rectangle->y + offset_y,
			   xcenter, crect->y + crect->height);
	  node = BTK_CTREE_ROW (node)->parent;
	}
      bdk_gc_set_clip_rectangle (ctree->lines_gc, NULL);
      clip_rectangle->y++;
      clip_rectangle->height--;
      break;
    }
  return offset;
}

static void
draw_row (BtkCList     *clist,
	  BdkRectangle *area,
	  bint          row,
	  BtkCListRow  *clist_row)
{
  BtkWidget *widget;
  BtkCTree  *ctree;
  BdkRectangle *crect;
  BdkRectangle row_rectangle;
  BdkRectangle cell_rectangle; 
  BdkRectangle clip_rectangle;
  BdkRectangle intersect_rectangle;
  bint last_column;
  bint column_left = 0;
  bint column_right = 0;
  bint offset = 0;
  bint state;
  bint i;

  g_return_if_fail (clist != NULL);

  /* bail now if we arn't drawable yet */
  if (!BTK_WIDGET_DRAWABLE (clist) || row < 0 || row >= clist->rows)
    return;

  widget = BTK_WIDGET (clist);
  ctree  = BTK_CTREE  (clist);

  /* if the function is passed the pointer to the row instead of null,
   * it avoids this expensive lookup */
  if (!clist_row)
    clist_row = (g_list_nth (clist->row_list, row))->data;

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

  bdk_gc_set_foreground (ctree->lines_gc,
			 &widget->style->fg[clist_row->state]);

  /* draw the cell borders */
  if (area)
    {
      crect = &intersect_rectangle;

      if (bdk_rectangle_intersect (area, &cell_rectangle, crect))
	bdk_draw_rectangle (clist->clist_window,
			    widget->style->base_gc[BTK_STATE_NORMAL], TRUE,
			    crect->x, crect->y, crect->width, crect->height);
    }
  else
    {
      crect = &cell_rectangle;

      bdk_draw_rectangle (clist->clist_window,
			  widget->style->base_gc[BTK_STATE_NORMAL], TRUE,
			  crect->x, crect->y, crect->width, crect->height);
    }

  /* horizontal black lines */
  if (ctree->line_style == BTK_CTREE_LINES_TABBED)
    { 

      column_right = (COLUMN_LEFT_XPIXEL (clist, ctree->tree_column) +
		      clist->column[ctree->tree_column].area.width +
		      COLUMN_INSET);
      column_left = (COLUMN_LEFT_XPIXEL (clist, ctree->tree_column) -
		     COLUMN_INSET - (ctree->tree_column != 0) * CELL_SPACING);

      switch (clist->column[ctree->tree_column].justification)
	{
	case BTK_JUSTIFY_CENTER:
	case BTK_JUSTIFY_FILL:
	case BTK_JUSTIFY_LEFT:
	  offset = (column_left + ctree->tree_indent *
		    (((BtkCTreeRow *)clist_row)->level - 1));

	  bdk_draw_line (clist->clist_window, ctree->lines_gc, 
			 MIN (offset + TAB_SIZE, column_right),
			 cell_rectangle.y,
			 clist->clist_window_width, cell_rectangle.y);
	  break;
	case BTK_JUSTIFY_RIGHT:
	  offset = (column_right - 1 - ctree->tree_indent *
		    (((BtkCTreeRow *)clist_row)->level - 1));

	  bdk_draw_line (clist->clist_window, ctree->lines_gc,
			 -1, cell_rectangle.y,
			 MAX (offset - TAB_SIZE, column_left),
			 cell_rectangle.y);
	  break;
	}
    }

  /* the last row has to clear its bottom cell spacing too */
  if (clist_row == clist->row_list_end->data)
    {
      cell_rectangle.y += clist->row_height + CELL_SPACING;

      if (!area || bdk_rectangle_intersect (area, &cell_rectangle, crect))
	{
	  bdk_draw_rectangle (clist->clist_window,
			      widget->style->base_gc[BTK_STATE_NORMAL], TRUE,
			      crect->x, crect->y, crect->width, crect->height);

	  /* horizontal black lines */
	  if (ctree->line_style == BTK_CTREE_LINES_TABBED)
	    { 
	      switch (clist->column[ctree->tree_column].justification)
		{
		case BTK_JUSTIFY_CENTER:
		case BTK_JUSTIFY_FILL:
		case BTK_JUSTIFY_LEFT:
		  bdk_draw_line (clist->clist_window, ctree->lines_gc, 
				 MIN (column_left + TAB_SIZE + COLUMN_INSET +
				      (((BtkCTreeRow *)clist_row)->level > 1) *
				      MIN (ctree->tree_indent / 2, TAB_SIZE),
				      column_right),
				 cell_rectangle.y,
				 clist->clist_window_width, cell_rectangle.y);
		  break;
		case BTK_JUSTIFY_RIGHT:
		  bdk_draw_line (clist->clist_window, ctree->lines_gc, 
				 -1, cell_rectangle.y,
				 MAX (column_right - TAB_SIZE - 1 -
				      COLUMN_INSET -
				      (((BtkCTreeRow *)clist_row)->level > 1) *
				      MIN (ctree->tree_indent / 2, TAB_SIZE),
				      column_left - 1), cell_rectangle.y);
		  break;
		}
	    }
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
      BangoLayout *layout = NULL;
      BangoRectangle logical_rect;

      bint width;
      bint height;
      bint pixmap_width;
      bint string_width;
      bint old_offset;

      if (!clist->column[i].visible)
	continue;

      get_cell_style (clist, clist_row, state, i, &style, &fg_gc, &bg_gc);

      /* calculate clipping rebunnyion */
      clip_rectangle.x = clist->column[i].area.x + clist->hoffset;
      clip_rectangle.width = clist->column[i].area.width;

      cell_rectangle.x = clip_rectangle.x - COLUMN_INSET - CELL_SPACING;
      cell_rectangle.width = (clip_rectangle.width + 2 * COLUMN_INSET +
			      (1 + (i == last_column)) * CELL_SPACING);
      cell_rectangle.y = clip_rectangle.y;
      cell_rectangle.height = clip_rectangle.height;

      string_width = 0;
      pixmap_width = 0;

      if (area && !bdk_rectangle_intersect (area, &cell_rectangle,
					    &intersect_rectangle))
	{
	  if (i != ctree->tree_column)
	    continue;
	}
      else
	{
	  bdk_draw_rectangle (clist->clist_window, bg_gc, TRUE,
			      crect->x, crect->y, crect->width, crect->height);


	  layout = _btk_clist_create_cell_layout (clist, clist_row, i);
	  if (layout)
	    {
	      bango_layout_get_pixel_extents (layout, NULL, &logical_rect);
	      width = logical_rect.width;
	    }
	  else
	    width = 0;

	  switch (clist_row->cell[i].type)
	    {
	    case BTK_CELL_PIXMAP:
	      bdk_drawable_get_size
		(BTK_CELL_PIXMAP (clist_row->cell[i])->pixmap, &pixmap_width,
		 &height);
	      width += pixmap_width;
	      break;
	    case BTK_CELL_PIXTEXT:
	      if (BTK_CELL_PIXTEXT (clist_row->cell[i])->pixmap)
		{
		  bdk_drawable_get_size
		    (BTK_CELL_PIXTEXT (clist_row->cell[i])->pixmap,
		     &pixmap_width, &height);
		  width += pixmap_width;
		}

	      if (BTK_CELL_PIXTEXT (clist_row->cell[i])->text &&
		  BTK_CELL_PIXTEXT (clist_row->cell[i])->pixmap)
		width +=  BTK_CELL_PIXTEXT (clist_row->cell[i])->spacing;

	      if (i == ctree->tree_column)
		width += (ctree->tree_indent *
			  ((BtkCTreeRow *)clist_row)->level);
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

	  if (i != ctree->tree_column)
	    {
	      offset += clist_row->cell[i].horizontal;
	      switch (clist_row->cell[i].type)
		{
		case BTK_CELL_PIXMAP:
		  draw_cell_pixmap
		    (clist->clist_window, &clip_rectangle, fg_gc,
		     BTK_CELL_PIXMAP (clist_row->cell[i])->pixmap,
		     BTK_CELL_PIXMAP (clist_row->cell[i])->mask,
		     offset,
		     clip_rectangle.y + clist_row->cell[i].vertical +
		     (clip_rectangle.height - height) / 2,
		     pixmap_width, height);
		  break;
		case BTK_CELL_PIXTEXT:
		  offset = draw_cell_pixmap
		    (clist->clist_window, &clip_rectangle, fg_gc,
		     BTK_CELL_PIXTEXT (clist_row->cell[i])->pixmap,
		     BTK_CELL_PIXTEXT (clist_row->cell[i])->mask,
		     offset,
		     clip_rectangle.y + clist_row->cell[i].vertical +
		     (clip_rectangle.height - height) / 2,
		     pixmap_width, height);
		  offset += BTK_CELL_PIXTEXT (clist_row->cell[i])->spacing;

		  /* Fall through */
		case BTK_CELL_TEXT:
		  if (layout)
		    {
		      bint row_center_offset = (clist->row_height - logical_rect.height) / 2;

		      bdk_gc_set_clip_rectangle (fg_gc, &clip_rectangle);
		      bdk_draw_layout (clist->clist_window, fg_gc,
				       offset,
				       row_rectangle.y + row_center_offset + clist_row->cell[i].vertical,
				       layout);
		      bdk_gc_set_clip_rectangle (fg_gc, NULL);
		      g_object_unref (B_OBJECT (layout));
		    }
		  break;
		default:
		  break;
		}
	      continue;
	    }
	}

      if (bg_gc == clist->bg_gc)
	bdk_gc_set_background (ctree->lines_gc, &clist_row->background);

      /* draw ctree->tree_column */
      cell_rectangle.y -= CELL_SPACING;
      cell_rectangle.height += CELL_SPACING;

      if (area && !bdk_rectangle_intersect (area, &cell_rectangle,
					    &intersect_rectangle))
	{
	  if (layout)
            g_object_unref (B_OBJECT (layout));
	  continue;
	}

      /* draw lines */
      offset = btk_ctree_draw_lines (ctree, (BtkCTreeRow *)clist_row, row, i,
				     state, &clip_rectangle, &cell_rectangle,
				     crect, area, style);

      /* draw expander */
      offset = btk_ctree_draw_expander (ctree, (BtkCTreeRow *)clist_row,
					style, &clip_rectangle, offset);

      if (clist->column[i].justification == BTK_JUSTIFY_RIGHT)
	offset -= ctree->tree_spacing;
      else
	offset += ctree->tree_spacing;

      if (clist->column[i].justification == BTK_JUSTIFY_RIGHT)
	offset -= (pixmap_width + clist_row->cell[i].horizontal);
      else
	offset += clist_row->cell[i].horizontal;

      old_offset = offset;
      offset = draw_cell_pixmap (clist->clist_window, &clip_rectangle, fg_gc,
				 BTK_CELL_PIXTEXT (clist_row->cell[i])->pixmap,
				 BTK_CELL_PIXTEXT (clist_row->cell[i])->mask,
				 offset, 
				 clip_rectangle.y + clist_row->cell[i].vertical
				 + (clip_rectangle.height - height) / 2,
				 pixmap_width, height);

      if (layout)
	{
	  bint row_center_offset = (clist->row_height - logical_rect.height) / 2;
	  
	  if (clist->column[i].justification == BTK_JUSTIFY_RIGHT)
	    {
	      offset = (old_offset - string_width);
	      if (BTK_CELL_PIXTEXT (clist_row->cell[i])->pixmap)
		offset -= BTK_CELL_PIXTEXT (clist_row->cell[i])->spacing;
	    }
	  else
	    {
	      if (BTK_CELL_PIXTEXT (clist_row->cell[i])->pixmap)
		offset += BTK_CELL_PIXTEXT (clist_row->cell[i])->spacing;
	    }
	  
	  bdk_gc_set_clip_rectangle (fg_gc, &clip_rectangle);
	  bdk_draw_layout (clist->clist_window, fg_gc,
			   offset,
			   row_rectangle.y + row_center_offset + clist_row->cell[i].vertical,
			   layout);

          g_object_unref (B_OBJECT (layout));
	}
      bdk_gc_set_clip_rectangle (fg_gc, NULL);
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
tree_draw_node (BtkCTree     *ctree, 
	        BtkCTreeNode *node)
{
  BtkCList *clist;
  
  clist = BTK_CLIST (ctree);

  if (CLIST_UNFROZEN (clist) && btk_ctree_is_viewable (ctree, node))
    {
      BtkCTreeNode *work;
      bint num = 0;
      
      work = BTK_CTREE_NODE (clist->row_list);
      while (work && work != node)
	{
	  work = BTK_CTREE_NODE_NEXT (work);
	  num++;
	}
      if (work && btk_clist_row_is_visible (clist, num) != BTK_VISIBILITY_NONE)
	BTK_CLIST_GET_CLASS (clist)->draw_row
	  (clist, NULL, num, BTK_CLIST_ROW ((GList *) node));
    }
}

static BtkCTreeNode *
btk_ctree_last_visible (BtkCTree     *ctree,
			BtkCTreeNode *node)
{
  BtkCTreeNode *work;
  
  if (!node)
    return NULL;

  work = BTK_CTREE_ROW (node)->children;

  if (!work || !BTK_CTREE_ROW (node)->expanded)
    return node;

  while (BTK_CTREE_ROW (work)->sibling)
    work = BTK_CTREE_ROW (work)->sibling;

  return btk_ctree_last_visible (ctree, work);
}

static void
btk_ctree_link (BtkCTree     *ctree,
		BtkCTreeNode *node,
		BtkCTreeNode *parent,
		BtkCTreeNode *sibling,
		bboolean      update_focus_row)
{
  BtkCList *clist;
  GList *list_end;
  GList *list;
  GList *work;
  bboolean visible = FALSE;
  bint rows = 0;
  
  if (sibling)
    g_return_if_fail (BTK_CTREE_ROW (sibling)->parent == parent);
  g_return_if_fail (node != NULL);
  g_return_if_fail (node != sibling);
  g_return_if_fail (node != parent);

  clist = BTK_CLIST (ctree);

  if (update_focus_row && clist->selection_mode == BTK_SELECTION_MULTIPLE)
    {
      BTK_CLIST_GET_CLASS (clist)->resync_selection (clist, NULL);
      
      g_list_free (clist->undo_selection);
      g_list_free (clist->undo_unselection);
      clist->undo_selection = NULL;
      clist->undo_unselection = NULL;
    }

  for (rows = 1, list_end = (GList *)node; list_end->next;
       list_end = list_end->next)
    rows++;

  BTK_CTREE_ROW (node)->parent = parent;
  BTK_CTREE_ROW (node)->sibling = sibling;

  if (!parent || (parent && (btk_ctree_is_viewable (ctree, parent) &&
			     BTK_CTREE_ROW (parent)->expanded)))
    {
      visible = TRUE;
      clist->rows += rows;
    }

  if (parent)
    work = (GList *)(BTK_CTREE_ROW (parent)->children);
  else
    work = clist->row_list;

  if (sibling)
    {
      if (work != (GList *)sibling)
	{
	  while (BTK_CTREE_ROW (work)->sibling != sibling)
	    work = (GList *)(BTK_CTREE_ROW (work)->sibling);
	  BTK_CTREE_ROW (work)->sibling = node;
	}

      if (sibling == BTK_CTREE_NODE (clist->row_list))
	clist->row_list = (GList *) node;
      if (BTK_CTREE_NODE_PREV (sibling) &&
	  BTK_CTREE_NODE_NEXT (BTK_CTREE_NODE_PREV (sibling)) == sibling)
	{
	  list = (GList *)BTK_CTREE_NODE_PREV (sibling);
	  list->next = (GList *)node;
	}
      
      list = (GList *)node;
      list->prev = (GList *)BTK_CTREE_NODE_PREV (sibling);
      list_end->next = (GList *)sibling;
      list = (GList *)sibling;
      list->prev = list_end;
      if (parent && BTK_CTREE_ROW (parent)->children == sibling)
	BTK_CTREE_ROW (parent)->children = node;
    }
  else
    {
      if (work)
	{
	  /* find sibling */
	  while (BTK_CTREE_ROW (work)->sibling)
	    work = (GList *)(BTK_CTREE_ROW (work)->sibling);
	  BTK_CTREE_ROW (work)->sibling = node;
	  
	  /* find last visible child of sibling */
	  work = (GList *) btk_ctree_last_visible (ctree,
						   BTK_CTREE_NODE (work));
	  
	  list_end->next = work->next;
	  if (work->next)
	    list = work->next->prev = list_end;
	  work->next = (GList *)node;
	  list = (GList *)node;
	  list->prev = work;
	}
      else
	{
	  if (parent)
	    {
	      BTK_CTREE_ROW (parent)->children = node;
	      list = (GList *)node;
	      list->prev = (GList *)parent;
	      if (BTK_CTREE_ROW (parent)->expanded)
		{
		  list_end->next = (GList *)BTK_CTREE_NODE_NEXT (parent);
		  if (BTK_CTREE_NODE_NEXT(parent))
		    {
		      list = (GList *)BTK_CTREE_NODE_NEXT (parent);
		      list->prev = list_end;
		    }
		  list = (GList *)parent;
		  list->next = (GList *)node;
		}
	      else
		list_end->next = NULL;
	    }
	  else
	    {
	      clist->row_list = (GList *)node;
	      list = (GList *)node;
	      list->prev = NULL;
	      list_end->next = NULL;
	    }
	}
    }

  btk_ctree_pre_recursive (ctree, node, tree_update_level, NULL); 

  if (clist->row_list_end == NULL ||
      clist->row_list_end->next == (GList *)node)
    clist->row_list_end = list_end;

  if (visible && update_focus_row)
    {
      bint pos;
	  
      pos = g_list_position (clist->row_list, (GList *)node);
  
      if (pos <= clist->focus_row)
	{
	  clist->focus_row += rows;
	  clist->undo_anchor = clist->focus_row;
	}
    }
}

static void
btk_ctree_unlink (BtkCTree     *ctree, 
		  BtkCTreeNode *node,
                  bboolean      update_focus_row)
{
  BtkCList *clist;
  bint rows;
  bint level;
  bint visible;
  BtkCTreeNode *work;
  BtkCTreeNode *parent;
  GList *list;

  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (node != NULL);

  clist = BTK_CLIST (ctree);
  
  if (update_focus_row && clist->selection_mode == BTK_SELECTION_MULTIPLE)
    {
      BTK_CLIST_GET_CLASS (clist)->resync_selection (clist, NULL);
      
      g_list_free (clist->undo_selection);
      g_list_free (clist->undo_unselection);
      clist->undo_selection = NULL;
      clist->undo_unselection = NULL;
    }

  visible = btk_ctree_is_viewable (ctree, node);

  /* clist->row_list_end unlinked ? */
  if (visible &&
      (BTK_CTREE_NODE_NEXT (node) == NULL ||
       (BTK_CTREE_ROW (node)->children &&
	btk_ctree_is_ancestor (ctree, node,
			       BTK_CTREE_NODE (clist->row_list_end)))))
    clist->row_list_end = (GList *) (BTK_CTREE_NODE_PREV (node));

  /* update list */
  rows = 0;
  level = BTK_CTREE_ROW (node)->level;
  work = BTK_CTREE_NODE_NEXT (node);
  while (work && BTK_CTREE_ROW (work)->level > level)
    {
      work = BTK_CTREE_NODE_NEXT (work);
      rows++;
    }

  if (visible)
    {
      clist->rows -= (rows + 1);

      if (update_focus_row)
	{
	  bint pos;
	  
	  pos = g_list_position (clist->row_list, (GList *)node);
	  if (pos + rows < clist->focus_row)
	    clist->focus_row -= (rows + 1);
	  else if (pos <= clist->focus_row)
	    {
	      if (!BTK_CTREE_ROW (node)->sibling)
		clist->focus_row = MAX (pos - 1, 0);
	      else
		clist->focus_row = pos;
	      
	      clist->focus_row = MIN (clist->focus_row, clist->rows - 1);
	    }
	  clist->undo_anchor = clist->focus_row;
	}
    }

  if (work)
    {
      list = (GList *)BTK_CTREE_NODE_PREV (work);
      list->next = NULL;
      list = (GList *)work;
      list->prev = (GList *)BTK_CTREE_NODE_PREV (node);
    }

  if (BTK_CTREE_NODE_PREV (node) &&
      BTK_CTREE_NODE_NEXT (BTK_CTREE_NODE_PREV (node)) == node)
    {
      list = (GList *)BTK_CTREE_NODE_PREV (node);
      list->next = (GList *)work;
    }

  /* update tree */
  parent = BTK_CTREE_ROW (node)->parent;
  if (parent)
    {
      if (BTK_CTREE_ROW (parent)->children == node)
	{
	  BTK_CTREE_ROW (parent)->children = BTK_CTREE_ROW (node)->sibling;
	  if (!BTK_CTREE_ROW (parent)->children)
	    btk_ctree_collapse (ctree, parent);
	}
      else
	{
	  BtkCTreeNode *sibling;

	  sibling = BTK_CTREE_ROW (parent)->children;
	  while (BTK_CTREE_ROW (sibling)->sibling != node)
	    sibling = BTK_CTREE_ROW (sibling)->sibling;
	  BTK_CTREE_ROW (sibling)->sibling = BTK_CTREE_ROW (node)->sibling;
	}
    }
  else
    {
      if (clist->row_list == (GList *)node)
	clist->row_list = (GList *) (BTK_CTREE_ROW (node)->sibling);
      else
	{
	  BtkCTreeNode *sibling;

	  sibling = BTK_CTREE_NODE (clist->row_list);
	  while (BTK_CTREE_ROW (sibling)->sibling != node)
	    sibling = BTK_CTREE_ROW (sibling)->sibling;
	  BTK_CTREE_ROW (sibling)->sibling = BTK_CTREE_ROW (node)->sibling;
	}
    }
}

static void
real_row_move (BtkCList *clist,
	       bint      source_row,
	       bint      dest_row)
{
  BtkCTree *ctree;
  BtkCTreeNode *node;

  g_return_if_fail (BTK_IS_CTREE (clist));

  if (BTK_CLIST_AUTO_SORT (clist))
    return;

  if (source_row < 0 || source_row >= clist->rows ||
      dest_row   < 0 || dest_row   >= clist->rows ||
      source_row == dest_row)
    return;

  ctree = BTK_CTREE (clist);
  node = BTK_CTREE_NODE (g_list_nth (clist->row_list, source_row));

  if (source_row < dest_row)
    {
      BtkCTreeNode *work; 

      dest_row++;
      work = BTK_CTREE_ROW (node)->children;

      while (work && BTK_CTREE_ROW (work)->level > BTK_CTREE_ROW (node)->level)
	{
	  work = BTK_CTREE_NODE_NEXT (work);
	  dest_row++;
	}

      if (dest_row > clist->rows)
	dest_row = clist->rows;
    }

  if (dest_row < clist->rows)
    {
      BtkCTreeNode *sibling;

      sibling = BTK_CTREE_NODE (g_list_nth (clist->row_list, dest_row));
      btk_ctree_move (ctree, node, BTK_CTREE_ROW (sibling)->parent, sibling);
    }
  else
    btk_ctree_move (ctree, node, NULL, NULL);
}

static void
real_tree_move (BtkCTree     *ctree,
		BtkCTreeNode *node,
		BtkCTreeNode *new_parent, 
		BtkCTreeNode *new_sibling)
{
  BtkCList *clist;
  BtkCTreeNode *work;
  bboolean visible = FALSE;

  g_return_if_fail (ctree != NULL);
  g_return_if_fail (node != NULL);
  g_return_if_fail (!new_sibling || 
		    BTK_CTREE_ROW (new_sibling)->parent == new_parent);

  if (new_parent && BTK_CTREE_ROW (new_parent)->is_leaf)
    return;

  /* new_parent != child of child */
  for (work = new_parent; work; work = BTK_CTREE_ROW (work)->parent)
    if (work == node)
      return;

  clist = BTK_CLIST (ctree);

  visible = btk_ctree_is_viewable (ctree, node);

  if (clist->selection_mode == BTK_SELECTION_MULTIPLE)
    {
      BTK_CLIST_GET_CLASS (clist)->resync_selection (clist, NULL);
      
      g_list_free (clist->undo_selection);
      g_list_free (clist->undo_unselection);
      clist->undo_selection = NULL;
      clist->undo_unselection = NULL;
    }

  if (BTK_CLIST_AUTO_SORT (clist))
    {
      if (new_parent == BTK_CTREE_ROW (node)->parent)
	return;
      
      if (new_parent)
	new_sibling = BTK_CTREE_ROW (new_parent)->children;
      else
	new_sibling = BTK_CTREE_NODE (clist->row_list);

      while (new_sibling && clist->compare
	     (clist, BTK_CTREE_ROW (node), BTK_CTREE_ROW (new_sibling)) > 0)
	new_sibling = BTK_CTREE_ROW (new_sibling)->sibling;
    }

  if (new_parent == BTK_CTREE_ROW (node)->parent && 
      new_sibling == BTK_CTREE_ROW (node)->sibling)
    return;

  btk_clist_freeze (clist);

  work = NULL;
  if (btk_ctree_is_viewable (ctree, node))
    work = BTK_CTREE_NODE (g_list_nth (clist->row_list, clist->focus_row));
      
  btk_ctree_unlink (ctree, node, FALSE);
  btk_ctree_link (ctree, node, new_parent, new_sibling, FALSE);
  
  if (work)
    {
      while (work &&  !btk_ctree_is_viewable (ctree, work))
	work = BTK_CTREE_ROW (work)->parent;
      clist->focus_row = g_list_position (clist->row_list, (GList *)work);
      clist->undo_anchor = clist->focus_row;
    }

  if (clist->column[ctree->tree_column].auto_resize &&
      !BTK_CLIST_AUTO_RESIZE_BLOCKED (clist) &&
      (visible || btk_ctree_is_viewable (ctree, node)))
    btk_clist_set_column_width
      (clist, ctree->tree_column,
       btk_clist_optimal_column_width (clist, ctree->tree_column));

  btk_clist_thaw (clist);
}

static void
change_focus_row_expansion (BtkCTree          *ctree,
			    BtkCTreeExpansionType action)
{
  BtkCList *clist;
  BtkCTreeNode *node;

  g_return_if_fail (BTK_IS_CTREE (ctree));

  clist = BTK_CLIST (ctree);

  if (bdk_display_pointer_is_grabbed (btk_widget_get_display (BTK_WIDGET (ctree))) && 
      BTK_WIDGET_HAS_GRAB (ctree))
    return;
  
  if (!(node =
	BTK_CTREE_NODE (g_list_nth (clist->row_list, clist->focus_row))) ||
      BTK_CTREE_ROW (node)->is_leaf || !(BTK_CTREE_ROW (node)->children))
    return;

  switch (action)
    {
    case BTK_CTREE_EXPANSION_EXPAND:
      btk_ctree_expand (ctree, node);
      break;
    case BTK_CTREE_EXPANSION_EXPAND_RECURSIVE:
      btk_ctree_expand_recursive (ctree, node);
      break;
    case BTK_CTREE_EXPANSION_COLLAPSE:
      btk_ctree_collapse (ctree, node);
      break;
    case BTK_CTREE_EXPANSION_COLLAPSE_RECURSIVE:
      btk_ctree_collapse_recursive (ctree, node);
      break;
    case BTK_CTREE_EXPANSION_TOGGLE:
      btk_ctree_toggle_expansion (ctree, node);
      break;
    case BTK_CTREE_EXPANSION_TOGGLE_RECURSIVE:
      btk_ctree_toggle_expansion_recursive (ctree, node);
      break;
    }
}

static void 
real_tree_expand (BtkCTree     *ctree,
		  BtkCTreeNode *node)
{
  BtkCList *clist;
  BtkCTreeNode *work;
  BtkRequisition requisition;
  bboolean visible;

  g_return_if_fail (BTK_IS_CTREE (ctree));

  if (!node || BTK_CTREE_ROW (node)->expanded || BTK_CTREE_ROW (node)->is_leaf)
    return;

  clist = BTK_CLIST (ctree);
  
  BTK_CLIST_GET_CLASS (clist)->resync_selection (clist, NULL);

  BTK_CTREE_ROW (node)->expanded = TRUE;

  visible = btk_ctree_is_viewable (ctree, node);
  /* get cell width if tree_column is auto resized */
  if (visible && clist->column[ctree->tree_column].auto_resize &&
      !BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    BTK_CLIST_GET_CLASS (clist)->cell_size_request
      (clist, &BTK_CTREE_ROW (node)->row, ctree->tree_column, &requisition);

  /* unref/unset closed pixmap */
  if (BTK_CELL_PIXTEXT 
      (BTK_CTREE_ROW (node)->row.cell[ctree->tree_column])->pixmap)
    {
      g_object_unref
	(BTK_CELL_PIXTEXT
	 (BTK_CTREE_ROW (node)->row.cell[ctree->tree_column])->pixmap);
      
      BTK_CELL_PIXTEXT
	(BTK_CTREE_ROW (node)->row.cell[ctree->tree_column])->pixmap = NULL;
      
      if (BTK_CELL_PIXTEXT 
	  (BTK_CTREE_ROW (node)->row.cell[ctree->tree_column])->mask)
	{
	  g_object_unref
	    (BTK_CELL_PIXTEXT 
	     (BTK_CTREE_ROW (node)->row.cell[ctree->tree_column])->mask);
	  BTK_CELL_PIXTEXT 
	    (BTK_CTREE_ROW (node)->row.cell[ctree->tree_column])->mask = NULL;
	}
    }

  /* set/ref opened pixmap */
  if (BTK_CTREE_ROW (node)->pixmap_opened)
    {
      BTK_CELL_PIXTEXT 
	(BTK_CTREE_ROW (node)->row.cell[ctree->tree_column])->pixmap = 
	g_object_ref (BTK_CTREE_ROW (node)->pixmap_opened);

      if (BTK_CTREE_ROW (node)->mask_opened) 
	BTK_CELL_PIXTEXT 
	  (BTK_CTREE_ROW (node)->row.cell[ctree->tree_column])->mask = 
	  g_object_ref (BTK_CTREE_ROW (node)->mask_opened);
    }


  work = BTK_CTREE_ROW (node)->children;
  if (work)
    {
      GList *list = (GList *)work;
      bint *cell_width = NULL;
      bint tmp = 0;
      bint row;
      bint i;
      
      if (visible && !BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
	{
	  cell_width = g_new0 (bint, clist->columns);
	  if (clist->column[ctree->tree_column].auto_resize)
	      cell_width[ctree->tree_column] = requisition.width;

	  while (work)
	    {
	      /* search maximum cell widths of auto_resize columns */
	      for (i = 0; i < clist->columns; i++)
		if (clist->column[i].auto_resize)
		  {
		    BTK_CLIST_GET_CLASS (clist)->cell_size_request
		      (clist, &BTK_CTREE_ROW (work)->row, i, &requisition);
		    cell_width[i] = MAX (requisition.width, cell_width[i]);
		  }

	      list = (GList *)work;
	      work = BTK_CTREE_NODE_NEXT (work);
	      tmp++;
	    }
	}
      else
	while (work)
	  {
	    list = (GList *)work;
	    work = BTK_CTREE_NODE_NEXT (work);
	    tmp++;
	  }

      list->next = (GList *)BTK_CTREE_NODE_NEXT (node);

      if (BTK_CTREE_NODE_NEXT (node))
	{
	  GList *tmp_list;

	  tmp_list = (GList *)BTK_CTREE_NODE_NEXT (node);
	  tmp_list->prev = list;
	}
      else
	clist->row_list_end = list;

      list = (GList *)node;
      list->next = (GList *)(BTK_CTREE_ROW (node)->children);

      if (visible)
	{
	  /* resize auto_resize columns if needed */
	  for (i = 0; i < clist->columns; i++)
	    if (clist->column[i].auto_resize &&
		cell_width[i] > clist->column[i].width)
	      btk_clist_set_column_width (clist, i, cell_width[i]);
	  g_free (cell_width);

	  /* update focus_row position */
	  row = g_list_position (clist->row_list, (GList *)node);
	  if (row < clist->focus_row)
	    clist->focus_row += tmp;

	  clist->rows += tmp;
	  CLIST_REFRESH (clist);
	}
    }
  else if (visible && clist->column[ctree->tree_column].auto_resize)
    /* resize tree_column if needed */
    column_auto_resize (clist, &BTK_CTREE_ROW (node)->row, ctree->tree_column,
			requisition.width);
}

static void 
real_tree_collapse (BtkCTree     *ctree,
		    BtkCTreeNode *node)
{
  BtkCList *clist;
  BtkCTreeNode *work;
  BtkRequisition requisition;
  bboolean visible;
  bint level;

  g_return_if_fail (BTK_IS_CTREE (ctree));

  if (!node || !BTK_CTREE_ROW (node)->expanded ||
      BTK_CTREE_ROW (node)->is_leaf)
    return;

  clist = BTK_CLIST (ctree);

  BTK_CLIST_GET_CLASS (clist)->resync_selection (clist, NULL);
  
  BTK_CTREE_ROW (node)->expanded = FALSE;
  level = BTK_CTREE_ROW (node)->level;

  visible = btk_ctree_is_viewable (ctree, node);
  /* get cell width if tree_column is auto resized */
  if (visible && clist->column[ctree->tree_column].auto_resize &&
      !BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    BTK_CLIST_GET_CLASS (clist)->cell_size_request
      (clist, &BTK_CTREE_ROW (node)->row, ctree->tree_column, &requisition);

  /* unref/unset opened pixmap */
  if (BTK_CELL_PIXTEXT 
      (BTK_CTREE_ROW (node)->row.cell[ctree->tree_column])->pixmap)
    {
      g_object_unref
	(BTK_CELL_PIXTEXT
	 (BTK_CTREE_ROW (node)->row.cell[ctree->tree_column])->pixmap);
      
      BTK_CELL_PIXTEXT
	(BTK_CTREE_ROW (node)->row.cell[ctree->tree_column])->pixmap = NULL;
      
      if (BTK_CELL_PIXTEXT 
	  (BTK_CTREE_ROW (node)->row.cell[ctree->tree_column])->mask)
	{
	  g_object_unref
	    (BTK_CELL_PIXTEXT 
	     (BTK_CTREE_ROW (node)->row.cell[ctree->tree_column])->mask);
	  BTK_CELL_PIXTEXT 
	    (BTK_CTREE_ROW (node)->row.cell[ctree->tree_column])->mask = NULL;
	}
    }

  /* set/ref closed pixmap */
  if (BTK_CTREE_ROW (node)->pixmap_closed)
    {
      BTK_CELL_PIXTEXT 
	(BTK_CTREE_ROW (node)->row.cell[ctree->tree_column])->pixmap = 
	g_object_ref (BTK_CTREE_ROW (node)->pixmap_closed);

      if (BTK_CTREE_ROW (node)->mask_closed) 
	BTK_CELL_PIXTEXT 
	  (BTK_CTREE_ROW (node)->row.cell[ctree->tree_column])->mask = 
	  g_object_ref (BTK_CTREE_ROW (node)->mask_closed);
    }

  work = BTK_CTREE_ROW (node)->children;
  if (work)
    {
      bint tmp = 0;
      bint row;
      GList *list;

      while (work && BTK_CTREE_ROW (work)->level > level)
	{
	  work = BTK_CTREE_NODE_NEXT (work);
	  tmp++;
	}

      if (work)
	{
	  list = (GList *)node;
	  list->next = (GList *)work;
	  list = (GList *)BTK_CTREE_NODE_PREV (work);
	  list->next = NULL;
	  list = (GList *)work;
	  list->prev = (GList *)node;
	}
      else
	{
	  list = (GList *)node;
	  list->next = NULL;
	  clist->row_list_end = (GList *)node;
	}

      if (visible)
	{
	  /* resize auto_resize columns if needed */
	  auto_resize_columns (clist);

	  row = g_list_position (clist->row_list, (GList *)node);
	  if (row < clist->focus_row)
	    clist->focus_row -= tmp;
	  clist->rows -= tmp;
	  CLIST_REFRESH (clist);
	}
    }
  else if (visible && clist->column[ctree->tree_column].auto_resize &&
	   !BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    /* resize tree_column if needed */
    column_auto_resize (clist, &BTK_CTREE_ROW (node)->row, ctree->tree_column,
			requisition.width);
    
}

static void
column_auto_resize (BtkCList    *clist,
		    BtkCListRow *clist_row,
		    bint         column,
		    bint         old_width)
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
      bint new_width;

      /* run a "btk_clist_optimal_column_width" but break, if
       * the column doesn't shrink */
      if (BTK_CLIST_SHOW_TITLES (clist) && clist->column[column].button)
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
	btk_clist_set_column_width (clist, column, new_width);
    }
}

static void
auto_resize_columns (BtkCList *clist)
{
  bint i;

  if (BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    return;

  for (i = 0; i < clist->columns; i++)
    column_auto_resize (clist, NULL, i, clist->column[i].width);
}

static void
cell_size_request (BtkCList       *clist,
		   BtkCListRow    *clist_row,
		   bint            column,
		   BtkRequisition *requisition)
{
  BtkCTree *ctree;
  bint width;
  bint height;
  BangoLayout *layout;
  BangoRectangle logical_rect;

  g_return_if_fail (BTK_IS_CTREE (clist));
  g_return_if_fail (requisition != NULL);

  ctree = BTK_CTREE (clist);

  layout = _btk_clist_create_cell_layout (clist, clist_row, column);
  if (layout)
    {
      bango_layout_get_pixel_extents (layout, NULL, &logical_rect);

      requisition->width = logical_rect.width;
      requisition->height = logical_rect.height;
      
      g_object_unref (B_OBJECT (layout));
    }
  else
    {
      requisition->width  = 0;
      requisition->height = 0;
    }

  switch (clist_row->cell[column].type)
    {
    case BTK_CELL_PIXTEXT:
      if (BTK_CELL_PIXTEXT (clist_row->cell[column])->pixmap)
	{
	  bdk_drawable_get_size (BTK_CELL_PIXTEXT
                                 (clist_row->cell[column])->pixmap,
                                 &width, &height);
	  width += BTK_CELL_PIXTEXT (clist_row->cell[column])->spacing;
	}
      else
	width = height = 0;
	  
      requisition->width += width;
      requisition->height = MAX (requisition->height, height);
      
      if (column == ctree->tree_column)
	{
	  requisition->width += (ctree->tree_spacing + ctree->tree_indent *
				 (((BtkCTreeRow *) clist_row)->level - 1));
	  switch (ctree->expander_style)
	    {
	    case BTK_CTREE_EXPANDER_NONE:
	      break;
	    case BTK_CTREE_EXPANDER_TRIANGLE:
	      requisition->width += PM_SIZE + 3;
	      break;
	    case BTK_CTREE_EXPANDER_SQUARE:
	    case BTK_CTREE_EXPANDER_CIRCULAR:
	      requisition->width += PM_SIZE + 1;
	      break;
	    }
	  if (ctree->line_style == BTK_CTREE_LINES_TABBED)
	    requisition->width += 3;
	}
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

static void
set_cell_contents (BtkCList    *clist,
		   BtkCListRow *clist_row,
		   bint         column,
		   BtkCellType  type,
		   const bchar *text,
		   buint8       spacing,
		   BdkPixmap   *pixmap,
		   BdkBitmap   *mask)
{
  bboolean visible = FALSE;
  BtkCTree *ctree;
  BtkRequisition requisition;
  bchar *old_text = NULL;
  BdkPixmap *old_pixmap = NULL;
  BdkBitmap *old_mask = NULL;

  g_return_if_fail (BTK_IS_CTREE (clist));
  g_return_if_fail (clist_row != NULL);

  ctree = BTK_CTREE (clist);

  if (clist->column[column].auto_resize &&
      !BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    {
      BtkCTreeNode *parent;

      parent = ((BtkCTreeRow *)clist_row)->parent;
      if (!parent || (parent && BTK_CTREE_ROW (parent)->expanded &&
		      btk_ctree_is_viewable (ctree, parent)))
	{
	  visible = TRUE;
	  BTK_CLIST_GET_CLASS (clist)->cell_size_request (clist, clist_row,
							 column, &requisition);
	}
    }

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
  if (column == ctree->tree_column && type != BTK_CELL_EMPTY)
    type = BTK_CELL_PIXTEXT;

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
      if (column == ctree->tree_column)
	{
	  clist_row->cell[column].type = BTK_CELL_PIXTEXT;
	  BTK_CELL_PIXTEXT (clist_row->cell[column])->spacing = spacing;
	  if (text)
	    BTK_CELL_PIXTEXT (clist_row->cell[column])->text = g_strdup (text);
	  else
	    BTK_CELL_PIXTEXT (clist_row->cell[column])->text = NULL;
	  if (pixmap)
	    {
	      BTK_CELL_PIXTEXT (clist_row->cell[column])->pixmap = pixmap;
	      BTK_CELL_PIXTEXT (clist_row->cell[column])->mask = mask;
	    }
	  else
	    {
	      BTK_CELL_PIXTEXT (clist_row->cell[column])->pixmap = NULL;
	      BTK_CELL_PIXTEXT (clist_row->cell[column])->mask = NULL;
	    }
	}
      else if (text && pixmap)
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
  
  if (visible && clist->column[column].auto_resize &&
      !BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    column_auto_resize (clist, clist_row, column, requisition.width);

  g_free (old_text);
  if (old_pixmap)
    g_object_unref (old_pixmap);
  if (old_mask)
    g_object_unref (old_mask);
}

static void 
set_node_info (BtkCTree     *ctree,
	       BtkCTreeNode *node,
	       const bchar  *text,
	       buint8        spacing,
	       BdkPixmap    *pixmap_closed,
	       BdkBitmap    *mask_closed,
	       BdkPixmap    *pixmap_opened,
	       BdkBitmap    *mask_opened,
	       bboolean      is_leaf,
	       bboolean      expanded)
{
  if (BTK_CTREE_ROW (node)->pixmap_opened)
    {
      g_object_unref (BTK_CTREE_ROW (node)->pixmap_opened);
      if (BTK_CTREE_ROW (node)->mask_opened) 
	g_object_unref (BTK_CTREE_ROW (node)->mask_opened);
    }
  if (BTK_CTREE_ROW (node)->pixmap_closed)
    {
      g_object_unref (BTK_CTREE_ROW (node)->pixmap_closed);
      if (BTK_CTREE_ROW (node)->mask_closed) 
	g_object_unref (BTK_CTREE_ROW (node)->mask_closed);
    }

  BTK_CTREE_ROW (node)->pixmap_opened = NULL;
  BTK_CTREE_ROW (node)->mask_opened   = NULL;
  BTK_CTREE_ROW (node)->pixmap_closed = NULL;
  BTK_CTREE_ROW (node)->mask_closed   = NULL;

  if (pixmap_closed)
    {
      BTK_CTREE_ROW (node)->pixmap_closed = g_object_ref (pixmap_closed);
      if (mask_closed) 
	BTK_CTREE_ROW (node)->mask_closed = g_object_ref (mask_closed);
    }
  if (pixmap_opened)
    {
      BTK_CTREE_ROW (node)->pixmap_opened = g_object_ref (pixmap_opened);
      if (mask_opened) 
	BTK_CTREE_ROW (node)->mask_opened = g_object_ref (mask_opened);
    }

  BTK_CTREE_ROW (node)->is_leaf  = is_leaf;
  BTK_CTREE_ROW (node)->expanded = (is_leaf) ? FALSE : expanded;

  if (BTK_CTREE_ROW (node)->expanded)
    btk_ctree_node_set_pixtext (ctree, node, ctree->tree_column,
				text, spacing, pixmap_opened, mask_opened);
  else 
    btk_ctree_node_set_pixtext (ctree, node, ctree->tree_column,
				text, spacing, pixmap_closed, mask_closed);
}

static void
tree_delete (BtkCTree     *ctree, 
	     BtkCTreeNode *node, 
	     bpointer      data)
{
  tree_unselect (ctree,  node, NULL);
  row_delete (ctree, BTK_CTREE_ROW (node));
  g_list_free_1 ((GList *)node);
}

static void
tree_delete_row (BtkCTree     *ctree, 
		 BtkCTreeNode *node, 
		 bpointer      data)
{
  row_delete (ctree, BTK_CTREE_ROW (node));
  g_list_free_1 ((GList *)node);
}

static void
tree_update_level (BtkCTree     *ctree, 
		   BtkCTreeNode *node, 
		   bpointer      data)
{
  if (!node)
    return;

  if (BTK_CTREE_ROW (node)->parent)
      BTK_CTREE_ROW (node)->level = 
	BTK_CTREE_ROW (BTK_CTREE_ROW (node)->parent)->level + 1;
  else
      BTK_CTREE_ROW (node)->level = 1;
}

static void
tree_select (BtkCTree     *ctree, 
	     BtkCTreeNode *node, 
	     bpointer      data)
{
  if (node && BTK_CTREE_ROW (node)->row.state != BTK_STATE_SELECTED &&
      BTK_CTREE_ROW (node)->row.selectable)
    btk_signal_emit (BTK_OBJECT (ctree), ctree_signals[TREE_SELECT_ROW],
		     node, -1);
}

static void
tree_unselect (BtkCTree     *ctree, 
	       BtkCTreeNode *node, 
	       bpointer      data)
{
  if (node && BTK_CTREE_ROW (node)->row.state == BTK_STATE_SELECTED)
    btk_signal_emit (BTK_OBJECT (ctree), ctree_signals[TREE_UNSELECT_ROW], 
		     node, -1);
}

static void
tree_expand (BtkCTree     *ctree, 
	     BtkCTreeNode *node, 
	     bpointer      data)
{
  if (node && !BTK_CTREE_ROW (node)->expanded)
    btk_signal_emit (BTK_OBJECT (ctree), ctree_signals[TREE_EXPAND], node);
}

static void
tree_collapse (BtkCTree     *ctree, 
	       BtkCTreeNode *node, 
	       bpointer      data)
{
  if (node && BTK_CTREE_ROW (node)->expanded)
    btk_signal_emit (BTK_OBJECT (ctree), ctree_signals[TREE_COLLAPSE], node);
}

static void
tree_collapse_to_depth (BtkCTree     *ctree, 
			BtkCTreeNode *node, 
			bint          depth)
{
  if (node && BTK_CTREE_ROW (node)->level == depth)
    btk_ctree_collapse_recursive (ctree, node);
}

static void
tree_toggle_expansion (BtkCTree     *ctree,
		       BtkCTreeNode *node,
		       bpointer      data)
{
  if (!node)
    return;

  if (BTK_CTREE_ROW (node)->expanded)
    btk_signal_emit (BTK_OBJECT (ctree), ctree_signals[TREE_COLLAPSE], node);
  else
    btk_signal_emit (BTK_OBJECT (ctree), ctree_signals[TREE_EXPAND], node);
}

static BtkCTreeRow *
row_new (BtkCTree *ctree)
{
  BtkCList *clist;
  BtkCTreeRow *ctree_row;
  int i;

  clist = BTK_CLIST (ctree);
  ctree_row = g_slice_new (BtkCTreeRow);
  ctree_row->row.cell = g_slice_alloc (sizeof (BtkCell) * clist->columns);

  for (i = 0; i < clist->columns; i++)
    {
      ctree_row->row.cell[i].type = BTK_CELL_EMPTY;
      ctree_row->row.cell[i].vertical = 0;
      ctree_row->row.cell[i].horizontal = 0;
      ctree_row->row.cell[i].style = NULL;
    }

  BTK_CELL_PIXTEXT (ctree_row->row.cell[ctree->tree_column])->text = NULL;

  ctree_row->row.fg_set     = FALSE;
  ctree_row->row.bg_set     = FALSE;
  ctree_row->row.style      = NULL;
  ctree_row->row.selectable = TRUE;
  ctree_row->row.state      = BTK_STATE_NORMAL;
  ctree_row->row.data       = NULL;
  ctree_row->row.destroy    = NULL;

  ctree_row->level         = 0;
  ctree_row->expanded      = FALSE;
  ctree_row->parent        = NULL;
  ctree_row->sibling       = NULL;
  ctree_row->children      = NULL;
  ctree_row->pixmap_closed = NULL;
  ctree_row->mask_closed   = NULL;
  ctree_row->pixmap_opened = NULL;
  ctree_row->mask_opened   = NULL;
  
  return ctree_row;
}

static void
row_delete (BtkCTree    *ctree,
	    BtkCTreeRow *ctree_row)
{
  BtkCList *clist;
  bint i;

  clist = BTK_CLIST (ctree);

  for (i = 0; i < clist->columns; i++)
    {
      BTK_CLIST_GET_CLASS (clist)->set_cell_contents
	(clist, &(ctree_row->row), i, BTK_CELL_EMPTY, NULL, 0, NULL, NULL);
      if (ctree_row->row.cell[i].style)
	{
	  if (btk_widget_get_realized (BTK_WIDGET (ctree)))
	    btk_style_detach (ctree_row->row.cell[i].style);
	  g_object_unref (ctree_row->row.cell[i].style);
	}
    }

  if (ctree_row->row.style)
    {
      if (btk_widget_get_realized (BTK_WIDGET (ctree)))
	btk_style_detach (ctree_row->row.style);
      g_object_unref (ctree_row->row.style);
    }

  if (ctree_row->pixmap_closed)
    {
      g_object_unref (ctree_row->pixmap_closed);
      if (ctree_row->mask_closed)
	g_object_unref (ctree_row->mask_closed);
    }

  if (ctree_row->pixmap_opened)
    {
      g_object_unref (ctree_row->pixmap_opened);
      if (ctree_row->mask_opened)
	g_object_unref (ctree_row->mask_opened);
    }

  if (ctree_row->row.destroy)
    {
      GDestroyNotify dnotify = ctree_row->row.destroy;
      bpointer ddata = ctree_row->row.data;

      ctree_row->row.destroy = NULL;
      ctree_row->row.data = NULL;

      dnotify (ddata);
    }

  g_slice_free1 (sizeof (BtkCell) * clist->columns, ctree_row->row.cell);
  g_slice_free (BtkCTreeRow, ctree_row);
}

static void
real_select_row (BtkCList *clist,
		 bint      row,
		 bint      column,
		 BdkEvent *event)
{
  GList *node;

  g_return_if_fail (BTK_IS_CTREE (clist));
  
  if ((node = g_list_nth (clist->row_list, row)) &&
      BTK_CTREE_ROW (node)->row.selectable)
    btk_signal_emit (BTK_OBJECT (clist), ctree_signals[TREE_SELECT_ROW],
		     node, column);
}

static void
real_unselect_row (BtkCList *clist,
		   bint      row,
		   bint      column,
		   BdkEvent *event)
{
  GList *node;

  g_return_if_fail (BTK_IS_CTREE (clist));

  if ((node = g_list_nth (clist->row_list, row)))
    btk_signal_emit (BTK_OBJECT (clist), ctree_signals[TREE_UNSELECT_ROW],
		     node, column);
}

static void
real_tree_select (BtkCTree     *ctree,
		  BtkCTreeNode *node,
		  bint          column)
{
  BtkCList *clist;
  GList *list;
  BtkCTreeNode *sel_row;
  bboolean node_selected;

  g_return_if_fail (BTK_IS_CTREE (ctree));

  if (!node || BTK_CTREE_ROW (node)->row.state == BTK_STATE_SELECTED ||
      !BTK_CTREE_ROW (node)->row.selectable)
    return;

  clist = BTK_CLIST (ctree);

  switch (clist->selection_mode)
    {
    case BTK_SELECTION_SINGLE:
    case BTK_SELECTION_BROWSE:

      node_selected = FALSE;
      list = clist->selection;

      while (list)
	{
	  sel_row = list->data;
	  list = list->next;
	  
	  if (node == sel_row)
	    node_selected = TRUE;
	  else
	    btk_signal_emit (BTK_OBJECT (ctree),
			     ctree_signals[TREE_UNSELECT_ROW], sel_row, column);
	}

      if (node_selected)
	return;

    default:
      break;
    }

  BTK_CTREE_ROW (node)->row.state = BTK_STATE_SELECTED;

  if (!clist->selection)
    {
      clist->selection = g_list_append (clist->selection, node);
      clist->selection_end = clist->selection;
    }
  else
    clist->selection_end = g_list_append (clist->selection_end, node)->next;

  tree_draw_node (ctree, node);
}

static void
real_tree_unselect (BtkCTree     *ctree,
		    BtkCTreeNode *node,
		    bint          column)
{
  BtkCList *clist;

  g_return_if_fail (BTK_IS_CTREE (ctree));

  if (!node || BTK_CTREE_ROW (node)->row.state != BTK_STATE_SELECTED)
    return;

  clist = BTK_CLIST (ctree);

  if (clist->selection_end && clist->selection_end->data == node)
    clist->selection_end = clist->selection_end->prev;

  clist->selection = g_list_remove (clist->selection, node);
  
  BTK_CTREE_ROW (node)->row.state = BTK_STATE_NORMAL;

  tree_draw_node (ctree, node);
}

static void
select_row_recursive (BtkCTree     *ctree, 
		      BtkCTreeNode *node, 
		      bpointer      data)
{
  if (!node || BTK_CTREE_ROW (node)->row.state == BTK_STATE_SELECTED ||
      !BTK_CTREE_ROW (node)->row.selectable)
    return;

  BTK_CLIST (ctree)->undo_unselection = 
    g_list_prepend (BTK_CLIST (ctree)->undo_unselection, node);
  btk_ctree_select (ctree, node);
}

static void
real_select_all (BtkCList *clist)
{
  BtkCTree *ctree;
  BtkCTreeNode *node;
  
  g_return_if_fail (BTK_IS_CTREE (clist));

  ctree = BTK_CTREE (clist);

  switch (clist->selection_mode)
    {
    case BTK_SELECTION_SINGLE:
    case BTK_SELECTION_BROWSE:
      return;

    case BTK_SELECTION_MULTIPLE:

      btk_clist_freeze (clist);

      g_list_free (clist->undo_selection);
      g_list_free (clist->undo_unselection);
      clist->undo_selection = NULL;
      clist->undo_unselection = NULL;
	  
      clist->anchor_state = BTK_STATE_SELECTED;
      clist->anchor = -1;
      clist->drag_pos = -1;
      clist->undo_anchor = clist->focus_row;

      for (node = BTK_CTREE_NODE (clist->row_list); node;
	   node = BTK_CTREE_NODE_NEXT (node))
	btk_ctree_pre_recursive (ctree, node, select_row_recursive, NULL);

      btk_clist_thaw (clist);
      break;

    default:
      /* do nothing */
      break;
    }
}

static void
real_unselect_all (BtkCList *clist)
{
  BtkCTree *ctree;
  BtkCTreeNode *node;
  GList *list;
 
  g_return_if_fail (BTK_IS_CTREE (clist));
  
  ctree = BTK_CTREE (clist);

  switch (clist->selection_mode)
    {
    case BTK_SELECTION_BROWSE:
      if (clist->focus_row >= 0)
	{
	  btk_ctree_select
	    (ctree,
	     BTK_CTREE_NODE (g_list_nth (clist->row_list, clist->focus_row)));
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
      node = list->data;
      list = list->next;
      btk_ctree_unselect (ctree, node);
    }
}

static bboolean
ctree_is_hot_spot (BtkCTree     *ctree, 
		   BtkCTreeNode *node,
		   bint          row, 
		   bint          x, 
		   bint          y)
{
  BtkCTreeRow *tree_row;
  BtkCList *clist;
  bint xl;
  bint yu;
  
  g_return_val_if_fail (BTK_IS_CTREE (ctree), FALSE);
  g_return_val_if_fail (node != NULL, FALSE);

  clist = BTK_CLIST (ctree);

  if (!clist->column[ctree->tree_column].visible ||
      ctree->expander_style == BTK_CTREE_EXPANDER_NONE)
    return FALSE;

  tree_row = BTK_CTREE_ROW (node);

  yu = (ROW_TOP_YPIXEL (clist, row) + (clist->row_height - PM_SIZE) / 2 -
	(clist->row_height - 1) % 2);

  if (clist->column[ctree->tree_column].justification == BTK_JUSTIFY_RIGHT)
    xl = (clist->column[ctree->tree_column].area.x + 
	  clist->column[ctree->tree_column].area.width - 1 + clist->hoffset -
	  (tree_row->level - 1) * ctree->tree_indent - PM_SIZE -
	  (ctree->line_style == BTK_CTREE_LINES_TABBED) * 3);
  else
    xl = (clist->column[ctree->tree_column].area.x + clist->hoffset +
	  (tree_row->level - 1) * ctree->tree_indent +
	  (ctree->line_style == BTK_CTREE_LINES_TABBED) * 3);

  return (x >= xl && x <= xl + PM_SIZE && y >= yu && y <= yu + PM_SIZE);
}

/***********************************************************
 ***********************************************************
 ***                  Public interface                   ***
 ***********************************************************
 ***********************************************************/


/***********************************************************
 *           Creation, insertion, deletion                 *
 ***********************************************************/

static BObject*
btk_ctree_constructor (GType                  type,
		       buint                  n_construct_properties,
		       BObjectConstructParam *construct_properties)
{
  BObject *object = B_OBJECT_CLASS (parent_class)->constructor (type,
								n_construct_properties,
								construct_properties);

  return object;
}

BtkWidget*
btk_ctree_new_with_titles (bint         columns, 
			   bint         tree_column,
			   bchar       *titles[])
{
  BtkWidget *widget;

  g_return_val_if_fail (columns > 0, NULL);
  g_return_val_if_fail (tree_column >= 0 && tree_column < columns, NULL);

  widget = g_object_new (BTK_TYPE_CTREE,
			   "n_columns", columns,
			   "tree_column", tree_column,
			   NULL);
  if (titles)
    {
      BtkCList *clist = BTK_CLIST (widget);
      buint i;

      for (i = 0; i < columns; i++)
	btk_clist_set_column_title (clist, i, titles[i]);
      btk_clist_column_titles_show (clist);
    }

  return widget;
}

BtkWidget *
btk_ctree_new (bint columns, 
	       bint tree_column)
{
  return btk_ctree_new_with_titles (columns, tree_column, NULL);
}

static bint
real_insert_row (BtkCList *clist,
		 bint      row,
		 bchar    *text[])
{
  BtkCTreeNode *parent = NULL;
  BtkCTreeNode *sibling;
  BtkCTreeNode *node;

  g_return_val_if_fail (BTK_IS_CTREE (clist), -1);

  sibling = BTK_CTREE_NODE (g_list_nth (clist->row_list, row));
  if (sibling)
    parent = BTK_CTREE_ROW (sibling)->parent;

  node = btk_ctree_insert_node (BTK_CTREE (clist), parent, sibling, text, 5,
				NULL, NULL, NULL, NULL, TRUE, FALSE);

  if (BTK_CLIST_AUTO_SORT (clist) || !sibling)
    return g_list_position (clist->row_list, (GList *) node);
  
  return row;
}


/**
 * btk_ctree_insert_node:
 * @pixmap_closed: (allow-none):
 * @mask_closed: (allow-none):
 * @pixmap_opened: (allow-none):
 * @mask_opened: (allow-none):
 */
BtkCTreeNode *
btk_ctree_insert_node (BtkCTree     *ctree,
		       BtkCTreeNode *parent,
		       BtkCTreeNode *sibling,
		       bchar        *text[],
		       buint8        spacing,
		       BdkPixmap    *pixmap_closed,
		       BdkBitmap    *mask_closed,
		       BdkPixmap    *pixmap_opened,
		       BdkBitmap    *mask_opened,
		       bboolean      is_leaf,
		       bboolean      expanded)
{
  BtkCList *clist;
  BtkCTreeRow *new_row;
  BtkCTreeNode *node;
  GList *list;
  bint i;

  g_return_val_if_fail (BTK_IS_CTREE (ctree), NULL);
  if (sibling)
    g_return_val_if_fail (BTK_CTREE_ROW (sibling)->parent == parent, NULL);

  if (parent && BTK_CTREE_ROW (parent)->is_leaf)
    return NULL;

  clist = BTK_CLIST (ctree);

  /* create the row */
  new_row = row_new (ctree);
  list = g_list_alloc ();
  list->data = new_row;
  node = BTK_CTREE_NODE (list);

  if (text)
    for (i = 0; i < clist->columns; i++)
      if (text[i] && i != ctree->tree_column)
	BTK_CLIST_GET_CLASS (clist)->set_cell_contents
	  (clist, &(new_row->row), i, BTK_CELL_TEXT, text[i], 0, NULL, NULL);

  set_node_info (ctree, node, text ?
		 text[ctree->tree_column] : NULL, spacing, pixmap_closed,
		 mask_closed, pixmap_opened, mask_opened, is_leaf, expanded);

  /* sorted insertion */
  if (BTK_CLIST_AUTO_SORT (clist))
    {
      if (parent)
	sibling = BTK_CTREE_ROW (parent)->children;
      else
	sibling = BTK_CTREE_NODE (clist->row_list);

      while (sibling && clist->compare
	     (clist, BTK_CTREE_ROW (node), BTK_CTREE_ROW (sibling)) > 0)
	sibling = BTK_CTREE_ROW (sibling)->sibling;
    }

  btk_ctree_link (ctree, node, parent, sibling, TRUE);

  if (text && !BTK_CLIST_AUTO_RESIZE_BLOCKED (clist) &&
      btk_ctree_is_viewable (ctree, node))
    {
      for (i = 0; i < clist->columns; i++)
	if (clist->column[i].auto_resize)
	  column_auto_resize (clist, &(new_row->row), i, 0);
    }

  if (clist->rows == 1)
    {
      clist->focus_row = 0;
      if (clist->selection_mode == BTK_SELECTION_BROWSE)
	btk_ctree_select (ctree, node);
    }


  CLIST_REFRESH (clist);

  return node;
}

BtkCTreeNode *
btk_ctree_insert_gnode (BtkCTree          *ctree,
			BtkCTreeNode      *parent,
			BtkCTreeNode      *sibling,
			GNode             *gnode,
			BtkCTreeGNodeFunc  func,
			bpointer           data)
{
  BtkCList *clist;
  BtkCTreeNode *cnode = NULL;
  BtkCTreeNode *child = NULL;
  BtkCTreeNode *new_child;
  GList *list;
  GNode *work;
  buint depth = 1;

  g_return_val_if_fail (BTK_IS_CTREE (ctree), NULL);
  g_return_val_if_fail (gnode != NULL, NULL);
  g_return_val_if_fail (func != NULL, NULL);
  if (sibling)
    g_return_val_if_fail (BTK_CTREE_ROW (sibling)->parent == parent, NULL);
  
  clist = BTK_CLIST (ctree);

  if (parent)
    depth = BTK_CTREE_ROW (parent)->level + 1;

  list = g_list_alloc ();
  list->data = row_new (ctree);
  cnode = BTK_CTREE_NODE (list);

  btk_clist_freeze (clist);

  set_node_info (ctree, cnode, "", 0, NULL, NULL, NULL, NULL, TRUE, FALSE);

  if (!func (ctree, depth, gnode, cnode, data))
    {
      tree_delete_row (ctree, cnode, NULL);
      btk_clist_thaw (clist);
      return NULL;
    }

  if (BTK_CLIST_AUTO_SORT (clist))
    {
      if (parent)
	sibling = BTK_CTREE_ROW (parent)->children;
      else
	sibling = BTK_CTREE_NODE (clist->row_list);

      while (sibling && clist->compare
	     (clist, BTK_CTREE_ROW (cnode), BTK_CTREE_ROW (sibling)) > 0)
	sibling = BTK_CTREE_ROW (sibling)->sibling;
    }

  btk_ctree_link (ctree, cnode, parent, sibling, TRUE);

  for (work = g_node_last_child (gnode); work; work = work->prev)
    {
      new_child = btk_ctree_insert_gnode (ctree, cnode, child,
					  work, func, data);
      if (new_child)
	child = new_child;
    }	
  
  btk_clist_thaw (clist);

  return cnode;
}

GNode *
btk_ctree_export_to_gnode (BtkCTree          *ctree,
			   GNode             *parent,
			   GNode             *sibling,
			   BtkCTreeNode      *node,
			   BtkCTreeGNodeFunc  func,
			   bpointer           data)
{
  BtkCTreeNode *work;
  GNode *gnode;
  bint depth;

  g_return_val_if_fail (BTK_IS_CTREE (ctree), NULL);
  g_return_val_if_fail (node != NULL, NULL);
  g_return_val_if_fail (func != NULL, NULL);
  if (sibling)
    {
      g_return_val_if_fail (parent != NULL, NULL);
      g_return_val_if_fail (sibling->parent == parent, NULL);
    }

  gnode = g_node_new (NULL);
  depth = g_node_depth (parent) + 1;
  
  if (!func (ctree, depth, gnode, node, data))
    {
      g_node_destroy (gnode);
      return NULL;
    }

  if (parent)
    g_node_insert_before (parent, sibling, gnode);

  if (!BTK_CTREE_ROW (node)->is_leaf)
    {
      GNode *new_sibling = NULL;

      for (work = BTK_CTREE_ROW (node)->children; work;
	   work = BTK_CTREE_ROW (work)->sibling)
	new_sibling = btk_ctree_export_to_gnode (ctree, gnode, new_sibling,
						 work, func, data);

      g_node_reverse_children (gnode);
    }

  return gnode;
}
  
static void
real_remove_row (BtkCList *clist,
		 bint      row)
{
  BtkCTreeNode *node;

  g_return_if_fail (BTK_IS_CTREE (clist));

  node = BTK_CTREE_NODE (g_list_nth (clist->row_list, row));

  if (node)
    btk_ctree_remove_node (BTK_CTREE (clist), node);
}

void
btk_ctree_remove_node (BtkCTree     *ctree, 
		       BtkCTreeNode *node)
{
  BtkCList *clist;

  g_return_if_fail (BTK_IS_CTREE (ctree));

  clist = BTK_CLIST (ctree);

  btk_clist_freeze (clist);

  if (node)
    {
      btk_ctree_unlink (ctree, node, TRUE);
      btk_ctree_post_recursive (ctree, node, BTK_CTREE_FUNC (tree_delete),
				NULL);
      if (clist->selection_mode == BTK_SELECTION_BROWSE && !clist->selection &&
	  clist->focus_row >= 0)
	btk_clist_select_row (clist, clist->focus_row, -1);

      auto_resize_columns (clist);
    }
  else
    btk_clist_clear (clist);

  btk_clist_thaw (clist);
}

static void
real_clear (BtkCList *clist)
{
  BtkCTree *ctree;
  BtkCTreeNode *work;
  BtkCTreeNode *ptr;

  g_return_if_fail (BTK_IS_CTREE (clist));

  ctree = BTK_CTREE (clist);

  /* remove all rows */
  work = BTK_CTREE_NODE (clist->row_list);
  clist->row_list = NULL;
  clist->row_list_end = NULL;

  BTK_CLIST_SET_FLAG (clist, CLIST_AUTO_RESIZE_BLOCKED);
  while (work)
    {
      ptr = work;
      work = BTK_CTREE_ROW (work)->sibling;
      btk_ctree_post_recursive (ctree, ptr, BTK_CTREE_FUNC (tree_delete_row), 
				NULL);
    }
  BTK_CLIST_UNSET_FLAG (clist, CLIST_AUTO_RESIZE_BLOCKED);

  parent_class->clear (clist);
}


/***********************************************************
 *  Generic recursive functions, querying / finding tree   *
 *  information                                            *
 ***********************************************************/


void
btk_ctree_post_recursive (BtkCTree     *ctree, 
			  BtkCTreeNode *node,
			  BtkCTreeFunc  func,
			  bpointer      data)
{
  BtkCTreeNode *work;
  BtkCTreeNode *tmp;

  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (func != NULL);

  if (node)
    work = BTK_CTREE_ROW (node)->children;
  else
    work = BTK_CTREE_NODE (BTK_CLIST (ctree)->row_list);

  while (work)
    {
      tmp = BTK_CTREE_ROW (work)->sibling;
      btk_ctree_post_recursive (ctree, work, func, data);
      work = tmp;
    }

  if (node)
    func (ctree, node, data);
}

void
btk_ctree_post_recursive_to_depth (BtkCTree     *ctree, 
				   BtkCTreeNode *node,
				   bint          depth,
				   BtkCTreeFunc  func,
				   bpointer      data)
{
  BtkCTreeNode *work;
  BtkCTreeNode *tmp;

  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (func != NULL);

  if (depth < 0)
    {
      btk_ctree_post_recursive (ctree, node, func, data);
      return;
    }

  if (node)
    work = BTK_CTREE_ROW (node)->children;
  else
    work = BTK_CTREE_NODE (BTK_CLIST (ctree)->row_list);

  if (work && BTK_CTREE_ROW (work)->level <= depth)
    {
      while (work)
	{
	  tmp = BTK_CTREE_ROW (work)->sibling;
	  btk_ctree_post_recursive_to_depth (ctree, work, depth, func, data);
	  work = tmp;
	}
    }

  if (node && BTK_CTREE_ROW (node)->level <= depth)
    func (ctree, node, data);
}

void
btk_ctree_pre_recursive (BtkCTree     *ctree, 
			 BtkCTreeNode *node,
			 BtkCTreeFunc  func,
			 bpointer      data)
{
  BtkCTreeNode *work;
  BtkCTreeNode *tmp;

  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (func != NULL);

  if (node)
    {
      work = BTK_CTREE_ROW (node)->children;
      func (ctree, node, data);
    }
  else
    work = BTK_CTREE_NODE (BTK_CLIST (ctree)->row_list);

  while (work)
    {
      tmp = BTK_CTREE_ROW (work)->sibling;
      btk_ctree_pre_recursive (ctree, work, func, data);
      work = tmp;
    }
}

void
btk_ctree_pre_recursive_to_depth (BtkCTree     *ctree, 
				  BtkCTreeNode *node,
				  bint          depth, 
				  BtkCTreeFunc  func,
				  bpointer      data)
{
  BtkCTreeNode *work;
  BtkCTreeNode *tmp;

  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (func != NULL);

  if (depth < 0)
    {
      btk_ctree_pre_recursive (ctree, node, func, data);
      return;
    }

  if (node)
    {
      work = BTK_CTREE_ROW (node)->children;
      if (BTK_CTREE_ROW (node)->level <= depth)
	func (ctree, node, data);
    }
  else
    work = BTK_CTREE_NODE (BTK_CLIST (ctree)->row_list);

  if (work && BTK_CTREE_ROW (work)->level <= depth)
    {
      while (work)
	{
	  tmp = BTK_CTREE_ROW (work)->sibling;
	  btk_ctree_pre_recursive_to_depth (ctree, work, depth, func, data);
	  work = tmp;
	}
    }
}

bboolean
btk_ctree_is_viewable (BtkCTree     *ctree, 
		       BtkCTreeNode *node)
{ 
  BtkCTreeRow *work;

  g_return_val_if_fail (BTK_IS_CTREE (ctree), FALSE);
  g_return_val_if_fail (node != NULL, FALSE);

  work = BTK_CTREE_ROW (node);

  while (work->parent && BTK_CTREE_ROW (work->parent)->expanded)
    work = BTK_CTREE_ROW (work->parent);

  if (!work->parent)
    return TRUE;

  return FALSE;
}

BtkCTreeNode * 
btk_ctree_last (BtkCTree     *ctree,
		BtkCTreeNode *node)
{
  g_return_val_if_fail (BTK_IS_CTREE (ctree), NULL);

  if (!node) 
    return NULL;

  while (BTK_CTREE_ROW (node)->sibling)
    node = BTK_CTREE_ROW (node)->sibling;
  
  if (BTK_CTREE_ROW (node)->children)
    return btk_ctree_last (ctree, BTK_CTREE_ROW (node)->children);
  
  return node;
}

BtkCTreeNode *
btk_ctree_find_node_ptr (BtkCTree    *ctree,
			 BtkCTreeRow *ctree_row)
{
  BtkCTreeNode *node;
  
  g_return_val_if_fail (BTK_IS_CTREE (ctree), NULL);
  g_return_val_if_fail (ctree_row != NULL, NULL);
  
  if (ctree_row->parent)
    node = BTK_CTREE_ROW (ctree_row->parent)->children;
  else
    node = BTK_CTREE_NODE (BTK_CLIST (ctree)->row_list);

  while (BTK_CTREE_ROW (node) != ctree_row)
    node = BTK_CTREE_ROW (node)->sibling;
  
  return node;
}

BtkCTreeNode *
btk_ctree_node_nth (BtkCTree *ctree,
		    buint     row)
{
  g_return_val_if_fail (BTK_IS_CTREE (ctree), NULL);

  if ((row >= BTK_CLIST(ctree)->rows))
    return NULL;
 
  return BTK_CTREE_NODE (g_list_nth (BTK_CLIST (ctree)->row_list, row));
}

bboolean
btk_ctree_find (BtkCTree     *ctree,
		BtkCTreeNode *node,
		BtkCTreeNode *child)
{
  if (!child)
    return FALSE;

  if (!node)
    node = BTK_CTREE_NODE (BTK_CLIST (ctree)->row_list);

  while (node)
    {
      if (node == child) 
	return TRUE;
      if (BTK_CTREE_ROW (node)->children)
	{
	  if (btk_ctree_find (ctree, BTK_CTREE_ROW (node)->children, child))
	    return TRUE;
	}
      node = BTK_CTREE_ROW (node)->sibling;
    }
  return FALSE;
}

bboolean
btk_ctree_is_ancestor (BtkCTree     *ctree,
		       BtkCTreeNode *node,
		       BtkCTreeNode *child)
{
  g_return_val_if_fail (BTK_IS_CTREE (ctree), FALSE);
  g_return_val_if_fail (node != NULL, FALSE);

  if (BTK_CTREE_ROW (node)->children)
    return btk_ctree_find (ctree, BTK_CTREE_ROW (node)->children, child);

  return FALSE;
}

BtkCTreeNode *
btk_ctree_find_by_row_data (BtkCTree     *ctree,
			    BtkCTreeNode *node,
			    bpointer      data)
{
  BtkCTreeNode *work;
  
  if (!node)
    node = BTK_CTREE_NODE (BTK_CLIST (ctree)->row_list);
  
  while (node)
    {
      if (BTK_CTREE_ROW (node)->row.data == data) 
	return node;
      if (BTK_CTREE_ROW (node)->children &&
	  (work = btk_ctree_find_by_row_data 
	   (ctree, BTK_CTREE_ROW (node)->children, data)))
	return work;
      node = BTK_CTREE_ROW (node)->sibling;
    }
  return NULL;
}

GList *
btk_ctree_find_all_by_row_data (BtkCTree     *ctree,
				BtkCTreeNode *node,
				bpointer      data)
{
  GList *list = NULL;

  g_return_val_if_fail (BTK_IS_CTREE (ctree), NULL);

  /* if node == NULL then look in the whole tree */
  if (!node)
    node = BTK_CTREE_NODE (BTK_CLIST (ctree)->row_list);

  while (node)
    {
      if (BTK_CTREE_ROW (node)->row.data == data)
        list = g_list_append (list, node);

      if (BTK_CTREE_ROW (node)->children)
        {
	  GList *sub_list;

          sub_list = btk_ctree_find_all_by_row_data (ctree,
						     BTK_CTREE_ROW
						     (node)->children,
						     data);
          list = g_list_concat (list, sub_list);
        }
      node = BTK_CTREE_ROW (node)->sibling;
    }
  return list;
}

BtkCTreeNode *
btk_ctree_find_by_row_data_custom (BtkCTree     *ctree,
				   BtkCTreeNode *node,
				   bpointer      data,
				   GCompareFunc  func)
{
  BtkCTreeNode *work;

  g_return_val_if_fail (func != NULL, NULL);

  if (!node)
    node = BTK_CTREE_NODE (BTK_CLIST (ctree)->row_list);

  while (node)
    {
      if (!func (BTK_CTREE_ROW (node)->row.data, data))
	return node;
      if (BTK_CTREE_ROW (node)->children &&
	  (work = btk_ctree_find_by_row_data_custom
	   (ctree, BTK_CTREE_ROW (node)->children, data, func)))
	return work;
      node = BTK_CTREE_ROW (node)->sibling;
    }
  return NULL;
}

GList *
btk_ctree_find_all_by_row_data_custom (BtkCTree     *ctree,
				       BtkCTreeNode *node,
				       bpointer      data,
				       GCompareFunc  func)
{
  GList *list = NULL;

  g_return_val_if_fail (BTK_IS_CTREE (ctree), NULL);
  g_return_val_if_fail (func != NULL, NULL);

  /* if node == NULL then look in the whole tree */
  if (!node)
    node = BTK_CTREE_NODE (BTK_CLIST (ctree)->row_list);

  while (node)
    {
      if (!func (BTK_CTREE_ROW (node)->row.data, data))
        list = g_list_append (list, node);

      if (BTK_CTREE_ROW (node)->children)
        {
	  GList *sub_list;

          sub_list = btk_ctree_find_all_by_row_data_custom (ctree,
							    BTK_CTREE_ROW
							    (node)->children,
							    data,
							    func);
          list = g_list_concat (list, sub_list);
        }
      node = BTK_CTREE_ROW (node)->sibling;
    }
  return list;
}

bboolean
btk_ctree_is_hot_spot (BtkCTree *ctree, 
		       bint      x, 
		       bint      y)
{
  BtkCTreeNode *node;
  bint column;
  bint row;
  
  g_return_val_if_fail (BTK_IS_CTREE (ctree), FALSE);

  if (btk_clist_get_selection_info (BTK_CLIST (ctree), x, y, &row, &column))
    if ((node = BTK_CTREE_NODE(g_list_nth (BTK_CLIST (ctree)->row_list, row))))
      return ctree_is_hot_spot (ctree, node, row, x, y);

  return FALSE;
}


/***********************************************************
 *   Tree signals : move, expand, collapse, (un)select     *
 ***********************************************************/


/**
 * btk_ctree_move:
 * @new_parent: (allow-none):
 * @new_sibling: (allow-none):
 */
void
btk_ctree_move (BtkCTree     *ctree,
		BtkCTreeNode *node,
		BtkCTreeNode *new_parent,
		BtkCTreeNode *new_sibling)
{
  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (node != NULL);
  
  btk_signal_emit (BTK_OBJECT (ctree), ctree_signals[TREE_MOVE], node,
		   new_parent, new_sibling);
}

void
btk_ctree_expand (BtkCTree     *ctree,
		  BtkCTreeNode *node)
{
  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (node != NULL);
  
  if (BTK_CTREE_ROW (node)->is_leaf)
    return;

  btk_signal_emit (BTK_OBJECT (ctree), ctree_signals[TREE_EXPAND], node);
}

void 
btk_ctree_expand_recursive (BtkCTree     *ctree,
			    BtkCTreeNode *node)
{
  BtkCList *clist;
  bboolean thaw = FALSE;

  g_return_if_fail (BTK_IS_CTREE (ctree));

  clist = BTK_CLIST (ctree);

  if (node && BTK_CTREE_ROW (node)->is_leaf)
    return;

  if (CLIST_UNFROZEN (clist) && (!node || btk_ctree_is_viewable (ctree, node)))
    {
      btk_clist_freeze (clist);
      thaw = TRUE;
    }

  btk_ctree_post_recursive (ctree, node, BTK_CTREE_FUNC (tree_expand), NULL);

  if (thaw)
    btk_clist_thaw (clist);
}

void 
btk_ctree_expand_to_depth (BtkCTree     *ctree,
			   BtkCTreeNode *node,
			   bint          depth)
{
  BtkCList *clist;
  bboolean thaw = FALSE;

  g_return_if_fail (BTK_IS_CTREE (ctree));

  clist = BTK_CLIST (ctree);

  if (node && BTK_CTREE_ROW (node)->is_leaf)
    return;

  if (CLIST_UNFROZEN (clist) && (!node || btk_ctree_is_viewable (ctree, node)))
    {
      btk_clist_freeze (clist);
      thaw = TRUE;
    }

  btk_ctree_post_recursive_to_depth (ctree, node, depth,
				     BTK_CTREE_FUNC (tree_expand), NULL);

  if (thaw)
    btk_clist_thaw (clist);
}

void
btk_ctree_collapse (BtkCTree     *ctree,
		    BtkCTreeNode *node)
{
  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (node != NULL);
  
  if (BTK_CTREE_ROW (node)->is_leaf)
    return;

  btk_signal_emit (BTK_OBJECT (ctree), ctree_signals[TREE_COLLAPSE], node);
}

void 
btk_ctree_collapse_recursive (BtkCTree     *ctree,
			      BtkCTreeNode *node)
{
  BtkCList *clist;
  bboolean thaw = FALSE;
  bint i;

  g_return_if_fail (BTK_IS_CTREE (ctree));

  if (node && BTK_CTREE_ROW (node)->is_leaf)
    return;

  clist = BTK_CLIST (ctree);

  if (CLIST_UNFROZEN (clist) && (!node || btk_ctree_is_viewable (ctree, node)))
    {
      btk_clist_freeze (clist);
      thaw = TRUE;
    }

  BTK_CLIST_SET_FLAG (clist, CLIST_AUTO_RESIZE_BLOCKED);
  btk_ctree_post_recursive (ctree, node, BTK_CTREE_FUNC (tree_collapse), NULL);
  BTK_CLIST_UNSET_FLAG (clist, CLIST_AUTO_RESIZE_BLOCKED);
  for (i = 0; i < clist->columns; i++)
    if (clist->column[i].auto_resize)
      btk_clist_set_column_width (clist, i,
				  btk_clist_optimal_column_width (clist, i));

  if (thaw)
    btk_clist_thaw (clist);
}

void 
btk_ctree_collapse_to_depth (BtkCTree     *ctree,
			     BtkCTreeNode *node,
			     bint          depth)
{
  BtkCList *clist;
  bboolean thaw = FALSE;
  bint i;

  g_return_if_fail (BTK_IS_CTREE (ctree));

  if (node && BTK_CTREE_ROW (node)->is_leaf)
    return;

  clist = BTK_CLIST (ctree);

  if (CLIST_UNFROZEN (clist) && (!node || btk_ctree_is_viewable (ctree, node)))
    {
      btk_clist_freeze (clist);
      thaw = TRUE;
    }

  BTK_CLIST_SET_FLAG (clist, CLIST_AUTO_RESIZE_BLOCKED);
  btk_ctree_post_recursive_to_depth (ctree, node, depth,
				     BTK_CTREE_FUNC (tree_collapse_to_depth),
				     BINT_TO_POINTER (depth));
  BTK_CLIST_UNSET_FLAG (clist, CLIST_AUTO_RESIZE_BLOCKED);
  for (i = 0; i < clist->columns; i++)
    if (clist->column[i].auto_resize)
      btk_clist_set_column_width (clist, i,
				  btk_clist_optimal_column_width (clist, i));

  if (thaw)
    btk_clist_thaw (clist);
}

void
btk_ctree_toggle_expansion (BtkCTree     *ctree,
			    BtkCTreeNode *node)
{
  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (node != NULL);
  
  if (BTK_CTREE_ROW (node)->is_leaf)
    return;

  tree_toggle_expansion (ctree, node, NULL);
}

void 
btk_ctree_toggle_expansion_recursive (BtkCTree     *ctree,
				      BtkCTreeNode *node)
{
  BtkCList *clist;
  bboolean thaw = FALSE;

  g_return_if_fail (BTK_IS_CTREE (ctree));
  
  if (node && BTK_CTREE_ROW (node)->is_leaf)
    return;

  clist = BTK_CLIST (ctree);

  if (CLIST_UNFROZEN (clist) && (!node || btk_ctree_is_viewable (ctree, node)))
    {
      btk_clist_freeze (clist);
      thaw = TRUE;
    }
  
  btk_ctree_post_recursive (ctree, node,
			    BTK_CTREE_FUNC (tree_toggle_expansion), NULL);

  if (thaw)
    btk_clist_thaw (clist);
}

void
btk_ctree_select (BtkCTree     *ctree, 
		  BtkCTreeNode *node)
{
  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (node != NULL);

  if (BTK_CTREE_ROW (node)->row.selectable)
    btk_signal_emit (BTK_OBJECT (ctree), ctree_signals[TREE_SELECT_ROW],
		     node, -1);
}

void
btk_ctree_unselect (BtkCTree     *ctree, 
		    BtkCTreeNode *node)
{
  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (node != NULL);

  btk_signal_emit (BTK_OBJECT (ctree), ctree_signals[TREE_UNSELECT_ROW],
		   node, -1);
}

void
btk_ctree_select_recursive (BtkCTree     *ctree, 
			    BtkCTreeNode *node)
{
  btk_ctree_real_select_recursive (ctree, node, TRUE);
}

void
btk_ctree_unselect_recursive (BtkCTree     *ctree, 
			      BtkCTreeNode *node)
{
  btk_ctree_real_select_recursive (ctree, node, FALSE);
}

void
btk_ctree_real_select_recursive (BtkCTree     *ctree, 
				 BtkCTreeNode *node, 
				 bint          state)
{
  BtkCList *clist;
  bboolean thaw = FALSE;

  g_return_if_fail (BTK_IS_CTREE (ctree));

  clist = BTK_CLIST (ctree);

  if ((state && 
       (clist->selection_mode ==  BTK_SELECTION_BROWSE ||
	clist->selection_mode == BTK_SELECTION_SINGLE)) ||
      (!state && clist->selection_mode ==  BTK_SELECTION_BROWSE))
    return;

  if (CLIST_UNFROZEN (clist) && (!node || btk_ctree_is_viewable (ctree, node)))
    {
      btk_clist_freeze (clist);
      thaw = TRUE;
    }

  if (clist->selection_mode == BTK_SELECTION_MULTIPLE)
    {
      BTK_CLIST_GET_CLASS (clist)->resync_selection (clist, NULL);
      
      g_list_free (clist->undo_selection);
      g_list_free (clist->undo_unselection);
      clist->undo_selection = NULL;
      clist->undo_unselection = NULL;
    }

  if (state)
    btk_ctree_post_recursive (ctree, node,
			      BTK_CTREE_FUNC (tree_select), NULL);
  else 
    btk_ctree_post_recursive (ctree, node,
			      BTK_CTREE_FUNC (tree_unselect), NULL);
  
  if (thaw)
    btk_clist_thaw (clist);
}


/***********************************************************
 *           Analogons of BtkCList functions               *
 ***********************************************************/


void 
btk_ctree_node_set_text (BtkCTree     *ctree,
			 BtkCTreeNode *node,
			 bint          column,
			 const bchar  *text)
{
  BtkCList *clist;

  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (node != NULL);

  if (column < 0 || column >= BTK_CLIST (ctree)->columns)
    return;
  
  clist = BTK_CLIST (ctree);

  BTK_CLIST_GET_CLASS (clist)->set_cell_contents
    (clist, &(BTK_CTREE_ROW (node)->row), column, BTK_CELL_TEXT,
     text, 0, NULL, NULL);

  tree_draw_node (ctree, node);
}


/**
 * btk_ctree_node_set_pixmap:
 * @mask: (allow-none):
 */
void
btk_ctree_node_set_pixmap (BtkCTree     *ctree,
			   BtkCTreeNode *node,
			   bint          column,
			   BdkPixmap    *pixmap,
			   BdkBitmap    *mask)
{
  BtkCList *clist;

  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (node != NULL);
  g_return_if_fail (pixmap != NULL);

  if (column < 0 || column >= BTK_CLIST (ctree)->columns)
    return;

  g_object_ref (pixmap);
  if (mask) 
    g_object_ref (mask);

  clist = BTK_CLIST (ctree);

  BTK_CLIST_GET_CLASS (clist)->set_cell_contents
    (clist, &(BTK_CTREE_ROW (node)->row), column, BTK_CELL_PIXMAP,
     NULL, 0, pixmap, mask);

  tree_draw_node (ctree, node);
}


/**
 * btk_ctree_node_set_pixtext:
 * @mask: (allow-none):
 */
void
btk_ctree_node_set_pixtext (BtkCTree     *ctree,
			    BtkCTreeNode *node,
			    bint          column,
			    const bchar  *text,
			    buint8        spacing,
			    BdkPixmap    *pixmap,
			    BdkBitmap    *mask)
{
  BtkCList *clist;

  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (node != NULL);
  if (column != ctree->tree_column)
    g_return_if_fail (pixmap != NULL);
  if (column < 0 || column >= BTK_CLIST (ctree)->columns)
    return;

  clist = BTK_CLIST (ctree);

  if (pixmap)
    {
      g_object_ref (pixmap);
      if (mask) 
	g_object_ref (mask);
    }

  BTK_CLIST_GET_CLASS (clist)->set_cell_contents
    (clist, &(BTK_CTREE_ROW (node)->row), column, BTK_CELL_PIXTEXT,
     text, spacing, pixmap, mask);

  tree_draw_node (ctree, node);
}


/**
 * btk_ctree_set_node_info:
 * @pixmap_closed: (allow-none):
 * @mask_closed: (allow-none):
 * @pixmap_opened: (allow-none):
 * @mask_opened: (allow-none):
 */
void
btk_ctree_set_node_info (BtkCTree     *ctree,
			 BtkCTreeNode *node,
			 const bchar  *text,
			 buint8        spacing,
			 BdkPixmap    *pixmap_closed,
			 BdkBitmap    *mask_closed,
			 BdkPixmap    *pixmap_opened,
			 BdkBitmap    *mask_opened,
			 bboolean      is_leaf,
			 bboolean      expanded)
{
  bboolean old_leaf;
  bboolean old_expanded;
 
  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (node != NULL);

  old_leaf = BTK_CTREE_ROW (node)->is_leaf;
  old_expanded = BTK_CTREE_ROW (node)->expanded;

  if (is_leaf && BTK_CTREE_ROW (node)->children)
    {
      BtkCTreeNode *work;
      BtkCTreeNode *ptr;
      
      work = BTK_CTREE_ROW (node)->children;
      while (work)
	{
	  ptr = work;
	  work = BTK_CTREE_ROW (work)->sibling;
	  btk_ctree_remove_node (ctree, ptr);
	}
    }

  set_node_info (ctree, node, text, spacing, pixmap_closed, mask_closed,
		 pixmap_opened, mask_opened, is_leaf, expanded);

  if (!is_leaf && !old_leaf)
    {
      BTK_CTREE_ROW (node)->expanded = old_expanded;
      if (expanded && !old_expanded)
	btk_ctree_expand (ctree, node);
      else if (!expanded && old_expanded)
	btk_ctree_collapse (ctree, node);
    }

  BTK_CTREE_ROW (node)->expanded = (is_leaf) ? FALSE : expanded;
  
  tree_draw_node (ctree, node);
}

void
btk_ctree_node_set_shift (BtkCTree     *ctree,
			  BtkCTreeNode *node,
			  bint          column,
			  bint          vertical,
			  bint          horizontal)
{
  BtkCList *clist;
  BtkRequisition requisition;
  bboolean visible = FALSE;

  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (node != NULL);

  if (column < 0 || column >= BTK_CLIST (ctree)->columns)
    return;

  clist = BTK_CLIST (ctree);

  if (clist->column[column].auto_resize &&
      !BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    {
      visible = btk_ctree_is_viewable (ctree, node);
      if (visible)
	BTK_CLIST_GET_CLASS (clist)->cell_size_request
	  (clist, &BTK_CTREE_ROW (node)->row, column, &requisition);
    }

  BTK_CTREE_ROW (node)->row.cell[column].vertical   = vertical;
  BTK_CTREE_ROW (node)->row.cell[column].horizontal = horizontal;

  if (visible)
    column_auto_resize (clist, &BTK_CTREE_ROW (node)->row,
			column, requisition.width);

  tree_draw_node (ctree, node);
}

static void
remove_grab (BtkCList *clist)
{
  if (bdk_display_pointer_is_grabbed (btk_widget_get_display (BTK_WIDGET (clist))) && 
      BTK_WIDGET_HAS_GRAB (clist))
    {
      btk_grab_remove (BTK_WIDGET (clist));
      bdk_display_pointer_ungrab (btk_widget_get_display (BTK_WIDGET (clist)),
				  BDK_CURRENT_TIME);
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

void
btk_ctree_node_set_selectable (BtkCTree     *ctree,
			       BtkCTreeNode *node,
			       bboolean      selectable)
{
  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (node != NULL);

  if (selectable == BTK_CTREE_ROW (node)->row.selectable)
    return;

  BTK_CTREE_ROW (node)->row.selectable = selectable;

  if (!selectable && BTK_CTREE_ROW (node)->row.state == BTK_STATE_SELECTED)
    {
      BtkCList *clist;

      clist = BTK_CLIST (ctree);

      if (clist->anchor >= 0 &&
	  clist->selection_mode == BTK_SELECTION_MULTIPLE)
	{
	  clist->drag_button = 0;
	  remove_grab (clist);

	  BTK_CLIST_GET_CLASS (clist)->resync_selection (clist, NULL);
	}
      btk_ctree_unselect (ctree, node);
    }      
}

bboolean
btk_ctree_node_get_selectable (BtkCTree     *ctree,
			       BtkCTreeNode *node)
{
  g_return_val_if_fail (node != NULL, FALSE);

  return BTK_CTREE_ROW (node)->row.selectable;
}

BtkCellType 
btk_ctree_node_get_cell_type (BtkCTree     *ctree,
			      BtkCTreeNode *node,
			      bint          column)
{
  g_return_val_if_fail (BTK_IS_CTREE (ctree), -1);
  g_return_val_if_fail (node != NULL, -1);

  if (column < 0 || column >= BTK_CLIST (ctree)->columns)
    return -1;

  return BTK_CTREE_ROW (node)->row.cell[column].type;
}

bboolean
btk_ctree_node_get_text (BtkCTree      *ctree,
			 BtkCTreeNode  *node,
			 bint           column,
			 bchar        **text)
{
  g_return_val_if_fail (BTK_IS_CTREE (ctree), FALSE);
  g_return_val_if_fail (node != NULL, FALSE);

  if (column < 0 || column >= BTK_CLIST (ctree)->columns)
    return FALSE;

  if (BTK_CTREE_ROW (node)->row.cell[column].type != BTK_CELL_TEXT)
    return FALSE;

  if (text)
    *text = BTK_CELL_TEXT (BTK_CTREE_ROW (node)->row.cell[column])->text;

  return TRUE;
}

bboolean
btk_ctree_node_get_pixmap (BtkCTree     *ctree,
			   BtkCTreeNode *node,
			   bint          column,
			   BdkPixmap   **pixmap,
			   BdkBitmap   **mask)
{
  g_return_val_if_fail (BTK_IS_CTREE (ctree), FALSE);
  g_return_val_if_fail (node != NULL, FALSE);

  if (column < 0 || column >= BTK_CLIST (ctree)->columns)
    return FALSE;

  if (BTK_CTREE_ROW (node)->row.cell[column].type != BTK_CELL_PIXMAP)
    return FALSE;

  if (pixmap)
    *pixmap = BTK_CELL_PIXMAP (BTK_CTREE_ROW (node)->row.cell[column])->pixmap;
  if (mask)
    *mask = BTK_CELL_PIXMAP (BTK_CTREE_ROW (node)->row.cell[column])->mask;

  return TRUE;
}

bboolean
btk_ctree_node_get_pixtext (BtkCTree      *ctree,
			    BtkCTreeNode  *node,
			    bint           column,
			    bchar        **text,
			    buint8        *spacing,
			    BdkPixmap    **pixmap,
			    BdkBitmap    **mask)
{
  g_return_val_if_fail (BTK_IS_CTREE (ctree), FALSE);
  g_return_val_if_fail (node != NULL, FALSE);
  
  if (column < 0 || column >= BTK_CLIST (ctree)->columns)
    return FALSE;
  
  if (BTK_CTREE_ROW (node)->row.cell[column].type != BTK_CELL_PIXTEXT)
    return FALSE;
  
  if (text)
    *text = BTK_CELL_PIXTEXT (BTK_CTREE_ROW (node)->row.cell[column])->text;
  if (spacing)
    *spacing = BTK_CELL_PIXTEXT (BTK_CTREE_ROW 
				 (node)->row.cell[column])->spacing;
  if (pixmap)
    *pixmap = BTK_CELL_PIXTEXT (BTK_CTREE_ROW 
				(node)->row.cell[column])->pixmap;
  if (mask)
    *mask = BTK_CELL_PIXTEXT (BTK_CTREE_ROW (node)->row.cell[column])->mask;
  
  return TRUE;
}

bboolean
btk_ctree_get_node_info (BtkCTree      *ctree,
			 BtkCTreeNode  *node,
			 bchar        **text,
			 buint8        *spacing,
			 BdkPixmap    **pixmap_closed,
			 BdkBitmap    **mask_closed,
			 BdkPixmap    **pixmap_opened,
			 BdkBitmap    **mask_opened,
			 bboolean      *is_leaf,
			 bboolean      *expanded)
{
  g_return_val_if_fail (BTK_IS_CTREE (ctree), FALSE);
  g_return_val_if_fail (node != NULL, FALSE);
  
  if (text)
    *text = BTK_CELL_PIXTEXT 
      (BTK_CTREE_ROW (node)->row.cell[ctree->tree_column])->text;
  if (spacing)
    *spacing = BTK_CELL_PIXTEXT 
      (BTK_CTREE_ROW (node)->row.cell[ctree->tree_column])->spacing;
  if (pixmap_closed)
    *pixmap_closed = BTK_CTREE_ROW (node)->pixmap_closed;
  if (mask_closed)
    *mask_closed = BTK_CTREE_ROW (node)->mask_closed;
  if (pixmap_opened)
    *pixmap_opened = BTK_CTREE_ROW (node)->pixmap_opened;
  if (mask_opened)
    *mask_opened = BTK_CTREE_ROW (node)->mask_opened;
  if (is_leaf)
    *is_leaf = BTK_CTREE_ROW (node)->is_leaf;
  if (expanded)
    *expanded = BTK_CTREE_ROW (node)->expanded;
  
  return TRUE;
}

void
btk_ctree_node_set_cell_style (BtkCTree     *ctree,
			       BtkCTreeNode *node,
			       bint          column,
			       BtkStyle     *style)
{
  BtkCList *clist;
  BtkRequisition requisition;
  bboolean visible = FALSE;

  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (node != NULL);

  clist = BTK_CLIST (ctree);

  if (column < 0 || column >= clist->columns)
    return;

  if (BTK_CTREE_ROW (node)->row.cell[column].style == style)
    return;

  if (clist->column[column].auto_resize &&
      !BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    {
      visible = btk_ctree_is_viewable (ctree, node);
      if (visible)
	BTK_CLIST_GET_CLASS (clist)->cell_size_request
	  (clist, &BTK_CTREE_ROW (node)->row, column, &requisition);
    }

  if (BTK_CTREE_ROW (node)->row.cell[column].style)
    {
      if (btk_widget_get_realized (BTK_WIDGET (ctree)))
        btk_style_detach (BTK_CTREE_ROW (node)->row.cell[column].style);
      g_object_unref (BTK_CTREE_ROW (node)->row.cell[column].style);
    }

  BTK_CTREE_ROW (node)->row.cell[column].style = style;

  if (BTK_CTREE_ROW (node)->row.cell[column].style)
    {
      g_object_ref (BTK_CTREE_ROW (node)->row.cell[column].style);
      
      if (btk_widget_get_realized (BTK_WIDGET (ctree)))
        BTK_CTREE_ROW (node)->row.cell[column].style =
	  btk_style_attach (BTK_CTREE_ROW (node)->row.cell[column].style,
			    clist->clist_window);
    }

  if (visible)
    column_auto_resize (clist, &BTK_CTREE_ROW (node)->row, column,
			requisition.width);

  tree_draw_node (ctree, node);
}

BtkStyle *
btk_ctree_node_get_cell_style (BtkCTree     *ctree,
			       BtkCTreeNode *node,
			       bint          column)
{
  g_return_val_if_fail (BTK_IS_CTREE (ctree), NULL);
  g_return_val_if_fail (node != NULL, NULL);

  if (column < 0 || column >= BTK_CLIST (ctree)->columns)
    return NULL;

  return BTK_CTREE_ROW (node)->row.cell[column].style;
}

void
btk_ctree_node_set_row_style (BtkCTree     *ctree,
			      BtkCTreeNode *node,
			      BtkStyle     *style)
{
  BtkCList *clist;
  BtkRequisition requisition;
  bboolean visible;
  bint *old_width = NULL;
  bint i;

  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (node != NULL);

  clist = BTK_CLIST (ctree);

  if (BTK_CTREE_ROW (node)->row.style == style)
    return;
  
  visible = btk_ctree_is_viewable (ctree, node);
  if (visible && !BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    {
      old_width = g_new (bint, clist->columns);
      for (i = 0; i < clist->columns; i++)
	if (clist->column[i].auto_resize)
	  {
	    BTK_CLIST_GET_CLASS (clist)->cell_size_request
	      (clist, &BTK_CTREE_ROW (node)->row, i, &requisition);
	    old_width[i] = requisition.width;
	  }
    }

  if (BTK_CTREE_ROW (node)->row.style)
    {
      if (btk_widget_get_realized (BTK_WIDGET (ctree)))
        btk_style_detach (BTK_CTREE_ROW (node)->row.style);
      g_object_unref (BTK_CTREE_ROW (node)->row.style);
    }

  BTK_CTREE_ROW (node)->row.style = style;

  if (BTK_CTREE_ROW (node)->row.style)
    {
      g_object_ref (BTK_CTREE_ROW (node)->row.style);
      
      if (btk_widget_get_realized (BTK_WIDGET (ctree)))
        BTK_CTREE_ROW (node)->row.style =
	  btk_style_attach (BTK_CTREE_ROW (node)->row.style,
			    clist->clist_window);
    }

  if (visible && !BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    {
      for (i = 0; i < clist->columns; i++)
	if (clist->column[i].auto_resize)
	  column_auto_resize (clist, &BTK_CTREE_ROW (node)->row, i,
			      old_width[i]);
      g_free (old_width);
    }
  tree_draw_node (ctree, node);
}

BtkStyle *
btk_ctree_node_get_row_style (BtkCTree     *ctree,
			      BtkCTreeNode *node)
{
  g_return_val_if_fail (BTK_IS_CTREE (ctree), NULL);
  g_return_val_if_fail (node != NULL, NULL);

  return BTK_CTREE_ROW (node)->row.style;
}

void
btk_ctree_node_set_foreground (BtkCTree       *ctree,
			       BtkCTreeNode   *node,
			       const BdkColor *color)
{
  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (node != NULL);

  if (color)
    {
      BTK_CTREE_ROW (node)->row.foreground = *color;
      BTK_CTREE_ROW (node)->row.fg_set = TRUE;
      if (btk_widget_get_realized (BTK_WIDGET (ctree)))
	bdk_colormap_alloc_color (btk_widget_get_colormap (BTK_WIDGET (ctree)),
                                  &BTK_CTREE_ROW (node)->row.foreground,
                                  FALSE, TRUE);
    }
  else
    BTK_CTREE_ROW (node)->row.fg_set = FALSE;

  tree_draw_node (ctree, node);
}

void
btk_ctree_node_set_background (BtkCTree       *ctree,
			       BtkCTreeNode   *node,
			       const BdkColor *color)
{
  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (node != NULL);

  if (color)
    {
      BTK_CTREE_ROW (node)->row.background = *color;
      BTK_CTREE_ROW (node)->row.bg_set = TRUE;
      if (btk_widget_get_realized (BTK_WIDGET (ctree)))
	bdk_colormap_alloc_color (btk_widget_get_colormap (BTK_WIDGET (ctree)),
                                  &BTK_CTREE_ROW (node)->row.background,
                                  FALSE, TRUE);
    }
  else
    BTK_CTREE_ROW (node)->row.bg_set = FALSE;

  tree_draw_node (ctree, node);
}

void
btk_ctree_node_set_row_data (BtkCTree     *ctree,
			     BtkCTreeNode *node,
			     bpointer      data)
{
  btk_ctree_node_set_row_data_full (ctree, node, data, NULL);
}

void
btk_ctree_node_set_row_data_full (BtkCTree       *ctree,
				  BtkCTreeNode   *node,
				  bpointer        data,
				  GDestroyNotify  destroy)
{
  GDestroyNotify dnotify;
  bpointer ddata;
  
  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (node != NULL);

  dnotify = BTK_CTREE_ROW (node)->row.destroy;
  ddata = BTK_CTREE_ROW (node)->row.data;
  
  BTK_CTREE_ROW (node)->row.data = data;
  BTK_CTREE_ROW (node)->row.destroy = destroy;

  if (dnotify)
    dnotify (ddata);
}

bpointer
btk_ctree_node_get_row_data (BtkCTree     *ctree,
			     BtkCTreeNode *node)
{
  g_return_val_if_fail (BTK_IS_CTREE (ctree), NULL);

  return node ? BTK_CTREE_ROW (node)->row.data : NULL;
}

void
btk_ctree_node_moveto (BtkCTree     *ctree,
		       BtkCTreeNode *node,
		       bint          column,
		       bfloat        row_align,
		       bfloat        col_align)
{
  bint row = -1;
  BtkCList *clist;

  g_return_if_fail (BTK_IS_CTREE (ctree));

  clist = BTK_CLIST (ctree);

  while (node && !btk_ctree_is_viewable (ctree, node))
    node = BTK_CTREE_ROW (node)->parent;

  if (node)
    row = g_list_position (clist->row_list, (GList *)node);
  
  btk_clist_moveto (clist, row, column, row_align, col_align);
}

BtkVisibility 
btk_ctree_node_is_visible (BtkCTree     *ctree,
                           BtkCTreeNode *node)
{
  bint row;
  
  g_return_val_if_fail (ctree != NULL, 0);
  g_return_val_if_fail (node != NULL, 0);
  
  row = g_list_position (BTK_CLIST (ctree)->row_list, (GList*) node);
  return btk_clist_row_is_visible (BTK_CLIST (ctree), row);
}


/***********************************************************
 *             BtkCTree specific functions                 *
 ***********************************************************/

void
btk_ctree_set_indent (BtkCTree *ctree, 
                      bint      indent)
{
  BtkCList *clist;

  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (indent >= 0);

  if (indent == ctree->tree_indent)
    return;

  clist = BTK_CLIST (ctree);
  ctree->tree_indent = indent;

  if (clist->column[ctree->tree_column].auto_resize &&
      !BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    btk_clist_set_column_width
      (clist, ctree->tree_column,
       btk_clist_optimal_column_width (clist, ctree->tree_column));
  else
    CLIST_REFRESH (ctree);
}

void
btk_ctree_set_spacing (BtkCTree *ctree, 
		       bint      spacing)
{
  BtkCList *clist;
  bint old_spacing;

  g_return_if_fail (BTK_IS_CTREE (ctree));
  g_return_if_fail (spacing >= 0);

  if (spacing == ctree->tree_spacing)
    return;

  clist = BTK_CLIST (ctree);

  old_spacing = ctree->tree_spacing;
  ctree->tree_spacing = spacing;

  if (clist->column[ctree->tree_column].auto_resize &&
      !BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    btk_clist_set_column_width (clist, ctree->tree_column,
				clist->column[ctree->tree_column].width +
				spacing - old_spacing);
  else
    CLIST_REFRESH (ctree);
}

void
btk_ctree_set_show_stub (BtkCTree *ctree, 
			 bboolean  show_stub)
{
  g_return_if_fail (BTK_IS_CTREE (ctree));

  show_stub = show_stub != FALSE;

  if (show_stub != ctree->show_stub)
    {
      BtkCList *clist;

      clist = BTK_CLIST (ctree);
      ctree->show_stub = show_stub;

      if (CLIST_UNFROZEN (clist) && clist->rows &&
	  btk_clist_row_is_visible (clist, 0) != BTK_VISIBILITY_NONE)
	BTK_CLIST_GET_CLASS (clist)->draw_row
	  (clist, NULL, 0, BTK_CLIST_ROW (clist->row_list));
    }
}

void 
btk_ctree_set_line_style (BtkCTree          *ctree, 
			  BtkCTreeLineStyle  line_style)
{
  BtkCList *clist;
  BtkCTreeLineStyle old_style;

  g_return_if_fail (BTK_IS_CTREE (ctree));

  if (line_style == ctree->line_style)
    return;

  clist = BTK_CLIST (ctree);

  old_style = ctree->line_style;
  ctree->line_style = line_style;

  if (clist->column[ctree->tree_column].auto_resize &&
      !BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    {
      if (old_style == BTK_CTREE_LINES_TABBED)
	btk_clist_set_column_width
	  (clist, ctree->tree_column,
	   clist->column[ctree->tree_column].width - 3);
      else if (line_style == BTK_CTREE_LINES_TABBED)
	btk_clist_set_column_width
	  (clist, ctree->tree_column,
	   clist->column[ctree->tree_column].width + 3);
    }

  if (btk_widget_get_realized (BTK_WIDGET (ctree)))
    {
      bint8 dashes[] = { 1, 1 };

      switch (line_style)
	{
	case BTK_CTREE_LINES_SOLID:
	  if (btk_widget_get_realized (BTK_WIDGET (ctree)))
	    bdk_gc_set_line_attributes (ctree->lines_gc, 1, BDK_LINE_SOLID, 
					BDK_CAP_BUTT, BDK_JOIN_MITER);
	  break;
	case BTK_CTREE_LINES_DOTTED:
	  if (btk_widget_get_realized (BTK_WIDGET (ctree)))
	    bdk_gc_set_line_attributes (ctree->lines_gc, 1, 
					BDK_LINE_ON_OFF_DASH, BDK_CAP_BUTT, BDK_JOIN_MITER);
	  bdk_gc_set_dashes (ctree->lines_gc, 0, dashes, G_N_ELEMENTS (dashes));
	  break;
	case BTK_CTREE_LINES_TABBED:
	  if (btk_widget_get_realized (BTK_WIDGET (ctree)))
	    bdk_gc_set_line_attributes (ctree->lines_gc, 1, BDK_LINE_SOLID, 
					BDK_CAP_BUTT, BDK_JOIN_MITER);
	  break;
	case BTK_CTREE_LINES_NONE:
	  break;
	default:
	  return;
	}
      CLIST_REFRESH (ctree);
    }
}

void 
btk_ctree_set_expander_style (BtkCTree              *ctree, 
			      BtkCTreeExpanderStyle  expander_style)
{
  BtkCList *clist;
  BtkCTreeExpanderStyle old_style;

  g_return_if_fail (BTK_IS_CTREE (ctree));

  if (expander_style == ctree->expander_style)
    return;

  clist = BTK_CLIST (ctree);

  old_style = ctree->expander_style;
  ctree->expander_style = expander_style;

  if (clist->column[ctree->tree_column].auto_resize &&
      !BTK_CLIST_AUTO_RESIZE_BLOCKED (clist))
    {
      bint new_width;

      new_width = clist->column[ctree->tree_column].width;
      switch (old_style)
	{
	case BTK_CTREE_EXPANDER_NONE:
	  break;
	case BTK_CTREE_EXPANDER_TRIANGLE:
	  new_width -= PM_SIZE + 3;
	  break;
	case BTK_CTREE_EXPANDER_SQUARE:
	case BTK_CTREE_EXPANDER_CIRCULAR:
	  new_width -= PM_SIZE + 1;
	  break;
	}

      switch (expander_style)
	{
	case BTK_CTREE_EXPANDER_NONE:
	  break;
	case BTK_CTREE_EXPANDER_TRIANGLE:
	  new_width += PM_SIZE + 3;
	  break;
	case BTK_CTREE_EXPANDER_SQUARE:
	case BTK_CTREE_EXPANDER_CIRCULAR:
	  new_width += PM_SIZE + 1;
	  break;
	}

      btk_clist_set_column_width (clist, ctree->tree_column, new_width);
    }

  if (BTK_WIDGET_DRAWABLE (clist))
    CLIST_REFRESH (clist);
}


/***********************************************************
 *             Tree sorting functions                      *
 ***********************************************************/


static void
tree_sort (BtkCTree     *ctree,
	   BtkCTreeNode *node,
	   bpointer      data)
{
  BtkCTreeNode *list_start;
  BtkCTreeNode *cmp;
  BtkCTreeNode *work;
  BtkCList *clist;

  clist = BTK_CLIST (ctree);

  if (node)
    list_start = BTK_CTREE_ROW (node)->children;
  else
    list_start = BTK_CTREE_NODE (clist->row_list);

  while (list_start)
    {
      cmp = list_start;
      work = BTK_CTREE_ROW (cmp)->sibling;
      while (work)
	{
	  if (clist->sort_type == BTK_SORT_ASCENDING)
	    {
	      if (clist->compare 
		  (clist, BTK_CTREE_ROW (work), BTK_CTREE_ROW (cmp)) < 0)
		cmp = work;
	    }
	  else
	    {
	      if (clist->compare 
		  (clist, BTK_CTREE_ROW (work), BTK_CTREE_ROW (cmp)) > 0)
		cmp = work;
	    }
	  work = BTK_CTREE_ROW (work)->sibling;
	}
      if (cmp == list_start)
	list_start = BTK_CTREE_ROW (cmp)->sibling;
      else
	{
	  btk_ctree_unlink (ctree, cmp, FALSE);
	  btk_ctree_link (ctree, cmp, node, list_start, FALSE);
	}
    }
}

void
btk_ctree_sort_recursive (BtkCTree     *ctree, 
			  BtkCTreeNode *node)
{
  BtkCList *clist;
  BtkCTreeNode *focus_node = NULL;

  g_return_if_fail (BTK_IS_CTREE (ctree));

  clist = BTK_CLIST (ctree);

  btk_clist_freeze (clist);

  if (clist->selection_mode == BTK_SELECTION_MULTIPLE)
    {
      BTK_CLIST_GET_CLASS (clist)->resync_selection (clist, NULL);
      
      g_list_free (clist->undo_selection);
      g_list_free (clist->undo_unselection);
      clist->undo_selection = NULL;
      clist->undo_unselection = NULL;
    }

  if (!node || (node && btk_ctree_is_viewable (ctree, node)))
    focus_node =
      BTK_CTREE_NODE (g_list_nth (clist->row_list, clist->focus_row));
      
  btk_ctree_post_recursive (ctree, node, BTK_CTREE_FUNC (tree_sort), NULL);

  if (!node)
    tree_sort (ctree, NULL, NULL);

  if (focus_node)
    {
      clist->focus_row = g_list_position (clist->row_list,(GList *)focus_node);
      clist->undo_anchor = clist->focus_row;
    }

  btk_clist_thaw (clist);
}

static void
real_sort_list (BtkCList *clist)
{
  btk_ctree_sort_recursive (BTK_CTREE (clist), NULL);
}

void
btk_ctree_sort_node (BtkCTree     *ctree, 
		     BtkCTreeNode *node)
{
  BtkCList *clist;
  BtkCTreeNode *focus_node = NULL;

  g_return_if_fail (BTK_IS_CTREE (ctree));

  clist = BTK_CLIST (ctree);

  btk_clist_freeze (clist);

  if (clist->selection_mode == BTK_SELECTION_MULTIPLE)
    {
      BTK_CLIST_GET_CLASS (clist)->resync_selection (clist, NULL);
      
      g_list_free (clist->undo_selection);
      g_list_free (clist->undo_unselection);
      clist->undo_selection = NULL;
      clist->undo_unselection = NULL;
    }

  if (!node || (node && btk_ctree_is_viewable (ctree, node)))
    focus_node = BTK_CTREE_NODE
      (g_list_nth (clist->row_list, clist->focus_row));

  tree_sort (ctree, node, NULL);

  if (focus_node)
    {
      clist->focus_row = g_list_position (clist->row_list,(GList *)focus_node);
      clist->undo_anchor = clist->focus_row;
    }

  btk_clist_thaw (clist);
}

/************************************************************************/

static void
fake_unselect_all (BtkCList *clist,
		   bint      row)
{
  GList *list;
  GList *focus_node = NULL;

  if (row >= 0 && (focus_node = g_list_nth (clist->row_list, row)))
    {
      if (BTK_CTREE_ROW (focus_node)->row.state == BTK_STATE_NORMAL &&
	  BTK_CTREE_ROW (focus_node)->row.selectable)
	{
	  BTK_CTREE_ROW (focus_node)->row.state = BTK_STATE_SELECTED;
	  
	  if (CLIST_UNFROZEN (clist) &&
	      btk_clist_row_is_visible (clist, row) != BTK_VISIBILITY_NONE)
	    BTK_CLIST_GET_CLASS (clist)->draw_row (clist, NULL, row,
						  BTK_CLIST_ROW (focus_node));
	}  
    }

  clist->undo_selection = clist->selection;
  clist->selection = NULL;
  clist->selection_end = NULL;
  
  for (list = clist->undo_selection; list; list = list->next)
    {
      if (list->data == focus_node)
	continue;

      BTK_CTREE_ROW ((GList *)(list->data))->row.state = BTK_STATE_NORMAL;
      tree_draw_node (BTK_CTREE (clist), BTK_CTREE_NODE (list->data));
    }
}

static GList *
selection_find (BtkCList *clist,
		bint      row_number,
		GList    *row_list_element)
{
  return g_list_find (clist->selection, row_list_element);
}

static void
resync_selection (BtkCList *clist, BdkEvent *event)
{
  BtkCTree *ctree;
  GList *list;
  BtkCTreeNode *node;
  bint i;
  bint e;
  bint row;
  bboolean unselect;

  g_return_if_fail (BTK_IS_CTREE (clist));

  if (clist->selection_mode != BTK_SELECTION_MULTIPLE)
    return;

  if (clist->anchor < 0 || clist->drag_pos < 0)
    return;

  ctree = BTK_CTREE (clist);
  
  clist->freeze_count++;

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
	  node = list->data;
	  list = list->next;
	  
	  unselect = TRUE;

	  if (btk_ctree_is_viewable (ctree, node))
	    {
	      row = g_list_position (clist->row_list, (GList *)node);
	      if (row >= i && row <= e)
		unselect = FALSE;
	    }
	  if (unselect && BTK_CTREE_ROW (node)->row.selectable)
	    {
	      BTK_CTREE_ROW (node)->row.state = BTK_STATE_SELECTED;
	      btk_ctree_unselect (ctree, node);
	      clist->undo_selection = g_list_prepend (clist->undo_selection,
						      node);
	    }
	}
    }    

  if (clist->anchor < clist->drag_pos)
    {
      for (node = BTK_CTREE_NODE (g_list_nth (clist->row_list, i)); i <= e;
	   i++, node = BTK_CTREE_NODE_NEXT (node))
	if (BTK_CTREE_ROW (node)->row.selectable)
	  {
	    if (g_list_find (clist->selection, node))
	      {
		if (BTK_CTREE_ROW (node)->row.state == BTK_STATE_NORMAL)
		  {
		    BTK_CTREE_ROW (node)->row.state = BTK_STATE_SELECTED;
		    btk_ctree_unselect (ctree, node);
		    clist->undo_selection =
		      g_list_prepend (clist->undo_selection, node);
		  }
	      }
	    else if (BTK_CTREE_ROW (node)->row.state == BTK_STATE_SELECTED)
	      {
		BTK_CTREE_ROW (node)->row.state = BTK_STATE_NORMAL;
		clist->undo_unselection =
		  g_list_prepend (clist->undo_unselection, node);
	      }
	  }
    }
  else
    {
      for (node = BTK_CTREE_NODE (g_list_nth (clist->row_list, e)); i <= e;
	   e--, node = BTK_CTREE_NODE_PREV (node))
	if (BTK_CTREE_ROW (node)->row.selectable)
	  {
	    if (g_list_find (clist->selection, node))
	      {
		if (BTK_CTREE_ROW (node)->row.state == BTK_STATE_NORMAL)
		  {
		    BTK_CTREE_ROW (node)->row.state = BTK_STATE_SELECTED;
		    btk_ctree_unselect (ctree, node);
		    clist->undo_selection =
		      g_list_prepend (clist->undo_selection, node);
		  }
	      }
	    else if (BTK_CTREE_ROW (node)->row.state == BTK_STATE_SELECTED)
	      {
		BTK_CTREE_ROW (node)->row.state = BTK_STATE_NORMAL;
		clist->undo_unselection =
		  g_list_prepend (clist->undo_unselection, node);
	      }
	  }
    }

  clist->undo_unselection = g_list_reverse (clist->undo_unselection);
  for (list = clist->undo_unselection; list; list = list->next)
    btk_ctree_select (ctree, list->data);

  clist->anchor = -1;
  clist->drag_pos = -1;

  if (!CLIST_UNFROZEN (clist))
    clist->freeze_count--;
}

static void
real_undo_selection (BtkCList *clist)
{
  BtkCTree *ctree;
  GList *work;

  g_return_if_fail (BTK_IS_CTREE (clist));

  if (clist->selection_mode != BTK_SELECTION_MULTIPLE)
    return;

  if (!(clist->undo_selection || clist->undo_unselection))
    {
      btk_clist_unselect_all (clist);
      return;
    }

  ctree = BTK_CTREE (clist);

  for (work = clist->undo_selection; work; work = work->next)
    if (BTK_CTREE_ROW (work->data)->row.selectable)
      btk_ctree_select (ctree, BTK_CTREE_NODE (work->data));

  for (work = clist->undo_unselection; work; work = work->next)
    if (BTK_CTREE_ROW (work->data)->row.selectable)
      btk_ctree_unselect (ctree, BTK_CTREE_NODE (work->data));

  if (btk_widget_has_focus (BTK_WIDGET (clist)) && clist->focus_row != clist->undo_anchor)
    {
      clist->focus_row = clist->undo_anchor;
      btk_widget_queue_draw (BTK_WIDGET (clist));
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

void
btk_ctree_set_drag_compare_func (BtkCTree                *ctree,
				 BtkCTreeCompareDragFunc  cmp_func)
{
  g_return_if_fail (BTK_IS_CTREE (ctree));

  ctree->drag_compare = cmp_func;
}

static bboolean
check_drag (BtkCTree        *ctree,
	    BtkCTreeNode    *drag_source,
	    BtkCTreeNode    *drag_target,
	    BtkCListDragPos  insert_pos)
{
  g_return_val_if_fail (BTK_IS_CTREE (ctree), FALSE);

  if (drag_source && drag_source != drag_target &&
      (!BTK_CTREE_ROW (drag_source)->children ||
       !btk_ctree_is_ancestor (ctree, drag_source, drag_target)))
    {
      switch (insert_pos)
	{
	case BTK_CLIST_DRAG_NONE:
	  return FALSE;
	case BTK_CLIST_DRAG_AFTER:
	  if (BTK_CTREE_ROW (drag_target)->sibling != drag_source)
	    return (!ctree->drag_compare ||
		    ctree->drag_compare (ctree,
					 drag_source,
					 BTK_CTREE_ROW (drag_target)->parent,
					 BTK_CTREE_ROW (drag_target)->sibling));
	  break;
	case BTK_CLIST_DRAG_BEFORE:
	  if (BTK_CTREE_ROW (drag_source)->sibling != drag_target)
	    return (!ctree->drag_compare ||
		    ctree->drag_compare (ctree,
					 drag_source,
					 BTK_CTREE_ROW (drag_target)->parent,
					 drag_target));
	  break;
	case BTK_CLIST_DRAG_INTO:
	  if (!BTK_CTREE_ROW (drag_target)->is_leaf &&
	      BTK_CTREE_ROW (drag_target)->children != drag_source)
	    return (!ctree->drag_compare ||
		    ctree->drag_compare (ctree,
					 drag_source,
					 drag_target,
					 BTK_CTREE_ROW (drag_target)->children));
	  break;
	}
    }
  return FALSE;
}



/************************************/
static void
drag_dest_info_destroy (bpointer data)
{
  BtkCListDestInfo *info = data;

  g_free (info);
}

static void
drag_dest_cell (BtkCList         *clist,
		bint              x,
		bint              y,
		BtkCListDestInfo *dest_info)
{
  BtkWidget *widget;

  widget = BTK_WIDGET (clist);

  dest_info->insert_pos = BTK_CLIST_DRAG_NONE;

  y -= (BTK_CONTAINER (widget)->border_width +
	widget->style->ythickness + clist->column_title_area.height);
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
      bint y_delta;
      bint h = 0;

      y_delta = y - ROW_TOP_YPIXEL (clist, dest_info->cell.row);
      
      if (BTK_CLIST_DRAW_DRAG_RECT(clist) &&
	  !BTK_CTREE_ROW (g_list_nth (clist->row_list,
				      dest_info->cell.row))->is_leaf)
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
btk_ctree_drag_begin (BtkWidget	     *widget,
		      BdkDragContext *context)
{
  BtkCList *clist;
  BtkCTree *ctree;
  bboolean use_icons;

  g_return_if_fail (BTK_IS_CTREE (widget));
  g_return_if_fail (context != NULL);

  clist = BTK_CLIST (widget);
  ctree = BTK_CTREE (widget);

  use_icons = BTK_CLIST_USE_DRAG_ICONS (clist);
  BTK_CLIST_UNSET_FLAG (clist, CLIST_USE_DRAG_ICONS);
  BTK_WIDGET_CLASS (parent_class)->drag_begin (widget, context);

  if (use_icons)
    {
      BtkCTreeNode *node;

      BTK_CLIST_SET_FLAG (clist, CLIST_USE_DRAG_ICONS);
      node = BTK_CTREE_NODE (g_list_nth (clist->row_list,
					 clist->click_cell.row));
      if (node)
	{
	  if (BTK_CELL_PIXTEXT
	      (BTK_CTREE_ROW (node)->row.cell[ctree->tree_column])->pixmap)
	    {
	      btk_drag_set_icon_pixmap
		(context,
		 btk_widget_get_colormap (widget),
		 BTK_CELL_PIXTEXT
		 (BTK_CTREE_ROW (node)->row.cell[ctree->tree_column])->pixmap,
		 BTK_CELL_PIXTEXT
		 (BTK_CTREE_ROW (node)->row.cell[ctree->tree_column])->mask,
		 -2, -2);
	      return;
	    }
	}
      btk_drag_set_icon_default (context);
    }
}

static bint
btk_ctree_drag_motion (BtkWidget      *widget,
		       BdkDragContext *context,
		       bint            x,
		       bint            y,
		       buint           time)
{
  BtkCList *clist;
  BtkCTree *ctree;
  BtkCListDestInfo new_info;
  BtkCListDestInfo *dest_info;

  g_return_val_if_fail (BTK_IS_CTREE (widget), FALSE);

  clist = BTK_CLIST (widget);
  ctree = BTK_CTREE (widget);

  dest_info = g_dataset_get_data (context, "btk-clist-drag-dest");

  if (!dest_info)
    {
      dest_info = g_new (BtkCListDestInfo, 1);
	  
      dest_info->cell.row    = -1;
      dest_info->cell.column = -1;
      dest_info->insert_pos  = BTK_CLIST_DRAG_NONE;

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
	  BtkCTreeNode *drag_source;
	  BtkCTreeNode *drag_target;

	  drag_source = BTK_CTREE_NODE (g_list_nth (clist->row_list,
						    clist->click_cell.row));
	  drag_target = BTK_CTREE_NODE (g_list_nth (clist->row_list,
						    new_info.cell.row));

	  if (btk_drag_get_source_widget (context) != widget ||
	      !check_drag (ctree, drag_source, drag_target,
			   new_info.insert_pos))
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
		  (clist,
		   g_list_nth (clist->row_list, dest_info->cell.row)->data,
		   dest_info->cell.row, dest_info->insert_pos);

	      dest_info->insert_pos  = new_info.insert_pos;
	      dest_info->cell.row    = new_info.cell.row;
	      dest_info->cell.column = new_info.cell.column;

	      BTK_CLIST_GET_CLASS (clist)->draw_drag_highlight
		(clist,
		 g_list_nth (clist->row_list, dest_info->cell.row)->data,
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

static void
btk_ctree_drag_data_received (BtkWidget        *widget,
			      BdkDragContext   *context,
			      bint              x,
			      bint              y,
			      BtkSelectionData *selection_data,
			      buint             info,
			      buint32           time)
{
  BtkCTree *ctree;
  BtkCList *clist;

  g_return_if_fail (BTK_IS_CTREE (widget));
  g_return_if_fail (context != NULL);
  g_return_if_fail (selection_data != NULL);

  ctree = BTK_CTREE (widget);
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
	  BtkCTreeNode *source_node;
	  BtkCTreeNode *dest_node;

	  drag_dest_cell (clist, x, y, &dest_info);
	  
	  source_node = BTK_CTREE_NODE (g_list_nth (clist->row_list,
						    source_info->row));
	  dest_node = BTK_CTREE_NODE (g_list_nth (clist->row_list,
						  dest_info.cell.row));

	  if (!source_node || !dest_node)
	    return;

	  switch (dest_info.insert_pos)
	    {
	    case BTK_CLIST_DRAG_NONE:
	      break;
	    case BTK_CLIST_DRAG_INTO:
	      if (check_drag (ctree, source_node, dest_node,
			      dest_info.insert_pos))
		btk_ctree_move (ctree, source_node, dest_node,
				BTK_CTREE_ROW (dest_node)->children);
	      g_dataset_remove_data (context, "btk-clist-drag-dest");
	      break;
	    case BTK_CLIST_DRAG_BEFORE:
	      if (check_drag (ctree, source_node, dest_node,
			      dest_info.insert_pos))
		btk_ctree_move (ctree, source_node,
				BTK_CTREE_ROW (dest_node)->parent, dest_node);
	      g_dataset_remove_data (context, "btk-clist-drag-dest");
	      break;
	    case BTK_CLIST_DRAG_AFTER:
	      if (check_drag (ctree, source_node, dest_node,
			      dest_info.insert_pos))
		btk_ctree_move (ctree, source_node,
				BTK_CTREE_ROW (dest_node)->parent, 
				BTK_CTREE_ROW (dest_node)->sibling);
	      g_dataset_remove_data (context, "btk-clist-drag-dest");
	      break;
	    }
	}
    }
}

GType
btk_ctree_node_get_type (void)
{
  static GType our_type = 0;
  
  if (our_type == 0)
    our_type = g_pointer_type_register_static ("BtkCTreeNode");

  return our_type;
}

#include "btkaliasdef.c"
