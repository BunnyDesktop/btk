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

#include "config.h"
#include "bdkx.h"
#include "bdkprivate-x11.h"
#include "bdkdisplay-x11.h"
#include "bdkalias.h"
#include <stdio.h>

static buint     bdk_xid_hash  (XID *xid);
static bboolean  bdk_xid_equal (XID *a,
				XID *b);


/* The 3 high bits of XIDs are unused. We use one to mark fonts, 
 * since we must be able to skip fonts when iterating over all XIDs.
 */
#define XID_FONT_BIT (1<<31)

void
_bdk_xid_table_insert (BdkDisplay *display,
		       XID	  *xid,
		       bpointer    data)
{
  BdkDisplayX11 *display_x11;

  g_return_if_fail (xid != NULL);
  g_return_if_fail (BDK_IS_DISPLAY (display));

  display_x11 = BDK_DISPLAY_X11 (display);

  if (!display_x11->xid_ht)
    display_x11->xid_ht = g_hash_table_new ((GHashFunc) bdk_xid_hash,
					    (GEqualFunc) bdk_xid_equal);

  if (g_hash_table_lookup (display_x11->xid_ht, xid))
    g_warning ("XID collision, trouble ahead");

  g_hash_table_insert (display_x11->xid_ht, xid, data);
}

void
_bdk_xid_table_remove (BdkDisplay *display,
		       XID	   xid)
{
  BdkDisplayX11 *display_x11;

  g_return_if_fail (BDK_IS_DISPLAY (display));

  display_x11 = BDK_DISPLAY_X11 (display);

  if (display_x11->xid_ht)
    g_hash_table_remove (display_x11->xid_ht, &xid);
}

/**
 * bdk_xid_table_lookup_for_display:
 * @display: the #BdkDisplay.
 * @xid: an X id.
 *
 * Returns the BDK object associated with the given X id.
 *
 * Return value: the associated Bdk object, which may be a #BdkPixmap,
 *     a #BdkWindow or a #BdkFont or %NULL if no object is associated
 *     with the X id.
 *
 * Since: 2.2
 *
 * Deprecated:2.24: This function will be removed in BTK+ 3.0. BTK+
 *     only stores windows in its X id table nowadays, so use
 *     bdk_x11_window_lookup_for_display() instead.
 */
bpointer
bdk_xid_table_lookup_for_display (BdkDisplay  *display,
				  XID	       xid)
{
  BdkDisplayX11 *display_x11;
  bpointer data = NULL;
  
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);
  
  display_x11 = BDK_DISPLAY_X11 (display);

  if (display_x11->xid_ht)
    data = g_hash_table_lookup (display_x11->xid_ht, &xid);
  
  return data;
}


/**
 * bdk_xid_table_lookup:
 * @xid: an X id.
 *
 * Returns the Bdk object associated with the given X id for the default
 * display.
 *
 * Return value: the associated Bdk object, which may be a #BdkPixmap,
 *     a #BdkWindow or a #BdkFont or %NULL if no object is associated
 *     with the X id.
 *
 * Deprecated:2.24: This function will be removed in BTK+ 3.0. BTK+
 *     only stores windows in its X id table nowadays, so use
 *     bdk_x11_window_lookup_for_display() instead.
 */
bpointer
bdk_xid_table_lookup (XID xid)
{
  return bdk_xid_table_lookup_for_display (bdk_display_get_default (), xid);
}

static buint
bdk_xid_hash (XID *xid)
{
  return *xid;
}

static bboolean
bdk_xid_equal (XID *a,
	       XID *b)
{
  return ((*a & ~XID_FONT_BIT) == (*b & ~XID_FONT_BIT));
}

#define __BDK_XID_C__
#include "bdkaliasdef.c"
