/* BTK - The GIMP Toolkit
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* By Owen Taylor <otaylor@btk.org>              98/4/4 */

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"
#include <string.h>

#include "bdk/bdkkeysyms.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkwindow.h"
#include "btkplug.h"
#include "btkprivate.h"
#include "btksocket.h"
#include "btksocketprivate.h"
#include "btkdnd.h"

#include "x11/bdkx.h"

#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif

#include "btkxembed.h"
#include "btkalias.h"

static gboolean xembed_get_info     (BdkWindow     *bdk_window,
				     unsigned long *version,
				     unsigned long *flags);

/* From Tk */
#define EMBEDDED_APP_WANTS_FOCUS NotifyNormal+20

BdkNativeWindow
_btk_socket_windowing_get_id (BtkSocket *socket)
{
  return BDK_WINDOW_XWINDOW (BTK_WIDGET (socket)->window);
}

void
_btk_socket_windowing_realize_window (BtkSocket *socket)
{
  BdkWindow *window = BTK_WIDGET (socket)->window;
  XWindowAttributes xattrs;

  XGetWindowAttributes (BDK_WINDOW_XDISPLAY (window),
			BDK_WINDOW_XWINDOW (window),
			&xattrs);

  /* Sooooo, it turns out that mozilla, as per the btk2xt code selects
     for input on the socket with a mask of 0x0fffff (for god knows why)
     which includes ButtonPressMask causing a BadAccess if someone else
     also selects for this. As per the client-side windows merge we always
     normally selects for button press so we can emulate it on client
     side children that selects for button press. However, we don't need
     this for BtkSocket, so we unselect it here, fixing the crashes in
     firefox. */
  XSelectInput (BDK_WINDOW_XDISPLAY (window),
		BDK_WINDOW_XWINDOW (window), 
		(xattrs.your_event_mask & ~ButtonPressMask) |
		SubstructureNotifyMask | SubstructureRedirectMask);
}

void
_btk_socket_windowing_end_embedding_toplevel (BtkSocket *socket)
{
  btk_window_remove_embedded_xid (BTK_WINDOW (btk_widget_get_toplevel (BTK_WIDGET (socket))),
				  BDK_WINDOW_XWINDOW (socket->plug_window));
}

void
_btk_socket_windowing_size_request (BtkSocket *socket)
{
  XSizeHints hints;
  long supplied;
	  
  bdk_error_trap_push ();

  socket->request_width = 1;
  socket->request_height = 1;
	  
  if (XGetWMNormalHints (BDK_WINDOW_XDISPLAY (socket->plug_window),
			 BDK_WINDOW_XWINDOW (socket->plug_window),
			 &hints, &supplied))
    {
      if (hints.flags & PMinSize)
	{
	  socket->request_width = MAX (hints.min_width, 1);
	  socket->request_height = MAX (hints.min_height, 1);
	}
      else if (hints.flags & PBaseSize)
	{
	  socket->request_width = MAX (hints.base_width, 1);
	  socket->request_height = MAX (hints.base_height, 1);
	}
    }
  socket->have_size = TRUE;
  
  bdk_error_trap_pop ();
}

void
_btk_socket_windowing_send_key_event (BtkSocket *socket,
				      BdkEvent  *bdk_event,
				      gboolean   mask_key_presses)
{
  XKeyEvent xkey;
  BdkScreen *screen = bdk_window_get_screen (socket->plug_window);

  memset (&xkey, 0, sizeof (xkey));
  xkey.type = (bdk_event->type == BDK_KEY_PRESS) ? KeyPress : KeyRelease;
  xkey.window = BDK_WINDOW_XWINDOW (socket->plug_window);
  xkey.root = BDK_WINDOW_XWINDOW (bdk_screen_get_root_window (screen));
  xkey.subwindow = None;
  xkey.time = bdk_event->key.time;
  xkey.x = 0;
  xkey.y = 0;
  xkey.x_root = 0;
  xkey.y_root = 0;
  xkey.state = bdk_event->key.state;
  xkey.keycode = bdk_event->key.hardware_keycode;
  xkey.same_screen = True;/* FIXME ? */

  bdk_error_trap_push ();
  XSendEvent (BDK_WINDOW_XDISPLAY (socket->plug_window),
	      BDK_WINDOW_XWINDOW (socket->plug_window),
	      False,
	      (mask_key_presses ? KeyPressMask : NoEventMask),
	      (XEvent *)&xkey);
  bdk_display_sync (bdk_screen_get_display (screen));
  bdk_error_trap_pop ();
}

void
_btk_socket_windowing_focus_change (BtkSocket *socket,
				    gboolean   focus_in)
{
  if (focus_in)
    _btk_xembed_send_focus_message (socket->plug_window,
				    XEMBED_FOCUS_IN, XEMBED_FOCUS_CURRENT);
  else
    _btk_xembed_send_message (socket->plug_window,
			      XEMBED_FOCUS_OUT, 0, 0, 0);
}

void
_btk_socket_windowing_update_active (BtkSocket *socket,
				     gboolean   active)
{
  _btk_xembed_send_message (socket->plug_window,
			    active ? XEMBED_WINDOW_ACTIVATE : XEMBED_WINDOW_DEACTIVATE,
			    0, 0, 0);
}

void
_btk_socket_windowing_update_modality (BtkSocket *socket,
				       gboolean   modality)
{
  _btk_xembed_send_message (socket->plug_window,
			    modality ? XEMBED_MODALITY_ON : XEMBED_MODALITY_OFF,
			    0, 0, 0);
}

void
_btk_socket_windowing_focus (BtkSocket       *socket,
			     BtkDirectionType direction)
{
  gint detail = -1;

  switch (direction)
    {
    case BTK_DIR_UP:
    case BTK_DIR_LEFT:
    case BTK_DIR_TAB_BACKWARD:
      detail = XEMBED_FOCUS_LAST;
      break;
    case BTK_DIR_DOWN:
    case BTK_DIR_RIGHT:
    case BTK_DIR_TAB_FORWARD:
      detail = XEMBED_FOCUS_FIRST;
      break;
    }
  
  _btk_xembed_send_focus_message (socket->plug_window, XEMBED_FOCUS_IN, detail);
}

void
_btk_socket_windowing_send_configure_event (BtkSocket *socket)
{
  XConfigureEvent xconfigure;
  gint x, y;

  g_return_if_fail (socket->plug_window != NULL);

  memset (&xconfigure, 0, sizeof (xconfigure));
  xconfigure.type = ConfigureNotify;

  xconfigure.event = BDK_WINDOW_XWINDOW (socket->plug_window);
  xconfigure.window = BDK_WINDOW_XWINDOW (socket->plug_window);

  /* The ICCCM says that synthetic events should have root relative
   * coordinates. We still aren't really ICCCM compliant, since
   * we don't send events when the real toplevel is moved.
   */
  bdk_error_trap_push ();
  bdk_window_get_origin (socket->plug_window, &x, &y);
  bdk_error_trap_pop ();
			 
  xconfigure.x = x;
  xconfigure.y = y;
  xconfigure.width = BTK_WIDGET(socket)->allocation.width;
  xconfigure.height = BTK_WIDGET(socket)->allocation.height;

  xconfigure.border_width = 0;
  xconfigure.above = None;
  xconfigure.override_redirect = False;

  bdk_error_trap_push ();
  XSendEvent (BDK_WINDOW_XDISPLAY (socket->plug_window),
	      BDK_WINDOW_XWINDOW (socket->plug_window),
	      False, NoEventMask, (XEvent *)&xconfigure);
  bdk_display_sync (btk_widget_get_display (BTK_WIDGET (socket)));
  bdk_error_trap_pop ();
}

void
_btk_socket_windowing_select_plug_window_input (BtkSocket *socket)
{
  XSelectInput (BDK_DISPLAY_XDISPLAY (btk_widget_get_display (BTK_WIDGET (socket))),
		BDK_WINDOW_XWINDOW (socket->plug_window),
		StructureNotifyMask | PropertyChangeMask);
}

void
_btk_socket_windowing_embed_get_info (BtkSocket *socket)
{
  unsigned long version;
  unsigned long flags;

  socket->xembed_version = -1;
  if (xembed_get_info (socket->plug_window, &version, &flags))
    {
      socket->xembed_version = MIN (BTK_XEMBED_PROTOCOL_VERSION, version);
      socket->is_mapped = (flags & XEMBED_MAPPED) != 0;
    }
  else
    {
      /* FIXME, we should probably actually check the state before we started */
      socket->is_mapped = TRUE;
    }
}

void
_btk_socket_windowing_embed_notify (BtkSocket *socket)
{
#ifdef HAVE_XFIXES
  BdkDisplay *display = btk_widget_get_display (BTK_WIDGET (socket));

  XFixesChangeSaveSet (BDK_DISPLAY_XDISPLAY (display),
		       BDK_WINDOW_XWINDOW (socket->plug_window),
		       SetModeInsert, SaveSetRoot, SaveSetUnmap);
#endif
  _btk_xembed_send_message (socket->plug_window,
			    XEMBED_EMBEDDED_NOTIFY, 0,
			    BDK_WINDOW_XWINDOW (BTK_WIDGET (socket)->window),
			    socket->xembed_version);
}

static gboolean
xembed_get_info (BdkWindow     *window,
		 unsigned long *version,
		 unsigned long *flags)
{
  BdkDisplay *display = bdk_window_get_display (window);
  Atom xembed_info_atom = bdk_x11_get_xatom_by_name_for_display (display, "_XEMBED_INFO");
  Atom type;
  int format;
  unsigned long nitems, bytes_after;
  unsigned char *data;
  unsigned long *data_long;
  int status;
  
  bdk_error_trap_push();
  status = XGetWindowProperty (BDK_DISPLAY_XDISPLAY (display),
			       BDK_WINDOW_XWINDOW (window),
			       xembed_info_atom,
			       0, 2, False,
			       xembed_info_atom, &type, &format,
			       &nitems, &bytes_after, &data);
  bdk_error_trap_pop();

  if (status != Success)
    return FALSE;		/* Window vanished? */

  if (type == None)		/* No info property */
    return FALSE;

  if (type != xembed_info_atom)
    {
      g_warning ("_XEMBED_INFO property has wrong type\n");
      return FALSE;
    }
  
  if (nitems < 2)
    {
      g_warning ("_XEMBED_INFO too short\n");
      XFree (data);
      return FALSE;
    }
  
  data_long = (unsigned long *)data;
  if (version)
    *version = data_long[0];
  if (flags)
    *flags = data_long[1] & XEMBED_MAPPED;
  
  XFree (data);
  return TRUE;
}

gboolean
_btk_socket_windowing_embed_get_focus_wrapped (void)
{
  return _btk_xembed_get_focus_wrapped ();
}

void
_btk_socket_windowing_embed_set_focus_wrapped (void)
{
  _btk_xembed_set_focus_wrapped ();
}

static void
handle_xembed_message (BtkSocket        *socket,
		       XEmbedMessageType message,
		       glong             detail,
		       glong             data1,
		       glong             data2,
		       guint32           time)
{
  BTK_NOTE (PLUGSOCKET,
	    g_message ("BtkSocket: %s received", _btk_xembed_message_name (message)));
  
  switch (message)
    {
    case XEMBED_EMBEDDED_NOTIFY:
    case XEMBED_WINDOW_ACTIVATE:
    case XEMBED_WINDOW_DEACTIVATE:
    case XEMBED_MODALITY_ON:
    case XEMBED_MODALITY_OFF:
    case XEMBED_FOCUS_IN:
    case XEMBED_FOCUS_OUT:
      g_warning ("BtkSocket: Invalid _XEMBED message %s received", _btk_xembed_message_name (message));
      break;
      
    case XEMBED_REQUEST_FOCUS:
      _btk_socket_claim_focus (socket, TRUE);
      break;

    case XEMBED_FOCUS_NEXT:
    case XEMBED_FOCUS_PREV:
      _btk_socket_advance_toplevel_focus (socket,
					  (message == XEMBED_FOCUS_NEXT ?
					   BTK_DIR_TAB_FORWARD : BTK_DIR_TAB_BACKWARD));
      break;
      
    case XEMBED_BTK_GRAB_KEY:
      _btk_socket_add_grabbed_key (socket, data1, data2);
      break; 
    case XEMBED_BTK_UNGRAB_KEY:
      _btk_socket_remove_grabbed_key (socket, data1, data2);
      break;

    case XEMBED_GRAB_KEY:
    case XEMBED_UNGRAB_KEY:
      break;
      
    default:
      BTK_NOTE (PLUGSOCKET,
		g_message ("BtkSocket: Ignoring unknown _XEMBED message of type %d", message));
      break;
    }
}

BdkFilterReturn
_btk_socket_windowing_filter_func (BdkXEvent *bdk_xevent,
				   BdkEvent  *event,
				   gpointer   data)
{
  BtkSocket *socket;
  BtkWidget *widget;
  BdkDisplay *display;
  XEvent *xevent;

  BdkFilterReturn return_val;
  
  socket = BTK_SOCKET (data);

  return_val = BDK_FILTER_CONTINUE;

  if (socket->plug_widget)
    return return_val;

  widget = BTK_WIDGET (socket);
  xevent = (XEvent *)bdk_xevent;
  display = btk_widget_get_display (widget);

  switch (xevent->type)
    {
    case ClientMessage:
      if (xevent->xclient.message_type == bdk_x11_get_xatom_by_name_for_display (display, "_XEMBED"))
	{
	  _btk_xembed_push_message (xevent);
	  handle_xembed_message (socket,
				 xevent->xclient.data.l[1],
				 xevent->xclient.data.l[2],
				 xevent->xclient.data.l[3],
				 xevent->xclient.data.l[4],
				 xevent->xclient.data.l[0]);
	  _btk_xembed_pop_message ();
	  
	  return_val = BDK_FILTER_REMOVE;
	}
      break;

    case CreateNotify:
      {
	XCreateWindowEvent *xcwe = &xevent->xcreatewindow;

	if (!socket->plug_window)
	  {
	    _btk_socket_add_window (socket, xcwe->window, FALSE);

	    if (socket->plug_window)
	      {
		BTK_NOTE (PLUGSOCKET, g_message ("BtkSocket - window created"));
	      }
	  }
	
	return_val = BDK_FILTER_REMOVE;
	
	break;
      }

    case ConfigureRequest:
      {
	XConfigureRequestEvent *xcre = &xevent->xconfigurerequest;
	
	if (!socket->plug_window)
	  _btk_socket_add_window (socket, xcre->window, FALSE);
	
	if (socket->plug_window)
	  {
	    BtkSocketPrivate *private = _btk_socket_get_private (socket);
	    
	    if (xcre->value_mask & (CWWidth | CWHeight))
	      {
		BTK_NOTE (PLUGSOCKET,
			  g_message ("BtkSocket - configure request: %d %d",
				     socket->request_width,
				     socket->request_height));

		private->resize_count++;
		btk_widget_queue_resize (widget);
	      }
	    else if (xcre->value_mask & (CWX | CWY))
	      {
		_btk_socket_windowing_send_configure_event (socket);
	      }
	    /* Ignore stacking requests. */
	    
	    return_val = BDK_FILTER_REMOVE;
	  }
	break;
      }

    case DestroyNotify:
      {
	XDestroyWindowEvent *xdwe = &xevent->xdestroywindow;

	/* Note that we get destroy notifies both from SubstructureNotify on
	 * our window and StructureNotify on socket->plug_window
	 */
	if (socket->plug_window && (xdwe->window == BDK_WINDOW_XWINDOW (socket->plug_window)))
	  {
	    gboolean result;
	    
	    BTK_NOTE (PLUGSOCKET, g_message ("BtkSocket - destroy notify"));
	    
	    bdk_window_destroy_notify (socket->plug_window);
	    _btk_socket_end_embedding (socket);

	    g_object_ref (widget);
	    g_signal_emit_by_name (widget, "plug-removed", &result);
	    if (!result)
	      btk_widget_destroy (widget);
	    g_object_unref (widget);
	    
	    return_val = BDK_FILTER_REMOVE;
	  }
	break;
      }

    case FocusIn:
      if (xevent->xfocus.mode == EMBEDDED_APP_WANTS_FOCUS)
	{
	  _btk_socket_claim_focus (socket, TRUE);
	}
      return_val = BDK_FILTER_REMOVE;
      break;
    case FocusOut:
      return_val = BDK_FILTER_REMOVE;
      break;
    case MapRequest:
      if (!socket->plug_window)
	{
	  _btk_socket_add_window (socket, xevent->xmaprequest.window, FALSE);
	}
	
      if (socket->plug_window)
	{
	  BTK_NOTE (PLUGSOCKET, g_message ("BtkSocket - Map Request"));

	  _btk_socket_handle_map_request (socket);
	  return_val = BDK_FILTER_REMOVE;
	}
      break;
    case PropertyNotify:
      if (socket->plug_window &&
	  xevent->xproperty.window == BDK_WINDOW_XWINDOW (socket->plug_window))
	{
	  BdkDragProtocol protocol;

	  if (xevent->xproperty.atom == bdk_x11_get_xatom_by_name_for_display (display, "WM_NORMAL_HINTS"))
	    {
	      BTK_NOTE (PLUGSOCKET, g_message ("BtkSocket - received PropertyNotify for plug's WM_NORMAL_HINTS"));
	      socket->have_size = FALSE;
	      btk_widget_queue_resize (widget);
	      return_val = BDK_FILTER_REMOVE;
	    }
	  else if ((xevent->xproperty.atom == bdk_x11_get_xatom_by_name_for_display (display, "XdndAware")) ||
	      (xevent->xproperty.atom == bdk_x11_get_xatom_by_name_for_display (display, "_MOTIF_DRAG_RECEIVER_INFO")))
	    {
	      bdk_error_trap_push ();
	      if (bdk_drag_get_protocol_for_display (display,
						     xevent->xproperty.window,
						     &protocol))
		btk_drag_dest_set_proxy (BTK_WIDGET (socket),
					 socket->plug_window,
					 protocol, TRUE);

	      bdk_display_sync (display);
	      bdk_error_trap_pop ();
	      return_val = BDK_FILTER_REMOVE;
	    }
	  else if (xevent->xproperty.atom == bdk_x11_get_xatom_by_name_for_display (display, "_XEMBED_INFO"))
	    {
	      unsigned long flags;
	      
	      if (xembed_get_info (socket->plug_window, NULL, &flags))
		{
		  gboolean was_mapped = socket->is_mapped;
		  gboolean is_mapped = (flags & XEMBED_MAPPED) != 0;

		  if (was_mapped != is_mapped)
		    {
		      if (is_mapped)
			_btk_socket_handle_map_request (socket);
		      else
			{
			  bdk_error_trap_push ();
			  bdk_window_show (socket->plug_window);
			  bdk_flush ();
			  bdk_error_trap_pop ();
			  
			  _btk_socket_unmap_notify (socket);
			}
		    }
		}
	      return_val = BDK_FILTER_REMOVE;
	    }
	}
      break;
    case ReparentNotify:
      {
	XReparentEvent *xre = &xevent->xreparent;

	BTK_NOTE (PLUGSOCKET, g_message ("BtkSocket - ReparentNotify received"));
	if (!socket->plug_window && xre->parent == BDK_WINDOW_XWINDOW (widget->window))
	  {
	    _btk_socket_add_window (socket, xre->window, FALSE);
	    
	    if (socket->plug_window)
	      {
		BTK_NOTE (PLUGSOCKET, g_message ("BtkSocket - window reparented"));
	      }
	    
	    return_val = BDK_FILTER_REMOVE;
	  }
        else
          {
            if (socket->plug_window && xre->window == BDK_WINDOW_XWINDOW (socket->plug_window) && xre->parent != BDK_WINDOW_XWINDOW (widget->window))
              {
                gboolean result;

                _btk_socket_end_embedding (socket);

                g_object_ref (widget);
                g_signal_emit_by_name (widget, "plug-removed", &result);
                if (!result)
                  btk_widget_destroy (widget);
                g_object_unref (widget);

                return_val = BDK_FILTER_REMOVE;
              }
          }

	break;
      }
    case UnmapNotify:
      if (socket->plug_window &&
	  xevent->xunmap.window == BDK_WINDOW_XWINDOW (socket->plug_window))
	{
	  BTK_NOTE (PLUGSOCKET, g_message ("BtkSocket - Unmap notify"));

	  _btk_socket_unmap_notify (socket);
	  return_val = BDK_FILTER_REMOVE;
	}
      break;
      
    }
  
  return return_val;
}
