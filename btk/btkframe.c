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
#include <string.h>
#include "btkframe.h"
#include "btklabel.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkbuildable.h"
#include "btkalias.h"

#define LABEL_PAD 1
#define LABEL_SIDE_PAD 2

enum {
  PROP_0,
  PROP_LABEL,
  PROP_LABEL_XALIGN,
  PROP_LABEL_YALIGN,
  PROP_SHADOW,
  PROP_SHADOW_TYPE,
  PROP_LABEL_WIDGET
};

static void btk_frame_set_property (BObject      *object,
				    guint         param_id,
				    const BValue *value,
				    BParamSpec   *pspec);
static void btk_frame_get_property (BObject     *object,
				    guint        param_id,
				    BValue      *value,
				    BParamSpec  *pspec);
static void btk_frame_paint         (BtkWidget      *widget,
				     BdkRectangle   *area);
static gint btk_frame_expose        (BtkWidget      *widget,
				     BdkEventExpose *event);
static void btk_frame_size_request  (BtkWidget      *widget,
				     BtkRequisition *requisition);
static void btk_frame_size_allocate (BtkWidget      *widget,
				     BtkAllocation  *allocation);
static void btk_frame_remove        (BtkContainer   *container,
				     BtkWidget      *child);
static void btk_frame_forall        (BtkContainer   *container,
				     gboolean	     include_internals,
			             BtkCallback     callback,
			             gpointer        callback_data);

static void btk_frame_compute_child_allocation      (BtkFrame      *frame,
						     BtkAllocation *child_allocation);
static void btk_frame_real_compute_child_allocation (BtkFrame      *frame,
						     BtkAllocation *child_allocation);

/* BtkBuildable */
static void btk_frame_buildable_init                (BtkBuildableIface *iface);
static void btk_frame_buildable_add_child           (BtkBuildable *buildable,
						     BtkBuilder   *builder,
						     BObject      *child,
						     const gchar  *type);

G_DEFINE_TYPE_WITH_CODE (BtkFrame, btk_frame, BTK_TYPE_BIN,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
						btk_frame_buildable_init))

static void
btk_frame_class_init (BtkFrameClass *class)
{
  BObjectClass *bobject_class;
  BtkWidgetClass *widget_class;
  BtkContainerClass *container_class;

  bobject_class = (BObjectClass*) class;
  widget_class = BTK_WIDGET_CLASS (class);
  container_class = BTK_CONTAINER_CLASS (class);

  bobject_class->set_property = btk_frame_set_property;
  bobject_class->get_property = btk_frame_get_property;

  g_object_class_install_property (bobject_class,
                                   PROP_LABEL,
                                   g_param_spec_string ("label",
                                                        P_("Label"),
                                                        P_("Text of the frame's label"),
                                                        NULL,
                                                        BTK_PARAM_READABLE |
							BTK_PARAM_WRITABLE));
  g_object_class_install_property (bobject_class,
				   PROP_LABEL_XALIGN,
				   g_param_spec_float ("label-xalign",
						       P_("Label xalign"),
						       P_("The horizontal alignment of the label"),
						       0.0,
						       1.0,
						       0.0,
						       BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_LABEL_YALIGN,
				   g_param_spec_float ("label-yalign",
						       P_("Label yalign"),
						       P_("The vertical alignment of the label"),
						       0.0,
						       1.0,
						       0.5,
						       BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_SHADOW,
                                   g_param_spec_enum ("shadow", NULL,
                                                      P_("Deprecated property, use shadow_type instead"),
						      BTK_TYPE_SHADOW_TYPE,
						      BTK_SHADOW_ETCHED_IN,
                                                      BTK_PARAM_READWRITE | G_PARAM_DEPRECATED));
  g_object_class_install_property (bobject_class,
                                   PROP_SHADOW_TYPE,
                                   g_param_spec_enum ("shadow-type",
                                                      P_("Frame shadow"),
                                                      P_("Appearance of the frame border"),
						      BTK_TYPE_SHADOW_TYPE,
						      BTK_SHADOW_ETCHED_IN,
                                                      BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_LABEL_WIDGET,
                                   g_param_spec_object ("label-widget",
                                                        P_("Label widget"),
                                                        P_("A widget to display in place of the usual frame label"),
                                                        BTK_TYPE_WIDGET,
                                                        BTK_PARAM_READWRITE));
  
  widget_class->expose_event = btk_frame_expose;
  widget_class->size_request = btk_frame_size_request;
  widget_class->size_allocate = btk_frame_size_allocate;

  container_class->remove = btk_frame_remove;
  container_class->forall = btk_frame_forall;

  class->compute_child_allocation = btk_frame_real_compute_child_allocation;
}

static void
btk_frame_buildable_init (BtkBuildableIface *iface)
{
  iface->add_child = btk_frame_buildable_add_child;
}

static void
btk_frame_buildable_add_child (BtkBuildable *buildable,
			       BtkBuilder   *builder,
			       BObject      *child,
			       const gchar  *type)
{
  if (type && strcmp (type, "label") == 0)
    btk_frame_set_label_widget (BTK_FRAME (buildable), BTK_WIDGET (child));
  else if (!type)
    btk_container_add (BTK_CONTAINER (buildable), BTK_WIDGET (child));
  else
    BTK_BUILDER_WARN_INVALID_CHILD_TYPE (BTK_FRAME (buildable), type);
}

static void
btk_frame_init (BtkFrame *frame)
{
  frame->label_widget = NULL;
  frame->shadow_type = BTK_SHADOW_ETCHED_IN;
  frame->label_xalign = 0.0;
  frame->label_yalign = 0.5;
}

static void 
btk_frame_set_property (BObject         *object,
			guint            prop_id,
			const BValue    *value,
			BParamSpec      *pspec)
{
  BtkFrame *frame;

  frame = BTK_FRAME (object);

  switch (prop_id)
    {
    case PROP_LABEL:
      btk_frame_set_label (frame, b_value_get_string (value));
      break;
    case PROP_LABEL_XALIGN:
      btk_frame_set_label_align (frame, b_value_get_float (value), 
				 frame->label_yalign);
      break;
    case PROP_LABEL_YALIGN:
      btk_frame_set_label_align (frame, frame->label_xalign, 
				 b_value_get_float (value));
      break;
    case PROP_SHADOW:
    case PROP_SHADOW_TYPE:
      btk_frame_set_shadow_type (frame, b_value_get_enum (value));
      break;
    case PROP_LABEL_WIDGET:
      btk_frame_set_label_widget (frame, b_value_get_object (value));
      break;
    default:      
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
btk_frame_get_property (BObject         *object,
			guint            prop_id,
			BValue          *value,
			BParamSpec      *pspec)
{
  BtkFrame *frame;

  frame = BTK_FRAME (object);

  switch (prop_id)
    {
    case PROP_LABEL:
      b_value_set_string (value, btk_frame_get_label (frame));
      break;
    case PROP_LABEL_XALIGN:
      b_value_set_float (value, frame->label_xalign);
      break;
    case PROP_LABEL_YALIGN:
      b_value_set_float (value, frame->label_yalign);
      break;
    case PROP_SHADOW:
    case PROP_SHADOW_TYPE:
      b_value_set_enum (value, frame->shadow_type);
      break;
    case PROP_LABEL_WIDGET:
      b_value_set_object (value,
                          frame->label_widget ?
                          B_OBJECT (frame->label_widget) : NULL);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * btk_frame_new:
 * @label: the text to use as the label of the frame
 * 
 * Creates a new #BtkFrame, with optional label @label.
 * If @label is %NULL, the label is omitted.
 * 
 * Return value: a new #BtkFrame widget
 **/
BtkWidget*
btk_frame_new (const gchar *label)
{
  return g_object_new (BTK_TYPE_FRAME, "label", label, NULL);
}

static void
btk_frame_remove (BtkContainer *container,
		  BtkWidget    *child)
{
  BtkFrame *frame = BTK_FRAME (container);

  if (frame->label_widget == child)
    btk_frame_set_label_widget (frame, NULL);
  else
    BTK_CONTAINER_CLASS (btk_frame_parent_class)->remove (container, child);
}

static void
btk_frame_forall (BtkContainer *container,
		  gboolean      include_internals,
		  BtkCallback   callback,
		  gpointer      callback_data)
{
  BtkBin *bin = BTK_BIN (container);
  BtkFrame *frame = BTK_FRAME (container);

  if (bin->child)
    (* callback) (bin->child, callback_data);

  if (frame->label_widget)
    (* callback) (frame->label_widget, callback_data);
}

/**
 * btk_frame_set_label:
 * @frame: a #BtkFrame
 * @label: (allow-none): the text to use as the label of the frame
 *
 * Sets the text of the label. If @label is %NULL,
 * the current label is removed.
 **/
void
btk_frame_set_label (BtkFrame *frame,
		     const gchar *label)
{
  g_return_if_fail (BTK_IS_FRAME (frame));

  if (!label)
    {
      btk_frame_set_label_widget (frame, NULL);
    }
  else
    {
      BtkWidget *child = btk_label_new (label);
      btk_widget_show (child);

      btk_frame_set_label_widget (frame, child);
    }
}

/**
 * btk_frame_get_label:
 * @frame: a #BtkFrame
 * 
 * If the frame's label widget is a #BtkLabel, returns the
 * text in the label widget. (The frame will have a #BtkLabel
 * for the label widget if a non-%NULL argument was passed
 * to btk_frame_new().)
 * 
 * Return value: the text in the label, or %NULL if there
 *               was no label widget or the lable widget was not
 *               a #BtkLabel. This string is owned by BTK+ and
 *               must not be modified or freed.
 **/
const gchar *
btk_frame_get_label (BtkFrame *frame)
{
  g_return_val_if_fail (BTK_IS_FRAME (frame), NULL);

  if (BTK_IS_LABEL (frame->label_widget))
    return btk_label_get_text (BTK_LABEL (frame->label_widget));
  else
    return NULL;
}

/**
 * btk_frame_set_label_widget:
 * @frame: a #BtkFrame
 * @label_widget: the new label widget
 * 
 * Sets the label widget for the frame. This is the widget that
 * will appear embedded in the top edge of the frame as a
 * title.
 **/
void
btk_frame_set_label_widget (BtkFrame  *frame,
			    BtkWidget *label_widget)
{
  gboolean need_resize = FALSE;
  
  g_return_if_fail (BTK_IS_FRAME (frame));
  g_return_if_fail (label_widget == NULL || BTK_IS_WIDGET (label_widget));
  g_return_if_fail (label_widget == NULL || label_widget->parent == NULL);
  
  if (frame->label_widget == label_widget)
    return;
  
  if (frame->label_widget)
    {
      need_resize = btk_widget_get_visible (frame->label_widget);
      btk_widget_unparent (frame->label_widget);
    }

  frame->label_widget = label_widget;
    
  if (label_widget)
    {
      frame->label_widget = label_widget;
      btk_widget_set_parent (label_widget, BTK_WIDGET (frame));
      need_resize |= btk_widget_get_visible (label_widget);
    }
  
  if (btk_widget_get_visible (BTK_WIDGET (frame)) && need_resize)
    btk_widget_queue_resize (BTK_WIDGET (frame));

  g_object_freeze_notify (B_OBJECT (frame));
  g_object_notify (B_OBJECT (frame), "label-widget");
  g_object_notify (B_OBJECT (frame), "label");
  g_object_thaw_notify (B_OBJECT (frame));
}

/**
 * btk_frame_get_label_widget:
 * @frame: a #BtkFrame
 *
 * Retrieves the label widget for the frame. See
 * btk_frame_set_label_widget().
 *
 * Return value: (transfer none): the label widget, or %NULL if there is none.
 **/
BtkWidget *
btk_frame_get_label_widget (BtkFrame *frame)
{
  g_return_val_if_fail (BTK_IS_FRAME (frame), NULL);

  return frame->label_widget;
}

/**
 * btk_frame_set_label_align:
 * @frame: a #BtkFrame
 * @xalign: The position of the label along the top edge
 *   of the widget. A value of 0.0 represents left alignment;
 *   1.0 represents right alignment.
 * @yalign: The y alignment of the label. A value of 0.0 aligns under 
 *   the frame; 1.0 aligns above the frame. If the values are exactly
 *   0.0 or 1.0 the gap in the frame won't be painted because the label
 *   will be completely above or below the frame.
 * 
 * Sets the alignment of the frame widget's label. The
 * default values for a newly created frame are 0.0 and 0.5.
 **/
void
btk_frame_set_label_align (BtkFrame *frame,
			   gfloat    xalign,
			   gfloat    yalign)
{
  g_return_if_fail (BTK_IS_FRAME (frame));

  xalign = CLAMP (xalign, 0.0, 1.0);
  yalign = CLAMP (yalign, 0.0, 1.0);

  g_object_freeze_notify (B_OBJECT (frame));
  if (xalign != frame->label_xalign)
    {
      frame->label_xalign = xalign;
      g_object_notify (B_OBJECT (frame), "label-xalign");
    }

  if (yalign != frame->label_yalign)
    {
      frame->label_yalign = yalign;
      g_object_notify (B_OBJECT (frame), "label-yalign");
    }

  g_object_thaw_notify (B_OBJECT (frame));
  btk_widget_queue_resize (BTK_WIDGET (frame));
}

/**
 * btk_frame_get_label_align:
 * @frame: a #BtkFrame
 * @xalign: (out) (allow-none): location to store X alignment of
 *     frame's label, or %NULL
 * @yalign: (out) (allow-none): location to store X alignment of
 *     frame's label, or %NULL
 * 
 * Retrieves the X and Y alignment of the frame's label. See
 * btk_frame_set_label_align().
 **/
void
btk_frame_get_label_align (BtkFrame *frame,
		           gfloat   *xalign,
			   gfloat   *yalign)
{
  g_return_if_fail (BTK_IS_FRAME (frame));

  if (xalign)
    *xalign = frame->label_xalign;
  if (yalign)
    *yalign = frame->label_yalign;
}

/**
 * btk_frame_set_shadow_type:
 * @frame: a #BtkFrame
 * @type: the new #BtkShadowType
 * 
 * Sets the shadow type for @frame.
 **/
void
btk_frame_set_shadow_type (BtkFrame      *frame,
			   BtkShadowType  type)
{
  BtkWidget *widget;

  g_return_if_fail (BTK_IS_FRAME (frame));

  if ((BtkShadowType) frame->shadow_type != type)
    {
      widget = BTK_WIDGET (frame);
      frame->shadow_type = type;
      g_object_notify (B_OBJECT (frame), "shadow-type");

      if (btk_widget_is_drawable (widget))
	{
	  btk_widget_queue_draw (widget);
	}
      
      btk_widget_queue_resize (widget);
    }
}

/**
 * btk_frame_get_shadow_type:
 * @frame: a #BtkFrame
 *
 * Retrieves the shadow type of the frame. See
 * btk_frame_set_shadow_type().
 *
 * Return value: the current shadow type of the frame.
 **/
BtkShadowType
btk_frame_get_shadow_type (BtkFrame *frame)
{
  g_return_val_if_fail (BTK_IS_FRAME (frame), BTK_SHADOW_ETCHED_IN);

  return frame->shadow_type;
}

static void
btk_frame_paint (BtkWidget    *widget,
		 BdkRectangle *area)
{
  BtkFrame *frame;
  gint x, y, width, height;

  if (btk_widget_is_drawable (widget))
    {
      frame = BTK_FRAME (widget);

      x = frame->child_allocation.x - widget->style->xthickness;
      y = frame->child_allocation.y - widget->style->ythickness;
      width = frame->child_allocation.width + 2 * widget->style->xthickness;
      height =  frame->child_allocation.height + 2 * widget->style->ythickness;

      if (frame->label_widget)
	{
	  BtkRequisition child_requisition;
	  gfloat xalign;
	  gint height_extra;
	  gint x2;

	  btk_widget_get_child_requisition (frame->label_widget, &child_requisition);

	  if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_LTR)
	    xalign = frame->label_xalign;
	  else
	    xalign = 1 - frame->label_xalign;

	  height_extra = MAX (0, child_requisition.height - widget->style->ythickness)
	    - frame->label_yalign * child_requisition.height;
	  y -= height_extra;
	  height += height_extra;
	  
	  x2 = widget->style->xthickness + (frame->child_allocation.width - child_requisition.width - 2 * LABEL_PAD - 2 * LABEL_SIDE_PAD) * xalign + LABEL_SIDE_PAD;
	  
	  /* If the label is completely over or under the frame we can omit the gap */
	  if (frame->label_yalign == 0.0 || frame->label_yalign == 1.0)
	    btk_paint_shadow (widget->style, widget->window,
			      widget->state, frame->shadow_type,
			      area, widget, "frame",
			      x, y, width, height);
	  else
	    btk_paint_shadow_gap (widget->style, widget->window,
				  widget->state, frame->shadow_type,
				  area, widget, "frame",
				  x, y, width, height,
				  BTK_POS_TOP,
				  x2, child_requisition.width + 2 * LABEL_PAD);
	}
       else
	 btk_paint_shadow (widget->style, widget->window,
			   widget->state, frame->shadow_type,
			   area, widget, "frame",
			   x, y, width, height);
    }
}

static gboolean
btk_frame_expose (BtkWidget      *widget,
		  BdkEventExpose *event)
{
  if (btk_widget_is_drawable (widget))
    {
      btk_frame_paint (widget, &event->area);

      BTK_WIDGET_CLASS (btk_frame_parent_class)->expose_event (widget, event);
    }

  return FALSE;
}

static void
btk_frame_size_request (BtkWidget      *widget,
			BtkRequisition *requisition)
{
  BtkFrame *frame = BTK_FRAME (widget);
  BtkBin *bin = BTK_BIN (widget);
  BtkRequisition child_requisition;
  
  if (frame->label_widget && btk_widget_get_visible (frame->label_widget))
    {
      btk_widget_size_request (frame->label_widget, &child_requisition);

      requisition->width = child_requisition.width + 2 * LABEL_PAD + 2 * LABEL_SIDE_PAD;
      requisition->height =
	MAX (0, child_requisition.height - widget->style->ythickness);
    }
  else
    {
      requisition->width = 0;
      requisition->height = 0;
    }
  
  if (bin->child && btk_widget_get_visible (bin->child))
    {
      btk_widget_size_request (bin->child, &child_requisition);

      requisition->width = MAX (requisition->width, child_requisition.width);
      requisition->height += child_requisition.height;
    }

  requisition->width += (BTK_CONTAINER (widget)->border_width +
			 BTK_WIDGET (widget)->style->xthickness) * 2;
  requisition->height += (BTK_CONTAINER (widget)->border_width +
			  BTK_WIDGET (widget)->style->ythickness) * 2;
}

static void
btk_frame_size_allocate (BtkWidget     *widget,
			 BtkAllocation *allocation)
{
  BtkFrame *frame = BTK_FRAME (widget);
  BtkBin *bin = BTK_BIN (widget);
  BtkAllocation new_allocation;

  widget->allocation = *allocation;

  btk_frame_compute_child_allocation (frame, &new_allocation);
  
  /* If the child allocation changed, that means that the frame is drawn
   * in a new place, so we must redraw the entire widget.
   */
  if (btk_widget_get_mapped (widget) &&
      (new_allocation.x != frame->child_allocation.x ||
       new_allocation.y != frame->child_allocation.y ||
       new_allocation.width != frame->child_allocation.width ||
       new_allocation.height != frame->child_allocation.height))
    bdk_window_invalidate_rect (widget->window, &widget->allocation, FALSE);
  
  if (bin->child && btk_widget_get_visible (bin->child))
    btk_widget_size_allocate (bin->child, &new_allocation);
  
  frame->child_allocation = new_allocation;
  
  if (frame->label_widget && btk_widget_get_visible (frame->label_widget))
    {
      BtkRequisition child_requisition;
      BtkAllocation child_allocation;
      gfloat xalign;

      btk_widget_get_child_requisition (frame->label_widget, &child_requisition);

      if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_LTR)
	xalign = frame->label_xalign;
      else
	xalign = 1 - frame->label_xalign;
      
      child_allocation.x = frame->child_allocation.x + LABEL_SIDE_PAD +
	(frame->child_allocation.width - child_requisition.width - 2 * LABEL_PAD - 2 * LABEL_SIDE_PAD) * xalign + LABEL_PAD;
      child_allocation.width = MIN (child_requisition.width, new_allocation.width - 2 * LABEL_PAD - 2 * LABEL_SIDE_PAD);

      child_allocation.y = frame->child_allocation.y - MAX (child_requisition.height, widget->style->ythickness);
      child_allocation.height = child_requisition.height;

      btk_widget_size_allocate (frame->label_widget, &child_allocation);
    }
}

static void
btk_frame_compute_child_allocation (BtkFrame      *frame,
				    BtkAllocation *child_allocation)
{
  g_return_if_fail (BTK_IS_FRAME (frame));
  g_return_if_fail (child_allocation != NULL);

  BTK_FRAME_GET_CLASS (frame)->compute_child_allocation (frame, child_allocation);
}

static void
btk_frame_real_compute_child_allocation (BtkFrame      *frame,
					 BtkAllocation *child_allocation)
{
  BtkWidget *widget = BTK_WIDGET (frame);
  BtkAllocation *allocation = &widget->allocation;
  BtkRequisition child_requisition;
  gint top_margin;

  if (frame->label_widget)
    {
      btk_widget_get_child_requisition (frame->label_widget, &child_requisition);
      top_margin = MAX (child_requisition.height, widget->style->ythickness);
    }
  else
    top_margin = widget->style->ythickness;
  
  child_allocation->x = (BTK_CONTAINER (frame)->border_width +
			 widget->style->xthickness);
  child_allocation->width = MAX(1, (gint)allocation->width - child_allocation->x * 2);
  
  child_allocation->y = (BTK_CONTAINER (frame)->border_width + top_margin);
  child_allocation->height = MAX (1, ((gint)allocation->height - child_allocation->y -
				      (gint)BTK_CONTAINER (frame)->border_width -
				      (gint)widget->style->ythickness));
  
  child_allocation->x += allocation->x;
  child_allocation->y += allocation->y;
}

#define __BTK_FRAME_C__
#include "btkaliasdef.c"
