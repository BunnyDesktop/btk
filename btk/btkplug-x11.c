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

#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkplug.h"
#include "btkprivate.h"
#include "btkplugprivate.h"

#include "x11/bdkx.h"

#include "btkxembed.h"
#include "btkalias.h"

static void xembed_set_info            (BdkWindow     *window,
					unsigned long  flags);

BdkNativeWindow
_btk_plug_windowing_get_id (BtkPlug *plug)
{
  return BDK_WINDOW_XWINDOW (BTK_WIDGET (plug)->window);
}

void
_btk_plug_windowing_realize_toplevel (BtkPlug *plug)
{
  xembed_set_info (BTK_WIDGET (plug)->window, 0);
}

void
_btk_plug_windowing_map_toplevel (BtkPlug *plug)
{
  xembed_set_info (BTK_WIDGET (plug)->window, XEMBED_MAPPED);
}

void
_btk_plug_windowing_unmap_toplevel (BtkPlug *plug)
{
  xembed_set_info (BTK_WIDGET (plug)->window, 0);
}

void
_btk_plug_windowing_set_focus (BtkPlug *plug)
{
  _btk_xembed_send_message (plug->socket_window,
			    XEMBED_REQUEST_FOCUS, 0, 0, 0);
}

void
_btk_plug_windowing_add_grabbed_key (BtkPlug        *plug,
				     guint           accelerator_key,
				     BdkModifierType accelerator_mods)
{
  _btk_xembed_send_message (plug->socket_window, XEMBED_BTK_GRAB_KEY, 0, 
			    accelerator_key, accelerator_mods);
}

void
_btk_plug_windowing_remove_grabbed_key (BtkPlug        *plug,
					guint           accelerator_key,
					BdkModifierType accelerator_mods)
{
  _btk_xembed_send_message (plug->socket_window, XEMBED_BTK_UNGRAB_KEY, 0, 
			    accelerator_key, accelerator_mods);
}

void
_btk_plug_windowing_focus_to_parent (BtkPlug         *plug,
				     BtkDirectionType direction)
{
  XEmbedMessageType message = XEMBED_FOCUS_PREV; /* Quiet GCC */
  
  switch (direction)
    {
    case BTK_DIR_UP:
    case BTK_DIR_LEFT:
    case BTK_DIR_TAB_BACKWARD:
      message = XEMBED_FOCUS_PREV;
      break;
    case BTK_DIR_DOWN:
    case BTK_DIR_RIGHT:
    case BTK_DIR_TAB_FORWARD:
      message = XEMBED_FOCUS_NEXT;
      break;
    }
  
  _btk_xembed_send_focus_message (plug->socket_window, message, 0);
}

static void
xembed_set_info (BdkWindow     *window,
		 unsigned long  flags)
{
  BdkDisplay *display = bdk_window_get_display (window);
  unsigned long buffer[2];

  Atom xembed_info_atom = bdk_x11_get_xatom_by_name_for_display (display, "_XEMBED_INFO");

  buffer[0] = BTK_XEMBED_PROTOCOL_VERSION;
  buffer[1] = flags;

  XChangeProperty (BDK_DISPLAY_XDISPLAY (display),
		   BDK_WINDOW_XWINDOW (window),
		   xembed_info_atom, xembed_info_atom, 32,
		   PropModeReplace,
		   (unsigned char *)buffer, 2);
}

static void
handle_xembed_message (BtkPlug           *plug,
		       XEmbedMessageType  message,
		       glong              detail,
		       glong              data1,
		       glong              data2,
		       guint32            time)
{
  BtkWindow *window = BTK_WINDOW (plug);

  BTK_NOTE (PLUGSOCKET,
	    g_message ("BtkPlug: %s received", _btk_xembed_message_name (message)));
  
  switch (message)
    {
    case XEMBED_EMBEDDED_NOTIFY:
      break;
    case XEMBED_WINDOW_ACTIVATE:
      _btk_window_set_is_active (window, TRUE);
      break;
    case XEMBED_WINDOW_DEACTIVATE:
      _btk_window_set_is_active (window, FALSE);
      break;
      
    case XEMBED_MODALITY_ON:
      _btk_plug_handle_modality_on (plug);
      break;
    case XEMBED_MODALITY_OFF:
      _btk_plug_handle_modality_off (plug);
      break;

    case XEMBED_FOCUS_IN:
      _btk_window_set_has_toplevel_focus (window, TRUE);
      switch (detail)
	{
	case XEMBED_FOCUS_FIRST:
	  _btk_plug_focus_first_last (plug, BTK_DIR_TAB_FORWARD);
	  break;
	case XEMBED_FOCUS_LAST:
	  _btk_plug_focus_first_last (plug, BTK_DIR_TAB_BACKWARD);
	  break;
	case XEMBED_FOCUS_CURRENT:
	  break;
	}
      break;

    case XEMBED_FOCUS_OUT:
      _btk_window_set_has_toplevel_focus (window, FALSE);
      break;
      
    case XEMBED_GRAB_KEY:
    case XEMBED_UNGRAB_KEY:
    case XEMBED_BTK_GRAB_KEY:
    case XEMBED_BTK_UNGRAB_KEY:
    case XEMBED_REQUEST_FOCUS:
    case XEMBED_FOCUS_NEXT:
    case XEMBED_FOCUS_PREV:
      g_warning ("BtkPlug: Invalid _XEMBED message %s received", _btk_xembed_message_name (message));
      break;
      
    default:
      BTK_NOTE(PLUGSOCKET,
	       g_message ("BtkPlug: Ignoring unknown _XEMBED message of type %d", message));
      break;
    }
}

BdkFilterReturn
_btk_plug_windowing_filter_func (BdkXEvent *bdk_xevent,
				 BdkEvent  *event,
				 gpointer   data)
{
  BdkScreen *screen = bdk_window_get_screen (event->any.window);
  BdkDisplay *display = bdk_screen_get_display (screen);
  BtkPlug *plug = BTK_PLUG (data);
  XEvent *xevent = (XEvent *)bdk_xevent;

  BdkFilterReturn return_val;
  
  return_val = BDK_FILTER_CONTINUE;

  switch (xevent->type)
    {
    case ClientMessage:
      if (xevent->xclient.message_type == bdk_x11_get_xatom_by_name_for_display (display, "_XEMBED"))
	{
	  _btk_xembed_push_message (xevent);
	  handle_xembed_message (plug,
				 xevent->xclient.data.l[1],
				 xevent->xclient.data.l[2],
				 xevent->xclient.data.l[3],
				 xevent->xclient.data.l[4],
				 xevent->xclient.data.l[0]);
	  _btk_xembed_pop_message ();

	  return_val = BDK_FILTER_REMOVE;
	}
      else if (xevent->xclient.message_type == bdk_x11_get_xatom_by_name_for_display (display, "WM_DELETE_WINDOW"))
	{
	  /* We filter these out because we take being reparented back to the
	   * root window as the reliable end of the embedding protocol
	   */

	  return_val = BDK_FILTER_REMOVE;
	}
      break;
    case ReparentNotify:
      {
	XReparentEvent *xre = &xevent->xreparent;
	gboolean was_embedded = plug->socket_window != NULL;

	BTK_NOTE (PLUGSOCKET, g_message("BtkPlug: ReparentNotify received"));

	return_val = BDK_FILTER_REMOVE;
	
	g_object_ref (plug);
	
	if (was_embedded)
	  {
	    /* End of embedding protocol for previous socket */
	    
	    BTK_NOTE (PLUGSOCKET, g_message ("BtkPlug: end of embedding"));
	    /* FIXME: race if we remove from another socket and
	     * then add to a local window before we get notification
	     * Probably need check in _btk_plug_add_to_socket
	     */
	    
	    if (xre->parent != BDK_WINDOW_XWINDOW (plug->socket_window))
	      {
		BtkWidget *widget = BTK_WIDGET (plug);

		bdk_window_set_user_data (plug->socket_window, NULL);
		g_object_unref (plug->socket_window);
		plug->socket_window = NULL;

		/* Emit a delete window, as if the user attempted
		 * to close the toplevel. Simple as to how we
		 * handle WM_DELETE_WINDOW, if it isn't handled
		 * we destroy the widget. BUt only do this if
		 * we are being reparented to the root window.
		 * Moving from one embedder to another should
		 * be invisible to the app.
		 */

		if (xre->parent == BDK_WINDOW_XWINDOW (bdk_screen_get_root_window (screen)))
		  {
		    BTK_NOTE (PLUGSOCKET, g_message ("BtkPlug: calling btk_plug_send_delete_event()"));
		    _btk_plug_send_delete_event (widget);

		    g_object_notify (G_OBJECT (plug), "embedded");
		  }
	      }
	    else
	      goto done;
	  }

	if (xre->parent != BDK_WINDOW_XWINDOW (bdk_screen_get_root_window (screen)))
	  {
	    /* Start of embedding protocol */

	    BTK_NOTE (PLUGSOCKET, g_message ("BtkPlug: start of embedding"));
	    plug->socket_window = bdk_window_lookup_for_display (display, xre->parent);
	    if (plug->socket_window)
	      {
		gpointer user_data = NULL;
		bdk_window_get_user_data (plug->socket_window, &user_data);

		if (user_data)
		  {
		    g_warning (G_STRLOC "Plug reparented unexpectedly into window in the same process");
		    plug->socket_window = NULL;
		    break; /* FIXME: shouldn't this unref the plug? i.e. "goto done;" instead */
		  }

		g_object_ref (plug->socket_window);
	      }
	    else
	      {
		plug->socket_window = bdk_window_foreign_new_for_display (display, xre->parent);
		if (!plug->socket_window) /* Already gone */
		  break; /* FIXME: shouldn't this unref the plug? i.e. "goto done;" instead */
	      }

	    _btk_plug_add_all_grabbed_keys (plug);

	    if (!was_embedded)
	      g_signal_emit_by_name (plug, "embedded");

	    g_object_notify (G_OBJECT (plug), "embedded");
	  }

      done:
	g_object_unref (plug);
	
	break;
      }
    }

  return return_val;
}
