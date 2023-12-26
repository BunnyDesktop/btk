/* BTK - The GIMP Toolkit
 * btkprintcontext.h: Print Context
 * Copyright (C) 2006, Red Hat, Inc.
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

#ifndef __BTK_PRINT_CONTEXT_H__
#define __BTK_PRINT_CONTEXT_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bango/bango.h>
#include <btk/btkpagesetup.h>


B_BEGIN_DECLS

typedef struct _BtkPrintContext BtkPrintContext;

#define BTK_TYPE_PRINT_CONTEXT    (btk_print_context_get_type ())
#define BTK_PRINT_CONTEXT(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PRINT_CONTEXT, BtkPrintContext))
#define BTK_IS_PRINT_CONTEXT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PRINT_CONTEXT))

GType          btk_print_context_get_type (void) B_GNUC_CONST;


/* Rendering */
bairo_t      *btk_print_context_get_bairo_context    (BtkPrintContext *context);

BtkPageSetup *btk_print_context_get_page_setup       (BtkPrintContext *context);
gdouble       btk_print_context_get_width            (BtkPrintContext *context);
gdouble       btk_print_context_get_height           (BtkPrintContext *context);
gdouble       btk_print_context_get_dpi_x            (BtkPrintContext *context);
gdouble       btk_print_context_get_dpi_y            (BtkPrintContext *context);
gboolean      btk_print_context_get_hard_margins     (BtkPrintContext *context,
						      gdouble         *top,
						      gdouble         *bottom,
						      gdouble         *left,
						      gdouble         *right);

/* Fonts */
BangoFontMap *btk_print_context_get_bango_fontmap    (BtkPrintContext *context);
BangoContext *btk_print_context_create_bango_context (BtkPrintContext *context);
BangoLayout  *btk_print_context_create_bango_layout  (BtkPrintContext *context);

/* Needed for preview implementations */
void         btk_print_context_set_bairo_context     (BtkPrintContext *context,
						      bairo_t         *cr,
						      double           dpi_x,
						      double           dpi_y);

B_END_DECLS

#endif /* __BTK_PRINT_CONTEXT_H__ */
