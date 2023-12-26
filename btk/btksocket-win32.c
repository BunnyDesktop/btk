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

#include "config.h"
#include <string.h>

#include "btkwindow.h"
#include "btkplug.h"
#include "btkprivate.h"
#include "btksocket.h"
#include "btksocketprivate.h"

#include "win32/bdkwin32.h"

#include "btkwin32embed.h"
#include "btkalias.h"

BdkNativeWindow
_btk_socket_windowing_get_id (BtkSocket *socket)
{
  g_return_val_if_fail (BTK_IS_SOCKET (socket), 0);
  g_return_val_if_fail (BTK_WIDGET_ANCHORED (socket), 0);

  if (!btk_widget_get_realized (socket))
    btk_widget_realize (BTK_WIDGET (socket));

  return (BdkNativeWindow) BDK_WINDOW_HWND (BTK_WIDGET (socket)->window);
}

void
_btk_socket_windowing_realize_window (BtkSocket *socket)
{
  /* XXX Anything needed? */
}

void
_btk_socket_windowing_end_embedding_toplevel (BtkSocket *socket)
{
  btk_window_remove_embedded_xid (BTK_WINDOW (btk_widget_get_toplevel (BTK_WIDGET (socket))),
				  BDK_WINDOW_HWND (socket->plug_window));
}

void
_btk_socket_windowing_size_request (BtkSocket *socket)
{
  MINMAXINFO mmi;

  socket->request_width = 1;
  socket->request_height = 1;
  
  mmi.ptMaxSize.x = mmi.ptMaxSize.y = 16000; /* ??? */
  mmi.ptMinTrackSize.x = mmi.ptMinTrackSize.y = 1;
  mmi.ptMaxTrackSize.x = mmi.ptMaxTrackSize.y = 16000; /* ??? */
  mmi.ptMaxPosition.x = mmi.ptMaxPosition.y = 0;

  if (SendMessage (BDK_WINDOW_HWND (socket->plug_window), WM_GETMINMAXINFO,
		   0, (LPARAM) &mmi) == 0)
    {
      socket->request_width = mmi.ptMinTrackSize.x;
      socket->request_height = mmi.ptMinTrackSize.y;
    }
  socket->have_size = TRUE;
}

void
_btk_socket_windowing_send_key_event (BtkSocket *socket,
				      BdkEvent  *bdk_event,
				      bboolean   mask_key_presses)
{
  PostMessage (BDK_WINDOW_HWND (socket->plug_window),
	       (bdk_event->type == BDK_KEY_PRESS ? WM_KEYDOWN : WM_KEYUP),
	       bdk_event->key.hardware_keycode, 0);
}

void
_btk_socket_windowing_focus_change (BtkSocket *socket,
				    bboolean   focus_in)
{
  if (focus_in)
    _btk_win32_embed_send_focus_message (socket->plug_window,
					 BTK_WIN32_EMBED_FOCUS_IN,
					 BTK_WIN32_EMBED_FOCUS_CURRENT);
  else
    _btk_win32_embed_send (socket->plug_window,
			   BTK_WIN32_EMBED_FOCUS_OUT,
			   0, 0);
}

void
_btk_socket_windowing_update_active (BtkSocket *socket,
				     bboolean   active)
{
  _btk_win32_embed_send (socket->plug_window,
			 (active ? BTK_WIN32_EMBED_WINDOW_ACTIVATE : BTK_WIN32_EMBED_WINDOW_DEACTIVATE),
			 0, 0);
}

void
_btk_socket_windowing_update_modality (BtkSocket *socket,
				       bboolean   modality)
{
  _btk_win32_embed_send (socket->plug_window,
			 (modality ? BTK_WIN32_EMBED_MODALITY_ON : BTK_WIN32_EMBED_MODALITY_OFF),
			 0, 0);
}

void
_btk_socket_windowing_focus (BtkSocket       *socket,
			     BtkDirectionType direction)
{
  int detail = -1;

  switch (direction)
    {
    case BTK_DIR_UP:
    case BTK_DIR_LEFT:
    case BTK_DIR_TAB_BACKWARD:
      detail = BTK_WIN32_EMBED_FOCUS_LAST;
      break;
    case BTK_DIR_DOWN:
    case BTK_DIR_RIGHT:
    case BTK_DIR_TAB_FORWARD:
      detail = BTK_WIN32_EMBED_FOCUS_FIRST;
      break;
    }
  
  _btk_win32_embed_send_focus_message (socket->plug_window,
				       BTK_WIN32_EMBED_FOCUS_IN,
				       detail);
}

void
_btk_socket_windowing_send_configure_event (BtkSocket *socket)
{
  /* XXX Nothing needed? */
}

void
_btk_socket_windowing_select_plug_window_input (BtkSocket *socket)
{
  /* XXX Nothing needed? */
}

void
_btk_socket_windowing_embed_get_info (BtkSocket *socket)
{
  socket->is_mapped = TRUE;	/* XXX ? */
}

void
_btk_socket_windowing_embed_notify (BtkSocket *socket)
{
  /* XXX Nothing needed? */
}

bboolean
_btk_socket_windowing_embed_get_focus_wrapped (void)
{
  return _btk_win32_embed_get_focus_wrapped ();
}

void
_btk_socket_windowing_embed_set_focus_wrapped (void)
{
  _btk_win32_embed_set_focus_wrapped ();
}

BdkFilterReturn
_btk_socket_windowing_filter_func (BdkXEvent *bdk_xevent,
				   BdkEvent  *event,
				   bpointer   data)
{
  BtkSocket *socket;
  BtkWidget *widget;
  MSG *msg;
  BdkFilterReturn return_val;

  socket = BTK_SOCKET (data);

  return_val = BDK_FILTER_CONTINUE;

  if (socket->plug_widget)
    return return_val;

  widget = BTK_WIDGET (socket);
  msg = (MSG *) bdk_xevent;

  switch (msg->message)
    {
    default:
      if (msg->message == _btk_win32_embed_message_type (BTK_WIN32_EMBED_PARENT_NOTIFY))
	{
	  BTK_NOTE (PLUGSOCKET, g_printerr ("BtkSocket: PARENT_NOTIFY received window=%p version=%d\n",
					    (bpointer) msg->wParam, (int) msg->lParam));
	  /* If we some day different protocols deployed need to add
	   * some more elaborate version handshake
	   */
	  if (msg->lParam != BTK_WIN32_EMBED_PROTOCOL_VERSION)
	    g_warning ("BTK Win32 embedding protocol version mismatch, "
		       "client uses version %d, we understand version %d",
		       (int) msg->lParam, BTK_WIN32_EMBED_PROTOCOL_VERSION);
	  if (!socket->plug_window)
	    {
	      _btk_socket_add_window (socket, (BdkNativeWindow) msg->wParam, FALSE);
	      
	      if (socket->plug_window)
		BTK_NOTE (PLUGSOCKET, g_printerr ("BtkSocket: window created"));
	      
	      return_val = BDK_FILTER_REMOVE;
	    }
	}
      else if (msg->message == _btk_win32_embed_message_type (BTK_WIN32_EMBED_EVENT_PLUG_MAPPED))
	{
	  bboolean was_mapped = socket->is_mapped;
	  bboolean is_mapped = msg->wParam != 0;

	  BTK_NOTE (PLUGSOCKET, g_printerr ("BtkSocket: PLUG_MAPPED received is_mapped:%d\n", is_mapped));
	  if (was_mapped != is_mapped)
	    {
	      if (is_mapped)
		_btk_socket_handle_map_request (socket);
	      else
		{
		  bdk_window_show (socket->plug_window);
		  _btk_socket_unmap_notify (socket);
		}
	    }
	  return_val = BDK_FILTER_REMOVE;
	}
      else if (msg->message == _btk_win32_embed_message_type (BTK_WIN32_EMBED_PLUG_RESIZED))
	{
	  BTK_NOTE (PLUGSOCKET, g_printerr ("BtkSocket: PLUG_RESIZED received\n"));
	  socket->have_size = FALSE;
	  btk_widget_queue_resize (widget);
	  return_val = BDK_FILTER_REMOVE;
	}
      else if (msg->message == _btk_win32_embed_message_type (BTK_WIN32_EMBED_REQUEST_FOCUS))
	{
	  BTK_NOTE (PLUGSOCKET, g_printerr ("BtkSocket: REQUEST_FOCUS received\n"));
	  _btk_win32_embed_push_message (msg);
	  _btk_socket_claim_focus (socket, TRUE);
	  _btk_win32_embed_pop_message ();
	  return_val = BDK_FILTER_REMOVE;
	}
      else if (msg->message == _btk_win32_embed_message_type (BTK_WIN32_EMBED_FOCUS_NEXT))
	{
	  BTK_NOTE (PLUGSOCKET, g_printerr ("BtkSocket: FOCUS_NEXT received\n"));
	  _btk_win32_embed_push_message (msg);
	  _btk_socket_advance_toplevel_focus (socket, BTK_DIR_TAB_FORWARD);
	  _btk_win32_embed_pop_message ();
	  return_val = BDK_FILTER_REMOVE;
	}
      else if (msg->message == _btk_win32_embed_message_type (BTK_WIN32_EMBED_FOCUS_PREV))
	{
	  BTK_NOTE (PLUGSOCKET, g_printerr ("BtkSocket: FOCUS_PREV received\n"));
	  _btk_win32_embed_push_message (msg);
	  _btk_socket_advance_toplevel_focus (socket, BTK_DIR_TAB_BACKWARD);
	  _btk_win32_embed_pop_message ();
	  return_val = BDK_FILTER_REMOVE;
	}
      else if (msg->message == _btk_win32_embed_message_type (BTK_WIN32_EMBED_GRAB_KEY))
	{
	  BTK_NOTE (PLUGSOCKET, g_printerr ("BtkSocket: GRAB_KEY received\n"));
	  _btk_win32_embed_push_message (msg);
	  _btk_socket_add_grabbed_key (socket, msg->wParam, msg->lParam);
	  _btk_win32_embed_pop_message ();
	  return_val = BDK_FILTER_REMOVE;
	}
      else if (msg->message == _btk_win32_embed_message_type (BTK_WIN32_EMBED_UNGRAB_KEY))
	{
	  BTK_NOTE (PLUGSOCKET, g_printerr ("BtkSocket: UNGRAB_KEY received\n"));
	  _btk_win32_embed_push_message (msg);
	  _btk_socket_remove_grabbed_key (socket, msg->wParam, msg->lParam);
	  _btk_win32_embed_pop_message ();
	  return_val = BDK_FILTER_REMOVE;
	}
      break;
    }

  return return_val;
}

