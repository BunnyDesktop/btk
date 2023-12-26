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

#undef BTK_DISABLE_DEPRECATED

#include <btk/btk.h>
#include "bailcombo.h"

static void         bail_combo_class_init              (BailComboClass *klass);
static void         bail_combo_init                    (BailCombo      *combo);
static void         bail_combo_real_initialize         (BatkObject      *obj,
                                                        bpointer       data);

static void         bail_combo_selection_changed_btk   (BtkWidget      *widget,
                                                        bpointer       data);

static bint         bail_combo_get_n_children          (BatkObject      *obj);
static BatkObject*   bail_combo_ref_child               (BatkObject      *obj,
                                                        bint           i);
static void         bail_combo_finalize                (BObject        *object);
static void         batk_action_interface_init          (BatkActionIface *iface);

static bboolean     bail_combo_do_action               (BatkAction      *action,
                                                        bint           i);
static bboolean     idle_do_action                     (bpointer       data);
static bint         bail_combo_get_n_actions           (BatkAction      *action)
;
static const bchar* bail_combo_get_description         (BatkAction      *action,
                                                        bint           i);
static const bchar* bail_combo_get_name                (BatkAction      *action,
                                                        bint           i);
static bboolean              bail_combo_set_description(BatkAction      *action,
                                                        bint           i,
                                                        const bchar    *desc);

static void         batk_selection_interface_init       (BatkSelectionIface *iface);
static bboolean     bail_combo_add_selection           (BatkSelection   *selection,
                                                        bint           i);
static bboolean     bail_combo_clear_selection         (BatkSelection   *selection);
static BatkObject*   bail_combo_ref_selection           (BatkSelection   *selection,
                                                        bint           i);
static bint         bail_combo_get_selection_count     (BatkSelection   *selection);
static bboolean     bail_combo_is_child_selected       (BatkSelection   *selection,
                                                        bint           i);
static bboolean     bail_combo_remove_selection        (BatkSelection   *selection,
                                                        bint           i);

static bint         _bail_combo_button_release         (bpointer       data);
static bint         _bail_combo_popup_release          (bpointer       data);


G_DEFINE_TYPE_WITH_CODE (BailCombo, bail_combo, BAIL_TYPE_CONTAINER,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_ACTION, batk_action_interface_init)
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_SELECTION, batk_selection_interface_init))

static void
bail_combo_class_init (BailComboClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);

  bobject_class->finalize = bail_combo_finalize;

  class->get_n_children = bail_combo_get_n_children;
  class->ref_child = bail_combo_ref_child;
  class->initialize = bail_combo_real_initialize;
}

static void
bail_combo_init (BailCombo      *combo)
{
  combo->press_description = NULL;
  combo->old_selection = NULL;
  combo->deselect_idle_handler = 0;
  combo->select_idle_handler = 0;
}

static void
bail_combo_real_initialize (BatkObject *obj,
                            bpointer  data)
{
  BtkCombo *combo;
  BtkList *list;
  GList *slist; 
  BailCombo *bail_combo;

  BATK_OBJECT_CLASS (bail_combo_parent_class)->initialize (obj, data);

  combo = BTK_COMBO (data);

  list = BTK_LIST (combo->list);
  slist = list->selection;

  bail_combo = BAIL_COMBO (obj);
  if (slist && slist->data)
    {
      bail_combo->old_selection = slist->data;
    }
  g_signal_connect (combo->list,
                    "selection_changed",
                    G_CALLBACK (bail_combo_selection_changed_btk),
                    data);
  batk_object_set_parent (btk_widget_get_accessible (combo->entry), obj);
  batk_object_set_parent (btk_widget_get_accessible (combo->popup), obj);

  obj->role = BATK_ROLE_COMBO_BOX;
}

static bboolean
notify_deselect (bpointer data)
{
  BailCombo *combo;

  combo = BAIL_COMBO (data);

  combo->old_selection = NULL;
  combo->deselect_idle_handler = 0;
  g_signal_emit_by_name (data, "selection_changed");

  return FALSE;
}

static bboolean
notify_select (bpointer data)
{
  BailCombo *combo;

  combo = BAIL_COMBO (data);

  combo->select_idle_handler = 0;
  g_signal_emit_by_name (data, "selection_changed");

  return FALSE;
}

static void
bail_combo_selection_changed_btk (BtkWidget      *widget,
                                  bpointer       data)
{
  BtkCombo *combo;
  BtkList *list;
  GList *slist; 
  BatkObject *obj;
  BailCombo *bail_combo;

  combo = BTK_COMBO (data);
  list = BTK_LIST (combo->list);
  
  slist = list->selection;

  obj = btk_widget_get_accessible (BTK_WIDGET (data));
  bail_combo = BAIL_COMBO (obj);
  if (slist && slist->data)
    {
      if (slist->data != bail_combo->old_selection)
        {
          bail_combo->old_selection = slist->data;
          if (bail_combo->select_idle_handler == 0)
            bail_combo->select_idle_handler = bdk_threads_add_idle (notify_select, bail_combo);
        }
      if (bail_combo->deselect_idle_handler)
        {
          g_source_remove (bail_combo->deselect_idle_handler);
          bail_combo->deselect_idle_handler = 0;       
        }
    }
  else
    {
      if (bail_combo->deselect_idle_handler == 0)
        bail_combo->deselect_idle_handler = bdk_threads_add_idle (notify_deselect, bail_combo);
      if (bail_combo->select_idle_handler)
        {
          g_source_remove (bail_combo->select_idle_handler);
          bail_combo->select_idle_handler = 0;       
        }
    }
}

/*
 * The children of a BailCombo are the list of items and the entry field
 */
static bint
bail_combo_get_n_children (BatkObject* obj)
{
  bint n_children = 2;
  BtkWidget *widget;

  g_return_val_if_fail (BAIL_IS_COMBO (obj), 0);

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;

  return n_children;
}

static BatkObject*
bail_combo_ref_child (BatkObject *obj,
                      bint      i)
{
  BatkObject *accessible;
  BtkWidget *widget;

  g_return_val_if_fail (BAIL_IS_COMBO (obj), NULL);

  if (i < 0 || i > 1)
    return NULL;

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  if (i == 0)
    accessible = btk_widget_get_accessible (BTK_COMBO (widget)->popup);
  else 
    accessible = btk_widget_get_accessible (BTK_COMBO (widget)->entry);

  g_object_ref (accessible);
  return accessible;
}

static void
batk_action_interface_init (BatkActionIface *iface)
{
  iface->do_action = bail_combo_do_action;
  iface->get_n_actions = bail_combo_get_n_actions;
  iface->get_description = bail_combo_get_description;
  iface->get_name = bail_combo_get_name;
  iface->set_description = bail_combo_set_description;
}

static bboolean
bail_combo_do_action (BatkAction *action,
                       bint      i)
{
  BailCombo *combo;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (action)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  if (!btk_widget_get_sensitive (widget) || !btk_widget_get_visible (widget))
    return FALSE;

  combo = BAIL_COMBO (action);
  if (i == 0)
    {
      if (combo->action_idle_handler)
        return FALSE;

      combo->action_idle_handler = bdk_threads_add_idle (idle_do_action, combo);
      return TRUE;
    }
  else
    return FALSE;
}

/*
 * This action is the pressing of the button on the combo box.
 * The behavior is different depending on whether the list is being
 * displayed or removed.
 *
 * A button press event is simulated on the appropriate widget and 
 * a button release event is simulated in an idle function.
 */
static bboolean
idle_do_action (bpointer data)
{
  BtkCombo *combo;
  BtkWidget *action_widget;
  BtkWidget *widget;
  BailCombo *bail_combo;
  bboolean do_popup;
  BdkEvent tmp_event;

  bail_combo = BAIL_COMBO (data);
  bail_combo->action_idle_handler = 0;
  widget = BTK_ACCESSIBLE (bail_combo)->widget;
  if (widget == NULL /* State is defunct */ ||
      !btk_widget_get_sensitive (widget) || !btk_widget_get_visible (widget))
    return FALSE;

  combo = BTK_COMBO (widget);

  do_popup = !btk_widget_get_mapped (combo->popwin);

  tmp_event.button.type = BDK_BUTTON_PRESS; 
  tmp_event.button.window = widget->window;
  tmp_event.button.button = 1; 
  tmp_event.button.send_event = TRUE;
  tmp_event.button.time = BDK_CURRENT_TIME;
  tmp_event.button.axes = NULL;

  if (do_popup)
    {
      /* Pop up list */
      action_widget = combo->button;

      btk_widget_event (action_widget, &tmp_event);

      /* FIXME !*/
      g_idle_add (_bail_combo_button_release, combo);
    }
    else
    {
      /* Pop down list */
      tmp_event.button.window = combo->list->window;
      bdk_window_set_user_data (combo->list->window, combo->button);
      action_widget = combo->popwin;
    
      btk_widget_event (action_widget, &tmp_event);
      /* FIXME !*/
      g_idle_add (_bail_combo_popup_release, combo);
    }

  return FALSE;
}

static bint
bail_combo_get_n_actions (BatkAction *action)
{
  /*
   * The default behavior of a combo box is to have one action -
   */
  return 1;
}

static const bchar*
bail_combo_get_description (BatkAction *action,
                           bint      i)
{
  if (i == 0)
    {
      BailCombo *combo;

      combo = BAIL_COMBO (action);
      return combo->press_description;
    }
  else
    return NULL;
}

static const bchar*
bail_combo_get_name (BatkAction *action,
                     bint      i)
{
  if (i == 0)
    return "press";
  else
    return NULL;
}

static void
batk_selection_interface_init (BatkSelectionIface *iface)
{
  iface->add_selection = bail_combo_add_selection;
  iface->clear_selection = bail_combo_clear_selection;
  iface->ref_selection = bail_combo_ref_selection;
  iface->get_selection_count = bail_combo_get_selection_count;
  iface->is_child_selected = bail_combo_is_child_selected;
  iface->remove_selection = bail_combo_remove_selection;
  /*
   * select_all_selection does not make sense for a combo box
   * so no implementation is provided.
   */
}

static bboolean
bail_combo_add_selection (BatkSelection   *selection,
                          bint           i)
{
  BtkCombo *combo;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  combo = BTK_COMBO (widget);

  btk_list_select_item (BTK_LIST (combo->list), i);
  return TRUE;
}

static bboolean 
bail_combo_clear_selection (BatkSelection   *selection)
{
  BtkCombo *combo;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  combo = BTK_COMBO (widget);

  btk_list_unselect_all (BTK_LIST (combo->list));
  return TRUE;
}

static BatkObject*
bail_combo_ref_selection (BatkSelection   *selection,
                          bint           i)
{
  BtkCombo *combo;
  GList * list;
  BtkWidget *item;
  BatkObject *obj;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  combo = BTK_COMBO (widget);

  /*
   * A combo box can have only one selection.
   */
  if (i != 0)
    return NULL;

  list = BTK_LIST (combo->list)->selection;

  if (list == NULL)
    return NULL;

  item = BTK_WIDGET (list->data);

  obj = btk_widget_get_accessible (item);
  g_object_ref (obj);
  return obj;
}

static bint
bail_combo_get_selection_count (BatkSelection   *selection)
{
  BtkCombo *combo;
  GList * list;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;

  combo = BTK_COMBO (widget);

  /*
   * The number of children currently selected is either 1 or 0 so we
   * do not bother to count the elements of the selected list.
   */
  list = BTK_LIST (combo->list)->selection;

  if (list == NULL)
    return 0;
  else
    return 1;
}

static bboolean
bail_combo_is_child_selected (BatkSelection   *selection,
                              bint           i)
{
  BtkCombo *combo;
  GList * list;
  BtkWidget *item;
  bint j;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  combo = BTK_COMBO (widget);

  list = BTK_LIST (combo->list)->selection;

  if (list == NULL)
    return FALSE;

  item = BTK_WIDGET (list->data);

  j = g_list_index (BTK_LIST (combo->list)->children, item);

  return (j == i);
}

static bboolean
bail_combo_remove_selection (BatkSelection   *selection,
                             bint           i)
{
  if (batk_selection_is_child_selected (selection, i))
    batk_selection_clear_selection (selection);

  return TRUE;
}

static bint 
_bail_combo_popup_release (bpointer data)
{
  BtkCombo *combo;
  BtkWidget *action_widget;
  BdkEvent tmp_event;

  BDK_THREADS_ENTER ();

  combo = BTK_COMBO (data);
  if (combo->current_button == 0)
    {
      BDK_THREADS_LEAVE ();
      return FALSE;
    }

  tmp_event.button.type = BDK_BUTTON_RELEASE; 
  tmp_event.button.button = 1; 
  tmp_event.button.time = BDK_CURRENT_TIME;
  action_widget = combo->button;

  btk_widget_event (action_widget, &tmp_event);

  BDK_THREADS_LEAVE ();

  return FALSE;
}

static bint 
_bail_combo_button_release (bpointer data)
{
  BtkCombo *combo;
  BtkWidget *action_widget;
  BdkEvent tmp_event;

  BDK_THREADS_ENTER ();

  combo = BTK_COMBO (data);
  if (combo->current_button == 0)
    {
      BDK_THREADS_LEAVE ();
      return FALSE;
    }

  tmp_event.button.type = BDK_BUTTON_RELEASE; 
  tmp_event.button.button = 1; 
  tmp_event.button.window = combo->list->window;
  tmp_event.button.time = BDK_CURRENT_TIME;
  bdk_window_set_user_data (combo->list->window, combo->button);
  action_widget = combo->list;

  btk_widget_event (action_widget, &tmp_event);

  BDK_THREADS_LEAVE ();

  return FALSE;
}

static bboolean
bail_combo_set_description (BatkAction      *action,
                            bint           i,
                            const bchar    *desc)
{
  if (i == 0)
    {
      BailCombo *combo;

      combo = BAIL_COMBO (action);
      g_free (combo->press_description);
      combo->press_description = g_strdup (desc);
      return TRUE;
    }
  else
    return FALSE;
}

static void
bail_combo_finalize (BObject            *object)
{
  BailCombo *combo = BAIL_COMBO (object);

  g_free (combo->press_description);
  if (combo->action_idle_handler)
    {
      g_source_remove (combo->action_idle_handler);
      combo->action_idle_handler = 0;
    }
  if (combo->deselect_idle_handler)
    {
      g_source_remove (combo->deselect_idle_handler);
      combo->deselect_idle_handler = 0;       
    }
  if (combo->select_idle_handler)
    {
      g_source_remove (combo->select_idle_handler);
      combo->select_idle_handler = 0;       
    }
  B_OBJECT_CLASS (bail_combo_parent_class)->finalize (object);
}
