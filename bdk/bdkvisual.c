/* BDK - The GIMP Drawing Kit
 * bdkvisual.c
 * 
 * Copyright 2001 Sun Microsystems Inc. 
 *
 * Erwann Chenede <erwann.chenede@sun.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include "bdkvisual.h"
#include "bdkscreen.h"
#include "bdkalias.h"

/**
 * bdk_list_visuals:
 * 
 * Lists the available visuals for the default screen.
 * (See bdk_screen_list_visuals())
 * A visual describes a hardware image data format.
 * For example, a visual might support 24-bit color, or 8-bit color,
 * and might expect pixels to be in a certain format.
 *
 * Call g_list_free() on the return value when you're finished with it.
 * 
 * Return value: (transfer container) (element-type BdkVisual):
 *     a list of visuals; the list must be freed, but not its contents
 **/
GList*
bdk_list_visuals (void)
{
  return bdk_screen_list_visuals (bdk_screen_get_default ());
}

/**
 * bdk_visual_get_system:
 * 
 * Get the system's default visual for the default BDK screen.
 * This is the visual for the root window of the display.
 * The return value should not be freed.
 * 
 * Return value: (transfer none): system visual
 **/
BdkVisual*
bdk_visual_get_system (void)
{
  return bdk_screen_get_system_visual (bdk_screen_get_default());
}

/**
 * bdk_visual_get_visual_type:
 * @visual: A #BdkVisual.
 *
 * Returns the type of visual this is (PseudoColor, TrueColor, etc).
 *
 * Return value: A #BdkVisualType stating the type of @visual.
 *
 * Since: 2.22
 */
BdkVisualType
bdk_visual_get_visual_type (BdkVisual *visual)
{
  g_return_val_if_fail (BDK_IS_VISUAL (visual), 0);

  return visual->type;
}

/**
 * bdk_visual_get_depth:
 * @visual: A #BdkVisual.
 *
 * Returns the bit depth of this visual.
 *
 * Return value: The bit depth of this visual.
 *
 * Since: 2.22
 */
bint
bdk_visual_get_depth (BdkVisual *visual)
{
  g_return_val_if_fail (BDK_IS_VISUAL (visual), 0);

  return visual->depth;
}

/**
 * bdk_visual_get_byte_order:
 * @visual: A #BdkVisual.
 *
 * Returns the byte order of this visual.
 *
 * Return value: A #BdkByteOrder stating the byte order of @visual.
 *
 * Since: 2.22
 */
BdkByteOrder
bdk_visual_get_byte_order (BdkVisual *visual)
{
  g_return_val_if_fail (BDK_IS_VISUAL (visual), 0);

  return visual->byte_order;
}

/**
 * bdk_visual_get_colormap_size:
 * @visual: A #BdkVisual.
 *
 * Returns the size of a colormap for this visual.
 *
 * Return value: The size of a colormap that is suitable for @visual.
 *
 * Since: 2.22
 */
bint
bdk_visual_get_colormap_size (BdkVisual *visual)
{
  g_return_val_if_fail (BDK_IS_VISUAL (visual), 0);

  return visual->colormap_size;
}

/**
 * bdk_visual_get_bits_per_rgb:
 * @visual: a #BdkVisual
 *
 * Returns the number of significant bits per red, green and blue value.
 *
 * Return value: The number of significant bits per color value for @visual.
 *
 * Since: 2.22
 */
bint
bdk_visual_get_bits_per_rgb (BdkVisual *visual)
{
  g_return_val_if_fail (BDK_IS_VISUAL (visual), 0);

  return visual->bits_per_rgb;
}

/**
 * bdk_visual_get_red_pixel_details:
 * @visual: A #BdkVisual.
 * @mask: (out) (allow-none): A pointer to a #buint32 to be filled in, or %NULL.
 * @shift: (out) (allow-none): A pointer to a #bint to be filled in, or %NULL.
 * @precision: (out) (allow-none): A pointer to a #bint to be filled in, or %NULL.
 *
 * Obtains values that are needed to calculate red pixel values in TrueColor
 * and DirectColor.  The "mask" is the significant bits within the pixel.
 * The "shift" is the number of bits left we must shift a primary for it
 * to be in position (according to the "mask").  Finally, "precision" refers
 * to how much precision the pixel value contains for a particular primary.
 *
 * Since: 2.22
 */
void
bdk_visual_get_red_pixel_details (BdkVisual *visual,
                                  buint32   *mask,
                                  bint      *shift,
                                  bint      *precision)
{
  g_return_if_fail (BDK_IS_VISUAL (visual));

  if (mask)
    *mask = visual->red_mask;

  if (shift)
    *shift = visual->red_shift;

  if (precision)
    *precision = visual->red_prec;
}

/**
 * bdk_visual_get_green_pixel_details:
 * @visual: a #BdkVisual
 * @mask: (out) (allow-none): A pointer to a #buint32 to be filled in, or %NULL.
 * @shift: (out) (allow-none): A pointer to a #bint to be filled in, or %NULL.
 * @precision: (out) (allow-none): A pointer to a #bint to be filled in, or %NULL.
 *
 * Obtains values that are needed to calculate green pixel values in TrueColor
 * and DirectColor.  The "mask" is the significant bits within the pixel.
 * The "shift" is the number of bits left we must shift a primary for it
 * to be in position (according to the "mask").  Finally, "precision" refers
 * to how much precision the pixel value contains for a particular primary.
 *
 * Since: 2.22
 */
void
bdk_visual_get_green_pixel_details (BdkVisual *visual,
                                    buint32   *mask,
                                    bint      *shift,
                                    bint      *precision)
{
  g_return_if_fail (BDK_IS_VISUAL (visual));

  if (mask)
    *mask = visual->green_mask;

  if (shift)
    *shift = visual->green_shift;

  if (precision)
    *precision = visual->green_prec;
}

/**
 * bdk_visual_get_blue_pixel_details:
 * @visual: a #BdkVisual
 * @mask: (out) (allow-none): A pointer to a #buint32 to be filled in, or %NULL.
 * @shift: (out) (allow-none): A pointer to a #bint to be filled in, or %NULL.
 * @precision: (out) (allow-none): A pointer to a #bint to be filled in, or %NULL.
 *
 * Obtains values that are needed to calculate blue pixel values in TrueColor
 * and DirectColor.  The "mask" is the significant bits within the pixel.
 * The "shift" is the number of bits left we must shift a primary for it
 * to be in position (according to the "mask").  Finally, "precision" refers
 * to how much precision the pixel value contains for a particular primary.
 *
 * Since: 2.22
 */
void
bdk_visual_get_blue_pixel_details (BdkVisual *visual,
                                   buint32   *mask,
                                   bint      *shift,
                                   bint      *precision)
{
  g_return_if_fail (BDK_IS_VISUAL (visual));

  if (mask)
    *mask = visual->blue_mask;

  if (shift)
    *shift = visual->blue_shift;

  if (precision)
    *precision = visual->blue_prec;
}

#define __BDK_VISUAL_C__
#include "bdkaliasdef.c"
