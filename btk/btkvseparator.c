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
#include "btkvseparator.h"
#include "btkalias.h"

/**
 * SECTION:btkvseparator
 * @Short_description: A vertical separator
 * @Title: BtkVSeparator
 * @See_also: #BtkHSeparator
 *
 * The #BtkVSeparator widget is a vertical separator, used to group the
 * widgets within a window. It displays a vertical line with a shadow to
 * make it appear sunken into the interface.
 */

G_DEFINE_TYPE (BtkVSeparator, btk_vseparator, BTK_TYPE_SEPARATOR)

static void
btk_vseparator_class_init (BtkVSeparatorClass *klass)
{
}

static void
btk_vseparator_init (BtkVSeparator *vseparator)
{
  btk_orientable_set_orientation (BTK_ORIENTABLE (vseparator),
                                  BTK_ORIENTATION_VERTICAL);
}

/**
 * btk_vseparator_new:
 *
 * Creates a new #BtkVSeparator.
 *
 * Returns: a new #BtkVSeparator.
 */
BtkWidget *
btk_vseparator_new (void)
{
  return g_object_new (BTK_TYPE_VSEPARATOR, NULL);
}

#define __BTK_VSEPARATOR_C__
#include "btkaliasdef.c"
