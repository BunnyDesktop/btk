/*
 * BTK - The GIMP Toolkit
 * Copyright (C) 1998, 1999 Red Hat, Inc.
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Bunny Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Author: James Henstridge <james@daa.com.au>
 *
 * Modified by the BTK+ Team and others 2003.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"

#include "btkradioaction.h"
#include "btkradiomenuitem.h"
#include "btktoggleactionprivate.h"
#include "btktoggletoolbutton.h"
#include "btkintl.h"
#include "btkprivate.h"
#include "btkalias.h"

#define BTK_RADIO_ACTION_GET_PRIVATE(obj) (B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_RADIO_ACTION, BtkRadioActionPrivate))

struct _BtkRadioActionPrivate 
{
  GSList *group;
  bint    value;
};

enum 
{
  CHANGED,
  LAST_SIGNAL
};

enum 
{
  PROP_0,
  PROP_VALUE,
  PROP_GROUP,
  PROP_CURRENT_VALUE
};

static void btk_radio_action_finalize     (BObject *object);
static void btk_radio_action_set_property (BObject         *object,
				           buint            prop_id,
				           const BValue    *value,
				           BParamSpec      *pspec);
static void btk_radio_action_get_property (BObject         *object,
				           buint            prop_id,
				           BValue          *value,
				           BParamSpec      *pspec);
static void btk_radio_action_activate     (BtkAction *action);
static BtkWidget *create_menu_item        (BtkAction *action);


G_DEFINE_TYPE (BtkRadioAction, btk_radio_action, BTK_TYPE_TOGGLE_ACTION)

static buint         radio_action_signals[LAST_SIGNAL] = { 0 };

static void
btk_radio_action_class_init (BtkRadioActionClass *klass)
{
  BObjectClass *bobject_class;
  BtkActionClass *action_class;

  bobject_class = B_OBJECT_CLASS (klass);
  action_class = BTK_ACTION_CLASS (klass);

  bobject_class->finalize = btk_radio_action_finalize;
  bobject_class->set_property = btk_radio_action_set_property;
  bobject_class->get_property = btk_radio_action_get_property;

  action_class->activate = btk_radio_action_activate;

  action_class->create_menu_item = create_menu_item;

  /**
   * BtkRadioAction:value:
   *
   * The value is an arbitrary integer which can be used as a
   * convenient way to determine which action in the group is 
   * currently active in an ::activate or ::changed signal handler.
   * See btk_radio_action_get_current_value() and #BtkRadioActionEntry
   * for convenient ways to get and set this property.
   *
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
				   PROP_VALUE,
				   g_param_spec_int ("value",
						     P_("The value"),
						     P_("The value returned by btk_radio_action_get_current_value() when this action is the current action of its group."),
						     B_MININT,
						     B_MAXINT,
						     0,
						     BTK_PARAM_READWRITE));

  /**
   * BtkRadioAction:group:
   *
   * Sets a new group for a radio action.
   *
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
				   PROP_GROUP,
				   g_param_spec_object ("group",
							P_("Group"),
							P_("The radio action whose group this action belongs to."),
							BTK_TYPE_RADIO_ACTION,
							BTK_PARAM_WRITABLE));

  /**
   * BtkRadioAction:current-value:
   *
   * The value property of the currently active member of the group to which
   * this action belongs. 
   *
   * Since: 2.10
   */
  g_object_class_install_property (bobject_class,
				   PROP_CURRENT_VALUE,
                                   g_param_spec_int ("current-value",
						     P_("The current value"),
						     P_("The value property of the currently active member of the group to which this action belongs."),
						     B_MININT,
						     B_MAXINT,
						     0,
						     BTK_PARAM_READWRITE));

  /**
   * BtkRadioAction::changed:
   * @action: the action on which the signal is emitted
   * @current: the member of @action<!-- -->s group which has just been activated
   *
   * The ::changed signal is emitted on every member of a radio group when the
   * active member is changed. The signal gets emitted after the ::activate signals
   * for the previous and current active members.
   *
   * Since: 2.4
   */
  radio_action_signals[CHANGED] =
    g_signal_new (I_("changed"),
		  B_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
		  G_STRUCT_OFFSET (BtkRadioActionClass, changed),  NULL, NULL,
		  g_cclosure_marshal_VOID__OBJECT,
		  B_TYPE_NONE, 1, BTK_TYPE_RADIO_ACTION);

  g_type_class_add_private (bobject_class, sizeof (BtkRadioActionPrivate));
}

static void
btk_radio_action_init (BtkRadioAction *action)
{
  action->private_data = BTK_RADIO_ACTION_GET_PRIVATE (action);
  action->private_data->group = b_slist_prepend (NULL, action);
  action->private_data->value = 0;

  btk_toggle_action_set_draw_as_radio (BTK_TOGGLE_ACTION (action), TRUE);
}

/**
 * btk_radio_action_new:
 * @name: A unique name for the action
 * @label: (allow-none): The label displayed in menu items and on buttons, or %NULL
 * @tooltip: (allow-none): A tooltip for this action, or %NULL
 * @stock_id: The stock icon to display in widgets representing this
 *   action, or %NULL
 * @value: The value which btk_radio_action_get_current_value() should
 *   return if this action is selected.
 *
 * Creates a new #BtkRadioAction object. To add the action to
 * a #BtkActionGroup and set the accelerator for the action,
 * call btk_action_group_add_action_with_accel().
 *
 * Return value: a new #BtkRadioAction
 *
 * Since: 2.4
 */
BtkRadioAction *
btk_radio_action_new (const bchar *name,
		      const bchar *label,
		      const bchar *tooltip,
		      const bchar *stock_id,
		      bint value)
{
  g_return_val_if_fail (name != NULL, NULL);

  return g_object_new (BTK_TYPE_RADIO_ACTION,
                       "name", name,
                       "label", label,
                       "tooltip", tooltip,
                       "stock-id", stock_id,
                       "value", value,
                       NULL);
}

static void
btk_radio_action_finalize (BObject *object)
{
  BtkRadioAction *action;
  GSList *tmp_list;

  action = BTK_RADIO_ACTION (object);

  action->private_data->group = b_slist_remove (action->private_data->group, action);

  tmp_list = action->private_data->group;

  while (tmp_list)
    {
      BtkRadioAction *tmp_action = tmp_list->data;

      tmp_list = tmp_list->next;
      tmp_action->private_data->group = action->private_data->group;
    }

  B_OBJECT_CLASS (btk_radio_action_parent_class)->finalize (object);
}

static void
btk_radio_action_set_property (BObject         *object,
			       buint            prop_id,
			       const BValue    *value,
			       BParamSpec      *pspec)
{
  BtkRadioAction *radio_action;
  
  radio_action = BTK_RADIO_ACTION (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      radio_action->private_data->value = b_value_get_int (value);
      break;
    case PROP_GROUP: 
      {
	BtkRadioAction *arg;
	GSList *slist = NULL;
	
	if (G_VALUE_HOLDS_OBJECT (value)) 
	  {
	    arg = BTK_RADIO_ACTION (b_value_get_object (value));
	    if (arg)
	      slist = btk_radio_action_get_group (arg);
	    btk_radio_action_set_group (radio_action, slist);
	  }
      }
      break;
    case PROP_CURRENT_VALUE:
      btk_radio_action_set_current_value (radio_action,
                                          b_value_get_int (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_radio_action_get_property (BObject    *object,
			       buint       prop_id,
			       BValue     *value,
			       BParamSpec *pspec)
{
  BtkRadioAction *radio_action;

  radio_action = BTK_RADIO_ACTION (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      b_value_set_int (value, radio_action->private_data->value);
      break;
    case PROP_CURRENT_VALUE:
      b_value_set_int (value,
                       btk_radio_action_get_current_value (radio_action));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_radio_action_activate (BtkAction *action)
{
  BtkRadioAction *radio_action;
  BtkToggleAction *toggle_action;
  BtkToggleAction *tmp_action;
  GSList *tmp_list;

  radio_action = BTK_RADIO_ACTION (action);
  toggle_action = BTK_TOGGLE_ACTION (action);

  if (toggle_action->private_data->active)
    {
      tmp_list = radio_action->private_data->group;

      while (tmp_list)
	{
	  tmp_action = tmp_list->data;
	  tmp_list = tmp_list->next;

	  if (tmp_action->private_data->active && (tmp_action != toggle_action)) 
	    {
	      toggle_action->private_data->active = !toggle_action->private_data->active;

	      break;
	    }
	}
      g_object_notify (B_OBJECT (action), "active");
    }
  else
    {
      toggle_action->private_data->active = !toggle_action->private_data->active;
      g_object_notify (B_OBJECT (action), "active");

      tmp_list = radio_action->private_data->group;
      while (tmp_list)
	{
	  tmp_action = tmp_list->data;
	  tmp_list = tmp_list->next;

	  if (tmp_action->private_data->active && (tmp_action != toggle_action))
	    {
	      _btk_action_emit_activate (BTK_ACTION (tmp_action));
	      break;
	    }
	}

      tmp_list = radio_action->private_data->group;
      while (tmp_list)
	{
	  tmp_action = tmp_list->data;
	  tmp_list = tmp_list->next;
	  
          g_object_notify (B_OBJECT (tmp_action), "current-value");

	  g_signal_emit (tmp_action, radio_action_signals[CHANGED], 0, radio_action);
	}
    }

  btk_toggle_action_toggled (toggle_action);
}

static BtkWidget *
create_menu_item (BtkAction *action)
{
  return g_object_new (BTK_TYPE_CHECK_MENU_ITEM, 
		       "draw-as-radio", TRUE,
		       NULL);
}

/**
 * btk_radio_action_get_group:
 * @action: the action object
 *
 * Returns the list representing the radio group for this object.
 * Note that the returned list is only valid until the next change
 * to the group. 
 *
 * A common way to set up a group of radio group is the following:
 * |[
 *   GSList *group = NULL;
 *   BtkRadioAction *action;
 *  
 *   while (/&ast; more actions to add &ast;/)
 *     {
 *        action = btk_radio_action_new (...);
 *        
 *        btk_radio_action_set_group (action, group);
 *        group = btk_radio_action_get_group (action);
 *     }
 * ]|
 *
 * Returns:  (element-type BtkAction) (transfer none): the list representing the radio group for this object
 *
 * Since: 2.4
 */
GSList *
btk_radio_action_get_group (BtkRadioAction *action)
{
  g_return_val_if_fail (BTK_IS_RADIO_ACTION (action), NULL);

  return action->private_data->group;
}

/**
 * btk_radio_action_set_group:
 * @action: the action object
 * @group: a list representing a radio group
 *
 * Sets the radio group for the radio action object.
 *
 * Since: 2.4
 */
void
btk_radio_action_set_group (BtkRadioAction *action, 
			    GSList         *group)
{
  g_return_if_fail (BTK_IS_RADIO_ACTION (action));
  g_return_if_fail (!b_slist_find (group, action));

  if (action->private_data->group)
    {
      GSList *slist;

      action->private_data->group = b_slist_remove (action->private_data->group, action);

      for (slist = action->private_data->group; slist; slist = slist->next)
	{
	  BtkRadioAction *tmp_action = slist->data;

	  tmp_action->private_data->group = action->private_data->group;
	}
    }

  action->private_data->group = b_slist_prepend (group, action);

  if (group)
    {
      GSList *slist;

      for (slist = action->private_data->group; slist; slist = slist->next)
	{
	  BtkRadioAction *tmp_action = slist->data;

	  tmp_action->private_data->group = action->private_data->group;
	}
    }
  else
    {
      btk_toggle_action_set_active (BTK_TOGGLE_ACTION (action), TRUE);
    }
}

/**
 * btk_radio_action_get_current_value:
 * @action: a #BtkRadioAction
 * 
 * Obtains the value property of the currently active member of 
 * the group to which @action belongs.
 * 
 * Return value: The value of the currently active group member
 *
 * Since: 2.4
 **/
bint
btk_radio_action_get_current_value (BtkRadioAction *action)
{
  GSList *slist;

  g_return_val_if_fail (BTK_IS_RADIO_ACTION (action), 0);

  if (action->private_data->group)
    {
      for (slist = action->private_data->group; slist; slist = slist->next)
	{
	  BtkToggleAction *toggle_action = slist->data;

	  if (toggle_action->private_data->active)
	    return BTK_RADIO_ACTION (toggle_action)->private_data->value;
	}
    }

  return action->private_data->value;
}

/**
 * btk_radio_action_set_current_value:
 * @action: a #BtkRadioAction
 * @current_value: the new value
 * 
 * Sets the currently active group member to the member with value
 * property @current_value.
 *
 * Since: 2.10
 **/
void
btk_radio_action_set_current_value (BtkRadioAction *action,
                                    bint            current_value)
{
  GSList *slist;

  g_return_if_fail (BTK_IS_RADIO_ACTION (action));

  if (action->private_data->group)
    {
      for (slist = action->private_data->group; slist; slist = slist->next)
	{
	  BtkRadioAction *radio_action = slist->data;

	  if (radio_action->private_data->value == current_value)
            {
              btk_toggle_action_set_active (BTK_TOGGLE_ACTION (radio_action),
                                            TRUE);
              return;
            }
	}
    }

  if (action->private_data->value == current_value)
    btk_toggle_action_set_active (BTK_TOGGLE_ACTION (action), TRUE);
  else
    g_warning ("Radio group does not contain an action with value '%d'",
	       current_value);
}

#define __BTK_RADIO_ACTION_C__
#include "btkaliasdef.c"
