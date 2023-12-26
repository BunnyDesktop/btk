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

#ifndef __BDK_IMAGE_H__
#define __BDK_IMAGE_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bdk/bdktypes.h>

B_BEGIN_DECLS

/* Types of images.
 *   Normal: Normal X image type. These are slow as they involve passing
 *	     the entire image through the X connection each time a draw
 *	     request is required. On Win32, a bitmap.
 *   Shared: Shared memory X image type. These are fast as the X server
 *	     and the program actually use the same piece of memory. They
 *	     should be used with care though as there is the possibility
 *	     for both the X server and the program to be reading/writing
 *	     the image simultaneously and producing undesired results.
 *	     On Win32, also a bitmap.
 */
typedef enum
{
  BDK_IMAGE_NORMAL,
  BDK_IMAGE_SHARED,
  BDK_IMAGE_FASTEST
} BdkImageType;

typedef struct _BdkImageClass BdkImageClass;

#define BDK_TYPE_IMAGE              (bdk_image_get_type ())
#define BDK_IMAGE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_IMAGE, BdkImage))
#define BDK_IMAGE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_IMAGE, BdkImageClass))
#define BDK_IS_IMAGE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_IMAGE))
#define BDK_IS_IMAGE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_IMAGE))
#define BDK_IMAGE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_IMAGE, BdkImageClass))

struct _BdkImage
{
  GObject parent_instance;

  /*< public >*/
  
  BdkImageType	GSEAL (type); /* read only. */
  BdkVisual    *GSEAL (visual);	    /* read only. visual used to create the image */
  BdkByteOrder	GSEAL (byte_order); /* read only. */
  gint		GSEAL (width);  /* read only. */
  gint		GSEAL (height); /* read only. */
  guint16	GSEAL (depth);  /* read only. */
  guint16	GSEAL (bpp);    /* read only. bytes per pixel */
  guint16	GSEAL (bpl);    /* read only. bytes per line */
  guint16       GSEAL (bits_per_pixel); /* read only. bits per pixel */
  gpointer	GSEAL (mem);

  BdkColormap  *GSEAL (colormap); /* read only. */

  /*< private >*/
  gpointer GSEAL (windowing_data); /* read only. */
};

struct _BdkImageClass
{
  GObjectClass parent_class;
};

GType     bdk_image_get_type   (void) B_GNUC_CONST;

#ifndef BDK_DISABLE_DEPRECATED
BdkImage*  bdk_image_new       (BdkImageType  type,
				BdkVisual    *visual,
				gint	      width,
				gint	      height);

BdkImage*  bdk_image_get       (BdkDrawable  *drawable,
				gint	      x,
				gint	      y,
				gint	      width,
				gint	      height);

BdkImage * bdk_image_ref       (BdkImage     *image);
void       bdk_image_unref     (BdkImage     *image);

void	   bdk_image_put_pixel (BdkImage     *image,
				gint	      x,
				gint	      y,
				guint32	      pixel);
guint32	   bdk_image_get_pixel (BdkImage     *image,
				gint	      x,
				gint	      y);

void       bdk_image_set_colormap (BdkImage    *image,
                                   BdkColormap *colormap);
BdkColormap* bdk_image_get_colormap (BdkImage    *image);

BdkImageType  bdk_image_get_image_type     (BdkImage *image);
BdkVisual    *bdk_image_get_visual         (BdkImage *image);
BdkByteOrder  bdk_image_get_byte_order     (BdkImage *image);
gint          bdk_image_get_width          (BdkImage *image);
gint          bdk_image_get_height         (BdkImage *image);
guint16       bdk_image_get_depth          (BdkImage *image);
guint16       bdk_image_get_bytes_per_pixel(BdkImage *image);
guint16       bdk_image_get_bytes_per_line (BdkImage *image);
guint16       bdk_image_get_bits_per_pixel (BdkImage *image);
gpointer      bdk_image_get_pixels         (BdkImage *image);

#ifdef BDK_ENABLE_BROKEN
BdkImage* bdk_image_new_bitmap (BdkVisual     *visual,
				gpointer      data,
				gint          width,
				gint          height);
#endif /* BDK_ENABLE_BROKEN */

#define bdk_image_destroy              g_object_unref
#endif /* BDK_DISABLE_DEPRECATED */

B_END_DECLS

#endif /* __BDK_IMAGE_H__ */
