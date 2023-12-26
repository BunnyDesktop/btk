/*
 * bdkscreen.h
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

#ifndef __BDK_SCREEN_H__
#define __BDK_SCREEN_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bairo.h>
#include "bdk/bdktypes.h"
#include "bdk/bdkdisplay.h"

B_BEGIN_DECLS

typedef struct _BdkScreenClass BdkScreenClass;

#define BDK_TYPE_SCREEN            (bdk_screen_get_type ())
#define BDK_SCREEN(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_SCREEN, BdkScreen))
#define BDK_SCREEN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_SCREEN, BdkScreenClass))
#define BDK_IS_SCREEN(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_SCREEN))
#define BDK_IS_SCREEN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_SCREEN))
#define BDK_SCREEN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_SCREEN, BdkScreenClass))

struct _BdkScreen
{
  GObject parent_instance;

  guint GSEAL (closed) : 1;

  BdkGC *GSEAL (normal_gcs[32]);
  BdkGC *GSEAL (exposure_gcs[32]);
  BdkGC *GSEAL (subwindow_gcs[32]);

  bairo_font_options_t *GSEAL (font_options);
  double GSEAL (resolution);	/* pixels/points scale factor for fonts */
};

struct _BdkScreenClass
{
  GObjectClass parent_class;

  void (*size_changed) (BdkScreen *screen);
  void (*composited_changed) (BdkScreen *screen);
  void (*monitors_changed) (BdkScreen *screen);
};

GType        bdk_screen_get_type              (void) B_GNUC_CONST;
BdkColormap *bdk_screen_get_default_colormap  (BdkScreen   *screen);
void         bdk_screen_set_default_colormap  (BdkScreen   *screen,
					       BdkColormap *colormap);
BdkColormap* bdk_screen_get_system_colormap   (BdkScreen   *screen);
BdkVisual*   bdk_screen_get_system_visual     (BdkScreen   *screen);
BdkColormap *bdk_screen_get_rgb_colormap      (BdkScreen   *screen);
BdkVisual *  bdk_screen_get_rgb_visual        (BdkScreen   *screen);
BdkColormap *bdk_screen_get_rgba_colormap     (BdkScreen   *screen);
BdkVisual *  bdk_screen_get_rgba_visual       (BdkScreen   *screen);
gboolean     bdk_screen_is_composited	      (BdkScreen   *screen);

BdkWindow *  bdk_screen_get_root_window       (BdkScreen   *screen);
BdkDisplay * bdk_screen_get_display           (BdkScreen   *screen);
gint         bdk_screen_get_number            (BdkScreen   *screen);
gint         bdk_screen_get_width             (BdkScreen   *screen);
gint         bdk_screen_get_height            (BdkScreen   *screen);
gint         bdk_screen_get_width_mm          (BdkScreen   *screen);
gint         bdk_screen_get_height_mm         (BdkScreen   *screen);

GList *      bdk_screen_list_visuals          (BdkScreen   *screen);
GList *      bdk_screen_get_toplevel_windows  (BdkScreen   *screen);
gchar *      bdk_screen_make_display_name     (BdkScreen   *screen);

gint          bdk_screen_get_n_monitors        (BdkScreen *screen);
gint          bdk_screen_get_primary_monitor   (BdkScreen *screen);
void          bdk_screen_get_monitor_geometry  (BdkScreen *screen,
						gint       monitor_num,
						BdkRectangle *dest);
gint          bdk_screen_get_monitor_at_point  (BdkScreen *screen,
						gint       x,
						gint       y);
gint          bdk_screen_get_monitor_at_window (BdkScreen *screen,
						BdkWindow *window);
gint          bdk_screen_get_monitor_width_mm  (BdkScreen *screen,
                                                gint       monitor_num);
gint          bdk_screen_get_monitor_height_mm (BdkScreen *screen,
                                                gint       monitor_num);
gchar *       bdk_screen_get_monitor_plug_name (BdkScreen *screen,
                                                gint       monitor_num);

void          bdk_screen_broadcast_client_message  (BdkScreen       *screen,
						    BdkEvent        *event);

BdkScreen *bdk_screen_get_default (void);

gboolean   bdk_screen_get_setting (BdkScreen   *screen,
				   const gchar *name,
				   GValue      *value);

void                        bdk_screen_set_font_options (BdkScreen                  *screen,
							 const bairo_font_options_t *options);
const bairo_font_options_t *bdk_screen_get_font_options (BdkScreen                  *screen);

void    bdk_screen_set_resolution (BdkScreen *screen,
				   gdouble    dpi);
gdouble bdk_screen_get_resolution (BdkScreen *screen);

BdkWindow *bdk_screen_get_active_window (BdkScreen *screen);
GList     *bdk_screen_get_window_stack  (BdkScreen *screen);

B_END_DECLS

#endif				/* __BDK_SCREEN_H__ */
