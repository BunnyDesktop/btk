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

#ifndef __BDK_DRAWABLE_H__
#define __BDK_DRAWABLE_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bdk/bdktypes.h>
#include <bdk/bdkgc.h>
#include <bdk/bdkrgb.h>
#include <bdk-pixbuf/bdk-pixbuf.h>

#include <bairo.h>

G_BEGIN_DECLS

typedef struct _BdkDrawableClass BdkDrawableClass;
typedef struct _BdkTrapezoid     BdkTrapezoid;

#define BDK_TYPE_DRAWABLE              (bdk_drawable_get_type ())
#define BDK_DRAWABLE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_DRAWABLE, BdkDrawable))
#define BDK_DRAWABLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_DRAWABLE, BdkDrawableClass))
#define BDK_IS_DRAWABLE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_DRAWABLE))
#define BDK_IS_DRAWABLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_DRAWABLE))
#define BDK_DRAWABLE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_DRAWABLE, BdkDrawableClass))

struct _BdkDrawable
{
  GObject parent_instance;
};
 
struct _BdkDrawableClass 
{
  GObjectClass parent_class;
  
  BdkGC *(*create_gc)    (BdkDrawable    *drawable,
		          BdkGCValues    *values,
		          BdkGCValuesMask mask);
  void (*draw_rectangle) (BdkDrawable  *drawable,
			  BdkGC	       *gc,
			  gboolean	filled,
			  gint		x,
			  gint		y,
			  gint		width,
			  gint		height);
  void (*draw_arc)       (BdkDrawable  *drawable,
			  BdkGC	       *gc,
			  gboolean	filled,
			  gint		x,
			  gint		y,
			  gint		width,
			  gint		height,
			  gint		angle1,
			  gint		angle2);
  void (*draw_polygon)   (BdkDrawable  *drawable,
			  BdkGC	       *gc,
			  gboolean	filled,
			  BdkPoint     *points,
			  gint		npoints);
  void (*draw_text)      (BdkDrawable  *drawable,
			  BdkFont      *font,
			  BdkGC	       *gc,
			  gint		x,
			  gint		y,
			  const gchar  *text,
			  gint		text_length);
  void (*draw_text_wc)   (BdkDrawable	 *drawable,
			  BdkFont	 *font,
			  BdkGC		 *gc,
			  gint		  x,
			  gint		  y,
			  const BdkWChar *text,
			  gint		  text_length);
  void (*draw_drawable)  (BdkDrawable  *drawable,
			  BdkGC	       *gc,
			  BdkDrawable  *src,
			  gint		xsrc,
			  gint		ysrc,
			  gint		xdest,
			  gint		ydest,
			  gint		width,
			  gint		height);
  void (*draw_points)	 (BdkDrawable  *drawable,
			  BdkGC	       *gc,
			  BdkPoint     *points,
			  gint		npoints);
  void (*draw_segments)	 (BdkDrawable  *drawable,
			  BdkGC	       *gc,
			  BdkSegment   *segs,
			  gint		nsegs);
  void (*draw_lines)     (BdkDrawable  *drawable,
			  BdkGC        *gc,
			  BdkPoint     *points,
			  gint          npoints);

  void (*draw_glyphs)    (BdkDrawable      *drawable,
			  BdkGC	           *gc,
			  BangoFont        *font,
			  gint              x,
			  gint              y,
			  BangoGlyphString *glyphs);

  void (*draw_image)     (BdkDrawable *drawable,
                          BdkGC	      *gc,
                          BdkImage    *image,
                          gint	       xsrc,
                          gint	       ysrc,
                          gint	       xdest,
                          gint	       ydest,
                          gint	       width,
                          gint	       height);
  
  gint (*get_depth)      (BdkDrawable  *drawable);
  void (*get_size)       (BdkDrawable  *drawable,
                          gint         *width,
                          gint         *height);

  void (*set_colormap)   (BdkDrawable  *drawable,
                          BdkColormap  *cmap);

  BdkColormap* (*get_colormap)	(BdkDrawable  *drawable);
  BdkVisual*   (*get_visual)	(BdkDrawable  *drawable);
  BdkScreen*   (*get_screen)	(BdkDrawable  *drawable);

  BdkImage*    (*get_image)  (BdkDrawable  *drawable,
                              gint          x,
                              gint          y,
                              gint          width,
                              gint          height);

  BdkRebunnyion*   (*get_clip_rebunnyion)    (BdkDrawable  *drawable);
  BdkRebunnyion*   (*get_visible_rebunnyion) (BdkDrawable  *drawable);

  BdkDrawable* (*get_composite_drawable) (BdkDrawable *drawable,
                                          gint         x,
                                          gint         y,
                                          gint         width,
                                          gint         height,
                                          gint        *composite_x_offset,
                                          gint        *composite_y_offset);

  void         (*draw_pixbuf) (BdkDrawable *drawable,
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
			       gint         y_dither);
  BdkImage*    (*_copy_to_image) (BdkDrawable    *drawable,
				  BdkImage       *image,
				  gint            src_x,
				  gint            src_y,
				  gint            dest_x,
				  gint            dest_y,
				  gint            width,
				  gint            height);
  
  void (*draw_glyphs_transformed) (BdkDrawable      *drawable,
				   BdkGC	    *gc,
				   BangoMatrix      *matrix,
				   BangoFont        *font,
				   gint              x,
				   gint              y,
				   BangoGlyphString *glyphs);
  void (*draw_trapezoids)         (BdkDrawable      *drawable,
				   BdkGC	    *gc,
				   BdkTrapezoid     *trapezoids,
				   gint              n_trapezoids);

  bairo_surface_t *(*ref_bairo_surface) (BdkDrawable *drawable);

  BdkDrawable *(*get_source_drawable) (BdkDrawable *drawable);

  void         (*set_bairo_clip)      (BdkDrawable *drawable,
				       bairo_t *cr);

  bairo_surface_t * (*create_bairo_surface) (BdkDrawable *drawable,
					     int width,
					     int height);

  void (*draw_drawable_with_src)  (BdkDrawable  *drawable,
				   BdkGC	       *gc,
				   BdkDrawable  *src,
				   gint		xsrc,
				   gint		ysrc,
				   gint		xdest,
				   gint		ydest,
				   gint		width,
				   gint		height,
				   BdkDrawable  *original_src);

  /* Padding for future expansion */
  void         (*_bdk_reserved7)  (void);
  void         (*_bdk_reserved9)  (void);
  void         (*_bdk_reserved10) (void);
  void         (*_bdk_reserved11) (void);
  void         (*_bdk_reserved12) (void);
  void         (*_bdk_reserved13) (void);
  void         (*_bdk_reserved14) (void);
  void         (*_bdk_reserved15) (void);
};

struct _BdkTrapezoid
{
  double y1, x11, x21, y2, x12, x22;
};

GType           bdk_drawable_get_type     (void) G_GNUC_CONST;

/* Manipulation of drawables
 */

#ifndef BDK_DISABLE_DEPRECATED
void            bdk_drawable_set_data     (BdkDrawable    *drawable,
					   const gchar    *key,
					   gpointer	  data,
					   GDestroyNotify  destroy_func);
gpointer        bdk_drawable_get_data     (BdkDrawable    *drawable,
					   const gchar    *key);
#endif /* BDK_DISABLE_DEPRECATED */

void	        bdk_drawable_set_colormap (BdkDrawable	  *drawable,
					   BdkColormap	  *colormap);
BdkColormap*    bdk_drawable_get_colormap (BdkDrawable	  *drawable);
gint            bdk_drawable_get_depth    (BdkDrawable	  *drawable);

#if !defined (BDK_DISABLE_DEPRECATED)
void            bdk_drawable_get_size     (BdkDrawable	  *drawable,
					   gint	          *width,
					   gint  	  *height);
BdkVisual*      bdk_drawable_get_visual   (BdkDrawable	  *drawable);
BdkScreen*	bdk_drawable_get_screen   (BdkDrawable    *drawable);
BdkDisplay*	bdk_drawable_get_display  (BdkDrawable    *drawable);
#endif /* BDK_DISABLE_DEPRECATED */

#ifndef BDK_DISABLE_DEPRECATED
BdkDrawable*    bdk_drawable_ref          (BdkDrawable    *drawable);
void            bdk_drawable_unref        (BdkDrawable    *drawable);
#endif /* BDK_DISABLE_DEPRECATED */

/* Drawing
 */
#ifndef BDK_DISABLE_DEPRECATED
void bdk_draw_point     (BdkDrawable      *drawable,
			 BdkGC            *gc,
			 gint              x,
			 gint              y);
void bdk_draw_line      (BdkDrawable      *drawable,
			 BdkGC            *gc,
			 gint              x1_,
			 gint              y1_,
			 gint              x2_,
			 gint              y2_);
void bdk_draw_rectangle (BdkDrawable      *drawable,
			 BdkGC            *gc,
			 gboolean          filled,
			 gint              x,
			 gint              y,
			 gint              width,
			 gint              height);
void bdk_draw_arc       (BdkDrawable      *drawable,
			 BdkGC            *gc,
			 gboolean          filled,
			 gint              x,
			 gint              y,
			 gint              width,
			 gint              height,
			 gint              angle1,
			 gint              angle2);
void bdk_draw_polygon   (BdkDrawable      *drawable,
			 BdkGC            *gc,
			 gboolean          filled,
			 const BdkPoint   *points,
			 gint              n_points);
void bdk_draw_string    (BdkDrawable      *drawable,
			 BdkFont          *font,
			 BdkGC            *gc,
			 gint              x,
			 gint              y,
			 const gchar      *string);
void bdk_draw_text      (BdkDrawable      *drawable,
			 BdkFont          *font,
			 BdkGC            *gc,
			 gint              x,
			 gint              y,
			 const gchar      *text,
			 gint              text_length);
void bdk_draw_text_wc   (BdkDrawable      *drawable,
			 BdkFont          *font,
			 BdkGC            *gc,
			 gint              x,
			 gint              y,
			 const BdkWChar   *text,
			 gint              text_length);
void bdk_draw_drawable  (BdkDrawable      *drawable,
			 BdkGC            *gc,
			 BdkDrawable      *src,
			 gint              xsrc,
			 gint              ysrc,
			 gint              xdest,
			 gint              ydest,
			 gint              width,
			 gint              height);
void bdk_draw_image     (BdkDrawable      *drawable,
			 BdkGC            *gc,
			 BdkImage         *image,
			 gint              xsrc,
			 gint              ysrc,
			 gint              xdest,
			 gint              ydest,
			 gint              width,
			 gint              height);
void bdk_draw_points    (BdkDrawable      *drawable,
			 BdkGC            *gc,
			 const BdkPoint   *points,
			 gint              n_points);
void bdk_draw_segments  (BdkDrawable      *drawable,
			 BdkGC            *gc,
			 const BdkSegment *segs,
			 gint              n_segs);
void bdk_draw_lines     (BdkDrawable      *drawable,
			 BdkGC            *gc,
			 const BdkPoint   *points,
			 gint              n_points);
void bdk_draw_pixbuf    (BdkDrawable      *drawable,
			 BdkGC            *gc,
			 const BdkPixbuf  *pixbuf,
			 gint              src_x,
			 gint              src_y,
			 gint              dest_x,
			 gint              dest_y,
			 gint              width,
			 gint              height,
			 BdkRgbDither      dither,
			 gint              x_dither,
			 gint              y_dither);

void bdk_draw_glyphs      (BdkDrawable      *drawable,
			   BdkGC            *gc,
			   BangoFont        *font,
			   gint              x,
			   gint              y,
			   BangoGlyphString *glyphs);
void bdk_draw_layout_line (BdkDrawable      *drawable,
			   BdkGC            *gc,
			   gint              x,
			   gint              y,
			   BangoLayoutLine  *line);
void bdk_draw_layout      (BdkDrawable      *drawable,
			   BdkGC            *gc,
			   gint              x,
			   gint              y,
			   BangoLayout      *layout);

void bdk_draw_layout_line_with_colors (BdkDrawable     *drawable,
                                       BdkGC           *gc,
                                       gint             x,
                                       gint             y,
                                       BangoLayoutLine *line,
                                       const BdkColor  *foreground,
                                       const BdkColor  *background);
void bdk_draw_layout_with_colors      (BdkDrawable     *drawable,
                                       BdkGC           *gc,
                                       gint             x,
                                       gint             y,
                                       BangoLayout     *layout,
                                       const BdkColor  *foreground,
                                       const BdkColor  *background);

void bdk_draw_glyphs_transformed (BdkDrawable        *drawable,
				  BdkGC	             *gc,
				  const BangoMatrix  *matrix,
				  BangoFont          *font,
				  gint                x,
				  gint                y,
				  BangoGlyphString   *glyphs);
void bdk_draw_trapezoids         (BdkDrawable        *drawable,
				  BdkGC	             *gc,
				  const BdkTrapezoid *trapezoids,
				  gint                n_trapezoids);

#define bdk_draw_pixmap                bdk_draw_drawable
#define bdk_draw_bitmap                bdk_draw_drawable

BdkImage* bdk_drawable_get_image      (BdkDrawable *drawable,
                                       gint         x,
                                       gint         y,
                                       gint         width,
                                       gint         height);
BdkImage *bdk_drawable_copy_to_image (BdkDrawable  *drawable,
				      BdkImage     *image,
				      gint          src_x,
				      gint          src_y,
				      gint          dest_x,
				      gint          dest_y,
				      gint          width,
				      gint          height);
#endif /* BDK_DISABLE_DEPRECATED */

BdkRebunnyion *bdk_drawable_get_clip_rebunnyion    (BdkDrawable *drawable);
BdkRebunnyion *bdk_drawable_get_visible_rebunnyion (BdkDrawable *drawable);

G_END_DECLS

#endif /* __BDK_DRAWABLE_H__ */
