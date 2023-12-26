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

#include "btkhbox.h"
#include "btkorientable.h"
#include "btkalias.h"


/**
 * SECTION:btkhbox
 * @Short_description: A horizontal container box
 * @Title: BtkHBox
 * @See_also: #BtkVBox
 *
 * #BtkHBox is a container that organizes child widgets into a single row.
 *
 * Use the #BtkBox packing interface to determine the arrangement,
 * spacing, width, and alignment of #BtkHBox children.
 *
 * All children are allocated the same height.
 */


G_DEFINE_TYPE (BtkHBox, btk_hbox, BTK_TYPE_BOX)

static void
btk_hbox_class_init (BtkHBoxClass *class)
{
}

static void
btk_hbox_init (BtkHBox *hbox)
{
  btk_orientable_set_orientation (BTK_ORIENTABLE (hbox),
                                  BTK_ORIENTATION_HORIZONTAL);

  _btk_box_set_old_defaults (BTK_BOX (hbox));
}

/**
 * btk_hbox_new:
 * @homogeneous: %TRUE if all children are to be given equal space allotments.
 * @spacing: the number of pixels to place by default between children.
 *
 * Creates a new #BtkHBox.
 *
 * Returns: a new #BtkHBox.
 */
BtkWidget *
btk_hbox_new (gboolean homogeneous,
	      gint     spacing)
{
  return g_object_new (BTK_TYPE_HBOX,
                       "spacing",     spacing,
                       "homogeneous", homogeneous ? TRUE : FALSE,
                       NULL);
}

#define __BTK_HBOX_C__
#include "btkaliasdef.c"
