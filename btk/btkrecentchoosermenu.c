/* BTK - The GIMP Toolkit
 * btkrecentchoosermenu.c - Recently used items menu widget
 * Copyright (C) 2005, Emmanuele Bassi
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

#include "btkrecentmanager.h"
#include "btkrecentfilter.h"
#include "btkrecentchooser.h"
#include "btkrecentchooserutils.h"
#include "btkrecentchooserprivate.h"
#include "btkrecentchoosermenu.h"

#include "btkstock.h"
#include "btkicontheme.h"
#include "btkiconfactory.h"
#include "btkintl.h"
#include "btksettings.h"
#include "btkmenushell.h"
#include "btkmenuitem.h"
#include "btkimagemenuitem.h"
#include "btkseparatormenuitem.h"
#include "btkmenu.h"
#include "btkimage.h"
#include "btklabel.h"
#include "btktooltip.h"
#include "btkactivatable.h"
#include "btktypebuiltins.h"
#include "btkprivate.h"
#include "btkalias.h"

struct _BtkRecentChooserMenuPrivate
{
  /* the recent manager object */
  BtkRecentManager *manager;
  
  /* size of the icons of the menu items */  
  bint icon_size;

  /* max size of the menu item label */
  bint label_width;

  bint first_recent_item_pos;
  BtkWidget *placeholder;

  /* RecentChooser properties */
  bint limit;  
  buint show_private : 1;
  buint show_not_found : 1;
  buint show_tips : 1;
  buint show_icons : 1;
  buint local_only : 1;
  
  buint show_numbers : 1;
  
  BtkRecentSortType sort_type;
  BtkRecentSortFunc sort_func;
  bpointer sort_data;
  GDestroyNotify sort_data_destroy;
  
  GSList *filters;
  BtkRecentFilter *current_filter;
 
  buint local_manager : 1;
  bulong manager_changed_id;

  bulong populate_id;
};

enum {
  PROP_0,
  PROP_SHOW_NUMBERS,

  /* activatable properties */
  PROP_ACTIVATABLE_RELATED_ACTION,
  PROP_ACTIVATABLE_USE_ACTION_APPEARANCE
};


#define FALLBACK_ICON_SIZE 	32
#define FALLBACK_ITEM_LIMIT 	10
#define DEFAULT_LABEL_WIDTH     30

#define BTK_RECENT_CHOOSER_MENU_GET_PRIVATE(obj)	(B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_RECENT_CHOOSER_MENU, BtkRecentChooserMenuPrivate))

static void     btk_recent_chooser_menu_finalize    (BObject                   *object);
static void     btk_recent_chooser_menu_dispose     (BObject                   *object);
static BObject *btk_recent_chooser_menu_constructor (GType                      type,
						     buint                      n_construct_properties,
						     BObjectConstructParam     *construct_params);

static void btk_recent_chooser_iface_init      (BtkRecentChooserIface     *iface);

static void btk_recent_chooser_menu_set_property (BObject      *object,
						  buint         prop_id,
						  const BValue *value,
						  BParamSpec   *pspec);
static void btk_recent_chooser_menu_get_property (BObject      *object,
						  buint         prop_id,
						  BValue       *value,
						  BParamSpec   *pspec);

static bboolean          btk_recent_chooser_menu_set_current_uri    (BtkRecentChooser  *chooser,
							             const bchar       *uri,
							             GError           **error);
static bchar *           btk_recent_chooser_menu_get_current_uri    (BtkRecentChooser  *chooser);
static bboolean          btk_recent_chooser_menu_select_uri         (BtkRecentChooser  *chooser,
								     const bchar       *uri,
								     GError           **error);
static void              btk_recent_chooser_menu_unselect_uri       (BtkRecentChooser  *chooser,
								     const bchar       *uri);
static void              btk_recent_chooser_menu_select_all         (BtkRecentChooser  *chooser);
static void              btk_recent_chooser_menu_unselect_all       (BtkRecentChooser  *chooser);
static GList *           btk_recent_chooser_menu_get_items          (BtkRecentChooser  *chooser);
static BtkRecentManager *btk_recent_chooser_menu_get_recent_manager (BtkRecentChooser  *chooser);
static void              btk_recent_chooser_menu_set_sort_func      (BtkRecentChooser  *chooser,
								     BtkRecentSortFunc  sort_func,
								     bpointer           sort_data,
								     GDestroyNotify     data_destroy);
static void              btk_recent_chooser_menu_add_filter         (BtkRecentChooser  *chooser,
								     BtkRecentFilter   *filter);
static void              btk_recent_chooser_menu_remove_filter      (BtkRecentChooser  *chooser,
								     BtkRecentFilter   *filter);
static GSList *          btk_recent_chooser_menu_list_filters       (BtkRecentChooser  *chooser);
static void              btk_recent_chooser_menu_set_current_filter (BtkRecentChooserMenu *menu,
								     BtkRecentFilter      *filter);

static void              btk_recent_chooser_menu_populate           (BtkRecentChooserMenu *menu);
static void              btk_recent_chooser_menu_set_show_tips      (BtkRecentChooserMenu *menu,
								     bboolean              show_tips);

static void     set_recent_manager (BtkRecentChooserMenu *menu,
				    BtkRecentManager     *manager);

static void     chooser_set_sort_type (BtkRecentChooserMenu *menu,
				       BtkRecentSortType     sort_type);

static bint     get_icon_size_for_widget (BtkWidget *widget);

static void     item_activate_cb   (BtkWidget        *widget,
			            bpointer          user_data);
static void     manager_changed_cb (BtkRecentManager *manager,
				    bpointer          user_data);

static void btk_recent_chooser_activatable_iface_init (BtkActivatableIface  *iface);
static void btk_recent_chooser_update                 (BtkActivatable       *activatable,
						       BtkAction            *action,
						       const bchar          *property_name);
static void btk_recent_chooser_sync_action_properties (BtkActivatable       *activatable,
						       BtkAction            *action);

G_DEFINE_TYPE_WITH_CODE (BtkRecentChooserMenu,
			 btk_recent_chooser_menu,
			 BTK_TYPE_MENU,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_RECENT_CHOOSER,
				 		btk_recent_chooser_iface_init)
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_ACTIVATABLE,
				 		btk_recent_chooser_activatable_iface_init))


static void
btk_recent_chooser_iface_init (BtkRecentChooserIface *iface)
{
  iface->set_current_uri = btk_recent_chooser_menu_set_current_uri;
  iface->get_current_uri = btk_recent_chooser_menu_get_current_uri;
  iface->select_uri = btk_recent_chooser_menu_select_uri;
  iface->unselect_uri = btk_recent_chooser_menu_unselect_uri;
  iface->select_all = btk_recent_chooser_menu_select_all;
  iface->unselect_all = btk_recent_chooser_menu_unselect_all;
  iface->get_items = btk_recent_chooser_menu_get_items;
  iface->get_recent_manager = btk_recent_chooser_menu_get_recent_manager;
  iface->set_sort_func = btk_recent_chooser_menu_set_sort_func;
  iface->add_filter = btk_recent_chooser_menu_add_filter;
  iface->remove_filter = btk_recent_chooser_menu_remove_filter;
  iface->list_filters = btk_recent_chooser_menu_list_filters;
}

static void
btk_recent_chooser_activatable_iface_init (BtkActivatableIface *iface)
{
  iface->update = btk_recent_chooser_update;
  iface->sync_action_properties = btk_recent_chooser_sync_action_properties;
}

static void
btk_recent_chooser_menu_class_init (BtkRecentChooserMenuClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);

  bobject_class->constructor = btk_recent_chooser_menu_constructor;
  bobject_class->dispose = btk_recent_chooser_menu_dispose;
  bobject_class->finalize = btk_recent_chooser_menu_finalize;
  bobject_class->set_property = btk_recent_chooser_menu_set_property;
  bobject_class->get_property = btk_recent_chooser_menu_get_property;

  _btk_recent_chooser_install_properties (bobject_class);

  /**
   * BtkRecentChooserMenu:show-numbers
   *
   * Whether the first ten items in the menu should be prepended by
   * a number acting as a unique mnemonic.
   *
   * Since: 2.10
   */
  g_object_class_install_property (bobject_class,
		  		   PROP_SHOW_NUMBERS,
				   g_param_spec_boolean ("show-numbers",
							 P_("Show Numbers"),
							 P_("Whether the items should be displayed with a number"),
							 FALSE,
							 BTK_PARAM_READWRITE));
  

  g_object_class_override_property (bobject_class, PROP_ACTIVATABLE_RELATED_ACTION, "related-action");
  g_object_class_override_property (bobject_class, PROP_ACTIVATABLE_USE_ACTION_APPEARANCE, "use-action-appearance");

  g_type_class_add_private (klass, sizeof (BtkRecentChooserMenuPrivate));
}

static void
btk_recent_chooser_menu_init (BtkRecentChooserMenu *menu)
{
  BtkRecentChooserMenuPrivate *priv;
  
  priv = BTK_RECENT_CHOOSER_MENU_GET_PRIVATE (menu);
  
  menu->priv = priv;
  
  priv->show_icons= TRUE;
  priv->show_numbers = FALSE;
  priv->show_tips = FALSE;
  priv->show_not_found = TRUE;
  priv->show_private = FALSE;
  priv->local_only = TRUE;
  
  priv->limit = FALLBACK_ITEM_LIMIT;
  priv->sort_type = BTK_RECENT_SORT_NONE;
  priv->icon_size = FALLBACK_ICON_SIZE;
  priv->label_width = DEFAULT_LABEL_WIDTH;
  
  priv->first_recent_item_pos = -1;
  priv->placeholder = NULL;

  priv->current_filter = NULL;
}

static void
btk_recent_chooser_menu_finalize (BObject *object)
{
  BtkRecentChooserMenu *menu = BTK_RECENT_CHOOSER_MENU (object);
  BtkRecentChooserMenuPrivate *priv = menu->priv;
  
  priv->manager = NULL;
  
  if (priv->sort_data_destroy)
    {
      priv->sort_data_destroy (priv->sort_data);
      priv->sort_data_destroy = NULL;
    }

  priv->sort_data = NULL;
  priv->sort_func = NULL;
  
  B_OBJECT_CLASS (btk_recent_chooser_menu_parent_class)->finalize (object);
}

static void
btk_recent_chooser_menu_dispose (BObject *object)
{
  BtkRecentChooserMenu *menu = BTK_RECENT_CHOOSER_MENU (object);
  BtkRecentChooserMenuPrivate *priv = menu->priv;

  if (priv->manager_changed_id)
    {
      if (priv->manager)
        g_signal_handler_disconnect (priv->manager, priv->manager_changed_id);

      priv->manager_changed_id = 0;
    }
  
  if (priv->populate_id)
    {
      g_source_remove (priv->populate_id);
      priv->populate_id = 0;
    }

  if (priv->current_filter)
    {
      g_object_unref (priv->current_filter);
      priv->current_filter = NULL;
    }
  
  B_OBJECT_CLASS (btk_recent_chooser_menu_parent_class)->dispose (object);
}

static BObject *
btk_recent_chooser_menu_constructor (GType                  type,
				     buint                  n_params,
				     BObjectConstructParam *params)
{
  BtkRecentChooserMenu *menu;
  BtkRecentChooserMenuPrivate *priv;
  BObjectClass *parent_class;
  BObject *object;
  
  parent_class = B_OBJECT_CLASS (btk_recent_chooser_menu_parent_class);
  object = parent_class->constructor (type, n_params, params);
  menu = BTK_RECENT_CHOOSER_MENU (object);
  priv = menu->priv;
  
  g_assert (priv->manager);

  /* we create a placeholder menuitem, to be used in case
   * the menu is empty. this placeholder will stay around
   * for the entire lifetime of the menu, and we just hide it
   * when it's not used. we have to do this, and do it here,
   * because we need a marker for the beginning of the recent
   * items list, so that we can insert the new items at the
   * right place when idly populating the menu in case the
   * user appended or prepended custom menu items to the
   * recent chooser menu widget.
   */
  priv->placeholder = btk_menu_item_new_with_label (_("No items found"));
  btk_widget_set_sensitive (priv->placeholder, FALSE);
  g_object_set_data (B_OBJECT (priv->placeholder),
                     "btk-recent-menu-placeholder",
                     BINT_TO_POINTER (TRUE));

  btk_menu_shell_insert (BTK_MENU_SHELL (menu), priv->placeholder, 0);
  btk_widget_set_no_show_all (priv->placeholder, TRUE);
  btk_widget_show (priv->placeholder);

  /* (re)populate the menu */
  btk_recent_chooser_menu_populate (menu);

  return object;
}

static void
btk_recent_chooser_menu_set_property (BObject      *object,
				      buint         prop_id,
				      const BValue *value,
				      BParamSpec   *pspec)
{
  BtkRecentChooserMenu *menu = BTK_RECENT_CHOOSER_MENU (object);
  BtkRecentChooserMenuPrivate *priv = menu->priv;
  
  switch (prop_id)
    {
    case PROP_SHOW_NUMBERS:
      priv->show_numbers = b_value_get_boolean (value);
      break;
    case BTK_RECENT_CHOOSER_PROP_RECENT_MANAGER:
      set_recent_manager (menu, b_value_get_object (value));
      break;
    case BTK_RECENT_CHOOSER_PROP_SHOW_PRIVATE:
      priv->show_private = b_value_get_boolean (value);
      break;
    case BTK_RECENT_CHOOSER_PROP_SHOW_NOT_FOUND:
      priv->show_not_found = b_value_get_boolean (value);
      break;
    case BTK_RECENT_CHOOSER_PROP_SHOW_TIPS:
      btk_recent_chooser_menu_set_show_tips (menu, b_value_get_boolean (value));
      break;
    case BTK_RECENT_CHOOSER_PROP_SHOW_ICONS:
      priv->show_icons = b_value_get_boolean (value);
      break;
    case BTK_RECENT_CHOOSER_PROP_SELECT_MULTIPLE:
      g_warning ("%s: Choosers of type `%s' do not support selecting multiple items.",
                 B_STRFUNC,
                 B_OBJECT_TYPE_NAME (object));
      break;
    case BTK_RECENT_CHOOSER_PROP_LOCAL_ONLY:
      priv->local_only = b_value_get_boolean (value);
      break;
    case BTK_RECENT_CHOOSER_PROP_LIMIT:
      priv->limit = b_value_get_int (value);
      break;
    case BTK_RECENT_CHOOSER_PROP_SORT_TYPE:
      chooser_set_sort_type (menu, b_value_get_enum (value));
      break;
    case BTK_RECENT_CHOOSER_PROP_FILTER:
      btk_recent_chooser_menu_set_current_filter (menu, b_value_get_object (value));
      break;
    case PROP_ACTIVATABLE_RELATED_ACTION:
      _btk_recent_chooser_set_related_action (BTK_RECENT_CHOOSER (menu), b_value_get_object (value));
      break;
    case PROP_ACTIVATABLE_USE_ACTION_APPEARANCE: 
      _btk_recent_chooser_set_use_action_appearance (BTK_RECENT_CHOOSER (menu), b_value_get_boolean (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_recent_chooser_menu_get_property (BObject    *object,
				      buint       prop_id,
				      BValue     *value,
				      BParamSpec *pspec)
{
  BtkRecentChooserMenu *menu = BTK_RECENT_CHOOSER_MENU (object);
  BtkRecentChooserMenuPrivate *priv = menu->priv;
  
  switch (prop_id)
    {
    case PROP_SHOW_NUMBERS:
      b_value_set_boolean (value, priv->show_numbers);
      break;
    case BTK_RECENT_CHOOSER_PROP_SHOW_TIPS:
      b_value_set_boolean (value, priv->show_tips);
      break;
    case BTK_RECENT_CHOOSER_PROP_LIMIT:
      b_value_set_int (value, priv->limit);
      break;
    case BTK_RECENT_CHOOSER_PROP_LOCAL_ONLY:
      b_value_set_boolean (value, priv->local_only);
      break;
    case BTK_RECENT_CHOOSER_PROP_SORT_TYPE:
      b_value_set_enum (value, priv->sort_type);
      break;
    case BTK_RECENT_CHOOSER_PROP_SHOW_PRIVATE:
      b_value_set_boolean (value, priv->show_private);
      break;
    case BTK_RECENT_CHOOSER_PROP_SHOW_NOT_FOUND:
      b_value_set_boolean (value, priv->show_not_found);
      break;
    case BTK_RECENT_CHOOSER_PROP_SHOW_ICONS:
      b_value_set_boolean (value, priv->show_icons);
      break;
    case BTK_RECENT_CHOOSER_PROP_SELECT_MULTIPLE:
      b_value_set_boolean (value, FALSE);
      break;
    case BTK_RECENT_CHOOSER_PROP_FILTER:
      b_value_set_object (value, priv->current_filter);
      break;
    case PROP_ACTIVATABLE_RELATED_ACTION:
      b_value_set_object (value, _btk_recent_chooser_get_related_action (BTK_RECENT_CHOOSER (menu)));
      break;
    case PROP_ACTIVATABLE_USE_ACTION_APPEARANCE: 
      b_value_set_boolean (value, _btk_recent_chooser_get_use_action_appearance (BTK_RECENT_CHOOSER (menu)));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static bboolean
btk_recent_chooser_menu_set_current_uri (BtkRecentChooser  *chooser,
					 const bchar       *uri,
					 GError           **error)
{
  BtkRecentChooserMenu *menu = BTK_RECENT_CHOOSER_MENU (chooser);
  GList *children, *l;
  BtkWidget *menu_item = NULL;
  bboolean found = FALSE;
  
  children = btk_container_get_children (BTK_CONTAINER (menu));
  
  for (l = children; l != NULL; l = l->next)
    {
      BtkRecentInfo *info;
      
      menu_item = BTK_WIDGET (l->data);
      
      info = g_object_get_data (B_OBJECT (menu_item), "btk-recent-info");
      if (!info)
        continue;
      
      if (strcmp (uri, btk_recent_info_get_uri (info)) == 0)
        {
          btk_menu_shell_activate_item (BTK_MENU_SHELL (menu),
	                                menu_item,
					TRUE);
	  found = TRUE;

	  break;
	}
    }

  g_list_free (children);
  
  if (!found)  
    {
      g_set_error (error, BTK_RECENT_CHOOSER_ERROR,
      		   BTK_RECENT_CHOOSER_ERROR_NOT_FOUND,
      		   _("No recently used resource found with URI `%s'"),
      		   uri);
    }
  
  return found;
}

static bchar *
btk_recent_chooser_menu_get_current_uri (BtkRecentChooser  *chooser)
{
  BtkRecentChooserMenu *menu = BTK_RECENT_CHOOSER_MENU (chooser);
  BtkWidget *menu_item;
  BtkRecentInfo *info;
  
  menu_item = btk_menu_get_active (BTK_MENU (menu));
  if (!menu_item)
    return NULL;
  
  info = g_object_get_data (B_OBJECT (menu_item), "btk-recent-info");
  if (!info)
    return NULL;
  
  return g_strdup (btk_recent_info_get_uri (info));
}

static bboolean
btk_recent_chooser_menu_select_uri (BtkRecentChooser  *chooser,
				    const bchar       *uri,
				    GError           **error)
{
  BtkRecentChooserMenu *menu = BTK_RECENT_CHOOSER_MENU (chooser);
  GList *children, *l;
  BtkWidget *menu_item = NULL;
  bboolean found = FALSE;
  
  children = btk_container_get_children (BTK_CONTAINER (menu));
  for (l = children; l != NULL; l = l->next)
    {
      BtkRecentInfo *info;
      
      menu_item = BTK_WIDGET (l->data);
      
      info = g_object_get_data (B_OBJECT (menu_item), "btk-recent-info");
      if (!info)
        continue;
      
      if (0 == strcmp (uri, btk_recent_info_get_uri (info)))
        found = TRUE;
    }

  g_list_free (children);
  
  if (!found)  
    {
      g_set_error (error, BTK_RECENT_CHOOSER_ERROR,
      		   BTK_RECENT_CHOOSER_ERROR_NOT_FOUND,
      		   _("No recently used resource found with URI `%s'"),
      		   uri);
      return FALSE;
    }
  else
    {
      btk_menu_shell_select_item (BTK_MENU_SHELL (menu), menu_item);

      return TRUE;
    }
}

static void
btk_recent_chooser_menu_unselect_uri (BtkRecentChooser *chooser,
				       const bchar     *uri)
{
  BtkRecentChooserMenu *menu = BTK_RECENT_CHOOSER_MENU (chooser);
  
  btk_menu_shell_deselect (BTK_MENU_SHELL (menu));
}

static void
btk_recent_chooser_menu_select_all (BtkRecentChooser *chooser)
{
  g_warning (_("This function is not implemented for "
               "widgets of class '%s'"),
             g_type_name (B_OBJECT_TYPE (chooser)));
}

static void
btk_recent_chooser_menu_unselect_all (BtkRecentChooser *chooser)
{
  g_warning (_("This function is not implemented for "
               "widgets of class '%s'"),
             g_type_name (B_OBJECT_TYPE (chooser)));
}

static void
btk_recent_chooser_menu_set_sort_func (BtkRecentChooser  *chooser,
				       BtkRecentSortFunc  sort_func,
				       bpointer           sort_data,
				       GDestroyNotify     data_destroy)
{
  BtkRecentChooserMenu *menu = BTK_RECENT_CHOOSER_MENU (chooser);
  BtkRecentChooserMenuPrivate *priv = menu->priv;
  
  if (priv->sort_data_destroy)
    {
      priv->sort_data_destroy (priv->sort_data);
      priv->sort_data_destroy = NULL;
    }
      
  priv->sort_func = NULL;
  priv->sort_data = NULL;
  priv->sort_data_destroy = NULL;
  
  if (sort_func)
    {
      priv->sort_func = sort_func;
      priv->sort_data = sort_data;
      priv->sort_data_destroy = data_destroy;
    }
}

static void
chooser_set_sort_type (BtkRecentChooserMenu *menu,
		       BtkRecentSortType     sort_type)
{
  if (menu->priv->sort_type == sort_type)
    return;

  menu->priv->sort_type = sort_type;
}


static GList *
btk_recent_chooser_menu_get_items (BtkRecentChooser *chooser)
{
  BtkRecentChooserMenu *menu = BTK_RECENT_CHOOSER_MENU (chooser);
  BtkRecentChooserMenuPrivate *priv = menu->priv;

  return _btk_recent_chooser_get_items (chooser,
                                        priv->current_filter,
                                        priv->sort_func,
                                        priv->sort_data);
}

static BtkRecentManager *
btk_recent_chooser_menu_get_recent_manager (BtkRecentChooser *chooser)
{
  BtkRecentChooserMenuPrivate *priv;
 
  priv = BTK_RECENT_CHOOSER_MENU (chooser)->priv;
  
  return priv->manager;
}

static void
btk_recent_chooser_menu_add_filter (BtkRecentChooser *chooser,
				    BtkRecentFilter  *filter)
{
  BtkRecentChooserMenu *menu;

  menu = BTK_RECENT_CHOOSER_MENU (chooser);
  
  btk_recent_chooser_menu_set_current_filter (menu, filter);
}

static void
btk_recent_chooser_menu_remove_filter (BtkRecentChooser *chooser,
				       BtkRecentFilter  *filter)
{
  BtkRecentChooserMenu *menu;

  menu = BTK_RECENT_CHOOSER_MENU (chooser);
  
  if (filter == menu->priv->current_filter)
    {
      g_object_unref (menu->priv->current_filter);
      menu->priv->current_filter = NULL;

      g_object_notify (B_OBJECT (menu), "filter");
    }
}

static GSList *
btk_recent_chooser_menu_list_filters (BtkRecentChooser *chooser)
{
  BtkRecentChooserMenu *menu;
  GSList *retval = NULL;

  menu = BTK_RECENT_CHOOSER_MENU (chooser);
 
  if (menu->priv->current_filter)
    retval = b_slist_prepend (retval, menu->priv->current_filter);

  return retval;
}

static void
btk_recent_chooser_menu_set_current_filter (BtkRecentChooserMenu *menu,
					    BtkRecentFilter      *filter)
{
  BtkRecentChooserMenuPrivate *priv;

  priv = menu->priv;
  
  if (priv->current_filter)
    g_object_unref (B_OBJECT (priv->current_filter));
  
  priv->current_filter = filter;

  if (priv->current_filter)
    g_object_ref_sink (priv->current_filter);

  btk_recent_chooser_menu_populate (menu);
  
  g_object_notify (B_OBJECT (menu), "filter");
}

/* taken from libeel/eel-strings.c */
static bchar *
escape_underscores (const bchar *string)
{
  bint underscores;
  const bchar *p;
  bchar *q;
  bchar *escaped;

  if (!string)
    return NULL;
	
  underscores = 0;
  for (p = string; *p != '\0'; p++)
    underscores += (*p == '_');

  if (underscores == 0)
    return g_strdup (string);

  escaped = g_new (char, strlen (string) + underscores + 1);
  for (p = string, q = escaped; *p != '\0'; p++, q++)
    {
      /* Add an extra underscore. */
      if (*p == '_')
        *q++ = '_';
      
      *q = *p;
    }
  
  *q = '\0';
	
  return escaped;
}

static void
btk_recent_chooser_menu_add_tip (BtkRecentChooserMenu *menu,
				 BtkRecentInfo        *info,
				 BtkWidget            *item)
{
  BtkRecentChooserMenuPrivate *priv;
  bchar *path;

  g_assert (info != NULL);
  g_assert (item != NULL);

  priv = menu->priv;
  
  path = btk_recent_info_get_uri_display (info);
  if (path)
    {
      bchar *tip_text = g_strdup_printf (_("Open '%s'"), path);

      btk_widget_set_tooltip_text (item, tip_text);
      btk_widget_set_has_tooltip (item, priv->show_tips);

      g_free (path);
      g_free (tip_text);
    }
}

static BtkWidget *
btk_recent_chooser_menu_create_item (BtkRecentChooserMenu *menu,
				     BtkRecentInfo        *info,
				     bint                  count)
{
  BtkRecentChooserMenuPrivate *priv;
  bchar *text;
  BtkWidget *item, *image, *label;
  BdkPixbuf *icon;

  g_assert (info != NULL);

  priv = menu->priv;

  if (priv->show_numbers)
    {
      bchar *name, *escaped;
      
      name = g_strdup (btk_recent_info_get_display_name (info));
      if (!name)
        name = g_strdup (_("Unknown item"));
      
      escaped = escape_underscores (name);
      
      /* avoid clashing mnemonics */
      if (count <= 10)
        /* This is the label format that is used for the first 10 items
         * in a recent files menu. The %d is the number of the item,
         * the %s is the name of the item. Please keep the _ in front
         * of the number to give these menu items a mnemonic.
         */
        text = g_strdup_printf (C_("recent menu label", "_%d. %s"), count, escaped);
      else
        /* This is the format that is used for items in a recent files menu.
         * The %d is the number of the item, the %s is the name of the item.
         */
        text = g_strdup_printf (C_("recent menu label", "%d. %s"), count, escaped);
      
      item = btk_image_menu_item_new_with_mnemonic (text);
      
      g_free (escaped);
      g_free (name);
    }
  else
    {
      text = g_strdup (btk_recent_info_get_display_name (info));
      item = btk_image_menu_item_new_with_label (text);
    }

  g_free (text);

  btk_image_menu_item_set_always_show_image (BTK_IMAGE_MENU_ITEM (item),
                                             TRUE);

  /* ellipsize the menu item label, in case the recent document
   * display name is huge.
   */
  label = BTK_BIN (item)->child;
  if (BTK_IS_LABEL (label))
    {
      btk_label_set_ellipsize (BTK_LABEL (label), BANGO_ELLIPSIZE_END);
      btk_label_set_max_width_chars (BTK_LABEL (label), priv->label_width);
    }
  
  if (priv->show_icons)
    {
      icon = btk_recent_info_get_icon (info, priv->icon_size);
        
      image = btk_image_new_from_pixbuf (icon);
      btk_image_menu_item_set_image (BTK_IMAGE_MENU_ITEM (item), image);
      g_object_unref (icon);
    }

  g_signal_connect (item, "activate",
  		    G_CALLBACK (item_activate_cb),
  		    menu);

  return item;
}

static void
btk_recent_chooser_menu_insert_item (BtkRecentChooserMenu *menu,
                                     BtkWidget            *menuitem,
                                     bint                  position)
{
  BtkRecentChooserMenuPrivate *priv = menu->priv;
  bint real_position;

  if (priv->first_recent_item_pos == -1)
    {
      GList *children, *l;

      children = btk_container_get_children (BTK_CONTAINER (menu));

      for (real_position = 0, l = children;
           l != NULL;
           real_position += 1, l = l->next)
        {
          BObject *child = l->data;
          bboolean is_placeholder = FALSE;

          is_placeholder =
            BPOINTER_TO_INT (g_object_get_data (child, "btk-recent-menu-placeholder"));

          if (is_placeholder)
            break;
        }

      g_list_free (children);
      priv->first_recent_item_pos = real_position;
    }
  else
    real_position = priv->first_recent_item_pos;

  btk_menu_shell_insert (BTK_MENU_SHELL (menu), menuitem,
                         real_position + position);
  btk_widget_show (menuitem);
}

/* removes the items we own from the menu */
static void
btk_recent_chooser_menu_dispose_items (BtkRecentChooserMenu *menu)
{
  GList *children, *l;
 
  children = btk_container_get_children (BTK_CONTAINER (menu));
  for (l = children; l != NULL; l = l->next)
    {
      BtkWidget *menu_item = BTK_WIDGET (l->data);
      bboolean has_mark = FALSE;
      
      /* check for our mark, in order to remove just the items we own */
      has_mark =
        BPOINTER_TO_INT (g_object_get_data (B_OBJECT (menu_item), "btk-recent-menu-mark"));

      if (has_mark)
        {
          BtkRecentInfo *info;
          
          /* destroy the attached RecentInfo struct, if found */
          info = g_object_get_data (B_OBJECT (menu_item), "btk-recent-info");
          if (info)
            g_object_set_data_full (B_OBJECT (menu_item), "btk-recent-info",
            			    NULL, NULL);
          
          /* and finally remove the item from the menu */
          btk_container_remove (BTK_CONTAINER (menu), menu_item);
        }
    }

  /* recalculate the position of the first recent item */
  menu->priv->first_recent_item_pos = -1;

  g_list_free (children);
}

typedef struct
{
  GList *items;
  bint n_items;
  bint loaded_items;
  bint displayed_items;
  BtkRecentChooserMenu *menu;
  BtkWidget *placeholder;
} MenuPopulateData;

static bboolean
idle_populate_func (bpointer data)
{
  MenuPopulateData *pdata;
  BtkRecentChooserMenuPrivate *priv;
  BtkRecentInfo *info;
  bboolean retval;
  BtkWidget *item;

  pdata = (MenuPopulateData *) data;
  priv = pdata->menu->priv;

  if (!pdata->items)
    {
      pdata->items = btk_recent_chooser_get_items (BTK_RECENT_CHOOSER (pdata->menu));
      if (!pdata->items)
        {
          /* show the placeholder here */
          btk_widget_show (pdata->placeholder);
          pdata->displayed_items = 1;
          priv->populate_id = 0;

	  return FALSE;
	}
      else
        btk_widget_hide (pdata->placeholder);
      
      pdata->n_items = g_list_length (pdata->items);
      pdata->loaded_items = 0;
    }

  info = g_list_nth_data (pdata->items, pdata->loaded_items);
  item = btk_recent_chooser_menu_create_item (pdata->menu,
                                              info,
					      pdata->displayed_items);
  if (!item)
    goto check_and_return;
      
  btk_recent_chooser_menu_add_tip (pdata->menu, info, item);
  btk_recent_chooser_menu_insert_item (pdata->menu, item,
                                       pdata->displayed_items);
  
  pdata->displayed_items += 1;
      
  /* mark the menu item as one of our own */
  g_object_set_data (B_OBJECT (item),
                     "btk-recent-menu-mark",
      		     BINT_TO_POINTER (TRUE));
      
  /* attach the RecentInfo object to the menu item, and own a reference
   * to it, so that it will be destroyed with the menu item when it's
   * not needed anymore.
   */
  g_object_set_data_full (B_OBJECT (item), "btk-recent-info",
      			  btk_recent_info_ref (info),
      			  (GDestroyNotify) btk_recent_info_unref);
  
check_and_return:
  pdata->loaded_items += 1;

  if (pdata->loaded_items == pdata->n_items)
    {
      g_list_foreach (pdata->items, (GFunc) btk_recent_info_unref, NULL);
      g_list_free (pdata->items);

      priv->populate_id = 0;

      retval = FALSE;
    }
  else
    retval = TRUE;

  return retval;
}

static void
idle_populate_clean_up (bpointer data)
{
  MenuPopulateData *pdata = data;

  if (pdata->menu->priv->populate_id == 0)
    {
      /* show the placeholder in case no item survived
       * the filtering process in the idle loop
       */
      if (!pdata->displayed_items)
        btk_widget_show (pdata->placeholder);
      g_object_unref (pdata->placeholder);

      g_slice_free (MenuPopulateData, data);
    }
}

static void
btk_recent_chooser_menu_populate (BtkRecentChooserMenu *menu)
{
  MenuPopulateData *pdata;
  BtkRecentChooserMenuPrivate *priv = menu->priv;

  if (menu->priv->populate_id)
    return;

  pdata = g_slice_new (MenuPopulateData);
  pdata->items = NULL;
  pdata->n_items = 0;
  pdata->loaded_items = 0;
  pdata->displayed_items = 0;
  pdata->menu = menu;
  pdata->placeholder = g_object_ref (priv->placeholder);

  priv->icon_size = get_icon_size_for_widget (BTK_WIDGET (menu));
  
  /* remove our menu items first */
  btk_recent_chooser_menu_dispose_items (menu);
  
  priv->populate_id = bdk_threads_add_idle_full (G_PRIORITY_HIGH_IDLE + 30,
  					         idle_populate_func,
					         pdata,
                                                 idle_populate_clean_up);
}

/* bounce activate signal from the recent menu item widget 
 * to the recent menu widget
 */
static void
item_activate_cb (BtkWidget *widget,
		  bpointer   user_data)
{
  BtkRecentChooser *chooser = BTK_RECENT_CHOOSER (user_data);
  
  _btk_recent_chooser_item_activated (chooser);
}

/* we force a redraw if the manager changes when we are showing */
static void
manager_changed_cb (BtkRecentManager *manager,
		    bpointer          user_data)
{
  BtkRecentChooserMenu *menu = BTK_RECENT_CHOOSER_MENU (user_data);

  btk_recent_chooser_menu_populate (menu);
}

static void
set_recent_manager (BtkRecentChooserMenu *menu,
		    BtkRecentManager     *manager)
{
  BtkRecentChooserMenuPrivate *priv = menu->priv;

  if (priv->manager)
    {
      if (priv->manager_changed_id)
        {
          g_signal_handler_disconnect (priv->manager, priv->manager_changed_id);
          priv->manager_changed_id = 0;
        }

      if (priv->populate_id)
        {
          g_source_remove (priv->populate_id);
          priv->populate_id = 0;
        }

      priv->manager = NULL;
    }
  
  if (manager)
    priv->manager = manager;
  else
    priv->manager = btk_recent_manager_get_default ();
  
  if (priv->manager)
    priv->manager_changed_id = g_signal_connect (priv->manager, "changed",
                                                 G_CALLBACK (manager_changed_cb),
                                                 menu);
}

static bint
get_icon_size_for_widget (BtkWidget *widget)
{
  BtkSettings *settings;
  bint width, height;

  if (btk_widget_has_screen (widget))
    settings = btk_settings_get_for_screen (btk_widget_get_screen (widget));
  else
    settings = btk_settings_get_default ();

  if (btk_icon_size_lookup_for_settings (settings, BTK_ICON_SIZE_MENU,
                                         &width, &height))
    return MAX (width, height);

  return FALLBACK_ICON_SIZE;
}

static void
foreach_set_shot_tips (BtkWidget *widget,
                       bpointer   user_data)
{
  BtkRecentChooserMenu *menu = user_data;
  BtkRecentChooserMenuPrivate *priv = menu->priv;
  bboolean has_mark;

  /* toggle the tooltip only on the items we create */
  has_mark =
    BPOINTER_TO_INT (g_object_get_data (B_OBJECT (widget), "btk-recent-menu-mark"));

  if (has_mark)
    btk_widget_set_has_tooltip (widget, priv->show_tips);
}

static void
btk_recent_chooser_menu_set_show_tips (BtkRecentChooserMenu *menu,
				       bboolean              show_tips)
{
  BtkRecentChooserMenuPrivate *priv = menu->priv;

  if (priv->show_tips == show_tips)
    return;
  
  priv->show_tips = show_tips;
  btk_container_foreach (BTK_CONTAINER (menu), foreach_set_shot_tips, menu);
}

static void
btk_recent_chooser_update (BtkActivatable *activatable,
			   BtkAction      *action,
			   const bchar    *property_name)
{
  if (strcmp (property_name, "sensitive") == 0)
    btk_widget_set_sensitive (BTK_WIDGET (activatable), btk_action_is_sensitive (action));

  _btk_recent_chooser_update (activatable, action, property_name);
}

static void
btk_recent_chooser_sync_action_properties (BtkActivatable *activatable,
				           BtkAction      *action)
{
  btk_widget_set_sensitive (BTK_WIDGET (activatable), btk_action_is_sensitive (action));

  _btk_recent_chooser_sync_action_properties (activatable, action);
}


/*
 * Public API
 */

/**
 * btk_recent_chooser_menu_new:
 *
 * Creates a new #BtkRecentChooserMenu widget.
 *
 * This kind of widget shows the list of recently used resources as
 * a menu, each item as a menu item.  Each item inside the menu might
 * have an icon, representing its MIME type, and a number, for mnemonic
 * access.
 *
 * This widget implements the #BtkRecentChooser interface.
 *
 * This widget creates its own #BtkRecentManager object.  See the
 * btk_recent_chooser_menu_new_for_manager() function to know how to create
 * a #BtkRecentChooserMenu widget bound to another #BtkRecentManager object.
 *
 * Return value: a new #BtkRecentChooserMenu
 *
 * Since: 2.10
 */
BtkWidget *
btk_recent_chooser_menu_new (void)
{
  return g_object_new (BTK_TYPE_RECENT_CHOOSER_MENU,
  		       "recent-manager", NULL,
  		       NULL);
}

/**
 * btk_recent_chooser_menu_new_for_manager:
 * @manager: a #BtkRecentManager
 *
 * Creates a new #BtkRecentChooserMenu widget using @manager as
 * the underlying recently used resources manager.
 *
 * This is useful if you have implemented your own recent manager,
 * or if you have a customized instance of a #BtkRecentManager
 * object or if you wish to share a common #BtkRecentManager object
 * among multiple #BtkRecentChooser widgets.
 *
 * Return value: a new #BtkRecentChooserMenu, bound to @manager.
 *
 * Since: 2.10
 */
BtkWidget *
btk_recent_chooser_menu_new_for_manager (BtkRecentManager *manager)
{
  g_return_val_if_fail (manager == NULL || BTK_IS_RECENT_MANAGER (manager), NULL);
  
  return g_object_new (BTK_TYPE_RECENT_CHOOSER_MENU,
  		       "recent-manager", manager,
  		       NULL);
}

/**
 * btk_recent_chooser_menu_get_show_numbers:
 * @menu: a #BtkRecentChooserMenu
 *
 * Returns the value set by btk_recent_chooser_menu_set_show_numbers().
 * 
 * Return value: %TRUE if numbers should be shown.
 *
 * Since: 2.10
 */
bboolean
btk_recent_chooser_menu_get_show_numbers (BtkRecentChooserMenu *menu)
{
  g_return_val_if_fail (BTK_IS_RECENT_CHOOSER_MENU (menu), FALSE);

  return menu->priv->show_numbers;
}

/**
 * btk_recent_chooser_menu_set_show_numbers:
 * @menu: a #BtkRecentChooserMenu
 * @show_numbers: whether to show numbers
 *
 * Sets whether a number should be added to the items of @menu.  The
 * numbers are shown to provide a unique character for a mnemonic to
 * be used inside ten menu item's label.  Only the first the items
 * get a number to avoid clashes.
 *
 * Since: 2.10
 */
void
btk_recent_chooser_menu_set_show_numbers (BtkRecentChooserMenu *menu,
					  bboolean              show_numbers)
{
  g_return_if_fail (BTK_IS_RECENT_CHOOSER_MENU (menu));

  if (menu->priv->show_numbers == show_numbers)
    return;

  menu->priv->show_numbers = show_numbers;
  g_object_notify (B_OBJECT (menu), "show-numbers");
}

#define __BTK_RECENT_CHOOSER_MENU_C__
#include "btkaliasdef.c"
