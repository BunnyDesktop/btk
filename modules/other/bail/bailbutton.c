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
#include <bdk/bdkkeysyms.h>
#include "bailbutton.h"
#include <libbail-util/bailmisc.h>

#define BAIL_BUTTON_ATTACHED_MENUS "btk-attached-menus"

static void                  bail_button_class_init       (BailButtonClass *klass);
static void                  bail_button_init             (BailButton      *button);

static const gchar*          bail_button_get_name         (BatkObject       *obj);
static gint                  bail_button_get_n_children   (BatkObject       *obj);
static BatkObject*            bail_button_ref_child        (BatkObject       *obj,
                                                           gint            i);
static BatkStateSet*          bail_button_ref_state_set    (BatkObject       *obj);
static void                  bail_button_notify_label_btk (BObject         *obj,
                                                           BParamSpec      *pspec,
                                                           gpointer        data);
static void                  bail_button_label_map_btk    (BtkWidget       *widget,
                                                           gpointer        data);

static void                  bail_button_real_initialize  (BatkObject       *obj,
                                                           gpointer        data);
static void                  bail_button_finalize         (BObject        *object);
static void                  bail_button_init_textutil    (BailButton     *button,
                                                           BtkWidget      *label);

static void                  bail_button_pressed_enter_handler  (BtkWidget       *widget);
static void                  bail_button_released_leave_handler (BtkWidget       *widget);
static gint                  bail_button_real_add_btk           (BtkContainer    *container,
                                                                 BtkWidget       *widget,
                                                                 gpointer        data);


static void                  batk_action_interface_init  (BatkActionIface *iface);
static gboolean              bail_button_do_action      (BatkAction      *action,
                                                         gint           i);
static gboolean              idle_do_action             (gpointer       data);
static gint                  bail_button_get_n_actions  (BatkAction      *action);
static const gchar*          bail_button_get_description(BatkAction      *action,
                                                         gint           i);
static const gchar*          bail_button_get_keybinding (BatkAction      *action,
                                                         gint           i);
static const gchar*          bail_button_action_get_name(BatkAction      *action,
                                                         gint           i);
static gboolean              bail_button_set_description(BatkAction      *action,
                                                         gint           i,
                                                         const gchar    *desc);
static void                  bail_button_notify_label_weak_ref (gpointer data,
                                                                BObject  *obj);
static void                  bail_button_notify_weak_ref       (gpointer data,
                                                                BObject  *obj);


/* BatkImage.h */
static void                  batk_image_interface_init   (BatkImageIface  *iface);
static const gchar*          bail_button_get_image_description
                                                        (BatkImage       *image);
static void	             bail_button_get_image_position
                                                        (BatkImage       *image,
                                                         gint	        *x,
                                                         gint	        *y,
                                                         BatkCoordType   coord_type);
static void                  bail_button_get_image_size (BatkImage       *image,
                                                         gint           *width,
                                                         gint           *height);
static gboolean              bail_button_set_image_description 
                                                        (BatkImage       *image,
                                                         const gchar    *description);

/* batktext.h */ 
static void	  batk_text_interface_init	   (BatkTextIface	*iface);

static gchar*	  bail_button_get_text		   (BatkText	      *text,
                                                    gint	      start_pos,
						    gint	      end_pos);
static gunichar	  bail_button_get_character_at_offset(BatkText	      *text,
						    gint	      offset);
static gchar*     bail_button_get_text_before_offset(BatkText	      *text,
 						    gint	      offset,
						    BatkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gchar*     bail_button_get_text_at_offset   (BatkText	      *text,
 						    gint	      offset,
						    BatkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gchar*     bail_button_get_text_after_offset(BatkText	      *text,
 						    gint	      offset,
						    BatkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gint	  bail_button_get_character_count  (BatkText	      *text);
static void bail_button_get_character_extents      (BatkText	      *text,
						    gint 	      offset,
		                                    gint 	      *x,
                    		   	            gint 	      *y,
                                		    gint 	      *width,
                                     		    gint 	      *height,
			        		    BatkCoordType      coords);
static gint bail_button_get_offset_at_point        (BatkText           *text,
                                                    gint              x,
                                                    gint              y,
			                            BatkCoordType      coords);
static BatkAttributeSet* bail_button_get_run_attributes 
                                                   (BatkText           *text,
              					    gint 	      offset,
                                                    gint 	      *start_offset,
					            gint	      *end_offset);
static BatkAttributeSet* bail_button_get_default_attributes
                                                   (BatkText           *text);
static BtkImage*             get_image_from_button      (BtkWidget      *button);
static BtkWidget*            get_label_from_button      (BtkWidget      *button,
                                                         gint           index,
                                                         gboolean       allow_many);
static gint                  get_n_labels_from_button   (BtkWidget      *button);
static void                  set_role_for_button        (BatkObject      *accessible,
                                                         BtkWidget      *button);

static gint                  get_n_attached_menus       (BtkWidget      *widget);
static BtkWidget*            get_nth_attached_menu      (BtkWidget      *widget,
                                                         gint           index);

G_DEFINE_TYPE_WITH_CODE (BailButton, bail_button, BAIL_TYPE_CONTAINER,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_ACTION, batk_action_interface_init)
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_IMAGE, batk_image_interface_init)
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_TEXT, batk_text_interface_init))

static void
bail_button_class_init (BailButtonClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);
  BailContainerClass *container_class;

  container_class = (BailContainerClass*)klass;

  bobject_class->finalize = bail_button_finalize;

  class->get_name = bail_button_get_name;
  class->get_n_children = bail_button_get_n_children;
  class->ref_child = bail_button_ref_child;
  class->ref_state_set = bail_button_ref_state_set;
  class->initialize = bail_button_real_initialize;

  container_class->add_btk = bail_button_real_add_btk;
  container_class->remove_btk = NULL;
}

static void
bail_button_init (BailButton *button)
{
  button->click_description = NULL;
  button->press_description = NULL;
  button->release_description = NULL;
  button->click_keybinding = NULL;
  button->action_queue = NULL;
  button->action_idle_handler = 0;
  button->textutil = NULL;
}

static const gchar*
bail_button_get_name (BatkObject *obj)
{
  const gchar* name = NULL;

  g_return_val_if_fail (BAIL_IS_BUTTON (obj), NULL);

  name = BATK_OBJECT_CLASS (bail_button_parent_class)->get_name (obj);
  if (name == NULL)
    {
      /*
       * Get the text on the label
       */
      BtkWidget *widget;
      BtkWidget *child;

      widget = BTK_ACCESSIBLE (obj)->widget;
      if (widget == NULL)
        /*
         * State is defunct
         */
        return NULL;

      g_return_val_if_fail (BTK_IS_BUTTON (widget), NULL);

      child = get_label_from_button (widget, 0, FALSE);
      if (BTK_IS_LABEL (child))
        name = btk_label_get_text (BTK_LABEL (child)); 
      else
        {
          BtkImage *image;

          image = get_image_from_button (widget);
          if (BTK_IS_IMAGE (image))
            {
              BatkObject *batk_obj;

              batk_obj = btk_widget_get_accessible (BTK_WIDGET (image));
              name = batk_object_get_name (batk_obj);
            }
        }
    }
  return name;
}

/*
 * A DownArrow in a BtkToggltButton whose parent is not a ColorCombo
 * has press as default action.
 */
static gboolean
bail_button_is_default_press (BtkWidget *widget)
{
  BtkWidget  *child;
  BtkWidget  *parent;
  gboolean ret = FALSE;
  const gchar *parent_type_name;

  child = BTK_BIN (widget)->child;
  if (BTK_IS_ARROW (child) &&
      BTK_ARROW (child)->arrow_type == BTK_ARROW_DOWN)
    {
      parent = btk_widget_get_parent (widget);
      if (parent)
        {
          parent_type_name = g_type_name (B_OBJECT_TYPE (parent));
          if (strcmp (parent_type_name, "ColorCombo"))
            return TRUE;
        }
    }

  return ret;
}

static void
bail_button_real_initialize (BatkObject *obj,
                             gpointer   data)
{
  BailButton *button = BAIL_BUTTON (obj);
  BtkWidget  *label;
  BtkWidget  *widget;

  BATK_OBJECT_CLASS (bail_button_parent_class)->initialize (obj, data);

  button->state = BTK_STATE_NORMAL;

  g_signal_connect (data,
                    "pressed",
                    G_CALLBACK (bail_button_pressed_enter_handler),
                    NULL);
  g_signal_connect (data,
                    "enter",
                    G_CALLBACK (bail_button_pressed_enter_handler),
                    NULL);
  g_signal_connect (data,
                    "released",
                    G_CALLBACK (bail_button_released_leave_handler),
                    NULL);
  g_signal_connect (data,
                    "leave",
                    G_CALLBACK (bail_button_released_leave_handler),
                    NULL);


  widget = BTK_WIDGET (data);
  label = get_label_from_button (widget, 0, FALSE);
  if (BTK_IS_LABEL (label))
    {
      if (btk_widget_get_mapped (label))
        bail_button_init_textutil (button, label);
      else 
        g_signal_connect (label,
                          "map",
                          G_CALLBACK (bail_button_label_map_btk),
                          button);
    }
  button->default_is_press = bail_button_is_default_press (widget);
    
  set_role_for_button (obj, data);
}

static void
bail_button_label_map_btk (BtkWidget *widget,
                           gpointer data)
{
  BailButton *button; 

  button = BAIL_BUTTON (data);
  bail_button_init_textutil (button, widget);
}

static void
bail_button_notify_label_btk (BObject           *obj,
                              BParamSpec        *pspec,
                              gpointer           data)
{
  BatkObject* batk_obj = BATK_OBJECT (data);
  BtkLabel *label;
  BailButton *bail_button;

  if (strcmp (pspec->name, "label") == 0)
    {
      const gchar* label_text;

      label = BTK_LABEL (obj);

      label_text = btk_label_get_text (label);

      bail_button = BAIL_BUTTON (batk_obj);
      bail_text_util_text_setup (bail_button->textutil, label_text);

      if (batk_obj->name == NULL)
      {
        /*
         * The label has changed so notify a change in accessible-name
         */
        g_object_notify (B_OBJECT (batk_obj), "accessible-name");
      }
      /*
       * The label is the only property which can be changed
       */
      g_signal_emit_by_name (batk_obj, "visible_data_changed");
    }
}

static void
bail_button_notify_weak_ref (gpointer data, BObject* obj)
{
  BtkLabel *label = NULL;

  BatkObject* batk_obj = BATK_OBJECT (obj);
  if (data && BTK_IS_WIDGET (data))
    {
      label = BTK_LABEL (data);
      if (label)
        {
          g_signal_handlers_disconnect_by_func (label,
                                                (GCallback) bail_button_notify_label_btk,
                                                BAIL_BUTTON (batk_obj));
          g_object_weak_unref (B_OBJECT (label),
                               bail_button_notify_label_weak_ref,
                               BAIL_BUTTON (batk_obj));
        }
    }
}

static void
bail_button_notify_label_weak_ref (gpointer data, BObject* obj)
{
  BtkLabel *label = NULL;
  BailButton *button = NULL;

  label = BTK_LABEL (obj);
  if (data && BAIL_IS_BUTTON (data))
    {
      button = BAIL_BUTTON (BATK_OBJECT (data));
      if (button)
        g_object_weak_unref (B_OBJECT (button), bail_button_notify_weak_ref,
                             label);
    }
}


static void
bail_button_init_textutil (BailButton  *button,
                           BtkWidget   *label)
{
  const gchar *label_text;

  if (button->textutil)
    g_object_unref (button->textutil);
  button->textutil = bail_text_util_new ();
  label_text = btk_label_get_text (BTK_LABEL (label));
  bail_text_util_text_setup (button->textutil, label_text);
  g_object_weak_ref (B_OBJECT (button),
                     bail_button_notify_weak_ref, label);
  g_object_weak_ref (B_OBJECT (label),
                     bail_button_notify_label_weak_ref, button);
  g_signal_connect (label,
                    "notify",
                    (GCallback) bail_button_notify_label_btk,
                    button);     
}

static gint
bail_button_real_add_btk (BtkContainer *container,
                          BtkWidget    *widget,
                          gpointer     data)
{
  BtkLabel *label;
  BailButton *button;

  if (BTK_IS_LABEL (widget))
    {
      const gchar* label_text;

      label = BTK_LABEL (widget);


      button = BAIL_BUTTON (data);
      if (!button->textutil)
        bail_button_init_textutil (button, widget);
      else
        {
          label_text = btk_label_get_text (label);
          bail_text_util_text_setup (button->textutil, label_text);
        }
    }

  return 1;
}

static void
batk_action_interface_init (BatkActionIface *iface)
{
  iface->do_action = bail_button_do_action;
  iface->get_n_actions = bail_button_get_n_actions;
  iface->get_description = bail_button_get_description;
  iface->get_keybinding = bail_button_get_keybinding;
  iface->get_name = bail_button_action_get_name;
  iface->set_description = bail_button_set_description;
}

static gboolean
bail_button_do_action (BatkAction *action,
                       gint      i)
{
  BtkWidget *widget;
  BailButton *button;
  gboolean return_value = TRUE;

  widget = BTK_ACCESSIBLE (action)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  if (!btk_widget_is_sensitive (widget) || !btk_widget_get_visible (widget))
    return FALSE;

  button = BAIL_BUTTON (action); 

  switch (i)
    {
    case 0:
    case 1:
    case 2:
      if (!button->action_queue) 
	{
	  button->action_queue = g_queue_new ();
	}
      g_queue_push_head (button->action_queue, GINT_TO_POINTER(i));
      if (!button->action_idle_handler)
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
  BailButton *bail_button;
  BdkEvent tmp_event;

  bail_button = BAIL_BUTTON (data);
  bail_button->action_idle_handler = 0;
  widget = BTK_ACCESSIBLE (bail_button)->widget;

  g_object_ref (bail_button);

  if (widget == NULL /* State is defunct */ ||
      !btk_widget_is_sensitive (widget) || !btk_widget_get_visible (widget))
    {
      g_object_unref (bail_button);
      return FALSE;
    }
  else
    {
      tmp_event.button.type = BDK_BUTTON_RELEASE;
      tmp_event.button.window = widget->window;
      tmp_event.button.button = 1;
      tmp_event.button.send_event = TRUE;
      tmp_event.button.time = BDK_CURRENT_TIME;
      tmp_event.button.axes = NULL;
      btk_widget_event (widget, &tmp_event);
    }

  button = BTK_BUTTON (widget); 
  while (!g_queue_is_empty (bail_button->action_queue)) 
    {
      gint action_number = GPOINTER_TO_INT(g_queue_pop_head (bail_button->action_queue));
      if (bail_button->default_is_press)
        {
          if (action_number == 0)
            action_number = 1;
          else if (action_number == 1)
            action_number = 0;
        }
      switch (action_number)
	{
	case 0:
	  /* first a press */ 

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

	  /* then a release */
	  tmp_event.button.type = BDK_BUTTON_RELEASE;
	  btk_widget_event (widget, &tmp_event);
	  button->in_button = FALSE;
	  g_signal_emit_by_name (button, "leave");
	  break;
	case 1:
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
	  break;
	case 2:
	  button->in_button = FALSE;
	  g_signal_emit_by_name (button, "leave");
	  break;
	default:
	  g_assert_not_reached ();
	  break;
	}
    }
  g_object_unref (bail_button);
  return FALSE;
}

static gint
bail_button_get_n_actions (BatkAction *action)
{
  return 3;
}

static const gchar*
bail_button_get_description (BatkAction *action,
                             gint      i)
{
  BailButton *button;
  const gchar *return_value;

  button = BAIL_BUTTON (action);

  if (button->default_is_press)
    {
      if (i == 0)
        i = 1;
      else if (i == 1)
        i = 0;
    }
  switch (i)
    {
    case 0:
      return_value = button->click_description;
      break;
    case 1:
      return_value = button->press_description;
      break;
    case 2:
      return_value = button->release_description;
      break;
    default:
      return_value = NULL;
      break;
    }
  return return_value; 
}

static const gchar*
bail_button_get_keybinding (BatkAction *action,
                            gint      i)
{
  BailButton *button;
  gchar *return_value = NULL;

  button = BAIL_BUTTON (action);
  if (button->default_is_press)
    {
      if (i == 0)
        i = 1;
      else if (i == 1)
        i = 0;
    }
  switch (i)
    {
    case 0:
      {
        /*
         * We look for a mnemonic on the label
         */
        BtkWidget *widget;
        BtkWidget *label;
        guint key_val; 

        widget = BTK_ACCESSIBLE (button)->widget;
        if (widget == NULL)
          /*
           * State is defunct
           */
          return NULL;

        g_return_val_if_fail (BTK_IS_BUTTON (widget), NULL);

        label = get_label_from_button (widget, 0, FALSE);
        if (BTK_IS_LABEL (label))
          {
            key_val = btk_label_get_mnemonic_keyval (BTK_LABEL (label)); 
            if (key_val != BDK_VoidSymbol)
              return_value = btk_accelerator_name (key_val, BDK_MOD1_MASK);
          }
        if (return_value == NULL)
          {
            /* Find labelled-by relation */
            BatkRelationSet *set;
            BatkRelation *relation;
            GPtrArray *target;
            gpointer target_object;

            set = batk_object_ref_relation_set (BATK_OBJECT (action));
            if (set)
              {
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
              }

            if (BTK_IS_LABEL (label))
              {
                key_val = btk_label_get_mnemonic_keyval (BTK_LABEL (label)); 
                if (key_val != BDK_VoidSymbol)
                  return_value = btk_accelerator_name (key_val, BDK_MOD1_MASK);
              }
          }
        g_free (button->click_keybinding);
        button->click_keybinding = return_value;
        break;
      }
    default:
      break;
    }
  return return_value; 
}

static const gchar*
bail_button_action_get_name (BatkAction *action,
                             gint      i)
{
  const gchar *return_value;
  BailButton *button;

  button = BAIL_BUTTON (action);

  if (button->default_is_press)
    {
      if (i == 0)
        i = 1;
      else if (i == 1)
        i = 0;
    }
  switch (i)
    {
    case 0:
      /*
       * This action is a "click" to activate a button or "toggle" to change
       * the state of a toggle button check box or radio button.
       */ 
      return_value = "click";
      break;
    case 1:
      /*
       * This action simulates a button press by simulating moving the
       * mouse into the button followed by pressing the left mouse button.
       */
      return_value = "press";
      break;
    case 2:
      /*
       * This action simulates releasing the left mouse button outside the 
       * button.
       *
       * To simulate releasing the left mouse button inside the button use
       * the click action.
       */
      return_value = "release";
      break;
    default:
      return_value = NULL;
      break;
    }
  return return_value; 
}

static gboolean
bail_button_set_description (BatkAction      *action,
                             gint           i,
                             const gchar    *desc)
{
  BailButton *button;
  gchar **value;

  button = BAIL_BUTTON (action);

  if (button->default_is_press)
    {
      if (i == 0)
        i = 1;
      else if (i == 1)
        i = 0;
    }
  switch (i)
    {
    case 0:
      value = &button->click_description;
      break;
    case 1:
      value = &button->press_description;
      break;
    case 2:
      value = &button->release_description;
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

static gint
bail_button_get_n_children (BatkObject* obj)
{
  BtkWidget *widget;
  gint n_children;

  g_return_val_if_fail (BAIL_IS_BUTTON (obj), 0);

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;

  /*
   * Check whether we have an attached menus for PanelMenuButton
   */
  n_children = get_n_attached_menus (widget);
  if (n_children > 0)
    return n_children;

  n_children = get_n_labels_from_button (widget);
  if (n_children <= 1)
    n_children = 0;

  return n_children;
}

static BatkObject*
bail_button_ref_child (BatkObject *obj,
                       gint      i)
{
  BtkWidget *widget;
  BtkWidget *child_widget;
  BatkObject *child;

  g_return_val_if_fail (BAIL_IS_BUTTON (obj), NULL);

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  if (i >= bail_button_get_n_children (obj))
    return NULL;

  if (get_n_attached_menus (widget) > 0)
  {
    child_widget = get_nth_attached_menu (widget, i);
  }
  else
    child_widget = NULL;    

  if (!child_widget) 
    {
      if (get_n_labels_from_button (widget) > 1)
        {
          child_widget = get_label_from_button (widget, i, TRUE);
        }
    }

  if (child_widget)
    {
      child = btk_widget_get_accessible (child_widget);
      g_object_ref (child);
    }
  else
    child = NULL;

  return child;
}

static BatkStateSet*
bail_button_ref_state_set (BatkObject *obj)
{
  BatkStateSet *state_set;
  BtkWidget *widget;

  state_set = BATK_OBJECT_CLASS (bail_button_parent_class)->ref_state_set (obj);
  widget = BTK_ACCESSIBLE (obj)->widget;

  if (widget == NULL)
    return state_set;

  if (btk_widget_get_state (widget) == BTK_STATE_ACTIVE)
    batk_state_set_add_state (state_set, BATK_STATE_ARMED);

  if (!btk_widget_get_can_focus (widget))
    batk_state_set_remove_state (state_set, BATK_STATE_SELECTABLE);


  return state_set;
}

/*
 * This is the signal handler for the "pressed" or "enter" signal handler
 * on the BtkButton.
 *
 * If the state is now BTK_STATE_ACTIVE we notify a property change
 */
static void
bail_button_pressed_enter_handler (BtkWidget       *widget)
{
  BatkObject *accessible;

  if (btk_widget_get_state (widget) == BTK_STATE_ACTIVE)
    {
      accessible = btk_widget_get_accessible (widget);
      batk_object_notify_state_change (accessible, BATK_STATE_ARMED, TRUE);
      BAIL_BUTTON (accessible)->state = BTK_STATE_ACTIVE;
    }
}

/*
 * This is the signal handler for the "released" or "leave" signal handler
 * on the BtkButton.
 *
 * If the state was BTK_STATE_ACTIVE we notify a property change
 */
static void
bail_button_released_leave_handler (BtkWidget       *widget)
{
  BatkObject *accessible;

  accessible = btk_widget_get_accessible (widget);
  if (BAIL_BUTTON (accessible)->state == BTK_STATE_ACTIVE)
    {
      batk_object_notify_state_change (accessible, BATK_STATE_ARMED, FALSE);
      BAIL_BUTTON (accessible)->state = BTK_STATE_NORMAL;
    }
}

static void
batk_image_interface_init (BatkImageIface *iface)
{
  iface->get_image_description = bail_button_get_image_description;
  iface->get_image_position = bail_button_get_image_position;
  iface->get_image_size = bail_button_get_image_size;
  iface->set_image_description = bail_button_set_image_description;
}

static BtkImage*
get_image_from_button (BtkWidget *button)
{
  BtkWidget *child;
  GList *list;
  BtkImage *image = NULL;

  child = btk_bin_get_child (BTK_BIN (button));
  if (BTK_IS_IMAGE (child))
    image = BTK_IMAGE (child);
  else
    {
      if (BTK_IS_ALIGNMENT (child))
        child = btk_bin_get_child (BTK_BIN (child));
      if (BTK_IS_CONTAINER (child))
        {
          list = btk_container_get_children (BTK_CONTAINER (child));
          if (!list)
            return NULL;
          if (BTK_IS_IMAGE (list->data))
            image = BTK_IMAGE (list->data);
          g_list_free (list);
        }
    }

  return image;
}

static const gchar*
bail_button_get_image_description (BatkImage *image) {

  BtkWidget *widget;
  BtkImage  *button_image;
  BatkObject *obj;

  widget = BTK_ACCESSIBLE (image)->widget;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  button_image = get_image_from_button (widget);

  if (button_image != NULL)
    {
      obj = btk_widget_get_accessible (BTK_WIDGET (button_image));
      return batk_image_get_image_description (BATK_IMAGE (obj));
    }
  else 
    return NULL;
}

static void
bail_button_get_image_position (BatkImage     *image,
                                gint	     *x,
                                gint	     *y,
                                BatkCoordType coord_type)
{
  BtkWidget *widget;
  BtkImage  *button_image;
  BatkObject *obj;

  widget = BTK_ACCESSIBLE (image)->widget;

  if (widget == NULL)
    {
    /*
     * State is defunct
     */
      *x = G_MININT;
      *y = G_MININT;
      return;
    }

  button_image = get_image_from_button (widget);

  if (button_image != NULL)
    {
      obj = btk_widget_get_accessible (BTK_WIDGET (button_image));
      batk_component_get_position (BATK_COMPONENT (obj), x, y, coord_type); 
    }
  else
    {
      *x = G_MININT;
      *y = G_MININT;
    }
}

static void
bail_button_get_image_size (BatkImage *image,
                            gint     *width,
                            gint     *height)
{
  BtkWidget *widget;
  BtkImage  *button_image;
  BatkObject *obj;

  widget = BTK_ACCESSIBLE (image)->widget;

  if (widget == NULL)
    {
    /*
     * State is defunct
     */
      *width = -1;
      *height = -1;
      return;
    }

  button_image = get_image_from_button (widget);

  if (button_image != NULL)
    {
      obj = btk_widget_get_accessible (BTK_WIDGET (button_image));
      batk_image_get_image_size (BATK_IMAGE (obj), width, height); 
    }
  else
    {
      *width = -1;
      *height = -1;
    }
}

static gboolean
bail_button_set_image_description (BatkImage    *image,
                                   const gchar *description)
{
  BtkWidget *widget;
  BtkImage  *button_image;
  BatkObject *obj;

  widget = BTK_ACCESSIBLE (image)->widget;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  button_image = get_image_from_button (widget);

  if (button_image != NULL) 
    {
      obj = btk_widget_get_accessible (BTK_WIDGET (button_image));
      return batk_image_set_image_description (BATK_IMAGE (obj), description);
    }
  else 
    return FALSE;
}

/* batktext.h */

static void
batk_text_interface_init (BatkTextIface *iface)
{
  iface->get_text = bail_button_get_text;
  iface->get_character_at_offset = bail_button_get_character_at_offset;
  iface->get_text_before_offset = bail_button_get_text_before_offset;
  iface->get_text_at_offset = bail_button_get_text_at_offset;
  iface->get_text_after_offset = bail_button_get_text_after_offset;
  iface->get_character_count = bail_button_get_character_count;
  iface->get_character_extents = bail_button_get_character_extents;
  iface->get_offset_at_point = bail_button_get_offset_at_point;
  iface->get_run_attributes = bail_button_get_run_attributes;
  iface->get_default_attributes = bail_button_get_default_attributes;
}

static gchar*
bail_button_get_text (BatkText *text,
                      gint    start_pos,
                      gint    end_pos)
{
  BtkWidget *widget;
  BtkWidget  *label;
  BailButton *button;
  const gchar *label_text;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  label = get_label_from_button (widget, 0, FALSE);

  if (!BTK_IS_LABEL (label))
    return NULL;

  button = BAIL_BUTTON (text);
  if (!button->textutil) 
    bail_button_init_textutil (button, label);

  label_text = btk_label_get_text (BTK_LABEL (label));

  if (label_text == NULL)
    return NULL;
  else
  {
    return bail_text_util_get_substring (button->textutil, 
                                         start_pos, end_pos);
  }
}

static gchar*
bail_button_get_text_before_offset (BatkText         *text,
				    gint            offset,
				    BatkTextBoundary boundary_type,
				    gint            *start_offset,
				    gint            *end_offset)
{
  BtkWidget *widget;
  BtkWidget *label;
  BailButton *button;
  
  widget = BTK_ACCESSIBLE (text)->widget;
  
  if (widget == NULL)
    /* State is defunct */
    return NULL;
  
  /* Get label */
  label = get_label_from_button (widget, 0, FALSE);

  if (!BTK_IS_LABEL(label))
    return NULL;

  button = BAIL_BUTTON (text);
  if (!button->textutil)
    bail_button_init_textutil (button, label);

  return bail_text_util_get_text (button->textutil,
                           btk_label_get_layout (BTK_LABEL (label)), BAIL_BEFORE_OFFSET, 
                           boundary_type, offset, start_offset, end_offset); 
}

static gchar*
bail_button_get_text_at_offset (BatkText         *text,
			        gint            offset,
			        BatkTextBoundary boundary_type,
 			        gint            *start_offset,
			        gint            *end_offset)
{
  BtkWidget *widget;
  BtkWidget *label;
  BailButton *button;
 
  widget = BTK_ACCESSIBLE (text)->widget;
  
  if (widget == NULL)
    /* State is defunct */
    return NULL;
  
  /* Get label */
  label = get_label_from_button (widget, 0, FALSE);

  if (!BTK_IS_LABEL(label))
    return NULL;

  button = BAIL_BUTTON (text);
  if (!button->textutil)
    bail_button_init_textutil (button, label);

  return bail_text_util_get_text (button->textutil,
                              btk_label_get_layout (BTK_LABEL (label)), BAIL_AT_OFFSET, 
                              boundary_type, offset, start_offset, end_offset);
}

static gchar*
bail_button_get_text_after_offset (BatkText         *text,
				   gint            offset,
				   BatkTextBoundary boundary_type,
				   gint            *start_offset,
				   gint            *end_offset)
{
  BtkWidget *widget;
  BtkWidget *label;
  BailButton *button;

  widget = BTK_ACCESSIBLE (text)->widget;
  
  if (widget == NULL)
  {
    /* State is defunct */
    return NULL;
  }
  
  /* Get label */
  label = get_label_from_button (widget, 0, FALSE);

  if (!BTK_IS_LABEL(label))
    return NULL;

  button = BAIL_BUTTON (text);
  if (!button->textutil)
    bail_button_init_textutil (button, label);

  return bail_text_util_get_text (button->textutil,
                           btk_label_get_layout (BTK_LABEL (label)), BAIL_AFTER_OFFSET, 
                           boundary_type, offset, start_offset, end_offset);
}

static gint
bail_button_get_character_count (BatkText *text)
{
  BtkWidget *widget;
  BtkWidget *label;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  label = get_label_from_button (widget, 0, FALSE);

  if (!BTK_IS_LABEL(label))
    return 0;

  return g_utf8_strlen (btk_label_get_text (BTK_LABEL (label)), -1);
}

static void
bail_button_get_character_extents (BatkText      *text,
				   gint         offset,
		                   gint         *x,
                    		   gint 	*y,
                                   gint 	*width,
                                   gint 	*height,
			           BatkCoordType coords)
{
  BtkWidget *widget;
  BtkWidget *label;
  BangoRectangle char_rect;
  gint index, x_layout, y_layout;
  const gchar *label_text;
 
  widget = BTK_ACCESSIBLE (text)->widget;

  if (widget == NULL)
    /* State is defunct */
    return;

  label = get_label_from_button (widget, 0, FALSE);

  if (!BTK_IS_LABEL(label))
    return;
  
  btk_label_get_layout_offsets (BTK_LABEL (label), &x_layout, &y_layout);
  label_text = btk_label_get_text (BTK_LABEL (label));
  index = g_utf8_offset_to_pointer (label_text, offset) - label_text;
  bango_layout_index_to_pos (btk_label_get_layout (BTK_LABEL (label)), index, &char_rect);
  
  bail_misc_get_extents_from_bango_rectangle (label, &char_rect, 
                    x_layout, y_layout, x, y, width, height, coords);
} 

static gint 
bail_button_get_offset_at_point (BatkText      *text,
                                 gint         x,
                                 gint         y,
			         BatkCoordType coords)
{ 
  BtkWidget *widget;
  BtkWidget *label;
  gint index, x_layout, y_layout;
  const gchar *label_text;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return -1;
  label = get_label_from_button (widget, 0, FALSE);

  if (!BTK_IS_LABEL(label))
    return -1;
  
  btk_label_get_layout_offsets (BTK_LABEL (label), &x_layout, &y_layout);
  
  index = bail_misc_get_index_at_point_in_layout (label, 
                                              btk_label_get_layout (BTK_LABEL (label)), 
                                              x_layout, y_layout, x, y, coords);
  label_text = btk_label_get_text (BTK_LABEL (label));
  if (index == -1)
    {
      if (coords == BATK_XY_WINDOW || coords == BATK_XY_SCREEN)
        return g_utf8_strlen (label_text, -1);

      return index;  
    }
  else
    return g_utf8_pointer_to_offset (label_text, label_text + index);  
}

static BatkAttributeSet*
bail_button_get_run_attributes (BatkText        *text,
                                gint 	      offset,
                                gint 	      *start_offset,
	                        gint	      *end_offset)
{
  BtkWidget *widget;
  BtkWidget *label;
  BatkAttributeSet *at_set = NULL;
  BtkJustification justify;
  BtkTextDirection dir;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  label = get_label_from_button (widget, 0, FALSE);

  if (!BTK_IS_LABEL(label))
    return NULL;
  
  /* Get values set for entire label, if any */
  justify = btk_label_get_justify (BTK_LABEL (label));
  if (justify != BTK_JUSTIFY_CENTER)
    {
      at_set = bail_misc_add_attribute (at_set, 
                                        BATK_TEXT_ATTR_JUSTIFICATION,
     g_strdup (batk_text_attribute_get_value (BATK_TEXT_ATTR_JUSTIFICATION, justify)));
    }
  dir = btk_widget_get_direction (label);
  if (dir == BTK_TEXT_DIR_RTL)
    {
      at_set = bail_misc_add_attribute (at_set, 
                                        BATK_TEXT_ATTR_DIRECTION,
     g_strdup (batk_text_attribute_get_value (BATK_TEXT_ATTR_DIRECTION, dir)));
    }

  at_set = bail_misc_layout_get_run_attributes (at_set,
                                                btk_label_get_layout (BTK_LABEL (label)),
                                                (gchar *) btk_label_get_text (BTK_LABEL (label)),
                                                offset,
                                                start_offset,
                                                end_offset);
  return at_set;
}

static BatkAttributeSet*
bail_button_get_default_attributes (BatkText        *text)
{
  BtkWidget *widget;
  BtkWidget *label;
  BatkAttributeSet *at_set = NULL;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  label = get_label_from_button (widget, 0, FALSE);

  if (!BTK_IS_LABEL(label))
    return NULL;

  at_set = bail_misc_get_default_attributes (at_set,
                                             btk_label_get_layout (BTK_LABEL (label)),
                                             widget);
  return at_set;
}

static gunichar 
bail_button_get_character_at_offset (BatkText	         *text,
                                     gint	         offset)
{
  BtkWidget *widget;
  BtkWidget *label;
  const gchar *string;
  gchar *index;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return '\0';

  label = get_label_from_button (widget, 0, FALSE);

  if (!BTK_IS_LABEL(label))
    return '\0';
  string = btk_label_get_text (BTK_LABEL (label));
  if (offset >= g_utf8_strlen (string, -1))
    return '\0';
  index = g_utf8_offset_to_pointer (string, offset);

  return g_utf8_get_char (index);
}

static void
bail_button_finalize (BObject            *object)
{
  BailButton *button = BAIL_BUTTON (object);

  g_free (button->click_description);
  g_free (button->press_description);
  g_free (button->release_description);
  g_free (button->click_keybinding);
  if (button->action_idle_handler)
    {
      g_source_remove (button->action_idle_handler);
      button->action_idle_handler = 0;
    }
  if (button->action_queue)
    {
      g_queue_free (button->action_queue);
    }
  if (button->textutil)
    {
      g_object_unref (button->textutil);
    }
  B_OBJECT_CLASS (bail_button_parent_class)->finalize (object);
}

static BtkWidget*
find_label_child (BtkContainer *container,
                  gint         *index,
                  gboolean     allow_many)
{
  GList *children, *tmp_list;
  BtkWidget *child;
 
  children = btk_container_get_children (container);

  child = NULL;
  for (tmp_list = children; tmp_list != NULL; tmp_list = tmp_list->next) 
    {
      if (BTK_IS_LABEL (tmp_list->data))
        {
          if (!allow_many)
            {
              if (child)
                {
                  child = NULL;
                  break;
                }
              child = BTK_WIDGET (tmp_list->data);
            }
          else
            {
              if (*index == 0)
                {
                  child = BTK_WIDGET (tmp_list->data);
                  break;
                }
              (*index)--;
	    }
        }
       /*
        * Label for button which are BtkTreeView column headers are in a 
        * BtkHBox in a BtkAlignment.
        */
      else if (BTK_IS_ALIGNMENT (tmp_list->data))
        {
          BtkWidget *widget;

          widget = btk_bin_get_child (BTK_BIN (tmp_list->data));
          if (BTK_IS_LABEL (widget))
            {
              if (!allow_many)
                {
                  if (child)
                    {
                      child = NULL;
                      break;
                    }
                  child = widget;
                }
              else
                {
                  if (*index == 0)
                    {
	              child = widget;
                      break;
                    }
                  (*index)--;
	        }
	    }
        }
      else if (BTK_IS_CONTAINER (tmp_list->data))
        {
          child = find_label_child (BTK_CONTAINER (tmp_list->data), index, allow_many);
          if (child)
            break;
        } 
    }
  g_list_free (children);
  return child;
}

static BtkWidget*
get_label_from_button (BtkWidget *button,
                       gint      index,
                       gboolean  allow_many)
{
  BtkWidget *child;

  if (index > 0 && !allow_many)
    g_warning ("Inconsistent values passed to get_label_from_button");

  child = btk_bin_get_child (BTK_BIN (button));
  if (BTK_IS_ALIGNMENT (child))
    child = btk_bin_get_child (BTK_BIN (child));

  if (BTK_IS_CONTAINER (child))
    child = find_label_child (BTK_CONTAINER (child), &index, allow_many);
  else if (!BTK_IS_LABEL (child))
    child = NULL;

  return child;
}

static void
count_labels (BtkContainer *container,
              gint         *n_labels)
{
  GList *children, *tmp_list;
 
  children = btk_container_get_children (container);

  for (tmp_list = children; tmp_list != NULL; tmp_list = tmp_list->next) 
    {
      if (BTK_IS_LABEL (tmp_list->data))
        {
          (*n_labels)++;
        }
       /*
        * Label for button which are BtkTreeView column headers are in a 
        * BtkHBox in a BtkAlignment.
        */
      else if (BTK_IS_ALIGNMENT (tmp_list->data))
        {
          BtkWidget *widget;

          widget = btk_bin_get_child (BTK_BIN (tmp_list->data));
          if (BTK_IS_LABEL (widget))
            (*n_labels)++;
        }
      else if (BTK_IS_CONTAINER (tmp_list->data))
        {
          count_labels (BTK_CONTAINER (tmp_list->data), n_labels);
        } 
    }
  g_list_free (children);
}

static gint
get_n_labels_from_button (BtkWidget *button)
{
  BtkWidget *child;
  gint n_labels;

  n_labels = 0;

  child = btk_bin_get_child (BTK_BIN (button));
  if (BTK_IS_ALIGNMENT (child))
    child = btk_bin_get_child (BTK_BIN (child));

  if (BTK_IS_CONTAINER (child))
    count_labels (BTK_CONTAINER (child), &n_labels);

  return n_labels;
}

static void
set_role_for_button (BatkObject *accessible,
                     BtkWidget *button)
{
  BtkWidget *parent;
  BatkRole role;

  parent = btk_widget_get_parent (button);
  if (BTK_IS_TREE_VIEW (parent))
    {
      role = BATK_ROLE_TABLE_COLUMN_HEADER;
      /*
       * Even though the accessible parent of the column header will
       * be reported as the table because the parent widget of the
       * BtkTreeViewColumn's button is the BtkTreeView we set
       * the accessible parent for column header to be the table
       * to ensure that batk_object_get_index_in_parent() returns
       * the correct value; see bail_widget_get_index_in_parent().
       */
      batk_object_set_parent (accessible, btk_widget_get_accessible (parent));
    }
  else
    role = BATK_ROLE_PUSH_BUTTON;

  accessible->role =  role;
}

static gint
get_n_attached_menus (BtkWidget  *widget)
{
  GList *list_menus;
  
  if (widget == NULL)
    return 0;

  list_menus = g_object_get_data (B_OBJECT (widget), BAIL_BUTTON_ATTACHED_MENUS);
  if (list_menus == NULL)
    return 0;

  return g_list_length (list_menus);
}

static BtkWidget*
get_nth_attached_menu (BtkWidget  *widget,
                       gint       index)
{
  BtkWidget *attached_menu;
  GList *list_menus;

  if (widget == NULL)
    return NULL;

  list_menus = g_object_get_data (B_OBJECT (widget), BAIL_BUTTON_ATTACHED_MENUS);
  if (list_menus == NULL ||
      index >= g_list_length (list_menus))
    return NULL;

  attached_menu = (BtkWidget *) g_list_nth_data (list_menus, index);

  return attached_menu;
}
