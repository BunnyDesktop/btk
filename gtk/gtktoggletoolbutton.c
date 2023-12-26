 /* btktoggletoolbutton.c
 *
 * Copyright (C) 2002 Anders Carlsson <andersca@bunny.org>
 * Copyright (C) 2002 James Henstridge <james@daa.com.au>
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
#include "btktoggletoolbutton.h"
#include "btkcheckmenuitem.h"
#include "btklabel.h"
#include "btktogglebutton.h"
#include "btkstock.h"
#include "btkintl.h"
#include "btkradiotoolbutton.h"
#include "btktoggleaction.h"
#include "btkactivatable.h"
#include "btkprivate.h"
#include "btkalias.h"

#define MENU_ID "btk-toggle-tool-button-menu-id"

enum {
  TOGGLED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_ACTIVE
};


#define BTK_TOGGLE_TOOL_BUTTON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_TOGGLE_TOOL_BUTTON, BtkToggleToolButtonPrivate))

struct _BtkToggleToolButtonPrivate
{
  guint active : 1;
};
  

static void     btk_toggle_tool_button_set_property        (GObject      *object,
							    guint         prop_id,
							    const GValue *value,
							    GParamSpec   *pspec);
static void     btk_toggle_tool_button_get_property        (GObject      *object,
							    guint         prop_id,
							    GValue       *value,
							    GParamSpec   *pspec);

static gboolean btk_toggle_tool_button_create_menu_proxy (BtkToolItem *button);

static void button_toggled      (BtkWidget           *widget,
				 BtkToggleToolButton *button);
static void menu_item_activated (BtkWidget           *widget,
				 BtkToggleToolButton *button);


static void btk_toggle_tool_button_activatable_interface_init (BtkActivatableIface  *iface);
static void btk_toggle_tool_button_update                     (BtkActivatable       *activatable,
							       BtkAction            *action,
							       const gchar          *property_name);
static void btk_toggle_tool_button_sync_action_properties     (BtkActivatable       *activatable,
							       BtkAction            *action);

static BtkActivatableIface *parent_activatable_iface;
static guint                toggle_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (BtkToggleToolButton, btk_toggle_tool_button, BTK_TYPE_TOOL_BUTTON,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_ACTIVATABLE,
						btk_toggle_tool_button_activatable_interface_init))

static void
btk_toggle_tool_button_class_init (BtkToggleToolButtonClass *klass)
{
  GObjectClass *object_class;
  BtkToolItemClass *toolitem_class;
  BtkToolButtonClass *toolbutton_class;

  object_class = (GObjectClass *)klass;
  toolitem_class = (BtkToolItemClass *)klass;
  toolbutton_class = (BtkToolButtonClass *)klass;

  object_class->set_property = btk_toggle_tool_button_set_property;
  object_class->get_property = btk_toggle_tool_button_get_property;

  toolitem_class->create_menu_proxy = btk_toggle_tool_button_create_menu_proxy;
  toolbutton_class->button_type = BTK_TYPE_TOGGLE_BUTTON;

  /**
   * BtkToggleToolButton:active:
   *
   * If the toggle tool button should be pressed in or not.
   *
   * Since: 2.8
   */
  g_object_class_install_property (object_class,
                                   PROP_ACTIVE,
                                   g_param_spec_boolean ("active",
							 P_("Active"),
							 P_("If the toggle button should be pressed in or not"),
							 FALSE,
							 BTK_PARAM_READWRITE));

/**
 * BtkToggleToolButton::toggled:
 * @toggle_tool_button: the object that emitted the signal
 *
 * Emitted whenever the toggle tool button changes state.
 **/
  toggle_signals[TOGGLED] =
    g_signal_new (I_("toggled"),
		  G_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkToggleToolButtonClass, toggled),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  g_type_class_add_private (object_class, sizeof (BtkToggleToolButtonPrivate));
}

static void
btk_toggle_tool_button_init (BtkToggleToolButton *button)
{
  BtkToolButton *tool_button = BTK_TOOL_BUTTON (button);
  BtkToggleButton *toggle_button = BTK_TOGGLE_BUTTON (_btk_tool_button_get_button (tool_button));

  button->priv = BTK_TOGGLE_TOOL_BUTTON_GET_PRIVATE (button);

  /* If the real button is a radio button, it may have been
   * active at the time it was created.
   */
  button->priv->active = btk_toggle_button_get_active (toggle_button);
    
  g_signal_connect_object (toggle_button,
			   "toggled", G_CALLBACK (button_toggled), button, 0);
}

static void
btk_toggle_tool_button_set_property (GObject      *object,
				     guint         prop_id,
				     const GValue *value,
				     GParamSpec   *pspec)
{
  BtkToggleToolButton *button = BTK_TOGGLE_TOOL_BUTTON (object);

  switch (prop_id)
    {
      case PROP_ACTIVE:
	btk_toggle_tool_button_set_active (button, 
					   g_value_get_boolean (value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
btk_toggle_tool_button_get_property (GObject    *object,
				     guint       prop_id,
				     GValue     *value,
				     GParamSpec *pspec)
{
  BtkToggleToolButton *button = BTK_TOGGLE_TOOL_BUTTON (object);

  switch (prop_id)
    {
      case PROP_ACTIVE:
        g_value_set_boolean (value, btk_toggle_tool_button_get_active (button));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static gboolean
btk_toggle_tool_button_create_menu_proxy (BtkToolItem *item)
{
  BtkToolButton *tool_button = BTK_TOOL_BUTTON (item);
  BtkToggleToolButton *toggle_tool_button = BTK_TOGGLE_TOOL_BUTTON (item);
  BtkWidget *menu_item = NULL;
  BtkStockItem stock_item;
  gboolean use_mnemonic = TRUE;
  const char *label;
  BtkWidget *label_widget;
  const gchar *label_text;
  const gchar *stock_id;

  if (_btk_tool_item_create_menu_proxy (item))
    return TRUE;

  label_widget = btk_tool_button_get_label_widget (tool_button);
  label_text = btk_tool_button_get_label (tool_button);
  stock_id = btk_tool_button_get_stock_id (tool_button);

  if (BTK_IS_LABEL (label_widget))
    {
      label = btk_label_get_label (BTK_LABEL (label_widget));
      use_mnemonic = btk_label_get_use_underline (BTK_LABEL (label_widget));
    }
  else if (label_text)
    {
      label = label_text;
      use_mnemonic = btk_tool_button_get_use_underline (tool_button);
    }
  else if (stock_id && btk_stock_lookup (stock_id, &stock_item))
    {
      label = stock_item.label;
    }
  else
    {
      label = "";
    }
  
  if (use_mnemonic)
    menu_item = btk_check_menu_item_new_with_mnemonic (label);
  else
    menu_item = btk_check_menu_item_new_with_label (label);

  btk_check_menu_item_set_active (BTK_CHECK_MENU_ITEM (menu_item),
				  toggle_tool_button->priv->active);

  if (BTK_IS_RADIO_TOOL_BUTTON (toggle_tool_button))
    {
      btk_check_menu_item_set_draw_as_radio (BTK_CHECK_MENU_ITEM (menu_item),
					     TRUE);
    }

  g_signal_connect_closure_by_id (menu_item,
				  g_signal_lookup ("activate", G_OBJECT_TYPE (menu_item)), 0,
				  g_cclosure_new_object (G_CALLBACK (menu_item_activated),
							 G_OBJECT (toggle_tool_button)),
				  FALSE);

  btk_tool_item_set_proxy_menu_item (item, MENU_ID, menu_item);
  
  return TRUE;
}

/* There are two activatable widgets, a toggle button and a menu item.
 *
 * If a widget is activated and the state of the tool button is the same as
 * the new state of the activated widget, then the other widget was the one
 * that was activated by the user and updated the tool button's state.
 *
 * If the state of the tool button is not the same as the new state of the
 * activated widget, then the activation was activated by the user, and the
 * widget needs to make sure the tool button is updated before the other
 * widget is activated. This will make sure the other widget a tool button
 * in a state that matches its own new state.
 */
static void
menu_item_activated (BtkWidget           *menu_item,
		     BtkToggleToolButton *toggle_tool_button)
{
  BtkToolButton *tool_button = BTK_TOOL_BUTTON (toggle_tool_button);
  gboolean menu_active = btk_check_menu_item_get_active (BTK_CHECK_MENU_ITEM (menu_item));

  if (toggle_tool_button->priv->active != menu_active)
    {
      toggle_tool_button->priv->active = menu_active;

      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (_btk_tool_button_get_button (tool_button)),
				    toggle_tool_button->priv->active);

      g_object_notify (G_OBJECT (toggle_tool_button), "active");
      g_signal_emit (toggle_tool_button, toggle_signals[TOGGLED], 0);
    }
}

static void
button_toggled (BtkWidget           *widget,
		BtkToggleToolButton *toggle_tool_button)
{
  gboolean toggle_active = BTK_TOGGLE_BUTTON (widget)->active;

  if (toggle_tool_button->priv->active != toggle_active)
    {
      BtkWidget *menu_item;
      
      toggle_tool_button->priv->active = toggle_active;
       
      if ((menu_item =
	   btk_tool_item_get_proxy_menu_item (BTK_TOOL_ITEM (toggle_tool_button), MENU_ID)))
	{
	  btk_check_menu_item_set_active (BTK_CHECK_MENU_ITEM (menu_item),
					  toggle_tool_button->priv->active);
	}

      g_object_notify (G_OBJECT (toggle_tool_button), "active");
      g_signal_emit (toggle_tool_button, toggle_signals[TOGGLED], 0);
    }
}

static void
btk_toggle_tool_button_activatable_interface_init (BtkActivatableIface *iface)
{
  parent_activatable_iface = g_type_interface_peek_parent (iface);
  iface->update = btk_toggle_tool_button_update;
  iface->sync_action_properties = btk_toggle_tool_button_sync_action_properties;
}

static void
btk_toggle_tool_button_update (BtkActivatable *activatable,
			       BtkAction      *action,
			       const gchar    *property_name)
{
  BtkToggleToolButton *button;

  parent_activatable_iface->update (activatable, action, property_name);

  button = BTK_TOGGLE_TOOL_BUTTON (activatable);

  if (strcmp (property_name, "active") == 0)
    {
      btk_action_block_activate (action);
      btk_toggle_tool_button_set_active (button, btk_toggle_action_get_active (BTK_TOGGLE_ACTION (action)));
      btk_action_unblock_activate (action);
    }
}

static void
btk_toggle_tool_button_sync_action_properties (BtkActivatable *activatable,
					       BtkAction      *action)
{
  BtkToggleToolButton *button;

  parent_activatable_iface->sync_action_properties (activatable, action);

  if (!BTK_IS_TOGGLE_ACTION (action))
    return;

  button = BTK_TOGGLE_TOOL_BUTTON (activatable);

  btk_action_block_activate (action);
  btk_toggle_tool_button_set_active (button, btk_toggle_action_get_active (BTK_TOGGLE_ACTION (action)));
  btk_action_unblock_activate (action);
}


/**
 * btk_toggle_tool_button_new:
 * 
 * Returns a new #BtkToggleToolButton
 * 
 * Return value: a newly created #BtkToggleToolButton
 * 
 * Since: 2.4
 **/
BtkToolItem *
btk_toggle_tool_button_new (void)
{
  BtkToolButton *button;

  button = g_object_new (BTK_TYPE_TOGGLE_TOOL_BUTTON,
			 NULL);
  
  return BTK_TOOL_ITEM (button);
}

/**
 * btk_toggle_tool_button_new_from_stock:
 * @stock_id: the name of the stock item 
 *
 * Creates a new #BtkToggleToolButton containing the image and text from a
 * stock item. Some stock ids have preprocessor macros like #BTK_STOCK_OK
 * and #BTK_STOCK_APPLY.
 *
 * It is an error if @stock_id is not a name of a stock item.
 * 
 * Return value: A new #BtkToggleToolButton
 * 
 * Since: 2.4
 **/
BtkToolItem *
btk_toggle_tool_button_new_from_stock (const gchar *stock_id)
{
  BtkToolButton *button;

  g_return_val_if_fail (stock_id != NULL, NULL);
  
  button = g_object_new (BTK_TYPE_TOGGLE_TOOL_BUTTON,
			 "stock-id", stock_id,
			 NULL);
  
  return BTK_TOOL_ITEM (button);
}

/**
 * btk_toggle_tool_button_set_active:
 * @button: a #BtkToggleToolButton
 * @is_active: whether @button should be active
 * 
 * Sets the status of the toggle tool button. Set to %TRUE if you
 * want the BtkToggleButton to be 'pressed in', and %FALSE to raise it.
 * This action causes the toggled signal to be emitted.
 * 
 * Since: 2.4
 **/
void
btk_toggle_tool_button_set_active (BtkToggleToolButton *button,
				   gboolean is_active)
{
  g_return_if_fail (BTK_IS_TOGGLE_TOOL_BUTTON (button));

  is_active = is_active != FALSE;

  if (button->priv->active != is_active)
    btk_button_clicked (BTK_BUTTON (_btk_tool_button_get_button (BTK_TOOL_BUTTON (button))));
}

/**
 * btk_toggle_tool_button_get_active:
 * @button: a #BtkToggleToolButton
 * 
 * Queries a #BtkToggleToolButton and returns its current state.
 * Returns %TRUE if the toggle button is pressed in and %FALSE if it is raised.
 * 
 * Return value: %TRUE if the toggle tool button is pressed in, %FALSE if not
 * 
 * Since: 2.4
 **/
gboolean
btk_toggle_tool_button_get_active (BtkToggleToolButton *button)
{
  g_return_val_if_fail (BTK_IS_TOGGLE_TOOL_BUTTON (button), FALSE);

  return button->priv->active;
}

#define __BTK_TOGGLE_TOOL_BUTTON_C__
#include "btkaliasdef.c"
