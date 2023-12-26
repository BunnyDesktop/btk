/* BDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998-1999 Tor Lillqvist
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
 * file for a list of people on the BTK+ Team.
 */

/*
 * BTK+ DirectFB backend
 * Copyright (C) 2001-2002  convergence integrated media GmbH
 * Copyright (C) 2002-2004  convergence GmbH
 * Written by Denis Oliver Kropp <dok@convergence.de> and
 *            Sven Neumann <sven@convergence.de>
 */

#include "config.h"
#include "bdk.h"
#include "bdkscreen.h"
#include "bdkdirectfb.h"
#include "bdkprivate-directfb.h"

#include "bdkinternals.h"
#include "bdkalias.h"


static BdkColormap *default_colormap = NULL;

BdkDisplay *
bdk_screen_get_display (BdkScreen *screen)
{
  return BDK_DISPLAY_OBJECT (_bdk_display);
}

BdkWindow *
bdk_screen_get_root_window (BdkScreen *screen)
{
  return _bdk_parent_root;
}

BdkColormap*
bdk_screen_get_default_colormap (BdkScreen *screen)
{
  return default_colormap;
}

void
bdk_screen_set_default_colormap (BdkScreen   *screen,
				 BdkColormap *colormap)
{
  BdkColormap *old_colormap;

  g_return_if_fail (BDK_IS_SCREEN (screen));
  g_return_if_fail (BDK_IS_COLORMAP (colormap));

  old_colormap = default_colormap;

  default_colormap = g_object_ref (colormap);

  if (old_colormap)
    g_object_unref (old_colormap);
}

gint
bdk_screen_get_n_monitors (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), 0);

  return 1;
}

gint
bdk_screen_get_primary_monitor (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), 0);

  return 0;
}

void
bdk_screen_get_monitor_geometry (BdkScreen    *screen,
				 gint          num_monitor,
				 BdkRectangle *dest)
{
  g_return_if_fail (BDK_IS_SCREEN (screen));
  g_return_if_fail (dest != NULL);

  dest->x      = 0;
  dest->y      = 0;
  dest->width  = bdk_screen_width ();
  dest->height = bdk_screen_height ();
}

gint
bdk_screen_get_monitor_width_mm (BdkScreen *screen,
                                 gint       monitor_num)
{
  return bdk_screen_get_width_mm (screen);
}

gint
bdk_screen_get_monitor_height_mm (BdkScreen *screen,
                                  gint       monitor_num)
{
  return bdk_screen_get_height_mm (screen);
}

gchar *
bdk_screen_get_monitor_plug_name (BdkScreen *screen,
                                  gint       monitor_num)
{
  return g_strdup ("DirectFB");
}

gint
bdk_screen_get_number (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), 0);

  return 0;
}


gchar *
_bdk_windowing_substitute_screen_number (const gchar *display_name,
					 int          screen_number)
{
  return g_strdup (display_name);
}

gchar *
bdk_screen_make_display_name (BdkScreen *screen)
{
  return g_strdup ("DirectFB");
}

gint
bdk_screen_get_width (BdkScreen *screen)
{
  DFBDisplayLayerConfig dlc;

  _bdk_display->layer->GetConfiguration (_bdk_display->layer, &dlc);

  return dlc.width;
}

gint
bdk_screen_get_height (BdkScreen *screen)
{
  DFBDisplayLayerConfig dlc;

  _bdk_display->layer->GetConfiguration (_bdk_display->layer, &dlc);

  return dlc.height;
}

gint
bdk_screen_get_width_mm (BdkScreen *screen)
{
  static gboolean first_call = TRUE;
  DFBDisplayLayerConfig dlc;

  if (first_call)
    {
      g_message ("bdk_screen_width_mm() assumes a screen resolution of 72 dpi");
      first_call = FALSE;
    }

  _bdk_display->layer->GetConfiguration (_bdk_display->layer, &dlc);

  return (dlc.width * 254) / 720;
}

gint
bdk_screen_get_height_mm (BdkScreen *screen)
{
  static gboolean first_call = TRUE;
  DFBDisplayLayerConfig dlc;

  if (first_call)
    {
      g_message
        ("bdk_screen_height_mm() assumes a screen resolution of 72 dpi");
      first_call = FALSE;
    }

  _bdk_display->layer->GetConfiguration (_bdk_display->layer, &dlc);

  return (dlc.height * 254) / 720;
}

BdkVisual *
bdk_screen_get_rgba_visual (BdkScreen *screen)
{
  static BdkVisual *rgba_visual;

  if (!rgba_visual)
    rgba_visual = bdk_directfb_visual_by_format (DSPF_ARGB);

  return rgba_visual;
}

BdkColormap *
bdk_screen_get_rgba_colormap (BdkScreen *screen)
{
  static BdkColormap *rgba_colormap;
  if (!rgba_colormap && bdk_screen_get_rgba_visual (screen))
    rgba_colormap = bdk_colormap_new (bdk_screen_get_rgba_visual (screen), FALSE);
  return rgba_colormap;
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

#define __BDK_SCREEN_X11_C__
#include "bdkaliasdef.c"
