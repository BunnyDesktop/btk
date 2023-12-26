/* bdkdrawable-quartz.c
 *
 * Copyright (C) 2005-2007 Imendio AB
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
#include <sys/time.h>
#include <bairo-quartz.h>
#include "bdkprivate-quartz.h"

static gpointer parent_class;

static bairo_user_data_key_t bdk_quartz_bairo_key;

typedef struct {
  BdkDrawable  *drawable;
  CGContextRef  cg_context;
} BdkQuartzBairoSurfaceData;

void
_bdk_windowing_set_bairo_surface_size (bairo_surface_t *surface,
				       int              width,
				       int              height)
{
  /* This is not supported with quartz surfaces. */
}

static void
bdk_quartz_bairo_surface_destroy (void *data)
{
  BdkQuartzBairoSurfaceData *surface_data = data;
  BdkDrawableImplQuartz *impl = BDK_DRAWABLE_IMPL_QUARTZ (surface_data->drawable);

  impl->bairo_surface = NULL;

  bdk_quartz_drawable_release_context (surface_data->drawable,
                                       surface_data->cg_context);

  g_free (surface_data);
}

bairo_surface_t *
_bdk_windowing_create_bairo_surface (BdkDrawable *drawable,
				     int          width,
				     int          height)
{
  CGContextRef cg_context;
  BdkQuartzBairoSurfaceData *surface_data;
  bairo_surface_t *surface;

  cg_context = bdk_quartz_drawable_get_context (drawable, TRUE);

  surface_data = g_new (BdkQuartzBairoSurfaceData, 1);
  surface_data->drawable = drawable;
  surface_data->cg_context = cg_context;

  if (cg_context)
    surface = bairo_quartz_surface_create_for_cg_context (cg_context,
                                                          width, height);
  else
    surface = bairo_quartz_surface_create(BAIRO_FORMAT_ARGB32, width, height);

  bairo_surface_set_user_data (surface, &bdk_quartz_bairo_key,
                               surface_data,
                               bdk_quartz_bairo_surface_destroy);

  return surface;
}

static bairo_surface_t *
bdk_quartz_ref_bairo_surface (BdkDrawable *drawable)
{
  BdkDrawableImplQuartz *impl = BDK_DRAWABLE_IMPL_QUARTZ (drawable);

  if (BDK_IS_WINDOW_IMPL_QUARTZ (drawable) &&
      BDK_WINDOW_DESTROYED (impl->wrapper))
    return NULL;

  if (!impl->bairo_surface)
    {
      int width, height;

      bdk_drawable_get_size (drawable, &width, &height);
      impl->bairo_surface = _bdk_windowing_create_bairo_surface (drawable,
                                                                 width, height);
    }
  else
    bairo_surface_reference (impl->bairo_surface);

  return impl->bairo_surface;
}

static void
bdk_quartz_set_colormap (BdkDrawable *drawable,
			 BdkColormap *colormap)
{
  BdkDrawableImplQuartz *impl = BDK_DRAWABLE_IMPL_QUARTZ (drawable);

  if (impl->colormap == colormap)
    return;
  
  if (impl->colormap)
    g_object_unref (impl->colormap);
  impl->colormap = colormap;
  if (impl->colormap)
    g_object_ref (impl->colormap);
}

static BdkColormap*
bdk_quartz_get_colormap (BdkDrawable *drawable)
{
  return BDK_DRAWABLE_IMPL_QUARTZ (drawable)->colormap;
}

static BdkScreen*
bdk_quartz_get_screen (BdkDrawable *drawable)
{
  return _bdk_screen;
}

static BdkVisual*
bdk_quartz_get_visual (BdkDrawable *drawable)
{
  return bdk_drawable_get_visual (BDK_DRAWABLE_IMPL_QUARTZ (drawable)->wrapper);
}

static int
bdk_quartz_get_depth (BdkDrawable *drawable)
{
  /* This is a bit bogus but I'm not sure the other way is better */

  return bdk_drawable_get_depth (BDK_DRAWABLE_IMPL_QUARTZ (drawable)->wrapper);
}

static void
bdk_quartz_draw_rectangle (BdkDrawable *drawable,
			   BdkGC       *gc,
			   gboolean     filled,
			   gint         x,
			   gint         y,
			   gint         width,
			   gint         height)
{
  CGContextRef context = bdk_quartz_drawable_get_context (drawable, FALSE);

  if (!context)
    return;

  if(!_bdk_quartz_gc_update_cg_context (gc,
					drawable,
					context,
					filled ?
					BDK_QUARTZ_CONTEXT_FILL :
					BDK_QUARTZ_CONTEXT_STROKE))
    {
      bdk_quartz_drawable_release_context (drawable, context);
      return;
    }
  if (filled)
    {
      CGRect rect = CGRectMake (x, y, width, height);

      CGContextFillRect (context, rect);
    }
  else
    {
      CGRect rect = CGRectMake (x + 0.5, y + 0.5, width, height);

      CGContextStrokeRect (context, rect);
    }

  bdk_quartz_drawable_release_context (drawable, context);
}

static void
bdk_quartz_draw_arc (BdkDrawable *drawable,
		     BdkGC       *gc,
		     gboolean     filled,
		     gint         x,
		     gint         y,
		     gint         width,
		     gint         height,
		     gint         angle1,
		     gint         angle2)
{
  CGContextRef context = bdk_quartz_drawable_get_context (drawable, FALSE);
  float start_angle, end_angle;
  gboolean clockwise = FALSE;

  if (!context)
    return;

  if (!_bdk_quartz_gc_update_cg_context (gc, drawable, context,
					 filled ?
					 BDK_QUARTZ_CONTEXT_FILL :
					 BDK_QUARTZ_CONTEXT_STROKE))
    {
      bdk_quartz_drawable_release_context (drawable, context);
      return;
    }
  start_angle = angle1 * 2.0 * G_PI / 360.0 / 64.0;
  end_angle = start_angle + angle2 * 2.0 * G_PI / 360.0 / 64.0;

  /*  angle2 is relative to angle1 and can be negative, which switches
   *  the drawing direction
   */
  if (angle2 < 0)
    clockwise = TRUE;

  /*  below, flip the coordinate system back to its original y-diretion
   *  so the angles passed to CGContextAddArc() are interpreted as
   *  expected
   *
   *  FIXME: the implementation below works only for perfect circles
   *  (width == height). Any other aspect ratio either scales the
   *  line width unevenly or scales away the path entirely for very
   *  small line widths (esp. for line_width == 0, which is a hair
   *  line on X11 but must be approximated with the thinnest possible
   *  line on quartz).
   */

  if (filled)
    {
      CGContextTranslateCTM (context,
                             x + width / 2.0,
                             y + height / 2.0);
      CGContextScaleCTM (context, 1.0, - (double)height / (double)width);

      CGContextMoveToPoint (context, 0, 0);
      CGContextAddArc (context, 0, 0, width / 2.0,
		       start_angle, end_angle,
		       clockwise);
      CGContextClosePath (context);
      CGContextFillPath (context);
    }
  else
    {
      CGContextTranslateCTM (context,
                             x + width / 2.0 + 0.5,
                             y + height / 2.0 + 0.5);
      CGContextScaleCTM (context, 1.0, - (double)height / (double)width);

      CGContextAddArc (context, 0, 0, width / 2.0,
		       start_angle, end_angle,
		       clockwise);
      CGContextStrokePath (context);
    }

  bdk_quartz_drawable_release_context (drawable, context);
}

static void
bdk_quartz_draw_polygon (BdkDrawable *drawable,
			 BdkGC       *gc,
			 gboolean     filled,
			 BdkPoint    *points,
			 gint         npoints)
{
  CGContextRef context = bdk_quartz_drawable_get_context (drawable, FALSE);
  int i;

  if (!context)
    return;

  if (!_bdk_quartz_gc_update_cg_context (gc, drawable, context,
					 filled ?
					 BDK_QUARTZ_CONTEXT_FILL :
					 BDK_QUARTZ_CONTEXT_STROKE))
    {
      bdk_quartz_drawable_release_context (drawable, context);
      return;
    }
  if (filled)
    {
      CGContextMoveToPoint (context, points[0].x, points[0].y);
      for (i = 1; i < npoints; i++)
	CGContextAddLineToPoint (context, points[i].x, points[i].y);

      CGContextClosePath (context);
      CGContextFillPath (context);
    }
  else
    {
      CGContextMoveToPoint (context, points[0].x + 0.5, points[0].y + 0.5);
      for (i = 1; i < npoints; i++)
	CGContextAddLineToPoint (context, points[i].x + 0.5, points[i].y + 0.5);

      CGContextClosePath (context);
      CGContextStrokePath (context);
    }

  bdk_quartz_drawable_release_context (drawable, context);
}

static void
bdk_quartz_draw_text (BdkDrawable *drawable,
		      BdkFont     *font,
		      BdkGC       *gc,
		      gint         x,
		      gint         y,
		      const gchar *text,
		      gint         text_length)
{
  /* FIXME: Implement */
}

static void
bdk_quartz_draw_text_wc (BdkDrawable    *drawable,
			 BdkFont	*font,
			 BdkGC	        *gc,
			 gint	         x,
			 gint	         y,
			 const BdkWChar *text,
			 gint	         text_length)
{
  /* FIXME: Implement */
}

static void
bdk_quartz_draw_drawable (BdkDrawable *drawable,
			  BdkGC       *gc,
			  BdkPixmap   *src,
			  gint         xsrc,
			  gint         ysrc,
			  gint         xdest,
			  gint         ydest,
			  gint         width,
			  gint         height,
			  BdkDrawable *original_src)
{
  int src_depth = bdk_drawable_get_depth (src);
  int dest_depth = bdk_drawable_get_depth (drawable);
  BdkDrawableImplQuartz *src_impl;

  if (BDK_IS_WINDOW_IMPL_QUARTZ (src))
    {
      BdkWindowImplQuartz *window_impl;

      window_impl = BDK_WINDOW_IMPL_QUARTZ (src);

      /* We do support moving areas on the same drawable, if it can be done
       * by using a scroll. FIXME: We need to check that the params support
       * this hack, and make sure it's done properly with any offsets etc?
       */
      if (drawable == (BdkDrawable *)window_impl)
        {
          NSRect rect = NSMakeRect (xsrc, ysrc, width, height);
          NSSize offset = NSMakeSize (xdest - xsrc, ydest - ysrc);
          BdkRectangle tmp_rect;
          BdkRebunnyion *orig_rebunnyion, *offset_rebunnyion, *need_display_rebunnyion;
          BdkWindow *window = BDK_DRAWABLE_IMPL_QUARTZ (drawable)->wrapper;

          /* Origin rebunnyion */
          tmp_rect.x = xsrc;
          tmp_rect.y = ysrc;
          tmp_rect.width = width;
          tmp_rect.height = height;
          orig_rebunnyion = bdk_rebunnyion_rectangle (&tmp_rect);

          /* Destination rebunnyion (or the offset rebunnyion) */
          offset_rebunnyion = bdk_rebunnyion_copy (orig_rebunnyion);
          bdk_rebunnyion_offset (offset_rebunnyion, offset.width, offset.height);

          need_display_rebunnyion = bdk_rebunnyion_copy (orig_rebunnyion);

          if (window_impl->in_paint_rect_count == 0)
            {
              BdkRebunnyion *bottom_border_rebunnyion;

              /* If we are not in drawRect:, we can use scrollRect:.
               * We apply scrollRect on the rectangle to be moved and
               * subtract this area from the rectangle that needs display.
               *
               * Note: any area in this moved rebunnyion that already needed
               * display will be handled by BDK (queue translation).
               *
               * Queuing the redraw below is important, otherwise the
               * results from scrollRect will not take effect!
               */
              [window_impl->view scrollRect:rect by:offset];

              bdk_rebunnyion_subtract (need_display_rebunnyion, offset_rebunnyion);

              /* Here we take special care with the bottom window border,
               * which extents 4 pixels and typically draws rounded corners.
               */
              tmp_rect.x = 0;
              tmp_rect.y = bdk_window_get_height (window) - 4;
              tmp_rect.width = bdk_window_get_width (window);
              tmp_rect.height = 4;

              if (bdk_rebunnyion_rect_in (offset_rebunnyion, &tmp_rect) !=
                  BDK_OVERLAP_RECTANGLE_OUT)
                {
                  /* We are copying pixels to the bottom border, we need
                   * to submit this area for redisplay to get the rounded
                   * corners drawn.
                   */
                  bdk_rebunnyion_union_with_rect (need_display_rebunnyion,
                                              &tmp_rect);
                }

              /* Compute whether the bottom border is moved elsewhere.
               * Because this part will have rounded corners, we have
               * to fill the contents of where the rounded corners used
               * to be. We post this area for redisplay.
               */
              bottom_border_rebunnyion = bdk_rebunnyion_rectangle (&tmp_rect);
              bdk_rebunnyion_intersect (bottom_border_rebunnyion,
                                    orig_rebunnyion);

              bdk_rebunnyion_offset (bottom_border_rebunnyion,
                                 offset.width, offset.height);

              bdk_rebunnyion_union (need_display_rebunnyion, bottom_border_rebunnyion);

              bdk_rebunnyion_destroy (bottom_border_rebunnyion);
            }
          else
            {
              /* If we cannot handle things with a scroll, we must redisplay
               * the union of the source area and the destination area.
               */
              bdk_rebunnyion_union (need_display_rebunnyion, offset_rebunnyion);
            }

          _bdk_quartz_window_set_needs_display_in_rebunnyion (window,
                                                          need_display_rebunnyion);

          bdk_rebunnyion_destroy (orig_rebunnyion);
          bdk_rebunnyion_destroy (offset_rebunnyion);
          bdk_rebunnyion_destroy (need_display_rebunnyion);
        }
      else
        g_warning ("Drawing with window source != dest is not supported");

      return;
    }
  else if (BDK_IS_DRAWABLE_IMPL_QUARTZ (src))
    src_impl = BDK_DRAWABLE_IMPL_QUARTZ (src);
  else if (BDK_IS_PIXMAP (src))
    src_impl = BDK_DRAWABLE_IMPL_QUARTZ (BDK_PIXMAP_OBJECT (src)->impl);
  else
    {
      g_warning ("Unsupported source %s", B_OBJECT_TYPE_NAME (src));
      return;
    }

  /* Handle drawable and pixmap sources. */
  if (src_depth == 1)
    {
      /* FIXME: src depth 1 is not supported yet */
      g_warning ("Source with depth 1 unsupported");
    }
  else if (dest_depth != 0 && src_depth == dest_depth)
    {
      BdkPixmapImplQuartz *pixmap_impl = BDK_PIXMAP_IMPL_QUARTZ (src_impl);
      CGContextRef context = bdk_quartz_drawable_get_context (drawable, FALSE);
      CGImageRef image;

      if (!context)
        return;

      if (!_bdk_quartz_gc_update_cg_context (gc, drawable, context,
					     BDK_QUARTZ_CONTEXT_STROKE))
	{
	  bdk_quartz_drawable_release_context (drawable, context);
	  return;
	}
      CGContextClipToRect (context, CGRectMake (xdest, ydest, width, height));
      CGContextTranslateCTM (context, xdest - xsrc, ydest - ysrc +
                             pixmap_impl->height);
      CGContextScaleCTM (context, 1.0, -1.0);

      image = _bdk_pixmap_get_cgimage (src);
      CGContextDrawImage (context,
                          CGRectMake (0, 0, pixmap_impl->width, pixmap_impl->height),
                          image);
      CGImageRelease (image);

      bdk_quartz_drawable_release_context (drawable, context);
    }
  else
    g_warning ("Attempt to draw a drawable with depth %d to a drawable with depth %d",
               src_depth, dest_depth);
}

static void
bdk_quartz_draw_points (BdkDrawable *drawable,
			BdkGC       *gc,
			BdkPoint    *points,
			gint         npoints)
{
  CGContextRef context = bdk_quartz_drawable_get_context (drawable, FALSE);
  int i;

  if (!context)
    return;

  if (!_bdk_quartz_gc_update_cg_context (gc, drawable, context,
					 BDK_QUARTZ_CONTEXT_STROKE |
					 BDK_QUARTZ_CONTEXT_FILL))
    {
      bdk_quartz_drawable_release_context (drawable, context);
      return;
    }
  /* Just draw 1x1 rectangles */
  for (i = 0; i < npoints; i++) 
    {
      CGRect rect = CGRectMake (points[i].x, points[i].y, 1, 1);
      CGContextFillRect (context, rect);
    }

  bdk_quartz_drawable_release_context (drawable, context);
}

static inline void
bdk_quartz_fix_cap_not_last_line (BdkGCQuartz *private,
				  gint         x1,
				  gint         y1,
				  gint         x2,
				  gint         y2,
				  gint        *xfix,
				  gint        *yfix)
{
  *xfix = 0;
  *yfix = 0;

  if (private->cap_style == BDK_CAP_NOT_LAST && private->line_width == 0)
    {
      /* fix only vertical and horizontal lines for now */

      if (y1 == y2 && x1 != x2)
	{
	  *xfix = (x1 < x2) ? -1 : 1;
	}
      else if (x1 == x2 && y1 != y2)
	{
	  *yfix = (y1 < y2) ? -1 : 1;
	}
    }
}

static void
bdk_quartz_draw_segments (BdkDrawable    *drawable,
			  BdkGC          *gc,
			  BdkSegment     *segs,
			  gint            nsegs)
{
  CGContextRef context = bdk_quartz_drawable_get_context (drawable, FALSE);
  BdkGCQuartz *private;
  int i;

  if (!context)
    return;

  private = BDK_GC_QUARTZ (gc);

  if (!_bdk_quartz_gc_update_cg_context (gc, drawable, context,
					 BDK_QUARTZ_CONTEXT_STROKE))
    {
      bdk_quartz_drawable_release_context (drawable, context);
      return;
    }
  for (i = 0; i < nsegs; i++)
    {
      gint xfix, yfix;

      bdk_quartz_fix_cap_not_last_line (private,
					segs[i].x1, segs[i].y1,
					segs[i].x2, segs[i].y2,
					&xfix, &yfix);

      CGContextMoveToPoint (context, segs[i].x1 + 0.5, segs[i].y1 + 0.5);
      CGContextAddLineToPoint (context, segs[i].x2 + 0.5 + xfix, segs[i].y2 + 0.5 + yfix);
    }
  
  CGContextStrokePath (context);

  bdk_quartz_drawable_release_context (drawable, context);
}

static void
bdk_quartz_draw_lines (BdkDrawable *drawable,
		       BdkGC       *gc,
		       BdkPoint    *points,
		       gint         npoints)
{
  CGContextRef context = bdk_quartz_drawable_get_context (drawable, FALSE);
  BdkGCQuartz *private;
  gint xfix, yfix;
  gint i;

  if (!context)
    return;

  private = BDK_GC_QUARTZ (gc);

  if (!_bdk_quartz_gc_update_cg_context (gc, drawable, context,
					 BDK_QUARTZ_CONTEXT_STROKE))
    {
      bdk_quartz_drawable_release_context (drawable, context);
      return;
    }
  CGContextMoveToPoint (context, points[0].x + 0.5, points[0].y + 0.5);

  for (i = 1; i < npoints - 1; i++)
    CGContextAddLineToPoint (context, points[i].x + 0.5, points[i].y + 0.5);

  bdk_quartz_fix_cap_not_last_line (private,
				    points[npoints - 2].x, points[npoints - 2].y,
				    points[npoints - 1].x, points[npoints - 1].y,
				    &xfix, &yfix);

  CGContextAddLineToPoint (context,
			   points[npoints - 1].x + 0.5 + xfix,
			   points[npoints - 1].y + 0.5 + yfix);

  CGContextStrokePath (context);

  bdk_quartz_drawable_release_context (drawable, context);
}

static void
bdk_quartz_draw_pixbuf (BdkDrawable     *drawable,
			BdkGC           *gc,
			BdkPixbuf       *pixbuf,
			gint             src_x,
			gint             src_y,
			gint             dest_x,
			gint             dest_y,
			gint             width,
			gint             height,
			BdkRgbDither     dither,
			gint             x_dither,
			gint             y_dither)
{
  CGContextRef context = bdk_quartz_drawable_get_context (drawable, FALSE);
  CGColorSpaceRef colorspace;
  CGDataProviderRef data_provider;
  CGImageRef image;
  void *data;
  int rowstride, pixbuf_width, pixbuf_height;
  gboolean has_alpha;

  if (!context)
    return;

  pixbuf_width = bdk_pixbuf_get_width (pixbuf);
  pixbuf_height = bdk_pixbuf_get_height (pixbuf);
  rowstride = bdk_pixbuf_get_rowstride (pixbuf);
  has_alpha = bdk_pixbuf_get_has_alpha (pixbuf);

  data = bdk_pixbuf_get_pixels (pixbuf);

  colorspace = CGColorSpaceCreateDeviceRGB ();
  data_provider = CGDataProviderCreateWithData (NULL, data, pixbuf_height * rowstride, NULL);

  image = CGImageCreate (pixbuf_width, pixbuf_height, 8,
			 has_alpha ? 32 : 24, rowstride, 
			 colorspace, 
			 has_alpha ? kCGImageAlphaLast : 0,
			 data_provider, NULL, FALSE, 
			 kCGRenderingIntentDefault);

  CGDataProviderRelease (data_provider);
  CGColorSpaceRelease (colorspace);

  if (!_bdk_quartz_gc_update_cg_context (gc, drawable, context,
					 BDK_QUARTZ_CONTEXT_STROKE))
    {
      bdk_quartz_drawable_release_context (drawable, context);
      return;
    }
  CGContextClipToRect (context, CGRectMake (dest_x, dest_y, width, height));
  CGContextTranslateCTM (context, dest_x - src_x, dest_y - src_y + pixbuf_height);
  CGContextScaleCTM (context, 1, -1);

  CGContextDrawImage (context, CGRectMake (0, 0, pixbuf_width, pixbuf_height), image);
  CGImageRelease (image);

  bdk_quartz_drawable_release_context (drawable, context);
}

static void
bdk_quartz_draw_image (BdkDrawable     *drawable,
		       BdkGC           *gc,
		       BdkImage        *image,
		       gint             xsrc,
		       gint             ysrc,
		       gint             xdest,
		       gint             ydest,
		       gint             width,
		       gint             height)
{
  CGContextRef context = bdk_quartz_drawable_get_context (drawable, FALSE);
  CGColorSpaceRef colorspace;
  CGDataProviderRef data_provider;
  CGImageRef cgimage;

  if (!context)
    return;

  colorspace = CGColorSpaceCreateDeviceRGB ();
  data_provider = CGDataProviderCreateWithData (NULL, image->mem, image->height * image->bpl, NULL);

  /* FIXME: Make sure that this function draws 32-bit images correctly,
  * also check endianness wrt kCGImageAlphaNoneSkipFirst */
  cgimage = CGImageCreate (image->width, image->height, 8,
			   32, image->bpl,
			   colorspace,
			   kCGImageAlphaNoneSkipFirst, 
			   data_provider, NULL, FALSE, kCGRenderingIntentDefault);

  CGDataProviderRelease (data_provider);
  CGColorSpaceRelease (colorspace);

  if (!_bdk_quartz_gc_update_cg_context (gc, drawable, context,
					 BDK_QUARTZ_CONTEXT_STROKE))
    {
      bdk_quartz_drawable_release_context (drawable, context);
      return;
    }

  CGContextClipToRect (context, CGRectMake (xdest, ydest, width, height));
  CGContextTranslateCTM (context, xdest - xsrc, ydest - ysrc + image->height);
  CGContextScaleCTM (context, 1, -1);

  CGContextDrawImage (context, CGRectMake (0, 0, image->width, image->height), cgimage);
  CGImageRelease (cgimage);

  bdk_quartz_drawable_release_context (drawable, context);
}

static void
bdk_drawable_impl_quartz_finalize (BObject *object)
{
  BdkDrawableImplQuartz *impl = BDK_DRAWABLE_IMPL_QUARTZ (object);

  if (impl->colormap)
    g_object_unref (impl->colormap);

  B_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bdk_drawable_impl_quartz_class_init (BdkDrawableImplQuartzClass *klass)
{
  BObjectClass *object_class = B_OBJECT_CLASS (klass);
  BdkDrawableClass *drawable_class = BDK_DRAWABLE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = bdk_drawable_impl_quartz_finalize;

  drawable_class->create_gc = _bdk_quartz_gc_new;
  drawable_class->draw_rectangle = bdk_quartz_draw_rectangle;
  drawable_class->draw_arc = bdk_quartz_draw_arc;
  drawable_class->draw_polygon = bdk_quartz_draw_polygon;
  drawable_class->draw_text = bdk_quartz_draw_text;
  drawable_class->draw_text_wc = bdk_quartz_draw_text_wc;
  drawable_class->draw_drawable_with_src = bdk_quartz_draw_drawable;
  drawable_class->draw_points = bdk_quartz_draw_points;
  drawable_class->draw_segments = bdk_quartz_draw_segments;
  drawable_class->draw_lines = bdk_quartz_draw_lines;
  drawable_class->draw_image = bdk_quartz_draw_image;
  drawable_class->draw_pixbuf = bdk_quartz_draw_pixbuf;

  drawable_class->ref_bairo_surface = bdk_quartz_ref_bairo_surface;

  drawable_class->set_colormap = bdk_quartz_set_colormap;
  drawable_class->get_colormap = bdk_quartz_get_colormap;

  drawable_class->get_depth = bdk_quartz_get_depth;
  drawable_class->get_screen = bdk_quartz_get_screen;
  drawable_class->get_visual = bdk_quartz_get_visual;

  drawable_class->_copy_to_image = _bdk_quartz_image_copy_to_image;
}

GType
bdk_drawable_impl_quartz_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      const GTypeInfo object_info =
      {
        sizeof (BdkDrawableImplQuartzClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) bdk_drawable_impl_quartz_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (BdkDrawableImplQuartz),
        0,              /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };
      
      object_type = g_type_register_static (BDK_TYPE_DRAWABLE,
                                            "BdkDrawableImplQuartz",
                                            &object_info, 0);
    }
  
  return object_type;
}

CGContextRef
bdk_quartz_drawable_get_context (BdkDrawable *drawable,
				 gboolean     antialias)
{
  if (!BDK_DRAWABLE_IMPL_QUARTZ_GET_CLASS (drawable)->get_context)
    {
      g_warning ("%s doesn't implement BdkDrawableImplQuartzClass::get_context()",
                 B_OBJECT_TYPE_NAME (drawable));
      return NULL;
    }

  return BDK_DRAWABLE_IMPL_QUARTZ_GET_CLASS (drawable)->get_context (drawable, antialias);
}

/* Help preventing "beam sync penalty" where CG makes all graphics code
 * block until the next vsync if we try to flush (including call display on
 * a view) too often. We do this by limiting the manual flushing done
 * outside of expose calls to less than some frequency when measured over
 * the last 4 flushes. This is a bit arbitray, but seems to make it possible
 * for some quick manual flushes (such as btkruler or gimp's marching ants)
 * without hitting the max flush frequency.
 *
 * If drawable NULL, no flushing is done, only registering that a flush was
 * done externally.
 */
void
_bdk_quartz_drawable_flush (BdkDrawable *drawable)
{
  static struct timeval prev_tv;
  static gint intervals[4];
  static gint index;
  struct timeval tv;
  gint ms;

  gettimeofday (&tv, NULL);
  ms = (tv.tv_sec - prev_tv.tv_sec) * 1000 + (tv.tv_usec - prev_tv.tv_usec) / 1000;
  intervals[index++ % 4] = ms;

  if (drawable)
    {
      ms = intervals[0] + intervals[1] + intervals[2] + intervals[3];

      /* ~25Hz on average. */
      if (ms > 4*40)
        {
          if (BDK_IS_WINDOW_IMPL_QUARTZ (drawable))
            {
              BdkWindowImplQuartz *window_impl = BDK_WINDOW_IMPL_QUARTZ (drawable);

              [window_impl->toplevel flushWindow];
            }

          prev_tv = tv;
        }
    }
  else
    prev_tv = tv;
}

void
bdk_quartz_drawable_release_context (BdkDrawable  *drawable, 
				     CGContextRef  cg_context)
{
  if (!cg_context)
    return;
  
  if (BDK_IS_WINDOW_IMPL_QUARTZ (drawable))
    {
      BdkWindowImplQuartz *window_impl = BDK_WINDOW_IMPL_QUARTZ (drawable);

      CGContextRestoreGState (cg_context);
      CGContextSetAllowsAntialiasing (cg_context, TRUE);

      /* See comment in bdk_quartz_drawable_get_context(). */
      if (window_impl->in_paint_rect_count == 0)
        {
          _bdk_quartz_drawable_flush (drawable);
          [window_impl->view unlockFocus];
        }
    }
  else if (BDK_IS_PIXMAP_IMPL_QUARTZ (drawable))
    CGContextRelease (cg_context);
}

void
_bdk_quartz_drawable_finish (BdkDrawable *drawable)
{
  BdkDrawableImplQuartz *impl = BDK_DRAWABLE_IMPL_QUARTZ (drawable);

  if (impl->bairo_surface)
    {
      bairo_surface_finish (impl->bairo_surface);
      bairo_surface_set_user_data (impl->bairo_surface, &bdk_quartz_bairo_key,
				   NULL, NULL);
      impl->bairo_surface = NULL;
    }
}  
