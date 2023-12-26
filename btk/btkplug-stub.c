/* BTK - The GIMP Toolkit
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* Stub implementation of backend-specific BtkPlug functions. */

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "btkplug.h"
#include "btkplugprivate.h"
#include "btkalias.h"

BdkNativeWindow
_btk_plug_windowing_get_id (BtkPlug *plug)
{
  return 0;
}

void
_btk_plug_windowing_realize_toplevel (BtkPlug *plug)
{
}

void
_btk_plug_windowing_map_toplevel (BtkPlug *plug)
{
}

void
_btk_plug_windowing_unmap_toplevel (BtkPlug *plug)
{
}

void
_btk_plug_windowing_set_focus (BtkPlug *plug)
{
}

void
_btk_plug_windowing_add_grabbed_key (BtkPlug        *plug,
				     buint           accelerator_key,
				     BdkModifierType accelerator_mods)
{
}

void
_btk_plug_windowing_remove_grabbed_key (BtkPlug        *plug,
					buint           accelerator_key,
					BdkModifierType accelerator_mods)
{
}

void
_btk_plug_windowing_focus_to_parent (BtkPlug         *plug,
				     BtkDirectionType direction)
{
}

BdkFilterReturn
_btk_plug_windowing_filter_func (BdkXEvent *bdk_xevent,
				 BdkEvent  *event,
				 bpointer   data)
{
  return BDK_FILTER_CONTINUE;
}
