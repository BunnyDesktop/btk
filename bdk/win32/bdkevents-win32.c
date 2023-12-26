/* BDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998-2002 Tor Lillqvist
 * Copyright (C) 2001,2009 Hans Breuer
 * Copyright (C) 2007-2009 Cody Russell
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

/* Cannot use TrackMouseEvent, as the stupid WM_MOUSELEAVE message
 * doesn't tell us where the mouse has gone. Thus we cannot use it to
 * generate a correct BdkNotifyType. Pity, as using TrackMouseEvent
 * otherwise would make it possible to reliably generate
 * BDK_LEAVE_NOTIFY events, which would help get rid of those pesky
 * tooltips sometimes popping up in the wrong place.
 *
 * Update: a combination of TrackMouseEvent, GetCursorPos and 
 * GetWindowPos can and is actually used to get rid of those
 * pesky tooltips. It should be possible to use this for the
 * whole ENTER/LEAVE NOTIFY handling but some platforms may
 * not have TrackMouseEvent at all (?) --hb
 */

#include "config.h"

#include <bunnylib/gprintf.h>

#include "bdk.h"
#include "bdkprivate-win32.h"
#include "bdkinput-win32.h"
#include "bdkkeysyms.h"

#include <windowsx.h>

#ifdef G_WITH_CYGWIN
#include <fcntl.h>
#include <errno.h>
#endif

#include <objbase.h>

#include <imm.h>

#ifndef XBUTTON1
#define XBUTTON1 1
#define XBUTTON2 2
#endif

#ifndef VK_XBUTTON1
#define VK_XBUTTON1 5
#define VK_XBUTTON2 6
#endif

#ifndef MK_XBUTTON1
#define MK_XBUTTON1 32
#define MK_XBUTTON2 64
#endif

/* Undefined flags: */
#define SWP_NOCLIENTSIZE 0x0800
#define SWP_NOCLIENTMOVE 0x1000
#define SWP_STATECHANGED 0x8000
/* 
 * Private function declarations
 */

#define SYNAPSIS_ICON_WINDOW_CLASS "SynTrackCursorWindowClass"

static bboolean bdk_event_translate (MSG        *msg,
				     bint       *ret_valp);
static void     handle_wm_paint     (MSG        *msg,
				     BdkWindow  *window,
				     bboolean    return_exposes,
				     BdkEvent  **event);

static bboolean bdk_event_prepare  (GSource     *source,
				    bint        *timeout);
static bboolean bdk_event_check    (GSource     *source);
static bboolean bdk_event_dispatch (GSource     *source,
				    GSourceFunc  callback,
				    bpointer     user_data);

/* Private variable declarations
 */

static GList *client_filters;	/* Filters for client messages */

static HCURSOR p_grab_cursor;

static GSourceFuncs event_funcs = {
  bdk_event_prepare,
  bdk_event_check,
  bdk_event_dispatch,
  NULL
};

GPollFD event_poll_fd;

static BdkWindow *mouse_window = NULL;
static BdkWindow *mouse_window_ignored_leave = NULL;
static bint current_x, current_y;
static bint current_root_x, current_root_y;
static UINT client_message;

static UINT got_bdk_events_message;
static HWND modal_win32_dialog = NULL;

#if 0
static HKL latin_locale = NULL;
#endif

static bboolean in_ime_composition = FALSE;
static UINT     modal_timer;
static UINT     sync_timer = 0;

static int debug_indent = 0;

static void
assign_object (bpointer lhsp,
	       bpointer rhs)
{
  if (*(bpointer *)lhsp != rhs)
    {
      if (*(bpointer *)lhsp != NULL)
	g_object_unref (*(bpointer *)lhsp);
      *(bpointer *)lhsp = rhs;
      if (rhs != NULL)
	g_object_ref (rhs);
    }
}

static void
track_mouse_event (DWORD dwFlags,
		   HWND  hwnd)
{
  TRACKMOUSEEVENT tme;

  tme.cbSize = sizeof(TRACKMOUSEEVENT);
  tme.dwFlags = dwFlags;
  tme.hwndTrack = hwnd;
  tme.dwHoverTime = HOVER_DEFAULT; /* not used */

  if (!TrackMouseEvent (&tme))
    WIN32_API_FAILED ("TrackMouseEvent");
  else if (dwFlags == TME_LEAVE)
    BDK_NOTE (EVENTS, g_print(" (TrackMouseEvent %p)", hwnd));
  else if (dwFlags == TME_CANCEL)
    BDK_NOTE (EVENTS, g_print(" (cancel TrackMouseEvent %p)", hwnd));
}

bulong
_bdk_win32_get_next_tick (bulong suggested_tick)
{
  static bulong cur_tick = 0;

  if (suggested_tick == 0)
    suggested_tick = GetTickCount ();
  if (suggested_tick <= cur_tick)
    return cur_tick;
  else
    return cur_tick = suggested_tick;
}

static void
generate_focus_event (BdkWindow *window,
		      bboolean   in)
{
  BdkEvent *event;

  event = bdk_event_new (BDK_FOCUS_CHANGE);
  event->focus_change.window = window;
  event->focus_change.in = in;

  _bdk_win32_append_event (event);
}

static void
generate_grab_broken_event (BdkWindow *window,
			    bboolean   keyboard,
			    BdkWindow *grab_window)
{
  BdkEvent *event = bdk_event_new (BDK_GRAB_BROKEN);

  event->grab_broken.window = window;
  event->grab_broken.send_event = 0;
  event->grab_broken.keyboard = keyboard;
  event->grab_broken.implicit = FALSE;
  event->grab_broken.grab_window = grab_window;
	  
  _bdk_win32_append_event (event);
}

static LRESULT 
inner_window_procedure (HWND   hwnd,
			UINT   message,
			WPARAM wparam,
			LPARAM lparam)
{
  MSG msg;
  DWORD pos;
  bint ret_val = 0;

  msg.hwnd = hwnd;
  msg.message = message;
  msg.wParam = wparam;
  msg.lParam = lparam;
  msg.time = _bdk_win32_get_next_tick (0);
  pos = GetMessagePos ();
  msg.pt.x = GET_X_LPARAM (pos);
  msg.pt.y = GET_Y_LPARAM (pos);

  if (bdk_event_translate (&msg, &ret_val))
    {
      /* If bdk_event_translate() returns TRUE, we return ret_val from
       * the window procedure.
       */
      if (modal_win32_dialog)
	PostMessageW (modal_win32_dialog, got_bdk_events_message,
		      (WPARAM) 1, 0);
      return ret_val;
    }
  else
    {
      /* Otherwise call DefWindowProcW(). */
      BDK_NOTE (EVENTS, g_print (" DefWindowProcW"));
      return DefWindowProcW (hwnd, message, wparam, lparam);
    }
}

LRESULT CALLBACK
_bdk_win32_window_procedure (HWND   hwnd,
                             UINT   message,
                             WPARAM wparam,
                             LPARAM lparam)
{
  LRESULT retval;

  BDK_NOTE (EVENTS, g_print ("%s%*s%s %p",
			     (debug_indent > 0 ? "\n" : ""),
			     debug_indent, "", 
			     _bdk_win32_message_to_string (message), hwnd));
  debug_indent += 2;
  retval = inner_window_procedure (hwnd, message, wparam, lparam);
  debug_indent -= 2;

  BDK_NOTE (EVENTS, g_print (" => %I64d%s", (bint64) retval, (debug_indent == 0 ? "\n" : "")));

  return retval;
}

void 
_bdk_events_init (void)
{
  GSource *source;

#if 0
  int i, j, n;

  /* List of languages that use a latin keyboard. Somewhat sorted in
   * "order of least surprise", in case we have to load one of them if
   * the user only has arabic loaded, for instance.
   */
  static int latin_languages[] = {
    LANG_ENGLISH,
    LANG_SPANISH,
    LANG_PORTUGUESE,
    LANG_FRENCH,
    LANG_GERMAN,
    /* Rest in numeric order */
    LANG_CZECH,
    LANG_DANISH,
    LANG_FINNISH,
    LANG_HUNGARIAN,
    LANG_ICELANDIC,
    LANG_ITALIAN,
    LANG_DUTCH,
    LANG_NORWEGIAN,
    LANG_POLISH,
    LANG_ROMANIAN,
    LANG_SLOVAK,
    LANG_ALBANIAN,
    LANG_SWEDISH,
    LANG_TURKISH,
    LANG_INDONESIAN,
    LANG_SLOVENIAN,
    LANG_ESTONIAN,
    LANG_LATVIAN,
    LANG_LITHUANIAN,
    LANG_VIETNAMESE,
    LANG_AFRIKAANS,
    LANG_FAEROESE
#ifdef LANG_SWAHILI
   ,LANG_SWAHILI
#endif
  };
#endif

  client_message = RegisterWindowMessage ("BDK_WIN32_CLIENT_MESSAGE");
  got_bdk_events_message = RegisterWindowMessage ("BDK_WIN32_GOT_EVENTS");

#if 0
  /* Check if we have some input locale identifier loaded that uses a
   * latin keyboard, to be able to get the virtual-key code for the
   * latin characters corresponding to ASCII control characters.
   */
  if ((n = GetKeyboardLayoutList (0, NULL)) == 0)
    WIN32_API_FAILED ("GetKeyboardLayoutList");
  else
    {
      HKL *hkl_list = g_new (HKL, n);
      if (GetKeyboardLayoutList (n, hkl_list) == 0)
	WIN32_API_FAILED ("GetKeyboardLayoutList");
      else
	{
	  for (i = 0; latin_locale == NULL && i < n; i++)
	    for (j = 0; j < G_N_ELEMENTS (latin_languages); j++)
	      if (PRIMARYLANGID (LOWORD (hkl_list[i])) == latin_languages[j])
		{
		  latin_locale = hkl_list [i];
		  break;
		}
	}
      g_free (hkl_list);
    }

  if (latin_locale == NULL)
    {
      /* Try to load a keyboard layout with latin characters then.
       */
      i = 0;
      while (latin_locale == NULL && i < G_N_ELEMENTS (latin_languages))
	{
	  char id[9];
	  g_sprintf (id, "%08x", MAKELANGID (latin_languages[i++], SUBLANG_DEFAULT));
	  latin_locale = LoadKeyboardLayout (id, KLF_NOTELLSHELL|KLF_SUBSTITUTE_OK);
	}
    }

  BDK_NOTE (EVENTS, g_print ("latin_locale = %08x\n", (buint) latin_locale));
#endif

  source = g_source_new (&event_funcs, sizeof (GSource));
  g_source_set_name (source, "BDK Win32 event source"); 
  g_source_set_priority (source, BDK_PRIORITY_EVENTS);

#ifdef G_WITH_CYGWIN
  event_poll_fd.fd = open ("/dev/windows", O_RDONLY);
  if (event_poll_fd.fd == -1)
    g_error ("can't open \"/dev/windows\": %s", g_strerror (errno));
#else
  event_poll_fd.fd = G_WIN32_MSG_HANDLE;
#endif
  event_poll_fd.events = G_IO_IN;
  
  g_source_add_poll (source, &event_poll_fd);
  g_source_set_can_recurse (source, TRUE);
  g_source_attach (source, NULL);
}

bboolean
bdk_events_pending (void)
{
  return (_bdk_event_queue_find_first (_bdk_display) ||
	  (modal_win32_dialog == NULL &&
	   GetQueueStatus (QS_ALLINPUT) != 0));
}

BdkEvent*
bdk_event_get_graphics_expose (BdkWindow *window)
{
  MSG msg;
  BdkEvent *event = NULL;

  g_return_val_if_fail (window != NULL, NULL);
  
  BDK_NOTE (EVENTS, g_print ("bdk_event_get_graphics_expose\n"));

  if (PeekMessageW (&msg, BDK_WINDOW_HWND (window), WM_PAINT, WM_PAINT, PM_REMOVE))
    {
      handle_wm_paint (&msg, window, TRUE, &event);
      if (event != NULL)
	{
	  BDK_NOTE (EVENTS, g_print ("bdk_event_get_graphics_expose: got it!\n"));
	  return event;
	}
    }
  
  BDK_NOTE (EVENTS, g_print ("bdk_event_get_graphics_expose: nope\n"));
  return NULL;	
}

#if 0 /* Unused, but might be useful to re-introduce in some debugging output? */

static char *
event_mask_string (BdkEventMask mask)
{
  static char bfr[500];
  char *p = bfr;

  *p = '\0';
#define BIT(x) \
  if (mask & BDK_##x##_MASK) \
    p += g_sprintf (p, "%s" #x, (p > bfr ? " " : ""))
  BIT (EXPOSURE);
  BIT (POINTER_MOTION);
  BIT (POINTER_MOTION_HINT);
  BIT (BUTTON_MOTION);
  BIT (BUTTON1_MOTION);
  BIT (BUTTON2_MOTION);
  BIT (BUTTON3_MOTION);
  BIT (BUTTON_PRESS);
  BIT (BUTTON_RELEASE);
  BIT (KEY_PRESS);
  BIT (KEY_RELEASE);
  BIT (ENTER_NOTIFY);
  BIT (LEAVE_NOTIFY);
  BIT (FOCUS_CHANGE);
  BIT (STRUCTURE);
  BIT (PROPERTY_CHANGE);
  BIT (VISIBILITY_NOTIFY);
  BIT (PROXIMITY_IN);
  BIT (PROXIMITY_OUT);
  BIT (SUBSTRUCTURE);
  BIT (SCROLL);
#undef BIT

  return bfr;
}

#endif

BdkGrabStatus
_bdk_windowing_pointer_grab (BdkWindow    *window,
			     BdkWindow    *native_window,
			     bboolean	owner_events,
			     BdkEventMask	event_mask,
			     BdkWindow    *confine_to,
			     BdkCursor    *cursor,
			     buint32	time)
{
  HCURSOR hcursor;
  BdkCursorPrivate *cursor_private;
  bint return_val;

  g_return_val_if_fail (window != NULL, 0);
  g_return_val_if_fail (BDK_IS_WINDOW (window), 0);
  g_return_val_if_fail (confine_to == NULL || BDK_IS_WINDOW (confine_to), 0);
  
  cursor_private = (BdkCursorPrivate*) cursor;
  
  if (!cursor)
    hcursor = NULL;
  else if ((hcursor = CopyCursor (cursor_private->hcursor)) == NULL)
    WIN32_API_FAILED ("CopyCursor");

  return_val = _bdk_input_grab_pointer (native_window,
					owner_events,
					event_mask,
					confine_to,
					time);

  if (return_val == BDK_GRAB_SUCCESS)
    {
      BdkWindowImplWin32 *impl = BDK_WINDOW_IMPL_WIN32 (((BdkWindowObject *) native_window)->impl);

      SetCapture (BDK_WINDOW_HWND (native_window));
      /* TODO_CSW: grab brokens, confine window, input_grab */
      if (p_grab_cursor != NULL)
	{
	  if (GetCursor () == p_grab_cursor)
	    SetCursor (NULL);
	  DestroyCursor (p_grab_cursor);
	}

      p_grab_cursor = hcursor;

      if (p_grab_cursor != NULL)
	SetCursor (p_grab_cursor);
      else if (impl->hcursor != NULL)
	SetCursor (impl->hcursor);
      else
	SetCursor (LoadCursor (NULL, IDC_ARROW));

    }

  return return_val;
}

void
bdk_display_pointer_ungrab (BdkDisplay *display,
                            buint32     time)
{
  BdkPointerGrabInfo *info;

  info = _bdk_display_get_last_pointer_grab (display);
  if (info)
    {
      info->serial_end = 0;
      ReleaseCapture ();
    }

  _bdk_input_ungrab_pointer (time);

  /* TODO_CSW: cursor, confines, etc */

  _bdk_display_pointer_grab_update (display, 0);
}


static BdkWindow *
find_window_for_mouse_event (BdkWindow* reported_window,
			     MSG*       msg)
{
  POINT pt;
  BdkWindow *event_window;
  HWND hwnd;
  RECT rect;
  BdkPointerGrabInfo *grab;

  grab = _bdk_display_get_last_pointer_grab (_bdk_display);
  if (grab == NULL)
    return reported_window;

  pt = msg->pt;

  if (!grab->owner_events)
    event_window = grab->native_window;
  else
    {
      event_window = NULL;
      hwnd = WindowFromPoint (pt);
      if (hwnd != NULL)
	{
	  POINT client_pt = pt;

	  ScreenToClient (hwnd, &client_pt);
	  GetClientRect (hwnd, &rect);
	  if (PtInRect (&rect, client_pt))
	    event_window = bdk_win32_handle_table_lookup ((BdkNativeWindow) hwnd);
	}
      if (event_window == NULL)
	event_window = grab->native_window;
    }

  /* need to also adjust the coordinates to the new window */
  ScreenToClient (BDK_WINDOW_HWND (event_window), &pt);

  /* ATTENTION: need to update client coords */
  msg->lParam = MAKELPARAM (pt.x, pt.y);

  return event_window;
}

BdkGrabStatus
bdk_keyboard_grab (BdkWindow *window,
		   bboolean   owner_events,
		   buint32    time)
{
  BdkDisplay *display;
  BdkWindow  *toplevel;

  g_return_val_if_fail (window != NULL, 0);
  g_return_val_if_fail (BDK_IS_WINDOW (window), 0);
  
  BDK_NOTE (EVENTS, g_print ("bdk_keyboard_grab %p%s\n",
			     BDK_WINDOW_HWND (window), owner_events ? " OWNER_EVENTS" : ""));

  display = bdk_drawable_get_display (window);
  toplevel = bdk_window_get_toplevel (window);

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
                             buint32 time)
{
  BDK_NOTE (EVENTS, g_print ("bdk_display_keyboard_ungrab\n"));
  _bdk_display_unset_has_keyboard_grab (display, FALSE);
}

void 
bdk_display_add_client_message_filter (BdkDisplay   *display,
				       BdkAtom       message_type,
				       BdkFilterFunc func,
				       bpointer      data)
{
  /* XXX */
  bdk_add_client_message_filter (message_type, func, data);
}

void
bdk_add_client_message_filter (BdkAtom       message_type,
			       BdkFilterFunc func,
			       bpointer      data)
{
  BdkClientFilter *filter = g_new (BdkClientFilter, 1);

  filter->type = message_type;
  filter->function = func;
  filter->data = data;
  
  client_filters = g_list_append (client_filters, filter);
}

static void
build_key_event_state (BdkEvent *event,
		       BYTE     *key_state)
{
  BdkWin32Keymap *keymap;

  event->key.state = 0;

  if (key_state[VK_SHIFT] & 0x80)
    event->key.state |= BDK_SHIFT_MASK;

  if (key_state[VK_CAPITAL] & 0x01)
    event->key.state |= BDK_LOCK_MASK;

  if (key_state[VK_LBUTTON] & 0x80)
    event->key.state |= BDK_BUTTON1_MASK;
  if (key_state[VK_MBUTTON] & 0x80)
    event->key.state |= BDK_BUTTON2_MASK;
  if (key_state[VK_RBUTTON] & 0x80)
    event->key.state |= BDK_BUTTON3_MASK;
  if (key_state[VK_XBUTTON1] & 0x80)
    event->key.state |= BDK_BUTTON4_MASK;
  if (key_state[VK_XBUTTON2] & 0x80)
    event->key.state |= BDK_BUTTON5_MASK;

  keymap = BDK_WIN32_KEYMAP (bdk_keymap_get_default ());
  event->key.group = _bdk_win32_keymap_get_active_group (keymap);

  if (_bdk_win32_keymap_has_altgr (keymap) &&
      (key_state[VK_LCONTROL] & 0x80) &&
      (key_state[VK_RMENU] & 0x80))
    {
      event->key.state |= BDK_MOD2_MASK;
      if (key_state[VK_RCONTROL] & 0x80)
	event->key.state |= BDK_CONTROL_MASK;
      if (key_state[VK_LMENU] & 0x80)
	event->key.state |= BDK_MOD1_MASK;
    }
  else
    {
      if (key_state[VK_CONTROL] & 0x80)
	event->key.state |= BDK_CONTROL_MASK;
      if (key_state[VK_MENU] & 0x80)
	event->key.state |= BDK_MOD1_MASK;
    }
}

static bint
build_pointer_event_state (MSG *msg)
{
  bint state;
  
  state = 0;

  if (msg->wParam & MK_CONTROL)
    state |= BDK_CONTROL_MASK;

  if ((msg->message != WM_LBUTTONDOWN &&
       (msg->wParam & MK_LBUTTON)) ||
      msg->message == WM_LBUTTONUP)
    state |= BDK_BUTTON1_MASK;

  if ((msg->message != WM_MBUTTONDOWN &&
       (msg->wParam & MK_MBUTTON)) ||
      msg->message == WM_MBUTTONUP)
    state |= BDK_BUTTON2_MASK;

  if ((msg->message != WM_RBUTTONDOWN &&
       (msg->wParam & MK_RBUTTON)) ||
      msg->message == WM_RBUTTONUP)
    state |= BDK_BUTTON3_MASK;

  if (((msg->message != WM_XBUTTONDOWN || HIWORD (msg->wParam) != XBUTTON1) &&
       (msg->wParam & MK_XBUTTON1)) ||
      (msg->message == WM_XBUTTONUP && HIWORD (msg->wParam) == XBUTTON1))
    state |= BDK_BUTTON4_MASK;

  if (((msg->message != WM_XBUTTONDOWN || HIWORD (msg->wParam) != XBUTTON2) &&
       (msg->wParam & MK_XBUTTON2)) ||
      (msg->message == WM_XBUTTONUP && HIWORD (msg->wParam) == XBUTTON2))
    state |= BDK_BUTTON5_MASK;

  if (msg->wParam & MK_SHIFT)
    state |= BDK_SHIFT_MASK;

  if (GetKeyState (VK_MENU) < 0)
    state |= BDK_MOD1_MASK;

  if (GetKeyState (VK_CAPITAL) & 0x1)
    state |= BDK_LOCK_MASK;

  return state;
}

static void
build_wm_ime_composition_event (BdkEvent *event,
				MSG      *msg,
				wchar_t   wc,
				BYTE     *key_state)
{
  event->key.time = _bdk_win32_get_next_tick (msg->time);
  
  build_key_event_state (event, key_state);

  event->key.hardware_keycode = 0; /* FIXME: What should it be? */
  event->key.string = NULL;
  event->key.length = 0;
  event->key.keyval = bdk_unicode_to_keyval (wc);
}

#ifdef G_ENABLE_DEBUG

static void
print_event_state (buint state)
{
#define CASE(bit) if (state & BDK_ ## bit ## _MASK) g_print (#bit " ");
  CASE (SHIFT);
  CASE (LOCK);
  CASE (CONTROL);
  CASE (MOD1);
  CASE (MOD2);
  CASE (MOD3);
  CASE (MOD4);
  CASE (MOD5);
  CASE (BUTTON1);
  CASE (BUTTON2);
  CASE (BUTTON3);
  CASE (BUTTON4);
  CASE (BUTTON5);
#undef CASE
}

void
_bdk_win32_print_event (const BdkEvent *event)
{
  bchar *escaped, *kvname;
  bchar *selection_name, *target_name, *property_name;

  g_print ("%s%*s===> ", (debug_indent > 0 ? "\n" : ""), debug_indent, "");
  switch (event->any.type)
    {
#define CASE(x) case x: g_print (#x); break;
    CASE (BDK_NOTHING);
    CASE (BDK_DELETE);
    CASE (BDK_DESTROY);
    CASE (BDK_EXPOSE);
    CASE (BDK_MOTION_NOTIFY);
    CASE (BDK_BUTTON_PRESS);
    CASE (BDK_2BUTTON_PRESS);
    CASE (BDK_3BUTTON_PRESS);
    CASE (BDK_BUTTON_RELEASE);
    CASE (BDK_KEY_PRESS);
    CASE (BDK_KEY_RELEASE);
    CASE (BDK_ENTER_NOTIFY);
    CASE (BDK_LEAVE_NOTIFY);
    CASE (BDK_FOCUS_CHANGE);
    CASE (BDK_CONFIGURE);
    CASE (BDK_MAP);
    CASE (BDK_UNMAP);
    CASE (BDK_PROPERTY_NOTIFY);
    CASE (BDK_SELECTION_CLEAR);
    CASE (BDK_SELECTION_REQUEST);
    CASE (BDK_SELECTION_NOTIFY);
    CASE (BDK_PROXIMITY_IN);
    CASE (BDK_PROXIMITY_OUT);
    CASE (BDK_DRAG_ENTER);
    CASE (BDK_DRAG_LEAVE);
    CASE (BDK_DRAG_MOTION);
    CASE (BDK_DRAG_STATUS);
    CASE (BDK_DROP_START);
    CASE (BDK_DROP_FINISHED);
    CASE (BDK_CLIENT_EVENT);
    CASE (BDK_VISIBILITY_NOTIFY);
    CASE (BDK_NO_EXPOSE);
    CASE (BDK_SCROLL);
    CASE (BDK_WINDOW_STATE);
    CASE (BDK_SETTING);
    CASE (BDK_OWNER_CHANGE);
    CASE (BDK_GRAB_BROKEN);
#undef CASE
    default: g_assert_not_reached ();
    }

  g_print (" %p ", event->any.window ? BDK_WINDOW_HWND (event->any.window) : NULL);

  switch (event->any.type)
    {
    case BDK_EXPOSE:
      g_print ("%s %d",
	       _bdk_win32_bdkrectangle_to_string (&event->expose.area),
	       event->expose.count);
      break;
    case BDK_MOTION_NOTIFY:
      g_print ("(%.4g,%.4g) (%.4g,%.4g) %s",
	       event->motion.x, event->motion.y,
	       event->motion.x_root, event->motion.y_root,
	       event->motion.is_hint ? "HINT " : "");
      print_event_state (event->motion.state);
      break;
    case BDK_BUTTON_PRESS:
    case BDK_2BUTTON_PRESS:
    case BDK_3BUTTON_PRESS:
    case BDK_BUTTON_RELEASE:
      g_print ("%d (%.4g,%.4g) (%.4g,%.4g) ",
	       event->button.button,
	       event->button.x, event->button.y,
	       event->button.x_root, event->button.y_root);
      print_event_state (event->button.state);
      break;
    case BDK_KEY_PRESS: 
    case BDK_KEY_RELEASE:
      if (event->key.length == 0)
	escaped = g_strdup ("");
      else
	escaped = g_strescape (event->key.string, NULL);
      kvname = bdk_keyval_name (event->key.keyval);
      g_print ("%#.02x group:%d %s %d:\"%s\" ",
	       event->key.hardware_keycode, event->key.group,
	       (kvname ? kvname : "??"),
	       event->key.length,
	       escaped);
      g_free (escaped);
      print_event_state (event->key.state);
      break;
    case BDK_ENTER_NOTIFY:
    case BDK_LEAVE_NOTIFY:
      g_print ("%p (%.4g,%.4g) (%.4g,%.4g) %s %s%s",
	       event->crossing.subwindow == NULL ? NULL : BDK_WINDOW_HWND (event->crossing.subwindow),
	       event->crossing.x, event->crossing.y,
	       event->crossing.x_root, event->crossing.y_root,
	       (event->crossing.mode == BDK_CROSSING_NORMAL ? "NORMAL" :
		(event->crossing.mode == BDK_CROSSING_GRAB ? "GRAB" :
		 (event->crossing.mode == BDK_CROSSING_UNGRAB ? "UNGRAB" :
		  "???"))),
	       (event->crossing.detail == BDK_NOTIFY_ANCESTOR ? "ANCESTOR" :
		(event->crossing.detail == BDK_NOTIFY_VIRTUAL ? "VIRTUAL" :
		 (event->crossing.detail == BDK_NOTIFY_INFERIOR ? "INFERIOR" :
		  (event->crossing.detail == BDK_NOTIFY_NONLINEAR ? "NONLINEAR" :
		   (event->crossing.detail == BDK_NOTIFY_NONLINEAR_VIRTUAL ? "NONLINEAR_VIRTUAL" :
		    (event->crossing.detail == BDK_NOTIFY_UNKNOWN ? "UNKNOWN" :
		     "???")))))),
	       event->crossing.focus ? " FOCUS" : "");
      print_event_state (event->crossing.state);
      break;
    case BDK_FOCUS_CHANGE:
      g_print ("%s", (event->focus_change.in ? "IN" : "OUT"));
      break;
    case BDK_CONFIGURE:
      g_print ("x:%d y:%d w:%d h:%d",
	       event->configure.x, event->configure.y,
	       event->configure.width, event->configure.height);
      break;
    case BDK_SELECTION_CLEAR:
    case BDK_SELECTION_REQUEST:
    case BDK_SELECTION_NOTIFY:
      selection_name = bdk_atom_name (event->selection.selection);
      target_name = bdk_atom_name (event->selection.target);
      property_name = bdk_atom_name (event->selection.property);
      g_print ("sel:%s tgt:%s prop:%s",
	       selection_name, target_name, property_name);
      g_free (selection_name);
      g_free (target_name);
      g_free (property_name);
      break;
    case BDK_DRAG_ENTER:
    case BDK_DRAG_LEAVE:
    case BDK_DRAG_MOTION:
    case BDK_DRAG_STATUS:
    case BDK_DROP_START:
    case BDK_DROP_FINISHED:
      if (event->dnd.context != NULL)
	g_print ("ctx:%p: %s %s src:%p dest:%p",
		 event->dnd.context,
		 _bdk_win32_drag_protocol_to_string (event->dnd.context->protocol),
		 event->dnd.context->is_source ? "SOURCE" : "DEST",
		 event->dnd.context->source_window == NULL ? NULL : BDK_WINDOW_HWND (event->dnd.context->source_window),
		 event->dnd.context->dest_window == NULL ? NULL : BDK_WINDOW_HWND (event->dnd.context->dest_window));
      break;
    case BDK_CLIENT_EVENT:
      g_print ("%s %d %ld %ld %ld %ld %ld",
	       bdk_atom_name (event->client.message_type),
	       event->client.data_format,
	       event->client.data.l[0],
	       event->client.data.l[1],
	       event->client.data.l[2],
	       event->client.data.l[3],
	       event->client.data.l[4]);
      break;
    case BDK_SCROLL:
      g_print ("(%.4g,%.4g) (%.4g,%.4g) %s ",
	       event->scroll.x, event->scroll.y,
	       event->scroll.x_root, event->scroll.y_root,
	       (event->scroll.direction == BDK_SCROLL_UP ? "UP" :
		(event->scroll.direction == BDK_SCROLL_DOWN ? "DOWN" :
		 (event->scroll.direction == BDK_SCROLL_LEFT ? "LEFT" :
		  (event->scroll.direction == BDK_SCROLL_RIGHT ? "RIGHT" :
		   "???")))));
      print_event_state (event->scroll.state);
      break;
    case BDK_WINDOW_STATE:
      g_print ("%s: %s",
	       _bdk_win32_window_state_to_string (event->window_state.changed_mask),
	       _bdk_win32_window_state_to_string (event->window_state.new_window_state));
    case BDK_SETTING:
      g_print ("%s: %s",
	       (event->setting.action == BDK_SETTING_ACTION_NEW ? "NEW" :
		(event->setting.action == BDK_SETTING_ACTION_CHANGED ? "CHANGED" :
		 (event->setting.action == BDK_SETTING_ACTION_DELETED ? "DELETED" :
		  "???"))),
	       (event->setting.name ? event->setting.name : "NULL"));
    case BDK_GRAB_BROKEN:
      g_print ("%s %s %p",
	       (event->grab_broken.keyboard ? "KEYBOARD" : "POINTER"),
	       (event->grab_broken.implicit ? "IMPLICIT" : "EXPLICIT"),
	       (event->grab_broken.grab_window ? BDK_WINDOW_HWND (event->grab_broken.grab_window) : 0));
    default:
      /* Nothing */
      break;
    }  
  g_print ("%s", (debug_indent == 0 ? "\n" : "")); 
}

static char *
decode_key_lparam (LPARAM lParam)
{
  static char buf[100];
  char *p = buf;

  if (HIWORD (lParam) & KF_UP)
    p += g_sprintf (p, "KF_UP ");
  if (HIWORD (lParam) & KF_REPEAT)
    p += g_sprintf (p, "KF_REPEAT ");
  if (HIWORD (lParam) & KF_ALTDOWN)
    p += g_sprintf (p, "KF_ALTDOWN ");
  if (HIWORD (lParam) & KF_EXTENDED)
    p += g_sprintf (p, "KF_EXTENDED ");
  p += g_sprintf (p, "sc:%d rep:%d", LOBYTE (HIWORD (lParam)), LOWORD (lParam));

  return buf;
}

#endif

static void
fixup_event (BdkEvent *event)
{
  if (event->any.window)
    g_object_ref (event->any.window);
  if (((event->any.type == BDK_ENTER_NOTIFY) ||
       (event->any.type == BDK_LEAVE_NOTIFY)) &&
      (event->crossing.subwindow != NULL))
    g_object_ref (event->crossing.subwindow);
  event->any.send_event = InSendMessage (); 
}

void
_bdk_win32_append_event (BdkEvent *event)
{
  GList *link;
  
  fixup_event (event);
#if 1
  link = _bdk_event_queue_append (_bdk_display, event);
  BDK_NOTE (EVENTS, _bdk_win32_print_event (event));
  /* event morphing, the passed in may not be valid afterwards */
  _bdk_windowing_got_event (_bdk_display, link, event, 0);
#else
  _bdk_event_queue_append (_bdk_display, event);
  BDK_NOTE (EVENTS, _bdk_win32_print_event (event));
#endif
}

static void
fill_key_event_string (BdkEvent *event)
{
  gunichar c;
  bchar buf[256];

  /* Fill in event->string crudely, since various programs
   * depend on it.
   */
  
  c = 0;
  if (event->key.keyval != BDK_VoidSymbol)
    c = bdk_keyval_to_unicode (event->key.keyval);

  if (c)
    {
      bsize bytes_written;
      bint len;
      
      /* Apply the control key - Taken from Xlib
       */
      if (event->key.state & BDK_CONTROL_MASK)
	{
	  if ((c >= '@' && c < '\177') || c == ' ')
	    c &= 0x1F;
	  else if (c == '2')
	    {
	      event->key.string = g_memdup ("\0\0", 2);
	      event->key.length = 1;
	      return;
	    }
	  else if (c >= '3' && c <= '7')
	    c -= ('3' - '\033');
	  else if (c == '8')
	    c = '\177';
	  else if (c == '/')
	    c = '_' & 0x1F;
	}
      
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
}

static BdkFilterReturn
apply_event_filters (BdkWindow  *window,
		     MSG        *msg,
		     GList      **filters)
{
  BdkFilterReturn result = BDK_FILTER_CONTINUE;
  BdkEvent *event;
  GList *node;
  GList *tmp_list;

  event = bdk_event_new (BDK_NOTHING);
  if (window != NULL)
    event->any.window = g_object_ref (window);
  ((BdkEventPrivate *)event)->flags |= BDK_EVENT_PENDING;

  /* I think BdkFilterFunc semantics require the passed-in event
   * to already be in the queue. The filter func can generate
   * more events and append them after it if it likes.
   */
  node = _bdk_event_queue_append (_bdk_display, event);
  
  tmp_list = *filters;
  while (tmp_list)
    {
      BdkEventFilter *filter = (BdkEventFilter *) tmp_list->data;
      GList *node;

      if ((filter->flags & BDK_EVENT_FILTER_REMOVED) != 0)
        {
          tmp_list = tmp_list->next;
          continue;
        }

      filter->ref_count++;
      result = filter->function (msg, event, filter->data);

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
        break;
    }

  if (result == BDK_FILTER_CONTINUE || result == BDK_FILTER_REMOVE)
    {
      _bdk_event_queue_remove_link (_bdk_display, node);
      g_list_free_1 (node);
      bdk_event_free (event);
    }
  else /* BDK_FILTER_TRANSLATE */
    {
      ((BdkEventPrivate *)event)->flags &= ~BDK_EVENT_PENDING;
      fixup_event (event);
      BDK_NOTE (EVENTS, _bdk_win32_print_event (event));
    }
  return result;
}

/*
 * On Windows, transient windows will not have their own taskbar entries.
 * Because of this, we must hide and restore groups of transients in both
 * directions.  That is, all transient children must be hidden or restored
 * with this window, but if this window's transient owner also has a
 * transient owner then this window's transient owner must be hidden/restored
 * with this one.  And etc, up the chain until we hit an ancestor that has no
 * transient owner.
 *
 * It would be a good idea if applications don't chain transient windows
 * together.  There's a limit to how much evil BTK can try to shield you
 * from.
 */
static void
show_window_recurse (BdkWindow *window, bboolean hide_window)
{
  BdkWindowImplWin32 *impl = BDK_WINDOW_IMPL_WIN32 (BDK_WINDOW_OBJECT (window)->impl);
  GSList *children = impl->transient_children;
  BdkWindow *child = NULL;

  if (!impl->changing_state)
    {
      impl->changing_state = TRUE;

      if (children != NULL)
	{
	  while (children != NULL)
	    {
	      child = children->data;
	      show_window_recurse (child, hide_window);

	      children = b_slist_next (children);
	    }
	}

      if (BDK_WINDOW_IS_MAPPED (window))
	{
	  if (!hide_window)
	    {
	      if (BDK_WINDOW_OBJECT (window)->state & BDK_WINDOW_STATE_ICONIFIED)
		{
		  if (BDK_WINDOW_OBJECT (window)->state & BDK_WINDOW_STATE_MAXIMIZED)
		    {
		      ShowWindow (BDK_WINDOW_HWND (window), SW_SHOWMAXIMIZED);
		    }
		  else
		    {
		      ShowWindow (BDK_WINDOW_HWND (window), SW_RESTORE);
		    }
		}
	    }
	  else
	    {
	      ShowWindow (BDK_WINDOW_HWND (window), SW_MINIMIZE);
	    }
	}

      impl->changing_state = FALSE;
    }
}

static void
do_show_window (BdkWindow *window, bboolean hide_window)
{
  BdkWindow *tmp_window = NULL;
  BdkWindowImplWin32 *tmp_impl = BDK_WINDOW_IMPL_WIN32 (BDK_WINDOW_OBJECT (window)->impl);

  if (!tmp_impl->changing_state)
    {
      /* Find the top-level window in our transient chain. */
      while (tmp_impl->transient_owner != NULL)
	{
	  tmp_window = tmp_impl->transient_owner;
	  tmp_impl = BDK_WINDOW_IMPL_WIN32 (BDK_WINDOW_OBJECT (tmp_window)->impl);
	}

      /* If we couldn't find one, use the window provided. */
      if (tmp_window == NULL)
	{
	  tmp_window = window;
	}

      /* Recursively show/hide every window in the chain. */
      if (tmp_window != window)
	{
	  show_window_recurse (tmp_window, hide_window);
	}
    }
}

static void
send_crossing_event (BdkDisplay                 *display,
		     BdkWindowObject            *window,
		     BdkEventType                type,
		     BdkCrossingMode             mode,
		     BdkNotifyType               notify_type,
		     BdkWindow                  *subwindow,
		     POINT                      *screen_pt,
		     BdkModifierType             mask,
		     buint32                     time_)
{
  BdkEvent *event;
  BdkPointerGrabInfo *grab;
  POINT pt;

  grab = _bdk_display_has_pointer_grab (display, 0);

  if (grab != NULL &&
      !grab->owner_events &&
      mode != BDK_CROSSING_UNGRAB)
    {
      /* !owner_event => only report events wrt grab window, ignore rest */
      if ((BdkWindow *)window != grab->native_window)
	return;
    }

  pt = *screen_pt;
  ScreenToClient (BDK_WINDOW_HWND (window), &pt);
  
  event = bdk_event_new (type);
  event->crossing.window = (BdkWindow *)window;
  event->crossing.subwindow = subwindow;
  event->crossing.time = _bdk_win32_get_next_tick (time_);
  event->crossing.x = pt.x;
  event->crossing.y = pt.y;
  event->crossing.x_root = screen_pt->x + _bdk_offset_x;
  event->crossing.y_root = screen_pt->y + _bdk_offset_y;
  event->crossing.mode = mode;
  event->crossing.detail = notify_type;
  event->crossing.mode = mode;
  event->crossing.detail = notify_type;
  event->crossing.focus = FALSE;
  event->crossing.state = mask;

  _bdk_win32_append_event (event);
  
  /*
  if (((BdkWindowObject *) window)->extension_events != 0)
    _bdk_input_crossing_event ((BdkWindow *)window, type == BDK_ENTER_NOTIFY);
  */
}

static BdkWindowObject *
get_native_parent (BdkWindowObject *window)
{
  if (window->parent != NULL)
    return window->parent->impl_window;
  return NULL;
}


static BdkWindowObject *
find_common_ancestor (BdkWindowObject *win1,
		      BdkWindowObject *win2)
{
  BdkWindowObject *tmp;
  GList *path1 = NULL, *path2 = NULL;
  GList *list1, *list2;

  tmp = win1;
  while (tmp != NULL && tmp->window_type != BDK_WINDOW_ROOT)
    {
      path1 = g_list_prepend (path1, tmp);
      tmp = get_native_parent (tmp);
    }

  tmp = win2;
  while (tmp != NULL && tmp->window_type != BDK_WINDOW_ROOT)
    {
      path2 = g_list_prepend (path2, tmp);
      tmp = get_native_parent (tmp);
    }

  list1 = path1;
  list2 = path2;
  tmp = NULL;
  while (list1 && list2 && (list1->data == list2->data))
    {
      tmp = (BdkWindowObject *)list1->data;
      list1 = g_list_next (list1);
      list2 = g_list_next (list2);
    }
  g_list_free (path1);
  g_list_free (path2);

  return tmp;
}

void
synthesize_crossing_events (BdkDisplay                 *display,
			    BdkWindow                  *src,
			    BdkWindow                  *dest,
			    BdkCrossingMode             mode,
			    POINT                      *screen_pt,
			    BdkModifierType             mask,
			    buint32                     time_,
			    bboolean                    non_linear)
{
  BdkWindowObject *c;
  BdkWindowObject *win, *last, *next;
  GList *path, *list;
  BdkWindowObject *a;
  BdkWindowObject *b;
  BdkNotifyType notify_type;

  a = (BdkWindowObject *)src;
  b = (BdkWindowObject *)dest;
  if (a == b)
    return; /* No crossings generated between src and dest */

  c = find_common_ancestor (a, b);

  non_linear |= (c != a) && (c != b);

  if (a) /* There might not be a source (i.e. if no previous pointer_in_window) */
    {
      /* Traverse up from a to (excluding) c sending leave events */
      if (non_linear)
	notify_type = BDK_NOTIFY_NONLINEAR;
      else if (c == a)
	notify_type = BDK_NOTIFY_INFERIOR;
      else
	notify_type = BDK_NOTIFY_ANCESTOR;
      send_crossing_event (display,
			   a, BDK_LEAVE_NOTIFY,
			   mode,
			   notify_type,
			   NULL,
			   screen_pt,
			   mask, time_);

      if (c != a)
	{
	  if (non_linear)
	    notify_type = BDK_NOTIFY_NONLINEAR_VIRTUAL;
	  else
	    notify_type = BDK_NOTIFY_VIRTUAL;

	  last = a;
	  win = get_native_parent (a);
	  while (win != c && win->window_type != BDK_WINDOW_ROOT)
	    {
	      send_crossing_event (display,
				   win, BDK_LEAVE_NOTIFY,
				   mode,
				   notify_type,
				   (BdkWindow *)last,
				   screen_pt,
				   mask, time_);

	      last = win;
	      win = get_native_parent (win);
	    }
	}
    }

  if (b) /* Might not be a dest, e.g. if we're moving out of the window */
    {
      /* Traverse down from c to b */
      if (c != b)
	{
	  path = NULL;
	  win = get_native_parent (b);
	  while (win != c && win->window_type != BDK_WINDOW_ROOT)
	    {
	      path = g_list_prepend (path, win);
	      win = get_native_parent (win);
	    }

	  if (non_linear)
	    notify_type = BDK_NOTIFY_NONLINEAR_VIRTUAL;
	  else
	    notify_type = BDK_NOTIFY_VIRTUAL;

	  list = path;
	  while (list)
	    {
	      win = (BdkWindowObject *)list->data;
	      list = g_list_next (list);
	      if (list)
		next = (BdkWindowObject *)list->data;
	      else
		next = b;

	      send_crossing_event (display,
				   win, BDK_ENTER_NOTIFY,
				   mode,
				   notify_type,
				   (BdkWindow *)next,
				   screen_pt,
				   mask, time_);
	    }
	  g_list_free (path);
	}


      if (non_linear)
	notify_type = BDK_NOTIFY_NONLINEAR;
      else if (c == a)
	notify_type = BDK_NOTIFY_ANCESTOR;
      else
	notify_type = BDK_NOTIFY_INFERIOR;

      send_crossing_event (display,
			   b, BDK_ENTER_NOTIFY,
			   mode,
			   notify_type,
			   NULL,
			   screen_pt,
			   mask, time_);
    }
}
			 
static void
synthesize_expose_events (BdkWindow *window)
{
  RECT r;
  HDC hdc;
  BdkDrawableImplWin32 *impl = BDK_DRAWABLE_IMPL_WIN32 (((BdkWindowObject *) window)->impl);
  GList *list = bdk_window_get_children (window);
  GList *head = list;
  BdkEvent *event;
  int k;
  
  while (list)
    {
      synthesize_expose_events ((BdkWindow *) list->data);
      list = list->next;
    }

  g_list_free (head);

  if (((BdkWindowObject *) window)->input_only)
    ;
  else if (!(hdc = GetDC (impl->handle)))
    WIN32_GDI_FAILED ("GetDC");
  else
    {
      if ((k = GetClipBox (hdc, &r)) == ERROR)
	WIN32_GDI_FAILED ("GetClipBox");
      else if (k != NULLREBUNNYION)
	{
	  event = bdk_event_new (BDK_EXPOSE);
	  event->expose.window = window;
	  event->expose.area.x = r.left;
	  event->expose.area.y = r.top;
	  event->expose.area.width = r.right - r.left;
	  event->expose.area.height = r.bottom - r.top;
	  event->expose.rebunnyion = bdk_rebunnyion_rectangle (&(event->expose.area));
	  event->expose.count = 0;
  
	  _bdk_win32_append_event (event);
	}
      GDI_CALL (ReleaseDC, (impl->handle, hdc));
    }
}

static void
update_colors (BdkWindow *window,
	       bboolean   top)
{
  HDC hdc;
  BdkDrawableImplWin32 *impl = BDK_DRAWABLE_IMPL_WIN32 (((BdkWindowObject *) window)->impl);
  GList *list = bdk_window_get_children (window);
  GList *head = list;

  BDK_NOTE (COLORMAP, (top ? g_print ("update_colors:") : (void) 0));

  while (list)
    {
      update_colors ((BdkWindow *) list->data, FALSE);
      list = list->next;
    }
  g_list_free (head);

  if (((BdkWindowObject *) window)->input_only ||
      impl->colormap == NULL)
    return;

  if (!(hdc = GetDC (impl->handle)))
    WIN32_GDI_FAILED ("GetDC");
  else
    {
      BdkColormapPrivateWin32 *cmapp = BDK_WIN32_COLORMAP_DATA (impl->colormap);
      HPALETTE holdpal;
      bint k;
      
      if ((holdpal = SelectPalette (hdc, cmapp->hpal, TRUE)) == NULL)
	WIN32_GDI_FAILED ("SelectPalette");
      else if ((k = RealizePalette (hdc)) == GDI_ERROR)
	WIN32_GDI_FAILED ("RealizePalette");
      else
	{
	  BDK_NOTE (COLORMAP,
		    (k > 0 ?
		     g_print (" %p pal=%p: realized %d colors\n"
			      "update_colors:",
			      impl->handle, cmapp->hpal, k) :
		     (void) 0,
		     g_print (" %p", impl->handle)));
	  GDI_CALL (UpdateColors, (hdc));
	  SelectPalette (hdc, holdpal, TRUE);
	  RealizePalette (hdc);
	}
      GDI_CALL (ReleaseDC, (impl->handle, hdc));
    }
  BDK_NOTE (COLORMAP, (top ? g_print ("\n") : (void) 0));
}

/* The check_extended flag controls whether to check if the windows want
 * events from extended input devices and if the message should be skipped
 * because an extended input device is active
 */
static bboolean
propagate (BdkWindow  **window,
	   MSG         *msg,
	   BdkWindow   *grab_window,
	   bboolean     grab_owner_events,
	   bint	        grab_mask,
	   bboolean   (*doesnt_want_it) (bint mask,
					 MSG *msg))
{
  if (grab_window != NULL && !grab_owner_events)
    {
      /* Event source is grabbed with owner_events FALSE */

      /* See if the event should be ignored because an extended input
       * device is used
       */
      if ((*doesnt_want_it) (grab_mask, msg))
	{
	  BDK_NOTE (EVENTS, g_print (" (grabber doesn't want it)"));
	  return FALSE;
	}
      else
	{
	  BDK_NOTE (EVENTS, g_print (" (to grabber)"));
	  assign_object (window, grab_window);
	  return TRUE;
	}
    }

  /* If we come here, we know that if grab_window != NULL then
   * grab_owner_events is TRUE
   */
  while (TRUE)
    {
      if ((*doesnt_want_it) (((BdkWindowObject *) *window)->event_mask, msg))
	{
	  /* Owner doesn't want it, propagate to parent. */
	  BdkWindow *parent = bdk_window_get_parent (*window);
	  if (parent == _bdk_root || parent == NULL)
	    {
	      /* No parent; check if grabbed */
	      if (grab_window != NULL)
		{
		  /* Event source is grabbed with owner_events TRUE */

		  if ((*doesnt_want_it) (grab_mask, msg))
		    {
		      /* Grabber doesn't want it either */
		      BDK_NOTE (EVENTS, g_print (" (grabber doesn't want it)"));
		      return FALSE;
		    }
		  else
		    {
		      /* Grabbed! */
		      BDK_NOTE (EVENTS, g_print (" (to grabber)"));
		      assign_object (window, grab_window);
		      return TRUE;
		    }
		}
	      else
		{
		  BDK_NOTE (EVENTS, g_print (" (undelivered)"));
		  return FALSE;
		}
	    }
	  else
	    {
	      assign_object (window, parent);
	      /* The only branch where we actually continue the loop */
	    }
	}
      else
	return TRUE;
    }
}

static bboolean
doesnt_want_key (bint mask,
		 MSG *msg)
{
  return (((msg->message == WM_KEYUP || msg->message == WM_SYSKEYUP) &&
	   !(mask & BDK_KEY_RELEASE_MASK)) ||
	  ((msg->message == WM_KEYDOWN || msg->message == WM_SYSKEYDOWN) &&
	   !(mask & BDK_KEY_PRESS_MASK)));
}

static bboolean
doesnt_want_char (bint mask,
		  MSG *msg)
{
  return !(mask & (BDK_KEY_PRESS_MASK | BDK_KEY_RELEASE_MASK));
}

void
_bdk_win32_emit_configure_event (BdkWindow *window)
{
  BdkWindowImplWin32 *window_impl;
  RECT client_rect;
  POINT point;
  BdkWindowObject *window_object;
  HWND hwnd;

  window_object = BDK_WINDOW_OBJECT (window);

  window_impl = BDK_WINDOW_IMPL_WIN32 (window_object->impl);
  if (window_impl->inhibit_configure)
    return;

  hwnd = BDK_WINDOW_HWND (window);

  GetClientRect (hwnd, &client_rect);
  point.x = client_rect.left; /* always 0 */
  point.y = client_rect.top;

  /* top level windows need screen coords */
  if (bdk_window_get_parent (window) == _bdk_root)
    {
      ClientToScreen (hwnd, &point);
      point.x += _bdk_offset_x;
      point.y += _bdk_offset_y;
    }

  window_object->width = client_rect.right - client_rect.left;
  window_object->height = client_rect.bottom - client_rect.top;
  
  window_object->x = point.x;
  window_object->y = point.y;

  _bdk_window_update_size (window);
  
  if (window_object->event_mask & BDK_STRUCTURE_MASK)
    {
      BdkEvent *event = bdk_event_new (BDK_CONFIGURE);

      event->configure.window = window;

      event->configure.width = client_rect.right - client_rect.left;
      event->configure.height = client_rect.bottom - client_rect.top;
      
      event->configure.x = point.x;
      event->configure.y = point.y;

      _bdk_win32_append_event (event);
    }
}

BdkRebunnyion *
_bdk_win32_hrgn_to_rebunnyion (HRGN hrgn)
{
  RGNDATA *rgndata;
  RECT *rects;
  BdkRebunnyion *result;
  bint nbytes;
  buint i;

  if ((nbytes = GetRebunnyionData (hrgn, 0, NULL)) == 0)
    {
      WIN32_GDI_FAILED ("GetRebunnyionData");
      return NULL;
    }

  rgndata = (RGNDATA *) g_malloc (nbytes);

  if (GetRebunnyionData (hrgn, nbytes, rgndata) == 0)
    {
      WIN32_GDI_FAILED ("GetRebunnyionData");
      g_free (rgndata);
      return NULL;
    }

  result = bdk_rebunnyion_new ();
  rects = (RECT *) rgndata->Buffer;
  for (i = 0; i < rgndata->rdh.nCount; i++)
    {
      BdkRectangle r;

      r.x = rects[i].left;
      r.y = rects[i].top;
      r.width = rects[i].right - r.x;
      r.height = rects[i].bottom - r.y;

      bdk_rebunnyion_union_with_rect (result, &r);
    }

  g_free (rgndata);

  return result;
}

static void
adjust_drag (LONG *drag,
	     LONG  curr,
	     bint  inc)
{
  if (*drag > curr)
    *drag = curr + ((*drag + inc/2 - curr) / inc) * inc;
  else
    *drag = curr - ((curr - *drag + inc/2) / inc) * inc;
}

static void
handle_wm_paint (MSG        *msg,
		 BdkWindow  *window,
		 bboolean    return_exposes,
		 BdkEvent  **event)
{
  HRGN hrgn = CreateRectRgn (0, 0, 0, 0);
  HDC hdc;
  PAINTSTRUCT paintstruct;
  BdkRebunnyion *update_rebunnyion;

  if (GetUpdateRgn (msg->hwnd, hrgn, FALSE) == ERROR)
    {
      WIN32_GDI_FAILED ("GetUpdateRgn");
      DeleteObject (hrgn);
      return;
    }

  hdc = BeginPaint (msg->hwnd, &paintstruct);

  BDK_NOTE (EVENTS, g_print (" %s %s dc %p%s",
			     _bdk_win32_rect_to_string (&paintstruct.rcPaint),
			     (paintstruct.fErase ? "erase" : ""),
			     hdc,
			     (return_exposes ? " return_exposes" : "")));

  EndPaint (msg->hwnd, &paintstruct);

  if ((paintstruct.rcPaint.right == paintstruct.rcPaint.left) ||
      (paintstruct.rcPaint.bottom == paintstruct.rcPaint.top))
    {
      BDK_NOTE (EVENTS, g_print (" (empty paintstruct, ignored)"));
      DeleteObject (hrgn);
      return;
    }

  if (return_exposes)
    {
      if (!BDK_WINDOW_DESTROYED (window))
	{
	  GList *list = _bdk_display->queued_events;

	  *event = bdk_event_new (BDK_EXPOSE);
	  (*event)->expose.window = window;
	  (*event)->expose.area.x = paintstruct.rcPaint.left;
	  (*event)->expose.area.y = paintstruct.rcPaint.top;
	  (*event)->expose.area.width = paintstruct.rcPaint.right - paintstruct.rcPaint.left;
	  (*event)->expose.area.height = paintstruct.rcPaint.bottom - paintstruct.rcPaint.top;
	  (*event)->expose.rebunnyion = _bdk_win32_hrgn_to_rebunnyion (hrgn);
	  (*event)->expose.count = 0;

	  while (list != NULL)
	    {
	      BdkEventPrivate *evp = list->data;

	      if (evp->event.any.type == BDK_EXPOSE &&
		  evp->event.any.window == window &&
		  !(evp->flags & BDK_EVENT_PENDING))
		evp->event.expose.count++;

	      list = list->next;
	    }
	}

      DeleteObject (hrgn);
      return;
    }

  update_rebunnyion = _bdk_win32_hrgn_to_rebunnyion (hrgn);
  if (!bdk_rebunnyion_empty (update_rebunnyion))
    _bdk_window_invalidate_for_expose (window, update_rebunnyion);
  bdk_rebunnyion_destroy (update_rebunnyion);

  DeleteObject (hrgn);
}

static VOID CALLBACK 
modal_timer_proc (HWND     hwnd,
		  UINT     msg,
		  UINT_PTR id,
		  DWORD    time)
{
  int arbitrary_limit = 10;

  while (_modal_operation_in_progress &&
	 g_main_context_pending (NULL) &&
	 arbitrary_limit--)
    g_main_context_iteration (NULL, FALSE);
}

void
_bdk_win32_begin_modal_call (void)
{
  g_assert (!_modal_operation_in_progress);

  _modal_operation_in_progress = TRUE;

  modal_timer = SetTimer (NULL, 0, 10, modal_timer_proc);
  if (modal_timer == 0)
    WIN32_API_FAILED ("SetTimer");
}

void
_bdk_win32_end_modal_call (void)
{
  g_assert (_modal_operation_in_progress);

  _modal_operation_in_progress = FALSE;

  if (modal_timer != 0)
    {
      API_CALL (KillTimer, (NULL, modal_timer));
      modal_timer = 0;
   }
}

static VOID CALLBACK
sync_timer_proc (HWND     hwnd,
		 UINT     msg,
		 UINT_PTR id,
		 DWORD    time)
{
  MSG message;
  if (PeekMessageW (&message, hwnd, WM_PAINT, WM_PAINT, PM_REMOVE))
    {
      return;
    }

  RedrawWindow (hwnd, NULL, NULL, RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);

  KillTimer (hwnd, sync_timer);
}

static void
handle_display_change (void)
{
  _bdk_monitor_init ();
  _bdk_root_window_size_init ();
  g_signal_emit_by_name (_bdk_screen, "size_changed");
}

static void
generate_button_event (BdkEventType type,
		       bint         button,
		       BdkWindow   *window,
		       MSG         *msg)
{
  BdkEvent *event = bdk_event_new (type);

  event->button.window = window;
  event->button.time = _bdk_win32_get_next_tick (msg->time);
  event->button.x = current_x = (bint16) GET_X_LPARAM (msg->lParam);
  event->button.y = current_y = (bint16) GET_Y_LPARAM (msg->lParam);
  event->button.x_root = msg->pt.x + _bdk_offset_x;
  event->button.y_root = msg->pt.y + _bdk_offset_y;
  event->button.axes = NULL;
  event->button.state = build_pointer_event_state (msg);
  event->button.button = button;
  event->button.device = _bdk_display->core_pointer;

  _bdk_win32_append_event (event);
}

static void
ensure_stacking_on_unminimize (MSG *msg)
{
  HWND rover;
  HWND lowest_transient = NULL;

  rover = msg->hwnd;
  while ((rover = GetNextWindow (rover, GW_HWNDNEXT)))
    {
      BdkWindow *rover_bdkw = bdk_win32_handle_table_lookup (rover);

      /* Checking window group not implemented yet */
      if (rover_bdkw)
	{
	  BdkWindowImplWin32 *rover_impl =
	    (BdkWindowImplWin32 *)((BdkWindowObject *)rover_bdkw)->impl;

	  if (BDK_WINDOW_IS_MAPPED (rover_bdkw) &&
	      (rover_impl->type_hint == BDK_WINDOW_TYPE_HINT_UTILITY ||
	       rover_impl->type_hint == BDK_WINDOW_TYPE_HINT_DIALOG ||
	       rover_impl->transient_owner != NULL))
	    {
	      lowest_transient = rover;
	    }
	}
    }
  if (lowest_transient != NULL)
    {
      BDK_NOTE (EVENTS, g_print (" restacking: %p", lowest_transient));
      SetWindowPos (msg->hwnd, lowest_transient, 0, 0, 0, 0,
		    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
    }
}

static bboolean
ensure_stacking_on_window_pos_changing (MSG       *msg,
					BdkWindow *window)
{
  BdkWindowImplWin32 *impl = (BdkWindowImplWin32 *)((BdkWindowObject *) window)->impl;
  WINDOWPOS *windowpos = (WINDOWPOS *) msg->lParam;

  if (GetActiveWindow () == msg->hwnd &&
      impl->type_hint != BDK_WINDOW_TYPE_HINT_UTILITY &&
      impl->type_hint != BDK_WINDOW_TYPE_HINT_DIALOG &&
      impl->transient_owner == NULL)
    {
      /* Make sure the window stays behind any transient-type windows
       * of the same window group.
       *
       * If the window is not active and being activated, we let
       * Windows bring it to the top and rely on the WM_ACTIVATEAPP
       * handling to bring any utility windows on top of it.
       */
      HWND rover;
      bboolean restacking;

      rover = windowpos->hwndInsertAfter;
      restacking = FALSE;
      while (rover)
	{
	  BdkWindow *rover_bdkw = bdk_win32_handle_table_lookup (rover);

	  /* Checking window group not implemented yet */
	  if (rover_bdkw)
	    {
	      BdkWindowImplWin32 *rover_impl =
		(BdkWindowImplWin32 *)((BdkWindowObject *)rover_bdkw)->impl;

	      if (BDK_WINDOW_IS_MAPPED (rover_bdkw) &&
		  (rover_impl->type_hint == BDK_WINDOW_TYPE_HINT_UTILITY ||
		   rover_impl->type_hint == BDK_WINDOW_TYPE_HINT_DIALOG ||
		   rover_impl->transient_owner != NULL))
		{
		  restacking = TRUE;
		  windowpos->hwndInsertAfter = rover;
		}
	    }
	  rover = GetNextWindow (rover, GW_HWNDNEXT);
	}

      if (restacking)
	{
	  BDK_NOTE (EVENTS, g_print (" restacking: %p", windowpos->hwndInsertAfter));
	  return TRUE;
	}
    }
  return FALSE;
}

static void
ensure_stacking_on_activate_app (MSG       *msg,
				 BdkWindow *window)
{
  BdkWindowImplWin32 *impl = (BdkWindowImplWin32 *)((BdkWindowObject *) window)->impl;

  if (impl->type_hint == BDK_WINDOW_TYPE_HINT_UTILITY ||
      impl->type_hint == BDK_WINDOW_TYPE_HINT_DIALOG ||
      impl->transient_owner != NULL)
    {
      SetWindowPos (msg->hwnd, HWND_TOP, 0, 0, 0, 0,
		    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
      return;
    }

  if (IsWindowVisible (msg->hwnd) &&
      msg->hwnd == GetActiveWindow ())
    {
      /* This window is not a transient-type window and it is the
       * activated window. Make sure this window is as visible as
       * possible, just below the lowest transient-type window of this
       * app.
       */
      HWND rover;

      rover = msg->hwnd;
      while ((rover = GetNextWindow (rover, GW_HWNDPREV)))
	{
	  BdkWindow *rover_bdkw = bdk_win32_handle_table_lookup (rover);

	  /* Checking window group not implemented yet */
	  if (rover_bdkw)
	    {
	      BdkWindowImplWin32 *rover_impl =
		(BdkWindowImplWin32 *)((BdkWindowObject *)rover_bdkw)->impl;

	      if (BDK_WINDOW_IS_MAPPED (rover_bdkw) &&
		  (rover_impl->type_hint == BDK_WINDOW_TYPE_HINT_UTILITY ||
		   rover_impl->type_hint == BDK_WINDOW_TYPE_HINT_DIALOG ||
		   rover_impl->transient_owner != NULL))
		{
		  BDK_NOTE (EVENTS, g_print (" restacking: %p", rover));
		  SetWindowPos (msg->hwnd, rover, 0, 0, 0, 0,
				SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
		  break;
		}
	    }
	}
    }
}

#define BDK_ANY_BUTTON_MASK (BDK_BUTTON1_MASK | \
			     BDK_BUTTON2_MASK | \
			     BDK_BUTTON3_MASK | \
			     BDK_BUTTON4_MASK | \
			     BDK_BUTTON5_MASK)

static bboolean
bdk_event_translate (MSG  *msg,
		     bint *ret_valp)
{
  RECT rect, *drag, orig_drag;
  POINT point;
  MINMAXINFO *mmi;
  HWND hwnd;
  HCURSOR hcursor;
  BYTE key_state[256];
  HIMC himc;
  WINDOWPOS *windowpos;
  bboolean ignore_leave;

  BdkEvent *event;

  wchar_t wbuf[100];
  bint ccount;

  BdkWindow *window = NULL;
  BdkWindowImplWin32 *impl;

  BdkWindow *orig_window, *new_window;

  BdkPointerGrabInfo *grab = NULL;
  BdkWindow *grab_window = NULL;

  static bint update_colors_counter = 0;
  bint button;
  BdkAtom target;

  bchar buf[256];
  bboolean return_val = FALSE;

  int i;

  if (_bdk_default_filters)
    {
      /* Apply global filters */

      BdkFilterReturn result = apply_event_filters (NULL, msg, &_bdk_default_filters);
      
      /* If result is BDK_FILTER_CONTINUE, we continue as if nothing
       * happened. If it is BDK_FILTER_REMOVE or BDK_FILTER_TRANSLATE,
       * we return TRUE, and DefWindowProcW() will not be called.
       */
      if (result == BDK_FILTER_REMOVE || result == BDK_FILTER_TRANSLATE)
	return TRUE;
    }

  window = bdk_win32_handle_table_lookup ((BdkNativeWindow) msg->hwnd);
  orig_window = window;

  if (window == NULL)
    {
      /* XXX Handle WM_QUIT here ? */
      if (msg->message == WM_QUIT)
	{
	  BDK_NOTE (EVENTS, g_print (" %d", (int) msg->wParam));
	  exit (msg->wParam);
	}
      else if (msg->message == WM_CREATE)
	{
	  window = (UNALIGNED BdkWindow*) (((LPCREATESTRUCTW) msg->lParam)->lpCreateParams);
	  BDK_WINDOW_HWND (window) = msg->hwnd;
	}
      else
	{
	  BDK_NOTE (EVENTS, g_print (" (no BdkWindow)"));
	}
      return FALSE;
    }
  
  g_object_ref (window);

  /* window's refcount has now been increased, so code below should
   * not just return from this function, but instead goto done (or
   * break out of the big switch). To protect against forgetting this,
   * #define return to a syntax error...
   */
#define return GOTO_DONE_INSTEAD
  
  if (!BDK_WINDOW_DESTROYED (window) && ((BdkWindowObject *) window)->filters)
    {
      /* Apply per-window filters */

      BdkFilterReturn result = apply_event_filters (window, msg, &((BdkWindowObject *) window)->filters);

      if (result == BDK_FILTER_REMOVE || result == BDK_FILTER_TRANSLATE)
	{
	  return_val = TRUE;
	  goto done;
	}
    }

  if (msg->message == client_message)
    {
      GList *tmp_list;
      BdkFilterReturn result = BDK_FILTER_CONTINUE;
      GList *node;

      BDK_NOTE (EVENTS, g_print (" client_message"));

      event = bdk_event_new (BDK_NOTHING);
      ((BdkEventPrivate *)event)->flags |= BDK_EVENT_PENDING;

      node = _bdk_event_queue_append (_bdk_display, event);

      tmp_list = client_filters;
      while (tmp_list)
	{
	  BdkClientFilter *filter = tmp_list->data;

	  tmp_list = tmp_list->next;

	  if (filter->type == BDK_POINTER_TO_ATOM (msg->wParam))
	    {
	      BDK_NOTE (EVENTS, g_print (" (match)"));

	      result = (*filter->function) (msg, event, filter->data);

	      if (result != BDK_FILTER_CONTINUE)
		break;
	    }
	}

      switch (result)
	{
	case BDK_FILTER_REMOVE:
	  _bdk_event_queue_remove_link (_bdk_display, node);
	  g_list_free_1 (node);
	  bdk_event_free (event);
	  return_val = TRUE;
	  goto done;

	case BDK_FILTER_TRANSLATE:
	  ((BdkEventPrivate *)event)->flags &= ~BDK_EVENT_PENDING;
	  BDK_NOTE (EVENTS, _bdk_win32_print_event (event));
	  return_val = TRUE;
	  goto done;

	case BDK_FILTER_CONTINUE:
	  /* Send unknown client messages on to Btk for it to use */

	  event->client.type = BDK_CLIENT_EVENT;
	  event->client.window = window;
	  event->client.message_type = BDK_POINTER_TO_ATOM (msg->wParam);
	  event->client.data_format = 32;
	  event->client.data.l[0] = msg->lParam;
	  for (i = 1; i < 5; i++)
	    event->client.data.l[i] = 0;
	  BDK_NOTE (EVENTS, _bdk_win32_print_event (event));
	  return_val = TRUE;
	  goto done;
	}
    }

  switch (msg->message)
    {
    case WM_INPUTLANGCHANGE:
      _bdk_input_locale = (HKL) msg->lParam;
      _bdk_win32_keymap_set_active_layout (BDK_WIN32_KEYMAP (bdk_keymap_get_default ()), _bdk_input_locale);
      _bdk_input_locale_is_ime = ImmIsIME (_bdk_input_locale);
      GetLocaleInfo (MAKELCID (LOWORD (_bdk_input_locale), SORT_DEFAULT),
		     LOCALE_IDEFAULTANSICODEPAGE,
		     buf, sizeof (buf));
      _bdk_input_codepage = atoi (buf);
      _bdk_keymap_serial++;
      BDK_NOTE (EVENTS,
		g_print (" cs:%lu hkl:%p%s cp:%d",
			 (bulong) msg->wParam,
			 (bpointer) msg->lParam, _bdk_input_locale_is_ime ? " (IME)" : "",
			 _bdk_input_codepage));
      break;

    case WM_SYSKEYUP:
    case WM_SYSKEYDOWN:
      BDK_NOTE (EVENTS,
		g_print (" %s ch:%.02x %s",
			 _bdk_win32_key_to_string (msg->lParam),
			 (int) msg->wParam,
			 decode_key_lparam (msg->lParam)));

      /* If posted without us having keyboard focus, ignore */
      if ((msg->wParam != VK_F10 && msg->wParam != VK_MENU) &&
	  !(HIWORD (msg->lParam) & KF_ALTDOWN))
	break;

      /* Let the system handle Alt-Tab, Alt-Space and Alt-F4 unless
       * the keyboard is grabbed.
       */
      if (_bdk_display->keyboard_grab.window == NULL &&
	  (msg->wParam == VK_TAB ||
	   msg->wParam == VK_SPACE ||
	   msg->wParam == VK_F4))
	break;

      /* Jump to code in common with WM_KEYUP and WM_KEYDOWN */
      goto keyup_or_down;

    case WM_KEYUP:
    case WM_KEYDOWN:
      BDK_NOTE (EVENTS, 
		g_print (" %s ch:%.02x %s",
			 _bdk_win32_key_to_string (msg->lParam),
			 (int) msg->wParam,
			 decode_key_lparam (msg->lParam)));

    keyup_or_down:

      /* Ignore key messages intended for the IME */
      if (msg->wParam == VK_PROCESSKEY ||
	  in_ime_composition)
	break;

      /* Ignore autorepeats on modifiers */
      if (msg->message == WM_KEYDOWN &&
          (msg->wParam == VK_MENU ||
           msg->wParam == VK_CONTROL ||
           msg->wParam == VK_SHIFT) &&
           ((HIWORD(msg->lParam) & KF_REPEAT) >= 1))
        break;

      if (!propagate (&window, msg,
		      _bdk_display->keyboard_grab.window,
		      _bdk_display->keyboard_grab.owner_events,
		      BDK_ALL_EVENTS_MASK,
		      doesnt_want_key))
	break;

      if (BDK_WINDOW_DESTROYED (window))
	break;

      impl = BDK_WINDOW_IMPL_WIN32 (BDK_WINDOW_OBJECT (window)->impl);

      API_CALL (GetKeyboardState, (key_state));

      ccount = 0;

      if (msg->wParam == VK_PACKET)
	{
	  ccount = ToUnicode (VK_PACKET, HIWORD (msg->lParam), key_state, wbuf, 1, 0);
	  if (ccount == 1)
	    {
	      if (wbuf[0] >= 0xD800 && wbuf[0] < 0xDC00)
	        {
		  if (msg->message == WM_KEYDOWN)
		    impl->leading_surrogate_keydown = wbuf[0];
		  else
		    impl->leading_surrogate_keyup = wbuf[0];

		  /* don't emit an event */
		  return_val = TRUE;
		  break;
	        }
	      else
		{
		  /* wait until an event is created */;
		}
	    }
	}

      event = bdk_event_new ((msg->message == WM_KEYDOWN ||
			      msg->message == WM_SYSKEYDOWN) ?
			     BDK_KEY_PRESS : BDK_KEY_RELEASE);
      event->key.window = window;
      event->key.time = _bdk_win32_get_next_tick (msg->time);
      event->key.keyval = BDK_VoidSymbol;
      event->key.string = NULL;
      event->key.length = 0;
      event->key.hardware_keycode = msg->wParam;
      if (HIWORD (msg->lParam) & KF_EXTENDED)
	{
	  switch (msg->wParam)
	    {
	    case VK_CONTROL:
	      event->key.hardware_keycode = VK_RCONTROL;
	      break;
	    case VK_SHIFT:	/* Actually, KF_EXTENDED is not set
				 * for the right shift key.
				 */
	      event->key.hardware_keycode = VK_RSHIFT;
	      break;
	    case VK_MENU:
	      event->key.hardware_keycode = VK_RMENU;
	      break;
	    }
	}
      else if (msg->wParam == VK_SHIFT &&
	       LOBYTE (HIWORD (msg->lParam)) == _bdk_win32_keymap_get_rshift_scancode (BDK_WIN32_KEYMAP (bdk_keymap_get_default ())))
	event->key.hardware_keycode = VK_RSHIFT;

      /* g_print ("ctrl:%02x lctrl:%02x rctrl:%02x alt:%02x lalt:%02x ralt:%02x\n", key_state[VK_CONTROL], key_state[VK_LCONTROL], key_state[VK_RCONTROL], key_state[VK_MENU], key_state[VK_LMENU], key_state[VK_RMENU]); */
      
      build_key_event_state (event, key_state);

      if (msg->wParam == VK_PACKET && ccount == 1)
	{
	  if (wbuf[0] >= 0xD800 && wbuf[0] < 0xDC00)
	    {
	      g_assert_not_reached ();
	    }
	  else if (wbuf[0] >= 0xDC00 && wbuf[0] < 0xE000)
	    {
	      wchar_t leading;

              if (msg->message == WM_KEYDOWN)
		leading = impl->leading_surrogate_keydown;
	      else
		leading = impl->leading_surrogate_keyup;

	      event->key.keyval = bdk_unicode_to_keyval ((leading - 0xD800) * 0x400 + wbuf[0] - 0xDC00 + 0x10000);
	    }
	  else
	    {
	      event->key.keyval = bdk_unicode_to_keyval (wbuf[0]);
	    }
	}
      else
	{
	  bdk_keymap_translate_keyboard_state (bdk_keymap_get_for_display (_bdk_display),
					       event->key.hardware_keycode,
					       event->key.state,
					       event->key.group,
					       &event->key.keyval,
					       NULL, NULL, NULL);
	}

      if (msg->message == WM_KEYDOWN)
	impl->leading_surrogate_keydown = 0;
      else
	impl->leading_surrogate_keyup = 0;

      fill_key_event_string (event);

      /* Reset MOD1_MASK if it is the Alt key itself */
      if (msg->wParam == VK_MENU)
	event->key.state &= ~BDK_MOD1_MASK;

      _bdk_win32_append_event (event);

      return_val = TRUE;
      break;

    case WM_SYSCHAR:
      if (msg->wParam != VK_SPACE)
	{
	  /* To prevent beeps, don't let DefWindowProcW() be called */
	  return_val = TRUE;
	  goto done;
	}
      break;

    case WM_IME_STARTCOMPOSITION:
      in_ime_composition = TRUE;
      break;

    case WM_IME_ENDCOMPOSITION:
      in_ime_composition = FALSE;
      break;

    case WM_IME_COMPOSITION:
      /* On Win2k WM_IME_CHAR doesn't work correctly for non-Unicode
       * applications. Thus, handle WM_IME_COMPOSITION with
       * GCS_RESULTSTR instead, fetch the Unicode chars from the IME
       * with ImmGetCompositionStringW().
       *
       * See for instance
       * http://groups.google.com/groups?selm=natX5.57%24g77.19788%40nntp2.onemain.com
       * and
       * http://groups.google.com/groups?selm=u2XfrXw5BHA.1628%40tkmsftngp02
       * for comments by other people that seems to have the same
       * experience. WM_IME_CHAR just gives question marks, apparently
       * because of going through some conversion to the current code
       * page.
       *
       * WM_IME_CHAR might work on NT4 or Win9x with ActiveIMM, but
       * use WM_IME_COMPOSITION there, too, to simplify the code.
       */
      BDK_NOTE (EVENTS, g_print (" %#lx", (long) msg->lParam));

      if (!(msg->lParam & GCS_RESULTSTR))
	break;

      if (!propagate (&window, msg,
		      _bdk_display->keyboard_grab.window,
		      _bdk_display->keyboard_grab.owner_events,
		      BDK_ALL_EVENTS_MASK,
		      doesnt_want_char))
	break;

      if (BDK_WINDOW_DESTROYED (window))
	break;

      himc = ImmGetContext (msg->hwnd);
      ccount = ImmGetCompositionStringW (himc, GCS_RESULTSTR,
					 wbuf, sizeof (wbuf));
      ImmReleaseContext (msg->hwnd, himc);

      ccount /= 2;

      API_CALL (GetKeyboardState, (key_state));

      for (i = 0; i < ccount; i++)
	{
	  if (((BdkWindowObject *) window)->event_mask & BDK_KEY_PRESS_MASK)
	    {
	      /* Build a key press event */
	      event = bdk_event_new (BDK_KEY_PRESS);
	      event->key.window = window;
	      build_wm_ime_composition_event (event, msg, wbuf[i], key_state);

	      _bdk_win32_append_event (event);
	    }
	  
	  if (((BdkWindowObject *) window)->event_mask & BDK_KEY_RELEASE_MASK)
	    {
	      /* Build a key release event.  */
	      event = bdk_event_new (BDK_KEY_RELEASE);
	      event->key.window = window;
	      build_wm_ime_composition_event (event, msg, wbuf[i], key_state);

	      _bdk_win32_append_event (event);
	    }
	}
      return_val = TRUE;
      break;

    case WM_LBUTTONDOWN:
      button = 1;
      goto buttondown0;

    case WM_MBUTTONDOWN:
      button = 2;
      goto buttondown0;

    case WM_RBUTTONDOWN:
      button = 3;
      goto buttondown0;

    case WM_XBUTTONDOWN:
      if (HIWORD (msg->wParam) == XBUTTON1)
	button = 4;
      else
	button = 5;

    buttondown0:
      BDK_NOTE (EVENTS, 
		g_print (" (%d,%d)",
			 GET_X_LPARAM (msg->lParam), GET_Y_LPARAM (msg->lParam)));

      assign_object (&window, find_window_for_mouse_event (window, msg));

      if (BDK_WINDOW_DESTROYED (window))
	break;

      grab = _bdk_display_get_last_pointer_grab (_bdk_display);
      if (grab == NULL)
	{
	  SetCapture (BDK_WINDOW_HWND (window));
	}

      generate_button_event (BDK_BUTTON_PRESS, button,
			     window, msg);

      return_val = TRUE;
      break;

    case WM_LBUTTONUP:
      button = 1;
      goto buttonup0;

    case WM_MBUTTONUP:
      button = 2;
      goto buttonup0;

    case WM_RBUTTONUP:
      button = 3;
      goto buttonup0;

    case WM_XBUTTONUP:
      if (HIWORD (msg->wParam) == XBUTTON1)
	button = 4;
      else
	button = 5;

    buttonup0:
      BDK_NOTE (EVENTS, 
		g_print (" (%d,%d)",
			 GET_X_LPARAM (msg->lParam), GET_Y_LPARAM (msg->lParam)));

      assign_object (&window, find_window_for_mouse_event (window, msg));
      grab = _bdk_display_get_last_pointer_grab (_bdk_display);
      if (grab != NULL && grab->implicit)
	{
	  bint state = build_pointer_event_state (msg);

	  /* We keep the implicit grab until no buttons at all are held down */
	  if ((state & BDK_ANY_BUTTON_MASK & ~(BDK_BUTTON1_MASK << (button - 1))) == 0)
	    {
	      ReleaseCapture ();

	      new_window = NULL;
	      hwnd = WindowFromPoint (msg->pt);
	      if (hwnd != NULL)
		{
		  POINT client_pt = msg->pt;

		  ScreenToClient (hwnd, &client_pt);
		  GetClientRect (hwnd, &rect);
		  if (PtInRect (&rect, client_pt))
		    new_window = bdk_win32_handle_table_lookup ((BdkNativeWindow) hwnd);
		}
	      synthesize_crossing_events (_bdk_display,
					  grab->native_window, new_window,
					  BDK_CROSSING_UNGRAB,
					  &msg->pt,
					  0, /* TODO: Set right mask */
					  msg->time,
					  FALSE);
	      assign_object (&mouse_window, new_window);
	      mouse_window_ignored_leave = NULL;
	    }
	}

      generate_button_event (BDK_BUTTON_RELEASE, button,
			     window, msg);

      return_val = TRUE;
      break;

    case WM_MOUSEMOVE:
      BDK_NOTE (EVENTS,
		g_print (" %p (%d,%d)",
			 (bpointer) msg->wParam,
			 GET_X_LPARAM (msg->lParam), GET_Y_LPARAM (msg->lParam)));

      new_window = window;

      grab = _bdk_display_get_last_pointer_grab (_bdk_display);
      if (grab != NULL)
	{
	  POINT pt;
	  pt = msg->pt;

	  new_window = NULL;
	  hwnd = WindowFromPoint (pt);
	  if (hwnd != NULL)
	    {
	      POINT client_pt = pt;

	      ScreenToClient (hwnd, &client_pt);
	      GetClientRect (hwnd, &rect);
	      if (PtInRect (&rect, client_pt))
		new_window = bdk_win32_handle_table_lookup ((BdkNativeWindow) hwnd);
	    }

	  if (!grab->owner_events &&
	      new_window != NULL &&
	      new_window != grab->native_window)
	    new_window = NULL;
	}

      if (mouse_window != new_window)
	{
	  BDK_NOTE (EVENTS, g_print (" mouse_sinwod %p -> %p",
				     mouse_window ? BDK_WINDOW_HWND (mouse_window) : NULL, 
				     new_window ? BDK_WINDOW_HWND (new_window) : NULL));
	  synthesize_crossing_events (_bdk_display,
				      mouse_window, new_window,
				      BDK_CROSSING_NORMAL,
				      &msg->pt,
				      0, /* TODO: Set right mask */
				      msg->time,
				      FALSE);
	  assign_object (&mouse_window, new_window);
	  mouse_window_ignored_leave = NULL;
	  if (new_window != NULL)
	    track_mouse_event (TME_LEAVE, BDK_WINDOW_HWND (new_window));
	}
      else if (new_window != NULL && 
	       new_window == mouse_window_ignored_leave)
	{
	  /* If we ignored a leave event for this window and we're now getting
	     input again we need to re-arm the mouse tracking, as that was
	     cancelled by the mouseleave. */
	  mouse_window_ignored_leave = NULL;
	  track_mouse_event (TME_LEAVE, BDK_WINDOW_HWND (new_window));
	}

      assign_object (&window, find_window_for_mouse_event (window, msg));

      /* If we haven't moved, don't create any BDK event. Windows
       * sends WM_MOUSEMOVE messages after a new window is shows under
       * the mouse, even if the mouse hasn't moved. This disturbs btk.
       */
      if (msg->pt.x + _bdk_offset_x == current_root_x &&
	  msg->pt.y + _bdk_offset_y == current_root_y)
	break;

      current_root_x = msg->pt.x + _bdk_offset_x;
      current_root_y = msg->pt.y + _bdk_offset_y;

      event = bdk_event_new (BDK_MOTION_NOTIFY);
      event->motion.window = window;
      event->motion.time = _bdk_win32_get_next_tick (msg->time);
      event->motion.x = current_x = (bint16) GET_X_LPARAM (msg->lParam);
      event->motion.y = current_y = (bint16) GET_Y_LPARAM (msg->lParam);
      event->motion.x_root = current_root_x;
      event->motion.y_root = current_root_y;
      event->motion.axes = NULL;
      event->motion.state = build_pointer_event_state (msg);
      event->motion.is_hint = FALSE;
      event->motion.device = _bdk_display->core_pointer;

      _bdk_win32_append_event (event);

      return_val = TRUE;
      break;

    case WM_NCMOUSEMOVE:
      BDK_NOTE (EVENTS,
		g_print (" (%d,%d)",
			 GET_X_LPARAM (msg->lParam), GET_Y_LPARAM (msg->lParam)));
      break;

    case WM_MOUSELEAVE:
      BDK_NOTE (EVENTS, g_print (" %d (%ld,%ld)",
				 HIWORD (msg->wParam), msg->pt.x, msg->pt.y));

      new_window = NULL;
      hwnd = WindowFromPoint (msg->pt);
      ignore_leave = FALSE;
      if (hwnd != NULL)
	{
	  char classname[64];

	  POINT client_pt = msg->pt;

	  /* The synapitics trackpad drivers have this irritating
	     feature where it pops up a window right under the pointer
	     when you scroll. We ignore the leave and enter events for 
	     this window */
	  if (GetClassNameA (hwnd, classname, sizeof(classname)) &&
	      strcmp (classname, SYNAPSIS_ICON_WINDOW_CLASS) == 0)
	    ignore_leave = TRUE;

	  ScreenToClient (hwnd, &client_pt);
	  GetClientRect (hwnd, &rect);
	  if (PtInRect (&rect, client_pt))
	    new_window = bdk_win32_handle_table_lookup ((BdkNativeWindow) hwnd);
	}

      if (!ignore_leave)
	synthesize_crossing_events (_bdk_display,
				    mouse_window, new_window,
				    BDK_CROSSING_NORMAL,
				    &msg->pt,
				    0, /* TODO: Set right mask */
				    msg->time,
				    FALSE);
      assign_object (&mouse_window, new_window);
      mouse_window_ignored_leave = ignore_leave ? new_window : NULL;


      return_val = TRUE;
      break;

    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
      BDK_NOTE (EVENTS, g_print (" %d", (short) HIWORD (msg->wParam)));

      /* WM_MOUSEWHEEL is delivered to the focus window. Work around
       * that. Also, the position is in screen coordinates, not client
       * coordinates as with the button messages. I love the
       * consistency of Windows.
       */
      point.x = GET_X_LPARAM (msg->lParam);
      point.y = GET_Y_LPARAM (msg->lParam);

      if ((hwnd = WindowFromPoint (point)) == NULL)
	break;
      
      {
	char classname[64];

	/* The synapitics trackpad drivers have this irritating
	   feature where it pops up a window right under the pointer
	   when you scroll. We backtrack and to the toplevel and
	   find the innermost child instead. */
	if (GetClassNameA (hwnd, classname, sizeof(classname)) &&
	    strcmp (classname, SYNAPSIS_ICON_WINDOW_CLASS) == 0)
	  {
	    HWND hwndc;

	    /* Find our toplevel window */
	    hwnd = GetAncestor (msg->hwnd, GA_ROOT);

	    /* Walk back up to the outermost child at the desired point */
	    do {
	      ScreenToClient (hwnd, &point);
	      hwndc = ChildWindowFromPoint (hwnd, point);
	      ClientToScreen (hwnd, &point);
	    } while (hwndc != hwnd && (hwnd = hwndc, 1));
	  }
      }

      msg->hwnd = hwnd;
      if ((new_window = bdk_win32_handle_table_lookup ((BdkNativeWindow) msg->hwnd)) == NULL)
	break;

      if (new_window != window)
	{
	  assign_object (&window, new_window);
	}

      ScreenToClient (msg->hwnd, &point);

      event = bdk_event_new (BDK_SCROLL);
      event->scroll.window = window;

      if (msg->message == WM_MOUSEWHEEL)
        {
	  event->scroll.direction = (((short) HIWORD (msg->wParam)) > 0) ?
	    BDK_SCROLL_UP : BDK_SCROLL_DOWN;
        }
      else if (msg->message == WM_MOUSEHWHEEL)
        {
	  event->scroll.direction = (((short) HIWORD (msg->wParam)) > 0) ?
	    BDK_SCROLL_RIGHT : BDK_SCROLL_LEFT;
        }
      event->scroll.time = _bdk_win32_get_next_tick (msg->time);
      event->scroll.x = (bint16) point.x;
      event->scroll.y = (bint16) point.y;
      event->scroll.x_root = (bint16) GET_X_LPARAM (msg->lParam) + _bdk_offset_x;
      event->scroll.y_root = (bint16) GET_Y_LPARAM (msg->lParam) + _bdk_offset_y;
      event->scroll.state = build_pointer_event_state (msg);
      event->scroll.device = _bdk_display->core_pointer;

      _bdk_win32_append_event (event);
      
      return_val = TRUE;
      break;

    case WM_HSCROLL:
      /* Just print more debugging information, don't actually handle it. */
      BDK_NOTE (EVENTS,
		(g_print (" %s",
			  (LOWORD (msg->wParam) == SB_ENDSCROLL ? "ENDSCROLL" :
			   (LOWORD (msg->wParam) == SB_LEFT ? "LEFT" :
			    (LOWORD (msg->wParam) == SB_RIGHT ? "RIGHT" :
			     (LOWORD (msg->wParam) == SB_LINELEFT ? "LINELEFT" :
			      (LOWORD (msg->wParam) == SB_LINERIGHT ? "LINERIGHT" :
			       (LOWORD (msg->wParam) == SB_PAGELEFT ? "PAGELEFT" :
				(LOWORD (msg->wParam) == SB_PAGERIGHT ? "PAGERIGHT" :
				 (LOWORD (msg->wParam) == SB_THUMBPOSITION ? "THUMBPOSITION" :
				  (LOWORD (msg->wParam) == SB_THUMBTRACK ? "THUMBTRACK" :
				   "???")))))))))),
		 (LOWORD (msg->wParam) == SB_THUMBPOSITION ||
		  LOWORD (msg->wParam) == SB_THUMBTRACK) ?
		 (g_print (" %d", HIWORD (msg->wParam)), 0) : 0));
      break;

    case WM_VSCROLL:
      /* Just print more debugging information, don't actually handle it. */
      BDK_NOTE (EVENTS,
		(g_print (" %s",
			  (LOWORD (msg->wParam) == SB_ENDSCROLL ? "ENDSCROLL" :
			   (LOWORD (msg->wParam) == SB_BOTTOM ? "BOTTOM" :
			    (LOWORD (msg->wParam) == SB_TOP ? "TOP" :
			     (LOWORD (msg->wParam) == SB_LINEDOWN ? "LINDOWN" :
			      (LOWORD (msg->wParam) == SB_LINEUP ? "LINEUP" :
			       (LOWORD (msg->wParam) == SB_PAGEDOWN ? "PAGEDOWN" :
				(LOWORD (msg->wParam) == SB_PAGEUP ? "PAGEUP" :
				 (LOWORD (msg->wParam) == SB_THUMBPOSITION ? "THUMBPOSITION" :
				  (LOWORD (msg->wParam) == SB_THUMBTRACK ? "THUMBTRACK" :
				   "???")))))))))),
		 (LOWORD (msg->wParam) == SB_THUMBPOSITION ||
		  LOWORD (msg->wParam) == SB_THUMBTRACK) ?
		 (g_print (" %d", HIWORD (msg->wParam)), 0) : 0));
      break;

    case WM_QUERYNEWPALETTE:
      if (bdk_visual_get_system ()->type == BDK_VISUAL_PSEUDO_COLOR)
	{
	  synthesize_expose_events (window);
	  update_colors_counter = 0;
	}
      return_val = TRUE;
      break;

    case WM_PALETTECHANGED:
      BDK_NOTE (EVENTS_OR_COLORMAP, g_print (" %p", (HWND) msg->wParam));
      if (bdk_visual_get_system ()->type != BDK_VISUAL_PSEUDO_COLOR)
	break;

      return_val = TRUE;

      if (msg->hwnd == (HWND) msg->wParam)
	break;

      if (++update_colors_counter == 5)
	{
	  synthesize_expose_events (window);
	  update_colors_counter = 0;
	  break;
	}
      
      update_colors (window, TRUE);
      break;

     case WM_MOUSEACTIVATE:
       {
	 if (bdk_window_get_window_type (window) == BDK_WINDOW_TEMP 
	     || !((BdkWindowObject *)window)->accept_focus)
	   {
	     *ret_valp = MA_NOACTIVATE;
	     return_val = TRUE;
	   }

	 if (_bdk_modal_blocked (bdk_window_get_toplevel (window)))
	   {
	     *ret_valp = MA_NOACTIVATEANDEAT;
	     return_val = TRUE;
	   }
       }

       break;

    case WM_KILLFOCUS:
      if (_bdk_display->keyboard_grab.window != NULL &&
	  !BDK_WINDOW_DESTROYED (_bdk_display->keyboard_grab.window))
	{
	  generate_grab_broken_event (_bdk_display->keyboard_grab.window, TRUE, NULL);
	}

      /* fallthrough */
    case WM_SETFOCUS:
      if (_bdk_display->keyboard_grab.window != NULL &&
	  !_bdk_display->keyboard_grab.owner_events)
	break;

      if (!(((BdkWindowObject *) window)->event_mask & BDK_FOCUS_CHANGE_MASK))
	break;

      if (BDK_WINDOW_DESTROYED (window))
	break;

      generate_focus_event (window, (msg->message == WM_SETFOCUS));
      return_val = TRUE;
      break;

    case WM_ERASEBKGND:
      BDK_NOTE (EVENTS, g_print (" %p", (HANDLE) msg->wParam));
      
      if (BDK_WINDOW_DESTROYED (window))
	break;

      return_val = TRUE;
      *ret_valp = 1;
      break;

    case WM_SYNCPAINT:
      sync_timer = SetTimer (BDK_WINDOW_HWND (window),
			     1,
			     200, sync_timer_proc);
      break;

    case WM_PAINT:
      handle_wm_paint (msg, window, FALSE, NULL);
      break;

    case WM_SETCURSOR:
      BDK_NOTE (EVENTS, g_print (" %#x %#x",
				 LOWORD (msg->lParam), HIWORD (msg->lParam)));

      grab = _bdk_display_get_last_pointer_grab (_bdk_display);
      if (grab != NULL)
	{
	  grab_window = grab->window;
	}

      if (grab_window == NULL && LOWORD (msg->lParam) != HTCLIENT)
	break;

      if (grab_window != NULL && p_grab_cursor != NULL)
	hcursor = p_grab_cursor;
      else if (!BDK_WINDOW_DESTROYED (window))
	hcursor = BDK_WINDOW_IMPL_WIN32 (((BdkWindowObject *) window)->impl)->hcursor;
      else
	hcursor = NULL;

      if (hcursor != NULL)
	{
	  BDK_NOTE (EVENTS, g_print (" (SetCursor(%p)", hcursor));
	  SetCursor (hcursor);
	  return_val = TRUE;
	  *ret_valp = TRUE;
	}
      break;

    case WM_SYSCOMMAND:
      switch (msg->wParam)
	{
	case SC_MINIMIZE:
	case SC_RESTORE:
	  do_show_window (window, msg->wParam == SC_MINIMIZE ? TRUE : FALSE);
	  break;
	}

      break;

    case WM_ENTERSIZEMOVE:
    case WM_ENTERMENULOOP:
      if (msg->message == WM_ENTERSIZEMOVE)
	_modal_move_resize_window = msg->hwnd;

      _bdk_win32_begin_modal_call ();
      break;

    case WM_EXITSIZEMOVE:
    case WM_EXITMENULOOP:
      if (_modal_operation_in_progress)
	{
	  _modal_move_resize_window = NULL;
	  _bdk_win32_end_modal_call ();
	}
      break;

    case WM_CAPTURECHANGED:
      /* Sometimes we don't get WM_EXITSIZEMOVE, for instance when you
	 select move/size in the menu and then click somewhere without
	 moving/resizing. We work around this using WM_CAPTURECHANGED. */
      if (_modal_operation_in_progress)
	{
	  _modal_move_resize_window = NULL;
	  _bdk_win32_end_modal_call ();
	}
      break;

    case WM_WINDOWPOSCHANGING:
      BDK_NOTE (EVENTS, (windowpos = (WINDOWPOS *) msg->lParam,
			 g_print (" %s %s %dx%d@%+d%+d now below %p",
				  _bdk_win32_window_pos_bits_to_string (windowpos->flags),
				  (windowpos->hwndInsertAfter == HWND_BOTTOM ? "BOTTOM" :
				   (windowpos->hwndInsertAfter == HWND_NOTOPMOST ? "NOTOPMOST" :
				    (windowpos->hwndInsertAfter == HWND_TOP ? "TOP" :
				     (windowpos->hwndInsertAfter == HWND_TOPMOST ? "TOPMOST" :
				      (sprintf (buf, "%p", windowpos->hwndInsertAfter),
				       buf))))),
				  windowpos->cx, windowpos->cy, windowpos->x, windowpos->y,
				  GetNextWindow (msg->hwnd, GW_HWNDPREV))));

      if (BDK_WINDOW_IS_MAPPED (window))
	return_val = ensure_stacking_on_window_pos_changing (msg, window);
      break;

    case WM_WINDOWPOSCHANGED:
      windowpos = (WINDOWPOS *) msg->lParam;
      BDK_NOTE (EVENTS, g_print (" %s %s %dx%d@%+d%+d",
				 _bdk_win32_window_pos_bits_to_string (windowpos->flags),
				 (windowpos->hwndInsertAfter == HWND_BOTTOM ? "BOTTOM" :
				  (windowpos->hwndInsertAfter == HWND_NOTOPMOST ? "NOTOPMOST" :
				   (windowpos->hwndInsertAfter == HWND_TOP ? "TOP" :
				    (windowpos->hwndInsertAfter == HWND_TOPMOST ? "TOPMOST" :
				     (sprintf (buf, "%p", windowpos->hwndInsertAfter),
				      buf))))),
				 windowpos->cx, windowpos->cy, windowpos->x, windowpos->y));

      /* Break grabs on unmap or minimize */
      if (windowpos->flags & SWP_HIDEWINDOW || 
	  ((windowpos->flags & SWP_STATECHANGED) && IsIconic (msg->hwnd)))
	{
	  grab = _bdk_display_get_last_pointer_grab (_bdk_display);
	  if (grab != NULL)
	    {
	      if (grab->window == window)
		bdk_pointer_ungrab (msg->time);
	    }

	  if (_bdk_display->keyboard_grab.window == window)
	    bdk_keyboard_ungrab (msg->time);
	}

      /* Send MAP events  */
      if ((windowpos->flags & SWP_SHOWWINDOW) &&
	  !BDK_WINDOW_DESTROYED (window))
	{
	  event = bdk_event_new (BDK_MAP);
	  event->any.window = window;
	  _bdk_win32_append_event (event);
	}

      /* Update window state */
      if (windowpos->flags & (SWP_STATECHANGED | SWP_SHOWWINDOW | SWP_HIDEWINDOW))
	{
	  BdkWindowState set_bits, unset_bits, old_state, new_state;

	  old_state = BDK_WINDOW_OBJECT (window)->state;

	  set_bits = 0;
	  unset_bits = 0;

	  if (IsWindowVisible (msg->hwnd))
	    unset_bits |= BDK_WINDOW_STATE_WITHDRAWN;
	  else
	    set_bits |= BDK_WINDOW_STATE_WITHDRAWN;

	  if (IsIconic (msg->hwnd))
	    set_bits |= BDK_WINDOW_STATE_ICONIFIED;
	  else
	    unset_bits |= BDK_WINDOW_STATE_ICONIFIED;

	  if (IsZoomed (msg->hwnd))
	    set_bits |= BDK_WINDOW_STATE_MAXIMIZED;
	  else
	    unset_bits |= BDK_WINDOW_STATE_MAXIMIZED;

	  bdk_synthesize_window_state (window, unset_bits, set_bits);

	  new_state = BDK_WINDOW_OBJECT (window)->state;

	  /* Whenever one window changes iconified state we need to also
	   * change the iconified state in all transient related windows,
	   * as windows doesn't give icons for transient childrens. 
	   */
	  if ((old_state & BDK_WINDOW_STATE_ICONIFIED) != 
	      (new_state & BDK_WINDOW_STATE_ICONIFIED))
	    do_show_window (window, (new_state & BDK_WINDOW_STATE_ICONIFIED));


	  /* When un-minimizing, make sure we're stacked under any 
	     transient-type windows. */
	  if (!(old_state & BDK_WINDOW_STATE_ICONIFIED) &&
	      (new_state & BDK_WINDOW_STATE_ICONIFIED))
	    ensure_stacking_on_unminimize (msg);
	}

      /* Show, New size or position => configure event */
      if (!(windowpos->flags & SWP_NOCLIENTMOVE) ||
	  !(windowpos->flags & SWP_NOCLIENTSIZE) ||
	  (windowpos->flags & SWP_SHOWWINDOW))
	{
	  if (BDK_WINDOW_TYPE (window) != BDK_WINDOW_CHILD &&
	      !IsIconic (msg->hwnd) &&
	      !BDK_WINDOW_DESTROYED (window))
	    _bdk_win32_emit_configure_event (window);

	  if (((BdkWindowObject *) window)->input_window != NULL)
	    _bdk_input_configure_event (window);
	}

      if ((windowpos->flags & SWP_HIDEWINDOW) &&
	  !BDK_WINDOW_DESTROYED (window))
	{
	  /* Send UNMAP events  */
	  event = bdk_event_new (BDK_UNMAP);
	  event->any.window = window;
	  _bdk_win32_append_event (event);

	  /* Make transient parent the forground window when window unmaps */
	  impl = BDK_WINDOW_IMPL_WIN32 (BDK_WINDOW_OBJECT (window)->impl);

	  if (impl->transient_owner && 
	      GetForegroundWindow () == BDK_WINDOW_HWND (window))
	    SetForegroundWindow (BDK_WINDOW_HWND (impl->transient_owner));
	}

      if (!(windowpos->flags & SWP_NOCLIENTSIZE))
	{
	  if (((BdkWindowObject *) window)->resize_count > 1)
	    ((BdkWindowObject *) window)->resize_count -= 1;
	}

      /* Call modal timer immediate so that we repaint faster after a resize. */
      if (_modal_operation_in_progress)
	modal_timer_proc (0,0,0,0);

      /* Claim as handled, so that WM_SIZE and WM_MOVE are avoided */
      return_val = TRUE;
      *ret_valp = 0;
      break;

    case WM_SIZING:
      GetWindowRect (BDK_WINDOW_HWND (window), &rect);
      drag = (RECT *) msg->lParam;
      BDK_NOTE (EVENTS, g_print (" %s curr:%s drag:%s",
				 (msg->wParam == WMSZ_BOTTOM ? "BOTTOM" :
				  (msg->wParam == WMSZ_BOTTOMLEFT ? "BOTTOMLEFT" :
				   (msg->wParam == WMSZ_LEFT ? "LEFT" :
				    (msg->wParam == WMSZ_TOPLEFT ? "TOPLEFT" :
				     (msg->wParam == WMSZ_TOP ? "TOP" :
				      (msg->wParam == WMSZ_TOPRIGHT ? "TOPRIGHT" :
				       (msg->wParam == WMSZ_RIGHT ? "RIGHT" :
					
					(msg->wParam == WMSZ_BOTTOMRIGHT ? "BOTTOMRIGHT" :
					 "???")))))))),
				 _bdk_win32_rect_to_string (&rect),
				 _bdk_win32_rect_to_string (drag)));

      impl = BDK_WINDOW_IMPL_WIN32 (((BdkWindowObject *) window)->impl);
      orig_drag = *drag;
      if (impl->hint_flags & BDK_HINT_RESIZE_INC)
	{
	  BDK_NOTE (EVENTS, g_print (" (RESIZE_INC)"));
	  if (impl->hint_flags & BDK_HINT_BASE_SIZE)
	    {
	      /* Resize in increments relative to the base size */
	      rect.left = rect.top = 0;
	      rect.right = impl->hints.base_width;
	      rect.bottom = impl->hints.base_height;
	      _bdk_win32_adjust_client_rect (window, &rect);
	      point.x = rect.left;
	      point.y = rect.top;
	      ClientToScreen (BDK_WINDOW_HWND (window), &point);
	      rect.left = point.x;
	      rect.top = point.y;
	      point.x = rect.right;
	      point.y = rect.bottom;
	      ClientToScreen (BDK_WINDOW_HWND (window), &point);
	      rect.right = point.x;
	      rect.bottom = point.y;
	      
	      BDK_NOTE (EVENTS, g_print (" (also BASE_SIZE, using %s)",
					 _bdk_win32_rect_to_string (&rect)));
	    }

	  switch (msg->wParam)
	    {
	    case WMSZ_BOTTOM:
	      if (drag->bottom == rect.bottom)
		break;
	      adjust_drag (&drag->bottom, rect.bottom, impl->hints.height_inc);
	      break;

	    case WMSZ_BOTTOMLEFT:
	      if (drag->bottom == rect.bottom && drag->left == rect.left)
		break;
	      adjust_drag (&drag->bottom, rect.bottom, impl->hints.height_inc);
	      adjust_drag (&drag->left, rect.left, impl->hints.width_inc);
	      break;

	    case WMSZ_LEFT:
	      if (drag->left == rect.left)
		break;
	      adjust_drag (&drag->left, rect.left, impl->hints.width_inc);
	      break;

	    case WMSZ_TOPLEFT:
	      if (drag->top == rect.top && drag->left == rect.left)
		break;
	      adjust_drag (&drag->top, rect.top, impl->hints.height_inc);
	      adjust_drag (&drag->left, rect.left, impl->hints.width_inc);
	      break;

	    case WMSZ_TOP:
	      if (drag->top == rect.top)
		break;
	      adjust_drag (&drag->top, rect.top, impl->hints.height_inc);
	      break;

	    case WMSZ_TOPRIGHT:
	      if (drag->top == rect.top && drag->right == rect.right)
		break;
	      adjust_drag (&drag->top, rect.top, impl->hints.height_inc);
	      adjust_drag (&drag->right, rect.right, impl->hints.width_inc);
	      break;

	    case WMSZ_RIGHT:
	      if (drag->right == rect.right)
		break;
	      adjust_drag (&drag->right, rect.right, impl->hints.width_inc);
	      break;

	    case WMSZ_BOTTOMRIGHT:
	      if (drag->bottom == rect.bottom && drag->right == rect.right)
		break;
	      adjust_drag (&drag->bottom, rect.bottom, impl->hints.height_inc);
	      adjust_drag (&drag->right, rect.right, impl->hints.width_inc);
	      break;
	    }

	  if (drag->bottom != orig_drag.bottom || drag->left != orig_drag.left ||
	      drag->top != orig_drag.top || drag->right != orig_drag.right)
	    {
	      *ret_valp = TRUE;
	      return_val = TRUE;
	      BDK_NOTE (EVENTS, g_print (" (handled RESIZE_INC: %s)",
					 _bdk_win32_rect_to_string (drag)));
	    }
	}

      /* WM_GETMINMAXINFO handles min_size and max_size hints? */

      if (impl->hint_flags & BDK_HINT_ASPECT)
	{
	  RECT decorated_rect;
	  RECT undecorated_drag;
	  int decoration_width, decoration_height;
	  bdouble drag_aspect;
	  int drag_width, drag_height, new_width, new_height;

	  GetClientRect (BDK_WINDOW_HWND (window), &rect);
	  decorated_rect = rect;
	  _bdk_win32_adjust_client_rect (window, &decorated_rect);

	  /* Set undecorated_drag to the client area being dragged
	   * out, in screen coordinates.
	   */
	  undecorated_drag = *drag;
	  undecorated_drag.left -= decorated_rect.left - rect.left;
	  undecorated_drag.right -= decorated_rect.right - rect.right;
	  undecorated_drag.top -= decorated_rect.top - rect.top;
	  undecorated_drag.bottom -= decorated_rect.bottom - rect.bottom;

	  decoration_width = (decorated_rect.right - decorated_rect.left) - (rect.right - rect.left);
	  decoration_height = (decorated_rect.bottom - decorated_rect.top) - (rect.bottom - rect.top);

	  drag_width = undecorated_drag.right - undecorated_drag.left;
	  drag_height = undecorated_drag.bottom - undecorated_drag.top;

	  drag_aspect = (bdouble) drag_width / drag_height;

	  BDK_NOTE (EVENTS, g_print (" (ASPECT:%g--%g curr: %g)",
				     impl->hints.min_aspect, impl->hints.max_aspect, drag_aspect));

	  if (drag_aspect < impl->hints.min_aspect)
	    {
	      /* Aspect is getting too narrow */
	      switch (msg->wParam)
		{
		case WMSZ_BOTTOM:
		case WMSZ_TOP:
		  /* User drags top or bottom edge outward. Keep height, increase width. */
		  new_width = impl->hints.min_aspect * drag_height;
		  drag->left -= (new_width - drag_width) / 2;
		  drag->right = drag->left + new_width + decoration_width;
		  break;
		case WMSZ_BOTTOMLEFT:
		case WMSZ_BOTTOMRIGHT:
		  /* User drags bottom-left or bottom-right corner down. Adjust height. */
		  new_height = drag_width / impl->hints.min_aspect;
		  drag->bottom = drag->top + new_height + decoration_height;
		  break;
		case WMSZ_LEFT:
		case WMSZ_RIGHT:
		  /* User drags left or right edge inward. Decrease height */
		  new_height = drag_width / impl->hints.min_aspect;
		  drag->top += (drag_height - new_height) / 2;
		  drag->bottom = drag->top + new_height + decoration_height;
		  break;
		case WMSZ_TOPLEFT:
		case WMSZ_TOPRIGHT:
		  /* User drags top-left or top-right corner up. Adjust height. */
		  new_height = drag_width / impl->hints.min_aspect;
		  drag->top = drag->bottom - new_height - decoration_height;
		}
	    }
	  else if (drag_aspect > impl->hints.max_aspect)
	    {
	      /* Aspect is getting too wide */
	      switch (msg->wParam)
		{
		case WMSZ_BOTTOM:
		case WMSZ_TOP:
		  /* User drags top or bottom edge inward. Decrease width. */
		  new_width = impl->hints.max_aspect * drag_height;
		  drag->left += (drag_width - new_width) / 2;
		  drag->right = drag->left + new_width + decoration_width;
		  break;
		case WMSZ_BOTTOMLEFT:
		case WMSZ_TOPLEFT:
		  /* User drags bottom-left or top-left corner left. Adjust width. */
		  new_width = impl->hints.max_aspect * drag_height;
		  drag->left = drag->right - new_width - decoration_width;
		  break;
		case WMSZ_BOTTOMRIGHT:
		case WMSZ_TOPRIGHT:
		  /* User drags bottom-right or top-right corner right. Adjust width. */
		  new_width = impl->hints.max_aspect * drag_height;
		  drag->right = drag->left + new_width + decoration_width;
		  break;
		case WMSZ_LEFT:
		case WMSZ_RIGHT:
		  /* User drags left or right edge outward. Increase height. */
		  new_height = drag_width / impl->hints.max_aspect;
		  drag->top -= (new_height - drag_height) / 2;
		  drag->bottom = drag->top + new_height + decoration_height;
		  break;
		}
	    }

	  *ret_valp = TRUE;
	  return_val = TRUE;
	  BDK_NOTE (EVENTS, g_print (" (handled ASPECT: %s)",
				     _bdk_win32_rect_to_string (drag)));
	}
      break;

    case WM_GETMINMAXINFO:
      if (BDK_WINDOW_DESTROYED (window))
	break;

      impl = BDK_WINDOW_IMPL_WIN32 (((BdkWindowObject *) window)->impl);
      mmi = (MINMAXINFO*) msg->lParam;
      BDK_NOTE (EVENTS, g_print (" (mintrack:%ldx%ld maxtrack:%ldx%ld "
				 "maxpos:%+ld%+ld maxsize:%ldx%ld)",
				 mmi->ptMinTrackSize.x, mmi->ptMinTrackSize.y,
				 mmi->ptMaxTrackSize.x, mmi->ptMaxTrackSize.y,
				 mmi->ptMaxPosition.x, mmi->ptMaxPosition.y,
				 mmi->ptMaxSize.x, mmi->ptMaxSize.y));

      if (impl->hint_flags & BDK_HINT_MIN_SIZE)
	{
	  rect.left = rect.top = 0;
	  rect.right = impl->hints.min_width;
	  rect.bottom = impl->hints.min_height;

	  _bdk_win32_adjust_client_rect (window, &rect);

	  mmi->ptMinTrackSize.x = rect.right - rect.left;
	  mmi->ptMinTrackSize.y = rect.bottom - rect.top;
	}

      if (impl->hint_flags & BDK_HINT_MAX_SIZE)
	{
	  int maxw, maxh;

	  rect.left = rect.top = 0;
	  rect.right = impl->hints.max_width;
	  rect.bottom = impl->hints.max_height;

	  _bdk_win32_adjust_client_rect (window, &rect);

	  /* at least on win9x we have the 16 bit trouble */
	  maxw = rect.right - rect.left;
	  maxh = rect.bottom - rect.top;
	  mmi->ptMaxTrackSize.x = maxw > 0 && maxw < B_MAXSHORT ? maxw : B_MAXSHORT;
	  mmi->ptMaxTrackSize.y = maxh > 0 && maxh < B_MAXSHORT ? maxh : B_MAXSHORT;
	}
      else
	{
	  mmi->ptMaxTrackSize.x = 30000;
	  mmi->ptMaxTrackSize.y = 30000;
	}

      if (impl->hint_flags & (BDK_HINT_MIN_SIZE | BDK_HINT_MAX_SIZE))
	{
	  /* Don't call DefWindowProcW() */
	  BDK_NOTE (EVENTS, g_print (" (handled, mintrack:%ldx%ld maxtrack:%ldx%ld "
				     "maxpos:%+ld%+ld maxsize:%ldx%ld)",
				     mmi->ptMinTrackSize.x, mmi->ptMinTrackSize.y,
				     mmi->ptMaxTrackSize.x, mmi->ptMaxTrackSize.y,
				     mmi->ptMaxPosition.x, mmi->ptMaxPosition.y,
				     mmi->ptMaxSize.x, mmi->ptMaxSize.y));
	  return_val = TRUE;
	}

      return_val = TRUE;
      break;

    case WM_CLOSE:
      if (BDK_WINDOW_DESTROYED (window))
	break;

      event = bdk_event_new (BDK_DELETE);
      event->any.window = window;

      _bdk_win32_append_event (event);

      impl = BDK_WINDOW_IMPL_WIN32 (BDK_WINDOW_OBJECT (window)->impl);

      if (impl->transient_owner && GetForegroundWindow() == BDK_WINDOW_HWND (window))
	{
	  SetForegroundWindow (BDK_WINDOW_HWND (impl->transient_owner));
	}

      return_val = TRUE;
      break;

    case WM_NCDESTROY:
      grab = _bdk_display_get_last_pointer_grab (_bdk_display);
      if (grab != NULL)
	{
	  if (grab->window == window)
	    bdk_pointer_ungrab (msg->time);
	}

      if (_bdk_display->keyboard_grab.window == window)
	bdk_keyboard_ungrab (msg->time);

      if ((window != NULL) && (msg->hwnd != GetDesktopWindow ()))
	bdk_window_destroy_notify (window);

      if (window == NULL || BDK_WINDOW_DESTROYED (window))
	break;

      event = bdk_event_new (BDK_DESTROY);
      event->any.window = window;

      _bdk_win32_append_event (event);

      return_val = TRUE;
      break;

    case WM_DISPLAYCHANGE:
      handle_display_change ();
      break;
      
    case WM_DESTROYCLIPBOARD:
      if (!_ignore_destroy_clipboard)
	{
	  event = bdk_event_new (BDK_SELECTION_CLEAR);
	  event->selection.window = window;
	  event->selection.selection = BDK_SELECTION_CLIPBOARD;
	  event->selection.time = _bdk_win32_get_next_tick (msg->time);
          _bdk_win32_append_event (event);
	}
      else
	{
	  return_val = TRUE;
	}

      break;

    case WM_RENDERFORMAT:
      BDK_NOTE (EVENTS, g_print (" %s", _bdk_win32_cf_to_string (msg->wParam)));

      if (!(target = g_hash_table_lookup (_format_atom_table, BINT_TO_POINTER (msg->wParam))))
	{
	  BDK_NOTE (EVENTS, g_print (" (target not found)"));
	  return_val = TRUE;
	  break;
	}

      /* We need to render to clipboard immediately, don't call
       * _bdk_win32_append_event()
       */
      if (_bdk_event_func)
	{
	  event = bdk_event_new (BDK_SELECTION_REQUEST);
	  event->selection.window = window;
	  event->selection.send_event = FALSE;
	  event->selection.selection = BDK_SELECTION_CLIPBOARD;
	  event->selection.target = target;
	  event->selection.property = _bdk_selection;
	  event->selection.requestor = msg->hwnd;
	  event->selection.time = msg->time;

	  fixup_event (event);
	  BDK_NOTE (EVENTS, g_print (" (calling bdk_event_func)"));
	  BDK_NOTE (EVENTS, _bdk_win32_print_event (event));
	  (*_bdk_event_func) (event, _bdk_event_data);
	  bdk_event_free (event);

	  /* Now the clipboard owner should have rendered */
	  if (!_delayed_rendering_data)
	    {
	      BDK_NOTE (EVENTS, g_print (" (no _delayed_rendering_data?)"));
	    }
	  else
	    {
	      if (msg->wParam == CF_DIB)
		{
		  _delayed_rendering_data =
		    _bdk_win32_selection_convert_to_dib (_delayed_rendering_data,
							 target);
		  if (!_delayed_rendering_data)
		    {
		      g_warning ("Cannot convert to DIB from delayed rendered image");
		      break;
		    }
		}

	      /* The requestor is holding the clipboard, no
	       * OpenClipboard() is required/possible
	       */
	      BDK_NOTE (DND,
			g_print (" SetClipboardData(%s,%p)",
				 _bdk_win32_cf_to_string (msg->wParam),
				 _delayed_rendering_data));

	      API_CALL (SetClipboardData, (msg->wParam, _delayed_rendering_data));
	      _delayed_rendering_data = NULL;
	    }
	}
      break;

    case WM_ACTIVATE:
      BDK_NOTE (EVENTS, g_print (" %s%s %p",
				 (LOWORD (msg->wParam) == WA_ACTIVE ? "ACTIVE" :
				  (LOWORD (msg->wParam) == WA_CLICKACTIVE ? "CLICKACTIVE" :
				   (LOWORD (msg->wParam) == WA_INACTIVE ? "INACTIVE" : "???"))),
				 HIWORD (msg->wParam) ? " minimized" : "",
				 (HWND) msg->lParam));
      /* We handle mouse clicks for modally-blocked windows under WM_MOUSEACTIVATE,
       * but we still need to deal with alt-tab, or with SetActiveWindow() type
       * situations.
       */
      if (_bdk_modal_blocked (window) && LOWORD (msg->wParam) == WA_ACTIVE)
	{
	  BdkWindow *modal_current = _bdk_modal_current ();
	  SetActiveWindow (BDK_WINDOW_HWND (modal_current));
	  *ret_valp = 0;
	  return_val = TRUE;
	  break;
	}

      /* Bring any tablet contexts to the top of the overlap order when
       * one of our windows is activated.
       * NOTE: It doesn't seem to work well if it is done in WM_ACTIVATEAPP
       * instead
       */
      if (LOWORD(msg->wParam) != WA_INACTIVE)
	_bdk_input_set_tablet_active ();
      break;

    case WM_ACTIVATEAPP:
      BDK_NOTE (EVENTS, g_print (" %s thread: %I64d",
				 msg->wParam ? "YES" : "NO",
				 (bint64) msg->lParam));
      if (msg->wParam && BDK_WINDOW_IS_MAPPED (window))
	ensure_stacking_on_activate_app (msg, window);
      break;

      /* Handle WINTAB events here, as we know that bdkinput.c will
       * use the fixed WT_DEFBASE as lcMsgBase, and we thus can use the
       * constants as case labels.
       */
    case WT_PACKET:
      BDK_NOTE (EVENTS, g_print (" %d %p",
				 (int) msg->wParam, (bpointer) msg->lParam));
      goto wintab;
      
    case WT_CSRCHANGE:
      BDK_NOTE (EVENTS, g_print (" %d %p",
				 (int) msg->wParam, (bpointer) msg->lParam));
      goto wintab;
      
    case WT_PROXIMITY:
      BDK_NOTE (EVENTS, g_print (" %p %d %d",
				 (bpointer) msg->wParam,
				 LOWORD (msg->lParam),
				 HIWORD (msg->lParam)));
      /* Fall through */
    wintab:

      event = bdk_event_new (BDK_NOTHING);
      event->any.window = NULL;

      if (_bdk_input_other_event (event, msg, window))
	_bdk_win32_append_event (event);
      else
	bdk_event_free (event);

      break;
    }

done:

  if (window)
    g_object_unref (window);
  
#undef return
  return return_val;
}

void
_bdk_events_queue (BdkDisplay *display)
{
  MSG msg;

  if (modal_win32_dialog != NULL)
    return;
  
  while (PeekMessageW (&msg, NULL, 0, 0, PM_REMOVE))
    {
      TranslateMessage (&msg);
      DispatchMessageW (&msg);
    }
}

static bboolean
bdk_event_prepare (GSource *source,
		   bint    *timeout)
{
  bboolean retval;

  BDK_THREADS_ENTER ();

  *timeout = -1;

  retval = (_bdk_event_queue_find_first (_bdk_display) != NULL ||
	    (modal_win32_dialog == NULL &&
	     GetQueueStatus (QS_ALLINPUT) != 0));

  BDK_THREADS_LEAVE ();

  return retval;
}

static bboolean
bdk_event_check (GSource *source)
{
  bboolean retval;
  
  BDK_THREADS_ENTER ();

  if (event_poll_fd.revents & G_IO_IN)
    {
      retval = (_bdk_event_queue_find_first (_bdk_display) != NULL ||
		(modal_win32_dialog == NULL &&
		 GetQueueStatus (QS_ALLINPUT) != 0));
    }
  else
    {
      retval = FALSE;
    }

  BDK_THREADS_LEAVE ();

  return retval;
}

static bboolean
bdk_event_dispatch (GSource     *source,
		    GSourceFunc  callback,
		    bpointer     user_data)
{
  BdkEvent *event;
 
  BDK_THREADS_ENTER ();

  _bdk_events_queue (_bdk_display);
  event = _bdk_event_unqueue (_bdk_display);

  if (event)
    {
      if (_bdk_event_func)
	(*_bdk_event_func) (event, _bdk_event_data);
      
      bdk_event_free (event);

      /* Do drag & drop if it is still pending */
      if (_dnd_source_state == BDK_WIN32_DND_PENDING) 
	{
	  _dnd_source_state = BDK_WIN32_DND_DRAGGING;
	  _bdk_win32_dnd_do_dragdrop ();
	  _dnd_source_state = BDK_WIN32_DND_NONE;
	}
    }
  
  BDK_THREADS_LEAVE ();

  return TRUE;
}

void
bdk_win32_set_modal_dialog_libbtk_only (HWND window)
{
  modal_win32_dialog = window;
}

static void
check_for_too_much_data (BdkEvent *event)
{
  if (event->client.data.l[1] ||
      event->client.data.l[2] ||
      event->client.data.l[3] ||
      event->client.data.l[4])
    {
      g_warning ("Only four bytes of data are passed in client messages on Win32\n");
    }
}

bboolean
bdk_event_send_client_message_for_display (BdkDisplay     *display,
                                           BdkEvent       *event, 
                                           BdkNativeWindow winid)
{
  check_for_too_much_data (event);

  return PostMessageW ((HWND) winid, client_message,
		       (WPARAM) event->client.message_type,
		       event->client.data.l[0]);
}

void
bdk_screen_broadcast_client_message (BdkScreen *screen, 
				     BdkEvent  *event)
{
  check_for_too_much_data (event);

  PostMessageW (HWND_BROADCAST, client_message,
	       (WPARAM) event->client.message_type,
		event->client.data.l[0]);
}

void
bdk_flush (void)
{
  bdk_display_sync (_bdk_display);
}

void
bdk_display_sync (BdkDisplay * display)
{
  g_return_if_fail (display == _bdk_display);

  GdiFlush ();
}

void
bdk_display_flush (BdkDisplay * display)
{
  g_return_if_fail (display == _bdk_display);

  GdiFlush ();
}

bboolean
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
