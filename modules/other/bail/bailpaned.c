/* BAIL - The BUNNY Accessibility Enabling Library
 * Copyright 2001, 2002, 2003 Sun Microsystems Inc.
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
#include <btk/btk.h>
#include "bailpaned.h"

static void         bail_paned_class_init          (BailPanedClass *klass); 

static void         bail_paned_init                (BailPaned      *paned);

static void         bail_paned_real_initialize     (BatkObject      *obj,
                                                    gpointer       data);
static void         bail_paned_size_allocate_btk   (BtkWidget      *widget,
                                                    BtkAllocation  *allocation);

static BatkStateSet* bail_paned_ref_state_set       (BatkObject      *accessible);

static void         batk_value_interface_init       (BatkValueIface  *iface);
static void         bail_paned_get_current_value   (BatkValue       *obj,
                                                    GValue         *value);
static void         bail_paned_get_maximum_value   (BatkValue       *obj,
                                                    GValue         *value);
static void         bail_paned_get_minimum_value   (BatkValue       *obj,
                                                    GValue         *value);
static gboolean     bail_paned_set_current_value   (BatkValue       *obj,
                                                    const GValue   *value);

G_DEFINE_TYPE_WITH_CODE (BailPaned, bail_paned, BAIL_TYPE_CONTAINER,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_VALUE, batk_value_interface_init))

static void
bail_paned_class_init (BailPanedClass *klass)
{
  BatkObjectClass  *class = BATK_OBJECT_CLASS (klass);

  class->ref_state_set = bail_paned_ref_state_set;
  class->initialize = bail_paned_real_initialize;
}

static void
bail_paned_init (BailPaned *paned)
{
}

static BatkStateSet*
bail_paned_ref_state_set (BatkObject *accessible)
{
  BatkStateSet *state_set;
  BtkWidget *widget;

  state_set = BATK_OBJECT_CLASS (bail_paned_parent_class)->ref_state_set (accessible);
  widget = BTK_ACCESSIBLE (accessible)->widget;

  if (widget == NULL)
    return state_set;

  if (BTK_IS_VPANED (widget))
    batk_state_set_add_state (state_set, BATK_STATE_VERTICAL);
  else if (BTK_IS_HPANED (widget))
    batk_state_set_add_state (state_set, BATK_STATE_HORIZONTAL);

  return state_set;
}

static void
bail_paned_real_initialize (BatkObject *obj,
                            gpointer  data)
{
  BATK_OBJECT_CLASS (bail_paned_parent_class)->initialize (obj, data);

  g_signal_connect (data,
                    "size_allocate",
                    G_CALLBACK (bail_paned_size_allocate_btk),
                    NULL);

  obj->role = BATK_ROLE_SPLIT_PANE;
}
 
static void
bail_paned_size_allocate_btk (BtkWidget      *widget,
                              BtkAllocation  *allocation)
{
  BatkObject *obj = btk_widget_get_accessible (widget);

  g_object_notify (G_OBJECT (obj), "accessible-value");
}


static void
batk_value_interface_init (BatkValueIface *iface)
{
  iface->get_current_value = bail_paned_get_current_value;
  iface->get_maximum_value = bail_paned_get_maximum_value;
  iface->get_minimum_value = bail_paned_get_minimum_value;
  iface->set_current_value = bail_paned_set_current_value;
}

static void
bail_paned_get_current_value (BatkValue             *obj,
                              GValue               *value)
{
  BtkWidget* widget;
  gint current_value;

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  current_value = btk_paned_get_position (BTK_PANED (widget));
  memset (value,  0, sizeof (GValue));
  g_value_init (value, G_TYPE_INT);
  g_value_set_int (value,current_value);
}

static void
bail_paned_get_maximum_value (BatkValue             *obj,
                              GValue               *value)
{
  BtkWidget* widget;
  gint maximum_value;

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  maximum_value = BTK_PANED (widget)->max_position;
  memset (value,  0, sizeof (GValue));
  g_value_init (value, G_TYPE_INT);
  g_value_set_int (value, maximum_value);
}

static void
bail_paned_get_minimum_value (BatkValue             *obj,
                              GValue               *value)
{
  BtkWidget* widget;
  gint minimum_value;

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  minimum_value = BTK_PANED (widget)->min_position;
  memset (value,  0, sizeof (GValue));
  g_value_init (value, G_TYPE_INT);
  g_value_set_int (value, minimum_value);
}

/*
 * Calling batk_value_set_current_value() is no guarantee that the value is
 * acceptable; it is necessary to listen for accessible-value signals
 * and check whether the current value has been changed or check what the 
 * maximum and minimum values are.
 */

static gboolean
bail_paned_set_current_value (BatkValue             *obj,
                              const GValue         *value)
{
  BtkWidget* widget;
  gint new_value;

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;

  if (G_VALUE_HOLDS_INT (value))
    {
      new_value = g_value_get_int (value);
      btk_paned_set_position (BTK_PANED (widget), new_value);

      return TRUE;
    }
  else
    return FALSE;
}
