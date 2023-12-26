/*
 * btkimmoduleime
 * Copyright (C) 2003 Takuro Ashie
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
 *
 * $Id$
 */

#include "config.h"
#include <string.h>

#include "btk/btkintl.h"
#include "btk/btkimmodule.h"
#include "btkimcontextime.h"

static const BtkIMContextInfo ime_info = {
  "ime",
  "Windows IME",
  "btk+",
  "",
  "ja:ko:zh",
};

static const BtkIMContextInfo *info_list[] = {
  &ime_info,
};

#ifndef INCLUDE_IM_ime
#define MODULE_ENTRY(type,function) G_MODULE_EXPORT type im_module_ ## function
#else
#define MODULE_ENTRY(type, function) type _btk_immodule_ime_ ## function
#endif

MODULE_ENTRY (void, init) (GTypeModule * module)
{
  btk_im_context_ime_register_type (module);
}

MODULE_ENTRY (void, exit) (void)
{
}

MODULE_ENTRY (void, list) (const BtkIMContextInfo *** contexts, int *n_contexts)
{
  *contexts = info_list;
  *n_contexts = G_N_ELEMENTS (info_list);
}

MODULE_ENTRY (BtkIMContext *, create) (const bchar * context_id)
{
  g_return_val_if_fail (context_id, NULL);

  if (!strcmp (context_id, "ime"))
    return g_object_new (BTK_TYPE_IM_CONTEXT_IME, NULL);
  else
    return NULL;
}
