/* BAIL - The BUNNY Accessibility Implementation Library
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

#ifndef __BAIL_CLIST_CELL_H__
#define __BAIL_CLIST_CELL_H__

#include <batk/batk.h>
#include <bail/bailcell.h>

B_BEGIN_DECLS

#define BAIL_TYPE_CLIST_CELL                     (bail_clist_cell_get_type ())
#define BAIL_CLIST_CELL(obj)                     (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_CLIST_CELL, BailCListCell))
#define BAIL_CLIST_CELL_CLASS(klass)             (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_CLIST_CELL, BailCListCellClass))
#define BAIL_IS_CLIST_CELL(obj)                  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_CLIST_CELL))
#define BAIL_IS_CLIST_CELL_CLASS(klass)          (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_CLIST_CELL))
#define BAIL_CLIST_CELL_GET_CLASS(obj)           (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_CLIST_CELL, BailCListCellClass))

typedef struct _BailCListCell                  BailCListCell;
typedef struct _BailCListCellClass             BailCListCellClass;

struct _BailCListCell
{
  BailCell parent;
};

GType bail_clist_cell_get_type (void);

struct _BailCListCellClass
{
  BailCellClass parent_class;
};

BatkObject *bail_clist_cell_new (void);

B_END_DECLS

#endif /* __BAIL_CLIST_CELL_H__ */
