/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2008 Jan Arne Petersen
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

#include <config.h>

#include <btk/btk.h>
#include "bailscalebutton.h"
#include "bailadjustment.h"
#include "bail-private-macros.h"

#include <string.h>

static void bail_scale_button_class_init (BailScaleButtonClass *klass);
static void bail_scale_button_init       (BailScaleButton      *button);

/* BailWidget */
static void bail_scale_button_notify_btk (BObject    *obj,
                                          BParamSpec *pspec);

/* BatkObject */
static void bail_scale_button_initialize (BatkObject *obj,
                                          gpointer   data);

/* BatkAction */
static void                  batk_action_interface_init        (BatkActionIface *iface);
static gboolean              bail_scale_button_do_action      (BatkAction      *action,
                                                               gint           i);
static gint                  bail_scale_button_get_n_actions  (BatkAction      *action);
static const gchar*          bail_scale_button_get_description(BatkAction      *action,
                                                               gint           i);
static const gchar*          bail_scale_button_action_get_name(BatkAction      *action,
                                                               gint           i);
static const gchar*          bail_scale_button_get_keybinding (BatkAction      *action,
                                                               gint           i);
static gboolean              bail_scale_button_set_description(BatkAction      *action,
                                                               gint           i,
                                                               const gchar    *desc);

/* BatkValue */
static void	batk_value_interface_init	        (BatkValueIface  *iface);
static void	bail_scale_button_get_current_value     (BatkValue       *obj,
                                                         BValue         *value);
static void	bail_scale_button_get_maximum_value     (BatkValue       *obj,
                                                         BValue         *value);
static void	bail_scale_button_get_minimum_value     (BatkValue       *obj,
                                                         BValue         *value);
static void	bail_scale_button_get_minimum_increment (BatkValue       *obj,
                                                         BValue         *value);
static gboolean	bail_scale_button_set_current_value     (BatkValue       *obj,
                                                         const BValue   *value);

G_DEFINE_TYPE_WITH_CODE (BailScaleButton, bail_scale_button, BAIL_TYPE_BUTTON,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_ACTION, batk_action_interface_init)
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_VALUE, batk_value_interface_init));

static void
bail_scale_button_class_init (BailScaleButtonClass *klass)
{
  BatkObjectClass *batk_object_class = BATK_OBJECT_CLASS (klass);
  BailWidgetClass *widget_class = BAIL_WIDGET_CLASS (klass);

  batk_object_class->initialize = bail_scale_button_initialize;

  widget_class->notify_btk = bail_scale_button_notify_btk;
}

static void
bail_scale_button_init (BailScaleButton *button)
{
}

static void
bail_scale_button_initialize (BatkObject *obj,
                              gpointer   data)
{
  BATK_OBJECT_CLASS (bail_scale_button_parent_class)->initialize (obj, data);

  obj->role = BATK_ROLE_SLIDER;
}

static void
batk_action_interface_init (BatkActionIface *iface)
{
  iface->do_action = bail_scale_button_do_action;
  iface->get_n_actions = bail_scale_button_get_n_actions;
  iface->get_description = bail_scale_button_get_description;
  iface->get_keybinding = bail_scale_button_get_keybinding;
  iface->get_name = bail_scale_button_action_get_name;
  iface->set_description = bail_scale_button_set_description;
}

static gboolean
bail_scale_button_do_action(BatkAction *action,
                            gint       i)
{
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (action)->widget;
  if (widget == NULL)
    return FALSE;

  if (!btk_widget_is_sensitive (widget) || !btk_widget_get_visible (widget))
    return FALSE;

  switch (i) {
    case 0:
      g_signal_emit_by_name (widget, "popup");
      return TRUE;
    case 1:
      g_signal_emit_by_name (widget, "podown");
      return TRUE;
    default:
      return FALSE;
  }
}

static gint
bail_scale_button_get_n_actions (BatkAction *action)
{
  return 2;
}

static const gchar*
bail_scale_button_get_description (BatkAction *action,
                                   gint       i)
{
  return NULL;
}

static const gchar*
bail_scale_button_action_get_name (BatkAction *action,
                                   gint       i)
{
  switch (i) {
    case 0:
      return "popup";
    case 1:
      return "popdown";
    default:
      return NULL;
  }
}

static const gchar*
bail_scale_button_get_keybinding (BatkAction *action,
                                  gint       i)
{
  return NULL;
}

static gboolean
bail_scale_button_set_description (BatkAction   *action,
                                   gint         i,
                                   const gchar *desc)
{
  return FALSE;
}


static void
batk_value_interface_init (BatkValueIface *iface)
{
  iface->get_current_value = bail_scale_button_get_current_value;
  iface->get_maximum_value = bail_scale_button_get_maximum_value;
  iface->get_minimum_value = bail_scale_button_get_minimum_value;
  iface->get_minimum_increment = bail_scale_button_get_minimum_increment;
  iface->set_current_value = bail_scale_button_set_current_value;
}

static void
bail_scale_button_get_current_value (BatkValue *obj,
                                     BValue   *value)
{
  BtkScaleButton *btk_scale_button;

  g_return_if_fail (BAIL_IS_SCALE_BUTTON (obj));

  btk_scale_button = BTK_SCALE_BUTTON (BTK_ACCESSIBLE (obj)->widget);

  b_value_set_double (b_value_init (value, B_TYPE_DOUBLE),
                      btk_scale_button_get_value (btk_scale_button));
}

static void
bail_scale_button_get_maximum_value (BatkValue *obj,
                                     BValue   *value)
{
  BtkWidget *btk_widget;
  BtkAdjustment *adj;

  g_return_if_fail (BAIL_IS_SCALE_BUTTON (obj));

  btk_widget = BTK_ACCESSIBLE (obj)->widget;
  if (btk_widget == NULL)
    return;

  adj = btk_scale_button_get_adjustment (BTK_SCALE_BUTTON (btk_widget));
  if (adj != NULL)
    b_value_set_double (b_value_init (value, B_TYPE_DOUBLE),
                        adj->upper);
}

static void
bail_scale_button_get_minimum_value (BatkValue *obj,
                                     BValue   *value)
{
  BtkWidget *btk_widget;
  BtkAdjustment *adj;

  g_return_if_fail (BAIL_IS_SCALE_BUTTON (obj));

  btk_widget = BTK_ACCESSIBLE (obj)->widget;
  if (btk_widget == NULL)
    return;

  adj = btk_scale_button_get_adjustment (BTK_SCALE_BUTTON (btk_widget));
  if (adj != NULL)
    b_value_set_double (b_value_init (value, B_TYPE_DOUBLE),
                        adj->lower);
}

static void
bail_scale_button_get_minimum_increment (BatkValue *obj,
                                         BValue   *value)
{
  BtkWidget *btk_widget;
  BtkAdjustment *adj;

  g_return_if_fail (BAIL_IS_SCALE_BUTTON (obj));

  btk_widget = BTK_ACCESSIBLE (obj)->widget;
  if (btk_widget == NULL)
    return;

  adj = btk_scale_button_get_adjustment (BTK_SCALE_BUTTON (btk_widget));
  if (adj != NULL)
    b_value_set_double (b_value_init (value, B_TYPE_DOUBLE),
                        adj->step_increment);
}

static gboolean
bail_scale_button_set_current_value (BatkValue     *obj,
                                     const BValue *value)
{
  BtkWidget *btk_widget;

  g_return_val_if_fail (BAIL_IS_SCALE_BUTTON (obj), FALSE);

  btk_widget = BTK_ACCESSIBLE (obj)->widget;
  if (btk_widget == NULL)
    return FALSE;

  if (G_VALUE_HOLDS_DOUBLE (value))
    {
      btk_scale_button_set_value (BTK_SCALE_BUTTON (btk_widget), b_value_get_double (value));
      return TRUE;
    }
  return FALSE;
}

static void
bail_scale_button_notify_btk (BObject    *obj,
                              BParamSpec *pspec)
{
  BtkScaleButton *btk_scale_button;
  BailScaleButton *scale_button;

  g_return_if_fail (BTK_IS_SCALE_BUTTON (obj));

  btk_scale_button = BTK_SCALE_BUTTON (obj);
  scale_button = BAIL_SCALE_BUTTON (btk_widget_get_accessible (BTK_WIDGET (btk_scale_button)));

  if (strcmp (pspec->name, "value") == 0)
    {
      g_object_notify (B_OBJECT (scale_button), "accessible-value");
    }
  else
    {
      BAIL_WIDGET_CLASS (bail_scale_button_parent_class)->notify_btk (obj, pspec);
    }
}


