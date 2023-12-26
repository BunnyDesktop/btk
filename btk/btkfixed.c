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
#include "btkfixed.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"
enum {
  CHILD_PROP_0,
  CHILD_PROP_X,
  CHILD_PROP_Y
};

static void btk_fixed_realize       (BtkWidget        *widget);
static void btk_fixed_size_request  (BtkWidget        *widget,
				     BtkRequisition   *requisition);
static void btk_fixed_size_allocate (BtkWidget        *widget,
				     BtkAllocation    *allocation);
static void btk_fixed_add           (BtkContainer     *container,
				     BtkWidget        *widget);
static void btk_fixed_remove        (BtkContainer     *container,
				     BtkWidget        *widget);
static void btk_fixed_forall        (BtkContainer     *container,
				     gboolean 	       include_internals,
				     BtkCallback       callback,
				     gpointer          callback_data);
static GType btk_fixed_child_type   (BtkContainer     *container);

static void btk_fixed_set_child_property (BtkContainer *container,
                                          BtkWidget    *child,
                                          guint         property_id,
                                          const BValue *value,
                                          BParamSpec   *pspec);
static void btk_fixed_get_child_property (BtkContainer *container,
                                          BtkWidget    *child,
                                          guint         property_id,
                                          BValue       *value,
                                          BParamSpec   *pspec);

G_DEFINE_TYPE (BtkFixed, btk_fixed, BTK_TYPE_CONTAINER)

static void
btk_fixed_class_init (BtkFixedClass *class)
{
  BtkWidgetClass *widget_class;
  BtkContainerClass *container_class;

  widget_class = (BtkWidgetClass*) class;
  container_class = (BtkContainerClass*) class;

  widget_class->realize = btk_fixed_realize;
  widget_class->size_request = btk_fixed_size_request;
  widget_class->size_allocate = btk_fixed_size_allocate;

  container_class->add = btk_fixed_add;
  container_class->remove = btk_fixed_remove;
  container_class->forall = btk_fixed_forall;
  container_class->child_type = btk_fixed_child_type;

  container_class->set_child_property = btk_fixed_set_child_property;
  container_class->get_child_property = btk_fixed_get_child_property;

  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_X,
					      g_param_spec_int ("x",
                                                                P_("X position"),
                                                                P_("X position of child widget"),
                                                                G_MININT,
                                                                G_MAXINT,
                                                                0,
                                                                BTK_PARAM_READWRITE));

  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_Y,
					      g_param_spec_int ("y",
                                                                P_("Y position"),
                                                                P_("Y position of child widget"),
                                                                G_MININT,
                                                                G_MAXINT,
                                                                0,
                                                                BTK_PARAM_READWRITE));
}

static GType
btk_fixed_child_type (BtkContainer     *container)
{
  return BTK_TYPE_WIDGET;
}

static void
btk_fixed_init (BtkFixed *fixed)
{
  btk_widget_set_has_window (BTK_WIDGET (fixed), FALSE);

  fixed->children = NULL;
}

BtkWidget*
btk_fixed_new (void)
{
  return g_object_new (BTK_TYPE_FIXED, NULL);
}

static BtkFixedChild*
get_child (BtkFixed  *fixed,
           BtkWidget *widget)
{
  GList *children;
  
  children = fixed->children;
  while (children)
    {
      BtkFixedChild *child;
      
      child = children->data;
      children = children->next;

      if (child->widget == widget)
        return child;
    }

  return NULL;
}

void
btk_fixed_put (BtkFixed       *fixed,
               BtkWidget      *widget,
               gint            x,
               gint            y)
{
  BtkFixedChild *child_info;

  g_return_if_fail (BTK_IS_FIXED (fixed));
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (btk_widget_get_parent (widget) == NULL);

  child_info = g_new (BtkFixedChild, 1);
  child_info->widget = widget;
  child_info->x = x;
  child_info->y = y;

  btk_widget_set_parent (widget, BTK_WIDGET (fixed));

  fixed->children = g_list_append (fixed->children, child_info);
}

static void
btk_fixed_move_internal (BtkFixed       *fixed,
                         BtkWidget      *widget,
                         gboolean        change_x,
                         gint            x,
                         gboolean        change_y,
                         gint            y)
{
  BtkFixedChild *child;
  
  g_return_if_fail (BTK_IS_FIXED (fixed));
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (widget->parent == BTK_WIDGET (fixed));  
  
  child = get_child (fixed, widget);

  g_assert (child);

  btk_widget_freeze_child_notify (widget);
  
  if (change_x)
    {
      child->x = x;
      btk_widget_child_notify (widget, "x");
    }

  if (change_y)
    {
      child->y = y;
      btk_widget_child_notify (widget, "y");
    }

  btk_widget_thaw_child_notify (widget);
  
  if (btk_widget_get_visible (widget) &&
      btk_widget_get_visible (BTK_WIDGET (fixed)))
    btk_widget_queue_resize (BTK_WIDGET (fixed));
}

void
btk_fixed_move (BtkFixed       *fixed,
                BtkWidget      *widget,
                gint            x,
                gint            y)
{
  btk_fixed_move_internal (fixed, widget, TRUE, x, TRUE, y);
}

static void
btk_fixed_set_child_property (BtkContainer    *container,
                              BtkWidget       *child,
                              guint            property_id,
                              const BValue    *value,
                              BParamSpec      *pspec)
{
  switch (property_id)
    {
    case CHILD_PROP_X:
      btk_fixed_move_internal (BTK_FIXED (container),
                               child,
                               TRUE, b_value_get_int (value),
                               FALSE, 0);
      break;
    case CHILD_PROP_Y:
      btk_fixed_move_internal (BTK_FIXED (container),
                               child,
                               FALSE, 0,
                               TRUE, b_value_get_int (value));
      break;
    default:
      BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
btk_fixed_get_child_property (BtkContainer *container,
                              BtkWidget    *child,
                              guint         property_id,
                              BValue       *value,
                              BParamSpec   *pspec)
{
  BtkFixedChild *fixed_child;

  fixed_child = get_child (BTK_FIXED (container), child);
  
  switch (property_id)
    {
    case CHILD_PROP_X:
      b_value_set_int (value, fixed_child->x);
      break;
    case CHILD_PROP_Y:
      b_value_set_int (value, fixed_child->y);
      break;
    default:
      BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
btk_fixed_realize (BtkWidget *widget)
{
  BdkWindowAttr attributes;
  gint attributes_mask;

  if (!btk_widget_get_has_window (widget))
    BTK_WIDGET_CLASS (btk_fixed_parent_class)->realize (widget);
  else
    {
      btk_widget_set_realized (widget, TRUE);

      attributes.window_type = BDK_WINDOW_CHILD;
      attributes.x = widget->allocation.x;
      attributes.y = widget->allocation.y;
      attributes.width = widget->allocation.width;
      attributes.height = widget->allocation.height;
      attributes.wclass = BDK_INPUT_OUTPUT;
      attributes.visual = btk_widget_get_visual (widget);
      attributes.colormap = btk_widget_get_colormap (widget);
      attributes.event_mask = btk_widget_get_events (widget);
      attributes.event_mask |= BDK_EXPOSURE_MASK | BDK_BUTTON_PRESS_MASK;
      
      attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;
      
      widget->window = bdk_window_new (btk_widget_get_parent_window (widget), &attributes, 
				       attributes_mask);
      bdk_window_set_user_data (widget->window, widget);
      
      widget->style = btk_style_attach (widget->style, widget->window);
      btk_style_set_background (widget->style, widget->window, BTK_STATE_NORMAL);
    }
}

static void
btk_fixed_size_request (BtkWidget      *widget,
			BtkRequisition *requisition)
{
  BtkFixed *fixed;  
  BtkFixedChild *child;
  GList *children;
  BtkRequisition child_requisition;

  fixed = BTK_FIXED (widget);
  requisition->width = 0;
  requisition->height = 0;

  children = fixed->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (btk_widget_get_visible (child->widget))
	{
          btk_widget_size_request (child->widget, &child_requisition);

          requisition->height = MAX (requisition->height,
                                     child->y +
                                     child_requisition.height);
          requisition->width = MAX (requisition->width,
                                    child->x +
                                    child_requisition.width);
	}
    }

  requisition->height += BTK_CONTAINER (fixed)->border_width * 2;
  requisition->width += BTK_CONTAINER (fixed)->border_width * 2;
}

static void
btk_fixed_size_allocate (BtkWidget     *widget,
			 BtkAllocation *allocation)
{
  BtkFixed *fixed;
  BtkFixedChild *child;
  BtkAllocation child_allocation;
  BtkRequisition child_requisition;
  GList *children;
  guint16 border_width;

  fixed = BTK_FIXED (widget);

  widget->allocation = *allocation;

  if (btk_widget_get_has_window (widget))
    {
      if (btk_widget_get_realized (widget))
	bdk_window_move_resize (widget->window,
				allocation->x, 
				allocation->y,
				allocation->width, 
				allocation->height);
    }
      
  border_width = BTK_CONTAINER (fixed)->border_width;
  
  children = fixed->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (btk_widget_get_visible (child->widget))
	{
	  btk_widget_get_child_requisition (child->widget, &child_requisition);
	  child_allocation.x = child->x + border_width;
	  child_allocation.y = child->y + border_width;

	  if (!btk_widget_get_has_window (widget))
	    {
	      child_allocation.x += widget->allocation.x;
	      child_allocation.y += widget->allocation.y;
	    }
	  
	  child_allocation.width = child_requisition.width;
	  child_allocation.height = child_requisition.height;
	  btk_widget_size_allocate (child->widget, &child_allocation);
	}
    }
}

static void
btk_fixed_add (BtkContainer *container,
	       BtkWidget    *widget)
{
  btk_fixed_put (BTK_FIXED (container), widget, 0, 0);
}

static void
btk_fixed_remove (BtkContainer *container,
		  BtkWidget    *widget)
{
  BtkFixed *fixed;
  BtkFixedChild *child;
  BtkWidget *widget_container;
  GList *children;

  fixed = BTK_FIXED (container);
  widget_container = BTK_WIDGET (container);

  children = fixed->children;
  while (children)
    {
      child = children->data;

      if (child->widget == widget)
	{
	  gboolean was_visible = btk_widget_get_visible (widget);
	  
	  btk_widget_unparent (widget);

	  fixed->children = g_list_remove_link (fixed->children, children);
	  g_list_free (children);
	  g_free (child);

	  if (was_visible && btk_widget_get_visible (widget_container))
	    btk_widget_queue_resize (widget_container);

	  break;
	}

      children = children->next;
    }
}

static void
btk_fixed_forall (BtkContainer *container,
		  gboolean	include_internals,
		  BtkCallback   callback,
		  gpointer      callback_data)
{
  BtkFixed *fixed = BTK_FIXED (container);
  BtkFixedChild *child;
  GList *children;

  children = fixed->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      (* callback) (child->widget, callback_data);
    }
}

/**
 * btk_fixed_set_has_window:
 * @fixed: a #BtkFixed
 * @has_window: %TRUE if a separate window should be created
 * 
 * Sets whether a #BtkFixed widget is created with a separate
 * #BdkWindow for @widget->window or not. (By default, it will be
 * created with no separate #BdkWindow). This function must be called
 * while the #BtkFixed is not realized, for instance, immediately after the
 * window is created.
 * 
 * This function was added to provide an easy migration path for
 * older applications which may expect #BtkFixed to have a separate window.
 *
 * Deprecated: 2.20: Use btk_widget_set_has_window() instead.
 **/
void
btk_fixed_set_has_window (BtkFixed *fixed,
			  gboolean  has_window)
{
  g_return_if_fail (BTK_IS_FIXED (fixed));
  g_return_if_fail (!btk_widget_get_realized (BTK_WIDGET (fixed)));

  if (has_window != btk_widget_get_has_window (BTK_WIDGET (fixed)))
    {
      btk_widget_set_has_window (BTK_WIDGET (fixed), has_window);
    }
}

/**
 * btk_fixed_get_has_window:
 * @fixed: a #BtkWidget
 * 
 * Gets whether the #BtkFixed has its own #BdkWindow.
 * See btk_fixed_set_has_window().
 * 
 * Return value: %TRUE if @fixed has its own window.
 *
 * Deprecated: 2.20: Use btk_widget_get_has_window() instead.
 **/
gboolean
btk_fixed_get_has_window (BtkFixed *fixed)
{
  g_return_val_if_fail (BTK_IS_FIXED (fixed), FALSE);

  return btk_widget_get_has_window (BTK_WIDGET (fixed));
}

#define __BTK_FIXED_C__
#include "btkaliasdef.c"
