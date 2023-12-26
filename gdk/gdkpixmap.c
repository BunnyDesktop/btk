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
#include "bdkpixmap.h"
#include "bdkinternals.h"
#include "bdkpixbuf.h"
#include "bdkscreen.h"
#include "bdkalias.h"

static BdkGC *bdk_pixmap_create_gc      (BdkDrawable     *drawable,
                                         BdkGCValues     *values,
                                         BdkGCValuesMask  mask);
static void   bdk_pixmap_draw_rectangle (BdkDrawable     *drawable,
					 BdkGC           *gc,
					 gboolean         filled,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height);
static void   bdk_pixmap_draw_arc       (BdkDrawable     *drawable,
					 BdkGC           *gc,
					 gboolean         filled,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height,
					 gint             angle1,
					 gint             angle2);
static void   bdk_pixmap_draw_polygon   (BdkDrawable     *drawable,
					 BdkGC           *gc,
					 gboolean         filled,
					 BdkPoint        *points,
					 gint             npoints);
static void   bdk_pixmap_draw_text      (BdkDrawable     *drawable,
					 BdkFont         *font,
					 BdkGC           *gc,
					 gint             x,
					 gint             y,
					 const gchar     *text,
					 gint             text_length);
static void   bdk_pixmap_draw_text_wc   (BdkDrawable     *drawable,
					 BdkFont         *font,
					 BdkGC           *gc,
					 gint             x,
					 gint             y,
					 const BdkWChar  *text,
					 gint             text_length);
static void   bdk_pixmap_draw_drawable  (BdkDrawable     *drawable,
					 BdkGC           *gc,
					 BdkPixmap       *src,
					 gint             xsrc,
					 gint             ysrc,
					 gint             xdest,
					 gint             ydest,
					 gint             width,
					 gint             height,
					 BdkPixmap       *original_src);
static void   bdk_pixmap_draw_points    (BdkDrawable     *drawable,
					 BdkGC           *gc,
					 BdkPoint        *points,
					 gint             npoints);
static void   bdk_pixmap_draw_segments  (BdkDrawable     *drawable,
					 BdkGC           *gc,
					 BdkSegment      *segs,
					 gint             nsegs);
static void   bdk_pixmap_draw_lines     (BdkDrawable     *drawable,
					 BdkGC           *gc,
					 BdkPoint        *points,
					 gint             npoints);

static void bdk_pixmap_draw_glyphs             (BdkDrawable      *drawable,
						BdkGC            *gc,
						BangoFont        *font,
						gint              x,
						gint              y,
						BangoGlyphString *glyphs);
static void bdk_pixmap_draw_glyphs_transformed (BdkDrawable      *drawable,
						BdkGC            *gc,
						BangoMatrix      *matrix,
						BangoFont        *font,
						gint              x,
						gint              y,
						BangoGlyphString *glyphs);

static void   bdk_pixmap_draw_image     (BdkDrawable     *drawable,
                                         BdkGC           *gc,
                                         BdkImage        *image,
                                         gint             xsrc,
                                         gint             ysrc,
                                         gint             xdest,
                                         gint             ydest,
                                         gint             width,
                                         gint             height);
static void   bdk_pixmap_draw_pixbuf    (BdkDrawable     *drawable,
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
static void  bdk_pixmap_draw_trapezoids (BdkDrawable     *drawable,
					 BdkGC	         *gc,
					 BdkTrapezoid    *trapezoids,
					 gint             n_trapezoids);

static void   bdk_pixmap_real_get_size  (BdkDrawable     *drawable,
                                         gint            *width,
                                         gint            *height);

static BdkImage* bdk_pixmap_copy_to_image (BdkDrawable *drawable,
					   BdkImage    *image,
					   gint         src_x,
					   gint         src_y,
					   gint         dest_x,
					   gint         dest_y,
					   gint         width,
					   gint         height);

static bairo_surface_t *bdk_pixmap_ref_bairo_surface (BdkDrawable *drawable);
static bairo_surface_t *bdk_pixmap_create_bairo_surface (BdkDrawable *drawable,
							 int width,
							 int height);

static BdkVisual*   bdk_pixmap_real_get_visual   (BdkDrawable *drawable);
static gint         bdk_pixmap_real_get_depth    (BdkDrawable *drawable);
static void         bdk_pixmap_real_set_colormap (BdkDrawable *drawable,
                                                  BdkColormap *cmap);
static BdkColormap* bdk_pixmap_real_get_colormap (BdkDrawable *drawable);
static BdkScreen*   bdk_pixmap_real_get_screen   (BdkDrawable *drawable);

static void bdk_pixmap_init       (BdkPixmapObject      *pixmap);
static void bdk_pixmap_class_init (BdkPixmapObjectClass *klass);
static void bdk_pixmap_finalize   (GObject              *object);

static gpointer parent_class = NULL;

GType
bdk_pixmap_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    object_type = g_type_register_static_simple (BDK_TYPE_DRAWABLE,
						 "BdkPixmap",
						 sizeof (BdkPixmapObjectClass),
						 (GClassInitFunc) bdk_pixmap_class_init,
						 sizeof (BdkPixmapObject),
						 (GInstanceInitFunc) bdk_pixmap_init,
						 0);
  
  return object_type;
}

static void
bdk_pixmap_init (BdkPixmapObject *pixmap)
{
  /* 0-initialization is good for all other fields. */
  pixmap->impl = g_object_new (_bdk_pixmap_impl_get_type (), NULL);
}

static void
bdk_pixmap_class_init (BdkPixmapObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BdkDrawableClass *drawable_class = BDK_DRAWABLE_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = bdk_pixmap_finalize;

  drawable_class->create_gc = bdk_pixmap_create_gc;
  drawable_class->draw_rectangle = bdk_pixmap_draw_rectangle;
  drawable_class->draw_arc = bdk_pixmap_draw_arc;
  drawable_class->draw_polygon = bdk_pixmap_draw_polygon;
  drawable_class->draw_text = bdk_pixmap_draw_text;
  drawable_class->draw_text_wc = bdk_pixmap_draw_text_wc;
  drawable_class->draw_drawable_with_src = bdk_pixmap_draw_drawable;
  drawable_class->draw_points = bdk_pixmap_draw_points;
  drawable_class->draw_segments = bdk_pixmap_draw_segments;
  drawable_class->draw_lines = bdk_pixmap_draw_lines;
  drawable_class->draw_glyphs = bdk_pixmap_draw_glyphs;
  drawable_class->draw_glyphs_transformed = bdk_pixmap_draw_glyphs_transformed;
  drawable_class->draw_image = bdk_pixmap_draw_image;
  drawable_class->draw_pixbuf = bdk_pixmap_draw_pixbuf;
  drawable_class->draw_trapezoids = bdk_pixmap_draw_trapezoids;
  drawable_class->get_depth = bdk_pixmap_real_get_depth;
  drawable_class->get_screen = bdk_pixmap_real_get_screen;
  drawable_class->get_size = bdk_pixmap_real_get_size;
  drawable_class->set_colormap = bdk_pixmap_real_set_colormap;
  drawable_class->get_colormap = bdk_pixmap_real_get_colormap;
  drawable_class->get_visual = bdk_pixmap_real_get_visual;
  drawable_class->_copy_to_image = bdk_pixmap_copy_to_image;
  drawable_class->ref_bairo_surface = bdk_pixmap_ref_bairo_surface;
  drawable_class->create_bairo_surface = bdk_pixmap_create_bairo_surface;
}

static void
bdk_pixmap_finalize (GObject *object)
{
  BdkPixmapObject *obj = (BdkPixmapObject *) object;

  g_object_unref (obj->impl);
  obj->impl = NULL;
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

BdkPixmap *
bdk_pixmap_new (BdkDrawable *drawable,
                gint         width,
                gint         height,
                gint         depth)
{
  BdkDrawable *source_drawable;

  if (drawable)
    source_drawable = _bdk_drawable_get_source_drawable (drawable);
  else
    source_drawable = NULL;
  return _bdk_pixmap_new (source_drawable, width, height, depth);
}

BdkPixmap *
bdk_bitmap_create_from_data (BdkDrawable *drawable,
                             const gchar *data,
                             gint         width,
                             gint         height)
{
  BdkDrawable *source_drawable;

  if (drawable)
    source_drawable = _bdk_drawable_get_source_drawable (drawable);
  else
    source_drawable = NULL;
  return _bdk_bitmap_create_from_data (source_drawable, data, width, height);
}

BdkPixmap*
bdk_pixmap_create_from_data (BdkDrawable    *drawable,
                             const gchar    *data,
                             gint            width,
                             gint            height,
                             gint            depth,
                             const BdkColor *fg,
                             const BdkColor *bg)
{
  BdkDrawable *source_drawable;

  source_drawable = _bdk_drawable_get_source_drawable (drawable);
  return _bdk_pixmap_create_from_data (source_drawable,
                                       data, width, height,
                                       depth, fg,bg);
}


static BdkGC *
bdk_pixmap_create_gc (BdkDrawable     *drawable,
                      BdkGCValues     *values,
                      BdkGCValuesMask  mask)
{
  return bdk_gc_new_with_values (((BdkPixmapObject *) drawable)->impl,
                                 values, mask);
}

static void
bdk_pixmap_draw_rectangle (BdkDrawable *drawable,
			   BdkGC       *gc,
			   gboolean     filled,
			   gint         x,
			   gint         y,
			   gint         width,
			   gint         height)
{
  BdkPixmapObject *private = (BdkPixmapObject *)drawable;

  _bdk_gc_remove_drawable_clip (gc);  
  bdk_draw_rectangle (private->impl, gc, filled,
                      x, y, width, height);
}

static void
bdk_pixmap_draw_arc (BdkDrawable *drawable,
		     BdkGC       *gc,
		     gboolean     filled,
		     gint         x,
		     gint         y,
		     gint         width,
		     gint         height,
		     gint         angle1,
		     gint         angle2)
{
  BdkPixmapObject *private = (BdkPixmapObject *)drawable;

  _bdk_gc_remove_drawable_clip (gc);  
  bdk_draw_arc (private->impl, gc, filled,
                x, y,
                width, height, angle1, angle2);
}

static void
bdk_pixmap_draw_polygon (BdkDrawable *drawable,
			 BdkGC       *gc,
			 gboolean     filled,
			 BdkPoint    *points,
			 gint         npoints)
{
  BdkPixmapObject *private = (BdkPixmapObject *)drawable;

  _bdk_gc_remove_drawable_clip (gc);  
  bdk_draw_polygon (private->impl, gc, filled, points, npoints);
}

static void
bdk_pixmap_draw_text (BdkDrawable *drawable,
		      BdkFont     *font,
		      BdkGC       *gc,
		      gint         x,
		      gint         y,
		      const gchar *text,
		      gint         text_length)
{
  BdkPixmapObject *private = (BdkPixmapObject *)drawable;

  _bdk_gc_remove_drawable_clip (gc);  
  bdk_draw_text (private->impl, font, gc,
                 x, y, text, text_length);
}

static void
bdk_pixmap_draw_text_wc (BdkDrawable    *drawable,
			 BdkFont        *font,
			 BdkGC          *gc,
			 gint            x,
			 gint            y,
			 const BdkWChar *text,
			 gint            text_length)
{
  BdkPixmapObject *private = (BdkPixmapObject *)drawable;

  _bdk_gc_remove_drawable_clip (gc);  
  bdk_draw_text_wc (private->impl, font, gc,
                    x, y, text, text_length);
}

static void
bdk_pixmap_draw_drawable (BdkDrawable *drawable,
			  BdkGC       *gc,
			  BdkPixmap   *src,
			  gint         xsrc,
			  gint         ysrc,
			  gint         xdest,
			  gint         ydest,
			  gint         width,
			  gint         height,
			  BdkPixmap   *original_src)
{
  BdkPixmapObject *private = (BdkPixmapObject *)drawable;

  _bdk_gc_remove_drawable_clip (gc);
  /* Call the method directly to avoid getting the composite drawable again */
  BDK_DRAWABLE_GET_CLASS (private->impl)->draw_drawable_with_src (private->impl, gc,
								  src,
								  xsrc, ysrc,
								  xdest, ydest,
								  width, height,
								  original_src);
}

static void
bdk_pixmap_draw_points (BdkDrawable *drawable,
			BdkGC       *gc,
			BdkPoint    *points,
			gint         npoints)
{
  BdkPixmapObject *private = (BdkPixmapObject *)drawable;

  _bdk_gc_remove_drawable_clip (gc);  
  bdk_draw_points (private->impl, gc, points, npoints);
}

static void
bdk_pixmap_draw_segments (BdkDrawable *drawable,
			  BdkGC       *gc,
			  BdkSegment  *segs,
			  gint         nsegs)
{
  BdkPixmapObject *private = (BdkPixmapObject *)drawable;

  _bdk_gc_remove_drawable_clip (gc);  
  bdk_draw_segments (private->impl, gc, segs, nsegs);
}

static void
bdk_pixmap_draw_lines (BdkDrawable *drawable,
		       BdkGC       *gc,
		       BdkPoint    *points,
		       gint         npoints)
{
  BdkPixmapObject *private = (BdkPixmapObject *)drawable;

  _bdk_gc_remove_drawable_clip (gc);  
  bdk_draw_lines (private->impl, gc, points, npoints);
}

static void
bdk_pixmap_draw_glyphs (BdkDrawable      *drawable,
                        BdkGC            *gc,
                        BangoFont        *font,
                        gint              x,
                        gint              y,
                        BangoGlyphString *glyphs)
{
  BdkPixmapObject *private = (BdkPixmapObject *)drawable;

  _bdk_gc_remove_drawable_clip (gc);  
  bdk_draw_glyphs (private->impl, gc, font, x, y, glyphs);
}

static void
bdk_pixmap_draw_glyphs_transformed (BdkDrawable      *drawable,
				    BdkGC            *gc,
				    BangoMatrix      *matrix,
				    BangoFont        *font,
				    gint              x,
				    gint              y,
				    BangoGlyphString *glyphs)
{
  BdkPixmapObject *private = (BdkPixmapObject *)drawable;

  _bdk_gc_remove_drawable_clip (gc);  
  bdk_draw_glyphs_transformed (private->impl, gc, matrix, font, x, y, glyphs);
}

static void
bdk_pixmap_draw_image (BdkDrawable     *drawable,
                       BdkGC           *gc,
                       BdkImage        *image,
                       gint             xsrc,
                       gint             ysrc,
                       gint             xdest,
                       gint             ydest,
                       gint             width,
                       gint             height)
{
  BdkPixmapObject *private = (BdkPixmapObject *)drawable;

  _bdk_gc_remove_drawable_clip (gc);  
  bdk_draw_image (private->impl, gc, image, xsrc, ysrc, xdest, ydest,
                  width, height);
}

static void
bdk_pixmap_draw_pixbuf (BdkDrawable     *drawable,
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
  BdkPixmapObject *private = (BdkPixmapObject *)drawable;

  if (gc)
    _bdk_gc_remove_drawable_clip (gc);  
  bdk_draw_pixbuf (private->impl, gc, pixbuf,
		   src_x, src_y, dest_x, dest_y, width, height,
		   dither, x_dither, y_dither);
}

static void
bdk_pixmap_draw_trapezoids (BdkDrawable     *drawable,
			    BdkGC	    *gc,
			    BdkTrapezoid    *trapezoids,
			    gint             n_trapezoids)
{
  BdkPixmapObject *private = (BdkPixmapObject *)drawable;

  _bdk_gc_remove_drawable_clip (gc);  
  bdk_draw_trapezoids (private->impl, gc, trapezoids, n_trapezoids);
}

static void
bdk_pixmap_real_get_size (BdkDrawable *drawable,
                          gint *width,
                          gint *height)
{
  g_return_if_fail (BDK_IS_PIXMAP (drawable));

  bdk_drawable_get_size (BDK_DRAWABLE (((BdkPixmapObject*)drawable)->impl),
                         width, height);
}

static BdkVisual*
bdk_pixmap_real_get_visual (BdkDrawable *drawable)
{
  BdkColormap *colormap;

  g_return_val_if_fail (BDK_IS_PIXMAP (drawable), NULL);

  colormap = bdk_drawable_get_colormap (drawable);
  return colormap ? bdk_colormap_get_visual (colormap) : NULL;
}

static gint
bdk_pixmap_real_get_depth (BdkDrawable *drawable)
{
  gint depth;
  
  g_return_val_if_fail (BDK_IS_PIXMAP (drawable), 0);

  depth = BDK_PIXMAP_OBJECT (drawable)->depth;

  return depth;
}

static void
bdk_pixmap_real_set_colormap (BdkDrawable *drawable,
                              BdkColormap *cmap)
{
  g_return_if_fail (BDK_IS_PIXMAP (drawable));  
  
  bdk_drawable_set_colormap (((BdkPixmapObject*)drawable)->impl, cmap);
}

static BdkColormap*
bdk_pixmap_real_get_colormap (BdkDrawable *drawable)
{
  g_return_val_if_fail (BDK_IS_PIXMAP (drawable), NULL);
  
  return bdk_drawable_get_colormap (((BdkPixmapObject*)drawable)->impl);
}

static BdkImage*
bdk_pixmap_copy_to_image (BdkDrawable     *drawable,
			  BdkImage        *image,
			  gint             src_x,
			  gint             src_y,
			  gint             dest_x,
			  gint             dest_y,
			  gint             width,
			  gint             height)
{
  g_return_val_if_fail (BDK_IS_PIXMAP (drawable), NULL);
  
  return bdk_drawable_copy_to_image (((BdkPixmapObject*)drawable)->impl,
				     image,
				     src_x, src_y, dest_x, dest_y,
				     width, height);
}

static bairo_surface_t *
bdk_pixmap_ref_bairo_surface (BdkDrawable *drawable)
{
  return _bdk_drawable_ref_bairo_surface (((BdkPixmapObject*)drawable)->impl);
}

static bairo_surface_t *
bdk_pixmap_create_bairo_surface (BdkDrawable *drawable,
				 int width,
				 int height)
{
  return _bdk_windowing_create_bairo_surface (BDK_PIXMAP_OBJECT(drawable)->impl,
					      width, height);
}



static BdkBitmap *
make_solid_mask (BdkScreen *screen, gint width, gint height)
{
  BdkBitmap *bitmap;
  BdkGC *gc;
  BdkGCValues gc_values;
  
  bitmap = bdk_pixmap_new (bdk_screen_get_root_window (screen),
			   width, height, 1);

  gc_values.foreground.pixel = 1;
  gc = bdk_gc_new_with_values (bitmap, &gc_values, BDK_GC_FOREGROUND);
  
  bdk_draw_rectangle (bitmap, gc, TRUE, 0, 0, width, height);
  
  g_object_unref (gc);
  
  return bitmap;
}

#define PACKED_COLOR(c) ((((c)->red   & 0xff00)  << 8) |   \
			  ((c)->green & 0xff00)        |   \
			  ((c)->blue             >> 8))

static BdkPixmap *
bdk_pixmap_colormap_new_from_pixbuf (BdkColormap    *colormap,
				     BdkBitmap     **mask,
				     const BdkColor *transparent_color,
				     BdkPixbuf      *pixbuf)
{
  BdkPixmap *pixmap;
  BdkPixbuf *render_pixbuf;
  BdkGC *tmp_gc;
  BdkScreen *screen = bdk_colormap_get_screen (colormap);
  
  pixmap = bdk_pixmap_new (bdk_screen_get_root_window (screen),
			   bdk_pixbuf_get_width (pixbuf),
			   bdk_pixbuf_get_height (pixbuf),
			   bdk_colormap_get_visual (colormap)->depth);
  bdk_drawable_set_colormap (pixmap, colormap);
  
  if (transparent_color)
    {
      guint32 packed_color = PACKED_COLOR (transparent_color);
      render_pixbuf = bdk_pixbuf_composite_color_simple (pixbuf,
							 bdk_pixbuf_get_width (pixbuf),
							 bdk_pixbuf_get_height (pixbuf),
							 BDK_INTERP_NEAREST,
							 255, 16, packed_color, packed_color);
    }
  else
    render_pixbuf = pixbuf;

  tmp_gc = _bdk_drawable_get_scratch_gc (pixmap, FALSE);
  bdk_draw_pixbuf (pixmap, tmp_gc, render_pixbuf, 0, 0, 0, 0,
		   bdk_pixbuf_get_width (render_pixbuf),
		   bdk_pixbuf_get_height (render_pixbuf),
		   BDK_RGB_DITHER_NORMAL, 0, 0);

  if (render_pixbuf != pixbuf)
    g_object_unref (render_pixbuf);

  if (mask)
    bdk_pixbuf_render_pixmap_and_mask_for_colormap (pixbuf, colormap, NULL, mask, 128);

  if (mask && !*mask)
    *mask = make_solid_mask (screen,
			     bdk_pixbuf_get_width (pixbuf),
			     bdk_pixbuf_get_height (pixbuf));

  return pixmap;
}

/**
 * bdk_pixmap_colormap_create_from_xpm:
 * @drawable: a #BdkDrawable, used to determine default values
 * for the new pixmap. Can be %NULL if @colormap is given.
 * @colormap: the #BdkColormap that the new pixmap will be use.
 * If omitted, the colormap for @window will be used.
 * @mask: a pointer to a place to store a bitmap representing
 * the transparency mask of the XPM file. Can be %NULL,
 * in which case transparency will be ignored.
 * @transparent_color: the color to be used for the pixels
 * that are transparent in the input file. Can be %NULL,
 * in which case a default color will be used.
 * @filename: the filename of a file containing XPM data.
 *
 * Create a pixmap from a XPM file using a particular colormap.
 *
 * Returns: (transfer none): the #BdkPixmap.
 *
 * Deprecated: 2.22: Use a #BdkPixbuf instead. You can use
 * bdk_pixbuf_new_from_file() to create it.
 * If you must use a pixmap, use bdk_pixmap_new() to
 * create it and Bairo to draw the pixbuf onto it.
 */
BdkPixmap*
bdk_pixmap_colormap_create_from_xpm (BdkDrawable    *drawable,
				     BdkColormap    *colormap,
				     BdkBitmap     **mask,
				     const BdkColor *transparent_color,
				     const gchar    *filename)
{
  BdkPixbuf *pixbuf;
  BdkPixmap *pixmap;

  g_return_val_if_fail (drawable != NULL || colormap != NULL, NULL);
  g_return_val_if_fail (drawable == NULL || BDK_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (colormap == NULL || BDK_IS_COLORMAP (colormap), NULL);

  if (colormap == NULL)
    colormap = bdk_drawable_get_colormap (drawable);
  
  pixbuf = bdk_pixbuf_new_from_file (filename, NULL);
  if (!pixbuf)
    return NULL;

  pixmap = bdk_pixmap_colormap_new_from_pixbuf (colormap, mask, transparent_color, pixbuf);

  g_object_unref (pixbuf);
  
  return pixmap;
}

/**
 * bdk_pixmap_create_from_xpm:
 * @drawable: a #BdkDrawable, used to determine default values
 * for the new pixmap.
 * @mask: (out) a pointer to a place to store a bitmap representing
 * the transparency mask of the XPM file. Can be %NULL,
 * in which case transparency will be ignored.
 * @transparent_color: the color to be used for the pixels
 * that are transparent in the input file. Can be %NULL,
 * in which case a default color will be used.
 * @filename: the filename of a file containing XPM data.
 *
 * Create a pixmap from a XPM file.
 *
 * Returns: (transfer none): the #BdkPixmap
 *
 * Deprecated: 2.22: Use a #BdkPixbuf instead. You can use
 * bdk_pixbuf_new_from_file() to create it.
 * If you must use a pixmap, use bdk_pixmap_new() to
 * create it and Bairo to draw the pixbuf onto it.
 */
BdkPixmap*
bdk_pixmap_create_from_xpm (BdkDrawable    *drawable,
			    BdkBitmap     **mask,
			    const BdkColor *transparent_color,
			    const gchar    *filename)
{
  return bdk_pixmap_colormap_create_from_xpm (drawable, NULL, mask,
					      transparent_color, filename);
}

/**
 * bdk_pixmap_colormap_create_from_xpm_d:
 * @drawable: a #BdkDrawable, used to determine default values
 *     for the new pixmap. Can be %NULL if @colormap is given.
 * @colormap: the #BdkColormap that the new pixmap will be use.
 *     If omitted, the colormap for @window will be used.
 * @mask: a pointer to a place to store a bitmap representing
 *     the transparency mask of the XPM file. Can be %NULL,
 *     in which case transparency will be ignored.
 * @transparent_color: the color to be used for the pixels
 *     that are transparent in the input file. Can be %NULL,
 *     in which case a default color will be used.
 * @data: Pointer to a string containing the XPM data.
 *
 * Create a pixmap from data in XPM format using a particular
 * colormap.
 *
 * Returns: (transfer none): the #BdkPixmap.
 *
 * Deprecated: 2.22: Use a #BdkPixbuf instead. You can use
 * bdk_pixbuf_new_from_xpm_data() to create it.
 * If you must use a pixmap, use bdk_pixmap_new() to
 * create it and Bairo to draw the pixbuf onto it.
 */
BdkPixmap*
bdk_pixmap_colormap_create_from_xpm_d (BdkDrawable     *drawable,
				       BdkColormap     *colormap,
				       BdkBitmap      **mask,
				       const BdkColor  *transparent_color,
				       gchar          **data)
{
  BdkPixbuf *pixbuf;
  BdkPixmap *pixmap;

  g_return_val_if_fail (drawable != NULL || colormap != NULL, NULL);
  g_return_val_if_fail (drawable == NULL || BDK_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (colormap == NULL || BDK_IS_COLORMAP (colormap), NULL);

  if (colormap == NULL)
    colormap = bdk_drawable_get_colormap (drawable);
  
  pixbuf = bdk_pixbuf_new_from_xpm_data ((const char **)data);
  if (!pixbuf)
    return NULL;

  pixmap = bdk_pixmap_colormap_new_from_pixbuf (colormap, mask, transparent_color, pixbuf);

  g_object_unref (pixbuf);

  return pixmap;
}

/**
 * bdk_pixmap_create_from_xpm_d:
 * @drawable: a #BdkDrawable, used to determine default values
 *     for the new pixmap.
 * @mask: (out): Pointer to a place to store a bitmap representing
 *     the transparency mask of the XPM file. Can be %NULL,
 *     in which case transparency will be ignored.
 * @transparent_color: This color will be used for the pixels
 *     that are transparent in the input file. Can be %NULL
 *     in which case a default color will be used.
 * @data: Pointer to a string containing the XPM data.
 *
 * Create a pixmap from data in XPM format.
 *
 * Returns: (transfer none): the #BdkPixmap.
 *
 * Deprecated: 2.22: Use a #BdkPixbuf instead. You can use
 * bdk_pixbuf_new_from_xpm_data() to create it.
 * If you must use a pixmap, use bdk_pixmap_new() to
 * create it and Bairo to draw the pixbuf onto it.
 */
BdkPixmap*
bdk_pixmap_create_from_xpm_d (BdkDrawable    *drawable,
			      BdkBitmap     **mask,
			      const BdkColor *transparent_color,
			      gchar         **data)
{
  return bdk_pixmap_colormap_create_from_xpm_d (drawable, NULL, mask,
						transparent_color, data);
}

static BdkScreen*
bdk_pixmap_real_get_screen (BdkDrawable *drawable)
{
    return bdk_drawable_get_screen (BDK_PIXMAP_OBJECT (drawable)->impl);
}

/**
 * bdk_pixmap_get_size:
 * @pixmap: a #BdkPixmap
 * @width: (out) (allow-none): location to store @pixmap's width, or %NULL
 * @height: (out) (allow-none): location to store @pixmap's height, or %NULL
 *
 * This function is purely to make it possible to query the size of pixmaps
 * even when compiling without deprecated symbols and you must use pixmaps.
 * It is identical to bdk_drawable_get_size(), but for pixmaps.
 *
 * Since: 2.24
 **/
void
bdk_pixmap_get_size (BdkPixmap *pixmap,
                     gint      *width,
                     gint      *height)
{
    g_return_if_fail (BDK_IS_PIXMAP (pixmap));

    bdk_drawable_get_size (pixmap, width, height);
}

#define __BDK_PIXMAP_C__
#include "bdkaliasdef.c"
