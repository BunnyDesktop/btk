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
#include <time.h>

#include "bdkscreen.h"
#include "bdkcolor.h"
#include "bdkinternals.h"
#include "bdkalias.h"

/**
 * bdk_colormap_ref:
 * @cmap: a #BdkColormap
 *
 * Deprecated function; use g_object_ref() instead.
 *
 * Return value: the colormap
 *
 * Deprecated: 2.0: Use g_object_ref() instead.
 **/
BdkColormap*
bdk_colormap_ref (BdkColormap *cmap)
{
  return (BdkColormap *) g_object_ref (cmap);
}

/**
 * bdk_colormap_unref:
 * @cmap: a #BdkColormap
 *
 * Deprecated function; use g_object_unref() instead.
 *
 * Deprecated: 2.0: Use g_object_unref() instead.
 **/
void
bdk_colormap_unref (BdkColormap *cmap)
{
  g_object_unref (cmap);
}


/**
 * bdk_colormap_get_visual:
 * @colormap: a #BdkColormap.
 * 
 * Returns the visual for which a given colormap was created.
 * 
 * Return value: the visual of the colormap.
 **/
BdkVisual *
bdk_colormap_get_visual (BdkColormap *colormap)
{
  g_return_val_if_fail (BDK_IS_COLORMAP (colormap), NULL);

  return colormap->visual;
}

/**
 * bdk_colors_store:
 * @colormap: a #BdkColormap.
 * @colors: the new color values.
 * @ncolors: the number of colors to change.
 * 
 * Changes the value of the first @ncolors colors in
 * a private colormap. This function is obsolete and
 * should not be used. See bdk_color_change().
 **/     
void
bdk_colors_store (BdkColormap   *colormap,
		  BdkColor      *colors,
		  bint           ncolors)
{
  bint i;

  for (i = 0; i < ncolors; i++)
    {
      colormap->colors[i].pixel = colors[i].pixel;
      colormap->colors[i].red = colors[i].red;
      colormap->colors[i].green = colors[i].green;
      colormap->colors[i].blue = colors[i].blue;
    }

  bdk_colormap_change (colormap, ncolors);
}

/**
 * bdk_color_copy:
 * @color: a #BdkColor.
 * 
 * Makes a copy of a color structure. The result
 * must be freed using bdk_color_free().
 * 
 * Return value: a copy of @color.
 **/
BdkColor*
bdk_color_copy (const BdkColor *color)
{
  BdkColor *new_color;
  
  g_return_val_if_fail (color != NULL, NULL);

  new_color = g_slice_new (BdkColor);
  *new_color = *color;
  return new_color;
}

/**
 * bdk_color_free:
 * @color: a #BdkColor.
 * 
 * Frees a color structure created with 
 * bdk_color_copy().
 **/
void
bdk_color_free (BdkColor *color)
{
  g_return_if_fail (color != NULL);

  g_slice_free (BdkColor, color);
}

/**
 * bdk_color_white:
 * @colormap: a #BdkColormap.
 * @color: the location to store the color.
 * 
 * Returns the white color for a given colormap. The resulting
 * value has already allocated been allocated. 
 * 
 * Return value: %TRUE if the allocation succeeded.
 **/
bboolean
bdk_color_white (BdkColormap *colormap,
		 BdkColor    *color)
{
  bint return_val;

  g_return_val_if_fail (colormap != NULL, FALSE);

  if (color)
    {
      color->red = 65535;
      color->green = 65535;
      color->blue = 65535;

      return_val = bdk_colormap_alloc_color (colormap, color, FALSE, TRUE);
    }
  else
    return_val = FALSE;

  return return_val;
}

/**
 * bdk_color_black:
 * @colormap: a #BdkColormap.
 * @color: the location to store the color.
 * 
 * Returns the black color for a given colormap. The resulting
 * value has already been allocated. 
 * 
 * Return value: %TRUE if the allocation succeeded.
 **/
bboolean
bdk_color_black (BdkColormap *colormap,
		 BdkColor    *color)
{
  bint return_val;

  g_return_val_if_fail (colormap != NULL, FALSE);

  if (color)
    {
      color->red = 0;
      color->green = 0;
      color->blue = 0;

      return_val = bdk_colormap_alloc_color (colormap, color, FALSE, TRUE);
    }
  else
    return_val = FALSE;

  return return_val;
}

/********************
 * Color allocation *
 ********************/

/**
 * bdk_colormap_alloc_color:
 * @colormap: a #BdkColormap.
 * @color: the color to allocate. On return the
 *    <structfield>pixel</structfield> field will be
 *    filled in if allocation succeeds.
 * @writeable: If %TRUE, the color is allocated writeable
 *    (their values can later be changed using bdk_color_change()).
 *    Writeable colors cannot be shared between applications.
 * @best_match: If %TRUE, BDK will attempt to do matching against
 *    existing colors if the color cannot be allocated as requested.
 *
 * Allocates a single color from a colormap.
 * 
 * Return value: %TRUE if the allocation succeeded.
 **/
bboolean
bdk_colormap_alloc_color (BdkColormap *colormap,
			  BdkColor    *color,
			  bboolean     writeable,
			  bboolean     best_match)
{
  bboolean success;

  bdk_colormap_alloc_colors (colormap, color, 1, writeable, best_match,
			     &success);

  return success;
}

/**
 * bdk_color_alloc:
 * @colormap: a #BdkColormap.
 * @color: The color to allocate. On return, the 
 *    <structfield>pixel</structfield> field will be filled in.
 * 
 * Allocates a single color from a colormap.
 * 
 * Return value: %TRUE if the allocation succeeded.
 *
 * Deprecated: 2.2: Use bdk_colormap_alloc_color() instead.
 **/
bboolean
bdk_color_alloc (BdkColormap *colormap,
		 BdkColor    *color)
{
  bboolean success;

  bdk_colormap_alloc_colors (colormap, color, 1, FALSE, TRUE, &success);

  return success;
}

/**
 * bdk_color_hash:
 * @colora: a #BdkColor.
 * 
 * A hash function suitable for using for a hash
 * table that stores #BdkColor's.
 * 
 * Return value: The hash function applied to @colora
 **/
buint
bdk_color_hash (const BdkColor *colora)
{
  return ((colora->red) +
	  (colora->green << 11) +
	  (colora->blue << 22) +
	  (colora->blue >> 6));
}

/**
 * bdk_color_equal:
 * @colora: a #BdkColor.
 * @colorb: another #BdkColor.
 * 
 * Compares two colors. 
 * 
 * Return value: %TRUE if the two colors compare equal
 **/
bboolean
bdk_color_equal (const BdkColor *colora,
		 const BdkColor *colorb)
{
  g_return_val_if_fail (colora != NULL, FALSE);
  g_return_val_if_fail (colorb != NULL, FALSE);

  return ((colora->red == colorb->red) &&
	  (colora->green == colorb->green) &&
	  (colora->blue == colorb->blue));
}

GType
bdk_color_get_type (void)
{
  static GType our_type = 0;
  
  if (our_type == 0)
    our_type = g_boxed_type_register_static (g_intern_static_string ("BdkColor"),
					     (GBoxedCopyFunc)bdk_color_copy,
					     (GBoxedFreeFunc)bdk_color_free);
  return our_type;
}

/**
 * bdk_color_parse:
 * @spec: the string specifying the color.
 * @color: (out): the #BdkColor to fill in
 *
 * Parses a textual specification of a color and fill in the
 * <structfield>red</structfield>, <structfield>green</structfield>,
 * and <structfield>blue</structfield> fields of a #BdkColor
 * structure. The color is <emphasis>not</emphasis> allocated, you
 * must call bdk_colormap_alloc_color() yourself. The string can
 * either one of a large set of standard names. (Taken from the X11
 * <filename>rgb.txt</filename> file), or it can be a hex value in the
 * form '&num;rgb' '&num;rrggbb' '&num;rrrgggbbb' or
 * '&num;rrrrggggbbbb' where 'r', 'g' and 'b' are hex digits of the
 * red, green, and blue components of the color, respectively. (White
 * in the four forms is '&num;fff' '&num;ffffff' '&num;fffffffff' and
 * '&num;ffffffffffff')
 * 
 * Return value: %TRUE if the parsing succeeded.
 **/
bboolean
bdk_color_parse (const bchar *spec,
		 BdkColor    *color)
{
  BangoColor bango_color;

  if (bango_color_parse (&bango_color, spec))
    {
      color->red = bango_color.red;
      color->green = bango_color.green;
      color->blue = bango_color.blue;

      return TRUE;
    }
  else
    return FALSE;
}

/**
 * bdk_color_to_string:
 * @color: a #BdkColor
 *
 * Returns a textual specification of @color in the hexadecimal form
 * <literal>&num;rrrrggggbbbb</literal>, where <literal>r</literal>,
 * <literal>g</literal> and <literal>b</literal> are hex digits
 * representing the red, green and blue components respectively.
 *
 * Return value: a newly-allocated text string
 *
 * Since: 2.12
 **/
bchar *
bdk_color_to_string (const BdkColor *color)
{
  BangoColor bango_color;

  g_return_val_if_fail (color != NULL, NULL);

  bango_color.red = color->red;
  bango_color.green = color->green;
  bango_color.blue = color->blue;

  return bango_color_to_string (&bango_color);
}

/**
 * bdk_colormap_get_system:
 * 
 * Gets the system's default colormap for the default screen. (See
 * bdk_colormap_get_system_for_screen ())
 * 
 * Return value: the default colormap.
 **/
BdkColormap*
bdk_colormap_get_system (void)
{
  return bdk_screen_get_system_colormap (bdk_screen_get_default ());
}

#define __BDK_COLOR_C__
#include "bdkaliasdef.c"
