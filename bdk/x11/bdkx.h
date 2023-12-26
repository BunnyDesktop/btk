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

#ifndef __BDK_X_H__
#define __BDK_X_H__

#include <bdk/bdkprivate.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

G_BEGIN_DECLS

#if (!defined (BDK_DISABLE_DEPRECATED) && !defined (BDK_MULTIHEAD_SAFE)) || defined (BDK_COMPILATION)
extern Display          *bdk_display;
#endif

Display *bdk_x11_drawable_get_xdisplay    (BdkDrawable *drawable);
XID      bdk_x11_drawable_get_xid         (BdkDrawable *drawable);
BdkDrawable *bdk_x11_window_get_drawable_impl (BdkWindow *window);
BdkDrawable *bdk_x11_pixmap_get_drawable_impl (BdkPixmap *pixmap);
Display *bdk_x11_image_get_xdisplay       (BdkImage    *image);
XImage  *bdk_x11_image_get_ximage         (BdkImage    *image);
Display *bdk_x11_colormap_get_xdisplay    (BdkColormap *colormap);
Colormap bdk_x11_colormap_get_xcolormap   (BdkColormap *colormap);
Display *bdk_x11_cursor_get_xdisplay      (BdkCursor   *cursor);
Cursor   bdk_x11_cursor_get_xcursor       (BdkCursor   *cursor);
Display *bdk_x11_display_get_xdisplay     (BdkDisplay  *display);
Visual * bdk_x11_visual_get_xvisual       (BdkVisual   *visual);
#if !defined(BDK_DISABLE_DEPRECATED) || defined(BDK_COMPILATION)
Display *bdk_x11_gc_get_xdisplay          (BdkGC       *gc);
GC       bdk_x11_gc_get_xgc               (BdkGC       *gc);
#endif
Screen * bdk_x11_screen_get_xscreen       (BdkScreen   *screen);
int      bdk_x11_screen_get_screen_number (BdkScreen   *screen);
void     bdk_x11_window_set_user_time     (BdkWindow   *window,
					   guint32      timestamp);
void     bdk_x11_window_move_to_current_desktop (BdkWindow   *window);

const char* bdk_x11_screen_get_window_manager_name (BdkScreen *screen);

#ifndef BDK_MULTIHEAD_SAFE
Window   bdk_x11_get_default_root_xwindow (void);
Display *bdk_x11_get_default_xdisplay     (void);
gint     bdk_x11_get_default_screen       (void);
#endif

#define BDK_COLORMAP_XDISPLAY(cmap)   (bdk_x11_colormap_get_xdisplay (cmap))
#define BDK_COLORMAP_XCOLORMAP(cmap)  (bdk_x11_colormap_get_xcolormap (cmap))
#define BDK_CURSOR_XDISPLAY(cursor)   (bdk_x11_cursor_get_xdisplay (cursor))
#define BDK_CURSOR_XCURSOR(cursor)    (bdk_x11_cursor_get_xcursor (cursor))
#define BDK_IMAGE_XDISPLAY(image)     (bdk_x11_image_get_xdisplay (image))
#define BDK_IMAGE_XIMAGE(image)       (bdk_x11_image_get_ximage (image))

#if (!defined (BDK_DISABLE_DEPRECATED) && !defined (BDK_MULTIHEAD_SAFE)) || defined (BDK_COMPILATION)
#define BDK_DISPLAY()                 bdk_display
#endif

#ifdef BDK_COMPILATION

#include "bdkprivate-x11.h"
#include "bdkscreen-x11.h"

#define BDK_DISPLAY_XDISPLAY(display) (BDK_DISPLAY_X11(display)->xdisplay)

#define BDK_WINDOW_XDISPLAY(win)      (BDK_SCREEN_X11 (BDK_WINDOW_SCREEN (win))->xdisplay)
#define BDK_WINDOW_XID(win)           (BDK_DRAWABLE_IMPL_X11(((BdkWindowObject *)win)->impl)->xid)
#define BDK_PIXMAP_XDISPLAY(pix)      (BDK_SCREEN_X11 (BDK_PIXMAP_SCREEN (pix))->xdisplay)
#define BDK_PIXMAP_XID(pix)           (BDK_DRAWABLE_IMPL_X11(((BdkPixmapObject *)pix)->impl)->xid)
#define BDK_DRAWABLE_XDISPLAY(win)    (BDK_IS_WINDOW (win) ? BDK_WINDOW_XDISPLAY (win) : BDK_PIXMAP_XDISPLAY (win))
#define BDK_DRAWABLE_XID(win)         (BDK_IS_WINDOW (win) ? BDK_WINDOW_XID (win) : BDK_PIXMAP_XID (win))
#define BDK_GC_XDISPLAY(gc)           (BDK_SCREEN_XDISPLAY(BDK_GC_X11(gc)->screen))
#define BDK_GC_XGC(gc)		      (BDK_GC_X11(gc)->xgc)
#define BDK_SCREEN_XDISPLAY(screen)   (BDK_SCREEN_X11 (screen)->xdisplay)
#define BDK_SCREEN_XSCREEN(screen)    (BDK_SCREEN_X11 (screen)->xscreen)
#define BDK_SCREEN_XNUMBER(screen)    (BDK_SCREEN_X11 (screen)->screen_num) 
#define BDK_VISUAL_XVISUAL(vis)       (((BdkVisualPrivate *) vis)->xvisual)
#define BDK_GC_GET_XGC(gc)	      (BDK_GC_X11(gc)->dirty_mask ? _bdk_x11_gc_flush (gc) : ((BdkGCX11 *)(gc))->xgc)
#define BDK_WINDOW_XWINDOW	      BDK_DRAWABLE_XID

#else /* BDK_COMPILATION */

#ifndef BDK_MULTIHEAD_SAFE
#define BDK_ROOT_WINDOW()             (bdk_x11_get_default_root_xwindow ())
#endif

#define BDK_DISPLAY_XDISPLAY(display) (bdk_x11_display_get_xdisplay (display))

#define BDK_WINDOW_XDISPLAY(win)      (bdk_x11_drawable_get_xdisplay (bdk_x11_window_get_drawable_impl (win)))
#define BDK_WINDOW_XID(win)           (bdk_x11_drawable_get_xid (win))
#define BDK_WINDOW_XWINDOW(win)       (bdk_x11_drawable_get_xid (win))
#define BDK_PIXMAP_XDISPLAY(win)      (bdk_x11_drawable_get_xdisplay (bdk_x11_pixmap_get_drawable_impl (win)))
#define BDK_PIXMAP_XID(win)           (bdk_x11_drawable_get_xid (win))
#define BDK_DRAWABLE_XDISPLAY(win)    (bdk_x11_drawable_get_xdisplay (win))
#define BDK_DRAWABLE_XID(win)         (bdk_x11_drawable_get_xid (win))
#define BDK_GC_XDISPLAY(gc)           (bdk_x11_gc_get_xdisplay (gc))
#define BDK_GC_XGC(gc)                (bdk_x11_gc_get_xgc (gc))
#define BDK_SCREEN_XDISPLAY(screen)   (bdk_x11_display_get_xdisplay (bdk_screen_get_display (screen)))
#define BDK_SCREEN_XSCREEN(screen)    (bdk_x11_screen_get_xscreen (screen))
#define BDK_SCREEN_XNUMBER(screen)    (bdk_x11_screen_get_screen_number (screen))
#define BDK_VISUAL_XVISUAL(visual)    (bdk_x11_visual_get_xvisual (visual))

#endif /* BDK_COMPILATION */

BdkVisual* bdk_x11_screen_lookup_visual (BdkScreen *screen,
					 VisualID   xvisualid);
#ifndef BDK_DISABLE_DEPRECATED
#ifndef BDK_MULTIHEAD_SAFE
BdkVisual* bdkx_visual_get            (VisualID   xvisualid);
#endif
#endif

#ifdef BDK_ENABLE_BROKEN
/* XXX: An X Colormap is useless unless we also have the visual. */
BdkColormap* bdkx_colormap_get (Colormap xcolormap);
#endif

BdkColormap *bdk_x11_colormap_foreign_new (BdkVisual *visual,
					   Colormap   xcolormap);

#if !defined (BDK_DISABLE_DEPRECATED) || defined (BDK_COMPILATION)
gpointer      bdk_xid_table_lookup_for_display (BdkDisplay *display,
						XID         xid);
#endif
guint32       bdk_x11_get_server_time  (BdkWindow       *window);
guint32       bdk_x11_display_get_user_time (BdkDisplay *display);

const gchar * bdk_x11_display_get_startup_notification_id (BdkDisplay *display);

void          bdk_x11_display_set_cursor_theme (BdkDisplay  *display,
						const gchar *theme,
						const gint   size);

void bdk_x11_display_broadcast_startup_message (BdkDisplay *display,
						const char *message_type,
						...) G_GNUC_NULL_TERMINATED;

/* returns TRUE if we support the given WM spec feature */
gboolean bdk_x11_screen_supports_net_wm_hint (BdkScreen *screen,
					      BdkAtom    property);

XID      bdk_x11_screen_get_monitor_output   (BdkScreen *screen,
                                              gint       monitor_num);

#ifndef BDK_MULTIHEAD_SAFE
#ifndef BDK_DISABLE_DEPRECATED
gpointer      bdk_xid_table_lookup   (XID              xid);
gboolean      bdk_net_wm_supports    (BdkAtom    property);
#endif
void          bdk_x11_grab_server    (void);
void          bdk_x11_ungrab_server  (void);
#endif

BdkDisplay   *bdk_x11_lookup_xdisplay (Display *xdisplay);


/* Functions to get the X Atom equivalent to the BdkAtom */
Atom	              bdk_x11_atom_to_xatom_for_display (BdkDisplay  *display,
							 BdkAtom      atom);
BdkAtom		      bdk_x11_xatom_to_atom_for_display (BdkDisplay  *display,
							 Atom	      xatom);
Atom		      bdk_x11_get_xatom_by_name_for_display (BdkDisplay  *display,
							     const gchar *atom_name);
const gchar *         bdk_x11_get_xatom_name_for_display (BdkDisplay  *display,
							  Atom         xatom);
#ifndef BDK_MULTIHEAD_SAFE
Atom                  bdk_x11_atom_to_xatom     (BdkAtom      atom);
BdkAtom               bdk_x11_xatom_to_atom     (Atom         xatom);
Atom                  bdk_x11_get_xatom_by_name (const gchar *atom_name);
const gchar *         bdk_x11_get_xatom_name    (Atom         xatom);
#endif

void	    bdk_x11_display_grab	      (BdkDisplay *display);
void	    bdk_x11_display_ungrab	      (BdkDisplay *display);
void        bdk_x11_register_standard_event_type (BdkDisplay *display,
						  gint        event_base,
						  gint        n_events);

#if !defined(BDK_DISABLE_DEPRECATED) || defined(BDK_COMPILATION)

gpointer             bdk_x11_font_get_xfont    (BdkFont *font);
#define BDK_FONT_XFONT(font)          (bdk_x11_font_get_xfont (font))

#define bdk_font_lookup_for_display(display, xid) ((BdkFont*) bdk_xid_table_lookup_for_display (display, ((xid)|XID_FONT_BIT)))

#endif /* !BDK_DISABLE_DEPRECATED || BDK_COMPILATION */

#ifndef BDK_DISABLE_DEPRECATED

Display *            bdk_x11_font_get_xdisplay (BdkFont *font);
const char *         bdk_x11_font_get_name     (BdkFont *font);

#define BDK_FONT_XDISPLAY(font)       (bdk_x11_font_get_xdisplay (font))

#ifndef BDK_MULTIHEAD_SAFE

#define bdk_font_lookup(xid)	   ((BdkFont*) bdk_xid_table_lookup (xid))

#endif /* BDK_MULTIHEAD_SAFE */
#endif /* BDK_DISABLE_DEPRECATED */

void        bdk_x11_set_sm_client_id (const gchar *sm_client_id);

BdkWindow  *bdk_x11_window_foreign_new_for_display (BdkDisplay *display,
                                                    Window      window);
BdkWindow  *bdk_x11_window_lookup_for_display      (BdkDisplay *display,
                                                    Window      window);

gint     bdk_x11_display_text_property_to_text_list (BdkDisplay   *display,
                                                     BdkAtom       encoding,
                                                     gint          format,
                                                     const guchar *text,
                                                     gint          length,
                                                     gchar      ***list);
void     bdk_x11_free_text_list                     (gchar       **list);
gint     bdk_x11_display_string_to_compound_text    (BdkDisplay   *display,
                                                     const gchar  *str,
                                                     BdkAtom      *encoding,
                                                     gint         *format,
                                                     guchar      **ctext,
                                                     gint         *length);
gboolean bdk_x11_display_utf8_to_compound_text      (BdkDisplay   *display,
                                                     const gchar  *str,
                                                     BdkAtom      *encoding,
                                                     gint         *format,
                                                     guchar      **ctext,
                                                     gint         *length);
void     bdk_x11_free_compound_text                 (guchar       *ctext);


G_END_DECLS

#endif /* __BDK_X_H__ */
