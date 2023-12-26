/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 2005 Novell, Inc.
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* By Tor Lillqvist <tml@novell.com> 2005 */

/*
 * Modified by the BTK+ Team and others 1997-2005.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "btkmarshalers.h"
#include "btkplug.h"
#include "btkplugprivate.h"

#include "win32/bdkwin32.h"

#include "btkwin32embed.h"
#include "btkalias.h"

#if defined(_MSC_VER) && (WINVER < 0x0500)
#ifndef GA_PARENT
#define GA_PARENT 1
#endif
WINUSERAPI HWND WINAPI GetAncestor(HWND,UINT);
#endif

BdkNativeWindow
_btk_plug_windowing_get_id (BtkPlug *plug)
{
  return (BdkNativeWindow) BDK_WINDOW_HWND (BTK_WIDGET (plug)->window);
}

void
_btk_plug_windowing_realize_toplevel (BtkPlug *plug)
{
  if (plug->socket_window)
    {
      _btk_win32_embed_send (plug->socket_window,
			     BTK_WIN32_EMBED_PARENT_NOTIFY,
			     (WPARAM) BDK_WINDOW_HWND (BTK_WIDGET (plug)->window),
			     BTK_WIN32_EMBED_PROTOCOL_VERSION);
      _btk_win32_embed_send (plug->socket_window,
			     BTK_WIN32_EMBED_EVENT_PLUG_MAPPED, 0, 0);
    }
}

void
_btk_plug_windowing_map_toplevel (BtkPlug *plug)
{
  if (plug->socket_window)
    _btk_win32_embed_send (plug->socket_window,
			   BTK_WIN32_EMBED_EVENT_PLUG_MAPPED,
			   1, 0);
}

void
_btk_plug_windowing_unmap_toplevel (BtkPlug *plug)
{
  if (plug->socket_window)
    _btk_win32_embed_send (plug->socket_window,
			   BTK_WIN32_EMBED_EVENT_PLUG_MAPPED,
			   0, 0);
}

void
_btk_plug_windowing_set_focus (BtkPlug *plug)
{
  if (plug->socket_window)
    _btk_win32_embed_send (plug->socket_window,
			   BTK_WIN32_EMBED_REQUEST_FOCUS,
			   0, 0);
}

void
_btk_plug_windowing_add_grabbed_key (BtkPlug        *plug,
				     guint           accelerator_key,
				     BdkModifierType accelerator_mods)
{
  if (plug->socket_window)
    _btk_win32_embed_send (plug->socket_window,
			   BTK_WIN32_EMBED_GRAB_KEY,
			   accelerator_key, accelerator_mods);
}

void
_btk_plug_windowing_remove_grabbed_key (BtkPlug        *plug,
					guint           accelerator_key,
					BdkModifierType accelerator_mods)
{
  if (plug->socket_window)
    _btk_win32_embed_send (plug->socket_window,
			   BTK_WIN32_EMBED_UNGRAB_KEY,
			   accelerator_key, accelerator_mods);
}

void
_btk_plug_windowing_focus_to_parent (BtkPlug         *plug,
				     BtkDirectionType direction)
{
  BtkWin32EmbedMessageType message = BTK_WIN32_EMBED_FOCUS_PREV;
  
  switch (direction)
    {
    case BTK_DIR_UP:
    case BTK_DIR_LEFT:
    case BTK_DIR_TAB_BACKWARD:
      message = BTK_WIN32_EMBED_FOCUS_PREV;
      break;
    case BTK_DIR_DOWN:
    case BTK_DIR_RIGHT:
    case BTK_DIR_TAB_FORWARD:
      message = BTK_WIN32_EMBED_FOCUS_NEXT;
      break;
    }
  
  _btk_win32_embed_send_focus_message (plug->socket_window, message, 0);
}

BdkFilterReturn
_btk_plug_windowing_filter_func (BdkXEvent *bdk_xevent,
				 BdkEvent  *event,
				 gpointer   data)
{
  BtkPlug *plug = BTK_PLUG (data);
  MSG *msg = (MSG *) bdk_xevent;
  BdkFilterReturn return_val = BDK_FILTER_CONTINUE;

  switch (msg->message)
    {
      /* What message should we look for to notice the reparenting?
       * Maybe WM_WINDOWPOSCHANGED will work? This is handled in the
       * X11 implementation by handling ReparentNotify. Handle this
       * only for cross-process embedding, otherwise we get odd
       * crashes in testsocket.
       */
    case WM_WINDOWPOSCHANGED:
      if (!plug->same_app)
	{
	  HWND parent = GetAncestor (msg->hwnd, GA_PARENT);
	  gboolean was_embedded = plug->socket_window != NULL;
	  BdkScreen *screen = bdk_drawable_get_screen (event->any.window);
	  BdkDisplay *display = bdk_screen_get_display (screen);

	  BTK_NOTE (PLUGSOCKET, g_printerr ("WM_WINDOWPOSCHANGED: hwnd=%p GA_PARENT=%p socket_window=%p\n", msg->hwnd, parent, plug->socket_window));
	  g_object_ref (plug);
	  if (was_embedded)
	    {
	      /* End of embedding protocol for previous socket */
	      if (parent != BDK_WINDOW_HWND (plug->socket_window))
		{
		  BtkWidget *widget = BTK_WIDGET (plug);

		  BTK_NOTE (PLUGSOCKET, g_printerr ("was_embedded, current parent != socket_window\n"));
		  bdk_window_set_user_data (plug->socket_window, NULL);
		  g_object_unref (plug->socket_window);
		  plug->socket_window = NULL;

		  /* Emit a delete window, as if the user attempted to
		   * close the toplevel. Only do this if we are being
		   * reparented to the desktop window. Moving from one
		   * embedder to another should be invisible to the app.
		   */
		  if (parent == GetDesktopWindow ())
		    {
		      BTK_NOTE (PLUGSOCKET, g_printerr ("current parent is root window\n"));
		      _btk_plug_send_delete_event (widget);
		      return_val = BDK_FILTER_REMOVE;
		    }
		}
	      else
		{
		  BTK_NOTE (PLUGSOCKET, g_printerr ("still same parent\n"));
		  goto done;
		}
	    }

	  if (parent != GetDesktopWindow ())
	    {
	      /* Start of embedding protocol */

	      BTK_NOTE (PLUGSOCKET, g_printerr ("start of embedding\n"));
	      plug->socket_window = bdk_window_lookup_for_display (display, (BdkNativeWindow) parent);
	      if (plug->socket_window)
		{
		  gpointer user_data = NULL;

		  BTK_NOTE (PLUGSOCKET, g_printerr ("already had socket_window\n"));
		  bdk_window_get_user_data (plug->socket_window, &user_data);

		  if (user_data)
		    {
		      g_warning (B_STRLOC "Plug reparented unexpectedly into window in the same process");
		      plug->socket_window = NULL;
		      break;
		    }

		  g_object_ref (plug->socket_window);
		}
	      else
		{
		  plug->socket_window = bdk_window_foreign_new_for_display (display, (BdkNativeWindow) parent);
		  if (!plug->socket_window) /* Already gone */
		    break;
		}

	      _btk_plug_add_all_grabbed_keys (plug);

	      if (!was_embedded)
		g_signal_emit_by_name (plug, "embedded");
	    }
	done:
	  g_object_unref (plug);
	}
      break;

    case WM_SIZE:
      if (!plug->same_app && plug->socket_window)
	{
	  _btk_win32_embed_send (plug->socket_window,
				 BTK_WIN32_EMBED_PLUG_RESIZED,
				 0, 0);
	}
      break;

    default:
      if (msg->message == _btk_win32_embed_message_type (BTK_WIN32_EMBED_WINDOW_ACTIVATE))
	{
	  BTK_NOTE (PLUGSOCKET, g_printerr ("BtkPlug: WINDOW_ACTIVATE received\n"));
	  _btk_win32_embed_push_message (msg);
	  _btk_window_set_is_active (BTK_WINDOW (plug), TRUE);
	  _btk_win32_embed_pop_message ();
	  return_val = BDK_FILTER_REMOVE;
	}
      else if (msg->message == _btk_win32_embed_message_type (BTK_WIN32_EMBED_WINDOW_DEACTIVATE))
	{
	  BTK_NOTE (PLUGSOCKET, g_printerr ("BtkPlug: WINDOW_DEACTIVATE received\n"));
	  _btk_win32_embed_push_message (msg);
	  _btk_window_set_is_active (BTK_WINDOW (plug), FALSE);
	  _btk_win32_embed_pop_message ();
	  return_val = BDK_FILTER_REMOVE;
	}
      else if (msg->message == _btk_win32_embed_message_type (BTK_WIN32_EMBED_FOCUS_IN))
	{
	  BTK_NOTE (PLUGSOCKET, g_printerr ("BtkPlug: FOCUS_IN received\n"));
	  _btk_win32_embed_push_message (msg);
	  _btk_window_set_has_toplevel_focus (BTK_WINDOW (plug), TRUE);
	  switch (msg->wParam)
	    {
	    case BTK_WIN32_EMBED_FOCUS_CURRENT:
	      break;
	    case BTK_WIN32_EMBED_FOCUS_FIRST:
	      _btk_plug_focus_first_last (plug, BTK_DIR_TAB_FORWARD);
	      break;
	    case BTK_WIN32_EMBED_FOCUS_LAST:
	      _btk_plug_focus_first_last (plug, BTK_DIR_TAB_BACKWARD);
	      break;
	    }
	  _btk_win32_embed_pop_message ();
	  return_val = BDK_FILTER_REMOVE;
	}
      else if (msg->message == _btk_win32_embed_message_type (BTK_WIN32_EMBED_FOCUS_OUT))
	{
	  BTK_NOTE (PLUGSOCKET, g_printerr ("BtkPlug: FOCUS_OUT received\n"));
	  _btk_win32_embed_push_message (msg);
	  _btk_window_set_has_toplevel_focus (BTK_WINDOW (plug), FALSE);
	  _btk_win32_embed_pop_message ();
	  return_val = BDK_FILTER_REMOVE;
	}
      else if (msg->message == _btk_win32_embed_message_type (BTK_WIN32_EMBED_MODALITY_ON))
	{
	  BTK_NOTE (PLUGSOCKET, g_printerr ("BtkPlug: MODALITY_ON received\n"));
	  _btk_win32_embed_push_message (msg);
	  _btk_plug_handle_modality_on (plug);
	  _btk_win32_embed_pop_message ();
	  return_val = BDK_FILTER_REMOVE;
	}
      else if (msg->message == _btk_win32_embed_message_type (BTK_WIN32_EMBED_MODALITY_OFF))
	{
	  BTK_NOTE (PLUGSOCKET, g_printerr ("BtkPlug: MODALITY_OFF received\n"));
	  _btk_win32_embed_push_message (msg);
	  _btk_plug_handle_modality_off (plug);
	  _btk_win32_embed_pop_message ();
	  return_val = BDK_FILTER_REMOVE;
	}
      break;
    }

  return return_val;
}
