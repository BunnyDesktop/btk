/* btktreeprivate.h
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

#ifndef __BTK_TREE_PRIVATE_H__
#define __BTK_TREE_PRIVATE_H__


B_BEGIN_DECLS


#include <btk/btktreeview.h>
#include <btk/btktreeselection.h>
#include <btk/btkrbtree.h>

#define TREE_VIEW_DRAG_WIDTH 6

typedef enum
{
  BTK_TREE_VIEW_IS_LIST = 1 << 0,
  BTK_TREE_VIEW_SHOW_EXPANDERS = 1 << 1,
  BTK_TREE_VIEW_IN_COLUMN_RESIZE = 1 << 2,
  BTK_TREE_VIEW_ARROW_PRELIT = 1 << 3,
  BTK_TREE_VIEW_HEADERS_VISIBLE = 1 << 4,
  BTK_TREE_VIEW_DRAW_KEYFOCUS = 1 << 5,
  BTK_TREE_VIEW_MODEL_SETUP = 1 << 6,
  BTK_TREE_VIEW_IN_COLUMN_DRAG = 1 << 7
} BtkTreeViewFlags;

typedef enum
{
  BTK_TREE_SELECT_MODE_TOGGLE = 1 << 0,
  BTK_TREE_SELECT_MODE_EXTEND = 1 << 1
}
BtkTreeSelectMode;

enum
{
  DRAG_COLUMN_WINDOW_STATE_UNSET = 0,
  DRAG_COLUMN_WINDOW_STATE_ORIGINAL = 1,
  DRAG_COLUMN_WINDOW_STATE_ARROW = 2,
  DRAG_COLUMN_WINDOW_STATE_ARROW_LEFT = 3,
  DRAG_COLUMN_WINDOW_STATE_ARROW_RIGHT = 4
};

enum
{
  RUBBER_BAND_OFF = 0,
  RUBBER_BAND_MAYBE_START = 1,
  RUBBER_BAND_ACTIVE = 2
};

#define BTK_TREE_VIEW_SET_FLAG(tree_view, flag)   B_STMT_START{ (tree_view->priv->flags|=flag); }B_STMT_END
#define BTK_TREE_VIEW_UNSET_FLAG(tree_view, flag) B_STMT_START{ (tree_view->priv->flags&=~(flag)); }B_STMT_END
#define BTK_TREE_VIEW_FLAG_SET(tree_view, flag)   ((tree_view->priv->flags&flag)==flag)
#define TREE_VIEW_HEADER_HEIGHT(tree_view)        (BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_HEADERS_VISIBLE)?tree_view->priv->header_height:0)
#define TREE_VIEW_COLUMN_REQUESTED_WIDTH(column)  (CLAMP (column->requested_width, (column->min_width!=-1)?column->min_width:column->requested_width, (column->max_width!=-1)?column->max_width:column->requested_width))
#define TREE_VIEW_DRAW_EXPANDERS(tree_view)       (!BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_IS_LIST)&&BTK_TREE_VIEW_FLAG_SET (tree_view, BTK_TREE_VIEW_SHOW_EXPANDERS))

 /* This lovely little value is used to determine how far away from the title bar
  * you can move the mouse and still have a column drag work.
  */
#define TREE_VIEW_COLUMN_DRAG_DEAD_MULTIPLIER(tree_view) (10*TREE_VIEW_HEADER_HEIGHT(tree_view))

typedef struct _BtkTreeViewColumnReorder BtkTreeViewColumnReorder;
struct _BtkTreeViewColumnReorder
{
  gint left_align;
  gint right_align;
  BtkTreeViewColumn *left_column;
  BtkTreeViewColumn *right_column;
};

struct _BtkTreeViewPrivate
{
  BtkTreeModel *model;

  guint flags;
  /* tree information */
  BtkRBTree *tree;

  /* Container info */
  GList *children;
  gint width;
  gint height;

  /* Adjustments */
  BtkAdjustment *hadjustment;
  BtkAdjustment *vadjustment;

  /* Sub windows */
  BdkWindow *bin_window;
  BdkWindow *header_window;

  /* Scroll position state keeping */
  BtkTreeRowReference *top_row;
  gint top_row_dy;
  /* dy == y pos of top_row + top_row_dy */
  /* we cache it for simplicity of the code */
  gint dy;

  guint presize_handler_timer;
  guint validate_rows_timer;
  guint scroll_sync_timer;

  /* Indentation and expander layout */
  gint expander_size;
  BtkTreeViewColumn *expander_column;

  gint level_indentation;

  /* Key navigation (focus), selection */
  gint cursor_offset;

  BtkTreeRowReference *anchor;
  BtkTreeRowReference *cursor;

  BtkTreeViewColumn *focus_column;

  /* Current pressed node, previously pressed, prelight */
  BtkRBNode *button_pressed_node;
  BtkRBTree *button_pressed_tree;

  gint pressed_button;
  gint press_start_x;
  gint press_start_y;

  gint event_last_x;
  gint event_last_y;

  guint last_button_time;
  gint last_button_x;
  gint last_button_y;

  BtkRBNode *prelight_node;
  BtkRBTree *prelight_tree;

  /* Cell Editing */
  BtkTreeViewColumn *edited_column;

  /* The node that's currently being collapsed or expanded */
  BtkRBNode *expanded_collapsed_node;
  BtkRBTree *expanded_collapsed_tree;
  guint expand_collapse_timeout;

  /* Auto expand/collapse timeout in hover mode */
  guint auto_expand_timeout;

  /* Selection information */
  BtkTreeSelection *selection;

  /* Header information */
  gint n_columns;
  GList *columns;
  gint header_height;

  BtkTreeViewColumnDropFunc column_drop_func;
  gpointer column_drop_func_data;
  GDestroyNotify column_drop_func_data_destroy;
  GList *column_drag_info;
  BtkTreeViewColumnReorder *cur_reorder;

  gint prev_width_before_expander;

  /* Interactive Header reordering */
  BdkWindow *drag_window;
  BdkWindow *drag_highlight_window;
  BtkTreeViewColumn *drag_column;
  gint drag_column_x;

  /* Interactive Header Resizing */
  gint drag_pos;
  gint x_drag;

  /* Non-interactive Header Resizing, expand flag support */
  gint prev_width;

  gint last_extra_space;
  gint last_extra_space_per_column;
  gint last_number_of_expand_columns;

  /* BATK Hack */
  BtkTreeDestroyCountFunc destroy_count_func;
  gpointer destroy_count_data;
  GDestroyNotify destroy_count_destroy;

  /* Scroll timeout (e.g. during dnd, rubber banding) */
  guint scroll_timeout;

  /* Row drag-and-drop */
  BtkTreeRowReference *drag_dest_row;
  BtkTreeViewDropPosition drag_dest_pos;
  guint open_dest_timeout;

  /* Rubber banding */
  gint rubber_band_status;
  gint rubber_band_x;
  gint rubber_band_y;
  gint rubber_band_extend;
  gint rubber_band_modify;

  BtkRBNode *rubber_band_start_node;
  BtkRBTree *rubber_band_start_tree;

  BtkRBNode *rubber_band_end_node;
  BtkRBTree *rubber_band_end_tree;

  /* fixed height */
  gint fixed_height;

  /* Scroll-to functionality when unrealized */
  BtkTreeRowReference *scroll_to_path;
  BtkTreeViewColumn *scroll_to_column;
  gfloat scroll_to_row_align;
  gfloat scroll_to_col_align;

  /* Interactive search */
  gint selected_iter;
  gint search_column;
  BtkTreeViewSearchPositionFunc search_position_func;
  BtkTreeViewSearchEqualFunc search_equal_func;
  gpointer search_user_data;
  GDestroyNotify search_destroy;
  gpointer search_position_user_data;
  GDestroyNotify search_position_destroy;
  BtkWidget *search_window;
  BtkWidget *search_entry;
  guint search_entry_changed_id;
  guint typeselect_flush_timeout;

  /* Grid and tree lines */
  BtkTreeViewGridLines grid_lines;
  double grid_line_dashes[2];
  int grid_line_width;

  gboolean tree_lines_enabled;
  double tree_line_dashes[2];
  int tree_line_width;

  /* Row separators */
  BtkTreeViewRowSeparatorFunc row_separator_func;
  gpointer row_separator_data;
  GDestroyNotify row_separator_destroy;

  /* Tooltip support */
  gint tooltip_column;

  /* Here comes the bitfield */
  guint scroll_to_use_align : 1;

  guint fixed_height_mode : 1;
  guint fixed_height_check : 1;

  guint reorderable : 1;
  guint header_has_focus : 1;
  guint drag_column_window_state : 3;
  /* hint to display rows in alternating colors */
  guint has_rules : 1;
  guint mark_rows_col_dirty : 1;

  /* for DnD */
  guint empty_view_drop : 1;

  guint modify_selection_pressed : 1;
  guint extend_selection_pressed : 1;

  guint init_hadjust_value : 1;

  guint in_top_row_to_dy : 1;

  /* interactive search */
  guint enable_search : 1;
  guint disable_popdown : 1;
  guint search_custom_entry_set : 1;
  
  guint hover_selection : 1;
  guint hover_expand : 1;
  guint imcontext_changed : 1;

  guint rubber_banding_enable : 1;

  guint in_grab : 1;

  guint post_validation_flag : 1;

  /* Whether our key press handler is to avoid sending an unhandled binding to the search entry */
  guint search_entry_avoid_unhandled_binding : 1;
};

#ifdef __GNUC__

#define TREE_VIEW_INTERNAL_ASSERT(expr, ret)     B_STMT_START{          \
     if (!(expr))                                                       \
       {                                                                \
         g_log (G_LOG_DOMAIN,                                           \
                G_LOG_LEVEL_CRITICAL,                                   \
		"%s (%s): assertion `%s' failed.\n"                     \
	        "There is a disparity between the internal view of the BtkTreeView,\n"    \
		"and the BtkTreeModel.  This generally means that the model has changed\n"\
		"without letting the view know.  Any display from now on is likely to\n"  \
		"be incorrect.\n",                                                        \
                B_STRLOC,                                               \
                B_STRFUNC,                                              \
                #expr);                                                 \
         return ret;                                                    \
       };                               }B_STMT_END

#define TREE_VIEW_INTERNAL_ASSERT_VOID(expr)     B_STMT_START{          \
     if (!(expr))                                                       \
       {                                                                \
         g_log (G_LOG_DOMAIN,                                           \
                G_LOG_LEVEL_CRITICAL,                                   \
		"%s (%s): assertion `%s' failed.\n"                     \
	        "There is a disparity between the internal view of the BtkTreeView,\n"    \
		"and the BtkTreeModel.  This generally means that the model has changed\n"\
		"without letting the view know.  Any display from now on is likely to\n"  \
		"be incorrect.\n",                                                        \
                B_STRLOC,                                               \
                B_STRFUNC,                                              \
                #expr);                                                 \
         return;                                                        \
       };                               }B_STMT_END

#else

#define TREE_VIEW_INTERNAL_ASSERT(expr, ret)     B_STMT_START{          \
     if (!(expr))                                                       \
       {                                                                \
         g_log (G_LOG_DOMAIN,                                           \
                G_LOG_LEVEL_CRITICAL,                                   \
		"file %s: line %d: assertion `%s' failed.\n"       \
	        "There is a disparity between the internal view of the BtkTreeView,\n"    \
		"and the BtkTreeModel.  This generally means that the model has changed\n"\
		"without letting the view know.  Any display from now on is likely to\n"  \
		"be incorrect.\n",                                                        \
                __FILE__,                                               \
                __LINE__,                                               \
                #expr);                                                 \
         return ret;                                                    \
       };                               }B_STMT_END

#define TREE_VIEW_INTERNAL_ASSERT_VOID(expr)     B_STMT_START{          \
     if (!(expr))                                                       \
       {                                                                \
         g_log (G_LOG_DOMAIN,                                           \
                G_LOG_LEVEL_CRITICAL,                                   \
		"file %s: line %d: assertion '%s' failed.\n"            \
	        "There is a disparity between the internal view of the BtkTreeView,\n"    \
		"and the BtkTreeModel.  This generally means that the model has changed\n"\
		"without letting the view know.  Any display from now on is likely to\n"  \
		"be incorrect.\n",                                                        \
                __FILE__,                                               \
                __LINE__,                                               \
                #expr);                                                 \
         return;                                                        \
       };                               }B_STMT_END
#endif


/* functions that shouldn't be exported */
void         _btk_tree_selection_internal_select_node (BtkTreeSelection  *selection,
						       BtkRBNode         *node,
						       BtkRBTree         *tree,
						       BtkTreePath       *path,
                                                       BtkTreeSelectMode  mode,
						       gboolean           override_browse_mode);
void         _btk_tree_selection_emit_changed         (BtkTreeSelection  *selection);
gboolean     _btk_tree_view_find_node                 (BtkTreeView       *tree_view,
						       BtkTreePath       *path,
						       BtkRBTree        **tree,
						       BtkRBNode        **node);
BtkTreePath *_btk_tree_view_find_path                 (BtkTreeView       *tree_view,
						       BtkRBTree         *tree,
						       BtkRBNode         *node);
void         _btk_tree_view_child_move_resize         (BtkTreeView       *tree_view,
						       BtkWidget         *widget,
						       gint               x,
						       gint               y,
						       gint               width,
						       gint               height);
void         _btk_tree_view_queue_draw_node           (BtkTreeView       *tree_view,
						       BtkRBTree         *tree,
						       BtkRBNode         *node,
						       const BdkRectangle *clip_rect);

void _btk_tree_view_column_realize_button   (BtkTreeViewColumn *column);
void _btk_tree_view_column_unrealize_button (BtkTreeViewColumn *column);
void _btk_tree_view_column_set_tree_view    (BtkTreeViewColumn *column,
					     BtkTreeView       *tree_view);
void _btk_tree_view_column_unset_model      (BtkTreeViewColumn *column,
					     BtkTreeModel      *old_model);
void _btk_tree_view_column_unset_tree_view  (BtkTreeViewColumn *column);
void _btk_tree_view_column_set_width        (BtkTreeViewColumn *column,
					     gint               width);
void _btk_tree_view_column_start_drag       (BtkTreeView       *tree_view,
					     BtkTreeViewColumn *column);
gboolean _btk_tree_view_column_cell_event   (BtkTreeViewColumn  *tree_column,
					     BtkCellEditable   **editable_widget,
					     BdkEvent           *event,
					     gchar              *path_string,
					     const BdkRectangle *background_area,
					     const BdkRectangle *cell_area,
					     guint               flags);
void _btk_tree_view_column_start_editing (BtkTreeViewColumn *tree_column,
					  BtkCellEditable   *editable_widget);
void _btk_tree_view_column_stop_editing  (BtkTreeViewColumn *tree_column);
void _btk_tree_view_install_mark_rows_col_dirty (BtkTreeView *tree_view);
void             _btk_tree_view_column_autosize          (BtkTreeView       *tree_view,
							  BtkTreeViewColumn *column);

gboolean         _btk_tree_view_column_has_editable_cell (BtkTreeViewColumn *column);
BtkCellRenderer *_btk_tree_view_column_get_edited_cell   (BtkTreeViewColumn *column);
gint             _btk_tree_view_column_count_special_cells (BtkTreeViewColumn *column);
BtkCellRenderer *_btk_tree_view_column_get_cell_at_pos   (BtkTreeViewColumn *column,
							  gint               x);

BtkTreeSelection* _btk_tree_selection_new                (void);
BtkTreeSelection* _btk_tree_selection_new_with_tree_view (BtkTreeView      *tree_view);
void              _btk_tree_selection_set_tree_view      (BtkTreeSelection *selection,
                                                          BtkTreeView      *tree_view);
gboolean          _btk_tree_selection_row_is_selectable  (BtkTreeSelection *selection,
							  BtkRBNode        *node,
							  BtkTreePath      *path);

void		  _btk_tree_view_column_cell_render      (BtkTreeViewColumn  *tree_column,
							  BdkWindow          *window,
							  const BdkRectangle *background_area,
							  const BdkRectangle *cell_area,
							  const BdkRectangle *expose_area,
							  guint               flags);
void		  _btk_tree_view_column_get_focus_area   (BtkTreeViewColumn  *tree_column,
							  const BdkRectangle *background_area,
							  const BdkRectangle *cell_area,
							  BdkRectangle       *focus_area);
gboolean	  _btk_tree_view_column_cell_focus       (BtkTreeViewColumn  *tree_column,
							  gint                direction,
							  gboolean            left,
							  gboolean            right);
void		  _btk_tree_view_column_cell_draw_focus  (BtkTreeViewColumn  *tree_column,
							  BdkWindow          *window,
							  const BdkRectangle *background_area,
							  const BdkRectangle *cell_area,
							  const BdkRectangle *expose_area,
							  guint               flags);
void		  _btk_tree_view_column_cell_set_dirty	 (BtkTreeViewColumn  *tree_column,
							  gboolean            install_handler);
void              _btk_tree_view_column_get_neighbor_sizes (BtkTreeViewColumn *column,
							    BtkCellRenderer   *cell,
							    gint              *left,
							    gint              *right);


B_END_DECLS


#endif /* __BTK_TREE_PRIVATE_H__ */

