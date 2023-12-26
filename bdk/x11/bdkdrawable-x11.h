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

#ifndef __BDK_DRAWABLE_X11_H__
#define __BDK_DRAWABLE_X11_H__

#include <bdk/bdkdrawable.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

B_BEGIN_DECLS

/* Drawable implementation for X11
 */

typedef enum
{
  BDK_X11_FORMAT_NONE,
  BDK_X11_FORMAT_EXACT_MASK,
  BDK_X11_FORMAT_ARGB_MASK,
  BDK_X11_FORMAT_ARGB
} BdkX11FormatType;

typedef struct _BdkDrawableImplX11 BdkDrawableImplX11;
typedef struct _BdkDrawableImplX11Class BdkDrawableImplX11Class;

#define BDK_TYPE_DRAWABLE_IMPL_X11              (_bdk_drawable_impl_x11_get_type ())
#define BDK_DRAWABLE_IMPL_X11(object)           (B_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_DRAWABLE_IMPL_X11, BdkDrawableImplX11))
#define BDK_DRAWABLE_IMPL_X11_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_DRAWABLE_IMPL_X11, BdkDrawableImplX11Class))
#define BDK_IS_DRAWABLE_IMPL_X11(object)        (B_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_DRAWABLE_IMPL_X11))
#define BDK_IS_DRAWABLE_IMPL_X11_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_DRAWABLE_IMPL_X11))
#define BDK_DRAWABLE_IMPL_X11_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_DRAWABLE_IMPL_X11, BdkDrawableImplX11Class))

struct _BdkDrawableImplX11
{
  BdkDrawable parent_instance;

  BdkDrawable *wrapper;
  
  BdkColormap *colormap;
  
  Window xid;
  BdkScreen *screen;

  Picture picture;
  bairo_surface_t *bairo_surface;
};
 
struct _BdkDrawableImplX11Class 
{
  BdkDrawableClass parent_class;

};

GType _bdk_drawable_impl_x11_get_type (void);

void  _bdk_x11_convert_to_format      (guchar           *src_buf,
                                       gint              src_rowstride,
                                       guchar           *dest_buf,
                                       gint              dest_rowstride,
                                       BdkX11FormatType  dest_format,
                                       BdkByteOrder      dest_byteorder,
                                       gint              width,
                                       gint              height);

/* Note that the following take BdkDrawableImplX11, not the wrapper drawable */
void _bdk_x11_drawable_finish           (BdkDrawable  *drawable);
void _bdk_x11_drawable_update_size      (BdkDrawable  *drawable);
BdkDrawable *bdk_x11_window_get_drawable_impl (BdkWindow *window);
BdkDrawable *bdk_x11_pixmap_get_drawable_impl (BdkPixmap *pixmap);

B_END_DECLS

#endif /* __BDK_DRAWABLE_X11_H__ */
