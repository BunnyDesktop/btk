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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* Needed for SEEK_END in SunOS */
#include <unistd.h>
#include <X11/Xlib.h>

#include "bdkx.h"

#include "bdkpixmap-x11.h"
#include "bdkprivate-x11.h"
#include "bdkscreen-x11.h"
#include "bdkdisplay-x11.h"

#include <bdk/bdkinternals.h>
#include "bdkalias.h"

typedef struct
{
  bchar *color_string;
  BdkColor color;
  bint transparent;
} _BdkPixmapColor;

typedef struct
{
  buint ncolors;
  BdkColormap *colormap;
  bulong pixels[1];
} _BdkPixmapInfo;

static void bdk_pixmap_impl_x11_get_size   (BdkDrawable        *drawable,
                                            bint               *width,
                                            bint               *height);

static void bdk_pixmap_impl_x11_dispose    (BObject            *object);
static void bdk_pixmap_impl_x11_finalize   (BObject            *object);

G_DEFINE_TYPE (BdkPixmapImplX11, bdk_pixmap_impl_x11, BDK_TYPE_DRAWABLE_IMPL_X11)

GType
_bdk_pixmap_impl_get_type (void)
{
  return bdk_pixmap_impl_x11_get_type ();
}

static void
bdk_pixmap_impl_x11_init (BdkPixmapImplX11 *impl)
{
  impl->width = 1;
  impl->height = 1;
}

static void
bdk_pixmap_impl_x11_class_init (BdkPixmapImplX11Class *klass)
{
  BObjectClass *object_class = B_OBJECT_CLASS (klass);
  BdkDrawableClass *drawable_class = BDK_DRAWABLE_CLASS (klass);
  
  object_class->dispose  = bdk_pixmap_impl_x11_dispose;
  object_class->finalize = bdk_pixmap_impl_x11_finalize;

  drawable_class->get_size = bdk_pixmap_impl_x11_get_size;
}

static void
bdk_pixmap_impl_x11_dispose (BObject *object)
{
  BdkPixmapImplX11 *impl = BDK_PIXMAP_IMPL_X11 (object);
  BdkPixmap *wrapper = BDK_PIXMAP (BDK_DRAWABLE_IMPL_X11 (impl)->wrapper);
  BdkDisplay *display = BDK_PIXMAP_DISPLAY (wrapper);

  if (!display->closed)
    {
      if (!impl->is_foreign)
	XFreePixmap (BDK_DISPLAY_XDISPLAY (display), BDK_PIXMAP_XID (wrapper));
    }

  _bdk_xid_table_remove (display, BDK_PIXMAP_XID (wrapper));

  B_OBJECT_CLASS (bdk_pixmap_impl_x11_parent_class)->dispose (object);
}

static void
bdk_pixmap_impl_x11_finalize (BObject *object)
{
  BdkPixmapImplX11 *impl = BDK_PIXMAP_IMPL_X11 (object);
  BdkPixmap *wrapper = BDK_PIXMAP (BDK_DRAWABLE_IMPL_X11 (impl)->wrapper);
  BdkDisplay *display = BDK_PIXMAP_DISPLAY (wrapper);

  if (!display->closed)
    {
      BdkDrawableImplX11 *draw_impl = BDK_DRAWABLE_IMPL_X11 (impl);

      _bdk_x11_drawable_finish (BDK_DRAWABLE (draw_impl));
    }

  B_OBJECT_CLASS (bdk_pixmap_impl_x11_parent_class)->finalize (object);
}

static void
bdk_pixmap_impl_x11_get_size   (BdkDrawable *drawable,
                                bint        *width,
                                bint        *height)
{
  if (width)
    *width = BDK_PIXMAP_IMPL_X11 (drawable)->width;
  if (height)
    *height = BDK_PIXMAP_IMPL_X11 (drawable)->height;
}

BdkPixmap*
_bdk_pixmap_new (BdkDrawable *drawable,
                 bint         width,
                 bint         height,
                 bint         depth)
{
  BdkPixmap *pixmap;
  BdkDrawableImplX11 *draw_impl;
  BdkPixmapImplX11 *pix_impl;
  BdkColormap *cmap;
  bint window_depth;
  
  g_return_val_if_fail (drawable == NULL || BDK_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail ((drawable != NULL) || (depth != -1), NULL);
  g_return_val_if_fail ((width != 0) && (height != 0), NULL);
  
  if (!drawable)
    {
      BDK_NOTE (MULTIHEAD, g_message ("need to specify the screen parent window "
				      "for bdk_pixmap_new() to be multihead safe"));
      drawable = bdk_screen_get_root_window (bdk_screen_get_default ());
    }

  if (BDK_IS_WINDOW (drawable) && BDK_WINDOW_DESTROYED (drawable))
    return NULL;

  window_depth = bdk_drawable_get_depth (BDK_DRAWABLE (drawable));
  if (depth == -1)
    depth = window_depth;

  pixmap = g_object_new (bdk_pixmap_get_type (), NULL);
  draw_impl = BDK_DRAWABLE_IMPL_X11 (BDK_PIXMAP_OBJECT (pixmap)->impl);
  pix_impl = BDK_PIXMAP_IMPL_X11 (BDK_PIXMAP_OBJECT (pixmap)->impl);
  draw_impl->wrapper = BDK_DRAWABLE (pixmap);
  
  draw_impl->screen = BDK_WINDOW_SCREEN (drawable);
  draw_impl->xid = XCreatePixmap (BDK_PIXMAP_XDISPLAY (pixmap),
                                  BDK_WINDOW_XID (drawable),
                                  width, height, depth);
  
  pix_impl->is_foreign = FALSE;
  pix_impl->width = width;
  pix_impl->height = height;
  BDK_PIXMAP_OBJECT (pixmap)->depth = depth;

  if (depth == window_depth)
    {
      cmap = bdk_drawable_get_colormap (drawable);
      if (cmap)
        bdk_drawable_set_colormap (pixmap, cmap);
    }
  
  _bdk_xid_table_insert (BDK_WINDOW_DISPLAY (drawable), 
			 &BDK_PIXMAP_XID (pixmap), pixmap);
  return pixmap;
}

BdkPixmap *
_bdk_bitmap_create_from_data (BdkDrawable *drawable,
                              const bchar *data,
                              bint         width,
                              bint         height)
{
  BdkPixmap *pixmap;
  BdkDrawableImplX11 *draw_impl;
  BdkPixmapImplX11 *pix_impl;
  
  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail ((width != 0) && (height != 0), NULL);
  g_return_val_if_fail (drawable == NULL || BDK_IS_DRAWABLE (drawable), NULL);

  if (!drawable)
    {
      BDK_NOTE (MULTIHEAD, g_message ("need to specify the screen parent window "
				     "for bdk_bitmap_create_from_data() to be multihead safe"));
      drawable = bdk_screen_get_root_window (bdk_screen_get_default ());
    }
  
  if (BDK_IS_WINDOW (drawable) && BDK_WINDOW_DESTROYED (drawable))
    return NULL;

  pixmap = g_object_new (bdk_pixmap_get_type (), NULL);
  draw_impl = BDK_DRAWABLE_IMPL_X11 (BDK_PIXMAP_OBJECT (pixmap)->impl);
  pix_impl = BDK_PIXMAP_IMPL_X11 (BDK_PIXMAP_OBJECT (pixmap)->impl);
  draw_impl->wrapper = BDK_DRAWABLE (pixmap);

  pix_impl->is_foreign = FALSE;
  pix_impl->width = width;
  pix_impl->height = height;
  BDK_PIXMAP_OBJECT (pixmap)->depth = 1;

  draw_impl->screen = BDK_WINDOW_SCREEN (drawable);
  draw_impl->xid = XCreateBitmapFromData (BDK_WINDOW_XDISPLAY (drawable),
                                          BDK_WINDOW_XID (drawable),
                                          (char *)data, width, height);

  _bdk_xid_table_insert (BDK_WINDOW_DISPLAY (drawable), 
			 &BDK_PIXMAP_XID (pixmap), pixmap);
  return pixmap;
}

BdkPixmap*
_bdk_pixmap_create_from_data (BdkDrawable    *drawable,
			      const bchar    *data,
			      bint            width,
			      bint            height,
			      bint            depth,
			      const BdkColor *fg,
			      const BdkColor *bg)
{
  BdkPixmap *pixmap;
  BdkDrawableImplX11 *draw_impl;
  BdkPixmapImplX11 *pix_impl;

  g_return_val_if_fail (drawable == NULL || BDK_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail (fg != NULL, NULL);
  g_return_val_if_fail (bg != NULL, NULL);
  g_return_val_if_fail ((drawable != NULL) || (depth != -1), NULL);
  g_return_val_if_fail ((width != 0) && (height != 0), NULL);

  if (!drawable)
    {
      BDK_NOTE (MULTIHEAD, g_message ("need to specify the screen parent window"
				      "for bdk_pixmap_create_from_data() to be multihead safe"));
      drawable = bdk_screen_get_root_window (bdk_screen_get_default ());
    }

  if (BDK_IS_WINDOW (drawable) && BDK_WINDOW_DESTROYED (drawable))
    return NULL;

  if (depth == -1)
    depth = bdk_drawable_get_visual (drawable)->depth;

  pixmap = g_object_new (bdk_pixmap_get_type (), NULL);
  draw_impl = BDK_DRAWABLE_IMPL_X11 (BDK_PIXMAP_OBJECT (pixmap)->impl);
  pix_impl = BDK_PIXMAP_IMPL_X11 (BDK_PIXMAP_OBJECT (pixmap)->impl);
  draw_impl->wrapper = BDK_DRAWABLE (pixmap);
  
  pix_impl->is_foreign = FALSE;
  pix_impl->width = width;
  pix_impl->height = height;
  BDK_PIXMAP_OBJECT (pixmap)->depth = depth;

  draw_impl->screen = BDK_DRAWABLE_SCREEN (drawable);
  draw_impl->xid = XCreatePixmapFromBitmapData (BDK_WINDOW_XDISPLAY (drawable),
                                                BDK_WINDOW_XID (drawable),
                                                (char *)data, width, height,
                                                fg->pixel, bg->pixel, depth);

  _bdk_xid_table_insert (BDK_WINDOW_DISPLAY (drawable),
			 &BDK_PIXMAP_XID (pixmap), pixmap);
  return pixmap;
}

/**
 * bdk_pixmap_foreign_new_for_display:
 * @display: The #BdkDisplay where @anid is located.
 * @anid: a native pixmap handle.
 * 
 * Wraps a native pixmap in a #BdkPixmap.
 * This may fail if the pixmap has been destroyed.
 *
 * For example in the X backend, a native pixmap handle is an Xlib
 * <type>XID</type>.
 *
 * Return value: the newly-created #BdkPixmap wrapper for the 
 *    native pixmap or %NULL if the pixmap has been destroyed.
 *
 * Since: 2.2
 **/
BdkPixmap *
bdk_pixmap_foreign_new_for_display (BdkDisplay      *display,
				    BdkNativeWindow  anid)
{
  Pixmap xpixmap;
  Window root_return;
  BdkScreen *screen;
  int x_ret, y_ret;
  unsigned int w_ret, h_ret, bw_ret, depth_ret;

  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);

  /* check to make sure we were passed something at
   * least a little sane */
  g_return_val_if_fail ((anid != 0), NULL);
  
  /* set the pixmap to the passed in value */
  xpixmap = anid;

  /* get information about the Pixmap to fill in the structure for
     the bdk window */
  if (!XGetGeometry (BDK_DISPLAY_XDISPLAY (display),
		     xpixmap, &root_return,
		     &x_ret, &y_ret, &w_ret, &h_ret, &bw_ret, &depth_ret))
    return NULL;
  
  screen = _bdk_x11_display_screen_for_xrootwin (display, root_return);
  return bdk_pixmap_foreign_new_for_screen (screen, anid, w_ret, h_ret, depth_ret);
}

/**
 * bdk_pixmap_foreign_new_for_screen:
 * @screen: a #BdkScreen
 * @anid: a native pixmap handle
 * @width: the width of the pixmap identified by @anid
 * @height: the height of the pixmap identified by @anid
 * @depth: the depth of the pixmap identified by @anid
 *
 * Wraps a native pixmap in a #BdkPixmap.
 * This may fail if the pixmap has been destroyed.
 *
 * For example in the X backend, a native pixmap handle is an Xlib
 * <type>XID</type>.
 *
 * This function is an alternative to bdk_pixmap_foreign_new_for_display()
 * for cases where the dimensions of the pixmap are known. For the X
 * backend, this avoids a roundtrip to the server.
 *
 * Return value: the newly-created #BdkPixmap wrapper for the 
 *    native pixmap or %NULL if the pixmap has been destroyed.
 * 
 * Since: 2.10
 */
BdkPixmap *
bdk_pixmap_foreign_new_for_screen (BdkScreen       *screen,
				   BdkNativeWindow  anid,
				   bint             width,
				   bint             height,
				   bint             depth)
{
  Pixmap xpixmap;
  BdkPixmap *pixmap;
  BdkDrawableImplX11 *draw_impl;
  BdkPixmapImplX11 *pix_impl;

  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);
  g_return_val_if_fail (anid != 0, NULL);
  g_return_val_if_fail (width > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);
  g_return_val_if_fail (depth > 0, NULL);

  pixmap = g_object_new (bdk_pixmap_get_type (), NULL);
  draw_impl = BDK_DRAWABLE_IMPL_X11 (BDK_PIXMAP_OBJECT (pixmap)->impl);
  pix_impl = BDK_PIXMAP_IMPL_X11 (BDK_PIXMAP_OBJECT (pixmap)->impl);
  draw_impl->wrapper = BDK_DRAWABLE (pixmap);

  xpixmap = anid;
  
  draw_impl->screen = screen;
  draw_impl->xid = xpixmap;

  pix_impl->is_foreign = TRUE;
  pix_impl->width = width;
  pix_impl->height = height;
  BDK_PIXMAP_OBJECT (pixmap)->depth = depth;
  
  _bdk_xid_table_insert (bdk_screen_get_display (screen), 
			 &BDK_PIXMAP_XID (pixmap), pixmap);

  return pixmap;
}

/**
 * bdk_pixmap_foreign_new:
 * @anid: a native pixmap handle.
 * 
 * Wraps a native window for the default display in a #BdkPixmap.
 * This may fail if the pixmap has been destroyed.
 *
 * For example in the X backend, a native pixmap handle is an Xlib
 * <type>XID</type>.
 *
 * Return value: the newly-created #BdkPixmap wrapper for the 
 *    native pixmap or %NULL if the pixmap has been destroyed.
 **/
BdkPixmap*
bdk_pixmap_foreign_new (BdkNativeWindow anid)
{
   return bdk_pixmap_foreign_new_for_display (bdk_display_get_default (), anid);
}

/**
 * bdk_pixmap_lookup:
 * @anid: a native pixmap handle.
 * 
 * Looks up the #BdkPixmap that wraps the given native pixmap handle.
 *
 * For example in the X backend, a native pixmap handle is an Xlib
 * <type>XID</type>.
 *
 * Return value: the #BdkPixmap wrapper for the native pixmap,
 *    or %NULL if there is none.
 **/
BdkPixmap*
bdk_pixmap_lookup (BdkNativeWindow anid)
{
  return (BdkPixmap*) bdk_xid_table_lookup_for_display (bdk_display_get_default (), anid);
}

/**
 * bdk_pixmap_lookup_for_display:
 * @display: the #BdkDisplay associated with @anid
 * @anid: a native pixmap handle.
 * 
 * Looks up the #BdkPixmap that wraps the given native pixmap handle.
 *
 * For example in the X backend, a native pixmap handle is an Xlib
 * <type>XID</type>.
 *
 * Return value: the #BdkPixmap wrapper for the native pixmap,
 *    or %NULL if there is none.
 *
 * Since: 2.2
 **/
BdkPixmap*
bdk_pixmap_lookup_for_display (BdkDisplay *display, BdkNativeWindow anid)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);
  return (BdkPixmap*) bdk_xid_table_lookup_for_display (display, anid);
}

#define __BDK_PIXMAP_X11_C__
#include  "bdkaliasdef.c"
