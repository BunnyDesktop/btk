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

#ifndef __BTK_IM_CONTEXT_XIM_H__
#define __BTK_IM_CONTEXT_XIM_H__

#include <btk/btk.h>
#include "x11/bdkx.h"

G_BEGIN_DECLS

extern GType btk_type_im_context_xim;

#define BTK_TYPE_IM_CONTEXT_XIM            (btk_type_im_context_xim)
#define BTK_IM_CONTEXT_XIM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_IM_CONTEXT_XIM, BtkIMContextXIM))
#define BTK_IM_CONTEXT_XIM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_IM_CONTEXT_XIM, BtkIMContextXIMClass))
#define BTK_IS_IM_CONTEXT_XIM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_IM_CONTEXT_XIM))
#define BTK_IS_IM_CONTEXT_XIM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_IM_CONTEXT_XIM))
#define BTK_IM_CONTEXT_XIM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_IM_CONTEXT_XIM, BtkIMContextXIMClass))


typedef struct _BtkIMContextXIM       BtkIMContextXIM;
typedef struct _BtkIMContextXIMClass  BtkIMContextXIMClass;

struct _BtkIMContextXIMClass
{
  BtkIMContextClass parent_class;
};

void btk_im_context_xim_register_type (GTypeModule *type_module);
BtkIMContext *btk_im_context_xim_new (void);

void btk_im_context_xim_shutdown (void);

G_END_DECLS

#endif /* __BTK_IM_CONTEXT_XIM_H__ */
