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

#include "config.h"
#include "bdkinputprivate.h"
#include "bdkdisplay-x11.h"
#include "bdkalias.h"

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

void
_bdk_input_init (BdkDisplay *display)
{
  BdkDisplayX11 *display_x11 = BDK_DISPLAY_X11 (display);
  
  _bdk_init_input_core (display);
  
  display_x11->input_devices = g_list_append (NULL, display->core_pointer);
  display->ignore_core_events = FALSE;
}

void 
bdk_device_get_state (BdkDevice       *device,
		      BdkWindow       *window,
		      bdouble         *axes,
		      BdkModifierType *mask)
{
  bint x_int, y_int;

  g_return_if_fail (device != NULL);
  g_return_if_fail (BDK_IS_WINDOW (window));

  bdk_window_get_pointer (window, &x_int, &y_int, mask);

  if (axes)
    {
      axes[0] = x_int;
      axes[1] = y_int;
    }
}

bboolean
_bdk_device_get_history (BdkDevice         *device,
			 BdkWindow         *window,
			 buint32            start,
			 buint32            stop,
			 BdkTimeCoord    ***events,
			 bint              *n_events)
{
  g_warning ("bdk_device_get_history() called for invalid device");
  return FALSE;
}

void
_bdk_input_select_events (BdkWindow        *impl_window,
			  BdkDevicePrivate *bdkdev)
{
}

bboolean
_bdk_input_other_event (BdkEvent *event, 
			XEvent *xevent, 
			BdkWindow *window)
{
  return FALSE;
}

void
_bdk_input_configure_event (XConfigureEvent *xevent,
			    BdkWindow       *window)
{
}

void 
_bdk_input_crossing_event (BdkWindow *window,
			   bboolean enter)
{
}

bint 
_bdk_input_grab_pointer (BdkWindow *     window,
			 BdkWindow      *native_window,
			 bint            owner_events,
			 BdkEventMask    event_mask,
			 BdkWindow *     confine_to,
			 buint32         time)
{
  return Success;
}

void
_bdk_input_ungrab_pointer (BdkDisplay *display,
			   buint32     time)
{
}

bboolean
bdk_device_set_mode (BdkDevice   *device,
		     BdkInputMode mode)
{
  return FALSE;
}

#define __BDK_INPUT_NONE_C__
#include "bdkaliasdef.c"
