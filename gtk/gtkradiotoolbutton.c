/* btkradiotoolbutton.c
 *
 * Copyright (C) 2002 Anders Carlsson <andersca@bunny.og>
 * Copyright (C) 2002 James Henstridge <james@daa.com.au>
 * Copyright (C) 2003 Soeren Sandmann <sandmann@daimi.au.dk>
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
#include "btkradiotoolbutton.h"
#include "btkradiobutton.h"
#include "btkintl.h"
#include "btkprivate.h"
#include "btkalias.h"

enum {
  PROP_0,
  PROP_GROUP
};

static void btk_radio_tool_button_set_property (GObject         *object,
						guint            prop_id,
						const GValue    *value,
						GParamSpec      *pspec);

G_DEFINE_TYPE (BtkRadioToolButton, btk_radio_tool_button, BTK_TYPE_TOGGLE_TOOL_BUTTON)

static void
btk_radio_tool_button_class_init (BtkRadioToolButtonClass *klass)
{
  GObjectClass *object_class;
  BtkToolButtonClass *toolbutton_class;

  object_class = (GObjectClass *)klass;
  toolbutton_class = (BtkToolButtonClass *)klass;

  object_class->set_property = btk_radio_tool_button_set_property;
  
  toolbutton_class->button_type = BTK_TYPE_RADIO_BUTTON;  

  /**
   * BtkRadioToolButton:group:
   *
   * Sets a new group for a radio tool button.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
				   PROP_GROUP,
				   g_param_spec_object ("group",
							P_("Group"),
							P_("The radio tool button whose group this button belongs to."),
							BTK_TYPE_RADIO_TOOL_BUTTON,
							BTK_PARAM_WRITABLE));

}

static void
btk_radio_tool_button_init (BtkRadioToolButton *button)
{
  BtkToolButton *tool_button = BTK_TOOL_BUTTON (button);
  btk_toggle_button_set_mode (BTK_TOGGLE_BUTTON (_btk_tool_button_get_button (tool_button)), FALSE);
}

static void
btk_radio_tool_button_set_property (GObject         *object,
				    guint            prop_id,
				    const GValue    *value,
				    GParamSpec      *pspec)
{
  BtkRadioToolButton *button;

  button = BTK_RADIO_TOOL_BUTTON (object);

  switch (prop_id)
    {
    case PROP_GROUP:
      {
	BtkRadioToolButton *arg;
	GSList *slist = NULL;
	if (G_VALUE_HOLDS_OBJECT (value)) 
	  {
	    arg = BTK_RADIO_TOOL_BUTTON (g_value_get_object (value));
	    if (arg)
	      slist = btk_radio_tool_button_get_group (arg);
	    btk_radio_tool_button_set_group (button, slist);
	  }
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * btk_radio_tool_button_new:
 * @group: (allow-none): An existing radio button group, or %NULL if you are creating a new group
 * 
 * Creates a new #BtkRadioToolButton, adding it to @group.
 * 
 * Return value: The new #BtkRadioToolButton
 * 
 * Since: 2.4
 **/
BtkToolItem *
btk_radio_tool_button_new (GSList *group)
{
  BtkRadioToolButton *button;
  
  button = g_object_new (BTK_TYPE_RADIO_TOOL_BUTTON,
			 NULL);

  btk_radio_tool_button_set_group (button, group);
  
  return BTK_TOOL_ITEM (button);
}

/**
 * btk_radio_tool_button_new_from_stock:
 * @group: (allow-none): an existing radio button group, or %NULL if you are creating a new group
 * @stock_id: the name of a stock item
 * 
 * Creates a new #BtkRadioToolButton, adding it to @group. 
 * The new #BtkRadioToolButton will contain an icon and label from the
 * stock item indicated by @stock_id.
 * 
 * Return value: The new #BtkRadioToolItem
 * 
 * Since: 2.4
 **/
BtkToolItem *
btk_radio_tool_button_new_from_stock (GSList      *group,
				      const gchar *stock_id)
{
  BtkRadioToolButton *button;

  g_return_val_if_fail (stock_id != NULL, NULL);
  
  button = g_object_new (BTK_TYPE_RADIO_TOOL_BUTTON,
			 "stock-id", stock_id,
			 NULL);


  btk_radio_tool_button_set_group (button, group);
  
  return BTK_TOOL_ITEM (button);
}

/**
 * btk_radio_tool_button_new_from_widget:
 * @group: An existing #BtkRadioToolButton
 *
 * Creates a new #BtkRadioToolButton adding it to the same group as @gruup
 *
 * Return value: (transfer none): The new #BtkRadioToolButton
 *
 * Since: 2.4
 **/
BtkToolItem *
btk_radio_tool_button_new_from_widget (BtkRadioToolButton *group)
{
  GSList *list = NULL;
  
  g_return_val_if_fail (BTK_IS_RADIO_TOOL_BUTTON (group), NULL);

  if (group)
    list = btk_radio_tool_button_get_group (BTK_RADIO_TOOL_BUTTON (group));
  
  return btk_radio_tool_button_new (list);
}

/**
 * btk_radio_tool_button_new_with_stock_from_widget:
 * @group: An existing #BtkRadioToolButton.
 * @stock_id: the name of a stock item
 *
 * Creates a new #BtkRadioToolButton adding it to the same group as @group.
 * The new #BtkRadioToolButton will contain an icon and label from the
 * stock item indicated by @stock_id.
 *
 * Return value: (transfer none): A new #BtkRadioToolButton
 *
 * Since: 2.4
 **/
BtkToolItem *
btk_radio_tool_button_new_with_stock_from_widget (BtkRadioToolButton *group,
						  const gchar        *stock_id)
{
  GSList *list = NULL;
  
  g_return_val_if_fail (BTK_IS_RADIO_TOOL_BUTTON (group), NULL);

  if (group)
    list = btk_radio_tool_button_get_group (group);
  
  return btk_radio_tool_button_new_from_stock (list, stock_id);
}

static BtkRadioButton *
get_radio_button (BtkRadioToolButton *button)
{
  return BTK_RADIO_BUTTON (_btk_tool_button_get_button (BTK_TOOL_BUTTON (button)));
}

/**
 * btk_radio_tool_button_get_group:
 * @button: a #BtkRadioToolButton
 *
 * Returns the radio button group @button belongs to.
 *
 * Return value: (transfer none): The group @button belongs to.
 *
 * Since: 2.4
 */
GSList *
btk_radio_tool_button_get_group (BtkRadioToolButton *button)
{
  g_return_val_if_fail (BTK_IS_RADIO_TOOL_BUTTON (button), NULL);

  return btk_radio_button_get_group (get_radio_button (button));
}

/**
 * btk_radio_tool_button_set_group:
 * @button: a #BtkRadioToolButton
 * @group: an existing radio button group
 * 
 * Adds @button to @group, removing it from the group it belonged to before.
 * 
 * Since: 2.4
 **/
void
btk_radio_tool_button_set_group (BtkRadioToolButton *button,
				 GSList             *group)
{
  g_return_if_fail (BTK_IS_RADIO_TOOL_BUTTON (button));

  btk_radio_button_set_group (get_radio_button (button), group);
}

#define __BTK_RADIO_TOOL_BUTTON_C__
#include "btkaliasdef.c"
