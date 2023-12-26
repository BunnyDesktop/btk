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

#ifndef __BTK_IM_CONTEXT_SIMPLE_H__
#define __BTK_IM_CONTEXT_SIMPLE_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkimcontext.h>


B_BEGIN_DECLS


#define BTK_TYPE_IM_CONTEXT_SIMPLE              (btk_im_context_simple_get_type ())
#define BTK_IM_CONTEXT_SIMPLE(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_IM_CONTEXT_SIMPLE, BtkIMContextSimple))
#define BTK_IM_CONTEXT_SIMPLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_IM_CONTEXT_SIMPLE, BtkIMContextSimpleClass))
#define BTK_IS_IM_CONTEXT_SIMPLE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_IM_CONTEXT_SIMPLE))
#define BTK_IS_IM_CONTEXT_SIMPLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_IM_CONTEXT_SIMPLE))
#define BTK_IM_CONTEXT_SIMPLE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_IM_CONTEXT_SIMPLE, BtkIMContextSimpleClass))


typedef struct _BtkIMContextSimple       BtkIMContextSimple;
typedef struct _BtkIMContextSimpleClass  BtkIMContextSimpleClass;

#define BTK_MAX_COMPOSE_LEN 7

struct _BtkIMContextSimple
{
  BtkIMContext object;

  GSList *GSEAL (tables);

  guint GSEAL (compose_buffer[BTK_MAX_COMPOSE_LEN + 1]);
  gunichar GSEAL (tentative_match);
  gint GSEAL (tentative_match_len);

  guint GSEAL (in_hex_sequence) : 1;
  guint GSEAL (modifiers_dropped) : 1;
};

struct _BtkIMContextSimpleClass
{
  BtkIMContextClass parent_class;
};

GType         btk_im_context_simple_get_type  (void) B_GNUC_CONST;
BtkIMContext *btk_im_context_simple_new       (void);

void          btk_im_context_simple_add_table (BtkIMContextSimple *context_simple,
					       guint16            *data,
					       gint                max_seq_len,
					       gint                n_seqs);


B_END_DECLS


#endif /* __BTK_IM_CONTEXT_SIMPLE_H__ */
