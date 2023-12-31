/* BTK - The GIMP Toolkit
 * btkprintbackendpapi.c: Default implementation of BtkPrintBackend 
 * for printing to papi 
 * Copyright (C) 2003, Red Hat, Inc.
 * Copyright (C) 2009, Sun Microsystems, Inc.
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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <papi.h>

#include <config.h>
#include <errno.h>
#include <bairo.h>
#include <bairo-ps.h>

#include <bunnylib/gi18n-lib.h>

#include "btk.h"
#include "btkprintbackendpapi.h"
#include "btkprinterpapi.h"
#include "btkprinter-private.h"

typedef struct _BtkPrintBackendPapiClass BtkPrintBackendPapiClass;

#define BTK_PRINT_BACKEND_PAPI_CLASS(klass)     (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_PRINT_BACKEND_PAPI, BtkPrintBackendPapiClass))
#define BTK_IS_PRINT_BACKEND_PAPI_CLASS(klass)  (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_PRINT_BACKEND_PAPI))
#define BTK_PRINT_BACKEND_PAPI_GET_CLASS(obj)   (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_PRINT_BACKEND_PAPI, BtkPrintBackendPapiClass))

#define _PAPI_MAX_CHUNK_SIZE 8192

static GType print_backend_papi_type = 0;

struct _BtkPrintBackendPapiClass
{
  BtkPrintBackendClass parent_class;
};

struct _BtkPrintBackendPapi
{
  BtkPrintBackend parent_instance;

  char *default_printer;  
};

typedef struct {
  BtkPrinter *printer;
} _PrinterStatus;

static BObjectClass *backend_parent_class;

static void                 btk_print_backend_papi_class_init      (BtkPrintBackendPapiClass *class);
static void                 btk_print_backend_papi_init            (BtkPrintBackendPapi      *impl);
static void                 btk_print_backend_papi_finalize        (BObject                  *object);
static void                 btk_print_backend_papi_dispose         (BObject                  *object);
static void                 papi_request_printer_list              (BtkPrintBackend          *print_backend);
static bboolean 	    papi_get_printer_list 		   (BtkPrintBackendPapi      *papi_backend);
static void                 papi_printer_request_details           (BtkPrinter               *printer);
static BtkPrintCapabilities papi_printer_get_capabilities          (BtkPrinter               *printer);
static void                 papi_printer_get_settings_from_options (BtkPrinter               *printer,
								   BtkPrinterOptionSet       *options,
								   BtkPrintSettings          *settings);
static BtkPrinterOptionSet *papi_printer_get_options               (BtkPrinter               *printer,
								   BtkPrintSettings          *settings,
								   BtkPageSetup              *page_setup,
								   BtkPrintCapabilities       capabilities);
static void                 papi_printer_prepare_for_print         (BtkPrinter               *printer,
								   BtkPrintJob               *print_job,
								   BtkPrintSettings          *settings,
								   BtkPageSetup              *page_setup);
static bairo_surface_t *    papi_printer_create_bairo_surface      (BtkPrinter               *printer,
								   BtkPrintSettings          *settings,
								   bdouble                   width,
								   bdouble                   height,
								   BUNNYIOChannel                *cache_io);
static void                 btk_print_backend_papi_print_stream    (BtkPrintBackend          *print_backend,
								   BtkPrintJob               *job,
								   BUNNYIOChannel                *data_io,
								   BtkPrintJobCompleteFunc   callback,
								   bpointer                  user_data,
								   GDestroyNotify            dnotify);

static bboolean             papi_display_printer_status            (bpointer user_data);
static void                 papi_display_printer_status_done       (bpointer user_data);

static void
btk_print_backend_papi_register_type (GTypeModule *module)
{
  const GTypeInfo print_backend_papi_info =
  {
    sizeof (BtkPrintBackendPapiClass),
    NULL,		/* base_init */
    NULL,		/* base_finalize */
    (GClassInitFunc) btk_print_backend_papi_class_init,
    NULL,		/* class_finalize */
    NULL,		/* class_data */
    sizeof (BtkPrintBackendPapi),
    0,		/* n_preallocs */
    (GInstanceInitFunc) btk_print_backend_papi_init,
  };

  print_backend_papi_type = g_type_module_register_type (module,
                                                        BTK_TYPE_PRINT_BACKEND,
                                                        "BtkPrintBackendPapi",
                                                        &print_backend_papi_info, 0);
}

G_MODULE_EXPORT void 
pb_module_init (GTypeModule *module)
{
  btk_print_backend_papi_register_type (module);
  btk_printer_papi_register_type (module);
}

G_MODULE_EXPORT void 
pb_module_exit (void)
{

}
  
G_MODULE_EXPORT BtkPrintBackend * 
pb_module_create (void)
{
  return btk_print_backend_papi_new ();
}

/*
 * BtkPrintBackendPapi
 */
GType
btk_print_backend_papi_get_type (void)
{
  return print_backend_papi_type;
}

/**
 * btk_print_backend_papi_new:
 *
 * Creates a new #BtkPrintBackendPapi object. #BtkPrintBackendPapi
 * implements the #BtkPrintBackend interface with direct access to
 * the filesystem using Unix/Linux API calls
 *
 * Return value: the new #BtkPrintBackendPapi object
 **/
BtkPrintBackend *
btk_print_backend_papi_new (void)
{
  return g_object_new (BTK_TYPE_PRINT_BACKEND_PAPI, NULL);
}

static void
btk_print_backend_papi_class_init (BtkPrintBackendPapiClass *class)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (class);
  BtkPrintBackendClass *backend_class = BTK_PRINT_BACKEND_CLASS (class);
  
  backend_parent_class = g_type_class_peek_parent (class);

  bobject_class->finalize = btk_print_backend_papi_finalize;
  bobject_class->dispose = btk_print_backend_papi_dispose;

  backend_class->request_printer_list = papi_request_printer_list;
  backend_class->printer_request_details = papi_printer_request_details;
  backend_class->printer_get_capabilities = papi_printer_get_capabilities;
  backend_class->printer_get_options = papi_printer_get_options;
  backend_class->printer_get_settings_from_options = papi_printer_get_settings_from_options;
  backend_class->printer_prepare_for_print = papi_printer_prepare_for_print;
  backend_class->printer_create_bairo_surface = papi_printer_create_bairo_surface;
  backend_class->print_stream = btk_print_backend_papi_print_stream;
}

static bairo_status_t
_bairo_write (void                *closure,
              const unsigned char *data,
              unsigned int         length)
{
  BUNNYIOChannel *io = (BUNNYIOChannel *)closure;
  bsize written;
  GError *error = NULL;

  BTK_NOTE (PRINTING,
            g_print ("PAPI Backend: Writting %i byte chunk to temp file\n", length));

  while (length > 0) 
    {
      g_io_channel_write_chars (io, (char *)data, length, &written, &error);

      if (error != NULL)
	{
	  BTK_NOTE (PRINTING,
                     g_print ("PAPI Backend: Error writting to temp file, %s\n", error->message));

          g_error_free (error);
	  return BAIRO_STATUS_WRITE_ERROR;
	}    

      BTK_NOTE (PRINTING,
                g_print ("PAPI Backend: Wrote %i bytes to temp file\n", written));

      data += written;
      length -= written;
    }

  return BAIRO_STATUS_SUCCESS;
}

static bairo_surface_t *
papi_printer_create_bairo_surface (BtkPrinter       *printer,
				  BtkPrintSettings *settings,
				  bdouble           width, 
				  bdouble           height,
				  BUNNYIOChannel       *cache_io)
{
  bairo_surface_t *surface;
  
  surface = bairo_ps_surface_create_for_stream (_bairo_write, cache_io, width, height);

  bairo_surface_set_fallback_resolution (surface,
                                         2.0 * btk_print_settings_get_printer_lpi (settings),
                                         2.0 * btk_print_settings_get_printer_lpi (settings));

  return surface;
}

typedef struct {
  BtkPrintBackend *backend;
  BtkPrintJobCompleteFunc callback;
  BtkPrintJob *job;
  bpointer user_data;
  GDestroyNotify dnotify;

  papi_service_t service;
  papi_stream_t stream;
} _PrintStreamData;

static void
papi_print_cb (BtkPrintBackendPapi *print_backend,
              GError             *error,
              bpointer            user_data)
{
  _PrintStreamData *ps = (_PrintStreamData *) user_data;

  if (ps->callback)
    ps->callback (ps->job, ps->user_data, error);

  if (ps->dnotify)
    ps->dnotify (ps->user_data);

  btk_print_job_set_status (ps->job, 
			    error ? BTK_PRINT_STATUS_FINISHED_ABORTED 
			          : BTK_PRINT_STATUS_FINISHED);

  if (ps->job)
    g_object_unref (ps->job);
  
  g_free (ps);
}

static bboolean
papi_write (BUNNYIOChannel   *source,
           BUNNYIOCondition  con,
           bpointer      user_data)
{
  bchar buf[_PAPI_MAX_CHUNK_SIZE];
  bsize bytes_read;
  GError *error;
  BUNNYIOStatus status;
  _PrintStreamData *ps = (_PrintStreamData *) user_data;
  papi_job_t job = NULL;

  error = NULL;
  status = g_io_channel_read_chars (source,
                                    buf,
                                    _PAPI_MAX_CHUNK_SIZE,
                                    &bytes_read,
                                    &error);

  /* Keep writing to PAPI input stream while there are data */
  if (status != G_IO_STATUS_ERROR)
  {
     papiJobStreamWrite (ps->service, ps->stream, buf, bytes_read);
  }
  
  /* Finish reading input stream data. Closing the stream and handle to service */
  if (bytes_read == 0) {
     papiJobStreamClose (ps->service, ps->stream, &job);
     ps->stream = NULL;
     papiJobFree (job);
     papiServiceDestroy (ps->service);
     ps->service = NULL;
  }

  if (error != NULL || status == G_IO_STATUS_EOF)
    {
      papi_print_cb (BTK_PRINT_BACKEND_PAPI (ps->backend), 
		    error, user_data);

      if (error)
	g_error_free (error);

      if (error != NULL)
        {
          BTK_NOTE (PRINTING,
                    g_print ("PAPI Backend: %s\n", error->message));

          g_error_free (error);
        } 

      return FALSE;
    }

  BTK_NOTE (PRINTING,
            g_print ("PAPI Backend: Writting %i byte chunk to papi pipe\n", bytes_read));

  return TRUE;
}

static void
btk_print_backend_papi_print_stream (BtkPrintBackend        *print_backend,
				    BtkPrintJob            *job,
				    BUNNYIOChannel             *data_io,
				    BtkPrintJobCompleteFunc callback,
				    bpointer                user_data,
				    GDestroyNotify          dnotify)
{
  GError *print_error = NULL;
  BtkPrinterPapi *printer;
  _PrintStreamData *ps;
  BtkPrintSettings *settings;
  bint argc;  
  bint in_fd;
  bchar **argv = NULL; 
  const bchar *title;
  char *prtnm = NULL;
  BtkPrintDuplex val;
  papi_status_t pstatus = NULL;
  papi_attribute_t **attrs = NULL;
  papi_job_ticket_t *ticket = NULL;
  
  printer = BTK_PRINTER_PAPI (btk_print_job_get_printer (job));
  settings = btk_print_job_get_settings (job);

  /* FIXME - the title should be set as the job-name */
  title = btk_print_job_get_title (job);

  ps = g_new0 (_PrintStreamData, 1);
  ps->callback = callback;
  ps->user_data = user_data;
  ps->dnotify = dnotify;
  ps->job = g_object_ref (job);
  ps->service = NULL;
  ps->stream = NULL;

   /* This cannot be queried yet with the current API */
  papiAttributeListAddString (&attrs, PAPI_ATTR_EXCL, "document-format", "application/postscript");
  val =  btk_print_settings_get_duplex (settings) ;
  if (val == BTK_PRINT_DUPLEX_HORIZONTAL)
    papiAttributeListAddString (&attrs, PAPI_ATTR_EXCL, "Duplex", "DuplexNoTumble");
  else if (val == BTK_PRINT_DUPLEX_VERTICAL)
    papiAttributeListAddString (&attrs, PAPI_ATTR_EXCL, "Duplex", "DuplexTumble");

  if (job->num_copies > 1) 
    {
      papiAttributeListAddInteger (&attrs, PAPI_ATTR_EXCL, "copies", job->num_copies); 
    }

  prtnm = strdup (btk_printer_get_name (BTK_PRINTER(printer)));

  if (papiServiceCreate (&(ps->service), prtnm, NULL, NULL, NULL,
                         PAPI_ENCRYPT_NEVER, NULL) != PAPI_OK)
    return;

  pstatus = papiJobStreamOpen (ps->service, prtnm, attrs, ticket, &(ps->stream));
  if (pstatus != PAPI_OK)
    {
      papiServiceDestroy (ps->service);
      ps->service = NULL;
      return;
    }

  /* Everything set up fine, so get ready to wait for input data stream */
  g_io_add_watch (data_io,
                  G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP,
                  (BUNNYIOFunc) papi_write,
                  ps);
}


static void
_papi_set_default_printer (BtkPrintBackendPapi *backend)
{
  char *def_printer = NULL;
  char *_default_attr[] = { "printer-name", NULL };
  papi_service_t service = NULL;
  papi_printer_t default_printer = NULL;
  papi_attribute_t **attrs = NULL;

  if (papiServiceCreate (&service, NULL, NULL, NULL, NULL, PAPI_ENCRYPT_NEVER,
                          NULL) != PAPI_OK)
    return;

  if (papiPrinterQuery (service, "_default", _default_attr, NULL, 
                        &default_printer) == PAPI_OK)  
    {
      if (default_printer != NULL)  
        {
          attrs = papiPrinterGetAttributeList (default_printer);

          if (attrs != NULL)
            if (papiAttributeListGetString (attrs, NULL, "printer-name", 
                                            &def_printer) == PAPI_OK) 
	      {
	        backend->default_printer = strdup (def_printer);
	      }
        }
    }

  papiPrinterFree (default_printer);
  papiServiceDestroy (service);
}

static void
btk_print_backend_papi_init (BtkPrintBackendPapi *backend)
{
  _papi_set_default_printer (backend);
}

static void
btk_print_backend_papi_finalize (BObject *object)
{
  BtkPrintBackendPapi *backend_papi;

  BTK_NOTE (PRINTING,
            g_print ("PAPI Backend: finalizing PAPI backend module\n"));

  backend_papi = BTK_PRINT_BACKEND_PAPI (object);

  g_free (backend_papi->default_printer);
  backend_papi->default_printer = NULL;

  backend_parent_class->finalize (object);
}


static void
btk_print_backend_papi_dispose (BObject *object)
{
  BtkPrintBackendPapi *backend_papi;

  BTK_NOTE (PRINTING,
            g_print ("PAPI Backend: %s\n", B_STRFUNC));

  backend_papi = BTK_PRINT_BACKEND_PAPI (object);

  backend_parent_class->dispose (object);
}

char **
get_all_list(papi_service_t svc)
{
        papi_status_t status;
        papi_printer_t printer = NULL;
        char *attr[] = { "member-names", NULL };
        char **names = NULL;

        status = papiPrinterQuery(svc, "_all", attr, NULL, &printer);
        if ((status == PAPI_OK) && (printer != NULL)) {
                papi_attribute_t **attributes =
                                        papiPrinterGetAttributeList(printer);
                if (attributes != NULL) {
                        void *iter = NULL;
                        char *member = NULL;

                        for (status = papiAttributeListGetString(attributes,
                                                &iter, "member-names", &member);
                                status == PAPI_OK;
                                status = papiAttributeListGetString(attributes,
                                                &iter, NULL, &member))
                                        list_append(&names, strdup(member));
                }
                papiPrinterFree(printer);
        }

        return (names);
}

static char **
get_printers_list(papi_service_t svc)
{
	papi_status_t status;
    	papi_printer_t *printers = NULL;
    	char *keys[] = { "printer-name", "printer-uri-supported", NULL };
    	char **names = NULL;

	status = papiPrintersList(svc, keys, NULL, &printers);
	if ((status == PAPI_OK) && (printers != NULL)) {
		int i;

		for (i = 0; printers[i] != NULL; i++) {
			papi_attribute_t **attributes =
				papiPrinterGetAttributeList(printers[i]);
    			char *name = NULL;
			
			(void) papiAttributeListGetString(attributes, NULL,
							"printer-name", &name);
			if ((name != NULL) && (strcmp(name, "_default") != 0))
    				list_append(&names, strdup(name));
    		}
    		papiPrinterListFree(printers);
    	}
    
    	return (names);
}

static void
papi_request_printer_list (BtkPrintBackend *backend)
{
  BtkPrintBackendPapi *papi_backend;

  papi_backend = BTK_PRINT_BACKEND_PAPI (backend);

  /* Get the list of printers using papi API */
  papi_get_printer_list (papi_backend); 
}

static bboolean
papi_get_printer_list (BtkPrintBackendPapi *papi_backend)
{
  int i;
  const char *attributes[] =  /* Attributes we're interested in */
    {
      "printer-name",
      "printer-uri-supported",
      NULL
    };
  papi_status_t status, status2;
  papi_service_t service = NULL;
  char **printers = NULL;
  BtkPrinter *printer;
  BtkPrinterPapi *papi_printer;
  GList *current_printer_list;
  BtkPrintBackend *backend = BTK_PRINT_BACKEND (papi_backend);
  
  if ((status = papiServiceCreate (&service, NULL, NULL, NULL, NULL,
		PAPI_ENCRYPT_NEVER, NULL)) != PAPI_OK)
    return FALSE; 

  if ((printers = get_all_list (service)) == NULL) 
    {
      printers = get_printers_list (service);
    }

  if (printers == NULL) 
    {
      papiServiceDestroy (service);
      return FALSE;
    }

  for (i = 0; printers[i] != NULL; i++) 
    {
      BtkPrinter *printer;
      char *name = NULL, *url = NULL;
      papi_attribute_t **attrs = NULL;

          printer = btk_print_backend_find_printer (backend, printers[i]);

	  if (!printer) 
	    {
              /* skip null printer name just in case */
              if (printers[i] == NULL)
                continue;

	      /* skip the alias _default and _all printers */
      	      if (strcmp(printers[i], "_default")==0 || strcmp(printers[i], "_all")==0)
	        continue;	

	      papi_printer = btk_printer_papi_new (printers[i], backend);
	      printer = BTK_PRINTER (papi_printer);

	      /* Only marked default printer to not have details so that
		 the request_details method will be called  at start up
	       */

	      if (papi_backend->default_printer != NULL)
	        if (strcmp (printers[i], papi_backend->default_printer)==0)
		  {
		    btk_printer_set_is_default (printer, TRUE);
	  	  }	

              btk_printer_set_icon_name (printer, "btk-print");
	      btk_print_backend_add_printer (backend, printer);
              btk_printer_set_is_active (printer, TRUE);

  	      /* btk_printer_set_has_details (printer, TRUE); */
	    }
    	  else 
            g_object_ref (printer);

      if (!btk_printer_is_active (printer))
        {
          btk_printer_set_is_active (printer, TRUE);
          btk_printer_set_is_new (printer, TRUE);
        }

      if (btk_printer_is_new (printer))
        {
          g_signal_emit_by_name (backend, "printer-added", printer);
          btk_printer_set_is_new (printer, FALSE);
        }

      g_object_unref (printer);
    }

  free (printers);
  papiServiceDestroy (service);

  /* To set that the list of printers added is complete */
  btk_print_backend_set_list_done (backend); 

  return TRUE;
}

static void
update_printer_status (BtkPrinter *printer)
{
  BtkPrintBackend *backend;
  BtkPrinterPapi *papi_printer;
  bboolean status_changed = FALSE;

  backend = btk_printer_get_backend (printer);
  papi_printer = BTK_PRINTER_PAPI (printer);

  /* if (status_changed) */
    g_signal_emit_by_name (BTK_PRINT_BACKEND (backend),
                           "printer-status-changed", printer);

}


static BtkPrinterOptionSet *
papi_printer_get_options (BtkPrinter           *printer,
			  BtkPrintSettings     *settings,
			  BtkPageSetup         *page_setup,
			  BtkPrintCapabilities  capabilities)
{
  BtkPrinterOptionSet *set;
  BtkPrinterOption *option;
  int i;
  char *print_at[] = { "now", "on-hold" };
  char *n_up[] = {"1"};

  /* Update the printer status before the printer options are displayed */
  update_printer_status (printer); 

  set = btk_printer_option_set_new ();

  /* non-ppd related settings */

  /* This maps to number-up-supported in PAPI. FIXME 
   * number-up-default is the default value. 
   * number-up-supported is the list of number of able to print per page 
   */
  option = btk_printer_option_new ("btk-n-up", "Pages Per Sheet", BTK_PRINTER_OPTION_TYPE_PICKONE);
  btk_printer_option_choices_from_array (option, G_N_ELEMENTS (n_up),
					 n_up, n_up);
  btk_printer_option_set (option, "1");
  btk_printer_option_set_add (set, option);
  g_object_unref (option);

  /* This maps to job-priority-supported and job-priority-default in PAPI - FIXME*/
  
  /* This relates to job-sheets-supported in PAPI  FIXME*/
  
  /* This relates to job-hold-until-supported in PAPI */
  option = btk_printer_option_new ("btk-print-time", "Print at", BTK_PRINTER_OPTION_TYPE_PICKONE);
  btk_printer_option_choices_from_array (option, G_N_ELEMENTS (print_at),
					 print_at, print_at);
  btk_printer_option_set (option, "now");
  btk_printer_option_set_add (set, option);
  g_object_unref (option);
  
  return set;
}

static void
papi_printer_get_settings_from_options (BtkPrinter          *printer,
				       BtkPrinterOptionSet *options,
				       BtkPrintSettings    *settings)
{
  BtkPrinterOption *option;

  option = btk_printer_option_set_lookup (options, "btk-n-up");
  if (option)
     btk_print_settings_set (settings, BTK_PRINT_SETTINGS_NUMBER_UP, option->value);

}

static void
papi_printer_prepare_for_print (BtkPrinter       *printer,
			       BtkPrintJob      *print_job,
			       BtkPrintSettings *settings,
			       BtkPageSetup     *page_setup)
{
  BtkPageSet page_set;
  double scale;
  BtkPaperSize *papersize = NULL;
  char *ppd_paper_name;

  print_job->print_pages = btk_print_settings_get_print_pages (settings);
  print_job->page_ranges = NULL;
  print_job->num_page_ranges = 0;
  
  if (print_job->print_pages == BTK_PRINT_PAGES_RANGES)
    print_job->page_ranges =
      btk_print_settings_get_page_ranges (settings,
					  &print_job->num_page_ranges);
  
  print_job->collate = btk_print_settings_get_collate (settings);
  print_job->reverse = btk_print_settings_get_reverse (settings);
  print_job->num_copies = btk_print_settings_get_n_copies (settings);
 
  scale = btk_print_settings_get_scale (settings);
  if (scale != 100.0)
    print_job->scale = scale/100.0;

  papersize = btk_page_setup_get_paper_size (page_setup);
  ppd_paper_name = btk_paper_size_get_ppd_name (papersize);

  page_set = btk_print_settings_get_page_set (settings);
  if (page_set == BTK_PAGE_SET_EVEN)
    print_job->page_set = BTK_PAGE_SET_EVEN;
  else if (page_set == BTK_PAGE_SET_ODD)
    print_job->page_set = BTK_PAGE_SET_ODD;
  else
    print_job->page_set = BTK_PAGE_SET_ALL;

  print_job->rotate_to_orientation = TRUE;

}

bboolean
is_local_printer (bchar *printer_uri)
{
  if (strncmp (printer_uri, "lpsched:", 8) == 0)
    return TRUE;
  else
    return FALSE;
}

void
merge_ppd_data (papi_attribute_t ***attributes, bchar *ppdfile)
{
  get_ppd_attrs (attributes, ppdfile);
}


static void
papi_display_printer_status_done (bpointer user_data)
{
  BtkPrinter *printer = (BtkPrinter *) user_data;
  BtkPrinterPapi *papi_printer;

  g_signal_emit_by_name (printer, "details-acquired", TRUE); 
  papi_printer = BTK_PRINTER_PAPI (printer);
  return;
}

#define IDLE 3
#define PROCESSING 4
#define STOPPED 5
static bboolean
papi_display_printer_status (bpointer user_data)
{
  BtkPrinter *printer = (BtkPrinter *) user_data;
  BtkPrinterPapi *papi_printer;
  bchar *loc, *printer_uri, *ppdfile;
  int state;
  papi_service_t service;
  papi_attribute_t **attrs = NULL;
  papi_printer_t current_printer = NULL;
  static int count = 0;

  papi_printer = BTK_PRINTER_PAPI (printer);
  if (papiServiceCreate (&service, NULL, NULL, NULL, NULL, PAPI_ENCRYPT_NEVER,
                          NULL) != PAPI_OK)
    return FALSE;

  if (papiPrinterQuery (service, papi_printer->printer_name, NULL, NULL,
                        &current_printer) != PAPI_OK) 
    {
       /* SUN_BRANDING */
       btk_printer_set_state_message (printer, _("printer offline"));
    }

  if (current_printer != NULL)
    {
        attrs = papiPrinterGetAttributeList (current_printer);
    }

  if (papiAttributeListGetString (attrs, NULL, "printer-info", &loc) == PAPI_OK)
    {
        btk_printer_set_location (printer, loc);
    }

  if (papiAttributeListGetInteger (attrs, NULL, "printer-state", &state) == PAPI_OK)
    {
      switch (state) 
        {
	  /* SUN_BRANDING */
	  case IDLE: btk_printer_set_state_message (printer, _("ready to print"));
		     break;
	  /* SUN_BRANDING */
	  case PROCESSING: btk_printer_set_state_message (printer, _("processing job"));
		           break;

	  /* SUN_BRANDING */
	  case STOPPED: btk_printer_set_state_message (printer, _("paused"));
		        break;
	  /* SUN_BRANDING */
	  default: btk_printer_set_state_message (printer, _("unknown"));
		   break;
        }
    }

  papiPrinterFree (current_printer);
  papiServiceDestroy (service);
  btk_printer_set_has_details (printer, TRUE);

  return FALSE;
}

static void  
papi_printer_request_details (BtkPrinter *printer)
{
  g_idle_add_full (G_PRIORITY_DEFAULT_IDLE, papi_display_printer_status, printer, papi_display_printer_status_done); 
}


static BtkPrintCapabilities
papi_printer_get_capabilities (BtkPrinter *printer)
{
  return BTK_PRINT_CAPABILITY_COPIES | BTK_PRINT_CAPABILITY_PAGE_SET ; 
}

