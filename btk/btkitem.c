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

#include "config.h"

#undef BTK_DISABLE_DEPRECATED

#include "btkitem.h"
#include "btkmarshalers.h"
#include "btkintl.h"
#include "btkalias.h"


enum {
  SELECT,
  DESELECT,
  TOGGLE,
  LAST_SIGNAL
};


static void btk_item_realize    (BtkWidget        *widget);
static bint btk_item_enter      (BtkWidget        *widget,
				 BdkEventCrossing *event);
static bint btk_item_leave      (BtkWidget        *widget,
				 BdkEventCrossing *event);


static buint item_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_ABSTRACT_TYPE (BtkItem, btk_item, BTK_TYPE_BIN)

static void
btk_item_class_init (BtkItemClass *class)
{
  BObjectClass *object_class;
  BtkWidgetClass *widget_class;

  object_class = (BObjectClass*) class;
  widget_class = (BtkWidgetClass*) class;

  widget_class->realize = btk_item_realize;
  widget_class->enter_notify_event = btk_item_enter;
  widget_class->leave_notify_event = btk_item_leave;

  class->select = NULL;
  class->deselect = NULL;
  class->toggle = NULL;

  item_signals[SELECT] =
    g_signal_new (I_("select"),
		  B_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkItemClass, select),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
  item_signals[DESELECT] =
    g_signal_new (I_("deselect"),
		  B_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkItemClass, deselect),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
  item_signals[TOGGLE] =
    g_signal_new (I_("toggle"),
		  B_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkItemClass, toggle),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
  widget_class->activate_signal = item_signals[TOGGLE];
}

static void
btk_item_init (BtkItem *item)
{
  btk_widget_set_has_window (BTK_WIDGET (item), TRUE);
}

void
btk_item_select (BtkItem *item)
{
  g_signal_emit (item, item_signals[SELECT], 0);
}

void
btk_item_deselect (BtkItem *item)
{
  g_signal_emit (item, item_signals[DESELECT], 0);
}

void
btk_item_toggle (BtkItem *item)
{
  g_signal_emit (item, item_signals[TOGGLE], 0);
}


static void
btk_item_realize (BtkWidget *widget)
{
  BdkWindowAttr attributes;
  bint attributes_mask;

  btk_widget_set_realized (widget, TRUE);

  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  attributes.event_mask = (btk_widget_get_events (widget) |
			   BDK_EXPOSURE_MASK |
			   BDK_BUTTON_PRESS_MASK |
			   BDK_BUTTON_RELEASE_MASK |
			   BDK_ENTER_NOTIFY_MASK |
			   BDK_LEAVE_NOTIFY_MASK |
			   BDK_POINTER_MOTION_MASK);

  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;
  widget->window = bdk_window_new (btk_widget_get_parent_window (widget), &attributes, attributes_mask);
  bdk_window_set_user_data (widget->window, widget);

  widget->style = btk_style_attach (widget->style, widget->window);
  btk_style_set_background (widget->style, widget->window, BTK_STATE_NORMAL);
  bdk_window_set_back_pixmap (widget->window, NULL, TRUE);
}

static bint
btk_item_enter (BtkWidget        *widget,
		BdkEventCrossing *event)
{
  g_return_val_if_fail (BTK_IS_ITEM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  return btk_widget_event (widget->parent, (BdkEvent*) event);
}

static bint
btk_item_leave (BtkWidget        *widget,
		BdkEventCrossing *event)
{
  g_return_val_if_fail (BTK_IS_ITEM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  return btk_widget_event (widget->parent, (BdkEvent*) event);
}

#define __BTK_ITEM_C__
#include "btkaliasdef.c"
