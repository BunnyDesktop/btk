/* BTK - The GIMP Toolkit
 * btkprintoperationpreview.h: Abstract print preview interface
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

#ifndef __BTK_PRINT_OPERATION_PREVIEW_H__
#define __BTK_PRINT_OPERATION_PREVIEW_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bairo.h>
#include <btk/btkprintcontext.h>

B_BEGIN_DECLS

#define BTK_TYPE_PRINT_OPERATION_PREVIEW                  (btk_print_operation_preview_get_type ())
#define BTK_PRINT_OPERATION_PREVIEW(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PRINT_OPERATION_PREVIEW, BtkPrintOperationPreview))
#define BTK_IS_PRINT_OPERATION_PREVIEW(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PRINT_OPERATION_PREVIEW))
#define BTK_PRINT_OPERATION_PREVIEW_GET_IFACE(obj)        (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BTK_TYPE_PRINT_OPERATION_PREVIEW, BtkPrintOperationPreviewIface))

typedef struct _BtkPrintOperationPreview      BtkPrintOperationPreview;      /*dummy typedef */
typedef struct _BtkPrintOperationPreviewIface BtkPrintOperationPreviewIface;


struct _BtkPrintOperationPreviewIface
{
  GTypeInterface g_iface;

  /* signals */
  void              (*ready)          (BtkPrintOperationPreview *preview,
				       BtkPrintContext          *context);
  void              (*got_page_size)  (BtkPrintOperationPreview *preview,
				       BtkPrintContext          *context,
				       BtkPageSetup             *page_setup);

  /* methods */
  void              (*render_page)    (BtkPrintOperationPreview *preview,
				       gint                      page_nr);
  gboolean          (*is_selected)    (BtkPrintOperationPreview *preview,
				       gint                      page_nr);
  void              (*end_preview)    (BtkPrintOperationPreview *preview);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
  void (*_btk_reserved5) (void);
  void (*_btk_reserved6) (void);
  void (*_btk_reserved7) (void);
};

GType   btk_print_operation_preview_get_type       (void) B_GNUC_CONST;

void     btk_print_operation_preview_render_page (BtkPrintOperationPreview *preview,
						  gint                      page_nr);
void     btk_print_operation_preview_end_preview (BtkPrintOperationPreview *preview);
gboolean btk_print_operation_preview_is_selected (BtkPrintOperationPreview *preview,
						  gint                      page_nr);

B_END_DECLS

#endif /* __BTK_PRINT_OPERATION_PREVIEW_H__ */
