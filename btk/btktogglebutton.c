/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"
#include "btklabel.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btktogglebutton.h"
#include "btktoggleaction.h"
#include "btkactivatable.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

#define DEFAULT_LEFT_POS  4
#define DEFAULT_TOP_POS   4
#define DEFAULT_SPACING   7

enum {
  TOGGLED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_ACTIVE,
  PROP_INCONSISTENT,
  PROP_DRAW_INDICATOR
};


static bint btk_toggle_button_expose        (BtkWidget            *widget,
					     BdkEventExpose       *event);
static bboolean btk_toggle_button_mnemonic_activate  (BtkWidget            *widget,
                                                      bboolean              group_cycling);
static void btk_toggle_button_pressed       (BtkButton            *button);
static void btk_toggle_button_released      (BtkButton            *button);
static void btk_toggle_button_clicked       (BtkButton            *button);
static void btk_toggle_button_set_property  (BObject              *object,
					     buint                 prop_id,
					     const BValue         *value,
					     BParamSpec           *pspec);
static void btk_toggle_button_get_property  (BObject              *object,
					     buint                 prop_id,
					     BValue               *value,
					     BParamSpec           *pspec);
static void btk_toggle_button_update_state  (BtkButton            *button);


static void btk_toggle_button_activatable_interface_init (BtkActivatableIface  *iface);
static void btk_toggle_button_update         	     (BtkActivatable       *activatable,
					 	      BtkAction            *action,
						      const bchar          *property_name);
static void btk_toggle_button_sync_action_properties (BtkActivatable       *activatable,
						      BtkAction            *action);

static BtkActivatableIface *parent_activatable_iface;
static buint                toggle_button_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (BtkToggleButton, btk_toggle_button, BTK_TYPE_BUTTON,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_ACTIVATABLE,
						btk_toggle_button_activatable_interface_init))

static void
btk_toggle_button_class_init (BtkToggleButtonClass *class)
{
  BObjectClass *bobject_class;
  BtkWidgetClass *widget_class;
  BtkButtonClass *button_class;

  bobject_class = B_OBJECT_CLASS (class);
  widget_class = (BtkWidgetClass*) class;
  button_class = (BtkButtonClass*) class;

  bobject_class->set_property = btk_toggle_button_set_property;
  bobject_class->get_property = btk_toggle_button_get_property;

  widget_class->expose_event = btk_toggle_button_expose;
  widget_class->mnemonic_activate = btk_toggle_button_mnemonic_activate;

  button_class->pressed = btk_toggle_button_pressed;
  button_class->released = btk_toggle_button_released;
  button_class->clicked = btk_toggle_button_clicked;
  button_class->enter = btk_toggle_button_update_state;
  button_class->leave = btk_toggle_button_update_state;

  class->toggled = NULL;

  g_object_class_install_property (bobject_class,
                                   PROP_ACTIVE,
                                   g_param_spec_boolean ("active",
							 P_("Active"),
							 P_("If the toggle button should be pressed in or not"),
							 FALSE,
							 BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_INCONSISTENT,
                                   g_param_spec_boolean ("inconsistent",
							 P_("Inconsistent"),
							 P_("If the toggle button is in an \"in between\" state"),
							 FALSE,
							 BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_DRAW_INDICATOR,
                                   g_param_spec_boolean ("draw-indicator",
							 P_("Draw Indicator"),
							 P_("If the toggle part of the button is displayed"),
							 FALSE,
							 BTK_PARAM_READWRITE));

  toggle_button_signals[TOGGLED] =
    g_signal_new (I_("toggled"),
		  B_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkToggleButtonClass, toggled),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
}

static void
btk_toggle_button_init (BtkToggleButton *toggle_button)
{
  toggle_button->active = FALSE;
  toggle_button->draw_indicator = FALSE;
  BTK_BUTTON (toggle_button)->depress_on_activate = TRUE;
}

static void
btk_toggle_button_activatable_interface_init (BtkActivatableIface *iface)
{
  parent_activatable_iface = g_type_interface_peek_parent (iface);
  iface->update = btk_toggle_button_update;
  iface->sync_action_properties = btk_toggle_button_sync_action_properties;
}

static void
btk_toggle_button_update (BtkActivatable *activatable,
			  BtkAction      *action,
			  const bchar    *property_name)
{
  BtkToggleButton *button;

  parent_activatable_iface->update (activatable, action, property_name);

  button = BTK_TOGGLE_BUTTON (activatable);

  if (strcmp (property_name, "active") == 0)
    {
      btk_action_block_activate (action);
      btk_toggle_button_set_active (button, btk_toggle_action_get_active (BTK_TOGGLE_ACTION (action)));
      btk_action_unblock_activate (action);
    }

}

static void
btk_toggle_button_sync_action_properties (BtkActivatable *activatable,
				          BtkAction      *action)
{
  BtkToggleButton *button;

  parent_activatable_iface->sync_action_properties (activatable, action);

  if (!BTK_IS_TOGGLE_ACTION (action))
    return;

  button = BTK_TOGGLE_BUTTON (activatable);

  btk_action_block_activate (action);
  btk_toggle_button_set_active (button, btk_toggle_action_get_active (BTK_TOGGLE_ACTION (action)));
  btk_action_unblock_activate (action);
}


BtkWidget*
btk_toggle_button_new (void)
{
  return g_object_new (BTK_TYPE_TOGGLE_BUTTON, NULL);
}

BtkWidget*
btk_toggle_button_new_with_label (const bchar *label)
{
  return g_object_new (BTK_TYPE_TOGGLE_BUTTON, "label", label, NULL);
}

/**
 * btk_toggle_button_new_with_mnemonic:
 * @label: the text of the button, with an underscore in front of the
 *         mnemonic character
 * @returns: a new #BtkToggleButton
 *
 * Creates a new #BtkToggleButton containing a label. The label
 * will be created using btk_label_new_with_mnemonic(), so underscores
 * in @label indicate the mnemonic for the button.
 **/
BtkWidget*
btk_toggle_button_new_with_mnemonic (const bchar *label)
{
  return g_object_new (BTK_TYPE_TOGGLE_BUTTON, 
		       "label", label, 
		       "use-underline", TRUE, 
		       NULL);
}

static void
btk_toggle_button_set_property (BObject      *object,
				buint         prop_id,
				const BValue *value,
				BParamSpec   *pspec)
{
  BtkToggleButton *tb;

  tb = BTK_TOGGLE_BUTTON (object);

  switch (prop_id)
    {
    case PROP_ACTIVE:
      btk_toggle_button_set_active (tb, b_value_get_boolean (value));
      break;
    case PROP_INCONSISTENT:
      btk_toggle_button_set_inconsistent (tb, b_value_get_boolean (value));
      break;
    case PROP_DRAW_INDICATOR:
      btk_toggle_button_set_mode (tb, b_value_get_boolean (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_toggle_button_get_property (BObject      *object,
				buint         prop_id,
				BValue       *value,
				BParamSpec   *pspec)
{
  BtkToggleButton *tb;

  tb = BTK_TOGGLE_BUTTON (object);

  switch (prop_id)
    {
    case PROP_ACTIVE:
      b_value_set_boolean (value, tb->active);
      break;
    case PROP_INCONSISTENT:
      b_value_set_boolean (value, tb->inconsistent);
      break;
    case PROP_DRAW_INDICATOR:
      b_value_set_boolean (value, tb->draw_indicator);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * btk_toggle_button_set_mode:
 * @toggle_button: a #BtkToggleButton
 * @draw_indicator: if %TRUE, draw the button as a separate indicator
 * and label; if %FALSE, draw the button like a normal button
 *
 * Sets whether the button is displayed as a separate indicator and label.
 * You can call this function on a checkbutton or a radiobutton with
 * @draw_indicator = %FALSE to make the button look like a normal button
 *
 * This function only affects instances of classes like #BtkCheckButton
 * and #BtkRadioButton that derive from #BtkToggleButton,
 * not instances of #BtkToggleButton itself.
 */
void
btk_toggle_button_set_mode (BtkToggleButton *toggle_button,
			    bboolean         draw_indicator)
{
  g_return_if_fail (BTK_IS_TOGGLE_BUTTON (toggle_button));

  draw_indicator = draw_indicator ? TRUE : FALSE;

  if (toggle_button->draw_indicator != draw_indicator)
    {
      toggle_button->draw_indicator = draw_indicator;
      BTK_BUTTON (toggle_button)->depress_on_activate = !draw_indicator;
      
      if (btk_widget_get_visible (BTK_WIDGET (toggle_button)))
	btk_widget_queue_resize (BTK_WIDGET (toggle_button));

      g_object_notify (B_OBJECT (toggle_button), "draw-indicator");
    }
}

/**
 * btk_toggle_button_get_mode:
 * @toggle_button: a #BtkToggleButton
 *
 * Retrieves whether the button is displayed as a separate indicator
 * and label. See btk_toggle_button_set_mode().
 *
 * Return value: %TRUE if the togglebutton is drawn as a separate indicator
 *   and label.
 **/
bboolean
btk_toggle_button_get_mode (BtkToggleButton *toggle_button)
{
  g_return_val_if_fail (BTK_IS_TOGGLE_BUTTON (toggle_button), FALSE);

  return toggle_button->draw_indicator;
}

void
btk_toggle_button_set_active (BtkToggleButton *toggle_button,
			      bboolean         is_active)
{
  g_return_if_fail (BTK_IS_TOGGLE_BUTTON (toggle_button));

  is_active = is_active != FALSE;

  if (toggle_button->active != is_active)
    btk_button_clicked (BTK_BUTTON (toggle_button));
}


bboolean
btk_toggle_button_get_active (BtkToggleButton *toggle_button)
{
  g_return_val_if_fail (BTK_IS_TOGGLE_BUTTON (toggle_button), FALSE);

  return (toggle_button->active) ? TRUE : FALSE;
}


void
btk_toggle_button_toggled (BtkToggleButton *toggle_button)
{
  g_return_if_fail (BTK_IS_TOGGLE_BUTTON (toggle_button));

  g_signal_emit (toggle_button, toggle_button_signals[TOGGLED], 0);
}

/**
 * btk_toggle_button_set_inconsistent:
 * @toggle_button: a #BtkToggleButton
 * @setting: %TRUE if state is inconsistent
 *
 * If the user has selected a range of elements (such as some text or
 * spreadsheet cells) that are affected by a toggle button, and the
 * current values in that range are inconsistent, you may want to
 * display the toggle in an "in between" state. This function turns on
 * "in between" display.  Normally you would turn off the inconsistent
 * state again if the user toggles the toggle button. This has to be
 * done manually, btk_toggle_button_set_inconsistent() only affects
 * visual appearance, it doesn't affect the semantics of the button.
 * 
 **/
void
btk_toggle_button_set_inconsistent (BtkToggleButton *toggle_button,
                                    bboolean         setting)
{
  g_return_if_fail (BTK_IS_TOGGLE_BUTTON (toggle_button));
  
  setting = setting != FALSE;

  if (setting != toggle_button->inconsistent)
    {
      toggle_button->inconsistent = setting;
      
      btk_toggle_button_update_state (BTK_BUTTON (toggle_button));
      btk_widget_queue_draw (BTK_WIDGET (toggle_button));

      g_object_notify (B_OBJECT (toggle_button), "inconsistent");      
    }
}

/**
 * btk_toggle_button_get_inconsistent:
 * @toggle_button: a #BtkToggleButton
 * 
 * Gets the value set by btk_toggle_button_set_inconsistent().
 * 
 * Return value: %TRUE if the button is displayed as inconsistent, %FALSE otherwise
 **/
bboolean
btk_toggle_button_get_inconsistent (BtkToggleButton *toggle_button)
{
  g_return_val_if_fail (BTK_IS_TOGGLE_BUTTON (toggle_button), FALSE);

  return toggle_button->inconsistent;
}

static bint
btk_toggle_button_expose (BtkWidget      *widget,
			  BdkEventExpose *event)
{
  if (btk_widget_is_drawable (widget))
    {
      BtkWidget *child = BTK_BIN (widget)->child;
      BtkButton *button = BTK_BUTTON (widget);
      BtkStateType state_type;
      BtkShadowType shadow_type;

      state_type = btk_widget_get_state (widget);
      
      if (BTK_TOGGLE_BUTTON (widget)->inconsistent)
        {
          if (state_type == BTK_STATE_ACTIVE)
            state_type = BTK_STATE_NORMAL;
          shadow_type = BTK_SHADOW_ETCHED_IN;
        }
      else
	shadow_type = button->depressed ? BTK_SHADOW_IN : BTK_SHADOW_OUT;

      _btk_button_paint (button, &event->area, state_type, shadow_type,
			 "togglebutton", "togglebuttondefault");

      if (child)
	btk_container_propagate_expose (BTK_CONTAINER (widget), child, event);
    }
  
  return FALSE;
}

static bboolean
btk_toggle_button_mnemonic_activate (BtkWidget *widget,
                                     bboolean   group_cycling)
{
  /*
   * We override the standard implementation in 
   * btk_widget_real_mnemonic_activate() in order to focus the widget even
   * if there is no mnemonic conflict.
   */
  if (btk_widget_get_can_focus (widget))
    btk_widget_grab_focus (widget);

  if (!group_cycling)
    btk_widget_activate (widget);

  return TRUE;
}

static void
btk_toggle_button_pressed (BtkButton *button)
{
  button->button_down = TRUE;

  btk_toggle_button_update_state (button);
  btk_widget_queue_draw (BTK_WIDGET (button));
}

static void
btk_toggle_button_released (BtkButton *button)
{
  if (button->button_down)
    {
      button->button_down = FALSE;

      if (button->in_button)
	btk_button_clicked (button);

      btk_toggle_button_update_state (button);
      btk_widget_queue_draw (BTK_WIDGET (button));
    }
}

static void
btk_toggle_button_clicked (BtkButton *button)
{
  BtkToggleButton *toggle_button = BTK_TOGGLE_BUTTON (button);
  toggle_button->active = !toggle_button->active;

  btk_toggle_button_toggled (toggle_button);

  btk_toggle_button_update_state (button);

  g_object_notify (B_OBJECT (toggle_button), "active");

  if (BTK_BUTTON_CLASS (btk_toggle_button_parent_class)->clicked)
    BTK_BUTTON_CLASS (btk_toggle_button_parent_class)->clicked (button);
}

static void
btk_toggle_button_update_state (BtkButton *button)
{
  BtkToggleButton *toggle_button = BTK_TOGGLE_BUTTON (button);
  bboolean depressed, touchscreen;
  BtkStateType new_state;

  g_object_get (btk_widget_get_settings (BTK_WIDGET (button)),
                "btk-touchscreen-mode", &touchscreen,
                NULL);

  if (toggle_button->inconsistent)
    depressed = FALSE;
  else if (button->in_button && button->button_down)
    depressed = TRUE;
  else
    depressed = toggle_button->active;
      
  if (!touchscreen && button->in_button && (!button->button_down || toggle_button->draw_indicator))
    new_state = BTK_STATE_PRELIGHT;
  else
    new_state = depressed ? BTK_STATE_ACTIVE : BTK_STATE_NORMAL;

  _btk_button_set_depressed (button, depressed); 
  btk_widget_set_state (BTK_WIDGET (toggle_button), new_state);
}

#define __BTK_TOGGLE_BUTTON_C__
#include "btkaliasdef.c"
