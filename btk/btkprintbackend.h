/* BTK - The GIMP Toolkit
 * btkprintbackend.h: Abstract printer backend interfaces
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

#ifndef __BTK_PRINT_BACKEND_H__
#define __BTK_PRINT_BACKEND_H__

/* This is a "semi-private" header; it is meant only for
 * alternate BtkPrintDialog backend modules; no stability guarantees
 * are made at this point
 */
#ifndef BTK_PRINT_BACKEND_ENABLE_UNSUPPORTED
#error "BtkPrintBackend is not supported API for general use"
#endif

#include <btk/btk.h>
#include <btk/btkunixprint.h>
#include <btk/btkprinteroptionset.h>

B_BEGIN_DECLS

typedef struct _BtkPrintBackendClass    BtkPrintBackendClass;
typedef struct _BtkPrintBackendPrivate  BtkPrintBackendPrivate;

#define BTK_PRINT_BACKEND_ERROR (btk_print_backend_error_quark ())

typedef enum
{
  /* TODO: add specific errors */
  BTK_PRINT_BACKEND_ERROR_GENERIC
} BtkPrintBackendError;

GQuark     btk_print_backend_error_quark      (void);

#define BTK_TYPE_PRINT_BACKEND                  (btk_print_backend_get_type ())
#define BTK_PRINT_BACKEND(obj)                  (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PRINT_BACKEND, BtkPrintBackend))
#define BTK_PRINT_BACKEND_CLASS(klass)          (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_PRINT_BACKEND, BtkPrintBackendClass))
#define BTK_IS_PRINT_BACKEND(obj)               (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PRINT_BACKEND))
#define BTK_IS_PRINT_BACKEND_CLASS(klass)       (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_PRINT_BACKEND))
#define BTK_PRINT_BACKEND_GET_CLASS(obj)        (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_PRINT_BACKEND, BtkPrintBackendClass))

typedef enum 
{
  BTK_PRINT_BACKEND_STATUS_UNKNOWN,
  BTK_PRINT_BACKEND_STATUS_OK,
  BTK_PRINT_BACKEND_STATUS_UNAVAILABLE
} BtkPrintBackendStatus;

struct _BtkPrintBackend
{
  BObject parent_instance;

  BtkPrintBackendPrivate *priv;
};

struct _BtkPrintBackendClass
{
  BObjectClass parent_class;

  /* Global backend methods: */
  void                   (*request_printer_list)            (BtkPrintBackend        *backend);
  void                   (*print_stream)                    (BtkPrintBackend        *backend,
							     BtkPrintJob            *job,
							     BUNNYIOChannel             *data_io,
							     BtkPrintJobCompleteFunc callback,
							     bpointer                user_data,
							     GDestroyNotify          dnotify);

  /* Printer methods: */
  void                  (*printer_request_details)           (BtkPrinter          *printer);
  bairo_surface_t *     (*printer_create_bairo_surface)      (BtkPrinter          *printer,
							      BtkPrintSettings    *settings,
							      bdouble              height,
							      bdouble              width,
							      BUNNYIOChannel          *cache_io);
  BtkPrinterOptionSet * (*printer_get_options)               (BtkPrinter          *printer,
							      BtkPrintSettings    *settings,
							      BtkPageSetup        *page_setup,
							      BtkPrintCapabilities capabilities);
  bboolean              (*printer_mark_conflicts)            (BtkPrinter          *printer,
							      BtkPrinterOptionSet *options);
  void                  (*printer_get_settings_from_options) (BtkPrinter          *printer,
							      BtkPrinterOptionSet *options,
							      BtkPrintSettings    *settings);
  void                  (*printer_prepare_for_print)         (BtkPrinter          *printer,
							      BtkPrintJob         *print_job,
							      BtkPrintSettings    *settings,
							      BtkPageSetup        *page_setup);
  GList  *              (*printer_list_papers)               (BtkPrinter          *printer);
  BtkPageSetup *        (*printer_get_default_page_size)     (BtkPrinter          *printer);
  bboolean              (*printer_get_hard_margins)          (BtkPrinter          *printer,
							      bdouble             *top,
							      bdouble             *bottom,
							      bdouble             *left,
							      bdouble             *right);
  BtkPrintCapabilities  (*printer_get_capabilities)          (BtkPrinter          *printer);

  /* Signals */
  void                  (*printer_list_changed)              (BtkPrintBackend     *backend);
  void                  (*printer_list_done)                 (BtkPrintBackend     *backend);
  void                  (*printer_added)                     (BtkPrintBackend     *backend,
							      BtkPrinter          *printer);
  void                  (*printer_removed)                   (BtkPrintBackend     *backend,
							      BtkPrinter          *printer);
  void                  (*printer_status_changed)            (BtkPrintBackend     *backend,
							      BtkPrinter          *printer);
  void                  (*request_password)                  (BtkPrintBackend     *backend,
                                                              bpointer             auth_info_required,
                                                              bpointer             auth_info_default,
                                                              bpointer             auth_info_display,
                                                              bpointer             auth_info_visible,
                                                              const bchar         *prompt);

  /* not a signal */
  void                  (*set_password)                      (BtkPrintBackend     *backend,
                                                              bchar              **auth_info_required,
                                                              bchar              **auth_info);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

GType   btk_print_backend_get_type       (void) B_GNUC_CONST;

GList      *btk_print_backend_get_printer_list     (BtkPrintBackend         *print_backend);
bboolean    btk_print_backend_printer_list_is_done (BtkPrintBackend         *print_backend);
BtkPrinter *btk_print_backend_find_printer         (BtkPrintBackend         *print_backend,
						    const bchar             *printer_name);
void        btk_print_backend_print_stream         (BtkPrintBackend         *print_backend,
						    BtkPrintJob             *job,
						    BUNNYIOChannel              *data_io,
						    BtkPrintJobCompleteFunc  callback,
						    bpointer                 user_data,
						    GDestroyNotify           dnotify);
GList *     btk_print_backend_load_modules         (void);
void        btk_print_backend_destroy              (BtkPrintBackend         *print_backend);
void        btk_print_backend_set_password         (BtkPrintBackend         *backend, 
                                                    bchar                  **auth_info_required,
                                                    bchar                  **auth_info);

/* Backend-only functions for BtkPrintBackend */

void        btk_print_backend_add_printer          (BtkPrintBackend         *print_backend,
						    BtkPrinter              *printer);
void        btk_print_backend_remove_printer       (BtkPrintBackend         *print_backend,
						    BtkPrinter              *printer);
void        btk_print_backend_set_list_done        (BtkPrintBackend         *backend);


/* Backend-only functions for BtkPrinter */
bboolean    btk_printer_is_new                (BtkPrinter      *printer);
void        btk_printer_set_accepts_pdf       (BtkPrinter      *printer,
					       bboolean         val);
void        btk_printer_set_accepts_ps        (BtkPrinter      *printer,
					       bboolean         val);
void        btk_printer_set_is_new            (BtkPrinter      *printer,
					       bboolean         val);
void        btk_printer_set_is_active         (BtkPrinter      *printer,
					       bboolean         val);
bboolean    btk_printer_set_is_paused         (BtkPrinter      *printer,
					       bboolean         val);
bboolean    btk_printer_set_is_accepting_jobs (BtkPrinter      *printer,
					       bboolean         val);
void        btk_printer_set_has_details       (BtkPrinter      *printer,
					       bboolean         val);
void        btk_printer_set_is_default        (BtkPrinter      *printer,
					       bboolean         val);
void        btk_printer_set_icon_name         (BtkPrinter      *printer,
					       const bchar     *icon);
bboolean    btk_printer_set_job_count         (BtkPrinter      *printer,
					       bint             count);
bboolean    btk_printer_set_location          (BtkPrinter      *printer,
					       const bchar     *location);
bboolean    btk_printer_set_description       (BtkPrinter      *printer,
					       const bchar     *description);
bboolean    btk_printer_set_state_message     (BtkPrinter      *printer,
					       const bchar     *message);


B_END_DECLS

#endif /* __BTK_PRINT_BACKEND_H__ */
