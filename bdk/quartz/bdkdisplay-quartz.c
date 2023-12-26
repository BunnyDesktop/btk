/* bdkdisplay-quartz.c
 *
 * Copyright (C) 2005 Imendio AB
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

#include "bdk.h"
#include "bdkprivate-quartz.h"
#include "bdkscreen-quartz.h"

BdkWindow *
bdk_display_get_default_group (BdkDisplay *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);

  /* FIXME: Implement */

  return NULL;
}

void
_bdk_windowing_set_default_display (BdkDisplay *display)
{
  g_assert (display == NULL || _bdk_display == display);
}

BdkDisplay *
bdk_display_open (const bchar *display_name)
{
  if (_bdk_display != NULL)
    return NULL;

  /* Initialize application */
  [NSApplication sharedApplication];

  _bdk_display = g_object_new (BDK_TYPE_DISPLAY, NULL);

  _bdk_visual_init ();

  _bdk_screen = _bdk_screen_quartz_new ();

  _bdk_windowing_window_init ();

  _bdk_events_init ();
  _bdk_input_init ();

#if 0
  /* FIXME: Remove the #if 0 when we have these functions */
  _bdk_dnd_init ();
#endif

  g_signal_emit_by_name (bdk_display_manager_get (),
			 "display_opened", _bdk_display);

  return _bdk_display;
}

const bchar *
bdk_display_get_name (BdkDisplay *display)
{
  static bchar *display_name = NULL;

  if (!display_name)
    {
      BDK_QUARTZ_ALLOC_POOL;
      display_name = g_strdup ([[[NSHost currentHost] name] UTF8String]);
      BDK_QUARTZ_RELEASE_POOL;
    }

  return display_name;
}

int
bdk_display_get_n_screens (BdkDisplay *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), 0);

  return 1;
}

BdkScreen *
bdk_display_get_screen (BdkDisplay *display,
			bint        screen_num)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (screen_num == 0, NULL);

  return _bdk_screen;
}

BdkScreen *
bdk_display_get_default_screen (BdkDisplay *display)
{
  return _bdk_screen;
}

void
bdk_display_beep (BdkDisplay *display)
{
  g_return_if_fail (BDK_IS_DISPLAY (display));

  NSBeep();
}

bboolean 
bdk_display_supports_selection_notification (BdkDisplay *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), FALSE);

  /* FIXME: Implement */
  return FALSE;
}

bboolean 
bdk_display_request_selection_notification (BdkDisplay *display,
                                            BdkAtom     selection)

{
  /* FIXME: Implement */
  return FALSE;
}

bboolean
bdk_display_supports_clipboard_persistence (BdkDisplay *display)
{
  /* FIXME: Implement */
  return FALSE;
}

bboolean 
bdk_display_supports_shapes (BdkDisplay *display)
{
  /* FIXME: Implement */
  return FALSE;
}

bboolean 
bdk_display_supports_input_shapes (BdkDisplay *display)
{
  /* FIXME: Implement */
  return FALSE;
}

void
bdk_display_store_clipboard (BdkDisplay    *display,
			     BdkWindow     *clipboard_window,
			     buint32        time_,
			     const BdkAtom *targets,
			     bint           n_targets)
{
  /* FIXME: Implement */
}


bboolean
bdk_display_supports_composite (BdkDisplay *display)
{
  /* FIXME: Implement */
  return FALSE;
}

bulong
_bdk_windowing_window_get_next_serial (BdkDisplay *display)
{
  return 0;
}
