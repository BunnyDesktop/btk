/* BAIL - The BUNNY Accessibility Implementation Library
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
#include "bailadjustment.h"

static void	 bail_adjustment_class_init        (BailAdjustmentClass *klass);

static void	 bail_adjustment_init              (BailAdjustment      *adjustment);

static void	 bail_adjustment_real_initialize   (BatkObject	        *obj,
                                                    gpointer            data);

static void	 batk_value_interface_init          (BatkValueIface       *iface);

static void	 bail_adjustment_get_current_value (BatkValue            *obj,
                                                    GValue              *value);
static void	 bail_adjustment_get_maximum_value (BatkValue            *obj,
                                                    GValue              *value);
static void	 bail_adjustment_get_minimum_value (BatkValue            *obj,
                                                    GValue              *value);
static void	 bail_adjustment_get_minimum_increment (BatkValue        *obj,
                                                    GValue              *value);
static gboolean	 bail_adjustment_set_current_value (BatkValue            *obj,
                                                    const GValue        *value);

static void      bail_adjustment_destroyed         (BtkAdjustment       *adjustment,
                                                    BailAdjustment      *bail_adjustment);

G_DEFINE_TYPE_WITH_CODE (BailAdjustment, bail_adjustment, BATK_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_VALUE, batk_value_interface_init))

static void	 
bail_adjustment_class_init (BailAdjustmentClass *klass)
{
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);

  class->initialize = bail_adjustment_real_initialize;
}

static void
bail_adjustment_init (BailAdjustment *adjustment)
{
}

BatkObject* 
bail_adjustment_new (BtkAdjustment *adjustment)
{
  GObject *object;
  BatkObject *batk_object;

  g_return_val_if_fail (BTK_IS_ADJUSTMENT (adjustment), NULL);

  object = g_object_new (BAIL_TYPE_ADJUSTMENT, NULL);

  batk_object = BATK_OBJECT (object);
  batk_object_initialize (batk_object, adjustment);

  return batk_object;
}

static void
bail_adjustment_real_initialize (BatkObject *obj,
                                 gpointer  data)
{
  BtkAdjustment *adjustment;

  BATK_OBJECT_CLASS (bail_adjustment_parent_class)->initialize (obj, data);

  adjustment = BTK_ADJUSTMENT (data);

  obj->role = BATK_ROLE_UNKNOWN;
  BAIL_ADJUSTMENT (obj)->adjustment = adjustment;

  g_signal_connect_object (G_OBJECT (adjustment),
                           "destroy",
                           G_CALLBACK (bail_adjustment_destroyed),
                           obj, 0);
}

static void	 
batk_value_interface_init (BatkValueIface *iface)
{
  iface->get_current_value = bail_adjustment_get_current_value;
  iface->get_maximum_value = bail_adjustment_get_maximum_value;
  iface->get_minimum_value = bail_adjustment_get_minimum_value;
  iface->get_minimum_increment = bail_adjustment_get_minimum_increment;
  iface->set_current_value = bail_adjustment_set_current_value;
}

static void	 
bail_adjustment_get_current_value (BatkValue             *obj,
                                   GValue               *value)
{
  BtkAdjustment* adjustment;
  gdouble current_value;
 
  adjustment = BAIL_ADJUSTMENT (obj)->adjustment;
  if (adjustment == NULL)
  {
    /* State is defunct */
    return;
  }

  current_value = adjustment->value;
  memset (value,  0, sizeof (GValue));
  g_value_init (value, G_TYPE_DOUBLE);
  g_value_set_double (value,current_value);
}

static void	 
bail_adjustment_get_maximum_value (BatkValue             *obj,
                                   GValue               *value)
{
  BtkAdjustment* adjustment;
  gdouble maximum_value;
 
  adjustment = BAIL_ADJUSTMENT (obj)->adjustment;
  if (adjustment == NULL)
  {
    /* State is defunct */
    return;
  }

  maximum_value = adjustment->upper;
  memset (value,  0, sizeof (GValue));
  g_value_init (value, G_TYPE_DOUBLE);
  g_value_set_double (value, maximum_value);
}

static void	 
bail_adjustment_get_minimum_value (BatkValue             *obj,
                                   GValue               *value)
{
  BtkAdjustment* adjustment;
  gdouble minimum_value;
 
  adjustment = BAIL_ADJUSTMENT (obj)->adjustment;
  if (adjustment == NULL)
  {
    /* State is defunct */
    return;
  }

  minimum_value = adjustment->lower;
  memset (value,  0, sizeof (GValue));
  g_value_init (value, G_TYPE_DOUBLE);
  g_value_set_double (value, minimum_value);
}

static void
bail_adjustment_get_minimum_increment (BatkValue        *obj,
                                       GValue          *value)
{
  BtkAdjustment* adjustment;
  gdouble minimum_increment;
 
  adjustment = BAIL_ADJUSTMENT (obj)->adjustment;
  if (adjustment == NULL)
  {
    /* State is defunct */
    return;
  }

  if (adjustment->step_increment != 0 &&
      adjustment->page_increment != 0)
    {
      if (ABS (adjustment->step_increment) < ABS (adjustment->page_increment))
        minimum_increment = adjustment->step_increment;
      else
        minimum_increment = adjustment->page_increment;
    }
  else if (adjustment->step_increment == 0 &&
           adjustment->page_increment == 0)
    {
      minimum_increment = 0;
    }
  else if (adjustment->step_increment == 0)
    {
      minimum_increment = adjustment->page_increment;
    }
  else
    {
      minimum_increment = adjustment->step_increment;
    }

  memset (value,  0, sizeof (GValue));
  g_value_init (value, G_TYPE_DOUBLE);
  g_value_set_double (value, minimum_increment);
}

static gboolean	 
bail_adjustment_set_current_value (BatkValue             *obj,
                                   const GValue         *value)
{
  if (G_VALUE_HOLDS_DOUBLE (value))
  {
    BtkAdjustment* adjustment;
    gdouble new_value;
 
    adjustment = BAIL_ADJUSTMENT (obj)->adjustment;
    if (adjustment == NULL)
    {
      /* State is defunct */
      return FALSE;
    }
    new_value = g_value_get_double (value);
    btk_adjustment_set_value (adjustment, new_value);

    return TRUE;
  }
  else
    return FALSE;
}

static void
bail_adjustment_destroyed (BtkAdjustment       *adjustment,
                           BailAdjustment      *bail_adjustment)
{
  /*
   * This is the signal handler for the "destroy" signal for the 
   * BtkAdjustment. We set the  pointer location to NULL;
   */
  bail_adjustment->adjustment = NULL;
}
