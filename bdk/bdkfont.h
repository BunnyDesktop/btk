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

#if !defined(BDK_DISABLE_DEPRECATED) || defined(BDK_COMPILATION)

#ifndef __BDK_FONT_H__
#define __BDK_FONT_H__

#include <bdk/bdktypes.h>
#include <bango/bango.h>

B_BEGIN_DECLS

#define BDK_TYPE_FONT bdk_font_get_type ()

/* Types of font.
 *   BDK_FONT_FONT: the font is an XFontStruct.
 *   BDK_FONT_FONTSET: the font is an XFontSet used for I18N.
 */
typedef enum
{
  BDK_FONT_FONT,
  BDK_FONT_FONTSET
} BdkFontType;

struct _BdkFont
{
  BdkFontType type;
  gint ascent;
  gint descent;
};

GType    bdk_font_get_type  (void) B_GNUC_CONST;

BdkFont* bdk_font_ref	    (BdkFont        *font);
void	 bdk_font_unref	    (BdkFont        *font);
gint	 bdk_font_id	    (const BdkFont  *font);
gboolean bdk_font_equal	    (const BdkFont  *fonta,
			     const BdkFont  *fontb);

BdkFont *bdk_font_load_for_display             (BdkDisplay           *display,
						const gchar          *font_name);
BdkFont *bdk_fontset_load_for_display          (BdkDisplay           *display,
						const gchar          *fontset_name);
BdkFont *bdk_font_from_description_for_display (BdkDisplay           *display,
						BangoFontDescription *font_desc);

#ifndef BDK_DISABLE_DEPRECATED

#ifndef BDK_MULTIHEAD_SAFE
BdkFont* bdk_font_load             (const gchar          *font_name);
BdkFont* bdk_fontset_load          (const gchar          *fontset_name);
BdkFont* bdk_font_from_description (BangoFontDescription *font_desc);
#endif

gint	 bdk_string_width   (BdkFont        *font,
			     const gchar    *string);
gint	 bdk_text_width	    (BdkFont        *font,
			     const gchar    *text,
			     gint            text_length);
gint	 bdk_text_width_wc  (BdkFont        *font,
			     const BdkWChar *text,
			     gint            text_length);
gint	 bdk_char_width	    (BdkFont        *font,
			     gchar           character);
gint	 bdk_char_width_wc  (BdkFont        *font,
			     BdkWChar        character);
gint	 bdk_string_measure (BdkFont        *font,
			     const gchar    *string);
gint	 bdk_text_measure   (BdkFont        *font,
			     const gchar    *text,
			     gint            text_length);
gint	 bdk_char_measure   (BdkFont        *font,
			     gchar           character);
gint	 bdk_string_height  (BdkFont        *font,
			     const gchar    *string);
gint	 bdk_text_height    (BdkFont        *font,
			     const gchar    *text,
			     gint            text_length);
gint	 bdk_char_height    (BdkFont        *font,
			     gchar           character);

void     bdk_text_extents   (BdkFont     *font,
			     const gchar *text,
			     gint         text_length,
			     gint        *lbearing,
			     gint        *rbearing,
			     gint        *width,
			     gint        *ascent,
			     gint        *descent);
void    bdk_text_extents_wc (BdkFont        *font,
			     const BdkWChar *text,
			     gint            text_length,
			     gint           *lbearing,
			     gint           *rbearing,
			     gint           *width,
			     gint           *ascent,
			     gint           *descent);
void     bdk_string_extents (BdkFont     *font,
			     const gchar *string,
			     gint        *lbearing,
			     gint        *rbearing,
			     gint        *width,
			     gint        *ascent,
			     gint        *descent);

BdkDisplay * bdk_font_get_display (BdkFont *font);

#endif /* BDK_DISABLE_DEPRECATED */

B_END_DECLS

#endif /* __BDK_FONT_H__ */

#endif /* !BDK_DISABLE_DEPRECATED || BDK_COMPILATION || BTK_COMPILATION */
