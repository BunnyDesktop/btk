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

#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "bdkx.h"
#include "bdkinput.h"
#include "bdkprivate.h"
#include "bdkinputprivate.h"
#include "bdkscreen-x11.h"
#include "bdkdisplay-x11.h"
#include "bdkalias.h"

static BdkDeviceAxis bdk_input_core_axes[] = {
  { BDK_AXIS_X, 0, 0 },
  { BDK_AXIS_Y, 0, 0 }
};

void
_bdk_init_input_core (BdkDisplay *display)
{
  BdkDevicePrivate *private;

  display->core_pointer = g_object_new (BDK_TYPE_DEVICE, NULL);
  private = (BdkDevicePrivate *)display->core_pointer;

  display->core_pointer->name = "Core Pointer";
  display->core_pointer->source = BDK_SOURCE_MOUSE;
  display->core_pointer->mode = BDK_MODE_SCREEN;
  display->core_pointer->has_cursor = TRUE;
  display->core_pointer->num_axes = 2;
  display->core_pointer->axes = bdk_input_core_axes;
  display->core_pointer->num_keys = 0;
  display->core_pointer->keys = NULL;

  private->display = display;
}

static void bdk_device_class_init (BdkDeviceClass *klass);
static void bdk_device_dispose    (GObject        *object);

static gpointer bdk_device_parent_class = NULL;

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
	  (GClassInitFunc) bdk_device_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (BdkDevicePrivate),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) NULL,
	};

      object_type = g_type_register_static (G_TYPE_OBJECT,
					    g_intern_static_string ("BdkDevice"),
					    &object_info, 0);
    }

  return object_type;
}

static void
bdk_device_class_init (BdkDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  bdk_device_parent_class = g_type_class_peek_parent (klass);

  object_class->dispose  = bdk_device_dispose;
}

static void
bdk_device_dispose (GObject *object)
{
  BdkDevicePrivate *bdkdev = (BdkDevicePrivate *) object;

  if (bdkdev->display && !BDK_IS_CORE (bdkdev))
    {
#ifndef XINPUT_NONE
      if (bdkdev->xdevice)
	{
	  XCloseDevice (BDK_DISPLAY_XDISPLAY (bdkdev->display), bdkdev->xdevice);
	  bdkdev->xdevice = NULL;
	}
      g_free (bdkdev->axes);
      g_free (bdkdev->axis_data);
      bdkdev->axes = NULL;
      bdkdev->axis_data = NULL;
#endif /* !XINPUT_NONE */

      g_free (bdkdev->info.name);
      g_free (bdkdev->info.keys);
      g_free (bdkdev->info.axes);

      bdkdev->info.name = NULL;
      bdkdev->info.keys = NULL;
      bdkdev->info.axes = NULL;
    }

  G_OBJECT_CLASS (bdk_device_parent_class)->dispose (object);
}

/**
 * bdk_devices_list:
 *
 * Returns the list of available input devices for the default display.
 * The list is statically allocated and should not be freed.
 *
 * Return value: (transfer none) (element-type BdkDevice): a list of #BdkDevice
 **/
GList *
bdk_devices_list (void)
{
  return bdk_display_list_devices (bdk_display_get_default ());
}

/**
 * bdk_display_list_devices:
 * @display: a #BdkDisplay
 *
 * Returns the list of available input devices attached to @display.
 * The list is statically allocated and should not be freed.
 *
 * Return value: a list of #BdkDevice
 *
 * Since: 2.2
 **/
GList *
bdk_display_list_devices (BdkDisplay *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);

  return BDK_DISPLAY_X11 (display)->input_devices;
}

/**
 * bdk_device_get_name:
 * @device: a #BdkDevice
 *
 * Determines the name of the device.
 *
 * Return value: a name
 *
 * Since: 2.22
 **/
const gchar *
bdk_device_get_name (BdkDevice *device)
{
  g_return_val_if_fail (BDK_IS_DEVICE (device), NULL);

  return device->name;
}

/**
 * bdk_device_get_source:
 * @device: a #BdkDevice
 *
 * Determines the type of the device.
 *
 * Return value: a #BdkInputSource
 *
 * Since: 2.22
 **/
BdkInputSource
bdk_device_get_source (BdkDevice *device)
{
  g_return_val_if_fail (BDK_IS_DEVICE (device), 0);

  return device->source;
}

/**
 * bdk_device_get_mode:
 * @device: a #BdkDevice
 *
 * Determines the mode of the device.
 *
 * Return value: a #BdkInputSource
 *
 * Since: 2.22
 **/
BdkInputMode
bdk_device_get_mode (BdkDevice *device)
{
  g_return_val_if_fail (BDK_IS_DEVICE (device), 0);

  return device->mode;
}

/**
 * bdk_device_get_has_cursor:
 * @device: a #BdkDevice
 *
 * Determines whether the pointer follows device motion.
 *
 * Return value: %TRUE if the pointer follows device motion
 *
 * Since: 2.22
 **/
gboolean
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

/**
 * bdk_device_get_key:
 * @device: a #BdkDevice.
 * @index: the index of the macro button to get.
 * @keyval: return value for the keyval.
 * @modifiers: return value for modifiers.
 *
 * If @index has a valid keyval, this function will
 * fill in @keyval and @modifiers with the keyval settings.
 *
 * Since: 2.22
 **/
void
bdk_device_get_key (BdkDevice       *device,
                    guint            index,
                    guint           *keyval,
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
		    guint           index,
		    guint           keyval,
		    BdkModifierType modifiers)
{
  g_return_if_fail (device != NULL);
  g_return_if_fail (index < device->num_keys);

  device->keys[index].keyval = keyval;
  device->keys[index].modifiers = modifiers;
}

/**
 * bdk_device_get_axis_use:
 * @device: a #BdkDevice.
 * @index: the index of the axis.
 *
 * Returns the axis use for @index.
 *
 * Returns: a #BdkAxisUse specifying how the axis is used.
 *
 * Since: 2.22
 **/
BdkAxisUse
bdk_device_get_axis_use (BdkDevice *device,
                         guint      index)
{
  g_return_val_if_fail (BDK_IS_DEVICE (device), BDK_AXIS_IGNORE);
  g_return_val_if_fail (index < device->num_axes, BDK_AXIS_IGNORE);

  return device->axes[index].use;
}

/**
 * bdk_device_get_n_keys:
 * @device: a #BdkDevice.
 *
 * Gets the number of keys of a device.
 *
 * Returns: the number of keys of @device
 *
 * Since: 2.24
 **/
gint
bdk_device_get_n_keys (BdkDevice *device)
{
  g_return_val_if_fail (BDK_IS_DEVICE (device), 0);

  return device->num_keys;
}

/**
 * bdk_device_get_n_axes:
 * @device: a #BdkDevice.
 *
 * Gets the number of axes of a device.
 *
 * Returns: the number of axes of @device
 *
 * Since: 2.22
 **/
gint
bdk_device_get_n_axes (BdkDevice *device)
{
  g_return_val_if_fail (BDK_IS_DEVICE (device), 0);

  return device->num_axes;
}

void
bdk_device_set_axis_use (BdkDevice   *device,
			 guint        index,
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

static gboolean
impl_coord_in_window (BdkWindow *window,
		      int impl_x,
		      int impl_y)
{
  BdkWindowObject *priv = (BdkWindowObject *)window;

  if (impl_x < priv->abs_x ||
      impl_x > priv->abs_x + priv->width)
    return FALSE;
  if (impl_y < priv->abs_y ||
      impl_y > priv->abs_y + priv->height)
    return FALSE;
  return TRUE;
}

/**
 * bdk_device_get_history:
 * @device: a #BdkDevice
 * @window: the window with respect to which which the event coordinates will be reported
 * @start: starting timestamp for range of events to return
 * @stop: ending timestamp for the range of events to return
 * @events: (array length=n_events) (out) (transfer none): location to store a newly-allocated array of #BdkTimeCoord, or %NULL
 * @n_events: location to store the length of @events, or %NULL
 *
 * Obtains the motion history for a device; given a starting and
 * ending timestamp, return all events in the motion history for
 * the device in the given range of time. Some windowing systems
 * do not support motion history, in which case, %FALSE will
 * be returned. (This is not distinguishable from the case where
 * motion history is supported and no events were found.)
 *
 * Return value: %TRUE if the windowing system supports motion history and
 *  at least one event was found.
 **/
gboolean
bdk_device_get_history  (BdkDevice         *device,
			 BdkWindow         *window,
			 guint32            start,
			 guint32            stop,
			 BdkTimeCoord    ***events,
			 gint              *n_events)
{
  BdkTimeCoord **coords = NULL;
  BdkWindow *impl_window;
  gboolean result = FALSE;
  int tmp_n_events = 0;

  g_return_val_if_fail (BDK_WINDOW_IS_X11 (window), FALSE);

  impl_window = _bdk_window_get_impl_window (window);

  if (BDK_WINDOW_DESTROYED (window))
    /* Nothing */ ;
  else if (BDK_IS_CORE (device))
    {
      XTimeCoord *xcoords;

      xcoords = XGetMotionEvents (BDK_DRAWABLE_XDISPLAY (window),
				  BDK_DRAWABLE_XID (impl_window),
				  start, stop, &tmp_n_events);
      if (xcoords)
	{
	  BdkWindowObject *priv = (BdkWindowObject *)window;
          int i, j;

	  coords = _bdk_device_allocate_history (device, tmp_n_events);
	  j = 0;

	  for (i = 0; i < tmp_n_events; i++)
	    {
	      if (impl_coord_in_window (window, xcoords[i].x, xcoords[i].y))
		{
		  coords[j]->time = xcoords[i].time;
		  coords[j]->axes[0] = xcoords[i].x - priv->abs_x;
		  coords[j]->axes[1] = xcoords[i].y - priv->abs_y;
		  j++;
		}
	    }

	  XFree (xcoords);

          /* free the events we allocated too much */
          for (i = j; i < tmp_n_events; i++)
            {
              g_free (coords[i]);
              coords[i] = NULL;
            }

          tmp_n_events = j;

          if (tmp_n_events > 0)
            {
              result = TRUE;
            }
          else
            {
              bdk_device_free_history (coords, tmp_n_events);
              coords = NULL;
            }
	}
    }
  else
    result = _bdk_device_get_history (device, window, start, stop, &coords, &tmp_n_events);

  if (n_events)
    *n_events = tmp_n_events;

  if (events)
    *events = coords;
  else if (coords)
    bdk_device_free_history (coords, tmp_n_events);

  return result;
}

BdkTimeCoord **
_bdk_device_allocate_history (BdkDevice *device,
			      gint       n_events)
{
  BdkTimeCoord **result = g_new (BdkTimeCoord *, n_events);
  gint i;

  for (i=0; i<n_events; i++)
    result[i] = g_malloc (sizeof (BdkTimeCoord) -
			  sizeof (double) * (BDK_MAX_TIMECOORD_AXES - device->num_axes));

  return result;
}

/**
 * bdk_device_free_history:
 * @events: (inout) (transfer none): an array of #BdkTimeCoord.
 * @n_events: the length of the array.
 *
 * Frees an array of #BdkTimeCoord that was returned by bdk_device_get_history().
 */
void
bdk_device_free_history (BdkTimeCoord **events,
			 gint           n_events)
{
  gint i;

  for (i=0; i<n_events; i++)
    g_free (events[i]);

  g_free (events);
}

static void
unset_extension_events (BdkWindow *window)
{
  BdkWindowObject *window_private;
  BdkWindowObject *impl_window;
  BdkDisplayX11 *display_x11;
  BdkInputWindow *iw;

  window_private = (BdkWindowObject*) window;
  impl_window = (BdkWindowObject *)_bdk_window_get_impl_window (window);
  iw = impl_window->input_window;

  display_x11 = BDK_DISPLAY_X11 (BDK_WINDOW_DISPLAY (window));

  if (window_private->extension_events != 0)
    {
      g_assert (iw != NULL);
      g_assert (g_list_find (iw->windows, window) != NULL);

      iw->windows = g_list_remove (iw->windows, window);
      if (iw->windows == NULL)
	{
	  impl_window->input_window = NULL;
	  display_x11->input_windows = g_list_remove (display_x11->input_windows, iw);
	  g_free (iw);
	}
    }

  window_private->extension_events = 0;
}

void
bdk_input_set_extension_events (BdkWindow *window, gint mask,
				BdkExtensionMode mode)
{
  BdkWindowObject *window_private;
  BdkWindowObject *impl_window;
  BdkInputWindow *iw;
  BdkDisplayX11 *display_x11;
#ifndef XINPUT_NONE
  GList *tmp_list;
#endif

  g_return_if_fail (window != NULL);
  g_return_if_fail (BDK_WINDOW_IS_X11 (window));

  window_private = (BdkWindowObject*) window;
  display_x11 = BDK_DISPLAY_X11 (BDK_WINDOW_DISPLAY (window));
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
      if (!iw)
	{
	  iw = g_new0 (BdkInputWindow,1);

	  iw->impl_window = (BdkWindow *)impl_window;

	  iw->windows = NULL;
	  iw->grabbed = FALSE;

	  display_x11->input_windows = g_list_append (display_x11->input_windows, iw);

#ifndef XINPUT_NONE
	  /* we might not receive ConfigureNotify so get the root_relative_geometry
	   * now, just in case */
	  _bdk_input_get_root_relative_geometry (window, &iw->root_x, &iw->root_y);
#endif /* !XINPUT_NONE */
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

#ifndef XINPUT_NONE
  for (tmp_list = display_x11->input_devices; tmp_list; tmp_list = tmp_list->next)
    {
      BdkDevicePrivate *bdkdev = tmp_list->data;

      if (!BDK_IS_CORE (bdkdev))
	_bdk_input_select_events ((BdkWindow *)impl_window, bdkdev);
    }
#endif /* !XINPUT_NONE */
}

void
_bdk_input_window_destroy (BdkWindow *window)
{
  unset_extension_events (window);
}

/**
 * bdk_device_get_axis:
 * @device: a #BdkDevice
 * @axes: pointer to an array of axes
 * @use: the use to look for
 * @value: location to store the found value.
 *
 * Interprets an array of double as axis values for a given device,
 * and locates the value in the array for a given axis use.
 *
 * Return value: %TRUE if the given axis use was found, otherwise %FALSE
 **/
gboolean
bdk_device_get_axis (BdkDevice  *device,
		     gdouble    *axes,
		     BdkAxisUse  use,
		     gdouble    *value)
{
  gint i;

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

#define __BDK_INPUT_C__
#include "bdkaliasdef.c"
