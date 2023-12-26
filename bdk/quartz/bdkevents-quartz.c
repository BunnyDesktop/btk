/* bdkevents-quartz.c
 *
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998-2002 Tor Lillqvist
 * Copyright (C) 2005-2008 Imendio AB
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

#include "config.h"
#include <sys/types.h>
#include <sys/sysctl.h>
#include <pthread.h>
#include <unistd.h>

#import <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

#include "bdkscreen.h"
#include "bdkkeysyms.h"
#include "bdkprivate-quartz.h"
#include "bdkinputprivate.h"

#define GRIP_WIDTH 15
#define GRIP_HEIGHT 15
#define BDK_LION_RESIZE 5

#define WINDOW_IS_TOPLEVEL(window)                      \
    (BDK_WINDOW_TYPE (window) != BDK_WINDOW_CHILD &&    \
     BDK_WINDOW_TYPE (window) != BDK_WINDOW_FOREIGN &&  \
     BDK_WINDOW_TYPE (window) != BDK_WINDOW_OFFSCREEN)

/* This is the window corresponding to the key window */
static BdkWindow   *current_keyboard_window;

/* This is the event mask from the last event */
static BdkEventMask current_event_mask;

static void append_event                        (BdkEvent  *event,
                                                 gboolean   windowing);

static BdkWindow *find_toplevel_under_pointer   (BdkDisplay *display,
                                                 NSPoint     screen_point,
                                                 gint       *x,
                                                 gint       *y);


NSEvent *
bdk_quartz_event_get_nsevent (BdkEvent *event)
{
  /* FIXME: If the event here is unallocated, we crash. */
  return ((BdkEventPrivate *) event)->windowing_data;
}

static void
bdk_quartz_ns_notification_callback (CFNotificationCenterRef  center,
                                     void                    *observer,
                                     CFStringRef              name,
                                     const void              *object,
                                     CFDictionaryRef          userInfo)
{
  BdkEvent new_event;

  new_event.type = BDK_SETTING;
  new_event.setting.window = bdk_screen_get_root_window (_bdk_screen);
  new_event.setting.send_event = FALSE;
  new_event.setting.action = BDK_SETTING_ACTION_CHANGED;
  new_event.setting.name = NULL;

  /* Translate name */
  if (CFStringCompare (name,
                       CFSTR("AppleNoRedisplayAppearancePreferenceChanged"),
                       0) == kCFCompareEqualTo)
    new_event.setting.name = "btk-primary-button-warps-slider";

  if (!new_event.setting.name)
    return;

  bdk_event_put (&new_event);
}

static void
bdk_quartz_events_init_notifications (void)
{
  static gboolean notifications_initialized = FALSE;

  if (notifications_initialized)
    return;
  notifications_initialized = TRUE;

  /* Initialize any handlers for notifications we want to push to BTK
   * through BdkEventSettings.
   */

  /* This is an undocumented *distributed* notification to listen for changes
   * in scrollbar jump behavior. It is used by LibreOffice and WebKit as well.
   */
  CFNotificationCenterAddObserver (CFNotificationCenterGetDistributedCenter (),
                                   NULL,
                                   &bdk_quartz_ns_notification_callback,
                                   CFSTR ("AppleNoRedisplayAppearancePreferenceChanged"),
                                   NULL,
                                   CFNotificationSuspensionBehaviorDeliverImmediately);
}

void
_bdk_events_init (void)
{
  _bdk_quartz_event_loop_init ();
  bdk_quartz_events_init_notifications ();

  current_keyboard_window = g_object_ref (_bdk_root);
}

gboolean
bdk_events_pending (void)
{
  return (_bdk_event_queue_find_first (_bdk_display) ||
	  (_bdk_quartz_event_loop_check_pending ()));
}

BdkEvent*
bdk_event_get_graphics_expose (BdkWindow *window)
{
  /* FIXME: Implement */
  return NULL;
}

BdkGrabStatus
bdk_keyboard_grab (BdkWindow  *window,
		   gint        owner_events,
		   guint32     time)
{
  BdkDisplay *display;
  BdkWindow  *toplevel;

  g_return_val_if_fail (window != NULL, 0);
  g_return_val_if_fail (BDK_IS_WINDOW (window), 0);

  display = bdk_drawable_get_display (window);
  toplevel = bdk_window_get_effective_toplevel (window);

  _bdk_display_set_has_keyboard_grab (display,
                                      window,
                                      toplevel,
                                      owner_events,
                                      0,
                                      time);

  return BDK_GRAB_SUCCESS;
}

void
bdk_display_keyboard_ungrab (BdkDisplay *display,
			     guint32     time)
{
  _bdk_display_unset_has_keyboard_grab (display, FALSE);
}

void
bdk_display_pointer_ungrab (BdkDisplay *display,
			    guint32     time)
{
  BdkPointerGrabInfo *grab;

  grab = _bdk_display_get_last_pointer_grab (display);
  if (grab)
    grab->serial_end = 0;

  _bdk_display_pointer_grab_update (display, 0);
}

BdkGrabStatus
_bdk_windowing_pointer_grab (BdkWindow    *window,
                             BdkWindow    *native,
                             gboolean	   owner_events,
                             BdkEventMask  event_mask,
                             BdkWindow    *confine_to,
                             BdkCursor    *cursor,
                             guint32       time)
{
  g_return_val_if_fail (BDK_IS_WINDOW (window), 0);
  g_return_val_if_fail (confine_to == NULL || BDK_IS_WINDOW (confine_to), 0);

  _bdk_display_add_pointer_grab (_bdk_display,
                                 window,
                                 native,
                                 owner_events,
                                 event_mask,
                                 0,
                                 time,
                                 FALSE);

  return BDK_GRAB_SUCCESS;
}

void
_bdk_quartz_events_break_all_grabs (guint32 time)
{
  BdkPointerGrabInfo *grab;

  if (_bdk_display->keyboard_grab.window)
    _bdk_display_unset_has_keyboard_grab (_bdk_display, FALSE);

  grab = _bdk_display_get_last_pointer_grab (_bdk_display);
  if (grab)
    {
      grab->serial_end = 0;
      grab->implicit_ungrab = TRUE;
    }

  _bdk_display_pointer_grab_update (_bdk_display, 0);
}

static void
fixup_event (BdkEvent *event)
{
  if (event->any.window)
    g_object_ref (event->any.window);
  if (((event->any.type == BDK_ENTER_NOTIFY) ||
       (event->any.type == BDK_LEAVE_NOTIFY)) &&
      (event->crossing.subwindow != NULL))
    g_object_ref (event->crossing.subwindow);
  event->any.send_event = FALSE;
}

static void
append_event (BdkEvent *event,
              gboolean  windowing)
{
  GList *node;

  fixup_event (event);
  node = _bdk_event_queue_append (_bdk_display, event);

  if (windowing)
    _bdk_windowing_got_event (_bdk_display, node, event, 0);
}

static gint
bdk_event_apply_filters (NSEvent *nsevent,
			 BdkEvent *event,
			 GList **filters)
{
  GList *tmp_list;
  BdkFilterReturn result;
  
  tmp_list = *filters;

  while (tmp_list)
    {
      BdkEventFilter *filter = (BdkEventFilter*) tmp_list->data;
      GList *node;

      if ((filter->flags & BDK_EVENT_FILTER_REMOVED) != 0)
        {
          tmp_list = tmp_list->next;
          continue;
        }

      filter->ref_count++;
      result = filter->function (nsevent, event, filter->data);

      /* get the next node after running the function since the
         function may add or remove a next node */
      node = tmp_list;
      tmp_list = tmp_list->next;

      filter->ref_count--;
      if (filter->ref_count == 0)
        {
          *filters = g_list_remove_link (*filters, node);
          g_list_free_1 (node);
          g_free (filter);
        }

      if (result != BDK_FILTER_CONTINUE)
        return result;
    }

  return BDK_FILTER_CONTINUE;
}

static guint32
get_time_from_ns_event (NSEvent *event)
{
  double time = [event timestamp];

  /* cast via double->uint64 conversion to make sure that it is
   * wrapped on 32-bit machines when it overflows
   */
  return (guint32) (guint64) (time * 1000.0);
}

static int
get_mouse_button_from_ns_event (NSEvent *event)
{
  NSInteger button;

  button = [event buttonNumber];

  switch (button)
    {
    case 0:
      return 1;
    case 1:
      return 3;
    case 2:
      return 2;
    default:
      return button + 1;
    }
}

static BdkModifierType
get_mouse_button_modifiers_from_ns_buttons (NSUInteger nsbuttons)
{
  BdkModifierType modifiers = 0;

  if (nsbuttons & (1 << 0))
    modifiers |= BDK_BUTTON1_MASK;
  if (nsbuttons & (1 << 1))
    modifiers |= BDK_BUTTON3_MASK;
  if (nsbuttons & (1 << 2))
    modifiers |= BDK_BUTTON2_MASK;
  if (nsbuttons & (1 << 3))
    modifiers |= BDK_BUTTON4_MASK;
  if (nsbuttons & (1 << 4))
    modifiers |= BDK_BUTTON5_MASK;

  return modifiers;
}

static BdkModifierType
get_mouse_button_modifiers_from_ns_event (NSEvent *event)
{
  int button;
  BdkModifierType state = 0;

  /* This maps buttons 1 to 5 to BDK_BUTTON[1-5]_MASK */
  button = get_mouse_button_from_ns_event (event);
  if (button >= 1 && button <= 5)
    state = (1 << (button + 7));

  return state;
}

static BdkModifierType
get_keyboard_modifiers_from_ns_flags (NSUInteger nsflags)
{
  BdkModifierType modifiers = 0;

  if (nsflags & NSAlphaShiftKeyMask)
    modifiers |= BDK_LOCK_MASK;
  if (nsflags & NSShiftKeyMask)
    modifiers |= BDK_SHIFT_MASK;
  if (nsflags & NSControlKeyMask)
    modifiers |= BDK_CONTROL_MASK;
  if (nsflags & NSAlternateKeyMask)
    modifiers |= BDK_MOD1_MASK;
  if (nsflags & NSCommandKeyMask)
    modifiers |= BDK_MOD2_MASK;

  return modifiers;
}

static BdkModifierType
get_keyboard_modifiers_from_ns_event (NSEvent *nsevent)
{
  return get_keyboard_modifiers_from_ns_flags ([nsevent modifierFlags]);
}

/* Return an event mask from an NSEvent */
static BdkEventMask
get_event_mask_from_ns_event (NSEvent *nsevent)
{
  switch ([nsevent type])
    {
    case NSLeftMouseDown:
    case NSRightMouseDown:
    case NSOtherMouseDown:
      return BDK_BUTTON_PRESS_MASK;
    case NSLeftMouseUp:
    case NSRightMouseUp:
    case NSOtherMouseUp:
      return BDK_BUTTON_RELEASE_MASK;
    case NSMouseMoved:
      return BDK_POINTER_MOTION_MASK | BDK_POINTER_MOTION_HINT_MASK;
    case NSScrollWheel:
      /* Since applications that want button press events can get
       * scroll events on X11 (since scroll wheel events are really
       * button press events there), we need to use BDK_BUTTON_PRESS_MASK too.
       */
      return BDK_SCROLL_MASK | BDK_BUTTON_PRESS_MASK;
    case NSLeftMouseDragged:
      return (BDK_POINTER_MOTION_MASK | BDK_POINTER_MOTION_HINT_MASK |
	      BDK_BUTTON_MOTION_MASK | BDK_BUTTON1_MOTION_MASK | 
	      BDK_BUTTON1_MASK);
    case NSRightMouseDragged:
      return (BDK_POINTER_MOTION_MASK | BDK_POINTER_MOTION_HINT_MASK |
	      BDK_BUTTON_MOTION_MASK | BDK_BUTTON3_MOTION_MASK | 
	      BDK_BUTTON3_MASK);
    case NSOtherMouseDragged:
      {
	BdkEventMask mask;

	mask = (BDK_POINTER_MOTION_MASK |
		BDK_POINTER_MOTION_HINT_MASK |
		BDK_BUTTON_MOTION_MASK);

	if (get_mouse_button_from_ns_event (nsevent) == 2)
	  mask |= (BDK_BUTTON2_MOTION_MASK | BDK_BUTTON2_MOTION_MASK | 
		   BDK_BUTTON2_MASK);

	return mask;
      }
    case NSKeyDown:
    case NSKeyUp:
    case NSFlagsChanged:
      {
        switch (_bdk_quartz_keys_event_type (nsevent))
	  {
	  case BDK_KEY_PRESS:
	    return BDK_KEY_PRESS_MASK;
	  case BDK_KEY_RELEASE:
	    return BDK_KEY_RELEASE_MASK;
	  case BDK_NOTHING:
	    return 0;
	  default:
	    g_assert_not_reached ();
	  }
      }
      break;

    case NSMouseEntered:
      return BDK_ENTER_NOTIFY_MASK;

    case NSMouseExited:
      return BDK_LEAVE_NOTIFY_MASK;

    default:
      g_assert_not_reached ();
    }

  return 0;
}

static void
get_window_point_from_screen_point (BdkWindow *window,
                                    NSPoint    screen_point,
                                    gint      *x,
                                    gint      *y)
{
  NSPoint point;
  NSWindow *nswindow;
  BdkWindowObject *private;

  private = (BdkWindowObject *)window;
  nswindow = ((BdkWindowImplQuartz *)private->impl)->toplevel;

  point = [nswindow convertScreenToBase:screen_point];

  *x = point.x;
  *y = private->height - point.y;
}

static gboolean
is_mouse_button_press_event (NSEventType type)
{
  switch (type)
    {
      case NSLeftMouseDown:
      case NSRightMouseDown:
      case NSOtherMouseDown:
        return TRUE;
    }

  return FALSE;
}

static BdkWindow *
get_toplevel_from_ns_event (NSEvent *nsevent,
                            NSPoint *screen_point,
                            gint    *x,
                            gint    *y)
{
  BdkWindow *toplevel = NULL;

  if ([nsevent window])
    {
      BdkQuartzView *view;
      BdkWindowObject *private;
      NSPoint point, view_point;
      NSRect view_frame;

      view = (BdkQuartzView *)[[nsevent window] contentView];

      toplevel = [view bdkWindow];
      private = BDK_WINDOW_OBJECT (toplevel);

      point = [nsevent locationInWindow];
      view_point = [view convertPoint:point fromView:nil];
      view_frame = [view frame];

      /* NSEvents come in with a window set, but with window coordinates
       * out of window bounds. For e.g. moved events this is fine, we use
       * this information to properly handle enter/leave notify and motion
       * events. For mouse button press/release, we want to avoid forwarding
       * these events however, because the window they relate to is not the
       * window set in the event. This situation appears to occur when button
       * presses come in just before (or just after?) a window is resized and
       * also when a button press occurs on the OS X window titlebar.
       *
       * By setting toplevel to NULL, we do another attempt to get the right
       * toplevel window below.
       */
      if (is_mouse_button_press_event ([nsevent type]) &&
          (view_point.x < view_frame.origin.x ||
           view_point.x >= view_frame.origin.x + view_frame.size.width ||
           view_point.y < view_frame.origin.y ||
           view_point.y >= view_frame.origin.y + view_frame.size.height))
        {
          toplevel = NULL;

          /* This is a hack for button presses to break all grabs. E.g. if
           * a menu is open and one clicks on the title bar (or anywhere
           * out of window bounds), we really want to pop down the menu (by
           * breaking the grabs) before OS X handles the action of the title
           * bar button.
           *
           * Because we cannot ingest this event into BDK, we have to do it
           * here, not very nice.
           */
          _bdk_quartz_events_break_all_grabs (get_time_from_ns_event (nsevent));
        }
      else
        {
          *screen_point = [[nsevent window] convertBaseToScreen:point];

          *x = point.x;
          *y = private->height - point.y;
        }
    }

  if (!toplevel)
    {
      /* Fallback used when no NSWindow set.  This happens e.g. when
       * we allow motion events without a window set in bdk_event_translate()
       * that occur immediately after the main menu bar was clicked/used.
       * This fallback will not return coordinates contained in a window's
       * titlebar.
       */
      *screen_point = [NSEvent mouseLocation];
      toplevel = find_toplevel_under_pointer (_bdk_display,
                                              *screen_point,
                                              x, y);
    }

  return toplevel;
}

static BdkEvent *
create_focus_event (BdkWindow *window,
		    gboolean   in)
{
  BdkEvent *event;

  event = bdk_event_new (BDK_FOCUS_CHANGE);
  event->focus_change.window = window;
  event->focus_change.in = in;

  return event;
}


static void
generate_motion_event (BdkWindow *window)
{
  NSPoint screen_point;
  BdkEvent *event;
  gint x, y, x_root, y_root;

  event = bdk_event_new (BDK_MOTION_NOTIFY);
  event->any.window = NULL;
  event->any.send_event = TRUE;

  screen_point = [NSEvent mouseLocation];

  _bdk_quartz_window_nspoint_to_bdk_xy (screen_point, &x_root, &y_root);
  get_window_point_from_screen_point (window, screen_point, &x, &y);

  event->any.type = BDK_MOTION_NOTIFY;
  event->motion.window = window;
  event->motion.time = get_time_from_ns_event ([NSApp currentEvent]);
  event->motion.x = x;
  event->motion.y = y;
  event->motion.x_root = x_root;
  event->motion.y_root = y_root;
  /* FIXME event->axes */
  event->motion.state = _bdk_quartz_events_get_current_keyboard_modifiers () |
                        _bdk_quartz_events_get_current_mouse_modifiers ();
  event->motion.is_hint = FALSE;
  event->motion.device = _bdk_display->core_pointer;

  append_event (event, TRUE);
}

/* Note: Used to both set a new focus window and to unset the old one. */
void
_bdk_quartz_events_update_focus_window (BdkWindow *window,
					gboolean   got_focus)
{
  BdkEvent *event;

  if (got_focus && window == current_keyboard_window)
    return;

  /* FIXME: Don't do this when grabbed? Or make BdkQuartzWindow
   * disallow it in the first place instead?
   */
  
  if (!got_focus && window == current_keyboard_window)
    {
      event = create_focus_event (current_keyboard_window, FALSE);
      append_event (event, FALSE);
      g_object_unref (current_keyboard_window);
      current_keyboard_window = NULL;
    }

  if (got_focus)
    {
      if (current_keyboard_window)
	{
	  event = create_focus_event (current_keyboard_window, FALSE);
	  append_event (event, FALSE);
	  g_object_unref (current_keyboard_window);
	  current_keyboard_window = NULL;
	}
      
      event = create_focus_event (window, TRUE);
      append_event (event, FALSE);
      current_keyboard_window = g_object_ref (window);

      /* We just became the active window.  Unlike X11, Mac OS X does
       * not send us motion events while the window does not have focus
       * ("is not key").  We send a dummy motion notify event now, so that
       * everything in the window is set to correct state.
       */
      generate_motion_event (window);
    }
}

void
_bdk_quartz_events_send_map_event (BdkWindow *window)
{
  BdkWindowObject *private = (BdkWindowObject *)window;
  BdkWindowImplQuartz *impl = BDK_WINDOW_IMPL_QUARTZ (private->impl);

  if (!impl->toplevel)
    return;

  if (private->event_mask & BDK_STRUCTURE_MASK)
    {
      BdkEvent event;

      event.any.type = BDK_MAP;
      event.any.window = window;
  
      bdk_event_put (&event);
    }
}

static BdkWindow *
find_toplevel_under_pointer (BdkDisplay *display,
                             NSPoint     screen_point,
                             gint       *x,
                             gint       *y)
{
  BdkWindow *toplevel;

  toplevel = display->pointer_info.toplevel_under_pointer;
  if (toplevel && WINDOW_IS_TOPLEVEL (toplevel))
    get_window_point_from_screen_point (toplevel, screen_point, x, y);

  if (toplevel)
    {
      /* If the coordinates are out of window bounds, this toplevel is not
       * under the pointer and we thus return NULL. This can occur when
       * toplevel under pointer has not yet been updated due to a very recent
       * window resize. Alternatively, we should no longer be relying on
       * the toplevel_under_pointer value which is maintained in bdkwindow.c.
       */
      BdkWindowObject *private = BDK_WINDOW_OBJECT (toplevel);
      if (*x < 0 || *y < 0 || *x >= private->width || *y >= private->height)
        return NULL;
    }

  return toplevel;
}

/* This function finds the correct window to send an event to, taking
 * into account grabs, event propagation, and event masks.
 */
static BdkWindow *
find_window_for_ns_event (NSEvent *nsevent, 
                          gint    *x, 
                          gint    *y,
                          gint    *x_root,
                          gint    *y_root)
{
  BdkQuartzView *view;
  BdkWindow *toplevel;
  NSPoint screen_point;
  NSEventType event_type;

  view = (BdkQuartzView *)[[nsevent window] contentView];

  toplevel = get_toplevel_from_ns_event (nsevent, &screen_point, x, y);
  if (!toplevel)
    return NULL;
  _bdk_quartz_window_nspoint_to_bdk_xy (screen_point, x_root, y_root);

  event_type = [nsevent type];

  switch (event_type)
    {
    case NSLeftMouseDown:
    case NSRightMouseDown:
    case NSOtherMouseDown:
    case NSLeftMouseUp:
    case NSRightMouseUp:
    case NSOtherMouseUp:
    case NSMouseMoved:
    case NSScrollWheel:
    case NSLeftMouseDragged:
    case NSRightMouseDragged:
    case NSOtherMouseDragged:
      {
	BdkDisplay *display;
        BdkPointerGrabInfo *grab;

        display = bdk_drawable_get_display (toplevel);

	/* From the docs for XGrabPointer:
	 *
	 * If owner_events is True and if a generated pointer event
	 * would normally be reported to this client, it is reported
	 * as usual. Otherwise, the event is reported with respect to
	 * the grab_window and is reported only if selected by
	 * event_mask. For either value of owner_events, unreported
	 * events are discarded.
	 */
        grab = _bdk_display_get_last_pointer_grab (display);
	if (WINDOW_IS_TOPLEVEL (toplevel) && grab)
	  {
            /* Implicit grabs do not go through XGrabPointer and thus the
             * event mask should not be checked.
             */
	    if (!grab->implicit
                && (grab->event_mask & get_event_mask_from_ns_event (nsevent)) == 0)
              return NULL;

            if (grab->owner_events)
              {
                /* For owner events, we need to use the toplevel under the
                 * pointer, not the window from the NSEvent, since that is
                 * reported with respect to the key window, which could be
                 * wrong.
                 */
                BdkWindow *toplevel_under_pointer;
                gint x_tmp, y_tmp;

                toplevel_under_pointer = find_toplevel_under_pointer (display,
                                                                      screen_point,
                                                                      &x_tmp, &y_tmp);
                if (toplevel_under_pointer)
                  {
                    toplevel = toplevel_under_pointer;
                    *x = x_tmp;
                    *y = y_tmp;
                  }

                return toplevel;
              }
            else
              {
                /* Finally check the grab window. */
		BdkWindow *grab_toplevel;

		grab_toplevel = bdk_window_get_effective_toplevel (grab->window);
                get_window_point_from_screen_point (grab_toplevel,
                                                    screen_point, x, y);

		return grab_toplevel;
	      }

	    return NULL;
	  }
	else 
	  {
	    /* The non-grabbed case. */
            BdkWindow *toplevel_under_pointer;
            gint x_tmp, y_tmp;

            /* Ignore all events but mouse moved that might be on the title
             * bar (above the content view). The reason is that otherwise
             * bdk gets confused about getting e.g. button presses with no
             * window (the title bar is not known to it).
             */
            if (event_type != NSMouseMoved)
              if (*y < 0)
                return NULL;

            /* As for owner events, we need to use the toplevel under the
             * pointer, not the window from the NSEvent.
             */
            toplevel_under_pointer = find_toplevel_under_pointer (display,
                                                                  screen_point,
                                                                  &x_tmp, &y_tmp);
            if (toplevel_under_pointer
                && WINDOW_IS_TOPLEVEL (toplevel_under_pointer))
              {
                BdkWindowObject *toplevel_private;
                BdkWindowImplQuartz *toplevel_impl;

                toplevel = toplevel_under_pointer;

                toplevel_private = (BdkWindowObject *)toplevel;
                toplevel_impl = (BdkWindowImplQuartz *)toplevel_private->impl;

                *x = x_tmp;
                *y = y_tmp;
              }

            return toplevel;
	  }
      }
      break;
      
    case NSMouseEntered:
    case NSMouseExited:
      /* Only handle our own entered/exited events, not the ones for the
       * titlebar buttons.
       */
      if ([view trackingRect] == [nsevent trackingNumber])
        return toplevel;
      else
        return NULL;

    case NSKeyDown:
    case NSKeyUp:
    case NSFlagsChanged:
      if (_bdk_display->keyboard_grab.window && !_bdk_display->keyboard_grab.owner_events)
        return bdk_window_get_effective_toplevel (_bdk_display->keyboard_grab.window);

      return toplevel;

    default:
      /* Ignore everything else. */
      break;
    }

  return NULL;
}

static void
fill_crossing_event (BdkWindow       *toplevel,
                     BdkEvent        *event,
                     NSEvent         *nsevent,
                     gint             x,
                     gint             y,
                     gint             x_root,
                     gint             y_root,
                     BdkEventType     event_type,
                     BdkCrossingMode  mode,
                     BdkNotifyType    detail)
{
  event->any.type = event_type;
  event->crossing.window = toplevel;
  event->crossing.subwindow = NULL;
  event->crossing.time = get_time_from_ns_event (nsevent);
  event->crossing.x = x;
  event->crossing.y = y;
  event->crossing.x_root = x_root;
  event->crossing.y_root = y_root;
  event->crossing.mode = mode;
  event->crossing.detail = detail;
  event->crossing.state = get_keyboard_modifiers_from_ns_event (nsevent) |
                         _bdk_quartz_events_get_current_mouse_modifiers ();

  /* FIXME: Focus and button state? */
}

static void
fill_button_event (BdkWindow *window,
                   BdkEvent  *event,
                   NSEvent   *nsevent,
                   gint       x,
                   gint       y,
                   gint       x_root,
                   gint       y_root)
{
  BdkEventType type;
  gint state;

  state = get_keyboard_modifiers_from_ns_event (nsevent) |
         _bdk_quartz_events_get_current_mouse_modifiers ();

  switch ([nsevent type])
    {
    case NSLeftMouseDown:
    case NSRightMouseDown:
    case NSOtherMouseDown:
      type = BDK_BUTTON_PRESS;
      state &= ~get_mouse_button_modifiers_from_ns_event (nsevent);
      break;

    case NSLeftMouseUp:
    case NSRightMouseUp:
    case NSOtherMouseUp:
      type = BDK_BUTTON_RELEASE;
      state |= get_mouse_button_modifiers_from_ns_event (nsevent);
      break;

    default:
      g_assert_not_reached ();
    }

  event->any.type = type;
  event->button.window = window;
  event->button.time = get_time_from_ns_event (nsevent);
  event->button.x = x;
  event->button.y = y;
  event->button.x_root = x_root;
  event->button.y_root = y_root;
  /* FIXME event->axes */
  event->button.state = state;
  event->button.button = get_mouse_button_from_ns_event (nsevent);
  event->button.device = _bdk_display->core_pointer;
}

static void
fill_motion_event (BdkWindow *window,
                   BdkEvent  *event,
                   NSEvent   *nsevent,
                   gint       x,
                   gint       y,
                   gint       x_root,
                   gint       y_root)
{
  event->any.type = BDK_MOTION_NOTIFY;
  event->motion.window = window;
  event->motion.time = get_time_from_ns_event (nsevent);
  event->motion.x = x;
  event->motion.y = y;
  event->motion.x_root = x_root;
  event->motion.y_root = y_root;
  /* FIXME event->axes */
  event->motion.state = get_keyboard_modifiers_from_ns_event (nsevent) |
                        _bdk_quartz_events_get_current_mouse_modifiers ();
  event->motion.is_hint = FALSE;
  event->motion.device = _bdk_display->core_pointer;
}

static void
fill_scroll_event (BdkWindow          *window,
                   BdkEvent           *event,
                   NSEvent            *nsevent,
                   gint                x,
                   gint                y,
                   gint                x_root,
                   gint                y_root,
                   BdkScrollDirection  direction)
{
  BdkWindowObject *private;
  NSPoint point;

  private = BDK_WINDOW_OBJECT (window);

  point = [nsevent locationInWindow];

  event->any.type = BDK_SCROLL;
  event->scroll.window = window;
  event->scroll.time = get_time_from_ns_event (nsevent);
  event->scroll.x = x;
  event->scroll.y = y;
  event->scroll.x_root = x_root;
  event->scroll.y_root = y_root;
  event->scroll.state = get_keyboard_modifiers_from_ns_event (nsevent);
  event->scroll.direction = direction;
  event->scroll.device = _bdk_display->core_pointer;
}

static void
fill_key_event (BdkWindow    *window,
                BdkEvent     *event,
                NSEvent      *nsevent,
                BdkEventType  type)
{
  BdkEventPrivate *priv;
  gchar buf[7];
  gunichar c = 0;

  priv = (BdkEventPrivate *) event;
  priv->windowing_data = [nsevent retain];

  event->any.type = type;
  event->key.window = window;
  event->key.time = get_time_from_ns_event (nsevent);
  event->key.state = get_keyboard_modifiers_from_ns_event (nsevent);
  event->key.hardware_keycode = [nsevent keyCode];
  event->key.group = ([nsevent modifierFlags] & NSAlternateKeyMask) ? 1 : 0;
  event->key.keyval = BDK_VoidSymbol;
  
  bdk_keymap_translate_keyboard_state (NULL,
				       event->key.hardware_keycode,
				       event->key.state, 
				       event->key.group,
				       &event->key.keyval,
				       NULL, NULL, NULL);

  event->key.is_modifier = _bdk_quartz_keys_is_modifier (event->key.hardware_keycode);

  /* If the key press is a modifier, the state should include the mask
   * for that modifier but only for releases, not presses. This
   * matches the X11 backend behavior.
   */
  if (event->key.is_modifier)
    {
      int mask = 0;

      switch (event->key.keyval)
        {
        case BDK_Meta_R:
        case BDK_Meta_L:
          mask = BDK_MOD2_MASK;
          break;
        case BDK_Shift_R:
        case BDK_Shift_L:
          mask = BDK_SHIFT_MASK;
          break;
        case BDK_Caps_Lock:
          mask = BDK_LOCK_MASK;
          break;
        case BDK_Alt_R:
        case BDK_Alt_L:
          mask = BDK_MOD1_MASK;
          break;
        case BDK_Control_R:
        case BDK_Control_L:
          mask = BDK_CONTROL_MASK;
          break;
        default:
          mask = 0;
        }

      if (type == BDK_KEY_PRESS)
        event->key.state &= ~mask;
      else if (type == BDK_KEY_RELEASE)
        event->key.state |= mask;
    }

  event->key.state |= _bdk_quartz_events_get_current_mouse_modifiers ();

  /* The X11 backend adds the first virtual modifier MOD2..MOD5 are
   * mapped to. Since we only have one virtual modifier in the quartz
   * backend, calling the standard function will do.
   */
  bdk_keymap_add_virtual_modifiers (NULL, &event->key.state);

  event->key.string = NULL;

  /* Fill in ->string since apps depend on it, taken from the x11 backend. */
  if (event->key.keyval != BDK_VoidSymbol)
    c = bdk_keyval_to_unicode (event->key.keyval);

  if (c)
    {
      gsize bytes_written;
      gint len;

      len = g_unichar_to_utf8 (c, buf);
      buf[len] = '\0';
      
      event->key.string = g_locale_from_utf8 (buf, len,
					      NULL, &bytes_written,
					      NULL);
      if (event->key.string)
	event->key.length = bytes_written;
    }
  else if (event->key.keyval == BDK_Escape)
    {
      event->key.length = 1;
      event->key.string = g_strdup ("\033");
    }
  else if (event->key.keyval == BDK_Return ||
	  event->key.keyval == BDK_KP_Enter)
    {
      event->key.length = 1;
      event->key.string = g_strdup ("\r");
    }

  if (!event->key.string)
    {
      event->key.length = 0;
      event->key.string = g_strdup ("");
    }

  BDK_NOTE(EVENTS,
    g_message ("key %s:\t\twindow: %p  key: %12s  %d",
	  type == BDK_KEY_PRESS ? "press" : "release",
	  event->key.window,
	  event->key.keyval ? bdk_keyval_name (event->key.keyval) : "(none)",
	  event->key.keyval));
}

static gboolean
synthesize_crossing_event (BdkWindow *window,
                           BdkEvent  *event,
                           NSEvent   *nsevent,
                           gint       x,
                           gint       y,
                           gint       x_root,
                           gint       y_root)
{
  BdkWindowObject *private;

  private = BDK_WINDOW_OBJECT (window);

  switch ([nsevent type])
    {
    case NSMouseEntered:
      /* Enter events are considered always to be from another toplevel
       * window, this shouldn't negatively affect any app or btk code,
       * and is the only way to make BtkMenu work. EEK EEK EEK.
       */
      if (!(private->event_mask & BDK_ENTER_NOTIFY_MASK))
        return FALSE;

      fill_crossing_event (window, event, nsevent,
                           x, y,
                           x_root, y_root,
                           BDK_ENTER_NOTIFY,
                           BDK_CROSSING_NORMAL,
                           BDK_NOTIFY_NONLINEAR);
      return TRUE;

    case NSMouseExited:
      /* See above */
      if (!(private->event_mask & BDK_LEAVE_NOTIFY_MASK))
        return FALSE;

      fill_crossing_event (window, event, nsevent,
                           x, y,
                           x_root, y_root,
                           BDK_LEAVE_NOTIFY,
                           BDK_CROSSING_NORMAL,
                           BDK_NOTIFY_NONLINEAR);
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

void
_bdk_quartz_synthesize_null_key_event (BdkWindow *window)
{
  BdkEvent *event;

  event = bdk_event_new (BDK_KEY_PRESS);
  event->any.type = BDK_KEY_PRESS;
  event->key.window = window;
  event->key.state = 0;
  event->key.hardware_keycode = 0;
  event->key.group = 0;
  event->key.keyval = BDK_VoidSymbol;
  append_event(event, FALSE);
}

BdkEventMask 
_bdk_quartz_events_get_current_event_mask (void)
{
  return current_event_mask;
}

BdkModifierType
_bdk_quartz_events_get_current_keyboard_modifiers (void)
{
  if (bdk_quartz_osx_version () >= BDK_OSX_SNOW_LEOPARD)
    {
      return get_keyboard_modifiers_from_ns_flags ([NSClassFromString(@"NSEvent") modifierFlags]);
    }
  else
    {
      guint carbon_modifiers = GetCurrentKeyModifiers ();
      BdkModifierType modifiers = 0;

      if (carbon_modifiers & alphaLock)
        modifiers |= BDK_LOCK_MASK;
      if (carbon_modifiers & shiftKey)
        modifiers |= BDK_SHIFT_MASK;
      if (carbon_modifiers & controlKey)
        modifiers |= BDK_CONTROL_MASK;
      if (carbon_modifiers & optionKey)
        modifiers |= BDK_MOD1_MASK;
      if (carbon_modifiers & cmdKey)
        modifiers |= BDK_MOD2_MASK;

      return modifiers;
    }
}

BdkModifierType
_bdk_quartz_events_get_current_mouse_modifiers (void)
{
  if (bdk_quartz_osx_version () >= BDK_OSX_SNOW_LEOPARD)
    {
      return get_mouse_button_modifiers_from_ns_buttons ([NSClassFromString(@"NSEvent") pressedMouseButtons]);
    }
  else
    {
      return get_mouse_button_modifiers_from_ns_buttons (GetCurrentButtonState ());
    }
}

/* Detect window resizing */

static gboolean
test_resize (NSEvent *event, BdkWindow *toplevel, gint x, gint y)
{
  BdkWindowObject *toplevel_private;
  BdkWindowImplQuartz *toplevel_impl;
  gboolean lion;

  /* Resizing from the resize indicator only begins if an NSLeftMouseButton
   * event is received in the resizing area.
   */
  toplevel_private = (BdkWindowObject *)toplevel;
  toplevel_impl = (BdkWindowImplQuartz *)toplevel_private->impl;
  if ([event type] == NSLeftMouseDown &&
      [toplevel_impl->toplevel showsResizeIndicator])
    {
      NSRect frame;

      /* If the resize indicator is visible and the event
       * is in the lower right 15x15 corner, we leave these
       * events to Cocoa as to be handled as resize events.
       * Applications may have widgets in this area.  These
       * will most likely be larger than 15x15 and for
       * scroll bars there are also other means to move
       * the scroll bar.  Since the resize indicator is
       * the only way of resizing windows on Mac OS, it
       * is too important to not make functional.
       */
      frame = [toplevel_impl->view bounds];
      if (x > frame.size.width - GRIP_WIDTH &&
          x < frame.size.width &&
          y > frame.size.height - GRIP_HEIGHT &&
          y < frame.size.height)
        return TRUE;
     }

  /* If we're on Lion and within 5 pixels of an edge,
   * then assume that the user wants to resize, and
   * return NULL to let Quartz get on with it. We check
   * the selector isRestorable to see if we're on 10.7.
   * This extra check is in case the user starts
   * dragging before BDK recognizes the grab.
   *
   * We perform this check for a button press of all buttons, because we
   * do receive, for instance, a right mouse down event for a BDK window
   * for x-coordinate range [-3, 0], but we do not want to forward this
   * into BDK. Forwarding such events into BDK will confuse the pointer
   * window finding code, because there are no BdkWindows present in
   * the range [-3, 0].
   */
  lion = bdk_quartz_osx_version () >= BDK_OSX_LION;
  if (lion &&
      ([event type] == NSLeftMouseDown ||
       [event type] == NSRightMouseDown ||
       [event type] == NSOtherMouseDown))
    {
      if (x < BDK_LION_RESIZE ||
          x > toplevel_private->width - BDK_LION_RESIZE ||
          y > toplevel_private->height - BDK_LION_RESIZE)
        return TRUE;
    }

  return FALSE;
}

static gboolean
bdk_event_translate (BdkEvent *event,
                     NSEvent  *nsevent)
{
  NSEventType event_type;
  NSWindow *nswindow;
  BdkWindow *window;
  int x, y;
  int x_root, y_root;
  gboolean return_val;
  BdkEvent *input_event;

  /* There is no support for real desktop wide grabs, so we break
   * grabs when the application loses focus (gets deactivated).
   */
  event_type = [nsevent type];
  if (event_type == NSAppKitDefined)
    {
      if ([nsevent subtype] == NSApplicationDeactivatedEventType)
        _bdk_quartz_events_break_all_grabs (get_time_from_ns_event (nsevent));

      /* This could potentially be used to break grabs when clicking
       * on the title. The subtype 20 is undocumented so it's probably
       * not a good idea: else if (subtype == 20) break_all_grabs ();
       */

      /* Leave all AppKit events to AppKit. */
      return FALSE;
    }

  if (_bdk_default_filters)
    {
      /* Apply global filters */
      BdkFilterReturn result;

      result = bdk_event_apply_filters (nsevent, event, &_bdk_default_filters);
      if (result != BDK_FILTER_CONTINUE)
        {
          return_val = (result == BDK_FILTER_TRANSLATE) ? TRUE : FALSE;
          goto done;
        }
    }

  nswindow = [nsevent window];

  /* Ignore events for windows not created by BDK. */
  if (nswindow && ![[nswindow contentView] isKindOfClass:[BdkQuartzView class]])
    return FALSE;

  /* Ignore events for ones with no windows */
  if (!nswindow)
    {
      BdkWindow *toplevel = NULL;

      if (event_type == NSMouseMoved)
        {
          /* Motion events received after clicking the menu bar do not have the
           * window field set.  Instead of giving up on the event immediately,
           * we first check whether this event is within our window bounds.
           */
          NSPoint screen_point = [NSEvent mouseLocation];
          gint x_tmp, y_tmp;

          toplevel = find_toplevel_under_pointer (_bdk_display,
                                                  screen_point,
                                                  &x_tmp, &y_tmp);
        }

      if (!toplevel)
        return FALSE;
    }

  /* Ignore events and break grabs while the window is being
   * dragged. This is a workaround for the window getting events for
   * the window title.
   */
  if ([(BdkQuartzWindow *)nswindow isInMove])
    {
      _bdk_quartz_events_break_all_grabs (get_time_from_ns_event (nsevent));
      return FALSE;
    }

  /* Also when in a manual resize, we ignore events so that these are
   * pushed to BdkQuartzWindow's sendEvent handler.
   */
  if ([(BdkQuartzWindow *)nswindow isInManualResize])
    return FALSE;

  /* Find the right BDK window to send the event to, taking grabs and
   * event masks into consideration.
   */
  window = find_window_for_ns_event (nsevent, &x, &y, &x_root, &y_root);
  if (!window)
    return FALSE;

  /* Quartz handles resizing on its own, so we want to stay out of the way. */
  if (test_resize (nsevent, window, x, y))
    return FALSE;

  /* Apply any window filters. */
  if (BDK_IS_WINDOW (window))
    {
      BdkWindowObject *filter_private = (BdkWindowObject *) window;
      BdkFilterReturn result;

      if (filter_private->filters)
        {
          g_object_ref (window);

          result = bdk_event_apply_filters (nsevent, event, &filter_private->filters);

          g_object_unref (window);

          if (result != BDK_FILTER_CONTINUE)
            {
              return_val = (result == BDK_FILTER_TRANSLATE) ? TRUE : FALSE;
              goto done;
            }
        }
    }

  /* If the app is not active leave the event to AppKit so the window gets
   * focused correctly and don't do click-through (so we behave like most
   * native apps). If the app is active, we focus the window and then handle
   * the event, also to match native apps.
   */
  if ((event_type == NSRightMouseDown ||
       event_type == NSOtherMouseDown ||
       event_type == NSLeftMouseDown))
    {
      BdkWindowObject *private = (BdkWindowObject *)window;
      BdkWindowImplQuartz *impl = BDK_WINDOW_IMPL_QUARTZ (private->impl);

      if (![NSApp isActive])
        {
          [NSApp activateIgnoringOtherApps:YES];
          return FALSE;
        }
      else if (![impl->toplevel isKeyWindow])
        {
          BdkPointerGrabInfo *grab;

          grab = _bdk_display_get_last_pointer_grab (_bdk_display);
          if (!grab)
            [impl->toplevel makeKeyWindow];
        }
    }

  current_event_mask = get_event_mask_from_ns_event (nsevent);

  return_val = TRUE;

  switch (event_type)
    {
    case NSLeftMouseDown:
    case NSRightMouseDown:
    case NSOtherMouseDown:
    case NSLeftMouseUp:
    case NSRightMouseUp:
    case NSOtherMouseUp:
      fill_button_event (window, event, nsevent, x, y, x_root, y_root);

      input_event = bdk_event_new (BDK_NOTHING);
      if (_bdk_input_fill_quartz_input_event (event, nsevent, input_event))
        append_event (input_event, TRUE);
      else
        bdk_event_free (input_event);
      break;

    case NSLeftMouseDragged:
    case NSRightMouseDragged:
    case NSOtherMouseDragged:
    case NSMouseMoved:
      fill_motion_event (window, event, nsevent, x, y, x_root, y_root);

      input_event = bdk_event_new (BDK_NOTHING);
      if (_bdk_input_fill_quartz_input_event (event, nsevent, input_event))
        append_event (input_event, TRUE);
      else
        bdk_event_free (input_event);
      break;

    case NSScrollWheel:
      {
	float dx = [nsevent deltaX];
	float dy = [nsevent deltaY];
	BdkScrollDirection direction;

        if (dy != 0)
          {
            if (dy < 0.0)
              direction = BDK_SCROLL_DOWN;
            else
              direction = BDK_SCROLL_UP;

            fill_scroll_event (window, event, nsevent, x, y, x_root, y_root, direction);
          }

        if (dx != 0)
          {
            if (dx < 0.0)
              direction = BDK_SCROLL_RIGHT;
            else
              direction = BDK_SCROLL_LEFT;

            fill_scroll_event (window, event, nsevent, x, y, x_root, y_root, direction);
          }
      }
      break;

    case NSMouseExited:
      if (WINDOW_IS_TOPLEVEL (window))
          [[NSCursor arrowCursor] set];
      /* fall through */
    case NSMouseEntered:
      return_val = synthesize_crossing_event (window, event, nsevent, x, y, x_root, y_root);
      break;

    case NSKeyDown:
    case NSKeyUp:
    case NSFlagsChanged:
      {
        BdkEventType type;

        type = _bdk_quartz_keys_event_type (nsevent);
        if (type == BDK_NOTHING)
          return_val = FALSE;
        else
          fill_key_event (window, event, nsevent, type);
      }
      break;

    case NSTabletProximity:
      _bdk_input_quartz_tablet_proximity ([nsevent pointingDeviceType]);
      return_val = FALSE;
      break;

    default:
      /* Ignore everything elsee. */
      return_val = FALSE;
      break;
    }

 done:
  if (return_val)
    {
      if (event->any.window)
	g_object_ref (event->any.window);
      if (((event->any.type == BDK_ENTER_NOTIFY) ||
	   (event->any.type == BDK_LEAVE_NOTIFY)) &&
	  (event->crossing.subwindow != NULL))
	g_object_ref (event->crossing.subwindow);
    }
  else
    {
      /* Mark this event as having no resources to be freed */
      event->any.window = NULL;
      event->any.type = BDK_NOTHING;
    }

  return return_val;
}

void
_bdk_events_queue (BdkDisplay *display)
{  
  NSEvent *nsevent;

  nsevent = _bdk_quartz_event_loop_get_pending ();
  if (nsevent)
    {
      BdkEvent *event;
      GList *node;

      event = bdk_event_new (BDK_NOTHING);

      event->any.window = NULL;
      event->any.send_event = FALSE;

      ((BdkEventPrivate *)event)->flags |= BDK_EVENT_PENDING;

      node = _bdk_event_queue_append (display, event);

      if (bdk_event_translate (event, nsevent))
        {
	  ((BdkEventPrivate *)event)->flags &= ~BDK_EVENT_PENDING;
          _bdk_windowing_got_event (display, node, event, 0);
        }
      else
        {
	  _bdk_event_queue_remove_link (display, node);
	  g_list_free_1 (node);
	  bdk_event_free (event);

          BDK_THREADS_LEAVE ();
          [NSApp sendEvent:nsevent];
          BDK_THREADS_ENTER ();
        }

      _bdk_quartz_event_loop_release_event (nsevent);
    }
}

void
bdk_flush (void)
{
  /* Not supported. */
}

void
bdk_display_add_client_message_filter (BdkDisplay   *display,
				       BdkAtom       message_type,
				       BdkFilterFunc func,
				       gpointer      data)
{
  /* Not supported. */
}

void
bdk_add_client_message_filter (BdkAtom       message_type,
			       BdkFilterFunc func,
			       gpointer      data)
{
  /* Not supported. */
}

void
bdk_display_sync (BdkDisplay *display)
{
  /* Not supported. */
}

void
bdk_display_flush (BdkDisplay *display)
{
  /* Not supported. */
}

gboolean
bdk_event_send_client_message_for_display (BdkDisplay      *display,
					   BdkEvent        *event,
					   BdkNativeWindow  winid)
{
  /* Not supported. */
  return FALSE;
}

void
bdk_screen_broadcast_client_message (BdkScreen *screen,
				     BdkEvent  *event)
{
  /* Not supported. */
}

gboolean
bdk_screen_get_setting (BdkScreen   *screen,
			const gchar *name,
			GValue      *value)
{
  if (strcmp (name, "btk-double-click-time") == 0)
    {
      NSUserDefaults *defaults;
      float t;

      BDK_QUARTZ_ALLOC_POOL;

      defaults = [NSUserDefaults standardUserDefaults];
            
      t = [defaults floatForKey:@"com.apple.mouse.doubleClickThreshold"];
      if (t == 0.0)
	{
	  /* No user setting, use the default in OS X. */
	  t = 0.5;
	}

      BDK_QUARTZ_RELEASE_POOL;

      g_value_set_int (value, t * 1000);

      return TRUE;
    }
  else if (strcmp (name, "btk-font-name") == 0)
    {
      NSString *name;
      char *str;

      BDK_QUARTZ_ALLOC_POOL;

      name = [[NSFont systemFontOfSize:0] familyName];

      /* Let's try to use the "views" font size (12pt) by default. This is
       * used for lists/text/other "content" which is the largest parts of
       * apps, using the "regular control" size (13pt) looks a bit out of
       * place. We might have to tweak this.
       */

      /* The size has to be hardcoded as there doesn't seem to be a way to
       * get the views font size programmatically.
       */
      str = g_strdup_printf ("%s 12", [name UTF8String]);
      g_value_set_string (value, str);
      g_free (str);

      BDK_QUARTZ_RELEASE_POOL;

      return TRUE;
    }
  else if (strcmp (name, "btk-primary-button-warps-slider") == 0)
    {
      BDK_QUARTZ_ALLOC_POOL;

      BOOL setting = [[NSUserDefaults standardUserDefaults] boolForKey:@"AppleScrollerPagingBehavior"];

      /* If the Apple property is YES, it means "warp" */
      g_value_set_boolean (value, setting == YES);

      BDK_QUARTZ_RELEASE_POOL;

      return TRUE;
    }
  
  /* FIXME: Add more settings */

  return FALSE;
}

void
_bdk_windowing_event_data_copy (const BdkEvent *src,
                                BdkEvent       *dst)
{
  BdkEventPrivate *priv_src = (BdkEventPrivate *) src;
  BdkEventPrivate *priv_dst = (BdkEventPrivate *) dst;

  if (priv_src->windowing_data)
    {
      priv_dst->windowing_data = priv_src->windowing_data;
      [(NSEvent *)priv_dst->windowing_data retain];
    }
}

void
_bdk_windowing_event_data_free (BdkEvent *event)
{
  BdkEventPrivate *priv = (BdkEventPrivate *) event;

  if (priv->windowing_data)
    {
      [(NSEvent *)priv->windowing_data release];
      priv->windowing_data = NULL;
    }
}
