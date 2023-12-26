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
#include "bdkinputprivate.h"
#include "bdkinternals.h"
#include "bdkx.h"
#include "bdk.h"		/* For bdk_error_trap_push()/pop() */
#include "bdkdisplay-x11.h"
#include "bdkalias.h"

#include <string.h>

/* Forward declarations */
static BdkDevicePrivate *bdk_input_device_new            (BdkDisplay       *display,
							  XDeviceInfo      *device,
							  gint              include_core);
static void              bdk_input_translate_coordinates (BdkDevicePrivate *bdkdev,
							  BdkWindow        *window,
							  gint             *axis_data,
							  gdouble          *axis_out,
							  gdouble          *x_out,
							  gdouble          *y_out);
static void              bdk_input_update_axes           (BdkDevicePrivate *bdkdev,
							  gint             axes_count,
							  gint             first_axis,
							  gint             *axis_data);
static guint             bdk_input_translate_state       (guint             state,
							  guint             device_state);

/* A temporary error handler for ignoring device unplugging-related errors. */
static int
ignore_errors (Display *display, XErrorEvent *event)
{
  return True;
}

BdkDevicePrivate *
_bdk_input_find_device (BdkDisplay *display,
			guint32     id)
{
  GList *tmp_list = BDK_DISPLAY_X11 (display)->input_devices;
  BdkDevicePrivate *bdkdev;
  while (tmp_list)
    {
      bdkdev = (BdkDevicePrivate *)(tmp_list->data);
      if (bdkdev->deviceid == id)
	return bdkdev;
      tmp_list = tmp_list->next;
    }
  return NULL;
}

void
_bdk_input_get_root_relative_geometry (BdkWindow *window,
				       int *x_ret, int *y_ret)
{
  Window child;
  gint x,y;

  XTranslateCoordinates (BDK_WINDOW_XDISPLAY (window),
			 BDK_WINDOW_XWINDOW (window),
			 BDK_WINDOW_XROOTWIN (window),
			 0, 0, &x, &y, &child);

  if (x_ret)
    *x_ret = x;
  if (y_ret)
    *y_ret = y;
}

static BdkDevicePrivate *
bdk_input_device_new (BdkDisplay  *display,
		      XDeviceInfo *device,
		      gint         include_core)
{
  BdkDevicePrivate *bdkdev;
  gchar *tmp_name;
  XAnyClassPtr class;
  gint i,j;

  bdkdev = g_object_new (BDK_TYPE_DEVICE, NULL);

  bdkdev->deviceid = device->id;
  bdkdev->display = display;

  if (device->name[0])
    bdkdev->info.name = g_strdup (device->name);
 else
   /* XFree86 3.2 gives an empty name to the default core devices,
      (fixed in 3.2A) */
   bdkdev->info.name = g_strdup ("pointer");

  bdkdev->info.mode = BDK_MODE_DISABLED;

  /* Try to figure out what kind of device this is by its name -
     could invite a very, very, long list... Lowercase name
     for comparison purposes */

  tmp_name = g_ascii_strdown (bdkdev->info.name, -1);

  if (strstr (tmp_name, "eraser"))
    bdkdev->info.source = BDK_SOURCE_ERASER;
  else if (strstr (tmp_name, "cursor"))
    bdkdev->info.source = BDK_SOURCE_CURSOR;
  else if (strstr (tmp_name, "wacom") ||
	   strstr (tmp_name, "pen"))
    bdkdev->info.source = BDK_SOURCE_PEN;
  else
    bdkdev->info.source = BDK_SOURCE_MOUSE;

  g_free(tmp_name);

  bdkdev->xdevice = NULL;

  /* step through the classes */

  bdkdev->info.num_axes = 0;
  bdkdev->info.num_keys = 0;
  bdkdev->info.axes = NULL;
  bdkdev->info.keys = NULL;
  bdkdev->axes = 0;
  bdkdev->info.has_cursor = 0;
  bdkdev->needs_update = FALSE;
  bdkdev->claimed = FALSE;
  memset(bdkdev->button_state, 0, sizeof (bdkdev->button_state));
  bdkdev->button_count = 0;

  class = device->inputclassinfo;
  for (i=0;i<device->num_classes;i++)
    {
      switch (class->class) {
      case ButtonClass:
	break;
      case KeyClass:
	{
	  XKeyInfo *xki = (XKeyInfo *)class;
	  /* Hack to catch XFree86 3.3.1 bug. Other devices better
	   * not have exactly 25 keys...
	   */
	  if ((xki->min_keycode == 8) && (xki->max_keycode == 32))
	    {
	      bdkdev->info.num_keys = 32;
	      bdkdev->min_keycode = 1;
	    }
	  else
	    {
	      bdkdev->info.num_keys = xki->max_keycode - xki->min_keycode + 1;
	      bdkdev->min_keycode = xki->min_keycode;
	    }
	  bdkdev->info.keys = g_new (BdkDeviceKey, bdkdev->info.num_keys);

	  for (j=0; j<bdkdev->info.num_keys; j++)
	    {
	      bdkdev->info.keys[j].keyval = 0;
	      bdkdev->info.keys[j].modifiers = 0;
	    }

	  break;
	}
      case ValuatorClass:
	{
	  XValuatorInfo *xvi = (XValuatorInfo *)class;
	  bdkdev->info.num_axes = xvi->num_axes;
	  bdkdev->axes = g_new (BdkAxisInfo, xvi->num_axes);
	  bdkdev->axis_data = g_new0 (gint, xvi->num_axes);
	  bdkdev->info.axes = g_new0 (BdkDeviceAxis, xvi->num_axes);
	  for (j=0;j<xvi->num_axes;j++)
	    {
	      bdkdev->axes[j].resolution =
		bdkdev->axes[j].xresolution = xvi->axes[j].resolution;
	      bdkdev->axes[j].min_value =
		bdkdev->axes[j].xmin_value = xvi->axes[j].min_value;
	      bdkdev->axes[j].max_value =
		bdkdev->axes[j].xmax_value = xvi->axes[j].max_value;
	      bdkdev->info.axes[j].use = BDK_AXIS_IGNORE;
	    }
	  j=0;
	  if (j<xvi->num_axes)
	    bdk_device_set_axis_use (&bdkdev->info, j++, BDK_AXIS_X);
	  if (j<xvi->num_axes)
	    bdk_device_set_axis_use (&bdkdev->info, j++, BDK_AXIS_Y);
	  if (j<xvi->num_axes)
	    bdk_device_set_axis_use (&bdkdev->info, j++, BDK_AXIS_PRESSURE);
	  if (j<xvi->num_axes)
	    bdk_device_set_axis_use (&bdkdev->info, j++, BDK_AXIS_XTILT);
	  if (j<xvi->num_axes)
	    bdk_device_set_axis_use (&bdkdev->info, j++, BDK_AXIS_YTILT);
	  if (j<xvi->num_axes)
	    bdk_device_set_axis_use (&bdkdev->info, j++, BDK_AXIS_WHEEL);

	  break;
	}
      }
      class = (XAnyClassPtr)(((char *)class) + class->length);
    }
  /* return NULL if no axes */
  if (!bdkdev->info.num_axes || !bdkdev->axes ||
      (!include_core && device->use == IsXPointer))
    goto error;

  if (device->use != IsXPointer)
    {
      bdk_error_trap_push ();
      bdkdev->xdevice = XOpenDevice (BDK_DISPLAY_XDISPLAY (display),
				     bdkdev->deviceid);

      /* return NULL if device is not ready */
      if (bdk_error_trap_pop ())
	goto error;
    }

  bdkdev->buttonpress_type = 0;
  bdkdev->buttonrelease_type = 0;
  bdkdev->keypress_type = 0;
  bdkdev->keyrelease_type = 0;
  bdkdev->motionnotify_type = 0;
  bdkdev->proximityin_type = 0;
  bdkdev->proximityout_type = 0;
  bdkdev->changenotify_type = 0;

  return bdkdev;

 error:

  g_object_unref (bdkdev);

  return NULL;
}

void
_bdk_input_common_find_events (BdkDevicePrivate *bdkdev,
			       gint mask,
			       XEventClass *classes,
			       int *num_classes)
{
  gint i;
  XEventClass class;

  i = 0;
  if (mask & BDK_BUTTON_PRESS_MASK)
    {
      DeviceButtonPress (bdkdev->xdevice, bdkdev->buttonpress_type,
			     class);
      if (class != 0)
	  classes[i++] = class;
      DeviceButtonPressGrab (bdkdev->xdevice, 0, class);
      if (class != 0)
	  classes[i++] = class;
    }
  if (mask & BDK_BUTTON_RELEASE_MASK)
    {
      DeviceButtonRelease (bdkdev->xdevice, bdkdev->buttonrelease_type,
			   class);
      if (class != 0)
	  classes[i++] = class;
    }
  if (mask & (BDK_POINTER_MOTION_MASK |
	      BDK_BUTTON1_MOTION_MASK | BDK_BUTTON2_MOTION_MASK |
	      BDK_BUTTON3_MOTION_MASK | BDK_BUTTON_MOTION_MASK))
    {
      DeviceMotionNotify (bdkdev->xdevice, bdkdev->motionnotify_type, class);
      if (class != 0)
	  classes[i++] = class;
      DeviceStateNotify (bdkdev->xdevice, bdkdev->devicestatenotify_type, class);
      if (class != 0)
	  classes[i++] = class;
    }
  if (mask & BDK_KEY_PRESS_MASK)
    {
      DeviceKeyPress (bdkdev->xdevice, bdkdev->keypress_type, class);
      if (class != 0)
	  classes[i++] = class;
    }
  if (mask & BDK_KEY_RELEASE_MASK)
    {
      DeviceKeyRelease (bdkdev->xdevice, bdkdev->keyrelease_type, class);
      if (class != 0)
	  classes[i++] = class;
    }
  if (mask & BDK_PROXIMITY_IN_MASK)
    {
      ProximityIn   (bdkdev->xdevice, bdkdev->proximityin_type, class);
      if (class != 0)
	  classes[i++] = class;
    }
  if (mask & BDK_PROXIMITY_OUT_MASK)
    {
      ProximityOut  (bdkdev->xdevice, bdkdev->proximityout_type, class);
      if (class != 0)
	  classes[i++] = class;
    }

  *num_classes = i;
}

void
_bdk_input_select_events (BdkWindow *impl_window,
			  BdkDevicePrivate *bdkdev)
{
  int (*old_handler) (Display *, XErrorEvent *);
  XEventClass classes[BDK_MAX_DEVICE_CLASSES];
  gint num_classes;
  guint event_mask;
  BdkWindowObject *w;
  BdkInputWindow *iw;
  GList *l;

  event_mask = 0;
  iw = ((BdkWindowObject *)impl_window)->input_window;

  if (bdkdev->info.mode != BDK_MODE_DISABLED &&
      iw != NULL)
    {
      for (l = iw->windows; l != NULL; l = l->next)
	{
	  w = l->data;
	  if (bdkdev->info.has_cursor || (w->extension_events & BDK_ALL_DEVICES_MASK))
	    event_mask |= w->extension_events;
	}
    }
  event_mask &= ~BDK_ALL_DEVICES_MASK;

  if (event_mask)
    event_mask |= BDK_PROXIMITY_OUT_MASK | BDK_BUTTON_PRESS_MASK | BDK_BUTTON_RELEASE_MASK;

  _bdk_input_common_find_events (bdkdev, event_mask,
				 classes, &num_classes);

  /* From X11 doc:
   * "XSelectExtensionEvent can generate a BadWindow or BadClass error."
   * In particular when a device is unplugged, a requested event class
   * could no longer be valid and raise a BadClass, which would cause
   * the program to crash.
   *
   * To handle this case gracefully, we simply ignore XSelectExtensionEvent() errors.
   * This is OK since there is no events to report for the unplugged device anyway.
   * So simply the device remains "silent".
   */
  old_handler = XSetErrorHandler (ignore_errors);
  XSelectExtensionEvent (BDK_WINDOW_XDISPLAY (impl_window),
			 BDK_WINDOW_XWINDOW (impl_window),
			 classes, num_classes);
  XSetErrorHandler (old_handler);
}

gint
_bdk_input_common_init (BdkDisplay *display,
			gint        include_core)
{
  XDeviceInfo   *devices;
  int num_devices, loop;
  int ignore, event_base;
  BdkDisplayX11 *display_x11 = BDK_DISPLAY_X11 (display);

  /* Init XInput extension */

  display_x11->input_devices = NULL;
  if (XQueryExtension (display_x11->xdisplay, "XInputExtension",
		       &ignore, &event_base, &ignore))
    {
      bdk_x11_register_standard_event_type (display,
					    event_base, 15 /* Number of events */);

      devices = XListInputDevices(display_x11->xdisplay, &num_devices);
      if (devices)
	{
	  for(loop=0; loop<num_devices; loop++)
	    {
	      BdkDevicePrivate *bdkdev = bdk_input_device_new(display,
							      &devices[loop],
							      include_core);
	      if (bdkdev)
		display_x11->input_devices = g_list_append(display_x11->input_devices, bdkdev);
	    }
	  XFreeDeviceList(devices);
	}
    }

  display_x11->input_devices = g_list_append (display_x11->input_devices, display->core_pointer);

  return TRUE;
}

static void
bdk_input_update_axes (BdkDevicePrivate *bdkdev,
		       gint             axes_count,
		       gint             first_axis,
		       gint             *axis_data)
{
  int i;
  g_return_if_fail (first_axis >= 0 && first_axis + axes_count <= bdkdev->info.num_axes);

  for (i = 0; i < axes_count; i++)
    bdkdev->axis_data[first_axis + i] = axis_data[i];
}

static void
bdk_input_translate_coordinates (BdkDevicePrivate *bdkdev,
				 BdkWindow        *window,
				 gint             *axis_data,
				 gdouble          *axis_out,
				 gdouble          *x_out,
				 gdouble          *y_out)
{
  BdkWindowObject *priv, *impl_window;

  int i;
  int x_axis = 0;
  int y_axis = 0;

  double device_width, device_height, x_min, y_min;
  double x_offset, y_offset, x_scale, y_scale;

  priv = (BdkWindowObject *) window;
  impl_window = (BdkWindowObject *)_bdk_window_get_impl_window (window);

  for (i=0; i<bdkdev->info.num_axes; i++)
    {
      switch (bdkdev->info.axes[i].use)
	{
	case BDK_AXIS_X:
	  x_axis = i;
	  break;
	case BDK_AXIS_Y:
	  y_axis = i;
	  break;
	default:
	  break;
	}
    }

  device_width = bdkdev->axes[x_axis].max_value - bdkdev->axes[x_axis].min_value;
  if (device_width > 0)
    {
      x_min = bdkdev->axes[x_axis].min_value;
    }
  else
    {
      device_width = bdk_screen_get_width (bdk_drawable_get_screen (window));
      x_min = 0;
    }

  device_height = bdkdev->axes[y_axis].max_value - bdkdev->axes[y_axis].min_value;
  if (device_height > 0)
    {
      y_min = bdkdev->axes[y_axis].min_value;
    }
  else
    {
      device_height = bdk_screen_get_height (bdk_drawable_get_screen (window));
      y_min = 0;
    }

  if (bdkdev->info.mode == BDK_MODE_SCREEN)
    {
      x_scale = bdk_screen_get_width (bdk_drawable_get_screen (window)) / device_width;
      y_scale = bdk_screen_get_height (bdk_drawable_get_screen (window)) / device_height;

      x_offset = - impl_window->input_window->root_x - priv->abs_x;
      y_offset = - impl_window->input_window->root_y - priv->abs_y;
    }
  else				/* BDK_MODE_WINDOW */
    {
      double x_resolution = bdkdev->axes[x_axis].resolution;
      double y_resolution = bdkdev->axes[y_axis].resolution;
      double device_aspect;
      /*
       * Some drivers incorrectly report the resolution of the device
       * as zero (in partiular linuxwacom < 0.5.3 with usb tablets).
       * This causes the device_aspect to become NaN and totally
       * breaks windowed mode.  If this is the case, the best we can
       * do is to assume the resolution is non-zero is equal in both
       * directions (which is true for many devices).  The absolute
       * value of the resolution doesn't matter since we only use the
       * ratio.
       */
      if ((x_resolution == 0) || (y_resolution == 0))
	{
	  x_resolution = 1;
	  y_resolution = 1;
	}
      device_aspect = (device_height*y_resolution) /
	(device_width*x_resolution);
      if (device_aspect * priv->width >= priv->height)
	{
	  /* device taller than window */
	  x_scale = priv->width / device_width;
	  y_scale = (x_scale * x_resolution) / y_resolution;

	  x_offset = 0;
	  y_offset = -(device_height * y_scale -  priv->height)/2;
	}
      else
	{
	  /* window taller than device */
	  y_scale = priv->height / device_height;
	  x_scale = (y_scale * y_resolution) / x_resolution;

	  y_offset = 0;
	  x_offset = - (device_width * x_scale - priv->width)/2;
	}
    }

  for (i=0; i<bdkdev->info.num_axes; i++)
    {
      switch (bdkdev->info.axes[i].use)
	{
	case BDK_AXIS_X:
	  axis_out[i] = x_offset + x_scale * (axis_data[x_axis] - x_min);
	  if (x_out)
	    *x_out = axis_out[i];
	  break;
	case BDK_AXIS_Y:
	  axis_out[i] = y_offset + y_scale * (axis_data[y_axis] - y_min);
	  if (y_out)
	    *y_out = axis_out[i];
	  break;
	default:
	  axis_out[i] =
	    (bdkdev->info.axes[i].max * (axis_data[i] - bdkdev->axes[i].min_value) +
	     bdkdev->info.axes[i].min * (bdkdev->axes[i].max_value - axis_data[i])) /
	    (bdkdev->axes[i].max_value - bdkdev->axes[i].min_value);
	  break;
	}
    }
}

/* combine the state of the core device and the device state
 * into one - for now we do this in a simple-minded manner -
 * we just take the keyboard portion of the core device and
 * the button portion (all of?) the device state.
 * Any button remapping should go on here.
 */
static guint
bdk_input_translate_state(guint state, guint device_state)
{
  return device_state | (state & 0xFF);
}


gboolean
_bdk_input_common_other_event (BdkEvent         *event,
			       XEvent           *xevent,
			       BdkWindow        *window,
			       BdkDevicePrivate *bdkdev)
{
  BdkWindowObject *priv, *impl_window;
  BdkInputWindow *input_window;

  priv = (BdkWindowObject *) window;
  impl_window = (BdkWindowObject *)_bdk_window_get_impl_window (window);
  input_window = impl_window->input_window;

  if ((xevent->type == bdkdev->buttonpress_type) ||
      (xevent->type == bdkdev->buttonrelease_type))
    {
      XDeviceButtonEvent *xdbe = (XDeviceButtonEvent *)(xevent);

      g_return_val_if_fail (xdbe->button < 256, FALSE);
      if (xdbe->type == bdkdev->buttonpress_type)
	{
	  event->button.type = BDK_BUTTON_PRESS;
	  if (!(bdkdev->button_state[xdbe->button/8] & 1 << (xdbe->button%8)))
	    {
	      bdkdev->button_state[xdbe->button/8] |= 1 << (xdbe->button%8);
	      bdkdev->button_count++;
	    }
	}
      else
	{
	  event->button.type = BDK_BUTTON_RELEASE;
	  if (bdkdev->button_state[xdbe->button/8] & 1 << (xdbe->button%8))
	    {
	      bdkdev->button_state[xdbe->button/8] &= ~(1 << (xdbe->button%8));
	      bdkdev->button_count--;
	    }
	}
      event->button.device = &bdkdev->info;
      event->button.window = window;
      event->button.time = xdbe->time;

      event->button.axes = g_new (gdouble, bdkdev->info.num_axes);
      bdk_input_update_axes (bdkdev, xdbe->axes_count, xdbe->first_axis,
			     xdbe->axis_data);
      bdk_input_translate_coordinates (bdkdev, window, bdkdev->axis_data,
				       event->button.axes,
				       &event->button.x, &event->button.y);
      event->button.x_root = event->button.x + priv->abs_x + input_window->root_x;
      event->button.y_root = event->button.y + priv->abs_y + input_window->root_y;
      event->button.state = bdk_input_translate_state (xdbe->state,xdbe->device_state);
      event->button.button = xdbe->button;

      if (event->button.type == BDK_BUTTON_PRESS)
	_bdk_event_button_generate (bdk_drawable_get_display (event->button.window),
				    event);

      BDK_NOTE (EVENTS,
	g_print ("button %s:\t\twindow: %ld  device: %ld  x,y: %f %f  button: %d\n",
		 (event->button.type == BDK_BUTTON_PRESS) ? "press" : "release",
		 xdbe->window,
		 xdbe->deviceid,
		 event->button.x, event->button.y,
		 xdbe->button));

      /* Update the timestamp of the latest user interaction, if the event has
       * a valid timestamp.
       */
      if (bdk_event_get_time (event) != BDK_CURRENT_TIME)
	bdk_x11_window_set_user_time (bdk_window_get_toplevel (window),
				      bdk_event_get_time (event));
      return TRUE;
  }

  if ((xevent->type == bdkdev->keypress_type) ||
      (xevent->type == bdkdev->keyrelease_type))
    {
      XDeviceKeyEvent *xdke = (XDeviceKeyEvent *)(xevent);

      BDK_NOTE (EVENTS,
	g_print ("device key %s:\twindow: %ld  device: %ld  keycode: %d\n",
		 (event->key.type == BDK_KEY_PRESS) ? "press" : "release",
		 xdke->window,
		 xdke->deviceid,
		 xdke->keycode));

      if (xdke->keycode < bdkdev->min_keycode ||
	  xdke->keycode >= bdkdev->min_keycode + bdkdev->info.num_keys)
	{
	  g_warning ("Invalid device key code received");
	  return FALSE;
	}

      event->key.keyval = bdkdev->info.keys[xdke->keycode - bdkdev->min_keycode].keyval;

      if (event->key.keyval == 0)
	{
	  BDK_NOTE (EVENTS,
	    g_print ("\t\ttranslation - NONE\n"));

	  return FALSE;
	}

      event->key.type = (xdke->type == bdkdev->keypress_type) ?
	BDK_KEY_PRESS : BDK_KEY_RELEASE;

      event->key.window = window;
      event->key.time = xdke->time;

      event->key.state = bdk_input_translate_state(xdke->state, xdke->device_state)
	| bdkdev->info.keys[xdke->keycode - bdkdev->min_keycode].modifiers;

      /* Add a string translation for the key event */
      if ((event->key.keyval >= 0x20) && (event->key.keyval <= 0xFF))
	{
	  event->key.length = 1;
	  event->key.string = g_new (gchar, 2);
	  event->key.string[0] = (gchar)event->key.keyval;
	  event->key.string[1] = 0;
	}
      else
	{
	  event->key.length = 0;
	  event->key.string = g_new0 (gchar, 1);
	}

      BDK_NOTE (EVENTS,
	g_print ("\t\ttranslation - keyval: %d modifiers: %#x\n",
		 event->key.keyval,
		 event->key.state));

      /* Update the timestamp of the latest user interaction, if the event has
       * a valid timestamp.
       */
      if (bdk_event_get_time (event) != BDK_CURRENT_TIME)
	bdk_x11_window_set_user_time (bdk_window_get_toplevel (window),
				      bdk_event_get_time (event));
      return TRUE;
    }

  if (xevent->type == bdkdev->motionnotify_type)
    {
      XDeviceMotionEvent *xdme = (XDeviceMotionEvent *)(xevent);

      event->motion.device = &bdkdev->info;

      event->motion.axes = g_new (gdouble, bdkdev->info.num_axes);
      bdk_input_update_axes (bdkdev, xdme->axes_count, xdme->first_axis, xdme->axis_data);
      bdk_input_translate_coordinates(bdkdev, window, bdkdev->axis_data,
				      event->motion.axes,
				      &event->motion.x,&event->motion.y);
      event->motion.x_root = event->motion.x + priv->abs_x + input_window->root_x;
      event->motion.y_root = event->motion.y + priv->abs_y + input_window->root_y;

      event->motion.type = BDK_MOTION_NOTIFY;
      event->motion.window = window;
      event->motion.time = xdme->time;
      event->motion.state = bdk_input_translate_state(xdme->state,
						      xdme->device_state);
      event->motion.is_hint = xdme->is_hint;

      BDK_NOTE (EVENTS,
	g_print ("motion notify:\t\twindow: %ld  device: %ld  x,y: %f %f  state %#4x  hint: %s\n",
		 xdme->window,
		 xdme->deviceid,
		 event->motion.x, event->motion.y,
		 event->motion.state,
		 (xdme->is_hint) ? "true" : "false"));


      /* Update the timestamp of the latest user interaction, if the event has
       * a valid timestamp.
       */
      if (bdk_event_get_time (event) != BDK_CURRENT_TIME)
	bdk_x11_window_set_user_time (bdk_window_get_toplevel (window),
				      bdk_event_get_time (event));
      return TRUE;
    }

  if (xevent->type == bdkdev->devicestatenotify_type)
    {
      int i;
      XDeviceStateNotifyEvent *xdse = (XDeviceStateNotifyEvent *)(xevent);
      XInputClass *input_class = (XInputClass *)xdse->data;
      for (i=0; i<xdse->num_classes; i++)
	{
	  if (input_class->class == ValuatorClass)
	    bdk_input_update_axes (bdkdev, bdkdev->info.num_axes, 0,
				   ((XValuatorState *)input_class)->valuators);
	  input_class = (XInputClass *)(((char *)input_class)+input_class->length);
	}

      BDK_NOTE (EVENTS,
	g_print ("device state notify:\t\twindow: %ld  device: %ld\n",
		 xdse->window,
		 xdse->deviceid));

      return FALSE;
    }
  if (xevent->type == bdkdev->proximityin_type ||
      xevent->type == bdkdev->proximityout_type)
    {
      XProximityNotifyEvent *xpne = (XProximityNotifyEvent *)(xevent);

      event->proximity.device = &bdkdev->info;
      event->proximity.type = (xevent->type == bdkdev->proximityin_type)?
	BDK_PROXIMITY_IN:BDK_PROXIMITY_OUT;
      event->proximity.window = window;
      event->proximity.time = xpne->time;

      /* Update the timestamp of the latest user interaction, if the event has
       * a valid timestamp.
       */
      if (bdk_event_get_time (event) != BDK_CURRENT_TIME)
	bdk_x11_window_set_user_time (bdk_window_get_toplevel (window),
				      bdk_event_get_time (event));
      return TRUE;
  }

  return FALSE;			/* wasn't one of our event types */
}

gboolean
_bdk_input_common_event_selected (BdkEvent         *event,
				  BdkWindow        *window,
				  BdkDevicePrivate *bdkdev)
{
  BdkWindowObject *priv = (BdkWindowObject *) window;

  switch (event->type) {
    case BDK_BUTTON_PRESS:
      return priv->extension_events & BDK_BUTTON_PRESS_MASK;

    case BDK_BUTTON_RELEASE:
      return priv->extension_events & BDK_BUTTON_RELEASE_MASK;

    case BDK_KEY_PRESS:
      return priv->extension_events & BDK_KEY_PRESS_MASK;

    case BDK_KEY_RELEASE:
      return priv->extension_events & BDK_KEY_RELEASE_MASK;

    case BDK_MOTION_NOTIFY:
      if (priv->extension_events & BDK_POINTER_MOTION_MASK)
	return TRUE;
      if (bdkdev->button_count && (priv->extension_events & BDK_BUTTON_MOTION_MASK))
	return TRUE;

      if ((bdkdev->button_state[0] & 1 << 1) && (priv->extension_events & BDK_BUTTON1_MOTION_MASK))
	return TRUE;
      if ((bdkdev->button_state[0] & 1 << 2) && (priv->extension_events & BDK_BUTTON2_MOTION_MASK))
	return TRUE;
      if ((bdkdev->button_state[0] & 1 << 3) && (priv->extension_events & BDK_BUTTON3_MOTION_MASK))
	return TRUE;

      return FALSE;

    case BDK_PROXIMITY_IN:
	  return priv->extension_events & BDK_PROXIMITY_IN_MASK;

    case BDK_PROXIMITY_OUT:
	  return priv->extension_events & BDK_PROXIMITY_OUT_MASK;

    default:
      return FALSE;
  }


}


gboolean
_bdk_device_get_history (BdkDevice         *device,
			 BdkWindow         *window,
			 guint32            start,
			 guint32            stop,
			 BdkTimeCoord    ***events,
			 gint              *n_events)
{
  BdkTimeCoord **coords;
  XDeviceTimeCoord *device_coords;
  BdkWindow *impl_window;
  BdkDevicePrivate *bdkdev;
  gint mode_return;
  gint axis_count_return;
  gint i;

  bdkdev = (BdkDevicePrivate *)device;

  impl_window = _bdk_window_get_impl_window (window);

  device_coords = XGetDeviceMotionEvents (BDK_WINDOW_XDISPLAY (impl_window),
					  bdkdev->xdevice,
					  start, stop,
					  n_events, &mode_return,
					  &axis_count_return);

  if (device_coords)
    {
      coords = _bdk_device_allocate_history (device, *n_events);

      for (i = 0; i < *n_events; i++)
	{
	  coords[i]->time = device_coords[i].time;

	  bdk_input_translate_coordinates (bdkdev, window,
					   device_coords[i].data,
					   coords[i]->axes, NULL, NULL);
	}

      XFreeDeviceMotionEvents (device_coords);

      *events = coords;

      return TRUE;
    }
  else
    return FALSE;
}

/**
 * bdk_device_get_state:
 * @device: a #BdkDevice.
 * @window: a #BdkWindow.
 * @axes: an array of doubles to store the values of the axes of @device in,
 * or %NULL.
 * @mask: location to store the modifiers, or %NULL.
 *
 * Gets the current state of a device.
 */
void
bdk_device_get_state (BdkDevice       *device,
		      BdkWindow       *window,
		      gdouble         *axes,
		      BdkModifierType *mask)
{
  gint i;

  g_return_if_fail (device != NULL);
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_IS_CORE (device))
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
      int (*old_handler) (Display *, XErrorEvent *);
      BdkDevicePrivate *bdkdev;
      XDeviceState *state = NULL;
      XInputClass *input_class;

      if (mask)
	bdk_window_get_pointer (window, NULL, NULL, mask);

      bdkdev = (BdkDevicePrivate *)device;

      /* From X11 doc: "XQueryDeviceState can generate a BadDevice error."
       * This would occur in particular when a device is unplugged,
       * which would cause the program to crash (see bug 575767).
       *
       * To handle this case gracefully, we simply ignore the device.
       * BTK+ 3 handles this better with XInput 2's hotplugging support;
       * but this is better than a crash in BTK+ 2.
       */
      old_handler = XSetErrorHandler (ignore_errors);
      state = XQueryDeviceState (BDK_WINDOW_XDISPLAY (window),
				 bdkdev->xdevice);
      XSetErrorHandler (old_handler);

      if (! state)
        return;

      input_class = state->data;
      for (i=0; i<state->num_classes; i++)
	{
	  switch (input_class->class)
	    {
	    case ValuatorClass:
	      if (axes)
		bdk_input_translate_coordinates (bdkdev, window,
						 ((XValuatorState *)input_class)->valuators,
						 axes, NULL, NULL);
	      break;

	    case ButtonClass:
	      if (mask)
		{
		  *mask &= 0xFF;
		  if (((XButtonState *)input_class)->num_buttons > 0)
		    *mask |= ((XButtonState *)input_class)->buttons[0] << 7;
		  /* BDK_BUTTON1_MASK = 1 << 8, and button n is stored
		   * in bit 1<<(n%8) in byte n/8. n = 1,2,... */
		}
	      break;
	    }
	  input_class = (XInputClass *)(((char *)input_class)+input_class->length);
	}
      XFreeDeviceState (state);
    }
}

#define __BDK_INPUT_X11_C__
#include "bdkaliasdef.c"
