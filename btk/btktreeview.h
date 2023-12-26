/* btktreeview.h
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

#ifndef __BTK_TREE_VIEW_H__
#define __BTK_TREE_VIEW_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcontainer.h>
#include <btk/btktreemodel.h>
#include <btk/btktreeviewcolumn.h>
#include <btk/btkdnd.h>
#include <btk/btkentry.h>

B_BEGIN_DECLS


typedef enum
{
  /* drop before/after this row */
  BTK_TREE_VIEW_DROP_BEFORE,
  BTK_TREE_VIEW_DROP_AFTER,
  /* drop as a child of this row (with fallback to before or after
   * if into is not possible)
   */
  BTK_TREE_VIEW_DROP_INTO_OR_BEFORE,
  BTK_TREE_VIEW_DROP_INTO_OR_AFTER
} BtkTreeViewDropPosition;

#define BTK_TYPE_TREE_VIEW		(btk_tree_view_get_type ())
#define BTK_TREE_VIEW(obj)		(B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TREE_VIEW, BtkTreeView))
#define BTK_TREE_VIEW_CLASS(klass)	(B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TREE_VIEW, BtkTreeViewClass))
#define BTK_IS_TREE_VIEW(obj)		(B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TREE_VIEW))
#define BTK_IS_TREE_VIEW_CLASS(klass)	(B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TREE_VIEW))
#define BTK_TREE_VIEW_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TREE_VIEW, BtkTreeViewClass))

typedef struct _BtkTreeView           BtkTreeView;
typedef struct _BtkTreeViewClass      BtkTreeViewClass;
typedef struct _BtkTreeViewPrivate    BtkTreeViewPrivate;
typedef struct _BtkTreeSelection      BtkTreeSelection;
typedef struct _BtkTreeSelectionClass BtkTreeSelectionClass;

struct _BtkTreeView
{
  BtkContainer parent;

  BtkTreeViewPrivate *GSEAL (priv);
};

struct _BtkTreeViewClass
{
  BtkContainerClass parent_class;

  void     (* set_scroll_adjustments)     (BtkTreeView       *tree_view,
				           BtkAdjustment     *hadjustment,
				           BtkAdjustment     *vadjustment);
  void     (* row_activated)              (BtkTreeView       *tree_view,
				           BtkTreePath       *path,
					   BtkTreeViewColumn *column);
  bboolean (* test_expand_row)            (BtkTreeView       *tree_view,
				           BtkTreeIter       *iter,
				           BtkTreePath       *path);
  bboolean (* test_collapse_row)          (BtkTreeView       *tree_view,
				           BtkTreeIter       *iter,
				           BtkTreePath       *path);
  void     (* row_expanded)               (BtkTreeView       *tree_view,
				           BtkTreeIter       *iter,
				           BtkTreePath       *path);
  void     (* row_collapsed)              (BtkTreeView       *tree_view,
				           BtkTreeIter       *iter,
				           BtkTreePath       *path);
  void     (* columns_changed)            (BtkTreeView       *tree_view);
  void     (* cursor_changed)             (BtkTreeView       *tree_view);

  /* Key Binding signals */
  bboolean (* move_cursor)                (BtkTreeView       *tree_view,
				           BtkMovementStep    step,
				           bint               count);
  bboolean (* select_all)                 (BtkTreeView       *tree_view);
  bboolean (* unselect_all)               (BtkTreeView       *tree_view);
  bboolean (* select_cursor_row)          (BtkTreeView       *tree_view,
					   bboolean           start_editing);
  bboolean (* toggle_cursor_row)          (BtkTreeView       *tree_view);
  bboolean (* expand_collapse_cursor_row) (BtkTreeView       *tree_view,
					   bboolean           logical,
					   bboolean           expand,
					   bboolean           open_all);
  bboolean (* select_cursor_parent)       (BtkTreeView       *tree_view);
  bboolean (* start_interactive_search)   (BtkTreeView       *tree_view);

  /* Padding for future expansion */
  void (*_btk_reserved0) (void);
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


typedef bboolean (* BtkTreeViewColumnDropFunc) (BtkTreeView             *tree_view,
						BtkTreeViewColumn       *column,
						BtkTreeViewColumn       *prev_column,
						BtkTreeViewColumn       *next_column,
						bpointer                 data);
typedef void     (* BtkTreeViewMappingFunc)    (BtkTreeView             *tree_view,
						BtkTreePath             *path,
						bpointer                 user_data);
typedef bboolean (*BtkTreeViewSearchEqualFunc) (BtkTreeModel            *model,
						bint                     column,
						const bchar             *key,
						BtkTreeIter             *iter,
						bpointer                 search_data);
typedef bboolean (*BtkTreeViewRowSeparatorFunc) (BtkTreeModel      *model,
						 BtkTreeIter       *iter,
						 bpointer           data);
typedef void     (*BtkTreeViewSearchPositionFunc) (BtkTreeView  *tree_view,
						   BtkWidget    *search_dialog,
						   bpointer      user_data);


/* Creators */
GType                  btk_tree_view_get_type                      (void) B_GNUC_CONST;
BtkWidget             *btk_tree_view_new                           (void);
BtkWidget             *btk_tree_view_new_with_model                (BtkTreeModel              *model);

/* Accessors */
BtkTreeModel          *btk_tree_view_get_model                     (BtkTreeView               *tree_view);
void                   btk_tree_view_set_model                     (BtkTreeView               *tree_view,
								    BtkTreeModel              *model);
BtkTreeSelection      *btk_tree_view_get_selection                 (BtkTreeView               *tree_view);
BtkAdjustment         *btk_tree_view_get_hadjustment               (BtkTreeView               *tree_view);
void                   btk_tree_view_set_hadjustment               (BtkTreeView               *tree_view,
								    BtkAdjustment             *adjustment);
BtkAdjustment         *btk_tree_view_get_vadjustment               (BtkTreeView               *tree_view);
void                   btk_tree_view_set_vadjustment               (BtkTreeView               *tree_view,
								    BtkAdjustment             *adjustment);
bboolean               btk_tree_view_get_headers_visible           (BtkTreeView               *tree_view);
void                   btk_tree_view_set_headers_visible           (BtkTreeView               *tree_view,
								    bboolean                   headers_visible);
void                   btk_tree_view_columns_autosize              (BtkTreeView               *tree_view);
bboolean               btk_tree_view_get_headers_clickable         (BtkTreeView *tree_view);
void                   btk_tree_view_set_headers_clickable         (BtkTreeView               *tree_view,
								    bboolean                   setting);
void                   btk_tree_view_set_rules_hint                (BtkTreeView               *tree_view,
								    bboolean                   setting);
bboolean               btk_tree_view_get_rules_hint                (BtkTreeView               *tree_view);

/* Column funtions */
bint                   btk_tree_view_append_column                 (BtkTreeView               *tree_view,
								    BtkTreeViewColumn         *column);
bint                   btk_tree_view_remove_column                 (BtkTreeView               *tree_view,
								    BtkTreeViewColumn         *column);
bint                   btk_tree_view_insert_column                 (BtkTreeView               *tree_view,
								    BtkTreeViewColumn         *column,
								    bint                       position);
bint                   btk_tree_view_insert_column_with_attributes (BtkTreeView               *tree_view,
								    bint                       position,
								    const bchar               *title,
								    BtkCellRenderer           *cell,
								    ...) B_GNUC_NULL_TERMINATED;
bint                   btk_tree_view_insert_column_with_data_func  (BtkTreeView               *tree_view,
								    bint                       position,
								    const bchar               *title,
								    BtkCellRenderer           *cell,
                                                                    BtkTreeCellDataFunc        func,
                                                                    bpointer                   data,
                                                                    GDestroyNotify             dnotify);
BtkTreeViewColumn     *btk_tree_view_get_column                    (BtkTreeView               *tree_view,
								    bint                       n);
GList                 *btk_tree_view_get_columns                   (BtkTreeView               *tree_view);
void                   btk_tree_view_move_column_after             (BtkTreeView               *tree_view,
								    BtkTreeViewColumn         *column,
								    BtkTreeViewColumn         *base_column);
void                   btk_tree_view_set_expander_column           (BtkTreeView               *tree_view,
								    BtkTreeViewColumn         *column);
BtkTreeViewColumn     *btk_tree_view_get_expander_column           (BtkTreeView               *tree_view);
void                   btk_tree_view_set_column_drag_function      (BtkTreeView               *tree_view,
								    BtkTreeViewColumnDropFunc  func,
								    bpointer                   user_data,
								    GDestroyNotify             destroy);

/* Actions */
void                   btk_tree_view_scroll_to_point               (BtkTreeView               *tree_view,
								    bint                       tree_x,
								    bint                       tree_y);
void                   btk_tree_view_scroll_to_cell                (BtkTreeView               *tree_view,
								    BtkTreePath               *path,
								    BtkTreeViewColumn         *column,
								    bboolean                   use_align,
								    bfloat                     row_align,
								    bfloat                     col_align);
void                   btk_tree_view_row_activated                 (BtkTreeView               *tree_view,
								    BtkTreePath               *path,
								    BtkTreeViewColumn         *column);
void                   btk_tree_view_expand_all                    (BtkTreeView               *tree_view);
void                   btk_tree_view_collapse_all                  (BtkTreeView               *tree_view);
void                   btk_tree_view_expand_to_path                (BtkTreeView               *tree_view,
								    BtkTreePath               *path);
bboolean               btk_tree_view_expand_row                    (BtkTreeView               *tree_view,
								    BtkTreePath               *path,
								    bboolean                   open_all);
bboolean               btk_tree_view_collapse_row                  (BtkTreeView               *tree_view,
								    BtkTreePath               *path);
void                   btk_tree_view_map_expanded_rows             (BtkTreeView               *tree_view,
								    BtkTreeViewMappingFunc     func,
								    bpointer                   data);
bboolean               btk_tree_view_row_expanded                  (BtkTreeView               *tree_view,
								    BtkTreePath               *path);
void                   btk_tree_view_set_reorderable               (BtkTreeView               *tree_view,
								    bboolean                   reorderable);
bboolean               btk_tree_view_get_reorderable               (BtkTreeView               *tree_view);
void                   btk_tree_view_set_cursor                    (BtkTreeView               *tree_view,
								    BtkTreePath               *path,
								    BtkTreeViewColumn         *focus_column,
								    bboolean                   start_editing);
void                   btk_tree_view_set_cursor_on_cell            (BtkTreeView               *tree_view,
								    BtkTreePath               *path,
								    BtkTreeViewColumn         *focus_column,
								    BtkCellRenderer           *focus_cell,
								    bboolean                   start_editing);
void                   btk_tree_view_get_cursor                    (BtkTreeView               *tree_view,
								    BtkTreePath              **path,
								    BtkTreeViewColumn        **focus_column);


/* Layout information */
BdkWindow             *btk_tree_view_get_bin_window                (BtkTreeView               *tree_view);
bboolean               btk_tree_view_get_path_at_pos               (BtkTreeView               *tree_view,
								    bint                       x,
								    bint                       y,
								    BtkTreePath              **path,
								    BtkTreeViewColumn        **column,
								    bint                      *cell_x,
								    bint                      *cell_y);
void                   btk_tree_view_get_cell_area                 (BtkTreeView               *tree_view,
								    BtkTreePath               *path,
								    BtkTreeViewColumn         *column,
								    BdkRectangle              *rect);
void                   btk_tree_view_get_background_area           (BtkTreeView               *tree_view,
								    BtkTreePath               *path,
								    BtkTreeViewColumn         *column,
								    BdkRectangle              *rect);
void                   btk_tree_view_get_visible_rect              (BtkTreeView               *tree_view,
								    BdkRectangle              *visible_rect);

#ifndef BTK_DISABLE_DEPRECATED
void                   btk_tree_view_widget_to_tree_coords         (BtkTreeView               *tree_view,
								    bint                       wx,
								    bint                       wy,
								    bint                      *tx,
								    bint                      *ty);
void                   btk_tree_view_tree_to_widget_coords         (BtkTreeView               *tree_view,
								    bint                       tx,
								    bint                       ty,
								    bint                      *wx,
								    bint                      *wy);
#endif /* !BTK_DISABLE_DEPRECATED */
bboolean               btk_tree_view_get_visible_range             (BtkTreeView               *tree_view,
								    BtkTreePath              **start_path,
								    BtkTreePath              **end_path);

/* Drag-and-Drop support */
void                   btk_tree_view_enable_model_drag_source      (BtkTreeView               *tree_view,
								    BdkModifierType            start_button_mask,
								    const BtkTargetEntry      *targets,
								    bint                       n_targets,
								    BdkDragAction              actions);
void                   btk_tree_view_enable_model_drag_dest        (BtkTreeView               *tree_view,
								    const BtkTargetEntry      *targets,
								    bint                       n_targets,
								    BdkDragAction              actions);
void                   btk_tree_view_unset_rows_drag_source        (BtkTreeView               *tree_view);
void                   btk_tree_view_unset_rows_drag_dest          (BtkTreeView               *tree_view);


/* These are useful to implement your own custom stuff. */
void                   btk_tree_view_set_drag_dest_row             (BtkTreeView               *tree_view,
								    BtkTreePath               *path,
								    BtkTreeViewDropPosition    pos);
void                   btk_tree_view_get_drag_dest_row             (BtkTreeView               *tree_view,
								    BtkTreePath              **path,
								    BtkTreeViewDropPosition   *pos);
bboolean               btk_tree_view_get_dest_row_at_pos           (BtkTreeView               *tree_view,
								    bint                       drag_x,
								    bint                       drag_y,
								    BtkTreePath              **path,
								    BtkTreeViewDropPosition   *pos);
BdkPixmap             *btk_tree_view_create_row_drag_icon          (BtkTreeView               *tree_view,
								    BtkTreePath               *path);

/* Interactive search */
void                       btk_tree_view_set_enable_search     (BtkTreeView                *tree_view,
								bboolean                    enable_search);
bboolean                   btk_tree_view_get_enable_search     (BtkTreeView                *tree_view);
bint                       btk_tree_view_get_search_column     (BtkTreeView                *tree_view);
void                       btk_tree_view_set_search_column     (BtkTreeView                *tree_view,
								bint                        column);
BtkTreeViewSearchEqualFunc btk_tree_view_get_search_equal_func (BtkTreeView                *tree_view);
void                       btk_tree_view_set_search_equal_func (BtkTreeView                *tree_view,
								BtkTreeViewSearchEqualFunc  search_equal_func,
								bpointer                    search_user_data,
								GDestroyNotify              search_destroy);

BtkEntry                     *btk_tree_view_get_search_entry         (BtkTreeView                   *tree_view);
void                          btk_tree_view_set_search_entry         (BtkTreeView                   *tree_view,
								      BtkEntry                      *entry);
BtkTreeViewSearchPositionFunc btk_tree_view_get_search_position_func (BtkTreeView                   *tree_view);
void                          btk_tree_view_set_search_position_func (BtkTreeView                   *tree_view,
								      BtkTreeViewSearchPositionFunc  func,
								      bpointer                       data,
								      GDestroyNotify                 destroy);

/* Convert between the different coordinate systems */
void btk_tree_view_convert_widget_to_tree_coords       (BtkTreeView *tree_view,
							bint         wx,
							bint         wy,
							bint        *tx,
							bint        *ty);
void btk_tree_view_convert_tree_to_widget_coords       (BtkTreeView *tree_view,
							bint         tx,
							bint         ty,
							bint        *wx,
							bint        *wy);
void btk_tree_view_convert_widget_to_bin_window_coords (BtkTreeView *tree_view,
							bint         wx,
							bint         wy,
							bint        *bx,
							bint        *by);
void btk_tree_view_convert_bin_window_to_widget_coords (BtkTreeView *tree_view,
							bint         bx,
							bint         by,
							bint        *wx,
							bint        *wy);
void btk_tree_view_convert_tree_to_bin_window_coords   (BtkTreeView *tree_view,
							bint         tx,
							bint         ty,
							bint        *bx,
							bint        *by);
void btk_tree_view_convert_bin_window_to_tree_coords   (BtkTreeView *tree_view,
							bint         bx,
							bint         by,
							bint        *tx,
							bint        *ty);

/* This function should really never be used.  It is just for use by BATK.
 */
typedef void (* BtkTreeDestroyCountFunc)  (BtkTreeView             *tree_view,
					   BtkTreePath             *path,
					   bint                     children,
					   bpointer                 user_data);
void btk_tree_view_set_destroy_count_func (BtkTreeView             *tree_view,
					   BtkTreeDestroyCountFunc  func,
					   bpointer                 data,
					   GDestroyNotify           destroy);

void     btk_tree_view_set_fixed_height_mode (BtkTreeView          *tree_view,
					      bboolean              enable);
bboolean btk_tree_view_get_fixed_height_mode (BtkTreeView          *tree_view);
void     btk_tree_view_set_hover_selection   (BtkTreeView          *tree_view,
					      bboolean              hover);
bboolean btk_tree_view_get_hover_selection   (BtkTreeView          *tree_view);
void     btk_tree_view_set_hover_expand      (BtkTreeView          *tree_view,
					      bboolean              expand);
bboolean btk_tree_view_get_hover_expand      (BtkTreeView          *tree_view);
void     btk_tree_view_set_rubber_banding    (BtkTreeView          *tree_view,
					      bboolean              enable);
bboolean btk_tree_view_get_rubber_banding    (BtkTreeView          *tree_view);

bboolean btk_tree_view_is_rubber_banding_active (BtkTreeView       *tree_view);

BtkTreeViewRowSeparatorFunc btk_tree_view_get_row_separator_func (BtkTreeView               *tree_view);
void                        btk_tree_view_set_row_separator_func (BtkTreeView                *tree_view,
								  BtkTreeViewRowSeparatorFunc func,
								  bpointer                    data,
								  GDestroyNotify              destroy);

BtkTreeViewGridLines        btk_tree_view_get_grid_lines         (BtkTreeView                *tree_view);
void                        btk_tree_view_set_grid_lines         (BtkTreeView                *tree_view,
								  BtkTreeViewGridLines        grid_lines);
bboolean                    btk_tree_view_get_enable_tree_lines  (BtkTreeView                *tree_view);
void                        btk_tree_view_set_enable_tree_lines  (BtkTreeView                *tree_view,
								  bboolean                    enabled);
void                        btk_tree_view_set_show_expanders     (BtkTreeView                *tree_view,
								  bboolean                    enabled);
bboolean                    btk_tree_view_get_show_expanders     (BtkTreeView                *tree_view);
void                        btk_tree_view_set_level_indentation  (BtkTreeView                *tree_view,
								  bint                        indentation);
bint                        btk_tree_view_get_level_indentation  (BtkTreeView                *tree_view);

/* Convenience functions for setting tooltips */
void          btk_tree_view_set_tooltip_row    (BtkTreeView       *tree_view,
						BtkTooltip        *tooltip,
						BtkTreePath       *path);
void          btk_tree_view_set_tooltip_cell   (BtkTreeView       *tree_view,
						BtkTooltip        *tooltip,
						BtkTreePath       *path,
						BtkTreeViewColumn *column,
						BtkCellRenderer   *cell);
bboolean      btk_tree_view_get_tooltip_context(BtkTreeView       *tree_view,
						bint              *x,
						bint              *y,
						bboolean           keyboard_tip,
						BtkTreeModel     **model,
						BtkTreePath      **path,
						BtkTreeIter       *iter);
void          btk_tree_view_set_tooltip_column (BtkTreeView       *tree_view,
					        bint               column);
bint          btk_tree_view_get_tooltip_column (BtkTreeView       *tree_view);

B_END_DECLS


#endif /* __BTK_TREE_VIEW_H__ */
