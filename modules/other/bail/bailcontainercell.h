/* BAIL - The BUNNY Accessibility Enabling Library
 * Copyright 2001 Sun Microsystems Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __BAIL_CONTAINER_CELL_H__
#define __BAIL_CONTAINER_CELL_H__

#include <batk/batk.h>
#include <bail/bailcell.h>

B_BEGIN_DECLS

#define BAIL_TYPE_CONTAINER_CELL            (bail_container_cell_get_type ())
#define BAIL_CONTAINER_CELL(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_CONTAINER_CELL, BailContainerCell))
#define BAIL_CONTAINER_CELL_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_CONTAINER_CELL, BailContainerCellClass))
#define BAIL_IS_CONTAINER_CELL(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_CONTAINER_CELL))
#define BAIL_IS_CONTAINER_CELL_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_CONTAINER_CELL))
#define BAIL_CONTAINER_CELL_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_CONTAINER_CELL, BailContainerCellClass))

typedef struct _BailContainerCell                  BailContainerCell;
typedef struct _BailContainerCellClass             BailContainerCellClass;

struct _BailContainerCell
{
  BailCell parent;
  GList *children;
  gint NChildren;
};

GType bail_container_cell_get_type (void);

struct _BailContainerCellClass
{
  BailCellClass parent_class;
};

BailContainerCell *
bail_container_cell_new (void);

void
bail_container_cell_add_child (BailContainerCell *container,
			       BailCell *child);

void
bail_container_cell_remove_child (BailContainerCell *container,
				  BailCell *child);

B_END_DECLS

#endif /* __BAIL_TREE_VIEW_TEXT_CELL_H__ */
