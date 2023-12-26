/* GIMP Drawing Kit
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

#include "bdkx.h"
#include "bdkrebunnyion-generic.h"

#include <bairo-xlib.h>

#include <stdlib.h>
#include <string.h>		/* for memcpy() */

#if defined (HAVE_IPC_H) && defined (HAVE_SHM_H) && defined (HAVE_XSHM_H)
#define USE_SHM
#endif

#ifdef USE_SHM
#include <X11/extensions/XShm.h>
#endif /* USE_SHM */

#include "bdkprivate-x11.h"
#include "bdkdrawable-x11.h"
#include "bdkpixmap-x11.h"
#include "bdkscreen-x11.h"
#include "bdkdisplay-x11.h"

#include "bdkalias.h"

static void bdk_x11_draw_rectangle (BdkDrawable    *drawable,
				    BdkGC          *gc,
				    gboolean        filled,
				    gint            x,
				    gint            y,
				    gint            width,
				    gint            height);
static void bdk_x11_draw_arc       (BdkDrawable    *drawable,
				    BdkGC          *gc,
				    gboolean        filled,
				    gint            x,
				    gint            y,
				    gint            width,
				    gint            height,
				    gint            angle1,
				    gint            angle2);
static void bdk_x11_draw_polygon   (BdkDrawable    *drawable,
				    BdkGC          *gc,
				    gboolean        filled,
				    BdkPoint       *points,
				    gint            npoints);
static void bdk_x11_draw_text      (BdkDrawable    *drawable,
				    BdkFont        *font,
				    BdkGC          *gc,
				    gint            x,
				    gint            y,
				    const gchar    *text,
				    gint            text_length);
static void bdk_x11_draw_text_wc   (BdkDrawable    *drawable,
				    BdkFont        *font,
				    BdkGC          *gc,
				    gint            x,
				    gint            y,
				    const BdkWChar *text,
				    gint            text_length);
static void bdk_x11_draw_drawable  (BdkDrawable    *drawable,
				    BdkGC          *gc,
				    BdkPixmap      *src,
				    gint            xsrc,
				    gint            ysrc,
				    gint            xdest,
				    gint            ydest,
				    gint            width,
				    gint            height,
				    BdkDrawable    *original_src);
static void bdk_x11_draw_points    (BdkDrawable    *drawable,
				    BdkGC          *gc,
				    BdkPoint       *points,
				    gint            npoints);
static void bdk_x11_draw_segments  (BdkDrawable    *drawable,
				    BdkGC          *gc,
				    BdkSegment     *segs,
				    gint            nsegs);
static void bdk_x11_draw_lines     (BdkDrawable    *drawable,
				    BdkGC          *gc,
				    BdkPoint       *points,
				    gint            npoints);

static void bdk_x11_draw_image     (BdkDrawable     *drawable,
                                    BdkGC           *gc,
                                    BdkImage        *image,
                                    gint             xsrc,
                                    gint             ysrc,
                                    gint             xdest,
                                    gint             ydest,
                                    gint             width,
                                    gint             height);
static void bdk_x11_draw_pixbuf    (BdkDrawable     *drawable,
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
				    gint             y_dither);

static bairo_surface_t *bdk_x11_ref_bairo_surface (BdkDrawable *drawable);
     
static void bdk_x11_set_colormap   (BdkDrawable    *drawable,
                                    BdkColormap    *colormap);

static BdkColormap* bdk_x11_get_colormap   (BdkDrawable    *drawable);
static gint         bdk_x11_get_depth      (BdkDrawable    *drawable);
static BdkScreen *  bdk_x11_get_screen	   (BdkDrawable    *drawable);
static BdkVisual*   bdk_x11_get_visual     (BdkDrawable    *drawable);

static void bdk_drawable_impl_x11_finalize   (GObject *object);

static const bairo_user_data_key_t bdk_x11_bairo_key;

G_DEFINE_TYPE (BdkDrawableImplX11, _bdk_drawable_impl_x11, BDK_TYPE_DRAWABLE)

static void
_bdk_drawable_impl_x11_class_init (BdkDrawableImplX11Class *klass)
{
  BdkDrawableClass *drawable_class = BDK_DRAWABLE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  object_class->finalize = bdk_drawable_impl_x11_finalize;
  
  drawable_class->create_gc = _bdk_x11_gc_new;
  drawable_class->draw_rectangle = bdk_x11_draw_rectangle;
  drawable_class->draw_arc = bdk_x11_draw_arc;
  drawable_class->draw_polygon = bdk_x11_draw_polygon;
  drawable_class->draw_text = bdk_x11_draw_text;
  drawable_class->draw_text_wc = bdk_x11_draw_text_wc;
  drawable_class->draw_drawable_with_src = bdk_x11_draw_drawable;
  drawable_class->draw_points = bdk_x11_draw_points;
  drawable_class->draw_segments = bdk_x11_draw_segments;
  drawable_class->draw_lines = bdk_x11_draw_lines;
  drawable_class->draw_image = bdk_x11_draw_image;
  drawable_class->draw_pixbuf = bdk_x11_draw_pixbuf;
  
  drawable_class->ref_bairo_surface = bdk_x11_ref_bairo_surface;

  drawable_class->set_colormap = bdk_x11_set_colormap;
  drawable_class->get_colormap = bdk_x11_get_colormap;

  drawable_class->get_depth = bdk_x11_get_depth;
  drawable_class->get_screen = bdk_x11_get_screen;
  drawable_class->get_visual = bdk_x11_get_visual;
  
  drawable_class->_copy_to_image = _bdk_x11_copy_to_image;
}

static void
_bdk_drawable_impl_x11_init (BdkDrawableImplX11 *impl)
{
}

static void
bdk_drawable_impl_x11_finalize (GObject *object)
{
  bdk_drawable_set_colormap (BDK_DRAWABLE (object), NULL);

  G_OBJECT_CLASS (_bdk_drawable_impl_x11_parent_class)->finalize (object);
}

/**
 * _bdk_x11_drawable_finish:
 * @drawable: a #BdkDrawableImplX11.
 * 
 * Performs necessary cleanup prior to freeing a pixmap or
 * destroying a window.
 **/
void
_bdk_x11_drawable_finish (BdkDrawable *drawable)
{
  BdkDrawableImplX11 *impl = BDK_DRAWABLE_IMPL_X11 (drawable);
  
  if (impl->picture)
    {
      XRenderFreePicture (BDK_SCREEN_XDISPLAY (impl->screen),
			  impl->picture);
      impl->picture = None;
    }
  
  if (impl->bairo_surface)
    {
      bairo_surface_finish (impl->bairo_surface);
      bairo_surface_set_user_data (impl->bairo_surface, &bdk_x11_bairo_key,
				   NULL, NULL);
    }
}

/**
 * _bdk_x11_drawable_update_size:
 * @drawable: a #BdkDrawableImplX11.
 * 
 * Updates the state of the drawable (in particular the drawable's
 * bairo surface) when its size has changed.
 **/
void
_bdk_x11_drawable_update_size (BdkDrawable *drawable)
{
  BdkDrawableImplX11 *impl = BDK_DRAWABLE_IMPL_X11 (drawable);
  
  if (impl->bairo_surface)
    {
      int width, height;
      
      bdk_drawable_get_size (drawable, &width, &height);
      bairo_xlib_surface_set_size (impl->bairo_surface, width, height);
    }
}

static void
try_pixmap (Display *xdisplay,
	    int      screen,
	    int      depth)
{
  Pixmap pixmap = XCreatePixmap (xdisplay,
				 RootWindow (xdisplay, screen),
				 1, 1, depth);
  XFreePixmap (xdisplay, pixmap);
}

gboolean
_bdk_x11_have_render (BdkDisplay *display)
{
  Display *xdisplay = BDK_DISPLAY_XDISPLAY (display);
  BdkDisplayX11 *x11display = BDK_DISPLAY_X11 (display);

  if (x11display->have_render == BDK_UNKNOWN)
    {
      int event_base, error_base;
      x11display->have_render =
	XRenderQueryExtension (xdisplay, &event_base, &error_base)
	? BDK_YES : BDK_NO;

      if (x11display->have_render == BDK_YES)
	{
	  /*
	   * Sun advertises RENDER, but fails to support 32-bit pixmaps.
	   * That is just no good.  Therefore, we check all screens
	   * for proper support.
	   */

	  int screen;
	  for (screen = 0; screen < ScreenCount (xdisplay); screen++)
	    {
	      int count;
	      int *depths = XListDepths (xdisplay, screen, &count);
	      gboolean has_8 = FALSE, has_32 = FALSE;

	      if (depths)
		{
		  int i;

		  for (i = 0; i < count; i++)
		    {
		      if (depths[i] == 8)
			has_8 = TRUE;
		      else if (depths[i] == 32)
			has_32 = TRUE;
		    }
		  XFree (depths);
		}

	      /* At this point, we might have a false positive;
	       * buggy versions of Xinerama only report depths for
	       * which there is an associated visual; so we actually
	       * go ahead and try create pixmaps.
	       */
	      if (!(has_8 && has_32))
		{
		  bdk_error_trap_push ();
		  if (!has_8)
		    try_pixmap (xdisplay, screen, 8);
		  if (!has_32)
		    try_pixmap (xdisplay, screen, 32);
		  XSync (xdisplay, False);
		  if (bdk_error_trap_pop () == 0)
		    {
		      has_8 = TRUE;
		      has_32 = TRUE;
		    }
		}
	      
	      if (!(has_8 && has_32))
		{
		  g_warning ("The X server advertises that RENDER support is present,\n"
			     "but fails to supply the necessary pixmap support.  In\n"
			     "other words, it is buggy.");
		  x11display->have_render = BDK_NO;
		  break;
		}
	    }
	}
    }

  return x11display->have_render == BDK_YES;
}

static Picture
bdk_x11_drawable_get_picture (BdkDrawable *drawable)
{
  BdkDrawableImplX11 *impl = BDK_DRAWABLE_IMPL_X11 (drawable);
  
  if (!impl->picture)
    {
      Display *xdisplay = BDK_SCREEN_XDISPLAY (impl->screen);
      XRenderPictFormat *format;
      
      BdkVisual *visual = bdk_drawable_get_visual (BDK_DRAWABLE_IMPL_X11 (drawable)->wrapper);
      if (!visual)
	return None;

      format = XRenderFindVisualFormat (xdisplay, BDK_VISUAL_XVISUAL (visual));
      if (format)
	{
	  XRenderPictureAttributes attributes;
	  attributes.graphics_exposures = False;
	  
	  impl->picture = XRenderCreatePicture (xdisplay, impl->xid, format,
						CPGraphicsExposure, &attributes);
	}
    }
  
  return impl->picture;
}

static void
bdk_x11_drawable_update_picture_clip (BdkDrawable *drawable,
				      BdkGC       *gc)
{
  BdkDrawableImplX11 *impl = BDK_DRAWABLE_IMPL_X11 (drawable);
  Display *xdisplay = BDK_SCREEN_XDISPLAY (impl->screen);
  Picture picture = bdk_x11_drawable_get_picture (drawable);
  BdkRebunnyion *clip_rebunnyion = gc ? _bdk_gc_get_clip_rebunnyion (gc) : NULL;

  if (clip_rebunnyion)
    {
      BdkRebunnyionBox *boxes = clip_rebunnyion->rects;
      gint n_boxes = clip_rebunnyion->numRects;
      XRectangle *rects = g_new (XRectangle, n_boxes);
      int i;

      for (i=0; i < n_boxes; i++)
	{
	  rects[i].x = CLAMP (boxes[i].x1 + gc->clip_x_origin, G_MINSHORT, G_MAXSHORT);
	  rects[i].y = CLAMP (boxes[i].y1 + gc->clip_y_origin, G_MINSHORT, G_MAXSHORT);
	  rects[i].width = CLAMP (boxes[i].x2 + gc->clip_x_origin, G_MINSHORT, G_MAXSHORT) - rects[i].x;
	  rects[i].height = CLAMP (boxes[i].y2 + gc->clip_y_origin, G_MINSHORT, G_MAXSHORT) - rects[i].y;
	}
      
      XRenderSetPictureClipRectangles (xdisplay, picture,
				       0, 0, rects, n_boxes);
      
      g_free (rects);
    }
  else
    {
      XRenderPictureAttributes pa;
      BdkBitmap *mask;
      gulong pa_mask;

      pa_mask = CPClipMask;
      if (gc && (mask = _bdk_gc_get_clip_mask (gc)))
	{
	  pa.clip_mask = BDK_PIXMAP_XID (mask);
	  pa.clip_x_origin = gc->clip_x_origin;
	  pa.clip_y_origin = gc->clip_y_origin;
	  pa_mask |= CPClipXOrigin | CPClipYOrigin;
	}
      else
	pa.clip_mask = None;

      XRenderChangePicture (xdisplay, picture,
			    pa_mask, &pa);
    }
}

/*****************************************************
 * X11 specific implementations of generic functions *
 *****************************************************/

static BdkColormap*
bdk_x11_get_colormap (BdkDrawable *drawable)
{
  BdkDrawableImplX11 *impl;

  impl = BDK_DRAWABLE_IMPL_X11 (drawable);

  return impl->colormap;
}

static void
bdk_x11_set_colormap (BdkDrawable *drawable,
                      BdkColormap *colormap)
{
  BdkDrawableImplX11 *impl;

  impl = BDK_DRAWABLE_IMPL_X11 (drawable);

  if (impl->colormap == colormap)
    return;
  
  if (impl->colormap)
    g_object_unref (impl->colormap);
  impl->colormap = colormap;
  if (impl->colormap)
    g_object_ref (impl->colormap);
}

/* Drawing
 */

static void
bdk_x11_draw_rectangle (BdkDrawable *drawable,
			BdkGC       *gc,
			gboolean     filled,
			gint         x,
			gint         y,
			gint         width,
			gint         height)
{
  BdkDrawableImplX11 *impl;

  impl = BDK_DRAWABLE_IMPL_X11 (drawable);
  
  if (filled)
    XFillRectangle (BDK_SCREEN_XDISPLAY (impl->screen), impl->xid,
		    BDK_GC_GET_XGC (gc), x, y, width, height);
  else
    XDrawRectangle (BDK_SCREEN_XDISPLAY (impl->screen), impl->xid,
		    BDK_GC_GET_XGC (gc), x, y, width, height);
}

static void
bdk_x11_draw_arc (BdkDrawable *drawable,
		  BdkGC       *gc,
		  gboolean     filled,
		  gint         x,
		  gint         y,
		  gint         width,
		  gint         height,
		  gint         angle1,
		  gint         angle2)
{
  BdkDrawableImplX11 *impl;

  impl = BDK_DRAWABLE_IMPL_X11 (drawable);

  
  if (filled)
    XFillArc (BDK_SCREEN_XDISPLAY (impl->screen), impl->xid,
	      BDK_GC_GET_XGC (gc), x, y, width, height, angle1, angle2);
  else
    XDrawArc (BDK_SCREEN_XDISPLAY (impl->screen), impl->xid,
	      BDK_GC_GET_XGC (gc), x, y, width, height, angle1, angle2);
}

static void
bdk_x11_draw_polygon (BdkDrawable *drawable,
		      BdkGC       *gc,
		      gboolean     filled,
		      BdkPoint    *points,
		      gint         npoints)
{
  XPoint *tmp_points;
  gint tmp_npoints, i;
  BdkDrawableImplX11 *impl;

  impl = BDK_DRAWABLE_IMPL_X11 (drawable);

  
  if (!filled &&
      (points[0].x != points[npoints-1].x || points[0].y != points[npoints-1].y))
    {
      tmp_npoints = npoints + 1;
      tmp_points = g_new (XPoint, tmp_npoints);
      tmp_points[npoints].x = points[0].x;
      tmp_points[npoints].y = points[0].y;
    }
  else
    {
      tmp_npoints = npoints;
      tmp_points = g_new (XPoint, tmp_npoints);
    }

  for (i=0; i<npoints; i++)
    {
      tmp_points[i].x = points[i].x;
      tmp_points[i].y = points[i].y;
    }
  
  if (filled)
    XFillPolygon (BDK_SCREEN_XDISPLAY (impl->screen), impl->xid,
		  BDK_GC_GET_XGC (gc), tmp_points, tmp_npoints, Complex, CoordModeOrigin);
  else
    XDrawLines (BDK_SCREEN_XDISPLAY (impl->screen), impl->xid,
		BDK_GC_GET_XGC (gc), tmp_points, tmp_npoints, CoordModeOrigin);

  g_free (tmp_points);
}

/* bdk_x11_draw_text
 *
 * Modified by Li-Da Lho to draw 16 bits and Multibyte strings
 *
 * Interface changed: add "BdkFont *font" to specify font or fontset explicitely
 */
static void
bdk_x11_draw_text (BdkDrawable *drawable,
		   BdkFont     *font,
		   BdkGC       *gc,
		   gint         x,
		   gint         y,
		   const gchar *text,
		   gint         text_length)
{
  BdkDrawableImplX11 *impl;
  Display *xdisplay;

  impl = BDK_DRAWABLE_IMPL_X11 (drawable);
  xdisplay = BDK_SCREEN_XDISPLAY (impl->screen);
  
  if (font->type == BDK_FONT_FONT)
    {
      XFontStruct *xfont = (XFontStruct *) BDK_FONT_XFONT (font);
      XSetFont(xdisplay, BDK_GC_GET_XGC (gc), xfont->fid);
      if ((xfont->min_byte1 == 0) && (xfont->max_byte1 == 0))
	{
	  XDrawString (xdisplay, impl->xid,
		       BDK_GC_GET_XGC (gc), x, y, text, text_length);
	}
      else
	{
	  XDrawString16 (xdisplay, impl->xid,
			 BDK_GC_GET_XGC (gc), x, y, (XChar2b *) text, text_length / 2);
	}
    }
  else if (font->type == BDK_FONT_FONTSET)
    {
      XFontSet fontset = (XFontSet) BDK_FONT_XFONT (font);
      XmbDrawString (xdisplay, impl->xid,
		     fontset, BDK_GC_GET_XGC (gc), x, y, text, text_length);
    }
  else
    g_error("undefined font type\n");
}

static void
bdk_x11_draw_text_wc (BdkDrawable    *drawable,
		      BdkFont	     *font,
		      BdkGC	     *gc,
		      gint	      x,
		      gint	      y,
		      const BdkWChar *text,
		      gint	      text_length)
{
  BdkDrawableImplX11 *impl;
  Display *xdisplay;

  impl = BDK_DRAWABLE_IMPL_X11 (drawable);
  xdisplay = BDK_SCREEN_XDISPLAY (impl->screen);
  
  if (font->type == BDK_FONT_FONT)
    {
      XFontStruct *xfont = (XFontStruct *) BDK_FONT_XFONT (font);
      gchar *text_8bit;
      gint i;
      XSetFont(xdisplay, BDK_GC_GET_XGC (gc), xfont->fid);
      text_8bit = g_new (gchar, text_length);
      for (i=0; i<text_length; i++) text_8bit[i] = text[i];
      XDrawString (xdisplay, impl->xid,
                   BDK_GC_GET_XGC (gc), x, y, text_8bit, text_length);
      g_free (text_8bit);
    }
  else if (font->type == BDK_FONT_FONTSET)
    {
      if (sizeof(BdkWChar) == sizeof(wchar_t))
	{
	  XwcDrawString (xdisplay, impl->xid,
			 (XFontSet) BDK_FONT_XFONT (font),
			 BDK_GC_GET_XGC (gc), x, y, (wchar_t *)text, text_length);
	}
      else
	{
	  wchar_t *text_wchar;
	  gint i;
	  text_wchar = g_new (wchar_t, text_length);
	  for (i=0; i<text_length; i++) text_wchar[i] = text[i];
	  XwcDrawString (xdisplay, impl->xid,
			 (XFontSet) BDK_FONT_XFONT (font),
			 BDK_GC_GET_XGC (gc), x, y, text_wchar, text_length);
	  g_free (text_wchar);
	}
    }
  else
    g_error("undefined font type\n");
}

static void
bdk_x11_draw_drawable (BdkDrawable *drawable,
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
  BdkDrawableImplX11 *impl;
  BdkDrawableImplX11 *src_impl;
  
  impl = BDK_DRAWABLE_IMPL_X11 (drawable);

  if (BDK_IS_DRAWABLE_IMPL_X11 (src))
    src_impl = BDK_DRAWABLE_IMPL_X11 (src);
  else if (BDK_IS_WINDOW (src))
    src_impl = BDK_DRAWABLE_IMPL_X11(((BdkWindowObject *)src)->impl);
  else
    src_impl = BDK_DRAWABLE_IMPL_X11(((BdkPixmapObject *)src)->impl);

  if (BDK_IS_WINDOW_IMPL_X11 (impl) &&
      BDK_IS_PIXMAP_IMPL_X11 (src_impl))
    {
      BdkPixmapImplX11 *src_pixmap = BDK_PIXMAP_IMPL_X11 (src_impl);
      /* Work around an Xserver bug where non-visible areas from
       * a pixmap to a window will clear the window background
       * in destination areas that are supposed to be clipped out.
       * This is a problem with client side windows as this means
       * things may draw outside the virtual windows. This could
       * also happen for window to window copies, but I don't
       * think we generate any calls like that.
       *
       * See: 
       * http://lists.freedesktop.org/archives/xorg/2009-February/043318.html
       */
      if (xsrc < 0)
	{
	  width += xsrc;
	  xdest -= xsrc;
	  xsrc = 0;
	}
      
      if (ysrc < 0)
	{
	  height += ysrc;
	  ydest -= ysrc;
	  ysrc = 0;
	}

      if (xsrc + width > src_pixmap->width)
	width = src_pixmap->width - xsrc;
      if (ysrc + height > src_pixmap->height)
	height = src_pixmap->height - ysrc;
    }
  
  if (src_depth == 1)
    {
      XCopyArea (BDK_SCREEN_XDISPLAY (impl->screen),
                 src_impl->xid,
		 impl->xid,
		 BDK_GC_GET_XGC (gc),
		 xsrc, ysrc,
		 width, height,
		 xdest, ydest);
    }
  else if (dest_depth != 0 && src_depth == dest_depth)
    {
      XCopyArea (BDK_SCREEN_XDISPLAY (impl->screen),
                 src_impl->xid,
		 impl->xid,
		 BDK_GC_GET_XGC (gc),
		 xsrc, ysrc,
		 width, height,
		 xdest, ydest);
    }
  else
    g_warning ("Attempt to draw a drawable with depth %d to a drawable with depth %d",
               src_depth, dest_depth);
}

static void
bdk_x11_draw_points (BdkDrawable *drawable,
		     BdkGC       *gc,
		     BdkPoint    *points,
		     gint         npoints)
{
  BdkDrawableImplX11 *impl;

  impl = BDK_DRAWABLE_IMPL_X11 (drawable);

  
  /* We special-case npoints == 1, because X will merge multiple
   * consecutive XDrawPoint requests into a PolyPoint request
   */
  if (npoints == 1)
    {
      XDrawPoint (BDK_SCREEN_XDISPLAY (impl->screen),
		  impl->xid,
		  BDK_GC_GET_XGC (gc),
		  points[0].x, points[0].y);
    }
  else
    {
      gint i;
      XPoint *tmp_points = g_new (XPoint, npoints);

      for (i=0; i<npoints; i++)
	{
	  tmp_points[i].x = points[i].x;
	  tmp_points[i].y = points[i].y;
	}
      
      XDrawPoints (BDK_SCREEN_XDISPLAY (impl->screen),
		   impl->xid,
		   BDK_GC_GET_XGC (gc),
		   tmp_points,
		   npoints,
		   CoordModeOrigin);

      g_free (tmp_points);
    }
}

static void
bdk_x11_draw_segments (BdkDrawable *drawable,
		       BdkGC       *gc,
		       BdkSegment  *segs,
		       gint         nsegs)
{
  BdkDrawableImplX11 *impl;

  impl = BDK_DRAWABLE_IMPL_X11 (drawable);

  
  /* We special-case nsegs == 1, because X will merge multiple
   * consecutive XDrawLine requests into a PolySegment request
   */
  if (nsegs == 1)
    {
      XDrawLine (BDK_SCREEN_XDISPLAY (impl->screen), impl->xid,
		 BDK_GC_GET_XGC (gc), segs[0].x1, segs[0].y1,
		 segs[0].x2, segs[0].y2);
    }
  else
    {
      gint i;
      XSegment *tmp_segs = g_new (XSegment, nsegs);

      for (i=0; i<nsegs; i++)
	{
	  tmp_segs[i].x1 = segs[i].x1;
	  tmp_segs[i].x2 = segs[i].x2;
	  tmp_segs[i].y1 = segs[i].y1;
	  tmp_segs[i].y2 = segs[i].y2;
	}
      
      XDrawSegments (BDK_SCREEN_XDISPLAY (impl->screen),
		     impl->xid,
		     BDK_GC_GET_XGC (gc),
		     tmp_segs, nsegs);

      g_free (tmp_segs);
    }
}

static void
bdk_x11_draw_lines (BdkDrawable *drawable,
		    BdkGC       *gc,
		    BdkPoint    *points,
		    gint         npoints)
{
  gint i;
  XPoint *tmp_points = g_new (XPoint, npoints);
  BdkDrawableImplX11 *impl;

  impl = BDK_DRAWABLE_IMPL_X11 (drawable);

  
  for (i=0; i<npoints; i++)
    {
      tmp_points[i].x = points[i].x;
      tmp_points[i].y = points[i].y;
    }
      
  XDrawLines (BDK_SCREEN_XDISPLAY (impl->screen),
	      impl->xid,
	      BDK_GC_GET_XGC (gc),
	      tmp_points, npoints,
	      CoordModeOrigin);

  g_free (tmp_points);
}

static void
bdk_x11_draw_image     (BdkDrawable     *drawable,
                        BdkGC           *gc,
                        BdkImage        *image,
                        gint             xsrc,
                        gint             ysrc,
                        gint             xdest,
                        gint             ydest,
                        gint             width,
                        gint             height)
{
  BdkDrawableImplX11 *impl;

  impl = BDK_DRAWABLE_IMPL_X11 (drawable);

#ifdef USE_SHM  
  if (image->type == BDK_IMAGE_SHARED)
    XShmPutImage (BDK_SCREEN_XDISPLAY (impl->screen), impl->xid,
                  BDK_GC_GET_XGC (gc), BDK_IMAGE_XIMAGE (image),
                  xsrc, ysrc, xdest, ydest, width, height, False);
  else
#endif
    XPutImage (BDK_SCREEN_XDISPLAY (impl->screen), impl->xid,
               BDK_GC_GET_XGC (gc), BDK_IMAGE_XIMAGE (image),
               xsrc, ysrc, xdest, ydest, width, height);
}

static gint
bdk_x11_get_depth (BdkDrawable *drawable)
{
  /* This is a bit bogus but I'm not sure the other way is better */

  return bdk_drawable_get_depth (BDK_DRAWABLE_IMPL_X11 (drawable)->wrapper);
}

static BdkDrawable *
get_impl_drawable (BdkDrawable *drawable)
{
  if (BDK_IS_WINDOW (drawable))
    return ((BdkWindowObject *)drawable)->impl;
  else if (BDK_IS_PIXMAP (drawable))
    return ((BdkPixmapObject *)drawable)->impl;
  else
    {
      g_warning (B_STRLOC " drawable is not a pixmap or window");
      return NULL;
    }
}

static BdkScreen*
bdk_x11_get_screen (BdkDrawable *drawable)
{
  if (BDK_IS_DRAWABLE_IMPL_X11 (drawable))
    return BDK_DRAWABLE_IMPL_X11 (drawable)->screen;
  else
    return BDK_DRAWABLE_IMPL_X11 (get_impl_drawable (drawable))->screen;
}

static BdkVisual*
bdk_x11_get_visual (BdkDrawable    *drawable)
{
  return bdk_drawable_get_visual (BDK_DRAWABLE_IMPL_X11 (drawable)->wrapper);
}

/**
 * bdk_x11_drawable_get_xdisplay:
 * @drawable: a #BdkDrawable.
 * 
 * Returns the display of a #BdkDrawable.
 * 
 * Return value: an Xlib <type>Display*</type>.
 **/
Display *
bdk_x11_drawable_get_xdisplay (BdkDrawable *drawable)
{
  if (BDK_IS_DRAWABLE_IMPL_X11 (drawable))
    return BDK_SCREEN_XDISPLAY (BDK_DRAWABLE_IMPL_X11 (drawable)->screen);
  else
    return BDK_SCREEN_XDISPLAY (BDK_DRAWABLE_IMPL_X11 (get_impl_drawable (drawable))->screen);
}

/**
 * bdk_x11_drawable_get_xid:
 * @drawable: a #BdkDrawable.
 * 
 * Returns the X resource (window or pixmap) belonging to a #BdkDrawable.
 * 
 * Return value: the ID of @drawable's X resource.
 **/
XID
bdk_x11_drawable_get_xid (BdkDrawable *drawable)
{
  BdkDrawable *impl;
  
  if (BDK_IS_WINDOW (drawable))
    {
      BdkWindow *window = (BdkWindow *)drawable;
      
      /* Try to ensure the window has a native window */
      if (!_bdk_window_has_impl (window))
	{
	  bdk_window_ensure_native (window);

	  /* We sync here to ensure the window is created in the Xserver when
	   * this function returns. This is required because the returned XID
	   * for this window must be valid immediately, even with another
	   * connection to the Xserver */
	  bdk_display_sync (bdk_drawable_get_display (window));
	}
      
      if (!BDK_WINDOW_IS_X11 (window))
        {
          g_warning (B_STRLOC " drawable is not a native X11 window");
          return None;
        }
      
      impl = ((BdkWindowObject *)drawable)->impl;
    }
  else if (BDK_IS_PIXMAP (drawable))
    impl = ((BdkPixmapObject *)drawable)->impl;
  else
    {
      g_warning (B_STRLOC " drawable is not a pixmap or window");
      return None;
    }

  return ((BdkDrawableImplX11 *)impl)->xid;
}

BdkDrawable *
bdk_x11_window_get_drawable_impl (BdkWindow *window)
{
  return ((BdkWindowObject *)window)->impl;
}
BdkDrawable *
bdk_x11_pixmap_get_drawable_impl (BdkPixmap *pixmap)
{
  return ((BdkPixmapObject *)pixmap)->impl;
}

/* Code for accelerated alpha compositing using the RENDER extension.
 * It's a bit long because there are lots of possibilities for
 * what's the fastest depending on the available picture formats,
 * whether we can used shared pixmaps, etc.
 */

static BdkX11FormatType
select_format (BdkDisplay         *display,
	       XRenderPictFormat **format,
	       XRenderPictFormat **mask)
{
  Display *xdisplay = BDK_DISPLAY_XDISPLAY (display);
  XRenderPictFormat pf;

  if (!_bdk_x11_have_render (display))
    return BDK_X11_FORMAT_NONE;
  
  /* Look for a 32-bit xRGB and Axxx formats that exactly match the
   * in memory data format. We can use them as pixmap and mask
   * to deal with non-premultiplied data.
   */

  pf.type = PictTypeDirect;
  pf.depth = 32;
  pf.direct.redMask = 0xff;
  pf.direct.greenMask = 0xff;
  pf.direct.blueMask = 0xff;
  
  pf.direct.alphaMask = 0;
  if (ImageByteOrder (xdisplay) == LSBFirst)
    {
      /* ABGR */
      pf.direct.red = 0;
      pf.direct.green = 8;
      pf.direct.blue = 16;
    }
  else
    {
      /* RGBA */
      pf.direct.red = 24;
      pf.direct.green = 16;
      pf.direct.blue = 8;
    }
  
  *format = XRenderFindFormat (xdisplay,
			       (PictFormatType | PictFormatDepth |
				PictFormatRedMask | PictFormatRed |
				PictFormatGreenMask | PictFormatGreen |
				PictFormatBlueMask | PictFormatBlue |
				PictFormatAlphaMask),
			       &pf,
			       0);

  pf.direct.alphaMask = 0xff;
  if (ImageByteOrder (xdisplay) == LSBFirst)
    {
      /* ABGR */
      pf.direct.alpha = 24;
    }
  else
    {
      pf.direct.alpha = 0;
    }
  
  *mask = XRenderFindFormat (xdisplay,
			     (PictFormatType | PictFormatDepth |
			      PictFormatAlphaMask | PictFormatAlpha),
			     &pf,
			     0);

  if (*format && *mask)
    return BDK_X11_FORMAT_EXACT_MASK;

  /* OK, that failed, now look for xRGB and Axxx formats in
   * RENDER's preferred order
   */
  pf.direct.alphaMask = 0;
  /* ARGB */
  pf.direct.red = 16;
  pf.direct.green = 8;
  pf.direct.blue = 0;
  
  *format = XRenderFindFormat (xdisplay,
			       (PictFormatType | PictFormatDepth |
				PictFormatRedMask | PictFormatRed |
				PictFormatGreenMask | PictFormatGreen |
				PictFormatBlueMask | PictFormatBlue |
				PictFormatAlphaMask),
			       &pf,
			       0);

  pf.direct.alphaMask = 0xff;
  pf.direct.alpha = 24;
  
  *mask = XRenderFindFormat (xdisplay,
			     (PictFormatType | PictFormatDepth |
			      PictFormatAlphaMask | PictFormatAlpha),
			     &pf,
			     0);

  if (*format && *mask)
    return BDK_X11_FORMAT_ARGB_MASK;

  /* Finally, if neither of the above worked, fall back to
   * looking for combined ARGB -- we'll premultiply ourselves.
   */

  pf.type = PictTypeDirect;
  pf.depth = 32;
  pf.direct.red = 16;
  pf.direct.green = 8;
  pf.direct.blue = 0;
  pf.direct.alphaMask = 0xff;
  pf.direct.alpha = 24;

  *format = XRenderFindFormat (xdisplay,
			       (PictFormatType | PictFormatDepth |
				PictFormatRedMask | PictFormatRed |
				PictFormatGreenMask | PictFormatGreen |
				PictFormatBlueMask | PictFormatBlue |
				PictFormatAlphaMask | PictFormatAlpha),
			       &pf,
			       0);
  *mask = NULL;

  if (*format)
    return BDK_X11_FORMAT_ARGB;

  return BDK_X11_FORMAT_NONE;
}

#if 0
static void
list_formats (XRenderPictFormat *pf)
{
  gint i;
  
  for (i=0 ;; i++)
    {
      XRenderPictFormat *pf = XRenderFindFormat (impl->xdisplay, 0, NULL, i);
      if (pf)
	{
	  g_print ("%2d R-%#06x/%#06x G-%#06x/%#06x B-%#06x/%#06x A-%#06x/%#06x\n",
		   pf->depth,
		   pf->direct.red,
		   pf->direct.redMask,
		   pf->direct.green,
		   pf->direct.greenMask,
		   pf->direct.blue,
		   pf->direct.blueMask,
		   pf->direct.alpha,
		   pf->direct.alphaMask);
	}
      else
	break;
    }
}
#endif  

void
_bdk_x11_convert_to_format (guchar           *src_buf,
                            gint              src_rowstride,
                            guchar           *dest_buf,
                            gint              dest_rowstride,
                            BdkX11FormatType  dest_format,
                            BdkByteOrder      dest_byteorder,
                            gint              width,
                            gint              height)
{
  gint i;

  for (i=0; i < height; i++)
    {
      switch (dest_format)
	{
	case BDK_X11_FORMAT_EXACT_MASK:
	  {
	    memcpy (dest_buf + i * dest_rowstride,
		    src_buf + i * src_rowstride,
		    width * 4);
	    break;
	  }
	case BDK_X11_FORMAT_ARGB_MASK:
	  {
	    guchar *row = src_buf + i * src_rowstride;
	    if (((gsize)row & 3) != 0)
	      {
		guchar *p = row;
		guint32 *q = (guint32 *)(dest_buf + i * dest_rowstride);
		guchar *end = p + 4 * width;

		while (p < end)
		  {
		    *q = (p[3] << 24) | (p[0] << 16) | (p[1] << 8) | p[2];
		    p += 4;
		    q++;
		  }
	      }
	    else
	      {
		guint32 *p = (guint32 *)row;
		guint32 *q = (guint32 *)(dest_buf + i * dest_rowstride);
		guint32 *end = p + width;

#if G_BYTE_ORDER == G_LITTLE_ENDIAN	    
		if (dest_byteorder == BDK_LSB_FIRST)
		  {
		    /* ABGR => ARGB */
		
		    while (p < end)
		      {
			*q = ( (*p & 0xff00ff00) |
			       ((*p & 0x000000ff) << 16) |
			       ((*p & 0x00ff0000) >> 16));
			q++;
			p++;
		      }
		  }
		else
		  {
		    /* ABGR => BGRA */
		
		    while (p < end)
		      {
			*q = (((*p & 0xff000000) >> 24) |
			      ((*p & 0x00ffffff) << 8));
			q++;
			p++;
		      }
		  }
#else /* G_BYTE_ORDER == G_BIG_ENDIAN */
		if (dest_byteorder == BDK_LSB_FIRST)
		  {
		    /* RGBA => BGRA */
		
		    while (p < end)
		      {
			*q = ( (*p & 0x00ff00ff) |
			       ((*p & 0x0000ff00) << 16) |
			       ((*p & 0xff000000) >> 16));
			q++;
			p++;
		      }
		  }
		else
		  {
		    /* RGBA => ARGB */
		
		    while (p < end)
		      {
			*q = (((*p & 0xffffff00) >> 8) |
			      ((*p & 0x000000ff) << 24));
			q++;
			p++;
		      }
		  }
#endif /* G_BYTE_ORDER*/	    
	      }
	    break;
	  }
	case BDK_X11_FORMAT_ARGB:
	  {
	    guchar *p = (src_buf + i * src_rowstride);
	    guchar *q = (dest_buf + i * dest_rowstride);
	    guchar *end = p + 4 * width;
	    guint t1,t2,t3;
	    
#define MULT(d,c,a,t) B_STMT_START { t = c * a; d = ((t >> 8) + t) >> 8; } B_STMT_END
	    
	    if (dest_byteorder == BDK_LSB_FIRST)
	      {
		while (p < end)
		  {
		    MULT(q[0], p[2], p[3], t1);
		    MULT(q[1], p[1], p[3], t2);
		    MULT(q[2], p[0], p[3], t3);
		    q[3] = p[3];
		    p += 4;
		    q += 4;
		  }
	      }
	    else
	      {
		while (p < end)
		  {
		    q[0] = p[3];
		    MULT(q[1], p[0], p[3], t1);
		    MULT(q[2], p[1], p[3], t2);
		    MULT(q[3], p[2], p[3], t3);
		    p += 4;
		    q += 4;
		  }
	      }
#undef MULT
	    break;
	  }
	case BDK_X11_FORMAT_NONE:
	  g_assert_not_reached ();
	  break;
	}
    }
}

static void
draw_with_images (BdkDrawable       *drawable,
		  BdkGC             *gc,
		  BdkX11FormatType   format_type,
		  XRenderPictFormat *format,
		  XRenderPictFormat *mask_format,
		  guchar            *src_rgb,
		  gint               src_rowstride,
		  gint               dest_x,
		  gint               dest_y,
		  gint               width,
		  gint               height)
{
  BdkScreen *screen = BDK_DRAWABLE_IMPL_X11 (drawable)->screen;
  Display *xdisplay = BDK_SCREEN_XDISPLAY (screen);
  BdkImage *image;
  BdkPixmap *pix;
  BdkGC *pix_gc;
  Picture pict;
  Picture dest_pict;
  Picture mask = None;
  gint x0, y0;

  pix = bdk_pixmap_new (bdk_screen_get_root_window (screen), width, height, 32);
						  
  pict = XRenderCreatePicture (xdisplay, 
			       BDK_PIXMAP_XID (pix),
			       format, 0, NULL);
  if (mask_format)
    mask = XRenderCreatePicture (xdisplay, 
				 BDK_PIXMAP_XID (pix),
				 mask_format, 0, NULL);

  dest_pict = bdk_x11_drawable_get_picture (drawable);  
  
  pix_gc = _bdk_drawable_get_scratch_gc (pix, FALSE);

  for (y0 = 0; y0 < height; y0 += BDK_SCRATCH_IMAGE_HEIGHT)
    {
      gint height1 = MIN (height - y0, BDK_SCRATCH_IMAGE_HEIGHT);
      for (x0 = 0; x0 < width; x0 += BDK_SCRATCH_IMAGE_WIDTH)
	{
	  gint xs0, ys0;
	  
	  gint width1 = MIN (width - x0, BDK_SCRATCH_IMAGE_WIDTH);
	  
	  image = _bdk_image_get_scratch (screen, width1, height1, 32, &xs0, &ys0);
	  
	  _bdk_x11_convert_to_format (src_rgb + y0 * src_rowstride + 4 * x0, src_rowstride,
                                      (guchar *)image->mem + ys0 * image->bpl + xs0 * image->bpp, image->bpl,
                                      format_type, image->byte_order, 
                                      width1, height1);

	  bdk_draw_image (pix, pix_gc,
			  image, xs0, ys0, x0, y0, width1, height1);
	}
    }
  
  XRenderComposite (xdisplay, PictOpOver, pict, mask, dest_pict, 
		    0, 0, 0, 0, dest_x, dest_y, width, height);

  XRenderFreePicture (xdisplay, pict);
  if (mask)
    XRenderFreePicture (xdisplay, mask);
  
  g_object_unref (pix);
}

typedef struct _ShmPixmapInfo ShmPixmapInfo;

struct _ShmPixmapInfo
{
  Display  *display;
  Pixmap    pix;
  Picture   pict;
  Picture   mask;
};

static void
shm_pixmap_info_destroy (gpointer data)
{
  ShmPixmapInfo *info = data;

  if (info->pict != None)
    XRenderFreePicture (info->display, info->pict);
  if (info->mask != None)
    XRenderFreePicture (info->display, info->mask);

  g_free (data);
}


#ifdef USE_SHM
/* Returns FALSE if we can't get a shm pixmap */
static gboolean
get_shm_pixmap_for_image (Display           *xdisplay,
			  BdkImage          *image,
			  XRenderPictFormat *format,
			  XRenderPictFormat *mask_format,
			  Pixmap            *pix,
			  Picture           *pict,
			  Picture           *mask)
{
  ShmPixmapInfo *info;
  
  if (image->type != BDK_IMAGE_SHARED)
    return FALSE;
  
  info = g_object_get_data (G_OBJECT (image), "bdk-x11-shm-pixmap");
  if (!info)
    {
      *pix = _bdk_x11_image_get_shm_pixmap (image);
      
      if (!*pix)
	return FALSE;
      
      info = g_new (ShmPixmapInfo, 1);
      info->display = xdisplay;
      info->pix = *pix;
      
      info->pict = XRenderCreatePicture (xdisplay, info->pix,
					 format, 0, NULL);
      if (mask_format)
	info->mask = XRenderCreatePicture (xdisplay, info->pix,
					   mask_format, 0, NULL);
      else
	info->mask = None;

      g_object_set_data_full (G_OBJECT (image), "bdk-x11-shm-pixmap", info,
	  shm_pixmap_info_destroy);
    }

  *pix = info->pix;
  *pict = info->pict;
  *mask = info->mask;

  return TRUE;
}

/* Returns FALSE if drawing with ShmPixmaps is not possible */
static gboolean
draw_with_pixmaps (BdkDrawable       *drawable,
		   BdkGC             *gc,
		   BdkX11FormatType   format_type,
		   XRenderPictFormat *format,
		   XRenderPictFormat *mask_format,
		   guchar            *src_rgb,
		   gint               src_rowstride,
		   gint               dest_x,
		   gint               dest_y,
		   gint               width,
		   gint               height)
{
  Display *xdisplay = BDK_SCREEN_XDISPLAY (BDK_DRAWABLE_IMPL_X11 (drawable)->screen);
  BdkImage *image;
  Pixmap pix;
  Picture pict;
  Picture dest_pict;
  Picture mask = None;
  gint x0, y0;

  dest_pict = bdk_x11_drawable_get_picture (drawable);
  
  for (y0 = 0; y0 < height; y0 += BDK_SCRATCH_IMAGE_HEIGHT)
    {
      gint height1 = MIN (height - y0, BDK_SCRATCH_IMAGE_HEIGHT);
      for (x0 = 0; x0 < width; x0 += BDK_SCRATCH_IMAGE_WIDTH)
	{
	  gint xs0, ys0;
	  
	  gint width1 = MIN (width - x0, BDK_SCRATCH_IMAGE_WIDTH);
	  
	  image = _bdk_image_get_scratch (BDK_DRAWABLE_IMPL_X11 (drawable)->screen,
					  width1, height1, 32, &xs0, &ys0);
	  if (!get_shm_pixmap_for_image (xdisplay, image, format, mask_format, &pix, &pict, &mask))
	    return FALSE;

	  _bdk_x11_convert_to_format (src_rgb + y0 * src_rowstride + 4 * x0, src_rowstride,
                                      (guchar *)image->mem + ys0 * image->bpl + xs0 * image->bpp, image->bpl,
                                      format_type, image->byte_order, 
                                      width1, height1);

	  XRenderComposite (xdisplay, PictOpOver, pict, mask, dest_pict, 
			    xs0, ys0, xs0, ys0, x0 + dest_x, y0 + dest_y,
			    width1, height1);
	}
    }

  return TRUE;
}
#endif

static void
bdk_x11_draw_pixbuf (BdkDrawable     *drawable,
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
  BdkX11FormatType format_type;
  XRenderPictFormat *format, *mask_format;
  gint rowstride;
#ifdef USE_SHM  
  gboolean use_pixmaps = TRUE;
#endif /* USE_SHM */
    
  format_type = select_format (bdk_drawable_get_display (drawable),
			       &format, &mask_format);

  if (format_type == BDK_X11_FORMAT_NONE ||
      !bdk_pixbuf_get_has_alpha (pixbuf) ||
      bdk_drawable_get_depth (drawable) == 1 ||
      (dither == BDK_RGB_DITHER_MAX && bdk_drawable_get_depth (drawable) != 24) ||
      bdk_x11_drawable_get_picture (drawable) == None)
    {
      BdkDrawable *wrapper = BDK_DRAWABLE_IMPL_X11 (drawable)->wrapper;
      BDK_DRAWABLE_CLASS (_bdk_drawable_impl_x11_parent_class)->draw_pixbuf (wrapper, gc, pixbuf,
									     src_x, src_y, dest_x, dest_y,
									     width, height,
									     dither, x_dither, y_dither);
      return;
    }

  bdk_x11_drawable_update_picture_clip (drawable, gc);

  rowstride = bdk_pixbuf_get_rowstride (pixbuf);

#ifdef USE_SHM
  if (use_pixmaps)
    {
      if (!draw_with_pixmaps (drawable, gc,
			      format_type, format, mask_format,
			      bdk_pixbuf_get_pixels (pixbuf) + src_y * rowstride + src_x * 4,
			      rowstride,
			      dest_x, dest_y, width, height))
	use_pixmaps = FALSE;
    }

  if (!use_pixmaps)
#endif /* USE_SHM */
    draw_with_images (drawable, gc,
		      format_type, format, mask_format,
		      bdk_pixbuf_get_pixels (pixbuf) + src_y * rowstride + src_x * 4,
		      rowstride,
		      dest_x, dest_y, width, height);
}

static void
bdk_x11_bairo_surface_destroy (void *data)
{
  BdkDrawableImplX11 *impl = data;

  impl->bairo_surface = NULL;
}

void
_bdk_windowing_set_bairo_surface_size (bairo_surface_t *surface,
				       int width,
				       int height)
{
  bairo_xlib_surface_set_size (surface, width, height);
}

bairo_surface_t *
_bdk_windowing_create_bairo_surface (BdkDrawable *drawable,
				     int width,
				     int height)
{
  BdkDrawableImplX11 *impl = BDK_DRAWABLE_IMPL_X11 (drawable);
  BdkVisual *visual;
    
  visual = bdk_drawable_get_visual (drawable);
  if (visual) 
    return bairo_xlib_surface_create (BDK_SCREEN_XDISPLAY (impl->screen),
				      impl->xid,
				      BDK_VISUAL_XVISUAL (visual),
				      width, height);
  else if (bdk_drawable_get_depth (drawable) == 1)
    return bairo_xlib_surface_create_for_bitmap (BDK_SCREEN_XDISPLAY (impl->screen),
						    impl->xid,
						    BDK_SCREEN_XSCREEN (impl->screen),
						    width, height);
  else
    {
      g_warning ("Using Bairo rendering requires the drawable argument to\n"
		 "have a specified colormap. All windows have a colormap,\n"
		 "however, pixmaps only have colormap by default if they\n"
		 "were created with a non-NULL window argument. Otherwise\n"
		 "a colormap must be set on them with bdk_drawable_set_colormap");
      return NULL;
    }
  
}

static bairo_surface_t *
bdk_x11_ref_bairo_surface (BdkDrawable *drawable)
{
  BdkDrawableImplX11 *impl = BDK_DRAWABLE_IMPL_X11 (drawable);

  if (BDK_IS_WINDOW_IMPL_X11 (drawable) &&
      BDK_WINDOW_DESTROYED (impl->wrapper))
    return NULL;

  if (!impl->bairo_surface)
    {
      int width, height;
  
      bdk_drawable_get_size (impl->wrapper, &width, &height);

      impl->bairo_surface = _bdk_windowing_create_bairo_surface (drawable, width, height);
      
      if (impl->bairo_surface)
	bairo_surface_set_user_data (impl->bairo_surface, &bdk_x11_bairo_key,
				     drawable, bdk_x11_bairo_surface_destroy);
    }
  else
    bairo_surface_reference (impl->bairo_surface);

  return impl->bairo_surface;
}

#define __BDK_DRAWABLE_X11_C__
#include "bdkaliasdef.c"
