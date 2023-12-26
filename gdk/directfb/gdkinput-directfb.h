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

struct _BdkDeviceClass
{
  GObjectClass parent_class;
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

#define BDK_IS_CORE(d) (((BdkDevice *)(d)) == _bdk_core_pointer)

extern GList *_bdk_input_devices;
extern GList *_bdk_input_windows;

extern gint   _bdk_input_ignore_core;

/* Function declarations */

/* The following functions are provided by each implementation
 */
gint             _bdk_input_window_none_event(BdkEvent          *event,
                                              gchar             *msg);
void             _bdk_input_configure_event  (BdkEventConfigure *event,
                                              BdkWindow         *window);
void             _bdk_input_enter_event      (BdkEventCrossing  *event,
                                              BdkWindow         *window);
gint             _bdk_input_other_event      (BdkEvent          *event,
                                              gchar             *msg,
                                              BdkWindow         *window);

/* These should be in bdkinternals.h */

BdkInputWindow  * bdk_input_window_find      (BdkWindow         *window);

void              bdk_input_window_destroy   (BdkWindow         *window);

gint             _bdk_input_enable_window    (BdkWindow         *window,
                                              BdkDevice         *bdkdev);
gint             _bdk_input_disable_window   (BdkWindow         *window,
                                              BdkDevice         *bdkdev);
gint             _bdk_input_grab_pointer     (BdkWindow         *window,
                                              gint               owner_events,
                                              BdkEventMask       event_mask,
                                              BdkWindow         *confine_to,
                                              guint32            time);
void             _bdk_input_ungrab_pointer   (guint32            time);
gboolean         _bdk_device_get_history     (BdkDevice         *device,
                                              BdkWindow         *window,
                                              guint32            start,
                                              guint32            stop,
                                              BdkTimeCoord    ***events,
                                              gint              *n_events);

gint             bdk_input_common_init        (gint              include_core);
gint             bdk_input_common_other_event (BdkEvent         *event,
                                               gchar            *msg,
                                               BdkInputWindow   *input_window,
                                               BdkWindow        *window);

void         _bdk_directfb_keyboard_init      (void);
void         _bdk_directfb_keyboard_exit      (void);

void         bdk_directfb_translate_key_event (DFBWindowEvent   *dfb_event,
                                               BdkEventKey      *event);

#endif /* __BDK_INPUT_DIRECTFB_H__ */
