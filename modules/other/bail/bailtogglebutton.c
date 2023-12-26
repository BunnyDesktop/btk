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
#include "bailtogglebutton.h"

static void      bail_toggle_button_class_init        (BailToggleButtonClass *klass);

static void      bail_toggle_button_init              (BailToggleButton      *button);

static void      bail_toggle_button_toggled_btk       (BtkWidget             *widget);

static void      bail_toggle_button_real_notify_btk   (BObject               *obj,
                                                       BParamSpec            *pspec);

static void      bail_toggle_button_real_initialize   (BatkObject             *obj,
                                                       bpointer              data);

static BatkStateSet* bail_toggle_button_ref_state_set  (BatkObject             *accessible);

G_DEFINE_TYPE (BailToggleButton, bail_toggle_button, BAIL_TYPE_BUTTON)

static void
bail_toggle_button_class_init (BailToggleButtonClass *klass)
{
  BailWidgetClass *widget_class;
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);

  widget_class = (BailWidgetClass*)klass;
  widget_class->notify_btk = bail_toggle_button_real_notify_btk;

  class->ref_state_set = bail_toggle_button_ref_state_set;
  class->initialize = bail_toggle_button_real_initialize;
}

static void
bail_toggle_button_init (BailToggleButton *button)
{
}

static void
bail_toggle_button_real_initialize (BatkObject *obj,
                                    bpointer  data)
{
  BATK_OBJECT_CLASS (bail_toggle_button_parent_class)->initialize (obj, data);

  g_signal_connect (data,
                    "toggled",
                    G_CALLBACK (bail_toggle_button_toggled_btk),
                    NULL);

  if (BTK_IS_CHECK_BUTTON (data))
    obj->role = BATK_ROLE_CHECK_BOX;
  else
    obj->role = BATK_ROLE_TOGGLE_BUTTON;
}

static void
bail_toggle_button_toggled_btk (BtkWidget       *widget)
{
  BatkObject *accessible;
  BtkToggleButton *toggle_button;

  toggle_button = BTK_TOGGLE_BUTTON (widget);

  accessible = btk_widget_get_accessible (widget);
  batk_object_notify_state_change (accessible, BATK_STATE_CHECKED, 
                                  toggle_button->active);
} 

static BatkStateSet*
bail_toggle_button_ref_state_set (BatkObject *accessible)
{
  BatkStateSet *state_set;
  BtkToggleButton *toggle_button;
  BtkWidget *widget;

  state_set = BATK_OBJECT_CLASS (bail_toggle_button_parent_class)->ref_state_set (accessible);
  widget = BTK_ACCESSIBLE (accessible)->widget;
 
  if (widget == NULL)
    return state_set;

  toggle_button = BTK_TOGGLE_BUTTON (widget);

  if (btk_toggle_button_get_active (toggle_button))
    batk_state_set_add_state (state_set, BATK_STATE_CHECKED);

  if (btk_toggle_button_get_inconsistent (toggle_button))
    {
      batk_state_set_remove_state (state_set, BATK_STATE_ENABLED);
      batk_state_set_add_state (state_set, BATK_STATE_INDETERMINATE);
    }
 
  return state_set;
}

static void
bail_toggle_button_real_notify_btk (BObject           *obj,
                                    BParamSpec        *pspec)
{
  BtkToggleButton *toggle_button = BTK_TOGGLE_BUTTON (obj);
  BatkObject *batk_obj;
  bboolean sensitive;
  bboolean inconsistent;

  batk_obj = btk_widget_get_accessible (BTK_WIDGET (toggle_button));
  sensitive = btk_widget_get_sensitive (BTK_WIDGET (toggle_button));
  inconsistent = btk_toggle_button_get_inconsistent (toggle_button);

  if (strcmp (pspec->name, "inconsistent") == 0)
    {
      batk_object_notify_state_change (batk_obj, BATK_STATE_INDETERMINATE, inconsistent);
      batk_object_notify_state_change (batk_obj, BATK_STATE_ENABLED, (sensitive && !inconsistent));
    }
  else if (strcmp (pspec->name, "sensitive") == 0)
    {
      /* Need to override bailwidget behavior of notifying for ENABLED */
      batk_object_notify_state_change (batk_obj, BATK_STATE_SENSITIVE, sensitive);
      batk_object_notify_state_change (batk_obj, BATK_STATE_ENABLED, (sensitive && !inconsistent));
    }
  else
    BAIL_WIDGET_CLASS (bail_toggle_button_parent_class)->notify_btk (obj, pspec);
}
