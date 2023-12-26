/* BDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998-2004 Tor Lillqvist
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "bdkpixmap.h"
#include "bdkinternals.h"
#include "bdki18n.h"
#include "bdkwin32.h"

bchar*
bdk_set_locale (void)
{
  if (!setlocale (LC_ALL, ""))
    g_warning ("locale not supported by C library");
  
  return g_win32_getlocale ();
}

bchar *
bdk_wcstombs (const BdkWChar *src)
{
  const bchar *charset;

  g_get_charset (&charset);
  return g_convert ((char *) src, -1, charset, "UCS-4LE", NULL, NULL, NULL);
}

bint
bdk_mbstowcs (BdkWChar    *dest,
	      const bchar *src,
	      bint         dest_max)
{
  bint retval;
  bsize nwritten;
  bint n_ucs4;
  gunichar *ucs4;
  const bchar *charset;

  g_get_charset (&charset);
  ucs4 = (gunichar *) g_convert (src, -1, "UCS-4LE", charset, NULL, &nwritten, NULL);
  n_ucs4 = nwritten * sizeof (BdkWChar);

  retval = MIN (dest_max, n_ucs4);
  memmove (dest, ucs4, retval * sizeof (BdkWChar));
  g_free (ucs4);

  return retval;
}
