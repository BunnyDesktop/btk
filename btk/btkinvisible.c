/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
#include <bdk/bdk.h>
#include "btkinvisible.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

enum {
  PROP_0,
  PROP_SCREEN,
  LAST_ARG
};

static void btk_invisible_destroy       (BtkObject         *object);
static void btk_invisible_realize       (BtkWidget         *widget);
static void btk_invisible_style_set     (BtkWidget         *widget,
					 BtkStyle          *previous_style);
static void btk_invisible_show          (BtkWidget         *widget);
static void btk_invisible_size_allocate (BtkWidget         *widget,
					 BtkAllocation     *allocation);
static void btk_invisible_set_property  (BObject           *object,
					 buint              prop_id,
					 const BValue      *value,
					 BParamSpec        *pspec);
static void btk_invisible_get_property  (BObject           *object,
					 buint              prop_id,
					 BValue		   *value,
					 BParamSpec        *pspec);

static BObject *btk_invisible_constructor (GType                  type,
					   buint                  n_construct_properties,
					   BObjectConstructParam *construct_params);

G_DEFINE_TYPE (BtkInvisible, btk_invisible, BTK_TYPE_WIDGET)

static void
btk_invisible_class_init (BtkInvisibleClass *class)
{
  BObjectClass	 *bobject_class;
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;

  widget_class = (BtkWidgetClass*) class;
  object_class = (BtkObjectClass*) class;
  bobject_class = (BObjectClass*) class;

  widget_class->realize = btk_invisible_realize;
  widget_class->style_set = btk_invisible_style_set;
  widget_class->show = btk_invisible_show;
  widget_class->size_allocate = btk_invisible_size_allocate;

  object_class->destroy = btk_invisible_destroy;
  bobject_class->set_property = btk_invisible_set_property;
  bobject_class->get_property = btk_invisible_get_property;
  bobject_class->constructor = btk_invisible_constructor;

  g_object_class_install_property (bobject_class,
				   PROP_SCREEN,
				   g_param_spec_object ("screen",
 							P_("Screen"),
 							P_("The screen where this window will be displayed"),
							BDK_TYPE_SCREEN,
 							BTK_PARAM_READWRITE));
}

static void
btk_invisible_init (BtkInvisible *invisible)
{
  BdkColormap *colormap;
  
  btk_widget_set_has_window (BTK_WIDGET (invisible), TRUE);
  _btk_widget_set_is_toplevel (BTK_WIDGET (invisible), TRUE);

  g_object_ref_sink (invisible);

  invisible->has_user_ref_count = TRUE;
  invisible->screen = bdk_screen_get_default ();
  
  colormap = _btk_widget_peek_colormap ();
  if (colormap)
    btk_widget_set_colormap (BTK_WIDGET (invisible), colormap);
}

static void
btk_invisible_destroy (BtkObject *object)
{
  BtkInvisible *invisible = BTK_INVISIBLE (object);
  
  if (invisible->has_user_ref_count)
    {
      invisible->has_user_ref_count = FALSE;
      g_object_unref (invisible);
    }

  BTK_OBJECT_CLASS (btk_invisible_parent_class)->destroy (object);  
}

/**
 * btk_invisible_new_for_screen:
 * @screen: a #BdkScreen which identifies on which
 *	    the new #BtkInvisible will be created.
 *
 * Creates a new #BtkInvisible object for a specified screen
 *
 * Return value: a newly created #BtkInvisible object
 *
 * Since: 2.2
 **/
BtkWidget* 
btk_invisible_new_for_screen (BdkScreen *screen)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);
  
  return g_object_new (BTK_TYPE_INVISIBLE, "screen", screen, NULL);
}

/**
 * btk_invisible_new:
 * 
 * Creates a new #BtkInvisible.
 * 
 * Return value: a new #BtkInvisible.
 **/
BtkWidget*
btk_invisible_new (void)
{
  return g_object_new (BTK_TYPE_INVISIBLE, NULL);
}

/**
 * btk_invisible_set_screen:
 * @invisible: a #BtkInvisible.
 * @screen: a #BdkScreen.
 *
 * Sets the #BdkScreen where the #BtkInvisible object will be displayed.
 *
 * Since: 2.2
 **/ 
void
btk_invisible_set_screen (BtkInvisible *invisible,
			  BdkScreen    *screen)
{
  BtkWidget *widget;
  BdkScreen *previous_screen;
  bboolean was_realized;
  
  g_return_if_fail (BTK_IS_INVISIBLE (invisible));
  g_return_if_fail (BDK_IS_SCREEN (screen));

  if (screen == invisible->screen)
    return;

  widget = BTK_WIDGET (invisible);

  previous_screen = invisible->screen;
  was_realized = btk_widget_get_realized (widget);

  if (was_realized)
    btk_widget_unrealize (widget);
  
  invisible->screen = screen;
  if (screen != previous_screen)
    _btk_widget_propagate_screen_changed (widget, previous_screen);
  g_object_notify (B_OBJECT (invisible), "screen");
  
  if (was_realized)
    btk_widget_realize (widget);
}

/**
 * btk_invisible_get_screen:
 * @invisible: a #BtkInvisible.
 *
 * Returns the #BdkScreen object associated with @invisible
 *
 * Return value: (transfer none): the associated #BdkScreen.
 *
 * Since: 2.2
 **/
BdkScreen *
btk_invisible_get_screen (BtkInvisible *invisible)
{
  g_return_val_if_fail (BTK_IS_INVISIBLE (invisible), NULL);
  
  return invisible->screen;
}

static void
btk_invisible_realize (BtkWidget *widget)
{
  BdkWindow *parent;
  BdkWindowAttr attributes;
  bint attributes_mask;

  btk_widget_set_realized (widget, TRUE);

  parent = btk_widget_get_parent_window (widget);
  if (parent == NULL)
    parent = btk_widget_get_root_window (widget);

  attributes.x = -100;
  attributes.y = -100;
  attributes.width = 10;
  attributes.height = 10;
  attributes.window_type = BDK_WINDOW_TEMP;
  attributes.wclass = BDK_INPUT_ONLY;
  attributes.override_redirect = TRUE;
  attributes.event_mask = btk_widget_get_events (widget);

  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_NOREDIR;

  widget->window = bdk_window_new (parent, &attributes, attributes_mask);
					      
  bdk_window_set_user_data (widget->window, widget);
  
  widget->style = btk_style_attach (widget->style, widget->window);
}

static void
btk_invisible_style_set (BtkWidget *widget,
			 BtkStyle  *previous_style)
{
  /* Don't chain up to parent implementation */
}

static void
btk_invisible_show (BtkWidget *widget)
{
  BTK_WIDGET_SET_FLAGS (widget, BTK_VISIBLE);
  btk_widget_map (widget);
}

static void
btk_invisible_size_allocate (BtkWidget     *widget,
			    BtkAllocation *allocation)
{
  widget->allocation = *allocation;
} 


static void 
btk_invisible_set_property  (BObject      *object,
			     buint         prop_id,
			     const BValue *value,
			     BParamSpec   *pspec)
{
  BtkInvisible *invisible = BTK_INVISIBLE (object);
  
  switch (prop_id)
    {
    case PROP_SCREEN:
      btk_invisible_set_screen (invisible, b_value_get_object (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
btk_invisible_get_property  (BObject      *object,
			     buint         prop_id,
			     BValue	  *value,
			     BParamSpec   *pspec)
{
  BtkInvisible *invisible = BTK_INVISIBLE (object);

  switch (prop_id)
    {
    case PROP_SCREEN:
      b_value_set_object (value, invisible->screen);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* We use a constructor here so that we can realize the invisible on
 * the correct screen after the "screen" property has been set
 */
static BObject*
btk_invisible_constructor (GType                  type,
			   buint                  n_construct_properties,
			   BObjectConstructParam *construct_params)
{
  BObject *object;

  object = B_OBJECT_CLASS (btk_invisible_parent_class)->constructor (type,
                                                                     n_construct_properties,
                                                                     construct_params);

  btk_widget_realize (BTK_WIDGET (object));

  return object;
}

#define __BTK_INVISIBLE_C__
#include "btkaliasdef.c"
