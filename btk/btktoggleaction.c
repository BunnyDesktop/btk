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

#include "btkintl.h"
#include "btktoggleaction.h"
#include "btktoggleactionprivate.h"
#include "btktoggletoolbutton.h"
#include "btktogglebutton.h"
#include "btkcheckmenuitem.h"
#include "btkprivate.h"
#include "btkalias.h"

enum 
{
  TOGGLED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_DRAW_AS_RADIO,
  PROP_ACTIVE
};

G_DEFINE_TYPE (BtkToggleAction, btk_toggle_action, BTK_TYPE_ACTION)

static void btk_toggle_action_activate     (BtkAction       *action);
static void set_property                   (BObject         *object,
					    guint            prop_id,
					    const BValue    *value,
					    BParamSpec      *pspec);
static void get_property                   (BObject         *object,
					    guint            prop_id,
					    BValue          *value,
					    BParamSpec      *pspec);
static BtkWidget *create_menu_item         (BtkAction       *action);


static BObjectClass *parent_class = NULL;
static guint         action_signals[LAST_SIGNAL] = { 0 };

static void
btk_toggle_action_class_init (BtkToggleActionClass *klass)
{
  BObjectClass *bobject_class;
  BtkActionClass *action_class;

  parent_class = g_type_class_peek_parent (klass);
  bobject_class = B_OBJECT_CLASS (klass);
  action_class = BTK_ACTION_CLASS (klass);

  bobject_class->set_property = set_property;
  bobject_class->get_property = get_property;

  action_class->activate = btk_toggle_action_activate;

  action_class->menu_item_type = BTK_TYPE_CHECK_MENU_ITEM;
  action_class->toolbar_item_type = BTK_TYPE_TOGGLE_TOOL_BUTTON;

  action_class->create_menu_item = create_menu_item;

  klass->toggled = NULL;

  /**
   * BtkToggleAction:draw-as-radio:
   *
   * Whether the proxies for this action look like radio action proxies.
   *
   * This is an appearance property and thus only applies if 
   * #BtkActivatable:use-action-appearance is %TRUE.
   */
  g_object_class_install_property (bobject_class,
                                   PROP_DRAW_AS_RADIO,
                                   g_param_spec_boolean ("draw-as-radio",
                                                         P_("Create the same proxies as a radio action"),
                                                         P_("Whether the proxies for this action look like radio action proxies"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));

  /**
   * BtkToggleAction:active:
   *
   * If the toggle action should be active in or not.
   *
   * Since: 2.10
   */
  g_object_class_install_property (bobject_class,
                                   PROP_ACTIVE,
                                   g_param_spec_boolean ("active",
                                                         P_("Active"),
                                                         P_("If the toggle action should be active in or not"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));

  action_signals[TOGGLED] =
    g_signal_new (I_("toggled"),
                  B_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BtkToggleActionClass, toggled),
		  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  B_TYPE_NONE, 0);

  g_type_class_add_private (bobject_class, sizeof (BtkToggleActionPrivate));
}

static void
btk_toggle_action_init (BtkToggleAction *action)
{
  action->private_data = BTK_TOGGLE_ACTION_GET_PRIVATE (action);
  action->private_data->active = FALSE;
  action->private_data->draw_as_radio = FALSE;
}

/**
 * btk_toggle_action_new:
 * @name: A unique name for the action
 * @label: (allow-none): The label displayed in menu items and on buttons, or %NULL
 * @tooltip: (allow-none): A tooltip for the action, or %NULL
 * @stock_id: The stock icon to display in widgets representing the
 *   action, or %NULL
 *
 * Creates a new #BtkToggleAction object. To add the action to
 * a #BtkActionGroup and set the accelerator for the action,
 * call btk_action_group_add_action_with_accel().
 *
 * Return value: a new #BtkToggleAction
 *
 * Since: 2.4
 */
BtkToggleAction *
btk_toggle_action_new (const gchar *name,
		       const gchar *label,
		       const gchar *tooltip,
		       const gchar *stock_id)
{
  g_return_val_if_fail (name != NULL, NULL);

  return g_object_new (BTK_TYPE_TOGGLE_ACTION,
		       "name", name,
		       "label", label,
		       "tooltip", tooltip,
		       "stock-id", stock_id,
		       NULL);
}

static void
get_property (BObject     *object,
	      guint        prop_id,
	      BValue      *value,
	      BParamSpec  *pspec)
{
  BtkToggleAction *action = BTK_TOGGLE_ACTION (object);
  
  switch (prop_id)
    {
    case PROP_DRAW_AS_RADIO:
      b_value_set_boolean (value, btk_toggle_action_get_draw_as_radio (action));
      break;
    case PROP_ACTIVE:
      b_value_set_boolean (value, btk_toggle_action_get_active (action));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
set_property (BObject      *object,
	      guint         prop_id,
	      const BValue *value,
	      BParamSpec   *pspec)
{
  BtkToggleAction *action = BTK_TOGGLE_ACTION (object);
  
  switch (prop_id)
    {
    case PROP_DRAW_AS_RADIO:
      btk_toggle_action_set_draw_as_radio (action, b_value_get_boolean (value));
      break;
    case PROP_ACTIVE:
      btk_toggle_action_set_active (action, b_value_get_boolean (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_toggle_action_activate (BtkAction *action)
{
  BtkToggleAction *toggle_action;

  g_return_if_fail (BTK_IS_TOGGLE_ACTION (action));

  toggle_action = BTK_TOGGLE_ACTION (action);

  toggle_action->private_data->active = !toggle_action->private_data->active;

  g_object_notify (B_OBJECT (action), "active");

  btk_toggle_action_toggled (toggle_action);
}

/**
 * btk_toggle_action_toggled:
 * @action: the action object
 *
 * Emits the "toggled" signal on the toggle action.
 *
 * Since: 2.4
 */
void
btk_toggle_action_toggled (BtkToggleAction *action)
{
  g_return_if_fail (BTK_IS_TOGGLE_ACTION (action));

  g_signal_emit (action, action_signals[TOGGLED], 0);
}

/**
 * btk_toggle_action_set_active:
 * @action: the action object
 * @is_active: whether the action should be checked or not
 *
 * Sets the checked state on the toggle action.
 *
 * Since: 2.4
 */
void
btk_toggle_action_set_active (BtkToggleAction *action, 
			      gboolean         is_active)
{
  g_return_if_fail (BTK_IS_TOGGLE_ACTION (action));

  is_active = is_active != FALSE;

  if (action->private_data->active != is_active)
    _btk_action_emit_activate (BTK_ACTION (action));
}

/**
 * btk_toggle_action_get_active:
 * @action: the action object
 *
 * Returns the checked state of the toggle action.

 * Returns: the checked state of the toggle action
 *
 * Since: 2.4
 */
gboolean
btk_toggle_action_get_active (BtkToggleAction *action)
{
  g_return_val_if_fail (BTK_IS_TOGGLE_ACTION (action), FALSE);

  return action->private_data->active;
}


/**
 * btk_toggle_action_set_draw_as_radio:
 * @action: the action object
 * @draw_as_radio: whether the action should have proxies like a radio 
 *    action
 *
 * Sets whether the action should have proxies like a radio action.
 *
 * Since: 2.4
 */
void
btk_toggle_action_set_draw_as_radio (BtkToggleAction *action, 
				     gboolean         draw_as_radio)
{
  g_return_if_fail (BTK_IS_TOGGLE_ACTION (action));

  draw_as_radio = draw_as_radio != FALSE;

  if (action->private_data->draw_as_radio != draw_as_radio)
    {
      action->private_data->draw_as_radio = draw_as_radio;
      
      g_object_notify (B_OBJECT (action), "draw-as-radio");      
    }
}

/**
 * btk_toggle_action_get_draw_as_radio:
 * @action: the action object
 *
 * Returns whether the action should have proxies like a radio action.
 *
 * Returns: whether the action should have proxies like a radio action.
 *
 * Since: 2.4
 */
gboolean
btk_toggle_action_get_draw_as_radio (BtkToggleAction *action)
{
  g_return_val_if_fail (BTK_IS_TOGGLE_ACTION (action), FALSE);

  return action->private_data->draw_as_radio;
}

static BtkWidget *
create_menu_item (BtkAction *action)
{
  BtkToggleAction *toggle_action = BTK_TOGGLE_ACTION (action);

  return g_object_new (BTK_TYPE_CHECK_MENU_ITEM, 
		       "draw-as-radio", toggle_action->private_data->draw_as_radio,
		       NULL);
}

#define __BTK_TOGGLE_ACTION_C__
#include "btkaliasdef.c"
