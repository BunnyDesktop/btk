/*
 * bdkscreen.c
 * 
 * Copyright 2001 Sun Microsystems Inc. 
 *
 * Erwann Chenede <erwann.chenede@sun.com>
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

#include "config.h"
#include "bdk.h"		/* For bdk_rectangle_intersect() */
#include "bdkcolor.h"
#include "bdkwindow.h"
#include "bdkscreen.h"
#include "bdkintl.h"
#include "bdkalias.h"

static void bdk_screen_dispose      (BObject        *object);
static void bdk_screen_finalize     (BObject        *object);
static void bdk_screen_set_property (BObject        *object,
				     guint           prop_id,
				     const BValue   *value,
				     BParamSpec     *pspec);
static void bdk_screen_get_property (BObject        *object,
				     guint           prop_id,
				     BValue         *value,
				     BParamSpec     *pspec);

enum
{
  PROP_0,
  PROP_FONT_OPTIONS,
  PROP_RESOLUTION
};

enum
{
  SIZE_CHANGED,
  COMPOSITED_CHANGED,
  MONITORS_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BdkScreen, bdk_screen, B_TYPE_OBJECT)

static void
bdk_screen_class_init (BdkScreenClass *klass)
{
  BObjectClass *object_class = B_OBJECT_CLASS (klass);

  object_class->dispose = bdk_screen_dispose;
  object_class->finalize = bdk_screen_finalize;
  object_class->set_property = bdk_screen_set_property;
  object_class->get_property = bdk_screen_get_property;
  
  g_object_class_install_property (object_class,
				   PROP_FONT_OPTIONS,
				   g_param_spec_pointer ("font-options",
							 P_("Font options"),
							 P_("The default font options for the screen"),
							 G_PARAM_READWRITE|G_PARAM_STATIC_NAME|
							G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
				   PROP_RESOLUTION,
				   g_param_spec_double ("resolution",
							P_("Font resolution"),
							P_("The resolution for fonts on the screen"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE,
							-1.0,
							G_PARAM_READWRITE|G_PARAM_STATIC_NAME|
							G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB));

  /**
   * BdkScreen::size-changed:
   * @screen: the object on which the signal is emitted
   * 
   * The ::size-changed signal is emitted when the pixel width or 
   * height of a screen changes.
   *
   * Since: 2.2
   */
  signals[SIZE_CHANGED] =
    g_signal_new (g_intern_static_string ("size-changed"),
                  B_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BdkScreenClass, size_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  B_TYPE_NONE,
                  0);

  /**
   * BdkScreen::composited-changed:
   * @screen: the object on which the signal is emitted
   *
   * The ::composited-changed signal is emitted when the composited
   * status of the screen changes
   *
   * Since: 2.10
   */
  signals[COMPOSITED_CHANGED] =
    g_signal_new (g_intern_static_string ("composited-changed"),
		  B_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BdkScreenClass, composited_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  B_TYPE_NONE,
		  0);
	
  /**
   * BdkScreen::monitors-changed:
   * @screen: the object on which the signal is emitted
   *
   * The ::monitors-changed signal is emitted when the number, size
   * or position of the monitors attached to the screen change. 
   *
   * Only for X11 and OS X for now. A future implementation for Win32
   * may be a possibility.
   *
   * Since: 2.14
   */
  signals[MONITORS_CHANGED] =
    g_signal_new (g_intern_static_string ("monitors-changed"),
		  B_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BdkScreenClass, monitors_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  B_TYPE_NONE,
		  0);
}

static void
bdk_screen_init (BdkScreen *screen)
{
  screen->resolution = -1.;
}

static void
bdk_screen_dispose (BObject *object)
{
  BdkScreen *screen = BDK_SCREEN (object);
  gint i;

  for (i = 0; i < 32; ++i)
    {
      if (screen->exposure_gcs[i])
        {
          g_object_unref (screen->exposure_gcs[i]);
          screen->exposure_gcs[i] = NULL;
        }

      if (screen->normal_gcs[i])
        {
          g_object_unref (screen->normal_gcs[i]);
          screen->normal_gcs[i] = NULL;
        }
    }

  B_OBJECT_CLASS (bdk_screen_parent_class)->dispose (object);
}

static void
bdk_screen_finalize (BObject *object)
{
  BdkScreen *screen = BDK_SCREEN (object);

  if (screen->font_options)
      bairo_font_options_destroy (screen->font_options);

  B_OBJECT_CLASS (bdk_screen_parent_class)->finalize (object);
}

void 
_bdk_screen_close (BdkScreen *screen)
{
  g_return_if_fail (BDK_IS_SCREEN (screen));

  if (!screen->closed)
    {
      screen->closed = TRUE;
      g_object_run_dispose (B_OBJECT (screen));
    }
}

/* Fallback used when the monitor "at" a point or window
 * doesn't exist.
 */
static gint
get_nearest_monitor (BdkScreen *screen,
		     gint       x,
		     gint       y)
{
  gint num_monitors, i;
  gint nearest_dist = G_MAXINT;
  gint nearest_monitor = 0;

  g_return_val_if_fail (BDK_IS_SCREEN (screen), -1);

  num_monitors = bdk_screen_get_n_monitors (screen);
  
  for (i = 0; i < num_monitors; i++)
    {
      BdkRectangle monitor;
      gint dist_x, dist_y, dist;
      
      bdk_screen_get_monitor_geometry (screen, i, &monitor);

      if (x < monitor.x)
	dist_x = monitor.x - x;
      else if (x >= monitor.x + monitor.width)
	dist_x = x - (monitor.x + monitor.width) + 1;
      else
	dist_x = 0;

      if (y < monitor.y)
	dist_y = monitor.y - y;
      else if (y >= monitor.y + monitor.height)
	dist_y = y - (monitor.y + monitor.height) + 1;
      else
	dist_y = 0;

      dist = dist_x + dist_y;
      if (dist < nearest_dist)
	{
	  nearest_dist = dist;
	  nearest_monitor = i;
	}
    }

  return nearest_monitor;
}

/**
 * bdk_screen_get_monitor_at_point:
 * @screen: a #BdkScreen.
 * @x: the x coordinate in the virtual screen.
 * @y: the y coordinate in the virtual screen.
 *
 * Returns the monitor number in which the point (@x,@y) is located.
 *
 * Returns: the monitor number in which the point (@x,@y) lies, or
 *   a monitor close to (@x,@y) if the point is not in any monitor.
 *
 * Since: 2.2
 **/
gint 
bdk_screen_get_monitor_at_point (BdkScreen *screen,
				 gint       x,
				 gint       y)
{
  gint num_monitors, i;
  
  g_return_val_if_fail (BDK_IS_SCREEN (screen), -1);

  num_monitors = bdk_screen_get_n_monitors (screen);
  
  for (i=0;i<num_monitors;i++)
    {
      BdkRectangle monitor;
      
      bdk_screen_get_monitor_geometry (screen, i, &monitor);

      if (x >= monitor.x &&
          x < monitor.x + monitor.width &&
          y >= monitor.y &&
          y < (monitor.y + monitor.height))
        return i;
    }

  return get_nearest_monitor (screen, x, y);
}

/**
 * bdk_screen_get_monitor_at_window:
 * @screen: a #BdkScreen.
 * @window: a #BdkWindow
 * @returns: the monitor number in which most of @window is located,
 *           or if @window does not intersect any monitors, a monitor,
 *           close to @window.
 *
 * Returns the number of the monitor in which the largest area of the 
 * bounding rectangle of @window resides.
 *
 * Since: 2.2
 **/
gint 
bdk_screen_get_monitor_at_window (BdkScreen      *screen,
				  BdkWindow	 *window)
{
  gint num_monitors, i, area = 0, screen_num = -1;
  BdkRectangle win_rect;

  g_return_val_if_fail (BDK_IS_SCREEN (screen), -1);

  bdk_window_get_geometry (window, &win_rect.x, &win_rect.y, &win_rect.width,
			   &win_rect.height, NULL);
  bdk_window_get_origin (window, &win_rect.x, &win_rect.y);
  num_monitors = bdk_screen_get_n_monitors (screen);
  
  for (i=0;i<num_monitors;i++)
    {
      BdkRectangle tmp_monitor, intersect;
      
      bdk_screen_get_monitor_geometry (screen, i, &tmp_monitor);
      bdk_rectangle_intersect (&win_rect, &tmp_monitor, &intersect);
      
      if (intersect.width * intersect.height > area)
	{ 
	  area = intersect.width * intersect.height;
	  screen_num = i;
	}
    }
  if (screen_num >= 0)
    return screen_num;
  else
    return get_nearest_monitor (screen,
				win_rect.x + win_rect.width / 2,
				win_rect.y + win_rect.height / 2);
}

/**
 * bdk_screen_width:
 * 
 * Returns the width of the default screen in pixels.
 * 
 * Return value: the width of the default screen in pixels.
 **/
gint
bdk_screen_width (void)
{
  return bdk_screen_get_width (bdk_screen_get_default ());
}

/**
 * bdk_screen_height:
 * 
 * Returns the height of the default screen in pixels.
 * 
 * Return value: the height of the default screen in pixels.
 **/
gint
bdk_screen_height (void)
{
  return bdk_screen_get_height (bdk_screen_get_default ());
}

/**
 * bdk_screen_width_mm:
 * 
 * Returns the width of the default screen in millimeters.
 * Note that on many X servers this value will not be correct.
 * 
 * Return value: the width of the default screen in millimeters,
 * though it is not always correct.
 **/
gint
bdk_screen_width_mm (void)
{
  return bdk_screen_get_width_mm (bdk_screen_get_default ());
}

/**
 * bdk_screen_height_mm:
 * 
 * Returns the height of the default screen in millimeters.
 * Note that on many X servers this value will not be correct.
 * 
 * Return value: the height of the default screen in millimeters,
 * though it is not always correct.
 **/
gint
bdk_screen_height_mm (void)
{
  return bdk_screen_get_height_mm (bdk_screen_get_default ());
}

/**
 * bdk_screen_set_font_options:
 * @screen: a #BdkScreen
 * @options: (allow-none): a #bairo_font_options_t, or %NULL to unset any
 *   previously set default font options.
 *
 * Sets the default font options for the screen. These
 * options will be set on any #BangoContext's newly created
 * with bdk_bango_context_get_for_screen(). Changing the
 * default set of font options does not affect contexts that
 * have already been created.
 *
 * Since: 2.10
 **/
void
bdk_screen_set_font_options (BdkScreen                  *screen,
			     const bairo_font_options_t *options)
{
  g_return_if_fail (BDK_IS_SCREEN (screen));

  if (screen->font_options != options)
    {
      if (screen->font_options)
        bairo_font_options_destroy (screen->font_options);

      if (options)
        screen->font_options = bairo_font_options_copy (options);
      else
        screen->font_options = NULL;

      g_object_notify (B_OBJECT (screen), "font-options");
    }
}

/**
 * bdk_screen_get_font_options:
 * @screen: a #BdkScreen
 * 
 * Gets any options previously set with bdk_screen_set_font_options().
 * 
 * Return value: the current font options, or %NULL if no default
 *  font options have been set.
 *
 * Since: 2.10
 **/
const bairo_font_options_t *
bdk_screen_get_font_options (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);

  return screen->font_options;
}

/**
 * bdk_screen_set_resolution:
 * @screen: a #BdkScreen
 * @dpi: the resolution in "dots per inch". (Physical inches aren't actually
 *   involved; the terminology is conventional.)
 
 * Sets the resolution for font handling on the screen. This is a
 * scale factor between points specified in a #BangoFontDescription
 * and bairo units. The default value is 96, meaning that a 10 point
 * font will be 13 units high. (10 * 96. / 72. = 13.3).
 *
 * Since: 2.10
 **/
void
bdk_screen_set_resolution (BdkScreen *screen,
			   gdouble    dpi)
{
  g_return_if_fail (BDK_IS_SCREEN (screen));

  if (dpi < 0)
    dpi = -1.0;

  if (screen->resolution != dpi)
    {
      screen->resolution = dpi;

      g_object_notify (B_OBJECT (screen), "resolution");
    }
}

/**
 * bdk_screen_get_resolution:
 * @screen: a #BdkScreen
 * 
 * Gets the resolution for font handling on the screen; see
 * bdk_screen_set_resolution() for full details.
 * 
 * Return value: the current resolution, or -1 if no resolution
 * has been set.
 *
 * Since: 2.10
 **/
gdouble
bdk_screen_get_resolution (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), -1.0);

  return screen->resolution;
}

static void
bdk_screen_get_property (BObject      *object,
			 guint         prop_id,
			 BValue       *value,
			 BParamSpec   *pspec)
{
  BdkScreen *screen = BDK_SCREEN (object);

  switch (prop_id)
    {
    case PROP_FONT_OPTIONS:
      b_value_set_pointer (value, (gpointer) bdk_screen_get_font_options (screen));
      break;
    case PROP_RESOLUTION:
      b_value_set_double (value, bdk_screen_get_resolution (screen));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bdk_screen_set_property (BObject      *object,
			 guint         prop_id,
			 const BValue *value,
			 BParamSpec   *pspec)
{
  BdkScreen *screen = BDK_SCREEN (object);

  switch (prop_id)
    {
    case PROP_FONT_OPTIONS:
      bdk_screen_set_font_options (screen, b_value_get_pointer (value));
      break;
    case PROP_RESOLUTION:
      bdk_screen_set_resolution (screen, b_value_get_double (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

#define __BDK_SCREEN_C__
#include "bdkaliasdef.c"
