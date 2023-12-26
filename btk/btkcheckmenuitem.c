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
 * Modified by the BTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"
#include "btkcheckmenuitem.h"
#include "btkaccellabel.h"
#include "btkactivatable.h"
#include "btktoggleaction.h"
#include "btkmarshalers.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

enum {
  TOGGLED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_ACTIVE,
  PROP_INCONSISTENT,
  PROP_DRAW_AS_RADIO
};

static gint btk_check_menu_item_expose               (BtkWidget             *widget,
						      BdkEventExpose        *event);
static void btk_check_menu_item_activate             (BtkMenuItem           *menu_item);
static void btk_check_menu_item_toggle_size_request  (BtkMenuItem           *menu_item,
						      gint                  *requisition);
static void btk_check_menu_item_draw_indicator       (BtkCheckMenuItem      *check_menu_item,
						      BdkRectangle          *area);
static void btk_real_check_menu_item_draw_indicator  (BtkCheckMenuItem      *check_menu_item,
						      BdkRectangle          *area);
static void btk_check_menu_item_set_property         (BObject               *object,
						      guint                  prop_id,
						      const BValue          *value,
						      BParamSpec            *pspec);
static void btk_check_menu_item_get_property         (BObject               *object,
						      guint                  prop_id,
						      BValue                *value,
						      BParamSpec            *pspec);

static void btk_check_menu_item_activatable_interface_init (BtkActivatableIface  *iface);
static void btk_check_menu_item_update                     (BtkActivatable       *activatable,
							    BtkAction            *action,
							    const gchar          *property_name);
static void btk_check_menu_item_sync_action_properties     (BtkActivatable       *activatable,
							    BtkAction            *action);

static BtkActivatableIface *parent_activatable_iface;
static guint                check_menu_item_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (BtkCheckMenuItem, btk_check_menu_item, BTK_TYPE_MENU_ITEM,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_ACTIVATABLE,
						btk_check_menu_item_activatable_interface_init))

static void
btk_check_menu_item_class_init (BtkCheckMenuItemClass *klass)
{
  BObjectClass *bobject_class;
  BtkWidgetClass *widget_class;
  BtkMenuItemClass *menu_item_class;
  
  bobject_class = B_OBJECT_CLASS (klass);
  widget_class = (BtkWidgetClass*) klass;
  menu_item_class = (BtkMenuItemClass*) klass;
  
  bobject_class->set_property = btk_check_menu_item_set_property;
  bobject_class->get_property = btk_check_menu_item_get_property;

  g_object_class_install_property (bobject_class,
                                   PROP_ACTIVE,
                                   g_param_spec_boolean ("active",
                                                         P_("Active"),
                                                         P_("Whether the menu item is checked"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_INCONSISTENT,
                                   g_param_spec_boolean ("inconsistent",
                                                         P_("Inconsistent"),
                                                         P_("Whether to display an \"inconsistent\" state"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_DRAW_AS_RADIO,
                                   g_param_spec_boolean ("draw-as-radio",
                                                         P_("Draw as radio menu item"),
                                                         P_("Whether the menu item looks like a radio menu item"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));
  
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("indicator-size",
                                                             P_("Indicator Size"),
                                                             P_("Size of check or radio indicator"),
                                                             0,
                                                             G_MAXINT,
                                                             13,
                                                             BTK_PARAM_READABLE));

  widget_class->expose_event = btk_check_menu_item_expose;
  
  menu_item_class->activate = btk_check_menu_item_activate;
  menu_item_class->hide_on_activate = FALSE;
  menu_item_class->toggle_size_request = btk_check_menu_item_toggle_size_request;
  
  klass->toggled = NULL;
  klass->draw_indicator = btk_real_check_menu_item_draw_indicator;

  check_menu_item_signals[TOGGLED] =
    g_signal_new (I_("toggled"),
		  B_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkCheckMenuItemClass, toggled),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
}

static void 
btk_check_menu_item_activatable_interface_init (BtkActivatableIface  *iface)
{
  parent_activatable_iface = g_type_interface_peek_parent (iface);
  iface->update = btk_check_menu_item_update;
  iface->sync_action_properties = btk_check_menu_item_sync_action_properties;
}

static void
btk_check_menu_item_update (BtkActivatable *activatable,
			    BtkAction      *action,
			    const gchar    *property_name)
{
  BtkCheckMenuItem *check_menu_item;

  check_menu_item = BTK_CHECK_MENU_ITEM (activatable);

  parent_activatable_iface->update (activatable, action, property_name);

  if (strcmp (property_name, "active") == 0)
    {
      btk_action_block_activate (action);
      btk_check_menu_item_set_active (check_menu_item, btk_toggle_action_get_active (BTK_TOGGLE_ACTION (action)));
      btk_action_unblock_activate (action);
    }

  if (!btk_activatable_get_use_action_appearance (activatable))
    return;

  if (strcmp (property_name, "draw-as-radio") == 0)
    btk_check_menu_item_set_draw_as_radio (check_menu_item,
					   btk_toggle_action_get_draw_as_radio (BTK_TOGGLE_ACTION (action)));
}

static void
btk_check_menu_item_sync_action_properties (BtkActivatable *activatable,
		                            BtkAction      *action)
{
  BtkCheckMenuItem *check_menu_item;

  check_menu_item = BTK_CHECK_MENU_ITEM (activatable);

  parent_activatable_iface->sync_action_properties (activatable, action);

  if (!BTK_IS_TOGGLE_ACTION (action))
    return;

  btk_action_block_activate (action);
  btk_check_menu_item_set_active (check_menu_item, btk_toggle_action_get_active (BTK_TOGGLE_ACTION (action)));
  btk_action_unblock_activate (action);
  
  if (!btk_activatable_get_use_action_appearance (activatable))
    return;

  btk_check_menu_item_set_draw_as_radio (check_menu_item,
					 btk_toggle_action_get_draw_as_radio (BTK_TOGGLE_ACTION (action)));
}

BtkWidget*
btk_check_menu_item_new (void)
{
  return g_object_new (BTK_TYPE_CHECK_MENU_ITEM, NULL);
}

BtkWidget*
btk_check_menu_item_new_with_label (const gchar *label)
{
  return g_object_new (BTK_TYPE_CHECK_MENU_ITEM, 
		       "label", label,
		       NULL);
}


/**
 * btk_check_menu_item_new_with_mnemonic:
 * @label: The text of the button, with an underscore in front of the
 *         mnemonic character
 * @returns: a new #BtkCheckMenuItem
 *
 * Creates a new #BtkCheckMenuItem containing a label. The label
 * will be created using btk_label_new_with_mnemonic(), so underscores
 * in @label indicate the mnemonic for the menu item.
 **/
BtkWidget*
btk_check_menu_item_new_with_mnemonic (const gchar *label)
{
  return g_object_new (BTK_TYPE_CHECK_MENU_ITEM, 
		       "label", label,
		       "use-underline", TRUE,
		       NULL);
}

void
btk_check_menu_item_set_active (BtkCheckMenuItem *check_menu_item,
				gboolean          is_active)
{
  g_return_if_fail (BTK_IS_CHECK_MENU_ITEM (check_menu_item));

  is_active = is_active != 0;

  if (check_menu_item->active != is_active)
    btk_menu_item_activate (BTK_MENU_ITEM (check_menu_item));
}

/**
 * btk_check_menu_item_get_active:
 * @check_menu_item: a #BtkCheckMenuItem
 * 
 * Returns whether the check menu item is active. See
 * btk_check_menu_item_set_active ().
 * 
 * Return value: %TRUE if the menu item is checked.
 */
gboolean
btk_check_menu_item_get_active (BtkCheckMenuItem *check_menu_item)
{
  g_return_val_if_fail (BTK_IS_CHECK_MENU_ITEM (check_menu_item), FALSE);

  return check_menu_item->active;
}

static void
btk_check_menu_item_toggle_size_request (BtkMenuItem *menu_item,
					 gint        *requisition)
{
  guint toggle_spacing;
  guint indicator_size;
  
  g_return_if_fail (BTK_IS_CHECK_MENU_ITEM (menu_item));
  
  btk_widget_style_get (BTK_WIDGET (menu_item),
			"toggle-spacing", &toggle_spacing,
			"indicator-size", &indicator_size,
			NULL);

  *requisition = indicator_size + toggle_spacing;
}

void
btk_check_menu_item_set_show_toggle (BtkCheckMenuItem *menu_item,
				     gboolean          always)
{
  g_return_if_fail (BTK_IS_CHECK_MENU_ITEM (menu_item));

#if 0
  menu_item->always_show_toggle = always != FALSE;
#endif  
}

void
btk_check_menu_item_toggled (BtkCheckMenuItem *check_menu_item)
{
  g_signal_emit (check_menu_item, check_menu_item_signals[TOGGLED], 0);
}

/**
 * btk_check_menu_item_set_inconsistent:
 * @check_menu_item: a #BtkCheckMenuItem
 * @setting: %TRUE to display an "inconsistent" third state check
 *
 * If the user has selected a range of elements (such as some text or
 * spreadsheet cells) that are affected by a boolean setting, and the
 * current values in that range are inconsistent, you may want to
 * display the check in an "in between" state. This function turns on
 * "in between" display.  Normally you would turn off the inconsistent
 * state again if the user explicitly selects a setting. This has to be
 * done manually, btk_check_menu_item_set_inconsistent() only affects
 * visual appearance, it doesn't affect the semantics of the widget.
 * 
 **/
void
btk_check_menu_item_set_inconsistent (BtkCheckMenuItem *check_menu_item,
                                      gboolean          setting)
{
  g_return_if_fail (BTK_IS_CHECK_MENU_ITEM (check_menu_item));
  
  setting = setting != FALSE;

  if (setting != check_menu_item->inconsistent)
    {
      check_menu_item->inconsistent = setting;
      btk_widget_queue_draw (BTK_WIDGET (check_menu_item));
      g_object_notify (B_OBJECT (check_menu_item), "inconsistent");
    }
}

/**
 * btk_check_menu_item_get_inconsistent:
 * @check_menu_item: a #BtkCheckMenuItem
 * 
 * Retrieves the value set by btk_check_menu_item_set_inconsistent().
 * 
 * Return value: %TRUE if inconsistent
 **/
gboolean
btk_check_menu_item_get_inconsistent (BtkCheckMenuItem *check_menu_item)
{
  g_return_val_if_fail (BTK_IS_CHECK_MENU_ITEM (check_menu_item), FALSE);

  return check_menu_item->inconsistent;
}

/**
 * btk_check_menu_item_set_draw_as_radio:
 * @check_menu_item: a #BtkCheckMenuItem
 * @draw_as_radio: whether @check_menu_item is drawn like a #BtkRadioMenuItem
 *
 * Sets whether @check_menu_item is drawn like a #BtkRadioMenuItem
 *
 * Since: 2.4
 **/
void
btk_check_menu_item_set_draw_as_radio (BtkCheckMenuItem *check_menu_item,
				       gboolean          draw_as_radio)
{
  g_return_if_fail (BTK_IS_CHECK_MENU_ITEM (check_menu_item));
  
  draw_as_radio = draw_as_radio != FALSE;

  if (draw_as_radio != check_menu_item->draw_as_radio)
    {
      check_menu_item->draw_as_radio = draw_as_radio;

      btk_widget_queue_draw (BTK_WIDGET (check_menu_item));

      g_object_notify (B_OBJECT (check_menu_item), "draw-as-radio");
    }
}

/**
 * btk_check_menu_item_get_draw_as_radio:
 * @check_menu_item: a #BtkCheckMenuItem
 * 
 * Returns whether @check_menu_item looks like a #BtkRadioMenuItem
 * 
 * Return value: Whether @check_menu_item looks like a #BtkRadioMenuItem
 * 
 * Since: 2.4
 **/
gboolean
btk_check_menu_item_get_draw_as_radio (BtkCheckMenuItem *check_menu_item)
{
  g_return_val_if_fail (BTK_IS_CHECK_MENU_ITEM (check_menu_item), FALSE);
  
  return check_menu_item->draw_as_radio;
}

static void
btk_check_menu_item_init (BtkCheckMenuItem *check_menu_item)
{
  check_menu_item->active = FALSE;
  check_menu_item->always_show_toggle = TRUE;
}

static gint
btk_check_menu_item_expose (BtkWidget      *widget,
			    BdkEventExpose *event)
{
  if (BTK_WIDGET_CLASS (btk_check_menu_item_parent_class)->expose_event)
    BTK_WIDGET_CLASS (btk_check_menu_item_parent_class)->expose_event (widget, event);

  btk_check_menu_item_draw_indicator (BTK_CHECK_MENU_ITEM (widget), &event->area);

  return FALSE;
}

static void
btk_check_menu_item_activate (BtkMenuItem *menu_item)
{
  BtkCheckMenuItem *check_menu_item = BTK_CHECK_MENU_ITEM (menu_item);
  check_menu_item->active = !check_menu_item->active;

  btk_check_menu_item_toggled (check_menu_item);
  btk_widget_queue_draw (BTK_WIDGET (check_menu_item));

  BTK_MENU_ITEM_CLASS (btk_check_menu_item_parent_class)->activate (menu_item);

  g_object_notify (B_OBJECT (check_menu_item), "active");
}

static void
btk_check_menu_item_draw_indicator (BtkCheckMenuItem *check_menu_item,
				    BdkRectangle     *area)
{
  if (BTK_CHECK_MENU_ITEM_GET_CLASS (check_menu_item)->draw_indicator)
    BTK_CHECK_MENU_ITEM_GET_CLASS (check_menu_item)->draw_indicator (check_menu_item, area);
}

static void
btk_real_check_menu_item_draw_indicator (BtkCheckMenuItem *check_menu_item,
					 BdkRectangle     *area)
{
  BtkWidget *widget;
  BtkStateType state_type;
  BtkShadowType shadow_type;
  gint x, y;

  widget = BTK_WIDGET (check_menu_item);

  if (btk_widget_is_drawable (widget))
    {
      guint offset;
      guint toggle_size;
      guint toggle_spacing;
      guint horizontal_padding;
      guint indicator_size;

      btk_widget_style_get (widget,
 			    "toggle-spacing", &toggle_spacing,
 			    "horizontal-padding", &horizontal_padding,
			    "indicator-size", &indicator_size,
 			    NULL);

      toggle_size = BTK_MENU_ITEM (check_menu_item)->toggle_size;
      offset = BTK_CONTAINER (check_menu_item)->border_width +
	widget->style->xthickness + 2; 

      if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_LTR)
	{
	  x = widget->allocation.x + offset + horizontal_padding +
	    (toggle_size - toggle_spacing - indicator_size) / 2;
	}
      else 
	{
	  x = widget->allocation.x + widget->allocation.width -
	    offset - horizontal_padding - toggle_size + toggle_spacing +
	    (toggle_size - toggle_spacing - indicator_size) / 2;
	}
      
      y = widget->allocation.y + (widget->allocation.height - indicator_size) / 2;

      if (check_menu_item->active ||
	  check_menu_item->always_show_toggle ||
	  (btk_widget_get_state (widget) == BTK_STATE_PRELIGHT))
	{
	  state_type = btk_widget_get_state (widget);
	  
	  if (check_menu_item->inconsistent)
	    shadow_type = BTK_SHADOW_ETCHED_IN;
	  else if (check_menu_item->active)
	    shadow_type = BTK_SHADOW_IN;
	  else 
	    shadow_type = BTK_SHADOW_OUT;
	  
	  if (!btk_widget_is_sensitive (widget))
	    state_type = BTK_STATE_INSENSITIVE;

	  if (check_menu_item->draw_as_radio)
	    {
	      btk_paint_option (widget->style, widget->window,
				state_type, shadow_type,
				area, widget, "option",
				x, y, indicator_size, indicator_size);
	    }
	  else
	    {
	      btk_paint_check (widget->style, widget->window,
			       state_type, shadow_type,
			       area, widget, "check",
			       x, y, indicator_size, indicator_size);
	    }
	}
    }
}


static void
btk_check_menu_item_get_property (BObject     *object,
				  guint        prop_id,
				  BValue      *value,
				  BParamSpec  *pspec)
{
  BtkCheckMenuItem *checkitem = BTK_CHECK_MENU_ITEM (object);
  
  switch (prop_id)
    {
    case PROP_ACTIVE:
      b_value_set_boolean (value, checkitem->active);
      break;
    case PROP_INCONSISTENT:
      b_value_set_boolean (value, checkitem->inconsistent);
      break;
    case PROP_DRAW_AS_RADIO:
      b_value_set_boolean (value, checkitem->draw_as_radio);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
btk_check_menu_item_set_property (BObject      *object,
				  guint         prop_id,
				  const BValue *value,
				  BParamSpec   *pspec)
{
  BtkCheckMenuItem *checkitem = BTK_CHECK_MENU_ITEM (object);
  
  switch (prop_id)
    {
    case PROP_ACTIVE:
      btk_check_menu_item_set_active (checkitem, b_value_get_boolean (value));
      break;
    case PROP_INCONSISTENT:
      btk_check_menu_item_set_inconsistent (checkitem, b_value_get_boolean (value));
      break;
    case PROP_DRAW_AS_RADIO:
      btk_check_menu_item_set_draw_as_radio (checkitem, b_value_get_boolean (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

#define __BTK_CHECK_MENU_ITEM_C__
#include "btkaliasdef.c"
