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

#ifndef __BDK_WIN32_H__
#define __BDK_WIN32_H__

#include <bdk/bdkprivate.h>

#ifndef STRICT
#define STRICT			/* We want strict type checks */
#endif
#include <windows.h>
#include <commctrl.h>

G_BEGIN_DECLS

#ifdef INSIDE_BDK_WIN32

#include "bdkprivate-win32.h"

#undef BDK_ROOT_PARENT /* internal access is direct */
#define BDK_ROOT_PARENT()             ((BdkWindow *) _bdk_parent_root)
#define BDK_WINDOW_HWND(win)          (BDK_DRAWABLE_IMPL_WIN32(((BdkWindowObject *)win)->impl)->handle)
#define BDK_PIXMAP_HBITMAP(pixmap)    (BDK_DRAWABLE_IMPL_WIN32(((BdkPixmapObject *)pixmap)->impl)->handle)
#define BDK_DRAWABLE_IMPL_WIN32_HANDLE(d) (((BdkDrawableImplWin32 *) d)->handle)
#define BDK_DRAWABLE_HANDLE(win)      (BDK_IS_WINDOW (win) ? BDK_WINDOW_HWND (win) : (BDK_IS_PIXMAP (win) ? BDK_PIXMAP_HBITMAP (win) : (BDK_IS_DRAWABLE_IMPL_WIN32 (win) ? BDK_DRAWABLE_IMPL_WIN32_HANDLE (win) : 0)))
#else
/* definition for exported 'internals' go here */
#define BDK_WINDOW_HWND(d) (bdk_win32_drawable_get_handle (d))

#endif

#define BDK_ROOT_WINDOW()             ((guint32) HWND_DESKTOP)
#define BDK_DISPLAY()                 NULL


/* These need to be here so btkstatusicon.c can pick them up if needed. */
#ifndef WM_XBUTTONDOWN
#define WM_XBUTTONDOWN 0x020B
#endif
#ifndef WM_XBUTTONUP
#define WM_XBUTTONUP 0x020C
#endif
#ifndef GET_XBUTTON_WPARAM
#define GET_XBUTTON_WPARAM(w) (HIWORD(w))
#endif
#ifndef XBUTTON1
#define XBUTTON1 1
#endif
#ifndef XBUTTON2
#define XBUTTON2 2
#endif


/* Return true if the BdkWindow is a win32 implemented window */
gboolean      bdk_win32_window_is_win32 (BdkWindow *window);
HWND          bdk_win32_window_get_impl_hwnd (BdkWindow *window);

/* Return the Bdk* for a particular HANDLE */
gpointer      bdk_win32_handle_table_lookup (BdkNativeWindow handle);

/* Translate from drawable to Windows handle */
HGDIOBJ       bdk_win32_drawable_get_handle (BdkDrawable *drawable);

/* Return a device context to draw in a drawable, given a BDK GC,
 * and a mask indicating which GC values might be used (for efficiency,
 * no need to muck around with text-related stuff if we aren't going
 * to output text, for instance).
 */
HDC           bdk_win32_hdc_get      (BdkDrawable    *drawable,
				      BdkGC          *gc,
				      BdkGCValuesMask usage);

/* Each HDC returned from bdk_win32_hdc_get must be released with
 * this function
 */
void          bdk_win32_hdc_release  (BdkDrawable    *drawable,
				      BdkGC          *gc,
				      BdkGCValuesMask usage);

void          bdk_win32_selection_add_targets (BdkWindow  *owner,
					       BdkAtom     selection,
					       gint	   n_targets,
					       BdkAtom    *targets);

/* For internal BTK use only */
BdkPixbuf    *bdk_win32_icon_to_pixbuf_libbtk_only (HICON hicon);
HICON         bdk_win32_pixbuf_to_hicon_libbtk_only (BdkPixbuf *pixbuf);
void          bdk_win32_set_modal_dialog_libbtk_only (HWND window);

BdkDrawable  *bdk_win32_begin_direct_draw_libbtk_only (BdkDrawable *drawable,
						       BdkGC *gc,
						       gpointer *priv_data,
						       gint *x_offset_out,
						       gint *y_offset_out);
void          bdk_win32_end_direct_draw_libbtk_only (gpointer priv_data);

BdkWindow *   bdk_win32_window_foreign_new_for_display (BdkDisplay *display,
                                                        BdkNativeWindow anid);
BdkWindow *   bdk_win32_window_lookup_for_display (BdkDisplay *display,
                                                   BdkNativeWindow anid);


G_END_DECLS

#endif /* __BDK_WIN32_H__ */
