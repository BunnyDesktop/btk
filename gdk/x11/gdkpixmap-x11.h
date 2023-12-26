/* BDK - The GIMP Drawing Kit
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

#ifndef __BDK_PIXMAP_X11_H__
#define __BDK_PIXMAP_X11_H__

#include <bdk/x11/bdkdrawable-x11.h>
#include <bdk/bdkpixmap.h>

G_BEGIN_DECLS

/* Pixmap implementation for X11
 */

typedef struct _BdkPixmapImplX11 BdkPixmapImplX11;
typedef struct _BdkPixmapImplX11Class BdkPixmapImplX11Class;

#define BDK_TYPE_PIXMAP_IMPL_X11              (bdk_pixmap_impl_x11_get_type ())
#define BDK_PIXMAP_IMPL_X11(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_PIXMAP_IMPL_X11, BdkPixmapImplX11))
#define BDK_PIXMAP_IMPL_X11_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_PIXMAP_IMPL_X11, BdkPixmapImplX11Class))
#define BDK_IS_PIXMAP_IMPL_X11(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_PIXMAP_IMPL_X11))
#define BDK_IS_PIXMAP_IMPL_X11_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_PIXMAP_IMPL_X11))
#define BDK_PIXMAP_IMPL_X11_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_PIXMAP_IMPL_X11, BdkPixmapImplX11Class))

struct _BdkPixmapImplX11
{
  BdkDrawableImplX11 parent_instance;

  gint width;
  gint height;

  guint is_foreign : 1;
};
 
struct _BdkPixmapImplX11Class 
{
  BdkDrawableImplX11Class parent_class;

};

GType bdk_pixmap_impl_x11_get_type (void);

G_END_DECLS

#endif /* __BDK_PIXMAP_X11_H__ */
