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

#ifndef __BDK_PIXMAP_H__
#define __BDK_PIXMAP_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bdk/bdktypes.h>
#include <bdk/bdkdrawable.h>

G_BEGIN_DECLS

typedef struct _BdkPixmapObject BdkPixmapObject;
typedef struct _BdkPixmapObjectClass BdkPixmapObjectClass;

#define BDK_TYPE_PIXMAP              (bdk_pixmap_get_type ())
#define BDK_PIXMAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_PIXMAP, BdkPixmap))
#define BDK_PIXMAP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_PIXMAP, BdkPixmapObjectClass))
#define BDK_IS_PIXMAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_PIXMAP))
#define BDK_IS_PIXMAP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_PIXMAP))
#define BDK_PIXMAP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_PIXMAP, BdkPixmapObjectClass))
#define BDK_PIXMAP_OBJECT(object)    ((BdkPixmapObject *) BDK_PIXMAP (object))

struct _BdkPixmapObject
{
  BdkDrawable parent_instance;
  
  BdkDrawable *GSEAL (impl);  /* window-system-specific delegate object */

  gint GSEAL (depth);
};

struct _BdkPixmapObjectClass
{
  BdkDrawableClass parent_class;

};

GType      bdk_pixmap_get_type          (void) G_GNUC_CONST;

/* Pixmaps
 */
BdkPixmap* bdk_pixmap_new		(BdkDrawable *drawable,
					 gint	      width,
					 gint	      height,
					 gint	      depth);
#ifndef BDK_DISABLE_DEPRECATED
BdkBitmap* bdk_bitmap_create_from_data	(BdkDrawable *drawable,
					 const gchar *data,
					 gint	      width,
					 gint	      height);
BdkPixmap* bdk_pixmap_create_from_data	(BdkDrawable    *drawable,
					 const gchar 	*data,
					 gint	     	 width,
					 gint	     	 height,
					 gint	         depth,
					 const BdkColor *fg,
					 const BdkColor *bg);

BdkPixmap* bdk_pixmap_create_from_xpm            (BdkDrawable    *drawable,
						  BdkBitmap     **mask,
						  const BdkColor *transparent_color,
						  const gchar    *filename);
BdkPixmap* bdk_pixmap_colormap_create_from_xpm   (BdkDrawable    *drawable,
						  BdkColormap    *colormap,
						  BdkBitmap     **mask,
						  const BdkColor *transparent_color,
						  const gchar    *filename);
BdkPixmap* bdk_pixmap_create_from_xpm_d          (BdkDrawable    *drawable,
						  BdkBitmap     **mask,
						  const BdkColor *transparent_color,
						  gchar         **data);
BdkPixmap* bdk_pixmap_colormap_create_from_xpm_d (BdkDrawable    *drawable,
						  BdkColormap    *colormap,
						  BdkBitmap     **mask,
						  const BdkColor *transparent_color,
						  gchar         **data);
#endif

void          bdk_pixmap_get_size                (BdkPixmap      *pixmap,
                                                  gint	         *width,
                                                  gint  	 *height);

/* Functions to create/lookup pixmaps from their native equivalents
 */
#ifndef BDK_MULTIHEAD_SAFE
BdkPixmap*    bdk_pixmap_foreign_new (BdkNativeWindow anid);
BdkPixmap*    bdk_pixmap_lookup      (BdkNativeWindow anid);
#endif /* BDK_MULTIHEAD_SAFE */

BdkPixmap*    bdk_pixmap_foreign_new_for_display (BdkDisplay      *display,
						  BdkNativeWindow  anid);
BdkPixmap*    bdk_pixmap_lookup_for_display      (BdkDisplay      *display,
						  BdkNativeWindow  anid);
BdkPixmap*    bdk_pixmap_foreign_new_for_screen  (BdkScreen       *screen,
						  BdkNativeWindow  anid,
						  gint             width,
                                                  gint             height,
                                                  gint             depth);

#ifndef BDK_DISABLE_DEPRECATED
#define bdk_bitmap_ref                 g_object_ref
#define bdk_bitmap_unref               g_object_unref
#define bdk_pixmap_ref                 g_object_ref
#define bdk_pixmap_unref               g_object_unref
#endif /* BDK_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* __BDK_PIXMAP_H__ */
