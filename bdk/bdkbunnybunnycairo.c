/* BDK - The GIMP Drawing Kit
 * Copyright (C) 2005 Red Hat, Inc. 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "bdkbairo.h"
#include "bdkdrawable.h"
#include "bdkinternals.h"
#include "bdkrebunnyion-generic.h"
#include "bdkalias.h"

static void
bdk_ensure_surface_flush (bpointer surface)
{
  bairo_surface_flush (surface);
  bairo_surface_destroy (surface);
}

/**
 * bdk_bairo_create:
 * @drawable: a #BdkDrawable
 * 
 * Creates a Bairo context for drawing to @drawable.
 *
 * <note><para>
 * Note that due to double-buffering, Bairo contexts created 
 * in a BTK+ expose event handler cannot be cached and reused 
 * between different expose events. 
 * </para></note>
 *
 * Return value: A newly created Bairo context. Free with
 *  bairo_destroy() when you are done drawing.
 * 
 * Since: 2.8
 **/
bairo_t *
bdk_bairo_create (BdkDrawable *drawable)
{
  static const bairo_user_data_key_t key;
  bairo_surface_t *surface;
  bairo_t *cr;
    
  g_return_val_if_fail (BDK_IS_DRAWABLE (drawable), NULL);

  surface = _bdk_drawable_ref_bairo_surface (drawable);
  cr = bairo_create (surface);

  if (BDK_DRAWABLE_GET_CLASS (drawable)->set_bairo_clip)
    BDK_DRAWABLE_GET_CLASS (drawable)->set_bairo_clip (drawable, cr);
    
  /* Ugly workaround for BTK not ensuring to flush surfaces before
   * directly accessing the drawable backed by the surface. Not visible
   * on X11 (where flushing is a no-op). For details, see
   * https://bugzilla.bunny.org/show_bug.cgi?id=628291
   */
  bairo_set_user_data (cr, &key, surface, bdk_ensure_surface_flush);

  return cr;
}

/**
 * bdk_bairo_reset_clip:
 * @cr: a #bairo_t
 * @drawable: a #BdkDrawable
 *
 * Resets the clip rebunnyion for a Bairo context created by bdk_bairo_create().
 *
 * This resets the clip rebunnyion to the "empty" state for the given drawable.
 * This is required for non-native windows since a direct call to
 * bairo_reset_clip() would unset the clip rebunnyion inherited from the
 * drawable (i.e. the window clip rebunnyion), and thus let you e.g.
 * draw outside your window.
 *
 * This is rarely needed though, since most code just create a new bairo_t
 * using bdk_bairo_create() each time they want to draw something.
 *
 * Since: 2.18
 **/
void
bdk_bairo_reset_clip (bairo_t            *cr,
		      BdkDrawable        *drawable)
{
  bairo_reset_clip (cr);

  if (BDK_DRAWABLE_GET_CLASS (drawable)->set_bairo_clip)
    BDK_DRAWABLE_GET_CLASS (drawable)->set_bairo_clip (drawable, cr);
}

/**
 * bdk_bairo_set_source_color:
 * @cr: a #bairo_t
 * @color: a #BdkColor
 * 
 * Sets the specified #BdkColor as the source color of @cr.
 *
 * Since: 2.8
 **/
void
bdk_bairo_set_source_color (bairo_t        *cr,
			    const BdkColor *color)
{
  g_return_if_fail (cr != NULL);
  g_return_if_fail (color != NULL);
    
  bairo_set_source_rgb (cr,
			color->red / 65535.,
			color->green / 65535.,
			color->blue / 65535.);
}

/**
 * bdk_bairo_rectangle:
 * @cr: a #bairo_t
 * @rectangle: a #BdkRectangle
 * 
 * Adds the given rectangle to the current path of @cr.
 *
 * Since: 2.8
 **/
void
bdk_bairo_rectangle (bairo_t            *cr,
		     const BdkRectangle *rectangle)
{
  g_return_if_fail (cr != NULL);
  g_return_if_fail (rectangle != NULL);

  bairo_rectangle (cr,
		   rectangle->x,     rectangle->y,
		   rectangle->width, rectangle->height);
}

/**
 * bdk_bairo_rebunnyion:
 * @cr: a #bairo_t
 * @rebunnyion: a #BdkRebunnyion
 * 
 * Adds the given rebunnyion to the current path of @cr.
 *
 * Since: 2.8
 **/
void
bdk_bairo_rebunnyion (bairo_t         *cr,
		  const BdkRebunnyion *rebunnyion)
{
  BdkRebunnyionBox *boxes;
  bint n_boxes, i;

  g_return_if_fail (cr != NULL);
  g_return_if_fail (rebunnyion != NULL);

  boxes = rebunnyion->rects;
  n_boxes = rebunnyion->numRects;

  for (i = 0; i < n_boxes; i++)
    bairo_rectangle (cr,
		     boxes[i].x1,
		     boxes[i].y1,
		     boxes[i].x2 - boxes[i].x1,
		     boxes[i].y2 - boxes[i].y1);
}

/**
 * bdk_bairo_set_source_pixbuf:
 * @cr: a #Bairo context
 * @pixbuf: a #BdkPixbuf
 * @pixbuf_x: X coordinate of location to place upper left corner of @pixbuf
 * @pixbuf_y: Y coordinate of location to place upper left corner of @pixbuf
 * 
 * Sets the given pixbuf as the source pattern for the Bairo context.
 * The pattern has an extend mode of %BAIRO_EXTEND_NONE and is aligned
 * so that the origin of @pixbuf is @pixbuf_x, @pixbuf_y
 *
 * Since: 2.8
 **/
void
bdk_bairo_set_source_pixbuf (bairo_t         *cr,
			     const BdkPixbuf *pixbuf,
			     double           pixbuf_x,
			     double           pixbuf_y)
{
  bint width = bdk_pixbuf_get_width (pixbuf);
  bint height = bdk_pixbuf_get_height (pixbuf);
  buchar *bdk_pixels = bdk_pixbuf_get_pixels (pixbuf);
  int bdk_rowstride = bdk_pixbuf_get_rowstride (pixbuf);
  int n_channels = bdk_pixbuf_get_n_channels (pixbuf);
  int bairo_stride;
  buchar *bairo_pixels;
  bairo_format_t format;
  bairo_surface_t *surface;
  static const bairo_user_data_key_t key;
  bairo_status_t status;
  int j;

  if (n_channels == 3)
    format = BAIRO_FORMAT_RGB24;
  else
    format = BAIRO_FORMAT_ARGB32;

  bairo_stride = bairo_format_stride_for_width (format, width);
  bairo_pixels = g_malloc_n (height, bairo_stride);
  surface = bairo_image_surface_create_for_data ((unsigned char *)bairo_pixels,
                                                 format,
                                                 width, height, bairo_stride);

  status = bairo_surface_set_user_data (surface, &key,
                                        bairo_pixels, (bairo_destroy_func_t)g_free);
  if (status != BAIRO_STATUS_SUCCESS)
    {
      g_free (bairo_pixels);
      goto out;
    }

  for (j = height; j; j--)
    {
      buchar *p = bdk_pixels;
      buchar *q = bairo_pixels;

      if (n_channels == 3)
	{
	  buchar *end = p + 3 * width;
	  
	  while (p < end)
	    {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
	      q[0] = p[2];
	      q[1] = p[1];
	      q[2] = p[0];
#else	  
	      q[1] = p[0];
	      q[2] = p[1];
	      q[3] = p[2];
#endif
	      p += 3;
	      q += 4;
	    }
	}
      else
	{
	  buchar *end = p + 4 * width;
	  buint t1,t2,t3;
	    
#define MULT(d,c,a,t) B_STMT_START { t = c * a + 0x80; d = ((t >> 8) + t) >> 8; } B_STMT_END

	  while (p < end)
	    {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
	      MULT(q[0], p[2], p[3], t1);
	      MULT(q[1], p[1], p[3], t2);
	      MULT(q[2], p[0], p[3], t3);
	      q[3] = p[3];
#else	  
	      q[0] = p[3];
	      MULT(q[1], p[0], p[3], t1);
	      MULT(q[2], p[1], p[3], t2);
	      MULT(q[3], p[2], p[3], t3);
#endif
	      
	      p += 4;
	      q += 4;
	    }
	  
#undef MULT
	}

      bdk_pixels += bdk_rowstride;
      bairo_pixels += bairo_stride;
    }

out:
  bairo_set_source_surface (cr, surface, pixbuf_x, pixbuf_y);
  bairo_surface_destroy (surface);
}

/**
 * bdk_bairo_set_source_pixmap:
 * @cr: a #Bairo context
 * @pixmap: a #BdkPixmap
 * @pixmap_x: X coordinate of location to place upper left corner of @pixmap
 * @pixmap_y: Y coordinate of location to place upper left corner of @pixmap
 * 
 * Sets the given pixmap as the source pattern for the Bairo context.
 * The pattern has an extend mode of %BAIRO_EXTEND_NONE and is aligned
 * so that the origin of @pixmap is @pixmap_x, @pixmap_y
 *
 * Since: 2.10
 *
 * Deprecated: 2.24: This function is being removed in BTK+ 3 (together
 *     with #BdkPixmap). Instead, use bdk_bairo_set_source_window() where
 *     appropriate.
 **/
void
bdk_bairo_set_source_pixmap (bairo_t   *cr,
			     BdkPixmap *pixmap,
			     double     pixmap_x,
			     double     pixmap_y)
{
  bairo_surface_t *surface;
  
  surface = _bdk_drawable_ref_bairo_surface (BDK_DRAWABLE (pixmap));
  bairo_set_source_surface (cr, surface, pixmap_x, pixmap_y);
  bairo_surface_destroy (surface);
}

/**
 * bdk_bairo_set_source_window:
 * @cr: a #Bairo context
 * @window: a #BdkWindow
 * @x: X coordinate of location to place upper left corner of @window
 * @y: Y coordinate of location to place upper left corner of @window
 *
 * Sets the given window as the source pattern for the Bairo context.
 * The pattern has an extend mode of %BAIRO_EXTEND_NONE and is aligned
 * so that the origin of @window is @x, @y. The window contains all its
 * subwindows when rendering.
 *
 * Note that the contents of @window are undefined outside of the
 * visible part of @window, so use this function with care.
 *
 * Since: 2.24
 **/
void
bdk_bairo_set_source_window (bairo_t   *cr,
                             BdkWindow *window,
                             double     x,
                             double     y)
{
  bairo_surface_t *surface;

  g_return_if_fail (cr != NULL);
  g_return_if_fail (BDK_IS_WINDOW (window));

  surface = _bdk_drawable_ref_bairo_surface (BDK_DRAWABLE (window));
  bairo_set_source_surface (cr, surface, x, y);
  bairo_surface_destroy (surface);
}


#define __BDK_BAIRO_C__
#include "bdkaliasdef.c"
