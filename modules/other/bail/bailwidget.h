/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2001 Sun Microsystems Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __BAIL_WIDGET_H__
#define __BAIL_WIDGET_H__

#include <btk/btk.h>

G_BEGIN_DECLS

#define BAIL_TYPE_WIDGET                     (bail_widget_get_type ())
#define BAIL_WIDGET(obj)                     (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_WIDGET, BailWidget))
#define BAIL_WIDGET_CLASS(klass)             (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_WIDGET, BailWidgetClass))
#define BAIL_IS_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_WIDGET))
#define BAIL_IS_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_WIDGET))
#define BAIL_WIDGET_GET_CLASS(obj)           (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_WIDGET, BailWidgetClass))

typedef struct _BailWidget                   BailWidget;
typedef struct _BailWidgetClass              BailWidgetClass;

struct _BailWidget
{
  BtkAccessible parent;
};

GType bail_widget_get_type (void);

struct _BailWidgetClass
{
  BtkAccessibleClass parent_class;

  /*
   * Signal handler for notify signal on BTK widget
   */
  void (*notify_btk)                   (GObject             *object,
                                        GParamSpec          *pspec);
  /*
   * Signal handler for focus_in_event and focus_out_event signal on BTK widget
   */
  gboolean (*focus_btk)                (BtkWidget           *widget,
                                        BdkEventFocus       *event);

};

BatkObject*     bail_widget_new         (BtkWidget       *widget);

G_END_DECLS

#endif /* __BAIL_WIDGET_H__ */
