/* BTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat Software
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

#ifndef __BTK_IM_MODULE_H__
#define __BTK_IM_MODULE_H__

#include <btk/btk.h>

B_BEGIN_DECLS

typedef struct _BtkIMContextInfo BtkIMContextInfo;

struct _BtkIMContextInfo
{
  const bchar *context_id;
  const bchar *context_name;
  const bchar *domain;
  const bchar *domain_dirname;
  const bchar *default_locales;
};

/* Functions for use within BTK+
 */
void           _btk_im_module_list                   (const BtkIMContextInfo ***contexts,
						      buint                    *n_contexts);
BtkIMContext * _btk_im_module_create                 (const bchar              *context_id);
const bchar  * _btk_im_module_get_default_context_id (BdkWindow                *client_window);

/* The following entry points are exported by each input method module
 */

/*
void          im_module_list   (const BtkIMContextInfo ***contexts,
				buint                    *n_contexts);
void          im_module_init   (BtkModule                *module);
void          im_module_exit   (void);
BtkIMContext *im_module_create (const bchar              *context_id);
*/

B_END_DECLS

#endif /* __BTK_IM_MODULE_H__ */
