/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2003 Sun Microsystems Inc.
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
#include "bailexpander.h"
#include <libbail-util/bailmisc.h>

static void                  bail_expander_class_init       (BailExpanderClass *klass);
static void                  bail_expander_init             (BailExpander      *expander);

static const gchar*          bail_expander_get_name         (BatkObject         *obj);
static gint                  bail_expander_get_n_children   (BatkObject         *obj)
;
static BatkObject*            bail_expander_ref_child        (BatkObject         *obj,
                                                             gint              i);

static BatkStateSet*          bail_expander_ref_state_set    (BatkObject         *obj);
static void                  bail_expander_real_notify_btk  (BObject           *obj,
                                                             BParamSpec        *pspec);
static void                  bail_expander_map_btk          (BtkWidget         *widget,
                                                             gpointer          data);

static void                  bail_expander_real_initialize  (BatkObject         *obj,
                                                             gpointer          data);
static void                  bail_expander_finalize         (BObject           *object);
static void                  bail_expander_init_textutil    (BailExpander      *expander,
                                                             BtkExpander       *widget);
static const gchar*          bail_expander_get_full_text    (BtkExpander       *widget);

static void                  batk_action_interface_init  (BatkActionIface *iface);
static gboolean              bail_expander_do_action    (BatkAction      *action,
                                                         gint           i);
static gboolean              idle_do_action             (gpointer       data);
static gint                  bail_expander_get_n_actions(BatkAction      *action);
static const gchar*          bail_expander_get_description
                                                        (BatkAction      *action,
                                                         gint           i);
static const gchar*          bail_expander_get_keybinding
                                                        (BatkAction      *action,
                                                         gint           i);
static const gchar*          bail_expander_action_get_name
                                                        (BatkAction      *action,
                                                         gint           i);
static gboolean              bail_expander_set_description
                                                        (BatkAction      *action,
                                                         gint           i,
                                                         const gchar    *desc);

/* batktext.h */ 
static void	  batk_text_interface_init	   (BatkTextIface	*iface);

static gchar*	  bail_expander_get_text	   (BatkText	      *text,
                                                    gint	      start_pos,
						    gint	      end_pos);
static gunichar	  bail_expander_get_character_at_offset
                                                   (BatkText	      *text,
						    gint	      offset);
static gchar*     bail_expander_get_text_before_offset
                                                   (BatkText	      *text,
 						    gint	      offset,
						    BatkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gchar*     bail_expander_get_text_at_offset (BatkText	      *text,
 						    gint	      offset,
						    BatkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gchar*     bail_expander_get_text_after_offset
                                                   (BatkText	      *text,
 						    gint	      offset,
						    BatkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gint	  bail_expander_get_character_count(BatkText	      *text);
static void bail_expander_get_character_extents    (BatkText	      *text,
						    gint 	      offset,
		                                    gint 	      *x,
                    		   	            gint 	      *y,
                                		    gint 	      *width,
                                     		    gint 	      *height,
			        		    BatkCoordType      coords);
static gint bail_expander_get_offset_at_point      (BatkText           *text,
                                                    gint              x,
                                                    gint              y,
			                            BatkCoordType      coords);
static BatkAttributeSet* bail_expander_get_run_attributes 
                                                   (BatkText           *text,
              					    gint 	      offset,
                                                    gint 	      *start_offset,
					            gint	      *end_offset);
static BatkAttributeSet* bail_expander_get_default_attributes
                                                   (BatkText           *text);

G_DEFINE_TYPE_WITH_CODE (BailExpander, bail_expander, BAIL_TYPE_CONTAINER,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_ACTION, batk_action_interface_init)
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_TEXT, batk_text_interface_init))

static void
bail_expander_class_init (BailExpanderClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);
  BailWidgetClass *widget_class;

  widget_class = (BailWidgetClass*)klass;
  widget_class->notify_btk = bail_expander_real_notify_btk;

  bobject_class->finalize = bail_expander_finalize;

  class->get_name = bail_expander_get_name;
  class->get_n_children = bail_expander_get_n_children;
  class->ref_child = bail_expander_ref_child;
  class->ref_state_set = bail_expander_ref_state_set;

  class->initialize = bail_expander_real_initialize;
}

static void
bail_expander_init (BailExpander *expander)
{
  expander->activate_description = NULL;
  expander->activate_keybinding = NULL;
  expander->action_idle_handler = 0;
  expander->textutil = NULL;
}

static const gchar*
bail_expander_get_name (BatkObject *obj)
{
  const gchar *name;
  g_return_val_if_fail (BAIL_IS_EXPANDER (obj), NULL);

  name = BATK_OBJECT_CLASS (bail_expander_parent_class)->get_name (obj);
  if (name != NULL)
    return name;
  else
    {
      /*
       * Get the text on the label
       */
      BtkWidget *widget;

      widget = BTK_ACCESSIBLE (obj)->widget;
      if (widget == NULL)
        /*
         * State is defunct
         */
        return NULL;

      g_return_val_if_fail (BTK_IS_EXPANDER (widget), NULL);

      return bail_expander_get_full_text (BTK_EXPANDER (widget));
    }
}

static gint
bail_expander_get_n_children (BatkObject* obj)
{
  BtkWidget *widget;
  GList *children;
  gint count = 0;

  g_return_val_if_fail (BAIL_IS_CONTAINER (obj), count);

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    return 0;

  children = btk_container_get_children (BTK_CONTAINER(widget));
  count = g_list_length (children);
  g_list_free (children);

  /* See if there is a label - if there is, reduce our count by 1
   * since we don't want the label included with the children.
   */
  if (btk_expander_get_label_widget (BTK_EXPANDER (widget)))
    count -= 1;

  return count; 
}

static BatkObject*
bail_expander_ref_child (BatkObject *obj,
                         gint      i)
{
  GList *children, *tmp_list;
  BatkObject *accessible;
  BtkWidget *widget;
  BtkWidget *label;
  gint index;
  gint count;

  g_return_val_if_fail (BAIL_IS_CONTAINER (obj), NULL);
  g_return_val_if_fail ((i >= 0), NULL);
  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    return NULL;

  children = btk_container_get_children (BTK_CONTAINER (widget));

  /* See if there is a label - if there is, we need to skip it
   * since we don't want the label included with the children.
   */
  label = btk_expander_get_label_widget (BTK_EXPANDER (widget));
  if (label) {
    count = g_list_length (children);
    for (index = 0; index <= i; index++) {
      tmp_list = g_list_nth (children, index);
      if (label == BTK_WIDGET (tmp_list->data)) {
	i += 1;
	break;
      }
    }
  }

  tmp_list = g_list_nth (children, i);
  if (!tmp_list)
    {
      g_list_free (children);
      return NULL;
    }  
  accessible = btk_widget_get_accessible (BTK_WIDGET (tmp_list->data));

  g_list_free (children);
  g_object_ref (accessible);
  return accessible; 
}

static void
bail_expander_real_initialize (BatkObject *obj,
                               gpointer   data)
{
  BailExpander *bail_expander = BAIL_EXPANDER (obj);
  BtkWidget  *expander;

  BATK_OBJECT_CLASS (bail_expander_parent_class)->initialize (obj, data);

  expander = BTK_WIDGET (data);
  if (btk_widget_get_mapped (expander))
    bail_expander_init_textutil (bail_expander, BTK_EXPANDER (expander));
  else 
    g_signal_connect (expander,
                      "map",
                      G_CALLBACK (bail_expander_map_btk),
                      bail_expander);
 
  obj->role = BATK_ROLE_TOGGLE_BUTTON;
}

static void
bail_expander_map_btk (BtkWidget *widget,
                       gpointer data)
{
  BailExpander *expander; 

  expander = BAIL_EXPANDER (data);
  bail_expander_init_textutil (expander, BTK_EXPANDER (widget));
}

static void
bail_expander_real_notify_btk (BObject    *obj,
                               BParamSpec *pspec)
{
  BatkObject* batk_obj;
  BtkExpander *expander;
  BailExpander *bail_expander;

  expander = BTK_EXPANDER (obj);
  batk_obj = btk_widget_get_accessible (BTK_WIDGET (expander));
;
  if (strcmp (pspec->name, "label") == 0)
    {
      const gchar* label_text;


      label_text = bail_expander_get_full_text (expander);

      bail_expander = BAIL_EXPANDER (batk_obj);
      if (bail_expander->textutil)
        bail_text_util_text_setup (bail_expander->textutil, label_text);

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
  else if (strcmp (pspec->name, "expanded") == 0)
    {
      batk_object_notify_state_change (batk_obj, BATK_STATE_CHECKED, 
                                      btk_expander_get_expanded (expander));
      batk_object_notify_state_change (batk_obj, BATK_STATE_EXPANDED, 
                                      btk_expander_get_expanded (expander));
      g_signal_emit_by_name (batk_obj, "visible_data_changed");
    }
  else
    BAIL_WIDGET_CLASS (bail_expander_parent_class)->notify_btk (obj, pspec);
}

static const gchar*
bail_expander_get_full_text (BtkExpander *widget)
{
  BtkWidget *label_widget;

  label_widget = btk_expander_get_label_widget (widget);

  if (!BTK_IS_LABEL (label_widget))
    return NULL;

  return btk_label_get_text (BTK_LABEL (label_widget));
}

static void
bail_expander_init_textutil (BailExpander *expander,
                             BtkExpander  *widget)
{
  const gchar *label_text;

  expander->textutil = bail_text_util_new ();
  label_text = bail_expander_get_full_text (widget);
  bail_text_util_text_setup (expander->textutil, label_text);
}

static void
batk_action_interface_init (BatkActionIface *iface)
{
  iface->do_action = bail_expander_do_action;
  iface->get_n_actions = bail_expander_get_n_actions;
  iface->get_description = bail_expander_get_description;
  iface->get_keybinding = bail_expander_get_keybinding;
  iface->get_name = bail_expander_action_get_name;
  iface->set_description = bail_expander_set_description;
}

static gboolean
bail_expander_do_action (BatkAction *action,
                         gint      i)
{
  BtkWidget *widget;
  BailExpander *expander;
  gboolean return_value = TRUE;

  widget = BTK_ACCESSIBLE (action)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  if (!btk_widget_is_sensitive (widget) || !btk_widget_get_visible (widget))
    return FALSE;

  expander = BAIL_EXPANDER (action);
  switch (i)
    {
    case 0:
      if (expander->action_idle_handler)
        return_value = FALSE;
      else
	expander->action_idle_handler = bdk_threads_add_idle (idle_do_action, expander);
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
  BtkWidget *widget;
  BailExpander *bail_expander;

  bail_expander = BAIL_EXPANDER (data);
  bail_expander->action_idle_handler = 0;

  widget = BTK_ACCESSIBLE (bail_expander)->widget;
  if (widget == NULL /* State is defunct */ ||
      !btk_widget_is_sensitive (widget) || !btk_widget_get_visible (widget))
    return FALSE;

  btk_widget_activate (widget);

  return FALSE;
}

static gint
bail_expander_get_n_actions (BatkAction *action)
{
  return 1;
}

static const gchar*
bail_expander_get_description (BatkAction *action,
                               gint      i)
{
  BailExpander *expander;
  const gchar *return_value;

  expander = BAIL_EXPANDER (action);

  switch (i)
    {
    case 0:
      return_value = expander->activate_description;
      break;
    default:
      return_value = NULL;
      break;
    }
  return return_value; 
}

static const gchar*
bail_expander_get_keybinding (BatkAction *action,
                              gint      i)
{
  BailExpander *expander;
  gchar *return_value = NULL;

  switch (i)
    {
    case 0:
      {
        /*
         * We look for a mnemonic on the label
         */
        BtkWidget *widget;
        BtkWidget *label;

        expander = BAIL_EXPANDER (action);
        widget = BTK_ACCESSIBLE (expander)->widget;
        if (widget == NULL)
          /*
           * State is defunct
           */
          return NULL;

        g_return_val_if_fail (BTK_IS_EXPANDER (widget), NULL);

        label = btk_expander_get_label_widget (BTK_EXPANDER (widget));
        if (BTK_IS_LABEL (label))
          {
            guint key_val; 

            key_val = btk_label_get_mnemonic_keyval (BTK_LABEL (label)); 
            if (key_val != BDK_VoidSymbol)
              return_value = btk_accelerator_name (key_val, BDK_MOD1_MASK);
            g_free (expander->activate_keybinding);
            expander->activate_keybinding = return_value;
          }
        break;
      }
    default:
      break;
    }
  return return_value; 
}

static const gchar*
bail_expander_action_get_name (BatkAction *action,
                               gint      i)
{
  const gchar *return_value;

  switch (i)
    {
    case 0:
      return_value = "activate";
      break;
    default:
      return_value = NULL;
      break;
    }
  return return_value; 
}

static gboolean
bail_expander_set_description (BatkAction      *action,
                               gint           i,
                               const gchar    *desc)
{
  BailExpander *expander;
  gchar **value;

  expander = BAIL_EXPANDER (action);

  switch (i)
    {
    case 0:
      value = &expander->activate_description;
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

static BatkStateSet*
bail_expander_ref_state_set (BatkObject *obj)
{
  BatkStateSet *state_set;
  BtkWidget *widget;
  BtkExpander *expander;

  state_set = BATK_OBJECT_CLASS (bail_expander_parent_class)->ref_state_set (obj);
  widget = BTK_ACCESSIBLE (obj)->widget;

  if (widget == NULL)
    return state_set;

  expander = BTK_EXPANDER (widget);

  batk_state_set_add_state (state_set, BATK_STATE_EXPANDABLE);

  if (btk_expander_get_expanded (expander)) {
    batk_state_set_add_state (state_set, BATK_STATE_CHECKED);
    batk_state_set_add_state (state_set, BATK_STATE_EXPANDED);
  }

  return state_set;
}

/* batktext.h */

static void
batk_text_interface_init (BatkTextIface *iface)
{
  iface->get_text = bail_expander_get_text;
  iface->get_character_at_offset = bail_expander_get_character_at_offset;
  iface->get_text_before_offset = bail_expander_get_text_before_offset;
  iface->get_text_at_offset = bail_expander_get_text_at_offset;
  iface->get_text_after_offset = bail_expander_get_text_after_offset;
  iface->get_character_count = bail_expander_get_character_count;
  iface->get_character_extents = bail_expander_get_character_extents;
  iface->get_offset_at_point = bail_expander_get_offset_at_point;
  iface->get_run_attributes = bail_expander_get_run_attributes;
  iface->get_default_attributes = bail_expander_get_default_attributes;
}

static gchar*
bail_expander_get_text (BatkText *text,
                        gint    start_pos,
                        gint    end_pos)
{
  BtkWidget *widget;
  BailExpander *expander;
  const gchar *label_text;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  expander = BAIL_EXPANDER (text);
  if (!expander->textutil) 
    bail_expander_init_textutil (expander, BTK_EXPANDER (widget));

  label_text = bail_expander_get_full_text (BTK_EXPANDER (widget));

  if (label_text == NULL)
    return NULL;
  else
    return bail_text_util_get_substring (expander->textutil, 
                                         start_pos, end_pos);
}

static gchar*
bail_expander_get_text_before_offset (BatkText         *text,
				      gint            offset,
				      BatkTextBoundary boundary_type,
				      gint            *start_offset,
				      gint            *end_offset)
{
  BtkWidget *widget;
  BailExpander *expander;
  BtkWidget *label;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  expander = BAIL_EXPANDER (text);
  if (!expander->textutil) 
    bail_expander_init_textutil (expander, BTK_EXPANDER (widget));

  label = btk_expander_get_label_widget (BTK_EXPANDER (widget));
  if (!BTK_IS_LABEL(label))
    return NULL;
  return bail_text_util_get_text (expander->textutil,
                           btk_label_get_layout (BTK_LABEL (label)),
                           BAIL_BEFORE_OFFSET, 
                           boundary_type, offset, start_offset, end_offset); 
}

static gchar*
bail_expander_get_text_at_offset (BatkText         *text,
			          gint            offset,
			          BatkTextBoundary boundary_type,
 			          gint            *start_offset,
			          gint            *end_offset)
{
  BtkWidget *widget;
  BailExpander *expander;
  BtkWidget *label;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  expander = BAIL_EXPANDER (text);
  if (!expander->textutil) 
    bail_expander_init_textutil (expander, BTK_EXPANDER (widget));

  label = btk_expander_get_label_widget (BTK_EXPANDER (widget));
  if (!BTK_IS_LABEL(label))
    return NULL;
  return bail_text_util_get_text (expander->textutil,
                           btk_label_get_layout (BTK_LABEL (label)),
                           BAIL_AT_OFFSET, 
                           boundary_type, offset, start_offset, end_offset);
}

static gchar*
bail_expander_get_text_after_offset (BatkText         *text,
				     gint            offset,
				     BatkTextBoundary boundary_type,
				     gint            *start_offset,
				     gint            *end_offset)
{
  BtkWidget *widget;
  BailExpander *expander;
  BtkWidget *label;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  expander = BAIL_EXPANDER (text);
  if (!expander->textutil) 
    bail_expander_init_textutil (expander, BTK_EXPANDER (widget));

  label = btk_expander_get_label_widget (BTK_EXPANDER (widget));
  if (!BTK_IS_LABEL(label))
    return NULL;
  return bail_text_util_get_text (expander->textutil,
                           btk_label_get_layout (BTK_LABEL (label)),
                           BAIL_AFTER_OFFSET, 
                           boundary_type, offset, start_offset, end_offset);
}

static gint
bail_expander_get_character_count (BatkText *text)
{
  BtkWidget *widget;
  BtkWidget *label;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  label = btk_expander_get_label_widget (BTK_EXPANDER (widget));
  if (!BTK_IS_LABEL(label))
    return 0;

  return g_utf8_strlen (btk_label_get_text (BTK_LABEL (label)), -1);
}

static void
bail_expander_get_character_extents (BatkText      *text,
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

  label = btk_expander_get_label_widget (BTK_EXPANDER (widget));
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
bail_expander_get_offset_at_point (BatkText      *text,
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
  label = btk_expander_get_label_widget (BTK_EXPANDER (widget));

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
bail_expander_get_run_attributes (BatkText *text,
                                  gint 	  offset,
                                  gint 	  *start_offset,
	                          gint	  *end_offset)
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

  label = btk_expander_get_label_widget (BTK_EXPANDER (widget));

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
bail_expander_get_default_attributes (BatkText *text)
{
  BtkWidget *widget;
  BtkWidget *label;
  BatkAttributeSet *at_set = NULL;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  label = btk_expander_get_label_widget (BTK_EXPANDER (widget));

  if (!BTK_IS_LABEL(label))
    return NULL;

  at_set = bail_misc_get_default_attributes (at_set,
                                             btk_label_get_layout (BTK_LABEL (label)),
                                             widget);
  return at_set;
}

static gunichar 
bail_expander_get_character_at_offset (BatkText *text,
                                       gint    offset)
{
  BtkWidget *widget;
  BtkWidget *label;
  const gchar *string;
  gchar *index;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return '\0';

  label = btk_expander_get_label_widget (BTK_EXPANDER (widget));

  if (!BTK_IS_LABEL(label))
    return '\0';
  string = btk_label_get_text (BTK_LABEL (label));
  if (offset >= g_utf8_strlen (string, -1))
    return '\0';
  index = g_utf8_offset_to_pointer (string, offset);

  return g_utf8_get_char (index);
}

static void
bail_expander_finalize (BObject *object)
{
  BailExpander *expander = BAIL_EXPANDER (object);

  g_free (expander->activate_description);
  g_free (expander->activate_keybinding);
  if (expander->action_idle_handler)
    {
      g_source_remove (expander->action_idle_handler);
      expander->action_idle_handler = 0;
    }
  if (expander->textutil)
    g_object_unref (expander->textutil);

  B_OBJECT_CLASS (bail_expander_parent_class)->finalize (object);
}
