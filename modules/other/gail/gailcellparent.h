/* BAIL - The GNOME Accessibility Implementation Library
 *
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

#ifndef __BAIL_CELL_PARENT_H__
#define __BAIL_CELL_PARENT_H__

#include <batk/batk.h>
#include <bail/bailcell.h>

G_BEGIN_DECLS

/*
 * The BailCellParent interface should be supported by any object which
 * contains children which are flyweights, i.e. do not have corresponding
 * widgets and the children need help from their parent to provide
 * functionality. One example is BailTreeView where the children BailCell
 * need help from the BailTreeView in order to implement 
 * batk_component_get_extents
 */

#define BAIL_TYPE_CELL_PARENT            (bail_cell_parent_get_type ())
#define BAIL_IS_CELL_PARENT(obj)         G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_CELL_PARENT)
#define BAIL_CELL_PARENT(obj)            G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_CELL_PARENT, BailCellParent)
#define BAIL_CELL_PARENT_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BAIL_TYPE_CELL_PARENT, BailCellParentIface))

#ifndef _TYPEDEF_BAIL_CELL_PARENT_
#define _TYPEDEF_BAIL_CELL_PARENT_
typedef struct _BailCellParent BailCellParent;
#endif
typedef struct _BailCellParentIface BailCellParentIface;

struct _BailCellParentIface
{
  GTypeInterface parent;
  void                  ( *get_cell_extents)      (BailCellParent        *parent,
                                                   BailCell              *cell,
                                                   gint                  *x,
                                                   gint                  *y,
                                                   gint                  *width,
                                                   gint                  *height,
                                                   BatkCoordType          coord_type);
  void                  ( *get_cell_area)         (BailCellParent        *parent,
                                                   BailCell              *cell,
                                                   BdkRectangle          *cell_rect);
  gboolean              ( *grab_focus)            (BailCellParent        *parent,
                                                   BailCell              *cell);
};

GType  bail_cell_parent_get_type               (void);

void   bail_cell_parent_get_cell_extents       (BailCellParent        *parent,
                                                BailCell              *cell,
                                                gint                  *x,
                                                gint                  *y,
                                                gint                  *width,
                                                gint                  *height,
                                                BatkCoordType          coord_type
);
void  bail_cell_parent_get_cell_area           (BailCellParent        *parent,
                                                BailCell              *cell,
                                                BdkRectangle          *cell_rect);
gboolean bail_cell_parent_grab_focus           (BailCellParent        *parent,
                                                BailCell              *cell);

G_END_DECLS

#endif /* __BAIL_CELL_PARENT_H__ */
