/* BtkPrinterPapi
 * Copyright (C) 2006 John (J5) Palmieri  <johnp@redhat.com>
 * Copyright (C) 2009 Ghee Teo <ghee.teo@sun.com>
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
#include "btkprinterpapi.h"

static void btk_printer_papi_init       (BtkPrinterPapi      *printer);
static void btk_printer_papi_class_init (BtkPrinterPapiClass *class);
static void btk_printer_papi_finalize   (BObject             *object);

static BtkPrinterClass *btk_printer_papi_parent_class;
static GType btk_printer_papi_type = 0;

void 
btk_printer_papi_register_type (GTypeModule *module)
{
  const GTypeInfo object_info =
  {
    sizeof (BtkPrinterPapiClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) btk_printer_papi_class_init,
    NULL,           /* class_finalize */
    NULL,           /* class_data */
    sizeof (BtkPrinterPapi),
    0,              /* n_preallocs */
    (GInstanceInitFunc) btk_printer_papi_init,
  };

 btk_printer_papi_type = g_type_module_register_type (module,
                                                      BTK_TYPE_PRINTER,
                                                      "BtkPrinterPapi",
                                                      &object_info, 0);
}

GType
btk_printer_papi_get_type (void)
{
  return btk_printer_papi_type;
}

static void
btk_printer_papi_class_init (BtkPrinterPapiClass *class)
{
  BObjectClass *object_class = (BObjectClass *) class;
	
  btk_printer_papi_parent_class = g_type_class_peek_parent (class);

  object_class->finalize = btk_printer_papi_finalize;
}

static void
btk_printer_papi_init (BtkPrinterPapi *printer)
{
  printer->printer_name = NULL;
}

static void
btk_printer_papi_finalize (BObject *object)
{
  BtkPrinterPapi *printer;

  g_return_if_fail (object != NULL);

  printer = BTK_PRINTER_PAPI (object);

  g_free(printer->printer_name);

  B_OBJECT_CLASS (btk_printer_papi_parent_class)->finalize (object);
}

/**
 * btk_printer_papi_new:
 *
 * Creates a new #BtkPrinterPapi.
 *
 * Return value: a new #BtkPrinterPapi
 *
 * Since: 2.10
 **/
BtkPrinterPapi *
btk_printer_papi_new (const char      *name,
		      BtkPrintBackend *backend)
{
  BObject *result;
  BtkPrinterPapi *pp;
  
  result = g_object_new (BTK_TYPE_PRINTER_PAPI,
			 "name", name,
			 "backend", backend,
			 "is-virtual", TRUE,
                         NULL);
  pp = BTK_PRINTER_PAPI(result);

  pp->printer_name = g_strdup (name);

  return (BtkPrinterPapi *) pp;
}

