/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Jsh MacDonald
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

#include "bdk/bdkkeysyms.h"

#undef BTK_DISABLE_DEPRECATED

#include "btkmenu.h"
#include "btkmenuitem.h"
#include "btkmarshalers.h"
#include "btkoptionmenu.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

#define CHILD_LEFT_SPACING        4
#define CHILD_RIGHT_SPACING       1
#define CHILD_TOP_SPACING         1
#define CHILD_BOTTOM_SPACING      1

typedef struct _BtkOptionMenuProps BtkOptionMenuProps;

struct _BtkOptionMenuProps
{
  gboolean interior_focus;
  BtkRequisition indicator_size;
  BtkBorder indicator_spacing;
  gint focus_width;
  gint focus_pad;
};

static const BtkOptionMenuProps default_props = {
  TRUE,
  { 7, 13 },
  { 7, 5, 2, 2 },		/* Left, right, top, bottom */
  1,
  0
};

static void btk_option_menu_destroy         (BtkObject          *object);
static void btk_option_menu_set_property    (GObject            *object,
					     guint               prop_id,
					     const GValue       *value,
					     GParamSpec         *pspec);
static void btk_option_menu_get_property    (GObject            *object,
					     guint               prop_id,
					     GValue             *value,
					     GParamSpec         *pspec);
static void btk_option_menu_size_request    (BtkWidget          *widget,
					     BtkRequisition     *requisition);
static void btk_option_menu_size_allocate   (BtkWidget          *widget,
					     BtkAllocation      *allocation);
static void btk_option_menu_paint           (BtkWidget          *widget,
					     BdkRectangle       *area);
static gint btk_option_menu_expose          (BtkWidget          *widget,
					     BdkEventExpose     *event);
static gint btk_option_menu_button_press    (BtkWidget          *widget,
					     BdkEventButton     *event);
static gint btk_option_menu_key_press	    (BtkWidget          *widget,
					     BdkEventKey        *event);
static void btk_option_menu_selection_done  (BtkMenuShell       *menu_shell,
					     BtkOptionMenu      *option_menu);
static void btk_option_menu_update_contents (BtkOptionMenu      *option_menu);
static void btk_option_menu_remove_contents (BtkOptionMenu      *option_menu);
static void btk_option_menu_calc_size       (BtkOptionMenu      *option_menu);
static void btk_option_menu_position        (BtkMenu            *menu,
					     gint               *x,
					     gint               *y,
					     gint               *scroll_offet,
					     gpointer            user_data);
static void btk_option_menu_show_all        (BtkWidget          *widget);
static void btk_option_menu_hide_all        (BtkWidget          *widget);
static gboolean btk_option_menu_mnemonic_activate (BtkWidget    *widget,
						   gboolean      group_cycling);
static GType btk_option_menu_child_type   (BtkContainer       *container);
static gint btk_option_menu_scroll_event    (BtkWidget          *widget,
					     BdkEventScroll     *event);

enum
{
  CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_MENU
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BtkOptionMenu, btk_option_menu, BTK_TYPE_BUTTON)

static void
btk_option_menu_class_init (BtkOptionMenuClass *class)
{
  GObjectClass *bobject_class;
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;
  BtkContainerClass *container_class;

  bobject_class = (GObjectClass*) class;
  object_class = (BtkObjectClass*) class;
  widget_class = (BtkWidgetClass*) class;
  container_class = (BtkContainerClass*) class;

  signals[CHANGED] =
    g_signal_new (I_("changed"),
                  G_OBJECT_CLASS_TYPE (class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkOptionMenuClass, changed),
                  NULL, NULL,
                  _btk_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  bobject_class->set_property = btk_option_menu_set_property;
  bobject_class->get_property = btk_option_menu_get_property;
  object_class->destroy = btk_option_menu_destroy;
  
  widget_class->size_request = btk_option_menu_size_request;
  widget_class->size_allocate = btk_option_menu_size_allocate;
  widget_class->expose_event = btk_option_menu_expose;
  widget_class->button_press_event = btk_option_menu_button_press;
  widget_class->key_press_event = btk_option_menu_key_press;
  widget_class->scroll_event = btk_option_menu_scroll_event;
  widget_class->show_all = btk_option_menu_show_all;
  widget_class->hide_all = btk_option_menu_hide_all;
  widget_class->mnemonic_activate = btk_option_menu_mnemonic_activate;

  container_class->child_type = btk_option_menu_child_type;

  g_object_class_install_property (bobject_class,
                                   PROP_MENU,
                                   g_param_spec_object ("menu",
                                                        P_("Menu"),
                                                        P_("The menu of options"),
                                                        BTK_TYPE_MENU,
                                                        BTK_PARAM_READWRITE));
  
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_boxed ("indicator-size",
							       P_("Indicator Size"),
							       P_("Size of dropdown indicator"),
							       BTK_TYPE_REQUISITION,
							       BTK_PARAM_READABLE));
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_boxed ("indicator-spacing",
							       P_("Indicator Spacing"),
							       P_("Spacing around indicator"),
							       BTK_TYPE_BORDER,
							       BTK_PARAM_READABLE));
}

static GType
btk_option_menu_child_type (BtkContainer       *container)
{
  return G_TYPE_NONE;
}

static void
btk_option_menu_init (BtkOptionMenu *option_menu)
{
  BtkWidget *widget = BTK_WIDGET (option_menu);

  btk_widget_set_can_focus (widget, TRUE);
  btk_widget_set_can_default (widget, FALSE);
  btk_widget_set_receives_default (widget, FALSE);

  option_menu->menu = NULL;
  option_menu->menu_item = NULL;
  option_menu->width = 0;
  option_menu->height = 0;
}

BtkWidget*
btk_option_menu_new (void)
{
  return g_object_new (BTK_TYPE_OPTION_MENU, NULL);
}

BtkWidget*
btk_option_menu_get_menu (BtkOptionMenu *option_menu)
{
  g_return_val_if_fail (BTK_IS_OPTION_MENU (option_menu), NULL);

  return option_menu->menu;
}

static void
btk_option_menu_detacher (BtkWidget     *widget,
			  BtkMenu	*menu)
{
  BtkOptionMenu *option_menu;

  g_return_if_fail (BTK_IS_OPTION_MENU (widget));

  option_menu = BTK_OPTION_MENU (widget);
  g_return_if_fail (option_menu->menu == (BtkWidget*) menu);

  btk_option_menu_remove_contents (option_menu);
  g_signal_handlers_disconnect_by_func (option_menu->menu,
					btk_option_menu_selection_done,
					option_menu);
  g_signal_handlers_disconnect_by_func (option_menu->menu,
					btk_option_menu_calc_size,
					option_menu);
  
  option_menu->menu = NULL;
  g_object_notify (G_OBJECT (option_menu), "menu");
}

void
btk_option_menu_set_menu (BtkOptionMenu *option_menu,
			  BtkWidget     *menu)
{
  g_return_if_fail (BTK_IS_OPTION_MENU (option_menu));
  g_return_if_fail (BTK_IS_MENU (menu));

  if (option_menu->menu != menu)
    {
      btk_option_menu_remove_menu (option_menu);

      option_menu->menu = menu;
      btk_menu_attach_to_widget (BTK_MENU (menu),
				 BTK_WIDGET (option_menu),
				 btk_option_menu_detacher);

      btk_option_menu_calc_size (option_menu);

      g_signal_connect_after (option_menu->menu, "selection-done",
			      G_CALLBACK (btk_option_menu_selection_done),
			      option_menu);
      g_signal_connect_swapped (option_menu->menu, "size-request",
				G_CALLBACK (btk_option_menu_calc_size),
				option_menu);

      if (BTK_WIDGET (option_menu)->parent)
	btk_widget_queue_resize (BTK_WIDGET (option_menu));

      btk_option_menu_update_contents (option_menu);
      
      g_object_notify (G_OBJECT (option_menu), "menu");
    }
}

void
btk_option_menu_remove_menu (BtkOptionMenu *option_menu)
{
  g_return_if_fail (BTK_IS_OPTION_MENU (option_menu));

  if (option_menu->menu)
    {
      if (BTK_MENU_SHELL (option_menu->menu)->active)
	btk_menu_shell_cancel (BTK_MENU_SHELL (option_menu->menu));
      
      btk_menu_detach (BTK_MENU (option_menu->menu));
    }
}

void
btk_option_menu_set_history (BtkOptionMenu *option_menu,
			     guint          index)
{
  BtkWidget *menu_item;

  g_return_if_fail (BTK_IS_OPTION_MENU (option_menu));

  if (option_menu->menu)
    {
      btk_menu_set_active (BTK_MENU (option_menu->menu), index);
      menu_item = btk_menu_get_active (BTK_MENU (option_menu->menu));

      if (menu_item != option_menu->menu_item)
        btk_option_menu_update_contents (option_menu);
    }
}

/**
 * btk_option_menu_get_history:
 * @option_menu: a #BtkOptionMenu
 * 
 * Retrieves the index of the currently selected menu item. The menu
 * items are numbered from top to bottom, starting with 0. 
 * 
 * Return value: index of the selected menu item, or -1 if there are no menu items
 * Deprecated: 2.4: Use #BtkComboBox instead.
 **/
gint
btk_option_menu_get_history (BtkOptionMenu *option_menu)
{
  BtkWidget *active_widget;
  
  g_return_val_if_fail (BTK_IS_OPTION_MENU (option_menu), -1);

  if (option_menu->menu)
    {
      active_widget = btk_menu_get_active (BTK_MENU (option_menu->menu));

      if (active_widget)
	return g_list_index (BTK_MENU_SHELL (option_menu->menu)->children,
                             active_widget);
      else
	return -1;
    }
  else
    return -1;
}

static void
btk_option_menu_set_property (GObject            *object,
			      guint               prop_id,
			      const GValue       *value,
			      GParamSpec         *pspec)
{
  BtkOptionMenu *option_menu = BTK_OPTION_MENU (object);

  switch (prop_id)
    {
    case PROP_MENU:
      btk_option_menu_set_menu (option_menu, g_value_get_object (value));
      break;
      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_option_menu_get_property (GObject            *object,
			      guint               prop_id,
			      GValue             *value,
			      GParamSpec         *pspec)
{
  BtkOptionMenu *option_menu = BTK_OPTION_MENU (object);

  switch (prop_id)
    {
    case PROP_MENU:
      g_value_set_object (value, option_menu->menu);
      break;
      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_option_menu_destroy (BtkObject *object)
{
  BtkOptionMenu *option_menu = BTK_OPTION_MENU (object);

  if (option_menu->menu)
    btk_widget_destroy (option_menu->menu);

  BTK_OBJECT_CLASS (btk_option_menu_parent_class)->destroy (object);
}

static void
btk_option_menu_get_props (BtkOptionMenu       *option_menu,
			   BtkOptionMenuProps  *props)
{
  BtkRequisition *indicator_size;
  BtkBorder *indicator_spacing;
  
  btk_widget_style_get (BTK_WIDGET (option_menu),
			"indicator-size", &indicator_size,
			"indicator-spacing", &indicator_spacing,
			"interior-focus", &props->interior_focus,
			"focus-line-width", &props->focus_width,
			"focus-padding", &props->focus_pad,
			NULL);

  if (indicator_size)
    props->indicator_size = *indicator_size;
  else
    props->indicator_size = default_props.indicator_size;

  if (indicator_spacing)
    props->indicator_spacing = *indicator_spacing;
  else
    props->indicator_spacing = default_props.indicator_spacing;

  btk_requisition_free (indicator_size);
  btk_border_free (indicator_spacing);
}

static void
btk_option_menu_size_request (BtkWidget      *widget,
			      BtkRequisition *requisition)
{
  BtkOptionMenu *option_menu = BTK_OPTION_MENU (widget);
  BtkOptionMenuProps props;
  gint tmp;
  BtkRequisition child_requisition = { 0, 0 };
      
  btk_option_menu_get_props (option_menu, &props);
 
  if (BTK_BIN (option_menu)->child && btk_widget_get_visible (BTK_BIN (option_menu)->child))
    {
      btk_widget_size_request (BTK_BIN (option_menu)->child, &child_requisition);

      requisition->width += child_requisition.width;
      requisition->height += child_requisition.height;
    }
  
  requisition->width = ((BTK_CONTAINER (widget)->border_width +
			 BTK_WIDGET (widget)->style->xthickness + props.focus_pad) * 2 +
			MAX (child_requisition.width, option_menu->width) +
 			props.indicator_size.width +
 			props.indicator_spacing.left + props.indicator_spacing.right +
			CHILD_LEFT_SPACING + CHILD_RIGHT_SPACING + props.focus_width * 2);
  requisition->height = ((BTK_CONTAINER (widget)->border_width +
			  BTK_WIDGET (widget)->style->ythickness + props.focus_pad) * 2 +
			 MAX (child_requisition.height, option_menu->height) +
			 CHILD_TOP_SPACING + CHILD_BOTTOM_SPACING + props.focus_width * 2);

  tmp = (requisition->height - MAX (child_requisition.height, option_menu->height) +
	 props.indicator_size.height + props.indicator_spacing.top + props.indicator_spacing.bottom);
  requisition->height = MAX (requisition->height, tmp);
}

static void
btk_option_menu_size_allocate (BtkWidget     *widget,
			       BtkAllocation *allocation)
{
  BtkWidget *child;
  BtkButton *button = BTK_BUTTON (widget);
  BtkAllocation child_allocation;
  BtkOptionMenuProps props;
  gint border_width;
    
  btk_option_menu_get_props (BTK_OPTION_MENU (widget), &props);
  border_width = BTK_CONTAINER (widget)->border_width;

  widget->allocation = *allocation;
  if (btk_widget_get_realized (widget))
    bdk_window_move_resize (button->event_window,
			    allocation->x + border_width, allocation->y + border_width,
			    allocation->width - border_width * 2, allocation->height - border_width * 2);

  child = BTK_BIN (widget)->child;
  if (child && btk_widget_get_visible (child))
    {
      gint xthickness = BTK_WIDGET (widget)->style->xthickness;
      gint ythickness = BTK_WIDGET (widget)->style->ythickness;
      
      child_allocation.x = widget->allocation.x + border_width + xthickness + props.focus_width + props.focus_pad + CHILD_LEFT_SPACING;
      child_allocation.y = widget->allocation.y + border_width + ythickness + props.focus_width + props.focus_pad + CHILD_TOP_SPACING;
      child_allocation.width = MAX (1, allocation->width - (border_width + xthickness + props.focus_width + props.focus_pad) * 2 -
				    props.indicator_size.width - props.indicator_spacing.left - props.indicator_spacing.right -
				    CHILD_LEFT_SPACING - CHILD_RIGHT_SPACING);
      child_allocation.height = MAX (1, allocation->height - (border_width + ythickness + props.focus_width + props.focus_pad) * 2 -
				     CHILD_TOP_SPACING - CHILD_BOTTOM_SPACING);

      if (btk_widget_get_direction (BTK_WIDGET (widget)) == BTK_TEXT_DIR_RTL) 
	child_allocation.x += props.indicator_size.width + props.indicator_spacing.left + props.indicator_spacing.right;

      btk_widget_size_allocate (child, &child_allocation);
    }
}

static void
btk_option_menu_paint (BtkWidget    *widget,
		       BdkRectangle *area)
{
  BdkRectangle button_area;
  BtkOptionMenuProps props;
  gint border_width;
  gint tab_x;

  g_return_if_fail (BTK_IS_OPTION_MENU (widget));
  g_return_if_fail (area != NULL);

  if (BTK_WIDGET_DRAWABLE (widget))
    {
      border_width = BTK_CONTAINER (widget)->border_width;
      btk_option_menu_get_props (BTK_OPTION_MENU (widget), &props);

      button_area.x = widget->allocation.x + border_width;
      button_area.y = widget->allocation.y + border_width;
      button_area.width = widget->allocation.width - 2 * border_width;
      button_area.height = widget->allocation.height - 2 * border_width;

      if (!props.interior_focus && btk_widget_has_focus (widget))
	{
	  button_area.x += props.focus_width + props.focus_pad;
	  button_area.y += props.focus_width + props.focus_pad;
	  button_area.width -= 2 * (props.focus_width + props.focus_pad);
	  button_area.height -= 2 * (props.focus_width + props.focus_pad);
	}
      
      btk_paint_box (widget->style, widget->window,
		     btk_widget_get_state (widget), BTK_SHADOW_OUT,
		     area, widget, "optionmenu",
		     button_area.x, button_area.y,
		     button_area.width, button_area.height);
      
      if (btk_widget_get_direction (BTK_WIDGET (widget)) == BTK_TEXT_DIR_RTL) 
	tab_x = button_area.x + props.indicator_spacing.right + 
	  widget->style->xthickness;
      else
	tab_x = button_area.x + button_area.width - 
	  props.indicator_size.width - props.indicator_spacing.right -
	  widget->style->xthickness;

      btk_paint_tab (widget->style, widget->window,
		     btk_widget_get_state (widget), BTK_SHADOW_OUT,
		     area, widget, "optionmenutab",
		     tab_x,
		     button_area.y + (button_area.height - props.indicator_size.height) / 2,
		     props.indicator_size.width, props.indicator_size.height);
      
      if (btk_widget_has_focus (widget))
	{
	  if (props.interior_focus)
	    {
	      button_area.x += widget->style->xthickness + props.focus_pad;
	      button_area.y += widget->style->ythickness + props.focus_pad;
	      button_area.width -= 2 * (widget->style->xthickness + props.focus_pad) +
		      props.indicator_spacing.left +
		      props.indicator_spacing.right +
		      props.indicator_size.width;
	      button_area.height -= 2 * (widget->style->ythickness + props.focus_pad);
	      if (btk_widget_get_direction (BTK_WIDGET (widget)) == BTK_TEXT_DIR_RTL) 
		button_area.x += props.indicator_spacing.left +
		  props.indicator_spacing.right +
		  props.indicator_size.width;
	    }
	  else
	    {
	      button_area.x -= props.focus_width + props.focus_pad;
	      button_area.y -= props.focus_width + props.focus_pad;
	      button_area.width += 2 * (props.focus_width + props.focus_pad);
	      button_area.height += 2 * (props.focus_width + props.focus_pad);
	    }
	    
	  btk_paint_focus (widget->style, widget->window, btk_widget_get_state (widget),
			   area, widget, "button",
			   button_area.x, 
			   button_area.y, 
			   button_area.width,
			   button_area.height);
	}
    }
}

static gint
btk_option_menu_expose (BtkWidget      *widget,
			BdkEventExpose *event)
{
  g_return_val_if_fail (BTK_IS_OPTION_MENU (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (BTK_WIDGET_DRAWABLE (widget))
    {
      btk_option_menu_paint (widget, &event->area);


      /* The following code tries to draw the child in two places at
       * once. It fails miserably for several reasons
       *
       * - If the child is not no-window, removing generates
       *   more expose events. Bad, bad, bad.
       * 
       * - Even if the child is no-window, removing it now (properly)
       *   clears the space where it was, so it does no good
       */
      
#if 0
      remove_child = FALSE;
      child = BTK_BUTTON (widget)->child;

      if (!child)
	{
	  if (!BTK_OPTION_MENU (widget)->menu)
	    return FALSE;
	  btk_option_menu_update_contents (BTK_OPTION_MENU (widget));
	  child = BTK_BUTTON (widget)->child;
	  if (!child)
	    return FALSE;
	  remove_child = TRUE;
	}

      child_event = *event;

      if (!btk_widget_get_has_window (child) &&
	  btk_widget_intersect (child, &event->area, &child_event.area))
	btk_widget_event (child, (BdkEvent*) &child_event);

      if (remove_child)
	btk_option_menu_remove_contents (BTK_OPTION_MENU (widget));
#else
      if (BTK_BIN (widget)->child)
	btk_container_propagate_expose (BTK_CONTAINER (widget),
					BTK_BIN (widget)->child,
					event);
#endif /* 0 */
    }

  return FALSE;
}

static gint
btk_option_menu_button_press (BtkWidget      *widget,
			      BdkEventButton *event)
{
  BtkOptionMenu *option_menu;
  BtkWidget *menu_item;

  g_return_val_if_fail (BTK_IS_OPTION_MENU (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  option_menu = BTK_OPTION_MENU (widget);

  if ((event->type == BDK_BUTTON_PRESS) &&
      (event->button == 1))
    {
      btk_option_menu_remove_contents (option_menu);
      btk_menu_popup (BTK_MENU (option_menu->menu), NULL, NULL,
		      btk_option_menu_position, option_menu,
		      event->button, event->time);
      menu_item = btk_menu_get_active (BTK_MENU (option_menu->menu));
      if (menu_item)
	btk_menu_shell_select_item (BTK_MENU_SHELL (option_menu->menu), menu_item);
      return TRUE;
    }

  return FALSE;
}

static gint
btk_option_menu_key_press (BtkWidget   *widget,
			   BdkEventKey *event)
{
  BtkOptionMenu *option_menu;
  BtkWidget *menu_item;

  g_return_val_if_fail (BTK_IS_OPTION_MENU (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  option_menu = BTK_OPTION_MENU (widget);

  switch (event->keyval)
    {
    case BDK_KP_Space:
    case BDK_space:
      btk_option_menu_remove_contents (option_menu);
      btk_menu_popup (BTK_MENU (option_menu->menu), NULL, NULL,
		      btk_option_menu_position, option_menu,
		      0, event->time);
      menu_item = btk_menu_get_active (BTK_MENU (option_menu->menu));
      if (menu_item)
	btk_menu_shell_select_item (BTK_MENU_SHELL (option_menu->menu), menu_item);
      return TRUE;
    }
  
  return FALSE;
}

static void
btk_option_menu_selection_done (BtkMenuShell  *menu_shell,
				BtkOptionMenu *option_menu)
{
  g_return_if_fail (menu_shell != NULL);
  g_return_if_fail (BTK_IS_OPTION_MENU (option_menu));

  btk_option_menu_update_contents (option_menu);
}

static void
btk_option_menu_changed (BtkOptionMenu *option_menu)
{
  g_return_if_fail (BTK_IS_OPTION_MENU (option_menu));

  g_signal_emit (option_menu, signals[CHANGED], 0);
}

static void
btk_option_menu_select_first_sensitive (BtkOptionMenu *option_menu)
{
  if (option_menu->menu)
    {
      GList *children = BTK_MENU_SHELL (option_menu->menu)->children;
      gint index = 0;

      while (children)
	{
	  if (btk_widget_get_sensitive (children->data))
	    {
	      btk_option_menu_set_history (option_menu, index);
	      return;
	    }
	  
	  children = children->next;
	  index++;
	}
    }
}

static void
btk_option_menu_item_state_changed_cb (BtkWidget      *widget,
				       BtkStateType    previous_state,
				       BtkOptionMenu  *option_menu)
{
  BtkWidget *child = BTK_BIN (option_menu)->child;

  if (child && btk_widget_get_sensitive (child) != btk_widget_is_sensitive (widget))
    btk_widget_set_sensitive (child, btk_widget_is_sensitive (widget));
}

static void
btk_option_menu_item_destroy_cb (BtkWidget     *widget,
				 BtkOptionMenu *option_menu)
{
  BtkWidget *child = BTK_BIN (option_menu)->child;

  if (child)
    {
      g_object_ref (child);
      btk_option_menu_remove_contents (option_menu);
      btk_widget_destroy (child);
      g_object_unref (child);

      btk_option_menu_select_first_sensitive (option_menu);
    }
}

static void
btk_option_menu_update_contents (BtkOptionMenu *option_menu)
{
  BtkWidget *child;
  BtkRequisition child_requisition;

  g_return_if_fail (BTK_IS_OPTION_MENU (option_menu));

  if (option_menu->menu)
    {
      BtkWidget *old_item = option_menu->menu_item;
      
      btk_option_menu_remove_contents (option_menu);

      option_menu->menu_item = btk_menu_get_active (BTK_MENU (option_menu->menu));
      if (option_menu->menu_item)
	{
	  g_object_ref (option_menu->menu_item);
	  child = BTK_BIN (option_menu->menu_item)->child;
	  if (child)
	    {
	      if (!btk_widget_is_sensitive (option_menu->menu_item))
		btk_widget_set_sensitive (child, FALSE);
	      btk_widget_reparent (child, BTK_WIDGET (option_menu));
	    }

	  g_signal_connect (option_menu->menu_item, "state-changed",
			    G_CALLBACK (btk_option_menu_item_state_changed_cb), option_menu);
	  g_signal_connect (option_menu->menu_item, "destroy",
			    G_CALLBACK (btk_option_menu_item_destroy_cb), option_menu);

	  btk_widget_size_request (child, &child_requisition);
	  btk_widget_size_allocate (BTK_WIDGET (option_menu),
				    &(BTK_WIDGET (option_menu)->allocation));

	  if (BTK_WIDGET_DRAWABLE (option_menu))
	    btk_widget_queue_draw (BTK_WIDGET (option_menu));
	}

      if (old_item != option_menu->menu_item)
        btk_option_menu_changed (option_menu);
    }
}

static void
btk_option_menu_remove_contents (BtkOptionMenu *option_menu)
{
  BtkWidget *child;
  
  g_return_if_fail (BTK_IS_OPTION_MENU (option_menu));

  if (option_menu->menu_item)
    {
      child = BTK_BIN (option_menu)->child;
  
      if (child)
	{
	  btk_widget_set_sensitive (child, TRUE);
	  btk_widget_set_state (child, BTK_STATE_NORMAL);
	  btk_widget_reparent (child, option_menu->menu_item);
	}

      g_signal_handlers_disconnect_by_func (option_menu->menu_item,
					    btk_option_menu_item_state_changed_cb,
					    option_menu);				     
      g_signal_handlers_disconnect_by_func (option_menu->menu_item,
					    btk_option_menu_item_destroy_cb,
					    option_menu);   
      
      g_object_unref (option_menu->menu_item);
      option_menu->menu_item = NULL;
    }
}

static void
btk_option_menu_calc_size (BtkOptionMenu *option_menu)
{
  BtkWidget *child;
  GList *children;
  BtkRequisition child_requisition;
  gint old_width = option_menu->width;
  gint old_height = option_menu->height;

  g_return_if_fail (BTK_IS_OPTION_MENU (option_menu));

  option_menu->width = 0;
  option_menu->height = 0;

  if (option_menu->menu)
    {
      children = BTK_MENU_SHELL (option_menu->menu)->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if (btk_widget_get_visible (child))
	    {
	      BtkWidget *inner = BTK_BIN (child)->child;

	      if (inner)
		{
		  btk_widget_size_request (inner, &child_requisition);

		  option_menu->width = MAX (option_menu->width, child_requisition.width);
		  option_menu->height = MAX (option_menu->height, child_requisition.height);
		}
	    }
	}
    }

  if (old_width != option_menu->width || old_height != option_menu->height)
    btk_widget_queue_resize (BTK_WIDGET (option_menu));
}

static void
btk_option_menu_position (BtkMenu  *menu,
			  gint     *x,
			  gint     *y,
			  gboolean *push_in,
			  gpointer  user_data)
{
  BtkOptionMenu *option_menu;
  BtkWidget *active;
  BtkWidget *child;
  BtkWidget *widget;
  BtkRequisition requisition;
  GList *children;
  gint screen_width;
  gint menu_xpos;
  gint menu_ypos;
  gint menu_width;

  g_return_if_fail (BTK_IS_OPTION_MENU (user_data));

  option_menu = BTK_OPTION_MENU (user_data);
  widget = BTK_WIDGET (option_menu);

  btk_widget_get_child_requisition (BTK_WIDGET (menu), &requisition);
  menu_width = requisition.width;

  active = btk_menu_get_active (BTK_MENU (option_menu->menu));
  bdk_window_get_origin (widget->window, &menu_xpos, &menu_ypos);

  /* set combo box type hint for menu popup */
  btk_window_set_type_hint (BTK_WINDOW (BTK_MENU (option_menu->menu)->toplevel),
			    BDK_WINDOW_TYPE_HINT_COMBO);

  menu_xpos += widget->allocation.x;
  menu_ypos += widget->allocation.y + widget->allocation.height / 2 - 2;

  if (active != NULL)
    {
      btk_widget_get_child_requisition (active, &requisition);
      menu_ypos -= requisition.height / 2;
    }

  children = BTK_MENU_SHELL (option_menu->menu)->children;
  while (children)
    {
      child = children->data;

      if (active == child)
	break;

      if (btk_widget_get_visible (child))
	{
	  btk_widget_get_child_requisition (child, &requisition);
	  menu_ypos -= requisition.height;
	}

      children = children->next;
    }

  if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL)
    menu_xpos = menu_xpos + widget->allocation.width - menu_width;

  /* Clamp the position on screen */
  screen_width = bdk_screen_get_width (btk_widget_get_screen (widget));
  
  if (menu_xpos < 0)
    menu_xpos = 0;
  else if ((menu_xpos + menu_width) > screen_width)
    menu_xpos -= ((menu_xpos + menu_width) - screen_width);

  *x = menu_xpos;
  *y = menu_ypos;
  *push_in = TRUE;
}


static void
btk_option_menu_show_all (BtkWidget *widget)
{
  BtkContainer *container;
  BtkOptionMenu *option_menu;
  
  g_return_if_fail (BTK_IS_OPTION_MENU (widget));
  container = BTK_CONTAINER (widget);
  option_menu = BTK_OPTION_MENU (widget);

  btk_widget_show (widget);
  btk_container_foreach (container, (BtkCallback) btk_widget_show_all, NULL);
  if (option_menu->menu)
    btk_widget_show_all (option_menu->menu);
  if (option_menu->menu_item)
    btk_widget_show_all (option_menu->menu_item);
}


static void
btk_option_menu_hide_all (BtkWidget *widget)
{
  BtkContainer *container;

  g_return_if_fail (BTK_IS_OPTION_MENU (widget));
  container = BTK_CONTAINER (widget);

  btk_widget_hide (widget);
  btk_container_foreach (container, (BtkCallback) btk_widget_hide_all, NULL);
}

static gboolean
btk_option_menu_mnemonic_activate (BtkWidget *widget,
				   gboolean   group_cycling)
{
  btk_widget_grab_focus (widget);
  return TRUE;
}

static gint
btk_option_menu_scroll_event (BtkWidget          *widget,
			      BdkEventScroll     *event)
{
  BtkOptionMenu *option_menu = BTK_OPTION_MENU (widget);
  gint index;
  gint n_children;
  gint index_dir;
  GList *l;
  BtkMenuItem *item;
    
  index = btk_option_menu_get_history (option_menu);

  if (index != -1)
    {
      n_children = g_list_length (BTK_MENU_SHELL (option_menu->menu)->children);
      
      if (event->direction == BDK_SCROLL_UP)
	index_dir = -1;
      else
	index_dir = 1;


      while (TRUE)
	{
	  index += index_dir;

	  if (index < 0)
	    break;
	  if (index >= n_children)
	    break;

	  l = g_list_nth (BTK_MENU_SHELL (option_menu->menu)->children, index);
	  item = BTK_MENU_ITEM (l->data);
	  if (btk_widget_get_visible (BTK_WIDGET (item)) &&
              btk_widget_is_sensitive (BTK_WIDGET (item)))
	    {
	      btk_option_menu_set_history (option_menu, index);
	      btk_menu_item_activate (BTK_MENU_ITEM (item));
	      break;
	    }
	      
	}
    }

  return TRUE;
}

#define __BTK_OPTION_MENU_C__
#include "btkaliasdef.c"

