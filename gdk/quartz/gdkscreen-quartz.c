/* bdkscreen-quartz.c
 *
 * Copyright (C) 2005 Imendio AB
 * Copyright (C) 2009  Kristian Rietveld  <kris@btk.org>
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
#include "bdkscreen-quartz.h"
#include "bdkprivate-quartz.h"
 

/* A couple of notes about this file are in order.  In BDK, a
 * BdkScreen can contain multiple monitors.  A BdkScreen has an
 * associated root window, in which the monitors are placed.  The
 * root window "spans" all monitors.  The origin is at the top-left
 * corner of the root window.
 *
 * Cocoa works differently.  The system has a "screen" (NSScreen) for
 * each monitor that is connected (note the conflicting definitions
 * of screen).  The screen containing the menu bar is screen 0 and the
 * bottom-left corner of this screen is the origin of the "monitor
 * coordinate space".  All other screens are positioned according to this
 * origin.  If the menu bar is on a secondary screen (for example on
 * a monitor hooked up to a laptop), then this screen is screen 0 and
 * other monitors will be positioned according to the "secondary screen".
 * The main screen is the monitor that shows the window that is currently
 * active (has focus), the position of the menu bar does not have influence
 * on this!
 *
 * Upon start up and changes in the layout of screens, we calculate the
 * size of the BdkScreen root window that is needed to be able to place
 * all monitors in the root window.  Once that size is known, we iterate
 * over the monitors and translate their Cocoa position to a position
 * in the root window of the BdkScreen.  This happens below in the
 * function bdk_screen_quartz_calculate_layout().
 *
 * A Cocoa coordinate is always relative to the origin of the monitor
 * coordinate space.  Such coordinates are mapped to their respective
 * position in the BdkScreen root window (_bdk_quartz_window_xy_to_bdk_xy)
 * and vice versa (_bdk_quartz_window_bdk_xy_to_xy).  Both functions can
 * be found in bdkwindow-quartz.c.  Note that Cocoa coordinates can have
 * negative values (in case a monitor is located left or below of screen 0),
 * but BDK coordinates can *not*!
 */

static void  bdk_screen_quartz_dispose          (GObject         *object);
static void  bdk_screen_quartz_finalize         (GObject         *object);
static void  bdk_screen_quartz_calculate_layout (BdkScreenQuartz *screen);

static void display_reconfiguration_callback (CGDirectDisplayID            display,
                                              CGDisplayChangeSummaryFlags  flags,
                                              void                        *userInfo);

G_DEFINE_TYPE (BdkScreenQuartz, _bdk_screen_quartz, BDK_TYPE_SCREEN);

static void
_bdk_screen_quartz_class_init (BdkScreenQuartzClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = bdk_screen_quartz_dispose;
  object_class->finalize = bdk_screen_quartz_finalize;
}

static void
_bdk_screen_quartz_init (BdkScreenQuartz *screen_quartz)
{
  BdkScreen *screen = BDK_SCREEN (screen_quartz);
  NSScreen *nsscreen;

  bdk_screen_set_default_colormap (screen,
                                   bdk_screen_get_system_colormap (screen));

  nsscreen = [[NSScreen screens] objectAtIndex:0];
  bdk_screen_set_resolution (screen,
                             72.0 * [nsscreen userSpaceScaleFactor]);

  bdk_screen_quartz_calculate_layout (screen_quartz);

  CGDisplayRegisterReconfigurationCallback (display_reconfiguration_callback,
                                            screen);

  screen_quartz->emit_monitors_changed = FALSE;
}

static void
bdk_screen_quartz_dispose (GObject *object)
{
  BdkScreenQuartz *screen = BDK_SCREEN_QUARTZ (object);

  if (screen->default_colormap)
    {
      g_object_unref (screen->default_colormap);
      screen->default_colormap = NULL;
    }

  if (screen->screen_changed_id)
    {
      g_source_remove (screen->screen_changed_id);
      screen->screen_changed_id = 0;
    }

  CGDisplayRemoveReconfigurationCallback (display_reconfiguration_callback,
                                          screen);

  G_OBJECT_CLASS (_bdk_screen_quartz_parent_class)->dispose (object);
}

static void
bdk_screen_quartz_screen_rects_free (BdkScreenQuartz *screen)
{
  screen->n_screens = 0;

  if (screen->screen_rects)
    {
      g_free (screen->screen_rects);
      screen->screen_rects = NULL;
    }
}

static void
bdk_screen_quartz_finalize (GObject *object)
{
  BdkScreenQuartz *screen = BDK_SCREEN_QUARTZ (object);

  bdk_screen_quartz_screen_rects_free (screen);
}


static void
bdk_screen_quartz_calculate_layout (BdkScreenQuartz *screen)
{
  NSArray *array;
  int i;
  int max_x, max_y;

  BDK_QUARTZ_ALLOC_POOL;

  bdk_screen_quartz_screen_rects_free (screen);

  array = [NSScreen screens];

  screen->width = 0;
  screen->height = 0;
  screen->min_x = 0;
  screen->min_y = 0;
  max_x = max_y = 0;

  /* We determine the minimum and maximum x and y coordinates
   * covered by the monitors.  From this we can deduce the width
   * and height of the root screen.
   */
  for (i = 0; i < [array count]; i++)
    {
      NSRect rect = [[array objectAtIndex:i] frame];

      screen->min_x = MIN (screen->min_x, rect.origin.x);
      max_x = MAX (max_x, rect.origin.x + rect.size.width);

      screen->min_y = MIN (screen->min_y, rect.origin.y);
      max_y = MAX (max_y, rect.origin.y + rect.size.height);
    }

  screen->width = max_x - screen->min_x;
  screen->height = max_y - screen->min_y;

  screen->n_screens = [array count];
  screen->screen_rects = g_new0 (BdkRectangle, screen->n_screens);

  for (i = 0; i < screen->n_screens; i++)
    {
      NSScreen *nsscreen;
      NSRect rect;

      nsscreen = [array objectAtIndex:i];
      rect = [nsscreen frame];

      screen->screen_rects[i].x = rect.origin.x - screen->min_x;
      screen->screen_rects[i].y
          = screen->height - (rect.origin.y + rect.size.height) + screen->min_y;
      screen->screen_rects[i].width = rect.size.width;
      screen->screen_rects[i].height = rect.size.height;
    }

  BDK_QUARTZ_RELEASE_POOL;
}


static void
process_display_reconfiguration (BdkScreenQuartz *screen)
{
  int width, height;

  width = bdk_screen_get_width (BDK_SCREEN (screen));
  height = bdk_screen_get_height (BDK_SCREEN (screen));

  bdk_screen_quartz_calculate_layout (BDK_SCREEN_QUARTZ (screen));

  _bdk_windowing_update_window_sizes (BDK_SCREEN (screen));

  if (screen->emit_monitors_changed)
    {
      g_signal_emit_by_name (screen, "monitors-changed");
      screen->emit_monitors_changed = FALSE;
    }

  if (width != bdk_screen_get_width (BDK_SCREEN (screen))
      || height != bdk_screen_get_height (BDK_SCREEN (screen)))
    g_signal_emit_by_name (screen, "size-changed");
}

static gboolean
screen_changed_idle (gpointer data)
{
  BdkScreenQuartz *screen = data;

  process_display_reconfiguration (data);

  screen->screen_changed_id = 0;

  return FALSE;
}

static void
display_reconfiguration_callback (CGDirectDisplayID            display,
                                  CGDisplayChangeSummaryFlags  flags,
                                  void                        *userInfo)
{
  BdkScreenQuartz *screen = userInfo;

  if (flags & kCGDisplayBeginConfigurationFlag)
    {
      /* Ignore the begin configuration signal. */
      return;
    }
  else
    {
      /* We save information about the changes, so we can emit
       * ::monitors-changed when appropriate.  This signal must be
       * emitted when the number, size of position of one of the
       * monitors changes.
       */
      if (flags & kCGDisplayMovedFlag
          || flags & kCGDisplayAddFlag
          || flags & kCGDisplayRemoveFlag
          || flags & kCGDisplayEnabledFlag
          || flags & kCGDisplayDisabledFlag)
        screen->emit_monitors_changed = TRUE;

      /* At this point Cocoa does not know about the new screen data
       * yet, so we delay our refresh into an idle handler.
       */
      if (!screen->screen_changed_id)
        screen->screen_changed_id = bdk_threads_add_idle (screen_changed_idle,
                                                          screen);
    }
}

BdkScreen *
_bdk_screen_quartz_new (void)
{
  return g_object_new (BDK_TYPE_SCREEN_QUARTZ, NULL);
}

BdkDisplay *
bdk_screen_get_display (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);

  return _bdk_display;
}


BdkWindow *
bdk_screen_get_root_window (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);

  return _bdk_root;
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
  if (screen_number != 0)
    return NULL;

  return g_strdup (display_name);
}

BdkColormap*
bdk_screen_get_default_colormap (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);

  return BDK_SCREEN_QUARTZ (screen)->default_colormap;
}

void
bdk_screen_set_default_colormap (BdkScreen   *screen,
				 BdkColormap *colormap)
{
  BdkColormap *old_colormap;
  
  g_return_if_fail (BDK_IS_SCREEN (screen));
  g_return_if_fail (BDK_IS_COLORMAP (colormap));

  old_colormap = BDK_SCREEN_QUARTZ (screen)->default_colormap;

  BDK_SCREEN_QUARTZ (screen)->default_colormap = g_object_ref (colormap);
  
  if (old_colormap)
    g_object_unref (old_colormap);
}

gint
bdk_screen_get_width (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), 0);

  return BDK_SCREEN_QUARTZ (screen)->width;
}

gint
bdk_screen_get_height (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), 0);

  return BDK_SCREEN_QUARTZ (screen)->height;
}

static gint
get_mm_from_pixels (NSScreen *screen, int pixels)
{
  /* userSpaceScaleFactor is in "pixels per point", 
   * 72 is the number of points per inch, 
   * and 25.4 is the number of millimeters per inch.
   */
#if MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_3
  float dpi = [screen userSpaceScaleFactor] * 72.0;
#else
  float dpi = 96.0 / 72.0;
#endif

  return (pixels / dpi) * 25.4;
}

static NSScreen *
get_nsscreen_for_monitor (gint monitor_num)
{
  NSArray *array;
  NSScreen *screen;

  BDK_QUARTZ_ALLOC_POOL;

  array = [NSScreen screens];
  screen = [array objectAtIndex:monitor_num];

  BDK_QUARTZ_RELEASE_POOL;

  return screen;
}

gint
bdk_screen_get_width_mm (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), 0);

  return get_mm_from_pixels (get_nsscreen_for_monitor (0),
                             BDK_SCREEN_QUARTZ (screen)->width);
}

gint
bdk_screen_get_height_mm (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), 0);

  return get_mm_from_pixels (get_nsscreen_for_monitor (0),
                             BDK_SCREEN_QUARTZ (screen)->height);
}

gint
bdk_screen_get_n_monitors (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), 0);

  return BDK_SCREEN_QUARTZ (screen)->n_screens;
}

gint
bdk_screen_get_primary_monitor (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), 0);

  return 0;
}

gint
bdk_screen_get_monitor_width_mm	(BdkScreen *screen,
				 gint       monitor_num)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), 0);
  g_return_val_if_fail (monitor_num < bdk_screen_get_n_monitors (screen), 0);
  g_return_val_if_fail (monitor_num >= 0, 0);

  return get_mm_from_pixels (get_nsscreen_for_monitor (monitor_num),
                             BDK_SCREEN_QUARTZ (screen)->screen_rects[monitor_num].width);
}

gint
bdk_screen_get_monitor_height_mm (BdkScreen *screen,
                                  gint       monitor_num)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), 0);
  g_return_val_if_fail (monitor_num < bdk_screen_get_n_monitors (screen), 0);
  g_return_val_if_fail (monitor_num >= 0, 0);

  return get_mm_from_pixels (get_nsscreen_for_monitor (monitor_num),
                             BDK_SCREEN_QUARTZ (screen)->screen_rects[monitor_num].height);
}

gchar *
bdk_screen_get_monitor_plug_name (BdkScreen *screen,
				  gint       monitor_num)
{
  /* FIXME: Is there some useful name we could use here? */
  return NULL;
}

void
bdk_screen_get_monitor_geometry (BdkScreen    *screen, 
				 gint          monitor_num,
				 BdkRectangle *dest)
{
  g_return_if_fail (BDK_IS_SCREEN (screen));
  g_return_if_fail (monitor_num < bdk_screen_get_n_monitors (screen));
  g_return_if_fail (monitor_num >= 0);

  *dest = BDK_SCREEN_QUARTZ (screen)->screen_rects[monitor_num];
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

  return TRUE;
}
