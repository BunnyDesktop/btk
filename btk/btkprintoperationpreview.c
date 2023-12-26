/* BTK - The GIMP Toolkit
 * btkprintoperationpreview.c: Abstract print preview interface
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

#include "config.h"

#include "btkprintoperationpreview.h"
#include "btkmarshalers.h"
#include "btkintl.h"
#include "btkalias.h"


static void btk_print_operation_preview_base_init (gpointer g_iface);

GType
btk_print_operation_preview_get_type (void)
{
  static GType print_operation_preview_type = 0;

  if (!print_operation_preview_type)
    {
      const GTypeInfo print_operation_preview_info =
      {
        sizeof (BtkPrintOperationPreviewIface), /* class_size */
	btk_print_operation_preview_base_init,   /* base_init */
	NULL,		/* base_finalize */
	NULL,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	0,
	0,              /* n_preallocs */
	NULL
      };

      print_operation_preview_type =
	g_type_register_static (B_TYPE_INTERFACE, I_("BtkPrintOperationPreview"),
				&print_operation_preview_info, 0);

      g_type_interface_add_prerequisite (print_operation_preview_type, B_TYPE_OBJECT);
    }

  return print_operation_preview_type;
}

static void
btk_print_operation_preview_base_init (gpointer g_iface)
{
  static gboolean initialized = FALSE;

  if (!initialized)
    {
      /**
       * BtkPrintOperationPreview::ready:
       * @preview: the object on which the signal is emitted
       * @context: the current #BtkPrintContext
       *
       * The ::ready signal gets emitted once per preview operation,
       * before the first page is rendered.
       * 
       * A handler for this signal can be used for setup tasks.
       */
      g_signal_new (I_("ready"),
		    BTK_TYPE_PRINT_OPERATION_PREVIEW,
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (BtkPrintOperationPreviewIface, ready),
		    NULL, NULL,
		    g_cclosure_marshal_VOID__OBJECT,
		    B_TYPE_NONE, 1,
		    BTK_TYPE_PRINT_CONTEXT);

      /**
       * BtkPrintOperationPreview::got-page-size:
       * @preview: the object on which the signal is emitted
       * @context: the current #BtkPrintContext
       * @page_setup: the #BtkPageSetup for the current page
       *
       * The ::got-page-size signal is emitted once for each page
       * that gets rendered to the preview. 
       *
       * A handler for this signal should update the @context
       * according to @page_setup and set up a suitable bairo
       * context, using btk_print_context_set_bairo_context().
       */
      g_signal_new (I_("got-page-size"),
		    BTK_TYPE_PRINT_OPERATION_PREVIEW,
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (BtkPrintOperationPreviewIface, got_page_size),
		    NULL, NULL,
		    _btk_marshal_VOID__OBJECT_OBJECT,
		    B_TYPE_NONE, 2,
		    BTK_TYPE_PRINT_CONTEXT,
		    BTK_TYPE_PAGE_SETUP);

      initialized = TRUE;
    }
}

/**
 * btk_print_operation_preview_render_page:
 * @preview: a #BtkPrintOperationPreview
 * @page_nr: the page to render
 *
 * Renders a page to the preview, using the print context that
 * was passed to the #BtkPrintOperation::preview handler together
 * with @preview.
 *
 * A custom iprint preview should use this function in its ::expose
 * handler to render the currently selected page.
 * 
 * Note that this function requires a suitable bairo context to 
 * be associated with the print context. 
 *
 * Since: 2.10 
 */
void    
btk_print_operation_preview_render_page (BtkPrintOperationPreview *preview,
					 gint			   page_nr)
{
  g_return_if_fail (BTK_IS_PRINT_OPERATION_PREVIEW (preview));

  BTK_PRINT_OPERATION_PREVIEW_GET_IFACE (preview)->render_page (preview,
								page_nr);
}

/**
 * btk_print_operation_preview_end_preview:
 * @preview: a #BtkPrintOperationPreview
 *
 * Ends a preview. 
 *
 * This function must be called to finish a custom print preview.
 *
 * Since: 2.10
 */
void
btk_print_operation_preview_end_preview (BtkPrintOperationPreview *preview)
{
  g_return_if_fail (BTK_IS_PRINT_OPERATION_PREVIEW (preview));

  BTK_PRINT_OPERATION_PREVIEW_GET_IFACE (preview)->end_preview (preview);
}

/**
 * btk_print_operation_preview_is_selected:
 * @preview: a #BtkPrintOperationPreview
 * @page_nr: a page number
 *
 * Returns whether the given page is included in the set of pages that
 * have been selected for printing.
 * 
 * Returns: %TRUE if the page has been selected for printing
 *
 * Since: 2.10
 */
gboolean
btk_print_operation_preview_is_selected (BtkPrintOperationPreview *preview,
					 gint                      page_nr)
{
  g_return_val_if_fail (BTK_IS_PRINT_OPERATION_PREVIEW (preview), FALSE);

  return BTK_PRINT_OPERATION_PREVIEW_GET_IFACE (preview)->is_selected (preview, page_nr);
}


#define __BTK_PRINT_OPERATION_PREVIEW_C__
#include "btkaliasdef.c"
