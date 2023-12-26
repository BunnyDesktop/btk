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
#include "btkeventbox.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

typedef struct
{
  gboolean above_child;
  BdkWindow *event_window;
} BtkEventBoxPrivate;

enum {
  PROP_0,
  PROP_VISIBLE_WINDOW,
  PROP_ABOVE_CHILD
};


#define BTK_EVENT_BOX_GET_PRIVATE(obj)  G_TYPE_INSTANCE_GET_PRIVATE((obj), BTK_TYPE_EVENT_BOX, BtkEventBoxPrivate)

static void     btk_event_box_realize       (BtkWidget        *widget);
static void     btk_event_box_unrealize     (BtkWidget        *widget);
static void     btk_event_box_map           (BtkWidget        *widget);
static void     btk_event_box_unmap         (BtkWidget        *widget);
static void     btk_event_box_size_request  (BtkWidget        *widget,
                                             BtkRequisition   *requisition);
static void     btk_event_box_size_allocate (BtkWidget        *widget,
                                             BtkAllocation    *allocation);
static void     btk_event_box_paint         (BtkWidget        *widget,
                                             BdkRectangle     *area);
static gboolean btk_event_box_expose        (BtkWidget        *widget,
                                             BdkEventExpose   *event);
static void     btk_event_box_set_property  (GObject          *object,
                                             guint             prop_id,
                                             const GValue     *value,
                                             GParamSpec       *pspec);
static void     btk_event_box_get_property  (GObject          *object,
                                             guint             prop_id,
                                             GValue           *value,
                                             GParamSpec       *pspec);

G_DEFINE_TYPE (BtkEventBox, btk_event_box, BTK_TYPE_BIN)

static void
btk_event_box_class_init (BtkEventBoxClass *class)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (class);
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (class);

  bobject_class->set_property = btk_event_box_set_property;
  bobject_class->get_property = btk_event_box_get_property;
  
  widget_class->realize = btk_event_box_realize;
  widget_class->unrealize = btk_event_box_unrealize;
  widget_class->map = btk_event_box_map;
  widget_class->unmap = btk_event_box_unmap;
  widget_class->size_request = btk_event_box_size_request;
  widget_class->size_allocate = btk_event_box_size_allocate;
  widget_class->expose_event = btk_event_box_expose;

  g_object_class_install_property (bobject_class,
                                   PROP_VISIBLE_WINDOW,
                                   g_param_spec_boolean ("visible-window",
                                                        P_("Visible Window"),
                                                        P_("Whether the event box is visible, as opposed to invisible and only used to trap events."),
                                                        TRUE,
                                                        BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_ABOVE_CHILD,
                                   g_param_spec_boolean ("above-child",
                                                        P_("Above child"),
                                                        P_("Whether the event-trapping window of the eventbox is above the window of the child widget as opposed to below it."),
                                                        FALSE,
                                                        BTK_PARAM_READWRITE));
  
  g_type_class_add_private (class, sizeof (BtkEventBoxPrivate));
}

static void
btk_event_box_init (BtkEventBox *event_box)
{
  BtkEventBoxPrivate *priv;

  btk_widget_set_has_window (BTK_WIDGET (event_box), TRUE);
 
  priv = BTK_EVENT_BOX_GET_PRIVATE (event_box);
  priv->above_child = FALSE;
}

BtkWidget*
btk_event_box_new (void)
{
  return g_object_new (BTK_TYPE_EVENT_BOX, NULL);
}

static void 
btk_event_box_set_property (GObject      *object,
			    guint         prop_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
  BtkEventBox *event_box;
  
  event_box = BTK_EVENT_BOX (object);
  
  switch (prop_id)
    {
    case PROP_VISIBLE_WINDOW:
      btk_event_box_set_visible_window (event_box, g_value_get_boolean (value));
      break;	  
    case PROP_ABOVE_CHILD:
      btk_event_box_set_above_child (event_box, g_value_get_boolean (value));
      break;	  
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
btk_event_box_get_property (GObject     *object,
			    guint        prop_id,
			    GValue      *value,
			    GParamSpec  *pspec)
{
  BtkEventBox *event_box;
  
  event_box = BTK_EVENT_BOX (object);
  
  switch (prop_id)
    {
    case PROP_VISIBLE_WINDOW:
      g_value_set_boolean (value, btk_event_box_get_visible_window (event_box));
      break;
    case PROP_ABOVE_CHILD:
      g_value_set_boolean (value, btk_event_box_get_above_child (event_box));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * btk_event_box_get_visible_window:
 * @event_box: a #BtkEventBox
 *
 * Returns whether the event box has a visible window.
 * See btk_event_box_set_visible_window() for details.
 *
 * Return value: %TRUE if the event box window is visible
 *
 * Since: 2.4
 **/
gboolean
btk_event_box_get_visible_window (BtkEventBox *event_box)
{
  g_return_val_if_fail (BTK_IS_EVENT_BOX (event_box), FALSE);

  return btk_widget_get_has_window (BTK_WIDGET (event_box));
}

/**
 * btk_event_box_set_visible_window:
 * @event_box: a #BtkEventBox
 * @visible_window: boolean value
 *
 * Set whether the event box uses a visible or invisible child
 * window. The default is to use visible windows.
 *
 * In an invisible window event box, the window that the
 * event box creates is a %BDK_INPUT_ONLY window, which 
 * means that it is invisible and only serves to receive
 * events.
 * 
 * A visible window event box creates a visible (%BDK_INPUT_OUTPUT)
 * window that acts as the parent window for all the widgets  
 * contained in the event box.
 * 
 * You should generally make your event box invisible if
 * you just want to trap events. Creating a visible window
 * may cause artifacts that are visible to the user, especially
 * if the user is using a theme with gradients or pixmaps.
 * 
 * The main reason to create a non input-only event box is if
 * you want to set the background to a different color or
 * draw on it.
 *
 * <note><para>
 * There is one unexpected issue for an invisible event box that has its
 * window below the child. (See btk_event_box_set_above_child().)
 * Since the input-only window is not an ancestor window of any windows
 * that descendent widgets of the event box create, events on these 
 * windows aren't propagated up by the windowing system, but only by BTK+.
 * The practical effect of this is if an event isn't in the event
 * mask for the descendant window (see btk_widget_add_events()),  
 * it won't be received by the event box. 
 * </para><para>
 * This problem doesn't occur for visible event boxes, because in
 * that case, the event box window is actually the ancestor of the
 * descendant windows, not just at the same place on the screen.
 * </para></note>
 * 
 * Since: 2.4
 **/
void
btk_event_box_set_visible_window (BtkEventBox *event_box,
				  gboolean visible_window)
{
  BtkWidget *widget;

  g_return_if_fail (BTK_IS_EVENT_BOX (event_box));

  widget = BTK_WIDGET (event_box);

  visible_window = visible_window != FALSE;

  if (visible_window != btk_widget_get_has_window (widget))
    {
      if (btk_widget_get_realized (widget))
	{
	  gboolean visible = btk_widget_get_visible (widget);

	  if (visible)
	    btk_widget_hide (widget);

	  btk_widget_unrealize (widget);

          btk_widget_set_has_window (widget, visible_window);

	  btk_widget_realize (widget);

	  if (visible)
	    btk_widget_show (widget);
	}
      else
	{
          btk_widget_set_has_window (widget, visible_window);
	}

      if (btk_widget_get_visible (widget))
	btk_widget_queue_resize (widget);
      
      g_object_notify (G_OBJECT (event_box), "visible-window");
    }
}

/**
 * btk_event_box_get_above_child:
 * @event_box: a #BtkEventBox
 *
 * Returns whether the event box window is above or below the
 * windows of its child. See btk_event_box_set_above_child() for
 * details.
 *
 * Return value: %TRUE if the event box window is above the window
 * of its child.
 *
 * Since: 2.4
 **/
gboolean
btk_event_box_get_above_child (BtkEventBox *event_box)
{
  BtkEventBoxPrivate *priv;

  g_return_val_if_fail (BTK_IS_EVENT_BOX (event_box), FALSE);

  priv = BTK_EVENT_BOX_GET_PRIVATE (event_box);

  return priv->above_child;
}

/**
 * btk_event_box_set_above_child:
 * @event_box: a #BtkEventBox
 * @above_child: %TRUE if the event box window is above the windows of its child
 *
 * Set whether the event box window is positioned above the windows of its child,
 * as opposed to below it. If the window is above, all events inside the
 * event box will go to the event box. If the window is below, events
 * in windows of child widgets will first got to that widget, and then
 * to its parents.
 *
 * The default is to keep the window below the child.
 * 
 * Since: 2.4
 **/
void
btk_event_box_set_above_child (BtkEventBox *event_box,
			       gboolean above_child)
{
  BtkWidget *widget;
  BtkEventBoxPrivate *priv;

  g_return_if_fail (BTK_IS_EVENT_BOX (event_box));

  widget = BTK_WIDGET (event_box);
  priv = BTK_EVENT_BOX_GET_PRIVATE (event_box);

  above_child = above_child != FALSE;

  if (priv->above_child != above_child)
    {
      priv->above_child = above_child;

      if (btk_widget_get_realized (widget))
	{
	  if (!btk_widget_get_has_window (widget))
	    {
	      if (above_child)
		bdk_window_raise (priv->event_window);
	      else
		bdk_window_lower (priv->event_window);
	    }
	  else
	    {
	      gboolean visible = btk_widget_get_visible (widget);

	      if (visible)
		btk_widget_hide (widget);
	      
	      btk_widget_unrealize (widget);
	      
	      btk_widget_realize (widget);
	      
	      if (visible)
		btk_widget_show (widget);
	    }
	}

      if (btk_widget_get_visible (widget))
	btk_widget_queue_resize (widget);
      
      g_object_notify (G_OBJECT (event_box), "above-child");
    }
}


static void
btk_event_box_realize (BtkWidget *widget)
{
  BdkWindowAttr attributes;
  gint attributes_mask;
  gint border_width;
  BtkEventBoxPrivate *priv;
  gboolean visible_window;

  btk_widget_set_realized (widget, TRUE);

  border_width = BTK_CONTAINER (widget)->border_width;
  
  attributes.x = widget->allocation.x + border_width;
  attributes.y = widget->allocation.y + border_width;
  attributes.width = widget->allocation.width - 2*border_width;
  attributes.height = widget->allocation.height - 2*border_width;
  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.event_mask = btk_widget_get_events (widget)
			| BDK_BUTTON_MOTION_MASK
			| BDK_BUTTON_PRESS_MASK
			| BDK_BUTTON_RELEASE_MASK
			| BDK_EXPOSURE_MASK
			| BDK_ENTER_NOTIFY_MASK
			| BDK_LEAVE_NOTIFY_MASK;

  priv = BTK_EVENT_BOX_GET_PRIVATE (widget);

  visible_window = btk_widget_get_has_window (widget);
  if (visible_window)
    {
      attributes.visual = btk_widget_get_visual (widget);
      attributes.colormap = btk_widget_get_colormap (widget);
      attributes.wclass = BDK_INPUT_OUTPUT;
      
      attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;
      
      widget->window = bdk_window_new (btk_widget_get_parent_window (widget),
				       &attributes, attributes_mask);
      bdk_window_set_user_data (widget->window, widget);
    }
  else
    {
      widget->window = btk_widget_get_parent_window (widget);
      g_object_ref (widget->window);
    }

  if (!visible_window || priv->above_child)
    {
      attributes.wclass = BDK_INPUT_ONLY;
      if (!visible_window)
        attributes_mask = BDK_WA_X | BDK_WA_Y;
      else
        attributes_mask = 0;

      priv->event_window = bdk_window_new (widget->window,
					   &attributes, attributes_mask);
      bdk_window_set_user_data (priv->event_window, widget);
    }


  widget->style = btk_style_attach (widget->style, widget->window);
  
  if (visible_window)
    btk_style_set_background (widget->style, widget->window, BTK_STATE_NORMAL);
}

static void
btk_event_box_unrealize (BtkWidget *widget)
{
  BtkEventBoxPrivate *priv;
  
  priv = BTK_EVENT_BOX_GET_PRIVATE (widget);
  
  if (priv->event_window != NULL)
    {
      bdk_window_set_user_data (priv->event_window, NULL);
      bdk_window_destroy (priv->event_window);
      priv->event_window = NULL;
    }

  BTK_WIDGET_CLASS (btk_event_box_parent_class)->unrealize (widget);
}

static void
btk_event_box_map (BtkWidget *widget)
{
  BtkEventBoxPrivate *priv;

  priv = BTK_EVENT_BOX_GET_PRIVATE (widget);

  if (priv->event_window != NULL && !priv->above_child)
    bdk_window_show (priv->event_window);

  BTK_WIDGET_CLASS (btk_event_box_parent_class)->map (widget);

  if (priv->event_window != NULL && priv->above_child)
    bdk_window_show (priv->event_window);
}

static void
btk_event_box_unmap (BtkWidget *widget)
{
  BtkEventBoxPrivate *priv;

  priv = BTK_EVENT_BOX_GET_PRIVATE (widget);

  if (priv->event_window != NULL)
    bdk_window_hide (priv->event_window);

  BTK_WIDGET_CLASS (btk_event_box_parent_class)->unmap (widget);
}



static void
btk_event_box_size_request (BtkWidget      *widget,
			    BtkRequisition *requisition)
{
  BtkBin *bin = BTK_BIN (widget);

  requisition->width = BTK_CONTAINER (widget)->border_width * 2;
  requisition->height = BTK_CONTAINER (widget)->border_width * 2;

  if (bin->child && btk_widget_get_visible (bin->child))
    {
      BtkRequisition child_requisition;
      
      btk_widget_size_request (bin->child, &child_requisition);

      requisition->width += child_requisition.width;
      requisition->height += child_requisition.height;
    }
}

static void
btk_event_box_size_allocate (BtkWidget     *widget,
			     BtkAllocation *allocation)
{
  BtkBin *bin;
  BtkAllocation child_allocation;
  BtkEventBoxPrivate *priv;
  
  widget->allocation = *allocation;
  bin = BTK_BIN (widget);
  
  if (!btk_widget_get_has_window (widget))
    {
      child_allocation.x = allocation->x + BTK_CONTAINER (widget)->border_width;
      child_allocation.y = allocation->y + BTK_CONTAINER (widget)->border_width;
    }
  else
    {
      child_allocation.x = 0;
      child_allocation.y = 0;
    }
  child_allocation.width = MAX (allocation->width - BTK_CONTAINER (widget)->border_width * 2, 0);
  child_allocation.height = MAX (allocation->height - BTK_CONTAINER (widget)->border_width * 2, 0);

  if (btk_widget_get_realized (widget))
    {
      priv = BTK_EVENT_BOX_GET_PRIVATE (widget);

      if (priv->event_window != NULL)
	bdk_window_move_resize (priv->event_window,
				child_allocation.x,
				child_allocation.y,
				child_allocation.width,
				child_allocation.height);
      
      if (btk_widget_get_has_window (widget))
	bdk_window_move_resize (widget->window,
				allocation->x + BTK_CONTAINER (widget)->border_width,
				allocation->y + BTK_CONTAINER (widget)->border_width,
				child_allocation.width,
				child_allocation.height);
    }
  
  if (bin->child)
    btk_widget_size_allocate (bin->child, &child_allocation);
}

static void
btk_event_box_paint (BtkWidget    *widget,
		     BdkRectangle *area)
{
  if (!btk_widget_get_app_paintable (widget))
    btk_paint_flat_box (widget->style, widget->window,
			widget->state, BTK_SHADOW_NONE,
			area, widget, "eventbox",
			0, 0, -1, -1);
}

static gboolean
btk_event_box_expose (BtkWidget      *widget,
		     BdkEventExpose *event)
{
  if (btk_widget_is_drawable (widget))
    {
      if (btk_widget_get_has_window (widget))
	btk_event_box_paint (widget, &event->area);

      BTK_WIDGET_CLASS (btk_event_box_parent_class)->expose_event (widget, event);
    }

  return FALSE;
}

#define __BTK_EVENT_BOX_C__
#include "btkaliasdef.c"
