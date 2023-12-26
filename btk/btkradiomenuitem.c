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
#include "btkaccellabel.h"
#include "btkmarshalers.h"
#include "btkradiomenuitem.h"
#include "btkactivatable.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"


enum {
  PROP_0,
  PROP_GROUP
};


static void btk_radio_menu_item_destroy        (BtkObject             *object);
static void btk_radio_menu_item_activate       (BtkMenuItem           *menu_item);
static void btk_radio_menu_item_set_property   (BObject               *object,
						buint                  prop_id,
						const BValue          *value,
						BParamSpec            *pspec);
static void btk_radio_menu_item_get_property   (BObject               *object,
						buint                  prop_id,
						BValue                *value,
						BParamSpec            *pspec);

static buint group_changed_signal = 0;

G_DEFINE_TYPE (BtkRadioMenuItem, btk_radio_menu_item, BTK_TYPE_CHECK_MENU_ITEM)

BtkWidget*
btk_radio_menu_item_new (GSList *group)
{
  BtkRadioMenuItem *radio_menu_item;

  radio_menu_item = g_object_new (BTK_TYPE_RADIO_MENU_ITEM, NULL);

  btk_radio_menu_item_set_group (radio_menu_item, group);

  return BTK_WIDGET (radio_menu_item);
}

static void
btk_radio_menu_item_set_property (BObject      *object,
				  buint         prop_id,
				  const BValue *value,
				  BParamSpec   *pspec)
{
  BtkRadioMenuItem *radio_menu_item;

  radio_menu_item = BTK_RADIO_MENU_ITEM (object);

  switch (prop_id)
    {
      GSList *slist;

    case PROP_GROUP:
      if (G_VALUE_HOLDS_OBJECT (value))
	slist = btk_radio_menu_item_get_group ((BtkRadioMenuItem*) b_value_get_object (value));
      else
	slist = NULL;
      btk_radio_menu_item_set_group (radio_menu_item, slist);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_radio_menu_item_get_property (BObject    *object,
				  buint       prop_id,
				  BValue     *value,
				  BParamSpec *pspec)
{
  switch (prop_id)
    {
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

void
btk_radio_menu_item_set_group (BtkRadioMenuItem *radio_menu_item,
			       GSList           *group)
{
  BtkWidget *old_group_singleton = NULL;
  BtkWidget *new_group_singleton = NULL;
  
  g_return_if_fail (BTK_IS_RADIO_MENU_ITEM (radio_menu_item));
  g_return_if_fail (!b_slist_find (group, radio_menu_item));

  if (radio_menu_item->group)
    {
      GSList *slist;

      radio_menu_item->group = b_slist_remove (radio_menu_item->group, radio_menu_item);
      
      if (radio_menu_item->group && !radio_menu_item->group->next)
	old_group_singleton = g_object_ref (radio_menu_item->group->data);
	  
      for (slist = radio_menu_item->group; slist; slist = slist->next)
	{
	  BtkRadioMenuItem *tmp_item;
	  
	  tmp_item = slist->data;
	  
	  tmp_item->group = radio_menu_item->group;
	}
    }
  
  if (group && !group->next)
    new_group_singleton = g_object_ref (group->data);
  
  radio_menu_item->group = b_slist_prepend (group, radio_menu_item);
  
  if (group)
    {
      GSList *slist;
      
      for (slist = group; slist; slist = slist->next)
	{
	  BtkRadioMenuItem *tmp_item;
	  
	  tmp_item = slist->data;
	  
	  tmp_item->group = radio_menu_item->group;
	}
    }
  else
    {
      BTK_CHECK_MENU_ITEM (radio_menu_item)->active = TRUE;
      /* btk_widget_set_state (BTK_WIDGET (radio_menu_item), BTK_STATE_ACTIVE);
       */
    }

  g_object_ref (radio_menu_item);

  g_object_notify (B_OBJECT (radio_menu_item), "group");
  g_signal_emit (radio_menu_item, group_changed_signal, 0);
  if (old_group_singleton)
    {
      g_signal_emit (old_group_singleton, group_changed_signal, 0);
      g_object_unref (old_group_singleton);
    }
  if (new_group_singleton)
    {
      g_signal_emit (new_group_singleton, group_changed_signal, 0);
      g_object_unref (new_group_singleton);
    }

  g_object_unref (radio_menu_item);
}


/**
 * btk_radio_menu_item_new_with_label:
 * @group: (element-type BtkRadioMenuItem) (transfer full):
 * @label: the text for the label
 *
 * Creates a new #BtkRadioMenuItem whose child is a simple #BtkLabel.
 *
 * Returns: (transfer none): A new #BtkRadioMenuItem
 */
BtkWidget*
btk_radio_menu_item_new_with_label (GSList *group,
				    const bchar *label)
{
  BtkWidget *radio_menu_item;
  BtkWidget *accel_label;

  radio_menu_item = btk_radio_menu_item_new (group);
  accel_label = btk_accel_label_new (label);
  btk_misc_set_alignment (BTK_MISC (accel_label), 0.0, 0.5);
  btk_container_add (BTK_CONTAINER (radio_menu_item), accel_label);
  btk_accel_label_set_accel_widget (BTK_ACCEL_LABEL (accel_label), radio_menu_item);
  btk_widget_show (accel_label);

  return radio_menu_item;
}


/**
 * btk_radio_menu_item_new_with_mnemonic:
 * @group: group the radio menu item is inside
 * @label: the text of the button, with an underscore in front of the
 *         mnemonic character
 * @returns: a new #BtkRadioMenuItem
 *
 * Creates a new #BtkRadioMenuItem containing a label. The label
 * will be created using btk_label_new_with_mnemonic(), so underscores
 * in @label indicate the mnemonic for the menu item.
 **/
BtkWidget*
btk_radio_menu_item_new_with_mnemonic (GSList *group,
				       const bchar *label)
{
  BtkWidget *radio_menu_item;
  BtkWidget *accel_label;

  radio_menu_item = btk_radio_menu_item_new (group);
  accel_label = g_object_new (BTK_TYPE_ACCEL_LABEL, NULL);
  btk_label_set_text_with_mnemonic (BTK_LABEL (accel_label), label);
  btk_misc_set_alignment (BTK_MISC (accel_label), 0.0, 0.5);

  btk_container_add (BTK_CONTAINER (radio_menu_item), accel_label);
  btk_accel_label_set_accel_widget (BTK_ACCEL_LABEL (accel_label), radio_menu_item);
  btk_widget_show (accel_label);

  return radio_menu_item;
}

/**
 * btk_radio_menu_item_new_from_widget:
 * @group: An existing #BtkRadioMenuItem
 *
 * Creates a new #BtkRadioMenuItem adding it to the same group as @group.
 *
 * Return value: (transfer none): The new #BtkRadioMenuItem
 *
 * Since: 2.4
 **/
BtkWidget *
btk_radio_menu_item_new_from_widget (BtkRadioMenuItem *group)
{
  GSList *list = NULL;
  
  g_return_val_if_fail (BTK_IS_RADIO_MENU_ITEM (group), NULL);

  if (group)
    list = btk_radio_menu_item_get_group (group);
  
  return btk_radio_menu_item_new (list);
}

/**
 * btk_radio_menu_item_new_with_mnemonic_from_widget:
 * @group: An existing #BtkRadioMenuItem
 * @label: the text of the button, with an underscore in front of the
 *         mnemonic character
 *
 * Creates a new BtkRadioMenuItem containing a label. The label will be
 * created using btk_label_new_with_mnemonic(), so underscores in label
 * indicate the mnemonic for the menu item.
 *
 * The new #BtkRadioMenuItem is added to the same group as @group.
 *
 * Return value: (transfer none): The new #BtkRadioMenuItem
 *
 * Since: 2.4
 **/
BtkWidget *
btk_radio_menu_item_new_with_mnemonic_from_widget (BtkRadioMenuItem *group,
						   const bchar *label)
{
  GSList *list = NULL;

  g_return_val_if_fail (BTK_IS_RADIO_MENU_ITEM (group), NULL);

  if (group)
    list = btk_radio_menu_item_get_group (group);

  return btk_radio_menu_item_new_with_mnemonic (list, label);
}

/**
 * btk_radio_menu_item_new_with_label_from_widget:
 * @group: an existing #BtkRadioMenuItem
 * @label: the text for the label
 *
 * Creates a new BtkRadioMenuItem whose child is a simple BtkLabel.
 * The new #BtkRadioMenuItem is added to the same group as @group.
 *
 * Return value: (transfer none): The new #BtkRadioMenuItem
 *
 * Since: 2.4
 **/
BtkWidget *
btk_radio_menu_item_new_with_label_from_widget (BtkRadioMenuItem *group,
						const bchar *label)
{
  GSList *list = NULL;

  g_return_val_if_fail (BTK_IS_RADIO_MENU_ITEM (group), NULL);

  if (group)
    list = btk_radio_menu_item_get_group (group);

  return btk_radio_menu_item_new_with_label (list, label);
}

/**
 * btk_radio_menu_item_get_group:
 * @radio_menu_item: a #BtkRadioMenuItem
 *
 * Returns the group to which the radio menu item belongs, as a #GList of
 * #BtkRadioMenuItem. The list belongs to BTK+ and should not be freed.
 *
 * Returns: (transfer none): the group of @radio_menu_item
 */
GSList*
btk_radio_menu_item_get_group (BtkRadioMenuItem *radio_menu_item)
{
  g_return_val_if_fail (BTK_IS_RADIO_MENU_ITEM (radio_menu_item), NULL);

  return radio_menu_item->group;
}


static void
btk_radio_menu_item_class_init (BtkRadioMenuItemClass *klass)
{
  BObjectClass *bobject_class;  
  BtkObjectClass *object_class;
  BtkMenuItemClass *menu_item_class;

  bobject_class = B_OBJECT_CLASS (klass);
  object_class = BTK_OBJECT_CLASS (klass);
  menu_item_class = BTK_MENU_ITEM_CLASS (klass);

  bobject_class->set_property = btk_radio_menu_item_set_property;
  bobject_class->get_property = btk_radio_menu_item_get_property;

  /**
   * BtkRadioMenuItem:group:
   * 
   * The radio menu item whose group this widget belongs to.
   * 
   * Since: 2.8
   */
  g_object_class_install_property (bobject_class,
				   PROP_GROUP,
				   g_param_spec_object ("group",
							P_("Group"),
							P_("The radio menu item whose group this widget belongs to."),
							BTK_TYPE_RADIO_MENU_ITEM,
							BTK_PARAM_WRITABLE));

  object_class->destroy = btk_radio_menu_item_destroy;

  menu_item_class->activate = btk_radio_menu_item_activate;

  /**
   * BtkStyle::group-changed:
   * @style: the object which received the signal
   *
   * Emitted when the group of radio menu items that a radio menu item belongs
   * to changes. This is emitted when a radio menu item switches from
   * being alone to being part of a group of 2 or more menu items, or
   * vice-versa, and when a button is moved from one group of 2 or
   * more menu items ton a different one, but not when the composition
   * of the group that a menu item belongs to changes.
   *
   * Since: 2.4
   */
  group_changed_signal = g_signal_new (I_("group-changed"),
				       B_OBJECT_CLASS_TYPE (object_class),
				       G_SIGNAL_RUN_FIRST,
				       G_STRUCT_OFFSET (BtkRadioMenuItemClass, group_changed),
				       NULL, NULL,
				       _btk_marshal_VOID__VOID,
				       B_TYPE_NONE, 0);
}

static void
btk_radio_menu_item_init (BtkRadioMenuItem *radio_menu_item)
{
  radio_menu_item->group = b_slist_prepend (NULL, radio_menu_item);
  btk_check_menu_item_set_draw_as_radio (BTK_CHECK_MENU_ITEM (radio_menu_item), TRUE);
}

static void
btk_radio_menu_item_destroy (BtkObject *object)
{
  BtkRadioMenuItem *radio_menu_item = BTK_RADIO_MENU_ITEM (object);
  BtkWidget *old_group_singleton = NULL;
  BtkRadioMenuItem *tmp_menu_item;
  GSList *tmp_list;
  bboolean was_in_group;

  was_in_group = radio_menu_item->group && radio_menu_item->group->next;
  
  radio_menu_item->group = b_slist_remove (radio_menu_item->group,
					   radio_menu_item);
  if (radio_menu_item->group && !radio_menu_item->group->next)
    old_group_singleton = radio_menu_item->group->data;

  tmp_list = radio_menu_item->group;

  while (tmp_list)
    {
      tmp_menu_item = tmp_list->data;
      tmp_list = tmp_list->next;

      tmp_menu_item->group = radio_menu_item->group;
    }

  /* this radio menu item is no longer in the group */
  radio_menu_item->group = NULL;
  
  if (old_group_singleton)
    g_signal_emit (old_group_singleton, group_changed_signal, 0);
  if (was_in_group)
    g_signal_emit (radio_menu_item, group_changed_signal, 0);

  BTK_OBJECT_CLASS (btk_radio_menu_item_parent_class)->destroy (object);
}

static void
btk_radio_menu_item_activate (BtkMenuItem *menu_item)
{
  BtkRadioMenuItem *radio_menu_item = BTK_RADIO_MENU_ITEM (menu_item);
  BtkCheckMenuItem *check_menu_item = BTK_CHECK_MENU_ITEM (menu_item);
  BtkCheckMenuItem *tmp_menu_item;
  BtkAction        *action;
  GSList *tmp_list;
  bint toggled;

  action = btk_activatable_get_related_action (BTK_ACTIVATABLE (menu_item));
  if (action && btk_menu_item_get_submenu (menu_item) == NULL)
    btk_action_activate (action);

  toggled = FALSE;

  if (check_menu_item->active)
    {
      tmp_menu_item = NULL;
      tmp_list = radio_menu_item->group;

      while (tmp_list)
	{
	  tmp_menu_item = tmp_list->data;
	  tmp_list = tmp_list->next;

	  if (tmp_menu_item->active && (tmp_menu_item != check_menu_item))
	    break;

	  tmp_menu_item = NULL;
	}

      if (tmp_menu_item)
	{
	  toggled = TRUE;
	  check_menu_item->active = !check_menu_item->active;
	}
    }
  else
    {
      toggled = TRUE;
      check_menu_item->active = !check_menu_item->active;

      tmp_list = radio_menu_item->group;
      while (tmp_list)
	{
	  tmp_menu_item = tmp_list->data;
	  tmp_list = tmp_list->next;

	  if (tmp_menu_item->active && (tmp_menu_item != check_menu_item))
	    {
	      btk_menu_item_activate (BTK_MENU_ITEM (tmp_menu_item));
	      break;
	    }
	}
    }

  if (toggled)
    {
      btk_check_menu_item_toggled (check_menu_item);
    }

  btk_widget_queue_draw (BTK_WIDGET (radio_menu_item));
}

#define __BTK_RADIO_MENU_ITEM_C__
#include "btkaliasdef.c"
