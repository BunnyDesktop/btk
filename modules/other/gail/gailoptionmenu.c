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

#undef BTK_DISABLE_DEPRECATED

#include <btk/btk.h>
#include <bdk/bdkkeysyms.h>

#include "bailoptionmenu.h"

static void                  bail_option_menu_class_init       (BailOptionMenuClass *klass);
static void                  bail_option_menu_init             (BailOptionMenu  *menu);
static void		     bail_option_menu_real_initialize  (BatkObject       *obj,
                                                                gpointer        data);

static gint                  bail_option_menu_get_n_children   (BatkObject       *obj);
static BatkObject*            bail_option_menu_ref_child        (BatkObject       *obj,
                                                                gint            i);
static gint                  bail_option_menu_real_add_btk     (BtkContainer    *container,
                                                                BtkWidget       *widget,
                                                                gpointer        data);
static gint                  bail_option_menu_real_remove_btk  (BtkContainer    *container,
                                                                BtkWidget       *widget,
                                                                gpointer        data);


static void                  batk_action_interface_init         (BatkActionIface  *iface);

static gboolean              bail_option_menu_do_action        (BatkAction       *action,
                                                                gint            i);
static gboolean              idle_do_action                    (gpointer        data);
static gint                  bail_option_menu_get_n_actions    (BatkAction       *action);
static const gchar*          bail_option_menu_get_description  (BatkAction       *action,
                                                                gint            i);
static const gchar*          bail_option_menu_action_get_name  (BatkAction       *action,
                                                                gint            i);
static gboolean              bail_option_menu_set_description  (BatkAction       *action,
                                                                gint            i,
                                                                const gchar     *desc);
static void                  bail_option_menu_changed          (BtkOptionMenu   *option_menu);

G_DEFINE_TYPE_WITH_CODE (BailOptionMenu, bail_option_menu, BAIL_TYPE_BUTTON,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_ACTION, batk_action_interface_init))

static void
bail_option_menu_class_init (BailOptionMenuClass *klass)
{
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);
  BailContainerClass *container_class;

  container_class = (BailContainerClass *) klass;

  class->get_n_children = bail_option_menu_get_n_children;
  class->ref_child = bail_option_menu_ref_child;
  class->initialize = bail_option_menu_real_initialize;

  container_class->add_btk = bail_option_menu_real_add_btk;
  container_class->remove_btk = bail_option_menu_real_remove_btk;
}

static void
bail_option_menu_init (BailOptionMenu  *menu)
{
}

static void
bail_option_menu_real_initialize (BatkObject *obj,
                                  gpointer  data)
{
  BtkOptionMenu *option_menu;

  BATK_OBJECT_CLASS (bail_option_menu_parent_class)->initialize (obj, data);

  option_menu = BTK_OPTION_MENU (data);

  g_signal_connect (option_menu, "changed",
                    G_CALLBACK (bail_option_menu_changed), NULL);

  obj->role = BATK_ROLE_COMBO_BOX;
}

static gint
bail_option_menu_get_n_children (BatkObject *obj)
{
  BtkWidget *widget;
  BtkOptionMenu *option_menu;
  gint n_children = 0;

  g_return_val_if_fail (BAIL_IS_OPTION_MENU (obj), 0);

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;

  option_menu = BTK_OPTION_MENU (widget);
  if (btk_option_menu_get_menu (option_menu))
      n_children++;

  return n_children;;
}

static BatkObject*
bail_option_menu_ref_child (BatkObject *obj,
                            gint      i)
{
  BtkWidget *widget;
  BatkObject *accessible;

  g_return_val_if_fail (BAIL_IS_OPTION_MENU (obj), NULL);

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;


  if (i == 0)
    accessible = g_object_ref (btk_widget_get_accessible (btk_option_menu_get_menu (BTK_OPTION_MENU (widget))));
   else
    accessible = NULL;

  return accessible;
}

static gint
bail_option_menu_real_add_btk (BtkContainer *container,
                               BtkWidget    *widget,
                               gpointer     data)
{
  BatkObject* batk_parent = BATK_OBJECT (data);
  BatkObject* batk_child = btk_widget_get_accessible (widget);

  BAIL_CONTAINER_CLASS (bail_option_menu_parent_class)->add_btk (container, widget, data);

  g_object_notify (G_OBJECT (batk_child), "accessible_parent");

  g_signal_emit_by_name (batk_parent, "children_changed::add",
			 1, batk_child, NULL);

  return 1;
}

static gint 
bail_option_menu_real_remove_btk (BtkContainer *container,
                                  BtkWidget    *widget,
                                  gpointer     data)
{
  BatkPropertyValues values = { NULL };
  BatkObject* batk_parent = BATK_OBJECT (data);
  BatkObject *batk_child = btk_widget_get_accessible (widget);

  g_value_init (&values.old_value, G_TYPE_POINTER);
  g_value_set_pointer (&values.old_value, batk_parent);

  values.property_name = "accessible-parent";
  g_signal_emit_by_name (batk_child,
                         "property_change::accessible-parent", &values, NULL);
  g_signal_emit_by_name (batk_parent, "children_changed::remove",
			 1, batk_child, NULL);

  return 1;
}

static void
batk_action_interface_init (BatkActionIface *iface)
{
  iface->do_action = bail_option_menu_do_action;
  iface->get_n_actions = bail_option_menu_get_n_actions;
  iface->get_description = bail_option_menu_get_description;
  iface->get_name = bail_option_menu_action_get_name;
  iface->set_description = bail_option_menu_set_description;
}

static gboolean
bail_option_menu_do_action (BatkAction *action,
                            gint      i)
{
  BtkWidget *widget;
  BailButton *button; 
  gboolean return_value = TRUE;

  button = BAIL_BUTTON (action);
  widget = BTK_ACCESSIBLE (action)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  if (!btk_widget_get_sensitive (widget) || !btk_widget_get_visible (widget))
    return FALSE;

  switch (i)
    {
    case 0:
      if (button->action_idle_handler)
        return_value = FALSE;
      else
        button->action_idle_handler = bdk_threads_add_idle (idle_do_action, button);
      break;
    default:
      return_value = FALSE;
      break;
    }
  return return_value; 
}

static gboolean 
idle_do_action (gpointer data)
{
  BtkButton *button; 
  BtkWidget *widget;
  BdkEvent tmp_event;
  BailButton *bail_button;

  bail_button = BAIL_BUTTON (data);
  bail_button->action_idle_handler = 0;

  widget = BTK_ACCESSIBLE (bail_button)->widget;
  if (widget == NULL /* State is defunct */ ||
      !btk_widget_get_sensitive (widget) || !btk_widget_get_visible (widget))
    return FALSE;

  button = BTK_BUTTON (widget); 

  button->in_button = TRUE;
  g_signal_emit_by_name (button, "enter");
  /*
   * Simulate a button press event. calling btk_button_pressed() does
   * not get the job done for a BtkOptionMenu.  
   */
  tmp_event.button.type = BDK_BUTTON_PRESS;
  tmp_event.button.window = widget->window;
  tmp_event.button.button = 1;
  tmp_event.button.send_event = TRUE;
  tmp_event.button.time = BDK_CURRENT_TIME;
  tmp_event.button.axes = NULL;

  btk_widget_event (widget, &tmp_event);

  return FALSE;
}

static gint
bail_option_menu_get_n_actions (BatkAction *action)
{
  return 1;
}

static const gchar*
bail_option_menu_get_description (BatkAction *action,
                                  gint      i)
{
  BailButton *button;
  const gchar *return_value;

  button = BAIL_BUTTON (action);

  switch (i)
    {
    case 0:
      return_value = button->press_description;
      break;
    default:
      return_value = NULL;
      break;
    }
  return return_value; 
}

static const gchar*
bail_option_menu_action_get_name (BatkAction *action,
                                  gint      i)
{
  const gchar *return_value;

  switch (i)
    {
    case 0:
      /*
       * This action simulates a button press by simulating moving the
       * mouse into the button followed by pressing the left mouse button.
       */
      return_value = "press";
      break;
    default:
      return_value = NULL;
      break;
  }
  return return_value; 
}

static gboolean
bail_option_menu_set_description (BatkAction      *action,
                                  gint           i,
                                  const gchar    *desc)
{
  BailButton *button;
  gchar **value;

  button = BAIL_BUTTON (action);

  switch (i)
    {
    case 0:
      value = &button->press_description;
      break;
    default:
      value = NULL;
      break;
    }

  if (value)
    {
      g_free (*value);
      *value = g_strdup (desc);
      return TRUE;
    }
  else
    return FALSE;
}

static void
bail_option_menu_changed (BtkOptionMenu   *option_menu)
{
  BailOptionMenu *bail_option_menu;

  bail_option_menu = BAIL_OPTION_MENU (btk_widget_get_accessible (BTK_WIDGET (option_menu)));
  g_object_notify (G_OBJECT (bail_option_menu), "accessible-name");
}

