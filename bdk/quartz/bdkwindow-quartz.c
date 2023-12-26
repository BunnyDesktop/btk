/* bdkwindow-quartz.c
 *
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 2005-2007 Imendio AB
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
#include <Carbon/Carbon.h>

#include "bdk.h"
#include "bdkwindowimpl.h"
#include "bdkprivate-quartz.h"
#include "bdkscreen-quartz.h"
#include "bdkinputprivate.h"

static gpointer parent_class;

static GSList   *update_nswindows;
static gboolean  in_process_all_updates = FALSE;

static GSList *main_window_stack;

#define FULLSCREEN_DATA "fullscreen-data"

typedef struct
{
  gint            x, y;
  gint            width, height;
  BdkWMDecoration decor;
} FullscreenSavedGeometry;


static void update_toplevel_order (void);
static void clear_toplevel_order  (void);

static FullscreenSavedGeometry *get_fullscreen_geometry (BdkWindow *window);

#define WINDOW_IS_TOPLEVEL(window)		     \
  (BDK_WINDOW_TYPE (window) != BDK_WINDOW_CHILD &&   \
   BDK_WINDOW_TYPE (window) != BDK_WINDOW_FOREIGN && \
   BDK_WINDOW_TYPE (window) != BDK_WINDOW_OFFSCREEN)

static void bdk_window_impl_iface_init (BdkWindowImplIface *iface);

gboolean
bdk_quartz_window_is_quartz (BdkWindow *window)
{
  return BDK_WINDOW_IS_QUARTZ (window);
}

NSView *
bdk_quartz_window_get_nsview (BdkWindow *window)
{
  BdkWindowObject *private = (BdkWindowObject *)window;

  g_return_val_if_fail (BDK_WINDOW_IS_QUARTZ (window), NULL);

  if (BDK_WINDOW_DESTROYED (window))
    return NULL;

  return ((BdkWindowImplQuartz *)private->impl)->view;
}

NSWindow *
bdk_quartz_window_get_nswindow (BdkWindow *window)
{
  BdkWindowObject *private = (BdkWindowObject *)window;

  if (BDK_WINDOW_DESTROYED (window))
    return NULL;

  return ((BdkWindowImplQuartz *)private->impl)->toplevel;
}

static CGContextRef
bdk_window_impl_quartz_get_context (BdkDrawable *drawable,
				    gboolean     antialias)
{
  BdkDrawableImplQuartz *drawable_impl = BDK_DRAWABLE_IMPL_QUARTZ (drawable);
  BdkWindowImplQuartz *window_impl = BDK_WINDOW_IMPL_QUARTZ (drawable);
  CGContextRef cg_context;

  if (BDK_WINDOW_DESTROYED (drawable_impl->wrapper))
    return NULL;

  /* Lock focus when not called as part of a drawRect call. This
   * is needed when called from outside "real" expose events, for
   * example for synthesized expose events when realizing windows
   * and for widgets that send fake expose events like the arrow
   * buttons in spinbuttons or the position marker in rulers.
   */
  if (window_impl->in_paint_rect_count == 0)
    {
      if (![window_impl->view lockFocusIfCanDraw])
        return NULL;
    }
  if (bdk_quartz_osx_version () < BDK_OSX_YOSEMITE)
       cg_context = [[NSGraphicsContext currentContext] graphicsPort];
  else
       cg_context = [[NSGraphicsContext currentContext] CGContext];

  if (!cg_context)
    return NULL;

  CGContextSaveGState (cg_context);
  CGContextSetAllowsAntialiasing (cg_context, antialias);

  /* We'll emulate the clipping caused by double buffering here */
  if (window_impl->begin_paint_count != 0)
    {
      CGRect rect;
      CGRect *cg_rects;
      BdkRectangle *rects;
      gint n_rects, i;

      bdk_rebunnyion_get_rectangles (window_impl->paint_clip_rebunnyion,
                                 &rects, &n_rects);

      if (n_rects == 1)
        cg_rects = &rect;
      else
        cg_rects = g_new (CGRect, n_rects);

      for (i = 0; i < n_rects; i++)
        {
          cg_rects[i].origin.x = rects[i].x;
          cg_rects[i].origin.y = rects[i].y;
          cg_rects[i].size.width = rects[i].width;
          cg_rects[i].size.height = rects[i].height;
        }

      CGContextClipToRects (cg_context, cg_rects, n_rects);

      g_free (rects);
      if (cg_rects != &rect)
        g_free (cg_rects);
    }

  return cg_context;
}

static void
check_grab_unmap (BdkWindow *window)
{
  BdkDisplay *display = bdk_drawable_get_display (window);

  _bdk_display_end_pointer_grab (display, 0, window, TRUE);

  if (display->keyboard_grab.window)
    {
      BdkWindowObject *private = BDK_WINDOW_OBJECT (window);
      BdkWindowObject *tmp = BDK_WINDOW_OBJECT (display->keyboard_grab.window);

      while (tmp && tmp != private)
	tmp = tmp->parent;

      if (tmp)
	_bdk_display_unset_has_keyboard_grab (display, TRUE);
    }
}

static void
check_grab_destroy (BdkWindow *window)
{
  BdkDisplay *display = bdk_drawable_get_display (window);
  BdkPointerGrabInfo *grab;

  /* Make sure there is no lasting grab in this native window */
  grab = _bdk_display_get_last_pointer_grab (display);
  if (grab && grab->native_window == window)
    {
      /* Serials are always 0 in quartz, but for clarity: */
      grab->serial_end = grab->serial_start;
      grab->implicit_ungrab = TRUE;
    }

  if (window == display->keyboard_grab.native_window &&
      display->keyboard_grab.window != NULL)
    _bdk_display_unset_has_keyboard_grab (display, TRUE);
}

static void
bdk_window_impl_quartz_finalize (GObject *object)
{
  BdkWindowImplQuartz *impl = BDK_WINDOW_IMPL_QUARTZ (object);

  check_grab_destroy (BDK_DRAWABLE_IMPL_QUARTZ (object)->wrapper);

  if (impl->paint_clip_rebunnyion)
    bdk_rebunnyion_destroy (impl->paint_clip_rebunnyion);

  if (impl->transient_for)
    g_object_unref (impl->transient_for);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bdk_window_impl_quartz_class_init (BdkWindowImplQuartzClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BdkDrawableImplQuartzClass *drawable_quartz_class = BDK_DRAWABLE_IMPL_QUARTZ_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = bdk_window_impl_quartz_finalize;

  drawable_quartz_class->get_context = bdk_window_impl_quartz_get_context;
}

static void
bdk_window_impl_quartz_init (BdkWindowImplQuartz *impl)
{
  impl->type_hint = BDK_WINDOW_TYPE_HINT_NORMAL;
}

static void
bdk_window_impl_quartz_begin_paint_rebunnyion (BdkPaintable    *paintable,
                                           BdkWindow       *window,
					   const BdkRebunnyion *rebunnyion)
{
  BdkWindowImplQuartz *impl = BDK_WINDOW_IMPL_QUARTZ (paintable);
  BdkWindowObject *private = (BdkWindowObject*)window;
  int n_rects;
  BdkRectangle *rects = NULL;
  BdkPixmap *bg_pixmap;
  BdkRebunnyion *clipped_and_offset_rebunnyion;
  gboolean free_clipped_and_offset_rebunnyion = TRUE;

  bg_pixmap = private->bg_pixmap;

  clipped_and_offset_rebunnyion = bdk_rebunnyion_copy (rebunnyion);

  bdk_rebunnyion_intersect (clipped_and_offset_rebunnyion,
                        private->clip_rebunnyion_with_children);
  bdk_rebunnyion_offset (clipped_and_offset_rebunnyion,
                     private->abs_x, private->abs_y);

  if (impl->begin_paint_count == 0)
    {
      impl->paint_clip_rebunnyion = clipped_and_offset_rebunnyion;
      free_clipped_and_offset_rebunnyion = FALSE;
    }
  else
    bdk_rebunnyion_union (impl->paint_clip_rebunnyion, clipped_and_offset_rebunnyion);

  impl->begin_paint_count++;

  if (bg_pixmap == BDK_NO_BG)
    goto done;

  bdk_rebunnyion_get_rectangles (clipped_and_offset_rebunnyion, &rects, &n_rects);

  if (n_rects == 0)
    goto done;

  if (bg_pixmap == NULL)
    {
      CGContextRef cg_context;
      CGColorRef color;
      gint i;

      cg_context = bdk_quartz_drawable_get_context (BDK_DRAWABLE (impl), FALSE);

      if (!cg_context)
        goto done;

      color = _bdk_quartz_colormap_get_cgcolor_from_pixel (window,
                                                           private->bg_color.pixel);
      CGContextSetFillColorWithColor (cg_context, color);
      CGColorRelease (color);
 
      for (i = 0; i < n_rects; i++)
        {
          CGContextFillRect (cg_context,
                             CGRectMake (rects[i].x, rects[i].y,
                                         rects[i].width, rects[i].height));
        }

      bdk_quartz_drawable_release_context (BDK_DRAWABLE (impl), cg_context);
    }
  else
    {
      int x, y;
      int x_offset, y_offset;
      int width, height;
      BdkGC *gc;

      x_offset = y_offset = 0;

      while (window && bg_pixmap == BDK_PARENT_RELATIVE_BG)
        {
          /* If this window should have the same background as the parent,
           * fetch the parent. (And if the same goes for the parent, fetch
           * the grandparent, etc.)
           */
          x_offset += ((BdkWindowObject *) window)->x;
          y_offset += ((BdkWindowObject *) window)->y;
          window = BDK_WINDOW (((BdkWindowObject *) window)->parent);
          bg_pixmap = ((BdkWindowObject *) window)->bg_pixmap;
        }

      if (bg_pixmap == NULL || bg_pixmap == BDK_NO_BG || bg_pixmap == BDK_PARENT_RELATIVE_BG)
        {
          /* Parent relative background but the parent doesn't have a
           * pixmap.
           */ 
          goto done;
        }

      /* Note: There should be a CG API to draw tiled images, we might
       * want to look into that for this. 
       */
      gc = bdk_gc_new (BDK_DRAWABLE (impl));

      bdk_drawable_get_size (BDK_DRAWABLE (bg_pixmap), &width, &height);

      x = -x_offset;
      while (x < (rects[0].x + rects[0].width))
        {
          if (x + width >= rects[0].x)
	    {
              y = -y_offset;
              while (y < (rects[0].y + rects[0].height))
                {
                  if (y + height >= rects[0].y)
                    bdk_draw_drawable (BDK_DRAWABLE (impl), gc, bg_pixmap, 0, 0, x, y, width, height);
		  
                  y += height;
                }
            }
          x += width;
        }

      g_object_unref (gc);
    }

 done:
  if (free_clipped_and_offset_rebunnyion)
    bdk_rebunnyion_destroy (clipped_and_offset_rebunnyion);
  g_free (rects);
}

static void
bdk_window_impl_quartz_end_paint (BdkPaintable *paintable)
{
  BdkWindowImplQuartz *impl = BDK_WINDOW_IMPL_QUARTZ (paintable);

  impl->begin_paint_count--;

  if (impl->begin_paint_count == 0)
    {
      bdk_rebunnyion_destroy (impl->paint_clip_rebunnyion);
      impl->paint_clip_rebunnyion = NULL;
    }
}

void
_bdk_quartz_window_set_needs_display_in_rect (BdkWindow    *window,
                                              BdkRectangle *rect)
{
  BdkWindowObject *private;
  BdkWindowImplQuartz *impl;

  private = BDK_WINDOW_OBJECT (window);
  impl = BDK_WINDOW_IMPL_QUARTZ (private->impl);

  if (!impl->needs_display_rebunnyion)
    impl->needs_display_rebunnyion = bdk_rebunnyion_new ();

  bdk_rebunnyion_union_with_rect (impl->needs_display_rebunnyion, rect);

  [impl->view setNeedsDisplayInRect:NSMakeRect (rect->x, rect->y,
                                                rect->width, rect->height)];

}

void
_bdk_quartz_window_set_needs_display_in_rebunnyion (BdkWindow    *window,
                                                BdkRebunnyion    *rebunnyion)
{
  BdkWindowObject *private;
  BdkWindowImplQuartz *impl;
  int i, n_rects;
  BdkRectangle *rects;

  private = BDK_WINDOW_OBJECT (window);
  impl = BDK_WINDOW_IMPL_QUARTZ (private->impl);

  if (!impl->needs_display_rebunnyion)
    impl->needs_display_rebunnyion = bdk_rebunnyion_new ();

  bdk_rebunnyion_union (impl->needs_display_rebunnyion, rebunnyion);

  bdk_rebunnyion_get_rectangles (rebunnyion, &rects, &n_rects);

  for (i = 0; i < n_rects; i++)
    [impl->view setNeedsDisplayInRect:NSMakeRect (rects[i].x, rects[i].y,
                                                  rects[i].width,
                                                  rects[i].height)];

  g_free (rects);

}

void
_bdk_windowing_window_process_updates_recurse (BdkWindow *window,
                                               BdkRebunnyion *rebunnyion)
{
  /* Make sure to only flush each toplevel at most once if we're called
   * from process_all_updates.
   */
  if (in_process_all_updates)
    {
      BdkWindow *toplevel;

      toplevel = bdk_window_get_effective_toplevel (window);
      if (toplevel && WINDOW_IS_TOPLEVEL (toplevel))
        {
          BdkWindowObject *toplevel_private;
          BdkWindowImplQuartz *toplevel_impl;
          NSWindow *nswindow;

          toplevel_private = (BdkWindowObject *)toplevel;
          toplevel_impl = (BdkWindowImplQuartz *)toplevel_private->impl;
          nswindow = toplevel_impl->toplevel;

          /* In theory, we could skip the flush disabling, since we only
           * have one NSView.
           */
          if (nswindow && ![nswindow isFlushWindowDisabled]) 
            {
              [nswindow retain];
              [nswindow disableFlushWindow];
              update_nswindows = g_slist_prepend (update_nswindows, nswindow);
            }
        }
    }

  if (WINDOW_IS_TOPLEVEL (window))
    _bdk_quartz_window_set_needs_display_in_rebunnyion (window, rebunnyion);
  else
    _bdk_window_process_updates_recurse (window, rebunnyion);

  /* NOTE: I'm not sure if we should displayIfNeeded here. It slows down a
   * lot (since it triggers the beam syncing) and things seem to work
   * without it.
   */
}

void
_bdk_windowing_before_process_all_updates (void)
{
  in_process_all_updates = TRUE;

  NSDisableScreenUpdates ();
}

void
_bdk_windowing_after_process_all_updates (void)
{
  GSList *old_update_nswindows = update_nswindows;
  GSList *tmp_list = update_nswindows;

  update_nswindows = NULL;

  while (tmp_list)
    {
      NSWindow *nswindow = tmp_list->data;

      [[nswindow contentView] displayIfNeeded];

      _bdk_quartz_drawable_flush (NULL);

      /* 10.14 needs to be told that the view needs to be redrawn, see
       * https://gitlab.bunny.org/BUNNY/btk/issues/1479 */
      if (bdk_quartz_osx_version() >= BDK_OSX_MOJAVE)
           [[nswindow contentView] setNeedsDisplay:YES];
      [nswindow enableFlushWindow];
      [nswindow flushWindow];
      [nswindow release];

      tmp_list = tmp_list->next;
    }

  g_slist_free (old_update_nswindows);

  in_process_all_updates = FALSE;

  NSEnableScreenUpdates ();
}

static void
bdk_window_impl_quartz_paintable_init (BdkPaintableIface *iface)
{
  iface->begin_paint_rebunnyion = bdk_window_impl_quartz_begin_paint_rebunnyion;
  iface->end_paint = bdk_window_impl_quartz_end_paint;
}

GType
_bdk_window_impl_quartz_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      const GTypeInfo object_info =
	{
	  sizeof (BdkWindowImplQuartzClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) bdk_window_impl_quartz_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (BdkWindowImplQuartz),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) bdk_window_impl_quartz_init,
	};

      const GInterfaceInfo paintable_info = 
	{
	  (GInterfaceInitFunc) bdk_window_impl_quartz_paintable_init,
	  NULL,
	  NULL
	};

      const GInterfaceInfo window_impl_info = 
	{
	  (GInterfaceInitFunc) bdk_window_impl_iface_init,
	  NULL,
	  NULL
	};

      object_type = g_type_register_static (BDK_TYPE_DRAWABLE_IMPL_QUARTZ,
                                            "BdkWindowImplQuartz",
                                            &object_info, 0);
      g_type_add_interface_static (object_type,
				   BDK_TYPE_PAINTABLE,
				   &paintable_info);
      g_type_add_interface_static (object_type,
				   BDK_TYPE_WINDOW_IMPL,
				   &window_impl_info);
    }

  return object_type;
}

GType
_bdk_window_impl_get_type (void)
{
  return _bdk_window_impl_quartz_get_type ();
}

static const gchar *
get_default_title (void)
{
  const char *title;

  title = g_get_application_name ();
  if (!title)
    title = g_get_prgname ();

  return title;
}

static void
get_ancestor_coordinates_from_child (BdkWindow *child_window,
				     gint       child_x,
				     gint       child_y,
				     BdkWindow *ancestor_window, 
				     gint      *ancestor_x, 
				     gint      *ancestor_y)
{
  BdkWindowObject *child_private = BDK_WINDOW_OBJECT (child_window);
  BdkWindowObject *ancestor_private = BDK_WINDOW_OBJECT (ancestor_window);

  while (child_private != ancestor_private)
    {
      child_x += child_private->x;
      child_y += child_private->y;

      child_private = child_private->parent;
    }

  *ancestor_x = child_x;
  *ancestor_y = child_y;
}

void
_bdk_quartz_window_debug_highlight (BdkWindow *window, gint number)
{
  BdkWindowObject *private = BDK_WINDOW_OBJECT (window);
  gint x, y;
  gint gx, gy;
  BdkWindow *toplevel;
  gint tx, ty;
  static NSWindow *debug_window[10];
  static NSRect old_rect[10];
  NSRect rect;
  NSColor *color;

  g_return_if_fail (number >= 0 && number <= 9);

  if (window == _bdk_root)
    return;

  if (window == NULL)
    {
      if (debug_window[number])
        [debug_window[number] close];
      debug_window[number] = NULL;

      return;
    }

  toplevel = bdk_window_get_effective_toplevel (window);
  get_ancestor_coordinates_from_child (window, 0, 0, toplevel, &x, &y);

  bdk_window_get_origin (toplevel, &tx, &ty);
  x += tx;
  y += ty;

  _bdk_quartz_window_bdk_xy_to_xy (x, y + private->height,
                                   &gx, &gy);

  rect = NSMakeRect (gx, gy, private->width, private->height);

  if (debug_window[number] && NSEqualRects (rect, old_rect[number]))
    return;

  old_rect[number] = rect;

  if (debug_window[number])
    [debug_window[number] close];

  debug_window[number] = [[NSWindow alloc] initWithContentRect:rect
                                                     styleMask:NSBorderlessWindowMask
			                               backing:NSBackingStoreBuffered
			                                 defer:NO];

  switch (number)
    {
    case 0:
      color = [NSColor redColor];
      break;
    case 1:
      color = [NSColor blueColor];
      break;
    case 2:
      color = [NSColor greenColor];
      break;
    case 3:
      color = [NSColor yellowColor];
      break;
    case 4:
      color = [NSColor brownColor];
      break;
    case 5:
      color = [NSColor purpleColor];
      break;
    default:
      color = [NSColor blackColor];
      break;
    }

  [debug_window[number] setBackgroundColor:color];
  [debug_window[number] setAlphaValue:0.4];
  [debug_window[number] setOpaque:NO];
  [debug_window[number] setReleasedWhenClosed:YES];
  [debug_window[number] setIgnoresMouseEvents:YES];
  [debug_window[number] setLevel:NSFloatingWindowLevel];

  [debug_window[number] orderFront:nil];
}

gboolean
_bdk_quartz_window_is_ancestor (BdkWindow *ancestor,
                                BdkWindow *window)
{
  if (ancestor == NULL || window == NULL)
    return FALSE;

  return (bdk_window_get_parent (window) == ancestor ||
          _bdk_quartz_window_is_ancestor (ancestor, 
                                          bdk_window_get_parent (window)));
}


/* See notes on top of bdkscreen-quartz.c */
void
_bdk_quartz_window_bdk_xy_to_xy (gint  bdk_x,
                                 gint  bdk_y,
                                 gint *ns_x,
                                 gint *ns_y)
{
  BdkScreenQuartz *screen_quartz = BDK_SCREEN_QUARTZ (_bdk_screen);

  if (ns_y)
    *ns_y = screen_quartz->height - bdk_y + screen_quartz->min_y;

  if (ns_x)
    *ns_x = bdk_x + screen_quartz->min_x;
}

void
_bdk_quartz_window_xy_to_bdk_xy (gint  ns_x,
                                 gint  ns_y,
                                 gint *bdk_x,
                                 gint *bdk_y)
{
  BdkScreenQuartz *screen_quartz = BDK_SCREEN_QUARTZ (_bdk_screen);

  if (bdk_y)
    *bdk_y = screen_quartz->height - ns_y + screen_quartz->min_y;

  if (bdk_x)
    *bdk_x = ns_x - screen_quartz->min_x;
}

void
_bdk_quartz_window_nspoint_to_bdk_xy (NSPoint  point,
                                      gint    *x,
                                      gint    *y)
{
  _bdk_quartz_window_xy_to_bdk_xy (point.x, point.y,
                                   x, y);
}

static BdkWindow *
find_child_window_helper (BdkWindow *window,
			  gint       x,
			  gint       y,
			  gint       x_offset,
			  gint       y_offset)
{
  BdkWindowImplQuartz *impl;
  GList *l;

  impl = BDK_WINDOW_IMPL_QUARTZ (BDK_WINDOW_OBJECT (window)->impl);

  if (window == _bdk_root)
    update_toplevel_order ();

  for (l = impl->sorted_children; l; l = l->next)
    {
      BdkWindowObject *child_private = l->data;
      BdkWindowImplQuartz *child_impl = BDK_WINDOW_IMPL_QUARTZ (child_private->impl);
      int temp_x, temp_y;

      if (!BDK_WINDOW_IS_MAPPED (child_private))
	continue;

      temp_x = x_offset + child_private->x;
      temp_y = y_offset + child_private->y;

      /* Special-case the root window. We have to include the title
       * bar in the checks, otherwise the window below the title bar
       * will be found i.e. events punch through. (If we can find a
       * better way to deal with the events in bdkevents-quartz, this
       * might not be needed.)
       */
      if (window == _bdk_root)
        {
          NSRect frame = NSMakeRect (0, 0, 100, 100);
          NSRect content;
          NSUInteger mask;
          int titlebar_height;

          mask = [child_impl->toplevel styleMask];

          /* Get the title bar height. */
          content = [NSWindow contentRectForFrameRect:frame
                                            styleMask:mask];
          titlebar_height = frame.size.height - content.size.height;

          if (titlebar_height > 0 &&
              x >= temp_x && y >= temp_y - titlebar_height &&
              x < temp_x + child_private->width && y < temp_y)
            {
              /* The root means "unknown" i.e. a window not managed by
               * BDK.
               */
              return (BdkWindow *)_bdk_root;
            }
        }

      if (x >= temp_x && y >= temp_y &&
	  x < temp_x + child_private->width && y < temp_y + child_private->height)
	{
	  /* Look for child windows. */
	  return find_child_window_helper (l->data,
					   x, y,
					   temp_x, temp_y);
	}
    }
  
  return window;
}

/* Given a BdkWindow and coordinates relative to it, returns the
 * innermost subwindow that contains the point. If the coordinates are
 * outside the passed in window, NULL is returned.
 */
BdkWindow *
_bdk_quartz_window_find_child (BdkWindow *window,
			       gint       x,
			       gint       y)
{
  BdkWindowObject *private = BDK_WINDOW_OBJECT (window);

  if (x >= 0 && y >= 0 && x < private->width && y < private->height)
    return find_child_window_helper (window, x, y, 0, 0);

  return NULL;
}


void
_bdk_quartz_window_did_become_main (BdkWindow *window)
{
  main_window_stack = g_slist_remove (main_window_stack, window);

  if (BDK_WINDOW_OBJECT (window)->window_type != BDK_WINDOW_TEMP)
    main_window_stack = g_slist_prepend (main_window_stack, window);

  clear_toplevel_order ();
}

void
_bdk_quartz_window_did_resign_main (BdkWindow *window)
{
  BdkWindow *new_window = NULL;

  if (main_window_stack)
    new_window = main_window_stack->data;
  else
    {
      GList *toplevels;

      toplevels = bdk_window_get_toplevels ();
      if (toplevels)
        new_window = toplevels->data;
      g_list_free (toplevels);
    }

  if (new_window &&
      new_window != window &&
      BDK_WINDOW_IS_MAPPED (new_window) &&
      WINDOW_IS_TOPLEVEL (new_window))
    {
      BdkWindowObject *private = (BdkWindowObject *) new_window;
      BdkWindowImplQuartz *impl = BDK_WINDOW_IMPL_QUARTZ (private->impl);

      [impl->toplevel makeKeyAndOrderFront:impl->toplevel];
    }

  clear_toplevel_order ();
}

static NSScreen *
get_nsscreen_for_point (gint x, gint y)
{
  int i;
  NSArray *screens;
  NSScreen *screen = NULL;

  BDK_QUARTZ_ALLOC_POOL;

  screens = [NSScreen screens];

  for (i = 0; i < [screens count]; i++)
    {
      NSRect rect = [[screens objectAtIndex:i] frame];

      if (x >= rect.origin.x && x <= rect.origin.x + rect.size.width &&
          y >= rect.origin.y && y <= rect.origin.y + rect.size.height)
        {
          screen = [screens objectAtIndex:i];
          break;
        }
    }

  BDK_QUARTZ_RELEASE_POOL;

  return screen;
}

void
_bdk_window_impl_new (BdkWindow     *window,
		      BdkWindow     *real_parent,
		      BdkScreen     *screen,
		      BdkVisual     *visual,
		      BdkEventMask   event_mask,
		      BdkWindowAttr *attributes,
		      gint           attributes_mask)
{
  BdkWindowObject *private;
  BdkWindowImplQuartz *impl;
  BdkDrawableImplQuartz *draw_impl;
  BdkWindowImplQuartz *parent_impl;

  BDK_QUARTZ_ALLOC_POOL;

  private = (BdkWindowObject *)window;

  impl = g_object_new (_bdk_window_impl_get_type (), NULL);
  private->impl = (BdkDrawable *)impl;
  draw_impl = BDK_DRAWABLE_IMPL_QUARTZ (impl);
  draw_impl->wrapper = BDK_DRAWABLE (window);

  parent_impl = BDK_WINDOW_IMPL_QUARTZ (private->parent->impl);

  switch (private->window_type)
    {
    case BDK_WINDOW_TOPLEVEL:
    case BDK_WINDOW_DIALOG:
    case BDK_WINDOW_TEMP:
      if (BDK_WINDOW_TYPE (private->parent) != BDK_WINDOW_ROOT)
	{
	  /* The common code warns for this case */
          parent_impl = BDK_WINDOW_IMPL_QUARTZ (BDK_WINDOW_OBJECT (_bdk_root)->impl);
	}
    }

  if (!private->input_only)
    {
      if (attributes_mask & BDK_WA_COLORMAP)
	{
	  draw_impl->colormap = attributes->colormap;
	  g_object_ref (attributes->colormap);
	}
      else
	{
	  if (visual == bdk_screen_get_system_visual (_bdk_screen))
	    {
	      draw_impl->colormap = bdk_screen_get_system_colormap (_bdk_screen);
	      g_object_ref (draw_impl->colormap);
	    }
	  else if (visual == bdk_screen_get_rgba_visual (_bdk_screen))
	    {
	      draw_impl->colormap = bdk_screen_get_rgba_colormap (_bdk_screen);
	      g_object_ref (draw_impl->colormap);
	    }
	  else
	    {
	      draw_impl->colormap = bdk_colormap_new (visual, FALSE);
	    }
	}
    }
  else
    {
      draw_impl->colormap = bdk_screen_get_system_colormap (_bdk_screen);
      g_object_ref (draw_impl->colormap);
    }

  /* Maintain the z-ordered list of children. */
  if (private->parent != (BdkWindowObject *)_bdk_root)
    parent_impl->sorted_children = g_list_prepend (parent_impl->sorted_children, window);
  else
    clear_toplevel_order ();

  bdk_window_set_cursor (window, ((attributes_mask & BDK_WA_CURSOR) ?
				  (attributes->cursor) :
				  NULL));

  switch (attributes->window_type) 
    {
    case BDK_WINDOW_TOPLEVEL:
    case BDK_WINDOW_DIALOG:
    case BDK_WINDOW_TEMP:
      {
        NSScreen *screen;
        NSRect screen_rect;
        NSRect content_rect;
        NSUInteger style_mask;
        int nx, ny;
        const char *title;

        /* initWithContentRect will place on the mainScreen by default.
         * We want to select the screen to place on ourselves.  We need
         * to find the screen the window will be on and correct the
         * content_rect coordinates to be relative to that screen.
         */
        _bdk_quartz_window_bdk_xy_to_xy (private->x, private->y, &nx, &ny);

        screen = get_nsscreen_for_point (nx, ny);
        screen_rect = [screen frame];
        nx -= screen_rect.origin.x;
        ny -= screen_rect.origin.y;

        content_rect = NSMakeRect (nx, ny - private->height,
                                   private->width,
                                   private->height);

        if (attributes->window_type == BDK_WINDOW_TEMP ||
            attributes->type_hint == BDK_WINDOW_TYPE_HINT_SPLASHSCREEN)
          {
            style_mask = NSBorderlessWindowMask;
          }
        else
          {
            style_mask = (NSTitledWindowMask |
                          NSClosableWindowMask |
                          NSMiniaturizableWindowMask |
                          NSResizableWindowMask);
          }

	impl->toplevel = [[BdkQuartzWindow alloc] initWithContentRect:content_rect 
			                                    styleMask:style_mask
			                                      backing:NSBackingStoreBuffered
			                                        defer:NO
                                                                screen:screen];

	if (attributes_mask & BDK_WA_TITLE)
	  title = attributes->title;
	else
	  title = get_default_title ();

	bdk_window_set_title (window, title);
  
	if (draw_impl->colormap == bdk_screen_get_rgba_colormap (_bdk_screen))
	  {
	    [impl->toplevel setOpaque:NO];
	    [impl->toplevel setBackgroundColor:[NSColor clearColor]];
	  }

        content_rect.origin.x = 0;
        content_rect.origin.y = 0;

	impl->view = [[BdkQuartzView alloc] initWithFrame:content_rect];
	[impl->view setBdkWindow:window];
	[impl->toplevel setContentView:impl->view];
	[impl->view release];
      }
      break;

    case BDK_WINDOW_CHILD:
      {
	BdkWindowImplQuartz *parent_impl = BDK_WINDOW_IMPL_QUARTZ (BDK_WINDOW_OBJECT (private->parent)->impl);

	if (!private->input_only)
	  {
	    NSRect frame_rect = NSMakeRect (private->x + private->parent->abs_x,
                                            private->y + private->parent->abs_y,
                                            private->width,
                                            private->height);
	
	    impl->view = [[BdkQuartzView alloc] initWithFrame:frame_rect];
	    
	    [impl->view setBdkWindow:window];

	    /* BdkWindows should be hidden by default */
	    [impl->view setHidden:YES];
	    [parent_impl->view addSubview:impl->view];
	    [impl->view release];
	  }
      }
      break;

    default:
      g_assert_not_reached ();
    }

  BDK_QUARTZ_RELEASE_POOL;

  if (attributes_mask & BDK_WA_TYPE_HINT)
    bdk_window_set_type_hint (window, attributes->type_hint);
}

void
_bdk_quartz_window_update_position (BdkWindow *window)
{
  NSRect frame_rect;
  NSRect content_rect;
  BdkWindowObject *private = (BdkWindowObject *)window;
  BdkWindowImplQuartz *impl = BDK_WINDOW_IMPL_QUARTZ (private->impl);

  BDK_QUARTZ_ALLOC_POOL;

  frame_rect = [impl->toplevel frame];
  content_rect = [impl->toplevel contentRectForFrameRect:frame_rect];

  _bdk_quartz_window_xy_to_bdk_xy (content_rect.origin.x,
                                   content_rect.origin.y + content_rect.size.height,
                                   &private->x, &private->y);


  BDK_QUARTZ_RELEASE_POOL;
}

void
_bdk_windowing_update_window_sizes (BdkScreen *screen)
{
  GList *windows, *list;
  BdkWindowObject *private = (BdkWindowObject *)_bdk_root;

  /* The size of the root window is so that it can contain all
   * monitors attached to this machine.  The monitors are laid out
   * within this root window.  We calculate the size of the root window
   * and the positions of the different monitors in bdkscreen-quartz.c.
   *
   * This data is updated when the monitor configuration is changed.
   */
  private->x = 0;
  private->y = 0;
  private->abs_x = 0;
  private->abs_y = 0;
  private->width = bdk_screen_get_width (screen);
  private->height = bdk_screen_get_height (screen);

  windows = bdk_screen_get_toplevel_windows (screen);

  for (list = windows; list; list = list->next)
    _bdk_quartz_window_update_position (list->data);

  g_list_free (windows);
}

void
_bdk_windowing_window_init (void)
{
  BdkWindowObject *private;
  BdkWindowImplQuartz *impl;
  BdkDrawableImplQuartz *drawable_impl;

  g_assert (_bdk_root == NULL);

  _bdk_root = g_object_new (BDK_TYPE_WINDOW, NULL);

  private = (BdkWindowObject *)_bdk_root;
  private->impl = g_object_new (_bdk_window_impl_get_type (), NULL);
  private->impl_window = private;

  impl = BDK_WINDOW_IMPL_QUARTZ (BDK_WINDOW_OBJECT (_bdk_root)->impl);

  _bdk_windowing_update_window_sizes (_bdk_screen);

  private->state = 0; /* We don't want BDK_WINDOW_STATE_WITHDRAWN here */
  private->window_type = BDK_WINDOW_ROOT;
  private->depth = 24;
  private->viewable = TRUE;

  drawable_impl = BDK_DRAWABLE_IMPL_QUARTZ (private->impl);
  
  drawable_impl->wrapper = BDK_DRAWABLE (private);
  drawable_impl->colormap = bdk_screen_get_system_colormap (_bdk_screen);
  g_object_ref (drawable_impl->colormap);
}

static void
_bdk_quartz_window_destroy (BdkWindow *window,
                            gboolean   recursing,
                            gboolean   foreign_destroy)
{
  BdkWindowObject *private;
  BdkWindowImplQuartz *impl;
  BdkWindowObject *parent;

  private = BDK_WINDOW_OBJECT (window);
  impl = BDK_WINDOW_IMPL_QUARTZ (private->impl);

  main_window_stack = g_slist_remove (main_window_stack, window);

  g_list_free (impl->sorted_children);
  impl->sorted_children = NULL;

  parent = private->parent;
  if (parent)
    {
      BdkWindowImplQuartz *parent_impl = BDK_WINDOW_IMPL_QUARTZ (parent->impl);

      parent_impl->sorted_children = g_list_remove (parent_impl->sorted_children, window);
    }

  _bdk_quartz_drawable_finish (BDK_DRAWABLE (impl));

  if (!recursing && !foreign_destroy)
    {
      BDK_QUARTZ_ALLOC_POOL;

      if (impl->toplevel)
	[impl->toplevel close];
      else if (impl->view)
	[impl->view removeFromSuperview];

      BDK_QUARTZ_RELEASE_POOL;
    }
}

void
_bdk_windowing_window_destroy_foreign (BdkWindow *window)
{
  /* Foreign windows aren't supported in OSX. */
}

/* FIXME: This might be possible to simplify with client-side windows. Also
 * note that already_mapped is not used yet, see the x11 backend.
*/
static void
bdk_window_quartz_show (BdkWindow *window, gboolean already_mapped)
{
  BdkWindowObject *private = (BdkWindowObject *)window;
  BdkWindowImplQuartz *impl = BDK_WINDOW_IMPL_QUARTZ (private->impl);
  gboolean focus_on_map;

  BDK_QUARTZ_ALLOC_POOL;

  if (!BDK_WINDOW_IS_MAPPED (window))
    focus_on_map = private->focus_on_map;
  else
    focus_on_map = TRUE;

  if (impl->toplevel && WINDOW_IS_TOPLEVEL (window))
    {
      gboolean make_key;

      make_key = (private->accept_focus && focus_on_map &&
                  private->window_type != BDK_WINDOW_TEMP);

      [(BdkQuartzWindow*)impl->toplevel showAndMakeKey:make_key];
      clear_toplevel_order ();

      _bdk_quartz_events_send_map_event (window);
    }
  else
    {
      [impl->view setHidden:NO];
    }

  [impl->view setNeedsDisplay:YES];

  bdk_synthesize_window_state (window, BDK_WINDOW_STATE_WITHDRAWN, 0);

  if (private->state & BDK_WINDOW_STATE_MAXIMIZED)
    bdk_window_maximize (window);

  if (private->state & BDK_WINDOW_STATE_ICONIFIED)
    bdk_window_iconify (window);

  if (impl->transient_for && !BDK_WINDOW_DESTROYED (impl->transient_for))
    _bdk_quartz_window_attach_to_parent (window);

  BDK_QUARTZ_RELEASE_POOL;
}

/* Temporarily unsets the parent window, if the window is a
 * transient. 
 */
void
_bdk_quartz_window_detach_from_parent (BdkWindow *window)
{
  BdkWindowImplQuartz *impl;

  g_return_if_fail (BDK_IS_WINDOW (window));

  impl = BDK_WINDOW_IMPL_QUARTZ (BDK_WINDOW_OBJECT (window)->impl);
  
  g_return_if_fail (impl->toplevel != NULL);

  if (impl->transient_for && !BDK_WINDOW_DESTROYED (impl->transient_for))
    {
      BdkWindowImplQuartz *parent_impl;

      parent_impl = BDK_WINDOW_IMPL_QUARTZ (BDK_WINDOW_OBJECT (impl->transient_for)->impl);
      [parent_impl->toplevel removeChildWindow:impl->toplevel];
      clear_toplevel_order ();
    }
}

/* Re-sets the parent window, if the window is a transient. */
void
_bdk_quartz_window_attach_to_parent (BdkWindow *window)
{
  BdkWindowImplQuartz *impl;

  g_return_if_fail (BDK_IS_WINDOW (window));

  impl = BDK_WINDOW_IMPL_QUARTZ (BDK_WINDOW_OBJECT (window)->impl);
  
  g_return_if_fail (impl->toplevel != NULL);

  if (impl->transient_for && !BDK_WINDOW_DESTROYED (impl->transient_for))
    {
      BdkWindowImplQuartz *parent_impl;

      parent_impl = BDK_WINDOW_IMPL_QUARTZ (BDK_WINDOW_OBJECT (impl->transient_for)->impl);
      [parent_impl->toplevel addChildWindow:impl->toplevel ordered:NSWindowAbove];
      clear_toplevel_order ();
    }
}

void
bdk_window_quartz_hide (BdkWindow *window)
{
  BdkWindowObject *private = (BdkWindowObject *)window;
  BdkWindowImplQuartz *impl;

  /* Make sure we're not stuck in fullscreen mode. */
  if (get_fullscreen_geometry (window))
    SetSystemUIMode (kUIModeNormal, 0);

  check_grab_unmap (window);

  _bdk_window_clear_update_area (window);

  impl = BDK_WINDOW_IMPL_QUARTZ (private->impl);

  if (window && WINDOW_IS_TOPLEVEL (window))
    {
     /* Update main window. */
      main_window_stack = g_slist_remove (main_window_stack, window);
      if ([NSApp mainWindow] == impl->toplevel)
        _bdk_quartz_window_did_resign_main (window);

      if (impl->transient_for)
        _bdk_quartz_window_detach_from_parent (window);

      [(BdkQuartzWindow*)impl->toplevel hide];
    }
  else if (impl->view)
    {
      [impl->view setHidden:YES];
    }
}

void
bdk_window_quartz_withdraw (BdkWindow *window)
{
  bdk_window_hide (window);
}

static void
move_resize_window_internal (BdkWindow *window,
			     gint       x,
			     gint       y,
			     gint       width,
			     gint       height)
{
  BdkWindowObject *private = (BdkWindowObject *)window;
  BdkWindowImplQuartz *impl;
  BdkRectangle old_visible;
  BdkRectangle new_visible;
  BdkRectangle scroll_rect;
  BdkRebunnyion *old_rebunnyion;
  BdkRebunnyion *expose_rebunnyion;
  NSSize delta;

  if (BDK_WINDOW_DESTROYED (window))
    return;

  impl = BDK_WINDOW_IMPL_QUARTZ (private->impl);

  if ((x == -1 || (x == private->x)) &&
      (y == -1 || (y == private->y)) &&
      (width == -1 || (width == private->width)) &&
      (height == -1 || (height == private->height)))
    {
      return;
    }

  if (!impl->toplevel)
    {
      /* The previously visible area of this window in a coordinate
       * system rooted at the origin of this window.
       */
      old_visible.x = -private->x;
      old_visible.y = -private->y;

      bdk_window_get_size (BDK_DRAWABLE (private->parent), 
                           &old_visible.width, 
                           &old_visible.height);
    }

  if (x != -1)
    {
      delta.width = x - private->x;
      private->x = x;
    }
  else
    {
      delta.width = 0;
    }

  if (y != -1)
    {
      delta.height = y - private->y;
      private->y = y;
    }
  else
    {
      delta.height = 0;
    }

  if (width != -1)
    private->width = width;

  if (height != -1)
    private->height = height;

  BDK_QUARTZ_ALLOC_POOL;

  if (impl->toplevel)
    {
      NSRect content_rect;
      NSRect frame_rect;
      gint gx, gy;

      _bdk_quartz_window_bdk_xy_to_xy (private->x, private->y + private->height,
                                       &gx, &gy);

      content_rect = NSMakeRect (gx, gy, private->width, private->height);

      frame_rect = [impl->toplevel frameRectForContentRect:content_rect];
      [impl->toplevel setFrame:frame_rect display:YES];
    }
  else 
    {
      if (!private->input_only)
        {
          NSRect nsrect;

          nsrect = NSMakeRect (private->x, private->y, private->width, private->height);

          /* The newly visible area of this window in a coordinate
           * system rooted at the origin of this window.
           */
          new_visible.x = -private->x;
          new_visible.y = -private->y;
          new_visible.width = old_visible.width;   /* parent has not changed size */
          new_visible.height = old_visible.height; /* parent has not changed size */

          expose_rebunnyion = bdk_rebunnyion_rectangle (&new_visible);
          old_rebunnyion = bdk_rebunnyion_rectangle (&old_visible);
          bdk_rebunnyion_subtract (expose_rebunnyion, old_rebunnyion);

          /* Determine what (if any) part of the previously visible
           * part of the window can be copied without a redraw
           */
          scroll_rect = old_visible;
          scroll_rect.x -= delta.width;
          scroll_rect.y -= delta.height;
          bdk_rectangle_intersect (&scroll_rect, &old_visible, &scroll_rect);

          if (!bdk_rebunnyion_empty (expose_rebunnyion))
            {
              BdkRectangle* rects;
              gint n_rects;
              gint n;

              if (scroll_rect.width != 0 && scroll_rect.height != 0)
                {
                  [impl->view scrollRect:NSMakeRect (scroll_rect.x,
                                                     scroll_rect.y,
                                                     scroll_rect.width,
                                                     scroll_rect.height)
			              by:delta];
                }

              [impl->view setFrame:nsrect];

              bdk_rebunnyion_get_rectangles (expose_rebunnyion, &rects, &n_rects);

              for (n = 0; n < n_rects; ++n)
                _bdk_quartz_window_set_needs_display_in_rect (window, &rects[n]);

              g_free (rects);
            }
          else
            {
              [impl->view setFrame:nsrect];
              [impl->view setNeedsDisplay:YES];
            }

          bdk_rebunnyion_destroy (expose_rebunnyion);
          bdk_rebunnyion_destroy (old_rebunnyion);
        }
    }

  BDK_QUARTZ_RELEASE_POOL;
}

static inline void
window_quartz_move (BdkWindow *window,
                    gint       x,
                    gint       y)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (((BdkWindowObject *)window)->state & BDK_WINDOW_STATE_FULLSCREEN)
    return;

  move_resize_window_internal (window, x, y, -1, -1);
}

static inline void
window_quartz_resize (BdkWindow *window,
                      gint       width,
                      gint       height)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (((BdkWindowObject *)window)->state & BDK_WINDOW_STATE_FULLSCREEN)
    return;

  if (width < 1)
    width = 1;
  if (height < 1)
    height = 1;

  move_resize_window_internal (window, -1, -1, width, height);
}

static inline void
window_quartz_move_resize (BdkWindow *window,
                           gint       x,
                           gint       y,
                           gint       width,
                           gint       height)
{
  if (width < 1)
    width = 1;
  if (height < 1)
    height = 1;

  move_resize_window_internal (window, x, y, width, height);
}

static void
bdk_window_quartz_move_resize (BdkWindow *window,
                               gboolean   with_move,
                               gint       x,
                               gint       y,
                               gint       width,
                               gint       height)
{
  if (with_move && (width < 0 && height < 0))
    window_quartz_move (window, x, y);
  else
    {
      if (with_move)
        window_quartz_move_resize (window, x, y, width, height);
      else
        window_quartz_resize (window, width, height);
    }
}

/* FIXME: This might need fixing (reparenting didn't work before client-side
 * windows either).
 */
static gboolean
bdk_window_quartz_reparent (BdkWindow *window,
                            BdkWindow *new_parent,
                            gint       x,
                            gint       y)
{
  BdkWindowObject *private, *old_parent_private, *new_parent_private;
  BdkWindowImplQuartz *impl, *old_parent_impl, *new_parent_impl;
  NSView *view, *new_parent_view;

  if (new_parent == _bdk_root)
    {
      /* Could be added, just needs implementing. */
      g_warning ("Reparenting to root window is not supported yet in the Mac OS X backend");
      return FALSE;
    }

  private = BDK_WINDOW_OBJECT (window);
  impl = BDK_WINDOW_IMPL_QUARTZ (private->impl);
  view = impl->view;

  new_parent_private = BDK_WINDOW_OBJECT (new_parent);
  new_parent_impl = BDK_WINDOW_IMPL_QUARTZ (new_parent_private->impl);
  new_parent_view = new_parent_impl->view;

  old_parent_private = BDK_WINDOW_OBJECT (private->parent);
  old_parent_impl = BDK_WINDOW_IMPL_QUARTZ (old_parent_private->impl);

  [view retain];

  [view removeFromSuperview];
  [new_parent_view addSubview:view];

  [view release];

  private->parent = new_parent_private;

  if (old_parent_private)
    {
      old_parent_impl->sorted_children = g_list_remove (old_parent_impl->sorted_children, window);
    }

  new_parent_impl->sorted_children = g_list_prepend (new_parent_impl->sorted_children, window);

  return FALSE;
}

/* Get the toplevel ordering from NSApp and update our own list. We do
 * this on demand since the NSApp's list is not up to date directly
 * after we get windowDidBecomeMain.
 */
static void
update_toplevel_order (void)
{
  BdkWindowObject *root;
  BdkWindowImplQuartz *root_impl;
  NSEnumerator *enumerator;
  id nswindow;
  GList *toplevels = NULL;

  root = BDK_WINDOW_OBJECT (_bdk_root);
  root_impl = BDK_WINDOW_IMPL_QUARTZ (root->impl);

  if (root_impl->sorted_children)
    return;

  BDK_QUARTZ_ALLOC_POOL;

  enumerator = [[NSApp orderedWindows] objectEnumerator];
  while ((nswindow = [enumerator nextObject]))
    {
      BdkWindow *window;

      if (![[nswindow contentView] isKindOfClass:[BdkQuartzView class]])
        continue;

      window = [(BdkQuartzView *)[nswindow contentView] bdkWindow];
      toplevels = g_list_prepend (toplevels, window);
    }

  BDK_QUARTZ_RELEASE_POOL;

  root_impl->sorted_children = g_list_reverse (toplevels);
}

static void
clear_toplevel_order (void)
{
  BdkWindowObject *root;
  BdkWindowImplQuartz *root_impl;

  root = BDK_WINDOW_OBJECT (_bdk_root);
  root_impl = BDK_WINDOW_IMPL_QUARTZ (root->impl);

  g_list_free (root_impl->sorted_children);
  root_impl->sorted_children = NULL;
}

static void
bdk_window_quartz_raise (BdkWindow *window)
{
  if (BDK_WINDOW_DESTROYED (window))
    return;

  if (WINDOW_IS_TOPLEVEL (window))
    {
      BdkWindowImplQuartz *impl;

      impl = BDK_WINDOW_IMPL_QUARTZ (BDK_WINDOW_OBJECT (window)->impl);
      [impl->toplevel orderFront:impl->toplevel];

      clear_toplevel_order ();
    }
  else
    {
      BdkWindowObject *parent = BDK_WINDOW_OBJECT (window)->parent;

      if (parent)
        {
          BdkWindowImplQuartz *impl;

          impl = (BdkWindowImplQuartz *)parent->impl;

          impl->sorted_children = g_list_remove (impl->sorted_children, window);
          impl->sorted_children = g_list_prepend (impl->sorted_children, window);
        }
    }
}

static void
bdk_window_quartz_lower (BdkWindow *window)
{
  if (BDK_WINDOW_DESTROYED (window))
    return;

  if (WINDOW_IS_TOPLEVEL (window))
    {
      BdkWindowImplQuartz *impl;

      impl = BDK_WINDOW_IMPL_QUARTZ (BDK_WINDOW_OBJECT (window)->impl);
      [impl->toplevel orderBack:impl->toplevel];

      clear_toplevel_order ();
    }
  else
    {
      BdkWindowObject *parent = BDK_WINDOW_OBJECT (window)->parent;

      if (parent)
        {
          BdkWindowImplQuartz *impl;

          impl = (BdkWindowImplQuartz *)parent->impl;

          impl->sorted_children = g_list_remove (impl->sorted_children, window);
          impl->sorted_children = g_list_append (impl->sorted_children, window);
        }
    }
}

static void
bdk_window_quartz_restack_toplevel (BdkWindow *window,
				    BdkWindow *sibling,
				    gboolean   above)
{
  BdkWindowImplQuartz *impl;
  gint sibling_num;

  impl = BDK_WINDOW_IMPL_QUARTZ (((BdkWindowObject *)sibling)->impl);
  sibling_num = [impl->toplevel windowNumber];

  impl = BDK_WINDOW_IMPL_QUARTZ (((BdkWindowObject *)window)->impl);

  if (above)
    [impl->toplevel orderWindow:NSWindowAbove relativeTo:sibling_num];
  else
    [impl->toplevel orderWindow:NSWindowBelow relativeTo:sibling_num];
}

static void
bdk_window_quartz_set_background (BdkWindow      *window,
                                  const BdkColor *color)
{
  /* FIXME: We could theoretically set the background color for toplevels
   * here. (Currently we draw the background before emitting expose events)
   */
}

static void
bdk_window_quartz_set_back_pixmap (BdkWindow *window,
                                   BdkPixmap *pixmap)
{
  /* FIXME: Could theoretically set some background image here. (Currently
   * the back pixmap is drawn before emitting expose events.
   */
}

static void
bdk_window_quartz_set_cursor (BdkWindow *window,
                              BdkCursor *cursor)
{
  BdkCursorPrivate *cursor_private;
  NSCursor *nscursor;

  cursor_private = (BdkCursorPrivate *)cursor;

  if (BDK_WINDOW_DESTROYED (window))
    return;

  if (!cursor)
    nscursor = [NSCursor arrowCursor];
  else 
    nscursor = cursor_private->nscursor;

  [nscursor set];
}

static void
bdk_window_quartz_get_geometry (BdkWindow *window,
                                gint      *x,
                                gint      *y,
                                gint      *width,
                                gint      *height,
                                gint      *depth)
{
  BdkWindowImplQuartz *impl;
  BdkWindowObject *private;
  NSRect ns_rect;

  if (BDK_WINDOW_DESTROYED (window))
    return;

  impl = BDK_WINDOW_IMPL_QUARTZ (BDK_WINDOW_OBJECT (window)->impl);
  private = BDK_WINDOW_OBJECT (window);
  if (window == _bdk_root)
    {
      if (x) 
        *x = 0;
      if (y) 
        *y = 0;

      if (width) 
        *width = private->width;
      if (height)
        *height = private->height;
    }
  else if (WINDOW_IS_TOPLEVEL (window))
    {
      ns_rect = [impl->toplevel contentRectForFrameRect:[impl->toplevel frame]];

      /* This doesn't work exactly as in X. There doesn't seem to be a
       * way to get the coords relative to the parent window (usually
       * the window frame), but that seems useless except for
       * borderless windows where it's relative to the root window. So
       * we return (0, 0) (should be something like (0, 22)) for
       * windows with borders and the root relative coordinates
       * otherwise.
       */
      if ([impl->toplevel styleMask] == NSBorderlessWindowMask)
        {
          _bdk_quartz_window_xy_to_bdk_xy (ns_rect.origin.x,
                                           ns_rect.origin.y + ns_rect.size.height,
                                           x, y);
        }
      else 
        {
          if (x)
            *x = 0;
          if (y)
            *y = 0;
        }

      if (width)
        *width = ns_rect.size.width;
      if (height)
        *height = ns_rect.size.height;
    }
  else
    {
      ns_rect = [impl->view frame];
      
      if (x)
        *x = ns_rect.origin.x;
      if (y)
        *y = ns_rect.origin.y;
      if (width)
        *width  = ns_rect.size.width;
      if (height)
        *height = ns_rect.size.height;
    }
    
  if (depth)
      *depth = bdk_drawable_get_depth (window);
}

static gint
bdk_window_quartz_get_root_coords (BdkWindow *window,
                                   gint       x,
                                   gint       y,
                                   gint      *root_x,
                                   gint      *root_y)
{
  BdkWindowObject *private;
  int tmp_x = 0, tmp_y = 0;
  BdkWindow *toplevel;
  NSRect content_rect;
  BdkWindowImplQuartz *impl;

  if (BDK_WINDOW_DESTROYED (window)) 
    {
      if (root_x)
	*root_x = 0;
      if (root_y)
	*root_y = 0;
      
      return 0;
    }

  if (window == _bdk_root)
    {
      if (root_x)
        *root_x = x;
      if (root_y)
        *root_y = y;

      return 1;
    }
  
  private = BDK_WINDOW_OBJECT (window);

  toplevel = bdk_window_get_toplevel (window);
  impl = BDK_WINDOW_IMPL_QUARTZ (BDK_WINDOW_OBJECT (toplevel)->impl);

  content_rect = [impl->toplevel contentRectForFrameRect:[impl->toplevel frame]];

  _bdk_quartz_window_xy_to_bdk_xy (content_rect.origin.x,
                                   content_rect.origin.y + content_rect.size.height,
                                   &tmp_x, &tmp_y);

  tmp_x += x;
  tmp_y += y;

  while (private != BDK_WINDOW_OBJECT (toplevel))
    {
      if (_bdk_window_has_impl ((BdkWindow *)private))
        {
          tmp_x += private->x;
          tmp_y += private->y;
        }

      private = private->parent;
    }

  if (root_x)
    *root_x = tmp_x;
  if (root_y)
    *root_y = tmp_y;

  return TRUE;
}

static gboolean
bdk_window_quartz_get_deskrelative_origin (BdkWindow *window,
                                           gint      *x,
                                           gint      *y)
{
  return bdk_window_get_origin (window, x, y);
}

void
bdk_window_get_root_origin (BdkWindow *window,
			    gint      *x,
			    gint      *y)
{
  BdkRectangle rect;

  rect.x = 0;
  rect.y = 0;
  
  bdk_window_get_frame_extents (window, &rect);

  if (x)
    *x = rect.x;

  if (y)
    *y = rect.y;
}

/* Returns coordinates relative to the passed in window. */
static BdkWindow *
bdk_window_quartz_get_pointer_helper (BdkWindow       *window,
                                      gint            *x,
                                      gint            *y,
                                      BdkModifierType *mask)
{
  BdkWindowObject *toplevel;
  BdkWindowObject *private;
  NSPoint point;
  gint x_tmp, y_tmp;
  BdkWindow *found_window;

  g_return_val_if_fail (window == NULL || BDK_IS_WINDOW (window), NULL);

  if (BDK_WINDOW_DESTROYED (window))
    {
      *x = 0;
      *y = 0;
      *mask = 0;
      return NULL;
    }
  
  toplevel = BDK_WINDOW_OBJECT (bdk_window_get_effective_toplevel (window));

  *mask = _bdk_quartz_events_get_current_keyboard_modifiers ()
	| _bdk_quartz_events_get_current_mouse_modifiers ();

  /* Get the y coordinate, needs to be flipped. */
  if (window == _bdk_root)
    {
      point = [NSEvent mouseLocation];
      _bdk_quartz_window_nspoint_to_bdk_xy (point, &x_tmp, &y_tmp);
    }
  else
    {
      BdkWindowImplQuartz *impl;
      NSWindow *nswindow;

      impl = BDK_WINDOW_IMPL_QUARTZ (toplevel->impl);
      private = BDK_WINDOW_OBJECT (toplevel);
      nswindow = impl->toplevel;

      point = [nswindow mouseLocationOutsideOfEventStream];

      x_tmp = point.x;
      y_tmp = private->height - point.y;

      window = (BdkWindow *)toplevel;
    }

  found_window = _bdk_quartz_window_find_child (window, x_tmp, y_tmp);

  /* We never return the root window. */
  if (found_window == _bdk_root)
    found_window = NULL;

  *x = x_tmp;
  *y = y_tmp;

  return found_window;
}

static gboolean
bdk_window_quartz_get_pointer (BdkWindow       *window,
                               gint            *x,
                               gint            *y,
                               BdkModifierType *mask)
{
  return bdk_window_quartz_get_pointer_helper (window, x, y, mask) != NULL;
}

/* Returns coordinates relative to the root. */
void
_bdk_windowing_get_pointer (BdkDisplay       *display,
                            BdkScreen       **screen,
                            gint             *x,
                            gint             *y,
                            BdkModifierType  *mask)
{
  g_return_if_fail (display == _bdk_display);
  
  *screen = _bdk_screen;
  bdk_window_quartz_get_pointer_helper (_bdk_root, x, y, mask);
}

void
bdk_display_warp_pointer (BdkDisplay *display,
			  BdkScreen  *screen,
			  gint        x,
			  gint        y)
{
  CGDisplayMoveCursorToPoint (CGMainDisplayID (), CGPointMake (x, y));
}

/* Returns coordinates relative to the found window. */
BdkWindow *
_bdk_windowing_window_at_pointer (BdkDisplay      *display,
				  gint            *win_x,
				  gint            *win_y,
                                  BdkModifierType *mask,
				  gboolean         get_toplevel)
{
  BdkWindow *found_window;
  gint x, y;
  BdkModifierType tmp_mask = 0;

  found_window = bdk_window_quartz_get_pointer_helper (_bdk_root,
                                                       &x, &y,
                                                       &tmp_mask);
  if (found_window)
    {
      BdkWindowObject *private;

      /* The coordinates returned above are relative the root, we want
       * coordinates relative the window here. 
       */
      private = BDK_WINDOW_OBJECT (found_window);
      while (private != BDK_WINDOW_OBJECT (_bdk_root))
	{
	  x -= private->x;
	  y -= private->y;
	  
	  private = private->parent;
	}

      *win_x = x;
      *win_y = y;
    }
  else
    {
      /* Mimic the X backend here, -1,-1 for unknown windows. */
      *win_x = -1;
      *win_y = -1;
    }

  if (mask)
    *mask = tmp_mask;

  if (get_toplevel)
    {
      BdkWindowObject *w = (BdkWindowObject *)found_window;
      /* Requested toplevel, find it. */
      /* TODO: This can be implemented more efficient by never
	 recursing into children in the first place */
      if (w)
	{
	  /* Convert to toplevel */
	  while (w->parent != NULL &&
		 w->parent->window_type != BDK_WINDOW_ROOT)
	    {
	      *win_x += w->x;
	      *win_y += w->y;
	      w = w->parent;
	    }
	  found_window = (BdkWindow *)w;
	}
    }

  return found_window;
}

static BdkEventMask  
bdk_window_quartz_get_events (BdkWindow *window)
{
  if (BDK_WINDOW_DESTROYED (window))
    return 0;
  else
    return BDK_WINDOW_OBJECT (window)->event_mask;
}

static void
bdk_window_quartz_set_events (BdkWindow       *window,
                              BdkEventMask     event_mask)
{
  /* The mask is set in the common code. */
}

void
bdk_window_set_urgency_hint (BdkWindow *window,
			     gboolean   urgent)
{
  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  /* FIXME: Implement */
}

void 
bdk_window_set_geometry_hints (BdkWindow         *window,
			       const BdkGeometry *geometry,
			       BdkWindowHints     geom_mask)
{
  BdkWindowImplQuartz *impl;

  g_return_if_fail (geometry != NULL);

  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;
  
  impl = BDK_WINDOW_IMPL_QUARTZ (((BdkWindowObject *) window)->impl);
  if (!impl->toplevel)
    return;

  if (geom_mask & BDK_HINT_POS)
    {
      /* FIXME: Implement */
    }

  if (geom_mask & BDK_HINT_USER_POS)
    {
      /* FIXME: Implement */
    }

  if (geom_mask & BDK_HINT_USER_SIZE)
    {
      /* FIXME: Implement */
    }
  
  if (geom_mask & BDK_HINT_MIN_SIZE)
    {
      NSSize size;

      size.width = geometry->min_width;
      size.height = geometry->min_height;

      [impl->toplevel setContentMinSize:size];
    }
  
  if (geom_mask & BDK_HINT_MAX_SIZE)
    {
      NSSize size;

      size.width = geometry->max_width;
      size.height = geometry->max_height;

      [impl->toplevel setContentMaxSize:size];
    }
  
  if (geom_mask & BDK_HINT_BASE_SIZE)
    {
      /* FIXME: Implement */
    }
  
  if (geom_mask & BDK_HINT_RESIZE_INC)
    {
      NSSize size;

      size.width = geometry->width_inc;
      size.height = geometry->height_inc;

      [impl->toplevel setContentResizeIncrements:size];
    }
  
  if (geom_mask & BDK_HINT_ASPECT)
    {
      NSSize size;

      if (geometry->min_aspect != geometry->max_aspect)
        {
          g_warning ("Only equal minimum and maximum aspect ratios are supported on Mac OS. Using minimum aspect ratio...");
        }

      size.width = geometry->min_aspect;
      size.height = 1.0;

      [impl->toplevel setContentAspectRatio:size];
    }

  if (geom_mask & BDK_HINT_WIN_GRAVITY)
    {
      /* FIXME: Implement */
    }
}

void
bdk_window_set_title (BdkWindow   *window,
		      const gchar *title)
{
  BdkWindowImplQuartz *impl;

  g_return_if_fail (title != NULL);

  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  impl = BDK_WINDOW_IMPL_QUARTZ (((BdkWindowObject *)window)->impl);

  if (impl->toplevel)
    {
      BDK_QUARTZ_ALLOC_POOL;
      [impl->toplevel setTitle:[NSString stringWithUTF8String:title]];
      BDK_QUARTZ_RELEASE_POOL;
    }
}

void          
bdk_window_set_role (BdkWindow   *window,
		     const gchar *role)
{
  if (BDK_WINDOW_DESTROYED (window) ||
      WINDOW_IS_TOPLEVEL (window))
    return;

  /* FIXME: Implement */
}

void
bdk_window_set_transient_for (BdkWindow *window, 
			      BdkWindow *parent)
{
  BdkWindowImplQuartz *window_impl;
  BdkWindowImplQuartz *parent_impl;

  if (BDK_WINDOW_DESTROYED (window)  || BDK_WINDOW_DESTROYED (parent) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  window_impl = BDK_WINDOW_IMPL_QUARTZ (BDK_WINDOW_OBJECT (window)->impl);
  if (!window_impl->toplevel)
    return;

  BDK_QUARTZ_ALLOC_POOL;

  if (window_impl->transient_for)
    {
      _bdk_quartz_window_detach_from_parent (window);

      g_object_unref (window_impl->transient_for);
      window_impl->transient_for = NULL;
    }

  parent_impl = BDK_WINDOW_IMPL_QUARTZ (BDK_WINDOW_OBJECT (parent)->impl);
  if (parent_impl->toplevel)
    {
      /* We save the parent because it needs to be unset/reset when
       * hiding and showing the window. 
       */

      /* We don't set transients for tooltips, they are already
       * handled by the window level being the top one. If we do, then
       * the parent window will be brought to the top just because the
       * tooltip is, which is not what we want.
       */
      if (bdk_window_get_type_hint (window) != BDK_WINDOW_TYPE_HINT_TOOLTIP)
        {
          window_impl->transient_for = g_object_ref (parent);

          /* We only add the window if it is shown, otherwise it will
           * be shown unconditionally here. If it is not shown, the
           * window will be added in show() instead.
           */
          if (!(BDK_WINDOW_OBJECT (window)->state & BDK_WINDOW_STATE_WITHDRAWN))
            _bdk_quartz_window_attach_to_parent (window);
        }
    }
  
  BDK_QUARTZ_RELEASE_POOL;
}

static void
bdk_window_quartz_shape_combine_rebunnyion (BdkWindow       *window,
                                        const BdkRebunnyion *shape,
                                        gint             x,
                                        gint             y)
{
  /* FIXME: Implement */
}

static void
bdk_window_quartz_input_shape_combine_rebunnyion (BdkWindow       *window,
                                              const BdkRebunnyion *shape_rebunnyion,
                                              gint             offset_x,
                                              gint             offset_y)
{
  /* FIXME: Implement */
}

void
bdk_window_set_override_redirect (BdkWindow *window,
				  gboolean override_redirect)
{
  /* FIXME: Implement */
}

void
bdk_window_set_accept_focus (BdkWindow *window,
			     gboolean accept_focus)
{
  BdkWindowObject *private;

  private = (BdkWindowObject *)window;  

  private->accept_focus = accept_focus != FALSE;
}

static gboolean 
bdk_window_quartz_set_static_gravities (BdkWindow *window,
                                        gboolean   use_static)
{
  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return FALSE;

  /* FIXME: Implement */
  return FALSE;
}

void
bdk_window_set_focus_on_map (BdkWindow *window,
			     gboolean focus_on_map)
{
  BdkWindowObject *private;

  private = (BdkWindowObject *)window;  
  
  private->focus_on_map = focus_on_map != FALSE;
}

void          
bdk_window_set_icon (BdkWindow *window, 
		     BdkWindow *icon_window,
		     BdkPixmap *pixmap,
		     BdkBitmap *mask)
{
  /* FIXME: Implement */
}

void          
bdk_window_set_icon_name (BdkWindow   *window, 
			  const gchar *name)
{
  /* FIXME: Implement */
}

void
bdk_window_focus (BdkWindow *window,
                  guint32    timestamp)
{
  BdkWindowObject *private;
  BdkWindowImplQuartz *impl;
	
  private = (BdkWindowObject*) window;
  impl = BDK_WINDOW_IMPL_QUARTZ (private->impl);

  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  if (private->accept_focus && private->window_type != BDK_WINDOW_TEMP)
    {
      BDK_QUARTZ_ALLOC_POOL;
      [impl->toplevel makeKeyAndOrderFront:impl->toplevel];
      clear_toplevel_order ();
      BDK_QUARTZ_RELEASE_POOL;
    }
}

void
bdk_window_set_hints (BdkWindow *window,
		      gint       x,
		      gint       y,
		      gint       min_width,
		      gint       min_height,
		      gint       max_width,
		      gint       max_height,
		      gint       flags)
{
  /* FIXME: Implement */
}

static gint
window_type_hint_to_level (BdkWindowTypeHint hint)
{
  /*  the order in this switch statement corresponds to the actual
   *  stacking order: the first group is top, the last group is bottom
   */
  switch (hint)
    {
    case BDK_WINDOW_TYPE_HINT_POPUP_MENU:
    case BDK_WINDOW_TYPE_HINT_COMBO:
    case BDK_WINDOW_TYPE_HINT_DND:
    case BDK_WINDOW_TYPE_HINT_TOOLTIP:
      return NSPopUpMenuWindowLevel;

    case BDK_WINDOW_TYPE_HINT_NOTIFICATION:
    case BDK_WINDOW_TYPE_HINT_SPLASHSCREEN:
      return NSStatusWindowLevel;

    case BDK_WINDOW_TYPE_HINT_MENU: /* Torn-off menu */
    case BDK_WINDOW_TYPE_HINT_DROPDOWN_MENU: /* Menu from menubar */
      return NSTornOffMenuWindowLevel;

    case BDK_WINDOW_TYPE_HINT_DOCK:
      return NSFloatingWindowLevel; /* NSDockWindowLevel is deprecated, and not replaced */

    case BDK_WINDOW_TYPE_HINT_UTILITY:
    case BDK_WINDOW_TYPE_HINT_DIALOG:  /* Dialog window */
    case BDK_WINDOW_TYPE_HINT_NORMAL:  /* Normal toplevel window */
    case BDK_WINDOW_TYPE_HINT_TOOLBAR: /* Window used to implement toolbars */
      return NSNormalWindowLevel;

    case BDK_WINDOW_TYPE_HINT_DESKTOP:
      return kCGDesktopWindowLevelKey; /* doesn't map to any real Cocoa model */

    default:
      break;
    }

  return NSNormalWindowLevel;
}

static gboolean
window_type_hint_to_shadow (BdkWindowTypeHint hint)
{
  switch (hint)
    {
    case BDK_WINDOW_TYPE_HINT_NORMAL:  /* Normal toplevel window */
    case BDK_WINDOW_TYPE_HINT_DIALOG:  /* Dialog window */
    case BDK_WINDOW_TYPE_HINT_DOCK:
    case BDK_WINDOW_TYPE_HINT_UTILITY:
    case BDK_WINDOW_TYPE_HINT_MENU: /* Torn-off menu */
    case BDK_WINDOW_TYPE_HINT_DROPDOWN_MENU: /* Menu from menubar */
    case BDK_WINDOW_TYPE_HINT_SPLASHSCREEN:
    case BDK_WINDOW_TYPE_HINT_POPUP_MENU:
    case BDK_WINDOW_TYPE_HINT_COMBO:
    case BDK_WINDOW_TYPE_HINT_NOTIFICATION:
    case BDK_WINDOW_TYPE_HINT_TOOLTIP:
      return TRUE;

    case BDK_WINDOW_TYPE_HINT_TOOLBAR: /* Window used to implement toolbars */
    case BDK_WINDOW_TYPE_HINT_DESKTOP: /* N/A */
    case BDK_WINDOW_TYPE_HINT_DND:
      break;

    default:
      break;
    }

  return FALSE;
}

static gboolean
window_type_hint_to_hides_on_deactivate (BdkWindowTypeHint hint)
{
  switch (hint)
    {
    case BDK_WINDOW_TYPE_HINT_UTILITY:
    case BDK_WINDOW_TYPE_HINT_MENU: /* Torn-off menu */
    case BDK_WINDOW_TYPE_HINT_SPLASHSCREEN:
    case BDK_WINDOW_TYPE_HINT_NOTIFICATION:
    case BDK_WINDOW_TYPE_HINT_TOOLTIP:
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

void
bdk_window_set_type_hint (BdkWindow        *window,
			  BdkWindowTypeHint hint)
{
  BdkWindowImplQuartz *impl;

  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  impl = BDK_WINDOW_IMPL_QUARTZ (((BdkWindowObject *) window)->impl);

  impl->type_hint = hint;

  /* Match the documentation, only do something if we're not mapped yet. */
  if (BDK_WINDOW_IS_MAPPED (window))
    return;

  [impl->toplevel setHasShadow: window_type_hint_to_shadow (hint)];
  [impl->toplevel setLevel: window_type_hint_to_level (hint)];
  [impl->toplevel setHidesOnDeactivate: window_type_hint_to_hides_on_deactivate (hint)];
}

BdkWindowTypeHint
bdk_window_get_type_hint (BdkWindow *window)
{
  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return BDK_WINDOW_TYPE_HINT_NORMAL;
  
  return BDK_WINDOW_IMPL_QUARTZ (((BdkWindowObject *) window)->impl)->type_hint;
}

void
bdk_window_set_modal_hint (BdkWindow *window,
			   gboolean   modal)
{
  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  /* FIXME: Implement */
}

void
bdk_window_set_skip_taskbar_hint (BdkWindow *window,
				  gboolean   skips_taskbar)
{
  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  /* FIXME: Implement */
}

void
bdk_window_set_skip_pager_hint (BdkWindow *window,
				gboolean   skips_pager)
{
  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  /* FIXME: Implement */
}

void
bdk_window_begin_resize_drag (BdkWindow     *window,
                              BdkWindowEdge  edge,
                              gint           button,
                              gint           root_x,
                              gint           root_y,
                              guint32        timestamp)
{
  BdkWindowObject *private;
  BdkWindowImplQuartz *impl;

  g_return_if_fail (BDK_IS_WINDOW (window));

  if (edge != BDK_WINDOW_EDGE_SOUTH_EAST)
    {
      g_warning ("Resizing is only implemented for BDK_WINDOW_EDGE_SOUTH_EAST on Mac OS");
      return;
    }

  if (BDK_WINDOW_DESTROYED (window))
    return;

  private = BDK_WINDOW_OBJECT (window);
  impl = BDK_WINDOW_IMPL_QUARTZ (private->impl);

  if (!impl->toplevel)
    {
      g_warning ("Can't call bdk_window_begin_resize_drag on non-toplevel window");
      return;
    }

  [(BdkQuartzWindow *)impl->toplevel beginManualResize];
}

void
bdk_window_begin_move_drag (BdkWindow *window,
                            gint       button,
                            gint       root_x,
                            gint       root_y,
                            guint32    timestamp)
{
  BdkWindowObject *private;
  BdkWindowImplQuartz *impl;

  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  private = BDK_WINDOW_OBJECT (window);
  impl = BDK_WINDOW_IMPL_QUARTZ (private->impl);

  if (!impl->toplevel)
    {
      g_warning ("Can't call bdk_window_begin_move_drag on non-toplevel window");
      return;
    }

  [(BdkQuartzWindow *)impl->toplevel beginManualMove];
}

void
bdk_window_set_icon_list (BdkWindow *window,
			  GList     *pixbufs)
{
  /* FIXME: Implement */
}

void
bdk_window_get_frame_extents (BdkWindow    *window,
                              BdkRectangle *rect)
{
  BdkWindowObject *private;
  BdkWindow *toplevel;
  BdkWindowImplQuartz *impl;
  NSRect ns_rect;

  g_return_if_fail (rect != NULL);

  private = BDK_WINDOW_OBJECT (window);

  rect->x = 0;
  rect->y = 0;
  rect->width = 1;
  rect->height = 1;
  
  toplevel = bdk_window_get_effective_toplevel (window);
  impl = BDK_WINDOW_IMPL_QUARTZ (BDK_WINDOW_OBJECT (toplevel)->impl);

  ns_rect = [impl->toplevel frame];

  _bdk_quartz_window_xy_to_bdk_xy (ns_rect.origin.x,
                                   ns_rect.origin.y + ns_rect.size.height,
                                   &rect->x, &rect->y);

  rect->width = ns_rect.size.width;
  rect->height = ns_rect.size.height;
}

/* Fake protocol to make gcc think that it's OK to call setStyleMask
   even if it isn't. We check to make sure before actually calling
   it. */

@protocol CanSetStyleMask
- (void)setStyleMask:(int)mask;
@end

void
bdk_window_set_decorations (BdkWindow       *window,
			    BdkWMDecoration  decorations)
{
  BdkWindowImplQuartz *impl;
  NSUInteger old_mask, new_mask;
  NSView *old_view;

  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  impl = BDK_WINDOW_IMPL_QUARTZ (BDK_WINDOW_OBJECT (window)->impl);

  if (decorations == 0 || BDK_WINDOW_TYPE (window) == BDK_WINDOW_TEMP ||
      impl->type_hint == BDK_WINDOW_TYPE_HINT_SPLASHSCREEN )
    {
      new_mask = NSBorderlessWindowMask;
    }
  else
    {
      /* FIXME: Honor other BDK_DECOR_* flags. */
      new_mask = (NSTitledWindowMask | NSClosableWindowMask |
                    NSMiniaturizableWindowMask | NSResizableWindowMask);
    }

  BDK_QUARTZ_ALLOC_POOL;

  old_mask = [impl->toplevel styleMask];

  if (old_mask != new_mask)
    {
      NSRect rect;

      old_view = [[impl->toplevel contentView] retain];

      rect = [impl->toplevel frame];

      /* Properly update the size of the window when the titlebar is
       * added or removed.
       */
      if (old_mask == NSBorderlessWindowMask &&
          new_mask != NSBorderlessWindowMask)
        {
          rect = [NSWindow frameRectForContentRect:rect styleMask:new_mask];

        }
      else if (old_mask != NSBorderlessWindowMask &&
               new_mask == NSBorderlessWindowMask)
        {
          rect = [NSWindow contentRectForFrameRect:rect styleMask:old_mask];
        }

      /* Note, before OS 10.6 there doesn't seem to be a way to change this
       * without recreating the toplevel. From 10.6 onward, a simple call to
       * setStyleMask takes care of most of this, except for ensuring that the
       * title is set.
       */
      if ([impl->toplevel respondsToSelector:@selector(setStyleMask:)])
        {
          NSString *title = [impl->toplevel title];

          [(id<CanSetStyleMask>)impl->toplevel setStyleMask:new_mask];

          /* It appears that unsetting and then resetting NSTitledWindowMask
           * does not reset the title in the title bar as might be expected.
           *
           * In theory we only need to set this if new_mask includes
           * NSTitledWindowMask. This behaved extremely oddly when
           * conditionalized upon that and since it has no side effects (i.e.
           * if NSTitledWindowMask is not requested, the title will not be
           * displayed) just do it unconditionally. We also must null check
           * 'title' before setting it to avoid crashing.
           */
          if (title)
            [impl->toplevel setTitle:title];
        }
      else
        {
          NSString *title = [impl->toplevel title];
          NSColor *bg = [impl->toplevel backgroundColor];
          NSScreen *screen = [impl->toplevel screen];

          /* Make sure the old window is closed, recall that releasedWhenClosed
           * is set on BdkQuartzWindows.
           */
          [impl->toplevel close];

          impl->toplevel = [[BdkQuartzWindow alloc] initWithContentRect:rect
                                                              styleMask:new_mask
                                                                backing:NSBackingStoreBuffered
                                                                  defer:NO
                                                                 screen:screen];
          [impl->toplevel setHasShadow: window_type_hint_to_shadow (impl->type_hint)];
          [impl->toplevel setLevel: window_type_hint_to_level (impl->type_hint)];
          if (title)
            [impl->toplevel setTitle:title];
          [impl->toplevel setBackgroundColor:bg];
          [impl->toplevel setHidesOnDeactivate: window_type_hint_to_hides_on_deactivate (impl->type_hint)];
          [impl->toplevel setContentView:old_view];
        }

      if (new_mask == NSBorderlessWindowMask)
        [impl->toplevel setContentSize:rect.size];
      else
        [impl->toplevel setFrame:rect display:YES];

      /* Invalidate the window shadow for non-opaque views that have shadow
       * enabled, to get the shadow shape updated.
       */
      if (![old_view isOpaque] && [impl->toplevel hasShadow])
        [(BdkQuartzView*)old_view setNeedsInvalidateShadow:YES];

      [old_view release];
    }

  BDK_QUARTZ_RELEASE_POOL;
}

gboolean
bdk_window_get_decorations (BdkWindow       *window,
			    BdkWMDecoration *decorations)
{
  BdkWindowImplQuartz *impl;

  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return FALSE;

  impl = BDK_WINDOW_IMPL_QUARTZ (BDK_WINDOW_OBJECT (window)->impl);

  if (decorations)
    {
      /* Borderless is 0, so we can't check it as a bit being set. */
      if ([impl->toplevel styleMask] == NSBorderlessWindowMask)
        {
          *decorations = 0;
        }
      else
        {
          /* FIXME: Honor the other BDK_DECOR_* flags. */
          *decorations = BDK_DECOR_ALL;
        }
    }

  return TRUE;
}

void
bdk_window_set_functions (BdkWindow    *window,
			  BdkWMFunction functions)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  /* FIXME: Implement */
}

gboolean
_bdk_windowing_window_queue_antiexpose (BdkWindow  *window,
					BdkRebunnyion  *area)
{
  return FALSE;
}

void
bdk_window_stick (BdkWindow *window)
{
  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;
}

void
bdk_window_unstick (BdkWindow *window)
{
  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;
}

void
bdk_window_maximize (BdkWindow *window)
{
  BdkWindowImplQuartz *impl;

  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  impl = BDK_WINDOW_IMPL_QUARTZ (BDK_WINDOW_OBJECT (window)->impl);

  if (BDK_WINDOW_IS_MAPPED (window))
    {
      BDK_QUARTZ_ALLOC_POOL;

      if (impl->toplevel && ![impl->toplevel isZoomed])
	[impl->toplevel zoom:nil];

      BDK_QUARTZ_RELEASE_POOL;
    }
  else
    {
      bdk_synthesize_window_state (window,
				   0,
				   BDK_WINDOW_STATE_MAXIMIZED);
    }
}

void
bdk_window_unmaximize (BdkWindow *window)
{
  BdkWindowImplQuartz *impl;

  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  impl = BDK_WINDOW_IMPL_QUARTZ (BDK_WINDOW_OBJECT (window)->impl);

  if (BDK_WINDOW_IS_MAPPED (window))
    {
      BDK_QUARTZ_ALLOC_POOL;

      if (impl->toplevel && [impl->toplevel isZoomed])
	[impl->toplevel zoom:nil];

      BDK_QUARTZ_RELEASE_POOL;
    }
  else
    {
      bdk_synthesize_window_state (window,
				   BDK_WINDOW_STATE_MAXIMIZED,
				   0);
    }
}

void
bdk_window_iconify (BdkWindow *window)
{
  BdkWindowImplQuartz *impl;

  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  impl = BDK_WINDOW_IMPL_QUARTZ (BDK_WINDOW_OBJECT (window)->impl);

  if (BDK_WINDOW_IS_MAPPED (window))
    {
      BDK_QUARTZ_ALLOC_POOL;

      if (impl->toplevel)
	[impl->toplevel miniaturize:nil];

      BDK_QUARTZ_RELEASE_POOL;
    }
  else
    {
      bdk_synthesize_window_state (window,
				   0,
				   BDK_WINDOW_STATE_ICONIFIED);
    }
}

void
bdk_window_deiconify (BdkWindow *window)
{
  BdkWindowImplQuartz *impl;

  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  impl = BDK_WINDOW_IMPL_QUARTZ (BDK_WINDOW_OBJECT (window)->impl);

  if (BDK_WINDOW_IS_MAPPED (window))
    {
      BDK_QUARTZ_ALLOC_POOL;

      if (impl->toplevel)
	[impl->toplevel deminiaturize:nil];

      BDK_QUARTZ_RELEASE_POOL;
    }
  else
    {
      bdk_synthesize_window_state (window,
				   BDK_WINDOW_STATE_ICONIFIED,
				   0);
    }
}

static FullscreenSavedGeometry *
get_fullscreen_geometry (BdkWindow *window)
{
  return g_object_get_data (G_OBJECT (window), FULLSCREEN_DATA);
}

void
bdk_window_fullscreen (BdkWindow *window)
{
  FullscreenSavedGeometry *geometry;
  BdkWindowObject *private = (BdkWindowObject *) window;
  BdkWindowImplQuartz *impl = BDK_WINDOW_IMPL_QUARTZ (private->impl);
  NSRect frame;

  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  geometry = get_fullscreen_geometry (window);
  if (!geometry)
    {
      geometry = g_new (FullscreenSavedGeometry, 1);

      geometry->x = private->x;
      geometry->y = private->y;
      geometry->width = private->width;
      geometry->height = private->height;

      if (!bdk_window_get_decorations (window, &geometry->decor))
        geometry->decor = BDK_DECOR_ALL;

      g_object_set_data_full (G_OBJECT (window),
                              FULLSCREEN_DATA, geometry, 
                              g_free);

      bdk_window_set_decorations (window, 0);

      frame = [[impl->toplevel screen] frame];
      move_resize_window_internal (window,
                                   0, 0, 
                                   frame.size.width, frame.size.height);
      [impl->toplevel setContentSize:frame.size];
      [impl->toplevel makeKeyAndOrderFront:impl->toplevel];

      clear_toplevel_order ();
    }

  if ([NSWindow respondsToSelector:@selector(toggleFullScreen:)])
    {
       [impl->toplevel toggleFullScreen:nil];
    }
  else
    {
      SetSystemUIMode (kUIModeAllHidden, kUIOptionAutoShowMenuBar);
    }

  bdk_synthesize_window_state (window, 0, BDK_WINDOW_STATE_FULLSCREEN);
}

void
bdk_window_unfullscreen (BdkWindow *window)
{
  BdkWindowObject *private = (BdkWindowObject *) window;
  BdkWindowImplQuartz *impl = BDK_WINDOW_IMPL_QUARTZ (private->impl);
  FullscreenSavedGeometry *geometry;

  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  geometry = get_fullscreen_geometry (window);
  if (geometry)
    {
      if ([NSWindow respondsToSelector:@selector(toggleFullScreen:)])
        {
          [impl->toplevel toggleFullScreen:nil];
        }
      else
        {
	  SetSystemUIMode (kUIModeNormal, 0);
	}

      move_resize_window_internal (window,
                                   geometry->x,
                                   geometry->y,
                                   geometry->width,
                                   geometry->height);

      bdk_window_set_decorations (window, geometry->decor);

      g_object_set_data (G_OBJECT (window), FULLSCREEN_DATA, NULL);

      [impl->toplevel makeKeyAndOrderFront:impl->toplevel];
      clear_toplevel_order ();

      bdk_synthesize_window_state (window, BDK_WINDOW_STATE_FULLSCREEN, 0);
    }
}

void
bdk_window_set_keep_above (BdkWindow *window, gboolean setting)
{
  BdkWindowObject *private = (BdkWindowObject *) window;
  BdkWindowImplQuartz *impl = BDK_WINDOW_IMPL_QUARTZ (private->impl);
  gint level;

  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  level = window_type_hint_to_level (bdk_window_get_type_hint (window));
  
  /* Adjust normal window level by one if necessary. */
  [impl->toplevel setLevel: level + (setting ? 1 : 0)];
}

void
bdk_window_set_keep_below (BdkWindow *window, gboolean setting)
{
  BdkWindowObject *private = (BdkWindowObject *) window;
  BdkWindowImplQuartz *impl = BDK_WINDOW_IMPL_QUARTZ (private->impl);
  gint level;

  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;
  
  level = window_type_hint_to_level (bdk_window_get_type_hint (window));
  
  /* Adjust normal window level by one if necessary. */
  [impl->toplevel setLevel: level - (setting ? 1 : 0)];
}

BdkWindow *
bdk_window_get_group (BdkWindow *window)
{
  g_return_val_if_fail (BDK_WINDOW_TYPE (window) != BDK_WINDOW_CHILD, NULL);

  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return NULL;

  /* FIXME: Implement */

  return NULL;
}

void          
bdk_window_set_group (BdkWindow *window, 
		      BdkWindow *leader)
{
  /* FIXME: Implement */	
}

BdkWindow*
bdk_window_foreign_new_for_display (BdkDisplay      *display,
				    BdkNativeWindow  anid)
{
  /* Foreign windows aren't supported in Mac OS X */
  return NULL;
}

BdkWindow*
bdk_window_lookup (BdkNativeWindow anid)
{
  /* Foreign windows aren't supported in Mac OS X */
  return NULL;
}

BdkWindow *
bdk_window_lookup_for_display (BdkDisplay *display, BdkNativeWindow anid)
{
  /* Foreign windows aren't supported in Mac OS X */
  return NULL;
}

void
bdk_window_enable_synchronized_configure (BdkWindow *window)
{
}

void
bdk_window_configure_finished (BdkWindow *window)
{
}

void
bdk_window_destroy_notify (BdkWindow *window)
{
  check_grab_destroy (window);
}

void 
_bdk_windowing_window_beep (BdkWindow *window)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  bdk_display_beep (_bdk_display);
}

void
bdk_window_set_opacity (BdkWindow *window,
			gdouble    opacity)
{
  BdkWindowObject *private = (BdkWindowObject *) window;
  BdkWindowImplQuartz *impl = BDK_WINDOW_IMPL_QUARTZ (private->impl);

  g_return_if_fail (BDK_IS_WINDOW (window));
  g_return_if_fail (WINDOW_IS_TOPLEVEL (window));

  if (BDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return;

  if (opacity < 0)
    opacity = 0;
  else if (opacity > 1)
    opacity = 1;

  [impl->toplevel setAlphaValue: opacity];
}

void
_bdk_windowing_window_set_composited (BdkWindow *window, gboolean composited)
{
}

BdkRebunnyion *
_bdk_windowing_get_shape_for_mask (BdkBitmap *mask)
{
  /* FIXME: implement */
  return NULL;
}

BdkRebunnyion *
_bdk_windowing_window_get_shape (BdkWindow *window)
{
  /* FIXME: implement */
  return NULL;
}

BdkRebunnyion *
_bdk_windowing_window_get_input_shape (BdkWindow *window)
{
  /* FIXME: implement */
  return NULL;
}

static void
bdk_window_impl_iface_init (BdkWindowImplIface *iface)
{
  iface->show = bdk_window_quartz_show;
  iface->hide = bdk_window_quartz_hide;
  iface->withdraw = bdk_window_quartz_withdraw;
  iface->set_events = bdk_window_quartz_set_events;
  iface->get_events = bdk_window_quartz_get_events;
  iface->raise = bdk_window_quartz_raise;
  iface->lower = bdk_window_quartz_lower;
  iface->restack_toplevel = bdk_window_quartz_restack_toplevel;
  iface->move_resize = bdk_window_quartz_move_resize;
  iface->set_background = bdk_window_quartz_set_background;
  iface->set_back_pixmap = bdk_window_quartz_set_back_pixmap;
  iface->reparent = bdk_window_quartz_reparent;
  iface->set_cursor = bdk_window_quartz_set_cursor;
  iface->get_geometry = bdk_window_quartz_get_geometry;
  iface->get_root_coords = bdk_window_quartz_get_root_coords;
  iface->get_pointer = bdk_window_quartz_get_pointer;
  iface->get_deskrelative_origin = bdk_window_quartz_get_deskrelative_origin;
  iface->shape_combine_rebunnyion = bdk_window_quartz_shape_combine_rebunnyion;
  iface->input_shape_combine_rebunnyion = bdk_window_quartz_input_shape_combine_rebunnyion;
  iface->set_static_gravities = bdk_window_quartz_set_static_gravities;
  iface->queue_antiexpose = _bdk_quartz_window_queue_antiexpose;
  iface->queue_translation = _bdk_quartz_window_queue_translation;
  iface->destroy = _bdk_quartz_window_destroy;
  iface->input_window_destroy = _bdk_input_window_destroy;
  iface->input_window_crossing = _bdk_input_window_crossing;
}
