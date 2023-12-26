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

/* bdkgeometry-win32.c: emulation of 32 bit coordinates within the
 * limits of Win32 GDI. The idea of big window emulation is more or less
 * a copy of the X11 version, and the equvalent of guffaw scrolling
 * is ScrollWindowEx(). While we determine the invalidated rebunnyion
 * ourself during scrolling, we do not pass SW_INVALIDATE to
 * ScrollWindowEx() to avoid a unnecessary WM_PAINT.
 *
 * Bits are always scrolled correctly by ScrollWindowEx(), but
 * some big children may hit the coordinate boundary (i.e.
 * win32_x/win32_y < -16383) after scrolling. They needed to be moved
 * back to the real position determined by bdk_window_compute_position().
 * This is handled in bdk_window_postmove().
 * 
 * The X11 version by Owen Taylor <otaylor@redhat.com>
 * Copyright Red Hat, Inc. 2000
 * Win32 hack by Tor Lillqvist <tml@iki.fi>
 * and Hans Breuer <hans@breuer.org>
 * Modified by Ivan, Wong Yat Cheung <email@ivanwong.info>
 * so that big window emulation finally works.
 */

#include "config.h"
#include "bdk.h"		/* For bdk_rectangle_intersect */
#include "bdkrebunnyion.h"
#include "bdkrebunnyion-generic.h"
#include "bdkinternals.h"
#include "bdkprivate-win32.h"

#define SIZE_LIMIT 32767

typedef struct _BdkWindowParentPos BdkWindowParentPos;

static void tmp_unset_bg (BdkWindow *window);
static void tmp_reset_bg (BdkWindow *window);

void
_bdk_window_move_resize_child (BdkWindow *window,
			       gint       x,
			       gint       y,
			       gint       width,
			       gint       height)
{
  BdkWindowImplWin32 *impl;
  BdkWindowObject *obj;

  g_return_if_fail (window != NULL);
  g_return_if_fail (BDK_IS_WINDOW (window));

  obj = BDK_WINDOW_OBJECT (window);
  impl = BDK_WINDOW_IMPL_WIN32 (obj->impl);

  BDK_NOTE (MISC, g_print ("_bdk_window_move_resize_child: %s@%+d%+d %dx%d@%+d%+d\n",
			   _bdk_win32_drawable_description (window),
			   obj->x, obj->y, width, height, x, y));

  if (width > 65535 || height > 65535)
  {
    g_warning ("Native children wider or taller than 65535 pixels are not supported.");

    if (width > 65535)
      width = 65535;
    if (height > 65535)
      height = 65535;
  }

  obj->x = x;
  obj->y = y;
  obj->width = width;
  obj->height = height;

  _bdk_win32_window_tmp_unset_parent_bg (window);
  _bdk_win32_window_tmp_unset_bg (window, TRUE);
  
  BDK_NOTE (MISC, g_print ("... SetWindowPos(%p,NULL,%d,%d,%d,%d,"
			   "NOACTIVATE|NOZORDER)\n",
			   BDK_WINDOW_HWND (window),
			   obj->x + obj->parent->abs_x, obj->y + obj->parent->abs_y, 
			   width, height));

  API_CALL (SetWindowPos, (BDK_WINDOW_HWND (window), NULL,
			   obj->x + obj->parent->abs_x, obj->y + obj->parent->abs_y, 
			   width, height,
			   SWP_NOACTIVATE | SWP_NOZORDER));

  //_bdk_win32_window_tmp_reset_parent_bg (window);
  _bdk_win32_window_tmp_reset_bg (window, TRUE);
}

void
_bdk_win32_window_tmp_unset_bg (BdkWindow *window,
				gboolean recurse)
{
  BdkWindowObject *private;

  g_return_if_fail (BDK_IS_WINDOW (window));

  private = (BdkWindowObject *)window;

  if (private->input_only || private->destroyed ||
      (private->window_type != BDK_WINDOW_ROOT &&
       !BDK_WINDOW_IS_MAPPED (window)))
    return;

  if (_bdk_window_has_impl (window) &&
      BDK_WINDOW_IS_WIN32 (window) &&
      private->window_type != BDK_WINDOW_ROOT &&
      private->window_type != BDK_WINDOW_FOREIGN)
    tmp_unset_bg (window);

  if (recurse)
    {
      GList *l;

      for (l = private->children; l != NULL; l = l->next)
	_bdk_win32_window_tmp_unset_bg (l->data, TRUE);
    }
}

static void
tmp_unset_bg (BdkWindow *window)
{
  BdkWindowImplWin32 *impl;
  BdkWindowObject *obj;

  obj = (BdkWindowObject *) window;
  impl = BDK_WINDOW_IMPL_WIN32 (obj->impl);

  impl->no_bg = TRUE;

  /*
   * The X version sets background = None to avoid updateing for a moment.
   * Not sure if this could really emulate it.
   */
  if (obj->bg_pixmap != BDK_NO_BG)
    {
      ///* handled in WM_ERASEBKGRND proceesing */;

      //HDC hdc = GetDC (BDK_WINDOW_HWND (window));
      //erase_background (window, hdc);
    }
}

static void
tmp_reset_bg (BdkWindow *window)
{
  BdkWindowObject *obj;
  BdkWindowImplWin32 *impl;

  obj = BDK_WINDOW_OBJECT (window);
  impl = BDK_WINDOW_IMPL_WIN32 (obj->impl);

  impl->no_bg = FALSE;
}

void
_bdk_win32_window_tmp_unset_parent_bg (BdkWindow *window)
{
  BdkWindowObject *private = (BdkWindowObject*)window;

  if (BDK_WINDOW_TYPE (private->parent) == BDK_WINDOW_ROOT)
    return;

  window = _bdk_window_get_impl_window ((BdkWindow*)private->parent);
  _bdk_win32_window_tmp_unset_bg (window, FALSE);
}

void
_bdk_win32_window_tmp_reset_bg (BdkWindow *window,
				gboolean   recurse)
{
  BdkWindowObject *private = (BdkWindowObject*)window;

  g_return_if_fail (BDK_IS_WINDOW (window));

  if (private->input_only || private->destroyed ||
      (private->window_type != BDK_WINDOW_ROOT && !BDK_WINDOW_IS_MAPPED (window)))
    return;

  if (_bdk_window_has_impl (window) &&
      BDK_WINDOW_IS_WIN32 (window) &&
      private->window_type != BDK_WINDOW_ROOT &&
      private->window_type != BDK_WINDOW_FOREIGN)
    {
      tmp_reset_bg (window);
    }

  if (recurse)
    {
      GList *l;

      for (l = private->children; l != NULL; l = l->next)
	_bdk_win32_window_tmp_reset_bg (l->data, TRUE);
    }
}

/*
void
_bdk_win32_window_tmp_reset_bg (BdkWindow *window)
{
  BdkWindowImplWin32 *impl;
  BdkWindowObject *obj;

  obj = (BdkWindowObject *) window;
  impl = BDK_WINDOW_IMPL_WIN32 (obj->impl);

  impl->no_bg = FALSE;
}
*/

#if 0
static BdkRebunnyion *
bdk_window_clip_changed (BdkWindow    *window,
			 BdkRectangle *old_clip,
			 BdkRectangle *new_clip)
{
  BdkWindowImplWin32 *impl;
  BdkWindowObject *obj;
  BdkRebunnyion *old_clip_rebunnyion;
  BdkRebunnyion *new_clip_rebunnyion;
  
  if (((BdkWindowObject *)window)->input_only)
    return NULL;

  obj = (BdkWindowObject *) window;
  impl = BDK_WINDOW_IMPL_WIN32 (obj->impl);
  
  old_clip_rebunnyion = bdk_rebunnyion_rectangle (old_clip);
  new_clip_rebunnyion = bdk_rebunnyion_rectangle (new_clip);

  /* Trim invalid rebunnyion of window to new clip rectangle
   */
  if (obj->update_area)
    bdk_rebunnyion_intersect (obj->update_area, new_clip_rebunnyion);

  /* Invalidate newly exposed portion of window
   */
  bdk_rebunnyion_subtract (new_clip_rebunnyion, old_clip_rebunnyion);
  if (!bdk_rebunnyion_empty (new_clip_rebunnyion))
    bdk_window_tmp_unset_bg (window);
  else
    {
      bdk_rebunnyion_destroy (new_clip_rebunnyion);
      new_clip_rebunnyion = NULL;
    }

  bdk_rebunnyion_destroy (old_clip_rebunnyion);

  return new_clip_rebunnyion;
}
#endif

#if 0
static void
bdk_window_post_scroll (BdkWindow    *window,
			BdkRebunnyion    *new_clip_rebunnyion)
{
  BDK_NOTE (EVENTS,
	    g_print ("bdk_window_clip_changed: invalidating rebunnyion: %s\n",
		     _bdk_win32_bdkrebunnyion_to_string (new_clip_rebunnyion)));

  bdk_window_invalidate_rebunnyion (window, new_clip_rebunnyion, FALSE);
  g_print ("bdk_window_post_scroll\n");
  bdk_rebunnyion_destroy (new_clip_rebunnyion);
}

#endif
