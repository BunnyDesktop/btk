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

#ifndef __BDK_INPUTPRIVATE_H__
#define __BDK_INPUTPRIVATE_H__

#include "config.h"
#include "bdkinput.h"
#include "bdkevents.h"
#include "bdkx.h"

#include <X11/Xlib.h>

#ifndef XINPUT_NONE
#include <X11/extensions/XInput.h>
#endif


typedef struct _BdkAxisInfo    BdkAxisInfo;
typedef struct _BdkDevicePrivate BdkDevicePrivate;

/* information about a device axis */
struct _BdkAxisInfo
{
  /* reported x resolution */
  bint xresolution;

  /* reported x minimum/maximum values */
  bint xmin_value, xmax_value;

  /* calibrated resolution (for aspect ration) - only relative values
     between axes used */
  bint resolution;
  
  /* calibrated minimum/maximum values */
  bint min_value, max_value;
};

#define BDK_INPUT_NUM_EVENTC 6

struct _BdkDevicePrivate
{
  BdkDevice info;

  buint32 deviceid;

  BdkDisplay *display;
  

#ifndef XINPUT_NONE
  /* information about the axes */
  BdkAxisInfo *axes;
  bint *axis_data;

  /* Information about XInput device */
  XDevice       *xdevice;

  /* minimum key code for device */
  bint min_keycode;	       

  int buttonpress_type, buttonrelease_type, keypress_type,
      keyrelease_type, motionnotify_type, proximityin_type, 
      proximityout_type, changenotify_type, devicestatenotify_type;

  /* true if we need to select a different set of events, but
     can't because this is the core pointer */
  bint needs_update;

  /* Mask of buttons (used for button grabs) */
  char button_state[32];
  bint button_count;

  /* true if we've claimed the device as active. (used only for XINPUT_GXI) */
  bint claimed;
#endif /* !XINPUT_NONE */
};

struct _BdkDeviceClass
{
  BObjectClass parent_class;
};

/* Addition used for extension_events mask */
#define BDK_ALL_DEVICES_MASK (1<<30)

struct _BdkInputWindow
{
  GList *windows; /* BdkWindow:s with extension_events set */

  /* bdk window */
  BdkWindow *impl_window; /* an impl window */
  BdkWindow *button_down_window;

  /* position relative to root window */
  bint root_x;
  bint root_y;

  /* Is there a pointer grab for this window ? */
  bint grabbed;
};

/* Global data */

#define BDK_IS_CORE(d) (((BdkDevice *)(d)) == ((BdkDevicePrivate *)(d))->display->core_pointer)

/* Function declarations */

BdkInputWindow *_bdk_input_window_find       (BdkWindow *window);
void            _bdk_input_window_destroy    (BdkWindow *window);
BdkTimeCoord ** _bdk_device_allocate_history (BdkDevice *device,
					      bint       n_events);
void            _bdk_init_input_core         (BdkDisplay *display);

/* The following functions are provided by each implementation
 * (xfree, gxi, and none)
 */
void             _bdk_input_configure_event  (XConfigureEvent  *xevent,
					      BdkWindow        *window);
void             _bdk_input_crossing_event   (BdkWindow        *window,
					      bboolean          enter);
bboolean         _bdk_input_other_event      (BdkEvent         *event,
					      XEvent           *xevent,
					      BdkWindow        *window);
bint             _bdk_input_grab_pointer     (BdkWindow        *window,
					      BdkWindow        *native_window,
					      bint              owner_events,
					      BdkEventMask      event_mask,
					      BdkWindow        *confine_to,
					      buint32           time);
void             _bdk_input_ungrab_pointer   (BdkDisplay       *display,
					      buint32           time);
bboolean         _bdk_device_get_history     (BdkDevice         *device,
					      BdkWindow         *window,
					      buint32            start,
					      buint32            stop,
					      BdkTimeCoord    ***events,
					      bint              *n_events);

#ifndef XINPUT_NONE

#define BDK_MAX_DEVICE_CLASSES 13

bint               _bdk_input_common_init               (BdkDisplay	  *display,
							 bint              include_core);
BdkDevicePrivate * _bdk_input_find_device               (BdkDisplay	  *display,
							 buint32           id);
void               _bdk_input_get_root_relative_geometry(BdkWindow        *window,
							 int              *x_ret,
							 int              *y_ret);
void               _bdk_input_common_find_events        (BdkDevicePrivate *bdkdev,
							 bint              mask,
							 XEventClass      *classes,
							 int              *num_classes);
void               _bdk_input_select_events             (BdkWindow        *impl_window,
							 BdkDevicePrivate *bdkdev);
bint               _bdk_input_common_other_event        (BdkEvent         *event,
							 XEvent           *xevent,
							 BdkWindow        *window,
							 BdkDevicePrivate *bdkdev);
bboolean	   _bdk_input_common_event_selected     (BdkEvent         *event,
							 BdkWindow        *window,
							 BdkDevicePrivate *bdkdev);

#endif /* !XINPUT_NONE */

#endif /* __BDK_INPUTPRIVATE_H__ */
