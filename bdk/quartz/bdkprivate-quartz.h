/* bdkwindow-quartz.c
 *
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

#ifndef __BDK_PRIVATE_QUARTZ_H__
#define __BDK_PRIVATE_QUARTZ_H__

#define BDK_QUARTZ_ALLOC_POOL NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init]
#define BDK_QUARTZ_RELEASE_POOL [pool release]

#include <bdk/bdkprivate.h>
#include <bdk/quartz/bdkpixmap-quartz.h>
#include <bdk/quartz/bdkwindow-quartz.h>
#include <bdk/quartz/bdkquartz.h>

#include <bdk/bdk.h>

#include "bdkinternals.h"

#include "config.h"

#define BDK_TYPE_GC_QUARTZ              (_bdk_gc_quartz_get_type ())
#define BDK_GC_QUARTZ(object)           (B_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_GC_QUARTZ, BdkGCQuartz))
#define BDK_GC_QUARTZ_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_GC_QUARTZ, BdkGCQuartzClass))
#define BDK_IS_GC_QUARTZ(object)        (B_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_GC_QUARTZ))
#define BDK_IS_GC_QUARTZ_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_GC_QUARTZ))
#define BDK_GC_QUARTZ_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_GC_QUARTZ, BdkGCQuartzClass))

#define BDK_DRAG_CONTEXT_PRIVATE(context) ((BdkDragContextPrivate *) BDK_DRAG_CONTEXT (context)->windowing_data)

typedef struct _BdkCursorPrivate BdkCursorPrivate;
typedef struct _BdkGCQuartz       BdkGCQuartz;
typedef struct _BdkGCQuartzClass  BdkGCQuartzClass;
typedef struct _BdkDragContextPrivate BdkDragContextPrivate;

struct _BdkGCQuartz
{
  BdkGC             parent_instance;

  BdkFont          *font;
  BdkFunction       function;
  BdkSubwindowMode  subwindow_mode;
  gboolean          graphics_exposures;

  gboolean          have_clip_rebunnyion;
  gboolean          have_clip_mask;
  CGImageRef        clip_mask;

  gint              line_width;
  BdkLineStyle      line_style;
  BdkCapStyle       cap_style;
  BdkJoinStyle      join_style;

  CGFloat          *dash_lengths;
  gint              dash_count;
  CGFloat           dash_phase;

  CGPatternRef      ts_pattern;
  void             *ts_pattern_info;

  guint             is_window : 1;
};

struct _BdkGCQuartzClass
{
  BdkGCClass parent_class;
};

struct _BdkVisualClass
{
  BObjectClass parent_class;
};

struct _BdkCursorPrivate
{
  BdkCursor cursor;
  NSCursor *nscursor;
};

struct _BdkDragContextPrivate
{
  id <NSDraggingInfo> dragging_info;
};

extern BdkDisplay *_bdk_display;
extern BdkScreen *_bdk_screen;
extern BdkWindow *_bdk_root;

extern BdkDragContext *_bdk_quartz_drag_source_context;

#define BDK_WINDOW_IS_QUARTZ(win)        (BDK_IS_WINDOW_IMPL_QUARTZ (((BdkWindowObject *)win)->impl))

/* Initialization */
void _bdk_windowing_update_window_sizes     (BdkScreen *screen);
void _bdk_windowing_window_init             (void);
void _bdk_events_init                       (void);
void _bdk_visual_init                       (void);
void _bdk_input_init                        (void);
void _bdk_quartz_event_loop_init            (void);

/* GC */
typedef enum {
  BDK_QUARTZ_CONTEXT_STROKE = 1 << 0,
  BDK_QUARTZ_CONTEXT_FILL   = 1 << 1,
  BDK_QUARTZ_CONTEXT_TEXT   = 1 << 2
} BdkQuartzContextValuesMask;

GType  _bdk_gc_quartz_get_type          (void);
BdkGC *_bdk_quartz_gc_new               (BdkDrawable                *drawable,
					 BdkGCValues                *values,
					 BdkGCValuesMask             values_mask);
gboolean _bdk_quartz_gc_update_cg_context (BdkGC                      *gc,
					   BdkDrawable                *drawable,
					   CGContextRef                context,
					   BdkQuartzContextValuesMask  mask);

/* Colormap */
CGColorRef _bdk_quartz_colormap_get_cgcolor_from_pixel (BdkDrawable *drawable,
                                                        guint32      pixel);

/* Window */
gboolean    _bdk_quartz_window_is_ancestor          (BdkWindow *ancestor,
                                                     BdkWindow *window);
void       _bdk_quartz_window_bdk_xy_to_xy          (gint       bdk_x,
                                                     gint       bdk_y,
                                                     gint      *ns_x,
                                                     gint      *ns_y);
void       _bdk_quartz_window_xy_to_bdk_xy          (gint       ns_x,
                                                     gint       ns_y,
                                                     gint      *bdk_x,
                                                     gint      *bdk_y);
void       _bdk_quartz_window_nspoint_to_bdk_xy     (NSPoint    point,
                                                     gint      *x,
                                                     gint      *y);
BdkWindow *_bdk_quartz_window_find_child            (BdkWindow *window,
						     gint       x,
						     gint       y);
void       _bdk_quartz_window_attach_to_parent      (BdkWindow *window);
void       _bdk_quartz_window_detach_from_parent    (BdkWindow *window);
void       _bdk_quartz_window_did_become_main       (BdkWindow *window);
void       _bdk_quartz_window_did_resign_main       (BdkWindow *window);
void       _bdk_quartz_window_debug_highlight       (BdkWindow *window,
                                                     gint       number);

void       _bdk_quartz_window_set_needs_display_in_rect (BdkWindow    *window,
                                                         BdkRectangle *rect);
void       _bdk_quartz_window_set_needs_display_in_rebunnyion (BdkWindow    *window,
                                                           BdkRebunnyion    *rebunnyion);

void       _bdk_quartz_window_update_position           (BdkWindow    *window);

/* Events */
typedef enum {
  BDK_QUARTZ_EVENT_SUBTYPE_EVENTLOOP
} BdkQuartzEventSubType;

void         _bdk_quartz_events_update_focus_window    (BdkWindow *new_window,
                                                        gboolean   got_focus);
void         _bdk_quartz_events_send_map_event         (BdkWindow *window);
BdkEventMask _bdk_quartz_events_get_current_event_mask (void);

BdkModifierType _bdk_quartz_events_get_current_keyboard_modifiers (void);
BdkModifierType _bdk_quartz_events_get_current_mouse_modifiers    (void);

void         _bdk_quartz_events_break_all_grabs         (guint32    time);

/* Event loop */
gboolean   _bdk_quartz_event_loop_check_pending (void);
NSEvent *  _bdk_quartz_event_loop_get_pending   (void);
void       _bdk_quartz_event_loop_release_event (NSEvent *event);

/* FIXME: image */
BdkImage *_bdk_quartz_image_copy_to_image (BdkDrawable *drawable,
					    BdkImage    *image,
					    gint         src_x,
					    gint         src_y,
					    gint         dest_x,
					    gint         dest_y,
					    gint         width,
					    gint         height);

/* Keys */
BdkEventType _bdk_quartz_keys_event_type  (NSEvent   *event);
gboolean     _bdk_quartz_keys_is_modifier (guint      keycode);
void         _bdk_quartz_synthesize_null_key_event (BdkWindow *window);

/* Drawable */
void        _bdk_quartz_drawable_finish (BdkDrawable *drawable);
void        _bdk_quartz_drawable_flush  (BdkDrawable *drawable);

/* Geometry */
void        _bdk_quartz_window_scroll      (BdkWindow       *window,
                                            gint             dx,
                                            gint             dy);
void        _bdk_quartz_window_queue_translation (BdkWindow *window,
						  BdkGC     *gc,
                                                  BdkRebunnyion *area,
                                                  gint       dx,
                                                  gint       dy);
gboolean    _bdk_quartz_window_queue_antiexpose  (BdkWindow *window,
                                                  BdkRebunnyion *area);

/* Pixmap */
CGImageRef _bdk_pixmap_get_cgimage (BdkPixmap *pixmap);

#endif /* __BDK_PRIVATE_QUARTZ_H__ */
