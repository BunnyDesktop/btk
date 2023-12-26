/* BtkPrinterPapi
 * Copyright (C) 2006 John (J5) Palmieri <johnp@redhat.com>
 * Copyright (C) 2009 Ghee Teo <ghee.teo@sun.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BTK_PRINTER_PAPI_H__
#define __BTK_PRINTER_PAPI_H__

#include <bunnylib.h>
#include <bunnylib-object.h>
#include <papi.h>

#include "btkunixprint.h"

G_BEGIN_DECLS

#define BTK_TYPE_PRINTER_PAPI                  (btk_printer_papi_get_type ())
#define BTK_PRINTER_PAPI(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PRINTER_PAPI, BtkPrinterPapi))
#define BTK_PRINTER_PAPI_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_PRINTER_PAPI, BtkPrinterPapiClass))
#define BTK_IS_PRINTER_PAPI(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PRINTER_PAPI))
#define BTK_IS_PRINTER_PAPI_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_PRINTER_PAPI))
#define BTK_PRINTER_PAPI_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_PRINTER_PAPI, BtkPrinterPapiClass))

typedef struct _BtkPrinterPapi	        BtkPrinterPapi;
typedef struct _BtkPrinterPapiClass     BtkPrinterPapiClass;
typedef struct _BtkPrinterPapiPrivate   BtkPrinterPapiPrivate;

struct _BtkPrinterPapi
{
  BtkPrinter parent_instance;

  gchar *printer_name;
};

struct _BtkPrinterPapiClass
{
  BtkPrinterClass parent_class;

};

GType                    btk_printer_papi_get_type      (void) G_GNUC_CONST;
void                     btk_printer_papi_register_type (GTypeModule     *module);
BtkPrinterPapi          *btk_printer_papi_new           (const char      *name, BtkPrintBackend *backend);

G_END_DECLS

#endif /* __BTK_PRINTER_PAPI_H__ */
