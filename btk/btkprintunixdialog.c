/* BtkPrintUnixDialog
 * Copyright (C) 2006 John (J5) Palmieri  <johnp@redhat.com>
 * Copyright (C) 2006 Alexander Larsson <alexl@redhat.com>
 * Copyright © 2006, 2007 Christian Persch
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
#include <ctype.h>
#include <stdio.h>
#include <math.h>

#include "btkintl.h"
#include "btkprivate.h"

#include "btkspinbutton.h"
#include "btkcellrendererpixbuf.h"
#include "btkcellrenderertext.h"
#include "btkstock.h"
#include "btkiconfactory.h"
#include "btkimage.h"
#include "btktreeselection.h"
#include "btknotebook.h"
#include "btkscrolledwindow.h"
#include "btkcombobox.h"
#include "btktogglebutton.h"
#include "btkradiobutton.h"
#include "btkdrawingarea.h"
#include "btkvbox.h"
#include "btktable.h"
#include "btkframe.h"
#include "btkalignment.h"
#include "btklabel.h"
#include "btkeventbox.h"
#include "btkbuildable.h"

#include "btkcustompaperunixdialog.h"
#include "btkprintbackend.h"
#include "btkprinter-private.h"
#include "btkprintunixdialog.h"
#include "btkprinteroptionwidget.h"
#include "btkprintutils.h"
#include "btkalias.h"

#include "btkmessagedialog.h"
#include "btkbutton.h"

#define EXAMPLE_PAGE_AREA_SIZE 110
#define RULER_DISTANCE 7.5
#define RULER_RADIUS 2


#define BTK_PRINT_UNIX_DIALOG_GET_PRIVATE(o)  \
   (B_TYPE_INSTANCE_GET_PRIVATE ((o), BTK_TYPE_PRINT_UNIX_DIALOG, BtkPrintUnixDialogPrivate))

static void     btk_print_unix_dialog_destroy      (BtkPrintUnixDialog *dialog);
static void     btk_print_unix_dialog_finalize     (BObject            *object);
static void     btk_print_unix_dialog_set_property (BObject            *object,
                                                    buint               prop_id,
                                                    const BValue       *value,
                                                    BParamSpec         *pspec);
static void     btk_print_unix_dialog_get_property (BObject            *object,
                                                    buint               prop_id,
                                                    BValue             *value,
                                                    BParamSpec         *pspec);
static void     btk_print_unix_dialog_style_set    (BtkWidget          *widget,
                                                    BtkStyle           *previous_style);
static void     populate_dialog                    (BtkPrintUnixDialog *dialog);
static void     unschedule_idle_mark_conflicts     (BtkPrintUnixDialog *dialog);
static void     selected_printer_changed           (BtkTreeSelection   *selection,
                                                    BtkPrintUnixDialog *dialog);
static void     clear_per_printer_ui               (BtkPrintUnixDialog *dialog);
static void     printer_added_cb                   (BtkPrintBackend    *backend,
                                                    BtkPrinter         *printer,
                                                    BtkPrintUnixDialog *dialog);
static void     printer_removed_cb                 (BtkPrintBackend    *backend,
                                                    BtkPrinter         *printer,
                                                    BtkPrintUnixDialog *dialog);
static void     printer_status_cb                  (BtkPrintBackend    *backend,
                                                    BtkPrinter         *printer,
                                                    BtkPrintUnixDialog *dialog);
static void     update_collate_icon                (BtkToggleButton    *toggle_button,
                                                    BtkPrintUnixDialog *dialog);
static bboolean dialog_get_collate                 (BtkPrintUnixDialog *dialog);
static bboolean dialog_get_reverse                 (BtkPrintUnixDialog *dialog);
static bint     dialog_get_n_copies                (BtkPrintUnixDialog *dialog);

static void     set_cell_sensitivity_func          (BtkTreeViewColumn *tree_column,
                                                    BtkCellRenderer   *cell,
                                                    BtkTreeModel      *model,
                                                    BtkTreeIter       *iter,
                                                    bpointer           data);
static bboolean set_active_printer                 (BtkPrintUnixDialog *dialog,
                                                    const bchar        *printer_name);
static void redraw_page_layout_preview             (BtkPrintUnixDialog *dialog);

/* BtkBuildable */
static void btk_print_unix_dialog_buildable_init                    (BtkBuildableIface *iface);
static BObject *btk_print_unix_dialog_buildable_get_internal_child  (BtkBuildable *buildable,
                                                                     BtkBuilder   *builder,
                                                                     const bchar  *childname);

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

enum {
  PAGE_SETUP_LIST_COL_PAGE_SETUP,
  PAGE_SETUP_LIST_COL_IS_SEPARATOR,
  PAGE_SETUP_LIST_N_COLS
};

enum {
  PROP_0,
  PROP_PAGE_SETUP,
  PROP_CURRENT_PAGE,
  PROP_PRINT_SETTINGS,
  PROP_SELECTED_PRINTER,
  PROP_MANUAL_CAPABILITIES,
  PROP_SUPPORT_SELECTION,
  PROP_HAS_SELECTION,
  PROP_EMBED_PAGE_SETUP
};

enum {
  PRINTER_LIST_COL_ICON,
  PRINTER_LIST_COL_NAME,
  PRINTER_LIST_COL_STATE,
  PRINTER_LIST_COL_JOBS,
  PRINTER_LIST_COL_LOCATION,
  PRINTER_LIST_COL_PRINTER_OBJ,
  PRINTER_LIST_N_COLS
};

struct BtkPrintUnixDialogPrivate
{
  BtkWidget *notebook;

  BtkWidget *printer_treeview;

  BtkPrintCapabilities manual_capabilities;
  BtkPrintCapabilities printer_capabilities;

  BtkTreeModel *printer_list;
  BtkTreeModelFilter *printer_list_filter;

  BtkPageSetup *page_setup;
  bboolean page_setup_set;
  bboolean embed_page_setup;
  BtkListStore *page_setup_list;
  BtkListStore *custom_paper_list;

  bboolean support_selection;
  bboolean has_selection;

  BtkWidget *all_pages_radio;
  BtkWidget *current_page_radio;
  BtkWidget *selection_radio;
  BtkWidget *range_table;
  BtkWidget *page_range_radio;
  BtkWidget *page_range_entry;

  BtkWidget *copies_spin;
  BtkWidget *collate_check;
  BtkWidget *reverse_check;
  BtkWidget *collate_image;
  BtkWidget *page_layout_preview;
  BtkWidget *scale_spin;
  BtkWidget *page_set_combo;
  BtkWidget *print_now_radio;
  BtkWidget *print_at_radio;
  BtkWidget *print_at_entry;
  BtkWidget *print_hold_radio;
  BtkWidget *preview_button;
  BtkWidget *paper_size_combo;
  BtkWidget *paper_size_combo_label;
  BtkWidget *orientation_combo;
  BtkWidget *orientation_combo_label;
  bboolean internal_page_setup_change;
  bboolean updating_print_at;
  BtkPrinterOptionWidget *pages_per_sheet;
  BtkPrinterOptionWidget *duplex;
  BtkPrinterOptionWidget *paper_type;
  BtkPrinterOptionWidget *paper_source;
  BtkPrinterOptionWidget *output_tray;
  BtkPrinterOptionWidget *job_prio;
  BtkPrinterOptionWidget *billing_info;
  BtkPrinterOptionWidget *cover_before;
  BtkPrinterOptionWidget *cover_after;
  BtkPrinterOptionWidget *number_up_layout;

  BtkWidget *conflicts_widget;

  BtkWidget *job_page;
  BtkWidget *finishing_table;
  BtkWidget *finishing_page;
  BtkWidget *image_quality_table;
  BtkWidget *image_quality_page;
  BtkWidget *color_table;
  BtkWidget *color_page;

  BtkWidget *advanced_vbox;
  BtkWidget *advanced_page;

  BtkWidget *extension_point;

  /* These are set initially on selected printer (either default printer,
   * printer taken from set settings, or user-selected), but when any
   * setting is changed by the user it is cleared.
   */
  BtkPrintSettings *initial_settings;

  BtkPrinterOption *number_up_layout_n_option;
  BtkPrinterOption *number_up_layout_2_option;

  /* This is the initial printer set by set_settings. We look for it in
   * the added printers. We clear this whenever the user manually changes
   * to another printer, when the user changes a setting or when we find
   * this printer.
   */
  char *waiting_for_printer;
  bboolean internal_printer_change;

  GList *print_backends;

  BtkPrinter *current_printer;
  BtkPrinter *request_details_printer;
  buint request_details_tag;
  BtkPrinterOptionSet *options;
  bulong options_changed_handler;
  bulong mark_conflicts_id;

  bchar *format_for_printer;

  bint current_page;
};

G_DEFINE_TYPE_WITH_CODE (BtkPrintUnixDialog, btk_print_unix_dialog, BTK_TYPE_DIALOG,
                         G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
                                                btk_print_unix_dialog_buildable_init))

static BtkBuildableIface *parent_buildable_iface;

static bboolean
is_default_printer (BtkPrintUnixDialog *dialog,
                    BtkPrinter         *printer)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;

  if (priv->format_for_printer)
    return strcmp (priv->format_for_printer,
                   btk_printer_get_name (printer)) == 0;
 else
   return btk_printer_is_default (printer);
}

static void
btk_print_unix_dialog_class_init (BtkPrintUnixDialogClass *class)
{
  BObjectClass *object_class;
  BtkWidgetClass *widget_class;

  object_class = (BObjectClass *) class;
  widget_class = (BtkWidgetClass *) class;

  object_class->finalize = btk_print_unix_dialog_finalize;
  object_class->set_property = btk_print_unix_dialog_set_property;
  object_class->get_property = btk_print_unix_dialog_get_property;

  widget_class->style_set = btk_print_unix_dialog_style_set;

  g_object_class_install_property (object_class,
                                   PROP_PAGE_SETUP,
                                   g_param_spec_object ("page-setup",
                                                        P_("Page Setup"),
                                                        P_("The BtkPageSetup to use"),
                                                        BTK_TYPE_PAGE_SETUP,
                                                        BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_CURRENT_PAGE,
                                   g_param_spec_int ("current-page",
                                                     P_("Current Page"),
                                                     P_("The current page in the document"),
                                                     -1,
                                                     B_MAXINT,
                                                     -1,
                                                     BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_PRINT_SETTINGS,
                                   g_param_spec_object ("print-settings",
                                                        P_("Print Settings"),
                                                        P_("The BtkPrintSettings used for initializing the dialog"),
                                                        BTK_TYPE_PRINT_SETTINGS,
                                                        BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_SELECTED_PRINTER,
                                   g_param_spec_object ("selected-printer",
                                                        P_("Selected Printer"),
                                                        P_("The BtkPrinter which is selected"),
                                                        BTK_TYPE_PRINTER,
                                                        BTK_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_MANUAL_CAPABILITIES,
                                   g_param_spec_flags ("manual-capabilities",
                                                       P_("Manual Capabilites"),
                                                       P_("Capabilities the application can handle"),
                                                       BTK_TYPE_PRINT_CAPABILITIES,
                                                       0,
                                                       BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_SUPPORT_SELECTION,
                                   g_param_spec_boolean ("support-selection",
                                                         P_("Support Selection"),
                                                         P_("Whether the dialog supports selection"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_HAS_SELECTION,
                                   g_param_spec_boolean ("has-selection",
                                                         P_("Has Selection"),
                                                         P_("Whether the application has a selection"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));

   g_object_class_install_property (object_class,
 				   PROP_EMBED_PAGE_SETUP,
 				   g_param_spec_boolean ("embed-page-setup",
 							 P_("Embed Page Setup"),
 							 P_("TRUE if page setup combos are embedded in BtkPrintUnixDialog"),
 							 FALSE,
 							 BTK_PARAM_READWRITE));

  g_type_class_add_private (class, sizeof (BtkPrintUnixDialogPrivate));
}

/* Returns a toplevel BtkWindow, or NULL if none */
static BtkWindow *
get_toplevel (BtkWidget *widget)
{
  BtkWidget *toplevel = NULL;

  toplevel = btk_widget_get_toplevel (widget);
  if (!btk_widget_is_toplevel (toplevel))
    return NULL;
  else
    return BTK_WINDOW (toplevel);
}

static void
set_busy_cursor (BtkPrintUnixDialog *dialog,
		 bboolean            busy)
{
  BtkWindow *toplevel;
  BdkDisplay *display;
  BdkCursor *cursor;

  toplevel = get_toplevel (BTK_WIDGET (dialog));
  if (!toplevel || !btk_widget_get_realized (BTK_WIDGET (toplevel)))
    return;

  display = btk_widget_get_display (BTK_WIDGET (toplevel));

  if (busy)
    cursor = bdk_cursor_new_for_display (display, BDK_WATCH);
  else
    cursor = NULL;

  bdk_window_set_cursor (BTK_WIDGET (toplevel)->window, cursor);
  bdk_display_flush (display);

  if (cursor)
    bdk_cursor_unref (cursor);
}

static void
add_custom_button_to_dialog (BtkDialog   *dialog,
                             const bchar *mnemonic_label,
                             const bchar *stock_id,
                             bint         response_id)
{
  BtkWidget *button = NULL;

  button = btk_button_new_with_mnemonic (mnemonic_label);
  btk_widget_set_can_default (button, TRUE);
  btk_button_set_image (BTK_BUTTON (button),
                        btk_image_new_from_stock (stock_id,
                                                  BTK_ICON_SIZE_BUTTON));
  btk_widget_show (button);

  btk_dialog_add_action_widget (BTK_DIALOG (dialog), button, response_id);
}

/* This function handles error messages before printing.
 */
static bboolean
error_dialogs (BtkPrintUnixDialog *print_dialog,
               bint                print_dialog_response_id,
               bpointer            data)
{
  BtkPrintUnixDialogPrivate *priv = print_dialog->priv;
  BtkPrinterOption          *option = NULL;
  BtkPrinter                *printer = NULL;
  BtkWindow                 *toplevel = NULL;
  BtkWidget                 *dialog = NULL;
  GFile                     *file = NULL;
  bchar                     *basename = NULL;
  bchar                     *dirname = NULL;
  int                        response;

  if (print_dialog != NULL && print_dialog_response_id == BTK_RESPONSE_OK)
    {
      printer = btk_print_unix_dialog_get_selected_printer (print_dialog);

      if (printer != NULL)
        {
          if (priv->request_details_tag || !btk_printer_is_accepting_jobs (printer))
            {
              g_signal_stop_emission_by_name (print_dialog, "response");
              return TRUE;
            }

          /* Shows overwrite confirmation dialog in the case of printing to file which
           * already exists. */
          if (btk_printer_is_virtual (printer))
            {
              option = btk_printer_option_set_lookup (priv->options,
                                                      "btk-main-page-custom-input");

              if (option != NULL &&
                  option->type == BTK_PRINTER_OPTION_TYPE_FILESAVE)
                {
                  file = g_file_new_for_uri (option->value);

                  if (file != NULL &&
                      g_file_query_exists (file, NULL))
                    {
                      toplevel = get_toplevel (BTK_WIDGET (print_dialog));

                      basename = g_file_get_basename (file);
                      dirname = g_file_get_parse_name (g_file_get_parent (file));

                      dialog = btk_message_dialog_new (toplevel,
                                                       BTK_DIALOG_MODAL |
                                                       BTK_DIALOG_DESTROY_WITH_PARENT,
                                                       BTK_MESSAGE_QUESTION,
                                                       BTK_BUTTONS_NONE,
                                                       _("A file named \"%s\" already exists.  Do you want to replace it?"),
                                                       basename);

                      btk_message_dialog_format_secondary_text (BTK_MESSAGE_DIALOG (dialog),
                                                                _("The file already exists in \"%s\".  Replacing it will "
                                                                "overwrite its contents."),
                                                                dirname);

                      btk_dialog_add_button (BTK_DIALOG (dialog),
                                             BTK_STOCK_CANCEL, BTK_RESPONSE_CANCEL);
                      add_custom_button_to_dialog (BTK_DIALOG (dialog),
                                                   _("_Replace"),
                                                   BTK_STOCK_PRINT,
                                                   BTK_RESPONSE_ACCEPT);
                      btk_dialog_set_alternative_button_order (BTK_DIALOG (dialog),
                                                               BTK_RESPONSE_ACCEPT,
                                                               BTK_RESPONSE_CANCEL,
                                                               -1);
                      btk_dialog_set_default_response (BTK_DIALOG (dialog),
                                                       BTK_RESPONSE_ACCEPT);

                      if (toplevel->group)
                        btk_window_group_add_window (toplevel->group,
                                                     BTK_WINDOW (dialog));

                      response = btk_dialog_run (BTK_DIALOG (dialog));

                      btk_widget_destroy (dialog);

                      g_free (dirname);
                      g_free (basename);

                      if (response != BTK_RESPONSE_ACCEPT)
                        {
                          g_signal_stop_emission_by_name (print_dialog, "response");
                          g_object_unref (file);
                          return TRUE;
                        }
                    }

                  g_object_unref (file);
                }
            }
        }
    }
  return FALSE;
}

static void
btk_print_unix_dialog_init (BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;

  priv = dialog->priv = BTK_PRINT_UNIX_DIALOG_GET_PRIVATE (dialog);
  priv->print_backends = NULL;
  priv->current_page = -1;
  priv->number_up_layout_n_option = NULL;
  priv->number_up_layout_2_option = NULL;

  priv->page_setup = btk_page_setup_new ();
  priv->page_setup_set = FALSE;
  priv->embed_page_setup = FALSE;
  priv->internal_page_setup_change = FALSE;

  priv->support_selection = FALSE;
  priv->has_selection = FALSE;

  g_signal_connect (dialog,
                    "destroy",
                    (GCallback) btk_print_unix_dialog_destroy,
                    NULL);

  g_signal_connect (dialog,
                    "response",
                    (GCallback) error_dialogs,
                    NULL);

  g_signal_connect (dialog,
                    "notify::page-setup",
                    (GCallback) redraw_page_layout_preview,
                    NULL);

  priv->preview_button = btk_button_new_from_stock (BTK_STOCK_PRINT_PREVIEW);
  btk_widget_show (priv->preview_button);

  btk_dialog_add_action_widget (BTK_DIALOG (dialog),
                                priv->preview_button,
                                BTK_RESPONSE_APPLY);
  btk_dialog_add_buttons (BTK_DIALOG (dialog),
                          BTK_STOCK_CANCEL, BTK_RESPONSE_CANCEL,
                          BTK_STOCK_PRINT, BTK_RESPONSE_OK,
                          NULL);
  btk_dialog_set_alternative_button_order (BTK_DIALOG (dialog),
                                           BTK_RESPONSE_APPLY,
                                           BTK_RESPONSE_OK,
                                           BTK_RESPONSE_CANCEL,
                                           -1);

  btk_dialog_set_default_response (BTK_DIALOG (dialog), BTK_RESPONSE_OK);
  btk_dialog_set_response_sensitive (BTK_DIALOG (dialog), BTK_RESPONSE_OK, FALSE);

  priv->page_setup_list = btk_list_store_new (PAGE_SETUP_LIST_N_COLS,
                                              B_TYPE_OBJECT,
                                              B_TYPE_BOOLEAN);

  priv->custom_paper_list = btk_list_store_new (1, B_TYPE_OBJECT);
  _btk_print_load_custom_papers (priv->custom_paper_list);

  populate_dialog (dialog);
}

static void
btk_print_unix_dialog_destroy (BtkPrintUnixDialog *dialog)
{
  /* Make sure we don't destroy custom widgets owned by the backends */
  clear_per_printer_ui (dialog);
}

static void
disconnect_printer_details_request (BtkPrintUnixDialog *dialog, bboolean details_failed)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;

  if (priv->request_details_tag)
    {
      g_signal_handler_disconnect (priv->request_details_printer,
                                   priv->request_details_tag);
      priv->request_details_tag = 0;
      set_busy_cursor (dialog, FALSE);
      if (details_failed)
        btk_list_store_set (BTK_LIST_STORE (priv->printer_list),
                            g_object_get_data (B_OBJECT (priv->request_details_printer),
                                               "btk-print-tree-iter"),
                            PRINTER_LIST_COL_STATE,
                             _("Getting printer information failed"),
                            -1);
      else
        btk_list_store_set (BTK_LIST_STORE (priv->printer_list),
                            g_object_get_data (B_OBJECT (priv->request_details_printer),
                                               "btk-print-tree-iter"),
                            PRINTER_LIST_COL_STATE,
                            btk_printer_get_state_message (priv->request_details_printer),
                            -1);
      g_object_unref (priv->request_details_printer);
      priv->request_details_printer = NULL;
    }
}

static void
btk_print_unix_dialog_finalize (BObject *object)
{
  BtkPrintUnixDialog *dialog = BTK_PRINT_UNIX_DIALOG (object);
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  BtkPrintBackend *backend;
  GList *node;

  unschedule_idle_mark_conflicts (dialog);
  disconnect_printer_details_request (dialog, FALSE);

  if (priv->current_printer)
    {
      g_object_unref (priv->current_printer);
      priv->current_printer = NULL;
    }

  if (priv->printer_list)
    {
      g_object_unref (priv->printer_list);
      priv->printer_list = NULL;
    }

  if (priv->custom_paper_list)
    {
      g_object_unref (priv->custom_paper_list);
      priv->custom_paper_list = NULL;
    }

  if (priv->printer_list_filter)
    {
      g_object_unref (priv->printer_list_filter);
      priv->printer_list_filter = NULL;
    }

  if (priv->options)
    {
      g_object_unref (priv->options);
      priv->options = NULL;
    }

  if (priv->number_up_layout_2_option)
    {
      priv->number_up_layout_2_option->choices[0] = NULL;
      priv->number_up_layout_2_option->choices[1] = NULL;
      g_free (priv->number_up_layout_2_option->choices_display[0]);
      g_free (priv->number_up_layout_2_option->choices_display[1]);
      priv->number_up_layout_2_option->choices_display[0] = NULL;
      priv->number_up_layout_2_option->choices_display[1] = NULL;
      g_object_unref (priv->number_up_layout_2_option);
      priv->number_up_layout_2_option = NULL;
    }

  if (priv->number_up_layout_n_option)
    {
      g_object_unref (priv->number_up_layout_n_option);
      priv->number_up_layout_n_option = NULL;
    }

 if (priv->page_setup)
    {
      g_object_unref (priv->page_setup);
      priv->page_setup = NULL;
    }

  if (priv->initial_settings)
    {
      g_object_unref (priv->initial_settings);
      priv->initial_settings = NULL;
    }

  g_free (priv->waiting_for_printer);
  priv->waiting_for_printer = NULL;

  g_free (priv->format_for_printer);
  priv->format_for_printer = NULL;

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

  if (priv->page_setup_list)
    {
      g_object_unref (priv->page_setup_list);
      priv->page_setup_list = NULL;
    }

  B_OBJECT_CLASS (btk_print_unix_dialog_parent_class)->finalize (object);
}

static void
printer_removed_cb (BtkPrintBackend    *backend,
                    BtkPrinter         *printer,
                    BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  BtkTreeIter *iter;

  iter = g_object_get_data (B_OBJECT (printer), "btk-print-tree-iter");
  btk_list_store_remove (BTK_LIST_STORE (priv->printer_list), iter);
}

static void
btk_print_unix_dialog_buildable_init (BtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->get_internal_child = btk_print_unix_dialog_buildable_get_internal_child;
}

static BObject *
btk_print_unix_dialog_buildable_get_internal_child (BtkBuildable *buildable,
                                                    BtkBuilder   *builder,
                                                    const bchar  *childname)
{
  if (strcmp (childname, "notebook") == 0)
    return B_OBJECT (BTK_PRINT_UNIX_DIALOG (buildable)->priv->notebook);

  return parent_buildable_iface->get_internal_child (buildable, builder, childname);
}

/* This function controls "sensitive" property of BtkCellRenderer based on pause
 * state of printers. */
void set_cell_sensitivity_func (BtkTreeViewColumn *tree_column,
                                BtkCellRenderer   *cell,
                                BtkTreeModel      *tree_model,
                                BtkTreeIter       *iter,
                                bpointer           data)
{
  BtkPrinter *printer;

  btk_tree_model_get (tree_model, iter, PRINTER_LIST_COL_PRINTER_OBJ, &printer, -1);

  if (printer != NULL && !btk_printer_is_accepting_jobs (printer))
    g_object_set (cell,
                  "sensitive", FALSE,
                  NULL);
  else
    g_object_set (cell,
                  "sensitive", TRUE,
                  NULL);
}

static void
printer_status_cb (BtkPrintBackend    *backend,
                   BtkPrinter         *printer,
                   BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  BtkTreeIter *iter;
  BtkTreeSelection *selection;

  iter = g_object_get_data (B_OBJECT (printer), "btk-print-tree-iter");

  btk_list_store_set (BTK_LIST_STORE (priv->printer_list), iter,
                      PRINTER_LIST_COL_ICON, btk_printer_get_icon_name (printer),
                      PRINTER_LIST_COL_STATE, btk_printer_get_state_message (printer),
                      PRINTER_LIST_COL_JOBS, btk_printer_get_job_count (printer),
                      PRINTER_LIST_COL_LOCATION, btk_printer_get_location (printer),
                      -1);

  /* When the pause state change then we need to update sensitive property
   * of BTK_RESPONSE_OK button inside of selected_printer_changed function. */
  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (priv->printer_treeview));
  selected_printer_changed (selection, dialog);

  if (btk_print_backend_printer_list_is_done (backend) &&
      btk_printer_is_default (printer) &&
      (btk_tree_selection_count_selected_rows (selection) == 0))
    set_active_printer (dialog, btk_printer_get_name (printer));
}

static void
printer_added_cb (BtkPrintBackend    *backend,
                  BtkPrinter         *printer,
                  BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  BtkTreeIter iter, filter_iter;
  BtkTreeSelection *selection;
  BtkTreePath *path;

  btk_list_store_append (BTK_LIST_STORE (priv->printer_list), &iter);

  g_object_set_data_full (B_OBJECT (printer),
                         "btk-print-tree-iter",
                          btk_tree_iter_copy (&iter),
                          (GDestroyNotify) btk_tree_iter_free);

  btk_list_store_set (BTK_LIST_STORE (priv->printer_list), &iter,
                      PRINTER_LIST_COL_ICON, btk_printer_get_icon_name (printer),
                      PRINTER_LIST_COL_NAME, btk_printer_get_name (printer),
                      PRINTER_LIST_COL_STATE, btk_printer_get_state_message (printer),
                      PRINTER_LIST_COL_JOBS, btk_printer_get_job_count (printer),
                      PRINTER_LIST_COL_LOCATION, btk_printer_get_location (printer),
                      PRINTER_LIST_COL_PRINTER_OBJ, printer,
                      -1);

  btk_tree_model_filter_convert_child_iter_to_iter (priv->printer_list_filter,
                                                    &filter_iter, &iter);
  path = btk_tree_model_get_path (BTK_TREE_MODEL (priv->printer_list_filter), &filter_iter);

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (priv->printer_treeview));

  if (priv->waiting_for_printer != NULL &&
      strcmp (btk_printer_get_name (printer),
              priv->waiting_for_printer) == 0)
    {
      priv->internal_printer_change = TRUE;
      btk_tree_selection_select_iter (selection, &filter_iter);
      btk_tree_view_scroll_to_cell (BTK_TREE_VIEW (priv->printer_treeview),
                                    path, NULL, TRUE, 0.5, 0.0);
      priv->internal_printer_change = FALSE;
      g_free (priv->waiting_for_printer);
      priv->waiting_for_printer = NULL;
    }
  else if (is_default_printer (dialog, printer) &&
           btk_tree_selection_count_selected_rows (selection) == 0)
    {
      priv->internal_printer_change = TRUE;
      btk_tree_selection_select_iter (selection, &filter_iter);
      btk_tree_view_scroll_to_cell (BTK_TREE_VIEW (priv->printer_treeview),
                                    path, NULL, TRUE, 0.5, 0.0);
      priv->internal_printer_change = FALSE;
    }

  btk_tree_path_free (path);
}

static void
printer_list_initialize (BtkPrintUnixDialog *dialog,
                         BtkPrintBackend    *print_backend)
{
  GList *list;
  GList *node;

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
load_print_backends (BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  GList *node;

  if (g_module_supported ())
    priv->print_backends = btk_print_backend_load_modules ();

  for (node = priv->print_backends; node != NULL; node = node->next)
    {
      BtkPrintBackend *backend = node->data;
      printer_list_initialize (dialog, backend);
    }
}

static void
btk_print_unix_dialog_set_property (BObject      *object,
                                    buint         prop_id,
                                    const BValue *value,
                                    BParamSpec   *pspec)

{
  BtkPrintUnixDialog *dialog = BTK_PRINT_UNIX_DIALOG (object);

  switch (prop_id)
    {
    case PROP_PAGE_SETUP:
      btk_print_unix_dialog_set_page_setup (dialog, b_value_get_object (value));
      break;
    case PROP_CURRENT_PAGE:
      btk_print_unix_dialog_set_current_page (dialog, b_value_get_int (value));
      break;
    case PROP_PRINT_SETTINGS:
      btk_print_unix_dialog_set_settings (dialog, b_value_get_object (value));
      break;
    case PROP_MANUAL_CAPABILITIES:
      btk_print_unix_dialog_set_manual_capabilities (dialog, b_value_get_flags (value));
      break;
    case PROP_SUPPORT_SELECTION:
      btk_print_unix_dialog_set_support_selection (dialog, b_value_get_boolean (value));
      break;
    case PROP_HAS_SELECTION:
      btk_print_unix_dialog_set_has_selection (dialog, b_value_get_boolean (value));
      break;
    case PROP_EMBED_PAGE_SETUP:
      btk_print_unix_dialog_set_embed_page_setup (dialog, b_value_get_boolean (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_print_unix_dialog_get_property (BObject    *object,
                                    buint       prop_id,
                                    BValue     *value,
                                    BParamSpec *pspec)
{
  BtkPrintUnixDialog *dialog = BTK_PRINT_UNIX_DIALOG (object);
  BtkPrintUnixDialogPrivate *priv = dialog->priv;

  switch (prop_id)
    {
    case PROP_PAGE_SETUP:
      b_value_set_object (value, priv->page_setup);
      break;
    case PROP_CURRENT_PAGE:
      b_value_set_int (value, priv->current_page);
      break;
    case PROP_PRINT_SETTINGS:
      b_value_take_object (value, btk_print_unix_dialog_get_settings (dialog));
      break;
    case PROP_SELECTED_PRINTER:
      b_value_set_object (value, priv->current_printer);
      break;
    case PROP_MANUAL_CAPABILITIES:
      b_value_set_flags (value, priv->manual_capabilities);
      break;
    case PROP_SUPPORT_SELECTION:
      b_value_set_boolean (value, priv->support_selection);
      break;
    case PROP_HAS_SELECTION:
      b_value_set_boolean (value, priv->has_selection);
      break;
    case PROP_EMBED_PAGE_SETUP:
      b_value_set_boolean (value, priv->embed_page_setup);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static bboolean
is_printer_active (BtkTreeModel       *model,
                   BtkTreeIter        *iter,
                   BtkPrintUnixDialog *dialog)
{
  bboolean result;
  BtkPrinter *printer;
  BtkPrintUnixDialogPrivate *priv = dialog->priv;

  btk_tree_model_get (model,
                      iter,
                      PRINTER_LIST_COL_PRINTER_OBJ,
                      &printer,
                      -1);

  if (printer == NULL)
    return FALSE;

  result = btk_printer_is_active (printer);

  if (result &&
      priv->manual_capabilities & (BTK_PRINT_CAPABILITY_GENERATE_PDF |
                                   BTK_PRINT_CAPABILITY_GENERATE_PS))
    {
       /* Check that the printer can handle at least one of the data
        * formats that the application supports.
        */
       result = ((priv->manual_capabilities & BTK_PRINT_CAPABILITY_GENERATE_PDF) &&
                 btk_printer_accepts_pdf (printer)) ||
                ((priv->manual_capabilities & BTK_PRINT_CAPABILITY_GENERATE_PS) &&
                 btk_printer_accepts_ps (printer));
    }

  g_object_unref (printer);

  return result;
}

static bint
default_printer_list_sort_func (BtkTreeModel *model,
                                BtkTreeIter  *a,
                                BtkTreeIter  *b,
                                bpointer      user_data)
{
  bchar *a_name;
  bchar *b_name;
  BtkPrinter *a_printer;
  BtkPrinter *b_printer;
  bint result;

  btk_tree_model_get (model, a,
                      PRINTER_LIST_COL_NAME, &a_name,
                      PRINTER_LIST_COL_PRINTER_OBJ, &a_printer,
                      -1);
  btk_tree_model_get (model, b,
                      PRINTER_LIST_COL_NAME, &b_name,
                      PRINTER_LIST_COL_PRINTER_OBJ, &b_printer,
                      -1);

  if (a_printer == NULL && b_printer == NULL)
    result = 0;
  else if (a_printer == NULL)
   result = B_MAXINT;
  else if (b_printer == NULL)
   result = B_MININT;
  else if (btk_printer_is_virtual (a_printer) && btk_printer_is_virtual (b_printer))
    result = 0;
  else if (btk_printer_is_virtual (a_printer) && !btk_printer_is_virtual (b_printer))
    result = B_MININT;
  else if (!btk_printer_is_virtual (a_printer) && btk_printer_is_virtual (b_printer))
    result = B_MAXINT;
  else if (a_name == NULL && b_name == NULL)
    result = 0;
  else if (a_name == NULL && b_name != NULL)
    result = 1;
  else if (a_name != NULL && b_name == NULL)
    result = -1;
  else
    result = g_ascii_strcasecmp (a_name, b_name);

  g_free (a_name);
  g_free (b_name);
  g_object_unref (a_printer);
  g_object_unref (b_printer);

  return result;
}

static void
create_printer_list_model (BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  BtkListStore *model;
  BtkTreeSortable *sort;

  model = btk_list_store_new (PRINTER_LIST_N_COLS,
                              B_TYPE_STRING,
                              B_TYPE_STRING,
                              B_TYPE_STRING,
                              B_TYPE_INT,
                              B_TYPE_STRING,
                              B_TYPE_OBJECT);

  priv->printer_list = (BtkTreeModel *)model;
  priv->printer_list_filter = (BtkTreeModelFilter *) btk_tree_model_filter_new ((BtkTreeModel *)model,
                                                                                        NULL);

  btk_tree_model_filter_set_visible_func (priv->printer_list_filter,
                                          (BtkTreeModelFilterVisibleFunc) is_printer_active,
                                          dialog,
                                          NULL);

  sort = BTK_TREE_SORTABLE (model);
  btk_tree_sortable_set_default_sort_func (sort,
                                           default_printer_list_sort_func,
                                           NULL,
                                           NULL);

  btk_tree_sortable_set_sort_column_id (sort,
                                        BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                                        BTK_SORT_ASCENDING);

}


static BtkWidget *
wrap_in_frame (const bchar *label,
               BtkWidget   *child)
{
  BtkWidget *frame, *alignment, *label_widget;
  bchar *bold_text;

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

static bboolean
setup_option (BtkPrintUnixDialog     *dialog,
              const bchar            *option_name,
              BtkPrinterOptionWidget *widget)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  BtkPrinterOption *option;

  option = btk_printer_option_set_lookup (priv->options, option_name);
  btk_printer_option_widget_set_source (widget, option);

  return option != NULL;
}

static void
add_option_to_extension_point (BtkPrinterOption *option,
                               bpointer          data)
{
  BtkWidget *extension_point = data;
  BtkWidget *widget;

  widget = btk_printer_option_widget_new (option);
  btk_widget_show (widget);

  if (btk_printer_option_widget_has_external_label (BTK_PRINTER_OPTION_WIDGET (widget)))
    {
      BtkWidget *label, *hbox;

      label = btk_printer_option_widget_get_external_label (BTK_PRINTER_OPTION_WIDGET (widget));
      btk_widget_show (label);
      btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
      btk_label_set_mnemonic_widget (BTK_LABEL (label), widget);

      hbox = btk_hbox_new (FALSE, 12);
      btk_box_pack_start (BTK_BOX (hbox), label, FALSE, FALSE, 0);
      btk_box_pack_start (BTK_BOX (hbox), widget, FALSE, FALSE, 0);
      btk_widget_show (hbox);

      btk_box_pack_start (BTK_BOX (extension_point), hbox, FALSE, FALSE, 0);
    }
  else
    btk_box_pack_start (BTK_BOX (extension_point), widget, FALSE, FALSE, 0);
}

static void
add_option_to_table (BtkPrinterOption *option,
                     bpointer          user_data)
{
  BtkTable *table;
  BtkWidget *label, *widget;
  bint row;

  table = BTK_TABLE (user_data);

  if (g_str_has_prefix (option->name, "btk-"))
    return;

  widget = btk_printer_option_widget_new (option);
  btk_widget_show (widget);

  row = table->nrows;
  btk_table_resize (table, table->nrows + 1, 2);

  if (btk_printer_option_widget_has_external_label (BTK_PRINTER_OPTION_WIDGET (widget)))
    {
      label = btk_printer_option_widget_get_external_label (BTK_PRINTER_OPTION_WIDGET (widget));
      btk_widget_show (label);

      btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
      btk_label_set_mnemonic_widget (BTK_LABEL (label), widget);

      btk_table_attach (table, label,
                        0, 1, row - 1 , row,  BTK_FILL, 0, 0, 0);

      btk_table_attach (table, widget,
                        1, 2, row - 1, row,  BTK_FILL, 0, 0, 0);
    }
  else
    btk_table_attach (table, widget,
                      0, 2, row - 1, row,  BTK_FILL, 0, 0, 0);
}

static void
setup_page_table (BtkPrinterOptionSet *options,
                  const bchar         *group,
                  BtkWidget           *table,
                  BtkWidget           *page)
{
  btk_printer_option_set_foreach_in_group (options, group,
                                           add_option_to_table,
                                           table);
  if (BTK_TABLE (table)->nrows == 1)
    btk_widget_hide (page);
  else
    btk_widget_show (page);
}

static void
update_print_at_option (BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  BtkPrinterOption *option;

  option = btk_printer_option_set_lookup (priv->options, "btk-print-time");

  if (option == NULL)
    return;

  if (priv->updating_print_at)
    return;

  if (btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (priv->print_at_radio)))
    btk_printer_option_set (option, "at");
  else if (btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (priv->print_hold_radio)))
    btk_printer_option_set (option, "on-hold");
  else
    btk_printer_option_set (option, "now");

  option = btk_printer_option_set_lookup (priv->options, "btk-print-time-text");
  if (option != NULL)
    {
      const char *text = btk_entry_get_text (BTK_ENTRY (priv->print_at_entry));
      btk_printer_option_set (option, text);
    }
}


static bboolean
setup_print_at (BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  BtkPrinterOption *option;

  option = btk_printer_option_set_lookup (priv->options, "btk-print-time");

  if (option == NULL)
    {
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (priv->print_now_radio),
                                    TRUE);
      btk_widget_set_sensitive (priv->print_at_radio, FALSE);
      btk_widget_set_sensitive (priv->print_at_entry, FALSE);
      btk_widget_set_sensitive (priv->print_hold_radio, FALSE);
      btk_entry_set_text (BTK_ENTRY (priv->print_at_entry), "");
      return FALSE;
    }

  priv->updating_print_at = TRUE;

  btk_widget_set_sensitive (priv->print_at_entry, FALSE);
  btk_widget_set_sensitive (priv->print_at_radio,
                            btk_printer_option_has_choice (option, "at"));

  btk_widget_set_sensitive (priv->print_hold_radio,
                            btk_printer_option_has_choice (option, "on-hold"));

  update_print_at_option (dialog);

  if (strcmp (option->value, "at") == 0)
    btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (priv->print_at_radio),
                                  TRUE);
  else if (strcmp (option->value, "on-hold") == 0)
    btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (priv->print_hold_radio),
                                  TRUE);
  else
    btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (priv->print_now_radio),
                                  TRUE);

  option = btk_printer_option_set_lookup (priv->options, "btk-print-time-text");
  if (option != NULL)
    btk_entry_set_text (BTK_ENTRY (priv->print_at_entry), option->value);

  priv->updating_print_at = FALSE;

  return TRUE;
}

static void
update_dialog_from_settings (BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  GList *groups, *l;
  bchar *group;
  BtkWidget *table, *frame;
  bboolean has_advanced, has_job;

  if (priv->current_printer == NULL)
    {
       clear_per_printer_ui (dialog);
       btk_widget_hide (priv->job_page);
       btk_widget_hide (priv->advanced_page);
       btk_widget_hide (priv->image_quality_page);
       btk_widget_hide (priv->finishing_page);
       btk_widget_hide (priv->color_page);
       btk_dialog_set_response_sensitive (BTK_DIALOG (dialog), BTK_RESPONSE_OK, FALSE);

       return;
    }

  setup_option (dialog, "btk-n-up", priv->pages_per_sheet);
  setup_option (dialog, "btk-n-up-layout", priv->number_up_layout);
  setup_option (dialog, "btk-duplex", priv->duplex);
  setup_option (dialog, "btk-paper-type", priv->paper_type);
  setup_option (dialog, "btk-paper-source", priv->paper_source);
  setup_option (dialog, "btk-output-tray", priv->output_tray);

  has_job = FALSE;
  has_job |= setup_option (dialog, "btk-job-prio", priv->job_prio);
  has_job |= setup_option (dialog, "btk-billing-info", priv->billing_info);
  has_job |= setup_option (dialog, "btk-cover-before", priv->cover_before);
  has_job |= setup_option (dialog, "btk-cover-after", priv->cover_after);
  has_job |= setup_print_at (dialog);

  if (has_job)
    btk_widget_show (priv->job_page);
  else
    btk_widget_hide (priv->job_page);

  setup_page_table (priv->options,
                    "ImageQualityPage",
                    priv->image_quality_table,
                    priv->image_quality_page);

  setup_page_table (priv->options,
                    "FinishingPage",
                    priv->finishing_table,
                    priv->finishing_page);

  setup_page_table (priv->options,
                    "ColorPage",
                    priv->color_table,
                    priv->color_page);

  btk_printer_option_set_foreach_in_group (priv->options,
                                           "BtkPrintDialogExtension",
                                           add_option_to_extension_point,
                                           priv->extension_point);

  /* Put the rest of the groups in the advanced page */
  groups = btk_printer_option_set_get_groups (priv->options);

  has_advanced = FALSE;
  for (l = groups; l != NULL; l = l->next)
    {
      group = l->data;

      if (group == NULL)
        continue;

      if (strcmp (group, "ImageQualityPage") == 0 ||
          strcmp (group, "ColorPage") == 0 ||
          strcmp (group, "FinishingPage") == 0 ||
          strcmp (group, "BtkPrintDialogExtension") == 0)
        continue;

      table = btk_table_new (1, 2, FALSE);
      btk_table_set_row_spacings (BTK_TABLE (table), 6);
      btk_table_set_col_spacings (BTK_TABLE (table), 12);

      btk_printer_option_set_foreach_in_group (priv->options,
                                               group,
                                               add_option_to_table,
                                               table);
      if (BTK_TABLE (table)->nrows == 1)
        btk_widget_destroy (table);
      else
        {
          has_advanced = TRUE;
          frame = wrap_in_frame (group, table);
          btk_widget_show (table);
          btk_widget_show (frame);

          btk_box_pack_start (BTK_BOX (priv->advanced_vbox),
                              frame, FALSE, FALSE, 0);
        }
    }

  if (has_advanced)
    btk_widget_show (priv->advanced_page);
  else
    btk_widget_hide (priv->advanced_page);

  g_list_foreach (groups, (GFunc) g_free, NULL);
  g_list_free (groups);
}

static void
update_dialog_from_capabilities (BtkPrintUnixDialog *dialog)
{
  BtkPrintCapabilities caps;
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  bboolean can_collate;
  const bchar *copies;

  copies = btk_entry_get_text (BTK_ENTRY (priv->copies_spin));
  can_collate = (*copies != '\0' && atoi (copies) > 1);

  caps = priv->manual_capabilities | priv->printer_capabilities;

  btk_widget_set_sensitive (priv->page_set_combo,
                            caps & BTK_PRINT_CAPABILITY_PAGE_SET);
  btk_widget_set_sensitive (priv->copies_spin,
                            caps & BTK_PRINT_CAPABILITY_COPIES);
  btk_widget_set_sensitive (priv->collate_check,
                            can_collate &&
                            (caps & BTK_PRINT_CAPABILITY_COLLATE));
  btk_widget_set_sensitive (priv->reverse_check,
                            caps & BTK_PRINT_CAPABILITY_REVERSE);
  btk_widget_set_sensitive (priv->scale_spin,
                            caps & BTK_PRINT_CAPABILITY_SCALE);
  btk_widget_set_sensitive (BTK_WIDGET (priv->pages_per_sheet),
                            caps & BTK_PRINT_CAPABILITY_NUMBER_UP);

  if (caps & BTK_PRINT_CAPABILITY_PREVIEW)
    btk_widget_show (priv->preview_button);
  else
    btk_widget_hide (priv->preview_button);

  update_collate_icon (NULL, dialog);

  btk_tree_model_filter_refilter (priv->printer_list_filter);
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
set_paper_size (BtkPrintUnixDialog *dialog,
		BtkPageSetup       *page_setup,
		bboolean            size_only,
		bboolean            add_item)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  BtkTreeModel *model;
  BtkTreeIter iter;
  BtkPageSetup *list_page_setup;

  if (!priv->internal_page_setup_change)
    return TRUE;

  if (page_setup == NULL)
    return FALSE;

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
	      btk_combo_box_set_active (BTK_COMBO_BOX (priv->orientation_combo),
					btk_page_setup_get_orientation (page_setup));
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
      btk_combo_box_set_active (BTK_COMBO_BOX (priv->orientation_combo),
				btk_page_setup_get_orientation (page_setup));
      return TRUE;
    }

  return FALSE;
}

static void
fill_custom_paper_sizes (BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
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
fill_paper_sizes (BtkPrintUnixDialog *dialog,
                  BtkPrinter         *printer)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  GList *list, *l;
  BtkPageSetup *page_setup;
  BtkPaperSize *paper_size;
  BtkTreeIter iter;
  bint i;

  btk_list_store_clear (priv->page_setup_list);

  if (printer == NULL || (list = btk_printer_list_papers (printer)) == NULL)
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
}

static void
update_paper_sizes (BtkPrintUnixDialog *dialog)
{
  BtkPageSetup *current_page_setup = NULL;
  BtkPrinter   *printer;

  printer = btk_print_unix_dialog_get_selected_printer (dialog);

  fill_paper_sizes (dialog, printer);

  current_page_setup = btk_page_setup_copy (btk_print_unix_dialog_get_page_setup (dialog));

  if (current_page_setup)
    {
      if (!set_paper_size (dialog, current_page_setup, FALSE, FALSE))
        set_paper_size (dialog, current_page_setup, TRUE, TRUE);

      g_object_unref (current_page_setup);
    }
}

static void
mark_conflicts (BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  BtkPrinter *printer;
  bboolean have_conflict;

  have_conflict = FALSE;

  printer = priv->current_printer;

  if (printer)
    {

      g_signal_handler_block (priv->options,
                              priv->options_changed_handler);

      btk_printer_option_set_clear_conflicts (priv->options);

      have_conflict = _btk_printer_mark_conflicts (printer,
                                                   priv->options);

      g_signal_handler_unblock (priv->options,
                                priv->options_changed_handler);
    }

  if (have_conflict)
    btk_widget_show (priv->conflicts_widget);
  else
    btk_widget_hide (priv->conflicts_widget);
}

static bboolean
mark_conflicts_callback (bpointer data)
{
  BtkPrintUnixDialog *dialog = data;
  BtkPrintUnixDialogPrivate *priv = dialog->priv;

  priv->mark_conflicts_id = 0;

  mark_conflicts (dialog);

  return FALSE;
}

static void
unschedule_idle_mark_conflicts (BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;

  if (priv->mark_conflicts_id != 0)
    {
      g_source_remove (priv->mark_conflicts_id);
      priv->mark_conflicts_id = 0;
    }
}

static void
schedule_idle_mark_conflicts (BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;

  if (priv->mark_conflicts_id != 0)
    return;

  priv->mark_conflicts_id = bdk_threads_add_idle (mark_conflicts_callback,
                                        dialog);
}

static void
options_changed_cb (BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;

  schedule_idle_mark_conflicts (dialog);

  g_free (priv->waiting_for_printer);
  priv->waiting_for_printer = NULL;
}

static void
remove_custom_widget (BtkWidget    *widget,
                      BtkContainer *container)
{
  btk_container_remove (container, widget);
}

static void
extension_point_clear_children (BtkContainer *container)
{
  btk_container_foreach (container,
                         (BtkCallback)remove_custom_widget,
                         container);
}

static void
clear_per_printer_ui (BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;

  btk_container_foreach (BTK_CONTAINER (priv->finishing_table),
                         (BtkCallback)btk_widget_destroy,
                         NULL);
  btk_table_resize (BTK_TABLE (priv->finishing_table), 1, 2);
  btk_container_foreach (BTK_CONTAINER (priv->image_quality_table),
                         (BtkCallback)btk_widget_destroy,
                         NULL);
  btk_table_resize (BTK_TABLE (priv->image_quality_table), 1, 2);
  btk_container_foreach (BTK_CONTAINER (priv->color_table),
                         (BtkCallback)btk_widget_destroy,
                         NULL);
  btk_table_resize (BTK_TABLE (priv->color_table), 1, 2);
  btk_container_foreach (BTK_CONTAINER (priv->advanced_vbox),
                         (BtkCallback)btk_widget_destroy,
                         NULL);
  extension_point_clear_children (BTK_CONTAINER (priv->extension_point));
}

static void
printer_details_acquired (BtkPrinter         *printer,
                          bboolean            success,
                          BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;

  disconnect_printer_details_request (dialog, !success);

  if (success)
    {
      BtkTreeSelection *selection;
      selection = btk_tree_view_get_selection (BTK_TREE_VIEW (priv->printer_treeview));

      selected_printer_changed (selection, dialog);
    }
}

static void
selected_printer_changed (BtkTreeSelection   *selection,
                          BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  BtkPrinter *printer;
  BtkTreeIter iter, filter_iter;

  /* Whenever the user selects a printer we stop looking for
     the printer specified in the initial settings */
  if (priv->waiting_for_printer &&
      !priv->internal_printer_change)
    {
      g_free (priv->waiting_for_printer);
      priv->waiting_for_printer = NULL;
    }

  disconnect_printer_details_request (dialog, FALSE);

  printer = NULL;
  if (btk_tree_selection_get_selected (selection, NULL, &filter_iter))
    {
      btk_tree_model_filter_convert_iter_to_child_iter (priv->printer_list_filter,
                                                        &iter,
                                                        &filter_iter);

      btk_tree_model_get (priv->printer_list, &iter,
                          PRINTER_LIST_COL_PRINTER_OBJ, &printer,
                          -1);
    }

  /* sets BTK_RESPONSE_OK button sensitivity depending on whether the printer
   * accepts/rejects jobs
   */
  if (printer != NULL)
    {
      if (!btk_printer_is_accepting_jobs (printer))
        {
          btk_dialog_set_response_sensitive (BTK_DIALOG (dialog), BTK_RESPONSE_OK, FALSE);
        }
      else
        {
          if (priv->current_printer == printer && btk_printer_has_details (printer))
            btk_dialog_set_response_sensitive (BTK_DIALOG (dialog), BTK_RESPONSE_OK, TRUE);
        }
    }

  if (printer != NULL && !btk_printer_has_details (printer))
    {
      btk_dialog_set_response_sensitive (BTK_DIALOG (dialog), BTK_RESPONSE_OK, FALSE);
      priv->request_details_tag =
        g_signal_connect (printer, "details-acquired",
                          G_CALLBACK (printer_details_acquired), dialog);
      /* take the reference */
      priv->request_details_printer = printer;
      set_busy_cursor (dialog, TRUE);
      btk_list_store_set (BTK_LIST_STORE (priv->printer_list),
                          g_object_get_data (B_OBJECT (printer), "btk-print-tree-iter"),
                          PRINTER_LIST_COL_STATE, _("Getting printer information..."),
                          -1);
      btk_printer_request_details (printer);
      return;
    }

  if (printer == priv->current_printer)
    {
      if (printer)
        g_object_unref (printer);
      return;
    }

  if (priv->options)
    {
      g_object_unref (priv->options);
      priv->options = NULL;

      clear_per_printer_ui (dialog);
    }

  if (priv->current_printer)
    {
      g_object_unref (priv->current_printer);
    }

  priv->printer_capabilities = 0;

  if (btk_printer_is_accepting_jobs (printer))
    btk_dialog_set_response_sensitive (BTK_DIALOG (dialog), BTK_RESPONSE_OK, TRUE);
  priv->current_printer = printer;

  if (printer != NULL)
    {
      if (!priv->page_setup_set)
        {
          /* if no explicit page setup has been set, use the printer default */
          BtkPageSetup *page_setup;

          page_setup = btk_printer_get_default_page_size (printer);

          if (!page_setup)
            page_setup = btk_page_setup_new ();

          if (page_setup && priv->page_setup)
            btk_page_setup_set_orientation (page_setup, btk_page_setup_get_orientation (priv->page_setup));

          g_object_unref (priv->page_setup);
          priv->page_setup = page_setup;
        }

      priv->printer_capabilities = btk_printer_get_capabilities (printer);
      priv->options = _btk_printer_get_options (printer,
                                                priv->initial_settings,
                                                priv->page_setup,
                                                priv->manual_capabilities);

      priv->options_changed_handler =
        g_signal_connect_swapped (priv->options, "changed", G_CALLBACK (options_changed_cb), dialog);
    }

  update_dialog_from_settings (dialog);
  update_dialog_from_capabilities (dialog);

  priv->internal_page_setup_change = TRUE;
  update_paper_sizes (dialog);
  priv->internal_page_setup_change = FALSE;

  g_object_notify ( B_OBJECT(dialog), "selected-printer");
}

static void
update_collate_icon (BtkToggleButton    *toggle_button,
                     BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;

  btk_widget_queue_draw (priv->collate_image);
}

static void
paint_page (BtkWidget *widget,
            bairo_t   *cr,
            bfloat     scale,
            bint       x_offset,
            bint       y_offset,
            bchar     *text,
            bint       text_x)
{
  bint x, y, width, height;
  bint text_y, linewidth;

  x = x_offset * scale;
  y = y_offset * scale;
  width = 20 * scale;
  height = 26 * scale;

  linewidth = 2;
  text_y = 21;

  bdk_bairo_set_source_color (cr, &widget->style->base[BTK_STATE_NORMAL]);
  bairo_rectangle (cr, x, y, width, height);
  bairo_fill (cr);

  bdk_bairo_set_source_color (cr, &widget->style->text[BTK_STATE_NORMAL]);
  bairo_set_line_width (cr, linewidth);
  bairo_rectangle (cr, x + linewidth/2.0, y + linewidth/2.0, width - linewidth, height - linewidth);
  bairo_stroke (cr);

  bairo_select_font_face (cr, "Sans",
                          BAIRO_FONT_SLANT_NORMAL,
                          BAIRO_FONT_WEIGHT_NORMAL);
  bairo_set_font_size (cr, (bint)(9 * scale));
  bairo_move_to (cr, x + (bint)(text_x * scale), y + (bint)(text_y * scale));
  bairo_show_text (cr, text);
}

static bboolean
draw_collate_cb (BtkWidget          *widget,
                 BdkEventExpose     *event,
                 BtkPrintUnixDialog *dialog)
{
  BtkSettings *settings;
  bairo_t *cr;
  bint size;
  bfloat scale;
  bboolean collate, reverse, rtl;
  bint copies;
  bint text_x;

  collate = dialog_get_collate (dialog);
  reverse = dialog_get_reverse (dialog);
  copies = dialog_get_n_copies (dialog);

  rtl = (btk_widget_get_direction (BTK_WIDGET (widget)) == BTK_TEXT_DIR_RTL);

  settings = btk_widget_get_settings (widget);
  btk_icon_size_lookup_for_settings (settings,
                                     BTK_ICON_SIZE_DIALOG,
                                     &size,
                                     NULL);
  scale = size / 48.0;
  text_x = rtl ? 4 : 11;

  cr = bdk_bairo_create (widget->window);

  bairo_translate (cr, widget->allocation.x, widget->allocation.y);

  if (copies == 1)
    {
      paint_page (widget, cr, scale, rtl ? 40: 15, 5, reverse ? "1" : "2", text_x);
      paint_page (widget, cr, scale, rtl ? 50: 5, 15, reverse ? "2" : "1", text_x);
    }
  else
    {
      paint_page (widget, cr, scale, rtl ? 40: 15, 5, collate == reverse ? "1" : "2", text_x);
      paint_page (widget, cr, scale, rtl ? 50: 5, 15, reverse ? "2" : "1", text_x);

      paint_page (widget, cr, scale, rtl ? 5 : 50, 5, reverse ? "1" : "2", text_x);
      paint_page (widget, cr, scale, rtl ? 15 : 40, 15, collate == reverse ? "2" : "1", text_x);
    }

  bairo_destroy (cr);

  return TRUE;
}

static void
btk_print_unix_dialog_style_set (BtkWidget *widget,
                                 BtkStyle  *previous_style)
{
  BTK_WIDGET_CLASS (btk_print_unix_dialog_parent_class)->style_set (widget, previous_style);

  if (btk_widget_has_screen (widget))
    {
      BtkPrintUnixDialog *dialog = (BtkPrintUnixDialog *)widget;
      BtkPrintUnixDialogPrivate *priv = dialog->priv;
      BtkSettings *settings;
      bint size;
      bfloat scale;

      settings = btk_widget_get_settings (widget);
      btk_icon_size_lookup_for_settings (settings,
                                         BTK_ICON_SIZE_DIALOG,
                                         &size,
                                         NULL);
      scale = size / 48.0;

      btk_widget_set_size_request (priv->collate_image,
                                   (50 + 20) * scale,
                                   (15 + 26) * scale);
    }
}

static void
update_entry_sensitivity (BtkWidget *button,
                          BtkWidget *range)
{
  bboolean active;

  active = btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (button));

  btk_widget_set_sensitive (range, active);

  if (active)
    btk_widget_grab_focus (range);
}

static void
emit_ok_response (BtkTreeView       *tree_view,
                  BtkTreePath       *path,
                  BtkTreeViewColumn *column,
                  bpointer          *user_data)
{
  BtkPrintUnixDialog *print_dialog;

  print_dialog = (BtkPrintUnixDialog *) user_data;

  btk_dialog_response (BTK_DIALOG (print_dialog), BTK_RESPONSE_OK);
}

static void
create_main_page (BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  BtkWidget *main_vbox, *label, *vbox, *hbox;
  BtkWidget *scrolled, *treeview, *frame, *table;
  BtkWidget *entry, *spinbutton;
  BtkWidget *radio, *check, *image;
  BtkCellRenderer *renderer;
  BtkTreeViewColumn *column;
  BtkTreeSelection *selection;
  BtkWidget *custom_input;
  const bchar *range_tooltip;

  main_vbox = btk_vbox_new (FALSE, 18);
  btk_container_set_border_width (BTK_CONTAINER (main_vbox), 12);
  btk_widget_show (main_vbox);

  vbox = btk_vbox_new (FALSE, 6);
  btk_box_pack_start (BTK_BOX (main_vbox), vbox, TRUE, TRUE, 0);
  btk_widget_show (vbox);

  scrolled = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled),
                                  BTK_POLICY_AUTOMATIC,
                                  BTK_POLICY_AUTOMATIC);
  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (scrolled),
                                       BTK_SHADOW_IN);
  btk_widget_show (scrolled);
  btk_box_pack_start (BTK_BOX (vbox), scrolled, TRUE, TRUE, 0);

  treeview = btk_tree_view_new_with_model ((BtkTreeModel *) priv->printer_list_filter);
  priv->printer_treeview = treeview;
  btk_tree_view_set_headers_visible (BTK_TREE_VIEW (treeview), TRUE);
  btk_tree_view_set_search_column (BTK_TREE_VIEW (treeview), PRINTER_LIST_COL_NAME);
  btk_tree_view_set_enable_search (BTK_TREE_VIEW (treeview), TRUE);
  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (treeview));
  btk_tree_selection_set_mode (selection, BTK_SELECTION_BROWSE);
  g_signal_connect (selection, "changed", G_CALLBACK (selected_printer_changed), dialog);

  renderer = btk_cell_renderer_pixbuf_new ();
  column = btk_tree_view_column_new_with_attributes ("",
                                                     renderer,
                                                     "icon-name",
                                                     PRINTER_LIST_COL_ICON,
                                                     NULL);
  btk_tree_view_column_set_cell_data_func (column, renderer, set_cell_sensitivity_func, NULL, NULL);
  btk_tree_view_append_column (BTK_TREE_VIEW (treeview), column);

  renderer = btk_cell_renderer_text_new ();
  column = btk_tree_view_column_new_with_attributes (_("Printer"),
                                                     renderer,
                                                     "text",
                                                     PRINTER_LIST_COL_NAME,
                                                     NULL);
  btk_tree_view_column_set_cell_data_func (column, renderer, set_cell_sensitivity_func, NULL, NULL);
  btk_tree_view_append_column (BTK_TREE_VIEW (treeview), column);

  renderer = btk_cell_renderer_text_new ();
  /* Translators: this is the header for the location column in the print dialog */
  column = btk_tree_view_column_new_with_attributes (_("Location"),
                                                     renderer,
                                                     "text",
                                                     PRINTER_LIST_COL_LOCATION,
                                                     NULL);
  btk_tree_view_column_set_cell_data_func (column, renderer, set_cell_sensitivity_func, NULL, NULL);
  btk_tree_view_append_column (BTK_TREE_VIEW (treeview), column);

  renderer = btk_cell_renderer_text_new ();
  g_object_set (renderer, "ellipsize", BANGO_ELLIPSIZE_END, NULL);
  /* Translators: this is the header for the printer status column in the print dialog */
  column = btk_tree_view_column_new_with_attributes (_("Status"),
                                                     renderer,
                                                     "text",
                                                     PRINTER_LIST_COL_STATE,
                                                     NULL);
  btk_tree_view_column_set_cell_data_func (column, renderer, set_cell_sensitivity_func, NULL, NULL);
  btk_tree_view_append_column (BTK_TREE_VIEW (treeview), column);

  g_signal_connect (BTK_TREE_VIEW (treeview), "row-activated", G_CALLBACK (emit_ok_response), dialog);

  btk_widget_show (treeview);
  btk_container_add (BTK_CONTAINER (scrolled), treeview);

  custom_input = btk_hbox_new (FALSE, 18);
  btk_widget_show (custom_input);
  btk_box_pack_start (BTK_BOX (vbox), custom_input, FALSE, FALSE, 0);
  priv->extension_point = custom_input;

  hbox = btk_hbox_new (FALSE, 18);
  btk_widget_show (hbox);
  btk_box_pack_start (BTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

  table = btk_table_new (4, 2, FALSE);
  priv->range_table = table;
  btk_table_set_row_spacings (BTK_TABLE (table), 6);
  btk_table_set_col_spacings (BTK_TABLE (table), 12);
  frame = wrap_in_frame (_("Range"), table);
  btk_box_pack_start (BTK_BOX (hbox), frame, TRUE, TRUE, 0);
  btk_widget_show (table);

  radio = btk_radio_button_new_with_mnemonic (NULL, _("_All Pages"));
  priv->all_pages_radio = radio;
  btk_widget_show (radio);
  btk_table_attach (BTK_TABLE (table), radio,
                    0, 2, 0, 1,  BTK_FILL, 0,
                    0, 0);
  radio = btk_radio_button_new_with_mnemonic (btk_radio_button_get_group (BTK_RADIO_BUTTON (radio)),
                                              _("C_urrent Page"));
  if (priv->current_page == -1)
    btk_widget_set_sensitive (radio, FALSE);
  priv->current_page_radio = radio;
  btk_widget_show (radio);
  btk_table_attach (BTK_TABLE (table), radio,
                    0, 2, 1, 2,  BTK_FILL, 0,
                    0, 0);

  radio = btk_radio_button_new_with_mnemonic (btk_radio_button_get_group (BTK_RADIO_BUTTON (radio)),
                                              _("Se_lection"));

  btk_widget_set_sensitive (radio, priv->has_selection);
  priv->selection_radio = radio;
  btk_table_attach (BTK_TABLE (table), radio,
                    0, 2, 2, 3,  BTK_FILL, 0,
                    0, 0);
  btk_table_set_row_spacing (BTK_TABLE (table), 2, 0);

  radio = btk_radio_button_new_with_mnemonic (btk_radio_button_get_group (BTK_RADIO_BUTTON (radio)), _("Pag_es:"));
  range_tooltip = _("Specify one or more page ranges,\n e.g. 1-3,7,11");
  btk_widget_set_tooltip_text (radio, range_tooltip);

  priv->page_range_radio = radio;
  btk_widget_show (radio);
  btk_table_attach (BTK_TABLE (table), radio,
                    0, 1, 3, 4,  BTK_FILL, 0,
                    0, 0);
  entry = btk_entry_new ();
  btk_widget_set_tooltip_text (entry, range_tooltip);
  btk_entry_set_activates_default (BTK_ENTRY (entry), TRUE);
  batk_object_set_name (btk_widget_get_accessible (entry), _("Pages"));
  batk_object_set_description (btk_widget_get_accessible (entry), range_tooltip);
  priv->page_range_entry = entry;
  btk_widget_show (entry);
  btk_table_attach (BTK_TABLE (table), entry,
                    1, 2, 3, 4,  BTK_FILL, 0,
                    0, 0);
  g_signal_connect (radio, "toggled", G_CALLBACK (update_entry_sensitivity), entry);
  update_entry_sensitivity (radio, entry);

  table = btk_table_new (3, 2, FALSE);
  btk_table_set_row_spacings (BTK_TABLE (table), 6);
  btk_table_set_col_spacings (BTK_TABLE (table), 12);
  frame = wrap_in_frame (_("Copies"), table);
  btk_box_pack_start (BTK_BOX (hbox), frame, TRUE, TRUE, 0);
  btk_widget_show (table);

  /* FIXME chpe: too much space between Copies and spinbutton, put those 2 in a hbox and make it span 2 columns */
  label = btk_label_new_with_mnemonic (_("Copie_s:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_widget_show (label);
  btk_table_attach (BTK_TABLE (table), label,
                    0, 1, 0, 1,  BTK_FILL, 0,
                    0, 0);
  spinbutton = btk_spin_button_new_with_range (1.0, 100.0, 1.0);
  btk_entry_set_activates_default (BTK_ENTRY (spinbutton), TRUE);
  priv->copies_spin = spinbutton;
  btk_widget_show (spinbutton);
  btk_table_attach (BTK_TABLE (table), spinbutton,
                    1, 2, 0, 1,  BTK_FILL, 0,
                    0, 0);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), spinbutton);
  g_signal_connect_swapped (spinbutton, "value-changed",
                            G_CALLBACK (update_dialog_from_capabilities), dialog);
  g_signal_connect_swapped (spinbutton, "changed",
                            G_CALLBACK (update_dialog_from_capabilities), dialog);

  check = btk_check_button_new_with_mnemonic (_("C_ollate"));
  priv->collate_check = check;
  g_signal_connect (check, "toggled", G_CALLBACK (update_collate_icon), dialog);
  btk_widget_show (check);
  btk_table_attach (BTK_TABLE (table), check,
                    0, 1, 1, 2,  BTK_FILL, 0,
                    0, 0);

  check = btk_check_button_new_with_mnemonic (_("_Reverse"));
  g_signal_connect (check, "toggled", G_CALLBACK (update_collate_icon), dialog);
  priv->reverse_check = check;
  btk_widget_show (check);
  btk_table_attach (BTK_TABLE (table), check,
                    0, 1, 2, 3,  BTK_FILL, 0,
                    0, 0);

  image = btk_drawing_area_new ();
  btk_widget_set_has_window (image, FALSE);

  priv->collate_image = image;
  btk_widget_show (image);
  btk_widget_set_size_request (image, 70, 90);
  btk_table_attach (BTK_TABLE (table), image,
                    1, 2, 1, 3, BTK_FILL, 0,
                    0, 0);
  g_signal_connect (image, "expose-event",
                    G_CALLBACK (draw_collate_cb), dialog);

  label = btk_label_new (_("General"));
  btk_widget_show (label);

  btk_notebook_append_page (BTK_NOTEBOOK (priv->notebook), main_vbox, label);
}

static bboolean
is_range_separator (bchar c)
{
  return (c == ',' || c == ';' || c == ':');
}

static BtkPageRange *
dialog_get_page_ranges (BtkPrintUnixDialog *dialog,
                        bint               *n_ranges_out)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  bint i, n_ranges;
  const bchar *text, *p;
  bchar *next;
  BtkPageRange *ranges;
  bint start, end;

  text = btk_entry_get_text (BTK_ENTRY (priv->page_range_entry));

  if (*text == 0)
    {
      *n_ranges_out = 0;
      return NULL;
    }

  n_ranges = 1;
  p = text;
  while (*p)
    {
      if (is_range_separator (*p))
        n_ranges++;
      p++;
    }

  ranges = g_new0 (BtkPageRange, n_ranges);

  i = 0;
  p = text;
  while (*p)
    {
      while (isspace (*p)) p++;

      if (*p == '-')
        {
          /* a half-open range like -2 */
          start = 1;
        }
      else
        {
          start = (int)strtol (p, &next, 10);
          if (start < 1)
            start = 1;
          p = next;
        }

      end = start;

      while (isspace (*p)) p++;

      if (*p == '-')
        {
          p++;
          end = (int)strtol (p, &next, 10);
          if (next == p) /* a half-open range like 2- */
            end = 0;
          else if (end < start)
            end = start;
        }

      ranges[i].start = start - 1;
      ranges[i].end = end - 1;
      i++;

      /* Skip until end or separator */
      while (*p && !is_range_separator (*p))
        p++;

      /* if not at end, skip separator */
      if (*p)
        p++;
    }

  *n_ranges_out = i;

  return ranges;
}

static void
dialog_set_page_ranges (BtkPrintUnixDialog *dialog,
                        BtkPageRange       *ranges,
                        bint                n_ranges)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  bint i;
  GString *s = g_string_new (NULL);

  for (i = 0; i < n_ranges; i++)
    {
      g_string_append_printf (s, "%d", ranges[i].start + 1);
      if (ranges[i].end > ranges[i].start)
        g_string_append_printf (s, "-%d", ranges[i].end + 1);
      else if (ranges[i].end == -1)
        g_string_append (s, "-");

      if (i != n_ranges - 1)
        g_string_append (s, ",");
    }

  btk_entry_set_text (BTK_ENTRY (priv->page_range_entry), s->str);

  g_string_free (s, TRUE);
}

static BtkPrintPages
dialog_get_print_pages (BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;

  if (btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (priv->all_pages_radio)))
    return BTK_PRINT_PAGES_ALL;
  else if (btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (priv->current_page_radio)))
    return BTK_PRINT_PAGES_CURRENT;
  else if (btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (priv->selection_radio)))
    return BTK_PRINT_PAGES_SELECTION;
  else
    return BTK_PRINT_PAGES_RANGES;
}

static void
dialog_set_print_pages (BtkPrintUnixDialog *dialog,
                        BtkPrintPages       pages)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;

  if (pages == BTK_PRINT_PAGES_RANGES)
    btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (priv->page_range_radio), TRUE);
  else if (pages == BTK_PRINT_PAGES_CURRENT)
    btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (priv->current_page_radio), TRUE);
  else if (pages == BTK_PRINT_PAGES_SELECTION)
    btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (priv->selection_radio), TRUE);
  else
    btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (priv->all_pages_radio), TRUE);
}

static bdouble
dialog_get_scale (BtkPrintUnixDialog *dialog)
{
  if (btk_widget_is_sensitive (dialog->priv->scale_spin))
    return btk_spin_button_get_value (BTK_SPIN_BUTTON (dialog->priv->scale_spin));
  else
    return 100.0;
}

static void
dialog_set_scale (BtkPrintUnixDialog *dialog,
                  bdouble             val)
{
  btk_spin_button_set_value (BTK_SPIN_BUTTON (dialog->priv->scale_spin), val);
}

static BtkPageSet
dialog_get_page_set (BtkPrintUnixDialog *dialog)
{
  if (btk_widget_is_sensitive (dialog->priv->page_set_combo))
    return (BtkPageSet)btk_combo_box_get_active (BTK_COMBO_BOX (dialog->priv->page_set_combo));
  else
    return BTK_PAGE_SET_ALL;
}

static void
dialog_set_page_set (BtkPrintUnixDialog *dialog,
                     BtkPageSet          val)
{
  btk_combo_box_set_active (BTK_COMBO_BOX (dialog->priv->page_set_combo),
                            (int)val);
}

static bint
dialog_get_n_copies (BtkPrintUnixDialog *dialog)
{
  if (btk_widget_is_sensitive (dialog->priv->copies_spin))
    return btk_spin_button_get_value_as_int (BTK_SPIN_BUTTON (dialog->priv->copies_spin));
  return 1;
}

static void
dialog_set_n_copies (BtkPrintUnixDialog *dialog,
                     bint                n_copies)
{
  btk_spin_button_set_value (BTK_SPIN_BUTTON (dialog->priv->copies_spin),
                             n_copies);
}

static bboolean
dialog_get_collate (BtkPrintUnixDialog *dialog)
{
  if (btk_widget_is_sensitive (dialog->priv->collate_check))
    return btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (dialog->priv->collate_check));
  return FALSE;
}

static void
dialog_set_collate (BtkPrintUnixDialog *dialog,
                    bboolean            collate)
{
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (dialog->priv->collate_check),
                                collate);
}

static bboolean
dialog_get_reverse (BtkPrintUnixDialog *dialog)
{
  if (btk_widget_is_sensitive (dialog->priv->reverse_check))
    return btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (dialog->priv->reverse_check));
  return FALSE;
}

static void
dialog_set_reverse (BtkPrintUnixDialog *dialog,
                    bboolean            reverse)
{
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (dialog->priv->reverse_check),
                                reverse);
}

static bint
dialog_get_pages_per_sheet (BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  const bchar *val;
  bint num;

  val = btk_printer_option_widget_get_value (priv->pages_per_sheet);

  num = 1;

  if (val)
    {
      num = atoi(val);
      if (num < 1)
        num = 1;
    }

  return num;
}

static BtkNumberUpLayout
dialog_get_number_up_layout (BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  BtkPrintCapabilities       caps;
  BtkNumberUpLayout          layout;
  const bchar               *val;
  GEnumClass                *enum_class;
  GEnumValue                *enum_value;

  val = btk_printer_option_widget_get_value (priv->number_up_layout);

  caps = priv->manual_capabilities | priv->printer_capabilities;

  if ((caps & BTK_PRINT_CAPABILITY_NUMBER_UP_LAYOUT) == 0)
    return BTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM;

  if (btk_widget_get_direction (BTK_WIDGET (dialog)) == BTK_TEXT_DIR_LTR)
    layout = BTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM;
  else
    layout = BTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_TOP_TO_BOTTOM;

  if (val == NULL)
    return layout;

  if (val[0] == '\0' && priv->options)
    {
      BtkPrinterOption *option = btk_printer_option_set_lookup (priv->options, "btk-n-up-layout");
      if (option)
        val = option->value;
    }

  enum_class = g_type_class_ref (BTK_TYPE_NUMBER_UP_LAYOUT);
  enum_value = g_enum_get_value_by_nick (enum_class, val);
  if (enum_value)
    layout = enum_value->value;
  g_type_class_unref (enum_class);

  return layout;
}

static bboolean
draw_page_cb (BtkWidget          *widget,
              BdkEventExpose     *event,
              BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  bairo_t *cr;
  bdouble ratio;
  bint w, h, tmp, shadow_offset;
  bint pages_x, pages_y, i, x, y, layout_w, layout_h;
  bdouble page_width, page_height;
  BtkPageOrientation orientation;
  bboolean landscape;
  BangoLayout *layout;
  BangoFontDescription *font;
  bchar *text;
  BdkColor *color;
  BtkNumberUpLayout number_up_layout;
  bint start_x, end_x, start_y, end_y;
  bint dx, dy;
  bboolean horizontal;
  BtkPageSetup *page_setup;
  bdouble paper_width, paper_height;
  bdouble pos_x, pos_y;
  bint pages_per_sheet;
  bboolean ltr = TRUE;

  orientation = btk_page_setup_get_orientation (priv->page_setup);
  landscape =
    (orientation == BTK_PAGE_ORIENTATION_LANDSCAPE) ||
    (orientation == BTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE);

  number_up_layout = dialog_get_number_up_layout (dialog);

  cr = bdk_bairo_create (widget->window);

  bairo_save (cr);

  page_setup = btk_print_unix_dialog_get_page_setup (dialog);  

  if (page_setup != NULL)
    {
      if (!landscape)
        {
          paper_width = btk_page_setup_get_paper_width (page_setup, BTK_UNIT_MM);
          paper_height = btk_page_setup_get_paper_height (page_setup, BTK_UNIT_MM);
        }
      else
        {
          paper_width = btk_page_setup_get_paper_height (page_setup, BTK_UNIT_MM);
          paper_height = btk_page_setup_get_paper_width (page_setup, BTK_UNIT_MM);
        }

      if (paper_width < paper_height)
        {
          h = EXAMPLE_PAGE_AREA_SIZE - 3;
          w = (paper_height != 0) ? h * paper_width / paper_height : 0;
        }
      else
        {
          w = EXAMPLE_PAGE_AREA_SIZE - 3;
          h = (paper_width != 0) ? w * paper_height / paper_width : 0;
        }

      if (paper_width == 0)
        w = 0;

      if (paper_height == 0)
        h = 0;
    }
  else
    {
      ratio = G_SQRT2;
      w = (EXAMPLE_PAGE_AREA_SIZE - 3) / ratio;
      h = EXAMPLE_PAGE_AREA_SIZE - 3;
    }

  pages_per_sheet = dialog_get_pages_per_sheet (dialog);
  switch (pages_per_sheet)
    {
    default:
    case 1:
      pages_x = 1; pages_y = 1;
      break;
    case 2:
      landscape = !landscape;
      pages_x = 1; pages_y = 2;
      break;
    case 4:
      pages_x = 2; pages_y = 2;
      break;
    case 6:
      landscape = !landscape;
      pages_x = 2; pages_y = 3;
      break;
    case 9:
      pages_x = 3; pages_y = 3;
      break;
    case 16:
      pages_x = 4; pages_y = 4;
      break;
    }

  if (landscape)
    {
      tmp = w;
      w = h;
      h = tmp;

      tmp = pages_x;
      pages_x = pages_y;
      pages_y = tmp;
    }

  pos_x = widget->allocation.x + (widget->allocation.width - w) / 2;
  pos_y = widget->allocation.y + (widget->allocation.height - h) / 2 - 10;
  color = &widget->style->text[BTK_STATE_NORMAL];
  bairo_translate (cr, pos_x, pos_y);

  shadow_offset = 3;

  color = &widget->style->text[BTK_STATE_NORMAL];
  bairo_set_source_rgba (cr, color->red / 65535., color->green / 65535., color->blue / 65535, 0.5);
  bairo_rectangle (cr, shadow_offset + 1, shadow_offset + 1, w, h);
  bairo_fill (cr);

  bdk_bairo_set_source_color (cr, &widget->style->base[BTK_STATE_NORMAL]);
  bairo_rectangle (cr, 1, 1, w, h);
  bairo_fill (cr);
  bairo_set_line_width (cr, 1.0);
  bairo_rectangle (cr, 0.5, 0.5, w + 1, h + 1);

  bdk_bairo_set_source_color (cr, &widget->style->text[BTK_STATE_NORMAL]);
  bairo_stroke (cr);

  i = 1;

  page_width = (double)w / pages_x;
  page_height = (double)h / pages_y;

  layout  = bango_bairo_create_layout (cr);

  font = bango_font_description_new ();
  bango_font_description_set_family (font, "sans");

  if (page_height > 0)
    bango_font_description_set_absolute_size (font, page_height * 0.4 * BANGO_SCALE);
  else
    bango_font_description_set_absolute_size (font, 1);

  bango_layout_set_font_description (layout, font);
  bango_font_description_free (font);

  bango_layout_set_width (layout, page_width * BANGO_SCALE);
  bango_layout_set_alignment (layout, BANGO_ALIGN_CENTER);

  switch (number_up_layout)
    {
      default:
      case BTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM:
        start_x = 0;
        end_x = pages_x - 1;
        start_y = 0;
        end_y = pages_y - 1;
        dx = 1;
        dy = 1;
        horizontal = TRUE;
        break;
      case BTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_BOTTOM_TO_TOP:
        start_x = 0;
        end_x = pages_x - 1;
        start_y = pages_y - 1;
        end_y = 0;
        dx = 1;
        dy = - 1;
        horizontal = TRUE;
        break;
      case BTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_TOP_TO_BOTTOM:
        start_x = pages_x - 1;
        end_x = 0;
        start_y = 0;
        end_y = pages_y - 1;
        dx = - 1;
        dy = 1;
        horizontal = TRUE;
        break;
      case BTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_BOTTOM_TO_TOP:
        start_x = pages_x - 1;
        end_x = 0;
        start_y = pages_y - 1;
        end_y = 0;
        dx = - 1;
        dy = - 1;
        horizontal = TRUE;
        break;
      case BTK_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_LEFT_TO_RIGHT:
        start_x = 0;
        end_x = pages_x - 1;
        start_y = 0;
        end_y = pages_y - 1;
        dx = 1;
        dy = 1;
        horizontal = FALSE;
        break;
      case BTK_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_RIGHT_TO_LEFT:
        start_x = pages_x - 1;
        end_x = 0;
        start_y = 0;
        end_y = pages_y - 1;
        dx = - 1;
        dy = 1;
        horizontal = FALSE;
        break;
      case BTK_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_LEFT_TO_RIGHT:
        start_x = 0;
        end_x = pages_x - 1;
        start_y = pages_y - 1;
        end_y = 0;
        dx = 1;
        dy = - 1;
        horizontal = FALSE;
        break;
      case BTK_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_RIGHT_TO_LEFT:
        start_x = pages_x - 1;
        end_x = 0;
        start_y = pages_y - 1;
        end_y = 0;
        dx = - 1;
        dy = - 1;
        horizontal = FALSE;
        break;
    }

  if (horizontal)
    for (y = start_y; y != end_y + dy; y += dy)
      {
        for (x = start_x; x != end_x + dx; x += dx)
          {
            text = g_strdup_printf ("%d", i++);
            bango_layout_set_text (layout, text, -1);
            g_free (text);
            bango_layout_get_size (layout, &layout_w, &layout_h);
            bairo_save (cr);
            bairo_translate (cr,
                             x * page_width,
                             y * page_height + (page_height - layout_h / 1024.0) / 2);

            bango_bairo_show_layout (cr, layout);
            bairo_restore (cr);
          }
      }
  else
    for (x = start_x; x != end_x + dx; x += dx)
      {
        for (y = start_y; y != end_y + dy; y += dy)
          {
            text = g_strdup_printf ("%d", i++);
            bango_layout_set_text (layout, text, -1);
            g_free (text);
            bango_layout_get_size (layout, &layout_w, &layout_h);
            bairo_save (cr);
            bairo_translate (cr,
                             x * page_width,
                             y * page_height + (page_height - layout_h / 1024.0) / 2);

            bango_bairo_show_layout (cr, layout);
            bairo_restore (cr);
          }
      }

  g_object_unref (layout);

  if (page_setup != NULL)
    {
      pos_x += 1;
      pos_y += 1;

      if (pages_per_sheet == 2 || pages_per_sheet == 6)
        {
          paper_width = btk_page_setup_get_paper_height (page_setup, _btk_print_get_default_user_units ());
          paper_height = btk_page_setup_get_paper_width (page_setup, _btk_print_get_default_user_units ());
        }
      else
        {
          paper_width = btk_page_setup_get_paper_width (page_setup, _btk_print_get_default_user_units ());
          paper_height = btk_page_setup_get_paper_height (page_setup, _btk_print_get_default_user_units ());
        }

      bairo_restore (cr);
      bairo_save (cr);

      layout  = bango_bairo_create_layout (cr);

      font = bango_font_description_new ();
      bango_font_description_set_family (font, "sans");

      BangoContext *bango_c = NULL;
      BangoFontDescription *bango_f = NULL;
      bint font_size = 12 * BANGO_SCALE;

      bango_c = btk_widget_get_bango_context (widget);
      if (bango_c != NULL)
        {
          bango_f = bango_context_get_font_description (bango_c);
          if (bango_f != NULL)
            font_size = bango_font_description_get_size (bango_f);
        }

      bango_font_description_set_size (font, font_size);
      bango_layout_set_font_description (layout, font);
      bango_font_description_free (font);

      bango_layout_set_width (layout, -1);
      bango_layout_set_alignment (layout, BANGO_ALIGN_CENTER);

      if (_btk_print_get_default_user_units () == BTK_UNIT_MM)
        text = g_strdup_printf ("%.1f mm", paper_height);
      else
        text = g_strdup_printf ("%.2f inch", paper_height);

      bango_layout_set_text (layout, text, -1);
      g_free (text);
      bango_layout_get_size (layout, &layout_w, &layout_h);

      ltr = btk_widget_get_direction (BTK_WIDGET (dialog)) == BTK_TEXT_DIR_LTR;

      if (ltr)
        bairo_translate (cr, pos_x - layout_w / BANGO_SCALE - 2 * RULER_DISTANCE,
                             widget->allocation.y + (widget->allocation.height - layout_h / BANGO_SCALE) / 2);
      else
        bairo_translate (cr, pos_x + w + shadow_offset + 2 * RULER_DISTANCE,
                             widget->allocation.y + (widget->allocation.height - layout_h / BANGO_SCALE) / 2);

      bango_bairo_show_layout (cr, layout);

      bairo_restore (cr);
      bairo_save (cr);

      if (_btk_print_get_default_user_units () == BTK_UNIT_MM)
        text = g_strdup_printf ("%.1f mm", paper_width);
      else
        text = g_strdup_printf ("%.2f inch", paper_width);

      bango_layout_set_text (layout, text, -1);
      g_free (text);
      bango_layout_get_size (layout, &layout_w, &layout_h);

      bairo_translate (cr, widget->allocation.x + (widget->allocation.width - layout_w / BANGO_SCALE) / 2,
                           pos_y + h + shadow_offset + 2 * RULER_DISTANCE);

      bango_bairo_show_layout (cr, layout);

      g_object_unref (layout);

      bairo_restore (cr);

      bairo_set_line_width (cr, 1);

      if (ltr)
        {
          bairo_move_to (cr, pos_x - RULER_DISTANCE, pos_y);
          bairo_line_to (cr, pos_x - RULER_DISTANCE, pos_y + h);
          bairo_stroke (cr);

          bairo_move_to (cr, pos_x - RULER_DISTANCE - RULER_RADIUS, pos_y - 0.5);
          bairo_line_to (cr, pos_x - RULER_DISTANCE + RULER_RADIUS, pos_y - 0.5);
          bairo_stroke (cr);

          bairo_move_to (cr, pos_x - RULER_DISTANCE - RULER_RADIUS, pos_y + h + 0.5);
          bairo_line_to (cr, pos_x - RULER_DISTANCE + RULER_RADIUS, pos_y + h + 0.5);
          bairo_stroke (cr);
        }
      else
        {
          bairo_move_to (cr, pos_x + w + shadow_offset + RULER_DISTANCE, pos_y);
          bairo_line_to (cr, pos_x + w + shadow_offset + RULER_DISTANCE, pos_y + h);
          bairo_stroke (cr);

          bairo_move_to (cr, pos_x + w + shadow_offset + RULER_DISTANCE - RULER_RADIUS, pos_y - 0.5);
          bairo_line_to (cr, pos_x + w + shadow_offset + RULER_DISTANCE + RULER_RADIUS, pos_y - 0.5);
          bairo_stroke (cr);

          bairo_move_to (cr, pos_x + w + shadow_offset + RULER_DISTANCE - RULER_RADIUS, pos_y + h + 0.5);
          bairo_line_to (cr, pos_x + w + shadow_offset + RULER_DISTANCE + RULER_RADIUS, pos_y + h + 0.5);
          bairo_stroke (cr);
        }

      bairo_move_to (cr, pos_x, pos_y + h + shadow_offset + RULER_DISTANCE);
      bairo_line_to (cr, pos_x + w, pos_y + h + shadow_offset + RULER_DISTANCE);
      bairo_stroke (cr);

      bairo_move_to (cr, pos_x - 0.5, pos_y + h + shadow_offset + RULER_DISTANCE - RULER_RADIUS);
      bairo_line_to (cr, pos_x - 0.5, pos_y + h + shadow_offset + RULER_DISTANCE + RULER_RADIUS);
      bairo_stroke (cr);

      bairo_move_to (cr, pos_x + w + 0.5, pos_y + h + shadow_offset + RULER_DISTANCE - RULER_RADIUS);
      bairo_line_to (cr, pos_x + w + 0.5, pos_y + h + shadow_offset + RULER_DISTANCE + RULER_RADIUS);
      bairo_stroke (cr);
    }

  bairo_destroy (cr);

  return TRUE;
}

static void
redraw_page_layout_preview (BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;

  if (priv->page_layout_preview)
    btk_widget_queue_draw (priv->page_layout_preview);
}

static void
update_number_up_layout (BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  BtkPrintCapabilities       caps;
  BtkPrinterOptionSet       *set;
  BtkNumberUpLayout          layout;
  BtkPrinterOption          *option;
  BtkPrinterOption          *old_option;
  BtkPageOrientation         page_orientation;

  set = priv->options;

  caps = priv->manual_capabilities | priv->printer_capabilities;

  if (caps & BTK_PRINT_CAPABILITY_NUMBER_UP_LAYOUT)
    {
      if (priv->number_up_layout_n_option == NULL)
        {
          priv->number_up_layout_n_option = btk_printer_option_set_lookup (set, "btk-n-up-layout");
          if (priv->number_up_layout_n_option == NULL)
            {
              char *n_up_layout[] = { "lrtb", "lrbt", "rltb", "rlbt", "tblr", "tbrl", "btlr", "btrl" };
               /* Translators: These strings name the possible arrangements of
                * multiple pages on a sheet when printing (same as in btkprintbackendcups.c)
                */
              char *n_up_layout_display[] = { N_("Left to right, top to bottom"), N_("Left to right, bottom to top"),
                                              N_("Right to left, top to bottom"), N_("Right to left, bottom to top"),
                                              N_("Top to bottom, left to right"), N_("Top to bottom, right to left"),
                                              N_("Bottom to top, left to right"), N_("Bottom to top, right to left") };
              int i;

              priv->number_up_layout_n_option = btk_printer_option_new ("btk-n-up-layout",
                                                                        _("Page Ordering"),
                                                                        BTK_PRINTER_OPTION_TYPE_PICKONE);
              btk_printer_option_allocate_choices (priv->number_up_layout_n_option, 8);

              for (i = 0; i < G_N_ELEMENTS (n_up_layout_display); i++)
                {
                  priv->number_up_layout_n_option->choices[i] = g_strdup (n_up_layout[i]);
                  priv->number_up_layout_n_option->choices_display[i] = g_strdup (_(n_up_layout_display[i]));
                }
            }
          g_object_ref (priv->number_up_layout_n_option);

          priv->number_up_layout_2_option = btk_printer_option_new ("btk-n-up-layout",
                                                                    _("Page Ordering"),
                                                                    BTK_PRINTER_OPTION_TYPE_PICKONE);
          btk_printer_option_allocate_choices (priv->number_up_layout_2_option, 2);
        }

      page_orientation = btk_page_setup_get_orientation (priv->page_setup);
      if (page_orientation == BTK_PAGE_ORIENTATION_PORTRAIT ||
          page_orientation == BTK_PAGE_ORIENTATION_REVERSE_PORTRAIT)
        {
          if (! (priv->number_up_layout_2_option->choices[0] == priv->number_up_layout_n_option->choices[0] &&
                 priv->number_up_layout_2_option->choices[1] == priv->number_up_layout_n_option->choices[2]))
            {
              g_free (priv->number_up_layout_2_option->choices_display[0]);
              g_free (priv->number_up_layout_2_option->choices_display[1]);
              priv->number_up_layout_2_option->choices[0] = priv->number_up_layout_n_option->choices[0];
              priv->number_up_layout_2_option->choices[1] = priv->number_up_layout_n_option->choices[2];
              priv->number_up_layout_2_option->choices_display[0] = g_strdup ( _("Left to right"));
              priv->number_up_layout_2_option->choices_display[1] = g_strdup ( _("Right to left"));
            }
        }
      else
        {
          if (! (priv->number_up_layout_2_option->choices[0] == priv->number_up_layout_n_option->choices[0] &&
                 priv->number_up_layout_2_option->choices[1] == priv->number_up_layout_n_option->choices[1]))
            {
              g_free (priv->number_up_layout_2_option->choices_display[0]);
              g_free (priv->number_up_layout_2_option->choices_display[1]);
              priv->number_up_layout_2_option->choices[0] = priv->number_up_layout_n_option->choices[0];
              priv->number_up_layout_2_option->choices[1] = priv->number_up_layout_n_option->choices[1];
              priv->number_up_layout_2_option->choices_display[0] = g_strdup ( _("Top to bottom"));
              priv->number_up_layout_2_option->choices_display[1] = g_strdup ( _("Bottom to top"));
            }
        }

      layout = dialog_get_number_up_layout (dialog);

      old_option = btk_printer_option_set_lookup (set, "btk-n-up-layout");
      if (old_option != NULL)
        btk_printer_option_set_remove (set, old_option);

      if (dialog_get_pages_per_sheet (dialog) != 1)
        {
          GEnumClass *enum_class;
          GEnumValue *enum_value;
          enum_class = g_type_class_ref (BTK_TYPE_NUMBER_UP_LAYOUT);

          if (dialog_get_pages_per_sheet (dialog) == 2)
            {
              option = priv->number_up_layout_2_option;

              switch (layout)
                {
                case BTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM:
                case BTK_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_LEFT_TO_RIGHT:
                  enum_value = g_enum_get_value (enum_class, BTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM);
                  break;

                case BTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_BOTTOM_TO_TOP:
                case BTK_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_LEFT_TO_RIGHT:
                  enum_value = g_enum_get_value (enum_class, BTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_BOTTOM_TO_TOP);
                  break;

                case BTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_TOP_TO_BOTTOM:
                case BTK_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_RIGHT_TO_LEFT:
                  enum_value = g_enum_get_value (enum_class, BTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_TOP_TO_BOTTOM);
                  break;

                case BTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_BOTTOM_TO_TOP:
                case BTK_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_RIGHT_TO_LEFT:
                  enum_value = g_enum_get_value (enum_class, BTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_BOTTOM_TO_TOP);
                  break;

                default:
                  g_assert_not_reached();
                  enum_value = NULL;
                }
            }
          else
            {
              option = priv->number_up_layout_n_option;

              enum_value = g_enum_get_value (enum_class, layout);
            }

          g_assert (enum_value != NULL);
          btk_printer_option_set (option, enum_value->value_nick);
          g_type_class_unref (enum_class);

          btk_printer_option_set_add (set, option);
        }
    }

  setup_option (dialog, "btk-n-up-layout", priv->number_up_layout);

  if (priv->number_up_layout != NULL)
    btk_widget_set_sensitive (BTK_WIDGET (priv->number_up_layout),
                              (caps & BTK_PRINT_CAPABILITY_NUMBER_UP_LAYOUT) &&
                              (dialog_get_pages_per_sheet (dialog) > 1));
}

static void
custom_paper_dialog_response_cb (BtkDialog *custom_paper_dialog,
				 bint       response_id,
				 bpointer   user_data)
{
  BtkPrintUnixDialog        *print_dialog = BTK_PRINT_UNIX_DIALOG (user_data);
  BtkPrintUnixDialogPrivate *priv = print_dialog->priv;
  BtkTreeModel              *model;
  BtkTreeIter                iter;

  _btk_print_load_custom_papers (priv->custom_paper_list);

  priv->internal_page_setup_change = TRUE;
  update_paper_sizes (print_dialog);
  priv->internal_page_setup_change = FALSE;

  if (priv->page_setup_set)
    {
      model = BTK_TREE_MODEL (priv->custom_paper_list);
      if (btk_tree_model_get_iter_first (model, &iter))
        {
          do
            {
              BtkPageSetup *page_setup;
              btk_tree_model_get (model, &iter, 0, &page_setup, -1);

              if (page_setup &&
                  g_strcmp0 (btk_paper_size_get_display_name (btk_page_setup_get_paper_size (page_setup)),
                             btk_paper_size_get_display_name (btk_page_setup_get_paper_size (priv->page_setup))) == 0)
                btk_print_unix_dialog_set_page_setup (print_dialog, page_setup);

              g_object_unref (page_setup);
            } while (btk_tree_model_iter_next (model, &iter));
        }
    }

  btk_widget_destroy (BTK_WIDGET (custom_paper_dialog));
}

static void
orientation_changed (BtkComboBox        *combo_box,
                     BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  BtkPageOrientation         orientation;
  BtkPageSetup              *page_setup;

  if (priv->internal_page_setup_change)
    return;

  orientation = (BtkPageOrientation) btk_combo_box_get_active (BTK_COMBO_BOX (priv->orientation_combo));

  if (priv->page_setup)
    {
      page_setup = btk_page_setup_copy (priv->page_setup);
      if (page_setup)
        btk_page_setup_set_orientation (page_setup, orientation);

      btk_print_unix_dialog_set_page_setup (dialog, page_setup);
    }

  redraw_page_layout_preview (dialog);
}

static void
paper_size_changed (BtkComboBox        *combo_box,
                    BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  BtkTreeIter iter;
  BtkPageSetup *page_setup, *last_page_setup;
  BtkPageOrientation orientation;

  if (priv->internal_page_setup_change)
    return;

  if (btk_combo_box_get_active_iter (combo_box, &iter))
    {
      btk_tree_model_get (btk_combo_box_get_model (combo_box),
                          &iter, PAGE_SETUP_LIST_COL_PAGE_SETUP, &page_setup, -1);

      if (page_setup == NULL)
        {
          BtkWidget *custom_paper_dialog;

          /* Change from "manage" menu item to last value */
          if (priv->page_setup)
            last_page_setup = g_object_ref (priv->page_setup);
          else
            last_page_setup = btk_page_setup_new (); /* "good" default */

          if (!set_paper_size (dialog, last_page_setup, FALSE, FALSE))
            set_paper_size (dialog, last_page_setup, TRUE, TRUE);
          g_object_unref (last_page_setup);

          /* And show the custom paper dialog */
          custom_paper_dialog = _btk_custom_paper_unix_dialog_new (BTK_WINDOW (dialog), _("Manage Custom Sizes"));
          g_signal_connect (custom_paper_dialog, "response", G_CALLBACK (custom_paper_dialog_response_cb), dialog);
          btk_window_present (BTK_WINDOW (custom_paper_dialog));

          return;
        }

      if (priv->page_setup)
        orientation = btk_page_setup_get_orientation (priv->page_setup);
      else
        orientation = BTK_PAGE_ORIENTATION_PORTRAIT;

      btk_page_setup_set_orientation (page_setup, orientation);
      btk_print_unix_dialog_set_page_setup (dialog, page_setup);

      g_object_unref (page_setup);
    }

  redraw_page_layout_preview (dialog);
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

static void
create_page_setup_page (BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  BtkWidget *main_vbox, *label, *hbox, *hbox2;
  BtkWidget *frame, *table, *widget;
  BtkWidget *combo, *spinbutton, *draw;
  BtkCellRenderer *cell;

  main_vbox = btk_vbox_new (FALSE, 18);
  btk_container_set_border_width (BTK_CONTAINER (main_vbox), 12);
  btk_widget_show (main_vbox);

  hbox = btk_hbox_new (FALSE, 18);
  btk_widget_show (hbox);
  btk_box_pack_start (BTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

  table = btk_table_new (5, 2, FALSE);
  btk_table_set_row_spacings (BTK_TABLE (table), 6);
  btk_table_set_col_spacings (BTK_TABLE (table), 12);
  frame = wrap_in_frame (_("Layout"), table);
  btk_box_pack_start (BTK_BOX (hbox), frame, TRUE, TRUE, 0);
  btk_widget_show (table);

  label = btk_label_new_with_mnemonic (_("T_wo-sided:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_widget_show (label);
  btk_table_attach (BTK_TABLE (table), label,
                    0, 1, 0, 1,  BTK_FILL, 0,
                    0, 0);

  widget = btk_printer_option_widget_new (NULL);
  priv->duplex = BTK_PRINTER_OPTION_WIDGET (widget);
  btk_widget_show (widget);
  btk_table_attach (BTK_TABLE (table), widget,
                    1, 2, 0, 1,  BTK_FILL, 0,
                    0, 0);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), widget);

  label = btk_label_new_with_mnemonic (_("Pages per _side:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_widget_show (label);
  btk_table_attach (BTK_TABLE (table), label,
                    0, 1, 1, 2,  BTK_FILL, 0,
                    0, 0);

  widget = btk_printer_option_widget_new (NULL);
  g_signal_connect_swapped (widget, "changed", G_CALLBACK (redraw_page_layout_preview), dialog);
  g_signal_connect_swapped (widget, "changed", G_CALLBACK (update_number_up_layout), dialog);
  priv->pages_per_sheet = BTK_PRINTER_OPTION_WIDGET (widget);
  btk_widget_show (widget);
  btk_table_attach (BTK_TABLE (table), widget,
                    1, 2, 1, 2,  BTK_FILL, 0,
                    0, 0);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), widget);

  label = btk_label_new_with_mnemonic (_("Page or_dering:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_widget_show (label);
  btk_table_attach (BTK_TABLE (table), label,
                    0, 1, 2, 3,  BTK_FILL, 0,
                    0, 0);

  widget = btk_printer_option_widget_new (NULL);
  g_signal_connect_swapped (widget, "changed", G_CALLBACK (redraw_page_layout_preview), dialog);
  priv->number_up_layout = BTK_PRINTER_OPTION_WIDGET (widget);
  btk_widget_show (widget);
  btk_table_attach (BTK_TABLE (table), widget,
                    1, 2, 2, 3,  BTK_FILL, 0,
                    0, 0);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), widget);

  label = btk_label_new_with_mnemonic (_("_Only print:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_widget_show (label);
  btk_table_attach (BTK_TABLE (table), label,
                    0, 1, 3, 4,  BTK_FILL, 0,
                    0, 0);

  combo = btk_combo_box_text_new ();
  priv->page_set_combo = combo;
  btk_widget_show (combo);
  btk_table_attach (BTK_TABLE (table), combo,
                    1, 2, 3, 4,  BTK_FILL, 0,
                    0, 0);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), combo);
  /* In enum order */
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo), _("All sheets"));
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo), _("Even sheets"));
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo), _("Odd sheets"));
  btk_combo_box_set_active (BTK_COMBO_BOX (combo), 0);

  label = btk_label_new_with_mnemonic (_("Sc_ale:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_widget_show (label);
  btk_table_attach (BTK_TABLE (table), label,
                    0, 1, 4, 5,  BTK_FILL, 0,
                    0, 0);

  hbox2 = btk_hbox_new (FALSE, 6);
  btk_widget_show (hbox2);
  btk_table_attach (BTK_TABLE (table), hbox2,
                    1, 2, 4, 5,  BTK_FILL, 0,
                    0, 0);

  spinbutton = btk_spin_button_new_with_range (1.0, 1000.0, 1.0);
  priv->scale_spin = spinbutton;
  btk_spin_button_set_digits (BTK_SPIN_BUTTON (spinbutton), 1);
  btk_spin_button_set_value (BTK_SPIN_BUTTON (spinbutton), 100.0);
  btk_box_pack_start (BTK_BOX (hbox2), spinbutton, FALSE, FALSE, 0);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), spinbutton);
  btk_widget_show (spinbutton);
  label = btk_label_new ("%"); /* FIXMEchpe does there exist any language where % needs to be translated? */
  btk_widget_show (label);
  btk_box_pack_start (BTK_BOX (hbox2), label, FALSE, FALSE, 0);

  table = btk_table_new (4, 2, FALSE);
  btk_table_set_row_spacings (BTK_TABLE (table), 6);
  btk_table_set_col_spacings (BTK_TABLE (table), 12);
  frame = wrap_in_frame (_("Paper"), table);
  btk_box_pack_start (BTK_BOX (hbox), frame, TRUE, TRUE, 6);
  btk_widget_show (table);

  label = btk_label_new_with_mnemonic (_("Paper _type:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_widget_show (label);
  btk_table_attach (BTK_TABLE (table), label,
                    0, 1, 0, 1,  BTK_FILL, 0,
                    0, 0);

  widget = btk_printer_option_widget_new (NULL);
  priv->paper_type = BTK_PRINTER_OPTION_WIDGET (widget);
  btk_widget_show (widget);
  btk_table_attach (BTK_TABLE (table), widget,
                    1, 2, 0, 1,  BTK_FILL, 0,
                    0, 0);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), widget);

  label = btk_label_new_with_mnemonic (_("Paper _source:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_widget_show (label);
  btk_table_attach (BTK_TABLE (table), label,
                    0, 1, 1, 2,  BTK_FILL, 0,
                    0, 0);

  widget = btk_printer_option_widget_new (NULL);
  priv->paper_source = BTK_PRINTER_OPTION_WIDGET (widget);
  btk_widget_show (widget);
  btk_table_attach (BTK_TABLE (table), widget,
                    1, 2, 1, 2,  BTK_FILL, 0,
                    0, 0);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), widget);

  label = btk_label_new_with_mnemonic (_("Output t_ray:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_widget_show (label);
  btk_table_attach (BTK_TABLE (table), label,
                    0, 1, 2, 3,  BTK_FILL, 0,
                    0, 0);

  widget = btk_printer_option_widget_new (NULL);
  priv->output_tray = BTK_PRINTER_OPTION_WIDGET (widget);
  btk_widget_show (widget);
  btk_table_attach (BTK_TABLE (table), widget,
                    1, 2, 2, 3,  BTK_FILL, 0,
                    0, 0);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), widget);


  label = btk_label_new_with_mnemonic (_("_Paper size:"));
  priv->paper_size_combo_label = BTK_WIDGET (label);
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_widget_show (label);
  btk_table_attach (BTK_TABLE (table), label,
		    0, 1, 3, 4,  BTK_FILL, 0,
		    0, 0);

  combo = btk_combo_box_new_with_model (BTK_TREE_MODEL (priv->page_setup_list));
  priv->paper_size_combo = BTK_WIDGET (combo);
  btk_combo_box_set_row_separator_func (BTK_COMBO_BOX (combo), 
                                        paper_size_row_is_separator, NULL, NULL);
  cell = btk_cell_renderer_text_new ();
  btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combo), cell, TRUE);
  btk_cell_layout_set_cell_data_func (BTK_CELL_LAYOUT (combo), cell,
                                      page_name_func, NULL, NULL);
  btk_table_attach (BTK_TABLE (table), combo,
		    1, 2, 3, 4,  BTK_FILL, 0,
		    0, 0);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), combo);
  btk_widget_set_sensitive (combo, FALSE);
  btk_widget_show (combo);


  label = btk_label_new_with_mnemonic (_("Or_ientation:"));
  priv->orientation_combo_label = BTK_WIDGET (label);
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_widget_show (label);
  btk_table_attach (BTK_TABLE (table), label,
		    0, 1, 4, 5,
		    BTK_FILL, 0, 0, 0);

  combo = btk_combo_box_text_new ();
  priv->orientation_combo = BTK_WIDGET (combo);
  btk_table_attach (BTK_TABLE (table), combo,
		    1, 2, 4, 5,  BTK_FILL, 0,
		    0, 0);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), combo);
  /* In enum order */
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo), _("Portrait"));
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo), _("Landscape"));
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo), _("Reverse portrait"));
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo), _("Reverse landscape"));
  btk_combo_box_set_active (BTK_COMBO_BOX (combo), 0);
  btk_widget_set_sensitive (combo, FALSE);
  btk_widget_show (combo);


  /* Add the page layout preview */
  hbox2 = btk_hbox_new (FALSE, 0);
  btk_widget_show (hbox2);
  btk_box_pack_start (BTK_BOX (main_vbox), hbox2, TRUE, TRUE, 0);

  draw = btk_drawing_area_new ();
  btk_widget_set_has_window (draw, FALSE);
  priv->page_layout_preview = draw;
  btk_widget_set_size_request (draw, 280, 160);
  g_signal_connect (draw, "expose-event", G_CALLBACK (draw_page_cb), dialog);
  btk_widget_show (draw);

  btk_box_pack_start (BTK_BOX (hbox2), draw, TRUE, FALSE, 0);

  label = btk_label_new (_("Page Setup"));
  btk_widget_show (label);

  btk_notebook_append_page (BTK_NOTEBOOK (priv->notebook),
                            main_vbox, label);
}

static void
create_job_page (BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  BtkWidget *main_table, *label;
  BtkWidget *frame, *table, *radio;
  BtkWidget *entry, *widget;
  const bchar *at_tooltip;
  const bchar *on_hold_tooltip;

  main_table = btk_table_new (2, 2, FALSE);
  btk_container_set_border_width (BTK_CONTAINER (main_table), 12);
  btk_table_set_row_spacings (BTK_TABLE (main_table), 18);
  btk_table_set_col_spacings (BTK_TABLE (main_table), 18);

  table = btk_table_new (2, 2, FALSE);
  btk_table_set_row_spacings (BTK_TABLE (table), 6);
  btk_table_set_col_spacings (BTK_TABLE (table), 12);
  frame = wrap_in_frame (_("Job Details"), table);
  btk_table_attach (BTK_TABLE (main_table), frame,
                    0, 1, 0, 1,  BTK_FILL, 0,
                    0, 0);
  btk_widget_show (table);

  label = btk_label_new_with_mnemonic (_("Pri_ority:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_widget_show (label);
  btk_table_attach (BTK_TABLE (table), label,
                    0, 1, 0, 1,  BTK_FILL, 0,
                    0, 0);

  widget = btk_printer_option_widget_new (NULL);
  priv->job_prio = BTK_PRINTER_OPTION_WIDGET (widget);
  btk_widget_show (widget);
  btk_table_attach (BTK_TABLE (table), widget,
                    1, 2, 0, 1,  BTK_FILL, 0,
                    0, 0);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), widget);

  label = btk_label_new_with_mnemonic (_("_Billing info:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_widget_show (label);
  btk_table_attach (BTK_TABLE (table), label,
                    0, 1, 1, 2,  BTK_FILL, 0,
                    0, 0);

  widget = btk_printer_option_widget_new (NULL);
  priv->billing_info = BTK_PRINTER_OPTION_WIDGET (widget);
  btk_widget_show (widget);
  btk_table_attach (BTK_TABLE (table), widget,
                    1, 2, 1, 2,  BTK_FILL, 0,
                    0, 0);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), widget);

  table = btk_table_new (2, 2, FALSE);
  btk_table_set_row_spacings (BTK_TABLE (table), 6);
  btk_table_set_col_spacings (BTK_TABLE (table), 12);
  frame = wrap_in_frame (_("Print Document"), table);
  btk_table_attach (BTK_TABLE (main_table), frame,
                    0, 1, 1, 2,  BTK_FILL, 0,
                    0, 0);
  btk_widget_show (table);

  /* Translators: this is one of the choices for the print at option
   * in the print dialog
   */
  radio = btk_radio_button_new_with_mnemonic (NULL, _("_Now"));
  priv->print_now_radio = radio;
  btk_widget_show (radio);
  btk_table_attach (BTK_TABLE (table), radio,
                    0, 2, 0, 1,  BTK_FILL, 0,
                    0, 0);
  /* Translators: this is one of the choices for the print at option
   * in the print dialog. It also serves as the label for an entry that
   * allows the user to enter a time.
   */
  radio = btk_radio_button_new_with_mnemonic (btk_radio_button_get_group (BTK_RADIO_BUTTON (radio)),
                                              _("A_t:"));

  /* Translators: Ability to parse the am/pm format depends on actual locale.
   * You can remove the am/pm values below for your locale if they are not
   * supported.
   */
  at_tooltip = _("Specify the time of print,\n e.g. 15:30, 2:35 pm, 14:15:20, 11:46:30 am, 4 pm");
  btk_widget_set_tooltip_text (radio, at_tooltip);
  priv->print_at_radio = radio;
  btk_widget_show (radio);
  btk_table_attach (BTK_TABLE (table), radio,
                    0, 1, 1, 2,  BTK_FILL, 0,
                    0, 0);

  entry = btk_entry_new ();
  btk_widget_set_tooltip_text (entry, at_tooltip);
  batk_object_set_name (btk_widget_get_accessible (entry), _("Time of print"));
  batk_object_set_description (btk_widget_get_accessible (entry), at_tooltip);
  priv->print_at_entry = entry;
  btk_widget_show (entry);
  btk_table_attach (BTK_TABLE (table), entry,
                    1, 2, 1, 2,  BTK_FILL, 0,
                    0, 0);

  g_signal_connect (radio, "toggled", G_CALLBACK (update_entry_sensitivity), entry);
  update_entry_sensitivity (radio, entry);

  /* Translators: this is one of the choices for the print at option
   * in the print dialog. It means that the print job will not be
   * printed until it explicitly gets 'released'.
   */
  radio = btk_radio_button_new_with_mnemonic (btk_radio_button_get_group (BTK_RADIO_BUTTON (radio)),
                                              _("On _hold"));
  on_hold_tooltip = _("Hold the job until it is explicitly released");
  btk_widget_set_tooltip_text (radio, on_hold_tooltip);
  priv->print_hold_radio = radio;
  btk_widget_show (radio);
  btk_table_attach (BTK_TABLE (table), radio,
                    0, 2, 2, 3,  BTK_FILL, 0,
                    0, 0);

  g_signal_connect_swapped (priv->print_now_radio, "toggled",
                            G_CALLBACK (update_print_at_option), dialog);
  g_signal_connect_swapped (priv->print_at_radio, "toggled",
                            G_CALLBACK (update_print_at_option), dialog);
  g_signal_connect_swapped (priv->print_at_entry, "changed",
                            G_CALLBACK (update_print_at_option), dialog);
  g_signal_connect_swapped (priv->print_hold_radio, "toggled",
                            G_CALLBACK (update_print_at_option), dialog);

  table = btk_table_new (2, 2, FALSE);
  btk_table_set_row_spacings (BTK_TABLE (table), 6);
  btk_table_set_col_spacings (BTK_TABLE (table), 12);
  frame = wrap_in_frame (_("Add Cover Page"), table);
  btk_table_attach (BTK_TABLE (main_table), frame,
                    1, 2, 0, 1,  BTK_FILL, 0,
                    0, 0);
  btk_widget_show (table);

  /* Translators, this is the label used for the option in the print
   * dialog that controls the front cover page.
   */
  label = btk_label_new_with_mnemonic (_("Be_fore:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_widget_show (label);
  btk_table_attach (BTK_TABLE (table), label,
                    0, 1, 0, 1,  BTK_FILL, 0,
                    0, 0);

  widget = btk_printer_option_widget_new (NULL);
  priv->cover_before = BTK_PRINTER_OPTION_WIDGET (widget);
  btk_widget_show (widget);
  btk_table_attach (BTK_TABLE (table), widget,
                    1, 2, 0, 1,  BTK_FILL, 0,
                    0, 0);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), widget);

  /* Translators, this is the label used for the option in the print
   * dialog that controls the back cover page.
   */
  label = btk_label_new_with_mnemonic (_("_After:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_widget_show (label);
  btk_table_attach (BTK_TABLE (table), label,
                    0, 1, 1, 2,  BTK_FILL, 0,
                    0, 0);

  widget = btk_printer_option_widget_new (NULL);
  priv->cover_after = BTK_PRINTER_OPTION_WIDGET (widget);
  btk_widget_show (widget);
  btk_table_attach (BTK_TABLE (table), widget,
                    1, 2, 1, 2,  BTK_FILL, 0,
                    0, 0);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), widget);

  /* Translators: this is the tab label for the notebook tab containing
   * job-specific options in the print dialog
   */
  label = btk_label_new (_("Job"));
  btk_widget_show (label);

  priv->job_page = main_table;
  btk_notebook_append_page (BTK_NOTEBOOK (priv->notebook),
                            main_table, label);
}

static void
create_optional_page (BtkPrintUnixDialog  *dialog,
                      const bchar         *text,
                      BtkWidget          **table_out,
                      BtkWidget          **page_out)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  BtkWidget *table, *label, *scrolled;

  scrolled = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled),
                                  BTK_POLICY_NEVER,
                                  BTK_POLICY_AUTOMATIC);

  table = btk_table_new (1, 2, FALSE);
  btk_table_set_row_spacings (BTK_TABLE (table), 6);
  btk_table_set_col_spacings (BTK_TABLE (table), 12);
  btk_container_set_border_width (BTK_CONTAINER (table), 12);
  btk_widget_show (table);

  btk_scrolled_window_add_with_viewport (BTK_SCROLLED_WINDOW (scrolled),
                                         table);
  btk_viewport_set_shadow_type (BTK_VIEWPORT (BTK_BIN(scrolled)->child),
                                BTK_SHADOW_NONE);

  label = btk_label_new (text);
  btk_widget_show (label);

  btk_notebook_append_page (BTK_NOTEBOOK (priv->notebook),
                            scrolled, label);

  *table_out = table;
  *page_out = scrolled;
}

static void
create_advanced_page (BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  BtkWidget *main_vbox, *label, *scrolled;

  scrolled = btk_scrolled_window_new (NULL, NULL);
  priv->advanced_page = scrolled;
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled),
                                  BTK_POLICY_NEVER,
                                  BTK_POLICY_AUTOMATIC);

  main_vbox = btk_vbox_new (FALSE, 18);
  btk_container_set_border_width (BTK_CONTAINER (main_vbox), 12);
  btk_widget_show (main_vbox);

  btk_scrolled_window_add_with_viewport (BTK_SCROLLED_WINDOW (scrolled),
                                         main_vbox);
  btk_viewport_set_shadow_type (BTK_VIEWPORT (BTK_BIN(scrolled)->child),
                                BTK_SHADOW_NONE);

  priv->advanced_vbox = main_vbox;

  label = btk_label_new (_("Advanced"));
  btk_widget_show (label);

  btk_notebook_append_page (BTK_NOTEBOOK (priv->notebook),
                            scrolled, label);
}

static void
populate_dialog (BtkPrintUnixDialog *print_dialog)
{
  BtkPrintUnixDialogPrivate *priv = print_dialog->priv;
  BtkDialog *dialog = BTK_DIALOG (print_dialog);
  BtkWidget *vbox, *conflict_hbox, *image, *label;

  btk_dialog_set_has_separator (dialog, FALSE);
  btk_container_set_border_width (BTK_CONTAINER (dialog), 5);
  btk_box_set_spacing (BTK_BOX (dialog->vbox), 2); /* 2 * 5 + 2 = 12 */
  btk_container_set_border_width (BTK_CONTAINER (dialog->action_area), 5);
  btk_box_set_spacing (BTK_BOX (dialog->action_area), 6);

  vbox = btk_vbox_new (FALSE, 6);
  btk_container_set_border_width (BTK_CONTAINER (vbox), 5);
  btk_box_pack_start (BTK_BOX (dialog->vbox), vbox, TRUE, TRUE, 0);
  btk_widget_show (vbox);

  priv->notebook = btk_notebook_new ();
  btk_box_pack_start (BTK_BOX (vbox), priv->notebook, TRUE, TRUE, 0);
  btk_widget_show (priv->notebook);

  create_printer_list_model (print_dialog);

  create_main_page (print_dialog);
  create_page_setup_page (print_dialog);
  create_job_page (print_dialog);
  /* Translators: this will appear as tab label in print dialog. */
  create_optional_page (print_dialog, _("Image Quality"),
                        &priv->image_quality_table,
                        &priv->image_quality_page);
  /* Translators: this will appear as tab label in print dialog. */
  create_optional_page (print_dialog, _("Color"),
                        &priv->color_table,
                        &priv->color_page);
  /* Translators: this will appear as tab label in print dialog. */
  /* It's a typographical term, as in "Binding and finishing" */
  create_optional_page (print_dialog, _("Finishing"),
                        &priv->finishing_table,
                        &priv->finishing_page);
  create_advanced_page (print_dialog);

  priv->conflicts_widget = conflict_hbox = btk_hbox_new (FALSE, 12);
  btk_box_pack_end (BTK_BOX (vbox), conflict_hbox, FALSE, FALSE, 0);
  image = btk_image_new_from_stock (BTK_STOCK_DIALOG_WARNING, BTK_ICON_SIZE_MENU);
  btk_widget_show (image);
  btk_box_pack_start (BTK_BOX (conflict_hbox), image, FALSE, TRUE, 0);
  label = btk_label_new (_("Some of the settings in the dialog conflict"));
  btk_widget_show (label);
  btk_box_pack_start (BTK_BOX (conflict_hbox), label, FALSE, TRUE, 0);

  load_print_backends (print_dialog);
}

/**
 * btk_print_unix_dialog_new:
 * @title: (allow-none): Title of the dialog, or %NULL
 * @parent: (allow-none): Transient parent of the dialog, or %NULL
 *
 * Creates a new #BtkPrintUnixDialog.
 *
 * Return value: a new #BtkPrintUnixDialog
 *
 * Since: 2.10
 **/
BtkWidget *
btk_print_unix_dialog_new (const bchar *title,
                           BtkWindow   *parent)
{
  BtkWidget *result;
  const bchar *_title = _("Print");

  if (title)
    _title = title;

  result = g_object_new (BTK_TYPE_PRINT_UNIX_DIALOG,
                         "transient-for", parent,
                         "title", _title,
                         "has-separator", FALSE,
                         NULL);

  return result;
}

/**
 * btk_print_unix_dialog_get_selected_printer:
 * @dialog: a #BtkPrintUnixDialog
 *
 * Gets the currently selected printer.
 *
 * Returns: (transfer none): the currently selected printer
 *
 * Since: 2.10
 */
BtkPrinter *
btk_print_unix_dialog_get_selected_printer (BtkPrintUnixDialog *dialog)
{
  g_return_val_if_fail (BTK_IS_PRINT_UNIX_DIALOG (dialog), NULL);

  return dialog->priv->current_printer;
}

/**
 * btk_print_unix_dialog_set_page_setup:
 * @dialog: a #BtkPrintUnixDialog
 * @page_setup: a #BtkPageSetup
 *
 * Sets the page setup of the #BtkPrintUnixDialog.
 *
 * Since: 2.10
 */
void
btk_print_unix_dialog_set_page_setup (BtkPrintUnixDialog *dialog,
                                      BtkPageSetup       *page_setup)
{
  BtkPrintUnixDialogPrivate *priv;

  g_return_if_fail (BTK_IS_PRINT_UNIX_DIALOG (dialog));
  g_return_if_fail (BTK_IS_PAGE_SETUP (page_setup));

  priv = dialog->priv;

  if (priv->page_setup != page_setup)
    {
      g_object_unref (priv->page_setup);
      priv->page_setup = g_object_ref (page_setup);

      priv->page_setup_set = TRUE;

      g_object_notify (B_OBJECT (dialog), "page-setup");
    }
}

/**
 * btk_print_unix_dialog_get_page_setup:
 * @dialog: a #BtkPrintUnixDialog
 *
 * Gets the page setup that is used by the #BtkPrintUnixDialog.
 *
 * Returns: (transfer none): the page setup of @dialog.
 *
 * Since: 2.10
 */
BtkPageSetup *
btk_print_unix_dialog_get_page_setup (BtkPrintUnixDialog *dialog)
{
  g_return_val_if_fail (BTK_IS_PRINT_UNIX_DIALOG (dialog), NULL);

  return dialog->priv->page_setup;
}

/**
 * btk_print_unix_dialog_get_page_setup_set:
 * @dialog: a #BtkPrintUnixDialog
 *
 * Gets the page setup that is used by the #BtkPrintUnixDialog.
 *
 * Returns: whether a page setup was set by user.
 *
 * Since: 2.18
 */
bboolean
btk_print_unix_dialog_get_page_setup_set (BtkPrintUnixDialog *dialog)
{
  g_return_val_if_fail (BTK_IS_PRINT_UNIX_DIALOG (dialog), FALSE);

  return dialog->priv->page_setup_set;
}

/**
 * btk_print_unix_dialog_set_current_page:
 * @dialog: a #BtkPrintUnixDialog
 * @current_page: the current page number.
 *
 * Sets the current page number. If @current_page is not -1, this enables
 * the current page choice for the range of pages to print.
 *
 * Since: 2.10
 */
void
btk_print_unix_dialog_set_current_page (BtkPrintUnixDialog *dialog,
                                        bint                current_page)
{
  BtkPrintUnixDialogPrivate *priv;

  g_return_if_fail (BTK_IS_PRINT_UNIX_DIALOG (dialog));

  priv = dialog->priv;

  if (priv->current_page != current_page)
    {
      priv->current_page = current_page;

      if (priv->current_page_radio)
        btk_widget_set_sensitive (priv->current_page_radio, current_page != -1);

      g_object_notify (B_OBJECT (dialog), "current-page");
    }
}

/**
 * btk_print_unix_dialog_get_current_page:
 * @dialog: a #BtkPrintUnixDialog
 *
 * Gets the current page of the #BtkPrintDialog.
 *
 * Returns: the current page of @dialog
 *
 * Since: 2.10
 */
bint
btk_print_unix_dialog_get_current_page (BtkPrintUnixDialog *dialog)
{
  g_return_val_if_fail (BTK_IS_PRINT_UNIX_DIALOG (dialog), -1);

  return dialog->priv->current_page;
}

static bboolean
set_active_printer (BtkPrintUnixDialog *dialog,
                    const bchar        *printer_name)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;
  BtkTreeModel *model;
  BtkTreeIter iter, filter_iter;
  BtkTreeSelection *selection;
  BtkPrinter *printer;

  model = BTK_TREE_MODEL (priv->printer_list);

  if (btk_tree_model_get_iter_first (model, &iter))
    {
      do
        {
          btk_tree_model_get (BTK_TREE_MODEL (priv->printer_list), &iter,
                              PRINTER_LIST_COL_PRINTER_OBJ, &printer, -1);
          if (printer == NULL)
            continue;

          if (strcmp (btk_printer_get_name (printer), printer_name) == 0)
            {
              btk_tree_model_filter_convert_child_iter_to_iter (priv->printer_list_filter,
                                                                &filter_iter, &iter);

              selection = btk_tree_view_get_selection (BTK_TREE_VIEW (priv->printer_treeview));
              priv->internal_printer_change = TRUE;
              btk_tree_selection_select_iter (selection, &filter_iter);
              priv->internal_printer_change = FALSE;
              g_free (priv->waiting_for_printer);
              priv->waiting_for_printer = NULL;

              g_object_unref (printer);
              return TRUE;
            }

          g_object_unref (printer);

        } while (btk_tree_model_iter_next (model, &iter));
    }

  return FALSE;
}

/**
 * btk_print_unix_dialog_set_settings:
 * @dialog: a #BtkPrintUnixDialog
 * @settings: (allow-none): a #BtkPrintSettings, or %NULL
 *
 * Sets the #BtkPrintSettings for the #BtkPrintUnixDialog. Typically,
 * this is used to restore saved print settings from a previous print
 * operation before the print dialog is shown.
 *
 * Since: 2.10
 **/
void
btk_print_unix_dialog_set_settings (BtkPrintUnixDialog *dialog,
                                    BtkPrintSettings   *settings)
{
  BtkPrintUnixDialogPrivate *priv;
  const bchar *printer;
  BtkPageRange *ranges;
  bint num_ranges;

  g_return_if_fail (BTK_IS_PRINT_UNIX_DIALOG (dialog));
  g_return_if_fail (settings == NULL || BTK_IS_PRINT_SETTINGS (settings));

  priv = dialog->priv;

  if (settings != NULL)
    {
      dialog_set_collate (dialog, btk_print_settings_get_collate (settings));
      dialog_set_reverse (dialog, btk_print_settings_get_reverse (settings));
      dialog_set_n_copies (dialog, btk_print_settings_get_n_copies (settings));
      dialog_set_scale (dialog, btk_print_settings_get_scale (settings));
      dialog_set_page_set (dialog, btk_print_settings_get_page_set (settings));
      dialog_set_print_pages (dialog, btk_print_settings_get_print_pages (settings));
      ranges = btk_print_settings_get_page_ranges (settings, &num_ranges);
      if (ranges)
        {
          dialog_set_page_ranges (dialog, ranges, num_ranges);
          g_free (ranges);
        }

      priv->format_for_printer =
        g_strdup (btk_print_settings_get (settings, "format-for-printer"));
    }

  if (priv->initial_settings)
    g_object_unref (priv->initial_settings);

  priv->initial_settings = settings;

  g_free (priv->waiting_for_printer);
  priv->waiting_for_printer = NULL;

  if (settings)
    {
      g_object_ref (settings);

      printer = btk_print_settings_get_printer (settings);

      if (printer && !set_active_printer (dialog, printer))
        priv->waiting_for_printer = g_strdup (printer);
    }

  g_object_notify (B_OBJECT (dialog), "print-settings");
}

/**
 * btk_print_unix_dialog_get_settings:
 * @dialog: a #BtkPrintUnixDialog
 *
 * Gets a new #BtkPrintSettings object that represents the
 * current values in the print dialog. Note that this creates a
 * <emphasis>new object</emphasis>, and you need to unref it
 * if don't want to keep it.
 *
 * Returns: a new #BtkPrintSettings object with the values from @dialog
 *
 * Since: 2.10
 */
BtkPrintSettings *
btk_print_unix_dialog_get_settings (BtkPrintUnixDialog *dialog)
{
  BtkPrintUnixDialogPrivate *priv;
  BtkPrintSettings *settings;
  BtkPrintPages print_pages;
  BtkPageRange *ranges;
  bint n_ranges;

  g_return_val_if_fail (BTK_IS_PRINT_UNIX_DIALOG (dialog), NULL);

  priv = dialog->priv;
  settings = btk_print_settings_new ();

  if (priv->current_printer)
    btk_print_settings_set_printer (settings,
                                    btk_printer_get_name (priv->current_printer));
  else
    btk_print_settings_set_printer (settings, "default");

  btk_print_settings_set (settings, "format-for-printer",
                          priv->format_for_printer);

  btk_print_settings_set_collate (settings,
                                  dialog_get_collate (dialog));

  btk_print_settings_set_reverse (settings,
                                  dialog_get_reverse (dialog));

  btk_print_settings_set_n_copies (settings,
                                   dialog_get_n_copies (dialog));

  btk_print_settings_set_scale (settings,
                                dialog_get_scale (dialog));

  btk_print_settings_set_page_set (settings,
                                   dialog_get_page_set (dialog));

  print_pages = dialog_get_print_pages (dialog);
  btk_print_settings_set_print_pages (settings, print_pages);

  ranges = dialog_get_page_ranges (dialog, &n_ranges);
  if (ranges)
    {
      btk_print_settings_set_page_ranges  (settings, ranges, n_ranges);
      g_free (ranges);
    }

  /* TODO: print when. How to handle? */

  if (priv->current_printer)
    _btk_printer_get_settings_from_options (priv->current_printer,
                                            priv->options,
                                            settings);

  return settings;
}

/**
 * btk_print_unix_dialog_add_custom_tab:
 * @dialog: a #BtkPrintUnixDialog
 * @child: the widget to put in the custom tab
 * @tab_label: the widget to use as tab label
 *
 * Adds a custom tab to the print dialog.
 *
 * Since: 2.10
 */
void
btk_print_unix_dialog_add_custom_tab (BtkPrintUnixDialog *dialog,
                                      BtkWidget          *child,
                                      BtkWidget          *tab_label)
{
  btk_notebook_insert_page (BTK_NOTEBOOK (dialog->priv->notebook),
                            child, tab_label, 2);
  btk_widget_show (child);
  btk_widget_show (tab_label);
}

/**
 * btk_print_unix_dialog_set_manual_capabilities:
 * @dialog: a #BtkPrintUnixDialog
 * @capabilities: the printing capabilities of your application
 *
 * This lets you specify the printing capabilities your application
 * supports. For instance, if you can handle scaling the output then
 * you pass #BTK_PRINT_CAPABILITY_SCALE. If you don't pass that, then
 * the dialog will only let you select the scale if the printing
 * system automatically handles scaling.
 *
 * Since: 2.10
 */
void
btk_print_unix_dialog_set_manual_capabilities (BtkPrintUnixDialog   *dialog,
                                               BtkPrintCapabilities  capabilities)
{
  BtkPrintUnixDialogPrivate *priv = dialog->priv;

  if (priv->manual_capabilities != capabilities)
    {
      priv->manual_capabilities = capabilities;
      update_dialog_from_capabilities (dialog);

      if (priv->current_printer)
        {
          BtkTreeSelection *selection;

          selection = btk_tree_view_get_selection (BTK_TREE_VIEW (priv->printer_treeview));

          g_object_unref (priv->current_printer);
          priv->current_printer = NULL;
          priv->internal_printer_change = TRUE;
          selected_printer_changed (selection, dialog);
          priv->internal_printer_change = FALSE;
       }

      g_object_notify (B_OBJECT (dialog), "manual-capabilities");
    }
}

/**
 * btk_print_unix_dialog_get_manual_capabilities:
 * @dialog: a #BtkPrintUnixDialog
 *
 * Gets the value of #BtkPrintUnixDialog::manual-capabilities property.
 *
 * Returns: the printing capabilities
 *
 * Since: 2.18
 */
BtkPrintCapabilities
btk_print_unix_dialog_get_manual_capabilities (BtkPrintUnixDialog *dialog)
{
  g_return_val_if_fail (BTK_IS_PRINT_UNIX_DIALOG (dialog), FALSE);

  return dialog->priv->manual_capabilities;
}

/**
 * btk_print_unix_dialog_set_support_selection:
 * @dialog: a #BtkPrintUnixDialog
 * @support_selection: %TRUE to allow print selection
 *
 * Sets whether the print dialog allows user to print a selection.
 *
 * Since: 2.18
 */
void
btk_print_unix_dialog_set_support_selection (BtkPrintUnixDialog *dialog,
                                             bboolean            support_selection)
{
  BtkPrintUnixDialogPrivate *priv;

  g_return_if_fail (BTK_IS_PRINT_UNIX_DIALOG (dialog));

  priv = dialog->priv;

  support_selection = support_selection != FALSE;
  if (priv->support_selection != support_selection)
    {
      priv->support_selection = support_selection;

      if (priv->selection_radio)
        {
          if (support_selection)
            {
              btk_widget_set_sensitive (priv->selection_radio, priv->has_selection);
              btk_table_set_row_spacing (BTK_TABLE (priv->range_table),
                                         2,
                                         btk_table_get_default_row_spacing (BTK_TABLE (priv->range_table)));
              btk_widget_show (priv->selection_radio);
            }
          else
            {
              btk_widget_set_sensitive (priv->selection_radio, FALSE);
              btk_table_set_row_spacing (BTK_TABLE (priv->range_table), 2, 0);
              btk_widget_hide (priv->selection_radio);
            }
        }

      g_object_notify (B_OBJECT (dialog), "support-selection");
    }
}

/**
 * btk_print_unix_dialog_get_support_selection:
 * @dialog: a #BtkPrintUnixDialog
 *
 * Gets the value of #BtkPrintUnixDialog::support-selection property.
 *
 * Returns: whether the application supports print of selection
 *
 * Since: 2.18
 */
bboolean
btk_print_unix_dialog_get_support_selection (BtkPrintUnixDialog *dialog)
{
  g_return_val_if_fail (BTK_IS_PRINT_UNIX_DIALOG (dialog), FALSE);

  return dialog->priv->support_selection;
}

/**
 * btk_print_unix_dialog_set_has_selection:
 * @dialog: a #BtkPrintUnixDialog
 * @has_selection: %TRUE indicates that a selection exists
 *
 * Sets whether a selection exists.
 *
 * Since: 2.18
 */
void
btk_print_unix_dialog_set_has_selection (BtkPrintUnixDialog *dialog,
                                         bboolean            has_selection)
{
  BtkPrintUnixDialogPrivate *priv;

  g_return_if_fail (BTK_IS_PRINT_UNIX_DIALOG (dialog));

  priv = dialog->priv;

  has_selection = has_selection != FALSE;
  if (priv->has_selection != has_selection)
    {
      priv->has_selection = has_selection;

      if (priv->selection_radio)
        {
          if (priv->support_selection)
            btk_widget_set_sensitive (priv->selection_radio, has_selection);
          else
            btk_widget_set_sensitive (priv->selection_radio, FALSE);
        }

      g_object_notify (B_OBJECT (dialog), "has-selection");
    }
}

/**
 * btk_print_unix_dialog_get_has_selection:
 * @dialog: a #BtkPrintUnixDialog
 *
 * Gets the value of #BtkPrintUnixDialog::has-selection property.
 *
 * Returns: whether there is a selection
 *
 * Since: 2.18
 */
bboolean
btk_print_unix_dialog_get_has_selection (BtkPrintUnixDialog *dialog)
{
  g_return_val_if_fail (BTK_IS_PRINT_UNIX_DIALOG (dialog), FALSE);

  return dialog->priv->has_selection;
}

/**
 * btk_print_unix_dialog_set_embed_page_setup
 * @dialog: a #BtkPrintUnixDialog
 * @embed: embed page setup selection
 *
 * Embed page size combo box and orientation combo box into page setup page.
 *
 * Since: 2.18
 */
void
btk_print_unix_dialog_set_embed_page_setup (BtkPrintUnixDialog *dialog,
                                            bboolean            embed)
{
  BtkPrintUnixDialogPrivate *priv;

  g_return_if_fail (BTK_IS_PRINT_UNIX_DIALOG (dialog));

  priv = dialog->priv;

  embed = embed != FALSE;
  if (priv->embed_page_setup != embed)
    {
      priv->embed_page_setup = embed;

      btk_widget_set_sensitive (priv->paper_size_combo, priv->embed_page_setup);
      btk_widget_set_sensitive (priv->orientation_combo, priv->embed_page_setup);

      if (priv->embed_page_setup)
        {
          if (priv->paper_size_combo != NULL)
            g_signal_connect (priv->paper_size_combo, "changed", G_CALLBACK (paper_size_changed), dialog);

          if (priv->orientation_combo)
            g_signal_connect (priv->orientation_combo, "changed", G_CALLBACK (orientation_changed), dialog);
        }
      else
        {
          if (priv->paper_size_combo != NULL)
            g_signal_handlers_disconnect_by_func (priv->paper_size_combo, G_CALLBACK (paper_size_changed), dialog);

          if (priv->orientation_combo)
            g_signal_handlers_disconnect_by_func (priv->orientation_combo, G_CALLBACK (orientation_changed), dialog);
        }

      priv->internal_page_setup_change = TRUE;
      update_paper_sizes (dialog);
      priv->internal_page_setup_change = FALSE;
    }
}

/**
 * btk_print_unix_dialog_get_embed_page_setup:
 * @dialog: a #BtkPrintUnixDialog
 *
 * Gets the value of #BtkPrintUnixDialog::embed-page-setup property.
 *
 * Returns: whether there is a selection
 *
 * Since: 2.18
 */
bboolean
btk_print_unix_dialog_get_embed_page_setup (BtkPrintUnixDialog *dialog)
{
  g_return_val_if_fail (BTK_IS_PRINT_UNIX_DIALOG (dialog), FALSE);

  return dialog->priv->embed_page_setup;
}

#define __BTK_PRINT_UNIX_DIALOG_C__
#include "btkaliasdef.c"
