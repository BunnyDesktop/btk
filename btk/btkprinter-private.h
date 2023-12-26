/* BTK - The GIMP Toolkit
 * btkprintoperation.h: Print Operation
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

#ifndef __BTK_PRINTER_PRIVATE_H__
#define __BTK_PRINTER_PRIVATE_H__

#include <btk/btk.h>
#include <btk/btkunixprint.h>
#include "btkprinteroptionset.h"

B_BEGIN_DECLS

BtkPrinterOptionSet *_btk_printer_get_options               (BtkPrinter          *printer,
							     BtkPrintSettings    *settings,
							     BtkPageSetup        *page_setup,
							     BtkPrintCapabilities capabilities);
bboolean             _btk_printer_mark_conflicts            (BtkPrinter          *printer,
							     BtkPrinterOptionSet *options);
void                 _btk_printer_get_settings_from_options (BtkPrinter          *printer,
							     BtkPrinterOptionSet *options,
							     BtkPrintSettings    *settings);
void                 _btk_printer_prepare_for_print         (BtkPrinter          *printer,
							     BtkPrintJob         *print_job,
							     BtkPrintSettings    *settings,
							     BtkPageSetup        *page_setup);
bairo_surface_t *    _btk_printer_create_bairo_surface      (BtkPrinter          *printer,
							     BtkPrintSettings    *settings,
							     bdouble              width,
							     bdouble              height,
							     BUNNYIOChannel          *cache_io);
GHashTable *         _btk_printer_get_custom_widgets        (BtkPrinter          *printer);

/* BtkPrintJob private methods: */
void btk_print_job_set_status (BtkPrintJob   *job,
			       BtkPrintStatus status);

B_END_DECLS
#endif /* __BTK_PRINT_OPERATION_PRIVATE_H__ */
