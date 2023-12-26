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

#include "btkorientable.h"
#include "btkseparator.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"


enum {
  PROP_0,
  PROP_ORIENTATION
};


typedef struct _BtkSeparatorPrivate BtkSeparatorPrivate;

struct _BtkSeparatorPrivate
{
  BtkOrientation orientation;
};

#define BTK_SEPARATOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_SEPARATOR, BtkSeparatorPrivate))


static void       btk_separator_set_property (GObject        *object,
                                              guint           prop_id,
                                              const GValue   *value,
                                              GParamSpec     *pspec);
static void       btk_separator_get_property (GObject        *object,
                                              guint           prop_id,
                                              GValue         *value,
                                              GParamSpec     *pspec);

static void       btk_separator_size_request (BtkWidget      *widget,
                                              BtkRequisition *requisition);
static gboolean   btk_separator_expose       (BtkWidget      *widget,
                                              BdkEventExpose *event);


G_DEFINE_ABSTRACT_TYPE_WITH_CODE (BtkSeparator, btk_separator, BTK_TYPE_WIDGET,
                                  G_IMPLEMENT_INTERFACE (BTK_TYPE_ORIENTABLE,
                                                         NULL))


static void
btk_separator_class_init (BtkSeparatorClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (class);

  object_class->set_property = btk_separator_set_property;
  object_class->get_property = btk_separator_get_property;

  widget_class->size_request = btk_separator_size_request;
  widget_class->expose_event = btk_separator_expose;

  g_object_class_override_property (object_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  g_type_class_add_private (object_class, sizeof (BtkSeparatorPrivate));
}

static void
btk_separator_init (BtkSeparator *separator)
{
  BtkWidget *widget = BTK_WIDGET (separator);
  BtkSeparatorPrivate *private = BTK_SEPARATOR_GET_PRIVATE (separator);

  btk_widget_set_has_window (BTK_WIDGET (separator), FALSE);

  private->orientation = BTK_ORIENTATION_HORIZONTAL;

  widget->requisition.width  = 1;
  widget->requisition.height = widget->style->ythickness;
}

static void
btk_separator_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  BtkSeparatorPrivate *private = BTK_SEPARATOR_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      private->orientation = g_value_get_enum (value);
      btk_widget_queue_resize (BTK_WIDGET (object));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_separator_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  BtkSeparatorPrivate *private = BTK_SEPARATOR_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, private->orientation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_separator_size_request (BtkWidget      *widget,
                            BtkRequisition *requisition)
{
  BtkSeparatorPrivate *private = BTK_SEPARATOR_GET_PRIVATE (widget);
  gboolean wide_separators;
  gint     separator_width;
  gint     separator_height;

  btk_widget_style_get (widget,
                        "wide-separators",  &wide_separators,
                        "separator-width",  &separator_width,
                        "separator-height", &separator_height,
                        NULL);

  requisition->width  = 1;
  requisition->height = 1;

  if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
    {
      if (wide_separators)
        requisition->height = separator_height;
      else
        requisition->height = widget->style->ythickness;
    }
  else
    {
      if (wide_separators)
        requisition->width = separator_width;
      else
        requisition->width = widget->style->xthickness;
    }
}

static gboolean
btk_separator_expose (BtkWidget      *widget,
                      BdkEventExpose *event)
{
  BtkSeparatorPrivate *private = BTK_SEPARATOR_GET_PRIVATE (widget);
  gboolean wide_separators;
  gint     separator_width;
  gint     separator_height;

  if (!btk_widget_is_drawable (widget))
    return FALSE;

  btk_widget_style_get (widget,
                        "wide-separators",  &wide_separators,
                        "separator-width",  &separator_width,
                        "separator-height", &separator_height,
                        NULL);

  if (private->orientation == BTK_ORIENTATION_HORIZONTAL)
    {
      if (wide_separators)
        btk_paint_box (widget->style, widget->window,
                       btk_widget_get_state (widget), BTK_SHADOW_ETCHED_OUT,
                       &event->area, widget, "hseparator",
                       widget->allocation.x,
                       widget->allocation.y + (widget->allocation.height -
                                               separator_height) / 2,
                       widget->allocation.width,
                       separator_height);
      else
        btk_paint_hline (widget->style, widget->window,
                         btk_widget_get_state (widget),
                         &event->area, widget, "hseparator",
                         widget->allocation.x,
                         widget->allocation.x + widget->allocation.width - 1,
                         widget->allocation.y + (widget->allocation.height -
                                                 widget->style->ythickness) / 2);
    }
  else
    {
      if (wide_separators)
        btk_paint_box (widget->style, widget->window,
                       btk_widget_get_state (widget), BTK_SHADOW_ETCHED_OUT,
                       &event->area, widget, "vseparator",
                       widget->allocation.x + (widget->allocation.width -
                                               separator_width) / 2,
                       widget->allocation.y,
                       separator_width,
                       widget->allocation.height);
      else
        btk_paint_vline (widget->style, widget->window,
                         btk_widget_get_state (widget),
                         &event->area, widget, "vseparator",
                         widget->allocation.y,
                         widget->allocation.y + widget->allocation.height - 1,
                         widget->allocation.x + (widget->allocation.width -
                                                 widget->style->xthickness) / 2);
    }

  return FALSE;
}

#if 0
/**
 * btk_separator_new:
 * @orientation: the separator's orientation.
 *
 * Creates a new #BtkSeparator with the given orientation.
 *
 * Return value: a new #BtkSeparator.
 *
 * Since: 2.16
 **/
BtkWidget *
btk_separator_new (BtkOrientation orientation)
{
  return g_object_new (BTK_TYPE_SEPARATOR,
                       "orientation", orientation,
                       NULL);
}
#endif


#define __BTK_SEPARATOR_C__
#include "btkaliasdef.c"
