/* btkseparatortoolitem.c
 *
 * Copyright (C) 2002 Anders Carlsson <andersca@bunny.org>
 * Copyright (C) 2002 James Henstridge <james@daa.com.au>
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
#include "btkseparatormenuitem.h"
#include "btkseparatortoolitem.h"
#include "btkintl.h"
#include "btktoolbar.h"
#include "btkprivate.h"
#include "btkalias.h"

#define MENU_ID "btk-separator-tool-item-menu-id"

enum {
  PROP_0,
  PROP_DRAW
};

static bboolean btk_separator_tool_item_create_menu_proxy (BtkToolItem               *item);
static void     btk_separator_tool_item_set_property      (BObject                   *object,
							   buint                      prop_id,
							   const BValue              *value,
							   BParamSpec                *pspec);
static void     btk_separator_tool_item_get_property       (BObject                   *object,
							   buint                      prop_id,
							   BValue                    *value,
							   BParamSpec                *pspec);
static void     btk_separator_tool_item_size_request      (BtkWidget                 *widget,
							   BtkRequisition            *requisition);
static bboolean btk_separator_tool_item_expose            (BtkWidget                 *widget,
							   BdkEventExpose            *event);
static void     btk_separator_tool_item_add               (BtkContainer              *container,
							   BtkWidget                 *child);
static bint     get_space_size                            (BtkToolItem               *tool_item);



#define BTK_SEPARATOR_TOOL_ITEM_GET_PRIVATE(obj)(B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_SEPARATOR_TOOL_ITEM, BtkSeparatorToolItemPrivate))

struct _BtkSeparatorToolItemPrivate
{
  buint draw : 1;
};

G_DEFINE_TYPE (BtkSeparatorToolItem, btk_separator_tool_item, BTK_TYPE_TOOL_ITEM)

static bint
get_space_size (BtkToolItem *tool_item)
{
  bint space_size = _btk_toolbar_get_default_space_size();
  BtkWidget *parent = BTK_WIDGET (tool_item)->parent;
  
  if (BTK_IS_TOOLBAR (parent))
    {
      btk_widget_style_get (parent,
			    "space-size", &space_size,
			    NULL);
    }
  
  return space_size;
}

static void
btk_separator_tool_item_class_init (BtkSeparatorToolItemClass *class)
{
  BObjectClass *object_class;
  BtkContainerClass *container_class;
  BtkToolItemClass *toolitem_class;
  BtkWidgetClass *widget_class;
  
  object_class = (BObjectClass *)class;
  container_class = (BtkContainerClass *)class;
  toolitem_class = (BtkToolItemClass *)class;
  widget_class = (BtkWidgetClass *)class;

  object_class->set_property = btk_separator_tool_item_set_property;
  object_class->get_property = btk_separator_tool_item_get_property;
  widget_class->size_request = btk_separator_tool_item_size_request;
  widget_class->expose_event = btk_separator_tool_item_expose;
  toolitem_class->create_menu_proxy = btk_separator_tool_item_create_menu_proxy;
  
  container_class->add = btk_separator_tool_item_add;
  
  g_object_class_install_property (object_class,
				   PROP_DRAW,
				   g_param_spec_boolean ("draw",
							 P_("Draw"),
							 P_("Whether the separator is drawn, or just blank"),
							 TRUE,
							 BTK_PARAM_READWRITE));
  
  g_type_class_add_private (object_class, sizeof (BtkSeparatorToolItemPrivate));
}

static void
btk_separator_tool_item_init (BtkSeparatorToolItem      *separator_item)
{
  separator_item->priv = BTK_SEPARATOR_TOOL_ITEM_GET_PRIVATE (separator_item);
  separator_item->priv->draw = TRUE;
}

static void
btk_separator_tool_item_add (BtkContainer *container,
			     BtkWidget    *child)
{
  g_warning ("attempt to add a child to an BtkSeparatorToolItem");
}

static bboolean
btk_separator_tool_item_create_menu_proxy (BtkToolItem *item)
{
  BtkWidget *menu_item = NULL;
  
  menu_item = btk_separator_menu_item_new();
  
  btk_tool_item_set_proxy_menu_item (item, MENU_ID, menu_item);
  
  return TRUE;
}

static void
btk_separator_tool_item_set_property (BObject      *object,
				      buint         prop_id,
				      const BValue *value,
				      BParamSpec   *pspec)
{
  BtkSeparatorToolItem *item = BTK_SEPARATOR_TOOL_ITEM (object);
  
  switch (prop_id)
    {
    case PROP_DRAW:
      btk_separator_tool_item_set_draw (item, b_value_get_boolean (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_separator_tool_item_get_property (BObject      *object,
				      buint         prop_id,
				      BValue       *value,
				      BParamSpec   *pspec)
{
  BtkSeparatorToolItem *item = BTK_SEPARATOR_TOOL_ITEM (object);
  
  switch (prop_id)
    {
    case PROP_DRAW:
      b_value_set_boolean (value, btk_separator_tool_item_get_draw (item));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_separator_tool_item_size_request (BtkWidget      *widget,
				      BtkRequisition *requisition)
{
  BtkToolItem *item = BTK_TOOL_ITEM (widget);
  BtkOrientation orientation = btk_tool_item_get_orientation (item);
  
  if (orientation == BTK_ORIENTATION_HORIZONTAL)
    {
      requisition->width = get_space_size (item);
      requisition->height = 1;
    }
  else
    {
      requisition->height = get_space_size (item);
      requisition->width = 1;
    }
}

static bboolean
btk_separator_tool_item_expose (BtkWidget      *widget,
				BdkEventExpose *event)
{
  BtkToolbar *toolbar = NULL;
  BtkSeparatorToolItemPrivate *priv =
      BTK_SEPARATOR_TOOL_ITEM_GET_PRIVATE (widget);

  if (priv->draw)
    {
      if (BTK_IS_TOOLBAR (widget->parent))
	toolbar = BTK_TOOLBAR (widget->parent);

      _btk_toolbar_paint_space_line (widget, toolbar,
				     &(event->area), &widget->allocation);
    }
  
  return FALSE;
}

/**
 * btk_separator_tool_item_new:
 * 
 * Create a new #BtkSeparatorToolItem
 * 
 * Return value: the new #BtkSeparatorToolItem
 * 
 * Since: 2.4
 */
BtkToolItem *
btk_separator_tool_item_new (void)
{
  BtkToolItem *self;
  
  self = g_object_new (BTK_TYPE_SEPARATOR_TOOL_ITEM,
		       NULL);
  
  return self;
}

/**
 * btk_separator_tool_item_get_draw:
 * @item: a #BtkSeparatorToolItem 
 * 
 * Returns whether @item is drawn as a line, or just blank. 
 * See btk_separator_tool_item_set_draw().
 * 
 * Return value: %TRUE if @item is drawn as a line, or just blank.
 * 
 * Since: 2.4
 */
bboolean
btk_separator_tool_item_get_draw (BtkSeparatorToolItem *item)
{
  g_return_val_if_fail (BTK_IS_SEPARATOR_TOOL_ITEM (item), FALSE);
  
  return item->priv->draw;
}

/**
 * btk_separator_tool_item_set_draw:
 * @item: a #BtkSeparatorToolItem
 * @draw: whether @item is drawn as a vertical line
 * 
 * Whether @item is drawn as a vertical line, or just blank.
 * Setting this to %FALSE along with btk_tool_item_set_expand() is useful
 * to create an item that forces following items to the end of the toolbar.
 * 
 * Since: 2.4
 */
void
btk_separator_tool_item_set_draw (BtkSeparatorToolItem *item,
				  bboolean              draw)
{
  g_return_if_fail (BTK_IS_SEPARATOR_TOOL_ITEM (item));

  draw = draw != FALSE;

  if (draw != item->priv->draw)
    {
      item->priv->draw = draw;

      btk_widget_queue_draw (BTK_WIDGET (item));

      g_object_notify (B_OBJECT (item), "draw");
    }
}

#define __BTK_SEPARATOR_TOOL_ITEM_C__
#include "btkaliasdef.c"
