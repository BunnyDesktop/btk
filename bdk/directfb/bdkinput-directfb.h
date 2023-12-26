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
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.
 */

/*
 * BTK+ DirectFB backend
 * Copyright (C) 2001-2002  convergence integrated media GmbH
 * Copyright (C) 2002       convergence GmbH
 * Written by Denis Oliver Kropp <dok@convergence.de> and
 *            Sven Neumann <sven@convergence.de>
 */

#ifndef __BDK_INPUT_DIRECTFB_H__
#define __BDK_INPUT_DIRECTFB_H__

extern BdkModifierType _bdk_directfb_modifiers;
extern int _bdk_directfb_mouse_x, _bdk_directfb_mouse_y;

typedef struct _BdkAxisInfo      BdkAxisInfo;

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

#define BDK_IS_CORE(d) (((BdkDevice *)(d)) == _bdk_core_pointer)

extern GList *_bdk_input_devices;
extern GList *_bdk_input_windows;

extern bint   _bdk_input_ignore_core;

/* Function declarations */

/* The following functions are provided by each implementation
 */
bint             _bdk_input_window_none_event(BdkEvent          *event,
                                              bchar             *msg);
void             _bdk_input_configure_event  (BdkEventConfigure *event,
                                              BdkWindow         *window);
void             _bdk_input_enter_event      (BdkEventCrossing  *event,
                                              BdkWindow         *window);
bint             _bdk_input_other_event      (BdkEvent          *event,
                                              bchar             *msg,
                                              BdkWindow         *window);

/* These should be in bdkinternals.h */

BdkInputWindow  * bdk_input_window_find      (BdkWindow         *window);

void              bdk_input_window_destroy   (BdkWindow         *window);

bint             _bdk_input_enable_window    (BdkWindow         *window,
                                              BdkDevice         *bdkdev);
bint             _bdk_input_disable_window   (BdkWindow         *window,
                                              BdkDevice         *bdkdev);
bint             _bdk_input_grab_pointer     (BdkWindow         *window,
                                              bint               owner_events,
                                              BdkEventMask       event_mask,
                                              BdkWindow         *confine_to,
                                              buint32            time);
void             _bdk_input_ungrab_pointer   (buint32            time);
bboolean         _bdk_device_get_history     (BdkDevice         *device,
                                              BdkWindow         *window,
                                              buint32            start,
                                              buint32            stop,
                                              BdkTimeCoord    ***events,
                                              bint              *n_events);

bint             bdk_input_common_init        (bint              include_core);
bint             bdk_input_common_other_event (BdkEvent         *event,
                                               bchar            *msg,
                                               BdkInputWindow   *input_window,
                                               BdkWindow        *window);

void         _bdk_directfb_keyboard_init      (void);
void         _bdk_directfb_keyboard_exit      (void);

void         bdk_directfb_translate_key_event (DFBWindowEvent   *dfb_event,
                                               BdkEventKey      *event);

#endif /* __BDK_INPUT_DIRECTFB_H__ */
