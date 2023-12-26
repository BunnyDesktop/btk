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

/* This file should really be one level up, in the backend-independent
 * BDK, and the x11/bdkinput.c could also be removed.
 * 
 * That stuff in x11/bdkinput.c which really *is* X11-dependent should
 * be in x11/bdkinput-x11.c.
 */

#include "config.h"

#include "bdkdisplay.h"
#include "bdkinput.h"

#include "bdkprivate-win32.h"
#include "bdkinput-win32.h"

static BdkDeviceAxis bdk_input_core_axes[] = {
  { BDK_AXIS_X, 0, 0 },
  { BDK_AXIS_Y, 0, 0 }
};

/* Global variables  */

GList            *_bdk_input_devices;
GList            *_bdk_input_windows;
bboolean          _bdk_input_in_proximity = 0;
bboolean          _bdk_input_inside_input_window = 0;

void
_bdk_init_input_core (BdkDisplay *display)
{
  display->core_pointer = g_object_new (BDK_TYPE_DEVICE, NULL);
  
  display->core_pointer->name = "Core Pointer";
  display->core_pointer->source = BDK_SOURCE_MOUSE;
  display->core_pointer->mode = BDK_MODE_SCREEN;
  display->core_pointer->has_cursor = TRUE;
  display->core_pointer->num_axes = 2;
  display->core_pointer->axes = bdk_input_core_axes;
  display->core_pointer->num_keys = 0;
  display->core_pointer->keys = NULL;
}

GType
bdk_device_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      const GTypeInfo object_info =
	{
	  sizeof (BdkDeviceClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) NULL,
	  NULL,			/* class_finalize */
	  NULL,			/* class_data */
	  sizeof (BdkDevicePrivate),
	  0,			/* n_preallocs */
	  (GInstanceInitFunc) NULL,
	};
      
      object_type = g_type_register_static (B_TYPE_OBJECT,
                                            g_intern_static_string ("BdkDevice"),
                                            &object_info, 0);
    }
  
  return object_type;
}

GList *
bdk_devices_list (void)
{
  return bdk_display_list_devices (_bdk_display);
}

GList *
bdk_display_list_devices (BdkDisplay *dpy)
{
  g_return_val_if_fail (dpy == _bdk_display, NULL);

  _bdk_input_wintab_init_check ();
  return _bdk_input_devices;
}

const bchar *
bdk_device_get_name (BdkDevice *device)
{
  g_return_val_if_fail (BDK_IS_DEVICE (device), NULL);

  return device->name;
}

BdkInputSource
bdk_device_get_source (BdkDevice *device)
{
  g_return_val_if_fail (BDK_IS_DEVICE (device), 0);

  return device->source;
}

BdkInputMode
bdk_device_get_mode (BdkDevice *device)
{
  g_return_val_if_fail (BDK_IS_DEVICE (device), 0);

  return device->mode;
}

bboolean
bdk_device_get_has_cursor (BdkDevice *device)
{
  g_return_val_if_fail (BDK_IS_DEVICE (device), FALSE);

  return device->has_cursor;
}

void
bdk_device_set_source (BdkDevice      *device,
		       BdkInputSource  source)
{
  g_return_if_fail (device != NULL);

  device->source = source;
}

void
bdk_device_get_key (BdkDevice       *device,
                    buint            index,
                    buint           *keyval,
                    BdkModifierType *modifiers)
{
  g_return_if_fail (BDK_IS_DEVICE (device));
  g_return_if_fail (index < device->num_keys);

  if (!device->keys[index].keyval &&
      !device->keys[index].modifiers)
    return;

  if (keyval)
    *keyval = device->keys[index].keyval;

  if (modifiers)
    *modifiers = device->keys[index].modifiers;
}

void
bdk_device_set_key (BdkDevice      *device,
		    buint           index,
		    buint           keyval,
		    BdkModifierType modifiers)
{
  g_return_if_fail (device != NULL);
  g_return_if_fail (index < device->num_keys);

  device->keys[index].keyval = keyval;
  device->keys[index].modifiers = modifiers;
}

BdkAxisUse
bdk_device_get_axis_use (BdkDevice *device,
                         buint      index)
{
  g_return_val_if_fail (BDK_IS_DEVICE (device), BDK_AXIS_IGNORE);
  g_return_val_if_fail (index < device->num_axes, BDK_AXIS_IGNORE);

  return device->axes[index].use;
}

bint
bdk_device_get_n_keys (BdkDevice *device)
{
  g_return_val_if_fail (BDK_IS_DEVICE (device), 0);

  return device->num_keys;
}

bint
bdk_device_get_n_axes (BdkDevice *device)
{
  g_return_val_if_fail (BDK_IS_DEVICE (device), 0);

  return device->num_axes;
}

void
bdk_device_set_axis_use (BdkDevice   *device,
			 buint        index,
			 BdkAxisUse   use)
{
  g_return_if_fail (device != NULL);
  g_return_if_fail (index < device->num_axes);

  device->axes[index].use = use;

  switch (use)
    {
    case BDK_AXIS_X:
    case BDK_AXIS_Y:
      device->axes[index].min = 0.;
      device->axes[index].max = 0.;
      break;
    case BDK_AXIS_XTILT:
    case BDK_AXIS_YTILT:
      device->axes[index].min = -1.;
      device->axes[index].max = 1;
      break;
    default:
      device->axes[index].min = 0.;
      device->axes[index].max = 1;
      break;
    }
}

bboolean
bdk_device_get_history  (BdkDevice         *device,
			 BdkWindow         *window,
			 buint32            start,
			 buint32            stop,
			 BdkTimeCoord    ***events,
			 bint              *n_events)
{
  g_return_val_if_fail (window != NULL, FALSE);
  g_return_val_if_fail (BDK_IS_WINDOW (window), FALSE);
  g_return_val_if_fail (events != NULL, FALSE);
  g_return_val_if_fail (n_events != NULL, FALSE);

  if (n_events)
    *n_events = 0;
  if (events)
    *events = NULL;

  if (BDK_WINDOW_DESTROYED (window))
    return FALSE;
    
  if (BDK_IS_CORE (device))
    return FALSE;
  else
    return _bdk_device_get_history (device, window, start, stop, events, n_events);
}

BdkTimeCoord ** 
_bdk_device_allocate_history (BdkDevice *device,
			      bint       n_events)
{
  BdkTimeCoord **result = g_new (BdkTimeCoord *, n_events);
  bint i;

  for (i=0; i<n_events; i++)
    result[i] = g_malloc (sizeof (BdkTimeCoord) -
			  sizeof (double) * (BDK_MAX_TIMECOORD_AXES - device->num_axes));

  return result;
}

void 
bdk_device_free_history (BdkTimeCoord **events,
			 bint           n_events)
{
  bint i;
  
  for (i=0; i<n_events; i++)
    g_free (events[i]);

  g_free (events);
}

/* FIXME: this routine currently needs to be called between creation
   and the corresponding configure event (because it doesn't get the
   root_relative_geometry).  This should work with
   btk_window_set_extension_events, but will likely fail in other
   cases */

static void
unset_extension_events (BdkWindow *window)
{
  BdkWindowObject *window_private;
  BdkWindowObject *impl_window;
  BdkInputWindow *iw;

  window_private = (BdkWindowObject*) window;
  impl_window = (BdkWindowObject *)_bdk_window_get_impl_window (window);
  iw = impl_window->input_window;

  if (window_private->extension_events != 0)
    {
      g_assert (iw != NULL);
      g_assert (g_list_find (iw->windows, window) != NULL);

      iw->windows = g_list_remove (iw->windows, window);
      if (iw->windows == NULL)
	{
	  impl_window->input_window = NULL;
	  _bdk_input_windows = g_list_remove(_bdk_input_windows,iw);
	  g_free (iw);
	}
    }

  window_private->extension_events = 0;
}

static void
bdk_input_get_root_relative_geometry (HWND w,
				      int  *x_ret,
				      int  *y_ret)
{
  POINT pt;

  pt.x = 0;
  pt.y = 0;
  ClientToScreen (w, &pt);

  if (x_ret)
    *x_ret = pt.x + _bdk_offset_x;
  if (y_ret)
    *y_ret = pt.y + _bdk_offset_y;
}

void
bdk_input_set_extension_events (BdkWindow *window, bint mask,
				BdkExtensionMode mode)
{
  BdkWindowObject *window_private;
  BdkWindowObject *impl_window;
  BdkInputWindow *iw;

  g_return_if_fail (window != NULL);
  g_return_if_fail (BDK_IS_WINDOW (window));

  window_private = (BdkWindowObject*) window;
  if (BDK_WINDOW_DESTROYED (window))
    return;

  impl_window = (BdkWindowObject *)_bdk_window_get_impl_window (window);

  if (mode == BDK_EXTENSION_EVENTS_ALL && mask != 0)
    mask |= BDK_ALL_DEVICES_MASK;

  if (mode == BDK_EXTENSION_EVENTS_NONE)
    mask = 0;

  iw = impl_window->input_window;

  if (mask != 0)
    {
      _bdk_input_wintab_init_check ();

      if (!iw)
	{
	  iw = g_new0 (BdkInputWindow,1);

	  iw->impl_window = (BdkWindow *)impl_window;

	  iw->windows = NULL;

	  _bdk_input_windows = g_list_append(_bdk_input_windows, iw);

	  bdk_input_get_root_relative_geometry (BDK_WINDOW_HWND (window), &iw->root_x, &iw->root_y);

	  impl_window->input_window = iw;
	}

      if (window_private->extension_events == 0)
	iw->windows = g_list_append (iw->windows, window);
      window_private->extension_events = mask;
    }
  else
    {
      unset_extension_events (window);
    }

  _bdk_input_select_events ((BdkWindow *)impl_window);
}

void
_bdk_input_window_destroy (BdkWindow *window)
{
  unset_extension_events (window);
}

void
_bdk_input_check_proximity (void)
{
  GList *l;
  bboolean new_proximity = FALSE;

  if (!_bdk_input_inside_input_window)
    {
      _bdk_display->ignore_core_events = FALSE;
      return;
    }

  for (l = _bdk_input_devices; l != NULL; l = l->next)
    {
      BdkDevicePrivate *bdkdev = l->data;

      if (bdkdev->info.mode != BDK_MODE_DISABLED &&
	  !BDK_IS_CORE (bdkdev))
	{
	  if (_bdk_input_in_proximity)
	    {
	      new_proximity = TRUE;
	      break;
	    }
	}
    }

  _bdk_display->ignore_core_events = new_proximity;
}

void
_bdk_input_crossing_event (BdkWindow *window,
			   bboolean enter)
{
  if (enter)
    {
      BdkWindowObject *priv = (BdkWindowObject *)window;
      BdkInputWindow *input_window;
      bint root_x, root_y;

      _bdk_input_inside_input_window = TRUE;

      input_window = priv->input_window;
      if (input_window != NULL)
	{
	  bdk_input_get_root_relative_geometry (BDK_WINDOW_HWND (window), 
						&root_x, &root_y);
	  input_window->root_x = root_x;
	  input_window->root_y = root_y;
	}
    }
  else
    {
      _bdk_input_inside_input_window = FALSE;
    }

  _bdk_input_check_proximity ();
}

bboolean
bdk_device_get_axis (BdkDevice  *device,
		     bdouble    *axes,
		     BdkAxisUse  use,
		     bdouble    *value)
{
  bint i;
  
  g_return_val_if_fail (device != NULL, FALSE);

  if (axes == NULL)
    return FALSE;
  
  for (i=0; i<device->num_axes; i++)
    if (device->axes[i].use == use)
      {
	if (value)
	  *value = axes[i];
	return TRUE;
      }
  
  return FALSE;
}

bboolean
bdk_device_set_mode (BdkDevice   *device,
		     BdkInputMode mode)
{
  GList *tmp_list;
  BdkDevicePrivate *bdkdev;
  BdkInputWindow *input_window;

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

  for (tmp_list = _bdk_input_windows; tmp_list; tmp_list = tmp_list->next)
    {
      input_window = (BdkInputWindow *)tmp_list->data;
      _bdk_input_select_events (input_window->impl_window);
    }

  if (!BDK_IS_CORE (bdkdev))
    _bdk_input_update_for_device_mode (bdkdev);

  return TRUE;
}
