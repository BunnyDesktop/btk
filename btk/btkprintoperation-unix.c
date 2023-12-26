/* BTK - The GIMP Toolkit
 * btkprintoperation-unix.c: Print Operation Details for Unix 
 *                           and Unix-like platforms
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

#include "config.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>       
#include <fcntl.h>

#include <bunnylib/gstdio.h>
#include "btkprintoperation-private.h"
#include "btkmessagedialog.h"

#include <bairo-pdf.h>
#include <bairo-ps.h>
#include "btkprivate.h"
#include "btkprintunixdialog.h"
#include "btkpagesetupunixdialog.h"
#include "btkprintbackend.h"
#include "btkprinter.h"
#include "btkprintjob.h"
#include "btklabel.h"
#include "btkintl.h"
#include "btkalias.h"

typedef struct 
{
  BtkWindow *parent;        /* just in case we need to throw error dialogs */
  GMainLoop *loop;
  bboolean data_sent;

  /* Real printing (not preview) */
  BtkPrintJob *job;         /* the job we are sending to the printer */
  bairo_surface_t *surface;
  bulong job_status_changed_tag;

  
} BtkPrintOperationUnix;

typedef struct _PrinterFinder PrinterFinder;

static void printer_finder_free (PrinterFinder *finder);
static void find_printer        (const bchar   *printer,
				 GFunc          func,
				 bpointer       data);

static void
unix_start_page (BtkPrintOperation *op,
		 BtkPrintContext   *print_context,
		 BtkPageSetup      *page_setup)
{
  BtkPrintOperationUnix *op_unix;  
  BtkPaperSize *paper_size;
  bairo_surface_type_t type;
  bdouble w, h;

  op_unix = op->priv->platform_data;
  
  paper_size = btk_page_setup_get_paper_size (page_setup);

  w = btk_paper_size_get_width (paper_size, BTK_UNIT_POINTS);
  h = btk_paper_size_get_height (paper_size, BTK_UNIT_POINTS);
  
  type = bairo_surface_get_type (op_unix->surface);

  if ((op->priv->manual_number_up < 2) ||
      (op->priv->page_position % op->priv->manual_number_up == 0))
    {
      if (type == BAIRO_SURFACE_TYPE_PS)
        {
          bairo_ps_surface_set_size (op_unix->surface, w, h);
          bairo_ps_surface_dsc_begin_page_setup (op_unix->surface);
          switch (btk_page_setup_get_orientation (page_setup))
            {
              case BTK_PAGE_ORIENTATION_PORTRAIT:
              case BTK_PAGE_ORIENTATION_REVERSE_PORTRAIT:
                bairo_ps_surface_dsc_comment (op_unix->surface, "%%PageOrientation: Portrait");
                break;

              case BTK_PAGE_ORIENTATION_LANDSCAPE:
              case BTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE:
                bairo_ps_surface_dsc_comment (op_unix->surface, "%%PageOrientation: Landscape");
                break;
            }
         }
      else if (type == BAIRO_SURFACE_TYPE_PDF)
        {
          bairo_pdf_surface_set_size (op_unix->surface, w, h);
        }
    }
}

static void
unix_end_page (BtkPrintOperation *op,
	       BtkPrintContext   *print_context)
{
  bairo_t *cr;

  cr = btk_print_context_get_bairo_context (print_context);

  if ((op->priv->manual_number_up < 2) ||
      ((op->priv->page_position + 1) % op->priv->manual_number_up == 0) ||
      (op->priv->page_position == op->priv->nr_of_pages_to_print - 1))
    bairo_show_page (cr);
}

static void
op_unix_free (BtkPrintOperationUnix *op_unix)
{
  if (op_unix->job)
    {
      if (op_unix->job_status_changed_tag > 0)
        g_signal_handler_disconnect (op_unix->job,
				     op_unix->job_status_changed_tag);
      g_object_unref (op_unix->job);
    }

  g_free (op_unix);
}

static bchar *
shell_command_substitute_file (const bchar *cmd,
			       const bchar *pdf_filename,
			       const bchar *settings_filename,
                               bboolean    *pdf_filename_replaced,
                               bboolean    *settings_filename_replaced)
{
  const bchar *inptr, *start;
  GString *final;

  g_return_val_if_fail (cmd != NULL, NULL);
  g_return_val_if_fail (pdf_filename != NULL, NULL);
  g_return_val_if_fail (settings_filename != NULL, NULL);

  final = g_string_new (NULL);

  *pdf_filename_replaced = FALSE;
  *settings_filename_replaced = FALSE;

  start = inptr = cmd;
  while ((inptr = strchr (inptr, '%')) != NULL) 
    {
      g_string_append_len (final, start, inptr - start);
      inptr++;
      switch (*inptr) 
        {
          case 'f':
            g_string_append (final, pdf_filename);
            *pdf_filename_replaced = TRUE;
            break;

          case 's':
            g_string_append (final, settings_filename);
            *settings_filename_replaced = TRUE;
            break;

          case '%':
            g_string_append_c (final, '%');
            break;

          default:
            g_string_append_c (final, '%');
            if (*inptr)
              g_string_append_c (final, *inptr);
            break;
        }
      if (*inptr)
        inptr++;
      start = inptr;
    }
  g_string_append (final, start);

  return g_string_free (final, FALSE);
}

void
_btk_print_operation_platform_backend_launch_preview (BtkPrintOperation *op,
						      bairo_surface_t   *surface,
						      BtkWindow         *parent,
						      const bchar       *filename)
{
  bint argc;
  bchar **argv;
  bchar *cmd;
  bchar *preview_cmd;
  BtkSettings *settings;
  BtkPrintSettings *print_settings = NULL;
  BtkPageSetup *page_setup;
  GKeyFile *key_file = NULL;
  bchar *data = NULL;
  bsize data_len;
  bchar *settings_filename = NULL;
  bchar *quoted_filename;
  bchar *quoted_settings_filename;
  bboolean filename_used = FALSE;
  bboolean settings_used = FALSE;
  BdkScreen *screen;
  GError *error = NULL;
  bint fd;
  bboolean retval;

  bairo_surface_destroy (surface);
 
  if (parent)
    screen = btk_window_get_screen (parent);
  else
    screen = bdk_screen_get_default ();

  fd = g_file_open_tmp ("settingsXXXXXX.ini", &settings_filename, &error);
  if (fd < 0) 
    goto out;

  key_file = g_key_file_new ();
  
  print_settings = btk_print_settings_copy (btk_print_operation_get_print_settings (op));

  if (print_settings != NULL)
    {
      btk_print_settings_set_reverse (print_settings, FALSE);
      btk_print_settings_set_page_set (print_settings, BTK_PAGE_SET_ALL);
      btk_print_settings_set_scale (print_settings, 1.0);
      btk_print_settings_set_number_up (print_settings, 1);
      btk_print_settings_set_number_up_layout (print_settings, BTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM);

      /*  These removals are neccessary because cups-* settings have higher priority
       *  than normal settings.
       */
      btk_print_settings_unset (print_settings, "cups-reverse");
      btk_print_settings_unset (print_settings, "cups-page-set");
      btk_print_settings_unset (print_settings, "cups-scale");
      btk_print_settings_unset (print_settings, "cups-number-up");
      btk_print_settings_unset (print_settings, "cups-number-up-layout");

      btk_print_settings_to_key_file (print_settings, key_file, NULL);
      g_object_unref (print_settings);
    }

  page_setup = btk_print_context_get_page_setup (op->priv->print_context);
  btk_page_setup_to_key_file (page_setup, key_file, NULL);

  g_key_file_set_string (key_file, "Print Job", "title", op->priv->job_name);

  data = g_key_file_to_data (key_file, &data_len, &error);
  if (!data)
    goto out;

  retval = g_file_set_contents (settings_filename, data, data_len, &error);
  if (!retval)
    goto out;

  settings = btk_settings_get_for_screen (screen);
  g_object_get (settings, "btk-print-preview-command", &preview_cmd, NULL);

  quoted_filename = g_shell_quote (filename);
  quoted_settings_filename = g_shell_quote (settings_filename);
  cmd = shell_command_substitute_file (preview_cmd, quoted_filename, quoted_settings_filename, &filename_used, &settings_used);
  g_shell_parse_argv (cmd, &argc, &argv, &error);

  g_free (preview_cmd);
  g_free (quoted_filename);
  g_free (quoted_settings_filename);
  g_free (cmd);

  if (error != NULL)
    goto out;

  bdk_spawn_on_screen (screen, NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error);

  g_strfreev (argv);

  if (error != NULL)
    {
      bchar* uri;

      g_warning ("%s %s", _("Error launching preview"), error->message);

      g_error_free (error);
      error = NULL;
      uri = g_filename_to_uri (filename, NULL, NULL);
      btk_show_uri (screen, uri, BDK_CURRENT_TIME, &error);
      g_free (uri);
    }

 out:
  if (error != NULL)
    {
      BtkWidget *edialog;
      edialog = btk_message_dialog_new (parent, 
                                        BTK_DIALOG_DESTROY_WITH_PARENT,
                                        BTK_MESSAGE_ERROR,
                                        BTK_BUTTONS_CLOSE,
                                        _("Error launching preview") /* FIXME better text */);
      btk_message_dialog_format_secondary_text (BTK_MESSAGE_DIALOG (edialog),
                                                "%s", error->message);
      g_signal_connect (edialog, "response",
                        G_CALLBACK (btk_widget_destroy), NULL);

      btk_window_present (BTK_WINDOW (edialog));

      g_error_free (error);

      filename_used = FALSE; 
      settings_used = FALSE;
   } 

  if (!filename_used)
    g_unlink (filename);

  if (!settings_used)
    g_unlink (settings_filename);

  if (fd > 0)
    close (fd);
  
  if (key_file)
    g_key_file_free (key_file);
  g_free (data);
  g_free (settings_filename);
}

static void
unix_finish_send  (BtkPrintJob *job,
                   bpointer     user_data, 
                   GError      *error)
{
  BtkPrintOperation *op = (BtkPrintOperation *) user_data;
  BtkPrintOperationUnix *op_unix = op->priv->platform_data;

  if (error != NULL)
    {
      BtkWidget *edialog;
      edialog = btk_message_dialog_new (op_unix->parent, 
                                        BTK_DIALOG_DESTROY_WITH_PARENT,
                                        BTK_MESSAGE_ERROR,
                                        BTK_BUTTONS_CLOSE,
                                        _("Error printing") /* FIXME better text */);
      btk_message_dialog_format_secondary_text (BTK_MESSAGE_DIALOG (edialog),
                                                "%s", error->message);
      btk_window_set_modal (BTK_WINDOW (edialog), TRUE);
      g_signal_connect (edialog, "response",
                        G_CALLBACK (btk_widget_destroy), NULL);

      btk_window_present (BTK_WINDOW (edialog));
    }

  op_unix->data_sent = TRUE;

  if (op_unix->loop)
    g_main_loop_quit (op_unix->loop);

  g_object_unref (op);
}

static void
unix_end_run (BtkPrintOperation *op,
	      bboolean           wait,
	      bboolean           cancelled)
{
  BtkPrintOperationUnix *op_unix = op->priv->platform_data;

  bairo_surface_finish (op_unix->surface);
  
  if (cancelled)
    return;

  if (wait)
    op_unix->loop = g_main_loop_new (NULL, FALSE);
  
  /* TODO: Check for error */
  if (op_unix->job != NULL)
    {
      g_object_ref (op);
      btk_print_job_send (op_unix->job,
                          unix_finish_send, 
                          op, NULL);
    }

  if (wait)
    {
      g_object_ref (op);
      if (!op_unix->data_sent)
	{
	  BDK_THREADS_LEAVE ();  
	  g_main_loop_run (op_unix->loop);
	  BDK_THREADS_ENTER ();  
	}
      g_main_loop_unref (op_unix->loop);
      op_unix->loop = NULL;
      g_object_unref (op);
    }
}

static void
job_status_changed_cb (BtkPrintJob       *job, 
		       BtkPrintOperation *op)
{
  _btk_print_operation_set_status (op, btk_print_job_get_status (job), NULL);
}


static void
print_setup_changed_cb (BtkPrintUnixDialog *print_dialog, 
                        BParamSpec         *pspec,
                        bpointer            user_data)
{
  BtkPageSetup             *page_setup;
  BtkPrintSettings         *print_settings;
  BtkPrintOperation        *op = user_data;
  BtkPrintOperationPrivate *priv = op->priv;

  page_setup = btk_print_unix_dialog_get_page_setup (print_dialog);
  print_settings = btk_print_unix_dialog_get_settings (print_dialog);

  g_signal_emit_by_name (op,
                         "update-custom-widget",
                         priv->custom_widget,
                         page_setup,
                         print_settings);
}

static BtkWidget *
get_print_dialog (BtkPrintOperation *op,
                  BtkWindow         *parent)
{
  BtkPrintOperationPrivate *priv = op->priv;
  BtkWidget *pd, *label;
  const bchar *custom_tab_label;

  pd = btk_print_unix_dialog_new (NULL, parent);

  btk_print_unix_dialog_set_manual_capabilities (BTK_PRINT_UNIX_DIALOG (pd),
						 BTK_PRINT_CAPABILITY_PAGE_SET |
						 BTK_PRINT_CAPABILITY_COPIES |
						 BTK_PRINT_CAPABILITY_COLLATE |
						 BTK_PRINT_CAPABILITY_REVERSE |
						 BTK_PRINT_CAPABILITY_SCALE |
						 BTK_PRINT_CAPABILITY_PREVIEW |
						 BTK_PRINT_CAPABILITY_NUMBER_UP |
						 BTK_PRINT_CAPABILITY_NUMBER_UP_LAYOUT);

  if (priv->print_settings)
    btk_print_unix_dialog_set_settings (BTK_PRINT_UNIX_DIALOG (pd),
					priv->print_settings);

  if (priv->default_page_setup)
    btk_print_unix_dialog_set_page_setup (BTK_PRINT_UNIX_DIALOG (pd), 
                                          priv->default_page_setup);

  btk_print_unix_dialog_set_embed_page_setup (BTK_PRINT_UNIX_DIALOG (pd),
                                              priv->embed_page_setup);

  btk_print_unix_dialog_set_current_page (BTK_PRINT_UNIX_DIALOG (pd), 
                                          priv->current_page);

  btk_print_unix_dialog_set_support_selection (BTK_PRINT_UNIX_DIALOG (pd),
                                               priv->support_selection);

  btk_print_unix_dialog_set_has_selection (BTK_PRINT_UNIX_DIALOG (pd),
                                           priv->has_selection);

  g_signal_emit_by_name (op, "create-custom-widget",
			 &priv->custom_widget);

  if (priv->custom_widget) 
    {
      custom_tab_label = priv->custom_tab_label;
      
      if (custom_tab_label == NULL)
	{
	  custom_tab_label = g_get_application_name ();
	  if (custom_tab_label == NULL)
	    custom_tab_label = _("Application");
	}

      label = btk_label_new (custom_tab_label);
      
      btk_print_unix_dialog_add_custom_tab (BTK_PRINT_UNIX_DIALOG (pd),
					    priv->custom_widget, label);

      g_signal_connect (pd, "notify::selected-printer", (GCallback) print_setup_changed_cb, op);
      g_signal_connect (pd, "notify::page-setup", (GCallback) print_setup_changed_cb, op);
    }
  
  return pd;
}
  
typedef struct 
{
  BtkPrintOperation           *op;
  bboolean                     do_print;
  bboolean                     do_preview;
  BtkPrintOperationResult      result;
  BtkPrintOperationPrintFunc   print_cb;
  GDestroyNotify               destroy;
  BtkWindow                   *parent;
  GMainLoop                   *loop;
} PrintResponseData;

static void
print_response_data_free (bpointer data)
{
  PrintResponseData *rdata = data;

  g_object_unref (rdata->op);
  g_free (rdata);
}

static void
finish_print (PrintResponseData *rdata,
	      BtkPrinter        *printer,
	      BtkPageSetup      *page_setup,
	      BtkPrintSettings  *settings,
	      bboolean           page_setup_set)
{
  BtkPrintOperation *op = rdata->op;
  BtkPrintOperationPrivate *priv = op->priv;
  BtkPrintJob *job;
  bdouble top, bottom, left, right;
  
  if (rdata->do_print)
    {
      btk_print_operation_set_print_settings (op, settings);
      priv->print_context = _btk_print_context_new (op);

      if (btk_print_settings_get_number_up (settings) < 2)
        {
	  if (printer && btk_printer_get_hard_margins (printer, &top, &bottom, &left, &right))
	    _btk_print_context_set_hard_margins (priv->print_context, top, bottom, left, right);
	}
      else
        {
	  /* Pages do not have any unprintable area when printing n-up as each page on the
	   * sheet has been scaled down and translated to a position within the printable
	   * area of the sheet.
	   */
	  _btk_print_context_set_hard_margins (priv->print_context, 0, 0, 0, 0);
	}

      if (page_setup != NULL &&
          (btk_print_operation_get_default_page_setup (op) == NULL ||
           page_setup_set))
        btk_print_operation_set_default_page_setup (op, page_setup);

      _btk_print_context_set_page_setup (priv->print_context, page_setup);

      if (!rdata->do_preview)
        {
	  BtkPrintOperationUnix *op_unix;
	  bairo_t *cr;
	  
	  op_unix = g_new0 (BtkPrintOperationUnix, 1);
	  priv->platform_data = op_unix;
	  priv->free_platform_data = (GDestroyNotify) op_unix_free;
	  op_unix->parent = rdata->parent;
	  
	  priv->start_page = unix_start_page;
	  priv->end_page = unix_end_page;
	  priv->end_run = unix_end_run;
	  
	  job = btk_print_job_new (priv->job_name, printer, settings, page_setup);
          op_unix->job = job;
          btk_print_job_set_track_print_status (job, priv->track_print_status);
	  
	  op_unix->surface = btk_print_job_get_surface (job, &priv->error);
	  if (op_unix->surface == NULL) 
            {
	      rdata->result = BTK_PRINT_OPERATION_RESULT_ERROR;
	      rdata->do_print = FALSE;
	      goto out;
            }
	  
	  cr = bairo_create (op_unix->surface);
	  btk_print_context_set_bairo_context (priv->print_context, cr, 72, 72);
	  bairo_destroy (cr);

          _btk_print_operation_set_status (op, btk_print_job_get_status (job), NULL);
	  
          op_unix->job_status_changed_tag =
	    g_signal_connect (job, "status-changed",
			      G_CALLBACK (job_status_changed_cb), op);
	  
          priv->print_pages = job->print_pages;
          priv->page_ranges = job->page_ranges;
          priv->num_page_ranges = job->num_page_ranges;
	  
          priv->manual_num_copies = job->num_copies;
          priv->manual_collation = job->collate;
          priv->manual_reverse = job->reverse;
          priv->manual_page_set = job->page_set;
          priv->manual_scale = job->scale;
          priv->manual_orientation = job->rotate_to_orientation;
          priv->manual_number_up = job->number_up;
          priv->manual_number_up_layout = job->number_up_layout;
        }
    } 
 out:
  if (rdata->print_cb)
    rdata->print_cb (op, rdata->parent, rdata->do_print, rdata->result); 

  if (rdata->destroy)
    rdata->destroy (rdata);
}

static void 
handle_print_response (BtkWidget *dialog,
		       bint       response,
		       bpointer   data)
{
  BtkPrintUnixDialog *pd = BTK_PRINT_UNIX_DIALOG (dialog);
  PrintResponseData *rdata = data;
  BtkPrintSettings *settings = NULL;
  BtkPageSetup *page_setup = NULL;
  BtkPrinter *printer = NULL;
  bboolean page_setup_set = FALSE;

  if (response == BTK_RESPONSE_OK)
    {
      printer = btk_print_unix_dialog_get_selected_printer (BTK_PRINT_UNIX_DIALOG (pd));

      rdata->result = BTK_PRINT_OPERATION_RESULT_APPLY;
      rdata->do_preview = FALSE;
      if (printer != NULL)
	rdata->do_print = TRUE;
    } 
  else if (response == BTK_RESPONSE_APPLY)
    {
      /* print preview */
      rdata->result = BTK_PRINT_OPERATION_RESULT_APPLY;
      rdata->do_preview = TRUE;
      rdata->do_print = TRUE;

      rdata->op->priv->action = BTK_PRINT_OPERATION_ACTION_PREVIEW;
    }

  if (rdata->do_print)
    {
      settings = btk_print_unix_dialog_get_settings (BTK_PRINT_UNIX_DIALOG (pd));
      page_setup = btk_print_unix_dialog_get_page_setup (BTK_PRINT_UNIX_DIALOG (pd));
      page_setup_set = btk_print_unix_dialog_get_page_setup_set (BTK_PRINT_UNIX_DIALOG (pd));

      /* Set new print settings now so that custom-widget options
       * can be added to the settings in the callback
       */
      btk_print_operation_set_print_settings (rdata->op, settings);
      g_signal_emit_by_name (rdata->op, "custom-widget-apply", rdata->op->priv->custom_widget);
    }
  
  finish_print (rdata, printer, page_setup, settings, page_setup_set);

  if (settings)
    g_object_unref (settings);
    
  btk_widget_destroy (BTK_WIDGET (pd));
 
}


static void
found_printer (BtkPrinter        *printer,
	       PrintResponseData *rdata)
{
  BtkPrintOperation *op = rdata->op;
  BtkPrintOperationPrivate *priv = op->priv;
  BtkPrintSettings *settings = NULL;
  BtkPageSetup *page_setup = NULL;
  
  if (rdata->loop)
    g_main_loop_quit (rdata->loop);

  if (printer != NULL) 
    {
      rdata->result = BTK_PRINT_OPERATION_RESULT_APPLY;

      rdata->do_print = TRUE;

      if (priv->print_settings)
	settings = btk_print_settings_copy (priv->print_settings);
      else
	settings = btk_print_settings_new ();

      btk_print_settings_set_printer (settings,
				      btk_printer_get_name (printer));
      
      if (priv->default_page_setup)
	page_setup = btk_page_setup_copy (priv->default_page_setup);
      else
	page_setup = btk_page_setup_new ();
  }
  
  finish_print (rdata, printer, page_setup, settings, FALSE);

  if (settings)
    g_object_unref (settings);
  
  if (page_setup)
    g_object_unref (page_setup);
}

void
_btk_print_operation_platform_backend_run_dialog_async (BtkPrintOperation          *op,
							bboolean                    show_dialog,
                                                        BtkWindow                  *parent,
							BtkPrintOperationPrintFunc  print_cb)
{
  BtkWidget *pd;
  PrintResponseData *rdata;
  const bchar *printer_name;

  rdata = g_new (PrintResponseData, 1);
  rdata->op = g_object_ref (op);
  rdata->do_print = FALSE;
  rdata->do_preview = FALSE;
  rdata->result = BTK_PRINT_OPERATION_RESULT_CANCEL;
  rdata->print_cb = print_cb;
  rdata->parent = parent;
  rdata->loop = NULL;
  rdata->destroy = print_response_data_free;
  
  if (show_dialog)
    {
      pd = get_print_dialog (op, parent);
      btk_window_set_modal (BTK_WINDOW (pd), TRUE);

      g_signal_connect (pd, "response", 
			G_CALLBACK (handle_print_response), rdata);
      
      btk_window_present (BTK_WINDOW (pd));
    }
  else
    {
      printer_name = NULL;
      if (op->priv->print_settings)
	printer_name = btk_print_settings_get_printer (op->priv->print_settings);
      
      find_printer (printer_name, (GFunc) found_printer, rdata);
    }
}

static bairo_status_t
write_preview (void                *closure,
               const unsigned char *data,
               unsigned int         length)
{
  bint fd = BPOINTER_TO_INT (closure);
  bssize written;
  
  while (length > 0) 
    {
      written = write (fd, data, length);

      if (written == -1)
	{
	  if (errno == EAGAIN || errno == EINTR)
	    continue;
	  
	  return BAIRO_STATUS_WRITE_ERROR;
	}    

      data += written;
      length -= written;
    }

  return BAIRO_STATUS_SUCCESS;
}

static void
close_preview (void *data)
{
  bint fd = BPOINTER_TO_INT (data);

  close (fd);
}

bairo_surface_t *
_btk_print_operation_platform_backend_create_preview_surface (BtkPrintOperation *op,
							      BtkPageSetup      *page_setup,
							      bdouble           *dpi_x,
							      bdouble           *dpi_y,
							      bchar            **target)
{
  bchar *filename;
  bint fd;
  BtkPaperSize *paper_size;
  bdouble w, h;
  bairo_surface_t *surface;
  static bairo_user_data_key_t key;
  
  filename = g_build_filename (g_get_tmp_dir (), "previewXXXXXX.pdf", NULL);
  fd = g_mkstemp (filename);

  if (fd < 0)
    {
      g_free (filename);
      return NULL;
    }

  *target = filename;
  
  paper_size = btk_page_setup_get_paper_size (page_setup);
  w = btk_paper_size_get_width (paper_size, BTK_UNIT_POINTS);
  h = btk_paper_size_get_height (paper_size, BTK_UNIT_POINTS);
    
  *dpi_x = *dpi_y = 72;
  surface = bairo_pdf_surface_create_for_stream (write_preview, BINT_TO_POINTER (fd), w, h);
 
  bairo_surface_set_user_data (surface, &key, BINT_TO_POINTER (fd), close_preview);

  return surface;
}

void
_btk_print_operation_platform_backend_preview_start_page (BtkPrintOperation *op,
							  bairo_surface_t   *surface,
							  bairo_t           *cr)
{
}

void
_btk_print_operation_platform_backend_preview_end_page (BtkPrintOperation *op,
							bairo_surface_t   *surface,
							bairo_t           *cr)
{
  bairo_show_page (cr);
}

void
_btk_print_operation_platform_backend_resize_preview_surface (BtkPrintOperation *op,
							      BtkPageSetup      *page_setup,
							      bairo_surface_t   *surface)
{
  BtkPaperSize *paper_size;
  bdouble w, h;
  
  paper_size = btk_page_setup_get_paper_size (page_setup);
  w = btk_paper_size_get_width (paper_size, BTK_UNIT_POINTS);
  h = btk_paper_size_get_height (paper_size, BTK_UNIT_POINTS);
  bairo_pdf_surface_set_size (surface, w, h);
}


BtkPrintOperationResult
_btk_print_operation_platform_backend_run_dialog (BtkPrintOperation *op,
						  bboolean           show_dialog,
						  BtkWindow         *parent,
						  bboolean          *do_print)
 {
  BtkWidget *pd;
  PrintResponseData rdata;
  bint response;  
  const bchar *printer_name;
   
  rdata.op = op;
  rdata.do_print = FALSE;
  rdata.do_preview = FALSE;
  rdata.result = BTK_PRINT_OPERATION_RESULT_CANCEL;
  rdata.print_cb = NULL;
  rdata.destroy = NULL;
  rdata.parent = parent;
  rdata.loop = NULL;

  if (show_dialog)
    {
      pd = get_print_dialog (op, parent);

      response = btk_dialog_run (BTK_DIALOG (pd));
      handle_print_response (pd, response, &rdata);
    }
  else
    {
      printer_name = NULL;
      if (op->priv->print_settings)
	printer_name = btk_print_settings_get_printer (op->priv->print_settings);
      
      rdata.loop = g_main_loop_new (NULL, FALSE);
      find_printer (printer_name,
		    (GFunc) found_printer, &rdata);

      BDK_THREADS_LEAVE ();  
      g_main_loop_run (rdata.loop);
      BDK_THREADS_ENTER ();  

      g_main_loop_unref (rdata.loop);
      rdata.loop = NULL;
    }

  *do_print = rdata.do_print;
  
  return rdata.result;
}


typedef struct 
{
  BtkPageSetup         *page_setup;
  BtkPageSetupDoneFunc  done_cb;
  bpointer              data;
  GDestroyNotify        destroy;
} PageSetupResponseData;

static void
page_setup_data_free (bpointer data)
{
  PageSetupResponseData *rdata = data;

  if (rdata->page_setup)
    g_object_unref (rdata->page_setup);

  g_free (rdata);
}

static void
handle_page_setup_response (BtkWidget *dialog,
			    bint       response,
			    bpointer   data)
{
  BtkPageSetupUnixDialog *psd;
  PageSetupResponseData *rdata = data;

  psd = BTK_PAGE_SETUP_UNIX_DIALOG (dialog);
  if (response == BTK_RESPONSE_OK)
    rdata->page_setup = btk_page_setup_unix_dialog_get_page_setup (psd);

  btk_widget_destroy (dialog);

  if (rdata->done_cb)
    rdata->done_cb (rdata->page_setup, rdata->data);

  if (rdata->destroy)
    rdata->destroy (rdata);
}

static BtkWidget *
get_page_setup_dialog (BtkWindow        *parent,
		       BtkPageSetup     *page_setup,
		       BtkPrintSettings *settings)
{
  BtkWidget *dialog;

  dialog = btk_page_setup_unix_dialog_new (NULL, parent);
  if (page_setup)
    btk_page_setup_unix_dialog_set_page_setup (BTK_PAGE_SETUP_UNIX_DIALOG (dialog),
					       page_setup);
  btk_page_setup_unix_dialog_set_print_settings (BTK_PAGE_SETUP_UNIX_DIALOG (dialog),
						 settings);

  return dialog;
}

/**
 * btk_print_run_page_setup_dialog:
 * @parent: (allow-none): transient parent
 * @page_setup: (allow-none): an existing #BtkPageSetup
 * @settings: a #BtkPrintSettings
 *
 * Runs a page setup dialog, letting the user modify the values from
 * @page_setup. If the user cancels the dialog, the returned #BtkPageSetup
 * is identical to the passed in @page_setup, otherwise it contains the 
 * modifications done in the dialog.
 *
 * Note that this function may use a recursive mainloop to show the page
 * setup dialog. See btk_print_run_page_setup_dialog_async() if this is 
 * a problem.
 * 
 * Return value: (transfer full): a new #BtkPageSetup
 *
 * Since: 2.10
 */
BtkPageSetup *
btk_print_run_page_setup_dialog (BtkWindow        *parent,
				 BtkPageSetup     *page_setup,
				 BtkPrintSettings *settings)
{
  BtkWidget *dialog;
  bint response;
  PageSetupResponseData rdata;  
  
  rdata.page_setup = NULL;
  rdata.done_cb = NULL;
  rdata.data = NULL;
  rdata.destroy = NULL;

  dialog = get_page_setup_dialog (parent, page_setup, settings);
  response = btk_dialog_run (BTK_DIALOG (dialog));
  handle_page_setup_response (dialog, response, &rdata);
 
  if (rdata.page_setup)
    return rdata.page_setup;
  else if (page_setup)
    return btk_page_setup_copy (page_setup);
  else
    return btk_page_setup_new ();
}

/**
 * btk_print_run_page_setup_dialog_async:
 * @parent: (allow-none): transient parent, or %NULL
 * @page_setup: (allow-none): an existing #BtkPageSetup, or %NULL
 * @settings: a #BtkPrintSettings
 * @done_cb: a function to call when the user saves the modified page setup
 * @data: user data to pass to @done_cb
 * 
 * Runs a page setup dialog, letting the user modify the values from @page_setup. 
 *
 * In contrast to btk_print_run_page_setup_dialog(), this function  returns after 
 * showing the page setup dialog on platforms that support this, and calls @done_cb 
 * from a signal handler for the ::response signal of the dialog.
 *
 * Since: 2.10
 */
void
btk_print_run_page_setup_dialog_async (BtkWindow            *parent,
				       BtkPageSetup         *page_setup,
				       BtkPrintSettings     *settings,
				       BtkPageSetupDoneFunc  done_cb,
				       bpointer              data)
{
  BtkWidget *dialog;
  PageSetupResponseData *rdata;
  
  dialog = get_page_setup_dialog (parent, page_setup, settings);
  btk_window_set_modal (BTK_WINDOW (dialog), TRUE);
  
  rdata = g_new (PageSetupResponseData, 1);
  rdata->page_setup = NULL;
  rdata->done_cb = done_cb;
  rdata->data = data;
  rdata->destroy = page_setup_data_free;

  g_signal_connect (dialog, "response",
		    G_CALLBACK (handle_page_setup_response), rdata);
 
  btk_window_present (BTK_WINDOW (dialog));
 }

struct _PrinterFinder 
{
  bboolean found_printer;
  GFunc func;
  bpointer data;
  bchar *printer_name;
  GList *backends;
  buint timeout_tag;
  BtkPrinter *printer;
  BtkPrinter *default_printer;
  BtkPrinter *first_printer;
};

static bboolean
find_printer_idle (bpointer data)
{
  PrinterFinder *finder = data;
  BtkPrinter *printer;

  if (finder->printer != NULL)
    printer = finder->printer;
  else if (finder->default_printer != NULL)
    printer = finder->default_printer;
  else if (finder->first_printer != NULL)
    printer = finder->first_printer;
  else
    printer = NULL;

  finder->func (printer, finder->data);
  
  printer_finder_free (finder);

  return FALSE;
}

static void
printer_added_cb (BtkPrintBackend *backend, 
                  BtkPrinter      *printer, 
		  PrinterFinder   *finder)
{
  if (finder->found_printer)
    return;

  /* FIXME this skips "Print to PDF" - is this intentional ? */
  if (btk_printer_is_virtual (printer))
    return;

  if (finder->printer_name != NULL &&
      strcmp (btk_printer_get_name (printer), finder->printer_name) == 0)
    {
      finder->printer = g_object_ref (printer);
      finder->found_printer = TRUE;
    }
  else if (finder->default_printer == NULL &&
	   btk_printer_is_default (printer))
    {
      finder->default_printer = g_object_ref (printer);
      if (finder->printer_name == NULL)
	finder->found_printer = TRUE;
    }
  else
    if (finder->first_printer == NULL)
      finder->first_printer = g_object_ref (printer);
  
  if (finder->found_printer)
    g_idle_add (find_printer_idle, finder);
}

static void
printer_list_done_cb (BtkPrintBackend *backend, 
		      PrinterFinder   *finder)
{
  finder->backends = g_list_remove (finder->backends, backend);
  
  g_signal_handlers_disconnect_by_func (backend, printer_added_cb, finder);
  g_signal_handlers_disconnect_by_func (backend, printer_list_done_cb, finder);
  
  btk_print_backend_destroy (backend);
  g_object_unref (backend);

  if (finder->backends == NULL && !finder->found_printer)
    g_idle_add (find_printer_idle, finder);
}

static void
find_printer_init (PrinterFinder   *finder,
		   BtkPrintBackend *backend)
{
  GList *list;
  GList *node;

  list = btk_print_backend_get_printer_list (backend);

  node = list;
  while (node != NULL)
    {
      printer_added_cb (backend, node->data, finder);
      node = node->next;

      if (finder->found_printer)
	break;
    }

  g_list_free (list);

  if (btk_print_backend_printer_list_is_done (backend))
    {
      finder->backends = g_list_remove (finder->backends, backend);
      btk_print_backend_destroy (backend);
      g_object_unref (backend);
    }
  else
    {
      g_signal_connect (backend, "printer-added", 
			(GCallback) printer_added_cb, 
			finder);
      g_signal_connect (backend, "printer-list-done", 
			(GCallback) printer_list_done_cb, 
			finder);
    }

}

static void
printer_finder_free (PrinterFinder *finder)
{
  GList *l;
  
  g_free (finder->printer_name);
  
  if (finder->printer)
    g_object_unref (finder->printer);
  
  if (finder->default_printer)
    g_object_unref (finder->default_printer);
  
  if (finder->first_printer)
    g_object_unref (finder->first_printer);

  for (l = finder->backends; l != NULL; l = l->next)
    {
      BtkPrintBackend *backend = l->data;
      g_signal_handlers_disconnect_by_func (backend, printer_added_cb, finder);
      g_signal_handlers_disconnect_by_func (backend, printer_list_done_cb, finder);
      btk_print_backend_destroy (backend);
      g_object_unref (backend);
    }
  
  g_list_free (finder->backends);
  
  g_free (finder);
}

static void 
find_printer (const bchar *printer,
	      GFunc        func,
	      bpointer     data)
{
  GList *node, *next;
  PrinterFinder *finder;

  finder = g_new0 (PrinterFinder, 1);

  finder->printer_name = g_strdup (printer);
  finder->func = func;
  finder->data = data;
  
  finder->backends = NULL;
  if (g_module_supported ())
    finder->backends = btk_print_backend_load_modules ();

  for (node = finder->backends; !finder->found_printer && node != NULL; node = next)
    {
      next = node->next;
      find_printer_init (finder, BTK_PRINT_BACKEND (node->data));
    }

  if (finder->backends == NULL && !finder->found_printer)
    g_idle_add (find_printer_idle, finder);
}

#define __BTK_PRINT_OPERATION_UNIX_C__
#include "btkaliasdef.c"
