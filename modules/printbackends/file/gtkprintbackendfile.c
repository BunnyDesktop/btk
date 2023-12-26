/* BTK - The GIMP Toolkit
 * btkprintbackendfile.c: Default implementation of BtkPrintBackend 
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
#include <bairo-svg.h>

#include <bunnylib/gi18n-lib.h>

#include "btk/btk.h"
#include "btk/btkprinter-private.h"

#include "btkprintbackendfile.h"

typedef struct _BtkPrintBackendFileClass BtkPrintBackendFileClass;

#define BTK_PRINT_BACKEND_FILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_PRINT_BACKEND_FILE, BtkPrintBackendFileClass))
#define BTK_IS_PRINT_BACKEND_FILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_PRINT_BACKEND_FILE))
#define BTK_PRINT_BACKEND_FILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_PRINT_BACKEND_FILE, BtkPrintBackendFileClass))

#define _STREAM_MAX_CHUNK_SIZE 8192

static GType print_backend_file_type = 0;

struct _BtkPrintBackendFileClass
{
  BtkPrintBackendClass parent_class;
};

struct _BtkPrintBackendFile
{
  BtkPrintBackend parent_instance;
};

typedef enum
{
  FORMAT_PDF,
  FORMAT_PS,
  FORMAT_SVG,
  N_FORMATS
} OutputFormat;

static const gchar* formats[N_FORMATS] =
{
  "pdf",
  "ps",
  "svg"
};

static GObjectClass *backend_parent_class;

static void                 btk_print_backend_file_class_init      (BtkPrintBackendFileClass *class);
static void                 btk_print_backend_file_init            (BtkPrintBackendFile      *impl);
static void                 file_printer_get_settings_from_options (BtkPrinter              *printer,
								    BtkPrinterOptionSet     *options,
								    BtkPrintSettings        *settings);
static BtkPrinterOptionSet *file_printer_get_options               (BtkPrinter              *printer,
								    BtkPrintSettings        *settings,
								    BtkPageSetup            *page_setup,
								    BtkPrintCapabilities     capabilities);
static void                 file_printer_prepare_for_print         (BtkPrinter              *printer,
								    BtkPrintJob             *print_job,
								    BtkPrintSettings        *settings,
								    BtkPageSetup            *page_setup);
static void                 btk_print_backend_file_print_stream    (BtkPrintBackend         *print_backend,
								    BtkPrintJob             *job,
								    BUNNYIOChannel              *data_io,
								    BtkPrintJobCompleteFunc  callback,
								    gpointer                 user_data,
								    GDestroyNotify           dnotify);
static bairo_surface_t *    file_printer_create_bairo_surface      (BtkPrinter              *printer,
								    BtkPrintSettings        *settings,
								    gdouble                  width,
								    gdouble                  height,
								    BUNNYIOChannel              *cache_io);

static GList *              file_printer_list_papers               (BtkPrinter              *printer);
static BtkPageSetup *       file_printer_get_default_page_size     (BtkPrinter              *printer);

static void
btk_print_backend_file_register_type (GTypeModule *module)
{
  const GTypeInfo print_backend_file_info =
  {
    sizeof (BtkPrintBackendFileClass),
    NULL,		/* base_init */
    NULL,		/* base_finalize */
    (GClassInitFunc) btk_print_backend_file_class_init,
    NULL,		/* class_finalize */
    NULL,		/* class_data */
    sizeof (BtkPrintBackendFile),
    0,		/* n_preallocs */
    (GInstanceInitFunc) btk_print_backend_file_init,
  };

  print_backend_file_type = g_type_module_register_type (module,
                                                         BTK_TYPE_PRINT_BACKEND,
                                                         "BtkPrintBackendFile",
                                                         &print_backend_file_info, 0);
}

G_MODULE_EXPORT void 
pb_module_init (GTypeModule *module)
{
  btk_print_backend_file_register_type (module);
}

G_MODULE_EXPORT void 
pb_module_exit (void)
{

}
  
G_MODULE_EXPORT BtkPrintBackend * 
pb_module_create (void)
{
  return btk_print_backend_file_new ();
}

/*
 * BtkPrintBackendFile
 */
GType
btk_print_backend_file_get_type (void)
{
  return print_backend_file_type;
}

/**
 * btk_print_backend_file_new:
 *
 * Creates a new #BtkPrintBackendFile object. #BtkPrintBackendFile
 * implements the #BtkPrintBackend interface with direct access to
 * the filesystem using Unix/Linux API calls
 *
 * Return value: the new #BtkPrintBackendFile object
 **/
BtkPrintBackend *
btk_print_backend_file_new (void)
{
  return g_object_new (BTK_TYPE_PRINT_BACKEND_FILE, NULL);
}

static void
btk_print_backend_file_class_init (BtkPrintBackendFileClass *class)
{
  BtkPrintBackendClass *backend_class = BTK_PRINT_BACKEND_CLASS (class);

  backend_parent_class = g_type_class_peek_parent (class);

  backend_class->print_stream = btk_print_backend_file_print_stream;
  backend_class->printer_create_bairo_surface = file_printer_create_bairo_surface;
  backend_class->printer_get_options = file_printer_get_options;
  backend_class->printer_get_settings_from_options = file_printer_get_settings_from_options;
  backend_class->printer_prepare_for_print = file_printer_prepare_for_print;
  backend_class->printer_list_papers = file_printer_list_papers;
  backend_class->printer_get_default_page_size = file_printer_get_default_page_size;
}

/* return N_FORMATS if no explicit format in the settings */
static OutputFormat
format_from_settings (BtkPrintSettings *settings)
{
  const gchar *value;
  gint i;

  if (settings == NULL)
    return N_FORMATS;

  value = btk_print_settings_get (settings,
                                  BTK_PRINT_SETTINGS_OUTPUT_FILE_FORMAT);
  if (value == NULL)
    return N_FORMATS;

  for (i = 0; i < N_FORMATS; ++i)
    if (strcmp (value, formats[i]) == 0)
      break;

  g_assert (i < N_FORMATS);

  return (OutputFormat) i;
}

static gchar *
output_file_from_settings (BtkPrintSettings *settings,
			   const gchar      *default_format)
{
  gchar *uri = NULL;
  
  if (settings)
    uri = g_strdup (btk_print_settings_get (settings, BTK_PRINT_SETTINGS_OUTPUT_URI));

  if (uri == NULL)
    { 
      const gchar *extension;
      gchar *name, *locale_name, *path;

      if (default_format)
        extension = default_format;
      else
        {
          OutputFormat format;

          format = format_from_settings (settings);
          switch (format)
            {
              default:
              case FORMAT_PDF:
                extension = "pdf";
                break;
              case FORMAT_PS:
                extension = "ps";
                break;
              case FORMAT_SVG:
                extension = "svg";
                break;
            }
        }
 
      /* default filename used for print-to-file */ 
      name = g_strdup_printf (_("output.%s"), extension);
      locale_name = g_filename_from_utf8 (name, -1, NULL, NULL, NULL);
      g_free (name);

      if (locale_name != NULL)
        {
	  gchar *current_dir = g_get_current_dir ();
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
  gsize written;
  GError *error;

  error = NULL;

  BTK_NOTE (PRINTING,
            g_print ("FILE Backend: Writting %i byte chunk to temp file\n", length));

  while (length > 0) 
    {
      g_io_channel_write_chars (io, (const gchar *) data, length, &written, &error);

      if (error != NULL)
	{
	  BTK_NOTE (PRINTING,
                     g_print ("FILE Backend: Error writting to temp file, %s\n", error->message));

          g_error_free (error);
	  return BAIRO_STATUS_WRITE_ERROR;
	}    

      BTK_NOTE (PRINTING,
                g_print ("FILE Backend: Wrote %i bytes to temp file\n", written));
      
      data += written;
      length -= written;
    }

  return BAIRO_STATUS_SUCCESS;
}


static bairo_surface_t *
file_printer_create_bairo_surface (BtkPrinter       *printer,
				   BtkPrintSettings *settings,
				   gdouble           width, 
				   gdouble           height,
				   BUNNYIOChannel       *cache_io)
{
  bairo_surface_t *surface;
  OutputFormat format;
  const bairo_svg_version_t *versions;
  int num_versions = 0;

  format = format_from_settings (settings);

  switch (format)
    {
      default:
      case FORMAT_PDF:
        surface = bairo_pdf_surface_create_for_stream (_bairo_write, cache_io, width, height);
        break;
      case FORMAT_PS:
        surface = bairo_ps_surface_create_for_stream (_bairo_write, cache_io, width, height);
        break;
      case FORMAT_SVG:
        surface = bairo_svg_surface_create_for_stream (_bairo_write, cache_io, width, height);
        bairo_svg_get_versions (&versions, &num_versions);
        if (num_versions > 0)
          bairo_svg_surface_restrict_to_version (surface, versions[num_versions - 1]);
        break;
    }

  bairo_surface_set_fallback_resolution (surface,
                                         2.0 * btk_print_settings_get_printer_lpi (settings),
                                         2.0 * btk_print_settings_get_printer_lpi (settings));

  return surface;
}

typedef struct {
  BtkPrintBackend *backend;
  BtkPrintJobCompleteFunc callback;
  BtkPrintJob *job;
  GFileOutputStream *target_io_stream;
  gpointer user_data;
  GDestroyNotify dnotify;
} _PrintStreamData;

static void
file_print_cb (BtkPrintBackendFile *print_backend,
               GError              *error,
               gpointer            user_data)
{
  _PrintStreamData *ps = (_PrintStreamData *) user_data;

  BDK_THREADS_ENTER ();

  if (ps->target_io_stream != NULL)
    g_output_stream_close (G_OUTPUT_STREAM (ps->target_io_stream), NULL, NULL);

  if (ps->callback)
    ps->callback (ps->job, ps->user_data, error);

  if (ps->dnotify)
    ps->dnotify (ps->user_data);

  btk_print_job_set_status (ps->job,
			    (error != NULL)?BTK_PRINT_STATUS_FINISHED_ABORTED:BTK_PRINT_STATUS_FINISHED);

  if (ps->job)
    g_object_unref (ps->job);
 
  g_free (ps);

  BDK_THREADS_LEAVE ();
}

static gboolean
file_write (BUNNYIOChannel   *source,
            BUNNYIOCondition  con,
            gpointer      user_data)
{
  gchar buf[_STREAM_MAX_CHUNK_SIZE];
  gsize bytes_read;
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
      gsize bytes_written;

      g_output_stream_write_all (G_OUTPUT_STREAM (ps->target_io_stream),
                                 buf,
                                 bytes_read,
                                 &bytes_written,
                                 NULL,
                                 &error);
    }

  if (error != NULL || read_status == G_IO_STATUS_EOF)
    {
      file_print_cb (BTK_PRINT_BACKEND_FILE (ps->backend), error, user_data);

      if (error != NULL)
        {
          BTK_NOTE (PRINTING,
                    g_print ("FILE Backend: %s\n", error->message));

          g_error_free (error);
        }

      return FALSE;
    }

  BTK_NOTE (PRINTING,
            g_print ("FILE Backend: Writting %i byte chunk to target file\n", bytes_read));

  return TRUE;
}

static void
btk_print_backend_file_print_stream (BtkPrintBackend        *print_backend,
				     BtkPrintJob            *job,
				     BUNNYIOChannel             *data_io,
				     BtkPrintJobCompleteFunc callback,
				     gpointer                user_data,
				     GDestroyNotify          dnotify)
{
  GError *internal_error = NULL;
  _PrintStreamData *ps;
  BtkPrintSettings *settings;
  gchar *uri;
  GFile *file = NULL;

  settings = btk_print_job_get_settings (job);

  ps = g_new0 (_PrintStreamData, 1);
  ps->callback = callback;
  ps->user_data = user_data;
  ps->dnotify = dnotify;
  ps->job = g_object_ref (job);
  ps->backend = print_backend;

  internal_error = NULL;
  uri = output_file_from_settings (settings, NULL);

  if (uri == NULL)
    goto error;

  file = g_file_new_for_uri (uri);
  ps->target_io_stream = g_file_replace (file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &internal_error);

  g_object_unref (file);
  g_free (uri);

error:
  if (internal_error != NULL)
    {
      file_print_cb (BTK_PRINT_BACKEND_FILE (print_backend),
                    internal_error, ps);

      g_error_free (internal_error);
      return;
    }

  g_io_add_watch (data_io, 
                  G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP,
                  (BUNNYIOFunc) file_write,
                  ps);
}

static void
btk_print_backend_file_init (BtkPrintBackendFile *backend)
{
  BtkPrinter *printer;
  
  printer = g_object_new (BTK_TYPE_PRINTER,
			  "name", _("Print to File"),
			  "backend", backend,
			  "is-virtual", TRUE,
			  "accepts-pdf", TRUE,
			  NULL); 

  btk_printer_set_has_details (printer, TRUE);
  btk_printer_set_icon_name (printer, "btk-save");
  btk_printer_set_is_active (printer, TRUE);

  btk_print_backend_add_printer (BTK_PRINT_BACKEND (backend), printer);
  g_object_unref (printer);

  btk_print_backend_set_list_done (BTK_PRINT_BACKEND (backend));
}

typedef struct {
  BtkPrinter          *printer;
  BtkPrinterOptionSet *set;
} _OutputFormatChangedData;

static void
set_printer_format_from_option_set (BtkPrinter          *printer,
				    BtkPrinterOptionSet *set)
{
  BtkPrinterOption *format_option;
  const gchar *value;
  gint i;

  format_option = btk_printer_option_set_lookup (set, "output-file-format");
  if (format_option && format_option->value)
    {
      value = format_option->value;
      if (value)
        {
	  for (i = 0; i < N_FORMATS; ++i)
	    if (strcmp (value, formats[i]) == 0)
	      break;

	  g_assert (i < N_FORMATS);

	  switch (i)
	    {
	      case FORMAT_PDF:
		btk_printer_set_accepts_pdf (printer, TRUE);
		btk_printer_set_accepts_ps (printer, FALSE);
		break;
	      case FORMAT_PS:
		btk_printer_set_accepts_pdf (printer, FALSE);
		btk_printer_set_accepts_ps (printer, TRUE);
		break;
	      case FORMAT_SVG:
	      default:
		btk_printer_set_accepts_pdf (printer, FALSE);
		btk_printer_set_accepts_ps (printer, FALSE);
		break;
	    }
	}
    }
}

static void
file_printer_output_file_format_changed (BtkPrinterOption    *format_option,
					 gpointer             user_data)
{
  BtkPrinterOption *uri_option;
  gchar            *base = NULL;
  _OutputFormatChangedData *data = (_OutputFormatChangedData *) user_data;

  if (! format_option->value)
    return;

  uri_option = btk_printer_option_set_lookup (data->set,
                                              "btk-main-page-custom-input");

  if (uri_option && uri_option->value)
    {
      const gchar *uri = uri_option->value;
      const gchar *dot = strrchr (uri, '.');

      if (dot)
        {
          gint i;

          /*  check if the file extension matches one of the known ones  */
          for (i = 0; i < N_FORMATS; i++)
            if (strcmp (dot + 1, formats[i]) == 0)
              break;

          if (i < N_FORMATS && strcmp (formats[i], format_option->value))
            {
              /*  the file extension is known but doesn't match the
               *  selected one, strip it away
               */
              base = g_strndup (uri, dot - uri);
            }
        }
      else
        {
          /*  there's no file extension  */
          base = g_strdup (uri);
        }
    }

  if (base)
    {
      gchar *tmp = g_strdup_printf ("%s.%s", base, format_option->value);

      btk_printer_option_set (uri_option, tmp);
      g_free (tmp);
      g_free (base);
    }

  set_printer_format_from_option_set (data->printer, data->set);
}

static BtkPrinterOptionSet *
file_printer_get_options (BtkPrinter           *printer,
			  BtkPrintSettings     *settings,
			  BtkPageSetup         *page_setup,
			  BtkPrintCapabilities  capabilities)
{
  BtkPrinterOptionSet *set;
  BtkPrinterOption *option;
  const gchar *n_up[] = {"1", "2", "4", "6", "9", "16" };
  const gchar *pages_per_sheet = NULL;
  const gchar *format_names[N_FORMATS] = { N_("PDF"), N_("Postscript"), N_("SVG") };
  const gchar *supported_formats[N_FORMATS];
  gchar *display_format_names[N_FORMATS];
  gint n_formats = 0;
  OutputFormat format;
  gchar *uri;
  gint current_format = 0;
  _OutputFormatChangedData *format_changed_data;

  format = format_from_settings (settings);

  set = btk_printer_option_set_new ();

  option = btk_printer_option_new ("btk-n-up", _("Pages per _sheet:"), BTK_PRINTER_OPTION_TYPE_PICKONE);
  btk_printer_option_choices_from_array (option, G_N_ELEMENTS (n_up),
					 (char **) n_up, (char **) n_up /* FIXME i18n (localised digits)! */);
  if (settings)
    pages_per_sheet = btk_print_settings_get (settings, BTK_PRINT_SETTINGS_NUMBER_UP);
  if (pages_per_sheet)
    btk_printer_option_set (option, pages_per_sheet);
  else
    btk_printer_option_set (option, "1");
  btk_printer_option_set_add (set, option);
  g_object_unref (option);

  if (capabilities & (BTK_PRINT_CAPABILITY_GENERATE_PDF | BTK_PRINT_CAPABILITY_GENERATE_PS))
    {
      if (capabilities & BTK_PRINT_CAPABILITY_GENERATE_PDF)
        {
	  if (format == FORMAT_PDF || format == N_FORMATS)
            {
              format = FORMAT_PDF;
	      current_format = n_formats;
            }
          supported_formats[n_formats] = formats[FORMAT_PDF];
	  display_format_names[n_formats] = _(format_names[FORMAT_PDF]);
	  n_formats++;
	}
      if (capabilities & BTK_PRINT_CAPABILITY_GENERATE_PS)
        {
	  if (format == FORMAT_PS || format == N_FORMATS)
	    current_format = n_formats;
          supported_formats[n_formats] = formats[FORMAT_PS];
          display_format_names[n_formats] = _(format_names[FORMAT_PS]);
	  n_formats++;
	}
    }
  else
    {
      switch (format)
        {
          default:
          case FORMAT_PDF:
            current_format = FORMAT_PDF;
            break;
          case FORMAT_PS:
            current_format = FORMAT_PS;
            break;
          case FORMAT_SVG:
            current_format = FORMAT_SVG;            
            break;
        }

      for (n_formats = 0; n_formats < N_FORMATS; ++n_formats)
        {
	  supported_formats[n_formats] = formats[n_formats];
          display_format_names[n_formats] = _(format_names[n_formats]);
	}
    }

  uri = output_file_from_settings (settings, supported_formats[current_format]);

  option = btk_printer_option_new ("btk-main-page-custom-input", _("File"), 
				   BTK_PRINTER_OPTION_TYPE_FILESAVE);
  btk_printer_option_set_activates_default (option, TRUE);
  btk_printer_option_set (option, uri);
  g_free (uri);
  option->group = g_strdup ("BtkPrintDialogExtension");
  btk_printer_option_set_add (set, option);

  if (n_formats > 1)
    {
      option = btk_printer_option_new ("output-file-format", _("_Output format"), 
				       BTK_PRINTER_OPTION_TYPE_ALTERNATIVE);
      option->group = g_strdup ("BtkPrintDialogExtension");

      btk_printer_option_choices_from_array (option, n_formats,
					     (char **) supported_formats,
					     display_format_names);
      btk_printer_option_set (option, supported_formats[current_format]);
      btk_printer_option_set_add (set, option);

      set_printer_format_from_option_set (printer, set);
      format_changed_data = g_new (_OutputFormatChangedData, 1);
      format_changed_data->printer = printer;
      format_changed_data->set = set;
      g_signal_connect_data (option, "changed",
			     G_CALLBACK (file_printer_output_file_format_changed),
			     format_changed_data, (GClosureNotify)g_free, 0);

      g_object_unref (option);
    }

  return set;
}

static void
file_printer_get_settings_from_options (BtkPrinter          *printer,
					BtkPrinterOptionSet *options,
					BtkPrintSettings    *settings)
{
  BtkPrinterOption *option;

  option = btk_printer_option_set_lookup (options, "btk-main-page-custom-input");
  btk_print_settings_set (settings, BTK_PRINT_SETTINGS_OUTPUT_URI, option->value);

  option = btk_printer_option_set_lookup (options, "output-file-format");
  if (option)
    btk_print_settings_set (settings, BTK_PRINT_SETTINGS_OUTPUT_FILE_FORMAT, option->value);

  option = btk_printer_option_set_lookup (options, "btk-n-up");
  if (option)
    btk_print_settings_set (settings, BTK_PRINT_SETTINGS_NUMBER_UP, option->value);

  option = btk_printer_option_set_lookup (options, "btk-n-up-layout");
  if (option)
    btk_print_settings_set (settings, BTK_PRINT_SETTINGS_NUMBER_UP_LAYOUT, option->value);
}

static void
file_printer_prepare_for_print (BtkPrinter       *printer,
				BtkPrintJob      *print_job,
				BtkPrintSettings *settings,
				BtkPageSetup     *page_setup)
{
  gdouble scale;

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
  print_job->number_up = btk_print_settings_get_number_up (settings);
  print_job->number_up_layout = btk_print_settings_get_number_up_layout (settings);

  scale = btk_print_settings_get_scale (settings);
  if (scale != 100.0)
    print_job->scale = scale/100.0;

  print_job->page_set = btk_print_settings_get_page_set (settings);
  print_job->rotate_to_orientation = TRUE;
}

static GList *
file_printer_list_papers (BtkPrinter *printer)
{
  GList *result = NULL;
  GList *papers, *p;
  BtkPageSetup *page_setup;

  papers = btk_paper_size_get_paper_sizes (TRUE);

  for (p = papers; p; p = p->next)
    {
      BtkPaperSize *paper_size = p->data;

      page_setup = btk_page_setup_new ();
      btk_page_setup_set_paper_size (page_setup, paper_size);
      btk_paper_size_free (paper_size);
      result = g_list_prepend (result, page_setup);
    }

  g_list_free (papers);

  return g_list_reverse (result);
}

static BtkPageSetup *
file_printer_get_default_page_size (BtkPrinter *printer)
{
  BtkPageSetup *result = NULL;

  return result;
}
