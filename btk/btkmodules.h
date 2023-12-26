/* BTK - The GIMP Toolkit
 * Copyright 1998-2002 Tim Janik, Red Hat, Inc., and others.
 * Copyright (C) 2003 Alex Graveley
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

#ifndef __BTK_MODULES_H__
#define __BTK_MODULES_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btksettings.h>


G_BEGIN_DECLS


/* Functions for use within BTK+
 */
gchar * _btk_find_module        (const gchar *name,
			         const gchar *type);
gchar **_btk_get_module_path    (const gchar *type);

void    _btk_modules_init             (gint         *argc,
				       gchar      ***argv,
				       const gchar  *btk_modules_args);
void    _btk_modules_settings_changed (BtkSettings  *settings,
				       const gchar  *modules);

typedef void	 (*BtkModuleInitFunc)        (gint	  *argc,
					      gchar      ***argv);
typedef void	 (*BtkModuleDisplayInitFunc) (BdkDisplay   *display);


G_END_DECLS


#endif /* __BTK_MODULES_H__ */
