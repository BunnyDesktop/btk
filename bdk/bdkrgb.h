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

#ifndef __BDK_RGB_H__
#define __BDK_RGB_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bdk/bdktypes.h>

B_BEGIN_DECLS

typedef struct _BdkRgbCmap BdkRgbCmap;

typedef enum
{
  BDK_RGB_DITHER_NONE,
  BDK_RGB_DITHER_NORMAL,
  BDK_RGB_DITHER_MAX
} BdkRgbDither;

#ifndef BDK_DISABLE_DEPRECATED

struct _BdkRgbCmap {
  buint32 colors[256];
  bint n_colors;

  /*< private >*/
  GSList *info_list;
};

void bdk_rgb_init (void);

bulong bdk_rgb_xpixel_from_rgb   (buint32      rgb) B_GNUC_CONST;
void   bdk_rgb_gc_set_foreground (BdkGC       *gc,
				  buint32      rgb);
void   bdk_rgb_gc_set_background (BdkGC       *gc,
				  buint32      rgb);
#define bdk_rgb_get_cmap               bdk_rgb_get_colormap

void   bdk_rgb_find_color        (BdkColormap *colormap,
				  BdkColor    *color);

void        bdk_draw_rgb_image              (BdkDrawable  *drawable,
					     BdkGC        *gc,
					     bint          x,
					     bint          y,
					     bint          width,
					     bint          height,
					     BdkRgbDither  dith,
					     const buchar *rgb_buf,
					     bint          rowstride);
void        bdk_draw_rgb_image_dithalign    (BdkDrawable  *drawable,
					     BdkGC        *gc,
					     bint          x,
					     bint          y,
					     bint          width,
					     bint          height,
					     BdkRgbDither  dith,
					     const buchar *rgb_buf,
					     bint          rowstride,
					     bint          xdith,
					     bint          ydith);
void        bdk_draw_rgb_32_image           (BdkDrawable  *drawable,
					     BdkGC        *gc,
					     bint          x,
					     bint          y,
					     bint          width,
					     bint          height,
					     BdkRgbDither  dith,
					     const buchar *buf,
					     bint          rowstride);
void        bdk_draw_rgb_32_image_dithalign (BdkDrawable  *drawable,
					     BdkGC        *gc,
					     bint          x,
					     bint          y,
					     bint          width,
					     bint          height,
					     BdkRgbDither  dith,
					     const buchar *buf,
					     bint          rowstride,
					     bint          xdith,
					     bint          ydith);
void        bdk_draw_gray_image             (BdkDrawable  *drawable,
					     BdkGC        *gc,
					     bint          x,
					     bint          y,
					     bint          width,
					     bint          height,
					     BdkRgbDither  dith,
					     const buchar *buf,
					     bint          rowstride);
void        bdk_draw_indexed_image          (BdkDrawable  *drawable,
					     BdkGC        *gc,
					     bint          x,
					     bint          y,
					     bint          width,
					     bint          height,
					     BdkRgbDither  dith,
					     const buchar *buf,
					     bint          rowstride,
					     BdkRgbCmap   *cmap);
BdkRgbCmap *bdk_rgb_cmap_new                (buint32      *colors,
					     bint          n_colors);
void        bdk_rgb_cmap_free               (BdkRgbCmap   *cmap);

void     bdk_rgb_set_verbose (bboolean verbose);

/* experimental colormap stuff */
void bdk_rgb_set_install    (bboolean install);
void bdk_rgb_set_min_colors (bint     min_colors);

#ifndef BDK_MULTIHEAD_SAFE
BdkColormap *bdk_rgb_get_colormap (void);
BdkVisual *  bdk_rgb_get_visual   (void);
bboolean     bdk_rgb_ditherable   (void);
bboolean     bdk_rgb_colormap_ditherable (BdkColormap *cmap);
#endif
#endif /* BDK_DISABLE_DEPRECATED */

B_END_DECLS


#endif /* __BDK_RGB_H__ */
