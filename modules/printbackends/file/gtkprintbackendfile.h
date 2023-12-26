/* BTK - The GIMP Toolkit
 * btkprintbackendpdf.h: Default implementation of BtkPrintBackend 
 * for printing to a file
 * Copyright (C) 2003, Red Hat, Inc.
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

#ifndef __BTK_PRINT_BACKEND_FILE_H__
#define __BTK_PRINT_BACKEND_FILE_H__

#include <bunnylib-object.h>
#include "btkprintbackend.h"

G_BEGIN_DECLS

#define BTK_TYPE_PRINT_BACKEND_FILE    (btk_print_backend_file_get_type ())
#define BTK_PRINT_BACKEND_FILE(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PRINT_BACKEND_FILE, BtkPrintBackendFile))
#define BTK_IS_PRINT_BACKEND_FILE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PRINT_BACKEND_FILE))

typedef struct _BtkPrintBackendFile    BtkPrintBackendFile;

BtkPrintBackend *btk_print_backend_file_new      (void);
GType            btk_print_backend_file_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __BTK_PRINT_BACKEND_FILE_H__ */
