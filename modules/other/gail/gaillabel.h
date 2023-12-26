/* BAIL - The GNOME Accessibility Implementation Library
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

#ifndef __BAIL_LABEL_H__
#define __BAIL_LABEL_H__

#include <bail/bailwidget.h>
#include <libbail-util/bailtextutil.h>

G_BEGIN_DECLS

#define BAIL_TYPE_LABEL                      (bail_label_get_type ())
#define BAIL_LABEL(obj)                      (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_LABEL, BailLabel))
#define BAIL_LABEL_CLASS(klass)              (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_LABEL, BailLabelClass))
#define BAIL_IS_LABEL(obj)                   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_LABEL))
#define BAIL_IS_LABEL_CLASS(klass)           (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_LABEL))
#define BAIL_LABEL_GET_CLASS(obj)            (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_LABEL, BailLabelClass))

typedef struct _BailLabel              BailLabel;
typedef struct _BailLabelClass         BailLabelClass;

struct _BailLabel
{
  BailWidget parent;

  BailTextUtil   *textutil;
  gint           cursor_position;
  gint           selection_bound;
  gint           label_length;
  guint          window_create_handler;
  gboolean       has_top_level;
};

GType bail_label_get_type (void);

struct _BailLabelClass
{
  BailWidgetClass parent_class;
};

G_END_DECLS

#endif /* __BAIL_LABEL_H__ */
