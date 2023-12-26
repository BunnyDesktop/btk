/* BTK - The GIMP Toolkit
 * btkprintbackend.h: Abstract printer backend interfaces
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
#include <string.h>

#include <bmodule.h>

#include "btkintl.h"
#include "btkmodules.h"
#include "btkmarshalers.h"
#include "btkprivate.h"
#include "btkprintbackend.h"
#include "btkalias.h"

#define BTK_PRINT_BACKEND_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), BTK_TYPE_PRINT_BACKEND, BtkPrintBackendPrivate))

static void btk_print_backend_dispose      (GObject      *object);
static void btk_print_backend_set_property (GObject      *object,
                                            guint         prop_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);
static void btk_print_backend_get_property (GObject      *object,
                                            guint         prop_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);

struct _BtkPrintBackendPrivate
{
  GHashTable *printers;
  guint printer_list_requested : 1;
  guint printer_list_done : 1;
  BtkPrintBackendStatus status;
  char **auth_info_required;
  char **auth_info;
};

enum {
  PRINTER_LIST_CHANGED,
  PRINTER_LIST_DONE,
  PRINTER_ADDED,
  PRINTER_REMOVED,
  PRINTER_STATUS_CHANGED,
  REQUEST_PASSWORD,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

enum 
{ 
  PROP_ZERO,
  PROP_STATUS
};

static GObjectClass *backend_parent_class;

GQuark
btk_print_backend_error_quark (void)
{
  static GQuark quark = 0;
  if (quark == 0)
    quark = g_quark_from_static_string ("btk-print-backend-error-quark");
  return quark;
}

/*****************************************
 *     BtkPrintBackendModule modules     *
 *****************************************/

typedef struct _BtkPrintBackendModule BtkPrintBackendModule;
typedef struct _BtkPrintBackendModuleClass BtkPrintBackendModuleClass;

struct _BtkPrintBackendModule
{
  GTypeModule parent_instance;
  
  GModule *library;

  void             (*init)     (GTypeModule    *module);
  void             (*exit)     (void);
  BtkPrintBackend* (*create)   (void);

  gchar *path;
};

struct _BtkPrintBackendModuleClass
{
  GTypeModuleClass parent_class;
};

G_DEFINE_TYPE (BtkPrintBackendModule, _btk_print_backend_module, G_TYPE_TYPE_MODULE)
#define BTK_TYPE_PRINT_BACKEND_MODULE      (_btk_print_backend_module_get_type ())
#define BTK_PRINT_BACKEND_MODULE(module)   (G_TYPE_CHECK_INSTANCE_CAST ((module), BTK_TYPE_PRINT_BACKEND_MODULE, BtkPrintBackendModule))

static GSList *loaded_backends;

static gboolean
btk_print_backend_module_load (GTypeModule *module)
{
  BtkPrintBackendModule *pb_module = BTK_PRINT_BACKEND_MODULE (module); 
  gpointer initp, exitp, createp;
 
  pb_module->library = g_module_open (pb_module->path, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);
  if (!pb_module->library)
    {
      g_warning ("%s", g_module_error());
      return FALSE;
    }
  
  /* extract symbols from the lib */
  if (!g_module_symbol (pb_module->library, "pb_module_init",
			&initp) ||
      !g_module_symbol (pb_module->library, "pb_module_exit", 
			&exitp) ||
      !g_module_symbol (pb_module->library, "pb_module_create", 
			&createp))
    {
      g_warning ("%s", g_module_error());
      g_module_close (pb_module->library);
      
      return FALSE;
    }

  pb_module->init = initp;
  pb_module->exit = exitp;
  pb_module->create = createp;

  /* call the printbackend's init function to let it */
  /* setup anything it needs to set up. */
  pb_module->init (module);

  return TRUE;
}

static void
btk_print_backend_module_unload (GTypeModule *module)
{
  BtkPrintBackendModule *pb_module = BTK_PRINT_BACKEND_MODULE (module);
  
  pb_module->exit();

  g_module_close (pb_module->library);
  pb_module->library = NULL;

  pb_module->init = NULL;
  pb_module->exit = NULL;
  pb_module->create = NULL;
}

/* This only will ever be called if an error occurs during
 * initialization
 */
static void
btk_print_backend_module_finalize (GObject *object)
{
  BtkPrintBackendModule *module = BTK_PRINT_BACKEND_MODULE (object);

  g_free (module->path);

  G_OBJECT_CLASS (_btk_print_backend_module_parent_class)->finalize (object);
}

static void
_btk_print_backend_module_class_init (BtkPrintBackendModuleClass *class)
{
  GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (class);
  GObjectClass *bobject_class = G_OBJECT_CLASS (class);

  module_class->load = btk_print_backend_module_load;
  module_class->unload = btk_print_backend_module_unload;

  bobject_class->finalize = btk_print_backend_module_finalize;
}

static void 
btk_print_backend_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  BtkPrintBackend *backend = BTK_PRINT_BACKEND (object);
  BtkPrintBackendPrivate *priv;

  priv = backend->priv = BTK_PRINT_BACKEND_GET_PRIVATE (backend); 

  switch (prop_id)
    {
    case PROP_STATUS:
      priv->status = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
btk_print_backend_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  BtkPrintBackend *backend = BTK_PRINT_BACKEND (object);
  BtkPrintBackendPrivate *priv;

  priv = backend->priv = BTK_PRINT_BACKEND_GET_PRIVATE (backend); 

  switch (prop_id)
    {
    case PROP_STATUS:
      g_value_set_int (value, priv->status);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_btk_print_backend_module_init (BtkPrintBackendModule *pb_module)
{
}

static BtkPrintBackend *
_btk_print_backend_module_create (BtkPrintBackendModule *pb_module)
{
  BtkPrintBackend *pb;
  
  if (g_type_module_use (G_TYPE_MODULE (pb_module)))
    {
      pb = pb_module->create ();
      g_type_module_unuse (G_TYPE_MODULE (pb_module));
      return pb;
    }
  return NULL;
}

static BtkPrintBackend *
_btk_print_backend_create (const gchar *backend_name)
{
  GSList *l;
  gchar *module_path;
  gchar *full_name;
  BtkPrintBackendModule *pb_module;
  BtkPrintBackend *pb;

  for (l = loaded_backends; l != NULL; l = l->next)
    {
      pb_module = l->data;
      
      if (strcmp (G_TYPE_MODULE (pb_module)->name, backend_name) == 0)
	return _btk_print_backend_module_create (pb_module);
    }

  pb = NULL;
  if (g_module_supported ())
    {
      full_name = g_strconcat ("printbackend-", backend_name, NULL);
      module_path = _btk_find_module (full_name, "printbackends");
      g_free (full_name);

      if (module_path)
	{
	  pb_module = g_object_new (BTK_TYPE_PRINT_BACKEND_MODULE, NULL);

	  g_type_module_set_name (G_TYPE_MODULE (pb_module), backend_name);
	  pb_module->path = g_strdup (module_path);

	  loaded_backends = g_slist_prepend (loaded_backends,
		   		             pb_module);

	  pb = _btk_print_backend_module_create (pb_module);

	  /* Increase use-count so that we don't unload print backends.
	   * There is a problem with module unloading in the cups module,
	   * see cups_dispatch_watch_finalize for details. 
	   */
	  g_type_module_use (G_TYPE_MODULE (pb_module));
	}
      
      g_free (module_path);
    }

  return pb;
}

/**
 * btk_printer_backend_load_modules:
 *
 * Return value: (element-type BtkPrintBackend) (transfer container):
 */
GList *
btk_print_backend_load_modules (void)
{
  GList *result;
  BtkPrintBackend *backend;
  gchar *setting;
  gchar **backends;
  gint i;
  BtkSettings *settings;

  result = NULL;

  settings = btk_settings_get_default ();
  if (settings)
    g_object_get (settings, "btk-print-backends", &setting, NULL);
  else
    setting = g_strdup (BTK_PRINT_BACKENDS);

  backends = g_strsplit (setting, ",", -1);

  for (i = 0; backends[i]; i++)
    {
      g_strchug (backends[i]);
      g_strchomp (backends[i]);
      backend = _btk_print_backend_create (backends[i]);
      
      if (backend)
        result = g_list_append (result, backend);
    }

  g_strfreev (backends);
  g_free (setting);

  return result;
}

/*****************************************
 *             BtkPrintBackend           *
 *****************************************/

G_DEFINE_TYPE (BtkPrintBackend, btk_print_backend, G_TYPE_OBJECT)

static void                 fallback_printer_request_details       (BtkPrinter          *printer);
static gboolean             fallback_printer_mark_conflicts        (BtkPrinter          *printer,
								    BtkPrinterOptionSet *options);
static gboolean             fallback_printer_get_hard_margins      (BtkPrinter          *printer,
                                                                    gdouble             *top,
                                                                    gdouble             *bottom,
                                                                    gdouble             *left,
                                                                    gdouble             *right);
static GList *              fallback_printer_list_papers           (BtkPrinter          *printer);
static BtkPageSetup *       fallback_printer_get_default_page_size (BtkPrinter          *printer);
static BtkPrintCapabilities fallback_printer_get_capabilities      (BtkPrinter          *printer);
static void                 request_password                       (BtkPrintBackend     *backend,
                                                                    gpointer             auth_info_required,
                                                                    gpointer             auth_info_default,
                                                                    gpointer             auth_info_display,
                                                                    gpointer             auth_info_visible,
                                                                    const gchar         *prompt);
  
static void
btk_print_backend_class_init (BtkPrintBackendClass *class)
{
  GObjectClass *object_class;
  object_class = (GObjectClass *) class;

  backend_parent_class = g_type_class_peek_parent (class);
  
  object_class->dispose = btk_print_backend_dispose;
  object_class->set_property = btk_print_backend_set_property;
  object_class->get_property = btk_print_backend_get_property;

  class->printer_request_details = fallback_printer_request_details;
  class->printer_mark_conflicts = fallback_printer_mark_conflicts;
  class->printer_get_hard_margins = fallback_printer_get_hard_margins;
  class->printer_list_papers = fallback_printer_list_papers;
  class->printer_get_default_page_size = fallback_printer_get_default_page_size;
  class->printer_get_capabilities = fallback_printer_get_capabilities;
  class->request_password = request_password;
  
  g_object_class_install_property (object_class, 
                                   PROP_STATUS,
                                   g_param_spec_int ("status",
                                                     "Status",
                                                     "The status of the print backend",
                                                     BTK_PRINT_BACKEND_STATUS_UNKNOWN,
                                                     BTK_PRINT_BACKEND_STATUS_UNAVAILABLE,
                                                     BTK_PRINT_BACKEND_STATUS_UNKNOWN,
                                                     BTK_PARAM_READWRITE)); 

  g_type_class_add_private (class, sizeof (BtkPrintBackendPrivate));
  
  signals[PRINTER_LIST_CHANGED] =
    g_signal_new (I_("printer-list-changed"),
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkPrintBackendClass, printer_list_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  signals[PRINTER_LIST_DONE] =
    g_signal_new (I_("printer-list-done"),
		    G_TYPE_FROM_CLASS (class),
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (BtkPrintBackendClass, printer_list_done),
		    NULL, NULL,
		    g_cclosure_marshal_VOID__VOID,
		    G_TYPE_NONE, 0);
  signals[PRINTER_ADDED] =
    g_signal_new (I_("printer-added"),
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkPrintBackendClass, printer_added),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1, BTK_TYPE_PRINTER);
  signals[PRINTER_REMOVED] =
    g_signal_new (I_("printer-removed"),
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkPrintBackendClass, printer_removed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1, BTK_TYPE_PRINTER);
  signals[PRINTER_STATUS_CHANGED] =
    g_signal_new (I_("printer-status-changed"),
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkPrintBackendClass, printer_status_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1, BTK_TYPE_PRINTER);
  signals[REQUEST_PASSWORD] =
    g_signal_new (I_("request-password"),
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkPrintBackendClass, request_password),
		  NULL, NULL,
		  _btk_marshal_VOID__POINTER_POINTER_POINTER_POINTER_STRING,
		  G_TYPE_NONE, 5, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_STRING);
}

static void
btk_print_backend_init (BtkPrintBackend *backend)
{
  BtkPrintBackendPrivate *priv;

  priv = backend->priv = BTK_PRINT_BACKEND_GET_PRIVATE (backend); 

  priv->printers = g_hash_table_new_full (g_str_hash, g_str_equal, 
					  (GDestroyNotify) g_free,
					  (GDestroyNotify) g_object_unref);
  priv->auth_info_required = NULL;
  priv->auth_info = NULL;
}

static void
btk_print_backend_dispose (GObject *object)
{
  BtkPrintBackend *backend;
  BtkPrintBackendPrivate *priv;

  backend = BTK_PRINT_BACKEND (object);
  priv = backend->priv;

  /* We unref the printers in dispose, not in finalize so that
   * we can break refcount cycles with btk_print_backend_destroy 
   */
  if (priv->printers)
    {
      g_hash_table_destroy (priv->printers);
      priv->printers = NULL;
    }

  backend_parent_class->dispose (object);
}


static void
fallback_printer_request_details (BtkPrinter *printer)
{
}

static gboolean
fallback_printer_mark_conflicts (BtkPrinter          *printer,
				 BtkPrinterOptionSet *options)
{
  return FALSE;
}

static gboolean
fallback_printer_get_hard_margins (BtkPrinter *printer,
				   gdouble    *top,
				   gdouble    *bottom,
				   gdouble    *left,
				   gdouble    *right)
{
  return FALSE;
}

static GList *
fallback_printer_list_papers (BtkPrinter *printer)
{
  return NULL;
}

static BtkPageSetup *
fallback_printer_get_default_page_size (BtkPrinter *printer)
{
  return NULL;
}

static BtkPrintCapabilities
fallback_printer_get_capabilities (BtkPrinter *printer)
{
  return 0;
}


static void
printer_hash_to_sorted_active_list (const gchar  *key,
                                    gpointer      value,
                                    GList       **out_list)
{
  BtkPrinter *printer;

  printer = BTK_PRINTER (value);

  if (btk_printer_get_name (printer) == NULL)
    return;

  if (!btk_printer_is_active (printer))
    return;

  *out_list = g_list_insert_sorted (*out_list, value, (GCompareFunc) btk_printer_compare);
}


void
btk_print_backend_add_printer (BtkPrintBackend *backend,
			       BtkPrinter      *printer)
{
  BtkPrintBackendPrivate *priv;
  
  g_return_if_fail (BTK_IS_PRINT_BACKEND (backend));

  priv = backend->priv;

  if (!priv->printers)
    return;
  
  g_hash_table_insert (priv->printers,
		       g_strdup (btk_printer_get_name (printer)), 
		       g_object_ref (printer));
}

void
btk_print_backend_remove_printer (BtkPrintBackend *backend,
				  BtkPrinter      *printer)
{
  BtkPrintBackendPrivate *priv;
  
  g_return_if_fail (BTK_IS_PRINT_BACKEND (backend));
  priv = backend->priv;

  if (!priv->printers)
    return;
  
  g_hash_table_remove (priv->printers,
		       btk_printer_get_name (printer));
}

void
btk_print_backend_set_list_done (BtkPrintBackend *backend)
{
  if (!backend->priv->printer_list_done)
    {
      backend->priv->printer_list_done = TRUE;
      g_signal_emit (backend, signals[PRINTER_LIST_DONE], 0);
    }
}


/**
 * btk_print_backend_get_printer_list:
 *
 * Return value: (element-type BtkPrinter) (transfer container):
 */
GList *
btk_print_backend_get_printer_list (BtkPrintBackend *backend)
{
  BtkPrintBackendPrivate *priv;
  GList *result;
  
  g_return_val_if_fail (BTK_IS_PRINT_BACKEND (backend), NULL);

  priv = backend->priv;

  result = NULL;
  if (priv->printers != NULL)
    g_hash_table_foreach (priv->printers,
                          (GHFunc) printer_hash_to_sorted_active_list,
                          &result);

  if (!priv->printer_list_requested && priv->printers != NULL)
    {
      if (BTK_PRINT_BACKEND_GET_CLASS (backend)->request_printer_list)
	BTK_PRINT_BACKEND_GET_CLASS (backend)->request_printer_list (backend);
      priv->printer_list_requested = TRUE;
    }

  return result;
}

gboolean
btk_print_backend_printer_list_is_done (BtkPrintBackend *print_backend)
{
  g_return_val_if_fail (BTK_IS_PRINT_BACKEND (print_backend), TRUE);

  return print_backend->priv->printer_list_done;
}

BtkPrinter *
btk_print_backend_find_printer (BtkPrintBackend *backend,
                                const gchar     *printer_name)
{
  BtkPrintBackendPrivate *priv;
  BtkPrinter *printer;
  
  g_return_val_if_fail (BTK_IS_PRINT_BACKEND (backend), NULL);

  priv = backend->priv;

  if (priv->printers)
    printer = g_hash_table_lookup (priv->printers, printer_name);
  else
    printer = NULL;

  return printer;  
}

void
btk_print_backend_print_stream (BtkPrintBackend        *backend,
                                BtkPrintJob            *job,
                                BUNNYIOChannel             *data_io,
                                BtkPrintJobCompleteFunc callback,
                                gpointer                user_data,
				GDestroyNotify          dnotify)
{
  g_return_if_fail (BTK_IS_PRINT_BACKEND (backend));

  BTK_PRINT_BACKEND_GET_CLASS (backend)->print_stream (backend,
						       job,
						       data_io,
						       callback,
						       user_data,
						       dnotify);
}

void 
btk_print_backend_set_password (BtkPrintBackend  *backend,
                                gchar           **auth_info_required,
                                gchar           **auth_info)
{
  g_return_if_fail (BTK_IS_PRINT_BACKEND (backend));

  if (BTK_PRINT_BACKEND_GET_CLASS (backend)->set_password)
    BTK_PRINT_BACKEND_GET_CLASS (backend)->set_password (backend, auth_info_required, auth_info);
}

static void
store_entry (BtkEntry  *entry,
             gpointer   user_data)
{
  gchar **data = (gchar **) user_data;

  if (*data != NULL)
    {
      memset (*data, 0, strlen (*data));
      g_free (*data);
    }

  *data = g_strdup (btk_entry_get_text (entry));
}

static void
password_dialog_response (BtkWidget       *dialog,
                          gint             response_id,
                          BtkPrintBackend *backend)
{
  BtkPrintBackendPrivate *priv = backend->priv;
  gint i;

  if (response_id == BTK_RESPONSE_OK)
    btk_print_backend_set_password (backend, priv->auth_info_required, priv->auth_info);
  else
    btk_print_backend_set_password (backend, priv->auth_info_required, NULL);

  for (i = 0; i < g_strv_length (priv->auth_info_required); i++)
    if (priv->auth_info[i] != NULL)
      {
        memset (priv->auth_info[i], 0, strlen (priv->auth_info[i]));
        g_free (priv->auth_info[i]);
        priv->auth_info[i] = NULL;
      }
  g_free (priv->auth_info);
  priv->auth_info = NULL;

  g_strfreev (priv->auth_info_required);

  btk_widget_destroy (dialog);

  g_object_unref (backend);
}

static void
request_password (BtkPrintBackend  *backend,
                  gpointer          auth_info_required,
                  gpointer          auth_info_default,
                  gpointer          auth_info_display,
                  gpointer          auth_info_visible,
                  const gchar      *prompt)
{
  BtkPrintBackendPrivate *priv = backend->priv;
  BtkWidget *dialog, *box, *main_box, *label, *icon, *vbox, *entry;
  BtkWidget *focus = NULL;
  gchar     *markup;
  gint       length;
  gint       i;
  gchar    **ai_required = (gchar **) auth_info_required;
  gchar    **ai_default = (gchar **) auth_info_default;
  gchar    **ai_display = (gchar **) auth_info_display;
  gboolean  *ai_visible = (gboolean *) auth_info_visible;

  priv->auth_info_required = g_strdupv (ai_required);
  length = g_strv_length (ai_required);
  priv->auth_info = g_new0 (gchar *, length + 1);

  dialog = btk_dialog_new_with_buttons ( _("Authentication"), NULL, BTK_DIALOG_MODAL, 
                                         BTK_STOCK_CANCEL, BTK_RESPONSE_CANCEL,
                                         BTK_STOCK_OK, BTK_RESPONSE_OK,
                                         NULL);

  btk_dialog_set_default_response (BTK_DIALOG (dialog), BTK_RESPONSE_OK);
  btk_dialog_set_has_separator (BTK_DIALOG (dialog), FALSE);

  main_box = btk_hbox_new (FALSE, 0);

  /* Left */
  icon = btk_image_new_from_stock (BTK_STOCK_DIALOG_AUTHENTICATION, BTK_ICON_SIZE_DIALOG);
  btk_misc_set_alignment (BTK_MISC (icon), 0.5, 0.0);
  btk_misc_set_padding (BTK_MISC (icon), 6, 6);


  /* Right */
  vbox = btk_vbox_new (FALSE, 0);
  btk_widget_set_size_request (BTK_WIDGET (vbox), 320, -1);

  /* Right - 1. */
  label = btk_label_new (NULL);
  markup = g_markup_printf_escaped ("<span weight=\"bold\" size=\"large\">%s</span>", prompt);
  btk_label_set_markup (BTK_LABEL (label), markup);
  btk_label_set_line_wrap (BTK_LABEL (label), TRUE);
  btk_widget_set_size_request (BTK_WIDGET (label), 320, -1);
  g_free (markup);


  /* Packing */
  btk_box_pack_start (BTK_BOX (BTK_DIALOG (dialog)->vbox), main_box, TRUE, FALSE, 0);

  btk_box_pack_start (BTK_BOX (main_box), icon, FALSE, FALSE, 6);
  btk_box_pack_start (BTK_BOX (main_box), vbox, FALSE, FALSE, 6);

  btk_box_pack_start (BTK_BOX (vbox), label, FALSE, TRUE, 6);
  
  /* Right - 2. */
  for (i = 0; i < length; i++)
    {
      priv->auth_info[i] = g_strdup (ai_default[i]);
      if (ai_display[i] != NULL)
        {
          box = btk_hbox_new (TRUE, 0);

          label = btk_label_new (ai_display[i]);
          btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);

          entry = btk_entry_new ();
          focus = entry;

          if (ai_default[i] != NULL)
            btk_entry_set_text (BTK_ENTRY (entry), ai_default[i]);

          btk_entry_set_visibility (BTK_ENTRY (entry), ai_visible[i]);
          btk_entry_set_activates_default (BTK_ENTRY (entry), TRUE);

          btk_box_pack_start (BTK_BOX (vbox), box, FALSE, TRUE, 6);

          btk_box_pack_start (BTK_BOX (box), label, TRUE, TRUE, 0);
          btk_box_pack_start (BTK_BOX (box), entry, TRUE, TRUE, 0);

          g_signal_connect (entry, "changed",
                            G_CALLBACK (store_entry), &(priv->auth_info[i]));
        }
    }

  if (focus != NULL)
    {
      btk_widget_grab_focus (focus);
      focus = NULL;
    }

  g_object_ref (backend);
  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (password_dialog_response), backend);

  btk_widget_show_all (dialog);
}

void
btk_print_backend_destroy (BtkPrintBackend *print_backend)
{
  /* The lifecycle of print backends and printers are tied, such that
   * the backend owns the printers, but the printers also ref the backend.
   * This is so that if the app has a reference to a printer its backend
   * will be around. However, this results in a cycle, which we break
   * with this call, which causes the print backend to release its printers.
   */
  g_object_run_dispose (G_OBJECT (print_backend));
}


#define __BTK_PRINT_BACKEND_C__
#include "btkaliasdef.c"
