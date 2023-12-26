/* BDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998-2002 Tor Lillqvist
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

#include "config.h"
#include "bdkimage.h"
#include "bdkpixmap.h"
#include "bdkscreen.h" /* bdk_screen_get_default() */
#include "bdkprivate-win32.h"

static GList *image_list = NULL;
static bpointer parent_class = NULL;

static void bdk_win32_image_destroy (BdkImage      *image);
static void bdk_image_init          (BdkImage      *image);
static void bdk_image_class_init    (BdkImageClass *klass);
static void bdk_image_finalize      (BObject       *object);

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
        (GInstanceInitFunc) bdk_image_init,
      };
      
      object_type = g_type_register_static (B_TYPE_OBJECT,
                                            "BdkImage",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void
bdk_image_init (BdkImage *image)
{
  image->windowing_data = NULL;
}

static void
bdk_image_class_init (BdkImageClass *klass)
{
  BObjectClass *object_class = B_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = bdk_image_finalize;
}

static void
bdk_image_finalize (BObject *object)
{
  BdkImage *image = BDK_IMAGE (object);

  bdk_win32_image_destroy (image);
  
  B_OBJECT_CLASS (parent_class)->finalize (object);
}

void
_bdk_image_exit (void)
{
  BdkImage *image;

  while (image_list)
    {
      image = image_list->data;
      bdk_win32_image_destroy (image);
    }
}

/*
 * Create a BdkImage _without_ an associated BdkPixmap. The caller is
 * responsible for creating a BdkPixmap object and making the association.
 */

static BdkImage *
_bdk_win32_new_image (BdkVisual *visual,
		      bint       width,
		      bint       height,
		      bint       depth,
		      buchar    *bits)
{
  BdkImage *image;

  image = g_object_new (bdk_image_get_type (), NULL);
  image->windowing_data = NULL;
  image->type = BDK_IMAGE_SHARED;
  image->visual = visual;
  image->byte_order = BDK_LSB_FIRST;
  image->width = width;
  image->height = height;
  image->depth = depth;
  image->bits_per_pixel = _bdk_windowing_get_bits_for_depth (bdk_display_get_default (), depth);
  switch (depth)
    {
    case 1:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
      image->bpp = 1;
      break;
    case 15:
    case 16:
      image->bpp = 2;
      break;
    case 24:
      image->bpp = image->bits_per_pixel / 8;
      break;
    case 32:
      image->bpp = 4;
      break;
    default:
      g_warning ("_bdk_win32_new_image: depth=%d", image->depth);
      g_assert_not_reached ();
    }
  if (depth == 1)
    image->bpl = ((width - 1)/32 + 1)*4;
  else if (depth == 4)
    image->bpl = ((width - 1)/8 + 1)*4;
  else
    image->bpl = ((width*image->bpp - 1)/4 + 1)*4;
  image->mem = bits;

  return image;
}

BdkImage *
bdk_image_new_bitmap (BdkVisual *visual,
		      bpointer   data,
		      bint       w,
		      bint       h)
{
  BdkPixmap *pixmap;
  BdkImage *image;
  buchar *bits;
  bint data_bpl = (w-1)/8 + 1;
  bint i;

  pixmap = bdk_pixmap_new (NULL, w, h, 1);

  if (pixmap == NULL)
    return NULL;

  BDK_NOTE (IMAGE, g_print ("bdk_image_new_bitmap: %dx%d=%p\n",
			    w, h, BDK_PIXMAP_HBITMAP (pixmap)));

  bits = BDK_PIXMAP_IMPL_WIN32 (BDK_PIXMAP_OBJECT (pixmap)->impl)->bits;
  image = _bdk_win32_new_image (visual, w, h, 1, bits);
  image->windowing_data = pixmap;
  
  if (data_bpl != image->bpl)
    {
      for (i = 0; i < h; i++)
	memmove ((buchar *) image->mem + i*image->bpl, ((buchar *) data) + i*data_bpl, data_bpl);
    }
  else
    memmove (image->mem, data, data_bpl*h);

  return image;
}

void
_bdk_windowing_image_init (void)
{
  /* Nothing needed AFAIK */
}

BdkImage*
_bdk_image_new_for_depth (BdkScreen    *screen,
			  BdkImageType  type,
			  BdkVisual    *visual,
			  bint          width,
			  bint          height,
			  bint          depth)
{
  BdkPixmap *pixmap;
  BdkImage *image;
  buchar *bits;

  g_return_val_if_fail (!visual || BDK_IS_VISUAL (visual), NULL);
  g_return_val_if_fail (visual || depth != -1, NULL);
  g_return_val_if_fail (screen == bdk_screen_get_default (), NULL);
 
  if (visual)
    depth = visual->depth;

  pixmap = bdk_pixmap_new (NULL, width, height, depth);

  if (pixmap == NULL)
    return NULL;

  BDK_NOTE (IMAGE, g_print ("_bdk_image_new_for_depth: %dx%dx%d=%p\n",
			    width, height, depth, BDK_PIXMAP_HBITMAP (pixmap)));
  
  bits = BDK_PIXMAP_IMPL_WIN32 (BDK_PIXMAP_OBJECT (pixmap)->impl)->bits;
  image = _bdk_win32_new_image (visual, width, height, depth, bits);
  image->windowing_data = pixmap;
  
  return image;
}

BdkImage*
_bdk_win32_copy_to_image (BdkDrawable    *drawable,
			  BdkImage       *image,
			  bint            src_x,
			  bint            src_y,
			  bint            dest_x,
			  bint            dest_y,
			  bint            width,
			  bint            height)
{
  BdkGC *gc;
  BdkScreen *screen = bdk_drawable_get_screen (drawable);
  
  g_return_val_if_fail (BDK_IS_DRAWABLE_IMPL_WIN32 (drawable), NULL);
  g_return_val_if_fail (image != NULL || (dest_x == 0 && dest_y == 0), NULL);

  BDK_NOTE (IMAGE, g_print ("_bdk_win32_copy_to_image: %p\n",
			    BDK_DRAWABLE_HANDLE (drawable)));

  if (!image)
    image = _bdk_image_new_for_depth (screen, BDK_IMAGE_FASTEST, NULL, width, height,
				      bdk_drawable_get_depth (drawable));

  gc = bdk_gc_new ((BdkDrawable *) image->windowing_data);
  _bdk_win32_blit
    (FALSE,
     BDK_DRAWABLE_IMPL_WIN32 (BDK_PIXMAP_OBJECT (image->windowing_data)->impl),
     gc, drawable, src_x, src_y, dest_x, dest_y, width, height);
  g_object_unref (gc);

  return image;
}

buint32
bdk_image_get_pixel (BdkImage *image,
		     bint      x,
		     bint      y)
{
  buchar *pixelp;

  g_return_val_if_fail (image != NULL, 0);
  g_return_val_if_fail (x >= 0 && x < image->width, 0);
  g_return_val_if_fail (y >= 0 && y < image->height, 0);

  if (!(x >= 0 && x < image->width && y >= 0 && y < image->height))
      return 0;

  if (image->depth == 1)
    return (((buchar *) image->mem)[y * image->bpl + (x >> 3)] & (1 << (7 - (x & 0x7)))) != 0;

  if (image->depth == 4)
    {
      pixelp = (buchar *) image->mem + y * image->bpl + (x >> 1);
      if (x&1)
	return (*pixelp) & 0x0F;

      return (*pixelp) >> 4;
    }
    
  pixelp = (buchar *) image->mem + y * image->bpl + x * image->bpp;
      
  switch (image->bpp)
    {
    case 1:
      return *pixelp;
      
      /* Windows is always LSB, no need to check image->byte_order. */
    case 2:
      return pixelp[0] | (pixelp[1] << 8);
      
    case 3:
      return pixelp[0] | (pixelp[1] << 8) | (pixelp[2] << 16);

    case 4:
      return pixelp[0] | (pixelp[1] << 8) | (pixelp[2] << 16);
    }
  g_assert_not_reached ();
  return 0;
}

void
bdk_image_put_pixel (BdkImage *image,
		     bint       x,
		     bint       y,
		     buint32    pixel)
{
  buchar *pixelp;

  g_return_if_fail (image != NULL);
  g_return_if_fail (x >= 0 && x < image->width);
  g_return_if_fail (y >= 0 && y < image->height);

  if  (!(x >= 0 && x < image->width && y >= 0 && y < image->height))
    return;

  GdiFlush ();
  if (image->depth == 1)
    if (pixel & 1)
      ((buchar *) image->mem)[y * image->bpl + (x >> 3)] |= (1 << (7 - (x & 0x7)));
    else
      ((buchar *) image->mem)[y * image->bpl + (x >> 3)] &= ~(1 << (7 - (x & 0x7)));
  else if (image->depth == 4)
    {
      pixelp = (buchar *) image->mem + y * image->bpl + (x >> 1);

      if (x&1)
	{
	  *pixelp &= 0xF0;
	  *pixelp |= (pixel & 0x0F);
	}
      else
	{
	  *pixelp &= 0x0F;
	  *pixelp |= (pixel << 4);
	}
    }
  else
    {
      pixelp = (buchar *) image->mem + y * image->bpl + x * image->bpp;
      
      /* Windows is always LSB, no need to check image->byte_order. */
      switch (image->bpp)
	{
	case 4:
	  pixelp[3] = 0;
	case 3:
	  pixelp[2] = ((pixel >> 16) & 0xFF);
	case 2:
	  pixelp[1] = ((pixel >> 8) & 0xFF);
	case 1:
	  pixelp[0] = (pixel & 0xFF);
	}
    }
}

static void
bdk_win32_image_destroy (BdkImage *image)
{
  BdkPixmap *pixmap;

  g_return_if_fail (BDK_IS_IMAGE (image));

  pixmap = image->windowing_data;

  if (pixmap == NULL)		/* This means that _bdk_image_exit()
				 * destroyed the image already, and
				 * now we're called a second time from
				 * _finalize()
				 */
    return;
  
  BDK_NOTE (IMAGE, g_print ("bdk_win32_image_destroy: %p\n",
			    BDK_PIXMAP_HBITMAP (pixmap)));

  g_object_unref (pixmap);
  image->windowing_data = NULL;
}

bint
_bdk_windowing_get_bits_for_depth (BdkDisplay *display,
                                   bint        depth)
{
  g_return_val_if_fail (display == bdk_display_get_default (), 0);

  switch (depth)
    {
    case 1:
      return 1;

    case 2:
    case 3:
    case 4:
      return 4;

    case 5:
    case 6:
    case 7:
    case 8:
      return 8;

    case 15:
    case 16:
      return 16;

    case 24:
    case 32:
      return 32;
    }
  g_assert_not_reached ();
  return 0;
}
