/* btkcelllayout.h
 * Copyright (C) 2003  Kristian Rietveld  <kris@btk.org>
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

#ifndef __BTK_CELL_LAYOUT_H__
#define __BTK_CELL_LAYOUT_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcellrenderer.h>
#include <btk/btktreeviewcolumn.h>
#include <btk/btkbuildable.h>
#include <btk/btkbuilder.h>

B_BEGIN_DECLS

#define BTK_TYPE_CELL_LAYOUT            (btk_cell_layout_get_type ())
#define BTK_CELL_LAYOUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_CELL_LAYOUT, BtkCellLayout))
#define BTK_IS_CELL_LAYOUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_CELL_LAYOUT))
#define BTK_CELL_LAYOUT_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BTK_TYPE_CELL_LAYOUT, BtkCellLayoutIface))

typedef struct _BtkCellLayout           BtkCellLayout; /* dummy typedef */
typedef struct _BtkCellLayoutIface      BtkCellLayoutIface;

/* keep in sync with BtkTreeCellDataFunc */
typedef void (* BtkCellLayoutDataFunc) (BtkCellLayout   *cell_layout,
                                        BtkCellRenderer *cell,
                                        BtkTreeModel    *tree_model,
                                        BtkTreeIter     *iter,
                                        gpointer         data);

struct _BtkCellLayoutIface
{
  GTypeInterface g_iface;

  /* Virtual Table */
  void (* pack_start)         (BtkCellLayout         *cell_layout,
                               BtkCellRenderer       *cell,
                               gboolean               expand);
  void (* pack_end)           (BtkCellLayout         *cell_layout,
                               BtkCellRenderer       *cell,
                               gboolean               expand);
  void (* clear)              (BtkCellLayout         *cell_layout);
  void (* add_attribute)      (BtkCellLayout         *cell_layout,
                               BtkCellRenderer       *cell,
                               const gchar           *attribute,
                               gint                   column);
  void (* set_cell_data_func) (BtkCellLayout         *cell_layout,
                               BtkCellRenderer       *cell,
                               BtkCellLayoutDataFunc  func,
                               gpointer               func_data,
                               GDestroyNotify         destroy);
  void (* clear_attributes)   (BtkCellLayout         *cell_layout,
                               BtkCellRenderer       *cell);
  void (* reorder)            (BtkCellLayout         *cell_layout,
                               BtkCellRenderer       *cell,
                               gint                   position);
  GList* (* get_cells)        (BtkCellLayout         *cell_layout);
};

GType btk_cell_layout_get_type           (void) B_GNUC_CONST;
void  btk_cell_layout_pack_start         (BtkCellLayout         *cell_layout,
                                          BtkCellRenderer       *cell,
                                          gboolean               expand);
void  btk_cell_layout_pack_end           (BtkCellLayout         *cell_layout,
                                          BtkCellRenderer       *cell,
                                          gboolean               expand);
GList *btk_cell_layout_get_cells         (BtkCellLayout         *cell_layout);
void  btk_cell_layout_clear              (BtkCellLayout         *cell_layout);
void  btk_cell_layout_set_attributes     (BtkCellLayout         *cell_layout,
                                          BtkCellRenderer       *cell,
                                          ...) B_GNUC_NULL_TERMINATED;
void  btk_cell_layout_add_attribute      (BtkCellLayout         *cell_layout,
                                          BtkCellRenderer       *cell,
                                          const gchar           *attribute,
                                          gint                   column);
void  btk_cell_layout_set_cell_data_func (BtkCellLayout         *cell_layout,
                                          BtkCellRenderer       *cell,
                                          BtkCellLayoutDataFunc  func,
                                          gpointer               func_data,
                                          GDestroyNotify         destroy);
void  btk_cell_layout_clear_attributes   (BtkCellLayout         *cell_layout,
                                          BtkCellRenderer       *cell);
void  btk_cell_layout_reorder            (BtkCellLayout         *cell_layout,
                                          BtkCellRenderer       *cell,
                                          gint                   position);
gboolean _btk_cell_layout_buildable_custom_tag_start (BtkBuildable  *buildable,
						      BtkBuilder    *builder,
						      GObject       *child,
						      const gchar   *tagname,
						      GMarkupParser *parser,
						      gpointer      *data);
void _btk_cell_layout_buildable_custom_tag_end       (BtkBuildable  *buildable,
						      BtkBuilder    *builder,
						      GObject       *child,
						      const gchar   *tagname,
						      gpointer      *data);
void _btk_cell_layout_buildable_add_child            (BtkBuildable  *buildable,
						      BtkBuilder    *builder,
						      GObject       *child,
						      const gchar   *type);

B_END_DECLS

#endif /* __BTK_CELL_LAYOUT_H__ */
