/* BtkPrinterCupsCups
 * Copyright (C) 2006 John (J5) Palmieri  <johnp@redhat.com>
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
#include "btkprintercups.h"

static void btk_printer_cups_init       (BtkPrinterCups      *printer);
static void btk_printer_cups_class_init (BtkPrinterCupsClass *class);
static void btk_printer_cups_finalize   (BObject             *object);

static BtkPrinterClass *btk_printer_cups_parent_class;
static GType btk_printer_cups_type = 0;

void 
btk_printer_cups_register_type (GTypeModule *module)
{
  const GTypeInfo object_info =
  {
    sizeof (BtkPrinterCupsClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) btk_printer_cups_class_init,
    NULL,           /* class_finalize */
    NULL,           /* class_data */
    sizeof (BtkPrinterCups),
    0,              /* n_preallocs */
    (GInstanceInitFunc) btk_printer_cups_init,
  };

 btk_printer_cups_type = g_type_module_register_type (module,
                                                      BTK_TYPE_PRINTER,
                                                      "BtkPrinterCups",
                                                      &object_info, 0);
}

GType
btk_printer_cups_get_type (void)
{
  return btk_printer_cups_type;
}

static void
btk_printer_cups_class_init (BtkPrinterCupsClass *class)
{
  BObjectClass *object_class = (BObjectClass *) class;
	
  btk_printer_cups_parent_class = g_type_class_peek_parent (class);

  object_class->finalize = btk_printer_cups_finalize;
}

static void
btk_printer_cups_init (BtkPrinterCups *printer)
{
  printer->device_uri = NULL;
  printer->printer_uri = NULL;
  printer->state = 0;
  printer->hostname = NULL;
  printer->port = 0;
  printer->ppd_name = NULL;
  printer->ppd_file = NULL;
  printer->default_cover_before = NULL;
  printer->default_cover_after = NULL;
  printer->remote = FALSE;
  printer->get_remote_ppd_poll = 0;
  printer->get_remote_ppd_attempts = 0;
  printer->remote_cups_connection_test = NULL;
  printer->auth_info_required = NULL;
#ifdef HAVE_CUPS_API_1_6
  printer->avahi_browsed = FALSE;
  printer->avahi_name = NULL;
  printer->avahi_type = NULL;
  printer->avahi_domain = NULL;
#endif
  printer->ipp_version_major = 1;
  printer->ipp_version_minor = 1;
  printer->supports_copies = FALSE;
  printer->supports_collate = FALSE;
  printer->supports_number_up = FALSE;
}

static void
btk_printer_cups_finalize (BObject *object)
{
  BtkPrinterCups *printer;

  g_return_if_fail (object != NULL);

  printer = BTK_PRINTER_CUPS (object);

  g_free (printer->device_uri);
  g_free (printer->printer_uri);
  g_free (printer->hostname);
  g_free (printer->ppd_name);
  g_free (printer->default_cover_before);
  g_free (printer->default_cover_after);
  g_strfreev (printer->auth_info_required);

#ifdef HAVE_CUPS_API_1_6
  g_free (printer->avahi_name);
  g_free (printer->avahi_type);
  g_free (printer->avahi_domain);
#endif

  if (printer->ppd_file)
    ppdClose (printer->ppd_file);

  if (printer->get_remote_ppd_poll > 0)
    g_source_remove (printer->get_remote_ppd_poll);
  printer->get_remote_ppd_attempts = 0;

  btk_cups_connection_test_free (printer->remote_cups_connection_test);

  B_OBJECT_CLASS (btk_printer_cups_parent_class)->finalize (object);
}

/**
 * btk_printer_cups_new:
 *
 * Creates a new #BtkPrinterCups.
 *
 * Return value: a new #BtkPrinterCups
 *
 * Since: 2.10
 **/
BtkPrinterCups *
btk_printer_cups_new (const char      *name,
		      BtkPrintBackend *backend)
{
  BObject *result;
  gboolean accepts_pdf;
  BtkPrinterCups *printer;

#if (CUPS_VERSION_MAJOR == 1 && CUPS_VERSION_MINOR >= 2) || CUPS_VERSION_MAJOR > 1
  accepts_pdf = TRUE;
#else
  accepts_pdf = FALSE;
#endif

  result = g_object_new (BTK_TYPE_PRINTER_CUPS,
			 "name", name,
			 "backend", backend,
			 "is-virtual", FALSE,
			 "accepts-pdf", accepts_pdf,
                         NULL);

  printer = BTK_PRINTER_CUPS (result);

  /*
   * IPP version 1.1 has to be supported
   * by all implementations according to rfc 2911
   */
  printer->ipp_version_major = 1;
  printer->ipp_version_minor = 1;

  return printer;
}

ppd_file_t *
btk_printer_cups_get_ppd (BtkPrinterCups *printer)
{
  return printer->ppd_file;
}

const gchar *
btk_printer_cups_get_ppd_name (BtkPrinterCups  *printer)
{
  const gchar *result;

  result = printer->ppd_name;

  if (result == NULL)
    result = btk_printer_get_name (BTK_PRINTER (printer));

  return result;
}
