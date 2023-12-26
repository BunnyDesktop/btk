/* BTK - The GIMP Toolkit
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

#include "config.h"

#include "btkorientable.h"
#include "btkvbox.h"
#include "btkalias.h"

/**
 * SECTION:btkvbox
 * @Short_description: A vertical container box
 * @Title: BtkVBox
 * @See_also: #BtkHBox
 *
 * A #BtkVBox is a container that organizes child widgets into a single column.
 *
 * Use the #BtkBox packing interface to determine the arrangement,
 * spacing, height, and alignment of #BtkVBox children.
 *
 * All children are allocated the same width.
 */

G_DEFINE_TYPE (BtkVBox, btk_vbox, BTK_TYPE_BOX)

static void
btk_vbox_class_init (BtkVBoxClass *class)
{
}

static void
btk_vbox_init (BtkVBox *vbox)
{
  btk_orientable_set_orientation (BTK_ORIENTABLE (vbox),
                                  BTK_ORIENTATION_VERTICAL);

  _btk_box_set_old_defaults (BTK_BOX (vbox));
}

/**
 * btk_vbox_new:
 * @homogeneous: %TRUE if all children are to be given equal space allotments.
 * @spacing: the number of pixels to place by default between children.
 *
 * Creates a new #BtkVBox.
 *
 * Returns: a new #BtkVBox.
 */
BtkWidget *
btk_vbox_new (bboolean homogeneous,
	      bint     spacing)
{
  return g_object_new (BTK_TYPE_VBOX,
                       "spacing",     spacing,
                       "homogeneous", homogeneous ? TRUE : FALSE,
                       NULL);
}

#define __BTK_VBOX_C__
#include "btkaliasdef.c"
