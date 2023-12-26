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

#include <stdio.h>

#include "bdktypes.h"
#include "bdkprivate.h"
#include "bdkalias.h"

buint               _bdk_debug_flags = 0;
bint                _bdk_error_code = 0;
bint                _bdk_error_warnings = TRUE;
GList              *_bdk_default_filters = NULL;
bchar              *_bdk_display_name = NULL;
bint                _bdk_screen_number = -1;
bchar              *_bdk_display_arg_name = NULL;
bboolean            _bdk_native_windows = FALSE;

GSList             *_bdk_displays = NULL;

GMutex              *bdk_threads_mutex = NULL;          /* Global BDK lock */
GCallback            bdk_threads_lock = NULL;
GCallback            bdk_threads_unlock = NULL;
