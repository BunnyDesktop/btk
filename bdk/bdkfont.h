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
  bint ascent;
  bint descent;
};

GType    bdk_font_get_type  (void) B_GNUC_CONST;

BdkFont* bdk_font_ref	    (BdkFont        *font);
void	 bdk_font_unref	    (BdkFont        *font);
bint	 bdk_font_id	    (const BdkFont  *font);
bboolean bdk_font_equal	    (const BdkFont  *fonta,
			     const BdkFont  *fontb);

BdkFont *bdk_font_load_for_display             (BdkDisplay           *display,
						const bchar          *font_name);
BdkFont *bdk_fontset_load_for_display          (BdkDisplay           *display,
						const bchar          *fontset_name);
BdkFont *bdk_font_from_description_for_display (BdkDisplay           *display,
						BangoFontDescription *font_desc);

#ifndef BDK_DISABLE_DEPRECATED

#ifndef BDK_MULTIHEAD_SAFE
BdkFont* bdk_font_load             (const bchar          *font_name);
BdkFont* bdk_fontset_load          (const bchar          *fontset_name);
BdkFont* bdk_font_from_description (BangoFontDescription *font_desc);
#endif

bint	 bdk_string_width   (BdkFont        *font,
			     const bchar    *string);
bint	 bdk_text_width	    (BdkFont        *font,
			     const bchar    *text,
			     bint            text_length);
bint	 bdk_text_width_wc  (BdkFont        *font,
			     const BdkWChar *text,
			     bint            text_length);
bint	 bdk_char_width	    (BdkFont        *font,
			     bchar           character);
bint	 bdk_char_width_wc  (BdkFont        *font,
			     BdkWChar        character);
bint	 bdk_string_measure (BdkFont        *font,
			     const bchar    *string);
bint	 bdk_text_measure   (BdkFont        *font,
			     const bchar    *text,
			     bint            text_length);
bint	 bdk_char_measure   (BdkFont        *font,
			     bchar           character);
bint	 bdk_string_height  (BdkFont        *font,
			     const bchar    *string);
bint	 bdk_text_height    (BdkFont        *font,
			     const bchar    *text,
			     bint            text_length);
bint	 bdk_char_height    (BdkFont        *font,
			     bchar           character);

void     bdk_text_extents   (BdkFont     *font,
			     const bchar *text,
			     bint         text_length,
			     bint        *lbearing,
			     bint        *rbearing,
			     bint        *width,
			     bint        *ascent,
			     bint        *descent);
void    bdk_text_extents_wc (BdkFont        *font,
			     const BdkWChar *text,
			     bint            text_length,
			     bint           *lbearing,
			     bint           *rbearing,
			     bint           *width,
			     bint           *ascent,
			     bint           *descent);
void     bdk_string_extents (BdkFont     *font,
			     const bchar *string,
			     bint        *lbearing,
			     bint        *rbearing,
			     bint        *width,
			     bint        *ascent,
			     bint        *descent);

BdkDisplay * bdk_font_get_display (BdkFont *font);

#endif /* BDK_DISABLE_DEPRECATED */

B_END_DECLS

#endif /* __BDK_FONT_H__ */

#endif /* !BDK_DISABLE_DEPRECATED || BDK_COMPILATION || BTK_COMPILATION */
