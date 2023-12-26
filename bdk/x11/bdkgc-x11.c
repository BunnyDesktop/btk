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

#include "bdkgc.h"
#include "bdkprivate-x11.h"
#include "bdkrebunnyion-generic.h"
#include "bdkx.h"
#include "bdkalias.h"

#include <string.h>

typedef enum {
  BDK_GC_DIRTY_CLIP = 1 << 0,
  BDK_GC_DIRTY_TS = 1 << 1
} BdkGCDirtyValues;

static void bdk_x11_gc_values_to_xvalues (BdkGCValues    *values,
					  BdkGCValuesMask mask,
					  XGCValues      *xvalues,
					  unsigned long  *xvalues_mask);

static void bdk_x11_gc_get_values (BdkGC           *gc,
				   BdkGCValues     *values);
static void bdk_x11_gc_set_values (BdkGC           *gc,
				   BdkGCValues     *values,
				   BdkGCValuesMask  values_mask);
static void bdk_x11_gc_set_dashes (BdkGC           *gc,
				   bint             dash_offset,
				   bint8            dash_list[],
				   bint             n);

static void bdk_gc_x11_finalize   (BObject         *object);

G_DEFINE_TYPE (BdkGCX11, _bdk_gc_x11, BDK_TYPE_GC)

static void
_bdk_gc_x11_class_init (BdkGCX11Class *klass)
{
  BObjectClass *object_class = B_OBJECT_CLASS (klass);
  BdkGCClass *gc_class = BDK_GC_CLASS (klass);
  
  object_class->finalize = bdk_gc_x11_finalize;

  gc_class->get_values = bdk_x11_gc_get_values;
  gc_class->set_values = bdk_x11_gc_set_values;
  gc_class->set_dashes = bdk_x11_gc_set_dashes;
}

static void
_bdk_gc_x11_init (BdkGCX11 *gc)
{
}

static void
bdk_gc_x11_finalize (BObject *object)
{
  BdkGCX11 *x11_gc = BDK_GC_X11 (object);
  
  XFreeGC (BDK_GC_XDISPLAY (x11_gc), BDK_GC_XGC (x11_gc));

  B_OBJECT_CLASS (_bdk_gc_x11_parent_class)->finalize (object);
}


BdkGC *
_bdk_x11_gc_new (BdkDrawable      *drawable,
		 BdkGCValues      *values,
		 BdkGCValuesMask   values_mask)
{
  BdkGC *gc;
  BdkGCX11 *private;
  
  XGCValues xvalues;
  unsigned long xvalues_mask;

  /* NOTICE that the drawable here has to be the impl drawable,
   * not the publically-visible drawables.
   */
  g_return_val_if_fail (BDK_IS_DRAWABLE_IMPL_X11 (drawable), NULL);

  gc = g_object_new (_bdk_gc_x11_get_type (), NULL);
  private = BDK_GC_X11 (gc);

  _bdk_gc_init (gc, drawable, values, values_mask);

  private->dirty_mask = 0;
  private->have_clip_mask = FALSE;
    
  private->screen = BDK_DRAWABLE_IMPL_X11 (drawable)->screen;

  private->depth = bdk_drawable_get_depth (drawable);

  if (values_mask & (BDK_GC_CLIP_X_ORIGIN | BDK_GC_CLIP_Y_ORIGIN))
    {
      values_mask &= ~(BDK_GC_CLIP_X_ORIGIN | BDK_GC_CLIP_Y_ORIGIN);
      private->dirty_mask |= BDK_GC_DIRTY_CLIP;
    }

  if (values_mask & (BDK_GC_TS_X_ORIGIN | BDK_GC_TS_Y_ORIGIN))
    {
      values_mask &= ~(BDK_GC_TS_X_ORIGIN | BDK_GC_TS_Y_ORIGIN);
      private->dirty_mask |= BDK_GC_DIRTY_TS;
    }

  if ((values_mask & BDK_GC_CLIP_MASK) && values->clip_mask)
    private->have_clip_mask = TRUE;

  xvalues.function = GXcopy;
  xvalues.fill_style = FillSolid;
  xvalues.arc_mode = ArcPieSlice;
  xvalues.subwindow_mode = ClipByChildren;
  xvalues.graphics_exposures = False;
  xvalues_mask = GCFunction | GCFillStyle | GCArcMode | GCSubwindowMode | GCGraphicsExposures;

  bdk_x11_gc_values_to_xvalues (values, values_mask, &xvalues, &xvalues_mask);
  
  private->xgc = XCreateGC (BDK_GC_XDISPLAY (gc),
                            BDK_DRAWABLE_IMPL_X11 (drawable)->xid,
                            xvalues_mask, &xvalues);

  return gc;
}

GC
_bdk_x11_gc_flush (BdkGC *gc)
{
  Display *xdisplay = BDK_GC_XDISPLAY (gc);
  BdkGCX11 *private = BDK_GC_X11 (gc);
  GC xgc = private->xgc;

  if (private->dirty_mask & BDK_GC_DIRTY_CLIP)
    {
      BdkRebunnyion *clip_rebunnyion = _bdk_gc_get_clip_rebunnyion (gc);
      
      if (!clip_rebunnyion)
	XSetClipOrigin (xdisplay, xgc,
			gc->clip_x_origin, gc->clip_y_origin);
      else
	{
	  XRectangle *rectangles;
          bint n_rects;

          _bdk_rebunnyion_get_xrectangles (clip_rebunnyion,
                                       gc->clip_x_origin,
                                       gc->clip_y_origin,
                                       &rectangles,
                                       &n_rects);
	  
	  XSetClipRectangles (xdisplay, xgc, 0, 0,
                              rectangles,
                              n_rects, YXBanded);
          
	  g_free (rectangles);
	}
    }

  if (private->dirty_mask & BDK_GC_DIRTY_TS)
    {
      XSetTSOrigin (xdisplay, xgc,
		    gc->ts_x_origin, gc->ts_y_origin);
    }

  private->dirty_mask = 0;
  return xgc;
}

static void
bdk_x11_gc_get_values (BdkGC       *gc,
		       BdkGCValues *values)
{
  XGCValues xvalues;
  
  if (XGetGCValues (BDK_GC_XDISPLAY (gc), BDK_GC_XGC (gc),
		    GCForeground | GCBackground | GCFont |
		    GCFunction | GCTile | GCStipple | /* GCClipMask | */
		    GCSubwindowMode | GCGraphicsExposures |
		    GCTileStipXOrigin | GCTileStipYOrigin |
		    GCClipXOrigin | GCClipYOrigin |
		    GCLineWidth | GCLineStyle | GCCapStyle |
		    GCFillStyle | GCJoinStyle, &xvalues))
    {
      values->foreground.pixel = xvalues.foreground;
      values->background.pixel = xvalues.background;
      values->font = bdk_font_lookup_for_display (BDK_GC_DISPLAY (gc),
						  xvalues.font);

      switch (xvalues.function)
	{
	case GXcopy:
	  values->function = BDK_COPY;
	  break;
	case GXinvert:
	  values->function = BDK_INVERT;
	  break;
	case GXxor:
	  values->function = BDK_XOR;
	  break;
	case GXclear:
	  values->function = BDK_CLEAR;
	  break;
	case GXand:
	  values->function = BDK_AND;
	  break;
	case GXandReverse:
	  values->function = BDK_AND_REVERSE;
	  break;
	case GXandInverted:
	  values->function = BDK_AND_INVERT;
	  break;
	case GXnoop:
	  values->function = BDK_NOOP;
	  break;
	case GXor:
	  values->function = BDK_OR;
	  break;
	case GXequiv:
	  values->function = BDK_EQUIV;
	  break;
	case GXorReverse:
	  values->function = BDK_OR_REVERSE;
	  break;
	case GXcopyInverted:
	  values->function =BDK_COPY_INVERT;
	  break;
	case GXorInverted:
	  values->function = BDK_OR_INVERT;
	  break;
	case GXnand:
	  values->function = BDK_NAND;
	  break;
	case GXset:
	  values->function = BDK_SET;
	  break;
	case GXnor:
	  values->function = BDK_NOR;
	  break;
	}

      switch (xvalues.fill_style)
	{
	case FillSolid:
	  values->fill = BDK_SOLID;
	  break;
	case FillTiled:
	  values->fill = BDK_TILED;
	  break;
	case FillStippled:
	  values->fill = BDK_STIPPLED;
	  break;
	case FillOpaqueStippled:
	  values->fill = BDK_OPAQUE_STIPPLED;
	  break;
	}

      values->tile = bdk_pixmap_lookup_for_display (BDK_GC_DISPLAY (gc),
						    xvalues.tile);
      values->stipple = bdk_pixmap_lookup_for_display (BDK_GC_DISPLAY (gc),
						       xvalues.stipple);
      values->clip_mask = NULL;
      values->subwindow_mode = xvalues.subwindow_mode;
      values->ts_x_origin = xvalues.ts_x_origin;
      values->ts_y_origin = xvalues.ts_y_origin;
      values->clip_x_origin = xvalues.clip_x_origin;
      values->clip_y_origin = xvalues.clip_y_origin;
      values->graphics_exposures = xvalues.graphics_exposures;
      values->line_width = xvalues.line_width;

      switch (xvalues.line_style)
	{
	case LineSolid:
	  values->line_style = BDK_LINE_SOLID;
	  break;
	case LineOnOffDash:
	  values->line_style = BDK_LINE_ON_OFF_DASH;
	  break;
	case LineDoubleDash:
	  values->line_style = BDK_LINE_DOUBLE_DASH;
	  break;
	}

      switch (xvalues.cap_style)
	{
	case CapNotLast:
	  values->cap_style = BDK_CAP_NOT_LAST;
	  break;
	case CapButt:
	  values->cap_style = BDK_CAP_BUTT;
	  break;
	case CapRound:
	  values->cap_style = BDK_CAP_ROUND;
	  break;
	case CapProjecting:
	  values->cap_style = BDK_CAP_PROJECTING;
	  break;
	}

      switch (xvalues.join_style)
	{
	case JoinMiter:
	  values->join_style = BDK_JOIN_MITER;
	  break;
	case JoinRound:
	  values->join_style = BDK_JOIN_ROUND;
	  break;
	case JoinBevel:
	  values->join_style = BDK_JOIN_BEVEL;
	  break;
	}
    }
  else
    {
      memset (values, 0, sizeof (BdkGCValues));
    }
}

static void
bdk_x11_gc_set_values (BdkGC           *gc,
		       BdkGCValues     *values,
		       BdkGCValuesMask  values_mask)
{
  BdkGCX11 *x11_gc;
  XGCValues xvalues;
  unsigned long xvalues_mask = 0;

  x11_gc = BDK_GC_X11 (gc);

  if (values_mask & (BDK_GC_CLIP_X_ORIGIN | BDK_GC_CLIP_Y_ORIGIN))
    {
      values_mask &= ~(BDK_GC_CLIP_X_ORIGIN | BDK_GC_CLIP_Y_ORIGIN);
      x11_gc->dirty_mask |= BDK_GC_DIRTY_CLIP;
    }

  if (values_mask & (BDK_GC_TS_X_ORIGIN | BDK_GC_TS_Y_ORIGIN))
    {
      values_mask &= ~(BDK_GC_TS_X_ORIGIN | BDK_GC_TS_Y_ORIGIN);
      x11_gc->dirty_mask |= BDK_GC_DIRTY_TS;
    }

  if (values_mask & BDK_GC_CLIP_MASK)
    {
      x11_gc->have_clip_rebunnyion = FALSE;
      x11_gc->have_clip_mask = values->clip_mask != NULL;
    }

  bdk_x11_gc_values_to_xvalues (values, values_mask, &xvalues, &xvalues_mask);

  XChangeGC (BDK_GC_XDISPLAY (gc),
	     BDK_GC_XGC (gc),
	     xvalues_mask,
	     &xvalues);
}

static void
bdk_x11_gc_set_dashes (BdkGC *gc,
		       bint   dash_offset,
		       bint8  dash_list[],
		       bint   n)
{
  g_return_if_fail (BDK_IS_GC (gc));
  g_return_if_fail (dash_list != NULL);

  XSetDashes (BDK_GC_XDISPLAY (gc), BDK_GC_XGC (gc),
	      dash_offset, (char *)dash_list, n);
}

static void
bdk_x11_gc_values_to_xvalues (BdkGCValues    *values,
			      BdkGCValuesMask mask,
			      XGCValues      *xvalues,
			      unsigned long  *xvalues_mask)
{
  /* Optimization for the common case (bdk_gc_new()) */
  if (values == NULL || mask == 0)
    return;
  
  if (mask & BDK_GC_FOREGROUND)
    {
      xvalues->foreground = values->foreground.pixel;
      *xvalues_mask |= GCForeground;
    }
  if (mask & BDK_GC_BACKGROUND)
    {
      xvalues->background = values->background.pixel;
      *xvalues_mask |= GCBackground;
    }
  if ((mask & BDK_GC_FONT) && (values->font->type == BDK_FONT_FONT))
    {
      xvalues->font = ((XFontStruct *) (BDK_FONT_XFONT (values->font)))->fid;
      *xvalues_mask |= GCFont;
    }
  if (mask & BDK_GC_FUNCTION)
    {
      switch (values->function)
	{
	case BDK_COPY:
	  xvalues->function = GXcopy;
	  break;
	case BDK_INVERT:
	  xvalues->function = GXinvert;
	  break;
	case BDK_XOR:
	  xvalues->function = GXxor;
	  break;
	case BDK_CLEAR:
	  xvalues->function = GXclear;
	  break;
	case BDK_AND:
	  xvalues->function = GXand;
	  break;
	case BDK_AND_REVERSE:
	  xvalues->function = GXandReverse;
	  break;
	case BDK_AND_INVERT:
	  xvalues->function = GXandInverted;
	  break;
	case BDK_NOOP:
	  xvalues->function = GXnoop;
	  break;
	case BDK_OR:
	  xvalues->function = GXor;
	  break;
	case BDK_EQUIV:
	  xvalues->function = GXequiv;
	  break;
	case BDK_OR_REVERSE:
	  xvalues->function = GXorReverse;
	  break;
	case BDK_COPY_INVERT:
	  xvalues->function = GXcopyInverted;
	  break;
	case BDK_OR_INVERT:
	  xvalues->function = GXorInverted;
	  break;
	case BDK_NAND:
	  xvalues->function = GXnand;
	  break;
	case BDK_SET:
	  xvalues->function = GXset;
	  break;
	case BDK_NOR:
	  xvalues->function = GXnor;
	  break;
	}
      *xvalues_mask |= GCFunction;
    }
  if (mask & BDK_GC_FILL)
    {
      switch (values->fill)
	{
	case BDK_SOLID:
	  xvalues->fill_style = FillSolid;
	  break;
	case BDK_TILED:
	  xvalues->fill_style = FillTiled;
	  break;
	case BDK_STIPPLED:
	  xvalues->fill_style = FillStippled;
	  break;
	case BDK_OPAQUE_STIPPLED:
	  xvalues->fill_style = FillOpaqueStippled;
	  break;
	}
      *xvalues_mask |= GCFillStyle;
    }
  if (mask & BDK_GC_TILE)
    {
      if (values->tile)
	xvalues->tile = BDK_DRAWABLE_XID (values->tile);
      else
	xvalues->tile = None;
      
      *xvalues_mask |= GCTile;
    }
  if (mask & BDK_GC_STIPPLE)
    {
      if (values->stipple)
	xvalues->stipple = BDK_DRAWABLE_XID (values->stipple);
      else
	xvalues->stipple = None;
      
      *xvalues_mask |= GCStipple;
    }
  if (mask & BDK_GC_CLIP_MASK)
    {
      if (values->clip_mask)
	xvalues->clip_mask = BDK_DRAWABLE_XID (values->clip_mask);
      else
	xvalues->clip_mask = None;

      *xvalues_mask |= GCClipMask;
      
    }
  if (mask & BDK_GC_SUBWINDOW)
    {
      xvalues->subwindow_mode = values->subwindow_mode;
      *xvalues_mask |= GCSubwindowMode;
    }
  if (mask & BDK_GC_TS_X_ORIGIN)
    {
      xvalues->ts_x_origin = values->ts_x_origin;
      *xvalues_mask |= GCTileStipXOrigin;
    }
  if (mask & BDK_GC_TS_Y_ORIGIN)
    {
      xvalues->ts_y_origin = values->ts_y_origin;
      *xvalues_mask |= GCTileStipYOrigin;
    }
  if (mask & BDK_GC_CLIP_X_ORIGIN)
    {
      xvalues->clip_x_origin = values->clip_x_origin;
      *xvalues_mask |= GCClipXOrigin;
    }
  if (mask & BDK_GC_CLIP_Y_ORIGIN)
    {
      xvalues->clip_y_origin = values->clip_y_origin;
      *xvalues_mask |= GCClipYOrigin;
    }

  if (mask & BDK_GC_EXPOSURES)
    {
      xvalues->graphics_exposures = values->graphics_exposures;
      *xvalues_mask |= GCGraphicsExposures;
    }

  if (mask & BDK_GC_LINE_WIDTH)
    {
      xvalues->line_width = values->line_width;
      *xvalues_mask |= GCLineWidth;
    }
  if (mask & BDK_GC_LINE_STYLE)
    {
      switch (values->line_style)
	{
	case BDK_LINE_SOLID:
	  xvalues->line_style = LineSolid;
	  break;
	case BDK_LINE_ON_OFF_DASH:
	  xvalues->line_style = LineOnOffDash;
	  break;
	case BDK_LINE_DOUBLE_DASH:
	  xvalues->line_style = LineDoubleDash;
	  break;
	}
      *xvalues_mask |= GCLineStyle;
    }
  if (mask & BDK_GC_CAP_STYLE)
    {
      switch (values->cap_style)
	{
	case BDK_CAP_NOT_LAST:
	  xvalues->cap_style = CapNotLast;
	  break;
	case BDK_CAP_BUTT:
	  xvalues->cap_style = CapButt;
	  break;
	case BDK_CAP_ROUND:
	  xvalues->cap_style = CapRound;
	  break;
	case BDK_CAP_PROJECTING:
	  xvalues->cap_style = CapProjecting;
	  break;
	}
      *xvalues_mask |= GCCapStyle;
    }
  if (mask & BDK_GC_JOIN_STYLE)
    {
      switch (values->join_style)
	{
	case BDK_JOIN_MITER:
	  xvalues->join_style = JoinMiter;
	  break;
	case BDK_JOIN_ROUND:
	  xvalues->join_style = JoinRound;
	  break;
	case BDK_JOIN_BEVEL:
	  xvalues->join_style = JoinBevel;
	  break;
	}
      *xvalues_mask |= GCJoinStyle;
    }

}

void
_bdk_windowing_gc_set_clip_rebunnyion (BdkGC           *gc,
				   const BdkRebunnyion *rebunnyion,
				   bboolean reset_origin)
{
  BdkGCX11 *x11_gc = BDK_GC_X11 (gc);

  /* Unset immediately, to make sure Xlib doesn't keep the
   * XID of an old clip mask cached
   */
  if ((x11_gc->have_clip_rebunnyion && !rebunnyion) || x11_gc->have_clip_mask)
    {
      XSetClipMask (BDK_GC_XDISPLAY (gc), BDK_GC_XGC (gc), None);
      x11_gc->have_clip_mask = FALSE;
    }

  x11_gc->have_clip_rebunnyion = rebunnyion != NULL;

  if (reset_origin)
    {
      gc->clip_x_origin = 0;
      gc->clip_y_origin = 0;
    }

  x11_gc->dirty_mask |= BDK_GC_DIRTY_CLIP;
}

void
_bdk_windowing_gc_copy (BdkGC *dst_gc,
			BdkGC *src_gc)
{
  BdkGCX11 *x11_src_gc = BDK_GC_X11 (src_gc);
  BdkGCX11 *x11_dst_gc = BDK_GC_X11 (dst_gc);

  XCopyGC (BDK_GC_XDISPLAY (src_gc), BDK_GC_XGC (src_gc), ~((~1) << GCLastBit),
	   BDK_GC_XGC (dst_gc));

  x11_dst_gc->dirty_mask = x11_src_gc->dirty_mask;
  x11_dst_gc->have_clip_rebunnyion = x11_src_gc->have_clip_rebunnyion;
  x11_dst_gc->have_clip_mask = x11_src_gc->have_clip_mask;
}

/**
 * bdk_gc_get_screen:
 * @gc: a #BdkGC.
 *
 * Gets the #BdkScreen for which @gc was created
 *
 * Returns: the #BdkScreen for @gc.
 *
 * Since: 2.2
 */
BdkScreen *  
bdk_gc_get_screen (BdkGC *gc)
{
  g_return_val_if_fail (BDK_IS_GC_X11 (gc), NULL);
  
  return BDK_GC_X11 (gc)->screen;
}

/**
 * bdk_x11_gc_get_xdisplay:
 * @gc: a #BdkGC.
 * 
 * Returns the display of a #BdkGC.
 * 
 * Return value: an Xlib <type>Display*</type>.
 *
 * Deprecated: 2.22: #BdkGC has been replaced by #bairo_t.
 **/
Display *
bdk_x11_gc_get_xdisplay (BdkGC *gc)
{
  g_return_val_if_fail (BDK_IS_GC_X11 (gc), NULL);

  return BDK_SCREEN_XDISPLAY (bdk_gc_get_screen (gc));
}

/**
 * bdk_x11_gc_get_xgc:
 * @gc: a #BdkGC.
 * 
 * Returns the X GC of a #BdkGC.
 * 
 * Return value: an Xlib <type>GC</type>.
 *
 * Deprecated: 2.22: #BdkGC has been replaced by #bairo_t.
 **/
GC
bdk_x11_gc_get_xgc (BdkGC *gc)
{
  BdkGCX11 *gc_x11;
  
  g_return_val_if_fail (BDK_IS_GC_X11 (gc), NULL);

  gc_x11 = BDK_GC_X11 (gc);

  if (gc_x11->dirty_mask)
    _bdk_x11_gc_flush (gc);

  return gc_x11->xgc;
}

#define __BDK_GC_X11_C__
#include "bdkaliasdef.c"
