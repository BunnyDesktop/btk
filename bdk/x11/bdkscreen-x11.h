/*
 * bdkscreen-x11.h
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

#ifndef __BDK_SCREEN_X11_H__
#define __BDK_SCREEN_X11_H__

#include "bdkprivate-x11.h"
#include "xsettings-client.h"
#include <bdk/bdkscreen.h>
#include <bdk/bdkvisual.h>
#include <X11/X.h>
#include <X11/Xlib.h>

B_BEGIN_DECLS
  
typedef struct _BdkScreenX11 BdkScreenX11;
typedef struct _BdkScreenX11Class BdkScreenX11Class;

#define BDK_TYPE_SCREEN_X11              (_bdk_screen_x11_get_type ())
#define BDK_SCREEN_X11(object)           (B_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_SCREEN_X11, BdkScreenX11))
#define BDK_SCREEN_X11_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_SCREEN_X11, BdkScreenX11Class))
#define BDK_IS_SCREEN_X11(object)        (B_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_SCREEN_X11))
#define BDK_IS_SCREEN_X11_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_SCREEN_X11))
#define BDK_SCREEN_X11_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_SCREEN_X11, BdkScreenX11Class))

typedef struct _BdkX11Monitor BdkX11Monitor;

struct _BdkScreenX11
{
  BdkScreen parent_instance;
  
  BdkDisplay *display;
  Display *xdisplay;
  Screen *xscreen;
  bint screen_num;
  Window xroot_window;
  BdkWindow *root_window;

  /* Window manager */
  long last_wmspec_check_time;
  Window wmspec_check_window;
  char *window_manager_name;
  /* TRUE if wmspec_check_window has changed since last
   * fetch of _NET_SUPPORTED
   */
  buint need_refetch_net_supported : 1;
  /* TRUE if wmspec_check_window has changed since last
   * fetch of window manager name
   */
  buint need_refetch_wm_name : 1;
  
  /* Visual Part */
  BdkVisualPrivate *system_visual;
  BdkVisualPrivate **visuals;
  bint nvisuals;
  bint available_depths[7];
  bint navailable_depths;
  BdkVisualType available_types[6];
  bint navailable_types;
  GHashTable *visual_hash;
  GHashTable *colormap_hash;
  BdkVisual *rgba_visual;
  
  /* Colormap Part */
  BdkColormap *default_colormap;
  BdkColormap *system_colormap;
  BdkColormap *rgba_colormap;

  /* X settings */
  XSettingsClient *xsettings_client;
  buint xsettings_in_init : 1;
  
  /* Xinerama/RandR 1.2 */
  bint		 n_monitors;
  BdkX11Monitor	*monitors;
  bint           primary_monitor;

  /* Bango renderer object singleton */
  BangoRenderer *renderer;

  /* Xft resources for the display, used for default values for
   * the Xft/ XSETTINGS
   */
  bboolean xft_init;		/* Whether we've intialized these values yet */
  bboolean xft_antialias;
  bboolean xft_hinting;
  bint xft_hintstyle;
  bint xft_rgba;
  bint xft_dpi;

  BdkAtom cm_selection_atom;
  bboolean is_composited;
};
  
struct _BdkScreenX11Class
{
  BdkScreenClass parent_class;

  void (* window_manager_changed) (BdkScreenX11 *screen_x11);
};

GType       _bdk_screen_x11_get_type (void);
BdkScreen * _bdk_x11_screen_new      (BdkDisplay *display,
				      bint	  screen_number);

void _bdk_x11_screen_setup                  (BdkScreen *screen);
void _bdk_x11_screen_window_manager_changed (BdkScreen *screen);
void _bdk_x11_screen_size_changed           (BdkScreen *screen,
					     XEvent    *event);
void _bdk_x11_screen_process_owner_change   (BdkScreen *screen,
					     XEvent    *event);

B_END_DECLS

#endif /* __BDK_SCREEN_X11_H__ */
