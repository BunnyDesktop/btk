/* BdkPixbuf library - Rendering functions
 *
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Author: Federico Mena-Quintero <federico@gimp.org>
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
#include <bdk/bdk.h>
#include <bdk-pixbuf/bdk-pixbuf.h>
#include "bdkpixbuf.h"
#include "bdkscreen.h"
#include "bdkinternals.h"
#include "bdkalias.h"



/**
 * bdk_pixbuf_render_threshold_alpha:
 * @pixbuf: A pixbuf.
 * @bitmap: Bitmap where the bilevel mask will be painted to.
 * @src_x: Source X coordinate.
 * @src_y: source Y coordinate.
 * @dest_x: Destination X coordinate.
 * @dest_y: Destination Y coordinate.
 * @width: Width of rebunnyion to threshold, or -1 to use pixbuf width
 * @height: Height of rebunnyion to threshold, or -1 to use pixbuf height
 * @alpha_threshold: Opacity values below this will be painted as zero; all
 * other values will be painted as one.
 *
 * Takes the opacity values in a rectangular portion of a pixbuf and thresholds
 * them to produce a bi-level alpha mask that can be used as a clipping mask for
 * a drawable.
 *
 **/
void
bdk_pixbuf_render_threshold_alpha (BdkPixbuf *pixbuf,
                                   BdkBitmap *bitmap,
                                   int        src_x,
                                   int        src_y,
                                   int        dest_x,
                                   int        dest_y,
                                   int        width,
                                   int        height,
                                   int        alpha_threshold)
{
  BdkGC *gc;
  BdkColor color;
  int x, y;
  buchar *p;
  int start, start_status;
  int status;

  g_return_if_fail (bdk_pixbuf_get_colorspace (pixbuf) == BDK_COLORSPACE_RGB);
  g_return_if_fail (bdk_pixbuf_get_n_channels (pixbuf) == 3 ||
                        bdk_pixbuf_get_n_channels (pixbuf) == 4);
  g_return_if_fail (bdk_pixbuf_get_bits_per_sample (pixbuf) == 8);

  if (width == -1) 
    width = bdk_pixbuf_get_width (pixbuf);
  if (height == -1)
    height = bdk_pixbuf_get_height (pixbuf);

  g_return_if_fail (bitmap != NULL);
  g_return_if_fail (width >= 0 && height >= 0);
  g_return_if_fail (src_x >= 0 && src_x + width <= bdk_pixbuf_get_width (pixbuf));
  g_return_if_fail (src_y >= 0 && src_y + height <= bdk_pixbuf_get_height (pixbuf));

  g_return_if_fail (alpha_threshold >= 0 && alpha_threshold <= 255);

  if (width == 0 || height == 0)
    return;

  gc = _bdk_drawable_get_scratch_gc (bitmap, FALSE);

  if (!bdk_pixbuf_get_has_alpha (pixbuf))
    {
      color.pixel = (alpha_threshold == 255) ? 0 : 1;
      bdk_gc_set_foreground (gc, &color);
      bdk_draw_rectangle (bitmap, gc, TRUE, dest_x, dest_y, width, height);
      return;
    }

  color.pixel = 0;
  bdk_gc_set_foreground (gc, &color);
  bdk_draw_rectangle (bitmap, gc, TRUE, dest_x, dest_y, width, height);

  color.pixel = 1;
  bdk_gc_set_foreground (gc, &color);

  for (y = 0; y < height; y++)
    {
      p = (bdk_pixbuf_get_pixels (pixbuf) + (y + src_y) * bdk_pixbuf_get_rowstride (pixbuf) + src_x * bdk_pixbuf_get_n_channels (pixbuf)
	   + bdk_pixbuf_get_n_channels (pixbuf) - 1);
	    
      start = 0;
      start_status = *p < alpha_threshold;
	    
      for (x = 0; x < width; x++)
	{
	  status = *p < alpha_threshold;
	  
	  if (status != start_status)
	    {
	      if (!start_status)
		bdk_draw_line (bitmap, gc,
			       start + dest_x, y + dest_y,
			       x - 1 + dest_x, y + dest_y);
	      
	      start = x;
	      start_status = status;
	    }
	  
	  p += bdk_pixbuf_get_n_channels (pixbuf);
	}
      
      if (!start_status)
	bdk_draw_line (bitmap, gc,
		       start + dest_x, y + dest_y,
		       x - 1 + dest_x, y + dest_y);
    }
}



/**
 * bdk_pixbuf_render_to_drawable:
 * @pixbuf: A pixbuf.
 * @drawable: Destination drawable.
 * @gc: GC used for rendering.
 * @src_x: Source X coordinate within pixbuf.
 * @src_y: Source Y coordinate within pixbuf.
 * @dest_x: Destination X coordinate within drawable.
 * @dest_y: Destination Y coordinate within drawable.
 * @width: Width of rebunnyion to render, in pixels, or -1 to use pixbuf width
 * @height: Height of rebunnyion to render, in pixels, or -1 to use pixbuf height
 * @dither: Dithering mode for BdkRGB.
 * @x_dither: X offset for dither.
 * @y_dither: Y offset for dither.
 *
 * Renders a rectangular portion of a pixbuf to a drawable while using the
 * specified GC.  This is done using BdkRGB, so the specified drawable must have
 * the BdkRGB visual and colormap.  Note that this function will ignore the
 * opacity information for images with an alpha channel; the GC must already
 * have the clipping mask set if you want transparent rebunnyions to show through.
 *
 * For an explanation of dither offsets, see the BdkRGB documentation.  In
 * brief, the dither offset is important when re-rendering partial rebunnyions of an
 * image to a rendered version of the full image, or for when the offsets to a
 * base position change, as in scrolling.  The dither matrix has to be shifted
 * for consistent visual results.  If you do not have any of these cases, the
 * dither offsets can be both zero.
 *
 * Deprecated: 2.4: This function is obsolete. Use bdk_draw_pixbuf() instead.
 **/
void
bdk_pixbuf_render_to_drawable (BdkPixbuf   *pixbuf,
			       BdkDrawable *drawable,
			       BdkGC       *gc,
			       int src_x,    int src_y,
			       int dest_x,   int dest_y,
			       int width,    int height,
			       BdkRgbDither dither,
			       int x_dither, int y_dither)
{
  bdk_draw_pixbuf (drawable, gc, pixbuf,
		   src_x, src_y, dest_x, dest_y, width, height,
		   dither, x_dither, y_dither);
}



/**
 * bdk_pixbuf_render_to_drawable_alpha:
 * @pixbuf: A pixbuf.
 * @drawable: Destination drawable.
 * @src_x: Source X coordinate within pixbuf.
 * @src_y: Source Y coordinates within pixbuf.
 * @dest_x: Destination X coordinate within drawable.
 * @dest_y: Destination Y coordinate within drawable.
 * @width: Width of rebunnyion to render, in pixels, or -1 to use pixbuf width.
 * @height: Height of rebunnyion to render, in pixels, or -1 to use pixbuf height.
 * @alpha_mode: Ignored. Present for backwards compatibility.
 * @alpha_threshold: Ignored. Present for backwards compatibility
 * @dither: Dithering mode for BdkRGB.
 * @x_dither: X offset for dither.
 * @y_dither: Y offset for dither.
 *
 * Renders a rectangular portion of a pixbuf to a drawable.  The destination
 * drawable must have a colormap. All windows have a colormap, however, pixmaps
 * only have colormap by default if they were created with a non-%NULL window argument.
 * Otherwise a colormap must be set on them with bdk_drawable_set_colormap.
 *
 * On older X servers, rendering pixbufs with an alpha channel involves round trips
 * to the X server, and may be somewhat slow.
 *
 * Deprecated: 2.4: This function is obsolete. Use bdk_draw_pixbuf() instead.
 **/
void
bdk_pixbuf_render_to_drawable_alpha (BdkPixbuf   *pixbuf,
				     BdkDrawable *drawable,
				     int src_x,    int src_y,
				     int dest_x,   int dest_y,
				     int width,    int height,
				     BdkPixbufAlphaMode alpha_mode,
				     int                alpha_threshold,
				     BdkRgbDither       dither,
				     int x_dither, int y_dither)
{
  bdk_draw_pixbuf (drawable, NULL, pixbuf,
		   src_x, src_y, dest_x, dest_y, width, height,
		   dither, x_dither, y_dither);
}

/**
 * bdk_pixbuf_render_pixmap_and_mask:
 * @pixbuf: A pixbuf.
 * @pixmap_return: Location to store a pointer to the created pixmap,
 *   or %NULL if the pixmap is not needed.
 * @mask_return: Location to store a pointer to the created mask,
 *   or %NULL if the mask is not needed.
 * @alpha_threshold: Threshold value for opacity values.
 *
 * Creates a pixmap and a mask bitmap which are returned in the @pixmap_return
 * and @mask_return arguments, respectively, and renders a pixbuf and its
 * corresponding thresholded alpha mask to them.  This is merely a convenience
 * function; applications that need to render pixbufs with dither offsets or to
 * given drawables should use bdk_draw_pixbuf() and bdk_pixbuf_render_threshold_alpha().
 *
 * The pixmap that is created is created for the colormap returned
 * by bdk_rgb_get_colormap(). You normally will want to instead use
 * the actual colormap for a widget, and use
 * bdk_pixbuf_render_pixmap_and_mask_for_colormap().
 *
 * If the pixbuf does not have an alpha channel, then *@mask_return will be set
 * to %NULL.
 **/
void
bdk_pixbuf_render_pixmap_and_mask (BdkPixbuf  *pixbuf,
				   BdkPixmap **pixmap_return,
				   BdkBitmap **mask_return,
				   int         alpha_threshold)
{
  bdk_pixbuf_render_pixmap_and_mask_for_colormap (pixbuf,
						  bdk_rgb_get_colormap (),
						  pixmap_return, mask_return,
						  alpha_threshold);
}

/**
 * bdk_pixbuf_render_pixmap_and_mask_for_colormap:
 * @pixbuf: A pixbuf.
 * @colormap: A #BdkColormap
 * @pixmap_return: Location to store a pointer to the created pixmap,
 *   or %NULL if the pixmap is not needed.
 * @mask_return: Location to store a pointer to the created mask,
 *   or %NULL if the mask is not needed.
 * @alpha_threshold: Threshold value for opacity values.
 *
 * Creates a pixmap and a mask bitmap which are returned in the @pixmap_return
 * and @mask_return arguments, respectively, and renders a pixbuf and its
 * corresponding tresholded alpha mask to them.  This is merely a convenience
 * function; applications that need to render pixbufs with dither offsets or to
 * given drawables should use bdk_draw_pixbuf(), and bdk_pixbuf_render_threshold_alpha().
 *
 * The pixmap that is created uses the #BdkColormap specified by @colormap.
 * This colormap must match the colormap of the window where the pixmap
 * will eventually be used or an error will result.
 *
 * If the pixbuf does not have an alpha channel, then *@mask_return will be set
 * to %NULL.
 **/
void
bdk_pixbuf_render_pixmap_and_mask_for_colormap (BdkPixbuf   *pixbuf,
						BdkColormap *colormap,
						BdkPixmap  **pixmap_return,
						BdkBitmap  **mask_return,
						int          alpha_threshold)
{
  BdkScreen *screen;

  g_return_if_fail (BDK_IS_PIXBUF (pixbuf));
  g_return_if_fail (BDK_IS_COLORMAP (colormap));

  screen = bdk_colormap_get_screen (colormap);
  
  if (pixmap_return)
    {
      BdkGC *gc;
      *pixmap_return = bdk_pixmap_new (bdk_screen_get_root_window (screen),
				       bdk_pixbuf_get_width (pixbuf), bdk_pixbuf_get_height (pixbuf),
				       bdk_colormap_get_visual (colormap)->depth);

      bdk_drawable_set_colormap (BDK_DRAWABLE (*pixmap_return), colormap);
      gc = _bdk_drawable_get_scratch_gc (*pixmap_return, FALSE);

      /* If the pixbuf has an alpha channel, using bdk_pixbuf_draw would give
       * random pixel values in the area that are within the mask, but semi-
       * transparent. So we treat the pixbuf like a pixbuf without alpha channel;
       * see bug #487865.
       */
      if (bdk_pixbuf_get_has_alpha (pixbuf))
        bdk_draw_rgb_32_image (*pixmap_return, gc,
                               0, 0,
                               bdk_pixbuf_get_width (pixbuf), bdk_pixbuf_get_height (pixbuf),
                               BDK_RGB_DITHER_NORMAL,
                               bdk_pixbuf_get_pixels (pixbuf), bdk_pixbuf_get_rowstride (pixbuf));
      else
        bdk_draw_pixbuf (*pixmap_return, gc, pixbuf, 
                         0, 0, 0, 0,
                         bdk_pixbuf_get_width (pixbuf), bdk_pixbuf_get_height (pixbuf),
                         BDK_RGB_DITHER_NORMAL,
                         0, 0);
    }
  
  if (mask_return)
    {
      if (bdk_pixbuf_get_has_alpha (pixbuf))
	{
	  *mask_return = bdk_pixmap_new (bdk_screen_get_root_window (screen),
					 bdk_pixbuf_get_width (pixbuf), bdk_pixbuf_get_height (pixbuf), 1);

	  bdk_pixbuf_render_threshold_alpha (pixbuf, *mask_return,
					     0, 0, 0, 0,
					     bdk_pixbuf_get_width (pixbuf), bdk_pixbuf_get_height (pixbuf),
					     alpha_threshold);
	}
      else
	*mask_return = NULL;
    }
}

#define __BDK_PIXBUF_RENDER_C__
#include "bdkaliasdef.c"

