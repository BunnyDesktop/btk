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
#include "btkadjustment.h"
#include "btkmarshalers.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

enum
{
  PROP_0,
  PROP_VALUE,
  PROP_LOWER,
  PROP_UPPER,
  PROP_STEP_INCREMENT,
  PROP_PAGE_INCREMENT,
  PROP_PAGE_SIZE
};

enum
{
  CHANGED,
  VALUE_CHANGED,
  LAST_SIGNAL
};


static void btk_adjustment_get_property                (GObject      *object,
                                                        guint         prop_id,
                                                        GValue       *value,
                                                        GParamSpec   *pspec);
static void btk_adjustment_set_property                (GObject      *object,
                                                        guint         prop_id,
                                                        const GValue *value,
                                                        GParamSpec   *pspec);
static void btk_adjustment_dispatch_properties_changed (GObject      *object,
                                                        guint         n_pspecs,
                                                        GParamSpec  **pspecs);

static guint adjustment_signals[LAST_SIGNAL] = { 0 };

static guint64 adjustment_changed_stamp = 0; /* protected by global bdk lock */

G_DEFINE_TYPE (BtkAdjustment, btk_adjustment, BTK_TYPE_OBJECT)

static void
btk_adjustment_class_init (BtkAdjustmentClass *class)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (class);

  bobject_class->set_property                = btk_adjustment_set_property;
  bobject_class->get_property                = btk_adjustment_get_property;
  bobject_class->dispatch_properties_changed = btk_adjustment_dispatch_properties_changed;

  class->changed = NULL;
  class->value_changed = NULL;

  /**
   * BtkAdjustment:value:
   * 
   * The value of the adjustment.
   * 
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
                                   PROP_VALUE,
                                   g_param_spec_double ("value",
							P_("Value"),
							P_("The value of the adjustment"),
							-G_MAXDOUBLE, 
							G_MAXDOUBLE, 
							0.0, 
							BTK_PARAM_READWRITE));
  
  /**
   * BtkAdjustment:lower:
   * 
   * The minimum value of the adjustment.
   * 
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
                                   PROP_LOWER,
                                   g_param_spec_double ("lower",
							P_("Minimum Value"),
							P_("The minimum value of the adjustment"),
							-G_MAXDOUBLE, 
							G_MAXDOUBLE, 
							0.0,
							BTK_PARAM_READWRITE));
  
  /**
   * BtkAdjustment:upper:
   * 
   * The maximum value of the adjustment. 
   * Note that values will be restricted by 
   * <literal>upper - page-size</literal> if the page-size 
   * property is nonzero.
   *
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
                                   PROP_UPPER,
                                   g_param_spec_double ("upper",
							P_("Maximum Value"),
							P_("The maximum value of the adjustment"),
							-G_MAXDOUBLE, 
							G_MAXDOUBLE, 
							0.0, 
							BTK_PARAM_READWRITE));
  
  /**
   * BtkAdjustment:step-increment:
   * 
   * The step increment of the adjustment.
   * 
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
                                   PROP_STEP_INCREMENT,
                                   g_param_spec_double ("step-increment",
							P_("Step Increment"),
							P_("The step increment of the adjustment"),
							-G_MAXDOUBLE, 
							G_MAXDOUBLE, 
							0.0, 
							BTK_PARAM_READWRITE));
  
  /**
   * BtkAdjustment:page-increment:
   * 
   * The page increment of the adjustment.
   * 
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
                                   PROP_PAGE_INCREMENT,
                                   g_param_spec_double ("page-increment",
							P_("Page Increment"),
							P_("The page increment of the adjustment"),
							-G_MAXDOUBLE, 
							G_MAXDOUBLE, 
							0.0, 
							BTK_PARAM_READWRITE));
  
  /**
   * BtkAdjustment:page-size:
   * 
   * The page size of the adjustment. 
   * Note that the page-size is irrelevant and should be set to zero
   * if the adjustment is used for a simple scalar value, e.g. in a 
   * #BtkSpinButton.
   * 
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
                                   PROP_PAGE_SIZE,
                                   g_param_spec_double ("page-size",
							P_("Page Size"),
							P_("The page size of the adjustment"),
							-G_MAXDOUBLE, 
							G_MAXDOUBLE, 
							0.0, 
							BTK_PARAM_READWRITE));

  adjustment_signals[CHANGED] =
    g_signal_new (I_("changed"),
		  G_OBJECT_CLASS_TYPE (class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
		  G_STRUCT_OFFSET (BtkAdjustmentClass, changed),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  adjustment_signals[VALUE_CHANGED] =
    g_signal_new (I_("value-changed"),
		  G_OBJECT_CLASS_TYPE (class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
		  G_STRUCT_OFFSET (BtkAdjustmentClass, value_changed),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
}

static void
btk_adjustment_init (BtkAdjustment *adjustment)
{
  adjustment->value = 0.0;
  adjustment->lower = 0.0;
  adjustment->upper = 0.0;
  adjustment->step_increment = 0.0;
  adjustment->page_increment = 0.0;
  adjustment->page_size = 0.0;
}

static void
btk_adjustment_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  BtkAdjustment *adjustment = BTK_ADJUSTMENT (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      g_value_set_double (value, adjustment->value);
      break;
    case PROP_LOWER:
      g_value_set_double (value, adjustment->lower);
      break;
    case PROP_UPPER:
      g_value_set_double (value, adjustment->upper);
      break;
    case PROP_STEP_INCREMENT:
      g_value_set_double (value, adjustment->step_increment);
      break;
    case PROP_PAGE_INCREMENT:
      g_value_set_double (value, adjustment->page_increment);
      break;
    case PROP_PAGE_SIZE:
      g_value_set_double (value, adjustment->page_size);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_adjustment_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  BtkAdjustment *adjustment = BTK_ADJUSTMENT (object);
  gdouble double_value = g_value_get_double (value);

  switch (prop_id)
    {
    case PROP_VALUE:
      btk_adjustment_set_value (adjustment, double_value);
      break;
    case PROP_LOWER:
      adjustment->lower = double_value;
      break;
    case PROP_UPPER:
      adjustment->upper = double_value;
      break;
    case PROP_STEP_INCREMENT:
      adjustment->step_increment = double_value;
      break;
    case PROP_PAGE_INCREMENT:
      adjustment->page_increment = double_value;
      break;
    case PROP_PAGE_SIZE:
      adjustment->page_size = double_value;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_adjustment_dispatch_properties_changed (GObject     *object,
                                            guint        n_pspecs,
                                            GParamSpec **pspecs)
{
  gboolean changed = FALSE;
  gint i;

  G_OBJECT_CLASS (btk_adjustment_parent_class)->dispatch_properties_changed (object, n_pspecs, pspecs);

  for (i = 0; i < n_pspecs; i++)
    switch (pspecs[i]->param_id)
      {
      case PROP_LOWER:
      case PROP_UPPER:
      case PROP_STEP_INCREMENT:
      case PROP_PAGE_INCREMENT:
      case PROP_PAGE_SIZE:
        changed = TRUE;
        break;
      default:
        break;
      }

  if (changed)
    {
      adjustment_changed_stamp++;
      btk_adjustment_changed (BTK_ADJUSTMENT (object));
    }
}

BtkObject *
btk_adjustment_new (gdouble value,
		    gdouble lower,
		    gdouble upper,
		    gdouble step_increment,
		    gdouble page_increment,
		    gdouble page_size)
{
  return g_object_new (BTK_TYPE_ADJUSTMENT,
		       "lower", lower,
		       "upper", upper,
		       "step-increment", step_increment,
		       "page-increment", page_increment,
		       "page-size", page_size,
		       "value", value,
		       NULL);
}

/**
 * btk_adjustment_get_value:
 * @adjustment: a #BtkAdjustment
 *
 * Gets the current value of the adjustment. See
 * btk_adjustment_set_value ().
 *
 * Return value: The current value of the adjustment.
 **/
gdouble
btk_adjustment_get_value (BtkAdjustment *adjustment)
{
  g_return_val_if_fail (BTK_IS_ADJUSTMENT (adjustment), 0.0);

  return adjustment->value;
}

void
btk_adjustment_set_value (BtkAdjustment *adjustment,
			  gdouble        value)
{
  g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));

  value = CLAMP (value, adjustment->lower, adjustment->upper);

  if (value != adjustment->value)
    {
      adjustment->value = value;

      btk_adjustment_value_changed (adjustment);
    }
}

/**
 * btk_adjustment_get_lower:
 * @adjustment: a #BtkAdjustment
 *
 * Retrieves the minimum value of the adjustment.
 *
 * Return value: The current minimum value of the adjustment.
 *
 * Since: 2.14
 **/
gdouble
btk_adjustment_get_lower (BtkAdjustment *adjustment)
{
  g_return_val_if_fail (BTK_IS_ADJUSTMENT (adjustment), 0.0);

  return adjustment->lower;
}

/**
 * btk_adjustment_set_lower:
 * @adjustment: a #BtkAdjustment
 * @lower: the new minimum value
 *
 * Sets the minimum value of the adjustment.
 *
 * When setting multiple adjustment properties via their individual
 * setters, multiple "changed" signals will be emitted. However, since
 * the emission of the "changed" signal is tied to the emission of the
 * "GObject::notify" signals of the changed properties, it's possible
 * to compress the "changed" signals into one by calling
 * g_object_freeze_notify() and g_object_thaw_notify() around the
 * calls to the individual setters.
 *
 * Alternatively, using a single g_object_set() for all the properties
 * to change, or using btk_adjustment_configure() has the same effect
 * of compressing "changed" emissions.
 *
 * Since: 2.14
 **/
void
btk_adjustment_set_lower (BtkAdjustment *adjustment,
                          gdouble        lower)
{
  g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));

  if (lower != adjustment->lower)
    g_object_set (adjustment, "lower", lower, NULL);
}

/**
 * btk_adjustment_get_upper:
 * @adjustment: a #BtkAdjustment
 *
 * Retrieves the maximum value of the adjustment.
 *
 * Return value: The current maximum value of the adjustment.
 *
 * Since: 2.14
 **/
gdouble
btk_adjustment_get_upper (BtkAdjustment *adjustment)
{
  g_return_val_if_fail (BTK_IS_ADJUSTMENT (adjustment), 0.0);

  return adjustment->upper;
}

/**
 * btk_adjustment_set_upper:
 * @adjustment: a #BtkAdjustment
 * @upper: the new maximum value
 *
 * Sets the maximum value of the adjustment.
 *
 * Note that values will be restricted by
 * <literal>upper - page-size</literal> if the page-size
 * property is nonzero.
 *
 * See btk_adjustment_set_lower() about how to compress multiple
 * emissions of the "changed" signal when setting multiple adjustment
 * properties.
 *
 * Since: 2.14
 **/
void
btk_adjustment_set_upper (BtkAdjustment *adjustment,
                          gdouble        upper)
{
  g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));

  if (upper != adjustment->upper)
    g_object_set (adjustment, "upper", upper, NULL);
}

/**
 * btk_adjustment_get_step_increment:
 * @adjustment: a #BtkAdjustment
 *
 * Retrieves the step increment of the adjustment.
 *
 * Return value: The current step increment of the adjustment.
 *
 * Since: 2.14
 **/
gdouble
btk_adjustment_get_step_increment (BtkAdjustment *adjustment)
{
  g_return_val_if_fail (BTK_IS_ADJUSTMENT (adjustment), 0.0);

  return adjustment->step_increment;
}

/**
 * btk_adjustment_set_step_increment:
 * @adjustment: a #BtkAdjustment
 * @step_increment: the new step increment
 *
 * Sets the step increment of the adjustment.
 *
 * See btk_adjustment_set_lower() about how to compress multiple
 * emissions of the "changed" signal when setting multiple adjustment
 * properties.
 *
 * Since: 2.14
 **/
void
btk_adjustment_set_step_increment (BtkAdjustment *adjustment,
                                   gdouble        step_increment)
{
  g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));

  if (step_increment != adjustment->step_increment)
    g_object_set (adjustment, "step-increment", step_increment, NULL);
}

/**
 * btk_adjustment_get_page_increment:
 * @adjustment: a #BtkAdjustment
 *
 * Retrieves the page increment of the adjustment.
 *
 * Return value: The current page increment of the adjustment.
 *
 * Since: 2.14
 **/
gdouble
btk_adjustment_get_page_increment (BtkAdjustment *adjustment)
{
  g_return_val_if_fail (BTK_IS_ADJUSTMENT (adjustment), 0.0);

  return adjustment->page_increment;
}

/**
 * btk_adjustment_set_page_increment:
 * @adjustment: a #BtkAdjustment
 * @page_increment: the new page increment
 *
 * Sets the page increment of the adjustment.
 *
 * See btk_adjustment_set_lower() about how to compress multiple
 * emissions of the "changed" signal when setting multiple adjustment
 * properties.
 *
 * Since: 2.14
 **/
void
btk_adjustment_set_page_increment (BtkAdjustment *adjustment,
                                   gdouble        page_increment)
{
  g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));

  if (page_increment != adjustment->page_increment)
    g_object_set (adjustment, "page-increment", page_increment, NULL);
}

/**
 * btk_adjustment_get_page_size:
 * @adjustment: a #BtkAdjustment
 *
 * Retrieves the page size of the adjustment.
 *
 * Return value: The current page size of the adjustment.
 *
 * Since: 2.14
 **/
gdouble
btk_adjustment_get_page_size (BtkAdjustment *adjustment)
{
  g_return_val_if_fail (BTK_IS_ADJUSTMENT (adjustment), 0.0);

  return adjustment->page_size;
}

/**
 * btk_adjustment_set_page_size:
 * @adjustment: a #BtkAdjustment
 * @page_size: the new page size
 *
 * Sets the page size of the adjustment.
 *
 * See btk_adjustment_set_lower() about how to compress multiple
 * emissions of the "changed" signal when setting multiple adjustment
 * properties.
 *
 * Since: 2.14
 **/
void
btk_adjustment_set_page_size (BtkAdjustment *adjustment,
                              gdouble        page_size)
{
  g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));

  if (page_size != adjustment->page_size)
    g_object_set (adjustment, "page-size", page_size, NULL);
}

/**
 * btk_adjustment_configure:
 * @adjustment: a #BtkAdjustment
 * @value: the new value
 * @lower: the new minimum value
 * @upper: the new maximum value
 * @step_increment: the new step increment
 * @page_increment: the new page increment
 * @page_size: the new page size
 *
 * Sets all properties of the adjustment at once.
 *
 * Use this function to avoid multiple emissions of the "changed"
 * signal. See btk_adjustment_set_lower() for an alternative way
 * of compressing multiple emissions of "changed" into one.
 *
 * Since: 2.14
 **/
void
btk_adjustment_configure (BtkAdjustment *adjustment,
                          gdouble        value,
                          gdouble        lower,
                          gdouble        upper,
                          gdouble        step_increment,
                          gdouble        page_increment,
                          gdouble        page_size)
{
  gboolean value_changed = FALSE;
  guint64 old_stamp = adjustment_changed_stamp;

  g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));

  g_object_freeze_notify (G_OBJECT (adjustment));

  g_object_set (adjustment,
                "lower", lower,
                "upper", upper,
                "step-increment", step_increment,
                "page-increment", page_increment,
                "page-size", page_size,
                NULL);

  /* don't use CLAMP() so we don't end up below lower if upper - page_size
   * is smaller than lower
   */
  value = MIN (value, upper - page_size);
  value = MAX (value, lower);

  if (value != adjustment->value)
    {
      /* set value manually to make sure "changed" is emitted with the
       * new value in place and is emitted before "value-changed"
       */
      adjustment->value = value;
      value_changed = TRUE;
    }

  g_object_thaw_notify (G_OBJECT (adjustment));

  if (old_stamp == adjustment_changed_stamp)
    btk_adjustment_changed (adjustment); /* force emission before ::value-changed */

  if (value_changed)
    btk_adjustment_value_changed (adjustment);
}

void
btk_adjustment_changed (BtkAdjustment *adjustment)
{
  g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));

  g_signal_emit (adjustment, adjustment_signals[CHANGED], 0);
}

void
btk_adjustment_value_changed (BtkAdjustment *adjustment)
{
  g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));

  g_signal_emit (adjustment, adjustment_signals[VALUE_CHANGED], 0);
  g_object_notify (G_OBJECT (adjustment), "value");
}

void
btk_adjustment_clamp_page (BtkAdjustment *adjustment,
			   gdouble        lower,
			   gdouble        upper)
{
  gboolean need_emission;

  g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));

  lower = CLAMP (lower, adjustment->lower, adjustment->upper);
  upper = CLAMP (upper, adjustment->lower, adjustment->upper);

  need_emission = FALSE;

  if (adjustment->value + adjustment->page_size < upper)
    {
      adjustment->value = upper - adjustment->page_size;
      need_emission = TRUE;
    }
  if (adjustment->value > lower)
    {
      adjustment->value = lower;
      need_emission = TRUE;
    }

  if (need_emission)
    btk_adjustment_value_changed (adjustment);
}

#define __BTK_ADJUSTMENT_C__
#include "btkaliasdef.c"
