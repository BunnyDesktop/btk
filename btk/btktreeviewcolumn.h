/* btktreeviewcolumn.h
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

#ifndef __BTK_TREE_VIEW_COLUMN_H__
#define __BTK_TREE_VIEW_COLUMN_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcellrenderer.h>
#include <btk/btktreemodel.h>
#include <btk/btktreesortable.h>

/* Not needed, retained for compatibility -Yosh */
#include <btk/btkobject.h>


B_BEGIN_DECLS


#define BTK_TYPE_TREE_VIEW_COLUMN	     (btk_tree_view_column_get_type ())
#define BTK_TREE_VIEW_COLUMN(obj)	     (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TREE_VIEW_COLUMN, BtkTreeViewColumn))
#define BTK_TREE_VIEW_COLUMN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TREE_VIEW_COLUMN, BtkTreeViewColumnClass))
#define BTK_IS_TREE_VIEW_COLUMN(obj)	     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TREE_VIEW_COLUMN))
#define BTK_IS_TREE_VIEW_COLUMN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TREE_VIEW_COLUMN))
#define BTK_TREE_VIEW_COLUMN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TREE_VIEW_COLUMN, BtkTreeViewColumnClass))

typedef enum
{
  BTK_TREE_VIEW_COLUMN_GROW_ONLY,
  BTK_TREE_VIEW_COLUMN_AUTOSIZE,
  BTK_TREE_VIEW_COLUMN_FIXED
} BtkTreeViewColumnSizing;

typedef struct _BtkTreeViewColumn      BtkTreeViewColumn;
typedef struct _BtkTreeViewColumnClass BtkTreeViewColumnClass;

typedef void (* BtkTreeCellDataFunc) (BtkTreeViewColumn *tree_column,
				      BtkCellRenderer   *cell,
				      BtkTreeModel      *tree_model,
				      BtkTreeIter       *iter,
				      gpointer           data);


struct _BtkTreeViewColumn
{
  BtkObject parent;

  BtkWidget *GSEAL (tree_view);
  BtkWidget *GSEAL (button);
  BtkWidget *GSEAL (child);
  BtkWidget *GSEAL (arrow);
  BtkWidget *GSEAL (alignment);
  BdkWindow *GSEAL (window);
  BtkCellEditable *GSEAL (editable_widget);
  gfloat GSEAL (xalign);
  guint GSEAL (property_changed_signal);
  gint GSEAL (spacing);

  /* Sizing fields */
  /* see btk+/doc/tree-column-sizing.txt for more information on them */
  BtkTreeViewColumnSizing GSEAL (column_type);
  gint GSEAL (requested_width);
  gint GSEAL (button_request);
  gint GSEAL (resized_width);
  gint GSEAL (width);
  gint GSEAL (fixed_width);
  gint GSEAL (min_width);
  gint GSEAL (max_width);

  /* dragging columns */
  gint GSEAL (drag_x);
  gint GSEAL (drag_y);

  gchar *GSEAL (title);
  GList *GSEAL (cell_list);

  /* Sorting */
  guint GSEAL (sort_clicked_signal);
  guint GSEAL (sort_column_changed_signal);
  gint GSEAL (sort_column_id);
  BtkSortType GSEAL (sort_order);

  /* Flags */
  guint GSEAL (visible)             : 1;
  guint GSEAL (resizable)           : 1;
  guint GSEAL (clickable)           : 1;
  guint GSEAL (dirty)               : 1;
  guint GSEAL (show_sort_indicator) : 1;
  guint GSEAL (maybe_reordered)     : 1;
  guint GSEAL (reorderable)         : 1;
  guint GSEAL (use_resized_width)   : 1;
  guint GSEAL (expand)              : 1;
};

struct _BtkTreeViewColumnClass
{
  BtkObjectClass parent_class;

  void (*clicked) (BtkTreeViewColumn *tree_column);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

GType                   btk_tree_view_column_get_type            (void) B_GNUC_CONST;
BtkTreeViewColumn      *btk_tree_view_column_new                 (void);
BtkTreeViewColumn      *btk_tree_view_column_new_with_attributes (const gchar             *title,
								  BtkCellRenderer         *cell,
								  ...) B_GNUC_NULL_TERMINATED;
void                    btk_tree_view_column_pack_start          (BtkTreeViewColumn       *tree_column,
								  BtkCellRenderer         *cell,
								  gboolean                 expand);
void                    btk_tree_view_column_pack_end            (BtkTreeViewColumn       *tree_column,
								  BtkCellRenderer         *cell,
								  gboolean                 expand);
void                    btk_tree_view_column_clear               (BtkTreeViewColumn       *tree_column);
#ifndef BTK_DISABLE_DEPRECATED
GList                  *btk_tree_view_column_get_cell_renderers  (BtkTreeViewColumn       *tree_column);
#endif
void                    btk_tree_view_column_add_attribute       (BtkTreeViewColumn       *tree_column,
								  BtkCellRenderer         *cell_renderer,
								  const gchar             *attribute,
								  gint                     column);
void                    btk_tree_view_column_set_attributes      (BtkTreeViewColumn       *tree_column,
								  BtkCellRenderer         *cell_renderer,
								  ...) B_GNUC_NULL_TERMINATED;
void                    btk_tree_view_column_set_cell_data_func  (BtkTreeViewColumn       *tree_column,
								  BtkCellRenderer         *cell_renderer,
								  BtkTreeCellDataFunc      func,
								  gpointer                 func_data,
								  GDestroyNotify           destroy);
void                    btk_tree_view_column_clear_attributes    (BtkTreeViewColumn       *tree_column,
								  BtkCellRenderer         *cell_renderer);
void                    btk_tree_view_column_set_spacing         (BtkTreeViewColumn       *tree_column,
								  gint                     spacing);
gint                    btk_tree_view_column_get_spacing         (BtkTreeViewColumn       *tree_column);
void                    btk_tree_view_column_set_visible         (BtkTreeViewColumn       *tree_column,
								  gboolean                 visible);
gboolean                btk_tree_view_column_get_visible         (BtkTreeViewColumn       *tree_column);
void                    btk_tree_view_column_set_resizable       (BtkTreeViewColumn       *tree_column,
								  gboolean                 resizable);
gboolean                btk_tree_view_column_get_resizable       (BtkTreeViewColumn       *tree_column);
void                    btk_tree_view_column_set_sizing          (BtkTreeViewColumn       *tree_column,
								  BtkTreeViewColumnSizing  type);
BtkTreeViewColumnSizing btk_tree_view_column_get_sizing          (BtkTreeViewColumn       *tree_column);
gint                    btk_tree_view_column_get_width           (BtkTreeViewColumn       *tree_column);
gint                    btk_tree_view_column_get_fixed_width     (BtkTreeViewColumn       *tree_column);
void                    btk_tree_view_column_set_fixed_width     (BtkTreeViewColumn       *tree_column,
								  gint                     fixed_width);
void                    btk_tree_view_column_set_min_width       (BtkTreeViewColumn       *tree_column,
								  gint                     min_width);
gint                    btk_tree_view_column_get_min_width       (BtkTreeViewColumn       *tree_column);
void                    btk_tree_view_column_set_max_width       (BtkTreeViewColumn       *tree_column,
								  gint                     max_width);
gint                    btk_tree_view_column_get_max_width       (BtkTreeViewColumn       *tree_column);
void                    btk_tree_view_column_clicked             (BtkTreeViewColumn       *tree_column);



/* Options for manipulating the column headers
 */
void                    btk_tree_view_column_set_title           (BtkTreeViewColumn       *tree_column,
								  const gchar             *title);
const gchar *           btk_tree_view_column_get_title           (BtkTreeViewColumn       *tree_column);
void                    btk_tree_view_column_set_expand          (BtkTreeViewColumn       *tree_column,
								  gboolean                 expand);
gboolean                btk_tree_view_column_get_expand          (BtkTreeViewColumn       *tree_column);
void                    btk_tree_view_column_set_clickable       (BtkTreeViewColumn       *tree_column,
								  gboolean                 clickable);
gboolean                btk_tree_view_column_get_clickable       (BtkTreeViewColumn       *tree_column);
void                    btk_tree_view_column_set_widget          (BtkTreeViewColumn       *tree_column,
								  BtkWidget               *widget);
BtkWidget              *btk_tree_view_column_get_widget          (BtkTreeViewColumn       *tree_column);
void                    btk_tree_view_column_set_alignment       (BtkTreeViewColumn       *tree_column,
								  gfloat                   xalign);
gfloat                  btk_tree_view_column_get_alignment       (BtkTreeViewColumn       *tree_column);
void                    btk_tree_view_column_set_reorderable     (BtkTreeViewColumn       *tree_column,
								  gboolean                 reorderable);
gboolean                btk_tree_view_column_get_reorderable     (BtkTreeViewColumn       *tree_column);



/* You probably only want to use btk_tree_view_column_set_sort_column_id.  The
 * other sorting functions exist primarily to let others do their own custom sorting.
 */
void                    btk_tree_view_column_set_sort_column_id  (BtkTreeViewColumn       *tree_column,
								  gint                     sort_column_id);
gint                    btk_tree_view_column_get_sort_column_id  (BtkTreeViewColumn       *tree_column);
void                    btk_tree_view_column_set_sort_indicator  (BtkTreeViewColumn       *tree_column,
								  gboolean                 setting);
gboolean                btk_tree_view_column_get_sort_indicator  (BtkTreeViewColumn       *tree_column);
void                    btk_tree_view_column_set_sort_order      (BtkTreeViewColumn       *tree_column,
								  BtkSortType              order);
BtkSortType             btk_tree_view_column_get_sort_order      (BtkTreeViewColumn       *tree_column);


/* These functions are meant primarily for interaction between the BtkTreeView and the column.
 */
void                    btk_tree_view_column_cell_set_cell_data  (BtkTreeViewColumn       *tree_column,
								  BtkTreeModel            *tree_model,
								  BtkTreeIter             *iter,
								  gboolean                 is_expander,
								  gboolean                 is_expanded);
void                    btk_tree_view_column_cell_get_size       (BtkTreeViewColumn       *tree_column,
								  const BdkRectangle      *cell_area,
								  gint                    *x_offset,
								  gint                    *y_offset,
								  gint                    *width,
								  gint                    *height);
gboolean                btk_tree_view_column_cell_is_visible     (BtkTreeViewColumn       *tree_column);
void                    btk_tree_view_column_focus_cell          (BtkTreeViewColumn       *tree_column,
								  BtkCellRenderer         *cell);
gboolean                btk_tree_view_column_cell_get_position   (BtkTreeViewColumn       *tree_column,
					                          BtkCellRenderer         *cell_renderer,
					                          gint                    *start_pos,
					                          gint                    *width);
void                    btk_tree_view_column_queue_resize        (BtkTreeViewColumn       *tree_column);
BtkWidget              *btk_tree_view_column_get_tree_view       (BtkTreeViewColumn       *tree_column);


B_END_DECLS


#endif /* __BTK_TREE_VIEW_COLUMN_H__ */
