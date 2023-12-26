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

#ifndef __BAIL_PIXMAP_H__
#define __BAIL_PIXMAP_H__

#include <bail/bailwidget.h>

G_BEGIN_DECLS

#define BAIL_TYPE_PIXMAP                      (bail_pixmap_get_type ())
#define BAIL_PIXMAP(obj)                      (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_PIXMAP, BailPixmap))
#define BAIL_PIXMAP_CLASS(klass)              (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_PIXMAP, BailPixmapClass))
#define BAIL_IS_PIXMAP(obj)                   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_PIXMAP))
#define BAIL_IS_PIXMAP_CLASS(klass)           (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_PIXMAP))
#define BAIL_PIXMAP_GET_CLASS(obj)            (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_PIXMAP, BailPixmapClass))

typedef struct _BailPixmap              BailPixmap;
typedef struct _BailPixmapClass         BailPixmapClass;

struct _BailPixmap
{
  BailWidget parent;

  gchar*     image_description;

};

GType bail_pixmap_get_type (void);

struct _BailPixmapClass
{
  BailWidgetClass parent_class;
};

G_END_DECLS

#endif /* __BAIL_PIXMAP_H__ */
