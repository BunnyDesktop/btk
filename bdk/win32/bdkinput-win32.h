/* BDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the BTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#ifndef __BDK_INPUT_WIN32_H__
#define __BDK_INPUT_WIN32_H__

#include <windows.h>
#include <wintab.h>

typedef struct _BdkAxisInfo    BdkAxisInfo;
typedef struct _BdkDevicePrivate BdkDevicePrivate;

/* information about a device axis */
struct _BdkAxisInfo
{
  /* calibrated resolution (for aspect ratio) - only relative values
     between axes used */
  gint resolution;
  
  /* calibrated minimum/maximum values */
  gint min_value, max_value;
};

struct _BdkDeviceClass
{
  BObjectClass parent_class;
};

struct _BdkDevicePrivate
{
  BdkDevice info;

  /* information about the axes */
  BdkAxisInfo *axes;

  gint button_state;

  gint *last_axis_data;

  /* WINTAB stuff: */
  HCTX hctx;
  /* Cursor number */
  UINT cursor;
  /* The cursor's CSR_PKTDATA */
  WTPKT pktdata;
  /* Azimuth and altitude axis */
  AXIS orientation_axes[2];
};

/* Addition used for extension_events mask */
#define BDK_ALL_DEVICES_MASK (1<<30)

struct _BdkInputWindow
{
  /* bdk window */
  GList *windows; /* BdkWindow:s with extension_events set */

  BdkWindow *impl_window; /* an impl window */

  /* position relative to root window */
  gint root_x;
  gint root_y;
};

/* Global data */

#define BDK_IS_CORE(d) (((BdkDevice *)(d)) == bdk_display_get_default ()->core_pointer)

extern GList *_bdk_input_devices;
extern GList *_bdk_input_windows;

extern gboolean _bdk_input_in_proximity;

/* Function declarations */
void             _bdk_init_input_core (BdkDisplay *display);

BdkTimeCoord ** _bdk_device_allocate_history (BdkDevice *device,
					      gint       n_events);

/* The following functions are provided by each implementation
 * (just wintab for now)
 */
void             _bdk_input_configure_event  (BdkWindow        *window);
gboolean         _bdk_input_other_event      (BdkEvent         *event,
					      MSG              *msg,
					      BdkWindow        *window);

void             _bdk_input_crossing_event   (BdkWindow        *window,
					      gboolean          enter);


/* These should be in bdkinternals.h */

BdkInputWindow  *_bdk_input_window_find      (BdkWindow        *window);

void             _bdk_input_window_destroy   (BdkWindow *window);

void             _bdk_input_select_events    (BdkWindow        *impl_window);
gint             _bdk_input_grab_pointer     (BdkWindow        *window,
					      gint              owner_events,
					      BdkEventMask      event_mask,
					      BdkWindow        *confine_to,
					      guint32           time);
void             _bdk_input_ungrab_pointer   (guint32           time);
gboolean         _bdk_device_get_history     (BdkDevice         *device,
					      BdkWindow         *window,
					      guint32            start,
					      guint32            stop,
					      BdkTimeCoord    ***events,
					      gint              *n_events);

void		_bdk_input_wintab_init_check (void);
void		_bdk_input_set_tablet_active (void);
void            _bdk_input_update_for_device_mode (BdkDevicePrivate *bdkdev);
void            _bdk_input_check_proximity (void);

#endif /* __BDK_INPUT_WIN32_H__ */
