/* BAIL - The GNOME Accessibility Implementation Library
 * Copyright 2001 Sun Microsystems Inc.
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

#ifndef __BAIL_TREE_VIEW_H__
#define __BAIL_TREE_VIEW_H__

#include <btk/btk.h>
#include <bail/bailcontainer.h>
#include <bail/bailcell.h>

G_BEGIN_DECLS

#define BAIL_TYPE_TREE_VIEW                  (bail_tree_view_get_type ())
#define BAIL_TREE_VIEW(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_TREE_VIEW, BailTreeView))
#define BAIL_TREE_VIEW_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_TREE_VIEW, BailTreeViewClass))
#define BAIL_IS_TREE_VIEW(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_TREE_VIEW))
#define BAIL_IS_TREE_VIEW_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_TREE_VIEW))
#define BAIL_TREE_VIEW_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_TREE_VIEW, BailTreeViewClass))

typedef struct _BailTreeView              BailTreeView;
typedef struct _BailTreeViewClass         BailTreeViewClass;

struct _BailTreeView
{
  BailContainer parent;

  BatkObject*	caption;
  BatkObject*	summary;
  gint          n_children_deleted;
  GArray*       col_data;
  GArray*	row_data;
  GList*        cell_data;
  BtkTreeModel  *tree_model;
  BatkObject     *focus_cell;
  BtkAdjustment *old_hadj;
  BtkAdjustment *old_vadj;
  guint         idle_expand_id;
  guint         idle_garbage_collect_id;
  guint         idle_cursor_changed_id;
  BtkTreePath   *idle_expand_path;
  gboolean      garbage_collection_pending;
};

GType bail_tree_view_get_type (void);

struct _BailTreeViewClass
{
  BailContainerClass parent_class;
};

BatkObject* bail_tree_view_ref_focus_cell (BtkTreeView *treeview);
 
G_END_DECLS

#endif /* __BAIL_TREE_VIEW_H__ */
