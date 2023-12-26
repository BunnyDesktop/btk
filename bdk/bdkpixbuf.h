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

#ifndef __BDK_PIXBUF_H__
#define __BDK_PIXBUF_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bairo.h>
#include <bdk/bdktypes.h>
#include <bdk/bdkrgb.h>
#include <bdk-pixbuf/bdk-pixbuf.h>

G_BEGIN_DECLS

/* Rendering to a drawable */

void bdk_pixbuf_render_threshold_alpha   (BdkPixbuf           *pixbuf,
					  BdkBitmap           *bitmap,
					  int                  src_x,
					  int                  src_y,
					  int                  dest_x,
					  int                  dest_y,
					  int                  width,
					  int                  height,
					  int                  alpha_threshold);
#ifndef BDK_DISABLE_DEPRECATED
void bdk_pixbuf_render_to_drawable       (BdkPixbuf           *pixbuf,
					  BdkDrawable         *drawable,
					  BdkGC               *gc,
					  int                  src_x,
					  int                  src_y,
					  int                  dest_x,
					  int                  dest_y,
					  int                  width,
					  int                  height,
					  BdkRgbDither         dither,
					  int                  x_dither,
					  int                  y_dither);
void bdk_pixbuf_render_to_drawable_alpha (BdkPixbuf           *pixbuf,
					  BdkDrawable         *drawable,
					  int                  src_x,
					  int                  src_y,
					  int                  dest_x,
					  int                  dest_y,
					  int                  width,
					  int                  height,
					  BdkPixbufAlphaMode   alpha_mode,
					  int                  alpha_threshold,
					  BdkRgbDither         dither,
					  int                  x_dither,
					  int                  y_dither);
#endif /* BDK_DISABLE_DEPRECATED */
void bdk_pixbuf_render_pixmap_and_mask_for_colormap (BdkPixbuf    *pixbuf,
						     BdkColormap  *colormap,
						     BdkPixmap   **pixmap_return,
						     BdkBitmap   **mask_return,
						     int           alpha_threshold);
#ifndef BDK_MULTIHEAD_SAFE
void bdk_pixbuf_render_pixmap_and_mask              (BdkPixbuf    *pixbuf,
						     BdkPixmap   **pixmap_return,
						     BdkBitmap   **mask_return,
						     int           alpha_threshold);
#endif


/* Fetching a rebunnyion from a drawable */
BdkPixbuf *bdk_pixbuf_get_from_drawable (BdkPixbuf   *dest,
					 BdkDrawable *src,
					 BdkColormap *cmap,
					 int          src_x,
					 int          src_y,
					 int          dest_x,
					 int          dest_y,
					 int          width,
					 int          height);

BdkPixbuf *bdk_pixbuf_get_from_image    (BdkPixbuf   *dest,
                                         BdkImage    *src,
                                         BdkColormap *cmap,
                                         int          src_x,
                                         int          src_y,
                                         int          dest_x,
                                         int          dest_y,
                                         int          width,
                                         int          height);

G_END_DECLS

#endif /* __BDK_PIXBUF_H__ */
