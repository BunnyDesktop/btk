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

#ifndef __BDK_DRAWABLE_QUARTZ_H__
#define __BDK_DRAWABLE_QUARTZ_H__

#include <bdk/bdkdrawable.h>

B_BEGIN_DECLS

/* Drawable implementation for Quartz
 */

typedef struct _BdkDrawableImplQuartz BdkDrawableImplQuartz;
typedef struct _BdkDrawableImplQuartzClass BdkDrawableImplQuartzClass;

#define BDK_TYPE_DRAWABLE_IMPL_QUARTZ              (bdk_drawable_impl_quartz_get_type ())
#define BDK_DRAWABLE_IMPL_QUARTZ(object)           (B_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_DRAWABLE_IMPL_QUARTZ, BdkDrawableImplQuartz))
#define BDK_DRAWABLE_IMPL_QUARTZ_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_DRAWABLE_IMPL_QUARTZ, BdkDrawableImplQuartzClass))
#define BDK_IS_DRAWABLE_IMPL_QUARTZ(object)        (B_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_DRAWABLE_IMPL_QUARTZ))
#define BDK_IS_DRAWABLE_IMPL_QUARTZ_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_DRAWABLE_IMPL_QUARTZ))
#define BDK_DRAWABLE_IMPL_QUARTZ_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_DRAWABLE_IMPL_QUARTZ, BdkDrawableImplQuartzClass))

struct _BdkDrawableImplQuartz
{
  BdkDrawable      parent_instance;

  BdkDrawable     *wrapper;

  BdkColormap     *colormap;

  bairo_surface_t *bairo_surface;
};
 
struct _BdkDrawableImplQuartzClass
{
  BdkDrawableClass parent_class;

  /* vtable */
  CGContextRef (*get_context) (BdkDrawable* drawable,
			       bboolean     antialias);
};

GType        bdk_drawable_impl_quartz_get_type   (void);
CGContextRef bdk_quartz_drawable_get_context     (BdkDrawable  *drawable, 
						  bboolean      antialias);
void         bdk_quartz_drawable_release_context (BdkDrawable  *drawable, 
						  CGContextRef  context);

B_END_DECLS

#endif /* __BDK_DRAWABLE_QUARTZ_H__ */
