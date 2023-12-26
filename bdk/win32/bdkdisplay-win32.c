/* BDK - The GIMP Drawing Kit
 * Copyright (C) 2002,2005 Hans Breuer
 * Copyright (C) 2003 Tor Lillqvist
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
#include "bdk.h"
#include "bdkprivate-win32.h"

#define HAVE_MONITOR_INFO

#if defined(_MSC_VER) && (WINVER < 0x500) && (WINVER > 0x0400)
#include <multimon.h>
#elif defined(_MSC_VER) && (WINVER <= 0x0400)
#undef HAVE_MONITOR_INFO
#endif

void
_bdk_windowing_set_default_display (BdkDisplay *display)
{
  g_assert (display == NULL || _bdk_display == display);
}

bulong
_bdk_windowing_window_get_next_serial (BdkDisplay *display)
{
	return 0;
}

#ifdef HAVE_MONITOR_INFO
static BOOL CALLBACK
count_monitor (HMONITOR hmonitor,
	       HDC      hdc,
	       LPRECT   rect,
	       LPARAM   data)
{
  bint *n = (bint *) data;

  (*n)++;

  return TRUE;
}

static BOOL CALLBACK
enum_monitor (HMONITOR hmonitor,
	      HDC      hdc,
	      LPRECT   rect,
	      LPARAM   data)
{
  /* The struct MONITORINFOEX definition is for some reason different
   * in the winuser.h bundled with mingw64 from that in MSDN and the
   * official 32-bit mingw (the MONITORINFO part is in a separate "mi"
   * member). So to keep this easily compileable with either, repeat
   * the MSDN definition it here.
   */
  typedef struct tagMONITORINFOEXA2 {
    DWORD cbSize;
    RECT  rcMonitor;
    RECT  rcWork;
    DWORD dwFlags;
    CHAR szDevice[CCHDEVICENAME];
  } MONITORINFOEXA2;

  MONITORINFOEXA2 monitor_info;
  HDC hDC;

  bint *index = (bint *) data;
  BdkWin32Monitor *monitor;

  if (*index >= _bdk_num_monitors)
    {
      (*index) += 1;

      return TRUE;
    }

  monitor = _bdk_monitors + *index;

  monitor_info.cbSize = sizeof (MONITORINFOEX);
  GetMonitorInfoA (hmonitor, (MONITORINFO *) &monitor_info);

#ifndef MONITORINFOF_PRIMARY
#define MONITORINFOF_PRIMARY 1
#endif

  monitor->name = g_strdup (monitor_info.szDevice);
  hDC = CreateDCA ("DISPLAY", monitor_info.szDevice, NULL, NULL);
  monitor->width_mm = GetDeviceCaps (hDC, HORZSIZE);
  monitor->height_mm = GetDeviceCaps (hDC, VERTSIZE);
  DeleteDC (hDC);
  monitor->rect.x = monitor_info.rcMonitor.left;
  monitor->rect.y = monitor_info.rcMonitor.top;
  monitor->rect.width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
  monitor->rect.height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;

  if (monitor_info.dwFlags & MONITORINFOF_PRIMARY &&
      *index != 0)
    {
      /* Put primary monitor at index 0, just in case somebody needs
       * to know which one is the primary.
       */
      BdkWin32Monitor temp = *monitor;
      *monitor = _bdk_monitors[0];
      _bdk_monitors[0] = temp;
    }

  (*index)++;

  return TRUE;
}
#endif /* HAVE_MONITOR_INFO */

void
_bdk_monitor_init (void)
{
#ifdef HAVE_MONITOR_INFO
  bint i, index;

  /* In case something happens between monitor counting and monitor
   * enumeration, repeat until the count matches up.
   * enum_monitor is coded to ignore any monitors past _bdk_num_monitors.
   */
  do
  {
    _bdk_num_monitors = 0;

    EnumDisplayMonitors (NULL, NULL, count_monitor, (LPARAM) &_bdk_num_monitors);

    _bdk_monitors = g_renew (BdkWin32Monitor, _bdk_monitors, _bdk_num_monitors);

    index = 0;
    EnumDisplayMonitors (NULL, NULL, enum_monitor, (LPARAM) &index);
  } while (index != _bdk_num_monitors);

  _bdk_offset_x = B_MININT;
  _bdk_offset_y = B_MININT;

  /* Calculate offset */
  for (i = 0; i < _bdk_num_monitors; i++)
    {
      _bdk_offset_x = MAX (_bdk_offset_x, -_bdk_monitors[i].rect.x);
      _bdk_offset_y = MAX (_bdk_offset_y, -_bdk_monitors[i].rect.y);
    }
  BDK_NOTE (MISC, g_print ("Multi-monitor offset: (%d,%d)\n",
			   _bdk_offset_x, _bdk_offset_y));

  /* Translate monitor coords into BDK coordinate space */
  for (i = 0; i < _bdk_num_monitors; i++)
    {
      _bdk_monitors[i].rect.x += _bdk_offset_x;
      _bdk_monitors[i].rect.y += _bdk_offset_y;
      BDK_NOTE (MISC, g_print ("Monitor %d: %dx%d@%+d%+d\n",
			       i, _bdk_monitors[i].rect.width,
			       _bdk_monitors[i].rect.height,
			       _bdk_monitors[i].rect.x,
			       _bdk_monitors[i].rect.y));
    }
#else
  HDC hDC;

  _bdk_num_monitors = 1;
  _bdk_monitors = g_renew (BdkWin32Monitor, _bdk_monitors, 1);

  _bdk_monitors[0].name = g_strdup ("DISPLAY");
  hDC = GetDC (NULL);
  _bdk_monitors[0].width_mm = GetDeviceCaps (hDC, HORZSIZE);
  _bdk_monitors[0].height_mm = GetDeviceCaps (hDC, VERTSIZE);
  ReleaseDC (NULL, hDC);
  _bdk_monitors[0].rect.x = 0;
  _bdk_monitors[0].rect.y = 0;
  _bdk_monitors[0].rect.width = GetSystemMetrics (SM_CXSCREEN);
  _bdk_monitors[0].rect.height = GetSystemMetrics (SM_CYSCREEN);
  _bdk_offset_x = 0;
  _bdk_offset_y = 0;
#endif
}

BdkDisplay *
bdk_display_open (const bchar *display_name)
{
  BDK_NOTE (MISC, g_print ("bdk_display_open: %s\n", (display_name ? display_name : "NULL")));

  if (display_name == NULL ||
      g_ascii_strcasecmp (display_name,
			  bdk_display_get_name (_bdk_display)) == 0)
    {
      if (_bdk_display != NULL)
	{
	  BDK_NOTE (MISC, g_print ("... return _bdk_display\n"));
	  return _bdk_display;
	}
    }
  else
    {
      BDK_NOTE (MISC, g_print ("... return NULL\n"));
      return NULL;
    }

  _bdk_display = g_object_new (BDK_TYPE_DISPLAY, NULL);
  _bdk_screen = g_object_new (BDK_TYPE_SCREEN, NULL);

  _bdk_monitor_init ();
  _bdk_visual_init ();
  bdk_screen_set_default_colormap (_bdk_screen,
                                   bdk_screen_get_system_colormap (_bdk_screen));
  _bdk_windowing_window_init (_bdk_screen);
  _bdk_windowing_image_init ();
  _bdk_events_init ();
  _bdk_input_init (_bdk_display);
  _bdk_dnd_init ();

  /* Precalculate display name */
  (void) bdk_display_get_name (_bdk_display);

  g_signal_emit_by_name (bdk_display_manager_get (),
			 "display_opened", _bdk_display);

  BDK_NOTE (MISC, g_print ("... _bdk_display now set up\n"));

  return _bdk_display;
}

const bchar *
bdk_display_get_name (BdkDisplay *display)
{
  HDESK hdesk = GetThreadDesktop (GetCurrentThreadId ());
  char dummy;
  char *desktop_name;
  HWINSTA hwinsta = GetProcessWindowStation ();
  char *window_station_name;
  DWORD n;
  DWORD session_id;
  char *display_name;
  static const char *display_name_cache = NULL;
  typedef BOOL (WINAPI *PFN_ProcessIdToSessionId) (DWORD, DWORD *);
  PFN_ProcessIdToSessionId processIdToSessionId;

  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);

  if (display_name_cache != NULL)
    return display_name_cache;

  n = 0;
  GetUserObjectInformation (hdesk, UOI_NAME, &dummy, 0, &n);
  if (n == 0)
    desktop_name = "Default";
  else
    {
      n++;
      desktop_name = g_alloca (n + 1);
      memset (desktop_name, 0, n + 1);

      if (!GetUserObjectInformation (hdesk, UOI_NAME, desktop_name, n, &n))
	desktop_name = "Default";
    }

  n = 0;
  GetUserObjectInformation (hwinsta, UOI_NAME, &dummy, 0, &n);
  if (n == 0)
    window_station_name = "WinSta0";
  else
    {
      n++;
      window_station_name = g_alloca (n + 1);
      memset (window_station_name, 0, n + 1);

      if (!GetUserObjectInformation (hwinsta, UOI_NAME, window_station_name, n, &n))
	window_station_name = "WinSta0";
    }

  processIdToSessionId = (PFN_ProcessIdToSessionId) GetProcAddress (GetModuleHandle ("kernel32.dll"), "ProcessIdToSessionId");
  if (!processIdToSessionId || !processIdToSessionId (GetCurrentProcessId (), &session_id))
    session_id = 0;

  display_name = g_strdup_printf ("%ld\\%s\\%s",
				  session_id,
				  window_station_name,
				  desktop_name);

  BDK_NOTE (MISC, g_print ("bdk_display_get_name: %s\n", display_name));

  display_name_cache = display_name;

  return display_name_cache;
}

bint
bdk_display_get_n_screens (BdkDisplay *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), 0);

  return 1;
}

BdkScreen *
bdk_display_get_screen (BdkDisplay *display,
			bint        screen_num)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (screen_num == 0, NULL);

  return _bdk_screen;
}

BdkScreen *
bdk_display_get_default_screen (BdkDisplay *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);

  return _bdk_screen;
}

BdkWindow *
bdk_display_get_default_group (BdkDisplay *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);

  g_warning ("bdk_display_get_default_group not yet implemented");

  return NULL;
}

bboolean
bdk_display_supports_selection_notification (BdkDisplay *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), FALSE);

  return TRUE;
}

static HWND _hwnd_next_viewer = NULL;
static int debug_indent = 0;

/*
 * maybe this should be integrated with the default message loop - or maybe not ;-)
 */
static LRESULT CALLBACK
inner_clipboard_window_procedure (HWND   hwnd,
                                  UINT   message,
                                  WPARAM wparam,
                                  LPARAM lparam)
{
  switch (message)
    {
    case WM_DESTROY: /* remove us from chain */
      {
        ChangeClipboardChain (hwnd, _hwnd_next_viewer);
        PostQuitMessage (0);
        return 0;
      }
    case WM_CHANGECBCHAIN:
      {
        HWND hwndRemove = (HWND) wparam; /* handle of window being removed */
        HWND hwndNext   = (HWND) lparam; /* handle of next window in chain */

        if (hwndRemove == _hwnd_next_viewer)
          _hwnd_next_viewer = hwndNext == hwnd ? NULL : hwndNext;
        else if (_hwnd_next_viewer != NULL)
          return SendMessage (_hwnd_next_viewer, message, wparam, lparam);

        return 0;
      }
#ifdef WM_CLIPBOARDUPDATE
    case WM_CLIPBOARDUPDATE:
#endif
    case WM_DRAWCLIPBOARD:
      {
        int success;
        HWND hwndOwner;
#ifdef G_ENABLE_DEBUG
        UINT nFormat = 0;
#endif
        BdkEvent *event;
        BdkWindow *owner;

        success = OpenClipboard (hwnd);
        g_return_val_if_fail (success, 0);
        hwndOwner = GetClipboardOwner ();
        owner = bdk_win32_window_lookup_for_display (_bdk_display, hwndOwner);
        if (owner == NULL)
          owner = bdk_win32_window_foreign_new_for_display (_bdk_display, hwndOwner);

        BDK_NOTE (DND, g_print (" drawclipboard owner: %p", hwndOwner));

#ifdef G_ENABLE_DEBUG
        if (_bdk_debug_flags & BDK_DEBUG_DND)
          {
            while ((nFormat = EnumClipboardFormats (nFormat)) != 0)
              g_print ("%s ", _bdk_win32_cf_to_string (nFormat));
          }
#endif

        BDK_NOTE (DND, g_print (" \n"));


        event = bdk_event_new (BDK_OWNER_CHANGE);
        event->owner_change.window = _bdk_root;
        event->owner_change.owner = owner;
        event->owner_change.reason = BDK_OWNER_CHANGE_NEW_OWNER;
        event->owner_change.selection = BDK_SELECTION_CLIPBOARD;
        event->owner_change.time = _bdk_win32_get_next_tick (0);
        event->owner_change.selection_time = BDK_CURRENT_TIME;
        _bdk_win32_append_event (event);

        CloseClipboard ();

        if (_hwnd_next_viewer != NULL)
          return SendMessage (_hwnd_next_viewer, message, wparam, lparam);

        /* clear error to avoid confusing SetClipboardViewer() return */
        SetLastError (0);
        return 0;
      }
    default:
      /* Otherwise call DefWindowProcW(). */
      BDK_NOTE (EVENTS, g_print (" DefWindowProcW"));
      return DefWindowProc (hwnd, message, wparam, lparam);
    }
}

static LRESULT CALLBACK
_clipboard_window_procedure (HWND   hwnd,
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
  retval = inner_clipboard_window_procedure (hwnd, message, wparam, lparam);
  debug_indent -= 2;

  BDK_NOTE (EVENTS, g_print (" => %I64d%s", (bint64) retval, (debug_indent == 0 ? "\n" : "")));

  return retval;
}

/*
 * Creates a hidden window and adds it to the clipboard chain
 */
static HWND
_bdk_win32_register_clipboard_notification (void)
{
  WNDCLASS wclass = { 0, };
  HWND     hwnd;
  ATOM     klass;

  wclass.lpszClassName = "BdkClipboardNotification";
  wclass.lpfnWndProc   = _clipboard_window_procedure;
  wclass.hInstance     = _bdk_app_hmodule;

  klass = RegisterClass (&wclass);
  if (!klass)
    return NULL;

  hwnd = CreateWindow (MAKEINTRESOURCE (klass),
                       NULL, WS_POPUP,
                       0, 0, 0, 0, NULL, NULL,
                       _bdk_app_hmodule, NULL);
  if (!hwnd)
    goto failed;

  SetLastError (0);
  _hwnd_next_viewer = SetClipboardViewer (hwnd);

  if (_hwnd_next_viewer == NULL && GetLastError() != 0)
    goto failed;

  /* FIXME: http://msdn.microsoft.com/en-us/library/ms649033(v=VS.85).aspx */
  /* This is only supported by Vista, and not yet by mingw64 */
  /* if (AddClipboardFormatListener (hwnd) == FALSE) */
  /*   goto failed; */

  return hwnd;

failed:
  g_critical ("Failed to install clipboard viewer");
  UnregisterClass (MAKEINTRESOURCE (klass), _bdk_app_hmodule);
  return NULL;
}

bboolean
bdk_display_request_selection_notification (BdkDisplay *display,
                                            BdkAtom     selection)

{
  static HWND hwndViewer = NULL;
  bboolean ret = FALSE;

  BDK_NOTE (DND,
            g_print ("bdk_display_request_selection_notification (..., %s)",
                     bdk_atom_name (selection)));

  if (selection == BDK_SELECTION_CLIPBOARD ||
      selection == BDK_SELECTION_PRIMARY)
    {
      if (!hwndViewer)
        {
          hwndViewer = _bdk_win32_register_clipboard_notification ();
          BDK_NOTE (DND, g_print (" registered"));
        }
      ret = (hwndViewer != NULL);
    }
  else
    {
      BDK_NOTE (DND, g_print (" unsupported"));
      ret = FALSE;
    }

  BDK_NOTE (DND, g_print (" -> %s\n", ret ? "TRUE" : "FALSE"));
  return ret;
}

bboolean
bdk_display_supports_clipboard_persistence (BdkDisplay *display)
{
  return FALSE;
}

void
bdk_display_store_clipboard (BdkDisplay    *display,
			     BdkWindow     *clipboard_window,
			     buint32        time_,
			     const BdkAtom *targets,
			     bint           n_targets)
{
}

bboolean
bdk_display_supports_shapes (BdkDisplay *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), FALSE);

  return TRUE;
}

bboolean
bdk_display_supports_input_shapes (BdkDisplay *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), FALSE);

  /* Not yet implemented. See comment in
   * bdk_window_input_shape_combine_mask().
   */

  return FALSE;
}

bboolean
bdk_display_supports_composite (BdkDisplay *display)
{
  return FALSE;
}
