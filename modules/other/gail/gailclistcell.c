/* BAIL - The GNOME Accessibility Implementation Library
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

#undef BTK_DISABLE_DEPRECATED

#include <btk/btk.h>
#include "bailclistcell.h"

static void	 bail_clist_cell_class_init        (BailCListCellClass *klass);
static void	 bail_clist_cell_init              (BailCListCell      *cell);

static const gchar* bail_clist_cell_get_name (BatkObject *accessible);

G_DEFINE_TYPE (BailCListCell, bail_clist_cell, BAIL_TYPE_CELL)

static void	 
bail_clist_cell_class_init (BailCListCellClass *klass)
{
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);

  class->get_name = bail_clist_cell_get_name;
}

static void
bail_clist_cell_init (BailCListCell *cell)
{
}

BatkObject* 
bail_clist_cell_new (void)
{
  GObject *object;
  BatkObject *batk_object;

  object = g_object_new (BAIL_TYPE_CLIST_CELL, NULL);

  g_return_val_if_fail (object != NULL, NULL);

  batk_object = BATK_OBJECT (object);
  batk_object->role = BATK_ROLE_TABLE_CELL;

  g_return_val_if_fail (!BATK_IS_TEXT (batk_object), NULL);
  
  return batk_object;
}

static const gchar*
bail_clist_cell_get_name (BatkObject *accessible)
{
  if (accessible->name)
    return accessible->name;
  else
    {
      /*
       * Get the cell's text if it exists
       */
      BailCell *cell = BAIL_CELL (accessible);
      BtkWidget* widget = cell->widget;
      BtkCellType cell_type;
      BtkCList *clist;
      gchar *text = NULL;
      gint row, column;

      if (widget == NULL)
        /*
         * State is defunct
         */
        return NULL;
 
      clist = BTK_CLIST (widget);
      g_return_val_if_fail (clist->columns, NULL);
      row = cell->index / clist->columns;
      column = cell->index % clist->columns;
      cell_type = btk_clist_get_cell_type (clist, row, column);
      switch (cell_type)
        {
        case BTK_CELL_TEXT:
          btk_clist_get_text (clist, row, column, &text);
          break;
        case BTK_CELL_PIXTEXT:
          btk_clist_get_pixtext (clist, row, column, &text, NULL, NULL, NULL);
          break;
        default:
          break;
        }
      return text;
    }
}
