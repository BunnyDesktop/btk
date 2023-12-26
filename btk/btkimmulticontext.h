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

#ifndef __BTK_IM_MULTICONTEXT_H__
#define __BTK_IM_MULTICONTEXT_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkimcontext.h>
#include <btk/btkmenushell.h>

B_BEGIN_DECLS

#define BTK_TYPE_IM_MULTICONTEXT              (btk_im_multicontext_get_type ())
#define BTK_IM_MULTICONTEXT(obj)              (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_IM_MULTICONTEXT, BtkIMMulticontext))
#define BTK_IM_MULTICONTEXT_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_IM_MULTICONTEXT, BtkIMMulticontextClass))
#define BTK_IS_IM_MULTICONTEXT(obj)           (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_IM_MULTICONTEXT))
#define BTK_IS_IM_MULTICONTEXT_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_IM_MULTICONTEXT))
#define BTK_IM_MULTICONTEXT_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_IM_MULTICONTEXT, BtkIMMulticontextClass))


typedef struct _BtkIMMulticontext        BtkIMMulticontext;
typedef struct _BtkIMMulticontextClass   BtkIMMulticontextClass;
typedef struct _BtkIMMulticontextPrivate BtkIMMulticontextPrivate;

struct _BtkIMMulticontext
{
  BtkIMContext object;

  BtkIMContext *GSEAL (slave);

  BtkIMMulticontextPrivate *GSEAL (priv);

  gchar *GSEAL (context_id);
};

struct _BtkIMMulticontextClass
{
  BtkIMContextClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

GType         btk_im_multicontext_get_type (void) B_GNUC_CONST;
BtkIMContext *btk_im_multicontext_new      (void);

void          btk_im_multicontext_append_menuitems (BtkIMMulticontext *context,
						    BtkMenuShell      *menushell);
const char  * btk_im_multicontext_get_context_id   (BtkIMMulticontext *context);

void          btk_im_multicontext_set_context_id   (BtkIMMulticontext *context,
                                                    const char        *context_id);
 
B_END_DECLS

#endif /* __BTK_IM_MULTICONTEXT_H__ */
