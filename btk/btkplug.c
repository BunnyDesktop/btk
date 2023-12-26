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

#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkplug.h"
#include "btkintl.h"
#include "btkprivate.h"
#include "btkplugprivate.h"

#include "btkalias.h"

/**
 * SECTION:btkplug
 * @Short_description: Toplevel for embedding into other processes
 * @Title: BtkPlug
 * @See_also: #BtkSocket
 *
 * Together with #BtkSocket, #BtkPlug provides the ability
 * to embed widgets from one process into another process
 * in a fashion that is transparent to the user. One
 * process creates a #BtkSocket widget and passes the
 * ID of that widget's window to the other process,
 * which then creates a #BtkPlug with that window ID.
 * Any widgets contained in the #BtkPlug then will appear
 * inside the first application's window.
 *
 * <note>
 * The #BtkPlug and #BtkSocket widgets are currently not available
 * on all platforms supported by BTK+.
 * </note>
 */

static void            btk_plug_get_property          (GObject     *object,
						       guint        prop_id,
						       GValue      *value,
						       GParamSpec  *pspec);
static void            btk_plug_finalize              (GObject          *object);
static void            btk_plug_realize               (BtkWidget        *widget);
static void            btk_plug_unrealize             (BtkWidget        *widget);
static void            btk_plug_show                  (BtkWidget        *widget);
static void            btk_plug_hide                  (BtkWidget        *widget);
static void            btk_plug_map                   (BtkWidget        *widget);
static void            btk_plug_unmap                 (BtkWidget        *widget);
static void            btk_plug_size_allocate         (BtkWidget        *widget,
						       BtkAllocation    *allocation);
static gboolean        btk_plug_key_press_event       (BtkWidget        *widget,
						       BdkEventKey      *event);
static gboolean        btk_plug_focus_event           (BtkWidget        *widget,
						       BdkEventFocus    *event);
static void            btk_plug_set_focus             (BtkWindow        *window,
						       BtkWidget        *focus);
static gboolean        btk_plug_focus                 (BtkWidget        *widget,
						       BtkDirectionType  direction);
static void            btk_plug_check_resize          (BtkContainer     *container);
static void            btk_plug_keys_changed          (BtkWindow        *window);

static BtkBinClass *bin_class = NULL;

typedef struct
{
  guint			 accelerator_key;
  BdkModifierType	 accelerator_mods;
} GrabbedKey;

enum {
  PROP_0,
  PROP_EMBEDDED,
  PROP_SOCKET_WINDOW
};

enum {
  EMBEDDED,
  LAST_SIGNAL
}; 

static guint plug_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BtkPlug, btk_plug, BTK_TYPE_WINDOW)

static void
btk_plug_get_property (GObject    *object,
		       guint       prop_id,
		       GValue     *value,
		       GParamSpec *pspec)
{
  BtkPlug *plug = BTK_PLUG (object);

  switch (prop_id)
    {
    case PROP_EMBEDDED:
      g_value_set_boolean (value, plug->socket_window != NULL);
      break;
    case PROP_SOCKET_WINDOW:
      g_value_set_object (value, plug->socket_window);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_plug_class_init (BtkPlugClass *class)
{
  GObjectClass *bobject_class = (GObjectClass *)class;
  BtkWidgetClass *widget_class = (BtkWidgetClass *)class;
  BtkWindowClass *window_class = (BtkWindowClass *)class;
  BtkContainerClass *container_class = (BtkContainerClass *)class;

  bin_class = g_type_class_peek (BTK_TYPE_BIN);

  bobject_class->get_property = btk_plug_get_property;
  bobject_class->finalize = btk_plug_finalize;
  
  widget_class->realize = btk_plug_realize;
  widget_class->unrealize = btk_plug_unrealize;
  widget_class->key_press_event = btk_plug_key_press_event;
  widget_class->focus_in_event = btk_plug_focus_event;
  widget_class->focus_out_event = btk_plug_focus_event;

  widget_class->show = btk_plug_show;
  widget_class->hide = btk_plug_hide;
  widget_class->map = btk_plug_map;
  widget_class->unmap = btk_plug_unmap;
  widget_class->size_allocate = btk_plug_size_allocate;

  widget_class->focus = btk_plug_focus;

  container_class->check_resize = btk_plug_check_resize;

  window_class->set_focus = btk_plug_set_focus;
  window_class->keys_changed = btk_plug_keys_changed;

  /**
   * BtkPlug:embedded:
   *
   * %TRUE if the plug is embedded in a socket.
   *
   * Since: 2.12
   */
  g_object_class_install_property (bobject_class,
				   PROP_EMBEDDED,
				   g_param_spec_boolean ("embedded",
							 P_("Embedded"),
							 P_("Whether or not the plug is embedded"),
							 FALSE,
							 BTK_PARAM_READABLE));

  /**
   * BtkPlug:socket-window:
   *
   * The window of the socket the plug is embedded in.
   *
   * Since: 2.14
   */
  g_object_class_install_property (bobject_class,
				   PROP_SOCKET_WINDOW,
				   g_param_spec_object ("socket-window",
							P_("Socket Window"),
							P_("The window of the socket the plug is embedded in"),
							BDK_TYPE_WINDOW,
							BTK_PARAM_READABLE));

  /**
   * BtkPlug::embedded:
   * @plug: the object on which the signal was emitted
   *
   * Gets emitted when the plug becomes embedded in a socket.
   */ 
  plug_signals[EMBEDDED] =
    g_signal_new (I_("embedded"),
		  G_OBJECT_CLASS_TYPE (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkPlugClass, embedded),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
}

static void
btk_plug_init (BtkPlug *plug)
{
  BtkWindow *window;

  window = BTK_WINDOW (plug);

  window->type = BTK_WINDOW_TOPLEVEL;
}

static void
btk_plug_set_is_child (BtkPlug  *plug,
		       gboolean  is_child)
{
  g_assert (!BTK_WIDGET (plug)->parent);
      
  if (is_child)
    {
      if (plug->modality_window)
	_btk_plug_handle_modality_off (plug);

      if (plug->modality_group)
	{
	  btk_window_group_remove_window (plug->modality_group, BTK_WINDOW (plug));
	  g_object_unref (plug->modality_group);
	  plug->modality_group = NULL;
	}
      
      /* As a toplevel, the MAPPED flag doesn't correspond
       * to whether the widget->window is mapped; we unmap
       * here, but don't bother remapping -- we will get mapped
       * by btk_widget_set_parent ().
       */
      if (btk_widget_get_mapped (BTK_WIDGET (plug)))
	btk_widget_unmap (BTK_WIDGET (plug));
      
      _btk_window_set_is_toplevel (BTK_WINDOW (plug), FALSE);
      btk_container_set_resize_mode (BTK_CONTAINER (plug), BTK_RESIZE_PARENT);

      _btk_widget_propagate_hierarchy_changed (BTK_WIDGET (plug), BTK_WIDGET (plug));
    }
  else
    {
      if (BTK_WINDOW (plug)->focus_widget)
	btk_window_set_focus (BTK_WINDOW (plug), NULL);
      if (BTK_WINDOW (plug)->default_widget)
	btk_window_set_default (BTK_WINDOW (plug), NULL);
	  
      plug->modality_group = btk_window_group_new ();
      btk_window_group_add_window (plug->modality_group, BTK_WINDOW (plug));
      
      _btk_window_set_is_toplevel (BTK_WINDOW (plug), TRUE);
      btk_container_set_resize_mode (BTK_CONTAINER (plug), BTK_RESIZE_QUEUE);

      _btk_widget_propagate_hierarchy_changed (BTK_WIDGET (plug), NULL);
    }
}

/**
 * btk_plug_get_id:
 * @plug: a #BtkPlug.
 * 
 * Gets the window ID of a #BtkPlug widget, which can then
 * be used to embed this window inside another window, for
 * instance with btk_socket_add_id().
 * 
 * Return value: the window ID for the plug
 **/
BdkNativeWindow
btk_plug_get_id (BtkPlug *plug)
{
  g_return_val_if_fail (BTK_IS_PLUG (plug), 0);

  if (!btk_widget_get_realized (BTK_WIDGET (plug)))
    btk_widget_realize (BTK_WIDGET (plug));

  return _btk_plug_windowing_get_id (plug);
}

/**
 * btk_plug_get_embedded:
 * @plug: a #BtkPlug
 *
 * Determines whether the plug is embedded in a socket.
 *
 * Return value: %TRUE if the plug is embedded in a socket
 *
 * Since: 2.14
 **/
gboolean
btk_plug_get_embedded (BtkPlug *plug)
{
  g_return_val_if_fail (BTK_IS_PLUG (plug), FALSE);

  return plug->socket_window != NULL;
}

/**
 * btk_plug_get_socket_window:
 * @plug: a #BtkPlug
 *
 * Retrieves the socket the plug is embedded in.
 *
 * Return value: (transfer none): the window of the socket, or %NULL
 *
 * Since: 2.14
 **/
BdkWindow *
btk_plug_get_socket_window (BtkPlug *plug)
{
  g_return_val_if_fail (BTK_IS_PLUG (plug), NULL);

  return plug->socket_window;
}

/**
 * _btk_plug_add_to_socket:
 * @plug: a #BtkPlug
 * @socket_: a #BtkSocket
 * 
 * Adds a plug to a socket within the same application.
 **/
void
_btk_plug_add_to_socket (BtkPlug   *plug,
			 BtkSocket *socket_)
{
  BtkWidget *widget;
  gint w, h;
  
  g_return_if_fail (BTK_IS_PLUG (plug));
  g_return_if_fail (BTK_IS_SOCKET (socket_));
  g_return_if_fail (btk_widget_get_realized (BTK_WIDGET (socket_)));

  widget = BTK_WIDGET (plug);

  btk_plug_set_is_child (plug, TRUE);
  plug->same_app = TRUE;
  socket_->same_app = TRUE;
  socket_->plug_widget = widget;

  plug->socket_window = BTK_WIDGET (socket_)->window;
  g_object_ref (plug->socket_window);
  g_signal_emit (plug, plug_signals[EMBEDDED], 0);
  g_object_notify (G_OBJECT (plug), "embedded");

  if (btk_widget_get_realized (widget))
    {
      w = bdk_window_get_width (widget->window);
      h = bdk_window_get_height (widget->window);
      bdk_window_reparent (widget->window, plug->socket_window, -w, -h);
    }

  btk_widget_set_parent (widget, BTK_WIDGET (socket_));

  g_signal_emit_by_name (socket_, "plug-added");
}

/**
 * _btk_plug_send_delete_event:
 * @widget: a #BtkWidget
 *
 * Send a BDK_DELETE event to the @widget and destroy it if
 * necessary. Internal BTK function, called from this file or the
 * backend-specific BtkPlug implementation.
 */
void
_btk_plug_send_delete_event (BtkWidget *widget)
{
  BdkEvent *event = bdk_event_new (BDK_DELETE);

  event->any.window = g_object_ref (widget->window);
  event->any.send_event = FALSE;

  g_object_ref (widget);

  if (!btk_widget_event (widget, event))
    btk_widget_destroy (widget);

  g_object_unref (widget);

  bdk_event_free (event);
}

/**
 * _btk_plug_remove_from_socket:
 * @plug: a #BtkPlug
 * @socket_: a #BtkSocket
 * 
 * Removes a plug from a socket within the same application.
 **/
void
_btk_plug_remove_from_socket (BtkPlug   *plug,
			      BtkSocket *socket_)
{
  BtkWidget *widget;
  gboolean result;
  gboolean widget_was_visible;

  g_return_if_fail (BTK_IS_PLUG (plug));
  g_return_if_fail (BTK_IS_SOCKET (socket_));
  g_return_if_fail (btk_widget_get_realized (BTK_WIDGET (plug)));

  widget = BTK_WIDGET (plug);

  if (BTK_WIDGET_IN_REPARENT (widget))
    return;

  g_object_ref (plug);
  g_object_ref (socket_);

  widget_was_visible = btk_widget_get_visible (widget);
  
  bdk_window_hide (widget->window);
  BTK_PRIVATE_SET_FLAG (plug, BTK_IN_REPARENT);
  bdk_window_reparent (widget->window,
		       btk_widget_get_root_window (widget),
		       0, 0);
  btk_widget_unparent (BTK_WIDGET (plug));
  BTK_PRIVATE_UNSET_FLAG (plug, BTK_IN_REPARENT);
  
  socket_->plug_widget = NULL;
  if (socket_->plug_window != NULL)
    {
      g_object_unref (socket_->plug_window);
      socket_->plug_window = NULL;
    }
  
  socket_->same_app = FALSE;

  plug->same_app = FALSE;
  if (plug->socket_window != NULL)
    {
      g_object_unref (plug->socket_window);
      plug->socket_window = NULL;
    }
  btk_plug_set_is_child (plug, FALSE);

  g_signal_emit_by_name (socket_, "plug-removed", &result);
  if (!result)
    btk_widget_destroy (BTK_WIDGET (socket_));

  if (widget->window)
    _btk_plug_send_delete_event (widget);

  g_object_unref (plug);

  if (widget_was_visible && btk_widget_get_visible (BTK_WIDGET (socket_)))
    btk_widget_queue_resize (BTK_WIDGET (socket_));

  g_object_unref (socket_);
}

/**
 * btk_plug_construct:
 * @plug: a #BtkPlug.
 * @socket_id: the XID of the socket's window.
 *
 * Finish the initialization of @plug for a given #BtkSocket identified by
 * @socket_id. This function will generally only be used by classes deriving from #BtkPlug.
 **/
void
btk_plug_construct (BtkPlug         *plug,
		    BdkNativeWindow  socket_id)
{
  btk_plug_construct_for_display (plug, bdk_display_get_default (), socket_id);
}

/**
 * btk_plug_construct_for_display:
 * @plug: a #BtkPlug.
 * @display: the #BdkDisplay associated with @socket_id's 
 *	     #BtkSocket.
 * @socket_id: the XID of the socket's window.
 *
 * Finish the initialization of @plug for a given #BtkSocket identified by
 * @socket_id which is currently displayed on @display.
 * This function will generally only be used by classes deriving from #BtkPlug.
 *
 * Since: 2.2
 **/
void
btk_plug_construct_for_display (BtkPlug         *plug,
				BdkDisplay	*display,
				BdkNativeWindow  socket_id)
{
  if (socket_id)
    {
      gpointer user_data = NULL;

      plug->socket_window = bdk_window_lookup_for_display (display, socket_id);
      if (plug->socket_window)
	{
	  bdk_window_get_user_data (plug->socket_window, &user_data);

	  if (user_data)
	    {
	      if (BTK_IS_SOCKET (user_data))
		_btk_plug_add_to_socket (plug, user_data);
	      else
		{
		  g_warning (B_STRLOC "Can't create BtkPlug as child of non-BtkSocket");
		  plug->socket_window = NULL;
		}
	    }
	  else
	    g_object_ref (plug->socket_window);
	}
      else
	plug->socket_window = bdk_window_foreign_new_for_display (display, socket_id);

      if (plug->socket_window) {
	g_signal_emit (plug, plug_signals[EMBEDDED], 0);

        g_object_notify (G_OBJECT (plug), "embedded");
      }
    }
}

/**
 * btk_plug_new:
 * @socket_id:  the window ID of the socket, or 0.
 * 
 * Creates a new plug widget inside the #BtkSocket identified
 * by @socket_id. If @socket_id is 0, the plug is left "unplugged" and
 * can later be plugged into a #BtkSocket by  btk_socket_add_id().
 * 
 * Return value: the new #BtkPlug widget.
 **/
BtkWidget*
btk_plug_new (BdkNativeWindow socket_id)
{
  return btk_plug_new_for_display (bdk_display_get_default (), socket_id);
}

/**
 * btk_plug_new_for_display:
 * @display: the #BdkDisplay on which @socket_id is displayed
 * @socket_id: the XID of the socket's window.
 * 
 * Create a new plug widget inside the #BtkSocket identified by socket_id.
 *
 * Return value: the new #BtkPlug widget.
 *
 * Since: 2.2
 */
BtkWidget*
btk_plug_new_for_display (BdkDisplay	  *display,
			  BdkNativeWindow  socket_id)
{
  BtkPlug *plug;

  plug = g_object_new (BTK_TYPE_PLUG, NULL);
  btk_plug_construct_for_display (plug, display, socket_id);
  return BTK_WIDGET (plug);
}

static void
btk_plug_finalize (GObject *object)
{
  BtkPlug *plug = BTK_PLUG (object);

  if (plug->grabbed_keys)
    {
      g_hash_table_destroy (plug->grabbed_keys);
      plug->grabbed_keys = NULL;
    }
  
  G_OBJECT_CLASS (btk_plug_parent_class)->finalize (object);
}

static void
btk_plug_unrealize (BtkWidget *widget)
{
  BtkPlug *plug = BTK_PLUG (widget);

  if (plug->socket_window != NULL)
    {
      bdk_window_set_user_data (plug->socket_window, NULL);
      g_object_unref (plug->socket_window);
      plug->socket_window = NULL;

      g_object_notify (G_OBJECT (widget), "embedded");
    }

  if (!plug->same_app)
    {
      if (plug->modality_window)
	_btk_plug_handle_modality_off (plug);

      btk_window_group_remove_window (plug->modality_group, BTK_WINDOW (plug));
      g_object_unref (plug->modality_group);
    }

  BTK_WIDGET_CLASS (btk_plug_parent_class)->unrealize (widget);
}

static void
btk_plug_realize (BtkWidget *widget)
{
  BtkWindow *window = BTK_WINDOW (widget);
  BtkPlug *plug = BTK_PLUG (widget);
  BdkWindowAttr attributes;
  gint attributes_mask;

  btk_widget_set_realized (widget, TRUE);

  attributes.window_type = BDK_WINDOW_CHILD;	/* XXX BDK_WINDOW_PLUG ? */
  attributes.title = window->title;
  attributes.wmclass_name = window->wmclass_name;
  attributes.wmclass_class = window->wmclass_class;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = BDK_INPUT_OUTPUT;

  /* this isn't right - we should match our parent's visual/colormap.
   * though that will require handling "foreign" colormaps */
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  attributes.event_mask = btk_widget_get_events (widget);
  attributes.event_mask |= (BDK_EXPOSURE_MASK |
			    BDK_KEY_PRESS_MASK |
			    BDK_KEY_RELEASE_MASK |
			    BDK_ENTER_NOTIFY_MASK |
			    BDK_LEAVE_NOTIFY_MASK |
			    BDK_STRUCTURE_MASK);

  attributes_mask = BDK_WA_VISUAL | BDK_WA_COLORMAP;
  attributes_mask |= (window->title ? BDK_WA_TITLE : 0);
  attributes_mask |= (window->wmclass_name ? BDK_WA_WMCLASS : 0);

  if (btk_widget_is_toplevel (widget))
    {
      attributes.window_type = BDK_WINDOW_TOPLEVEL;

      bdk_error_trap_push ();
      if (plug->socket_window)
	widget->window = bdk_window_new (plug->socket_window, 
					 &attributes, attributes_mask);
      else /* If it's a passive plug, we use the root window */
	widget->window = bdk_window_new (btk_widget_get_root_window (widget),
					 &attributes, attributes_mask);

      bdk_display_sync (btk_widget_get_display (widget));
      if (bdk_error_trap_pop ()) /* Uh-oh */
	{
	  bdk_error_trap_push ();
	  bdk_window_destroy (widget->window);
	  bdk_flush ();
	  bdk_error_trap_pop ();
	  widget->window = bdk_window_new (btk_widget_get_root_window (widget),
					   &attributes, attributes_mask);
	}
      
      bdk_window_add_filter (widget->window,
			     _btk_plug_windowing_filter_func,
			     widget);

      plug->modality_group = btk_window_group_new ();
      btk_window_group_add_window (plug->modality_group, window);
      
      _btk_plug_windowing_realize_toplevel (plug);
    }
  else
    widget->window = bdk_window_new (btk_widget_get_parent_window (widget), 
				     &attributes, attributes_mask);      
  
  bdk_window_set_user_data (widget->window, window);

  widget->style = btk_style_attach (widget->style, widget->window);
  btk_style_set_background (widget->style, widget->window, BTK_STATE_NORMAL);

  bdk_window_enable_synchronized_configure (widget->window);
}

static void
btk_plug_show (BtkWidget *widget)
{
  if (btk_widget_is_toplevel (widget))
    BTK_WIDGET_CLASS (btk_plug_parent_class)->show (widget);
  else
    BTK_WIDGET_CLASS (bin_class)->show (widget);
}

static void
btk_plug_hide (BtkWidget *widget)
{
  if (btk_widget_is_toplevel (widget))
    BTK_WIDGET_CLASS (btk_plug_parent_class)->hide (widget);
  else
    BTK_WIDGET_CLASS (bin_class)->hide (widget);
}

/* From bdkinternals.h */
void bdk_synthesize_window_state (BdkWindow     *window,
                                  BdkWindowState unset_flags,
                                  BdkWindowState set_flags);

static void
btk_plug_map (BtkWidget *widget)
{
  if (btk_widget_is_toplevel (widget))
    {
      BtkBin *bin = BTK_BIN (widget);
      BtkPlug *plug = BTK_PLUG (widget);
      
      btk_widget_set_mapped (widget, TRUE);

      if (bin->child &&
	  btk_widget_get_visible (bin->child) &&
	  !btk_widget_get_mapped (bin->child))
	btk_widget_map (bin->child);

      _btk_plug_windowing_map_toplevel (plug);
      
      bdk_synthesize_window_state (widget->window,
				   BDK_WINDOW_STATE_WITHDRAWN,
				   0);
    }
  else
    BTK_WIDGET_CLASS (bin_class)->map (widget);
}

static void
btk_plug_unmap (BtkWidget *widget)
{
  if (btk_widget_is_toplevel (widget))
    {
      BtkPlug *plug = BTK_PLUG (widget);

      btk_widget_set_mapped (widget, FALSE);

      bdk_window_hide (widget->window);

      _btk_plug_windowing_unmap_toplevel (plug);
      
      bdk_synthesize_window_state (widget->window,
				   0,
				   BDK_WINDOW_STATE_WITHDRAWN);
    }
  else
    BTK_WIDGET_CLASS (bin_class)->unmap (widget);
}

static void
btk_plug_size_allocate (BtkWidget     *widget,
			BtkAllocation *allocation)
{
  if (btk_widget_is_toplevel (widget))
    BTK_WIDGET_CLASS (btk_plug_parent_class)->size_allocate (widget, allocation);
  else
    {
      BtkBin *bin = BTK_BIN (widget);

      widget->allocation = *allocation;

      if (btk_widget_get_realized (widget))
	bdk_window_move_resize (widget->window,
				allocation->x, allocation->y,
				allocation->width, allocation->height);

      if (bin->child && btk_widget_get_visible (bin->child))
	{
	  BtkAllocation child_allocation;
	  
	  child_allocation.x = child_allocation.y = BTK_CONTAINER (widget)->border_width;
	  child_allocation.width =
	    MAX (1, (gint)allocation->width - child_allocation.x * 2);
	  child_allocation.height =
	    MAX (1, (gint)allocation->height - child_allocation.y * 2);
	  
	  btk_widget_size_allocate (bin->child, &child_allocation);
	}
      
    }
}

static gboolean
btk_plug_key_press_event (BtkWidget   *widget,
			  BdkEventKey *event)
{
  if (btk_widget_is_toplevel (widget))
    return BTK_WIDGET_CLASS (btk_plug_parent_class)->key_press_event (widget, event);
  else
    return FALSE;
}

static gboolean
btk_plug_focus_event (BtkWidget      *widget,
		      BdkEventFocus  *event)
{
  /* We eat focus-in events and focus-out events, since they
   * can be generated by something like a keyboard grab on
   * a child of the plug.
   */
  return FALSE;
}

static void
btk_plug_set_focus (BtkWindow *window,
		    BtkWidget *focus)
{
  BtkPlug *plug = BTK_PLUG (window);

  BTK_WINDOW_CLASS (btk_plug_parent_class)->set_focus (window, focus);
  
  /* Ask for focus from embedder
   */

  if (focus && !window->has_toplevel_focus)
    _btk_plug_windowing_set_focus (plug);
}

static guint
grabbed_key_hash (gconstpointer a)
{
  const GrabbedKey *key = a;
  guint h;
  
  h = key->accelerator_key << 16;
  h ^= key->accelerator_key >> 16;
  h ^= key->accelerator_mods;

  return h;
}

static gboolean
grabbed_key_equal (gconstpointer a, gconstpointer b)
{
  const GrabbedKey *keya = a;
  const GrabbedKey *keyb = b;

  return (keya->accelerator_key == keyb->accelerator_key &&
	  keya->accelerator_mods == keyb->accelerator_mods);
}

static void
add_grabbed_key (gpointer key, gpointer val, gpointer data)
{
  GrabbedKey *grabbed_key = key;
  BtkPlug *plug = data;

  if (!plug->grabbed_keys ||
      !g_hash_table_lookup (plug->grabbed_keys, grabbed_key))
    {
      _btk_plug_windowing_add_grabbed_key (plug,
					   grabbed_key->accelerator_key,
					   grabbed_key->accelerator_mods);
    }
}

static void
add_grabbed_key_always (gpointer key,
			gpointer val,
			gpointer data)
{
  GrabbedKey *grabbed_key = key;
  BtkPlug *plug = data;

  _btk_plug_windowing_add_grabbed_key (plug,
				       grabbed_key->accelerator_key,
				       grabbed_key->accelerator_mods);
}

/**
 * _btk_plug_add_all_grabbed_keys:
 *
 * @plug: a #BtkPlug
 *
 * Calls _btk_plug_windowing_add_grabbed_key() on all the grabbed keys
 * in the @plug.
 */
void
_btk_plug_add_all_grabbed_keys (BtkPlug *plug)
{
  if (plug->grabbed_keys)
    g_hash_table_foreach (plug->grabbed_keys, add_grabbed_key_always, plug);
}

static void
remove_grabbed_key (gpointer key, gpointer val, gpointer data)
{
  GrabbedKey *grabbed_key = key;
  BtkPlug *plug = data;

  if (!plug->grabbed_keys ||
      !g_hash_table_lookup (plug->grabbed_keys, grabbed_key))
    {
      _btk_plug_windowing_remove_grabbed_key (plug, 
					      grabbed_key->accelerator_key,
					      grabbed_key->accelerator_mods);
    }
}

static void
keys_foreach (BtkWindow      *window,
	      guint           keyval,
	      BdkModifierType modifiers,
	      gboolean        is_mnemonic,
	      gpointer        data)
{
  GHashTable *new_grabbed_keys = data;
  GrabbedKey *key = g_slice_new (GrabbedKey);

  key->accelerator_key = keyval;
  key->accelerator_mods = modifiers;
  
  g_hash_table_replace (new_grabbed_keys, key, key);
}

static void
grabbed_key_free (gpointer data)
{
  g_slice_free (GrabbedKey, data);
}

static void
btk_plug_keys_changed (BtkWindow *window)
{
  GHashTable *new_grabbed_keys, *old_grabbed_keys;
  BtkPlug *plug = BTK_PLUG (window);

  new_grabbed_keys = g_hash_table_new_full (grabbed_key_hash, grabbed_key_equal, (GDestroyNotify)grabbed_key_free, NULL);
  _btk_window_keys_foreach (window, keys_foreach, new_grabbed_keys);

  if (plug->socket_window)
    g_hash_table_foreach (new_grabbed_keys, add_grabbed_key, plug);

  old_grabbed_keys = plug->grabbed_keys;
  plug->grabbed_keys = new_grabbed_keys;

  if (old_grabbed_keys)
    {
      if (plug->socket_window)
	g_hash_table_foreach (old_grabbed_keys, remove_grabbed_key, plug);
      g_hash_table_destroy (old_grabbed_keys);
    }
}

static gboolean
btk_plug_focus (BtkWidget        *widget,
		BtkDirectionType  direction)
{
  BtkBin *bin = BTK_BIN (widget);
  BtkPlug *plug = BTK_PLUG (widget);
  BtkWindow *window = BTK_WINDOW (widget);
  BtkContainer *container = BTK_CONTAINER (widget);
  BtkWidget *old_focus_child = container->focus_child;
  BtkWidget *parent;
  
  /* We override BtkWindow's behavior, since we don't want wrapping here.
   */
  if (old_focus_child)
    {
      if (btk_widget_child_focus (old_focus_child, direction))
	return TRUE;

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
    }
  else
    {
      /* Try to focus the first widget in the window */
      if (bin->child && btk_widget_child_focus (bin->child, direction))
        return TRUE;
    }

  if (!BTK_CONTAINER (window)->focus_child)
    _btk_plug_windowing_focus_to_parent (plug, direction);

  return FALSE;
}

static void
btk_plug_check_resize (BtkContainer *container)
{
  if (btk_widget_is_toplevel (BTK_WIDGET (container)))
    BTK_CONTAINER_CLASS (btk_plug_parent_class)->check_resize (container);
  else
    BTK_CONTAINER_CLASS (bin_class)->check_resize (container);
}

/**
 * _btk_plug_handle_modality_on:
 *
 * @plug: a #BtkPlug
 *
 * Called from the BtkPlug backend when the corresponding socket has
 * told the plug that it modality has toggled on.
 */
void
_btk_plug_handle_modality_on (BtkPlug *plug)
{
  if (!plug->modality_window)
    {
      plug->modality_window = btk_window_new (BTK_WINDOW_POPUP);
      btk_window_set_screen (BTK_WINDOW (plug->modality_window),
			     btk_widget_get_screen (BTK_WIDGET (plug)));
      btk_widget_realize (plug->modality_window);
      btk_window_group_add_window (plug->modality_group, BTK_WINDOW (plug->modality_window));
      btk_grab_add (plug->modality_window);
    }
}

/**
 * _btk_plug_handle_modality_off:
 *
 * @plug: a #BtkPlug
 *
 * Called from the BtkPlug backend when the corresponding socket has
 * told the plug that it modality has toggled off.
 */
void
_btk_plug_handle_modality_off (BtkPlug *plug)
{
  if (plug->modality_window)
    {
      btk_widget_destroy (plug->modality_window);
      plug->modality_window = NULL;
    }
}

/**
 * _btk_plug_focus_first_last:
 *
 * @plug: a #BtkPlug
 * @direction: a direction
 *
 * Called from the BtkPlug backend when the corresponding socket has
 * told the plug that it has received the focus.
 */
void
_btk_plug_focus_first_last (BtkPlug          *plug,
			    BtkDirectionType  direction)
{
  BtkWindow *window = BTK_WINDOW (plug);
  BtkWidget *parent;

  if (window->focus_widget)
    {
      parent = window->focus_widget->parent;
      while (parent)
	{
	  btk_container_set_focus_child (BTK_CONTAINER (parent), NULL);
	  parent = BTK_WIDGET (parent)->parent;
	}
      
      btk_window_set_focus (BTK_WINDOW (plug), NULL);
    }

  btk_widget_child_focus (BTK_WIDGET (plug), direction);
}

#define __BTK_PLUG_C__
#include "btkaliasdef.c"
