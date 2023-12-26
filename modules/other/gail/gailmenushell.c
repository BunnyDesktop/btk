/* BAIL - The GNOME Accessibility Implementation Library
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
#include "bailmenushell.h"

static void         bail_menu_shell_class_init          (BailMenuShellClass *klass);
static void         bail_menu_shell_init                (BailMenuShell      *menu_shell);
static void         bail_menu_shell_initialize          (BatkObject          *accessible,
                                                         gpointer            data);
static void         batk_selection_interface_init        (BatkSelectionIface  *iface);
static gboolean     bail_menu_shell_add_selection       (BatkSelection   *selection,
                                                         gint           i);
static gboolean     bail_menu_shell_clear_selection     (BatkSelection   *selection);
static BatkObject*   bail_menu_shell_ref_selection       (BatkSelection   *selection,
                                                         gint           i);
static gint         bail_menu_shell_get_selection_count (BatkSelection   *selection);
static gboolean     bail_menu_shell_is_child_selected   (BatkSelection   *selection,
                                                         gint           i);
static gboolean     bail_menu_shell_remove_selection    (BatkSelection   *selection,
                                                         gint           i);

G_DEFINE_TYPE_WITH_CODE (BailMenuShell, bail_menu_shell, BAIL_TYPE_CONTAINER,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_SELECTION, batk_selection_interface_init))

static void
bail_menu_shell_class_init (BailMenuShellClass *klass)
{
  BatkObjectClass *batk_object_class = BATK_OBJECT_CLASS (klass);

  batk_object_class->initialize = bail_menu_shell_initialize;
}

static void
bail_menu_shell_init (BailMenuShell *menu_shell)
{
}

static void
bail_menu_shell_initialize (BatkObject *accessible,
                            gpointer  data)
{
  BATK_OBJECT_CLASS (bail_menu_shell_parent_class)->initialize (accessible, data);

  if (BTK_IS_MENU_BAR (data))
    accessible->role = BATK_ROLE_MENU_BAR;
  else
    /*
     * Accessible object for Menu is created in bailmenu.c
     */
    accessible->role = BATK_ROLE_UNKNOWN;
}

static void
batk_selection_interface_init (BatkSelectionIface *iface)
{
  iface->add_selection = bail_menu_shell_add_selection;
  iface->clear_selection = bail_menu_shell_clear_selection;
  iface->ref_selection = bail_menu_shell_ref_selection;
  iface->get_selection_count = bail_menu_shell_get_selection_count;
  iface->is_child_selected = bail_menu_shell_is_child_selected;
  iface->remove_selection = bail_menu_shell_remove_selection;
  /*
   * select_all_selection does not make sense for a menu_shell
   * so no implementation is provided.
   */
}

static gboolean
bail_menu_shell_add_selection (BatkSelection *selection,
                               gint          i)
{
  BtkMenuShell *shell;
  GList *item;
  guint length;
  BtkWidget *widget;

  widget =  BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
  {
    /* State is defunct */
    return FALSE;
  }

  shell = BTK_MENU_SHELL (widget);
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
bail_menu_shell_clear_selection (BatkSelection   *selection)
{
  BtkMenuShell *shell;
  BtkWidget *widget;

  widget =  BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
  {
    /* State is defunct */
    return FALSE;
  }

  shell = BTK_MENU_SHELL (widget);

  btk_menu_shell_deselect (shell);
  return TRUE;
}

static BatkObject*
bail_menu_shell_ref_selection (BatkSelection   *selection,
                               gint           i)
{
  BtkMenuShell *shell;
  BatkObject *obj;
  BtkWidget *widget;

  if (i != 0)
    return NULL;

  widget =  BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
  {
    /* State is defunct */
    return NULL;
  }

  shell = BTK_MENU_SHELL (widget);
  
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
bail_menu_shell_get_selection_count (BatkSelection   *selection)
{
  BtkMenuShell *shell;
  BtkWidget *widget;

  widget =  BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
  {
    /* State is defunct */
    return 0;
  }

  shell = BTK_MENU_SHELL (widget);

  /*
   * Identifies the currently selected menu item
   */
  if (shell->active_menu_item == NULL)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

static gboolean
bail_menu_shell_is_child_selected (BatkSelection   *selection,
                                   gint           i)
{
  BtkMenuShell *shell;
  gint j;
  BtkWidget *widget;

  widget =  BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
  {
    /* State is defunct */
    return FALSE;
  }

  shell = BTK_MENU_SHELL (widget);
  if (shell->active_menu_item == NULL)
    return FALSE;
  
  j = g_list_index (shell->children, shell->active_menu_item);

  return (j==i);   
}

static gboolean
bail_menu_shell_remove_selection (BatkSelection   *selection,
                                  gint           i)
{
  BtkMenuShell *shell;
  BtkWidget *widget;

  if (i != 0)
    return FALSE;

  widget =  BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
  {
    /* State is defunct */
    return FALSE;
  }

  shell = BTK_MENU_SHELL (widget);

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
