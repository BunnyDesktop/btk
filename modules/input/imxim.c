/* BTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
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
#include "btk/btkintl.h"
#include "btk/btkimmodule.h"
#include "btkimcontextxim.h"
#include <string.h>

static const BtkIMContextInfo xim_ja_info = { 
  "xim",		           /* ID */
  N_("X Input Method"),            /* Human readable name */
  GETTEXT_PACKAGE,		   /* Translation domain */
  BTK_LOCALEDIR,		   /* Dir for bindtextdomain (not strictly needed for "btk+") */
  "ko:ja:th:zh"		           /* Languages for which this module is the default */
};

static const BtkIMContextInfo *info_list[] = {
  &xim_ja_info
};

#ifndef INCLUDE_IM_xim
#define MODULE_ENTRY(type, function) G_MODULE_EXPORT type im_module_ ## function
#else
#define MODULE_ENTRY(type, function) type _btk_immodule_xim_ ## function
#endif

MODULE_ENTRY (void, init) (GTypeModule *type_module)
{
  btk_im_context_xim_register_type (type_module);
}

MODULE_ENTRY (void, exit) (void)
{
  btk_im_context_xim_shutdown ();
}

MODULE_ENTRY (void, list) (const BtkIMContextInfo ***contexts,
			   int                      *n_contexts)
{
  *contexts = info_list;
  *n_contexts = G_N_ELEMENTS (info_list);
}

MODULE_ENTRY (BtkIMContext *, create) (const bchar *context_id)
{
  if (strcmp (context_id, "xim") == 0)
    return btk_im_context_xim_new ();
  else
    return NULL;
}
