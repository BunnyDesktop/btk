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

#include "config.h"
#include <stdlib.h>
#include <sys/types.h>

#include "bdk.h"		/* For bdk_flush() */
#include "bdkimage.h"
#include "bdkprivate.h"
#include "bdkinternals.h"	/* For scratch_image code */
#include "bdkalias.h"

/**
 * bdk_image_ref:
 * @image: a #BdkImage
 *
 * Deprecated function; use g_object_ref() instead.
 * 
 * Return value: the image
 *
 * Deprecated: 2.0: Use g_object_ref() instead.
 **/
BdkImage *
bdk_image_ref (BdkImage *image)
{
  g_return_val_if_fail (BDK_IS_IMAGE (image), NULL);

  return g_object_ref (image);
}

/**
 * bdk_image_unref:
 * @image: a #BdkImage
 *
 * Deprecated function; use g_object_unref() instead.
 *
 * Deprecated: 2.0: Use g_object_unref() instead.
 **/
void
bdk_image_unref (BdkImage *image)
{
  g_return_if_fail (BDK_IS_IMAGE (image));

  g_object_unref (image);
}

/**
 * bdk_image_get:
 * @drawable: a #BdkDrawable
 * @x: x coordinate in @window
 * @y: y coordinate in @window
 * @width: width of area in @window
 * @height: height of area in @window
 * 
 * This is a deprecated wrapper for bdk_drawable_get_image();
 * bdk_drawable_get_image() should be used instead. Or even better: in
 * most cases bdk_pixbuf_get_from_drawable() is the most convenient
 * choice.
 * 
 * Return value: a new #BdkImage or %NULL
 **/
BdkImage*
bdk_image_get (BdkWindow *drawable,
	       gint       x,
	       gint       y,
	       gint       width,
	       gint       height)
{
  g_return_val_if_fail (BDK_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (x >= 0, NULL);
  g_return_val_if_fail (y >= 0, NULL);
  g_return_val_if_fail (width >= 0, NULL);
  g_return_val_if_fail (height >= 0, NULL);
  
  return bdk_drawable_get_image (drawable, x, y, width, height);
}

/**
 * bdk_image_set_colormap:
 * @image: a #BdkImage
 * @colormap: a #BdkColormap
 * 
 * Sets the colormap for the image to the given colormap.  Normally
 * there's no need to use this function, images are created with the
 * correct colormap if you get the image from a drawable. If you
 * create the image from scratch, use the colormap of the drawable you
 * intend to render the image to.
 *
 * Deprecated: 2.22: #BdkImage should not be used anymore.
 **/
void
bdk_image_set_colormap (BdkImage       *image,
                        BdkColormap    *colormap)
{
  g_return_if_fail (BDK_IS_IMAGE (image));
  g_return_if_fail (BDK_IS_COLORMAP (colormap));

  if (image->colormap != colormap)
    {
      if (image->colormap)
	g_object_unref (image->colormap);

      image->colormap = colormap;
      g_object_ref (image->colormap);
    }
}

/**
 * bdk_image_get_colormap:
 * @image: a #BdkImage
 * 
 * Retrieves the colormap for a given image, if it exists.  An image
 * will have a colormap if the drawable from which it was created has
 * a colormap, or if a colormap was set explicitely with
 * bdk_image_set_colormap().
 * 
 * Return value: colormap for the image
 *
 * Deprecated: 2.22: #BdkImage should not be used anymore.
 **/
BdkColormap *
bdk_image_get_colormap (BdkImage *image)
{
  g_return_val_if_fail (BDK_IS_IMAGE (image), NULL);

  return image->colormap;
}

/**
 * bdk_image_get_image_type:
 * @image: a #BdkImage
 *
 * Determines the type of a given image.
 *
 * Return value: the #BdkImageType of the image
 *
 * Since: 2.22
 *
 * Deprecated: 2.22: #BdkImage should not be used anymore.
 **/
BdkImageType
bdk_image_get_image_type (BdkImage *image)
{
  g_return_val_if_fail (BDK_IS_IMAGE (image), 0);

  return image->type;
}

/**
 * bdk_image_get_visual:
 * @image: a #BdkImage
 *
 * Determines the visual that was used to create the image.
 *
 * Return value: a #BdkVisual
 *
 * Since: 2.22
 *
 * Deprecated: 2.22: #BdkImage should not be used anymore.
 **/
BdkVisual *
bdk_image_get_visual (BdkImage *image)
{
  g_return_val_if_fail (BDK_IS_IMAGE (image), NULL);

  return image->visual;
}

/**
 * bdk_image_get_byte_order:
 * @image: a #BdkImage
 *
 * Determines the byte order of the image.
 *
 * Return value: a #BdkVisual
 *
 * Since: 2.22
 *
 * Deprecated: 2.22: #BdkImage should not be used anymore.
 **/
BdkByteOrder
bdk_image_get_byte_order (BdkImage *image)
{
  g_return_val_if_fail (BDK_IS_IMAGE (image), 0);

  return image->byte_order;
}

/**
 * bdk_image_get_width:
 * @image: a #BdkImage
 *
 * Determines the width of the image.
 *
 * Return value: the width
 *
 * Since: 2.22
 *
 * Deprecated: 2.22: #BdkImage should not be used anymore.
 **/
gint
bdk_image_get_width (BdkImage *image)
{
  g_return_val_if_fail (BDK_IS_IMAGE (image), 0);

  return image->width;
}

/**
 * bdk_image_get_height:
 * @image: a #BdkImage
 *
 * Determines the height of the image.
 *
 * Return value: the height
 *
 * Since: 2.22
 *
 * Deprecated: 2.22: #BdkImage should not be used anymore.
 **/
gint
bdk_image_get_height (BdkImage *image)
{
  g_return_val_if_fail (BDK_IS_IMAGE (image), 0);

  return image->height;
}

/**
 * bdk_image_get_depth:
 * @image: a #BdkImage
 *
 * Determines the depth of the image.
 *
 * Return value: the depth
 *
 * Since: 2.22
 *
 * Deprecated: 2.22: #BdkImage should not be used anymore.
 **/
guint16
bdk_image_get_depth (BdkImage *image)
{
  g_return_val_if_fail (BDK_IS_IMAGE (image), 0);

  return image->depth;
}

/**
 * bdk_image_get_bytes_per_pixel:
 * @image: a #BdkImage
 *
 * Determines the number of bytes per pixel of the image.
 *
 * Return value: the bytes per pixel
 *
 * Since: 2.22
 *
 * Deprecated: 2.22: #BdkImage should not be used anymore.
 **/
guint16
bdk_image_get_bytes_per_pixel (BdkImage *image)
{
  g_return_val_if_fail (BDK_IS_IMAGE (image), 0);

  return image->bpp;
}

/**
 * bdk_image_get_bytes_per_line:
 * @image: a #BdkImage
 *
 * Determines the number of bytes per line of the image.
 *
 * Return value: the bytes per line
 *
 * Since: 2.22
 *
 * Deprecated: 2.22: #BdkImage should not be used anymore.
 **/
guint16
bdk_image_get_bytes_per_line (BdkImage *image)
{
  g_return_val_if_fail (BDK_IS_IMAGE (image), 0);

  return image->bpl;
}

/**
 * bdk_image_get_bits_per_pixel:
 * @image: a #BdkImage
 *
 * Determines the number of bits per pixel of the image.
 *
 * Return value: the bits per pixel
 *
 * Since: 2.22
 *
 * Deprecated: 2.22: #BdkImage should not be used anymore.
 **/
guint16
bdk_image_get_bits_per_pixel (BdkImage *image)
{
  g_return_val_if_fail (BDK_IS_IMAGE (image), 0);

  return image->bits_per_pixel;
}

/**
 * bdk_image_get_pixels:
 * @image: a #BdkImage
 *
 * Returns a pointer to the pixel data of the image.
 *
 * Returns: the pixel data of the image
 *
 * Since: 2.22
 *
 * Deprecated: 2.22: #BdkImage should not be used anymore.
 */
gpointer
bdk_image_get_pixels (BdkImage *image)
{
  g_return_val_if_fail (BDK_IS_IMAGE (image), NULL);

  return image->mem;
}

/* We have N_REBUNNYION BDK_SCRATCH_IMAGE_WIDTH x BDK_SCRATCH_IMAGE_HEIGHT rebunnyions divided
 * up between n_images different images. possible_n_images gives
 * various divisors of N_REBUNNYIONS. The reason for allowing this
 * flexibility is that we want to create as few images as possible,
 * but we want to deal with the abberant systems that have a SHMMAX
 * limit less than
 *
 * BDK_SCRATCH_IMAGE_WIDTH * BDK_SCRATCH_IMAGE_HEIGHT * N_REBUNNYIONS * 4 (384k)
 *
 * (Are there any such?)
 */
#define N_REBUNNYIONS 6
static const int possible_n_images[] = { 1, 2, 3, 6 };

/* We allocate one BdkScratchImageInfo structure for each
 * depth where we are allocating scratch images. (Future: one
 * per depth, per display)
 */
typedef struct _BdkScratchImageInfo BdkScratchImageInfo;

struct _BdkScratchImageInfo {
  gint depth;
  
  gint n_images;
  BdkImage *static_image[N_REBUNNYIONS];
  gint static_image_idx;

  /* In order to optimize filling fractions, we simultaneously fill in up
   * to three rebunnyions of size BDK_SCRATCH_IMAGE_WIDTH * BDK_SCRATCH_IMAGE_HEIGHT: one
   * for images that are taller than BDK_SCRATCH_IMAGE_HEIGHT / 2, and must
   * be tiled horizontally. One for images that are wider than
   * BDK_SCRATCH_IMAGE_WIDTH / 2 and must be tiled vertically, and a third
   * for images smaller than BDK_SCRATCH_IMAGE_HEIGHT / 2 x BDK_SCRATCH_IMAGE_WIDTH x 2
   * that we tile in horizontal rows.
   */
  gint horiz_idx;
  gint horiz_y;
  gint vert_idx;
  gint vert_x;
  
  /* tile_y1 and tile_y2 define the horizontal band into
   * which we are tiling images. tile_x is the x extent to
   * which that is filled
   */
  gint tile_idx;
  gint tile_x;
  gint tile_y1;
  gint tile_y2;

  BdkScreen *screen;
};

static GSList *scratch_image_infos = NULL;

static gboolean
allocate_scratch_images (BdkScratchImageInfo *info,
			 gint                 n_images,
			 gboolean             shared)
{
  gint i;
  
  for (i = 0; i < n_images; i++)
    {
      info->static_image[i] = _bdk_image_new_for_depth (info->screen,
							shared ? BDK_IMAGE_SHARED : BDK_IMAGE_NORMAL,
							NULL,
							BDK_SCRATCH_IMAGE_WIDTH * (N_REBUNNYIONS / n_images), 
							BDK_SCRATCH_IMAGE_HEIGHT,
							info->depth);
      
      if (!info->static_image[i])
	{
	  gint j;
	  
	  for (j = 0; j < i; j++)
	    g_object_unref (info->static_image[j]);
	  
	  return FALSE;
	}
    }
  
  return TRUE;
}

static void
scratch_image_info_display_closed (BdkDisplay          *display,
                                   gboolean             is_error,
                                   BdkScratchImageInfo *image_info)
{
  gint i;

  g_signal_handlers_disconnect_by_func (display,
                                        scratch_image_info_display_closed,
                                        image_info);

  scratch_image_infos = g_slist_remove (scratch_image_infos, image_info);

  for (i = 0; i < image_info->n_images; i++)
    g_object_unref (image_info->static_image[i]);

  g_free (image_info);
}

static BdkScratchImageInfo *
scratch_image_info_for_depth (BdkScreen *screen,
			      gint       depth)
{
  GSList *tmp_list;
  BdkScratchImageInfo *image_info;
  gint i;

  tmp_list = scratch_image_infos;
  while (tmp_list)
    {
      image_info = tmp_list->data;
      if (image_info->depth == depth && image_info->screen == screen)
	return image_info;
      
      tmp_list = tmp_list->next;
    }

  image_info = g_new (BdkScratchImageInfo, 1);

  image_info->depth = depth;
  image_info->screen = screen;

  g_signal_connect (bdk_screen_get_display (screen), "closed",
                    G_CALLBACK (scratch_image_info_display_closed),
                    image_info);

  /* Try to allocate as few possible shared images */
  for (i=0; i < G_N_ELEMENTS (possible_n_images); i++)
    {
      if (allocate_scratch_images (image_info, possible_n_images[i], TRUE))
	{
	  image_info->n_images = possible_n_images[i];
	  break;
	}
    }

  /* If that fails, just allocate N_REBUNNYIONS normal images */
  if (i == G_N_ELEMENTS (possible_n_images))
    {
      allocate_scratch_images (image_info, N_REBUNNYIONS, FALSE);
      image_info->n_images = N_REBUNNYIONS;
    }

  image_info->static_image_idx = 0;

  image_info->horiz_y = BDK_SCRATCH_IMAGE_HEIGHT;
  image_info->vert_x = BDK_SCRATCH_IMAGE_WIDTH;
  image_info->tile_x = BDK_SCRATCH_IMAGE_WIDTH;
  image_info->tile_y1 = image_info->tile_y2 = BDK_SCRATCH_IMAGE_HEIGHT;

  scratch_image_infos = g_slist_prepend (scratch_image_infos, image_info);

  return image_info;
}

/* Defining NO_FLUSH can cause inconsistent screen updates, but is useful
   for performance evaluation. */

#undef NO_FLUSH

#ifdef VERBOSE
static gint sincelast;
#endif

static gint
alloc_scratch_image (BdkScratchImageInfo *image_info)
{
  if (image_info->static_image_idx == N_REBUNNYIONS)
    {
#ifndef NO_FLUSH
      bdk_flush ();
#endif
#ifdef VERBOSE
      g_print ("flush, %d puts since last flush\n", sincelast);
      sincelast = 0;
#endif
      image_info->static_image_idx = 0;

      /* Mark all rebunnyions that we might be filling in as completely
       * full, to force new tiles to be allocated for subsequent
       * images
       */
      image_info->horiz_y = BDK_SCRATCH_IMAGE_HEIGHT;
      image_info->vert_x = BDK_SCRATCH_IMAGE_WIDTH;
      image_info->tile_x = BDK_SCRATCH_IMAGE_WIDTH;
      image_info->tile_y1 = image_info->tile_y2 = BDK_SCRATCH_IMAGE_HEIGHT;
    }
  return image_info->static_image_idx++;
}

/**
 * _bdk_image_get_scratch:
 * @screen: a #BdkScreen
 * @width: desired width
 * @height: desired height
 * @depth: depth of image 
 * @x: X location within returned image of scratch image
 * @y: Y location within returned image of scratch image
 * 
 * Allocates an image of size width/height, up to a maximum
 * of BDK_SCRATCH_IMAGE_WIDTHxBDK_SCRATCH_IMAGE_HEIGHT that is
 * suitable to use on @screen.
 * 
 * Return value: a scratch image. This must be used by a
 *  call to bdk_image_put() before any other calls to
 *  _bdk_image_get_scratch()
 **/
BdkImage *
_bdk_image_get_scratch (BdkScreen   *screen,
			gint	     width,			
			gint	     height,
			gint	     depth,
			gint	    *x,
			gint	    *y)
{
  BdkScratchImageInfo *image_info;
  BdkImage *image;
  gint idx;
  
  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);

  image_info = scratch_image_info_for_depth (screen, depth);

  if (width >= (BDK_SCRATCH_IMAGE_WIDTH >> 1))
    {
      if (height >= (BDK_SCRATCH_IMAGE_HEIGHT >> 1))
	{
	  idx = alloc_scratch_image (image_info);
	  *x = 0;
	  *y = 0;
	}
      else
	{
	  if (height + image_info->horiz_y > BDK_SCRATCH_IMAGE_HEIGHT)
	    {
	      image_info->horiz_idx = alloc_scratch_image (image_info);
	      image_info->horiz_y = 0;
	    }
	  idx = image_info->horiz_idx;
	  *x = 0;
	  *y = image_info->horiz_y;
	  image_info->horiz_y += height;
	}
    }
  else
    {
      if (height >= (BDK_SCRATCH_IMAGE_HEIGHT >> 1))
	{
	  if (width + image_info->vert_x > BDK_SCRATCH_IMAGE_WIDTH)
	    {
	      image_info->vert_idx = alloc_scratch_image (image_info);
	      image_info->vert_x = 0;
	    }
	  idx = image_info->vert_idx;
	  *x = image_info->vert_x;
	  *y = 0;
	  /* using 3 and -4 would be slightly more efficient on 32-bit machines
	     with > 1bpp displays */
	  image_info->vert_x += (width + 7) & -8;
	}
      else
	{
	  if (width + image_info->tile_x > BDK_SCRATCH_IMAGE_WIDTH)
	    {
	      image_info->tile_y1 = image_info->tile_y2;
	      image_info->tile_x = 0;
	    }
	  if (height + image_info->tile_y1 > BDK_SCRATCH_IMAGE_HEIGHT)
	    {
	      image_info->tile_idx = alloc_scratch_image (image_info);
	      image_info->tile_x = 0;
	      image_info->tile_y1 = 0;
	      image_info->tile_y2 = 0;
	    }
	  if (height + image_info->tile_y1 > image_info->tile_y2)
	    image_info->tile_y2 = height + image_info->tile_y1;
	  idx = image_info->tile_idx;
	  *x = image_info->tile_x;
	  *y = image_info->tile_y1;
	  image_info->tile_x += (width + 7) & -8;
	}
    }
  image = image_info->static_image[idx * image_info->n_images / N_REBUNNYIONS];
  *x += BDK_SCRATCH_IMAGE_WIDTH * (idx % (N_REBUNNYIONS / image_info->n_images));
#ifdef VERBOSE
  g_print ("index %d, x %d, y %d (%d x %d)\n", idx, *x, *y, width, height);
  sincelast++;
#endif
  return image;
}

BdkImage*
bdk_image_new (BdkImageType  type,
	       BdkVisual    *visual,
	       gint          width,
	       gint          height)
{
  return _bdk_image_new_for_depth (bdk_visual_get_screen (visual), type,
				   visual, width, height, -1);
}

#define __BDK_IMAGE_C__
#include "bdkaliasdef.c"
