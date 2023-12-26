/* BDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998-1999 Tor Lillqvist
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

/*
  Main entry point for 2.6 seems to be open_display so
  most stuff in main is moved over to bdkdisplay-directfb.c
  I'll move stub functions here that make no sense for directfb
  and true globals
  Michael Emmel
*/

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include "bdk.h"

#include "bdkdisplay.h"
#include "bdkdirectfb.h"
#include "bdkprivate-directfb.h"

#include "bdkinternals.h"

#include "bdkinput-directfb.h"

#include "bdkintl.h"
#include "bdkalias.h"


void
_bdk_windowing_init (void)
{
  /* Not that usable called before parse_args
   */
}

void
bdk_set_use_xshm (gboolean use_xshm)
{
}

gboolean
bdk_get_use_xshm (void)
{
  return FALSE;
}

void
_bdk_windowing_display_set_sm_client_id (BdkDisplay *display,const gchar *sm_client_id)
{
  g_message ("bdk_set_sm_client_id() is unimplemented.");
}


void
_bdk_windowing_exit (void)
{

  if (_bdk_display->buffer)
    _bdk_display->buffer->Release (_bdk_display->buffer);

  _bdk_directfb_keyboard_exit ();

  if (_bdk_display->keyboard)
    _bdk_display->keyboard->Release (_bdk_display->keyboard);

  _bdk_display->layer->Release (_bdk_display->layer);

  _bdk_display->directfb->Release (_bdk_display->directfb);

  g_free (_bdk_display);
  _bdk_display = NULL;
}

gchar *
bdk_get_display (void)
{
  return g_strdup (bdk_display_get_name (bdk_display_get_default ()));
}


/* utils */
static const guint type_masks[] =
  {
    BDK_STRUCTURE_MASK,        /* BDK_DELETE            =  0, */
    BDK_STRUCTURE_MASK,        /* BDK_DESTROY           =  1, */
    BDK_EXPOSURE_MASK,         /* BDK_EXPOSE            =  2, */
    BDK_POINTER_MOTION_MASK,   /* BDK_MOTION_NOTIFY     =  3, */
    BDK_BUTTON_PRESS_MASK,     /* BDK_BUTTON_PRESS      =  4, */
    BDK_BUTTON_PRESS_MASK,     /* BDK_2BUTTON_PRESS     =  5, */
    BDK_BUTTON_PRESS_MASK,     /* BDK_3BUTTON_PRESS     =  6, */
    BDK_BUTTON_RELEASE_MASK,   /* BDK_BUTTON_RELEASE    =  7, */
    BDK_KEY_PRESS_MASK,        /* BDK_KEY_PRESS         =  8, */
    BDK_KEY_RELEASE_MASK,      /* BDK_KEY_RELEASE       =  9, */
    BDK_ENTER_NOTIFY_MASK,     /* BDK_ENTER_NOTIFY      = 10, */
    BDK_LEAVE_NOTIFY_MASK,     /* BDK_LEAVE_NOTIFY      = 11, */
    BDK_FOCUS_CHANGE_MASK,     /* BDK_FOCUS_CHANGE      = 12, */
    BDK_STRUCTURE_MASK,        /* BDK_CONFIGURE         = 13, */
    BDK_VISIBILITY_NOTIFY_MASK,/* BDK_MAP               = 14, */
    BDK_VISIBILITY_NOTIFY_MASK,/* BDK_UNMAP             = 15, */
    BDK_PROPERTY_CHANGE_MASK,  /* BDK_PROPERTY_NOTIFY   = 16, */
    BDK_PROPERTY_CHANGE_MASK,  /* BDK_SELECTION_CLEAR   = 17, */
    BDK_PROPERTY_CHANGE_MASK,  /* BDK_SELECTION_REQUEST = 18, */
    BDK_PROPERTY_CHANGE_MASK,  /* BDK_SELECTION_NOTIFY  = 19, */
    BDK_PROXIMITY_IN_MASK,     /* BDK_PROXIMITY_IN      = 20, */
    BDK_PROXIMITY_OUT_MASK,    /* BDK_PROXIMITY_OUT     = 21, */
    BDK_ALL_EVENTS_MASK,       /* BDK_DRAG_ENTER        = 22, */
    BDK_ALL_EVENTS_MASK,       /* BDK_DRAG_LEAVE        = 23, */
    BDK_ALL_EVENTS_MASK,       /* BDK_DRAG_MOTION       = 24, */
    BDK_ALL_EVENTS_MASK,       /* BDK_DRAG_STATUS       = 25, */
    BDK_ALL_EVENTS_MASK,       /* BDK_DROP_START        = 26, */
    BDK_ALL_EVENTS_MASK,       /* BDK_DROP_FINISHED     = 27, */
    BDK_ALL_EVENTS_MASK,       /* BDK_CLIENT_EVENT      = 28, */
    BDK_VISIBILITY_NOTIFY_MASK,/* BDK_VISIBILITY_NOTIFY = 29, */
    BDK_EXPOSURE_MASK,         /* BDK_NO_EXPOSE         = 30, */
    BDK_SCROLL_MASK            /* BDK_SCROLL            = 31  */
  };

BdkWindow *
bdk_directfb_other_event_window (BdkWindow    *window,
                                 BdkEventType  type)
{
  guint32    evmask;
  BdkWindow *w;

  w = window;
  while (w != _bdk_parent_root)
    {
      /* Huge hack, so that we don't propagate events to BtkWindow->frame */
      if ((w != window) &&
          (BDK_WINDOW_OBJECT (w)->window_type != BDK_WINDOW_CHILD) &&
          (g_object_get_data (G_OBJECT (w), "bdk-window-child-handler")))
        break;

      evmask = BDK_WINDOW_OBJECT (w)->event_mask;

      if (evmask & type_masks[type])
        return w;

      w = bdk_window_get_parent (w);
    }

  return NULL;
}

BdkWindow *
bdk_directfb_pointer_event_window (BdkWindow    *window,
                                   BdkEventType  type)
{
  guint            evmask;
  BdkModifierType  mask;
  BdkWindow       *w;

  bdk_directfb_mouse_get_info (NULL, NULL, &mask);

  if (_bdk_directfb_pointer_grab_window && !_bdk_directfb_pointer_grab_owner_events )
    {
      evmask = _bdk_directfb_pointer_grab_events;

      if (evmask & (BDK_BUTTON1_MOTION_MASK |
                    BDK_BUTTON2_MOTION_MASK |
                    BDK_BUTTON3_MOTION_MASK))
        {
          if (((mask & BDK_BUTTON1_MASK) &&
               (evmask & BDK_BUTTON1_MOTION_MASK)) ||
              ((mask & BDK_BUTTON2_MASK) &&
               (evmask & BDK_BUTTON2_MOTION_MASK)) ||
              ((mask & BDK_BUTTON3_MASK) &&
               (evmask & BDK_BUTTON3_MOTION_MASK)))
            evmask |= BDK_POINTER_MOTION_MASK;
        }

      if (evmask & type_masks[type]) {

        if (_bdk_directfb_pointer_grab_owner_events) {
          return _bdk_directfb_pointer_grab_window;
        } else {
          BdkWindowObject *obj = BDK_WINDOW_OBJECT (window);
          while (obj != NULL &&
                 obj != BDK_WINDOW_OBJECT (_bdk_directfb_pointer_grab_window)) {
            obj = (BdkWindowObject *)obj->parent;
          }
          if (obj == BDK_WINDOW_OBJECT (_bdk_directfb_pointer_grab_window)) {
            return window;
          } else {
            //was not  child of the grab window so return the grab window
            return _bdk_directfb_pointer_grab_window;
          }
        }
      }
    }

  w = window;
  while (w != _bdk_parent_root)
    {
      /* Huge hack, so that we don't propagate events to BtkWindow->frame */
      if ((w != window) &&
          (BDK_WINDOW_OBJECT (w)->window_type != BDK_WINDOW_CHILD) &&
          (g_object_get_data (G_OBJECT (w), "bdk-window-child-handler")))
        break;

      evmask = BDK_WINDOW_OBJECT (w)->event_mask;

      if (evmask & (BDK_BUTTON1_MOTION_MASK |
                    BDK_BUTTON2_MOTION_MASK |
                    BDK_BUTTON3_MOTION_MASK))
        {
          if (((mask & BDK_BUTTON1_MASK) &&
               (evmask & BDK_BUTTON1_MOTION_MASK)) ||
              ((mask & BDK_BUTTON2_MASK) &&
               (evmask & BDK_BUTTON2_MOTION_MASK)) ||
              ((mask & BDK_BUTTON3_MASK) &&
               (evmask & BDK_BUTTON3_MOTION_MASK)))
            evmask |= BDK_POINTER_MOTION_MASK;
        }

      if (evmask & type_masks[type])
        return w;

      w = bdk_window_get_parent (w);
    }

  return NULL;
}

BdkWindow *
bdk_directfb_keyboard_event_window (BdkWindow    *window,
                                    BdkEventType  type)
{
  guint32    evmask;
  BdkWindow *w;

  if (_bdk_directfb_keyboard_grab_window &&
      !_bdk_directfb_keyboard_grab_owner_events)
    {
      return _bdk_directfb_keyboard_grab_window;
    }

  w = window;
  while (w != _bdk_parent_root)
    {
      /* Huge hack, so that we don't propagate events to BtkWindow->frame */
      if ((w != window) &&
          (BDK_WINDOW_OBJECT (w)->window_type != BDK_WINDOW_CHILD) &&
          (g_object_get_data (G_OBJECT (w), "bdk-window-child-handler")))
        break;

      evmask = BDK_WINDOW_OBJECT (w)->event_mask;

      if (evmask & type_masks[type])
        return w;

      w = bdk_window_get_parent (w);
    }
  return w;
}


void
bdk_directfb_event_fill (BdkEvent     *event,
                         BdkWindow    *window,
                         BdkEventType  type)
{
  guint32 the_time = bdk_directfb_get_time ();

  event->any.type       = type;
  event->any.window     = g_object_ref (window);
  event->any.send_event = FALSE;

  switch (type)
    {
    case BDK_MOTION_NOTIFY:
      event->motion.time = the_time;
      event->motion.axes = NULL;
      break;
    case BDK_BUTTON_PRESS:
    case BDK_2BUTTON_PRESS:
    case BDK_3BUTTON_PRESS:
    case BDK_BUTTON_RELEASE:
      event->button.time = the_time;
      event->button.axes = NULL;
      break;
    case BDK_KEY_PRESS:
    case BDK_KEY_RELEASE:
      event->key.time = the_time;
      break;
    case BDK_ENTER_NOTIFY:
    case BDK_LEAVE_NOTIFY:
      event->crossing.time = the_time;
      break;
    case BDK_PROPERTY_NOTIFY:
      event->property.time = the_time;
      break;
    case BDK_SELECTION_CLEAR:
    case BDK_SELECTION_REQUEST:
    case BDK_SELECTION_NOTIFY:
      event->selection.time = the_time;
      break;
    case BDK_PROXIMITY_IN:
    case BDK_PROXIMITY_OUT:
      event->proximity.time = the_time;
      break;
    case BDK_DRAG_ENTER:
    case BDK_DRAG_LEAVE:
    case BDK_DRAG_MOTION:
    case BDK_DRAG_STATUS:
    case BDK_DROP_START:
    case BDK_DROP_FINISHED:
      event->dnd.time = the_time;
      break;
    case BDK_SCROLL:
      event->scroll.time = the_time;
      break;
    case BDK_FOCUS_CHANGE:
    case BDK_CONFIGURE:
    case BDK_MAP:
    case BDK_UNMAP:
    case BDK_CLIENT_EVENT:
    case BDK_VISIBILITY_NOTIFY:
    case BDK_NO_EXPOSE:
    case BDK_DELETE:
    case BDK_DESTROY:
    case BDK_EXPOSE:
    default:
      break;
    }
}

BdkEvent *
bdk_directfb_event_make (BdkWindow    *window,
                         BdkEventType  type)
{
  BdkEvent *event = bdk_event_new (BDK_NOTHING);

  bdk_directfb_event_fill (event, window, type);

  _bdk_event_queue_append (bdk_display_get_default (), event);

  return event;
}

void
bdk_error_trap_push (void)
{
}

gint
bdk_error_trap_pop (void)
{
  return 0;
}

BdkGrabStatus
bdk_keyboard_grab (BdkWindow *window,
                   gint       owner_events,
                   guint32    time)
{
  return bdk_directfb_keyboard_grab (bdk_display_get_default(),
                                     window,
                                     owner_events,
                                     time);
}

/*
 *--------------------------------------------------------------
 * bdk_pointer_grab
 *
 *   Grabs the pointer to a specific window
 *
 * Arguments:
 *   "window" is the window which will receive the grab
 *   "owner_events" specifies whether events will be reported as is,
 *     or relative to "window"
 *   "event_mask" masks only interesting events
 *   "confine_to" limits the cursor movement to the specified window
 *   "cursor" changes the cursor for the duration of the grab
 *   "time" specifies the time
 *
 * Results:
 *
 * Side effects:
 *   requires a corresponding call to bdk_pointer_ungrab
 *
 *--------------------------------------------------------------
 */


BdkGrabStatus
_bdk_windowing_pointer_grab (BdkWindow    *window,
                             BdkWindow    *native,
                             gboolean      owner_events,
                             BdkEventMask  event_mask,
                             BdkWindow    *confine_to,
                             BdkCursor    *cursor,
                             guint32       time)
{
  g_return_val_if_fail (BDK_IS_WINDOW (window), 0);
  g_return_val_if_fail (confine_to == NULL || BDK_IS_WINDOW (confine_to), 0);

  _bdk_display_add_pointer_grab (&_bdk_display->parent,
                                 window,
                                 native,
                                 owner_events,
                                 event_mask,
                                 0,
                                 time,
                                 FALSE);

  return BDK_GRAB_SUCCESS;
}

#define __BDK_MAIN_X11_C__
#include "bdkaliasdef.c"
