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

#undef BDK_DISABLE_DEPRECATED

#include "config.h"
#include "bdk.h"

#include <string.h>

#include "bdkdirectfb.h"
#include "bdkprivate-directfb.h"

#include "bdkgc.h"
#include "bdkfont.h"
#include "bdkpixmap.h"
#include "bdkrebunnyion-generic.h"

#include "bdkalias.h"

static void bdk_directfb_gc_get_values (BdkGC           *gc,
                                        BdkGCValues     *values);
static void bdk_directfb_gc_set_values (BdkGC           *gc,
                                        BdkGCValues     *values,
                                        BdkGCValuesMask  values_mask);
static void bdk_directfb_gc_set_dashes (BdkGC           *gc,
                                        gint             dash_offset,
                                        gint8            dash_list[],
                                        gint             n);


static void bdk_gc_directfb_finalize   (GObject            *object);

G_DEFINE_TYPE (BdkGCDirectFB, _bdk_gc_directfb, BDK_TYPE_GC)

static void
_bdk_gc_directfb_init (BdkGCDirectFB *directfb_gc)
{
}

static void
_bdk_gc_directfb_class_init (BdkGCDirectFBClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BdkGCClass   *gc_class     = BDK_GC_CLASS (klass);

  object_class->finalize = bdk_gc_directfb_finalize;

  gc_class->get_values = bdk_directfb_gc_get_values;
  gc_class->set_values = bdk_directfb_gc_set_values;
  gc_class->set_dashes = bdk_directfb_gc_set_dashes;
}

static void
bdk_gc_directfb_finalize (GObject *object)
{
  BdkGCDirectFB *directfb_gc = BDK_GC_DIRECTFB (object);

  if (directfb_gc->clip_rebunnyion.numRects)
    temp_rebunnyion_deinit (&directfb_gc->clip_rebunnyion);
  if (directfb_gc->values.clip_mask)
    g_object_unref (directfb_gc->values.clip_mask);
  if (directfb_gc->values.stipple)
    g_object_unref (directfb_gc->values.stipple);
  if (directfb_gc->values.tile)
    g_object_unref (directfb_gc->values.tile);

  G_OBJECT_CLASS (_bdk_gc_directfb_parent_class)->finalize (object);
}


BdkGC *
_bdk_directfb_gc_new (BdkDrawable     *drawable,
                      BdkGCValues     *values,
                      BdkGCValuesMask  values_mask)
{
  BdkGC         *gc;
  BdkGCDirectFB *private;

  /* NOTICE that the drawable here has to be the impl drawable, not the
     publicly visible drawable. */
  g_return_val_if_fail (BDK_IS_DRAWABLE_IMPL_DIRECTFB (drawable), NULL);

  gc = BDK_GC (g_object_new (_bdk_gc_directfb_get_type (), NULL));

  _bdk_gc_init (gc, drawable, values, values_mask);

  private = BDK_GC_DIRECTFB (gc);
#if 0
  private->values.background.pixel = 0;
  private->values.background.red   = 0;
  private->values.background.green = 0;
  private->values.background.blue  = 0;

  private->values.foreground.pixel = 0;
  private->values.foreground.red   = 0;
  private->values.foreground.green = 0;
  private->values.foreground.blue  = 0;
#endif

  private->values.cap_style = BDK_CAP_BUTT;

  bdk_directfb_gc_set_values (gc, values, values_mask);

  return gc;
}

static void
bdk_directfb_gc_get_values (BdkGC       *gc,
                            BdkGCValues *values)
{
  *values = BDK_GC_DIRECTFB (gc)->values;
}

#if 0
void
_bdk_windowing_gc_get_foreground (BdkGC    *gc,
                                  BdkColor *color)
{
  BdkGCDirectFB *private = BDK_GC_DIRECTFB (gc);
  *color =private->values.foreground;
}
#endif

static void
bdk_directfb_gc_set_values (BdkGC           *gc,
                            BdkGCValues     *values,
                            BdkGCValuesMask  values_mask)
{
  BdkGCDirectFB *private = BDK_GC_DIRECTFB (gc);

  if (values_mask & BDK_GC_FOREGROUND)
    {
      private->values.foreground = values->foreground;
      private->values_mask |= BDK_GC_FOREGROUND;
    }

  if (values_mask & BDK_GC_BACKGROUND)
    {
      private->values.background = values->background;
      private->values_mask |= BDK_GC_BACKGROUND;
    }

  if (values_mask & BDK_GC_FONT)
    {
      BdkFont *oldf = private->values.font;

      private->values.font = bdk_font_ref (values->font);
      private->values_mask |= BDK_GC_FONT;

      if (oldf)
        bdk_font_unref (oldf);
    }

  if (values_mask & BDK_GC_FUNCTION)
    {
      private->values.function = values->function;
      private->values_mask |= BDK_GC_FUNCTION;
    }

  if (values_mask & BDK_GC_FILL)
    {
      private->values.fill = values->fill;
      private->values_mask |= BDK_GC_FILL;
    }

  if (values_mask & BDK_GC_TILE)
    {
      BdkPixmap *oldpm = private->values.tile;

      if (values->tile)
        g_assert (BDK_PIXMAP_OBJECT (values->tile)->depth > 1);

      private->values.tile = values->tile ? g_object_ref (values->tile) : NULL;
      private->values_mask |= BDK_GC_TILE;

      if (oldpm)
        g_object_unref (oldpm);
    }

  if (values_mask & BDK_GC_STIPPLE)
    {
      BdkPixmap *oldpm = private->values.stipple;

      if (values->stipple)
        g_assert (BDK_PIXMAP_OBJECT (values->stipple)->depth == 1);

      private->values.stipple = (values->stipple ?
                                 g_object_ref (values->stipple) : NULL);
      private->values_mask |= BDK_GC_STIPPLE;

      if (oldpm)
        g_object_unref (oldpm);
    }

  if (values_mask & BDK_GC_CLIP_MASK)
    {
      BdkPixmap *oldpm = private->values.clip_mask;

      private->values.clip_mask = (values->clip_mask ?
                                   g_object_ref (values->clip_mask) : NULL);
      private->values_mask |= BDK_GC_CLIP_MASK;

      if (oldpm)
        g_object_unref (oldpm);

      temp_rebunnyion_reset (&private->clip_rebunnyion);
    }

  if (values_mask & BDK_GC_SUBWINDOW)
    {
      private->values.subwindow_mode = values->subwindow_mode;
      private->values_mask |= BDK_GC_SUBWINDOW;
    }

  if (values_mask & BDK_GC_TS_X_ORIGIN)
    {
      private->values.ts_x_origin = values->ts_x_origin;
      private->values_mask |= BDK_GC_TS_X_ORIGIN;
    }

  if (values_mask & BDK_GC_TS_Y_ORIGIN)
    {
      private->values.ts_y_origin = values->ts_y_origin;
      private->values_mask |= BDK_GC_TS_Y_ORIGIN;
    }

  if (values_mask & BDK_GC_CLIP_X_ORIGIN)
    {
      private->values.clip_x_origin = BDK_GC (gc)->clip_x_origin = values->clip_x_origin;
      private->values_mask |= BDK_GC_CLIP_X_ORIGIN;
    }

  if (values_mask & BDK_GC_CLIP_Y_ORIGIN)
    {
      private->values.clip_y_origin = BDK_GC (gc)->clip_y_origin = values->clip_y_origin;
      private->values_mask |= BDK_GC_CLIP_Y_ORIGIN;
    }

  if (values_mask & BDK_GC_EXPOSURES)
    {
      private->values.graphics_exposures = values->graphics_exposures;
      private->values_mask |= BDK_GC_EXPOSURES;
    }

  if (values_mask & BDK_GC_LINE_WIDTH)
    {
      private->values.line_width = values->line_width;
      private->values_mask |= BDK_GC_LINE_WIDTH;
    }

  if (values_mask & BDK_GC_LINE_STYLE)
    {
      private->values.line_style = values->line_style;
      private->values_mask |= BDK_GC_LINE_STYLE;
    }

  if (values_mask & BDK_GC_CAP_STYLE)
    {
      private->values.cap_style = values->cap_style;
      private->values_mask |= BDK_GC_CAP_STYLE;
    }

  if (values_mask & BDK_GC_JOIN_STYLE)
    {
      private->values.join_style = values->join_style;
      private->values_mask |= BDK_GC_JOIN_STYLE;
    }
}

static void
bdk_directfb_gc_set_dashes (BdkGC *gc,
                            gint   dash_offset,
                            gint8  dash_list[],
                            gint   n)
{
  g_warning ("bdk_directfb_gc_set_dashes not implemented");
}

static void
gc_unset_clip_mask (BdkGC *gc)
{
  BdkGCDirectFB *data = BDK_GC_DIRECTFB (gc);

  if (data->values.clip_mask)
    {
      g_object_unref (data->values.clip_mask);
      data->values.clip_mask = NULL;
      data->values_mask &= ~ BDK_GC_CLIP_MASK;
    }
}


void
_bdk_windowing_gc_set_clip_rebunnyion (BdkGC           *gc,
                                   const BdkRebunnyion *rebunnyion,
                                   gboolean         reset_origin)
{
  BdkGCDirectFB *data;

  g_return_if_fail (gc != NULL);

  data = BDK_GC_DIRECTFB (gc);

  if (rebunnyion == &data->clip_rebunnyion)
    return;

  if (rebunnyion)
    temp_rebunnyion_init_copy (&data->clip_rebunnyion, rebunnyion);
  else
    temp_rebunnyion_reset (&data->clip_rebunnyion);

  if (reset_origin)
    {
      gc->clip_x_origin          = 0;
      gc->clip_y_origin          = 0;
      data->values.clip_x_origin = 0;
      data->values.clip_y_origin = 0;
    }

  gc_unset_clip_mask (gc);
}

void
_bdk_windowing_gc_copy (BdkGC *dst_gc,
                        BdkGC *src_gc)
{
  BdkGCDirectFB *dst_private;

  g_return_if_fail (dst_gc != NULL);
  g_return_if_fail (src_gc != NULL);

  dst_private = BDK_GC_DIRECTFB (dst_gc);

  temp_rebunnyion_reset(&dst_private->clip_rebunnyion);

  if (dst_private->values_mask & BDK_GC_FONT)
    bdk_font_unref (dst_private->values.font);
  if (dst_private->values_mask & BDK_GC_TILE)
    g_object_unref (dst_private->values.tile);
  if (dst_private->values_mask & BDK_GC_STIPPLE)
    g_object_unref (dst_private->values.stipple);
  if (dst_private->values_mask & BDK_GC_CLIP_MASK)
    g_object_unref (dst_private->values.clip_mask);

  *dst_gc = *src_gc;
  if (dst_private->values_mask & BDK_GC_FONT)
    bdk_font_ref (dst_private->values.font);
  if (dst_private->values_mask & BDK_GC_TILE)
    g_object_ref (dst_private->values.tile);
  if (dst_private->values_mask & BDK_GC_STIPPLE)
    g_object_ref (dst_private->values.stipple);
  if (dst_private->values_mask & BDK_GC_CLIP_MASK)
    g_object_ref (dst_private->values.clip_mask);
}

/**
 * bdk_gc_get_screen:
 * @gc: a #BdkGC.
 *
 * Gets the #BdkScreen for which @gc was created
 *
 * Returns: the #BdkScreen for @gc.
 *
 * Since: 2.2
 */
BdkScreen *
bdk_gc_get_screen (BdkGC *gc)
{
  g_return_val_if_fail (BDK_IS_GC_DIRECTFB (gc), NULL);

  return _bdk_screen;
}
#define __BDK_GC_X11_C__
#include "bdkaliasdef.c"
