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

#include <bdk/bdkkeysyms.h>

#undef BTK_DISABLE_DEPRECATED
#define __BTK_LIST_ITEM_C__

#include "btkbindings.h"
#include "btklabel.h"
#include "btklistitem.h"
#include "btklist.h"
#include "btkmarshalers.h"
#include "btksignal.h"
#include "btkintl.h"

#include "btkalias.h"


enum
{
  TOGGLE_FOCUS_ROW,
  SELECT_ALL,
  UNSELECT_ALL,
  UNDO_SELECTION,
  START_SELECTION,
  END_SELECTION,
  TOGGLE_ADD_MODE,
  EXTEND_SELECTION,
  SCROLL_VERTICAL,
  SCROLL_HORIZONTAL,
  LAST_SIGNAL
};

static void btk_list_item_class_init        (BtkListItemClass *klass);
static void btk_list_item_init              (BtkListItem      *list_item);
static void btk_list_item_realize           (BtkWidget        *widget);
static void btk_list_item_size_request      (BtkWidget        *widget,
					     BtkRequisition   *requisition);
static void btk_list_item_size_allocate     (BtkWidget        *widget,
					     BtkAllocation    *allocation);
static void btk_list_item_style_set         (BtkWidget        *widget,
					     BtkStyle         *previous_style);
static bint btk_list_item_button_press      (BtkWidget        *widget,
					     BdkEventButton   *event);
static bint btk_list_item_expose            (BtkWidget        *widget,
					     BdkEventExpose   *event);
static void btk_real_list_item_select       (BtkItem          *item);
static void btk_real_list_item_deselect     (BtkItem          *item);
static void btk_real_list_item_toggle       (BtkItem          *item);


static BtkItemClass *parent_class = NULL;
static buint list_item_signals[LAST_SIGNAL] = {0};


BtkType
btk_list_item_get_type (void)
{
  static BtkType list_item_type = 0;

  if (!list_item_type)
    {
      static const BtkTypeInfo list_item_info =
      {
	"BtkListItem",
	sizeof (BtkListItem),
	sizeof (BtkListItemClass),
	(BtkClassInitFunc) btk_list_item_class_init,
	(BtkObjectInitFunc) btk_list_item_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (BtkClassInitFunc) NULL,
      };

      I_("BtkListItem");
      list_item_type = btk_type_unique (btk_item_get_type (), &list_item_info);
    }

  return list_item_type;
}

static void
btk_list_item_class_init (BtkListItemClass *class)
{
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;
  BtkItemClass *item_class;
  BtkBindingSet *binding_set;

  object_class = (BtkObjectClass*) class;
  widget_class = (BtkWidgetClass*) class;
  item_class = (BtkItemClass*) class;

  parent_class = btk_type_class (btk_item_get_type ());

  widget_class->realize = btk_list_item_realize;
  widget_class->size_request = btk_list_item_size_request;
  widget_class->size_allocate = btk_list_item_size_allocate;
  widget_class->style_set = btk_list_item_style_set;
  widget_class->button_press_event = btk_list_item_button_press;
  widget_class->expose_event = btk_list_item_expose;

  item_class->select = btk_real_list_item_select;
  item_class->deselect = btk_real_list_item_deselect;
  item_class->toggle = btk_real_list_item_toggle;

  class->toggle_focus_row = NULL;
  class->select_all = NULL;
  class->unselect_all = NULL;
  class->undo_selection = NULL;
  class->start_selection = NULL;
  class->end_selection = NULL;
  class->extend_selection = NULL;
  class->scroll_horizontal = NULL;
  class->scroll_vertical = NULL;
  class->toggle_add_mode = NULL;

  list_item_signals[TOGGLE_FOCUS_ROW] =
    btk_signal_new (I_("toggle-focus-row"),
                    BTK_RUN_LAST | BTK_RUN_ACTION,
                    BTK_CLASS_TYPE (object_class),
                    BTK_SIGNAL_OFFSET (BtkListItemClass, toggle_focus_row),
                    _btk_marshal_VOID__VOID,
                    BTK_TYPE_NONE, 0);
  list_item_signals[SELECT_ALL] =
    btk_signal_new (I_("select-all"),
                    BTK_RUN_LAST | BTK_RUN_ACTION,
                    BTK_CLASS_TYPE (object_class),
                    BTK_SIGNAL_OFFSET (BtkListItemClass, select_all),
                    _btk_marshal_VOID__VOID,
                    BTK_TYPE_NONE, 0);
  list_item_signals[UNSELECT_ALL] =
    btk_signal_new (I_("unselect-all"),
                    BTK_RUN_LAST | BTK_RUN_ACTION,
                    BTK_CLASS_TYPE (object_class),
                    BTK_SIGNAL_OFFSET (BtkListItemClass, unselect_all),
                    _btk_marshal_VOID__VOID,
                    BTK_TYPE_NONE, 0);
  list_item_signals[UNDO_SELECTION] =
    btk_signal_new (I_("undo-selection"),
		    BTK_RUN_LAST | BTK_RUN_ACTION,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkListItemClass, undo_selection),
		    _btk_marshal_VOID__VOID,
		    BTK_TYPE_NONE, 0);
  list_item_signals[START_SELECTION] =
    btk_signal_new (I_("start-selection"),
		    BTK_RUN_LAST | BTK_RUN_ACTION,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkListItemClass, start_selection),
		    _btk_marshal_VOID__VOID,
		    BTK_TYPE_NONE, 0);
  list_item_signals[END_SELECTION] =
    btk_signal_new (I_("end-selection"),
		    BTK_RUN_LAST | BTK_RUN_ACTION,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkListItemClass, end_selection),
		    _btk_marshal_VOID__VOID,
		    BTK_TYPE_NONE, 0);
  list_item_signals[TOGGLE_ADD_MODE] =
    btk_signal_new (I_("toggle-add-mode"),
		    BTK_RUN_LAST | BTK_RUN_ACTION,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkListItemClass, toggle_add_mode),
		    _btk_marshal_VOID__VOID,
		    BTK_TYPE_NONE, 0);
  list_item_signals[EXTEND_SELECTION] =
    btk_signal_new (I_("extend-selection"),
                    BTK_RUN_LAST | BTK_RUN_ACTION,
                    BTK_CLASS_TYPE (object_class),
                    BTK_SIGNAL_OFFSET (BtkListItemClass, extend_selection),
                    _btk_marshal_VOID__ENUM_FLOAT_BOOLEAN,
                    BTK_TYPE_NONE, 3,
		    BTK_TYPE_SCROLL_TYPE, BTK_TYPE_FLOAT, BTK_TYPE_BOOL);
  list_item_signals[SCROLL_VERTICAL] =
    btk_signal_new (I_("scroll-vertical"),
                    BTK_RUN_LAST | BTK_RUN_ACTION,
                    BTK_CLASS_TYPE (object_class),
                    BTK_SIGNAL_OFFSET (BtkListItemClass, scroll_vertical),
                    _btk_marshal_VOID__ENUM_FLOAT,
                    BTK_TYPE_NONE, 2, BTK_TYPE_SCROLL_TYPE, BTK_TYPE_FLOAT);
  list_item_signals[SCROLL_HORIZONTAL] =
    btk_signal_new (I_("scroll-horizontal"),
                    BTK_RUN_LAST | BTK_RUN_ACTION,
                    BTK_CLASS_TYPE (object_class),
                    BTK_SIGNAL_OFFSET (BtkListItemClass, scroll_horizontal),
                    _btk_marshal_VOID__ENUM_FLOAT,
                    BTK_TYPE_NONE, 2, BTK_TYPE_SCROLL_TYPE, BTK_TYPE_FLOAT);

  binding_set = btk_binding_set_by_class (class);
  btk_binding_entry_add_signal (binding_set, BDK_Up, 0,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_BACKWARD,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Up, 0,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_BACKWARD,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_Down, 0,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_FORWARD,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Down, 0,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_FORWARD,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_Page_Up, 0,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_PAGE_BACKWARD,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Page_Up, 0,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_PAGE_BACKWARD,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_Page_Down, 0,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_PAGE_FORWARD,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Page_Down, 0,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_PAGE_FORWARD,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_Home, BDK_CONTROL_MASK,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Home, BDK_CONTROL_MASK,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_End, BDK_CONTROL_MASK,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 1.0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_End, BDK_CONTROL_MASK,
				"scroll-vertical", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 1.0);
  
  btk_binding_entry_add_signal (binding_set, BDK_Up, BDK_SHIFT_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_BACKWARD,
				BTK_TYPE_FLOAT, 0.0, BTK_TYPE_BOOL, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Up, BDK_SHIFT_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_BACKWARD,
				BTK_TYPE_FLOAT, 0.0, BTK_TYPE_BOOL, TRUE);
  
  btk_binding_entry_add_signal (binding_set, BDK_Down, BDK_SHIFT_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_FORWARD,
				BTK_TYPE_FLOAT, 0.0, BTK_TYPE_BOOL, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Down, BDK_SHIFT_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_FORWARD,
				BTK_TYPE_FLOAT, 0.0, BTK_TYPE_BOOL, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_Page_Up, BDK_SHIFT_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_PAGE_BACKWARD,
				BTK_TYPE_FLOAT, 0.0, BTK_TYPE_BOOL, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Page_Up, BDK_SHIFT_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_PAGE_BACKWARD,
				BTK_TYPE_FLOAT, 0.0, BTK_TYPE_BOOL, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_Page_Down, BDK_SHIFT_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_PAGE_FORWARD,
				BTK_TYPE_FLOAT, 0.0, BTK_TYPE_BOOL, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Page_Down, BDK_SHIFT_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_PAGE_FORWARD,
				BTK_TYPE_FLOAT, 0.0, BTK_TYPE_BOOL, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_Home,
				BDK_SHIFT_MASK | BDK_CONTROL_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 0.0, BTK_TYPE_BOOL, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Home,
				BDK_SHIFT_MASK | BDK_CONTROL_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 0.0, BTK_TYPE_BOOL, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_End,
				BDK_SHIFT_MASK | BDK_CONTROL_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 1.0, BTK_TYPE_BOOL, TRUE);
  btk_binding_entry_add_signal (binding_set, BDK_KP_End,
				BDK_SHIFT_MASK | BDK_CONTROL_MASK,
				"extend-selection", 3,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 1.0, BTK_TYPE_BOOL, TRUE);

  
  btk_binding_entry_add_signal (binding_set, BDK_Left, 0,
				"scroll-horizontal", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_BACKWARD,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Left, 0,
				"scroll-horizontal", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_BACKWARD,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_Right, 0,
				"scroll-horizontal", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_FORWARD,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Right, 0,
				"scroll-horizontal", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_STEP_FORWARD,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_Home, 0,
				"scroll-horizontal", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Home, 0,
				"scroll-horizontal", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 0.0);
  btk_binding_entry_add_signal (binding_set, BDK_End, 0,
				"scroll-horizontal", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 1.0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_End, 0,
				"scroll-horizontal", 2,
				BTK_TYPE_ENUM, BTK_SCROLL_JUMP,
				BTK_TYPE_FLOAT, 1.0);
  
  btk_binding_entry_add_signal (binding_set, BDK_Escape, 0,
				"undo-selection", 0);
  btk_binding_entry_add_signal (binding_set, BDK_space, 0,
				"toggle-focus-row", 0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Space, 0,
				"toggle-focus-row", 0);
  btk_binding_entry_add_signal (binding_set, BDK_space, BDK_CONTROL_MASK,
				"toggle-add-mode", 0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Space, BDK_CONTROL_MASK,
				"toggle-add-mode", 0);
  btk_binding_entry_add_signal (binding_set, BDK_slash, BDK_CONTROL_MASK,
				"select-all", 0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Divide, BDK_CONTROL_MASK,
				"select-all", 0);
  btk_binding_entry_add_signal (binding_set, '\\', BDK_CONTROL_MASK,
				"unselect-all", 0);
  btk_binding_entry_add_signal (binding_set, BDK_Shift_L,
				BDK_RELEASE_MASK | BDK_SHIFT_MASK,
				"end-selection", 0);
  btk_binding_entry_add_signal (binding_set, BDK_Shift_R,
				BDK_RELEASE_MASK | BDK_SHIFT_MASK,
				"end-selection", 0);
  btk_binding_entry_add_signal (binding_set, BDK_Shift_R,
				BDK_RELEASE_MASK | BDK_SHIFT_MASK |
				BDK_CONTROL_MASK,
				"end-selection", 0);
}

static void
btk_list_item_init (BtkListItem *list_item)
{
  btk_widget_set_can_focus (BTK_WIDGET (list_item), TRUE);
}

BtkWidget*
btk_list_item_new (void)
{
  return BTK_WIDGET (btk_type_new (btk_list_item_get_type ()));
}

BtkWidget*
btk_list_item_new_with_label (const bchar *label)
{
  BtkWidget *list_item;
  BtkWidget *label_widget;

  list_item = btk_list_item_new ();
  label_widget = btk_label_new (label);
  btk_misc_set_alignment (BTK_MISC (label_widget), 0.0, 0.5);
  btk_misc_set_padding (BTK_MISC (label_widget), 0, 1);
  
  btk_container_add (BTK_CONTAINER (list_item), label_widget);
  btk_widget_show (label_widget);

  return list_item;
}

void
btk_list_item_select (BtkListItem *list_item)
{
  btk_item_select (BTK_ITEM (list_item));
}

void
btk_list_item_deselect (BtkListItem *list_item)
{
  btk_item_deselect (BTK_ITEM (list_item));
}


static void
btk_list_item_realize (BtkWidget *widget)
{
  BdkWindowAttr attributes;
  bint attributes_mask;

  /*BTK_WIDGET_CLASS (parent_class)->realize (widget);*/

  g_return_if_fail (BTK_IS_LIST_ITEM (widget));

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
			   BDK_BUTTON1_MOTION_MASK |
			   BDK_POINTER_MOTION_HINT_MASK |
			   BDK_KEY_PRESS_MASK |
			   BDK_KEY_RELEASE_MASK |
			   BDK_ENTER_NOTIFY_MASK |
			   BDK_LEAVE_NOTIFY_MASK);

  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;
  widget->window = bdk_window_new (btk_widget_get_parent_window (widget), &attributes, attributes_mask);
  bdk_window_set_user_data (widget->window, widget);

  widget->style = btk_style_attach (widget->style, widget->window);
  bdk_window_set_background (widget->window, 
			     &widget->style->base[BTK_STATE_NORMAL]);
}

static void
btk_list_item_size_request (BtkWidget      *widget,
			    BtkRequisition *requisition)
{
  BtkBin *bin;
  BtkRequisition child_requisition;
  bint focus_width;
  bint focus_pad;

  g_return_if_fail (BTK_IS_LIST_ITEM (widget));
  g_return_if_fail (requisition != NULL);

  bin = BTK_BIN (widget);
  btk_widget_style_get (widget,
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			NULL);

  requisition->width = 2 * (BTK_CONTAINER (widget)->border_width +
			    widget->style->xthickness + focus_width + focus_pad - 1);
  requisition->height = 2 * (BTK_CONTAINER (widget)->border_width +
			     focus_width + focus_pad - 1);

  if (bin->child && btk_widget_get_visible (bin->child))
    {
      btk_widget_size_request (bin->child, &child_requisition);

      requisition->width += child_requisition.width;
      requisition->height += child_requisition.height;
    }
}

static void
btk_list_item_size_allocate (BtkWidget     *widget,
			     BtkAllocation *allocation)
{
  BtkBin *bin;
  BtkAllocation child_allocation;

  g_return_if_fail (BTK_IS_LIST_ITEM (widget));
  g_return_if_fail (allocation != NULL);

  widget->allocation = *allocation;
  if (btk_widget_get_realized (widget))
    bdk_window_move_resize (widget->window,
			    allocation->x, allocation->y,
			    allocation->width, allocation->height);

  bin = BTK_BIN (widget);

  if (bin->child)
    {
      child_allocation.x = (BTK_CONTAINER (widget)->border_width +
			    widget->style->xthickness);
      child_allocation.y = BTK_CONTAINER (widget)->border_width;
      child_allocation.width = allocation->width - child_allocation.x * 2;
      child_allocation.height = allocation->height - child_allocation.y * 2;

      btk_widget_size_allocate (bin->child, &child_allocation);
    }
}

static void 
btk_list_item_style_set	(BtkWidget      *widget,
			 BtkStyle       *previous_style)
{
  BtkStyle *style;

  g_return_if_fail (widget != NULL);

  if (previous_style && btk_widget_get_realized (widget))
    {
      style = btk_widget_get_style (widget);
      bdk_window_set_background (btk_widget_get_window (widget),
                                 &style->base[btk_widget_get_state (widget)]);
    }
}

static bint
btk_list_item_button_press (BtkWidget      *widget,
			    BdkEventButton *event)
{
  if (event->type == BDK_BUTTON_PRESS && !btk_widget_has_focus (widget))
    btk_widget_grab_focus (widget);

  return FALSE;
}

static bint
btk_list_item_expose (BtkWidget      *widget,
		      BdkEventExpose *event)
{
  g_return_val_if_fail (widget != NULL, FALSE);

  if (btk_widget_is_drawable (widget))
    {
      if (widget->state == BTK_STATE_NORMAL)
        {
          bdk_window_set_back_pixmap (widget->window, NULL, TRUE);
          bdk_window_clear_area (widget->window, event->area.x, event->area.y,
                                 event->area.width, event->area.height);
        }
      else
        {
          btk_paint_flat_box (widget->style, widget->window, 
                              widget->state, BTK_SHADOW_ETCHED_OUT,
                              &event->area, widget, "listitem",
                              0, 0, -1, -1);           
        }

      BTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);

      if (btk_widget_has_focus (widget))
        {
          if (BTK_IS_LIST (widget->parent) && BTK_LIST (widget->parent)->add_mode)
            btk_paint_focus (widget->style, widget->window, btk_widget_get_state (widget),
                             NULL, widget, "add-mode",
                             0, 0, widget->allocation.width, widget->allocation.height);
          else
            btk_paint_focus (widget->style, widget->window, btk_widget_get_state (widget),
                             NULL, widget, NULL,
                             0, 0, widget->allocation.width, widget->allocation.height);
        }
    }

  return FALSE;
}

static void
btk_real_list_item_select (BtkItem *item)
{
  g_return_if_fail (BTK_IS_LIST_ITEM (item));

  switch (BTK_WIDGET (item)->state)
    {
    case BTK_STATE_SELECTED:
    case BTK_STATE_INSENSITIVE:
      break;
    default:
      btk_widget_set_state (BTK_WIDGET (item), BTK_STATE_SELECTED);
      break;
    }
}

static void
btk_real_list_item_deselect (BtkItem *item)
{
  g_return_if_fail (BTK_IS_LIST_ITEM (item));

  if (BTK_WIDGET (item)->state == BTK_STATE_SELECTED)
    btk_widget_set_state (BTK_WIDGET (item), BTK_STATE_NORMAL);
}

static void
btk_real_list_item_toggle (BtkItem *item)
{
  g_return_if_fail (BTK_IS_LIST_ITEM (item));
  
  switch (BTK_WIDGET (item)->state)
    {
    case BTK_STATE_SELECTED:
      btk_widget_set_state (BTK_WIDGET (item), BTK_STATE_NORMAL);
      break;
    case BTK_STATE_INSENSITIVE:
      break;
    default:
      btk_widget_set_state (BTK_WIDGET (item), BTK_STATE_SELECTED);
      break;
    }
}

#include "btkaliasdef.c"
