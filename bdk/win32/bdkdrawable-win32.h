/* BDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#ifndef __BDK_DRAWABLE_WIN32_H__
#define __BDK_DRAWABLE_WIN32_H__

#include <bdk/bdkdrawable.h>
#include <bdk/win32/bdkwin32.h>

B_BEGIN_DECLS

/* Drawable implementation for Win32
 */

typedef struct _BdkDrawableImplWin32 BdkDrawableImplWin32;
typedef struct _BdkDrawableImplWin32Class BdkDrawableImplWin32Class;

#define BDK_TYPE_DRAWABLE_IMPL_WIN32              (_bdk_drawable_impl_win32_get_type ())
#define BDK_DRAWABLE_IMPL_WIN32(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_DRAWABLE_IMPL_WIN32, BdkDrawableImplWin32))
#define BDK_DRAWABLE_IMPL_WIN32_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_DRAWABLE_IMPL_WIN32, BdkDrawableImplWin32Class))
#define BDK_IS_DRAWABLE_IMPL_WIN32(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_DRAWABLE_IMPL_WIN32))
#define BDK_IS_DRAWABLE_IMPL_WIN32_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_DRAWABLE_IMPL_WIN32))
#define BDK_DRAWABLE_IMPL_WIN32_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_DRAWABLE_IMPL_WIN32, BdkDrawableImplWin32Class))

struct _BdkDrawableImplWin32
{
  BdkDrawable parent_instance;
  BdkDrawable *wrapper;
  BdkColormap *colormap;
  HANDLE handle;

  guint hdc_count;
  HDC hdc;
  HBITMAP saved_dc_bitmap;	/* Original bitmap for dc */
  bairo_surface_t *bairo_surface;
};
 
struct _BdkDrawableImplWin32Class 
{
  BdkDrawableClass parent_class;
};

GType _bdk_drawable_impl_win32_get_type (void);

HDC  _bdk_win32_drawable_acquire_dc (BdkDrawable *drawable);
void _bdk_win32_drawable_release_dc (BdkDrawable *drawable);
void _bdk_win32_drawable_finish     (BdkDrawable *drawable);

B_END_DECLS

#endif /* __BDK_DRAWABLE_WIN32_H__ */
