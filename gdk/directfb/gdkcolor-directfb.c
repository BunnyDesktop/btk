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
 * Copyright (C) 2002       convergence GmbH
 * Written by Denis Oliver Kropp <dok@convergence.de> and
 *            Sven Neumann <sven@convergence.de>
 */

#include "config.h"
#include "bdk.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bdkcolor.h"
#include "bdkinternals.h"
#include "bdkdirectfb.h"
#include "bdkprivate-directfb.h"
#include "bdkalias.h"


typedef struct {
  BdkColorInfo     *info;
  IDirectFBPalette *palette;
} BdkColormapPrivateDirectFB;


static void  bdk_colormap_finalize (GObject *object);

static gint  bdk_colormap_alloc_pseudocolors (BdkColormap *colormap,
                                              BdkColor    *colors,
                                              gint         ncolors,
                                              gboolean     writeable,
                                              gboolean     best_match,
                                              gboolean    *success);
static void  bdk_directfb_allocate_color_key (BdkColormap *colormap);


G_DEFINE_TYPE (BdkColormap, bdk_colormap, G_TYPE_OBJECT)

static void
bdk_colormap_init (BdkColormap *colormap)
{
  colormap->size           = 0;
  colormap->colors         = NULL;
  colormap->windowing_data = NULL;
}

static void
bdk_colormap_class_init (BdkColormapClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bdk_colormap_finalize;
}

static void
bdk_colormap_finalize (GObject *object)
{
  BdkColormap                *colormap = BDK_COLORMAP (object);
  BdkColormapPrivateDirectFB *private  = colormap->windowing_data;

  g_free (colormap->colors);

  if (private)
    {
      g_free (private->info);

      if (private->palette)
        private->palette->Release (private->palette);

      g_free (private);
      colormap->windowing_data = NULL;
    }

  G_OBJECT_CLASS (bdk_colormap_parent_class)->finalize (object);
}

BdkColormap*
bdk_colormap_new (BdkVisual *visual,
                  gboolean   private_cmap)
{
  BdkColormap *colormap;
  gint         i;

  g_return_val_if_fail (visual != NULL, NULL);

  colormap = g_object_new (bdk_colormap_get_type (), NULL);
  colormap->visual = visual;
  colormap->size   = visual->colormap_size;

  switch (visual->type)
    {
    case BDK_VISUAL_PSEUDO_COLOR:
      {
        IDirectFB                  *dfb = _bdk_display->directfb;
        IDirectFBPalette           *palette;
        BdkColormapPrivateDirectFB *private;
        DFBPaletteDescription       dsc;

        dsc.flags = DPDESC_SIZE;
        dsc.size  = colormap->size;
        if (!dfb->CreatePalette (dfb, &dsc, &palette))
          return NULL;

        colormap->colors = g_new0 (BdkColor, colormap->size);

        private = g_new0 (BdkColormapPrivateDirectFB, 1);
        private->info = g_new0 (BdkColorInfo, colormap->size);

	if (visual == bdk_visual_get_system ())
	  {
            /* save the first (transparent) palette entry */
            private->info[0].ref_count++;
          }

        private->palette = palette;

        colormap->windowing_data = private;

        bdk_directfb_allocate_color_key (colormap);
      }
      break;

    case BDK_VISUAL_STATIC_COLOR:
      colormap->colors = g_new0 (BdkColor, colormap->size);
      for (i = 0; i < colormap->size; i++)
        {
          BdkColor *color = colormap->colors + i;

          color->pixel = i;
          color->red   = (i & 0xE0) <<  8 | (i & 0xE0);
          color->green = (i & 0x1C) << 11 | (i & 0x1C) << 3;
          color->blue  = (i & 0x03) << 14 | (i & 0x03) << 6;
        }
      break;

    default:
      break;
    }

  return colormap;
}

BdkScreen*
bdk_colormap_get_screen (BdkColormap *cmap)
{
  return _bdk_screen;
}

BdkColormap*
bdk_screen_get_system_colormap (BdkScreen *screen)
{
  static BdkColormap *colormap = NULL;

  if (!colormap)
    {
      BdkVisual *visual = bdk_visual_get_system ();

      /* special case PSEUDO_COLOR to use the system palette */
      if (visual->type == BDK_VISUAL_PSEUDO_COLOR)
        {
          BdkColormapPrivateDirectFB *private;
          IDirectFBSurface           *surface;

          colormap = g_object_new (bdk_colormap_get_type (), NULL);

          colormap->visual = visual;
          colormap->size   = visual->colormap_size;
          colormap->colors = g_new0 (BdkColor, colormap->size);

          private = g_new0 (BdkColormapPrivateDirectFB, 1);
          private->info = g_new0 (BdkColorInfo, colormap->size);

          surface = BDK_WINDOW_IMPL_DIRECTFB (
                        BDK_WINDOW_OBJECT (_bdk_parent_root)->impl)->drawable.surface;
          surface->GetPalette (surface, &private->palette);

          colormap->windowing_data = private;

          /* save the first (transparent) palette entry */
          private->info[0].ref_count++;

          bdk_directfb_allocate_color_key (colormap);
        }
      else
        {
          colormap = bdk_colormap_new (visual, FALSE);
        }
    }

  return colormap;
}

gint
bdk_colormap_get_system_size (void)
{
  BdkVisual *visual;

  visual = bdk_visual_get_system ();

  return visual->colormap_size;
}

void
bdk_colormap_change (BdkColormap *colormap,
                     gint         ncolors)
{
  g_message ("bdk_colormap_change() is deprecated and unimplemented");
}

gboolean
bdk_colors_alloc (BdkColormap   *colormap,
                  gboolean       contiguous,
                  gulong        *planes,
                  gint           nplanes,
                  gulong        *pixels,
                  gint           npixels)
{
  /* g_message ("bdk_colors_alloc() is deprecated and unimplemented"); */

  return TRUE;  /* return TRUE here to make BdkRGB happy */
}

void
bdk_colors_free (BdkColormap *colormap,
                 gulong      *in_pixels,
                 gint         in_npixels,
                 gulong       planes)
{
  /* g_message ("bdk_colors_free() is deprecated and unimplemented"); */
}

void
bdk_colormap_free_colors (BdkColormap    *colormap,
                          const BdkColor *colors,
                          gint            ncolors)
{
  BdkColormapPrivateDirectFB *private;
  gint                        i;

  g_return_if_fail (BDK_IS_COLORMAP (colormap));
  g_return_if_fail (colors != NULL);

  private = colormap->windowing_data;
  if (!private)
    return;

  for (i = 0; i < ncolors; i++)
    {
      gint  index = colors[i].pixel;

      if (index < 0 || index >= colormap->size)
        continue;

      if (private->info[index].ref_count)
        private->info[index].ref_count--;
    }
}

gint
bdk_colormap_alloc_colors (BdkColormap *colormap,
                           BdkColor    *colors,
                           gint         ncolors,
                           gboolean     writeable,
                           gboolean     best_match,
                           gboolean    *success)
{
  BdkVisual *visual;
  gint       i;

  g_return_val_if_fail (BDK_IS_COLORMAP (colormap), 0);
  g_return_val_if_fail (colors != NULL, 0);
  g_return_val_if_fail (success != NULL, 0);

  switch (colormap->visual->type)
    {
    case BDK_VISUAL_TRUE_COLOR:
      visual = colormap->visual;

      for (i = 0; i < ncolors; i++)
        {
          colors[i].pixel =
            (((colors[i].red
               >> (16 - visual->red_prec))   << visual->red_shift)   +
             ((colors[i].green
               >> (16 - visual->green_prec)) << visual->green_shift) +
             ((colors[i].blue
               >> (16 - visual->blue_prec))  << visual->blue_shift));

          success[i] = TRUE;
        }
      break;

    case BDK_VISUAL_PSEUDO_COLOR:
      return bdk_colormap_alloc_pseudocolors (colormap,
                                              colors, ncolors,
                                              writeable, best_match,
                                              success);
      break;

    case BDK_VISUAL_STATIC_COLOR:
      for (i = 0; i < ncolors; i++)
        {
          colors[i].pixel = (((colors[i].red   & 0xE000) >> 8)  |
                             ((colors[i].green & 0xE000) >> 11) |
                             ((colors[i].blue  & 0xC000) >> 14));
          success[i] = TRUE;
        }
      break;

    default:
      for (i = 0; i < ncolors; i++)
        success[i] = FALSE;
      break;
    }

  return 0;
}

gboolean
bdk_color_change (BdkColormap *colormap,
                  BdkColor    *color)
{
  BdkColormapPrivateDirectFB *private;
  IDirectFBPalette           *palette;

  g_return_val_if_fail (BDK_IS_COLORMAP (colormap), FALSE);
  g_return_val_if_fail (color != NULL, FALSE);

  private = colormap->windowing_data;
  if (!private)
    return FALSE;

  palette = private->palette;
  if (!palette)
    return FALSE;

  if (color->pixel < 0 || color->pixel >= colormap->size)
    return FALSE;

  if (private->info[color->pixel].flags & BDK_COLOR_WRITEABLE)
    {
      DFBColor  entry = { 0xFF,
                          color->red   >> 8,
                          color->green >> 8,
                          color->blue  >> 8 };

      if (palette->SetEntries (palette, &entry, 1, color->pixel) != DFB_OK)
        return FALSE;

      colormap->colors[color->pixel] = *color;
      return TRUE;
    }

  return FALSE;
}

void
bdk_colormap_query_color (BdkColormap *colormap,
                          gulong       pixel,
                          BdkColor    *result)
{
  BdkVisual *visual;

  g_return_if_fail (BDK_IS_COLORMAP (colormap));

  visual = bdk_colormap_get_visual (colormap);

  switch (visual->type)
    {
    case BDK_VISUAL_TRUE_COLOR:
      result->red = 65535. *
        (gdouble)((pixel & visual->red_mask) >> visual->red_shift) /
        ((1 << visual->red_prec) - 1);

      result->green = 65535. *
        (gdouble)((pixel & visual->green_mask) >> visual->green_shift) /
        ((1 << visual->green_prec) - 1);

      result->blue = 65535. *
        (gdouble)((pixel & visual->blue_mask) >> visual->blue_shift) /
        ((1 << visual->blue_prec) - 1);
      break;

    case BDK_VISUAL_STATIC_COLOR:
    case BDK_VISUAL_PSEUDO_COLOR:
      if (pixel >= 0 && pixel < colormap->size)
        {
          result->red   = colormap->colors[pixel].red;
          result->green = colormap->colors[pixel].green;
          result->blue  = colormap->colors[pixel].blue;
        }
      else
        g_warning ("bdk_colormap_query_color: pixel outside colormap");
      break;

    case BDK_VISUAL_DIRECT_COLOR:
    case BDK_VISUAL_GRAYSCALE:
    case BDK_VISUAL_STATIC_GRAY:
      /* unsupported */
      g_assert_not_reached ();
      break;
    }
}

IDirectFBPalette *
bdk_directfb_colormap_get_palette (BdkColormap *colormap)
{
  BdkColormapPrivateDirectFB *private;

  g_return_val_if_fail (BDK_IS_COLORMAP (colormap), NULL);

  private = colormap->windowing_data;

  if (private && private->palette)
    return private->palette;
  else
    return NULL;
}

static gint
bdk_colormap_alloc_pseudocolors (BdkColormap *colormap,
                                 BdkColor    *colors,
                                 gint         ncolors,
                                 gboolean     writeable,
                                 gboolean     best_match,
                                 gboolean    *success)
{
  BdkColormapPrivateDirectFB *private = colormap->windowing_data;
  IDirectFBPalette           *palette;
  gint                        i, j;
  gint                        remaining = ncolors;

  palette = private->palette;

  for (i = 0; i < ncolors; i++)
    {
      guint     index;
      DFBColor  lookup = { 0xFF,
                           colors[i].red   >> 8,
                           colors[i].green >> 8,
                           colors[i].blue  >> 8 };

      success[i] = FALSE;

      if (writeable)
        {
          /* look for an empty slot and allocate a new color */
          for (j = 0; j < colormap->size; j++)
            if (private->info[j].ref_count == 0)
              {
                index = j;

                palette->SetEntries (palette, &lookup, 1, index);

                private->info[index].flags = BDK_COLOR_WRITEABLE;

                colors[i].pixel = index;
                colormap->colors[index] = colors[i];

                goto allocated;
              }
        }
      else
        {
          palette->FindBestMatch (palette,
                                  lookup.r, lookup.g, lookup.b, lookup.a,
                                  &index);

          if (index < 0 || index > colormap->size)
            continue;

          /* check if we have an exact (non-writeable) match */
          if (private->info[index].ref_count &&
              !(private->info[index].flags & BDK_COLOR_WRITEABLE))
            {
              DFBColor  entry;

              palette->GetEntries (palette, &entry, 1, index);

              if (entry.a == 0xFF &&
                  entry.r == lookup.r && entry.g == lookup.g && entry.b == lookup.b)
                {
                  colors[i].pixel = index;

                  goto allocated;
                }
            }

          /* look for an empty slot and allocate a new color */
          for (j = 0; j < colormap->size; j++)
            if (private->info[j].ref_count == 0)
              {
                index = j;

                palette->SetEntries (palette, &lookup, 1, index);
    		private->info[index].flags = 0;

                colors[i].pixel = index;
                colormap->colors[index] = colors[i];

                goto allocated;
              }

          /* if that failed, use the best match */
          if (best_match &&
              !(private->info[index].flags & BDK_COLOR_WRITEABLE))
            {
#if 0
               g_print ("best match for (%d %d %d)  ",
                       colormap->colors[index].red,
                       colormap->colors[index].green,
                       colormap->colors[index].blue);
#endif

              colors[i].pixel = index;

              goto allocated;
            }
        }

      /* if we got here, all attempts failed */
      continue;

    allocated:
      private->info[index].ref_count++;

#if 0
      g_print ("cmap %p: allocated (%d %d %d) %d [%d]\n", colormap,
                colors[i].red, colors[i].green, colors[i].blue, colors[i].pixel,
	        private->info[index].ref_count);
#endif

      success[i] = TRUE;
      remaining--;
    }

  return remaining;
}

/* dirty hack for color_keying */
static void
bdk_directfb_allocate_color_key (BdkColormap *colormap)
{
  BdkColormapPrivateDirectFB *private = colormap->windowing_data;
  IDirectFBPalette           *palette = private->palette;

  if (!bdk_directfb_enable_color_keying)
    return;

  palette->SetEntries (palette, &bdk_directfb_bg_color, 1, 255);

  colormap->colors[255].pixel = 255;
  colormap->colors[255].red   = ((bdk_directfb_bg_color_key.r << 8)
                                 | bdk_directfb_bg_color_key.r);
  colormap->colors[255].green = ((bdk_directfb_bg_color_key.g << 8)
                                 | bdk_directfb_bg_color_key.g);
  colormap->colors[255].blue  = ((bdk_directfb_bg_color_key.b << 8)
                                 | bdk_directfb_bg_color_key.b);

  private->info[255].ref_count++;
}
#define __BDK_COLOR_X11_C__
#include "bdkaliasdef.c"
