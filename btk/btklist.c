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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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
#include <string.h> /* memset */

#undef BTK_DISABLE_DEPRECATED
#define __BTK_LIST_C__

#include "btklist.h"
#include "btklistitem.h"
#include "btkmain.h"
#include "btksignal.h"
#include "btklabel.h"
#include "btkmarshalers.h"
#include "btkintl.h"

#include "btkalias.h"

enum {
  SELECTION_CHANGED,
  SELECT_CHILD,
  UNSELECT_CHILD,
  LAST_SIGNAL
};

enum {
  ARG_0,
  ARG_SELECTION_MODE
};

#define SCROLL_TIME  100

/*** BtkList Methods ***/
static void btk_list_class_init	     (BtkListClass   *klass);
static void btk_list_init	     (BtkList	     *list);
static void btk_list_set_arg         (BtkObject      *object,
				      BtkArg         *arg,
				      buint           arg_id);
static void btk_list_get_arg         (BtkObject      *object,
				      BtkArg         *arg,
				      buint           arg_id);
/*** BtkObject Methods ***/
static void btk_list_dispose	     (BObject	     *object);

/*** BtkWidget Methods ***/
static void btk_list_size_request    (BtkWidget	     *widget,
				      BtkRequisition *requisition);
static void btk_list_size_allocate   (BtkWidget	     *widget,
				      BtkAllocation  *allocation);
static void btk_list_realize	     (BtkWidget	     *widget);
static void btk_list_unmap	     (BtkWidget	     *widget);
static void btk_list_style_set	     (BtkWidget      *widget,
				      BtkStyle       *previous_style);
static bint btk_list_motion_notify   (BtkWidget      *widget,
				      BdkEventMotion *event);
static bint btk_list_button_press    (BtkWidget      *widget,
				      BdkEventButton *event);
static bint btk_list_button_release  (BtkWidget	     *widget,
				      BdkEventButton *event);

static bboolean btk_list_focus       (BtkWidget        *widget,
                                      BtkDirectionType  direction);

/*** BtkContainer Methods ***/
static void btk_list_add	     (BtkContainer     *container,
				      BtkWidget        *widget);
static void btk_list_remove	     (BtkContainer     *container,
				      BtkWidget        *widget);
static void btk_list_forall	     (BtkContainer     *container,
				      bboolean          include_internals,
				      BtkCallback       callback,
				      bpointer          callback_data);
static BtkType btk_list_child_type   (BtkContainer     *container);
static void btk_list_set_focus_child (BtkContainer     *container,
				      BtkWidget        *widget);

/*** BtkList Private Functions ***/
static void btk_list_move_focus_child      (BtkList       *list,
					    BtkScrollType  scroll_type,
					    bfloat         position);
static bint btk_list_horizontal_timeout    (BtkWidget     *list);
static bint btk_list_vertical_timeout      (BtkWidget     *list);
static void btk_list_remove_items_internal (BtkList       *list,
					    GList         *items,
					    bboolean       no_unref);

/*** BtkList Selection Methods ***/
static void btk_real_list_select_child	        (BtkList   *list,
						 BtkWidget *child);
static void btk_real_list_unselect_child        (BtkList   *list,
						 BtkWidget *child);

/*** BtkList Selection Functions ***/
static void btk_list_set_anchor                 (BtkList   *list,
					         bboolean   add_mode,
					         bint       anchor,
					         BtkWidget *undo_focus_child);
static void btk_list_fake_unselect_all          (BtkList   *list,
			                         BtkWidget *item);
static void btk_list_fake_toggle_row            (BtkList   *list,
					         BtkWidget *item);
static void btk_list_update_extended_selection  (BtkList   *list,
					         bint       row);
static void btk_list_reset_extended_selection   (BtkList   *list);

/*** BtkListItem Signal Functions ***/
static void btk_list_signal_drag_begin         (BtkWidget      *widget,
						BdkDragContext *context,
						BtkList        *list);
static void btk_list_signal_toggle_focus_row   (BtkListItem   *list_item,
						BtkList       *list);
static void btk_list_signal_select_all         (BtkListItem   *list_item,
						BtkList       *list);
static void btk_list_signal_unselect_all       (BtkListItem   *list_item,
						BtkList       *list);
static void btk_list_signal_undo_selection     (BtkListItem   *list_item,
						BtkList       *list);
static void btk_list_signal_start_selection    (BtkListItem   *list_item,
						BtkList       *list);
static void btk_list_signal_end_selection      (BtkListItem   *list_item,
						BtkList       *list);
static void btk_list_signal_extend_selection   (BtkListItem   *list_item,
						BtkScrollType  scroll_type,
						bfloat         position,
						bboolean       auto_start_selection,
						BtkList       *list);
static void btk_list_signal_scroll_horizontal  (BtkListItem   *list_item,
						BtkScrollType  scroll_type,
						bfloat         position,
						BtkList       *list);
static void btk_list_signal_scroll_vertical    (BtkListItem   *list_item,
						BtkScrollType  scroll_type,
						bfloat         position,
						BtkList       *list);
static void btk_list_signal_toggle_add_mode    (BtkListItem   *list_item,
						BtkList       *list);
static void btk_list_signal_item_select        (BtkListItem   *list_item,
						BtkList       *list);
static void btk_list_signal_item_deselect      (BtkListItem   *list_item,
						BtkList       *list);
static void btk_list_signal_item_toggle        (BtkListItem   *list_item,
						BtkList       *list);


static void btk_list_drag_begin (BtkWidget      *widget,
				 BdkDragContext *context);


static BtkContainerClass *parent_class = NULL;
static buint list_signals[LAST_SIGNAL] = { 0 };

static const bchar vadjustment_key[] = "btk-vadjustment";
static buint        vadjustment_key_id = 0;
static const bchar hadjustment_key[] = "btk-hadjustment";
static buint        hadjustment_key_id = 0;

BtkType
btk_list_get_type (void)
{
  static BtkType list_type = 0;

  if (!list_type)
    {
      static const BtkTypeInfo list_info =
      {
	"BtkList",
	sizeof (BtkList),
	sizeof (BtkListClass),
	(BtkClassInitFunc) btk_list_class_init,
	(BtkObjectInitFunc) btk_list_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (BtkClassInitFunc) NULL,
      };

      I_("BtkList");
      list_type = btk_type_unique (BTK_TYPE_CONTAINER, &list_info);
    }

  return list_type;
}

static void
btk_list_class_init (BtkListClass *class)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (class);
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;
  BtkContainerClass *container_class;

  object_class = (BtkObjectClass*) class;
  widget_class = (BtkWidgetClass*) class;
  container_class = (BtkContainerClass*) class;

  parent_class = btk_type_class (BTK_TYPE_CONTAINER);

  vadjustment_key_id = g_quark_from_static_string (vadjustment_key);
  hadjustment_key_id = g_quark_from_static_string (hadjustment_key);

  bobject_class->dispose = btk_list_dispose;


  object_class->set_arg = btk_list_set_arg;
  object_class->get_arg = btk_list_get_arg;

  widget_class->unmap = btk_list_unmap;
  widget_class->style_set = btk_list_style_set;
  widget_class->realize = btk_list_realize;
  widget_class->button_press_event = btk_list_button_press;
  widget_class->button_release_event = btk_list_button_release;
  widget_class->motion_notify_event = btk_list_motion_notify;
  widget_class->size_request = btk_list_size_request;
  widget_class->size_allocate = btk_list_size_allocate;
  widget_class->drag_begin = btk_list_drag_begin;
  widget_class->focus = btk_list_focus;
  
  container_class->add = btk_list_add;
  container_class->remove = btk_list_remove;
  container_class->forall = btk_list_forall;
  container_class->child_type = btk_list_child_type;
  container_class->set_focus_child = btk_list_set_focus_child;

  class->selection_changed = NULL;
  class->select_child = btk_real_list_select_child;
  class->unselect_child = btk_real_list_unselect_child;

  list_signals[SELECTION_CHANGED] =
    btk_signal_new (I_("selection-changed"),
		    BTK_RUN_FIRST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkListClass, selection_changed),
		    _btk_marshal_VOID__VOID,
		    BTK_TYPE_NONE, 0);
  list_signals[SELECT_CHILD] =
    btk_signal_new (I_("select-child"),
		    BTK_RUN_FIRST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkListClass, select_child),
		    _btk_marshal_VOID__OBJECT,
		    BTK_TYPE_NONE, 1,
		    BTK_TYPE_WIDGET);
  list_signals[UNSELECT_CHILD] =
    btk_signal_new (I_("unselect-child"),
		    BTK_RUN_FIRST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkListClass, unselect_child),
		    _btk_marshal_VOID__OBJECT,
		    BTK_TYPE_NONE, 1,
		    BTK_TYPE_WIDGET);
  
  btk_object_add_arg_type ("BtkList::selection-mode",
			   BTK_TYPE_SELECTION_MODE, 
			   BTK_ARG_READWRITE | G_PARAM_STATIC_NAME,
			   ARG_SELECTION_MODE);
}

static void
btk_list_init (BtkList *list)
{
  list->children = NULL;
  list->selection = NULL;

  list->undo_selection = NULL;
  list->undo_unselection = NULL;

  list->last_focus_child = NULL;
  list->undo_focus_child = NULL;

  list->htimer = 0;
  list->vtimer = 0;

  list->anchor = -1;
  list->drag_pos = -1;
  list->anchor_state = BTK_STATE_SELECTED;

  list->selection_mode = BTK_SELECTION_SINGLE;
  list->drag_selection = FALSE;
  list->add_mode = FALSE;
}

static void
btk_list_set_arg (BtkObject      *object,
		  BtkArg         *arg,
		  buint           arg_id)
{
  BtkList *list = BTK_LIST (object);
  
  switch (arg_id)
    {
    case ARG_SELECTION_MODE:
      btk_list_set_selection_mode (list, BTK_VALUE_ENUM (*arg));
      break;
    }
}

static void
btk_list_get_arg (BtkObject      *object,
		  BtkArg         *arg,
		  buint           arg_id)
{
  BtkList *list = BTK_LIST (object);
  
  switch (arg_id)
    {
    case ARG_SELECTION_MODE: 
      BTK_VALUE_ENUM (*arg) = list->selection_mode; 
      break;
    default:
      arg->type = BTK_TYPE_INVALID;
      break;
    }
}

BtkWidget*
btk_list_new (void)
{
  return BTK_WIDGET (btk_type_new (BTK_TYPE_LIST));
}


/* Private BtkObject Methods :
 * 
 * btk_list_dispose
 */
static void
btk_list_dispose (BObject *object)
{
  btk_list_clear_items (BTK_LIST (object), 0, -1);

  B_OBJECT_CLASS (parent_class)->dispose (object);
}


/* Private BtkWidget Methods :
 * 
 * btk_list_size_request
 * btk_list_size_allocate
 * btk_list_realize
 * btk_list_unmap
 * btk_list_motion_notify
 * btk_list_button_press
 * btk_list_button_release
 */
static void
btk_list_size_request (BtkWidget      *widget,
		       BtkRequisition *requisition)
{
  BtkList *list = BTK_LIST (widget);
  BtkWidget *child;
  GList *children;

  requisition->width = 0;
  requisition->height = 0;

  children = list->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (btk_widget_get_visible (child))
	{
	  BtkRequisition child_requisition;
	  
	  btk_widget_size_request (child, &child_requisition);

	  requisition->width = MAX (requisition->width,
				    child_requisition.width);
	  requisition->height += child_requisition.height;
	}
    }

  requisition->width += BTK_CONTAINER (list)->border_width * 2;
  requisition->height += BTK_CONTAINER (list)->border_width * 2;

  requisition->width = MAX (requisition->width, 1);
  requisition->height = MAX (requisition->height, 1);
}

static void
btk_list_size_allocate (BtkWidget     *widget,
			BtkAllocation *allocation)
{
  BtkList *list = BTK_LIST (widget);
  BtkWidget *child;
  BtkAllocation child_allocation;
  GList *children;

  widget->allocation = *allocation;
  if (btk_widget_get_realized (widget))
    bdk_window_move_resize (widget->window,
			    allocation->x, allocation->y,
			    allocation->width, allocation->height);

  if (list->children)
    {
      child_allocation.x = BTK_CONTAINER (list)->border_width;
      child_allocation.y = BTK_CONTAINER (list)->border_width;
      child_allocation.width = MAX (1, (bint)allocation->width -
				    child_allocation.x * 2);

      children = list->children;

      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if (btk_widget_get_visible (child))
	    {
	      BtkRequisition child_requisition;
	      btk_widget_get_child_requisition (child, &child_requisition);
	      
	      child_allocation.height = child_requisition.height;

	      btk_widget_size_allocate (child, &child_allocation);

	      child_allocation.y += child_allocation.height;
	    }
	}
    }
}

static void
btk_list_realize (BtkWidget *widget)
{
  BdkWindowAttr attributes;
  bint attributes_mask;

  btk_widget_set_realized (widget, TRUE);

  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  attributes.event_mask = btk_widget_get_events (widget) | BDK_EXPOSURE_MASK;

  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;

  widget->window = bdk_window_new (btk_widget_get_parent_window (widget),
				   &attributes, attributes_mask);
  bdk_window_set_user_data (widget->window, widget);

  widget->style = btk_style_attach (widget->style, widget->window);
  bdk_window_set_background (widget->window, 
			     &widget->style->base[BTK_STATE_NORMAL]);
}

static bboolean
list_has_grab (BtkList *list)
{
  return (BTK_WIDGET_HAS_GRAB (list) &&
	  bdk_display_pointer_is_grabbed (btk_widget_get_display (BTK_WIDGET (list))));
	  
}

static void
btk_list_unmap (BtkWidget *widget)
{
  BtkList *list = BTK_LIST (widget);

  if (!btk_widget_get_mapped (widget))
    return;

  btk_widget_set_mapped (widget, FALSE);

  if (list_has_grab (list))
    {
      btk_list_end_drag_selection (list);

      if (list->anchor != -1 && list->selection_mode == BTK_SELECTION_MULTIPLE)
	btk_list_end_selection (list);
    }

  bdk_window_hide (widget->window);
}

static bint
btk_list_motion_notify (BtkWidget      *widget,
			BdkEventMotion *event)
{
  BtkList *list = BTK_LIST (widget);
  BtkWidget *item = NULL;
  BtkAdjustment *adj;
  BtkContainer *container;
  GList *work;
  bint x;
  bint y;
  bint row = -1;
  bint focus_row = 0;
  bint length = 0;

  if (!list->drag_selection || !list->children)
    return FALSE;

  container = BTK_CONTAINER (widget);

  if (event->is_hint || event->window != widget->window)
    bdk_window_get_pointer (widget->window, &x, &y, NULL);
  else
    {
      x = event->x;
      y = event->y;
    }

  adj = btk_object_get_data_by_id (BTK_OBJECT (list), hadjustment_key_id);

  /* horizontal autoscrolling */
  if (adj && widget->allocation.width > adj->page_size &&
      (x < adj->value || x >= adj->value + adj->page_size))
    {
      if (list->htimer == 0)
	{
	  list->htimer = bdk_threads_add_timeout
	    (SCROLL_TIME, (GSourceFunc) btk_list_horizontal_timeout, widget);
	  
	  if (!((x < adj->value && adj->value <= 0) ||
		(x > adj->value + adj->page_size &&
		 adj->value >= adj->upper - adj->page_size)))
	    {
	      bdouble value;

	      if (x < adj->value)
		value = adj->value + (x - adj->value) / 2 - 1;
	      else
		value = adj->value + 1 + (x - adj->value - adj->page_size) / 2;

	      btk_adjustment_set_value (adj,
					CLAMP (value, 0.0,
					       adj->upper - adj->page_size));
	    }
	}
      else
	return FALSE;
    }

  
  /* vertical autoscrolling */
  for (work = list->children; work; length++, work = work->next)
    {
      if (row < 0)
	{
	  item = BTK_WIDGET (work->data);
	  if (item->allocation.y > y || 
	      (item->allocation.y <= y &&
	       item->allocation.y + item->allocation.height > y))
	    row = length;
	}

      if (work->data == container->focus_child)
	focus_row = length;
    }
  
  if (row < 0)
    row = length - 1;

  if (list->vtimer != 0)
    return FALSE;

  if (!((y < 0 && focus_row == 0) ||
	(y > widget->allocation.height && focus_row >= length - 1)))
    list->vtimer = bdk_threads_add_timeout (SCROLL_TIME,
				  (GSourceFunc) btk_list_vertical_timeout,
				  list);

  if (row != focus_row)
    btk_widget_grab_focus (item);

  switch (list->selection_mode)
    {
    case BTK_SELECTION_BROWSE:
      btk_list_select_child (list, item);
      break;
    case BTK_SELECTION_MULTIPLE:
      btk_list_update_extended_selection (list, row);
      break;
    default:
      break;
    }

  return FALSE;
}

static bint
btk_list_button_press (BtkWidget      *widget,
		       BdkEventButton *event)
{
  BtkList *list = BTK_LIST (widget);
  BtkWidget *item;

  if (event->button != 1)
    return FALSE;

  item = btk_get_event_widget ((BdkEvent*) event);

  while (item && !BTK_IS_LIST_ITEM (item))
    item = item->parent;

  if (item && (item->parent == widget))
    {
      bint last_focus_row;
      bint focus_row;

      if (event->type == BDK_BUTTON_PRESS)
	{
	  btk_grab_add (widget);
	  list->drag_selection = TRUE;
	}
      else if (list_has_grab (list))
	btk_list_end_drag_selection (list);
	  
      if (!btk_widget_has_focus(item))
	btk_widget_grab_focus (item);

      if (list->add_mode)
	{
	  list->add_mode = FALSE;
	  btk_widget_queue_draw (item);
	}
      
      switch (list->selection_mode)
	{
	case BTK_SELECTION_SINGLE:
	  if (event->type != BDK_BUTTON_PRESS)
	    btk_list_select_child (list, item);
	  else
	    list->undo_focus_child = item;
	  break;
	  
	case BTK_SELECTION_BROWSE:
	  break;

	case BTK_SELECTION_MULTIPLE:
	  focus_row = g_list_index (list->children, item);

	  if (list->last_focus_child)
	    last_focus_row = g_list_index (list->children,
					   list->last_focus_child);
	  else
	    {
	      last_focus_row = focus_row;
	      list->last_focus_child = item;
	    }

	  if (event->type != BDK_BUTTON_PRESS)
	    {
	      if (list->anchor >= 0)
		{
		  btk_list_update_extended_selection (list, focus_row);
		  btk_list_end_selection (list);
		}
	      btk_list_select_child (list, item);
	      break;
	    }
	      
	  if (event->state & BDK_CONTROL_MASK)
	    {
	      if (event->state & BDK_SHIFT_MASK)
		{
		  if (list->anchor < 0)
		    {
		      g_list_free (list->undo_selection);
		      g_list_free (list->undo_unselection);
		      list->undo_selection = NULL;
		      list->undo_unselection = NULL;

		      list->anchor = last_focus_row;
		      list->drag_pos = last_focus_row;
		      list->undo_focus_child = list->last_focus_child;
		    }
		  btk_list_update_extended_selection (list, focus_row);
		}
	      else
		{
		  if (list->anchor < 0)
		    btk_list_set_anchor (list, TRUE,
					 focus_row, list->last_focus_child);
		  else
		    btk_list_update_extended_selection (list, focus_row);
		}
	      break;
	    }

	  if (event->state & BDK_SHIFT_MASK)
	    {
	      btk_list_set_anchor (list, FALSE,
				   last_focus_row, list->last_focus_child);
	      btk_list_update_extended_selection (list, focus_row);
	      break;
	    }

	  if (list->anchor < 0)
	    btk_list_set_anchor (list, FALSE, focus_row,
				 list->last_focus_child);
	  else
	    btk_list_update_extended_selection (list, focus_row);
	  break;
	  
	default:
	  break;
	}

      return TRUE;
    }

  return FALSE;
}

static bint
btk_list_button_release (BtkWidget	*widget,
			 BdkEventButton *event)
{
  BtkList *list = BTK_LIST (widget);
  BtkWidget *item;

  /* we don't handle button 2 and 3 */
  if (event->button != 1)
    return FALSE;

  if (list->drag_selection)
    {
      btk_list_end_drag_selection (list);

      switch (list->selection_mode)
	{
	case BTK_SELECTION_MULTIPLE:
 	  if (!(event->state & BDK_SHIFT_MASK))
	    btk_list_end_selection (list);
	  break;

	case BTK_SELECTION_SINGLE:

	  item = btk_get_event_widget ((BdkEvent*) event);
  
	  while (item && !BTK_IS_LIST_ITEM (item))
	    item = item->parent;
	  
	  if (item && item->parent == widget)
	    {
	      if (list->undo_focus_child == item)
		btk_list_toggle_row (list, item);
	    }
	  list->undo_focus_child = NULL;
	  break;

	default:
	  break;
	}

      return TRUE;
    }
  
  return FALSE;
}

static void 
btk_list_style_set	(BtkWidget      *widget,
			 BtkStyle       *previous_style)
{
  BtkStyle *style;

  if (previous_style && btk_widget_get_realized (widget))
    {
      style = btk_widget_get_style (widget);
      bdk_window_set_background (btk_widget_get_window (widget),
                                 &style->base[btk_widget_get_state (widget)]);
    }
}

/* BtkContainer Methods :
 * btk_list_add
 * btk_list_remove
 * btk_list_forall
 * btk_list_child_type
 * btk_list_set_focus_child
 * btk_list_focus
 */
static void
btk_list_add (BtkContainer *container,
	      BtkWidget	   *widget)
{
  GList *item_list;

  g_return_if_fail (BTK_IS_LIST_ITEM (widget));

  item_list = g_list_alloc ();
  item_list->data = widget;
  
  btk_list_append_items (BTK_LIST (container), item_list);
}

static void
btk_list_remove (BtkContainer *container,
		 BtkWidget    *widget)
{
  GList *item_list;

  g_return_if_fail (container == BTK_CONTAINER (widget->parent));
  
  item_list = g_list_alloc ();
  item_list->data = widget;
  
  btk_list_remove_items (BTK_LIST (container), item_list);
  
  g_list_free (item_list);
}

static void
btk_list_forall (BtkContainer  *container,
		 bboolean       include_internals,
		 BtkCallback	callback,
		 bpointer	callback_data)
{
  BtkList *list = BTK_LIST (container);
  BtkWidget *child;
  GList *children;

  children = list->children;

  while (children)
    {
      child = children->data;
      children = children->next;

      (* callback) (child, callback_data);
    }
}

static BtkType
btk_list_child_type (BtkContainer *container)
{
  return BTK_TYPE_LIST_ITEM;
}

static void
btk_list_set_focus_child (BtkContainer *container,
			  BtkWidget    *child)
{
  BtkList *list;

  g_return_if_fail (BTK_IS_LIST (container));
 
  if (child)
    g_return_if_fail (BTK_IS_WIDGET (child));

  list = BTK_LIST (container);

  if (child != container->focus_child)
    {
      if (container->focus_child)
	{
	  list->last_focus_child = container->focus_child;
	  g_object_unref (container->focus_child);
	}
      container->focus_child = child;
      if (container->focus_child)
        g_object_ref (container->focus_child);
    }

  /* check for v adjustment */
  if (container->focus_child)
    {
      BtkAdjustment *adjustment;

      adjustment = btk_object_get_data_by_id (BTK_OBJECT (container),
					      vadjustment_key_id);
      if (adjustment)
        btk_adjustment_clamp_page (adjustment,
                                   container->focus_child->allocation.y,
                                   (container->focus_child->allocation.y +
                                    container->focus_child->allocation.height));
      switch (list->selection_mode)
	{
	case BTK_SELECTION_BROWSE:
	  btk_list_select_child (list, child);
	  break;
	case BTK_SELECTION_MULTIPLE:
	  if (!list->last_focus_child && !list->add_mode)
	    {
	      list->undo_focus_child = list->last_focus_child;
	      btk_list_unselect_all (list);
	      btk_list_select_child (list, child);
	    }
	  break;
	default:
	  break;
	}
    }
}

static bboolean
btk_list_focus (BtkWidget        *widget,
		BtkDirectionType  direction)
{
  bint return_val = FALSE;
  BtkContainer *container;

  container = BTK_CONTAINER (widget);
  
  if (container->focus_child == NULL ||
      !btk_widget_has_focus (container->focus_child))
    {
      if (BTK_LIST (container)->last_focus_child)
	btk_container_set_focus_child
	  (container, BTK_LIST (container)->last_focus_child);

      if (BTK_WIDGET_CLASS (parent_class)->focus)
	return_val = BTK_WIDGET_CLASS (parent_class)->focus (widget,
                                                             direction);
    }

  if (!return_val)
    {
      BtkList *list;

      list =  BTK_LIST (container);
      if (list->selection_mode == BTK_SELECTION_MULTIPLE && list->anchor >= 0)
	btk_list_end_selection (list);

      if (container->focus_child)
	list->last_focus_child = container->focus_child;
    }

  return return_val;
}


/* Public BtkList Methods :
 *
 * btk_list_insert_items
 * btk_list_append_items
 * btk_list_prepend_items
 * btk_list_remove_items
 * btk_list_remove_items_no_unref
 * btk_list_clear_items
 *
 * btk_list_child_position
 */
void
btk_list_insert_items (BtkList *list,
		       GList   *items,
		       bint	position)
{
  BtkWidget *widget;
  GList *tmp_list;
  GList *last;
  bint nchildren;

  g_return_if_fail (BTK_IS_LIST (list));

  if (!items)
    return;

  btk_list_end_drag_selection (list);
  if (list->selection_mode == BTK_SELECTION_MULTIPLE && list->anchor >= 0)
    btk_list_end_selection (list);

  tmp_list = items;
  while (tmp_list)
    {
      widget = tmp_list->data;
      tmp_list = tmp_list->next;

      btk_widget_set_parent (widget, BTK_WIDGET (list));
      btk_signal_connect (BTK_OBJECT (widget), "drag-begin",
			  G_CALLBACK (btk_list_signal_drag_begin),
			  list);
      btk_signal_connect (BTK_OBJECT (widget), "toggle-focus-row",
			  G_CALLBACK (btk_list_signal_toggle_focus_row),
			  list);
      btk_signal_connect (BTK_OBJECT (widget), "select-all",
			  G_CALLBACK (btk_list_signal_select_all),
			  list);
      btk_signal_connect (BTK_OBJECT (widget), "unselect-all",
			  G_CALLBACK (btk_list_signal_unselect_all),
			  list);
      btk_signal_connect (BTK_OBJECT (widget), "undo-selection",
			  G_CALLBACK (btk_list_signal_undo_selection),
			  list);
      btk_signal_connect (BTK_OBJECT (widget), "start-selection",
			  G_CALLBACK (btk_list_signal_start_selection),
			  list);
      btk_signal_connect (BTK_OBJECT (widget), "end-selection",
			  G_CALLBACK (btk_list_signal_end_selection),
			  list);
      btk_signal_connect (BTK_OBJECT (widget), "extend-selection",
			  G_CALLBACK (btk_list_signal_extend_selection),
			  list);
      btk_signal_connect (BTK_OBJECT (widget), "scroll-horizontal",
			  G_CALLBACK (btk_list_signal_scroll_horizontal),
			  list);
      btk_signal_connect (BTK_OBJECT (widget), "scroll-vertical",
			  G_CALLBACK (btk_list_signal_scroll_vertical),
			  list);
      btk_signal_connect (BTK_OBJECT (widget), "toggle-add-mode",
			  G_CALLBACK (btk_list_signal_toggle_add_mode),
			  list);
      btk_signal_connect (BTK_OBJECT (widget), "select",
			  G_CALLBACK (btk_list_signal_item_select),
			  list);
      btk_signal_connect (BTK_OBJECT (widget), "deselect",
			  G_CALLBACK (btk_list_signal_item_deselect),
			  list);
      btk_signal_connect (BTK_OBJECT (widget), "toggle",
			  G_CALLBACK (btk_list_signal_item_toggle),
			  list);
    }


  nchildren = g_list_length (list->children);
  if ((position < 0) || (position > nchildren))
    position = nchildren;

  if (position == nchildren)
    {
      if (list->children)
	{
	  tmp_list = g_list_last (list->children);
	  tmp_list->next = items;
	  items->prev = tmp_list;
	}
      else
	{
	  list->children = items;
	}
    }
  else
    {
      tmp_list = g_list_nth (list->children, position);
      last = g_list_last (items);

      if (tmp_list->prev)
	tmp_list->prev->next = items;
      last->next = tmp_list;
      items->prev = tmp_list->prev;
      tmp_list->prev = last;

      if (tmp_list == list->children)
	list->children = items;
    }
  
  if (list->children && !list->selection &&
      (list->selection_mode == BTK_SELECTION_BROWSE))
    {
      widget = list->children->data;
      btk_list_select_child (list, widget);
    }
}

void
btk_list_append_items (BtkList *list,
		       GList   *items)
{
  g_return_if_fail (BTK_IS_LIST (list));

  btk_list_insert_items (list, items, -1);
}

void
btk_list_prepend_items (BtkList *list,
			GList	*items)
{
  g_return_if_fail (BTK_IS_LIST (list));

  btk_list_insert_items (list, items, 0);
}

void
btk_list_remove_items (BtkList	*list,
		       GList	*items)
{
  btk_list_remove_items_internal (list, items, FALSE);
}

void
btk_list_remove_items_no_unref (BtkList	 *list,
				GList	 *items)
{
  btk_list_remove_items_internal (list, items, TRUE);
}

void
btk_list_clear_items (BtkList *list,
		      bint     start,
		      bint     end)
{
  BtkContainer *container;
  BtkWidget *widget;
  BtkWidget *new_focus_child = NULL;
  GList *start_list;
  GList *end_list;
  GList *tmp_list;
  buint nchildren;
  bboolean grab_focus = FALSE;

  g_return_if_fail (BTK_IS_LIST (list));

  nchildren = g_list_length (list->children);

  if (nchildren == 0)
    return;

  if ((end < 0) || (end > nchildren))
    end = nchildren;

  if (start >= end)
    return;

  container = BTK_CONTAINER (list);

  btk_list_end_drag_selection (list);
  if (list->selection_mode == BTK_SELECTION_MULTIPLE)
    {
      if (list->anchor >= 0)
	btk_list_end_selection (list);

      btk_list_reset_extended_selection (list);
    }

  start_list = g_list_nth (list->children, start);
  end_list = g_list_nth (list->children, end);

  if (start_list->prev)
    start_list->prev->next = end_list;
  if (end_list && end_list->prev)
    end_list->prev->next = NULL;
  if (end_list)
    end_list->prev = start_list->prev;
  if (start_list == list->children)
    list->children = end_list;

  if (container->focus_child)
    {
      if (g_list_find (start_list, container->focus_child))
	{
	  if (start_list->prev)
	    new_focus_child = start_list->prev->data;
	  else if (list->children)
	    new_focus_child = list->children->data;

	  if (btk_widget_has_focus (container->focus_child))
	    grab_focus = TRUE;
	}
    }

  tmp_list = start_list;
  while (tmp_list)
    {
      widget = tmp_list->data;
      tmp_list = tmp_list->next;

      g_object_ref (widget);

      if (widget->state == BTK_STATE_SELECTED)
	btk_list_unselect_child (list, widget);

      btk_signal_disconnect_by_data (BTK_OBJECT (widget), (bpointer) list);
      btk_widget_unparent (widget);
      
      if (widget == list->undo_focus_child)
	list->undo_focus_child = NULL;
      if (widget == list->last_focus_child)
	list->last_focus_child = NULL;

      g_object_unref (widget);
    }

  g_list_free (start_list);

  if (new_focus_child)
    {
      if (grab_focus)
	btk_widget_grab_focus (new_focus_child);
      else if (container->focus_child)
	btk_container_set_focus_child (container, new_focus_child);

      if ((list->selection_mode == BTK_SELECTION_BROWSE ||
	   list->selection_mode == BTK_SELECTION_MULTIPLE) && !list->selection)
	{
	  list->last_focus_child = new_focus_child; 
	  btk_list_select_child (list, new_focus_child);
	}
    }

  if (btk_widget_get_visible (BTK_WIDGET (list)))
    btk_widget_queue_resize (BTK_WIDGET (list));
}

bint
btk_list_child_position (BtkList   *list,
			 BtkWidget *child)
{
  GList *children;
  bint pos;

  g_return_val_if_fail (BTK_IS_LIST (list), -1);
  g_return_val_if_fail (child != NULL, -1);

  pos = 0;
  children = list->children;

  while (children)
    {
      if (child == BTK_WIDGET (children->data))
	return pos;

      pos += 1;
      children = children->next;
    }

  return -1;
}


/* Private BtkList Insert/Remove Item Functions:
 *
 * btk_list_remove_items_internal
 */
static void
btk_list_remove_items_internal (BtkList	 *list,
				GList	 *items,
				bboolean  no_unref)
{
  BtkWidget *widget;
  BtkWidget *new_focus_child;
  BtkWidget *old_focus_child;
  BtkContainer *container;
  GList *tmp_list;
  GList *work;
  bboolean grab_focus = FALSE;
  
  g_return_if_fail (BTK_IS_LIST (list));

  if (!items)
    return;
  
  container = BTK_CONTAINER (list);

  btk_list_end_drag_selection (list);
  if (list->selection_mode == BTK_SELECTION_MULTIPLE)
    {
      if (list->anchor >= 0)
	btk_list_end_selection (list);

      btk_list_reset_extended_selection (list);
    }

  tmp_list = items;
  while (tmp_list)
    {
      widget = tmp_list->data;
      tmp_list = tmp_list->next;
      
      if (widget->state == BTK_STATE_SELECTED)
	btk_list_unselect_child (list, widget);
    }

  if (container->focus_child)
    {
      old_focus_child = new_focus_child = container->focus_child;
      if (btk_widget_has_focus (container->focus_child))
	grab_focus = TRUE;
    }
  else
    old_focus_child = new_focus_child = list->last_focus_child;

  tmp_list = items;
  while (tmp_list)
    {
      widget = tmp_list->data;
      tmp_list = tmp_list->next;

      g_object_ref (widget);
      if (no_unref)
	g_object_ref (widget);

      if (widget == new_focus_child) 
	{
	  work = g_list_find (list->children, widget);

	  if (work)
	    {
	      if (work->next)
		new_focus_child = work->next->data;
	      else if (list->children != work && work->prev)
		new_focus_child = work->prev->data;
	      else
		new_focus_child = NULL;
	    }
	}

      btk_signal_disconnect_by_data (BTK_OBJECT (widget), (bpointer) list);
      list->children = g_list_remove (list->children, widget);
      btk_widget_unparent (widget);

      if (widget == list->undo_focus_child)
	list->undo_focus_child = NULL;
      if (widget == list->last_focus_child)
	list->last_focus_child = NULL;

      g_object_unref (widget);
    }
  
  if (new_focus_child && new_focus_child != old_focus_child)
    {
      if (grab_focus)
	btk_widget_grab_focus (new_focus_child);
      else if (container->focus_child)
	btk_container_set_focus_child (container, new_focus_child);

      if (list->selection_mode == BTK_SELECTION_BROWSE && !list->selection)
	{
	  list->last_focus_child = new_focus_child; 
	  btk_list_select_child (list, new_focus_child);
	}
    }

  if (btk_widget_get_visible (BTK_WIDGET (list)))
    btk_widget_queue_resize (BTK_WIDGET (list));
}


/* Public BtkList Selection Methods :
 *
 * btk_list_set_selection_mode
 * btk_list_select_item
 * btk_list_unselect_item
 * btk_list_select_child
 * btk_list_unselect_child
 * btk_list_select_all
 * btk_list_unselect_all
 * btk_list_extend_selection
 * btk_list_end_drag_selection
 * btk_list_start_selection
 * btk_list_end_selection
 * btk_list_toggle_row
 * btk_list_toggle_focus_row
 * btk_list_toggle_add_mode
 * btk_list_undo_selection
 */
void
btk_list_set_selection_mode (BtkList	      *list,
			     BtkSelectionMode  mode)
{
  g_return_if_fail (BTK_IS_LIST (list));

  if (list->selection_mode == mode)
    return;

  list->selection_mode = mode;

  switch (mode)
    {
    case BTK_SELECTION_SINGLE:
    case BTK_SELECTION_BROWSE:
      btk_list_unselect_all (list);
      break;
    default:
      break;
    }
}

void
btk_list_select_item (BtkList *list,
		      bint     item)
{
  GList *tmp_list;

  g_return_if_fail (BTK_IS_LIST (list));

  tmp_list = g_list_nth (list->children, item);
  if (tmp_list)
    btk_list_select_child (list, BTK_WIDGET (tmp_list->data));
}

void
btk_list_unselect_item (BtkList *list,
			bint	 item)
{
  GList *tmp_list;

  g_return_if_fail (BTK_IS_LIST (list));

  tmp_list = g_list_nth (list->children, item);
  if (tmp_list)
    btk_list_unselect_child (list, BTK_WIDGET (tmp_list->data));
}

void
btk_list_select_child (BtkList	 *list,
		       BtkWidget *child)
{
  btk_signal_emit (BTK_OBJECT (list), list_signals[SELECT_CHILD], child);
}

void
btk_list_unselect_child (BtkList   *list,
			 BtkWidget *child)
{
  btk_signal_emit (BTK_OBJECT (list), list_signals[UNSELECT_CHILD], child);
}

void
btk_list_select_all (BtkList *list)
{
  BtkContainer *container;
 
  g_return_if_fail (BTK_IS_LIST (list));

  if (!list->children)
    return;
  
  if (list_has_grab (list))
    btk_list_end_drag_selection (list);

  if (list->selection_mode == BTK_SELECTION_MULTIPLE && list->anchor >= 0)
    btk_list_end_selection (list);

  container = BTK_CONTAINER (list);

  switch (list->selection_mode)
    {
    case BTK_SELECTION_BROWSE:
      if (container->focus_child)
	{
	  btk_list_select_child (list, container->focus_child);
	  return;
	}
      break;
    case BTK_SELECTION_MULTIPLE:
      g_list_free (list->undo_selection);
      g_list_free (list->undo_unselection);
      list->undo_selection = NULL;
      list->undo_unselection = NULL;

      if (list->children &&
	  btk_widget_get_state (list->children->data) != BTK_STATE_SELECTED)
	btk_list_fake_toggle_row (list, BTK_WIDGET (list->children->data));

      list->anchor_state =  BTK_STATE_SELECTED;
      list->anchor = 0;
      list->drag_pos = 0;
      list->undo_focus_child = container->focus_child;
      btk_list_update_extended_selection (list, g_list_length(list->children));
      btk_list_end_selection (list);
      return;
    default:
      break;
    }
}

void
btk_list_unselect_all (BtkList *list)
{
  BtkContainer *container;
  BtkWidget *item;
  GList *work;
 
  g_return_if_fail (BTK_IS_LIST (list));

  if (!list->children)
    return;

  if (list_has_grab (list))
    btk_list_end_drag_selection (list);

  if (list->selection_mode == BTK_SELECTION_MULTIPLE && list->anchor >= 0)
    btk_list_end_selection (list);

  container = BTK_CONTAINER (list);

  switch (list->selection_mode)
    {
    case BTK_SELECTION_BROWSE:
      if (container->focus_child)
	{
	  btk_list_select_child (list, container->focus_child);
	  return;
	}
      break;
    case BTK_SELECTION_MULTIPLE:
      btk_list_reset_extended_selection (list);
      break;
    default:
      break;
    }

  work = list->selection;

  while (work)
    {
      item = work->data;
      work = work->next;
      btk_list_unselect_child (list, item);
    }
}

void
btk_list_extend_selection (BtkList       *list,
			   BtkScrollType  scroll_type,
			   bfloat         position,
			   bboolean       auto_start_selection)
{
  BtkContainer *container;

  g_return_if_fail (BTK_IS_LIST (list));

  if (list_has_grab (list) ||
      list->selection_mode != BTK_SELECTION_MULTIPLE)
    return;

  container = BTK_CONTAINER (list);

  if (auto_start_selection)
    {
      bint focus_row;

      focus_row = g_list_index (list->children, container->focus_child);
      btk_list_set_anchor (list, list->add_mode, focus_row,
			   container->focus_child);
    }
  else if (list->anchor < 0)
    return;

  btk_list_move_focus_child (list, scroll_type, position);
  btk_list_update_extended_selection 
    (list, g_list_index (list->children, container->focus_child));
}

void
btk_list_end_drag_selection (BtkList *list)
{
  g_return_if_fail (BTK_IS_LIST (list));

  list->drag_selection = FALSE;
  if (BTK_WIDGET_HAS_GRAB (list))
    btk_grab_remove (BTK_WIDGET (list));

  if (list->htimer)
    {
      g_source_remove (list->htimer);
      list->htimer = 0;
    }
  if (list->vtimer)
    {
      g_source_remove (list->vtimer);
      list->vtimer = 0;
    }
}

void
btk_list_start_selection (BtkList *list)
{
  BtkContainer *container;
  bint focus_row;

  g_return_if_fail (BTK_IS_LIST (list));

  if (list_has_grab (list))
    return;

  container = BTK_CONTAINER (list);

  if ((focus_row = g_list_index (list->selection, container->focus_child))
      >= 0)
    btk_list_set_anchor (list, list->add_mode,
			 focus_row, container->focus_child);
}

void
btk_list_end_selection (BtkList *list)
{
  bint i;
  bint e;
  bboolean top_down;
  GList *work;
  BtkWidget *item;
  bint item_index;

  g_return_if_fail (BTK_IS_LIST (list));

  if (list_has_grab (list) || list->anchor < 0)
    return;

  i = MIN (list->anchor, list->drag_pos);
  e = MAX (list->anchor, list->drag_pos);

  top_down = (list->anchor < list->drag_pos);

  list->anchor = -1;
  list->drag_pos = -1;
  
  if (list->undo_selection)
    {
      work = list->selection;
      list->selection = list->undo_selection;
      list->undo_selection = work;
      work = list->selection;
      while (work)
	{
	  item = work->data;
	  work = work->next;
	  item_index = g_list_index (list->children, item);
	  if (item_index < i || item_index > e)
	    {
	      btk_widget_set_state (item, BTK_STATE_SELECTED);
	      btk_list_unselect_child (list, item);
	      list->undo_selection = g_list_prepend (list->undo_selection,
						     item);
	    }
	}
    }    

  if (top_down)
    {
      for (work = g_list_nth (list->children, i); i <= e;
	   i++, work = work->next)
	{
	  item = work->data;
	  if (g_list_find (list->selection, item))
	    {
	      if (item->state == BTK_STATE_NORMAL)
		{
		  btk_widget_set_state (item, BTK_STATE_SELECTED);
		  btk_list_unselect_child (list, item);
		  list->undo_selection = g_list_prepend (list->undo_selection,
							 item);
		}
	    }
	  else if (item->state == BTK_STATE_SELECTED)
	    {
	      btk_widget_set_state (item, BTK_STATE_NORMAL);
	      list->undo_unselection = g_list_prepend (list->undo_unselection,
						       item);
	    }
	}
    }
  else
    {
      for (work = g_list_nth (list->children, e); i <= e;
	   e--, work = work->prev)
	{
	  item = work->data;
	  if (g_list_find (list->selection, item))
	    {
	      if (item->state == BTK_STATE_NORMAL)
		{
		  btk_widget_set_state (item, BTK_STATE_SELECTED);
		  btk_list_unselect_child (list, item);
		  list->undo_selection = g_list_prepend (list->undo_selection,
							 item);
		}
	    }
	  else if (item->state == BTK_STATE_SELECTED)
	    {
	      btk_widget_set_state (item, BTK_STATE_NORMAL);
	      list->undo_unselection = g_list_prepend (list->undo_unselection,
						       item);
	    }
	}
    }

  for (work = g_list_reverse (list->undo_unselection); work; work = work->next)
    btk_list_select_child (list, BTK_WIDGET (work->data));


}

void
btk_list_toggle_row (BtkList   *list,
		     BtkWidget *item)
{
  g_return_if_fail (BTK_IS_LIST (list));
  g_return_if_fail (BTK_IS_LIST_ITEM (item));

  switch (list->selection_mode)
    {
    case BTK_SELECTION_MULTIPLE:
    case BTK_SELECTION_SINGLE:
      if (item->state == BTK_STATE_SELECTED)
	{
	  btk_list_unselect_child (list, item);
	  return;
	}
    case BTK_SELECTION_BROWSE:
      btk_list_select_child (list, item);
      break;
    }
}

void
btk_list_toggle_focus_row (BtkList *list)
{
  BtkContainer *container;
  bint focus_row;

  g_return_if_fail (list != 0);
  g_return_if_fail (BTK_IS_LIST (list));

  container = BTK_CONTAINER (list);

  if (list_has_grab (list) || !container->focus_child)
    return;

  switch (list->selection_mode)
    {
    case  BTK_SELECTION_SINGLE:
      btk_list_toggle_row (list, container->focus_child);
      break;
    case BTK_SELECTION_MULTIPLE:
      if ((focus_row = g_list_index (list->children, container->focus_child))
	  < 0)
	return;

      g_list_free (list->undo_selection);
      g_list_free (list->undo_unselection);
      list->undo_selection = NULL;
      list->undo_unselection = NULL;

      list->anchor = focus_row;
      list->drag_pos = focus_row;
      list->undo_focus_child = container->focus_child;

      if (list->add_mode)
	btk_list_fake_toggle_row (list, container->focus_child);
      else
	btk_list_fake_unselect_all (list, container->focus_child);
      
      btk_list_end_selection (list);
      break;
    default:
      break;
    }
}

void
btk_list_toggle_add_mode (BtkList *list)
{
  BtkContainer *container;

  g_return_if_fail (list != 0);
  g_return_if_fail (BTK_IS_LIST (list));
  
  if (list_has_grab (list) ||
      list->selection_mode != BTK_SELECTION_MULTIPLE)
    return;
  
  container = BTK_CONTAINER (list);

  if (list->add_mode)
    {
      list->add_mode = FALSE;
      list->anchor_state = BTK_STATE_SELECTED;
    }
  else
    list->add_mode = TRUE;
  
  if (container->focus_child)
    btk_widget_queue_draw (container->focus_child);
}

void
btk_list_undo_selection (BtkList *list)
{
  GList *work;

  g_return_if_fail (BTK_IS_LIST (list));

  if (list->selection_mode != BTK_SELECTION_MULTIPLE ||
      list_has_grab (list))
    return;
  
  if (list->anchor >= 0)
    btk_list_end_selection (list);

  if (!(list->undo_selection || list->undo_unselection))
    {
      btk_list_unselect_all (list);
      return;
    }

  for (work = list->undo_selection; work; work = work->next)
    btk_list_select_child (list, BTK_WIDGET (work->data));

  for (work = list->undo_unselection; work; work = work->next)
    btk_list_unselect_child (list, BTK_WIDGET (work->data));

  if (list->undo_focus_child)
    {
      BtkContainer *container;

      container = BTK_CONTAINER (list);

      if (container->focus_child &&
	  btk_widget_has_focus (container->focus_child))
	btk_widget_grab_focus (list->undo_focus_child);
      else
	btk_container_set_focus_child (container, list->undo_focus_child);
    }

  list->undo_focus_child = NULL;
 
  g_list_free (list->undo_selection);
  g_list_free (list->undo_unselection);
  list->undo_selection = NULL;
  list->undo_unselection = NULL;
}


/* Private BtkList Selection Methods :
 *
 * btk_real_list_select_child
 * btk_real_list_unselect_child
 */
static void
btk_real_list_select_child (BtkList   *list,
			    BtkWidget *child)
{
  g_return_if_fail (BTK_IS_LIST (list));
  g_return_if_fail (BTK_IS_LIST_ITEM (child));

  switch (child->state)
    {
    case BTK_STATE_SELECTED:
    case BTK_STATE_INSENSITIVE:
      break;
    default:
      btk_list_item_select (BTK_LIST_ITEM (child));
      break;
    }
}

static void
btk_real_list_unselect_child (BtkList	*list,
			      BtkWidget *child)
{
  g_return_if_fail (BTK_IS_LIST (list));
  g_return_if_fail (BTK_IS_LIST_ITEM (child));

  if (child->state == BTK_STATE_SELECTED)
    btk_list_item_deselect (BTK_LIST_ITEM (child));
}


/* Private BtkList Selection Functions :
 *
 * btk_list_set_anchor
 * btk_list_fake_unselect_all
 * btk_list_fake_toggle_row
 * btk_list_update_extended_selection
 * btk_list_reset_extended_selection
 */
static void
btk_list_set_anchor (BtkList   *list,
		     bboolean   add_mode,
		     bint       anchor,
		     BtkWidget *undo_focus_child)
{
  GList *work;

  g_return_if_fail (BTK_IS_LIST (list));
  
  if (list->selection_mode != BTK_SELECTION_MULTIPLE || list->anchor >= 0)
    return;

  g_list_free (list->undo_selection);
  g_list_free (list->undo_unselection);
  list->undo_selection = NULL;
  list->undo_unselection = NULL;

  if ((work = g_list_nth (list->children, anchor)))
    {
      if (add_mode)
	btk_list_fake_toggle_row (list, BTK_WIDGET (work->data));
      else
	{
	  btk_list_fake_unselect_all (list, BTK_WIDGET (work->data));
	  list->anchor_state = BTK_STATE_SELECTED;
	}
    }

  list->anchor = anchor;
  list->drag_pos = anchor;
  list->undo_focus_child = undo_focus_child;
}

static void
btk_list_fake_unselect_all (BtkList   *list,
			    BtkWidget *item)
{
  GList *work;

  if (item && item->state == BTK_STATE_NORMAL)
    btk_widget_set_state (item, BTK_STATE_SELECTED);

  list->undo_selection = list->selection;
  list->selection = NULL;
  
  for (work = list->undo_selection; work; work = work->next)
    if (work->data != item)
      btk_widget_set_state (BTK_WIDGET (work->data), BTK_STATE_NORMAL);
}

static void
btk_list_fake_toggle_row (BtkList   *list,
			  BtkWidget *item)
{
  if (!item)
    return;
  
  if (item->state == BTK_STATE_NORMAL)
    {
      list->anchor_state = BTK_STATE_SELECTED;
      btk_widget_set_state (item, BTK_STATE_SELECTED);
    }
  else
    {
      list->anchor_state = BTK_STATE_NORMAL;
      btk_widget_set_state (item, BTK_STATE_NORMAL);
    }
}

static void
btk_list_update_extended_selection (BtkList *list,
				    bint     row)
{
  bint i;
  GList *work;
  bint s1 = -1;
  bint s2 = -1;
  bint e1 = -1;
  bint e2 = -1;
  bint length;

  if (row < 0)
    row = 0;

  length = g_list_length (list->children);
  if (row >= length)
    row = length - 1;

  if (list->selection_mode != BTK_SELECTION_MULTIPLE || !list->anchor < 0)
    return;

  /* extending downwards */
  if (row > list->drag_pos && list->anchor <= list->drag_pos)
    {
      s2 = list->drag_pos + 1;
      e2 = row;
    }
  /* extending upwards */
  else if (row < list->drag_pos && list->anchor >= list->drag_pos)
    {
      s2 = row;
      e2 = list->drag_pos - 1;
    }
  else if (row < list->drag_pos && list->anchor < list->drag_pos)
    {
      e1 = list->drag_pos;
      /* row and drag_pos on different sides of anchor :
	 take back the selection between anchor and drag_pos,
         select between anchor and row */
      if (row < list->anchor)
	{
	  s1 = list->anchor + 1;
	  s2 = row;
	  e2 = list->anchor - 1;
	}
      /* take back the selection between anchor and drag_pos */
      else
	s1 = row + 1;
    }
  else if (row > list->drag_pos && list->anchor > list->drag_pos)
    {
      s1 = list->drag_pos;
      /* row and drag_pos on different sides of anchor :
	 take back the selection between anchor and drag_pos,
         select between anchor and row */
      if (row > list->anchor)
	{
	  e1 = list->anchor - 1;
	  s2 = list->anchor + 1;
	  e2 = row;
	}
      /* take back the selection between anchor and drag_pos */
      else
	e1 = row - 1;
    }

  list->drag_pos = row;

  /* restore the elements between s1 and e1 */
  if (s1 >= 0)
    {
      for (i = s1, work = g_list_nth (list->children, i); i <= e1;
	   i++, work = work->next)
	{
	  if (g_list_find (list->selection, work->data))
            btk_widget_set_state (BTK_WIDGET (work->data), BTK_STATE_SELECTED);
          else
            btk_widget_set_state (BTK_WIDGET (work->data), BTK_STATE_NORMAL);
	}
    }

  /* extend the selection between s2 and e2 */
  if (s2 >= 0)
    {
      for (i = s2, work = g_list_nth (list->children, i); i <= e2;
	   i++, work = work->next)
	if (BTK_WIDGET (work->data)->state != list->anchor_state)
	  btk_widget_set_state (BTK_WIDGET (work->data), list->anchor_state);
    }
}

static void
btk_list_reset_extended_selection (BtkList *list)
{ 
  g_return_if_fail (list != 0);
  g_return_if_fail (BTK_IS_LIST (list));

  g_list_free (list->undo_selection);
  g_list_free (list->undo_unselection);
  list->undo_selection = NULL;
  list->undo_unselection = NULL;

  list->anchor = -1;
  list->drag_pos = -1;
  list->undo_focus_child = BTK_CONTAINER (list)->focus_child;
}

/* Public BtkList Scroll Methods :
 *
 * btk_list_scroll_horizontal
 * btk_list_scroll_vertical
 */
void
btk_list_scroll_horizontal (BtkList       *list,
			    BtkScrollType  scroll_type,
			    bfloat         position)
{
  BtkAdjustment *adj;

  g_return_if_fail (list != 0);
  g_return_if_fail (BTK_IS_LIST (list));

  if (list_has_grab (list))
    return;

  if (!(adj =
	btk_object_get_data_by_id (BTK_OBJECT (list), hadjustment_key_id)))
    return;

  switch (scroll_type)
    {
    case BTK_SCROLL_STEP_UP:
    case BTK_SCROLL_STEP_BACKWARD:
      adj->value = CLAMP (adj->value - adj->step_increment, adj->lower,
			  adj->upper - adj->page_size);
      break;
    case BTK_SCROLL_STEP_DOWN:
    case BTK_SCROLL_STEP_FORWARD:
      adj->value = CLAMP (adj->value + adj->step_increment, adj->lower,
			  adj->upper - adj->page_size);
      break;
    case BTK_SCROLL_PAGE_UP:
    case BTK_SCROLL_PAGE_BACKWARD:
      adj->value = CLAMP (adj->value - adj->page_increment, adj->lower,
			  adj->upper - adj->page_size);
      break;
    case BTK_SCROLL_PAGE_DOWN:
    case BTK_SCROLL_PAGE_FORWARD:
      adj->value = CLAMP (adj->value + adj->page_increment, adj->lower,
			  adj->upper - adj->page_size);
      break;
    case BTK_SCROLL_JUMP:
      adj->value = CLAMP (adj->lower + (adj->upper - adj->lower) * position,
			  adj->lower, adj->upper - adj->page_size);
      break;
    default:
      break;
    }
  btk_adjustment_value_changed (adj);
}

void
btk_list_scroll_vertical (BtkList       *list,
			  BtkScrollType  scroll_type,
			  bfloat         position)
{
  g_return_if_fail (BTK_IS_LIST (list));

  if (list_has_grab (list))
    return;

  if (list->selection_mode == BTK_SELECTION_MULTIPLE)
    {
      BtkContainer *container;

      if (list->anchor >= 0)
	return;

      container = BTK_CONTAINER (list);
      list->undo_focus_child = container->focus_child;
      btk_list_move_focus_child (list, scroll_type, position);
      if (container->focus_child != list->undo_focus_child && !list->add_mode)
	{
	  btk_list_unselect_all (list);
	  btk_list_select_child (list, container->focus_child);
	}
    }
  else
    btk_list_move_focus_child (list, scroll_type, position);
}


/* Private BtkList Scroll/Focus Functions :
 *
 * btk_list_move_focus_child
 * btk_list_horizontal_timeout
 * btk_list_vertical_timeout
 */
static void
btk_list_move_focus_child (BtkList       *list,
			   BtkScrollType  scroll_type,
			   bfloat         position)
{
  BtkContainer *container;
  GList *work;
  BtkWidget *item;
  BtkAdjustment *adj;
  bint new_value;

  g_return_if_fail (list != 0);
  g_return_if_fail (BTK_IS_LIST (list));

  container = BTK_CONTAINER (list);

  if (container->focus_child)
    work = g_list_find (list->children, container->focus_child);
  else
    work = list->children;

  if (!work)
    return;

  switch (scroll_type)
    {
    case BTK_SCROLL_STEP_BACKWARD:
      work = work->prev;
      if (work)
	btk_widget_grab_focus (BTK_WIDGET (work->data));
      break;
    case BTK_SCROLL_STEP_FORWARD:
      work = work->next;
      if (work)
	btk_widget_grab_focus (BTK_WIDGET (work->data));
      break;
    case BTK_SCROLL_PAGE_BACKWARD:
      if (!work->prev)
	return;

      item = work->data;
      adj = btk_object_get_data_by_id (BTK_OBJECT (list), vadjustment_key_id);

      if (adj)
	{
	  bboolean correct = FALSE;

	  new_value = adj->value;

	  if (item->allocation.y <= adj->value)
	    {
	      new_value = MAX (item->allocation.y + item->allocation.height
			       - adj->page_size, adj->lower);
	      correct = TRUE;
	    }

	  if (item->allocation.y > new_value)
	    for (; work; work = work->prev)
	      {
		item = BTK_WIDGET (work->data);
		if (item->allocation.y <= new_value &&
		    item->allocation.y + item->allocation.height > new_value)
		  break;
	      }
	  else
	    for (; work; work = work->next)
	      {
		item = BTK_WIDGET (work->data);
		if (item->allocation.y <= new_value &&
		    item->allocation.y + item->allocation.height > new_value)
		  break;
	      }

	  if (correct && work && work->next && item->allocation.y < new_value)
	    item = work->next->data;
	}
      else
	item = list->children->data;
	  
      btk_widget_grab_focus (item);
      break;
    case BTK_SCROLL_PAGE_FORWARD:
      if (!work->next)
	return;

      item = work->data;
      adj = btk_object_get_data_by_id (BTK_OBJECT (list), vadjustment_key_id);

      if (adj)
	{
	  bboolean correct = FALSE;

	  new_value = adj->value;

	  if (item->allocation.y + item->allocation.height >=
	      adj->value + adj->page_size)
	    {
	      new_value = item->allocation.y;
	      correct = TRUE;
	    }

	  new_value = MIN (new_value + adj->page_size, adj->upper);

	  if (item->allocation.y > new_value)
	    for (; work; work = work->prev)
	      {
		item = BTK_WIDGET (work->data);
		if (item->allocation.y <= new_value &&
		    item->allocation.y + item->allocation.height > new_value)
		  break;
	      }
	  else
	    for (; work; work = work->next)
	      {
		item = BTK_WIDGET (work->data);
		if (item->allocation.y <= new_value &&
		    item->allocation.y + item->allocation.height > new_value)
		  break;
	      }

	  if (correct && work && work->prev &&
	      item->allocation.y + item->allocation.height - 1 > new_value)
	    item = work->prev->data;
	}
      else
	item = g_list_last (work)->data;
	  
      btk_widget_grab_focus (item);
      break;
    case BTK_SCROLL_JUMP:
      new_value = BTK_WIDGET(list)->allocation.height * CLAMP (position, 0, 1);

      for (item = NULL, work = list->children; work; work =work->next)
	{
	  item = BTK_WIDGET (work->data);
	  if (item->allocation.y <= new_value &&
	      item->allocation.y + item->allocation.height > new_value)
	    break;
	}

      btk_widget_grab_focus (item);
      break;
    default:
      break;
    }
}

static void
do_fake_motion (BtkWidget *list)
{
  BdkEvent *event = bdk_event_new (BDK_MOTION_NOTIFY);

  event->motion.send_event = TRUE;

  btk_list_motion_notify (list, (BdkEventMotion *)event);
  bdk_event_free (event);
}

static bint
btk_list_horizontal_timeout (BtkWidget *list)
{
  BTK_LIST (list)->htimer = 0;
  do_fake_motion (list);
  
  return FALSE;
}

static bint
btk_list_vertical_timeout (BtkWidget *list)
{
  BTK_LIST (list)->vtimer = 0;
  do_fake_motion (list);

  return FALSE;
}


/* Private BtkListItem Signal Functions :
 *
 * btk_list_signal_toggle_focus_row
 * btk_list_signal_select_all
 * btk_list_signal_unselect_all
 * btk_list_signal_undo_selection
 * btk_list_signal_start_selection
 * btk_list_signal_end_selection
 * btk_list_signal_extend_selection
 * btk_list_signal_scroll_horizontal
 * btk_list_signal_scroll_vertical
 * btk_list_signal_toggle_add_mode
 * btk_list_signal_item_select
 * btk_list_signal_item_deselect
 * btk_list_signal_item_toggle
 */
static void
btk_list_signal_toggle_focus_row (BtkListItem *list_item,
				  BtkList     *list)
{
  g_return_if_fail (BTK_IS_LIST_ITEM (list_item));
  g_return_if_fail (BTK_IS_LIST (list));

  btk_list_toggle_focus_row (list);
}

static void
btk_list_signal_select_all (BtkListItem *list_item,
			    BtkList     *list)
{
  g_return_if_fail (BTK_IS_LIST_ITEM (list_item));
  g_return_if_fail (BTK_IS_LIST (list));

  btk_list_select_all (list);
}

static void
btk_list_signal_unselect_all (BtkListItem *list_item,
			      BtkList     *list)
{
  g_return_if_fail (BTK_IS_LIST_ITEM (list_item));
  g_return_if_fail (BTK_IS_LIST (list));

  btk_list_unselect_all (list);
}

static void
btk_list_signal_undo_selection (BtkListItem *list_item,
				BtkList     *list)
{
  g_return_if_fail (BTK_IS_LIST_ITEM (list_item));
  g_return_if_fail (BTK_IS_LIST (list));

  btk_list_undo_selection (list);
}

static void
btk_list_signal_start_selection (BtkListItem *list_item,
				 BtkList     *list)
{
  g_return_if_fail (BTK_IS_LIST_ITEM (list_item));
  g_return_if_fail (BTK_IS_LIST (list));

  btk_list_start_selection (list);
}

static void
btk_list_signal_end_selection (BtkListItem *list_item,
			       BtkList     *list)
{
  g_return_if_fail (BTK_IS_LIST_ITEM (list_item));
  g_return_if_fail (BTK_IS_LIST (list));

  btk_list_end_selection (list);
}

static void
btk_list_signal_extend_selection (BtkListItem   *list_item,
				  BtkScrollType  scroll_type,
				  bfloat         position,
				  bboolean       auto_start_selection,
				  BtkList       *list)
{
  g_return_if_fail (BTK_IS_LIST_ITEM (list_item));
  g_return_if_fail (BTK_IS_LIST (list));

  btk_list_extend_selection (list, scroll_type, position,
			     auto_start_selection);
}

static void
btk_list_signal_scroll_horizontal (BtkListItem   *list_item,
				   BtkScrollType  scroll_type,
				   bfloat         position,
				   BtkList       *list)
{
  g_return_if_fail (BTK_IS_LIST_ITEM (list_item));
  g_return_if_fail (BTK_IS_LIST (list));

  btk_list_scroll_horizontal (list, scroll_type, position);
}

static void
btk_list_signal_scroll_vertical (BtkListItem   *list_item,
				 BtkScrollType  scroll_type,
				 bfloat         position,
				 BtkList       *list)
{
  g_return_if_fail (BTK_IS_LIST_ITEM (list_item));
  g_return_if_fail (BTK_IS_LIST (list));

  btk_list_scroll_vertical (list, scroll_type, position);
}

static void
btk_list_signal_toggle_add_mode (BtkListItem *list_item,
				 BtkList     *list)
{
  g_return_if_fail (BTK_IS_LIST_ITEM (list_item));
  g_return_if_fail (BTK_IS_LIST (list));

  btk_list_toggle_add_mode (list);
}

static void
btk_list_signal_item_select (BtkListItem *list_item,
			     BtkList     *list)
{
  GList *selection;
  GList *tmp_list;
  GList *sel_list;

  g_return_if_fail (BTK_IS_LIST_ITEM (list_item));
  g_return_if_fail (BTK_IS_LIST (list));

  if (BTK_WIDGET (list_item)->state != BTK_STATE_SELECTED)
    return;

  switch (list->selection_mode)
    {
    case BTK_SELECTION_SINGLE:
    case BTK_SELECTION_BROWSE:
      sel_list = NULL;
      selection = list->selection;

      while (selection)
	{
	  tmp_list = selection;
	  selection = selection->next;

	  if (tmp_list->data == list_item)
	    sel_list = tmp_list;
	  else
	    btk_list_item_deselect (BTK_LIST_ITEM (tmp_list->data));
	}

      if (!sel_list)
	{
	  list->selection = g_list_prepend (list->selection, list_item);
	  g_object_ref (list_item);
	}
      btk_signal_emit (BTK_OBJECT (list), list_signals[SELECTION_CHANGED]);
      break;
    case BTK_SELECTION_MULTIPLE:
      if (list->anchor >= 0)
	return;
    }
}

static void
btk_list_signal_item_deselect (BtkListItem *list_item,
			       BtkList     *list)
{
  GList *node;

  g_return_if_fail (BTK_IS_LIST_ITEM (list_item));
  g_return_if_fail (BTK_IS_LIST (list));

  if (BTK_WIDGET (list_item)->state != BTK_STATE_NORMAL)
    return;

  node = g_list_find (list->selection, list_item);

  if (node)
    {
      list->selection = g_list_remove_link (list->selection, node);
      g_list_free_1 (node);
      g_object_unref (list_item);
      btk_signal_emit (BTK_OBJECT (list), list_signals[SELECTION_CHANGED]);
    }
}

static void
btk_list_signal_item_toggle (BtkListItem *list_item,
			     BtkList     *list)
{
  g_return_if_fail (BTK_IS_LIST_ITEM (list_item));
  g_return_if_fail (BTK_IS_LIST (list));

  if ((list->selection_mode == BTK_SELECTION_BROWSE ||
       list->selection_mode == BTK_SELECTION_MULTIPLE) &&
      BTK_WIDGET (list_item)->state == BTK_STATE_NORMAL)
    {
      btk_widget_set_state (BTK_WIDGET (list_item), BTK_STATE_SELECTED);
      return;
    }
  
  switch (BTK_WIDGET (list_item)->state)
    {
    case BTK_STATE_SELECTED:
      btk_list_signal_item_select (list_item, list);
      break;
    case BTK_STATE_NORMAL:
      btk_list_signal_item_deselect (list_item, list);
      break;
    default:
      break;
    }
}

static void
btk_list_signal_drag_begin (BtkWidget      *widget,
			    BdkDragContext *context,
			    BtkList	    *list)
{
  g_return_if_fail (BTK_IS_LIST_ITEM (widget));
  g_return_if_fail (BTK_IS_LIST (list));

  btk_list_drag_begin (BTK_WIDGET (list), context);
}

static void
btk_list_drag_begin (BtkWidget      *widget,
		     BdkDragContext *context)
{
  BtkList *list;

  g_return_if_fail (BTK_IS_LIST (widget));
  g_return_if_fail (context != NULL);

  list = BTK_LIST (widget);

  if (list->drag_selection)
    {
      btk_list_end_drag_selection (list);

      switch (list->selection_mode)
	{
	case BTK_SELECTION_MULTIPLE:
	  btk_list_end_selection (list);
	  break;
	case BTK_SELECTION_SINGLE:
	  list->undo_focus_child = NULL;
	  break;
	default:
	  break;
	}
    }
}

#include "btkaliasdef.c"
