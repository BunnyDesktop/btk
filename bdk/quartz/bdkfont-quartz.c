/* bdkwindow-quartz.c
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

#include "bdkfont.h"

BdkFont*
bdk_font_load_for_display (BdkDisplay  *display,
			   const bchar *font_name)
{
  /* FIXME: Implement */
  return NULL;
}

BdkFont*
bdk_font_from_description_for_display (BdkDisplay           *display,
				       BangoFontDescription *desc)
{
  /* FIXME: Implement */
  return NULL;
}

BdkFont *
bdk_fontset_load_for_display (BdkDisplay  *display,
			      const bchar *fontset_name)
{
  return NULL;
}

BdkFont*
bdk_fontset_load (const bchar *fontset_name)
{
  return NULL;
}

bint
bdk_text_width (BdkFont      *font,
		const bchar  *text,
		bint          text_length)
{
  /* FIXME: Implement */
  return -1;
}

void
bdk_text_extents (BdkFont     *font,
                  const bchar *text,
                  bint         text_length,
		  bint        *lbearing,
		  bint        *rbearing,
		  bint        *width,
		  bint        *ascent,
		  bint        *descent)
{
  /* FIXME: Implement */
}

bint
bdk_text_width_wc (BdkFont	  *font,
		   const BdkWChar *text,
		   bint		   text_length)
{
  /* FIXME: Implement */
  return 0;
}


void
bdk_text_extents_wc (BdkFont        *font,
		     const BdkWChar *text,
		     bint            text_length,
		     bint           *lbearing,
		     bint           *rbearing,
		     bint           *width,
		     bint           *ascent,
		     bint           *descent)
{
  /* FIXME: Implement */
}

void
_bdk_font_destroy (BdkFont *font)
{
  /* FIXME: Implement */
}

bint
_bdk_font_strlen (BdkFont     *font,
		  const bchar *str)
{
  /* FIXME: Implement */
  return -1;
}

bint
bdk_font_id (const BdkFont *font)
{
  /* FIXME: Implement */
  return 0;
}

bboolean
bdk_font_equal (const BdkFont *fonta,
                const BdkFont *fontb)
{
  /* FIXME: Implement */
  return FALSE;
}

BdkDisplay* 
bdk_font_get_display (BdkFont* font)
{
  /* FIXME: Implement */ 
  return NULL;
}
