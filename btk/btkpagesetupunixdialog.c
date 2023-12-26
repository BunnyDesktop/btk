/* BtkPageSetupUnixDialog 
 * Copyright (C) 2006 Alexander Larsson <alexl@redhat.com>
 * Copyright © 2006, 2007, 2008 Christian Persch
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


#include "config.h"
#include <string.h>
#include <locale.h>

#include "btkintl.h"
#include "btkprivate.h"

#include "btkliststore.h"
#include "btkstock.h"
#include "btktreeviewcolumn.h"
#include "btktreeselection.h"
#include "btktreemodel.h"
#include "btkbutton.h"
#include "btkscrolledwindow.h"
#include "btkvbox.h"
#include "btkhbox.h"
#include "btkframe.h"
#include "btkeventbox.h"
#include "btkcombobox.h"
#include "btktogglebutton.h"
#include "btkradiobutton.h"
#include "btklabel.h"
#include "btktable.h"
#include "btkcelllayout.h"
#include "btkcellrenderertext.h"
#include "btkalignment.h"
#include "btkspinbutton.h"
#include "btkbbox.h"
#include "btkhbbox.h"

#include "btkpagesetupunixdialog.h"
#include "btkcustompaperunixdialog.h"
#include "btkprintbackend.h"
#include "btkpapersize.h"
#include "btkprintutils.h"
#include "btkalias.h"


struct BtkPageSetupUnixDialogPrivate
{
  BtkListStore *printer_list;
  BtkListStore *page_setup_list;
  BtkListStore *custom_paper_list;
  
  GList *print_backends;

  BtkWidget *printer_combo;
  BtkWidget *paper_size_combo;
  BtkWidget *paper_size_label;

  BtkWidget *portrait_radio;
  BtkWidget *reverse_portrait_radio;
  BtkWidget *landscape_radio;
  BtkWidget *reverse_landscape_radio;

  buint request_details_tag;
  BtkPrinter *request_details_printer;
  
  BtkPrintSettings *print_settings;

  /* Save last setup so we can re-set it after selecting manage custom sizes */
  BtkPageSetup *last_setup;

  bchar *waiting_for_printer;
};

enum {
  PRINTER_LIST_COL_NAME,
  PRINTER_LIST_COL_PRINTER,
  PRINTER_LIST_N_COLS
};

enum {
  PAGE_SETUP_LIST_COL_PAGE_SETUP,
  PAGE_SETUP_LIST_COL_IS_SEPARATOR,
  PAGE_SETUP_LIST_N_COLS
};

G_DEFINE_TYPE (BtkPageSetupUnixDialog, btk_page_setup_unix_dialog, BTK_TYPE_DIALOG)

#define BTK_PAGE_SETUP_UNIX_DIALOG_GET_PRIVATE(o)  \
   (B_TYPE_INSTANCE_GET_PRIVATE ((o), BTK_TYPE_PAGE_SETUP_UNIX_DIALOG, BtkPageSetupUnixDialogPrivate))

static void btk_page_setup_unix_dialog_finalize  (BObject                *object);
static void populate_dialog                      (BtkPageSetupUnixDialog *dialog);
static void fill_paper_sizes_from_printer        (BtkPageSetupUnixDialog *dialog,
						  BtkPrinter             *printer);
static void printer_added_cb                     (BtkPrintBackend        *backend,
						  BtkPrinter             *printer,
						  BtkPageSetupUnixDialog *dialog);
static void printer_removed_cb                   (BtkPrintBackend        *backend,
						  BtkPrinter             *printer,
						  BtkPageSetupUnixDialog *dialog);
static void printer_status_cb                    (BtkPrintBackend        *backend,
						  BtkPrinter             *printer,
						  BtkPageSetupUnixDialog *dialog);



static const bchar const common_paper_sizes[][16] = {
  "na_letter",
  "na_legal",
  "iso_a4",
  "iso_a5",
  "roc_16k",
  "iso_b5",
  "jis_b5",
  "na_number-10",
  "iso_dl",
  "jpn_chou3",
  "na_ledger",
  "iso_a3",
};


static void
btk_page_setup_unix_dialog_class_init (BtkPageSetupUnixDialogClass *class)
{
  BObjectClass *object_class;
  BtkWidgetClass *widget_class;

  object_class = (BObjectClass *) class;
  widget_class = (BtkWidgetClass *) class;

  object_class->finalize = btk_page_setup_unix_dialog_finalize;

  g_type_class_add_private (class, sizeof (BtkPageSetupUnixDialogPrivate));
}

static void
btk_page_setup_unix_dialog_init (BtkPageSetupUnixDialog *dialog)
{
  BtkPageSetupUnixDialogPrivate *priv;
  BtkTreeIter iter;
  bchar *tmp;

  priv = dialog->priv = BTK_PAGE_SETUP_UNIX_DIALOG_GET_PRIVATE (dialog);

  priv->print_backends = NULL;

  priv->printer_list = btk_list_store_new (PRINTER_LIST_N_COLS,
						   B_TYPE_STRING,
						   B_TYPE_OBJECT);

  btk_list_store_append (priv->printer_list, &iter);
  tmp = g_strdup_printf ("<b>%s</b>\n%s", _("Any Printer"), _("For portable documents"));
  btk_list_store_set (priv->printer_list, &iter,
                      PRINTER_LIST_COL_NAME, tmp,
                      PRINTER_LIST_COL_PRINTER, NULL,
                      -1);
  g_free (tmp);

  priv->page_setup_list = btk_list_store_new (PAGE_SETUP_LIST_N_COLS,
						      B_TYPE_OBJECT,
						      B_TYPE_BOOLEAN);

  priv->custom_paper_list = btk_list_store_new (1, B_TYPE_OBJECT);
  _btk_print_load_custom_papers (priv->custom_paper_list);

  populate_dialog (dialog);
  
  btk_dialog_add_buttons (BTK_DIALOG (dialog), 
                          BTK_STOCK_CANCEL, BTK_RESPONSE_CANCEL,
                          BTK_STOCK_APPLY, BTK_RESPONSE_OK,
                          NULL);
  btk_dialog_set_alternative_button_order (BTK_DIALOG (dialog),
					   BTK_RESPONSE_OK,
					   BTK_RESPONSE_CANCEL,
					   -1);

  btk_dialog_set_default_response (BTK_DIALOG (dialog), BTK_RESPONSE_OK);
}

static void
btk_page_setup_unix_dialog_finalize (BObject *object)
{
  BtkPageSetupUnixDialog *dialog = BTK_PAGE_SETUP_UNIX_DIALOG (object);
  BtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  BtkPrintBackend *backend;
  GList *node;
  
  if (priv->request_details_tag)
    {
      g_signal_handler_disconnect (priv->request_details_printer,
				   priv->request_details_tag);
      g_object_unref (priv->request_details_printer);
      priv->request_details_printer = NULL;
      priv->request_details_tag = 0;
    }
  
  if (priv->printer_list)
    {
      g_object_unref (priv->printer_list);
      priv->printer_list = NULL;
    }

  if (priv->page_setup_list)
    {
      g_object_unref (priv->page_setup_list);
      priv->page_setup_list = NULL;
    }

  if (priv->custom_paper_list)
    {
      g_object_unref (priv->custom_paper_list);
      priv->custom_paper_list = NULL;
    }

  if (priv->print_settings)
    {
      g_object_unref (priv->print_settings);
      priv->print_settings = NULL;
    }

  g_free (priv->waiting_for_printer);
  priv->waiting_for_printer = NULL;

  for (node = priv->print_backends; node != NULL; node = node->next)
    {
      backend = BTK_PRINT_BACKEND (node->data);

      g_signal_handlers_disconnect_by_func (backend, printer_added_cb, dialog);
      g_signal_handlers_disconnect_by_func (backend, printer_removed_cb, dialog);
      g_signal_handlers_disconnect_by_func (backend, printer_status_cb, dialog);

      btk_print_backend_destroy (backend);
      g_object_unref (backend);
    }
  
  g_list_free (priv->print_backends);
  priv->print_backends = NULL;

  B_OBJECT_CLASS (btk_page_setup_unix_dialog_parent_class)->finalize (object);
}

static void
printer_added_cb (BtkPrintBackend        *backend, 
		  BtkPrinter             *printer, 
		  BtkPageSetupUnixDialog *dialog)
{
  BtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  BtkTreeIter iter;
  bchar *str;
  const bchar *location;

  if (btk_printer_is_virtual (printer))
    return;

  location = btk_printer_get_location (printer);
  if (location == NULL)
    location = "";
  str = g_strdup_printf ("<b>%s</b>\n%s",
			 btk_printer_get_name (printer),
			 location);

  btk_list_store_append (priv->printer_list, &iter);
  btk_list_store_set (priv->printer_list, &iter,
                      PRINTER_LIST_COL_NAME, str,
                      PRINTER_LIST_COL_PRINTER, printer,
                      -1);

  g_object_set_data_full (B_OBJECT (printer),
			  "btk-print-tree-iter",
                          btk_tree_iter_copy (&iter),
                          (GDestroyNotify) btk_tree_iter_free);
  g_free (str);

  if (priv->waiting_for_printer != NULL &&
      strcmp (priv->waiting_for_printer,
	      btk_printer_get_name (printer)) == 0)
    {
      btk_combo_box_set_active_iter (BTK_COMBO_BOX (priv->printer_combo),
				     &iter);
      priv->waiting_for_printer = NULL;
    }
}

static void
printer_removed_cb (BtkPrintBackend        *backend, 
		    BtkPrinter             *printer, 
		    BtkPageSetupUnixDialog *dialog)
{
  BtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  BtkTreeIter *iter;

  iter = g_object_get_data (B_OBJECT (printer), "btk-print-tree-iter");
  btk_list_store_remove (BTK_LIST_STORE (priv->printer_list), iter);
}


static void
printer_status_cb (BtkPrintBackend        *backend, 
                   BtkPrinter             *printer, 
		   BtkPageSetupUnixDialog *dialog)
{
  BtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  BtkTreeIter *iter;
  bchar *str;
  const bchar *location;
  
  iter = g_object_get_data (B_OBJECT (printer), "btk-print-tree-iter");

  location = btk_printer_get_location (printer);
  if (location == NULL)
    location = "";
  str = g_strdup_printf ("<b>%s</b>\n%s",
			 btk_printer_get_name (printer),
			 location);
  btk_list_store_set (priv->printer_list, iter,
                      PRINTER_LIST_COL_NAME, str,
                      -1);
  g_free (str);
}

static void
printer_list_initialize (BtkPageSetupUnixDialog *dialog,
			 BtkPrintBackend        *print_backend)
{
  GList *list, *node;
  
  g_return_if_fail (print_backend != NULL);

  g_signal_connect_object (print_backend, 
			   "printer-added", 
			   (GCallback) printer_added_cb, 
			   B_OBJECT (dialog), 0);

  g_signal_connect_object (print_backend, 
			   "printer-removed", 
			   (GCallback) printer_removed_cb, 
			   B_OBJECT (dialog), 0);

  g_signal_connect_object (print_backend, 
			   "printer-status-changed", 
			   (GCallback) printer_status_cb, 
			   B_OBJECT (dialog), 0);

  list = btk_print_backend_get_printer_list (print_backend);

  node = list;
  while (node != NULL)
    {
      printer_added_cb (print_backend, node->data, dialog);
      node = node->next;
    }

  g_list_free (list);
  
}

static void
load_print_backends (BtkPageSetupUnixDialog *dialog)
{
  BtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  GList *node;

  if (g_module_supported ())
    priv->print_backends = btk_print_backend_load_modules ();

  for (node = priv->print_backends; node != NULL; node = node->next)
    printer_list_initialize (dialog, BTK_PRINT_BACKEND (node->data));
}

static bboolean
paper_size_row_is_separator (BtkTreeModel *model,
			     BtkTreeIter  *iter,
			     bpointer      data)
{
  bboolean separator;

  btk_tree_model_get (model, iter, PAGE_SETUP_LIST_COL_IS_SEPARATOR, &separator, -1);
  return separator;
}

static BtkPageSetup *
get_current_page_setup (BtkPageSetupUnixDialog *dialog)
{
  BtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  BtkPageSetup *current_page_setup;
  BtkComboBox *combo_box;
  BtkTreeIter iter;

  current_page_setup = NULL;
  
  combo_box = BTK_COMBO_BOX (priv->paper_size_combo);
  if (btk_combo_box_get_active_iter (combo_box, &iter))
    btk_tree_model_get (BTK_TREE_MODEL (priv->page_setup_list), &iter,
			PAGE_SETUP_LIST_COL_PAGE_SETUP, &current_page_setup, -1);

  if (current_page_setup)
    return current_page_setup;

  /* No selected page size, return the default one.
   * This is used to set the first page setup when the dialog is created
   * as there is no selection on the first printer_changed.
   */ 
  return btk_page_setup_new ();
}

static bboolean
page_setup_is_equal (BtkPageSetup *a, 
		     BtkPageSetup *b)
{
  return
    btk_paper_size_is_equal (btk_page_setup_get_paper_size (a),
			     btk_page_setup_get_paper_size (b)) &&
    btk_page_setup_get_top_margin (a, BTK_UNIT_MM) == btk_page_setup_get_top_margin (b, BTK_UNIT_MM) &&
    btk_page_setup_get_bottom_margin (a, BTK_UNIT_MM) == btk_page_setup_get_bottom_margin (b, BTK_UNIT_MM) &&
    btk_page_setup_get_left_margin (a, BTK_UNIT_MM) == btk_page_setup_get_left_margin (b, BTK_UNIT_MM) &&
    btk_page_setup_get_right_margin (a, BTK_UNIT_MM) == btk_page_setup_get_right_margin (b, BTK_UNIT_MM);
}

static bboolean
page_setup_is_same_size (BtkPageSetup *a,
			 BtkPageSetup *b)
{
  return btk_paper_size_is_equal (btk_page_setup_get_paper_size (a),
				  btk_page_setup_get_paper_size (b));
}

static bboolean
set_paper_size (BtkPageSetupUnixDialog *dialog,
		BtkPageSetup           *page_setup,
		bboolean                size_only,
		bboolean                add_item)
{
  BtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  BtkTreeModel *model;
  BtkTreeIter iter;
  BtkPageSetup *list_page_setup;

  model = BTK_TREE_MODEL (priv->page_setup_list);

  if (btk_tree_model_get_iter_first (model, &iter))
    {
      do
	{
	  btk_tree_model_get (BTK_TREE_MODEL (priv->page_setup_list), &iter,
			      PAGE_SETUP_LIST_COL_PAGE_SETUP, &list_page_setup, -1);
	  if (list_page_setup == NULL)
	    continue;
	  
	  if ((size_only && page_setup_is_same_size (page_setup, list_page_setup)) ||
	      (!size_only && page_setup_is_equal (page_setup, list_page_setup)))
	    {
	      btk_combo_box_set_active_iter (BTK_COMBO_BOX (priv->paper_size_combo),
					     &iter);
	      g_object_unref (list_page_setup);
	      return TRUE;
	    }
	      
	  g_object_unref (list_page_setup);
	  
	} while (btk_tree_model_iter_next (model, &iter));
    }

  if (add_item)
    {
      btk_list_store_append (priv->page_setup_list, &iter);
      btk_list_store_set (priv->page_setup_list, &iter,
			  PAGE_SETUP_LIST_COL_IS_SEPARATOR, TRUE,
			  -1);
      btk_list_store_append (priv->page_setup_list, &iter);
      btk_list_store_set (priv->page_setup_list, &iter,
			  PAGE_SETUP_LIST_COL_PAGE_SETUP, page_setup,
			  -1);
      btk_combo_box_set_active_iter (BTK_COMBO_BOX (priv->paper_size_combo),
				     &iter);
      return TRUE;
    }

  return FALSE;
}

static void
fill_custom_paper_sizes (BtkPageSetupUnixDialog *dialog)
{
  BtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  BtkTreeIter iter, paper_iter;
  BtkTreeModel *model;

  model = BTK_TREE_MODEL (priv->custom_paper_list);
  if (btk_tree_model_get_iter_first (model, &iter))
    {
      btk_list_store_append (priv->page_setup_list, &paper_iter);
      btk_list_store_set (priv->page_setup_list, &paper_iter,
			  PAGE_SETUP_LIST_COL_IS_SEPARATOR, TRUE,
			  -1);
      do
	{
	  BtkPageSetup *page_setup;
	  btk_tree_model_get (model, &iter, 0, &page_setup, -1);

	  btk_list_store_append (priv->page_setup_list, &paper_iter);
	  btk_list_store_set (priv->page_setup_list, &paper_iter,
			      PAGE_SETUP_LIST_COL_PAGE_SETUP, page_setup,
			      -1);

	  g_object_unref (page_setup);
	} while (btk_tree_model_iter_next (model, &iter));
    }
  
  btk_list_store_append (priv->page_setup_list, &paper_iter);
  btk_list_store_set (priv->page_setup_list, &paper_iter,
                      PAGE_SETUP_LIST_COL_IS_SEPARATOR, TRUE,
                      -1);
  btk_list_store_append (priv->page_setup_list, &paper_iter);
  btk_list_store_set (priv->page_setup_list, &paper_iter,
                      PAGE_SETUP_LIST_COL_PAGE_SETUP, NULL,
                      -1);
}

static void
fill_paper_sizes_from_printer (BtkPageSetupUnixDialog *dialog,
			       BtkPrinter             *printer)
{
  BtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  GList *list, *l;
  BtkPageSetup *current_page_setup, *page_setup;
  BtkPaperSize *paper_size;
  BtkTreeIter iter;
  bint i;

  btk_list_store_clear (priv->page_setup_list);

  if (printer == NULL)
    {
      for (i = 0; i < G_N_ELEMENTS (common_paper_sizes); i++)
	{
	  page_setup = btk_page_setup_new ();
	  paper_size = btk_paper_size_new (common_paper_sizes[i]);
	  btk_page_setup_set_paper_size_and_default_margins (page_setup, paper_size);
	  btk_paper_size_free (paper_size);
	  
	  btk_list_store_append (priv->page_setup_list, &iter);
	  btk_list_store_set (priv->page_setup_list, &iter,
			      PAGE_SETUP_LIST_COL_PAGE_SETUP, page_setup,
			      -1);
	  g_object_unref (page_setup);
	}
    }
  else
    {
      list = btk_printer_list_papers (printer);
      /* TODO: We should really sort this list so interesting size
	 are at the top */
      for (l = list; l != NULL; l = l->next)
	{
	  page_setup = l->data;
	  btk_list_store_append (priv->page_setup_list, &iter);
	  btk_list_store_set (priv->page_setup_list, &iter,
			      PAGE_SETUP_LIST_COL_PAGE_SETUP, page_setup,
			      -1);
	  g_object_unref (page_setup);
	}
      g_list_free (list);
    }

  fill_custom_paper_sizes (dialog);
  
  current_page_setup = NULL;

  /* When selecting a different printer, select its default paper size */
  if (printer != NULL)
    current_page_setup = btk_printer_get_default_page_size (printer);

  if (current_page_setup == NULL)
    current_page_setup = get_current_page_setup (dialog);

  if (!set_paper_size (dialog, current_page_setup, FALSE, FALSE))
    set_paper_size (dialog, current_page_setup, TRUE, TRUE);
  
  if (current_page_setup)
    g_object_unref (current_page_setup);
}

static void
printer_changed_finished_callback (BtkPrinter             *printer,
				   bboolean                success,
				   BtkPageSetupUnixDialog *dialog)
{
  BtkPageSetupUnixDialogPrivate *priv = dialog->priv;

  g_signal_handler_disconnect (priv->request_details_printer,
			       priv->request_details_tag);
  g_object_unref (priv->request_details_printer);
  priv->request_details_tag = 0;
  priv->request_details_printer = NULL;
  
  if (success)
    fill_paper_sizes_from_printer (dialog, printer);

}

static void
printer_changed_callback (BtkComboBox            *combo_box,
			  BtkPageSetupUnixDialog *dialog)
{
  BtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  BtkPrinter *printer;
  BtkTreeIter iter;

  /* If we're waiting for a specific printer but the user changed
   * to another printer, cancel that wait. 
   */
  if (priv->waiting_for_printer)
    {
      g_free (priv->waiting_for_printer);
      priv->waiting_for_printer = NULL;
    }
  
  if (priv->request_details_tag)
    {
      g_signal_handler_disconnect (priv->request_details_printer,
				   priv->request_details_tag);
      g_object_unref (priv->request_details_printer);
      priv->request_details_printer = NULL;
      priv->request_details_tag = 0;
    }
  
  if (btk_combo_box_get_active_iter (combo_box, &iter))
    {
      btk_tree_model_get (btk_combo_box_get_model (combo_box), &iter,
			  PRINTER_LIST_COL_PRINTER, &printer, -1);

      if (printer == NULL || btk_printer_has_details (printer))
	fill_paper_sizes_from_printer (dialog, printer);
      else
	{
	  priv->request_details_printer = g_object_ref (printer);
	  priv->request_details_tag =
	    g_signal_connect (printer, "details-acquired",
			      G_CALLBACK (printer_changed_finished_callback), dialog);
	  btk_printer_request_details (printer);

	}

      if (printer)
	g_object_unref (printer);

      if (priv->print_settings)
	{
	  const char *name = NULL;

	  if (printer)
	    name = btk_printer_get_name (printer);
	  
	  btk_print_settings_set (priv->print_settings,
				  "format-for-printer", name);
	}
    }
}

/* We do this munging because we don't want to show zero digits
   after the decimal point, and not to many such digits if they
   are nonzero. I wish printf let you specify max precision for %f... */
static bchar *
double_to_string (bdouble d, 
		  BtkUnit unit)
{
  bchar *val, *p;
  struct lconv *locale_data;
  const bchar *decimal_point;
  bint decimal_point_len;

  locale_data = localeconv ();
  decimal_point = locale_data->decimal_point;
  decimal_point_len = strlen (decimal_point);
  
  /* Max two decimal digits for inch, max one for mm */
  if (unit == BTK_UNIT_INCH)
    val = g_strdup_printf ("%.2f", d);
  else
    val = g_strdup_printf ("%.1f", d);

  if (strstr (val, decimal_point))
    {
      p = val + strlen (val) - 1;
      while (*p == '0')
        p--;
      if (p - val + 1 >= decimal_point_len &&
	  strncmp (p - (decimal_point_len - 1), decimal_point, decimal_point_len) == 0)
        p -= decimal_point_len;
      p[1] = '\0';
    }

  return val;
}


static void
custom_paper_dialog_response_cb (BtkDialog *custom_paper_dialog,
				 bint       response_id,
				 bpointer   user_data)
{
  BtkPageSetupUnixDialog *page_setup_dialog = BTK_PAGE_SETUP_UNIX_DIALOG (user_data);
  BtkPageSetupUnixDialogPrivate *priv = page_setup_dialog->priv;

  _btk_print_load_custom_papers (priv->custom_paper_list);

  /* Update printer page list */
  printer_changed_callback (BTK_COMBO_BOX (priv->printer_combo), page_setup_dialog);

  btk_widget_destroy (BTK_WIDGET (custom_paper_dialog));
}

static void
paper_size_changed (BtkComboBox            *combo_box,
		    BtkPageSetupUnixDialog *dialog)
{
  BtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  BtkTreeIter iter;
  BtkPageSetup *page_setup, *last_page_setup;
  BtkUnit unit;
  bchar *str, *w, *h;
  bchar *top, *bottom, *left, *right;
  BtkLabel *label;
  const bchar *unit_str;

  label = BTK_LABEL (priv->paper_size_label);
   
  if (btk_combo_box_get_active_iter (combo_box, &iter))
    {
      btk_tree_model_get (btk_combo_box_get_model (combo_box),
			  &iter, PAGE_SETUP_LIST_COL_PAGE_SETUP, &page_setup, -1);

      if (page_setup == NULL)
	{
          BtkWidget *custom_paper_dialog;

          /* Change from "manage" menu item to last value */
          if (priv->last_setup)
            last_page_setup = g_object_ref (priv->last_setup);
          else
	    last_page_setup = btk_page_setup_new (); /* "good" default */
	  set_paper_size (dialog, last_page_setup, FALSE, TRUE);
          g_object_unref (last_page_setup);

          /* And show the custom paper dialog */
          custom_paper_dialog = _btk_custom_paper_unix_dialog_new (BTK_WINDOW (dialog), NULL);
          g_signal_connect (custom_paper_dialog, "response", G_CALLBACK (custom_paper_dialog_response_cb), dialog);
          btk_window_present (BTK_WINDOW (custom_paper_dialog));

          return;
	}

      if (priv->last_setup)
	g_object_unref (priv->last_setup);

      priv->last_setup = g_object_ref (page_setup);
      
      unit = _btk_print_get_default_user_units ();

      if (unit == BTK_UNIT_MM)
	unit_str = _("mm");
      else
	unit_str = _("inch");
	

      w = double_to_string (btk_page_setup_get_paper_width (page_setup, unit),
			    unit);
      h = double_to_string (btk_page_setup_get_paper_height (page_setup, unit),
			    unit);
      str = g_strdup_printf ("%s x %s %s", w, h, unit_str);
      g_free (w);
      g_free (h);
      
      btk_label_set_text (label, str);
      g_free (str);

      top = double_to_string (btk_page_setup_get_top_margin (page_setup, unit), unit);
      bottom = double_to_string (btk_page_setup_get_bottom_margin (page_setup, unit), unit);
      left = double_to_string (btk_page_setup_get_left_margin (page_setup, unit), unit);
      right = double_to_string (btk_page_setup_get_right_margin (page_setup, unit), unit);

      str = g_strdup_printf (_("Margins:\n"
			       " Left: %s %s\n"
			       " Right: %s %s\n"
			       " Top: %s %s\n"
			       " Bottom: %s %s"
			       ),
			     left, unit_str,
			     right, unit_str,
			     top, unit_str,
			     bottom, unit_str);
      g_free (top);
      g_free (bottom);
      g_free (left);
      g_free (right);
      
      btk_widget_set_tooltip_text (priv->paper_size_label, str);
      g_free (str);
      
      g_object_unref (page_setup);
    }
  else
    {
      btk_label_set_text (label, "");
      btk_widget_set_tooltip_text (priv->paper_size_label, NULL);
      if (priv->last_setup)
	g_object_unref (priv->last_setup);
      priv->last_setup = NULL;
    }
}

static void
page_name_func (BtkCellLayout   *cell_layout,
		BtkCellRenderer *cell,
		BtkTreeModel    *tree_model,
		BtkTreeIter     *iter,
		bpointer         data)
{
  BtkPageSetup *page_setup;
  BtkPaperSize *paper_size;
  
  btk_tree_model_get (tree_model, iter,
		      PAGE_SETUP_LIST_COL_PAGE_SETUP, &page_setup, -1);
  if (page_setup)
    {
      paper_size = btk_page_setup_get_paper_size (page_setup);
      g_object_set (cell, "text",  btk_paper_size_get_display_name (paper_size), NULL);
      g_object_unref (page_setup);
    }
  else
    g_object_set (cell, "text",  _("Manage Custom Sizes..."), NULL);
      
}

static BtkWidget *
create_radio_button (GSList      *group,
		     const bchar *stock_id)
{
  BtkWidget *radio_button, *image, *label, *hbox;
  BtkStockItem item;

  radio_button = btk_radio_button_new (group);
  image = btk_image_new_from_stock (stock_id, BTK_ICON_SIZE_LARGE_TOOLBAR);
  btk_stock_lookup (stock_id, &item);
  label = btk_label_new (item.label);
  hbox = btk_hbox_new (0, 6);
  btk_container_add (BTK_CONTAINER (radio_button), hbox);
  btk_container_add (BTK_CONTAINER (hbox), image);
  btk_container_add (BTK_CONTAINER (hbox), label);

  btk_widget_show_all (radio_button);

  return radio_button;
}

static void
populate_dialog (BtkPageSetupUnixDialog *ps_dialog)
{
  BtkPageSetupUnixDialogPrivate *priv = ps_dialog->priv;
  BtkDialog *dialog = BTK_DIALOG (ps_dialog);
  BtkWidget *table, *label, *combo, *radio_button;
  BtkCellRenderer *cell;

  btk_window_set_resizable (BTK_WINDOW (dialog), FALSE);

  btk_dialog_set_has_separator (dialog, FALSE);
  btk_container_set_border_width (BTK_CONTAINER (dialog), 5);
  btk_box_set_spacing (BTK_BOX (dialog->vbox), 2); /* 2 * 5 + 2 = 12 */
  btk_container_set_border_width (BTK_CONTAINER (dialog->action_area), 5);
  btk_box_set_spacing (BTK_BOX (dialog->action_area), 6);

  table = btk_table_new (5, 4, FALSE);
  btk_table_set_row_spacings (BTK_TABLE (table), 6);
  btk_table_set_col_spacings (BTK_TABLE (table), 12);
  btk_container_set_border_width (BTK_CONTAINER (table), 5);
  btk_box_pack_start (BTK_BOX (dialog->vbox), table, TRUE, TRUE, 0);
  btk_widget_show (table);

  label = btk_label_new_with_mnemonic (_("_Format for:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_table_attach (BTK_TABLE (table), label,
		    0, 1, 0, 1,
		    BTK_FILL, 0, 0, 0);
  btk_widget_show (label);

  combo = btk_combo_box_new_with_model (BTK_TREE_MODEL (priv->printer_list));
  priv->printer_combo = combo;

  cell = btk_cell_renderer_text_new ();
  btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combo), cell, TRUE);
  btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (combo), cell,
                                  "markup", PRINTER_LIST_COL_NAME,
                                  NULL);

  btk_table_attach (BTK_TABLE (table), combo,
		    1, 4, 0, 1,
		    BTK_FILL | BTK_EXPAND, 0, 0, 0);
  btk_widget_show (combo);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), combo);

  label = btk_label_new_with_mnemonic (_("_Paper size:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_table_attach (BTK_TABLE (table), label,
		    0, 1, 1, 2,
		    BTK_FILL, 0, 0, 0);
  btk_widget_show (label);

  combo = btk_combo_box_new_with_model (BTK_TREE_MODEL (priv->page_setup_list));
  priv->paper_size_combo = combo;
  btk_combo_box_set_row_separator_func (BTK_COMBO_BOX (combo), 
					paper_size_row_is_separator, NULL, NULL);
  
  cell = btk_cell_renderer_text_new ();
  btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combo), cell, TRUE);
  btk_cell_layout_set_cell_data_func (BTK_CELL_LAYOUT (combo), cell,
				      page_name_func, NULL, NULL);

  btk_table_attach (BTK_TABLE (table), combo,
		    1, 4, 1, 2,
		    BTK_FILL | BTK_EXPAND, 0, 0, 0);
  btk_widget_show (combo);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), combo);

  label = btk_label_new (NULL);
  priv->paper_size_label = label;
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_table_attach (BTK_TABLE (table), label,
		    1, 4, 2, 3,
		    BTK_FILL, 0, 0, 0);
  btk_widget_show (label);

  label = btk_label_new_with_mnemonic (_("_Orientation:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_table_attach (BTK_TABLE (table), label,
		    0, 1, 3, 4,
		    BTK_FILL, 0, 0, 0);
  btk_widget_show (label);

  radio_button = create_radio_button (NULL, BTK_STOCK_ORIENTATION_PORTRAIT);
  priv->portrait_radio = radio_button;
  btk_table_attach (BTK_TABLE (table), radio_button,
		    1, 2, 3, 4,
		    BTK_EXPAND|BTK_FILL, 0, 0, 0);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), radio_button);

  radio_button = create_radio_button (btk_radio_button_get_group (BTK_RADIO_BUTTON (radio_button)),
				      BTK_STOCK_ORIENTATION_REVERSE_PORTRAIT);
  priv->reverse_portrait_radio = radio_button;
  btk_table_attach (BTK_TABLE (table), radio_button,
		    2, 3, 3, 4,
		    BTK_EXPAND|BTK_FILL, 0, 0, 0);

  radio_button = create_radio_button (btk_radio_button_get_group (BTK_RADIO_BUTTON (radio_button)),
				      BTK_STOCK_ORIENTATION_LANDSCAPE);
  priv->landscape_radio = radio_button;
  btk_table_attach (BTK_TABLE (table), radio_button,
		    1, 2, 4, 5,
		    BTK_EXPAND|BTK_FILL, 0, 0, 0);
  btk_widget_show (radio_button);

  btk_table_set_row_spacing (BTK_TABLE (table), 3, 0);
  
  radio_button = create_radio_button (btk_radio_button_get_group (BTK_RADIO_BUTTON (radio_button)),
				      BTK_STOCK_ORIENTATION_REVERSE_LANDSCAPE);
  priv->reverse_landscape_radio = radio_button;
  btk_table_attach (BTK_TABLE (table), radio_button,
		    2, 3, 4, 5,
		    BTK_EXPAND|BTK_FILL, 0, 0, 0);


  g_signal_connect (priv->paper_size_combo, "changed", G_CALLBACK (paper_size_changed), ps_dialog);
  g_signal_connect (priv->printer_combo, "changed", G_CALLBACK (printer_changed_callback), ps_dialog);
  btk_combo_box_set_active (BTK_COMBO_BOX (priv->printer_combo), 0);

  load_print_backends (ps_dialog);
}

/**
 * btk_page_setup_unix_dialog_new:
 * @title: (allow-none): the title of the dialog, or %NULL
 * @parent: (allow-none): transient parent of the dialog, or %NULL
 *
 * Creates a new page setup dialog.
 *
 * Returns: the new #BtkPageSetupUnixDialog
 *
 * Since: 2.10
 */
BtkWidget *
btk_page_setup_unix_dialog_new (const bchar *title,
				BtkWindow   *parent)
{
  BtkWidget *result;

  if (title == NULL)
    title = _("Page Setup");
  
  result = g_object_new (BTK_TYPE_PAGE_SETUP_UNIX_DIALOG,
                         "title", title,
                         NULL);

  if (parent)
    btk_window_set_transient_for (BTK_WINDOW (result), parent);

  return result;
}

static BtkPageOrientation
get_orientation (BtkPageSetupUnixDialog *dialog)
{
  BtkPageSetupUnixDialogPrivate *priv = dialog->priv;

  if (btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (priv->portrait_radio)))
    return BTK_PAGE_ORIENTATION_PORTRAIT;
  if (btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (priv->landscape_radio)))
    return BTK_PAGE_ORIENTATION_LANDSCAPE;
  if (btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (priv->reverse_landscape_radio)))
    return BTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE;
  return BTK_PAGE_ORIENTATION_REVERSE_PORTRAIT;
}

static void
set_orientation (BtkPageSetupUnixDialog *dialog, 
		 BtkPageOrientation      orientation)
{
  BtkPageSetupUnixDialogPrivate *priv = dialog->priv;

  switch (orientation)
    {
    case BTK_PAGE_ORIENTATION_REVERSE_PORTRAIT:
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (priv->reverse_portrait_radio), TRUE);
      break;
    case BTK_PAGE_ORIENTATION_PORTRAIT:
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (priv->portrait_radio), TRUE);
      break;
    case BTK_PAGE_ORIENTATION_LANDSCAPE:
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (priv->landscape_radio), TRUE);
      break;
    case BTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE:
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (priv->reverse_landscape_radio), TRUE);
      break;
    }
}

/**
 * btk_page_setup_unix_dialog_set_page_setup:
 * @dialog: a #BtkPageSetupUnixDialog
 * @page_setup: a #BtkPageSetup
 * 
 * Sets the #BtkPageSetup from which the page setup
 * dialog takes its values.
 *
 * Since: 2.10
 **/
void
btk_page_setup_unix_dialog_set_page_setup (BtkPageSetupUnixDialog *dialog,
					   BtkPageSetup           *page_setup)
{
  if (page_setup)
    {
      set_paper_size (dialog, page_setup, FALSE, TRUE);
      set_orientation (dialog, btk_page_setup_get_orientation (page_setup));
    }
}

/**
 * btk_page_setup_unix_dialog_get_page_setup:
 * @dialog: a #BtkPageSetupUnixDialog
 * 
 * Gets the currently selected page setup from the dialog. 
 * 
 * Returns: (transfer none): the current page setup 
 *
 * Since: 2.10
 **/
BtkPageSetup *
btk_page_setup_unix_dialog_get_page_setup (BtkPageSetupUnixDialog *dialog)
{
  BtkPageSetup *page_setup;
  
  page_setup = get_current_page_setup (dialog);

  btk_page_setup_set_orientation (page_setup, get_orientation (dialog));

  return page_setup;
}

static bboolean
set_active_printer (BtkPageSetupUnixDialog *dialog,
		    const bchar            *printer_name)
{
  BtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  BtkTreeModel *model;
  BtkTreeIter iter;
  BtkPrinter *printer;

  model = BTK_TREE_MODEL (priv->printer_list);

  if (btk_tree_model_get_iter_first (model, &iter))
    {
      do
	{
	  btk_tree_model_get (BTK_TREE_MODEL (priv->printer_list), &iter,
			      PRINTER_LIST_COL_PRINTER, &printer, -1);
	  if (printer == NULL)
	    continue;
	  
	  if (strcmp (btk_printer_get_name (printer), printer_name) == 0)
	    {
	      btk_combo_box_set_active_iter (BTK_COMBO_BOX (priv->printer_combo),
					     &iter);
	      g_object_unref (printer);
	      return TRUE;
	    }
	      
	  g_object_unref (printer);
	  
	} while (btk_tree_model_iter_next (model, &iter));
    }
  
  return FALSE;
}

/**
 * btk_page_setup_unix_dialog_set_print_settings:
 * @dialog: a #BtkPageSetupUnixDialog
 * @print_settings: a #BtkPrintSettings
 * 
 * Sets the #BtkPrintSettings from which the page setup dialog
 * takes its values.
 * 
 * Since: 2.10
 **/
void
btk_page_setup_unix_dialog_set_print_settings (BtkPageSetupUnixDialog *dialog,
					       BtkPrintSettings       *print_settings)
{
  BtkPageSetupUnixDialogPrivate *priv = dialog->priv;
  const bchar *format_for_printer;

  if (priv->print_settings == print_settings) return;

  if (priv->print_settings)
    g_object_unref (priv->print_settings);

  priv->print_settings = print_settings;

  if (print_settings)
    {
      g_object_ref (print_settings);

      format_for_printer = btk_print_settings_get (print_settings, "format-for-printer");

      /* Set printer if in list, otherwise set when 
       * that printer is added 
       */
      if (format_for_printer &&
	  !set_active_printer (dialog, format_for_printer))
	priv->waiting_for_printer = g_strdup (format_for_printer); 
    }
}

/**
 * btk_page_setup_unix_dialog_get_print_settings:
 * @dialog: a #BtkPageSetupUnixDialog
 * 
 * Gets the current print settings from the dialog.
 * 
 * Returns: (transfer none): the current print settings
 *
 * Since: 2.10
 **/
BtkPrintSettings *
btk_page_setup_unix_dialog_get_print_settings (BtkPageSetupUnixDialog *dialog)
{
  BtkPageSetupUnixDialogPrivate *priv = dialog->priv;

  return priv->print_settings;
}

#define __BTK_PAGE_SETUP_UNIX_DIALOG_C__
#include "btkaliasdef.c"
