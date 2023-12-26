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
#include "btkviewport.h"
#include "btkintl.h"
#include "btkmarshalers.h"
#include "btkprivate.h"
#include "btkalias.h"

/**
 * SECTION:btkviewport
 * @Short_description: An adapter which makes widgets scrollable
 * @Title: BtkViewport
 * @See_also:#BtkScrolledWindow, #BtkAdjustment
 *
 * The #BtkViewport widget acts as an adaptor class, implementing
 * scrollability for child widgets that lack their own scrolling
 * capabilities. Use #BtkViewport to scroll child widgets such as
 * #BtkTable, #BtkBox, and so on.
 *
 * If a widget has native scrolling abilities, such as #BtkTextView,
 * #BtkTreeView or #BtkIconview, it can be added to a #BtkScrolledWindow
 * with btk_container_add(). If a widget does not, you must first add the
 * widget to a #BtkViewport, then add the viewport to the scrolled window.
 * The convenience function btk_scrolled_window_add_with_viewport() does
 * exactly this, so you can ignore the presence of the viewport.
 */

enum {
  PROP_0,
  PROP_HADJUSTMENT,
  PROP_VADJUSTMENT,
  PROP_SHADOW_TYPE
};


static void btk_viewport_finalize                 (BObject          *object);
static void btk_viewport_destroy                  (BtkObject        *object);
static void btk_viewport_set_property             (BObject         *object,
						   buint            prop_id,
						   const BValue    *value,
						   BParamSpec      *pspec);
static void btk_viewport_get_property             (BObject         *object,
						   buint            prop_id,
						   BValue          *value,
						   BParamSpec      *pspec);
static void btk_viewport_set_scroll_adjustments	  (BtkViewport	    *viewport,
						   BtkAdjustment    *hadjustment,
						   BtkAdjustment    *vadjustment);
static void btk_viewport_realize                  (BtkWidget        *widget);
static void btk_viewport_unrealize                (BtkWidget        *widget);
static void btk_viewport_paint                    (BtkWidget        *widget,
						   BdkRectangle     *area);
static bint btk_viewport_expose                   (BtkWidget        *widget,
						   BdkEventExpose   *event);
static void btk_viewport_add                      (BtkContainer     *container,
						   BtkWidget        *widget);
static void btk_viewport_size_request             (BtkWidget        *widget,
						   BtkRequisition   *requisition);
static void btk_viewport_size_allocate            (BtkWidget        *widget,
						   BtkAllocation    *allocation);
static void btk_viewport_adjustment_value_changed (BtkAdjustment    *adjustment,
						   bpointer          data);
static void btk_viewport_style_set                (BtkWidget *widget,
			                           BtkStyle  *previous_style);

G_DEFINE_TYPE (BtkViewport, btk_viewport, BTK_TYPE_BIN)

static void
btk_viewport_class_init (BtkViewportClass *class)
{
  BtkObjectClass *object_class;
  BObjectClass   *bobject_class;
  BtkWidgetClass *widget_class;
  BtkContainerClass *container_class;

  object_class = (BtkObjectClass*) class;
  bobject_class = B_OBJECT_CLASS (class);
  widget_class = (BtkWidgetClass*) class;
  container_class = (BtkContainerClass*) class;

  bobject_class->finalize = btk_viewport_finalize;
  bobject_class->set_property = btk_viewport_set_property;
  bobject_class->get_property = btk_viewport_get_property;
  object_class->destroy = btk_viewport_destroy;
  
  widget_class->realize = btk_viewport_realize;
  widget_class->unrealize = btk_viewport_unrealize;
  widget_class->expose_event = btk_viewport_expose;
  widget_class->size_request = btk_viewport_size_request;
  widget_class->size_allocate = btk_viewport_size_allocate;
  widget_class->style_set = btk_viewport_style_set;
  
  container_class->add = btk_viewport_add;

  class->set_scroll_adjustments = btk_viewport_set_scroll_adjustments;

  g_object_class_install_property (bobject_class,
                                   PROP_HADJUSTMENT,
                                   g_param_spec_object ("hadjustment",
							P_("Horizontal adjustment"),
							P_("The BtkAdjustment that determines the values of the horizontal position for this viewport"),
                                                        BTK_TYPE_ADJUSTMENT,
                                                        BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (bobject_class,
                                   PROP_VADJUSTMENT,
                                   g_param_spec_object ("vadjustment",
							P_("Vertical adjustment"),
							P_("The BtkAdjustment that determines the values of the vertical position for this viewport"),
                                                        BTK_TYPE_ADJUSTMENT,
                                                        BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (bobject_class,
                                   PROP_SHADOW_TYPE,
                                   g_param_spec_enum ("shadow-type",
						      P_("Shadow type"),
						      P_("Determines how the shadowed box around the viewport is drawn"),
						      BTK_TYPE_SHADOW_TYPE,
						      BTK_SHADOW_IN,
						      BTK_PARAM_READWRITE));

  /**
   * BtkViewport::set-scroll-adjustments
   * @horizontal: the horizontal #BtkAdjustment
   * @vertical: the vertical #BtkAdjustment
   *
   * Set the scroll adjustments for the viewport. Usually scrolled containers
   * like #BtkScrolledWindow will emit this signal to connect two instances
   * of #BtkScrollbar to the scroll directions of the #BtkViewport.
   */
  widget_class->set_scroll_adjustments_signal =
    g_signal_new (I_("set-scroll-adjustments"),
		  B_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkViewportClass, set_scroll_adjustments),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT_OBJECT,
		  B_TYPE_NONE, 2,
		  BTK_TYPE_ADJUSTMENT,
		  BTK_TYPE_ADJUSTMENT);
}

static void
btk_viewport_set_property (BObject         *object,
			   buint            prop_id,
			   const BValue    *value,
			   BParamSpec      *pspec)
{
  BtkViewport *viewport;

  viewport = BTK_VIEWPORT (object);

  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      btk_viewport_set_hadjustment (viewport, b_value_get_object (value));
      break;
    case PROP_VADJUSTMENT:
      btk_viewport_set_vadjustment (viewport, b_value_get_object (value));
      break;
    case PROP_SHADOW_TYPE:
      btk_viewport_set_shadow_type (viewport, b_value_get_enum (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_viewport_get_property (BObject         *object,
			   buint            prop_id,
			   BValue          *value,
			   BParamSpec      *pspec)
{
  BtkViewport *viewport;

  viewport = BTK_VIEWPORT (object);

  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      b_value_set_object (value, viewport->hadjustment);
      break;
    case PROP_VADJUSTMENT:
      b_value_set_object (value, viewport->vadjustment);
      break;
    case PROP_SHADOW_TYPE:
      b_value_set_enum (value, viewport->shadow_type);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_viewport_init (BtkViewport *viewport)
{
  btk_widget_set_has_window (BTK_WIDGET (viewport), TRUE);

  btk_widget_set_redraw_on_allocate (BTK_WIDGET (viewport), FALSE);
  btk_container_set_resize_mode (BTK_CONTAINER (viewport), BTK_RESIZE_QUEUE);
  
  viewport->shadow_type = BTK_SHADOW_IN;
  viewport->view_window = NULL;
  viewport->bin_window = NULL;
  viewport->hadjustment = NULL;
  viewport->vadjustment = NULL;
}

/**
 * btk_viewport_new:
 * @hadjustment: horizontal adjustment.
 * @vadjustment: vertical adjustment.
 * @returns: a new #BtkViewport.
 *
 * Creates a new #BtkViewport with the given adjustments.
 *
 **/
BtkWidget*
btk_viewport_new (BtkAdjustment *hadjustment,
		  BtkAdjustment *vadjustment)
{
  BtkWidget *viewport;

  viewport = g_object_new (BTK_TYPE_VIEWPORT,
			     "hadjustment", hadjustment,
			     "vadjustment", vadjustment,
			     NULL);

  return viewport;
}

#define ADJUSTMENT_POINTER(viewport, orientation)         \
  (((orientation) == BTK_ORIENTATION_HORIZONTAL) ?        \
     &(viewport)->hadjustment : &(viewport)->vadjustment)

static void
viewport_disconnect_adjustment (BtkViewport    *viewport,
				BtkOrientation  orientation)
{
  BtkAdjustment **adjustmentp = ADJUSTMENT_POINTER (viewport, orientation);

  if (*adjustmentp)
    {
      g_signal_handlers_disconnect_by_func (*adjustmentp,
					    btk_viewport_adjustment_value_changed,
					    viewport);
      g_object_unref (*adjustmentp);
      *adjustmentp = NULL;
    }
}

static void
btk_viewport_finalize (BObject *object)
{
  BtkViewport *viewport = BTK_VIEWPORT (object);

  viewport_disconnect_adjustment (viewport, BTK_ORIENTATION_HORIZONTAL);
  viewport_disconnect_adjustment (viewport, BTK_ORIENTATION_VERTICAL);

  B_OBJECT_CLASS (btk_viewport_parent_class)->finalize (object);
}

static void
btk_viewport_destroy (BtkObject *object)
{
  BtkViewport *viewport = BTK_VIEWPORT (object);

  viewport_disconnect_adjustment (viewport, BTK_ORIENTATION_HORIZONTAL);
  viewport_disconnect_adjustment (viewport, BTK_ORIENTATION_VERTICAL);

  BTK_OBJECT_CLASS (btk_viewport_parent_class)->destroy (object);
}

/**
 * btk_viewport_get_hadjustment:
 * @viewport: a #BtkViewport.
 *
 * Returns the horizontal adjustment of the viewport.
 *
 * Return value: (transfer none): the horizontal adjustment of @viewport.
 **/
BtkAdjustment*
btk_viewport_get_hadjustment (BtkViewport *viewport)
{
  g_return_val_if_fail (BTK_IS_VIEWPORT (viewport), NULL);

  if (!viewport->hadjustment)
    btk_viewport_set_hadjustment (viewport, NULL);

  return viewport->hadjustment;
}

/**
 * btk_viewport_get_vadjustment:
 * @viewport: a #BtkViewport.
 * 
 * Returns the vertical adjustment of the viewport.
 *
 * Return value: (transfer none): the vertical adjustment of @viewport.
 **/
BtkAdjustment*
btk_viewport_get_vadjustment (BtkViewport *viewport)
{
  g_return_val_if_fail (BTK_IS_VIEWPORT (viewport), NULL);

  if (!viewport->vadjustment)
    btk_viewport_set_vadjustment (viewport, NULL);

  return viewport->vadjustment;
}

static void
viewport_get_view_allocation (BtkViewport   *viewport,
			      BtkAllocation *view_allocation)
{
  BtkWidget *widget = BTK_WIDGET (viewport);
  BtkAllocation *allocation = &widget->allocation;
  bint border_width = BTK_CONTAINER (viewport)->border_width;
  
  view_allocation->x = 0;
  view_allocation->y = 0;

  if (viewport->shadow_type != BTK_SHADOW_NONE)
    {
      view_allocation->x = widget->style->xthickness;
      view_allocation->y = widget->style->ythickness;
    }

  view_allocation->width = MAX (1, allocation->width - view_allocation->x * 2 - border_width * 2);
  view_allocation->height = MAX (1, allocation->height - view_allocation->y * 2 - border_width * 2);
}

static void
viewport_reclamp_adjustment (BtkAdjustment *adjustment,
			     bboolean      *value_changed)
{
  bdouble value = adjustment->value;
  
  value = CLAMP (value, 0, adjustment->upper - adjustment->page_size);
  if (value != adjustment->value)
    {
      adjustment->value = value;
      if (value_changed)
	*value_changed = TRUE;
    }
  else if (value_changed)
    *value_changed = FALSE;
}

static void
viewport_set_hadjustment_values (BtkViewport *viewport,
				 bboolean    *value_changed)
{
  BtkBin *bin = BTK_BIN (viewport);
  BtkAllocation view_allocation;
  BtkAdjustment *hadjustment = btk_viewport_get_hadjustment (viewport);
  bdouble old_page_size;
  bdouble old_upper;
  bdouble old_value;
  
  viewport_get_view_allocation (viewport, &view_allocation);  

  old_page_size = hadjustment->page_size;
  old_upper = hadjustment->upper;
  old_value = hadjustment->value;
  hadjustment->page_size = view_allocation.width;
  hadjustment->step_increment = view_allocation.width * 0.1;
  hadjustment->page_increment = view_allocation.width * 0.9;
  
  hadjustment->lower = 0;

  if (bin->child && btk_widget_get_visible (bin->child))
    {
      BtkRequisition child_requisition;
      
      btk_widget_get_child_requisition (bin->child, &child_requisition);
      hadjustment->upper = MAX (child_requisition.width, view_allocation.width);
    }
  else
    hadjustment->upper = view_allocation.width;

  if (btk_widget_get_direction (BTK_WIDGET (viewport)) == BTK_TEXT_DIR_RTL) 
    {
      bdouble dist = old_upper - (old_value + old_page_size);
      hadjustment->value = hadjustment->upper - dist - hadjustment->page_size;
      viewport_reclamp_adjustment (hadjustment, value_changed);
      *value_changed = (old_value != hadjustment->value);
    }
  else
    viewport_reclamp_adjustment (hadjustment, value_changed);
}

static void
viewport_set_vadjustment_values (BtkViewport *viewport,
				 bboolean    *value_changed)
{
  BtkBin *bin = BTK_BIN (viewport);
  BtkAllocation view_allocation;
  BtkAdjustment *vadjustment = btk_viewport_get_vadjustment (viewport);

  viewport_get_view_allocation (viewport, &view_allocation);  

  vadjustment->page_size = view_allocation.height;
  vadjustment->step_increment = view_allocation.height * 0.1;
  vadjustment->page_increment = view_allocation.height * 0.9;
  
  vadjustment->lower = 0;

  if (bin->child && btk_widget_get_visible (bin->child))
    {
      BtkRequisition child_requisition;
      
      btk_widget_get_child_requisition (bin->child, &child_requisition);
      vadjustment->upper = MAX (child_requisition.height, view_allocation.height);
    }
  else
    vadjustment->upper = view_allocation.height;

  viewport_reclamp_adjustment (vadjustment, value_changed);
}

static void
viewport_set_adjustment (BtkViewport    *viewport,
			 BtkOrientation  orientation,
			 BtkAdjustment  *adjustment)
{
  BtkAdjustment **adjustmentp = ADJUSTMENT_POINTER (viewport, orientation);
  bboolean value_changed;

  if (adjustment && adjustment == *adjustmentp)
    return;

  if (!adjustment)
    adjustment = BTK_ADJUSTMENT (btk_adjustment_new (0.0, 0.0, 0.0,
						     0.0, 0.0, 0.0));
  viewport_disconnect_adjustment (viewport, orientation);
  *adjustmentp = adjustment;
  g_object_ref_sink (adjustment);

  if (orientation == BTK_ORIENTATION_HORIZONTAL)
    viewport_set_hadjustment_values (viewport, &value_changed);
  else
    viewport_set_vadjustment_values (viewport, &value_changed);

  g_signal_connect (adjustment, "value-changed",
		    G_CALLBACK (btk_viewport_adjustment_value_changed),
		    viewport);

  btk_adjustment_changed (adjustment);
  
  if (value_changed)
    btk_adjustment_value_changed (adjustment);
  else
    btk_viewport_adjustment_value_changed (adjustment, viewport);
}

/**
 * btk_viewport_set_hadjustment:
 * @viewport: a #BtkViewport.
 * @adjustment: (allow-none): a #BtkAdjustment.
 *
 * Sets the horizontal adjustment of the viewport.
 **/
void
btk_viewport_set_hadjustment (BtkViewport   *viewport,
			      BtkAdjustment *adjustment)
{
  g_return_if_fail (BTK_IS_VIEWPORT (viewport));
  if (adjustment)
    g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));

  viewport_set_adjustment (viewport, BTK_ORIENTATION_HORIZONTAL, adjustment);

  g_object_notify (B_OBJECT (viewport), "hadjustment");
}

/**
 * btk_viewport_set_vadjustment:
 * @viewport: a #BtkViewport.
 * @adjustment: (allow-none): a #BtkAdjustment.
 *
 * Sets the vertical adjustment of the viewport.
 **/
void
btk_viewport_set_vadjustment (BtkViewport   *viewport,
			      BtkAdjustment *adjustment)
{
  g_return_if_fail (BTK_IS_VIEWPORT (viewport));
  if (adjustment)
    g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));

  viewport_set_adjustment (viewport, BTK_ORIENTATION_VERTICAL, adjustment);

  g_object_notify (B_OBJECT (viewport), "vadjustment");
}

static void
btk_viewport_set_scroll_adjustments (BtkViewport      *viewport,
				     BtkAdjustment    *hadjustment,
				     BtkAdjustment    *vadjustment)
{
  btk_viewport_set_hadjustment (viewport, hadjustment);
  btk_viewport_set_vadjustment (viewport, vadjustment);
}

/** 
 * btk_viewport_set_shadow_type:
 * @viewport: a #BtkViewport.
 * @type: the new shadow type.
 *
 * Sets the shadow type of the viewport.
 **/ 
void
btk_viewport_set_shadow_type (BtkViewport   *viewport,
			      BtkShadowType  type)
{
  g_return_if_fail (BTK_IS_VIEWPORT (viewport));

  if ((BtkShadowType) viewport->shadow_type != type)
    {
      viewport->shadow_type = type;

      if (btk_widget_get_visible (BTK_WIDGET (viewport)))
	{
	  btk_widget_size_allocate (BTK_WIDGET (viewport), &(BTK_WIDGET (viewport)->allocation));
	  btk_widget_queue_draw (BTK_WIDGET (viewport));
	}

      g_object_notify (B_OBJECT (viewport), "shadow-type");
    }
}

/**
 * btk_viewport_get_shadow_type:
 * @viewport: a #BtkViewport
 *
 * Gets the shadow type of the #BtkViewport. See
 * btk_viewport_set_shadow_type().
 
 * Return value: the shadow type 
 **/
BtkShadowType
btk_viewport_get_shadow_type (BtkViewport *viewport)
{
  g_return_val_if_fail (BTK_IS_VIEWPORT (viewport), BTK_SHADOW_NONE);

  return viewport->shadow_type;
}

/**
 * btk_viewport_get_bin_window:
 * @viewport: a #BtkViewport
 *
 * Gets the bin window of the #BtkViewport.
 *
 * Return value: (transfer none): a #BdkWindow
 *
 * Since: 2.20
 **/
BdkWindow*
btk_viewport_get_bin_window (BtkViewport *viewport)
{
  g_return_val_if_fail (BTK_IS_VIEWPORT (viewport), NULL);

  return viewport->bin_window;
}

/**
 * btk_viewport_get_view_window:
 * @viewport: a #BtkViewport
 *
 * Gets the view window of the #BtkViewport.
 *
 * Return value: (transfer none): a #BdkWindow
 *
 * Since: 2.22
 **/
BdkWindow*
btk_viewport_get_view_window (BtkViewport *viewport)
{
  g_return_val_if_fail (BTK_IS_VIEWPORT (viewport), NULL);

  return viewport->view_window;
}

static void
btk_viewport_realize (BtkWidget *widget)
{
  BtkViewport *viewport = BTK_VIEWPORT (widget);
  BtkBin *bin = BTK_BIN (widget);
  BtkAdjustment *hadjustment = btk_viewport_get_hadjustment (viewport);
  BtkAdjustment *vadjustment = btk_viewport_get_vadjustment (viewport);
  bint border_width = BTK_CONTAINER (widget)->border_width;
  
  BtkAllocation view_allocation;
  BdkWindowAttr attributes;
  bint attributes_mask;
  bint event_mask;

  btk_widget_set_realized (widget, TRUE);

  attributes.x = widget->allocation.x + border_width;
  attributes.y = widget->allocation.y + border_width;
  attributes.width = widget->allocation.width - border_width * 2;
  attributes.height = widget->allocation.height - border_width * 2;
  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);

  event_mask = btk_widget_get_events (widget) | BDK_EXPOSURE_MASK;
  /* We select on button_press_mask so that button 4-5 scrolls are trapped.
   */
  attributes.event_mask = event_mask | BDK_BUTTON_PRESS_MASK;

  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;

  widget->window = bdk_window_new (btk_widget_get_parent_window (widget),
				   &attributes, attributes_mask);
  bdk_window_set_user_data (widget->window, viewport);

  viewport_get_view_allocation (viewport, &view_allocation);
  
  attributes.x = view_allocation.x;
  attributes.y = view_allocation.y;
  attributes.width = view_allocation.width;
  attributes.height = view_allocation.height;
  attributes.event_mask = 0;

  viewport->view_window = bdk_window_new (widget->window, &attributes, attributes_mask);
  bdk_window_set_user_data (viewport->view_window, viewport);

  bdk_window_set_back_pixmap (viewport->view_window, NULL, FALSE);
  
  attributes.x = - hadjustment->value;
  attributes.y = - vadjustment->value;
  attributes.width = hadjustment->upper;
  attributes.height = vadjustment->upper;
  
  attributes.event_mask = event_mask;

  viewport->bin_window = bdk_window_new (viewport->view_window, &attributes, attributes_mask);
  bdk_window_set_user_data (viewport->bin_window, viewport);

  if (bin->child)
    btk_widget_set_parent_window (bin->child, viewport->bin_window);

  widget->style = btk_style_attach (widget->style, widget->window);
  btk_style_set_background (widget->style, widget->window, BTK_STATE_NORMAL);
  btk_style_set_background (widget->style, viewport->bin_window, BTK_STATE_NORMAL);

  /* Call paint here to allow a theme to set the background without flashing
   */
  btk_paint_flat_box(widget->style, viewport->bin_window, BTK_STATE_NORMAL,
		     BTK_SHADOW_NONE,
		     NULL, widget, "viewportbin",
		     0, 0, -1, -1);
   
  bdk_window_show (viewport->bin_window);
  bdk_window_show (viewport->view_window);
}

static void
btk_viewport_unrealize (BtkWidget *widget)
{
  BtkViewport *viewport = BTK_VIEWPORT (widget);

  bdk_window_set_user_data (viewport->view_window, NULL);
  bdk_window_destroy (viewport->view_window);
  viewport->view_window = NULL;

  bdk_window_set_user_data (viewport->bin_window, NULL);
  bdk_window_destroy (viewport->bin_window);
  viewport->bin_window = NULL;

  BTK_WIDGET_CLASS (btk_viewport_parent_class)->unrealize (widget);
}

static void
btk_viewport_paint (BtkWidget    *widget,
		    BdkRectangle *area)
{
  if (btk_widget_is_drawable (widget))
    {
      BtkViewport *viewport = BTK_VIEWPORT (widget);

      btk_paint_shadow (widget->style, widget->window,
			BTK_STATE_NORMAL, viewport->shadow_type,
			area, widget, "viewport",
			0, 0, -1, -1);
    }
}

static bint
btk_viewport_expose (BtkWidget      *widget,
		     BdkEventExpose *event)
{
  BtkViewport *viewport;

  if (btk_widget_is_drawable (widget))
    {
      viewport = BTK_VIEWPORT (widget);

      if (event->window == widget->window)
	btk_viewport_paint (widget, &event->area);
      else if (event->window == viewport->bin_window)
	{
	  btk_paint_flat_box(widget->style, viewport->bin_window, 
			     BTK_STATE_NORMAL, BTK_SHADOW_NONE,
			     &event->area, widget, "viewportbin",
			     0, 0, -1, -1);

	  BTK_WIDGET_CLASS (btk_viewport_parent_class)->expose_event (widget, event);
	}
    }

  return FALSE;
}

static void
btk_viewport_add (BtkContainer *container,
		  BtkWidget    *child)
{
  BtkBin *bin = BTK_BIN (container);

  g_return_if_fail (bin->child == NULL);

  btk_widget_set_parent_window (child, BTK_VIEWPORT (bin)->bin_window);

  BTK_CONTAINER_CLASS (btk_viewport_parent_class)->add (container, child);
}

static void
btk_viewport_size_request (BtkWidget      *widget,
			   BtkRequisition *requisition)
{
  BtkBin *bin = BTK_BIN (widget);
  BtkRequisition child_requisition;

  requisition->width = BTK_CONTAINER (widget)->border_width;

  requisition->height = BTK_CONTAINER (widget)->border_width;

  if (BTK_VIEWPORT (widget)->shadow_type != BTK_SHADOW_NONE)
    {
      requisition->width += 2 * widget->style->xthickness;
      requisition->height += 2 * widget->style->ythickness;
    }

  if (bin->child && btk_widget_get_visible (bin->child))
    {
      btk_widget_size_request (bin->child, &child_requisition);
      requisition->width += child_requisition.width;
      requisition->height += child_requisition.height;
    }
}

static void
btk_viewport_size_allocate (BtkWidget     *widget,
			    BtkAllocation *allocation)
{
  BtkViewport *viewport = BTK_VIEWPORT (widget);
  BtkBin *bin = BTK_BIN (widget);
  bint border_width = BTK_CONTAINER (widget)->border_width;
  bboolean hadjustment_value_changed, vadjustment_value_changed;
  BtkAdjustment *hadjustment = btk_viewport_get_hadjustment (viewport);
  BtkAdjustment *vadjustment = btk_viewport_get_vadjustment (viewport);
  BtkAllocation child_allocation;

  /* If our size changed, and we have a shadow, queue a redraw on widget->window to
   * redraw the shadow correctly.
   */
  if (btk_widget_get_mapped (widget) &&
      viewport->shadow_type != BTK_SHADOW_NONE &&
      (widget->allocation.width != allocation->width ||
       widget->allocation.height != allocation->height))
    bdk_window_invalidate_rect (widget->window, NULL, FALSE);
  
  widget->allocation = *allocation;
  
  viewport_set_hadjustment_values (viewport, &hadjustment_value_changed);
  viewport_set_vadjustment_values (viewport, &vadjustment_value_changed);
  
  child_allocation.x = 0;
  child_allocation.y = 0;
  child_allocation.width = hadjustment->upper;
  child_allocation.height = vadjustment->upper;
  if (btk_widget_get_realized (widget))
    {
      BtkAllocation view_allocation;
      bdk_window_move_resize (widget->window,
			      allocation->x + border_width,
			      allocation->y + border_width,
			      allocation->width - border_width * 2,
			      allocation->height - border_width * 2);
      
      viewport_get_view_allocation (viewport, &view_allocation);
      bdk_window_move_resize (viewport->view_window,
			      view_allocation.x,
			      view_allocation.y,
			      view_allocation.width,
			      view_allocation.height);
      bdk_window_move_resize (viewport->bin_window,
                              - hadjustment->value,
                              - vadjustment->value,
                              child_allocation.width,
                              child_allocation.height);
    }
  if (bin->child && btk_widget_get_visible (bin->child))
    btk_widget_size_allocate (bin->child, &child_allocation);

  btk_adjustment_changed (hadjustment);
  btk_adjustment_changed (vadjustment);
  if (hadjustment_value_changed)
    btk_adjustment_value_changed (hadjustment);
  if (vadjustment_value_changed)
    btk_adjustment_value_changed (vadjustment);
}

static void
btk_viewport_adjustment_value_changed (BtkAdjustment *adjustment,
				       bpointer       data)
{
  BtkViewport *viewport = BTK_VIEWPORT (data);
  BtkBin *bin = BTK_BIN (data);

  if (bin->child && btk_widget_get_visible (bin->child) &&
      btk_widget_get_realized (BTK_WIDGET (viewport)))
    {
      BtkAdjustment *hadjustment = btk_viewport_get_hadjustment (viewport);
      BtkAdjustment *vadjustment = btk_viewport_get_vadjustment (viewport);
      bint old_x, old_y;
      bint new_x, new_y;

      bdk_window_get_position (viewport->bin_window, &old_x, &old_y);
      new_x = - hadjustment->value;
      new_y = - vadjustment->value;

      if (new_x != old_x || new_y != old_y)
	{
	  bdk_window_move (viewport->bin_window, new_x, new_y);
	  bdk_window_process_updates (viewport->bin_window, TRUE);
	}
    }
}

static void
btk_viewport_style_set (BtkWidget *widget,
			BtkStyle  *previous_style)
{
   if (btk_widget_get_realized (widget) &&
       btk_widget_get_has_window (widget))
     {
	BtkViewport *viewport = BTK_VIEWPORT (widget);

	btk_style_set_background (widget->style, viewport->bin_window, BTK_STATE_NORMAL);
	btk_style_set_background (widget->style, widget->window, widget->state);
     }
}

#define __BTK_VIEWPORT_C__
#include "btkaliasdef.c"
