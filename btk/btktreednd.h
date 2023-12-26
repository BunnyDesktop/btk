/* btktreednd.h
 * Copyright (C) 2001  Red Hat, Inc.
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

#ifndef __BTK_TREE_DND_H__
#define __BTK_TREE_DND_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btktreemodel.h>
#include <btk/btkdnd.h>

B_BEGIN_DECLS

#define BTK_TYPE_TREE_DRAG_SOURCE            (btk_tree_drag_source_get_type ())
#define BTK_TREE_DRAG_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TREE_DRAG_SOURCE, BtkTreeDragSource))
#define BTK_IS_TREE_DRAG_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TREE_DRAG_SOURCE))
#define BTK_TREE_DRAG_SOURCE_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BTK_TYPE_TREE_DRAG_SOURCE, BtkTreeDragSourceIface))

typedef struct _BtkTreeDragSource      BtkTreeDragSource; /* Dummy typedef */
typedef struct _BtkTreeDragSourceIface BtkTreeDragSourceIface;

struct _BtkTreeDragSourceIface
{
  GTypeInterface g_iface;

  /* VTable - not signals */

  gboolean     (* row_draggable)        (BtkTreeDragSource   *drag_source,
                                         BtkTreePath         *path);

  gboolean     (* drag_data_get)        (BtkTreeDragSource   *drag_source,
                                         BtkTreePath         *path,
                                         BtkSelectionData    *selection_data);

  gboolean     (* drag_data_delete)     (BtkTreeDragSource *drag_source,
                                         BtkTreePath       *path);
};

GType           btk_tree_drag_source_get_type   (void) B_GNUC_CONST;

/* Returns whether the given row can be dragged */
gboolean btk_tree_drag_source_row_draggable    (BtkTreeDragSource *drag_source,
                                                BtkTreePath       *path);

/* Deletes the given row, or returns FALSE if it can't */
gboolean btk_tree_drag_source_drag_data_delete (BtkTreeDragSource *drag_source,
                                                BtkTreePath       *path);

/* Fills in selection_data with type selection_data->target based on
 * the row denoted by path, returns TRUE if it does anything
 */
gboolean btk_tree_drag_source_drag_data_get    (BtkTreeDragSource *drag_source,
                                                BtkTreePath       *path,
                                                BtkSelectionData  *selection_data);

#define BTK_TYPE_TREE_DRAG_DEST            (btk_tree_drag_dest_get_type ())
#define BTK_TREE_DRAG_DEST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TREE_DRAG_DEST, BtkTreeDragDest))
#define BTK_IS_TREE_DRAG_DEST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TREE_DRAG_DEST))
#define BTK_TREE_DRAG_DEST_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BTK_TYPE_TREE_DRAG_DEST, BtkTreeDragDestIface))

typedef struct _BtkTreeDragDest      BtkTreeDragDest; /* Dummy typedef */
typedef struct _BtkTreeDragDestIface BtkTreeDragDestIface;

struct _BtkTreeDragDestIface
{
  GTypeInterface g_iface;

  /* VTable - not signals */

  gboolean     (* drag_data_received) (BtkTreeDragDest   *drag_dest,
                                       BtkTreePath       *dest,
                                       BtkSelectionData  *selection_data);

  gboolean     (* row_drop_possible)  (BtkTreeDragDest   *drag_dest,
                                       BtkTreePath       *dest_path,
				       BtkSelectionData  *selection_data);
};

GType           btk_tree_drag_dest_get_type   (void) B_GNUC_CONST;

/* Inserts a row before dest which contains data in selection_data,
 * or returns FALSE if it can't
 */
gboolean btk_tree_drag_dest_drag_data_received (BtkTreeDragDest   *drag_dest,
						BtkTreePath       *dest,
						BtkSelectionData  *selection_data);


/* Returns TRUE if we can drop before path; path may not exist. */
gboolean btk_tree_drag_dest_row_drop_possible  (BtkTreeDragDest   *drag_dest,
						BtkTreePath       *dest_path,
						BtkSelectionData  *selection_data);


/* The selection data would normally have target type BTK_TREE_MODEL_ROW in this
 * case. If the target is wrong these functions return FALSE.
 */
gboolean btk_tree_set_row_drag_data            (BtkSelectionData  *selection_data,
						BtkTreeModel      *tree_model,
						BtkTreePath       *path);
gboolean btk_tree_get_row_drag_data            (BtkSelectionData  *selection_data,
						BtkTreeModel     **tree_model,
						BtkTreePath      **path);

B_END_DECLS

#endif /* __BTK_TREE_DND_H__ */
