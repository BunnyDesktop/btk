/* BTK - The GIMP Toolkit
 * Recent chooser action for BtkUIManager
 *
 * Copyright (C) 2007, Emmanuele Bassi
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

#include "btkintl.h"
#include "btkrecentaction.h"
#include "btkimagemenuitem.h"
#include "btkmenutoolbutton.h"
#include "btkrecentchooser.h"
#include "btkrecentchoosermenu.h"
#include "btkrecentchooserutils.h"
#include "btkrecentchooserprivate.h"
#include "btkprivate.h"
#include "btkalias.h"

#define FALLBACK_ITEM_LIMIT     10

#define BTK_RECENT_ACTION_GET_PRIVATE(obj)      \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj),    \
         BTK_TYPE_RECENT_ACTION,                \
         BtkRecentActionPrivate))

struct _BtkRecentActionPrivate
{
  BtkRecentManager *manager;

  guint show_numbers   : 1;

  /* RecentChooser properties */
  guint show_private   : 1;
  guint show_not_found : 1;
  guint show_tips      : 1;
  guint show_icons     : 1;
  guint local_only     : 1;

  gint limit;

  BtkRecentSortType sort_type;
  BtkRecentSortFunc sort_func;
  gpointer          sort_data;
  GDestroyNotify    data_destroy;

  BtkRecentFilter *current_filter;

  GSList *choosers;
  BtkRecentChooser *current_chooser;
};

enum
{
  PROP_0,

  PROP_SHOW_NUMBERS
};

static void btk_recent_chooser_iface_init (BtkRecentChooserIface *iface);

G_DEFINE_TYPE_WITH_CODE (BtkRecentAction,
                         btk_recent_action,
                         BTK_TYPE_ACTION,
                         G_IMPLEMENT_INTERFACE (BTK_TYPE_RECENT_CHOOSER,
                                                btk_recent_chooser_iface_init));

static gboolean
btk_recent_action_set_current_uri (BtkRecentChooser  *chooser,
                                   const gchar       *uri,
                                   GError           **error)
{
  BtkRecentAction *action = BTK_RECENT_ACTION (chooser);
  BtkRecentActionPrivate *priv = action->priv;
  GSList *l;

  for (l = priv->choosers; l; l = l->next)
    {
      BtkRecentChooser *recent_chooser = l->data;

      if (!btk_recent_chooser_set_current_uri (recent_chooser, uri, error))
        return FALSE;
    }

  return TRUE;
}

static gchar *
btk_recent_action_get_current_uri (BtkRecentChooser *chooser)
{
  BtkRecentAction *recent_action = BTK_RECENT_ACTION (chooser);
  BtkRecentActionPrivate *priv = recent_action->priv;

  if (priv->current_chooser)
    return btk_recent_chooser_get_current_uri (priv->current_chooser);

  return NULL;
}

static gboolean
btk_recent_action_select_uri (BtkRecentChooser  *chooser,
                              const gchar       *uri,
                              GError           **error)
{
  BtkRecentAction *action = BTK_RECENT_ACTION (chooser);
  BtkRecentActionPrivate *priv = action->priv;
  GSList *l;

  for (l = priv->choosers; l; l = l->next)
    {
      BtkRecentChooser *recent_chooser = l->data;

      if (!btk_recent_chooser_select_uri (recent_chooser, uri, error))
        return FALSE;
    }

  return TRUE;
}

static void
btk_recent_action_unselect_uri (BtkRecentChooser *chooser,
                                const gchar      *uri)
{
  BtkRecentAction *action = BTK_RECENT_ACTION (chooser);
  BtkRecentActionPrivate *priv = action->priv;
  GSList *l;

  for (l = priv->choosers; l; l = l->next)
    {
      BtkRecentChooser *chooser = l->data;
      
      btk_recent_chooser_unselect_uri (chooser, uri);
    }
}

static void
btk_recent_action_select_all (BtkRecentChooser *chooser)
{
  g_warning (_("This function is not implemented for "
               "widgets of class '%s'"),
             g_type_name (G_OBJECT_TYPE (chooser)));
}

static void
btk_recent_action_unselect_all (BtkRecentChooser *chooser)
{
  g_warning (_("This function is not implemented for "
               "widgets of class '%s'"),
             g_type_name (G_OBJECT_TYPE (chooser)));
}


static GList *
btk_recent_action_get_items (BtkRecentChooser *chooser)
{
  BtkRecentAction *action = BTK_RECENT_ACTION (chooser);
  BtkRecentActionPrivate *priv = action->priv;

  return _btk_recent_chooser_get_items (chooser,
                                        priv->current_filter,
                                        priv->sort_func,
                                        priv->sort_data);
}

static BtkRecentManager *
btk_recent_action_get_recent_manager (BtkRecentChooser *chooser)
{
  return BTK_RECENT_ACTION_GET_PRIVATE (chooser)->manager;
}

static void
btk_recent_action_set_sort_func (BtkRecentChooser  *chooser,
                                 BtkRecentSortFunc  sort_func,
                                 gpointer           sort_data,
                                 GDestroyNotify     data_destroy)
{
  BtkRecentAction *action = BTK_RECENT_ACTION (chooser);
  BtkRecentActionPrivate *priv = action->priv;
  GSList *l;
  
  if (priv->data_destroy)
    {
      priv->data_destroy (priv->sort_data);
      priv->data_destroy = NULL;
    }
      
  priv->sort_func = NULL;
  priv->sort_data = NULL;
  
  if (sort_func)
    {
      priv->sort_func = sort_func;
      priv->sort_data = sort_data;
      priv->data_destroy = data_destroy;
    }

  for (l = priv->choosers; l; l = l->next)
    {
      BtkRecentChooser *chooser_menu = l->data;
      
      btk_recent_chooser_set_sort_func (chooser_menu, priv->sort_func,
                                        priv->sort_data,
                                        priv->data_destroy);
    }
}

static void
set_current_filter (BtkRecentAction *action,
                    BtkRecentFilter *filter)
{
  BtkRecentActionPrivate *priv = action->priv;

  g_object_ref (action);

  if (priv->current_filter)
    g_object_unref (priv->current_filter);

  priv->current_filter = filter;

  if (priv->current_filter)
    g_object_ref_sink (priv->current_filter);

  g_object_notify (G_OBJECT (action), "filter");

  g_object_unref (action);
}

static void
btk_recent_action_add_filter (BtkRecentChooser *chooser,
                              BtkRecentFilter  *filter)
{
  BtkRecentActionPrivate *priv = BTK_RECENT_ACTION_GET_PRIVATE (chooser);

  if (priv->current_filter != filter)
    set_current_filter (BTK_RECENT_ACTION (chooser), filter);
}

static void
btk_recent_action_remove_filter (BtkRecentChooser *chooser,
                                 BtkRecentFilter  *filter)
{
  BtkRecentActionPrivate *priv = BTK_RECENT_ACTION_GET_PRIVATE (chooser);

  if (priv->current_filter == filter)
    set_current_filter (BTK_RECENT_ACTION (chooser), NULL);
}

static GSList *
btk_recent_action_list_filters (BtkRecentChooser *chooser)
{
  GSList *retval = NULL;
  BtkRecentFilter *current_filter;

  current_filter = BTK_RECENT_ACTION_GET_PRIVATE (chooser)->current_filter;
  retval = g_slist_prepend (retval, current_filter);

  return retval;
}


static void
btk_recent_chooser_iface_init (BtkRecentChooserIface *iface)
{
  iface->set_current_uri = btk_recent_action_set_current_uri;
  iface->get_current_uri = btk_recent_action_get_current_uri;
  iface->select_uri = btk_recent_action_select_uri;
  iface->unselect_uri = btk_recent_action_unselect_uri;
  iface->select_all = btk_recent_action_select_all;
  iface->unselect_all = btk_recent_action_unselect_all;
  iface->get_items = btk_recent_action_get_items;
  iface->get_recent_manager = btk_recent_action_get_recent_manager;
  iface->set_sort_func = btk_recent_action_set_sort_func;
  iface->add_filter = btk_recent_action_add_filter;
  iface->remove_filter = btk_recent_action_remove_filter;
  iface->list_filters = btk_recent_action_list_filters;
}

static void
btk_recent_action_activate (BtkAction *action)
{
  /* we have probably been invoked by a menu tool button or by a
   * direct call of btk_action_activate(); since no item has been
   * selected, we must unset the current recent chooser pointer
   */
  BTK_RECENT_ACTION_GET_PRIVATE (action)->current_chooser = NULL;
}

static void
delegate_selection_changed (BtkRecentAction  *action,
                            BtkRecentChooser *chooser)
{
  BtkRecentActionPrivate *priv = action->priv;

  priv->current_chooser = chooser;

  g_signal_emit_by_name (action, "selection-changed");
}

static void
delegate_item_activated (BtkRecentAction  *action,
                         BtkRecentChooser *chooser)
{
  BtkRecentActionPrivate *priv = action->priv;

  priv->current_chooser = chooser;

  g_signal_emit_by_name (action, "item-activated");
}

static void
btk_recent_action_connect_proxy (BtkAction *action,
                                 BtkWidget *widget)
{
  BtkRecentAction *recent_action = BTK_RECENT_ACTION (action);
  BtkRecentActionPrivate *priv = recent_action->priv;

  /* it can only be a recent chooser implementor anyway... */
  if (BTK_IS_RECENT_CHOOSER (widget) &&
      !g_slist_find (priv->choosers, widget))
    {
      if (priv->sort_func)
        {
          btk_recent_chooser_set_sort_func (BTK_RECENT_CHOOSER (widget),
                                            priv->sort_func,
                                            priv->sort_data,
                                            priv->data_destroy);
        }

      g_signal_connect_swapped (widget, "selection_changed",
                                G_CALLBACK (delegate_selection_changed),
                                action);
      g_signal_connect_swapped (widget, "item-activated",
                                G_CALLBACK (delegate_item_activated),
                                action);
    }

  if (BTK_ACTION_CLASS (btk_recent_action_parent_class)->connect_proxy)
    BTK_ACTION_CLASS (btk_recent_action_parent_class)->connect_proxy (action, widget);
}

static void
btk_recent_action_disconnect_proxy (BtkAction *action,
                                    BtkWidget *widget)
{
  BtkRecentAction *recent_action = BTK_RECENT_ACTION (action);
  BtkRecentActionPrivate *priv = recent_action->priv;

  /* if it was one of the recent choosers we created, remove it
   * from the list
   */
  if (g_slist_find (priv->choosers, widget))
    priv->choosers = g_slist_remove (priv->choosers, widget);

  if (BTK_ACTION_CLASS (btk_recent_action_parent_class)->disconnect_proxy)
    BTK_ACTION_CLASS (btk_recent_action_parent_class)->disconnect_proxy (action, widget);
}

static BtkWidget *
btk_recent_action_create_menu (BtkAction *action)
{
  BtkRecentAction *recent_action = BTK_RECENT_ACTION (action);
  BtkRecentActionPrivate *priv = recent_action->priv;
  BtkWidget *widget;

  widget = g_object_new (BTK_TYPE_RECENT_CHOOSER_MENU,
                         "show-private", priv->show_private,
                         "show-not-found", priv->show_not_found,
                         "show-tips", priv->show_tips,
                         "show-icons", priv->show_icons,
                         "show-numbers", priv->show_numbers,
                         "limit", priv->limit,
                         "sort-type", priv->sort_type,
                         "recent-manager", priv->manager,
                         "filter", priv->current_filter,
                         "local-only", priv->local_only,
                         NULL);
  
  if (priv->sort_func)
    {
      btk_recent_chooser_set_sort_func (BTK_RECENT_CHOOSER (widget),
                                        priv->sort_func,
                                        priv->sort_data,
                                        priv->data_destroy);
    }

  g_signal_connect_swapped (widget, "selection_changed",
                            G_CALLBACK (delegate_selection_changed),
                            recent_action);
  g_signal_connect_swapped (widget, "item-activated",
                            G_CALLBACK (delegate_item_activated),
                            recent_action);

  /* keep track of the choosers we create */
  priv->choosers = g_slist_prepend (priv->choosers, widget);

  return widget;
}

static BtkWidget *
btk_recent_action_create_menu_item (BtkAction *action)
{
  BtkWidget *menu;
  BtkWidget *menuitem;

  menu = btk_recent_action_create_menu (action);
  menuitem = g_object_new (BTK_TYPE_IMAGE_MENU_ITEM, NULL);
  btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), menu);
  btk_widget_show (menu);

  return menuitem;
}

static BtkWidget *
btk_recent_action_create_tool_item (BtkAction *action)
{
  BtkWidget *menu;
  BtkWidget *toolitem;

  menu = btk_recent_action_create_menu (action);
  toolitem = g_object_new (BTK_TYPE_MENU_TOOL_BUTTON, NULL);
  btk_menu_tool_button_set_menu (BTK_MENU_TOOL_BUTTON (toolitem), menu);
  btk_widget_show (menu);

  return toolitem;
}

static void
set_recent_manager (BtkRecentAction  *action,
                    BtkRecentManager *manager)
{
  BtkRecentActionPrivate *priv = action->priv;

  if (manager)
    priv->manager = NULL;
  else
    priv->manager = btk_recent_manager_get_default ();
}

static void
btk_recent_action_finalize (GObject *bobject)
{
  BtkRecentAction *action = BTK_RECENT_ACTION (bobject);
  BtkRecentActionPrivate *priv = action->priv;

  priv->manager = NULL;
  
  if (priv->data_destroy)
    {
      priv->data_destroy (priv->sort_data);
      priv->data_destroy = NULL;
    }

  priv->sort_data = NULL;
  priv->sort_func = NULL;

  g_slist_free (priv->choosers);

  G_OBJECT_CLASS (btk_recent_action_parent_class)->finalize (bobject);
}

static void
btk_recent_action_dispose (GObject *bobject)
{
  BtkRecentAction *action = BTK_RECENT_ACTION (bobject);
  BtkRecentActionPrivate *priv = action->priv;

  if (priv->current_filter)
    {
      g_object_unref (priv->current_filter);
      priv->current_filter = NULL;
    }

  G_OBJECT_CLASS (btk_recent_action_parent_class)->dispose (bobject);
}

static void
btk_recent_action_set_property (GObject      *bobject,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  BtkRecentAction *action = BTK_RECENT_ACTION (bobject);
  BtkRecentActionPrivate *priv = action->priv;

  switch (prop_id)
    {
    case PROP_SHOW_NUMBERS:
      priv->show_numbers = g_value_get_boolean (value);
      break;
    case BTK_RECENT_CHOOSER_PROP_SHOW_PRIVATE:
      priv->show_private = g_value_get_boolean (value);
      break;
    case BTK_RECENT_CHOOSER_PROP_SHOW_NOT_FOUND:
      priv->show_not_found = g_value_get_boolean (value);
      break;
    case BTK_RECENT_CHOOSER_PROP_SHOW_TIPS:
      priv->show_tips = g_value_get_boolean (value);
      break;
    case BTK_RECENT_CHOOSER_PROP_SHOW_ICONS:
      priv->show_icons = g_value_get_boolean (value);
      break;
    case BTK_RECENT_CHOOSER_PROP_LIMIT:
      priv->limit = g_value_get_int (value);
      break;
    case BTK_RECENT_CHOOSER_PROP_LOCAL_ONLY:
      priv->local_only = g_value_get_boolean (value);
      break;
    case BTK_RECENT_CHOOSER_PROP_SORT_TYPE:
      priv->sort_type = g_value_get_enum (value);
      break;
    case BTK_RECENT_CHOOSER_PROP_FILTER:
      set_current_filter (action, g_value_get_object (value));
      break;
    case BTK_RECENT_CHOOSER_PROP_SELECT_MULTIPLE:
      g_warning ("%s: Choosers of type `%s' do not support selecting multiple items.",
                 G_STRFUNC,
                 G_OBJECT_TYPE_NAME (bobject));
      return;
    case BTK_RECENT_CHOOSER_PROP_RECENT_MANAGER:
      set_recent_manager (action, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (bobject, prop_id, pspec);
      return;
    }
}

static void
btk_recent_action_get_property (GObject    *bobject,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  BtkRecentActionPrivate *priv = BTK_RECENT_ACTION_GET_PRIVATE (bobject);

  switch (prop_id)
    {
    case PROP_SHOW_NUMBERS:
      g_value_set_boolean (value, priv->show_numbers);
      break;
    case BTK_RECENT_CHOOSER_PROP_SHOW_PRIVATE:
      g_value_set_boolean (value, priv->show_private);
      break;
    case BTK_RECENT_CHOOSER_PROP_SHOW_NOT_FOUND:
      g_value_set_boolean (value, priv->show_not_found);
      break;
    case BTK_RECENT_CHOOSER_PROP_SHOW_TIPS:
      g_value_set_boolean (value, priv->show_tips);
      break;
    case BTK_RECENT_CHOOSER_PROP_SHOW_ICONS:
      g_value_set_boolean (value, priv->show_icons);
      break;
    case BTK_RECENT_CHOOSER_PROP_LIMIT:
      g_value_set_int (value, priv->limit);
      break;
    case BTK_RECENT_CHOOSER_PROP_LOCAL_ONLY:
      g_value_set_boolean (value, priv->local_only);
      break;
    case BTK_RECENT_CHOOSER_PROP_SORT_TYPE:
      g_value_set_enum (value, priv->sort_type);
      break;
    case BTK_RECENT_CHOOSER_PROP_FILTER:
      g_value_set_object (value, priv->current_filter);
      break;
    case BTK_RECENT_CHOOSER_PROP_SELECT_MULTIPLE:
      g_value_set_boolean (value, FALSE);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (bobject, prop_id, pspec);
      break;
    }
}

static void
btk_recent_action_class_init (BtkRecentActionClass *klass)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (klass);
  BtkActionClass *action_class = BTK_ACTION_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtkRecentActionPrivate));

  bobject_class->finalize = btk_recent_action_finalize;
  bobject_class->dispose = btk_recent_action_dispose;
  bobject_class->set_property = btk_recent_action_set_property;
  bobject_class->get_property = btk_recent_action_get_property;

  action_class->activate = btk_recent_action_activate;
  action_class->connect_proxy = btk_recent_action_connect_proxy;
  action_class->disconnect_proxy = btk_recent_action_disconnect_proxy;
  action_class->create_menu_item = btk_recent_action_create_menu_item;
  action_class->create_tool_item = btk_recent_action_create_tool_item;
  action_class->create_menu = btk_recent_action_create_menu;
  action_class->menu_item_type = BTK_TYPE_IMAGE_MENU_ITEM;
  action_class->toolbar_item_type = BTK_TYPE_MENU_TOOL_BUTTON;

  _btk_recent_chooser_install_properties (bobject_class);

  g_object_class_install_property (bobject_class,
                                   PROP_SHOW_NUMBERS,
                                   g_param_spec_boolean ("show-numbers",
                                                         P_("Show Numbers"),
                                                         P_("Whether the items should be displayed with a number"),
                                                         FALSE,
                                                         G_PARAM_READWRITE));

}

static void
btk_recent_action_init (BtkRecentAction *action)
{
  BtkRecentActionPrivate *priv;

  action->priv = priv = BTK_RECENT_ACTION_GET_PRIVATE (action);

  priv->show_numbers = FALSE;
  priv->show_icons = TRUE;
  priv->show_tips = FALSE;
  priv->show_not_found = TRUE;
  priv->show_private = FALSE;
  priv->local_only = TRUE;

  priv->limit = FALLBACK_ITEM_LIMIT;

  priv->sort_type = BTK_RECENT_SORT_NONE;
  priv->sort_func = NULL;
  priv->sort_data = NULL;
  priv->data_destroy = NULL;

  priv->current_filter = NULL;

  priv->manager = NULL;
}

/**
 * btk_recent_action_new:
 * @name: a unique name for the action
 * @label: (allow-none): the label displayed in menu items and on buttons, or %NULL
 * @tooltip: (allow-none): a tooltip for the action, or %NULL
 * @stock_id: the stock icon to display in widgets representing the
 *   action, or %NULL
 *
 * Creates a new #BtkRecentAction object. To add the action to
 * a #BtkActionGroup and set the accelerator for the action,
 * call btk_action_group_add_action_with_accel().
 *
 * Return value: the newly created #BtkRecentAction.
 *
 * Since: 2.12
 */
BtkAction *
btk_recent_action_new (const gchar *name,
                       const gchar *label,
                       const gchar *tooltip,
                       const gchar *stock_id)
{
  g_return_val_if_fail (name != NULL, NULL);

  return g_object_new (BTK_TYPE_RECENT_ACTION,
                       "name", name,
                       "label", label,
                       "tooltip", tooltip,
                       "stock-id", stock_id,
                       NULL);
}

/**
 * btk_recent_action_new_for_manager:
 * @name: a unique name for the action
 * @label: (allow-none): the label displayed in menu items and on buttons, or %NULL
 * @tooltip: (allow-none): a tooltip for the action, or %NULL
 * @stock_id: the stock icon to display in widgets representing the
 *   action, or %NULL
 * @manager: (allow-none): a #BtkRecentManager, or %NULL for using the default
 *   #BtkRecentManager
 *
 * Creates a new #BtkRecentAction object. To add the action to
 * a #BtkActionGroup and set the accelerator for the action,
 * call btk_action_group_add_action_with_accel().
 *
 * Return value: the newly created #BtkRecentAction
 * 
 * Since: 2.12
 */
BtkAction *
btk_recent_action_new_for_manager (const gchar      *name,
                                   const gchar      *label,
                                   const gchar      *tooltip,
                                   const gchar      *stock_id,
                                   BtkRecentManager *manager)
{
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (manager == NULL || BTK_IS_RECENT_MANAGER (manager), NULL);

  return g_object_new (BTK_TYPE_RECENT_ACTION,
                       "name", name,
                       "label", label,
                       "tooltip", tooltip,
                       "stock-id", stock_id,
                       "recent-manager", manager,
                       NULL);
}

/**
 * btk_recent_action_get_show_numbers:
 * @action: a #BtkRecentAction
 *
 * Returns the value set by btk_recent_chooser_menu_set_show_numbers().
 *
 * Return value: %TRUE if numbers should be shown.
 *
 * Since: 2.12
 */
gboolean
btk_recent_action_get_show_numbers (BtkRecentAction *action)
{
  g_return_val_if_fail (BTK_IS_RECENT_ACTION (action), FALSE);

  return action->priv->show_numbers;
}

/**
 * btk_recent_action_set_show_numbers:
 * @action: a #BtkRecentAction
 * @show_numbers: %TRUE if the shown items should be numbered
 *
 * Sets whether a number should be added to the items shown by the
 * widgets representing @action. The numbers are shown to provide
 * a unique character for a mnemonic to be used inside the menu item's
 * label. Only the first ten items get a number to avoid clashes.
 *
 * Since: 2.12
 */
void
btk_recent_action_set_show_numbers (BtkRecentAction *action,
                                    gboolean         show_numbers)
{
  BtkRecentActionPrivate *priv;

  g_return_if_fail (BTK_IS_RECENT_ACTION (action));

  priv = action->priv;

  if (priv->show_numbers != show_numbers)
    {
      g_object_ref (action);

      priv->show_numbers = show_numbers;

      g_object_notify (G_OBJECT (action), "show-numbers");
      g_object_unref (action);
    }
}

#define __BTK_RECENT_ACTION_C__
#include "btkaliasdef.c"
