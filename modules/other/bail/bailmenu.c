/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2001, 2002, 2003 Sun Microsystems Inc.
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

#undef BTK_DISABLE_DEPRECATED

#include "bailmenu.h"

static void bail_menu_class_init (BailMenuClass *klass);
static void bail_menu_init       (BailMenu      *accessible);

static void	  bail_menu_real_initialize     (BatkObject *obj,
                                                 gpointer  data);

static BatkObject* bail_menu_get_parent          (BatkObject *accessible);
static gint       bail_menu_get_index_in_parent (BatkObject *accessible);

G_DEFINE_TYPE (BailMenu, bail_menu, BAIL_TYPE_MENU_SHELL)

static void
bail_menu_class_init (BailMenuClass *klass)
{
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);

  class->get_parent = bail_menu_get_parent;
  class->get_index_in_parent = bail_menu_get_index_in_parent;
  class->initialize = bail_menu_real_initialize;
}

static void
bail_menu_init (BailMenu *accessible)
{
}

static void
bail_menu_real_initialize (BatkObject *obj,
                           gpointer  data)
{
  BATK_OBJECT_CLASS (bail_menu_parent_class)->initialize (obj, data);

  obj->role = BATK_ROLE_MENU;

  g_object_set_data (B_OBJECT (obj), "batk-component-layer",
		     GINT_TO_POINTER (BATK_LAYER_POPUP));
}

static BatkObject*
bail_menu_get_parent (BatkObject *accessible)
{
  BatkObject *parent;

  parent = accessible->accessible_parent;

  if (parent != NULL)
    {
      g_return_val_if_fail (BATK_IS_OBJECT (parent), NULL);
    }
  else
    {
      BtkWidget *widget, *parent_widget;

      widget = BTK_ACCESSIBLE (accessible)->widget;
      if (widget == NULL)
        {
          /*
           * State is defunct
           */
          return NULL;
        }
      g_return_val_if_fail (BTK_IS_MENU (widget), NULL);

      /*
       * If the menu is attached to a menu item or a button (Bunny Menu)
       * report the menu item as parent.
       */
      parent_widget = btk_menu_get_attach_widget (BTK_MENU (widget));

      if (!BTK_IS_MENU_ITEM (parent_widget) && !BTK_IS_BUTTON (parent_widget) && !BTK_IS_COMBO_BOX (parent_widget) && !BTK_IS_OPTION_MENU (parent_widget))
        parent_widget = widget->parent;

      if (parent_widget == NULL)
        return NULL;

      parent = btk_widget_get_accessible (parent_widget);
      batk_object_set_parent (accessible, parent);
    }
  return parent;
}

static gint
bail_menu_get_index_in_parent (BatkObject *accessible)
{
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (accessible)->widget;

  if (widget == NULL)
    {
      /*
       * State is defunct
       */
      return -1;
    }
  g_return_val_if_fail (BTK_IS_MENU (widget), -1);

  if (btk_menu_get_attach_widget (BTK_MENU (widget)))
    {
      return 0;
    }
  return BATK_OBJECT_CLASS (bail_menu_parent_class)->get_index_in_parent (accessible);
}
