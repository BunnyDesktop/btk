/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1999 Peter Mattis, Spencer Kimball and Josh MacDonald
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

#include <stdlib.h>
#include <string.h>

#include "bdkconfig.h"

#include "bdk/bdkkeysyms.h"

#include "btkdnd.h"
#include "btkiconfactory.h"
#include "btkicontheme.h"
#include "btkimage.h"
#include "btkinvisible.h"
#include "btkmain.h"
#include "btkplug.h"
#include "btkstock.h"
#include "btkwindow.h"
#include "btkintl.h"
#include "btkquartz.h"
#include "btkalias.h"
#include "bdk/quartz/bdkquartz.h"

typedef struct _BtkDragSourceSite BtkDragSourceSite;
typedef struct _BtkDragSourceInfo BtkDragSourceInfo;
typedef struct _BtkDragDestSite BtkDragDestSite;
typedef struct _BtkDragDestInfo BtkDragDestInfo;
typedef struct _BtkDragFindData BtkDragFindData;

static void     btk_drag_find_widget            (BtkWidget        *widget,
						 BtkDragFindData  *data);
static void     btk_drag_dest_site_destroy      (gpointer          data);
static void     btk_drag_dest_leave             (BtkWidget        *widget,
						 BdkDragContext   *context,
						 guint             time);
static BtkDragDestInfo *btk_drag_get_dest_info  (BdkDragContext   *context,
						 gboolean          create);
static void btk_drag_source_site_destroy        (gpointer           data);

static BtkDragSourceInfo *btk_drag_get_source_info (BdkDragContext *context,
						    gboolean        create);

extern BdkDragContext *bdk_quartz_drag_source_context (); /* bdk/quartz/bdkdnd-quartz.c */

struct _BtkDragSourceSite 
{
  BdkModifierType    start_button_mask;
  BtkTargetList     *target_list;        /* Targets for drag data */
  BdkDragAction      actions;            /* Possible actions */

  /* Drag icon */
  BtkImageType icon_type;
  union
  {
    BtkImagePixmapData pixmap;
    BtkImagePixbufData pixbuf;
    BtkImageStockData stock;
    BtkImageIconNameData name;
  } icon_data;
  BdkBitmap *icon_mask;

  BdkColormap       *colormap;	         /* Colormap for drag icon */

  /* Stored button press information to detect drag beginning */
  gint               state;
  gint               x, y;
};

struct _BtkDragSourceInfo 
{
  BtkWidget         *source_widget;
  BtkWidget         *widget;
  BtkTargetList     *target_list; /* Targets for drag data */
  BdkDragAction      possible_actions; /* Actions allowed by source */
  BdkDragContext    *context;	  /* drag context */
  NSEvent           *nsevent;     /* what started it */
  gint hot_x, hot_y;		  /* Hot spot for drag */
  BdkPixbuf         *icon_pixbuf;
  gboolean           success;
  gboolean           delete;
};

struct _BtkDragDestSite 
{
  BtkDestDefaults    flags;
  BtkTargetList     *target_list;
  BdkDragAction      actions;
  guint              have_drag : 1;
  guint              track_motion : 1;
};

struct _BtkDragDestInfo 
{
  BtkWidget         *widget;	   /* Widget in which drag is in */
  BdkDragContext    *context;	   /* Drag context */
  guint              dropped : 1;     /* Set after we receive a drop */
  gint               drop_x, drop_y; /* Position of drop */
};

struct _BtkDragFindData 
{
  gint x;
  gint y;
  BdkDragContext *context;
  BtkDragDestInfo *info;
  gboolean found;
  gboolean toplevel;
  gboolean (*callback) (BtkWidget *widget, BdkDragContext *context,
			gint x, gint y, guint32 time);
  guint32 time;
};


@interface BtkDragSourceOwner : NSObject {
  BtkDragSourceInfo *info;
}

@end

@implementation BtkDragSourceOwner
-(void)pasteboard:(NSPasteboard *)sender provideDataForType:(NSString *)type
{
  guint target_info;
  BtkSelectionData selection_data;

  selection_data.selection = BDK_NONE;
  selection_data.data = NULL;
  selection_data.length = -1;
  selection_data.target = bdk_quartz_pasteboard_type_to_atom_libbtk_only (type);
  selection_data.display = bdk_display_get_default ();

  if (btk_target_list_find (info->target_list, 
			    selection_data.target, 
			    &target_info)) 
    {
      g_signal_emit_by_name (info->widget, "drag-data-get",
			     info->context,
			     &selection_data,
			     target_info,
			     time);

      if (selection_data.length >= 0)
        _btk_quartz_set_selection_data_for_pasteboard (sender, &selection_data);
      
      g_free (selection_data.data);
    }
}

- (id)initWithInfo:(BtkDragSourceInfo *)anInfo
{
  self = [super init];

  if (self) 
    {
      info = anInfo;
    }

  return self;
}

@end

void 
btk_drag_get_data (BtkWidget      *widget,
		   BdkDragContext *context,
		   BdkAtom         target,
		   guint32         time)
{
  id <NSDraggingInfo> dragging_info;
  NSPasteboard *pasteboard;
  BtkSelectionData *selection_data;
  BtkDragDestInfo *info;
  BtkDragDestSite *site;

  dragging_info = bdk_quartz_drag_context_get_dragging_info_libbtk_only (context);
  pasteboard = [dragging_info draggingPasteboard];

  info = btk_drag_get_dest_info (context, FALSE);
  site = g_object_get_data (G_OBJECT (widget), "btk-drag-dest");

  selection_data = _btk_quartz_get_selection_data_from_pasteboard (pasteboard,
								   target, 0);

  if (site && site->target_list)
    {
      guint target_info;
      
      if (btk_target_list_find (site->target_list, 
				selection_data->target,
				&target_info))
	{
	  if (!(site->flags & BTK_DEST_DEFAULT_DROP) ||
	      selection_data->length >= 0)
	    g_signal_emit_by_name (widget,
				   "drag-data-received",
				   context, info->drop_x, info->drop_y,
				   selection_data,
				   target_info, time);
	}
    }
  else
    {
      g_signal_emit_by_name (widget,
			     "drag-data-received",
			     context, info->drop_x, info->drop_y,
			     selection_data,
			     0, time);
    }
  
  if (site && site->flags & BTK_DEST_DEFAULT_DROP)
    {
      btk_drag_finish (context, 
		       (selection_data->length >= 0),
		       (context->action == BDK_ACTION_MOVE),
		       time);
    }      
}

void 
btk_drag_finish (BdkDragContext *context,
		 gboolean        success,
		 gboolean        del,
		 guint32         time)
{
  BtkDragSourceInfo *info;
  BdkDragContext* source_context = bdk_quartz_drag_source_context ();

  if (source_context)
    {
      info = btk_drag_get_source_info (source_context, FALSE);
      if (info)
        {
          info->success = success;
          info->delete = del;
        }
    }
}

static void
btk_drag_dest_info_destroy (gpointer data)
{
  BtkDragDestInfo *info = data;

  g_free (info);
}

static BtkDragDestInfo *
btk_drag_get_dest_info (BdkDragContext *context,
			gboolean        create)
{
  BtkDragDestInfo *info;
  static GQuark info_quark = 0;
  if (!info_quark)
    info_quark = g_quark_from_static_string ("btk-dest-info");
  
  info = g_object_get_qdata (G_OBJECT (context), info_quark);
  if (!info && create)
    {
      info = g_new (BtkDragDestInfo, 1);
      info->widget = NULL;
      info->context = context;
      info->dropped = FALSE;
      g_object_set_qdata_full (G_OBJECT (context), info_quark,
			       info, btk_drag_dest_info_destroy);
    }

  return info;
}

static GQuark dest_info_quark = 0;

static BtkDragSourceInfo *
btk_drag_get_source_info (BdkDragContext *context,
			  gboolean        create)
{
  BtkDragSourceInfo *info;

  if (!dest_info_quark)
    dest_info_quark = g_quark_from_static_string ("btk-source-info");
  
  info = g_object_get_qdata (G_OBJECT (context), dest_info_quark);
  if (!info && create)
    {
      info = g_new0 (BtkDragSourceInfo, 1);
      info->context = context;
      g_object_set_qdata (G_OBJECT (context), dest_info_quark, info);
    }

  return info;
}

static void
btk_drag_clear_source_info (BdkDragContext *context)
{
  g_object_set_qdata (G_OBJECT (context), dest_info_quark, NULL);
}

BtkWidget *
btk_drag_get_source_widget (BdkDragContext *context)
{
  BtkDragSourceInfo *info;
  BdkDragContext* real_source_context = bdk_quartz_drag_source_context();

  if (!real_source_context)
    return NULL;

  info = btk_drag_get_source_info (real_source_context, FALSE);
  if (!info)
     return NULL;

  return info->source_widget;
}

/*************************************************************
 * btk_drag_highlight_expose:
 *     Callback for expose_event for highlighted widgets.
 *   arguments:
 *     widget:
 *     event:
 *     data:
 *   results:
 *************************************************************/

static gboolean
btk_drag_highlight_expose (BtkWidget      *widget,
			   BdkEventExpose *event,
			   gpointer        data)
{
  gint x, y, width, height;
  
  if (btk_widget_is_drawable (widget))
    {
      bairo_t *cr;
      
      if (!btk_widget_get_has_window (widget))
	{
	  x = widget->allocation.x;
	  y = widget->allocation.y;
	  width = widget->allocation.width;
	  height = widget->allocation.height;
	}
      else
	{
	  x = 0;
	  y = 0;
	  width = bdk_window_get_width (widget->window);
	  height = bdk_window_get_height (widget->window);
	}
      
      btk_paint_shadow (widget->style, widget->window,
		        BTK_STATE_NORMAL, BTK_SHADOW_OUT,
		        NULL, widget, "dnd",
			x, y, width, height);

      cr = bdk_bairo_create (widget->window);
      bairo_set_source_rgb (cr, 0.0, 0.0, 0.0); /* black */
      bairo_set_line_width (cr, 1.0);
      bairo_rectangle (cr,
		       x + 0.5, y + 0.5,
		       width - 1, height - 1);
      bairo_stroke (cr);
      bairo_destroy (cr);
    }

  return FALSE;
}

/*************************************************************
 * btk_drag_highlight:
 *     Highlight the given widget in the default manner.
 *   arguments:
 *     widget:
 *   results:
 *************************************************************/

void 
btk_drag_highlight (BtkWidget  *widget)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  g_signal_connect_after (widget, "expose-event",
			  G_CALLBACK (btk_drag_highlight_expose),
			  NULL);

  btk_widget_queue_draw (widget);
}

/*************************************************************
 * btk_drag_unhighlight:
 *     Refresh the given widget to remove the highlight.
 *   arguments:
 *     widget:
 *   results:
 *************************************************************/

void 
btk_drag_unhighlight (BtkWidget *widget)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  g_signal_handlers_disconnect_by_func (widget,
					btk_drag_highlight_expose,
					NULL);
  
  btk_widget_queue_draw (widget);
}

static NSWindow *
get_toplevel_nswindow (BtkWidget *widget)
{
  BtkWidget *toplevel = btk_widget_get_toplevel (widget);
  
  if (btk_widget_is_toplevel (toplevel) && toplevel->window)
    return [bdk_quartz_window_get_nsview (toplevel->window) window];
  else
    return NULL;
}

static void
register_types (BtkWidget *widget, BtkDragDestSite *site)
{
  if (site->target_list)
    {
      NSWindow *nswindow = get_toplevel_nswindow (widget);
      NSSet *types;
      NSAutoreleasePool *pool;

      if (!nswindow)
	return;

      pool = [[NSAutoreleasePool alloc] init];
      types = _btk_quartz_target_list_to_pasteboard_types (site->target_list);

      [nswindow registerForDraggedTypes:[types allObjects]];

      [types release];
      [pool release];
    }
}

static void
btk_drag_dest_realized (BtkWidget *widget, 
			gpointer   user_data)
{
  BtkDragDestSite *site = user_data;

  register_types (widget, site);
}

static void
btk_drag_dest_hierarchy_changed (BtkWidget *widget,
				 BtkWidget *previous_toplevel,
				 gpointer   user_data)
{
  BtkDragDestSite *site = user_data;

  register_types (widget, site);
}

static void
btk_drag_dest_site_destroy (gpointer data)
{
  BtkDragDestSite *site = data;
    
  if (site->target_list)
    btk_target_list_unref (site->target_list);

  g_free (site);
}

void 
btk_drag_dest_set (BtkWidget            *widget,
		   BtkDestDefaults       flags,
		   const BtkTargetEntry *targets,
		   gint                  n_targets,
		   BdkDragAction         actions)
{
  BtkDragDestSite *old_site, *site;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  old_site = g_object_get_data (G_OBJECT (widget), "btk-drag-dest");

  site = g_new (BtkDragDestSite, 1);
  site->flags = flags;
  site->have_drag = FALSE;
  if (targets)
    site->target_list = btk_target_list_new (targets, n_targets);
  else
    site->target_list = NULL;
  site->actions = actions;

  if (old_site)
    site->track_motion = old_site->track_motion;
  else
    site->track_motion = FALSE;

  btk_drag_dest_unset (widget);

  if (btk_widget_get_realized (widget))
    btk_drag_dest_realized (widget, site);

  g_signal_connect (widget, "realize",
		    G_CALLBACK (btk_drag_dest_realized), site);
  g_signal_connect (widget, "hierarchy-changed",
		    G_CALLBACK (btk_drag_dest_hierarchy_changed), site);

  g_object_set_data_full (G_OBJECT (widget), I_("btk-drag-dest"),
			  site, btk_drag_dest_site_destroy);
}

void 
btk_drag_dest_set_proxy (BtkWidget      *widget,
			 BdkWindow      *proxy_window,
			 BdkDragProtocol protocol,
			 gboolean        use_coordinates)
{
  g_warning ("btk_drag_dest_set_proxy is not supported on Mac OS X.");
}

void 
btk_drag_dest_unset (BtkWidget *widget)
{
  BtkDragDestSite *old_site;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  old_site = g_object_get_data (G_OBJECT (widget), "btk-drag-dest");
  if (old_site)
    {
      g_signal_handlers_disconnect_by_func (widget,
                                            btk_drag_dest_realized,
                                            old_site);
      g_signal_handlers_disconnect_by_func (widget,
                                            btk_drag_dest_hierarchy_changed,
                                            old_site);
    }

  g_object_set_data (G_OBJECT (widget), I_("btk-drag-dest"), NULL);
}

BtkTargetList*
btk_drag_dest_get_target_list (BtkWidget *widget)
{
  BtkDragDestSite *site;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);
  
  site = g_object_get_data (G_OBJECT (widget), "btk-drag-dest");

  return site ? site->target_list : NULL;  
}

void
btk_drag_dest_set_target_list (BtkWidget      *widget,
                               BtkTargetList  *target_list)
{
  BtkDragDestSite *site;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  
  site = g_object_get_data (G_OBJECT (widget), "btk-drag-dest");
  
  if (!site)
    {
      g_warning ("Can't set a target list on a widget until you've called btk_drag_dest_set() "
                 "to make the widget into a drag destination");
      return;
    }

  if (target_list)
    btk_target_list_ref (target_list);
  
  if (site->target_list)
    btk_target_list_unref (site->target_list);

  site->target_list = target_list;

  register_types (widget, site);
}

void
btk_drag_dest_add_text_targets (BtkWidget *widget)
{
  BtkTargetList *target_list;

  target_list = btk_drag_dest_get_target_list (widget);
  if (target_list)
    btk_target_list_ref (target_list);
  else
    target_list = btk_target_list_new (NULL, 0);
  btk_target_list_add_text_targets (target_list, 0);
  btk_drag_dest_set_target_list (widget, target_list);
  btk_target_list_unref (target_list);
}

void
btk_drag_dest_add_image_targets (BtkWidget *widget)
{
  BtkTargetList *target_list;

  target_list = btk_drag_dest_get_target_list (widget);
  if (target_list)
    btk_target_list_ref (target_list);
  else
    target_list = btk_target_list_new (NULL, 0);
  btk_target_list_add_image_targets (target_list, 0, FALSE);
  btk_drag_dest_set_target_list (widget, target_list);
  btk_target_list_unref (target_list);
}

void
btk_drag_dest_add_uri_targets (BtkWidget *widget)
{
  BtkTargetList *target_list;

  target_list = btk_drag_dest_get_target_list (widget);
  if (target_list)
    btk_target_list_ref (target_list);
  else
    target_list = btk_target_list_new (NULL, 0);
  btk_target_list_add_uri_targets (target_list, 0);
  btk_drag_dest_set_target_list (widget, target_list);
  btk_target_list_unref (target_list);
}

static void
prepend_and_ref_widget (BtkWidget *widget,
			gpointer   data)
{
  GSList **slist_p = data;

  *slist_p = g_slist_prepend (*slist_p, g_object_ref (widget));
}

static void
btk_drag_find_widget (BtkWidget       *widget,
		      BtkDragFindData *data)
{
  BtkAllocation new_allocation;
  gint allocation_to_window_x = 0;
  gint allocation_to_window_y = 0;
  gint x_offset = 0;
  gint y_offset = 0;

  if (data->found || !btk_widget_get_mapped (widget) || !btk_widget_get_sensitive (widget))
    return;

  /* Note that in the following code, we only count the
   * position as being inside a WINDOW widget if it is inside
   * widget->window; points that are outside of widget->window
   * but within the allocation are not counted. This is consistent
   * with the way we highlight drag targets.
   *
   * data->x,y are relative to widget->parent->window (if
   * widget is not a toplevel, widget->window otherwise).
   * We compute the allocation of widget in the same coordinates,
   * clipping to widget->window, and all intermediate
   * windows. If data->x,y is inside that, then we translate
   * our coordinates to be relative to widget->window and
   * recurse.
   */  
  new_allocation = widget->allocation;

  if (widget->parent)
    {
      gint tx, ty;
      BdkWindow *window = widget->window;

      /* Compute the offset from allocation-relative to
       * window-relative coordinates.
       */
      allocation_to_window_x = widget->allocation.x;
      allocation_to_window_y = widget->allocation.y;

      if (btk_widget_get_has_window (widget))
	{
	  /* The allocation is relative to the parent window for
	   * window widgets, not to widget->window.
	   */
          bdk_window_get_position (window, &tx, &ty);
	  
          allocation_to_window_x -= tx;
          allocation_to_window_y -= ty;
	}

      new_allocation.x = 0 + allocation_to_window_x;
      new_allocation.y = 0 + allocation_to_window_y;
      
      while (window && window != widget->parent->window)
	{
	  BdkRectangle window_rect = { 0, 0, 0, 0 };
	  
	  window_rect.width = bdk_window_get_width (window);
	  window_rect.height = bdk_window_get_height (window);

	  bdk_rectangle_intersect (&new_allocation, &window_rect, &new_allocation);

	  bdk_window_get_position (window, &tx, &ty);
	  new_allocation.x += tx;
	  x_offset += tx;
	  new_allocation.y += ty;
	  y_offset += ty;
	  
	  window = bdk_window_get_parent (window);
	}

      if (!window)		/* Window and widget heirarchies didn't match. */
	return;
    }

  if (data->toplevel ||
      ((data->x >= new_allocation.x) && (data->y >= new_allocation.y) &&
       (data->x < new_allocation.x + new_allocation.width) && 
       (data->y < new_allocation.y + new_allocation.height)))
    {
      /* First, check if the drag is in a valid drop site in
       * one of our children 
       */
      if (BTK_IS_CONTAINER (widget))
	{
	  BtkDragFindData new_data = *data;
	  GSList *children = NULL;
	  GSList *tmp_list;
	  
	  new_data.x -= x_offset;
	  new_data.y -= y_offset;
	  new_data.found = FALSE;
	  new_data.toplevel = FALSE;
	  
	  /* need to reference children temporarily in case the
	   * ::drag-motion/::drag-drop callbacks change the widget hierarchy.
	   */
	  btk_container_forall (BTK_CONTAINER (widget), prepend_and_ref_widget, &children);
	  for (tmp_list = children; tmp_list; tmp_list = tmp_list->next)
	    {
	      if (!new_data.found && btk_widget_is_drawable (tmp_list->data))
		btk_drag_find_widget (tmp_list->data, &new_data);
	      g_object_unref (tmp_list->data);
	    }
	  g_slist_free (children);
	  
	  data->found = new_data.found;
	}

      /* If not, and this widget is registered as a drop site, check to
       * emit "drag-motion" to check if we are actually in
       * a drop site.
       */
      if (!data->found &&
	  g_object_get_data (G_OBJECT (widget), "btk-drag-dest"))
	{
	  data->found = data->callback (widget,
					data->context,
					data->x - x_offset - allocation_to_window_x,
					data->y - y_offset - allocation_to_window_y,
					data->time);
	  /* If so, send a "drag-leave" to the last widget */
	  if (data->found)
	    {
	      if (data->info->widget && data->info->widget != widget)
		{
		  btk_drag_dest_leave (data->info->widget, data->context, data->time);
		}
	      data->info->widget = widget;
	    }
	}
    }
}

static void  
btk_drag_dest_leave (BtkWidget      *widget,
		     BdkDragContext *context,
		     guint           time)
{
  BtkDragDestSite *site;

  site = g_object_get_data (G_OBJECT (widget), "btk-drag-dest");
  g_return_if_fail (site != NULL);

  if ((site->flags & BTK_DEST_DEFAULT_HIGHLIGHT) && site->have_drag)
    btk_drag_unhighlight (widget);
  
  if (!(site->flags & BTK_DEST_DEFAULT_MOTION) || site->have_drag ||
      site->track_motion)
    g_signal_emit_by_name (widget, "drag-leave", context, time);
  
  site->have_drag = FALSE;
}

static gboolean
btk_drag_dest_motion (BtkWidget	     *widget,
		      BdkDragContext *context,
		      gint            x,
		      gint            y,
		      guint           time)
{
  BtkDragDestSite *site;
  BdkDragAction action = 0;
  gboolean retval;

  site = g_object_get_data (G_OBJECT (widget), "btk-drag-dest");
  g_return_val_if_fail (site != NULL, FALSE);

  if (site->track_motion || site->flags & BTK_DEST_DEFAULT_MOTION)
    {
      if (context->suggested_action & site->actions)
	action = context->suggested_action;
      
      if (action && btk_drag_dest_find_target (widget, context, NULL))
	{
	  if (!site->have_drag)
	    {
	      site->have_drag = TRUE;
	      if (site->flags & BTK_DEST_DEFAULT_HIGHLIGHT)
		btk_drag_highlight (widget);
	    }
	  
	  bdk_drag_status (context, action, time);
	}
      else
	{
	  bdk_drag_status (context, 0, time);
	  if (!site->track_motion)
	    return TRUE;
	}
    }

  g_signal_emit_by_name (widget, "drag-motion",
			 context, x, y, time, &retval);

  return (site->flags & BTK_DEST_DEFAULT_MOTION) ? TRUE : retval;
}

static gboolean
btk_drag_dest_drop (BtkWidget	     *widget,
		    BdkDragContext   *context,
		    gint              x,
		    gint              y,
		    guint             time)
{
  BtkDragDestSite *site;
  BtkDragDestInfo *info;
  gboolean retval;

  site = g_object_get_data (G_OBJECT (widget), "btk-drag-dest");
  g_return_val_if_fail (site != NULL, FALSE);

  info = btk_drag_get_dest_info (context, FALSE);
  g_return_val_if_fail (info != NULL, FALSE);

  info->drop_x = x;
  info->drop_y = y;

  if (site->flags & BTK_DEST_DEFAULT_DROP)
    {
      BdkAtom target = btk_drag_dest_find_target (widget, context, NULL);

      if (target == BDK_NONE)
	{
	  btk_drag_finish (context, FALSE, FALSE, time);
	  return TRUE;
	}
      else
	btk_drag_get_data (widget, context, target, time);
    }
  
  g_signal_emit_by_name (widget, "drag-drop",
			 context, x, y, time, &retval);

  return (site->flags & BTK_DEST_DEFAULT_DROP) ? TRUE : retval;
}

void
btk_drag_dest_set_track_motion (BtkWidget *widget,
				gboolean   track_motion)
{
  BtkDragDestSite *site;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  site = g_object_get_data (G_OBJECT (widget), "btk-drag-dest");
  
  g_return_if_fail (site != NULL);

  site->track_motion = track_motion != FALSE;
}

gboolean
btk_drag_dest_get_track_motion (BtkWidget *widget)
{
  BtkDragDestSite *site;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  site = g_object_get_data (G_OBJECT (widget), "btk-drag-dest");

  if (site)
    return site->track_motion;

  return FALSE;
}

void
_btk_drag_dest_handle_event (BtkWidget *toplevel,
			     BdkEvent  *event)
{
  BtkDragDestInfo *info;
  BdkDragContext *context;

  g_return_if_fail (toplevel != NULL);
  g_return_if_fail (event != NULL);

  context = event->dnd.context;

  info = btk_drag_get_dest_info (context, TRUE);

  /* Find the widget for the event */
  switch (event->type)
    {
    case BDK_DRAG_ENTER:
      break;

    case BDK_DRAG_LEAVE:
      if (info->widget)
	{
	  btk_drag_dest_leave (info->widget, context, event->dnd.time);
	  info->widget = NULL;
	}
      break;

    case BDK_DRAG_MOTION:
    case BDK_DROP_START:
      {
	BtkDragFindData data;
	gint tx, ty;

	if (event->type == BDK_DROP_START)
	  {
	    info->dropped = TRUE;
	    /* We send a leave here so that the widget unhighlights
	     * properly.
	     */
	    if (info->widget)
	      {
		btk_drag_dest_leave (info->widget, context, event->dnd.time);
		info->widget = NULL;
	      }
	  }

	bdk_window_get_position (toplevel->window, &tx, &ty);
	
	data.x = event->dnd.x_root - tx;
	data.y = event->dnd.y_root - ty;
 	data.context = context;
	data.info = info;
	data.found = FALSE;
	data.toplevel = TRUE;
	data.callback = (event->type == BDK_DRAG_MOTION) ?
	  btk_drag_dest_motion : btk_drag_dest_drop;
	data.time = event->dnd.time;
	
	btk_drag_find_widget (toplevel, &data);

	if (info->widget && !data.found)
	  {
	    btk_drag_dest_leave (info->widget, context, event->dnd.time);
	    info->widget = NULL;
	  }

	/* Send a reply.
	 */
	if (event->type == BDK_DRAG_MOTION)
	  {
	    if (!data.found)
	      bdk_drag_status (context, 0, event->dnd.time);
	  }

	break;
      default:
	g_assert_not_reached ();
      }
    }
}


BdkAtom
btk_drag_dest_find_target (BtkWidget      *widget,
                           BdkDragContext *context,
                           BtkTargetList  *target_list)
{
  id <NSDraggingInfo> dragging_info;
  NSPasteboard *pasteboard;
  BtkWidget *source_widget;
  GList *tmp_target;
  GList *tmp_source = NULL;
  GList *source_targets;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), BDK_NONE);
  g_return_val_if_fail (BDK_IS_DRAG_CONTEXT (context), BDK_NONE);
  g_return_val_if_fail (!context->is_source, BDK_NONE);

  dragging_info = bdk_quartz_drag_context_get_dragging_info_libbtk_only (context);
  pasteboard = [dragging_info draggingPasteboard];

  source_widget = btk_drag_get_source_widget (context);

  if (target_list == NULL)
    target_list = btk_drag_dest_get_target_list (widget);
  
  if (target_list == NULL)
    return BDK_NONE;

  source_targets = _btk_quartz_pasteboard_types_to_atom_list ([pasteboard types]);
  tmp_target = target_list->list;
  while (tmp_target)
    {
      BtkTargetPair *pair = tmp_target->data;
      tmp_source = source_targets;
      while (tmp_source)
	{
	  if (tmp_source->data == GUINT_TO_POINTER (pair->target))
	    {
	      if ((!(pair->flags & BTK_TARGET_SAME_APP) || source_widget) &&
		  (!(pair->flags & BTK_TARGET_SAME_WIDGET) || (source_widget == widget)))
		{
		  g_list_free (source_targets);
		  return pair->target;
		}
	      else
		break;
	    }
	  tmp_source = tmp_source->next;
	}
      tmp_target = tmp_target->next;
    }

  g_list_free (source_targets);
  return BDK_NONE;
}

static gboolean
btk_drag_begin_idle (gpointer arg)
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  BdkDragContext* context = (BdkDragContext*) arg;
  BtkDragSourceInfo* info = btk_drag_get_source_info (context, FALSE);
  NSWindow *nswindow;
  NSPasteboard *pasteboard;
  BtkDragSourceOwner *owner;
  NSPoint point;
  NSSet *types;
  NSImage *drag_image;

  g_assert (info != NULL);

  pasteboard = [NSPasteboard pasteboardWithName:NSDragPboard];
  owner = [[BtkDragSourceOwner alloc] initWithInfo:info];

  types = _btk_quartz_target_list_to_pasteboard_types (info->target_list);

  [pasteboard declareTypes:[types allObjects] owner:owner];

  [owner release];
  [types release];

  if ((nswindow = get_toplevel_nswindow (info->source_widget)) == NULL)
     return FALSE;
  
  /* Ref the context. It's unreffed when the drag has been aborted */
  g_object_ref (info->context);

  /* FIXME: If the event isn't a mouse event, use the global cursor position instead */
  point = [info->nsevent locationInWindow];

  /* Account for the given hotspot position. The y position must be
   * corrected to the NSWindow coordinate system.
   */
  point.x -= info->hot_x;
  point.y += -(bdk_pixbuf_get_height (info->icon_pixbuf) - info->hot_y);

  drag_image = _btk_quartz_create_image_from_pixbuf (info->icon_pixbuf);
  if (drag_image == NULL)
    {
      g_object_unref (info->context);
      return FALSE;
    }

  [nswindow dragImage:drag_image
                   at:point
               offset:NSZeroSize
                event:info->nsevent
           pasteboard:pasteboard
               source:nswindow
            slideBack:YES];

  [info->nsevent release];
  [drag_image release];

  [pool release];

  return FALSE;
}

static BdkDragContext *
btk_drag_begin_internal (BtkWidget         *widget,
			 BtkDragSourceSite *site,
			 BtkTargetList     *target_list,
			 BdkDragAction      actions,
			 gint               button,
			 BdkEvent          *event)
{
  BtkDragSourceInfo *info;
  BdkDragContext *context;
  NSWindow *nswindow = get_toplevel_nswindow (widget);
  NSPoint point = {0, 0};
  gdouble x, y;
  double time = (double)g_get_real_time ();
  NSEvent *nsevent;
  NSTimeInterval nstime;

  if (event)
    {
      if (bdk_event_get_coords (event, &x, &y))
        {
          /* We need to translate (x, y) to coordinates relative to the
           * toplevel BdkWindow, which should be the BdkWindow backing
           * nswindow. Then, we convert to the NSWindow coordinate system.
           */
          BdkWindow *window = event->any.window;
          BdkWindow *toplevel = bdk_window_get_effective_toplevel (window);

          while (window != toplevel)
            {
              double old_x = x;
              double old_y = y;

              bdk_window_coords_to_parent (window, old_x, old_y,
                                           &x, &y);
              window = bdk_window_get_effective_parent (window);
            }

          point.x = x;
          point.y = bdk_window_get_height (window) - y;
        }
      time = (double)bdk_event_get_time (event);
    }
  nstime = [[NSDate dateWithTimeIntervalSince1970: time / 1000] timeIntervalSinceReferenceDate];
  nsevent = [NSEvent mouseEventWithType: NSLeftMouseDown
        	      location: point
		      modifierFlags: 0
	              timestamp: nstime
		      windowNumber: [nswindow windowNumber]
		      context: [nswindow graphicsContext]
		      eventNumber: 0
		      clickCount: 1
	              pressure: 0.0 ];

  BdkWindow *window = [[nswindow contentView] bdkWindow];
  g_return_val_if_fail(nsevent != NULL, NULL);

  context = bdk_drag_begin (window, NULL);
  g_return_val_if_fail( context != NULL, NULL);
  context->is_source = TRUE;

  info = btk_drag_get_source_info (context, TRUE);
  info->nsevent = nsevent;
  [info->nsevent retain];

  info->source_widget = g_object_ref (widget);
  info->widget = g_object_ref (widget);
  info->target_list = target_list;
  btk_target_list_ref (target_list);

  info->possible_actions = actions;
  
  g_signal_emit_by_name (widget, "drag-begin", info->context);

  /* Ensure that we have an icon before we start the drag; the
   * application may have set one in ::drag_begin, or it may
   * not have set one.
   */
  if (!info->icon_pixbuf)
    {
      if (!site || site->icon_type == BTK_IMAGE_EMPTY)
	btk_drag_set_icon_default (context);
      else
	switch (site->icon_type)
	  {
	  case BTK_IMAGE_PIXMAP:
	    /* This is not supported, so just set a small transparent pixbuf
	     * since we need to have something.
	     */
	    if (0)
	      btk_drag_set_icon_pixmap (context,
					site->colormap,
					site->icon_data.pixmap.pixmap,
					site->icon_mask,
					-2, -2);
	    else
	      {
		BdkPixbuf *pixbuf;

		pixbuf = bdk_pixbuf_new (BDK_COLORSPACE_RGB, FALSE, 8, 1, 1);
 		bdk_pixbuf_fill (pixbuf, 0xffffff);

 		btk_drag_set_icon_pixbuf (context,
 					  pixbuf,
					  0, 0);

 		g_object_unref (pixbuf);
	      }
	    break;
	  case BTK_IMAGE_PIXBUF:
	    btk_drag_set_icon_pixbuf (context,
				      site->icon_data.pixbuf.pixbuf,
				      -2, -2);
	    break;
	  case BTK_IMAGE_STOCK:
	    btk_drag_set_icon_stock (context,
				     site->icon_data.stock.stock_id,
				     -2, -2);
	    break;
	  case BTK_IMAGE_ICON_NAME:
	    btk_drag_set_icon_name (context,
			    	    site->icon_data.name.icon_name,
				    -2, -2);
	    break;
	  case BTK_IMAGE_EMPTY:
	  default:
	    g_assert_not_reached();
	    break;
	  }
    }

  /* drag will begin in an idle handler to avoid nested run loops */

  g_idle_add_full (G_PRIORITY_HIGH_IDLE, btk_drag_begin_idle, context, NULL);

  bdk_pointer_ungrab (0);

  return context;
}

BdkDragContext *
btk_drag_begin (BtkWidget         *widget,
		BtkTargetList     *targets,
		BdkDragAction      actions,
		gint               button,
		BdkEvent          *event)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (btk_widget_get_realized (widget), NULL);
  g_return_val_if_fail (targets != NULL, NULL);

  return btk_drag_begin_internal (widget, NULL, targets,
				  actions, button, event);
}


static gboolean
btk_drag_source_event_cb (BtkWidget      *widget,
			  BdkEvent       *event,
			  gpointer        data)
{
  BtkDragSourceSite *site;
  gboolean retval = FALSE;
  site = (BtkDragSourceSite *)data;

  switch (event->type)
    {
    case BDK_BUTTON_PRESS:
      if ((BDK_BUTTON1_MASK << (event->button.button - 1)) & site->start_button_mask)
	{
	  site->state |= (BDK_BUTTON1_MASK << (event->button.button - 1));
	  site->x = event->button.x;
	  site->y = event->button.y;
	}
      break;
      
    case BDK_BUTTON_RELEASE:
      if ((BDK_BUTTON1_MASK << (event->button.button - 1)) & site->start_button_mask)
	site->state &= ~(BDK_BUTTON1_MASK << (event->button.button - 1));
      break;
      
    case BDK_MOTION_NOTIFY:
      if (site->state & event->motion.state & site->start_button_mask)
	{
	  /* FIXME: This is really broken and can leave us
	   * with a stuck grab
	   */
	  int i;
	  for (i=1; i<6; i++)
	    {
	      if (site->state & event->motion.state & 
		  BDK_BUTTON1_MASK << (i - 1))
		break;
	    }

	  if (btk_drag_check_threshold (widget, site->x, site->y,
					event->motion.x, event->motion.y))
	    {
	      site->state = 0;
	      btk_drag_begin_internal (widget, site, site->target_list,
				       site->actions, 
				       i, event);

	      retval = TRUE;
	    }
	}
      break;
      
    default:			/* hit for 2/3BUTTON_PRESS */
      break;
    }
  
  return retval;
}

void 
btk_drag_source_set (BtkWidget            *widget,
		     BdkModifierType       start_button_mask,
		     const BtkTargetEntry *targets,
		     gint                  n_targets,
		     BdkDragAction         actions)
{
  BtkDragSourceSite *site;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  site = g_object_get_data (G_OBJECT (widget), "btk-site-data");

  btk_widget_add_events (widget,
			 btk_widget_get_events (widget) |
			 BDK_BUTTON_PRESS_MASK | BDK_BUTTON_RELEASE_MASK |
			 BDK_BUTTON_MOTION_MASK);

  if (site)
    {
      if (site->target_list)
	btk_target_list_unref (site->target_list);
    }
  else
    {
      site = g_new0 (BtkDragSourceSite, 1);

      site->icon_type = BTK_IMAGE_EMPTY;
      
      g_signal_connect (widget, "button-press-event",
			G_CALLBACK (btk_drag_source_event_cb),
			site);
      g_signal_connect (widget, "button-release-event",
			G_CALLBACK (btk_drag_source_event_cb),
			site);
      g_signal_connect (widget, "motion-notify-event",
			G_CALLBACK (btk_drag_source_event_cb),
			site);
      
      g_object_set_data_full (G_OBJECT (widget),
			      I_("btk-site-data"), 
			      site, btk_drag_source_site_destroy);
    }

  site->start_button_mask = start_button_mask;

  site->target_list = btk_target_list_new (targets, n_targets);

  site->actions = actions;
}

/*************************************************************
 * btk_drag_source_unset
 *     Unregister this widget as a drag source.
 *   arguments:
 *     widget:
 *   results:
 *************************************************************/

void 
btk_drag_source_unset (BtkWidget *widget)
{
  BtkDragSourceSite *site;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  site = g_object_get_data (G_OBJECT (widget), "btk-site-data");

  if (site)
    {
      g_signal_handlers_disconnect_by_func (widget,
					    btk_drag_source_event_cb,
					    site);
      g_object_set_data (G_OBJECT (widget), I_("btk-site-data"), NULL);
    }
}

BtkTargetList *
btk_drag_source_get_target_list (BtkWidget *widget)
{
  BtkDragSourceSite *site;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  site = g_object_get_data (G_OBJECT (widget), "btk-site-data");

  return site ? site->target_list : NULL;

}

void
btk_drag_source_set_target_list (BtkWidget     *widget,
                                 BtkTargetList *target_list)
{
  BtkDragSourceSite *site;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  site = g_object_get_data (G_OBJECT (widget), "btk-site-data");
  if (site == NULL)
    {
      g_warning ("btk_drag_source_set_target_list() requires the widget "
		 "to already be a drag source.");
      return;
    }

  if (target_list)
    btk_target_list_ref (target_list);

  if (site->target_list)
    btk_target_list_unref (site->target_list);

  site->target_list = target_list;
}

/**
 * btk_drag_source_add_text_targets:
 * @widget: a #BtkWidget that's is a drag source
 *
 * Add the text targets supported by #BtkSelection to
 * the target list of the drag source.  The targets
 * are added with @info = 0. If you need another value, 
 * use btk_target_list_add_text_targets() and
 * btk_drag_source_set_target_list().
 * 
 * Since: 2.6
 **/
void
btk_drag_source_add_text_targets (BtkWidget *widget)
{
  BtkTargetList *target_list;

  target_list = btk_drag_source_get_target_list (widget);
  if (target_list)
    btk_target_list_ref (target_list);
  else
    target_list = btk_target_list_new (NULL, 0);
  btk_target_list_add_text_targets (target_list, 0);
  btk_drag_source_set_target_list (widget, target_list);
  btk_target_list_unref (target_list);
}

void
btk_drag_source_add_image_targets (BtkWidget *widget)
{
  BtkTargetList *target_list;

  target_list = btk_drag_source_get_target_list (widget);
  if (target_list)
    btk_target_list_ref (target_list);
  else
    target_list = btk_target_list_new (NULL, 0);
  btk_target_list_add_image_targets (target_list, 0, TRUE);
  btk_drag_source_set_target_list (widget, target_list);
  btk_target_list_unref (target_list);
}

void
btk_drag_source_add_uri_targets (BtkWidget *widget)
{
  BtkTargetList *target_list;

  target_list = btk_drag_source_get_target_list (widget);
  if (target_list)
    btk_target_list_ref (target_list);
  else
    target_list = btk_target_list_new (NULL, 0);
  btk_target_list_add_uri_targets (target_list, 0);
  btk_drag_source_set_target_list (widget, target_list);
  btk_target_list_unref (target_list);
}

static void
btk_drag_source_unset_icon (BtkDragSourceSite *site)
{
  switch (site->icon_type)
    {
    case BTK_IMAGE_EMPTY:
      break;
    case BTK_IMAGE_PIXMAP:
      if (site->icon_data.pixmap.pixmap)
	g_object_unref (site->icon_data.pixmap.pixmap);
      if (site->icon_mask)
	g_object_unref (site->icon_mask);
      break;
    case BTK_IMAGE_PIXBUF:
      g_object_unref (site->icon_data.pixbuf.pixbuf);
      break;
    case BTK_IMAGE_STOCK:
      g_free (site->icon_data.stock.stock_id);
      break;
    case BTK_IMAGE_ICON_NAME:
      g_free (site->icon_data.name.icon_name);
      break;
    default:
      g_assert_not_reached();
      break;
    }
  site->icon_type = BTK_IMAGE_EMPTY;
  
  if (site->colormap)
    g_object_unref (site->colormap);
  site->colormap = NULL;
}

static void 
btk_drag_source_site_destroy (gpointer data)
{
  BtkDragSourceSite *site = data;

  if (site->target_list)
    btk_target_list_unref (site->target_list);

  btk_drag_source_unset_icon (site);
  g_free (site);
}

void 
btk_drag_source_set_icon (BtkWidget     *widget,
			  BdkColormap   *colormap,
			  BdkPixmap     *pixmap,
			  BdkBitmap     *mask)
{
  BtkDragSourceSite *site;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (BDK_IS_COLORMAP (colormap));
  g_return_if_fail (BDK_IS_PIXMAP (pixmap));
  g_return_if_fail (!mask || BDK_IS_PIXMAP (mask));

  site = g_object_get_data (G_OBJECT (widget), "btk-site-data");
  g_return_if_fail (site != NULL);
  
  g_object_ref (colormap);
  g_object_ref (pixmap);
  if (mask)
    g_object_ref (mask);

  btk_drag_source_unset_icon (site);

  site->icon_type = BTK_IMAGE_PIXMAP;
  
  site->icon_data.pixmap.pixmap = pixmap;
  site->icon_mask = mask;
  site->colormap = colormap;
}

void 
btk_drag_source_set_icon_pixbuf (BtkWidget   *widget,
				 BdkPixbuf   *pixbuf)
{
  BtkDragSourceSite *site;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (BDK_IS_PIXBUF (pixbuf));

  site = g_object_get_data (G_OBJECT (widget), "btk-site-data");
  g_return_if_fail (site != NULL); 
  g_object_ref (pixbuf);

  btk_drag_source_unset_icon (site);

  site->icon_type = BTK_IMAGE_PIXBUF;
  site->icon_data.pixbuf.pixbuf = pixbuf;
}

/**
 * btk_drag_source_set_icon_stock:
 * @widget: a #BtkWidget
 * @stock_id: the ID of the stock icon to use
 *
 * Sets the icon that will be used for drags from a particular source
 * to a stock icon. 
 **/
void 
btk_drag_source_set_icon_stock (BtkWidget   *widget,
				const gchar *stock_id)
{
  BtkDragSourceSite *site;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (stock_id != NULL);

  site = g_object_get_data (G_OBJECT (widget), "btk-site-data");
  g_return_if_fail (site != NULL);
  
  btk_drag_source_unset_icon (site);

  site->icon_type = BTK_IMAGE_STOCK;
  site->icon_data.stock.stock_id = g_strdup (stock_id);
}

/**
 * btk_drag_source_set_icon_name:
 * @widget: a #BtkWidget
 * @icon_name: name of icon to use
 * 
 * Sets the icon that will be used for drags from a particular source
 * to a themed icon. See the docs for #BtkIconTheme for more details.
 *
 * Since: 2.8
 **/
void 
btk_drag_source_set_icon_name (BtkWidget   *widget,
			       const gchar *icon_name)
{
  BtkDragSourceSite *site;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (icon_name != NULL);

  site = g_object_get_data (G_OBJECT (widget), "btk-site-data");
  g_return_if_fail (site != NULL);

  btk_drag_source_unset_icon (site);

  site->icon_type = BTK_IMAGE_ICON_NAME;
  site->icon_data.name.icon_name = g_strdup (icon_name);
}


/**
 * btk_drag_set_icon_widget:
 * @context: the context for a drag. (This must be called 
          with a  context for the source side of a drag)
 * @widget: a toplevel window to use as an icon.
 * @hot_x: the X offset within @widget of the hotspot.
 * @hot_y: the Y offset within @widget of the hotspot.
 * 
 * Changes the icon for a widget to a given widget. BTK+
 * will not destroy the icon, so if you don't want
 * it to persist, you should connect to the "drag-end" 
 * signal and destroy it yourself.
 **/
void 
btk_drag_set_icon_widget (BdkDragContext    *context,
			  BtkWidget         *widget,
			  gint               hot_x,
			  gint               hot_y)
{
  g_return_if_fail (BDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (context->is_source);
  g_return_if_fail (BTK_IS_WIDGET (widget));

  g_warning ("btk_drag_set_icon_widget is not supported on Mac OS X");
}

static void
set_icon_stock_pixbuf (BdkDragContext    *context,
		       const gchar       *stock_id,
		       BdkPixbuf         *pixbuf,
		       gint               hot_x,
		       gint               hot_y)
{
  BtkDragSourceInfo *info;

  info = btk_drag_get_source_info (context, FALSE);

  if (stock_id)
    {
      pixbuf = btk_widget_render_icon (info->widget, stock_id,
				       BTK_ICON_SIZE_DND, NULL);

      if (!pixbuf)
	{
	  g_warning ("Cannot load drag icon from stock_id %s", stock_id);
	  return;
	}
    }
  else
    g_object_ref (pixbuf);

  if (info->icon_pixbuf)
    g_object_unref (info->icon_pixbuf);
  info->icon_pixbuf = pixbuf;
  info->hot_x = hot_x;
  info->hot_y = hot_y;
}

/**
 * btk_drag_set_icon_pixbuf:
 * @context: the context for a drag. (This must be called 
 *            with a  context for the source side of a drag)
 * @pixbuf: the #BdkPixbuf to use as the drag icon.
 * @hot_x: the X offset within @widget of the hotspot.
 * @hot_y: the Y offset within @widget of the hotspot.
 * 
 * Sets @pixbuf as the icon for a given drag.
 **/
void 
btk_drag_set_icon_pixbuf  (BdkDragContext *context,
			   BdkPixbuf      *pixbuf,
			   gint            hot_x,
			   gint            hot_y)
{
  g_return_if_fail (BDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (context->is_source);
  g_return_if_fail (BDK_IS_PIXBUF (pixbuf));

  set_icon_stock_pixbuf (context, NULL, pixbuf, hot_x, hot_y);
}

/**
 * btk_drag_set_icon_stock:
 * @context: the context for a drag. (This must be called 
 *            with a  context for the source side of a drag)
 * @stock_id: the ID of the stock icon to use for the drag.
 * @hot_x: the X offset within the icon of the hotspot.
 * @hot_y: the Y offset within the icon of the hotspot.
 * 
 * Sets the icon for a given drag from a stock ID.
 **/
void 
btk_drag_set_icon_stock  (BdkDragContext *context,
			  const gchar    *stock_id,
			  gint            hot_x,
			  gint            hot_y)
{

  g_return_if_fail (BDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (context->is_source);
  g_return_if_fail (stock_id != NULL);

  set_icon_stock_pixbuf (context, stock_id, NULL, hot_x, hot_y);
}

/**
 * btk_drag_set_icon_pixmap:
 * @context: the context for a drag. (This must be called 
 *            with a  context for the source side of a drag)
 * @colormap: the colormap of the icon 
 * @pixmap: the image data for the icon 
 * @mask: the transparency mask for the icon
 * @hot_x: the X offset within @pixmap of the hotspot.
 * @hot_y: the Y offset within @pixmap of the hotspot.
 * 
 * Sets @pixmap as the icon for a given drag. BTK+ retains
 * references for the arguments, and will release them when
 * they are no longer needed. In general, btk_drag_set_icon_pixbuf()
 * will be more convenient to use.
 **/
void 
btk_drag_set_icon_pixmap (BdkDragContext    *context,
			  BdkColormap       *colormap,
			  BdkPixmap         *pixmap,
			  BdkBitmap         *mask,
			  gint               hot_x,
			  gint               hot_y)
{
  BdkPixbuf *pixbuf;

  g_return_if_fail (BDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (context->is_source);
  g_return_if_fail (BDK_IS_COLORMAP (colormap));
  g_return_if_fail (BDK_IS_PIXMAP (pixmap));

  pixbuf = bdk_pixbuf_get_from_drawable (NULL, pixmap, colormap,
                                         0, 0, /* src */
                                         0, 0, /* dst */
                                         -1, -1);

  btk_drag_set_icon_pixbuf (context, pixbuf, hot_x, hot_y);
  g_object_unref (pixbuf);
}

/**
 * btk_drag_set_icon_name:
 * @context: the context for a drag. (This must be called 
 *            with a context for the source side of a drag)
 * @icon_name: name of icon to use
 * @hot_x: the X offset of the hotspot within the icon
 * @hot_y: the Y offset of the hotspot within the icon
 * 
 * Sets the icon for a given drag from a named themed icon. See
 * the docs for #BtkIconTheme for more details. Note that the
 * size of the icon depends on the icon theme (the icon is
 * loaded at the symbolic size #BTK_ICON_SIZE_DND), thus 
 * @hot_x and @hot_y have to be used with care.
 *
 * Since: 2.8
 **/
void 
btk_drag_set_icon_name (BdkDragContext *context,
			const gchar    *icon_name,
			gint            hot_x,
			gint            hot_y)
{
  BdkScreen *screen;
  BtkSettings *settings;
  BtkIconTheme *icon_theme;
  BdkPixbuf *pixbuf;
  gint width, height, icon_size;

  g_return_if_fail (BDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (context->is_source);
  g_return_if_fail (icon_name != NULL);

  screen = bdk_window_get_screen (context->source_window);
  g_return_if_fail (screen != NULL);

  settings = btk_settings_get_for_screen (screen);
  if (btk_icon_size_lookup_for_settings (settings,
					 BTK_ICON_SIZE_DND,
					 &width, &height))
    icon_size = MAX (width, height);
  else 
    icon_size = 32; /* default value for BTK_ICON_SIZE_DND */ 

  icon_theme = btk_icon_theme_get_for_screen (screen);

  pixbuf = btk_icon_theme_load_icon (icon_theme, icon_name,
		  		     icon_size, 0, NULL);
  if (pixbuf)
    set_icon_stock_pixbuf (context, NULL, pixbuf, hot_x, hot_y);
  else
    g_warning ("Cannot load drag icon from icon name %s", icon_name);
}

/**
 * btk_drag_set_icon_default:
 * @context: the context for a drag. (This must be called 
             with a  context for the source side of a drag)
 * 
 * Sets the icon for a particular drag to the default
 * icon.
 **/
void 
btk_drag_set_icon_default (BdkDragContext    *context)
{
  g_return_if_fail (BDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (context->is_source);

  btk_drag_set_icon_stock (context, BTK_STOCK_DND, -2, -2);
}

void 
btk_drag_set_default_icon (BdkColormap   *colormap,
			   BdkPixmap     *pixmap,
			   BdkBitmap     *mask,
			   gint           hot_x,
			   gint           hot_y)
{
  g_warning ("btk_drag_set_default_icon is not supported on Mac OS X.");
}

static void
btk_drag_source_info_destroy (BtkDragSourceInfo *info)
{
  NSPasteboard *pasteboard;
  NSAutoreleasePool *pool;

  if (info->icon_pixbuf)
    g_object_unref (info->icon_pixbuf);

  g_signal_emit_by_name (info->widget, "drag-end", 
			 info->context);

  if (info->source_widget)
    g_object_unref (info->source_widget);

  if (info->widget)
    g_object_unref (info->widget);

  btk_target_list_unref (info->target_list);

  pool = [[NSAutoreleasePool alloc] init];

  /* Empty the pasteboard, so that it will not accidentally access
   * info->context after it has been destroyed.
   */
  pasteboard = [NSPasteboard pasteboardWithName: NSDragPboard];
  [pasteboard declareTypes: nil owner: nil];

  [pool release];

  btk_drag_clear_source_info (info->context);
  g_object_unref (info->context);

  g_free (info);
  info = NULL;
}

static gboolean
drag_drop_finished_idle_cb (gpointer data)
{
  BtkDragSourceInfo* info = (BtkDragSourceInfo*) data;

  if (info->success)
    btk_drag_source_info_destroy (data);

  return FALSE;
}

static void
btk_drag_drop_finished (BtkDragSourceInfo *info,
                        BtkDragResult      result)
{
  gboolean success = (result == BTK_DRAG_RESULT_SUCCESS);

  if (!success)
    g_signal_emit_by_name (info->source_widget, "drag-failed",
                           info->context, BTK_DRAG_RESULT_NO_TARGET, &success);

  if (success && info->delete)
    g_signal_emit_by_name (info->source_widget, "drag-data-delete",
                           info->context);

  /* Workaround for the fact that the NS API blocks until the drag is
   * over. This way the context is still valid when returning from
   * drag_begin, even if it will still be quite useless. See bug #501588.
  */
  g_idle_add (drag_drop_finished_idle_cb, info);
}

/*************************************************************
 * _btk_drag_source_handle_event:
 *     Called from widget event handling code on Drag events
 *     for drag sources.
 *
 *   arguments:
 *     toplevel: Toplevel widget that received the event
 *     event:
 *   results:
 *************************************************************/

void
_btk_drag_source_handle_event (BtkWidget *widget,
			       BdkEvent  *event)
{
  BtkDragSourceInfo *info;
  BdkDragContext *context;
  BtkDragResult result;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (event != NULL);

  context = event->dnd.context;
  info = btk_drag_get_source_info (context, FALSE);
  if (!info)
    return;

  switch (event->type)
    {
    case BDK_DROP_FINISHED:
      result = (bdk_drag_context_get_dest_window (context) != NULL) ? BTK_DRAG_RESULT_SUCCESS : BTK_DRAG_RESULT_NO_TARGET;
      btk_drag_drop_finished (info, result);
      break;
    default:
      g_assert_not_reached ();
    }  
}


gboolean
btk_drag_check_threshold (BtkWidget *widget,
			  gint       start_x,
			  gint       start_y,
			  gint       current_x,
			  gint       current_y)
{
  gint drag_threshold;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  g_object_get (btk_widget_get_settings (widget),
		"btk-dnd-drag-threshold", &drag_threshold,
		NULL);
  
  return (ABS (current_x - start_x) > drag_threshold ||
	  ABS (current_y - start_y) > drag_threshold);
}

#define __BTK_DND_C__
#include "btkaliasdef.c"
