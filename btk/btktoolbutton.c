/* btktoolbutton.c
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
#include "btktoolbutton.h"
#include "btkbutton.h"
#include "btkhbox.h"
#include "btkiconfactory.h"
#include "btkimage.h"
#include "btkimagemenuitem.h"
#include "btklabel.h"
#include "btkstock.h"
#include "btkvbox.h"
#include "btkintl.h"
#include "btktoolbar.h"
#include "btkactivatable.h"
#include "btkprivate.h"
#include "btkalias.h"

#include <string.h>

#define MENU_ID "btk-tool-button-menu-id"

enum {
  CLICKED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_LABEL,
  PROP_USE_UNDERLINE,
  PROP_LABEL_WIDGET,
  PROP_STOCK_ID,
  PROP_ICON_NAME,
  PROP_ICON_WIDGET
};

static void btk_tool_button_init          (BtkToolButton      *button,
					   BtkToolButtonClass *klass);
static void btk_tool_button_class_init    (BtkToolButtonClass *klass);
static void btk_tool_button_set_property  (BObject            *object,
					   guint               prop_id,
					   const BValue       *value,
					   BParamSpec         *pspec);
static void btk_tool_button_get_property  (BObject            *object,
					   guint               prop_id,
					   BValue             *value,
					   BParamSpec         *pspec);
static void btk_tool_button_property_notify (BObject          *object,
					     BParamSpec       *pspec);
static void btk_tool_button_finalize      (BObject            *object);

static void btk_tool_button_toolbar_reconfigured (BtkToolItem *tool_item);
static gboolean   btk_tool_button_create_menu_proxy (BtkToolItem     *item);
static void       button_clicked                    (BtkWidget       *widget,
						     BtkToolButton   *button);
static void btk_tool_button_style_set      (BtkWidget          *widget,
					    BtkStyle           *prev_style);

static void btk_tool_button_construct_contents (BtkToolItem *tool_item);

static void btk_tool_button_activatable_interface_init (BtkActivatableIface  *iface);
static void btk_tool_button_update                     (BtkActivatable       *activatable,
							BtkAction            *action,
							const gchar          *property_name);
static void btk_tool_button_sync_action_properties     (BtkActivatable       *activatable,
							BtkAction            *action);


struct _BtkToolButtonPrivate
{
  BtkWidget *button;

  gchar *stock_id;
  gchar *icon_name;
  gchar *label_text;
  BtkWidget *label_widget;
  BtkWidget *icon_widget;

  BtkSizeGroup *text_size_group;

  guint use_underline : 1;
  guint contents_invalid : 1;
};

static BObjectClass        *parent_class = NULL;
static BtkActivatableIface *parent_activatable_iface;
static guint                toolbutton_signals[LAST_SIGNAL] = { 0 };

#define BTK_TOOL_BUTTON_GET_PRIVATE(obj)(B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_TOOL_BUTTON, BtkToolButtonPrivate))

GType
btk_tool_button_get_type (void)
{
  static GType type = 0;
  
  if (!type)
    {
      const GInterfaceInfo activatable_info =
      {
        (GInterfaceInitFunc) btk_tool_button_activatable_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      type = g_type_register_static_simple (BTK_TYPE_TOOL_ITEM,
					    I_("BtkToolButton"),
					    sizeof (BtkToolButtonClass),
					    (GClassInitFunc) btk_tool_button_class_init,
					    sizeof (BtkToolButton),
					    (GInstanceInitFunc) btk_tool_button_init,
					    0);

      g_type_add_interface_static (type, BTK_TYPE_ACTIVATABLE,
                                   &activatable_info);
    }
  return type;
}

static void
btk_tool_button_class_init (BtkToolButtonClass *klass)
{
  BObjectClass *object_class;
  BtkWidgetClass *widget_class;
  BtkToolItemClass *tool_item_class;
  
  parent_class = g_type_class_peek_parent (klass);
  
  object_class = (BObjectClass *)klass;
  widget_class = (BtkWidgetClass *)klass;
  tool_item_class = (BtkToolItemClass *)klass;
  
  object_class->set_property = btk_tool_button_set_property;
  object_class->get_property = btk_tool_button_get_property;
  object_class->notify = btk_tool_button_property_notify;
  object_class->finalize = btk_tool_button_finalize;

  widget_class->style_set = btk_tool_button_style_set;

  tool_item_class->create_menu_proxy = btk_tool_button_create_menu_proxy;
  tool_item_class->toolbar_reconfigured = btk_tool_button_toolbar_reconfigured;
  
  klass->button_type = BTK_TYPE_BUTTON;

  /* Properties are interpreted like this:
   *
   *          - if the tool button has an icon_widget, then that widget
   *            will be used as the icon. Otherwise, if the tool button
   *            has a stock id, the corresponding stock icon will be
   *            used. Otherwise, if the tool button has an icon name,
   *            the corresponding icon from the theme will be used.
   *            Otherwise, the tool button will not have an icon.
   *
   *          - if the tool button has a label_widget then that widget
   *            will be used as the label. Otherwise, if the tool button
   *            has a label text, that text will be used as label. Otherwise,
   *            if the toolbutton has a stock id, the corresponding text
   *            will be used as label. Otherwise, if the tool button has
   *            an icon name, the corresponding icon name from the theme will
   *            be used. Otherwise, the toolbutton will have an empty label.
   *
   *	      - The use_underline property only has an effect when the label
   *            on the toolbutton comes from the label property (ie. not from
   *            label_widget or from stock_id).
   *
   *            In that case, if use_underline is set,
   *
   *			- underscores are removed from the label text before
   *                      the label is shown on the toolbutton unless the
   *                      underscore is followed by another underscore
   *
   *			- an underscore indicates that the next character when
   *                      used in the overflow menu should be used as a
   *                      mnemonic.
   *
   *		In short: use_underline = TRUE means that the label text has
   *            the form "_Open" and the toolbar should take appropriate
   *            action.
   */

  g_object_class_install_property (object_class,
				   PROP_LABEL,
				   g_param_spec_string ("label",
							P_("Label"),
							P_("Text to show in the item."),
							NULL,
							BTK_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_USE_UNDERLINE,
				   g_param_spec_boolean ("use-underline",
							 P_("Use underline"),
							 P_("If set, an underline in the label property indicates that the next character should be used for the mnemonic accelerator key in the overflow menu"),
							 FALSE,
							 BTK_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_LABEL_WIDGET,
				   g_param_spec_object ("label-widget",
							P_("Label widget"),
							P_("Widget to use as the item label"),
							BTK_TYPE_WIDGET,
							BTK_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_STOCK_ID,
				   g_param_spec_string ("stock-id",
							P_("Stock Id"),
							P_("The stock icon displayed on the item"),
							NULL,
							BTK_PARAM_READWRITE));

  /**
   * BtkToolButton:icon-name:
   * 
   * The name of the themed icon displayed on the item.
   * This property only has an effect if not overridden by "label", 
   * "icon_widget" or "stock_id" properties.
   *
   * Since: 2.8 
   */
  g_object_class_install_property (object_class,
				   PROP_ICON_NAME,
				   g_param_spec_string ("icon-name",
							P_("Icon name"),
							P_("The name of the themed icon displayed on the item"),
							NULL,
							BTK_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_ICON_WIDGET,
				   g_param_spec_object ("icon-widget",
							P_("Icon widget"),
							P_("Icon widget to display in the item"),
							BTK_TYPE_WIDGET,
							BTK_PARAM_READWRITE));

  /**
   * BtkButton:icon-spacing:
   * 
   * Spacing in pixels between the icon and label.
   * 
   * Since: 2.10
   */
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("icon-spacing",
							     P_("Icon spacing"),
							     P_("Spacing in pixels between the icon and label"),
							     0,
							     G_MAXINT,
							     3,
							     BTK_PARAM_READWRITE));

/**
 * BtkToolButton::clicked:
 * @toolbutton: the object that emitted the signal
 *
 * This signal is emitted when the tool button is clicked with the mouse
 * or activated with the keyboard.
 **/
  toolbutton_signals[CLICKED] =
    g_signal_new (I_("clicked"),
		  B_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkToolButtonClass, clicked),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
  
  g_type_class_add_private (object_class, sizeof (BtkToolButtonPrivate));
}

static void
btk_tool_button_init (BtkToolButton      *button,
		      BtkToolButtonClass *klass)
{
  BtkToolItem *toolitem = BTK_TOOL_ITEM (button);
  
  button->priv = BTK_TOOL_BUTTON_GET_PRIVATE (button);

  button->priv->contents_invalid = TRUE;

  btk_tool_item_set_homogeneous (toolitem, TRUE);

  /* create button */
  button->priv->button = g_object_new (klass->button_type, NULL);
  btk_button_set_focus_on_click (BTK_BUTTON (button->priv->button), FALSE);
  g_signal_connect_object (button->priv->button, "clicked",
			   G_CALLBACK (button_clicked), button, 0);

  btk_container_add (BTK_CONTAINER (button), button->priv->button);
  btk_widget_show (button->priv->button);
}

static void
btk_tool_button_construct_contents (BtkToolItem *tool_item)
{
  BtkToolButton *button = BTK_TOOL_BUTTON (tool_item);
  BtkWidget *label = NULL;
  BtkWidget *icon = NULL;
  BtkToolbarStyle style;
  gboolean need_label = FALSE;
  gboolean need_icon = FALSE;
  BtkIconSize icon_size;
  BtkWidget *box = NULL;
  guint icon_spacing;
  BtkOrientation text_orientation = BTK_ORIENTATION_HORIZONTAL;
  BtkSizeGroup *size_group = NULL;

  button->priv->contents_invalid = FALSE;

  btk_widget_style_get (BTK_WIDGET (tool_item), 
			"icon-spacing", &icon_spacing,
			NULL);

  if (button->priv->icon_widget && button->priv->icon_widget->parent)
    {
      btk_container_remove (BTK_CONTAINER (button->priv->icon_widget->parent),
			    button->priv->icon_widget);
    }

  if (button->priv->label_widget && button->priv->label_widget->parent)
    {
      btk_container_remove (BTK_CONTAINER (button->priv->label_widget->parent),
			    button->priv->label_widget);
    }

  if (BTK_BIN (button->priv->button)->child)
    {
      /* Note: we are not destroying the label_widget or icon_widget
       * here because they were removed from their containers above
       */
      btk_widget_destroy (BTK_BIN (button->priv->button)->child);
    }

  style = btk_tool_item_get_toolbar_style (BTK_TOOL_ITEM (button));
  
  if (style != BTK_TOOLBAR_TEXT)
    need_icon = TRUE;

  if (style != BTK_TOOLBAR_ICONS && style != BTK_TOOLBAR_BOTH_HORIZ)
    need_label = TRUE;

  if (style == BTK_TOOLBAR_BOTH_HORIZ &&
      (btk_tool_item_get_is_important (BTK_TOOL_ITEM (button)) ||
       btk_tool_item_get_orientation (BTK_TOOL_ITEM (button)) == BTK_ORIENTATION_VERTICAL ||
       btk_tool_item_get_text_orientation (BTK_TOOL_ITEM (button)) == BTK_ORIENTATION_VERTICAL))
    {
      need_label = TRUE;
    }
  
  if (style == BTK_TOOLBAR_ICONS && button->priv->icon_widget == NULL &&
      button->priv->stock_id == NULL && button->priv->icon_name == NULL)
    {
      need_label = TRUE;
      need_icon = FALSE;
      style = BTK_TOOLBAR_TEXT;
    }

  if (style == BTK_TOOLBAR_TEXT && button->priv->label_widget == NULL &&
      button->priv->stock_id == NULL && button->priv->label_text == NULL)
    {
      need_label = FALSE;
      need_icon = TRUE;
      style = BTK_TOOLBAR_ICONS;
    }

  if (need_label)
    {
      if (button->priv->label_widget)
	{
	  label = button->priv->label_widget;
	}
      else
	{
	  BtkStockItem stock_item;
	  gboolean elide;
	  gchar *label_text;

	  if (button->priv->label_text)
	    {
	      label_text = button->priv->label_text;
	      elide = button->priv->use_underline;
	    }
	  else if (button->priv->stock_id && btk_stock_lookup (button->priv->stock_id, &stock_item))
	    {
	      label_text = stock_item.label;
	      elide = TRUE;
	    }
	  else
	    {
	      label_text = "";
	      elide = FALSE;
	    }

	  if (elide)
	    label_text = _btk_toolbar_elide_underscores (label_text);
	  else
	    label_text = g_strdup (label_text);

	  label = btk_label_new (label_text);

	  g_free (label_text);
	  
	  btk_widget_show (label);
	}

      if (BTK_IS_LABEL (label))
        {
          btk_label_set_ellipsize (BTK_LABEL (label),
			           btk_tool_item_get_ellipsize_mode (BTK_TOOL_ITEM (button)));
          text_orientation = btk_tool_item_get_text_orientation (BTK_TOOL_ITEM (button));
          if (text_orientation == BTK_ORIENTATION_HORIZONTAL)
	    {
              btk_label_set_angle (BTK_LABEL (label), 0);
              btk_misc_set_alignment (BTK_MISC (label),
                                      btk_tool_item_get_text_alignment (BTK_TOOL_ITEM (button)),
                                      0.5);
            }
          else
            {
              btk_label_set_ellipsize (BTK_LABEL (label), BANGO_ELLIPSIZE_NONE);
	      if (btk_widget_get_direction (BTK_WIDGET (tool_item)) == BTK_TEXT_DIR_RTL)
	        btk_label_set_angle (BTK_LABEL (label), -90);
	      else
	        btk_label_set_angle (BTK_LABEL (label), 90);
              btk_misc_set_alignment (BTK_MISC (label),
                                      0.5,
                                      1 - btk_tool_item_get_text_alignment (BTK_TOOL_ITEM (button)));
            }
        }
    }

  icon_size = btk_tool_item_get_icon_size (BTK_TOOL_ITEM (button));
  if (need_icon)
    {
      if (button->priv->icon_widget)
	{
	  icon = button->priv->icon_widget;
	  
	  if (BTK_IS_IMAGE (icon))
	    {
	      g_object_set (button->priv->icon_widget,
			    "icon-size", icon_size,
			    NULL);
	    }
	}
      else if (button->priv->stock_id && 
	       btk_icon_factory_lookup_default (button->priv->stock_id))
	{
	  icon = btk_image_new_from_stock (button->priv->stock_id, icon_size);
	  btk_widget_show (icon);
	}
      else if (button->priv->icon_name)
	{
	  icon = btk_image_new_from_icon_name (button->priv->icon_name, icon_size);
	  btk_widget_show (icon);
	}

      if (BTK_IS_MISC (icon) && text_orientation == BTK_ORIENTATION_HORIZONTAL)
	btk_misc_set_alignment (BTK_MISC (icon),
				1.0 - btk_tool_item_get_text_alignment (BTK_TOOL_ITEM (button)),
				0.5);
      else if (BTK_IS_MISC (icon))
	btk_misc_set_alignment (BTK_MISC (icon),
				0.5,
				btk_tool_item_get_text_alignment (BTK_TOOL_ITEM (button)));

      if (icon)
	{
	  size_group = btk_tool_item_get_text_size_group (BTK_TOOL_ITEM (button));
	  if (size_group != NULL)
	    btk_size_group_add_widget (size_group, icon);
	}
    }

  switch (style)
    {
    case BTK_TOOLBAR_ICONS:
      if (icon)
	btk_container_add (BTK_CONTAINER (button->priv->button), icon);
      break;

    case BTK_TOOLBAR_BOTH:
      if (text_orientation == BTK_ORIENTATION_HORIZONTAL)
	box = btk_vbox_new (FALSE, icon_spacing);
      else
	box = btk_hbox_new (FALSE, icon_spacing);
      if (icon)
	btk_box_pack_start (BTK_BOX (box), icon, TRUE, TRUE, 0);
      btk_box_pack_end (BTK_BOX (box), label, FALSE, TRUE, 0);
      btk_container_add (BTK_CONTAINER (button->priv->button), box);
      break;

    case BTK_TOOLBAR_BOTH_HORIZ:
      if (text_orientation == BTK_ORIENTATION_HORIZONTAL)
	{
	  box = btk_hbox_new (FALSE, icon_spacing);
	  if (icon)
	    btk_box_pack_start (BTK_BOX (box), icon, label? FALSE : TRUE, TRUE, 0);
	  if (label)
	    btk_box_pack_end (BTK_BOX (box), label, TRUE, TRUE, 0);
	}
      else
	{
	  box = btk_vbox_new (FALSE, icon_spacing);
	  if (icon)
	    btk_box_pack_end (BTK_BOX (box), icon, label ? FALSE : TRUE, TRUE, 0);
	  if (label)
	    btk_box_pack_start (BTK_BOX (box), label, TRUE, TRUE, 0);
	}
      btk_container_add (BTK_CONTAINER (button->priv->button), box);
      break;

    case BTK_TOOLBAR_TEXT:
      btk_container_add (BTK_CONTAINER (button->priv->button), label);
      break;
    }

  if (box)
    btk_widget_show (box);

  btk_button_set_relief (BTK_BUTTON (button->priv->button),
			 btk_tool_item_get_relief_style (BTK_TOOL_ITEM (button)));

  btk_tool_item_rebuild_menu (tool_item);
  
  btk_widget_queue_resize (BTK_WIDGET (button));
}

static void
btk_tool_button_set_property (BObject         *object,
			      guint            prop_id,
			      const BValue    *value,
			      BParamSpec      *pspec)
{
  BtkToolButton *button = BTK_TOOL_BUTTON (object);
  
  switch (prop_id)
    {
    case PROP_LABEL:
      btk_tool_button_set_label (button, b_value_get_string (value));
      break;
    case PROP_USE_UNDERLINE:
      btk_tool_button_set_use_underline (button, b_value_get_boolean (value));
      break;
    case PROP_LABEL_WIDGET:
      btk_tool_button_set_label_widget (button, b_value_get_object (value));
      break;
    case PROP_STOCK_ID:
      btk_tool_button_set_stock_id (button, b_value_get_string (value));
      break;
    case PROP_ICON_NAME:
      btk_tool_button_set_icon_name (button, b_value_get_string (value));
      break;
    case PROP_ICON_WIDGET:
      btk_tool_button_set_icon_widget (button, b_value_get_object (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_tool_button_property_notify (BObject          *object,
				 BParamSpec       *pspec)
{
  BtkToolButton *button = BTK_TOOL_BUTTON (object);

  if (button->priv->contents_invalid ||
      strcmp ("is-important", pspec->name) == 0)
    btk_tool_button_construct_contents (BTK_TOOL_ITEM (object));

  if (parent_class->notify)
    parent_class->notify (object, pspec);
}

static void
btk_tool_button_get_property (BObject         *object,
			      guint            prop_id,
			      BValue          *value,
			      BParamSpec      *pspec)
{
  BtkToolButton *button = BTK_TOOL_BUTTON (object);

  switch (prop_id)
    {
    case PROP_LABEL:
      b_value_set_string (value, btk_tool_button_get_label (button));
      break;
    case PROP_LABEL_WIDGET:
      b_value_set_object (value, btk_tool_button_get_label_widget (button));
      break;
    case PROP_USE_UNDERLINE:
      b_value_set_boolean (value, btk_tool_button_get_use_underline (button));
      break;
    case PROP_STOCK_ID:
      b_value_set_string (value, button->priv->stock_id);
      break;
    case PROP_ICON_NAME:
      b_value_set_string (value, button->priv->icon_name);
      break;
    case PROP_ICON_WIDGET:
      b_value_set_object (value, button->priv->icon_widget);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_tool_button_finalize (BObject *object)
{
  BtkToolButton *button = BTK_TOOL_BUTTON (object);

  g_free (button->priv->stock_id);
  g_free (button->priv->icon_name);
  g_free (button->priv->label_text);

  if (button->priv->label_widget)
    g_object_unref (button->priv->label_widget);

  if (button->priv->icon_widget)
    g_object_unref (button->priv->icon_widget);
  
  parent_class->finalize (object);
}

static BtkWidget *
clone_image_menu_size (BtkImage *image, BtkSettings *settings)
{
  BtkImageType storage_type = btk_image_get_storage_type (image);

  if (storage_type == BTK_IMAGE_STOCK)
    {
      gchar *stock_id;
      btk_image_get_stock (image, &stock_id, NULL);
      return btk_image_new_from_stock (stock_id, BTK_ICON_SIZE_MENU);
    }
  else if (storage_type == BTK_IMAGE_ICON_NAME)
    {
      const gchar *icon_name;
      btk_image_get_icon_name (image, &icon_name, NULL);
      return btk_image_new_from_icon_name (icon_name, BTK_ICON_SIZE_MENU);
    }
  else if (storage_type == BTK_IMAGE_ICON_SET)
    {
      BtkIconSet *icon_set;
      btk_image_get_icon_set (image, &icon_set, NULL);
      return btk_image_new_from_icon_set (icon_set, BTK_ICON_SIZE_MENU);
    }
  else if (storage_type == BTK_IMAGE_GICON)
    {
      GIcon *icon;
      btk_image_get_gicon (image, &icon, NULL);
      return btk_image_new_from_gicon (icon, BTK_ICON_SIZE_MENU);
    }
  else if (storage_type == BTK_IMAGE_PIXBUF)
    {
      gint width, height;
      
      if (settings &&
	  btk_icon_size_lookup_for_settings (settings, BTK_ICON_SIZE_MENU,
					     &width, &height))
	{
	  BdkPixbuf *src_pixbuf, *dest_pixbuf;
	  BtkWidget *cloned_image;

	  src_pixbuf = btk_image_get_pixbuf (image);
	  dest_pixbuf = bdk_pixbuf_scale_simple (src_pixbuf, width, height,
						 BDK_INTERP_BILINEAR);

	  cloned_image = btk_image_new_from_pixbuf (dest_pixbuf);
	  g_object_unref (dest_pixbuf);

	  return cloned_image;
	}
    }

  return NULL;
}
      
static gboolean
btk_tool_button_create_menu_proxy (BtkToolItem *item)
{
  BtkToolButton *button = BTK_TOOL_BUTTON (item);
  BtkWidget *menu_item;
  BtkWidget *menu_image = NULL;
  BtkStockItem stock_item;
  gboolean use_mnemonic = TRUE;
  const char *label;

  if (_btk_tool_item_create_menu_proxy (item))
    return TRUE;
 
  if (BTK_IS_LABEL (button->priv->label_widget))
    {
      label = btk_label_get_label (BTK_LABEL (button->priv->label_widget));
      use_mnemonic = btk_label_get_use_underline (BTK_LABEL (button->priv->label_widget));
    }
  else if (button->priv->label_text)
    {
      label = button->priv->label_text;
      use_mnemonic = button->priv->use_underline;
    }
  else if (button->priv->stock_id && btk_stock_lookup (button->priv->stock_id, &stock_item))
    {
      label = stock_item.label;
    }
  else
    {
      label = "";
    }
  
  if (use_mnemonic)
    menu_item = btk_image_menu_item_new_with_mnemonic (label);
  else
    menu_item = btk_image_menu_item_new_with_label (label);

  if (BTK_IS_IMAGE (button->priv->icon_widget))
    {
      menu_image = clone_image_menu_size (BTK_IMAGE (button->priv->icon_widget),
					  btk_widget_get_settings (BTK_WIDGET (button)));
    }
  else if (button->priv->stock_id)
    {
      menu_image = btk_image_new_from_stock (button->priv->stock_id, BTK_ICON_SIZE_MENU);
    }

  if (menu_image)
    btk_image_menu_item_set_image (BTK_IMAGE_MENU_ITEM (menu_item), menu_image);

  g_signal_connect_closure_by_id (menu_item,
				  g_signal_lookup ("activate", B_OBJECT_TYPE (menu_item)), 0,
				  g_cclosure_new_object_swap (G_CALLBACK (btk_button_clicked),
							      B_OBJECT (BTK_TOOL_BUTTON (button)->priv->button)),
				  FALSE);

  btk_tool_item_set_proxy_menu_item (BTK_TOOL_ITEM (button), MENU_ID, menu_item);
  
  return TRUE;
}

static void
button_clicked (BtkWidget     *widget,
		BtkToolButton *button)
{
  BtkAction *action;

  action = btk_activatable_get_related_action (BTK_ACTIVATABLE (button));
  
  if (action)
    btk_action_activate (action);

  g_signal_emit_by_name (button, "clicked");
}

static void
btk_tool_button_toolbar_reconfigured (BtkToolItem *tool_item)
{
  btk_tool_button_construct_contents (tool_item);
}

static void 
btk_tool_button_update_icon_spacing (BtkToolButton *button)
{
  BtkWidget *box;
  guint spacing;

  box = BTK_BIN (button->priv->button)->child;
  if (BTK_IS_BOX (box))
    {
      btk_widget_style_get (BTK_WIDGET (button), 
			    "icon-spacing", &spacing,
			    NULL);
      btk_box_set_spacing (BTK_BOX (box), spacing);      
    }
}

static void
btk_tool_button_style_set (BtkWidget *widget,
			   BtkStyle  *prev_style)
{
  btk_tool_button_update_icon_spacing (BTK_TOOL_BUTTON (widget));
}

static void 
btk_tool_button_activatable_interface_init (BtkActivatableIface  *iface)
{
  parent_activatable_iface = g_type_interface_peek_parent (iface);
  iface->update = btk_tool_button_update;
  iface->sync_action_properties = btk_tool_button_sync_action_properties;
}

static void
btk_tool_button_update (BtkActivatable *activatable,
			BtkAction      *action,
			const gchar    *property_name)
{
  BtkToolButton *button;
  BtkWidget *image;

  parent_activatable_iface->update (activatable, action, property_name);

  if (!btk_activatable_get_use_action_appearance (activatable))
    return;

  button = BTK_TOOL_BUTTON (activatable);
  
  if (strcmp (property_name, "short-label") == 0)
    btk_tool_button_set_label (button, btk_action_get_short_label (action));
  else if (strcmp (property_name, "stock-id") == 0)
    btk_tool_button_set_stock_id (button, btk_action_get_stock_id (action));
  else if (strcmp (property_name, "gicon") == 0)
    {
      const gchar *stock_id = btk_action_get_stock_id (action);
      GIcon *icon = btk_action_get_gicon (action);
      BtkIconSize icon_size = BTK_ICON_SIZE_BUTTON;

      if ((stock_id && btk_icon_factory_lookup_default (stock_id)) || !icon)
	image = NULL;
      else 
	{   
	  image = btk_tool_button_get_icon_widget (button);
	  icon_size = btk_tool_item_get_icon_size (BTK_TOOL_ITEM (button));

	  if (!image)
	    image = btk_image_new ();
	}

      btk_tool_button_set_icon_widget (button, image);
      btk_image_set_from_gicon (BTK_IMAGE (image), icon, icon_size);

    }
  else if (strcmp (property_name, "icon-name") == 0)
    btk_tool_button_set_icon_name (button, btk_action_get_icon_name (action));
}

static void
btk_tool_button_sync_action_properties (BtkActivatable *activatable,
				        BtkAction      *action)
{
  BtkToolButton *button;
  GIcon         *icon;
  const gchar   *stock_id;

  parent_activatable_iface->sync_action_properties (activatable, action);

  if (!action)
    return;

  if (!btk_activatable_get_use_action_appearance (activatable))
    return;

  button = BTK_TOOL_BUTTON (activatable);
  stock_id = btk_action_get_stock_id (action);

  btk_tool_button_set_label (button, btk_action_get_short_label (action));
  btk_tool_button_set_use_underline (button, TRUE);
  btk_tool_button_set_stock_id (button, stock_id);
  btk_tool_button_set_icon_name (button, btk_action_get_icon_name (action));

  if (stock_id && btk_icon_factory_lookup_default (stock_id))
      btk_tool_button_set_icon_widget (button, NULL);
  else if ((icon = btk_action_get_gicon (action)) != NULL)
    {
      BtkIconSize icon_size = btk_tool_item_get_icon_size (BTK_TOOL_ITEM (button));
      BtkWidget  *image = btk_tool_button_get_icon_widget (button);
      
      if (!image)
	{
	  image = btk_image_new ();
	  btk_widget_show (image);
	  btk_tool_button_set_icon_widget (button, image);
	}

      btk_image_set_from_gicon (BTK_IMAGE (image), icon, icon_size);
    }
  else if (btk_action_get_icon_name (action))
    btk_tool_button_set_icon_name (button, btk_action_get_icon_name (action));
  else
    btk_tool_button_set_label (button, btk_action_get_short_label (action));
}

/**
 * btk_tool_button_new_from_stock:
 * @stock_id: the name of the stock item 
 *
 * Creates a new #BtkToolButton containing the image and text from a
 * stock item. Some stock ids have preprocessor macros like #BTK_STOCK_OK
 * and #BTK_STOCK_APPLY.
 *
 * It is an error if @stock_id is not a name of a stock item.
 * 
 * Return value: A new #BtkToolButton
 * 
 * Since: 2.4
 **/
BtkToolItem *
btk_tool_button_new_from_stock (const gchar *stock_id)
{
  BtkToolButton *button;

  g_return_val_if_fail (stock_id != NULL, NULL);
    
  button = g_object_new (BTK_TYPE_TOOL_BUTTON,
			 "stock-id", stock_id,
			 NULL);

  return BTK_TOOL_ITEM (button);
}

/**
 * btk_tool_button_new:
 * @label: (allow-none): a string that will be used as label, or %NULL
 * @icon_widget: (allow-none): a #BtkMisc widget that will be used as icon widget, or %NULL
 *
 * Creates a new %BtkToolButton using @icon_widget as icon and @label as
 * label.
 *
 * Return value: A new #BtkToolButton
 * 
 * Since: 2.4
 **/
BtkToolItem *
btk_tool_button_new (BtkWidget	 *icon_widget,
		     const gchar *label)
{
  BtkToolButton *button;

  g_return_val_if_fail (icon_widget == NULL || BTK_IS_MISC (icon_widget), NULL);

  button = g_object_new (BTK_TYPE_TOOL_BUTTON,
                         "label", label,
                         "icon-widget", icon_widget,
			 NULL);

  return BTK_TOOL_ITEM (button);  
}

/**
 * btk_tool_button_set_label:
 * @button: a #BtkToolButton
 * @label: (allow-none): a string that will be used as label, or %NULL.
 *
 * Sets @label as the label used for the tool button. The "label" property
 * only has an effect if not overridden by a non-%NULL "label_widget" property.
 * If both the "label_widget" and "label" properties are %NULL, the label
 * is determined by the "stock_id" property. If the "stock_id" property is also
 * %NULL, @button will not have a label.
 * 
 * Since: 2.4
 **/
void
btk_tool_button_set_label (BtkToolButton *button,
			   const gchar   *label)
{
  gchar *old_label;
  gchar *elided_label;
  BatkObject *accessible;
  
  g_return_if_fail (BTK_IS_TOOL_BUTTON (button));

  old_label = button->priv->label_text;

  button->priv->label_text = g_strdup (label);
  button->priv->contents_invalid = TRUE;     

  if (label)
    {
      elided_label = _btk_toolbar_elide_underscores (label);
      accessible = btk_widget_get_accessible (BTK_WIDGET (button->priv->button));
      batk_object_set_name (accessible, elided_label);
      g_free (elided_label);
    }

  g_free (old_label);
 
  g_object_notify (B_OBJECT (button), "label");
}

/**
 * btk_tool_button_get_label:
 * @button: a #BtkToolButton
 * 
 * Returns the label used by the tool button, or %NULL if the tool button
 * doesn't have a label. or uses a the label from a stock item. The returned
 * string is owned by BTK+, and must not be modified or freed.
 * 
 * Return value: The label, or %NULL
 * 
 * Since: 2.4
 **/
const gchar *
btk_tool_button_get_label (BtkToolButton *button)
{
  g_return_val_if_fail (BTK_IS_TOOL_BUTTON (button), NULL);

  return button->priv->label_text;
}

/**
 * btk_tool_button_set_use_underline:
 * @button: a #BtkToolButton
 * @use_underline: whether the button label has the form "_Open"
 *
 * If set, an underline in the label property indicates that the next character
 * should be used for the mnemonic accelerator key in the overflow menu. For
 * example, if the label property is "_Open" and @use_underline is %TRUE,
 * the label on the tool button will be "Open" and the item on the overflow
 * menu will have an underlined 'O'.
 * 
 * Labels shown on tool buttons never have mnemonics on them; this property
 * only affects the menu item on the overflow menu.
 * 
 * Since: 2.4
 **/
void
btk_tool_button_set_use_underline (BtkToolButton *button,
				   gboolean       use_underline)
{
  g_return_if_fail (BTK_IS_TOOL_BUTTON (button));

  use_underline = use_underline != FALSE;

  if (use_underline != button->priv->use_underline)
    {
      button->priv->use_underline = use_underline;
      button->priv->contents_invalid = TRUE;

      g_object_notify (B_OBJECT (button), "use-underline");
    }
}

/**
 * btk_tool_button_get_use_underline:
 * @button: a #BtkToolButton
 * 
 * Returns whether underscores in the label property are used as mnemonics
 * on menu items on the overflow menu. See btk_tool_button_set_use_underline().
 * 
 * Return value: %TRUE if underscores in the label property are used as
 * mnemonics on menu items on the overflow menu.
 * 
 * Since: 2.4
 **/
gboolean
btk_tool_button_get_use_underline (BtkToolButton *button)
{
  g_return_val_if_fail (BTK_IS_TOOL_BUTTON (button), FALSE);

  return button->priv->use_underline;
}

/**
 * btk_tool_button_set_stock_id:
 * @button: a #BtkToolButton
 * @stock_id: (allow-none): a name of a stock item, or %NULL
 *
 * Sets the name of the stock item. See btk_tool_button_new_from_stock().
 * The stock_id property only has an effect if not
 * overridden by non-%NULL "label" and "icon_widget" properties.
 * 
 * Since: 2.4
 **/
void
btk_tool_button_set_stock_id (BtkToolButton *button,
			      const gchar   *stock_id)
{
  gchar *old_stock_id;
  
  g_return_if_fail (BTK_IS_TOOL_BUTTON (button));

  old_stock_id = button->priv->stock_id;

  button->priv->stock_id = g_strdup (stock_id);
  button->priv->contents_invalid = TRUE;

  g_free (old_stock_id);
  
  g_object_notify (B_OBJECT (button), "stock-id");
}

/**
 * btk_tool_button_get_stock_id:
 * @button: a #BtkToolButton
 * 
 * Returns the name of the stock item. See btk_tool_button_set_stock_id().
 * The returned string is owned by BTK+ and must not be freed or modifed.
 * 
 * Return value: the name of the stock item for @button.
 * 
 * Since: 2.4
 **/
const gchar *
btk_tool_button_get_stock_id (BtkToolButton *button)
{
  g_return_val_if_fail (BTK_IS_TOOL_BUTTON (button), NULL);

  return button->priv->stock_id;
}

/**
 * btk_tool_button_set_icon_name
 * @button: a #BtkToolButton
 * @icon_name: (allow-none): the name of the themed icon
 *
 * Sets the icon for the tool button from a named themed icon.
 * See the docs for #BtkIconTheme for more details.
 * The "icon_name" property only has an effect if not
 * overridden by non-%NULL "label", "icon_widget" and "stock_id"
 * properties.
 * 
 * Since: 2.8
 **/
void
btk_tool_button_set_icon_name (BtkToolButton *button,
			       const gchar   *icon_name)
{
  gchar *old_icon_name;

  g_return_if_fail (BTK_IS_TOOL_BUTTON (button));

  old_icon_name = button->priv->icon_name;

  button->priv->icon_name = g_strdup (icon_name);
  button->priv->contents_invalid = TRUE; 

  g_free (old_icon_name);

  g_object_notify (B_OBJECT (button), "icon-name");
}

/**
 * btk_tool_button_get_icon_name
 * @button: a #BtkToolButton
 * 
 * Returns the name of the themed icon for the tool button,
 * see btk_tool_button_set_icon_name().
 *
 * Returns: the icon name or %NULL if the tool button has
 * no themed icon
 * 
 * Since: 2.8
 **/
const gchar*
btk_tool_button_get_icon_name (BtkToolButton *button)
{
  g_return_val_if_fail (BTK_IS_TOOL_BUTTON (button), NULL);

  return button->priv->icon_name;
}

/**
 * btk_tool_button_set_icon_widget:
 * @button: a #BtkToolButton
 * @icon_widget: (allow-none): the widget used as icon, or %NULL
 *
 * Sets @icon as the widget used as icon on @button. If @icon_widget is
 * %NULL the icon is determined by the "stock_id" property. If the
 * "stock_id" property is also %NULL, @button will not have an icon.
 * 
 * Since: 2.4
 **/
void
btk_tool_button_set_icon_widget (BtkToolButton *button,
				 BtkWidget     *icon_widget)
{
  g_return_if_fail (BTK_IS_TOOL_BUTTON (button));
  g_return_if_fail (icon_widget == NULL || BTK_IS_WIDGET (icon_widget));

  if (icon_widget != button->priv->icon_widget)
    {
      if (button->priv->icon_widget)
	{
	  if (button->priv->icon_widget->parent)
	    btk_container_remove (BTK_CONTAINER (button->priv->icon_widget->parent),
				    button->priv->icon_widget);

	  g_object_unref (button->priv->icon_widget);
	}
      
      if (icon_widget)
	g_object_ref_sink (icon_widget);

      button->priv->icon_widget = icon_widget;
      button->priv->contents_invalid = TRUE;
      
      g_object_notify (B_OBJECT (button), "icon-widget");
    }
}

/**
 * btk_tool_button_set_label_widget:
 * @button: a #BtkToolButton
 * @label_widget: (allow-none): the widget used as label, or %NULL
 *
 * Sets @label_widget as the widget that will be used as the label
 * for @button. If @label_widget is %NULL the "label" property is used
 * as label. If "label" is also %NULL, the label in the stock item
 * determined by the "stock_id" property is used as label. If
 * "stock_id" is also %NULL, @button does not have a label.
 * 
 * Since: 2.4
 **/
void
btk_tool_button_set_label_widget (BtkToolButton *button,
				  BtkWidget     *label_widget)
{
  g_return_if_fail (BTK_IS_TOOL_BUTTON (button));
  g_return_if_fail (label_widget == NULL || BTK_IS_WIDGET (label_widget));

  if (label_widget != button->priv->label_widget)
    {
      if (button->priv->label_widget)
	{
	  if (button->priv->label_widget->parent)
	    btk_container_remove (BTK_CONTAINER (button->priv->label_widget->parent),
		    	          button->priv->label_widget);
	  
	  g_object_unref (button->priv->label_widget);
	}
      
      if (label_widget)
	g_object_ref_sink (label_widget);

      button->priv->label_widget = label_widget;
      button->priv->contents_invalid = TRUE;
      
      g_object_notify (B_OBJECT (button), "label-widget");
    }
}

/**
 * btk_tool_button_get_label_widget:
 * @button: a #BtkToolButton
 *
 * Returns the widget used as label on @button.
 * See btk_tool_button_set_label_widget().
 *
 * Return value: (transfer none): The widget used as label
 *     on @button, or %NULL.
 *
 * Since: 2.4
 **/
BtkWidget *
btk_tool_button_get_label_widget (BtkToolButton *button)
{
  g_return_val_if_fail (BTK_IS_TOOL_BUTTON (button), NULL);

  return button->priv->label_widget;
}

/**
 * btk_tool_button_get_icon_widget:
 * @button: a #BtkToolButton
 *
 * Return the widget used as icon widget on @button.
 * See btk_tool_button_set_icon_widget().
 *
 * Return value: (transfer none): The widget used as icon
 *     on @button, or %NULL.
 *
 * Since: 2.4
 **/
BtkWidget *
btk_tool_button_get_icon_widget (BtkToolButton *button)
{
  g_return_val_if_fail (BTK_IS_TOOL_BUTTON (button), NULL);

  return button->priv->icon_widget;
}

BtkWidget *
_btk_tool_button_get_button (BtkToolButton *button)
{
  g_return_val_if_fail (BTK_IS_TOOL_BUTTON (button), NULL);

  return button->priv->button;
}


#define __BTK_TOOL_BUTTON_C__
#include "btkaliasdef.c"
