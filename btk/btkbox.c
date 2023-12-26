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

#include "btkbox.h"
#include "btkorientable.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

enum {
  PROP_0,
  PROP_ORIENTATION,
  PROP_SPACING,
  PROP_HOMOGENEOUS
};

enum {
  CHILD_PROP_0,
  CHILD_PROP_EXPAND,
  CHILD_PROP_FILL,
  CHILD_PROP_PADDING,
  CHILD_PROP_PACK_TYPE,
  CHILD_PROP_POSITION
};


typedef struct _BtkBoxPrivate BtkBoxPrivate;

struct _BtkBoxPrivate
{
  BtkOrientation orientation;
  guint          default_expand : 1;
  guint          spacing_set    : 1;
};

#define BTK_BOX_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_BOX, BtkBoxPrivate))


static void btk_box_set_property       (GObject        *object,
                                        guint           prop_id,
                                        const GValue   *value,
                                        GParamSpec     *pspec);
static void btk_box_get_property       (GObject        *object,
                                        guint           prop_id,
                                        GValue         *value,
                                        GParamSpec     *pspec);

static void btk_box_size_request       (BtkWidget      *widget,
                                        BtkRequisition *requisition);
static void btk_box_size_allocate      (BtkWidget      *widget,
                                        BtkAllocation  *allocation);

static void btk_box_add                (BtkContainer   *container,
                                        BtkWidget      *widget);
static void btk_box_remove             (BtkContainer   *container,
                                        BtkWidget      *widget);
static void btk_box_forall             (BtkContainer   *container,
                                        gboolean        include_internals,
                                        BtkCallback     callback,
                                        gpointer        callback_data);
static void btk_box_set_child_property (BtkContainer   *container,
                                        BtkWidget      *child,
                                        guint           property_id,
                                        const GValue   *value,
                                        GParamSpec     *pspec);
static void btk_box_get_child_property (BtkContainer   *container,
                                        BtkWidget      *child,
                                        guint           property_id,
                                        GValue         *value,
                                        GParamSpec     *pspec);
static GType btk_box_child_type        (BtkContainer   *container);


G_DEFINE_ABSTRACT_TYPE_WITH_CODE (BtkBox, btk_box, BTK_TYPE_CONTAINER,
                                  G_IMPLEMENT_INTERFACE (BTK_TYPE_ORIENTABLE,
                                                         NULL));

static void
btk_box_class_init (BtkBoxClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (class);
  BtkContainerClass *container_class = BTK_CONTAINER_CLASS (class);

  object_class->set_property = btk_box_set_property;
  object_class->get_property = btk_box_get_property;

  widget_class->size_request = btk_box_size_request;
  widget_class->size_allocate = btk_box_size_allocate;

  container_class->add = btk_box_add;
  container_class->remove = btk_box_remove;
  container_class->forall = btk_box_forall;
  container_class->child_type = btk_box_child_type;
  container_class->set_child_property = btk_box_set_child_property;
  container_class->get_child_property = btk_box_get_child_property;

  g_object_class_override_property (object_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  g_object_class_install_property (object_class,
                                   PROP_SPACING,
                                   g_param_spec_int ("spacing",
                                                     P_("Spacing"),
                                                     P_("The amount of space between children"),
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_HOMOGENEOUS,
                                   g_param_spec_boolean ("homogeneous",
							 P_("Homogeneous"),
							 P_("Whether the children should all be the same size"),
							 FALSE,
							 BTK_PARAM_READWRITE));

  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_EXPAND,
					      g_param_spec_boolean ("expand", 
								    P_("Expand"), 
								    P_("Whether the child should receive extra space when the parent grows"),
								    TRUE,
								    BTK_PARAM_READWRITE));
  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_FILL,
					      g_param_spec_boolean ("fill", 
								    P_("Fill"), 
								    P_("Whether extra space given to the child should be allocated to the child or used as padding"),
								    TRUE,
								    BTK_PARAM_READWRITE));
  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_PADDING,
					      g_param_spec_uint ("padding", 
								 P_("Padding"), 
								 P_("Extra space to put between the child and its neighbors, in pixels"),
								 0, G_MAXINT, 0,
								 BTK_PARAM_READWRITE));
  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_PACK_TYPE,
					      g_param_spec_enum ("pack-type", 
								 P_("Pack type"), 
								 P_("A BtkPackType indicating whether the child is packed with reference to the start or end of the parent"),
								 BTK_TYPE_PACK_TYPE, BTK_PACK_START,
								 BTK_PARAM_READWRITE));
  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_POSITION,
					      g_param_spec_int ("position", 
								P_("Position"), 
								P_("The index of the child in the parent"),
								-1, G_MAXINT, 0,
								BTK_PARAM_READWRITE));

  g_type_class_add_private (object_class, sizeof (BtkBoxPrivate));
}

static void
btk_box_init (BtkBox *box)
{
  BtkBoxPrivate *private = BTK_BOX_GET_PRIVATE (box);

  btk_widget_set_has_window (BTK_WIDGET (box), FALSE);
  btk_widget_set_redraw_on_allocate (BTK_WIDGET (box), FALSE);

  box->children = NULL;
  box->spacing = 0;
  box->homogeneous = FALSE;

  private->orientation = BTK_ORIENTATION_HORIZONTAL;
  private->default_expand = FALSE;
  private->spacing_set = FALSE;
}

static void
btk_box_set_property (GObject      *object,
                      guint         prop_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  BtkBox *box = BTK_BOX (object);
  BtkBoxPrivate *private = BTK_BOX_GET_PRIVATE (box);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      private->orientation = g_value_get_enum (value);
      btk_widget_queue_resize (BTK_WIDGET (box));
      break;
    case PROP_SPACING:
      btk_box_set_spacing (box, g_value_get_int (value));
      break;
    case PROP_HOMOGENEOUS:
      btk_box_set_homogeneous (box, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_box_get_property (GObject    *object,
                      guint       prop_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  BtkBox *box = BTK_BOX (object);
  BtkBoxPrivate *private = BTK_BOX_GET_PRIVATE (box);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, private->orientation);
      break;
    case PROP_SPACING:
      g_value_set_int (value, box->spacing);
      break;
    case PROP_HOMOGENEOUS:
      g_value_set_boolean (value, box->homogeneous);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_box_size_request (BtkWidget      *widget,
                      BtkRequisition *requisition)
{
  BtkBox *box = BTK_BOX (widget);
  BtkBoxPrivate *private = BTK_BOX_GET_PRIVATE (box);
  BtkBoxChild *child;
  GList *children;
  gint nvis_children;
  gint width;
  gint height;

  requisition->width = 0;
  requisition->height = 0;
  nvis_children = 0;

  children = box->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (btk_widget_get_visible (child->widget))
	{
	  BtkRequisition child_requisition;

	  btk_widget_size_request (child->widget, &child_requisition);

	  if (box->homogeneous)
	    {
	      width = child_requisition.width + child->padding * 2;
	      height = child_requisition.height + child->padding * 2;

              if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
                requisition->width = MAX (requisition->width, width);
              else
                requisition->height = MAX (requisition->height, height);
	    }
	  else
	    {
              if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
                requisition->width += child_requisition.width + child->padding * 2;
              else
                requisition->height += child_requisition.height + child->padding * 2;
	    }

          if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
            requisition->height = MAX (requisition->height, child_requisition.height);
          else
            requisition->width = MAX (requisition->width, child_requisition.width);

	  nvis_children += 1;
	}
    }

  if (nvis_children > 0)
    {
      if (box->homogeneous)
        {
          if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
            requisition->width *= nvis_children;
          else
            requisition->height *= nvis_children;
        }

      if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
        requisition->width += (nvis_children - 1) * box->spacing;
      else
        requisition->height += (nvis_children - 1) * box->spacing;
    }

  requisition->width += BTK_CONTAINER (box)->border_width * 2;
  requisition->height += BTK_CONTAINER (box)->border_width * 2;
}

static void
btk_box_size_allocate (BtkWidget     *widget,
                       BtkAllocation *allocation)
{
  BtkBox *box = BTK_BOX (widget);
  BtkBoxPrivate *private = BTK_BOX_GET_PRIVATE (box);
  BtkBoxChild *child;
  GList *children;
  BtkAllocation child_allocation;
  gint nvis_children = 0;
  gint nexpand_children = 0;
  gint child_width = 0;
  gint child_height = 0;
  gint width = 0;
  gint height = 0;
  gint extra = 0;
  gint x = 0;
  gint y = 0;
  BtkTextDirection direction;

  widget->allocation = *allocation;

  direction = btk_widget_get_direction (widget);

  for (children = box->children; children; children = children->next)
    {
      child = children->data;

      if (btk_widget_get_visible (child->widget))
	{
	  nvis_children += 1;
	  if (child->expand)
	    nexpand_children += 1;
	}
    }

  if (nvis_children > 0)
    {
      if (box->homogeneous)
	{
          if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
            {
              width = (allocation->width -
                       BTK_CONTAINER (box)->border_width * 2 -
                       (nvis_children - 1) * box->spacing);
              extra = width / nvis_children;
            }
          else
            {
              height = (allocation->height -
                        BTK_CONTAINER (box)->border_width * 2 -
                        (nvis_children - 1) * box->spacing);
              extra = height / nvis_children;
            }
	}
      else if (nexpand_children > 0)
	{
          if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
            {
              width = (gint) allocation->width - (gint) widget->requisition.width;
              extra = width / nexpand_children;
            }
          else
            {
              height = (gint) allocation->height - (gint) widget->requisition.height;
              extra = height / nexpand_children;
            }
	}

      if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
        {
          x = allocation->x + BTK_CONTAINER (box)->border_width;
          child_allocation.y = allocation->y + BTK_CONTAINER (box)->border_width;
          child_allocation.height = MAX (1, (gint) allocation->height - (gint) BTK_CONTAINER (box)->border_width * 2);
        }
      else
        {
          y = allocation->y + BTK_CONTAINER (box)->border_width;
          child_allocation.x = allocation->x + BTK_CONTAINER (box)->border_width;
          child_allocation.width = MAX (1, (gint) allocation->width - (gint) BTK_CONTAINER (box)->border_width * 2);
        }

      children = box->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if ((child->pack == BTK_PACK_START) && btk_widget_get_visible (child->widget))
	    {
	      if (box->homogeneous)
		{
		  if (nvis_children == 1)
                    {
                      child_width = width;
                      child_height = height;
                    }
		  else
                    {
                      child_width = extra;
                      child_height = extra;
                    }

		  nvis_children -= 1;
		  width -= extra;
                  height -= extra;
		}
	      else
		{
		  BtkRequisition child_requisition;

		  btk_widget_get_child_requisition (child->widget, &child_requisition);

		  child_width = child_requisition.width + child->padding * 2;
		  child_height = child_requisition.height + child->padding * 2;

		  if (child->expand)
		    {
		      if (nexpand_children == 1)
                        {
                          child_width += width;
                          child_height += height;
                        }
		      else
                        {
                          child_width += extra;
                          child_height += extra;
                        }

		      nexpand_children -= 1;
		      width -= extra;
                      height -= extra;
		    }
		}

	      if (child->fill)
		{
                  if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
                    {
                      child_allocation.width = MAX (1, (gint) child_width - (gint) child->padding * 2);
                      child_allocation.x = x + child->padding;
                    }
                  else
                    {
                      child_allocation.height = MAX (1, child_height - (gint)child->padding * 2);
                      child_allocation.y = y + child->padding;
                    }
		}
	      else
		{
		  BtkRequisition child_requisition;

		  btk_widget_get_child_requisition (child->widget, &child_requisition);
                  if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
                    {
                      child_allocation.width = child_requisition.width;
                      child_allocation.x = x + (child_width - child_allocation.width) / 2;
                    }
                  else
                    {
                      child_allocation.height = child_requisition.height;
                      child_allocation.y = y + (child_height - child_allocation.height) / 2;
                    }
		}

	      if (direction == BTK_TEXT_DIR_RTL &&
                  private->orientation == BTK_ORIENTATION_HORIZONTAL)
                {
                  child_allocation.x = allocation->x + allocation->width - (child_allocation.x - allocation->x) - child_allocation.width;
                }

	      btk_widget_size_allocate (child->widget, &child_allocation);

	      x += child_width + box->spacing;
	      y += child_height + box->spacing;
	    }
	}

      x = allocation->x + allocation->width - BTK_CONTAINER (box)->border_width;
      y = allocation->y + allocation->height - BTK_CONTAINER (box)->border_width;

      children = box->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if ((child->pack == BTK_PACK_END) && btk_widget_get_visible (child->widget))
	    {
	      BtkRequisition child_requisition;

	      btk_widget_get_child_requisition (child->widget, &child_requisition);

              if (box->homogeneous)
                {
                  if (nvis_children == 1)
                    {
                      child_width = width;
                      child_height = height;
                    }
                  else
                    {
                      child_width = extra;
                      child_height = extra;
                   }

                  nvis_children -= 1;
                  width -= extra;
                  height -= extra;
                }
              else
                {
		  child_width = child_requisition.width + child->padding * 2;
		  child_height = child_requisition.height + child->padding * 2;

                  if (child->expand)
                    {
                      if (nexpand_children == 1)
                        {
                          child_width += width;
                          child_height += height;
                         }
                      else
                        {
                          child_width += extra;
                          child_height += extra;
                        }

                      nexpand_children -= 1;
                      width -= extra;
                      height -= extra;
                    }
                }

              if (child->fill)
                {
                  if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
                    {
                      child_allocation.width = MAX (1, (gint)child_width - (gint)child->padding * 2);
                      child_allocation.x = x + child->padding - child_width;
                    }
                  else
                    {
                      child_allocation.height = MAX (1, child_height - (gint)child->padding * 2);
                      child_allocation.y = y + child->padding - child_height;
                     }
                }
              else
                {
                  if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
                    {
                      child_allocation.width = child_requisition.width;
                      child_allocation.x = x + (child_width - child_allocation.width) / 2 - child_width;
                    }
                  else
                    {
                      child_allocation.height = child_requisition.height;
                      child_allocation.y = y + (child_height - child_allocation.height) / 2 - child_height;
                    }
                }

	      if (direction == BTK_TEXT_DIR_RTL &&
                  private->orientation == BTK_ORIENTATION_HORIZONTAL)
                {
                  child_allocation.x = allocation->x + allocation->width - (child_allocation.x - allocation->x) - child_allocation.width;
                }

              btk_widget_size_allocate (child->widget, &child_allocation);

              x -= (child_width + box->spacing);
              y -= (child_height + box->spacing);
	    }
	}
    }
}

static GType
btk_box_child_type (BtkContainer   *container)
{
  return BTK_TYPE_WIDGET;
}

static void
btk_box_set_child_property (BtkContainer *container,
                            BtkWidget    *child,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  gboolean expand = 0;
  gboolean fill = 0;
  guint padding = 0;
  BtkPackType pack_type = 0;

  if (property_id != CHILD_PROP_POSITION)
    btk_box_query_child_packing (BTK_BOX (container),
				 child,
				 &expand,
				 &fill,
				 &padding,
				 &pack_type);
  switch (property_id)
    {
    case CHILD_PROP_EXPAND:
      btk_box_set_child_packing (BTK_BOX (container),
				 child,
				 g_value_get_boolean (value),
				 fill,
				 padding,
				 pack_type);
      break;
    case CHILD_PROP_FILL:
      btk_box_set_child_packing (BTK_BOX (container),
				 child,
				 expand,
				 g_value_get_boolean (value),
				 padding,
				 pack_type);
      break;
    case CHILD_PROP_PADDING:
      btk_box_set_child_packing (BTK_BOX (container),
				 child,
				 expand,
				 fill,
				 g_value_get_uint (value),
				 pack_type);
      break;
    case CHILD_PROP_PACK_TYPE:
      btk_box_set_child_packing (BTK_BOX (container),
				 child,
				 expand,
				 fill,
				 padding,
				 g_value_get_enum (value));
      break;
    case CHILD_PROP_POSITION:
      btk_box_reorder_child (BTK_BOX (container),
			     child,
			     g_value_get_int (value));
      break;
    default:
      BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
btk_box_get_child_property (BtkContainer *container,
			    BtkWidget    *child,
			    guint         property_id,
			    GValue       *value,
			    GParamSpec   *pspec)
{
  gboolean expand = 0;
  gboolean fill = 0;
  guint padding = 0;
  BtkPackType pack_type = 0;
  GList *list;

  if (property_id != CHILD_PROP_POSITION)
    btk_box_query_child_packing (BTK_BOX (container),
				 child,
				 &expand,
				 &fill,
				 &padding,
				 &pack_type);
  switch (property_id)
    {
      guint i;
    case CHILD_PROP_EXPAND:
      g_value_set_boolean (value, expand);
      break;
    case CHILD_PROP_FILL:
      g_value_set_boolean (value, fill);
      break;
    case CHILD_PROP_PADDING:
      g_value_set_uint (value, padding);
      break;
    case CHILD_PROP_PACK_TYPE:
      g_value_set_enum (value, pack_type);
      break;
    case CHILD_PROP_POSITION:
      i = 0;
      for (list = BTK_BOX (container)->children; list; list = list->next)
	{
	  BtkBoxChild *child_entry;

	  child_entry = list->data;
	  if (child_entry->widget == child)
	    break;
	  i++;
	}
      g_value_set_int (value, list ? i : -1);
      break;
    default:
      BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
btk_box_pack (BtkBox      *box,
              BtkWidget   *child,
              gboolean     expand,
              gboolean     fill,
              guint        padding,
              BtkPackType  pack_type)
{
  BtkBoxChild *child_info;

  g_return_if_fail (BTK_IS_BOX (box));
  g_return_if_fail (BTK_IS_WIDGET (child));
  g_return_if_fail (child->parent == NULL);

  child_info = g_new (BtkBoxChild, 1);
  child_info->widget = child;
  child_info->padding = padding;
  child_info->expand = expand ? TRUE : FALSE;
  child_info->fill = fill ? TRUE : FALSE;
  child_info->pack = pack_type;
  child_info->is_secondary = FALSE;

  box->children = g_list_append (box->children, child_info);

  btk_widget_freeze_child_notify (child);

  btk_widget_set_parent (child, BTK_WIDGET (box));
  
  btk_widget_child_notify (child, "expand");
  btk_widget_child_notify (child, "fill");
  btk_widget_child_notify (child, "padding");
  btk_widget_child_notify (child, "pack-type");
  btk_widget_child_notify (child, "position");
  btk_widget_thaw_child_notify (child);
}

/**
 * btk_box_new:
 * @orientation: the box' orientation.
 * @homogeneous: %TRUE if all children are to be given equal space allocations.
 * @spacing: the number of pixels to place by default between children.
 *
 * Creates a new #BtkHBox.
 *
 * Return value: a new #BtkHBox.
 *
 * Since: 2.16
 **/
BtkWidget*
_btk_box_new (BtkOrientation orientation,
              gboolean       homogeneous,
              gint           spacing)
{
  return g_object_new (BTK_TYPE_BOX,
                       "orientation", orientation,
                       "spacing",     spacing,
                       "homogeneous", homogeneous ? TRUE : FALSE,
                       NULL);
}

/**
 * btk_box_pack_start:
 * @box: a #BtkBox
 * @child: the #BtkWidget to be added to @box
 * @expand: %TRUE if the new child is to be given extra space allocated to
 * @box.  The extra space will be divided evenly between all children of
 * @box that use this option
 * @fill: %TRUE if space given to @child by the @expand option is
 *   actually allocated to @child, rather than just padding it.  This
 *   parameter has no effect if @expand is set to %FALSE.  A child is
 *   always allocated the full height of a #BtkHBox and the full width 
 *   of a #BtkVBox. This option affects the other dimension
 * @padding: extra space in pixels to put between this child and its
 *   neighbors, over and above the global amount specified by
 *   #BtkBox:spacing property.  If @child is a widget at one of the 
 *   reference ends of @box, then @padding pixels are also put between 
 *   @child and the reference edge of @box
 *
 * Adds @child to @box, packed with reference to the start of @box.
 * The @child is packed after any other child packed with reference 
 * to the start of @box.
 */
void
btk_box_pack_start (BtkBox    *box,
		    BtkWidget *child,
		    gboolean   expand,
		    gboolean   fill,
		    guint      padding)
{
  btk_box_pack (box, child, expand, fill, padding, BTK_PACK_START);
}

/**
 * btk_box_pack_end:
 * @box: a #BtkBox
 * @child: the #BtkWidget to be added to @box
 * @expand: %TRUE if the new child is to be given extra space allocated 
 *   to @box. The extra space will be divided evenly between all children 
 *   of @box that use this option
 * @fill: %TRUE if space given to @child by the @expand option is
 *   actually allocated to @child, rather than just padding it.  This
 *   parameter has no effect if @expand is set to %FALSE.  A child is
 *   always allocated the full height of a #BtkHBox and the full width 
 *   of a #BtkVBox.  This option affects the other dimension
 * @padding: extra space in pixels to put between this child and its
 *   neighbors, over and above the global amount specified by
 *   #BtkBox:spacing property.  If @child is a widget at one of the 
 *   reference ends of @box, then @padding pixels are also put between 
 *   @child and the reference edge of @box
 *
 * Adds @child to @box, packed with reference to the end of @box.  
 * The @child is packed after (away from end of) any other child 
 * packed with reference to the end of @box.
 */
void
btk_box_pack_end (BtkBox    *box,
		  BtkWidget *child,
		  gboolean   expand,
		  gboolean   fill,
		  guint      padding)
{
  btk_box_pack (box, child, expand, fill, padding, BTK_PACK_END);
}

/**
 * btk_box_pack_start_defaults:
 * @box: a #BtkBox
 * @widget: the #BtkWidget to be added to @box
 *
 * Adds @widget to @box, packed with reference to the start of @box.
 * The child is packed after any other child packed with reference 
 * to the start of @box. 
 * 
 * Parameters for how to pack the child @widget, #BtkBox:expand, 
 * #BtkBox:fill and #BtkBox:padding, are given their default
 * values, %TRUE, %TRUE, and 0, respectively.
 *
 * Deprecated: 2.14: Use btk_box_pack_start()
 */
void
btk_box_pack_start_defaults (BtkBox    *box,
			     BtkWidget *child)
{
  btk_box_pack_start (box, child, TRUE, TRUE, 0);
}

/**
 * btk_box_pack_end_defaults:
 * @box: a #BtkBox
 * @widget: the #BtkWidget to be added to @box
 *
 * Adds @widget to @box, packed with reference to the end of @box.
 * The child is packed after any other child packed with reference 
 * to the start of @box. 
 * 
 * Parameters for how to pack the child @widget, #BtkBox:expand, 
 * #BtkBox:fill and #BtkBox:padding, are given their default
 * values, %TRUE, %TRUE, and 0, respectively.
 *
 * Deprecated: 2.14: Use btk_box_pack_end()
 */
void
btk_box_pack_end_defaults (BtkBox    *box,
			   BtkWidget *child)
{
  btk_box_pack_end (box, child, TRUE, TRUE, 0);
}

/**
 * btk_box_set_homogeneous:
 * @box: a #BtkBox
 * @homogeneous: a boolean value, %TRUE to create equal allotments,
 *   %FALSE for variable allotments
 * 
 * Sets the #BtkBox:homogeneous property of @box, controlling 
 * whether or not all children of @box are given equal space 
 * in the box.
 */
void
btk_box_set_homogeneous (BtkBox  *box,
			 gboolean homogeneous)
{
  g_return_if_fail (BTK_IS_BOX (box));

  if ((homogeneous ? TRUE : FALSE) != box->homogeneous)
    {
      box->homogeneous = homogeneous ? TRUE : FALSE;
      g_object_notify (G_OBJECT (box), "homogeneous");
      btk_widget_queue_resize (BTK_WIDGET (box));
    }
}

/**
 * btk_box_get_homogeneous:
 * @box: a #BtkBox
 *
 * Returns whether the box is homogeneous (all children are the
 * same size). See btk_box_set_homogeneous().
 *
 * Return value: %TRUE if the box is homogeneous.
 **/
gboolean
btk_box_get_homogeneous (BtkBox *box)
{
  g_return_val_if_fail (BTK_IS_BOX (box), FALSE);

  return box->homogeneous;
}

/**
 * btk_box_set_spacing:
 * @box: a #BtkBox
 * @spacing: the number of pixels to put between children
 *
 * Sets the #BtkBox:spacing property of @box, which is the 
 * number of pixels to place between children of @box.
 */
void
btk_box_set_spacing (BtkBox *box,
		     gint    spacing)
{
  g_return_if_fail (BTK_IS_BOX (box));

  if (spacing != box->spacing)
    {
      box->spacing = spacing;
      _btk_box_set_spacing_set (box, TRUE);

      g_object_notify (G_OBJECT (box), "spacing");

      btk_widget_queue_resize (BTK_WIDGET (box));
    }
}

/**
 * btk_box_get_spacing:
 * @box: a #BtkBox
 * 
 * Gets the value set by btk_box_set_spacing().
 * 
 * Return value: spacing between children
 **/
gint
btk_box_get_spacing (BtkBox *box)
{
  g_return_val_if_fail (BTK_IS_BOX (box), 0);

  return box->spacing;
}

void
_btk_box_set_spacing_set (BtkBox  *box,
                          gboolean spacing_set)
{
  BtkBoxPrivate *private;

  g_return_if_fail (BTK_IS_BOX (box));

  private = BTK_BOX_GET_PRIVATE (box);

  private->spacing_set = spacing_set ? TRUE : FALSE;
}

gboolean
_btk_box_get_spacing_set (BtkBox *box)
{
  BtkBoxPrivate *private;

  g_return_val_if_fail (BTK_IS_BOX (box), FALSE);

  private = BTK_BOX_GET_PRIVATE (box);

  return private->spacing_set;
}

/**
 * btk_box_reorder_child:
 * @box: a #BtkBox
 * @child: the #BtkWidget to move
 * @position: the new position for @child in the list of children 
 *   of @box, starting from 0. If negative, indicates the end of 
 *   the list
 *
 * Moves @child to a new @position in the list of @box children.  
 * The list is the <structfield>children</structfield> field of
 * #BtkBox-struct, and contains both widgets packed #BTK_PACK_START 
 * as well as widgets packed #BTK_PACK_END, in the order that these 
 * widgets were added to @box.
 * 
 * A widget's position in the @box children list determines where 
 * the widget is packed into @box.  A child widget at some position 
 * in the list will be packed just after all other widgets of the 
 * same packing type that appear earlier in the list.
 */ 
void
btk_box_reorder_child (BtkBox    *box,
		       BtkWidget *child,
		       gint       position)
{
  GList *old_link;
  GList *new_link;
  BtkBoxChild *child_info = NULL;
  gint old_position;

  g_return_if_fail (BTK_IS_BOX (box));
  g_return_if_fail (BTK_IS_WIDGET (child));

  old_link = box->children;
  old_position = 0;
  while (old_link)
    {
      child_info = old_link->data;
      if (child_info->widget == child)
	break;

      old_link = old_link->next;
      old_position++;
    }

  g_return_if_fail (old_link != NULL);

  if (position == old_position)
    return;

  box->children = g_list_delete_link (box->children, old_link);

  if (position < 0)
    new_link = NULL;
  else
    new_link = g_list_nth (box->children, position);

  box->children = g_list_insert_before (box->children, new_link, child_info);

  btk_widget_child_notify (child, "position");
  if (btk_widget_get_visible (child)
      && btk_widget_get_visible (BTK_WIDGET (box)))
    btk_widget_queue_resize (child);
}

/**
 * btk_box_query_child_packing:
 * @box: a #BtkBox
 * @child: the #BtkWidget of the child to query
 * @expand: pointer to return location for #BtkBox:expand child property 
 * @fill: pointer to return location for #BtkBox:fill child property 
 * @padding: pointer to return location for #BtkBox:padding child property 
 * @pack_type: pointer to return location for #BtkBox:pack-type child property 
 * 
 * Obtains information about how @child is packed into @box.
 */
void
btk_box_query_child_packing (BtkBox      *box,
			     BtkWidget   *child,
			     gboolean    *expand,
			     gboolean    *fill,
			     guint       *padding,
			     BtkPackType *pack_type)
{
  GList *list;
  BtkBoxChild *child_info = NULL;

  g_return_if_fail (BTK_IS_BOX (box));
  g_return_if_fail (BTK_IS_WIDGET (child));

  list = box->children;
  while (list)
    {
      child_info = list->data;
      if (child_info->widget == child)
	break;

      list = list->next;
    }

  if (list)
    {
      if (expand)
	*expand = child_info->expand;
      if (fill)
	*fill = child_info->fill;
      if (padding)
	*padding = child_info->padding;
      if (pack_type)
	*pack_type = child_info->pack;
    }
}

/**
 * btk_box_set_child_packing:
 * @box: a #BtkBox
 * @child: the #BtkWidget of the child to set
 * @expand: the new value of the #BtkBox:expand child property 
 * @fill: the new value of the #BtkBox:fill child property
 * @padding: the new value of the #BtkBox:padding child property
 * @pack_type: the new value of the #BtkBox:pack-type child property
 *
 * Sets the way @child is packed into @box.
 */
void
btk_box_set_child_packing (BtkBox      *box,
			   BtkWidget   *child,
			   gboolean     expand,
			   gboolean     fill,
			   guint        padding,
			   BtkPackType  pack_type)
{
  GList *list;
  BtkBoxChild *child_info = NULL;

  g_return_if_fail (BTK_IS_BOX (box));
  g_return_if_fail (BTK_IS_WIDGET (child));

  list = box->children;
  while (list)
    {
      child_info = list->data;
      if (child_info->widget == child)
	break;

      list = list->next;
    }

  btk_widget_freeze_child_notify (child);
  if (list)
    {
      child_info->expand = expand != FALSE;
      btk_widget_child_notify (child, "expand");
      child_info->fill = fill != FALSE;
      btk_widget_child_notify (child, "fill");
      child_info->padding = padding;
      btk_widget_child_notify (child, "padding");
      if (pack_type == BTK_PACK_END)
	child_info->pack = BTK_PACK_END;
      else
	child_info->pack = BTK_PACK_START;
      btk_widget_child_notify (child, "pack-type");

      if (btk_widget_get_visible (child)
          && btk_widget_get_visible (BTK_WIDGET (box)))
	btk_widget_queue_resize (child);
    }
  btk_widget_thaw_child_notify (child);
}

void
_btk_box_set_old_defaults (BtkBox *box)
{
  BtkBoxPrivate *private;

  g_return_if_fail (BTK_IS_BOX (box));

  private = BTK_BOX_GET_PRIVATE (box);

  private->default_expand = TRUE;
}

static void
btk_box_add (BtkContainer *container,
	     BtkWidget    *widget)
{
  BtkBoxPrivate *private = BTK_BOX_GET_PRIVATE (container);

  btk_box_pack_start (BTK_BOX (container), widget,
                      private->default_expand,
                      private->default_expand,
                      0);
}

static void
btk_box_remove (BtkContainer *container,
		BtkWidget    *widget)
{
  BtkBox *box = BTK_BOX (container);
  BtkBoxChild *child;
  GList *children;

  children = box->children;
  while (children)
    {
      child = children->data;

      if (child->widget == widget)
	{
	  gboolean was_visible;

	  was_visible = btk_widget_get_visible (widget);
	  btk_widget_unparent (widget);

	  box->children = g_list_remove_link (box->children, children);
	  g_list_free (children);
	  g_free (child);

	  /* queue resize regardless of btk_widget_get_visible (container),
	   * since that's what is needed by toplevels.
	   */
	  if (was_visible)
	    btk_widget_queue_resize (BTK_WIDGET (container));

	  break;
	}

      children = children->next;
    }
}

static void
btk_box_forall (BtkContainer *container,
		gboolean      include_internals,
		BtkCallback   callback,
		gpointer      callback_data)
{
  BtkBox *box = BTK_BOX (container);
  BtkBoxChild *child;
  GList *children;

  children = box->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (child->pack == BTK_PACK_START)
	(* callback) (child->widget, callback_data);
    }

  children = g_list_last (box->children);
  while (children)
    {
      child = children->data;
      children = children->prev;

      if (child->pack == BTK_PACK_END)
	(* callback) (child->widget, callback_data);
    }
}

#define __BTK_BOX_C__
#include "btkaliasdef.c"
