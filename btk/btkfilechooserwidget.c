/* BTK - The GIMP Toolkit
 * btkfilechooserwidget.c: Embeddable file selector widget
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

#include "btkfilechooserwidget.h"
#include "btkfilechooserdefault.h"
#include "btkfilechooserutils.h"
#include "btktypebuiltins.h"
#include "btkfilechooserembed.h"
#include "btkintl.h"
#include "btkalias.h"

#define BTK_FILE_CHOOSER_WIDGET_GET_PRIVATE(o)  (BTK_FILE_CHOOSER_WIDGET (o)->priv)

static void btk_file_chooser_widget_finalize     (BObject                   *object);

static BObject* btk_file_chooser_widget_constructor  (GType                  type,
						      guint                  n_construct_properties,
						      BObjectConstructParam *construct_params);
static void     btk_file_chooser_widget_set_property (BObject               *object,
						      guint                  prop_id,
						      const BValue          *value,
						      BParamSpec            *pspec);
static void     btk_file_chooser_widget_get_property (BObject               *object,
						      guint                  prop_id,
						      BValue                *value,
						      BParamSpec            *pspec);

G_DEFINE_TYPE_WITH_CODE (BtkFileChooserWidget, btk_file_chooser_widget, BTK_TYPE_VBOX,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_FILE_CHOOSER,
						_btk_file_chooser_delegate_iface_init)
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_FILE_CHOOSER_EMBED,
						_btk_file_chooser_embed_delegate_iface_init))

static void
btk_file_chooser_widget_class_init (BtkFileChooserWidgetClass *class)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (class);

  bobject_class->constructor = btk_file_chooser_widget_constructor;
  bobject_class->set_property = btk_file_chooser_widget_set_property;
  bobject_class->get_property = btk_file_chooser_widget_get_property;
  bobject_class->finalize = btk_file_chooser_widget_finalize;

  _btk_file_chooser_install_properties (bobject_class);

  g_type_class_add_private (class, sizeof (BtkFileChooserWidgetPrivate));
}

static void
btk_file_chooser_widget_init (BtkFileChooserWidget *chooser_widget)
{
  BtkFileChooserWidgetPrivate *priv = B_TYPE_INSTANCE_GET_PRIVATE (chooser_widget,
								   BTK_TYPE_FILE_CHOOSER_WIDGET,
								   BtkFileChooserWidgetPrivate);
  chooser_widget->priv = priv;
}

static void
btk_file_chooser_widget_finalize (BObject *object)
{
  BtkFileChooserWidget *chooser = BTK_FILE_CHOOSER_WIDGET (object);

  g_free (chooser->priv->file_system);

  B_OBJECT_CLASS (btk_file_chooser_widget_parent_class)->finalize (object);
}

static BObject*
btk_file_chooser_widget_constructor (GType                  type,
				     guint                  n_construct_properties,
				     BObjectConstructParam *construct_params)
{
  BtkFileChooserWidgetPrivate *priv;
  BObject *object;
  
  object = B_OBJECT_CLASS (btk_file_chooser_widget_parent_class)->constructor (type,
									       n_construct_properties,
									       construct_params);
  priv = BTK_FILE_CHOOSER_WIDGET_GET_PRIVATE (object);

  btk_widget_push_composite_child ();

  priv->impl = _btk_file_chooser_default_new ();
  
  btk_box_pack_start (BTK_BOX (object), priv->impl, TRUE, TRUE, 0);
  btk_widget_show (priv->impl);

  _btk_file_chooser_set_delegate (BTK_FILE_CHOOSER (object),
				  BTK_FILE_CHOOSER (priv->impl));

  _btk_file_chooser_embed_set_delegate (BTK_FILE_CHOOSER_EMBED (object),
					BTK_FILE_CHOOSER_EMBED (priv->impl));
  
  btk_widget_pop_composite_child ();

  return object;
}

static void
btk_file_chooser_widget_set_property (BObject         *object,
				      guint            prop_id,
				      const BValue    *value,
				      BParamSpec      *pspec)
{
  BtkFileChooserWidgetPrivate *priv = BTK_FILE_CHOOSER_WIDGET_GET_PRIVATE (object);

  switch (prop_id)
    {
    case BTK_FILE_CHOOSER_PROP_FILE_SYSTEM_BACKEND:
      g_free (priv->file_system);
      priv->file_system = b_value_dup_string (value);
      break;
    default:
      g_object_set_property (B_OBJECT (priv->impl), pspec->name, value);
      break;
    }
}

static void
btk_file_chooser_widget_get_property (BObject         *object,
				      guint            prop_id,
				      BValue          *value,
				      BParamSpec      *pspec)
{
  BtkFileChooserWidgetPrivate *priv = BTK_FILE_CHOOSER_WIDGET_GET_PRIVATE (object);
  
  g_object_get_property (B_OBJECT (priv->impl), pspec->name, value);
}

/**
 * btk_file_chooser_widget_new:
 * @action: Open or save mode for the widget
 * 
 * Creates a new #BtkFileChooserWidget.  This is a file chooser widget that can
 * be embedded in custom windows, and it is the same widget that is used by
 * #BtkFileChooserDialog.
 * 
 * Return value: a new #BtkFileChooserWidget
 *
 * Since: 2.4
 **/
BtkWidget *
btk_file_chooser_widget_new (BtkFileChooserAction action)
{
  return g_object_new (BTK_TYPE_FILE_CHOOSER_WIDGET,
		       "action", action,
		       NULL);
}

/**
 * btk_file_chooser_widget_new_with_backend:
 * @action: Open or save mode for the widget
 * @backend: The name of the specific filesystem backend to use.
 * 
 * Creates a new #BtkFileChooserWidget with a specified backend.  This is
 * especially useful if you use btk_file_chooser_set_local_only() to allow
 * non-local files.  This is a file chooser widget that can be embedded in
 * custom windows and it is the same widget that is used by
 * #BtkFileChooserDialog.
 * 
 * Return value: a new #BtkFileChooserWidget
 *
 * Since: 2.4
 * Deprecated: 2.14: Use btk_file_chooser_widget_new() instead.
 **/
BtkWidget *
btk_file_chooser_widget_new_with_backend (BtkFileChooserAction  action,
					  const gchar          *backend)
{
  return g_object_new (BTK_TYPE_FILE_CHOOSER_WIDGET,
		       "action", action,
		       NULL);
}

#define __BTK_FILE_CHOOSER_WIDGET_C__
#include "btkaliasdef.c"
