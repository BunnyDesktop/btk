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

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#ifndef __BDK_WINDOW_IMPL_H__
#define __BDK_WINDOW_IMPL_H__

#include <bdk/bdkwindow.h>

G_BEGIN_DECLS

#define BDK_TYPE_WINDOW_IMPL           (bdk_window_impl_get_type ())
#define BDK_WINDOW_IMPL(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), BDK_TYPE_WINDOW_IMPL, BdkWindowImpl))
#define BDK_IS_WINDOW_IMPL(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BDK_TYPE_WINDOW_IMPL))
#define BDK_WINDOW_IMPL_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BDK_TYPE_WINDOW_IMPL, BdkWindowImplIface))

typedef struct _BdkWindowImpl       BdkWindowImpl;      /* dummy */
typedef struct _BdkWindowImplIface  BdkWindowImplIface;

struct _BdkWindowImplIface
{
  GTypeInterface g_iface;

  void         (* show)                 (BdkWindow       *window,
					 gboolean         already_mapped);
  void         (* hide)                 (BdkWindow       *window);
  void         (* withdraw)             (BdkWindow       *window);
  void         (* raise)                (BdkWindow       *window);
  void         (* lower)                (BdkWindow       *window);
  void         (* restack_under)        (BdkWindow       *window,
					 GList           *native_siblings);
  void         (* restack_toplevel)     (BdkWindow       *window,
					 BdkWindow       *sibling,
					 gboolean        above);

  void         (* move_resize)          (BdkWindow       *window,
                                         gboolean         with_move,
                                         gint             x,
                                         gint             y,
                                         gint             width,
                                         gint             height);
  void         (* set_background)       (BdkWindow       *window,
                                         const BdkColor  *color);
  void         (* set_back_pixmap)      (BdkWindow       *window,
                                         BdkPixmap       *pixmap);

  BdkEventMask (* get_events)           (BdkWindow       *window);
  void         (* set_events)           (BdkWindow       *window,
                                         BdkEventMask     event_mask);
  
  gboolean     (* reparent)             (BdkWindow       *window,
                                         BdkWindow       *new_parent,
                                         gint             x,
                                         gint             y);
  void         (* clear_rebunnyion)         (BdkWindow       *window,
					 BdkRebunnyion       *rebunnyion,
					 gboolean         send_expose);
  
  void         (* set_cursor)           (BdkWindow       *window,
                                         BdkCursor       *cursor);

  void         (* get_geometry)         (BdkWindow       *window,
                                         gint            *x,
                                         gint            *y,
                                         gint            *width,
                                         gint            *height,
                                         gint            *depth);
  gint         (* get_root_coords)      (BdkWindow       *window,
					 gint             x,
					 gint             y,
                                         gint            *root_x,
                                         gint            *root_y);
  gint         (* get_deskrelative_origin) (BdkWindow       *window,
                                         gint            *x,
                                         gint            *y);
  gboolean     (* get_pointer)          (BdkWindow       *window,
                                         gint            *x,
                                         gint            *y,
					 BdkModifierType  *mask);

  void         (* shape_combine_rebunnyion) (BdkWindow       *window,
                                         const BdkRebunnyion *shape_rebunnyion,
                                         gint             offset_x,
                                         gint             offset_y);
  void         (* input_shape_combine_rebunnyion) (BdkWindow       *window,
					       const BdkRebunnyion *shape_rebunnyion,
					       gint             offset_x,
					       gint             offset_y);

  gboolean     (* set_static_gravities) (BdkWindow       *window,
				         gboolean         use_static);

  /* Called before processing updates for a window. This gives the windowing
   * layer a chance to save the rebunnyion for later use in avoiding duplicate
   * exposes. The return value indicates whether the function has a saved
   * the rebunnyion; if the result is TRUE, then the windowing layer is responsible
   * for destroying the rebunnyion later.
   */
  gboolean     (* queue_antiexpose)     (BdkWindow       *window,
					 BdkRebunnyion       *update_area);
  void         (* queue_translation)    (BdkWindow       *window,
					 BdkGC           *gc,
					 BdkRebunnyion       *area,
					 gint            dx,
					 gint            dy);

/* Called to do the windowing system specific part of bdk_window_destroy(),
 *
 * window: The window being destroyed
 * recursing: If TRUE, then this is being called because a parent
 *            was destroyed. This generally means that the call to the windowing system
 *            to destroy the window can be omitted, since it will be destroyed as a result
 *            of the parent being destroyed. Unless @foreign_destroy
 *            
 * foreign_destroy: If TRUE, the window or a parent was destroyed by some external 
 *            agency. The window has already been destroyed and no windowing
 *            system calls should be made. (This may never happen for some
 *            windowing systems.)
 */
  void         (* destroy)              (BdkWindow       *window,
					 gboolean         recursing,
					 gboolean         foreign_destroy);

  void         (* input_window_destroy) (BdkWindow       *window);
  void         (* input_window_crossing)(BdkWindow       *window,
					 gboolean         enter);
  gboolean     supports_native_bg;
};

/* Interface Functions */
GType bdk_window_impl_get_type (void) G_GNUC_CONST;

/* private definitions from bdkwindow.h */

struct _BdkWindowRedirect
{
  BdkWindowObject *redirected;
  BdkDrawable *pixmap;

  gint src_x;
  gint src_y;
  gint dest_x;
  gint dest_y;
  gint width;
  gint height;

  BdkRebunnyion *damage;
  guint damage_idle;
};

G_END_DECLS

#endif /* __BDK_WINDOW_IMPL_H__ */
