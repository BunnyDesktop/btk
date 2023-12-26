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
#include <math.h>
#include <bango/bangobairo.h>
#include <bdk-pixbuf/bdk-pixbuf.h>
#include "bdkbairo.h"
#include "bdkdrawable.h"
#include "bdkinternals.h"
#include "bdkwindow.h"
#include "bdkscreen.h"
#include "bdkpixbuf.h"
#include "bdkalias.h"

static BdkImage*    bdk_drawable_real_get_image (BdkDrawable     *drawable,
						 gint             x,
						 gint             y,
						 gint             width,
						 gint             height);
static BdkDrawable* bdk_drawable_real_get_composite_drawable (BdkDrawable  *drawable,
							      gint          x,
							      gint          y,
							      gint          width,
							      gint          height,
							      gint         *composite_x_offset,
							      gint         *composite_y_offset);
static BdkRebunnyion *  bdk_drawable_real_get_visible_rebunnyion     (BdkDrawable  *drawable);
static void         bdk_drawable_real_draw_pixbuf            (BdkDrawable  *drawable,
							      BdkGC        *gc,
							      BdkPixbuf    *pixbuf,
							      gint          src_x,
							      gint          src_y,
							      gint          dest_x,
							      gint          dest_y,
							      gint          width,
							      gint          height,
							      BdkRgbDither  dither,
							      gint          x_dither,
							      gint          y_dither);
static void         bdk_drawable_real_draw_drawable          (BdkDrawable  *drawable,
							      BdkGC	   *gc,
							      BdkDrawable  *src,
							      gint          xsrc,
							      gint	    ysrc,
							      gint	    xdest,
							      gint	    ydest,
							      gint	    width,
							      gint	    height);
     

G_DEFINE_ABSTRACT_TYPE (BdkDrawable, bdk_drawable, G_TYPE_OBJECT)

static void
bdk_drawable_class_init (BdkDrawableClass *klass)
{
  klass->get_image = bdk_drawable_real_get_image;
  klass->get_composite_drawable = bdk_drawable_real_get_composite_drawable;
  /* Default implementation for clip and visible rebunnyion is the same */
  klass->get_clip_rebunnyion = bdk_drawable_real_get_visible_rebunnyion;
  klass->get_visible_rebunnyion = bdk_drawable_real_get_visible_rebunnyion;
  klass->draw_pixbuf = bdk_drawable_real_draw_pixbuf;
  klass->draw_drawable = bdk_drawable_real_draw_drawable;
}

static void
bdk_drawable_init (BdkDrawable *drawable)
{
}

/* Manipulation of drawables
 */

/**
 * bdk_drawable_set_data:
 * @drawable: a #BdkDrawable
 * @key: name to store the data under
 * @data: arbitrary data
 * @destroy_func: (allow-none): function to free @data, or %NULL
 *
 * This function is equivalent to g_object_set_data(),
 * the #GObject variant should be used instead.
 * 
 **/
void          
bdk_drawable_set_data (BdkDrawable   *drawable,
		       const gchar   *key,
		       gpointer	      data,
		       GDestroyNotify destroy_func)
{
  g_return_if_fail (BDK_IS_DRAWABLE (drawable));
  
  g_object_set_qdata_full (G_OBJECT (drawable),
                           g_quark_from_string (key),
                           data,
                           destroy_func);
}

/**
 * bdk_drawable_get_data:
 * @drawable: a #BdkDrawable
 * @key: name the data was stored under
 * 
 * Equivalent to g_object_get_data(); the #GObject variant should be
 * used instead.
 * 
 * Return value: the data stored at @key
 **/
gpointer
bdk_drawable_get_data (BdkDrawable   *drawable,
		       const gchar   *key)
{
  g_return_val_if_fail (BDK_IS_DRAWABLE (drawable), NULL);
  
  return g_object_get_qdata (G_OBJECT (drawable),
                             g_quark_try_string (key));
}

/**
 * bdk_drawable_get_size:
 * @drawable: a #BdkDrawable
 * @width: (out) (allow-none): location to store drawable's width, or %NULL
 * @height: (out) (allow-none): location to store drawable's height, or %NULL
 *
 * Fills *@width and *@height with the size of @drawable.
 * @width or @height can be %NULL if you only want the other one.
 *
 * On the X11 platform, if @drawable is a #BdkWindow, the returned
 * size is the size reported in the most-recently-processed configure
 * event, rather than the current size on the X server.
 *
 * Deprecated: 2.24: Use bdk_window_get_width() and bdk_window_get_height() for
 *             #BdkWindows. Use bdk_pixmap_get_size() for #BdkPixmaps.
 */
void
bdk_drawable_get_size (BdkDrawable *drawable,
		       gint        *width,
		       gint        *height)
{
  g_return_if_fail (BDK_IS_DRAWABLE (drawable));

  BDK_DRAWABLE_GET_CLASS (drawable)->get_size (drawable, width, height);  
}

/**
 * bdk_drawable_get_visual:
 * @drawable: a #BdkDrawable
 * 
 * Gets the #BdkVisual describing the pixel format of @drawable.
 * 
 * Return value: a #BdkVisual
 *
 * Deprecated: 2.24: Use bdk_window_get_visual()
 */
BdkVisual*
bdk_drawable_get_visual (BdkDrawable *drawable)
{
  g_return_val_if_fail (BDK_IS_DRAWABLE (drawable), NULL);
  
  return BDK_DRAWABLE_GET_CLASS (drawable)->get_visual (drawable);
}

/**
 * bdk_drawable_get_depth:
 * @drawable: a #BdkDrawable
 * 
 * Obtains the bit depth of the drawable, that is, the number of bits
 * that make up a pixel in the drawable's visual. Examples are 8 bits
 * per pixel, 24 bits per pixel, etc.
 * 
 * Return value: number of bits per pixel
 **/
gint
bdk_drawable_get_depth (BdkDrawable *drawable)
{
  g_return_val_if_fail (BDK_IS_DRAWABLE (drawable), 0);

  return BDK_DRAWABLE_GET_CLASS (drawable)->get_depth (drawable);
}
/**
 * bdk_drawable_get_screen:
 * @drawable: a #BdkDrawable
 * 
 * Gets the #BdkScreen associated with a #BdkDrawable.
 * 
 * Return value: the #BdkScreen associated with @drawable
 *
 * Since: 2.2
 *
 * Deprecated: 2.24: Use bdk_window_get_screen() instead
 **/
BdkScreen*
bdk_drawable_get_screen (BdkDrawable *drawable)
{
  g_return_val_if_fail (BDK_IS_DRAWABLE (drawable), NULL);

  return BDK_DRAWABLE_GET_CLASS (drawable)->get_screen (drawable);
}

/**
 * bdk_drawable_get_display:
 * @drawable: a #BdkDrawable
 * 
 * Gets the #BdkDisplay associated with a #BdkDrawable.
 * 
 * Return value: the #BdkDisplay associated with @drawable
 *
 * Since: 2.2
 *
 * Deprecated: 2.24: Use bdk_window_get_display() instead
 **/
BdkDisplay*
bdk_drawable_get_display (BdkDrawable *drawable)
{
  g_return_val_if_fail (BDK_IS_DRAWABLE (drawable), NULL);

  return bdk_screen_get_display (bdk_drawable_get_screen (drawable));
}

/**
 * bdk_drawable_set_colormap:
 * @drawable: a #BdkDrawable
 * @colormap: a #BdkColormap
 *
 * Sets the colormap associated with @drawable. Normally this will
 * happen automatically when the drawable is created; you only need to
 * use this function if the drawable-creating function did not have a
 * way to determine the colormap, and you then use drawable operations
 * that require a colormap. The colormap for all drawables and
 * graphics contexts you intend to use together should match. i.e.
 * when using a #BdkGC to draw to a drawable, or copying one drawable
 * to another, the colormaps should match.
 * 
 **/
void
bdk_drawable_set_colormap (BdkDrawable *drawable,
                           BdkColormap *cmap)
{
  g_return_if_fail (BDK_IS_DRAWABLE (drawable));
  g_return_if_fail (cmap == NULL || bdk_drawable_get_depth (drawable)
                    == cmap->visual->depth);

  BDK_DRAWABLE_GET_CLASS (drawable)->set_colormap (drawable, cmap);
}

/**
 * bdk_drawable_get_colormap:
 * @drawable: a #BdkDrawable
 * 
 * Gets the colormap for @drawable, if one is set; returns
 * %NULL otherwise.
 * 
 * Return value: the colormap, or %NULL
 **/
BdkColormap*
bdk_drawable_get_colormap (BdkDrawable *drawable)
{
  g_return_val_if_fail (BDK_IS_DRAWABLE (drawable), NULL);

  return BDK_DRAWABLE_GET_CLASS (drawable)->get_colormap (drawable);
}

/**
 * bdk_drawable_ref:
 * @drawable: a #BdkDrawable
 * 
 * Deprecated equivalent of calling g_object_ref() on @drawable.
 * (Drawables were not objects in previous versions of BDK.)
 * 
 * Return value: the same @drawable passed in
 *
 * Deprecated: 2.0: Use g_object_ref() instead.
 **/
BdkDrawable*
bdk_drawable_ref (BdkDrawable *drawable)
{
  return (BdkDrawable *) g_object_ref (drawable);
}

/**
 * bdk_drawable_unref:
 * @drawable: a #BdkDrawable
 *
 * Deprecated equivalent of calling g_object_unref() on @drawable.
 * 
 * Deprecated: 2.0: Use g_object_unref() instead.
 **/
void
bdk_drawable_unref (BdkDrawable *drawable)
{
  g_return_if_fail (BDK_IS_DRAWABLE (drawable));

  g_object_unref (drawable);
}

/* Drawing
 */

/**
 * bdk_draw_point:
 * @drawable: a #BdkDrawable (a #BdkWindow or a #BdkPixmap).
 * @gc: a #BdkGC.
 * @x: the x coordinate of the point.
 * @y: the y coordinate of the point.
 * 
 * Draws a point, using the foreground color and other attributes of 
 * the #BdkGC.
 *
 * Deprecated: 2.22: Use bairo_rectangle() and bairo_fill() or 
 * bairo_move_to() and bairo_stroke() instead.
 **/
void
bdk_draw_point (BdkDrawable *drawable,
                BdkGC       *gc,
                gint         x,
                gint         y)
{
  BdkPoint point;

  g_return_if_fail (BDK_IS_DRAWABLE (drawable));
  g_return_if_fail (BDK_IS_GC (gc));

  point.x = x;
  point.y = y;
  
  BDK_DRAWABLE_GET_CLASS (drawable)->draw_points (drawable, gc, &point, 1);
}

/**
 * bdk_draw_line:
 * @drawable: a #BdkDrawable (a #BdkWindow or a #BdkPixmap). 
 * @gc: a #BdkGC.
 * @x1_: the x coordinate of the start point.
 * @y1_: the y coordinate of the start point.
 * @x2_: the x coordinate of the end point.
 * @y2_: the y coordinate of the end point.
 * 
 * Draws a line, using the foreground color and other attributes of 
 * the #BdkGC.
 *
 * Deprecated: 2.22: Use bairo_line_to() and bairo_stroke() instead.
 * Be aware that the default line width in Bairo is 2 pixels and that your
 * coordinates need to describe the center of the line. To draw a single
 * pixel wide pixel-aligned line, you would use:
 * |[bairo_set_line_width (cr, 1.0);
 * bairo_set_line_cap (cr, BAIRO_LINE_CAP_SQUARE);
 * bairo_move_to (cr, 0.5, 0.5);
 * bairo_line_to (cr, 9.5, 0.5);
 * bairo_stroke (cr);]|
 * See also <ulink url="http://bairographics.org/FAQ/#sharp_lines">the Bairo
 * FAQ</ulink> on this topic.
 **/
void
bdk_draw_line (BdkDrawable *drawable,
	       BdkGC       *gc,
	       gint         x1,
	       gint         y1,
	       gint         x2,
	       gint         y2)
{
  BdkSegment segment;

  g_return_if_fail (BDK_IS_DRAWABLE (drawable));
  g_return_if_fail (BDK_IS_GC (gc));

  segment.x1 = x1;
  segment.y1 = y1;
  segment.x2 = x2;
  segment.y2 = y2;
  BDK_DRAWABLE_GET_CLASS (drawable)->draw_segments (drawable, gc, &segment, 1);
}

/**
 * bdk_draw_rectangle:
 * @drawable: a #BdkDrawable (a #BdkWindow or a #BdkPixmap).
 * @gc: a #BdkGC.
 * @filled: %TRUE if the rectangle should be filled.
 * @x: the x coordinate of the left edge of the rectangle.
 * @y: the y coordinate of the top edge of the rectangle.
 * @width: the width of the rectangle.
 * @height: the height of the rectangle.
 * 
 * Draws a rectangular outline or filled rectangle, using the foreground color
 * and other attributes of the #BdkGC.
 *
 * A rectangle drawn filled is 1 pixel smaller in both dimensions than a 
 * rectangle outlined. Calling 
 * <literal>bdk_draw_rectangle (window, gc, TRUE, 0, 0, 20, 20)</literal> 
 * results in a filled rectangle 20 pixels wide and 20 pixels high. Calling
 * <literal>bdk_draw_rectangle (window, gc, FALSE, 0, 0, 20, 20)</literal> 
 * results in an outlined rectangle with corners at (0, 0), (0, 20), (20, 20),
 * and (20, 0), which makes it 21 pixels wide and 21 pixels high.
 *
 * Deprecated: 2.22: Use bairo_rectangle() and bairo_fill() or bairo_stroke()
 * instead. For stroking, the same caveats for converting code apply as for
 * bdk_draw_line().
 **/
void
bdk_draw_rectangle (BdkDrawable *drawable,
		    BdkGC       *gc,
		    gboolean     filled,
		    gint         x,
		    gint         y,
		    gint         width,
		    gint         height)
{  
  g_return_if_fail (BDK_IS_DRAWABLE (drawable));
  g_return_if_fail (BDK_IS_GC (gc));

  if (width < 0 || height < 0)
    {
      gint real_width;
      gint real_height;
      
      bdk_drawable_get_size (drawable, &real_width, &real_height);

      if (width < 0)
        width = real_width;
      if (height < 0)
        height = real_height;
    }

  BDK_DRAWABLE_GET_CLASS (drawable)->draw_rectangle (drawable, gc, filled, x, y,
                                                     width, height);
}

/**
 * bdk_draw_arc:
 * @drawable: a #BdkDrawable (a #BdkWindow or a #BdkPixmap).
 * @gc: a #BdkGC.
 * @filled: %TRUE if the arc should be filled, producing a 'pie slice'.
 * @x: the x coordinate of the left edge of the bounding rectangle.
 * @y: the y coordinate of the top edge of the bounding rectangle.
 * @width: the width of the bounding rectangle.
 * @height: the height of the bounding rectangle.
 * @angle1: the start angle of the arc, relative to the 3 o'clock position,
 *     counter-clockwise, in 1/64ths of a degree.
 * @angle2: the end angle of the arc, relative to @angle1, in 1/64ths 
 *     of a degree.
 * 
 * Draws an arc or a filled 'pie slice'. The arc is defined by the bounding
 * rectangle of the entire ellipse, and the start and end angles of the part 
 * of the ellipse to be drawn.
 *
 * Deprecated: 2.22: Use bairo_arc() and bairo_fill() or bairo_stroke()
 * instead. Note that arcs just like any drawing operation in Bairo are
 * antialiased unless you call bairo_set_antialias().
 **/
void
bdk_draw_arc (BdkDrawable *drawable,
	      BdkGC       *gc,
	      gboolean     filled,
	      gint         x,
	      gint         y,
	      gint         width,
	      gint         height,
	      gint         angle1,
	      gint         angle2)
{  
  g_return_if_fail (BDK_IS_DRAWABLE (drawable));
  g_return_if_fail (BDK_IS_GC (gc));

  if (width < 0 || height < 0)
    {
      gint real_width;
      gint real_height;
      
      bdk_drawable_get_size (drawable, &real_width, &real_height);

      if (width < 0)
        width = real_width;
      if (height < 0)
        height = real_height;
    }

  BDK_DRAWABLE_GET_CLASS (drawable)->draw_arc (drawable, gc, filled,
                                               x, y, width, height, angle1, angle2);
}

/**
 * bdk_draw_polygon:
 * @drawable: a #BdkDrawable (a #BdkWindow or a #BdkPixmap).
 * @gc: a #BdkGC.
 * @filled: %TRUE if the polygon should be filled. The polygon is closed
 *     automatically, connecting the last point to the first point if 
 *     necessary.
 * @points: an array of #BdkPoint structures specifying the points making 
 *     up the polygon.
 * @n_points: the number of points.
 * 
 * Draws an outlined or filled polygon.
 *
 * Deprecated: 2.22: Use bairo_line_to() or bairo_append_path() and
 * bairo_fill() or bairo_stroke() instead.
 **/
void
bdk_draw_polygon (BdkDrawable    *drawable,
		  BdkGC          *gc,
		  gboolean        filled,
		  const BdkPoint *points,
		  gint            n_points)
{
  g_return_if_fail (BDK_IS_DRAWABLE (drawable));
  g_return_if_fail (BDK_IS_GC (gc));

  BDK_DRAWABLE_GET_CLASS (drawable)->draw_polygon (drawable, gc, filled,
                                                   (BdkPoint *) points,
                                                   n_points);
}

/* bdk_draw_string
 *
 * Modified by Li-Da Lho to draw 16 bits and Multibyte strings
 *
 * Interface changed: add "BdkFont *font" to specify font or fontset explicitely
 */
/**
 * bdk_draw_string:
 * @drawable: a #BdkDrawable (a #BdkWindow or a #BdkPixmap).
 * @font: a #BdkFont.
 * @gc: a #BdkGC.
 * @x: the x coordinate of the left edge of the text.
 * @y: the y coordinate of the baseline of the text.
 * @string:  the string of characters to draw.
 * 
 * Draws a string of characters in the given font or fontset.
 * 
 * Deprecated: 2.4: Use bdk_draw_layout() instead.
 **/
void
bdk_draw_string (BdkDrawable *drawable,
		 BdkFont     *font,
		 BdkGC       *gc,
		 gint         x,
		 gint         y,
		 const gchar *string)
{
  bdk_draw_text (drawable, font, gc, x, y, string, _bdk_font_strlen (font, string));
}

/* bdk_draw_text
 *
 * Modified by Li-Da Lho to draw 16 bits and Multibyte strings
 *
 * Interface changed: add "BdkFont *font" to specify font or fontset explicitely
 */
/**
 * bdk_draw_text:
 * @drawable: a #BdkDrawable (a #BdkWindow or a #BdkPixmap).
 * @font: a #BdkFont.
 * @gc: a #BdkGC.
 * @x: the x coordinate of the left edge of the text.
 * @y: the y coordinate of the baseline of the text.
 * @text:  the characters to draw.
 * @text_length: the number of characters of @text to draw.
 * 
 * Draws a number of characters in the given font or fontset.
 *
 * Deprecated: 2.4: Use bdk_draw_layout() instead.
 **/
void
bdk_draw_text (BdkDrawable *drawable,
	       BdkFont     *font,
	       BdkGC       *gc,
	       gint         x,
	       gint         y,
	       const gchar *text,
	       gint         text_length)
{
  g_return_if_fail (BDK_IS_DRAWABLE (drawable));
  g_return_if_fail (font != NULL);
  g_return_if_fail (BDK_IS_GC (gc));
  g_return_if_fail (text != NULL);

  BDK_DRAWABLE_GET_CLASS (drawable)->draw_text (drawable, font, gc, x, y, text, text_length);
}

/**
 * bdk_draw_text_wc:
 * @drawable: a #BdkDrawable (a #BdkWindow or a #BdkPixmap).
 * @font: a #BdkFont.
 * @gc: a #BdkGC.
 * @x: the x coordinate of the left edge of the text.
 * @y: the y coordinate of the baseline of the text.
 * @text: the wide characters to draw.
 * @text_length: the number of characters to draw.
 * 
 * Draws a number of wide characters using the given font of fontset.
 * If the font is a 1-byte font, the string is converted into 1-byte 
 * characters (discarding the high bytes) before output.
 * 
 * Deprecated: 2.4: Use bdk_draw_layout() instead.
 **/
void
bdk_draw_text_wc (BdkDrawable	 *drawable,
		  BdkFont	 *font,
		  BdkGC		 *gc,
		  gint		  x,
		  gint		  y,
		  const BdkWChar *text,
		  gint		  text_length)
{
  g_return_if_fail (BDK_IS_DRAWABLE (drawable));
  g_return_if_fail (font != NULL);
  g_return_if_fail (BDK_IS_GC (gc));
  g_return_if_fail (text != NULL);

  BDK_DRAWABLE_GET_CLASS (drawable)->draw_text_wc (drawable, font, gc, x, y, text, text_length);
}

/**
 * bdk_draw_drawable:
 * @drawable: a #BdkDrawable
 * @gc: a #BdkGC sharing the drawable's visual and colormap
 * @src: the source #BdkDrawable, which may be the same as @drawable
 * @xsrc: X position in @src of rectangle to draw
 * @ysrc: Y position in @src of rectangle to draw
 * @xdest: X position in @drawable where the rectangle should be drawn
 * @ydest: Y position in @drawable where the rectangle should be drawn
 * @width: width of rectangle to draw, or -1 for entire @src width
 * @height: height of rectangle to draw, or -1 for entire @src height
 *
 * Copies the @width x @height rebunnyion of @src at coordinates (@xsrc,
 * @ysrc) to coordinates (@xdest, @ydest) in @drawable.
 * @width and/or @height may be given as -1, in which case the entire
 * @src drawable will be copied.
 *
 * Most fields in @gc are not used for this operation, but notably the
 * clip mask or clip rebunnyion will be honored.
 *
 * The source and destination drawables must have the same visual and
 * colormap, or errors will result. (On X11, failure to match
 * visual/colormap results in a BadMatch error from the X server.)
 * A common cause of this problem is an attempt to draw a bitmap to
 * a color drawable. The way to draw a bitmap is to set the bitmap as 
 * the stipple on the #BdkGC, set the fill mode to %BDK_STIPPLED, and 
 * then draw the rectangle.
 *
 * Deprecated: 2.22: Use bdk_bairo_set_source_pixmap(), bairo_rectangle()
 * and bairo_fill() to draw pixmap on top of other drawables. Also keep
 * in mind that the limitations on allowed sources do not apply to Bairo.
 **/
void
bdk_draw_drawable (BdkDrawable *drawable,
		   BdkGC       *gc,
		   BdkDrawable *src,
		   gint         xsrc,
		   gint         ysrc,
		   gint         xdest,
		   gint         ydest,
		   gint         width,
		   gint         height)
{
  BdkDrawable *composite;
  gint composite_x_offset = 0;
  gint composite_y_offset = 0;

  g_return_if_fail (BDK_IS_DRAWABLE (drawable));
  g_return_if_fail (BDK_IS_DRAWABLE (src));
  g_return_if_fail (BDK_IS_GC (gc));

  if (width < 0 || height < 0)
    {
      gint real_width;
      gint real_height;
      
      bdk_drawable_get_size (src, &real_width, &real_height);

      if (width < 0)
        width = real_width;
      if (height < 0)
        height = real_height;
    }


  composite =
    BDK_DRAWABLE_GET_CLASS (src)->get_composite_drawable (src,
                                                          xsrc, ysrc,
                                                          width, height,
                                                          &composite_x_offset,
                                                          &composite_y_offset);

  /* TODO: For non-native windows this may copy stuff from other overlapping
     windows. We should clip that and (for windows with bg != None) clear that
     area in the destination instead. */

  if (BDK_DRAWABLE_GET_CLASS (drawable)->draw_drawable_with_src)
    BDK_DRAWABLE_GET_CLASS (drawable)->draw_drawable_with_src (drawable, gc,
							       composite,
							       xsrc - composite_x_offset,
							       ysrc - composite_y_offset,
							       xdest, ydest,
							       width, height,
							       src);
  else /* backwards compat for old out-of-tree implementations of BdkDrawable (are there any?) */
    BDK_DRAWABLE_GET_CLASS (drawable)->draw_drawable (drawable, gc,
						      composite,
						      xsrc - composite_x_offset,
						      ysrc - composite_y_offset,
						      xdest, ydest,
						      width, height);

  g_object_unref (composite);
}

/**
 * bdk_draw_image:
 * @drawable: a #BdkDrawable (a #BdkWindow or a #BdkPixmap).
 * @gc: a #BdkGC.
 * @image: the #BdkImage to draw.
 * @xsrc: the left edge of the source rectangle within @image.
 * @ysrc: the top of the source rectangle within @image.
 * @xdest: the x coordinate of the destination within @drawable.
 * @ydest: the y coordinate of the destination within @drawable.
 * @width: the width of the area to be copied, or -1 to make the area 
 *     extend to the right edge of @image.
 * @height: the height of the area to be copied, or -1 to make the area 
 *     extend to the bottom edge of @image.
 * 
 * Draws a #BdkImage onto a drawable.
 * The depth of the #BdkImage must match the depth of the #BdkDrawable.
 *
 * Deprecated: 2.22: Do not use #BdkImage anymore, instead use Bairo image
 * surfaces.
 **/
void
bdk_draw_image (BdkDrawable *drawable,
		BdkGC       *gc,
		BdkImage    *image,
		gint         xsrc,
		gint         ysrc,
		gint         xdest,
		gint         ydest,
		gint         width,
		gint         height)
{
  g_return_if_fail (BDK_IS_DRAWABLE (drawable));
  g_return_if_fail (BDK_IS_IMAGE (image));
  g_return_if_fail (BDK_IS_GC (gc));

  if (width == -1)
    width = image->width;
  if (height == -1)
    height = image->height;

  BDK_DRAWABLE_GET_CLASS (drawable)->draw_image (drawable, gc, image, xsrc, ysrc,
                                                 xdest, ydest, width, height);
}

/**
 * bdk_draw_pixbuf:
 * @drawable: Destination drawable.
 * @gc: (allow-none): a #BdkGC, used for clipping, or %NULL
 * @pixbuf: a #BdkPixbuf
 * @src_x: Source X coordinate within pixbuf.
 * @src_y: Source Y coordinates within pixbuf.
 * @dest_x: Destination X coordinate within drawable.
 * @dest_y: Destination Y coordinate within drawable.
 * @width: Width of rebunnyion to render, in pixels, or -1 to use pixbuf width.
 * @height: Height of rebunnyion to render, in pixels, or -1 to use pixbuf height.
 * @dither: Dithering mode for #BdkRGB.
 * @x_dither: X offset for dither.
 * @y_dither: Y offset for dither.
 * 
 * Renders a rectangular portion of a pixbuf to a drawable.  The destination
 * drawable must have a colormap. All windows have a colormap, however, pixmaps
 * only have colormap by default if they were created with a non-%NULL window 
 * argument. Otherwise a colormap must be set on them with 
 * bdk_drawable_set_colormap().
 *
 * On older X servers, rendering pixbufs with an alpha channel involves round 
 * trips to the X server, and may be somewhat slow.
 *
 * If BDK is built with the Sun mediaLib library, the bdk_draw_pixbuf
 * function is accelerated using mediaLib, which provides hardware
 * acceleration on Intel, AMD, and Sparc chipsets.  If desired, mediaLib
 * support can be turned off by setting the BDK_DISABLE_MEDIALIB environment
 * variable.
 *
 * Since: 2.2
 *
 * Deprecated: 2.22: Use bdk_bairo_set_source_pixbuf() and bairo_paint() or
 * bairo_rectangle() and bairo_fill() instead.
 **/
void
bdk_draw_pixbuf (BdkDrawable     *drawable,
                 BdkGC           *gc,
                 const BdkPixbuf *pixbuf,
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
  g_return_if_fail (BDK_IS_DRAWABLE (drawable));
  g_return_if_fail (gc == NULL || BDK_IS_GC (gc));
  g_return_if_fail (BDK_IS_PIXBUF (pixbuf));

  if (width == 0 || height == 0)
    return;

  if (width == -1)
    width = bdk_pixbuf_get_width (pixbuf);
  if (height == -1)
    height = bdk_pixbuf_get_height (pixbuf);

  BDK_DRAWABLE_GET_CLASS (drawable)->draw_pixbuf (drawable, gc,
                                                  (BdkPixbuf *) pixbuf,
						  src_x, src_y, dest_x, dest_y,
                                                  width, height,
						  dither, x_dither, y_dither);
}

/**
 * bdk_draw_points:
 * @drawable: a #BdkDrawable (a #BdkWindow or a #BdkPixmap).
 * @gc: a #BdkGC.
 * @points: an array of #BdkPoint structures.
 * @n_points: the number of points to be drawn.
 * 
 * Draws a number of points, using the foreground color and other 
 * attributes of the #BdkGC.
 *
 * Deprecated: 2.22: Use @n_points calls to bairo_rectangle() and
 * bairo_fill() instead.
 **/
void
bdk_draw_points (BdkDrawable    *drawable,
		 BdkGC          *gc,
		 const BdkPoint *points,
		 gint            n_points)
{
  g_return_if_fail (BDK_IS_DRAWABLE (drawable));
  g_return_if_fail ((points != NULL) && (n_points > 0));
  g_return_if_fail (BDK_IS_GC (gc));
  g_return_if_fail (n_points >= 0);

  if (n_points == 0)
    return;

  BDK_DRAWABLE_GET_CLASS (drawable)->draw_points (drawable, gc,
                                                  (BdkPoint *) points, n_points);
}

/**
 * bdk_draw_segments:
 * @drawable: a #BdkDrawable (a #BdkWindow or a #BdkPixmap).
 * @gc: a #BdkGC.
 * @segs: an array of #BdkSegment structures specifying the start and 
 *   end points of the lines to be drawn.
 * @n_segs: the number of line segments to draw, i.e. the size of the 
 *   @segs array.
 * 
 * Draws a number of unconnected lines.
 *
 * Deprecated: 2.22: Use bairo_move_to(), bairo_line_to() and bairo_stroke()
 * instead. See the documentation of bdk_draw_line() for notes on line drawing
 * with Bairo.
 **/
void
bdk_draw_segments (BdkDrawable      *drawable,
		   BdkGC            *gc,
		   const BdkSegment *segs,
		   gint              n_segs)
{
  g_return_if_fail (BDK_IS_DRAWABLE (drawable));

  if (n_segs == 0)
    return;

  g_return_if_fail (segs != NULL);
  g_return_if_fail (BDK_IS_GC (gc));
  g_return_if_fail (n_segs >= 0);

  BDK_DRAWABLE_GET_CLASS (drawable)->draw_segments (drawable, gc,
                                                    (BdkSegment *) segs, n_segs);
}

/**
 * bdk_draw_lines:
 * @drawable: a #BdkDrawable (a #BdkWindow or a #BdkPixmap).
 * @gc: a #BdkGC.
 * @points: an array of #BdkPoint structures specifying the endpoints of the
 * @n_points: the size of the @points array.
 * 
 * Draws a series of lines connecting the given points.
 * The way in which joins between lines are draw is determined by the
 * #BdkCapStyle value in the #BdkGC. This can be set with
 * bdk_gc_set_line_attributes().
 *
 * Deprecated: 2.22: Use bairo_line_to() and bairo_stroke() instead. See the
 * documentation of bdk_draw_line() for notes on line drawing with Bairo.
 **/
void
bdk_draw_lines (BdkDrawable    *drawable,
		BdkGC          *gc,
		const BdkPoint *points,
		gint            n_points)
{
  g_return_if_fail (BDK_IS_DRAWABLE (drawable));
  g_return_if_fail (points != NULL);
  g_return_if_fail (BDK_IS_GC (gc));
  g_return_if_fail (n_points >= 0);

  if (n_points == 0)
    return;

  BDK_DRAWABLE_GET_CLASS (drawable)->draw_lines (drawable, gc,
                                                 (BdkPoint *) points, n_points);
}

static void
real_draw_glyphs (BdkDrawable       *drawable,
		  BdkGC	            *gc,
		  const BangoMatrix *matrix,
		  BangoFont         *font,
		  gdouble            x,
		  gdouble            y,
		  BangoGlyphString  *glyphs)
{
  bairo_t *cr;

  cr = bdk_bairo_create (drawable);
  _bdk_gc_update_context (gc, cr, NULL, NULL, TRUE, drawable);

  if (matrix)
    {
      bairo_matrix_t bairo_matrix;

      bairo_matrix.xx = matrix->xx;
      bairo_matrix.yx = matrix->yx;
      bairo_matrix.xy = matrix->xy;
      bairo_matrix.yy = matrix->yy;
      bairo_matrix.x0 = matrix->x0;
      bairo_matrix.y0 = matrix->y0;
      
      bairo_set_matrix (cr, &bairo_matrix);
    }

  bairo_move_to (cr, x, y);
  bango_bairo_show_glyph_string (cr, font, glyphs);

  bairo_destroy (cr);
}

/**
 * bdk_draw_glyphs:
 * @drawable: a #BdkDrawable
 * @gc: a #BdkGC
 * @font: font to be used
 * @x: X coordinate of baseline origin
 * @y: Y coordinate of baseline origin
 * @glyphs: the glyph string to draw
 *
 * This is a low-level function; 99% of text rendering should be done
 * using bdk_draw_layout() instead.
 *
 * A glyph is a single image in a font. This function draws a sequence of
 * glyphs.  To obtain a sequence of glyphs you have to understand a
 * lot about internationalized text handling, which you don't want to
 * understand; thus, use bdk_draw_layout() instead of this function,
 * bdk_draw_layout() handles the details.
 * 
 * Deprecated: 2.22: Use bango_bairo_show_glyphs() instead.
 **/
void
bdk_draw_glyphs (BdkDrawable      *drawable,
		 BdkGC            *gc,
		 BangoFont        *font,
		 gint              x,
		 gint              y,
		 BangoGlyphString *glyphs)
{
  g_return_if_fail (BDK_IS_DRAWABLE (drawable));
  g_return_if_fail (BDK_IS_GC (gc));
  
  real_draw_glyphs (drawable, gc, NULL, font,
		    x, y, glyphs);
}

/**
 * bdk_draw_glyphs_transformed:
 * @drawable: a #BdkDrawable
 * @gc: a #BdkGC
 * @matrix: (allow-none): a #BangoMatrix, or %NULL to use an identity transformation
 * @font: the font in which to draw the string
 * @x:       the x position of the start of the string (in Bango
 *           units in user space coordinates)
 * @y:       the y position of the baseline (in Bango units
 *           in user space coordinates)
 * @glyphs:  the glyph string to draw
 * 
 * Renders a #BangoGlyphString onto a drawable, possibly
 * transforming the layed-out coordinates through a transformation
 * matrix. Note that the transformation matrix for @font is not
 * changed, so to produce correct rendering results, the @font
 * must have been loaded using a #BangoContext with an identical
 * transformation matrix to that passed in to this function.
 *
 * See also bdk_draw_glyphs(), bdk_draw_layout().
 *
 * Since: 2.6
 * 
 * Deprecated: 2.22: Use bango_bairo_show_glyphs() instead.
 **/
void
bdk_draw_glyphs_transformed (BdkDrawable       *drawable,
			     BdkGC	       *gc,
			     const BangoMatrix *matrix,
			     BangoFont         *font,
			     gint               x,
			     gint               y,
			     BangoGlyphString  *glyphs)
{
  g_return_if_fail (BDK_IS_DRAWABLE (drawable));
  g_return_if_fail (BDK_IS_GC (gc));

  real_draw_glyphs (drawable, gc, matrix, font,
		    x / BANGO_SCALE, y / BANGO_SCALE, glyphs);
}

/**
 * bdk_draw_trapezoids:
 * @drawable: a #BdkDrawable
 * @gc: a #BdkGC
 * @trapezoids: an array of #BdkTrapezoid structures
 * @n_trapezoids: the number of trapezoids to draw
 * 
 * Draws a set of anti-aliased trapezoids. The trapezoids are
 * combined using saturation addition, then drawn over the background
 * as a set. This is low level functionality used internally to implement
 * rotated underlines and backgrouds when rendering a BangoLayout and is
 * likely not useful for applications.
 *
 * Since: 2.6
 *
 * Deprecated: 2.22: Use Bairo path contruction functions and bairo_fill()
 * instead.
 **/
void
bdk_draw_trapezoids (BdkDrawable        *drawable,
		     BdkGC	        *gc,
		     const BdkTrapezoid *trapezoids,
		     gint                n_trapezoids)
{
  bairo_t *cr;
  int i;

  g_return_if_fail (BDK_IS_DRAWABLE (drawable));
  g_return_if_fail (BDK_IS_GC (gc));
  g_return_if_fail (n_trapezoids == 0 || trapezoids != NULL);

  cr = bdk_bairo_create (drawable);
  _bdk_gc_update_context (gc, cr, NULL, NULL, TRUE, drawable);
  
  for (i = 0; i < n_trapezoids; i++)
    {
      bairo_move_to (cr, trapezoids[i].x11, trapezoids[i].y1);
      bairo_line_to (cr, trapezoids[i].x21, trapezoids[i].y1);
      bairo_line_to (cr, trapezoids[i].x22, trapezoids[i].y2);
      bairo_line_to (cr, trapezoids[i].x12, trapezoids[i].y2);
      bairo_close_path (cr);
    }

  bairo_fill (cr);

  bairo_destroy (cr);
}

/**
 * bdk_drawable_copy_to_image:
 * @drawable: a #BdkDrawable
 * @image: (allow-none): a #BdkDrawable, or %NULL if a new @image should be created.
 * @src_x: x coordinate on @drawable
 * @src_y: y coordinate on @drawable
 * @dest_x: x coordinate within @image. Must be 0 if @image is %NULL
 * @dest_y: y coordinate within @image. Must be 0 if @image is %NULL
 * @width: width of rebunnyion to get
 * @height: height or rebunnyion to get
 *
 * Copies a portion of @drawable into the client side image structure
 * @image. If @image is %NULL, creates a new image of size @width x @height
 * and copies into that. See bdk_drawable_get_image() for further details.
 * 
 * Return value: @image, or a new a #BdkImage containing the contents
 *               of @drawable
 * 
 * Since: 2.4
 *
 * Deprecated: 2.22: Use @drawable as the source and draw to a Bairo image
 * surface if you want to download contents to the client.
 **/
BdkImage*
bdk_drawable_copy_to_image (BdkDrawable *drawable,
			    BdkImage    *image,
			    gint         src_x,
			    gint         src_y,
			    gint         dest_x,
			    gint         dest_y,
			    gint         width,
			    gint         height)
{
  BdkDrawable *composite;
  gint composite_x_offset = 0;
  gint composite_y_offset = 0;
  BdkImage *retval;
  BdkColormap *cmap;
  
  g_return_val_if_fail (BDK_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (src_x >= 0, NULL);
  g_return_val_if_fail (src_y >= 0, NULL);

  /* FIXME? Note race condition since we get the size then
   * get the image, and the size may have changed.
   */
  
  if (width < 0 || height < 0)
    bdk_drawable_get_size (drawable,
                           width < 0 ? &width : NULL,
                           height < 0 ? &height : NULL);
  
  composite =
    BDK_DRAWABLE_GET_CLASS (drawable)->get_composite_drawable (drawable,
                                                               src_x, src_y,
                                                               width, height,
                                                               &composite_x_offset,
                                                               &composite_y_offset); 
  
  retval = BDK_DRAWABLE_GET_CLASS (composite)->_copy_to_image (composite,
							       image,
							       src_x - composite_x_offset,
							       src_y - composite_y_offset,
							       dest_x, dest_y,
							       width, height);

  g_object_unref (composite);

  if (!image && retval)
    {
      cmap = bdk_drawable_get_colormap (drawable);
      
      if (cmap)
	bdk_image_set_colormap (retval, cmap);
    }
  
  return retval;
}

/**
 * bdk_drawable_get_image:
 * @drawable: a #BdkDrawable
 * @x: x coordinate on @drawable
 * @y: y coordinate on @drawable
 * @width: width of rebunnyion to get
 * @height: height or rebunnyion to get
 * 
 * A #BdkImage stores client-side image data (pixels). In contrast,
 * #BdkPixmap and #BdkWindow are server-side
 * objects. bdk_drawable_get_image() obtains the pixels from a
 * server-side drawable as a client-side #BdkImage.  The format of a
 * #BdkImage depends on the #BdkVisual of the current display, which
 * makes manipulating #BdkImage extremely difficult; therefore, in
 * most cases you should use bdk_pixbuf_get_from_drawable() instead of
 * this lower-level function. A #BdkPixbuf contains image data in a
 * canonicalized RGB format, rather than a display-dependent format.
 * Of course, there's a convenience vs. speed tradeoff here, so you'll
 * want to think about what makes sense for your application.
 *
 * @x, @y, @width, and @height define the rebunnyion of @drawable to
 * obtain as an image.
 *
 * You would usually copy image data to the client side if you intend
 * to examine the values of individual pixels, for example to darken
 * an image or add a red tint. It would be prohibitively slow to
 * make a round-trip request to the windowing system for each pixel,
 * so instead you get all of them at once, modify them, then copy
 * them all back at once.
 *
 * If the X server or other windowing system backend is on the local
 * machine, this function may use shared memory to avoid copying
 * the image data.
 *
 * If the source drawable is a #BdkWindow and partially offscreen
 * or obscured, then the obscured portions of the returned image
 * will contain undefined data.
 * 
 * Return value: a #BdkImage containing the contents of @drawable
 *
 * Deprecated: 2.22: Use @drawable as the source and draw to a Bairo image
 * surface if you want to download contents to the client.
 **/
BdkImage*
bdk_drawable_get_image (BdkDrawable *drawable,
                        gint         x,
                        gint         y,
                        gint         width,
                        gint         height)
{
  BdkDrawable *composite;
  gint composite_x_offset = 0;
  gint composite_y_offset = 0;
  BdkImage *retval;
  BdkColormap *cmap;
  
  g_return_val_if_fail (BDK_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (x >= 0, NULL);
  g_return_val_if_fail (y >= 0, NULL);

  /* FIXME? Note race condition since we get the size then
   * get the image, and the size may have changed.
   */
  
  if (width < 0 || height < 0)
    bdk_drawable_get_size (drawable,
                           width < 0 ? &width : NULL,
                           height < 0 ? &height : NULL);
  
  composite =
    BDK_DRAWABLE_GET_CLASS (drawable)->get_composite_drawable (drawable,
                                                               x, y,
                                                               width, height,
                                                               &composite_x_offset,
                                                               &composite_y_offset); 
  
  retval = BDK_DRAWABLE_GET_CLASS (composite)->get_image (composite,
                                                          x - composite_x_offset,
                                                          y - composite_y_offset,
                                                          width, height);

  g_object_unref (composite);

  cmap = bdk_drawable_get_colormap (drawable);
  
  if (retval && cmap)
    bdk_image_set_colormap (retval, cmap);
  
  return retval;
}

static BdkImage*
bdk_drawable_real_get_image (BdkDrawable     *drawable,
			     gint             x,
			     gint             y,
			     gint             width,
			     gint             height)
{
  return bdk_drawable_copy_to_image (drawable, NULL, x, y, 0, 0, width, height);
}

static BdkDrawable *
bdk_drawable_real_get_composite_drawable (BdkDrawable *drawable,
                                          gint         x,
                                          gint         y,
                                          gint         width,
                                          gint         height,
                                          gint        *composite_x_offset,
                                          gint        *composite_y_offset)
{
  g_return_val_if_fail (BDK_IS_DRAWABLE (drawable), NULL);

  *composite_x_offset = 0;
  *composite_y_offset = 0;
  
  return g_object_ref (drawable);
}

/**
 * bdk_drawable_get_clip_rebunnyion:
 * @drawable: a #BdkDrawable
 * 
 * Computes the rebunnyion of a drawable that potentially can be written
 * to by drawing primitives. This rebunnyion will not take into account
 * the clip rebunnyion for the GC, and may also not take into account
 * other factors such as if the window is obscured by other windows,
 * but no area outside of this rebunnyion will be affected by drawing
 * primitives.
 * 
 * Returns: a #BdkRebunnyion. This must be freed with bdk_rebunnyion_destroy()
 *          when you are done.
 **/
BdkRebunnyion *
bdk_drawable_get_clip_rebunnyion (BdkDrawable *drawable)
{
  g_return_val_if_fail (BDK_IS_DRAWABLE (drawable), NULL);

  return BDK_DRAWABLE_GET_CLASS (drawable)->get_clip_rebunnyion (drawable);
}

/**
 * bdk_drawable_get_visible_rebunnyion:
 * @drawable: a #BdkDrawable
 * 
 * Computes the rebunnyion of a drawable that is potentially visible.
 * This does not necessarily take into account if the window is
 * obscured by other windows, but no area outside of this rebunnyion
 * is visible.
 * 
 * Returns: a #BdkRebunnyion. This must be freed with bdk_rebunnyion_destroy()
 *          when you are done.
 **/
BdkRebunnyion *
bdk_drawable_get_visible_rebunnyion (BdkDrawable *drawable)
{
  g_return_val_if_fail (BDK_IS_DRAWABLE (drawable), NULL);

  return BDK_DRAWABLE_GET_CLASS (drawable)->get_visible_rebunnyion (drawable);
}

static BdkRebunnyion *
bdk_drawable_real_get_visible_rebunnyion (BdkDrawable *drawable)
{
  BdkRectangle rect;

  rect.x = 0;
  rect.y = 0;

  bdk_drawable_get_size (drawable, &rect.width, &rect.height);

  return bdk_rebunnyion_rectangle (&rect);
}

/**
 * _bdk_drawable_ref_bairo_surface:
 * @drawable: a #BdkDrawable
 * 
 * Obtains a #bairo_surface_t for the given drawable. If a
 * #bairo_surface_t for the drawable already exists, it will be
 * referenced, otherwise a new surface will be created.
 * 
 * Return value: a newly referenced #bairo_surface_t that points
 *  to @drawable. Unref with bairo_surface_destroy()
 **/
bairo_surface_t *
_bdk_drawable_ref_bairo_surface (BdkDrawable *drawable)
{
  g_return_val_if_fail (BDK_IS_DRAWABLE (drawable), NULL);

  return BDK_DRAWABLE_GET_CLASS (drawable)->ref_bairo_surface (drawable);
}

static void
composite (guchar *src_buf,
	   gint    src_rowstride,
	   guchar *dest_buf,
	   gint    dest_rowstride,
	   gint    width,
	   gint    height)
{
  guchar *src = src_buf;
  guchar *dest = dest_buf;

  while (height--)
    {
      gint twidth = width;
      guchar *p = src;
      guchar *q = dest;

      while (twidth--)
	{
	  guchar a = p[3];
	  guint t;

	  t = a * p[0] + (255 - a) * q[0] + 0x80;
	  q[0] = (t + (t >> 8)) >> 8;
	  t = a * p[1] + (255 - a) * q[1] + 0x80;
	  q[1] = (t + (t >> 8)) >> 8;
	  t = a * p[2] + (255 - a) * q[2] + 0x80;
	  q[2] = (t + (t >> 8)) >> 8;

	  p += 4;
	  q += 3;
	}
      
      src += src_rowstride;
      dest += dest_rowstride;
    }
}

static void
composite_0888 (guchar      *src_buf,
		gint         src_rowstride,
		guchar      *dest_buf,
		gint         dest_rowstride,
		BdkByteOrder dest_byte_order,
		gint         width,
		gint         height)
{
  guchar *src = src_buf;
  guchar *dest = dest_buf;

  while (height--)
    {
      gint twidth = width;
      guchar *p = src;
      guchar *q = dest;

      if (dest_byte_order == BDK_LSB_FIRST)
	{
	  while (twidth--)
	    {
	      guint t;
	      
	      t = p[3] * p[2] + (255 - p[3]) * q[0] + 0x80;
	      q[0] = (t + (t >> 8)) >> 8;
	      t = p[3] * p[1] + (255 - p[3]) * q[1] + 0x80;
	      q[1] = (t + (t >> 8)) >> 8;
	      t = p[3] * p[0] + (255 - p[3]) * q[2] + 0x80;
	      q[2] = (t + (t >> 8)) >> 8;
	      p += 4;
	      q += 4;
	    }
	}
      else
	{
	  while (twidth--)
	    {
	      guint t;
	      
	      t = p[3] * p[0] + (255 - p[3]) * q[1] + 0x80;
	      q[1] = (t + (t >> 8)) >> 8;
	      t = p[3] * p[1] + (255 - p[3]) * q[2] + 0x80;
	      q[2] = (t + (t >> 8)) >> 8;
	      t = p[3] * p[2] + (255 - p[3]) * q[3] + 0x80;
	      q[3] = (t + (t >> 8)) >> 8;
	      p += 4;
	      q += 4;
	    }
	}
      
      src += src_rowstride;
      dest += dest_rowstride;
    }
}

#ifdef USE_MEDIALIB
static void
composite_0888_medialib (guchar      *src_buf,
			 gint         src_rowstride,
			 guchar      *dest_buf,
			 gint         dest_rowstride,
			 BdkByteOrder dest_byte_order,
			 gint         width,
			 gint         height)
{
  guchar *src  = src_buf;
  guchar *dest = dest_buf;

  mlib_image img_src, img_dst;

  mlib_ImageSetStruct (&img_dst,
                       MLIB_BYTE,
                       4,
                       width,
                       height,
                       dest_rowstride,
                       dest_buf);

  mlib_ImageSetStruct (&img_src,
                       MLIB_BYTE,
                       4,
                       width,
                       height,
                       src_rowstride,
                       src_buf);

  if (dest_byte_order == BDK_LSB_FIRST)
      mlib_ImageBlendRGBA2BGRA (&img_dst, &img_src);
  else
      mlib_ImageBlendRGBA2ARGB (&img_dst, &img_src);
}
#endif

static void
composite_565 (guchar      *src_buf,
	       gint         src_rowstride,
	       guchar      *dest_buf,
	       gint         dest_rowstride,
	       BdkByteOrder dest_byte_order,
	       gint         width,
	       gint         height)
{
  guchar *src = src_buf;
  guchar *dest = dest_buf;

  while (height--)
    {
      gint twidth = width;
      guchar *p = src;
      gushort *q = (gushort *)dest;

      while (twidth--)
	{
	  guchar a = p[3];
	  guint tr, tg, tb;
	  guint tr1, tg1, tb1;
	  guint tmp = *q;

#if 1
	  /* This is fast, and corresponds to what composite() above does
	   * if we converted to 8-bit first.
	   */
	  tr = (tmp & 0xf800);
	  tr1 = a * p[0] + (255 - a) * ((tr >> 8) + (tr >> 13)) + 0x80;
	  tg = (tmp & 0x07e0);
	  tg1 = a * p[1] + (255 - a) * ((tg >> 3) + (tg >> 9)) + 0x80;
	  tb = (tmp & 0x001f);
	  tb1 = a * p[2] + (255 - a) * ((tb << 3) + (tb >> 2)) + 0x80;

	  *q = (((tr1 + (tr1 >> 8)) & 0xf800) |
		(((tg1 + (tg1 >> 8)) & 0xfc00) >> 5)  |
		((tb1 + (tb1 >> 8)) >> 11));
#else
	  /* This version correspond to the result we get with XRENDER -
	   * a bit of precision is lost since we convert to 8 bit after premultiplying
	   * instead of at the end
	   */
	  guint tr2, tg2, tb2;
	  guint tr3, tg3, tb3;
	  
	  tr = (tmp & 0xf800);
	  tr1 = (255 - a) * ((tr >> 8) + (tr >> 13)) + 0x80;
	  tr2 = a * p[0] + 0x80;
	  tr3 = ((tr1 + (tr1 >> 8)) >> 8) + ((tr2 + (tr2 >> 8)) >> 8);

	  tg = (tmp & 0x07e0);
	  tg1 = (255 - a) * ((tg >> 3) + (tg >> 9)) + 0x80;
	  tg2 = a * p[0] + 0x80;
	  tg3 = ((tg1 + (tg1 >> 8)) >> 8) + ((tg2 + (tg2 >> 8)) >> 8);

	  tb = (tmp & 0x001f);
	  tb1 = (255 - a) * ((tb << 3) + (tb >> 2)) + 0x80;
	  tb2 = a * p[0] + 0x80;
	  tb3 = ((tb1 + (tb1 >> 8)) >> 8) + ((tb2 + (tb2 >> 8)) >> 8);

	  *q = (((tr3 & 0xf8) << 8) |
		((tg3 & 0xfc) << 3) |
		((tb3 >> 3)));
#endif
	  
	  p += 4;
	  q++;
	}
      
      src += src_rowstride;
      dest += dest_rowstride;
    }
}

/* Implementation of the old vfunc in terms of the new one
   in case someone calls it directly (which they shouldn't!) */
static void
bdk_drawable_real_draw_drawable (BdkDrawable  *drawable,
				 BdkGC	       *gc,
				 BdkDrawable  *src,
				 gint		xsrc,
				 gint		ysrc,
				 gint		xdest,
				 gint		ydest,
				 gint		width,
				 gint		height)
{
  BDK_DRAWABLE_GET_CLASS (drawable)->draw_drawable_with_src (drawable,
							     gc,
							     src,
							     xsrc,
							     ysrc,
							     xdest,
							     ydest,
							     width,
							     height,
							     src);
}

static void
bdk_drawable_real_draw_pixbuf (BdkDrawable  *drawable,
			       BdkGC        *gc,
			       BdkPixbuf    *pixbuf,
			       gint          src_x,
			       gint          src_y,
			       gint          dest_x,
			       gint          dest_y,
			       gint          width,
			       gint          height,
			       BdkRgbDither  dither,
			       gint          x_dither,
			       gint          y_dither)
{
  BdkPixbuf *composited = NULL;
  gint dwidth, dheight;
  BdkRebunnyion *clip;
  BdkRebunnyion *drect;
  BdkRectangle tmp_rect;
  BdkDrawable  *real_drawable;

  g_return_if_fail (BDK_IS_PIXBUF (pixbuf));
  g_return_if_fail (bdk_pixbuf_get_colorspace (pixbuf) == BDK_COLORSPACE_RGB);
  g_return_if_fail (bdk_pixbuf_get_n_channels (pixbuf) == 3 ||
                    bdk_pixbuf_get_n_channels (pixbuf) == 4);
  g_return_if_fail (bdk_pixbuf_get_bits_per_sample (pixbuf) == 8);

  g_return_if_fail (drawable != NULL);

  if (width == -1) 
    width = bdk_pixbuf_get_width (pixbuf);
  if (height == -1)
    height = bdk_pixbuf_get_height (pixbuf);

  g_return_if_fail (width >= 0 && height >= 0);
  g_return_if_fail (src_x >= 0 && src_x + width <= bdk_pixbuf_get_width (pixbuf));
  g_return_if_fail (src_y >= 0 && src_y + height <= bdk_pixbuf_get_height (pixbuf));

  /* Clip to the drawable; this is required for get_from_drawable() so
   * can't be done implicitly
   */
  
  if (dest_x < 0)
    {
      src_x -= dest_x;
      width += dest_x;
      dest_x = 0;
    }

  if (dest_y < 0)
    {
      src_y -= dest_y;
      height += dest_y;
      dest_y = 0;
    }

  bdk_drawable_get_size (drawable, &dwidth, &dheight);

  if ((dest_x + width) > dwidth)
    width = dwidth - dest_x;

  if ((dest_y + height) > dheight)
    height = dheight - dest_y;

  if (width <= 0 || height <= 0)
    return;

  /* Clip to the clip rebunnyion; this avoids getting more
   * image data from the server than we need to.
   */
  
  tmp_rect.x = dest_x;
  tmp_rect.y = dest_y;
  tmp_rect.width = width;
  tmp_rect.height = height;

  drect = bdk_rebunnyion_rectangle (&tmp_rect);
  clip = bdk_drawable_get_clip_rebunnyion (drawable);

  bdk_rebunnyion_intersect (drect, clip);

  bdk_rebunnyion_get_clipbox (drect, &tmp_rect);
  
  bdk_rebunnyion_destroy (drect);
  bdk_rebunnyion_destroy (clip);

  if (tmp_rect.width == 0 ||
      tmp_rect.height == 0)
    return;
  
  /* Actually draw */
  if (!gc)
    gc = _bdk_drawable_get_scratch_gc (drawable, FALSE);

  /* Drawable is a wrapper here, but at this time we
     have already retargeted the destination to any
     impl window and set the clip, so what we really
     want to do is draw directly on the impl, ignoring
     client side subwindows. We also use the impl
     in the pixmap target case to avoid resetting the
     already set clip on the GC. */
  if (BDK_IS_WINDOW (drawable))
    real_drawable = BDK_WINDOW_OBJECT (drawable)->impl;
  else
    real_drawable = BDK_PIXMAP_OBJECT (drawable)->impl;

  if (bdk_pixbuf_get_has_alpha (pixbuf))
    {
      BdkVisual *visual = bdk_drawable_get_visual (drawable);
      void (*composite_func) (guchar       *src_buf,
			      gint          src_rowstride,
			      guchar       *dest_buf,
			      gint          dest_rowstride,
			      BdkByteOrder  dest_byte_order,
			      gint          width,
			      gint          height) = NULL;

      /* First we see if we have a visual-specific composition function that can composite
       * the pixbuf data directly onto the image
       */
      if (visual)
	{
	  gint bits_per_pixel = _bdk_windowing_get_bits_for_depth (bdk_drawable_get_display (drawable),
								   visual->depth);
	  
	  if (visual->byte_order == (G_BYTE_ORDER == G_BIG_ENDIAN ? BDK_MSB_FIRST : BDK_LSB_FIRST) &&
	      visual->depth == 16 &&
	      visual->red_mask   == 0xf800 &&
	      visual->green_mask == 0x07e0 &&
	      visual->blue_mask  == 0x001f)
	    composite_func = composite_565;
	  else if (visual->depth == 24 && bits_per_pixel == 32 &&
		   visual->red_mask   == 0xff0000 &&
		   visual->green_mask == 0x00ff00 &&
		   visual->blue_mask  == 0x0000ff)
	    {
#ifdef USE_MEDIALIB
	      if (_bdk_use_medialib ())
	        composite_func = composite_0888_medialib;
	      else
	        composite_func = composite_0888;
#else
	      composite_func = composite_0888;
#endif
	    }
	}

      /* We can't use our composite func if we are required to dither
       */
      if (composite_func && !(dither == BDK_RGB_DITHER_MAX && visual->depth != 24))
	{
	  gint x0, y0;
	  for (y0 = 0; y0 < height; y0 += BDK_SCRATCH_IMAGE_HEIGHT)
	    {
	      gint height1 = MIN (height - y0, BDK_SCRATCH_IMAGE_HEIGHT);
	      for (x0 = 0; x0 < width; x0 += BDK_SCRATCH_IMAGE_WIDTH)
		{
		  gint xs0, ys0;
		  
		  gint width1 = MIN (width - x0, BDK_SCRATCH_IMAGE_WIDTH);
		  
		  BdkImage *image = _bdk_image_get_scratch (bdk_drawable_get_screen (drawable),
							    width1, height1,
							    bdk_drawable_get_depth (drawable), &xs0, &ys0);
		  
		  bdk_drawable_copy_to_image (drawable, image,
					      dest_x + x0, dest_y + y0,
					      xs0, ys0,
					      width1, height1);
		  (*composite_func) (bdk_pixbuf_get_pixels (pixbuf) + (src_y + y0) * bdk_pixbuf_get_rowstride (pixbuf) + (src_x + x0) * 4,
				     bdk_pixbuf_get_rowstride (pixbuf),
				     (guchar*)image->mem + ys0 * image->bpl + xs0 * image->bpp,
				     image->bpl,
				     visual->byte_order,
				     width1, height1);
		  bdk_draw_image (real_drawable, gc, image,
				  xs0, ys0,
				  dest_x + x0, dest_y + y0,
				  width1, height1);
		}
	    }
	  
	  goto out;
	}
      else
	{
	  /* No special composition func, convert dest to 24 bit RGB data, composite against
	   * that, and convert back.
	   */
	  composited = bdk_pixbuf_get_from_drawable (NULL,
						     drawable,
						     NULL,
						     dest_x, dest_y,
						     0, 0,
						     width, height);
	  
	  if (composited)
	    composite (bdk_pixbuf_get_pixels (pixbuf) + src_y * bdk_pixbuf_get_rowstride (pixbuf) + src_x * 4,
		       bdk_pixbuf_get_rowstride (pixbuf),
		       bdk_pixbuf_get_pixels (composited),
		       bdk_pixbuf_get_rowstride (composited),
		       width, height);
	}
    }

  if (composited)
    {
      src_x = 0;
      src_y = 0;
      pixbuf = composited;
    }
  
  if (bdk_pixbuf_get_n_channels (pixbuf) == 4)
    {
      guchar *buf = bdk_pixbuf_get_pixels (pixbuf) + src_y * bdk_pixbuf_get_rowstride (pixbuf) + src_x * 4;

      bdk_draw_rgb_32_image_dithalign (real_drawable, gc,
				       dest_x, dest_y,
				       width, height,
				       dither,
				       buf, bdk_pixbuf_get_rowstride (pixbuf),
				       x_dither, y_dither);
    }
  else				/* n_channels == 3 */
    {
      guchar *buf = bdk_pixbuf_get_pixels (pixbuf) + src_y * bdk_pixbuf_get_rowstride (pixbuf) + src_x * 3;

      bdk_draw_rgb_image_dithalign (real_drawable, gc,
				    dest_x, dest_y,
				    width, height,
				    dither,
				    buf, bdk_pixbuf_get_rowstride (pixbuf),
				    x_dither, y_dither);
    }

 out:
  if (composited)
    g_object_unref (composited);
}

/************************************************************************/

/**
 * _bdk_drawable_get_scratch_gc:
 * @drawable: A #BdkDrawable
 * @graphics_exposures: Whether the returned #BdkGC should generate graphics exposures 
 * 
 * Returns a #BdkGC suitable for drawing on @drawable. The #BdkGC has
 * the standard values for @drawable, except for the graphics_exposures
 * field which is determined by the @graphics_exposures parameter.
 *
 * The foreground color of the returned #BdkGC is undefined. The #BdkGC
 * must not be altered in any way, except to change its foreground color.
 * 
 * Return value: A #BdkGC suitable for drawing on @drawable
 * 
 * Since: 2.4
 **/
BdkGC *
_bdk_drawable_get_scratch_gc (BdkDrawable *drawable,
			      gboolean     graphics_exposures)
{
  BdkScreen *screen;
  gint depth;

  g_return_val_if_fail (BDK_IS_DRAWABLE (drawable), NULL);

  screen = bdk_drawable_get_screen (drawable);

  g_return_val_if_fail (!screen->closed, NULL);

  depth = bdk_drawable_get_depth (drawable) - 1;

  if (graphics_exposures)
    {
      if (!screen->exposure_gcs[depth])
	{
	  BdkGCValues values;
	  BdkGCValuesMask mask;

	  values.graphics_exposures = TRUE;
	  mask = BDK_GC_EXPOSURES;  

	  screen->exposure_gcs[depth] =
	    bdk_gc_new_with_values (drawable, &values, mask);
	}

      return screen->exposure_gcs[depth];
    }
  else
    {
      if (!screen->normal_gcs[depth])
	{
	  screen->normal_gcs[depth] =
	    bdk_gc_new (drawable);
	}

      return screen->normal_gcs[depth];
    }
}

/**
 * _bdk_drawable_get_subwindow_scratch_gc:
 * @drawable: A #BdkDrawable
 * 
 * Returns a #BdkGC suitable for drawing on @drawable. The #BdkGC has
 * the standard values for @drawable, except for the graphics_exposures
 * field which is %TRUE and the subwindow mode which is %BDK_INCLUDE_INFERIORS.
 *
 * The foreground color of the returned #BdkGC is undefined. The #BdkGC
 * must not be altered in any way, except to change its foreground color.
 * 
 * Return value: A #BdkGC suitable for drawing on @drawable
 * 
 * Since: 2.18
 **/
BdkGC *
_bdk_drawable_get_subwindow_scratch_gc (BdkDrawable *drawable)
{
  BdkScreen *screen;
  gint depth;

  g_return_val_if_fail (BDK_IS_DRAWABLE (drawable), NULL);

  screen = bdk_drawable_get_screen (drawable);

  g_return_val_if_fail (!screen->closed, NULL);

  depth = bdk_drawable_get_depth (drawable) - 1;

  if (!screen->subwindow_gcs[depth])
    {
      BdkGCValues values;
      BdkGCValuesMask mask;
      
      values.graphics_exposures = TRUE;
      values.subwindow_mode = BDK_INCLUDE_INFERIORS;
      mask = BDK_GC_EXPOSURES | BDK_GC_SUBWINDOW;  
      
      screen->subwindow_gcs[depth] =
	bdk_gc_new_with_values (drawable, &values, mask);
    }
  
  return screen->subwindow_gcs[depth];
}


/*
 * _bdk_drawable_get_source_drawable:
 * @drawable: a #BdkDrawable
 *
 * Returns a drawable for the passed @drawable that is guaranteed to be
 * usable to create a pixmap (e.g.: not an offscreen window).
 *
 * Since: 2.16
 */
BdkDrawable *
_bdk_drawable_get_source_drawable (BdkDrawable *drawable)
{
  g_return_val_if_fail (BDK_IS_DRAWABLE (drawable), NULL);

  if (BDK_DRAWABLE_GET_CLASS (drawable)->get_source_drawable)
    return BDK_DRAWABLE_GET_CLASS (drawable)->get_source_drawable (drawable);

  return drawable;
}

bairo_surface_t *
_bdk_drawable_create_bairo_surface (BdkDrawable *drawable,
				    int width,
				    int height)
{
  return BDK_DRAWABLE_GET_CLASS (drawable)->create_bairo_surface (drawable,
								  width, height);
}


#define __BDK_DRAW_C__
#include "bdkaliasdef.c"
