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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include "bdk.h"		/* For bdk_rectangle_intersect */
#include "bdkprivate-x11.h"
#include "bdkx.h"
#include "bdkrebunnyion.h"
#include "bdkinternals.h"
#include "bdkscreen-x11.h"
#include "bdkdisplay-x11.h"
#include "bdkwindow-x11.h"
#include "bdkalias.h"

typedef struct _BdkWindowQueueItem BdkWindowQueueItem;
typedef struct _BdkWindowParentPos BdkWindowParentPos;

typedef enum {
  BDK_WINDOW_QUEUE_TRANSLATE,
  BDK_WINDOW_QUEUE_ANTIEXPOSE
} BdkWindowQueueType;

struct _BdkWindowQueueItem
{
  BdkWindow *window;
  gulong serial;
  BdkWindowQueueType type;
  union {
    struct {
      BdkRebunnyion *area;
      gint dx;
      gint dy;
    } translate;
    struct {
      BdkRebunnyion *area;
    } antiexpose;
  } u;
};

void
_bdk_window_move_resize_child (BdkWindow *window,
			       gint       x,
			       gint       y,
			       gint       width,
			       gint       height)
{
  BdkWindowObject *obj;

  g_return_if_fail (window != NULL);
  g_return_if_fail (BDK_IS_WINDOW (window));

  obj = BDK_WINDOW_OBJECT (window);

  if (width > 65535 ||
      height > 65535)
    {
      g_warning ("Native children wider or taller than 65535 pixels are not supported");

      if (width > 65535)
	width = 65535;
      if (height > 65535)
	height = 65535;
    }

  obj->x = x;
  obj->y = y;
  obj->width = width;
  obj->height = height;

  /* We don't really care about origin overflow, because on overflow
     the window won't be visible anyway and thus it will be shaped
     to nothing */

  _bdk_x11_window_tmp_unset_parent_bg (window);
  _bdk_x11_window_tmp_unset_bg (window, TRUE);
  XMoveResizeWindow (BDK_WINDOW_XDISPLAY (window),
		     BDK_WINDOW_XID (window),
		     obj->x + obj->parent->abs_x,
		     obj->y + obj->parent->abs_y,
		     width, height);
  _bdk_x11_window_tmp_reset_parent_bg (window);
  _bdk_x11_window_tmp_reset_bg (window, TRUE);
}

static Bool
expose_serial_predicate (Display *xdisplay,
			 XEvent  *xev,
			 XPointer arg)
{
  gulong *serial = (gulong *)arg;

  if (xev->xany.type == Expose || xev->xany.type == GraphicsExpose)
    *serial = MIN (*serial, xev->xany.serial);

  return False;
}

/* Find oldest possible serial for an outstanding expose event
 */
static gulong
find_current_serial (Display *xdisplay)
{
  XEvent xev;
  gulong serial = NextRequest (xdisplay);
  
  XSync (xdisplay, False);

  XCheckIfEvent (xdisplay, &xev, expose_serial_predicate, (XPointer)&serial);

  return serial;
}

static void
queue_delete_link (GQueue *queue,
		   GList  *link)
{
  if (queue->tail == link)
    queue->tail = link->prev;
  
  queue->head = g_list_remove_link (queue->head, link);
  g_list_free_1 (link);
  queue->length--;
}

static void
queue_item_free (BdkWindowQueueItem *item)
{
  if (item->window)
    {
      g_object_remove_weak_pointer (G_OBJECT (item->window),
				    (gpointer *)&(item->window));
    }
  
  if (item->type == BDK_WINDOW_QUEUE_ANTIEXPOSE)
    bdk_rebunnyion_destroy (item->u.antiexpose.area);
  else
    {
      if (item->u.translate.area)
	bdk_rebunnyion_destroy (item->u.translate.area);
    }
  
  g_free (item);
}

static void
bdk_window_queue (BdkWindow          *window,
		  BdkWindowQueueItem *item)
{
  BdkDisplayX11 *display_x11 = BDK_DISPLAY_X11 (BDK_WINDOW_DISPLAY (window));
  
  if (!display_x11->translate_queue)
    display_x11->translate_queue = g_queue_new ();

  /* Keep length of queue finite by, if it grows too long,
   * figuring out the latest relevant serial and discarding
   * irrelevant queue items.
   */
  if (display_x11->translate_queue->length >= 64)
    {
      gulong serial = find_current_serial (BDK_WINDOW_XDISPLAY (window));
      GList *tmp_list = display_x11->translate_queue->head;
      
      while (tmp_list)
	{
	  BdkWindowQueueItem *item = tmp_list->data;
	  GList *next = tmp_list->next;
	  
	  /* an overflow-safe (item->serial < serial) */
	  if (item->serial - serial > (gulong) G_MAXLONG)
	    {
	      queue_delete_link (display_x11->translate_queue, tmp_list);
	      queue_item_free (item);
	    }

	  tmp_list = next;
	}
    }

  /* Catch the case where someone isn't processing events and there
   * is an event stuck in the event queue with an old serial:
   * If we can't reduce the queue length by the above method,
   * discard anti-expose items. (We can't discard translate
   * items 
   */
  if (display_x11->translate_queue->length >= 64)
    {
      GList *tmp_list = display_x11->translate_queue->head;
      
      while (tmp_list)
	{
	  BdkWindowQueueItem *item = tmp_list->data;
	  GList *next = tmp_list->next;
	  
	  if (item->type == BDK_WINDOW_QUEUE_ANTIEXPOSE)
	    {
	      queue_delete_link (display_x11->translate_queue, tmp_list);
	      queue_item_free (item);
	    }

	  tmp_list = next;
	}
    }

  item->window = window;
  item->serial = NextRequest (BDK_WINDOW_XDISPLAY (window));
  
  g_object_add_weak_pointer (G_OBJECT (window),
			     (gpointer *)&(item->window));

  g_queue_push_tail (display_x11->translate_queue, item);
}

void
_bdk_x11_window_queue_translation (BdkWindow *window,
				   BdkGC     *gc,
				   BdkRebunnyion *area,
				   gint       dx,
				   gint       dy)
{
  BdkWindowQueueItem *item = g_new (BdkWindowQueueItem, 1);
  item->type = BDK_WINDOW_QUEUE_TRANSLATE;
  item->u.translate.area = area ? bdk_rebunnyion_copy (area) : NULL;
  item->u.translate.dx = dx;
  item->u.translate.dy = dy;

  /* Ensure that the gc is flushed so that we get the right
     serial from NextRequest in bdk_window_queue, i.e. the
     the serial for the XCopyArea, not the ones from flushing
     the gc. */
  _bdk_x11_gc_flush (gc);
  bdk_window_queue (window, item);
}

gboolean
_bdk_x11_window_queue_antiexpose (BdkWindow *window,
				  BdkRebunnyion *area)
{
  BdkWindowQueueItem *item = g_new (BdkWindowQueueItem, 1);
  item->type = BDK_WINDOW_QUEUE_ANTIEXPOSE;
  item->u.antiexpose.area = area;

  bdk_window_queue (window, item);

  return TRUE;
}

void
_bdk_window_process_expose (BdkWindow    *window,
			    gulong        serial,
			    BdkRectangle *area)
{
  BdkRebunnyion *invalidate_rebunnyion = bdk_rebunnyion_rectangle (area);
  BdkDisplayX11 *display_x11 = BDK_DISPLAY_X11 (BDK_WINDOW_DISPLAY (window));

  if (display_x11->translate_queue)
    {
      GList *tmp_list = display_x11->translate_queue->head;

      while (tmp_list)
	{
	  BdkWindowQueueItem *item = tmp_list->data;
          GList *next = tmp_list->next;

	  /* an overflow-safe (serial < item->serial) */
	  if (serial - item->serial > (gulong) G_MAXLONG)
	    {
	      if (item->window == window)
		{
		  if (item->type == BDK_WINDOW_QUEUE_TRANSLATE)
		    {
		      if (item->u.translate.area)
			{
			  BdkRebunnyion *intersection;

			  intersection = bdk_rebunnyion_copy (invalidate_rebunnyion);
			  bdk_rebunnyion_intersect (intersection, item->u.translate.area);
			  bdk_rebunnyion_subtract (invalidate_rebunnyion, intersection);
			  bdk_rebunnyion_offset (intersection, item->u.translate.dx, item->u.translate.dy);
			  bdk_rebunnyion_union (invalidate_rebunnyion, intersection);
			  bdk_rebunnyion_destroy (intersection);
			}
		      else
			bdk_rebunnyion_offset (invalidate_rebunnyion, item->u.translate.dx, item->u.translate.dy);
		    }
		  else		/* anti-expose */
		    {
		      bdk_rebunnyion_subtract (invalidate_rebunnyion, item->u.antiexpose.area);
		    }
		}
	    }
	  else
	    {
	      queue_delete_link (display_x11->translate_queue, tmp_list);
	      queue_item_free (item);
	    }
	  tmp_list = next;
	}
    }

  if (!bdk_rebunnyion_empty (invalidate_rebunnyion))
    _bdk_window_invalidate_for_expose (window, invalidate_rebunnyion);

  bdk_rebunnyion_destroy (invalidate_rebunnyion);
}

#define __BDK_GEOMETRY_X11_C__
#include "bdkaliasdef.c"
