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

#include "config.h"
#include <string.h>

#include "btkactiongroup.h"
#include "btkbuildable.h"
#include "btkiconfactory.h"
#include "btkicontheme.h"
#include "btkstock.h"
#include "btktoggleaction.h"
#include "btkradioaction.h"
#include "btkaccelmap.h"
#include "btkmarshalers.h"
#include "btkbuilderprivate.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

#define BTK_ACTION_GROUP_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_ACTION_GROUP, BtkActionGroupPrivate))

struct _BtkActionGroupPrivate 
{
  gchar           *name;
  gboolean	   sensitive;
  gboolean	   visible;
  GHashTable      *actions;

  BtkTranslateFunc translate_func;
  gpointer         translate_data;
  GDestroyNotify   translate_notify;
};

enum 
{
  CONNECT_PROXY,
  DISCONNECT_PROXY,
  PRE_ACTIVATE,
  POST_ACTIVATE,
  LAST_SIGNAL
};

enum 
{
  PROP_0,
  PROP_NAME,
  PROP_SENSITIVE,
  PROP_VISIBLE
};

static void       btk_action_group_init            (BtkActionGroup      *self);
static void       btk_action_group_class_init      (BtkActionGroupClass *class);
static void       btk_action_group_finalize        (GObject             *object);
static void       btk_action_group_set_property    (GObject             *object,
						    guint                prop_id,
						    const GValue        *value,
						    GParamSpec          *pspec);
static void       btk_action_group_get_property    (GObject             *object,
						    guint                prop_id,
						    GValue              *value,
						    GParamSpec          *pspec);
static BtkAction *btk_action_group_real_get_action (BtkActionGroup      *self,
						    const gchar         *name);

/* BtkBuildable */
static void btk_action_group_buildable_init (BtkBuildableIface *iface);
static void btk_action_group_buildable_add_child (BtkBuildable  *buildable,
						  BtkBuilder    *builder,
						  GObject       *child,
						  const gchar   *type);
static void btk_action_group_buildable_set_name (BtkBuildable *buildable,
						 const gchar  *name);
static const gchar* btk_action_group_buildable_get_name (BtkBuildable *buildable);
static gboolean btk_action_group_buildable_custom_tag_start (BtkBuildable     *buildable,
							     BtkBuilder       *builder,
							     GObject          *child,
							     const gchar      *tagname,
							     GMarkupParser    *parser,
							     gpointer         *data);
static void btk_action_group_buildable_custom_tag_end (BtkBuildable *buildable,
						       BtkBuilder   *builder,
						       GObject      *child,
						       const gchar  *tagname,
						       gpointer     *user_data);

GType
btk_action_group_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      const GTypeInfo type_info =
      {
        sizeof (BtkActionGroupClass),
	NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) btk_action_group_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (BtkActionGroup),
        0, /* n_preallocs */
        (GInstanceInitFunc) btk_action_group_init,
      };

      const GInterfaceInfo buildable_info =
      {
	(GInterfaceInitFunc) btk_action_group_buildable_init,
	NULL,
	NULL
      };

      type = g_type_register_static (G_TYPE_OBJECT, I_("BtkActionGroup"),
				     &type_info, 0);

      g_type_add_interface_static (type,
				   BTK_TYPE_BUILDABLE,
				   &buildable_info);
    }
  return type;
}

static GObjectClass *parent_class = NULL;
static guint         action_group_signals[LAST_SIGNAL] = { 0 };

static void
btk_action_group_class_init (BtkActionGroupClass *klass)
{
  GObjectClass *bobject_class;

  bobject_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent (klass);

  bobject_class->finalize = btk_action_group_finalize;
  bobject_class->set_property = btk_action_group_set_property;
  bobject_class->get_property = btk_action_group_get_property;
  klass->get_action = btk_action_group_real_get_action;

  g_object_class_install_property (bobject_class,
				   PROP_NAME,
				   g_param_spec_string ("name",
							P_("Name"),
							P_("A name for the action group."),
							NULL,
							BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (bobject_class,
				   PROP_SENSITIVE,
				   g_param_spec_boolean ("sensitive",
							 P_("Sensitive"),
							 P_("Whether the action group is enabled."),
							 TRUE,
							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_VISIBLE,
				   g_param_spec_boolean ("visible",
							 P_("Visible"),
							 P_("Whether the action group is visible."),
							 TRUE,
							 BTK_PARAM_READWRITE));

  /**
   * BtkActionGroup::connect-proxy:
   * @action_group: the group
   * @action: the action
   * @proxy: the proxy
   *
   * The ::connect-proxy signal is emitted after connecting a proxy to 
   * an action in the group. Note that the proxy may have been connected 
   * to a different action before.
   *
   * This is intended for simple customizations for which a custom action
   * class would be too clumsy, e.g. showing tooltips for menuitems in the
   * statusbar.
   *
   * #BtkUIManager proxies the signal and provides global notification 
   * just before any action is connected to a proxy, which is probably more
   * convenient to use.
   *
   * Since: 2.4
   */
  action_group_signals[CONNECT_PROXY] =
    g_signal_new (I_("connect-proxy"),
		  G_OBJECT_CLASS_TYPE (klass),
		  0, 0, NULL, NULL,
		  _btk_marshal_VOID__OBJECT_OBJECT,
		  G_TYPE_NONE, 2,
		  BTK_TYPE_ACTION, BTK_TYPE_WIDGET);

  /**
   * BtkActionGroup::disconnect-proxy:
   * @action_group: the group
   * @action: the action
   * @proxy: the proxy
   *
   * The ::disconnect-proxy signal is emitted after disconnecting a proxy 
   * from an action in the group. 
   *
   * #BtkUIManager proxies the signal and provides global notification 
   * just before any action is connected to a proxy, which is probably more
   * convenient to use.
   *
   * Since: 2.4
   */
  action_group_signals[DISCONNECT_PROXY] =
    g_signal_new (I_("disconnect-proxy"),
		  G_OBJECT_CLASS_TYPE (klass),
		  0, 0, NULL, NULL,
		  _btk_marshal_VOID__OBJECT_OBJECT,
		  G_TYPE_NONE, 2, 
		  BTK_TYPE_ACTION, BTK_TYPE_WIDGET);

  /**
   * BtkActionGroup::pre-activate:
   * @action_group: the group
   * @action: the action
   *
   * The ::pre-activate signal is emitted just before the @action in the
   * @action_group is activated
   *
   * This is intended for #BtkUIManager to proxy the signal and provide global
   * notification just before any action is activated.
   *
   * Since: 2.4
   */
  action_group_signals[PRE_ACTIVATE] =
    g_signal_new (I_("pre-activate"),
		  G_OBJECT_CLASS_TYPE (klass),
		  0, 0, NULL, NULL,
		  _btk_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1, 
		  BTK_TYPE_ACTION);

  /**
   * BtkActionGroup::post-activate:
   * @action_group: the group
   * @action: the action
   *
   * The ::post-activate signal is emitted just after the @action in the
   * @action_group is activated
   *
   * This is intended for #BtkUIManager to proxy the signal and provide global
   * notification just after any action is activated.
   *
   * Since: 2.4
   */
  action_group_signals[POST_ACTIVATE] =
    g_signal_new (I_("post-activate"),
		  G_OBJECT_CLASS_TYPE (klass),
		  0, 0, NULL, NULL,
		  _btk_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1, 
		  BTK_TYPE_ACTION);

  g_type_class_add_private (bobject_class, sizeof (BtkActionGroupPrivate));
}


static void 
remove_action (BtkAction *action) 
{
  g_object_set (action, I_("action-group"), NULL, NULL);
  g_object_unref (action);
}

static void
btk_action_group_init (BtkActionGroup *self)
{
  BtkActionGroupPrivate *private;

  private = BTK_ACTION_GROUP_GET_PRIVATE (self);

  private->name = NULL;
  private->sensitive = TRUE;
  private->visible = TRUE;
  private->actions = g_hash_table_new_full (g_str_hash, g_str_equal,
                                            NULL,
                                            (GDestroyNotify) remove_action);
  private->translate_func = NULL;
  private->translate_data = NULL;
  private->translate_notify = NULL;
}

static void
btk_action_group_buildable_init (BtkBuildableIface *iface)
{
  iface->add_child = btk_action_group_buildable_add_child;
  iface->set_name = btk_action_group_buildable_set_name;
  iface->get_name = btk_action_group_buildable_get_name;
  iface->custom_tag_start = btk_action_group_buildable_custom_tag_start;
  iface->custom_tag_end = btk_action_group_buildable_custom_tag_end;
}

static void
btk_action_group_buildable_add_child (BtkBuildable  *buildable,
				      BtkBuilder    *builder,
				      GObject       *child,
				      const gchar   *type)
{
  btk_action_group_add_action_with_accel (BTK_ACTION_GROUP (buildable),
					  BTK_ACTION (child), NULL);
}

static void
btk_action_group_buildable_set_name (BtkBuildable *buildable,
				     const gchar  *name)
{
  BtkActionGroup *self = BTK_ACTION_GROUP (buildable);
  BtkActionGroupPrivate *private = BTK_ACTION_GROUP_GET_PRIVATE (self);

  private->name = g_strdup (name);
}

static const gchar *
btk_action_group_buildable_get_name (BtkBuildable *buildable)
{
  BtkActionGroup *self = BTK_ACTION_GROUP (buildable);
  BtkActionGroupPrivate *private = BTK_ACTION_GROUP_GET_PRIVATE (self);
  return private->name;
}

typedef struct {
  GObject         *child;
  guint            key;
  BdkModifierType  modifiers;
} AcceleratorParserData;

static void
accelerator_start_element (GMarkupParseContext *context,
			   const gchar         *element_name,
			   const gchar        **names,
			   const gchar        **values,
			   gpointer             user_data,
			   GError             **error)
{
  gint i;
  guint key = 0;
  BdkModifierType modifiers = 0;
  AcceleratorParserData *parser_data = (AcceleratorParserData*)user_data;

  if (strcmp (element_name, "accelerator") != 0)
    g_warning ("Unknown <accelerator> tag: %s", element_name);

  for (i = 0; names[i]; i++)
    {
      if (strcmp (names[i], "key") == 0)
	key = bdk_keyval_from_name (values[i]);
      else if (strcmp (names[i], "modifiers") == 0)
	{
	  if (!_btk_builder_flags_from_string (BDK_TYPE_MODIFIER_TYPE,
					       values[i],
					       &modifiers,
					       error))
	      return;
	}
    }

  if (key == 0)
    {
      g_warning ("<accelerator> requires a key attribute");
      return;
    }
  parser_data->key = key;
  parser_data->modifiers = modifiers;
}

static const GMarkupParser accelerator_parser =
  {
    accelerator_start_element
  };

static gboolean
btk_action_group_buildable_custom_tag_start (BtkBuildable     *buildable,
					     BtkBuilder       *builder,
					     GObject          *child,
					     const gchar      *tagname,
					     GMarkupParser    *parser,
					     gpointer         *user_data)
{
  AcceleratorParserData *parser_data;

  if (child && strcmp (tagname, "accelerator") == 0)
    {
      parser_data = g_slice_new0 (AcceleratorParserData);
      parser_data->child = child;
      *user_data = parser_data;
      *parser = accelerator_parser;

      return TRUE;
    }
  return FALSE;
}

static void
btk_action_group_buildable_custom_tag_end (BtkBuildable *buildable,
					   BtkBuilder   *builder,
					   GObject      *child,
					   const gchar  *tagname,
					   gpointer     *user_data)
{
  AcceleratorParserData *data;
  
  if (strcmp (tagname, "accelerator") == 0)
    {
      BtkActionGroup *action_group;
      BtkActionGroupPrivate *private;
      BtkAction *action;
      gchar *accel_path;
      
      data = (AcceleratorParserData*)user_data;
      action_group = BTK_ACTION_GROUP (buildable);
      private = BTK_ACTION_GROUP_GET_PRIVATE (action_group);
      action = BTK_ACTION (child);
	
      accel_path = g_strconcat ("<Actions>/",
				private->name, "/",
				btk_action_get_name (action), NULL);

      if (btk_accel_map_lookup_entry (accel_path, NULL))
	btk_accel_map_change_entry (accel_path, data->key, data->modifiers, TRUE);
      else
	btk_accel_map_add_entry (accel_path, data->key, data->modifiers);

      btk_action_set_accel_path (action, accel_path);
      
      g_free (accel_path);
      g_slice_free (AcceleratorParserData, data);
    }
}

/**
 * btk_action_group_new:
 * @name: the name of the action group.
 *
 * Creates a new #BtkActionGroup object. The name of the action group
 * is used when associating <link linkend="Action-Accel">keybindings</link> 
 * with the actions.
 *
 * Returns: the new #BtkActionGroup
 *
 * Since: 2.4
 */
BtkActionGroup *
btk_action_group_new (const gchar *name)
{
  BtkActionGroup *self;
  BtkActionGroupPrivate *private;

  self = g_object_new (BTK_TYPE_ACTION_GROUP, NULL);
  private = BTK_ACTION_GROUP_GET_PRIVATE (self);
  private->name = g_strdup (name);

  return self;
}

static void
btk_action_group_finalize (GObject *object)
{
  BtkActionGroup *self;
  BtkActionGroupPrivate *private;

  self = BTK_ACTION_GROUP (object);
  private = BTK_ACTION_GROUP_GET_PRIVATE (self);

  g_free (private->name);
  private->name = NULL;

  g_hash_table_destroy (private->actions);
  private->actions = NULL;

  if (private->translate_notify)
    private->translate_notify (private->translate_data);

  parent_class->finalize (object);
}

static void
btk_action_group_set_property (GObject         *object,
			       guint            prop_id,
			       const GValue    *value,
			       GParamSpec      *pspec)
{
  BtkActionGroup *self;
  BtkActionGroupPrivate *private;
  gchar *tmp;
  
  self = BTK_ACTION_GROUP (object);
  private = BTK_ACTION_GROUP_GET_PRIVATE (self);

  switch (prop_id)
    {
    case PROP_NAME:
      tmp = private->name;
      private->name = g_value_dup_string (value);
      g_free (tmp);
      break;
    case PROP_SENSITIVE:
      btk_action_group_set_sensitive (self, g_value_get_boolean (value));
      break;
    case PROP_VISIBLE:
      btk_action_group_set_visible (self, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_action_group_get_property (GObject    *object,
			       guint       prop_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
  BtkActionGroup *self;
  BtkActionGroupPrivate *private;
  
  self = BTK_ACTION_GROUP (object);
  private = BTK_ACTION_GROUP_GET_PRIVATE (self);

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, private->name);
      break;
    case PROP_SENSITIVE:
      g_value_set_boolean (value, private->sensitive);
      break;
    case PROP_VISIBLE:
      g_value_set_boolean (value, private->visible);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static BtkAction *
btk_action_group_real_get_action (BtkActionGroup *self,
				  const gchar    *action_name)
{
  BtkActionGroupPrivate *private;

  private = BTK_ACTION_GROUP_GET_PRIVATE (self);

  return g_hash_table_lookup (private->actions, action_name);
}

/**
 * btk_action_group_get_name:
 * @action_group: the action group
 *
 * Gets the name of the action group.
 *
 * Returns: the name of the action group.
 * 
 * Since: 2.4
 */
const gchar *
btk_action_group_get_name (BtkActionGroup *action_group)
{
  BtkActionGroupPrivate *private;

  g_return_val_if_fail (BTK_IS_ACTION_GROUP (action_group), NULL);

  private = BTK_ACTION_GROUP_GET_PRIVATE (action_group);

  return private->name;
}

/**
 * btk_action_group_get_sensitive:
 * @action_group: the action group
 *
 * Returns %TRUE if the group is sensitive.  The constituent actions
 * can only be logically sensitive (see btk_action_is_sensitive()) if
 * they are sensitive (see btk_action_get_sensitive()) and their group
 * is sensitive.
 * 
 * Return value: %TRUE if the group is sensitive.
 *
 * Since: 2.4
 */
gboolean
btk_action_group_get_sensitive (BtkActionGroup *action_group)
{
  BtkActionGroupPrivate *private;

  g_return_val_if_fail (BTK_IS_ACTION_GROUP (action_group), FALSE);

  private = BTK_ACTION_GROUP_GET_PRIVATE (action_group);

  return private->sensitive;
}

static void
cb_set_action_sensitivity (const gchar *name, 
			   BtkAction   *action)
{
  /* Minor optimization, the action_groups state only affects actions 
   * that are themselves sensitive */
  g_object_notify (G_OBJECT (action), "sensitive");

}

/**
 * btk_action_group_set_sensitive:
 * @action_group: the action group
 * @sensitive: new sensitivity
 *
 * Changes the sensitivity of @action_group
 * 
 * Since: 2.4
 */
void
btk_action_group_set_sensitive (BtkActionGroup *action_group, 
				gboolean        sensitive)
{
  BtkActionGroupPrivate *private;

  g_return_if_fail (BTK_IS_ACTION_GROUP (action_group));

  private = BTK_ACTION_GROUP_GET_PRIVATE (action_group);
  sensitive = sensitive != FALSE;

  if (private->sensitive != sensitive)
    {
      private->sensitive = sensitive;
      g_hash_table_foreach (private->actions, 
			    (GHFunc) cb_set_action_sensitivity, NULL);

      g_object_notify (G_OBJECT (action_group), "sensitive");
    }
}

/**
 * btk_action_group_get_visible:
 * @action_group: the action group
 *
 * Returns %TRUE if the group is visible.  The constituent actions
 * can only be logically visible (see btk_action_is_visible()) if
 * they are visible (see btk_action_get_visible()) and their group
 * is visible.
 * 
 * Return value: %TRUE if the group is visible.
 * 
 * Since: 2.4
 */
gboolean
btk_action_group_get_visible (BtkActionGroup *action_group)
{
  BtkActionGroupPrivate *private;

  g_return_val_if_fail (BTK_IS_ACTION_GROUP (action_group), FALSE);

  private = BTK_ACTION_GROUP_GET_PRIVATE (action_group);

  return private->visible;
}

static void
cb_set_action_visiblity (const gchar *name, 
			 BtkAction   *action)
{
  /* Minor optimization, the action_groups state only affects actions 
   * that are themselves visible */
  g_object_notify (G_OBJECT (action), "visible");
}

/**
 * btk_action_group_set_visible:
 * @action_group: the action group
 * @visible: new visiblity
 *
 * Changes the visible of @action_group.
 * 
 * Since: 2.4
 */
void
btk_action_group_set_visible (BtkActionGroup *action_group, 
			      gboolean        visible)
{
  BtkActionGroupPrivate *private;

  g_return_if_fail (BTK_IS_ACTION_GROUP (action_group));

  private = BTK_ACTION_GROUP_GET_PRIVATE (action_group);
  visible = visible != FALSE;

  if (private->visible != visible)
    {
      private->visible = visible;
      g_hash_table_foreach (private->actions, 
			    (GHFunc) cb_set_action_visiblity, NULL);

      g_object_notify (G_OBJECT (action_group), "visible");
    }
}

/**
 * btk_action_group_get_action:
 * @action_group: the action group
 * @action_name: the name of the action
 *
 * Looks up an action in the action group by name.
 *
 * Returns: (transfer none): the action, or %NULL if no action by that name exists
 *
 * Since: 2.4
 */
BtkAction *
btk_action_group_get_action (BtkActionGroup *action_group,
			     const gchar    *action_name)
{
  g_return_val_if_fail (BTK_IS_ACTION_GROUP (action_group), NULL);
  g_return_val_if_fail (BTK_ACTION_GROUP_GET_CLASS (action_group)->get_action != NULL, NULL);

  return BTK_ACTION_GROUP_GET_CLASS (action_group)->get_action (action_group,
                                                                action_name);
}

static gboolean
check_unique_action (BtkActionGroup *action_group,
	             const gchar    *action_name)
{
  if (btk_action_group_get_action (action_group, action_name) != NULL)
    {
      BtkActionGroupPrivate *private;

      private = BTK_ACTION_GROUP_GET_PRIVATE (action_group);

      g_warning ("Refusing to add non-unique action '%s' to action group '%s'",
	 	 action_name,
		 private->name);
      return FALSE;
    }

  return TRUE;
}

/**
 * btk_action_group_add_action:
 * @action_group: the action group
 * @action: an action
 *
 * Adds an action object to the action group. Note that this function
 * does not set up the accel path of the action, which can lead to problems
 * if a user tries to modify the accelerator of a menuitem associated with
 * the action. Therefore you must either set the accel path yourself with
 * btk_action_set_accel_path(), or use 
 * <literal>btk_action_group_add_action_with_accel (..., NULL)</literal>.
 *
 * Since: 2.4
 */
void
btk_action_group_add_action (BtkActionGroup *action_group,
			     BtkAction      *action)
{
  BtkActionGroupPrivate *private;
  const gchar *name;

  g_return_if_fail (BTK_IS_ACTION_GROUP (action_group));
  g_return_if_fail (BTK_IS_ACTION (action));

  name = btk_action_get_name (action);
  g_return_if_fail (name != NULL);
  
  if (!check_unique_action (action_group, name))
    return;

  private = BTK_ACTION_GROUP_GET_PRIVATE (action_group);

  g_hash_table_insert (private->actions, 
		       (gpointer) name,
                       g_object_ref (action));
  g_object_set (action, I_("action-group"), action_group, NULL);
}

/**
 * btk_action_group_add_action_with_accel:
 * @action_group: the action group
 * @action: the action to add
 * @accelerator: (allow-none): the accelerator for the action, in
 *   the format understood by btk_accelerator_parse(), or "" for no accelerator, or
 *   %NULL to use the stock accelerator
 *
 * Adds an action object to the action group and sets up the accelerator.
 *
 * If @accelerator is %NULL, attempts to use the accelerator associated 
 * with the stock_id of the action. 
 *
 * Accel paths are set to
 * <literal>&lt;Actions&gt;/<replaceable>group-name</replaceable>/<replaceable>action-name</replaceable></literal>.
 *
 * Since: 2.4
 */
void
btk_action_group_add_action_with_accel (BtkActionGroup *action_group,
					BtkAction      *action,
					const gchar    *accelerator)
{
  BtkActionGroupPrivate *private;
  gchar *accel_path;
  guint  accel_key = 0;
  BdkModifierType accel_mods;
  const gchar *name;

  name = btk_action_get_name (action);
  if (!check_unique_action (action_group, name))
    return;

  private = BTK_ACTION_GROUP_GET_PRIVATE (action_group);
  accel_path = g_strconcat ("<Actions>/",
			    private->name, "/", name, NULL);

  if (accelerator)
    {
      if (accelerator[0] == 0) 
	accel_key = 0;
      else
	{
	  btk_accelerator_parse (accelerator, &accel_key, &accel_mods);
	  if (accel_key == 0)
	    g_warning ("Unable to parse accelerator '%s' for action '%s'",
		       accelerator, name);
	}
    }
  else 
    {
      gchar *stock_id;
      BtkStockItem stock_item;

      g_object_get (action, "stock-id", &stock_id, NULL);

      if (stock_id && btk_stock_lookup (stock_id, &stock_item))
        {
          accel_key = stock_item.keyval;
          accel_mods = stock_item.modifier;
	}

      g_free (stock_id);
    }

  if (accel_key)
    btk_accel_map_add_entry (accel_path, accel_key, accel_mods);

  btk_action_set_accel_path (action, accel_path);
  btk_action_group_add_action (action_group, action);

  g_free (accel_path);
}

/**
 * btk_action_group_remove_action:
 * @action_group: the action group
 * @action: an action
 *
 * Removes an action object from the action group.
 *
 * Since: 2.4
 */
void
btk_action_group_remove_action (BtkActionGroup *action_group,
				BtkAction      *action)
{
  BtkActionGroupPrivate *private;
  const gchar *name;

  g_return_if_fail (BTK_IS_ACTION_GROUP (action_group));
  g_return_if_fail (BTK_IS_ACTION (action));

  name = btk_action_get_name (action);
  g_return_if_fail (name != NULL);

  private = BTK_ACTION_GROUP_GET_PRIVATE (action_group);

  g_hash_table_remove (private->actions, name);
}

static void
add_single_action (gpointer key, 
		   gpointer value, 
		   gpointer user_data)
{
  GList **list = user_data;

  *list = g_list_prepend (*list, value);
}

/**
 * btk_action_group_list_actions:
 * @action_group: the action group
 *
 * Lists the actions in the action group.
 *
 * Returns: (element-type BtkAction) (transfer container): an allocated list of the action objects in the action group
 *
 * Since: 2.4
 */
GList *
btk_action_group_list_actions (BtkActionGroup *action_group)
{
  BtkActionGroupPrivate *private;
  GList *actions = NULL;

  g_return_val_if_fail (BTK_IS_ACTION_GROUP (action_group), NULL);

  private = BTK_ACTION_GROUP_GET_PRIVATE (action_group);
  
  g_hash_table_foreach (private->actions, add_single_action, &actions);

  return g_list_reverse (actions);
}


/**
 * btk_action_group_add_actions:
 * @action_group: the action group
 * @entries: an array of action descriptions
 * @n_entries: the number of entries
 * @user_data: data to pass to the action callbacks
 *
 * This is a convenience function to create a number of actions and add them 
 * to the action group.
 *
 * The "activate" signals of the actions are connected to the callbacks and 
 * their accel paths are set to 
 * <literal>&lt;Actions&gt;/<replaceable>group-name</replaceable>/<replaceable>action-name</replaceable></literal>.  
 * 
 * Since: 2.4
 */
void
btk_action_group_add_actions (BtkActionGroup       *action_group,
			      const BtkActionEntry *entries,
			      guint                 n_entries,
			      gpointer              user_data)
{
  btk_action_group_add_actions_full (action_group, 
				     entries, n_entries, 
				     user_data, NULL);
}

typedef struct _SharedData  SharedData;

struct _SharedData {
  guint          ref_count;
  gpointer       data;
  GDestroyNotify destroy;
};

static void
shared_data_unref (gpointer data)
{
  SharedData *shared_data = (SharedData *)data;

  shared_data->ref_count--;
  if (shared_data->ref_count == 0)
    {
      if (shared_data->destroy)
	shared_data->destroy (shared_data->data);

      g_slice_free (SharedData, shared_data);
    }
}


/**
 * btk_action_group_add_actions_full:
 * @action_group: the action group
 * @entries: an array of action descriptions
 * @n_entries: the number of entries
 * @user_data: data to pass to the action callbacks
 * @destroy: destroy notification callback for @user_data
 *
 * This variant of btk_action_group_add_actions() adds a #GDestroyNotify
 * callback for @user_data. 
 * 
 * Since: 2.4
 */
void
btk_action_group_add_actions_full (BtkActionGroup       *action_group,
				   const BtkActionEntry *entries,
				   guint                 n_entries,
				   gpointer              user_data,
				   GDestroyNotify        destroy)
{

  /* Keep this in sync with the other 
   * btk_action_group_add_..._actions_full() functions.
   */
  guint i;
  SharedData *shared_data;

  g_return_if_fail (BTK_IS_ACTION_GROUP (action_group));

  shared_data = g_slice_new0 (SharedData);
  shared_data->ref_count = 1;
  shared_data->data = user_data;
  shared_data->destroy = destroy;

  for (i = 0; i < n_entries; i++)
    {
      BtkAction *action;
      const gchar *label;
      const gchar *tooltip;

      if (!check_unique_action (action_group, entries[i].name))
        continue;

      label = btk_action_group_translate_string (action_group, entries[i].label);
      tooltip = btk_action_group_translate_string (action_group, entries[i].tooltip);

      action = btk_action_new (entries[i].name,
			       label,
			       tooltip,
			       NULL);

      if (entries[i].stock_id) 
	{
	  g_object_set (action, "stock-id", entries[i].stock_id, NULL);
	  if (btk_icon_theme_has_icon (btk_icon_theme_get_default (), 
				       entries[i].stock_id))
	    g_object_set (action, "icon-name", entries[i].stock_id, NULL);
	}
	  
      if (entries[i].callback)
	{
	  GClosure *closure;

	  closure = g_cclosure_new (entries[i].callback, user_data, NULL);
	  g_closure_add_finalize_notifier (closure, shared_data, 
					   (GClosureNotify)shared_data_unref);
	  shared_data->ref_count++;

	  g_signal_connect_closure (action, "activate", closure, FALSE);
	}
	  
      btk_action_group_add_action_with_accel (action_group, 
					      action,
					      entries[i].accelerator);
      g_object_unref (action);
    }

  shared_data_unref (shared_data);
}

/**
 * btk_action_group_add_toggle_actions:
 * @action_group: the action group
 * @entries: an array of toggle action descriptions
 * @n_entries: the number of entries
 * @user_data: data to pass to the action callbacks
 *
 * This is a convenience function to create a number of toggle actions and add them 
 * to the action group.
 *
 * The "activate" signals of the actions are connected to the callbacks and 
 * their accel paths are set to 
 * <literal>&lt;Actions&gt;/<replaceable>group-name</replaceable>/<replaceable>action-name</replaceable></literal>.  
 * 
 * Since: 2.4
 */
void
btk_action_group_add_toggle_actions (BtkActionGroup             *action_group,
				     const BtkToggleActionEntry *entries,
				     guint                       n_entries,
				     gpointer                    user_data)
{
  btk_action_group_add_toggle_actions_full (action_group, 
					    entries, n_entries, 
					    user_data, NULL);
}


/**
 * btk_action_group_add_toggle_actions_full:
 * @action_group: the action group
 * @entries: an array of toggle action descriptions
 * @n_entries: the number of entries
 * @user_data: data to pass to the action callbacks
 * @destroy: destroy notification callback for @user_data
 *
 * This variant of btk_action_group_add_toggle_actions() adds a 
 * #GDestroyNotify callback for @user_data. 
 * 
 * Since: 2.4
 */
void
btk_action_group_add_toggle_actions_full (BtkActionGroup             *action_group,
					  const BtkToggleActionEntry *entries,
					  guint                       n_entries,
					  gpointer                    user_data,
					  GDestroyNotify              destroy)
{
  /* Keep this in sync with the other 
   * btk_action_group_add_..._actions_full() functions.
   */
  guint i;
  SharedData *shared_data;

  g_return_if_fail (BTK_IS_ACTION_GROUP (action_group));

  shared_data = g_slice_new0 (SharedData);
  shared_data->ref_count = 1;
  shared_data->data = user_data;
  shared_data->destroy = destroy;

  for (i = 0; i < n_entries; i++)
    {
      BtkToggleAction *action;
      const gchar *label;
      const gchar *tooltip;

      if (!check_unique_action (action_group, entries[i].name))
        continue;

      label = btk_action_group_translate_string (action_group, entries[i].label);
      tooltip = btk_action_group_translate_string (action_group, entries[i].tooltip);

      action = btk_toggle_action_new (entries[i].name,
				      label,
				      tooltip,
				      NULL);

      if (entries[i].stock_id) 
	{
	  if (btk_icon_factory_lookup_default (entries[i].stock_id))
	    g_object_set (action, "stock-id", entries[i].stock_id, NULL);
	  else
	    g_object_set (action, "icon-name", entries[i].stock_id, NULL);
	}

      btk_toggle_action_set_active (action, entries[i].is_active);

      if (entries[i].callback)
	{
	  GClosure *closure;

	  closure = g_cclosure_new (entries[i].callback, user_data, NULL);
	  g_closure_add_finalize_notifier (closure, shared_data, 
					   (GClosureNotify)shared_data_unref);
	  shared_data->ref_count++;

	  g_signal_connect_closure (action, "activate", closure, FALSE);
	}
	  
      btk_action_group_add_action_with_accel (action_group, 
					      BTK_ACTION (action),
					      entries[i].accelerator);
      g_object_unref (action);
    }

  shared_data_unref (shared_data);
}

/**
 * btk_action_group_add_radio_actions:
 * @action_group: the action group
 * @entries: an array of radio action descriptions
 * @n_entries: the number of entries
 * @value: the value of the action to activate initially, or -1 if
 *   no action should be activated
 * @on_change: the callback to connect to the changed signal
 * @user_data: data to pass to the action callbacks
 * 
 * This is a convenience routine to create a group of radio actions and
 * add them to the action group. 
 *
 * The "changed" signal of the first radio action is connected to the 
 * @on_change callback and the accel paths of the actions are set to 
 * <literal>&lt;Actions&gt;/<replaceable>group-name</replaceable>/<replaceable>action-name</replaceable></literal>.  
 * 
 * Since: 2.4
 **/
void            
btk_action_group_add_radio_actions (BtkActionGroup            *action_group,
				    const BtkRadioActionEntry *entries,
				    guint                      n_entries,
				    gint                       value,
				    GCallback                  on_change,
				    gpointer                   user_data)
{
  btk_action_group_add_radio_actions_full (action_group, 
					   entries, n_entries, 
					   value,
					   on_change, user_data, NULL);
}

/**
 * btk_action_group_add_radio_actions_full:
 * @action_group: the action group
 * @entries: an array of radio action descriptions
 * @n_entries: the number of entries
 * @value: the value of the action to activate initially, or -1 if
 *   no action should be activated
 * @on_change: the callback to connect to the changed signal
 * @user_data: data to pass to the action callbacks
 * @destroy: destroy notification callback for @user_data
 *
 * This variant of btk_action_group_add_radio_actions() adds a 
 * #GDestroyNotify callback for @user_data. 
 * 
 * Since: 2.4
 **/
void            
btk_action_group_add_radio_actions_full (BtkActionGroup            *action_group,
					 const BtkRadioActionEntry *entries,
					 guint                      n_entries,
					 gint                       value,
					 GCallback                  on_change,
					 gpointer                   user_data,
					 GDestroyNotify             destroy)
{
  /* Keep this in sync with the other 
   * btk_action_group_add_..._actions_full() functions.
   */
  guint i;
  GSList *group = NULL;
  BtkRadioAction *first_action = NULL;

  g_return_if_fail (BTK_IS_ACTION_GROUP (action_group));

  for (i = 0; i < n_entries; i++)
    {
      BtkRadioAction *action;
      const gchar *label;
      const gchar *tooltip; 

      if (!check_unique_action (action_group, entries[i].name))
        continue;

      label = btk_action_group_translate_string (action_group, entries[i].label);
      tooltip = btk_action_group_translate_string (action_group, entries[i].tooltip);

      action = btk_radio_action_new (entries[i].name,
				     label,
				     tooltip,
				     NULL,
				     entries[i].value);

      if (entries[i].stock_id) 
	{
	  if (btk_icon_factory_lookup_default (entries[i].stock_id))
	    g_object_set (action, "stock-id", entries[i].stock_id, NULL);
	  else
	    g_object_set (action, "icon-name", entries[i].stock_id, NULL);
	}

      if (i == 0) 
	first_action = action;

      btk_radio_action_set_group (action, group);
      group = btk_radio_action_get_group (action);

      if (value == entries[i].value)
	btk_toggle_action_set_active (BTK_TOGGLE_ACTION (action), TRUE);

      btk_action_group_add_action_with_accel (action_group, 
					      BTK_ACTION (action),
					      entries[i].accelerator);
      g_object_unref (action);
    }

  if (on_change && first_action)
    g_signal_connect_data (first_action, "changed",
			   on_change, user_data, 
			   (GClosureNotify)destroy, 0);
}

/**
 * btk_action_group_set_translate_func:
 * @action_group: a #BtkActionGroup
 * @func: a #BtkTranslateFunc
 * @data: data to be passed to @func and @notify
 * @notify: a #GDestroyNotify function to be called when @action_group is
 *   destroyed and when the translation function is changed again
 *
 * Sets a function to be used for translating the @label and @tooltip of 
 * #BtkActionGroupEntry<!-- -->s added by btk_action_group_add_actions().
 *
 * If you're using gettext(), it is enough to set the translation domain
 * with btk_action_group_set_translation_domain().
 *
 * Since: 2.4 
 **/
void
btk_action_group_set_translate_func (BtkActionGroup   *action_group,
				     BtkTranslateFunc  func,
				     gpointer          data,
				     GDestroyNotify    notify)
{
  BtkActionGroupPrivate *private;

  g_return_if_fail (BTK_IS_ACTION_GROUP (action_group));
  
  private = BTK_ACTION_GROUP_GET_PRIVATE (action_group);

  if (private->translate_notify)
    private->translate_notify (private->translate_data);
      
  private->translate_func = func;
  private->translate_data = data;
  private->translate_notify = notify;
}

static gchar *
dgettext_swapped (const gchar *msgid, 
		  const gchar *domainname)
{
  /* Pass through g_dgettext if and only if msgid is nonempty. */
  if (msgid && *msgid) 
    return (gchar*) g_dgettext (domainname, msgid); 
  else
    return (gchar*) msgid;
}

/**
 * btk_action_group_set_translation_domain:
 * @action_group: a #BtkActionGroup
 * @domain: the translation domain to use for g_dgettext() calls
 * 
 * Sets the translation domain and uses g_dgettext() for translating the 
 * @label and @tooltip of #BtkActionEntry<!-- -->s added by 
 * btk_action_group_add_actions().
 *
 * If you're not using gettext() for localization, see 
 * btk_action_group_set_translate_func().
 *
 * Since: 2.4
 **/
void 
btk_action_group_set_translation_domain (BtkActionGroup *action_group,
					 const gchar    *domain)
{
  g_return_if_fail (BTK_IS_ACTION_GROUP (action_group));

  btk_action_group_set_translate_func (action_group, 
				       (BtkTranslateFunc)dgettext_swapped,
				       g_strdup (domain),
				       g_free);
} 


/**
 * btk_action_group_translate_string:
 * @action_group: a #BtkActionGroup
 * @string: a string
 *
 * Translates a string using the specified translate_func(). This
 * is mainly intended for language bindings.
 *
 * Returns: the translation of @string
 *
 * Since: 2.6
 **/
const gchar *
btk_action_group_translate_string (BtkActionGroup *action_group,
				   const gchar    *string)
{
  BtkActionGroupPrivate *private;
  BtkTranslateFunc translate_func;
  gpointer translate_data;
  
  g_return_val_if_fail (BTK_IS_ACTION_GROUP (action_group), string);
  
  if (string == NULL)
    return NULL;

  private = BTK_ACTION_GROUP_GET_PRIVATE (action_group);

  translate_func = private->translate_func;
  translate_data = private->translate_data;
  
  if (translate_func)
    return translate_func (string, translate_data);
  else
    return string;
}

/* Protected for use by BtkAction */
void
_btk_action_group_emit_connect_proxy  (BtkActionGroup *action_group,
                                       BtkAction      *action,
                                       BtkWidget      *proxy)
{
  g_signal_emit (action_group, action_group_signals[CONNECT_PROXY], 0, 
                 action, proxy);
}

void
_btk_action_group_emit_disconnect_proxy  (BtkActionGroup *action_group,
                                          BtkAction      *action,
                                          BtkWidget      *proxy)
{
  g_signal_emit (action_group, action_group_signals[DISCONNECT_PROXY], 0, 
                 action, proxy);
}

void
_btk_action_group_emit_pre_activate  (BtkActionGroup *action_group,
				      BtkAction      *action)
{
  g_signal_emit (action_group, action_group_signals[PRE_ACTIVATE], 0, action);
}

void
_btk_action_group_emit_post_activate (BtkActionGroup *action_group,
				      BtkAction      *action)
{
  g_signal_emit (action_group, action_group_signals[POST_ACTIVATE], 0, action);
}

#define __BTK_ACTION_GROUP_C__
#include "btkaliasdef.c"
