/* BtkCustomPaperUnixDialog
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
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

#ifdef HAVE__NL_MEASUREMENT_MEASUREMENT
#include <langinfo.h>
#endif

#include "btkintl.h"
#include "btkprivate.h"

#include "btkliststore.h"

#include "btktreeviewcolumn.h"
#include "btklabel.h"
#include "btkspinbutton.h"

#include "btkcustompaperunixdialog.h"
#include "btkprintbackend.h"
#include "btkprintutils.h"
#include "btkalias.h"

#define CUSTOM_PAPER_FILENAME ".btk-custom-papers"


typedef struct
{
  BtkUnit    display_unit;
  BtkWidget *spin_button;
} UnitWidget;

struct BtkCustomPaperUnixDialogPrivate
{

  BtkWidget *treeview;
  BtkWidget *values_box;
  BtkWidget *printer_combo;
  BtkWidget *width_widget;
  BtkWidget *height_widget;
  BtkWidget *top_widget;
  BtkWidget *bottom_widget;
  BtkWidget *left_widget;
  BtkWidget *right_widget;

  BtkTreeViewColumn *text_column;

  gulong printer_inserted_tag;
  gulong printer_removed_tag;

  guint request_details_tag;
  BtkPrinter *request_details_printer;

  guint non_user_change : 1;

  BtkListStore *custom_paper_list;
  BtkListStore *printer_list;

  GList *print_backends;

  gchar *waiting_for_printer;
};

enum {
  PRINTER_LIST_COL_NAME,
  PRINTER_LIST_COL_PRINTER,
  PRINTER_LIST_N_COLS
};

G_DEFINE_TYPE (BtkCustomPaperUnixDialog, btk_custom_paper_unix_dialog, BTK_TYPE_DIALOG)

#define BTK_CUSTOM_PAPER_UNIX_DIALOG_GET_PRIVATE(o)  \
   (B_TYPE_INSTANCE_GET_PRIVATE ((o), BTK_TYPE_CUSTOM_PAPER_UNIX_DIALOG, BtkCustomPaperUnixDialogPrivate))

static void btk_custom_paper_unix_dialog_finalize  (BObject                *object);
static void populate_dialog                        (BtkCustomPaperUnixDialog *dialog);
static void printer_added_cb                       (BtkPrintBackend        *backend,
						    BtkPrinter             *printer,
						    BtkCustomPaperUnixDialog *dialog);
static void printer_removed_cb                     (BtkPrintBackend        *backend,
						    BtkPrinter             *printer,
						    BtkCustomPaperUnixDialog *dialog);
static void printer_status_cb                      (BtkPrintBackend        *backend,
						    BtkPrinter             *printer,
						    BtkCustomPaperUnixDialog *dialog);



BtkUnit
_btk_print_get_default_user_units (void)
{
  /* Translate to the default units to use for presenting
   * lengths to the user. Translate to default:inch if you
   * want inches, otherwise translate to default:mm.
   * Do *not* translate it to "predefinito:mm", if it
   * it isn't default:mm or default:inch it will not work
   */
  gchar *e = _("default:mm");

#ifdef HAVE__NL_MEASUREMENT_MEASUREMENT
  gchar *imperial = NULL;

  imperial = nl_langinfo (_NL_MEASUREMENT_MEASUREMENT);
  if (imperial && imperial[0] == 2 )
    return BTK_UNIT_INCH;  /* imperial */
  if (imperial && imperial[0] == 1 )
    return BTK_UNIT_MM;  /* metric */
#endif

  if (strcmp (e, "default:inch")==0)
    return BTK_UNIT_INCH;
  else if (strcmp (e, "default:mm"))
    g_warning ("Whoever translated default:mm did so wrongly.\n");
  return BTK_UNIT_MM;
}

static char *
custom_paper_get_filename (void)
{
  gchar *filename;

  filename = g_build_filename (g_get_home_dir (),
			       CUSTOM_PAPER_FILENAME, NULL);
  g_assert (filename != NULL);
  return filename;
}

GList *
_btk_load_custom_papers (void)
{
  GKeyFile *keyfile;
  gchar *filename;
  gchar **groups;
  gsize n_groups, i;
  gboolean load_ok;
  GList *result = NULL;

  filename = custom_paper_get_filename ();

  keyfile = g_key_file_new ();
  load_ok = g_key_file_load_from_file (keyfile, filename, 0, NULL);
  g_free (filename);
  if (!load_ok)
    {
      g_key_file_free (keyfile);
      return NULL;
    }

  groups = g_key_file_get_groups (keyfile, &n_groups);
  for (i = 0; i < n_groups; ++i)
    {
      BtkPageSetup *page_setup;

      page_setup = btk_page_setup_new_from_key_file (keyfile, groups[i], NULL);
      if (!page_setup)
        continue;

      result = g_list_prepend (result, page_setup);
    }

  g_strfreev (groups);
  g_key_file_free (keyfile);

  return g_list_reverse (result);
}

void
_btk_print_load_custom_papers (BtkListStore *store)
{
  BtkTreeIter iter;
  GList *papers, *p;
  BtkPageSetup *page_setup;

  btk_list_store_clear (store);

  papers = _btk_load_custom_papers ();
  for (p = papers; p; p = p->next)
    {
      page_setup = p->data;
      btk_list_store_append (store, &iter);
      btk_list_store_set (store, &iter,
			  0, page_setup,
			  -1);
      g_object_unref (page_setup);
    }

  g_list_free (papers);
}

void
_btk_print_save_custom_papers (BtkListStore *store)
{
  BtkTreeModel *model = BTK_TREE_MODEL (store);
  BtkTreeIter iter;
  GKeyFile *keyfile;
  gchar *filename, *data;
  gsize len;
  gint i = 0;

  keyfile = g_key_file_new ();

  if (btk_tree_model_get_iter_first (model, &iter))
    {
      do
	{
	  BtkPageSetup *page_setup;
	  gchar group[32];

	  g_snprintf (group, sizeof (group), "Paper%u", i);

	  btk_tree_model_get (model, &iter, 0, &page_setup, -1);

	  btk_page_setup_to_key_file (page_setup, keyfile, group);

	  ++i;
	} while (btk_tree_model_iter_next (model, &iter));
    }

  filename = custom_paper_get_filename ();
  data = g_key_file_to_data (keyfile, &len, NULL);
  g_file_set_contents (filename, data, len, NULL);
  g_free (data);
  g_free (filename);
}

static void
btk_custom_paper_unix_dialog_class_init (BtkCustomPaperUnixDialogClass *class)
{
  BObjectClass *object_class;
  BtkWidgetClass *widget_class;

  object_class = (BObjectClass *) class;
  widget_class = (BtkWidgetClass *) class;

  object_class->finalize = btk_custom_paper_unix_dialog_finalize;

  g_type_class_add_private (class, sizeof (BtkCustomPaperUnixDialogPrivate));
}

static void
custom_paper_dialog_response_cb (BtkDialog *dialog,
				 gint       response,
				 gpointer   user_data)
{
  BtkCustomPaperUnixDialogPrivate *priv = BTK_CUSTOM_PAPER_UNIX_DIALOG (dialog)->priv;

  _btk_print_save_custom_papers (priv->custom_paper_list);
}

static void
btk_custom_paper_unix_dialog_init (BtkCustomPaperUnixDialog *dialog)
{
  BtkCustomPaperUnixDialogPrivate *priv;
  BtkTreeIter iter;

  priv = dialog->priv = BTK_CUSTOM_PAPER_UNIX_DIALOG_GET_PRIVATE (dialog);

  priv->print_backends = NULL;

  priv->request_details_printer = NULL;
  priv->request_details_tag = 0;

  priv->printer_list = btk_list_store_new (PRINTER_LIST_N_COLS,
					   B_TYPE_STRING,
					   B_TYPE_OBJECT);

  btk_list_store_append (priv->printer_list, &iter);

  priv->custom_paper_list = btk_list_store_new (1, B_TYPE_OBJECT);
  _btk_print_load_custom_papers (priv->custom_paper_list);

  populate_dialog (dialog);

  btk_dialog_add_buttons (BTK_DIALOG (dialog),
                          BTK_STOCK_CLOSE, BTK_RESPONSE_CLOSE,
                          NULL);

  btk_dialog_set_default_response (BTK_DIALOG (dialog), BTK_RESPONSE_CLOSE);

  g_signal_connect (dialog, "response", G_CALLBACK (custom_paper_dialog_response_cb), NULL);
}

static void
btk_custom_paper_unix_dialog_finalize (BObject *object)
{
  BtkCustomPaperUnixDialog *dialog = BTK_CUSTOM_PAPER_UNIX_DIALOG (object);
  BtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  BtkPrintBackend *backend;
  GList *node;

  if (priv->printer_list)
    {
      g_signal_handler_disconnect (priv->printer_list, priv->printer_inserted_tag);
      g_signal_handler_disconnect (priv->printer_list, priv->printer_removed_tag);
      g_object_unref (priv->printer_list);
      priv->printer_list = NULL;
    }

  if (priv->request_details_tag)
    {
      g_signal_handler_disconnect (priv->request_details_printer,
				   priv->request_details_tag);
      g_object_unref (priv->request_details_printer);
      priv->request_details_printer = NULL;
      priv->request_details_tag = 0;
    }

  if (priv->custom_paper_list)
    {
      g_object_unref (priv->custom_paper_list);
      priv->custom_paper_list = NULL;
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

  B_OBJECT_CLASS (btk_custom_paper_unix_dialog_parent_class)->finalize (object);
}

/**
 * btk_custom_paper_unix_dialog_new:
 * @title: (allow-none): the title of the dialog, or %NULL
 * @parent: (allow-none): transient parent of the dialog, or %NULL
 *
 * Creates a new custom paper dialog.
 *
 * Returns: the new #BtkCustomPaperUnixDialog
 *
 * Since: 2.18
 */
BtkWidget *
_btk_custom_paper_unix_dialog_new (BtkWindow   *parent,
				  const gchar *title)
{
  BtkWidget *result;

  if (title == NULL)
    title = _("Manage Custom Sizes");

  result = g_object_new (BTK_TYPE_CUSTOM_PAPER_UNIX_DIALOG,
                         "title", title,
                         "transient-for", parent,
                         "modal", parent != NULL,
                         "destroy-with-parent", TRUE,
                         NULL);

  return result;
}

static void
printer_added_cb (BtkPrintBackend          *backend,
		  BtkPrinter               *printer,
		  BtkCustomPaperUnixDialog *dialog)
{
  BtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  BtkTreeIter iter;
  gchar *str;

  if (btk_printer_is_virtual (printer))
    return;

  str = g_strdup_printf ("<b>%s</b>",
			 btk_printer_get_name (printer));

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
		    BtkCustomPaperUnixDialog *dialog)
{
  BtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  BtkTreeIter *iter;

  iter = g_object_get_data (B_OBJECT (printer), "btk-print-tree-iter");
  btk_list_store_remove (BTK_LIST_STORE (priv->printer_list), iter);
}


static void
printer_status_cb (BtkPrintBackend        *backend,
                   BtkPrinter             *printer,
		   BtkCustomPaperUnixDialog *dialog)
{
  BtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  BtkTreeIter *iter;
  gchar *str;

  iter = g_object_get_data (B_OBJECT (printer), "btk-print-tree-iter");

  str = g_strdup_printf ("<b>%s</b>",
			 btk_printer_get_name (printer));
  btk_list_store_set (priv->printer_list, iter,
                      PRINTER_LIST_COL_NAME, str,
                      -1);
  g_free (str);
}

static void
printer_list_initialize (BtkCustomPaperUnixDialog *dialog,
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
load_print_backends (BtkCustomPaperUnixDialog *dialog)
{
  BtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  GList *node;

  if (g_module_supported ())
    priv->print_backends = btk_print_backend_load_modules ();

  for (node = priv->print_backends; node != NULL; node = node->next)
    printer_list_initialize (dialog, BTK_PRINT_BACKEND (node->data));
}

static void unit_widget_changed (BtkCustomPaperUnixDialog *dialog);

static BtkWidget *
new_unit_widget (BtkCustomPaperUnixDialog *dialog,
		 BtkUnit                   unit,
		 BtkWidget                *mnemonic_label)
{
  BtkWidget *hbox, *button, *label;
  UnitWidget *data;

  data = g_new0 (UnitWidget, 1);
  data->display_unit = unit;

  hbox = btk_hbox_new (FALSE, 6);

  button = btk_spin_button_new_with_range (0.0, 9999.0, 1);
  if (unit == BTK_UNIT_INCH)
    btk_spin_button_set_digits (BTK_SPIN_BUTTON (button), 2);
  else
    btk_spin_button_set_digits (BTK_SPIN_BUTTON (button), 1);

  btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);
  btk_widget_show (button);

  data->spin_button = button;

  g_signal_connect_swapped (button, "value-changed",
			    G_CALLBACK (unit_widget_changed), dialog);

  if (unit == BTK_UNIT_INCH)
    label = btk_label_new (_("inch"));
  else
    label = btk_label_new (_("mm"));

  btk_box_pack_start (BTK_BOX (hbox), label, FALSE, FALSE, 0);
  btk_widget_show (label);
  btk_label_set_mnemonic_widget (BTK_LABEL (mnemonic_label), button);

  g_object_set_data_full (B_OBJECT (hbox), "unit-data", data, g_free);

  return hbox;
}

static double
unit_widget_get (BtkWidget *unit_widget)
{
  UnitWidget *data = g_object_get_data (B_OBJECT (unit_widget), "unit-data");
  return _btk_print_convert_to_mm (btk_spin_button_get_value (BTK_SPIN_BUTTON (data->spin_button)),
				   data->display_unit);
}

static void
unit_widget_set (BtkWidget *unit_widget,
		 gdouble    value)
{
  UnitWidget *data;

  data = g_object_get_data (B_OBJECT (unit_widget), "unit-data");
  btk_spin_button_set_value (BTK_SPIN_BUTTON (data->spin_button),
			     _btk_print_convert_from_mm (value, data->display_unit));
}

static void
custom_paper_printer_data_func (BtkCellLayout   *cell_layout,
				BtkCellRenderer *cell,
				BtkTreeModel    *tree_model,
				BtkTreeIter     *iter,
				gpointer         data)
{
  BtkPrinter *printer;

  btk_tree_model_get (tree_model, iter,
		      PRINTER_LIST_COL_PRINTER, &printer, -1);

  if (printer)
    g_object_set (cell, "text",  btk_printer_get_name (printer), NULL);
  else
    g_object_set (cell, "text",  _("Margins from Printer..."), NULL);

  if (printer)
    g_object_unref (printer);
}

static void
update_combo_sensitivity_from_printers (BtkCustomPaperUnixDialog *dialog)
{
  BtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  BtkTreeIter iter;
  gboolean sensitive;
  BtkTreeSelection *selection;
  BtkTreeModel *model;

  sensitive = FALSE;
  model = BTK_TREE_MODEL (priv->printer_list);
  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (priv->treeview));
  if (btk_tree_model_get_iter_first (model, &iter) &&
      btk_tree_model_iter_next (model, &iter) &&
      btk_tree_selection_get_selected (selection, NULL, &iter))
    sensitive = TRUE;

  btk_widget_set_sensitive (priv->printer_combo, sensitive);
}

static void
update_custom_widgets_from_list (BtkCustomPaperUnixDialog *dialog)
{
  BtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  BtkTreeSelection *selection;
  BtkTreeModel *model;
  BtkTreeIter iter;
  BtkPageSetup *page_setup;

  model = btk_tree_view_get_model (BTK_TREE_VIEW (priv->treeview));
  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (priv->treeview));

  priv->non_user_change = TRUE;
  if (btk_tree_selection_get_selected (selection, NULL, &iter))
    {
      btk_tree_model_get (model, &iter, 0, &page_setup, -1);

      unit_widget_set (priv->width_widget,
		       btk_page_setup_get_paper_width (page_setup, BTK_UNIT_MM));
      unit_widget_set (priv->height_widget,
		       btk_page_setup_get_paper_height (page_setup, BTK_UNIT_MM));
      unit_widget_set (priv->top_widget,
		       btk_page_setup_get_top_margin (page_setup, BTK_UNIT_MM));
      unit_widget_set (priv->bottom_widget,
		       btk_page_setup_get_bottom_margin (page_setup, BTK_UNIT_MM));
      unit_widget_set (priv->left_widget,
		       btk_page_setup_get_left_margin (page_setup, BTK_UNIT_MM));
      unit_widget_set (priv->right_widget,
		       btk_page_setup_get_right_margin (page_setup, BTK_UNIT_MM));

      btk_widget_set_sensitive (priv->values_box, TRUE);
    }
  else
    {
      btk_widget_set_sensitive (priv->values_box, FALSE);
    }

  if (priv->printer_list)
    update_combo_sensitivity_from_printers (dialog);
  priv->non_user_change = FALSE;
}

static void
selected_custom_paper_changed (BtkTreeSelection         *selection,
			       BtkCustomPaperUnixDialog *dialog)
{
  update_custom_widgets_from_list (dialog);
}

static void
unit_widget_changed (BtkCustomPaperUnixDialog *dialog)
{
  BtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  gdouble w, h, top, bottom, left, right;
  BtkTreeSelection *selection;
  BtkTreeIter iter;
  BtkPageSetup *page_setup;
  BtkPaperSize *paper_size;

  if (priv->non_user_change)
    return;

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (priv->treeview));

  if (btk_tree_selection_get_selected (selection, NULL, &iter))
    {
      btk_tree_model_get (BTK_TREE_MODEL (priv->custom_paper_list), &iter, 0, &page_setup, -1);

      w = unit_widget_get (priv->width_widget);
      h = unit_widget_get (priv->height_widget);

      paper_size = btk_page_setup_get_paper_size (page_setup);
      btk_paper_size_set_size (paper_size, w, h, BTK_UNIT_MM);

      top = unit_widget_get (priv->top_widget);
      bottom = unit_widget_get (priv->bottom_widget);
      left = unit_widget_get (priv->left_widget);
      right = unit_widget_get (priv->right_widget);

      btk_page_setup_set_top_margin (page_setup, top, BTK_UNIT_MM);
      btk_page_setup_set_bottom_margin (page_setup, bottom, BTK_UNIT_MM);
      btk_page_setup_set_left_margin (page_setup, left, BTK_UNIT_MM);
      btk_page_setup_set_right_margin (page_setup, right, BTK_UNIT_MM);

      g_object_unref (page_setup);
    }
}

static gboolean
custom_paper_name_used (BtkCustomPaperUnixDialog *dialog,
			const gchar              *name)
{
  BtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  BtkTreeModel *model;
  BtkTreeIter iter;
  BtkPageSetup *page_setup;
  BtkPaperSize *paper_size;

  model = btk_tree_view_get_model (BTK_TREE_VIEW (priv->treeview));

  if (btk_tree_model_get_iter_first (model, &iter))
    {
      do
	{
	  btk_tree_model_get (model, &iter, 0, &page_setup, -1);
	  paper_size = btk_page_setup_get_paper_size (page_setup);
	  if (strcmp (name,
		      btk_paper_size_get_name (paper_size)) == 0)
	    {
	      g_object_unref (page_setup);
	      return TRUE;
	    }
	  g_object_unref (page_setup);
	} while (btk_tree_model_iter_next (model, &iter));
    }

  return FALSE;
}

static void
add_custom_paper (BtkCustomPaperUnixDialog *dialog)
{
  BtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  BtkListStore *store;
  BtkPageSetup *page_setup;
  BtkPaperSize *paper_size;
  BtkTreeSelection *selection;
  BtkTreePath *path;
  BtkTreeIter iter;
  gchar *name;
  gint i;

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (priv->treeview));
  store = priv->custom_paper_list;

  i = 1;
  name = NULL;
  do
    {
      g_free (name);
      name = g_strdup_printf (_("Custom Size %d"), i);
      i++;
    } while (custom_paper_name_used (dialog, name));

  page_setup = btk_page_setup_new ();
  paper_size = btk_paper_size_new_custom (name, name,
					  btk_page_setup_get_paper_width (page_setup, BTK_UNIT_MM),
					  btk_page_setup_get_paper_height (page_setup, BTK_UNIT_MM),
					  BTK_UNIT_MM);
  btk_page_setup_set_paper_size (page_setup, paper_size);
  btk_paper_size_free (paper_size);

  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, page_setup, -1);
  g_object_unref (page_setup);

  btk_tree_selection_select_iter (selection, &iter);
  path = btk_tree_model_get_path (BTK_TREE_MODEL (store), &iter);
  btk_widget_grab_focus (priv->treeview);
  btk_tree_view_set_cursor (BTK_TREE_VIEW (priv->treeview), path,
			    priv->text_column, TRUE);
  btk_tree_path_free (path);
  g_free (name);
}

static void
remove_custom_paper (BtkCustomPaperUnixDialog *dialog)
{
  BtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  BtkTreeSelection *selection;
  BtkTreeIter iter;
  BtkListStore *store;

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (priv->treeview));
  store = priv->custom_paper_list;

  if (btk_tree_selection_get_selected (selection, NULL, &iter))
    {
      BtkTreePath *path = btk_tree_model_get_path (BTK_TREE_MODEL (store), &iter);
      btk_list_store_remove (store, &iter);

      if (btk_tree_model_get_iter (BTK_TREE_MODEL (store), &iter, path))
	btk_tree_selection_select_iter (selection, &iter);
      else if (btk_tree_path_prev (path) && btk_tree_model_get_iter (BTK_TREE_MODEL (store), &iter, path))
	btk_tree_selection_select_iter (selection, &iter);

      btk_tree_path_free (path);
    }
}

static void
set_margins_from_printer (BtkCustomPaperUnixDialog *dialog,
			  BtkPrinter               *printer)
{
  BtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  gdouble top, bottom, left, right;

  top = bottom = left = right = 0;
  if (!btk_printer_get_hard_margins (printer, &top, &bottom, &left, &right))
    return;

  priv->non_user_change = TRUE;
  unit_widget_set (priv->top_widget, _btk_print_convert_to_mm (top, BTK_UNIT_POINTS));
  unit_widget_set (priv->bottom_widget, _btk_print_convert_to_mm (bottom, BTK_UNIT_POINTS));
  unit_widget_set (priv->left_widget, _btk_print_convert_to_mm (left, BTK_UNIT_POINTS));
  unit_widget_set (priv->right_widget, _btk_print_convert_to_mm (right, BTK_UNIT_POINTS));
  priv->non_user_change = FALSE;

  /* Only send one change */
  unit_widget_changed (dialog);
}

static void
get_margins_finished_callback (BtkPrinter               *printer,
			       gboolean                  success,
			       BtkCustomPaperUnixDialog *dialog)
{
  BtkCustomPaperUnixDialogPrivate *priv = dialog->priv;

  g_signal_handler_disconnect (priv->request_details_printer,
			       priv->request_details_tag);
  g_object_unref (priv->request_details_printer);
  priv->request_details_tag = 0;
  priv->request_details_printer = NULL;

  if (success)
    set_margins_from_printer (dialog, printer);

  btk_combo_box_set_active (BTK_COMBO_BOX (priv->printer_combo), 0);
}

static void
margins_from_printer_changed (BtkCustomPaperUnixDialog *dialog)
{
  BtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  BtkTreeIter iter;
  BtkComboBox *combo;
  BtkPrinter *printer;

  combo = BTK_COMBO_BOX (priv->printer_combo);

  if (priv->request_details_tag)
    {
      g_signal_handler_disconnect (priv->request_details_printer,
				   priv->request_details_tag);
      g_object_unref (priv->request_details_printer);
      priv->request_details_printer = NULL;
      priv->request_details_tag = 0;
    }

  if (btk_combo_box_get_active_iter (combo, &iter))
    {
      btk_tree_model_get (btk_combo_box_get_model (combo), &iter,
			  PRINTER_LIST_COL_PRINTER, &printer, -1);

      if (printer)
	{
	  if (btk_printer_has_details (printer))
	    {
	      set_margins_from_printer (dialog, printer);
	      btk_combo_box_set_active (combo, 0);
	    }
	  else
	    {
	      priv->request_details_printer = g_object_ref (printer);
	      priv->request_details_tag =
		g_signal_connect (printer, "details-acquired",
				  G_CALLBACK (get_margins_finished_callback), dialog);
	      btk_printer_request_details (printer);
	    }

	  g_object_unref (printer);
	}
    }
}

static void
custom_size_name_edited (BtkCellRenderer          *cell,
			 gchar                    *path_string,
			 gchar                    *new_text,
			 BtkCustomPaperUnixDialog *dialog)
{
  BtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  BtkTreePath *path;
  BtkTreeIter iter;
  BtkListStore *store;
  BtkPageSetup *page_setup;
  BtkPaperSize *paper_size;

  store = priv->custom_paper_list;
  path = btk_tree_path_new_from_string (path_string);
  btk_tree_model_get_iter (BTK_TREE_MODEL (store), &iter, path);
  btk_tree_model_get (BTK_TREE_MODEL (store), &iter, 0, &page_setup, -1);
  btk_tree_path_free (path);

  paper_size = btk_paper_size_new_custom (new_text, new_text,
					  btk_page_setup_get_paper_width (page_setup, BTK_UNIT_MM),
					  btk_page_setup_get_paper_height (page_setup, BTK_UNIT_MM),
					  BTK_UNIT_MM);
  btk_page_setup_set_paper_size (page_setup, paper_size);
  btk_paper_size_free (paper_size);

  g_object_unref (page_setup);
}

static void
custom_name_func (BtkTreeViewColumn *tree_column,
		  BtkCellRenderer   *cell,
		  BtkTreeModel      *tree_model,
		  BtkTreeIter       *iter,
		  gpointer           data)
{
  BtkPageSetup *page_setup;
  BtkPaperSize *paper_size;

  btk_tree_model_get (tree_model, iter, 0, &page_setup, -1);
  if (page_setup)
    {
      paper_size = btk_page_setup_get_paper_size (page_setup);
      g_object_set (cell, "text",  btk_paper_size_get_display_name (paper_size), NULL);
      g_object_unref (page_setup);
    }
}

static BtkWidget *
wrap_in_frame (const gchar *label,
               BtkWidget   *child)
{
  BtkWidget *frame, *alignment, *label_widget;
  gchar *bold_text;

  label_widget = btk_label_new (NULL);
  btk_misc_set_alignment (BTK_MISC (label_widget), 0.0, 0.5);
  btk_widget_show (label_widget);

  bold_text = g_markup_printf_escaped ("<b>%s</b>", label);
  btk_label_set_markup (BTK_LABEL (label_widget), bold_text);
  g_free (bold_text);

  frame = btk_vbox_new (FALSE, 6);
  btk_box_pack_start (BTK_BOX (frame), label_widget, FALSE, FALSE, 0);

  alignment = btk_alignment_new (0.0, 0.0, 1.0, 1.0);
  btk_alignment_set_padding (BTK_ALIGNMENT (alignment),
			     0, 0, 12, 0);
  btk_box_pack_start (BTK_BOX (frame), alignment, FALSE, FALSE, 0);

  btk_container_add (BTK_CONTAINER (alignment), child);

  btk_widget_show (frame);
  btk_widget_show (alignment);

  return frame;
}

static void
populate_dialog (BtkCustomPaperUnixDialog *dialog)
{
  BtkCustomPaperUnixDialogPrivate *priv = dialog->priv;
  BtkWidget *image, *table, *label, *widget, *frame, *combo;
  BtkWidget *hbox, *vbox, *treeview, *scrolled, *button_box, *button;
  BtkCellRenderer *cell;
  BtkTreeViewColumn *column;
  BtkTreeIter iter;
  BtkTreeSelection *selection;
  BtkUnit user_units;

  btk_dialog_set_has_separator (BTK_DIALOG (dialog), FALSE);
  btk_container_set_border_width (BTK_CONTAINER (dialog), 5);
  btk_box_set_spacing (BTK_BOX (BTK_DIALOG (dialog)->vbox), 2); /* 2 * 5 + 2 = 12 */
  btk_container_set_border_width (BTK_CONTAINER (BTK_DIALOG (dialog)->action_area), 5);
  btk_box_set_spacing (BTK_BOX (BTK_DIALOG (dialog)->action_area), 6);

  hbox = btk_hbox_new (FALSE, 18);
  btk_container_set_border_width (BTK_CONTAINER (hbox), 5);
  btk_box_pack_start (BTK_BOX (BTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);
  btk_widget_show (hbox);

  vbox = btk_vbox_new (FALSE, 6);
  btk_box_pack_start (BTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  btk_widget_show (vbox);

  scrolled = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled),
				  BTK_POLICY_AUTOMATIC,
				  BTK_POLICY_AUTOMATIC);
  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (scrolled),
				       BTK_SHADOW_IN);
  btk_box_pack_start (BTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
  btk_widget_show (scrolled);

  treeview = btk_tree_view_new_with_model (BTK_TREE_MODEL (priv->custom_paper_list));
  priv->treeview = treeview;
  btk_tree_view_set_headers_visible (BTK_TREE_VIEW (treeview), FALSE);
  btk_widget_set_size_request (treeview, 140, -1);

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (treeview));
  btk_tree_selection_set_mode (selection, BTK_SELECTION_BROWSE);
  g_signal_connect (selection, "changed", G_CALLBACK (selected_custom_paper_changed), dialog);

  cell = btk_cell_renderer_text_new ();
  g_object_set (cell, "editable", TRUE, NULL);
  g_signal_connect (cell, "edited", 
		    G_CALLBACK (custom_size_name_edited), dialog);
  priv->text_column = column =
    btk_tree_view_column_new_with_attributes ("paper", cell,
					      NULL);
  btk_tree_view_column_set_cell_data_func  (column, cell, custom_name_func, NULL, NULL);

  btk_tree_view_append_column (BTK_TREE_VIEW (treeview), column);

  btk_container_add (BTK_CONTAINER (scrolled), treeview);
  btk_widget_show (treeview);

  button_box = btk_hbox_new (FALSE, 6);
  btk_box_pack_start (BTK_BOX (vbox), button_box, FALSE, FALSE, 0);
  btk_widget_show (button_box);

  button = btk_button_new ();
  image = btk_image_new_from_stock (BTK_STOCK_ADD, BTK_ICON_SIZE_BUTTON);
  btk_widget_show (image);
  btk_container_add (BTK_CONTAINER (button), image);
  btk_box_pack_start (BTK_BOX (button_box), button, FALSE, FALSE, 0);
  btk_widget_show (button);

  g_signal_connect_swapped (button, "clicked", G_CALLBACK (add_custom_paper), dialog);

  button = btk_button_new ();
  image = btk_image_new_from_stock (BTK_STOCK_REMOVE, BTK_ICON_SIZE_BUTTON);
  btk_widget_show (image);
  btk_container_add (BTK_CONTAINER (button), image);
  btk_box_pack_start (BTK_BOX (button_box), button, FALSE, FALSE, 0);
  btk_widget_show (button);

  g_signal_connect_swapped (button, "clicked", G_CALLBACK (remove_custom_paper), dialog);

  user_units = _btk_print_get_default_user_units ();

  vbox = btk_vbox_new (FALSE, 18);
  priv->values_box = vbox;
  btk_box_pack_start (BTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  btk_widget_show (vbox);

  table = btk_table_new (2, 2, FALSE);

  btk_table_set_row_spacings (BTK_TABLE (table), 6);
  btk_table_set_col_spacings (BTK_TABLE (table), 12);

  label = btk_label_new_with_mnemonic (_("_Width:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_widget_show (label);
  btk_table_attach (BTK_TABLE (table), label,
		    0, 1, 0, 1, BTK_FILL, 0, 0, 0);

  widget = new_unit_widget (dialog, user_units, label);
  priv->width_widget = widget;
  btk_table_attach (BTK_TABLE (table), widget,
		    1, 2, 0, 1, BTK_FILL, 0, 0, 0);
  btk_widget_show (widget);

  label = btk_label_new_with_mnemonic (_("_Height:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_widget_show (label);
  btk_table_attach (BTK_TABLE (table), label,
		    0, 1, 1, 2, BTK_FILL, 0, 0, 0);

  widget = new_unit_widget (dialog, user_units, label);
  priv->height_widget = widget;
  btk_table_attach (BTK_TABLE (table), widget,
		    1, 2, 1, 2, BTK_FILL, 0, 0, 0);
  btk_widget_show (widget);

  frame = wrap_in_frame (_("Paper Size"), table);
  btk_widget_show (table);
  btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);
  btk_widget_show (frame);

  table = btk_table_new (5, 2, FALSE);
  btk_table_set_row_spacings (BTK_TABLE (table), 6);
  btk_table_set_col_spacings (BTK_TABLE (table), 12);

  label = btk_label_new_with_mnemonic (_("_Top:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_table_attach (BTK_TABLE (table), label,
		    0, 1, 0, 1, BTK_FILL, 0, 0, 0);
  btk_widget_show (label);

  widget = new_unit_widget (dialog, user_units, label);
  priv->top_widget = widget;
  btk_table_attach (BTK_TABLE (table), widget,
		    1, 2, 0, 1, BTK_FILL, 0, 0, 0);
  btk_widget_show (widget);

  label = btk_label_new_with_mnemonic (_("_Bottom:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_table_attach (BTK_TABLE (table), label,
		    0, 1 , 1, 2, BTK_FILL, 0, 0, 0);
  btk_widget_show (label);

  widget = new_unit_widget (dialog, user_units, label);
  priv->bottom_widget = widget;
  btk_table_attach (BTK_TABLE (table), widget,
		    1, 2, 1, 2, BTK_FILL, 0, 0, 0);
  btk_widget_show (widget);

  label = btk_label_new_with_mnemonic (_("_Left:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_table_attach (BTK_TABLE (table), label,
		    0, 1, 2, 3, BTK_FILL, 0, 0, 0);
  btk_widget_show (label);

  widget = new_unit_widget (dialog, user_units, label);
  priv->left_widget = widget;
  btk_table_attach (BTK_TABLE (table), widget,
		    1, 2, 2, 3, BTK_FILL, 0, 0, 0);
  btk_widget_show (widget);

  label = btk_label_new_with_mnemonic (_("_Right:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_table_attach (BTK_TABLE (table), label,
		    0, 1, 3, 4, BTK_FILL, 0, 0, 0);
  btk_widget_show (label);

  widget = new_unit_widget (dialog, user_units, label);
  priv->right_widget = widget;
  btk_table_attach (BTK_TABLE (table), widget,
		    1, 2, 3, 4, BTK_FILL, 0, 0, 0);
  btk_widget_show (widget);

  hbox = btk_hbox_new (FALSE, 0);
  btk_table_attach (BTK_TABLE (table), hbox,
		    0, 2, 4, 5, BTK_FILL | BTK_EXPAND, 0, 0, 0);
  btk_widget_show (hbox);

  combo = btk_combo_box_new_with_model (BTK_TREE_MODEL (priv->printer_list));
  priv->printer_combo = combo;

  priv->printer_inserted_tag =
    g_signal_connect_swapped (priv->printer_list, "row-inserted",
			      G_CALLBACK (update_combo_sensitivity_from_printers), dialog);
  priv->printer_removed_tag =
    g_signal_connect_swapped (priv->printer_list, "row-deleted",
			      G_CALLBACK (update_combo_sensitivity_from_printers), dialog);
  update_combo_sensitivity_from_printers (dialog);

  cell = btk_cell_renderer_text_new ();
  btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combo), cell, TRUE);
  btk_cell_layout_set_cell_data_func (BTK_CELL_LAYOUT (combo), cell,
				      custom_paper_printer_data_func,
				      NULL, NULL);

  btk_combo_box_set_active (BTK_COMBO_BOX (combo), 0);
  btk_box_pack_start (BTK_BOX (hbox), combo, FALSE, FALSE, 0);
  btk_widget_show (combo);

  g_signal_connect_swapped (combo, "changed",
			    G_CALLBACK (margins_from_printer_changed), dialog);

  frame = wrap_in_frame (_("Paper Margins"), table);
  btk_widget_show (table);
  btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);
  btk_widget_show (frame);

  update_custom_widgets_from_list (dialog);

  /* If no custom sizes, add one */
  if (!btk_tree_model_get_iter_first (BTK_TREE_MODEL (priv->custom_paper_list),
				      &iter))
    {
      /* Need to realize treeview so we can start the rename */
      btk_widget_realize (treeview);
      add_custom_paper (dialog);
    }

  btk_window_present (BTK_WINDOW (dialog));

  load_print_backends (dialog);
}


#define __BTK_CUSTOM_PAPER_UNIX_DIALOG_C__
#include "btkaliasdef.c"
