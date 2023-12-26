/* BAIL - The GNOME Accessibility Implementation Library
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
#include <bdk/bdkkeysyms.h>
#include "bailrange.h"
#include "bailadjustment.h"
#include "bail-private-macros.h"

static void	    bail_range_class_init        (BailRangeClass *klass);

static void         bail_range_init              (BailRange      *range);

static void         bail_range_real_initialize   (BatkObject      *obj,
                                                  gpointer      data);

static void         bail_range_finalize          (GObject        *object);

static BatkStateSet* bail_range_ref_state_set     (BatkObject      *obj);


static void         bail_range_real_notify_btk   (GObject        *obj,
                                                  GParamSpec     *pspec);

static void	    batk_value_interface_init	 (BatkValueIface  *iface);
static void	    bail_range_get_current_value (BatkValue       *obj,
                                                  GValue         *value);
static void	    bail_range_get_maximum_value (BatkValue       *obj,
                                                  GValue         *value);
static void	    bail_range_get_minimum_value (BatkValue       *obj,
                                                  GValue         *value);
static void         bail_range_get_minimum_increment (BatkValue       *obj,
                                                      GValue         *value);
static gboolean	    bail_range_set_current_value (BatkValue       *obj,
                                                  const GValue   *value);
static void         bail_range_value_changed     (BtkAdjustment  *adjustment,
                                                  gpointer       data);

static void         batk_action_interface_init    (BatkActionIface *iface);
static gboolean     bail_range_do_action        (BatkAction       *action,
                                                gint            i);
static gboolean     idle_do_action              (gpointer        data);
static gint         bail_range_get_n_actions    (BatkAction       *action);
static const gchar* bail_range_get_description  (BatkAction    *action,
                                                         gint          i);
static const gchar* bail_range_get_keybinding   (BatkAction     *action,
                                                         gint            i);
static const gchar* bail_range_action_get_name  (BatkAction    *action,
                                                        gint            i);
static gboolean   bail_range_set_description  (BatkAction       *action,
                                              gint            i,
                                              const gchar     *desc);

G_DEFINE_TYPE_WITH_CODE (BailRange, bail_range, BAIL_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_ACTION, batk_action_interface_init)
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_VALUE, batk_value_interface_init))

static void	 
bail_range_class_init		(BailRangeClass *klass)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (klass);
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);
  BailWidgetClass *widget_class;

  widget_class = (BailWidgetClass*)klass;

  widget_class->notify_btk = bail_range_real_notify_btk;

  class->ref_state_set = bail_range_ref_state_set;
  class->initialize = bail_range_real_initialize;

  bobject_class->finalize = bail_range_finalize;
}

static void
bail_range_init (BailRange      *range)
{
}

static void
bail_range_real_initialize (BatkObject *obj,
                            gpointer  data)
{
  BailRange *range = BAIL_RANGE (obj);
  BtkRange *btk_range;

  BATK_OBJECT_CLASS (bail_range_parent_class)->initialize (obj, data);

  btk_range = BTK_RANGE (data);
  /*
   * If a BtkAdjustment already exists for the BtkRange,
   * create the BailAdjustment
   */
  if (btk_range->adjustment)
    {
      range->adjustment = bail_adjustment_new (btk_range->adjustment);
      g_signal_connect (btk_range->adjustment,
                        "value-changed",
                        G_CALLBACK (bail_range_value_changed),
                        range);
    }
  else
    range->adjustment = NULL;
  range->activate_keybinding=NULL;
  range->activate_description=NULL;
  /*
   * Assumed to BtkScale (either BtkHScale or BtkVScale)
   */
  obj->role = BATK_ROLE_SLIDER;
}

static BatkStateSet*
bail_range_ref_state_set (BatkObject *obj)
{
  BatkStateSet *state_set;
  BtkWidget *widget;
  BtkRange *range;

  state_set = BATK_OBJECT_CLASS (bail_range_parent_class)->ref_state_set (obj);
  widget = BTK_ACCESSIBLE (obj)->widget;

  if (widget == NULL)
    return state_set;

  range = BTK_RANGE (widget);

  /*
   * We do not generate property change for orientation change as there
   * is no interface to change the orientation which emits a notification
   */
  if (range->orientation == BTK_ORIENTATION_HORIZONTAL)
    batk_state_set_add_state (state_set, BATK_STATE_HORIZONTAL);
  else
    batk_state_set_add_state (state_set, BATK_STATE_VERTICAL);

  return state_set;
}

static void	 
batk_value_interface_init (BatkValueIface *iface)
{
  iface->get_current_value = bail_range_get_current_value;
  iface->get_maximum_value = bail_range_get_maximum_value;
  iface->get_minimum_value = bail_range_get_minimum_value;
  iface->get_minimum_increment = bail_range_get_minimum_increment;
  iface->set_current_value = bail_range_set_current_value;
}

static void	 
bail_range_get_current_value (BatkValue		*obj,
                              GValue		*value)
{
  BailRange *range;

  g_return_if_fail (BAIL_IS_RANGE (obj));

  range = BAIL_RANGE (obj);
  if (range->adjustment == NULL)
    /*
     * Adjustment has not been specified
     */
    return;

  batk_value_get_current_value (BATK_VALUE (range->adjustment), value);
}

static void	 
bail_range_get_maximum_value (BatkValue		*obj,
                              GValue		*value)
{
  BailRange *range;
  BtkRange *btk_range;
  BtkAdjustment *btk_adjustment;
  gdouble max = 0;

  g_return_if_fail (BAIL_IS_RANGE (obj));

  range = BAIL_RANGE (obj);
  if (range->adjustment == NULL)
    /*
     * Adjustment has not been specified
     */
    return;
 
  batk_value_get_maximum_value (BATK_VALUE (range->adjustment), value);

  btk_range = BTK_RANGE (btk_accessible_get_widget (BTK_ACCESSIBLE (range)));
  g_return_if_fail (btk_range);

  btk_adjustment = btk_range_get_adjustment (btk_range);
  max = g_value_get_double (value);
  max -=  btk_adjustment_get_page_size (btk_adjustment);

  if (btk_range_get_restrict_to_fill_level (btk_range))
    max = MIN (max, btk_range_get_fill_level (btk_range));

  g_value_set_double (value, max);
}

static void	 
bail_range_get_minimum_value (BatkValue		*obj,
                              GValue		*value)
{
  BailRange *range;

  g_return_if_fail (BAIL_IS_RANGE (obj));

  range = BAIL_RANGE (obj);
  if (range->adjustment == NULL)
    /*
     * Adjustment has not been specified
     */
    return;

  batk_value_get_minimum_value (BATK_VALUE (range->adjustment), value);
}

static void
bail_range_get_minimum_increment (BatkValue *obj, GValue *value)
{
 BailRange *range;

  g_return_if_fail (BAIL_IS_RANGE (obj));

  range = BAIL_RANGE (obj);
  if (range->adjustment == NULL)
    /*
     * Adjustment has not been specified
     */
    return;

  batk_value_get_minimum_increment (BATK_VALUE (range->adjustment), value);
}

static gboolean	 bail_range_set_current_value (BatkValue		*obj,
                                               const GValue	*value)
{
  BtkWidget *widget;

  g_return_val_if_fail (BAIL_IS_RANGE (obj), FALSE);

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    return FALSE;

  if (G_VALUE_HOLDS_DOUBLE (value))
    {
      BtkRange *range = BTK_RANGE (widget);
      gdouble new_value;

      new_value = g_value_get_double (value);
      btk_range_set_value (range, new_value);
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

static void
bail_range_finalize (GObject            *object)
{
  BailRange *range = BAIL_RANGE (object);

  if (range->adjustment)
    {
      /*
       * The BtkAdjustment may live on so we need to dicsonnect the
       * signal handler
       */
      if (BAIL_ADJUSTMENT (range->adjustment)->adjustment)
        {
          g_signal_handlers_disconnect_by_func (BAIL_ADJUSTMENT (range->adjustment)->adjustment,
                                                (void *)bail_range_value_changed,
                                                range);
        }
      g_object_unref (range->adjustment);
      range->adjustment = NULL;
    }
  range->activate_keybinding=NULL;
  range->activate_description=NULL;
  if (range->action_idle_handler)
   {
    g_source_remove (range->action_idle_handler);
    range->action_idle_handler = 0;
   }

  G_OBJECT_CLASS (bail_range_parent_class)->finalize (object);
}


static void
bail_range_real_notify_btk (GObject           *obj,
                            GParamSpec        *pspec)
{
  BtkWidget *widget = BTK_WIDGET (obj);
  BailRange *range = BAIL_RANGE (btk_widget_get_accessible (widget));

  if (strcmp (pspec->name, "adjustment") == 0)
    {
      /*
       * Get rid of the BailAdjustment for the BtkAdjustment
       * which was associated with the range.
       */
      if (range->adjustment)
        {
          g_object_unref (range->adjustment);
          range->adjustment = NULL;
        }
      /*
       * Create the BailAdjustment when notify for "adjustment" property
       * is received
       */
      range->adjustment = bail_adjustment_new (BTK_RANGE (widget)->adjustment);
      g_signal_connect (BTK_RANGE (widget)->adjustment,
                        "value-changed",
                        G_CALLBACK (bail_range_value_changed),
                        range);
    }
  else
    BAIL_WIDGET_CLASS (bail_range_parent_class)->notify_btk (obj, pspec);
}

static void
bail_range_value_changed (BtkAdjustment    *adjustment,
                          gpointer         data)
{
  BailRange *range;

  g_return_if_fail (adjustment != NULL);
  bail_return_if_fail (data != NULL);

  range = BAIL_RANGE (data);

  g_object_notify (G_OBJECT (range), "accessible-value");
}

static void
batk_action_interface_init (BatkActionIface *iface)
{
  iface->do_action = bail_range_do_action;
  iface->get_n_actions = bail_range_get_n_actions;
  iface->get_description = bail_range_get_description;
  iface->get_keybinding = bail_range_get_keybinding;
  iface->get_name = bail_range_action_get_name;
  iface->set_description = bail_range_set_description;
}

static gboolean
bail_range_do_action (BatkAction *action,
                     gint      i)
{
  BailRange *range;
  BtkWidget *widget;
  gboolean return_value = TRUE;

  range = BAIL_RANGE (action);
  widget = BTK_ACCESSIBLE (action)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;
  if (!btk_widget_get_sensitive (widget) || !btk_widget_get_visible (widget))
    return FALSE;
  if(i==0)
   {
    if (range->action_idle_handler)
      return_value = FALSE;
    else
      range->action_idle_handler = bdk_threads_add_idle (idle_do_action, range);
   }
  else
     return_value = FALSE;
  return return_value;
}

static gboolean
idle_do_action (gpointer data)
{
  BailRange *range;
  BtkWidget *widget;

  range = BAIL_RANGE (data);
  range->action_idle_handler = 0;
  widget = BTK_ACCESSIBLE (range)->widget;
  if (widget == NULL /* State is defunct */ ||
     !btk_widget_get_sensitive (widget) || !btk_widget_get_visible (widget))
    return FALSE;

   btk_widget_activate (widget);

   return FALSE;
}

static gint
bail_range_get_n_actions (BatkAction *action)
{
    return 1;
}

static const gchar*
bail_range_get_description (BatkAction *action,
                              gint      i)
{
  BailRange *range;
  const gchar *return_value;

  range = BAIL_RANGE (action);
  if (i==0)
   return_value = range->activate_description;
  else
   return_value = NULL;
  return return_value;
}

static const gchar*
bail_range_get_keybinding (BatkAction *action,
                              gint      i)
{
  BailRange *range;
  gchar *return_value = NULL;
  range = BAIL_RANGE (action);
  if(i==0)
   {
    BtkWidget *widget;
    BtkWidget *label;
    BatkRelationSet *set;
    BatkRelation *relation;
    GPtrArray *target;
    gpointer target_object;
    guint key_val;

    range = BAIL_RANGE (action);
    widget = BTK_ACCESSIBLE (range)->widget;
    if (widget == NULL)
       return NULL;
    set = batk_object_ref_relation_set (BATK_OBJECT (action));

    if (!set)
      return NULL;
    label = NULL;
    relation = batk_relation_set_get_relation_by_type (set, BATK_RELATION_LABELLED_BY);    
    if (relation)
     {
      target = batk_relation_get_target (relation);
      target_object = g_ptr_array_index (target, 0);
      if (BTK_IS_ACCESSIBLE (target_object))
         label = BTK_ACCESSIBLE (target_object)->widget;
     }
    g_object_unref (set);
    if (BTK_IS_LABEL (label))
     {
      key_val = btk_label_get_mnemonic_keyval (BTK_LABEL (label));
      if (key_val != BDK_VoidSymbol)
         return_value = btk_accelerator_name (key_val, BDK_MOD1_MASK);
      }
    g_free (range->activate_keybinding);
    range->activate_keybinding = return_value;
   }
  return return_value;
}

static const gchar*
bail_range_action_get_name (BatkAction *action,
                           gint      i)
{
  const gchar *return_value;
  
  if (i==0)
   return_value = "activate";
  else
   return_value = NULL;

  return return_value;
}

static gboolean
bail_range_set_description (BatkAction      *action,
                           gint           i,
                           const gchar    *desc)
{
  BailRange *range;
  gchar **value;

  range = BAIL_RANGE (action);
  
  if (i==0)
   value = &range->activate_description;
  else
   value = NULL;

  if (value)
   {
    g_free (*value);
    *value = g_strdup (desc);
    return TRUE;
   }
  else
   return FALSE;
}


