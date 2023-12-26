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

#ifndef __BAIL_RENDERER_CELL_H__
#define __BAIL_RENDERER_CELL_H__

#include <batk/batk.h>
#include <bail/bailcell.h>

B_BEGIN_DECLS

#define BAIL_TYPE_RENDERER_CELL            (bail_renderer_cell_get_type ())
#define BAIL_RENDERER_CELL(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_RENDERER_CELL, BailRendererCell))
#define BAIL_RENDERER_CELL_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_RENDERER_CELL, BailRendererCellClass))
#define BAIL_IS_RENDERER_CELL(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_RENDERER_CELL))
#define BAIL_IS_RENDERER_CELL_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_RENDERER_CELL))
#define BAIL_RENDERER_CELL_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_RENDERER_CELL, BailRendererCellClass))

typedef struct _BailRendererCell                  BailRendererCell;
typedef struct _BailRendererCellClass             BailRendererCellClass;

struct _BailRendererCell
{
  BailCell parent;
  BtkCellRenderer *renderer;
};

GType bail_renderer_cell_get_type (void);

struct _BailRendererCellClass
{
  BailCellClass parent_class;
  gchar **property_list;
  gboolean (*update_cache)(BailRendererCell *cell, gboolean emit_change_signal);
};

gboolean
bail_renderer_cell_update_cache (BailRendererCell *cell, gboolean emit_change_signal);

BatkObject *bail_renderer_cell_new (void);

B_END_DECLS

#endif /* __BAIL_TREE_VIEW_TEXT_CELL_H__ */
