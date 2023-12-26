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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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
#include <string.h>

#include "bdkbairo.h"
#include "bdkgc.h"
#include "bdkinternals.h"
#include "bdkpixmap.h"
#include "bdkrgb.h"
#include "bdkprivate.h"
#include "bdkalias.h"

static void bdk_gc_finalize   (BObject      *object);

typedef struct _BdkGCPrivate BdkGCPrivate;

struct _BdkGCPrivate
{
  BdkRebunnyion *clip_rebunnyion;

  buint32 rebunnyion_tag_applied;
  int rebunnyion_tag_offset_x;
  int rebunnyion_tag_offset_y;

  BdkRebunnyion *old_clip_rebunnyion;
  BdkPixmap *old_clip_mask;

  BdkBitmap *stipple;
  BdkPixmap *tile;

  BdkPixmap *clip_mask;

  buint32 fg_pixel;
  buint32 bg_pixel;

  buint subwindow_mode : 1;
  buint fill : 2;
  buint exposures : 2;
};

#define BDK_GC_GET_PRIVATE(o) (B_TYPE_INSTANCE_GET_PRIVATE ((o), BDK_TYPE_GC, BdkGCPrivate))

G_DEFINE_TYPE (BdkGC, bdk_gc, B_TYPE_OBJECT)

static void
bdk_gc_class_init (BdkGCClass *class)
{
  BObjectClass *object_class = B_OBJECT_CLASS (class);
  
  object_class->finalize = bdk_gc_finalize;

  g_type_class_add_private (object_class, sizeof (BdkGCPrivate));
}

static void
bdk_gc_init (BdkGC *gc)
{
  BdkGCPrivate *priv = BDK_GC_GET_PRIVATE (gc);

  priv->fill = BDK_SOLID;

  /* These are the default X11 value, which we match. They are clearly
   * wrong for TrueColor displays, so apps have to change them.
   */
  priv->fg_pixel = 0;
  priv->bg_pixel = 1;
}

/**
 * bdk_gc_new:
 * @drawable: a #BdkDrawable. The created GC must always be used
 *   with drawables of the same depth as this one.
 *
 * Create a new graphics context with default values. 
 *
 * Returns: the new graphics context.
 *
 * Deprecated: 2.22: Use Bairo for rendering.
 **/
BdkGC*
bdk_gc_new (BdkDrawable *drawable)
{
  g_return_val_if_fail (drawable != NULL, NULL);

  return bdk_gc_new_with_values (drawable, NULL, 0);
}

/**
 * bdk_gc_new_with_values:
 * @drawable: a #BdkDrawable. The created GC must always be used
 *   with drawables of the same depth as this one.
 * @values: a structure containing initial values for the GC.
 * @values_mask: a bit mask indicating which fields in @values
 *   are set.
 * 
 * Create a new GC with the given initial values.
 * 
 * Return value: the new graphics context.
 *
 * Deprecated: 2.22: Use Bairo for rendering.
 **/
BdkGC*
bdk_gc_new_with_values (BdkDrawable	*drawable,
			BdkGCValues	*values,
			BdkGCValuesMask	 values_mask)
{
  g_return_val_if_fail (drawable != NULL, NULL);

  return BDK_DRAWABLE_GET_CLASS (drawable)->create_gc (drawable,
						       values,
						       values_mask);
}

/**
 * _bdk_gc_init:
 * @gc: a #BdkGC
 * @drawable: a #BdkDrawable.
 * @values: a structure containing initial values for the GC.
 * @values_mask: a bit mask indicating which fields in @values
 *   are set.
 * 
 * Does initialization of the generic portions of a #BdkGC
 * created with the specified values and values_mask. This
 * should be called out of the implementation of
 * BdkDrawable.create_gc() immediately after creating the
 * #BdkGC object.
 *
 * Deprecated: 2.22: Use Bairo for rendering.
 **/
void
_bdk_gc_init (BdkGC           *gc,
	      BdkDrawable     *drawable,
	      BdkGCValues     *values,
	      BdkGCValuesMask  values_mask)
{
  BdkGCPrivate *priv;

  g_return_if_fail (BDK_IS_GC (gc));

  priv = BDK_GC_GET_PRIVATE (gc);

  if (values_mask & BDK_GC_CLIP_X_ORIGIN)
    gc->clip_x_origin = values->clip_x_origin;
  if (values_mask & BDK_GC_CLIP_Y_ORIGIN)
    gc->clip_y_origin = values->clip_y_origin;
  if ((values_mask & BDK_GC_CLIP_MASK) && values->clip_mask)
    priv->clip_mask = g_object_ref (values->clip_mask);
  if (values_mask & BDK_GC_TS_X_ORIGIN)
    gc->ts_x_origin = values->ts_x_origin;
  if (values_mask & BDK_GC_TS_Y_ORIGIN)
    gc->ts_y_origin = values->ts_y_origin;
  if (values_mask & BDK_GC_FILL)
    priv->fill = values->fill;
  if (values_mask & BDK_GC_STIPPLE)
    {
      priv->stipple = values->stipple;
      if (priv->stipple)
	g_object_ref (priv->stipple);
    }
  if (values_mask & BDK_GC_TILE)
    {
      priv->tile = values->tile;
      if (priv->tile)
	g_object_ref (priv->tile);
    }
  if (values_mask & BDK_GC_FOREGROUND)
    priv->fg_pixel = values->foreground.pixel;
  if (values_mask & BDK_GC_BACKGROUND)
    priv->bg_pixel = values->background.pixel;
  if (values_mask & BDK_GC_SUBWINDOW)
    priv->subwindow_mode = values->subwindow_mode;
  if (values_mask & BDK_GC_EXPOSURES)
    priv->exposures = values->graphics_exposures;
  else
    priv->exposures = TRUE;

  gc->colormap = bdk_drawable_get_colormap (drawable);
  if (gc->colormap)
    g_object_ref (gc->colormap);
}

static void
bdk_gc_finalize (BObject *object)
{
  BdkGC *gc = BDK_GC (object);
  BdkGCPrivate *priv = BDK_GC_GET_PRIVATE (gc);

  if (priv->clip_rebunnyion)
    bdk_rebunnyion_destroy (priv->clip_rebunnyion);
  if (priv->old_clip_rebunnyion)
    bdk_rebunnyion_destroy (priv->old_clip_rebunnyion);
  if (priv->clip_mask)
    g_object_unref (priv->clip_mask);
  if (priv->old_clip_mask)
    g_object_unref (priv->old_clip_mask);
  if (gc->colormap)
    g_object_unref (gc->colormap);
  if (priv->tile)
    g_object_unref (priv->tile);
  if (priv->stipple)
    g_object_unref (priv->stipple);

  B_OBJECT_CLASS (bdk_gc_parent_class)->finalize (object);
}

/**
 * bdk_gc_ref:
 * @gc: a #BdkGC
 *
 * Deprecated function; use g_object_ref() instead.
 *
 * Return value: the gc.
 *
 * Deprecated: 2.0: Use g_object_ref() instead.
 **/
BdkGC *
bdk_gc_ref (BdkGC *gc)
{
  return (BdkGC *) g_object_ref (gc);
}

/**
 * bdk_gc_unref:
 * @gc: a #BdkGC
 *
 * Decrement the reference count of @gc.
 *
 * Deprecated: 2.0: Use g_object_unref() instead.
 **/
void
bdk_gc_unref (BdkGC *gc)
{
  g_object_unref (gc);
}

/**
 * bdk_gc_get_values:
 * @gc:  a #BdkGC.
 * @values: the #BdkGCValues structure in which to store the results.
 * 
 * Retrieves the current values from a graphics context. Note that 
 * only the pixel values of the @values->foreground and @values->background
 * are filled, use bdk_colormap_query_color() to obtain the rgb values
 * if you need them.
 *
 * Deprecated: 2.22: Use Bairo for rendering.
 **/
void
bdk_gc_get_values (BdkGC       *gc,
		   BdkGCValues *values)
{
  g_return_if_fail (BDK_IS_GC (gc));
  g_return_if_fail (values != NULL);

  BDK_GC_GET_CLASS (gc)->get_values (gc, values);
}

/**
 * bdk_gc_set_values:
 * @gc: a #BdkGC
 * @values: struct containing the new values
 * @values_mask: mask indicating which struct fields are to be used
 *
 * Sets attributes of a graphics context in bulk. For each flag set in
 * @values_mask, the corresponding field will be read from @values and
 * set as the new value for @gc. If you're only setting a few values
 * on @gc, calling individual "setter" functions is likely more
 * convenient.
 *
 * Deprecated: 2.22: Use Bairo for rendering.
 **/
void
bdk_gc_set_values (BdkGC           *gc,
		   BdkGCValues	   *values,
		   BdkGCValuesMask  values_mask)
{
  BdkGCPrivate *priv;

  g_return_if_fail (BDK_IS_GC (gc));
  g_return_if_fail (values != NULL);

  priv = BDK_GC_GET_PRIVATE (gc);

  if ((values_mask & BDK_GC_CLIP_X_ORIGIN) ||
      (values_mask & BDK_GC_CLIP_Y_ORIGIN) ||
      (values_mask & BDK_GC_CLIP_MASK) ||
      (values_mask & BDK_GC_SUBWINDOW))
    _bdk_gc_remove_drawable_clip (gc);
  
  if (values_mask & BDK_GC_CLIP_X_ORIGIN)
    gc->clip_x_origin = values->clip_x_origin;
  if (values_mask & BDK_GC_CLIP_Y_ORIGIN)
    gc->clip_y_origin = values->clip_y_origin;
  if (values_mask & BDK_GC_TS_X_ORIGIN)
    gc->ts_x_origin = values->ts_x_origin;
  if (values_mask & BDK_GC_TS_Y_ORIGIN)
    gc->ts_y_origin = values->ts_y_origin;
  if (values_mask & BDK_GC_CLIP_MASK)
    {
      if (priv->clip_mask)
	{
	  g_object_unref (priv->clip_mask);
	  priv->clip_mask = NULL;
	}
      if (values->clip_mask)
	priv->clip_mask = g_object_ref (values->clip_mask);
      
      if (priv->clip_rebunnyion)
	{
	  bdk_rebunnyion_destroy (priv->clip_rebunnyion);
	  priv->clip_rebunnyion = NULL;
	}
    }
  if (values_mask & BDK_GC_FILL)
    priv->fill = values->fill;
  if (values_mask & BDK_GC_STIPPLE)
    {
      if (priv->stipple != values->stipple)
	{
	  if (priv->stipple)
	    g_object_unref (priv->stipple);
	  priv->stipple = values->stipple;
	  if (priv->stipple)
	    g_object_ref (priv->stipple);
	}
    }
  if (values_mask & BDK_GC_TILE)
    {
      if (priv->tile != values->tile)
	{
	  if (priv->tile)
	    g_object_unref (priv->tile);
	  priv->tile = values->tile;
	  if (priv->tile)
	    g_object_ref (priv->tile);
	}
    }
  if (values_mask & BDK_GC_FOREGROUND)
    priv->fg_pixel = values->foreground.pixel;
  if (values_mask & BDK_GC_BACKGROUND)
    priv->bg_pixel = values->background.pixel;
  if (values_mask & BDK_GC_SUBWINDOW)
    priv->subwindow_mode = values->subwindow_mode;
  if (values_mask & BDK_GC_EXPOSURES)
    priv->exposures = values->graphics_exposures;
  
  BDK_GC_GET_CLASS (gc)->set_values (gc, values, values_mask);
}

/**
 * bdk_gc_set_foreground:
 * @gc: a #BdkGC.
 * @color: the new foreground color.
 * 
 * Sets the foreground color for a graphics context.
 * Note that this function uses @color->pixel, use 
 * bdk_gc_set_rgb_fg_color() to specify the foreground 
 * color as red, green, blue components.
 *
 * Deprecated: 2.22: Use bdk_bairo_set_source_color() to use a #BdkColor
 * as the source in Bairo.
 **/
void
bdk_gc_set_foreground (BdkGC	      *gc,
		       const BdkColor *color)
{
  BdkGCValues values;

  g_return_if_fail (BDK_IS_GC (gc));
  g_return_if_fail (color != NULL);

  values.foreground = *color;
  bdk_gc_set_values (gc, &values, BDK_GC_FOREGROUND);
}

/**
 * bdk_gc_set_background:
 * @gc: a #BdkGC.
 * @color: the new background color.
 * 
 * Sets the background color for a graphics context.
 * Note that this function uses @color->pixel, use 
 * bdk_gc_set_rgb_bg_color() to specify the background 
 * color as red, green, blue components.
 *
 * Deprecated: 2.22: Use bdk_bairo_set_source_color() to use a #BdkColor
 * as the source in Bairo. Note that if you want to draw a background and a
 * foreground in Bairo, you need to call drawing functions (like bairo_fill())
 * twice.
 **/
void
bdk_gc_set_background (BdkGC	      *gc,
		       const BdkColor *color)
{
  BdkGCValues values;

  g_return_if_fail (BDK_IS_GC (gc));
  g_return_if_fail (color != NULL);

  values.background = *color;
  bdk_gc_set_values (gc, &values, BDK_GC_BACKGROUND);
}

/**
 * bdk_gc_set_font:
 * @gc: a #BdkGC.
 * @font: the new font. 
 * 
 * Sets the font for a graphics context. (Note that
 * all text-drawing functions in BDK take a @font
 * argument; the value set here is used when that
 * argument is %NULL.)
 **/
void
bdk_gc_set_font (BdkGC	 *gc,
		 BdkFont *font)
{
  BdkGCValues values;

  g_return_if_fail (BDK_IS_GC (gc));
  g_return_if_fail (font != NULL);

  values.font = font;
  bdk_gc_set_values (gc, &values, BDK_GC_FONT);
}

/**
 * bdk_gc_set_function:
 * @gc: a #BdkGC.
 * @function: the #BdkFunction to use
 * 
 * Determines how the current pixel values and the
 * pixel values being drawn are combined to produce
 * the final pixel values.
 *
 * Deprecated: 2.22: Use bairo_set_operator() with Bairo.
 **/
void
bdk_gc_set_function (BdkGC	 *gc,
		     BdkFunction  function)
{
  BdkGCValues values;

  g_return_if_fail (BDK_IS_GC (gc));

  values.function = function;
  bdk_gc_set_values (gc, &values, BDK_GC_FUNCTION);
}

/**
 * bdk_gc_set_fill:
 * @gc: a #BdkGC.
 * @fill: the new fill mode.
 * 
 * Set the fill mode for a graphics context.
 *
 * Deprecated: 2.22: You can achieve tiling in Bairo by using
 * bairo_pattern_set_extend() on the source. For stippling, see the
 * deprecation comments on bdk_gc_set_stipple().
 **/
void
bdk_gc_set_fill (BdkGC	 *gc,
		 BdkFill  fill)
{
  BdkGCValues values;

  g_return_if_fail (BDK_IS_GC (gc));

  values.fill = fill;
  bdk_gc_set_values (gc, &values, BDK_GC_FILL);
}

/**
 * bdk_gc_set_tile:
 * @gc:  a #BdkGC.
 * @tile:  the new tile pixmap.
 * 
 * Set a tile pixmap for a graphics context.
 * This will only be used if the fill mode
 * is %BDK_TILED.
 *
 * Deprecated: 2.22: The following code snippet sets a tiling #BdkPixmap
 * as the source in Bairo:
 * |[bdk_bairo_set_source_pixmap (cr, tile, ts_origin_x, ts_origin_y);
 * bairo_pattern_set_extend (bairo_get_source (cr), BAIRO_EXTEND_REPEAT);]|
 **/
void
bdk_gc_set_tile (BdkGC	   *gc,
		 BdkPixmap *tile)
{
  BdkGCValues values;

  g_return_if_fail (BDK_IS_GC (gc));

  values.tile = tile;
  bdk_gc_set_values (gc, &values, BDK_GC_TILE);
}

/**
 * bdk_gc_set_stipple:
 * @gc: a #BdkGC.
 * @stipple: the new stipple bitmap.
 * 
 * Set the stipple bitmap for a graphics context. The
 * stipple will only be used if the fill mode is
 * %BDK_STIPPLED or %BDK_OPAQUE_STIPPLED.
 *
 * Deprecated: 2.22: Stippling has no direct replacement in Bairo. If you
 * want to achieve an identical look, you can use the stipple bitmap as a
 * mask. Most likely, this involves rendering the source to an intermediate
 * surface using bairo_push_group() first, so that you can then use
 * bairo_mask() to achieve the stippled look.
 **/
void
bdk_gc_set_stipple (BdkGC     *gc,
		    BdkPixmap *stipple)
{
  BdkGCValues values;

  g_return_if_fail (BDK_IS_GC (gc));

  values.stipple = stipple;
  bdk_gc_set_values (gc, &values, BDK_GC_STIPPLE);
}

/**
 * bdk_gc_set_ts_origin:
 * @gc:  a #BdkGC.
 * @x: the x-coordinate of the origin.
 * @y: the y-coordinate of the origin.
 * 
 * Set the origin when using tiles or stipples with
 * the GC. The tile or stipple will be aligned such
 * that the upper left corner of the tile or stipple
 * will coincide with this point.
 *
 * Deprecated: 2.22: You can set the origin for tiles and stipples in Bairo
 * by changing the source's matrix using bairo_pattern_set_matrix(). Or you
 * can specify it with bdk_bairo_set_source_pixmap() as shown in the example
 * for bdk_gc_set_tile().
 **/
void
bdk_gc_set_ts_origin (BdkGC *gc,
		      bint   x,
		      bint   y)
{
  BdkGCValues values;

  g_return_if_fail (BDK_IS_GC (gc));

  values.ts_x_origin = x;
  values.ts_y_origin = y;
  
  bdk_gc_set_values (gc, &values,
		     BDK_GC_TS_X_ORIGIN | BDK_GC_TS_Y_ORIGIN);
}

/**
 * bdk_gc_set_clip_origin:
 * @gc: a #BdkGC.
 * @x: the x-coordinate of the origin.
 * @y: the y-coordinate of the origin.
 * 
 * Sets the origin of the clip mask. The coordinates are
 * interpreted relative to the upper-left corner of
 * the destination drawable of the current operation.
 *
 * Deprecated: 2.22: Use bairo_translate() before applying the clip path in
 * Bairo.
 **/
void
bdk_gc_set_clip_origin (BdkGC *gc,
			bint   x,
			bint   y)
{
  BdkGCValues values;

  g_return_if_fail (BDK_IS_GC (gc));

  values.clip_x_origin = x;
  values.clip_y_origin = y;
  
  bdk_gc_set_values (gc, &values,
		     BDK_GC_CLIP_X_ORIGIN | BDK_GC_CLIP_Y_ORIGIN);
}

/**
 * bdk_gc_set_clip_mask:
 * @gc: the #BdkGC.
 * @mask: a bitmap.
 * 
 * Sets the clip mask for a graphics context from a bitmap.
 * The clip mask is interpreted relative to the clip
 * origin. (See bdk_gc_set_clip_origin()).
 *
 * Deprecated: 2.22: Use bairo_mask() instead.
 **/
void
bdk_gc_set_clip_mask (BdkGC	*gc,
		      BdkBitmap *mask)
{
  BdkGCValues values;
  
  g_return_if_fail (BDK_IS_GC (gc));
  
  values.clip_mask = mask;
  bdk_gc_set_values (gc, &values, BDK_GC_CLIP_MASK);
}

/* Takes ownership of passed in rebunnyion */
static void
_bdk_gc_set_clip_rebunnyion_real (BdkGC     *gc,
			      BdkRebunnyion *rebunnyion,
			      bboolean reset_origin)
{
  BdkGCPrivate *priv = BDK_GC_GET_PRIVATE (gc);

  if (priv->clip_mask)
    {
      g_object_unref (priv->clip_mask);
      priv->clip_mask = NULL;
    }
  
  if (priv->clip_rebunnyion)
    bdk_rebunnyion_destroy (priv->clip_rebunnyion);

  priv->clip_rebunnyion = rebunnyion;

  _bdk_windowing_gc_set_clip_rebunnyion (gc, rebunnyion, reset_origin);
}

/* Doesn't copy rebunnyion, allows not to reset origin */
void
_bdk_gc_set_clip_rebunnyion_internal (BdkGC     *gc,
				  BdkRebunnyion *rebunnyion,
				  bboolean reset_origin)
{
  _bdk_gc_remove_drawable_clip (gc);
  _bdk_gc_set_clip_rebunnyion_real (gc, rebunnyion, reset_origin);
}


void
_bdk_gc_add_drawable_clip (BdkGC     *gc,
			   buint32    rebunnyion_tag,
			   BdkRebunnyion *rebunnyion,
			   int        offset_x,
			   int        offset_y)
{
  BdkGCPrivate *priv = BDK_GC_GET_PRIVATE (gc);

  if (priv->rebunnyion_tag_applied == rebunnyion_tag &&
      offset_x == priv->rebunnyion_tag_offset_x &&
      offset_y == priv->rebunnyion_tag_offset_y)
    return; /* Already appied this drawable rebunnyion */
  
  if (priv->rebunnyion_tag_applied)
    _bdk_gc_remove_drawable_clip (gc);

  rebunnyion = bdk_rebunnyion_copy (rebunnyion);
  if (offset_x != 0 || offset_y != 0)
    bdk_rebunnyion_offset (rebunnyion, offset_x, offset_y);

  if (priv->clip_mask)
    {
      int w, h;
      BdkPixmap *new_mask;
      BdkGC *tmp_gc;
      BdkColor black = {0, 0, 0, 0};
      BdkRectangle r;
      BdkOverlapType overlap;

      bdk_drawable_get_size (priv->clip_mask, &w, &h);

      r.x = 0;
      r.y = 0;
      r.width = w;
      r.height = h;

      /* Its quite common to expose areas that are completely in or outside
       * the rebunnyion, so we try to avoid allocating bitmaps that are just fully
       * set or completely unset.
       */
      overlap = bdk_rebunnyion_rect_in (rebunnyion, &r);
      if (overlap == BDK_OVERLAP_RECTANGLE_PART)
	{
	   /* The rebunnyion and the mask intersect, create a new clip mask that
	      includes both areas */
	  priv->old_clip_mask = g_object_ref (priv->clip_mask);
	  new_mask = bdk_pixmap_new (priv->old_clip_mask, w, h, -1);
	  tmp_gc = _bdk_drawable_get_scratch_gc ((BdkDrawable *)new_mask, FALSE);

	  bdk_gc_set_foreground (tmp_gc, &black);
	  bdk_draw_rectangle (new_mask, tmp_gc, TRUE, 0, 0, -1, -1);
	  _bdk_gc_set_clip_rebunnyion_internal (tmp_gc, rebunnyion, TRUE); /* Takes ownership of rebunnyion */
	  bdk_draw_drawable  (new_mask,
			      tmp_gc,
			      priv->old_clip_mask,
			      0, 0,
			      0, 0,
			      -1, -1);
	  bdk_gc_set_clip_rebunnyion (tmp_gc, NULL);
	  bdk_gc_set_clip_mask (gc, new_mask);
	  g_object_unref (new_mask);
	}
      else if (overlap == BDK_OVERLAP_RECTANGLE_OUT)
	{
	  /* No intersection, set empty clip rebunnyion */
	  BdkRebunnyion *empty = bdk_rebunnyion_new ();

	  bdk_rebunnyion_destroy (rebunnyion);
	  priv->old_clip_mask = g_object_ref (priv->clip_mask);
	  priv->clip_rebunnyion = empty;
	  _bdk_windowing_gc_set_clip_rebunnyion (gc, empty, FALSE);
	}
      else
	{
	  /* Completely inside rebunnyion, don't set unnecessary clip */
	  bdk_rebunnyion_destroy (rebunnyion);
	  return;
	}
    }
  else
    {
      priv->old_clip_rebunnyion = priv->clip_rebunnyion;
      priv->clip_rebunnyion = rebunnyion;
      if (priv->old_clip_rebunnyion)
	bdk_rebunnyion_intersect (rebunnyion, priv->old_clip_rebunnyion);

      _bdk_windowing_gc_set_clip_rebunnyion (gc, priv->clip_rebunnyion, FALSE);
    }

  priv->rebunnyion_tag_applied = rebunnyion_tag;
  priv->rebunnyion_tag_offset_x = offset_x;
  priv->rebunnyion_tag_offset_y = offset_y;
}

void
_bdk_gc_remove_drawable_clip (BdkGC *gc)
{
  BdkGCPrivate *priv = BDK_GC_GET_PRIVATE (gc);

  if (priv->rebunnyion_tag_applied)
    {
      priv->rebunnyion_tag_applied = 0;
      if (priv->old_clip_mask)
	{
	  bdk_gc_set_clip_mask (gc, priv->old_clip_mask);
	  g_object_unref (priv->old_clip_mask);
	  priv->old_clip_mask = NULL;

	  if (priv->clip_rebunnyion)
	    {
	      g_object_unref (priv->clip_rebunnyion);
	      priv->clip_rebunnyion = NULL;
	    }
	}
      else
	{
	  _bdk_gc_set_clip_rebunnyion_real (gc, priv->old_clip_rebunnyion, FALSE);
	  priv->old_clip_rebunnyion = NULL;
	}
    }
}

/**
 * bdk_gc_set_clip_rectangle:
 * @gc: a #BdkGC.
 * @rectangle: the rectangle to clip to.
 * 
 * Sets the clip mask for a graphics context from a
 * rectangle. The clip mask is interpreted relative to the clip
 * origin. (See bdk_gc_set_clip_origin()).
 *
 * Deprecated: 2.22: Use bairo_rectangle() and bairo_clip() in Bairo.
 **/
void
bdk_gc_set_clip_rectangle (BdkGC              *gc,
			   const BdkRectangle *rectangle)
{
  BdkRebunnyion *rebunnyion;
  
  g_return_if_fail (BDK_IS_GC (gc));

  _bdk_gc_remove_drawable_clip (gc);
  
  if (rectangle)
    rebunnyion = bdk_rebunnyion_rectangle (rectangle);
  else
    rebunnyion = NULL;

  _bdk_gc_set_clip_rebunnyion_real (gc, rebunnyion, TRUE);
}

/**
 * bdk_gc_set_clip_rebunnyion:
 * @gc: a #BdkGC.
 * @rebunnyion: the #BdkRebunnyion. 
 * 
 * Sets the clip mask for a graphics context from a rebunnyion structure.
 * The clip mask is interpreted relative to the clip origin. (See
 * bdk_gc_set_clip_origin()).
 *
 * Deprecated: 2.22: Use bdk_bairo_rebunnyion() and bairo_clip() in Bairo.
 **/
void
bdk_gc_set_clip_rebunnyion (BdkGC           *gc,
			const BdkRebunnyion *rebunnyion)
{
  BdkRebunnyion *copy;

  g_return_if_fail (BDK_IS_GC (gc));

  _bdk_gc_remove_drawable_clip (gc);
  
  if (rebunnyion)
    copy = bdk_rebunnyion_copy (rebunnyion);
  else
    copy = NULL;

  _bdk_gc_set_clip_rebunnyion_real (gc, copy, TRUE);
}

/**
 * _bdk_gc_get_clip_rebunnyion:
 * @gc: a #BdkGC
 * 
 * Gets the current clip rebunnyion for @gc, if any.
 * 
 * Return value: the clip rebunnyion for the GC, or %NULL.
 *   (if a clip mask is set, the return will be %NULL)
 *   This value is owned by the GC and must not be freed.
 **/
BdkRebunnyion *
_bdk_gc_get_clip_rebunnyion (BdkGC *gc)
{
  g_return_val_if_fail (BDK_IS_GC (gc), NULL);

  return BDK_GC_GET_PRIVATE (gc)->clip_rebunnyion;
}

/**
 * _bdk_gc_get_clip_mask:
 * @gc: a #BdkGC
 *
 * Gets the current clip mask for @gc, if any.
 *
 * Return value: the clip mask for the GC, or %NULL.
 *   (if a clip rebunnyion is set, the return will be %NULL)
 *   This value is owned by the GC and must not be freed.
 **/
BdkBitmap *
_bdk_gc_get_clip_mask (BdkGC *gc)
{
  g_return_val_if_fail (BDK_IS_GC (gc), NULL);

  return BDK_GC_GET_PRIVATE (gc)->clip_mask;
}

/**
 * _bdk_gc_get_fill:
 * @gc: a #BdkGC
 * 
 * Gets the current file style for the GC
 * 
 * Return value: the file style for the GC
 **/
BdkFill
_bdk_gc_get_fill (BdkGC *gc)
{
  g_return_val_if_fail (BDK_IS_GC (gc), BDK_SOLID);

  return BDK_GC_GET_PRIVATE (gc)->fill;
}

bboolean
_bdk_gc_get_exposures (BdkGC *gc)
{
  g_return_val_if_fail (BDK_IS_GC (gc), FALSE);

  return BDK_GC_GET_PRIVATE (gc)->exposures;
}

/**
 * _bdk_gc_get_tile:
 * @gc: a #BdkGC
 * 
 * Gets the tile pixmap for @gc, if any
 * 
 * Return value: the tile set on the GC, or %NULL. The
 *   value is owned by the GC and must not be freed.
 **/
BdkPixmap *
_bdk_gc_get_tile (BdkGC *gc)
{
  g_return_val_if_fail (BDK_IS_GC (gc), NULL);

  return BDK_GC_GET_PRIVATE (gc)->tile;
}

/**
 * _bdk_gc_get_stipple:
 * @gc: a #BdkGC
 * 
 * Gets the stipple pixmap for @gc, if any
 * 
 * Return value: the stipple set on the GC, or %NULL. The
 *   value is owned by the GC and must not be freed.
 **/
BdkBitmap *
_bdk_gc_get_stipple (BdkGC *gc)
{
  g_return_val_if_fail (BDK_IS_GC (gc), NULL);

  return BDK_GC_GET_PRIVATE (gc)->stipple;
}

/**
 * _bdk_gc_get_fg_pixel:
 * @gc: a #BdkGC
 * 
 * Gets the foreground pixel value for @gc. If the
 * foreground pixel has never been set, returns the
 * default value 0.
 * 
 * Return value: the foreground pixel value of the GC
 **/
buint32
_bdk_gc_get_fg_pixel (BdkGC *gc)
{
  g_return_val_if_fail (BDK_IS_GC (gc), 0);
  
  return BDK_GC_GET_PRIVATE (gc)->fg_pixel;
}

/**
 * _bdk_gc_get_bg_pixel:
 * @gc: a #BdkGC
 * 
 * Gets the background pixel value for @gc.If the
 * foreground pixel has never been set, returns the
 * default value 1.
 * 
 * Return value: the foreground pixel value of the GC
 **/
buint32
_bdk_gc_get_bg_pixel (BdkGC *gc)
{
  g_return_val_if_fail (BDK_IS_GC (gc), 0);
  
  return BDK_GC_GET_PRIVATE (gc)->bg_pixel;
}

/**
 * bdk_gc_set_subwindow:
 * @gc: a #BdkGC.
 * @mode: the subwindow mode.
 * 
 * Sets how drawing with this GC on a window will affect child
 * windows of that window. 
 *
 * Deprecated: 2.22: There is no replacement. If you need to control
 * subwindows, you must use drawing operations of the underlying window
 * system manually. Bairo will always use %BDK_INCLUDE_INFERIORS on sources
 * and masks and %BDK_CLIP_BY_CHILDREN on targets.
 **/
void
bdk_gc_set_subwindow (BdkGC	       *gc,
		      BdkSubwindowMode	mode)
{
  BdkGCValues values;
  BdkGCPrivate *priv = BDK_GC_GET_PRIVATE (gc);

  g_return_if_fail (BDK_IS_GC (gc));

  /* This could get called a lot to reset the subwindow mode in
     the client side clipping, so bail out early */ 
  if (priv->subwindow_mode == mode)
    return;
  
  values.subwindow_mode = mode;
  bdk_gc_set_values (gc, &values, BDK_GC_SUBWINDOW);
}

BdkSubwindowMode
_bdk_gc_get_subwindow (BdkGC *gc)
{
  BdkGCPrivate *priv = BDK_GC_GET_PRIVATE (gc);

  return priv->subwindow_mode;
}

/**
 * bdk_gc_set_exposures:
 * @gc: a #BdkGC.
 * @exposures: if %TRUE, exposure events will be generated.
 * 
 * Sets whether copying non-visible portions of a drawable
 * using this graphics context generate exposure events
 * for the corresponding rebunnyions of the destination
 * drawable. (See bdk_draw_drawable()).
 *
 * Deprecated: 2.22: There is no replacement. If you need to control
 * exposures, you must use drawing operations of the underlying window
 * system or use bdk_window_invalidate_rect(). Bairo will never
 * generate exposures.
 **/
void
bdk_gc_set_exposures (BdkGC     *gc,
		      bboolean   exposures)
{
  BdkGCValues values;

  g_return_if_fail (BDK_IS_GC (gc));

  values.graphics_exposures = exposures;
  bdk_gc_set_values (gc, &values, BDK_GC_EXPOSURES);
}

/**
 * bdk_gc_set_line_attributes:
 * @gc: a #BdkGC.
 * @line_width: the width of lines.
 * @line_style: the dash-style for lines.
 * @cap_style: the manner in which the ends of lines are drawn.
 * @join_style: the in which lines are joined together.
 * 
 * Sets various attributes of how lines are drawn. See
 * the corresponding members of #BdkGCValues for full
 * explanations of the arguments.
 *
 * Deprecated: 2.22: Use the Bairo functions bairo_set_line_width(),
 * bairo_set_line_join(), bairo_set_line_cap() and bairo_set_dash()
 * to affect the stroking behavior in Bairo. Keep in mind that the default
 * attributes of a #bairo_t are different from the default attributes of
 * a #BdkGC.
 **/
void
bdk_gc_set_line_attributes (BdkGC	*gc,
			    bint	 line_width,
			    BdkLineStyle line_style,
			    BdkCapStyle	 cap_style,
			    BdkJoinStyle join_style)
{
  BdkGCValues values;

  values.line_width = line_width;
  values.line_style = line_style;
  values.cap_style = cap_style;
  values.join_style = join_style;

  bdk_gc_set_values (gc, &values,
		     BDK_GC_LINE_WIDTH |
		     BDK_GC_LINE_STYLE |
		     BDK_GC_CAP_STYLE |
		     BDK_GC_JOIN_STYLE);
}

/**
 * bdk_gc_set_dashes:
 * @gc: a #BdkGC.
 * @dash_offset: the phase of the dash pattern.
 * @dash_list: an array of dash lengths.
 * @n: the number of elements in @dash_list.
 * 
 * Sets the way dashed-lines are drawn. Lines will be
 * drawn with alternating on and off segments of the
 * lengths specified in @dash_list. The manner in
 * which the on and off segments are drawn is determined
 * by the @line_style value of the GC. (This can
 * be changed with bdk_gc_set_line_attributes().)
 *
 * The @dash_offset defines the phase of the pattern, 
 * specifying how many pixels into the dash-list the pattern 
 * should actually begin.
 *
 * Deprecated: 2.22: Use bairo_set_dash() to set the dash in Bairo.
 **/
void
bdk_gc_set_dashes (BdkGC *gc,
		   bint	  dash_offset,
		   bint8  dash_list[],
		   bint   n)
{
  g_return_if_fail (BDK_IS_GC (gc));
  g_return_if_fail (dash_list != NULL);

  BDK_GC_GET_CLASS (gc)->set_dashes (gc, dash_offset, dash_list, n);
}

/**
 * bdk_gc_offset:
 * @gc: a #BdkGC
 * @x_offset: amount by which to offset the GC in the X direction
 * @y_offset: amount by which to offset the GC in the Y direction
 * 
 * Offset attributes such as the clip and tile-stipple origins
 * of the GC so that drawing at x - x_offset, y - y_offset with
 * the offset GC  has the same effect as drawing at x, y with the original
 * GC.
 *
 * Deprecated: 2.22: There is no direct replacement, as this is just a
 * convenience function for bdk_gc_set_ts_origin and bdk_gc_set_clip_origin().
 **/
void
bdk_gc_offset (BdkGC *gc,
	       bint   x_offset,
	       bint   y_offset)
{
  if (x_offset != 0 || y_offset != 0)
    {
      BdkGCValues values;

      values.clip_x_origin = gc->clip_x_origin - x_offset;
      values.clip_y_origin = gc->clip_y_origin - y_offset;
      values.ts_x_origin = gc->ts_x_origin - x_offset;
      values.ts_y_origin = gc->ts_y_origin - y_offset;
      
      bdk_gc_set_values (gc, &values,
			 BDK_GC_CLIP_X_ORIGIN |
			 BDK_GC_CLIP_Y_ORIGIN |
			 BDK_GC_TS_X_ORIGIN |
			 BDK_GC_TS_Y_ORIGIN);
    }
}

/**
 * bdk_gc_copy:
 * @dst_gc: the destination graphics context.
 * @src_gc: the source graphics context.
 * 
 * Copy the set of values from one graphics context
 * onto another graphics context.
 *
 * Deprecated: 2.22: Use Bairo for drawing. bairo_save() and bairo_restore()
 * can be helpful in cases where you'd have copied a #BdkGC.
 **/
void
bdk_gc_copy (BdkGC *dst_gc,
	     BdkGC *src_gc)
{
  BdkGCPrivate *dst_priv, *src_priv;
  
  g_return_if_fail (BDK_IS_GC (dst_gc));
  g_return_if_fail (BDK_IS_GC (src_gc));

  dst_priv = BDK_GC_GET_PRIVATE (dst_gc);
  src_priv = BDK_GC_GET_PRIVATE (src_gc);

  _bdk_windowing_gc_copy (dst_gc, src_gc);

  dst_gc->clip_x_origin = src_gc->clip_x_origin;
  dst_gc->clip_y_origin = src_gc->clip_y_origin;
  dst_gc->ts_x_origin = src_gc->ts_x_origin;
  dst_gc->ts_y_origin = src_gc->ts_y_origin;

  if (src_gc->colormap)
    g_object_ref (src_gc->colormap);

  if (dst_gc->colormap)
    g_object_unref (dst_gc->colormap);

  dst_gc->colormap = src_gc->colormap;

  if (dst_priv->clip_rebunnyion)
    bdk_rebunnyion_destroy (dst_priv->clip_rebunnyion);

  if (src_priv->clip_rebunnyion)
    dst_priv->clip_rebunnyion = bdk_rebunnyion_copy (src_priv->clip_rebunnyion);
  else
    dst_priv->clip_rebunnyion = NULL;

  dst_priv->rebunnyion_tag_applied = src_priv->rebunnyion_tag_applied;
  
  if (dst_priv->old_clip_rebunnyion)
    bdk_rebunnyion_destroy (dst_priv->old_clip_rebunnyion);

  if (src_priv->old_clip_rebunnyion)
    dst_priv->old_clip_rebunnyion = bdk_rebunnyion_copy (src_priv->old_clip_rebunnyion);
  else
    dst_priv->old_clip_rebunnyion = NULL;

  if (src_priv->clip_mask)
    dst_priv->clip_mask = g_object_ref (src_priv->clip_mask);
  else
    dst_priv->clip_mask = NULL;
  
  if (src_priv->old_clip_mask)
    dst_priv->old_clip_mask = g_object_ref (src_priv->old_clip_mask);
  else
    dst_priv->old_clip_mask = NULL;
  
  dst_priv->fill = src_priv->fill;
  
  if (dst_priv->stipple)
    g_object_unref (dst_priv->stipple);
  dst_priv->stipple = src_priv->stipple;
  if (dst_priv->stipple)
    g_object_ref (dst_priv->stipple);
  
  if (dst_priv->tile)
    g_object_unref (dst_priv->tile);
  dst_priv->tile = src_priv->tile;
  if (dst_priv->tile)
    g_object_ref (dst_priv->tile);

  dst_priv->fg_pixel = src_priv->fg_pixel;
  dst_priv->bg_pixel = src_priv->bg_pixel;
  dst_priv->subwindow_mode = src_priv->subwindow_mode;
  dst_priv->exposures = src_priv->exposures;
}

/**
 * bdk_gc_set_colormap:
 * @gc: a #BdkGC
 * @colormap: a #BdkColormap
 * 
 * Sets the colormap for the GC to the given colormap. The depth
 * of the colormap's visual must match the depth of the drawable
 * for which the GC was created.
 *
 * Deprecated: 2.22: There is no replacement. Bairo handles colormaps
 * automatically, so there is no need to care about them.
 **/
void
bdk_gc_set_colormap (BdkGC       *gc,
		     BdkColormap *colormap)
{
  g_return_if_fail (BDK_IS_GC (gc));
  g_return_if_fail (BDK_IS_COLORMAP (colormap));

  if (gc->colormap != colormap)
    {
      if (gc->colormap)
	g_object_unref (gc->colormap);

      gc->colormap = colormap;
      g_object_ref (gc->colormap);
    }
    
}

/**
 * bdk_gc_get_colormap:
 * @gc: a #BdkGC
 * 
 * Retrieves the colormap for a given GC, if it exists.
 * A GC will have a colormap if the drawable for which it was created
 * has a colormap, or if a colormap was set explicitely with
 * bdk_gc_set_colormap.
 * 
 * Return value: the colormap of @gc, or %NULL if @gc doesn't have one.
 *
 * Deprecated: 2.22: There is no replacement. Bairo handles colormaps
 * automatically, so there is no need to care about them.
 **/
BdkColormap *
bdk_gc_get_colormap (BdkGC *gc)
{
  g_return_val_if_fail (BDK_IS_GC (gc), NULL);

  return gc->colormap;
}

static BdkColormap *
bdk_gc_get_colormap_warn (BdkGC *gc)
{
  BdkColormap *colormap = bdk_gc_get_colormap (gc);
  if (!colormap)
    {
      g_warning ("bdk_gc_set_rgb_fg_color() and bdk_gc_set_rgb_bg_color() can\n"
		 "only be used on GC's with a colormap. A GC will have a colormap\n"
		 "if it is created for a drawable with a colormap, or if a\n"
		 "colormap has been set explicitly with bdk_gc_set_colormap.\n");
      return NULL;
    }

  return colormap;
}

/**
 * bdk_gc_set_rgb_fg_color:
 * @gc: a #BdkGC
 * @color: an unallocated #BdkColor.
 * 
 * Set the foreground color of a GC using an unallocated color. The
 * pixel value for the color will be determined using BdkRGB. If the
 * colormap for the GC has not previously been initialized for BdkRGB,
 * then for pseudo-color colormaps (colormaps with a small modifiable
 * number of colors), a colorcube will be allocated in the colormap.
 * 
 * Calling this function for a GC without a colormap is an error.
 *
 * Deprecated: 2.22: Use bdk_bairo_set_source_color() instead.
 **/
void
bdk_gc_set_rgb_fg_color (BdkGC          *gc,
			 const BdkColor *color)
{
  BdkColormap *cmap;
  BdkColor tmp_color;

  g_return_if_fail (BDK_IS_GC (gc));
  g_return_if_fail (color != NULL);

  cmap = bdk_gc_get_colormap_warn (gc);
  if (!cmap)
    return;

  tmp_color = *color;
  bdk_rgb_find_color (cmap, &tmp_color);
  bdk_gc_set_foreground (gc, &tmp_color);
}

/**
 * bdk_gc_set_rgb_bg_color:
 * @gc: a #BdkGC
 * @color: an unallocated #BdkColor.
 * 
 * Set the background color of a GC using an unallocated color. The
 * pixel value for the color will be determined using BdkRGB. If the
 * colormap for the GC has not previously been initialized for BdkRGB,
 * then for pseudo-color colormaps (colormaps with a small modifiable
 * number of colors), a colorcube will be allocated in the colormap.
 * 
 * Calling this function for a GC without a colormap is an error.
 *
 * Deprecated: 2.22: Use bdk_bairo_set_source_color() instead.
 **/
void
bdk_gc_set_rgb_bg_color (BdkGC          *gc,
			 const BdkColor *color)
{
  BdkColormap *cmap;
  BdkColor tmp_color;

  g_return_if_fail (BDK_IS_GC (gc));
  g_return_if_fail (color != NULL);

  cmap = bdk_gc_get_colormap_warn (gc);
  if (!cmap)
    return;

  tmp_color = *color;
  bdk_rgb_find_color (cmap, &tmp_color);
  bdk_gc_set_background (gc, &tmp_color);
}

static bairo_surface_t *
make_stipple_tile_surface (bairo_t   *cr,
			   BdkBitmap *stipple,
			   BdkColor  *foreground,
			   BdkColor  *background)
{
  bairo_t *tmp_cr;
  bairo_surface_t *surface; 
  bairo_surface_t *alpha_surface;
  bint width, height;

  bdk_drawable_get_size (stipple,
			 &width, &height);
  
  alpha_surface = _bdk_drawable_ref_bairo_surface (stipple);
  
  surface = bairo_surface_create_similar (bairo_get_target (cr),
					  BAIRO_CONTENT_COLOR_ALPHA,
					  width, height);

  tmp_cr = bairo_create (surface);
  
  bairo_set_operator (tmp_cr, BAIRO_OPERATOR_SOURCE);
 
  if (background)
      bdk_bairo_set_source_color (tmp_cr, background);
  else
      bairo_set_source_rgba (tmp_cr, 0, 0, 0 ,0);

  bairo_paint (tmp_cr);

  bairo_set_operator (tmp_cr, BAIRO_OPERATOR_OVER);

  bdk_bairo_set_source_color (tmp_cr, foreground);
  bairo_mask_surface (tmp_cr, alpha_surface, 0, 0);
  
  bairo_destroy (tmp_cr);
  bairo_surface_destroy (alpha_surface);

  return surface;
}

static void
gc_get_foreground (BdkGC    *gc,
		   BdkColor *color)
{
  BdkGCPrivate *priv = BDK_GC_GET_PRIVATE (gc);
  
  color->pixel = priv->bg_pixel;

  if (gc->colormap)
    bdk_colormap_query_color (gc->colormap, priv->fg_pixel, color);
  else
    g_warning ("No colormap in gc_get_foreground");
}

static void
gc_get_background (BdkGC    *gc,
		   BdkColor *color)
{
  BdkGCPrivate *priv = BDK_GC_GET_PRIVATE (gc);
  
  color->pixel = priv->bg_pixel;

  if (gc->colormap)
    bdk_colormap_query_color (gc->colormap, priv->bg_pixel, color);
  else
    g_warning ("No colormap in gc_get_background");
}

/**
 * _bdk_gc_update_context:
 * @gc: a #BdkGC
 * @cr: a #bairo_t
 * @override_foreground: a foreground color to use to override the
 *   foreground color of the GC
 * @override_stipple: a stipple pattern to use to override the
 *   stipple from the GC. If this is present and the fill mode
 *   of the GC isn't %BDK_STIPPLED or %BDK_OPAQUE_STIPPLED
 *   the fill mode will be forced to %BDK_STIPPLED
 * @gc_changed: pass %FALSE if the @gc has not changed since the
 *     last call to this function
 * @target_drawable: The drawable you're drawing in. If passed in
 *     this is used for client side window clip emulation.
 * 
 * Set the attributes of a bairo context to match those of a #BdkGC
 * as far as possible. Some aspects of a #BdkGC, such as clip masks
 * and functions other than %BDK_COPY are not currently handled.
 **/
void
_bdk_gc_update_context (BdkGC          *gc,
                        bairo_t        *cr,
                        const BdkColor *override_foreground,
                        BdkBitmap      *override_stipple,
                        bboolean        gc_changed,
			BdkDrawable    *target_drawable)
{
  BdkGCPrivate *priv;
  BdkFill fill;
  BdkColor foreground;
  BdkColor background;
  bairo_surface_t *tile_surface = NULL;
  BdkBitmap *stipple = NULL;

  g_return_if_fail (BDK_IS_GC (gc));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (override_stipple == NULL || BDK_IS_PIXMAP (override_stipple));

  priv = BDK_GC_GET_PRIVATE (gc);

  _bdk_gc_remove_drawable_clip (gc);

  fill = priv->fill;
  if (override_stipple && fill != BDK_OPAQUE_STIPPLED)
    fill = BDK_STIPPLED;

  if (fill != BDK_TILED)
    {
      if (override_foreground)
	foreground = *override_foreground;
      else
	gc_get_foreground (gc, &foreground);
    }

  if (fill == BDK_OPAQUE_STIPPLED)
    gc_get_background (gc, &background);


  switch (fill)
    {
    case BDK_SOLID:
      break;
    case BDK_TILED:
      if (!priv->tile)
	fill = BDK_SOLID;
      break;
    case BDK_STIPPLED:
    case BDK_OPAQUE_STIPPLED:
      if (override_stipple)
	stipple = override_stipple;
      else
	stipple = priv->stipple;
      
      if (!stipple)
	fill = BDK_SOLID;
      break;
    }
  
  switch (fill)
    {
    case BDK_SOLID:
      bdk_bairo_set_source_color (cr, &foreground);
      break;
    case BDK_TILED:
      tile_surface = _bdk_drawable_ref_bairo_surface (priv->tile);
      break;
    case BDK_STIPPLED:
      tile_surface = make_stipple_tile_surface (cr, stipple, &foreground, NULL);
      break;
    case BDK_OPAQUE_STIPPLED:
      tile_surface = make_stipple_tile_surface (cr, stipple, &foreground, &background);
      break;
    }

  /* Tiles, stipples, and clip rebunnyions are all specified in device space,
   * not user space. For the clip rebunnyion, we can simply change the matrix,
   * clip, then clip back, but for the source pattern, we need to
   * compute the right matrix.
   *
   * What we want is:
   *
   *     CTM_inverse * Pattern_matrix = Translate(- ts_x, - ts_y)
   *
   * (So that ts_x, ts_y in device space is taken to 0,0 in pattern
   * space). So, pattern_matrix = CTM * Translate(- ts_x, - tx_y);
   */

  if (tile_surface)
    {
      bairo_pattern_t *pattern = bairo_pattern_create_for_surface (tile_surface);
      bairo_matrix_t user_to_device;
      bairo_matrix_t user_to_pattern;
      bairo_matrix_t device_to_pattern;

      bairo_get_matrix (cr, &user_to_device);
      bairo_matrix_init_translate (&device_to_pattern,
				   - gc->ts_x_origin, - gc->ts_y_origin);
      bairo_matrix_multiply (&user_to_pattern,
			     &user_to_device, &device_to_pattern);
      
      bairo_pattern_set_matrix (pattern, &user_to_pattern);
      bairo_pattern_set_extend (pattern, BAIRO_EXTEND_REPEAT);
      bairo_set_source (cr, pattern);
      
      bairo_surface_destroy (tile_surface);
      bairo_pattern_destroy (pattern);
    }

  if (!gc_changed)
    return;

  bairo_reset_clip (cr);
  /* The reset above resets the window clip rect, so we want to re-set that */
  if (target_drawable && BDK_DRAWABLE_GET_CLASS (target_drawable)->set_bairo_clip)
    BDK_DRAWABLE_GET_CLASS (target_drawable)->set_bairo_clip (target_drawable, cr);

  if (priv->clip_rebunnyion)
    {
      bairo_save (cr);

      bairo_identity_matrix (cr);
      bairo_translate (cr, gc->clip_x_origin, gc->clip_y_origin);

      bairo_new_path (cr);
      bdk_bairo_rebunnyion (cr, priv->clip_rebunnyion);

      bairo_restore (cr);

      bairo_clip (cr);
    }

}


#define __BDK_GC_C__
#include "bdkaliasdef.c"
