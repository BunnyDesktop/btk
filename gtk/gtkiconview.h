/* btkiconview.h
 * Copyright (C) 2002, 2004  Anders Carlsson <andersca@gnome.org>
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

#ifndef __BTK_ICON_VIEW_H__
#define __BTK_ICON_VIEW_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcontainer.h>
#include <btk/btktreemodel.h>
#include <btk/btkcellrenderer.h>
#include <btk/btkselection.h>
#include <btk/btktooltip.h>

G_BEGIN_DECLS

#define BTK_TYPE_ICON_VIEW            (btk_icon_view_get_type ())
#define BTK_ICON_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_ICON_VIEW, BtkIconView))
#define BTK_ICON_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_ICON_VIEW, BtkIconViewClass))
#define BTK_IS_ICON_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_ICON_VIEW))
#define BTK_IS_ICON_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_ICON_VIEW))
#define BTK_ICON_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_ICON_VIEW, BtkIconViewClass))

typedef struct _BtkIconView           BtkIconView;
typedef struct _BtkIconViewClass      BtkIconViewClass;
typedef struct _BtkIconViewPrivate    BtkIconViewPrivate;

typedef void (* BtkIconViewForeachFunc)     (BtkIconView      *icon_view,
					     BtkTreePath      *path,
					     gpointer          data);

typedef enum
{
  BTK_ICON_VIEW_NO_DROP,
  BTK_ICON_VIEW_DROP_INTO,
  BTK_ICON_VIEW_DROP_LEFT,
  BTK_ICON_VIEW_DROP_RIGHT,
  BTK_ICON_VIEW_DROP_ABOVE,
  BTK_ICON_VIEW_DROP_BELOW
} BtkIconViewDropPosition;

struct _BtkIconView
{
  BtkContainer parent;

  BtkIconViewPrivate *GSEAL (priv);
};

struct _BtkIconViewClass
{
  BtkContainerClass parent_class;

  void    (* set_scroll_adjustments) (BtkIconView      *icon_view,
				      BtkAdjustment    *hadjustment,
				      BtkAdjustment    *vadjustment);
  
  void    (* item_activated)         (BtkIconView      *icon_view,
				      BtkTreePath      *path);
  void    (* selection_changed)      (BtkIconView      *icon_view);

  /* Key binding signals */
  void    (* select_all)             (BtkIconView      *icon_view);
  void    (* unselect_all)           (BtkIconView      *icon_view);
  void    (* select_cursor_item)     (BtkIconView      *icon_view);
  void    (* toggle_cursor_item)     (BtkIconView      *icon_view);
  gboolean (* move_cursor)           (BtkIconView      *icon_view,
				      BtkMovementStep   step,
				      gint              count);
  gboolean (* activate_cursor_item)  (BtkIconView      *icon_view);
};

GType          btk_icon_view_get_type          (void) G_GNUC_CONST;
BtkWidget *    btk_icon_view_new               (void);
BtkWidget *    btk_icon_view_new_with_model    (BtkTreeModel   *model);

void           btk_icon_view_set_model         (BtkIconView    *icon_view,
 					        BtkTreeModel   *model);
BtkTreeModel * btk_icon_view_get_model         (BtkIconView    *icon_view);
void           btk_icon_view_set_text_column   (BtkIconView    *icon_view,
	 	 			        gint            column);
gint           btk_icon_view_get_text_column   (BtkIconView    *icon_view);
void           btk_icon_view_set_markup_column (BtkIconView    *icon_view,
					        gint            column);
gint           btk_icon_view_get_markup_column (BtkIconView    *icon_view);
void           btk_icon_view_set_pixbuf_column (BtkIconView    *icon_view,
					        gint            column);
gint           btk_icon_view_get_pixbuf_column (BtkIconView    *icon_view);

void           btk_icon_view_set_orientation   (BtkIconView    *icon_view,
	   			                BtkOrientation  orientation);
BtkOrientation btk_icon_view_get_orientation   (BtkIconView    *icon_view);
void           btk_icon_view_set_item_orientation (BtkIconView    *icon_view,
	   			                   BtkOrientation  orientation);
BtkOrientation btk_icon_view_get_item_orientation (BtkIconView    *icon_view);
void           btk_icon_view_set_columns       (BtkIconView    *icon_view,
		 			        gint            columns);
gint           btk_icon_view_get_columns       (BtkIconView    *icon_view);
void           btk_icon_view_set_item_width    (BtkIconView    *icon_view,
					        gint            item_width);
gint           btk_icon_view_get_item_width    (BtkIconView    *icon_view);
void           btk_icon_view_set_spacing       (BtkIconView    *icon_view, 
		 			        gint            spacing);
gint           btk_icon_view_get_spacing       (BtkIconView    *icon_view);
void           btk_icon_view_set_row_spacing   (BtkIconView    *icon_view, 
					        gint            row_spacing);
gint           btk_icon_view_get_row_spacing   (BtkIconView    *icon_view);
void           btk_icon_view_set_column_spacing (BtkIconView    *icon_view, 
					        gint            column_spacing);
gint           btk_icon_view_get_column_spacing (BtkIconView    *icon_view);
void           btk_icon_view_set_margin        (BtkIconView    *icon_view, 
					        gint            margin);
gint           btk_icon_view_get_margin        (BtkIconView    *icon_view);
void           btk_icon_view_set_item_padding  (BtkIconView    *icon_view, 
					        gint            item_padding);
gint           btk_icon_view_get_item_padding  (BtkIconView    *icon_view);


BtkTreePath *  btk_icon_view_get_path_at_pos   (BtkIconView     *icon_view,
						gint             x,
						gint             y);
gboolean       btk_icon_view_get_item_at_pos   (BtkIconView     *icon_view,
						gint              x,
						gint              y,
						BtkTreePath     **path,
						BtkCellRenderer **cell);
gboolean       btk_icon_view_get_visible_range (BtkIconView      *icon_view,
						BtkTreePath     **start_path,
						BtkTreePath     **end_path);

void           btk_icon_view_selected_foreach   (BtkIconView            *icon_view,
						 BtkIconViewForeachFunc  func,
						 gpointer                data);
void           btk_icon_view_set_selection_mode (BtkIconView            *icon_view,
						 BtkSelectionMode        mode);
BtkSelectionMode btk_icon_view_get_selection_mode (BtkIconView            *icon_view);
void             btk_icon_view_select_path        (BtkIconView            *icon_view,
						   BtkTreePath            *path);
void             btk_icon_view_unselect_path      (BtkIconView            *icon_view,
						   BtkTreePath            *path);
gboolean         btk_icon_view_path_is_selected   (BtkIconView            *icon_view,
						   BtkTreePath            *path);
gint             btk_icon_view_get_item_row       (BtkIconView            *icon_view,
                                                   BtkTreePath            *path);
gint             btk_icon_view_get_item_column    (BtkIconView            *icon_view,
                                                   BtkTreePath            *path);
GList           *btk_icon_view_get_selected_items (BtkIconView            *icon_view);
void             btk_icon_view_select_all         (BtkIconView            *icon_view);
void             btk_icon_view_unselect_all       (BtkIconView            *icon_view);
void             btk_icon_view_item_activated     (BtkIconView            *icon_view,
						   BtkTreePath            *path);
void             btk_icon_view_set_cursor         (BtkIconView            *icon_view,
						   BtkTreePath            *path,
						   BtkCellRenderer        *cell,
						   gboolean                start_editing);
gboolean         btk_icon_view_get_cursor         (BtkIconView            *icon_view,
						   BtkTreePath           **path,
						   BtkCellRenderer       **cell);
void             btk_icon_view_scroll_to_path     (BtkIconView            *icon_view,
                                                   BtkTreePath            *path,
						   gboolean                use_align,
						   gfloat                  row_align,
                                                   gfloat                  col_align);

/* Drag-and-Drop support */
void                   btk_icon_view_enable_model_drag_source (BtkIconView              *icon_view,
							       BdkModifierType           start_button_mask,
							       const BtkTargetEntry     *targets,
							       gint                      n_targets,
							       BdkDragAction             actions);
void                   btk_icon_view_enable_model_drag_dest   (BtkIconView              *icon_view,
							       const BtkTargetEntry     *targets,
							       gint                      n_targets,
							       BdkDragAction             actions);
void                   btk_icon_view_unset_model_drag_source  (BtkIconView              *icon_view);
void                   btk_icon_view_unset_model_drag_dest    (BtkIconView              *icon_view);
void                   btk_icon_view_set_reorderable          (BtkIconView              *icon_view,
							       gboolean                  reorderable);
gboolean               btk_icon_view_get_reorderable          (BtkIconView              *icon_view);


/* These are useful to implement your own custom stuff. */
void                   btk_icon_view_set_drag_dest_item       (BtkIconView              *icon_view,
							       BtkTreePath              *path,
							       BtkIconViewDropPosition   pos);
void                   btk_icon_view_get_drag_dest_item       (BtkIconView              *icon_view,
							       BtkTreePath             **path,
							       BtkIconViewDropPosition  *pos);
gboolean               btk_icon_view_get_dest_item_at_pos     (BtkIconView              *icon_view,
							       gint                      drag_x,
							       gint                      drag_y,
							       BtkTreePath             **path,
							       BtkIconViewDropPosition  *pos);
BdkPixmap             *btk_icon_view_create_drag_icon         (BtkIconView              *icon_view,
							       BtkTreePath              *path);

void    btk_icon_view_convert_widget_to_bin_window_coords     (BtkIconView *icon_view,
                                                               gint         wx,
                                                               gint         wy,
                                                               gint        *bx,
                                                               gint        *by);


void    btk_icon_view_set_tooltip_item                        (BtkIconView     *icon_view,
                                                               BtkTooltip      *tooltip,
                                                               BtkTreePath     *path);
void    btk_icon_view_set_tooltip_cell                        (BtkIconView     *icon_view,
                                                               BtkTooltip      *tooltip,
                                                               BtkTreePath     *path,
                                                               BtkCellRenderer *cell);
gboolean btk_icon_view_get_tooltip_context                    (BtkIconView       *icon_view,
                                                               gint              *x,
                                                               gint              *y,
                                                               gboolean           keyboard_tip,
                                                               BtkTreeModel     **model,
                                                               BtkTreePath      **path,
                                                               BtkTreeIter       *iter);
void     btk_icon_view_set_tooltip_column                     (BtkIconView       *icon_view,
                                                               gint               column);
gint     btk_icon_view_get_tooltip_column                     (BtkIconView       *icon_view);


G_END_DECLS

#endif /* __BTK_ICON_VIEW_H__ */
