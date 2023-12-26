/* BTK - The GIMP Toolkit
 * Copyright (C) 2001 Red Hat, Inc.
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

#include "config.h"
#include "btkimagemenuitem.h"
#include "btkaccellabel.h"
#include "btkintl.h"
#include "btkstock.h"
#include "btkiconfactory.h"
#include "btkimage.h"
#include "btkmenubar.h"
#include "btkcontainer.h"
#include "btkwindow.h"
#include "btkactivatable.h"
#include "btkprivate.h"
#include "btkalias.h"

static void btk_image_menu_item_destroy              (BtkObject        *object);
static void btk_image_menu_item_size_request         (BtkWidget        *widget,
                                                      BtkRequisition   *requisition);
static void btk_image_menu_item_size_allocate        (BtkWidget        *widget,
                                                      BtkAllocation    *allocation);
static void btk_image_menu_item_map                  (BtkWidget        *widget);
static void btk_image_menu_item_remove               (BtkContainer     *container,
                                                      BtkWidget        *child);
static void btk_image_menu_item_toggle_size_request  (BtkMenuItem      *menu_item,
						      bint             *requisition);
static void btk_image_menu_item_set_label            (BtkMenuItem      *menu_item,
						      const bchar      *label);
static const bchar *btk_image_menu_item_get_label (BtkMenuItem *menu_item);

static void btk_image_menu_item_forall               (BtkContainer    *container,
						      bboolean	       include_internals,
						      BtkCallback      callback,
						      bpointer         callback_data);

static void btk_image_menu_item_finalize             (BObject         *object);
static void btk_image_menu_item_set_property         (BObject         *object,
						      buint            prop_id,
						      const BValue    *value,
						      BParamSpec      *pspec);
static void btk_image_menu_item_get_property         (BObject         *object,
						      buint            prop_id,
						      BValue          *value,
						      BParamSpec      *pspec);
static void btk_image_menu_item_screen_changed       (BtkWidget        *widget,
						      BdkScreen        *previous_screen);

static void btk_image_menu_item_recalculate          (BtkImageMenuItem *image_menu_item);

static void btk_image_menu_item_activatable_interface_init (BtkActivatableIface  *iface);
static void btk_image_menu_item_update                     (BtkActivatable       *activatable,
							    BtkAction            *action,
							    const bchar          *property_name);
static void btk_image_menu_item_sync_action_properties     (BtkActivatable       *activatable,
							    BtkAction            *action);

typedef struct {
  bchar *label;
  buint  use_stock         : 1;
  buint  always_show_image : 1;
} BtkImageMenuItemPrivate;

enum {
  PROP_0,
  PROP_IMAGE,
  PROP_USE_STOCK,
  PROP_ACCEL_GROUP,
  PROP_ALWAYS_SHOW_IMAGE
};

static BtkActivatableIface *parent_activatable_iface;


G_DEFINE_TYPE_WITH_CODE (BtkImageMenuItem, btk_image_menu_item, BTK_TYPE_MENU_ITEM,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_ACTIVATABLE,
						btk_image_menu_item_activatable_interface_init))

#define GET_PRIVATE(object)  \
  (B_TYPE_INSTANCE_GET_PRIVATE ((object), BTK_TYPE_IMAGE_MENU_ITEM, BtkImageMenuItemPrivate))

static void
btk_image_menu_item_class_init (BtkImageMenuItemClass *klass)
{
  BObjectClass *bobject_class = (BObjectClass*) klass;
  BtkObjectClass *object_class = (BtkObjectClass*) klass;
  BtkWidgetClass *widget_class = (BtkWidgetClass*) klass;
  BtkMenuItemClass *menu_item_class = (BtkMenuItemClass*) klass;
  BtkContainerClass *container_class = (BtkContainerClass*) klass;

  object_class->destroy = btk_image_menu_item_destroy;

  widget_class->screen_changed = btk_image_menu_item_screen_changed;
  widget_class->size_request = btk_image_menu_item_size_request;
  widget_class->size_allocate = btk_image_menu_item_size_allocate;
  widget_class->map = btk_image_menu_item_map;

  container_class->forall = btk_image_menu_item_forall;
  container_class->remove = btk_image_menu_item_remove;
  
  menu_item_class->toggle_size_request = btk_image_menu_item_toggle_size_request;
  menu_item_class->set_label           = btk_image_menu_item_set_label;
  menu_item_class->get_label           = btk_image_menu_item_get_label;

  bobject_class->finalize     = btk_image_menu_item_finalize;
  bobject_class->set_property = btk_image_menu_item_set_property;
  bobject_class->get_property = btk_image_menu_item_get_property;
  
  g_object_class_install_property (bobject_class,
                                   PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        P_("Image widget"),
                                                        P_("Child widget to appear next to the menu text"),
                                                        BTK_TYPE_WIDGET,
                                                        BTK_PARAM_READWRITE));
  /**
   * BtkImageMenuItem:use-stock:
   *
   * If %TRUE, the label set in the menuitem is used as a
   * stock id to select the stock item for the item.
   *
   * Since: 2.16
   **/
  g_object_class_install_property (bobject_class,
                                   PROP_USE_STOCK,
                                   g_param_spec_boolean ("use-stock",
							 P_("Use stock"),
							 P_("Whether to use the label text to create a stock menu item"),
							 FALSE,
							 BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  /**
   * BtkImageMenuItem:always-show-image:
   *
   * If %TRUE, the menu item will ignore the #BtkSettings:btk-menu-images 
   * setting and always show the image, if available.
   *
   * Use this property if the menuitem would be useless or hard to use
   * without the image. 
   *
   * Since: 2.16
   **/
  g_object_class_install_property (bobject_class,
                                   PROP_ALWAYS_SHOW_IMAGE,
                                   g_param_spec_boolean ("always-show-image",
							 P_("Always show image"),
							 P_("Whether the image will always be shown"),
							 FALSE,
							 BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  /**
   * BtkImageMenuItem:accel-group:
   *
   * The Accel Group to use for stock accelerator keys
   *
   * Since: 2.16
   **/
  g_object_class_install_property (bobject_class,
                                   PROP_ACCEL_GROUP,
                                   g_param_spec_object ("accel-group",
							P_("Accel Group"),
							P_("The Accel Group to use for stock accelerator keys"),
							BTK_TYPE_ACCEL_GROUP,
							BTK_PARAM_WRITABLE));

  g_type_class_add_private (klass, sizeof (BtkImageMenuItemPrivate));
}

static void
btk_image_menu_item_init (BtkImageMenuItem *image_menu_item)
{
  BtkImageMenuItemPrivate *priv = GET_PRIVATE (image_menu_item);

  priv->use_stock   = FALSE;
  priv->label  = NULL;

  image_menu_item->image = NULL;
}

static void 
btk_image_menu_item_finalize (BObject *object)
{
  BtkImageMenuItemPrivate *priv = GET_PRIVATE (object);

  g_free (priv->label);
  priv->label  = NULL;

  B_OBJECT_CLASS (btk_image_menu_item_parent_class)->finalize (object);
}

static void
btk_image_menu_item_set_property (BObject         *object,
                                  buint            prop_id,
                                  const BValue    *value,
                                  BParamSpec      *pspec)
{
  BtkImageMenuItem *image_menu_item = BTK_IMAGE_MENU_ITEM (object);
  
  switch (prop_id)
    {
    case PROP_IMAGE:
      btk_image_menu_item_set_image (image_menu_item, (BtkWidget *) b_value_get_object (value));
      break;
    case PROP_USE_STOCK:
      btk_image_menu_item_set_use_stock (image_menu_item, b_value_get_boolean (value));
      break;
    case PROP_ALWAYS_SHOW_IMAGE:
      btk_image_menu_item_set_always_show_image (image_menu_item, b_value_get_boolean (value));
      break;
    case PROP_ACCEL_GROUP:
      btk_image_menu_item_set_accel_group (image_menu_item, b_value_get_object (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_image_menu_item_get_property (BObject         *object,
                                  buint            prop_id,
                                  BValue          *value,
                                  BParamSpec      *pspec)
{
  BtkImageMenuItem *image_menu_item = BTK_IMAGE_MENU_ITEM (object);
  
  switch (prop_id)
    {
    case PROP_IMAGE:
      b_value_set_object (value, btk_image_menu_item_get_image (image_menu_item));
      break;
    case PROP_USE_STOCK:
      b_value_set_boolean (value, btk_image_menu_item_get_use_stock (image_menu_item));      
      break;
    case PROP_ALWAYS_SHOW_IMAGE:
      b_value_set_boolean (value, btk_image_menu_item_get_always_show_image (image_menu_item));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static bboolean
show_image (BtkImageMenuItem *image_menu_item)
{
  BtkImageMenuItemPrivate *priv = GET_PRIVATE (image_menu_item);
  BtkSettings *settings = btk_widget_get_settings (BTK_WIDGET (image_menu_item));
  bboolean show;

  if (priv->always_show_image)
    show = TRUE;
  else
    g_object_get (settings, "btk-menu-images", &show, NULL);

  return show;
}

static void
btk_image_menu_item_map (BtkWidget *widget)
{
  BtkImageMenuItem *image_menu_item = BTK_IMAGE_MENU_ITEM (widget);

  BTK_WIDGET_CLASS (btk_image_menu_item_parent_class)->map (widget);

  if (image_menu_item->image)
    g_object_set (image_menu_item->image,
                  "visible", show_image (image_menu_item),
                  NULL);
}

static void
btk_image_menu_item_destroy (BtkObject *object)
{
  BtkImageMenuItem *image_menu_item = BTK_IMAGE_MENU_ITEM (object);

  if (image_menu_item->image)
    btk_container_remove (BTK_CONTAINER (image_menu_item),
                          image_menu_item->image);

  BTK_OBJECT_CLASS (btk_image_menu_item_parent_class)->destroy (object);
}

static void
btk_image_menu_item_toggle_size_request (BtkMenuItem *menu_item,
					 bint        *requisition)
{
  BtkImageMenuItem *image_menu_item = BTK_IMAGE_MENU_ITEM (menu_item);
  BtkPackDirection pack_dir;
  
  if (BTK_IS_MENU_BAR (BTK_WIDGET (menu_item)->parent))
    pack_dir = btk_menu_bar_get_child_pack_direction (BTK_MENU_BAR (BTK_WIDGET (menu_item)->parent));
  else
    pack_dir = BTK_PACK_DIRECTION_LTR;

  *requisition = 0;

  if (image_menu_item->image && btk_widget_get_visible (image_menu_item->image))
    {
      BtkRequisition image_requisition;
      buint toggle_spacing;
      btk_widget_get_child_requisition (image_menu_item->image,
                                        &image_requisition);

      btk_widget_style_get (BTK_WIDGET (menu_item),
			    "toggle-spacing", &toggle_spacing,
			    NULL);
      
      if (pack_dir == BTK_PACK_DIRECTION_LTR || pack_dir == BTK_PACK_DIRECTION_RTL)
	{
	  if (image_requisition.width > 0)
	    *requisition = image_requisition.width + toggle_spacing;
	}
      else
	{
	  if (image_requisition.height > 0)
	    *requisition = image_requisition.height + toggle_spacing;
	}
    }
}

static void
btk_image_menu_item_recalculate (BtkImageMenuItem *image_menu_item)
{
  BtkImageMenuItemPrivate *priv = GET_PRIVATE (image_menu_item);
  BtkStockItem             stock_item;
  BtkWidget               *image;
  const bchar             *resolved_label = priv->label;

  if (priv->use_stock && priv->label)
    {

      if (!image_menu_item->image)
	{
	  image = btk_image_new_from_stock (priv->label, BTK_ICON_SIZE_MENU);
	  btk_image_menu_item_set_image (image_menu_item, image);
	}

      if (btk_stock_lookup (priv->label, &stock_item))
	  resolved_label = stock_item.label;

	btk_menu_item_set_use_underline (BTK_MENU_ITEM (image_menu_item), TRUE);
    }

  BTK_MENU_ITEM_CLASS
    (btk_image_menu_item_parent_class)->set_label (BTK_MENU_ITEM (image_menu_item), resolved_label);

}

static void 
btk_image_menu_item_set_label (BtkMenuItem      *menu_item,
			       const bchar      *label)
{
  BtkImageMenuItemPrivate *priv = GET_PRIVATE (menu_item);

  if (priv->label != label)
    {
      g_free (priv->label);
      priv->label = g_strdup (label);

      btk_image_menu_item_recalculate (BTK_IMAGE_MENU_ITEM (menu_item));

      g_object_notify (B_OBJECT (menu_item), "label");

    }
}

static const bchar *
btk_image_menu_item_get_label (BtkMenuItem *menu_item)
{
  BtkImageMenuItemPrivate *priv = GET_PRIVATE (menu_item);
  
  return priv->label;
}

static void
btk_image_menu_item_size_request (BtkWidget      *widget,
                                  BtkRequisition *requisition)
{
  BtkImageMenuItem *image_menu_item;
  bint child_width = 0;
  bint child_height = 0;
  BtkPackDirection pack_dir;
  
  if (BTK_IS_MENU_BAR (widget->parent))
    pack_dir = btk_menu_bar_get_child_pack_direction (BTK_MENU_BAR (widget->parent));
  else
    pack_dir = BTK_PACK_DIRECTION_LTR;

  image_menu_item = BTK_IMAGE_MENU_ITEM (widget);

  if (image_menu_item->image && btk_widget_get_visible (image_menu_item->image))
    {
      BtkRequisition child_requisition;
      
      btk_widget_size_request (image_menu_item->image,
                               &child_requisition);

      child_width = child_requisition.width;
      child_height = child_requisition.height;
    }

  BTK_WIDGET_CLASS (btk_image_menu_item_parent_class)->size_request (widget, requisition);

  /* not done with height since that happens via the
   * toggle_size_request
   */
  if (pack_dir == BTK_PACK_DIRECTION_LTR || pack_dir == BTK_PACK_DIRECTION_RTL)
    requisition->height = MAX (requisition->height, child_height);
  else
    requisition->width = MAX (requisition->width, child_width);
    
  
  /* Note that BtkMenuShell always size requests before
   * toggle_size_request, so toggle_size_request will be able to use
   * image_menu_item->image->requisition
   */
}

static void
btk_image_menu_item_size_allocate (BtkWidget     *widget,
                                   BtkAllocation *allocation)
{
  BtkImageMenuItem *image_menu_item;
  BtkPackDirection pack_dir;
  
  if (BTK_IS_MENU_BAR (widget->parent))
    pack_dir = btk_menu_bar_get_child_pack_direction (BTK_MENU_BAR (widget->parent));
  else
    pack_dir = BTK_PACK_DIRECTION_LTR;
  
  image_menu_item = BTK_IMAGE_MENU_ITEM (widget);  

  BTK_WIDGET_CLASS (btk_image_menu_item_parent_class)->size_allocate (widget, allocation);

  if (image_menu_item->image && btk_widget_get_visible (image_menu_item->image))
    {
      bint x, y, offset;
      BtkRequisition child_requisition;
      BtkAllocation child_allocation;
      buint horizontal_padding, toggle_spacing;

      btk_widget_style_get (widget,
			    "horizontal-padding", &horizontal_padding,
			    "toggle-spacing", &toggle_spacing,
			    NULL);
      
      /* Man this is lame hardcoding action, but I can't
       * come up with a solution that's really better.
       */

      btk_widget_get_child_requisition (image_menu_item->image,
                                        &child_requisition);

      if (pack_dir == BTK_PACK_DIRECTION_LTR ||
	  pack_dir == BTK_PACK_DIRECTION_RTL)
	{
	  offset = BTK_CONTAINER (image_menu_item)->border_width +
	    widget->style->xthickness;
	  
	  if ((btk_widget_get_direction (widget) == BTK_TEXT_DIR_LTR) ==
	      (pack_dir == BTK_PACK_DIRECTION_LTR))
	    x = offset + horizontal_padding +
	      (BTK_MENU_ITEM (image_menu_item)->toggle_size -
	       toggle_spacing - child_requisition.width) / 2;
	  else
	    x = widget->allocation.width - offset - horizontal_padding -
	      BTK_MENU_ITEM (image_menu_item)->toggle_size + toggle_spacing +
	      (BTK_MENU_ITEM (image_menu_item)->toggle_size -
	       toggle_spacing - child_requisition.width) / 2;
	  
	  y = (widget->allocation.height - child_requisition.height) / 2;
	}
      else
	{
	  offset = BTK_CONTAINER (image_menu_item)->border_width +
	    widget->style->ythickness;
	  
	  if ((btk_widget_get_direction (widget) == BTK_TEXT_DIR_LTR) ==
	      (pack_dir == BTK_PACK_DIRECTION_TTB))
	    y = offset + horizontal_padding +
	      (BTK_MENU_ITEM (image_menu_item)->toggle_size -
	       toggle_spacing - child_requisition.height) / 2;
	  else
	    y = widget->allocation.height - offset - horizontal_padding -
	      BTK_MENU_ITEM (image_menu_item)->toggle_size + toggle_spacing +
	      (BTK_MENU_ITEM (image_menu_item)->toggle_size -
	       toggle_spacing - child_requisition.height) / 2;

	  x = (widget->allocation.width - child_requisition.width) / 2;
	}
      
      child_allocation.width = child_requisition.width;
      child_allocation.height = child_requisition.height;
      child_allocation.x = widget->allocation.x + MAX (x, 0);
      child_allocation.y = widget->allocation.y + MAX (y, 0);

      btk_widget_size_allocate (image_menu_item->image, &child_allocation);
    }
}

static void
btk_image_menu_item_forall (BtkContainer   *container,
                            bboolean	    include_internals,
                            BtkCallback     callback,
                            bpointer        callback_data)
{
  BtkImageMenuItem *image_menu_item = BTK_IMAGE_MENU_ITEM (container);

  BTK_CONTAINER_CLASS (btk_image_menu_item_parent_class)->forall (container,
                                                                  include_internals,
                                                                  callback,
                                                                  callback_data);

  if (include_internals && image_menu_item->image)
    (* callback) (image_menu_item->image, callback_data);
}


static void 
btk_image_menu_item_activatable_interface_init (BtkActivatableIface  *iface)
{
  parent_activatable_iface = g_type_interface_peek_parent (iface);
  iface->update = btk_image_menu_item_update;
  iface->sync_action_properties = btk_image_menu_item_sync_action_properties;
}

static bboolean
activatable_update_stock_id (BtkImageMenuItem *image_menu_item, BtkAction *action)
{
  BtkWidget   *image;
  const bchar *stock_id  = btk_action_get_stock_id (action);

  image = btk_image_menu_item_get_image (image_menu_item);
	  
  if (BTK_IS_IMAGE (image) &&
      stock_id && btk_icon_factory_lookup_default (stock_id))
    {
      btk_image_set_from_stock (BTK_IMAGE (image), stock_id, BTK_ICON_SIZE_MENU);
      return TRUE;
    }

  return FALSE;
}

static bboolean
activatable_update_gicon (BtkImageMenuItem *image_menu_item, BtkAction *action)
{
  BtkWidget   *image;
  GIcon       *icon = btk_action_get_gicon (action);
  const bchar *stock_id = btk_action_get_stock_id (action);

  image = btk_image_menu_item_get_image (image_menu_item);

  if (icon && BTK_IS_IMAGE (image) &&
      !(stock_id && btk_icon_factory_lookup_default (stock_id)))
    {
      btk_image_set_from_gicon (BTK_IMAGE (image), icon, BTK_ICON_SIZE_MENU);
      return TRUE;
    }

  return FALSE;
}

static void
activatable_update_icon_name (BtkImageMenuItem *image_menu_item, BtkAction *action)
{
  BtkWidget   *image;
  const bchar *icon_name = btk_action_get_icon_name (action);

  image = btk_image_menu_item_get_image (image_menu_item);
	  
  if (BTK_IS_IMAGE (image) && 
      (btk_image_get_storage_type (BTK_IMAGE (image)) == BTK_IMAGE_EMPTY ||
       btk_image_get_storage_type (BTK_IMAGE (image)) == BTK_IMAGE_ICON_NAME))
    {
      btk_image_set_from_icon_name (BTK_IMAGE (image), icon_name, BTK_ICON_SIZE_MENU);
    }
}

static void
btk_image_menu_item_update (BtkActivatable *activatable,
			    BtkAction      *action,
			    const bchar    *property_name)
{
  BtkImageMenuItem *image_menu_item;
  bboolean   use_appearance;

  image_menu_item = BTK_IMAGE_MENU_ITEM (activatable);

  parent_activatable_iface->update (activatable, action, property_name);

  use_appearance = btk_activatable_get_use_action_appearance (activatable);
  if (!use_appearance)
    return;

  if (strcmp (property_name, "stock-id") == 0)
    activatable_update_stock_id (image_menu_item, action);
  else if (strcmp (property_name, "gicon") == 0)
    activatable_update_gicon (image_menu_item, action);
  else if (strcmp (property_name, "icon-name") == 0)
    activatable_update_icon_name (image_menu_item, action);
}

static void 
btk_image_menu_item_sync_action_properties (BtkActivatable *activatable,
			                    BtkAction      *action)
{
  BtkImageMenuItem *image_menu_item;
  BtkWidget *image;
  bboolean   use_appearance;

  image_menu_item = BTK_IMAGE_MENU_ITEM (activatable);

  parent_activatable_iface->sync_action_properties (activatable, action);

  if (!action)
    return;

  use_appearance = btk_activatable_get_use_action_appearance (activatable);
  if (!use_appearance)
    return;

  image = btk_image_menu_item_get_image (image_menu_item);
  if (image && !BTK_IS_IMAGE (image))
    {
      btk_image_menu_item_set_image (image_menu_item, NULL);
      image = NULL;
    }
  
  if (!image)
    {
      image = btk_image_new ();
      btk_widget_show (image);
      btk_image_menu_item_set_image (BTK_IMAGE_MENU_ITEM (activatable),
				     image);
    }
  
  if (!activatable_update_stock_id (image_menu_item, action) &&
      !activatable_update_gicon (image_menu_item, action))
    activatable_update_icon_name (image_menu_item, action);

  btk_image_menu_item_set_always_show_image (image_menu_item,
                                             btk_action_get_always_show_image (action));
}


/**
 * btk_image_menu_item_new:
 * @returns: a new #BtkImageMenuItem.
 *
 * Creates a new #BtkImageMenuItem with an empty label.
 **/
BtkWidget*
btk_image_menu_item_new (void)
{
  return g_object_new (BTK_TYPE_IMAGE_MENU_ITEM, NULL);
}

/**
 * btk_image_menu_item_new_with_label:
 * @label: the text of the menu item.
 * @returns: a new #BtkImageMenuItem.
 *
 * Creates a new #BtkImageMenuItem containing a label. 
 **/
BtkWidget*
btk_image_menu_item_new_with_label (const bchar *label)
{
  return g_object_new (BTK_TYPE_IMAGE_MENU_ITEM, 
		       "label", label,
		       NULL);
}


/**
 * btk_image_menu_item_new_with_mnemonic:
 * @label: the text of the menu item, with an underscore in front of the
 *         mnemonic character
 * @returns: a new #BtkImageMenuItem
 *
 * Creates a new #BtkImageMenuItem containing a label. The label
 * will be created using btk_label_new_with_mnemonic(), so underscores
 * in @label indicate the mnemonic for the menu item.
 **/
BtkWidget*
btk_image_menu_item_new_with_mnemonic (const bchar *label)
{
  return g_object_new (BTK_TYPE_IMAGE_MENU_ITEM, 
		       "use-underline", TRUE,
		       "label", label,
		       NULL);
}

/**
 * btk_image_menu_item_new_from_stock:
 * @stock_id: the name of the stock item.
 * @accel_group: (allow-none): the #BtkAccelGroup to add the menu items 
 *   accelerator to, or %NULL.
 * @returns: a new #BtkImageMenuItem.
 *
 * Creates a new #BtkImageMenuItem containing the image and text from a 
 * stock item. Some stock ids have preprocessor macros like #BTK_STOCK_OK 
 * and #BTK_STOCK_APPLY.
 *
 * If you want this menu item to have changeable accelerators, then pass in
 * %NULL for accel_group. Next call btk_menu_item_set_accel_path() with an
 * appropriate path for the menu item, use btk_stock_lookup() to look up the
 * standard accelerator for the stock item, and if one is found, call
 * btk_accel_map_add_entry() to register it.
 **/
BtkWidget*
btk_image_menu_item_new_from_stock (const bchar      *stock_id,
				    BtkAccelGroup    *accel_group)
{
  return g_object_new (BTK_TYPE_IMAGE_MENU_ITEM, 
		       "label", stock_id,
		       "use-stock", TRUE,
		       "accel-group", accel_group,
		       NULL);
}

/**
 * btk_image_menu_item_set_use_stock:
 * @image_menu_item: a #BtkImageMenuItem
 * @use_stock: %TRUE if the menuitem should use a stock item
 *
 * If %TRUE, the label set in the menuitem is used as a
 * stock id to select the stock item for the item.
 *
 * Since: 2.16
 */
void
btk_image_menu_item_set_use_stock (BtkImageMenuItem *image_menu_item,
				   bboolean          use_stock)
{
  BtkImageMenuItemPrivate *priv;

  g_return_if_fail (BTK_IS_IMAGE_MENU_ITEM (image_menu_item));

  priv = GET_PRIVATE (image_menu_item);

  if (priv->use_stock != use_stock)
    {
      priv->use_stock = use_stock;

      btk_image_menu_item_recalculate (image_menu_item);

      g_object_notify (B_OBJECT (image_menu_item), "use-stock");
    }
}

/**
 * btk_image_menu_item_get_use_stock:
 * @image_menu_item: a #BtkImageMenuItem
 *
 * Checks whether the label set in the menuitem is used as a
 * stock id to select the stock item for the item.
 *
 * Returns: %TRUE if the label set in the menuitem is used as a
 *     stock id to select the stock item for the item
 *
 * Since: 2.16
 */
bboolean
btk_image_menu_item_get_use_stock (BtkImageMenuItem *image_menu_item)
{
  BtkImageMenuItemPrivate *priv;

  g_return_val_if_fail (BTK_IS_IMAGE_MENU_ITEM (image_menu_item), FALSE);

  priv = GET_PRIVATE (image_menu_item);

  return priv->use_stock;
}

/**
 * btk_image_menu_item_set_always_show_image:
 * @image_menu_item: a #BtkImageMenuItem
 * @always_show: %TRUE if the menuitem should always show the image
 *
 * If %TRUE, the menu item will ignore the #BtkSettings:btk-menu-images 
 * setting and always show the image, if available.
 *
 * Use this property if the menuitem would be useless or hard to use
 * without the image. 
 * 
 * Since: 2.16
 */
void
btk_image_menu_item_set_always_show_image (BtkImageMenuItem *image_menu_item,
                                           bboolean          always_show)
{
  BtkImageMenuItemPrivate *priv;

  g_return_if_fail (BTK_IS_IMAGE_MENU_ITEM (image_menu_item));

  priv = GET_PRIVATE (image_menu_item);

  if (priv->always_show_image != always_show)
    {
      priv->always_show_image = always_show;

      if (image_menu_item->image)
        {
          if (show_image (image_menu_item))
            btk_widget_show (image_menu_item->image);
          else
            btk_widget_hide (image_menu_item->image);
        }

      g_object_notify (B_OBJECT (image_menu_item), "always-show-image");
    }
}

/**
 * btk_image_menu_item_get_always_show_image:
 * @image_menu_item: a #BtkImageMenuItem
 *
 * Returns whether the menu item will ignore the #BtkSettings:btk-menu-images
 * setting and always show the image, if available.
 * 
 * Returns: %TRUE if the menu item will always show the image
 *
 * Since: 2.16
 */
bboolean
btk_image_menu_item_get_always_show_image (BtkImageMenuItem *image_menu_item)
{
  BtkImageMenuItemPrivate *priv;

  g_return_val_if_fail (BTK_IS_IMAGE_MENU_ITEM (image_menu_item), FALSE);

  priv = GET_PRIVATE (image_menu_item);

  return priv->always_show_image;
}


/**
 * btk_image_menu_item_set_accel_group:
 * @image_menu_item: a #BtkImageMenuItem
 * @accel_group: the #BtkAccelGroup
 *
 * Specifies an @accel_group to add the menu items accelerator to
 * (this only applies to stock items so a stock item must already
 * be set, make sure to call btk_image_menu_item_set_use_stock()
 * and btk_menu_item_set_label() with a valid stock item first).
 *
 * If you want this menu item to have changeable accelerators then
 * you shouldnt need this (see btk_image_menu_item_new_from_stock()).
 *
 * Since: 2.16
 */
void
btk_image_menu_item_set_accel_group (BtkImageMenuItem *image_menu_item, 
				     BtkAccelGroup    *accel_group)
{
  BtkImageMenuItemPrivate *priv;
  BtkStockItem             stock_item;

  /* Silent return for the constructor */
  if (!accel_group) 
    return;
  
  g_return_if_fail (BTK_IS_IMAGE_MENU_ITEM (image_menu_item));
  g_return_if_fail (BTK_IS_ACCEL_GROUP (accel_group));

  priv = GET_PRIVATE (image_menu_item);

  if (priv->use_stock && priv->label && btk_stock_lookup (priv->label, &stock_item))
    if (stock_item.keyval)
      {
	btk_widget_add_accelerator (BTK_WIDGET (image_menu_item),
				    "activate",
				    accel_group,
				    stock_item.keyval,
				    stock_item.modifier,
				    BTK_ACCEL_VISIBLE);
	
	g_object_notify (B_OBJECT (image_menu_item), "accel-group");
      }
}

/** 
 * btk_image_menu_item_set_image:
 * @image_menu_item: a #BtkImageMenuItem.
 * @image: (allow-none): a widget to set as the image for the menu item.
 *
 * Sets the image of @image_menu_item to the given widget.
 * Note that it depends on the show-menu-images setting whether
 * the image will be displayed or not.
 **/ 
void
btk_image_menu_item_set_image (BtkImageMenuItem *image_menu_item,
                               BtkWidget        *image)
{
  g_return_if_fail (BTK_IS_IMAGE_MENU_ITEM (image_menu_item));

  if (image == image_menu_item->image)
    return;

  if (image_menu_item->image)
    btk_container_remove (BTK_CONTAINER (image_menu_item),
			  image_menu_item->image);

  image_menu_item->image = image;

  if (image == NULL)
    return;

  btk_widget_set_parent (image, BTK_WIDGET (image_menu_item));
  g_object_set (image,
		"visible", show_image (image_menu_item),
		"no-show-all", TRUE,
		NULL);

  g_object_notify (B_OBJECT (image_menu_item), "image");
}

/**
 * btk_image_menu_item_get_image:
 * @image_menu_item: a #BtkImageMenuItem
 *
 * Gets the widget that is currently set as the image of @image_menu_item.
 * See btk_image_menu_item_set_image().
 *
 * Return value: (transfer none): the widget set as image of @image_menu_item
 **/
BtkWidget*
btk_image_menu_item_get_image (BtkImageMenuItem *image_menu_item)
{
  g_return_val_if_fail (BTK_IS_IMAGE_MENU_ITEM (image_menu_item), NULL);

  return image_menu_item->image;
}

static void
btk_image_menu_item_remove (BtkContainer *container,
                            BtkWidget    *child)
{
  BtkImageMenuItem *image_menu_item;

  image_menu_item = BTK_IMAGE_MENU_ITEM (container);

  if (child == image_menu_item->image)
    {
      bboolean widget_was_visible;
      
      widget_was_visible = btk_widget_get_visible (child);
      
      btk_widget_unparent (child);
      image_menu_item->image = NULL;
      
      if (widget_was_visible &&
          btk_widget_get_visible (BTK_WIDGET (container)))
        btk_widget_queue_resize (BTK_WIDGET (container));

      g_object_notify (B_OBJECT (image_menu_item), "image");
    }
  else
    {
      BTK_CONTAINER_CLASS (btk_image_menu_item_parent_class)->remove (container, child);
    }
}

static void 
show_image_change_notify (BtkImageMenuItem *image_menu_item)
{
  if (image_menu_item->image)
    {
      if (show_image (image_menu_item))
	btk_widget_show (image_menu_item->image);
      else
	btk_widget_hide (image_menu_item->image);
    }
}

static void
traverse_container (BtkWidget *widget,
		    bpointer   data)
{
  if (BTK_IS_IMAGE_MENU_ITEM (widget))
    show_image_change_notify (BTK_IMAGE_MENU_ITEM (widget));
  else if (BTK_IS_CONTAINER (widget))
    btk_container_forall (BTK_CONTAINER (widget), traverse_container, NULL);
}

static void
btk_image_menu_item_setting_changed (BtkSettings *settings)
{
  GList *list, *l;

  list = btk_window_list_toplevels ();

  for (l = list; l; l = l->next)
    btk_container_forall (BTK_CONTAINER (l->data), 
			  traverse_container, NULL);

  g_list_free (list);  
}

static void
btk_image_menu_item_screen_changed (BtkWidget *widget,
				    BdkScreen *previous_screen)
{
  BtkSettings *settings;
  buint show_image_connection;

  if (!btk_widget_has_screen (widget))
    return;

  settings = btk_widget_get_settings (widget);
  
  show_image_connection = 
    BPOINTER_TO_UINT (g_object_get_data (B_OBJECT (settings), 
					 "btk-image-menu-item-connection"));
  
  if (show_image_connection)
    return;

  show_image_connection =
    g_signal_connect (settings, "notify::btk-menu-images",
		      G_CALLBACK (btk_image_menu_item_setting_changed), NULL);
  g_object_set_data (B_OBJECT (settings), 
		     I_("btk-image-menu-item-connection"),
		     BUINT_TO_POINTER (show_image_connection));

  show_image_change_notify (BTK_IMAGE_MENU_ITEM (widget));
}

#define __BTK_IMAGE_MENU_ITEM_C__
#include "btkaliasdef.c"
