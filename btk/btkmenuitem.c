/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#define BTK_MENU_INTERNALS

#include "config.h"
#include <string.h>

#include "btkaccellabel.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkmenu.h"
#include "btkmenubar.h"
#include "btkseparatormenuitem.h"
#include "btkprivate.h"
#include "btkbuildable.h"
#include "btkactivatable.h"
#include "btkintl.h"
#include "btkalias.h"


typedef struct {
  BtkAction *action;
  gboolean   use_action_appearance;
} BtkMenuItemPrivate;

enum {
  ACTIVATE,
  ACTIVATE_ITEM,
  TOGGLE_SIZE_REQUEST,
  TOGGLE_SIZE_ALLOCATE,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_RIGHT_JUSTIFIED,
  PROP_SUBMENU,
  PROP_ACCEL_PATH,
  PROP_LABEL,
  PROP_USE_UNDERLINE,

  /* activatable properties */
  PROP_ACTIVATABLE_RELATED_ACTION,
  PROP_ACTIVATABLE_USE_ACTION_APPEARANCE
};


static void btk_menu_item_dispose        (BObject          *object);
static void btk_menu_item_set_property   (BObject          *object,
					  guint             prop_id,
					  const BValue     *value,
					  BParamSpec       *pspec);
static void btk_menu_item_get_property   (BObject          *object,
					  guint             prop_id,
					  BValue           *value,
					  BParamSpec       *pspec);
static void btk_menu_item_destroy        (BtkObject        *object);
static void btk_menu_item_size_request   (BtkWidget        *widget,
					  BtkRequisition   *requisition);
static void btk_menu_item_size_allocate  (BtkWidget        *widget,
					  BtkAllocation    *allocation);
static void btk_menu_item_realize        (BtkWidget        *widget);
static void btk_menu_item_unrealize      (BtkWidget        *widget);
static void btk_menu_item_map            (BtkWidget        *widget);
static void btk_menu_item_unmap          (BtkWidget        *widget);
static void btk_menu_item_paint          (BtkWidget        *widget,
					  BdkRectangle     *area);
static gint btk_menu_item_expose         (BtkWidget        *widget,
					  BdkEventExpose   *event);
static void btk_menu_item_parent_set     (BtkWidget        *widget,
					  BtkWidget        *previous_parent);


static void btk_real_menu_item_select               (BtkItem     *item);
static void btk_real_menu_item_deselect             (BtkItem     *item);
static void btk_real_menu_item_activate             (BtkMenuItem *item);
static void btk_real_menu_item_activate_item        (BtkMenuItem *item);
static void btk_real_menu_item_toggle_size_request  (BtkMenuItem *menu_item,
						     gint        *requisition);
static void btk_real_menu_item_toggle_size_allocate (BtkMenuItem *menu_item,
						     gint         allocation);
static gboolean btk_menu_item_mnemonic_activate     (BtkWidget   *widget,
						     gboolean     group_cycling);

static void btk_menu_item_ensure_label   (BtkMenuItem      *menu_item);
static gint btk_menu_item_popup_timeout  (gpointer          data);
static void btk_menu_item_position_menu  (BtkMenu          *menu,
					  gint             *x,
					  gint             *y,
					  gboolean         *push_in,
					  gpointer          user_data);
static void btk_menu_item_show_all       (BtkWidget        *widget);
static void btk_menu_item_hide_all       (BtkWidget        *widget);
static void btk_menu_item_forall         (BtkContainer    *container,
					  gboolean         include_internals,
					  BtkCallback      callback,
					  gpointer         callback_data);
static gboolean btk_menu_item_can_activate_accel (BtkWidget *widget,
						  guint      signal_id);

static void btk_real_menu_item_set_label (BtkMenuItem     *menu_item,
					  const gchar     *label);
static const gchar * btk_real_menu_item_get_label (BtkMenuItem *menu_item);


static void btk_menu_item_buildable_interface_init (BtkBuildableIface   *iface);
static void btk_menu_item_buildable_add_child      (BtkBuildable        *buildable,
						    BtkBuilder          *builder,
						    BObject             *child,
						    const gchar         *type);
static void btk_menu_item_buildable_custom_finished(BtkBuildable        *buildable,
						    BtkBuilder          *builder,
						    BObject             *child,
						    const gchar         *tagname,
						    gpointer             user_data);

static void btk_menu_item_activatable_interface_init (BtkActivatableIface  *iface);
static void btk_menu_item_update                     (BtkActivatable       *activatable,
						      BtkAction            *action,
						      const gchar          *property_name);
static void btk_menu_item_sync_action_properties     (BtkActivatable       *activatable,
						      BtkAction            *action);
static void btk_menu_item_set_related_action         (BtkMenuItem          *menu_item, 
						      BtkAction            *action);
static void btk_menu_item_set_use_action_appearance  (BtkMenuItem          *menu_item, 
						      gboolean              use_appearance);


static guint menu_item_signals[LAST_SIGNAL] = { 0 };

static BtkBuildableIface *parent_buildable_iface;

G_DEFINE_TYPE_WITH_CODE (BtkMenuItem, btk_menu_item, BTK_TYPE_ITEM,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
						btk_menu_item_buildable_interface_init)
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_ACTIVATABLE,
						btk_menu_item_activatable_interface_init))

#define GET_PRIVATE(object)  \
  (B_TYPE_INSTANCE_GET_PRIVATE ((object), BTK_TYPE_MENU_ITEM, BtkMenuItemPrivate))

static void
btk_menu_item_class_init (BtkMenuItemClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);
  BtkObjectClass *object_class = BTK_OBJECT_CLASS (klass);
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (klass);
  BtkContainerClass *container_class = BTK_CONTAINER_CLASS (klass);
  BtkItemClass *item_class = BTK_ITEM_CLASS (klass);

  bobject_class->dispose      = btk_menu_item_dispose;
  bobject_class->set_property = btk_menu_item_set_property;
  bobject_class->get_property = btk_menu_item_get_property;

  object_class->destroy = btk_menu_item_destroy;

  widget_class->size_request = btk_menu_item_size_request;
  widget_class->size_allocate = btk_menu_item_size_allocate;
  widget_class->expose_event = btk_menu_item_expose;
  widget_class->realize = btk_menu_item_realize;
  widget_class->unrealize = btk_menu_item_unrealize;
  widget_class->map = btk_menu_item_map;
  widget_class->unmap = btk_menu_item_unmap;
  widget_class->show_all = btk_menu_item_show_all;
  widget_class->hide_all = btk_menu_item_hide_all;
  widget_class->mnemonic_activate = btk_menu_item_mnemonic_activate;
  widget_class->parent_set = btk_menu_item_parent_set;
  widget_class->can_activate_accel = btk_menu_item_can_activate_accel;
  
  container_class->forall = btk_menu_item_forall;

  item_class->select      = btk_real_menu_item_select;
  item_class->deselect    = btk_real_menu_item_deselect;

  klass->activate             = btk_real_menu_item_activate;
  klass->activate_item        = btk_real_menu_item_activate_item;
  klass->toggle_size_request  = btk_real_menu_item_toggle_size_request;
  klass->toggle_size_allocate = btk_real_menu_item_toggle_size_allocate;
  klass->set_label            = btk_real_menu_item_set_label;
  klass->get_label            = btk_real_menu_item_get_label;

  klass->hide_on_activate = TRUE;

  menu_item_signals[ACTIVATE] =
    g_signal_new (I_("activate"),
		  B_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkMenuItemClass, activate),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
  widget_class->activate_signal = menu_item_signals[ACTIVATE];

  menu_item_signals[ACTIVATE_ITEM] =
    g_signal_new (I_("activate-item"),
		  B_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkMenuItemClass, activate_item),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);

  menu_item_signals[TOGGLE_SIZE_REQUEST] =
    g_signal_new (I_("toggle-size-request"),
		  B_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkMenuItemClass, toggle_size_request),
		  NULL, NULL,
		  _btk_marshal_VOID__POINTER,
		  B_TYPE_NONE, 1,
		  B_TYPE_POINTER);

  menu_item_signals[TOGGLE_SIZE_ALLOCATE] =
    g_signal_new (I_("toggle-size-allocate"),
		  B_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_FIRST,
 		  G_STRUCT_OFFSET (BtkMenuItemClass, toggle_size_allocate),
		  NULL, NULL,
		  _btk_marshal_VOID__INT,
		  B_TYPE_NONE, 1,
		  B_TYPE_INT);

  /**
   * BtkMenuItem:right-justified:
   *
   * Sets whether the menu item appears justified at the right side of a menu bar.
   *
   * Since: 2.14
   **/
  g_object_class_install_property (bobject_class,
                                   PROP_RIGHT_JUSTIFIED,
                                   g_param_spec_boolean ("right-justified",
                                                         P_("Right Justified"),
                                                         P_("Sets whether the menu item appears justified at the right side of a menu bar"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));

  /**
   * BtkMenuItem:submenu:
   *
   * The submenu attached to the menu item, or NULL if it has none.
   *
   * Since: 2.12
   **/
  g_object_class_install_property (bobject_class,
                                   PROP_SUBMENU,
                                   g_param_spec_object ("submenu",
                                                        P_("Submenu"),
                                                        P_("The submenu attached to the menu item, or NULL if it has none"),
                                                        BTK_TYPE_MENU,
                                                        BTK_PARAM_READWRITE));
  

  /**
   * BtkMenuItem:accel-path:
   *
   * Sets the accelerator path of the menu item, through which runtime
   * changes of the menu item's accelerator caused by the user can be
   * identified and saved to persistant storage.
   *
   * Since: 2.14
   **/
  g_object_class_install_property (bobject_class,
                                   PROP_ACCEL_PATH,
                                   g_param_spec_string ("accel-path",
                                                        P_("Accel Path"),
                                                        P_("Sets the accelerator path of the menu item"),
                                                        NULL,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkMenuItem:label:
   *
   * The text for the child label.
   *
   * Since: 2.16
   **/
  g_object_class_install_property (bobject_class,
                                   PROP_LABEL,
                                   g_param_spec_string ("label",
							P_("Label"),
							P_("The text for the child label"),
							"",
							BTK_PARAM_READWRITE));

  /**
   * BtkMenuItem:use-underline:
   *
   * %TRUE if underlines in the text indicate mnemonics  
   *
   * Since: 2.16
   **/
  g_object_class_install_property (bobject_class,
                                   PROP_USE_UNDERLINE,
                                   g_param_spec_boolean ("use-underline",
							 P_("Use underline"),
							 P_("If set, an underline in the text indicates "
							    "the next character should be used for the "
							    "mnemonic accelerator key"),
							 FALSE,
							 BTK_PARAM_READWRITE));

  g_object_class_override_property (bobject_class, PROP_ACTIVATABLE_RELATED_ACTION, "related-action");
  g_object_class_override_property (bobject_class, PROP_ACTIVATABLE_USE_ACTION_APPEARANCE, "use-action-appearance");

  btk_widget_class_install_style_property_parser (widget_class,
						  g_param_spec_enum ("selected-shadow-type",
								     "Selected Shadow Type",
								     "Shadow type when item is selected",
								     BTK_TYPE_SHADOW_TYPE,
								     BTK_SHADOW_NONE,
								     BTK_PARAM_READABLE),
						  btk_rc_property_parse_enum);

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("horizontal-padding",
							     "Horizontal Padding",
							     "Padding to left and right of the menu item",
							     0,
							     G_MAXINT,
							     3,
							     BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("toggle-spacing",
							     "Icon Spacing",
							     "Space between icon and label",
							     0,
							     G_MAXINT,
							     5,
							     BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("arrow-spacing",
							     "Arrow Spacing",
							     "Space between label and arrow",
							     0,
							     G_MAXINT,
							     10,
							     BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_float ("arrow-scaling",
                                                               P_("Arrow Scaling"),
                                                               P_("Amount of space used up by arrow, relative to the menu item's font size"),
                                                               0.0, 2.0, 0.8,
                                                               BTK_PARAM_READABLE));

  /**
   * BtkMenuItem:width-chars:
   *
   * The minimum desired width of the menu item in characters.
   *
   * Since: 2.14
   **/
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("width-chars",
                                                             P_("Width in Characters"),
                                                             P_("The minimum desired width of the menu item in characters"),
                                                             0, G_MAXINT, 12,
                                                             BTK_PARAM_READABLE));

  g_type_class_add_private (object_class, sizeof (BtkMenuItemPrivate));
}

static void
btk_menu_item_init (BtkMenuItem *menu_item)
{
  BtkMenuItemPrivate *priv = GET_PRIVATE (menu_item);

  btk_widget_set_has_window (BTK_WIDGET (menu_item), FALSE);

  priv->action = NULL;
  priv->use_action_appearance = TRUE;
  
  menu_item->submenu = NULL;
  menu_item->toggle_size = 0;
  menu_item->accelerator_width = 0;
  menu_item->show_submenu_indicator = FALSE;
  if (btk_widget_get_direction (BTK_WIDGET (menu_item)) == BTK_TEXT_DIR_RTL)
    menu_item->submenu_direction = BTK_DIRECTION_LEFT;
  else
    menu_item->submenu_direction = BTK_DIRECTION_RIGHT;
  menu_item->submenu_placement = BTK_TOP_BOTTOM;
  menu_item->right_justify = FALSE;

  menu_item->timer = 0;
}

BtkWidget*
btk_menu_item_new (void)
{
  return g_object_new (BTK_TYPE_MENU_ITEM, NULL);
}

BtkWidget*
btk_menu_item_new_with_label (const gchar *label)
{
  return g_object_new (BTK_TYPE_MENU_ITEM, 
		       "label", label,
		       NULL);
}


/**
 * btk_menu_item_new_with_mnemonic:
 * @label: The text of the button, with an underscore in front of the
 *         mnemonic character
 * @returns: a new #BtkMenuItem
 *
 * Creates a new #BtkMenuItem containing a label. The label
 * will be created using btk_label_new_with_mnemonic(), so underscores
 * in @label indicate the mnemonic for the menu item.
 **/
BtkWidget*
btk_menu_item_new_with_mnemonic (const gchar *label)
{
  return g_object_new (BTK_TYPE_MENU_ITEM, 
		       "use-underline", TRUE,
		       "label", label,
		       NULL);
}

static void
btk_menu_item_dispose (BObject *object)
{
  BtkMenuItem *menu_item = BTK_MENU_ITEM (object);
  BtkMenuItemPrivate *priv = GET_PRIVATE (menu_item);

  if (priv->action)
    {
      btk_action_disconnect_accelerator (priv->action);
      btk_activatable_do_set_related_action (BTK_ACTIVATABLE (menu_item), NULL);
      
      priv->action = NULL;
    }
  B_OBJECT_CLASS (btk_menu_item_parent_class)->dispose (object);
}

static void 
btk_menu_item_set_property (BObject      *object,
			    guint         prop_id,
			    const BValue *value,
			    BParamSpec   *pspec)
{
  BtkMenuItem *menu_item = BTK_MENU_ITEM (object);
  
  switch (prop_id)
    {
    case PROP_RIGHT_JUSTIFIED:
      btk_menu_item_set_right_justified (menu_item, b_value_get_boolean (value));
      break;
    case PROP_SUBMENU:
      btk_menu_item_set_submenu (menu_item, b_value_get_object (value));
      break;
    case PROP_ACCEL_PATH:
      btk_menu_item_set_accel_path (menu_item, b_value_get_string (value));
      break;
    case PROP_LABEL:
      btk_menu_item_set_label (menu_item, b_value_get_string (value));
      break;
    case PROP_USE_UNDERLINE:
      btk_menu_item_set_use_underline (menu_item, b_value_get_boolean (value));
      break;
    case PROP_ACTIVATABLE_RELATED_ACTION:
      btk_menu_item_set_related_action (menu_item, b_value_get_object (value));
      break;
    case PROP_ACTIVATABLE_USE_ACTION_APPEARANCE:
      btk_menu_item_set_use_action_appearance (menu_item, b_value_get_boolean (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
btk_menu_item_get_property (BObject    *object,
			    guint       prop_id,
			    BValue     *value,
			    BParamSpec *pspec)
{
  BtkMenuItem *menu_item = BTK_MENU_ITEM (object);
  BtkMenuItemPrivate *priv = GET_PRIVATE (menu_item);
  
  switch (prop_id)
    {
    case PROP_RIGHT_JUSTIFIED:
      b_value_set_boolean (value, btk_menu_item_get_right_justified (menu_item));
      break;
    case PROP_SUBMENU:
      b_value_set_object (value, btk_menu_item_get_submenu (menu_item));
      break;
    case PROP_ACCEL_PATH:
      b_value_set_string (value, btk_menu_item_get_accel_path (menu_item));
      break;
    case PROP_LABEL:
      b_value_set_string (value, btk_menu_item_get_label (menu_item));
      break;
    case PROP_USE_UNDERLINE:
      b_value_set_boolean (value, btk_menu_item_get_use_underline (menu_item));
      break;
    case PROP_ACTIVATABLE_RELATED_ACTION:
      b_value_set_object (value, priv->action);
      break;
    case PROP_ACTIVATABLE_USE_ACTION_APPEARANCE:
      b_value_set_boolean (value, priv->use_action_appearance);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_menu_item_destroy (BtkObject *object)
{
  BtkMenuItem *menu_item = BTK_MENU_ITEM (object);

  if (menu_item->submenu)
    btk_widget_destroy (menu_item->submenu);

  BTK_OBJECT_CLASS (btk_menu_item_parent_class)->destroy (object);
}

static void
btk_menu_item_detacher (BtkWidget *widget,
			BtkMenu   *menu)
{
  BtkMenuItem *menu_item = BTK_MENU_ITEM (widget);

  g_return_if_fail (menu_item->submenu == (BtkWidget*) menu);

  menu_item->submenu = NULL;
}

static void
btk_menu_item_buildable_interface_init (BtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = btk_menu_item_buildable_add_child;
  iface->custom_finished = btk_menu_item_buildable_custom_finished;
}

static void 
btk_menu_item_buildable_add_child (BtkBuildable *buildable,
				   BtkBuilder   *builder,
				   BObject      *child,
				   const gchar  *type)
{
  if (type && strcmp (type, "submenu") == 0)
	btk_menu_item_set_submenu (BTK_MENU_ITEM (buildable),
				   BTK_WIDGET (child));
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}


static void 
btk_menu_item_buildable_custom_finished (BtkBuildable        *buildable,
					 BtkBuilder          *builder,
					 BObject             *child,
					 const gchar         *tagname,
					 gpointer             user_data)
{
  BtkWidget *toplevel;

  if (strcmp (tagname, "accelerator") == 0)
    {
      BtkMenuShell *menu_shell = (BtkMenuShell *) BTK_WIDGET (buildable)->parent;
      BtkWidget *attach;

      if (menu_shell)
	{
	  while (BTK_IS_MENU (menu_shell) &&
		 (attach = btk_menu_get_attach_widget (BTK_MENU (menu_shell))) != NULL)
	    menu_shell = (BtkMenuShell *)attach->parent;
	  
	  toplevel = btk_widget_get_toplevel (BTK_WIDGET (menu_shell));
	}
      else
	{
	  /* Fall back to something ... */
	  toplevel = btk_widget_get_toplevel (BTK_WIDGET (buildable));

	  g_warning ("found a BtkMenuItem '%s' without a parent BtkMenuShell, assigned accelerators wont work.",
		     btk_buildable_get_name (buildable));
	}

      /* Feed the correct toplevel to the BtkWidget accelerator parsing code */
      _btk_widget_buildable_finish_accelerator (BTK_WIDGET (buildable), toplevel, user_data);
    }
  else
    parent_buildable_iface->custom_finished (buildable, builder, child, tagname, user_data);
}


static void
btk_menu_item_activatable_interface_init (BtkActivatableIface *iface)
{
  iface->update = btk_menu_item_update;
  iface->sync_action_properties = btk_menu_item_sync_action_properties;
}

static void
activatable_update_label (BtkMenuItem *menu_item, BtkAction *action)
{
  BtkWidget *child = BTK_BIN (menu_item)->child;

  if (BTK_IS_LABEL (child))
    {
      const gchar *label;

      label = btk_action_get_label (action);
      btk_menu_item_set_label (menu_item, label);
    }
}

gboolean _btk_menu_is_empty (BtkWidget *menu);

static void
btk_menu_item_update (BtkActivatable *activatable,
		      BtkAction      *action,
		      const gchar    *property_name)
{
  BtkMenuItem *menu_item = BTK_MENU_ITEM (activatable);
  BtkMenuItemPrivate *priv = GET_PRIVATE (menu_item);

  if (strcmp (property_name, "visible") == 0)
    _btk_action_sync_menu_visible (action, BTK_WIDGET (menu_item), 
				   _btk_menu_is_empty (btk_menu_item_get_submenu (menu_item)));
  else if (strcmp (property_name, "sensitive") == 0)
    btk_widget_set_sensitive (BTK_WIDGET (menu_item), btk_action_is_sensitive (action));
  else if (priv->use_action_appearance)
    {
      if (strcmp (property_name, "label") == 0)
	activatable_update_label (menu_item, action);
    }
}

static void
btk_menu_item_sync_action_properties (BtkActivatable *activatable,
				      BtkAction      *action)
{
  BtkMenuItem *menu_item = BTK_MENU_ITEM (activatable);
  BtkMenuItemPrivate *priv = GET_PRIVATE (menu_item);

  if (!priv->use_action_appearance || !action)
    {
      BtkWidget *label = BTK_BIN (menu_item)->child;

      label = BTK_BIN (menu_item)->child;

      if (BTK_IS_ACCEL_LABEL (label))
        btk_accel_label_set_accel_widget (BTK_ACCEL_LABEL (label), BTK_WIDGET (menu_item));
    }

  if (!action)
    return;

  _btk_action_sync_menu_visible (action, BTK_WIDGET (menu_item),
				 _btk_menu_is_empty (btk_menu_item_get_submenu (menu_item)));

  btk_widget_set_sensitive (BTK_WIDGET (menu_item), btk_action_is_sensitive (action));

  if (priv->use_action_appearance)
    {
      BtkWidget *label = BTK_BIN (menu_item)->child;

      /* make sure label is a label */
      if (label && !BTK_IS_LABEL (label))
	{
	  btk_container_remove (BTK_CONTAINER (menu_item), label);
	  label = NULL;
	}

      btk_menu_item_ensure_label (menu_item);
      btk_menu_item_set_use_underline (menu_item, TRUE);

      label = BTK_BIN (menu_item)->child;

      if (BTK_IS_ACCEL_LABEL (label) && btk_action_get_accel_path (action))
        {
          btk_accel_label_set_accel_widget (BTK_ACCEL_LABEL (label), NULL);
          btk_accel_label_set_accel_closure (BTK_ACCEL_LABEL (label),
                                             btk_action_get_accel_closure (action));
        }

      activatable_update_label (menu_item, action);
    }
}

static void
btk_menu_item_set_related_action (BtkMenuItem *menu_item,
				  BtkAction   *action)
{
    BtkMenuItemPrivate *priv = GET_PRIVATE (menu_item);

    if (priv->action == action)
      return;

    if (priv->action)
      {
	btk_action_disconnect_accelerator (priv->action);
      }

    if (action)
      {
	const gchar *accel_path;
	
	accel_path = btk_action_get_accel_path (action);
	if (accel_path)
	  {
	    btk_action_connect_accelerator (action);
	    btk_menu_item_set_accel_path (menu_item, accel_path);
	  }
      }

    btk_activatable_do_set_related_action (BTK_ACTIVATABLE (menu_item), action);

    priv->action = action;
}

static void
btk_menu_item_set_use_action_appearance (BtkMenuItem *menu_item,
					 gboolean     use_appearance)
{
    BtkMenuItemPrivate *priv = GET_PRIVATE (menu_item);

    if (priv->use_action_appearance != use_appearance)
      {
	priv->use_action_appearance = use_appearance;

	btk_activatable_sync_action_properties (BTK_ACTIVATABLE (menu_item), priv->action);
      }
}


/**
 * btk_menu_item_set_submenu:
 * @menu_item: a #BtkMenuItem
 * @submenu: (allow-none): the submenu, or %NULL
 *
 * Sets or replaces the menu item's submenu, or removes it when a %NULL
 * submenu is passed.
 **/
void
btk_menu_item_set_submenu (BtkMenuItem *menu_item,
			   BtkWidget   *submenu)
{
  g_return_if_fail (BTK_IS_MENU_ITEM (menu_item));
  g_return_if_fail (submenu == NULL || BTK_IS_MENU (submenu));
  
  if (menu_item->submenu != submenu)
    {
      if (menu_item->submenu)
	btk_menu_detach (BTK_MENU (menu_item->submenu));

      if (submenu)
	{
	  menu_item->submenu = submenu;
	  btk_menu_attach_to_widget (BTK_MENU (submenu),
				     BTK_WIDGET (menu_item),
				     btk_menu_item_detacher);
	}

      if (BTK_WIDGET (menu_item)->parent)
	btk_widget_queue_resize (BTK_WIDGET (menu_item));

      g_object_notify (B_OBJECT (menu_item), "submenu");
    }
}

/**
 * btk_menu_item_get_submenu:
 * @menu_item: a #BtkMenuItem
 *
 * Gets the submenu underneath this menu item, if any.
 * See btk_menu_item_set_submenu().
 *
 * Return value: (transfer none): submenu for this menu item, or %NULL if none.
 **/
BtkWidget *
btk_menu_item_get_submenu (BtkMenuItem *menu_item)
{
  g_return_val_if_fail (BTK_IS_MENU_ITEM (menu_item), NULL);

  return menu_item->submenu;
}

/**
 * btk_menu_item_remove_submenu:
 * @menu_item: a #BtkMenuItem
 *
 * Removes the widget's submenu.
 *
 * Deprecated: 2.12: btk_menu_item_remove_submenu() is deprecated and
 *                   should not be used in newly written code. Use
 *                   btk_menu_item_set_submenu() instead.
 **/
void
btk_menu_item_remove_submenu (BtkMenuItem *menu_item)
{
  g_return_if_fail (BTK_IS_MENU_ITEM (menu_item));

  btk_menu_item_set_submenu (menu_item, NULL);
}

void _btk_menu_item_set_placement (BtkMenuItem         *menu_item,
				   BtkSubmenuPlacement  placement);

void
_btk_menu_item_set_placement (BtkMenuItem         *menu_item,
			     BtkSubmenuPlacement  placement)
{
  g_return_if_fail (BTK_IS_MENU_ITEM (menu_item));

  menu_item->submenu_placement = placement;
}

void
btk_menu_item_select (BtkMenuItem *menu_item)
{
  g_return_if_fail (BTK_IS_MENU_ITEM (menu_item));

  btk_item_select (BTK_ITEM (menu_item));

  /* Enable themeing of the parent menu item depending on whether
   * something is selected in its submenu
   */
  if (BTK_IS_MENU (BTK_WIDGET (menu_item)->parent))
    {
      BtkMenu *menu = BTK_MENU (BTK_WIDGET (menu_item)->parent);

      if (menu->parent_menu_item)
        btk_widget_queue_draw (BTK_WIDGET (menu->parent_menu_item));
    }
}

void
btk_menu_item_deselect (BtkMenuItem *menu_item)
{
  g_return_if_fail (BTK_IS_MENU_ITEM (menu_item));

  btk_item_deselect (BTK_ITEM (menu_item));

  /* Enable themeing of the parent menu item depending on whether
   * something is selected in its submenu
   */
  if (BTK_IS_MENU (BTK_WIDGET (menu_item)->parent))
    {
      BtkMenu *menu = BTK_MENU (BTK_WIDGET (menu_item)->parent);

      if (menu->parent_menu_item)
        btk_widget_queue_draw (BTK_WIDGET (menu->parent_menu_item));
    }
}

void
btk_menu_item_activate (BtkMenuItem *menu_item)
{
  g_return_if_fail (BTK_IS_MENU_ITEM (menu_item));

  g_signal_emit (menu_item, menu_item_signals[ACTIVATE], 0);
}

void
btk_menu_item_toggle_size_request (BtkMenuItem *menu_item,
				   gint        *requisition)
{
  g_return_if_fail (BTK_IS_MENU_ITEM (menu_item));

  g_signal_emit (menu_item, menu_item_signals[TOGGLE_SIZE_REQUEST], 0, requisition);
}

void
btk_menu_item_toggle_size_allocate (BtkMenuItem *menu_item,
				    gint         allocation)
{
  g_return_if_fail (BTK_IS_MENU_ITEM (menu_item));

  g_signal_emit (menu_item, menu_item_signals[TOGGLE_SIZE_ALLOCATE], 0, allocation);
}

static void
btk_menu_item_accel_width_foreach (BtkWidget *widget,
				   gpointer data)
{
  guint *width = data;

  if (BTK_IS_ACCEL_LABEL (widget))
    {
      guint w;

      w = btk_accel_label_get_accel_width (BTK_ACCEL_LABEL (widget));
      *width = MAX (*width, w);
    }
  else if (BTK_IS_CONTAINER (widget))
    btk_container_foreach (BTK_CONTAINER (widget),
			   btk_menu_item_accel_width_foreach,
			   data);
}

static gint
get_minimum_width (BtkWidget *widget)
{
  BangoContext *context;
  BangoFontMetrics *metrics;
  gint width;
  gint width_chars;

  context = btk_widget_get_bango_context (widget);
  metrics = bango_context_get_metrics (context,
				       widget->style->font_desc,
				       bango_context_get_language (context));

  width = bango_font_metrics_get_approximate_char_width (metrics);

  bango_font_metrics_unref (metrics);

  btk_widget_style_get (widget, "width-chars", &width_chars, NULL);

  return BANGO_PIXELS (width_chars * width);
}

static void
btk_menu_item_size_request (BtkWidget      *widget,
			    BtkRequisition *requisition)
{
  BtkMenuItem *menu_item;
  BtkBin *bin;
  guint accel_width;
  guint horizontal_padding;
  BtkPackDirection pack_dir;
  BtkPackDirection child_pack_dir;

  g_return_if_fail (BTK_IS_MENU_ITEM (widget));
  g_return_if_fail (requisition != NULL);

  btk_widget_style_get (widget,
 			"horizontal-padding", &horizontal_padding,
			NULL);
  
  bin = BTK_BIN (widget);
  menu_item = BTK_MENU_ITEM (widget);

  if (BTK_IS_MENU_BAR (widget->parent))
    {
      pack_dir = btk_menu_bar_get_pack_direction (BTK_MENU_BAR (widget->parent));
      child_pack_dir = btk_menu_bar_get_child_pack_direction (BTK_MENU_BAR (widget->parent));
    }
  else
    {
      pack_dir = BTK_PACK_DIRECTION_LTR;
      child_pack_dir = BTK_PACK_DIRECTION_LTR;
    }

  requisition->width = (BTK_CONTAINER (widget)->border_width +
			widget->style->xthickness) * 2;
  requisition->height = (BTK_CONTAINER (widget)->border_width +
			 widget->style->ythickness) * 2;

  if ((pack_dir == BTK_PACK_DIRECTION_LTR || pack_dir == BTK_PACK_DIRECTION_RTL) &&
      (child_pack_dir == BTK_PACK_DIRECTION_LTR || child_pack_dir == BTK_PACK_DIRECTION_RTL))
    requisition->width += 2 * horizontal_padding;
  else if ((pack_dir == BTK_PACK_DIRECTION_TTB || pack_dir == BTK_PACK_DIRECTION_BTT) &&
      (child_pack_dir == BTK_PACK_DIRECTION_TTB || child_pack_dir == BTK_PACK_DIRECTION_BTT))
    requisition->height += 2 * horizontal_padding;

  if (bin->child && btk_widget_get_visible (bin->child))
    {
      BtkRequisition child_requisition;
      
      btk_widget_size_request (bin->child, &child_requisition);

      requisition->width += child_requisition.width;
      requisition->height += child_requisition.height;

      if (menu_item->submenu && menu_item->show_submenu_indicator)
	{
	  guint arrow_spacing;
	  
	  btk_widget_style_get (widget,
				"arrow-spacing", &arrow_spacing,
				NULL);

	  requisition->width += child_requisition.height;
	  requisition->width += arrow_spacing;

	  requisition->width = MAX (requisition->width, get_minimum_width (widget));
	}
    }
  else /* separator item */
    {
      gboolean wide_separators;
      gint     separator_height;

      btk_widget_style_get (widget,
                            "wide-separators",  &wide_separators,
                            "separator-height", &separator_height,
                            NULL);

      if (wide_separators)
        requisition->height += separator_height + widget->style->ythickness;
      else
        requisition->height += widget->style->ythickness * 2;
    }

  accel_width = 0;
  btk_container_foreach (BTK_CONTAINER (menu_item),
			 btk_menu_item_accel_width_foreach,
			 &accel_width);
  menu_item->accelerator_width = accel_width;
}

static void
btk_menu_item_size_allocate (BtkWidget     *widget,
			     BtkAllocation *allocation)
{
  BtkMenuItem *menu_item;
  BtkBin *bin;
  BtkAllocation child_allocation;
  BtkTextDirection direction;
  BtkPackDirection pack_dir;
  BtkPackDirection child_pack_dir;

  g_return_if_fail (BTK_IS_MENU_ITEM (widget));
  g_return_if_fail (allocation != NULL);

  menu_item = BTK_MENU_ITEM (widget);
  bin = BTK_BIN (widget);
  
  direction = btk_widget_get_direction (widget);

  if (BTK_IS_MENU_BAR (widget->parent))
    {
      pack_dir = btk_menu_bar_get_pack_direction (BTK_MENU_BAR (widget->parent));
      child_pack_dir = btk_menu_bar_get_child_pack_direction (BTK_MENU_BAR (widget->parent));
    }
  else
    {
      pack_dir = BTK_PACK_DIRECTION_LTR;
      child_pack_dir = BTK_PACK_DIRECTION_LTR;
    }
    
  widget->allocation = *allocation;

  if (bin->child)
    {
      BtkRequisition child_requisition;
      guint horizontal_padding;

      btk_widget_style_get (widget,
			    "horizontal-padding", &horizontal_padding,
			    NULL);

      child_allocation.x = BTK_CONTAINER (widget)->border_width + widget->style->xthickness;
      child_allocation.y = BTK_CONTAINER (widget)->border_width + widget->style->ythickness;

      if ((pack_dir == BTK_PACK_DIRECTION_LTR || pack_dir == BTK_PACK_DIRECTION_RTL) &&
	  (child_pack_dir == BTK_PACK_DIRECTION_LTR || child_pack_dir == BTK_PACK_DIRECTION_RTL))
	child_allocation.x += horizontal_padding;
      else if ((pack_dir == BTK_PACK_DIRECTION_TTB || pack_dir == BTK_PACK_DIRECTION_BTT) &&
	       (child_pack_dir == BTK_PACK_DIRECTION_TTB || child_pack_dir == BTK_PACK_DIRECTION_BTT))
	child_allocation.y += horizontal_padding;
      
      child_allocation.width = MAX (1, (gint)allocation->width - child_allocation.x * 2);
      child_allocation.height = MAX (1, (gint)allocation->height - child_allocation.y * 2);

      if (child_pack_dir == BTK_PACK_DIRECTION_LTR ||
	  child_pack_dir == BTK_PACK_DIRECTION_RTL)
	{
	  if ((direction == BTK_TEXT_DIR_LTR) == (child_pack_dir != BTK_PACK_DIRECTION_RTL))
	    child_allocation.x += BTK_MENU_ITEM (widget)->toggle_size;
	  child_allocation.width -= BTK_MENU_ITEM (widget)->toggle_size;
	}
      else
	{
	  if ((direction == BTK_TEXT_DIR_LTR) == (child_pack_dir != BTK_PACK_DIRECTION_BTT))
	    child_allocation.y += BTK_MENU_ITEM (widget)->toggle_size;
	  child_allocation.height -= BTK_MENU_ITEM (widget)->toggle_size;
	}

      child_allocation.x += widget->allocation.x;
      child_allocation.y += widget->allocation.y;

      btk_widget_get_child_requisition (bin->child, &child_requisition);
      if (menu_item->submenu && menu_item->show_submenu_indicator) 
	{
	  if (direction == BTK_TEXT_DIR_RTL)
	    child_allocation.x += child_requisition.height;
	  child_allocation.width -= child_requisition.height;
	}
      
      if (child_allocation.width < 1)
	child_allocation.width = 1;

      btk_widget_size_allocate (bin->child, &child_allocation);
    }

  if (btk_widget_get_realized (widget))
    bdk_window_move_resize (menu_item->event_window,
                            allocation->x, allocation->y,
                            allocation->width, allocation->height);

  if (menu_item->submenu)
    btk_menu_reposition (BTK_MENU (menu_item->submenu));
}

static void
btk_menu_item_realize (BtkWidget *widget)
{
  BtkMenuItem *menu_item = BTK_MENU_ITEM (widget);
  BdkWindowAttr attributes;
  gint attributes_mask;

  btk_widget_set_realized (widget, TRUE);

  widget->window = btk_widget_get_parent_window (widget);
  g_object_ref (widget->window);
  
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.wclass = BDK_INPUT_ONLY;
  attributes.event_mask = (btk_widget_get_events (widget) |
			   BDK_BUTTON_PRESS_MASK |
			   BDK_BUTTON_RELEASE_MASK |
			   BDK_ENTER_NOTIFY_MASK |
			   BDK_LEAVE_NOTIFY_MASK |
			   BDK_POINTER_MOTION_MASK);

  attributes_mask = BDK_WA_X | BDK_WA_Y;
  menu_item->event_window = bdk_window_new (btk_widget_get_parent_window (widget), &attributes, attributes_mask);
  bdk_window_set_user_data (menu_item->event_window, widget);

  widget->style = btk_style_attach (widget->style, widget->window);
}

static void
btk_menu_item_unrealize (BtkWidget *widget)
{
  BtkMenuItem *menu_item = BTK_MENU_ITEM (widget);

  bdk_window_set_user_data (menu_item->event_window, NULL);
  bdk_window_destroy (menu_item->event_window);
  menu_item->event_window = NULL;

  BTK_WIDGET_CLASS (btk_menu_item_parent_class)->unrealize (widget);
}

static void
btk_menu_item_map (BtkWidget *widget)
{
  BtkMenuItem *menu_item = BTK_MENU_ITEM (widget);
  
  BTK_WIDGET_CLASS (btk_menu_item_parent_class)->map (widget);

  bdk_window_show (menu_item->event_window);
}

static void
btk_menu_item_unmap (BtkWidget *widget)
{
  BtkMenuItem *menu_item = BTK_MENU_ITEM (widget);
    
  bdk_window_hide (menu_item->event_window);

  BTK_WIDGET_CLASS (btk_menu_item_parent_class)->unmap (widget);
}

static void
btk_menu_item_paint (BtkWidget    *widget,
		     BdkRectangle *area)
{
  BtkMenuItem *menu_item;
  BtkStateType state_type;
  BtkShadowType shadow_type, selected_shadow_type;
  gint width, height;
  gint x, y;
  gint border_width = BTK_CONTAINER (widget)->border_width;

  if (btk_widget_is_drawable (widget))
    {
      menu_item = BTK_MENU_ITEM (widget);

      state_type = widget->state;
      
      x = widget->allocation.x + border_width;
      y = widget->allocation.y + border_width;
      width = widget->allocation.width - border_width * 2;
      height = widget->allocation.height - border_width * 2;
      
      if ((state_type == BTK_STATE_PRELIGHT) &&
	  (BTK_BIN (menu_item)->child))
	{
	  btk_widget_style_get (widget,
				"selected-shadow-type", &selected_shadow_type,
				NULL);
	  btk_paint_box (widget->style,
			 widget->window,
			 BTK_STATE_PRELIGHT,
			 selected_shadow_type,
			 area, widget, "menuitem",
			 x, y, width, height);
	}
  
      if (menu_item->submenu && menu_item->show_submenu_indicator)
	{
	  gint arrow_x, arrow_y;
	  gint arrow_size;
	  gint arrow_extent;
	  guint horizontal_padding;
          gfloat arrow_scaling;
	  BtkTextDirection direction;
	  BtkArrowType arrow_type;
	  BangoContext *context;
	  BangoFontMetrics *metrics;

	  direction = btk_widget_get_direction (widget);
      
 	  btk_widget_style_get (widget,
 				"horizontal-padding", &horizontal_padding,
                                "arrow-scaling", &arrow_scaling,
 				NULL);
 	  
	  context = btk_widget_get_bango_context (BTK_BIN (menu_item)->child);
	  metrics = bango_context_get_metrics (context, 
					       BTK_WIDGET (BTK_BIN (menu_item)->child)->style->font_desc,
					       bango_context_get_language (context));

	  arrow_size = (BANGO_PIXELS (bango_font_metrics_get_ascent (metrics) +
                                      bango_font_metrics_get_descent (metrics)));

	  bango_font_metrics_unref (metrics);

	  arrow_extent = arrow_size * arrow_scaling;

	  shadow_type = BTK_SHADOW_OUT;
	  if (state_type == BTK_STATE_PRELIGHT)
	    shadow_type = BTK_SHADOW_IN;

	  if (direction == BTK_TEXT_DIR_LTR)
	    {
	      arrow_x = x + width - horizontal_padding - arrow_extent;
	      arrow_type = BTK_ARROW_RIGHT;
	    }
	  else
	    {
	      arrow_x = x + horizontal_padding;
	      arrow_type = BTK_ARROW_LEFT;
	    }

	  arrow_y = y + (height - arrow_extent) / 2;

	  btk_paint_arrow (widget->style, widget->window,
			   state_type, shadow_type, 
			   area, widget, "menuitem", 
			   arrow_type, TRUE,
			   arrow_x, arrow_y,
			   arrow_extent, arrow_extent);
	}
      else if (!BTK_BIN (menu_item)->child)
	{
          gboolean wide_separators;
          gint     separator_height;
	  guint    horizontal_padding;

	  btk_widget_style_get (widget,
                                "wide-separators",    &wide_separators,
                                "separator-height",   &separator_height,
                                "horizontal-padding", &horizontal_padding,
                                NULL);

          if (wide_separators)
            btk_paint_box (widget->style, widget->window,
                           BTK_STATE_NORMAL, BTK_SHADOW_ETCHED_OUT,
                           area, widget, "hseparator",
                           widget->allocation.x + horizontal_padding + widget->style->xthickness,
                           widget->allocation.y + (widget->allocation.height -
                                                   separator_height -
                                                   widget->style->ythickness) / 2,
                           widget->allocation.width -
                           2 * (horizontal_padding + widget->style->xthickness),
                           separator_height);
          else
            btk_paint_hline (widget->style, widget->window,
                             BTK_STATE_NORMAL, area, widget, "menuitem",
                             widget->allocation.x + horizontal_padding + widget->style->xthickness,
                             widget->allocation.x + widget->allocation.width - horizontal_padding - widget->style->xthickness - 1,
                             widget->allocation.y + (widget->allocation.height -
                                                     widget->style->ythickness) / 2);
	}
    }
}

static gint
btk_menu_item_expose (BtkWidget      *widget,
		      BdkEventExpose *event)
{
  g_return_val_if_fail (BTK_IS_MENU_ITEM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (btk_widget_is_drawable (widget))
    {
      btk_menu_item_paint (widget, &event->area);

      BTK_WIDGET_CLASS (btk_menu_item_parent_class)->expose_event (widget, event);
    }

  return FALSE;
}

static void
btk_real_menu_item_select (BtkItem *item)
{
  BtkMenuItem *menu_item;
  gboolean touchscreen_mode;

  g_return_if_fail (BTK_IS_MENU_ITEM (item));

  menu_item = BTK_MENU_ITEM (item);

  g_object_get (btk_widget_get_settings (BTK_WIDGET (item)),
                "btk-touchscreen-mode", &touchscreen_mode,
                NULL);

  if (!touchscreen_mode &&
      menu_item->submenu &&
      (!btk_widget_get_mapped (menu_item->submenu) ||
       BTK_MENU (menu_item->submenu)->tearoff_active))
    {
      _btk_menu_item_popup_submenu (BTK_WIDGET (menu_item), TRUE);
    }

  btk_widget_set_state (BTK_WIDGET (menu_item), BTK_STATE_PRELIGHT);
  btk_widget_queue_draw (BTK_WIDGET (menu_item));
}

static void
btk_real_menu_item_deselect (BtkItem *item)
{
  BtkMenuItem *menu_item;

  g_return_if_fail (BTK_IS_MENU_ITEM (item));

  menu_item = BTK_MENU_ITEM (item);

  if (menu_item->submenu)
    _btk_menu_item_popdown_submenu (BTK_WIDGET (menu_item));

  btk_widget_set_state (BTK_WIDGET (menu_item), BTK_STATE_NORMAL);
  btk_widget_queue_draw (BTK_WIDGET (menu_item));
}

static gboolean
btk_menu_item_mnemonic_activate (BtkWidget *widget,
				 gboolean   group_cycling)
{
  if (BTK_IS_MENU_SHELL (widget->parent))
    _btk_menu_shell_set_keyboard_mode (BTK_MENU_SHELL (widget->parent), TRUE);

  if (group_cycling &&
      widget->parent &&
      BTK_IS_MENU_SHELL (widget->parent) &&
      BTK_MENU_SHELL (widget->parent)->active)
    {
      btk_menu_shell_select_item (BTK_MENU_SHELL (widget->parent),
				  widget);
    }
  else
    g_signal_emit (widget, menu_item_signals[ACTIVATE_ITEM], 0);
  
  return TRUE;
}

static void 
btk_real_menu_item_activate (BtkMenuItem *menu_item)
{
  BtkMenuItemPrivate *priv;

  priv = GET_PRIVATE (menu_item);

  if (priv->action)
    btk_action_activate (priv->action);
}


static void
btk_real_menu_item_activate_item (BtkMenuItem *menu_item)
{
  BtkMenuItemPrivate *priv;
  BtkWidget *widget;

  g_return_if_fail (BTK_IS_MENU_ITEM (menu_item));

  priv   = GET_PRIVATE (menu_item);
  widget = BTK_WIDGET (menu_item);
  
  if (widget->parent &&
      BTK_IS_MENU_SHELL (widget->parent))
    {
      if (menu_item->submenu == NULL)
	btk_menu_shell_activate_item (BTK_MENU_SHELL (widget->parent),
				      widget, TRUE);
      else
	{
	  BtkMenuShell *menu_shell = BTK_MENU_SHELL (widget->parent);

	  btk_menu_shell_select_item (BTK_MENU_SHELL (widget->parent), widget);
	  _btk_menu_item_popup_submenu (widget, FALSE);

	  btk_menu_shell_select_first (BTK_MENU_SHELL (menu_item->submenu), TRUE);
	}
    }
}

static void
btk_real_menu_item_toggle_size_request (BtkMenuItem *menu_item,
					gint        *requisition)
{
  g_return_if_fail (BTK_IS_MENU_ITEM (menu_item));

  *requisition = 0;
}

static void
btk_real_menu_item_toggle_size_allocate (BtkMenuItem *menu_item,
					 gint         allocation)
{
  g_return_if_fail (BTK_IS_MENU_ITEM (menu_item));

  menu_item->toggle_size = allocation;
}

static void
btk_real_menu_item_set_label (BtkMenuItem *menu_item,
			      const gchar *label)
{
  btk_menu_item_ensure_label (menu_item);

  if (BTK_IS_LABEL (BTK_BIN (menu_item)->child))
    {
      btk_label_set_label (BTK_LABEL (BTK_BIN (menu_item)->child), label ? label : "");
      
      g_object_notify (B_OBJECT (menu_item), "label");
    }
}

static const gchar *
btk_real_menu_item_get_label (BtkMenuItem *menu_item)
{
  btk_menu_item_ensure_label (menu_item);

  if (BTK_IS_LABEL (BTK_BIN (menu_item)->child))
    return btk_label_get_label (BTK_LABEL (BTK_BIN (menu_item)->child));

  return NULL;
}

static void
free_timeval (GTimeVal *val)
{
  g_slice_free (GTimeVal, val);
}

static void
btk_menu_item_real_popup_submenu (BtkWidget *widget,
                                  gboolean   remember_exact_time)
{
  BtkMenuItem *menu_item = BTK_MENU_ITEM (widget);

  if (btk_widget_is_sensitive (menu_item->submenu) && widget->parent)
    {
      gboolean take_focus;
      BtkMenuPositionFunc menu_position_func;

      take_focus = btk_menu_shell_get_take_focus (BTK_MENU_SHELL (widget->parent));
      btk_menu_shell_set_take_focus (BTK_MENU_SHELL (menu_item->submenu),
                                     take_focus);

      if (remember_exact_time)
        {
          GTimeVal *popup_time = g_slice_new0 (GTimeVal);

          g_get_current_time (popup_time);

          g_object_set_data_full (B_OBJECT (menu_item->submenu),
                                  "btk-menu-exact-popup-time", popup_time,
                                  (GDestroyNotify) free_timeval);
        }
      else
        {
          g_object_set_data (B_OBJECT (menu_item->submenu),
                             "btk-menu-exact-popup-time", NULL);
        }

      /* btk_menu_item_position_menu positions the submenu from the
       * menuitems position. If the menuitem doesn't have a window,
       * that doesn't work. In that case we use the default
       * positioning function instead which places the submenu at the
       * mouse cursor.
       */
      if (widget->window)
        menu_position_func = btk_menu_item_position_menu;
      else
        menu_position_func = NULL;

      btk_menu_popup (BTK_MENU (menu_item->submenu),
                      widget->parent,
                      widget,
                      menu_position_func,
                      menu_item,
                      BTK_MENU_SHELL (widget->parent)->button,
                      0);
    }

  /* Enable themeing of the parent menu item depending on whether
   * its submenu is shown or not.
   */
  btk_widget_queue_draw (widget);
}

static gint
btk_menu_item_popup_timeout (gpointer data)
{
  BtkMenuItem *menu_item;
  BtkWidget *parent;
  
  menu_item = BTK_MENU_ITEM (data);

  parent = BTK_WIDGET (menu_item)->parent;

  if ((BTK_IS_MENU_SHELL (parent) && BTK_MENU_SHELL (parent)->active) || 
      (BTK_IS_MENU (parent) && BTK_MENU (parent)->torn_off))
    {
      btk_menu_item_real_popup_submenu (BTK_WIDGET (menu_item), TRUE);
      if (menu_item->timer_from_keypress && menu_item->submenu)
	BTK_MENU_SHELL (menu_item->submenu)->ignore_enter = TRUE;
    }

  menu_item->timer = 0;

  return FALSE;  
}

static gint
get_popup_delay (BtkWidget *widget)
{
  if (BTK_IS_MENU_SHELL (widget->parent))
    {
      return _btk_menu_shell_get_popup_delay (BTK_MENU_SHELL (widget->parent));
    }
  else
    {
      gint popup_delay;

      g_object_get (btk_widget_get_settings (widget),
		    "btk-menu-popup-delay", &popup_delay,
		    NULL);

      return popup_delay;
    }
}

void
_btk_menu_item_popup_submenu (BtkWidget *widget,
                              gboolean   with_delay)
{
  BtkMenuItem *menu_item = BTK_MENU_ITEM (widget);

  if (menu_item->timer)
    {
      g_source_remove (menu_item->timer);
      menu_item->timer = 0;
      with_delay = FALSE;
    }

  if (with_delay)
    {
      gint popup_delay = get_popup_delay (widget);

      if (popup_delay > 0)
	{
	  BdkEvent *event = btk_get_current_event ();

	  menu_item->timer = bdk_threads_add_timeout (popup_delay,
                                                      btk_menu_item_popup_timeout,
                                                      menu_item);

	  if (event &&
	      event->type != BDK_BUTTON_PRESS &&
	      event->type != BDK_ENTER_NOTIFY)
	    menu_item->timer_from_keypress = TRUE;
	  else
	    menu_item->timer_from_keypress = FALSE;

	  if (event)
	    bdk_event_free (event);

          return;
        }
    }

  btk_menu_item_real_popup_submenu (widget, FALSE);
}

void
_btk_menu_item_popdown_submenu (BtkWidget *widget)
{
  BtkMenuItem *menu_item;

  menu_item = BTK_MENU_ITEM (widget);

  if (menu_item->submenu)
    {
      g_object_set_data (B_OBJECT (menu_item->submenu),
                         "btk-menu-exact-popup-time", NULL);

      if (menu_item->timer)
        {
          g_source_remove (menu_item->timer);
          menu_item->timer = 0;
        }
      else
        btk_menu_popdown (BTK_MENU (menu_item->submenu));

      btk_widget_queue_draw (widget);
    }
}

static void
get_offsets (BtkMenu *menu,
	     gint    *horizontal_offset,
	     gint    *vertical_offset)
{
  gint vertical_padding;
  gint horizontal_padding;
  
  btk_widget_style_get (BTK_WIDGET (menu),
			"horizontal-offset", horizontal_offset,
			"vertical-offset", vertical_offset,
			"horizontal-padding", &horizontal_padding,
			"vertical-padding", &vertical_padding,
			NULL);

  *vertical_offset -= BTK_WIDGET (menu)->style->ythickness;
  *vertical_offset -= vertical_padding;
  *horizontal_offset += horizontal_padding;
}

static void
btk_menu_item_position_menu (BtkMenu  *menu,
			     gint     *x,
			     gint     *y,
			     gboolean *push_in,
			     gpointer  user_data)
{
  BtkMenuItem *menu_item;
  BtkWidget *widget;
  BtkMenuItem *parent_menu_item;
  BdkScreen *screen;
  gint twidth, theight;
  gint tx, ty;
  BtkTextDirection direction;
  BdkRectangle monitor;
  gint monitor_num;
  gint horizontal_offset;
  gint vertical_offset;
  gint parent_xthickness;
  gint available_left, available_right;

  g_return_if_fail (menu != NULL);
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);

  menu_item = BTK_MENU_ITEM (user_data);
  widget = BTK_WIDGET (user_data);

  if (push_in)
    *push_in = FALSE;

  direction = btk_widget_get_direction (widget);

  twidth = BTK_WIDGET (menu)->requisition.width;
  theight = BTK_WIDGET (menu)->requisition.height;

  screen = btk_widget_get_screen (BTK_WIDGET (menu));
  monitor_num = bdk_screen_get_monitor_at_window (screen, menu_item->event_window);
  if (monitor_num < 0)
    monitor_num = 0;
  bdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

  if (!bdk_window_get_origin (widget->window, &tx, &ty))
    {
      g_warning ("Menu not on screen");
      return;
    }

  tx += widget->allocation.x;
  ty += widget->allocation.y;

  get_offsets (menu, &horizontal_offset, &vertical_offset);

  available_left = tx - monitor.x;
  available_right = monitor.x + monitor.width - (tx + widget->allocation.width);

  if (BTK_IS_MENU_BAR (widget->parent))
    {
      menu_item->from_menubar = TRUE;
    }
  else if (BTK_IS_MENU (widget->parent))
    {
      if (BTK_MENU (widget->parent)->parent_menu_item)
	menu_item->from_menubar = BTK_MENU_ITEM (BTK_MENU (widget->parent)->parent_menu_item)->from_menubar;
      else
	menu_item->from_menubar = FALSE;
    }
  else
    {
      menu_item->from_menubar = FALSE;
    }
  
  switch (menu_item->submenu_placement)
    {
    case BTK_TOP_BOTTOM:
      if (direction == BTK_TEXT_DIR_LTR)
	menu_item->submenu_direction = BTK_DIRECTION_RIGHT;
      else 
	{
	  menu_item->submenu_direction = BTK_DIRECTION_LEFT;
	  tx += widget->allocation.width - twidth;
	}
      if ((ty + widget->allocation.height + theight) <= monitor.y + monitor.height)
	ty += widget->allocation.height;
      else if ((ty - theight) >= monitor.y)
	ty -= theight;
      else if (monitor.y + monitor.height - (ty + widget->allocation.height) > ty)
	ty += widget->allocation.height;
      else
	ty -= theight;
      break;

    case BTK_LEFT_RIGHT:
      if (BTK_IS_MENU (widget->parent))
	parent_menu_item = BTK_MENU_ITEM (BTK_MENU (widget->parent)->parent_menu_item);
      else
	parent_menu_item = NULL;
      
      parent_xthickness = widget->parent->style->xthickness;

      if (parent_menu_item && !BTK_MENU (widget->parent)->torn_off)
	{
	  menu_item->submenu_direction = parent_menu_item->submenu_direction;
	}
      else
	{
	  if (direction == BTK_TEXT_DIR_LTR)
	    menu_item->submenu_direction = BTK_DIRECTION_RIGHT;
	  else
	    menu_item->submenu_direction = BTK_DIRECTION_LEFT;
	}

      switch (menu_item->submenu_direction)
	{
	case BTK_DIRECTION_LEFT:
	  if (tx - twidth - parent_xthickness - horizontal_offset >= monitor.x ||
	      available_left >= available_right)
	    tx -= twidth + parent_xthickness + horizontal_offset;
	  else
	    {
	      menu_item->submenu_direction = BTK_DIRECTION_RIGHT;
	      tx += widget->allocation.width + parent_xthickness + horizontal_offset;
	    }
	  break;

	case BTK_DIRECTION_RIGHT:
	  if (tx + widget->allocation.width + parent_xthickness + horizontal_offset + twidth <= monitor.x + monitor.width ||
	      available_right >= available_left)
	    tx += widget->allocation.width + parent_xthickness + horizontal_offset;
	  else
	    {
	      menu_item->submenu_direction = BTK_DIRECTION_LEFT;
	      tx -= twidth + parent_xthickness + horizontal_offset;
	    }
	  break;
	}

      ty += vertical_offset;
      
      /* If the height of the menu doesn't fit we move it upward. */
      ty = CLAMP (ty, monitor.y, MAX (monitor.y, monitor.y + monitor.height - theight));
      break;
    }

  /* If we have negative, tx, here it is because we can't get
   * the menu all the way on screen. Favor the left portion.
   */
  *x = CLAMP (tx, monitor.x, MAX (monitor.x, monitor.x + monitor.width - twidth));
  *y = ty;

  btk_menu_set_monitor (menu, monitor_num);

  if (!btk_widget_get_visible (menu->toplevel))
    {
      btk_window_set_type_hint (BTK_WINDOW (menu->toplevel), menu_item->from_menubar?
				BDK_WINDOW_TYPE_HINT_DROPDOWN_MENU : BDK_WINDOW_TYPE_HINT_POPUP_MENU);
    }
}

/**
 * btk_menu_item_set_right_justified:
 * @menu_item: a #BtkMenuItem.
 * @right_justified: if %TRUE the menu item will appear at the 
 *   far right if added to a menu bar.
 * 
 * Sets whether the menu item appears justified at the right
 * side of a menu bar. This was traditionally done for "Help" menu
 * items, but is now considered a bad idea. (If the widget
 * layout is reversed for a right-to-left language like Hebrew
 * or Arabic, right-justified-menu-items appear at the left.)
 **/
void
btk_menu_item_set_right_justified (BtkMenuItem *menu_item,
				   gboolean     right_justified)
{
  g_return_if_fail (BTK_IS_MENU_ITEM (menu_item));

  right_justified = right_justified != FALSE;

  if (right_justified != menu_item->right_justify)
    {
      menu_item->right_justify = right_justified;
      btk_widget_queue_resize (BTK_WIDGET (menu_item));
    }
}

/**
 * btk_menu_item_get_right_justified:
 * @menu_item: a #BtkMenuItem
 * 
 * Gets whether the menu item appears justified at the right
 * side of the menu bar.
 * 
 * Return value: %TRUE if the menu item will appear at the
 *   far right if added to a menu bar.
 **/
gboolean
btk_menu_item_get_right_justified (BtkMenuItem *menu_item)
{
  g_return_val_if_fail (BTK_IS_MENU_ITEM (menu_item), FALSE);
  
  return menu_item->right_justify;
}


static void
btk_menu_item_show_all (BtkWidget *widget)
{
  BtkMenuItem *menu_item;

  g_return_if_fail (BTK_IS_MENU_ITEM (widget));

  menu_item = BTK_MENU_ITEM (widget);

  /* show children including submenu */
  if (menu_item->submenu)
    btk_widget_show_all (menu_item->submenu);
  btk_container_foreach (BTK_CONTAINER (widget), (BtkCallback) btk_widget_show_all, NULL);

  btk_widget_show (widget);
}

static void
btk_menu_item_hide_all (BtkWidget *widget)
{
  BtkMenuItem *menu_item;

  g_return_if_fail (BTK_IS_MENU_ITEM (widget));

  btk_widget_hide (widget);

  menu_item = BTK_MENU_ITEM (widget);

  /* hide children including submenu */
  btk_container_foreach (BTK_CONTAINER (widget), (BtkCallback) btk_widget_hide_all, NULL);
  if (menu_item->submenu)
    btk_widget_hide_all (menu_item->submenu);
}

static gboolean
btk_menu_item_can_activate_accel (BtkWidget *widget,
				  guint      signal_id)
{
  /* Chain to the parent BtkMenu for further checks */
  return (btk_widget_is_sensitive (widget) && btk_widget_get_visible (widget) &&
          widget->parent && btk_widget_can_activate_accel (widget->parent, signal_id));
}

static void
btk_menu_item_accel_name_foreach (BtkWidget *widget,
				  gpointer data)
{
  const gchar **path_p = data;

  if (!*path_p)
    {
      if (BTK_IS_LABEL (widget))
	{
	  *path_p = btk_label_get_text (BTK_LABEL (widget));
	  if (*path_p && (*path_p)[0] == 0)
	    *path_p = NULL;
	}
      else if (BTK_IS_CONTAINER (widget))
	btk_container_foreach (BTK_CONTAINER (widget),
			       btk_menu_item_accel_name_foreach,
			       data);
    }
}

static void
btk_menu_item_parent_set (BtkWidget *widget,
			  BtkWidget *previous_parent)
{
  BtkMenuItem *menu_item = BTK_MENU_ITEM (widget);
  BtkMenu *menu = BTK_IS_MENU (widget->parent) ? BTK_MENU (widget->parent) : NULL;

  if (menu)
    _btk_menu_item_refresh_accel_path (menu_item,
				       menu->accel_path,
				       menu->accel_group,
				       TRUE);

  if (BTK_WIDGET_CLASS (btk_menu_item_parent_class)->parent_set)
    BTK_WIDGET_CLASS (btk_menu_item_parent_class)->parent_set (widget, previous_parent);
}

void
_btk_menu_item_refresh_accel_path (BtkMenuItem   *menu_item,
				   const gchar   *prefix,
				   BtkAccelGroup *accel_group,
				   gboolean       group_changed)
{
  const gchar *path;
  BtkWidget *widget;

  g_return_if_fail (BTK_IS_MENU_ITEM (menu_item));
  g_return_if_fail (!accel_group || BTK_IS_ACCEL_GROUP (accel_group));

  widget = BTK_WIDGET (menu_item);

  if (!accel_group)
    {
      btk_widget_set_accel_path (widget, NULL, NULL);
      return;
    }

  path = _btk_widget_get_accel_path (widget, NULL);
  if (!path)					/* no active accel_path yet */
    {
      path = menu_item->accel_path;
      if (!path && prefix)
	{
	  const gchar *postfix = NULL;
          gchar *new_path;

	  /* try to construct one from label text */
	  btk_container_foreach (BTK_CONTAINER (menu_item),
				 btk_menu_item_accel_name_foreach,
				 &postfix);
          if (postfix)
            {
              new_path = g_strconcat (prefix, "/", postfix, NULL);
              path = menu_item->accel_path = (char*)g_intern_string (new_path);
              g_free (new_path);
            }
	}
      if (path)
	btk_widget_set_accel_path (widget, path, accel_group);
    }
  else if (group_changed)			/* reinstall accelerators */
    btk_widget_set_accel_path (widget, path, accel_group);
}

/**
 * btk_menu_item_set_accel_path
 * @menu_item:  a valid #BtkMenuItem
 * @accel_path: (allow-none): accelerator path, corresponding to this menu item's
 *              functionality, or %NULL to unset the current path.
 *
 * Set the accelerator path on @menu_item, through which runtime changes of the
 * menu item's accelerator caused by the user can be identified and saved to
 * persistant storage (see btk_accel_map_save() on this).
 * To setup a default accelerator for this menu item, call
 * btk_accel_map_add_entry() with the same @accel_path.
 * See also btk_accel_map_add_entry() on the specifics of accelerator paths,
 * and btk_menu_set_accel_path() for a more convenient variant of this function.
 *
 * This function is basically a convenience wrapper that handles calling
 * btk_widget_set_accel_path() with the appropriate accelerator group for
 * the menu item.
 *
 * Note that you do need to set an accelerator on the parent menu with
 * btk_menu_set_accel_group() for this to work.
 *
 * Note that @accel_path string will be stored in a #GQuark. Therefore, if you
 * pass a static string, you can save some memory by interning it first with 
 * g_intern_static_string().
 */
void
btk_menu_item_set_accel_path (BtkMenuItem *menu_item,
			      const gchar *accel_path)
{
  BtkWidget *widget;

  g_return_if_fail (BTK_IS_MENU_ITEM (menu_item));
  g_return_if_fail (accel_path == NULL ||
		    (accel_path[0] == '<' && strchr (accel_path, '/')));

  widget = BTK_WIDGET (menu_item);

  /* store new path */
  menu_item->accel_path = (char*)g_intern_string (accel_path);

  /* forget accelerators associated with old path */
  btk_widget_set_accel_path (widget, NULL, NULL);

  /* install accelerators associated with new path */
  if (BTK_IS_MENU (widget->parent))
    {
      BtkMenu *menu = BTK_MENU (widget->parent);

      if (menu->accel_group)
	_btk_menu_item_refresh_accel_path (BTK_MENU_ITEM (widget),
					   NULL,
					   menu->accel_group,
					   FALSE);
    }
}

/**
 * btk_menu_item_get_accel_path
 * @menu_item:  a valid #BtkMenuItem
 *
 * Retrieve the accelerator path that was previously set on @menu_item.
 *
 * See btk_menu_item_set_accel_path() for details.
 *
 * Returns: the accelerator path corresponding to this menu item's
 *              functionality, or %NULL if not set
 *
 * Since: 2.14
 */
const gchar *
btk_menu_item_get_accel_path (BtkMenuItem *menu_item)
{
  g_return_val_if_fail (BTK_IS_MENU_ITEM (menu_item), NULL);

  return menu_item->accel_path;
}

static void
btk_menu_item_forall (BtkContainer *container,
		      gboolean      include_internals,
		      BtkCallback   callback,
		      gpointer      callback_data)
{
  BtkBin *bin;

  g_return_if_fail (BTK_IS_MENU_ITEM (container));
  g_return_if_fail (callback != NULL);

  bin = BTK_BIN (container);

  if (bin->child)
    callback (bin->child, callback_data);
}

gboolean
_btk_menu_item_is_selectable (BtkWidget *menu_item)
{
  if ((!BTK_BIN (menu_item)->child &&
       B_OBJECT_TYPE (menu_item) == BTK_TYPE_MENU_ITEM) ||
      BTK_IS_SEPARATOR_MENU_ITEM (menu_item) ||
      !btk_widget_is_sensitive (menu_item) ||
      !btk_widget_get_visible (menu_item))
    return FALSE;

  return TRUE;
}

static void
btk_menu_item_ensure_label (BtkMenuItem *menu_item)
{
  BtkWidget *accel_label;

  if (!BTK_BIN (menu_item)->child)
    {
      accel_label = g_object_new (BTK_TYPE_ACCEL_LABEL, NULL);
      btk_misc_set_alignment (BTK_MISC (accel_label), 0.0, 0.5);

      btk_container_add (BTK_CONTAINER (menu_item), accel_label);
      btk_accel_label_set_accel_widget (BTK_ACCEL_LABEL (accel_label), 
					BTK_WIDGET (menu_item));
      btk_widget_show (accel_label);
    }
}

/**
 * btk_menu_item_set_label:
 * @menu_item: a #BtkMenuItem
 * @label: the text you want to set
 *
 * Sets @text on the @menu_item label
 *
 * Since: 2.16
 **/
void
btk_menu_item_set_label (BtkMenuItem *menu_item,
			 const gchar *label)
{
  g_return_if_fail (BTK_IS_MENU_ITEM (menu_item));

  BTK_MENU_ITEM_GET_CLASS (menu_item)->set_label (menu_item, label);
}

/**
 * btk_menu_item_get_label:
 * @menu_item: a #BtkMenuItem
 *
 * Sets @text on the @menu_item label
 *
 * Returns: The text in the @menu_item label. This is the internal
 *   string used by the label, and must not be modified.
 *
 * Since: 2.16
 **/
const gchar *
btk_menu_item_get_label (BtkMenuItem *menu_item)
{
  g_return_val_if_fail (BTK_IS_MENU_ITEM (menu_item), NULL);

  return BTK_MENU_ITEM_GET_CLASS (menu_item)->get_label (menu_item);
}

/**
 * btk_menu_item_set_use_underline:
 * @menu_item: a #BtkMenuItem
 * @setting: %TRUE if underlines in the text indicate mnemonics  
 *
 * If true, an underline in the text indicates the next character should be
 * used for the mnemonic accelerator key.
 *
 * Since: 2.16
 **/
void
btk_menu_item_set_use_underline (BtkMenuItem *menu_item,
				 gboolean     setting)
{
  g_return_if_fail (BTK_IS_MENU_ITEM (menu_item));

  btk_menu_item_ensure_label (menu_item);

  if (BTK_IS_LABEL (BTK_BIN (menu_item)->child))
    {
      btk_label_set_use_underline (BTK_LABEL (BTK_BIN (menu_item)->child), setting);

      g_object_notify (B_OBJECT (menu_item), "use-underline");
    }
}

/**
 * btk_menu_item_get_use_underline:
 * @menu_item: a #BtkMenuItem
 *
 * Checks if an underline in the text indicates the next character should be
 * used for the mnemonic accelerator key.
 *
 * Return value: %TRUE if an embedded underline in the label indicates
 *               the mnemonic accelerator key.
 *
 * Since: 2.16
 **/
gboolean
btk_menu_item_get_use_underline (BtkMenuItem *menu_item)
{
  g_return_val_if_fail (BTK_IS_MENU_ITEM (menu_item), FALSE);

  btk_menu_item_ensure_label (menu_item);
  
  if (BTK_IS_LABEL (BTK_BIN (menu_item)->child))
    return btk_label_get_use_underline (BTK_LABEL (BTK_BIN (menu_item)->child));

  return FALSE;
}



#define __BTK_MENU_ITEM_C__
#include "btkaliasdef.c"
