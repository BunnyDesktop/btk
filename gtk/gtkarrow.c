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
 * Modified by the BTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

/**
 * SECTION:btkarrow
 * @Short_description: Displays an arrow
 * @Title: BtkArrow
 * @See_also: btk_paint_arrow()
 *
 * BtkArrow should be used to draw simple arrows that need to point in
 * one of the four cardinal directions (up, down, left, or right).  The
 * style of the arrow can be one of shadow in, shadow out, etched in, or
 * etched out.  Note that these directions and style types may be
 * ammended in versions of BTK+ to come.
 *
 * BtkArrow will fill any space alloted to it, but since it is inherited
 * from #BtkMisc, it can be padded and/or aligned, to fill exactly the
 * space the programmer desires.
 *
 * Arrows are created with a call to btk_arrow_new().  The direction or
 * style of an arrow can be changed after creation by using btk_arrow_set().
 */

#include "config.h"
#include <math.h>
#include "btkarrow.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

#define MIN_ARROW_SIZE  15

enum {
  PROP_0,
  PROP_ARROW_TYPE,
  PROP_SHADOW_TYPE
};


static void     btk_arrow_set_property (GObject        *object,
                                        guint           prop_id,
                                        const GValue   *value,
                                        GParamSpec     *pspec);
static void     btk_arrow_get_property (GObject        *object,
                                        guint           prop_id,
                                        GValue         *value,
                                        GParamSpec     *pspec);
static gboolean btk_arrow_expose       (BtkWidget      *widget,
                                        BdkEventExpose *event);


G_DEFINE_TYPE (BtkArrow, btk_arrow, BTK_TYPE_MISC)


static void
btk_arrow_class_init (BtkArrowClass *class)
{
  GObjectClass *bobject_class;
  BtkWidgetClass *widget_class;

  bobject_class = (GObjectClass*) class;
  widget_class = (BtkWidgetClass*) class;

  bobject_class->set_property = btk_arrow_set_property;
  bobject_class->get_property = btk_arrow_get_property;

  widget_class->expose_event = btk_arrow_expose;

  g_object_class_install_property (bobject_class,
                                   PROP_ARROW_TYPE,
                                   g_param_spec_enum ("arrow-type",
                                                      P_("Arrow direction"),
                                                      P_("The direction the arrow should point"),
						      BTK_TYPE_ARROW_TYPE,
						      BTK_ARROW_RIGHT,
                                                      BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_SHADOW_TYPE,
                                   g_param_spec_enum ("shadow-type",
                                                      P_("Arrow shadow"),
                                                      P_("Appearance of the shadow surrounding the arrow"),
						      BTK_TYPE_SHADOW_TYPE,
						      BTK_SHADOW_OUT,
                                                      BTK_PARAM_READWRITE));

  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_float ("arrow-scaling",
                                                               P_("Arrow Scaling"),
                                                               P_("Amount of space used up by arrow"),
                                                               0.0, 1.0, 0.7,
                                                               BTK_PARAM_READABLE));
}

static void
btk_arrow_set_property (GObject         *object,
			guint            prop_id,
			const GValue    *value,
			GParamSpec      *pspec)
{
  BtkArrow *arrow = BTK_ARROW (object);

  switch (prop_id)
    {
    case PROP_ARROW_TYPE:
      btk_arrow_set (arrow,
		     g_value_get_enum (value),
		     arrow->shadow_type);
      break;
    case PROP_SHADOW_TYPE:
      btk_arrow_set (arrow,
		     arrow->arrow_type,
		     g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_arrow_get_property (GObject         *object,
			guint            prop_id,
			GValue          *value,
			GParamSpec      *pspec)
{
  BtkArrow *arrow = BTK_ARROW (object);

  switch (prop_id)
    {
    case PROP_ARROW_TYPE:
      g_value_set_enum (value, arrow->arrow_type);
      break;
    case PROP_SHADOW_TYPE:
      g_value_set_enum (value, arrow->shadow_type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_arrow_init (BtkArrow *arrow)
{
  btk_widget_set_has_window (BTK_WIDGET (arrow), FALSE);

  BTK_WIDGET (arrow)->requisition.width = MIN_ARROW_SIZE + BTK_MISC (arrow)->xpad * 2;
  BTK_WIDGET (arrow)->requisition.height = MIN_ARROW_SIZE + BTK_MISC (arrow)->ypad * 2;

  arrow->arrow_type = BTK_ARROW_RIGHT;
  arrow->shadow_type = BTK_SHADOW_OUT;
}

/**
 * btk_arrow_new:
 * @arrow_type: a valid #BtkArrowType.
 * @shadow_type: a valid #BtkShadowType.
 *
 * Creates a new #BtkArrow widget.
 *
 * Returns: the new #BtkArrow widget.
 */
BtkWidget*
btk_arrow_new (BtkArrowType  arrow_type,
	       BtkShadowType shadow_type)
{
  BtkArrow *arrow;

  arrow = g_object_new (BTK_TYPE_ARROW, NULL);

  arrow->arrow_type = arrow_type;
  arrow->shadow_type = shadow_type;

  return BTK_WIDGET (arrow);
}

/**
 * btk_arrow_set:
 * @arrow: a widget of type #BtkArrow.
 * @arrow_type: a valid #BtkArrowType.
 * @shadow_type: a valid #BtkShadowType.
 *
 * Sets the direction and style of the #BtkArrow, @arrow.
 */
void
btk_arrow_set (BtkArrow      *arrow,
	       BtkArrowType   arrow_type,
	       BtkShadowType  shadow_type)
{
  BtkWidget *widget;

  g_return_if_fail (BTK_IS_ARROW (arrow));

  if (   ((BtkArrowType) arrow->arrow_type != arrow_type)
      || ((BtkShadowType) arrow->shadow_type != shadow_type))
    {
      g_object_freeze_notify (G_OBJECT (arrow));

      if ((BtkArrowType) arrow->arrow_type != arrow_type)
        {
          arrow->arrow_type = arrow_type;
          g_object_notify (G_OBJECT (arrow), "arrow-type");
        }

      if ((BtkShadowType) arrow->shadow_type != shadow_type)
        {
          arrow->shadow_type = shadow_type;
          g_object_notify (G_OBJECT (arrow), "shadow-type");
        }

      g_object_thaw_notify (G_OBJECT (arrow));

      widget = BTK_WIDGET (arrow);
      if (btk_widget_is_drawable (widget))
	btk_widget_queue_draw (widget);
    }
}


static gboolean
btk_arrow_expose (BtkWidget      *widget,
		  BdkEventExpose *event)
{
  if (btk_widget_is_drawable (widget))
    {
      BtkArrow *arrow = BTK_ARROW (widget);
      BtkMisc *misc = BTK_MISC (widget);
      BtkShadowType shadow_type;
      gint width, height;
      gint x, y;
      gint extent;
      gfloat xalign;
      BtkArrowType effective_arrow_type;
      gfloat arrow_scaling;

      btk_widget_style_get (widget, "arrow-scaling", &arrow_scaling, NULL);

      width = widget->allocation.width - misc->xpad * 2;
      height = widget->allocation.height - misc->ypad * 2;
      extent = MIN (width, height) * arrow_scaling;
      effective_arrow_type = arrow->arrow_type;

      if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_LTR)
	xalign = misc->xalign;
      else
	{
	  xalign = 1.0 - misc->xalign;
	  if (arrow->arrow_type == BTK_ARROW_LEFT)
	    effective_arrow_type = BTK_ARROW_RIGHT;
	  else if (arrow->arrow_type == BTK_ARROW_RIGHT)
	    effective_arrow_type = BTK_ARROW_LEFT;
	}

      x = floor (widget->allocation.x + misc->xpad
		 + ((widget->allocation.width - extent) * xalign));
      y = floor (widget->allocation.y + misc->ypad
		 + ((widget->allocation.height - extent) * misc->yalign));

      shadow_type = arrow->shadow_type;

      if (widget->state == BTK_STATE_ACTIVE)
	{
          if (shadow_type == BTK_SHADOW_IN)
            shadow_type = BTK_SHADOW_OUT;
          else if (shadow_type == BTK_SHADOW_OUT)
            shadow_type = BTK_SHADOW_IN;
          else if (shadow_type == BTK_SHADOW_ETCHED_IN)
            shadow_type = BTK_SHADOW_ETCHED_OUT;
          else if (shadow_type == BTK_SHADOW_ETCHED_OUT)
            shadow_type = BTK_SHADOW_ETCHED_IN;
	}

      btk_paint_arrow (widget->style, widget->window,
		       widget->state, shadow_type,
		       &event->area, widget, "arrow",
		       effective_arrow_type, TRUE,
		       x, y, extent, extent);
    }

  return FALSE;
}

#define __BTK_ARROW_C__
#include "btkaliasdef.c"
