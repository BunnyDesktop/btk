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

#include "bdkdirectfb.h"
#include "bdkprivate-directfb.h"

#include "bdkscreen.h"
#include "bdkvisual.h"
#include "bdkalias.h"


struct _BdkVisualClass
{
  BObjectClass parent_class;
};


static void                bdk_visual_decompose_mask  (bulong   mask,
                                                       bint    *shift,
                                                       bint    *prec);
static BdkVisualDirectFB * bdk_directfb_visual_create (DFBSurfacePixelFormat  pixelformat);


static DFBSurfacePixelFormat formats[] =
  {
    DSPF_ARGB,
    DSPF_LUT8,
    DSPF_RGB32,
    DSPF_RGB24,
    DSPF_RGB16,
    DSPF_ARGB1555,
    DSPF_RGB332
  };

BdkVisual                * system_visual                                = NULL;
static BdkVisualDirectFB * visuals[G_N_ELEMENTS (formats) + 1]          = { NULL };
static bint                available_depths[G_N_ELEMENTS (formats) + 1] = {0};
static BdkVisualType       available_types[G_N_ELEMENTS (formats) + 1]  = {0};


static void
bdk_visual_finalize (BObject *object)
{
  g_error ("A BdkVisual object was finalized. This should not happen");
}

static void
bdk_visual_class_init (BObjectClass *class)
{
  class->finalize = bdk_visual_finalize;
}

GType
bdk_visual_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      const GTypeInfo object_info =
        {
          sizeof (BdkVisualClass),
          (GBaseInitFunc) NULL,
          (GBaseFinalizeFunc) NULL,
          (GClassInitFunc) bdk_visual_class_init,
          NULL,           /* class_finalize */
          NULL,           /* class_data */
          sizeof (BdkVisualDirectFB),
          0,              /* n_preallocs */
          (GInstanceInitFunc) NULL,
        };

      object_type = g_type_register_static (B_TYPE_OBJECT,
                                            "BdkVisual",
                                            &object_info, 0);
    }

  return object_type;
}

void
_bdk_visual_init (void)
{
  DFBDisplayLayerConfig  dlc;
  DFBSurfaceDescription  desc;
  IDirectFBSurface      *dest;
  bint                   i, c;


  _bdk_display->layer->GetConfiguration (_bdk_display->layer, &dlc);
  g_assert (dlc.pixelformat != DSPF_UNKNOWN);

  dest = bdk_display_dfb_create_surface (_bdk_display, dlc.pixelformat, 8, 8);
  g_assert (dest != NULL);

  /* We could provide all visuals since DirectFB allows us to mix
     surface formats. Blitting with format conversion can however
     be incredibly slow, so we've choosen to register only those
     visuals that can be blitted to the display layer in hardware.

     If you want to use a special pixelformat that is not registered
     here, you can create it using the DirectFB-specific function
     bdk_directfb_visual_by_format().
     Note:
     changed to do all formats but we should redo this code
     to ensure the base format ARGB LUT8 RGB etc then add ones supported
     by the hardware
  */
  for (i = 0; i < G_N_ELEMENTS (formats); i++)
    {
      IDirectFBSurface    *src;
      DFBAccelerationMask  acc;

      desc.flags = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
      desc.width       = 8;
      desc.height      = 8;
      desc.pixelformat = formats[i];
      //call direct so fail silently  is ok
      if (_bdk_display->directfb->CreateSurface (_bdk_display->directfb,
                                                 &desc, &src) != DFB_OK)
        continue;

      visuals[i] = bdk_directfb_visual_create (formats[i]);

      dest->GetAccelerationMask (dest, src, &acc);

      if (acc & DFXL_BLIT || formats[i] == dlc.pixelformat)
        {
          system_visual = BDK_VISUAL (visuals[i]);
        }

      src->Release (src);
    }

  dest->Release (dest);

  //fallback to ARGB must be supported
  if (!system_visual)
    {
      g_assert (visuals[DSPF_ARGB] != NULL);
      system_visual = BDK_VISUAL(visuals[DSPF_ARGB]);
    }

  g_assert (system_visual != NULL);
}

bint
bdk_visual_get_best_depth (void)
{
  return system_visual->depth;
}

BdkVisualType
bdk_visual_get_best_type (void)
{
  return system_visual->type;
}

BdkVisual *
bdk_screen_get_system_visual (BdkScreen *screen)
{
  g_assert (system_visual);
  return system_visual;
}

BdkVisual *
bdk_visual_get_best (void)
{
  return system_visual;
}

BdkVisual *
bdk_visual_get_best_with_depth (bint depth)
{
  bint i;

  for (i = 0; visuals[i]; i++)
    {
      if (visuals[i]) {
        BdkVisual *visual = BDK_VISUAL (visuals[i]);

        if (depth == visual->depth)
          return visual;
      }
    }

  return NULL;
}

BdkVisual *
bdk_visual_get_best_with_type (BdkVisualType visual_type)
{
  bint i;

  for (i = 0; visuals[i]; i++)
    {
      if (visuals[i]) {
        BdkVisual *visual = BDK_VISUAL (visuals[i]);

        if (visual_type == visual->type)
          return visual;
      }
    }

  return NULL;
}

BdkVisual*
bdk_visual_get_best_with_both (bint          depth,
                               BdkVisualType visual_type)
{
  bint i;

  for (i = 0; visuals[i]; i++)
    {
      if (visuals[i]) {
        BdkVisual *visual = BDK_VISUAL (visuals[i]);

        if (depth == visual->depth && visual_type == visual->type)
          return visual;
      }
    }

  return system_visual;
}

void
bdk_query_depths (bint **depths,
                  bint  *count)
{
  bint i;

  for (i = 0; available_depths[i]; i++)
    ;

  *count = i;
  *depths = available_depths;
}

void
bdk_query_visual_types (BdkVisualType **visual_types,
                        bint           *count)
{
  bint i;

  for (i = 0; available_types[i]; i++)
    ;

  *count = i;
  *visual_types = available_types;
}

GList *
bdk_screen_list_visuals (BdkScreen *screen)
{
  GList *list = NULL;
  bint   i;

  for (i = 0; visuals[i]; i++)
    if (visuals[i]) {
      BdkVisual * vis = BDK_VISUAL (visuals[i]);
      list = g_list_append (list,vis);
    }

  return list;
}

/**
 * bdk_directfb_visual_by_format:
 * @pixel_format: the pixel_format of the requested visual
 *
 * This function is specific to the DirectFB backend. It allows
 * to specify a BdkVisual by @pixel_format.
 *
 * At startup, only those visuals that can be blitted
 * hardware-accelerated are registered.  By using
 * bdk_directfb_visual_by_format() you can retrieve visuals that
 * don't match this criteria since this function will try to create
 * a new visual for the desired @pixel_format for you.
 *
 * Return value: a pointer to the BdkVisual or %NULL if the
 * pixel_format is unsupported.
 **/
BdkVisual *
bdk_directfb_visual_by_format (DFBSurfacePixelFormat pixel_format)
{
  bint i;

  /* first check if one the registered visuals matches */
  for (i = 0; visuals[i]; i++)
    if (visuals[i] && visuals[i]->format == pixel_format)
      return BDK_VISUAL (visuals[i]);

  /* none matched, try to create a new one for this pixel_format */
  {
    DFBSurfaceDescription  desc;
    IDirectFBSurface      *test;

    desc.flags       = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
    desc.width       = 8;
    desc.height      = 8;
    desc.pixelformat = pixel_format;

    if (_bdk_display->directfb->CreateSurface (_bdk_display->directfb,
                                               &desc, &test) != DFB_OK)
      return NULL;

    test->Release (test);
  }

  return BDK_VISUAL (bdk_directfb_visual_create (pixel_format));
}

BdkScreen *
bdk_visual_get_screen (BdkVisual *visual)
{
  g_return_val_if_fail (BDK_IS_VISUAL (visual), NULL);

  return bdk_screen_get_default ();
}

static void
bdk_visual_decompose_mask (bulong  mask,
                           bint   *shift,
                           bint   *prec)
{
  *shift = 0;
  *prec  = 0;

  while (!(mask & 0x1))
    {
      (*shift)++;
      mask >>= 1;
    }

  while (mask & 0x1)
    {
      (*prec)++;
      mask >>= 1;
    }
}

static BdkVisualDirectFB *
bdk_directfb_visual_create (DFBSurfacePixelFormat  pixelformat)
{
  BdkVisual *visual;
  bint       i;

  for (i = 0; i < G_N_ELEMENTS (formats); i++)
    if (formats[i] == pixelformat)
      break;

  if (i ==  G_N_ELEMENTS (formats))
    {
      g_warning ("unsupported pixelformat");
      return NULL;
    }

  visual = g_object_new (BDK_TYPE_VISUAL, NULL);

  switch (pixelformat)
    {
    case DSPF_LUT8:
      visual->type         = BDK_VISUAL_PSEUDO_COLOR;
      visual->bits_per_rgb = 8;
      break;

    case DSPF_RGB332:
      visual->type         = BDK_VISUAL_STATIC_COLOR;
      visual->bits_per_rgb = 3;
      break;

    case DSPF_ARGB1555:
      visual->type         = BDK_VISUAL_TRUE_COLOR;
      visual->red_mask     = 0x00007C00;
      visual->green_mask   = 0x000003E0;
      visual->blue_mask    = 0x0000001F;
      visual->bits_per_rgb = 5;
      break;

    case DSPF_RGB16:
      visual->type         = BDK_VISUAL_TRUE_COLOR;
      visual->red_mask     = 0x0000F800;
      visual->green_mask   = 0x000007E0;
      visual->blue_mask    = 0x0000001F;
      visual->bits_per_rgb = 6;
      break;

    case DSPF_RGB24:
    case DSPF_RGB32:
    case DSPF_ARGB:
      visual->type         = BDK_VISUAL_TRUE_COLOR;
      visual->red_mask     = 0x00FF0000;
      visual->green_mask   = 0x0000FF00;
      visual->blue_mask    = 0x000000FF;
      visual->bits_per_rgb = 8;
      break;

    default:
      g_assert_not_reached ();
    }

#if G_BYTE_ORDER == G_BIG_ENDIAN
  visual->byte_order = BDK_MSB_FIRST;
#else
  visual->byte_order = BDK_LSB_FIRST;
#endif

  visual->depth      = DFB_BITS_PER_PIXEL (pixelformat);

  switch (visual->type)
    {
    case BDK_VISUAL_TRUE_COLOR:
      bdk_visual_decompose_mask (visual->red_mask,
                                 &visual->red_shift, &visual->red_prec);
      bdk_visual_decompose_mask (visual->green_mask,
                                 &visual->green_shift, &visual->green_prec);
      bdk_visual_decompose_mask (visual->blue_mask,
                                 &visual->blue_shift, &visual->blue_prec);

      /* the number of possible levels per color component */
      visual->colormap_size = 1 << MAX (visual->red_prec,
                                        MAX (visual->green_prec,
                                             visual->blue_prec));
      break;

    case BDK_VISUAL_STATIC_COLOR:
    case BDK_VISUAL_PSEUDO_COLOR:
      visual->colormap_size = 1 << visual->depth;

      visual->red_mask    = 0;
      visual->red_shift   = 0;
      visual->red_prec    = 0;

      visual->green_mask  = 0;
      visual->green_shift = 0;
      visual->green_prec  = 0;

      visual->blue_mask   = 0;
      visual->blue_shift  = 0;
      visual->blue_prec   = 0;

      break;

    default:
      g_assert_not_reached ();
    }

  ((BdkVisualDirectFB *)visual)->format = pixelformat;

  for (i = 0; available_depths[i]; i++)
    if (available_depths[i] == visual->depth)
      break;
  if (!available_depths[i])
    available_depths[i] = visual->depth;

  for (i = 0; available_types[i]; i++)
    if (available_types[i] == visual->type)
      break;
  if (!available_types[i])
    available_types[i] = visual->type;

  return (BdkVisualDirectFB *) visual;
}

#define __BDK_VISUAL_X11_C__
#include "bdkaliasdef.c"
