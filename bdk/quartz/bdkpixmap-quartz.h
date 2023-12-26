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

#ifndef __BDK_PIXMAP_QUARTZ_H__
#define __BDK_PIXMAP_QUARTZ_H__

#include <ApplicationServices/ApplicationServices.h>
#include <bdk/quartz/bdkdrawable-quartz.h>
#include <bdk/bdkpixmap.h>

B_BEGIN_DECLS

/* Pixmap implementation for Quartz
 */

typedef struct _BdkPixmapImplQuartz BdkPixmapImplQuartz;
typedef struct _BdkPixmapImplQuartzClass BdkPixmapImplQuartzClass;

#define BDK_TYPE_PIXMAP_IMPL_QUARTZ              (_bdk_pixmap_impl_quartz_get_type ())
#define BDK_PIXMAP_IMPL_QUARTZ(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_PIXMAP_IMPL_QUARTZ, BdkPixmapImplQuartz))
#define BDK_PIXMAP_IMPL_QUARTZ_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_PIXMAP_IMPL_QUARTZ, BdkPixmapImplQuartzClass))
#define BDK_IS_PIXMAP_IMPL_QUARTZ(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_PIXMAP_IMPL_QUARTZ))
#define BDK_IS_PIXMAP_IMPL_QUARTZ_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_PIXMAP_IMPL_QUARTZ))
#define BDK_PIXMAP_IMPL_QUARTZ_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_PIXMAP_IMPL_QUARTZ, BdkPixmapImplQuartzClass))

struct _BdkPixmapImplQuartz
{
  BdkDrawableImplQuartz parent_instance;

  gint width;
  gint height;

  void *data;
  CGDataProviderRef data_provider;
};
 
struct _BdkPixmapImplQuartzClass 
{
  BdkDrawableImplQuartzClass parent_class;
};

GType _bdk_pixmap_impl_quartz_get_type (void);

B_END_DECLS

#endif /* __BDK_PIXMAP_QUARTZ_H__ */
