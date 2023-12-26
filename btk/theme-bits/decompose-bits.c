/* BTK - The GIMP Toolkit
 * Copyright (C) 2002, Owen Taylor
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
#include <bdk-pixbuf/bdk-pixbuf.h>
#include <bunnylib/gprintf.h>
#include <stdlib.h>
#include <string.h>

#define BYTES_PER_OUTPUT_LINE 15

static bint
output_byte (buchar byte,
	     bint   online)
{
  if (online == BYTES_PER_OUTPUT_LINE)
    {
      g_printf (",\n  ");
      online = 0;
    }
  else if (online)
    {
      g_printf (",");
    }

  g_printf ("0x%02x", byte);
  return online + 1;
}

static void
do_part (BdkPixbuf  *pixbuf,
	 bint        part1_index,
	 bint        part2_index,
	 bint        part3_index,
	 const char *base_name,
	 const char *part_name)
{
  const buchar *pixels = bdk_pixbuf_get_pixels (pixbuf);
  const buchar *color1;
  const buchar *color2;
  const buchar *color3;
  bint rowstride = bdk_pixbuf_get_rowstride (pixbuf);
  bint n_channels = bdk_pixbuf_get_n_channels (pixbuf);
  bint width = bdk_pixbuf_get_width (pixbuf);
  bint height = bdk_pixbuf_get_height (pixbuf);
  bint online = 0;

  color1 = pixels + part1_index * n_channels;
  color2 = pixels + part2_index * n_channels;
  color3 = pixels + part3_index * n_channels;
  pixels += rowstride;
  
  g_printf ("static const buchar %s_%s_bits[] = {\n", base_name, part_name);
  g_printf ("  ");

  while (height--)
    {
      buchar bit = 1;
      buchar byte = 0;
      const buchar *p = pixels;
      bint n = width;

      while (n--)
	{
	  if ((part1_index >= 0 && memcmp (p, color1, n_channels) == 0) ||
	      (part2_index >= 0 && memcmp (p, color2, n_channels) == 0) ||
	      (part3_index >= 0 && memcmp (p, color3, n_channels) == 0))
	    byte |= bit;

	  if (bit == 0x80)
	    {
	      online = output_byte (byte, online);
	      byte = 0;
	      bit = 1;
	    }
	  else
	    bit <<= 1;

	  p += n_channels;
	}

      if (width & 7)		/* a leftover partial byte */
	online = output_byte (byte, online);

      pixels += rowstride;
    }
  
  g_printf ("};\n");
}

typedef enum {
  PART_BLACK,
  PART_DARK,
  PART_MID,
  PART_LIGHT,
  PART_TEXT,
  PART_TEXT_AA,
  PART_BASE,
  PART_LAST
} Part;

static const char *part_names[PART_LAST] = {
  "black",
  "dark",
  "mid",
  "light",
  "text",
  "aa",
  "base",
};

int main (int argc, char **argv)
{
  bchar *progname = g_path_get_basename (argv[0]);
  BdkPixbuf *pixbuf;
  GError *error = NULL;
  bint i;

  if (argc != 3)
    {
      g_fprintf (stderr, "%s: Usage: %s FILE BASE\n", progname, progname);
      exit (1);
    }

  g_type_init ();
  
  pixbuf = bdk_pixbuf_new_from_file (argv[1], &error);
  if (!pixbuf)
    {
      g_fprintf (stderr, "%s: cannot open file '%s': %s\n", progname, argv[1], error->message);
      exit (1);
    }
  
  if (bdk_pixbuf_get_width (pixbuf) < PART_LAST)
    {
      g_fprintf (stderr, "%s: source image must be at least %d pixels wide\n", progname, PART_LAST);
      exit (1);
    }

  if (bdk_pixbuf_get_height (pixbuf) < 1)
    {
      g_fprintf (stderr, "%s: source image must be at least 1 pixel height\n", progname);
      exit (1);
    }

  g_printf ("/*\n * Extracted from %s, width=%d, height=%d\n */\n", argv[1],
	  bdk_pixbuf_get_width (pixbuf), bdk_pixbuf_get_height (pixbuf) - 1);

  for (i = 0; i < PART_LAST; i++)
    {
      /* As a bit of a hack, we want the base image to extend over the text
       * and text_aa parts so that we can draw the image either with or without
       * the indicator
       */
      if (i == PART_BASE)
	do_part (pixbuf, PART_BASE, PART_TEXT_AA, PART_TEXT, argv[2], part_names[i]);
      else
	do_part (pixbuf, i, -1, -1, argv[2], part_names[i]);
    }
  return 0;
}
