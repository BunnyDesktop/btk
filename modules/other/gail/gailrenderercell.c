/* BAIL - The BUNNY Accessibility Enabling Library
 * Copyright 2001 Sun Microsystems Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <btk/btk.h>
#include "bailrenderercell.h"

static void      bail_renderer_cell_class_init          (BailRendererCellClass *klass);
static void      bail_renderer_cell_init                (BailRendererCell      *renderer_cell);

static void      bail_renderer_cell_finalize            (GObject               *object);

G_DEFINE_TYPE (BailRendererCell, bail_renderer_cell, BAIL_TYPE_CELL)

static void 
bail_renderer_cell_class_init (BailRendererCellClass *klass)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (klass);

  klass->property_list = NULL;

  bobject_class->finalize = bail_renderer_cell_finalize;
}

static void
bail_renderer_cell_init (BailRendererCell *renderer_cell)
{
  renderer_cell->renderer = NULL;
}

static void
bail_renderer_cell_finalize (GObject  *object)
{
  BailRendererCell *renderer_cell = BAIL_RENDERER_CELL (object);

  if (renderer_cell->renderer)
    g_object_unref (renderer_cell->renderer);
  G_OBJECT_CLASS (bail_renderer_cell_parent_class)->finalize (object);
}

gboolean
bail_renderer_cell_update_cache (BailRendererCell *cell, 
                                 gboolean         emit_change_signal)
{
  BailRendererCellClass *class = BAIL_RENDERER_CELL_GET_CLASS(cell);
  if (class->update_cache)
    return (class->update_cache)(cell, emit_change_signal);
  return FALSE;
}

BatkObject*
bail_renderer_cell_new (void)
{
  GObject *object;
  BatkObject *batk_object;
  BailRendererCell *cell;

  object = g_object_new (BAIL_TYPE_RENDERER_CELL, NULL);

  g_return_val_if_fail (object != NULL, NULL);

  batk_object = BATK_OBJECT (object);
  batk_object->role = BATK_ROLE_TABLE_CELL;

  cell = BAIL_RENDERER_CELL(object);

  return batk_object;
}
