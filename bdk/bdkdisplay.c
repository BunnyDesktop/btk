/* BDK - The GIMP Drawing Kit
 * bdkdisplay.c
 * 
 * Copyright 2001 Sun Microsystems Inc. 
 *
 * Erwann Chenede <erwann.chenede@sun.com>
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

#include "config.h"
#include <math.h>
#include <bunnylib.h>
#include "bdk.h"		/* bdk_event_send_client_message() */
#include "bdkdisplay.h"
#include "bdkwindowimpl.h"
#include "bdkinternals.h"
#include "bdkmarshalers.h"
#include "bdkscreen.h"
#include "bdkalias.h"

enum {
  CLOSED,
  LAST_SIGNAL
};

static void bdk_display_dispose    (BObject         *object);
static void bdk_display_finalize   (BObject         *object);


static void       singlehead_get_pointer (BdkDisplay       *display,
					  BdkScreen       **screen,
					  bint             *x,
					  bint             *y,
					  BdkModifierType  *mask);
static BdkWindow* singlehead_window_get_pointer (BdkDisplay       *display,
						 BdkWindow        *window,
						 bint             *x,
						 bint             *y,
						 BdkModifierType  *mask);
static BdkWindow* singlehead_window_at_pointer  (BdkDisplay       *display,
						 bint             *win_x,
						 bint             *win_y);

static BdkWindow* singlehead_default_window_get_pointer (BdkWindow       *window,
							 bint            *x,
							 bint            *y,
							 BdkModifierType *mask);
static BdkWindow* singlehead_default_window_at_pointer  (BdkScreen       *screen,
							 bint            *win_x,
							 bint            *win_y);
static BdkWindow *bdk_window_real_window_get_pointer     (BdkDisplay       *display,
                                                          BdkWindow        *window,
                                                          bint             *x,
                                                          bint             *y,
                                                          BdkModifierType  *mask);
static BdkWindow *bdk_display_real_get_window_at_pointer (BdkDisplay       *display,
                                                          bint             *win_x,
                                                          bint             *win_y);

static buint signals[LAST_SIGNAL] = { 0 };

static char *bdk_sm_client_id;

static const BdkDisplayPointerHooks default_pointer_hooks = {
  _bdk_windowing_get_pointer,
  bdk_window_real_window_get_pointer,
  bdk_display_real_get_window_at_pointer
};

static const BdkDisplayPointerHooks singlehead_pointer_hooks = {
  singlehead_get_pointer,
  singlehead_window_get_pointer,
  singlehead_window_at_pointer
};

static const BdkPointerHooks singlehead_default_pointer_hooks = {
  singlehead_default_window_get_pointer,
  singlehead_default_window_at_pointer
};

static const BdkPointerHooks *singlehead_current_pointer_hooks = &singlehead_default_pointer_hooks;

G_DEFINE_TYPE (BdkDisplay, bdk_display, B_TYPE_OBJECT)

static void
bdk_display_class_init (BdkDisplayClass *class)
{
  BObjectClass *object_class = B_OBJECT_CLASS (class);
  
  object_class->finalize = bdk_display_finalize;
  object_class->dispose = bdk_display_dispose;

  /**
   * BdkDisplay::closed:
   * @display: the object on which the signal is emitted
   * @is_error: %TRUE if the display was closed due to an error
   *
   * The ::closed signal is emitted when the connection to the windowing
   * system for @display is closed.
   *
   * Since: 2.2
   */   
  signals[CLOSED] =
    g_signal_new (g_intern_static_string ("closed"),
		  B_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BdkDisplayClass, closed),
		  NULL, NULL,
		  _bdk_marshal_VOID__BOOLEAN,
		  B_TYPE_NONE,
		  1,
		  B_TYPE_BOOLEAN);
}

static void
bdk_display_init (BdkDisplay *display)
{
  _bdk_displays = b_slist_prepend (_bdk_displays, display);

  display->button_click_time[0] = display->button_click_time[1] = 0;
  display->button_window[0] = display->button_window[1] = NULL;
  display->button_number[0] = display->button_number[1] = -1;
  display->button_x[0] = display->button_x[1] = 0;
  display->button_y[0] = display->button_y[1] = 0;

  display->double_click_time = 250;
  display->double_click_distance = 5;

  display->pointer_hooks = &default_pointer_hooks;
}

static void
bdk_display_dispose (BObject *object)
{
  BdkDisplay *display = BDK_DISPLAY_OBJECT (object);

  g_list_foreach (display->queued_events, (GFunc)bdk_event_free, NULL);
  g_list_free (display->queued_events);
  display->queued_events = NULL;
  display->queued_tail = NULL;

  _bdk_displays = b_slist_remove (_bdk_displays, object);

  if (bdk_display_get_default() == display)
    {
      if (_bdk_displays)
        bdk_display_manager_set_default_display (bdk_display_manager_get(),
                                                 _bdk_displays->data);
      else
        bdk_display_manager_set_default_display (bdk_display_manager_get(),
                                                 NULL);
    }

  B_OBJECT_CLASS (bdk_display_parent_class)->dispose (object);
}

static void
bdk_display_finalize (BObject *object)
{
  B_OBJECT_CLASS (bdk_display_parent_class)->finalize (object);
}

/**
 * bdk_display_close:
 * @display: a #BdkDisplay
 *
 * Closes the connection to the windowing system for the given display,
 * and cleans up associated resources.
 *
 * Since: 2.2
 */
void
bdk_display_close (BdkDisplay *display)
{
  g_return_if_fail (BDK_IS_DISPLAY (display));

  if (!display->closed)
    {
      display->closed = TRUE;
      
      g_signal_emit (display, signals[CLOSED], 0, FALSE);
      g_object_run_dispose (B_OBJECT (display));
      
      g_object_unref (display);
    }
}

/**
 * bdk_display_is_closed:
 * @display: a #BdkDisplay
 *
 * Finds out if the display has been closed.
 *
 * Returns: %TRUE if the display is closed.
 *
 * Since: 2.22
 */
bboolean
bdk_display_is_closed  (BdkDisplay  *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), FALSE);

  return display->closed;
}

/**
 * bdk_display_get_event:
 * @display: a #BdkDisplay
 * 
 * Gets the next #BdkEvent to be processed for @display, fetching events from the
 * windowing system if necessary.
 * 
 * Return value: the next #BdkEvent to be processed, or %NULL if no events
 * are pending. The returned #BdkEvent should be freed with bdk_event_free().
 *
 * Since: 2.2
 **/
BdkEvent*
bdk_display_get_event (BdkDisplay *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);
  
  _bdk_events_queue (display);
  return _bdk_event_unqueue (display);
}

/**
 * bdk_display_peek_event:
 * @display: a #BdkDisplay 
 * 
 * Gets a copy of the first #BdkEvent in the @display's event queue, without
 * removing the event from the queue.  (Note that this function will
 * not get more events from the windowing system.  It only checks the events
 * that have already been moved to the BDK event queue.)
 * 
 * Return value: a copy of the first #BdkEvent on the event queue, or %NULL 
 * if no events are in the queue. The returned #BdkEvent should be freed with
 * bdk_event_free().
 *
 * Since: 2.2
 **/
BdkEvent*
bdk_display_peek_event (BdkDisplay *display)
{
  GList *tmp_list;

  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);

  tmp_list = _bdk_event_queue_find_first (display);
  
  if (tmp_list)
    return bdk_event_copy (tmp_list->data);
  else
    return NULL;
}

/**
 * bdk_display_put_event:
 * @display: a #BdkDisplay
 * @event: a #BdkEvent.
 *
 * Appends a copy of the given event onto the front of the event
 * queue for @display.
 *
 * Since: 2.2
 **/
void
bdk_display_put_event (BdkDisplay     *display,
		       const BdkEvent *event)
{
  g_return_if_fail (BDK_IS_DISPLAY (display));
  g_return_if_fail (event != NULL);

  _bdk_event_queue_append (display, bdk_event_copy (event));
  /* If the main loop is blocking in a different thread, wake it up */
  g_main_context_wakeup (NULL); 
}

/**
 * bdk_pointer_ungrab:
 * @time_: a timestamp from a #BdkEvent, or %BDK_CURRENT_TIME if no 
 *  timestamp is available.
 *
 * Ungrabs the pointer on the default display, if it is grabbed by this 
 * application.
 **/
void
bdk_pointer_ungrab (buint32 time)
{
  bdk_display_pointer_ungrab (bdk_display_get_default (), time);
}

/**
 * bdk_pointer_is_grabbed:
 * 
 * Returns %TRUE if the pointer on the default display is currently 
 * grabbed by this application.
 *
 * Note that this does not take the inmplicit pointer grab on button
 * presses into account.

 * Return value: %TRUE if the pointer is currently grabbed by this application.* 
 **/
bboolean
bdk_pointer_is_grabbed (void)
{
  return bdk_display_pointer_is_grabbed (bdk_display_get_default ());
}

/**
 * bdk_keyboard_ungrab:
 * @time_: a timestamp from a #BdkEvent, or %BDK_CURRENT_TIME if no
 *        timestamp is available.
 * 
 * Ungrabs the keyboard on the default display, if it is grabbed by this 
 * application.
 **/
void
bdk_keyboard_ungrab (buint32 time)
{
  bdk_display_keyboard_ungrab (bdk_display_get_default (), time);
}

/**
 * bdk_beep:
 * 
 * Emits a short beep on the default display.
 **/
void
bdk_beep (void)
{
  bdk_display_beep (bdk_display_get_default ());
}

/**
 * bdk_event_send_client_message:
 * @event: the #BdkEvent to send, which should be a #BdkEventClient.
 * @winid:  the window to send the X ClientMessage event to.
 * 
 * Sends an X ClientMessage event to a given window (which must be
 * on the default #BdkDisplay.)
 * This could be used for communicating between different applications,
 * though the amount of data is limited to 20 bytes.
 * 
 * Return value: non-zero on success.
 **/
bboolean
bdk_event_send_client_message (BdkEvent        *event,
			       BdkNativeWindow  winid)
{
  g_return_val_if_fail (event != NULL, FALSE);

  return bdk_event_send_client_message_for_display (bdk_display_get_default (),
						    event, winid);
}

/**
 * bdk_event_send_clientmessage_toall:
 * @event: the #BdkEvent to send, which should be a #BdkEventClient.
 *
 * Sends an X ClientMessage event to all toplevel windows on the default
 * #BdkScreen.
 *
 * Toplevel windows are determined by checking for the WM_STATE property, as
 * described in the Inter-Client Communication Conventions Manual (ICCCM).
 * If no windows are found with the WM_STATE property set, the message is sent
 * to all children of the root window.
 **/
void
bdk_event_send_clientmessage_toall (BdkEvent *event)
{
  g_return_if_fail (event != NULL);

  bdk_screen_broadcast_client_message (bdk_screen_get_default (), event);
}

/**
 * bdk_device_get_core_pointer:
 * 
 * Returns the core pointer device for the default display.
 * 
 * Return value: the core pointer device; this is owned by the
 *   display and should not be freed.
 **/
BdkDevice *
bdk_device_get_core_pointer (void)
{
  return bdk_display_get_core_pointer (bdk_display_get_default ());
}

/**
 * bdk_display_get_core_pointer:
 * @display: a #BdkDisplay
 * 
 * Returns the core pointer device for the given display
 * 
 * Return value: the core pointer device; this is owned by the
 *   display and should not be freed.
 *
 * Since: 2.2
 **/
BdkDevice *
bdk_display_get_core_pointer (BdkDisplay *display)
{
  return display->core_pointer;
}

/**
 * bdk_set_sm_client_id:
 * @sm_client_id: the client id assigned by the session manager when the
 *    connection was opened, or %NULL to remove the property.
 * 
 * Sets the <literal>SM_CLIENT_ID</literal> property on the application's leader window so that
 * the window manager can save the application's state using the X11R6 ICCCM
 * session management protocol.
 *
 * See the X Session Management Library documentation for more information on
 * session management and the Inter-Client Communication Conventions Manual
 * (ICCCM) for information on the <literal>WM_CLIENT_LEADER</literal> property. 
 * (Both documents are part of the X Window System distribution.)
 *
 * Deprecated:2.24: Use bdk_x11_set_sm_client_id() instead
 **/
void
bdk_set_sm_client_id (const bchar* sm_client_id)
{
  GSList *displays, *tmp_list;
  
  g_free (bdk_sm_client_id);
  bdk_sm_client_id = g_strdup (sm_client_id);

  displays = bdk_display_manager_list_displays (bdk_display_manager_get ());
  for (tmp_list = displays; tmp_list; tmp_list = tmp_list->next)
    _bdk_windowing_display_set_sm_client_id (tmp_list->data, sm_client_id);

  b_slist_free (displays);
}

/**
 * _bdk_get_sm_client_id:
 * 
 * Gets the client ID set with bdk_set_sm_client_id(), if any.
 * 
 * Return value: Session ID, or %NULL if bdk_set_sm_client_id()
 *               has never been called.
 **/
const char *
_bdk_get_sm_client_id (void)
{
  return bdk_sm_client_id;
}

void
_bdk_display_enable_motion_hints (BdkDisplay *display)
{
  bulong serial;
  
  if (display->pointer_info.motion_hint_serial != 0)
    {
      serial = _bdk_windowing_window_get_next_serial (display);
      /* We might not actually generate the next request, so
	 make sure this triggers always, this may cause it to
	 trigger slightly too early, but this is just a hint
	 anyway. */
      if (serial > 0)
	serial--;
      if (serial < display->pointer_info.motion_hint_serial)
	display->pointer_info.motion_hint_serial = serial;
    }
}

/**
 * bdk_display_get_pointer:
 * @display: a #BdkDisplay
 * @screen: (out) (allow-none): location to store the screen that the
 *          cursor is on, or %NULL.
 * @x: (out) (allow-none): location to store root window X coordinate of pointer, or %NULL.
 * @y: (out) (allow-none): location to store root window Y coordinate of pointer, or %NULL.
 * @mask: (out) (allow-none): location to store current modifier mask, or %NULL
 *
 * Gets the current location of the pointer and the current modifier
 * mask for a given display.
 *
 * Since: 2.2
 **/
void
bdk_display_get_pointer (BdkDisplay      *display,
			 BdkScreen      **screen,
			 bint            *x,
			 bint            *y,
			 BdkModifierType *mask)
{
  BdkScreen *tmp_screen;
  bint tmp_x, tmp_y;
  BdkModifierType tmp_mask;
  
  g_return_if_fail (BDK_IS_DISPLAY (display));

  display->pointer_hooks->get_pointer (display, &tmp_screen, &tmp_x, &tmp_y, &tmp_mask);

  if (screen)
    *screen = tmp_screen;
  if (x)
    *x = tmp_x;
  if (y)
    *y = tmp_y;
  if (mask)
    *mask = tmp_mask;
}

static BdkWindow *
bdk_display_real_get_window_at_pointer (BdkDisplay *display,
                                        bint       *win_x,
                                        bint       *win_y)
{
  BdkWindow *window;
  bint x, y;

  window = _bdk_windowing_window_at_pointer (display, &x, &y, NULL, FALSE);

  /* This might need corrections, as the native window returned
     may contain client side children */
  if (window)
    {
      double xx, yy;

      window = _bdk_window_find_descendant_at (window,
					       x, y,
					       &xx, &yy);
      x = floor (xx + 0.5);
      y = floor (yy + 0.5);
    }

  *win_x = x;
  *win_y = y;

  return window;
}

static BdkWindow *
bdk_window_real_window_get_pointer (BdkDisplay       *display,
                                    BdkWindow        *window,
                                    bint             *x,
                                    bint             *y,
                                    BdkModifierType  *mask)
{
  BdkWindowObject *private;
  bint tmpx, tmpy;
  BdkModifierType tmp_mask;
  bboolean normal_child;

  private = (BdkWindowObject *) window;

  normal_child = BDK_WINDOW_IMPL_GET_IFACE (private->impl)->get_pointer (window,
									 &tmpx, &tmpy,
									 &tmp_mask);
  /* We got the coords on the impl, convert to the window */
  tmpx -= private->abs_x;
  tmpy -= private->abs_y;

  if (x)
    *x = tmpx;
  if (y)
    *y = tmpy;
  if (mask)
    *mask = tmp_mask;

  if (normal_child)
    return _bdk_window_find_child_at (window, tmpx, tmpy);
  return NULL;
}

/**
 * bdk_display_get_window_at_pointer:
 * @display: a #BdkDisplay
 * @win_x: (out) (allow-none): return location for x coordinate of the pointer location relative
 *    to the window origin, or %NULL
 * @win_y: (out) (allow-none): return location for y coordinate of the pointer location relative
 &    to the window origin, or %NULL
 *
 * Obtains the window underneath the mouse pointer, returning the location
 * of the pointer in that window in @win_x, @win_y for @screen. Returns %NULL
 * if the window under the mouse pointer is not known to BDK (for example, 
 * belongs to another application).
 *
 * Returns: (transfer none): the window under the mouse pointer, or %NULL
 *
 * Since: 2.2
 **/
BdkWindow *
bdk_display_get_window_at_pointer (BdkDisplay *display,
				   bint       *win_x,
				   bint       *win_y)
{
  bint tmp_x, tmp_y;
  BdkWindow *window;

  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);

  window = display->pointer_hooks->window_at_pointer (display, &tmp_x, &tmp_y);

  if (win_x)
    *win_x = tmp_x;
  if (win_y)
    *win_y = tmp_y;

  return window;
}

/**
 * bdk_display_set_pointer_hooks:
 * @display: a #BdkDisplay
 * @new_hooks: a table of pointers to functions for getting
 *   quantities related to the current pointer position,
 *   or %NULL to restore the default table.
 * 
 * This function allows for hooking into the operation
 * of getting the current location of the pointer on a particular
 * display. This is only useful for such low-level tools as an
 * event recorder. Applications should never have any
 * reason to use this facility.
 *
 * Return value: the previous pointer hook table
 *
 * Since: 2.2
 *
 * Deprecated: 2.24: This function will go away in BTK 3 for lack of use cases.
 **/
BdkDisplayPointerHooks *
bdk_display_set_pointer_hooks (BdkDisplay                   *display,
			       const BdkDisplayPointerHooks *new_hooks)
{
  const BdkDisplayPointerHooks *result;

  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);
  result = display->pointer_hooks;

  if (new_hooks)
    display->pointer_hooks = new_hooks;
  else
    display->pointer_hooks = &default_pointer_hooks;

  return (BdkDisplayPointerHooks *)result;
}

static void
singlehead_get_pointer (BdkDisplay       *display,
			BdkScreen       **screen,
			bint             *x,
			bint             *y,
			BdkModifierType  *mask)
{
  BdkScreen *default_screen = bdk_display_get_default_screen (display);
  BdkWindow *root_window = bdk_screen_get_root_window (default_screen);

  *screen = default_screen;

  singlehead_current_pointer_hooks->get_pointer (root_window, x, y, mask);
}

static BdkWindow*
singlehead_window_get_pointer (BdkDisplay       *display,
			       BdkWindow        *window,
			       bint             *x,
			       bint             *y,
			       BdkModifierType  *mask)
{
  return singlehead_current_pointer_hooks->get_pointer (window, x, y, mask);
}

static BdkWindow*
singlehead_window_at_pointer   (BdkDisplay *display,
				bint       *win_x,
				bint       *win_y)
{
  BdkScreen *default_screen = bdk_display_get_default_screen (display);

  return singlehead_current_pointer_hooks->window_at_pointer (default_screen,
							      win_x, win_y);
}

static BdkWindow*
singlehead_default_window_get_pointer (BdkWindow       *window,
				       bint            *x,
				       bint            *y,
				       BdkModifierType *mask)
{
  return bdk_window_real_window_get_pointer (bdk_drawable_get_display (window),
                                             window, x, y, mask);
}

static BdkWindow*
singlehead_default_window_at_pointer  (BdkScreen       *screen,
				       bint            *win_x,
				       bint            *win_y)
{
  return bdk_display_real_get_window_at_pointer (bdk_screen_get_display (screen),
                                                 win_x, win_y);
}

/**
 * bdk_set_pointer_hooks:
 * @new_hooks: a table of pointers to functions for getting
 *   quantities related to the current pointer position,
 *   or %NULL to restore the default table.
 * 
 * This function allows for hooking into the operation
 * of getting the current location of the pointer. This
 * is only useful for such low-level tools as an
 * event recorder. Applications should never have any
 * reason to use this facility.
 *
 * This function is not multihead safe. For multihead operation,
 * see bdk_display_set_pointer_hooks().
 * 
 * Return value: the previous pointer hook table
 *
 * Deprecated: 2.24: This function will go away in BTK 3 for lack of use cases.
 **/
BdkPointerHooks *
bdk_set_pointer_hooks (const BdkPointerHooks *new_hooks)
{
  const BdkPointerHooks *result = singlehead_current_pointer_hooks;

  if (new_hooks)
    singlehead_current_pointer_hooks = new_hooks;
  else
    singlehead_current_pointer_hooks = &singlehead_default_pointer_hooks;

  bdk_display_set_pointer_hooks (bdk_display_get_default (),
				 &singlehead_pointer_hooks);
  
  return (BdkPointerHooks *)result;
}

static void
generate_grab_broken_event (BdkWindow *window,
			    bboolean   keyboard,
			    bboolean   implicit,
			    BdkWindow *grab_window)
{
  g_return_if_fail (window != NULL);

  if (!BDK_WINDOW_DESTROYED (window))
    {
      BdkEvent event;
      event.type = BDK_GRAB_BROKEN;
      event.grab_broken.window = window;
      event.grab_broken.send_event = 0;
      event.grab_broken.keyboard = keyboard;
      event.grab_broken.implicit = implicit;
      event.grab_broken.grab_window = grab_window;
      bdk_event_put (&event);
    }
}

BdkPointerGrabInfo *
_bdk_display_get_last_pointer_grab (BdkDisplay *display)
{
  GList *l;

  l = g_list_last (display->pointer_grabs);

  if (l == NULL)
    return NULL;
  else
    return (BdkPointerGrabInfo *)l->data;
}


BdkPointerGrabInfo *
_bdk_display_add_pointer_grab (BdkDisplay *display,
			       BdkWindow *window,
			       BdkWindow *native_window,
			       bboolean owner_events,
			       BdkEventMask event_mask,
			       unsigned long serial_start,
			       buint32 time,
			       bboolean implicit)
{
  BdkPointerGrabInfo *info, *other_info;
  GList *l;

  info = g_new0 (BdkPointerGrabInfo, 1);

  info->window = g_object_ref (window);
  info->native_window = g_object_ref (native_window);
  info->serial_start = serial_start;
  info->serial_end = B_MAXULONG;
  info->owner_events = owner_events;
  info->event_mask = event_mask;
  info->time = time;
  info->implicit = implicit;

  /* Find the first grab that has a larger start time (if any) and insert
   * before that. I.E we insert after already existing grabs with same
   * start time */
  for (l = display->pointer_grabs; l != NULL; l = l->next)
    {
      other_info = l->data;
      
      if (info->serial_start < other_info->serial_start)
	break;
    }
  display->pointer_grabs =
    g_list_insert_before (display->pointer_grabs, l, info);

  /* Make sure the new grab end before next grab */
  if (l)
    {
      other_info = l->data;
      info->serial_end = other_info->serial_start;
    }
  
  /* Find any previous grab and update its end time */
  l = g_list_find  (display->pointer_grabs, info);
  l = l->prev;
  if (l)
    {
      other_info = l->data;
      other_info->serial_end = serial_start;
    }

  return info;
}

static void
free_pointer_grab (BdkPointerGrabInfo *info)
{
  g_object_unref (info->window);
  g_object_unref (info->native_window);
  g_free (info);
}

/* _bdk_synthesize_crossing_events only works inside one toplevel.
   This function splits things into two calls if needed, converting the
   coordinates to the right toplevel */
static void
synthesize_crossing_events (BdkDisplay *display,
			    BdkWindow *src_window,
			    BdkWindow *dest_window,
			    BdkCrossingMode crossing_mode,
			    buint32 time,
			    bulong serial)
{
  BdkWindow *src_toplevel, *dest_toplevel;
  BdkModifierType state;
  int x, y;

  /* We use the native crossing events if all native */
  if (_bdk_native_windows)
    return;
  
  if (src_window)
    src_toplevel = bdk_window_get_toplevel (src_window);
  else
    src_toplevel = NULL;
  if (dest_window)
    dest_toplevel = bdk_window_get_toplevel (dest_window);
  else
    dest_toplevel = NULL;

  if (src_toplevel == NULL && dest_toplevel == NULL)
    return;
  
  if (src_toplevel == NULL ||
      src_toplevel == dest_toplevel)
    {
      /* Same toplevels */
      bdk_window_get_pointer (dest_toplevel,
			      &x, &y, &state);
      _bdk_synthesize_crossing_events (display,
				       src_window,
				       dest_window,
				       crossing_mode,
				       x, y, state,
				       time,
				       NULL,
				       serial, FALSE);
    }
  else if (dest_toplevel == NULL)
    {
      bdk_window_get_pointer (src_toplevel,
			      &x, &y, &state);
      _bdk_synthesize_crossing_events (display,
				       src_window,
				       NULL,
				       crossing_mode,
				       x, y, state,
				       time,
				       NULL,
				       serial, FALSE);
    }
  else
    {
      /* Different toplevels */
      bdk_window_get_pointer (src_toplevel,
			      &x, &y, &state);
      _bdk_synthesize_crossing_events (display,
				       src_window,
				       NULL,
				       crossing_mode,
				       x, y, state,
				       time,
				       NULL,
				       serial, FALSE);
      bdk_window_get_pointer (dest_toplevel,
			      &x, &y, &state);
      _bdk_synthesize_crossing_events (display,
				       NULL,
				       dest_window,
				       crossing_mode,
				       x, y, state,
				       time,
				       NULL,
				       serial, FALSE);
    }
}

static BdkWindow *
get_current_toplevel (BdkDisplay *display,
		      int *x_out, int *y_out,
		      BdkModifierType *state_out)
{
  BdkWindow *pointer_window;
  int x, y;
  BdkModifierType state;

  pointer_window = _bdk_windowing_window_at_pointer (display,  &x, &y, &state, TRUE);
  if (pointer_window != NULL &&
      (BDK_WINDOW_DESTROYED (pointer_window) ||
       BDK_WINDOW_TYPE (pointer_window) == BDK_WINDOW_ROOT ||
       BDK_WINDOW_TYPE (pointer_window) == BDK_WINDOW_FOREIGN))
    pointer_window = NULL;

  *x_out = x;
  *y_out = y;
  *state_out = state;
  return pointer_window;
}

static void
switch_to_pointer_grab (BdkDisplay *display,
			BdkPointerGrabInfo *grab,
			BdkPointerGrabInfo *last_grab,
			buint32 time,
			bulong serial)
{
  BdkWindow *src_window, *pointer_window, *new_toplevel;
  GList *old_grabs;
  BdkModifierType state;
  int x, y;

  /* Temporarily unset pointer to make sure we send the crossing events below */
  old_grabs = display->pointer_grabs;
  display->pointer_grabs = NULL;
  
  if (grab)
    {
      /* New grab is in effect */
      
      /* We need to generate crossing events for the grab.
       * However, there are never any crossing events for implicit grabs
       * TODO: ... Actually, this could happen if the pointer window
       *           doesn't have button mask so a parent gets the event... 
       */
      if (!grab->implicit)
	{
	  /* We send GRAB crossing events from the window under the pointer to the
	     grab window. Except if there is an old grab then we start from that */
	  if (last_grab)
	    src_window = last_grab->window;
	  else
	    src_window = display->pointer_info.window_under_pointer;
	  
	  if (src_window != grab->window)
	    {
	      synthesize_crossing_events (display,
					  src_window, grab->window,
					  BDK_CROSSING_GRAB, time, serial);
	    }

	  /* !owner_event Grabbing a window that we're not inside, current status is
	     now NULL (i.e. outside grabbed window) */
	  if (!grab->owner_events && display->pointer_info.window_under_pointer != grab->window)
	    _bdk_display_set_window_under_pointer (display, NULL);
	}

      grab->activated = TRUE;
    }

  if (last_grab)
    {
      new_toplevel = NULL;

      if (grab == NULL /* ungrab */ ||
	  (!last_grab->owner_events && grab->owner_events) /* switched to owner_events */ )
	{
	  /* We force check what window we're in, and update the toplevel_under_pointer info,
	   * as that won't get told of this change with toplevel enter events.
	   */
	  if (display->pointer_info.toplevel_under_pointer)
	    g_object_unref (display->pointer_info.toplevel_under_pointer);
	  display->pointer_info.toplevel_under_pointer = NULL;

	  new_toplevel = get_current_toplevel (display, &x, &y, &state);
	  if (new_toplevel)
	    {
	      /* w is now toplevel and x,y in toplevel coords */
	      display->pointer_info.toplevel_under_pointer = g_object_ref (new_toplevel);
	      display->pointer_info.toplevel_x = x;
	      display->pointer_info.toplevel_y = y;
	      display->pointer_info.state = state;
	    }
	}

      if (grab == NULL) /* Ungrabbed, send events */
	{
	  pointer_window = NULL;
	  if (new_toplevel)
	    {
	      /* Find (possibly virtual) child window */
	      pointer_window =
		_bdk_window_find_descendant_at (new_toplevel,
						x, y,
						NULL, NULL);
	    }
	  
	  if (pointer_window != last_grab->window)
	    synthesize_crossing_events (display,
					last_grab->window, pointer_window,
					BDK_CROSSING_UNGRAB, time, serial);
	  
	  /* We're now ungrabbed, update the window_under_pointer */
	  _bdk_display_set_window_under_pointer (display, pointer_window);
	}
    }
  
  display->pointer_grabs = old_grabs;

}

void
_bdk_display_pointer_grab_update (BdkDisplay *display,
				  bulong current_serial)
{
  BdkPointerGrabInfo *current_grab, *next_grab;
  buint32 time;
  
  time = display->last_event_time;

  while (display->pointer_grabs != NULL)
    {
      current_grab = display->pointer_grabs->data;

      if (current_grab->serial_start > current_serial)
	return; /* Hasn't started yet */
      
      if (current_grab->serial_end > current_serial)
	{
	  /* This one hasn't ended yet.
	     its the currently active one or scheduled to be active */

	  if (!current_grab->activated)
	    switch_to_pointer_grab (display, current_grab, NULL, time, current_serial);
	  
	  break;
	}


      next_grab = NULL;
      if (display->pointer_grabs->next)
	{
	  /* This is the next active grab */
	  next_grab = display->pointer_grabs->next->data;
	  
	  if (next_grab->serial_start > current_serial)
	    next_grab = NULL; /* Actually its not yet active */
	}

      if ((next_grab == NULL && current_grab->implicit_ungrab) ||
	  (next_grab != NULL && current_grab->window != next_grab->window))
	generate_grab_broken_event (BDK_WINDOW (current_grab->window),
				    FALSE, current_grab->implicit,
				    next_grab? next_grab->window : NULL);

      /* Remove old grab */
      display->pointer_grabs =
	g_list_delete_link (display->pointer_grabs,
			    display->pointer_grabs);
      
      switch_to_pointer_grab (display,
			      next_grab, current_grab,
			      time, current_serial);
      
      free_pointer_grab (current_grab);
    }
}

static GList *
find_pointer_grab (BdkDisplay *display,
		   bulong serial)
{
  BdkPointerGrabInfo *grab;
  GList *l;

  for (l = display->pointer_grabs; l != NULL; l = l->next)
    {
      grab = l->data;

      if (serial >= grab->serial_start && serial < grab->serial_end)
	return l;
    }
  
  return NULL;
}



BdkPointerGrabInfo *
_bdk_display_has_pointer_grab (BdkDisplay *display,
			       bulong serial)
{
  GList *l;

  l = find_pointer_grab (display, serial);
  if (l)
    return l->data;
  
  return NULL;
}

/* Returns true if last grab was ended
 * If if_child is non-NULL, end the grab only if the grabbed
 * window is the same as if_child or a descendant of it */
bboolean
_bdk_display_end_pointer_grab (BdkDisplay *display,
			       bulong serial,
			       BdkWindow *if_child,
			       bboolean implicit)
{
  BdkPointerGrabInfo *grab;
  GList *l;

  l = find_pointer_grab (display, serial);
  
  if (l == NULL)
    return FALSE;

  grab = l->data;
  if (grab &&
      (if_child == NULL ||
       _bdk_window_event_parent_of (if_child, grab->window)))
    {
      grab->serial_end = serial;
      grab->implicit_ungrab = implicit;
      return l->next == NULL;
    }
  
  return FALSE;
}

void
_bdk_display_set_has_keyboard_grab (BdkDisplay *display,
				    BdkWindow *window,
				    BdkWindow *native_window,
				    bboolean owner_events,
				    unsigned long serial,
				    buint32 time)
{
  if (display->keyboard_grab.window != NULL &&
      display->keyboard_grab.window != window)
    generate_grab_broken_event (display->keyboard_grab.window,
				TRUE, FALSE, window);
  
  display->keyboard_grab.window = window;
  display->keyboard_grab.native_window = native_window;
  display->keyboard_grab.owner_events = owner_events;
  display->keyboard_grab.serial = serial;
  display->keyboard_grab.time = time;      
}

void
_bdk_display_unset_has_keyboard_grab (BdkDisplay *display,
				      bboolean implicit)
{
  if (implicit)
    generate_grab_broken_event (display->keyboard_grab.window,
				TRUE, FALSE, NULL);
  display->keyboard_grab.window = NULL;  
}

/**
 * bdk_keyboard_grab_info_libbtk_only:
 * @display: the display for which to get the grab information
 * @grab_window: location to store current grab window
 * @owner_events: location to store boolean indicating whether
 *   the @owner_events flag to bdk_keyboard_grab() was %TRUE.
 * 
 * Determines information about the current keyboard grab.
 * This is not public API and must not be used by applications.
 * 
 * Return value: %TRUE if this application currently has the
 *  keyboard grabbed.
 **/
bboolean
bdk_keyboard_grab_info_libbtk_only (BdkDisplay *display,
				    BdkWindow **grab_window,
				    bboolean   *owner_events)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), FALSE);

  if (display->keyboard_grab.window)
    {
      if (grab_window)
        *grab_window = display->keyboard_grab.window;
      if (owner_events)
        *owner_events = display->keyboard_grab.owner_events;

      return TRUE;
    }
  else
    return FALSE;
}

/**
 * bdk_pointer_grab_info_libbtk_only:
 * @display: the #BdkDisplay for which to get the grab information
 * @grab_window: location to store current grab window
 * @owner_events: location to store boolean indicating whether
 *   the @owner_events flag to bdk_pointer_grab() was %TRUE.
 * 
 * Determines information about the current pointer grab.
 * This is not public API and must not be used by applications.
 * 
 * Return value: %TRUE if this application currently has the
 *  pointer grabbed.
 **/
bboolean
bdk_pointer_grab_info_libbtk_only (BdkDisplay *display,
				   BdkWindow **grab_window,
				   bboolean   *owner_events)
{
  BdkPointerGrabInfo *info;
  
  g_return_val_if_fail (BDK_IS_DISPLAY (display), FALSE);

  /* What we're interested in is the steady state (ie last grab),
     because we're interested e.g. if we grabbed so that we
     can ungrab, even if our grab is not active just yet. */
  info = _bdk_display_get_last_pointer_grab (display);
  
  if (info)
    {
      if (grab_window)
        *grab_window = info->window;
      if (owner_events)
        *owner_events = info->owner_events;

      return TRUE;
    }
  else
    return FALSE;
}


/**
 * bdk_display_pointer_is_grabbed:
 * @display: a #BdkDisplay
 *
 * Test if the pointer is grabbed.
 *
 * Returns: %TRUE if an active X pointer grab is in effect
 *
 * Since: 2.2
 */
bboolean
bdk_display_pointer_is_grabbed (BdkDisplay *display)
{
  BdkPointerGrabInfo *info;
  
  g_return_val_if_fail (BDK_IS_DISPLAY (display), TRUE);

  /* What we're interested in is the steady state (ie last grab),
     because we're interested e.g. if we grabbed so that we
     can ungrab, even if our grab is not active just yet. */
  info = _bdk_display_get_last_pointer_grab (display);
  
  return (info && !info->implicit);
}

#define __BDK_DISPLAY_C__
#include "bdkaliasdef.c"
