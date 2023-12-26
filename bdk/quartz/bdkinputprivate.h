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
#include "bdkquartz.h"

typedef struct _BdkAxisInfo    BdkAxisInfo;
typedef struct _BdkInputVTable BdkInputVTable;
typedef struct _BdkDevicePrivate BdkDevicePrivate;

struct _BdkInputVTable {
  bint (*set_mode) (buint32 deviceid, BdkInputMode mode);
  void (*set_axes) (buint32 deviceid, BdkAxisUse *axes);
  void (*set_key)  (buint32 deviceid,
		    buint   index,
		    buint   keyval,
		    BdkModifierType modifiers);
	
  BdkTimeCoord* (*motion_events) (BdkWindow *window,
				  buint32 deviceid,
				  buint32 start,
				  buint32 stop,
				  bint *nevents_return);
  void (*get_pointer)   (BdkWindow       *window,
			 buint32	  deviceid,
			 bdouble         *x,
			 bdouble         *y,
			 bdouble         *pressure,
			 bdouble         *xtilt,
			 bdouble         *ytilt,
			 BdkModifierType *mask);
  bint (*grab_pointer) (BdkWindow *     window,
			bint            owner_events,
			BdkEventMask    event_mask,
			BdkWindow *     confine_to,
			buint32         time);
  void (*ungrab_pointer) (buint32 time);

  void (*configure_event) (BdkEventConfigure *event, BdkWindow *window);
  void (*enter_event) (BdkEventCrossing *event, BdkWindow *window);
  bint (*other_event) (BdkEvent *event, BdkWindow *window);
  /* Handle an unidentified event. Returns TRUE if handled, FALSE
     otherwise */
  bint (*window_none_event) (BdkEvent *event);
  bint (*enable_window) (BdkWindow *window, BdkDevicePrivate *bdkdev);
  bint (*disable_window) (BdkWindow *window, BdkDevicePrivate *bdkdev);
};

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

struct _BdkDevicePrivate {
  BdkDevice  info;

  bint last_state;
  bdouble *last_axes_state;
};

struct _BdkDeviceClass
{
  BObjectClass parent_class;
};

struct _BdkInputWindow
{
  /* bdk window */
  BdkWindow *window;

  /* Extension mode (BDK_EXTENSION_EVENTS_ALL/CURSOR) */
  BdkExtensionMode mode;

  /* position relative to root window */
  bint root_x;
  bint root_y;

  /* rectangles relative to window of windows obscuring this one */
  BdkRectangle *obscuring;
  bint num_obscuring;

  /* Is there a pointer grab for this window ? */
  bint grabbed;
};

/* Global data */

extern const BdkDevice bdk_input_core_info;

extern BdkInputVTable bdk_input_vtable;
/* information about network port and host for gxid daemon */
extern bchar           *_bdk_input_gxid_host;
extern bint             _bdk_input_gxid_port;
extern bint             _bdk_input_ignore_core;

/* Function declarations */

BdkInputWindow *   _bdk_input_window_find    (BdkWindow        *window);
void               _bdk_input_window_destroy (BdkWindow        *window);
void               _bdk_input_init           (void);
void               _bdk_input_exit           (void);
bint               _bdk_input_enable_window  (BdkWindow        *window,
					      BdkDevicePrivate *bdkdev);
bint               _bdk_input_disable_window (BdkWindow        *window,
					      BdkDevicePrivate *bdkdev);
void               _bdk_init_input_core      (void);

void               _bdk_input_window_crossing (BdkWindow       *window,
                                               bboolean         enter);

void               _bdk_input_quartz_tablet_proximity (NSPointingDeviceType deviceType);
bboolean           _bdk_input_fill_quartz_input_event (BdkEvent  *event,
                                                       NSEvent   *nsevent,
                                                       BdkEvent  *input_event);

void _bdk_input_exit           (void);

#endif /* __BDK_INPUTPRIVATE_H__ */
