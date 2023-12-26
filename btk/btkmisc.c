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
#include "btkcontainer.h"
#include "btkmisc.h"
#include "btkintl.h"
#include "btkprivate.h"
#include "btkalias.h"


enum {
  PROP_0,
  PROP_XALIGN,
  PROP_YALIGN,
  PROP_XPAD,
  PROP_YPAD
};

static void btk_misc_realize      (BtkWidget    *widget);
static void btk_misc_set_property (BObject         *object,
				   buint            prop_id,
				   const BValue    *value,
				   BParamSpec      *pspec);
static void btk_misc_get_property (BObject         *object,
				   buint            prop_id,
				   BValue          *value,
				   BParamSpec      *pspec);


G_DEFINE_ABSTRACT_TYPE (BtkMisc, btk_misc, BTK_TYPE_WIDGET)

static void
btk_misc_class_init (BtkMiscClass *class)
{
  BObjectClass   *bobject_class;
  BtkWidgetClass *widget_class;

  bobject_class = B_OBJECT_CLASS (class);
  widget_class = (BtkWidgetClass*) class;

  bobject_class->set_property = btk_misc_set_property;
  bobject_class->get_property = btk_misc_get_property;
  
  widget_class->realize = btk_misc_realize;

  g_object_class_install_property (bobject_class,
                                   PROP_XALIGN,
                                   g_param_spec_float ("xalign",
						       P_("X align"),
						       P_("The horizontal alignment, from 0 (left) to 1 (right). Reversed for RTL layouts."),
						       0.0,
						       1.0,
						       0.5,
						       BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_YALIGN,
                                   g_param_spec_float ("yalign",
						       P_("Y align"),
						       P_("The vertical alignment, from 0 (top) to 1 (bottom)"),
						       0.0,
						       1.0,
						       0.5,
						       BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_XPAD,
                                   g_param_spec_int ("xpad",
						     P_("X pad"),
						     P_("The amount of space to add on the left and right of the widget, in pixels"),
						     0,
						     B_MAXINT,
						     0,
						     BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_YPAD,
                                   g_param_spec_int ("ypad",
						     P_("Y pad"),
						     P_("The amount of space to add on the top and bottom of the widget, in pixels"),
						     0,
						     B_MAXINT,
						     0,
						     BTK_PARAM_READWRITE));
}

static void
btk_misc_init (BtkMisc *misc)
{
  misc->xalign = 0.5;
  misc->yalign = 0.5;
  misc->xpad = 0;
  misc->ypad = 0;
}

static void
btk_misc_set_property (BObject      *object,
		       buint         prop_id,
		       const BValue *value,
		       BParamSpec   *pspec)
{
  BtkMisc *misc;

  misc = BTK_MISC (object);

  switch (prop_id)
    {
    case PROP_XALIGN:
      btk_misc_set_alignment (misc, b_value_get_float (value), misc->yalign);
      break;
    case PROP_YALIGN:
      btk_misc_set_alignment (misc, misc->xalign, b_value_get_float (value));
      break;
    case PROP_XPAD:
      btk_misc_set_padding (misc, b_value_get_int (value), misc->ypad);
      break;
    case PROP_YPAD:
      btk_misc_set_padding (misc, misc->xpad, b_value_get_int (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_misc_get_property (BObject      *object,
		       buint         prop_id,
		       BValue       *value,
		       BParamSpec   *pspec)
{
  BtkMisc *misc;

  misc = BTK_MISC (object);

  switch (prop_id)
    {
    case PROP_XALIGN:
      b_value_set_float (value, misc->xalign);
      break;
    case PROP_YALIGN:
      b_value_set_float (value, misc->yalign);
      break;
    case PROP_XPAD:
      b_value_set_int (value, misc->xpad);
      break;
    case PROP_YPAD:
      b_value_set_int (value, misc->ypad);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

void
btk_misc_set_alignment (BtkMisc *misc,
			bfloat   xalign,
			bfloat   yalign)
{
  BtkWidget *widget;

  g_return_if_fail (BTK_IS_MISC (misc));

  if (xalign < 0.0)
    xalign = 0.0;
  else if (xalign > 1.0)
    xalign = 1.0;

  if (yalign < 0.0)
    yalign = 0.0;
  else if (yalign > 1.0)
    yalign = 1.0;

  if ((xalign != misc->xalign) || (yalign != misc->yalign))
    {
      g_object_freeze_notify (B_OBJECT (misc));
      if (xalign != misc->xalign)
	g_object_notify (B_OBJECT (misc), "xalign");

      if (yalign != misc->yalign)
	g_object_notify (B_OBJECT (misc), "yalign");

      misc->xalign = xalign;
      misc->yalign = yalign;
      
      /* clear the area that was allocated before the change
       */
      widget = BTK_WIDGET (misc);
      if (btk_widget_is_drawable (widget))
        btk_widget_queue_draw (widget);

      g_object_thaw_notify (B_OBJECT (misc));
    }
}

/**
 * btk_misc_get_alignment:
 * @misc: a #BtkMisc
 * @xalign: (out) (allow-none): location to store X alignment of @misc, or %NULL
 * @yalign: (out) (allow-none): location to store Y alignment of @misc, or %NULL
 *
 * Gets the X and Y alignment of the widget within its allocation. 
 * See btk_misc_set_alignment().
 **/
void
btk_misc_get_alignment (BtkMisc *misc,
		        bfloat  *xalign,
			bfloat  *yalign)
{
  g_return_if_fail (BTK_IS_MISC (misc));

  if (xalign)
    *xalign = misc->xalign;
  if (yalign)
    *yalign = misc->yalign;
}

void
btk_misc_set_padding (BtkMisc *misc,
		      bint     xpad,
		      bint     ypad)
{
  BtkRequisition *requisition;
  
  g_return_if_fail (BTK_IS_MISC (misc));
  
  if (xpad < 0)
    xpad = 0;
  if (ypad < 0)
    ypad = 0;
  
  if ((xpad != misc->xpad) || (ypad != misc->ypad))
    {
      g_object_freeze_notify (B_OBJECT (misc));
      if (xpad != misc->xpad)
	g_object_notify (B_OBJECT (misc), "xpad");

      if (ypad != misc->ypad)
	g_object_notify (B_OBJECT (misc), "ypad");

      requisition = &(BTK_WIDGET (misc)->requisition);
      requisition->width -= misc->xpad * 2;
      requisition->height -= misc->ypad * 2;
      
      misc->xpad = xpad;
      misc->ypad = ypad;
      
      requisition->width += misc->xpad * 2;
      requisition->height += misc->ypad * 2;
      
      if (btk_widget_is_drawable (BTK_WIDGET (misc)))
	btk_widget_queue_resize (BTK_WIDGET (misc));

      g_object_thaw_notify (B_OBJECT (misc));
    }
}

/**
 * btk_misc_get_padding:
 * @misc: a #BtkMisc
 * @xpad: (out) (allow-none): location to store padding in the X
 *        direction, or %NULL
 * @ypad: (out) (allow-none): location to store padding in the Y
 *        direction, or %NULL
 *
 * Gets the padding in the X and Y directions of the widget. 
 * See btk_misc_set_padding().
 **/
void
btk_misc_get_padding (BtkMisc *misc,
		      bint    *xpad,
		      bint    *ypad)
{
  g_return_if_fail (BTK_IS_MISC (misc));

  if (xpad)
    *xpad = misc->xpad;
  if (ypad)
    *ypad = misc->ypad;
}

static void
btk_misc_realize (BtkWidget *widget)
{
  BdkWindowAttr attributes;
  bint attributes_mask;

  btk_widget_set_realized (widget, TRUE);

  if (!btk_widget_get_has_window (widget))
    {
      widget->window = btk_widget_get_parent_window (widget);
      g_object_ref (widget->window);
      widget->style = btk_style_attach (widget->style, widget->window);
    }
  else
    {
      attributes.window_type = BDK_WINDOW_CHILD;
      attributes.x = widget->allocation.x;
      attributes.y = widget->allocation.y;
      attributes.width = widget->allocation.width;
      attributes.height = widget->allocation.height;
      attributes.wclass = BDK_INPUT_OUTPUT;
      attributes.visual = btk_widget_get_visual (widget);
      attributes.colormap = btk_widget_get_colormap (widget);
      attributes.event_mask = btk_widget_get_events (widget) | BDK_EXPOSURE_MASK;
      attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;

      widget->window = bdk_window_new (btk_widget_get_parent_window (widget), &attributes, attributes_mask);
      bdk_window_set_user_data (widget->window, widget);

      widget->style = btk_style_attach (widget->style, widget->window);
      bdk_window_set_back_pixmap (widget->window, NULL, TRUE);
    }
}

#define __BTK_MISC_C__
#include "btkaliasdef.c"
