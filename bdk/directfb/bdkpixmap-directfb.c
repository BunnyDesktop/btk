/* BDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998-1999 Tor Lillqvist
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
 * file for a list of people on the BTK+ Team.
 */

/*
 * BTK+ DirectFB backend
 * Copyright (C) 2001-2002  convergence integrated media GmbH
 * Copyright (C) 2002-2004  convergence GmbH
 * Written by Denis Oliver Kropp <dok@convergence.de> and
 *            Sven Neumann <sven@convergence.de>
 */

#include "config.h"
#include "bdk.h"

#include <stdlib.h>
#include <string.h>

#include "bdkdirectfb.h"
#include "bdkprivate-directfb.h"

#include "bdkinternals.h"

#include "bdkpixmap.h"
#include "bdkalias.h"


static void bdk_pixmap_impl_directfb_init       (BdkPixmapImplDirectFB      *pixmap);
static void bdk_pixmap_impl_directfb_class_init (BdkPixmapImplDirectFBClass *klass);
static void bdk_pixmap_impl_directfb_finalize   (GObject                    *object);


static gpointer parent_class = NULL;

G_DEFINE_TYPE (BdkPixmapImplDirectFB,
               bdk_pixmap_impl_directfb,
               BDK_TYPE_DRAWABLE_IMPL_DIRECTFB);

GType
_bdk_pixmap_impl_get_type (void)
{
  return bdk_pixmap_impl_directfb_get_type ();
}

static void
bdk_pixmap_impl_directfb_init (BdkPixmapImplDirectFB *impl)
{
  BdkDrawableImplDirectFB *draw_impl = BDK_DRAWABLE_IMPL_DIRECTFB (impl);
  draw_impl->width  = 1;
  draw_impl->height = 1;
}

static void
bdk_pixmap_impl_directfb_class_init (BdkPixmapImplDirectFBClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = bdk_pixmap_impl_directfb_finalize;
}

static void
bdk_pixmap_impl_directfb_finalize (GObject *object)
{
  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (object);
}

BdkPixmap *
_bdk_pixmap_new (BdkDrawable *drawable,
                 gint       width,
                 gint       height,
                 gint       depth)
{
  DFBSurfacePixelFormat    format;
  IDirectFBSurface        *surface;
  BdkPixmap               *pixmap;
  BdkDrawableImplDirectFB *draw_impl;

  g_return_val_if_fail (drawable == NULL || BDK_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (drawable != NULL || depth != -1, NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);

  if (!drawable)
    drawable = _bdk_parent_root;

  if (BDK_IS_WINDOW (drawable) && BDK_WINDOW_DESTROYED (drawable))
    return NULL;

  BDK_NOTE (MISC, g_print ("bdk_pixmap_new: %dx%dx%d\n",
                           width, height, depth));

  if (depth == -1)
    depth = bdk_drawable_get_depth (BDK_DRAWABLE (drawable));

  switch (depth)
    {
    case  1:
      format = DSPF_A8;
      break;
    case  8:
      format = DSPF_LUT8;
      break;
    case 15:
      format = DSPF_ARGB1555;
      break;
    case 16:
      format = DSPF_RGB16;
      break;
    case 24:
      format = DSPF_RGB24;
      break;
    case 32:
      format = DSPF_RGB32;
      break;
    default:
      g_message ("unimplemented %s for depth %d", G_STRFUNC, depth);
      return NULL;
    }

  if (!(surface =
	bdk_display_dfb_create_surface (_bdk_display, format, width, height))) {
    g_assert (surface != NULL);
    return NULL;
  }

  pixmap = g_object_new (bdk_pixmap_get_type (), NULL);
  draw_impl = BDK_DRAWABLE_IMPL_DIRECTFB (BDK_PIXMAP_OBJECT (pixmap)->impl);
  draw_impl->surface = surface;
  surface->Clear (surface, 0x0, 0x0, 0x0, 0x0);
  surface->GetSize (surface, &draw_impl->width, &draw_impl->height);
  surface->GetPixelFormat (surface, &draw_impl->format);

  draw_impl->abs_x = draw_impl->abs_y = 0;

  BDK_PIXMAP_OBJECT (pixmap)->depth = depth;

  return pixmap;
}

BdkPixmap *
_bdk_bitmap_create_from_data (BdkDrawable *drawable,
                              const gchar *data,
                              gint         width,
                              gint         height)
{
  BdkPixmap *pixmap;

  g_return_val_if_fail (drawable == NULL || BDK_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);

  BDK_NOTE (MISC, g_print ("bdk_bitmap_create_from_data: %dx%d\n",
                           width, height));

  pixmap = bdk_pixmap_new (drawable, width, height, 1);

#define GET_PIXEL(data,pixel)                                           \
  ((data[(pixel / 8)] & (0x1 << ((pixel) % 8))) >> ((pixel) % 8))

  if (pixmap)
    {
      guchar *dst;
      gint    pitch;

      IDirectFBSurface *surface;

      surface = BDK_DRAWABLE_IMPL_DIRECTFB (BDK_PIXMAP_OBJECT (pixmap)->impl)->surface;

      if (surface->Lock (surface, DSLF_WRITE, (void**)(&dst), &pitch) == DFB_OK)
        {
          gint i, j;

          for (i = 0; i < height; i++)
            {
	      for (j = 0; j < width; j++)
		{
		  dst[j] = GET_PIXEL (data, j) * 255;
		}

              data += (width + 7) / 8;
	      dst += pitch;
            }

          surface->Unlock (surface);
        }
    }

#undef GET_PIXEL

  return pixmap;
}

BdkPixmap *
_bdk_pixmap_create_from_data (BdkDrawable    *drawable,
                              const gchar    *data,
                              gint            width,
                              gint            height,
                              gint            depth,
                              const BdkColor *fg,
                              const BdkColor *bg)
{
  BdkPixmap *pixmap;

  g_return_val_if_fail (drawable == NULL || BDK_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail (drawable != NULL || depth > 0, NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);

  BDK_NOTE (MISC, g_print ("bdk_pixmap_create_from_data: %dx%dx%d\n",
                           width, height, depth));

  pixmap = bdk_pixmap_new (drawable, width, height, depth);

  if (pixmap)
    {
      IDirectFBSurface *surface;
      gchar            *dst;
      gint              pitch;
      gint              src_pitch;

      depth = bdk_drawable_get_depth (pixmap);
      src_pitch = width * ((depth + 7) / 8);

      surface = BDK_DRAWABLE_IMPL_DIRECTFB (BDK_PIXMAP_OBJECT (pixmap)->impl)->surface;

      if (surface->Lock (surface,
                         DSLF_WRITE, (void**)(&dst), &pitch) == DFB_OK)
        {
          gint i;

          for (i = 0; i < height; i++)
            {
              memcpy (dst, data, src_pitch);
              dst += pitch;
              data += src_pitch;
            }

          surface->Unlock (surface);
        }
    }

  return pixmap;
}

BdkPixmap *
bdk_pixmap_foreign_new (BdkNativeWindow anid)
{
  g_warning (" bdk_pixmap_foreign_new unsuporrted \n");
  return NULL;
}

BdkPixmap *
bdk_pixmap_foreign_new_for_display (BdkDisplay *display, BdkNativeWindow anid)
{
  return bdk_pixmap_foreign_new (anid);
}

BdkPixmap *
bdk_pixmap_foreign_new_for_screen (BdkScreen       *screen,
                                   BdkNativeWindow  anid,
                                   gint             width,
                                   gint             height,
                                   gint             depth)
{
  /*Use the root drawable for now since only one screen */
  return bdk_pixmap_new (NULL, width, height, depth);
}


BdkPixmap *
bdk_pixmap_lookup (BdkNativeWindow anid)
{
  g_warning (" bdk_pixmap_lookup unsuporrted \n");
  return NULL;
}

BdkPixmap *
bdk_pixmap_lookup_for_display (BdkDisplay *display, BdkNativeWindow anid)
{
  return bdk_pixmap_lookup (anid);
}

#define __BDK_PIXMAP_X11_C__
#include "bdkaliasdef.c"
