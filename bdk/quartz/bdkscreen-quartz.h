/* bdkscreen-quartz.h
 *
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

#ifndef __BDK_SCREEN_QUARTZ_H__
#define __BDK_SCREEN_QUARTZ_H__

B_BEGIN_DECLS

typedef struct _BdkScreenQuartz BdkScreenQuartz;
typedef struct _BdkScreenQuartzClass BdkScreenQuartzClass;

#define BDK_TYPE_SCREEN_QUARTZ              (_bdk_screen_quartz_get_type ())
#define BDK_SCREEN_QUARTZ(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_SCREEN_QUARTZ, BdkScreenQuartz))
#define BDK_SCREEN_QUARTZ_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_SCREEN_QUARTZ, BdkScreenQuartzClass))
#define BDK_IS_SCREEN_QUARTZ(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_SCREEN_QUARTZ))
#define BDK_IS_SCREEN_QUARTZ_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_SCREEN_QUARTZ))
#define BDK_SCREEN_QUARTZ_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_SCREEN_QUARTZ, BdkScreenQuartzClass))

struct _BdkScreenQuartz
{
  BdkScreen parent_instance;

  BdkDisplay *display;
  BdkColormap *default_colormap;

  /* Origin of "root window" in Cocoa coordinates */
  gint min_x;
  gint min_y;

  gint width;
  gint height;

  int n_screens;
  BdkRectangle *screen_rects;

  guint screen_changed_id;

  guint emit_monitors_changed : 1;
};

struct _BdkScreenQuartzClass
{
  BdkScreenClass parent_class;
};

GType      _bdk_screen_quartz_get_type (void);
BdkScreen *_bdk_screen_quartz_new      (void);

B_END_DECLS

#endif /* _BDK_SCREEN_QUARTZ_H_ */
