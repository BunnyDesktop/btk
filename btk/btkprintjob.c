/* BtkPrintJob
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#include <bunnylib/gstdio.h>
#include "btkintl.h"
#include "btkprivate.h"

#include "btkprintjob.h"
#include "btkprinter.h"
#include "btkprinter-private.h"
#include "btkprintbackend.h"
#include "btkalias.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

struct _BtkPrintJobPrivate
{
  gchar *title;

  BUNNYIOChannel *spool_io;
  bairo_surface_t *surface;

  BtkPrintStatus status;
  BtkPrintBackend *backend;  
  BtkPrinter *printer;
  BtkPrintSettings *settings;
  BtkPageSetup *page_setup;

  guint printer_set : 1;
  guint page_setup_set : 1;
  guint settings_set  : 1;
  guint track_print_status : 1;
};


#define BTK_PRINT_JOB_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), BTK_TYPE_PRINT_JOB, BtkPrintJobPrivate))

static void     btk_print_job_finalize     (GObject               *object);
static void     btk_print_job_set_property (GObject               *object,
					    guint                  prop_id,
					    const GValue          *value,
					    GParamSpec            *pspec);
static void     btk_print_job_get_property (GObject               *object,
					    guint                  prop_id,
					    GValue                *value,
					    GParamSpec            *pspec);
static GObject* btk_print_job_constructor  (GType                  type,
					    guint                  n_construct_properties,
					    GObjectConstructParam *construct_params);

enum {
  STATUS_CHANGED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_TITLE,
  PROP_PRINTER,
  PROP_PAGE_SETUP,
  PROP_SETTINGS,
  PROP_TRACK_PRINT_STATUS
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BtkPrintJob, btk_print_job, G_TYPE_OBJECT)

static void
btk_print_job_class_init (BtkPrintJobClass *class)
{
  GObjectClass *object_class;
  object_class = (GObjectClass *) class;

  object_class->finalize = btk_print_job_finalize;
  object_class->constructor = btk_print_job_constructor;
  object_class->set_property = btk_print_job_set_property;
  object_class->get_property = btk_print_job_get_property;

  g_type_class_add_private (class, sizeof (BtkPrintJobPrivate));

  g_object_class_install_property (object_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title",
						        P_("Title"),
						        P_("Title of the print job"),
						        NULL,
							BTK_PARAM_READWRITE |
						        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_PRINTER,
                                   g_param_spec_object ("printer",
						        P_("Printer"),
						        P_("Printer to print the job to"),
						        BTK_TYPE_PRINTER,
							BTK_PARAM_READWRITE |
						        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_SETTINGS,
                                   g_param_spec_object ("settings",
						        P_("Settings"),
						        P_("Printer settings"),
						        BTK_TYPE_PRINT_SETTINGS,
							BTK_PARAM_READWRITE |
						        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_PAGE_SETUP,
                                   g_param_spec_object ("page-setup",
						        P_("Page Setup"),
						        P_("Page Setup"),
						        BTK_TYPE_PAGE_SETUP,
							BTK_PARAM_READWRITE |
						        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
				   PROP_TRACK_PRINT_STATUS,
				   g_param_spec_boolean ("track-print-status",
							 P_("Track Print Status"),
							 P_("TRUE if the print job will continue to emit "
							    "status-changed signals after the print data "
							    "has been sent to the printer or print server."),
							 FALSE,
							 BTK_PARAM_READWRITE));
  

  /**
   * BtkPrintJob::status-changed:
   * @job: the #BtkPrintJob object on which the signal was emitted
   *
   * Gets emitted when the status of a job changes. The signal handler
   * can use btk_print_job_get_status() to obtain the new status.
   *
   * Since: 2.10
   */
  signals[STATUS_CHANGED] =
    g_signal_new (I_("status-changed"),
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkPrintJobClass, status_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
}

static void
btk_print_job_init (BtkPrintJob *job)
{
  BtkPrintJobPrivate *priv;

  priv = job->priv = BTK_PRINT_JOB_GET_PRIVATE (job); 

  priv->spool_io = NULL;

  priv->title = g_strdup ("");
  priv->surface = NULL;
  priv->backend = NULL;
  priv->printer = NULL;

  priv->printer_set = FALSE;
  priv->settings_set = FALSE;
  priv->page_setup_set = FALSE;
  priv->status = BTK_PRINT_STATUS_INITIAL;
  priv->track_print_status = FALSE;
  
  job->print_pages = BTK_PRINT_PAGES_ALL;
  job->page_ranges = NULL;
  job->num_page_ranges = 0;
  job->collate = FALSE;
  job->reverse = FALSE;
  job->num_copies = 1;
  job->scale = 1.0;
  job->page_set = BTK_PAGE_SET_ALL;
  job->rotate_to_orientation = FALSE;
  job->number_up = 1;
  job->number_up_layout = BTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM;
}


static GObject*
btk_print_job_constructor (GType                  type,
			   guint                  n_construct_properties,
			   GObjectConstructParam *construct_params)
{
  BtkPrintJob *job;
  BtkPrintJobPrivate *priv;
  GObject *object;

  object =
    G_OBJECT_CLASS (btk_print_job_parent_class)->constructor (type,
							      n_construct_properties,
							      construct_params);

  job = BTK_PRINT_JOB (object);

  priv = job->priv;
  g_assert (priv->printer_set &&
	    priv->settings_set &&
	    priv->page_setup_set);
  
  _btk_printer_prepare_for_print (priv->printer,
				  job,
				  priv->settings,
				  priv->page_setup);

  return object;
}


static void
btk_print_job_finalize (GObject *object)
{
  BtkPrintJob *job = BTK_PRINT_JOB (object);
  BtkPrintJobPrivate *priv = job->priv;

  if (priv->spool_io != NULL)
    {
      g_io_channel_unref (priv->spool_io);
      priv->spool_io = NULL;
    }
  
  if (priv->backend)
    g_object_unref (priv->backend);

  if (priv->printer)
    g_object_unref (priv->printer);

  if (priv->surface)
    bairo_surface_destroy (priv->surface);

  if (priv->settings)
    g_object_unref (priv->settings);
  
  if (priv->page_setup)
    g_object_unref (priv->page_setup);

  g_free (job->page_ranges);
  job->page_ranges = NULL;
  
  g_free (priv->title);
  priv->title = NULL;

  G_OBJECT_CLASS (btk_print_job_parent_class)->finalize (object);
}

/**
 * btk_print_job_new:
 * @title: the job title
 * @printer: a #BtkPrinter
 * @settings: a #BtkPrintSettings
 * @page_setup: a #BtkPageSetup
 *
 * Creates a new #BtkPrintJob.
 *
 * Return value: a new #BtkPrintJob
 *
 * Since: 2.10
 **/
BtkPrintJob *
btk_print_job_new (const gchar      *title,
		   BtkPrinter       *printer,
		   BtkPrintSettings *settings,
		   BtkPageSetup     *page_setup)
{
  GObject *result;
  result = g_object_new (BTK_TYPE_PRINT_JOB,
                         "title", title,
			 "printer", printer,
			 "settings", settings,
			 "page-setup", page_setup,
			 NULL);
  return (BtkPrintJob *) result;
}

/**
 * btk_print_job_get_settings:
 * @job: a #BtkPrintJob
 * 
 * Gets the #BtkPrintSettings of the print job.
 * 
 * Return value: (transfer none): the settings of @job
 *
 * Since: 2.10
 */
BtkPrintSettings *
btk_print_job_get_settings (BtkPrintJob *job)
{
  g_return_val_if_fail (BTK_IS_PRINT_JOB (job), NULL);
  
  return job->priv->settings;
}

/**
 * btk_print_job_get_printer:
 * @job: a #BtkPrintJob
 * 
 * Gets the #BtkPrinter of the print job.
 * 
 * Return value: (transfer none): the printer of @job
 *
 * Since: 2.10
 */
BtkPrinter *
btk_print_job_get_printer (BtkPrintJob *job)
{
  g_return_val_if_fail (BTK_IS_PRINT_JOB (job), NULL);
  
  return job->priv->printer;
}

/**
 * btk_print_job_get_title:
 * @job: a #BtkPrintJob
 * 
 * Gets the job title.
 * 
 * Return value: the title of @job
 *
 * Since: 2.10
 */
const gchar *
btk_print_job_get_title (BtkPrintJob *job)
{
  g_return_val_if_fail (BTK_IS_PRINT_JOB (job), NULL);
  
  return job->priv->title;
}

/**
 * btk_print_job_get_status:
 * @job: a #BtkPrintJob
 * 
 * Gets the status of the print job.
 * 
 * Return value: the status of @job
 *
 * Since: 2.10
 */
BtkPrintStatus
btk_print_job_get_status (BtkPrintJob *job)
{
  g_return_val_if_fail (BTK_IS_PRINT_JOB (job), BTK_PRINT_STATUS_FINISHED);
  
  return job->priv->status;
}

void
btk_print_job_set_status (BtkPrintJob   *job,
			  BtkPrintStatus status)
{
  BtkPrintJobPrivate *priv;

  g_return_if_fail (BTK_IS_PRINT_JOB (job));

  priv = job->priv;

  if (priv->status == status)
    return;

  priv->status = status;
  g_signal_emit (job, signals[STATUS_CHANGED], 0);
}

/**
 * btk_print_job_set_source_file:
 * @job: a #BtkPrintJob
 * @filename: the file to be printed
 * @error: return location for errors
 * 
 * Make the #BtkPrintJob send an existing document to the 
 * printing system. The file can be in any format understood
 * by the platforms printing system (typically PostScript,
 * but on many platforms PDF may work too). See 
 * btk_printer_accepts_pdf() and btk_printer_accepts_ps().
 * 
 * Returns: %FALSE if an error occurred
 *
 * Since: 2.10
 **/
gboolean
btk_print_job_set_source_file (BtkPrintJob *job,
			       const gchar *filename,
			       GError     **error)
{
  BtkPrintJobPrivate *priv;
  GError *tmp_error;

  tmp_error = NULL;

  g_return_val_if_fail (BTK_IS_PRINT_JOB (job), FALSE);

  priv = job->priv;

  priv->spool_io = g_io_channel_new_file (filename, "r", &tmp_error);

  if (tmp_error == NULL)
    g_io_channel_set_encoding (priv->spool_io, NULL, &tmp_error);

  if (tmp_error != NULL)
    {
      g_propagate_error (error, tmp_error);
      return FALSE;
    }

  return TRUE;
}

/**
 * btk_print_job_get_surface:
 * @job: a #BtkPrintJob
 * @error: (allow-none): return location for errors, or %NULL
 * 
 * Gets a bairo surface onto which the pages of
 * the print job should be rendered.
 * 
 * Return value: (transfer none): the bairo surface of @job
 *
 * Since: 2.10
 **/
bairo_surface_t *
btk_print_job_get_surface (BtkPrintJob  *job,
			   GError      **error)
{
  BtkPrintJobPrivate *priv;
  gchar *filename = NULL;
  gdouble width, height;
  BtkPaperSize *paper_size;
  int fd;
  GError *tmp_error;

  tmp_error = NULL;

  g_return_val_if_fail (BTK_IS_PRINT_JOB (job), NULL);

  priv = job->priv;

  if (priv->surface)
    return priv->surface;
 
  g_return_val_if_fail (priv->spool_io == NULL, NULL);
 
  fd = g_file_open_tmp ("btkprint_XXXXXX", 
			 &filename, 
			 &tmp_error);
  if (fd == -1)
    {
      g_free (filename);
      g_propagate_error (error, tmp_error);
      return NULL;
    }

  fchmod (fd, S_IRUSR | S_IWUSR);
  
#ifdef G_ENABLE_DEBUG 
  /* If we are debugging printing don't delete the tmp files */
  if (!(btk_debug_flags & BTK_DEBUG_PRINTING))
#endif /* G_ENABLE_DEBUG */
  g_unlink (filename);
  g_free (filename);

  paper_size = btk_page_setup_get_paper_size (priv->page_setup);
  width = btk_paper_size_get_width (paper_size, BTK_UNIT_POINTS);
  height = btk_paper_size_get_height (paper_size, BTK_UNIT_POINTS);
 
  priv->spool_io = g_io_channel_unix_new (fd);
  g_io_channel_set_close_on_unref (priv->spool_io, TRUE);
  g_io_channel_set_encoding (priv->spool_io, NULL, &tmp_error);
  
  if (tmp_error != NULL)
    {
      g_io_channel_unref (priv->spool_io);
      priv->spool_io = NULL;
      g_propagate_error (error, tmp_error);
      return NULL;
    }

  priv->surface = _btk_printer_create_bairo_surface (priv->printer,
						     priv->settings,
						     width, height,
						     priv->spool_io);
  
  return priv->surface;
}

/**
 * btk_print_job_set_track_print_status:
 * @job: a #BtkPrintJob
 * @track_status: %TRUE to track status after printing
 * 
 * If track_status is %TRUE, the print job will try to continue report
 * on the status of the print job in the printer queues and printer. This
 * can allow your application to show things like "out of paper" issues,
 * and when the print job actually reaches the printer.
 * 
 * This function is often implemented using some form of polling, so it should
 * not be enabled unless needed.
 *
 * Since: 2.10
 */
void
btk_print_job_set_track_print_status (BtkPrintJob *job,
				      gboolean     track_status)
{
  BtkPrintJobPrivate *priv;

  g_return_if_fail (BTK_IS_PRINT_JOB (job));

  priv = job->priv;

  track_status = track_status != FALSE;

  if (priv->track_print_status != track_status)
    {
      priv->track_print_status = track_status;
      
      g_object_notify (G_OBJECT (job), "track-print-status");
    }
}

/**
 * btk_print_job_get_track_print_status:
 * @job: a #BtkPrintJob
 *
 * Returns wheter jobs will be tracked after printing.
 * For details, see btk_print_job_set_track_print_status().
 *
 * Return value: %TRUE if print job status will be reported after printing
 *
 * Since: 2.10
 */
gboolean
btk_print_job_get_track_print_status (BtkPrintJob *job)
{
  BtkPrintJobPrivate *priv;

  g_return_val_if_fail (BTK_IS_PRINT_JOB (job), FALSE);

  priv = job->priv;
  
  return priv->track_print_status;
}

static void
btk_print_job_set_property (GObject      *object,
	                    guint         prop_id,
	                    const GValue *value,
                            GParamSpec   *pspec)

{
  BtkPrintJob *job = BTK_PRINT_JOB (object);
  BtkPrintJobPrivate *priv = job->priv;
  BtkPrintSettings *settings;

  switch (prop_id)
    {
    case PROP_TITLE:
      g_free (priv->title);
      priv->title = g_value_dup_string (value);
      break;
    
    case PROP_PRINTER:
      priv->printer = BTK_PRINTER (g_value_dup_object (value));
      priv->printer_set = TRUE;
      priv->backend = g_object_ref (btk_printer_get_backend (priv->printer));
      break;

    case PROP_PAGE_SETUP:
      priv->page_setup = BTK_PAGE_SETUP (g_value_dup_object (value));
      priv->page_setup_set = TRUE;
      break;
      
    case PROP_SETTINGS:
      /* We save a copy of the settings since we modify
       * if when preparing the printer job. */
      settings = BTK_PRINT_SETTINGS (g_value_get_object (value));
      priv->settings = btk_print_settings_copy (settings);
      priv->settings_set = TRUE;
      break;

    case PROP_TRACK_PRINT_STATUS:
      btk_print_job_set_track_print_status (job, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_print_job_get_property (GObject    *object,
			    guint       prop_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
  BtkPrintJob *job = BTK_PRINT_JOB (object);
  BtkPrintJobPrivate *priv = job->priv;

  switch (prop_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;
    case PROP_PRINTER:
      g_value_set_object (value, priv->printer);
      break;
    case PROP_SETTINGS:
      g_value_set_object (value, priv->settings);
      break;
    case PROP_PAGE_SETUP:
      g_value_set_object (value, priv->page_setup);
      break;
    case PROP_TRACK_PRINT_STATUS:
      g_value_set_boolean (value, priv->track_print_status);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * btk_print_job_send:
 * @job: a BtkPrintJob
 * @callback: function to call when the job completes or an error occurs
 * @user_data: user data that gets passed to @callback
 * @dnotify: destroy notify for @user_data
 * 
 * Sends the print job off to the printer.  
 * 
 * Since: 2.10
 **/
void
btk_print_job_send (BtkPrintJob             *job,
                    BtkPrintJobCompleteFunc  callback,
                    gpointer                 user_data,
		    GDestroyNotify           dnotify)
{
  BtkPrintJobPrivate *priv;

  g_return_if_fail (BTK_IS_PRINT_JOB (job));

  priv = job->priv;
  g_return_if_fail (priv->spool_io != NULL);
  
  btk_print_job_set_status (job, BTK_PRINT_STATUS_SENDING_DATA);
  
  g_io_channel_seek_position (priv->spool_io, 0, G_SEEK_SET, NULL);
  
  btk_print_backend_print_stream (priv->backend, job,
				  priv->spool_io,
                                  callback, user_data, dnotify);
}


#define __BTK_PRINT_JOB_C__
#include "btkaliasdef.c"
