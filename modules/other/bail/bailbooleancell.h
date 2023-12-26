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

#ifndef __BAIL_BOOLEAN_CELL_H__
#define __BAIL_BOOLEAN_CELL_H__

#include <batk/batk.h>
#include <bail/bailrenderercell.h>

B_BEGIN_DECLS

#define BAIL_TYPE_BOOLEAN_CELL            (bail_boolean_cell_get_type ())
#define BAIL_BOOLEAN_CELL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_BOOLEAN_CELL, BailBooleanCell))
#define BAIL_BOOLEAN_CELL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_BOOLEAN_CELL, BailBooleanCellClass))
#define BAIL_IS_BOOLEAN_CELL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_BOOLEAN_CELL))
#define BAIL_IS_BOOLEAN_CELL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_BOOLEAN_CELL))
#define BAIL_BOOLEAN_CELL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_BOOLEAN_CELL, BailBooleanCellClass))

typedef struct _BailBooleanCell                  BailBooleanCell;
typedef struct _BailBooleanCellClass             BailBooleanCellClass;

struct _BailBooleanCell
{
  BailRendererCell parent;
  gboolean cell_value;
  gboolean cell_sensitive;
};

 GType bail_boolean_cell_get_type (void);

struct _BailBooleanCellClass
{
  BailRendererCellClass parent_class;
};

BatkObject *bail_boolean_cell_new (void);

B_END_DECLS

#endif /* __BAIL_TREE_VIEW_BOOLEAN_CELL_H__ */
