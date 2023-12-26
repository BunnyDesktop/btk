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

#include "btkhpaned.h"
#include "btkorientable.h"
#include "btkalias.h"

G_DEFINE_TYPE (BtkHPaned, btk_hpaned, BTK_TYPE_PANED)

static void
btk_hpaned_class_init (BtkHPanedClass *class)
{
}

static void
btk_hpaned_init (BtkHPaned *hpaned)
{
  btk_orientable_set_orientation (BTK_ORIENTABLE (hpaned),
                                  BTK_ORIENTATION_HORIZONTAL);
}

BtkWidget *
btk_hpaned_new (void)
{
  return g_object_new (BTK_TYPE_HPANED, NULL);
}

#define __BTK_HPANED_C__
#include "btkaliasdef.c"
