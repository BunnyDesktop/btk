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

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

/*
 * Private uninstalled header defining things local to X windowing code
 */

#ifndef __BDK_PRIVATE_X11_H__
#define __BDK_PRIVATE_X11_H__

#include <bdk/bdkcursor.h>
#include <bdk/bdkprivate.h>
#include <bdk/x11/bdkwindow-x11.h>
#include <bdk/x11/bdkpixmap-x11.h>
#include <bdk/x11/bdkdisplay-x11.h>
#include <bdk/bdkinternals.h>

#define BDK_TYPE_GC_X11              (_bdk_gc_x11_get_type ())
#define BDK_GC_X11(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_GC_X11, BdkGCX11))
#define BDK_GC_X11_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_GC_X11, BdkGCX11Class))
#define BDK_IS_GC_X11(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_GC_X11))
#define BDK_IS_GC_X11_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_GC_X11))
#define BDK_GC_X11_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_GC_X11, BdkGCX11Class))

typedef struct _BdkCursorPrivate       BdkCursorPrivate;
typedef struct _BdkVisualPrivate       BdkVisualPrivate;
typedef struct _BdkGCX11      BdkGCX11;
typedef struct _BdkGCX11Class BdkGCX11Class;

struct _BdkGCX11
{
  BdkGC parent_instance;
  
  GC xgc;
  BdkScreen *screen;
  guint16 dirty_mask;
  guint have_clip_rebunnyion : 1;
  guint have_clip_mask : 1;
  guint depth : 8;
};

struct _BdkGCX11Class
{
  BdkGCClass parent_class;
};

struct _BdkCursorPrivate
{
  BdkCursor cursor;
  Cursor xcursor;
  BdkDisplay *display;
  gchar *name;
  guint serial;
};

struct _BdkVisualPrivate
{
  BdkVisual visual;
  Visual *xvisual;
  BdkScreen *screen;
};

#define XID_FONT_BIT (1<<31)

void _bdk_xid_table_insert (BdkDisplay *display,
			    XID        *xid,
			    gpointer    data);
void _bdk_xid_table_remove (BdkDisplay *display,
			    XID         xid);
gint _bdk_send_xevent      (BdkDisplay *display,
			    Window      window,
			    gboolean    propagate,
			    glong       event_mask,
			    XEvent     *event_send);

GType _bdk_gc_x11_get_type (void);

gboolean _bdk_x11_have_render           (BdkDisplay *display);

BdkGC *_bdk_x11_gc_new                  (BdkDrawable     *drawable,
					 BdkGCValues     *values,
					 BdkGCValuesMask  values_mask);

BdkImage *_bdk_x11_copy_to_image       (BdkDrawable *drawable,
					BdkImage    *image,
					gint         src_x,
					gint         src_y,
					gint         dest_x,
					gint         dest_y,
					gint         width,
					gint         height);
Pixmap   _bdk_x11_image_get_shm_pixmap (BdkImage    *image);

/* Routines from bdkgeometry-x11.c */
void _bdk_window_move_resize_child (BdkWindow     *window,
                                    gint           x,
                                    gint           y,
                                    gint           width,
                                    gint           height);
void _bdk_window_process_expose    (BdkWindow     *window,
                                    gulong         serial,
                                    BdkRectangle  *area);

gboolean _bdk_x11_window_queue_antiexpose  (BdkWindow *window,
					    BdkRebunnyion *area);
void     _bdk_x11_window_queue_translation (BdkWindow *window,
					    BdkGC     *gc,
					    BdkRebunnyion *area,
					    gint       dx,
					    gint       dy);

void     _bdk_selection_window_destroyed   (BdkWindow            *window);
gboolean _bdk_selection_filter_clear_event (XSelectionClearEvent *event);

BdkRebunnyion* _xwindow_get_shape              (Display *xdisplay,
                                            Window window,
                                            gint shape_type);

void     _bdk_rebunnyion_get_xrectangles       (const BdkRebunnyion      *rebunnyion,
                                            gint                  x_offset,
                                            gint                  y_offset,
                                            XRectangle          **rects,
                                            gint                 *n_rects);

gboolean _bdk_moveresize_handle_event   (XEvent     *event);
gboolean _bdk_moveresize_configure_done (BdkDisplay *display,
					 BdkWindow  *window);

void _bdk_keymap_state_changed    (BdkDisplay      *display,
				   XEvent          *event);
void _bdk_keymap_keys_changed     (BdkDisplay      *display);
gint _bdk_x11_get_group_for_state (BdkDisplay      *display,
				   BdkModifierType  state);
void _bdk_keymap_add_virtual_modifiers_compat (BdkKeymap       *keymap,
                                               BdkModifierType *modifiers);
gboolean _bdk_keymap_key_is_modifier   (BdkKeymap       *keymap,
					guint            keycode);

GC _bdk_x11_gc_flush (BdkGC *gc);

void _bdk_x11_initialize_locale (void);

void _bdk_xgrab_check_unmap        (BdkWindow *window,
				    gulong     serial);
void _bdk_xgrab_check_destroy      (BdkWindow *window);

gboolean _bdk_x11_display_is_root_window (BdkDisplay *display,
					  Window      xroot_window);

void _bdk_x11_precache_atoms (BdkDisplay          *display,
			      const gchar * const *atom_names,
			      gint                 n_atoms);

void _bdk_x11_events_init_screen   (BdkScreen *screen);
void _bdk_x11_events_uninit_screen (BdkScreen *screen);

void _bdk_events_init           (BdkDisplay *display);
void _bdk_events_uninit         (BdkDisplay *display);
void _bdk_windowing_window_init (BdkScreen *screen);
void _bdk_visual_init           (BdkScreen *screen);
void _bdk_dnd_init		(BdkDisplay *display);
void _bdk_windowing_image_init  (BdkDisplay *display);
void _bdk_input_init            (BdkDisplay *display);

BangoRenderer *_bdk_x11_renderer_get (BdkDrawable *drawable,
				      BdkGC       *gc);

void _bdk_x11_cursor_update_theme (BdkCursor *cursor);
void _bdk_x11_cursor_display_finalize (BdkDisplay *display);

gboolean _bdk_x11_get_xft_setting (BdkScreen   *screen,
				   const gchar *name,
				   GValue      *value);

extern BdkDrawableClass  _bdk_x11_drawable_class;
extern gboolean	         _bdk_use_xshm;
extern const int         _bdk_nenvent_masks;
extern const int         _bdk_event_mask_table[];
extern BdkAtom		 _bdk_selection_property;
extern gboolean          _bdk_synchronize;

#define BDK_PIXMAP_SCREEN(pix)	      (BDK_DRAWABLE_IMPL_X11 (((BdkPixmapObject *)pix)->impl)->screen)
#define BDK_PIXMAP_DISPLAY(pix)       (BDK_SCREEN_X11 (BDK_PIXMAP_SCREEN (pix))->display)
#define BDK_PIXMAP_XROOTWIN(pix)      (BDK_SCREEN_X11 (BDK_PIXMAP_SCREEN (pix))->xroot_window)
#define BDK_DRAWABLE_DISPLAY(win)     (BDK_IS_WINDOW (win) ? BDK_WINDOW_DISPLAY (win) : BDK_PIXMAP_DISPLAY (win))
#define BDK_DRAWABLE_SCREEN(win)      (BDK_IS_WINDOW (win) ? BDK_WINDOW_SCREEN (win) : BDK_PIXMAP_SCREEN (win))
#define BDK_DRAWABLE_XROOTWIN(win)    (BDK_IS_WINDOW (win) ? BDK_WINDOW_XROOTWIN (win) : BDK_PIXMAP_XROOTWIN (win))
#define BDK_SCREEN_DISPLAY(screen)    (BDK_SCREEN_X11 (screen)->display)
#define BDK_SCREEN_XROOTWIN(screen)   (BDK_SCREEN_X11 (screen)->xroot_window)
#define BDK_WINDOW_SCREEN(win)	      (BDK_DRAWABLE_IMPL_X11 (((BdkWindowObject *)win)->impl)->screen)
#define BDK_WINDOW_DISPLAY(win)       (BDK_SCREEN_X11 (BDK_WINDOW_SCREEN (win))->display)
#define BDK_WINDOW_XROOTWIN(win)      (BDK_SCREEN_X11 (BDK_WINDOW_SCREEN (win))->xroot_window)
#define BDK_GC_DISPLAY(gc)            (BDK_SCREEN_DISPLAY (BDK_GC_X11(gc)->screen))
#define BDK_WINDOW_IS_X11(win)        (BDK_IS_WINDOW_IMPL_X11 (((BdkWindowObject *)win)->impl))

#endif /* __BDK_PRIVATE_X11_H__ */
