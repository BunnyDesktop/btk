/* BtkPrinterCups
 * Copyright (C) 2006 John (J5) Palmieri <johnp@redhat.com>
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

#ifndef __BTK_PRINTER_CUPS_H__
#define __BTK_PRINTER_CUPS_H__

#include <bunnylib-object.h>
#include <cups/cups.h>
#include <cups/ppd.h>
#include "btkcupsutils.h"

#include <btk/btkunixprint.h>

B_BEGIN_DECLS

#define BTK_TYPE_PRINTER_CUPS                  (btk_printer_cups_get_type ())
#define BTK_PRINTER_CUPS(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PRINTER_CUPS, BtkPrinterCups))
#define BTK_PRINTER_CUPS_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_PRINTER_CUPS, BtkPrinterCupsClass))
#define BTK_IS_PRINTER_CUPS(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PRINTER_CUPS))
#define BTK_IS_PRINTER_CUPS_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_PRINTER_CUPS))
#define BTK_PRINTER_CUPS_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_PRINTER_CUPS, BtkPrinterCupsClass))

typedef struct _BtkPrinterCups	        BtkPrinterCups;
typedef struct _BtkPrinterCupsClass     BtkPrinterCupsClass;
typedef struct _BtkPrinterCupsPrivate   BtkPrinterCupsPrivate;

struct _BtkPrinterCups
{
  BtkPrinter parent_instance;

  gchar *device_uri;
  gchar *printer_uri;
  gchar *hostname;
  gint port;
  gchar **auth_info_required;

  ipp_pstate_t state;
  gboolean reading_ppd;
  gchar      *ppd_name;
  ppd_file_t *ppd_file;

  gchar  *default_cover_before;
  gchar  *default_cover_after;

  gboolean remote;
  guint get_remote_ppd_poll;
  gint  get_remote_ppd_attempts;
  BtkCupsConnectionTest *remote_cups_connection_test;
#ifdef HAVE_CUPS_API_1_6
  gboolean  avahi_browsed;
  gchar    *avahi_name;
  gchar    *avahi_type;
  gchar    *avahi_domain;
#endif
  guchar ipp_version_major;
  guchar ipp_version_minor;
  gboolean supports_copies;
  gboolean supports_collate;
  gboolean supports_number_up;
};

struct _BtkPrinterCupsClass
{
  BtkPrinterClass parent_class;

};

GType                    btk_printer_cups_get_type      (void) B_GNUC_CONST;
void                     btk_printer_cups_register_type (GTypeModule     *module);
BtkPrinterCups          *btk_printer_cups_new           (const char      *name,
							 BtkPrintBackend *backend);
ppd_file_t 		*btk_printer_cups_get_ppd       (BtkPrinterCups  *printer);
const gchar		*btk_printer_cups_get_ppd_name  (BtkPrinterCups  *printer);

B_END_DECLS

#endif /* __BTK_PRINTER_CUPS_H__ */
