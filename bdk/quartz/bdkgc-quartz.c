/* bdkgc-quartz.c
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

#include "bdkgc.h"
#include "bdkprivate-quartz.h"

static bpointer parent_class = NULL;

static void
bdk_quartz_gc_get_values (BdkGC       *gc,
			  BdkGCValues *values)
{
  BdkGCQuartz *private;

  private = BDK_GC_QUARTZ (gc);

  values->foreground.pixel = _bdk_gc_get_fg_pixel (gc);
  values->background.pixel = _bdk_gc_get_bg_pixel (gc);

  values->font = private->font;

  values->function = private->function;

  values->fill = _bdk_gc_get_fill (gc);
  values->tile = _bdk_gc_get_tile (gc);
  values->stipple = _bdk_gc_get_stipple (gc);
  
  /* The X11 backend always returns a NULL clip_mask. */
  values->clip_mask = NULL;

  values->ts_x_origin = gc->ts_x_origin;
  values->ts_y_origin = gc->ts_y_origin;
  values->clip_x_origin = gc->clip_x_origin;
  values->clip_y_origin = gc->clip_y_origin;

  values->graphics_exposures = private->graphics_exposures;

  values->line_width = private->line_width;
  values->line_style = private->line_style;
  values->cap_style = private->cap_style;
  values->join_style = private->join_style;
}


static void
data_provider_release (void *info, const void *data, size_t size)
{
  g_free (info);
}

static CGImageRef
create_clip_mask (BdkPixmap *source_pixmap)
{
  int width, height, bytes_per_row, bits_per_pixel;
  void *data;
  CGImageRef source;
  CGImageRef clip_mask;
  CGContextRef cg_context;
  CGDataProviderRef data_provider;

  /* We need to flip the clip mask here, because this cannot be done during
   * the drawing process when this mask will be used to do clipping.  We
   * quickly create a new CGImage, set up a CGContext, draw the source
   * image while flipping, and done.  If this appears too slow in the
   * future, we would look into doing this by hand on the actual raw
   * data.
   */
  source = _bdk_pixmap_get_cgimage (source_pixmap);

  width = CGImageGetWidth (source);
  height = CGImageGetHeight (source);
  bytes_per_row = CGImageGetBytesPerRow (source);
  bits_per_pixel = CGImageGetBitsPerPixel (source);

  data = g_malloc (height * bytes_per_row);
  data_provider = CGDataProviderCreateWithData (data, data,
                                                height * bytes_per_row,
                                                data_provider_release);

  clip_mask = CGImageCreate (width, height, 8,
                             bits_per_pixel,
                             bytes_per_row,
                             CGImageGetColorSpace (source),
                             CGImageGetAlphaInfo (source),
                             data_provider, NULL, FALSE,
                             kCGRenderingIntentDefault);
  CGDataProviderRelease (data_provider);

  cg_context = CGBitmapContextCreate (data,
                                      width, height,
                                      CGImageGetBitsPerComponent (source),
                                      bytes_per_row,
                                      CGImageGetColorSpace (source),
                                      CGImageGetBitmapInfo (source));

  if (cg_context)
    {
      CGContextTranslateCTM (cg_context, 0, height);
      CGContextScaleCTM (cg_context, 1.0, -1.0);

      CGContextDrawImage (cg_context,
                          CGRectMake (0, 0, width, height), source);

      CGContextRelease (cg_context);
    }

  return clip_mask;
}

static void
bdk_quartz_gc_set_values (BdkGC           *gc,
			  BdkGCValues     *values,
			  BdkGCValuesMask  mask)
{
  BdkGCQuartz *private = BDK_GC_QUARTZ (gc);

  if (mask & BDK_GC_FONT)
    {
      /* FIXME: implement font */
    }

  if (mask & BDK_GC_FUNCTION)
    private->function = values->function;

  if (mask & BDK_GC_SUBWINDOW)
    private->subwindow_mode = values->subwindow_mode;

  if (mask & BDK_GC_EXPOSURES)
    private->graphics_exposures = values->graphics_exposures;

  if (mask & BDK_GC_CLIP_MASK)
    {
      private->have_clip_rebunnyion = FALSE;
      private->have_clip_mask = values->clip_mask != NULL;
      if (private->clip_mask)
	CGImageRelease (private->clip_mask);

      if (values->clip_mask)
        private->clip_mask = create_clip_mask (values->clip_mask);
      else
	private->clip_mask = NULL;
    }

  if (mask & BDK_GC_LINE_WIDTH)
    private->line_width = values->line_width;

  if (mask & BDK_GC_LINE_STYLE)
    private->line_style = values->line_style;

  if (mask & BDK_GC_CAP_STYLE)
    private->cap_style = values->cap_style;

  if (mask & BDK_GC_JOIN_STYLE)
    private->join_style = values->join_style;
}

static void
bdk_quartz_gc_set_dashes (BdkGC *gc,
			  bint   dash_offset,
			  bint8  dash_list[],
			  bint   n)
{
  BdkGCQuartz *private = BDK_GC_QUARTZ (gc);
  bint i;

  private->dash_count = n;
  g_free (private->dash_lengths);
  private->dash_lengths = g_new (CGFloat, n);
  for (i = 0; i < n; i++)
    private->dash_lengths[i] = (CGFloat) dash_list[i];
  private->dash_phase = (CGFloat) dash_offset;
}

static void
bdk_gc_quartz_finalize (BObject *object)
{
  BdkGCQuartz *private = BDK_GC_QUARTZ (object);

  if (private->clip_mask)
    CGImageRelease (private->clip_mask);

  if (private->ts_pattern)
    CGPatternRelease (private->ts_pattern);

  B_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bdk_gc_quartz_class_init (BdkGCQuartzClass *klass)
{
  BObjectClass *object_class = B_OBJECT_CLASS (klass);
  BdkGCClass *gc_class = BDK_GC_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = bdk_gc_quartz_finalize;

  gc_class->get_values = bdk_quartz_gc_get_values;
  gc_class->set_values = bdk_quartz_gc_set_values;
  gc_class->set_dashes = bdk_quartz_gc_set_dashes;
}

static void
bdk_gc_quartz_init (BdkGCQuartz *gc_quartz)
{
  gc_quartz->function = BDK_COPY;
  gc_quartz->subwindow_mode = BDK_CLIP_BY_CHILDREN;
  gc_quartz->graphics_exposures = TRUE;
  gc_quartz->line_width = 0;
  gc_quartz->line_style = BDK_LINE_SOLID;
  gc_quartz->cap_style = BDK_CAP_BUTT;
  gc_quartz->join_style = BDK_JOIN_MITER;
}

GType
_bdk_gc_quartz_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      const GTypeInfo object_info =
      {
        sizeof (BdkGCQuartzClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) bdk_gc_quartz_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (BdkGCQuartz),
        0,              /* n_preallocs */
        (GInstanceInitFunc) bdk_gc_quartz_init,
      };
      
      object_type = g_type_register_static (BDK_TYPE_GC,
                                            "BdkGCQuartz",
                                            &object_info, 0);
    }
  
  return object_type;
}

BdkGC *
_bdk_quartz_gc_new (BdkDrawable      *drawable,
		    BdkGCValues      *values,
		    BdkGCValuesMask   values_mask)
{
  BdkGC *gc;

  gc = g_object_new (BDK_TYPE_GC_QUARTZ, NULL);

  _bdk_gc_init (gc, drawable, values, values_mask);

  bdk_quartz_gc_set_values (gc, values, values_mask);

  return gc;
}

void
_bdk_windowing_gc_set_clip_rebunnyion (BdkGC           *gc,
				   const BdkRebunnyion *rebunnyion,
				   bboolean         reset_origin)
{
  BdkGCQuartz *private = BDK_GC_QUARTZ (gc);

  if ((private->have_clip_rebunnyion && ! rebunnyion) || private->have_clip_mask)
    {
      if (private->clip_mask)
	{
	  CGImageRelease (private->clip_mask);
	  private->clip_mask = NULL;
	}
      private->have_clip_mask = FALSE;
    }

  private->have_clip_rebunnyion = rebunnyion != NULL;

  if (reset_origin)
    {
      gc->clip_x_origin = 0;
      gc->clip_y_origin = 0;
    }
}

void
_bdk_windowing_gc_copy (BdkGC *dst_gc,
			BdkGC *src_gc)
{
  BdkGCQuartz *dst_quartz_gc = BDK_GC_QUARTZ (dst_gc);
  BdkGCQuartz *src_quartz_gc = BDK_GC_QUARTZ (src_gc);

  if (dst_quartz_gc->font)
    bdk_font_unref (dst_quartz_gc->font);
  dst_quartz_gc->font = src_quartz_gc->font;
  if (dst_quartz_gc->font)
    bdk_font_ref (dst_quartz_gc->font);

  dst_quartz_gc->function = src_quartz_gc->function;
  dst_quartz_gc->subwindow_mode = src_quartz_gc->subwindow_mode;
  dst_quartz_gc->graphics_exposures = src_quartz_gc->graphics_exposures;

  dst_quartz_gc->have_clip_rebunnyion = src_quartz_gc->have_clip_rebunnyion;
  dst_quartz_gc->have_clip_mask = src_quartz_gc->have_clip_mask;

  if (dst_quartz_gc->clip_mask)
    {
      CGImageRelease (dst_quartz_gc->clip_mask);
      dst_quartz_gc->clip_mask = NULL;
    }
  
  if (src_quartz_gc->clip_mask)
    dst_quartz_gc->clip_mask = _bdk_pixmap_get_cgimage (BDK_PIXMAP (src_quartz_gc->clip_mask));

  dst_quartz_gc->line_width = src_quartz_gc->line_width;
  dst_quartz_gc->line_style = src_quartz_gc->line_style;
  dst_quartz_gc->cap_style = src_quartz_gc->cap_style;
  dst_quartz_gc->join_style = src_quartz_gc->join_style;

  g_free (dst_quartz_gc->dash_lengths);
  dst_quartz_gc->dash_lengths = g_memdup (src_quartz_gc->dash_lengths,
					  sizeof (CGFloat) * src_quartz_gc->dash_count);
  dst_quartz_gc->dash_count = src_quartz_gc->dash_count;
  dst_quartz_gc->dash_phase = src_quartz_gc->dash_phase;
}

BdkScreen *  
bdk_gc_get_screen (BdkGC *gc)
{
  return _bdk_screen;
}

struct PatternCallbackInfo
{
  BdkGCQuartz *private_gc;
  BdkDrawable *drawable;
};

static void
pattern_callback_info_release (void *info)
{
  g_free (info);
}

static void
bdk_quartz_draw_tiled_pattern (void         *info,
			       CGContextRef  context)
{
  struct PatternCallbackInfo *pinfo = info;
  BdkGC       *gc = BDK_GC (pinfo->private_gc);
  CGImageRef   pattern_image;
  size_t       width, height;

  if (!context)
    return;
  
  pattern_image = _bdk_pixmap_get_cgimage (BDK_PIXMAP (_bdk_gc_get_tile (gc)));

  width = CGImageGetWidth (pattern_image);
  height = CGImageGetHeight (pattern_image);

  CGContextDrawImage (context, 
		      CGRectMake (0, 0, width, height),
		      pattern_image);
  CGImageRelease (pattern_image);
}

static void
bdk_quartz_draw_stippled_pattern (void         *info,
				  CGContextRef  context)
{
  struct PatternCallbackInfo *pinfo = info;
  BdkGC      *gc = BDK_GC (pinfo->private_gc);
  CGImageRef  pattern_image;
  CGRect      rect;
  CGColorRef  color;

  if (!context)
    return;

  pattern_image = _bdk_pixmap_get_cgimage (BDK_PIXMAP (_bdk_gc_get_stipple (gc)));
  rect = CGRectMake (0, 0,
		     CGImageGetWidth (pattern_image),
		     CGImageGetHeight (pattern_image));

  CGContextClipToMask (context, rect, pattern_image);
  color = _bdk_quartz_colormap_get_cgcolor_from_pixel (pinfo->drawable,
                                                       _bdk_gc_get_fg_pixel (gc));
  CGContextSetFillColorWithColor (context, color);
  CGColorRelease (color);

  CGContextFillRect (context, rect);

  CGImageRelease (pattern_image);
}

static void
bdk_quartz_draw_opaque_stippled_pattern (void         *info,
					 CGContextRef  context)
{
  struct PatternCallbackInfo *pinfo = info;
  BdkGC      *gc = BDK_GC (pinfo->private_gc);
  CGImageRef  pattern_image;
  CGRect      rect;
  CGColorRef  color;

  if (!context)
    return;

  pattern_image = _bdk_pixmap_get_cgimage (BDK_PIXMAP (_bdk_gc_get_stipple (gc)));
  rect = CGRectMake (0, 0,
		     CGImageGetWidth (pattern_image),
		     CGImageGetHeight (pattern_image));

  color = _bdk_quartz_colormap_get_cgcolor_from_pixel (pinfo->drawable,
                                                       _bdk_gc_get_bg_pixel (gc));
  CGContextSetFillColorWithColor (context, color);
  CGColorRelease (color);

  CGContextFillRect (context, rect);

  CGContextClipToMask (context, rect, pattern_image);
  color = _bdk_quartz_colormap_get_cgcolor_from_pixel (pinfo->drawable,
                                                       _bdk_gc_get_fg_pixel (gc));
  CGContextSetFillColorWithColor (context, color);
  CGColorRelease (color);

  CGContextFillRect (context, rect);

  CGImageRelease (pattern_image);
}

bboolean
_bdk_quartz_gc_update_cg_context (BdkGC                      *gc,
				  BdkDrawable                *drawable,
				  CGContextRef                context,
				  BdkQuartzContextValuesMask  mask)
{
  BdkGCQuartz *private;
  buint32      fg_pixel;
  buint32      bg_pixel;

  g_return_val_if_fail (gc == NULL || BDK_IS_GC (gc), FALSE);

  if (!gc || !context)
    return FALSE;

  private = BDK_GC_QUARTZ (gc);

  if (private->have_clip_rebunnyion)
    {
      CGRect rect;
      CGRect *cg_rects;
      BdkRectangle *rects;
      bint n_rects, i;

      bdk_rebunnyion_get_rectangles (_bdk_gc_get_clip_rebunnyion (gc),
				 &rects, &n_rects);

      if (!n_rects)
	  return FALSE;

      if (n_rects == 1)
	cg_rects = &rect;
      else
	cg_rects = g_new (CGRect, n_rects);

      for (i = 0; i < n_rects; i++)
	{
	  cg_rects[i].origin.x = rects[i].x + gc->clip_x_origin;
	  cg_rects[i].origin.y = rects[i].y + gc->clip_y_origin;
	  cg_rects[i].size.width = rects[i].width;
	  cg_rects[i].size.height = rects[i].height;
	}

      CGContextClipToRects (context, cg_rects, n_rects);

      g_free (rects);
      if (cg_rects != &rect)
	g_free (cg_rects);
    }
  else if (private->have_clip_mask && private->clip_mask)
    {
      /* Note: This is 10.4 only. For lower versions, we need to transform the
       * mask into a rebunnyion.
       */
      CGContextClipToMask (context,
			   CGRectMake (gc->clip_x_origin, gc->clip_y_origin,
				       CGImageGetWidth (private->clip_mask),
				       CGImageGetHeight (private->clip_mask)),
			   private->clip_mask);
    }

  fg_pixel = _bdk_gc_get_fg_pixel (gc);
  bg_pixel = _bdk_gc_get_bg_pixel (gc);

  {
    CGBlendMode blend_mode = kCGBlendModeNormal;

    switch (private->function)
      {
      case BDK_COPY:
	blend_mode = kCGBlendModeNormal;
	break;

      case BDK_INVERT:
      case BDK_XOR:
	blend_mode = kCGBlendModeExclusion;
	fg_pixel = 0xffffffff;
	bg_pixel = 0xffffffff;
	break;

      case BDK_CLEAR:
      case BDK_AND:
      case BDK_AND_REVERSE:
      case BDK_AND_INVERT:
      case BDK_NOOP:
      case BDK_OR:
      case BDK_EQUIV:
      case BDK_OR_REVERSE:
      case BDK_COPY_INVERT:
      case BDK_OR_INVERT:
      case BDK_NAND:
      case BDK_NOR:
      case BDK_SET:
	blend_mode = kCGBlendModeNormal; /* FIXME */
	break;
      }

    CGContextSetBlendMode (context, blend_mode);
  }

  /* FIXME: implement subwindow mode */

  /* FIXME: implement graphics exposures */

  if (mask & BDK_QUARTZ_CONTEXT_STROKE)
    {
      CGLineCap  line_cap  = kCGLineCapButt;
      CGLineJoin line_join = kCGLineJoinMiter;
      CGColorRef color;

      color = _bdk_quartz_colormap_get_cgcolor_from_pixel (drawable,
                                                           fg_pixel);
      CGContextSetStrokeColorWithColor (context, color);
      CGColorRelease (color);

      CGContextSetLineWidth (context, MAX (1.0, private->line_width));

      switch (private->line_style)
	{
	case BDK_LINE_SOLID:
	  CGContextSetLineDash (context, 0.0, NULL, 0);
	  break;

	case BDK_LINE_DOUBLE_DASH:
	  /* FIXME: Implement; for now, fall back to BDK_LINE_ON_OFF_DASH */

	case BDK_LINE_ON_OFF_DASH:
	  CGContextSetLineDash (context, private->dash_phase,
				private->dash_lengths, private->dash_count);
	  break;
	}

      switch (private->cap_style)
        {
	case BDK_CAP_NOT_LAST:
	  /* FIXME: Implement; for now, fall back to BDK_CAP_BUTT */
	case BDK_CAP_BUTT:
	  line_cap = kCGLineCapButt;
	  break;
	case BDK_CAP_ROUND:
	  line_cap = kCGLineCapRound;
	  break;
	case BDK_CAP_PROJECTING:
	  line_cap = kCGLineCapSquare;
	  break;
	}

      CGContextSetLineCap (context, line_cap);

      switch (private->join_style)
        {
	case BDK_JOIN_MITER:
	  line_join = kCGLineJoinMiter;
	  break;
	case BDK_JOIN_ROUND:
	  line_join = kCGLineJoinRound;
	  break;
	case BDK_JOIN_BEVEL:
	  line_join = kCGLineJoinBevel;
	  break;
	}

      CGContextSetLineJoin (context, line_join);
    }

  if (mask & BDK_QUARTZ_CONTEXT_FILL)
    {
      BdkFill         fill = _bdk_gc_get_fill (gc);
      CGColorSpaceRef baseSpace;
      CGColorSpaceRef patternSpace;
      CGFloat         alpha     = 1.0;

      if (fill == BDK_SOLID)
	{
          CGColorRef color;

	  color = _bdk_quartz_colormap_get_cgcolor_from_pixel (drawable,
                                                               fg_pixel);
	  CGContextSetFillColorWithColor (context, color);
          CGColorRelease (color);
	}
      else
	{
          struct PatternCallbackInfo *info;

	  if (!private->ts_pattern)
	    {
	      bfloat     width, height;
	      bboolean   is_colored = FALSE;
	      CGPatternCallbacks callbacks =  { 0, NULL, NULL };
	      CGPoint    phase;
              BdkPixmapImplQuartz *pix_impl = NULL;

              info = g_new (struct PatternCallbackInfo, 1);
              private->ts_pattern_info = info;

              /* Won't ref to avoid circular dependencies */
              info->drawable = drawable;
              info->private_gc = private;

              callbacks.releaseInfo = pattern_callback_info_release;

	      switch (fill)
		{
		case BDK_TILED:
		  pix_impl = BDK_PIXMAP_IMPL_QUARTZ (BDK_PIXMAP_OBJECT (_bdk_gc_get_tile (gc))->impl);
		  width = pix_impl->width;
		  height = pix_impl->height;
		  is_colored = TRUE;
		  callbacks.drawPattern = bdk_quartz_draw_tiled_pattern;
		  break;
		case BDK_STIPPLED:
		  pix_impl = BDK_PIXMAP_IMPL_QUARTZ (BDK_PIXMAP_OBJECT (_bdk_gc_get_stipple (gc))->impl);
		  width = pix_impl->width;
		  height = pix_impl->height;
		  is_colored = FALSE;
		  callbacks.drawPattern = bdk_quartz_draw_stippled_pattern;
		  break;
		case BDK_OPAQUE_STIPPLED:
		  pix_impl = BDK_PIXMAP_IMPL_QUARTZ (BDK_PIXMAP_OBJECT (_bdk_gc_get_stipple (gc))->impl);
		  width = pix_impl->width;
		  height = pix_impl->height;
		  is_colored = TRUE;
		  callbacks.drawPattern = bdk_quartz_draw_opaque_stippled_pattern;
		  break;
		default:
		  break;
		}

	      phase = CGPointApplyAffineTransform (CGPointMake (gc->ts_x_origin, gc->ts_y_origin), CGContextGetCTM (context));
	      CGContextSetPatternPhase (context, CGSizeMake (phase.x, phase.y));

	      private->ts_pattern = CGPatternCreate (info,
						     CGRectMake (0, 0, width, height),
						     CGAffineTransformIdentity,
						     width, height,
						     kCGPatternTilingConstantSpacing,
						     is_colored,
						     &callbacks);
	    }
          else
            info = (struct PatternCallbackInfo *)private->ts_pattern_info;

          /* Update drawable in the pattern callback info.  Again, we
           * won't ref to avoid circular dependencies.
           */
          info->drawable = drawable;

	  baseSpace = (fill == BDK_STIPPLED) ? CGColorSpaceCreateDeviceRGB () : NULL;
	  patternSpace = CGColorSpaceCreatePattern (baseSpace);

	  CGContextSetFillColorSpace (context, patternSpace);
	  CGColorSpaceRelease (patternSpace);
	  CGColorSpaceRelease (baseSpace);

	  if (fill == BDK_STIPPLED)
            {
              CGColorRef color;
              const CGFloat *components;

              color = _bdk_quartz_colormap_get_cgcolor_from_pixel (drawable,
                                                                   fg_pixel);
              components = CGColorGetComponents (color);

              CGContextSetFillPattern (context, private->ts_pattern,
                                       components);
              CGColorRelease (color);
            }
          else
            CGContextSetFillPattern (context, private->ts_pattern, &alpha);
       }
    }

  if (mask & BDK_QUARTZ_CONTEXT_TEXT)
    {
      /* FIXME: implement text */
    }

  if (BDK_IS_WINDOW_IMPL_QUARTZ (drawable))
      private->is_window = TRUE;
  else
      private->is_window = FALSE;

  return TRUE;
}
