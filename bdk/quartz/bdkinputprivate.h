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
  gint (*set_mode) (guint32 deviceid, BdkInputMode mode);
  void (*set_axes) (guint32 deviceid, BdkAxisUse *axes);
  void (*set_key)  (guint32 deviceid,
		    guint   index,
		    guint   keyval,
		    BdkModifierType modifiers);
	
  BdkTimeCoord* (*motion_events) (BdkWindow *window,
				  guint32 deviceid,
				  guint32 start,
				  guint32 stop,
				  gint *nevents_return);
  void (*get_pointer)   (BdkWindow       *window,
			 guint32	  deviceid,
			 gdouble         *x,
			 gdouble         *y,
			 gdouble         *pressure,
			 gdouble         *xtilt,
			 gdouble         *ytilt,
			 BdkModifierType *mask);
  gint (*grab_pointer) (BdkWindow *     window,
			gint            owner_events,
			BdkEventMask    event_mask,
			BdkWindow *     confine_to,
			guint32         time);
  void (*ungrab_pointer) (guint32 time);

  void (*configure_event) (BdkEventConfigure *event, BdkWindow *window);
  void (*enter_event) (BdkEventCrossing *event, BdkWindow *window);
  gint (*other_event) (BdkEvent *event, BdkWindow *window);
  /* Handle an unidentified event. Returns TRUE if handled, FALSE
     otherwise */
  gint (*window_none_event) (BdkEvent *event);
  gint (*enable_window) (BdkWindow *window, BdkDevicePrivate *bdkdev);
  gint (*disable_window) (BdkWindow *window, BdkDevicePrivate *bdkdev);
};

/* information about a device axis */
struct _BdkAxisInfo
{
  /* reported x resolution */
  gint xresolution;

  /* reported x minimum/maximum values */
  gint xmin_value, xmax_value;

  /* calibrated resolution (for aspect ration) - only relative values
     between axes used */
  gint resolution;
  
  /* calibrated minimum/maximum values */
  gint min_value, max_value;
};

#define BDK_INPUT_NUM_EVENTC 6

struct _BdkDevicePrivate {
  BdkDevice  info;

  gint last_state;
  gdouble *last_axes_state;
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
  gint root_x;
  gint root_y;

  /* rectangles relative to window of windows obscuring this one */
  BdkRectangle *obscuring;
  gint num_obscuring;

  /* Is there a pointer grab for this window ? */
  gint grabbed;
};

/* Global data */

extern const BdkDevice bdk_input_core_info;

extern BdkInputVTable bdk_input_vtable;
/* information about network port and host for gxid daemon */
extern gchar           *_bdk_input_gxid_host;
extern gint             _bdk_input_gxid_port;
extern gint             _bdk_input_ignore_core;

/* Function declarations */

BdkInputWindow *   _bdk_input_window_find    (BdkWindow        *window);
void               _bdk_input_window_destroy (BdkWindow        *window);
void               _bdk_input_init           (void);
void               _bdk_input_exit           (void);
gint               _bdk_input_enable_window  (BdkWindow        *window,
					      BdkDevicePrivate *bdkdev);
gint               _bdk_input_disable_window (BdkWindow        *window,
					      BdkDevicePrivate *bdkdev);
void               _bdk_init_input_core      (void);

void               _bdk_input_window_crossing (BdkWindow       *window,
                                               gboolean         enter);

void               _bdk_input_quartz_tablet_proximity (NSPointingDeviceType deviceType);
gboolean           _bdk_input_fill_quartz_input_event (BdkEvent  *event,
                                                       NSEvent   *nsevent,
                                                       BdkEvent  *input_event);

void _bdk_input_exit           (void);

#endif /* __BDK_INPUTPRIVATE_H__ */
