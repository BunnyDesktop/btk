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

#include "config.h"
#include "bdk.h"

#include "bdkdirectfb.h"
#include "bdkprivate-directfb.h"

#include "bdkalias.h"


BdkDisplayDFB *_bdk_display = NULL;
BdkScreen     *_bdk_screen  = NULL;

gboolean bdk_directfb_apply_focus_opacity = FALSE;
gboolean bdk_directfb_enable_color_keying = FALSE;
DFBColor bdk_directfb_bg_color            = { 0, 0, 0, 0 };
DFBColor bdk_directfb_bg_color_key        = { 0, 0, 0, 0 };
gboolean bdk_directfb_monochrome_fonts    = FALSE;

BdkWindow    *_bdk_directfb_pointer_grab_window        = NULL;
BdkWindow    *_bdk_directfb_keyboard_grab_window       = NULL;
BdkWindow    *_bdk_directfb_pointer_grab_confine       = NULL;
gboolean      _bdk_directfb_pointer_grab_owner_events  = FALSE;
gboolean      _bdk_directfb_keyboard_grab_owner_events = FALSE;
BdkEventMask  _bdk_directfb_pointer_grab_events        = 0;
BdkEventMask  _bdk_directfb_keyboard_grab_events       = 0;
BdkCursor    *_bdk_directfb_pointer_grab_cursor        = NULL;

BdkAtom _bdk_selection_property = 0;

#include "bdkaliasdef.c"
