/* BtkPrinter
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

#include "btkintl.h"
#include "btkprivate.h"

#include "btkprinter.h"
#include "btkprinter-private.h"
#include "btkprintbackend.h"
#include "btkprintjob.h"
#include "btkalias.h"

#define BTK_PRINTER_GET_PRIVATE(o)  \
   (B_TYPE_INSTANCE_GET_PRIVATE ((o), BTK_TYPE_PRINTER, BtkPrinterPrivate))

static void btk_printer_finalize     (BObject *object);

struct _BtkPrinterPrivate
{
  gchar *name;
  gchar *location;
  gchar *description;
  gchar *icon_name;

  guint is_active         : 1;
  guint is_paused         : 1;
  guint is_accepting_jobs : 1;
  guint is_new            : 1;
  guint is_virtual        : 1;
  guint is_default        : 1;
  guint has_details       : 1;
  guint accepts_pdf       : 1;
  guint accepts_ps        : 1;

  gchar *state_message;  
  gint job_count;

  BtkPrintBackend *backend;
};

enum {
  DETAILS_ACQUIRED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_NAME,
  PROP_BACKEND,
  PROP_IS_VIRTUAL,
  PROP_STATE_MESSAGE,
  PROP_LOCATION,
  PROP_ICON_NAME,
  PROP_JOB_COUNT,
  PROP_ACCEPTS_PDF,
  PROP_ACCEPTS_PS,
  PROP_PAUSED,
  PROP_ACCEPTING_JOBS
};

static guint signals[LAST_SIGNAL] = { 0 };

static void btk_printer_set_property (BObject      *object,
				      guint         prop_id,
				      const BValue *value,
				      BParamSpec   *pspec);
static void btk_printer_get_property (BObject      *object,
				      guint         prop_id,
				      BValue       *value,
				      BParamSpec   *pspec);

G_DEFINE_TYPE (BtkPrinter, btk_printer, B_TYPE_OBJECT)

static void
btk_printer_class_init (BtkPrinterClass *class)
{
  BObjectClass *object_class;
  object_class = (BObjectClass *) class;

  object_class->finalize = btk_printer_finalize;

  object_class->set_property = btk_printer_set_property;
  object_class->get_property = btk_printer_get_property;

  g_type_class_add_private (class, sizeof (BtkPrinterPrivate));

  g_object_class_install_property (B_OBJECT_CLASS (class),
                                   PROP_NAME,
                                   g_param_spec_string ("name",
						        P_("Name"),
						        P_("Name of the printer"),
						        "",
							BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (B_OBJECT_CLASS (class),
                                   PROP_BACKEND,
                                   g_param_spec_object ("backend",
						        P_("Backend"),
						        P_("Backend for the printer"),
						        BTK_TYPE_PRINT_BACKEND,
							BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (B_OBJECT_CLASS (class),
                                   PROP_IS_VIRTUAL,
                                   g_param_spec_boolean ("is-virtual",
							 P_("Is Virtual"),
							 P_("FALSE if this represents a real hardware printer"),
							 FALSE,
							 BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (B_OBJECT_CLASS (class),
                                   PROP_ACCEPTS_PDF,
                                   g_param_spec_boolean ("accepts-pdf",
							 P_("Accepts PDF"),
							 P_("TRUE if this printer can accept PDF"),
							 FALSE,
							 BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (B_OBJECT_CLASS (class),
                                   PROP_ACCEPTS_PS,
                                   g_param_spec_boolean ("accepts-ps",
							 P_("Accepts PostScript"),
							 P_("TRUE if this printer can accept PostScript"),
							 TRUE,
							 BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (B_OBJECT_CLASS (class),
                                   PROP_STATE_MESSAGE,
                                   g_param_spec_string ("state-message",
						        P_("State Message"),
						        P_("String giving the current state of the printer"),
						        "",
							BTK_PARAM_READABLE));
  g_object_class_install_property (B_OBJECT_CLASS (class),
                                   PROP_LOCATION,
                                   g_param_spec_string ("location",
						        P_("Location"),
						        P_("The location of the printer"),
						        "",
							BTK_PARAM_READABLE));
  g_object_class_install_property (B_OBJECT_CLASS (class),
                                   PROP_ICON_NAME,
                                   g_param_spec_string ("icon-name",
						        P_("Icon Name"),
						        P_("The icon name to use for the printer"),
						        "",
							BTK_PARAM_READABLE));
  g_object_class_install_property (B_OBJECT_CLASS (class),
                                   PROP_JOB_COUNT,
				   g_param_spec_int ("job-count",
 						     P_("Job Count"),
 						     P_("Number of jobs queued in the printer"),
 						     0,
 						     G_MAXINT,
 						     0,
 						     BTK_PARAM_READABLE));

  /**
   * BtkPrinter:paused:
   *
   * This property is %TRUE if this printer is paused. 
   * A paused printer still accepts jobs, but it does 
   * not print them.
   *
   * Since: 2.14
   */
  g_object_class_install_property (B_OBJECT_CLASS (class),
                                   PROP_PAUSED,
                                   g_param_spec_boolean ("paused",
							 P_("Paused Printer"),
							 P_("TRUE if this printer is paused"),
							 FALSE,
							 BTK_PARAM_READABLE));
  /**
   * BtkPrinter:accepting-jobs:
   *
   * This property is %TRUE if the printer is accepting jobs.
   *
   * Since: 2.14
   */ 
  g_object_class_install_property (B_OBJECT_CLASS (class),
                                   PROP_ACCEPTING_JOBS,
                                   g_param_spec_boolean ("accepting-jobs",
							 P_("Accepting Jobs"),
							 P_("TRUE if this printer is accepting new jobs"),
							 TRUE,
							 BTK_PARAM_READABLE));

  /**
   * BtkPrinter::details-acquired:
   * @printer: the #BtkPrinter on which the signal is emitted
   * @success: %TRUE if the details were successfully acquired
   *
   * Gets emitted in response to a request for detailed information
   * about a printer from the print backend. The @success parameter
   * indicates if the information was actually obtained.
   *
   * Since: 2.10
   */
  signals[DETAILS_ACQUIRED] =
    g_signal_new (I_("details-acquired"),
		  B_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkPrinterClass, details_acquired),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__BOOLEAN,
		  B_TYPE_NONE, 1, B_TYPE_BOOLEAN);
}

static void
btk_printer_init (BtkPrinter *printer)
{
  BtkPrinterPrivate *priv;

  priv = printer->priv = BTK_PRINTER_GET_PRIVATE (printer); 

  priv->name = NULL;
  priv->location = NULL;
  priv->description = NULL;
  priv->icon_name = NULL;

  priv->is_active = TRUE;
  priv->is_paused = FALSE;
  priv->is_accepting_jobs = TRUE;
  priv->is_new = TRUE;
  priv->has_details = FALSE;
  priv->accepts_pdf = FALSE;
  priv->accepts_ps = TRUE;

  priv->state_message = NULL;  
  priv->job_count = 0;
}

static void
btk_printer_finalize (BObject *object)
{
  BtkPrinter *printer = BTK_PRINTER (object);
  BtkPrinterPrivate *priv = printer->priv;

  g_free (priv->name);
  g_free (priv->location);
  g_free (priv->description);
  g_free (priv->state_message);
  g_free (priv->icon_name);

  if (priv->backend)
    g_object_unref (priv->backend);

  B_OBJECT_CLASS (btk_printer_parent_class)->finalize (object);
}

static void
btk_printer_set_property (BObject         *object,
			  guint            prop_id,
			  const BValue    *value,
			  BParamSpec      *pspec)
{
  BtkPrinter *printer = BTK_PRINTER (object);
  BtkPrinterPrivate *priv = printer->priv;

  switch (prop_id)
    {
    case PROP_NAME:
      priv->name = b_value_dup_string (value);
      break;
    
    case PROP_BACKEND:
      priv->backend = BTK_PRINT_BACKEND (b_value_dup_object (value));
      break;

    case PROP_IS_VIRTUAL:
      priv->is_virtual = b_value_get_boolean (value);
      break;

    case PROP_ACCEPTS_PDF:
      priv->accepts_pdf = b_value_get_boolean (value);
      break;

    case PROP_ACCEPTS_PS:
      priv->accepts_ps = b_value_get_boolean (value);
      break;

    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_printer_get_property (BObject    *object,
			  guint       prop_id,
			  BValue     *value,
			  BParamSpec *pspec)
{
  BtkPrinter *printer = BTK_PRINTER (object);
  BtkPrinterPrivate *priv = printer->priv;

  switch (prop_id)
    {
    case PROP_NAME:
      if (priv->name)
	b_value_set_string (value, priv->name);
      else
	b_value_set_static_string (value, "");
      break;
    case PROP_BACKEND:
      b_value_set_object (value, priv->backend);
      break;
    case PROP_STATE_MESSAGE:
      if (priv->state_message)
	b_value_set_string (value, priv->state_message);
      else
	b_value_set_static_string (value, "");
      break;
    case PROP_LOCATION:
      if (priv->location)
	b_value_set_string (value, priv->location);
      else
	b_value_set_static_string (value, "");
      break;
    case PROP_ICON_NAME:
      if (priv->icon_name)
	b_value_set_string (value, priv->icon_name);
      else
	b_value_set_static_string (value, "");
      break;
    case PROP_JOB_COUNT:
      b_value_set_int (value, priv->job_count);
      break;
    case PROP_IS_VIRTUAL:
      b_value_set_boolean (value, priv->is_virtual);
      break;
    case PROP_ACCEPTS_PDF:
      b_value_set_boolean (value, priv->accepts_pdf);
      break;
    case PROP_ACCEPTS_PS:
      b_value_set_boolean (value, priv->accepts_ps);
      break;
    case PROP_PAUSED:
      b_value_set_boolean (value, priv->is_paused);
      break;
    case PROP_ACCEPTING_JOBS:
      b_value_set_boolean (value, priv->is_accepting_jobs);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * btk_printer_new:
 * @name: the name of the printer
 * @backend: a #BtkPrintBackend
 * @virtual_: whether the printer is virtual
 *
 * Creates a new #BtkPrinter.
 *
 * Return value: a new #BtkPrinter
 *
 * Since: 2.10
 **/
BtkPrinter *
btk_printer_new (const gchar     *name,
		 BtkPrintBackend *backend,
		 gboolean         virtual_)
{
  BObject *result;
  
  result = g_object_new (BTK_TYPE_PRINTER,
			 "name", name,
			 "backend", backend,
			 "is-virtual", virtual_,
                         NULL);

  return (BtkPrinter *) result;
}

/**
 * btk_printer_get_backend:
 * @printer: a #BtkPrinter
 * 
 * Returns the backend of the printer.
 * 
 * Return value: (transfer none): the backend of @printer
 * 
 * Since: 2.10
 */
BtkPrintBackend *
btk_printer_get_backend (BtkPrinter *printer)
{
  g_return_val_if_fail (BTK_IS_PRINTER (printer), NULL);
  
  return printer->priv->backend;
}

/**
 * btk_printer_get_name:
 * @printer: a #BtkPrinter
 * 
 * Returns the name of the printer.
 * 
 * Return value: the name of @printer
 *
 * Since: 2.10
 */
const gchar *
btk_printer_get_name (BtkPrinter *printer)
{
  g_return_val_if_fail (BTK_IS_PRINTER (printer), NULL);

  return printer->priv->name;
}

/**
 * btk_printer_get_description:
 * @printer: a #BtkPrinter
 * 
 * Gets the description of the printer.
 * 
 * Return value: the description of @printer
 *
 * Since: 2.10
 */
const gchar *
btk_printer_get_description (BtkPrinter *printer)
{
  g_return_val_if_fail (BTK_IS_PRINTER (printer), NULL);
  
  return printer->priv->description;
}

gboolean
btk_printer_set_description (BtkPrinter  *printer,
			     const gchar *description)
{
  BtkPrinterPrivate *priv;

  g_return_val_if_fail (BTK_IS_PRINTER (printer), FALSE);

  priv = printer->priv;

  if (g_strcmp0 (priv->description, description) == 0)
    return FALSE;

  g_free (priv->description);
  priv->description = g_strdup (description);
  
  return TRUE;
}

/**
 * btk_printer_get_state_message:
 * @printer: a #BtkPrinter
 * 
 * Returns the state message describing the current state
 * of the printer.
 * 
 * Return value: the state message of @printer
 *
 * Since: 2.10
 */
const gchar *
btk_printer_get_state_message (BtkPrinter *printer)
{
  g_return_val_if_fail (BTK_IS_PRINTER (printer), NULL);

  return printer->priv->state_message;
}

gboolean
btk_printer_set_state_message (BtkPrinter  *printer,
			       const gchar *message)
{
  BtkPrinterPrivate *priv;

  g_return_val_if_fail (BTK_IS_PRINTER (printer), FALSE);

  priv = printer->priv;

  if (g_strcmp0 (priv->state_message, message) == 0)
    return FALSE;

  g_free (priv->state_message);
  priv->state_message = g_strdup (message);
  g_object_notify (B_OBJECT (printer), "state-message");

  return TRUE;
}

/**
 * btk_printer_get_location:
 * @printer: a #BtkPrinter
 * 
 * Returns a description of the location of the printer.
 * 
 * Return value: the location of @printer
 *
 * Since: 2.10
 */
const gchar *
btk_printer_get_location (BtkPrinter *printer)
{
  g_return_val_if_fail (BTK_IS_PRINTER (printer), NULL);

  return printer->priv->location;
}

gboolean
btk_printer_set_location (BtkPrinter  *printer,
			  const gchar *location)
{
  BtkPrinterPrivate *priv;

  g_return_val_if_fail (BTK_IS_PRINTER (printer), FALSE);

  priv = printer->priv;

  if (g_strcmp0 (priv->location, location) == 0)
    return FALSE;

  g_free (priv->location);
  priv->location = g_strdup (location);
  g_object_notify (B_OBJECT (printer), "location");
  
  return TRUE;
}

/**
 * btk_printer_get_icon_name:
 * @printer: a #BtkPrinter
 * 
 * Gets the name of the icon to use for the printer.
 * 
 * Return value: the icon name for @printer
 *
 * Since: 2.10
 */
const gchar *
btk_printer_get_icon_name (BtkPrinter *printer)
{
  g_return_val_if_fail (BTK_IS_PRINTER (printer), NULL);

  return printer->priv->icon_name;
}

void
btk_printer_set_icon_name (BtkPrinter  *printer,
			   const gchar *icon)
{
  BtkPrinterPrivate *priv;

  g_return_if_fail (BTK_IS_PRINTER (printer));

  priv = printer->priv;

  g_free (priv->icon_name);
  priv->icon_name = g_strdup (icon);
  g_object_notify (B_OBJECT (printer), "icon-name");
}

/**
 * btk_printer_get_job_count:
 * @printer: a #BtkPrinter
 * 
 * Gets the number of jobs currently queued on the printer.
 * 
 * Return value: the number of jobs on @printer
 *
 * Since: 2.10
 */
gint 
btk_printer_get_job_count (BtkPrinter *printer)
{
  g_return_val_if_fail (BTK_IS_PRINTER (printer), 0);

  return printer->priv->job_count;
}

gboolean
btk_printer_set_job_count (BtkPrinter *printer,
			   gint        count)
{
  BtkPrinterPrivate *priv;

  g_return_val_if_fail (BTK_IS_PRINTER (printer), FALSE);

  priv = printer->priv;

  if (priv->job_count == count)
    return FALSE;

  priv->job_count = count;
  
  g_object_notify (B_OBJECT (printer), "job-count");
  
  return TRUE;
}

/**
 * btk_printer_has_details:
 * @printer: a #BtkPrinter
 * 
 * Returns whether the printer details are available.
 * 
 * Return value: %TRUE if @printer details are available
 *
 * Since: 2.12
 */
gboolean
btk_printer_has_details (BtkPrinter *printer)
{
  g_return_val_if_fail (BTK_IS_PRINTER (printer), FALSE);

  return printer->priv->has_details;
}

void
btk_printer_set_has_details (BtkPrinter *printer,
			     gboolean val)
{
  printer->priv->has_details = val;
}

/**
 * btk_printer_is_active:
 * @printer: a #BtkPrinter
 * 
 * Returns whether the printer is currently active (i.e. 
 * accepts new jobs).
 * 
 * Return value: %TRUE if @printer is active
 *
 * Since: 2.10
 */
gboolean
btk_printer_is_active (BtkPrinter *printer)
{
  g_return_val_if_fail (BTK_IS_PRINTER (printer), TRUE);
  
  return printer->priv->is_active;
}

void
btk_printer_set_is_active (BtkPrinter *printer,
			   gboolean val)
{
  g_return_if_fail (BTK_IS_PRINTER (printer));

  printer->priv->is_active = val;
}

/**
 * btk_printer_is_paused:
 * @printer: a #BtkPrinter
 * 
 * Returns whether the printer is currently paused.
 * A paused printer still accepts jobs, but it is not
 * printing them.
 * 
 * Return value: %TRUE if @printer is paused
 *
 * Since: 2.14
 */
gboolean
btk_printer_is_paused (BtkPrinter *printer)
{
  g_return_val_if_fail (BTK_IS_PRINTER (printer), TRUE);
  
  return printer->priv->is_paused;
}

gboolean
btk_printer_set_is_paused (BtkPrinter *printer,
			   gboolean    val)
{
  BtkPrinterPrivate *priv;

  g_return_val_if_fail (BTK_IS_PRINTER (printer), FALSE);

  priv = printer->priv;

  if (val == priv->is_paused)
    return FALSE;

  priv->is_paused = val;

  return TRUE;
}

/**
 * btk_printer_is_accepting_jobs:
 * @printer: a #BtkPrinter
 * 
 * Returns whether the printer is accepting jobs
 * 
 * Return value: %TRUE if @printer is accepting jobs
 *
 * Since: 2.14
 */
gboolean
btk_printer_is_accepting_jobs (BtkPrinter *printer)
{
  g_return_val_if_fail (BTK_IS_PRINTER (printer), TRUE);
  
  return printer->priv->is_accepting_jobs;
}

gboolean
btk_printer_set_is_accepting_jobs (BtkPrinter *printer,
				   gboolean val)
{
  BtkPrinterPrivate *priv;

  g_return_val_if_fail (BTK_IS_PRINTER (printer), FALSE);

  priv = printer->priv;

  if (val == priv->is_accepting_jobs)
    return FALSE;

  priv->is_accepting_jobs = val;

  return TRUE;
}

/**
 * btk_printer_is_virtual:
 * @printer: a #BtkPrinter
 * 
 * Returns whether the printer is virtual (i.e. does not
 * represent actual printer hardware, but something like 
 * a CUPS class).
 * 
 * Return value: %TRUE if @printer is virtual
 *
 * Since: 2.10
 */
gboolean
btk_printer_is_virtual (BtkPrinter *printer)
{
  g_return_val_if_fail (BTK_IS_PRINTER (printer), TRUE);
  
  return printer->priv->is_virtual;
}

/**
 * btk_printer_accepts_pdf:
 * @printer: a #BtkPrinter
 *
 * Returns whether the printer accepts input in
 * PDF format.  
 *
 * Return value: %TRUE if @printer accepts PDF
 *
 * Since: 2.10
 */
gboolean 
btk_printer_accepts_pdf (BtkPrinter *printer)
{ 
  g_return_val_if_fail (BTK_IS_PRINTER (printer), TRUE);
  
  return printer->priv->accepts_pdf;
}

void
btk_printer_set_accepts_pdf (BtkPrinter *printer,
			     gboolean val)
{
  g_return_if_fail (BTK_IS_PRINTER (printer));

  printer->priv->accepts_pdf = val;
}

/**
 * btk_printer_accepts_ps:
 * @printer: a #BtkPrinter
 *
 * Returns whether the printer accepts input in
 * PostScript format.  
 *
 * Return value: %TRUE if @printer accepts PostScript
 *
 * Since: 2.10
 */
gboolean 
btk_printer_accepts_ps (BtkPrinter *printer)
{ 
  g_return_val_if_fail (BTK_IS_PRINTER (printer), TRUE);
  
  return printer->priv->accepts_ps;
}

void
btk_printer_set_accepts_ps (BtkPrinter *printer,
			    gboolean val)
{
  g_return_if_fail (BTK_IS_PRINTER (printer));

  printer->priv->accepts_ps = val;
}

gboolean
btk_printer_is_new (BtkPrinter *printer)
{
  g_return_val_if_fail (BTK_IS_PRINTER (printer), FALSE);
  
  return printer->priv->is_new;
}

void
btk_printer_set_is_new (BtkPrinter *printer,
			gboolean val)
{
  g_return_if_fail (BTK_IS_PRINTER (printer));

  printer->priv->is_new = val;
}


/**
 * btk_printer_is_default:
 * @printer: a #BtkPrinter
 * 
 * Returns whether the printer is the default printer.
 * 
 * Return value: %TRUE if @printer is the default
 *
 * Since: 2.10
 */
gboolean
btk_printer_is_default (BtkPrinter *printer)
{
  g_return_val_if_fail (BTK_IS_PRINTER (printer), FALSE);
  
  return printer->priv->is_default;
}

void
btk_printer_set_is_default (BtkPrinter *printer,
			    gboolean    val)
{
  g_return_if_fail (BTK_IS_PRINTER (printer));

  printer->priv->is_default = val;
}

/**
 * btk_printer_request_details:
 * @printer: a #BtkPrinter
 * 
 * Requests the printer details. When the details are available,
 * the #BtkPrinter::details-acquired signal will be emitted on @printer.
 * 
 * Since: 2.12
 */
void
btk_printer_request_details (BtkPrinter *printer)
{
  BtkPrintBackendClass *backend_class;

  g_return_if_fail (BTK_IS_PRINTER (printer));

  backend_class = BTK_PRINT_BACKEND_GET_CLASS (printer->priv->backend);
  backend_class->printer_request_details (printer);
}

BtkPrinterOptionSet *
_btk_printer_get_options (BtkPrinter           *printer,
			  BtkPrintSettings     *settings,
			  BtkPageSetup         *page_setup,
			  BtkPrintCapabilities  capabilities)
{
  BtkPrintBackendClass *backend_class = BTK_PRINT_BACKEND_GET_CLASS (printer->priv->backend);
  return backend_class->printer_get_options (printer, settings, page_setup, capabilities);
}

gboolean
_btk_printer_mark_conflicts (BtkPrinter          *printer,
			     BtkPrinterOptionSet *options)
{
  BtkPrintBackendClass *backend_class = BTK_PRINT_BACKEND_GET_CLASS (printer->priv->backend);
  return backend_class->printer_mark_conflicts (printer, options);
}
  
void
_btk_printer_get_settings_from_options (BtkPrinter          *printer,
					BtkPrinterOptionSet *options,
					BtkPrintSettings    *settings)
{
  BtkPrintBackendClass *backend_class = BTK_PRINT_BACKEND_GET_CLASS (printer->priv->backend);
  backend_class->printer_get_settings_from_options (printer, options, settings);
}

void
_btk_printer_prepare_for_print (BtkPrinter       *printer,
				BtkPrintJob      *print_job,
				BtkPrintSettings *settings,
				BtkPageSetup     *page_setup)
{
  BtkPrintBackendClass *backend_class = BTK_PRINT_BACKEND_GET_CLASS (printer->priv->backend);
  backend_class->printer_prepare_for_print (printer, print_job, settings, page_setup);
}

bairo_surface_t *
_btk_printer_create_bairo_surface (BtkPrinter       *printer,
				   BtkPrintSettings *settings,
				   gdouble           width, 
				   gdouble           height,
				   BUNNYIOChannel       *cache_io)
{
  BtkPrintBackendClass *backend_class = BTK_PRINT_BACKEND_GET_CLASS (printer->priv->backend);

  return backend_class->printer_create_bairo_surface (printer, settings,
						      width, height, cache_io);
}

/**
 * btk_printer_list_papers:
 * @printer: a #BtkPrinter
 * 
 * Lists all the paper sizes @printer supports.
 * This will return and empty list unless the printer's details are 
 * available, see btk_printer_has_details() and btk_printer_request_details().
 *
 * Return value: (element-type BtkPageSetup) (transfer full): a newly allocated list of newly allocated #BtkPageSetup s.
 *
 * Since: 2.12
 */
GList  *
btk_printer_list_papers (BtkPrinter *printer)
{
  BtkPrintBackendClass *backend_class;

  g_return_val_if_fail (BTK_IS_PRINTER (printer), NULL);

  backend_class = BTK_PRINT_BACKEND_GET_CLASS (printer->priv->backend);
  return backend_class->printer_list_papers (printer);
}

/**
 * btk_printer_get_default_page_size:
 * @printer: a #BtkPrinter
 *
 * Returns default page size of @printer.
 * 
 * Return value: a newly allocated #BtkPageSetup with default page size of the printer.
 *
 * Since: 2.14
 */
BtkPageSetup  *
btk_printer_get_default_page_size (BtkPrinter *printer)
{
  BtkPrintBackendClass *backend_class;

  g_return_val_if_fail (BTK_IS_PRINTER (printer), NULL);

  backend_class = BTK_PRINT_BACKEND_GET_CLASS (printer->priv->backend);
  return backend_class->printer_get_default_page_size (printer);
}

/**
 * btk_printer_get_hard_margins:
 * @printer: a #BtkPrinter
 * @top: (out): a location to store the top margin in
 * @bottom: (out): a location to store the bottom margin in
 * @left: (out): a location to store the left margin in
 * @right: (out): a location to store the right margin in
 *
 * Retrieve the hard margins of @printer, i.e. the margins that define
 * the area at the borders of the paper that the printer cannot print to.
 *
 * Note: This will not succeed unless the printer's details are available,
 * see btk_printer_has_details() and btk_printer_request_details().
 *
 * Return value: %TRUE iff the hard margins were retrieved
 *
 * Since: 2.20
 */
gboolean
btk_printer_get_hard_margins (BtkPrinter *printer,
			      gdouble    *top,
			      gdouble    *bottom,
			      gdouble    *left,
			      gdouble    *right)
{
  BtkPrintBackendClass *backend_class = BTK_PRINT_BACKEND_GET_CLASS (printer->priv->backend);

  return backend_class->printer_get_hard_margins (printer, top, bottom, left, right);
}

/**
 * btk_printer_get_capabilities:
 * @printer: a #BtkPrinter
 * 
 * Returns the printer's capabilities.
 *
 * This is useful when you're using #BtkPrintUnixDialog's manual-capabilities 
 * setting and need to know which settings the printer can handle and which 
 * you must handle yourself.
 *
 * This will return 0 unless the printer's details are available, see
 * btk_printer_has_details() and btk_printer_request_details().
 *
 * Return value: the printer's capabilities
 *
 * Since: 2.12
 */
BtkPrintCapabilities
btk_printer_get_capabilities (BtkPrinter *printer)
{
  BtkPrintBackendClass *backend_class;

  g_return_val_if_fail (BTK_IS_PRINTER (printer), 0);

  backend_class = BTK_PRINT_BACKEND_GET_CLASS (printer->priv->backend);
  return backend_class->printer_get_capabilities (printer);
}

/**
 * btk_printer_compare:
 * @a: a #BtkPrinter
 * @b: another #BtkPrinter
 *
 * Compares two printers.
 * 
 * Return value: 0 if the printer match, a negative value if @a &lt; @b, 
 *   or a positive value if @a &gt; @b
 *
 * Since: 2.10
 */
gint
btk_printer_compare (BtkPrinter *a, 
                     BtkPrinter *b)
{
  const char *name_a, *name_b;
  
  g_assert (BTK_IS_PRINTER (a) && BTK_IS_PRINTER (b));
  
  name_a = btk_printer_get_name (a);
  name_b = btk_printer_get_name (b);
  if (name_a == NULL  && name_b == NULL)
    return 0;
  else if (name_a == NULL)
    return G_MAXINT;
  else if (name_b == NULL)
    return G_MININT;
  else
    return g_ascii_strcasecmp (name_a, name_b);
}


typedef struct 
{
  GList *backends;
  BtkPrinterFunc func;
  gpointer data;
  GDestroyNotify destroy;
  GMainLoop *loop;
} PrinterList;

static void list_done_cb (BtkPrintBackend *backend, 
			  PrinterList     *printer_list);

static void
stop_enumeration (PrinterList *printer_list)
{
  GList *list, *next;
  BtkPrintBackend *backend;

  for (list = printer_list->backends; list; list = next)
    {
      next = list->next;
      backend = BTK_PRINT_BACKEND (list->data);
      list_done_cb (backend, printer_list);
    }
}

static void 
free_printer_list (PrinterList *printer_list)
{
  if (printer_list->destroy)
    printer_list->destroy (printer_list->data);

  if (printer_list->loop)
    {    
      g_main_loop_quit (printer_list->loop);
      g_main_loop_unref (printer_list->loop);
    }

  g_free (printer_list);
}

static gboolean
list_added_cb (BtkPrintBackend *backend, 
	       BtkPrinter      *printer, 
	       PrinterList     *printer_list)
{
  if (printer_list->func (printer, printer_list->data))
    {
      stop_enumeration (printer_list);
      return TRUE;
    }

  return FALSE;
}

static void
backend_status_changed (BObject    *object,
                        BParamSpec *pspec,
                        gpointer    data)
{
  BtkPrintBackend *backend = BTK_PRINT_BACKEND (object);
  PrinterList *printer_list = data;
  BtkPrintBackendStatus status;

  g_object_get (backend, "status", &status, NULL);
 
  if (status == BTK_PRINT_BACKEND_STATUS_UNAVAILABLE)
    list_done_cb (backend, printer_list);  
}

static void
list_printers_remove_backend (PrinterList     *printer_list,
                              BtkPrintBackend *backend)
{
  printer_list->backends = g_list_remove (printer_list->backends, backend);
  btk_print_backend_destroy (backend);
  g_object_unref (backend);

  if (printer_list->backends == NULL)
    free_printer_list (printer_list);
}

static void
list_done_cb (BtkPrintBackend *backend,
	      PrinterList     *printer_list)
{
  g_signal_handlers_disconnect_by_func (backend, list_added_cb, printer_list);
  g_signal_handlers_disconnect_by_func (backend, list_done_cb, printer_list);
  g_signal_handlers_disconnect_by_func (backend, backend_status_changed, printer_list);

  list_printers_remove_backend(printer_list, backend);
}

static gboolean
list_printers_init (PrinterList     *printer_list,
		    BtkPrintBackend *backend)
{
  GList *list, *node;
  BtkPrintBackendStatus status;

  list = btk_print_backend_get_printer_list (backend);

  for (node = list; node != NULL; node = node->next)
    {
      if (list_added_cb (backend, node->data, printer_list))
        {
          g_list_free (list);
          return TRUE;
        }
    }

  g_list_free (list);

  g_object_get (backend, "status", &status, NULL);
  
  if (status == BTK_PRINT_BACKEND_STATUS_UNAVAILABLE || 
      btk_print_backend_printer_list_is_done (backend))
    list_printers_remove_backend(printer_list, backend);
  else
    {
      g_signal_connect (backend, "printer-added", 
			(GCallback) list_added_cb, 
			printer_list);
      g_signal_connect (backend, "printer-list-done", 
			(GCallback) list_done_cb, 
			printer_list);
      g_signal_connect (backend, "notify::status", 
                        (GCallback) backend_status_changed,
                        printer_list);     
    }

  return FALSE;
}

/**
 * btk_enumerate_printers:
 * @func: a function to call for each printer
 * @data: user data to pass to @func
 * @destroy: function to call if @data is no longer needed
 * @wait: if %TRUE, wait in a recursive mainloop until
 *    all printers are enumerated; otherwise return early
 *
 * Calls a function for all #BtkPrinter<!-- -->s. 
 * If @func returns %TRUE, the enumeration is stopped.
 *
 * Since: 2.10
 */
void
btk_enumerate_printers (BtkPrinterFunc func,
			gpointer       data,
			GDestroyNotify destroy,
			gboolean       wait)
{
  PrinterList *printer_list;
  GList *node, *next;
  BtkPrintBackend *backend;

  printer_list = g_new0 (PrinterList, 1);

  printer_list->func = func;
  printer_list->data = data;
  printer_list->destroy = destroy;

  if (g_module_supported ())
    printer_list->backends = btk_print_backend_load_modules ();
  
  if (printer_list->backends == NULL)
    {
      free_printer_list (printer_list);
      return;
    }

  for (node = printer_list->backends; node != NULL; node = next)
    {
      next = node->next;
      backend = BTK_PRINT_BACKEND (node->data);
      if (list_printers_init (printer_list, backend))
        return;
    }

  if (wait && printer_list->backends)
    {
      printer_list->loop = g_main_loop_new (NULL, FALSE);

      BDK_THREADS_LEAVE ();  
      g_main_loop_run (printer_list->loop);
      BDK_THREADS_ENTER ();  
    }
}

GType
btk_print_capabilities_get_type (void)
{
  static GType etype = 0;

  if (B_UNLIKELY (etype == 0))
    {
      static const GFlagsValue values[] = {
        { BTK_PRINT_CAPABILITY_PAGE_SET, "BTK_PRINT_CAPABILITY_PAGE_SET", "page-set" },
        { BTK_PRINT_CAPABILITY_COPIES, "BTK_PRINT_CAPABILITY_COPIES", "copies" },
        { BTK_PRINT_CAPABILITY_COLLATE, "BTK_PRINT_CAPABILITY_COLLATE", "collate" },
        { BTK_PRINT_CAPABILITY_REVERSE, "BTK_PRINT_CAPABILITY_REVERSE", "reverse" },
        { BTK_PRINT_CAPABILITY_SCALE, "BTK_PRINT_CAPABILITY_SCALE", "scale" },
        { BTK_PRINT_CAPABILITY_GENERATE_PDF, "BTK_PRINT_CAPABILITY_GENERATE_PDF", "generate-pdf" },
        { BTK_PRINT_CAPABILITY_GENERATE_PS, "BTK_PRINT_CAPABILITY_GENERATE_PS", "generate-ps" },
        { BTK_PRINT_CAPABILITY_PREVIEW, "BTK_PRINT_CAPABILITY_PREVIEW", "preview" },
	{ BTK_PRINT_CAPABILITY_NUMBER_UP, "BTK_PRINT_CAPABILITY_NUMBER_UP", "number-up"},
        { BTK_PRINT_CAPABILITY_NUMBER_UP_LAYOUT, "BTK_PRINT_CAPABILITY_NUMBER_UP_LAYOUT", "number-up-layout" },
        { 0, NULL, NULL }
      };

      etype = g_flags_register_static (I_("BtkPrintCapabilities"), values);
    }

  return etype;
}


#define __BTK_PRINTER_C__
#include "btkaliasdef.c"
