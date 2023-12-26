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

#include "config.h"
#include <string.h>		/* For memset() */

#include "bdk.h"
#include "bdkinternals.h"
#include "bdkalias.h"

typedef struct _BdkIOClosure BdkIOClosure;

struct _BdkIOClosure
{
  BdkInputFunction function;
  BdkInputCondition condition;
  GDestroyNotify notify;
  gpointer data;
};

/* Private variable declarations
 */

BdkEventFunc   _bdk_event_func = NULL;    /* Callback for events */
gpointer       _bdk_event_data = NULL;
GDestroyNotify _bdk_event_notify = NULL;

/*********************************************
 * Functions for maintaining the event queue *
 *********************************************/

/**
 * _bdk_event_queue_find_first:
 * @display: a #BdkDisplay
 * 
 * Find the first event on the queue that is not still
 * being filled in.
 * 
 * Return value: Pointer to the list node for that event, or NULL.
 **/
GList*
_bdk_event_queue_find_first (BdkDisplay *display)
{
  GList *tmp_list = display->queued_events;

  while (tmp_list)
    {
      BdkEventPrivate *event = tmp_list->data;
      if (!(event->flags & BDK_EVENT_PENDING))
	return tmp_list;

      tmp_list = g_list_next (tmp_list);
    }

  return NULL;
}

/**
 * _bdk_event_queue_prepend:
 * @display: a #BdkDisplay
 * @event: Event to prepend.
 *
 * Prepends an event before the head of the event queue.
 *
 * Returns: the newly prepended list node.
 **/
GList*
_bdk_event_queue_prepend (BdkDisplay *display,
			  BdkEvent   *event)
{
  display->queued_events = g_list_prepend (display->queued_events, event);
  if (!display->queued_tail)
    display->queued_tail = display->queued_events;
  return display->queued_events;
}

/**
 * _bdk_event_queue_append:
 * @display: a #BdkDisplay
 * @event: Event to append.
 * 
 * Appends an event onto the tail of the event queue.
 *
 * Returns: the newly appended list node.
 **/
GList *
_bdk_event_queue_append (BdkDisplay *display,
			 BdkEvent   *event)
{
  display->queued_tail = g_list_append (display->queued_tail, event);
  
  if (!display->queued_events)
    display->queued_events = display->queued_tail;
  else
    display->queued_tail = display->queued_tail->next;

  return display->queued_tail;
}

/**
 * _bdk_event_queue_insert_after:
 * @display: a #BdkDisplay
 * @sibling: Append after this event.
 * @event: Event to append.
 *
 * Appends an event after the specified event, or if it isn't in
 * the queue, onto the tail of the event queue.
 *
 * Returns: the newly appended list node.
 *
 * Since: 2.16
 */
GList*
_bdk_event_queue_insert_after (BdkDisplay *display,
                               BdkEvent   *sibling,
                               BdkEvent   *event)
{
  GList *prev = g_list_find (display->queued_events, sibling);
  if (prev && prev->next)
    {
      display->queued_events = g_list_insert_before (display->queued_events, prev->next, event);
      return prev->next;
    }
  else
    return _bdk_event_queue_append (display, event);
}

/**
 * _bdk_event_queue_insert_after:
 * @display: a #BdkDisplay
 * @sibling: Append after this event.
 * @event: Event to append.
 *
 * Appends an event before the specified event, or if it isn't in
 * the queue, onto the tail of the event queue.
 *
 * Returns: the newly appended list node.
 *
 * Since: 2.16
 */
GList*
_bdk_event_queue_insert_before (BdkDisplay *display,
				BdkEvent   *sibling,
				BdkEvent   *event)
{
  GList *next = g_list_find (display->queued_events, sibling);
  if (next)
    {
      display->queued_events = g_list_insert_before (display->queued_events, next, event);
      return next->prev;
    }
  else
    return _bdk_event_queue_append (display, event);
}


/**
 * _bdk_event_queue_remove_link:
 * @display: a #BdkDisplay
 * @node: node to remove
 * 
 * Removes a specified list node from the event queue.
 **/
void
_bdk_event_queue_remove_link (BdkDisplay *display,
			      GList      *node)
{
  if (node->prev)
    node->prev->next = node->next;
  else
    display->queued_events = node->next;
  
  if (node->next)
    node->next->prev = node->prev;
  else
    display->queued_tail = node->prev;
}

/**
 * _bdk_event_unqueue:
 * @display: a #BdkDisplay
 * 
 * Removes and returns the first event from the event
 * queue that is not still being filled in.
 * 
 * Return value: the event, or %NULL. Ownership is transferred
 * to the caller.
 **/
BdkEvent*
_bdk_event_unqueue (BdkDisplay *display)
{
  BdkEvent *event = NULL;
  GList *tmp_list;

  tmp_list = _bdk_event_queue_find_first (display);

  if (tmp_list)
    {
      event = tmp_list->data;
      _bdk_event_queue_remove_link (display, tmp_list);
      g_list_free_1 (tmp_list);
    }

  return event;
}

/**
 * bdk_event_handler_set:
 * @func: the function to call to handle events from BDK.
 * @data: user data to pass to the function. 
 * @notify: the function to call when the handler function is removed, i.e. when
 *          bdk_event_handler_set() is called with another event handler.
 * 
 * Sets the function to call to handle all events from BDK.
 *
 * Note that BTK+ uses this to install its own event handler, so it is
 * usually not useful for BTK+ applications. (Although an application
 * can call this function then call btk_main_do_event() to pass
 * events to BTK+.)
 **/
void 
bdk_event_handler_set (BdkEventFunc   func,
		       gpointer       data,
		       GDestroyNotify notify)
{
  if (_bdk_event_notify)
    (*_bdk_event_notify) (_bdk_event_data);

  _bdk_event_func = func;
  _bdk_event_data = data;
  _bdk_event_notify = notify;
}

/**
 * bdk_event_get:
 * 
 * Checks all open displays for a #BdkEvent to process,to be processed
 * on, fetching events from the windowing system if necessary.
 * See bdk_display_get_event().
 * 
 * Return value: the next #BdkEvent to be processed, or %NULL if no events
 * are pending. The returned #BdkEvent should be freed with bdk_event_free().
 **/
BdkEvent*
bdk_event_get (void)
{
  GSList *tmp_list;

  for (tmp_list = _bdk_displays; tmp_list; tmp_list = tmp_list->next)
    {
      BdkEvent *event = bdk_display_get_event (tmp_list->data);
      if (event)
	return event;
    }

  return NULL;
}

/**
 * bdk_event_peek:
 *
 * If there is an event waiting in the event queue of some open
 * display, returns a copy of it. See bdk_display_peek_event().
 * 
 * Return value: a copy of the first #BdkEvent on some event queue, or %NULL if no
 * events are in any queues. The returned #BdkEvent should be freed with
 * bdk_event_free().
 **/
BdkEvent*
bdk_event_peek (void)
{
  GSList *tmp_list;

  for (tmp_list = _bdk_displays; tmp_list; tmp_list = tmp_list->next)
    {
      BdkEvent *event = bdk_display_peek_event (tmp_list->data);
      if (event)
	return event;
    }

  return NULL;
}

/**
 * bdk_event_put:
 * @event: a #BdkEvent.
 *
 * Appends a copy of the given event onto the front of the event
 * queue for event->any.window's display, or the default event
 * queue if event->any.window is %NULL. See bdk_display_put_event().
 **/
void
bdk_event_put (const BdkEvent *event)
{
  BdkDisplay *display;
  
  g_return_if_fail (event != NULL);

  if (event->any.window)
    display = bdk_drawable_get_display (event->any.window);
  else
    {
      BDK_NOTE (MULTIHEAD,
		g_message ("Falling back to default display for bdk_event_put()"));
      display = bdk_display_get_default ();
    }

  bdk_display_put_event (display, event);
}

static GHashTable *event_hash = NULL;

/**
 * bdk_event_new:
 * @type: a #BdkEventType 
 * 
 * Creates a new event of the given type. All fields are set to 0.
 * 
 * Return value: a newly-allocated #BdkEvent. The returned #BdkEvent 
 * should be freed with bdk_event_free().
 *
 * Since: 2.2
 **/
BdkEvent*
bdk_event_new (BdkEventType type)
{
  BdkEventPrivate *new_private;
  BdkEvent *new_event;
  
  if (!event_hash)
    event_hash = g_hash_table_new (g_direct_hash, NULL);

  new_private = g_slice_new0 (BdkEventPrivate);
  
  new_private->flags = 0;
  new_private->screen = NULL;

  g_hash_table_insert (event_hash, new_private, GUINT_TO_POINTER (1));

  new_event = (BdkEvent *) new_private;

  new_event->any.type = type;

  /*
   * Bytewise 0 initialization is reasonable for most of the 
   * current event types. Explicitely initialize double fields
   * since I trust bytewise 0 == 0. less than for integers
   * or pointers.
   */
  switch (type)
    {
    case BDK_MOTION_NOTIFY:
      new_event->motion.x = 0.;
      new_event->motion.y = 0.;
      new_event->motion.x_root = 0.;
      new_event->motion.y_root = 0.;
      break;
    case BDK_BUTTON_PRESS:
    case BDK_2BUTTON_PRESS:
    case BDK_3BUTTON_PRESS:
    case BDK_BUTTON_RELEASE:
      new_event->button.x = 0.;
      new_event->button.y = 0.;
      new_event->button.x_root = 0.;
      new_event->button.y_root = 0.;
      break;
    case BDK_SCROLL:
      new_event->scroll.x = 0.;
      new_event->scroll.y = 0.;
      new_event->scroll.x_root = 0.;
      new_event->scroll.y_root = 0.;
      break;
    case BDK_ENTER_NOTIFY:
    case BDK_LEAVE_NOTIFY:
      new_event->crossing.x = 0.;
      new_event->crossing.y = 0.;
      new_event->crossing.x_root = 0.;
      new_event->crossing.y_root = 0.;
      break;
    default:
      break;
    }
  
  return new_event;
}

static gboolean
bdk_event_is_allocated (const BdkEvent *event)
{
  if (event_hash)
    return g_hash_table_lookup (event_hash, event) != NULL;

  return FALSE;
}
 
/**
 * bdk_event_copy:
 * @event: a #BdkEvent
 * 
 * Copies a #BdkEvent, copying or incrementing the reference count of the
 * resources associated with it (e.g. #BdkWindow's and strings).
 * 
 * Return value: a copy of @event. The returned #BdkEvent should be freed with
 * bdk_event_free().
 **/
BdkEvent*
bdk_event_copy (const BdkEvent *event)
{
  BdkEventPrivate *new_private;
  BdkEvent *new_event;
  
  g_return_val_if_fail (event != NULL, NULL);
  
  new_event = bdk_event_new (BDK_NOTHING);
  new_private = (BdkEventPrivate *)new_event;

  *new_event = *event;
  if (new_event->any.window)
    g_object_ref (new_event->any.window);

  if (bdk_event_is_allocated (event))
    {
      BdkEventPrivate *private = (BdkEventPrivate *)event;

      new_private->screen = private->screen;
    }
  
  switch (event->any.type)
    {
    case BDK_KEY_PRESS:
    case BDK_KEY_RELEASE:
      new_event->key.string = g_strdup (event->key.string);
      break;
      
    case BDK_ENTER_NOTIFY:
    case BDK_LEAVE_NOTIFY:
      if (event->crossing.subwindow != NULL)
	g_object_ref (event->crossing.subwindow);
      break;
      
    case BDK_DRAG_ENTER:
    case BDK_DRAG_LEAVE:
    case BDK_DRAG_MOTION:
    case BDK_DRAG_STATUS:
    case BDK_DROP_START:
    case BDK_DROP_FINISHED:
      g_object_ref (event->dnd.context);
      break;
      
    case BDK_EXPOSE:
    case BDK_DAMAGE:
      if (event->expose.rebunnyion)
	new_event->expose.rebunnyion = bdk_rebunnyion_copy (event->expose.rebunnyion);
      break;
      
    case BDK_SETTING:
      new_event->setting.name = g_strdup (new_event->setting.name);
      break;

    case BDK_BUTTON_PRESS:
    case BDK_BUTTON_RELEASE:
      if (event->button.axes) 
	new_event->button.axes = g_memdup (event->button.axes, 
					     sizeof (gdouble) * event->button.device->num_axes);
      break;

    case BDK_MOTION_NOTIFY:
      if (event->motion.axes) 
	new_event->motion.axes = g_memdup (event->motion.axes, 
					   sizeof (gdouble) * event->motion.device->num_axes);
      
      break;
      
    default:
      break;
    }

  if (bdk_event_is_allocated (event))
    _bdk_windowing_event_data_copy (event, new_event);
  
  return new_event;
}

/**
 * bdk_event_free:
 * @event:  a #BdkEvent.
 * 
 * Frees a #BdkEvent, freeing or decrementing any resources associated with it.
 * Note that this function should only be called with events returned from
 * functions such as bdk_event_peek(), bdk_event_get(),
 * bdk_event_get_graphics_expose() and bdk_event_copy() and bdk_event_new().
 **/
void
bdk_event_free (BdkEvent *event)
{
  g_return_if_fail (event != NULL);

  if (event->any.window)
    g_object_unref (event->any.window);
  
  switch (event->any.type)
    {
    case BDK_KEY_PRESS:
    case BDK_KEY_RELEASE:
      g_free (event->key.string);
      break;
      
    case BDK_ENTER_NOTIFY:
    case BDK_LEAVE_NOTIFY:
      if (event->crossing.subwindow != NULL)
	g_object_unref (event->crossing.subwindow);
      break;
      
    case BDK_DRAG_ENTER:
    case BDK_DRAG_LEAVE:
    case BDK_DRAG_MOTION:
    case BDK_DRAG_STATUS:
    case BDK_DROP_START:
    case BDK_DROP_FINISHED:
      g_object_unref (event->dnd.context);
      break;

    case BDK_BUTTON_PRESS:
    case BDK_BUTTON_RELEASE:
      g_free (event->button.axes);
      break;
      
    case BDK_EXPOSE:
    case BDK_DAMAGE:
      if (event->expose.rebunnyion)
	bdk_rebunnyion_destroy (event->expose.rebunnyion);
      break;
      
    case BDK_MOTION_NOTIFY:
      g_free (event->motion.axes);
      break;
      
    case BDK_SETTING:
      g_free (event->setting.name);
      break;
      
    default:
      break;
    }

  _bdk_windowing_event_data_free (event);

  g_hash_table_remove (event_hash, event);
  g_slice_free (BdkEventPrivate, (BdkEventPrivate*) event);
}

/**
 * bdk_event_get_time:
 * @event: a #BdkEvent
 * 
 * Returns the time stamp from @event, if there is one; otherwise
 * returns #BDK_CURRENT_TIME. If @event is %NULL, returns #BDK_CURRENT_TIME.
 * 
 * Return value: time stamp field from @event
 **/
guint32
bdk_event_get_time (const BdkEvent *event)
{
  if (event)
    switch (event->type)
      {
      case BDK_MOTION_NOTIFY:
	return event->motion.time;
      case BDK_BUTTON_PRESS:
      case BDK_2BUTTON_PRESS:
      case BDK_3BUTTON_PRESS:
      case BDK_BUTTON_RELEASE:
	return event->button.time;
      case BDK_SCROLL:
        return event->scroll.time;
      case BDK_KEY_PRESS:
      case BDK_KEY_RELEASE:
	return event->key.time;
      case BDK_ENTER_NOTIFY:
      case BDK_LEAVE_NOTIFY:
	return event->crossing.time;
      case BDK_PROPERTY_NOTIFY:
	return event->property.time;
      case BDK_SELECTION_CLEAR:
      case BDK_SELECTION_REQUEST:
      case BDK_SELECTION_NOTIFY:
	return event->selection.time;
      case BDK_PROXIMITY_IN:
      case BDK_PROXIMITY_OUT:
	return event->proximity.time;
      case BDK_DRAG_ENTER:
      case BDK_DRAG_LEAVE:
      case BDK_DRAG_MOTION:
      case BDK_DRAG_STATUS:
      case BDK_DROP_START:
      case BDK_DROP_FINISHED:
	return event->dnd.time;
      case BDK_CLIENT_EVENT:
      case BDK_VISIBILITY_NOTIFY:
      case BDK_NO_EXPOSE:
      case BDK_CONFIGURE:
      case BDK_FOCUS_CHANGE:
      case BDK_NOTHING:
      case BDK_DAMAGE:
      case BDK_DELETE:
      case BDK_DESTROY:
      case BDK_EXPOSE:
      case BDK_MAP:
      case BDK_UNMAP:
      case BDK_WINDOW_STATE:
      case BDK_SETTING:
      case BDK_OWNER_CHANGE:
      case BDK_GRAB_BROKEN:
      case BDK_EVENT_LAST:
        /* return current time */
        break;
      }
  
  return BDK_CURRENT_TIME;
}

/**
 * bdk_event_get_state:
 * @event: a #BdkEvent or NULL
 * @state: (out): return location for state
 * 
 * If the event contains a "state" field, puts that field in @state. Otherwise
 * stores an empty state (0). Returns %TRUE if there was a state field
 * in the event. @event may be %NULL, in which case it's treated
 * as if the event had no state field.
 * 
 * Return value: %TRUE if there was a state field in the event 
 **/
gboolean
bdk_event_get_state (const BdkEvent        *event,
                     BdkModifierType       *state)
{
  g_return_val_if_fail (state != NULL, FALSE);
  
  if (event)
    switch (event->type)
      {
      case BDK_MOTION_NOTIFY:
	*state = event->motion.state;
        return TRUE;
      case BDK_BUTTON_PRESS:
      case BDK_2BUTTON_PRESS:
      case BDK_3BUTTON_PRESS:
      case BDK_BUTTON_RELEASE:
        *state =  event->button.state;
        return TRUE;
      case BDK_SCROLL:
	*state =  event->scroll.state;
        return TRUE;
      case BDK_KEY_PRESS:
      case BDK_KEY_RELEASE:
	*state =  event->key.state;
        return TRUE;
      case BDK_ENTER_NOTIFY:
      case BDK_LEAVE_NOTIFY:
	*state =  event->crossing.state;
        return TRUE;
      case BDK_PROPERTY_NOTIFY:
	*state =  event->property.state;
        return TRUE;
      case BDK_VISIBILITY_NOTIFY:
      case BDK_CLIENT_EVENT:
      case BDK_NO_EXPOSE:
      case BDK_CONFIGURE:
      case BDK_FOCUS_CHANGE:
      case BDK_SELECTION_CLEAR:
      case BDK_SELECTION_REQUEST:
      case BDK_SELECTION_NOTIFY:
      case BDK_PROXIMITY_IN:
      case BDK_PROXIMITY_OUT:
      case BDK_DAMAGE:
      case BDK_DRAG_ENTER:
      case BDK_DRAG_LEAVE:
      case BDK_DRAG_MOTION:
      case BDK_DRAG_STATUS:
      case BDK_DROP_START:
      case BDK_DROP_FINISHED:
      case BDK_NOTHING:
      case BDK_DELETE:
      case BDK_DESTROY:
      case BDK_EXPOSE:
      case BDK_MAP:
      case BDK_UNMAP:
      case BDK_WINDOW_STATE:
      case BDK_SETTING:
      case BDK_OWNER_CHANGE:
      case BDK_GRAB_BROKEN:
      case BDK_EVENT_LAST:
        /* no state field */
        break;
      }

  *state = 0;
  return FALSE;
}

/**
 * bdk_event_get_coords:
 * @event: a #BdkEvent
 * @x_win: (out): location to put event window x coordinate
 * @y_win: (out): location to put event window y coordinate
 * 
 * Extract the event window relative x/y coordinates from an event.
 * 
 * Return value: %TRUE if the event delivered event window coordinates
 **/
gboolean
bdk_event_get_coords (const BdkEvent *event,
		      gdouble        *x_win,
		      gdouble        *y_win)
{
  gdouble x = 0, y = 0;
  gboolean fetched = TRUE;
  
  g_return_val_if_fail (event != NULL, FALSE);

  switch (event->type)
    {
    case BDK_CONFIGURE:
      x = event->configure.x;
      y = event->configure.y;
      break;
    case BDK_ENTER_NOTIFY:
    case BDK_LEAVE_NOTIFY:
      x = event->crossing.x;
      y = event->crossing.y;
      break;
    case BDK_SCROLL:
      x = event->scroll.x;
      y = event->scroll.y;
      break;
    case BDK_BUTTON_PRESS:
    case BDK_2BUTTON_PRESS:
    case BDK_3BUTTON_PRESS:
    case BDK_BUTTON_RELEASE:
      x = event->button.x;
      y = event->button.y;
      break;
    case BDK_MOTION_NOTIFY:
      x = event->motion.x;
      y = event->motion.y;
      break;
    default:
      fetched = FALSE;
      break;
    }

  if (x_win)
    *x_win = x;
  if (y_win)
    *y_win = y;

  return fetched;
}

/**
 * bdk_event_get_root_coords:
 * @event: a #BdkEvent
 * @x_root: (out): location to put root window x coordinate
 * @y_root: (out): location to put root window y coordinate
 * 
 * Extract the root window relative x/y coordinates from an event.
 * 
 * Return value: %TRUE if the event delivered root window coordinates
 **/
gboolean
bdk_event_get_root_coords (const BdkEvent *event,
			   gdouble        *x_root,
			   gdouble        *y_root)
{
  gdouble x = 0, y = 0;
  gboolean fetched = TRUE;
  
  g_return_val_if_fail (event != NULL, FALSE);

  switch (event->type)
    {
    case BDK_MOTION_NOTIFY:
      x = event->motion.x_root;
      y = event->motion.y_root;
      break;
    case BDK_SCROLL:
      x = event->scroll.x_root;
      y = event->scroll.y_root;
      break;
    case BDK_BUTTON_PRESS:
    case BDK_2BUTTON_PRESS:
    case BDK_3BUTTON_PRESS:
    case BDK_BUTTON_RELEASE:
      x = event->button.x_root;
      y = event->button.y_root;
      break;
    case BDK_ENTER_NOTIFY:
    case BDK_LEAVE_NOTIFY:
      x = event->crossing.x_root;
      y = event->crossing.y_root;
      break;
    case BDK_DRAG_ENTER:
    case BDK_DRAG_LEAVE:
    case BDK_DRAG_MOTION:
    case BDK_DRAG_STATUS:
    case BDK_DROP_START:
    case BDK_DROP_FINISHED:
      x = event->dnd.x_root;
      y = event->dnd.y_root;
      break;
    default:
      fetched = FALSE;
      break;
    }

  if (x_root)
    *x_root = x;
  if (y_root)
    *y_root = y;

  return fetched;
}

/**
 * bdk_event_get_axis:
 * @event: a #BdkEvent
 * @axis_use: the axis use to look for
 * @value: (out): location to store the value found
 * 
 * Extract the axis value for a particular axis use from
 * an event structure.
 * 
 * Return value: %TRUE if the specified axis was found, otherwise %FALSE
 **/
gboolean
bdk_event_get_axis (const BdkEvent *event,
		    BdkAxisUse      axis_use,
		    gdouble        *value)
{
  gdouble *axes;
  BdkDevice *device;
  
  g_return_val_if_fail (event != NULL, FALSE);
  
  if (axis_use == BDK_AXIS_X || axis_use == BDK_AXIS_Y)
    {
      gdouble x, y;
      
      switch (event->type)
	{
	case BDK_MOTION_NOTIFY:
	  x = event->motion.x;
	  y = event->motion.y;
	  break;
	case BDK_SCROLL:
	  x = event->scroll.x;
	  y = event->scroll.y;
	  break;
	case BDK_BUTTON_PRESS:
	case BDK_BUTTON_RELEASE:
	  x = event->button.x;
	  y = event->button.y;
	  break;
	case BDK_ENTER_NOTIFY:
	case BDK_LEAVE_NOTIFY:
	  x = event->crossing.x;
	  y = event->crossing.y;
	  break;
	  
	default:
	  return FALSE;
	}

      if (axis_use == BDK_AXIS_X && value)
	*value = x;
      if (axis_use == BDK_AXIS_Y && value)
	*value = y;

      return TRUE;
    }
  else if (event->type == BDK_BUTTON_PRESS ||
	   event->type == BDK_BUTTON_RELEASE)
    {
      device = event->button.device;
      axes = event->button.axes;
    }
  else if (event->type == BDK_MOTION_NOTIFY)
    {
      device = event->motion.device;
      axes = event->motion.axes;
    }
  else
    return FALSE;

  return bdk_device_get_axis (device, axes, axis_use, value);
}

/**
 * bdk_event_request_motions:
 * @event: a valid #BdkEvent
 *
 * Request more motion notifies if @event is a motion notify hint event.
 * This function should be used instead of bdk_window_get_pointer() to
 * request further motion notifies, because it also works for extension
 * events where motion notifies are provided for devices other than the
 * core pointer. Coordinate extraction, processing and requesting more
 * motion events from a %BDK_MOTION_NOTIFY event usually works like this:
 *
 * |[
 * { 
 *   /&ast; motion_event handler &ast;/
 *   x = motion_event->x;
 *   y = motion_event->y;
 *   /&ast; handle (x,y) motion &ast;/
 *   bdk_event_request_motions (motion_event); /&ast; handles is_hint events &ast;/
 * }
 * ]|
 *
 * Since: 2.12
 **/
void
bdk_event_request_motions (const BdkEventMotion *event)
{
  BdkDisplay *display;
  
  g_return_if_fail (event != NULL);
  
  if (event->type == BDK_MOTION_NOTIFY && event->is_hint)
    {
      bdk_device_get_state (event->device, event->window, NULL, NULL);
      
      display = bdk_drawable_get_display (event->window);
      _bdk_display_enable_motion_hints (display);
    }
}

/**
 * bdk_event_set_screen:
 * @event: a #BdkEvent
 * @screen: a #BdkScreen
 * 
 * Sets the screen for @event to @screen. The event must
 * have been allocated by BTK+, for instance, by
 * bdk_event_copy().
 *
 * Since: 2.2
 **/
void
bdk_event_set_screen (BdkEvent  *event,
		      BdkScreen *screen)
{
  BdkEventPrivate *private;
  
  g_return_if_fail (bdk_event_is_allocated (event));

  private = (BdkEventPrivate *)event;
  
  private->screen = screen;
}

/**
 * bdk_event_get_screen:
 * @event: a #BdkEvent
 * 
 * Returns the screen for the event. The screen is
 * typically the screen for <literal>event->any.window</literal>, but
 * for events such as mouse events, it is the screen
 * where the pointer was when the event occurs -
 * that is, the screen which has the root window 
 * to which <literal>event->motion.x_root</literal> and
 * <literal>event->motion.y_root</literal> are relative.
 * 
 * Return value: the screen for the event
 *
 * Since: 2.2
 **/
BdkScreen *
bdk_event_get_screen (const BdkEvent *event)
{
  if (bdk_event_is_allocated (event))
    {
      BdkEventPrivate *private = (BdkEventPrivate *)event;

      if (private->screen)
	return private->screen;
    }

  if (event->any.window)
    return bdk_drawable_get_screen (event->any.window);

  return NULL;
}

/**
 * bdk_set_show_events:
 * @show_events:  %TRUE to output event debugging information.
 * 
 * Sets whether a trace of received events is output.
 * Note that BTK+ must be compiled with debugging (that is,
 * configured using the <option>--enable-debug</option> option)
 * to use this option.
 **/
void
bdk_set_show_events (gboolean show_events)
{
  if (show_events)
    _bdk_debug_flags |= BDK_DEBUG_EVENTS;
  else
    _bdk_debug_flags &= ~BDK_DEBUG_EVENTS;
}

/**
 * bdk_get_show_events:
 * 
 * Gets whether event debugging output is enabled.
 * 
 * Return value: %TRUE if event debugging output is enabled.
 **/
gboolean
bdk_get_show_events (void)
{
  return (_bdk_debug_flags & BDK_DEBUG_EVENTS) != 0;
}

static void
bdk_io_destroy (gpointer data)
{
  BdkIOClosure *closure = data;

  if (closure->notify)
    closure->notify (closure->data);

  g_free (closure);
}

/* What do we do with G_IO_NVAL?
 */
#define READ_CONDITION (G_IO_IN | G_IO_HUP | G_IO_ERR)
#define WRITE_CONDITION (G_IO_OUT | G_IO_ERR)
#define EXCEPTION_CONDITION (G_IO_PRI)

static gboolean  
bdk_io_invoke (BUNNYIOChannel   *source,
	       BUNNYIOCondition  condition,
	       gpointer      data)
{
  BdkIOClosure *closure = data;
  BdkInputCondition bdk_cond = 0;

  if (condition & READ_CONDITION)
    bdk_cond |= BDK_INPUT_READ;
  if (condition & WRITE_CONDITION)
    bdk_cond |= BDK_INPUT_WRITE;
  if (condition & EXCEPTION_CONDITION)
    bdk_cond |= BDK_INPUT_EXCEPTION;

  if (closure->condition & bdk_cond)
    closure->function (closure->data, g_io_channel_unix_get_fd (source), bdk_cond);

  return TRUE;
}

/**
 * bdk_input_add_full:
 * @source: a file descriptor.
 * @condition: the condition.
 * @function: the callback function.
 * @data: callback data passed to @function.
 * @destroy: callback function to call with @data when the input
 * handler is removed.
 *
 * Establish a callback when a condition becomes true on
 * a file descriptor.
 *
 * Returns: a tag that can later be used as an argument to
 * bdk_input_remove().
 *
 * Deprecated: 2.14: Use g_io_add_watch_full() on a #BUNNYIOChannel
 */
gint
bdk_input_add_full (gint	      source,
		    BdkInputCondition condition,
		    BdkInputFunction  function,
		    gpointer	      data,
		    GDestroyNotify    destroy)
{
  guint result;
  BdkIOClosure *closure = g_new (BdkIOClosure, 1);
  BUNNYIOChannel *channel;
  BUNNYIOCondition cond = 0;

  closure->function = function;
  closure->condition = condition;
  closure->notify = destroy;
  closure->data = data;

  if (condition & BDK_INPUT_READ)
    cond |= READ_CONDITION;
  if (condition & BDK_INPUT_WRITE)
    cond |= WRITE_CONDITION;
  if (condition & BDK_INPUT_EXCEPTION)
    cond |= EXCEPTION_CONDITION;

  channel = g_io_channel_unix_new (source);
  result = g_io_add_watch_full (channel, G_PRIORITY_DEFAULT, cond, 
				bdk_io_invoke,
				closure, bdk_io_destroy);
  g_io_channel_unref (channel);

  return result;
}

/**
 * bdk_input_add:
 * @source: a file descriptor.
 * @condition: the condition.
 * @function: the callback function.
 * @data: callback data passed to @function.
 *
 * Establish a callback when a condition becomes true on
 * a file descriptor.
 *
 * Returns: a tag that can later be used as an argument to
 * bdk_input_remove().
 *
 * Deprecated: 2.14: Use g_io_add_watch() on a #BUNNYIOChannel
 */
gint
bdk_input_add (gint		 source,
	       BdkInputCondition condition,
	       BdkInputFunction	 function,
	       gpointer		 data)
{
  return bdk_input_add_full (source, condition, function, data, NULL);
}

void
bdk_input_remove (gint tag)
{
  g_source_remove (tag);
}

static void
bdk_synthesize_click (BdkDisplay *display,
		      BdkEvent   *event,
		      gint	  nclicks)
{
  BdkEvent temp_event;
  BdkEvent *event_copy;
  GList *link;
  
  g_return_if_fail (event != NULL);
  
  temp_event = *event;
  temp_event.type = (nclicks == 2) ? BDK_2BUTTON_PRESS : BDK_3BUTTON_PRESS;

  event_copy = bdk_event_copy (&temp_event);
  link = _bdk_event_queue_append (display, event_copy);
}

void
_bdk_event_button_generate (BdkDisplay *display,
			    BdkEvent   *event)
{
  if ((event->button.time < (display->button_click_time[1] + 2*display->double_click_time)) &&
      (event->button.window == display->button_window[1]) &&
      (event->button.button == display->button_number[1]) &&
      (ABS (event->button.x - display->button_x[1]) <= display->double_click_distance) &&
      (ABS (event->button.y - display->button_y[1]) <= display->double_click_distance))
{
      bdk_synthesize_click (display, event, 3);
            
      display->button_click_time[1] = 0;
      display->button_click_time[0] = 0;
      display->button_window[1] = NULL;
      display->button_window[0] = NULL;
      display->button_number[1] = -1;
      display->button_number[0] = -1;
      display->button_x[0] = display->button_x[1] = 0;
      display->button_y[0] = display->button_y[1] = 0;
    }
  else if ((event->button.time < (display->button_click_time[0] + display->double_click_time)) &&
	   (event->button.window == display->button_window[0]) &&
	   (event->button.button == display->button_number[0]) &&
	   (ABS (event->button.x - display->button_x[0]) <= display->double_click_distance) &&
	   (ABS (event->button.y - display->button_y[0]) <= display->double_click_distance))
    {
      bdk_synthesize_click (display, event, 2);
      
      display->button_click_time[1] = display->button_click_time[0];
      display->button_click_time[0] = event->button.time;
      display->button_window[1] = display->button_window[0];
      display->button_window[0] = event->button.window;
      display->button_number[1] = display->button_number[0];
      display->button_number[0] = event->button.button;
      display->button_x[1] = display->button_x[0];
      display->button_x[0] = event->button.x;
      display->button_y[1] = display->button_y[0];
      display->button_y[0] = event->button.y;
    }
  else
    {
      display->button_click_time[1] = 0;
      display->button_click_time[0] = event->button.time;
      display->button_window[1] = NULL;
      display->button_window[0] = event->button.window;
      display->button_number[1] = -1;
      display->button_number[0] = event->button.button;
      display->button_x[1] = 0;
      display->button_x[0] = event->button.x;
      display->button_y[1] = 0;
      display->button_y[0] = event->button.y;
    }
}

void
bdk_synthesize_window_state (BdkWindow     *window,
                             BdkWindowState unset_flags,
                             BdkWindowState set_flags)
{
  BdkEvent temp_event;
  BdkWindowState old;
  
  g_return_if_fail (window != NULL);
  
  temp_event.window_state.window = window;
  temp_event.window_state.type = BDK_WINDOW_STATE;
  temp_event.window_state.send_event = FALSE;
  
  old = ((BdkWindowObject*) temp_event.window_state.window)->state;
  
  temp_event.window_state.new_window_state = old;
  temp_event.window_state.new_window_state |= set_flags;
  temp_event.window_state.new_window_state &= ~unset_flags;
  temp_event.window_state.changed_mask = temp_event.window_state.new_window_state ^ old;

  if (temp_event.window_state.new_window_state == old)
    return; /* No actual work to do, nothing changed. */

  /* Actually update the field in BdkWindow, this is sort of an odd
   * place to do it, but seems like the safest since it ensures we expose no
   * inconsistent state to the user.
   */
  
  ((BdkWindowObject*) window)->state = temp_event.window_state.new_window_state;

  if (temp_event.window_state.changed_mask & BDK_WINDOW_STATE_WITHDRAWN)
    _bdk_window_update_viewable (window);

  /* We only really send the event to toplevels, since
   * all the window states don't apply to non-toplevels.
   * Non-toplevels do use the BDK_WINDOW_STATE_WITHDRAWN flag
   * internally so we needed to update window->state.
   */
  switch (((BdkWindowObject*) window)->window_type)
    {
    case BDK_WINDOW_TOPLEVEL:
    case BDK_WINDOW_DIALOG:
    case BDK_WINDOW_TEMP: /* ? */
      bdk_display_put_event (bdk_drawable_get_display (window), &temp_event);
      break;
      
    case BDK_WINDOW_FOREIGN:
    case BDK_WINDOW_ROOT:
    case BDK_WINDOW_CHILD:
      break;
    }
}

/**
 * bdk_display_set_double_click_time:
 * @display: a #BdkDisplay
 * @msec: double click time in milliseconds (thousandths of a second) 
 * 
 * Sets the double click time (two clicks within this time interval
 * count as a double click and result in a #BDK_2BUTTON_PRESS event).
 * Applications should <emphasis>not</emphasis> set this, it is a global 
 * user-configured setting.
 *
 * Since: 2.2
 **/
void
bdk_display_set_double_click_time (BdkDisplay *display,
				   guint       msec)
{
  display->double_click_time = msec;
}

/**
 * bdk_set_double_click_time:
 * @msec: double click time in milliseconds (thousandths of a second)
 *
 * Set the double click time for the default display. See
 * bdk_display_set_double_click_time(). 
 * See also bdk_display_set_double_click_distance().
 * Applications should <emphasis>not</emphasis> set this, it is a 
 * global user-configured setting.
 **/
void
bdk_set_double_click_time (guint msec)
{
  bdk_display_set_double_click_time (bdk_display_get_default (), msec);
}

/**
 * bdk_display_set_double_click_distance:
 * @display: a #BdkDisplay
 * @distance: distance in pixels
 * 
 * Sets the double click distance (two clicks within this distance
 * count as a double click and result in a #BDK_2BUTTON_PRESS event).
 * See also bdk_display_set_double_click_time().
 * Applications should <emphasis>not</emphasis> set this, it is a global 
 * user-configured setting.
 *
 * Since: 2.4
 **/
void
bdk_display_set_double_click_distance (BdkDisplay *display,
				       guint       distance)
{
  display->double_click_distance = distance;
}

GType
bdk_event_get_type (void)
{
  static GType our_type = 0;
  
  if (our_type == 0)
    our_type = g_boxed_type_register_static (g_intern_static_string ("BdkEvent"),
					     (GBoxedCopyFunc)bdk_event_copy,
					     (GBoxedFreeFunc)bdk_event_free);
  return our_type;
}

/**
 * bdk_setting_get:
 * @name: the name of the setting.
 * @value: location to store the value of the setting.
 *
 * Obtains a desktop-wide setting, such as the double-click time,
 * for the default screen. See bdk_screen_get_setting().
 *
 * Returns: %TRUE if the setting existed and a value was stored
 *   in @value, %FALSE otherwise.
 **/
gboolean
bdk_setting_get (const gchar *name,
		 BValue      *value)
{
  return bdk_screen_get_setting (bdk_screen_get_default (), name, value);
}

#define __BDK_EVENTS_C__
#include "bdkaliasdef.c"
