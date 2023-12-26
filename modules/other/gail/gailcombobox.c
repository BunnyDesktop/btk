/* BAIL - The GNOME Accessibility Implementation Library
 * Copyright 2004 Sun Microsystems Inc.
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

#undef BTK_DISABLE_DEPRECATED

#include "config.h"

#include <btk/btk.h>
#include <bdk/bdkkeysyms.h>
#include "bailcombobox.h"

static void         bail_combo_box_class_init              (BailComboBoxClass *klass);
static void         bail_combo_box_init                    (BailComboBox      *combo_box);
static void         bail_combo_box_real_initialize         (BatkObject      *obj,
                                                            gpointer       data);

static void         bail_combo_box_changed_btk             (BtkWidget      *widget);

static const gchar* bail_combo_box_get_name                (BatkObject      *obj);
static gint         bail_combo_box_get_n_children          (BatkObject      *obj);
static BatkObject*   bail_combo_box_ref_child               (BatkObject      *obj,
                                                            gint           i);
static void         bail_combo_box_finalize                (GObject        *object);
static void         batk_action_interface_init              (BatkActionIface *iface);

static gboolean     bail_combo_box_do_action               (BatkAction      *action,
                                                            gint           i);
static gboolean     idle_do_action                         (gpointer       data);
static gint         bail_combo_box_get_n_actions           (BatkAction      *action);
static const gchar* bail_combo_box_get_description         (BatkAction      *action,
                                                            gint           i);
static const gchar* bail_combo_box_get_keybinding          (BatkAction       *action,
		                                             gint            i);
static const gchar* bail_combo_box_action_get_name         (BatkAction      *action,
                                                            gint           i);
static gboolean              bail_combo_box_set_description(BatkAction      *action,
                                                            gint           i,
                                                            const gchar    *desc);
static void         batk_selection_interface_init           (BatkSelectionIface *iface);
static gboolean     bail_combo_box_add_selection           (BatkSelection   *selection,
                                                            gint           i);
static gboolean     bail_combo_box_clear_selection         (BatkSelection   *selection);
static BatkObject*   bail_combo_box_ref_selection           (BatkSelection   *selection,
                                                            gint           i);
static gint         bail_combo_box_get_selection_count     (BatkSelection   *selection);
static gboolean     bail_combo_box_is_child_selected       (BatkSelection   *selection,
                                                            gint           i);
static gboolean     bail_combo_box_remove_selection        (BatkSelection   *selection,
                                                            gint           i);

G_DEFINE_TYPE_WITH_CODE (BailComboBox, bail_combo_box, BAIL_TYPE_CONTAINER,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_ACTION, batk_action_interface_init)
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_SELECTION, batk_selection_interface_init))

static void
bail_combo_box_class_init (BailComboBoxClass *klass)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (klass);
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);

  bobject_class->finalize = bail_combo_box_finalize;

  class->get_name = bail_combo_box_get_name;
  class->get_n_children = bail_combo_box_get_n_children;
  class->ref_child = bail_combo_box_ref_child;
  class->initialize = bail_combo_box_real_initialize;
}

static void
bail_combo_box_init (BailComboBox      *combo_box)
{
  combo_box->press_description = NULL;
  combo_box->press_keybinding = NULL;
  combo_box->old_selection = -1;
  combo_box->name = NULL;
  combo_box->popup_set = FALSE;
}

static void
bail_combo_box_real_initialize (BatkObject *obj,
                                gpointer  data)
{
  BtkComboBox *combo_box;
  BailComboBox *bail_combo_box;
  BatkObject *popup;

  BATK_OBJECT_CLASS (bail_combo_box_parent_class)->initialize (obj, data);

  combo_box = BTK_COMBO_BOX (data);

  bail_combo_box = BAIL_COMBO_BOX (obj);

  g_signal_connect (combo_box,
                    "changed",
                    G_CALLBACK (bail_combo_box_changed_btk),
                    NULL);
  bail_combo_box->old_selection = btk_combo_box_get_active (combo_box);

  popup = btk_combo_box_get_popup_accessible (combo_box);
  if (popup)
    {
      batk_object_set_parent (popup, obj);
      bail_combo_box->popup_set = TRUE;
    }
  if (btk_combo_box_get_has_entry (combo_box))
    batk_object_set_parent (btk_widget_get_accessible (btk_bin_get_child (BTK_BIN (combo_box))), obj);

  obj->role = BATK_ROLE_COMBO_BOX;
}

static void
bail_combo_box_changed_btk (BtkWidget *widget)
{
  BtkComboBox *combo_box;
  BatkObject *obj;
  BailComboBox *bail_combo_box;
  gint index;

  combo_box = BTK_COMBO_BOX (widget);

  index = btk_combo_box_get_active (combo_box);
  obj = btk_widget_get_accessible (widget);
  bail_combo_box = BAIL_COMBO_BOX (obj);
  if (bail_combo_box->old_selection != index)
    {
      bail_combo_box->old_selection = index;
      g_object_notify (G_OBJECT (obj), "accessible-name");
      g_signal_emit_by_name (obj, "selection_changed");
    }
}

static const gchar*
bail_combo_box_get_name (BatkObject *obj)
{
  BtkWidget *widget;
  BtkComboBox *combo_box;
  BailComboBox *bail_combo_box;
  BtkTreeIter iter;
  const gchar *name;
  BtkTreeModel *model;
  gint n_columns;
  gint i;

  g_return_val_if_fail (BAIL_IS_COMBO_BOX (obj), NULL);

  name = BATK_OBJECT_CLASS (bail_combo_box_parent_class)->get_name (obj);
  if (name)
    return name;

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  combo_box = BTK_COMBO_BOX (widget);
  bail_combo_box = BAIL_COMBO_BOX (obj);
  if (btk_combo_box_get_active_iter (combo_box, &iter))
    {
      model = btk_combo_box_get_model (combo_box);
      n_columns = btk_tree_model_get_n_columns (model);
      for (i = 0; i < n_columns; i++)
        {
          GValue value = { 0, };

          btk_tree_model_get_value (model, &iter, i, &value);
          if (G_VALUE_HOLDS_STRING (&value))
            {
	      if (bail_combo_box->name) g_free (bail_combo_box->name);
              bail_combo_box->name =  g_strdup ((gchar *) 
						g_value_get_string (&value));
	      g_value_unset (&value);
              break;
            }
	  else
	    g_value_unset (&value);
        }
    }
  return bail_combo_box->name;
}

/*
 * The children of a BailComboBox are the list of items and the entry field
 * if it is editable.
 */
static gint
bail_combo_box_get_n_children (BatkObject* obj)
{
  gint n_children = 0;
  BtkWidget *widget;

  g_return_val_if_fail (BAIL_IS_COMBO_BOX (obj), 0);

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;

  n_children++;
  if (btk_combo_box_get_has_entry (BTK_COMBO_BOX (widget)) ||
      BTK_IS_COMBO_BOX_ENTRY (widget))
    n_children ++;

  return n_children;
}

static BatkObject*
bail_combo_box_ref_child (BatkObject *obj,
                          gint      i)
{
  BtkWidget *widget;
  BatkObject *child;
  BailComboBox *box;

  g_return_val_if_fail (BAIL_IS_COMBO_BOX (obj), NULL);

  widget = BTK_ACCESSIBLE (obj)->widget;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  if (i == 0)
    {
      child = btk_combo_box_get_popup_accessible (BTK_COMBO_BOX (widget));
      box = BAIL_COMBO_BOX (obj);
      if (box->popup_set == FALSE)
        {
          batk_object_set_parent (child, obj);
          box->popup_set = TRUE;
        }
    }
  else if (i == 1 && (btk_combo_box_get_has_entry (BTK_COMBO_BOX (widget)) ||
                      BTK_IS_COMBO_BOX_ENTRY (widget)))
    {
      child = btk_widget_get_accessible (btk_bin_get_child (BTK_BIN (widget)));
    }
  else
    {
      return NULL;
    }
  return g_object_ref (child);
}

static void
batk_action_interface_init (BatkActionIface *iface)
{
  iface->do_action = bail_combo_box_do_action;
  iface->get_n_actions = bail_combo_box_get_n_actions;
  iface->get_description = bail_combo_box_get_description;
  iface->get_keybinding = bail_combo_box_get_keybinding;
  iface->get_name = bail_combo_box_action_get_name;
  iface->set_description = bail_combo_box_set_description;
}

static gboolean
bail_combo_box_do_action (BatkAction *action,
                          gint      i)
{
  BailComboBox *combo_box;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (action)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  if (!btk_widget_get_sensitive (widget) || !btk_widget_get_visible (widget))
    return FALSE;

  combo_box = BAIL_COMBO_BOX (action);
  if (i == 0)
    {
      if (combo_box->action_idle_handler)
        return FALSE;

      combo_box->action_idle_handler = bdk_threads_add_idle (idle_do_action, combo_box);
      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
idle_do_action (gpointer data)
{
  BtkComboBox *combo_box;
  BtkWidget *widget;
  BailComboBox *bail_combo_box;
  BatkObject *popup;
  gboolean do_popup;

  bail_combo_box = BAIL_COMBO_BOX (data);
  bail_combo_box->action_idle_handler = 0;
  widget = BTK_ACCESSIBLE (bail_combo_box)->widget;
  if (widget == NULL || /* State is defunct */
      !btk_widget_get_sensitive (widget) || !btk_widget_get_visible (widget))
    return FALSE;

  combo_box = BTK_COMBO_BOX (widget);

  popup = btk_combo_box_get_popup_accessible (combo_box);
  do_popup = !btk_widget_get_mapped (BTK_ACCESSIBLE (popup)->widget);
  if (do_popup)
      btk_combo_box_popup (combo_box);
  else
      btk_combo_box_popdown (combo_box);

  return FALSE;
}

static gint
bail_combo_box_get_n_actions (BatkAction *action)
{
  /*
   * The default behavior of a combo_box box is to have one action -
   */
  return 1;
}

static const gchar*
bail_combo_box_get_description (BatkAction *action,
                           gint      i)
{
  if (i == 0)
    {
      BailComboBox *combo_box;

      combo_box = BAIL_COMBO_BOX (action);
      return combo_box->press_description;
    }
  else
    return NULL;
}

static const gchar*
bail_combo_box_get_keybinding (BatkAction *action,
		                    gint      i)
{
  BailComboBox *combo_box;
  gchar *return_value = NULL;
  switch (i)
  {
     case 0:
      {
	  BtkWidget *widget;
	  BtkWidget *label;
	  BatkRelationSet *set;
	  BatkRelation *relation;
	  GPtrArray *target;
	  gpointer target_object;
	  guint key_val;

	  combo_box = BAIL_COMBO_BOX (action);
	  widget = BTK_ACCESSIBLE (combo_box)->widget;
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
	     {
	        label = BTK_ACCESSIBLE (target_object)->widget;
	     }
	  }
	  g_object_unref (set);
	  if (BTK_IS_LABEL (label))
	  {
             key_val = btk_label_get_mnemonic_keyval (BTK_LABEL (label));
	     if (key_val != BDK_VoidSymbol)
             return_value = btk_accelerator_name (key_val, BDK_MOD1_MASK);
	  }
	   g_free (combo_box->press_keybinding);
	   combo_box->press_keybinding = return_value;
	   break;
       }
    default:
	   break;
  }
  return return_value;
}


static const gchar*
bail_combo_box_action_get_name (BatkAction *action,
                                gint      i)
{
  if (i == 0)
    return "press";
  else
    return NULL;
}

static gboolean
bail_combo_box_set_description (BatkAction   *action,
                                gint        i,
                                const gchar *desc)
{
  if (i == 0)
    {
      BailComboBox *combo_box;

      combo_box = BAIL_COMBO_BOX (action);
      g_free (combo_box->press_description);
      combo_box->press_description = g_strdup (desc);
      return TRUE;
    }
  else
    return FALSE;
}

static void
batk_selection_interface_init (BatkSelectionIface *iface)
{
  iface->add_selection = bail_combo_box_add_selection;
  iface->clear_selection = bail_combo_box_clear_selection;
  iface->ref_selection = bail_combo_box_ref_selection;
  iface->get_selection_count = bail_combo_box_get_selection_count;
  iface->is_child_selected = bail_combo_box_is_child_selected;
  iface->remove_selection = bail_combo_box_remove_selection;
  /*
   * select_all_selection does not make sense for a combo_box
   * so no implementation is provided.
   */
}

static gboolean
bail_combo_box_add_selection (BatkSelection *selection,
                              gint         i)
{
  BtkComboBox *combo_box;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  combo_box = BTK_COMBO_BOX (widget);

  btk_combo_box_set_active (combo_box, i);
  return TRUE;
}

static gboolean 
bail_combo_box_clear_selection (BatkSelection *selection)
{
  BtkComboBox *combo_box;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  combo_box = BTK_COMBO_BOX (widget);

  btk_combo_box_set_active (combo_box, -1);
  return TRUE;
}

static BatkObject*
bail_combo_box_ref_selection (BatkSelection *selection,
                              gint         i)
{
  BtkComboBox *combo_box;
  BtkWidget *widget;
  BatkObject *obj;
  gint index;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  combo_box = BTK_COMBO_BOX (widget);

  /*
   * A combo_box box can have only one selection.
   */
  if (i != 0)
    return NULL;

  obj = btk_combo_box_get_popup_accessible (combo_box);
  index = btk_combo_box_get_active (combo_box);
  return batk_object_ref_accessible_child (obj, index);
}

static gint
bail_combo_box_get_selection_count (BatkSelection *selection)
{
  BtkComboBox *combo_box;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;

  combo_box = BTK_COMBO_BOX (widget);

  return (btk_combo_box_get_active (combo_box) == -1) ? 0 : 1;
}

static gboolean
bail_combo_box_is_child_selected (BatkSelection *selection,
                                  gint         i)
{
  BtkComboBox *combo_box;
  gint j;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  combo_box = BTK_COMBO_BOX (widget);

  j = btk_combo_box_get_active (combo_box);

  return (j == i);
}

static gboolean
bail_combo_box_remove_selection (BatkSelection *selection,
                                 gint         i)
{
  if (batk_selection_is_child_selected (selection, i))
    batk_selection_clear_selection (selection);

  return TRUE;
}

static void
bail_combo_box_finalize (GObject *object)
{
  BailComboBox *combo_box = BAIL_COMBO_BOX (object);

  g_free (combo_box->press_description);
  g_free (combo_box->press_keybinding);
  g_free (combo_box->name);
  if (combo_box->action_idle_handler)
    {
      g_source_remove (combo_box->action_idle_handler);
      combo_box->action_idle_handler = 0;
    }
  G_OBJECT_CLASS (bail_combo_box_parent_class)->finalize (object);
}
