/*
 * btkimmoduleime
 * Copyright (C) 2003 Takuro Ashie
 * Copyright (C) 2003 Kazuki IWAMOTO
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

#include <btk/btk.h>

extern GType btk_type_im_context_ime;

#define BTK_TYPE_IM_CONTEXT_IME            btk_type_im_context_ime
#define BTK_IM_CONTEXT_IME(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_IM_CONTEXT_IME, BtkIMContextIME))
#define BTK_IM_CONTEXT_IME_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_IM_CONTEXT_IME, BtkIMContextIMEClass))
#define BTK_IS_IM_CONTEXT_IME(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_IM_CONTEXT_IME))
#define BTK_IS_IM_CONTEXT_IME_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_IM_CONTEXT_IME))
#define BTK_IM_CONTEXT_IME_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_IM_CONTEXT_IME, BtkIMContextIMEClass))

typedef struct _BtkIMContextIME BtkIMContextIME;
typedef struct _BtkIMContextIMEPrivate BtkIMContextIMEPrivate;
typedef struct _BtkIMContextIMEClass BtkIMContextIMEClass;

struct _BtkIMContextIME
{
  BtkIMContext object;

  BdkWindow *client_window;
  BdkWindow *toplevel;
  guint use_preedit : 1;
  guint preediting : 1;
  guint opened : 1;
  guint focus : 1;
  BdkRectangle cursor_location;

  BtkIMContextIMEPrivate *priv;
};

struct _BtkIMContextIMEClass
{
  BtkIMContextClass parent_class;
};


void          btk_im_context_ime_register_type (GTypeModule * type_module);
BtkIMContext *btk_im_context_ime_new           (void);
