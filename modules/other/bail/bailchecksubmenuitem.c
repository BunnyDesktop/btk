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

#include <string.h>
#include <btk/btk.h>
#include "bailchecksubmenuitem.h"

static void      bail_check_sub_menu_item_class_init        (BailCheckSubMenuItemClass *klass);

static void      bail_check_sub_menu_item_init              (BailCheckSubMenuItem   *item);

static void      bail_check_sub_menu_item_toggled_btk       (BtkWidget              *widget);

static void      bail_check_sub_menu_item_real_notify_btk   (BObject                *obj,
                                                             BParamSpec             *pspec);

static void      bail_check_sub_menu_item_real_initialize   (BatkObject              *obj,
                                                             bpointer               data);

static BatkStateSet* bail_check_sub_menu_item_ref_state_set  (BatkObject              *accessible);

G_DEFINE_TYPE (BailCheckSubMenuItem, bail_check_sub_menu_item, BAIL_TYPE_SUB_MENU_ITEM)

static void
bail_check_sub_menu_item_class_init (BailCheckSubMenuItemClass *klass)
{
  BailWidgetClass *widget_class;
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);

  widget_class = (BailWidgetClass*)klass;
  widget_class->notify_btk = bail_check_sub_menu_item_real_notify_btk;

  class->ref_state_set = bail_check_sub_menu_item_ref_state_set;
  class->initialize = bail_check_sub_menu_item_real_initialize;
}

static void
bail_check_sub_menu_item_init (BailCheckSubMenuItem *item)
{
}

BatkObject* 
bail_check_sub_menu_item_new (BtkWidget *widget)
{
  BObject *object;
  BatkObject *accessible;

  g_return_val_if_fail (BTK_IS_CHECK_MENU_ITEM (widget), NULL);

  object = g_object_new (BAIL_TYPE_CHECK_SUB_MENU_ITEM, NULL);

  accessible = BATK_OBJECT (object);
  batk_object_initialize (accessible, widget);

  return accessible;
}

static void
bail_check_sub_menu_item_real_initialize (BatkObject *obj,
                                          bpointer  data)
{
  BATK_OBJECT_CLASS (bail_check_sub_menu_item_parent_class)->initialize (obj, data);

  g_signal_connect (data,
                    "toggled",
                    G_CALLBACK (bail_check_sub_menu_item_toggled_btk),
                    NULL);

  obj->role = BATK_ROLE_CHECK_MENU_ITEM;
}

static void
bail_check_sub_menu_item_toggled_btk (BtkWidget       *widget)
{
  BatkObject *accessible;
  BtkCheckMenuItem *check_menu_item;

  check_menu_item = BTK_CHECK_MENU_ITEM (widget);

  accessible = btk_widget_get_accessible (widget);
  batk_object_notify_state_change (accessible, BATK_STATE_CHECKED, 
                                  check_menu_item->active);
} 

static BatkStateSet*
bail_check_sub_menu_item_ref_state_set (BatkObject *accessible)
{
  BatkStateSet *state_set;
  BtkCheckMenuItem *check_menu_item;
  BtkWidget *widget;

  state_set = BATK_OBJECT_CLASS (bail_check_sub_menu_item_parent_class)->ref_state_set (accessible);
  widget = BTK_ACCESSIBLE (accessible)->widget;
 
  if (widget == NULL)
    return state_set;

  check_menu_item = BTK_CHECK_MENU_ITEM (widget);

  if (btk_check_menu_item_get_active (check_menu_item))
    batk_state_set_add_state (state_set, BATK_STATE_CHECKED);

  if (btk_check_menu_item_get_inconsistent (check_menu_item))
    {
      batk_state_set_remove_state (state_set, BATK_STATE_ENABLED);
      batk_state_set_add_state (state_set, BATK_STATE_INDETERMINATE);
    }
 
  return state_set;
}

static void
bail_check_sub_menu_item_real_notify_btk (BObject           *obj,
                                          BParamSpec        *pspec)
{
  BtkCheckMenuItem *check_menu_item = BTK_CHECK_MENU_ITEM (obj);
  BatkObject *batk_obj;
  bboolean sensitive;
  bboolean inconsistent;

  batk_obj = btk_widget_get_accessible (BTK_WIDGET (check_menu_item));
  sensitive = btk_widget_get_sensitive (BTK_WIDGET (check_menu_item));
  inconsistent = btk_check_menu_item_get_inconsistent (check_menu_item);

  if (strcmp (pspec->name, "inconsistent") == 0)
    {
      batk_object_notify_state_change (batk_obj, BATK_STATE_INDETERMINATE, inconsistent);
      batk_object_notify_state_change (batk_obj, BATK_STATE_ENABLED, (sensitive && !inconsistent));
    }
  else if (strcmp (pspec->name, "sensitive") == 0)
    {
      /* Need to override bailwidget behavior of notifying for ENABLED */
      batk_object_notify_state_change (batk_obj, BATK_STATE_SENSITIVE, sensitive);
      batk_object_notify_state_change (batk_obj, BATK_STATE_ENABLED, (sensitive && !inconsistent));
    }
  else
    BAIL_WIDGET_CLASS (bail_check_sub_menu_item_parent_class)->notify_btk (obj, pspec);
}
