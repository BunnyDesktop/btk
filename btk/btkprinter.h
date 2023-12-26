/* BtkPrinter
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

#ifndef __BTK_PRINTER_H__
#define __BTK_PRINTER_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_UNIX_PRINT_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btkunixprint.h> can be included directly."
#endif

#include <bairo.h>
#include <btk/btk.h>

B_BEGIN_DECLS

#define BTK_TYPE_PRINT_CAPABILITIES (btk_print_capabilities_get_type ())

/* Note, this type is manually registered with GObject in btkprinter.c
 * If you add any flags, update the registration as well!
 */
typedef enum
{
  BTK_PRINT_CAPABILITY_PAGE_SET         = 1 << 0,
  BTK_PRINT_CAPABILITY_COPIES           = 1 << 1,
  BTK_PRINT_CAPABILITY_COLLATE          = 1 << 2,
  BTK_PRINT_CAPABILITY_REVERSE          = 1 << 3,
  BTK_PRINT_CAPABILITY_SCALE            = 1 << 4,
  BTK_PRINT_CAPABILITY_GENERATE_PDF     = 1 << 5,
  BTK_PRINT_CAPABILITY_GENERATE_PS      = 1 << 6,
  BTK_PRINT_CAPABILITY_PREVIEW          = 1 << 7,
  BTK_PRINT_CAPABILITY_NUMBER_UP        = 1 << 8,
  BTK_PRINT_CAPABILITY_NUMBER_UP_LAYOUT = 1 << 9
} BtkPrintCapabilities;

GType btk_print_capabilities_get_type (void) B_GNUC_CONST;

#define BTK_TYPE_PRINTER                  (btk_printer_get_type ())
#define BTK_PRINTER(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PRINTER, BtkPrinter))
#define BTK_PRINTER_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_PRINTER, BtkPrinterClass))
#define BTK_IS_PRINTER(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PRINTER))
#define BTK_IS_PRINTER_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_PRINTER))
#define BTK_PRINTER_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_PRINTER, BtkPrinterClass))

typedef struct _BtkPrinter          BtkPrinter;
typedef struct _BtkPrinterClass     BtkPrinterClass;
typedef struct _BtkPrinterPrivate   BtkPrinterPrivate;
typedef struct _BtkPrintBackend     BtkPrintBackend;

struct _BtkPrintBackend;

struct _BtkPrinter
{
  GObject parent_instance;

  BtkPrinterPrivate *GSEAL (priv);
};

struct _BtkPrinterClass
{
  GObjectClass parent_class;

  void (*details_acquired) (BtkPrinter *printer,
                            gboolean    success);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
  void (*_btk_reserved5) (void);
  void (*_btk_reserved6) (void);
  void (*_btk_reserved7) (void);
};

GType                    btk_printer_get_type              (void) B_GNUC_CONST;
BtkPrinter              *btk_printer_new                   (const gchar     *name,
							    BtkPrintBackend *backend,
							    gboolean         virtual_);
BtkPrintBackend         *btk_printer_get_backend           (BtkPrinter      *printer);
const gchar *            btk_printer_get_name              (BtkPrinter      *printer);
const gchar *            btk_printer_get_state_message     (BtkPrinter      *printer);
const gchar *            btk_printer_get_description       (BtkPrinter      *printer);
const gchar *            btk_printer_get_location          (BtkPrinter      *printer);
const gchar *            btk_printer_get_icon_name         (BtkPrinter      *printer);
gint                     btk_printer_get_job_count         (BtkPrinter      *printer);
gboolean                 btk_printer_is_active             (BtkPrinter      *printer);
gboolean                 btk_printer_is_paused             (BtkPrinter      *printer);
gboolean                 btk_printer_is_accepting_jobs     (BtkPrinter      *printer);
gboolean                 btk_printer_is_virtual            (BtkPrinter      *printer);
gboolean                 btk_printer_is_default            (BtkPrinter      *printer);
gboolean                 btk_printer_accepts_pdf           (BtkPrinter      *printer);
gboolean                 btk_printer_accepts_ps            (BtkPrinter      *printer);
GList                   *btk_printer_list_papers           (BtkPrinter      *printer);
BtkPageSetup            *btk_printer_get_default_page_size (BtkPrinter      *printer);
gint                     btk_printer_compare               (BtkPrinter *a,
						    	    BtkPrinter *b);
gboolean                 btk_printer_has_details           (BtkPrinter       *printer);
void                     btk_printer_request_details       (BtkPrinter       *printer);
BtkPrintCapabilities     btk_printer_get_capabilities      (BtkPrinter       *printer);
gboolean                 btk_printer_get_hard_margins      (BtkPrinter       *printer,
                                                            gdouble          *top,
                                                            gdouble          *bottom,
                                                            gdouble          *left,
                                                            gdouble          *right);

typedef gboolean (*BtkPrinterFunc) (BtkPrinter *printer,
				    gpointer    data);

void                     btk_enumerate_printers        (BtkPrinterFunc   func,
							gpointer         data,
							GDestroyNotify   destroy,
							gboolean         wait);

B_END_DECLS

#endif /* __BTK_PRINTER_H__ */
