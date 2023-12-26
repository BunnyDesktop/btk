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
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <locale.h>

#include "bdkx.h"
#include "bdkfont.h"
#include "bdkprivate-x11.h"
#include "bdkinternals.h"
#include "bdkdisplay-x11.h"
#include "bdkscreen-x11.h"
#include "bdkalias.h"

typedef struct _BdkFontPrivateX        BdkFontPrivateX;

struct _BdkFontPrivateX
{
  BdkFontPrivate base;
  /* XFontStruct *xfont; */
  /* generic pointer point to XFontStruct or XFontSet */
  gpointer xfont;
  BdkDisplay *display;

  GSList *names;
  XID xid;
};

static GHashTable *
bdk_font_name_hash_get (BdkDisplay *display)
{
  GHashTable *result;
  static GQuark font_name_quark = 0;

  if (!font_name_quark)
    font_name_quark = g_quark_from_static_string ("bdk-font-hash");

  result = g_object_get_qdata (G_OBJECT (display), font_name_quark);

  if (!result)
    {
      result = g_hash_table_new (g_str_hash, g_str_equal);
      g_object_set_qdata_full (G_OBJECT (display),
         font_name_quark, result, (GDestroyNotify) g_hash_table_destroy);
    }

  return result;
}

static GHashTable *
bdk_fontset_name_hash_get (BdkDisplay *display)
{
  GHashTable *result;
  static GQuark fontset_name_quark = 0;
  
  if (!fontset_name_quark)
    fontset_name_quark = g_quark_from_static_string ("bdk-fontset-hash");

  result = g_object_get_qdata (G_OBJECT (display), fontset_name_quark);

  if (!result)
    {
      result = g_hash_table_new (g_str_hash, g_str_equal);
      g_object_set_qdata_full (G_OBJECT (display),
         fontset_name_quark, result, (GDestroyNotify) g_hash_table_destroy);
    }

  return result;
}

/** 
 * bdk_font_get_display:
 * @font: the #BdkFont.
 *
 * Returns the #BdkDisplay for @font.
 *
 * Returns: the corresponding #BdkDisplay.
 *
 * Since: 2.2
 **/
BdkDisplay* 
bdk_font_get_display (BdkFont* font)
{
  return ((BdkFontPrivateX *)font)->display;
}

static void
bdk_font_hash_insert (BdkFontType  type, 
		      BdkFont     *font, 
		      const gchar *font_name)
{
  BdkFontPrivateX *private = (BdkFontPrivateX *)font;
  GHashTable *hash = (type == BDK_FONT_FONT) ?
    bdk_font_name_hash_get (private->display) : bdk_fontset_name_hash_get (private->display);

  private->names = g_slist_prepend (private->names, g_strdup (font_name));
  g_hash_table_insert (hash, private->names->data, font);
}

static void
bdk_font_hash_remove (BdkFontType type, 
		      BdkFont    *font)
{
  BdkFontPrivateX *private = (BdkFontPrivateX *)font;
  GSList *tmp_list;
  GHashTable *hash = (type == BDK_FONT_FONT) ?
    bdk_font_name_hash_get (private->display) : bdk_fontset_name_hash_get (private->display);

  tmp_list = private->names;
  while (tmp_list)
    {
      g_hash_table_remove (hash, tmp_list->data);
      g_free (tmp_list->data);
      
      tmp_list = tmp_list->next;
    }

  g_slist_free (private->names);
  private->names = NULL;
}

static BdkFont *
bdk_font_hash_lookup (BdkDisplay  *display, 
		      BdkFontType  type, 
		      const gchar *font_name)
{
  BdkFont *result;
  GHashTable *hash;
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);

  hash = (type == BDK_FONT_FONT) ? bdk_font_name_hash_get (display) : 
				   bdk_fontset_name_hash_get (display);
  if (!hash)
    return NULL;
  else
    {
      result = g_hash_table_lookup (hash, font_name);
      if (result)
	bdk_font_ref (result);
      
      return result;
    }
}

/**
 * bdk_font_load_for_display:
 * @display: a #BdkDisplay
 * @font_name: a XLFD describing the font to load.
 * @returns: a #BdkFont, or %NULL if the font could not be loaded.
 *
 * Loads a font for use on @display.
 *
 * The font may be newly loaded or looked up the font in a cache. 
 * You should make no assumptions about the initial reference count.
 *
 * Since: 2.2
 */
BdkFont *
bdk_font_load_for_display (BdkDisplay  *display, 
			   const gchar *font_name)
{
  BdkFont *font;
  BdkFontPrivateX *private;
  XFontStruct *xfont;

  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (font_name != NULL, NULL);
  
  font = bdk_font_hash_lookup (display, BDK_FONT_FONT, font_name);
  if (font)
    return font;

  xfont = XLoadQueryFont (BDK_DISPLAY_XDISPLAY (display), font_name);
  if (xfont == NULL)
    return NULL;

  font = bdk_font_lookup_for_display (display, xfont->fid);
  if (font != NULL) 
    {
      private = (BdkFontPrivateX *) font;
      if (xfont != private->xfont)
	XFreeFont (BDK_DISPLAY_XDISPLAY (display), xfont);

      bdk_font_ref (font);
    }
  else
    {
      private = g_new (BdkFontPrivateX, 1);
      private->display = display;
      private->xfont = xfont;
      private->base.ref_count = 1;
      private->names = NULL;
      private->xid = xfont->fid | XID_FONT_BIT;
 
      font = (BdkFont*) private;
      font->type = BDK_FONT_FONT;
      font->ascent =  xfont->ascent;
      font->descent = xfont->descent;
      
      _bdk_xid_table_insert (display, &private->xid, font);
    }

  bdk_font_hash_insert (BDK_FONT_FONT, font, font_name);

  return font;
}

/**
 * bdk_font_from_description_for_display:
 * @display: a #BdkDisplay
 * @font_desc: a #BangoFontDescription.
 * 
 * Loads a #BdkFont based on a Bango font description for use on @display. 
 * This font will only be an approximation of the Bango font, and
 * internationalization will not be handled correctly. This function
 * should only be used for legacy code that cannot be easily converted
 * to use Bango. Using Bango directly will produce better results.
 * 
 * Return value: the newly loaded font, or %NULL if the font
 * cannot be loaded.
 *
 * Since: 2.2
 */
BdkFont *
bdk_font_from_description_for_display (BdkDisplay           *display,
				       BangoFontDescription *font_desc)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (font_desc != NULL, NULL);

  return bdk_font_load_for_display (display, "fixed");
}

/**
 * bdk_fontset_load_for_display:
 * @display: a #BdkDisplay
 * @fontset_name: a comma-separated list of XLFDs describing
 *   the component fonts of the fontset to load.
 * @returns: a #BdkFont, or %NULL if the fontset could not be loaded.
 * 
 * Loads a fontset for use on @display.
 *
 * The fontset may be newly loaded or looked up in a cache. 
 * You should make no assumptions about the initial reference count.
 *
 * Since: 2.2
 */
BdkFont *
bdk_fontset_load_for_display (BdkDisplay  *display,
			      const gchar *fontset_name)
{
  BdkFont *font;
  BdkFontPrivateX *private;
  XFontSet fontset;
  gint  missing_charset_count;
  gchar **missing_charset_list;
  gchar *def_string;

  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);
  
  font = bdk_font_hash_lookup (display, BDK_FONT_FONTSET, fontset_name);
  if (font)
    return font;

  private = g_new (BdkFontPrivateX, 1);
  font = (BdkFont*) private;

  private->display = display;
  fontset = XCreateFontSet (BDK_DISPLAY_XDISPLAY (display), fontset_name,
			    &missing_charset_list, &missing_charset_count,
			    &def_string);

  if (missing_charset_count)
    {
      gint i;
      g_printerr ("The font \"%s\" does not support all the required character sets for the current locale \"%s\"\n",
                 fontset_name, setlocale (LC_ALL, NULL));
      for (i=0;i<missing_charset_count;i++)
	g_printerr ("  (Missing character set \"%s\")\n",
                    missing_charset_list[i]);
      XFreeStringList (missing_charset_list);
    }

  private->base.ref_count = 1;

  if (!fontset)
    {
      g_free (font);
      return NULL;
    }
  else
    {
      gint num_fonts;
      gint i;
      XFontStruct **font_structs;
      gchar **font_names;
      
      private->xfont = fontset;
      font->type = BDK_FONT_FONTSET;
      num_fonts = XFontsOfFontSet (fontset, &font_structs, &font_names);

      font->ascent = font->descent = 0;
      
      for (i = 0; i < num_fonts; i++)
	{
	  font->ascent = MAX (font->ascent, font_structs[i]->ascent);
	  font->descent = MAX (font->descent, font_structs[i]->descent);
	}
 
      private->names = NULL;
      bdk_font_hash_insert (BDK_FONT_FONTSET, font, fontset_name);
      
      return font;
    }
}

/**
 * bdk_fontset_load:
 * @fontset_name: a comma-separated list of XLFDs describing
 *     the component fonts of the fontset to load.
 * 
 * Loads a fontset.
 *
 * The fontset may be newly loaded or looked up in a cache. 
 * You should make no assumptions about the initial reference count.
 * 
 * Return value: a #BdkFont, or %NULL if the fontset could not be loaded.
 **/
BdkFont*
bdk_fontset_load (const gchar *fontset_name)
{
  return bdk_fontset_load_for_display (bdk_display_get_default (), fontset_name);
}

void
_bdk_font_destroy (BdkFont *font)
{
  BdkFontPrivateX *private = (BdkFontPrivateX *)font;
  
  bdk_font_hash_remove (font->type, font);
      
  switch (font->type)
    {
    case BDK_FONT_FONT:
      _bdk_xid_table_remove (private->display, private->xid);
      XFreeFont (BDK_DISPLAY_XDISPLAY (private->display),
		  (XFontStruct *) private->xfont);
      break;
    case BDK_FONT_FONTSET:
      XFreeFontSet (BDK_DISPLAY_XDISPLAY (private->display),
		    (XFontSet) private->xfont);
      break;
    default:
      g_error ("unknown font type.");
      break;
    }
  g_free (font);
}

gint
_bdk_font_strlen (BdkFont     *font,
		  const gchar *str)
{
  BdkFontPrivateX *font_private;
  gint length = 0;

  g_return_val_if_fail (font != NULL, -1);
  g_return_val_if_fail (str != NULL, -1);

  font_private = (BdkFontPrivateX*) font;

  if (font->type == BDK_FONT_FONT)
    {
      XFontStruct *xfont = (XFontStruct *) font_private->xfont;
      if ((xfont->min_byte1 == 0) && (xfont->max_byte1 == 0))
	{
	  length = strlen (str);
	}
      else
	{
	  guint16 *string_2b = (guint16 *)str;
	    
	  while (*(string_2b++))
	    length++;
	}
    }
  else if (font->type == BDK_FONT_FONTSET)
    {
      length = strlen (str);
    }
  else
    g_error("undefined font type\n");

  return length;
}

/**
 * bdk_font_id:
 * @font: a #BdkFont.
 * 
 * Returns the X Font ID for the given font. 
 * 
 * Return value: the numeric X Font ID
 **/
gint
bdk_font_id (const BdkFont *font)
{
  const BdkFontPrivateX *font_private;

  g_return_val_if_fail (font != NULL, 0);

  font_private = (const BdkFontPrivateX*) font;

  if (font->type == BDK_FONT_FONT)
    {
      return ((XFontStruct *) font_private->xfont)->fid;
    }
  else
    {
      return 0;
    }
}

/**
 * bdk_font_equal:
 * @fonta: a #BdkFont.
 * @fontb: another #BdkFont.
 * 
 * Compares two fonts for equality. Single fonts compare equal
 * if they have the same X font ID. This operation does
 * not currently work correctly for fontsets.
 * 
 * Return value: %TRUE if the fonts are equal.
 **/
gboolean
bdk_font_equal (const BdkFont *fonta,
                const BdkFont *fontb)
{
  const BdkFontPrivateX *privatea;
  const BdkFontPrivateX *privateb;

  g_return_val_if_fail (fonta != NULL, FALSE);
  g_return_val_if_fail (fontb != NULL, FALSE);

  privatea = (const BdkFontPrivateX*) fonta;
  privateb = (const BdkFontPrivateX*) fontb;

  if (fonta->type == BDK_FONT_FONT && fontb->type == BDK_FONT_FONT)
    {
      return (((XFontStruct *) privatea->xfont)->fid ==
	      ((XFontStruct *) privateb->xfont)->fid);
    }
  else if (fonta->type == BDK_FONT_FONTSET && fontb->type == BDK_FONT_FONTSET)
    {
      gchar *namea, *nameb;

      namea = XBaseFontNameListOfFontSet((XFontSet) privatea->xfont);
      nameb = XBaseFontNameListOfFontSet((XFontSet) privateb->xfont);
      
      return (strcmp(namea, nameb) == 0);
    }
  else
    /* fontset != font */
    return FALSE;
}

/**
 * bdk_text_width:
 * @font: a #BdkFont
 * @text: the text to measure.
 * @text_length: the length of the text in bytes.
 * 
 * Determines the width of a given string.
 * 
 * Return value: the width of the string in pixels.
 **/
gint
bdk_text_width (BdkFont      *font,
		const gchar  *text,
		gint          text_length)
{
  BdkFontPrivateX *private;
  gint width;
  XFontStruct *xfont;
  XFontSet fontset;

  g_return_val_if_fail (font != NULL, -1);
  g_return_val_if_fail (text != NULL, -1);

  private = (BdkFontPrivateX*) font;

  switch (font->type)
    {
    case BDK_FONT_FONT:
      xfont = (XFontStruct *) private->xfont;
      if ((xfont->min_byte1 == 0) && (xfont->max_byte1 == 0))
	{
	  width = XTextWidth (xfont, text, text_length);
	}
      else
	{
	  width = XTextWidth16 (xfont, (XChar2b *) text, text_length / 2);
	}
      break;
    case BDK_FONT_FONTSET:
      fontset = (XFontSet) private->xfont;
      width = XmbTextEscapement (fontset, text, text_length);
      break;
    default:
      width = 0;
    }
  return width;
}

/**
 * bdk_text_width_wc:
 * @font: a #BdkFont
 * @text: the text to measure.
 * @text_length: the length of the text in characters.
 * 
 * Determines the width of a given wide-character string.
 * 
 * Return value: the width of the string in pixels.
 **/
gint
bdk_text_width_wc (BdkFont	  *font,
		   const BdkWChar *text,
		   gint		   text_length)
{
  BdkFontPrivateX *private;
  gint width;
  XFontStruct *xfont;
  XFontSet fontset;

  g_return_val_if_fail (font != NULL, -1);
  g_return_val_if_fail (text != NULL, -1);

  private = (BdkFontPrivateX*) font;

  switch (font->type)
    {
    case BDK_FONT_FONT:
      xfont = (XFontStruct *) private->xfont;
      if ((xfont->min_byte1 == 0) && (xfont->max_byte1 == 0))
        {
          gchar *text_8bit;
          gint i;
          text_8bit = g_new (gchar, text_length);
          for (i=0; i<text_length; i++) text_8bit[i] = text[i];
          width = XTextWidth (xfont, text_8bit, text_length);
          g_free (text_8bit);
        }
      else
        {
          width = 0;
        }
      break;
    case BDK_FONT_FONTSET:
      if (sizeof(BdkWChar) == sizeof(wchar_t))
	{
	  fontset = (XFontSet) private->xfont;
	  width = XwcTextEscapement (fontset, (wchar_t *)text, text_length);
	}
      else
	{
	  wchar_t *text_wchar;
	  gint i;
	  fontset = (XFontSet) private->xfont;
	  text_wchar = g_new(wchar_t, text_length);
	  for (i=0; i<text_length; i++) text_wchar[i] = text[i];
	  width = XwcTextEscapement (fontset, text_wchar, text_length);
	  g_free (text_wchar);
	}
      break;
    default:
      width = 0;
    }
  return width;
}

/**
 * bdk_text_extents:
 * @font: a #BdkFont
 * @text: the text to measure
 * @text_length: the length of the text in bytes. (If the
 *    font is a 16-bit font, this is twice the length
 *    of the text in characters.)
 * @lbearing: the left bearing of the string.
 * @rbearing: the right bearing of the string.
 * @width: the width of the string.
 * @ascent: the ascent of the string.
 * @descent: the descent of the string.
 * 
 * Gets the metrics of a string.
 **/
void
bdk_text_extents (BdkFont     *font,
                  const gchar *text,
                  gint         text_length,
		  gint        *lbearing,
		  gint        *rbearing,
		  gint        *width,
		  gint        *ascent,
		  gint        *descent)
{
  BdkFontPrivateX *private;
  XCharStruct overall;
  XFontStruct *xfont;
  XFontSet    fontset;
  XRectangle  ink, logical;
  int direction;
  int font_ascent;
  int font_descent;

  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);

  private = (BdkFontPrivateX*) font;

  switch (font->type)
    {
    case BDK_FONT_FONT:
      xfont = (XFontStruct *) private->xfont;
      if ((xfont->min_byte1 == 0) && (xfont->max_byte1 == 0))
	{
	  XTextExtents (xfont, text, text_length,
			&direction, &font_ascent, &font_descent,
			&overall);
	}
      else
	{
	  XTextExtents16 (xfont, (XChar2b *) text, text_length / 2,
			  &direction, &font_ascent, &font_descent,
			  &overall);
	}
      if (lbearing)
	*lbearing = overall.lbearing;
      if (rbearing)
	*rbearing = overall.rbearing;
      if (width)
	*width = overall.width;
      if (ascent)
	*ascent = overall.ascent;
      if (descent)
	*descent = overall.descent;
      break;
    case BDK_FONT_FONTSET:
      fontset = (XFontSet) private->xfont;
      XmbTextExtents (fontset, text, text_length, &ink, &logical);
      if (lbearing)
	*lbearing = ink.x;
      if (rbearing)
	*rbearing = ink.x + ink.width;
      if (width)
	*width = logical.width;
      if (ascent)
	*ascent = -ink.y;
      if (descent)
	*descent = ink.y + ink.height;
      break;
    }

}

/**
 * bdk_text_extents_wc:
 * @font: a #BdkFont
 * @text: the text to measure.
 * @text_length: the length of the text in character.
 * @lbearing: the left bearing of the string.
 * @rbearing: the right bearing of the string.
 * @width: the width of the string.
 * @ascent: the ascent of the string.
 * @descent: the descent of the string.
 * 
 * Gets the metrics of a string of wide characters.
 **/
void
bdk_text_extents_wc (BdkFont        *font,
		     const BdkWChar *text,
		     gint            text_length,
		     gint           *lbearing,
		     gint           *rbearing,
		     gint           *width,
		     gint           *ascent,
		     gint           *descent)
{
  BdkFontPrivateX *private;
  XCharStruct overall;
  XFontStruct *xfont;
  XFontSet    fontset;
  XRectangle  ink, logical;
  int direction;
  int font_ascent;
  int font_descent;

  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);

  private = (BdkFontPrivateX*) font;

  switch (font->type)
    {
    case BDK_FONT_FONT:
      {
	gchar *text_8bit;
	gint i;

	xfont = (XFontStruct *) private->xfont;
	g_return_if_fail ((xfont->min_byte1 == 0) && (xfont->max_byte1 == 0));

	text_8bit = g_new (gchar, text_length);
	for (i=0; i<text_length; i++) 
	  text_8bit[i] = text[i];

	XTextExtents (xfont, text_8bit, text_length,
		      &direction, &font_ascent, &font_descent,
		      &overall);
	g_free (text_8bit);
	
	if (lbearing)
	  *lbearing = overall.lbearing;
	if (rbearing)
	  *rbearing = overall.rbearing;
	if (width)
	  *width = overall.width;
	if (ascent)
	  *ascent = overall.ascent;
	if (descent)
	  *descent = overall.descent;
	break;
      }
    case BDK_FONT_FONTSET:
      fontset = (XFontSet) private->xfont;

      if (sizeof(BdkWChar) == sizeof(wchar_t))
	XwcTextExtents (fontset, (wchar_t *)text, text_length, &ink, &logical);
      else
	{
	  wchar_t *text_wchar;
	  gint i;
	  
	  text_wchar = g_new (wchar_t, text_length);
	  for (i = 0; i < text_length; i++)
	    text_wchar[i] = text[i];
	  XwcTextExtents (fontset, text_wchar, text_length, &ink, &logical);
	  g_free (text_wchar);
	}
      if (lbearing)
	*lbearing = ink.x;
      if (rbearing)
	*rbearing = ink.x + ink.width;
      if (width)
	*width = logical.width;
      if (ascent)
	*ascent = -ink.y;
      if (descent)
	*descent = ink.y + ink.height;
      break;
    }

}

/**
 * bdk_x11_font_get_xdisplay:
 * @font: a #BdkFont.
 * 
 * Returns the display of a #BdkFont.
 * 
 * Return value:  an Xlib <type>Display*</type>.
 **/
Display *
bdk_x11_font_get_xdisplay (BdkFont *font)
{
  g_return_val_if_fail (font != NULL, NULL);

  return BDK_DISPLAY_XDISPLAY (((BdkFontPrivateX *)font)->display);
}

/**
 * bdk_x11_font_get_xfont:
 * @font: a #BdkFont.
 * 
 * Returns the X font belonging to a #BdkFont.
 * 
 * Return value: an Xlib <type>XFontStruct*</type> or an <type>XFontSet</type>.
 **/
gpointer
bdk_x11_font_get_xfont (BdkFont *font)
{
  g_return_val_if_fail (font != NULL, NULL);

  return ((BdkFontPrivateX *)font)->xfont;
}

/**
 * bdk_x11_font_get_name:
 * @font: a #BdkFont.
 * 
 * Return the X Logical Font Description (for font->type == BDK_FONT_FONT)
 * or comma separated list of XLFDs (for font->type == BDK_FONT_FONTSET)
 * that was used to load the font. If the same font was loaded
 * via multiple names, which name is returned is undefined.
 * 
 * Return value: the name of the font. This string is owned
 *   by BDK and must not be modified or freed.
 **/
const char *
bdk_x11_font_get_name (BdkFont *font)
{
  BdkFontPrivateX *private = (BdkFontPrivateX *)font;

  g_return_val_if_fail (font != NULL, NULL);

  g_assert (private->names);

  return private->names->data;
}
     
#define __BDK_FONT_X11_C__
#include "bdkaliasdef.c"
