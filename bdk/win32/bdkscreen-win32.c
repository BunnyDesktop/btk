/* BDK - The GIMP Drawing Kit
 * Copyright (C) 2002 Hans Breuer
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
#include "bdkprivate-win32.h"

static BdkColormap *default_colormap = NULL;

BdkDisplay *
bdk_screen_get_display (BdkScreen *screen)
{
  return _bdk_display;
}

BdkWindow *
bdk_screen_get_root_window (BdkScreen *screen)
{
  return _bdk_root;
}

BdkColormap *
bdk_screen_get_default_colormap (BdkScreen *screen)
{
  return default_colormap;
}

void
bdk_screen_set_default_colormap (BdkScreen   *screen,
				 BdkColormap *colormap)
{
  BdkColormap *old_colormap;
  
  g_return_if_fail (screen == _bdk_screen);
  g_return_if_fail (BDK_IS_COLORMAP (colormap));

  old_colormap = default_colormap;

  default_colormap = g_object_ref (colormap);
  
  if (old_colormap)
    g_object_unref (old_colormap);
}

gint
bdk_screen_get_n_monitors (BdkScreen *screen)
{
  g_return_val_if_fail (screen == _bdk_screen, 0);

  return _bdk_num_monitors;
}

gint
bdk_screen_get_primary_monitor (BdkScreen *screen)
{
  g_return_val_if_fail (screen == _bdk_screen, 0);

  return 0;
}

gint
bdk_screen_get_monitor_width_mm (BdkScreen *screen,
                                 gint       num_monitor)
{
  g_return_val_if_fail (screen == _bdk_screen, 0);
  g_return_val_if_fail (num_monitor < _bdk_num_monitors, 0);
  g_return_val_if_fail (num_monitor >= 0, 0);

  return _bdk_monitors[num_monitor].width_mm;
}

gint
bdk_screen_get_monitor_height_mm (BdkScreen *screen,
                                  gint       num_monitor)
{
  g_return_val_if_fail (screen == _bdk_screen, 0);
  g_return_val_if_fail (num_monitor < _bdk_num_monitors, 0);
  g_return_val_if_fail (num_monitor >= 0, 0);

  return _bdk_monitors[num_monitor].height_mm;
}

gchar *
bdk_screen_get_monitor_plug_name (BdkScreen *screen,
                                  gint       num_monitor)
{
  g_return_val_if_fail (screen == _bdk_screen, 0);
  g_return_val_if_fail (num_monitor < _bdk_num_monitors, 0);
  g_return_val_if_fail (num_monitor >= 0, 0);

  return g_strdup (_bdk_monitors[num_monitor].name);
}

void
bdk_screen_get_monitor_geometry (BdkScreen    *screen, 
				 gint          num_monitor,
				 BdkRectangle *dest)
{
  g_return_if_fail (screen == _bdk_screen);
  g_return_if_fail (num_monitor < _bdk_num_monitors);
  g_return_if_fail (num_monitor >= 0);

  *dest = _bdk_monitors[num_monitor].rect;
}

BdkColormap *
bdk_screen_get_rgba_colormap (BdkScreen *screen)
{
  g_return_val_if_fail (screen == _bdk_screen, NULL);

  return NULL;
}
  
BdkVisual *
bdk_screen_get_rgba_visual (BdkScreen *screen)
{
  g_return_val_if_fail (screen == _bdk_screen, NULL);

  return NULL;
}
  
gint
bdk_screen_get_number (BdkScreen *screen)
{
  g_return_val_if_fail (screen == _bdk_screen, 0);  
  
  return 0;
}

gchar * 
_bdk_windowing_substitute_screen_number (const gchar *display_name,
					 int          screen_number)
{
  if (screen_number != 0)
    return NULL;

  return g_strdup (display_name);
}

gchar *
bdk_screen_make_display_name (BdkScreen *screen)
{
  return g_strdup (bdk_display_get_name (_bdk_display));
}

BdkWindow *
bdk_screen_get_active_window (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);

  return NULL;
}

GList *
bdk_screen_get_window_stack (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);

  return NULL;
}

gboolean
bdk_screen_is_composited (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), FALSE);

  return FALSE;
}
