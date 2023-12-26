/* btkactivatable.c
 * Copyright (C) 2008 Tristan Van Berkom <tristan.van.berkom@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:btkactivatable
 * @Short_Description: An interface for activatable widgets
 * @Title: BtkActivatable
 *
 * Activatable widgets can be connected to a #BtkAction and reflects
 * the state of its action. A #BtkActivatable can also provide feedback
 * through its action, as they are responsible for activating their
 * related actions.
 *
 * <refsect2>
 * <title>Implementing BtkActivatable</title>
 * <para>
 * When extending a class that is already #BtkActivatable; it is only
 * necessary to implement the #BtkActivatable->sync_action_properties()
 * and #BtkActivatable->update() methods and chain up to the parent
 * implementation, however when introducing
 * a new #BtkActivatable class; the #BtkActivatable:related-action and
 * #BtkActivatable:use-action-appearance properties need to be handled by
 * the implementor. Handling these properties is mostly a matter of installing
 * the action pointer and boolean flag on your instance, and calling
 * btk_activatable_do_set_related_action() and
 * btk_activatable_sync_action_properties() at the appropriate times.
 * </para>
 * <example>
 * <title>A class fragment implementing #BtkActivatable</title>
 * <programlisting><![CDATA[
 *
 * enum {
 * ...
 *
 * PROP_ACTIVATABLE_RELATED_ACTION,
 * PROP_ACTIVATABLE_USE_ACTION_APPEARANCE
 * }
 * 
 * struct _FooBarPrivate
 * {
 * 
 *   ...
 * 
 *   BtkAction      *action;
 *   gboolean        use_action_appearance;
 * };
 * 
 * ...
 * 
 * static void foo_bar_activatable_interface_init         (BtkActivatableIface  *iface);
 * static void foo_bar_activatable_update                 (BtkActivatable       *activatable,
 * 						           BtkAction            *action,
 * 						           const gchar          *property_name);
 * static void foo_bar_activatable_sync_action_properties (BtkActivatable       *activatable,
 * 						           BtkAction            *action);
 * ...
 *
 *
 * static void
 * foo_bar_class_init (FooBarClass *klass)
 * {
 *
 *   ...
 *
 *   g_object_class_override_property (bobject_class, PROP_ACTIVATABLE_RELATED_ACTION, "related-action");
 *   g_object_class_override_property (bobject_class, PROP_ACTIVATABLE_USE_ACTION_APPEARANCE, "use-action-appearance");
 *
 *   ...
 * }
 *
 *
 * static void
 * foo_bar_activatable_interface_init (BtkActivatableIface  *iface)
 * {
 *   iface->update = foo_bar_activatable_update;
 *   iface->sync_action_properties = foo_bar_activatable_sync_action_properties;
 * }
 * 
 * ... Break the reference using btk_activatable_do_set_related_action()...
 *
 * static void 
 * foo_bar_dispose (BObject *object)
 * {
 *   FooBar *bar = FOO_BAR (object);
 *   FooBarPrivate *priv = FOO_BAR_GET_PRIVATE (bar);
 * 
 *   ...
 * 
 *   if (priv->action)
 *     {
 *       btk_activatable_do_set_related_action (BTK_ACTIVATABLE (bar), NULL);
 *       priv->action = NULL;
 *     }
 *   B_OBJECT_CLASS (foo_bar_parent_class)->dispose (object);
 * }
 * 
 * ... Handle the "related-action" and "use-action-appearance" properties ...
 *
 * static void
 * foo_bar_set_property (BObject         *object,
 *                       guint            prop_id,
 *                       const BValue    *value,
 *                       BParamSpec      *pspec)
 * {
 *   FooBar *bar = FOO_BAR (object);
 *   FooBarPrivate *priv = FOO_BAR_GET_PRIVATE (bar);
 * 
 *   switch (prop_id)
 *     {
 * 
 *       ...
 * 
 *     case PROP_ACTIVATABLE_RELATED_ACTION:
 *       foo_bar_set_related_action (bar, b_value_get_object (value));
 *       break;
 *     case PROP_ACTIVATABLE_USE_ACTION_APPEARANCE:
 *       foo_bar_set_use_action_appearance (bar, b_value_get_boolean (value));
 *       break;
 *     default:
 *       B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
 *       break;
 *     }
 * }
 * 
 * static void
 * foo_bar_get_property (BObject         *object,
 *                          guint            prop_id,
 *                          BValue          *value,
 *                          BParamSpec      *pspec)
 * {
 *   FooBar *bar = FOO_BAR (object);
 *   FooBarPrivate *priv = FOO_BAR_GET_PRIVATE (bar);
 * 
 *   switch (prop_id)
 *     { 
 * 
 *       ...
 * 
 *     case PROP_ACTIVATABLE_RELATED_ACTION:
 *       b_value_set_object (value, priv->action);
 *       break;
 *     case PROP_ACTIVATABLE_USE_ACTION_APPEARANCE:
 *       b_value_set_boolean (value, priv->use_action_appearance);
 *       break;
 *     default:
 *       B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
 *       break;
 *     }
 * }
 * 
 * 
 * static void
 * foo_bar_set_use_action_appearance (FooBar   *bar, 
 * 				   gboolean  use_appearance)
 * {
 *   FooBarPrivate *priv = FOO_BAR_GET_PRIVATE (bar);
 * 
 *   if (priv->use_action_appearance != use_appearance)
 *     {
 *       priv->use_action_appearance = use_appearance;
 *       
 *       btk_activatable_sync_action_properties (BTK_ACTIVATABLE (bar), priv->action);
 *     }
 * }
 * 
 * ... call btk_activatable_do_set_related_action() and then assign the action pointer, 
 * no need to reference the action here since btk_activatable_do_set_related_action() already 
 * holds a reference here for you...
 * static void
 * foo_bar_set_related_action (FooBar    *bar, 
 * 			    BtkAction *action)
 * {
 *   FooBarPrivate *priv = FOO_BAR_GET_PRIVATE (bar);
 * 
 *   if (priv->action == action)
 *     return;
 * 
 *   btk_activatable_do_set_related_action (BTK_ACTIVATABLE (bar), action);
 * 
 *   priv->action = action;
 * }
 * 
 * ... Selectively reset and update activatable depending on the use-action-appearance property ...
 * static void
 * btk_button_activatable_sync_action_properties (BtkActivatable       *activatable,
 * 		                                  BtkAction            *action)
 * {
 *   BtkButtonPrivate *priv = BTK_BUTTON_GET_PRIVATE (activatable);
 * 
 *   if (!action)
 *     return;
 * 
 *   if (btk_action_is_visible (action))
 *     btk_widget_show (BTK_WIDGET (activatable));
 *   else
 *     btk_widget_hide (BTK_WIDGET (activatable));
 *   
 *   btk_widget_set_sensitive (BTK_WIDGET (activatable), btk_action_is_sensitive (action));
 * 
 *   ...
 *   
 *   if (priv->use_action_appearance)
 *     {
 *       if (btk_action_get_stock_id (action))
 * 	foo_bar_set_stock (button, btk_action_get_stock_id (action));
 *       else if (btk_action_get_label (action))
 * 	foo_bar_set_label (button, btk_action_get_label (action));
 * 
 *       ...
 * 
 *     }
 * }
 * 
 * static void 
 * foo_bar_activatable_update (BtkActivatable       *activatable,
 * 			       BtkAction            *action,
 * 			       const gchar          *property_name)
 * {
 *   FooBarPrivate *priv = FOO_BAR_GET_PRIVATE (activatable);
 * 
 *   if (strcmp (property_name, "visible") == 0)
 *     {
 *       if (btk_action_is_visible (action))
 * 	btk_widget_show (BTK_WIDGET (activatable));
 *       else
 * 	btk_widget_hide (BTK_WIDGET (activatable));
 *     }
 *   else if (strcmp (property_name, "sensitive") == 0)
 *     btk_widget_set_sensitive (BTK_WIDGET (activatable), btk_action_is_sensitive (action));
 * 
 *   ...
 * 
 *   if (!priv->use_action_appearance)
 *     return;
 * 
 *   if (strcmp (property_name, "stock-id") == 0)
 *     foo_bar_set_stock (button, btk_action_get_stock_id (action));
 *   else if (strcmp (property_name, "label") == 0)
 *     foo_bar_set_label (button, btk_action_get_label (action));
 * 
 *   ...
 * }]]></programlisting>
 * </example>
 * </refsect2>
 */

#include "config.h"
#include "btkactivatable.h"
#include "btkactiongroup.h"
#include "btktypeutils.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"


static void btk_activatable_class_init (gpointer g_iface);

GType
btk_activatable_get_type (void)
{
  static GType activatable_type = 0;

  if (!activatable_type) {
    activatable_type =
      g_type_register_static_simple (B_TYPE_INTERFACE, I_("BtkActivatable"),
				     sizeof (BtkActivatableIface),
				     (GClassInitFunc) btk_activatable_class_init,
				     0, NULL, 0);

    g_type_interface_add_prerequisite (activatable_type, B_TYPE_OBJECT);
  }

  return activatable_type;
}

static void
btk_activatable_class_init (gpointer g_iface)
{
  /**
   * BtkActivatable:related-action:
   * 
   * The action that this activatable will activate and receive
   * updates from for various states and possibly appearance.
   *
   * <note><para>#BtkActivatable implementors need to handle the this property and 
   * call btk_activatable_do_set_related_action() when it changes.</para></note>
   *
   * Since: 2.16
   */
  g_object_interface_install_property (g_iface,
				       g_param_spec_object ("related-action",
							    P_("Related Action"),
							    P_("The action this activatable will activate and receive updates from"),
							    BTK_TYPE_ACTION,
							    BTK_PARAM_READWRITE));

  /**
   * BtkActivatable:use-action-appearance:
   * 
   * Whether this activatable should reset its layout
   * and appearance when setting the related action or when
   * the action changes appearance.
   *
   * See the #BtkAction documentation directly to find which properties
   * should be ignored by the #BtkActivatable when this property is %FALSE.
   *
   * <note><para>#BtkActivatable implementors need to handle this property
   * and call btk_activatable_sync_action_properties() on the activatable
   * widget when it changes.</para></note>
   *
   * Since: 2.16
   */
  g_object_interface_install_property (g_iface,
				       g_param_spec_boolean ("use-action-appearance",
							     P_("Use Action Appearance"),
							     P_("Whether to use the related actions appearance properties"),
							     TRUE,
							     BTK_PARAM_READWRITE));


}

static void
btk_activatable_update (BtkActivatable *activatable,
			BtkAction      *action,
			const gchar    *property_name)
{
  BtkActivatableIface *iface;

  g_return_if_fail (BTK_IS_ACTIVATABLE (activatable));

  iface = BTK_ACTIVATABLE_GET_IFACE (activatable);
  if (iface->update)
    iface->update (activatable, action, property_name);
  else
    g_critical ("BtkActivatable->update() unimplemented for type %s", 
		g_type_name (B_OBJECT_TYPE (activatable)));
}

/**
 * btk_activatable_sync_action_properties:
 * @activatable: a #BtkActivatable
 * @action: (allow-none): the related #BtkAction or %NULL
 *
 * This is called to update the activatable completely, this is called
 * internally when the #BtkActivatable::related-action property is set
 * or unset and by the implementing class when
 * #BtkActivatable::use-action-appearance changes.
 *
 * Since: 2.16
 **/
void
btk_activatable_sync_action_properties (BtkActivatable *activatable,
		                        BtkAction      *action)
{
  BtkActivatableIface *iface;

  g_return_if_fail (BTK_IS_ACTIVATABLE (activatable));

  iface = BTK_ACTIVATABLE_GET_IFACE (activatable);
  if (iface->sync_action_properties)
    iface->sync_action_properties (activatable, action);
  else
    g_critical ("BtkActivatable->sync_action_properties() unimplemented for type %s", 
		g_type_name (B_OBJECT_TYPE (activatable)));
}


/**
 * btk_activatable_set_related_action:
 * @activatable: a #BtkActivatable
 * @action: the #BtkAction to set
 *
 * Sets the related action on the @activatable object.
 *
 * <note><para>#BtkActivatable implementors need to handle the #BtkActivatable:related-action
 * property and call btk_activatable_do_set_related_action() when it changes.</para></note>
 *
 * Since: 2.16
 **/
void
btk_activatable_set_related_action (BtkActivatable *activatable,
				    BtkAction      *action)
{
  g_return_if_fail (BTK_IS_ACTIVATABLE (activatable));
  g_return_if_fail (action == NULL || BTK_IS_ACTION (action));

  g_object_set (activatable, "related-action", action, NULL);
}

static void
btk_activatable_action_notify (BtkAction      *action,
			       BParamSpec     *pspec,
			       BtkActivatable *activatable)
{
  btk_activatable_update (activatable, action, pspec->name);
}

/**
 * btk_activatable_do_set_related_action:
 * @activatable: a #BtkActivatable
 * @action: the #BtkAction to set
 * 
 * This is a utility function for #BtkActivatable implementors.
 * 
 * When implementing #BtkActivatable you must call this when
 * handling changes of the #BtkActivatable:related-action, and
 * you must also use this to break references in #BObject->dispose().
 *
 * This function adds a reference to the currently set related
 * action for you, it also makes sure the #BtkActivatable->update()
 * method is called when the related #BtkAction properties change
 * and registers to the action's proxy list.
 *
 * <note><para>Be careful to call this before setting the local
 * copy of the #BtkAction property, since this function uses 
 * btk_activatable_get_action() to retrieve the previous action</para></note>
 *
 * Since: 2.16
 */
void
btk_activatable_do_set_related_action (BtkActivatable *activatable,
				       BtkAction      *action)
{
  BtkAction *prev_action;

  prev_action = btk_activatable_get_related_action (activatable);
  
  if (prev_action != action)
    {
      if (prev_action)
	{
	  g_signal_handlers_disconnect_by_func (prev_action, btk_activatable_action_notify, activatable);
	  
          /* Check the type so that actions can be activatable too. */
          if (BTK_IS_WIDGET (activatable))
            _btk_action_remove_from_proxy_list (prev_action, BTK_WIDGET (activatable));
	  
          /* Some apps are using the object data directly...
           * so continue to set it for a bit longer
           */
          g_object_set_data (B_OBJECT (activatable), "btk-action", NULL);

          /*
           * We don't want prev_action to be activated
           * during the sync_action_properties() call when syncing "active".
           */ 
          btk_action_block_activate (prev_action);
	}
      
      /* Some applications rely on their proxy UI to be set up
       * before they receive the ::connect-proxy signal, so we
       * need to call sync_action_properties() before add_to_proxy_list().
       */
      btk_activatable_sync_action_properties (activatable, action);

      if (prev_action)
        {
          btk_action_unblock_activate (prev_action);
	  g_object_unref (prev_action);
        }

      if (action)
	{
	  g_object_ref (action);

	  g_signal_connect (B_OBJECT (action), "notify", G_CALLBACK (btk_activatable_action_notify), activatable);

          if (BTK_IS_WIDGET (activatable))
            _btk_action_add_to_proxy_list (action, BTK_WIDGET (activatable));

          g_object_set_data (B_OBJECT (activatable), "btk-action", action);
	}
    }
}

/**
 * btk_activatable_get_related_action:
 * @activatable: a #BtkActivatable
 *
 * Gets the related #BtkAction for @activatable.
 *
 * Returns: (transfer none): the related #BtkAction if one is set.
 *
 * Since: 2.16
 **/
BtkAction *
btk_activatable_get_related_action (BtkActivatable *activatable)
{
  BtkAction *action;

  g_return_val_if_fail (BTK_IS_ACTIVATABLE (activatable), NULL);

  g_object_get (activatable, "related-action", &action, NULL);

  /* g_object_get() gives us a ref... */
  if (action)
    g_object_unref (action);

  return action;
}

/**
 * btk_activatable_set_use_action_appearance:
 * @activatable: a #BtkActivatable
 * @use_appearance: whether to use the actions appearance
 *
 * Sets whether this activatable should reset its layout and appearance
 * when setting the related action or when the action changes appearance
 *
 * <note><para>#BtkActivatable implementors need to handle the
 * #BtkActivatable:use-action-appearance property and call
 * btk_activatable_sync_action_properties() to update @activatable
 * if needed.</para></note>
 *
 * Since: 2.16
 **/
void
btk_activatable_set_use_action_appearance (BtkActivatable *activatable,
					   gboolean        use_appearance)
{
  g_object_set (activatable, "use-action-appearance", use_appearance, NULL);
}

/**
 * btk_activatable_get_use_action_appearance:
 * @activatable: a #BtkActivatable
 *
 * Gets whether this activatable should reset its layout
 * and appearance when setting the related action or when
 * the action changes appearance.
 *
 * Returns: whether @activatable uses its actions appearance.
 *
 * Since: 2.16
 **/
gboolean
btk_activatable_get_use_action_appearance  (BtkActivatable *activatable)
{
  gboolean use_appearance;

  g_object_get (activatable, "use-action-appearance", &use_appearance, NULL);  

  return use_appearance;
}

#define __BTK_ACTIVATABLE_C__
#include "btkaliasdef.c"
