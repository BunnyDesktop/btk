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

#ifndef __BDK_WINDOW_X11_H__
#define __BDK_WINDOW_X11_H__

#include <bdk/x11/bdkdrawable-x11.h>

#ifdef HAVE_XDAMAGE
#include <X11/extensions/Xdamage.h>
#endif

#ifdef HAVE_XSYNC
#include <X11/extensions/sync.h>
#endif

B_BEGIN_DECLS

typedef struct _BdkToplevelX11 BdkToplevelX11;
typedef struct _BdkWindowImplX11 BdkWindowImplX11;
typedef struct _BdkWindowImplX11Class BdkWindowImplX11Class;
typedef struct _BdkXPositionInfo BdkXPositionInfo;

/* Window implementation for X11
 */

#define BDK_TYPE_WINDOW_IMPL_X11              (bdk_window_impl_x11_get_type ())
#define BDK_WINDOW_IMPL_X11(object)           (B_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_WINDOW_IMPL_X11, BdkWindowImplX11))
#define BDK_WINDOW_IMPL_X11_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_WINDOW_IMPL_X11, BdkWindowImplX11Class))
#define BDK_IS_WINDOW_IMPL_X11(object)        (B_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_WINDOW_IMPL_X11))
#define BDK_IS_WINDOW_IMPL_X11_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_WINDOW_IMPL_X11))
#define BDK_WINDOW_IMPL_X11_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_WINDOW_IMPL_X11, BdkWindowImplX11Class))

struct _BdkWindowImplX11
{
  BdkDrawableImplX11 parent_instance;

  BdkToplevelX11 *toplevel;	/* Toplevel-specific information */
  BdkCursor *cursor;
  bint8 toplevel_window_type;
  buint no_bg : 1;	        /* Set when the window background is temporarily
				 * unset during resizing and scaling */
  buint override_redirect : 1;
  buint use_synchronized_configure : 1;

#if defined (HAVE_XCOMPOSITE) && defined(HAVE_XDAMAGE) && defined (HAVE_XFIXES)
  Damage damage;
#endif
};
 
struct _BdkWindowImplX11Class 
{
  BdkDrawableImplX11Class parent_class;
};

struct _BdkToplevelX11
{

  /* Set if the window, or any descendent of it, is the server's focus window
   */
  buint has_focus_window : 1;

  /* Set if window->has_focus_window and the focus isn't grabbed elsewhere.
   */
  buint has_focus : 1;

  /* Set if the pointer is inside this window. (This is needed for
   * for focus tracking)
   */
  buint has_pointer : 1;
  
  /* Set if the window is a descendent of the focus window and the pointer is
   * inside it. (This is the case where the window will receive keystroke
   * events even window->has_focus_window is FALSE)
   */
  buint has_pointer_focus : 1;

  /* Set if we are requesting these hints */
  buint skip_taskbar_hint : 1;
  buint skip_pager_hint : 1;
  buint urgency_hint : 1;

  buint on_all_desktops : 1;   /* _NET_WM_STICKY == 0xFFFFFFFF */

  buint have_sticky : 1;	/* _NET_WM_STATE_STICKY */
  buint have_maxvert : 1;       /* _NET_WM_STATE_MAXIMIZED_VERT */
  buint have_maxhorz : 1;       /* _NET_WM_STATE_MAXIMIZED_HORZ */
  buint have_fullscreen : 1;    /* _NET_WM_STATE_FULLSCREEN */
  buint have_hidden : 1;	/* _NET_WM_STATE_HIDDEN */

  buint is_leader : 1;
  
  bulong map_serial;	/* Serial of last transition from unmapped */
  
  BdkPixmap *icon_pixmap;
  BdkPixmap *icon_mask;
  BdkPixmap *icon_window;
  BdkWindow *group_leader;

  /* Time of most recent user interaction. */
  bulong user_time;

  /* We use an extra X window for toplevel windows that we XSetInputFocus()
   * to in order to avoid getting keyboard events redirected to subwindows
   * that might not even be part of this app
   */
  Window focus_window;
 
#ifdef HAVE_XSYNC
  XID update_counter;
  XSyncValue pending_counter_value; /* latest _NET_WM_SYNC_REQUEST value received */
  XSyncValue current_counter_value; /* Latest _NET_WM_SYNC_REQUEST value received
				     * where we have also seen the corresponding
				     * ConfigureNotify
				     */
#endif
};

GType bdk_window_impl_x11_get_type (void);

void            bdk_x11_window_set_user_time        (BdkWindow *window,
						     buint32    timestamp);

BdkToplevelX11 *_bdk_x11_window_get_toplevel        (BdkWindow *window);
void            _bdk_x11_window_tmp_unset_bg        (BdkWindow *window,
						     bboolean   recurse);
void            _bdk_x11_window_tmp_reset_bg        (BdkWindow *window,
						     bboolean   recurse);
void            _bdk_x11_window_tmp_unset_parent_bg (BdkWindow *window);
void            _bdk_x11_window_tmp_reset_parent_bg (BdkWindow *window);

BdkCursor      *_bdk_x11_window_get_cursor    (BdkWindow *window);
void            _bdk_x11_window_get_offsets   (BdkWindow *window,
                                               bint      *x_offset,
                                               bint      *y_offset);

B_END_DECLS

#endif /* __BDK_WINDOW_X11_H__ */
