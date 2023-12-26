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

#ifndef __BDK_REBUNNYION_H__
#define __BDK_REBUNNYION_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bdk/bdktypes.h>

G_BEGIN_DECLS

#ifndef BDK_DISABLE_DEPRECATED
/* GC fill rule for polygons
 *  EvenOddRule
 *  WindingRule
 */
typedef enum
{
  BDK_EVEN_ODD_RULE,
  BDK_WINDING_RULE
} BdkFillRule;
#endif

/* Types of overlapping between a rectangle and a rebunnyion
 * BDK_OVERLAP_RECTANGLE_IN: rectangle is in rebunnyion
 * BDK_OVERLAP_RECTANGLE_OUT: rectangle in not in rebunnyion
 * BDK_OVERLAP_RECTANGLE_PART: rectangle in partially in rebunnyion
 */
typedef enum
{
  BDK_OVERLAP_RECTANGLE_IN,
  BDK_OVERLAP_RECTANGLE_OUT,
  BDK_OVERLAP_RECTANGLE_PART
} BdkOverlapType;

#ifndef BDK_DISABLE_DEPRECATED
typedef void (* BdkSpanFunc) (BdkSpan *span,
                              gpointer data);
#endif

BdkRebunnyion    * bdk_rebunnyion_new             (void);
#ifndef BDK_DISABLE_DEPRECATED
BdkRebunnyion    * bdk_rebunnyion_polygon         (const BdkPoint     *points,
                                           gint                n_points,
                                           BdkFillRule         fill_rule);
#endif
BdkRebunnyion    * bdk_rebunnyion_copy            (const BdkRebunnyion    *rebunnyion);
BdkRebunnyion    * bdk_rebunnyion_rectangle       (const BdkRectangle *rectangle);
void           bdk_rebunnyion_destroy         (BdkRebunnyion          *rebunnyion);

void	       bdk_rebunnyion_get_clipbox     (const BdkRebunnyion    *rebunnyion,
                                           BdkRectangle       *rectangle);
void           bdk_rebunnyion_get_rectangles  (const BdkRebunnyion    *rebunnyion,
                                           BdkRectangle      **rectangles,
                                           gint               *n_rectangles);

gboolean       bdk_rebunnyion_empty           (const BdkRebunnyion    *rebunnyion);
gboolean       bdk_rebunnyion_equal           (const BdkRebunnyion    *rebunnyion1,
                                           const BdkRebunnyion    *rebunnyion2);
#ifndef BDK_DISABLE_DEPRECATED
gboolean       bdk_rebunnyion_rect_equal      (const BdkRebunnyion    *rebunnyion,
                                           const BdkRectangle *rectangle);
#endif
gboolean       bdk_rebunnyion_point_in        (const BdkRebunnyion    *rebunnyion,
                                           int                 x,
                                           int                 y);
BdkOverlapType bdk_rebunnyion_rect_in         (const BdkRebunnyion    *rebunnyion,
                                           const BdkRectangle *rectangle);

void           bdk_rebunnyion_offset          (BdkRebunnyion          *rebunnyion,
                                           gint                dx,
                                           gint                dy);
#ifndef BDK_DISABLE_DEPRECATED
void           bdk_rebunnyion_shrink          (BdkRebunnyion          *rebunnyion,
                                           gint                dx,
                                           gint                dy);
#endif
void           bdk_rebunnyion_union_with_rect (BdkRebunnyion          *rebunnyion,
                                           const BdkRectangle *rect);
void           bdk_rebunnyion_intersect       (BdkRebunnyion          *source1,
                                           const BdkRebunnyion    *source2);
void           bdk_rebunnyion_union           (BdkRebunnyion          *source1,
                                           const BdkRebunnyion    *source2);
void           bdk_rebunnyion_subtract        (BdkRebunnyion          *source1,
                                           const BdkRebunnyion    *source2);
void           bdk_rebunnyion_xor             (BdkRebunnyion          *source1,
                                           const BdkRebunnyion    *source2);

#ifndef BDK_DISABLE_DEPRECATED
void   bdk_rebunnyion_spans_intersect_foreach (BdkRebunnyion          *rebunnyion,
                                           const BdkSpan      *spans,
                                           int                 n_spans,
                                           gboolean            sorted,
                                           BdkSpanFunc         function,
                                           gpointer            data);
#endif

G_END_DECLS

#endif /* __BDK_REBUNNYION_H__ */

