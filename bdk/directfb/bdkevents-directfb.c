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
 * file for a list of people on the BTK+ Team.
 */

/*
 * BTK+ DirectFB backend
 * Copyright (C) 2001-2002  convergence integrated media GmbH
 * Copyright (C) 2002-2004  convergence GmbH
 * Written by Denis Oliver Kropp <dok@convergence.de> and
 *            Sven Neumann <sven@convergence.de>
 */

#include "config.h"
#include "bdk.h"
#include "bdkdirectfb.h"
#include "bdkprivate-directfb.h"

#include "bdkinternals.h"

#include "bdkkeysyms.h"

#include "bdkinput-directfb.h"
#include <string.h>

#ifndef __BDK_X_H__
#define __BDK_X_H__
gboolean bdk_net_wm_supports (BdkAtom property);
#endif

#include "bdkalias.h"

#define EventBuffer _bdk_display->buffer
#define DirectFB _bdk_display->directfb

#include "bdkaliasdef.c"

D_DEBUG_DOMAIN (BDKDFB_Events, "BDKDFB/Events", "BDK DirectFB Events");
D_DEBUG_DOMAIN (BDKDFB_MouseEvents, "BDKDFB/Events/Mouse", "BDK DirectFB Mouse Events");
D_DEBUG_DOMAIN (BDKDFB_WindowEvents, "BDKDFB/Events/Window", "BDK DirectFB Window Events");
D_DEBUG_DOMAIN (BDKDFB_KeyEvents, "BDKDFB/Events/Key", "BDK DirectFB Key Events");

/*********************************************
 * Functions for maintaining the event queue *
 *********************************************/

static gboolean bdk_event_translate  (BdkEvent       *event,
                                      DFBWindowEvent *dfbevent,
                                      BdkWindow      *window);

/*
 * Private variable declarations
 */
static GList *client_filters;  /* Filters for client messages */

static gint
bdk_event_apply_filters (DFBWindowEvent *dfbevent,
                         BdkEvent *event,
                         GList *filters)
{
  GList *tmp_list;
  BdkFilterReturn result;

  tmp_list = filters;

  while (tmp_list)
    {
      BdkEventFilter *filter = (BdkEventFilter*) tmp_list->data;

      tmp_list = tmp_list->next;
      result = filter->function (dfbevent, event, filter->data);
      if (result !=  BDK_FILTER_CONTINUE)
        return result;
    }

  return BDK_FILTER_CONTINUE;
}

static void
dfb_events_process_window_event (DFBWindowEvent *dfbevent)
{
  BdkDisplay *display = bdk_display_get_default ();
  BdkWindow  *window;
  BdkEvent   *event;
  GList      *node;

  window = bdk_directfb_window_id_table_lookup (dfbevent->window_id);
  if (!window)
    return;

  event = bdk_event_new (BDK_NOTHING);

  event->any.window = NULL;

  ((BdkEventPrivate *)event)->flags |= BDK_EVENT_PENDING;

  node = _bdk_event_queue_append (display, event);

  if (bdk_event_translate (event, dfbevent, window))
    {
      ((BdkEventPrivate *)event)->flags &= ~BDK_EVENT_PENDING;
      _bdk_windowing_got_event (display, node, event, 0);
    }
  else
    {
      _bdk_event_queue_remove_link (display, node);
      g_list_free_1 (node);
      bdk_event_free (event);
    }
}

static gboolean
bdk_event_send_client_message_by_window (BdkEvent *event,
                                         BdkWindow *window)
{
  DFBUserEvent evt;

  g_return_val_if_fail(event != NULL, FALSE);
  g_return_val_if_fail(BDK_IS_WINDOW(window), FALSE);

  evt.clazz = DFEC_USER;
  evt.type = GPOINTER_TO_UINT (BDK_ATOM_TO_POINTER (event->client.message_type));
  evt.data = (void *) event->client.data.l[0];

  _bdk_display->buffer->PostEvent (_bdk_display->buffer, DFB_EVENT (&evt));

  return TRUE;
}

static void
dfb_events_dispatch (void)
{
  BdkDisplay *display = bdk_display_get_default ();
  BdkEvent   *event;

  BDK_THREADS_ENTER ();

  while ((event = _bdk_event_unqueue (display)) != NULL)
    {
      if (_bdk_event_func)
        (*_bdk_event_func) (event, _bdk_event_data);

      bdk_event_free (event);
    }

  BDK_THREADS_LEAVE ();
}

static gboolean
dfb_events_io_func (BUNNYIOChannel   *channel,
                    BUNNYIOCondition  condition,
                    gpointer      data)
{
  gsize      i;
  gsize      read;
  BUNNYIOStatus  result;
  DFBEvent   buf[23];
  DFBEvent  *event;

  result = g_io_channel_read_chars (channel,
                                    (gchar *) buf, sizeof (buf), &read, NULL);

  if (result == G_IO_STATUS_ERROR)
    {
      g_warning ("%s: BUNNYIOError occured", B_STRFUNC);
      return TRUE;
    }

  read /= sizeof (DFBEvent);

  for (i = 0, event = buf; i < read; i++, event++)
    {
      switch (event->clazz)
        {
        case DFEC_WINDOW:
          /* TODO workaround to prevent two DWET_ENTER in a row from being delivered */
          if (event->window.type == DWET_ENTER) {
            if (i > 0 && buf[i - 1].window.type != DWET_ENTER)
              dfb_events_process_window_event (&event->window);
          }
          else
            dfb_events_process_window_event (&event->window);
          break;

        case DFEC_USER:
          {
            GList *list;

            BDK_NOTE (EVENTS, g_print (" client_message"));

            for (list = client_filters; list; list = list->next)
              {
                BdkClientFilter *filter     = list->data;
                DFBUserEvent    *user_event = (DFBUserEvent *) event;
                BdkAtom          type;

                type = BDK_POINTER_TO_ATOM (GUINT_TO_POINTER (user_event->type));

                if (filter->type == type)
                  {
                    if (filter->function (user_event,
                                          NULL,
                                          filter->data) != BDK_FILTER_CONTINUE)
                      break;
                  }
              }
          }
          break;

        default:
          break;
        }
    }

  EventBuffer->Reset (EventBuffer);

  dfb_events_dispatch ();

  return TRUE;
}

void
_bdk_events_init (void)
{
  BUNNYIOChannel *channel;
  GSource    *source;
  DFBResult   ret;
  gint        fd;

  ret = DirectFB->CreateEventBuffer (DirectFB, &EventBuffer);
  if (ret)
    {
      DirectFBError ("_bdk_events_init: "
                     "IDirectFB::CreateEventBuffer() failed", ret);
      return;
    }

  ret = EventBuffer->CreateFileDescriptor (EventBuffer, &fd);
  if (ret)
    {
      DirectFBError ("_bdk_events_init: "
                     "IDirectFBEventBuffer::CreateFileDescriptor() failed",
                     ret);
      return;
    }

  channel = g_io_channel_unix_new (fd);

  g_io_channel_set_encoding (channel, NULL, NULL);
  g_io_channel_set_buffered (channel, FALSE);

  source = g_io_create_watch (channel, G_IO_IN);

  g_source_set_priority (source, G_PRIORITY_DEFAULT);
  g_source_set_can_recurse (source, TRUE);
  g_source_set_callback (source, (GSourceFunc) dfb_events_io_func, NULL, NULL);

  g_source_attach (source, NULL);
  g_source_unref (source);
}

gboolean
bdk_events_pending (void)
{
  BdkDisplay *display = bdk_display_get_default ();

  return _bdk_event_queue_find_first (display) ? TRUE : FALSE;
}

BdkEvent *
bdk_event_get_graphics_expose (BdkWindow *window)
{
  BdkDisplay *display;
  GList      *list;

  g_return_val_if_fail (BDK_IS_WINDOW (window), NULL);

  display = bdk_drawable_get_display (BDK_DRAWABLE (window));

  for (list = _bdk_event_queue_find_first (display); list; list = list->next)
    {
      BdkEvent *event = list->data;
      if (event->type == BDK_EXPOSE && event->expose.window == window)
        break;
    }

  if (list)
    {
      BdkEvent *retval = list->data;

      _bdk_event_queue_remove_link (display, list);
      g_list_free_1 (list);

      return retval;
    }

  return NULL;
}

void
_bdk_events_queue (BdkDisplay *display)
{
}

void
bdk_flush (void)
{
  bdk_display_flush (BDK_DISPLAY_OBJECT (_bdk_display));
}

/* Sends a ClientMessage to all toplevel client windows */
gboolean
bdk_event_send_client_message_for_display (BdkDisplay *display,
                                           BdkEvent   *event,
                                           guint32     xid)
{
  BdkWindow *win = NULL;
  gboolean ret = TRUE;

  g_return_val_if_fail (event != NULL, FALSE);

  win = bdk_window_lookup_for_display (display, (BdkNativeWindow) xid);

  g_return_val_if_fail (win != NULL, FALSE);

  if ((BDK_WINDOW_OBJECT (win)->window_type != BDK_WINDOW_CHILD) &&
      (g_object_get_data (B_OBJECT (win), "bdk-window-child-handler")))
    {
      /* Managed window, check children */
      GList *ltmp = NULL;
      for (ltmp = BDK_WINDOW_OBJECT (win)->children; ltmp; ltmp = ltmp->next)
        {
          ret &= bdk_event_send_client_message_by_window (event,
                                                          BDK_WINDOW(ltmp->data));
        }
    }
  else
    {
      ret &= bdk_event_send_client_message_by_window (event, win);
    }

  return ret;
}

/*****/

guint32
bdk_directfb_get_time (void)
{
  GTimeVal tv;

  g_get_current_time (&tv);

  return (guint32) tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void
bdk_directfb_event_windows_add (BdkWindow *window)
{
  BdkWindowImplDirectFB *impl;

  g_return_if_fail (BDK_IS_WINDOW (window));

  impl = BDK_WINDOW_IMPL_DIRECTFB (BDK_WINDOW_OBJECT (window)->impl);

  if (!impl->window)
    return;

  if (EventBuffer)
    impl->window->AttachEventBuffer (impl->window, EventBuffer);
  else
    impl->window->CreateEventBuffer (impl->window, &EventBuffer);
}

void
bdk_directfb_event_windows_remove (BdkWindow *window)
{
  BdkWindowImplDirectFB *impl;

  g_return_if_fail (BDK_IS_WINDOW (window));

  impl = BDK_WINDOW_IMPL_DIRECTFB (BDK_WINDOW_OBJECT (window)->impl);

  if (!impl->window)
    return;

  if (EventBuffer)
    impl->window->DetachEventBuffer (impl->window, EventBuffer);
  /* FIXME: should we warn if (! EventBuffer) ? */
}

BdkWindow *
bdk_directfb_child_at (BdkWindow *window,
                       gint      *winx,
                       gint      *winy)
{
  BdkWindowObject *private;
  GList           *list;

  g_return_val_if_fail (BDK_IS_WINDOW (window), NULL);

  private = BDK_WINDOW_OBJECT (window);
  for (list = private->children; list; list = list->next)
    {
      BdkWindowObject *win = list->data;
      gint wx, wy, ww, wh;

      bdk_window_get_geometry (BDK_WINDOW (win), &wx, &wy, &ww, &wh, NULL);

      if (BDK_WINDOW_IS_MAPPED (win) &&
          *winx >= wx  && *winx <  wx + ww  &&
          *winy >= wy  && *winy <  wy + wh)
        {
          *winx -= win->x;
          *winy -= win->y;

          return bdk_directfb_child_at (BDK_WINDOW (win), winx, winy);
        }
    }

  return window;
}

static gboolean
bdk_event_translate (BdkEvent       *event,
                     DFBWindowEvent *dfbevent,
                     BdkWindow      *window)
{
  BdkWindowObject *private;
  BdkDisplay      *display;
  /* BdkEvent        *event    = NULL; */
  gboolean return_val = FALSE;

  g_return_val_if_fail (event != NULL, FALSE);
  g_return_val_if_fail (dfbevent != NULL, FALSE);
  g_return_val_if_fail (BDK_IS_WINDOW (window), FALSE);

  D_DEBUG_AT (BDKDFB_Events, "%s( %p, %p, %p )\n", B_STRFUNC,
              event, dfbevent, window);

  private = BDK_WINDOW_OBJECT (window);

  g_object_ref (B_OBJECT (window));

  event->any.window = NULL;
  event->any.send_event = FALSE;

  /*
   * Apply global filters
   *
   * If result is BDK_FILTER_CONTINUE, we continue as if nothing
   * happened. If it is BDK_FILTER_REMOVE or BDK_FILTER_TRANSLATE,
   * we return TRUE and won't dispatch the event.
   */
  if (_bdk_default_filters)
    {
      BdkFilterReturn result;
      result = bdk_event_apply_filters (dfbevent, event, _bdk_default_filters);

      if (result != BDK_FILTER_CONTINUE)
        {
          return_val = (result == BDK_FILTER_TRANSLATE) ? TRUE : FALSE;
          goto done;
        }
    }

  /* Apply per-window filters */
  if (BDK_IS_WINDOW (window))
    {
      BdkFilterReturn result;

      if (private->filters)
	{
	  result = bdk_event_apply_filters (dfbevent, event, private->filters);

	  if (result != BDK_FILTER_CONTINUE)
	    {
	      return_val = (result == BDK_FILTER_TRANSLATE) ? TRUE : FALSE;
	      goto done;
	    }
	}
    }

  display = bdk_drawable_get_display (BDK_DRAWABLE (window));

  return_val = TRUE;

  switch (dfbevent->type)
    {
    case DWET_BUTTONDOWN:
    case DWET_BUTTONUP:
      /* Backend store */
      _bdk_directfb_mouse_x = dfbevent->cx;
      _bdk_directfb_mouse_y = dfbevent->cy;

      /* Event translation */
      bdk_directfb_event_fill (event, window,
                               dfbevent->type == DWET_BUTTONDOWN ?
                               BDK_BUTTON_PRESS : BDK_BUTTON_RELEASE);
      switch (dfbevent->button)
        {
        case DIBI_LEFT:
          event->button.button = 1;
          break;

        case DIBI_MIDDLE:
            event->button.button = 2;
            break;

        case DIBI_RIGHT:
          event->button.button = 3;
          break;

          default:
            event->button.button = dfbevent->button + 1;
            break;
        }

      event->button.window = window;
      event->button.x_root = dfbevent->cx;
      event->button.y_root = dfbevent->cy;
      event->button.x      = dfbevent->x;
      event->button.y      = dfbevent->y;
      event->button.state  = _bdk_directfb_modifiers;
      event->button.device = display->core_pointer;
      bdk_event_set_screen (event, _bdk_screen);

      D_DEBUG_AT (BDKDFB_MouseEvents, "  -> %s at %ix%i\n",
                  event->type == BDK_BUTTON_PRESS ? "buttonpress" : "buttonrelease",
                  (gint) event->button.x, (gint) event->button.y);
      break;

    case DWET_MOTION:
      /* Backend store */
      _bdk_directfb_mouse_x = dfbevent->cx;
      _bdk_directfb_mouse_y = dfbevent->cy;

      /* Event translation */
      bdk_directfb_event_fill (event, window, BDK_MOTION_NOTIFY);
      event->motion.x_root  = dfbevent->cx;
      event->motion.y_root  = dfbevent->cy;
      event->motion.x       = dfbevent->x;
      event->motion.y       = dfbevent->y;
      event->motion.axes    = NULL;
      event->motion.state   = _bdk_directfb_modifiers;
      event->motion.is_hint = FALSE;
      event->motion.device  = display->core_pointer;
      bdk_event_set_screen (event, _bdk_screen);

      D_DEBUG_AT (BDKDFB_MouseEvents, "  -> move pointer to %ix%i\n",
                  (gint) event->button.x, (gint) event->button.y);
      break;

    case DWET_GOTFOCUS:
      bdk_directfb_event_fill (event, window, BDK_FOCUS_CHANGE);
      event->focus_change.window = window;
      event->focus_change.in     = TRUE;
      break;

    case DWET_LOSTFOCUS:
      bdk_directfb_event_fill (event, window, BDK_FOCUS_CHANGE);
      event->focus_change.window = window;
      event->focus_change.in     = FALSE;
      break;

    case DWET_POSITION:
      bdk_directfb_event_fill (event, window, BDK_CONFIGURE);
      event->configure.x      = dfbevent->x;
      event->configure.y      = dfbevent->y;
      event->configure.width  = private->width;
      event->configure.height = private->height;
      break;

    case DWET_POSITION_SIZE:
      event->configure.x = dfbevent->x;
      event->configure.y = dfbevent->y;
      /* fallthru */

    case DWET_SIZE:
      bdk_directfb_event_fill (event, window, BDK_CONFIGURE);
      event->configure.window = window;
      event->configure.width  = dfbevent->w;
      event->configure.height = dfbevent->h;

      D_DEBUG_AT (BDKDFB_WindowEvents,
                  "  -> configure window %p at %ix%i-%ix%i\n",
                  window, event->configure.x, event->configure.y,
                  event->configure.width, event->configure.height);
      break;

    case DWET_KEYDOWN:
    case DWET_KEYUP:
      bdk_directfb_event_fill (event, window,
                               dfbevent->type == DWET_KEYUP ?
                               BDK_KEY_RELEASE : BDK_KEY_PRESS);
      event->key.window = window;
      bdk_directfb_translate_key_event (dfbevent, (BdkEventKey *) event);

      D_DEBUG_AT (BDKDFB_KeyEvents, "  -> key window=%p val=%x code=%x str=%s\n",
                  window,  event->key.keyval, event->key.hardware_keycode,
                  event->key.string);
      break;

    case DWET_ENTER:
    case DWET_LEAVE:
      /* Backend store */
      _bdk_directfb_mouse_x = dfbevent->cx;
      _bdk_directfb_mouse_y = dfbevent->cy;

      /* Event translation */
      bdk_directfb_event_fill (event, window,
                               dfbevent->type == DWET_ENTER ?
                               BDK_ENTER_NOTIFY : BDK_LEAVE_NOTIFY);
      event->crossing.window    = g_object_ref (window);
      event->crossing.subwindow = NULL;
      event->crossing.time      = BDK_CURRENT_TIME;
      event->crossing.x         = dfbevent->x;
      event->crossing.y         = dfbevent->y;
      event->crossing.x_root    = dfbevent->cx;
      event->crossing.y_root    = dfbevent->cy;
      event->crossing.mode      = BDK_CROSSING_NORMAL;
      event->crossing.detail    = BDK_NOTIFY_ANCESTOR;
      event->crossing.state     = 0;

      /**/
      if (bdk_directfb_apply_focus_opacity)
        {
          if (dfbevent->type == DWET_ENTER)
            {
              if (BDK_WINDOW_IS_MAPPED (window))
                BDK_WINDOW_IMPL_DIRECTFB (private->impl)->window->SetOpacity
                  (BDK_WINDOW_IMPL_DIRECTFB (private->impl)->window,
                   (BDK_WINDOW_IMPL_DIRECTFB (private->impl)->opacity >> 1) +
                   (BDK_WINDOW_IMPL_DIRECTFB (private->impl)->opacity >> 2));
            }
          else
            {
              BDK_WINDOW_IMPL_DIRECTFB (private->impl)->window->SetOpacity
                (BDK_WINDOW_IMPL_DIRECTFB (private->impl)->window,
                 BDK_WINDOW_IMPL_DIRECTFB (private->impl)->opacity);
            }
        }

      D_DEBUG_AT (BDKDFB_WindowEvents, "  -> %s window %p at relative=%ix%i absolute=%ix%i\n",
                  dfbevent->type == DWET_ENTER ? "enter" : "leave",
                  window, (gint) event->crossing.x, (gint) event->crossing.y,
                  (gint) event->crossing.x_root, (gint) event->crossing.y_root);
      break;

    case DWET_CLOSE:
      bdk_directfb_event_fill (event, window, BDK_DELETE);
      break;

    case DWET_DESTROYED:
      bdk_directfb_event_fill (event, window, BDK_DESTROY);
      bdk_window_destroy_notify (window);
      break;

    case DWET_WHEEL:
      /* Backend store */
      _bdk_directfb_mouse_x = dfbevent->cx;
      _bdk_directfb_mouse_y = dfbevent->cy;

      /* Event translation */
      bdk_directfb_event_fill (event, window, BDK_SCROLL);
      event->scroll.direction = (dfbevent->step > 0 ?
                                 BDK_SCROLL_UP : BDK_SCROLL_DOWN);
      event->scroll.x_root    = dfbevent->cx;
      event->scroll.y_root    = dfbevent->cy;
      event->scroll.x         = dfbevent->x;
      event->scroll.y         = dfbevent->y;
      event->scroll.state     = _bdk_directfb_modifiers;
      event->scroll.device    = display->core_pointer;

      D_DEBUG_AT (BDKDFB_MouseEvents, "  -> mouse scroll %s at %ix%i\n",
                  event->scroll.direction == BDK_SCROLL_UP ? "up" : "down",
                  (gint) event->scroll.x, (gint) event->scroll.y);
      break;

    default:
      g_message ("unhandled DirectFB windowing event 0x%08x", dfbevent->type);
    }

 done:

  g_object_unref (B_OBJECT (window));

  return return_val;
}

gboolean
bdk_screen_get_setting (BdkScreen   *screen,
                        const gchar *name,
                        BValue      *value)
{
  return FALSE;
}

void
bdk_display_add_client_message_filter (BdkDisplay   *display,
                                       BdkAtom       message_type,
                                       BdkFilterFunc func,
                                       gpointer      data)
{
  /* XXX: display should be used */
  BdkClientFilter *filter = g_new (BdkClientFilter, 1);

  filter->type = message_type;
  filter->function = func;
  filter->data = data;
  client_filters = g_list_append (client_filters, filter);
}


void
bdk_add_client_message_filter (BdkAtom       message_type,
                               BdkFilterFunc func,
                               gpointer      data)
{
  bdk_display_add_client_message_filter (bdk_display_get_default (),
                                         message_type, func, data);
}

void
bdk_screen_broadcast_client_message (BdkScreen *screen,
				     BdkEvent  *sev)
{
  BdkWindow       *root_window;
  BdkWindowObject *private;
  GList           *top_level = NULL;

  g_return_if_fail (BDK_IS_SCREEN (screen));
  g_return_if_fail (sev != NULL);

  root_window = bdk_screen_get_root_window (screen);

  g_return_if_fail (BDK_IS_WINDOW (root_window));

  private = BDK_WINDOW_OBJECT (root_window);

  for (top_level = private->children; top_level; top_level = top_level->next)
    {
      bdk_event_send_client_message_for_display (bdk_drawable_get_display (BDK_DRAWABLE (root_window)),
                                                 sev,
                                                 (guint32)(BDK_WINDOW_DFB_ID (BDK_WINDOW (top_level->data))));
    }
}


/**
 * bdk_net_wm_supports:
 * @property: a property atom.
 *
 * This function is specific to the X11 backend of BDK, and indicates
 * whether the window manager for the default screen supports a certain
 * hint from the Extended Window Manager Hints Specification. See
 * bdk_x11_screen_supports_net_wm_hint() for complete details.
 *
 * Return value: %TRUE if the window manager supports @property
 **/


gboolean
bdk_net_wm_supports (BdkAtom property)
{
  return FALSE;
}

void
_bdk_windowing_event_data_copy (const BdkEvent *src,
                                BdkEvent       *dst)
{
}

void
_bdk_windowing_event_data_free (BdkEvent *event)
{
}

#define __BDK_EVENTS_X11_C__
#include "bdkaliasdef.c"
