/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * BtkAspectFrame: Ensure that the child window has a specified aspect ratio
 *    or, if obey_child, has the same aspect ratio as its requested size
 *
 *     Copyright Owen Taylor                          4/9/97
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
 * Modified by the BTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

/**
 * SECTION:btkaspectframe
 * @Short_description: A frame that constrains its child to a particular aspect ratio
 * @Title: BtkAspectFrame
 *
 * The #BtkAspectFrame is useful when you want
 * pack a widget so that it can resize but always retains
 * the same aspect ratio. For instance, one might be
 * drawing a small preview of a larger image. #BtkAspectFrame
 * derives from #BtkFrame, so it can draw a label and
 * a frame around the child. The frame will be
 * "shrink-wrapped" to the size of the child.
 */

#include "config.h"
#include "btkaspectframe.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

enum {
  PROP_0,
  PROP_XALIGN,
  PROP_YALIGN,
  PROP_RATIO,
  PROP_OBEY_CHILD
};

static void btk_aspect_frame_set_property (BObject         *object,
					   guint            prop_id,
					   const BValue    *value,
					   BParamSpec      *pspec);
static void btk_aspect_frame_get_property (BObject         *object,
					   guint            prop_id,
					   BValue          *value,
					   BParamSpec      *pspec);
static void btk_aspect_frame_compute_child_allocation (BtkFrame            *frame,
						       BtkAllocation       *child_allocation);

#define MAX_RATIO 10000.0
#define MIN_RATIO 0.0001

G_DEFINE_TYPE (BtkAspectFrame, btk_aspect_frame, BTK_TYPE_FRAME)

static void
btk_aspect_frame_class_init (BtkAspectFrameClass *class)
{
  BObjectClass *bobject_class;
  BtkFrameClass *frame_class;
  
  bobject_class = (BObjectClass*) class;
  frame_class = (BtkFrameClass*) class;
  
  bobject_class->set_property = btk_aspect_frame_set_property;
  bobject_class->get_property = btk_aspect_frame_get_property;

  frame_class->compute_child_allocation = btk_aspect_frame_compute_child_allocation;

  g_object_class_install_property (bobject_class,
                                   PROP_XALIGN,
                                   g_param_spec_float ("xalign",
                                                       P_("Horizontal Alignment"),
                                                       P_("X alignment of the child"),
                                                       0.0, 1.0, 0.5,
                                                       BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_YALIGN,
                                   g_param_spec_float ("yalign",
                                                       P_("Vertical Alignment"),
                                                       P_("Y alignment of the child"),
                                                       0.0, 1.0, 0.5,
                                                       BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_RATIO,
                                   g_param_spec_float ("ratio",
                                                       P_("Ratio"),
                                                       P_("Aspect ratio if obey_child is FALSE"),
                                                       MIN_RATIO, MAX_RATIO, 1.0,
                                                       BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_OBEY_CHILD,
                                   g_param_spec_boolean ("obey-child",
                                                         P_("Obey child"),
                                                         P_("Force aspect ratio to match that of the frame's child"),
                                                         TRUE,
                                                         BTK_PARAM_READWRITE));
}

static void
btk_aspect_frame_init (BtkAspectFrame *aspect_frame)
{
  aspect_frame->xalign = 0.5;
  aspect_frame->yalign = 0.5;
  aspect_frame->ratio = 1.0;
  aspect_frame->obey_child = TRUE;
}

static void
btk_aspect_frame_set_property (BObject         *object,
			       guint            prop_id,
			       const BValue    *value,
			       BParamSpec      *pspec)
{
  BtkAspectFrame *aspect_frame = BTK_ASPECT_FRAME (object);
  
  switch (prop_id)
    {
      /* g_object_notify is handled by the _frame_set function */
    case PROP_XALIGN:
      btk_aspect_frame_set (aspect_frame,
			    b_value_get_float (value),
			    aspect_frame->yalign,
			    aspect_frame->ratio,
			    aspect_frame->obey_child);
      break;
    case PROP_YALIGN:
      btk_aspect_frame_set (aspect_frame,
			    aspect_frame->xalign,
			    b_value_get_float (value),
			    aspect_frame->ratio,
			    aspect_frame->obey_child);
      break;
    case PROP_RATIO:
      btk_aspect_frame_set (aspect_frame,
			    aspect_frame->xalign,
			    aspect_frame->yalign,
			    b_value_get_float (value),
			    aspect_frame->obey_child);
      break;
    case PROP_OBEY_CHILD:
      btk_aspect_frame_set (aspect_frame,
			    aspect_frame->xalign,
			    aspect_frame->yalign,
			    aspect_frame->ratio,
			    b_value_get_boolean (value));
      break;
    default:
       B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_aspect_frame_get_property (BObject         *object,
			       guint            prop_id,
			       BValue          *value,
			       BParamSpec      *pspec)
{
  BtkAspectFrame *aspect_frame = BTK_ASPECT_FRAME (object);
  
  switch (prop_id)
    {
    case PROP_XALIGN:
      b_value_set_float (value, aspect_frame->xalign);
      break;
    case PROP_YALIGN:
      b_value_set_float (value, aspect_frame->yalign);
      break;
    case PROP_RATIO:
      b_value_set_float (value, aspect_frame->ratio);
      break;
    case PROP_OBEY_CHILD:
      b_value_set_boolean (value, aspect_frame->obey_child);
      break;
    default:
       B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * btk_aspect_frame_new:
 * @label: Label text.
 * @xalign: Horizontal alignment of the child within the allocation of
 *  the #BtkAspectFrame. This ranges from 0.0 (left aligned)
 *  to 1.0 (right aligned)
 * @yalign: Vertical alignment of the child within the allocation of
 *  the #BtkAspectFrame. This ranges from 0.0 (left aligned)
 *  to 1.0 (right aligned)
 * @ratio: The desired aspect ratio.
 * @obey_child: If %TRUE, @ratio is ignored, and the aspect
 *  ratio is taken from the requistion of the child.
 *
 * Create a new #BtkAspectFrame.
 *
 * Returns: the new #BtkAspectFrame.
 */
BtkWidget*
btk_aspect_frame_new (const gchar *label,
		      gfloat       xalign,
		      gfloat       yalign,
		      gfloat       ratio,
		      gboolean     obey_child)
{
  BtkAspectFrame *aspect_frame;

  aspect_frame = g_object_new (BTK_TYPE_ASPECT_FRAME, NULL);

  aspect_frame->xalign = CLAMP (xalign, 0.0, 1.0);
  aspect_frame->yalign = CLAMP (yalign, 0.0, 1.0);
  aspect_frame->ratio = CLAMP (ratio, MIN_RATIO, MAX_RATIO);
  aspect_frame->obey_child = obey_child != FALSE;

  btk_frame_set_label (BTK_FRAME(aspect_frame), label);

  return BTK_WIDGET (aspect_frame);
}

/**
 * btk_aspect_frame_set:
 * @aspect_frame: a #BtkAspectFrame
 * @xalign: Horizontal alignment of the child within the allocation of
 *  the #BtkAspectFrame. This ranges from 0.0 (left aligned)
 *  to 1.0 (right aligned)
 * @yalign: Vertical alignment of the child within the allocation of
 *  the #BtkAspectFrame. This ranges from 0.0 (left aligned)
 *  to 1.0 (right aligned)
 * @ratio: The desired aspect ratio.
 * @obey_child: If %TRUE, @ratio is ignored, and the aspect
 *  ratio is taken from the requistion of the child.
 *
 * Set parameters for an existing #BtkAspectFrame.
 */
void
btk_aspect_frame_set (BtkAspectFrame *aspect_frame,
		      gfloat          xalign,
		      gfloat          yalign,
		      gfloat          ratio,
		      gboolean        obey_child)
{
  g_return_if_fail (BTK_IS_ASPECT_FRAME (aspect_frame));
  
  xalign = CLAMP (xalign, 0.0, 1.0);
  yalign = CLAMP (yalign, 0.0, 1.0);
  ratio = CLAMP (ratio, MIN_RATIO, MAX_RATIO);
  obey_child = obey_child != FALSE;
  
  if (   (aspect_frame->xalign != xalign)
      || (aspect_frame->yalign != yalign)
      || (aspect_frame->ratio != ratio)
      || (aspect_frame->obey_child != obey_child))
    {
      g_object_freeze_notify (B_OBJECT (aspect_frame));

      if (aspect_frame->xalign != xalign)
        {
          aspect_frame->xalign = xalign;
          g_object_notify (B_OBJECT (aspect_frame), "xalign");
        }
      if (aspect_frame->yalign != yalign)
        {
          aspect_frame->yalign = yalign;
          g_object_notify (B_OBJECT (aspect_frame), "yalign");
        }
      if (aspect_frame->ratio != ratio)
        {
          aspect_frame->ratio = ratio;
          g_object_notify (B_OBJECT (aspect_frame), "ratio");
        }
      if (aspect_frame->obey_child != obey_child)
        {
          aspect_frame->obey_child = obey_child;
          g_object_notify (B_OBJECT (aspect_frame), "obey-child");
        }
      g_object_thaw_notify (B_OBJECT (aspect_frame));

      btk_widget_queue_resize (BTK_WIDGET (aspect_frame));
    }
}

static void
btk_aspect_frame_compute_child_allocation (BtkFrame      *frame,
					   BtkAllocation *child_allocation)
{
  BtkAspectFrame *aspect_frame = BTK_ASPECT_FRAME (frame);
  BtkBin *bin = BTK_BIN (frame);
  gdouble ratio;

  if (bin->child && btk_widget_get_visible (bin->child))
    {
      BtkAllocation full_allocation;
      
      if (aspect_frame->obey_child)
	{
	  BtkRequisition child_requisition;

	  btk_widget_get_child_requisition (bin->child, &child_requisition);
	  if (child_requisition.height != 0)
	    {
	      ratio = ((gdouble) child_requisition.width /
		       child_requisition.height);
	      if (ratio < MIN_RATIO)
		ratio = MIN_RATIO;
	    }
	  else if (child_requisition.width != 0)
	    ratio = MAX_RATIO;
	  else
	    ratio = 1.0;
	}
      else
	ratio = aspect_frame->ratio;

      BTK_FRAME_CLASS (btk_aspect_frame_parent_class)->compute_child_allocation (frame, &full_allocation);
      
      if (ratio * full_allocation.height > full_allocation.width)
	{
	  child_allocation->width = full_allocation.width;
	  child_allocation->height = full_allocation.width / ratio + 0.5;
	}
      else
	{
	  child_allocation->width = ratio * full_allocation.height + 0.5;
	  child_allocation->height = full_allocation.height;
	}
      
      child_allocation->x = full_allocation.x + aspect_frame->xalign * (full_allocation.width - child_allocation->width);
      child_allocation->y = full_allocation.y + aspect_frame->yalign * (full_allocation.height - child_allocation->height);
    }
  else
    BTK_FRAME_CLASS (btk_aspect_frame_parent_class)->compute_child_allocation (frame, child_allocation);
}

#define __BTK_ASPECT_FRAME_C__
#include "btkaliasdef.c"
