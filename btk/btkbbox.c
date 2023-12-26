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
#include "btkbbox.h"
#include "btkhbbox.h"
#include "btkvbbox.h"
#include "btkorientable.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

enum {
  PROP_0,
  PROP_LAYOUT_STYLE
};

enum {
  CHILD_PROP_0,
  CHILD_PROP_SECONDARY
};

static void btk_button_box_set_property       (BObject           *object,
					       guint              prop_id,
					       const BValue      *value,
					       BParamSpec        *pspec);
static void btk_button_box_get_property       (BObject           *object,
					       guint              prop_id,
					       BValue            *value,
					       BParamSpec        *pspec);
static void btk_button_box_size_request       (BtkWidget         *widget,
                                               BtkRequisition    *requisition);
static void btk_button_box_size_allocate      (BtkWidget         *widget,
                                               BtkAllocation     *allocation);
static void btk_button_box_set_child_property (BtkContainer      *container,
					       BtkWidget         *child,
					       guint              property_id,
					       const BValue      *value,
					       BParamSpec        *pspec);
static void btk_button_box_get_child_property (BtkContainer      *container,
					       BtkWidget         *child,
					       guint              property_id,
					       BValue            *value,
					       BParamSpec        *pspec);

#define DEFAULT_CHILD_MIN_WIDTH 85
#define DEFAULT_CHILD_MIN_HEIGHT 27
#define DEFAULT_CHILD_IPAD_X 4
#define DEFAULT_CHILD_IPAD_Y 0

G_DEFINE_ABSTRACT_TYPE (BtkButtonBox, btk_button_box, BTK_TYPE_BOX)

static void
btk_button_box_class_init (BtkButtonBoxClass *class)
{
  BtkWidgetClass *widget_class;
  BObjectClass *bobject_class;
  BtkContainerClass *container_class;

  bobject_class = B_OBJECT_CLASS (class);
  widget_class = (BtkWidgetClass*) class;
  container_class = (BtkContainerClass*) class;

  bobject_class->set_property = btk_button_box_set_property;
  bobject_class->get_property = btk_button_box_get_property;

  widget_class->size_request = btk_button_box_size_request;
  widget_class->size_allocate = btk_button_box_size_allocate;

  container_class->set_child_property = btk_button_box_set_child_property;
  container_class->get_child_property = btk_button_box_get_child_property;

  /* FIXME we need to override the "spacing" property on BtkBox once
   * libbobject allows that.
   */
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("child-min-width",
							     P_("Minimum child width"),
							     P_("Minimum width of buttons inside the box"),
							     0,
							     G_MAXINT,
                                                             DEFAULT_CHILD_MIN_WIDTH,
							     BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("child-min-height",
							     P_("Minimum child height"),
							     P_("Minimum height of buttons inside the box"),
							     0,
							     G_MAXINT,
                                                             DEFAULT_CHILD_MIN_HEIGHT,
							     BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("child-internal-pad-x",
							     P_("Child internal width padding"),
							     P_("Amount to increase child's size on either side"),
							     0,
							     G_MAXINT,
                                                             DEFAULT_CHILD_IPAD_X,
							     BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("child-internal-pad-y",
							     P_("Child internal height padding"),
							     P_("Amount to increase child's size on the top and bottom"),
							     0,
							     G_MAXINT,
                                                             DEFAULT_CHILD_IPAD_Y,
							     BTK_PARAM_READABLE));
  g_object_class_install_property (bobject_class,
                                   PROP_LAYOUT_STYLE,
                                   g_param_spec_enum ("layout-style",
                                                      P_("Layout style"),
                                                      P_("How to lay out the buttons in the box. Possible values are: default, spread, edge, start and end"),
						      BTK_TYPE_BUTTON_BOX_STYLE,
						      BTK_BUTTONBOX_DEFAULT_STYLE,
                                                      BTK_PARAM_READWRITE));

  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_SECONDARY,
					      g_param_spec_boolean ("secondary", 
								    P_("Secondary"),
								    P_("If TRUE, the child appears in a secondary group of children, suitable for, e.g., help buttons"),
								    FALSE,
								    BTK_PARAM_READWRITE));
}

static void
btk_button_box_init (BtkButtonBox *button_box)
{
  BTK_BOX (button_box)->spacing = 0;
  button_box->child_min_width = BTK_BUTTONBOX_DEFAULT;
  button_box->child_min_height = BTK_BUTTONBOX_DEFAULT;
  button_box->child_ipad_x = BTK_BUTTONBOX_DEFAULT;
  button_box->child_ipad_y = BTK_BUTTONBOX_DEFAULT;
  button_box->layout_style = BTK_BUTTONBOX_DEFAULT_STYLE;
}

static void
btk_button_box_set_property (BObject         *object,
			     guint            prop_id,
			     const BValue    *value,
			     BParamSpec      *pspec)
{
  switch (prop_id)
    {
    case PROP_LAYOUT_STYLE:
      btk_button_box_set_layout (BTK_BUTTON_BOX (object),
				 b_value_get_enum (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_button_box_get_property (BObject         *object,
			     guint            prop_id,
			     BValue          *value,
			     BParamSpec      *pspec)
{
  switch (prop_id)
    {
    case PROP_LAYOUT_STYLE:
      b_value_set_enum (value, BTK_BUTTON_BOX (object)->layout_style);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_button_box_set_child_property (BtkContainer    *container,
				   BtkWidget       *child,
				   guint            property_id,
				   const BValue    *value,
				   BParamSpec      *pspec)
{
  switch (property_id)
    {
    case CHILD_PROP_SECONDARY:
      btk_button_box_set_child_secondary (BTK_BUTTON_BOX (container), child,
					  b_value_get_boolean (value));
      break;
    default:
      BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
btk_button_box_get_child_property (BtkContainer *container,
				   BtkWidget    *child,
				   guint         property_id,
				   BValue       *value,
				   BParamSpec   *pspec)
{
  switch (property_id)
    {
    case CHILD_PROP_SECONDARY:
      b_value_set_boolean (value, 
			   btk_button_box_get_child_secondary (BTK_BUTTON_BOX (container), 
							       child));
      break;
    default:
      BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

/* set per widget values for spacing, child size and child internal padding */

void 
btk_button_box_set_child_size (BtkButtonBox *widget, 
                               gint width, gint height)
{
  g_return_if_fail (BTK_IS_BUTTON_BOX (widget));

  widget->child_min_width = width;
  widget->child_min_height = height;
}

void 
btk_button_box_set_child_ipadding (BtkButtonBox *widget,
                                   gint ipad_x, gint ipad_y)
{
  g_return_if_fail (BTK_IS_BUTTON_BOX (widget));

  widget->child_ipad_x = ipad_x;
  widget->child_ipad_y = ipad_y;
}

void
btk_button_box_set_layout (BtkButtonBox      *widget, 
                           BtkButtonBoxStyle  layout_style)
{
  g_return_if_fail (BTK_IS_BUTTON_BOX (widget));
  g_return_if_fail (layout_style >= BTK_BUTTONBOX_DEFAULT_STYLE &&
		    layout_style <= BTK_BUTTONBOX_CENTER);

  if (widget->layout_style != layout_style)
    {
      widget->layout_style = layout_style;
      g_object_notify (B_OBJECT (widget), "layout-style");
      btk_widget_queue_resize (BTK_WIDGET (widget));
    }
}


/* get per widget values for spacing, child size and child internal padding */

void 
btk_button_box_get_child_size (BtkButtonBox *widget,
                               gint *width, gint *height)
{
  g_return_if_fail (BTK_IS_BUTTON_BOX (widget));
  g_return_if_fail (width != NULL);
  g_return_if_fail (height != NULL);

  *width  = widget->child_min_width;
  *height = widget->child_min_height;
}

void
btk_button_box_get_child_ipadding (BtkButtonBox *widget,
                                   gint* ipad_x, gint *ipad_y)
{
  g_return_if_fail (BTK_IS_BUTTON_BOX (widget));
  g_return_if_fail (ipad_x != NULL);
  g_return_if_fail (ipad_y != NULL);

  *ipad_x = widget->child_ipad_x;
  *ipad_y = widget->child_ipad_y;
}

BtkButtonBoxStyle 
btk_button_box_get_layout (BtkButtonBox *widget)
{
  g_return_val_if_fail (BTK_IS_BUTTON_BOX (widget), BTK_BUTTONBOX_SPREAD);
  
  return widget->layout_style;
}

/**
 * btk_button_box_get_child_secondary:
 * @widget: a #BtkButtonBox
 * @child: a child of @widget 
 * 
 * Returns whether @child should appear in a secondary group of children.
 *
 * Return value: whether @child should appear in a secondary group of children.
 *
 * Since: 2.4
 **/
gboolean 
btk_button_box_get_child_secondary (BtkButtonBox *widget,
				    BtkWidget    *child)
{
  GList *list;
  BtkBoxChild *child_info;

  g_return_val_if_fail (BTK_IS_BUTTON_BOX (widget), FALSE);
  g_return_val_if_fail (BTK_IS_WIDGET (child), FALSE);

  child_info = NULL;
  list = BTK_BOX (widget)->children;
  while (list)
    {
      child_info = list->data;
      if (child_info->widget == child)
	break;

      list = list->next;
    }

  g_return_val_if_fail (list != NULL, FALSE);

  return child_info->is_secondary;
}

/**
 * btk_button_box_set_child_secondary
 * @widget: a #BtkButtonBox
 * @child: a child of @widget
 * @is_secondary: if %TRUE, the @child appears in a secondary group of the
 *                button box.
 *
 * Sets whether @child should appear in a secondary group of children.
 * A typical use of a secondary child is the help button in a dialog.
 *
 * This group appears after the other children if the style
 * is %BTK_BUTTONBOX_START, %BTK_BUTTONBOX_SPREAD or
 * %BTK_BUTTONBOX_EDGE, and before the other children if the style
 * is %BTK_BUTTONBOX_END. For horizontal button boxes, the definition
 * of before/after depends on direction of the widget (see
 * btk_widget_set_direction()). If the style is %BTK_BUTTONBOX_START
 * or %BTK_BUTTONBOX_END, then the secondary children are aligned at
 * the other end of the button box from the main children. For the
 * other styles, they appear immediately next to the main children.
 **/
void 
btk_button_box_set_child_secondary (BtkButtonBox *widget, 
				    BtkWidget    *child,
				    gboolean      is_secondary)
{
  GList *list;
  
  g_return_if_fail (BTK_IS_BUTTON_BOX (widget));
  g_return_if_fail (BTK_IS_WIDGET (child));
  g_return_if_fail (child->parent == BTK_WIDGET (widget));

  list = BTK_BOX (widget)->children;
  while (list)
    {
      BtkBoxChild *child_info = list->data;
      if (child_info->widget == child)
	{
	  child_info->is_secondary = is_secondary;
	  break;
	}

      list = list->next;
    }

  btk_widget_child_notify (child, "secondary");

  if (btk_widget_get_visible (BTK_WIDGET (widget))
      && btk_widget_get_visible (child))
    btk_widget_queue_resize (child);
}

/* Ask children how much space they require and round up 
   to match minimum size and internal padding.
   Returns the size each single child should have. */
void
_btk_button_box_child_requisition (BtkWidget *widget,
                                   int       *nvis_children,
				   int       *nvis_secondaries,
                                   int       *width,
                                   int       *height)
{
  BtkButtonBox *bbox;
  BtkBoxChild *child;
  GList *children;
  gint nchildren;
  gint nsecondaries;
  gint needed_width;
  gint needed_height;
  BtkRequisition child_requisition;
  gint ipad_w;
  gint ipad_h;
  gint width_default;
  gint height_default;
  gint ipad_x_default;
  gint ipad_y_default;
  
  gint child_min_width;
  gint child_min_height;
  gint ipad_x;
  gint ipad_y;
  
  g_return_if_fail (BTK_IS_BUTTON_BOX (widget));

  bbox = BTK_BUTTON_BOX (widget);

  btk_widget_style_get (widget,
                        "child-min-width", &width_default,
                        "child-min-height", &height_default,
                        "child-internal-pad-x", &ipad_x_default,
                        "child-internal-pad-y", &ipad_y_default, 
			NULL);
  
  child_min_width = bbox->child_min_width   != BTK_BUTTONBOX_DEFAULT
	  ? bbox->child_min_width : width_default;
  child_min_height = bbox->child_min_height !=BTK_BUTTONBOX_DEFAULT
	  ? bbox->child_min_height : height_default;
  ipad_x = bbox->child_ipad_x != BTK_BUTTONBOX_DEFAULT
	  ? bbox->child_ipad_x : ipad_x_default;
  ipad_y = bbox->child_ipad_y != BTK_BUTTONBOX_DEFAULT
	  ? bbox->child_ipad_y : ipad_y_default;

  nchildren = 0;
  nsecondaries = 0;
  children = BTK_BOX(bbox)->children;
  needed_width = child_min_width;
  needed_height = child_min_height;  
  ipad_w = ipad_x * 2;
  ipad_h = ipad_y * 2;
  
  while (children)
    {
      child = children->data;
      children = children->next;

      if (btk_widget_get_visible (child->widget))
	{
	  nchildren += 1;
	  btk_widget_size_request (child->widget, &child_requisition);

	  if (child_requisition.width + ipad_w > needed_width)
	    needed_width = child_requisition.width + ipad_w;
	  if (child_requisition.height + ipad_h > needed_height)
	    needed_height = child_requisition.height + ipad_h;
	  if (child->is_secondary)
	    nsecondaries++;
	}
    }

  if (nvis_children)
    *nvis_children = nchildren;
  if (nvis_secondaries)
    *nvis_secondaries = nsecondaries;
  if (width)
    *width = needed_width;
  if (height)
    *height = needed_height;
}

/* this is a kludge function to support the deprecated
 * btk_[vh]button_box_set_layout_default() just in case anyone is still
 * using it (why?)
 */
static BtkButtonBoxStyle
btk_button_box_kludge_get_layout_default (BtkButtonBox *widget)
{
  BtkOrientation orientation;

  orientation = btk_orientable_get_orientation (BTK_ORIENTABLE (widget));

  if (orientation == BTK_ORIENTATION_HORIZONTAL)
    return _btk_hbutton_box_get_layout_default ();
  else
    return _btk_vbutton_box_get_layout_default ();
}

static void
btk_button_box_size_request (BtkWidget      *widget,
                             BtkRequisition *requisition)
{
  BtkBox *box;
  BtkButtonBox *bbox;
  gint nvis_children;
  gint child_width;
  gint child_height;
  gint spacing;
  BtkButtonBoxStyle layout;
  BtkOrientation orientation;

  box = BTK_BOX (widget);
  bbox = BTK_BUTTON_BOX (widget);

  orientation = btk_orientable_get_orientation (BTK_ORIENTABLE (widget));
  spacing = box->spacing;
  layout = bbox->layout_style != BTK_BUTTONBOX_DEFAULT_STYLE
	  ? bbox->layout_style : btk_button_box_kludge_get_layout_default (BTK_BUTTON_BOX (widget));

  _btk_button_box_child_requisition (widget,
                                     &nvis_children,
				     NULL,
                                     &child_width,
                                     &child_height);

  if (nvis_children == 0)
    {
      requisition->width = 0;
      requisition->height = 0;
    }
  else
    {
      switch (layout)
        {
          case BTK_BUTTONBOX_SPREAD:
            if (orientation == BTK_ORIENTATION_HORIZONTAL)
              requisition->width =
                      nvis_children*child_width + ((nvis_children+1)*spacing);
            else
              requisition->height =
                      nvis_children*child_height + ((nvis_children+1)*spacing);

            break;
          case BTK_BUTTONBOX_EDGE:
          case BTK_BUTTONBOX_START:
          case BTK_BUTTONBOX_END:
          case BTK_BUTTONBOX_CENTER:
            if (orientation == BTK_ORIENTATION_HORIZONTAL)
              requisition->width =
                      nvis_children*child_width + ((nvis_children-1)*spacing);
            else
              requisition->height =
                      nvis_children*child_height + ((nvis_children-1)*spacing);

            break;
          default:
            g_assert_not_reached ();
            break;
        }

      if (orientation == BTK_ORIENTATION_HORIZONTAL)
        requisition->height = child_height;
      else
        requisition->width = child_width;
    }

  requisition->width += BTK_CONTAINER (box)->border_width * 2;
  requisition->height += BTK_CONTAINER (box)->border_width * 2;
}

static void
btk_button_box_size_allocate (BtkWidget     *widget,
                              BtkAllocation *allocation)
{
  BtkBox *base_box;
  BtkButtonBox *box;
  BtkBoxChild *child;
  GList *children;
  BtkAllocation child_allocation;
  gint nvis_children;
  gint n_secondaries;
  gint child_width;
  gint child_height;
  gint x = 0;
  gint y = 0;
  gint secondary_x = 0;
  gint secondary_y = 0;
  gint width = 0;
  gint height = 0;
  gint childspace;
  gint childspacing = 0;
  BtkButtonBoxStyle layout;
  gint spacing;
  BtkOrientation orientation;

  orientation = btk_orientable_get_orientation (BTK_ORIENTABLE (widget));
  base_box = BTK_BOX (widget);
  box = BTK_BUTTON_BOX (widget);
  spacing = base_box->spacing;
  layout = box->layout_style != BTK_BUTTONBOX_DEFAULT_STYLE
	  ? box->layout_style : btk_button_box_kludge_get_layout_default (BTK_BUTTON_BOX (widget));
  _btk_button_box_child_requisition (widget,
                                     &nvis_children,
                                     &n_secondaries,
                                     &child_width,
                                     &child_height);
  widget->allocation = *allocation;

  if (orientation == BTK_ORIENTATION_HORIZONTAL)
    width = allocation->width - BTK_CONTAINER (box)->border_width*2;
  else
    height = allocation->height - BTK_CONTAINER (box)->border_width*2;

  switch (layout)
    {
      case BTK_BUTTONBOX_SPREAD:

        if (orientation == BTK_ORIENTATION_HORIZONTAL)
          {
            childspacing = (width - (nvis_children * child_width))
                    / (nvis_children + 1);
            x = allocation->x + BTK_CONTAINER (box)->border_width
                    + childspacing;
            secondary_x = x + ((nvis_children - n_secondaries)
                            * (child_width + childspacing));
          }
        else
          {
            childspacing = (height - (nvis_children * child_height))
                    / (nvis_children + 1);
            y = allocation->y + BTK_CONTAINER (box)->border_width
                    + childspacing;
            secondary_y = y + ((nvis_children - n_secondaries)
                            * (child_height + childspacing));
          }

        break;

      case BTK_BUTTONBOX_EDGE:

        if (orientation == BTK_ORIENTATION_HORIZONTAL)
          {
            if (nvis_children >= 2)
              {
                childspacing = (width - (nvis_children * child_width))
                      / (nvis_children - 1);
                x = allocation->x + BTK_CONTAINER (box)->border_width;
                secondary_x = x + ((nvis_children - n_secondaries)
                                   * (child_width + childspacing));
              }
            else
              {
                /* one or zero children, just center */
                childspacing = width;
                x = secondary_x = allocation->x
                      + (allocation->width - child_width) / 2;
              }
          }
        else
          {
            if (nvis_children >= 2)
              {
                childspacing = (height - (nvis_children*child_height))
                        / (nvis_children-1);
                y = allocation->y + BTK_CONTAINER (box)->border_width;
                secondary_y = y + ((nvis_children - n_secondaries)
                                * (child_height + childspacing));
              }
            else
              {
                /* one or zero children, just center */
                childspacing = height;
                y = secondary_y = allocation->y
                        + (allocation->height - child_height) / 2;
              }
          }

        break;

      case BTK_BUTTONBOX_START:

        if (orientation == BTK_ORIENTATION_HORIZONTAL)
          {
            childspacing = spacing;
            x = allocation->x + BTK_CONTAINER (box)->border_width;
            secondary_x = allocation->x + allocation->width
              - child_width * n_secondaries
              - spacing * (n_secondaries - 1)
              - BTK_CONTAINER (box)->border_width;
          }
        else
          {
            childspacing = spacing;
            y = allocation->y + BTK_CONTAINER (box)->border_width;
            secondary_y = allocation->y + allocation->height
              - child_height * n_secondaries
              - spacing * (n_secondaries - 1)
              - BTK_CONTAINER (box)->border_width;
          }

        break;

      case BTK_BUTTONBOX_END:

        if (orientation == BTK_ORIENTATION_HORIZONTAL)
          {
            childspacing = spacing;
            x = allocation->x + allocation->width
              - child_width * (nvis_children - n_secondaries)
              - spacing * (nvis_children - n_secondaries - 1)
              - BTK_CONTAINER (box)->border_width;
            secondary_x = allocation->x + BTK_CONTAINER (box)->border_width;
          }
        else
          {
            childspacing = spacing;
            y = allocation->y + allocation->height
              - child_height * (nvis_children - n_secondaries)
              - spacing * (nvis_children - n_secondaries - 1)
              - BTK_CONTAINER (box)->border_width;
            secondary_y = allocation->y + BTK_CONTAINER (box)->border_width;
          }

        break;

      case BTK_BUTTONBOX_CENTER:

        if (orientation == BTK_ORIENTATION_HORIZONTAL)
          {
            childspacing = spacing;
            x = allocation->x +
              (allocation->width
               - (child_width * (nvis_children - n_secondaries)
               + spacing * (nvis_children - n_secondaries - 1))) / 2
              + (n_secondaries * child_width + n_secondaries * spacing) / 2;
            secondary_x = allocation->x + BTK_CONTAINER (box)->border_width;
          }
        else
          {
            childspacing = spacing;
            y = allocation->y +
              (allocation->height
               - (child_height * (nvis_children - n_secondaries)
                  + spacing * (nvis_children - n_secondaries - 1))) / 2
              + (n_secondaries * child_height + n_secondaries * spacing) / 2;
            secondary_y = allocation->y + BTK_CONTAINER (box)->border_width;
          }

        break;

      default:
        g_assert_not_reached ();
        break;
    }

    if (orientation == BTK_ORIENTATION_HORIZONTAL)
      {
        y = allocation->y + (allocation->height - child_height) / 2;
        childspace = child_width + childspacing;
      }
    else
      {
        x = allocation->x + (allocation->width - child_width) / 2;
        childspace = child_height + childspacing;
      }

  children = BTK_BOX (box)->children;

  while (children)
    {
      child = children->data;
      children = children->next;

      if (btk_widget_get_visible (child->widget))
        {
          child_allocation.width = child_width;
          child_allocation.height = child_height;

          if (orientation == BTK_ORIENTATION_HORIZONTAL)
            {
              child_allocation.y = y;

              if (child->is_secondary)
                {
                  child_allocation.x = secondary_x;
                  secondary_x += childspace;
                }
              else
                {
                  child_allocation.x = x;
                  x += childspace;
                }

              if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL)
                  child_allocation.x = (allocation->x + allocation->width)
                          - (child_allocation.x + child_width - allocation->x);
            }
          else
            {
              child_allocation.x = x;

              if (child->is_secondary)
                {
                  child_allocation.y = secondary_y;
                  secondary_y += childspace;
                }
              else
                {
                  child_allocation.y = y;
                  y += childspace;
                }
            }

          btk_widget_size_allocate (child->widget, &child_allocation);
        }
    }
}

#define __BTK_BUTTON_BOX_C__
#include "btkaliasdef.c"
