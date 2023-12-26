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

#include <locale.h>

#include "bdki18n.h"
#include "bdkinternals.h"
#include "bdkprivate-quartz.h"

gchar*
bdk_set_locale (void)
{
  if (!setlocale (LC_ALL,""))
    g_warning ("locale not supported by C library");
  
  return setlocale (LC_ALL, NULL);
}

gchar *
bdk_wcstombs (const BdkWChar *src)
{
  gchar *mbstr;

  gint length = 0;
  gint i;

  while (src[length] != 0)
    length++;
  
  mbstr = g_new (gchar, length + 1);
  
  for (i = 0; i < length + 1; i++)
    mbstr[i] = src[i];

  return mbstr;
}

gint
bdk_mbstowcs (BdkWChar *dest, const gchar *src, gint dest_max)
{
  gint i;
  
  for (i = 0; i < dest_max && src[i]; i++)
    dest[i] = src[i];

  return i;
}

