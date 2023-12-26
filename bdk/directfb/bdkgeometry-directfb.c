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

#include "config.h"
#include "bdk.h"        /* For bdk_rectangle_intersect */

#include "bdkdirectfb.h"
#include "bdkprivate-directfb.h"

#include "bdkinternals.h"
#include "bdkalias.h"


void
_bdk_directfb_window_get_offsets (BdkWindow *window,
                                  bint      *x_offset,
                                  bint      *y_offset)
{
  if (x_offset)
    *x_offset = 0;
  if (y_offset)
    *y_offset = 0;
}

bboolean
_bdk_windowing_window_queue_antiexpose (BdkWindow *window,
                                        BdkRebunnyion *area)
{
  return FALSE;
}

/**
 * bdk_window_scroll:
 * @window: a #BdkWindow
 * @dx: Amount to scroll in the X direction
 * @dy: Amount to scroll in the Y direction
 *
 * Scroll the contents of its window, both pixels and children, by
 * the given amount. Portions of the window that the scroll operation
 * brings in from offscreen areas are invalidated.
 **/
void
_bdk_directfb_window_scroll (BdkWindow *window,
                             bint       dx,
                             bint       dy)
{
  BdkWindowObject         *private;
  BdkDrawableImplDirectFB *impl;
  BdkRebunnyion               *invalidate_rebunnyion = NULL;
  GList                   *list;

  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  private = BDK_WINDOW_OBJECT (window);
  impl = BDK_DRAWABLE_IMPL_DIRECTFB (private->impl);

  if (dx == 0 && dy == 0)
    return;

  /* Move the current invalid rebunnyion */
  if (private->update_area)
    bdk_rebunnyion_offset (private->update_area, dx, dy);

  if (BDK_WINDOW_IS_MAPPED (window))
    {
      BdkRectangle  clip_rect = {  0,  0, impl->width, impl->height };
      BdkRectangle  rect      = { dx, dy, impl->width, impl->height };

      invalidate_rebunnyion = bdk_rebunnyion_rectangle (&clip_rect);

      if (bdk_rectangle_intersect (&rect, &clip_rect, &rect) &&
          (!private->update_area ||
           !bdk_rebunnyion_rect_in (private->update_area, &rect)))
        {
          BdkRebunnyion *rebunnyion;

          rebunnyion = bdk_rebunnyion_rectangle (&rect);
          bdk_rebunnyion_subtract (invalidate_rebunnyion, rebunnyion);
          bdk_rebunnyion_destroy (rebunnyion);

          if (impl->surface)
            {
              DFBRebunnyion update = { rect.x, rect.y,
                                   rect.x + rect.width  - 1,
                                   rect.y + rect.height - 1 };

              impl->surface->SetClip (impl->surface, &update);
              impl->surface->Blit (impl->surface, impl->surface, NULL, dx, dy);
              impl->surface->SetClip (impl->surface, NULL);
              impl->surface->Flip (impl->surface, &update, 0);
            }
        }
    }

  for (list = private->children; list; list = list->next)
    {
      BdkWindowObject         *obj      = BDK_WINDOW_OBJECT (list->data);
      BdkDrawableImplDirectFB *obj_impl = BDK_DRAWABLE_IMPL_DIRECTFB (obj->impl);

      _bdk_directfb_move_resize_child (list->data,
                                       obj->x + dx,
                                       obj->y + dy,
                                       obj_impl->width,
                                       obj_impl->height);
    }

  if (invalidate_rebunnyion)
    {
      bdk_window_invalidate_rebunnyion (window, invalidate_rebunnyion, TRUE);
      bdk_rebunnyion_destroy (invalidate_rebunnyion);
    }
}

/**
 * bdk_window_move_rebunnyion:
 * @window: a #BdkWindow
 * @rebunnyion: The #BdkRebunnyion to move
 * @dx: Amount to move in the X direction
 * @dy: Amount to move in the Y direction
 *
 * Move the part of @window indicated by @rebunnyion by @dy pixels in the Y
 * direction and @dx pixels in the X direction. The portions of @rebunnyion
 * that not covered by the new position of @rebunnyion are invalidated.
 *
 * Child windows are not moved.
 *
 * Since: 2.8
 **/
void
_bdk_directfb_window_move_rebunnyion (BdkWindow       *window,
                                  const BdkRebunnyion *rebunnyion,
                                  bint             dx,
                                  bint             dy)
{
  BdkWindowObject         *private;
  BdkDrawableImplDirectFB *impl;
  BdkRebunnyion               *window_clip;
  BdkRebunnyion               *src_rebunnyion;
  BdkRebunnyion               *brought_in;
  BdkRebunnyion               *dest_rebunnyion;
  BdkRebunnyion               *moving_invalid_rebunnyion;
  BdkRectangle             dest_extents;

  g_return_if_fail (BDK_IS_WINDOW (window));
  g_return_if_fail (rebunnyion != NULL);

  if (BDK_WINDOW_DESTROYED (window))
    return;

  private = BDK_WINDOW_OBJECT (window);
  impl = BDK_DRAWABLE_IMPL_DIRECTFB (private->impl);

  if (dx == 0 && dy == 0)
    return;

  BdkRectangle  clip_rect = {  0,  0, impl->width, impl->height };
  window_clip = bdk_rebunnyion_rectangle (&clip_rect);

  /* compute source rebunnyions */
  src_rebunnyion = bdk_rebunnyion_copy (rebunnyion);
  brought_in = bdk_rebunnyion_copy (rebunnyion);
  bdk_rebunnyion_intersect (src_rebunnyion, window_clip);

  bdk_rebunnyion_subtract (brought_in, src_rebunnyion);
  bdk_rebunnyion_offset (brought_in, dx, dy);

  /* compute destination rebunnyions */
  dest_rebunnyion = bdk_rebunnyion_copy (src_rebunnyion);
  bdk_rebunnyion_offset (dest_rebunnyion, dx, dy);
  bdk_rebunnyion_intersect (dest_rebunnyion, window_clip);
  bdk_rebunnyion_get_clipbox (dest_rebunnyion, &dest_extents);

  bdk_rebunnyion_destroy (window_clip);

  /* calculating moving part of current invalid area */
  moving_invalid_rebunnyion = NULL;
  if (private->update_area)
    {
      moving_invalid_rebunnyion = bdk_rebunnyion_copy (private->update_area);
      bdk_rebunnyion_intersect (moving_invalid_rebunnyion, src_rebunnyion);
      bdk_rebunnyion_offset (moving_invalid_rebunnyion, dx, dy);
    }

  /* invalidate all of the src rebunnyion */
  bdk_window_invalidate_rebunnyion (window, src_rebunnyion, FALSE);

  /* un-invalidate destination rebunnyion */
  if (private->update_area)
    bdk_rebunnyion_subtract (private->update_area, dest_rebunnyion);

  /* invalidate moving parts of existing update area */
  if (moving_invalid_rebunnyion)
    {
      bdk_window_invalidate_rebunnyion (window, moving_invalid_rebunnyion, FALSE);
      bdk_rebunnyion_destroy (moving_invalid_rebunnyion);
    }

  /* invalidate area brought in from off-screen */
  bdk_window_invalidate_rebunnyion (window, brought_in, FALSE);
  bdk_rebunnyion_destroy (brought_in);

  /* Actually do the moving */
  if (impl->surface)
    {
      DFBRectangle source = { dest_extents.x - dx,
                              dest_extents.y - dy,
                              dest_extents.width,
                              dest_extents.height};
      DFBRebunnyion destination = { dest_extents.x,
                                dest_extents.y,
                                dest_extents.x + dest_extents.width - 1,
                                dest_extents.y + dest_extents.height - 1};

      impl->surface->SetClip (impl->surface, &destination);
      impl->surface->Blit (impl->surface, impl->surface, &source, dx, dy);
      impl->surface->SetClip (impl->surface, NULL);
      impl->surface->Flip (impl->surface, &destination, 0);
    }
  bdk_rebunnyion_destroy (src_rebunnyion);
  bdk_rebunnyion_destroy (dest_rebunnyion);
}

#define __BDK_GEOMETRY_X11_C__
#include "bdkaliasdef.c"
