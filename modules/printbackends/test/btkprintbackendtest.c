/* BTK - The GIMP Toolkit
 * btkprintbackendpdf.c: Test implementation of BtkPrintBackend 
 * for printing to a test
 * Copyright (C) 2007, Red Hat, Inc.
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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <bairo.h>
#include <bairo-pdf.h>
#include <bairo-ps.h>

#include <bunnylib/gi18n-lib.h>

#include <btk/btkprintbackend.h>
#include <btk/btkunixprint.h>
#include <btk/btkprinter-private.h>

#include "btkprintbackendtest.h"


typedef struct _BtkPrintBackendTestClass BtkPrintBackendTestClass;

#define BTK_PRINT_BACKEND_TEST_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_PRINT_BACKEND_TEST, BtkPrintBackendTestClass))
#define BTK_IS_PRINT_BACKEND_TEST_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_PRINT_BACKEND_TEST))
#define BTK_PRINT_BACKENDTEST_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_PRINT_BACKEND_TEST, BtkPrintBackendTestClass))

#define _STREAM_MAX_CHUNK_SIZE 8192

static GType print_backend_test_type = 0;

struct _BtkPrintBackendTestClass
{
  BtkPrintBackendClass parent_class;
};

struct _BtkPrintBackendTest
{
  BtkPrintBackend parent_instance;
};

typedef enum
{
  FORMAT_PDF,
  FORMAT_PS,
  N_FORMATS
} OutputFormat;

static const bchar* formats[N_FORMATS] =
{
  "pdf",
  "ps"
};

static BObjectClass *backend_parent_class;

static void                 btk_print_backend_test_class_init      (BtkPrintBackendTestClass *class);
static void                 btk_print_backend_test_init            (BtkPrintBackendTest      *impl);
static void                 test_printer_get_settings_from_options (BtkPrinter              *printer,
								    BtkPrinterOptionSet     *options,
								    BtkPrintSettings        *settings);
static BtkPrinterOptionSet *test_printer_get_options               (BtkPrinter              *printer,
								    BtkPrintSettings        *settings,
								    BtkPageSetup            *page_setup,
								    BtkPrintCapabilities     capabilities);
static void                 test_printer_prepare_for_print         (BtkPrinter              *printer,
								    BtkPrintJob             *print_job,
								    BtkPrintSettings        *settings,
								    BtkPageSetup            *page_setup);
static void                 btk_print_backend_test_print_stream    (BtkPrintBackend         *print_backend,
								    BtkPrintJob             *job,
								    BUNNYIOChannel              *data_io,
								    BtkPrintJobCompleteFunc  callback,
								    bpointer                 user_data,
								    GDestroyNotify           dnotify);
static bairo_surface_t *    test_printer_create_bairo_surface      (BtkPrinter              *printer,
								    BtkPrintSettings        *settings,
								    bdouble                  width,
								    bdouble                  height,
								    BUNNYIOChannel              *cache_io);

static void                 test_printer_request_details           (BtkPrinter              *printer);

static void
btk_print_backend_test_register_type (GTypeModule *module)
{
  const GTypeInfo print_backend_test_info =
  {
    sizeof (BtkPrintBackendTestClass),
    NULL,		/* base_init */
    NULL,		/* base_finalize */
    (GClassInitFunc) btk_print_backend_test_class_init,
    NULL,		/* class_finalize */
    NULL,		/* class_data */
    sizeof (BtkPrintBackendTest),
    0,		/* n_preallocs */
    (GInstanceInitFunc) btk_print_backend_test_init,
  };

  print_backend_test_type = g_type_module_register_type (module,
                                                         BTK_TYPE_PRINT_BACKEND,
                                                         "BtkPrintBackendTest",
                                                         &print_backend_test_info, 0);
}

G_MODULE_EXPORT void 
pb_module_init (GTypeModule *module)
{
  btk_print_backend_test_register_type (module);
}

G_MODULE_EXPORT void 
pb_module_exit (void)
{

}
  
G_MODULE_EXPORT BtkPrintBackend * 
pb_module_create (void)
{
  return btk_print_backend_test_new ();
}

/*
 * BtkPrintBackendTest
 */
GType
btk_print_backend_test_get_type (void)
{
  return print_backend_test_type;
}

/**
 * btk_print_backend_test_new:
 *
 * Creates a new #BtkPrintBackendTest object. #BtkPrintBackendTest
 * implements the #BtkPrintBackend interface with direct access to
 * the testsystem using Unix/Linux API calls
 *
 * Return value: the new #BtkPrintBackendTest object
 **/
BtkPrintBackend *
btk_print_backend_test_new (void)
{
  return g_object_new (BTK_TYPE_PRINT_BACKEND_TEST, NULL);
}

static void
btk_print_backend_test_class_init (BtkPrintBackendTestClass *class)
{
  BtkPrintBackendClass *backend_class = BTK_PRINT_BACKEND_CLASS (class);

  backend_parent_class = g_type_class_peek_parent (class);

  backend_class->print_stream = btk_print_backend_test_print_stream;
  backend_class->printer_create_bairo_surface = test_printer_create_bairo_surface;
  backend_class->printer_get_options = test_printer_get_options;
  backend_class->printer_get_settings_from_options = test_printer_get_settings_from_options;
  backend_class->printer_prepare_for_print = test_printer_prepare_for_print;
  backend_class->printer_request_details = test_printer_request_details;
}

/* return N_FORMATS if no explicit format in the settings */
static OutputFormat
format_from_settings (BtkPrintSettings *settings)
{
  const bchar *value;
  bint i;

  if (settings == NULL)
    return N_FORMATS;

  value = btk_print_settings_get (settings, BTK_PRINT_SETTINGS_OUTPUT_FILE_FORMAT);
  if (value == NULL)
    return N_FORMATS;

  for (i = 0; i < N_FORMATS; ++i)
    if (strcmp (value, formats[i]) == 0)
      break;

  g_assert (i < N_FORMATS);

  return (OutputFormat) i;
}

static bchar *
output_test_from_settings (BtkPrintSettings *settings,
			   const bchar      *default_format)
{
  bchar *uri = NULL;
  
  if (settings)
    uri = g_strdup (btk_print_settings_get (settings, BTK_PRINT_SETTINGS_OUTPUT_URI));

  if (uri == NULL)
    { 
      const bchar *extension;
      bchar *name, *locale_name, *path;

      if (default_format)
        extension = default_format;
      else
        {
          OutputFormat format;

          format = format_from_settings (settings);
          extension = format == FORMAT_PS ? "ps" : "pdf";
        }
 
      /* default filename used for print-to-test */ 
      name = g_strdup_printf (_("test-output.%s"), extension);
      locale_name = g_filename_from_utf8 (name, -1, NULL, NULL, NULL);
      g_free (name);

      if (locale_name != NULL)
        {
	  bchar *current_dir = g_get_current_dir ();
          path = g_build_filename (current_dir, locale_name, NULL);
          g_free (locale_name);

          uri = g_filename_to_uri (path, NULL, NULL);
          g_free (path);
	  g_free (current_dir);
	}
    }

  return uri;
}

static bairo_status_t
_bairo_write (void                *closure,
              const unsigned char *data,
              unsigned int         length)
{
  BUNNYIOChannel *io = (BUNNYIOChannel *)closure;
  bsize written;
  GError *error;

  error = NULL;

  BTK_NOTE (PRINTING,
            g_print ("TEST Backend: Writing %i byte chunk to temp test\n", length));

  while (length > 0) 
    {
      g_io_channel_write_chars (io, (const bchar *) data, length, &written, &error);

      if (error != NULL)
	{
	  BTK_NOTE (PRINTING,
                     g_print ("TEST Backend: Error writing to temp test, %s\n", error->message));

          g_error_free (error);
	  return BAIRO_STATUS_WRITE_ERROR;
	}    

      BTK_NOTE (PRINTING,
                g_print ("TEST Backend: Wrote %i bytes to temp test\n", (int)written));
      
      data += written;
      length -= written;
    }

  return BAIRO_STATUS_SUCCESS;
}


static bairo_surface_t *
test_printer_create_bairo_surface (BtkPrinter       *printer,
				   BtkPrintSettings *settings,
				   bdouble           width, 
				   bdouble           height,
				   BUNNYIOChannel       *cache_io)
{
  bairo_surface_t *surface;
  OutputFormat format;

  format = format_from_settings (settings);

  if (format == FORMAT_PS)
    surface = bairo_ps_surface_create_for_stream (_bairo_write, cache_io, width, height);
  else
    surface = bairo_pdf_surface_create_for_stream (_bairo_write, cache_io, width, height);

  bairo_surface_set_fallback_resolution (surface,
                                         2.0 * btk_print_settings_get_printer_lpi (settings),
                                         2.0 * btk_print_settings_get_printer_lpi (settings));

  return surface;
}

typedef struct {
  BtkPrintBackend *backend;
  BtkPrintJobCompleteFunc callback;
  BtkPrintJob *job;
  BUNNYIOChannel *target_io;
  bpointer user_data;
  GDestroyNotify dnotify;
} _PrintStreamData;

static void
test_print_cb (BtkPrintBackendTest *print_backend,
               GError              *error,
               bpointer            user_data)
{
  _PrintStreamData *ps = (_PrintStreamData *) user_data;

  if (ps->target_io != NULL)
    g_io_channel_unref (ps->target_io);

  if (ps->callback)
    ps->callback (ps->job, ps->user_data, error);

  if (ps->dnotify)
    ps->dnotify (ps->user_data);

  btk_print_job_set_status (ps->job,
			    (error != NULL)?BTK_PRINT_STATUS_FINISHED_ABORTED:BTK_PRINT_STATUS_FINISHED);

  if (ps->job)
    g_object_unref (ps->job);
 
  g_free (ps);
}

static bboolean
test_write (BUNNYIOChannel   *source,
            BUNNYIOCondition  con,
            bpointer      user_data)
{
  bchar buf[_STREAM_MAX_CHUNK_SIZE];
  bsize bytes_read;
  GError *error;
  BUNNYIOStatus read_status;
  _PrintStreamData *ps = (_PrintStreamData *) user_data;

  error = NULL;

  read_status = 
    g_io_channel_read_chars (source,
                             buf,
                             _STREAM_MAX_CHUNK_SIZE,
                             &bytes_read,
                             &error);

  if (read_status != G_IO_STATUS_ERROR)
    {
      bsize bytes_written;

      g_io_channel_write_chars (ps->target_io, 
                                buf, 
				bytes_read, 
				&bytes_written, 
				&error);
    }

  if (error != NULL || read_status == G_IO_STATUS_EOF)
    {
      test_print_cb (BTK_PRINT_BACKEND_TEST (ps->backend), error, user_data);

      if (error != NULL)
        {
          BTK_NOTE (PRINTING,
                    g_print ("TEST Backend: %s\n", error->message));

          g_error_free (error);
        }

      return FALSE;
    }

  BTK_NOTE (PRINTING,
            g_print ("TEST Backend: Writing %i byte chunk to target test\n", (int)bytes_read));

  return TRUE;
}

static void
btk_print_backend_test_print_stream (BtkPrintBackend        *print_backend,
				     BtkPrintJob            *job,
				     BUNNYIOChannel             *data_io,
				     BtkPrintJobCompleteFunc callback,
				     bpointer                user_data,
				     GDestroyNotify          dnotify)
{
  GError *internal_error = NULL;
  BtkPrinter *printer;
  _PrintStreamData *ps;
  BtkPrintSettings *settings;
  bchar *uri, *testname;

  printer = btk_print_job_get_printer (job);
  settings = btk_print_job_get_settings (job);

  ps = g_new0 (_PrintStreamData, 1);
  ps->callback = callback;
  ps->user_data = user_data;
  ps->dnotify = dnotify;
  ps->job = g_object_ref (job);
  ps->backend = print_backend;

  internal_error = NULL;
  uri = output_test_from_settings (settings, NULL);
  testname = g_filename_from_uri (uri, NULL, &internal_error);
  g_free (uri);

  if (testname == NULL)
    goto error;

  ps->target_io = g_io_channel_new_file (testname, "w", &internal_error);

  g_free (testname);

  if (internal_error == NULL)
    g_io_channel_set_encoding (ps->target_io, NULL, &internal_error);

error:
  if (internal_error != NULL)
    {
      test_print_cb (BTK_PRINT_BACKEND_TEST (print_backend),
                    internal_error, ps);

      g_error_free (internal_error);
      return;
    }

  g_io_add_watch (data_io, 
                  G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP,
                  (BUNNYIOFunc) test_write,
                  ps);
}

static void
btk_print_backend_test_init (BtkPrintBackendTest *backend)
{
  BtkPrinter *printer;
  int i;

  /* make 100 of these printers */
  for (i = 0; i < 100; i++)
    {
      char *name;
 
      name = g_strdup_printf ("%s %i", _("Print to Test Printer"), i);
      printer = g_object_new (BTK_TYPE_PRINTER,
			      "name", name,
			      "backend", backend,
			      "is-virtual", FALSE, /* treat printer like a real one*/
			      NULL); 
      g_free (name);

      g_message ("TEST Backend: Adding printer %d\n", i);

      btk_printer_set_has_details (printer, FALSE);
      btk_printer_set_icon_name (printer, "btk-delete"); /* use a delete icon just for fun */
      btk_printer_set_is_active (printer, TRUE);

      btk_print_backend_add_printer (BTK_PRINT_BACKEND (backend), printer);
      g_object_unref (printer);
    }

  btk_print_backend_set_list_done (BTK_PRINT_BACKEND (backend));
}

static BtkPrinterOptionSet *
test_printer_get_options (BtkPrinter           *printer,
			  BtkPrintSettings     *settings,
			  BtkPageSetup         *page_setup,
			  BtkPrintCapabilities  capabilities)
{
  BtkPrinterOptionSet *set;
  BtkPrinterOption *option;
  const bchar *n_up[] = { "1" };
  OutputFormat format;

  format = format_from_settings (settings);

  set = btk_printer_option_set_new ();

  option = btk_printer_option_new ("btk-n-up", _("Pages per _sheet:"), BTK_PRINTER_OPTION_TYPE_PICKONE);
  btk_printer_option_choices_from_array (option, G_N_ELEMENTS (n_up),
					 (char **) n_up, (char **) n_up /* FIXME i18n (localised digits)! */);
  btk_printer_option_set (option, "1");
  btk_printer_option_set_add (set, option);
  g_object_unref (option);

  return set;
}

static void
test_printer_get_settings_from_options (BtkPrinter          *printer,
					BtkPrinterOptionSet *options,
					BtkPrintSettings    *settings)
{
}

static void
test_printer_prepare_for_print (BtkPrinter       *printer,
				BtkPrintJob      *print_job,
				BtkPrintSettings *settings,
				BtkPageSetup     *page_setup)
{
  bdouble scale;

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

  print_job->page_set = btk_print_settings_get_page_set (settings);
  print_job->rotate_to_orientation = TRUE;
}

static bboolean
test_printer_details_aquired_cb (BtkPrinter *printer)
{
  bboolean success;
  bint weight;

  /* weight towards success */
  weight = g_random_int_range (0, 100);

  success = FALSE;
  if (weight < 75)
    success = TRUE;

  g_message ("success %i", success);
  btk_printer_set_has_details (printer, success);
  g_signal_emit_by_name (printer, "details-acquired", success);

  return FALSE;
}

static void
test_printer_request_details (BtkPrinter *printer)
{
  bint weight;
  bint time;
  /* set the timer to succeed or fail at a random time interval */
  /* weight towards the shorter end */
  weight = g_random_int_range (0, 100);
  if (weight < 50)
    time = g_random_int_range (0, 2);
  else if (weight < 75)
    time = g_random_int_range (1, 5);
  else
    time = g_random_int_range (1, 10);

  g_message ("Gathering details in %i seconds", time);

  if (time == 0)
    time = 10;
  else
    time *= 1000;

  g_timeout_add (time, (GSourceFunc) test_printer_details_aquired_cb, printer);
}


