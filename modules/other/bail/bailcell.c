/* BAIL - The BUNNY Accessibility Implementation Library
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

#include <string.h>
#include <stdlib.h>
#include <btk/btk.h>
#include "bailcontainercell.h"
#include "bailcell.h"
#include "bailcellparent.h"

static void	    bail_cell_class_init          (BailCellClass       *klass);
static void         bail_cell_destroyed           (BtkWidget           *widget,
                                                   BailCell            *cell);

static void         bail_cell_init                (BailCell            *cell);
static void         bail_cell_object_finalize     (BObject             *cell);
static BatkStateSet* bail_cell_ref_state_set       (BatkObject           *obj);
static bint         bail_cell_get_index_in_parent (BatkObject           *obj);

/* BatkAction */

static void         batk_action_interface_init 
                                                  (BatkActionIface      *iface);
static ActionInfo * _bail_cell_get_action_info    (BailCell            *cell,
			                           bint                index);
static void         _bail_cell_destroy_action_info 
                                                  (bpointer            data,
				                   bpointer            user_data);

static bint         bail_cell_action_get_n_actions 
                                                  (BatkAction           *action);
static const bchar *
                    bail_cell_action_get_name     (BatkAction           *action,
			                           bint                index);
static const bchar *
                    bail_cell_action_get_description 
                                                  (BatkAction           *action,
				                   bint                index);
static bboolean     bail_cell_action_set_description 
                                                  (BatkAction           *action,
				                   bint                index,
				                   const bchar         *desc);
static const bchar *
                    bail_cell_action_get_keybinding 
                                                  (BatkAction           *action,
				                   bint                index);
static bboolean     bail_cell_action_do_action    (BatkAction           *action,
			                           bint                index);
static bboolean     idle_do_action                (bpointer            data);

static void         batk_component_interface_init  (BatkComponentIface   *iface);
static void         bail_cell_get_extents         (BatkComponent        *component,
                                                   bint                *x,
                                                   bint                *y,
                                                   bint                *width,
                                                   bint                *height,
                                                   BatkCoordType        coord_type);
static bboolean     bail_cell_grab_focus         (BatkComponent        *component);

G_DEFINE_TYPE_WITH_CODE (BailCell, bail_cell, BATK_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_ACTION, batk_action_interface_init)
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_COMPONENT, batk_component_interface_init))

static void	 
bail_cell_class_init (BailCellClass *klass)
{
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);
  BObjectClass *g_object_class = B_OBJECT_CLASS (klass);

  g_object_class->finalize = bail_cell_object_finalize;

  class->get_index_in_parent = bail_cell_get_index_in_parent;
  class->ref_state_set = bail_cell_ref_state_set;
}

void 
bail_cell_initialise (BailCell  *cell,
                      BtkWidget *widget,
                      BatkObject *parent,
                      bint      index)
{
  g_return_if_fail (BAIL_IS_CELL (cell));
  g_return_if_fail (BTK_IS_WIDGET (widget));

  cell->widget = widget;
  batk_object_set_parent (BATK_OBJECT (cell), parent);
  cell->index = index;

  g_signal_connect_object (B_OBJECT (widget),
                           "destroy",
                           G_CALLBACK (bail_cell_destroyed ),
                           cell, 0);
}

static void
bail_cell_destroyed (BtkWidget       *widget,
                     BailCell        *cell)
{
  /*
   * This is the signal handler for the "destroy" signal for the 
   * BtkWidget. We set the  pointer location to NULL;
   */
  cell->widget = NULL;
}

static void
bail_cell_init (BailCell *cell)
{
  cell->state_set = batk_state_set_new ();
  cell->widget = NULL;
  cell->action_list = NULL;
  cell->index = 0;
  batk_state_set_add_state (cell->state_set, BATK_STATE_TRANSIENT);
  batk_state_set_add_state (cell->state_set, BATK_STATE_ENABLED);
  batk_state_set_add_state (cell->state_set, BATK_STATE_SENSITIVE);
  batk_state_set_add_state (cell->state_set, BATK_STATE_SELECTABLE);
  cell->refresh_index = NULL;
}

static void
bail_cell_object_finalize (BObject *obj)
{
  BailCell *cell = BAIL_CELL (obj);
  BatkRelationSet *relation_set;
  BatkRelation *relation;
  GPtrArray *target;
  bpointer target_object;
  bint i;

  if (cell->state_set)
    g_object_unref (cell->state_set);
  if (cell->action_list)
    {
      g_list_foreach (cell->action_list, _bail_cell_destroy_action_info, NULL);
      g_list_free (cell->action_list);
    }
  if (cell->action_idle_handler)
    {
      g_source_remove (cell->action_idle_handler);
      cell->action_idle_handler = 0;
    }
  relation_set = batk_object_ref_relation_set (BATK_OBJECT (obj));
  if (BATK_IS_RELATION_SET (relation_set))
    {
      relation = batk_relation_set_get_relation_by_type (relation_set, 
                                                        BATK_RELATION_NODE_CHILD_OF);
      if (relation)
        {
          target = batk_relation_get_target (relation);
          for (i = 0; i < target->len; i++)
            {
              target_object = g_ptr_array_index (target, i);
              if (BAIL_IS_CELL (target_object))
                {
                  g_object_unref (target_object);
                }
            }
        }
      g_object_unref (relation_set);
    }
  B_OBJECT_CLASS (bail_cell_parent_class)->finalize (obj);
}

static BatkStateSet *
bail_cell_ref_state_set (BatkObject *obj)
{
  BailCell *cell = BAIL_CELL (obj);
  g_assert (cell->state_set);

  g_object_ref(cell->state_set);
  return cell->state_set;
}

bboolean
bail_cell_add_state (BailCell     *cell, 
                     BatkStateType state_type,
                     bboolean     emit_signal)
{
  if (!batk_state_set_contains_state (cell->state_set, state_type))
    {
      bboolean rc;
      BatkObject *parent;
    
      rc = batk_state_set_add_state (cell->state_set, state_type);
      /*
       * The signal should only be generated if the value changed,
       * not when the cell is set up.  So states that are set
       * initially should pass FALSE as the emit_signal argument.
       */

      if (emit_signal)
        {
          batk_object_notify_state_change (BATK_OBJECT (cell), state_type, TRUE);
          /* If state_type is BATK_STATE_VISIBLE, additional notification */
          if (state_type == BATK_STATE_VISIBLE)
            g_signal_emit_by_name (cell, "visible_data_changed");
        }

      /* 
       * If the parent is a flyweight container cell, propagate the state 
       * change to it also 
       */

      parent = batk_object_get_parent (BATK_OBJECT (cell));
      if (BAIL_IS_CONTAINER_CELL (parent))
        bail_cell_add_state (BAIL_CELL (parent), state_type, emit_signal);
      return rc;
    }
  else
    return FALSE;
}

bboolean
bail_cell_remove_state (BailCell     *cell, 
                        BatkStateType state_type,
                        bboolean     emit_signal)
{
  if (batk_state_set_contains_state (cell->state_set, state_type))
    {
      bboolean rc;
      BatkObject *parent;

      parent = batk_object_get_parent (BATK_OBJECT (cell));

      rc = batk_state_set_remove_state (cell->state_set, state_type);
      /*
       * The signal should only be generated if the value changed,
       * not when the cell is set up.  So states that are set
       * initially should pass FALSE as the emit_signal argument.
       */

      if (emit_signal)
        {
          batk_object_notify_state_change (BATK_OBJECT (cell), state_type, FALSE);
          /* If state_type is BATK_STATE_VISIBLE, additional notification */
          if (state_type == BATK_STATE_VISIBLE)
            g_signal_emit_by_name (cell, "visible_data_changed");
        }

      /* 
       * If the parent is a flyweight container cell, propagate the state 
       * change to it also 
       */

      if (BAIL_IS_CONTAINER_CELL (parent))
        bail_cell_remove_state (BAIL_CELL (parent), state_type, emit_signal);
      return rc;
    }
  else
    return FALSE;
}

static bint
bail_cell_get_index_in_parent (BatkObject *obj)
{
  BailCell *cell;

  g_assert (BAIL_IS_CELL (obj));

  cell = BAIL_CELL (obj);
  if (batk_state_set_contains_state (cell->state_set, BATK_STATE_STALE))
    if (cell->refresh_index)
      {
        cell->refresh_index (cell);
        batk_state_set_remove_state (cell->state_set, BATK_STATE_STALE);
      }
  return cell->index;
}

static void
batk_action_interface_init (BatkActionIface *iface)
{
  iface->get_n_actions = bail_cell_action_get_n_actions;
  iface->do_action = bail_cell_action_do_action;
  iface->get_name = bail_cell_action_get_name;
  iface->get_description = bail_cell_action_get_description;
  iface->set_description = bail_cell_action_set_description;
  iface->get_keybinding = bail_cell_action_get_keybinding;
}

/*
 * Deprecated: 2.22: The action interface is added for all cells now.
 */
void
bail_cell_type_add_action_interface (GType type)
{
}

bboolean
bail_cell_add_action (BailCell    *cell,
		      const bchar *action_name,
		      const bchar *action_description,
		      const bchar *action_keybinding,
		      ACTION_FUNC action_func)
{
  ActionInfo *info;
  g_return_val_if_fail (BAIL_IS_CELL (cell), FALSE);
  info = g_new (ActionInfo, 1);

  if (action_name != NULL)
    info->name = g_strdup (action_name);
  else
    info->name = NULL;
  if (action_description != NULL)
    info->description = g_strdup (action_description);
  else
    info->description = NULL;
  if (action_keybinding != NULL)
    info->keybinding = g_strdup (action_keybinding);
  else
    info->keybinding = NULL;
  info->do_action_func = action_func;

  cell->action_list = g_list_append (cell->action_list, (bpointer) info);
  return TRUE;
}

bboolean
bail_cell_remove_action (BailCell *cell,
			 bint     action_index)
{
  GList *list_node;

  g_return_val_if_fail (BAIL_IS_CELL (cell), FALSE);
  list_node = g_list_nth (cell->action_list, action_index);
  if (!list_node)
    return FALSE;
  _bail_cell_destroy_action_info (list_node->data, NULL);
  cell->action_list = g_list_remove_link (cell->action_list, list_node);
  return TRUE;
}


bboolean
bail_cell_remove_action_by_name (BailCell    *cell,
				 const bchar *action_name)
{
  GList *list_node;
  bboolean action_found= FALSE;
  
  g_return_val_if_fail (BAIL_IS_CELL (cell), FALSE);
  for (list_node = cell->action_list; list_node && !action_found; 
                    list_node = list_node->next)
    {
      if (!strcmp (((ActionInfo *)(list_node->data))->name, action_name))
	{
	  action_found = TRUE;
	  break;
	}
    }
  if (!action_found)
    return FALSE;
  _bail_cell_destroy_action_info (list_node->data, NULL);
  cell->action_list = g_list_remove_link (cell->action_list, list_node);
  return TRUE;
}

static ActionInfo *
_bail_cell_get_action_info (BailCell *cell,
			    bint     index)
{
  GList *list_node;

  g_return_val_if_fail (BAIL_IS_CELL (cell), NULL);
  if (cell->action_list == NULL)
    return NULL;
  list_node = g_list_nth (cell->action_list, index);
  if (!list_node)
    return NULL;
  return (ActionInfo *) (list_node->data);
}


static void
_bail_cell_destroy_action_info (bpointer action_info, 
                                bpointer user_data)
{
  ActionInfo *info = (ActionInfo *)action_info;
  g_assert (info != NULL);
  g_free (info->name);
  g_free (info->description);
  g_free (info->keybinding);
  g_free (info);
}
static bint
bail_cell_action_get_n_actions (BatkAction *action)
{
  BailCell *cell = BAIL_CELL(action);
  if (cell->action_list != NULL)
    return g_list_length (cell->action_list);
  else
    return 0;
}

static const bchar *
bail_cell_action_get_name (BatkAction *action,
			   bint      index)
{
  BailCell *cell = BAIL_CELL(action);
  ActionInfo *info = _bail_cell_get_action_info (cell, index);

  if (info == NULL)
    return NULL;
  return info->name;
}

static const bchar *
bail_cell_action_get_description (BatkAction *action,
				  bint      index)
{
  BailCell *cell = BAIL_CELL(action);
  ActionInfo *info = _bail_cell_get_action_info (cell, index);

  if (info == NULL)
    return NULL;
  return info->description;
}

static bboolean
bail_cell_action_set_description (BatkAction   *action,
				  bint        index,
				  const bchar *desc)
{
  BailCell *cell = BAIL_CELL(action);
  ActionInfo *info = _bail_cell_get_action_info (cell, index);

  if (info == NULL)
    return FALSE;
  g_free (info->description);
  info->description = g_strdup (desc);
  return TRUE;
}

static const bchar *
bail_cell_action_get_keybinding (BatkAction *action,
				 bint      index)
{
  BailCell *cell = BAIL_CELL(action);
  ActionInfo *info = _bail_cell_get_action_info (cell, index);
  if (info == NULL)
    return NULL;
  return info->keybinding;
}

static bboolean
bail_cell_action_do_action (BatkAction *action,
			    bint      index)
{
  BailCell *cell = BAIL_CELL(action);
  ActionInfo *info = _bail_cell_get_action_info (cell, index);
  if (info == NULL)
    return FALSE;
  if (info->do_action_func == NULL)
    return FALSE;
  if (cell->action_idle_handler)
    return FALSE;
  cell->action_func = info->do_action_func;
  cell->action_idle_handler = bdk_threads_add_idle (idle_do_action, cell);
  return TRUE;
}

static bboolean
idle_do_action (bpointer data)
{
  BailCell *cell;

  cell = BAIL_CELL (data);
  cell->action_idle_handler = 0;
  cell->action_func (cell);

  return FALSE;
}

static void
batk_component_interface_init (BatkComponentIface *iface)
{
  iface->get_extents = bail_cell_get_extents;
  iface->grab_focus = bail_cell_grab_focus;
}

static void 
bail_cell_get_extents (BatkComponent *component,
                       bint         *x,
                       bint         *y,
                       bint         *width,
                       bint         *height,
                       BatkCoordType coord_type)
{
  BailCell *bailcell;
  BatkObject *cell_parent;

  g_assert (BAIL_IS_CELL (component));

  bailcell = BAIL_CELL (component);

  cell_parent = btk_widget_get_accessible (bailcell->widget);

  bail_cell_parent_get_cell_extents (BAIL_CELL_PARENT (cell_parent), 
                                   bailcell, x, y, width, height, coord_type);
}

static bboolean 
bail_cell_grab_focus (BatkComponent *component)
{
  BailCell *bailcell;
  BatkObject *cell_parent;

  g_assert (BAIL_IS_CELL (component));

  bailcell = BAIL_CELL (component);

  cell_parent = btk_widget_get_accessible (bailcell->widget);

  return bail_cell_parent_grab_focus (BAIL_CELL_PARENT (cell_parent), 
                                      bailcell);
}
