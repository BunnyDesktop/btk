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

#include <btk/btk.h>
#include <bdk/bdkkeysyms.h>
#include "bailmenuitem.h"
#include "bailsubmenuitem.h"

#define KEYBINDING_SEPARATOR ";"

static void bail_menu_item_class_init  (BailMenuItemClass *klass);
static void bail_menu_item_init        (BailMenuItem      *menu_item);

static void                  bail_menu_item_real_initialize
                                                          (BatkObject       *obj,
                                                           bpointer        data);
static bint                  bail_menu_item_get_n_children (BatkObject      *obj);
static BatkObject*            bail_menu_item_ref_child      (BatkObject      *obj,
                                                            bint           i);
static BatkStateSet*          bail_menu_item_ref_state_set  (BatkObject      *obj);
static void                  bail_menu_item_finalize       (BObject        *object);

static void                  batk_action_interface_init     (BatkActionIface *iface);
static bboolean              bail_menu_item_do_action      (BatkAction      *action,
                                                            bint           i);
static bboolean              idle_do_action                (bpointer       data);
static bint                  bail_menu_item_get_n_actions  (BatkAction      *action);
static const bchar*          bail_menu_item_get_description(BatkAction      *action,
                                                            bint           i);
static const bchar*          bail_menu_item_get_name       (BatkAction      *action,
                                                            bint           i);
static const bchar*          bail_menu_item_get_keybinding (BatkAction      *action,
                                                            bint           i);
static bboolean              bail_menu_item_set_description(BatkAction      *action,
                                                            bint           i,
                                                            const bchar    *desc);
static void                  menu_item_select              (BtkItem        *item);
static void                  menu_item_deselect            (BtkItem        *item);
static void                  menu_item_selection           (BtkItem        *item,
                                                            bboolean       selected);
static bboolean              find_accel                    (BtkAccelKey    *key,
                                                            GClosure       *closure,
                                                            bpointer       data);
static bboolean              find_accel_new                (BtkAccelKey    *key,
                                                            GClosure       *closure,
                                                            bpointer       data);

G_DEFINE_TYPE_WITH_CODE (BailMenuItem, bail_menu_item, BAIL_TYPE_ITEM,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_ACTION, batk_action_interface_init))

static void
bail_menu_item_class_init (BailMenuItemClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);

  bobject_class->finalize = bail_menu_item_finalize;

  class->get_n_children = bail_menu_item_get_n_children;
  class->ref_child = bail_menu_item_ref_child;
  class->ref_state_set = bail_menu_item_ref_state_set;
  class->initialize = bail_menu_item_real_initialize;
}

static void
bail_menu_item_real_initialize (BatkObject *obj,
                                bpointer  data)
{
  BtkWidget *widget;
  BtkWidget *parent;

  BATK_OBJECT_CLASS (bail_menu_item_parent_class)->initialize (obj, data);

  g_signal_connect (data,
                    "select",
                    G_CALLBACK (menu_item_select),
                    NULL);
  g_signal_connect (data,
                    "deselect",
                    G_CALLBACK (menu_item_deselect),
                    NULL);
  widget = BTK_WIDGET (data);
  parent = btk_widget_get_parent (widget);
  if (BTK_IS_MENU (parent))
    {
      BtkWidget *parent_widget;

      parent_widget =  btk_menu_get_attach_widget (BTK_MENU (parent));

      if (!BTK_IS_MENU_ITEM (parent_widget))
        parent_widget = btk_widget_get_parent (widget);
       if (parent_widget)
        {
          batk_object_set_parent (obj, btk_widget_get_accessible (parent_widget));
        }
    }
  g_object_set_data (B_OBJECT (obj), "batk-component-layer",
                     BINT_TO_POINTER (BATK_LAYER_POPUP));

  if (BTK_IS_TEAROFF_MENU_ITEM (data))
    obj->role = BATK_ROLE_TEAR_OFF_MENU_ITEM;
  else if (BTK_IS_SEPARATOR_MENU_ITEM (data))
    obj->role = BATK_ROLE_SEPARATOR;
  else
    obj->role = BATK_ROLE_MENU_ITEM;
}

static void
bail_menu_item_init (BailMenuItem *menu_item)
{
  menu_item->click_keybinding = NULL;
  menu_item->click_description = NULL;
}

BatkObject*
bail_menu_item_new (BtkWidget *widget)
{
  BObject *object;
  BatkObject *accessible;
  
  g_return_val_if_fail (BTK_IS_MENU_ITEM (widget), NULL);

  if (btk_menu_item_get_submenu (BTK_MENU_ITEM (widget)))
    return bail_sub_menu_item_new (widget);

  object = g_object_new (BAIL_TYPE_MENU_ITEM, NULL);

  accessible = BATK_OBJECT (object);
  batk_object_initialize (accessible, widget);

  return accessible;
}

GList *
get_children (BtkWidget *submenu)
{
  GList *children;

  children = btk_container_get_children (BTK_CONTAINER (submenu));
  if (g_list_length (children) == 0)
    {
      /*
       * If menu is empty it may be because the menu items are created only 
       * on demand. For example, in bunny-panel the menu items are created
       * only when "show" signal is emitted on the menu.
       *
       * The following hack forces the menu items to be created.
       */
      if (!btk_widget_get_visible (submenu))
        {
          /* FIXME BTK_WIDGET_SET_FLAGS (submenu, BTK_VISIBLE); */
          g_signal_emit_by_name (submenu, "show");
          /* FIXME BTK_WIDGET_UNSET_FLAGS (submenu, BTK_VISIBLE); */
        }
      g_list_free (children);
      children = btk_container_get_children (BTK_CONTAINER (submenu));
    }
  return children;
}

/*
 * If a menu item has a submenu return the items of the submenu as the 
 * accessible children; otherwise expose no accessible children.
 */
static bint
bail_menu_item_get_n_children (BatkObject* obj)
{
  BtkWidget *widget;
  BtkWidget *submenu;
  bint count = 0;

  g_return_val_if_fail (BAIL_IS_MENU_ITEM (obj), count);

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    return count;

  submenu = btk_menu_item_get_submenu (BTK_MENU_ITEM (widget));
  if (submenu)
    {
      GList *children;

      children = get_children (submenu);
      count = g_list_length (children);
      g_list_free (children);
    }
  return count;
}

static BatkObject*
bail_menu_item_ref_child (BatkObject *obj,
                          bint       i)
{
  BatkObject  *accessible;
  BtkWidget *widget;
  BtkWidget *submenu;

  g_return_val_if_fail (BAIL_IS_MENU_ITEM (obj), NULL);
  g_return_val_if_fail ((i >= 0), NULL);

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    return NULL;

  submenu = btk_menu_item_get_submenu (BTK_MENU_ITEM (widget));
  if (submenu)
    {
      GList *children;
      GList *tmp_list;

      children = get_children (submenu);
      tmp_list = g_list_nth (children, i);
      if (!tmp_list)
        {
          g_list_free (children);
          return NULL;
        }
      accessible = btk_widget_get_accessible (BTK_WIDGET (tmp_list->data));
      g_list_free (children);
      g_object_ref (accessible);
    }
  else
    accessible = NULL;

  return accessible;
}

static BatkStateSet*
bail_menu_item_ref_state_set (BatkObject *obj)
{
  BatkObject *menu_item;
  BatkStateSet *state_set, *parent_state_set;

  state_set = BATK_OBJECT_CLASS (bail_menu_item_parent_class)->ref_state_set (obj);

  menu_item = batk_object_get_parent (obj);

  if (menu_item)
    {
      if (!BTK_IS_MENU_ITEM (BTK_ACCESSIBLE (menu_item)->widget))
        return state_set;

      parent_state_set = batk_object_ref_state_set (menu_item);
      if (!batk_state_set_contains_state (parent_state_set, BATK_STATE_SELECTED))
        {
          batk_state_set_remove_state (state_set, BATK_STATE_FOCUSED);
          batk_state_set_remove_state (state_set, BATK_STATE_SHOWING);
        }
    }
  return state_set;
}

static void
batk_action_interface_init (BatkActionIface *iface)
{
  iface->do_action = bail_menu_item_do_action;
  iface->get_n_actions = bail_menu_item_get_n_actions;
  iface->get_description = bail_menu_item_get_description;
  iface->get_name = bail_menu_item_get_name;
  iface->get_keybinding = bail_menu_item_get_keybinding;
  iface->set_description = bail_menu_item_set_description;
}

static bboolean
bail_menu_item_do_action (BatkAction *action,
                          bint      i)
{
  if (i == 0)
    {
      BtkWidget *item;
      BailMenuItem *bail_menu_item;

      item = BTK_ACCESSIBLE (action)->widget;
      if (item == NULL)
        /* State is defunct */
        return FALSE;

      if (!btk_widget_get_sensitive (item) || !btk_widget_get_visible (item))
        return FALSE;

      bail_menu_item = BAIL_MENU_ITEM (action);
      if (bail_menu_item->action_idle_handler)
        return FALSE;
      else
	{
	  bail_menu_item->action_idle_handler =
            bdk_threads_add_idle_full (G_PRIORITY_DEFAULT_IDLE,
                                       idle_do_action,
                                       g_object_ref (bail_menu_item),
                                       (GDestroyNotify) g_object_unref);
	}
      return TRUE;
    }
  else
    return FALSE;
}

static void
ensure_menus_unposted (BailMenuItem *menu_item)
{
  BatkObject *parent;
  BtkWidget *widget;

  parent = batk_object_get_parent (BATK_OBJECT (menu_item));
  while (parent)
    {
      if (BTK_IS_ACCESSIBLE (parent))
        {
          widget = BTK_ACCESSIBLE (parent)->widget;
          if (BTK_IS_MENU (widget))
            {
              if (btk_widget_get_mapped (widget))
                btk_menu_shell_cancel (BTK_MENU_SHELL (widget));

              return;
            }
        }
      parent = batk_object_get_parent (parent);
    }
}

static bboolean
idle_do_action (bpointer data)
{
  BtkWidget *item;
  BtkWidget *item_parent;
  BailMenuItem *menu_item;
  bboolean item_mapped;

  menu_item = BAIL_MENU_ITEM (data);
  menu_item->action_idle_handler = 0;
  item = BTK_ACCESSIBLE (menu_item)->widget;
  if (item == NULL /* State is defunct */ ||
      !btk_widget_get_sensitive (item) || !btk_widget_get_visible (item))
    return FALSE;

  item_parent = btk_widget_get_parent (item);
  btk_menu_shell_select_item (BTK_MENU_SHELL (item_parent), item);
  item_mapped = btk_widget_get_mapped (item);
  /*
   * This is what is called when <Return> is pressed for a menu item
   */
  g_signal_emit_by_name (item_parent, "activate_current",  
                         /*force_hide*/ 1); 
  if (!item_mapped)
    ensure_menus_unposted (menu_item);

  return FALSE;
}

static bint
bail_menu_item_get_n_actions (BatkAction *action)
{
  /*
   * Menu item has 1 action
   */
  return 1;
}

static const bchar*
bail_menu_item_get_description (BatkAction *action,
                                bint      i)
{
  if (i == 0)
    {
      BailMenuItem *item;

      item = BAIL_MENU_ITEM (action);
      return item->click_description;
    }
  else
    return NULL;
}

static const bchar*
bail_menu_item_get_name (BatkAction *action,
                         bint      i)
{
  if (i == 0)
    return "click";
  else
    return NULL;
}

static const bchar*
bail_menu_item_get_keybinding (BatkAction *action,
                               bint      i)
{
  /*
   * This function returns a string of the form A;B;C where
   * A is the keybinding for the widget; B is the keybinding to traverse
   * from the menubar and C is the accelerator.
   * The items in the keybinding to traverse from the menubar are separated
   * by ":".
   */
  BailMenuItem  *bail_menu_item;
  bchar *keybinding = NULL;
  bchar *item_keybinding = NULL;
  bchar *full_keybinding = NULL;
  bchar *accelerator = NULL;

  bail_menu_item = BAIL_MENU_ITEM (action);
  if (i == 0)
    {
      BtkWidget *item;
      BtkWidget *temp_item;
      BtkWidget *child;
      BtkWidget *parent;

      item = BTK_ACCESSIBLE (action)->widget;
      if (item == NULL)
        /* State is defunct */
        return NULL;

      temp_item = item;
      while (TRUE)
        {
          BdkModifierType mnemonic_modifier = 0;
          buint key_val;
          bchar *key, *temp_keybinding;

          child = btk_bin_get_child (BTK_BIN (temp_item));
          if (child == NULL)
            {
              /* Possibly a tear off menu item; it could also be a menu 
               * separator generated by btk_item_factory_create_items()
               */
              return NULL;
            }
          parent = btk_widget_get_parent (temp_item);
          if (!parent)
            {
              /*
               * parent can be NULL when activating a window from the panel
               */
              return NULL;
            }
          g_return_val_if_fail (BTK_IS_MENU_SHELL (parent), NULL);
          if (BTK_IS_MENU_BAR (parent))
            {
              BtkWidget *toplevel;

              toplevel = btk_widget_get_toplevel (parent);
              if (toplevel && BTK_IS_WINDOW (toplevel))
                mnemonic_modifier = btk_window_get_mnemonic_modifier (
                                       BTK_WINDOW (toplevel));
            }
          if (BTK_IS_LABEL (child))
            {
              key_val = btk_label_get_mnemonic_keyval (BTK_LABEL (child));
              if (key_val != BDK_VoidSymbol)
                {
                  key = btk_accelerator_name (key_val, mnemonic_modifier);
                  if (full_keybinding)
                    temp_keybinding = g_strconcat (key, ":", full_keybinding, NULL);
                  else 
                    temp_keybinding = g_strconcat (key, NULL);
                  if (temp_item == item)
                    {
                      item_keybinding = g_strdup (key); 
                    }
                  g_free (key);
                  g_free (full_keybinding);
                  full_keybinding = temp_keybinding;
                }
              else
                {
                  /* No keybinding */
                  g_free (full_keybinding);
                  full_keybinding = NULL;
                  break;
                }        
            }        
          if (BTK_IS_MENU_BAR (parent))
            /* We have reached the menu bar so we are finished */
            break;
          g_return_val_if_fail (BTK_IS_MENU (parent), NULL);
          temp_item = btk_menu_get_attach_widget (BTK_MENU (parent));
          if (!BTK_IS_MENU_ITEM (temp_item))
            {
              /* 
               * Menu is attached to something other than a menu item;
               * probably an option menu
               */
              g_free (full_keybinding);
              full_keybinding = NULL;
              break;
            }
        }

      parent = btk_widget_get_parent (item);
      if (BTK_IS_MENU (parent))
        {
          BtkAccelGroup *group; 
          BtkAccelKey *key;

          group = btk_menu_get_accel_group (BTK_MENU (parent));

          if (group)
            {
              key = btk_accel_group_find (group, find_accel, item);
            }
          else
            {
              /*
               * If the menu item is created using BtkAction and BtkUIManager
               * we get here.
               */
              key = NULL;
              child = BTK_BIN (item)->child;
              if (BTK_IS_ACCEL_LABEL (child))
                {
                  BtkAccelLabel *accel_label;

                  accel_label = BTK_ACCEL_LABEL (child);
                  if (accel_label->accel_closure)
                    {
                      key = btk_accel_group_find (accel_label->accel_group,
                                                  find_accel_new,
                                                  accel_label->accel_closure);
                    }
               
                }
            }

          if (key)
            {           
              accelerator = btk_accelerator_name (key->accel_key,
                                                  key->accel_mods);
            }
        }
    }
  /*
   * Concatenate the bindings
   */
  if (item_keybinding || full_keybinding || accelerator)
    {
      bchar *temp;
      if (item_keybinding)
        {
          keybinding = g_strconcat (item_keybinding, KEYBINDING_SEPARATOR, NULL);
          g_free (item_keybinding);
        }
      else
        keybinding = g_strconcat (KEYBINDING_SEPARATOR, NULL);

      if (full_keybinding)
        {
          temp = g_strconcat (keybinding, full_keybinding, 
                              KEYBINDING_SEPARATOR, NULL);
          g_free (full_keybinding);
        }
      else
        temp = g_strconcat (keybinding, KEYBINDING_SEPARATOR, NULL);

      g_free (keybinding);
      keybinding = temp;
      if (accelerator)
        {
          temp = g_strconcat (keybinding, accelerator, NULL);
          g_free (accelerator);
          g_free (keybinding);
          keybinding = temp;
      }
    }
  g_free (bail_menu_item->click_keybinding);
  bail_menu_item->click_keybinding = keybinding;
  return keybinding;
}

static bboolean
bail_menu_item_set_description (BatkAction      *action,
                                bint           i,
                                const bchar    *desc)
{
  if (i == 0)
    {
      BailMenuItem *item;

      item = BAIL_MENU_ITEM (action);
      g_free (item->click_description);
      item->click_description = g_strdup (desc);
      return TRUE;
    }
  else
    return FALSE;
}

static void
bail_menu_item_finalize (BObject *object)
{
  BailMenuItem *menu_item = BAIL_MENU_ITEM (object);

  g_free (menu_item->click_keybinding);
  g_free (menu_item->click_description);
  if (menu_item->action_idle_handler)
    {
      g_source_remove (menu_item->action_idle_handler);
      menu_item->action_idle_handler = 0;
    }

  B_OBJECT_CLASS (bail_menu_item_parent_class)->finalize (object);
}

static void
menu_item_select (BtkItem *item)
{
  menu_item_selection (item, TRUE);
}

static void
menu_item_deselect (BtkItem *item)
{
  menu_item_selection (item, FALSE);
}

static void
menu_item_selection (BtkItem  *item,
                     bboolean selected)
{
  BatkObject *obj, *parent;
  bint i;

  obj = btk_widget_get_accessible (BTK_WIDGET (item));
  batk_object_notify_state_change (obj, BATK_STATE_SELECTED, selected);

  for (i = 0; i < batk_object_get_n_accessible_children (obj); i++)
    {
      BatkObject *child;
      child = batk_object_ref_accessible_child (obj, i);
      batk_object_notify_state_change (child, BATK_STATE_SHOWING, selected);
      g_object_unref (child);
    }
  parent = batk_object_get_parent (obj);
  g_signal_emit_by_name (parent, "selection_changed"); 
}

static bboolean
find_accel (BtkAccelKey *key,
            GClosure    *closure,
            bpointer     data)
{
  /*
   * We assume that closure->data points to the widget
   * pending btk_widget_get_accel_closures being made public
   */
  return data == (bpointer) closure->data;
}

static bboolean
find_accel_new (BtkAccelKey *key,
                GClosure    *closure,
                bpointer     data)
{
  return data == (bpointer) closure;
}
