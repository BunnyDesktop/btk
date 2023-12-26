/* bdkpixmap-quartz.c
 *
 * Copyright (C) 2005 Imendio AB
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

#include "config.h"

#include "bdkpixmap.h"
#include "bdkprivate-quartz.h"

static gpointer parent_class;

static void
bdk_pixmap_impl_quartz_init (BdkPixmapImplQuartz *impl)
{
}

static void
bdk_pixmap_impl_quartz_get_size (BdkDrawable *drawable,
				gint        *width,
				gint        *height)
{
  if (width)
    *width = BDK_PIXMAP_IMPL_QUARTZ (drawable)->width;
  if (height)
    *height = BDK_PIXMAP_IMPL_QUARTZ (drawable)->height;
}

static void
bdk_pixmap_impl_quartz_get_image_parameters (BdkPixmap           *pixmap,
                                             gint                *bits_per_component,
                                             gint                *bits_per_pixel,
                                             gint                *bytes_per_row,
                                             CGColorSpaceRef     *colorspace,
                                             CGImageAlphaInfo    *alpha_info)
{
  BdkPixmapImplQuartz *impl = BDK_PIXMAP_IMPL_QUARTZ (BDK_PIXMAP_OBJECT (pixmap)->impl);
  gint depth = BDK_PIXMAP_OBJECT (pixmap)->depth;

  switch (depth)
    {
      case 24:
        if (bits_per_component)
          *bits_per_component = 8;

        if (bits_per_pixel)
          *bits_per_pixel = 32;

        if (bytes_per_row)
          *bytes_per_row = impl->width * 4;

        if (colorspace)
          *colorspace = CGColorSpaceCreateDeviceRGB ();

        if (alpha_info)
          *alpha_info = kCGImageAlphaNoneSkipLast;
        break;

      case 32:
        if (bits_per_component)
          *bits_per_component = 8;

        if (bits_per_pixel)
          *bits_per_pixel = 32;

        if (bytes_per_row)
          *bytes_per_row = impl->width * 4;

        if (colorspace)
          *colorspace = CGColorSpaceCreateDeviceRGB ();

        if (alpha_info)
          *alpha_info = kCGImageAlphaPremultipliedFirst;
        break;

      case 1:
        if (bits_per_component)
          *bits_per_component = 8;

        if (bits_per_pixel)
          *bits_per_pixel = 8;

        if (bytes_per_row)
          *bytes_per_row = impl->width;

        if (colorspace)
          *colorspace = CGColorSpaceCreateWithName (kCGColorSpaceGenericGray);

        if (alpha_info)
          *alpha_info = kCGImageAlphaNone;
        break;

      default:
        g_assert_not_reached ();
        break;
    }
}

static CGContextRef
bdk_pixmap_impl_quartz_get_context (BdkDrawable *drawable,
				    gboolean     antialias)
{
  BdkPixmapImplQuartz *impl = BDK_PIXMAP_IMPL_QUARTZ (drawable);
  CGContextRef cg_context;
  gint bits_per_component, bytes_per_row;
  CGColorSpaceRef colorspace;
  CGImageAlphaInfo alpha_info;

  bdk_pixmap_impl_quartz_get_image_parameters (BDK_DRAWABLE_IMPL_QUARTZ (drawable)->wrapper,
                                               &bits_per_component,
                                               NULL,
                                               &bytes_per_row,
                                               &colorspace,
                                               &alpha_info);

  cg_context = CGBitmapContextCreate (impl->data,
                                      impl->width, impl->height,
                                      bits_per_component,
                                      bytes_per_row,
                                      colorspace,
                                      alpha_info);

  if (cg_context)
    {
      CGContextSetAllowsAntialiasing (cg_context, antialias);

      CGColorSpaceRelease (colorspace);

      /* convert coordinates from core graphics to btk+ */
      CGContextTranslateCTM (cg_context, 0, impl->height);
      CGContextScaleCTM (cg_context, 1.0, -1.0);
    }
  return cg_context;
}

static void
bdk_pixmap_impl_quartz_finalize (GObject *object)
{
  BdkPixmapImplQuartz *impl = BDK_PIXMAP_IMPL_QUARTZ (object);

  CGDataProviderRelease (impl->data_provider);

  _bdk_quartz_drawable_finish (BDK_DRAWABLE (impl));

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bdk_pixmap_impl_quartz_class_init (BdkPixmapImplQuartzClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BdkDrawableClass *drawable_class = BDK_DRAWABLE_CLASS (klass);
  BdkDrawableImplQuartzClass *drawable_quartz_class = BDK_DRAWABLE_IMPL_QUARTZ_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = bdk_pixmap_impl_quartz_finalize;

  drawable_class->get_size = bdk_pixmap_impl_quartz_get_size;

  drawable_quartz_class->get_context = bdk_pixmap_impl_quartz_get_context;
}

GType
_bdk_pixmap_impl_quartz_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      const GTypeInfo object_info =
      {
        sizeof (BdkPixmapImplQuartzClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) bdk_pixmap_impl_quartz_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (BdkPixmapImplQuartz),
        0,              /* n_preallocs */
        (GInstanceInitFunc) bdk_pixmap_impl_quartz_init
      };
      
      object_type = g_type_register_static (BDK_TYPE_DRAWABLE_IMPL_QUARTZ,
                                            "BdkPixmapImplQuartz",
                                            &object_info,
					    0);
    }
  
  return object_type;
}

GType
_bdk_pixmap_impl_get_type (void)
{
  return _bdk_pixmap_impl_quartz_get_type ();
}

static inline gboolean
depth_supported (int depth)
{
  if (depth != 24 && depth != 32 && depth != 1)
    {
      g_warning ("Unsupported bit depth %d\n", depth);
      return FALSE;
    }

  return TRUE;
}

CGImageRef
_bdk_pixmap_get_cgimage (BdkPixmap *pixmap)
{
  BdkPixmapImplQuartz *impl = BDK_PIXMAP_IMPL_QUARTZ (BDK_PIXMAP_OBJECT (pixmap)->impl);
  gint bits_per_component, bits_per_pixel, bytes_per_row;
  CGColorSpaceRef colorspace;
  CGImageAlphaInfo alpha_info;
  CGImageRef image;

  bdk_pixmap_impl_quartz_get_image_parameters (pixmap,
                                               &bits_per_component,
                                               &bits_per_pixel,
                                               &bytes_per_row,
                                               &colorspace,
                                               &alpha_info);

  image = CGImageCreate (impl->width, impl->height,
                         bits_per_component, bits_per_pixel,
                         bytes_per_row, colorspace,
                         alpha_info,
                         impl->data_provider, NULL, FALSE, 
                         kCGRenderingIntentDefault);
  CGColorSpaceRelease (colorspace);

  return image;
}

static void
data_provider_release (void *info, const void *data, size_t size)
{
  g_free (info);
}

BdkPixmap*
_bdk_pixmap_new (BdkDrawable *drawable,
                 gint         width,
                 gint         height,
                 gint         depth)
{
  BdkPixmap *pixmap;
  BdkDrawableImplQuartz *draw_impl;
  BdkPixmapImplQuartz *pix_impl;
  gint window_depth;
  gint bytes_per_row;

  g_return_val_if_fail (drawable == NULL || BDK_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail ((drawable != NULL) || (depth != -1), NULL);
  g_return_val_if_fail ((width != 0) && (height != 0), NULL);

  if (BDK_IS_WINDOW (drawable) && BDK_WINDOW_DESTROYED (drawable))
    return NULL;

  if (!drawable)
    drawable = bdk_screen_get_root_window (bdk_screen_get_default ());

  window_depth = bdk_drawable_get_depth (BDK_DRAWABLE (drawable));

  if (depth == -1)
    depth = window_depth;

  if (!depth_supported (depth))
    return NULL;

  pixmap = g_object_new (bdk_pixmap_get_type (), NULL);
  draw_impl = BDK_DRAWABLE_IMPL_QUARTZ (BDK_PIXMAP_OBJECT (pixmap)->impl);
  pix_impl = BDK_PIXMAP_IMPL_QUARTZ (BDK_PIXMAP_OBJECT (pixmap)->impl);
  draw_impl->wrapper = BDK_DRAWABLE (pixmap);

  g_assert (depth == 24 || depth == 32 || depth == 1);

  pix_impl->width = width;
  pix_impl->height = height;
  BDK_PIXMAP_OBJECT (pixmap)->depth = depth;

  bdk_pixmap_impl_quartz_get_image_parameters (pixmap,
                                               NULL, NULL,
                                               &bytes_per_row,
                                               NULL, NULL);

  pix_impl->data = g_malloc (height * bytes_per_row);
  pix_impl->data_provider = CGDataProviderCreateWithData (pix_impl->data,
                                                          pix_impl->data,
                                                          height * bytes_per_row,
                                                          data_provider_release);

  if (depth == window_depth) {
    BdkColormap *colormap = bdk_drawable_get_colormap (drawable);

    if (colormap)
      bdk_drawable_set_colormap (pixmap, colormap);
  }

  return pixmap;
}

BdkPixmap *
_bdk_bitmap_create_from_data (BdkDrawable *window,
                              const gchar *data,
                              gint         width,
                              gint         height)
{
  BdkPixmap *pixmap;
  BdkPixmapImplQuartz *impl;
  int x, y, bpl;

  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail ((width != 0) && (height != 0), NULL);
  g_return_val_if_fail (window == NULL || BDK_IS_DRAWABLE (window), NULL);

  pixmap = bdk_pixmap_new (window, width, height, 1);
  impl = BDK_PIXMAP_IMPL_QUARTZ (BDK_PIXMAP_OBJECT (pixmap)->impl);

  /* Bytes per line: Each line consumes an integer number of bytes, possibly
   * ignoring any excess bits. */
  bpl = (width + 7) / 8;
  for (y = 0; y < height; y++)
    {
      guchar *dst = impl->data + y * width;
      const gchar *src = data + (y * bpl);   
      for (x = 0; x < width; x++)
	{
	  if ((src[x / 8] >> x % 8) & 1)
	    *dst = 0xff;
	  else
	    *dst = 0;

	  dst++;
	}
    }

  return pixmap;
}

BdkPixmap*
_bdk_pixmap_create_from_data (BdkDrawable    *drawable,
                              const gchar    *data,
                              gint            width,
                              gint            height,
                              gint            depth,
                              const BdkColor *fg,
                              const BdkColor *bg)
{	
  /* FIXME: Implement */
  return NULL;
}

BdkPixmap *
bdk_pixmap_foreign_new_for_display (BdkDisplay      *display,
				    BdkNativeWindow  anid)
{
  return NULL;
}

BdkPixmap*
bdk_pixmap_foreign_new (BdkNativeWindow anid)
{
   return NULL;
}

BdkPixmap *
bdk_pixmap_foreign_new_for_screen (BdkScreen       *screen,
				   BdkNativeWindow  anid,
				   gint             width,
				   gint             height,
				   gint             depth)
{
  return NULL;
}

BdkPixmap*
bdk_pixmap_lookup (BdkNativeWindow anid)
{
  return NULL;
}

BdkPixmap*
bdk_pixmap_lookup_for_display (BdkDisplay *display, BdkNativeWindow anid)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);
  return NULL;
}
