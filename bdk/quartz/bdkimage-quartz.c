/* bdkimage-quartz.c
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

#include "bdk.h"
#include "bdkimage.h"
#include "bdkprivate-quartz.h"

static BObjectClass *parent_class;

BdkImage *
_bdk_quartz_image_copy_to_image (BdkDrawable *drawable,
				 BdkImage    *image,
				 bint         src_x,
				 bint         src_y,
				 bint         dest_x,
				 bint         dest_y,
				 bint         width,
				 bint         height)
{
  BdkScreen *screen;
  
  g_return_val_if_fail (BDK_IS_DRAWABLE_IMPL_QUARTZ (drawable), NULL);
  g_return_val_if_fail (image != NULL || (dest_x == 0 && dest_y == 0), NULL);

  screen = bdk_drawable_get_screen (drawable);
  if (!image)
    image = _bdk_image_new_for_depth (screen, BDK_IMAGE_FASTEST, NULL, 
				      width, height,
				      bdk_drawable_get_depth (drawable));
  
  if (BDK_IS_PIXMAP_IMPL_QUARTZ (drawable))
    {
      BdkPixmapImplQuartz *pix_impl;
      bint bytes_per_row;
      buchar *data;
      int x, y;

      pix_impl = BDK_PIXMAP_IMPL_QUARTZ (drawable);
      data = (buchar *)(pix_impl->data);

      if (src_x + width > pix_impl->width || src_y + height > pix_impl->height)
      	{
          g_warning ("Out of bounds copy-area for pixmap -> image conversion\n");
          return image;
        }

      switch (bdk_drawable_get_depth (drawable))
        {
        case 24:
          bytes_per_row = pix_impl->width * 4;
          for (y = 0; y < height; y++)
            {
              buchar *src = data + ((y + src_y) * bytes_per_row) + (src_x * 4);

              for (x = 0; x < width; x++)
                {
                  bint32 pixel;
	  
                  /* RGB24, 4 bytes per pixel, skip first. */
                  pixel = src[0] << 16 | src[1] << 8 | src[2];
                  src += 4;

                  bdk_image_put_pixel (image, dest_x + x, dest_y + y, pixel);
                }
            }
          break;

        case 32:
          bytes_per_row = pix_impl->width * 4;
          for (y = 0; y < height; y++)
            {
              buchar *src = data + ((y + src_y) * bytes_per_row) + (src_x * 4);

              for (x = 0; x < width; x++)
                {
                  bint32 pixel;
	  
                  /* ARGB32, 4 bytes per pixel. */
                  pixel = src[0] << 24 | src[1] << 16 | src[2] << 8 | src[3];
                  src += 4;

                  bdk_image_put_pixel (image, dest_x + x, dest_y + y, pixel);
                }
            }
          break;

        case 1: /* TODO: optimize */
          bytes_per_row = pix_impl->width;
          for (y = 0; y < height; y++)
            {
              buchar *src = data + ((y + src_y) * bytes_per_row) + src_x;

              for (x = 0; x < width; x++)
                {
                  bint32 pixel;
	  
                  /* 8 bits */
                  pixel = src[0];
                  src++;

                  bdk_image_put_pixel (image, dest_x + x, dest_y + y, pixel);
                }
            }
          break;

        default:
          g_warning ("Unsupported bit depth %d\n", bdk_drawable_get_depth (drawable));
          return image;
        }
    }
  else if (BDK_IS_WINDOW_IMPL_QUARTZ (drawable))
    {
      BdkQuartzView *view;
      NSBitmapImageRep *rep;
      buchar *data;
      int x, y;
      NSSize size;
      NSBitmapFormat format;
      bboolean has_alpha;
      bint bpp;
      bint r_byte = 0;
      bint g_byte = 1;
      bint b_byte = 2;
      bint a_byte = 3;
      bboolean le_image_data = FALSE;

      if (BDK_WINDOW_IMPL_QUARTZ (drawable) == BDK_WINDOW_IMPL_QUARTZ (BDK_WINDOW_OBJECT (_bdk_root)->impl))
        {
          /* Special case for the root window. */
	  CGRect rect = CGRectMake (src_x, src_y, width, height);
          CGImageRef root_image_ref = CGWindowListCreateImage (rect,
                                                               kCGWindowListOptionOnScreenOnly,
                                                               kCGNullWindowID,
                                                               kCGWindowImageDefault);

          /* HACK: the NSBitmapImageRep does not copy and convert
           * CGImageRef's data so it matches what NSBitmapImageRep can
           * express in its API (which is RGBA and ARGB, premultiplied
           * and unpremultiplied), it only references the CGImageRef.
           * Therefore we need to do the host byte swapping ourselves.
           */
          if (CGImageGetBitmapInfo (root_image_ref) & kCGBitmapByteOrder32Little)
            {
              r_byte = 3;
              g_byte = 2;
              b_byte = 1;
              a_byte = 0;

              le_image_data = TRUE;
            }

          rep = [[NSBitmapImageRep alloc] initWithCGImage: root_image_ref];
          CGImageRelease (root_image_ref);
        }
      else
        {
	  NSRect rect = NSMakeRect (src_x, src_y, width, height);
          view = BDK_WINDOW_IMPL_QUARTZ (drawable)->view;

          /* We return the image even if we can't copy to it. */
          if (![view lockFocusIfCanDraw])
            return image;

          rep = [[NSBitmapImageRep alloc] initWithFocusedViewRect: rect];
          [view unlockFocus];
        }

      data = [rep bitmapData];
      size = [rep size];
      format = [rep bitmapFormat];
      has_alpha = [rep hasAlpha];
      bpp = [rep bitsPerPixel] / 8;

      /* MORE HACK: AlphaFirst seems set for le_image_data, which is
       * technically correct, but really apple, are you kidding, it's
       * in fact ABGR, not ARGB as promised in NSBitmapImageRep's API.
       */
      if (!le_image_data && (format & NSAlphaFirstBitmapFormat))
        {
          r_byte = 1;
          g_byte = 2;
          b_byte = 3;
          a_byte = 0;
        }

      for (y = 0; y < size.height; y++)
        {
          buchar *src = data + y * [rep bytesPerRow];

          for (x = 0; x < size.width; x++)
            {
              buchar r = src[r_byte];
              buchar g = src[g_byte];
              buchar b = src[b_byte];
              bint32 pixel;

              if (has_alpha)
                {
                  buchar alpha = src[a_byte];

                  /* unpremultiply if alpha > 0 */
                  if (! (format & NSAlphaNonpremultipliedBitmapFormat) && alpha)
                    {
                      r = r * 255 / alpha;
                      g = g * 255 / alpha;
                      b = b * 255 / alpha;
                    }

                  if (image->byte_order == BDK_MSB_FIRST)
                    pixel = alpha | b << 8 | g << 16 | r << 24;
                  else
                    pixel = alpha << 24 | b << 16 | g << 8 | r;
                }
              else
                {
                  if (image->byte_order == BDK_MSB_FIRST)
                    pixel = b | g << 8 | r << 16;
                  else
                    pixel = b << 16 | g << 8 | r;
                }

              src += bpp;

              bdk_image_put_pixel (image, dest_x + x, dest_y + y, pixel);
            }
        }

      [rep release];
    }

  return image;
}

static void
bdk_image_finalize (BObject *object)
{
  BdkImage *image = BDK_IMAGE (object);

  g_free (image->mem);

  B_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bdk_image_class_init (BdkImageClass *klass)
{
  BObjectClass *object_class = B_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  
  object_class->finalize = bdk_image_finalize;
}

GType
bdk_image_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      const GTypeInfo object_info =
      {
        sizeof (BdkImageClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) bdk_image_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (BdkImage),
        0,              /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };
      
      object_type = g_type_register_static (B_TYPE_OBJECT,
                                            "BdkImage",
                                            &object_info,
					    0);
    }
  
  return object_type;
}

BdkImage *
bdk_image_new_bitmap (BdkVisual *visual, bpointer data, bint width, bint height)
{
  /* We don't implement this function because it's broken, deprecated and 
   * tricky to implement. */
  g_warning ("This function is unimplemented");

  return NULL;
}

BdkImage*
_bdk_image_new_for_depth (BdkScreen    *screen,
			  BdkImageType  type,
			  BdkVisual    *visual,
			  bint          width,
			  bint          height,
			  bint          depth)
{
  BdkImage *image;

  if (visual)
    depth = visual->depth;

  g_assert (depth == 24 || depth == 32);

  image = g_object_new (bdk_image_get_type (), NULL);
  image->type = type;
  image->visual = visual;
  image->width = width;
  image->height = height;
  image->depth = depth;

  image->byte_order = (G_BYTE_ORDER == G_LITTLE_ENDIAN) ? BDK_LSB_FIRST : BDK_MSB_FIRST;

  /* We only support images with bpp 4 */
  image->bpp = 4;
  image->bpl = image->width * image->bpp;
  image->bits_per_pixel = image->bpp * 8;
  
  image->mem = g_malloc (image->bpl * image->height);
  memset (image->mem, 0x00, image->bpl * image->height);

  return image;
}

buint32
bdk_image_get_pixel (BdkImage *image,
		     bint x,
		     bint y)
{
  buchar *ptr;

  g_return_val_if_fail (image != NULL, 0);
  g_return_val_if_fail (x >= 0 && x < image->width, 0);
  g_return_val_if_fail (y >= 0 && y < image->height, 0);

  ptr = image->mem + y * image->bpl + x * image->bpp;

  return *(buint32 *)ptr;
}

void
bdk_image_put_pixel (BdkImage *image,
		     bint x,
		     bint y,
		     buint32 pixel)
{
  buchar *ptr;

  ptr = image->mem + y * image->bpl + x * image->bpp;

  *(buint32 *)ptr = pixel;
}

bint
_bdk_windowing_get_bits_for_depth (BdkDisplay *display,
				   bint        depth)
{
  if (depth == 24 || depth == 32)
    return 32;
  else
    g_assert_not_reached ();

  return 0;
}
