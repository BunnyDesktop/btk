/* BDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1999 Tor Lillqvist
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
#include "bdkinput-directfb.h"

#include "bdkinput.h"
#include "bdkkeysyms.h"
#include "bdkalias.h"


static BdkDeviceAxis bdk_input_core_axes[] =
  {
    { BDK_AXIS_X, 0, 0 },
    { BDK_AXIS_Y, 0, 0 }
  };


BdkDevice *_bdk_core_pointer      = NULL;
GList     *_bdk_input_devices     = NULL;
gboolean   _bdk_input_ignore_core = FALSE;

int _bdk_directfb_mouse_x = 0;
int _bdk_directfb_mouse_y = 0;


void
_bdk_init_input_core (void)
{
  BdkDisplay *display = BDK_DISPLAY_OBJECT (_bdk_display);
  
  _bdk_core_pointer = g_object_new (BDK_TYPE_DEVICE, NULL);
  
  _bdk_core_pointer->name       = "Core Pointer";
  _bdk_core_pointer->source     = BDK_SOURCE_MOUSE;
  _bdk_core_pointer->mode       = BDK_MODE_SCREEN;
  _bdk_core_pointer->has_cursor = TRUE;
  _bdk_core_pointer->num_axes   = 2;
  _bdk_core_pointer->axes       = bdk_input_core_axes;
  _bdk_core_pointer->num_keys   = 0;
  _bdk_core_pointer->keys       = NULL;
  display->core_pointer         = _bdk_core_pointer;
}

static void
bdk_device_finalize (GObject *object)
{
  g_error ("A BdkDevice object was finalized. This should not happen");
}

static void
bdk_device_class_init (GObjectClass *class)
{
  class->finalize = bdk_device_finalize;
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
          (GClassInitFunc) bdk_device_class_init,
          NULL,           /* class_finalize */
          NULL,           /* class_data */
          sizeof (BdkDevice),
          0,              /* n_preallocs */
          (GInstanceInitFunc) NULL,
        };

      object_type = g_type_register_static (G_TYPE_OBJECT,
                                            "BdkDevice",
                                            &object_info, 0);
    }

  return object_type;
}

/**
 * bdk_devices_list:
 *
 * Returns the list of available input devices for the default display.
 * The list is statically allocated and should not be freed.
 *
 * Return value: a list of #BdkDevice
 **/
GList *
bdk_devices_list (void)
{
  return _bdk_input_devices;
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
bdk_display_list_devices (BdkDisplay *dpy)
{
  return _bdk_input_devices;
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
      device->axes[index].min =  0.0;
      device->axes[index].max =  0.0;
      break;
    case BDK_AXIS_XTILT:
    case BDK_AXIS_YTILT:
      device->axes[index].min = -1.0;
      device->axes[index].max =  1.0;
      break;
    default:
      device->axes[index].min =  0.0;
      device->axes[index].max =  1.0;
      break;
    }
}

gboolean
bdk_device_set_mode (BdkDevice    *device,
                     BdkInputMode  mode)
{
  g_message ("unimplemented %s", G_STRFUNC);

  return FALSE;
}

gboolean
bdk_device_get_history  (BdkDevice      *device,
                         BdkWindow      *window,
                         guint32         start,
                         guint32         stop,
                         BdkTimeCoord ***events,
                         gint           *n_events)
{
  g_return_val_if_fail (BDK_IS_WINDOW (window), FALSE);
  g_return_val_if_fail (events != NULL, FALSE);
  g_return_val_if_fail (n_events != NULL, FALSE);

  *n_events = 0;
  *events = NULL;

  if (BDK_WINDOW_DESTROYED (window))
    return FALSE;

  if (BDK_IS_CORE (device))
    return FALSE;
  else
    return FALSE;
  //TODODO_bdk_device_get_history (device, window, start, stop, events, n_events);
}

void
bdk_device_free_history (BdkTimeCoord **events,
                         gint           n_events)
{
  gint i;

  for (i = 0; i < n_events; i++)
    g_free (events[i]);

  g_free (events);
}

void
bdk_device_get_state (BdkDevice       *device,
                      BdkWindow       *window,
                      gdouble         *axes,
                      BdkModifierType *mask)
{
  g_return_if_fail (device != NULL);
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (mask)
    *mask = _bdk_directfb_modifiers;
}

void
bdk_directfb_mouse_get_info (gint            *x,
                             gint            *y,
                             BdkModifierType *mask)
{
  if (x)
    *x = _bdk_directfb_mouse_x;

  if (y)
    *y = _bdk_directfb_mouse_y;

  if (mask)
    *mask = _bdk_directfb_modifiers;
}

void
bdk_input_set_extension_events (BdkWindow        *window,
                                gint              mask,
                                BdkExtensionMode  mode)
{
  g_message ("unimplemented %s", G_STRFUNC);
}

void
bdk_device_set_key (BdkDevice       *device,
                    guint            index,
                    guint            keyval,
                    BdkModifierType  modifiers)
{
  g_return_if_fail (device != NULL);
  g_return_if_fail (index < device->num_keys);

  device->keys[index].keyval    = keyval;
  device->keys[index].modifiers = modifiers;
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

  for (i = 0; i < device->num_axes; i++)
    if (device->axes[i].use == use)
      {
	if (value)
	  *value = axes[i];
	return TRUE;
      }

  return FALSE;
}

void
_bdk_input_init (void)
{
  _bdk_init_input_core ();
  _bdk_input_devices = g_list_append (NULL, _bdk_core_pointer);
  _bdk_input_ignore_core = FALSE;
}

void
_bdk_input_exit (void)
{
  GList     *tmp_list;
  BdkDevice *bdkdev;

  for (tmp_list = _bdk_input_devices; tmp_list; tmp_list = tmp_list->next)
    {
      bdkdev = (BdkDevice *)(tmp_list->data);
      if (!BDK_IS_CORE (bdkdev))
	{
	  bdk_device_set_mode ((BdkDevice *)bdkdev, BDK_MODE_DISABLED);

	  g_free (bdkdev->name);
	  g_free (bdkdev->axes);
	  g_free (bdkdev->keys);
	  g_free (bdkdev);
	}
    }

  g_list_free (_bdk_input_devices);
}

#define __BDK_INPUT_NONE_C__
#define __BDK_INPUT_C__
#include "bdkaliasdef.c"
