/*
 * bdkdisplay.h
 * 
 * Copyright 2001 Sun Microsystems Inc. 
 *
 * Erwann Chenede <erwann.chenede@sun.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __BDK_DISPLAY_H__
#define __BDK_DISPLAY_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bdk/bdktypes.h>
#include <bdk/bdkevents.h>

G_BEGIN_DECLS

typedef struct _BdkDisplayClass BdkDisplayClass;
typedef struct _BdkDisplayPointerHooks BdkDisplayPointerHooks;

#define BDK_TYPE_DISPLAY              (bdk_display_get_type ())
#define BDK_DISPLAY_OBJECT(object)    (G_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_DISPLAY, BdkDisplay))
#define BDK_DISPLAY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_DISPLAY, BdkDisplayClass))
#define BDK_IS_DISPLAY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_DISPLAY))
#define BDK_IS_DISPLAY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_DISPLAY))
#define BDK_DISPLAY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_DISPLAY, BdkDisplayClass))

/* Tracks information about the keyboard grab on this display */
typedef struct
{
  BdkWindow *window;
  BdkWindow *native_window;
  gulong serial;
  gboolean owner_events;
  guint32 time;
} BdkKeyboardGrabInfo;

/* Tracks information about which window and position the pointer last was in.
 * This is useful when we need to synthesize events later.
 * Note that we track toplevel_under_pointer using enter/leave events,
 * so in the case of a grab, either with owner_events==FALSE or with the
 * pointer in no clients window the x/y coordinates may actually be outside
 * the window.
 */
typedef struct
{
  BdkWindow *toplevel_under_pointer; /* The toplevel window with mouse inside, tracked via native events */
  BdkWindow *window_under_pointer; /* The window that last got sent a normal enter event */
  gdouble toplevel_x, toplevel_y; 
  guint32 state;
  guint32 button;
  gulong motion_hint_serial; /* 0 == didn't deliver hinted motion event */
} BdkPointerWindowInfo;

struct _BdkDisplay
{
  GObject parent_instance;

  /*< private >*/
  GList *GSEAL (queued_events);
  GList *GSEAL (queued_tail);

  /* Information for determining if the latest button click
   * is part of a double-click or triple-click
   */
  guint32 GSEAL (button_click_time[2]);	/* The last 2 button click times. */
  BdkWindow *GSEAL (button_window[2]);  /* The last 2 windows to receive button presses. */
  gint GSEAL (button_number[2]);        /* The last 2 buttons to be pressed. */

  guint GSEAL (double_click_time);	/* Maximum time between clicks in msecs */
  BdkDevice *GSEAL (core_pointer);	/* Core pointer device */

  const BdkDisplayPointerHooks *GSEAL (pointer_hooks); /* Current hooks for querying pointer */
  
  guint GSEAL (closed) : 1;		/* Whether this display has been closed */
  guint GSEAL (ignore_core_events) : 1; /* Don't send core motion and button event */

  guint GSEAL (double_click_distance);	/* Maximum distance between clicks in pixels */
  gint GSEAL (button_x[2]);             /* The last 2 button click positions. */
  gint GSEAL (button_y[2]);

  GList *GSEAL (pointer_grabs);
  BdkKeyboardGrabInfo GSEAL (keyboard_grab);
  BdkPointerWindowInfo GSEAL (pointer_info);

  /* Last reported event time from server */
  guint32 GSEAL (last_event_time);
};

struct _BdkDisplayClass
{
  GObjectClass parent_class;
  
  const gchar *              (*get_display_name)   (BdkDisplay *display);
  gint			     (*get_n_screens)      (BdkDisplay *display);
  BdkScreen *		     (*get_screen)         (BdkDisplay *display,
						    gint        screen_num);
  BdkScreen *		     (*get_default_screen) (BdkDisplay *display);

  
  /* Signals */
  void (*closed) (BdkDisplay *display,
		  gboolean    is_error);
};

struct _BdkDisplayPointerHooks
{
  void (*get_pointer)              (BdkDisplay      *display,
				    BdkScreen      **screen,
				    gint            *x,
				    gint            *y,
				    BdkModifierType *mask);
  BdkWindow* (*window_get_pointer) (BdkDisplay      *display,
				    BdkWindow       *window,
				    gint            *x,
				    gint            *y,
				    BdkModifierType *mask);
  BdkWindow* (*window_at_pointer)  (BdkDisplay      *display,
				    gint            *win_x,
				    gint            *win_y);
};

GType       bdk_display_get_type (void) G_GNUC_CONST;
BdkDisplay *bdk_display_open                (const gchar *display_name);

const gchar * bdk_display_get_name         (BdkDisplay *display);

gint        bdk_display_get_n_screens      (BdkDisplay  *display);
BdkScreen * bdk_display_get_screen         (BdkDisplay  *display,
					    gint         screen_num);
BdkScreen * bdk_display_get_default_screen (BdkDisplay  *display);
void        bdk_display_pointer_ungrab     (BdkDisplay  *display,
					    guint32      time_);
void        bdk_display_keyboard_ungrab    (BdkDisplay  *display,
					    guint32      time_);
gboolean    bdk_display_pointer_is_grabbed (BdkDisplay  *display);
void        bdk_display_beep               (BdkDisplay  *display);
void        bdk_display_sync               (BdkDisplay  *display);
void        bdk_display_flush              (BdkDisplay  *display);

void	    bdk_display_close		       (BdkDisplay  *display);
gboolean    bdk_display_is_closed          (BdkDisplay  *display);

GList *     bdk_display_list_devices       (BdkDisplay  *display);

BdkEvent* bdk_display_get_event  (BdkDisplay     *display);
BdkEvent* bdk_display_peek_event (BdkDisplay     *display);
void      bdk_display_put_event  (BdkDisplay     *display,
				  const BdkEvent *event);

void bdk_display_add_client_message_filter (BdkDisplay   *display,
					    BdkAtom       message_type,
					    BdkFilterFunc func,
					    gpointer      data);

void bdk_display_set_double_click_time     (BdkDisplay   *display,
					    guint         msec);
void bdk_display_set_double_click_distance (BdkDisplay   *display,
					    guint         distance);

BdkDisplay *bdk_display_get_default (void);

BdkDevice  *bdk_display_get_core_pointer (BdkDisplay *display);

void             bdk_display_get_pointer           (BdkDisplay             *display,
						    BdkScreen             **screen,
						    gint                   *x,
						    gint                   *y,
						    BdkModifierType        *mask);
BdkWindow *      bdk_display_get_window_at_pointer (BdkDisplay             *display,
						    gint                   *win_x,
						    gint                   *win_y);
void             bdk_display_warp_pointer          (BdkDisplay             *display,
						    BdkScreen              *screen,
						    gint                   x,
						    gint                   y);

#ifndef BDK_DISABLE_DEPRECATED
BdkDisplayPointerHooks *bdk_display_set_pointer_hooks (BdkDisplay                   *display,
						       const BdkDisplayPointerHooks *new_hooks);
#endif

BdkDisplay *bdk_display_open_default_libbtk_only (void);

gboolean bdk_display_supports_cursor_alpha     (BdkDisplay    *display);
gboolean bdk_display_supports_cursor_color     (BdkDisplay    *display);
guint    bdk_display_get_default_cursor_size   (BdkDisplay    *display);
void     bdk_display_get_maximal_cursor_size   (BdkDisplay    *display,
						guint         *width,
						guint         *height);

BdkWindow *bdk_display_get_default_group       (BdkDisplay *display); 

gboolean bdk_display_supports_selection_notification (BdkDisplay *display);
gboolean bdk_display_request_selection_notification  (BdkDisplay *display,
						      BdkAtom     selection);

gboolean bdk_display_supports_clipboard_persistence (BdkDisplay    *display);
void     bdk_display_store_clipboard                (BdkDisplay    *display,
						     BdkWindow     *clipboard_window,
						     guint32        time_,
						     const BdkAtom *targets,
						     gint           n_targets);

gboolean bdk_display_supports_shapes           (BdkDisplay    *display);
gboolean bdk_display_supports_input_shapes     (BdkDisplay    *display);
gboolean bdk_display_supports_composite        (BdkDisplay    *display);

G_END_DECLS

#endif	/* __BDK_DISPLAY_H__ */
