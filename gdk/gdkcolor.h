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

#ifndef __BDK_COLOR_H__
#define __BDK_COLOR_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bairo.h>
#include <bdk/bdktypes.h>

G_BEGIN_DECLS

/* The color type.
 *   A color consists of red, green and blue values in the
 *    range 0-65535 and a pixel value. The pixel value is highly
 *    dependent on the depth and colormap which this color will
 *    be used to draw into. Therefore, sharing colors between
 *    colormaps is a bad idea.
 */
struct _BdkColor
{
  guint32 pixel;
  guint16 red;
  guint16 green;
  guint16 blue;
};

/* The colormap type.
 */

typedef struct _BdkColormapClass BdkColormapClass;

#define BDK_TYPE_COLORMAP              (bdk_colormap_get_type ())
#define BDK_COLORMAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_COLORMAP, BdkColormap))
#define BDK_COLORMAP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_COLORMAP, BdkColormapClass))
#define BDK_IS_COLORMAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_COLORMAP))
#define BDK_IS_COLORMAP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_COLORMAP))
#define BDK_COLORMAP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_COLORMAP, BdkColormapClass))

#define BDK_TYPE_COLOR                 (bdk_color_get_type ())

struct _BdkColormap
{
  /*< private >*/
  GObject parent_instance;

  /*< public >*/
  gint      GSEAL (size);
  BdkColor *GSEAL (colors);

  /*< private >*/
  BdkVisual *GSEAL (visual);
  
  gpointer GSEAL (windowing_data);
};

struct _BdkColormapClass
{
  GObjectClass parent_class;

};

GType        bdk_colormap_get_type (void) G_GNUC_CONST;

BdkColormap* bdk_colormap_new	  (BdkVisual   *visual,
				   gboolean	allocate);

#ifndef BDK_DISABLE_DEPRECATED
BdkColormap* bdk_colormap_ref	  (BdkColormap *cmap);
void	     bdk_colormap_unref	  (BdkColormap *cmap);
#endif 

#ifndef BDK_MULTIHEAD_SAFE
BdkColormap* bdk_colormap_get_system	        (void);
#endif

BdkScreen *bdk_colormap_get_screen (BdkColormap *cmap);

#ifndef BDK_DISABLE_DEPRECATED
gint bdk_colormap_get_system_size  (void);
#endif

#if !defined (BDK_DISABLE_DEPRECATED) || defined (BDK_COMPILATION)
/* Used by bdk_colors_store () */
void bdk_colormap_change (BdkColormap	*colormap,
			  gint		 ncolors);
#endif 

gint  bdk_colormap_alloc_colors   (BdkColormap    *colormap,
				   BdkColor       *colors,
				   gint            n_colors,
				   gboolean        writeable,
				   gboolean        best_match,
				   gboolean       *success);
gboolean bdk_colormap_alloc_color (BdkColormap    *colormap,
				   BdkColor       *color,
				   gboolean        writeable,
				   gboolean        best_match);
void     bdk_colormap_free_colors (BdkColormap    *colormap,
				   const BdkColor *colors,
				   gint            n_colors);
void     bdk_colormap_query_color (BdkColormap    *colormap,
				   gulong          pixel,
				   BdkColor       *result);

BdkVisual *bdk_colormap_get_visual (BdkColormap *colormap);

BdkColor *bdk_color_copy      (const BdkColor *color);
void      bdk_color_free      (BdkColor       *color);
gboolean  bdk_color_parse     (const gchar    *spec,
			       BdkColor       *color);
guint     bdk_color_hash      (const BdkColor *colora);
gboolean  bdk_color_equal     (const BdkColor *colora,
			       const BdkColor *colorb);
gchar *   bdk_color_to_string (const BdkColor *color);

GType     bdk_color_get_type (void) G_GNUC_CONST;

/* The following functions are deprecated */
#ifndef BDK_DISABLE_DEPRECATED
void bdk_colors_store	 (BdkColormap	*colormap,
			  BdkColor	*colors,
			  gint		 ncolors);
gint bdk_color_white	 (BdkColormap	*colormap,
			  BdkColor	*color);
gint bdk_color_black	 (BdkColormap	*colormap,
			  BdkColor	*color);
gint bdk_color_alloc	 (BdkColormap	*colormap,
			  BdkColor	*color);
gint bdk_color_change	 (BdkColormap	*colormap,
			  BdkColor	*color);
#endif /* BDK_DISABLE_DEPRECATED */

#if !defined (BDK_DISABLE_DEPRECATED) || defined (BDK_COMPILATION)
/* Used by bdk_rgb_try_colormap () */
gint bdk_colors_alloc	 (BdkColormap	*colormap,
			  gboolean	 contiguous,
			  gulong	*planes,
			  gint		 nplanes,
			  gulong	*pixels,
			  gint		 npixels);
void bdk_colors_free	 (BdkColormap	*colormap,
			  gulong	*pixels,
			  gint		 npixels,
			  gulong	 planes);
#endif /* !BDK_DISABLE_DEPRECATED || BDK_COMPILATION */

G_END_DECLS

#endif /* __BDK_COLOR_H__ */
