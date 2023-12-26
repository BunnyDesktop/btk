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
 * Modified by the BTK+ Team and others 1997-2005.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#include <config.h>
#include <math.h>
#include "bdk.h"
#include "bdkwindow.h"
#include "bdkinternals.h"
#include "bdkwindowimpl.h"
#include "bdkpixmap.h"
#include "bdkdrawable.h"
#include "bdktypes.h"
#include "bdkscreen.h"
#include "bdkgc.h"
#include "bdkcolor.h"
#include "bdkcursor.h"
#include "bdkalias.h"

/* LIMITATIONS:
 *
 * Offscreen windows can't be the child of a foreign window,
 *   nor contain foreign windows
 * BDK_POINTER_MOTION_HINT_MASK isn't effective
 */

typedef struct _BdkOffscreenWindowClass BdkOffscreenWindowClass;

struct _BdkOffscreenWindow
{
  BdkDrawable parent_instance;

  BdkWindow *wrapper;
  BdkCursor *cursor;
  BdkColormap *colormap;
  BdkScreen *screen;

  BdkPixmap *pixmap;
  BdkWindow *embedder;
};

struct _BdkOffscreenWindowClass
{
  BdkDrawableClass parent_class;
};

#define BDK_OFFSCREEN_WINDOW_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_OFFSCREEN_WINDOW, BdkOffscreenWindowClass))
#define BDK_IS_OFFSCREEN_WINDOW_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_OFFSCREEN_WINDOW))
#define BDK_OFFSCREEN_WINDOW_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_OFFSCREEN_WINDOW, BdkOffscreenWindowClass))

static void       bdk_offscreen_window_impl_iface_init    (BdkWindowImplIface         *iface);
static void       bdk_offscreen_window_hide               (BdkWindow                  *window);

G_DEFINE_TYPE_WITH_CODE (BdkOffscreenWindow,
			 bdk_offscreen_window,
			 BDK_TYPE_DRAWABLE,
			 G_IMPLEMENT_INTERFACE (BDK_TYPE_WINDOW_IMPL,
						bdk_offscreen_window_impl_iface_init));


static void
bdk_offscreen_window_finalize (BObject *object)
{
  BdkOffscreenWindow *offscreen = BDK_OFFSCREEN_WINDOW (object);

  if (offscreen->cursor)
    bdk_cursor_unref (offscreen->cursor);

  offscreen->cursor = NULL;

  g_object_unref (offscreen->pixmap);

  B_OBJECT_CLASS (bdk_offscreen_window_parent_class)->finalize (object);
}

static void
bdk_offscreen_window_init (BdkOffscreenWindow *window)
{
}

static void
bdk_offscreen_window_destroy (BdkWindow *window,
			      gboolean   recursing,
			      gboolean   foreign_destroy)
{
  BdkWindowObject *private = BDK_WINDOW_OBJECT (window);
  BdkOffscreenWindow *offscreen;

  offscreen = BDK_OFFSCREEN_WINDOW (private->impl);

  bdk_offscreen_window_set_embedder (window, NULL);
  
  if (!recursing)
    bdk_offscreen_window_hide (window);

  g_object_unref (offscreen->colormap);
  offscreen->colormap = NULL;
}

static gboolean
is_parent_of (BdkWindow *parent,
	      BdkWindow *child)
{
  BdkWindow *w;

  w = child;
  while (w != NULL)
    {
      if (w == parent)
	return TRUE;

      w = bdk_window_get_parent (w);
    }

  return FALSE;
}

static BdkGC *
bdk_offscreen_window_create_gc (BdkDrawable     *drawable,
				BdkGCValues     *values,
				BdkGCValuesMask  values_mask)
{
  BdkOffscreenWindow *offscreen = BDK_OFFSCREEN_WINDOW (drawable);

  return bdk_gc_new_with_values (offscreen->pixmap, values, values_mask);
}

static BdkImage*
bdk_offscreen_window_copy_to_image (BdkDrawable    *drawable,
				    BdkImage       *image,
				    gint            src_x,
				    gint            src_y,
				    gint            dest_x,
				    gint            dest_y,
				    gint            width,
				    gint            height)
{
  BdkOffscreenWindow *offscreen = BDK_OFFSCREEN_WINDOW (drawable);

  return bdk_drawable_copy_to_image (offscreen->pixmap,
				     image,
				     src_x,
				     src_y,
				     dest_x, dest_y,
				     width, height);
}

static bairo_surface_t *
bdk_offscreen_window_ref_bairo_surface (BdkDrawable *drawable)
{
  BdkOffscreenWindow *offscreen = BDK_OFFSCREEN_WINDOW (drawable);

  return _bdk_drawable_ref_bairo_surface (offscreen->pixmap);
}

static BdkColormap*
bdk_offscreen_window_get_colormap (BdkDrawable *drawable)
{
  BdkOffscreenWindow *offscreen = BDK_OFFSCREEN_WINDOW (drawable);

  return offscreen->colormap;
}

static void
bdk_offscreen_window_set_colormap (BdkDrawable *drawable,
				   BdkColormap*colormap)
{
  BdkOffscreenWindow *offscreen = BDK_OFFSCREEN_WINDOW (drawable);

  if (colormap && BDK_WINDOW_DESTROYED (offscreen->wrapper))
    return;

  if (offscreen->colormap == colormap)
    return;

  if (offscreen->colormap)
    g_object_unref (offscreen->colormap);

  offscreen->colormap = colormap;
  if (offscreen->colormap)
    g_object_ref (offscreen->colormap);
}


static gint
bdk_offscreen_window_get_depth (BdkDrawable *drawable)
{
  BdkOffscreenWindow *offscreen = BDK_OFFSCREEN_WINDOW (drawable);

  return bdk_drawable_get_depth (offscreen->wrapper);
}

static BdkDrawable *
bdk_offscreen_window_get_source_drawable (BdkDrawable  *drawable)
{
  BdkOffscreenWindow *offscreen = BDK_OFFSCREEN_WINDOW (drawable);

  return _bdk_drawable_get_source_drawable (offscreen->pixmap);
}

static BdkDrawable *
bdk_offscreen_window_get_composite_drawable (BdkDrawable *drawable,
					     gint         x,
					     gint         y,
					     gint         width,
					     gint         height,
					     gint        *composite_x_offset,
					     gint        *composite_y_offset)
{
  BdkOffscreenWindow *offscreen = BDK_OFFSCREEN_WINDOW (drawable);

  return g_object_ref (offscreen->pixmap);
}

static BdkScreen*
bdk_offscreen_window_get_screen (BdkDrawable *drawable)
{
  BdkOffscreenWindow *offscreen = BDK_OFFSCREEN_WINDOW (drawable);

  return offscreen->screen;
}

static BdkVisual*
bdk_offscreen_window_get_visual (BdkDrawable    *drawable)
{
  BdkOffscreenWindow *offscreen = BDK_OFFSCREEN_WINDOW (drawable);

  return bdk_drawable_get_visual (offscreen->wrapper);
}

static void
add_damage (BdkOffscreenWindow *offscreen,
	    int x, int y,
	    int w, int h,
	    gboolean is_line)
{
  BdkRectangle rect;
  BdkRebunnyion *damage;

  rect.x = x;
  rect.y = y;
  rect.width = w;
  rect.height = h;

  if (is_line)
    {
      /* This should really take into account line width, line
       * joins (and miter) and line caps. But these are hard
       * to compute, rarely used and generally a pain. And in
       * the end a snug damage rectangle is not that important
       * as multiple damages are generally created anyway.
       *
       * So, we just add some padding around the rect.
       * We use a padding of 3 pixels, plus an extra row
       * below and on the right for the normal line size. I.E.
       * line from (0,0) to (2,0) gets h=0 but is really
       * at least one pixel tall.
       */
      rect.x -= 3;
      rect.y -= 3;
      rect.width += 7;
      rect.height += 7;
    }

  damage = bdk_rebunnyion_rectangle (&rect);
  _bdk_window_add_damage (offscreen->wrapper, damage);
  bdk_rebunnyion_destroy (damage);
}

static BdkDrawable *
get_real_drawable (BdkOffscreenWindow *offscreen)
{
  BdkPixmapObject *pixmap;
  pixmap = (BdkPixmapObject *) offscreen->pixmap;
  return BDK_DRAWABLE (pixmap->impl);
}

static void
bdk_offscreen_window_draw_drawable (BdkDrawable *drawable,
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
  BdkOffscreenWindow *offscreen = BDK_OFFSCREEN_WINDOW (drawable);
  BdkDrawable *real_drawable = get_real_drawable (offscreen);

  bdk_draw_drawable (real_drawable, gc,
		     src, xsrc, ysrc,
		     xdest, ydest,
		     width, height);

  add_damage (offscreen, xdest, ydest, width, height, FALSE);
}

static void
bdk_offscreen_window_draw_rectangle (BdkDrawable  *drawable,
				     BdkGC	  *gc,
				     gboolean	   filled,
				     gint	   x,
				     gint	   y,
				     gint	   width,
				     gint	   height)
{
  BdkOffscreenWindow *offscreen = BDK_OFFSCREEN_WINDOW (drawable);
  BdkDrawable *real_drawable = get_real_drawable (offscreen);

  bdk_draw_rectangle (real_drawable,
		      gc, filled, x, y, width, height);

  add_damage (offscreen, x, y, width, height, !filled);

}

static void
bdk_offscreen_window_draw_arc (BdkDrawable  *drawable,
			       BdkGC	       *gc,
			       gboolean	filled,
			       gint		x,
			       gint		y,
			       gint		width,
			       gint		height,
			       gint		angle1,
			       gint		angle2)
{
  BdkOffscreenWindow *offscreen = BDK_OFFSCREEN_WINDOW (drawable);
  BdkDrawable *real_drawable = get_real_drawable (offscreen);

  bdk_draw_arc (real_drawable,
		gc,
		filled,
		x,
		y,
		width,
		height,
		angle1,
		angle2);
  add_damage (offscreen, x, y, width, height, !filled);
}

static void
bdk_offscreen_window_draw_polygon (BdkDrawable  *drawable,
				   BdkGC	       *gc,
				   gboolean	filled,
				   BdkPoint     *points,
				   gint		npoints)
{
  BdkOffscreenWindow *offscreen = BDK_OFFSCREEN_WINDOW (drawable);
  BdkDrawable *real_drawable = get_real_drawable (offscreen);

  bdk_draw_polygon (real_drawable,
		    gc,
		    filled,
		    points,
		    npoints);

  if (npoints > 0)
    {
      int min_x, min_y, max_x, max_y, i;

      min_x = max_x = points[0].x;
      min_y = max_y = points[0].y;

	for (i = 1; i < npoints; i++)
	  {
	    min_x = MIN (min_x, points[i].x);
	    max_x = MAX (max_x, points[i].x);
	    min_y = MIN (min_y, points[i].y);
	    max_y = MAX (max_y, points[i].y);
	  }

	add_damage (offscreen, min_x, min_y,
		    max_x - min_x,
		    max_y - min_y, !filled);
    }
}

static void
bdk_offscreen_window_draw_text (BdkDrawable  *drawable,
				BdkFont      *font,
				BdkGC	       *gc,
				gint		x,
				gint		y,
				const gchar  *text,
				gint		text_length)
{
  BdkOffscreenWindow *offscreen = BDK_OFFSCREEN_WINDOW (drawable);
  BdkDrawable *real_drawable = get_real_drawable (offscreen);
  BdkWindowObject *private = BDK_WINDOW_OBJECT (offscreen->wrapper);

  bdk_draw_text (real_drawable,
		 font,
		 gc,
		 x,
		 y,
		 text,
		 text_length);

  /* Hard to compute the minimal size, not that often used anyway. */
  add_damage (offscreen, 0, 0, private->width, private->height, FALSE);
}

static void
bdk_offscreen_window_draw_text_wc (BdkDrawable	 *drawable,
				   BdkFont	 *font,
				   BdkGC		 *gc,
				   gint		  x,
				   gint		  y,
				   const BdkWChar *text,
				   gint		  text_length)
{
  BdkOffscreenWindow *offscreen = BDK_OFFSCREEN_WINDOW (drawable);
  BdkDrawable *real_drawable = get_real_drawable (offscreen);
  BdkWindowObject *private = BDK_WINDOW_OBJECT (offscreen->wrapper);

  bdk_draw_text_wc (real_drawable,
		    font,
		    gc,
		    x,
		    y,
		    text,
		    text_length);

  /* Hard to compute the minimal size, not that often used anyway. */
  add_damage (offscreen, 0, 0, private->width, private->height, FALSE);
}

static void
bdk_offscreen_window_draw_points (BdkDrawable  *drawable,
				  BdkGC	       *gc,
				  BdkPoint     *points,
				  gint		npoints)
{
  BdkOffscreenWindow *offscreen = BDK_OFFSCREEN_WINDOW (drawable);
  BdkDrawable *real_drawable = get_real_drawable (offscreen);

  bdk_draw_points (real_drawable,
		   gc,
		   points,
		   npoints);


  if (npoints > 0)
    {
      int min_x, min_y, max_x, max_y, i;

      min_x = max_x = points[0].x;
      min_y = max_y = points[0].y;

	for (i = 1; i < npoints; i++)
	  {
	    min_x = MIN (min_x, points[i].x);
	    max_x = MAX (max_x, points[i].x);
	    min_y = MIN (min_y, points[i].y);
	    max_y = MAX (max_y, points[i].y);
	  }

	add_damage (offscreen, min_x, min_y,
		    max_x - min_x + 1,
		    max_y - min_y + 1,
		    FALSE);
    }
}

static void
bdk_offscreen_window_draw_segments (BdkDrawable  *drawable,
				    BdkGC	 *gc,
				    BdkSegment   *segs,
				    gint	  nsegs)
{
  BdkOffscreenWindow *offscreen = BDK_OFFSCREEN_WINDOW (drawable);
  BdkDrawable *real_drawable = get_real_drawable (offscreen);

  bdk_draw_segments (real_drawable,
		     gc,
		     segs,
		     nsegs);

  if (nsegs > 0)
    {
      int min_x, min_y, max_x, max_y, i;

      min_x = max_x = segs[0].x1;
      min_y = max_y = segs[0].y1;

	for (i = 0; i < nsegs; i++)
	  {
	    min_x = MIN (min_x, segs[i].x1);
	    max_x = MAX (max_x, segs[i].x1);
	    min_x = MIN (min_x, segs[i].x2);
	    max_x = MAX (max_x, segs[i].x2);
	    min_y = MIN (min_y, segs[i].y1);
	    max_y = MAX (max_y, segs[i].y1);
	    min_y = MIN (min_y, segs[i].y2);
	    max_y = MAX (max_y, segs[i].y2);
	  }

	add_damage (offscreen, min_x, min_y,
		    max_x - min_x,
		    max_y - min_y, TRUE);
    }

}

static void
bdk_offscreen_window_draw_lines (BdkDrawable  *drawable,
				 BdkGC        *gc,
				 BdkPoint     *points,
				 gint          npoints)
{
  BdkOffscreenWindow *offscreen = BDK_OFFSCREEN_WINDOW (drawable);
  BdkDrawable *real_drawable = get_real_drawable (offscreen);
  BdkWindowObject *private = BDK_WINDOW_OBJECT (offscreen->wrapper);

  bdk_draw_lines (real_drawable,
		  gc,
		  points,
		  npoints);

  /* Hard to compute the minimal size, as we don't know the line
     width, and since joins are hard to calculate.
     Its not that often used anyway, damage it all */
  add_damage (offscreen, 0, 0, private->width, private->height, TRUE);
}

static void
bdk_offscreen_window_draw_image (BdkDrawable *drawable,
				 BdkGC	      *gc,
				 BdkImage    *image,
				 gint	       xsrc,
				 gint	       ysrc,
				 gint	       xdest,
				 gint	       ydest,
				 gint	       width,
				 gint	       height)
{
  BdkOffscreenWindow *offscreen = BDK_OFFSCREEN_WINDOW (drawable);
  BdkDrawable *real_drawable = get_real_drawable (offscreen);

  bdk_draw_image (real_drawable,
		  gc,
		  image,
		  xsrc,
		  ysrc,
		  xdest,
		  ydest,
		  width,
		  height);

  add_damage (offscreen, xdest, ydest, width, height, FALSE);
}


static void
bdk_offscreen_window_draw_pixbuf (BdkDrawable *drawable,
				  BdkGC       *gc,
				  BdkPixbuf   *pixbuf,
				  gint         src_x,
				  gint         src_y,
				  gint         dest_x,
				  gint         dest_y,
				  gint         width,
				  gint         height,
				  BdkRgbDither dither,
				  gint         x_dither,
				  gint         y_dither)
{
  BdkOffscreenWindow *offscreen = BDK_OFFSCREEN_WINDOW (drawable);
  BdkDrawable *real_drawable = get_real_drawable (offscreen);

  bdk_draw_pixbuf (real_drawable,
		   gc,
		   pixbuf,
		   src_x,
		   src_y,
		   dest_x,
		   dest_y,
		   width,
		   height,
		   dither,
		   x_dither,
		   y_dither);

  add_damage (offscreen, dest_x, dest_y, width, height, FALSE);

}

void
_bdk_offscreen_window_new (BdkWindow     *window,
			   BdkScreen     *screen,
			   BdkVisual     *visual,
			   BdkWindowAttr *attributes,
			   gint           attributes_mask)
{
  BdkWindowObject *private;
  BdkOffscreenWindow *offscreen;

  g_return_if_fail (attributes != NULL);

  if (attributes->wclass != BDK_INPUT_OUTPUT)
    return; /* Can't support input only offscreens */

  private = (BdkWindowObject *)window;

  if (private->parent != NULL && BDK_WINDOW_DESTROYED (private->parent))
    return;

  private->impl = g_object_new (BDK_TYPE_OFFSCREEN_WINDOW, NULL);
  offscreen = BDK_OFFSCREEN_WINDOW (private->impl);
  offscreen->wrapper = window;

  offscreen->screen = screen;

  if (attributes_mask & BDK_WA_COLORMAP)
    offscreen->colormap = g_object_ref (attributes->colormap);
  else
    {
      if (bdk_screen_get_system_visual (screen) == visual)
	{
	  offscreen->colormap = bdk_screen_get_system_colormap (screen);
	  g_object_ref (offscreen->colormap);
	}
      else
	offscreen->colormap = bdk_colormap_new (visual, FALSE);
    }

  offscreen->pixmap = bdk_pixmap_new ((BdkDrawable *)private->parent,
				      private->width,
				      private->height,
				      private->depth);
  bdk_drawable_set_colormap (offscreen->pixmap, offscreen->colormap);
}

static gboolean
bdk_offscreen_window_reparent (BdkWindow *window,
			       BdkWindow *new_parent,
			       gint       x,
			       gint       y)
{
  BdkWindowObject *private = (BdkWindowObject *)window;
  BdkWindowObject *new_parent_private = (BdkWindowObject *)new_parent;
  BdkWindowObject *old_parent;
  gboolean was_mapped;

  if (new_parent)
    {
      /* No input-output children of input-only windows */
      if (new_parent_private->input_only && !private->input_only)
	return FALSE;

      /* Don't create loops in hierarchy */
      if (is_parent_of (window, new_parent))
	return FALSE;
    }

  was_mapped = BDK_WINDOW_IS_MAPPED (window);

  bdk_window_hide (window);

  if (private->parent)
    private->parent->children = g_list_remove (private->parent->children, window);

  old_parent = private->parent;
  private->parent = new_parent_private;
  private->x = x;
  private->y = y;

  if (new_parent_private)
    private->parent->children = g_list_prepend (private->parent->children, window);

  _bdk_synthesize_crossing_events_for_geometry_change (window);
  if (old_parent)
    _bdk_synthesize_crossing_events_for_geometry_change (BDK_WINDOW (old_parent));

  return was_mapped;
}

static void
from_embedder (BdkWindow *window,
	       double embedder_x, double embedder_y,
	       double *offscreen_x, double *offscreen_y)
{
  BdkWindowObject *private;

  private = (BdkWindowObject *)window;

  g_signal_emit_by_name (private->impl_window,
			 "from-embedder",
			 embedder_x, embedder_y,
			 offscreen_x, offscreen_y,
			 NULL);
}

static void
to_embedder (BdkWindow *window,
	     double offscreen_x, double offscreen_y,
	     double *embedder_x, double *embedder_y)
{
  BdkWindowObject *private;

  private = (BdkWindowObject *)window;

  g_signal_emit_by_name (private->impl_window,
			 "to-embedder",
			 offscreen_x, offscreen_y,
			 embedder_x, embedder_y,
			 NULL);
}

static gint
bdk_offscreen_window_get_root_coords (BdkWindow *window,
				      gint       x,
				      gint       y,
				      gint      *root_x,
				      gint      *root_y)
{
  BdkWindowObject *private = BDK_WINDOW_OBJECT (window);
  BdkOffscreenWindow *offscreen;
  int tmpx, tmpy;

  tmpx = x;
  tmpy = y;

  offscreen = BDK_OFFSCREEN_WINDOW (private->impl);
  if (offscreen->embedder)
    {
      double dx, dy;
      to_embedder (window,
		   x, y,
		   &dx, &dy);
      tmpx = floor (dx + 0.5);
      tmpy = floor (dy + 0.5);
      bdk_window_get_root_coords (offscreen->embedder,
				  tmpx, tmpy,
				  &tmpx, &tmpy);

    }

  if (root_x)
    *root_x = tmpx;
  if (root_y)
    *root_y = tmpy;

  return TRUE;
}

static gint
bdk_offscreen_window_get_deskrelative_origin (BdkWindow *window,
					      gint      *x,
					      gint      *y)
{
  BdkWindowObject *private = BDK_WINDOW_OBJECT (window);
  BdkOffscreenWindow *offscreen;
  int tmpx, tmpy;

  tmpx = 0;
  tmpy = 0;

  offscreen = BDK_OFFSCREEN_WINDOW (private->impl);
  if (offscreen->embedder)
    {
      double dx, dy;
      bdk_window_get_deskrelative_origin (offscreen->embedder,
					  &tmpx, &tmpy);

      to_embedder (window,
		   0, 0,
		   &dx, &dy);
      tmpx = floor (tmpx + dx + 0.5);
      tmpy = floor (tmpy + dy + 0.5);
    }


  if (x)
    *x = tmpx;
  if (y)
    *y = tmpy;

  return TRUE;
}

static gboolean
bdk_offscreen_window_get_pointer (BdkWindow       *window,
				  gint            *x,
				  gint            *y,
				  BdkModifierType *mask)
{
  BdkWindowObject *private = BDK_WINDOW_OBJECT (window);
  BdkOffscreenWindow *offscreen;
  int tmpx, tmpy;
  double dtmpx, dtmpy;
  BdkModifierType tmpmask;

  tmpx = 0;
  tmpy = 0;
  tmpmask = 0;

  offscreen = BDK_OFFSCREEN_WINDOW (private->impl);
  if (offscreen->embedder != NULL)
    {
      bdk_window_get_pointer (offscreen->embedder, &tmpx, &tmpy, &tmpmask);
      from_embedder (window,
		     tmpx, tmpy,
		     &dtmpx, &dtmpy);
      tmpx = floor (dtmpx + 0.5);
      tmpy = floor (dtmpy + 0.5);
    }

  if (x)
    *x = tmpx;
  if (y)
    *y = tmpy;
  if (mask)
    *mask = tmpmask;
  return TRUE;
}

BdkDrawable *
_bdk_offscreen_window_get_real_drawable (BdkOffscreenWindow *offscreen)
{
  return get_real_drawable (offscreen);
}

/**
 * bdk_offscreen_window_get_pixmap:
 * @window: a #BdkWindow
 *
 * Gets the offscreen pixmap that an offscreen window renders into.
 * If you need to keep this around over window resizes, you need to
 * add a reference to it.
 *
 * Returns: The offscreen pixmap, or %NULL if not offscreen
 *
 * Since: 2.18
 */
BdkPixmap *
bdk_offscreen_window_get_pixmap (BdkWindow *window)
{
  BdkWindowObject *private = (BdkWindowObject *)window;
  BdkOffscreenWindow *offscreen;

  g_return_val_if_fail (BDK_IS_WINDOW (window), FALSE);

  if (!BDK_IS_OFFSCREEN_WINDOW (private->impl))
    return NULL;

  offscreen = BDK_OFFSCREEN_WINDOW (private->impl);
  return offscreen->pixmap;
}

static void
bdk_offscreen_window_raise (BdkWindow *window)
{
  /* bdk_window_raise already changed the stacking order */
  _bdk_synthesize_crossing_events_for_geometry_change (window);
}

static void
bdk_offscreen_window_lower (BdkWindow *window)
{
  /* bdk_window_lower already changed the stacking order */
  _bdk_synthesize_crossing_events_for_geometry_change (window);
}

static void
bdk_offscreen_window_move_resize_internal (BdkWindow *window,
					   gint       x,
					   gint       y,
					   gint       width,
					   gint       height,
					   gboolean   send_expose_events)
{
  BdkWindowObject *private = (BdkWindowObject *)window;
  BdkOffscreenWindow *offscreen;
  gint dx, dy, dw, dh;
  BdkGC *gc;
  BdkPixmap *old_pixmap;

  offscreen = BDK_OFFSCREEN_WINDOW (private->impl);

  if (width < 1)
    width = 1;
  if (height < 1)
    height = 1;

  if (private->destroyed)
    return;

  dx = x - private->x;
  dy = y - private->y;
  dw = width - private->width;
  dh = height - private->height;

  private->x = x;
  private->y = y;

  if (private->width != width ||
      private->height != height)
    {
      private->width = width;
      private->height = height;

      old_pixmap = offscreen->pixmap;
      offscreen->pixmap = bdk_pixmap_new (BDK_DRAWABLE (old_pixmap),
					  width,
					  height,
					  private->depth);

      gc = _bdk_drawable_get_scratch_gc (offscreen->pixmap, FALSE);
      bdk_draw_drawable (offscreen->pixmap,
			 gc,
			 old_pixmap,
			 0,0, 0, 0,
			 -1, -1);
      g_object_unref (old_pixmap);
    }

  if (BDK_WINDOW_IS_MAPPED (private))
    {
      // TODO: Only invalidate new area, i.e. for larger windows
      bdk_window_invalidate_rect (window, NULL, TRUE);
      _bdk_synthesize_crossing_events_for_geometry_change (window);
    }
}

static void
bdk_offscreen_window_move_resize (BdkWindow *window,
				  gboolean   with_move,
				  gint       x,
				  gint       y,
				  gint       width,
				  gint       height)
{
  BdkWindowObject *private = (BdkWindowObject *)window;
  BdkOffscreenWindow *offscreen;

  offscreen = BDK_OFFSCREEN_WINDOW (private->impl);

  if (!with_move)
    {
      x = private->x;
      y = private->y;
    }

  if (width < 0)
    width = private->width;

  if (height < 0)
    height = private->height;

  bdk_offscreen_window_move_resize_internal (window, x, y,
					     width, height,
					     TRUE);
}

static void
bdk_offscreen_window_show (BdkWindow *window,
			   gboolean already_mapped)
{
  BdkWindowObject *private = (BdkWindowObject *)window;

  bdk_window_clear_area_e (window, 0, 0,
			   private->width, private->height);
}


static void
bdk_offscreen_window_hide (BdkWindow *window)
{
  BdkWindowObject *private;
  BdkOffscreenWindow *offscreen;
  BdkDisplay *display;

  g_return_if_fail (window != NULL);

  private = (BdkWindowObject*) window;
  offscreen = BDK_OFFSCREEN_WINDOW (private->impl);

  /* May need to break grabs on children */
  display = bdk_drawable_get_display (window);

  /* TODO: This needs updating to the new grab world */
#if 0
  if (display->pointer_grab.window != NULL)
    {
      if (is_parent_of (window, display->pointer_grab.window))
	{
	  /* Call this ourselves, even though bdk_display_pointer_ungrab
	     does so too, since we want to pass implicit == TRUE so the
	     broken grab event is generated */
	  _bdk_display_unset_has_pointer_grab (display,
					       TRUE,
					       FALSE,
					       BDK_CURRENT_TIME);
	  bdk_display_pointer_ungrab (display, BDK_CURRENT_TIME);
	}
    }
#endif
}

static void
bdk_offscreen_window_withdraw (BdkWindow *window)
{
}

static BdkEventMask
bdk_offscreen_window_get_events (BdkWindow *window)
{
  return 0;
}

static void
bdk_offscreen_window_set_events (BdkWindow       *window,
				 BdkEventMask     event_mask)
{
}

static void
bdk_offscreen_window_set_background (BdkWindow      *window,
				     const BdkColor *color)
{
  BdkWindowObject *private = (BdkWindowObject *)window;
  BdkColormap *colormap = bdk_drawable_get_colormap (window);

  private->bg_color = *color;
  bdk_colormap_query_color (colormap, private->bg_color.pixel, &private->bg_color);

  if (private->bg_pixmap &&
      private->bg_pixmap != BDK_PARENT_RELATIVE_BG &&
      private->bg_pixmap != BDK_NO_BG)
    g_object_unref (private->bg_pixmap);

  private->bg_pixmap = NULL;
}

static void
bdk_offscreen_window_set_back_pixmap (BdkWindow *window,
				      BdkPixmap *pixmap)
{
  BdkWindowObject *private = (BdkWindowObject *)window;

  if (pixmap &&
      private->bg_pixmap != BDK_PARENT_RELATIVE_BG &&
      private->bg_pixmap != BDK_NO_BG &&
      !bdk_drawable_get_colormap (pixmap))
    {
      g_warning ("bdk_window_set_back_pixmap(): pixmap must have a colormap");
      return;
    }

  if (private->bg_pixmap &&
      private->bg_pixmap != BDK_PARENT_RELATIVE_BG &&
      private->bg_pixmap != BDK_NO_BG)
    g_object_unref (private->bg_pixmap);

  private->bg_pixmap = pixmap;

  if (pixmap &&
      private->bg_pixmap != BDK_PARENT_RELATIVE_BG &&
      private->bg_pixmap != BDK_NO_BG)
    g_object_ref (pixmap);
}

static void
bdk_offscreen_window_shape_combine_rebunnyion (BdkWindow       *window,
					   const BdkRebunnyion *shape_rebunnyion,
					   gint             offset_x,
					   gint             offset_y)
{
}

static void
bdk_offscreen_window_input_shape_combine_rebunnyion (BdkWindow       *window,
						 const BdkRebunnyion *shape_rebunnyion,
						 gint             offset_x,
						 gint             offset_y)
{
}

static gboolean
bdk_offscreen_window_set_static_gravities (BdkWindow *window,
					   gboolean   use_static)
{
  return TRUE;
}

static void
bdk_offscreen_window_set_cursor (BdkWindow *window,
				 BdkCursor *cursor)
{
  BdkWindowObject *private = (BdkWindowObject *)window;
  BdkOffscreenWindow *offscreen;

  offscreen = BDK_OFFSCREEN_WINDOW (private->impl);

  if (offscreen->cursor)
    {
      bdk_cursor_unref (offscreen->cursor);
      offscreen->cursor = NULL;
    }

  if (cursor)
    offscreen->cursor = bdk_cursor_ref (cursor);

  /* TODO: The cursor is never actually used... */
}

static void
bdk_offscreen_window_get_geometry (BdkWindow *window,
				   gint      *x,
				   gint      *y,
				   gint      *width,
				   gint      *height,
				   gint      *depth)
{
  BdkWindowObject *private = (BdkWindowObject *)window;

  g_return_if_fail (window == NULL || BDK_IS_WINDOW (window));

  if (!BDK_WINDOW_DESTROYED (window))
    {
      if (x)
	*x = private->x;
      if (y)
	*y = private->y;
      if (width)
	*width = private->width;
      if (height)
	*height = private->height;
      if (depth)
	*depth = private->depth;
    }
}

static gboolean
bdk_offscreen_window_queue_antiexpose (BdkWindow *window,
				       BdkRebunnyion *area)
{
  return FALSE;
}

static void
bdk_offscreen_window_queue_translation (BdkWindow *window,
					BdkGC     *gc,
					BdkRebunnyion *area,
					gint       dx,
					gint       dy)
{
}

/**
 * bdk_offscreen_window_set_embedder:
 * @window: a #BdkWindow
 * @embedder: the #BdkWindow that @window gets embedded in
 *
 * Sets @window to be embedded in @embedder.
 *
 * To fully embed an offscreen window, in addition to calling this
 * function, it is also necessary to handle the #BdkWindow::pick-embedded-child
 * signal on the @embedder and the #BdkWindow::to-embedder and
 * #BdkWindow::from-embedder signals on @window.
 *
 * Since: 2.18
 */
void
bdk_offscreen_window_set_embedder (BdkWindow     *window,
				   BdkWindow     *embedder)
{
  BdkWindowObject *private = (BdkWindowObject *)window;
  BdkOffscreenWindow *offscreen;

  g_return_if_fail (BDK_IS_WINDOW (window));

  if (!BDK_IS_OFFSCREEN_WINDOW (private->impl))
    return;

  offscreen = BDK_OFFSCREEN_WINDOW (private->impl);

  if (embedder)
    {
      g_object_ref (embedder);
      BDK_WINDOW_OBJECT (embedder)->num_offscreen_children++;
    }

  if (offscreen->embedder)
    {
      g_object_unref (offscreen->embedder);
      BDK_WINDOW_OBJECT (offscreen->embedder)->num_offscreen_children--;
    }

  offscreen->embedder = embedder;
}

/**
 * bdk_offscreen_window_get_embedder:
 * @window: a #BdkWindow
 *
 * Gets the window that @window is embedded in.
 *
 * Returns: the embedding #BdkWindow, or %NULL if @window is not an
 *     embedded offscreen window
 *
 * Since: 2.18
 */
BdkWindow *
bdk_offscreen_window_get_embedder (BdkWindow *window)
{
  BdkWindowObject *private = (BdkWindowObject *)window;
  BdkOffscreenWindow *offscreen;

  g_return_val_if_fail (BDK_IS_WINDOW (window), NULL);

  if (!BDK_IS_OFFSCREEN_WINDOW (private->impl))
    return NULL;

  offscreen = BDK_OFFSCREEN_WINDOW (private->impl);

  return offscreen->embedder;
}

static void
bdk_offscreen_window_class_init (BdkOffscreenWindowClass *klass)
{
  BdkDrawableClass *drawable_class = BDK_DRAWABLE_CLASS (klass);
  BObjectClass *object_class = B_OBJECT_CLASS (klass);

  object_class->finalize = bdk_offscreen_window_finalize;

  drawable_class->create_gc = bdk_offscreen_window_create_gc;
  drawable_class->_copy_to_image = bdk_offscreen_window_copy_to_image;
  drawable_class->ref_bairo_surface = bdk_offscreen_window_ref_bairo_surface;
  drawable_class->set_colormap = bdk_offscreen_window_set_colormap;
  drawable_class->get_colormap = bdk_offscreen_window_get_colormap;
  drawable_class->get_depth = bdk_offscreen_window_get_depth;
  drawable_class->get_screen = bdk_offscreen_window_get_screen;
  drawable_class->get_visual = bdk_offscreen_window_get_visual;
  drawable_class->get_source_drawable = bdk_offscreen_window_get_source_drawable;
  drawable_class->get_composite_drawable = bdk_offscreen_window_get_composite_drawable;

  drawable_class->draw_rectangle = bdk_offscreen_window_draw_rectangle;
  drawable_class->draw_arc = bdk_offscreen_window_draw_arc;
  drawable_class->draw_polygon = bdk_offscreen_window_draw_polygon;
  drawable_class->draw_text = bdk_offscreen_window_draw_text;
  drawable_class->draw_text_wc = bdk_offscreen_window_draw_text_wc;
  drawable_class->draw_drawable_with_src = bdk_offscreen_window_draw_drawable;
  drawable_class->draw_points = bdk_offscreen_window_draw_points;
  drawable_class->draw_segments = bdk_offscreen_window_draw_segments;
  drawable_class->draw_lines = bdk_offscreen_window_draw_lines;
  drawable_class->draw_image = bdk_offscreen_window_draw_image;
  drawable_class->draw_pixbuf = bdk_offscreen_window_draw_pixbuf;
}

static void
bdk_offscreen_window_impl_iface_init (BdkWindowImplIface *iface)
{
  iface->show = bdk_offscreen_window_show;
  iface->hide = bdk_offscreen_window_hide;
  iface->withdraw = bdk_offscreen_window_withdraw;
  iface->raise = bdk_offscreen_window_raise;
  iface->lower = bdk_offscreen_window_lower;
  iface->move_resize = bdk_offscreen_window_move_resize;
  iface->set_background = bdk_offscreen_window_set_background;
  iface->set_back_pixmap = bdk_offscreen_window_set_back_pixmap;
  iface->get_events = bdk_offscreen_window_get_events;
  iface->set_events = bdk_offscreen_window_set_events;
  iface->reparent = bdk_offscreen_window_reparent;
  iface->set_cursor = bdk_offscreen_window_set_cursor;
  iface->get_geometry = bdk_offscreen_window_get_geometry;
  iface->shape_combine_rebunnyion = bdk_offscreen_window_shape_combine_rebunnyion;
  iface->input_shape_combine_rebunnyion = bdk_offscreen_window_input_shape_combine_rebunnyion;
  iface->set_static_gravities = bdk_offscreen_window_set_static_gravities;
  iface->queue_antiexpose = bdk_offscreen_window_queue_antiexpose;
  iface->queue_translation = bdk_offscreen_window_queue_translation;
  iface->get_root_coords = bdk_offscreen_window_get_root_coords;
  iface->get_deskrelative_origin = bdk_offscreen_window_get_deskrelative_origin;
  iface->get_pointer = bdk_offscreen_window_get_pointer;
  iface->destroy = bdk_offscreen_window_destroy;
}

#define __BDK_OFFSCREEN_WINDOW_C__
#include "bdkaliasdef.c"
