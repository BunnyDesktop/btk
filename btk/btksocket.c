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
#include "btkintl.h"

#include "btkalias.h"

/**
 * SECTION:btksocket
 * @Short_description: Container for widgets from other processes
 * @Title: BtkSocket
 * @See_also: #BtkPlug, <ulink url="http://www.freedesktop.org/Standards/xembed-spec">XEmbed</ulink>
 *
 * Together with #BtkPlug, #BtkSocket provides the ability
 * to embed widgets from one process into another process
 * in a fashion that is transparent to the user. One
 * process creates a #BtkSocket widget and passes
 * that widget's window ID to the other process,
 * which then creates a #BtkPlug with that window ID.
 * Any widgets contained in the #BtkPlug then will appear
 * inside the first application's window.
 *
 * The socket's window ID is obtained by using
 * btk_socket_get_id(). Before using this function,
 * the socket must have been realized, and for hence,
 * have been added to its parent.
 *
 * <example>
 * <title>Obtaining the window ID of a socket.</title>
 * <programlisting>
 * BtkWidget *socket = btk_socket_new (<!-- -->);
 * btk_widget_show (socket);
 * btk_container_add (BTK_CONTAINER (parent), socket);
 *
 * /<!---->* The following call is only necessary if one of
 *  * the ancestors of the socket is not yet visible.
 *  *<!---->/
 * btk_widget_realize (socket);
 * g_print ("The ID of the sockets window is %#x\n",
 *          btk_socket_get_id (socket));
 * </programlisting>
 * </example>
 *
 * Note that if you pass the window ID of the socket to another
 * process that will create a plug in the socket, you
 * must make sure that the socket widget is not destroyed
 * until that plug is created. Violating this rule will
 * cause unpredictable consequences, the most likely
 * consequence being that the plug will appear as a
 * separate toplevel window. You can check if the plug
 * has been created by using btk_socket_get_plug_window(). If
 * it returns a non-%NULL value, then the plug has been
 * successfully created inside of the socket.
 *
 * When BTK+ is notified that the embedded window has been
 * destroyed, then it will destroy the socket as well. You
 * should always, therefore, be prepared for your sockets
 * to be destroyed at any time when the main event loop
 * is running. To prevent this from happening, you can
 * connect to the #BtkSocket::plug-removed signal.
 *
 * The communication between a #BtkSocket and a #BtkPlug follows the
 * <ulink url="http://www.freedesktop.org/Standards/xembed-spec">XEmbed</ulink>
 * protocol. This protocol has also been implemented in other toolkits, e.g.
 * <application>Qt</application>, allowing the same level of integration
 * when embedding a <application>Qt</application> widget in BTK or vice versa.
 *
 * A socket can also be used to swallow arbitrary
 * pre-existing top-level windows using btk_socket_steal(),
 * though the integration when this is done will not be as close
 * as between a #BtkPlug and a #BtkSocket.
 *
 * <note>
 * The #BtkPlug and #BtkSocket widgets are currently not available
 * on all platforms supported by BTK+.
 * </note>
 */

/* Forward declararations */

static void     btk_socket_finalize             (BObject          *object);
static void     btk_socket_notify               (BObject          *object,
						 BParamSpec       *pspec);
static void     btk_socket_realize              (BtkWidget        *widget);
static void     btk_socket_unrealize            (BtkWidget        *widget);
static void     btk_socket_size_request         (BtkWidget        *widget,
						 BtkRequisition   *requisition);
static void     btk_socket_size_allocate        (BtkWidget        *widget,
						 BtkAllocation    *allocation);
static void     btk_socket_hierarchy_changed    (BtkWidget        *widget,
						 BtkWidget        *old_toplevel);
static void     btk_socket_grab_notify          (BtkWidget        *widget,
						 bboolean          was_grabbed);
static bboolean btk_socket_key_event            (BtkWidget        *widget,
						 BdkEventKey      *event);
static bboolean btk_socket_focus                (BtkWidget        *widget,
						 BtkDirectionType  direction);
static void     btk_socket_remove               (BtkContainer     *container,
						 BtkWidget        *widget);
static void     btk_socket_forall               (BtkContainer     *container,
						 bboolean          include_internals,
						 BtkCallback       callback,
						 bpointer          callback_data);


/* Local data */

typedef struct
{
  buint			 accel_key;
  BdkModifierType	 accel_mods;
} GrabbedKey;

enum {
  PLUG_ADDED,
  PLUG_REMOVED,
  LAST_SIGNAL
}; 

static buint socket_signals[LAST_SIGNAL] = { 0 };

/*
 * _btk_socket_get_private:
 *
 * @socket: a #BtkSocket
 *
 * Returns the private data associated with a BtkSocket, creating it
 * first if necessary.
 */
BtkSocketPrivate *
_btk_socket_get_private (BtkSocket *socket)
{
  return B_TYPE_INSTANCE_GET_PRIVATE (socket, BTK_TYPE_SOCKET, BtkSocketPrivate);
}

G_DEFINE_TYPE (BtkSocket, btk_socket, BTK_TYPE_CONTAINER)

static void
btk_socket_finalize (BObject *object)
{
  BtkSocket *socket = BTK_SOCKET (object);
  
  g_object_unref (socket->accel_group);
  socket->accel_group = NULL;

  B_OBJECT_CLASS (btk_socket_parent_class)->finalize (object);
}

static void
btk_socket_class_init (BtkSocketClass *class)
{
  BtkWidgetClass *widget_class;
  BtkContainerClass *container_class;
  BObjectClass *bobject_class;

  bobject_class = (BObjectClass *) class;
  widget_class = (BtkWidgetClass*) class;
  container_class = (BtkContainerClass*) class;

  bobject_class->finalize = btk_socket_finalize;
  bobject_class->notify = btk_socket_notify;

  widget_class->realize = btk_socket_realize;
  widget_class->unrealize = btk_socket_unrealize;
  widget_class->size_request = btk_socket_size_request;
  widget_class->size_allocate = btk_socket_size_allocate;
  widget_class->hierarchy_changed = btk_socket_hierarchy_changed;
  widget_class->grab_notify = btk_socket_grab_notify;
  widget_class->key_press_event = btk_socket_key_event;
  widget_class->key_release_event = btk_socket_key_event;
  widget_class->focus = btk_socket_focus;

  /* We don't want to show_all/hide_all the in-process
   * plug, if any.
   */
  widget_class->show_all = btk_widget_show;
  widget_class->hide_all = btk_widget_hide;
  
  container_class->remove = btk_socket_remove;
  container_class->forall = btk_socket_forall;

  /**
   * BtkSocket::plug-added:
   * @socket_: the object which received the signal
   *
   * This signal is emitted when a client is successfully
   * added to the socket. 
   */
  socket_signals[PLUG_ADDED] =
    g_signal_new (I_("plug-added"),
		  B_OBJECT_CLASS_TYPE (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkSocketClass, plug_added),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);

  /**
   * BtkSocket::plug-removed:
   * @socket_: the object which received the signal
   *
   * This signal is emitted when a client is removed from the socket. 
   * The default action is to destroy the #BtkSocket widget, so if you 
   * want to reuse it you must add a signal handler that returns %TRUE. 
   *
   * Return value: %TRUE to stop other handlers from being invoked.
   */
  socket_signals[PLUG_REMOVED] =
    g_signal_new (I_("plug-removed"),
		  B_OBJECT_CLASS_TYPE (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkSocketClass, plug_removed),
                  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__VOID,
		  B_TYPE_BOOLEAN, 0);

  g_type_class_add_private (bobject_class, sizeof (BtkSocketPrivate));
}

static void
btk_socket_init (BtkSocket *socket)
{
  socket->request_width = 0;
  socket->request_height = 0;
  socket->current_width = 0;
  socket->current_height = 0;
  
  socket->plug_window = NULL;
  socket->plug_widget = NULL;
  socket->focus_in = FALSE;
  socket->have_size = FALSE;
  socket->need_map = FALSE;
  socket->active = FALSE;

  socket->accel_group = btk_accel_group_new ();
  g_object_set_data (B_OBJECT (socket->accel_group), I_("btk-socket"), socket);
}

/**
 * btk_socket_new:
 * 
 * Create a new empty #BtkSocket.
 * 
 * Return value:  the new #BtkSocket.
 **/
BtkWidget*
btk_socket_new (void)
{
  BtkSocket *socket;

  socket = g_object_new (BTK_TYPE_SOCKET, NULL);

  return BTK_WIDGET (socket);
}

/**
 * btk_socket_steal:
 * @socket_: a #BtkSocket
 * @wid: the window ID of an existing toplevel window.
 * 
 * Reparents a pre-existing toplevel window into a #BtkSocket. This is
 * meant to embed clients that do not know about embedding into a
 * #BtkSocket, however doing so is inherently unreliable, and using
 * this function is not recommended.
 *
 * The #BtkSocket must have already be added into a toplevel window
 *  before you can make this call.
 **/
void           
btk_socket_steal (BtkSocket      *socket,
		  BdkNativeWindow wid)
{
  g_return_if_fail (BTK_IS_SOCKET (socket));
  g_return_if_fail (BTK_WIDGET_ANCHORED (socket));

  if (!btk_widget_get_realized (BTK_WIDGET (socket)))
    btk_widget_realize (BTK_WIDGET (socket));

  _btk_socket_add_window (socket, wid, TRUE);
}

/**
 * btk_socket_add_id:
 * @socket_: a #BtkSocket
 * @window_id: the window ID of a client participating in the XEMBED protocol.
 *
 * Adds an XEMBED client, such as a #BtkPlug, to the #BtkSocket.  The
 * client may be in the same process or in a different process. 
 * 
 * To embed a #BtkPlug in a #BtkSocket, you can either create the
 * #BtkPlug with <literal>btk_plug_new (0)</literal>, call 
 * btk_plug_get_id() to get the window ID of the plug, and then pass that to the
 * btk_socket_add_id(), or you can call btk_socket_get_id() to get the
 * window ID for the socket, and call btk_plug_new() passing in that
 * ID.
 *
 * The #BtkSocket must have already be added into a toplevel window
 *  before you can make this call.
 **/
void           
btk_socket_add_id (BtkSocket      *socket,
		   BdkNativeWindow window_id)
{
  g_return_if_fail (BTK_IS_SOCKET (socket));
  g_return_if_fail (BTK_WIDGET_ANCHORED (socket));

  if (!btk_widget_get_realized (BTK_WIDGET (socket)))
    btk_widget_realize (BTK_WIDGET (socket));

  _btk_socket_add_window (socket, window_id, TRUE);
}

/**
 * btk_socket_get_id:
 * @socket_: a #BtkSocket.
 * 
 * Gets the window ID of a #BtkSocket widget, which can then
 * be used to create a client embedded inside the socket, for
 * instance with btk_plug_new(). 
 *
 * The #BtkSocket must have already be added into a toplevel window 
 * before you can make this call.
 * 
 * Return value: the window ID for the socket
 **/
BdkNativeWindow
btk_socket_get_id (BtkSocket *socket)
{
  g_return_val_if_fail (BTK_IS_SOCKET (socket), 0);
  g_return_val_if_fail (BTK_WIDGET_ANCHORED (socket), 0);

  if (!btk_widget_get_realized (BTK_WIDGET (socket)))
    btk_widget_realize (BTK_WIDGET (socket));

  return _btk_socket_windowing_get_id (socket);
}

/**
 * btk_socket_get_plug_window:
 * @socket_: a #BtkSocket.
 *
 * Retrieves the window of the plug. Use this to check if the plug has
 * been created inside of the socket.
 *
 * Return value: (transfer none): the window of the plug if available, or %NULL
 *
 * Since:  2.14
 **/
BdkWindow*
btk_socket_get_plug_window (BtkSocket *socket)
{
  g_return_val_if_fail (BTK_IS_SOCKET (socket), NULL);

  return socket->plug_window;
}

static void
btk_socket_realize (BtkWidget *widget)
{
  BtkSocket *socket = BTK_SOCKET (widget);
  BdkWindowAttr attributes;
  bint attributes_mask;

  btk_widget_set_realized (widget, TRUE);

  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  attributes.event_mask = BDK_FOCUS_CHANGE_MASK;

  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;

  widget->window = bdk_window_new (btk_widget_get_parent_window (widget), 
				   &attributes, attributes_mask);
  bdk_window_set_user_data (widget->window, socket);

  widget->style = btk_style_attach (widget->style, widget->window);
  btk_style_set_background (widget->style, widget->window, BTK_STATE_NORMAL);

  _btk_socket_windowing_realize_window (socket);

  bdk_window_add_filter (widget->window,
			 _btk_socket_windowing_filter_func,
			 widget);

  /* We sync here so that we make sure that if the XID for
   * our window is passed to another application, SubstructureRedirectMask
   * will be set by the time the other app creates its window.
   */
  bdk_display_sync (btk_widget_get_display (widget));
}

/**
 * _btk_socket_end_embedding:
 *
 * @socket: a #BtkSocket
 *
 * Called to end the embedding of a plug in the socket.
 */
void
_btk_socket_end_embedding (BtkSocket *socket)
{
  BtkSocketPrivate *private = _btk_socket_get_private (socket);
  BtkWidget *toplevel = btk_widget_get_toplevel (BTK_WIDGET (socket));
  
  if (BTK_IS_WINDOW (toplevel))
    _btk_socket_windowing_end_embedding_toplevel (socket);

  g_object_unref (socket->plug_window);
  socket->plug_window = NULL;
  socket->current_width = 0;
  socket->current_height = 0;
  private->resize_count = 0;

  btk_accel_group_disconnect (socket->accel_group, NULL);
}

static void
btk_socket_unrealize (BtkWidget *widget)
{
  BtkSocket *socket = BTK_SOCKET (widget);

  btk_widget_set_realized (widget, FALSE);

  if (socket->plug_widget)
    {
      _btk_plug_remove_from_socket (BTK_PLUG (socket->plug_widget), socket);
    }
  else if (socket->plug_window)
    {
      _btk_socket_end_embedding (socket);
    }

  BTK_WIDGET_CLASS (btk_socket_parent_class)->unrealize (widget);
}

static void
btk_socket_size_request (BtkWidget      *widget,
			 BtkRequisition *requisition)
{
  BtkSocket *socket = BTK_SOCKET (widget);

  if (socket->plug_widget)
    {
      btk_widget_size_request (socket->plug_widget, requisition);
    }
  else
    {
      if (socket->is_mapped && !socket->have_size && socket->plug_window)
	_btk_socket_windowing_size_request (socket);

      if (socket->is_mapped && socket->have_size)
	{
	  requisition->width = MAX (socket->request_width, 1);
	  requisition->height = MAX (socket->request_height, 1);
	}
      else
	{
	  requisition->width = 1;
	  requisition->height = 1;
	}
    }
}

static void
btk_socket_size_allocate (BtkWidget     *widget,
			  BtkAllocation *allocation)
{
  BtkSocket *socket = BTK_SOCKET (widget);

  widget->allocation = *allocation;
  if (btk_widget_get_realized (widget))
    {
      bdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);

      if (socket->plug_widget)
	{
	  BtkAllocation child_allocation;

	  child_allocation.x = 0;
	  child_allocation.y = 0;
	  child_allocation.width = allocation->width;
	  child_allocation.height = allocation->height;

	  btk_widget_size_allocate (socket->plug_widget, &child_allocation);
	}
      else if (socket->plug_window)
	{
	  BtkSocketPrivate *private = _btk_socket_get_private (socket);
	  
	  bdk_error_trap_push ();
	  
	  if (allocation->width != socket->current_width ||
	      allocation->height != socket->current_height)
	    {
	      bdk_window_move_resize (socket->plug_window,
				      0, 0,
				      allocation->width, allocation->height);
	      if (private->resize_count)
		private->resize_count--;
	      
	      BTK_NOTE (PLUGSOCKET,
			g_message ("BtkSocket - allocated: %d %d",
				   allocation->width, allocation->height));
	      socket->current_width = allocation->width;
	      socket->current_height = allocation->height;
	    }

	  if (socket->need_map)
	    {
	      bdk_window_show (socket->plug_window);
	      socket->need_map = FALSE;
	    }

	  while (private->resize_count)
 	    {
 	      _btk_socket_windowing_send_configure_event (socket);
 	      private->resize_count--;
 	      BTK_NOTE (PLUGSOCKET,
			g_message ("BtkSocket - sending synthetic configure: %d %d",
				   allocation->width, allocation->height));
 	    }
	  
	  bdk_display_sync (btk_widget_get_display (widget));
	  bdk_error_trap_pop ();
	}
    }
}

static bboolean
activate_key (BtkAccelGroup  *accel_group,
	      BObject        *acceleratable,
	      buint           accel_key,
	      BdkModifierType accel_mods,
	      GrabbedKey     *grabbed_key)
{
  BdkEvent *bdk_event = btk_get_current_event ();
  
  BtkSocket *socket = g_object_get_data (B_OBJECT (accel_group), "btk-socket");
  bboolean retval = FALSE;

  if (bdk_event && bdk_event->type == BDK_KEY_PRESS && socket->plug_window)
    {
      _btk_socket_windowing_send_key_event (socket, bdk_event, TRUE);
      retval = TRUE;
    }

  if (bdk_event)
    bdk_event_free (bdk_event);

  return retval;
}

static bboolean
find_accel_key (BtkAccelKey *key,
		GClosure    *closure,
		bpointer     data)
{
  GrabbedKey *grabbed_key = data;
  
  return (key->accel_key == grabbed_key->accel_key &&
	  key->accel_mods == grabbed_key->accel_mods);
}

/**
 * _btk_socket_add_grabbed_key:
 *
 * @socket: a #BtkSocket
 * @keyval: a key
 * @modifiers: modifiers for the key
 *
 * Called from the BtkSocket platform-specific backend when the
 * corresponding plug has told the socket to grab a key.
 */
void
_btk_socket_add_grabbed_key (BtkSocket       *socket,
			     buint            keyval,
			     BdkModifierType  modifiers)
{
  GClosure *closure;
  GrabbedKey *grabbed_key;

  grabbed_key = g_new (GrabbedKey, 1);
  
  grabbed_key->accel_key = keyval;
  grabbed_key->accel_mods = modifiers;

  if (btk_accel_group_find (socket->accel_group,
			    find_accel_key,
			    &grabbed_key))
    {
      g_warning ("BtkSocket: request to add already present grabbed key %u,%#x\n",
		 keyval, modifiers);
      g_free (grabbed_key);
      return;
    }

  closure = g_cclosure_new (G_CALLBACK (activate_key), grabbed_key, (GClosureNotify)g_free);

  btk_accel_group_connect (socket->accel_group, keyval, modifiers, BTK_ACCEL_LOCKED,
			   closure);
}

/**
 * _btk_socket_remove_grabbed_key:
 *
 * @socket: a #BtkSocket
 * @keyval: a key
 * @modifiers: modifiers for the key
 *
 * Called from the BtkSocket backend when the corresponding plug has
 * told the socket to remove a key grab.
 */
void
_btk_socket_remove_grabbed_key (BtkSocket      *socket,
				buint           keyval,
				BdkModifierType modifiers)
{
  if (!btk_accel_group_disconnect_key (socket->accel_group, keyval, modifiers))
    g_warning ("BtkSocket: request to remove non-present grabbed key %u,%#x\n",
	       keyval, modifiers);
}

static void
socket_update_focus_in (BtkSocket *socket)
{
  bboolean focus_in = FALSE;

  if (socket->plug_window)
    {
      BtkWidget *toplevel = btk_widget_get_toplevel (BTK_WIDGET (socket));

      if (btk_widget_is_toplevel (toplevel) &&
	  BTK_WINDOW (toplevel)->has_toplevel_focus &&
	  btk_widget_is_focus (BTK_WIDGET (socket)))
	focus_in = TRUE;
    }

  if (focus_in != socket->focus_in)
    {
      socket->focus_in = focus_in;

      _btk_socket_windowing_focus_change (socket, focus_in);
    }
}

static void
socket_update_active (BtkSocket *socket)
{
  bboolean active = FALSE;

  if (socket->plug_window)
    {
      BtkWidget *toplevel = btk_widget_get_toplevel (BTK_WIDGET (socket));

      if (btk_widget_is_toplevel (toplevel) &&
	  BTK_WINDOW (toplevel)->is_active)
	active = TRUE;
    }

  if (active != socket->active)
    {
      socket->active = active;

      _btk_socket_windowing_update_active (socket, active);
    }
}

static void
btk_socket_hierarchy_changed (BtkWidget *widget,
			      BtkWidget *old_toplevel)
{
  BtkSocket *socket = BTK_SOCKET (widget);
  BtkWidget *toplevel = btk_widget_get_toplevel (widget);

  if (toplevel && !BTK_IS_WINDOW (toplevel))
    toplevel = NULL;

  if (toplevel != socket->toplevel)
    {
      if (socket->toplevel)
	{
	  btk_window_remove_accel_group (BTK_WINDOW (socket->toplevel), socket->accel_group);
	  g_signal_handlers_disconnect_by_func (socket->toplevel,
						socket_update_focus_in,
						socket);
	  g_signal_handlers_disconnect_by_func (socket->toplevel,
						socket_update_active,
						socket);
	}

      socket->toplevel = toplevel;

      if (toplevel)
	{
	  btk_window_add_accel_group (BTK_WINDOW (socket->toplevel), socket->accel_group);
	  g_signal_connect_swapped (socket->toplevel, "notify::has-toplevel-focus",
				    G_CALLBACK (socket_update_focus_in), socket);
	  g_signal_connect_swapped (socket->toplevel, "notify::is-active",
				    G_CALLBACK (socket_update_active), socket);
	}

      socket_update_focus_in (socket);
      socket_update_active (socket);
    }
}

static void
btk_socket_grab_notify (BtkWidget *widget,
			bboolean   was_grabbed)
{
  BtkSocket *socket = BTK_SOCKET (widget);

  if (!socket->same_app)
    _btk_socket_windowing_update_modality (socket, !was_grabbed);
}

static bboolean
btk_socket_key_event (BtkWidget   *widget,
                      BdkEventKey *event)
{
  BtkSocket *socket = BTK_SOCKET (widget);
  
  if (btk_widget_has_focus (widget) && socket->plug_window && !socket->plug_widget)
    {
      _btk_socket_windowing_send_key_event (socket, (BdkEvent *) event, FALSE);

      return TRUE;
    }
  else
    return FALSE;
}

static void
btk_socket_notify (BObject    *object,
		   BParamSpec *pspec)
{
  if (!strcmp (pspec->name, "is-focus"))
    return;
  socket_update_focus_in (BTK_SOCKET (object));
}

/**
 * _btk_socket_claim_focus:
 *
 * @socket: a #BtkSocket
 * @send_event: huh?
 *
 * Claims focus for the socket. XXX send_event?
 */
void
_btk_socket_claim_focus (BtkSocket *socket,
			 bboolean   send_event)
{
  BtkWidget *widget = BTK_WIDGET (socket);

  if (!send_event)
    socket->focus_in = TRUE;	/* Otherwise, our notify handler will send FOCUS_IN  */
      
  /* Oh, the trickery... */
  
  btk_widget_set_can_focus (widget, TRUE);
  btk_widget_grab_focus (widget);
  btk_widget_set_can_focus (widget, FALSE);
}

static bboolean
btk_socket_focus (BtkWidget       *widget,
		  BtkDirectionType direction)
{
  BtkSocket *socket = BTK_SOCKET (widget);

  if (socket->plug_widget)
    return btk_widget_child_focus (socket->plug_widget, direction);

  if (!btk_widget_is_focus (widget))
    {
      _btk_socket_windowing_focus (socket, direction);
      _btk_socket_claim_focus (socket, FALSE);
 
      return TRUE;
    }
  else
    return FALSE;
}

static void
btk_socket_remove (BtkContainer *container,
		   BtkWidget    *child)
{
  BtkSocket *socket = BTK_SOCKET (container);

  g_return_if_fail (child == socket->plug_widget);

  _btk_plug_remove_from_socket (BTK_PLUG (socket->plug_widget), socket);
}

static void
btk_socket_forall (BtkContainer *container,
		   bboolean      include_internals,
		   BtkCallback   callback,
		   bpointer      callback_data)
{
  BtkSocket *socket = BTK_SOCKET (container);

  if (socket->plug_widget)
    (* callback) (socket->plug_widget, callback_data);
}

/**
 * _btk_socket_add_window:
 *
 * @socket: a #BtkSocket
 * @xid: the native identifier for a window
 * @need_reparent: whether the socket's plug's window needs to be
 *		   reparented to the socket
 *
 * Adds a window to a BtkSocket.
 */
void
_btk_socket_add_window (BtkSocket       *socket,
			BdkNativeWindow  xid,
			bboolean         need_reparent)
{
  BtkWidget *widget = BTK_WIDGET (socket);
  BdkDisplay *display = btk_widget_get_display (widget);
  bpointer user_data = NULL;
  
  socket->plug_window = bdk_window_lookup_for_display (display, xid);

  if (socket->plug_window)
    {
      g_object_ref (socket->plug_window);
      bdk_window_get_user_data (socket->plug_window, &user_data);
    }

  if (user_data)		/* A widget's window in this process */
    {
      BtkWidget *child_widget = user_data;

      if (!BTK_IS_PLUG (child_widget))
	{
	  g_warning (B_STRLOC ": Can't add non-BtkPlug to BtkSocket");
	  socket->plug_window = NULL;
	  bdk_error_trap_pop ();
	  
	  return;
	}

      _btk_plug_add_to_socket (BTK_PLUG (child_widget), socket);
    }
  else				/* A foreign window */
    {
      BtkWidget *toplevel;
      BdkDragProtocol protocol;

      bdk_error_trap_push ();

      if (!socket->plug_window)
	{  
	  socket->plug_window = bdk_window_foreign_new_for_display (display, xid);
	  if (!socket->plug_window) /* was deleted before we could get it */
	    {
	      bdk_error_trap_pop ();
	      return;
	    }
	}
	
      _btk_socket_windowing_select_plug_window_input (socket);

      if (bdk_error_trap_pop ())
	{
	  g_object_unref (socket->plug_window);
	  socket->plug_window = NULL;
	  return;
	}
      
      /* OK, we now will reliably get destroy notification on socket->plug_window */

      bdk_error_trap_push ();

      if (need_reparent)
	{
	  bdk_window_hide (socket->plug_window); /* Shouldn't actually be necessary for XEMBED, but just in case */
	  bdk_window_reparent (socket->plug_window, widget->window, 0, 0);
	}

      socket->have_size = FALSE;

      _btk_socket_windowing_embed_get_info (socket);

      socket->need_map = socket->is_mapped;

      if (bdk_drag_get_protocol_for_display (display, xid, &protocol))
	btk_drag_dest_set_proxy (BTK_WIDGET (socket), socket->plug_window, 
				 protocol, TRUE);

      bdk_display_sync (display);
      bdk_error_trap_pop ();

      bdk_window_add_filter (socket->plug_window,
			     _btk_socket_windowing_filter_func,
			     socket);

      /* Add a pointer to the socket on our toplevel window */

      toplevel = btk_widget_get_toplevel (BTK_WIDGET (socket));
      if (BTK_IS_WINDOW (toplevel))
	btk_window_add_embedded_xid (BTK_WINDOW (toplevel), xid);

      _btk_socket_windowing_embed_notify (socket);

      socket_update_active (socket);
      socket_update_focus_in (socket);

      btk_widget_queue_resize (BTK_WIDGET (socket));
    }

  if (socket->plug_window)
    g_signal_emit (socket, socket_signals[PLUG_ADDED], 0);
}

/**
 * _btk_socket_handle_map_request:
 *
 * @socket: a #BtkSocket
 *
 * Called from the BtkSocket backend when the plug has been mapped.
 */
void
_btk_socket_handle_map_request (BtkSocket *socket)
{
  if (!socket->is_mapped)
    {
      socket->is_mapped = TRUE;
      socket->need_map = TRUE;

      btk_widget_queue_resize (BTK_WIDGET (socket));
    }
}

/**
 * _btk_socket_unmap_notify:
 *
 * @socket: a #BtkSocket
 *
 * Called from the BtkSocket backend when the plug has been unmapped ???
 */
void
_btk_socket_unmap_notify (BtkSocket *socket)
{
  if (socket->is_mapped)
    {
      socket->is_mapped = FALSE;
      btk_widget_queue_resize (BTK_WIDGET (socket));
    }
}

/**
 * _btk_socket_advance_toplevel_focus:
 *
 * @socket: a #BtkSocket
 * @direction: a direction
 *
 * Called from the BtkSocket backend when the corresponding plug
 * has told the socket to move the focus.
 */
void
_btk_socket_advance_toplevel_focus (BtkSocket        *socket,
				    BtkDirectionType  direction)
{
  BtkBin *bin;
  BtkWindow *window;
  BtkContainer *container;
  BtkWidget *toplevel;
  BtkWidget *old_focus_child;
  BtkWidget *parent;

  toplevel = btk_widget_get_toplevel (BTK_WIDGET (socket));
  if (!toplevel)
    return;

  if (!btk_widget_is_toplevel (toplevel) || BTK_IS_PLUG (toplevel))
    {
      btk_widget_child_focus (toplevel,direction);
      return;
    }

  container = BTK_CONTAINER (toplevel);
  window = BTK_WINDOW (toplevel);
  bin = BTK_BIN (toplevel);

  /* This is a copy of btk_window_focus(), modified so that we
   * can detect wrap-around.
   */
  old_focus_child = container->focus_child;
  
  if (old_focus_child)
    {
      if (btk_widget_child_focus (old_focus_child, direction))
	return;

      /* We are allowed exactly one wrap-around per sequence of focus
       * events
       */
      if (_btk_socket_windowing_embed_get_focus_wrapped ())
	return;
      else
	_btk_socket_windowing_embed_set_focus_wrapped ();
    }

  if (window->focus_widget)
    {
      /* Wrapped off the end, clear the focus setting for the toplevel */
      parent = window->focus_widget->parent;
      while (parent)
	{
	  btk_container_set_focus_child (BTK_CONTAINER (parent), NULL);
	  parent = BTK_WIDGET (parent)->parent;
	}
      
      btk_window_set_focus (BTK_WINDOW (container), NULL);
    }

  /* Now try to focus the first widget in the window */
  if (bin->child)
    {
      if (btk_widget_child_focus (bin->child, direction))
        return;
    }
}

#define __BTK_SOCKET_C__
#include "btkaliasdef.c"
