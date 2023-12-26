/* BTK - The GIMP Toolkit
 * btkprintbackendcups.h: Default implementation of BtkPrintBackend for the Common Unix Print System (CUPS)
 * Copyright (C) 2006, 2007 Red Hat, Inc.
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

#ifndef __BTK_PRINT_BACKEND_CUPS_H__
#define __BTK_PRINT_BACKEND_CUPS_H__

#include <bunnylib-object.h>
#include "btkprintbackend.h"

B_BEGIN_DECLS

#define BTK_TYPE_PRINT_BACKEND_CUPS             (btk_print_backend_cups_get_type ())
#define BTK_PRINT_BACKEND_CUPS(obj)             (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PRINT_BACKEND_CUPS, BtkPrintBackendCups))
#define BTK_IS_PRINT_BACKEND_CUPS(obj)          (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PRINT_BACKEND_CUPS))

typedef struct _BtkPrintBackendCups      BtkPrintBackendCups;

BtkPrintBackend *btk_print_backend_cups_new      (void);
GType          btk_print_backend_cups_get_type (void) B_GNUC_CONST;

B_END_DECLS

#endif /* __BTK_PRINT_BACKEND_CUPS_H__ */


