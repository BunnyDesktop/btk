/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

#include "btkorientable.h"
#include "btkvscrollbar.h"
#include "btkintl.h"
#include "btkalias.h"

/**
 * SECTION:btkvscrollbar
 * @Short_description: A vertical scrollbar
 * @Title: BtkVScrollbar
 * @See_also:#BtkScrollbar, #BtkScrolledWindow
 *
 * The #BtkVScrollbar widget is a widget arranged vertically creating a
 * scrollbar. See #BtkScrollbar for details on
 * scrollbars. #BtkAdjustment pointers may be added to handle the
 * adjustment of the scrollbar or it may be left %NULL in which case one
 * will be created for you. See #BtkScrollbar for a description of what the
 * fields in an adjustment represent for a scrollbar.
 */

G_DEFINE_TYPE (BtkVScrollbar, btk_vscrollbar, BTK_TYPE_SCROLLBAR)

static void
btk_vscrollbar_class_init (BtkVScrollbarClass *class)
{
  BTK_RANGE_CLASS (class)->stepper_detail = "vscrollbar";
}

static void
btk_vscrollbar_init (BtkVScrollbar *vscrollbar)
{
  btk_orientable_set_orientation (BTK_ORIENTABLE (vscrollbar),
                                  BTK_ORIENTATION_VERTICAL);
}

/**
 * btk_vscrollbar_new:
 * @adjustment: (allow-none): the #BtkAdjustment to use, or %NULL to create a new adjustment
 *
 * Creates a new vertical scrollbar.
 *
 * Returns: the new #BtkVScrollbar
 */
BtkWidget *
btk_vscrollbar_new (BtkAdjustment *adjustment)
{
  g_return_val_if_fail (adjustment == NULL || BTK_IS_ADJUSTMENT (adjustment),
                        NULL);

  return g_object_new (BTK_TYPE_VSCROLLBAR,
                       "adjustment", adjustment,
                       NULL);
}

#define __BTK_VSCROLLBAR_C__
#include "btkaliasdef.c"
