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
#include "bailbooleancell.h"

static void      bail_boolean_cell_class_init          (BailBooleanCellClass *klass);
static void      bail_boolean_cell_init                (BailBooleanCell *cell);
/* Misc */

static gboolean bail_boolean_cell_update_cache         (BailRendererCell     *cell,
                                                        gboolean             emit_change_signal);

gchar *bail_boolean_cell_property_list[] = {
  "active",
  "radio",
  "sensitive",
  NULL
};

G_DEFINE_TYPE (BailBooleanCell, bail_boolean_cell, BAIL_TYPE_RENDERER_CELL)

static void 
bail_boolean_cell_class_init (BailBooleanCellClass *klass)
{
  BailRendererCellClass *renderer_cell_class = BAIL_RENDERER_CELL_CLASS (klass);

  renderer_cell_class->update_cache = bail_boolean_cell_update_cache;
  renderer_cell_class->property_list = bail_boolean_cell_property_list;
}

static void
bail_boolean_cell_init (BailBooleanCell *cell)
{
}

BatkObject* 
bail_boolean_cell_new (void)
{
  GObject *object;
  BatkObject *batk_object;
  BailRendererCell *cell;
  BailBooleanCell *boolean_cell;

  object = g_object_new (BAIL_TYPE_BOOLEAN_CELL, NULL);

  g_return_val_if_fail (object != NULL, NULL);

  batk_object = BATK_OBJECT (object);
  batk_object->role = BATK_ROLE_TABLE_CELL;

  cell = BAIL_RENDERER_CELL(object);
  boolean_cell = BAIL_BOOLEAN_CELL(object);

  cell->renderer = btk_cell_renderer_toggle_new ();
  g_object_ref_sink (cell->renderer);
  boolean_cell->cell_value = FALSE;
  boolean_cell->cell_sensitive = TRUE;
  return batk_object;
}

static gboolean
bail_boolean_cell_update_cache (BailRendererCell *cell, 
                                gboolean         emit_change_signal)
{
  BailBooleanCell *boolean_cell = BAIL_BOOLEAN_CELL (cell);
  gboolean rv = FALSE;
  gboolean new_boolean;
  gboolean new_sensitive;

  g_object_get (G_OBJECT(cell->renderer), "active", &new_boolean,
                                          "sensitive", &new_sensitive, NULL);

  if (boolean_cell->cell_value != new_boolean)
    {
      rv = TRUE;
      boolean_cell->cell_value = !(boolean_cell->cell_value);

      /* Update cell's state */

    if (new_boolean)
      bail_cell_add_state (BAIL_CELL (cell), BATK_STATE_CHECKED, emit_change_signal);
    else
      bail_cell_remove_state (BAIL_CELL (cell), BATK_STATE_CHECKED, emit_change_signal);
    }

  if (boolean_cell->cell_sensitive != new_sensitive)
    {
      rv = TRUE;
      boolean_cell->cell_sensitive = !(boolean_cell->cell_sensitive);

      /* Update cell's state */

      if (new_sensitive)
        bail_cell_add_state (BAIL_CELL (cell), BATK_STATE_SENSITIVE, emit_change_signal);
      else
        bail_cell_remove_state (BAIL_CELL (cell), BATK_STATE_SENSITIVE, emit_change_signal);
    }

  return rv;
}
