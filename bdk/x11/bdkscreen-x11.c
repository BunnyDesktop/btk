 /*
 * bdkscreen-x11.c
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

#include <stdlib.h>
#include <string.h>

#include <bunnylib.h>
#include "bdkscreen.h"
#include "bdkscreen-x11.h"
#include "bdkdisplay.h"
#include "bdkdisplay-x11.h"
#include "bdkx.h"
#include "bdkalias.h"

#include <X11/Xatom.h>

#ifdef HAVE_SOLARIS_XINERAMA
#include <X11/extensions/xinerama.h>
#endif
#ifdef HAVE_XFREE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#ifdef HAVE_RANDR
#include <X11/extensions/Xrandr.h>
#endif

#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif

static void         bdk_screen_x11_dispose     (BObject		  *object);
static void         bdk_screen_x11_finalize    (BObject		  *object);
static void	    init_randr_support	       (BdkScreen	  *screen);
static void	    deinit_multihead           (BdkScreen         *screen);

enum
{
  WINDOW_MANAGER_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BdkScreenX11, _bdk_screen_x11, BDK_TYPE_SCREEN)

struct _BdkX11Monitor
{
  BdkRectangle  geometry;
  XID		output;
  int		width_mm;
  int		height_mm;
  char *	output_name;
  char *	manufacturer;
};

static void
_bdk_screen_x11_class_init (BdkScreenX11Class *klass)
{
  BObjectClass *object_class = B_OBJECT_CLASS (klass);
  
  object_class->dispose = bdk_screen_x11_dispose;
  object_class->finalize = bdk_screen_x11_finalize;

  signals[WINDOW_MANAGER_CHANGED] =
    g_signal_new (g_intern_static_string ("window_manager_changed"),
                  B_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BdkScreenX11Class, window_manager_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  B_TYPE_NONE,
                  0);
}

static void
_bdk_screen_x11_init (BdkScreenX11 *screen)
{
}

/**
 * bdk_screen_get_display:
 * @screen: a #BdkScreen
 *
 * Gets the display to which the @screen belongs.
 * 
 * Returns: the display to which @screen belongs
 *
 * Since: 2.2
 **/
BdkDisplay *
bdk_screen_get_display (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);

  return BDK_SCREEN_X11 (screen)->display;
}
/**
 * bdk_screen_get_width:
 * @screen: a #BdkScreen
 *
 * Gets the width of @screen in pixels
 * 
 * Returns: the width of @screen in pixels.
 *
 * Since: 2.2
 **/
gint
bdk_screen_get_width (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), 0);

  return WidthOfScreen (BDK_SCREEN_X11 (screen)->xscreen);
}

/**
 * bdk_screen_get_height:
 * @screen: a #BdkScreen
 *
 * Gets the height of @screen in pixels
 * 
 * Returns: the height of @screen in pixels.
 *
 * Since: 2.2
 **/
gint
bdk_screen_get_height (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), 0);

  return HeightOfScreen (BDK_SCREEN_X11 (screen)->xscreen);
}

/**
 * bdk_screen_get_width_mm:
 * @screen: a #BdkScreen
 *
 * Gets the width of @screen in millimeters. 
 * Note that on some X servers this value will not be correct.
 * 
 * Returns: the width of @screen in millimeters.
 *
 * Since: 2.2
 **/
gint
bdk_screen_get_width_mm (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), 0);  

  return WidthMMOfScreen (BDK_SCREEN_X11 (screen)->xscreen);
}

/**
 * bdk_screen_get_height_mm:
 * @screen: a #BdkScreen
 *
 * Returns the height of @screen in millimeters. 
 * Note that on some X servers this value will not be correct.
 * 
 * Returns: the heigth of @screen in millimeters.
 *
 * Since: 2.2
 **/
gint
bdk_screen_get_height_mm (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), 0);

  return HeightMMOfScreen (BDK_SCREEN_X11 (screen)->xscreen);
}

/**
 * bdk_screen_get_number:
 * @screen: a #BdkScreen
 *
 * Gets the index of @screen among the screens in the display
 * to which it belongs. (See bdk_screen_get_display())
 * 
 * Returns: the index
 *
 * Since: 2.2
 **/
gint
bdk_screen_get_number (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), 0);
  
  return BDK_SCREEN_X11 (screen)->screen_num;
}

/**
 * bdk_screen_get_root_window:
 * @screen: a #BdkScreen
 *
 * Gets the root window of @screen.
 *
 * Returns: (transfer none): the root window
 *
 * Since: 2.2
 **/
BdkWindow *
bdk_screen_get_root_window (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);

  return BDK_SCREEN_X11 (screen)->root_window;
}

/**
 * bdk_screen_get_default_colormap:
 * @screen: a #BdkScreen
 *
 * Gets the default colormap for @screen.
 * 
 * Returns: (transfer none): the default #BdkColormap.
 *
 * Since: 2.2
 **/
BdkColormap *
bdk_screen_get_default_colormap (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);

  return BDK_SCREEN_X11 (screen)->default_colormap;
}

/**
 * bdk_screen_set_default_colormap:
 * @screen: a #BdkScreen
 * @colormap: a #BdkColormap
 *
 * Sets the default @colormap for @screen.
 *
 * Since: 2.2
 **/
void
bdk_screen_set_default_colormap (BdkScreen   *screen,
				 BdkColormap *colormap)
{
  BdkColormap *old_colormap;
  
  g_return_if_fail (BDK_IS_SCREEN (screen));
  g_return_if_fail (BDK_IS_COLORMAP (colormap));

  old_colormap = BDK_SCREEN_X11 (screen)->default_colormap;

  BDK_SCREEN_X11 (screen)->default_colormap = g_object_ref (colormap);
  
  if (old_colormap)
    g_object_unref (old_colormap);
}

static void
bdk_screen_x11_dispose (BObject *object)
{
  BdkScreenX11 *screen_x11 = BDK_SCREEN_X11 (object);

  _bdk_x11_events_uninit_screen (BDK_SCREEN (object));

  if (screen_x11->default_colormap)
    {
      g_object_unref (screen_x11->default_colormap);
      screen_x11->default_colormap = NULL;
    }

  if (screen_x11->system_colormap)
    {
      g_object_unref (screen_x11->system_colormap);
      screen_x11->system_colormap = NULL;
    }

  if (screen_x11->rgba_colormap)
    {
      g_object_unref (screen_x11->rgba_colormap);
      screen_x11->rgba_colormap = NULL;
    }

  if (screen_x11->root_window)
    _bdk_window_destroy (screen_x11->root_window, TRUE);

  B_OBJECT_CLASS (_bdk_screen_x11_parent_class)->dispose (object);

  screen_x11->xdisplay = NULL;
  screen_x11->xscreen = NULL;
  screen_x11->screen_num = -1;
  screen_x11->xroot_window = None;
  screen_x11->wmspec_check_window = None;
}

static void
bdk_screen_x11_finalize (BObject *object)
{
  BdkScreenX11 *screen_x11 = BDK_SCREEN_X11 (object);
  gint          i;

  if (screen_x11->root_window)
    g_object_unref (screen_x11->root_window);

  if (screen_x11->renderer)
    g_object_unref (screen_x11->renderer);

  /* Visual Part */
  for (i = 0; i < screen_x11->nvisuals; i++)
    g_object_unref (screen_x11->visuals[i]);
  g_free (screen_x11->visuals);
  g_hash_table_destroy (screen_x11->visual_hash);

  g_free (screen_x11->window_manager_name);

  g_hash_table_destroy (screen_x11->colormap_hash);

  deinit_multihead (BDK_SCREEN (object));
  
  B_OBJECT_CLASS (_bdk_screen_x11_parent_class)->finalize (object);
}

/**
 * bdk_screen_get_n_monitors:
 * @screen: a #BdkScreen
 *
 * Returns the number of monitors which @screen consists of.
 *
 * Returns: number of monitors which @screen consists of
 *
 * Since: 2.2
 */
gint
bdk_screen_get_n_monitors (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), 0);

  return BDK_SCREEN_X11 (screen)->n_monitors;
}

/**
 * bdk_screen_get_primary_monitor:
 * @screen: a #BdkScreen.
 *
 * Gets the primary monitor for @screen.  The primary monitor
 * is considered the monitor where the 'main desktop' lives.
 * While normal application windows typically allow the window
 * manager to place the windows, specialized desktop applications
 * such as panels should place themselves on the primary monitor.
 *
 * If no primary monitor is configured by the user, the return value
 * will be 0, defaulting to the first monitor.
 *
 * Returns: An integer index for the primary monitor, or 0 if none is configured.
 *
 * Since: 2.20
 */
gint
bdk_screen_get_primary_monitor (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), 0);

  return BDK_SCREEN_X11 (screen)->primary_monitor;
}

/**
 * bdk_screen_get_monitor_width_mm:
 * @screen: a #BdkScreen
 * @monitor_num: number of the monitor, between 0 and bdk_screen_get_n_monitors (screen)
 *
 * Gets the width in millimeters of the specified monitor, if available.
 *
 * Returns: the width of the monitor, or -1 if not available
 *
 * Since: 2.14
 */
gint
bdk_screen_get_monitor_width_mm	(BdkScreen *screen,
				 gint       monitor_num)
{
  BdkScreenX11 *screen_x11 = BDK_SCREEN_X11 (screen);

  g_return_val_if_fail (BDK_IS_SCREEN (screen), -1);
  g_return_val_if_fail (monitor_num >= 0, -1);
  g_return_val_if_fail (monitor_num < screen_x11->n_monitors, -1);

  return screen_x11->monitors[monitor_num].width_mm;
}

/**
 * bdk_screen_get_monitor_height_mm:
 * @screen: a #BdkScreen
 * @monitor_num: number of the monitor, between 0 and bdk_screen_get_n_monitors (screen)
 *
 * Gets the height in millimeters of the specified monitor.
 *
 * Returns: the height of the monitor, or -1 if not available
 *
 * Since: 2.14
 */
gint
bdk_screen_get_monitor_height_mm (BdkScreen *screen,
                                  gint       monitor_num)
{
  BdkScreenX11 *screen_x11 = BDK_SCREEN_X11 (screen);

  g_return_val_if_fail (BDK_IS_SCREEN (screen), -1);
  g_return_val_if_fail (monitor_num >= 0, -1);
  g_return_val_if_fail (monitor_num < screen_x11->n_monitors, -1);

  return screen_x11->monitors[monitor_num].height_mm;
}

/**
 * bdk_screen_get_monitor_plug_name:
 * @screen: a #BdkScreen
 * @monitor_num: number of the monitor, between 0 and bdk_screen_get_n_monitors (screen)
 *
 * Returns the output name of the specified monitor.
 * Usually something like VGA, DVI, or TV, not the actual
 * product name of the display device.
 *
 * Returns: a newly-allocated string containing the name of the monitor,
 *   or %NULL if the name cannot be determined
 *
 * Since: 2.14
 */
gchar *
bdk_screen_get_monitor_plug_name (BdkScreen *screen,
				  gint       monitor_num)
{
  BdkScreenX11 *screen_x11 = BDK_SCREEN_X11 (screen);

  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);
  g_return_val_if_fail (monitor_num >= 0, NULL);
  g_return_val_if_fail (monitor_num < screen_x11->n_monitors, NULL);

  return g_strdup (screen_x11->monitors[monitor_num].output_name);
}

/**
 * bdk_x11_screen_get_monitor_output:
 * @screen: a #BdkScreen
 * @monitor_num: number of the monitor, between 0 and bdk_screen_get_n_monitors (screen)
 *
 * Gets the XID of the specified output/monitor.
 * If the X server does not support version 1.2 of the RANDR
 * extension, 0 is returned.
 *
 * Returns: the XID of the monitor
 *
 * Since: 2.14
 */
XID
bdk_x11_screen_get_monitor_output (BdkScreen *screen,
                                   gint       monitor_num)
{
  BdkScreenX11 *screen_x11 = BDK_SCREEN_X11 (screen);

  g_return_val_if_fail (BDK_IS_SCREEN (screen), None);
  g_return_val_if_fail (monitor_num >= 0, None);
  g_return_val_if_fail (monitor_num < screen_x11->n_monitors, None);

  return screen_x11->monitors[monitor_num].output;
}

/**
 * bdk_screen_get_monitor_geometry:
 * @screen: a #BdkScreen
 * @monitor_num: the monitor number, between 0 and bdk_screen_get_n_monitors (screen)
 * @dest: a #BdkRectangle to be filled with the monitor geometry
 *
 * Retrieves the #BdkRectangle representing the size and position of
 * the individual monitor within the entire screen area.
 *
 * Note that the size of the entire screen area can be retrieved via
 * bdk_screen_get_width() and bdk_screen_get_height().
 *
 * Since: 2.2
 */
void
bdk_screen_get_monitor_geometry (BdkScreen    *screen,
				 gint          monitor_num,
				 BdkRectangle *dest)
{
  BdkScreenX11 *screen_x11 = BDK_SCREEN_X11 (screen);

  g_return_if_fail (BDK_IS_SCREEN (screen));
  g_return_if_fail (monitor_num >= 0);
  g_return_if_fail (monitor_num < screen_x11->n_monitors);

  if (dest)
    *dest = screen_x11->monitors[monitor_num].geometry;
}

/**
 * bdk_screen_get_rgba_colormap:
 * @screen: a #BdkScreen.
 * 
 * Gets a colormap to use for creating windows or pixmaps with an
 * alpha channel. The windowing system on which BTK+ is running
 * may not support this capability, in which case %NULL will
 * be returned. Even if a non-%NULL value is returned, its
 * possible that the window's alpha channel won't be honored
 * when displaying the window on the screen: in particular, for
 * X an appropriate windowing manager and compositing manager
 * must be running to provide appropriate display.
 *
 * This functionality is not implemented in the Windows backend.
 *
 * For setting an overall opacity for a top-level window, see
 * bdk_window_set_opacity().

 * Return value: (transfer none): a colormap to use for windows with
 *     an alpha channel or %NULL if the capability is not available.
 *
 * Since: 2.8
 **/
BdkColormap *
bdk_screen_get_rgba_colormap (BdkScreen *screen)
{
  BdkScreenX11 *screen_x11;

  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);

  screen_x11 = BDK_SCREEN_X11 (screen);

  if (!screen_x11->rgba_visual)
    return NULL;

  if (!screen_x11->rgba_colormap)
    screen_x11->rgba_colormap = bdk_colormap_new (screen_x11->rgba_visual,
						  FALSE);
  
  return screen_x11->rgba_colormap;
}

/**
 * bdk_screen_get_rgba_visual:
 * @screen: a #BdkScreen
 * 
 * Gets a visual to use for creating windows or pixmaps with an
 * alpha channel. See the docs for bdk_screen_get_rgba_colormap()
 * for caveats.
 * 
 * Return value: (transfer none): a visual to use for windows with an
 *     alpha channel or %NULL if the capability is not available.
 *
 * Since: 2.8
 **/
BdkVisual *
bdk_screen_get_rgba_visual (BdkScreen *screen)
{
  BdkScreenX11 *screen_x11;

  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);

  screen_x11 = BDK_SCREEN_X11 (screen);

  return screen_x11->rgba_visual;
}

/**
 * bdk_x11_screen_get_xscreen:
 * @screen: a #BdkScreen.
 * @returns: (transfer none): an Xlib <type>Screen*</type>
 *
 * Returns the screen of a #BdkScreen.
 *
 * Since: 2.2
 */
Screen *
bdk_x11_screen_get_xscreen (BdkScreen *screen)
{
  return BDK_SCREEN_X11 (screen)->xscreen;
}

/**
 * bdk_x11_screen_get_screen_number:
 * @screen: a #BdkScreen.
 * @returns: the position of @screen among the screens of
 *   its display.
 *
 * Returns the index of a #BdkScreen.
 *
 * Since: 2.2
 */
int
bdk_x11_screen_get_screen_number (BdkScreen *screen)
{
  return BDK_SCREEN_X11 (screen)->screen_num;
}

static gboolean
check_is_composited (BdkDisplay *display,
		     BdkScreenX11 *screen_x11)
{
  Atom xselection = bdk_x11_atom_to_xatom_for_display (display, screen_x11->cm_selection_atom);
  Window xwindow;
  
  xwindow = XGetSelectionOwner (BDK_DISPLAY_XDISPLAY (display), xselection);

  return xwindow != None;
}

static BdkAtom
make_cm_atom (int screen_number)
{
  gchar *name = g_strdup_printf ("_NET_WM_CM_S%d", screen_number);
  BdkAtom atom = bdk_atom_intern (name, FALSE);
  g_free (name);
  return atom;
}

static void
init_monitor_geometry (BdkX11Monitor *monitor,
		       int x, int y, int width, int height)
{
  monitor->geometry.x = x;
  monitor->geometry.y = y;
  monitor->geometry.width = width;
  monitor->geometry.height = height;

  monitor->output = None;
  monitor->width_mm = -1;
  monitor->height_mm = -1;
  monitor->output_name = NULL;
  monitor->manufacturer = NULL;
}

static gboolean
init_fake_xinerama (BdkScreen *screen)
{
#ifdef G_ENABLE_DEBUG
  BdkScreenX11 *screen_x11 = BDK_SCREEN_X11 (screen);
  XSetWindowAttributes atts;
  Window win;
  gint w, h;

  if (!(_bdk_debug_flags & BDK_DEBUG_XINERAMA))
    return FALSE;
  
  /* Fake Xinerama mode by splitting the screen into 4 monitors.
   * Also draw a little cross to make the monitor boundaries visible.
   */
  w = WidthOfScreen (screen_x11->xscreen);
  h = HeightOfScreen (screen_x11->xscreen);

  screen_x11->n_monitors = 4;
  screen_x11->monitors = g_new0 (BdkX11Monitor, 4);
  init_monitor_geometry (&screen_x11->monitors[0], 0, 0, w / 2, h / 2);
  init_monitor_geometry (&screen_x11->monitors[1], w / 2, 0, w / 2, h / 2);
  init_monitor_geometry (&screen_x11->monitors[2], 0, h / 2, w / 2, h / 2);
  init_monitor_geometry (&screen_x11->monitors[3], w / 2, h / 2, w / 2, h / 2);
  
  atts.override_redirect = 1;
  atts.background_pixel = WhitePixel(BDK_SCREEN_XDISPLAY (screen), 
				     screen_x11->screen_num);
  win = XCreateWindow(BDK_SCREEN_XDISPLAY (screen), 
		      screen_x11->xroot_window, 0, h / 2, w, 1, 0, 
		      DefaultDepth(BDK_SCREEN_XDISPLAY (screen), 
				   screen_x11->screen_num),
		      InputOutput, 
		      DefaultVisual(BDK_SCREEN_XDISPLAY (screen), 
				    screen_x11->screen_num),
		      CWOverrideRedirect|CWBackPixel, 
		      &atts);
  XMapRaised(BDK_SCREEN_XDISPLAY (screen), win); 
  win = XCreateWindow(BDK_SCREEN_XDISPLAY (screen), 
		      screen_x11->xroot_window, w/2 , 0, 1, h, 0, 
		      DefaultDepth(BDK_SCREEN_XDISPLAY (screen), 
				   screen_x11->screen_num),
		      InputOutput, 
		      DefaultVisual(BDK_SCREEN_XDISPLAY (screen), 
				    screen_x11->screen_num),
		      CWOverrideRedirect|CWBackPixel, 
		      &atts);
  XMapRaised(BDK_SCREEN_XDISPLAY (screen), win);
  return TRUE;
#endif
  
  return FALSE;
}

static void
free_monitors (BdkX11Monitor *monitors,
               gint           n_monitors)
{
  int i;

  for (i = 0; i < n_monitors; ++i)
    {
      g_free (monitors[i].output_name);
      g_free (monitors[i].manufacturer);
    }

  g_free (monitors);
}

#ifdef HAVE_RANDR
static int
monitor_compare_function (BdkX11Monitor *monitor1,
                          BdkX11Monitor *monitor2)
{
  /* Sort the leftmost/topmost monitors first.
   * For "cloned" monitors, sort the bigger ones first
   * (giving preference to taller monitors over wider
   * monitors)
   */

  if (monitor1->geometry.x != monitor2->geometry.x)
    return monitor1->geometry.x - monitor2->geometry.x;

  if (monitor1->geometry.y != monitor2->geometry.y)
    return monitor1->geometry.y - monitor2->geometry.y;

  if (monitor1->geometry.height != monitor2->geometry.height)
    return - (monitor1->geometry.height - monitor2->geometry.height);

  if (monitor1->geometry.width != monitor2->geometry.width)
    return - (monitor1->geometry.width - monitor2->geometry.width);

  return 0;
}
#endif

#ifdef HAVE_RANDR15
static gboolean
init_randr15 (BdkScreen *screen)
{
  BdkDisplay *display = bdk_screen_get_display (screen);
  BdkDisplayX11 *display_x11 = BDK_DISPLAY_X11 (display);
  BdkScreenX11 *x11_screen = BDK_SCREEN_X11 (screen);
  XRRMonitorInfo *rr_monitors;
  int num_rr_monitors;
  int i;
  GArray *monitors;
  XID primary_output = None;

  if (!display_x11->have_randr15)
    return FALSE;

  rr_monitors = XRRGetMonitors (x11_screen->xdisplay,
                                x11_screen->xroot_window,
                                True,
                                &num_rr_monitors);
  if (!rr_monitors)
    return FALSE;

  monitors = g_array_sized_new (FALSE, TRUE, sizeof (BdkX11Monitor),
                                num_rr_monitors);
  for (i = 0; i < num_rr_monitors; i++)
    {
      BdkX11Monitor monitor;
      init_monitor_geometry (&monitor,
                             rr_monitors[i].x,
                             rr_monitors[i].y,
                             rr_monitors[i].width,
                             rr_monitors[i].height);

      monitor.width_mm = rr_monitors[i].mwidth;
      monitor.height_mm = rr_monitors[i].mheight;
      monitor.output = rr_monitors[i].outputs[0];
      if (rr_monitors[i].primary)
        primary_output = monitor.output;

      g_array_append_val (monitors, monitor);
    }
  XRRFreeMonitors (rr_monitors);

  g_array_sort (monitors,
                (GCompareFunc) monitor_compare_function);
  x11_screen->n_monitors = monitors->len;
  x11_screen->monitors = (BdkX11Monitor *) g_array_free (monitors, FALSE);

  x11_screen->primary_monitor = 0;

  for (i = 0; i < x11_screen->n_monitors; i++)
    {
      if (x11_screen->monitors[i].output == primary_output)
        {
          x11_screen->primary_monitor = i;
          break;
        }
    }

  return x11_screen->n_monitors > 0;
}
#endif

static gboolean
init_randr13 (BdkScreen *screen)
{
#ifdef HAVE_RANDR
  BdkDisplay *display = bdk_screen_get_display (screen);
  BdkDisplayX11 *display_x11 = BDK_DISPLAY_X11 (display);
  BdkScreenX11 *screen_x11 = BDK_SCREEN_X11 (screen);
  Display *dpy = BDK_SCREEN_XDISPLAY (screen);
  XRRScreenResources *resources;
  RROutput primary_output;
  RROutput first_output = None;
  int i;
  GArray *monitors;
  gboolean randr12_compat = FALSE;

  if (!display_x11->have_randr13)
      return FALSE;

  resources = XRRGetScreenResourcesCurrent (screen_x11->xdisplay,
				            screen_x11->xroot_window);
  if (!resources)
    return FALSE;

  monitors = g_array_sized_new (FALSE, TRUE, sizeof (BdkX11Monitor),
                                resources->noutput);

  for (i = 0; i < resources->noutput; ++i)
    {
      XRROutputInfo *output =
	XRRGetOutputInfo (dpy, resources, resources->outputs[i]);

      /* Non RandR1.2 X driver have output name "default" */
      randr12_compat |= !g_strcmp0 (output->name, "default");

      if (output->connection == RR_Disconnected)
        {
          XRRFreeOutputInfo (output);
          continue;
        }

      if (output->crtc)
	{
	  BdkX11Monitor monitor;
	  XRRCrtcInfo *crtc = XRRGetCrtcInfo (dpy, resources, output->crtc);

	  monitor.geometry.x = crtc->x;
	  monitor.geometry.y = crtc->y;
	  monitor.geometry.width = crtc->width;
	  monitor.geometry.height = crtc->height;

	  monitor.output = resources->outputs[i];
	  monitor.width_mm = output->mm_width;
	  monitor.height_mm = output->mm_height;
	  monitor.output_name = g_strdup (output->name);
	  /* FIXME: need EDID parser */
	  monitor.manufacturer = NULL;

	  g_array_append_val (monitors, monitor);

          XRRFreeCrtcInfo (crtc);
	}

      XRRFreeOutputInfo (output);
    }

  if (resources->noutput > 0)
    first_output = resources->outputs[0];

  XRRFreeScreenResources (resources);

  /* non RandR 1.2 X driver doesn't return any usable multihead data */
  if (randr12_compat)
    {
      guint n_monitors = monitors->len;

      free_monitors ((BdkX11Monitor *)g_array_free (monitors, FALSE),
		     n_monitors);

      return FALSE;
    }

  g_array_sort (monitors,
                (GCompareFunc) monitor_compare_function);
  screen_x11->n_monitors = monitors->len;
  screen_x11->monitors = (BdkX11Monitor *)g_array_free (monitors, FALSE);

  screen_x11->primary_monitor = 0;

  primary_output = XRRGetOutputPrimary (screen_x11->xdisplay,
                                        screen_x11->xroot_window);

  for (i = 0; i < screen_x11->n_monitors; ++i)
    {
      if (screen_x11->monitors[i].output == primary_output)
	{
	  screen_x11->primary_monitor = i;
	  break;
	}

      /* No RandR1.3+ available or no primary set, fall back to prefer LVDS as primary if present */
      if (primary_output == None &&
          g_ascii_strncasecmp (screen_x11->monitors[i].output_name, "LVDS", 4) == 0)
	{
	  screen_x11->primary_monitor = i;
	  break;
	}

      /* No primary specified and no LVDS found */
      if (screen_x11->monitors[i].output == first_output)
	screen_x11->primary_monitor = i;
    }

  return screen_x11->n_monitors > 0;
#endif

  return FALSE;
}

static gboolean
init_solaris_xinerama (BdkScreen *screen)
{
#ifdef HAVE_SOLARIS_XINERAMA
  Display *dpy = BDK_SCREEN_XDISPLAY (screen);
  int screen_no = bdk_screen_get_number (screen);
  BdkScreenX11 *screen_x11 = BDK_SCREEN_X11 (screen);
  XRectangle monitors[MAXFRAMEBUFFERS];
  unsigned char hints[16];
  gint result;
  int n_monitors;
  int i;
  
  if (!XineramaGetState (dpy, screen_no))
    return FALSE;

  result = XineramaGetInfo (dpy, screen_no, monitors, hints, &n_monitors);

  /* Yes I know it should be Success but the current implementation 
   * returns the num of monitor
   */
  if (result == 0)
    {
      return FALSE;
    }

  screen_x11->monitors = g_new0 (BdkX11Monitor, n_monitors);
  screen_x11->n_monitors = n_monitors;

  for (i = 0; i < n_monitors; i++)
    {
      init_monitor_geometry (&screen_x11->monitors[i],
			     monitors[i].x, monitors[i].y,
			     monitors[i].width, monitors[i].height);
    }

  screen_x11->primary_monitor = 0;

  return TRUE;
#endif /* HAVE_SOLARIS_XINERAMA */

  return FALSE;
}

static gboolean
init_xfree_xinerama (BdkScreen *screen)
{
#ifdef HAVE_XFREE_XINERAMA
  Display *dpy = BDK_SCREEN_XDISPLAY (screen);
  BdkScreenX11 *screen_x11 = BDK_SCREEN_X11 (screen);
  XineramaScreenInfo *monitors;
  int i, n_monitors;
  
  if (!XineramaIsActive (dpy))
    return FALSE;

  monitors = XineramaQueryScreens (dpy, &n_monitors);
  
  if (n_monitors <= 0 || monitors == NULL)
    {
      /* If Xinerama doesn't think we have any monitors, try acting as
       * though we had no Xinerama. If the "no monitors" condition
       * is because XRandR 1.2 is currently switching between CRTCs,
       * we'll be notified again when we have our monitor back,
       * and can go back into Xinerama-ish mode at that point.
       */
      if (monitors)
	XFree (monitors);
      
      return FALSE;
    }

  screen_x11->n_monitors = n_monitors;
  screen_x11->monitors = g_new0 (BdkX11Monitor, n_monitors);
  
  for (i = 0; i < n_monitors; ++i)
    {
      init_monitor_geometry (&screen_x11->monitors[i],
			     monitors[i].x_org, monitors[i].y_org,
			     monitors[i].width, monitors[i].height);
    }
  
  XFree (monitors);
  
  screen_x11->primary_monitor = 0;

  return TRUE;
#endif /* HAVE_XFREE_XINERAMA */
  
  return FALSE;
}

static void
deinit_multihead (BdkScreen *screen)
{
  BdkScreenX11 *screen_x11 = BDK_SCREEN_X11 (screen);

  free_monitors (screen_x11->monitors, screen_x11->n_monitors);

  screen_x11->n_monitors = 0;
  screen_x11->monitors = NULL;
}

static gboolean
compare_monitor (BdkX11Monitor *m1,
                 BdkX11Monitor *m2)
{
  if (m1->geometry.x != m2->geometry.x ||
      m1->geometry.y != m2->geometry.y ||
      m1->geometry.width != m2->geometry.width ||
      m1->geometry.height != m2->geometry.height)
    return FALSE;

  if (m1->width_mm != m2->width_mm ||
      m1->height_mm != m2->height_mm)
    return FALSE;

  if (g_strcmp0 (m1->output_name, m2->output_name) != 0)
    return FALSE;

  if (g_strcmp0 (m1->manufacturer, m2->manufacturer) != 0)
    return FALSE;

  return TRUE;
}

static gboolean
compare_monitors (BdkX11Monitor *monitors1, gint n_monitors1,
                  BdkX11Monitor *monitors2, gint n_monitors2)
{
  gint i;

  if (n_monitors1 != n_monitors2)
    return FALSE;

  for (i = 0; i < n_monitors1; i++)
    {
      if (!compare_monitor (monitors1 + i, monitors2 + i))
        return FALSE;
    }

  return TRUE;
}

static void
init_multihead (BdkScreen *screen)
{
  BdkScreenX11 *screen_x11 = BDK_SCREEN_X11 (screen);
  int opcode, firstevent, firsterror;

  /* There are four different implementations of multihead support: 
   *
   *  1. Fake Xinerama for debugging purposes
   *  2. RandR 1.2
   *  3. Solaris Xinerama
   *  4. XFree86/Xorg Xinerama
   *
   * We use them in that order.
   */
  if (init_fake_xinerama (screen))
    return;

#ifdef HAVE_RANDR15
  if (init_randr15 (screen))
    return;
#endif

  if (init_randr13 (screen))
    return;

  if (XQueryExtension (BDK_SCREEN_XDISPLAY (screen), "XINERAMA",
		       &opcode, &firstevent, &firsterror))
    {
      if (init_solaris_xinerama (screen))
	return;
      
      if (init_xfree_xinerama (screen))
	return;
    }

  /* No multihead support of any kind for this screen */
  screen_x11->n_monitors = 1;
  screen_x11->monitors = g_new0 (BdkX11Monitor, 1);
  screen_x11->primary_monitor = 0;

  init_monitor_geometry (screen_x11->monitors, 0, 0,
			 WidthOfScreen (screen_x11->xscreen),
			 HeightOfScreen (screen_x11->xscreen));
}

BdkScreen *
_bdk_x11_screen_new (BdkDisplay *display,
		     gint	 screen_number) 
{
  BdkScreen *screen;
  BdkScreenX11 *screen_x11;
  BdkDisplayX11 *display_x11 = BDK_DISPLAY_X11 (display);

  screen = g_object_new (BDK_TYPE_SCREEN_X11, NULL);

  screen_x11 = BDK_SCREEN_X11 (screen);
  screen_x11->display = display;
  screen_x11->xdisplay = display_x11->xdisplay;
  screen_x11->xscreen = ScreenOfDisplay (display_x11->xdisplay, screen_number);
  screen_x11->screen_num = screen_number;
  screen_x11->xroot_window = RootWindow (display_x11->xdisplay,screen_number);
  screen_x11->wmspec_check_window = None;
  /* we want this to be always non-null */
  screen_x11->window_manager_name = g_strdup ("unknown");
  
  init_multihead (screen);
  init_randr_support (screen);
  
  _bdk_visual_init (screen);
  _bdk_windowing_window_init (screen);
  
  return screen;
}

/*
 * It is important that we first request the selection
 * notification, and then setup the initial state of
 * is_composited to avoid a race condition here.
 */
void
_bdk_x11_screen_setup (BdkScreen *screen)
{
  BdkScreenX11 *screen_x11 = BDK_SCREEN_X11 (screen);

  screen_x11->cm_selection_atom = make_cm_atom (screen_x11->screen_num);
  bdk_display_request_selection_notification (screen_x11->display,
					      screen_x11->cm_selection_atom);
  screen_x11->is_composited = check_is_composited (screen_x11->display, screen_x11);
}

/**
 * bdk_screen_is_composited:
 * @screen: a #BdkScreen
 * 
 * Returns whether windows with an RGBA visual can reasonably
 * be expected to have their alpha channel drawn correctly on
 * the screen.
 *
 * On X11 this function returns whether a compositing manager is
 * compositing @screen.
 * 
 * Return value: Whether windows with RGBA visuals can reasonably be
 * expected to have their alpha channels drawn correctly on the screen.
 * 
 * Since: 2.10
 **/
gboolean
bdk_screen_is_composited (BdkScreen *screen)
{
  BdkScreenX11 *screen_x11;

  g_return_val_if_fail (BDK_IS_SCREEN (screen), FALSE);

  screen_x11 = BDK_SCREEN_X11 (screen);

  return screen_x11->is_composited;
}

static void
init_randr_support (BdkScreen * screen)
{
  BdkScreenX11 *screen_x11 = BDK_SCREEN_X11 (screen);
  
  XSelectInput (BDK_SCREEN_XDISPLAY (screen),
		screen_x11->xroot_window,
		StructureNotifyMask);

#ifdef HAVE_RANDR
  XRRSelectInput (BDK_SCREEN_XDISPLAY (screen),
		  screen_x11->xroot_window,
		  RRScreenChangeNotifyMask	|
		  RRCrtcChangeNotifyMask	|
		  RROutputPropertyNotifyMask);
#endif
}

static void
process_monitors_change (BdkScreen *screen)
{
  BdkScreenX11 *screen_x11 = BDK_SCREEN_X11 (screen);
  gint		 n_monitors;
  gint		 primary_monitor;
  BdkX11Monitor	*monitors;
  gboolean changed;

  primary_monitor = screen_x11->primary_monitor;
  n_monitors = screen_x11->n_monitors;
  monitors = screen_x11->monitors;

  screen_x11->n_monitors = 0;
  screen_x11->monitors = NULL;

  init_multihead (screen);

  changed =
    !compare_monitors (monitors, n_monitors,
		       screen_x11->monitors, screen_x11->n_monitors) ||
    screen_x11->primary_monitor != primary_monitor;

  free_monitors (monitors, n_monitors);

  if (changed)
    g_signal_emit_by_name (screen, "monitors-changed");
}

void
_bdk_x11_screen_size_changed (BdkScreen *screen,
			      XEvent    *event)
{
  gint width, height;
#ifdef HAVE_RANDR
  BdkDisplayX11 *display_x11;
#endif

  width = bdk_screen_get_width (screen);
  height = bdk_screen_get_height (screen);

#ifdef HAVE_RANDR
  display_x11 = BDK_DISPLAY_X11 (bdk_screen_get_display (screen));

  if (display_x11->have_randr13 && event->type == ConfigureNotify)
    return;

  XRRUpdateConfiguration (event);
#else
  if (event->type == ConfigureNotify)
    {
      XConfigureEvent *rcevent = (XConfigureEvent *) event;
      Screen	    *xscreen = bdk_x11_screen_get_xscreen (screen);

      xscreen->width   = rcevent->width;
      xscreen->height  = rcevent->height;
    }
  else
    return;
#endif

  process_monitors_change (screen);

  if (width != bdk_screen_get_width (screen) ||
      height != bdk_screen_get_height (screen))
    g_signal_emit_by_name (screen, "size-changed");
}

void
_bdk_x11_screen_window_manager_changed (BdkScreen *screen)
{
  g_signal_emit (screen, signals[WINDOW_MANAGER_CHANGED], 0);
}

void
_bdk_x11_screen_process_owner_change (BdkScreen *screen,
				      XEvent *event)
{
#ifdef HAVE_XFIXES
  XFixesSelectionNotifyEvent *selection_event = (XFixesSelectionNotifyEvent *)event;
  BdkScreenX11 *screen_x11 = BDK_SCREEN_X11 (screen);
  Atom xcm_selection_atom = bdk_x11_atom_to_xatom_for_display (screen_x11->display,
							       screen_x11->cm_selection_atom);

  if (selection_event->selection == xcm_selection_atom)
    {
      gboolean composited = selection_event->owner != None;

      if (composited != screen_x11->is_composited)
	{
	  screen_x11->is_composited = composited;

	  g_signal_emit_by_name (screen, "composited-changed");
	}
    }
#endif
}

/**
 * _bdk_windowing_substitute_screen_number:
 * @display_name : The name of a display, in the form used by 
 *                 bdk_display_open (). If %NULL a default value
 *                 will be used. On X11, this is derived from the DISPLAY
 *                 environment variable.
 * @screen_number : The number of a screen within the display
 *                  referred to by @display_name.
 *
 * Modifies a @display_name to make @screen_number the default
 * screen when the display is opened.
 *
 * Return value: a newly allocated string holding the resulting
 *   display name. Free with g_free().
 */
gchar * 
_bdk_windowing_substitute_screen_number (const gchar *display_name,
					 gint         screen_number)
{
  GString *str;
  gchar   *p;

  if (!display_name)
    display_name = getenv ("DISPLAY");

  if (!display_name)
    return NULL;

  str = g_string_new (display_name);

  p = strrchr (str->str, '.');
  if (p && p >	strchr (str->str, ':'))
    g_string_truncate (str, p - str->str);

  g_string_append_printf (str, ".%d", screen_number);

  return g_string_free (str, FALSE);
}

/**
 * bdk_screen_make_display_name:
 * @screen: a #BdkScreen
 * 
 * Determines the name to pass to bdk_display_open() to get
 * a #BdkDisplay with this screen as the default screen.
 * 
 * Return value: a newly allocated string, free with g_free()
 *
 * Since: 2.2
 **/
gchar *
bdk_screen_make_display_name (BdkScreen *screen)
{
  const gchar *old_display;

  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);

  old_display = bdk_display_get_name (bdk_screen_get_display (screen));

  return _bdk_windowing_substitute_screen_number (old_display, 
						  bdk_screen_get_number (screen));
}

/**
 * bdk_screen_get_active_window
 * @screen: a #BdkScreen
 *
 * Returns the screen's currently active window.
 *
 * On X11, this is done by inspecting the _NET_ACTIVE_WINDOW property
 * on the root window, as described in the <ulink
 * url="http://www.freedesktop.org/Standards/wm-spec">Extended Window
 * Manager Hints</ulink>. If there is no currently currently active
 * window, or the window manager does not support the
 * _NET_ACTIVE_WINDOW hint, this function returns %NULL.
 *
 * On other platforms, this function may return %NULL, depending on whether
 * it is implementable on that platform.
 *
 * The returned window should be unrefed using g_object_unref() when
 * no longer needed.
 *
 * Return value: the currently active window, or %NULL.
 *
 * Since: 2.10
 **/
BdkWindow *
bdk_screen_get_active_window (BdkScreen *screen)
{
  BdkScreenX11 *screen_x11;
  BdkWindow *ret = NULL;
  Atom type_return;
  gint format_return;
  gulong nitems_return;
  gulong bytes_after_return;
  guchar *data = NULL;

  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);

  if (!bdk_x11_screen_supports_net_wm_hint (screen,
                                            bdk_atom_intern_static_string ("_NET_ACTIVE_WINDOW")))
    return NULL;

  screen_x11 = BDK_SCREEN_X11 (screen);

  if (XGetWindowProperty (screen_x11->xdisplay, screen_x11->xroot_window,
	                  bdk_x11_get_xatom_by_name_for_display (screen_x11->display,
			                                         "_NET_ACTIVE_WINDOW"),
		          0, 1, False, XA_WINDOW, &type_return,
		          &format_return, &nitems_return,
                          &bytes_after_return, &data)
      == Success)
    {
      if ((type_return == XA_WINDOW) && (format_return == 32) && (data))
        {
          BdkNativeWindow window = *(BdkNativeWindow *) data;

          if (window != None)
            {
              ret = bdk_window_foreign_new_for_display (screen_x11->display,
                                                        *(BdkNativeWindow *) data);
            }
        }
    }

  if (data)
    XFree (data);

  return ret;
}

/**
 * bdk_screen_get_window_stack:
 * @screen: a #BdkScreen
 *
 * Returns a #GList of #BdkWindow<!-- -->s representing the current
 * window stack.
 *
 * On X11, this is done by inspecting the _NET_CLIENT_LIST_STACKING
 * property on the root window, as described in the <ulink
 * url="http://www.freedesktop.org/Standards/wm-spec">Extended Window
 * Manager Hints</ulink>. If the window manager does not support the
 * _NET_CLIENT_LIST_STACKING hint, this function returns %NULL.
 *
 * On other platforms, this function may return %NULL, depending on whether
 * it is implementable on that platform.
 *
 * The returned list is newly allocated and owns references to the
 * windows it contains, so it should be freed using g_list_free() and
 * its windows unrefed using g_object_unref() when no longer needed.
 *
 * Return value: (transfer full) (element-type BdkWindow):
 *     a list of #BdkWindow<!-- -->s for the current window stack,
 *               or %NULL.
 *
 * Since: 2.10
 **/
GList *
bdk_screen_get_window_stack (BdkScreen *screen)
{
  BdkScreenX11 *screen_x11;
  GList *ret = NULL;
  Atom type_return;
  gint format_return;
  gulong nitems_return;
  gulong bytes_after_return;
  guchar *data = NULL;

  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);

  if (!bdk_x11_screen_supports_net_wm_hint (screen,
                                            bdk_atom_intern_static_string ("_NET_CLIENT_LIST_STACKING")))
    return NULL;

  screen_x11 = BDK_SCREEN_X11 (screen);

  if (XGetWindowProperty (screen_x11->xdisplay, screen_x11->xroot_window,
	                  bdk_x11_get_xatom_by_name_for_display (screen_x11->display,
			                                         "_NET_CLIENT_LIST_STACKING"),
		          0, G_MAXLONG, False, XA_WINDOW, &type_return,
		          &format_return, &nitems_return,
                          &bytes_after_return, &data)
      == Success)
    {
      if ((type_return == XA_WINDOW) && (format_return == 32) &&
          (data) && (nitems_return > 0))
        {
          gulong *stack = (gulong *) data;
          BdkWindow *win;
          int i;

          for (i = 0; i < nitems_return; i++)
            {
              win = bdk_window_foreign_new_for_display (screen_x11->display,
                                                        (BdkNativeWindow)stack[i]);

              if (win != NULL)
                ret = g_list_append (ret, win);
            }
        }
    }

  if (data)
    XFree (data);

  return ret;
}

#define __BDK_SCREEN_X11_C__
#include "bdkaliasdef.c"
