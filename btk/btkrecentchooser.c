/* BTK - The GIMP Toolkit
 * btkrecentchooser.c - Abstract interface for recent file selectors GUIs
 *
 * Copyright (C) 2006, Emmanuele Bassi
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

#include "btkrecentchooser.h"
#include "btkrecentchooserprivate.h"
#include "btkrecentmanager.h"
#include "btkrecentaction.h"
#include "btkactivatable.h"
#include "btkintl.h"
#include "btktypebuiltins.h"
#include "btkprivate.h"
#include "btkmarshalers.h"
#include "btkalias.h"


enum
{
  ITEM_ACTIVATED,
  SELECTION_CHANGED,

  LAST_SIGNAL
};

static void     btk_recent_chooser_class_init         (bpointer          g_iface);
static bboolean recent_chooser_has_show_numbers       (BtkRecentChooser *chooser);

static GQuark      quark_btk_related_action               = 0;
static GQuark      quark_btk_use_action_appearance        = 0;
static const bchar btk_related_action_key[]               = "btk-related-action";
static const bchar btk_use_action_appearance_key[]        = "btk-use-action-appearance";


static buint chooser_signals[LAST_SIGNAL] = { 0, };

GType
btk_recent_chooser_get_type (void)
{
  static GType chooser_type = 0;
  
  if (!chooser_type)
    {
      chooser_type = g_type_register_static_simple (B_TYPE_INTERFACE,
						    I_("BtkRecentChooser"),
						    sizeof (BtkRecentChooserIface),
						    (GClassInitFunc) btk_recent_chooser_class_init,
						    0, NULL, 0);
      
      g_type_interface_add_prerequisite (chooser_type, B_TYPE_OBJECT);
    }
  
  return chooser_type;
}

static void
btk_recent_chooser_class_init (bpointer g_iface)
{
  GType iface_type = B_TYPE_FROM_INTERFACE (g_iface);

  quark_btk_related_action        = g_quark_from_static_string (btk_related_action_key);
  quark_btk_use_action_appearance = g_quark_from_static_string (btk_use_action_appearance_key);
  
  /**
   * BtkRecentChooser::selection-changed
   * @chooser: the object which received the signal
   *
   * This signal is emitted when there is a change in the set of
   * selected recently used resources.  This can happen when a user
   * modifies the selection with the mouse or the keyboard, or when
   * explicitely calling functions to change the selection.
   *
   * Since: 2.10
   */
  chooser_signals[SELECTION_CHANGED] =
    g_signal_new (I_("selection-changed"),
                  iface_type,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkRecentChooserIface, selection_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  B_TYPE_NONE, 0);
   
  /**
   * BtkRecentChooser::item-activated
   * @chooser: the object which received the signal
   *
   * This signal is emitted when the user "activates" a recent item
   * in the recent chooser.  This can happen by double-clicking on an item
   * in the recently used resources list, or by pressing
   * <keycap>Enter</keycap>.
   *
   * Since: 2.10
   */
  chooser_signals[ITEM_ACTIVATED] =
    g_signal_new (I_("item-activated"),
                  iface_type,
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkRecentChooserIface, item_activated),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);

  /**
   * BtkRecentChooser:recent-manager:
   *
   * The #BtkRecentManager instance used by the #BtkRecentChooser to
   * display the list of recently used resources.
   *
   * Since: 2.10
   */
  g_object_interface_install_property (g_iface,
  				       g_param_spec_object ("recent-manager",
  				       			    P_("Recent Manager"),
  				       			    P_("The RecentManager object to use"),
  				       			    BTK_TYPE_RECENT_MANAGER,
  				       			    BTK_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
  /**
   * BtkRecentManager:show-private:
   *
   * Whether this #BtkRecentChooser should display recently used resources
   * marked with the "private" flag. Such resources should be considered
   * private to the applications and groups that have added them.
   *
   * Since: 2.10
   */
  g_object_interface_install_property (g_iface,
   				       g_param_spec_boolean ("show-private",
   							     P_("Show Private"),
   							     P_("Whether the private items should be displayed"),
   							     FALSE,
   							     BTK_PARAM_READWRITE));
  /**
   * BtkRecentChooser:show-tips:
   *
   * Whether this #BtkRecentChooser should display a tooltip containing the
   * full path of the recently used resources.
   *
   * Since: 2.10
   */
  g_object_interface_install_property (g_iface,
  				       g_param_spec_boolean ("show-tips",
  				       			     P_("Show Tooltips"),
  				       			     P_("Whether there should be a tooltip on the item"),
  				       			     FALSE,
  				       			     BTK_PARAM_READWRITE));
  /**
   * BtkRecentChooser:show-icons:
   *
   * Whether this #BtkRecentChooser should display an icon near the item.
   *
   * Since: 2.10
   */
  g_object_interface_install_property (g_iface,
  				       g_param_spec_boolean ("show-icons",
  				       			     P_("Show Icons"),
  				       			     P_("Whether there should be an icon near the item"),
  				       			     TRUE,
  				       			     BTK_PARAM_READWRITE));
  /**
   * BtkRecentChooser:show-not-found:
   *
   * Whether this #BtkRecentChooser should display the recently used resources
   * even if not present anymore. Setting this to %FALSE will perform a
   * potentially expensive check on every local resource (every remote
   * resource will always be displayed).
   *
   * Since: 2.10
   */
  g_object_interface_install_property (g_iface,
  				       g_param_spec_boolean ("show-not-found",
  				       			     P_("Show Not Found"),
  				       			     P_("Whether the items pointing to unavailable resources should be displayed"),
  				       			     TRUE,
  				       			     BTK_PARAM_READWRITE));
  /**
   * BtkRecentChooser:select-multiple:
   *
   * Allow the user to select multiple resources.
   *
   * Since: 2.10
   */
  g_object_interface_install_property (g_iface,
   				       g_param_spec_boolean ("select-multiple",
   							     P_("Select Multiple"),
   							     P_("Whether to allow multiple items to be selected"),
   							     FALSE,
   							     BTK_PARAM_READWRITE));
  /**
   * BtkRecentChooser:local-only:
   *
   * Whether this #BtkRecentChooser should display only local (file:)
   * resources.
   *
   * Since: 2.10
   */
  g_object_interface_install_property (g_iface,
		  		       g_param_spec_boolean ("local-only",
					       		     P_("Local only"),
							     P_("Whether the selected resource(s) should be limited to local file: URIs"),
							     TRUE,
							     BTK_PARAM_READWRITE));
  /**
   * BtkRecentChooser:limit:
   *
   * The maximum number of recently used resources to be displayed,
   * or -1 to display all items. By default, the
   * BtkSetting:btk-recent-files-limit setting is respected: you can
   * override that limit on a particular instance of #BtkRecentChooser
   * by setting this property.
   *
   * Since: 2.10
   */
  g_object_interface_install_property (g_iface,
   				       g_param_spec_int ("limit",
   							 P_("Limit"),
   							 P_("The maximum number of items to be displayed"),
   							 -1,
   							 B_MAXINT,
   							 -1,
   							 BTK_PARAM_READWRITE));
  /**
   * BtkRecentChooser:sort-type:
   *
   * Sorting order to be used when displaying the recently used resources.
   *
   * Since: 2.10
   */
  g_object_interface_install_property (g_iface,
		  		       g_param_spec_enum ("sort-type",
					       		  P_("Sort Type"),
							  P_("The sorting order of the items displayed"),
							  BTK_TYPE_RECENT_SORT_TYPE,
							  BTK_RECENT_SORT_NONE,
							  BTK_PARAM_READWRITE));
  /**
   * BtkRecentChooser:filter:
   *
   * The #BtkRecentFilter object to be used when displaying
   * the recently used resources.
   *
   * Since: 2.10
   */
  g_object_interface_install_property (g_iface,
  				       g_param_spec_object ("filter",
  				       			    P_("Filter"),
  				       			    P_("The current filter for selecting which resources are displayed"),
  				       			    BTK_TYPE_RECENT_FILTER,
  				       			    BTK_PARAM_READWRITE));
}

GQuark
btk_recent_chooser_error_quark (void)
{
  return g_quark_from_static_string ("btk-recent-chooser-error-quark");
}

/**
 * _btk_recent_chooser_get_recent_manager:
 * @chooser: a #BtkRecentChooser
 *
 * Gets the #BtkRecentManager used by @chooser.
 *
 * Return value: the recent manager for @chooser.
 *
 * Since: 2.10
 */
BtkRecentManager *
_btk_recent_chooser_get_recent_manager (BtkRecentChooser *chooser)
{
  g_return_val_if_fail (BTK_IS_RECENT_CHOOSER (chooser), NULL);
  
  return BTK_RECENT_CHOOSER_GET_IFACE (chooser)->get_recent_manager (chooser);
}

/**
 * btk_recent_chooser_set_show_private:
 * @chooser: a #BtkRecentChooser
 * @show_private: %TRUE to show private items, %FALSE otherwise
 *
 * Whether to show recently used resources marked registered as private.
 *
 * Since: 2.10
 */
void
btk_recent_chooser_set_show_private (BtkRecentChooser *chooser,
				     bboolean          show_private)
{
  g_return_if_fail (BTK_IS_RECENT_CHOOSER (chooser));
  
  g_object_set (chooser, "show-private", show_private, NULL);
}

/**
 * btk_recent_chooser_get_show_private:
 * @chooser: a #BtkRecentChooser
 *
 * Returns whether @chooser should display recently used resources
 * registered as private.
 *
 * Return value: %TRUE if the recent chooser should show private items,
 *   %FALSE otherwise.
 *
 * Since: 2.10
 */
bboolean
btk_recent_chooser_get_show_private (BtkRecentChooser *chooser)
{
  bboolean show_private;
  
  g_return_val_if_fail (BTK_IS_RECENT_CHOOSER (chooser), FALSE);
  
  g_object_get (chooser, "show-private", &show_private, NULL);
  
  return show_private;
}

/**
 * btk_recent_chooser_set_show_not_found:
 * @chooser: a #BtkRecentChooser
 * @show_not_found: whether to show the local items we didn't find
 *
 * Sets whether @chooser should display the recently used resources that
 * it didn't find.  This only applies to local resources.
 *
 * Since: 2.10
 */
void
btk_recent_chooser_set_show_not_found (BtkRecentChooser *chooser,
				       bboolean          show_not_found)
{
  g_return_if_fail (BTK_IS_RECENT_CHOOSER (chooser));
  
  g_object_set (chooser, "show-not-found", show_not_found, NULL);
}

/**
 * btk_recent_chooser_get_show_not_found:
 * @chooser: a #BtkRecentChooser
 *
 * Retrieves whether @chooser should show the recently used resources that
 * were not found.
 *
 * Return value: %TRUE if the resources not found should be displayed, and
 *   %FALSE otheriwse.
 *
 * Since: 2.10
 */
bboolean
btk_recent_chooser_get_show_not_found (BtkRecentChooser *chooser)
{
  bboolean show_not_found;
  
  g_return_val_if_fail (BTK_IS_RECENT_CHOOSER (chooser), FALSE);
  
  g_object_get (chooser, "show-not-found", &show_not_found, NULL);
  
  return show_not_found;
}

/**
 * btk_recent_chooser_set_show_icons:
 * @chooser: a #BtkRecentChooser
 * @show_icons: whether to show an icon near the resource
 *
 * Sets whether @chooser should show an icon near the resource when
 * displaying it.
 *
 * Since: 2.10
 */
void
btk_recent_chooser_set_show_icons (BtkRecentChooser *chooser,
				   bboolean          show_icons)
{
  g_return_if_fail (BTK_IS_RECENT_CHOOSER (chooser));
  
  g_object_set (chooser, "show-icons", show_icons, NULL);
}

/**
 * btk_recent_chooser_get_show_icons:
 * @chooser: a #BtkRecentChooser
 *
 * Retrieves whether @chooser should show an icon near the resource.
 *
 * Return value: %TRUE if the icons should be displayed, %FALSE otherwise.
 *
 * Since: 2.10
 */
bboolean
btk_recent_chooser_get_show_icons (BtkRecentChooser *chooser)
{
  bboolean show_icons;
  
  g_return_val_if_fail (BTK_IS_RECENT_CHOOSER (chooser), FALSE);
  
  g_object_get (chooser, "show-icons", &show_icons, NULL);
  
  return show_icons;
}

/**
 * btk_recent_chooser_set_select_multiple:
 * @chooser: a #BtkRecentChooser
 * @select_multiple: %TRUE if @chooser can select more than one item
 *
 * Sets whether @chooser can select multiple items.
 *
 * Since: 2.10
 */
void
btk_recent_chooser_set_select_multiple (BtkRecentChooser *chooser,
					bboolean          select_multiple)
{
  g_return_if_fail (BTK_IS_RECENT_CHOOSER (chooser));
  
  g_object_set (chooser, "select-multiple", select_multiple, NULL);
}

/**
 * btk_recent_chooser_get_select_multiple:
 * @chooser: a #BtkRecentChooser
 *
 * Gets whether @chooser can select multiple items.
 *
 * Return value: %TRUE if @chooser can select more than one item.
 *
 * Since: 2.10
 */
bboolean
btk_recent_chooser_get_select_multiple (BtkRecentChooser *chooser)
{
  bboolean select_multiple;
  
  g_return_val_if_fail (BTK_IS_RECENT_CHOOSER (chooser), FALSE);
  
  g_object_get (chooser, "select-multiple", &select_multiple, NULL);
  
  return select_multiple;
}

/**
 * btk_recent_chooser_set_local_only:
 * @chooser: a #BtkRecentChooser
 * @local_only: %TRUE if only local files can be shown
 * 
 * Sets whether only local resources, that is resources using the file:// URI
 * scheme, should be shown in the recently used resources selector.  If
 * @local_only is %TRUE (the default) then the shown resources are guaranteed
 * to be accessible through the operating system native file system.
 *
 * Since: 2.10
 */
void
btk_recent_chooser_set_local_only (BtkRecentChooser *chooser,
				   bboolean          local_only)
{
  g_return_if_fail (BTK_IS_RECENT_CHOOSER (chooser));

  g_object_set (chooser, "local-only", local_only, NULL);
}

/**
 * btk_recent_chooser_get_local_only:
 * @chooser: a #BtkRecentChooser
 *
 * Gets whether only local resources should be shown in the recently used
 * resources selector.  See btk_recent_chooser_set_local_only()
 *
 * Return value: %TRUE if only local resources should be shown.
 *
 * Since: 2.10
 */
bboolean
btk_recent_chooser_get_local_only (BtkRecentChooser *chooser)
{
  bboolean local_only;

  g_return_val_if_fail (BTK_IS_RECENT_CHOOSER (chooser), FALSE);

  g_object_get (chooser, "local-only", &local_only, NULL);

  return local_only;
}

/**
 * btk_recent_chooser_set_limit:
 * @chooser: a #BtkRecentChooser
 * @limit: a positive integer, or -1 for all items
 *
 * Sets the number of items that should be returned by
 * btk_recent_chooser_get_items() and btk_recent_chooser_get_uris().
 *
 * Since: 2.10
 */
void
btk_recent_chooser_set_limit (BtkRecentChooser *chooser,
			      bint              limit)
{
  g_return_if_fail (BTK_IS_RECENT_CHOOSER (chooser));
  
  g_object_set (chooser, "limit", limit, NULL);
}

/**
 * btk_recent_chooser_get_limit:
 * @chooser: a #BtkRecentChooser
 *
 * Gets the number of items returned by btk_recent_chooser_get_items()
 * and btk_recent_chooser_get_uris().
 *
 * Return value: A positive integer, or -1 meaning that all items are
 *   returned.
 *
 * Since: 2.10
 */
bint
btk_recent_chooser_get_limit (BtkRecentChooser *chooser)
{
  bint limit;
  
  g_return_val_if_fail (BTK_IS_RECENT_CHOOSER (chooser), 10);
  
  g_object_get (chooser, "limit", &limit, NULL);
  
  return limit;
}

/**
 * btk_recent_chooser_set_show_tips:
 * @chooser: a #BtkRecentChooser
 * @show_tips: %TRUE if tooltips should be shown
 *
 * Sets whether to show a tooltips containing the full path of each
 * recently used resource in a #BtkRecentChooser widget.
 *
 * Since: 2.10
 */
void
btk_recent_chooser_set_show_tips (BtkRecentChooser *chooser,
				  bboolean          show_tips)
{
  g_return_if_fail (BTK_IS_RECENT_CHOOSER (chooser));
  
  g_object_set (chooser, "show-tips", show_tips, NULL);
}

/**
 * btk_recent_chooser_get_show_tips:
 * @chooser: a #BtkRecentChooser
 *
 * Gets whether @chooser should display tooltips containing the full path
 * of a recently user resource.
 *
 * Return value: %TRUE if the recent chooser should show tooltips,
 *   %FALSE otherwise.
 *
 * Since: 2.10
 */
bboolean
btk_recent_chooser_get_show_tips (BtkRecentChooser *chooser)
{
  bboolean show_tips;
  
  g_return_val_if_fail (BTK_IS_RECENT_CHOOSER (chooser), FALSE);
  
  g_object_get (chooser, "show-tips", &show_tips, NULL);
  
  return show_tips;
}

static bboolean
recent_chooser_has_show_numbers (BtkRecentChooser *chooser)
{
  BParamSpec *pspec;
  
  /* This is the result of a minor screw up: the "show-numbers" property
   * was removed from the BtkRecentChooser interface, but the accessors
   * remained in the interface API; now we need to check whether the
   * implementation of the RecentChooser interface has a "show-numbers"
   * boolean property installed before accessing it, and avoid an
   * assertion failure using a more graceful warning.  This should really
   * go away as soon as we can break API and remove these accessors.
   */
  pspec = g_object_class_find_property (B_OBJECT_GET_CLASS (chooser),
		  			"show-numbers");

  return (pspec && pspec->value_type == B_TYPE_BOOLEAN);
}

/**
 * btk_recent_chooser_set_show_numbers:
 * @chooser: a #BtkRecentChooser
 * @show_numbers: %TRUE to show numbers, %FALSE otherwise
 *
 * Whether to show recently used resources prepended by a unique number.
 *
 * Deprecated: 2.12: Use btk_recent_chooser_menu_set_show_numbers() instead.
 *
 * Since: 2.10
 */
void
btk_recent_chooser_set_show_numbers (BtkRecentChooser *chooser,
				     bboolean          show_numbers)
{
  g_return_if_fail (BTK_IS_RECENT_CHOOSER (chooser));

  if (!recent_chooser_has_show_numbers (chooser))
    {
      g_warning ("Choosers of type `%s' do not support showing numbers",
		 B_OBJECT_TYPE_NAME (chooser));
      
      return;
    }
  
  g_object_set (chooser, "show-numbers", show_numbers, NULL);
}

/**
 * btk_recent_chooser_get_show_numbers:
 * @chooser: a #BtkRecentChooser
 *
 * Returns whether @chooser should display recently used resources
 * prepended by a unique number.
 *
 * Deprecated: 2.12: use btk_recent_chooser_menu_get_show_numbers() instead.
 *
 * Return value: %TRUE if the recent chooser should show display numbers,
 *   %FALSE otherwise.
 *
 * Since: 2.10
 */
bboolean
btk_recent_chooser_get_show_numbers (BtkRecentChooser *chooser)
{
  BParamSpec *pspec;
  bboolean show_numbers;
  
  g_return_val_if_fail (BTK_IS_RECENT_CHOOSER (chooser), FALSE);

  pspec = g_object_class_find_property (B_OBJECT_GET_CLASS (chooser),
		  			"show-numbers");
  if (!pspec || pspec->value_type != B_TYPE_BOOLEAN)
    {
      g_warning ("Choosers of type `%s' do not support showing numbers",
		 B_OBJECT_TYPE_NAME (chooser));

      return FALSE;
    }
  
  g_object_get (chooser, "show-numbers", &show_numbers, NULL);
  
  return show_numbers;
}


/**
 * btk_recent_chooser_set_sort_type:
 * @chooser: a #BtkRecentChooser
 * @sort_type: sort order that the chooser should use
 *
 * Changes the sorting order of the recently used resources list displayed by
 * @chooser.
 *
 * Since: 2.10
 */
void
btk_recent_chooser_set_sort_type (BtkRecentChooser  *chooser,
				  BtkRecentSortType  sort_type)
{  
  g_return_if_fail (BTK_IS_RECENT_CHOOSER (chooser));
  
  g_object_set (chooser, "sort-type", sort_type, NULL);
}

/**
 * btk_recent_chooser_get_sort_type:
 * @chooser: a #BtkRecentChooser
 *
 * Gets the value set by btk_recent_chooser_set_sort_type().
 *
 * Return value: the sorting order of the @chooser.
 *
 * Since: 2.10
 */
BtkRecentSortType
btk_recent_chooser_get_sort_type (BtkRecentChooser *chooser)
{
  BtkRecentSortType sort_type;
  
  g_return_val_if_fail (BTK_IS_RECENT_CHOOSER (chooser), BTK_RECENT_SORT_NONE);
  
  g_object_get (chooser, "sort-type", &sort_type, NULL);

  return sort_type;
}

/**
 * btk_recent_chooser_set_sort_func:
 * @chooser: a #BtkRecentChooser
 * @sort_func: the comparison function
 * @sort_data: (allow-none): user data to pass to @sort_func, or %NULL
 * @data_destroy: (allow-none): destroy notifier for @sort_data, or %NULL
 *
 * Sets the comparison function used when sorting to be @sort_func.  If
 * the @chooser has the sort type set to #BTK_RECENT_SORT_CUSTOM then
 * the chooser will sort using this function.
 *
 * To the comparison function will be passed two #BtkRecentInfo structs and
 * @sort_data;  @sort_func should return a positive integer if the first
 * item comes before the second, zero if the two items are equal and
 * a negative integer if the first item comes after the second.
 *
 * Since: 2.10
 */
void
btk_recent_chooser_set_sort_func  (BtkRecentChooser  *chooser,
				   BtkRecentSortFunc  sort_func,
				   bpointer           sort_data,
				   GDestroyNotify     data_destroy)
{
  g_return_if_fail (BTK_IS_RECENT_CHOOSER (chooser));
  
  BTK_RECENT_CHOOSER_GET_IFACE (chooser)->set_sort_func (chooser,
  							 sort_func,
  							 sort_data,
  							 data_destroy);
}

/**
 * btk_recent_chooser_set_current_uri:
 * @chooser: a #BtkRecentChooser
 * @uri: a URI
 * @error: (allow-none): return location for a #GError, or %NULL
 *
 * Sets @uri as the current URI for @chooser.
 *
 * Return value: %TRUE if the URI was found.
 *
 * Since: 2.10
 */
bboolean
btk_recent_chooser_set_current_uri (BtkRecentChooser  *chooser,
				    const bchar       *uri,
				    GError           **error)
{
  g_return_val_if_fail (BTK_IS_RECENT_CHOOSER (chooser), FALSE);
  
  return BTK_RECENT_CHOOSER_GET_IFACE (chooser)->set_current_uri (chooser, uri, error);
}

/**
 * btk_recent_chooser_get_current_uri:
 * @chooser: a #BtkRecentChooser
 *
 * Gets the URI currently selected by @chooser.
 *
 * Return value: a newly allocated string holding a URI.
 *
 * Since: 2.10
 */
bchar *
btk_recent_chooser_get_current_uri (BtkRecentChooser *chooser)
{
  g_return_val_if_fail (BTK_IS_RECENT_CHOOSER (chooser), NULL);
  
  return BTK_RECENT_CHOOSER_GET_IFACE (chooser)->get_current_uri (chooser);
}

/**
 * btk_recent_chooser_get_current_item:
 * @chooser: a #BtkRecentChooser
 * 
 * Gets the #BtkRecentInfo currently selected by @chooser.
 *
 * Return value: a #BtkRecentInfo.  Use btk_recent_info_unref() when
 *   when you have finished using it.
 *
 * Since: 2.10
 */
BtkRecentInfo *
btk_recent_chooser_get_current_item (BtkRecentChooser *chooser)
{
  BtkRecentManager *manager;
  BtkRecentInfo *retval;
  bchar *uri;
  
  g_return_val_if_fail (BTK_IS_RECENT_CHOOSER (chooser), NULL);
  
  uri = btk_recent_chooser_get_current_uri (chooser);
  if (!uri)
    return NULL;
  
  manager = _btk_recent_chooser_get_recent_manager (chooser);
  retval = btk_recent_manager_lookup_item (manager, uri, NULL);
  g_free (uri);
  
  return retval;
}

/**
 * btk_recent_chooser_select_uri:
 * @chooser: a #BtkRecentChooser
 * @uri: a URI
 * @error: (allow-none): return location for a #GError, or %NULL
 *
 * Selects @uri inside @chooser.
 *
 * Return value: %TRUE if @uri was found.
 *
 * Since: 2.10
 */
bboolean
btk_recent_chooser_select_uri (BtkRecentChooser  *chooser,
			       const bchar       *uri,
			       GError           **error)
{
  g_return_val_if_fail (BTK_IS_RECENT_CHOOSER (chooser), FALSE);
  
  return BTK_RECENT_CHOOSER_GET_IFACE (chooser)->select_uri (chooser, uri, error);
}

/**
 * btk_recent_chooser_unselect_uri:
 * @chooser: a #BtkRecentChooser
 * @uri: a URI
 *
 * Unselects @uri inside @chooser.
 *
 * Since: 2.10
 */
void
btk_recent_chooser_unselect_uri (BtkRecentChooser *chooser,
				 const bchar      *uri)
{
  g_return_if_fail (BTK_IS_RECENT_CHOOSER (chooser));
  
  BTK_RECENT_CHOOSER_GET_IFACE (chooser)->unselect_uri (chooser, uri);
}

/**
 * btk_recent_chooser_select_all:
 * @chooser: a #BtkRecentChooser
 *
 * Selects all the items inside @chooser, if the @chooser supports
 * multiple selection.
 *
 * Since: 2.10
 */
void
btk_recent_chooser_select_all (BtkRecentChooser *chooser)
{
  g_return_if_fail (BTK_IS_RECENT_CHOOSER (chooser));
  
  BTK_RECENT_CHOOSER_GET_IFACE (chooser)->select_all (chooser);
}

/**
 * btk_recent_chooser_unselect_all:
 * @chooser: a #BtkRecentChooser
 *
 * Unselects all the items inside @chooser.
 * 
 * Since: 2.10
 */
void
btk_recent_chooser_unselect_all (BtkRecentChooser *chooser)
{
  g_return_if_fail (BTK_IS_RECENT_CHOOSER (chooser));
  
  BTK_RECENT_CHOOSER_GET_IFACE (chooser)->unselect_all (chooser);
}

/**
 * btk_recent_chooser_get_items:
 * @chooser: a #BtkRecentChooser
 *
 * Gets the list of recently used resources in form of #BtkRecentInfo objects.
 *
 * The return value of this function is affected by the "sort-type" and
 * "limit" properties of @chooser.
 *
 * Return value:  (element-type BtkRecentInfo) (transfer full): A newly allocated
 *   list of #BtkRecentInfo objects.  You should
 *   use btk_recent_info_unref() on every item of the list, and then free
 *   the list itself using g_list_free().
 *
 * Since: 2.10
 */
GList *
btk_recent_chooser_get_items (BtkRecentChooser *chooser)
{
  g_return_val_if_fail (BTK_IS_RECENT_CHOOSER (chooser), NULL);
  
  return BTK_RECENT_CHOOSER_GET_IFACE (chooser)->get_items (chooser);
}

/**
 * btk_recent_chooser_get_uris:
 * @chooser: a #BtkRecentChooser
 * @length: (allow-none): return location for a the length of the URI list, or %NULL
 *
 * Gets the URI of the recently used resources.
 *
 * The return value of this function is affected by the "sort-type" and "limit"
 * properties of @chooser.
 *
 * Since the returned array is %NULL terminated, @length may be %NULL.
 * 
 * Return value: (array length=length zero-terminated=1) (transfer full):
 *     A newly allocated, %NULL-terminated array of strings. Use
 *     g_strfreev() to free it.
 *
 * Since: 2.10
 */
bchar **
btk_recent_chooser_get_uris (BtkRecentChooser *chooser,
                             bsize            *length)
{
  GList *items, *l;
  bchar **retval;
  bsize n_items, i;
  
  items = btk_recent_chooser_get_items (chooser);
  
  n_items = g_list_length (items);
  retval = g_new0 (bchar *, n_items + 1);
  
  for (l = items, i = 0; l != NULL; l = l->next)
    {
      BtkRecentInfo *info = (BtkRecentInfo *) l->data;
      const bchar *uri;
      
      g_assert (info != NULL);
      
      uri = btk_recent_info_get_uri (info);
      g_assert (uri != NULL);
      
      retval[i++] = g_strdup (uri);
    }
  retval[i] = NULL;
  
  if (length)
    *length = i;
  
  g_list_foreach (items,
  		  (GFunc) btk_recent_info_unref,
  		  NULL);
  g_list_free (items);
  
  return retval;
}

/**
 * btk_recent_chooser_add_filter:
 * @chooser: a #BtkRecentChooser
 * @filter: a #BtkRecentFilter
 *
 * Adds @filter to the list of #BtkRecentFilter objects held by @chooser.
 *
 * If no previous filter objects were defined, this function will call
 * btk_recent_chooser_set_filter().
 *
 * Since: 2.10
 */
void
btk_recent_chooser_add_filter (BtkRecentChooser *chooser,
			       BtkRecentFilter  *filter)
{
  g_return_if_fail (BTK_IS_RECENT_CHOOSER (chooser));
  g_return_if_fail (BTK_IS_RECENT_FILTER (filter));
  
  BTK_RECENT_CHOOSER_GET_IFACE (chooser)->add_filter (chooser, filter);
}

/**
 * btk_recent_chooser_remove_filter:
 * @chooser: a #BtkRecentChooser
 * @filter: a #BtkRecentFilter
 *
 * Removes @filter from the list of #BtkRecentFilter objects held by @chooser.
 *
 * Since: 2.10
 */
void
btk_recent_chooser_remove_filter (BtkRecentChooser *chooser,
				  BtkRecentFilter  *filter)
{
  g_return_if_fail (BTK_IS_RECENT_CHOOSER (chooser));
  g_return_if_fail (BTK_IS_RECENT_FILTER (filter));
  
  BTK_RECENT_CHOOSER_GET_IFACE (chooser)->remove_filter (chooser, filter);
}

/**
 * btk_recent_chooser_list_filters:
 * @chooser: a #BtkRecentChooser
 *
 * Gets the #BtkRecentFilter objects held by @chooser.
 *
 * Return value: (element-type BtkRecentFilter) (transfer container): A singly linked list
 *   of #BtkRecentFilter objects.  You
 *   should just free the returned list using b_slist_free().
 *
 * Since: 2.10
 */
GSList *
btk_recent_chooser_list_filters (BtkRecentChooser *chooser)
{
  g_return_val_if_fail (BTK_IS_RECENT_CHOOSER (chooser), NULL);
  
  return BTK_RECENT_CHOOSER_GET_IFACE (chooser)->list_filters (chooser);
}

/**
 * btk_recent_chooser_set_filter:
 * @chooser: a #BtkRecentChooser
 * @filter: a #BtkRecentFilter
 *
 * Sets @filter as the current #BtkRecentFilter object used by @chooser
 * to affect the displayed recently used resources.
 *
 * Since: 2.10
 */
void
btk_recent_chooser_set_filter (BtkRecentChooser *chooser,
			       BtkRecentFilter  *filter)
{
  g_return_if_fail (BTK_IS_RECENT_CHOOSER (chooser));
  g_return_if_fail (BTK_IS_RECENT_FILTER (filter));
  
  g_object_set (B_OBJECT (chooser), "filter", filter, NULL);
}

/**
 * btk_recent_chooser_get_filter:
 * @chooser: a #BtkRecentChooser
 *
 * Gets the #BtkRecentFilter object currently used by @chooser to affect
 * the display of the recently used resources.
 *
 * Return value: (transfer none): a #BtkRecentFilter object.
 *
 * Since: 2.10
 */
BtkRecentFilter *
btk_recent_chooser_get_filter (BtkRecentChooser *chooser)
{
  BtkRecentFilter *filter;
  
  g_return_val_if_fail (BTK_IS_RECENT_CHOOSER (chooser), NULL);
  
  g_object_get (B_OBJECT (chooser), "filter", &filter, NULL);
  
  /* we need this hack because g_object_get() increases the refcount
   * of the returned object; see also btk_file_chooser_get_filter()
   * inside btkfilechooser.c
   */
  if (filter)
    g_object_unref (filter);
  
  return filter;
}

void
_btk_recent_chooser_item_activated (BtkRecentChooser *chooser)
{
  g_return_if_fail (BTK_IS_RECENT_CHOOSER (chooser));
  
  g_signal_emit (chooser, chooser_signals[ITEM_ACTIVATED], 0);
}

void
_btk_recent_chooser_selection_changed (BtkRecentChooser *chooser)
{
  g_return_if_fail (BTK_IS_RECENT_CHOOSER (chooser));

  g_signal_emit (chooser, chooser_signals[SELECTION_CHANGED], 0);
}

void
_btk_recent_chooser_update (BtkActivatable *activatable,
			    BtkAction      *action,
			    const bchar    *property_name)
{
  BtkRecentChooser *recent_chooser = BTK_RECENT_CHOOSER (activatable);
  BtkRecentChooser *action_chooser = BTK_RECENT_CHOOSER (action);
  BtkRecentAction  *recent_action  = BTK_RECENT_ACTION (action);

  if (strcmp (property_name, "show-numbers") == 0 && recent_chooser_has_show_numbers (recent_chooser))
    g_object_set (recent_chooser, "show-numbers",
                  btk_recent_action_get_show_numbers (recent_action), NULL);
  else if (strcmp (property_name, "show-private") == 0)
    btk_recent_chooser_set_show_private (recent_chooser, btk_recent_chooser_get_show_private (action_chooser));
  else if (strcmp (property_name, "show-not-found") == 0)
    btk_recent_chooser_set_show_not_found (recent_chooser, btk_recent_chooser_get_show_not_found (action_chooser));
  else if (strcmp (property_name, "show-tips") == 0)
    btk_recent_chooser_set_show_tips (recent_chooser, btk_recent_chooser_get_show_tips (action_chooser));
  else if (strcmp (property_name, "show-icons") == 0)
    btk_recent_chooser_set_show_icons (recent_chooser, btk_recent_chooser_get_show_icons (action_chooser));
  else if (strcmp (property_name, "limit") == 0)
    btk_recent_chooser_set_limit (recent_chooser, btk_recent_chooser_get_limit (action_chooser));
  else if (strcmp (property_name, "local-only") == 0)
    btk_recent_chooser_set_local_only (recent_chooser, btk_recent_chooser_get_local_only (action_chooser));
  else if (strcmp (property_name, "sort-type") == 0)
    btk_recent_chooser_set_sort_type (recent_chooser, btk_recent_chooser_get_sort_type (action_chooser));
  else if (strcmp (property_name, "filter") == 0)
    btk_recent_chooser_set_filter (recent_chooser, btk_recent_chooser_get_filter (action_chooser));
}

void
_btk_recent_chooser_sync_action_properties (BtkActivatable *activatable,
			                    BtkAction      *action)
{
  BtkRecentChooser *recent_chooser = BTK_RECENT_CHOOSER (activatable);
  BtkRecentChooser *action_chooser = BTK_RECENT_CHOOSER (action);

  if (!action)
    return;

  if (recent_chooser_has_show_numbers (recent_chooser))
    g_object_set (recent_chooser, "show-numbers", 
                  btk_recent_action_get_show_numbers (BTK_RECENT_ACTION (action)),
                  NULL);
  btk_recent_chooser_set_show_private (recent_chooser, btk_recent_chooser_get_show_private (action_chooser));
  btk_recent_chooser_set_show_not_found (recent_chooser, btk_recent_chooser_get_show_not_found (action_chooser));
  btk_recent_chooser_set_show_tips (recent_chooser, btk_recent_chooser_get_show_tips (action_chooser));
  btk_recent_chooser_set_show_icons (recent_chooser, btk_recent_chooser_get_show_icons (action_chooser));
  btk_recent_chooser_set_limit (recent_chooser, btk_recent_chooser_get_limit (action_chooser));
  btk_recent_chooser_set_local_only (recent_chooser, btk_recent_chooser_get_local_only (action_chooser));
  btk_recent_chooser_set_sort_type (recent_chooser, btk_recent_chooser_get_sort_type (action_chooser));
  btk_recent_chooser_set_filter (recent_chooser, btk_recent_chooser_get_filter (action_chooser));
}

void
_btk_recent_chooser_set_related_action (BtkRecentChooser *recent_chooser,
					BtkAction        *action)
{
  BtkAction *prev_action;

  prev_action = g_object_get_qdata (B_OBJECT (recent_chooser), quark_btk_related_action);

  if (prev_action == action)
    return;

  btk_activatable_do_set_related_action (BTK_ACTIVATABLE (recent_chooser), action);
  g_object_set_qdata (B_OBJECT (recent_chooser), quark_btk_related_action, action);
}

BtkAction *
_btk_recent_chooser_get_related_action (BtkRecentChooser *recent_chooser)
{
  return g_object_get_qdata (B_OBJECT (recent_chooser), quark_btk_related_action);
}

/* The default for use-action-appearance is TRUE, so we try to set the
 * qdata backwards for this case.
 */
void
_btk_recent_chooser_set_use_action_appearance (BtkRecentChooser *recent_chooser, 
					       bboolean          use_appearance)
{
  BtkAction *action;
  bboolean   use_action_appearance;

  action                = g_object_get_qdata (B_OBJECT (recent_chooser), quark_btk_related_action);
  use_action_appearance = !BPOINTER_TO_INT (g_object_get_qdata (B_OBJECT (recent_chooser), quark_btk_use_action_appearance));

  if (use_action_appearance != use_appearance)
    {

      g_object_set_qdata (B_OBJECT (recent_chooser), quark_btk_use_action_appearance, BINT_TO_POINTER (!use_appearance));
 
      btk_activatable_sync_action_properties (BTK_ACTIVATABLE (recent_chooser), action);
    }
}

bboolean
_btk_recent_chooser_get_use_action_appearance (BtkRecentChooser *recent_chooser)
{
  return !BPOINTER_TO_INT (g_object_get_qdata (B_OBJECT (recent_chooser), quark_btk_use_action_appearance));
}

#define __BTK_RECENT_CHOOSER_C__
#include "btkaliasdef.c"
