/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2002 Sun Microsystems Inc.
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
#include "bailradiomenuitem.h"
#include "bailradiosubmenuitem.h"

static void      bail_radio_menu_item_class_init        (BailRadioMenuItemClass *klass);
static void      bail_radio_menu_item_init              (BailRadioMenuItem      *radio_menu_item);

static BatkRelationSet* bail_radio_menu_item_ref_relation_set (BatkObject       *obj);

G_DEFINE_TYPE (BailRadioMenuItem, bail_radio_menu_item, BAIL_TYPE_CHECK_MENU_ITEM)

static void
bail_radio_menu_item_class_init (BailRadioMenuItemClass *klass)
{
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);

  class->ref_relation_set = bail_radio_menu_item_ref_relation_set;
}

BatkObject* 
bail_radio_menu_item_new (BtkWidget *widget)
{
  BObject *object;
  BatkObject *accessible;

  g_return_val_if_fail (BTK_IS_RADIO_MENU_ITEM (widget), NULL);

  if (btk_menu_item_get_submenu (BTK_MENU_ITEM (widget)))
    return bail_radio_sub_menu_item_new (widget);

  object = g_object_new (BAIL_TYPE_RADIO_MENU_ITEM, NULL);

  accessible = BATK_OBJECT (object);
  batk_object_initialize (accessible, widget);

  accessible->role = BATK_ROLE_RADIO_MENU_ITEM;
  return accessible;
}

static void
bail_radio_menu_item_init (BailRadioMenuItem *radio_menu_item)
{
  radio_menu_item->old_group = NULL;
}

BatkRelationSet*
bail_radio_menu_item_ref_relation_set (BatkObject *obj)
{
  BtkWidget *widget;
  BatkRelationSet *relation_set;
  GSList *list;
  BailRadioMenuItem *radio_menu_item;

  g_return_val_if_fail (BAIL_IS_RADIO_MENU_ITEM (obj), NULL);

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
  {
    /*
     * State is defunct
     */
    return NULL;
  }
  radio_menu_item = BAIL_RADIO_MENU_ITEM (obj);

  relation_set = BATK_OBJECT_CLASS (bail_radio_menu_item_parent_class)->ref_relation_set (obj);

  /*
   * If the radio menu_item'group has changed remove the relation
   */
  list = btk_radio_menu_item_get_group (BTK_RADIO_MENU_ITEM (widget));
  
  if (radio_menu_item->old_group != list)
    {
      BatkRelation *relation;

      relation = batk_relation_set_get_relation_by_type (relation_set, BATK_RELATION_MEMBER_OF);
      batk_relation_set_remove (relation_set, relation);
    }

  if (!batk_relation_set_contains (relation_set, BATK_RELATION_MEMBER_OF))
  {
    /*
     * Get the members of the menu_item group
     */

    radio_menu_item->old_group = list;
    if (list)
    {
      BatkObject **accessible_array;
      buint list_length;
      BatkRelation* relation;
      bint i = 0;

      list_length = b_slist_length (list);
      accessible_array = (BatkObject**) g_malloc (sizeof (BatkObject *) * 
                          list_length);
      while (list != NULL)
      {
        BtkWidget* list_item = list->data;

        accessible_array[i++] = btk_widget_get_accessible (list_item);

        list = list->next;
      }
      relation = batk_relation_new (accessible_array, list_length,
                                   BATK_RELATION_MEMBER_OF);
      g_free (accessible_array);

      batk_relation_set_add (relation_set, relation);
      /*
       * Unref the relation so that it is not leaked.
       */
      g_object_unref (relation);
    }
  }
  return relation_set;
}
