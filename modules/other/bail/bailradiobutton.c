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

#include <btk/btk.h>
#include "bailradiobutton.h"

static void      bail_radio_button_class_init        (BailRadioButtonClass *klass);
static void      bail_radio_button_init              (BailRadioButton      *radio_button);
static void      bail_radio_button_initialize        (BatkObject            *accessible,
                                                      gpointer              data);

static BatkRelationSet* bail_radio_button_ref_relation_set (BatkObject       *obj);

G_DEFINE_TYPE (BailRadioButton, bail_radio_button, BAIL_TYPE_TOGGLE_BUTTON)

static void
bail_radio_button_class_init (BailRadioButtonClass *klass)
{
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);

  class->initialize = bail_radio_button_initialize;
  class->ref_relation_set = bail_radio_button_ref_relation_set;
}

static void
bail_radio_button_init (BailRadioButton *radio_button)
{
  radio_button->old_group = NULL;
}

static void
bail_radio_button_initialize (BatkObject *accessible,
                              gpointer  data)
{
  BATK_OBJECT_CLASS (bail_radio_button_parent_class)->initialize (accessible, data);

  accessible->role = BATK_ROLE_RADIO_BUTTON;
}

BatkRelationSet*
bail_radio_button_ref_relation_set (BatkObject *obj)
{
  BtkWidget *widget;
  BatkRelationSet *relation_set;
  GSList *list;
  BailRadioButton *radio_button;

  g_return_val_if_fail (BAIL_IS_RADIO_BUTTON (obj), NULL);

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
  {
    /*
     * State is defunct
     */
    return NULL;
  }
  radio_button = BAIL_RADIO_BUTTON (obj);

  relation_set = BATK_OBJECT_CLASS (bail_radio_button_parent_class)->ref_relation_set (obj);

  /*
   * If the radio button'group has changed remove the relation
   */
  list = btk_radio_button_get_group (BTK_RADIO_BUTTON (widget));
  
  if (radio_button->old_group != list)
    {
      BatkRelation *relation;

      relation = batk_relation_set_get_relation_by_type (relation_set, BATK_RELATION_MEMBER_OF);
      batk_relation_set_remove (relation_set, relation);
    }

  if (!batk_relation_set_contains (relation_set, BATK_RELATION_MEMBER_OF))
  {
    /*
     * Get the members of the button group
     */

    radio_button->old_group = list;
    if (list)
    {
      BatkObject **accessible_array;
      guint list_length;
      BatkRelation* relation;
      gint i = 0;

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
