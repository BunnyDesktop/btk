/* BTK - The GIMP Toolkit
 * Copyright (C) 2001 Red Hat, Inc.
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

#include "config.h"

#include <math.h>
#include <stdlib.h>

#include "btkvscale.h"
#include "btkorientable.h"
#include "btkalias.h"

/**
 * SECTION:btkvscale
 * @Short_description: A vertical slider widget for selecting a value from a range
 * @Title: BtkVScale
 *
 * The #BtkVScale widget is used to allow the user to select a value using
 * a vertical slider. To create one, use btk_hscale_new_with_range().
 *
 * The position to show the current value, and the number of decimal places
 * shown can be set using the parent #BtkScale class's functions.
 */

G_DEFINE_TYPE (BtkVScale, btk_vscale, BTK_TYPE_SCALE)

static void
btk_vscale_class_init (BtkVScaleClass *class)
{
  BtkRangeClass *range_class = BTK_RANGE_CLASS (class);

  range_class->slider_detail = "vscale";
}

static void
btk_vscale_init (BtkVScale *vscale)
{
  btk_orientable_set_orientation (BTK_ORIENTABLE (vscale),
                                  BTK_ORIENTATION_VERTICAL);
}
/**
 * btk_vscale_new:
 * @adjustment: the #BtkAdjustment which sets the range of the scale.
 *
 * Creates a new #BtkVScale.
 *
 * Returns: a new #BtkVScale.
 */
BtkWidget *
btk_vscale_new (BtkAdjustment *adjustment)
{
  g_return_val_if_fail (adjustment == NULL || BTK_IS_ADJUSTMENT (adjustment),
                        NULL);

  return g_object_new (BTK_TYPE_VSCALE,
                       "adjustment", adjustment,
                       NULL);
}

/**
 * btk_vscale_new_with_range:
 * @min: minimum value
 * @max: maximum value
 * @step: step increment (tick size) used with keyboard shortcuts
 *
 * Creates a new vertical scale widget that lets the user input a
 * number between @min and @max (including @min and @max) with the
 * increment @step.  @step must be nonzero; it's the distance the
 * slider moves when using the arrow keys to adjust the scale value.
 *
 * Note that the way in which the precision is derived works best if @step
 * is a power of ten. If the resulting precision is not suitable for your
 * needs, use btk_scale_set_digits() to correct it.
 *
 * Return value: a new #BtkVScale
 **/
BtkWidget *
btk_vscale_new_with_range (gdouble min,
                           gdouble max,
                           gdouble step)
{
  BtkObject *adj;
  BtkScale *scale;
  gint digits;

  g_return_val_if_fail (min < max, NULL);
  g_return_val_if_fail (step != 0.0, NULL);

  adj = btk_adjustment_new (min, min, max, step, 10 * step, 0);

  if (fabs (step) >= 1.0 || step == 0.0)
    {
      digits = 0;
    }
  else
    {
      digits = abs ((gint) floor (log10 (fabs (step))));
      if (digits > 5)
        digits = 5;
    }

  scale = g_object_new (BTK_TYPE_VSCALE,
                        "adjustment", adj,
                        "digits", digits,
                        NULL);

  return BTK_WIDGET (scale);
}

#define __BTK_VSCALE_C__
#include "btkaliasdef.c"
