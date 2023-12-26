/* bdkdrawable-quartz.h
 *
 * Copyright (C) 2005 Imendio AB
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

#ifndef __BDK_WINDOW_QUARTZ_H__
#define __BDK_WINDOW_QUARTZ_H__

#include <bdk/quartz/bdkdrawable-quartz.h>
#import <bdk/quartz/BdkQuartzView.h>
#import <bdk/quartz/BdkQuartzWindow.h>

G_BEGIN_DECLS

/* Window implementation for Quartz
 */

typedef struct _BdkWindowImplQuartz BdkWindowImplQuartz;
typedef struct _BdkWindowImplQuartzClass BdkWindowImplQuartzClass;

#define BDK_TYPE_WINDOW_IMPL_QUARTZ              (_bdk_window_impl_quartz_get_type ())
#define BDK_WINDOW_IMPL_QUARTZ(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_WINDOW_IMPL_QUARTZ, BdkWindowImplQuartz))
#define BDK_WINDOW_IMPL_QUARTZ_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_WINDOW_IMPL_QUARTZ, BdkWindowImplQuartzClass))
#define BDK_IS_WINDOW_IMPL_QUARTZ(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_WINDOW_IMPL_QUARTZ))
#define BDK_IS_WINDOW_IMPL_QUARTZ_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_WINDOW_IMPL_QUARTZ))
#define BDK_WINDOW_IMPL_QUARTZ_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_WINDOW_IMPL_QUARTZ, BdkWindowImplQuartzClass))

struct _BdkWindowImplQuartz
{
  BdkDrawableImplQuartz parent_instance;

  NSWindow *toplevel;
  NSTrackingRectTag tracking_rect;
  BdkQuartzView *view;

  BdkWindowTypeHint type_hint;

  BdkRebunnyion *paint_clip_rebunnyion;
  gint begin_paint_count;
  gint in_paint_rect_count;

  BdkWindow *transient_for;

  /* Sorted by z-order */
  GList *sorted_children;

  BdkRebunnyion *needs_display_rebunnyion;
};
 
struct _BdkWindowImplQuartzClass 
{
  BdkDrawableImplQuartzClass parent_class;
};

GType _bdk_window_impl_quartz_get_type (void);

G_END_DECLS

#endif /* __BDK_WINDOW_QUARTZ_H__ */
