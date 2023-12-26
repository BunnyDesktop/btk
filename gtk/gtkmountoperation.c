/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* BTK - The GIMP Toolkit
 * Copyright (C) Christian Kellner <gicmo@gnome.org>
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

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#include "config.h"

#include <string.h>

#include "btkmountoperationprivate.h"
#include "btkalignment.h"
#include "btkbox.h"
#include "btkentry.h"
#include "btkhbox.h"
#include "btkintl.h"
#include "btklabel.h"
#include "btkvbox.h"
#include "btkmessagedialog.h"
#include "btkmisc.h"
#include "btkmountoperation.h"
#include "btkprivate.h"
#include "btkradiobutton.h"
#include "btkstock.h"
#include "btktable.h"
#include "btkwindow.h"
#include "btktreeview.h"
#include "btktreeselection.h"
#include "btkcellrenderertext.h"
#include "btkcellrendererpixbuf.h"
#include "btkscrolledwindow.h"
#include "btkicontheme.h"
#include "btkimagemenuitem.h"
#include "btkmain.h"
#include "btkalias.h"

/**
 * SECTION:filesystem
 * @short_description: Functions for working with BUNNYIO
 * @Title: Filesystem utilities
 *
 * The functions and objects described here make working with BTK+ and
 * BUNNYIO more convenient.
 *
 * #BtkMountOperation is needed when mounting volumes:
 * It is an implementation of #GMountOperation that can be used with
 * BUNNYIO functions for mounting volumes such as
 * g_file_mount_enclosing_volume(), g_file_mount_mountable(),
 * g_volume_mount(), g_mount_unmount_with_operation() and others.
 *
 * When necessary, #BtkMountOperation shows dialogs to ask for
 * passwords, questions or show processes blocking unmount.
 *
 * btk_show_uri() is a convenient way to launch applications for URIs.
 *
 * Another object that is worth mentioning in this context is
 * #BdkAppLaunchContext, which provides visual feedback when lauching
 * applications.
 */

static void   btk_mount_operation_finalize     (GObject          *object);
static void   btk_mount_operation_set_property (GObject          *object,
                                                guint             prop_id,
                                                const GValue     *value,
                                                GParamSpec       *pspec);
static void   btk_mount_operation_get_property (GObject          *object,
                                                guint             prop_id,
                                                GValue           *value,
                                                GParamSpec       *pspec);

static void   btk_mount_operation_ask_password (GMountOperation *op,
                                                const char      *message,
                                                const char      *default_user,
                                                const char      *default_domain,
                                                GAskPasswordFlags flags);

static void   btk_mount_operation_ask_question (GMountOperation *op,
                                                const char      *message,
                                                const char      *choices[]);

static void   btk_mount_operation_show_processes (GMountOperation *op,
                                                  const char      *message,
                                                  GArray          *processes,
                                                  const char      *choices[]);

static void   btk_mount_operation_aborted      (GMountOperation *op);

G_DEFINE_TYPE (BtkMountOperation, btk_mount_operation, G_TYPE_MOUNT_OPERATION);

enum {
  PROP_0,
  PROP_PARENT,
  PROP_IS_SHOWING,
  PROP_SCREEN

};

struct _BtkMountOperationPrivate {
  BtkWindow *parent_window;
  BtkDialog *dialog;
  BdkScreen *screen;

  /* for the ask-password dialog */
  BtkWidget *entry_container;
  BtkWidget *username_entry;
  BtkWidget *domain_entry;
  BtkWidget *password_entry;
  BtkWidget *anonymous_toggle;

  GAskPasswordFlags ask_flags;
  GPasswordSave     password_save;
  gboolean          anonymous;

  /* for the show-processes dialog */
  BtkWidget *process_tree_view;
  BtkListStore *process_list_store;
};

static void
btk_mount_operation_class_init (BtkMountOperationClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  GMountOperationClass *mount_op_class = G_MOUNT_OPERATION_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtkMountOperationPrivate));

  object_class->finalize     = btk_mount_operation_finalize;
  object_class->get_property = btk_mount_operation_get_property;
  object_class->set_property = btk_mount_operation_set_property;

  mount_op_class->ask_password = btk_mount_operation_ask_password;
  mount_op_class->ask_question = btk_mount_operation_ask_question;
  mount_op_class->show_processes = btk_mount_operation_show_processes;
  mount_op_class->aborted = btk_mount_operation_aborted;

  g_object_class_install_property (object_class,
                                   PROP_PARENT,
                                   g_param_spec_object ("parent",
                                                        P_("Parent"),
                                                        P_("The parent window"),
                                                        BTK_TYPE_WINDOW,
                                                        BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_IS_SHOWING,
                                   g_param_spec_boolean ("is-showing",
                                                         P_("Is Showing"),
                                                         P_("Are we showing a dialog"),
                                                         FALSE,
                                                         BTK_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_SCREEN,
                                   g_param_spec_object ("screen",
                                                        P_("Screen"),
                                                        P_("The screen where this window will be displayed."),
                                                        BDK_TYPE_SCREEN,
                                                        BTK_PARAM_READWRITE));
}

static void
btk_mount_operation_init (BtkMountOperation *operation)
{
  operation->priv = G_TYPE_INSTANCE_GET_PRIVATE (operation,
                                                 BTK_TYPE_MOUNT_OPERATION,
                                                 BtkMountOperationPrivate);
}

static void
btk_mount_operation_finalize (GObject *object)
{
  BtkMountOperation *operation = BTK_MOUNT_OPERATION (object);
  BtkMountOperationPrivate *priv = operation->priv;

  if (priv->parent_window)
    {
      g_signal_handlers_disconnect_by_func (priv->parent_window,
                                            btk_widget_destroyed,
                                            &priv->parent_window);
      g_object_unref (priv->parent_window);
    }

  if (priv->screen)
    g_object_unref (priv->screen);

  G_OBJECT_CLASS (btk_mount_operation_parent_class)->finalize (object);
}

static void
btk_mount_operation_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  BtkMountOperation *operation = BTK_MOUNT_OPERATION (object);

  switch (prop_id)
    {
    case PROP_PARENT:
      btk_mount_operation_set_parent (operation, g_value_get_object (value));
      break;

    case PROP_SCREEN:
      btk_mount_operation_set_screen (operation, g_value_get_object (value));
      break;

    case PROP_IS_SHOWING:
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_mount_operation_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  BtkMountOperation *operation = BTK_MOUNT_OPERATION (object);
  BtkMountOperationPrivate *priv = operation->priv;

  switch (prop_id)
    {
    case PROP_PARENT:
      g_value_set_object (value, priv->parent_window);
      break;

    case PROP_IS_SHOWING:
      g_value_set_boolean (value, priv->dialog != NULL);
      break;

    case PROP_SCREEN:
      g_value_set_object (value, priv->screen);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
remember_button_toggled (BtkToggleButton   *button,
                         BtkMountOperation *operation)
{
  BtkMountOperationPrivate *priv = operation->priv;

  if (btk_toggle_button_get_active (button))
    {
      gpointer data;

      data = g_object_get_data (G_OBJECT (button), "password-save");
      priv->password_save = GPOINTER_TO_INT (data);
    }
}

static void
pw_dialog_got_response (BtkDialog         *dialog,
                        gint               response_id,
                        BtkMountOperation *mount_op)
{
  BtkMountOperationPrivate *priv = mount_op->priv;
  GMountOperation *op = G_MOUNT_OPERATION (mount_op);

  if (response_id == BTK_RESPONSE_OK)
    {
      const char *text;

      if (priv->ask_flags & G_ASK_PASSWORD_ANONYMOUS_SUPPORTED)
        g_mount_operation_set_anonymous (op, priv->anonymous);

      if (priv->username_entry)
        {
          text = btk_entry_get_text (BTK_ENTRY (priv->username_entry));
          g_mount_operation_set_username (op, text);
        }

      if (priv->domain_entry)
        {
          text = btk_entry_get_text (BTK_ENTRY (priv->domain_entry));
          g_mount_operation_set_domain (op, text);
        }

      if (priv->password_entry)
        {
          text = btk_entry_get_text (BTK_ENTRY (priv->password_entry));
          g_mount_operation_set_password (op, text);
        }

      if (priv->ask_flags & G_ASK_PASSWORD_SAVING_SUPPORTED)
        g_mount_operation_set_password_save (op, priv->password_save);

      g_mount_operation_reply (op, G_MOUNT_OPERATION_HANDLED);
    }
  else
    g_mount_operation_reply (op, G_MOUNT_OPERATION_ABORTED);

  priv->dialog = NULL;
  g_object_notify (G_OBJECT (op), "is-showing");
  btk_widget_destroy (BTK_WIDGET (dialog));
  g_object_unref (op);
}

static gboolean
entry_has_input (BtkWidget *entry_widget)
{
  const char *text;

  if (entry_widget == NULL)
    return TRUE;

  text = btk_entry_get_text (BTK_ENTRY (entry_widget));

  return text != NULL && text[0] != '\0';
}

static gboolean
pw_dialog_input_is_valid (BtkMountOperation *operation)
{
  BtkMountOperationPrivate *priv = operation->priv;
  gboolean is_valid = TRUE;

  /* We don't require password to be non-empty here
   * since there are situations where it is not needed,
   * see bug 578365.
   * We may add a way for the backend to specify that it
   * definitively needs a password.
   */
  is_valid = entry_has_input (priv->username_entry) &&
             entry_has_input (priv->domain_entry);

  return is_valid;
}

static void
pw_dialog_verify_input (BtkEditable       *editable,
                        BtkMountOperation *operation)
{
  BtkMountOperationPrivate *priv = operation->priv;
  gboolean is_valid;

  is_valid = pw_dialog_input_is_valid (operation);
  btk_dialog_set_response_sensitive (BTK_DIALOG (priv->dialog),
                                     BTK_RESPONSE_OK,
                                     is_valid);
}

static void
pw_dialog_anonymous_toggled (BtkWidget         *widget,
                             BtkMountOperation *operation)
{
  BtkMountOperationPrivate *priv = operation->priv;
  gboolean is_valid;

  priv->anonymous = widget == priv->anonymous_toggle;

  if (priv->anonymous)
    is_valid = TRUE;
  else
    is_valid = pw_dialog_input_is_valid (operation);

  btk_widget_set_sensitive (priv->entry_container, priv->anonymous == FALSE);
  btk_dialog_set_response_sensitive (BTK_DIALOG (priv->dialog),
                                     BTK_RESPONSE_OK,
                                     is_valid);
}


static void
pw_dialog_cycle_focus (BtkWidget         *widget,
                       BtkMountOperation *operation)
{
  BtkMountOperationPrivate *priv;
  BtkWidget *next_widget = NULL;

  priv = operation->priv;

  if (widget == priv->username_entry)
    {
      if (priv->domain_entry != NULL)
        next_widget = priv->domain_entry;
      else if (priv->password_entry != NULL)
        next_widget = priv->password_entry;
    }
  else if (widget == priv->domain_entry && priv->password_entry)
    next_widget = priv->password_entry;

  if (next_widget)
    btk_widget_grab_focus (next_widget);
  else if (pw_dialog_input_is_valid (operation))
    btk_window_activate_default (BTK_WINDOW (priv->dialog));
}

static BtkWidget *
table_add_entry (BtkWidget  *table,
                 int         row,
                 const char *label_text,
                 const char *value,
                 gpointer    user_data)
{
  BtkWidget *entry;
  BtkWidget *label;

  label = btk_label_new_with_mnemonic (label_text);
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);

  entry = btk_entry_new ();

  if (value)
    btk_entry_set_text (BTK_ENTRY (entry), value);

  btk_table_attach (BTK_TABLE (table), label,
                    0, 1, row, row + 1,
                    BTK_FILL, BTK_EXPAND | BTK_FILL, 0, 0);
  btk_table_attach_defaults (BTK_TABLE (table), entry,
                             1, 2, row, row + 1);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), entry);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (pw_dialog_verify_input), user_data);

  g_signal_connect (entry, "activate",
                    G_CALLBACK (pw_dialog_cycle_focus), user_data);

  return entry;
}

static void
btk_mount_operation_ask_password (GMountOperation   *mount_op,
                                  const char        *message,
                                  const char        *default_user,
                                  const char        *default_domain,
                                  GAskPasswordFlags  flags)
{
  BtkMountOperation *operation;
  BtkMountOperationPrivate *priv;
  BtkWidget *widget;
  BtkDialog *dialog;
  BtkWindow *window;
  BtkWidget *entry_container;
  BtkWidget *hbox, *main_vbox, *vbox, *icon;
  BtkWidget *table;
  BtkWidget *message_label;
  gboolean   can_anonymous;
  guint      rows;
  const gchar *secondary;

  operation = BTK_MOUNT_OPERATION (mount_op);
  priv = operation->priv;

  priv->ask_flags = flags;

  widget = btk_dialog_new ();
  dialog = BTK_DIALOG (widget);
  window = BTK_WINDOW (widget);

  priv->dialog = dialog;

  /* Set the dialog up with HIG properties */
  btk_dialog_set_has_separator (dialog, FALSE);
  btk_container_set_border_width (BTK_CONTAINER (dialog), 5);
  btk_box_set_spacing (BTK_BOX (dialog->vbox), 2); /* 2 * 5 + 2 = 12 */
  btk_container_set_border_width (BTK_CONTAINER (dialog->action_area), 5);
  btk_box_set_spacing (BTK_BOX (dialog->action_area), 6);

  btk_window_set_resizable (window, FALSE);
  btk_window_set_title (window, "");
  btk_window_set_icon_name (window, BTK_STOCK_DIALOG_AUTHENTICATION);

  btk_dialog_add_buttons (dialog,
                          BTK_STOCK_CANCEL, BTK_RESPONSE_CANCEL,
                          _("Co_nnect"), BTK_RESPONSE_OK,
                          NULL);
  btk_dialog_set_default_response (dialog, BTK_RESPONSE_OK);

  btk_dialog_set_alternative_button_order (dialog,
                                           BTK_RESPONSE_OK,
                                           BTK_RESPONSE_CANCEL,
                                           -1);

  /* Build contents */
  hbox = btk_hbox_new (FALSE, 12);
  btk_container_set_border_width (BTK_CONTAINER (hbox), 5);
  btk_box_pack_start (BTK_BOX (dialog->vbox), hbox, TRUE, TRUE, 0);

  icon = btk_image_new_from_stock (BTK_STOCK_DIALOG_AUTHENTICATION,
                                   BTK_ICON_SIZE_DIALOG);

  btk_misc_set_alignment (BTK_MISC (icon), 0.5, 0.0);
  btk_box_pack_start (BTK_BOX (hbox), icon, FALSE, FALSE, 0);

  main_vbox = btk_vbox_new (FALSE, 18);
  btk_box_pack_start (BTK_BOX (hbox), main_vbox, TRUE, TRUE, 0);

  secondary = strstr (message, "\n");
  if (secondary != NULL)
    {
      gchar *s;
      gchar *primary;

      primary = g_strndup (message, secondary - message + 1);
      s = g_strdup_printf ("<big><b>%s</b></big>%s", primary, secondary);

      message_label = btk_label_new (NULL);
      btk_label_set_markup (BTK_LABEL (message_label), s);
      btk_misc_set_alignment (BTK_MISC (message_label), 0.0, 0.5);
      btk_label_set_line_wrap (BTK_LABEL (message_label), TRUE);
      btk_box_pack_start (BTK_BOX (main_vbox), BTK_WIDGET (message_label),
                          FALSE, TRUE, 0);

      g_free (s);
      g_free (primary);
    }
  else
    {
      message_label = btk_label_new (message);
      btk_misc_set_alignment (BTK_MISC (message_label), 0.0, 0.5);
      btk_label_set_line_wrap (BTK_LABEL (message_label), TRUE);
      btk_box_pack_start (BTK_BOX (main_vbox), BTK_WIDGET (message_label),
                          FALSE, FALSE, 0);
    }

  vbox = btk_vbox_new (FALSE, 6);
  btk_box_pack_start (BTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);

  can_anonymous = flags & G_ASK_PASSWORD_ANONYMOUS_SUPPORTED;

  priv->anonymous_toggle = NULL;
  if (can_anonymous)
    {
      BtkWidget *anon_box;
      BtkWidget *choice;
      GSList    *group;

      anon_box = btk_vbox_new (FALSE, 6);
      btk_box_pack_start (BTK_BOX (vbox), anon_box,
                          FALSE, FALSE, 0);

      choice = btk_radio_button_new_with_mnemonic (NULL, _("Connect _anonymously"));
      btk_box_pack_start (BTK_BOX (anon_box),
                          choice,
                          FALSE, FALSE, 0);
      g_signal_connect (choice, "toggled",
                        G_CALLBACK (pw_dialog_anonymous_toggled), operation);
      priv->anonymous_toggle = choice;

      group = btk_radio_button_get_group (BTK_RADIO_BUTTON (choice));
      choice = btk_radio_button_new_with_mnemonic (group, _("Connect as u_ser:"));
      btk_box_pack_start (BTK_BOX (anon_box),
                          choice,
                          FALSE, FALSE, 0);
      g_signal_connect (choice, "toggled",
                        G_CALLBACK (pw_dialog_anonymous_toggled), operation);
    }

  rows = 0;

  if (flags & G_ASK_PASSWORD_NEED_PASSWORD)
    rows++;

  if (flags & G_ASK_PASSWORD_NEED_USERNAME)
    rows++;

  if (flags &G_ASK_PASSWORD_NEED_DOMAIN)
    rows++;

  /* The table that holds the entries */
  entry_container = btk_alignment_new (0.0, 0.0, 1.0, 1.0);

  btk_alignment_set_padding (BTK_ALIGNMENT (entry_container),
                             0, 0, can_anonymous ? 12 : 0, 0);

  btk_box_pack_start (BTK_BOX (vbox), entry_container,
                      FALSE, FALSE, 0);
  priv->entry_container = entry_container;

  table = btk_table_new (rows, 2, FALSE);
  btk_table_set_col_spacings (BTK_TABLE (table), 12);
  btk_table_set_row_spacings (BTK_TABLE (table), 6);
  btk_container_add (BTK_CONTAINER (entry_container), table);

  rows = 0;

  priv->username_entry = NULL;
  if (flags & G_ASK_PASSWORD_NEED_USERNAME)
    priv->username_entry = table_add_entry (table, rows++, _("_Username:"),
                                            default_user, operation);

  priv->domain_entry = NULL;
  if (flags & G_ASK_PASSWORD_NEED_DOMAIN)
    priv->domain_entry = table_add_entry (table, rows++, _("_Domain:"),
                                          default_domain, operation);

  priv->password_entry = NULL;
  if (flags & G_ASK_PASSWORD_NEED_PASSWORD)
    {
      priv->password_entry = table_add_entry (table, rows++, _("_Password:"),
                                              NULL, operation);
      btk_entry_set_visibility (BTK_ENTRY (priv->password_entry), FALSE);
    }

   if (flags & G_ASK_PASSWORD_SAVING_SUPPORTED)
    {
      BtkWidget    *choice;
      BtkWidget    *remember_box;
      GSList       *group;
      GPasswordSave password_save;

      remember_box = btk_vbox_new (FALSE, 6);
      btk_box_pack_start (BTK_BOX (vbox), remember_box,
                          FALSE, FALSE, 0);

      password_save = g_mount_operation_get_password_save (mount_op);
      
      choice = btk_radio_button_new_with_mnemonic (NULL, _("Forget password _immediately"));
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (choice),
                                    password_save == G_PASSWORD_SAVE_NEVER);
      g_object_set_data (G_OBJECT (choice), "password-save",
                         GINT_TO_POINTER (G_PASSWORD_SAVE_NEVER));
      g_signal_connect (choice, "toggled",
                        G_CALLBACK (remember_button_toggled), operation);
      btk_box_pack_start (BTK_BOX (remember_box), choice, FALSE, FALSE, 0);

      group = btk_radio_button_get_group (BTK_RADIO_BUTTON (choice));
      choice = btk_radio_button_new_with_mnemonic (group, _("Remember password until you _logout"));
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (choice),
                                    password_save == G_PASSWORD_SAVE_FOR_SESSION);
      g_object_set_data (G_OBJECT (choice), "password-save",
                         GINT_TO_POINTER (G_PASSWORD_SAVE_FOR_SESSION));
      g_signal_connect (choice, "toggled",
                        G_CALLBACK (remember_button_toggled), operation);
      btk_box_pack_start (BTK_BOX (remember_box), choice, FALSE, FALSE, 0);

      group = btk_radio_button_get_group (BTK_RADIO_BUTTON (choice));
      choice = btk_radio_button_new_with_mnemonic (group, _("Remember _forever"));
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (choice),
                                    password_save == G_PASSWORD_SAVE_PERMANENTLY);
      g_object_set_data (G_OBJECT (choice), "password-save",
                         GINT_TO_POINTER (G_PASSWORD_SAVE_PERMANENTLY));
      g_signal_connect (choice, "toggled",
                        G_CALLBACK (remember_button_toggled), operation);
      btk_box_pack_start (BTK_BOX (remember_box), choice, FALSE, FALSE, 0);
    }

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (pw_dialog_got_response), operation);

  if (can_anonymous)
    {
      /* The anonymous option will be active by default,
       * ensure the toggled signal is emitted for it.
       */
      btk_toggle_button_toggled (BTK_TOGGLE_BUTTON (priv->anonymous_toggle));
    }
  else if (! pw_dialog_input_is_valid (operation))
    btk_dialog_set_response_sensitive (dialog, BTK_RESPONSE_OK, FALSE);

  g_object_notify (G_OBJECT (operation), "is-showing");

  if (priv->parent_window)
    {
      btk_window_set_transient_for (window, priv->parent_window);
      btk_window_set_modal (window, TRUE);
    }
  else if (priv->screen)
    btk_window_set_screen (BTK_WINDOW (dialog), priv->screen);

  btk_widget_show_all (BTK_WIDGET (dialog));

  g_object_ref (operation);
}

static void
question_dialog_button_clicked (BtkDialog       *dialog,
                                gint             button_number,
                                GMountOperation *op)
{
  BtkMountOperationPrivate *priv;
  BtkMountOperation *operation;

  operation = BTK_MOUNT_OPERATION (op);
  priv = operation->priv;

  if (button_number >= 0)
    {
      g_mount_operation_set_choice (op, button_number);
      g_mount_operation_reply (op, G_MOUNT_OPERATION_HANDLED);
    }
  else
    g_mount_operation_reply (op, G_MOUNT_OPERATION_ABORTED);

  priv->dialog = NULL;
  g_object_notify (G_OBJECT (operation), "is-showing");
  btk_widget_destroy (BTK_WIDGET (dialog));
  g_object_unref (op);
}

static void
btk_mount_operation_ask_question (GMountOperation *op,
                                  const char      *message,
                                  const char      *choices[])
{
  BtkMountOperationPrivate *priv;
  BtkWidget  *dialog;
  const char *secondary = NULL;
  char       *primary;
  int        count, len = 0;

  g_return_if_fail (BTK_IS_MOUNT_OPERATION (op));
  g_return_if_fail (message != NULL);
  g_return_if_fail (choices != NULL);

  priv = BTK_MOUNT_OPERATION (op)->priv;

  primary = strstr (message, "\n");
  if (primary)
    {
      secondary = primary + 1;
      primary = g_strndup (message, primary - message);
    }

  dialog = btk_message_dialog_new (priv->parent_window, 0,
                                   BTK_MESSAGE_QUESTION,
                                   BTK_BUTTONS_NONE, "%s",
                                   primary != NULL ? primary : message);
  g_free (primary);

  if (secondary)
    btk_message_dialog_format_secondary_text (BTK_MESSAGE_DIALOG (dialog),
                                              "%s", secondary);

  /* First count the items in the list then
   * add the buttons in reverse order */

  while (choices[len] != NULL)
    len++;

  for (count = len - 1; count >= 0; count--)
    btk_dialog_add_button (BTK_DIALOG (dialog), choices[count], count);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (question_dialog_button_clicked), op);

  priv->dialog = BTK_DIALOG (dialog);
  g_object_notify (G_OBJECT (op), "is-showing");

  if (priv->parent_window == NULL && priv->screen)
    btk_window_set_screen (BTK_WINDOW (dialog), priv->screen);

  btk_widget_show (dialog);
  g_object_ref (op);
}

static void
show_processes_button_clicked (BtkDialog       *dialog,
                               gint             button_number,
                               GMountOperation *op)
{
  BtkMountOperationPrivate *priv;
  BtkMountOperation *operation;

  operation = BTK_MOUNT_OPERATION (op);
  priv = operation->priv;

  if (button_number >= 0)
    {
      g_mount_operation_set_choice (op, button_number);
      g_mount_operation_reply (op, G_MOUNT_OPERATION_HANDLED);
    }
  else
    g_mount_operation_reply (op, G_MOUNT_OPERATION_ABORTED);

  priv->dialog = NULL;
  g_object_notify (G_OBJECT (operation), "is-showing");
  btk_widget_destroy (BTK_WIDGET (dialog));
  g_object_unref (op);
}

static gint
pid_equal (gconstpointer a,
           gconstpointer b)
{
  GPid pa, pb;

  pa = *((GPid *) a);
  pb = *((GPid *) b);

  return GPOINTER_TO_INT(pb) - GPOINTER_TO_INT(pa);
}

static void
diff_sorted_arrays (GArray         *array1,
                    GArray         *array2,
                    GCompareFunc   compare,
                    GArray         *added_indices,
                    GArray         *removed_indices)
{
  gint order;
  guint n1, n2;
  guint elem_size;

  n1 = n2 = 0;

  elem_size = g_array_get_element_size (array1);
  g_assert (elem_size == g_array_get_element_size (array2));

  while (n1 < array1->len && n2 < array2->len)
    {
      order = (*compare) (((const char*) array1->data) + n1 * elem_size,
                          ((const char*) array2->data) + n2 * elem_size);
      if (order < 0)
        {
          g_array_append_val (removed_indices, n1);
          n1++;
        }
      else if (order > 0)
        {
          g_array_append_val (added_indices, n2);
          n2++;
        }
      else
        { /* same item */
          n1++;
          n2++;
        }
    }

  while (n1 < array1->len)
    {
      g_array_append_val (removed_indices, n1);
      n1++;
    }
  while (n2 < array2->len)
    {
      g_array_append_val (added_indices, n2);
      n2++;
    }
}


static void
add_pid_to_process_list_store (BtkMountOperation              *mount_operation,
                               BtkMountOperationLookupContext *lookup_context,
                               BtkListStore                   *list_store,
                               GPid                            pid)
{
  gchar *command_line;
  gchar *name;
  BdkPixbuf *pixbuf;
  gchar *markup;
  BtkTreeIter iter;

  name = NULL;
  pixbuf = NULL;
  command_line = NULL;
  _btk_mount_operation_lookup_info (lookup_context,
                                    pid,
                                    24,
                                    &name,
                                    &command_line,
                                    &pixbuf);

  if (name == NULL)
    name = g_strdup_printf (_("Unknown Application (PID %d)"), pid);

  if (command_line == NULL)
    command_line = g_strdup ("");

  if (pixbuf == NULL)
    {
      BtkIconTheme *theme;
      theme = btk_icon_theme_get_for_screen (btk_widget_get_screen (BTK_WIDGET (mount_operation->priv->dialog)));
      pixbuf = btk_icon_theme_load_icon (theme,
                                         "application-x-executable",
                                         24,
                                         0,
                                         NULL);
    }

  markup = g_strdup_printf ("<b>%s</b>\n"
                            "<small>%s</small>",
                            name,
                            command_line);

  btk_list_store_append (list_store, &iter);
  btk_list_store_set (list_store, &iter,
                      0, pixbuf,
                      1, markup,
                      2, pid,
                      -1);

  if (pixbuf != NULL)
    g_object_unref (pixbuf);
  g_free (markup);
  g_free (name);
  g_free (command_line);
}

static void
remove_pid_from_process_list_store (BtkMountOperation *mount_operation,
                                    BtkListStore      *list_store,
                                    GPid               pid)
{
  BtkTreeIter iter;
  GPid pid_of_item;

  if (btk_tree_model_get_iter_first (BTK_TREE_MODEL (list_store), &iter))
    {
      do
        {
          btk_tree_model_get (BTK_TREE_MODEL (list_store),
                              &iter,
                              2, &pid_of_item,
                              -1);

          if (pid_of_item == pid)
            {
              btk_list_store_remove (list_store, &iter);
              break;
            }
        }
      while (btk_tree_model_iter_next (BTK_TREE_MODEL (list_store), &iter));
    }
}


static void
update_process_list_store (BtkMountOperation *mount_operation,
                           BtkListStore      *list_store,
                           GArray            *processes)
{
  guint n;
  BtkMountOperationLookupContext *lookup_context;
  GArray *current_pids;
  GArray *pid_indices_to_add;
  GArray *pid_indices_to_remove;
  BtkTreeIter iter;
  GPid pid;

  /* Just removing all items and adding new ones will screw up the
   * focus handling in the treeview - so compute the delta, and add/remove
   * items as appropriate
   */
  current_pids = g_array_new (FALSE, FALSE, sizeof (GPid));
  pid_indices_to_add = g_array_new (FALSE, FALSE, sizeof (gint));
  pid_indices_to_remove = g_array_new (FALSE, FALSE, sizeof (gint));

  if (btk_tree_model_get_iter_first (BTK_TREE_MODEL (list_store), &iter))
    {
      do
        {
          btk_tree_model_get (BTK_TREE_MODEL (list_store),
                              &iter,
                              2, &pid,
                              -1);

          g_array_append_val (current_pids, pid);
        }
      while (btk_tree_model_iter_next (BTK_TREE_MODEL (list_store), &iter));
    }

  g_array_sort (current_pids, pid_equal);
  g_array_sort (processes, pid_equal);

  diff_sorted_arrays (current_pids, processes, pid_equal, pid_indices_to_add, pid_indices_to_remove);

  for (n = 0; n < pid_indices_to_remove->len; n++)
    {
      pid = g_array_index (current_pids, GPid, n);
      remove_pid_from_process_list_store (mount_operation, list_store, pid);
    }

  if (pid_indices_to_add->len > 0)
    {
      lookup_context = _btk_mount_operation_lookup_context_get (btk_widget_get_display (mount_operation->priv->process_tree_view));
      for (n = 0; n < pid_indices_to_add->len; n++)
        {
          pid = g_array_index (processes, GPid, n);
          add_pid_to_process_list_store (mount_operation, lookup_context, list_store, pid);
        }
      _btk_mount_operation_lookup_context_free (lookup_context);
    }

  /* select the first item, if we went from a zero to a non-zero amount of processes */
  if (current_pids->len == 0 && pid_indices_to_add->len > 0)
    {
      if (btk_tree_model_get_iter_first (BTK_TREE_MODEL (list_store), &iter))
        {
          BtkTreeSelection *tree_selection;
          tree_selection = btk_tree_view_get_selection (BTK_TREE_VIEW (mount_operation->priv->process_tree_view));
          btk_tree_selection_select_iter (tree_selection, &iter);
        }
    }

  g_array_unref (current_pids);
  g_array_unref (pid_indices_to_add);
  g_array_unref (pid_indices_to_remove);
}

static void
on_end_process_activated (BtkMenuItem *item,
                          gpointer user_data)
{
  BtkMountOperation *op = BTK_MOUNT_OPERATION (user_data);
  BtkTreeSelection *selection;
  BtkTreeIter iter;
  GPid pid_to_kill;
  GError *error;

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (op->priv->process_tree_view));

  if (!btk_tree_selection_get_selected (selection,
                                        NULL,
                                        &iter))
    goto out;

  btk_tree_model_get (BTK_TREE_MODEL (op->priv->process_list_store),
                      &iter,
                      2, &pid_to_kill,
                      -1);

  /* TODO: We might want to either
   *
   *       - Be smart about things and send SIGKILL rather than SIGTERM if
   *         this is the second time the user requests killing a process
   *
   *       - Or, easier (but worse user experience), offer both "End Process"
   *         and "Terminate Process" options
   *
   *      But that's not how things work right now....
   */
  error = NULL;
  if (!_btk_mount_operation_kill_process (pid_to_kill, &error))
    {
      BtkWidget *dialog;
      gint response;

      /* Use BTK_DIALOG_DESTROY_WITH_PARENT here since the parent dialog can be
       * indeed be destroyed via the GMountOperation::abort signal... for example,
       * this is triggered if the user yanks the device while we are showing
       * the dialog...
       */
      dialog = btk_message_dialog_new (BTK_WINDOW (op->priv->dialog),
                                       BTK_DIALOG_MODAL | BTK_DIALOG_DESTROY_WITH_PARENT,
                                       BTK_MESSAGE_ERROR,
                                       BTK_BUTTONS_CLOSE,
                                       _("Unable to end process"));
      btk_message_dialog_format_secondary_text (BTK_MESSAGE_DIALOG (dialog),
                                                "%s",
                                                error->message);

      btk_widget_show_all (dialog);
      response = btk_dialog_run (BTK_DIALOG (dialog));

      /* BTK_RESPONSE_NONE means the dialog were programmatically destroy, e.g. that
       * BTK_DIALOG_DESTROY_WITH_PARENT kicked in - so it would trigger a warning to
       * destroy the dialog in that case
       */
      if (response != BTK_RESPONSE_NONE)
        btk_widget_destroy (dialog);

      g_error_free (error);
    }

 out:
  ;
}

static gboolean
do_popup_menu_for_process_tree_view (BtkWidget         *widget,
                                     BdkEventButton    *event,
                                     BtkMountOperation *op)
{
  BtkWidget *menu;
  BtkWidget *item;
  gint button;
  gint event_time;
  gboolean popped_up_menu;

  popped_up_menu = FALSE;

  menu = btk_menu_new ();

  item = btk_image_menu_item_new_with_mnemonic (_("_End Process"));
  btk_image_menu_item_set_image (BTK_IMAGE_MENU_ITEM (item),
                                 btk_image_new_from_stock (BTK_STOCK_CLOSE, BTK_ICON_SIZE_MENU));
  g_signal_connect (item, "activate",
                    G_CALLBACK (on_end_process_activated),
                    op);
  btk_menu_shell_append (BTK_MENU_SHELL (menu), item);
  btk_widget_show_all (menu);

  if (event != NULL)
    {
      BtkTreePath *path;
      BtkTreeSelection *selection;

      if (btk_tree_view_get_path_at_pos (BTK_TREE_VIEW (op->priv->process_tree_view),
                                         (gint) event->x,
                                         (gint) event->y,
                                         &path,
                                         NULL,
                                         NULL,
                                         NULL))
        {
          selection = btk_tree_view_get_selection (BTK_TREE_VIEW (op->priv->process_tree_view));
          btk_tree_selection_select_path (selection, path);
          btk_tree_path_free (path);
        }
      else
        {
          /* don't popup a menu if the user right-clicked in an area with no rows */
          goto out;
        }

      button = event->button;
      event_time = event->time;
    }
  else
    {
      button = 0;
      event_time = btk_get_current_event_time ();
    }

  btk_menu_popup (BTK_MENU (menu),
                  NULL,
                  widget,
                  NULL,
                  NULL,
                  button,
                  event_time);

  popped_up_menu = TRUE;

 out:
  return popped_up_menu;
}

static gboolean
on_popup_menu_for_process_tree_view (BtkWidget *widget,
                                     gpointer   user_data)
{
  BtkMountOperation *op = BTK_MOUNT_OPERATION (user_data);
  return do_popup_menu_for_process_tree_view (widget, NULL, op);
}

static gboolean
on_button_press_event_for_process_tree_view (BtkWidget      *widget,
                                             BdkEventButton *event,
                                             gpointer        user_data)
{
  BtkMountOperation *op = BTK_MOUNT_OPERATION (user_data);
  gboolean ret;

  ret = FALSE;

  if (_btk_button_event_triggers_context_menu (event))
    {
      ret = do_popup_menu_for_process_tree_view (widget, event, op);
    }

  return ret;
}

static void
create_show_processes_dialog (GMountOperation *op,
                              const char      *message,
                              const char      *choices[])
{
  BtkMountOperationPrivate *priv;
  BtkWidget  *dialog;
  const char *secondary = NULL;
  char       *primary;
  int        count, len = 0;
  BtkWidget *label;
  BtkWidget *tree_view;
  BtkWidget *scrolled_window;
  BtkWidget *vbox;
  BtkWidget *content_area;
  BtkTreeViewColumn *column;
  BtkCellRenderer *renderer;
  BtkListStore *list_store;
  gchar *s;

  priv = BTK_MOUNT_OPERATION (op)->priv;

  primary = strstr (message, "\n");
  if (primary)
    {
      secondary = primary + 1;
      primary = g_strndup (message, primary - message);
    }

  dialog = btk_dialog_new ();

  if (priv->parent_window != NULL)
    btk_window_set_transient_for (BTK_WINDOW (dialog), priv->parent_window);
  btk_window_set_title (BTK_WINDOW (dialog), "");
  btk_dialog_set_has_separator (BTK_DIALOG (dialog), FALSE);

  content_area = btk_dialog_get_content_area (BTK_DIALOG (dialog));
  vbox = btk_vbox_new (FALSE, 12);
  btk_container_set_border_width (BTK_CONTAINER (vbox), 12);
  btk_box_pack_start (BTK_BOX (content_area), vbox, TRUE, TRUE, 0);

  if (secondary != NULL)
    {
      s = g_strdup_printf ("<big><b>%s</b></big>\n\n%s", primary, secondary);
    }
  else
    {
      s = g_strdup_printf ("%s", primary);
    }
  g_free (primary);
  label = btk_label_new (NULL);
  btk_label_set_markup (BTK_LABEL (label), s);
  g_free (s);
  btk_box_pack_start (BTK_BOX (vbox), label, TRUE, TRUE, 0);

  /* First count the items in the list then
   * add the buttons in reverse order */

  while (choices[len] != NULL)
    len++;

  for (count = len - 1; count >= 0; count--)
    btk_dialog_add_button (BTK_DIALOG (dialog), choices[count], count);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (show_processes_button_clicked), op);

  priv->dialog = BTK_DIALOG (dialog);
  g_object_notify (G_OBJECT (op), "is-showing");

  if (priv->parent_window == NULL && priv->screen)
    btk_window_set_screen (BTK_WINDOW (dialog), priv->screen);

  tree_view = btk_tree_view_new ();
  /* TODO: should use EM's when btk+ RI patches land */
  btk_widget_set_size_request (tree_view,
                               300,
                               120);

  column = btk_tree_view_column_new ();
  renderer = btk_cell_renderer_pixbuf_new ();
  btk_tree_view_column_pack_start (column, renderer, FALSE);
  btk_tree_view_column_set_attributes (column, renderer,
                                       "pixbuf", 0,
                                       NULL);
  renderer = btk_cell_renderer_text_new ();
  g_object_set (renderer,
                "ellipsize", BANGO_ELLIPSIZE_MIDDLE,
                "ellipsize-set", TRUE,
                NULL);
  btk_tree_view_column_pack_start (column, renderer, TRUE);
  btk_tree_view_column_set_attributes (column, renderer,
                                       "markup", 1,
                                       NULL);
  btk_tree_view_append_column (BTK_TREE_VIEW (tree_view), column);
  btk_tree_view_set_headers_visible (BTK_TREE_VIEW (tree_view), FALSE);


  scrolled_window = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window),
                                  BTK_POLICY_NEVER,
                                  BTK_POLICY_AUTOMATIC);
  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (scrolled_window), BTK_SHADOW_IN);

  btk_container_add (BTK_CONTAINER (scrolled_window), tree_view);
  btk_box_pack_start (BTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);

  g_signal_connect (tree_view, "popup-menu",
                    G_CALLBACK (on_popup_menu_for_process_tree_view),
                    op);
  g_signal_connect (tree_view, "button-press-event",
                    G_CALLBACK (on_button_press_event_for_process_tree_view),
                    op);

  list_store = btk_list_store_new (3,
                                   BDK_TYPE_PIXBUF,
                                   G_TYPE_STRING,
                                   G_TYPE_INT);

  btk_tree_view_set_model (BTK_TREE_VIEW (tree_view), BTK_TREE_MODEL (list_store));

  priv->process_list_store = list_store;
  priv->process_tree_view = tree_view;
  /* set pointers to NULL when dialog goes away */
  g_object_add_weak_pointer (G_OBJECT (list_store), (gpointer *) &priv->process_list_store);
  g_object_add_weak_pointer (G_OBJECT (tree_view), (gpointer *) &priv->process_tree_view);

  g_object_unref (list_store);

  btk_widget_show_all (dialog);
  g_object_ref (op);
}

static void
btk_mount_operation_show_processes (GMountOperation *op,
                                    const char      *message,
                                    GArray          *processes,
                                    const char      *choices[])
{
  BtkMountOperationPrivate *priv;

  g_return_if_fail (BTK_IS_MOUNT_OPERATION (op));
  g_return_if_fail (message != NULL);
  g_return_if_fail (processes != NULL);
  g_return_if_fail (choices != NULL);

  priv = BTK_MOUNT_OPERATION (op)->priv;

  if (priv->process_list_store == NULL)
    {
      /* need to create the dialog */
      create_show_processes_dialog (op, message, choices);
    }

  /* otherwise, we're showing the dialog, assume messages+choices hasn't changed */

  update_process_list_store (BTK_MOUNT_OPERATION (op),
                             priv->process_list_store,
                             processes);
}

static void
btk_mount_operation_aborted (GMountOperation *op)
{
  BtkMountOperationPrivate *priv;

  priv = BTK_MOUNT_OPERATION (op)->priv;

  if (priv->dialog != NULL)
    {
      btk_widget_destroy (BTK_WIDGET (priv->dialog));
      priv->dialog = NULL;
      g_object_notify (G_OBJECT (op), "is-showing");
      g_object_unref (op);
    }
}

/**
 * btk_mount_operation_new:
 * @parent: (allow-none): transient parent of the window, or %NULL
 *
 * Creates a new #BtkMountOperation
 *
 * Returns: a new #BtkMountOperation
 *
 * Since: 2.14
 */
GMountOperation *
btk_mount_operation_new (BtkWindow *parent)
{
  GMountOperation *mount_operation;

  mount_operation = g_object_new (BTK_TYPE_MOUNT_OPERATION,
                                  "parent", parent, NULL);

  return mount_operation;
}

/**
 * btk_mount_operation_is_showing:
 * @op: a #BtkMountOperation
 *
 * Returns whether the #BtkMountOperation is currently displaying
 * a window.
 *
 * Returns: %TRUE if @op is currently displaying a window
 *
 * Since: 2.14
 */
gboolean
btk_mount_operation_is_showing (BtkMountOperation *op)
{
  g_return_val_if_fail (BTK_IS_MOUNT_OPERATION (op), FALSE);

  return op->priv->dialog != NULL;
}

/**
 * btk_mount_operation_set_parent:
 * @op: a #BtkMountOperation
 * @parent: (allow-none): transient parent of the window, or %NULL
 *
 * Sets the transient parent for windows shown by the
 * #BtkMountOperation.
 *
 * Since: 2.14
 */
void
btk_mount_operation_set_parent (BtkMountOperation *op,
                                BtkWindow         *parent)
{
  BtkMountOperationPrivate *priv;

  g_return_if_fail (BTK_IS_MOUNT_OPERATION (op));
  g_return_if_fail (parent == NULL || BTK_IS_WINDOW (parent));

  priv = op->priv;

  if (priv->parent_window == parent)
    return;

  if (priv->parent_window)
    {
      g_signal_handlers_disconnect_by_func (priv->parent_window,
                                            btk_widget_destroyed,
                                            &priv->parent_window);
      g_object_unref (priv->parent_window);
    }
  priv->parent_window = parent;
  if (priv->parent_window)
    {
      g_object_ref (priv->parent_window);
      g_signal_connect (priv->parent_window, "destroy",
                        G_CALLBACK (btk_widget_destroyed),
                        &priv->parent_window);
    }

  if (priv->dialog)
    btk_window_set_transient_for (BTK_WINDOW (priv->dialog), priv->parent_window);

  g_object_notify (G_OBJECT (op), "parent");
}

/**
 * btk_mount_operation_get_parent:
 * @op: a #BtkMountOperation
 *
 * Gets the transient parent used by the #BtkMountOperation
 *
 * Returns: (transfer none): the transient parent for windows shown by @op
 *
 * Since: 2.14
 */
BtkWindow *
btk_mount_operation_get_parent (BtkMountOperation *op)
{
  g_return_val_if_fail (BTK_IS_MOUNT_OPERATION (op), NULL);

  return op->priv->parent_window;
}

/**
 * btk_mount_operation_set_screen:
 * @op: a #BtkMountOperation
 * @screen: a #BdkScreen
 *
 * Sets the screen to show windows of the #BtkMountOperation on.
 *
 * Since: 2.14
 */
void
btk_mount_operation_set_screen (BtkMountOperation *op,
                                BdkScreen         *screen)
{
  BtkMountOperationPrivate *priv;

  g_return_if_fail (BTK_IS_MOUNT_OPERATION (op));
  g_return_if_fail (BDK_IS_SCREEN (screen));

  priv = op->priv;

  if (priv->screen == screen)
    return;

  if (priv->screen)
    g_object_unref (priv->screen);

  priv->screen = g_object_ref (screen);

  if (priv->dialog)
    btk_window_set_screen (BTK_WINDOW (priv->dialog), screen);

  g_object_notify (G_OBJECT (op), "screen");
}

/**
 * btk_mount_operation_get_screen:
 * @op: a #BtkMountOperation
 *
 * Gets the screen on which windows of the #BtkMountOperation
 * will be shown.
 *
 * Returns: (transfer none): the screen on which windows of @op are shown
 *
 * Since: 2.14
 */
BdkScreen *
btk_mount_operation_get_screen (BtkMountOperation *op)
{
  BtkMountOperationPrivate *priv;

  g_return_val_if_fail (BTK_IS_MOUNT_OPERATION (op), NULL);

  priv = op->priv;

  if (priv->dialog)
    return btk_window_get_screen (BTK_WINDOW (priv->dialog));
  else if (priv->parent_window)
    return btk_window_get_screen (BTK_WINDOW (priv->parent_window));
  else if (priv->screen)
    return priv->screen;
  else
    return bdk_screen_get_default ();
}

#define __BTK_MOUNT_OPERATION_C__
#include "btkaliasdef.c"
