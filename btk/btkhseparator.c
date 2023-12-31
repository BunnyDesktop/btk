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

#include "btkhseparator.h"
#include "btkorientable.h"
#include "btkalias.h"

G_DEFINE_TYPE (BtkHSeparator, btk_hseparator, BTK_TYPE_SEPARATOR)

static void
btk_hseparator_class_init (BtkHSeparatorClass *class)
{
}

static void
btk_hseparator_init (BtkHSeparator *hseparator)
{
  btk_orientable_set_orientation (BTK_ORIENTABLE (hseparator),
                                  BTK_ORIENTATION_HORIZONTAL);
}

BtkWidget *
btk_hseparator_new (void)
{
  return g_object_new (BTK_TYPE_HSEPARATOR, NULL);
}

#define __BTK_HSEPARATOR_C__
#include "btkaliasdef.c"
