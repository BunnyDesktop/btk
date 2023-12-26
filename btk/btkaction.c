/*
 * BTK - The GIMP Toolkit
 * Copyright (C) 1998, 1999 Red Hat, Inc.
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Bunny Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Author: James Henstridge <james@daa.com.au>
 *
 * Modified by the BTK+ Team and others 2003.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

/**
 * SECTION:btkaction
 * @Short_description: An action which can be triggered by a menu or toolbar item
 * @Title: BtkAction
 * @See_also: #BtkActionGroup, #BtkUIManager
 *
 * Actions represent operations that the user can be perform, along with
 * some information how it should be presented in the interface. Each action
 * provides methods to create icons, menu items and toolbar items
 * representing itself.
 *
 * As well as the callback that is called when the action gets activated,
 * the following also gets associated with the action:
 * <itemizedlist>
 *   <listitem><para>a name (not translated, for path lookup)</para></listitem>
 *   <listitem><para>a label (translated, for display)</para></listitem>
 *   <listitem><para>an accelerator</para></listitem>
 *   <listitem><para>whether label indicates a stock id</para></listitem>
 *   <listitem><para>a tooltip (optional, translated)</para></listitem>
 *   <listitem><para>a toolbar label (optional, shorter than label)</para></listitem>
 * </itemizedlist>
 * The action will also have some state information:
 * <itemizedlist>
 *   <listitem><para>visible (shown/hidden)</para></listitem>
 *   <listitem><para>sensitive (enabled/disabled)</para></listitem>
 * </itemizedlist>
 * Apart from regular actions, there are <link linkend="BtkToggleAction">toggle
 * actions</link>, which can be toggled between two states and <link
 * linkend="BtkRadioAction">radio actions</link>, of which only one in a group
 * can be in the "active" state. Other actions can be implemented as #BtkAction
 * subclasses.
 *
 * Each action can have one or more proxy menu item, toolbar button or
 * other proxy widgets.  Proxies mirror the state of the action (text
 * label, tooltip, icon, visible, sensitive, etc), and should change when
 * the action's state changes. When the proxy is activated, it should
 * activate its action.
 */

#include "config.h"

#include "btkaction.h"
#include "btkactiongroup.h"
#include "btkaccellabel.h"
#include "btkbutton.h"
#include "btkiconfactory.h"
#include "btkimage.h"
#include "btkimagemenuitem.h"
#include "btkintl.h"
#include "btklabel.h"
#include "btkmarshalers.h"
#include "btkmenuitem.h"
#include "btkstock.h"
#include "btktearoffmenuitem.h"
#include "btktoolbutton.h"
#include "btktoolbar.h"
#include "btkprivate.h"
#include "btkbuildable.h"
#include "btkactivatable.h"
#include "btkalias.h"


#define BTK_ACTION_GET_PRIVATE(obj) (B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_ACTION, BtkActionPrivate))

struct _BtkActionPrivate 
{
  const gchar *name; /* interned */
  gchar *label;
  gchar *short_label;
  gchar *tooltip;
  gchar *stock_id; /* stock icon */
  gchar *icon_name; /* themed icon */
  GIcon *gicon;

  guint sensitive          : 1;
  guint visible            : 1;
  guint label_set          : 1; /* these two used so we can set label */
  guint short_label_set    : 1; /* based on stock id */
  guint visible_horizontal : 1;
  guint visible_vertical   : 1;
  guint is_important       : 1;
  guint hide_if_empty      : 1;
  guint visible_overflown  : 1;
  guint always_show_image  : 1;
  guint recursion_guard    : 1;
  guint activate_blocked   : 1;

  /* accelerator */
  guint          accel_count;
  BtkAccelGroup *accel_group;
  GClosure      *accel_closure;
  GQuark         accel_quark;

  BtkActionGroup *action_group;

  /* list of proxy widgets */
  GSList *proxies;
};

enum 
{
  ACTIVATE,
  LAST_SIGNAL
};

enum 
{
  PROP_0,
  PROP_NAME,
  PROP_LABEL,
  PROP_SHORT_LABEL,
  PROP_TOOLTIP,
  PROP_STOCK_ID,
  PROP_ICON_NAME,
  PROP_GICON,
  PROP_VISIBLE_HORIZONTAL,
  PROP_VISIBLE_VERTICAL,
  PROP_VISIBLE_OVERFLOWN,
  PROP_IS_IMPORTANT,
  PROP_HIDE_IF_EMPTY,
  PROP_SENSITIVE,
  PROP_VISIBLE,
  PROP_ACTION_GROUP,
  PROP_ALWAYS_SHOW_IMAGE
};

/* BtkBuildable */
static void btk_action_buildable_init             (BtkBuildableIface *iface);
static void btk_action_buildable_set_name         (BtkBuildable *buildable,
						   const gchar  *name);
static const gchar* btk_action_buildable_get_name (BtkBuildable *buildable);

G_DEFINE_TYPE_WITH_CODE (BtkAction, btk_action, B_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
						btk_action_buildable_init))

static void btk_action_finalize     (BObject *object);
static void btk_action_set_property (BObject         *object,
				     guint            prop_id,
				     const BValue    *value,
				     BParamSpec      *pspec);
static void btk_action_get_property (BObject         *object,
				     guint            prop_id,
				     BValue          *value,
				     BParamSpec      *pspec);
static void btk_action_set_action_group (BtkAction	*action,
					 BtkActionGroup *action_group);

static BtkWidget *create_menu_item    (BtkAction *action);
static BtkWidget *create_tool_item    (BtkAction *action);
static void       connect_proxy       (BtkAction *action,
				       BtkWidget *proxy);
static void       disconnect_proxy    (BtkAction *action,
				       BtkWidget *proxy);
 
static void       closure_accel_activate (GClosure     *closure,
					  BValue       *return_value,
					  guint         n_param_values,
					  const BValue *param_values,
					  gpointer      invocation_hint,
					  gpointer      marshal_data);

static guint         action_signals[LAST_SIGNAL] = { 0 };


static void
btk_action_class_init (BtkActionClass *klass)
{
  BObjectClass *bobject_class;

  bobject_class = B_OBJECT_CLASS (klass);

  bobject_class->finalize     = btk_action_finalize;
  bobject_class->set_property = btk_action_set_property;
  bobject_class->get_property = btk_action_get_property;

  klass->activate = NULL;

  klass->create_menu_item  = create_menu_item;
  klass->create_tool_item  = create_tool_item;
  klass->create_menu       = NULL;
  klass->menu_item_type    = BTK_TYPE_IMAGE_MENU_ITEM;
  klass->toolbar_item_type = BTK_TYPE_TOOL_BUTTON;
  klass->connect_proxy    = connect_proxy;
  klass->disconnect_proxy = disconnect_proxy;

  g_object_class_install_property (bobject_class,
				   PROP_NAME,
				   g_param_spec_string ("name",
							P_("Name"),
							P_("A unique name for the action."),
							NULL,
							BTK_PARAM_READWRITE | 
							G_PARAM_CONSTRUCT_ONLY));

  /**
   * BtkAction:label:
   *
   * The label used for menu items and buttons that activate
   * this action. If the label is %NULL, BTK+ uses the stock 
   * label specified via the stock-id property.
   *
   * This is an appearance property and thus only applies if 
   * #BtkActivatable:use-action-appearance is %TRUE.
   */
  g_object_class_install_property (bobject_class,
				   PROP_LABEL,
				   g_param_spec_string ("label",
							P_("Label"),
							P_("The label used for menu items and buttons "
							   "that activate this action."),
							NULL,
							BTK_PARAM_READWRITE));

  /**
   * BtkAction:short-label:
   *
   * A shorter label that may be used on toolbar buttons.
   *
   * This is an appearance property and thus only applies if 
   * #BtkActivatable:use-action-appearance is %TRUE.
   */
  g_object_class_install_property (bobject_class,
				   PROP_SHORT_LABEL,
				   g_param_spec_string ("short-label",
							P_("Short label"),
							P_("A shorter label that may be used on toolbar buttons."),
							NULL,
							BTK_PARAM_READWRITE));


  g_object_class_install_property (bobject_class,
				   PROP_TOOLTIP,
				   g_param_spec_string ("tooltip",
							P_("Tooltip"),
							P_("A tooltip for this action."),
							NULL,
							BTK_PARAM_READWRITE));

  /**
   * BtkAction:stock-id:
   *
   * The stock icon displayed in widgets representing this action.
   *
   * This is an appearance property and thus only applies if 
   * #BtkActivatable:use-action-appearance is %TRUE.
   */
  g_object_class_install_property (bobject_class,
				   PROP_STOCK_ID,
				   g_param_spec_string ("stock-id",
							P_("Stock Icon"),
							P_("The stock icon displayed in widgets representing "
							   "this action."),
							NULL,
							BTK_PARAM_READWRITE));
  /**
   * BtkAction:gicon:
   *
   * The #GIcon displayed in the #BtkAction.
   *
   * Note that the stock icon is preferred, if the #BtkAction:stock-id 
   * property holds the id of an existing stock icon.
   *
   * This is an appearance property and thus only applies if 
   * #BtkActivatable:use-action-appearance is %TRUE.
   *
   * Since: 2.16
   */
  g_object_class_install_property (bobject_class,
				   PROP_GICON,
				   g_param_spec_object ("gicon",
							P_("GIcon"),
							P_("The GIcon being displayed"),
							B_TYPE_ICON,
 							BTK_PARAM_READWRITE));							
  /**
   * BtkAction:icon-name:
   *
   * The name of the icon from the icon theme. 
   * 
   * Note that the stock icon is preferred, if the #BtkAction:stock-id 
   * property holds the id of an existing stock icon, and the #GIcon is
   * preferred if the #BtkAction:gicon property is set. 
   *
   * This is an appearance property and thus only applies if 
   * #BtkActivatable:use-action-appearance is %TRUE.
   *
   * Since: 2.10
   */
  g_object_class_install_property (bobject_class,
				   PROP_ICON_NAME,
				   g_param_spec_string ("icon-name",
							P_("Icon Name"),
							P_("The name of the icon from the icon theme"),
							NULL,
 							BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
				   PROP_VISIBLE_HORIZONTAL,
				   g_param_spec_boolean ("visible-horizontal",
							 P_("Visible when horizontal"),
							 P_("Whether the toolbar item is visible when the toolbar "
							    "is in a horizontal orientation."),
							 TRUE,
							 BTK_PARAM_READWRITE));
  /**
   * BtkAction:visible-overflown:
   *
   * When %TRUE, toolitem proxies for this action are represented in the 
   * toolbar overflow menu.
   *
   * Since: 2.6
   */
  g_object_class_install_property (bobject_class,
				   PROP_VISIBLE_OVERFLOWN,
				   g_param_spec_boolean ("visible-overflown",
							 P_("Visible when overflown"),
							 P_("When TRUE, toolitem proxies for this action "
							    "are represented in the toolbar overflow menu."),
							 TRUE,
							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_VISIBLE_VERTICAL,
				   g_param_spec_boolean ("visible-vertical",
							 P_("Visible when vertical"),
							 P_("Whether the toolbar item is visible when the toolbar "
							    "is in a vertical orientation."),
							 TRUE,
							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_IS_IMPORTANT,
				   g_param_spec_boolean ("is-important",
							 P_("Is important"),
							 P_("Whether the action is considered important. "
							    "When TRUE, toolitem proxies for this action "
							    "show text in BTK_TOOLBAR_BOTH_HORIZ mode."),
							 FALSE,
							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_HIDE_IF_EMPTY,
				   g_param_spec_boolean ("hide-if-empty",
							 P_("Hide if empty"),
							 P_("When TRUE, empty menu proxies for this action are hidden."),
							 TRUE,
							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_SENSITIVE,
				   g_param_spec_boolean ("sensitive",
							 P_("Sensitive"),
							 P_("Whether the action is enabled."),
							 TRUE,
							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_VISIBLE,
				   g_param_spec_boolean ("visible",
							 P_("Visible"),
							 P_("Whether the action is visible."),
							 TRUE,
							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_ACTION_GROUP,
				   g_param_spec_object ("action-group",
							 P_("Action Group"),
							 P_("The BtkActionGroup this BtkAction is associated with, or NULL (for internal use)."),
							 BTK_TYPE_ACTION_GROUP,
							 BTK_PARAM_READWRITE));

  /**
   * BtkAction:always-show-image:
   *
   * If %TRUE, the action's menu item proxies will ignore the #BtkSettings:btk-menu-images 
   * setting and always show their image, if available.
   *
   * Use this property if the menu item would be useless or hard to use
   * without their image. 
   *
   * Since: 2.20
   **/
  g_object_class_install_property (bobject_class,
                                   PROP_ALWAYS_SHOW_IMAGE,
                                   g_param_spec_boolean ("always-show-image",
                                                         P_("Always show image"),
                                                         P_("Whether the image will always be shown"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  /**
   * BtkAction::activate:
   * @action: the #BtkAction
   *
   * The "activate" signal is emitted when the action is activated.
   *
   * Since: 2.4
   */
  action_signals[ACTIVATE] =
    g_signal_new (I_("activate"),
		  B_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
		  G_STRUCT_OFFSET (BtkActionClass, activate),  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);

  g_type_class_add_private (bobject_class, sizeof (BtkActionPrivate));
}


static void
btk_action_init (BtkAction *action)
{
  action->private_data = BTK_ACTION_GET_PRIVATE (action);

  action->private_data->name = NULL;
  action->private_data->label = NULL;
  action->private_data->short_label = NULL;
  action->private_data->tooltip = NULL;
  action->private_data->stock_id = NULL;
  action->private_data->icon_name = NULL;
  action->private_data->visible_horizontal = TRUE;
  action->private_data->visible_vertical   = TRUE;
  action->private_data->visible_overflown  = TRUE;
  action->private_data->is_important = FALSE;
  action->private_data->hide_if_empty = TRUE;
  action->private_data->always_show_image = FALSE;
  action->private_data->activate_blocked = FALSE;

  action->private_data->sensitive = TRUE;
  action->private_data->visible = TRUE;

  action->private_data->label_set = FALSE;
  action->private_data->short_label_set = FALSE;

  action->private_data->accel_count = 0;
  action->private_data->accel_group = NULL;
  action->private_data->accel_quark = 0;
  action->private_data->accel_closure = 
    g_closure_new_object (sizeof (GClosure), B_OBJECT (action));
  g_closure_set_marshal (action->private_data->accel_closure, 
			 closure_accel_activate);
  g_closure_ref (action->private_data->accel_closure);
  g_closure_sink (action->private_data->accel_closure);

  action->private_data->action_group = NULL;

  action->private_data->proxies = NULL;
  action->private_data->gicon = NULL;  
}

static void
btk_action_buildable_init (BtkBuildableIface *iface)
{
  iface->set_name = btk_action_buildable_set_name;
  iface->get_name = btk_action_buildable_get_name;
}

static void
btk_action_buildable_set_name (BtkBuildable *buildable,
			       const gchar  *name)
{
  BtkAction *action = BTK_ACTION (buildable);

  action->private_data->name = g_intern_string (name);
}

static const gchar *
btk_action_buildable_get_name (BtkBuildable *buildable)
{
  BtkAction *action = BTK_ACTION (buildable);

  return action->private_data->name;
}

/**
 * btk_action_new:
 * @name: A unique name for the action
 * @label: (allow-none): the label displayed in menu items and on buttons, or %NULL
 * @tooltip: (allow-none): a tooltip for the action, or %NULL
 * @stock_id: the stock icon to display in widgets representing the
 *   action, or %NULL
 *
 * Creates a new #BtkAction object. To add the action to a
 * #BtkActionGroup and set the accelerator for the action,
 * call btk_action_group_add_action_with_accel().
 * See <xref linkend="XML-UI"/> for information on allowed action
 * names.
 *
 * Return value: a new #BtkAction
 *
 * Since: 2.4
 */
BtkAction *
btk_action_new (const gchar *name,
		const gchar *label,
		const gchar *tooltip,
		const gchar *stock_id)
{
  g_return_val_if_fail (name != NULL, NULL);

  return g_object_new (BTK_TYPE_ACTION,
                       "name", name,
		       "label", label,
		       "tooltip", tooltip,
		       "stock-id", stock_id,
		       NULL);
}

static void
btk_action_finalize (BObject *object)
{
  BtkAction *action;
  action = BTK_ACTION (object);

  g_free (action->private_data->label);
  g_free (action->private_data->short_label);
  g_free (action->private_data->tooltip);
  g_free (action->private_data->stock_id);
  g_free (action->private_data->icon_name);
  
  if (action->private_data->gicon)
    g_object_unref (action->private_data->gicon);

  g_closure_unref (action->private_data->accel_closure);
  if (action->private_data->accel_group)
    g_object_unref (action->private_data->accel_group);

  B_OBJECT_CLASS (btk_action_parent_class)->finalize (object);  
}

static void
btk_action_set_property (BObject         *object,
			 guint            prop_id,
			 const BValue    *value,
			 BParamSpec      *pspec)
{
  BtkAction *action;
  
  action = BTK_ACTION (object);

  switch (prop_id)
    {
    case PROP_NAME:
      action->private_data->name = g_intern_string (b_value_get_string (value));
      break;
    case PROP_LABEL:
      btk_action_set_label (action, b_value_get_string (value));
      break;
    case PROP_SHORT_LABEL:
      btk_action_set_short_label (action, b_value_get_string (value));
      break;
    case PROP_TOOLTIP:
      btk_action_set_tooltip (action, b_value_get_string (value));
      break;
    case PROP_STOCK_ID:
      btk_action_set_stock_id (action, b_value_get_string (value));
      break;
    case PROP_GICON:
      btk_action_set_gicon (action, b_value_get_object (value));
      break;
    case PROP_ICON_NAME:
      btk_action_set_icon_name (action, b_value_get_string (value));
      break;
    case PROP_VISIBLE_HORIZONTAL:
      btk_action_set_visible_horizontal (action, b_value_get_boolean (value));
      break;
    case PROP_VISIBLE_VERTICAL:
      btk_action_set_visible_vertical (action, b_value_get_boolean (value));
      break;
    case PROP_VISIBLE_OVERFLOWN:
      action->private_data->visible_overflown = b_value_get_boolean (value);
      break;
    case PROP_IS_IMPORTANT:
      btk_action_set_is_important (action, b_value_get_boolean (value));
      break;
    case PROP_HIDE_IF_EMPTY:
      action->private_data->hide_if_empty = b_value_get_boolean (value);
      break;
    case PROP_SENSITIVE:
      btk_action_set_sensitive (action, b_value_get_boolean (value));
      break;
    case PROP_VISIBLE:
      btk_action_set_visible (action, b_value_get_boolean (value));
      break;
    case PROP_ACTION_GROUP:
      btk_action_set_action_group (action, b_value_get_object (value));
      break;
    case PROP_ALWAYS_SHOW_IMAGE:
      btk_action_set_always_show_image (action, b_value_get_boolean (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_action_get_property (BObject    *object,
			 guint       prop_id,
			 BValue     *value,
			 BParamSpec *pspec)
{
  BtkAction *action;

  action = BTK_ACTION (object);

  switch (prop_id)
    {
    case PROP_NAME:
      b_value_set_static_string (value, action->private_data->name);
      break;
    case PROP_LABEL:
      b_value_set_string (value, action->private_data->label);
      break;
    case PROP_SHORT_LABEL:
      b_value_set_string (value, action->private_data->short_label);
      break;
    case PROP_TOOLTIP:
      b_value_set_string (value, action->private_data->tooltip);
      break;
    case PROP_STOCK_ID:
      b_value_set_string (value, action->private_data->stock_id);
      break;
    case PROP_ICON_NAME:
      b_value_set_string (value, action->private_data->icon_name);
      break;
    case PROP_GICON:
      b_value_set_object (value, action->private_data->gicon);
      break;
    case PROP_VISIBLE_HORIZONTAL:
      b_value_set_boolean (value, action->private_data->visible_horizontal);
      break;
    case PROP_VISIBLE_VERTICAL:
      b_value_set_boolean (value, action->private_data->visible_vertical);
      break;
    case PROP_VISIBLE_OVERFLOWN:
      b_value_set_boolean (value, action->private_data->visible_overflown);
      break;
    case PROP_IS_IMPORTANT:
      b_value_set_boolean (value, action->private_data->is_important);
      break;
    case PROP_HIDE_IF_EMPTY:
      b_value_set_boolean (value, action->private_data->hide_if_empty);
      break;
    case PROP_SENSITIVE:
      b_value_set_boolean (value, action->private_data->sensitive);
      break;
    case PROP_VISIBLE:
      b_value_set_boolean (value, action->private_data->visible);
      break;
    case PROP_ACTION_GROUP:
      b_value_set_object (value, action->private_data->action_group);
      break;
    case PROP_ALWAYS_SHOW_IMAGE:
      b_value_set_boolean (value, action->private_data->always_show_image);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static BtkWidget *
create_menu_item (BtkAction *action)
{
  GType menu_item_type;

  menu_item_type = BTK_ACTION_GET_CLASS (action)->menu_item_type;

  return g_object_new (menu_item_type, NULL);
}

static BtkWidget *
create_tool_item (BtkAction *action)
{
  GType toolbar_item_type;

  toolbar_item_type = BTK_ACTION_GET_CLASS (action)->toolbar_item_type;

  return g_object_new (toolbar_item_type, NULL);
}

static void
remove_proxy (BtkAction *action,
	      BtkWidget *proxy)
{
  action->private_data->proxies = b_slist_remove (action->private_data->proxies, proxy);
}

static void
connect_proxy (BtkAction *action,
	       BtkWidget *proxy)
{
  action->private_data->proxies = b_slist_prepend (action->private_data->proxies, proxy);

  if (action->private_data->action_group)
    _btk_action_group_emit_connect_proxy (action->private_data->action_group, action, proxy);

}

static void
disconnect_proxy (BtkAction *action,
		  BtkWidget *proxy)
{
  remove_proxy (action, proxy);

  if (action->private_data->action_group)
    _btk_action_group_emit_disconnect_proxy (action->private_data->action_group, action, proxy);
}

/**
 * _btk_action_sync_menu_visible:
 * @action: (allow-none): a #BtkAction, or %NULL to determine the action from @proxy
 * @proxy: a proxy menu item
 * @empty: whether the submenu attached to @proxy is empty
 * 
 * Updates the visibility of @proxy from the visibility of @action
 * according to the following rules:
 * <itemizedlist>
 * <listitem><para>if @action is invisible, @proxy is too
 * </para></listitem>
 * <listitem><para>if @empty is %TRUE, hide @proxy unless the "hide-if-empty" 
 *   property of @action indicates otherwise
 * </para></listitem>
 * </itemizedlist>
 * 
 * This function is used in the implementation of #BtkUIManager.
 **/
void
_btk_action_sync_menu_visible (BtkAction *action,
			       BtkWidget *proxy,
			       gboolean   empty)
{
  gboolean visible = TRUE;
  gboolean hide_if_empty = TRUE;

  g_return_if_fail (BTK_IS_MENU_ITEM (proxy));
  g_return_if_fail (action == NULL || BTK_IS_ACTION (action));

  if (action == NULL)
    action = btk_activatable_get_related_action (BTK_ACTIVATABLE (proxy));

  if (action)
    {
      /* a BtkMenu for a <popup/> doesn't have to have an action */
      visible = btk_action_is_visible (action);
      hide_if_empty = action->private_data->hide_if_empty;
    }

  if (visible && !(empty && hide_if_empty))
    btk_widget_show (proxy);
  else
    btk_widget_hide (proxy);
}

void
_btk_action_emit_activate (BtkAction *action)
{
  BtkActionGroup *group = action->private_data->action_group;

  if (group != NULL)
    {
      g_object_ref (action);
      g_object_ref (group);
      _btk_action_group_emit_pre_activate (group, action);
    }

  g_signal_emit (action, action_signals[ACTIVATE], 0);

  if (group != NULL)
    {
      _btk_action_group_emit_post_activate (group, action);
      g_object_unref (group);
      g_object_unref (action);
    }
}

/**
 * btk_action_activate:
 * @action: the action object
 *
 * Emits the "activate" signal on the specified action, if it isn't 
 * insensitive. This gets called by the proxy widgets when they get 
 * activated.
 *
 * It can also be used to manually activate an action.
 *
 * Since: 2.4
 */
void
btk_action_activate (BtkAction *action)
{
  g_return_if_fail (BTK_IS_ACTION (action));
  
  if (action->private_data->activate_blocked)
    return;

  if (btk_action_is_sensitive (action))
    _btk_action_emit_activate (action);
}

/**
 * btk_action_block_activate:
 * @action: a #BtkAction
 *
 * Disable activation signals from the action 
 *
 * This is needed when updating the state of your proxy
 * #BtkActivatable widget could result in calling btk_action_activate(),
 * this is a convenience function to avoid recursing in those
 * cases (updating toggle state for instance).
 *
 * Since: 2.16
 */
void
btk_action_block_activate (BtkAction *action)
{
  g_return_if_fail (BTK_IS_ACTION (action));

  action->private_data->activate_blocked = TRUE;
}

/**
 * btk_action_unblock_activate:
 * @action: a #BtkAction
 *
 * Reenable activation signals from the action 
 *
 * Since: 2.16
 */
void
btk_action_unblock_activate (BtkAction *action)
{
  g_return_if_fail (BTK_IS_ACTION (action));

  action->private_data->activate_blocked = FALSE;
}

/**
 * btk_action_create_icon:
 * @action: the action object
 * @icon_size: (type int): the size of the icon that should be created.
 *
 * This function is intended for use by action implementations to
 * create icons displayed in the proxy widgets.
 *
 * Returns: (transfer none): a widget that displays the icon for this action.
 *
 * Since: 2.4
 */
BtkWidget *
btk_action_create_icon (BtkAction *action, BtkIconSize icon_size)
{
  g_return_val_if_fail (BTK_IS_ACTION (action), NULL);

  if (action->private_data->stock_id &&
      btk_icon_factory_lookup_default (action->private_data->stock_id))
    return btk_image_new_from_stock (action->private_data->stock_id, icon_size);
  else if (action->private_data->gicon)
    return btk_image_new_from_gicon (action->private_data->gicon, icon_size);
  else if (action->private_data->icon_name)
    return btk_image_new_from_icon_name (action->private_data->icon_name, icon_size);
  else
    return NULL;
}

/**
 * btk_action_create_menu_item:
 * @action: the action object
 *
 * Creates a menu item widget that proxies for the given action.
 *
 * Returns: (transfer none): a menu item connected to the action.
 *
 * Since: 2.4
 */
BtkWidget *
btk_action_create_menu_item (BtkAction *action)
{
  BtkWidget *menu_item;

  g_return_val_if_fail (BTK_IS_ACTION (action), NULL);

  menu_item = BTK_ACTION_GET_CLASS (action)->create_menu_item (action);

  btk_activatable_set_use_action_appearance (BTK_ACTIVATABLE (menu_item), TRUE);
  btk_activatable_set_related_action (BTK_ACTIVATABLE (menu_item), action);

  return menu_item;
}

/**
 * btk_action_create_tool_item:
 * @action: the action object
 *
 * Creates a toolbar item widget that proxies for the given action.
 *
 * Returns: (transfer none): a toolbar item connected to the action.
 *
 * Since: 2.4
 */
BtkWidget *
btk_action_create_tool_item (BtkAction *action)
{
  BtkWidget *button;

  g_return_val_if_fail (BTK_IS_ACTION (action), NULL);

  button = BTK_ACTION_GET_CLASS (action)->create_tool_item (action);

  btk_activatable_set_use_action_appearance (BTK_ACTIVATABLE (button), TRUE);
  btk_activatable_set_related_action (BTK_ACTIVATABLE (button), action);

  return button;
}

void
_btk_action_add_to_proxy_list (BtkAction     *action,
			       BtkWidget     *proxy)
{
  g_return_if_fail (BTK_IS_ACTION (action));
  g_return_if_fail (BTK_IS_WIDGET (proxy));
 
  BTK_ACTION_GET_CLASS (action)->connect_proxy (action, proxy);
}

void
_btk_action_remove_from_proxy_list (BtkAction     *action,
				    BtkWidget     *proxy)
{
  g_return_if_fail (BTK_IS_ACTION (action));
  g_return_if_fail (BTK_IS_WIDGET (proxy));

  BTK_ACTION_GET_CLASS (action)->disconnect_proxy (action, proxy);
}

/**
 * btk_action_connect_proxy:
 * @action: the action object
 * @proxy: the proxy widget
 *
 * Connects a widget to an action object as a proxy.  Synchronises 
 * various properties of the action with the widget (such as label 
 * text, icon, tooltip, etc), and attaches a callback so that the 
 * action gets activated when the proxy widget does.
 *
 * If the widget is already connected to an action, it is disconnected
 * first.
 *
 * Since: 2.4
 *
 * Deprecated: 2.16: Use btk_activatable_set_related_action() instead.
 */
void
btk_action_connect_proxy (BtkAction *action,
			  BtkWidget *proxy)
{
  g_return_if_fail (BTK_IS_ACTION (action));
  g_return_if_fail (BTK_IS_WIDGET (proxy));
  g_return_if_fail (BTK_IS_ACTIVATABLE (proxy));

  btk_activatable_set_use_action_appearance (BTK_ACTIVATABLE (proxy), TRUE);

  btk_activatable_set_related_action (BTK_ACTIVATABLE (proxy), action);
}

/**
 * btk_action_disconnect_proxy:
 * @action: the action object
 * @proxy: the proxy widget
 *
 * Disconnects a proxy widget from an action.  
 * Does <emphasis>not</emphasis> destroy the widget, however.
 *
 * Since: 2.4
 *
 * Deprecated: 2.16: Use btk_activatable_set_related_action() instead.
 */
void
btk_action_disconnect_proxy (BtkAction *action,
			     BtkWidget *proxy)
{
  g_return_if_fail (BTK_IS_ACTION (action));
  g_return_if_fail (BTK_IS_WIDGET (proxy));

  btk_activatable_set_related_action (BTK_ACTIVATABLE (proxy), NULL);
}

/**
 * btk_action_get_proxies:
 * @action: the action object
 * 
 * Returns the proxy widgets for an action.
 * See also btk_widget_get_action().
 *
 * Return value: (element-type BtkWidget) (transfer none): a #GSList of proxy widgets. The list is owned by BTK+
 * and must not be modified.
 *
 * Since: 2.4
 **/
GSList*
btk_action_get_proxies (BtkAction *action)
{
  g_return_val_if_fail (BTK_IS_ACTION (action), NULL);

  return action->private_data->proxies;
}


/**
 * btk_widget_get_action:
 * @widget: a #BtkWidget
 *
 * Returns the #BtkAction that @widget is a proxy for. 
 * See also btk_action_get_proxies().
 *
 * Returns: the action that a widget is a proxy for, or
 *  %NULL, if it is not attached to an action.
 *
 * Since: 2.10
 *
 * Deprecated: 2.16: Use btk_activatable_get_related_action() instead.
 */
BtkAction*
btk_widget_get_action (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  if (BTK_IS_ACTIVATABLE (widget))
    return btk_activatable_get_related_action (BTK_ACTIVATABLE (widget));

  return NULL;
}

/**
 * btk_action_get_name:
 * @action: the action object
 * 
 * Returns the name of the action.
 * 
 * Return value: the name of the action. The string belongs to BTK+ and should not
 *   be freed.
 *
 * Since: 2.4
 **/
const gchar *
btk_action_get_name (BtkAction *action)
{
  g_return_val_if_fail (BTK_IS_ACTION (action), NULL);

  return action->private_data->name;
}

/**
 * btk_action_is_sensitive:
 * @action: the action object
 * 
 * Returns whether the action is effectively sensitive.
 *
 * Return value: %TRUE if the action and its associated action group 
 * are both sensitive.
 *
 * Since: 2.4
 **/
gboolean
btk_action_is_sensitive (BtkAction *action)
{
  BtkActionPrivate *priv;
  g_return_val_if_fail (BTK_IS_ACTION (action), FALSE);

  priv = action->private_data;
  return priv->sensitive &&
    (priv->action_group == NULL ||
     btk_action_group_get_sensitive (priv->action_group));
}

/**
 * btk_action_get_sensitive:
 * @action: the action object
 * 
 * Returns whether the action itself is sensitive. Note that this doesn't 
 * necessarily mean effective sensitivity. See btk_action_is_sensitive() 
 * for that.
 *
 * Return value: %TRUE if the action itself is sensitive.
 *
 * Since: 2.4
 **/
gboolean
btk_action_get_sensitive (BtkAction *action)
{
  g_return_val_if_fail (BTK_IS_ACTION (action), FALSE);

  return action->private_data->sensitive;
}

/**
 * btk_action_set_sensitive:
 * @action: the action object
 * @sensitive: %TRUE to make the action sensitive
 * 
 * Sets the ::sensitive property of the action to @sensitive. Note that 
 * this doesn't necessarily mean effective sensitivity. See 
 * btk_action_is_sensitive() 
 * for that.
 *
 * Since: 2.6
 **/
void
btk_action_set_sensitive (BtkAction *action,
			  gboolean   sensitive)
{
  g_return_if_fail (BTK_IS_ACTION (action));

  sensitive = sensitive != FALSE;
  
  if (action->private_data->sensitive != sensitive)
    {
      action->private_data->sensitive = sensitive;

      g_object_notify (B_OBJECT (action), "sensitive");
    }
}

/**
 * btk_action_is_visible:
 * @action: the action object
 * 
 * Returns whether the action is effectively visible.
 *
 * Return value: %TRUE if the action and its associated action group 
 * are both visible.
 *
 * Since: 2.4
 **/
gboolean
btk_action_is_visible (BtkAction *action)
{
  BtkActionPrivate *priv;
  g_return_val_if_fail (BTK_IS_ACTION (action), FALSE);

  priv = action->private_data;
  return priv->visible &&
    (priv->action_group == NULL ||
     btk_action_group_get_visible (priv->action_group));
}

/**
 * btk_action_get_visible:
 * @action: the action object
 * 
 * Returns whether the action itself is visible. Note that this doesn't 
 * necessarily mean effective visibility. See btk_action_is_sensitive() 
 * for that.
 *
 * Return value: %TRUE if the action itself is visible.
 *
 * Since: 2.4
 **/
gboolean
btk_action_get_visible (BtkAction *action)
{
  g_return_val_if_fail (BTK_IS_ACTION (action), FALSE);

  return action->private_data->visible;
}

/**
 * btk_action_set_visible:
 * @action: the action object
 * @visible: %TRUE to make the action visible
 * 
 * Sets the ::visible property of the action to @visible. Note that 
 * this doesn't necessarily mean effective visibility. See 
 * btk_action_is_visible() 
 * for that.
 *
 * Since: 2.6
 **/
void
btk_action_set_visible (BtkAction *action,
			gboolean   visible)
{
  g_return_if_fail (BTK_IS_ACTION (action));

  visible = visible != FALSE;
  
  if (action->private_data->visible != visible)
    {
      action->private_data->visible = visible;

      g_object_notify (B_OBJECT (action), "visible");
    }
}
/**
 * btk_action_set_is_important:
 * @action: the action object
 * @is_important: %TRUE to make the action important
 *
 * Sets whether the action is important, this attribute is used
 * primarily by toolbar items to decide whether to show a label
 * or not.
 *
 * Since: 2.16
 */
void 
btk_action_set_is_important (BtkAction *action,
			     gboolean   is_important)
{
  g_return_if_fail (BTK_IS_ACTION (action));

  is_important = is_important != FALSE;
  
  if (action->private_data->is_important != is_important)
    {
      action->private_data->is_important = is_important;
      
      g_object_notify (B_OBJECT (action), "is-important");
    }  
}

/**
 * btk_action_get_is_important:
 * @action: a #BtkAction
 *
 * Checks whether @action is important or not
 * 
 * Returns: whether @action is important
 *
 * Since: 2.16
 */
gboolean 
btk_action_get_is_important (BtkAction *action)
{
  g_return_val_if_fail (BTK_IS_ACTION (action), FALSE);

  return action->private_data->is_important;
}

/**
 * btk_action_set_always_show_image:
 * @action: a #BtkAction
 * @always_show: %TRUE if menuitem proxies should always show their image
 *
 * Sets whether @action<!-- -->'s menu item proxies will ignore the
 * #BtkSettings:btk-menu-images setting and always show their image, if available.
 *
 * Use this if the menu item would be useless or hard to use
 * without their image.
 *
 * Since: 2.20
 */
void
btk_action_set_always_show_image (BtkAction *action,
                                  gboolean   always_show)
{
  BtkActionPrivate *priv;

  g_return_if_fail (BTK_IS_ACTION (action));

  priv = action->private_data;

  always_show = always_show != FALSE;
  
  if (priv->always_show_image != always_show)
    {
      priv->always_show_image = always_show;

      g_object_notify (B_OBJECT (action), "always-show-image");
    }
}

/**
 * btk_action_get_always_show_image:
 * @action: a #BtkAction
 *
 * Returns whether @action<!-- -->'s menu item proxies will ignore the
 * #BtkSettings:btk-menu-images setting and always show their image,
 * if available.
 *
 * Returns: %TRUE if the menu item proxies will always show their image
 *
 * Since: 2.20
 */
gboolean
btk_action_get_always_show_image  (BtkAction *action)
{
  g_return_val_if_fail (BTK_IS_ACTION (action), FALSE);

  return action->private_data->always_show_image;
}

/**
 * btk_action_set_label:
 * @action: a #BtkAction
 * @label: the label text to set
 *
 * Sets the label of @action.
 *
 * Since: 2.16
 */
void 
btk_action_set_label (BtkAction	  *action,
		      const gchar *label)
{
  gchar *tmp;
  
  g_return_if_fail (BTK_IS_ACTION (action));
  
  tmp = action->private_data->label;
  action->private_data->label = g_strdup (label);
  g_free (tmp);
  action->private_data->label_set = (action->private_data->label != NULL);
  /* if label is unset, then use the label from the stock item */
  if (!action->private_data->label_set && action->private_data->stock_id)
    {
      BtkStockItem stock_item;
      
      if (btk_stock_lookup (action->private_data->stock_id, &stock_item))
	action->private_data->label = g_strdup (stock_item.label);
    }

  g_object_notify (B_OBJECT (action), "label");
  
  /* if short_label is unset, set short_label=label */
  if (!action->private_data->short_label_set)
    {
      btk_action_set_short_label (action, action->private_data->label);
      action->private_data->short_label_set = FALSE;
    }
}

/**
 * btk_action_get_label:
 * @action: a #BtkAction
 *
 * Gets the label text of @action.
 *
 * Returns: the label text
 *
 * Since: 2.16
 */
const gchar *
btk_action_get_label (BtkAction *action)
{
  g_return_val_if_fail (BTK_IS_ACTION (action), NULL);

  return action->private_data->label;
}

/**
 * btk_action_set_short_label:
 * @action: a #BtkAction
 * @short_label: the label text to set
 *
 * Sets a shorter label text on @action.
 *
 * Since: 2.16
 */
void 
btk_action_set_short_label (BtkAction   *action,
			    const gchar *short_label)
{
  gchar *tmp;

  g_return_if_fail (BTK_IS_ACTION (action));

  tmp = action->private_data->short_label;
  action->private_data->short_label = g_strdup (short_label);
  g_free (tmp);
  action->private_data->short_label_set = (action->private_data->short_label != NULL);
  /* if short_label is unset, then use the value of label */
  if (!action->private_data->short_label_set)
    action->private_data->short_label = g_strdup (action->private_data->label);

  g_object_notify (B_OBJECT (action), "short-label");
}

/**
 * btk_action_get_short_label:
 * @action: a #BtkAction
 *
 * Gets the short label text of @action.
 *
 * Returns: the short label text.
 *
 * Since: 2.16
 */
const gchar *
btk_action_get_short_label (BtkAction *action)
{
  g_return_val_if_fail (BTK_IS_ACTION (action), NULL);

  return action->private_data->short_label;
}

/**
 * btk_action_set_visible_horizontal:
 * @action: a #BtkAction
 * @visible_horizontal: whether the action is visible horizontally
 *
 * Sets whether @action is visible when horizontal
 *
 * Since: 2.16
 */
void
btk_action_set_visible_horizontal (BtkAction *action,
				   gboolean   visible_horizontal)
{
  g_return_if_fail (BTK_IS_ACTION (action));

  g_return_if_fail (BTK_IS_ACTION (action));

  visible_horizontal = visible_horizontal != FALSE;
  
  if (action->private_data->visible_horizontal != visible_horizontal)
    {
      action->private_data->visible_horizontal = visible_horizontal;
      
      g_object_notify (B_OBJECT (action), "visible-horizontal");
    }  
}

/**
 * btk_action_get_visible_horizontal:
 * @action: a #BtkAction
 *
 * Checks whether @action is visible when horizontal
 * 
 * Returns: whether @action is visible when horizontal
 *
 * Since: 2.16
 */
gboolean 
btk_action_get_visible_horizontal (BtkAction *action)
{
  g_return_val_if_fail (BTK_IS_ACTION (action), FALSE);

  return action->private_data->visible_horizontal;
}

/**
 * btk_action_set_visible_vertical:
 * @action: a #BtkAction
 * @visible_vertical: whether the action is visible vertically
 *
 * Sets whether @action is visible when vertical 
 *
 * Since: 2.16
 */
void 
btk_action_set_visible_vertical (BtkAction *action,
				 gboolean   visible_vertical)
{
  g_return_if_fail (BTK_IS_ACTION (action));

  g_return_if_fail (BTK_IS_ACTION (action));

  visible_vertical = visible_vertical != FALSE;
  
  if (action->private_data->visible_vertical != visible_vertical)
    {
      action->private_data->visible_vertical = visible_vertical;
      
      g_object_notify (B_OBJECT (action), "visible-vertical");
    }  
}

/**
 * btk_action_get_visible_vertical:
 * @action: a #BtkAction
 *
 * Checks whether @action is visible when horizontal
 * 
 * Returns: whether @action is visible when horizontal
 *
 * Since: 2.16
 */
gboolean 
btk_action_get_visible_vertical (BtkAction *action)
{
  g_return_val_if_fail (BTK_IS_ACTION (action), FALSE);

  return action->private_data->visible_vertical;
}

/**
 * btk_action_set_tooltip:
 * @action: a #BtkAction
 * @tooltip: the tooltip text
 *
 * Sets the tooltip text on @action
 *
 * Since: 2.16
 */
void 
btk_action_set_tooltip (BtkAction   *action,
			const gchar *tooltip)
{
  gchar *tmp;

  g_return_if_fail (BTK_IS_ACTION (action));

  tmp = action->private_data->tooltip;
  action->private_data->tooltip = g_strdup (tooltip);
  g_free (tmp);

  g_object_notify (B_OBJECT (action), "tooltip");
}

/**
 * btk_action_get_tooltip:
 * @action: a #BtkAction
 *
 * Gets the tooltip text of @action.
 *
 * Returns: the tooltip text
 *
 * Since: 2.16
 */
const gchar *
btk_action_get_tooltip (BtkAction *action)
{
  g_return_val_if_fail (BTK_IS_ACTION (action), NULL);

  return action->private_data->tooltip;
}

/**
 * btk_action_set_stock_id:
 * @action: a #BtkAction
 * @stock_id: the stock id
 *
 * Sets the stock id on @action
 *
 * Since: 2.16
 */
void 
btk_action_set_stock_id (BtkAction   *action,
			 const gchar *stock_id)
{
  gchar *tmp;

  g_return_if_fail (BTK_IS_ACTION (action));

  g_return_if_fail (BTK_IS_ACTION (action));

  tmp = action->private_data->stock_id;
  action->private_data->stock_id = g_strdup (stock_id);
  g_free (tmp);

  g_object_notify (B_OBJECT (action), "stock-id");
  
  /* update label and short_label if appropriate */
  if (!action->private_data->label_set)
    {
      BtkStockItem stock_item;
      
      if (action->private_data->stock_id &&
	  btk_stock_lookup (action->private_data->stock_id, &stock_item))
	btk_action_set_label (action, stock_item.label);
      else 
	btk_action_set_label (action, NULL);
      
      action->private_data->label_set = FALSE;
    }
}

/**
 * btk_action_get_stock_id:
 * @action: a #BtkAction
 *
 * Gets the stock id of @action.
 *
 * Returns: the stock id
 *
 * Since: 2.16
 */
const gchar *
btk_action_get_stock_id (BtkAction *action)
{
  g_return_val_if_fail (BTK_IS_ACTION (action), NULL);

  return action->private_data->stock_id;
}

/**
 * btk_action_set_icon_name:
 * @action: a #BtkAction
 * @icon_name: the icon name to set
 *
 * Sets the icon name on @action
 *
 * Since: 2.16
 */
void 
btk_action_set_icon_name (BtkAction   *action,
			  const gchar *icon_name)
{
  gchar *tmp;

  g_return_if_fail (BTK_IS_ACTION (action));

  tmp = action->private_data->icon_name;
  action->private_data->icon_name = g_strdup (icon_name);
  g_free (tmp);

  g_object_notify (B_OBJECT (action), "icon-name");
}

/**
 * btk_action_get_icon_name:
 * @action: a #BtkAction
 *
 * Gets the icon name of @action.
 *
 * Returns: the icon name
 *
 * Since: 2.16
 */
const gchar *
btk_action_get_icon_name (BtkAction *action)
{
  g_return_val_if_fail (BTK_IS_ACTION (action), NULL);

  return action->private_data->icon_name;
}

/**
 * btk_action_set_gicon:
 * @action: a #BtkAction
 * @icon: the #GIcon to set
 *
 * Sets the icon of @action.
 *
 * Since: 2.16
 */
void
btk_action_set_gicon (BtkAction *action,
                      GIcon     *icon)
{
  g_return_if_fail (BTK_IS_ACTION (action));

  if (action->private_data->gicon)
    g_object_unref (action->private_data->gicon);

  action->private_data->gicon = icon;

  if (action->private_data->gicon)
    g_object_ref (action->private_data->gicon);

  g_object_notify (B_OBJECT (action), "gicon");
}

/**
 * btk_action_get_gicon:
 * @action: a #BtkAction
 *
 * Gets the gicon of @action.
 *
 * Returns: (transfer none): The action's #GIcon if one is set.
 *
 * Since: 2.16
 */
GIcon *
btk_action_get_gicon (BtkAction *action)
{
  g_return_val_if_fail (BTK_IS_ACTION (action), NULL);

  return action->private_data->gicon;
}

/**
 * btk_action_block_activate_from:
 * @action: the action object
 * @proxy: a proxy widget
 *
 * Disables calls to the btk_action_activate()
 * function by signals on the given proxy widget.  This is used to
 * break notification loops for things like check or radio actions.
 *
 * This function is intended for use by action implementations.
 * 
 * Since: 2.4
 *
 * Deprecated: 2.16: activatables are now responsible for activating the
 * action directly so this doesnt apply anymore.
 */
void
btk_action_block_activate_from (BtkAction *action, 
				BtkWidget *proxy)
{
  g_return_if_fail (BTK_IS_ACTION (action));
  
  g_signal_handlers_block_by_func (proxy, G_CALLBACK (btk_action_activate),
				   action);

  btk_action_block_activate (action);
}

/**
 * btk_action_unblock_activate_from:
 * @action: the action object
 * @proxy: a proxy widget
 *
 * Re-enables calls to the btk_action_activate()
 * function by signals on the given proxy widget.  This undoes the
 * blocking done by btk_action_block_activate_from().
 *
 * This function is intended for use by action implementations.
 * 
 * Since: 2.4
 *
 * Deprecated: 2.16: activatables are now responsible for activating the
 * action directly so this doesnt apply anymore.
 */
void
btk_action_unblock_activate_from (BtkAction *action, 
				  BtkWidget *proxy)
{
  g_return_if_fail (BTK_IS_ACTION (action));

  g_signal_handlers_unblock_by_func (proxy, G_CALLBACK (btk_action_activate),
				     action);

  btk_action_unblock_activate (action);
}

static void
closure_accel_activate (GClosure     *closure,
                        BValue       *return_value,
                        guint         n_param_values,
                        const BValue *param_values,
                        gpointer      invocation_hint,
                        gpointer      marshal_data)
{
  if (btk_action_is_sensitive (BTK_ACTION (closure->data)))
    {
      _btk_action_emit_activate (BTK_ACTION (closure->data));
      
      /* we handled the accelerator */
      b_value_set_boolean (return_value, TRUE);
    }
}

static void
btk_action_set_action_group (BtkAction	    *action,
			     BtkActionGroup *action_group)
{
  if (action->private_data->action_group == NULL)
    g_return_if_fail (BTK_IS_ACTION_GROUP (action_group));
  else
    g_return_if_fail (action_group == NULL);

  action->private_data->action_group = action_group;
}

/**
 * btk_action_set_accel_path:
 * @action: the action object
 * @accel_path: the accelerator path
 *
 * Sets the accel path for this action.  All proxy widgets associated
 * with the action will have this accel path, so that their
 * accelerators are consistent.
 *
 * Note that @accel_path string will be stored in a #GQuark. Therefore, if you
 * pass a static string, you can save some memory by interning it first with 
 * g_intern_static_string().
 *
 * Since: 2.4
 */
void
btk_action_set_accel_path (BtkAction   *action, 
			   const gchar *accel_path)
{
  g_return_if_fail (BTK_IS_ACTION (action));

  action->private_data->accel_quark = g_quark_from_string (accel_path);
}

/**
 * btk_action_get_accel_path:
 * @action: the action object
 *
 * Returns the accel path for this action.  
 *
 * Since: 2.6
 *
 * Returns: the accel path for this action, or %NULL
 *   if none is set. The returned string is owned by BTK+ 
 *   and must not be freed or modified.
 */
const gchar *
btk_action_get_accel_path (BtkAction *action)
{
  g_return_val_if_fail (BTK_IS_ACTION (action), NULL);

  if (action->private_data->accel_quark)
    return g_quark_to_string (action->private_data->accel_quark);
  else
    return NULL;
}

/**
 * btk_action_get_accel_closure:
 * @action: the action object
 *
 * Returns the accel closure for this action.
 *
 * Since: 2.8
 *
 * Returns: (transfer none): the accel closure for this action. The
 *          returned closure is owned by BTK+ and must not be unreffed
 *          or modified.
 */
GClosure *
btk_action_get_accel_closure (BtkAction *action)
{
  g_return_val_if_fail (BTK_IS_ACTION (action), NULL);

  return action->private_data->accel_closure;
}


/**
 * btk_action_set_accel_group:
 * @action: the action object
 * @accel_group: (allow-none): a #BtkAccelGroup or %NULL
 *
 * Sets the #BtkAccelGroup in which the accelerator for this action
 * will be installed.
 *
 * Since: 2.4
 **/
void
btk_action_set_accel_group (BtkAction     *action,
			    BtkAccelGroup *accel_group)
{
  g_return_if_fail (BTK_IS_ACTION (action));
  g_return_if_fail (accel_group == NULL || BTK_IS_ACCEL_GROUP (accel_group));
  
  if (accel_group)
    g_object_ref (accel_group);
  if (action->private_data->accel_group)
    g_object_unref (action->private_data->accel_group);

  action->private_data->accel_group = accel_group;
}

/**
 * btk_action_connect_accelerator:
 * @action: a #BtkAction
 * 
 * Installs the accelerator for @action if @action has an
 * accel path and group. See btk_action_set_accel_path() and 
 * btk_action_set_accel_group()
 *
 * Since multiple proxies may independently trigger the installation
 * of the accelerator, the @action counts the number of times this
 * function has been called and doesn't remove the accelerator until
 * btk_action_disconnect_accelerator() has been called as many times.
 *
 * Since: 2.4
 **/
void 
btk_action_connect_accelerator (BtkAction *action)
{
  g_return_if_fail (BTK_IS_ACTION (action));

  if (!action->private_data->accel_quark ||
      !action->private_data->accel_group)
    return;

  if (action->private_data->accel_count == 0)
    {
      const gchar *accel_path = 
	g_quark_to_string (action->private_data->accel_quark);
      
      btk_accel_group_connect_by_path (action->private_data->accel_group,
				       accel_path,
				       action->private_data->accel_closure);
    }

  action->private_data->accel_count++;
}

/**
 * btk_action_disconnect_accelerator:
 * @action: a #BtkAction
 * 
 * Undoes the effect of one call to btk_action_connect_accelerator().
 *
 * Since: 2.4
 **/
void 
btk_action_disconnect_accelerator (BtkAction *action)
{
  g_return_if_fail (BTK_IS_ACTION (action));

  if (!action->private_data->accel_quark ||
      !action->private_data->accel_group)
    return;

  action->private_data->accel_count--;

  if (action->private_data->accel_count == 0)
    btk_accel_group_disconnect (action->private_data->accel_group,
				action->private_data->accel_closure);
}

/**
 * btk_action_create_menu:
 * @action: a #BtkAction
 *
 * If @action provides a #BtkMenu widget as a submenu for the menu
 * item or the toolbar item it creates, this function returns an
 * instance of that menu.
 *
 * Return value: (transfer none): the menu item provided by the
 *               action, or %NULL.
 *
 * Since: 2.12
 */
BtkWidget *
btk_action_create_menu (BtkAction *action)
{
  g_return_val_if_fail (BTK_IS_ACTION (action), NULL);

  if (BTK_ACTION_GET_CLASS (action)->create_menu)
    return BTK_ACTION_GET_CLASS (action)->create_menu (action);

  return NULL;
}

#define __BTK_ACTION_C__
#include "btkaliasdef.c"
