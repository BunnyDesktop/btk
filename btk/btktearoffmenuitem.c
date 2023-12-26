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

#include "btkmenu.h"
#include "btktearoffmenuitem.h"
#include "btkintl.h"
#include "btkalias.h"

#define ARROW_SIZE 10
#define TEAR_LENGTH 5
#define BORDER_SPACING  3

static void btk_tearoff_menu_item_size_request (BtkWidget             *widget,
				                BtkRequisition        *requisition);
static gint btk_tearoff_menu_item_expose     (BtkWidget             *widget,
					      BdkEventExpose        *event);
static void btk_tearoff_menu_item_activate   (BtkMenuItem           *menu_item);
static void btk_tearoff_menu_item_parent_set (BtkWidget             *widget,
					      BtkWidget             *previous);

G_DEFINE_TYPE (BtkTearoffMenuItem, btk_tearoff_menu_item, BTK_TYPE_MENU_ITEM)

BtkWidget*
btk_tearoff_menu_item_new (void)
{
  return g_object_new (BTK_TYPE_TEAROFF_MENU_ITEM, NULL);
}

static void
btk_tearoff_menu_item_class_init (BtkTearoffMenuItemClass *klass)
{
  BtkWidgetClass *widget_class;
  BtkMenuItemClass *menu_item_class;

  widget_class = (BtkWidgetClass*) klass;
  menu_item_class = (BtkMenuItemClass*) klass;

  widget_class->expose_event = btk_tearoff_menu_item_expose;
  widget_class->size_request = btk_tearoff_menu_item_size_request;
  widget_class->parent_set = btk_tearoff_menu_item_parent_set;

  menu_item_class->activate = btk_tearoff_menu_item_activate;
}

static void
btk_tearoff_menu_item_init (BtkTearoffMenuItem *tearoff_menu_item)
{
  tearoff_menu_item->torn_off = FALSE;
}

static void
btk_tearoff_menu_item_size_request (BtkWidget      *widget,
				    BtkRequisition *requisition)
{
  requisition->width = (BTK_CONTAINER (widget)->border_width +
			widget->style->xthickness +
			BORDER_SPACING) * 2;
  requisition->height = (BTK_CONTAINER (widget)->border_width +
			 widget->style->ythickness) * 2;

  if (BTK_IS_MENU (widget->parent) && BTK_MENU (widget->parent)->torn_off)
    {
      requisition->height += ARROW_SIZE;
    }
  else
    {
      requisition->height += widget->style->ythickness + 4;
    }
}

static void
btk_tearoff_menu_item_paint (BtkWidget   *widget,
			     BdkRectangle *area)
{
  BtkMenuItem *menu_item;
  BtkShadowType shadow_type;
  gint width, height;
  gint x, y;
  gint right_max;
  BtkArrowType arrow_type;
  BtkTextDirection direction;
  
  if (btk_widget_is_drawable (widget))
    {
      menu_item = BTK_MENU_ITEM (widget);

      direction = btk_widget_get_direction (widget);

      x = widget->allocation.x + BTK_CONTAINER (menu_item)->border_width;
      y = widget->allocation.y + BTK_CONTAINER (menu_item)->border_width;
      width = widget->allocation.width - BTK_CONTAINER (menu_item)->border_width * 2;
      height = widget->allocation.height - BTK_CONTAINER (menu_item)->border_width * 2;
      right_max = x + width;

      if (widget->state == BTK_STATE_PRELIGHT)
	{
	  gint selected_shadow_type;
	  
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
      else
	bdk_window_clear_area (widget->window, area->x, area->y, area->width, area->height);

      if (BTK_IS_MENU (widget->parent) && BTK_MENU (widget->parent)->torn_off)
	{
	  gint arrow_x;

	  if (widget->state == BTK_STATE_PRELIGHT)
	    shadow_type = BTK_SHADOW_IN;
	  else
	    shadow_type = BTK_SHADOW_OUT;

	  if (menu_item->toggle_size > ARROW_SIZE)
	    {
	      if (direction == BTK_TEXT_DIR_LTR) {
		arrow_x = x + (menu_item->toggle_size - ARROW_SIZE)/2;
		arrow_type = BTK_ARROW_LEFT;
	      }
	      else {
		arrow_x = x + width - menu_item->toggle_size + (menu_item->toggle_size - ARROW_SIZE)/2; 
		arrow_type = BTK_ARROW_RIGHT;	    
	      }
	      x += menu_item->toggle_size + BORDER_SPACING;
	    }
	  else
	    {
	      if (direction == BTK_TEXT_DIR_LTR) {
		arrow_x = ARROW_SIZE / 2;
		arrow_type = BTK_ARROW_LEFT;
	      }
	      else {
		arrow_x = x + width - 2 * ARROW_SIZE + ARROW_SIZE / 2; 
		arrow_type = BTK_ARROW_RIGHT;	    
	      }
	      x += 2 * ARROW_SIZE;
	    }


	  btk_paint_arrow (widget->style, widget->window,
			   widget->state, shadow_type,
			   NULL, widget, "tearoffmenuitem",
			   arrow_type, FALSE,
			   arrow_x, y + height / 2 - 5, 
			   ARROW_SIZE, ARROW_SIZE);
	}

      while (x < right_max)
	{
	  gint x1, x2;

	  if (direction == BTK_TEXT_DIR_LTR) {
	    x1 = x;
	    x2 = MIN (x + TEAR_LENGTH, right_max);
	  }
	  else {
	    x1 = right_max - x;
	    x2 = MAX (right_max - x - TEAR_LENGTH, 0);
	  }
	  
	  btk_paint_hline (widget->style, widget->window, BTK_STATE_NORMAL,
			   NULL, widget, "tearoffmenuitem",
			   x1, x2, y + (height - widget->style->ythickness) / 2);
	  x += 2 * TEAR_LENGTH;
	}
    }
}

static gint
btk_tearoff_menu_item_expose (BtkWidget      *widget,
			    BdkEventExpose *event)
{
  btk_tearoff_menu_item_paint (widget, &event->area);

  return FALSE;
}

static void
btk_tearoff_menu_item_activate (BtkMenuItem *menu_item)
{
  if (BTK_IS_MENU (BTK_WIDGET (menu_item)->parent))
    {
      BtkMenu *menu = BTK_MENU (BTK_WIDGET (menu_item)->parent);
      
      btk_widget_queue_resize (BTK_WIDGET (menu_item));
      btk_menu_set_tearoff_state (BTK_MENU (BTK_WIDGET (menu_item)->parent),
				  !menu->torn_off);
    }
}

static void
tearoff_state_changed (BtkMenu            *menu,
		       BParamSpec         *pspec,
		       gpointer            data)
{
  BtkTearoffMenuItem *tearoff_menu_item = BTK_TEAROFF_MENU_ITEM (data);

  tearoff_menu_item->torn_off = btk_menu_get_tearoff_state (menu);
}

static void
btk_tearoff_menu_item_parent_set (BtkWidget *widget,
				  BtkWidget *previous)
{
  BtkTearoffMenuItem *tearoff_menu_item = BTK_TEAROFF_MENU_ITEM (widget);
  BtkMenu *menu = BTK_IS_MENU (widget->parent) ? BTK_MENU (widget->parent) : NULL;

  if (previous)
    g_signal_handlers_disconnect_by_func (previous, 
					  tearoff_state_changed, 
					  tearoff_menu_item);
  
  if (menu)
    {
      tearoff_menu_item->torn_off = btk_menu_get_tearoff_state (menu);
      g_signal_connect (menu, "notify::tearoff-state", 
			G_CALLBACK (tearoff_state_changed), 
			tearoff_menu_item);
    }  
}

#define __BTK_TEAROFF_MENU_ITEM_C__
#include "btkaliasdef.c"
