/* BTK+ Pixbuf Engine
 * Copyright (C) 1998-2000 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Written by Owen Taylor <otaylor@redhat.com>, based on code by
 * Carsten Haitzler <raster@rasterman.com>
 */

#include "pixbuf.h"
#include "pixbuf-style.h"
#include "pixbuf-rc-style.h"
#include <bmodule.h>

G_MODULE_EXPORT void
theme_init (GTypeModule *module)
{
  pixbuf_rc_style_register_type (module);
  pixbuf_style_register_type (module);
}

G_MODULE_EXPORT void
theme_exit (void)
{
}

G_MODULE_EXPORT BtkRcStyle *
theme_create_rc_style (void)
{
  return g_object_new (PIXBUF_TYPE_RC_STYLE, NULL);
}

/* The following function will be called by BTK+ when the module
 * is loaded and checks to see if we are compatible with the
 * version of BTK+ that loads us.
 */
G_MODULE_EXPORT const bchar* g_module_check_init (GModule *module);
const bchar*
g_module_check_init (GModule *module)
{
  return btk_check_version (BTK_MAJOR_VERSION,
			    BTK_MINOR_VERSION,
			    BTK_MICRO_VERSION - BTK_INTERFACE_AGE);
}
