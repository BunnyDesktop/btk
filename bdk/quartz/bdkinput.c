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

#import <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

#include "bdkprivate-quartz.h"
#include "bdkscreen-quartz.h"
#include "bdkinput.h"
#include "bdkprivate.h"
#include "bdkinputprivate.h"


#define N_CORE_POINTER_AXES 2
#define N_INPUT_DEVICE_AXES 5


static BdkDeviceAxis bdk_input_core_axes[] = {
  { BDK_AXIS_X, 0, 0 },
  { BDK_AXIS_Y, 0, 0 }
};

static BdkDeviceAxis bdk_quartz_pen_axes[] = {
  { BDK_AXIS_X, 0, 0 },
  { BDK_AXIS_Y, 0, 0 },
  { BDK_AXIS_PRESSURE, 0, 1 },
  { BDK_AXIS_XTILT, -1, 1 },
  { BDK_AXIS_YTILT, -1, 1 }
};

static BdkDeviceAxis bdk_quartz_cursor_axes[] = {
  { BDK_AXIS_X, 0, 0 },
  { BDK_AXIS_Y, 0, 0 },
  { BDK_AXIS_PRESSURE, 0, 1 },
  { BDK_AXIS_XTILT, -1, 1 },
  { BDK_AXIS_YTILT, -1, 1 }
};

static BdkDeviceAxis bdk_quartz_eraser_axes[] = {
  { BDK_AXIS_X, 0, 0 },
  { BDK_AXIS_Y, 0, 0 },
  { BDK_AXIS_PRESSURE, 0, 1 },
  { BDK_AXIS_XTILT, -1, 1 },
  { BDK_AXIS_YTILT, -1, 1 }
};


/* Global variables  */
static GList     *_bdk_input_windows = NULL;
static GList     *_bdk_input_devices = NULL;
static BdkDevice *_bdk_core_pointer = NULL;
static BdkDevice *_bdk_quartz_pen = NULL;
static BdkDevice *_bdk_quartz_cursor = NULL;
static BdkDevice *_bdk_quartz_eraser = NULL;
static BdkDevice *active_device = NULL;

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
        sizeof (BdkDevicePrivate),
        0,              /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };
      
      object_type = g_type_register_static (G_TYPE_OBJECT,
                                            "BdkDevice",
                                            &object_info, 0);
    }
  
  return object_type;
}

GList *
bdk_devices_list (void)
{
  return _bdk_input_devices;
}

GList *
bdk_display_list_devices (BdkDisplay *dpy)
{
  return _bdk_input_devices;
}

const gchar *
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

gboolean
bdk_device_get_has_cursor (BdkDevice *device)
{
  g_return_val_if_fail (BDK_IS_DEVICE (device), FALSE);

  return device->has_cursor;
}

void
bdk_device_set_source (BdkDevice *device,
		       BdkInputSource source)
{
  device->source = source;
}

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
#if 0
  /* Remapping axes is unsupported for now */
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
#endif
}

/**
 * bdk_input_set_device_state:
 * @device: The devices to set
 * @mask: The new button mask
 * @axes: The new axes values
 *
 * Set the state of a device's inputs for later
 * retrieval by bdk_device_get_state.
 */
static void
bdk_input_set_device_state (BdkDevice *device,
                            BdkModifierType mask,
                            gdouble *axes)
{
  BdkDevicePrivate *priv;
  gint i;

  if (device != _bdk_core_pointer)
    {
      priv = (BdkDevicePrivate *)device;
      priv->last_state = mask;

      for (i = 0; i < device->num_axes; ++i)
        priv->last_axes_state[i] = axes[i];
    }
}

void
bdk_device_get_state (BdkDevice       *device,
                      BdkWindow       *window,
                      gdouble         *axes,
                      BdkModifierType *mask)
{
  BdkDevicePrivate *priv;
  gint i;

  if (device == _bdk_core_pointer)
    {
      gint x_int, y_int;

      bdk_window_get_pointer (window, &x_int, &y_int, mask);

      if (axes)
        {
          axes[0] = x_int;
          axes[1] = y_int;
        }
    }
  else
    {
      priv = (BdkDevicePrivate *)device;

      if (mask)
        *mask = priv->last_state;

      if (axes)
        for (i = 0; i < device->num_axes; ++i)
          axes[i] = priv->last_axes_state[i];
    }
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

gboolean
bdk_device_get_history  (BdkDevice         *device,
			 BdkWindow         *window,
			 guint32            start,
			 guint32            stop,
			 BdkTimeCoord    ***events,
			 gint              *n_events)
{
  g_return_val_if_fail (window != NULL, FALSE);
  g_return_val_if_fail (BDK_WINDOW_IS_QUARTZ (window), FALSE);
  g_return_val_if_fail (events != NULL, FALSE);
  g_return_val_if_fail (n_events != NULL, FALSE);

  *n_events = 0;
  *events = NULL;
  return FALSE;
}

gboolean
bdk_device_set_mode (BdkDevice   *device,
                     BdkInputMode mode)
{
  /* FIXME: Window mode isn't supported yet */
  if (device != _bdk_core_pointer &&
      (mode == BDK_MODE_DISABLED || mode == BDK_MODE_SCREEN))
    {
      device->mode = mode;
      return TRUE;
    }

  return FALSE;
}

gint
_bdk_input_enable_window (BdkWindow *window, BdkDevicePrivate *bdkdev)
{
  return TRUE;
}

gint
_bdk_input_disable_window (BdkWindow *window, BdkDevicePrivate *bdkdev)
{
  return TRUE;
}


BdkInputWindow *
_bdk_input_window_find(BdkWindow *window)
{
  GList *tmp_list;

  for (tmp_list=_bdk_input_windows; tmp_list; tmp_list=tmp_list->next)
    if (((BdkInputWindow *)(tmp_list->data))->window == window)
      return (BdkInputWindow *)(tmp_list->data);

  return NULL;      /* Not found */
}

/* FIXME: this routine currently needs to be called between creation
   and the corresponding configure event (because it doesn't get the
   root_relative_geometry).  This should work with
   btk_window_set_extension_events, but will likely fail in other
   cases */

void
bdk_input_set_extension_events (BdkWindow *window, gint mask,
				BdkExtensionMode mode)
{
  BdkWindowObject *window_private;
  GList *tmp_list;
  BdkInputWindow *iw;

  g_return_if_fail (window != NULL);
  g_return_if_fail (BDK_WINDOW_IS_QUARTZ (window));

  window_private = (BdkWindowObject*) window;

  if (mode == BDK_EXTENSION_EVENTS_NONE)
    mask = 0;

  if (mask != 0)
    {
      iw = g_new(BdkInputWindow,1);

      iw->window = window;
      iw->mode = mode;

      iw->obscuring = NULL;
      iw->num_obscuring = 0;
      iw->grabbed = FALSE;

      _bdk_input_windows = g_list_append (_bdk_input_windows,iw);
      window_private->extension_events = mask;

      /* Add enter window events to the event mask */
      /* FIXME, this is not needed for XINPUT_NONE */
      bdk_window_set_events (window,
			     bdk_window_get_events (window) | 
			     BDK_ENTER_NOTIFY_MASK);
    }
  else
    {
      iw = _bdk_input_window_find (window);
      if (iw)
	{
	  _bdk_input_windows = g_list_remove (_bdk_input_windows,iw);
	  g_free (iw);
	}

      window_private->extension_events = 0;
    }

  for (tmp_list = _bdk_input_devices; tmp_list; tmp_list = tmp_list->next)
    {
      BdkDevicePrivate *bdkdev = (BdkDevicePrivate *)(tmp_list->data);

      if (bdkdev != (BdkDevicePrivate *)_bdk_core_pointer)
	{
	  if (mask != 0 && bdkdev->info.mode != BDK_MODE_DISABLED
	      && (bdkdev->info.has_cursor || mode == BDK_EXTENSION_EVENTS_ALL))
	    _bdk_input_enable_window (window,bdkdev);
	  else
	    _bdk_input_disable_window (window,bdkdev);
	}
    }
}

void
_bdk_input_window_destroy (BdkWindow *window)
{
  BdkInputWindow *input_window;

  input_window = _bdk_input_window_find (window);
  g_return_if_fail (input_window != NULL);

  _bdk_input_windows = g_list_remove (_bdk_input_windows,input_window);
  g_free (input_window);
}

void
_bdk_input_init (void)
{
  BdkDevicePrivate *priv;

  _bdk_core_pointer = g_object_new (BDK_TYPE_DEVICE, NULL);
  _bdk_core_pointer->name = "Core Pointer";
  _bdk_core_pointer->source = BDK_SOURCE_MOUSE;
  _bdk_core_pointer->mode = BDK_MODE_SCREEN;
  _bdk_core_pointer->has_cursor = TRUE;
  _bdk_core_pointer->num_axes = N_CORE_POINTER_AXES;
  _bdk_core_pointer->axes = bdk_input_core_axes;
  _bdk_core_pointer->num_keys = 0;
  _bdk_core_pointer->keys = NULL;

  _bdk_display->core_pointer = _bdk_core_pointer;
  _bdk_input_devices = g_list_append (NULL, _bdk_core_pointer);

  _bdk_quartz_pen = g_object_new (BDK_TYPE_DEVICE, NULL);
  _bdk_quartz_pen->name = "Quartz Pen";
  _bdk_quartz_pen->source = BDK_SOURCE_PEN;
  _bdk_quartz_pen->mode = BDK_MODE_SCREEN;
  _bdk_quartz_pen->has_cursor = TRUE;
  _bdk_quartz_pen->num_axes = N_INPUT_DEVICE_AXES;
  _bdk_quartz_pen->axes = bdk_quartz_pen_axes;
  _bdk_quartz_pen->num_keys = 0;
  _bdk_quartz_pen->keys = NULL;

  priv = (BdkDevicePrivate *)_bdk_quartz_pen;
  priv->last_axes_state = g_malloc_n (_bdk_quartz_pen->num_axes, sizeof (gdouble));

  _bdk_input_devices = g_list_append (_bdk_input_devices, _bdk_quartz_pen);

  _bdk_quartz_cursor = g_object_new (BDK_TYPE_DEVICE, NULL);
  _bdk_quartz_cursor->name = "Quartz Cursor";
  _bdk_quartz_cursor->source = BDK_SOURCE_CURSOR;
  _bdk_quartz_cursor->mode = BDK_MODE_SCREEN;
  _bdk_quartz_cursor->has_cursor = TRUE;
  _bdk_quartz_cursor->num_axes = N_INPUT_DEVICE_AXES;
  _bdk_quartz_cursor->axes = bdk_quartz_cursor_axes;
  _bdk_quartz_cursor->num_keys = 0;
  _bdk_quartz_cursor->keys = NULL;

  priv = (BdkDevicePrivate *)_bdk_quartz_cursor;
  priv->last_axes_state = g_malloc_n (_bdk_quartz_cursor->num_axes, sizeof (gdouble));

  _bdk_input_devices = g_list_append (_bdk_input_devices, _bdk_quartz_cursor);

  _bdk_quartz_eraser = g_object_new (BDK_TYPE_DEVICE, NULL);
  _bdk_quartz_eraser->name = "Quartz Eraser";
  _bdk_quartz_eraser->source = BDK_SOURCE_ERASER;
  _bdk_quartz_eraser->mode = BDK_MODE_SCREEN;
  _bdk_quartz_eraser->has_cursor = TRUE;
  _bdk_quartz_eraser->num_axes = N_INPUT_DEVICE_AXES;
  _bdk_quartz_eraser->axes = bdk_quartz_eraser_axes;
  _bdk_quartz_eraser->num_keys = 0;
  _bdk_quartz_eraser->keys = NULL;

  priv = (BdkDevicePrivate *)_bdk_quartz_eraser;
  priv->last_axes_state = g_malloc_n (_bdk_quartz_eraser->num_axes, sizeof (gdouble));

  _bdk_input_devices = g_list_append (_bdk_input_devices, _bdk_quartz_eraser);

  active_device = _bdk_core_pointer;
}

void
_bdk_input_exit (void)
{
  GList *tmp_list;
  BdkDevicePrivate *bdkdev;

  for (tmp_list = _bdk_input_devices; tmp_list; tmp_list = tmp_list->next)
    {
      bdkdev = (BdkDevicePrivate *)(tmp_list->data);
      if (bdkdev != (BdkDevicePrivate *)_bdk_core_pointer)
        {
          bdk_device_set_mode ((BdkDevice *)bdkdev, BDK_MODE_DISABLED);

          g_free (bdkdev->info.name);
          g_free (bdkdev->info.axes);
          g_free (bdkdev->info.keys);
          g_free (bdkdev->last_axes_state);
          g_free (bdkdev);
        }
    }

  g_list_free (_bdk_input_devices);

  for (tmp_list = _bdk_input_windows; tmp_list; tmp_list = tmp_list->next)
    {
      g_free (tmp_list->data);
    }
  g_list_free (_bdk_input_windows);
}

gboolean
bdk_device_get_axis (BdkDevice *device, gdouble *axes, BdkAxisUse use, gdouble *value)
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
_bdk_input_window_crossing (BdkWindow *window,
                            gboolean   enter)
{
}

/**
 * _bdk_input_quartz_tablet_proximity:
 * @deviceType: The result of [nsevent pointingDeviceType]
 *
 * Update the current active device based on a proximity event.
 */
void
_bdk_input_quartz_tablet_proximity (NSPointingDeviceType deviceType)
{
  if (deviceType == NSPenPointingDevice)
    active_device = _bdk_quartz_pen;
  else if (deviceType == NSCursorPointingDevice)
    active_device = _bdk_quartz_cursor;
  else if (deviceType == NSEraserPointingDevice)
    active_device = _bdk_quartz_eraser;
  else
    active_device = _bdk_core_pointer;
}

/**
 * _bdk_input_fill_quartz_input_event:
 * @event: The BDK mouse event.
 * @nsevent: The NSEvent that generated the mouse event.
 * @input_event: (out): Return location for the input event.
 *
 * Handle extended input for the passed event, the BdkEvent object
 * passed in should be a filled mouse button or motion event.
 *
 * Return value: %TRUE if an extended input event was generated.
 */
gboolean
_bdk_input_fill_quartz_input_event (BdkEvent *event,
                                    NSEvent  *nsevent,
                                    BdkEvent *input_event)
{
  gdouble *axes;
  gint x, y;
  gint x_target, y_target;
  gdouble x_root, y_root;
  gint state;
  BdkInputWindow *iw;
  BdkWindow *target_window;
  BdkScreenQuartz *screen_quartz;

  if ([nsevent subtype] == NSTabletProximityEventSubtype)
    {
      _bdk_input_quartz_tablet_proximity ([nsevent pointingDeviceType]);
    }
  else if (([nsevent subtype] != NSTabletPointEventSubtype) ||
           (active_device == _bdk_core_pointer) ||
           (active_device->mode == BDK_MODE_DISABLED))
    {
      _bdk_display->ignore_core_events = FALSE;
      return FALSE;
    }

  switch (event->any.type)
    {
      case BDK_MOTION_NOTIFY:
        x = event->motion.x;
        y = event->motion.y;
        state = event->motion.state;
        break;
      case BDK_BUTTON_PRESS:
      case BDK_BUTTON_RELEASE:
        x = event->button.x;
        y = event->button.y;
        state = event->button.state;
        break;
      default:
        /* Not an input related event */
        return FALSE;
        break;
    }

  /* Input events won't be propagated through windows that aren't listening
   * for input events, so _bdk_window_get_input_window_for_event finds the
   * window to directly send the event to.
   */
  target_window = _bdk_window_get_input_window_for_event (event->any.window,
                                                          event->any.type,
                                                          0, x, y, 0);

  iw = _bdk_input_window_find (target_window);

  if (!iw)
    {
      /* Return if the target window doesn't have extended events enabled or
       * hasn't asked for this type of event.
       */
      _bdk_display->ignore_core_events = FALSE;
      return FALSE;
    }

  /* The cursor is inside an extended events window, block propagation of the
   * core motion / button events
   */
  _bdk_display->ignore_core_events = TRUE;

  axes = g_malloc_n (N_INPUT_DEVICE_AXES, sizeof (gdouble));

  bdk_window_get_origin (target_window, &x_target, &y_target);

  /* Equation for root x & y taken from _bdk_quartz_window_xy_to_bdk_xy
   * recalculated here to get doubles instead of ints.
   */
  screen_quartz = BDK_SCREEN_QUARTZ (_bdk_screen);
  x_root = [NSEvent mouseLocation].x - screen_quartz->min_x;
  y_root = screen_quartz->height - [NSEvent mouseLocation].y + screen_quartz->min_y;

  axes[0] = x_root - x_target;
  axes[1] = y_root - y_target;
  axes[2] = [nsevent pressure];
  axes[3] = [nsevent tilt].x;
  axes[4] = [nsevent tilt].y;

  bdk_input_set_device_state (active_device, state, axes);

  input_event->any.window     = target_window;
  input_event->any.type       = event->any.type;
  input_event->any.send_event = event->any.send_event;

  switch (event->any.type)
    {
      case BDK_MOTION_NOTIFY:
        input_event->motion.device = active_device;
        input_event->motion.x      = axes[0];
        input_event->motion.y      = axes[1];
        input_event->motion.axes   = axes;
        input_event->motion.x_root = x_root;
        input_event->motion.y_root = y_root;

        input_event->motion.time    = event->motion.time;
        input_event->motion.state   = event->motion.state;
        input_event->motion.is_hint = event->motion.is_hint;
        break;
      case BDK_BUTTON_PRESS:
      case BDK_BUTTON_RELEASE:
        input_event->button.device = active_device;
        input_event->button.x      = axes[0];
        input_event->button.y      = axes[1];
        input_event->button.axes   = axes;
        input_event->button.x_root = x_root;
        input_event->button.y_root = y_root;

        input_event->button.time   = event->button.time;
        input_event->button.state  = event->button.state;
        input_event->button.button = event->button.button;
        break;
      default:
        return FALSE;
        break;
    }

  return TRUE;
}
