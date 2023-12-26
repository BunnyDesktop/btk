/* BDK - The GIMP Drawing Kit
 * Copyright (C) 2005 Red Hat, Inc. 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __BDK_BAIRO_H__
#define __BDK_BAIRO_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bdk/bdkcolor.h>
#include <bdk/bdkpixbuf.h>
#include <bango/bangobairo.h>

G_BEGIN_DECLS

bairo_t *bdk_bairo_create            (BdkDrawable        *drawable);
void     bdk_bairo_reset_clip        (bairo_t            *cr,
				      BdkDrawable        *drawable);

void     bdk_bairo_set_source_color  (bairo_t            *cr,
                                      const BdkColor     *color);
void     bdk_bairo_set_source_pixbuf (bairo_t            *cr,
                                      const BdkPixbuf    *pixbuf,
                                      double              pixbuf_x,
                                      double              pixbuf_y);
void     bdk_bairo_set_source_pixmap (bairo_t            *cr,
                                      BdkPixmap          *pixmap,
                                      double              pixmap_x,
                                      double              pixmap_y);
void     bdk_bairo_set_source_window (bairo_t            *cr,
                                      BdkWindow          *window,
                                      double              x,
                                      double              y);

void     bdk_bairo_rectangle         (bairo_t            *cr,
                                      const BdkRectangle *rectangle);
void     bdk_bairo_rebunnyion            (bairo_t            *cr,
                                      const BdkRebunnyion    *rebunnyion);

G_END_DECLS

#endif /* __BDK_BAIRO_H__ */
