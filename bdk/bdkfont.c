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

#undef BDK_DISABLE_DEPRECATED

#include "config.h"
#include "bdkdisplay.h"
#include "bdkfont.h"
#include "bdkinternals.h"
#include "bdkalias.h"

GType
bdk_font_get_type (void)
{
  static GType our_type = 0;
  
  if (our_type == 0)
    our_type = g_boxed_type_register_static (g_intern_static_string ("BdkFont"),
					     (GBoxedCopyFunc)bdk_font_ref,
					     (GBoxedFreeFunc)bdk_font_unref);
  return our_type;
}

/**
 * bdk_font_ref:
 * @font: a #BdkFont
 * 
 * Increases the reference count of a font by one.
 * 
 * Return value: @font
 **/
BdkFont*
bdk_font_ref (BdkFont *font)
{
  BdkFontPrivate *private;

  g_return_val_if_fail (font != NULL, NULL);

  private = (BdkFontPrivate*) font;
  private->ref_count += 1;
  return font;
}

/**
 * bdk_font_unref:
 * @font: a #BdkFont
 * 
 * Decreases the reference count of a font by one.
 * If the result is zero, destroys the font.
 **/
void
bdk_font_unref (BdkFont *font)
{
  BdkFontPrivate *private;
  private = (BdkFontPrivate*) font;

  g_return_if_fail (font != NULL);
  g_return_if_fail (private->ref_count > 0);

  private->ref_count -= 1;
  if (private->ref_count == 0)
    _bdk_font_destroy (font);
}

/**
 * bdk_string_width:
 * @font:  a #BdkFont
 * @string: the nul-terminated string to measure
 * 
 * Determines the width of a nul-terminated string.
 * (The distance from the origin of the string to the 
 * point where the next string in a sequence of strings
 * should be drawn)
 * 
 * Return value: the width of the string in pixels.
 **/
gint
bdk_string_width (BdkFont     *font,
		  const gchar *string)
{
  g_return_val_if_fail (font != NULL, -1);
  g_return_val_if_fail (string != NULL, -1);

  return bdk_text_width (font, string, _bdk_font_strlen (font, string));
}

/**
 * bdk_char_width:
 * @font: a #BdkFont
 * @character: the character to measure.
 * 
 * Determines the width of a given character.
 * 
 * Return value: the width of the character in pixels.
 *
 * Deprecated: 2.2: Use bdk_text_extents() instead.
 **/
gint
bdk_char_width (BdkFont *font,
		gchar    character)
{
  g_return_val_if_fail (font != NULL, -1);

  return bdk_text_width (font, &character, 1);
}

/**
 * bdk_char_width_wc:
 * @font: a #BdkFont
 * @character: the character to measure.
 * 
 * Determines the width of a given wide character. (Encoded
 * in the wide-character encoding of the current locale).
 * 
 * Return value: the width of the character in pixels.
 **/
gint
bdk_char_width_wc (BdkFont *font,
		   BdkWChar character)
{
  g_return_val_if_fail (font != NULL, -1);

  return bdk_text_width_wc (font, &character, 1);
}

/**
 * bdk_string_measure:
 * @font: a #BdkFont
 * @string: the nul-terminated string to measure.
 * 
 * Determines the distance from the origin to the rightmost
 * portion of a nul-terminated string when drawn. This is not the
 * correct value for determining the origin of the next
 * portion when drawing text in multiple pieces.
 * See bdk_string_width().
 * 
 * Return value: the right bearing of the string in pixels.
 **/
gint
bdk_string_measure (BdkFont     *font,
                    const gchar *string)
{
  g_return_val_if_fail (font != NULL, -1);
  g_return_val_if_fail (string != NULL, -1);

  return bdk_text_measure (font, string, _bdk_font_strlen (font, string));
}

/**
 * bdk_string_extents:
 * @font: a #BdkFont.
 * @string: the nul-terminated string to measure.
 * @lbearing: the left bearing of the string.
 * @rbearing: the right bearing of the string.
 * @width: the width of the string.
 * @ascent: the ascent of the string.
 * @descent: the descent of the string.
 * 
 * Gets the metrics of a nul-terminated string.
 **/
void
bdk_string_extents (BdkFont     *font,
		    const gchar *string,
		    gint        *lbearing,
		    gint        *rbearing,
		    gint        *width,
		    gint        *ascent,
		    gint        *descent)
{
  g_return_if_fail (font != NULL);
  g_return_if_fail (string != NULL);

  bdk_text_extents (font, string, _bdk_font_strlen (font, string),
		    lbearing, rbearing, width, ascent, descent);
}


/**
 * bdk_text_measure:
 * @font: a #BdkFont
 * @text: the text to measure.
 * @text_length: the length of the text in bytes.
 * 
 * Determines the distance from the origin to the rightmost
 * portion of a string when drawn. This is not the
 * correct value for determining the origin of the next
 * portion when drawing text in multiple pieces. 
 * See bdk_text_width().
 * 
 * Return value: the right bearing of the string in pixels.
 **/
gint
bdk_text_measure (BdkFont     *font,
                  const gchar *text,
                  gint         text_length)
{
  gint rbearing;

  g_return_val_if_fail (font != NULL, -1);
  g_return_val_if_fail (text != NULL, -1);

  bdk_text_extents (font, text, text_length, NULL, &rbearing, NULL, NULL, NULL);
  return rbearing;
}

/**
 * bdk_char_measure:
 * @font: a #BdkFont
 * @character: the character to measure.
 * 
 * Determines the distance from the origin to the rightmost
 * portion of a character when drawn. This is not the
 * correct value for determining the origin of the next
 * portion when drawing text in multiple pieces. 
 * 
 * Return value: the right bearing of the character in pixels.
 **/
gint
bdk_char_measure (BdkFont *font,
                  gchar    character)
{
  g_return_val_if_fail (font != NULL, -1);

  return bdk_text_measure (font, &character, 1);
}

/**
 * bdk_string_height:
 * @font: a #BdkFont
 * @string: the nul-terminated string to measure.
 * 
 * Determines the total height of a given nul-terminated
 * string. This value is not generally useful, because you
 * cannot determine how this total height will be drawn in
 * relation to the baseline. See bdk_string_extents().
 * 
 * Return value: the height of the string in pixels.
 **/
gint
bdk_string_height (BdkFont     *font,
		   const gchar *string)
{
  g_return_val_if_fail (font != NULL, -1);
  g_return_val_if_fail (string != NULL, -1);

  return bdk_text_height (font, string, _bdk_font_strlen (font, string));
}

/**
 * bdk_text_height:
 * @font: a #BdkFont
 * @text: the text to measure.
 * @text_length: the length of the text in bytes.
 * 
 * Determines the total height of a given string.
 * This value is not generally useful, because you cannot
 * determine how this total height will be drawn in
 * relation to the baseline. See bdk_text_extents().
 * 
 * Return value: the height of the string in pixels.
 **/
gint
bdk_text_height (BdkFont     *font,
		 const gchar *text,
		 gint         text_length)
{
  gint ascent, descent;

  g_return_val_if_fail (font != NULL, -1);
  g_return_val_if_fail (text != NULL, -1);

  bdk_text_extents (font, text, text_length, NULL, NULL, NULL, &ascent, &descent);
  return ascent + descent;
}

/**
 * bdk_char_height:
 * @font: a #BdkFont
 * @character: the character to measure.
 * 
 * Determines the total height of a given character.
 * This value is not generally useful, because you cannot
 * determine how this total height will be drawn in
 * relation to the baseline. See bdk_text_extents().
 * 
 * Return value: the height of the character in pixels.
 *
 * Deprecated: 2.2: Use bdk_text_extents() instead.
 **/
gint
bdk_char_height (BdkFont *font,
		 gchar    character)
{
  g_return_val_if_fail (font != NULL, -1);

  return bdk_text_height (font, &character, 1);
}

/**
 * bdk_font_from_description:
 * @font_desc: a #BangoFontDescription.
 * 
 * Load a #BdkFont based on a Bango font description. This font will
 * only be an approximation of the Bango font, and
 * internationalization will not be handled correctly. This function
 * should only be used for legacy code that cannot be easily converted
 * to use Bango. Using Bango directly will produce better results.
 * 
 * Return value: the newly loaded font, or %NULL if the font
 * cannot be loaded.
 **/
BdkFont*
bdk_font_from_description (BangoFontDescription *font_desc)
{
  return bdk_font_from_description_for_display (bdk_display_get_default (),font_desc);
}

/**
 * bdk_font_load:
 * @font_name: a XLFD describing the font to load.
 * 
 * Loads a font.
 * 
 * The font may be newly loaded or looked up the font in a cache. 
 * You should make no assumptions about the initial reference count.
 * 
 * Return value: a #BdkFont, or %NULL if the font could not be loaded.
 **/
BdkFont*
bdk_font_load (const gchar *font_name)
{  
   return bdk_font_load_for_display (bdk_display_get_default(), font_name);
}

#define __BDK_FONT_C__
#include "bdkaliasdef.c"
