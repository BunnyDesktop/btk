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


#include "bdkdirectfb.h"
#include "bdkprivate-directfb.h"

#include "bdkinternals.h"

#include "bdkimage.h"
#include "bdkalias.h"


static GList    *image_list   = NULL;
static bpointer  parent_class = NULL;

static void bdk_directfb_image_destroy (BdkImage      *image);
static void bdk_image_init             (BdkImage      *image);
static void bdk_image_class_init       (BdkImageClass *klass);
static void bdk_image_finalize         (BObject       *object);

G_DEFINE_TYPE (BdkImage, bdk_image, B_TYPE_OBJECT)

static void
bdk_image_init (BdkImage *image)
{
  image->windowing_data = g_new0 (BdkImageDirectFB, 1);
  image->mem = NULL;

  image_list = g_list_prepend (image_list, image);
}

static void
bdk_image_class_init (BdkImageClass *klass)
{
  BObjectClass *object_class = B_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = bdk_image_finalize;
}

static void
bdk_image_finalize (BObject *object)
{
  BdkImage *image;

  image = BDK_IMAGE (object);

  image_list = g_list_remove (image_list, image);

  if (image->depth == 1)
    g_free (image->mem);

  bdk_directfb_image_destroy (image);

  if (B_OBJECT_CLASS (parent_class)->finalize)
    B_OBJECT_CLASS (parent_class)->finalize (object);
}


/* this function is called from the atexit handler! */
void
_bdk_image_exit (void)
{
  BObject *image;

  while (image_list)
    {
      image = image_list->data;

      bdk_image_finalize (image);
    }
}

BdkImage *
bdk_image_new_bitmap (BdkVisual *visual,
                      bpointer   data,
                      bint       w,
                      bint       h)
{
  BdkImage         *image;
  BdkImageDirectFB *private;

  image = g_object_new (bdk_image_get_type (), NULL);
  private = image->windowing_data;

  image->type   = BDK_IMAGE_SHARED;
  image->visual = visual;
  image->width  = w;
  image->height = h;
  image->depth  = 1;

  BDK_NOTE (MISC, g_print ("bdk_image_new_bitmap: %dx%d\n", w, h));

  g_message ("not fully implemented %s", B_STRFUNC);

  image->bpl = (w + 7) / 8;
  image->mem = g_malloc (image->bpl * h);
#if G_BYTE_ORDER == G_BIG_ENDIAN
  image->byte_order = BDK_MSB_FIRST;
#else
  image->byte_order = BDK_LSB_FIRST;
#endif
  image->bpp = 1;

  return image;
}

void
_bdk_windowing_image_init (void)
{
}

BdkImage*
_bdk_image_new_for_depth (BdkScreen    *screen,
                          BdkImageType  type,
                          BdkVisual    *visual,
                          bint          width,
                          bint          height,
                          bint          depth)
{
  BdkImage              *image;
  BdkImageDirectFB      *private;
  DFBResult              ret;
  bint                   pitch;
  DFBSurfacePixelFormat  format;
  IDirectFBSurface      *surface;

  if (type == BDK_IMAGE_FASTEST || type == BDK_IMAGE_NORMAL)
    type = BDK_IMAGE_SHARED;

  if (visual)
    depth = visual->depth;

  switch (depth)
    {
    case 8:
      format = DSPF_LUT8;
      break;
    case 15:
      format = DSPF_ARGB1555;
      break;
    case 16:
      format = DSPF_RGB16;
      break;
    case 24:
      format = DSPF_RGB32;
      break;
    case 32:
      format = DSPF_ARGB;
      break;
    default:
      g_message ("unimplemented %s for depth %d", B_STRFUNC, depth);
      return NULL;
    }

  surface = bdk_display_dfb_create_surface (_bdk_display, format,
                                            width, height);
  if (!surface)
    {
      return NULL;
    }
  surface->GetPixelFormat (surface, &format);

  image = g_object_new (bdk_image_get_type (), NULL);
  private = image->windowing_data;

  private->surface = surface;

  ret = surface->Lock (surface, DSLF_WRITE, &image->mem, &pitch);
  if (ret)
    {
      DirectFBError ("IDirectFBSurface::Lock() for writing failed!\n", ret);
      g_object_unref (image);
      return NULL;
    }

  image->type           = type;
  image->visual         = visual;
#if G_BYTE_ORDER == G_BIG_ENDIAN
  image->byte_order	= BDK_MSB_FIRST;
#else
  image->byte_order 	= BDK_LSB_FIRST;
#endif
  image->width          = width;
  image->height         = height;
  image->depth          = depth;
  image->bpp            = DFB_BYTES_PER_PIXEL (format);
  image->bpl            = pitch;
  image->bits_per_pixel = DFB_BITS_PER_PIXEL (format);

  image_list = g_list_prepend (image_list, image);

  return image;
}


BdkImage*
_bdk_directfb_copy_to_image (BdkDrawable *drawable,
                             BdkImage    *image,
                             bint         src_x,
                             bint         src_y,
                             bint         dest_x,
                             bint         dest_y,
                             bint         width,
                             bint         height)
{
  BdkDrawableImplDirectFB *impl;
  BdkImageDirectFB        *private;
  int                      pitch;
  DFBRectangle             rect  = { src_x, src_y, width, height };
  IDirectFBDisplayLayer   *layer = _bdk_display->layer;

  g_return_val_if_fail (BDK_IS_DRAWABLE_IMPL_DIRECTFB (drawable), NULL);
  g_return_val_if_fail (image != NULL || (dest_x == 0 && dest_y == 0), NULL);

  impl = BDK_DRAWABLE_IMPL_DIRECTFB (drawable);

  if (impl->wrapper == _bdk_parent_root)
    {
      DFBResult ret;

      ret = layer->SetCooperativeLevel (layer, DLSCL_ADMINISTRATIVE);
      if (ret)
        {
          DirectFBError ("_bdk_directfb_copy_to_image - SetCooperativeLevel",
                         ret);
          return NULL;
        }

      ret = layer->GetSurface (layer, &impl->surface);
      if (ret)
        {
          layer->SetCooperativeLevel (layer, DLSCL_SHARED);
          DirectFBError ("_bdk_directfb_copy_to_image - GetSurface", ret);
          return NULL;
        }
    }

  if (!impl->surface)
    return NULL;

  if (!image)
    image =  bdk_image_new (BDK_IMAGE_NORMAL,
                            bdk_drawable_get_visual (drawable), width, height);

  private = image->windowing_data;

  private->surface->Unlock (private->surface);

  private->surface->Blit (private->surface,
                          impl->surface, &rect, dest_x, dest_y);

  private->surface->Lock (private->surface,
                          DSLF_READ | DSLF_WRITE,
                          &image->mem, &pitch);
  image->bpl = pitch;

  if (impl->wrapper == _bdk_parent_root)
    {
      impl->surface->Release (impl->surface);
      impl->surface = NULL;
      layer->SetCooperativeLevel (layer, DLSCL_SHARED);
    }

  return image;
}

buint32
bdk_image_get_pixel (BdkImage *image,
                     bint      x,
                     bint      y)
{
  buint32 pixel = 0;

  g_return_val_if_fail (BDK_IS_IMAGE (image), 0);

  if (!(x >= 0 && x < image->width && y >= 0 && y < image->height))
    return 0;

  if (image->depth == 1)
    pixel = (((buchar *) image->mem)[y * image->bpl + (x >> 3)] & (1 << (7 - (x & 0x7)))) != 0;
  else
    {
      buchar *pixelp = (buchar *) image->mem + y * image->bpl + x * image->bpp;

      switch (image->bpp)
        {
        case 1:
          pixel = *pixelp;
          break;

        case 2:
          pixel = pixelp[0] | (pixelp[1] << 8);
          break;

        case 3:
          pixel = pixelp[0] | (pixelp[1] << 8) | (pixelp[2] << 16);
          break;

        case 4:
          pixel = pixelp[0] | (pixelp[1] << 8) | (pixelp[2] << 16);
          break;
        }
    }

  return pixel;
}

void
bdk_image_put_pixel (BdkImage *image,
                     bint       x,
                     bint       y,
                     buint32    pixel)
{
  g_return_if_fail (image != NULL);

  if (!(x >= 0 && x < image->width && y >= 0 && y < image->height))
    return;

  if (image->depth == 1)
    if (pixel & 1)
      ((buchar *) image->mem)[y * image->bpl + (x >> 3)] |= (1 << (7 - (x & 0x7)));
    else
      ((buchar *) image->mem)[y * image->bpl + (x >> 3)] &= ~(1 << (7 - (x & 0x7)));
  else
    {
      buchar *pixelp = (buchar *) image->mem + y * image->bpl + x * image->bpp;

      switch (image->bpp)
        {
        case 4:
          pixelp[3] = 0xFF;
        case 3:
          pixelp[2] = ((pixel >> 16) & 0xFF);
        case 2:
          pixelp[1] = ((pixel >> 8) & 0xFF);
        case 1:
          pixelp[0] = (pixel & 0xFF);
        }
    }
}

static void
bdk_directfb_image_destroy (BdkImage *image)
{
  BdkImageDirectFB *private;

  g_return_if_fail (BDK_IS_IMAGE (image));

  private = image->windowing_data;

  if (!private)
    return;

  BDK_NOTE (MISC, g_print ("bdk_directfb_image_destroy: %#lx\n",
                           (bulong) private->surface));

  private->surface->Unlock (private->surface);
  private->surface->Release (private->surface);

  g_free (private);
  image->windowing_data = NULL;
}

bint
_bdk_windowing_get_bits_for_depth (BdkDisplay *display,
                                   bint        depth)
{
  switch (depth)
    {
    case 1:
    case 8:
      return 8;
    case 15:
    case 16:
      return 16;
    case 24:
    case 32:
      return 32;
    }

  return 0;
}

#define __BDK_IMAGE_X11_C__
#include "bdkaliasdef.c"
