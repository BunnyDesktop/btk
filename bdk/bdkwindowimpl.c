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
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"
#include "bdkwindowimpl.h"
#include "bdkinternals.h"

#include "bdkalias.h"

GType
bdk_window_impl_get_type (void)
{
  static GType gtype = 0;

  if (B_UNLIKELY (!gtype))
    {
      gtype = g_type_register_static_simple (B_TYPE_INTERFACE,
                                             "BdkWindowImpl",
                                             sizeof (BdkWindowImplIface),
                                             NULL, 0, NULL, 0);
      g_type_interface_add_prerequisite (gtype, B_TYPE_OBJECT);
    }

  return gtype;
}

#define __BDK_WINDOW_IMPL_C__
#include "bdkaliasdef.c"

