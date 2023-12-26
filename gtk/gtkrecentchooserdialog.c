/* BTK - The GIMP Toolkit
 * btkrecentchooserdialog.c: Recent files selector dialog
 * Copyright (C) 2006 Emmanuele Bassi
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

#include "btkrecentchooserdialog.h"
#include "btkrecentchooserwidget.h"
#include "btkrecentchooserutils.h"
#include "btkrecentmanager.h"
#include "btktypebuiltins.h"
#include "btkalias.h"

#include <stdarg.h>

struct _BtkRecentChooserDialogPrivate
{
  BtkRecentManager *manager;
  
  BtkWidget *chooser;
};

#define BTK_RECENT_CHOOSER_DIALOG_GET_PRIVATE(obj)	(BTK_RECENT_CHOOSER_DIALOG (obj)->priv)

static void btk_recent_chooser_dialog_class_init (BtkRecentChooserDialogClass *klass);
static void btk_recent_chooser_dialog_init       (BtkRecentChooserDialog      *dialog);
static void btk_recent_chooser_dialog_finalize   (GObject                     *object);

static GObject *btk_recent_chooser_dialog_constructor (GType                  type,
						       guint                  n_construct_properties,
						       GObjectConstructParam *construct_params);

static void btk_recent_chooser_dialog_set_property (GObject      *object,
						    guint         prop_id,
						    const GValue *value,
						    GParamSpec   *pspec);
static void btk_recent_chooser_dialog_get_property (GObject      *object,
						    guint         prop_id,
						    GValue       *value,
						    GParamSpec   *pspec);

static void btk_recent_chooser_dialog_map       (BtkWidget *widget);
static void btk_recent_chooser_dialog_unmap     (BtkWidget *widget);

G_DEFINE_TYPE_WITH_CODE (BtkRecentChooserDialog,
			 btk_recent_chooser_dialog,
			 BTK_TYPE_DIALOG,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_RECENT_CHOOSER,
		       				_btk_recent_chooser_delegate_iface_init))

static void
btk_recent_chooser_dialog_class_init (BtkRecentChooserDialogClass *klass)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (klass);
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (klass);
  
  bobject_class->set_property = btk_recent_chooser_dialog_set_property;
  bobject_class->get_property = btk_recent_chooser_dialog_get_property;
  bobject_class->constructor = btk_recent_chooser_dialog_constructor;
  bobject_class->finalize = btk_recent_chooser_dialog_finalize;
  
  widget_class->map = btk_recent_chooser_dialog_map;
  widget_class->unmap = btk_recent_chooser_dialog_unmap;
  
  _btk_recent_chooser_install_properties (bobject_class);
  
  g_type_class_add_private (klass, sizeof (BtkRecentChooserDialogPrivate));
}

static void
btk_recent_chooser_dialog_init (BtkRecentChooserDialog *dialog)
{
  BtkRecentChooserDialogPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog,
  								     BTK_TYPE_RECENT_CHOOSER_DIALOG,
  								     BtkRecentChooserDialogPrivate);
  BtkDialog *rc_dialog = BTK_DIALOG (dialog);
  
  dialog->priv = priv;

  btk_dialog_set_has_separator (rc_dialog, FALSE);
  btk_container_set_border_width (BTK_CONTAINER (rc_dialog), 5);
  btk_box_set_spacing (BTK_BOX (rc_dialog->vbox), 2); /* 2 * 5 + 2 = 12 */
  btk_container_set_border_width (BTK_CONTAINER (rc_dialog->action_area), 5);

}

/* we intercept the BtkRecentChooser::item_activated signal and try to
 * make the dialog emit a valid response signal
 */
static void
btk_recent_chooser_item_activated_cb (BtkRecentChooser *chooser,
				      gpointer          user_data)
{
  BtkRecentChooserDialog *dialog;
  GList *children, *l;

  dialog = BTK_RECENT_CHOOSER_DIALOG (user_data);

  if (btk_window_activate_default (BTK_WINDOW (dialog)))
    return;
  
  children = btk_container_get_children (BTK_CONTAINER (BTK_DIALOG (dialog)->action_area));
  
  for (l = children; l; l = l->next)
    {
      BtkWidget *widget;
      gint response_id;
      
      widget = BTK_WIDGET (l->data);
      response_id = btk_dialog_get_response_for_widget (BTK_DIALOG (dialog), widget);
      
      if (response_id == BTK_RESPONSE_ACCEPT ||
          response_id == BTK_RESPONSE_OK     ||
          response_id == BTK_RESPONSE_YES    ||
          response_id == BTK_RESPONSE_APPLY)
        {
          g_list_free (children);
	  
          btk_dialog_response (BTK_DIALOG (dialog), response_id);

          return;
        }
    }
  
  g_list_free (children);
}

static GObject *
btk_recent_chooser_dialog_constructor (GType                  type,
				       guint                  n_construct_properties,
				       GObjectConstructParam *construct_params)
{
  GObject *object;
  BtkRecentChooserDialogPrivate *priv;
  
  object = G_OBJECT_CLASS (btk_recent_chooser_dialog_parent_class)->constructor (type,
		  							         n_construct_properties,
										 construct_params);
  priv = BTK_RECENT_CHOOSER_DIALOG_GET_PRIVATE (object);
  
  btk_widget_push_composite_child ();
  
  if (priv->manager)
    priv->chooser = g_object_new (BTK_TYPE_RECENT_CHOOSER_WIDGET,
  				  "recent-manager", priv->manager,
  				  NULL);
  else
    priv->chooser = g_object_new (BTK_TYPE_RECENT_CHOOSER_WIDGET, NULL);
  
  g_signal_connect (priv->chooser, "item-activated",
  		    G_CALLBACK (btk_recent_chooser_item_activated_cb),
  		    object);

  btk_container_set_border_width (BTK_CONTAINER (priv->chooser), 5);
  btk_box_pack_start (BTK_BOX (BTK_DIALOG (object)->vbox),
                      priv->chooser, TRUE, TRUE, 0);
  btk_widget_show (priv->chooser);
  
  _btk_recent_chooser_set_delegate (BTK_RECENT_CHOOSER (object),
  				    BTK_RECENT_CHOOSER (priv->chooser));
  
  btk_widget_pop_composite_child ();
  
  return object;
}

static void
btk_recent_chooser_dialog_set_property (GObject      *object,
					guint         prop_id,
					const GValue *value,
					GParamSpec   *pspec)
{
  BtkRecentChooserDialogPrivate *priv;
  
  priv = BTK_RECENT_CHOOSER_DIALOG_GET_PRIVATE (object);
  
  switch (prop_id)
    {
    case BTK_RECENT_CHOOSER_PROP_RECENT_MANAGER:
      priv->manager = g_value_get_object (value);
      break;
    default:
      g_object_set_property (G_OBJECT (priv->chooser), pspec->name, value);
      break;
    }
}

static void
btk_recent_chooser_dialog_get_property (GObject      *object,
					guint         prop_id,
					GValue       *value,
					GParamSpec   *pspec)
{
  BtkRecentChooserDialogPrivate *priv;
  
  priv = BTK_RECENT_CHOOSER_DIALOG_GET_PRIVATE (object);
  
  g_object_get_property (G_OBJECT (priv->chooser), pspec->name, value);
}

static void
btk_recent_chooser_dialog_finalize (GObject *object)
{
  BtkRecentChooserDialog *dialog = BTK_RECENT_CHOOSER_DIALOG (object);
 
  dialog->priv->manager = NULL;
  
  G_OBJECT_CLASS (btk_recent_chooser_dialog_parent_class)->finalize (object);
}

static void
btk_recent_chooser_dialog_map (BtkWidget *widget)
{
  BtkRecentChooserDialog *dialog = BTK_RECENT_CHOOSER_DIALOG (widget);
  BtkRecentChooserDialogPrivate *priv = dialog->priv;
  
  if (!btk_widget_get_mapped (priv->chooser))
    btk_widget_map (priv->chooser);

  BTK_WIDGET_CLASS (btk_recent_chooser_dialog_parent_class)->map (widget);
}

static void
btk_recent_chooser_dialog_unmap (BtkWidget *widget)
{
  BtkRecentChooserDialog *dialog = BTK_RECENT_CHOOSER_DIALOG (widget);
  BtkRecentChooserDialogPrivate *priv = dialog->priv;
  
  BTK_WIDGET_CLASS (btk_recent_chooser_dialog_parent_class)->unmap (widget);
  
  btk_widget_unmap (priv->chooser);
}

static BtkWidget *
btk_recent_chooser_dialog_new_valist (const gchar      *title,
				      BtkWindow        *parent,
				      BtkRecentManager *manager,
				      const gchar      *first_button_text,
				      va_list           varargs)
{
  BtkWidget *result;
  const char *button_text = first_button_text;
  gint response_id;
  
  result = g_object_new (BTK_TYPE_RECENT_CHOOSER_DIALOG,
                         "title", title,
                         "recent-manager", manager,
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
 * btk_recent_chooser_dialog_new:
 * @title: (allow-none): Title of the dialog, or %NULL
 * @parent: (allow-none): Transient parent of the dialog, or %NULL,
 * @first_button_text: (allow-none): stock ID or text to go in the first button, or %NULL
 * @Varargs: response ID for the first button, then additional (button, id)
 *   pairs, ending with %NULL
 *
 * Creates a new #BtkRecentChooserDialog.  This function is analogous to
 * btk_dialog_new_with_buttons().
 *
 * Return value: a new #BtkRecentChooserDialog
 *
 * Since: 2.10
 */
BtkWidget *
btk_recent_chooser_dialog_new (const gchar *title,
			       BtkWindow   *parent,
			       const gchar *first_button_text,
			       ...)
{
  BtkWidget *result;
  va_list varargs;
  
  va_start (varargs, first_button_text);
  result = btk_recent_chooser_dialog_new_valist (title,
  						 parent,
  						 NULL,
  						 first_button_text,
  						 varargs);
  va_end (varargs);
  
  return result;
}

/**
 * btk_recent_chooser_dialog_new_for_manager:
 * @title: (allow-none): Title of the dialog, or %NULL
 * @parent: (allow-none): Transient parent of the dialog, or %NULL,
 * @manager: a #BtkRecentManager
 * @first_button_text: (allow-none): stock ID or text to go in the first button, or %NULL
 * @Varargs: response ID for the first button, then additional (button, id)
 *   pairs, ending with %NULL
 *
 * Creates a new #BtkRecentChooserDialog with a specified recent manager.
 *
 * This is useful if you have implemented your own recent manager, or if you
 * have a customized instance of a #BtkRecentManager object.
 *
 * Return value: a new #BtkRecentChooserDialog
 *
 * Since: 2.10
 */
BtkWidget *
btk_recent_chooser_dialog_new_for_manager (const gchar      *title,
			                   BtkWindow        *parent,
			                   BtkRecentManager *manager,
			                   const gchar      *first_button_text,
			                   ...)
{
  BtkWidget *result;
  va_list varargs;
  
  va_start (varargs, first_button_text);
  result = btk_recent_chooser_dialog_new_valist (title,
  						 parent,
  						 manager,
  						 first_button_text,
  						 varargs);
  va_end (varargs);
  
  return result;
}

#define __BTK_RECENT_CHOOSER_DIALOG_C__
#include "btkaliasdef.c"
