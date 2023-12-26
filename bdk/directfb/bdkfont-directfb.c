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

#undef BDK_DISABLE_DEPRECATED

#include "config.h"
#include "bdk.h"

#include <string.h>

#include "bdkdirectfb.h"
#include "bdkprivate-directfb.h"

#include "bdkinternals.h"

#include "bdkfont.h"
#include "bdkalias.h"


typedef struct _BdkFontDirectFB  BdkFontDirectFB;

struct _BdkFontDirectFB
{
  BdkFontPrivate    base;
  gint              size;
  IDirectFBFont    *dfbfont;
};


static BdkFont *
bdk_directfb_bogus_font (gint height)
{
  BdkFont         *font;
  BdkFontDirectFB *private;

  private = g_new0 (BdkFontDirectFB, 1);
  font = (BdkFont *) private;

  font->type              = BDK_FONT_FONT;
  font->ascent            = height * 3 / 4;
  font->descent           = height / 4;
  private->size           = height;
  private->base.ref_count = 1;
  return font;
}

BdkFont *
bdk_font_from_description_for_display (BdkDisplay *display,
                                       BangoFontDescription *font_desc)
{
  gint size;

  g_return_val_if_fail (font_desc, NULL);

  size = bango_font_description_get_size (font_desc);

  return bdk_directfb_bogus_font (BANGO_PIXELS (size));
}

/* ********************* */

BdkFont *
bdk_fontset_load (const gchar *fontset_name)
{
  return bdk_directfb_bogus_font (10);
}

BdkFont *
bdk_fontset_load_for_display (BdkDisplay *display,const gchar *font_name)
{
  return bdk_directfb_bogus_font (10);
}

BdkFont *
bdk_font_load_for_display (BdkDisplay *display, const gchar *font_name)
{
  return bdk_directfb_bogus_font (10);
}

void
_bdk_font_destroy (BdkFont *font)
{
  switch (font->type)
    {
    case BDK_FONT_FONT:
      break;
    case BDK_FONT_FONTSET:
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
  BdkFontDirectFB *font_private;
  gint             length = 0;

  g_return_val_if_fail (font != NULL, -1);
  g_return_val_if_fail (str != NULL, -1);

  font_private = (BdkFontDirectFB*) font;

  if (font->type == BDK_FONT_FONT)
    {
      guint16 *string_2b = (guint16 *)str;

      while (*(string_2b++))
        length++;
    }
  else if (font->type == BDK_FONT_FONTSET)
    {
      length = strlen (str);
    }
  else
    g_error("undefined font type\n");

  return length;
}

gint
bdk_font_id (const BdkFont *font)
{
  const BdkFontDirectFB *font_private;

  g_return_val_if_fail (font != NULL, 0);

  font_private = (const BdkFontDirectFB*) font;

  if (font->type == BDK_FONT_FONT)
    {
      return -1;
    }
  else
    {
      return 0;
    }
}

gint
bdk_font_equal (const BdkFont *fonta,
                const BdkFont *fontb)
{
  const BdkFontDirectFB *privatea;
  const BdkFontDirectFB *privateb;

  g_return_val_if_fail (fonta != NULL, FALSE);
  g_return_val_if_fail (fontb != NULL, FALSE);

  privatea = (const BdkFontDirectFB*) fonta;
  privateb = (const BdkFontDirectFB*) fontb;

  if (fonta == fontb)
    return TRUE;

  return FALSE;
}

gint
bdk_text_width (BdkFont      *font,
                const gchar  *text,
                gint          text_length)
{
  BdkFontDirectFB *private;

  private = (BdkFontDirectFB*) font;

  return (text_length * private->size) / 2;
}

gint
bdk_text_width_wc (BdkFont        *font,
                   const BdkWChar *text,
                   gint            text_length)
{
  return 0;
}

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
  if (ascent)
    *ascent = font->ascent;
  if (descent)
    *descent = font->descent;
  if (width)
    *width = bdk_text_width (font, text, text_length);
  if (lbearing)
    *lbearing = 0;
  if (rbearing)
    *rbearing = 0;
}

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
  char *realstr;
  int i;

  realstr = alloca (text_length + 1);

  for(i = 0; i < text_length; i++)
    realstr[i] = text[i];

  realstr[i] = '\0';

  return bdk_text_extents (font,
                           realstr,
                           text_length,
                           lbearing,
                           rbearing,
                           width,
                           ascent,
                           descent);
}

BdkFont *
bdk_font_lookup (BdkNativeWindow xid)
{
  g_warning(" bdk_font_lookup unimplemented \n");
  return NULL;
}

BdkDisplay *
bdk_font_get_display (BdkFont* font)
{
  g_warning(" bdk_font_get_display unimplemented \n");
  return NULL;
}

#define __BDK_FONT_X11_C__
#include "bdkaliasdef.c"
