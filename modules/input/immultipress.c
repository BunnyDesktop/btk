/* Copyright (C) 2006 Openismus GmbH
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

#include "btkimcontextmultipress.h"
#include <btk/btkimmodule.h> /* For BtkIMContextInfo */
#include <config.h>
#include <bunnylib/gi18n.h>
#include <string.h> /* For strcmp() */

#define CONTEXT_ID "multipress"
 
/** NOTE: Change the default language from "" to "*" to enable this input method by default for all locales.
 */
static const BtkIMContextInfo info = { 
  CONTEXT_ID,		   /* ID */
  N_("Multipress"),     /* Human readable name */
  GETTEXT_PACKAGE,	   /* Translation domain. Defined in configure.ac */
  MULTIPRESS_LOCALEDIR,	   /* Dir for bindtextdomain (not strictly needed for "btk+"). Defined in the Makefile.am */
  ""			   /* Languages for which this module is the default */
};

static const BtkIMContextInfo *info_list[] = {
  &info
};

#ifndef INCLUDE_IM_multipress
#define MODULE_ENTRY(type, function) G_MODULE_EXPORT type im_module_ ## function
#else
#define MODULE_ENTRY(type, function) type _btk_immodule_multipress_ ## function
#endif

MODULE_ENTRY (void, init) (GTypeModule *module)
{
  btk_im_context_multipress_register_type(module);
}

MODULE_ENTRY (void, exit) (void)
{
}

MODULE_ENTRY (void, list) (const BtkIMContextInfo ***contexts,
			   int                      *n_contexts)
{
  *contexts = info_list;
  *n_contexts = G_N_ELEMENTS (info_list);
}

MODULE_ENTRY (BtkIMContext *, create) (const bchar *context_id)
{
  if (strcmp (context_id, CONTEXT_ID) == 0)
  {
    BtkIMContext* imcontext = BTK_IM_CONTEXT(g_object_new (BTK_TYPE_IM_CONTEXT_MULTIPRESS, NULL));
    return imcontext;
  }
  else
    return NULL;
}
