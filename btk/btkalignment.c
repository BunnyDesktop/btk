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

/**
 * SECTION:btkalignment
 * @Short_description: A widget which controls the alignment and size of its child
 * @Title: BtkAlignment
 *
 * The #BtkAlignment widget controls the alignment and size of its child widget.
 * It has four settings: xscale, yscale, xalign, and yalign.
 *
 * The scale settings are used to specify how much the child widget should
 * expand to fill the space allocated to the #BtkAlignment.
 * The values can range from 0 (meaning the child doesn't expand at all) to
 * 1 (meaning the child expands to fill all of the available space).
 *
 * The align settings are used to place the child widget within the available
 * area. The values range from 0 (top or left) to 1 (bottom or right).
 * Of course, if the scale settings are both set to 1, the alignment settings
 * have no effect.
 */

#include "config.h"
#include "btkalignment.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

enum {
  PROP_0,

  PROP_XALIGN,
  PROP_YALIGN,
  PROP_XSCALE,
  PROP_YSCALE,

  PROP_TOP_PADDING,
  PROP_BOTTOM_PADDING,
  PROP_LEFT_PADDING,
  PROP_RIGHT_PADDING
};

#define BTK_ALIGNMENT_GET_PRIVATE(o)  (B_TYPE_INSTANCE_GET_PRIVATE ((o), BTK_TYPE_ALIGNMENT, BtkAlignmentPrivate))

struct _BtkAlignmentPrivate
{
  buint padding_top;
  buint padding_bottom;
  buint padding_left;
  buint padding_right;
};

static void btk_alignment_size_request  (BtkWidget         *widget,
					 BtkRequisition    *requisition);
static void btk_alignment_size_allocate (BtkWidget         *widget,
					 BtkAllocation     *allocation);
static void btk_alignment_set_property (BObject         *object,
                                        buint            prop_id,
                                        const BValue    *value,
                                        BParamSpec      *pspec);
static void btk_alignment_get_property (BObject         *object,
                                        buint            prop_id,
                                        BValue          *value,
                                        BParamSpec      *pspec);

G_DEFINE_TYPE (BtkAlignment, btk_alignment, BTK_TYPE_BIN)

static void
btk_alignment_class_init (BtkAlignmentClass *class)
{
  BObjectClass *bobject_class;
  BtkWidgetClass *widget_class;

  bobject_class = (BObjectClass*) class;
  widget_class = (BtkWidgetClass*) class;
  
  bobject_class->set_property = btk_alignment_set_property;
  bobject_class->get_property = btk_alignment_get_property;

  widget_class->size_request = btk_alignment_size_request;
  widget_class->size_allocate = btk_alignment_size_allocate;

  g_object_class_install_property (bobject_class,
                                   PROP_XALIGN,
                                   g_param_spec_float("xalign",
                                                      P_("Horizontal alignment"),
                                                      P_("Horizontal position of child in available space. 0.0 is left aligned, 1.0 is right aligned"),
                                                      0.0,
                                                      1.0,
                                                      0.5,
                                                      BTK_PARAM_READWRITE));
   
  g_object_class_install_property (bobject_class,
                                   PROP_YALIGN,
                                   g_param_spec_float("yalign",
                                                      P_("Vertical alignment"),
                                                      P_("Vertical position of child in available space. 0.0 is top aligned, 1.0 is bottom aligned"),
                                                      0.0,
                                                      1.0,
						      0.5,
                                                      BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_XSCALE,
                                   g_param_spec_float("xscale",
                                                      P_("Horizontal scale"),
                                                      P_("If available horizontal space is bigger than needed for the child, how much of it to use for the child. 0.0 means none, 1.0 means all"),
                                                      0.0,
                                                      1.0,
                                                      1.0,
                                                      BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_YSCALE,
                                   g_param_spec_float("yscale",
                                                      P_("Vertical scale"),
                                                      P_("If available vertical space is bigger than needed for the child, how much of it to use for the child. 0.0 means none, 1.0 means all"),
                                                      0.0,
                                                      1.0,
                                                      1.0,
                                                      BTK_PARAM_READWRITE));


/**
 * BtkAlignment:top-padding:
 *
 * The padding to insert at the top of the widget.
 *
 * Since: 2.4
 */
  g_object_class_install_property (bobject_class,
                                   PROP_TOP_PADDING,
                                   g_param_spec_uint("top-padding",
                                                      P_("Top Padding"),
                                                      P_("The padding to insert at the top of the widget."),
                                                      0,
                                                      B_MAXINT,
                                                      0,
                                                      BTK_PARAM_READWRITE));

/**
 * BtkAlignment:bottom-padding:
 *
 * The padding to insert at the bottom of the widget.
 *
 * Since: 2.4
 */
  g_object_class_install_property (bobject_class,
                                   PROP_BOTTOM_PADDING,
                                   g_param_spec_uint("bottom-padding",
                                                      P_("Bottom Padding"),
                                                      P_("The padding to insert at the bottom of the widget."),
                                                      0,
                                                      B_MAXINT,
                                                      0,
                                                      BTK_PARAM_READWRITE));

/**
 * BtkAlignment:left-padding:
 *
 * The padding to insert at the left of the widget.
 *
 * Since: 2.4
 */
  g_object_class_install_property (bobject_class,
                                   PROP_LEFT_PADDING,
                                   g_param_spec_uint("left-padding",
                                                      P_("Left Padding"),
                                                      P_("The padding to insert at the left of the widget."),
                                                      0,
                                                      B_MAXINT,
                                                      0,
                                                      BTK_PARAM_READWRITE));

/**
 * BtkAlignment:right-padding:
 *
 * The padding to insert at the right of the widget.
 *
 * Since: 2.4
 */
  g_object_class_install_property (bobject_class,
                                   PROP_RIGHT_PADDING,
                                   g_param_spec_uint("right-padding",
                                                      P_("Right Padding"),
                                                      P_("The padding to insert at the right of the widget."),
                                                      0,
                                                      B_MAXINT,
                                                      0,
                                                      BTK_PARAM_READWRITE));

  g_type_class_add_private (bobject_class, sizeof (BtkAlignmentPrivate));  
}

static void
btk_alignment_init (BtkAlignment *alignment)
{
  BtkAlignmentPrivate *priv;
  
  btk_widget_set_has_window (BTK_WIDGET (alignment), FALSE);
  btk_widget_set_redraw_on_allocate (BTK_WIDGET (alignment), FALSE);

  alignment->xalign = 0.5;
  alignment->yalign = 0.5;
  alignment->xscale = 1.0;
  alignment->yscale = 1.0;

  /* Initialize padding with default values: */
  priv = BTK_ALIGNMENT_GET_PRIVATE (alignment);
  priv->padding_top = 0;
  priv->padding_bottom = 0;
  priv->padding_left = 0;
  priv->padding_right = 0;
}

/**
 * btk_alignment_new:
 * @xalign: the horizontal alignment of the child widget, from 0 (left) to 1
 *  (right).
 * @yalign: the vertical alignment of the child widget, from 0 (top) to 1
 *  (bottom).
 * @xscale: the amount that the child widget expands horizontally to fill up
 *  unused space, from 0 to 1.
 *  A value of 0 indicates that the child widget should never expand.
 *  A value of 1 indicates that the child widget will expand to fill all of the
 *  space allocated for the #BtkAlignment.
 * @yscale: the amount that the child widget expands vertically to fill up
 *  unused space, from 0 to 1. The values are similar to @xscale.
 *
 * Creates a new #BtkAlignment.
 *
 * Returns: the new #BtkAlignment.
 */
BtkWidget*
btk_alignment_new (bfloat xalign,
		   bfloat yalign,
		   bfloat xscale,
		   bfloat yscale)
{
  BtkAlignment *alignment;

  alignment = g_object_new (BTK_TYPE_ALIGNMENT, NULL);

  alignment->xalign = CLAMP (xalign, 0.0, 1.0);
  alignment->yalign = CLAMP (yalign, 0.0, 1.0);
  alignment->xscale = CLAMP (xscale, 0.0, 1.0);
  alignment->yscale = CLAMP (yscale, 0.0, 1.0);

  return BTK_WIDGET (alignment);
}

static void
btk_alignment_set_property (BObject         *object,
			    buint            prop_id,
			    const BValue    *value,
			    BParamSpec      *pspec)
{
  BtkAlignment *alignment;
  BtkAlignmentPrivate *priv;
  
  alignment = BTK_ALIGNMENT (object);
  priv = BTK_ALIGNMENT_GET_PRIVATE (alignment);
  
  switch (prop_id)
    {
    case PROP_XALIGN:
      btk_alignment_set (alignment,
			 b_value_get_float (value),
			 alignment->yalign,
			 alignment->xscale,
			 alignment->yscale);
      break;
    case PROP_YALIGN:
      btk_alignment_set (alignment,
			 alignment->xalign,
			 b_value_get_float (value),
			 alignment->xscale,
			 alignment->yscale);
      break;
    case PROP_XSCALE:
      btk_alignment_set (alignment,
			 alignment->xalign,
			 alignment->yalign,
			 b_value_get_float (value),
			 alignment->yscale);
      break;
    case PROP_YSCALE:
      btk_alignment_set (alignment,
			 alignment->xalign,
			 alignment->yalign,
			 alignment->xscale,
			 b_value_get_float (value));
      break;
      
    /* Padding: */
    case PROP_TOP_PADDING:
      btk_alignment_set_padding (alignment,
			 b_value_get_uint (value),
			 priv->padding_bottom,
			 priv->padding_left,
			 priv->padding_right);
      break;
    case PROP_BOTTOM_PADDING:
      btk_alignment_set_padding (alignment,
			 priv->padding_top,
			 b_value_get_uint (value),
			 priv->padding_left,
			 priv->padding_right);
      break;
    case PROP_LEFT_PADDING:
      btk_alignment_set_padding (alignment,
			 priv->padding_top,
			 priv->padding_bottom,
			 b_value_get_uint (value),
			 priv->padding_right);
      break;
    case PROP_RIGHT_PADDING:
      btk_alignment_set_padding (alignment,
			 priv->padding_top,
			 priv->padding_bottom,
			 priv->padding_left,
			 b_value_get_uint (value));
      break;
    
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_alignment_get_property (BObject         *object,
			    buint            prop_id,
			    BValue          *value,
			    BParamSpec      *pspec)
{
  BtkAlignment *alignment;
  BtkAlignmentPrivate *priv;

  alignment = BTK_ALIGNMENT (object);
  priv = BTK_ALIGNMENT_GET_PRIVATE (alignment);
   
  switch (prop_id)
    {
    case PROP_XALIGN:
      b_value_set_float(value, alignment->xalign);
      break;
    case PROP_YALIGN:
      b_value_set_float(value, alignment->yalign);
      break;
    case PROP_XSCALE:
      b_value_set_float(value, alignment->xscale);
      break;
    case PROP_YSCALE:
      b_value_set_float(value, alignment->yscale);
      break;

    /* Padding: */
    case PROP_TOP_PADDING:
      b_value_set_uint (value, priv->padding_top);
      break;
    case PROP_BOTTOM_PADDING:
      b_value_set_uint (value, priv->padding_bottom);
      break;
    case PROP_LEFT_PADDING:
      b_value_set_uint (value, priv->padding_left);
      break;
    case PROP_RIGHT_PADDING:
      b_value_set_uint (value, priv->padding_right);
      break;
      
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * btk_alignment_set:
 * @alignment: a #BtkAlignment.
 * @xalign: the horizontal alignment of the child widget, from 0 (left) to 1
 *  (right).
 * @yalign: the vertical alignment of the child widget, from 0 (top) to 1
 *  (bottom).
 * @xscale: the amount that the child widget expands horizontally to fill up
 *  unused space, from 0 to 1.
 *  A value of 0 indicates that the child widget should never expand.
 *  A value of 1 indicates that the child widget will expand to fill all of the
 *  space allocated for the #BtkAlignment.
 * @yscale: the amount that the child widget expands vertically to fill up
 *  unused space, from 0 to 1. The values are similar to @xscale.
 *
 * Sets the #BtkAlignment values.
 */
void
btk_alignment_set (BtkAlignment *alignment,
		   bfloat        xalign,
		   bfloat        yalign,
		   bfloat        xscale,
		   bfloat        yscale)
{
  g_return_if_fail (BTK_IS_ALIGNMENT (alignment));

  xalign = CLAMP (xalign, 0.0, 1.0);
  yalign = CLAMP (yalign, 0.0, 1.0);
  xscale = CLAMP (xscale, 0.0, 1.0);
  yscale = CLAMP (yscale, 0.0, 1.0);

  if (   (alignment->xalign != xalign)
      || (alignment->yalign != yalign)
      || (alignment->xscale != xscale)
      || (alignment->yscale != yscale))
    {
      g_object_freeze_notify (B_OBJECT (alignment));
      if (alignment->xalign != xalign)
        {
           alignment->xalign = xalign;
           g_object_notify (B_OBJECT (alignment), "xalign");
        }
      if (alignment->yalign != yalign)
        {
           alignment->yalign = yalign;
           g_object_notify (B_OBJECT (alignment), "yalign");
        }
      if (alignment->xscale != xscale)
        {
           alignment->xscale = xscale;
           g_object_notify (B_OBJECT (alignment), "xscale");
        }
      if (alignment->yscale != yscale)
        {
           alignment->yscale = yscale;
           g_object_notify (B_OBJECT (alignment), "yscale");
        }
      g_object_thaw_notify (B_OBJECT (alignment));

      if (BTK_BIN (alignment)->child)
        btk_widget_queue_resize (BTK_BIN (alignment)->child);
      btk_widget_queue_draw (BTK_WIDGET (alignment));
    }
}


static void
btk_alignment_size_request (BtkWidget      *widget,
			    BtkRequisition *requisition)
{
  BtkBin *bin;
  BtkAlignmentPrivate *priv;

  bin = BTK_BIN (widget);
  priv = BTK_ALIGNMENT_GET_PRIVATE (widget);

  requisition->width = BTK_CONTAINER (widget)->border_width * 2;
  requisition->height = BTK_CONTAINER (widget)->border_width * 2;

  if (bin->child && btk_widget_get_visible (bin->child))
    {
      BtkRequisition child_requisition;
      
      btk_widget_size_request (bin->child, &child_requisition);

      requisition->width += child_requisition.width;
      requisition->height += child_requisition.height;

      /* Request extra space for the padding: */
      requisition->width += (priv->padding_left + priv->padding_right);
      requisition->height += (priv->padding_top + priv->padding_bottom);
    }
}

static void
btk_alignment_size_allocate (BtkWidget     *widget,
			     BtkAllocation *allocation)
{
  BtkAlignment *alignment;
  BtkBin *bin;
  BtkAllocation child_allocation;
  BtkRequisition child_requisition;
  bint width, height;
  bint border_width;
  bint padding_horizontal, padding_vertical;
  BtkAlignmentPrivate *priv;

  padding_horizontal = 0;
  padding_vertical = 0;

  widget->allocation = *allocation;
  alignment = BTK_ALIGNMENT (widget);
  bin = BTK_BIN (widget);
  
  if (bin->child && btk_widget_get_visible (bin->child))
    {
      btk_widget_get_child_requisition (bin->child, &child_requisition);

      border_width = BTK_CONTAINER (alignment)->border_width;

      priv = BTK_ALIGNMENT_GET_PRIVATE (widget);
      padding_horizontal = priv->padding_left + priv->padding_right;
      padding_vertical = priv->padding_top + priv->padding_bottom;

      width  = MAX (1, allocation->width - padding_horizontal - 2 * border_width);
      height = MAX (1, allocation->height - padding_vertical - 2 * border_width);
    
      if (width > child_requisition.width)
	child_allocation.width = (child_requisition.width *
				  (1.0 - alignment->xscale) +
				  width * alignment->xscale);
      else
	child_allocation.width = width;
      
      if (height > child_requisition.height)
	child_allocation.height = (child_requisition.height *
				   (1.0 - alignment->yscale) +
				   height * alignment->yscale);
      else
	child_allocation.height = height;

      if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL)
	child_allocation.x = (1.0 - alignment->xalign) * (width - child_allocation.width) + allocation->x + border_width + priv->padding_right;
      else 
	child_allocation.x = alignment->xalign * (width - child_allocation.width) + allocation->x + border_width + priv->padding_left;

      child_allocation.y = alignment->yalign * (height - child_allocation.height) + allocation->y + border_width + priv->padding_top;

      btk_widget_size_allocate (bin->child, &child_allocation);
    }
}

/**
 * btk_alignment_set_padding:
 * @alignment: a #BtkAlignment
 * @padding_top: the padding at the top of the widget
 * @padding_bottom: the padding at the bottom of the widget
 * @padding_left: the padding at the left of the widget
 * @padding_right: the padding at the right of the widget.
 *
 * Sets the padding on the different sides of the widget.
 * The padding adds blank space to the sides of the widget. For instance,
 * this can be used to indent the child widget towards the right by adding
 * padding on the left.
 *
 * Since: 2.4
 */
void
btk_alignment_set_padding (BtkAlignment    *alignment,
			   buint            padding_top,
			   buint            padding_bottom,
			   buint            padding_left,
			   buint            padding_right)
{
  BtkAlignmentPrivate *priv;
  
  g_return_if_fail (BTK_IS_ALIGNMENT (alignment));

  priv = BTK_ALIGNMENT_GET_PRIVATE (alignment);

  g_object_freeze_notify (B_OBJECT (alignment));

  if (priv->padding_top != padding_top)
    {
      priv->padding_top = padding_top;
      g_object_notify (B_OBJECT (alignment), "top-padding");
    }
  if (priv->padding_bottom != padding_bottom)
    {
      priv->padding_bottom = padding_bottom;
      g_object_notify (B_OBJECT (alignment), "bottom-padding");
    }
  if (priv->padding_left != padding_left)
    {
      priv->padding_left = padding_left;
      g_object_notify (B_OBJECT (alignment), "left-padding");
    }
  if (priv->padding_right != padding_right)
    {
      priv->padding_right = padding_right;
      g_object_notify (B_OBJECT (alignment), "right-padding");
    }

  g_object_thaw_notify (B_OBJECT (alignment));
  
  /* Make sure that the widget and children are redrawn with the new setting: */
  if (BTK_BIN (alignment)->child)
    btk_widget_queue_resize (BTK_BIN (alignment)->child);

  btk_widget_queue_draw (BTK_WIDGET (alignment));
}

/**
 * btk_alignment_get_padding:
 * @alignment: a #BtkAlignment
 * @padding_top: (out) (allow-none): location to store the padding for
 *     the top of the widget, or %NULL
 * @padding_bottom: (out) (allow-none): location to store the padding
 *     for the bottom of the widget, or %NULL
 * @padding_left: (out) (allow-none): location to store the padding
 *     for the left of the widget, or %NULL
 * @padding_right: (out) (allow-none): location to store the padding
 *     for the right of the widget, or %NULL
 *
 * Gets the padding on the different sides of the widget.
 * See btk_alignment_set_padding ().
 *
 * Since: 2.4
 */
void
btk_alignment_get_padding (BtkAlignment    *alignment,
			   buint           *padding_top,
			   buint           *padding_bottom,
			   buint           *padding_left,
			   buint           *padding_right)
{
  BtkAlignmentPrivate *priv;
 
  g_return_if_fail (BTK_IS_ALIGNMENT (alignment));

  priv = BTK_ALIGNMENT_GET_PRIVATE (alignment);
  if(padding_top)
    *padding_top = priv->padding_top;
  if(padding_bottom)
    *padding_bottom = priv->padding_bottom;
  if(padding_left)
    *padding_left = priv->padding_left;
  if(padding_right)
    *padding_right = priv->padding_right;
}

#define __BTK_ALIGNMENT_C__
#include "btkaliasdef.c"
