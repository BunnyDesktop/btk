/* BDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1999 Peter Mattis, Spencer Kimball and Josh MacDonald
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

#include "bdkdnd.h"
#include "bdkproperty.h"
#include "bdkalias.h"

typedef struct _BdkDragContextPrivate BdkDragContextPrivate;

typedef enum
{
  BDK_DRAG_STATUS_DRAG,
  BDK_DRAG_STATUS_MOTION_WAIT,
  BDK_DRAG_STATUS_ACTION_WAIT,
  BDK_DRAG_STATUS_DROP
} BtkDragStatus;

/* Structure that holds information about a drag in progress.
 * this is used on both source and destination sides.
 */
struct _BdkDragContextPrivate
{
  BdkAtom local_selection;

  guint16 last_x;		/* Coordinates from last event */
  guint16 last_y;
  guint   drag_status : 4;	/* current status of drag      */
};

/* Drag Contexts */

static GList          *contexts          = NULL;
static BdkDragContext *current_dest_drag = NULL;


#define BDK_DRAG_CONTEXT_PRIVATE_DATA(ctx) ((BdkDragContextPrivate *) BDK_DRAG_CONTEXT (ctx)->windowing_data)

static void bdk_drag_context_finalize (GObject *object);

G_DEFINE_TYPE (BdkDragContext, bdk_drag_context, G_TYPE_OBJECT)

static void
bdk_drag_context_init (BdkDragContext *dragcontext)
{
  BdkDragContextPrivate *private;

  private = G_TYPE_INSTANCE_GET_PRIVATE (dragcontext,
                                         BDK_TYPE_DRAG_CONTEXT,
                                         BdkDragContextPrivate);

  dragcontext->windowing_data = private;

  contexts = g_list_prepend (contexts, dragcontext);
}

static void
bdk_drag_context_class_init (BdkDragContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bdk_drag_context_finalize;

  g_type_class_add_private (object_class, sizeof (BdkDragContextPrivate));
}

static void
bdk_drag_context_finalize (GObject *object)
{
  BdkDragContext *context = BDK_DRAG_CONTEXT (object);

  g_list_free (context->targets);

  if (context->source_window)
    g_object_unref (context->source_window);

  if (context->dest_window)
    g_object_unref (context->dest_window);

  contexts = g_list_remove (contexts, context);

  G_OBJECT_CLASS (bdk_drag_context_parent_class)->finalize (object);
}

BdkDragContext *
bdk_drag_context_new (void)
{
  return g_object_new (bdk_drag_context_get_type (), NULL);
}

void
bdk_drag_context_ref (BdkDragContext *context)
{
  g_object_ref (context);
}

void
bdk_drag_context_unref (BdkDragContext *context)
{
  g_object_unref (context);
}

static BdkDragContext *
bdk_drag_context_find (gboolean     is_source,
		       BdkWindow   *source,
		       BdkWindow   *dest)
{
  BdkDragContext        *context;
  BdkDragContextPrivate *private;
  GList                 *list;

  for (list = contexts; list; list = list->next)
    {
      context = (BdkDragContext *) list->data;
      private = BDK_DRAG_CONTEXT_PRIVATE_DATA (context);

      if ((!context->is_source == !is_source) &&
	  ((source == NULL) ||
           (context->source_window && (context->source_window == source))) &&
	  ((dest == NULL) ||
           (context->dest_window && (context->dest_window == dest))))
	  return context;
    }

  return NULL;
}


/************************** Public API ***********************/

void
_bdk_dnd_init (void)
{
}

/* Source side */

static void
local_send_leave (BdkDragContext  *context,
		  guint32          time)
{
  BdkEvent event;

  if ((current_dest_drag != NULL) &&
      (current_dest_drag->protocol == BDK_DRAG_PROTO_LOCAL) &&
      (current_dest_drag->source_window == context->source_window))
    {
      event.dnd.type       = BDK_DRAG_LEAVE;
      event.dnd.window     = context->dest_window;
      /* Pass ownership of context to the event */
      event.dnd.context    = current_dest_drag;
      event.dnd.send_event = FALSE;
      event.dnd.time       = time;

      current_dest_drag = NULL;

      bdk_event_put (&event);
    }
}

static void
local_send_enter (BdkDragContext *context,
		  guint32         time)
{
  BdkDragContextPrivate *private;
  BdkDragContext        *new_context;
  BdkEvent event;

  private = BDK_DRAG_CONTEXT_PRIVATE_DATA (context);

  if (!private->local_selection)
    private->local_selection = bdk_atom_intern ("LocalDndSelection", FALSE);

  if (current_dest_drag != NULL)
    {
      g_object_unref (current_dest_drag);
      current_dest_drag = NULL;
    }

  new_context = bdk_drag_context_new ();
  new_context->protocol  = BDK_DRAG_PROTO_LOCAL;
  new_context->is_source = FALSE;

  new_context->source_window = g_object_ref (context->source_window);

  new_context->dest_window   = g_object_ref (context->dest_window);

  new_context->targets = g_list_copy (context->targets);

  bdk_window_set_events (new_context->source_window,
			 bdk_window_get_events (new_context->source_window) |
			 BDK_PROPERTY_CHANGE_MASK);
  new_context->actions = context->actions;

  event.dnd.type       = BDK_DRAG_ENTER;
  event.dnd.window     = context->dest_window;
  event.dnd.send_event = FALSE;
  event.dnd.context    = new_context;
  event.dnd.time       = time;

  current_dest_drag = new_context;

  (BDK_DRAG_CONTEXT_PRIVATE_DATA (new_context))->local_selection =
    private->local_selection;

  bdk_event_put (&event);
}

static void
local_send_motion (BdkDragContext  *context,
		    gint            x_root,
		    gint            y_root,
		    BdkDragAction   action,
		    guint32         time)
{
  BdkEvent event;

  if ((current_dest_drag != NULL) &&
      (current_dest_drag->protocol == BDK_DRAG_PROTO_LOCAL) &&
      (current_dest_drag->source_window == context->source_window))
    {
      event.dnd.type       = BDK_DRAG_MOTION;
      event.dnd.window     = current_dest_drag->dest_window;
      event.dnd.send_event = FALSE;
      event.dnd.context    = current_dest_drag;
      event.dnd.time       = time;
      event.dnd.x_root     = x_root;
      event.dnd.y_root     = y_root;

      current_dest_drag->suggested_action = action;
      current_dest_drag->actions          = action;

      (BDK_DRAG_CONTEXT_PRIVATE_DATA (current_dest_drag))->last_x = x_root;
      (BDK_DRAG_CONTEXT_PRIVATE_DATA (current_dest_drag))->last_y = y_root;

      BDK_DRAG_CONTEXT_PRIVATE_DATA (context)->drag_status = BDK_DRAG_STATUS_MOTION_WAIT;

      bdk_event_put (&event);
    }
}

static void
local_send_drop (BdkDragContext *context,
                 guint32         time)
{
  BdkEvent event;

  if ((current_dest_drag != NULL) &&
      (current_dest_drag->protocol == BDK_DRAG_PROTO_LOCAL) &&
      (current_dest_drag->source_window == context->source_window))
    {
      BdkDragContextPrivate *private;
      private = BDK_DRAG_CONTEXT_PRIVATE_DATA (current_dest_drag);

      event.dnd.type       = BDK_DROP_START;
      event.dnd.window     = current_dest_drag->dest_window;
      event.dnd.send_event = FALSE;
      event.dnd.context    = current_dest_drag;
      event.dnd.time       = time;
      event.dnd.x_root     = private->last_x;
      event.dnd.y_root     = private->last_y;

      bdk_event_put (&event);
    }
}

static void
bdk_drag_do_leave (BdkDragContext *context,
                   guint32         time)
{
  if (context->dest_window)
    {
      switch (context->protocol)
	{
	case BDK_DRAG_PROTO_LOCAL:
	  local_send_leave (context, time);
	  break;

	default:
	  break;
	}

      g_object_unref (context->dest_window);
      context->dest_window = NULL;
    }
}

BdkDragContext *
bdk_drag_begin (BdkWindow *window,
		GList     *targets)
{
  GList          *list;
  BdkDragContext *new_context;

  g_return_val_if_fail (window != NULL, NULL);

  g_object_ref (window);

  new_context = bdk_drag_context_new ();
  new_context->is_source     = TRUE;
  new_context->source_window = window;
  new_context->targets       = NULL;
  new_context->actions       = 0;

  for (list = targets; list; list = list->next)
    new_context->targets = g_list_append (new_context->targets, list->data);

  return new_context;
}

guint32
bdk_drag_get_protocol_for_display(BdkDisplay *display, guint32          xid,
                                   BdkDragProtocol *protocol)
{
  BdkWindow *window;

  window = bdk_window_lookup ((BdkNativeWindow) xid);

  if (window &&
      GPOINTER_TO_INT (bdk_drawable_get_data (window, "bdk-dnd-registered")))
    {
      *protocol = BDK_DRAG_PROTO_LOCAL;
      return xid;
    }

  *protocol = BDK_DRAG_PROTO_NONE;
  return 0;
}

void
bdk_drag_find_window_for_screen (BdkDragContext   *context,
                                 BdkWindow        *drag_window,
                                 BdkScreen        *screen,
                                 gint              x_root,
                                 gint              y_root,
                                 BdkWindow       **dest_window,
                                 BdkDragProtocol  *protocol)
{
  BdkWindow *dest;

  g_return_if_fail (context != NULL);

  dest = bdk_window_get_pointer (NULL, &x_root, &y_root, NULL);

  if (context->dest_window != dest)
    {
      guint32 recipient;

      /* Check if new destination accepts drags, and which protocol */
      if ((recipient = bdk_drag_get_protocol (BDK_WINDOW_DFB_ID (dest),
                                              protocol)))
	{
	  *dest_window = bdk_window_lookup ((BdkNativeWindow) recipient);
	  if (dest_window)
            g_object_ref (*dest_window);
	}
      else
	*dest_window = NULL;
    }
  else
    {
      *dest_window = context->dest_window;
      if (*dest_window)
	g_object_ref (*dest_window);

      *protocol = context->protocol;
    }
}

gboolean
bdk_drag_motion (BdkDragContext  *context,
		 BdkWindow       *dest_window,
		 BdkDragProtocol  protocol,
		 gint             x_root,
		 gint             y_root,
		 BdkDragAction    suggested_action,
		 BdkDragAction    possible_actions,
		 guint32          time)
{
  BdkDragContextPrivate *private;

  g_return_val_if_fail (context != NULL, FALSE);

  private = BDK_DRAG_CONTEXT_PRIVATE_DATA (context);

  if (context->dest_window != dest_window)
    {
      BdkEvent  event;

      /* Send a leave to the last destination */
      bdk_drag_do_leave (context, time);
      private->drag_status = BDK_DRAG_STATUS_DRAG;

      /* Check if new destination accepts drags, and which protocol */
      if (dest_window)
	{
	  context->dest_window = g_object_ref (dest_window);
	  context->protocol = protocol;

	  switch (protocol)
	    {
	    case BDK_DRAG_PROTO_LOCAL:
	      local_send_enter (context, time);
	      break;

	    default:
	      break;
	    }
	  context->suggested_action = suggested_action;
	}
      else
	{
	  context->dest_window = NULL;
	  context->action = 0;
	}

      /* Push a status event, to let the client know that
       * the drag changed
       */

      event.dnd.type       = BDK_DRAG_STATUS;
      event.dnd.window     = context->source_window;
      /* We use this to signal a synthetic status. Perhaps
       * we should use an extra field...
       */
      event.dnd.send_event = TRUE;
      event.dnd.context    = context;
      event.dnd.time       = time;

      bdk_event_put (&event);
    }
  else
    {
      context->suggested_action = suggested_action;
    }

  /* Send a drag-motion event */

  private->last_x = x_root;
  private->last_y = y_root;

  if (context->dest_window)
    {
      if (private->drag_status == BDK_DRAG_STATUS_DRAG)
	{
	  switch (context->protocol)
	    {
	    case BDK_DRAG_PROTO_LOCAL:
	      local_send_motion (context,
                                 x_root, y_root, suggested_action, time);
	      break;

	    case BDK_DRAG_PROTO_NONE:
	      g_warning ("BDK_DRAG_PROTO_NONE is not valid in bdk_drag_motion()");
	      break;
	    default:
	      break;
	    }
	}
      else
	return TRUE;
    }

  return FALSE;
}

void
bdk_drag_drop (BdkDragContext *context,
	       guint32         time)
{
  g_return_if_fail (context != NULL);

  if (context->dest_window)
    {
      switch (context->protocol)
	{
	case BDK_DRAG_PROTO_LOCAL:
	  local_send_drop (context, time);
	  break;
	case BDK_DRAG_PROTO_NONE:
	  g_warning ("BDK_DRAG_PROTO_NONE is not valid in bdk_drag_drop()");
	  break;
	default:
	  break;
	}
    }
}

void
bdk_drag_abort (BdkDragContext *context,
		guint32         time)
{
  g_return_if_fail (context != NULL);

  bdk_drag_do_leave (context, time);
}

/* Destination side */

void
bdk_drag_status (BdkDragContext   *context,
		 BdkDragAction     action,
		 guint32           time)
{
  BdkDragContextPrivate *private;
  BdkDragContext        *src_context;
  BdkEvent event;

  g_return_if_fail (context != NULL);

  private = BDK_DRAG_CONTEXT_PRIVATE_DATA (context);

  src_context = bdk_drag_context_find (TRUE,
				       context->source_window,
				       context->dest_window);

  if (src_context)
    {
      BdkDragContextPrivate *private;

      private = BDK_DRAG_CONTEXT_PRIVATE_DATA (src_context);

      if (private->drag_status == BDK_DRAG_STATUS_MOTION_WAIT)
	private->drag_status = BDK_DRAG_STATUS_DRAG;

      event.dnd.type       = BDK_DRAG_STATUS;
      event.dnd.window     = src_context->source_window;
      event.dnd.send_event = FALSE;
      event.dnd.context    = src_context;
      event.dnd.time       = time;

      src_context->action = action;

      bdk_event_put (&event);
    }
}

void
bdk_drop_reply (BdkDragContext   *context,
		gboolean          ok,
		guint32           time)
{
  g_return_if_fail (context != NULL);
}

void
bdk_drop_finish (BdkDragContext   *context,
		 gboolean          success,
		 guint32           time)
{
  BdkDragContextPrivate *private;
  BdkDragContext        *src_context;
  BdkEvent event;

  g_return_if_fail (context != NULL);

  private = BDK_DRAG_CONTEXT_PRIVATE_DATA (context);

  src_context = bdk_drag_context_find (TRUE,
				       context->source_window,
				       context->dest_window);
  if (src_context)
    {
      g_object_ref (src_context);

      event.dnd.type       = BDK_DROP_FINISHED;
      event.dnd.window     = src_context->source_window;
      event.dnd.send_event = FALSE;
      event.dnd.context    = src_context;

      bdk_event_put (&event);
    }
}

gboolean
bdk_drag_drop_succeeded (BdkDragContext *context)
{
	g_warning("bdk_drag_drop_succeeded unimplemented \n");
	return TRUE;
}

void
bdk_window_register_dnd (BdkWindow      *window)
{
  g_return_if_fail (window != NULL);

  if (GPOINTER_TO_INT (bdk_drawable_get_data (window, "bdk-dnd-registered")))
    return;

  bdk_drawable_set_data (window, "bdk-dnd-registered",
                         GINT_TO_POINTER (TRUE), NULL);
}

/*************************************************************
 * bdk_drag_get_selection:
 *     Returns the selection atom for the current source window
 *   arguments:
 *
 *   results:
 *************************************************************/

BdkAtom
bdk_drag_get_selection (BdkDragContext *context)
{
  g_return_val_if_fail (context != NULL, BDK_NONE);

  if (context->protocol == BDK_DRAG_PROTO_LOCAL)
    return (BDK_DRAG_CONTEXT_PRIVATE_DATA (context))->local_selection;
  else
    return BDK_NONE;
}

#define __BDK_DND_X11_C__
#include "bdkaliasdef.c"
