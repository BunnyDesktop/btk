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
#include <string.h>
#include "bdkinputprivate.h"
#include "bdkdisplay-x11.h"
#include "bdkalias.h"

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

/* forward declarations */

static void bdk_input_check_proximity (BdkDisplay *display);

void
_bdk_input_init(BdkDisplay *display)
{
  _bdk_init_input_core (display);
  display->ignore_core_events = FALSE;
  _bdk_input_common_init (display, FALSE);
}

gboolean
bdk_device_set_mode (BdkDevice      *device,
		     BdkInputMode    mode)
{
  GList *tmp_list;
  BdkDevicePrivate *bdkdev;
  BdkInputWindow *input_window;
  BdkDisplayX11 *display_impl;

  if (BDK_IS_CORE (device))
    return FALSE;

  bdkdev = (BdkDevicePrivate *)device;

  if (device->mode == mode)
    return TRUE;

  device->mode = mode;

  if (mode == BDK_MODE_WINDOW)
    device->has_cursor = FALSE;
  else if (mode == BDK_MODE_SCREEN)
    device->has_cursor = TRUE;

  display_impl = BDK_DISPLAY_X11 (bdkdev->display);
  for (tmp_list = display_impl->input_windows; tmp_list; tmp_list = tmp_list->next)
    {
      input_window = (BdkInputWindow *)tmp_list->data;
      _bdk_input_select_events (input_window->impl_window, bdkdev);
    }

  return TRUE;
}

static int
ignore_errors (Display *display, XErrorEvent *event)
{
  return True;
}

static void
bdk_input_check_proximity (BdkDisplay *display)
{
  BdkDisplayX11 *display_impl = BDK_DISPLAY_X11 (display);
  GList *tmp_list = display_impl->input_devices;
  gint new_proximity = 0;

  while (tmp_list && !new_proximity)
    {
      BdkDevicePrivate *bdkdev = (BdkDevicePrivate *)(tmp_list->data);

      if (bdkdev->info.mode != BDK_MODE_DISABLED
	  && !BDK_IS_CORE (bdkdev)
	  && bdkdev->xdevice)
	{
      int (*old_handler) (Display *, XErrorEvent *);
      XDeviceState *state = NULL;
      XInputClass *xic;
      int i;

      /* From X11 doc: "XQueryDeviceState can generate a BadDevice error."
       * This would occur in particular when a device is unplugged,
       * which would cause the program to crash (see bug 575767).
       *
       * To handle this case gracefully, we simply ignore the device.
       * BTK+ 3 handles this better with XInput 2's hotplugging support;
       * but this is better than a crash in BTK+ 2.
       */
      old_handler = XSetErrorHandler (ignore_errors);
      state = XQueryDeviceState(display_impl->xdisplay, bdkdev->xdevice);
      XSetErrorHandler (old_handler);

      if (! state)
        {
          /* Broken device. It may have been disconnected.
           * Ignore it.
           */
          tmp_list = tmp_list->next;
          continue;
        }

	  xic = state->data;
	  for (i=0; i<state->num_classes; i++)
	    {
	      if (xic->class == ValuatorClass)
		{
		  XValuatorState *xvs = (XValuatorState *)xic;
		  if ((xvs->mode & ProximityState) == InProximity)
		    {
		      new_proximity = TRUE;
		    }
		  break;
		}
	      xic = (XInputClass *)((char *)xic + xic->length);
	    }

	  XFreeDeviceState (state);
	}
      tmp_list = tmp_list->next;
    }

  display->ignore_core_events = new_proximity;
}

void
_bdk_input_configure_event (XConfigureEvent *xevent,
			    BdkWindow       *window)
{
  BdkWindowObject *priv = (BdkWindowObject *)window;
  BdkInputWindow *input_window;
  gint root_x, root_y;

  input_window = priv->input_window;
  if (input_window != NULL)
    {
      _bdk_input_get_root_relative_geometry (window, &root_x, &root_y);
      input_window->root_x = root_x;
      input_window->root_y = root_y;
    }
}

void
_bdk_input_crossing_event (BdkWindow *window,
			   gboolean enter)
{
  BdkDisplay *display = BDK_WINDOW_DISPLAY (window);
  BdkDisplayX11 *display_impl = BDK_DISPLAY_X11 (display);
  BdkWindowObject *priv = (BdkWindowObject *)window;
  BdkInputWindow *input_window;
  gint root_x, root_y;

  if (enter)
    {
      bdk_input_check_proximity(display);

      input_window = priv->input_window;
      if (input_window != NULL)
	{
	  _bdk_input_get_root_relative_geometry (window, &root_x, &root_y);
	  input_window->root_x = root_x;
	  input_window->root_y = root_y;
	}
    }
  else
    display->ignore_core_events = FALSE;
}

static BdkEventType
get_input_event_type (BdkDevicePrivate *bdkdev,
		      XEvent *xevent,
		      int *core_x, int *core_y)
{
  if (xevent->type == bdkdev->buttonpress_type)
    {
      XDeviceButtonEvent *xie = (XDeviceButtonEvent *)(xevent);
      *core_x = xie->x;
      *core_y = xie->y;
      return BDK_BUTTON_PRESS;
    }

  if (xevent->type == bdkdev->buttonrelease_type)
    {
      XDeviceButtonEvent *xie = (XDeviceButtonEvent *)(xevent);
      *core_x = xie->x;
      *core_y = xie->y;
      return BDK_BUTTON_RELEASE;
    }

  if (xevent->type == bdkdev->keypress_type)
    {
      XDeviceKeyEvent *xie = (XDeviceKeyEvent *)(xevent);
      *core_x = xie->x;
      *core_y = xie->y;
      return BDK_KEY_PRESS;
    }

  if (xevent->type == bdkdev->keyrelease_type)
    {
      XDeviceKeyEvent *xie = (XDeviceKeyEvent *)(xevent);
      *core_x = xie->x;
      *core_y = xie->y;
      return BDK_KEY_RELEASE;
    }

  if (xevent->type == bdkdev->motionnotify_type)
    {
      XDeviceMotionEvent *xie = (XDeviceMotionEvent *)(xevent);
      *core_x = xie->x;
      *core_y = xie->y;
      return BDK_MOTION_NOTIFY;
    }

  if (xevent->type == bdkdev->proximityin_type)
    {
      XProximityNotifyEvent *xie = (XProximityNotifyEvent *)(xevent);
      *core_x = xie->x;
      *core_y = xie->y;
      return BDK_PROXIMITY_IN;
    }

  if (xevent->type == bdkdev->proximityout_type)
    {
      XProximityNotifyEvent *xie = (XProximityNotifyEvent *)(xevent);
      *core_x = xie->x;
      *core_y = xie->y;
      return BDK_PROXIMITY_OUT;
    }

  *core_x = 0;
  *core_y = 0;
  return BDK_NOTHING;
}


gboolean
_bdk_input_other_event (BdkEvent *event,
			XEvent *xevent,
			BdkWindow *event_window)
{
  BdkWindow *window;
  BdkWindowObject *priv;
  BdkInputWindow *iw;
  BdkDevicePrivate *bdkdev;
  BdkEventType event_type;
  int x, y;
  BdkDisplay *display = BDK_WINDOW_DISPLAY (event_window);
  BdkDisplayX11 *display_impl = BDK_DISPLAY_X11 (display);

  /* This is a sort of a hack, as there isn't any XDeviceAnyEvent -
     but it's potentially faster than scanning through the types of
     every device. If we were deceived, then it won't match any of
     the types for the device anyways */
  bdkdev = _bdk_input_find_device (display,
				   ((XDeviceButtonEvent *)xevent)->deviceid);
  if (!bdkdev)
    return FALSE;			/* we don't handle it - not an XInput event */

  event_type = get_input_event_type (bdkdev, xevent, &x, &y);
  if (event_type == BDK_NOTHING)
    return FALSE;

  /* If we're not getting any event window its likely because we're outside the
     window and there is no grab. We should still report according to the
     implicit grab though. */
  iw = ((BdkWindowObject *)event_window)->input_window;

  if (iw->button_down_window)
    window = iw->button_down_window;
  else
    window = _bdk_window_get_input_window_for_event (event_window,
                                                     event_type,
						     /* TODO: Seems wrong, but the code used to ignore button motion handling here... */
						     0, 
                                                     x, y,
                                                     xevent->xany.serial);
  priv = (BdkWindowObject *)window;
  if (window == NULL)
    return FALSE;

  if (bdkdev->info.mode == BDK_MODE_DISABLED ||
      priv->extension_events == 0 ||
      !(bdkdev->info.has_cursor || (priv->extension_events & BDK_ALL_DEVICES_MASK)))
    return FALSE;

  if (!display->ignore_core_events && priv->extension_events != 0)
    bdk_input_check_proximity (BDK_WINDOW_DISPLAY (window));

  if (!_bdk_input_common_other_event (event, xevent, window, bdkdev))
    return FALSE;

  if (event->type == BDK_BUTTON_PRESS)
    iw->button_down_window = window;
  if (event->type == BDK_BUTTON_RELEASE && !bdkdev->button_count)
    iw->button_down_window = NULL;

  if (event->type == BDK_PROXIMITY_OUT &&
      display->ignore_core_events)
    bdk_input_check_proximity (BDK_WINDOW_DISPLAY (window));

  return _bdk_input_common_event_selected(event, window, bdkdev);
}

gint
_bdk_input_grab_pointer (BdkWindow      *window,
			 BdkWindow      *native_window, /* This is the toplevel */
			 gint            owner_events,
			 BdkEventMask    event_mask,
			 BdkWindow *     confine_to,
			 guint32         time)
{
  BdkInputWindow *input_window;
  BdkWindowObject *priv, *impl_window;
  gboolean need_ungrab;
  BdkDevicePrivate *bdkdev;
  GList *tmp_list;
  XEventClass event_classes[BDK_MAX_DEVICE_CLASSES];
  gint num_classes;
  gint result;
  BdkDisplayX11 *display_impl  = BDK_DISPLAY_X11 (BDK_WINDOW_DISPLAY (window));

  tmp_list = display_impl->input_windows;
  need_ungrab = FALSE;

  while (tmp_list)
    {
      input_window = (BdkInputWindow *)tmp_list->data;

      if (input_window->grabbed)
	{
	  input_window->grabbed = FALSE;
	  need_ungrab = TRUE;
	  break;
	}
      tmp_list = tmp_list->next;
    }

  priv = (BdkWindowObject *)window;
  impl_window = (BdkWindowObject *)_bdk_window_get_impl_window (window);
  input_window = impl_window->input_window;
  if (priv->extension_events)
    {
      g_assert (input_window != NULL);
      input_window->grabbed = TRUE;

      tmp_list = display_impl->input_devices;
      while (tmp_list)
	{
	  bdkdev = (BdkDevicePrivate *)tmp_list->data;
	  if (!BDK_IS_CORE (bdkdev) && bdkdev->xdevice)
	    {
	      _bdk_input_common_find_events (bdkdev, event_mask,
					     event_classes, &num_classes);
#ifdef G_ENABLE_DEBUG
	      if (_bdk_debug_flags & BDK_DEBUG_NOGRABS)
		result = GrabSuccess;
	      else
#endif
		result = XGrabDevice (display_impl->xdisplay, bdkdev->xdevice,
				      BDK_WINDOW_XWINDOW (native_window),
				      owner_events, num_classes, event_classes,
				      GrabModeAsync, GrabModeAsync, time);

	      /* FIXME: if failure occurs on something other than the first
		 device, things will be badly inconsistent */
	      if (result != Success)
		return result;
	    }
	  tmp_list = tmp_list->next;
	}
    }
  else
    {
      tmp_list = display_impl->input_devices;
      while (tmp_list)
	{
	  bdkdev = (BdkDevicePrivate *)tmp_list->data;
	  if (!BDK_IS_CORE (bdkdev) && bdkdev->xdevice &&
	      ((bdkdev->button_count != 0) || need_ungrab))
	    {
	      XUngrabDevice (display_impl->xdisplay, bdkdev->xdevice, time);
	      memset (bdkdev->button_state, 0, sizeof (bdkdev->button_state));
	      bdkdev->button_count = 0;
	    }

	  tmp_list = tmp_list->next;
	}
    }

  return Success;
}

void
_bdk_input_ungrab_pointer (BdkDisplay *display,
			   guint32 time)
{
  BdkInputWindow *input_window = NULL; /* Quiet GCC */
  BdkDevicePrivate *bdkdev;
  GList *tmp_list;
  BdkDisplayX11 *display_impl = BDK_DISPLAY_X11 (display);

  tmp_list = display_impl->input_windows;
  while (tmp_list)
    {
      input_window = (BdkInputWindow *)tmp_list->data;
      if (input_window->grabbed)
	break;
      tmp_list = tmp_list->next;
    }

  if (tmp_list)			/* we found a grabbed window */
    {
      input_window->grabbed = FALSE;

      tmp_list = display_impl->input_devices;
      while (tmp_list)
	{
	  bdkdev = (BdkDevicePrivate *)tmp_list->data;
	  if (!BDK_IS_CORE (bdkdev) && bdkdev->xdevice)
	    XUngrabDevice( display_impl->xdisplay, bdkdev->xdevice, time);

	  tmp_list = tmp_list->next;
	}
    }
}

#define __BDK_INPUT_XFREE_C__
#include "bdkaliasdef.c"
