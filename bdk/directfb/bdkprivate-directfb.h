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
 * file for a list of people on the BTK+ Team.
 */

/*
 * BTK+ DirectFB backend
 * Copyright (C) 2001-2002  convergence integrated media GmbH
 * Copyright (C) 2002-2004  convergence GmbH
 * Written by Denis Oliver Kropp <dok@convergence.de> and
 *            Sven Neumann <sven@convergence.de>
 */

#ifndef __BDK_PRIVATE_DIRECTFB_H__
#define __BDK_PRIVATE_DIRECTFB_H__

//#include <bdk/bdk.h>
#include <bdk/bdkprivate.h>
#include "bdkinternals.h"
#include "bdkcursor.h"
#include "bdkdisplay-directfb.h"
#include "bdkrebunnyion-generic.h"
#include <bairo.h>

#include <string.h>

#include <directfb_util.h>


#define BDK_TYPE_DRAWABLE_IMPL_DIRECTFB       (bdk_drawable_impl_directfb_get_type ())
#define BDK_DRAWABLE_IMPL_DIRECTFB(object)    (B_TYPE_CHECK_INSTANCE_CAST ((object), \
                                                                           BDK_TYPE_DRAWABLE_IMPL_DIRECTFB, \
                                                                           BdkDrawableImplDirectFB))
#define BDK_IS_DRAWABLE_IMPL_DIRECTFB(object) (B_TYPE_CHECK_INSTANCE_TYPE ((object), \
                                                                           BDK_TYPE_DRAWABLE_IMPL_DIRECTFB))

#define BDK_TYPE_WINDOW_IMPL_DIRECTFB         (bdk_window_impl_directfb_get_type ())
#define BDK_WINDOW_IMPL_DIRECTFB(object)      (B_TYPE_CHECK_INSTANCE_CAST ((object), \
                                                                           BDK_TYPE_WINDOW_IMPL_DIRECTFB, \
                                                                           BdkWindowImplDirectFB))
#define BDK_IS_WINDOW_IMPL_DIRECTFB(object)   (B_TYPE_CHECK_INSTANCE_TYPE ((object), \
                                                                           BDK_TYPE_WINDOW_IMPL_DIRECTFB))

#define BDK_TYPE_PIXMAP_IMPL_DIRECTFB         (bdk_pixmap_impl_directfb_get_type ())
#define BDK_PIXMAP_IMPL_DIRECTFB(object)      (B_TYPE_CHECK_INSTANCE_CAST ((object), \
                                                                           BDK_TYPE_PIXMAP_IMPL_DIRECTFB, \
                                                                           BdkPixmapImplDirectFB))
#define BDK_IS_PIXMAP_IMPL_DIRECTFB(object)   (B_TYPE_CHECK_INSTANCE_TYPE ((object), \
                                                                           BDK_TYPE_PIXMAP_IMPL_DIRECTFB))


typedef struct _BdkDrawableImplDirectFB BdkDrawableImplDirectFB;
typedef struct _BdkWindowImplDirectFB   BdkWindowImplDirectFB;
typedef struct _BdkPixmapImplDirectFB   BdkPixmapImplDirectFB;


struct _BdkDrawableImplDirectFB
{
  BdkDrawable parent_object;

  BdkDrawable *wrapper;

  bboolean buffered;

  BdkRebunnyion paint_rebunnyion;
  bint      paint_depth;
  bint      width;
  bint      height;
  bint      abs_x;
  bint      abs_y;

  BdkRebunnyion clip_rebunnyion;

  BdkColormap *colormap;

  IDirectFBSurface      *surface;
  DFBSurfacePixelFormat  format;
  bairo_surface_t       *bairo_surface;
};

typedef struct
{
  BdkDrawableClass parent_class;
} BdkDrawableImplDirectFBClass;

GType      bdk_drawable_impl_directfb_get_type (void);

void bdk_directfb_event_fill (BdkEvent     *event,
                              BdkWindow    *window,
                              BdkEventType  type);
BdkEvent *bdk_directfb_event_make (BdkWindow    *window,
                                   BdkEventType  type);


/*
 * Pixmap
 */

struct _BdkPixmapImplDirectFB
{
  BdkDrawableImplDirectFB parent_instance;
};

typedef struct
{
  BdkDrawableImplDirectFBClass parent_class;
} BdkPixmapImplDirectFBClass;

GType bdk_pixmap_impl_directfb_get_type (void);



/*
 * Window
 */

typedef struct
{
  bulong   length;
  BdkAtom  type;
  bint     format;
  buchar   data[1];
} BdkWindowProperty;


struct _BdkWindowImplDirectFB
{
  BdkDrawableImplDirectFB drawable;

  IDirectFBWindow        *window;

  DFBWindowID             dfb_id;

  BdkCursor              *cursor;
  GHashTable             *properties;

  buint8                  opacity;

  BdkWindowTypeHint       type_hint;

  DFBUpdates              flips;
  DFBRebunnyion               flip_rebunnyions[4];
};

typedef struct
{
  BdkDrawableImplDirectFBClass parent_class;
} BdkWindowImplDirectFBClass;

GType bdk_window_impl_directfb_get_type        (void);

void  bdk_directfb_window_send_crossing_events (BdkWindow       *src,
                                                BdkWindow       *dest,
                                                BdkCrossingMode  mode);

BdkWindow * bdk_directfb_window_find_toplevel  (BdkWindow       *window);


void        bdk_directfb_window_id_table_insert (DFBWindowID  dfb_id,
                                                 BdkWindow   *window);
void        bdk_directfb_window_id_table_remove (DFBWindowID  dfb_id);
BdkWindow * bdk_directfb_window_id_table_lookup (DFBWindowID  dfb_id);

void        _bdk_directfb_window_get_offsets    (BdkWindow       *window,
                                                 bint            *x_offset,
                                                 bint            *y_offset);
void        _bdk_directfb_window_scroll         (BdkWindow       *window,
                                                 bint             dx,
                                                 bint             dy);
void        _bdk_directfb_window_move_rebunnyion    (BdkWindow       *window,
                                                 const BdkRebunnyion *rebunnyion,
                                                 bint             dx,
                                                 bint             dy);


typedef struct
{
  BdkCursor         cursor;

  bint              hot_x;
  bint              hot_y;
  IDirectFBSurface *shape;
} BdkCursorDirectFB;

typedef struct
{
  BdkVisual              visual;
  DFBSurfacePixelFormat  format;
} BdkVisualDirectFB;

typedef struct
{
  IDirectFBSurface *surface;
} BdkImageDirectFB;


#define BDK_TYPE_GC_DIRECTFB       (_bdk_gc_directfb_get_type ())
#define BDK_GC_DIRECTFB(object)    (B_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_GC_DIRECTFB, BdkGCDirectFB))
#define BDK_IS_GC_DIRECTFB(object) (B_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_GC_DIRECTFB))

typedef struct
{
  BdkGC             parent_instance;

  BdkRebunnyion         clip_rebunnyion;

  BdkGCValuesMask   values_mask;
  BdkGCValues       values;
} BdkGCDirectFB;

typedef struct
{
  BdkGCClass        parent_class;
} BdkGCDirectFBClass;

GType     _bdk_gc_directfb_get_type (void);

BdkGC    *_bdk_directfb_gc_new      (BdkDrawable     *drawable,
                                     BdkGCValues     *values,
                                     BdkGCValuesMask  values_mask);

BdkImage* _bdk_directfb_copy_to_image (BdkDrawable  *drawable,
                                       BdkImage     *image,
                                       bint          src_x,
                                       bint          src_y,
                                       bint          dest_x,
                                       bint          dest_y,
                                       bint          width,
                                       bint          height);

void       bdk_directfb_event_windows_add    (BdkWindow *window);
void       bdk_directfb_event_windows_remove (BdkWindow *window);

BdkGrabStatus bdk_directfb_keyboard_grab  (BdkDisplay          *display,
                                           BdkWindow           *window,
                                           bint                 owner_events,
                                           buint32              time);

void          bdk_directfb_keyboard_ungrab(BdkDisplay          *display,
                                           buint32              time);

BdkGrabStatus bdk_directfb_pointer_grab   (BdkWindow           *window,
                                           bint                 owner_events,
                                           BdkEventMask         event_mask,
                                           BdkWindow           *confine_to,
                                           BdkCursor           *cursor,
                                           buint32              time,
                                           bboolean             implicit_grab);
void          bdk_directfb_pointer_ungrab (buint32              time,
                                           bboolean             implicit_grab);

buint32       bdk_directfb_get_time       (void);

BdkWindow  *bdk_directfb_pointer_event_window  (BdkWindow    *window,
                                                BdkEventType  type);
BdkWindow  *bdk_directfb_keyboard_event_window (BdkWindow    *window,
                                                BdkEventType  type);
BdkWindow  *bdk_directfb_other_event_window    (BdkWindow    *window,
                                                BdkEventType  type);
void       _bdk_selection_window_destroyed    (BdkWindow       *window);

void       _bdk_directfb_move_resize_child (BdkWindow *window,
                                            bint       x,
                                            bint       y,
                                            bint       width,
                                            bint       height);

BdkWindow  *bdk_directfb_child_at          (BdkWindow *window,
                                            bint      *x,
                                            bint      *y);

BdkWindow  *bdk_directfb_window_find_focus (void);

void        bdk_directfb_change_focus      (BdkWindow *new_focus_window);

void        bdk_directfb_mouse_get_info    (bint            *x,
                                            bint            *y,
                                            BdkModifierType *mask);

/**********************/
/*  Global variables  */
/**********************/

extern BdkDisplayDFB *_bdk_display;

/* Pointer grab info */
extern BdkWindow           * _bdk_directfb_pointer_grab_window;
extern bboolean              _bdk_directfb_pointer_grab_owner_events;
extern BdkWindow           * _bdk_directfb_pointer_grab_confine;
extern BdkEventMask          _bdk_directfb_pointer_grab_events;
extern BdkCursor           * _bdk_directfb_pointer_grab_cursor;

/* Keyboard grab info */
extern BdkWindow           * _bdk_directfb_keyboard_grab_window;
extern BdkEventMask          _bdk_directfb_keyboard_grab_events;
extern bboolean              _bdk_directfb_keyboard_grab_owner_events;

extern BdkScreen  *  _bdk_screen;

extern BdkAtom               _bdk_selection_property;


IDirectFBPalette * bdk_directfb_colormap_get_palette (BdkColormap *colormap);


/* these are Linux-FB specific functions used for window decorations */

typedef bboolean (* BdkWindowChildChanged) (BdkWindow *window,
                                            bint       x,
                                            bint       y,
                                            bint       width,
                                            bint       height,
                                            bpointer   user_data);
typedef void     (* BdkWindowChildGetPos)  (BdkWindow *window,
                                            bint      *x,
                                            bint      *y,
                                            bpointer   user_data);

void bdk_fb_window_set_child_handler (BdkWindow              *window,
                                      BdkWindowChildChanged  changed,
                                      BdkWindowChildGetPos   get_pos,
                                      bpointer               user_data);

void bdk_directfb_clip_rebunnyion (BdkDrawable  *drawable,
                               BdkGC        *gc,
                               BdkRectangle *draw_rect,
                               BdkRebunnyion    *ret_clip);


/* Utilities for avoiding mallocs */

static inline void
temp_rebunnyion_init_copy (BdkRebunnyion       *rebunnyion,
                       const BdkRebunnyion *source)
{
  if (rebunnyion != source) /*  don't want to copy to itself */
    {
      if (rebunnyion->size < source->numRects)
        {
          if (rebunnyion->rects && rebunnyion->rects != &rebunnyion->extents)
            g_free (rebunnyion->rects);

          rebunnyion->rects = g_new (BdkRebunnyionBox, source->numRects);
          rebunnyion->size  = source->numRects;
        }

      rebunnyion->numRects = source->numRects;
      rebunnyion->extents  = source->extents;

      memcpy (rebunnyion->rects, source->rects, source->numRects * sizeof (BdkRebunnyionBox));
    }
}

static inline void
temp_rebunnyion_init_rectangle (BdkRebunnyion          *rebunnyion,
                            const BdkRectangle *rect)
{
  rebunnyion->numRects   = 1;
  rebunnyion->rects      = &rebunnyion->extents;
  rebunnyion->extents.x1 = rect->x;
  rebunnyion->extents.y1 = rect->y;
  rebunnyion->extents.x2 = rect->x + rect->width;
  rebunnyion->extents.y2 = rect->y + rect->height;
  rebunnyion->size       = 1;
}

static inline void
temp_rebunnyion_init_rectangle_vals (BdkRebunnyion *rebunnyion,
                                 int        x,
                                 int        y,
                                 int        w,
                                 int        h)
{
  rebunnyion->numRects   = 1;
  rebunnyion->rects      = &rebunnyion->extents;
  rebunnyion->extents.x1 = x;
  rebunnyion->extents.y1 = y;
  rebunnyion->extents.x2 = x + w;
  rebunnyion->extents.y2 = y + h;
  rebunnyion->size       = 1;
}

static inline void
temp_rebunnyion_reset (BdkRebunnyion *rebunnyion)
{
  if (rebunnyion->size > 32 && rebunnyion->rects && rebunnyion->rects != &rebunnyion->extents) {
    g_free (rebunnyion->rects);

    rebunnyion->size  = 1;
    rebunnyion->rects = &rebunnyion->extents;
  }

  rebunnyion->numRects = 0;
}

static inline void
temp_rebunnyion_deinit (BdkRebunnyion *rebunnyion)
{
  if (rebunnyion->rects && rebunnyion->rects != &rebunnyion->extents) {
    g_free (rebunnyion->rects);
    rebunnyion->rects = NULL;
  }

  rebunnyion->numRects = 0;
}


#define BDKDFB_RECTANGLE_VALS_FROM_BOX(s)   (s)->x1, (s)->y1, (s)->x2-(s)->x1, (s)->y2-(s)->y1


#endif /* __BDK_PRIVATE_DIRECTFB_H__ */
