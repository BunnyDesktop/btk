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
#include "bailspinbutton.h"
#include "bailadjustment.h"
#include "bail-private-macros.h"

static void      bail_spin_button_class_init        (BailSpinButtonClass *klass);
static void      bail_spin_button_init              (BailSpinButton *button);
static void      bail_spin_button_real_initialize   (BatkObject      *obj,
                                                     gpointer       data);
static void      bail_spin_button_finalize          (BObject        *object);

static void      batk_value_interface_init           (BatkValueIface  *iface);

static void      bail_spin_button_real_notify_btk   (BObject        *obj,
                                                     BParamSpec     *pspec);

static void      bail_spin_button_get_current_value (BatkValue       *obj,
                                                     BValue         *value);
static void      bail_spin_button_get_maximum_value (BatkValue       *obj,
                                                     BValue         *value);
static void      bail_spin_button_get_minimum_value (BatkValue       *obj,
                                                     BValue         *value);
static void      bail_spin_button_get_minimum_increment (BatkValue       *obj,
                                                         BValue         *value);
static gboolean  bail_spin_button_set_current_value (BatkValue       *obj,
                                                     const BValue   *value);
static void      bail_spin_button_value_changed     (BtkAdjustment  *adjustment,
                                                     gpointer       data);
        
G_DEFINE_TYPE_WITH_CODE (BailSpinButton, bail_spin_button, BAIL_TYPE_ENTRY,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_VALUE, batk_value_interface_init))

static void
bail_spin_button_class_init (BailSpinButtonClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);
  BailWidgetClass *widget_class;

  widget_class = (BailWidgetClass*)klass;

  widget_class->notify_btk = bail_spin_button_real_notify_btk;

  class->initialize = bail_spin_button_real_initialize;

  bobject_class->finalize = bail_spin_button_finalize;
}

static void
bail_spin_button_init (BailSpinButton *button)
{
}

static void
bail_spin_button_real_initialize (BatkObject *obj,
                                  gpointer  data)
{
  BailSpinButton *spin_button = BAIL_SPIN_BUTTON (obj);
  BtkSpinButton *btk_spin_button;

  BATK_OBJECT_CLASS (bail_spin_button_parent_class)->initialize (obj, data);

  btk_spin_button = BTK_SPIN_BUTTON (data); 
  /*
   * If a BtkAdjustment already exists for the spin_button, 
   * create the BailAdjustment
   */
  if (btk_spin_button->adjustment)
    {
      spin_button->adjustment = bail_adjustment_new (btk_spin_button->adjustment);
      g_signal_connect (btk_spin_button->adjustment,
                        "value-changed",
                        G_CALLBACK (bail_spin_button_value_changed),
                        obj);
    }
  else
    spin_button->adjustment = NULL;

  obj->role = BATK_ROLE_SPIN_BUTTON;

}

static void
batk_value_interface_init (BatkValueIface *iface)
{
  iface->get_current_value = bail_spin_button_get_current_value;
  iface->get_maximum_value = bail_spin_button_get_maximum_value;
  iface->get_minimum_value = bail_spin_button_get_minimum_value;
  iface->get_minimum_increment = bail_spin_button_get_minimum_increment;
  iface->set_current_value = bail_spin_button_set_current_value;
}

static void
bail_spin_button_get_current_value (BatkValue       *obj,
                                    BValue         *value)
{
  BailSpinButton *spin_button;

  g_return_if_fail (BAIL_IS_SPIN_BUTTON (obj));

  spin_button = BAIL_SPIN_BUTTON (obj);
  if (spin_button->adjustment == NULL)
    /*
     * Adjustment has not been specified
     */
    return;

  batk_value_get_current_value (BATK_VALUE (spin_button->adjustment), value);
}

static void      
bail_spin_button_get_maximum_value (BatkValue       *obj,
                                    BValue         *value)
{
  BailSpinButton *spin_button;

  g_return_if_fail (BAIL_IS_SPIN_BUTTON (obj));

  spin_button = BAIL_SPIN_BUTTON (obj);
  if (spin_button->adjustment == NULL)
    /*
     * Adjustment has not been specified
     */
    return;

  batk_value_get_maximum_value (BATK_VALUE (spin_button->adjustment), value);
}

static void 
bail_spin_button_get_minimum_value (BatkValue       *obj,
                                    BValue         *value)
{
 BailSpinButton *spin_button;

  g_return_if_fail (BAIL_IS_SPIN_BUTTON (obj));

  spin_button = BAIL_SPIN_BUTTON (obj);
  if (spin_button->adjustment == NULL)
    /*
     * Adjustment has not been specified
     */
    return;

  batk_value_get_minimum_value (BATK_VALUE (spin_button->adjustment), value);
}

static void
bail_spin_button_get_minimum_increment (BatkValue *obj, BValue *value)
{
 BailSpinButton *spin_button;

  g_return_if_fail (BAIL_IS_SPIN_BUTTON (obj));

  spin_button = BAIL_SPIN_BUTTON (obj);
  if (spin_button->adjustment == NULL)
    /*
     * Adjustment has not been specified
     */
    return;

  batk_value_get_minimum_increment (BATK_VALUE (spin_button->adjustment), value);
}

static gboolean  
bail_spin_button_set_current_value (BatkValue       *obj,
                                    const BValue   *value)
{
 BailSpinButton *spin_button;

  g_return_val_if_fail (BAIL_IS_SPIN_BUTTON (obj), FALSE);

  spin_button = BAIL_SPIN_BUTTON (obj);
  if (spin_button->adjustment == NULL)
    /*
     * Adjustment has not been specified
     */
    return FALSE;

  return batk_value_set_current_value (BATK_VALUE (spin_button->adjustment), value);
}

static void
bail_spin_button_finalize (BObject            *object)
{
  BailSpinButton *spin_button = BAIL_SPIN_BUTTON (object);

  if (spin_button->adjustment)
    {
      g_object_unref (spin_button->adjustment);
      spin_button->adjustment = NULL;
    }
  B_OBJECT_CLASS (bail_spin_button_parent_class)->finalize (object);
}


static void
bail_spin_button_real_notify_btk (BObject    *obj,
                                  BParamSpec *pspec)
{
  BtkWidget *widget = BTK_WIDGET (obj);
  BailSpinButton *spin_button = BAIL_SPIN_BUTTON (btk_widget_get_accessible (widget));

  if (strcmp (pspec->name, "adjustment") == 0)
    {
      /*
       * Get rid of the BailAdjustment for the BtkAdjustment
       * which was associated with the spin_button.
       */
      BtkSpinButton* btk_spin_button;

      if (spin_button->adjustment)
        {
          g_object_unref (spin_button->adjustment);
          spin_button->adjustment = NULL;
        }
      /*
       * Create the BailAdjustment when notify for "adjustment" property
       * is received
       */
      btk_spin_button = BTK_SPIN_BUTTON (widget);
      spin_button->adjustment = bail_adjustment_new (btk_spin_button->adjustment);
      g_signal_connect (btk_spin_button->adjustment,
                        "value-changed",
                        G_CALLBACK (bail_spin_button_value_changed),
                        spin_button);
    }
  else
    BAIL_WIDGET_CLASS (bail_spin_button_parent_class)->notify_btk (obj, pspec);
}


static void
bail_spin_button_value_changed (BtkAdjustment    *adjustment,
                                gpointer         data)
{
  BailSpinButton *spin_button;

  bail_return_if_fail (adjustment != NULL);
  bail_return_if_fail (data != NULL);

  spin_button = BAIL_SPIN_BUTTON (data);

  g_object_notify (B_OBJECT (spin_button), "accessible-value");
}

