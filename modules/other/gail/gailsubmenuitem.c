/* BAIL - The GNOME Accessibility Implementation Library
 * Copyright 2002, 2003 Sun Microsystems Inc.
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
#include "bailsubmenuitem.h"

static void         bail_sub_menu_item_class_init       (BailSubMenuItemClass *klass);
static void         bail_sub_menu_item_init             (BailSubMenuItem *item);
static void         bail_sub_menu_item_real_initialize  (BatkObject      *obj,
                                                         gpointer       data);

static void         batk_selection_interface_init        (BatkSelectionIface  *iface);
static gboolean     bail_sub_menu_item_add_selection    (BatkSelection   *selection,
                                                         gint           i);
static gboolean     bail_sub_menu_item_clear_selection  (BatkSelection   *selection);
static BatkObject*   bail_sub_menu_item_ref_selection    (BatkSelection   *selection,
                                                         gint           i);
static gint         bail_sub_menu_item_get_selection_count
                                                        (BatkSelection   *selection);
static gboolean     bail_sub_menu_item_is_child_selected
                                                        (BatkSelection   *selection,
                                                         gint           i);
static gboolean     bail_sub_menu_item_remove_selection (BatkSelection   *selection,
                                                         gint           i);
static gint         menu_item_add_btk                   (BtkContainer   *container,
                                                         BtkWidget      *widget);
static gint         menu_item_remove_btk                (BtkContainer   *container,
                                                         BtkWidget      *widget);

G_DEFINE_TYPE_WITH_CODE (BailSubMenuItem, bail_sub_menu_item, BAIL_TYPE_MENU_ITEM,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_SELECTION, batk_selection_interface_init))

static void
bail_sub_menu_item_class_init (BailSubMenuItemClass *klass)
{
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);

  class->initialize = bail_sub_menu_item_real_initialize;
}

static void
bail_sub_menu_item_init (BailSubMenuItem *item)
{
}

static void
bail_sub_menu_item_real_initialize (BatkObject *obj,
                                   gpointer   data)
{
  BtkWidget *submenu;

  BATK_OBJECT_CLASS (bail_sub_menu_item_parent_class)->initialize (obj, data);

  submenu = btk_menu_item_get_submenu (BTK_MENU_ITEM (data));
  g_return_if_fail (submenu);

  g_signal_connect (submenu,
                    "add",
                    G_CALLBACK (menu_item_add_btk),
                    NULL);
  g_signal_connect (submenu,
                    "remove",
                    G_CALLBACK (menu_item_remove_btk),
                    NULL);

  obj->role = BATK_ROLE_MENU;
}

BatkObject*
bail_sub_menu_item_new (BtkWidget *widget)
{
  GObject *object;
  BatkObject *accessible;

  g_return_val_if_fail (BTK_IS_MENU_ITEM (widget), NULL);

  object = g_object_new (BAIL_TYPE_SUB_MENU_ITEM, NULL);

  accessible = BATK_OBJECT (object);
  batk_object_initialize (accessible, widget);

  return accessible;
}

static void
batk_selection_interface_init (BatkSelectionIface *iface)
{
  iface->add_selection = bail_sub_menu_item_add_selection;
  iface->clear_selection = bail_sub_menu_item_clear_selection;
  iface->ref_selection = bail_sub_menu_item_ref_selection;
  iface->get_selection_count = bail_sub_menu_item_get_selection_count;
  iface->is_child_selected = bail_sub_menu_item_is_child_selected;
  iface->remove_selection = bail_sub_menu_item_remove_selection;
  /*
   * select_all_selection does not make sense for a submenu of a menu item
   * so no implementation is provided.
   */
}

static gboolean
bail_sub_menu_item_add_selection (BatkSelection *selection,
                                  gint          i)
{
  BtkMenuShell *shell;
  GList *item;
  guint length;
  BtkWidget *widget;
  BtkWidget *submenu;

  widget =  BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;

  submenu = btk_menu_item_get_submenu (BTK_MENU_ITEM (widget));
  g_return_val_if_fail (BTK_IS_MENU_SHELL (submenu), FALSE);
  shell = BTK_MENU_SHELL (submenu);
  length = g_list_length (shell->children);
  if (i < 0 || i > length)
    return FALSE;

  item = g_list_nth (shell->children, i);
  g_return_val_if_fail (item != NULL, FALSE);
  g_return_val_if_fail (BTK_IS_MENU_ITEM(item->data), FALSE);
   
  btk_menu_shell_select_item (shell, BTK_WIDGET (item->data));
  return TRUE;
}

static gboolean
bail_sub_menu_item_clear_selection (BatkSelection   *selection)
{
  BtkMenuShell *shell;
  BtkWidget *widget;
  BtkWidget *submenu;

  widget =  BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;

  submenu = btk_menu_item_get_submenu (BTK_MENU_ITEM (widget));
  g_return_val_if_fail (BTK_IS_MENU_SHELL (submenu), FALSE);
  shell = BTK_MENU_SHELL (submenu);

  btk_menu_shell_deselect (shell);
  return TRUE;
}

static BatkObject*
bail_sub_menu_item_ref_selection (BatkSelection   *selection,
                                  gint           i)
{
  BtkMenuShell *shell;
  BatkObject *obj;
  BtkWidget *widget;
  BtkWidget *submenu;

  if (i != 0)
    return NULL;

  widget =  BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  submenu = btk_menu_item_get_submenu (BTK_MENU_ITEM (widget));
  g_return_val_if_fail (BTK_IS_MENU_SHELL (submenu), NULL);
  shell = BTK_MENU_SHELL (submenu);
  
  if (shell->active_menu_item != NULL)
    {
      obj = btk_widget_get_accessible (shell->active_menu_item);
      g_object_ref (obj);
      return obj;
    }
  else
    {
      return NULL;
    }
}

static gint
bail_sub_menu_item_get_selection_count (BatkSelection   *selection)
{
  BtkMenuShell *shell;
  BtkWidget *widget;
  BtkWidget *submenu;

  widget =  BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  submenu = btk_menu_item_get_submenu (BTK_MENU_ITEM (widget));
  g_return_val_if_fail (BTK_IS_MENU_SHELL (submenu), FALSE);
  shell = BTK_MENU_SHELL (submenu);

  /*
   * Identifies the currently selected menu item
   */
  if (shell->active_menu_item == NULL)
    return 0;
  else
    return 1;
}

static gboolean
bail_sub_menu_item_is_child_selected (BatkSelection   *selection,
                                       gint           i)
{
  BtkMenuShell *shell;
  gint j;
  BtkWidget *widget;
  BtkWidget *submenu;

  widget =  BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;

  submenu = btk_menu_item_get_submenu (BTK_MENU_ITEM (widget));
  g_return_val_if_fail (BTK_IS_MENU_SHELL (submenu), FALSE);
  shell = BTK_MENU_SHELL (submenu);

  if (shell->active_menu_item == NULL)
    return FALSE;
  
  j = g_list_index (shell->children, shell->active_menu_item);

  return (j==i);   
}

static gboolean
bail_sub_menu_item_remove_selection (BatkSelection   *selection,
                                  gint           i)
{
  BtkMenuShell *shell;
  BtkWidget *widget;
  BtkWidget *submenu;

  if (i != 0)
    return FALSE;

  widget =  BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;

  submenu = btk_menu_item_get_submenu (BTK_MENU_ITEM (widget));
  g_return_val_if_fail (BTK_IS_MENU_SHELL (submenu), FALSE);
  shell = BTK_MENU_SHELL (submenu);

  if (shell->active_menu_item && 
      BTK_MENU_ITEM (shell->active_menu_item)->submenu)
    {
    /*
     * Menu item contains a menu and it is the selected menu item
     * so deselect it.
     */
      btk_menu_shell_deselect (shell);
    }
  return TRUE;
}

static gint
menu_item_add_btk (BtkContainer *container,
                   BtkWidget	*widget)
{
  BtkWidget *parent_widget;
  BatkObject *batk_parent;
  BatkObject *batk_child;
  BailContainer *bail_container;
  gint index;

  g_return_val_if_fail (BTK_IS_MENU (container), 1);

  parent_widget = btk_menu_get_attach_widget (BTK_MENU (container));
  if (BTK_IS_MENU_ITEM (parent_widget))
    {
      batk_parent = btk_widget_get_accessible (parent_widget);
      batk_child = btk_widget_get_accessible (widget);

      bail_container = BAIL_CONTAINER (batk_parent);
      g_object_notify (G_OBJECT (batk_child), "accessible_parent");

      g_list_free (bail_container->children);
      bail_container->children = btk_container_get_children (container);
      index = g_list_index (bail_container->children, widget);
      g_signal_emit_by_name (batk_parent, "children_changed::add",
                             index, batk_child, NULL);
    }
  return 1;
}

static gint
menu_item_remove_btk (BtkContainer *container,
                      BtkWidget	   *widget)
{
  BtkWidget *parent_widget;
  BatkObject *batk_parent;
  BatkObject *batk_child;
  BailContainer *bail_container;
  BatkPropertyValues values = { NULL };
  gint index;
  gint list_length;

  g_return_val_if_fail (BTK_IS_MENU (container), 1);

  parent_widget = btk_menu_get_attach_widget (BTK_MENU (container));
  if (BTK_IS_MENU_ITEM (parent_widget))
    {
      batk_parent = btk_widget_get_accessible (parent_widget);
      batk_child = btk_widget_get_accessible (widget);

      bail_container = BAIL_CONTAINER (batk_parent);
      g_value_init (&values.old_value, G_TYPE_POINTER);
      g_value_set_pointer (&values.old_value, batk_parent);
      values.property_name = "accessible-parent";
      g_signal_emit_by_name (batk_child,
                             "property_change::accessible-parent", &values, NULL);

      index = g_list_index (bail_container->children, widget);
      list_length = g_list_length (bail_container->children);
      g_list_free (bail_container->children);
      bail_container->children = btk_container_get_children (container);
      if (index >= 0 && index <= list_length)
        g_signal_emit_by_name (batk_parent, "children_changed::remove",
                               index, batk_child, NULL);
    }
  return 1;
}
