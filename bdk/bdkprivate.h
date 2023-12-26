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

#ifndef __BDK_PRIVATE_H__
#define __BDK_PRIVATE_H__

#include <bdk/bdk.h>

B_BEGIN_DECLS

#define BDK_PARENT_RELATIVE_BG ((BdkPixmap *)1L)
#define BDK_NO_BG ((BdkPixmap *)2L)

#ifndef BDK_COMPILATION
#define BDK_WINDOW_TYPE(d) (bdk_window_get_window_type (BDK_WINDOW (d)))
#define BDK_WINDOW_DESTROYED(d) (bdk_window_is_destroyed (BDK_WINDOW (d)))
#endif

void bdk_window_destroy_notify	     (BdkWindow *window);

void bdk_synthesize_window_state (BdkWindow     *window,
                                  BdkWindowState unset_flags,
                                  BdkWindowState set_flags);

/* Tests whether a pair of x,y may cause overflows when converted to Bango
 * units (multiplied by BANGO_SCALE).  We don't allow the entire range, leave
 * some space for additions afterwards, to be safe...
 */
#define BDK_BANGO_UNITS_OVERFLOWS(x,y) (B_UNLIKELY ( \
	(y) >= BANGO_PIXELS (B_MAXINT-BANGO_SCALE)/2 || \
	(x) >= BANGO_PIXELS (B_MAXINT-BANGO_SCALE)/2 || \
	(y) <=-BANGO_PIXELS (B_MAXINT-BANGO_SCALE)/2 || \
	(x) <=-BANGO_PIXELS (B_MAXINT-BANGO_SCALE)/2))

B_END_DECLS

#endif /* __BDK_PRIVATE_H__ */
