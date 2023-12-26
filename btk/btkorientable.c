/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * btkorientable.c
 * Copyright (C) 2008 Imendio AB
 * Contact: Michael Natterer <mitch@imendio.com>
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

#include "btkorientable.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"


typedef BtkOrientableIface BtkOrientableInterface;
G_DEFINE_INTERFACE (BtkOrientable, btk_orientable, B_TYPE_OBJECT)

static void
btk_orientable_default_init (BtkOrientableInterface *iface)
{
  /**
   * BtkOrientable:orientation:
   *
   * The orientation of the orientable.
   *
   * Since: 2.16
   **/
  g_object_interface_install_property (iface,
                                       g_param_spec_enum ("orientation",
                                                          P_("Orientation"),
                                                          P_("The orientation of the orientable"),
                                                          BTK_TYPE_ORIENTATION,
                                                          BTK_ORIENTATION_HORIZONTAL,
                                                          BTK_PARAM_READWRITE));
}

/**
 * btk_orientable_set_orientation:
 * @orientable: a #BtkOrientable
 * @orientation: the orientable's new orientation.
 *
 * Sets the orientation of the @orientable.
 *
 * Since: 2.16
 **/
void
btk_orientable_set_orientation (BtkOrientable  *orientable,
                                BtkOrientation  orientation)
{
  g_return_if_fail (BTK_IS_ORIENTABLE (orientable));

  g_object_set (orientable,
                "orientation", orientation,
                NULL);
}

/**
 * btk_orientable_get_orientation:
 * @orientable: a #BtkOrientable
 *
 * Retrieves the orientation of the @orientable.
 *
 * Return value: the orientation of the @orientable.
 *
 * Since: 2.16
 **/
BtkOrientation
btk_orientable_get_orientation (BtkOrientable *orientable)
{
  BtkOrientation orientation;

  g_return_val_if_fail (BTK_IS_ORIENTABLE (orientable),
                        BTK_ORIENTATION_HORIZONTAL);

  g_object_get (orientable,
                "orientation", &orientation,
                NULL);

  return orientation;
}

#define __BTK_ORIENTABLE_C__
#include "btkaliasdef.c"
