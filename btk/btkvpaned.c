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
#include "btkvpaned.h"
#include "btkalias.h"

/**
 * SECTION:btkvpaned
 * @Short_description: A container with two panes arranged vertically
 * @Title: BtkVPaned
 *
 * The VPaned widget is a container widget with two
 * children arranged vertically. The division between
 * the two panes is adjustable by the user by dragging
 * a handle. See #BtkPaned for details.
 */

G_DEFINE_TYPE (BtkVPaned, btk_vpaned, BTK_TYPE_PANED)

static void
btk_vpaned_class_init (BtkVPanedClass *class)
{
}

static void
btk_vpaned_init (BtkVPaned *vpaned)
{
  btk_orientable_set_orientation (BTK_ORIENTABLE (vpaned),
                                  BTK_ORIENTATION_VERTICAL);
}

/**
 * btk_vpaned_new:
 *
 * Create a new #BtkVPaned
 *
 * Returns: the new #BtkVPaned
 */
BtkWidget *
btk_vpaned_new (void)
{
  return g_object_new (BTK_TYPE_VPANED, NULL);
}

#define __BTK_VPANED_C__
#include "btkaliasdef.c"
