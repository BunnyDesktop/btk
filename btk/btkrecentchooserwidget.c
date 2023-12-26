/* BTK - The GIMP Toolkit
 * btkrecentchooserwidget.c: embeddable recently used resources chooser widget
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

#include "btkrecentchooserwidget.h"
#include "btkrecentchooserdefault.h"
#include "btkrecentchooserutils.h"
#include "btktypebuiltins.h"
#include "btkalias.h"

struct _BtkRecentChooserWidgetPrivate
{
  BtkRecentManager *manager;
  
  BtkWidget *chooser;
};

#define BTK_RECENT_CHOOSER_WIDGET_GET_PRIVATE(obj)	(BTK_RECENT_CHOOSER_WIDGET (obj)->priv)

static BObject *btk_recent_chooser_widget_constructor  (GType                  type,
						        guint                  n_params,
						        BObjectConstructParam *params);
static void     btk_recent_chooser_widget_set_property (BObject               *object,
						        guint                  prop_id,
						        const BValue          *value,
						        BParamSpec            *pspec);
static void     btk_recent_chooser_widget_get_property (BObject               *object,
						        guint                  prop_id,
						        BValue                *value,
						        BParamSpec            *pspec);
static void     btk_recent_chooser_widget_finalize     (BObject               *object);


G_DEFINE_TYPE_WITH_CODE (BtkRecentChooserWidget,
		         btk_recent_chooser_widget,
			 BTK_TYPE_VBOX,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_RECENT_CHOOSER,
						_btk_recent_chooser_delegate_iface_init))

static void
btk_recent_chooser_widget_class_init (BtkRecentChooserWidgetClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);

  bobject_class->constructor = btk_recent_chooser_widget_constructor;
  bobject_class->set_property = btk_recent_chooser_widget_set_property;
  bobject_class->get_property = btk_recent_chooser_widget_get_property;
  bobject_class->finalize = btk_recent_chooser_widget_finalize;

  _btk_recent_chooser_install_properties (bobject_class);

  g_type_class_add_private (klass, sizeof (BtkRecentChooserWidgetPrivate));
}


static void
btk_recent_chooser_widget_init (BtkRecentChooserWidget *widget)
{
  widget->priv = B_TYPE_INSTANCE_GET_PRIVATE (widget, BTK_TYPE_RECENT_CHOOSER_WIDGET,
					      BtkRecentChooserWidgetPrivate);
}

static BObject *
btk_recent_chooser_widget_constructor (GType                  type,
				       guint                  n_params,
				       BObjectConstructParam *params)
{
  BObject *object;
  BtkRecentChooserWidgetPrivate *priv;

  object = B_OBJECT_CLASS (btk_recent_chooser_widget_parent_class)->constructor (type,
										 n_params,
										 params);

  priv = BTK_RECENT_CHOOSER_WIDGET_GET_PRIVATE (object);
  priv->chooser = _btk_recent_chooser_default_new (priv->manager);
  
  
  btk_container_add (BTK_CONTAINER (object), priv->chooser);
  btk_widget_show (priv->chooser);
  _btk_recent_chooser_set_delegate (BTK_RECENT_CHOOSER (object),
				    BTK_RECENT_CHOOSER (priv->chooser));

  return object;
}

static void
btk_recent_chooser_widget_set_property (BObject      *object,
				        guint         prop_id,
				        const BValue *value,
				        BParamSpec   *pspec)
{
  BtkRecentChooserWidgetPrivate *priv;

  priv = BTK_RECENT_CHOOSER_WIDGET_GET_PRIVATE (object);
  
  switch (prop_id)
    {
    case BTK_RECENT_CHOOSER_PROP_RECENT_MANAGER:
      priv->manager = b_value_get_object (value);
      break;
    default:
      g_object_set_property (B_OBJECT (priv->chooser), pspec->name, value);
      break;
    }
}

static void
btk_recent_chooser_widget_get_property (BObject    *object,
				        guint       prop_id,
				        BValue     *value,
				        BParamSpec *pspec)
{
  BtkRecentChooserWidgetPrivate *priv;

  priv = BTK_RECENT_CHOOSER_WIDGET_GET_PRIVATE (object);

  g_object_get_property (B_OBJECT (priv->chooser), pspec->name, value);
}

static void
btk_recent_chooser_widget_finalize (BObject *object)
{
  BtkRecentChooserWidgetPrivate *priv;
  
  priv = BTK_RECENT_CHOOSER_WIDGET_GET_PRIVATE (object);
  priv->manager = NULL;
  
  B_OBJECT_CLASS (btk_recent_chooser_widget_parent_class)->finalize (object);
}

/*
 * Public API
 */

/**
 * btk_recent_chooser_widget_new:
 * 
 * Creates a new #BtkRecentChooserWidget object.  This is an embeddable widget
 * used to access the recently used resources list.
 *
 * Return value: a new #BtkRecentChooserWidget
 *
 * Since: 2.10
 */
BtkWidget *
btk_recent_chooser_widget_new (void)
{
  return g_object_new (BTK_TYPE_RECENT_CHOOSER_WIDGET, NULL);
}

/**
 * btk_recent_chooser_widget_new_for_manager:
 * @manager: a #BtkRecentManager
 *
 * Creates a new #BtkRecentChooserWidget with a specified recent manager.
 *
 * This is useful if you have implemented your own recent manager, or if you
 * have a customized instance of a #BtkRecentManager object.
 *
 * Return value: a new #BtkRecentChooserWidget
 *
 * Since: 2.10
 */
BtkWidget *
btk_recent_chooser_widget_new_for_manager (BtkRecentManager *manager)
{
  g_return_val_if_fail (manager == NULL || BTK_IS_RECENT_MANAGER (manager), NULL);
  
  return g_object_new (BTK_TYPE_RECENT_CHOOSER_WIDGET,
  		       "recent-manager", manager,
  		       NULL);
}

#define __BTK_RECENT_CHOOSER_WIDGET_C__
#include "btkaliasdef.c"
