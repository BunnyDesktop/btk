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
#include "btkcheckbutton.h"
#include "btklabel.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"


#define INDICATOR_SIZE     13
#define INDICATOR_SPACING  2


static void btk_check_button_size_request        (BtkWidget           *widget,
						  BtkRequisition      *requisition);
static void btk_check_button_size_allocate       (BtkWidget           *widget,
						  BtkAllocation       *allocation);
static gint btk_check_button_expose              (BtkWidget           *widget,
						  BdkEventExpose      *event);
static void btk_check_button_paint               (BtkWidget           *widget,
						  BdkRectangle        *area);
static void btk_check_button_draw_indicator      (BtkCheckButton      *check_button,
						  BdkRectangle        *area);
static void btk_real_check_button_draw_indicator (BtkCheckButton      *check_button,
						  BdkRectangle        *area);

G_DEFINE_TYPE (BtkCheckButton, btk_check_button, BTK_TYPE_TOGGLE_BUTTON)

static void
btk_check_button_class_init (BtkCheckButtonClass *class)
{
  BtkWidgetClass *widget_class;
  
  widget_class = (BtkWidgetClass*) class;
  
  widget_class->size_request = btk_check_button_size_request;
  widget_class->size_allocate = btk_check_button_size_allocate;
  widget_class->expose_event = btk_check_button_expose;

  class->draw_indicator = btk_real_check_button_draw_indicator;

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("indicator-size",
							     P_("Indicator Size"),
							     P_("Size of check or radio indicator"),
							     0,
							     G_MAXINT,
							     INDICATOR_SIZE,
							     BTK_PARAM_READABLE));
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("indicator-spacing",
							     P_("Indicator Spacing"),
							     P_("Spacing around check or radio indicator"),
							     0,
							     G_MAXINT,
							     INDICATOR_SPACING,
							     BTK_PARAM_READABLE));
}

static void
btk_check_button_init (BtkCheckButton *check_button)
{
  btk_widget_set_has_window (BTK_WIDGET (check_button), FALSE);
  btk_widget_set_receives_default (BTK_WIDGET (check_button), FALSE);
  BTK_TOGGLE_BUTTON (check_button)->draw_indicator = TRUE;
  BTK_BUTTON (check_button)->depress_on_activate = FALSE;
}

BtkWidget*
btk_check_button_new (void)
{
  return g_object_new (BTK_TYPE_CHECK_BUTTON, NULL);
}


BtkWidget*
btk_check_button_new_with_label (const gchar *label)
{
  return g_object_new (BTK_TYPE_CHECK_BUTTON, "label", label, NULL);
}

/**
 * btk_check_button_new_with_mnemonic:
 * @label: The text of the button, with an underscore in front of the
 *         mnemonic character
 * @returns: a new #BtkCheckButton
 *
 * Creates a new #BtkCheckButton containing a label. The label
 * will be created using btk_label_new_with_mnemonic(), so underscores
 * in @label indicate the mnemonic for the check button.
 **/
BtkWidget*
btk_check_button_new_with_mnemonic (const gchar *label)
{
  return g_object_new (BTK_TYPE_CHECK_BUTTON, 
                       "label", label, 
                       "use-underline", TRUE, 
                       NULL);
}


/* This should only be called when toggle_button->draw_indicator
 * is true.
 */
static void
btk_check_button_paint (BtkWidget    *widget,
			BdkRectangle *area)
{
  BtkCheckButton *check_button = BTK_CHECK_BUTTON (widget);
  
  if (btk_widget_is_drawable (widget))
    {
      gint border_width;
      gint interior_focus;
      gint focus_width;
      gint focus_pad;
	  
      btk_widget_style_get (widget,
			    "interior-focus", &interior_focus,
			    "focus-line-width", &focus_width,
			    "focus-padding", &focus_pad,
			    NULL);

      btk_check_button_draw_indicator (check_button, area);
      
      border_width = BTK_CONTAINER (widget)->border_width;
      if (btk_widget_has_focus (widget))
	{
	  BtkWidget *child = BTK_BIN (widget)->child;
	  
	  if (interior_focus && child && btk_widget_get_visible (child))
	    btk_paint_focus (widget->style, widget->window, btk_widget_get_state (widget),
			     area, widget, "checkbutton",
			     child->allocation.x - focus_width - focus_pad,
			     child->allocation.y - focus_width - focus_pad,
			     child->allocation.width + 2 * (focus_width + focus_pad),
			     child->allocation.height + 2 * (focus_width + focus_pad));
	  else
	    btk_paint_focus (widget->style, widget->window, btk_widget_get_state (widget),
			     area, widget, "checkbutton",
			     border_width + widget->allocation.x,
			     border_width + widget->allocation.y,
			     widget->allocation.width - 2 * border_width,
			     widget->allocation.height - 2 * border_width);
	}
    }
}

void
_btk_check_button_get_props (BtkCheckButton *check_button,
			     gint           *indicator_size,
			     gint           *indicator_spacing)
{
  BtkWidget *widget =  BTK_WIDGET (check_button);

  if (indicator_size)
    btk_widget_style_get (widget, "indicator-size", indicator_size, NULL);

  if (indicator_spacing)
    btk_widget_style_get (widget, "indicator-spacing", indicator_spacing, NULL);
}

static void
btk_check_button_size_request (BtkWidget      *widget,
			       BtkRequisition *requisition)
{
  BtkToggleButton *toggle_button = BTK_TOGGLE_BUTTON (widget);
  
  if (toggle_button->draw_indicator)
    {
      BtkWidget *child;
      gint temp;
      gint indicator_size;
      gint indicator_spacing;
      gint border_width = BTK_CONTAINER (widget)->border_width;
      gint focus_width;
      gint focus_pad;

      btk_widget_style_get (BTK_WIDGET (widget),
			    "focus-line-width", &focus_width,
			    "focus-padding", &focus_pad,
			    NULL);
 
      requisition->width = border_width * 2;
      requisition->height = border_width * 2;

      _btk_check_button_get_props (BTK_CHECK_BUTTON (widget),
 				   &indicator_size, &indicator_spacing);
      
      child = BTK_BIN (widget)->child;
      if (child && btk_widget_get_visible (child))
	{
	  BtkRequisition child_requisition;
	  
	  btk_widget_size_request (child, &child_requisition);

	  requisition->width += child_requisition.width + indicator_spacing;
	  requisition->height += child_requisition.height;
	}
      
      requisition->width += (indicator_size + indicator_spacing * 2 + 2 * (focus_width + focus_pad));
      
      temp = indicator_size + indicator_spacing * 2;
      requisition->height = MAX (requisition->height, temp) + 2 * (focus_width + focus_pad);
    }
  else
    BTK_WIDGET_CLASS (btk_check_button_parent_class)->size_request (widget, requisition);
}

static void
btk_check_button_size_allocate (BtkWidget     *widget,
				BtkAllocation *allocation)
{
  BtkCheckButton *check_button;
  BtkToggleButton *toggle_button;
  BtkButton *button;
  BtkAllocation child_allocation;

  button = BTK_BUTTON (widget);
  check_button = BTK_CHECK_BUTTON (widget);
  toggle_button = BTK_TOGGLE_BUTTON (widget);

  if (toggle_button->draw_indicator)
    {
      gint indicator_size;
      gint indicator_spacing;
      gint focus_width;
      gint focus_pad;
      
      _btk_check_button_get_props (check_button, &indicator_size, &indicator_spacing);
      btk_widget_style_get (widget,
			    "focus-line-width", &focus_width,
			    "focus-padding", &focus_pad,
			    NULL);
						    
      widget->allocation = *allocation;
      if (btk_widget_get_realized (widget))
	bdk_window_move_resize (button->event_window,
				allocation->x, allocation->y,
				allocation->width, allocation->height);
      
      if (BTK_BIN (button)->child && btk_widget_get_visible (BTK_BIN (button)->child))
	{
	  BtkRequisition child_requisition;
 	  gint border_width = BTK_CONTAINER (widget)->border_width;

	  btk_widget_get_child_requisition (BTK_BIN (button)->child, &child_requisition);
 
	  child_allocation.width = MIN (child_requisition.width,
					allocation->width -
					((border_width + focus_width + focus_pad) * 2
					 + indicator_size + indicator_spacing * 3));
	  child_allocation.width = MAX (child_allocation.width, 1);

	  child_allocation.height = MIN (child_requisition.height,
					 allocation->height - (border_width + focus_width + focus_pad) * 2);
	  child_allocation.height = MAX (child_allocation.height, 1);
	  
	  child_allocation.x = (border_width + indicator_size + indicator_spacing * 3 +
				widget->allocation.x + focus_width + focus_pad);
	  child_allocation.y = widget->allocation.y +
		  (allocation->height - child_allocation.height) / 2;

	  if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL)
	    child_allocation.x = allocation->x + allocation->width
	      - (child_allocation.x - allocation->x + child_allocation.width);
	  
	  btk_widget_size_allocate (BTK_BIN (button)->child, &child_allocation);
	}
    }
  else
    BTK_WIDGET_CLASS (btk_check_button_parent_class)->size_allocate (widget, allocation);
}

static gint
btk_check_button_expose (BtkWidget      *widget,
			 BdkEventExpose *event)
{
  BtkToggleButton *toggle_button;
  BtkBin *bin;
  
  toggle_button = BTK_TOGGLE_BUTTON (widget);
  bin = BTK_BIN (widget);
  
  if (btk_widget_is_drawable (widget))
    {
      if (toggle_button->draw_indicator)
	{
	  btk_check_button_paint (widget, &event->area);
	  
	  if (bin->child)
	    btk_container_propagate_expose (BTK_CONTAINER (widget),
					    bin->child,
					    event);
	}
      else if (BTK_WIDGET_CLASS (btk_check_button_parent_class)->expose_event)
	BTK_WIDGET_CLASS (btk_check_button_parent_class)->expose_event (widget, event);
    }
  
  return FALSE;
}


static void
btk_check_button_draw_indicator (BtkCheckButton *check_button,
				 BdkRectangle   *area)
{
  BtkCheckButtonClass *class;
  
  g_return_if_fail (BTK_IS_CHECK_BUTTON (check_button));
  
  class = BTK_CHECK_BUTTON_GET_CLASS (check_button);

  if (class->draw_indicator)
    class->draw_indicator (check_button, area);
}

static void
btk_real_check_button_draw_indicator (BtkCheckButton *check_button,
				      BdkRectangle   *area)
{
  BtkWidget *widget;
  BtkWidget *child;
  BtkButton *button;
  BtkToggleButton *toggle_button;
  BtkStateType state_type;
  BtkShadowType shadow_type;
  gint x, y;
  gint indicator_size;
  gint indicator_spacing;
  gint focus_width;
  gint focus_pad;
  gboolean interior_focus;

  widget = BTK_WIDGET (check_button);

  if (btk_widget_is_drawable (widget))
    {
      button = BTK_BUTTON (check_button);
      toggle_button = BTK_TOGGLE_BUTTON (check_button);
  
      btk_widget_style_get (widget, 
			    "interior-focus", &interior_focus,
			    "focus-line-width", &focus_width, 
			    "focus-padding", &focus_pad, 
			    NULL);

      _btk_check_button_get_props (check_button, &indicator_size, &indicator_spacing);

      x = widget->allocation.x + indicator_spacing + BTK_CONTAINER (widget)->border_width;
      y = widget->allocation.y + (widget->allocation.height - indicator_size) / 2;

      child = BTK_BIN (check_button)->child;
      if (!interior_focus || !(child && btk_widget_get_visible (child)))
	x += focus_width + focus_pad;      

      if (toggle_button->inconsistent)
	shadow_type = BTK_SHADOW_ETCHED_IN;
      else if (toggle_button->active)
	shadow_type = BTK_SHADOW_IN;
      else
	shadow_type = BTK_SHADOW_OUT;

      if (button->activate_timeout || (button->button_down && button->in_button))
	state_type = BTK_STATE_ACTIVE;
      else if (button->in_button)
	state_type = BTK_STATE_PRELIGHT;
      else if (!btk_widget_is_sensitive (widget))
	state_type = BTK_STATE_INSENSITIVE;
      else
	state_type = BTK_STATE_NORMAL;
      
      if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL)
	x = widget->allocation.x + widget->allocation.width - (indicator_size + x - widget->allocation.x);

      if (btk_widget_get_state (widget) == BTK_STATE_PRELIGHT)
	{
	  BdkRectangle restrict_area;
	  BdkRectangle new_area;
	      
	  restrict_area.x = widget->allocation.x + BTK_CONTAINER (widget)->border_width;
	  restrict_area.y = widget->allocation.y + BTK_CONTAINER (widget)->border_width;
	  restrict_area.width = widget->allocation.width - (2 * BTK_CONTAINER (widget)->border_width);
	  restrict_area.height = widget->allocation.height - (2 * BTK_CONTAINER (widget)->border_width);
	  
	  if (bdk_rectangle_intersect (area, &restrict_area, &new_area))
	    {
	      btk_paint_flat_box (widget->style, widget->window, BTK_STATE_PRELIGHT,
				  BTK_SHADOW_ETCHED_OUT, 
				  area, widget, "checkbutton",
				  new_area.x, new_area.y,
				  new_area.width, new_area.height);
	    }
	}

      btk_paint_check (widget->style, widget->window,
		       state_type, shadow_type,
		       area, widget, "checkbutton",
		       x, y, indicator_size, indicator_size);
    }
}

#define __BTK_CHECK_BUTTON_C__
#include "btkaliasdef.c"
