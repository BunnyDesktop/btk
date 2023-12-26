/* bdkcolor-quartz.c
 *
 * Copyright (C) 2005 Imendio AB
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

#include "config.h"

#include "bdkcolor.h"
#include "bdkprivate-quartz.h"

GType
bdk_colormap_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      const GTypeInfo object_info =
      {
        sizeof (BdkColormapClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) NULL,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (BdkColormap),
        0,              /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };
      
      object_type = g_type_register_static (G_TYPE_OBJECT,
                                            "BdkColormap",
                                            &object_info,
					    0);
    }
  
  return object_type;
}

BdkColormap *
bdk_colormap_new (BdkVisual *visual,
		  gint       private_cmap)
{
  g_return_val_if_fail (visual != NULL, NULL);

  /* FIXME: Implement */
  return NULL;
}

BdkColormap *
bdk_screen_get_system_colormap (BdkScreen *screen)
{
  static BdkColormap *colormap = NULL;

  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);

  if (!colormap)
    {
      colormap = g_object_new (BDK_TYPE_COLORMAP, NULL);

      colormap->visual = bdk_visual_get_system ();
      colormap->size = colormap->visual->colormap_size;
    }

  return colormap;
}


BdkColormap *
bdk_screen_get_rgba_colormap (BdkScreen *screen)
{
  static BdkColormap *colormap = NULL;

  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);

  if (!colormap)
    {
      colormap = g_object_new (BDK_TYPE_COLORMAP, NULL);

      colormap->visual = bdk_screen_get_rgba_visual (screen);
      colormap->size = colormap->visual->colormap_size;
    }

  return colormap;
}

gint
bdk_colormap_get_system_size (void)
{
  /* FIXME: Implement */
  return 0;
}

void
bdk_colormap_change (BdkColormap *colormap,
		     gint         ncolors)
{
  /* FIXME: Implement */
}

gboolean
bdk_colors_alloc (BdkColormap   *colormap,
		  gboolean       contiguous,
		  gulong        *planes,
		  gint           nplanes,
		  gulong        *pixels,
		  gint           npixels)
{
  return TRUE;
}

void
bdk_colors_free (BdkColormap *colormap,
		 gulong      *pixels,
		 gint         npixels,
		 gulong       planes)
{
}

void
bdk_colormap_free_colors (BdkColormap    *colormap,
                          const BdkColor *colors,
                          gint            n_colors)
{
  /* This function shouldn't do anything since colors are never allocated. */
}

gint
bdk_colormap_alloc_colors (BdkColormap *colormap,
			   BdkColor    *colors,
			   gint         ncolors,
			   gboolean     writeable,
			   gboolean     best_match,
			   gboolean    *success)
{
  int i;
  int alpha;

  g_return_val_if_fail (BDK_IS_COLORMAP (colormap), ncolors);
  g_return_val_if_fail (colors != NULL, ncolors);
  g_return_val_if_fail (success != NULL, ncolors);

  if (bdk_colormap_get_visual (colormap)->depth == 32)
    alpha = 0xff;
  else
    alpha = 0;

  for (i = 0; i < ncolors; i++)
    {
      colors[i].pixel = alpha << 24 |
        ((colors[i].red >> 8) & 0xff) << 16 |
        ((colors[i].green >> 8) & 0xff) << 8 |
        ((colors[i].blue >> 8) & 0xff);
    }

  *success = TRUE;

  return 0;
}

void
bdk_colormap_query_color (BdkColormap *colormap,
			  gulong       pixel,
			  BdkColor    *result)
{
  result->red = pixel >> 16 & 0xff;
  result->red += result->red << 8;

  result->green = pixel >> 8 & 0xff;
  result->green += result->green << 8;

  result->blue = pixel & 0xff;
  result->blue += result->blue << 8;
}

BdkScreen*
bdk_colormap_get_screen (BdkColormap *cmap)
{
  g_return_val_if_fail (cmap != NULL, NULL);

  return bdk_screen_get_default ();
}

CGColorRef
_bdk_quartz_colormap_get_cgcolor_from_pixel (BdkDrawable *drawable,
                                             guint32      pixel)
{
  CGFloat components[4] = { 0.0f, };
  CGColorRef color;
  CGColorSpaceRef colorspace;
  const BdkVisual *visual;
  BdkColormap *colormap;

  colormap = bdk_drawable_get_colormap (drawable);
  if (colormap)
    visual = bdk_colormap_get_visual (colormap);
  else
    visual = bdk_visual_get_best_with_depth (bdk_drawable_get_depth (drawable));

  switch (visual->type)
    {
      case BDK_VISUAL_STATIC_GRAY:
      case BDK_VISUAL_GRAYSCALE:
        components[0] = (pixel & 0xff) / 255.0f;

        if (visual->depth == 1)
          components[0] = components[0] == 0.0f ? 0.0f : 1.0f;
        components[1] = 1.0f;

        colorspace = CGColorSpaceCreateWithName (kCGColorSpaceGenericGray);
        color = CGColorCreate (colorspace, components);
        CGColorSpaceRelease (colorspace);
        break;

      default:
        components[0] = (pixel >> 16 & 0xff) / 255.0;
        components[1] = (pixel >> 8  & 0xff) / 255.0;
        components[2] = (pixel       & 0xff) / 255.0;

        if (visual->depth == 32)
          components[3] = (pixel >> 24 & 0xff) / 255.0;
        else
          components[3] = 1.0;

        colorspace = CGColorSpaceCreateDeviceRGB ();
        color = CGColorCreate (colorspace, components);
        CGColorSpaceRelease (colorspace);
        break;
    }

  return color;
}

gboolean
bdk_color_change (BdkColormap *colormap,
		  BdkColor    *color)
{
  if (color->pixel < 0 || color->pixel >= colormap->size)
    return FALSE;

  return TRUE;
}

