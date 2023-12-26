/* bdkvisual-quartz.c
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

#include "config.h"

#include "bdkvisual.h"
#include "bdkprivate-quartz.h"

static BdkVisual *system_visual;
static BdkVisual *rgba_visual;
static BdkVisual *gray_visual;

static void
bdk_visual_finalize (BObject *object)
{
  g_error ("A BdkVisual object was finalized. This should not happen");
}

static void
bdk_visual_class_init (BObjectClass *class)
{
  class->finalize = bdk_visual_finalize;
}

GType
bdk_visual_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      const GTypeInfo object_info =
      {
        sizeof (BdkVisualClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) bdk_visual_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (BdkVisual),
        0,              /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };
      
      object_type = g_type_register_static (B_TYPE_OBJECT,
                                            "BdkVisual",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void
bdk_visual_decompose_mask (gulong  mask,
			   gint   *shift,
			   gint   *prec)
{
  *shift = 0;
  *prec = 0;

  while (!(mask & 0x1))
    {
      (*shift)++;
      mask >>= 1;
    }

  while (mask & 0x1)
    {
      (*prec)++;
      mask >>= 1;
    }
}

static BdkVisual *
create_standard_visual (gint depth)
{
  BdkVisual *visual = g_object_new (BDK_TYPE_VISUAL, NULL);

  visual->depth = depth;
  visual->byte_order = BDK_MSB_FIRST; /* FIXME: Should this be different on intel macs? */
  visual->colormap_size = 0;
  
  visual->type = BDK_VISUAL_TRUE_COLOR;

  visual->red_mask = 0xff0000;
  visual->green_mask = 0xff00;
  visual->blue_mask = 0xff;
  
  bdk_visual_decompose_mask (visual->red_mask,
			     &visual->red_shift,
			     &visual->red_prec);
  bdk_visual_decompose_mask (visual->green_mask,
			     &visual->green_shift,
			     &visual->green_prec);
  bdk_visual_decompose_mask (visual->blue_mask,
			     &visual->blue_shift,
			     &visual->blue_prec);

  return visual;
}

static BdkVisual *
create_gray_visual (void)
{
  BdkVisual *visual = g_object_new (BDK_TYPE_VISUAL, NULL);

  visual->depth = 1;
  visual->byte_order = BDK_MSB_FIRST;
  visual->colormap_size = 0;

  visual->type = BDK_VISUAL_STATIC_GRAY;

  return visual;
}

void
_bdk_visual_init (void)
{
  system_visual = create_standard_visual (24);
  rgba_visual = create_standard_visual (32);
  gray_visual = create_gray_visual ();
}

/* We prefer the system visual for now ... */
gint
bdk_visual_get_best_depth (void)
{
  return system_visual->depth;
}

BdkVisualType
bdk_visual_get_best_type (void)
{
  return system_visual->type;
}

BdkVisual *
bdk_screen_get_rgba_visual (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);

  return rgba_visual;
}

BdkVisual*
bdk_screen_get_system_visual (BdkScreen *screen)
{
  return system_visual;
}

BdkVisual*
bdk_visual_get_best (void)
{
  return system_visual;
}

BdkVisual*
bdk_visual_get_best_with_depth (gint depth)
{
  BdkVisual *visual = NULL;

  switch (depth)
    {
      case 32:
        visual = rgba_visual;
        break;

      case 24:
        visual = system_visual;
        break;

      case 1:
        visual = gray_visual;
        break;

      default:
        visual = NULL;
    }

  return visual;
}

BdkVisual*
bdk_visual_get_best_with_type (BdkVisualType visual_type)
{
  if (system_visual->type == visual_type)
    return system_visual;
  else if (gray_visual->type == visual_type)
    return gray_visual;

  return NULL;
}

BdkVisual*
bdk_visual_get_best_with_both (gint          depth,
			       BdkVisualType visual_type)
{
  if (system_visual->depth == depth
      && system_visual->type == visual_type)
    return system_visual;
  else if (rgba_visual->depth == depth
           && rgba_visual->type == visual_type)
    return rgba_visual;
  else if (gray_visual->depth == depth
           && gray_visual->type == visual_type)
    return gray_visual;

  return NULL;
}

/* For these, we also prefer the system visual */
void
bdk_query_depths  (gint **depths,
		   gint  *count)
{
  *count = 1;
  *depths = &system_visual->depth;
}

void
bdk_query_visual_types (BdkVisualType **visual_types,
			gint           *count)
{
  *count = 1;
  *visual_types = &system_visual->type;
}

GList*
bdk_screen_list_visuals (BdkScreen *screen)
{
  GList *visuals = NULL;

  visuals = g_list_append (visuals, system_visual);
  visuals = g_list_append (visuals, rgba_visual);
  visuals = g_list_append (visuals, gray_visual);

  return visuals;
}

BdkScreen *
bdk_visual_get_screen (BdkVisual *visual)
{
  g_return_val_if_fail (BDK_IS_VISUAL (visual), NULL);

  return bdk_screen_get_default ();
}

