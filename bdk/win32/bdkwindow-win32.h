/* BDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#ifndef __BDK_WINDOW_WIN32_H__
#define __BDK_WINDOW_WIN32_H__

#include <bdk/win32/bdkdrawable-win32.h>

B_BEGIN_DECLS

typedef struct _BdkWin32PositionInfo    BdkWin32PositionInfo;

#if 0
struct _BdkWin32PositionInfo
{
  gint x;
  gint y;
  gint width;
  gint height;
  gint x_offset;		/* Offsets to add to Win32 coordinates */
  gint y_offset;		/* within window to get BDK coodinates */
  guint big : 1;
  guint mapped : 1;
  guint no_bg : 1;	        /* Set when the window background
				 * is temporarily unset during resizing
				 * and scaling
				 */
  BdkRectangle clip_rect;	/* visible rectangle of window */
};
#endif


/* Window implementation for Win32
 */

typedef struct _BdkWindowImplWin32 BdkWindowImplWin32;
typedef struct _BdkWindowImplWin32Class BdkWindowImplWin32Class;

#define BDK_TYPE_WINDOW_IMPL_WIN32              (_bdk_window_impl_win32_get_type ())
#define BDK_WINDOW_IMPL_WIN32(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_WINDOW_IMPL_WIN32, BdkWindowImplWin32))
#define BDK_WINDOW_IMPL_WIN32_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_WINDOW_IMPL_WIN32, BdkWindowImplWin32Class))
#define BDK_IS_WINDOW_IMPL_WIN32(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_WINDOW_IMPL_WIN32))
#define BDK_IS_WINDOW_IMPL_WIN32_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_WINDOW_IMPL_WIN32))
#define BDK_WINDOW_IMPL_WIN32_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_WINDOW_IMPL_WIN32, BdkWindowImplWin32Class))

struct _BdkWindowImplWin32
{
  BdkDrawableImplWin32 parent_instance;

  gint8 toplevel_window_type;

  HCURSOR hcursor;
  HICON   hicon_big;
  HICON   hicon_small;

  /* When VK_PACKET sends us a leading surrogate, it's stashed here.
   * Later, when another VK_PACKET sends a tailing surrogate, we make up
   * a full unicode character from them, or discard the leading surrogate,
   * if the next key is not a tailing surrogate.
   */
  wchar_t leading_surrogate_keydown;
  wchar_t leading_surrogate_keyup;

  /* Window size hints */
  gint hint_flags;
  BdkGeometry hints;

  BdkEventMask native_event_mask;

  BdkWindowTypeHint type_hint;

  BdkEventMask extension_events_mask;

  BdkWindow *transient_owner;
  GSList    *transient_children;
  gint       num_transients;
  gboolean   changing_state;

  gint initial_x;
  gint initial_y;

  guint no_bg : 1;
  guint inhibit_configure : 1;
  guint override_redirect : 1;
};
 
struct _BdkWindowImplWin32Class 
{
  BdkDrawableImplWin32Class parent_class;
};

GType _bdk_window_impl_win32_get_type (void);

void  _bdk_win32_window_tmp_unset_bg  (BdkWindow *window,
				       gboolean   recurse);
void  _bdk_win32_window_tmp_reset_bg  (BdkWindow *window,
				       gboolean   recurse);

void  _bdk_win32_window_tmp_unset_parent_bg (BdkWindow *window);
void  _bdk_win32_window_tmp_reset_parent_bg (BdkWindow *window);

B_END_DECLS

#endif /* __BDK_WINDOW_WIN32_H__ */
