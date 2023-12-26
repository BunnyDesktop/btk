/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* BTK - The GIMP Toolkit
 * btkfilechooserdialog.c: File selector dialog
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
#include "btkfilechooserprivate.h"
#include "btkfilechooserdialog.h"
#include "btkfilechooserwidget.h"
#include "btkfilechooserutils.h"
#include "btkfilechooserembed.h"
#include "btkfilechoosersettings.h"
#include "btkfilesystem.h"
#include "btktypebuiltins.h"
#include "btkintl.h"
#include "btkalias.h"

#include <stdarg.h>

#define BTK_FILE_CHOOSER_DIALOG_GET_PRIVATE(o)  (BTK_FILE_CHOOSER_DIALOG (o)->priv)

static void btk_file_chooser_dialog_finalize   (GObject                   *object);

static GObject* btk_file_chooser_dialog_constructor  (GType                  type,
						      guint                  n_construct_properties,
						      GObjectConstructParam *construct_params);
static void     btk_file_chooser_dialog_set_property (GObject               *object,
						      guint                  prop_id,
						      const GValue          *value,
						      GParamSpec            *pspec);
static void     btk_file_chooser_dialog_get_property (GObject               *object,
						      guint                  prop_id,
						      GValue                *value,
						      GParamSpec            *pspec);

static void     btk_file_chooser_dialog_map          (BtkWidget             *widget);

static void response_cb (BtkDialog *dialog,
			 gint       response_id);

G_DEFINE_TYPE_WITH_CODE (BtkFileChooserDialog, btk_file_chooser_dialog, BTK_TYPE_DIALOG,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_FILE_CHOOSER,
						_btk_file_chooser_delegate_iface_init))

static void
btk_file_chooser_dialog_class_init (BtkFileChooserDialogClass *class)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (class);
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (class);

  bobject_class->constructor = btk_file_chooser_dialog_constructor;
  bobject_class->set_property = btk_file_chooser_dialog_set_property;
  bobject_class->get_property = btk_file_chooser_dialog_get_property;
  bobject_class->finalize = btk_file_chooser_dialog_finalize;

  widget_class->map       = btk_file_chooser_dialog_map;

  _btk_file_chooser_install_properties (bobject_class);

  g_type_class_add_private (class, sizeof (BtkFileChooserDialogPrivate));
}

static void
btk_file_chooser_dialog_init (BtkFileChooserDialog *dialog)
{
  BtkFileChooserDialogPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog,
								   BTK_TYPE_FILE_CHOOSER_DIALOG,
								   BtkFileChooserDialogPrivate);
  BtkDialog *fc_dialog = BTK_DIALOG (dialog);

  dialog->priv = priv;
  dialog->priv->response_requested = FALSE;

  btk_dialog_set_has_separator (fc_dialog, FALSE);
  btk_container_set_border_width (BTK_CONTAINER (fc_dialog), 5);
  btk_box_set_spacing (BTK_BOX (fc_dialog->vbox), 2); /* 2 * 5 + 2 = 12 */
  btk_container_set_border_width (BTK_CONTAINER (fc_dialog->action_area), 5);

  btk_window_set_role (BTK_WINDOW (dialog), "BtkFileChooserDialog");

  /* We do a signal connection here rather than overriding the method in
   * class_init because BtkDialog::response is a RUN_LAST signal.  We want *our*
   * handler to be run *first*, regardless of whether the user installs response
   * handlers of his own.
   */
  g_signal_connect (dialog, "response",
		    G_CALLBACK (response_cb), NULL);
}

static void
btk_file_chooser_dialog_finalize (GObject *object)
{
  BtkFileChooserDialog *dialog = BTK_FILE_CHOOSER_DIALOG (object);

  g_free (dialog->priv->file_system);

  G_OBJECT_CLASS (btk_file_chooser_dialog_parent_class)->finalize (object);  
}

static gboolean
is_stock_accept_response_id (int response_id)
{
  return (response_id == BTK_RESPONSE_ACCEPT
	  || response_id == BTK_RESPONSE_OK
	  || response_id == BTK_RESPONSE_YES
	  || response_id == BTK_RESPONSE_APPLY);
}

/* Callback used when the user activates a file in the file chooser widget */
static void
file_chooser_widget_file_activated (BtkFileChooser       *chooser,
				    BtkFileChooserDialog *dialog)
{
  GList *children, *l;

  if (btk_window_activate_default (BTK_WINDOW (dialog)))
    return;

  /* There probably isn't a default widget, so make things easier for the
   * programmer by looking for a reasonable button on our own.
   */

  children = btk_container_get_children (BTK_CONTAINER (BTK_DIALOG (dialog)->action_area));

  for (l = children; l; l = l->next)
    {
      BtkWidget *widget;
      int response_id;

      widget = BTK_WIDGET (l->data);
      response_id = btk_dialog_get_response_for_widget (BTK_DIALOG (dialog), widget);
      if (is_stock_accept_response_id (response_id))
	{
	  btk_widget_activate (widget); /* Should we btk_dialog_response (dialog, response_id) instead? */
	  break;
	}
    }

  g_list_free (children);
}

#if 0
/* FIXME: to see why this function is ifdef-ed out, see the comment below in
 * file_chooser_widget_default_size_changed().
 */
static void
load_position (int *out_xpos, int *out_ypos)
{
  BtkFileChooserSettings *settings;
  int x, y, width, height;

  settings = _btk_file_chooser_settings_new ();
  _btk_file_chooser_settings_get_geometry (settings, &x, &y, &width, &height);
  g_object_unref (settings);

  *out_xpos = x;
  *out_ypos = y;
}
#endif

static void
file_chooser_widget_default_size_changed (BtkWidget            *widget,
					  BtkFileChooserDialog *dialog)
{
  BtkFileChooserDialogPrivate *priv;
  gint default_width, default_height;
  BtkRequisition req, widget_req;

  priv = BTK_FILE_CHOOSER_DIALOG_GET_PRIVATE (dialog);

  /* Unset any previously set size */
  btk_widget_set_size_request (BTK_WIDGET (dialog), -1, -1);

  if (btk_widget_is_drawable (widget))
    {
      /* Force a size request of everything before we start.  This will make sure
       * that widget->requisition is meaningful. */
      btk_widget_size_request (BTK_WIDGET (dialog), &req);
      btk_widget_size_request (widget, &widget_req);
    }

  _btk_file_chooser_embed_get_default_size (BTK_FILE_CHOOSER_EMBED (priv->widget),
					    &default_width, &default_height);

  btk_window_resize (BTK_WINDOW (dialog), default_width, default_height);

  if (!btk_widget_get_mapped (BTK_WIDGET (dialog)))
    {
#if 0
      /* FIXME: the code to restore the position does not work yet.  It is not
       * clear whether it is actually desirable --- if enabled, applications
       * would not be able to say "center the file chooser on top of my toplevel
       * window".  So, we don't use this code at all.
       */
      load_position (&xpos, &ypos);
      if (xpos >= 0 && ypos >= 0)
	{
	  btk_window_set_position (BTK_WINDOW (dialog), BTK_WIN_POS_NONE);
	  btk_window_move (BTK_WINDOW (dialog), xpos, ypos);
	}
#endif
    }
}

static void
file_chooser_widget_response_requested (BtkWidget            *widget,
					BtkFileChooserDialog *dialog)
{
  GList *children, *l;

  dialog->priv->response_requested = TRUE;

  if (btk_window_activate_default (BTK_WINDOW (dialog)))
    return;

  /* There probably isn't a default widget, so make things easier for the
   * programmer by looking for a reasonable button on our own.
   */

  children = btk_container_get_children (BTK_CONTAINER (BTK_DIALOG (dialog)->action_area));

  for (l = children; l; l = l->next)
    {
      BtkWidget *widget;
      int response_id;

      widget = BTK_WIDGET (l->data);
      response_id = btk_dialog_get_response_for_widget (BTK_DIALOG (dialog), widget);
      if (is_stock_accept_response_id (response_id))
	{
	  btk_widget_activate (widget); /* Should we btk_dialog_response (dialog, response_id) instead? */
	  break;
	}
    }

  if (l == NULL)
    dialog->priv->response_requested = FALSE;

  g_list_free (children);
}
  
static GObject*
btk_file_chooser_dialog_constructor (GType                  type,
				     guint                  n_construct_properties,
				     GObjectConstructParam *construct_params)
{
  BtkFileChooserDialogPrivate *priv;
  GObject *object;

  object = G_OBJECT_CLASS (btk_file_chooser_dialog_parent_class)->constructor (type,
									       n_construct_properties,
									       construct_params);
  priv = BTK_FILE_CHOOSER_DIALOG_GET_PRIVATE (object);

  btk_widget_push_composite_child ();

  if (priv->file_system)
    priv->widget = g_object_new (BTK_TYPE_FILE_CHOOSER_WIDGET,
				 "file-system-backend", priv->file_system,
				 NULL);
  else
    priv->widget = g_object_new (BTK_TYPE_FILE_CHOOSER_WIDGET, NULL);

  g_signal_connect (priv->widget, "file-activated",
		    G_CALLBACK (file_chooser_widget_file_activated), object);
  g_signal_connect (priv->widget, "default-size-changed",
		    G_CALLBACK (file_chooser_widget_default_size_changed), object);
  g_signal_connect (priv->widget, "response-requested",
		    G_CALLBACK (file_chooser_widget_response_requested), object);

  btk_container_set_border_width (BTK_CONTAINER (priv->widget), 5);
  btk_box_pack_start (BTK_BOX (BTK_DIALOG (object)->vbox), priv->widget, TRUE, TRUE, 0);

  btk_widget_show (priv->widget);

  _btk_file_chooser_set_delegate (BTK_FILE_CHOOSER (object),
				  BTK_FILE_CHOOSER (priv->widget));

  btk_widget_pop_composite_child ();

  return object;
}

static void
btk_file_chooser_dialog_set_property (GObject         *object,
				      guint            prop_id,
				      const GValue    *value,
				      GParamSpec      *pspec)

{
  BtkFileChooserDialogPrivate *priv = BTK_FILE_CHOOSER_DIALOG_GET_PRIVATE (object);

  switch (prop_id)
    {
    case BTK_FILE_CHOOSER_PROP_FILE_SYSTEM_BACKEND:
      g_free (priv->file_system);
      priv->file_system = g_value_dup_string (value);
      break;
    default:
      g_object_set_property (G_OBJECT (priv->widget), pspec->name, value);
      break;
    }
}

static void
btk_file_chooser_dialog_get_property (GObject         *object,
				      guint            prop_id,
				      GValue          *value,
				      GParamSpec      *pspec)
{
  BtkFileChooserDialogPrivate *priv = BTK_FILE_CHOOSER_DIALOG_GET_PRIVATE (object);

  g_object_get_property (G_OBJECT (priv->widget), pspec->name, value);
}

static void
foreach_ensure_default_response_cb (BtkWidget *widget,
				    gpointer   data)
{
  BtkFileChooserDialog *dialog = BTK_FILE_CHOOSER_DIALOG (data);
  int response_id;

  response_id = btk_dialog_get_response_for_widget (BTK_DIALOG (dialog), widget);
  if (is_stock_accept_response_id (response_id))
    btk_dialog_set_default_response (BTK_DIALOG (dialog), response_id);
}

static void
ensure_default_response (BtkFileChooserDialog *dialog)
{
  btk_container_foreach (BTK_CONTAINER (BTK_DIALOG (dialog)->action_area),
			 foreach_ensure_default_response_cb,
			 dialog);
}

/* BtkWidget::map handler */
static void
btk_file_chooser_dialog_map (BtkWidget *widget)
{
  BtkFileChooserDialog *dialog = BTK_FILE_CHOOSER_DIALOG (widget);
  BtkFileChooserDialogPrivate *priv = BTK_FILE_CHOOSER_DIALOG_GET_PRIVATE (dialog);

  ensure_default_response (dialog);

  _btk_file_chooser_embed_initial_focus (BTK_FILE_CHOOSER_EMBED (priv->widget));

  BTK_WIDGET_CLASS (btk_file_chooser_dialog_parent_class)->map (widget);
}

/* BtkDialog::response handler */
static void
response_cb (BtkDialog *dialog,
	     gint       response_id)
{
  BtkFileChooserDialogPrivate *priv;

  priv = BTK_FILE_CHOOSER_DIALOG_GET_PRIVATE (dialog);

  /* Act only on response IDs we recognize */
  if (is_stock_accept_response_id (response_id)
      && !priv->response_requested
      && !_btk_file_chooser_embed_should_respond (BTK_FILE_CHOOSER_EMBED (priv->widget)))
    {
      g_signal_stop_emission_by_name (dialog, "response");
    }

  priv->response_requested = FALSE;
}

static BtkWidget *
btk_file_chooser_dialog_new_valist (const gchar          *title,
				    BtkWindow            *parent,
				    BtkFileChooserAction  action,
				    const gchar          *backend,
				    const gchar          *first_button_text,
				    va_list               varargs)
{
  BtkWidget *result;
  const char *button_text = first_button_text;
  gint response_id;

  result = g_object_new (BTK_TYPE_FILE_CHOOSER_DIALOG,
			 "title", title,
			 "action", action,
			 NULL);

  if (parent)
    btk_window_set_transient_for (BTK_WINDOW (result), parent);

  while (button_text)
    {
      response_id = va_arg (varargs, gint);
      btk_dialog_add_button (BTK_DIALOG (result), button_text, response_id);
      button_text = va_arg (varargs, const gchar *);
    }

  return result;
}

/**
 * btk_file_chooser_dialog_new:
 * @title: (allow-none): Title of the dialog, or %NULL
 * @parent: (allow-none): Transient parent of the dialog, or %NULL
 * @action: Open or save mode for the dialog
 * @first_button_text: (allow-none): stock ID or text to go in the first button, or %NULL
 * @Varargs: response ID for the first button, then additional (button, id) pairs, ending with %NULL
 *
 * Creates a new #BtkFileChooserDialog.  This function is analogous to
 * btk_dialog_new_with_buttons().
 *
 * Return value: a new #BtkFileChooserDialog
 *
 * Since: 2.4
 **/
BtkWidget *
btk_file_chooser_dialog_new (const gchar         *title,
			     BtkWindow           *parent,
			     BtkFileChooserAction action,
			     const gchar         *first_button_text,
			     ...)
{
  BtkWidget *result;
  va_list varargs;
  
  va_start (varargs, first_button_text);
  result = btk_file_chooser_dialog_new_valist (title, parent, action,
					       NULL, first_button_text,
					       varargs);
  va_end (varargs);

  return result;
}

/**
 * btk_file_chooser_dialog_new_with_backend:
 * @title: (allow-none): Title of the dialog, or %NULL
 * @parent: (allow-none): Transient parent of the dialog, or %NULL
 * @action: Open or save mode for the dialog
 * @backend: The name of the specific filesystem backend to use.
 * @first_button_text: (allow-none): stock ID or text to go in the first button, or %NULL
 * @Varargs: response ID for the first button, then additional (button, id) pairs, ending with %NULL
 *
 * Creates a new #BtkFileChooserDialog with a specified backend. This is
 * especially useful if you use btk_file_chooser_set_local_only() to allow
 * non-local files and you use a more expressive vfs, such as gnome-vfs,
 * to load files.
 *
 * Return value: a new #BtkFileChooserDialog
 *
 * Since: 2.4
 * Deprecated: 2.14: Use btk_file_chooser_dialog_new() instead.
 **/
BtkWidget *
btk_file_chooser_dialog_new_with_backend (const gchar          *title,
					  BtkWindow            *parent,
					  BtkFileChooserAction  action,
					  const gchar          *backend,
					  const gchar          *first_button_text,
					  ...)
{
  BtkWidget *result;
  va_list varargs;
  
  va_start (varargs, first_button_text);
  result = btk_file_chooser_dialog_new_valist (title, parent, action,
					       backend, first_button_text,
					       varargs);
  va_end (varargs);

  return result;
}

#define __BTK_FILE_CHOOSER_DIALOG_C__
#include "btkaliasdef.c"
