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
#include "bailcontainercell.h"

static void       bail_container_cell_class_init          (BailContainerCellClass *klass);
static void       bail_container_cell_init                (BailContainerCell   *cell);
static void       bail_container_cell_finalize            (BObject             *obj);


static void       _bail_container_cell_recompute_child_indices 
                                                          (BailContainerCell *container);

static void       bail_container_cell_refresh_child_index (BailCell *cell);

static bint       bail_container_cell_get_n_children      (BatkObject *obj);

static BatkObject* bail_container_cell_ref_child           (BatkObject *obj,
                                                           bint      child);

G_DEFINE_TYPE (BailContainerCell, bail_container_cell, BAIL_TYPE_CELL)

static void 
bail_container_cell_class_init (BailContainerCellClass *klass)
{
  BatkObjectClass *class = BATK_OBJECT_CLASS(klass);
  BObjectClass *g_object_class = B_OBJECT_CLASS (klass);

  g_object_class->finalize = bail_container_cell_finalize;

  class->get_n_children = bail_container_cell_get_n_children;
  class->ref_child = bail_container_cell_ref_child;
}

static void
bail_container_cell_init (BailContainerCell   *cell)
{
}

BailContainerCell * 
bail_container_cell_new (void)
{
  BObject *object;
  BatkObject *batk_object;
  BailContainerCell *container;

  object = g_object_new (BAIL_TYPE_CONTAINER_CELL, NULL);

  g_return_val_if_fail (object != NULL, NULL);

  batk_object = BATK_OBJECT (object);
  batk_object->role = BATK_ROLE_TABLE_CELL;

  container = BAIL_CONTAINER_CELL(object);
  container->children = NULL;
  container->NChildren = 0;
  return container;
}

static void
bail_container_cell_finalize (BObject *obj)
{
  BailContainerCell *container = BAIL_CONTAINER_CELL (obj);
  GList *list;

  list = container->children;
  while (list)
  {
    g_object_unref (list->data);
    list = list->next;
  }
  g_list_free (container->children);
  
  B_OBJECT_CLASS (bail_container_cell_parent_class)->finalize (obj);
}


void
bail_container_cell_add_child (BailContainerCell *container,
			       BailCell *child)
{
  bint child_index;

  g_return_if_fail (BAIL_IS_CONTAINER_CELL(container));
  g_return_if_fail (BAIL_IS_CELL(child));

  child_index = container->NChildren++;
  container->children = g_list_append (container->children, (bpointer) child);
  child->index = child_index;
  batk_object_set_parent (BATK_OBJECT (child), BATK_OBJECT (container));
  child->refresh_index = bail_container_cell_refresh_child_index;
}


void
bail_container_cell_remove_child (BailContainerCell *container,
				  BailCell *child)
{
  g_return_if_fail (BAIL_IS_CONTAINER_CELL(container));
  g_return_if_fail (BAIL_IS_CELL(child));
  g_return_if_fail (container->NChildren > 0);

  container->children = g_list_remove (container->children, (bpointer) child);
  _bail_container_cell_recompute_child_indices (container);
  container->NChildren--;
}


static void
_bail_container_cell_recompute_child_indices (BailContainerCell *container)
{
  bint cur_index = 0;
  GList *temp_list;

  g_return_if_fail (BAIL_IS_CONTAINER_CELL(container));

  for (temp_list = container->children; temp_list; temp_list = temp_list->next)
    {
      BAIL_CELL(temp_list->data)->index = cur_index;
      cur_index++;
    }
}


static void
bail_container_cell_refresh_child_index (BailCell *cell)
{
  BailContainerCell *container;
  g_return_if_fail (BAIL_IS_CELL(cell));
  container = BAIL_CONTAINER_CELL (batk_object_get_parent (BATK_OBJECT(cell)));
  g_return_if_fail (BAIL_IS_CONTAINER_CELL (container));
  _bail_container_cell_recompute_child_indices (container);
}



static bint
bail_container_cell_get_n_children (BatkObject *obj)
{
  BailContainerCell *cell;
  g_return_val_if_fail (BAIL_IS_CONTAINER_CELL(obj), 0);
  cell = BAIL_CONTAINER_CELL(obj);
  return cell->NChildren;
}


static BatkObject *
bail_container_cell_ref_child (BatkObject *obj,
			       bint       child)
{
  BailContainerCell *cell;
  GList *list_node;

  g_return_val_if_fail (BAIL_IS_CONTAINER_CELL(obj), NULL);
  cell = BAIL_CONTAINER_CELL(obj);
  
  list_node = g_list_nth (cell->children, child);
  if (!list_node)
    return NULL;

  return g_object_ref (BATK_OBJECT (list_node->data));
}
