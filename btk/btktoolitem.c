/* btktoolitem.c
 *
 * Copyright (C) 2002 Anders Carlsson <andersca@bunny.org>
 * Copyright (C) 2002 James Henstridge <james@daa.com.au>
 * Copyright (C) 2003 Soeren Sandmann <sandmann@daimi.au.dk>
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

#undef BTK_DISABLE_DEPRECATED /* BtkTooltips */

#include "btktoolitem.h"
#include "btkmarshalers.h"
#include "btktoolshell.h"
#include "btkseparatormenuitem.h"
#include "btkactivatable.h"
#include "btkintl.h"
#include "btkmain.h"
#include "btkprivate.h"
#include "btkalias.h"

/**
 * SECTION:btktoolitem
 * @short_description: The base class of widgets that can be added to BtkToolShell
 * @see_also: <variablelist>
 *   <varlistentry>
 *     <term>#BtkToolbar</term>
 *     <listitem><para>The toolbar widget</para></listitem>
 *   </varlistentry>
 *   <varlistentry>
 *     <term>#BtkToolButton</term>
 *     <listitem><para>A subclass of #BtkToolItem that displays buttons on
 *         the toolbar</para></listitem>
 *   </varlistentry>
 *   <varlistentry>
 *     <term>#BtkSeparatorToolItem</term>
 *     <listitem><para>A subclass of #BtkToolItem that separates groups of
 *         items on a toolbar</para></listitem>
 *   </varlistentry>
 * </variablelist>
 *
 * #BtkToolItem<!-- -->s are widgets that can appear on a toolbar. To
 * create a toolbar item that contain something else than a button, use
 * btk_tool_item_new(). Use btk_container_add() to add a child
 * widget to the tool item.
 *
 * For toolbar items that contain buttons, see the #BtkToolButton,
 * #BtkToggleToolButton and #BtkRadioToolButton classes.
 *
 * See the #BtkToolbar class for a description of the toolbar widget, and
 * #BtkToolShell for a description of the tool shell interface.
 */

/**
 * BtkToolItem:
 *
 * The BtkToolItem struct contains only private data.
 * It should only be accessed through the functions described below.
 */

enum {
  CREATE_MENU_PROXY,
  TOOLBAR_RECONFIGURED,
  SET_TOOLTIP,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_VISIBLE_HORIZONTAL,
  PROP_VISIBLE_VERTICAL,
  PROP_IS_IMPORTANT,

  /* activatable properties */
  PROP_ACTIVATABLE_RELATED_ACTION,
  PROP_ACTIVATABLE_USE_ACTION_APPEARANCE
};

#define BTK_TOOL_ITEM_GET_PRIVATE(o)  (B_TYPE_INSTANCE_GET_PRIVATE ((o), BTK_TYPE_TOOL_ITEM, BtkToolItemPrivate))

struct _BtkToolItemPrivate
{
  gchar *tip_text;
  gchar *tip_private;

  guint visible_horizontal : 1;
  guint visible_vertical : 1;
  guint homogeneous : 1;
  guint expand : 1;
  guint use_drag_window : 1;
  guint is_important : 1;

  BdkWindow *drag_window;
  
  gchar *menu_item_id;
  BtkWidget *menu_item;

  BtkAction *action;
  gboolean   use_action_appearance;
};
  
static void btk_tool_item_finalize     (BObject         *object);
static void btk_tool_item_dispose      (BObject         *object);
static void btk_tool_item_parent_set   (BtkWidget       *toolitem,
				        BtkWidget       *parent);
static void btk_tool_item_set_property (BObject         *object,
					guint            prop_id,
					const BValue    *value,
					BParamSpec      *pspec);
static void btk_tool_item_get_property (BObject         *object,
					guint            prop_id,
					BValue          *value,
					BParamSpec      *pspec);
static void btk_tool_item_property_notify (BObject      *object,
					   BParamSpec   *pspec);
static void btk_tool_item_realize       (BtkWidget      *widget);
static void btk_tool_item_unrealize     (BtkWidget      *widget);
static void btk_tool_item_map           (BtkWidget      *widget);
static void btk_tool_item_unmap         (BtkWidget      *widget);
static void btk_tool_item_size_request  (BtkWidget      *widget,
					 BtkRequisition *requisition);
static void btk_tool_item_size_allocate (BtkWidget      *widget,
					 BtkAllocation  *allocation);
static gboolean btk_tool_item_real_set_tooltip (BtkToolItem *tool_item,
						BtkTooltips *tooltips,
						const gchar *tip_text,
						const gchar *tip_private);

static void btk_tool_item_activatable_interface_init (BtkActivatableIface  *iface);
static void btk_tool_item_update                     (BtkActivatable       *activatable,
						      BtkAction            *action,
						      const gchar          *property_name);
static void btk_tool_item_sync_action_properties     (BtkActivatable       *activatable,
						      BtkAction            *action);
static void btk_tool_item_set_related_action         (BtkToolItem          *item, 
						      BtkAction            *action);
static void btk_tool_item_set_use_action_appearance  (BtkToolItem          *item, 
						      gboolean              use_appearance);

static guint toolitem_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (BtkToolItem, btk_tool_item, BTK_TYPE_BIN,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_ACTIVATABLE,
						btk_tool_item_activatable_interface_init))

static void
btk_tool_item_class_init (BtkToolItemClass *klass)
{
  BObjectClass *object_class;
  BtkWidgetClass *widget_class;
  
  object_class = (BObjectClass *)klass;
  widget_class = (BtkWidgetClass *)klass;
  
  object_class->set_property = btk_tool_item_set_property;
  object_class->get_property = btk_tool_item_get_property;
  object_class->finalize     = btk_tool_item_finalize;
  object_class->dispose      = btk_tool_item_dispose;
  object_class->notify       = btk_tool_item_property_notify;

  widget_class->realize       = btk_tool_item_realize;
  widget_class->unrealize     = btk_tool_item_unrealize;
  widget_class->map           = btk_tool_item_map;
  widget_class->unmap         = btk_tool_item_unmap;
  widget_class->size_request  = btk_tool_item_size_request;
  widget_class->size_allocate = btk_tool_item_size_allocate;
  widget_class->parent_set    = btk_tool_item_parent_set;

  klass->create_menu_proxy = _btk_tool_item_create_menu_proxy;
  klass->set_tooltip       = btk_tool_item_real_set_tooltip;
  
  g_object_class_install_property (object_class,
				   PROP_VISIBLE_HORIZONTAL,
				   g_param_spec_boolean ("visible-horizontal",
							 P_("Visible when horizontal"),
							 P_("Whether the toolbar item is visible when the toolbar is in a horizontal orientation."),
							 TRUE,
							 BTK_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_VISIBLE_VERTICAL,
				   g_param_spec_boolean ("visible-vertical",
							 P_("Visible when vertical"),
							 P_("Whether the toolbar item is visible when the toolbar is in a vertical orientation."),
							 TRUE,
							 BTK_PARAM_READWRITE));
  g_object_class_install_property (object_class,
 				   PROP_IS_IMPORTANT,
 				   g_param_spec_boolean ("is-important",
 							 P_("Is important"),
 							 P_("Whether the toolbar item is considered important. When TRUE, toolbar buttons show text in BTK_TOOLBAR_BOTH_HORIZ mode"),
 							 FALSE,
 							 BTK_PARAM_READWRITE));

  g_object_class_override_property (object_class, PROP_ACTIVATABLE_RELATED_ACTION, "related-action");
  g_object_class_override_property (object_class, PROP_ACTIVATABLE_USE_ACTION_APPEARANCE, "use-action-appearance");


/**
 * BtkToolItem::create-menu-proxy:
 * @tool_item: the object the signal was emitted on
 *
 * This signal is emitted when the toolbar needs information from @tool_item
 * about whether the item should appear in the toolbar overflow menu. In
 * response the tool item should either
 * <itemizedlist>
 * <listitem>call btk_tool_item_set_proxy_menu_item() with a %NULL
 * pointer and return %TRUE to indicate that the item should not appear
 * in the overflow menu
 * </listitem>
 * <listitem> call btk_tool_item_set_proxy_menu_item() with a new menu
 * item and return %TRUE, or 
 * </listitem>
 * <listitem> return %FALSE to indicate that the signal was not
 * handled by the item. This means that
 * the item will not appear in the overflow menu unless a later handler
 * installs a menu item.
 * </listitem>
 * </itemizedlist>
 *
 * The toolbar may cache the result of this signal. When the tool item changes
 * how it will respond to this signal it must call btk_tool_item_rebuild_menu()
 * to invalidate the cache and ensure that the toolbar rebuilds its overflow
 * menu.
 *
 * Return value: %TRUE if the signal was handled, %FALSE if not
 **/
  toolitem_signals[CREATE_MENU_PROXY] =
    g_signal_new (I_("create-menu-proxy"),
		  B_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkToolItemClass, create_menu_proxy),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__VOID,
		  B_TYPE_BOOLEAN, 0);

/**
 * BtkToolItem::toolbar-reconfigured:
 * @tool_item: the object the signal was emitted on
 *
 * This signal is emitted when some property of the toolbar that the
 * item is a child of changes. For custom subclasses of #BtkToolItem,
 * the default handler of this signal use the functions
 * <itemizedlist>
 * <listitem>btk_tool_shell_get_orientation()</listitem>
 * <listitem>btk_tool_shell_get_style()</listitem>
 * <listitem>btk_tool_shell_get_icon_size()</listitem>
 * <listitem>btk_tool_shell_get_relief_style()</listitem>
 * </itemizedlist>
 * to find out what the toolbar should look like and change
 * themselves accordingly.
 **/
  toolitem_signals[TOOLBAR_RECONFIGURED] =
    g_signal_new (I_("toolbar-reconfigured"),
		  B_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkToolItemClass, toolbar_reconfigured),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
/**
 * BtkToolItem::set-tooltip:
 * @tool_item: the object the signal was emitted on
 * @tooltips: the #BtkTooltips
 * @tip_text: the tooltip text
 * @tip_private: the tooltip private text
 *
 * This signal is emitted when the toolitem's tooltip changes.
 * Application developers can use btk_tool_item_set_tooltip() to
 * set the item's tooltip.
 *
 * Return value: %TRUE if the signal was handled, %FALSE if not
 *
 * Deprecated: 2.12: With the new tooltip API, there is no
 *   need to use this signal anymore.
 **/
  toolitem_signals[SET_TOOLTIP] =
    g_signal_new (I_("set-tooltip"),
		  B_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkToolItemClass, set_tooltip),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__OBJECT_STRING_STRING,
		  B_TYPE_BOOLEAN, 3,
		  BTK_TYPE_TOOLTIPS,
		  B_TYPE_STRING,
		  B_TYPE_STRING);		  

  g_type_class_add_private (object_class, sizeof (BtkToolItemPrivate));
}

static void
btk_tool_item_init (BtkToolItem *toolitem)
{
  btk_widget_set_can_focus (BTK_WIDGET (toolitem), FALSE);

  toolitem->priv = BTK_TOOL_ITEM_GET_PRIVATE (toolitem);

  toolitem->priv->visible_horizontal = TRUE;
  toolitem->priv->visible_vertical = TRUE;
  toolitem->priv->homogeneous = FALSE;
  toolitem->priv->expand = FALSE;

  toolitem->priv->use_action_appearance = TRUE;
}

static void
btk_tool_item_finalize (BObject *object)
{
  BtkToolItem *item = BTK_TOOL_ITEM (object);

  g_free (item->priv->menu_item_id);

  if (item->priv->menu_item)
    g_object_unref (item->priv->menu_item);

  B_OBJECT_CLASS (btk_tool_item_parent_class)->finalize (object);
}

static void
btk_tool_item_dispose (BObject *object)
{
  BtkToolItem *item = BTK_TOOL_ITEM (object);

  if (item->priv->action)
    {
      btk_activatable_do_set_related_action (BTK_ACTIVATABLE (item), NULL);      
      item->priv->action = NULL;
    }
  B_OBJECT_CLASS (btk_tool_item_parent_class)->dispose (object);
}


static void
btk_tool_item_parent_set (BtkWidget   *toolitem,
			  BtkWidget   *prev_parent)
{
  if (BTK_WIDGET (toolitem)->parent != NULL)
    btk_tool_item_toolbar_reconfigured (BTK_TOOL_ITEM (toolitem));
}

static void
btk_tool_item_set_property (BObject      *object,
			    guint         prop_id,
			    const BValue *value,
			    BParamSpec   *pspec)
{
  BtkToolItem *toolitem = BTK_TOOL_ITEM (object);

  switch (prop_id)
    {
    case PROP_VISIBLE_HORIZONTAL:
      btk_tool_item_set_visible_horizontal (toolitem, b_value_get_boolean (value));
      break;
    case PROP_VISIBLE_VERTICAL:
      btk_tool_item_set_visible_vertical (toolitem, b_value_get_boolean (value));
      break;
    case PROP_IS_IMPORTANT:
      btk_tool_item_set_is_important (toolitem, b_value_get_boolean (value));
      break;
    case PROP_ACTIVATABLE_RELATED_ACTION:
      btk_tool_item_set_related_action (toolitem, b_value_get_object (value));
      break;
    case PROP_ACTIVATABLE_USE_ACTION_APPEARANCE:
      btk_tool_item_set_use_action_appearance (toolitem, b_value_get_boolean (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_tool_item_get_property (BObject    *object,
			    guint       prop_id,
			    BValue     *value,
			    BParamSpec *pspec)
{
  BtkToolItem *toolitem = BTK_TOOL_ITEM (object);

  switch (prop_id)
    {
    case PROP_VISIBLE_HORIZONTAL:
      b_value_set_boolean (value, toolitem->priv->visible_horizontal);
      break;
    case PROP_VISIBLE_VERTICAL:
      b_value_set_boolean (value, toolitem->priv->visible_vertical);
      break;
    case PROP_IS_IMPORTANT:
      b_value_set_boolean (value, toolitem->priv->is_important);
      break;
    case PROP_ACTIVATABLE_RELATED_ACTION:
      b_value_set_object (value, toolitem->priv->action);
      break;
    case PROP_ACTIVATABLE_USE_ACTION_APPEARANCE:
      b_value_set_boolean (value, toolitem->priv->use_action_appearance);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_tool_item_property_notify (BObject    *object,
			       BParamSpec *pspec)
{
  BtkToolItem *tool_item = BTK_TOOL_ITEM (object);

  if (tool_item->priv->menu_item && strcmp (pspec->name, "sensitive") == 0)
    btk_widget_set_sensitive (tool_item->priv->menu_item,
			      btk_widget_get_sensitive (BTK_WIDGET (tool_item)));
}

static void
create_drag_window (BtkToolItem *toolitem)
{
  BtkWidget *widget;
  BdkWindowAttr attributes;
  gint attributes_mask, border_width;

  g_return_if_fail (toolitem->priv->use_drag_window == TRUE);

  widget = BTK_WIDGET (toolitem);
  border_width = BTK_CONTAINER (toolitem)->border_width;

  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x + border_width;
  attributes.y = widget->allocation.y + border_width;
  attributes.width = widget->allocation.width - border_width * 2;
  attributes.height = widget->allocation.height - border_width * 2;
  attributes.wclass = BDK_INPUT_ONLY;
  attributes.event_mask = btk_widget_get_events (widget);
  attributes.event_mask |= (BDK_BUTTON_PRESS_MASK | BDK_BUTTON_RELEASE_MASK);

  attributes_mask = BDK_WA_X | BDK_WA_Y;

  toolitem->priv->drag_window = bdk_window_new (btk_widget_get_parent_window (widget),
					  &attributes, attributes_mask);
  bdk_window_set_user_data (toolitem->priv->drag_window, toolitem);
}

static void
btk_tool_item_realize (BtkWidget *widget)
{
  BtkToolItem *toolitem;

  toolitem = BTK_TOOL_ITEM (widget);
  btk_widget_set_realized (widget, TRUE);

  widget->window = btk_widget_get_parent_window (widget);
  g_object_ref (widget->window);

  if (toolitem->priv->use_drag_window)
    create_drag_window(toolitem);

  widget->style = btk_style_attach (widget->style, widget->window);
}

static void
destroy_drag_window (BtkToolItem *toolitem)
{
  if (toolitem->priv->drag_window)
    {
      bdk_window_set_user_data (toolitem->priv->drag_window, NULL);
      bdk_window_destroy (toolitem->priv->drag_window);
      toolitem->priv->drag_window = NULL;
    }
}

static void
btk_tool_item_unrealize (BtkWidget *widget)
{
  BtkToolItem *toolitem;

  toolitem = BTK_TOOL_ITEM (widget);

  destroy_drag_window (toolitem);
  
  BTK_WIDGET_CLASS (btk_tool_item_parent_class)->unrealize (widget);
}

static void
btk_tool_item_map (BtkWidget *widget)
{
  BtkToolItem *toolitem;

  toolitem = BTK_TOOL_ITEM (widget);
  BTK_WIDGET_CLASS (btk_tool_item_parent_class)->map (widget);
  if (toolitem->priv->drag_window)
    bdk_window_show (toolitem->priv->drag_window);
}

static void
btk_tool_item_unmap (BtkWidget *widget)
{
  BtkToolItem *toolitem;

  toolitem = BTK_TOOL_ITEM (widget);
  if (toolitem->priv->drag_window)
    bdk_window_hide (toolitem->priv->drag_window);
  BTK_WIDGET_CLASS (btk_tool_item_parent_class)->unmap (widget);
}

static void
btk_tool_item_size_request (BtkWidget      *widget,
			    BtkRequisition *requisition)
{
  BtkWidget *child = BTK_BIN (widget)->child;

  if (child && btk_widget_get_visible (child))
    {
      btk_widget_size_request (child, requisition);
    }
  else
    {
      requisition->height = 0;
      requisition->width = 0;
    }
  
  requisition->width += (BTK_CONTAINER (widget)->border_width) * 2;
  requisition->height += (BTK_CONTAINER (widget)->border_width) * 2;
}

static void
btk_tool_item_size_allocate (BtkWidget     *widget,
			     BtkAllocation *allocation)
{
  BtkToolItem *toolitem = BTK_TOOL_ITEM (widget);
  BtkAllocation child_allocation;
  gint border_width;
  BtkWidget *child = BTK_BIN (widget)->child;

  widget->allocation = *allocation;
  border_width = BTK_CONTAINER (widget)->border_width;

  if (toolitem->priv->drag_window)
    bdk_window_move_resize (toolitem->priv->drag_window,
                            widget->allocation.x + border_width,
                            widget->allocation.y + border_width,
                            widget->allocation.width - border_width * 2,
                            widget->allocation.height - border_width * 2);
  
  if (child && btk_widget_get_visible (child))
    {
      child_allocation.x = allocation->x + border_width;
      child_allocation.y = allocation->y + border_width;
      child_allocation.width = allocation->width - 2 * border_width;
      child_allocation.height = allocation->height - 2 * border_width;
      
      btk_widget_size_allocate (child, &child_allocation);
    }
}

gboolean
_btk_tool_item_create_menu_proxy (BtkToolItem *item)
{
  BtkWidget *menu_item;
  gboolean visible_overflown;

  if (item->priv->action)
    {
      g_object_get (item->priv->action, "visible-overflown", &visible_overflown, NULL);
    
      if (visible_overflown)
	{
	  menu_item = btk_action_create_menu_item (item->priv->action);

	  g_object_ref_sink (menu_item);
      	  btk_tool_item_set_proxy_menu_item (item, "btk-action-menu-item", menu_item);
	  g_object_unref (menu_item);
	}
      else
	btk_tool_item_set_proxy_menu_item (item, "btk-action-menu-item", NULL);

      return TRUE;
    }

  return FALSE;
}

static void
btk_tool_item_activatable_interface_init (BtkActivatableIface *iface)
{
  iface->update = btk_tool_item_update;
  iface->sync_action_properties = btk_tool_item_sync_action_properties;
}

static void
btk_tool_item_update (BtkActivatable *activatable,
		      BtkAction      *action,
	     	      const gchar    *property_name)
{
  if (strcmp (property_name, "visible") == 0)
    {
      if (btk_action_is_visible (action))
	btk_widget_show (BTK_WIDGET (activatable));
      else
	btk_widget_hide (BTK_WIDGET (activatable));
    }
  else if (strcmp (property_name, "sensitive") == 0)
    btk_widget_set_sensitive (BTK_WIDGET (activatable), btk_action_is_sensitive (action));
  else if (strcmp (property_name, "tooltip") == 0)
    btk_tool_item_set_tooltip_text (BTK_TOOL_ITEM (activatable),
				    btk_action_get_tooltip (action));
  else if (strcmp (property_name, "visible-horizontal") == 0)
    btk_tool_item_set_visible_horizontal (BTK_TOOL_ITEM (activatable),
					  btk_action_get_visible_horizontal (action));
  else if (strcmp (property_name, "visible-vertical") == 0)
    btk_tool_item_set_visible_vertical (BTK_TOOL_ITEM (activatable),
					btk_action_get_visible_vertical (action));
  else if (strcmp (property_name, "is-important") == 0)
    btk_tool_item_set_is_important (BTK_TOOL_ITEM (activatable),
				    btk_action_get_is_important (action));
}

static void
btk_tool_item_sync_action_properties (BtkActivatable *activatable,
				      BtkAction      *action)
{
  if (!action)
    return;

  if (btk_action_is_visible (action))
    btk_widget_show (BTK_WIDGET (activatable));
  else
    btk_widget_hide (BTK_WIDGET (activatable));
  
  btk_widget_set_sensitive (BTK_WIDGET (activatable), btk_action_is_sensitive (action));
  
  btk_tool_item_set_tooltip_text (BTK_TOOL_ITEM (activatable),
				  btk_action_get_tooltip (action));
  btk_tool_item_set_visible_horizontal (BTK_TOOL_ITEM (activatable),
					btk_action_get_visible_horizontal (action));
  btk_tool_item_set_visible_vertical (BTK_TOOL_ITEM (activatable),
				      btk_action_get_visible_vertical (action));
  btk_tool_item_set_is_important (BTK_TOOL_ITEM (activatable),
				  btk_action_get_is_important (action));
}

static void
btk_tool_item_set_related_action (BtkToolItem *item, 
				  BtkAction   *action)
{
  if (item->priv->action == action)
    return;

  btk_activatable_do_set_related_action (BTK_ACTIVATABLE (item), action);

  item->priv->action = action;

  if (action)
    {
      btk_tool_item_rebuild_menu (item);
    }
}

static void
btk_tool_item_set_use_action_appearance (BtkToolItem *item,
					 gboolean     use_appearance)
{
  if (item->priv->use_action_appearance != use_appearance)
    {
      item->priv->use_action_appearance = use_appearance;

      btk_activatable_sync_action_properties (BTK_ACTIVATABLE (item), item->priv->action);
    }
}


/**
 * btk_tool_item_new:
 * 
 * Creates a new #BtkToolItem
 * 
 * Return value: the new #BtkToolItem
 * 
 * Since: 2.4
 **/
BtkToolItem *
btk_tool_item_new (void)
{
  BtkToolItem *item;

  item = g_object_new (BTK_TYPE_TOOL_ITEM, NULL);

  return item;
}

/**
 * btk_tool_item_get_ellipsize_mode:
 * @tool_item: a #BtkToolItem
 *
 * Returns the ellipsize mode used for @tool_item. Custom subclasses of
 * #BtkToolItem should call this function to find out how text should
 * be ellipsized.
 *
 * Return value: a #BangoEllipsizeMode indicating how text in @tool_item
 * should be ellipsized.
 *
 * Since: 2.20
 **/
BangoEllipsizeMode
btk_tool_item_get_ellipsize_mode (BtkToolItem *tool_item)
{
  BtkWidget *parent;
  
  g_return_val_if_fail (BTK_IS_TOOL_ITEM (tool_item), BTK_ORIENTATION_HORIZONTAL);

  parent = BTK_WIDGET (tool_item)->parent;
  if (!parent || !BTK_IS_TOOL_SHELL (parent))
    return BANGO_ELLIPSIZE_NONE;

  return btk_tool_shell_get_ellipsize_mode (BTK_TOOL_SHELL (parent));
}

/**
 * btk_tool_item_get_icon_size:
 * @tool_item: a #BtkToolItem
 * 
 * Returns the icon size used for @tool_item. Custom subclasses of
 * #BtkToolItem should call this function to find out what size icons
 * they should use.
 * 
 * Return value: (type int): a #BtkIconSize indicating the icon size
 * used for @tool_item
 * 
 * Since: 2.4
 **/
BtkIconSize
btk_tool_item_get_icon_size (BtkToolItem *tool_item)
{
  BtkWidget *parent;

  g_return_val_if_fail (BTK_IS_TOOL_ITEM (tool_item), BTK_ICON_SIZE_LARGE_TOOLBAR);

  parent = BTK_WIDGET (tool_item)->parent;
  if (!parent || !BTK_IS_TOOL_SHELL (parent))
    return BTK_ICON_SIZE_LARGE_TOOLBAR;

  return btk_tool_shell_get_icon_size (BTK_TOOL_SHELL (parent));
}

/**
 * btk_tool_item_get_orientation:
 * @tool_item: a #BtkToolItem 
 * 
 * Returns the orientation used for @tool_item. Custom subclasses of
 * #BtkToolItem should call this function to find out what size icons
 * they should use.
 * 
 * Return value: a #BtkOrientation indicating the orientation
 * used for @tool_item
 * 
 * Since: 2.4
 **/
BtkOrientation
btk_tool_item_get_orientation (BtkToolItem *tool_item)
{
  BtkWidget *parent;
  
  g_return_val_if_fail (BTK_IS_TOOL_ITEM (tool_item), BTK_ORIENTATION_HORIZONTAL);

  parent = BTK_WIDGET (tool_item)->parent;
  if (!parent || !BTK_IS_TOOL_SHELL (parent))
    return BTK_ORIENTATION_HORIZONTAL;

  return btk_tool_shell_get_orientation (BTK_TOOL_SHELL (parent));
}

/**
 * btk_tool_item_get_toolbar_style:
 * @tool_item: a #BtkToolItem 
 * 
 * Returns the toolbar style used for @tool_item. Custom subclasses of
 * #BtkToolItem should call this function in the handler of the
 * BtkToolItem::toolbar_reconfigured signal to find out in what style
 * the toolbar is displayed and change themselves accordingly 
 *
 * Possibilities are:
 * <itemizedlist>
 * <listitem> BTK_TOOLBAR_BOTH, meaning the tool item should show
 * both an icon and a label, stacked vertically </listitem>
 * <listitem> BTK_TOOLBAR_ICONS, meaning the toolbar shows
 * only icons </listitem>
 * <listitem> BTK_TOOLBAR_TEXT, meaning the tool item should only
 * show text</listitem>
 * <listitem> BTK_TOOLBAR_BOTH_HORIZ, meaning the tool item should show
 * both an icon and a label, arranged horizontally (however, note the 
 * #BtkToolButton::has_text_horizontally that makes tool buttons not
 * show labels when the toolbar style is BTK_TOOLBAR_BOTH_HORIZ.
 * </listitem>
 * </itemizedlist>
 * 
 * Return value: A #BtkToolbarStyle indicating the toolbar style used
 * for @tool_item.
 * 
 * Since: 2.4
 **/
BtkToolbarStyle
btk_tool_item_get_toolbar_style (BtkToolItem *tool_item)
{
  BtkWidget *parent;
  
  g_return_val_if_fail (BTK_IS_TOOL_ITEM (tool_item), BTK_TOOLBAR_ICONS);

  parent = BTK_WIDGET (tool_item)->parent;
  if (!parent || !BTK_IS_TOOL_SHELL (parent))
    return BTK_TOOLBAR_ICONS;

  return btk_tool_shell_get_style (BTK_TOOL_SHELL (parent));
}

/**
 * btk_tool_item_get_relief_style:
 * @tool_item: a #BtkToolItem 
 * 
 * Returns the relief style of @tool_item. See btk_button_set_relief_style().
 * Custom subclasses of #BtkToolItem should call this function in the handler
 * of the #BtkToolItem::toolbar_reconfigured signal to find out the
 * relief style of buttons.
 * 
 * Return value: a #BtkReliefStyle indicating the relief style used
 * for @tool_item.
 * 
 * Since: 2.4
 **/
BtkReliefStyle 
btk_tool_item_get_relief_style (BtkToolItem *tool_item)
{
  BtkWidget *parent;
  
  g_return_val_if_fail (BTK_IS_TOOL_ITEM (tool_item), BTK_RELIEF_NONE);

  parent = BTK_WIDGET (tool_item)->parent;
  if (!parent || !BTK_IS_TOOL_SHELL (parent))
    return BTK_RELIEF_NONE;

  return btk_tool_shell_get_relief_style (BTK_TOOL_SHELL (parent));
}

/**
 * btk_tool_item_get_text_alignment:
 * @tool_item: a #BtkToolItem: 
 * 
 * Returns the text alignment used for @tool_item. Custom subclasses of
 * #BtkToolItem should call this function to find out how text should
 * be aligned.
 * 
 * Return value: a #gfloat indicating the horizontal text alignment
 * used for @tool_item
 * 
 * Since: 2.20
 **/
gfloat
btk_tool_item_get_text_alignment (BtkToolItem *tool_item)
{
  BtkWidget *parent;
  
  g_return_val_if_fail (BTK_IS_TOOL_ITEM (tool_item), BTK_ORIENTATION_HORIZONTAL);

  parent = BTK_WIDGET (tool_item)->parent;
  if (!parent || !BTK_IS_TOOL_SHELL (parent))
    return 0.5;

  return btk_tool_shell_get_text_alignment (BTK_TOOL_SHELL (parent));
}

/**
 * btk_tool_item_get_text_orientation:
 * @tool_item: a #BtkToolItem
 *
 * Returns the text orientation used for @tool_item. Custom subclasses of
 * #BtkToolItem should call this function to find out how text should
 * be orientated.
 *
 * Return value: a #BtkOrientation indicating the text orientation
 * used for @tool_item
 *
 * Since: 2.20
 */
BtkOrientation
btk_tool_item_get_text_orientation (BtkToolItem *tool_item)
{
  BtkWidget *parent;
  
  g_return_val_if_fail (BTK_IS_TOOL_ITEM (tool_item), BTK_ORIENTATION_HORIZONTAL);

  parent = BTK_WIDGET (tool_item)->parent;
  if (!parent || !BTK_IS_TOOL_SHELL (parent))
    return BTK_ORIENTATION_HORIZONTAL;

  return btk_tool_shell_get_text_orientation (BTK_TOOL_SHELL (parent));
}

/**
 * btk_tool_item_get_text_size_group:
 * @tool_item: a #BtkToolItem
 *
 * Returns the size group used for labels in @tool_item.
 * Custom subclasses of #BtkToolItem should call this function
 * and use the size group for labels.
 *
 * Return value: (transfer none): a #BtkSizeGroup
 *
 * Since: 2.20
 */
BtkSizeGroup *
btk_tool_item_get_text_size_group (BtkToolItem *tool_item)
{
  BtkWidget *parent;
  
  g_return_val_if_fail (BTK_IS_TOOL_ITEM (tool_item), NULL);

  parent = BTK_WIDGET (tool_item)->parent;
  if (!parent || !BTK_IS_TOOL_SHELL (parent))
    return NULL;

  return btk_tool_shell_get_text_size_group (BTK_TOOL_SHELL (parent));
}

/**
 * btk_tool_item_set_expand:
 * @tool_item: a #BtkToolItem
 * @expand: Whether @tool_item is allocated extra space
 *
 * Sets whether @tool_item is allocated extra space when there
 * is more room on the toolbar then needed for the items. The
 * effect is that the item gets bigger when the toolbar gets bigger
 * and smaller when the toolbar gets smaller.
 *
 * Since: 2.4
 */
void
btk_tool_item_set_expand (BtkToolItem *tool_item,
			  gboolean     expand)
{
  g_return_if_fail (BTK_IS_TOOL_ITEM (tool_item));
    
  expand = expand != FALSE;

  if (tool_item->priv->expand != expand)
    {
      tool_item->priv->expand = expand;
      btk_widget_child_notify (BTK_WIDGET (tool_item), "expand");
      btk_widget_queue_resize (BTK_WIDGET (tool_item));
    }
}

/**
 * btk_tool_item_get_expand:
 * @tool_item: a #BtkToolItem 
 * 
 * Returns whether @tool_item is allocated extra space.
 * See btk_tool_item_set_expand().
 * 
 * Return value: %TRUE if @tool_item is allocated extra space.
 * 
 * Since: 2.4
 **/
gboolean
btk_tool_item_get_expand (BtkToolItem *tool_item)
{
  g_return_val_if_fail (BTK_IS_TOOL_ITEM (tool_item), FALSE);

  return tool_item->priv->expand;
}

/**
 * btk_tool_item_set_homogeneous:
 * @tool_item: a #BtkToolItem 
 * @homogeneous: whether @tool_item is the same size as other homogeneous items
 * 
 * Sets whether @tool_item is to be allocated the same size as other
 * homogeneous items. The effect is that all homogeneous items will have
 * the same width as the widest of the items.
 * 
 * Since: 2.4
 **/
void
btk_tool_item_set_homogeneous (BtkToolItem *tool_item,
			       gboolean     homogeneous)
{
  g_return_if_fail (BTK_IS_TOOL_ITEM (tool_item));
    
  homogeneous = homogeneous != FALSE;

  if (tool_item->priv->homogeneous != homogeneous)
    {
      tool_item->priv->homogeneous = homogeneous;
      btk_widget_child_notify (BTK_WIDGET (tool_item), "homogeneous");
      btk_widget_queue_resize (BTK_WIDGET (tool_item));
    }
}

/**
 * btk_tool_item_get_homogeneous:
 * @tool_item: a #BtkToolItem 
 * 
 * Returns whether @tool_item is the same size as other homogeneous
 * items. See btk_tool_item_set_homogeneous().
 * 
 * Return value: %TRUE if the item is the same size as other homogeneous
 * items.
 * 
 * Since: 2.4
 **/
gboolean
btk_tool_item_get_homogeneous (BtkToolItem *tool_item)
{
  g_return_val_if_fail (BTK_IS_TOOL_ITEM (tool_item), FALSE);

  return tool_item->priv->homogeneous;
}

/**
 * btk_tool_item_get_is_important:
 * @tool_item: a #BtkToolItem
 * 
 * Returns whether @tool_item is considered important. See
 * btk_tool_item_set_is_important()
 * 
 * Return value: %TRUE if @tool_item is considered important.
 * 
 * Since: 2.4
 **/
gboolean
btk_tool_item_get_is_important (BtkToolItem *tool_item)
{
  g_return_val_if_fail (BTK_IS_TOOL_ITEM (tool_item), FALSE);

  return tool_item->priv->is_important;
}

/**
 * btk_tool_item_set_is_important:
 * @tool_item: a #BtkToolItem
 * @is_important: whether the tool item should be considered important
 * 
 * Sets whether @tool_item should be considered important. The #BtkToolButton
 * class uses this property to determine whether to show or hide its label
 * when the toolbar style is %BTK_TOOLBAR_BOTH_HORIZ. The result is that
 * only tool buttons with the "is_important" property set have labels, an
 * effect known as "priority text"
 * 
 * Since: 2.4
 **/
void
btk_tool_item_set_is_important (BtkToolItem *tool_item, gboolean is_important)
{
  g_return_if_fail (BTK_IS_TOOL_ITEM (tool_item));

  is_important = is_important != FALSE;

  if (is_important != tool_item->priv->is_important)
    {
      tool_item->priv->is_important = is_important;

      btk_widget_queue_resize (BTK_WIDGET (tool_item));

      g_object_notify (B_OBJECT (tool_item), "is-important");
    }
}

static gboolean
btk_tool_item_real_set_tooltip (BtkToolItem *tool_item,
				BtkTooltips *tooltips,
				const gchar *tip_text,
				const gchar *tip_private)
{
  BtkWidget *child = BTK_BIN (tool_item)->child;

  if (!child)
    return FALSE;

  btk_widget_set_tooltip_text (child, tip_text);

  return TRUE;
}

/**
 * btk_tool_item_set_tooltip:
 * @tool_item: a #BtkToolItem
 * @tooltips: The #BtkTooltips object to be used
 * @tip_text: (allow-none): text to be used as tooltip text for @tool_item
 * @tip_private: (allow-none): text to be used as private tooltip text
 *
 * Sets the #BtkTooltips object to be used for @tool_item, the
 * text to be displayed as tooltip on the item and the private text
 * to be used. See btk_tooltips_set_tip().
 * 
 * Since: 2.4
 *
 * Deprecated: 2.12: Use btk_tool_item_set_tooltip_text() instead.
 **/
void
btk_tool_item_set_tooltip (BtkToolItem *tool_item,
			   BtkTooltips *tooltips,
			   const gchar *tip_text,
			   const gchar *tip_private)
{
  gboolean retval;
  
  g_return_if_fail (BTK_IS_TOOL_ITEM (tool_item));

  g_signal_emit (tool_item, toolitem_signals[SET_TOOLTIP], 0,
		 tooltips, tip_text, tip_private, &retval);
}

/**
 * btk_tool_item_set_tooltip_text:
 * @tool_item: a #BtkToolItem 
 * @text: text to be used as tooltip for @tool_item
 *
 * Sets the text to be displayed as tooltip on the item.
 * See btk_widget_set_tooltip_text().
 *
 * Since: 2.12
 **/
void
btk_tool_item_set_tooltip_text (BtkToolItem *tool_item,
			        const gchar *text)
{
  BtkWidget *child;

  g_return_if_fail (BTK_IS_TOOL_ITEM (tool_item));

  child = BTK_BIN (tool_item)->child;

  if (child)
    btk_widget_set_tooltip_text (child, text);
}

/**
 * btk_tool_item_set_tooltip_markup:
 * @tool_item: a #BtkToolItem 
 * @markup: markup text to be used as tooltip for @tool_item
 *
 * Sets the markup text to be displayed as tooltip on the item.
 * See btk_widget_set_tooltip_markup().
 *
 * Since: 2.12
 **/
void
btk_tool_item_set_tooltip_markup (BtkToolItem *tool_item,
				  const gchar *markup)
{
  BtkWidget *child;

  g_return_if_fail (BTK_IS_TOOL_ITEM (tool_item));

  child = BTK_BIN (tool_item)->child;

  if (child)
    btk_widget_set_tooltip_markup (child, markup);
}

/**
 * btk_tool_item_set_use_drag_window:
 * @tool_item: a #BtkToolItem 
 * @use_drag_window: Whether @tool_item has a drag window.
 * 
 * Sets whether @tool_item has a drag window. When %TRUE the
 * toolitem can be used as a drag source through btk_drag_source_set().
 * When @tool_item has a drag window it will intercept all events,
 * even those that would otherwise be sent to a child of @tool_item.
 * 
 * Since: 2.4
 **/
void
btk_tool_item_set_use_drag_window (BtkToolItem *toolitem,
				   gboolean     use_drag_window)
{
  g_return_if_fail (BTK_IS_TOOL_ITEM (toolitem));

  use_drag_window = use_drag_window != FALSE;

  if (toolitem->priv->use_drag_window != use_drag_window)
    {
      toolitem->priv->use_drag_window = use_drag_window;
      
      if (use_drag_window)
	{
	  if (!toolitem->priv->drag_window &&
              btk_widget_get_realized (BTK_WIDGET (toolitem)))
	    {
	      create_drag_window(toolitem);
	      if (btk_widget_get_mapped (BTK_WIDGET (toolitem)))
		bdk_window_show (toolitem->priv->drag_window);
	    }
	}
      else
	{
	  destroy_drag_window (toolitem);
	}
    }
}

/**
 * btk_tool_item_get_use_drag_window:
 * @tool_item: a #BtkToolItem 
 * 
 * Returns whether @tool_item has a drag window. See
 * btk_tool_item_set_use_drag_window().
 * 
 * Return value: %TRUE if @tool_item uses a drag window.
 * 
 * Since: 2.4
 **/
gboolean
btk_tool_item_get_use_drag_window (BtkToolItem *toolitem)
{
  g_return_val_if_fail (BTK_IS_TOOL_ITEM (toolitem), FALSE);

  return toolitem->priv->use_drag_window;
}

/**
 * btk_tool_item_set_visible_horizontal:
 * @tool_item: a #BtkToolItem
 * @visible_horizontal: Whether @tool_item is visible when in horizontal mode
 * 
 * Sets whether @tool_item is visible when the toolbar is docked horizontally.
 * 
 * Since: 2.4
 **/
void
btk_tool_item_set_visible_horizontal (BtkToolItem *toolitem,
				      gboolean     visible_horizontal)
{
  g_return_if_fail (BTK_IS_TOOL_ITEM (toolitem));

  visible_horizontal = visible_horizontal != FALSE;

  if (toolitem->priv->visible_horizontal != visible_horizontal)
    {
      toolitem->priv->visible_horizontal = visible_horizontal;

      g_object_notify (B_OBJECT (toolitem), "visible-horizontal");

      btk_widget_queue_resize (BTK_WIDGET (toolitem));
    }
}

/**
 * btk_tool_item_get_visible_horizontal:
 * @tool_item: a #BtkToolItem 
 * 
 * Returns whether the @tool_item is visible on toolbars that are
 * docked horizontally.
 * 
 * Return value: %TRUE if @tool_item is visible on toolbars that are
 * docked horizontally.
 * 
 * Since: 2.4
 **/
gboolean
btk_tool_item_get_visible_horizontal (BtkToolItem *toolitem)
{
  g_return_val_if_fail (BTK_IS_TOOL_ITEM (toolitem), FALSE);

  return toolitem->priv->visible_horizontal;
}

/**
 * btk_tool_item_set_visible_vertical:
 * @tool_item: a #BtkToolItem 
 * @visible_vertical: whether @tool_item is visible when the toolbar
 * is in vertical mode
 *
 * Sets whether @tool_item is visible when the toolbar is docked
 * vertically. Some tool items, such as text entries, are too wide to be
 * useful on a vertically docked toolbar. If @visible_vertical is %FALSE
 * @tool_item will not appear on toolbars that are docked vertically.
 * 
 * Since: 2.4
 **/
void
btk_tool_item_set_visible_vertical (BtkToolItem *toolitem,
				    gboolean     visible_vertical)
{
  g_return_if_fail (BTK_IS_TOOL_ITEM (toolitem));

  visible_vertical = visible_vertical != FALSE;

  if (toolitem->priv->visible_vertical != visible_vertical)
    {
      toolitem->priv->visible_vertical = visible_vertical;

      g_object_notify (B_OBJECT (toolitem), "visible-vertical");

      btk_widget_queue_resize (BTK_WIDGET (toolitem));
    }
}

/**
 * btk_tool_item_get_visible_vertical:
 * @tool_item: a #BtkToolItem 
 * 
 * Returns whether @tool_item is visible when the toolbar is docked vertically.
 * See btk_tool_item_set_visible_vertical().
 * 
 * Return value: Whether @tool_item is visible when the toolbar is docked vertically
 * 
 * Since: 2.4
 **/
gboolean
btk_tool_item_get_visible_vertical (BtkToolItem *toolitem)
{
  g_return_val_if_fail (BTK_IS_TOOL_ITEM (toolitem), FALSE);

  return toolitem->priv->visible_vertical;
}

/**
 * btk_tool_item_retrieve_proxy_menu_item:
 * @tool_item: a #BtkToolItem 
 * 
 * Returns the #BtkMenuItem that was last set by
 * btk_tool_item_set_proxy_menu_item(), ie. the #BtkMenuItem
 * that is going to appear in the overflow menu.
 *
 * Return value: (transfer none): The #BtkMenuItem that is going to appear in the
 * overflow menu for @tool_item.
 *
 * Since: 2.4
 **/
BtkWidget *
btk_tool_item_retrieve_proxy_menu_item (BtkToolItem *tool_item)
{
  gboolean retval;
  
  g_return_val_if_fail (BTK_IS_TOOL_ITEM (tool_item), NULL);

  g_signal_emit (tool_item, toolitem_signals[CREATE_MENU_PROXY], 0,
		 &retval);
  
  return tool_item->priv->menu_item;
}

/**
 * btk_tool_item_get_proxy_menu_item:
 * @tool_item: a #BtkToolItem
 * @menu_item_id: a string used to identify the menu item
 *
 * If @menu_item_id matches the string passed to
 * btk_tool_item_set_proxy_menu_item() return the corresponding #BtkMenuItem.
 *
 * Custom subclasses of #BtkToolItem should use this function to
 * update their menu item when the #BtkToolItem changes. That the
 * @menu_item_id<!-- -->s must match ensures that a #BtkToolItem
 * will not inadvertently change a menu item that they did not create.
 *
 * Return value: (transfer none): The #BtkMenuItem passed to
 *     btk_tool_item_set_proxy_menu_item(), if the @menu_item_id<!-- -->s
 *     match.
 *
 * Since: 2.4
 **/
BtkWidget *
btk_tool_item_get_proxy_menu_item (BtkToolItem *tool_item,
				   const gchar *menu_item_id)
{
  g_return_val_if_fail (BTK_IS_TOOL_ITEM (tool_item), NULL);
  g_return_val_if_fail (menu_item_id != NULL, NULL);

  if (tool_item->priv->menu_item_id && strcmp (tool_item->priv->menu_item_id, menu_item_id) == 0)
    return tool_item->priv->menu_item;

  return NULL;
}

/**
 * btk_tool_item_rebuild_menu:
 * @tool_item: a #BtkToolItem
 *
 * Calling this function signals to the toolbar that the
 * overflow menu item for @tool_item has changed. If the
 * overflow menu is visible when this function it called,
 * the menu will be rebuilt.
 *
 * The function must be called when the tool item changes what it
 * will do in response to the #BtkToolItem::create-menu-proxy signal.
 *
 * Since: 2.6
 */
void
btk_tool_item_rebuild_menu (BtkToolItem *tool_item)
{
  BtkWidget *widget;
  
  g_return_if_fail (BTK_IS_TOOL_ITEM (tool_item));

  widget = BTK_WIDGET (tool_item);

  if (BTK_IS_TOOL_SHELL (widget->parent))
    btk_tool_shell_rebuild_menu (BTK_TOOL_SHELL (widget->parent));
}

/**
 * btk_tool_item_set_proxy_menu_item:
 * @tool_item: a #BtkToolItem
 * @menu_item_id: a string used to identify @menu_item
 * @menu_item: a #BtkMenuItem to be used in the overflow menu
 * 
 * Sets the #BtkMenuItem used in the toolbar overflow menu. The
 * @menu_item_id is used to identify the caller of this function and
 * should also be used with btk_tool_item_get_proxy_menu_item().
 *
 * See also #BtkToolItem::create-menu-proxy.
 * 
 * Since: 2.4
 **/
void
btk_tool_item_set_proxy_menu_item (BtkToolItem *tool_item,
				   const gchar *menu_item_id,
				   BtkWidget   *menu_item)
{
  g_return_if_fail (BTK_IS_TOOL_ITEM (tool_item));
  g_return_if_fail (menu_item == NULL || BTK_IS_MENU_ITEM (menu_item));
  g_return_if_fail (menu_item_id != NULL);

  g_free (tool_item->priv->menu_item_id);
      
  tool_item->priv->menu_item_id = g_strdup (menu_item_id);

  if (tool_item->priv->menu_item != menu_item)
    {
      if (tool_item->priv->menu_item)
	g_object_unref (tool_item->priv->menu_item);
      
      if (menu_item)
	{
	  g_object_ref_sink (menu_item);

	  btk_widget_set_sensitive (menu_item,
				    btk_widget_get_sensitive (BTK_WIDGET (tool_item)));
	}
      
      tool_item->priv->menu_item = menu_item;
    }
}

/**
 * btk_tool_item_toolbar_reconfigured:
 * @tool_item: a #BtkToolItem
 *
 * Emits the signal #BtkToolItem::toolbar_reconfigured on @tool_item.
 * #BtkToolbar and other #BtkToolShell implementations use this function
 * to notify children, when some aspect of their configuration changes.
 *
 * Since: 2.14
 **/
void
btk_tool_item_toolbar_reconfigured (BtkToolItem *tool_item)
{
  /* The slightely inaccurate name "btk_tool_item_toolbar_reconfigured" was
   * choosen over "btk_tool_item_tool_shell_reconfigured", since the function
   * emits the "toolbar-reconfigured" signal, not "tool-shell-reconfigured".
   * Its not possible to rename the signal, and emitting another name than
   * indicated by the function name would be quite confusing. That's the
   * price of providing stable APIs.
   */
  g_return_if_fail (BTK_IS_TOOL_ITEM (tool_item));

  g_signal_emit (tool_item, toolitem_signals[TOOLBAR_RECONFIGURED], 0);
  
  if (tool_item->priv->drag_window)
    bdk_window_raise (tool_item->priv->drag_window);

  btk_widget_queue_resize (BTK_WIDGET (tool_item));
}

#define __BTK_TOOL_ITEM_C__
#include "btkaliasdef.c"
