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

/* Uninstalled header defining types and functions internal to BDK */

#ifndef __BDK_INTERNALS_H__
#define __BDK_INTERNALS_H__

#include <bunnyio/bunnyio.h>
#include <bdk/bdktypes.h>
#include <bdk/bdkwindow.h>
#include <bdk/bdkprivate.h>
#ifdef USE_MEDIALIB
#include <bdk/bdkmedialib.h>
#endif

B_BEGIN_DECLS

/**********************
 * General Facilities * 
 **********************/

/* Debugging support */

typedef struct _BdkColorInfo           BdkColorInfo;
typedef struct _BdkEventFilter	       BdkEventFilter;
typedef struct _BdkClientFilter	       BdkClientFilter;

typedef enum {
  BDK_COLOR_WRITEABLE = 1 << 0
} BdkColorInfoFlags;

struct _BdkColorInfo
{
  BdkColorInfoFlags flags;
  guint ref_count;
};

typedef enum {
  BDK_EVENT_FILTER_REMOVED = 1 << 0
} BdkEventFilterFlags;

struct _BdkEventFilter {
  BdkFilterFunc function;
  gpointer data;
  BdkEventFilterFlags flags;
  guint ref_count;
};

struct _BdkClientFilter {
  BdkAtom       type;
  BdkFilterFunc function;
  gpointer      data;
};

typedef enum {
  BDK_DEBUG_MISC          = 1 << 0,
  BDK_DEBUG_EVENTS        = 1 << 1,
  BDK_DEBUG_DND           = 1 << 2,
  BDK_DEBUG_XIM           = 1 << 3,
  BDK_DEBUG_NOGRABS       = 1 << 4,
  BDK_DEBUG_COLORMAP	  = 1 << 5,
  BDK_DEBUG_BDKRGB	  = 1 << 6,
  BDK_DEBUG_GC		  = 1 << 7,
  BDK_DEBUG_PIXMAP	  = 1 << 8,
  BDK_DEBUG_IMAGE	  = 1 << 9,
  BDK_DEBUG_INPUT	  = 1 <<10,
  BDK_DEBUG_CURSOR	  = 1 <<11,
  BDK_DEBUG_MULTIHEAD	  = 1 <<12,
  BDK_DEBUG_XINERAMA	  = 1 <<13,
  BDK_DEBUG_DRAW	  = 1 <<14,
  BDK_DEBUG_EVENTLOOP     = 1 <<15
} BdkDebugFlag;

#ifndef BDK_DISABLE_DEPRECATED

typedef struct _BdkFontPrivate	       BdkFontPrivate;

struct _BdkFontPrivate
{
  BdkFont font;
  guint ref_count;
};

#endif /* BDK_DISABLE_DEPRECATED */

extern GList            *_bdk_default_filters;
extern BdkWindow  	*_bdk_parent_root;
extern gint		 _bdk_error_code;
extern gint		 _bdk_error_warnings;

extern guint _bdk_debug_flags;
extern gboolean _bdk_native_windows;

#ifdef G_ENABLE_DEBUG

#define BDK_NOTE(type,action)		     B_STMT_START { \
    if (_bdk_debug_flags & BDK_DEBUG_##type)		    \
       { action; };			     } B_STMT_END

#else /* !G_ENABLE_DEBUG */

#define BDK_NOTE(type,action)

#endif /* G_ENABLE_DEBUG */

/* Arg parsing */

typedef enum 
{
  BDK_ARG_STRING,
  BDK_ARG_INT,
  BDK_ARG_BOOL,
  BDK_ARG_NOBOOL,
  BDK_ARG_CALLBACK
} BdkArgType;

typedef struct _BdkArgContext BdkArgContext;
typedef struct _BdkArgDesc BdkArgDesc;

typedef void (*BdkArgFunc) (const char *name, const char *arg, gpointer data);

struct _BdkArgContext
{
  GPtrArray *tables;
  gpointer cb_data;
};

struct _BdkArgDesc
{
  const char *name;
  BdkArgType type;
  gpointer location;
  BdkArgFunc callback;
};

/* Event handling */

typedef struct _BdkEventPrivate BdkEventPrivate;

typedef enum
{
  /* Following flag is set for events on the event queue during
   * translation and cleared afterwards.
   */
  BDK_EVENT_PENDING = 1 << 0
} BdkEventFlags;

struct _BdkEventPrivate
{
  BdkEvent   event;
  guint      flags;
  BdkScreen *screen;
  gpointer   windowing_data;
};

/* Tracks information about the pointer grab on this display */
typedef struct
{
  BdkWindow *window;
  BdkWindow *native_window;
  gulong serial_start;
  gulong serial_end; /* exclusive, i.e. not active on serial_end */
  gboolean owner_events;
  guint event_mask;
  gboolean implicit;
  guint32 time;

  gboolean activated;
  gboolean implicit_ungrab;
} BdkPointerGrabInfo;

typedef struct _BdkInputWindow BdkInputWindow;

/* Private version of BdkWindowObject. The initial part of this strucuture
   is public for historical reasons. Don't change that part */
typedef struct _BdkWindowPaint             BdkWindowPaint;

struct _BdkWindowObject
{
  /* vvvvvvv THIS PART IS PUBLIC. DON'T CHANGE vvvvvvvvvvvvvv */
  BdkDrawable parent_instance;

  BdkDrawable *impl; /* window-system-specific delegate object */  
  
  BdkWindowObject *parent;

  gpointer user_data;

  gint x;
  gint y;
  
  gint extension_events;

  GList *filters;
  GList *children;

  BdkColor bg_color;
  BdkPixmap *bg_pixmap;
  
  GSList *paint_stack;
  
  BdkRebunnyion *update_area;
  guint update_freeze_count;
  
  guint8 window_type;
  guint8 depth;
  guint8 resize_count;

  BdkWindowState state;
  
  guint guffaw_gravity : 1;
  guint input_only : 1;
  guint modal_hint : 1;
  guint composited : 1;
  
  guint destroyed : 2;

  guint accept_focus : 1;
  guint focus_on_map : 1;
  guint shaped : 1;
  
  BdkEventMask event_mask;

  guint update_and_descendants_freeze_count;

  BdkWindowRedirect *redirect;

  /* ^^^^^^^^^^ THIS PART IS PUBLIC. DON'T CHANGE ^^^^^^^^^^ */
  
  /* The BdkWindowObject that has the impl, ref:ed if another window.
   * This ref is required to keep the wrapper of the impl window alive
   * for as long as any BdkWindow references the impl. */
  BdkWindowObject *impl_window; 
  int abs_x, abs_y; /* Absolute offset in impl */
  gint width, height;
  guint32 clip_tag;
  BdkRebunnyion *clip_rebunnyion; /* Clip rebunnyion (wrt toplevel) in window coords */
  BdkRebunnyion *clip_rebunnyion_with_children; /* Clip rebunnyion in window coords */
  BdkCursor *cursor;
  gint8 toplevel_window_type;
  guint synthesize_crossing_event_queued : 1;
  guint effective_visibility : 2;
  guint visibility : 2; /* The visibility wrt the toplevel (i.e. based on clip_rebunnyion) */
  guint native_visibility : 2; /* the native visibility of a impl windows */
  guint viewable : 1; /* mapped and all parents mapped */
  guint applied_shape : 1;

  guint num_offscreen_children;
  BdkWindowPaint *implicit_paint;
  BdkInputWindow *input_window; /* only set for impl windows */

  GList *outstanding_moves;

  BdkRebunnyion *shape;
  BdkRebunnyion *input_shape;
  
  bairo_surface_t *bairo_surface;
  guint outstanding_surfaces; /* only set on impl window */

  bairo_pattern_t *background;
};

#define BDK_WINDOW_TYPE(d) (((BdkWindowObject*)(BDK_WINDOW (d)))->window_type)
#define BDK_WINDOW_DESTROYED(d) (((BdkWindowObject*)(BDK_WINDOW (d)))->destroyed)

extern BdkEventFunc   _bdk_event_func;    /* Callback for events */
extern gpointer       _bdk_event_data;
extern GDestroyNotify _bdk_event_notify;

extern GSList    *_bdk_displays;
extern gchar     *_bdk_display_name;
extern gint       _bdk_screen_number;
extern gchar     *_bdk_display_arg_name;

void      _bdk_events_queue  (BdkDisplay *display);
BdkEvent* _bdk_event_unqueue (BdkDisplay *display);

void _bdk_event_filter_unref        (BdkWindow      *window,
				     BdkEventFilter *filter);

GList* _bdk_event_queue_find_first   (BdkDisplay *display);
void   _bdk_event_queue_remove_link  (BdkDisplay *display,
				      GList      *node);
GList* _bdk_event_queue_prepend      (BdkDisplay *display,
				      BdkEvent   *event);
GList* _bdk_event_queue_append       (BdkDisplay *display,
				      BdkEvent   *event);
GList* _bdk_event_queue_insert_after (BdkDisplay *display,
                                      BdkEvent   *after_event,
                                      BdkEvent   *event);
GList* _bdk_event_queue_insert_before(BdkDisplay *display,
                                      BdkEvent   *after_event,
                                      BdkEvent   *event);
void   _bdk_event_button_generate    (BdkDisplay *display,
				      BdkEvent   *event);

void _bdk_windowing_event_data_copy (const BdkEvent *src,
                                     BdkEvent       *dst);
void _bdk_windowing_event_data_free (BdkEvent       *event);

void bdk_synthesize_window_state (BdkWindow     *window,
                                  BdkWindowState unset_flags,
                                  BdkWindowState set_flags);

#define BDK_SCRATCH_IMAGE_WIDTH 256
#define BDK_SCRATCH_IMAGE_HEIGHT 64

BdkImage* _bdk_image_new_for_depth (BdkScreen    *screen,
				    BdkImageType  type,
				    BdkVisual    *visual,
				    gint          width,
				    gint          height,
				    gint          depth);
BdkImage *_bdk_image_get_scratch (BdkScreen *screen,
				  gint	     width,
				  gint	     height,
				  gint	     depth,
				  gint	    *x,
				  gint	    *y);

BdkImage *_bdk_drawable_copy_to_image (BdkDrawable  *drawable,
				       BdkImage     *image,
				       gint          src_x,
				       gint          src_y,
				       gint          dest_x,
				       gint          dest_y,
				       gint          width,
				       gint          height);

bairo_surface_t *_bdk_drawable_ref_bairo_surface (BdkDrawable *drawable);

BdkDrawable *_bdk_drawable_get_source_drawable (BdkDrawable *drawable);
bairo_surface_t * _bdk_drawable_create_bairo_surface (BdkDrawable *drawable,
						      int width,
						      int height);

/* GC caching */
BdkGC *_bdk_drawable_get_scratch_gc (BdkDrawable *drawable,
				     gboolean     graphics_exposures);
BdkGC *_bdk_drawable_get_subwindow_scratch_gc (BdkDrawable *drawable);

void _bdk_gc_update_context (BdkGC          *gc,
			     bairo_t        *cr,
			     const BdkColor *override_foreground,
			     BdkBitmap      *override_stipple,
			     gboolean        gc_changed,
			     BdkDrawable    *target_drawable);

/*************************************
 * Interfaces used by windowing code *
 *************************************/

BdkPixmap *_bdk_pixmap_new               (BdkDrawable    *drawable,
                                          gint            width,
                                          gint            height,
                                          gint            depth);
BdkPixmap *_bdk_pixmap_create_from_data  (BdkDrawable    *drawable,
                                          const gchar    *data,
                                          gint            width,
                                          gint            height,
                                          gint            depth,
                                          const BdkColor *fg,
                                          const BdkColor *bg);
BdkPixmap *_bdk_bitmap_create_from_data  (BdkDrawable    *drawable,
                                          const gchar    *data,
                                          gint            width,
                                          gint            height);

void       _bdk_window_impl_new          (BdkWindow      *window,
					  BdkWindow      *real_parent,
					  BdkScreen      *screen,
					  BdkVisual      *visual,
					  BdkEventMask    event_mask,
                                          BdkWindowAttr  *attributes,
                                          gint            attributes_mask);
void       _bdk_window_destroy           (BdkWindow      *window,
                                          gboolean        foreign_destroy);
void       _bdk_window_clear_update_area (BdkWindow      *window);
void       _bdk_window_update_size       (BdkWindow      *window);
gboolean   _bdk_window_update_viewable   (BdkWindow      *window);

void       _bdk_window_process_updates_recurse (BdkWindow *window,
                                                BdkRebunnyion *expose_rebunnyion);

void       _bdk_screen_close             (BdkScreen      *screen);

const char *_bdk_get_sm_client_id (void);

void _bdk_gc_init (BdkGC           *gc,
		   BdkDrawable     *drawable,
		   BdkGCValues     *values,
		   BdkGCValuesMask  values_mask);

BdkRebunnyion *_bdk_gc_get_clip_rebunnyion (BdkGC *gc);
BdkBitmap *_bdk_gc_get_clip_mask   (BdkGC *gc);
gboolean   _bdk_gc_get_exposures   (BdkGC *gc);
BdkFill    _bdk_gc_get_fill        (BdkGC *gc);
BdkPixmap *_bdk_gc_get_tile        (BdkGC *gc);
BdkBitmap *_bdk_gc_get_stipple     (BdkGC *gc);
guint32    _bdk_gc_get_fg_pixel    (BdkGC *gc);
guint32    _bdk_gc_get_bg_pixel    (BdkGC *gc);
void      _bdk_gc_add_drawable_clip     (BdkGC     *gc,
					 guint32    rebunnyion_tag,
					 BdkRebunnyion *rebunnyion,
					 int        offset_x,
					 int        offset_y);
void      _bdk_gc_remove_drawable_clip  (BdkGC     *gc);
void       _bdk_gc_set_clip_rebunnyion_internal (BdkGC     *gc,
					     BdkRebunnyion *rebunnyion,
					     gboolean reset_origin);
BdkSubwindowMode _bdk_gc_get_subwindow (BdkGC *gc);

BdkDrawable *_bdk_drawable_begin_direct_draw (BdkDrawable *drawable,
					      BdkGC *gc,
					      gpointer *priv_data,
					      gint *x_offset_out,
					      gint *y_offset_out);
void         _bdk_drawable_end_direct_draw (gpointer priv_data);


/*****************************************
 * Interfaces provided by windowing code *
 *****************************************/

/* Font/string functions implemented in module-specific code */
gint _bdk_font_strlen (BdkFont *font, const char *str);
void _bdk_font_destroy (BdkFont *font);

void _bdk_colormap_real_destroy (BdkColormap *colormap);

void _bdk_cursor_destroy (BdkCursor *cursor);

void     _bdk_windowing_init                    (void);

extern const GOptionEntry _bdk_windowing_args[];
void     _bdk_windowing_set_default_display     (BdkDisplay *display);

gchar *_bdk_windowing_substitute_screen_number (const gchar *display_name,
					        gint         screen_number);

gulong   _bdk_windowing_window_get_next_serial  (BdkDisplay *display);
void     _bdk_windowing_window_get_offsets      (BdkWindow  *window,
						 gint       *x_offset,
						 gint       *y_offset);
BdkRebunnyion *_bdk_windowing_window_get_shape      (BdkWindow  *window);
BdkRebunnyion *_bdk_windowing_window_get_input_shape(BdkWindow  *window);
BdkRebunnyion *_bdk_windowing_get_shape_for_mask    (BdkBitmap *mask);
void     _bdk_windowing_window_beep             (BdkWindow *window);


void       _bdk_windowing_get_pointer        (BdkDisplay       *display,
					      BdkScreen       **screen,
					      gint             *x,
					      gint             *y,
					      BdkModifierType  *mask);
BdkWindow* _bdk_windowing_window_at_pointer  (BdkDisplay       *display,
					      gint             *win_x,
					      gint             *win_y,
					      BdkModifierType  *mask,
					      gboolean          get_toplevel);
BdkGrabStatus _bdk_windowing_pointer_grab    (BdkWindow        *window,
					      BdkWindow        *native,
					      gboolean          owner_events,
					      BdkEventMask      event_mask,
					      BdkWindow        *confine_to,
					      BdkCursor        *cursor,
					      guint32           time);
void _bdk_windowing_got_event                (BdkDisplay       *display,
					      GList            *event_link,
					      BdkEvent         *event,
					      gulong            serial);

void _bdk_windowing_window_process_updates_recurse (BdkWindow *window,
                                                    BdkRebunnyion *expose_rebunnyion);
void _bdk_windowing_before_process_all_updates     (void);
void _bdk_windowing_after_process_all_updates      (void);

/* Return the number of bits-per-pixel for images of the specified depth. */
gint _bdk_windowing_get_bits_for_depth (BdkDisplay *display,
					gint        depth);


#define BDK_WINDOW_IS_MAPPED(window) ((((BdkWindowObject*)window)->state & BDK_WINDOW_STATE_WITHDRAWN) == 0)


/* Called when bdk_window_destroy() is called on a foreign window
 * or an ancestor of the foreign window. It should generally reparent
 * the window out of it's current heirarchy, hide it, and then
 * send a message to the owner requesting that the window be destroyed.
 */
void _bdk_windowing_window_destroy_foreign (BdkWindow *window);

void _bdk_windowing_display_set_sm_client_id (BdkDisplay  *display,
					      const gchar *sm_client_id);

void _bdk_windowing_window_set_composited (BdkWindow *window,
					   gboolean composited);

#define BDK_TYPE_PAINTABLE            (_bdk_paintable_get_type ())
#define BDK_PAINTABLE(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BDK_TYPE_PAINTABLE, BdkPaintable))
#define BDK_IS_PAINTABLE(obj)	      (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BDK_TYPE_PAINTABLE))
#define BDK_PAINTABLE_GET_IFACE(obj)  (B_TYPE_INSTANCE_GET_INTERFACE ((obj), BDK_TYPE_PAINTABLE, BdkPaintableIface))

typedef struct _BdkPaintable        BdkPaintable;
typedef struct _BdkPaintableIface   BdkPaintableIface;

struct _BdkPaintableIface
{
  GTypeInterface g_iface;
  
  void (* begin_paint_rebunnyion)       (BdkPaintable    *paintable,
                                     BdkWindow       *window,
                                     const BdkRebunnyion *rebunnyion);
  void (* end_paint)                (BdkPaintable    *paintable);
};

GType _bdk_paintable_get_type (void) B_GNUC_CONST;

/* Implementation types */
GType _bdk_window_impl_get_type (void) B_GNUC_CONST;
GType _bdk_pixmap_impl_get_type (void) B_GNUC_CONST;


/**
 * _bdk_windowing_gc_set_clip_rebunnyion:
 * @gc: a #BdkGC
 * @rebunnyion: the new clip rebunnyion
 * @reset_origin: if TRUE, reset the clip_x/y_origin values to 0
 * 
 * Do any window-system specific processing necessary
 * for a change in clip rebunnyion. Since the clip origin
 * will likely change before the GC is used with the
 * new clip, frequently this function will only set a flag and
 * do the real processing later.
 *
 * When this function is called, _bdk_gc_get_clip_rebunnyion
 * will already return the new rebunnyion.
 **/
void _bdk_windowing_gc_set_clip_rebunnyion (BdkGC           *gc,
					const BdkRebunnyion *rebunnyion,
					gboolean reset_origin);

/**
 * _bdk_windowing_gc_copy:
 * @dst_gc: a #BdkGC from the BDK backend
 * @src_gc: a #BdkGC from the BDK backend
 * 
 * Copies backend specific state from @src_gc to @dst_gc.
 * This is called before the generic state is copied, so
 * the old generic state is still available from @dst_gc
 **/
void _bdk_windowing_gc_copy (BdkGC *dst_gc,
			     BdkGC *src_gc);
     
/* Queries the current foreground color of a BdkGC */
void _bdk_windowing_gc_get_foreground (BdkGC    *gc,
				       BdkColor *color);
/* Queries the current background color of a BdkGC */
void _bdk_windowing_gc_get_background (BdkGC    *gc,
				       BdkColor *color);

struct BdkAppLaunchContextPrivate
{
  BdkDisplay *display;
  BdkScreen *screen;
  gint workspace;
  guint32 timestamp;
  GIcon *icon;
  char *icon_name;
};

char *_bdk_windowing_get_startup_notify_id (GAppLaunchContext *context,
					    GAppInfo          *info, 
					    GList             *files);
void  _bdk_windowing_launch_failed         (GAppLaunchContext *context, 
				            const char        *startup_notify_id);

BdkPointerGrabInfo *_bdk_display_get_active_pointer_grab (BdkDisplay *display);
void _bdk_display_pointer_grab_update                    (BdkDisplay *display,
							  gulong current_serial);
BdkPointerGrabInfo *_bdk_display_get_last_pointer_grab (BdkDisplay *display);
BdkPointerGrabInfo *_bdk_display_add_pointer_grab  (BdkDisplay *display,
						    BdkWindow *window,
						    BdkWindow *native_window,
						    gboolean owner_events,
						    BdkEventMask event_mask,
						    unsigned long serial_start,
						    guint32 time,
						    gboolean implicit);
BdkPointerGrabInfo * _bdk_display_has_pointer_grab (BdkDisplay *display,
						    gulong serial);
gboolean _bdk_display_end_pointer_grab (BdkDisplay *display,
					gulong serial,
					BdkWindow *if_child,
					gboolean implicit);
void _bdk_display_set_has_keyboard_grab (BdkDisplay *display,
					 BdkWindow *window,
					 BdkWindow *native_window,
					 gboolean owner_events,
					 unsigned long serial,
					 guint32 time);
void _bdk_display_unset_has_keyboard_grab (BdkDisplay *display,
					   gboolean implicit);
void _bdk_display_enable_motion_hints     (BdkDisplay *display);


void _bdk_window_invalidate_for_expose (BdkWindow       *window,
					BdkRebunnyion       *rebunnyion);

void _bdk_windowing_set_bairo_surface_size (bairo_surface_t *surface,
					    int width,
					    int height);

bairo_surface_t * _bdk_windowing_create_bairo_surface (BdkDrawable *drawable,
						       int width,
						       int height);
BdkWindow * _bdk_window_find_child_at (BdkWindow *window,
				       int x, int y);
BdkWindow * _bdk_window_find_descendant_at (BdkWindow *toplevel,
					    double x, double y,
					    double *found_x,
					    double *found_y);

void _bdk_window_add_damage (BdkWindow *toplevel,
			     BdkRebunnyion *damaged_rebunnyion);

BdkEvent * _bdk_make_event (BdkWindow    *window,
			    BdkEventType  type,
			    BdkEvent     *event_in_queue,
			    gboolean      before_event);
gboolean _bdk_window_event_parent_of (BdkWindow *parent,
                                      BdkWindow *child);

void _bdk_synthesize_crossing_events (BdkDisplay                 *display,
				      BdkWindow                  *src,
				      BdkWindow                  *dest,
				      BdkCrossingMode             mode,
				      gint                        toplevel_x,
				      gint                        toplevel_y,
				      BdkModifierType             mask,
				      guint32                     time_,
				      BdkEvent                   *event_in_queue,
				      gulong                      serial,
				      gboolean                    non_linear);
void _bdk_display_set_window_under_pointer (BdkDisplay *display,
					    BdkWindow *window);


void _bdk_synthesize_crossing_events_for_geometry_change (BdkWindow *changed_window);

BdkRebunnyion *_bdk_window_calculate_full_clip_rebunnyion    (BdkWindow     *window,
                                                      BdkWindow     *base_window,
                                                      gboolean       do_children,
                                                      gint          *base_x_offset,
                                                      gint          *base_y_offset);
gboolean    _bdk_window_has_impl (BdkWindow *window);
BdkWindow * _bdk_window_get_impl_window (BdkWindow *window);
BdkWindow *_bdk_window_get_input_window_for_event (BdkWindow *native_window,
						   BdkEventType event_type,
						   BdkModifierType mask,
						   int x, int y,
						   gulong serial);
BdkRebunnyion  *_bdk_rebunnyion_new_from_yxbanded_rects (BdkRectangle *rects, int n_rects);

/*****************************
 * offscreen window routines *
 *****************************/
typedef struct _BdkOffscreenWindow      BdkOffscreenWindow;
#define BDK_TYPE_OFFSCREEN_WINDOW            (bdk_offscreen_window_get_type())
#define BDK_OFFSCREEN_WINDOW(object)         (B_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_OFFSCREEN_WINDOW, BdkOffscreenWindow))
#define BDK_IS_OFFSCREEN_WINDOW(object)      (B_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_OFFSCREEN_WINDOW))
GType bdk_offscreen_window_get_type (void);
BdkDrawable * _bdk_offscreen_window_get_real_drawable (BdkOffscreenWindow *window);
void       _bdk_offscreen_window_new                 (BdkWindow     *window,
						      BdkScreen     *screen,
						      BdkVisual     *visual,
						      BdkWindowAttr *attributes,
						      gint           attributes_mask);


/************************************
 * Initialization and exit routines *
 ************************************/

void _bdk_image_exit  (void);
void _bdk_windowing_exit (void);

B_END_DECLS

#endif /* __BDK_INTERNALS_H__ */
