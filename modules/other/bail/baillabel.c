/* BAIL - The BUNNY Accessibility Enabling Library
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
#include "baillabel.h"
#include "bailwindow.h"
#include <libbail-util/bailmisc.h>

static void       bail_label_class_init            (BailLabelClass    *klass);
static void       bail_label_init                  (BailLabel         *label);
static void	  bail_label_real_initialize	   (BatkObject 	      *obj,
                                                    gpointer	      data);
static void	  bail_label_real_notify_btk	   (BObject	      *obj,
                                                    BParamSpec	      *pspec);
static void       bail_label_map_btk               (BtkWidget         *widget,
                                                    gpointer          data);
static void       bail_label_init_text_util        (BailLabel         *bail_label,
                                                    BtkWidget         *widget);
static void       bail_label_finalize              (BObject           *object);

static void       batk_text_interface_init          (BatkTextIface      *iface);

/* batkobject.h */

static const gchar*          bail_label_get_name         (BatkObject         *accessible);
static BatkStateSet*          bail_label_ref_state_set	 (BatkObject	    *accessible);
static BatkRelationSet*       bail_label_ref_relation_set (BatkObject         *accessible);

/* batktext.h */

static gchar*	  bail_label_get_text		   (BatkText	      *text,
                                                    gint	      start_pos,
						    gint	      end_pos);
static gunichar	  bail_label_get_character_at_offset(BatkText	      *text,
						    gint	      offset);
static gchar*     bail_label_get_text_before_offset(BatkText	      *text,
 						    gint	      offset,
						    BatkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gchar*     bail_label_get_text_at_offset    (BatkText	      *text,
 						    gint	      offset,
						    BatkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gchar*     bail_label_get_text_after_offset    (BatkText	      *text,
 						    gint	      offset,
						    BatkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gint	  bail_label_get_character_count   (BatkText	      *text);
static gint	  bail_label_get_caret_offset	   (BatkText	      *text);
static gboolean	  bail_label_set_caret_offset	   (BatkText	      *text,
                                                    gint	      offset);
static gint	  bail_label_get_n_selections	   (BatkText	      *text);
static gchar*	  bail_label_get_selection	   (BatkText	      *text,
                                                    gint	      selection_num,
                                                    gint	      *start_offset,
                                                    gint	      *end_offset);
static gboolean	  bail_label_add_selection	   (BatkText	      *text,
                                                    gint	      start_offset,
                                                    gint	      end_offset);
static gboolean	  bail_label_remove_selection	   (BatkText	      *text,
                                                    gint	      selection_num);
static gboolean	  bail_label_set_selection	   (BatkText	      *text,
                                                    gint	      selection_num,
                                                    gint	      start_offset,
						    gint	      end_offset);
static void bail_label_get_character_extents       (BatkText	      *text,
						    gint 	      offset,
		                                    gint 	      *x,
                    		   	            gint 	      *y,
                                		    gint 	      *width,
                                     		    gint 	      *height,
			        		    BatkCoordType      coords);
static gint bail_label_get_offset_at_point         (BatkText           *text,
                                                    gint              x,
                                                    gint              y,
			                            BatkCoordType      coords);
static BatkAttributeSet* bail_label_get_run_attributes 
                                                   (BatkText           *text,
              					    gint 	      offset,
                                                    gint 	      *start_offset,
					            gint	      *end_offset);
static BatkAttributeSet* bail_label_get_default_attributes
                                                   (BatkText           *text);

G_DEFINE_TYPE_WITH_CODE (BailLabel, bail_label, BAIL_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_TEXT, batk_text_interface_init))

static void
bail_label_class_init (BailLabelClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);
  BatkObjectClass  *class = BATK_OBJECT_CLASS (klass);
  BailWidgetClass *widget_class;

  bobject_class->finalize = bail_label_finalize;

  widget_class = (BailWidgetClass*)klass;
  widget_class->notify_btk = bail_label_real_notify_btk;

  class->get_name = bail_label_get_name;
  class->ref_state_set = bail_label_ref_state_set;
  class->ref_relation_set = bail_label_ref_relation_set;
  class->initialize = bail_label_real_initialize;
}

static void
bail_label_init (BailLabel *label)
{
}

static void
bail_label_real_initialize (BatkObject *obj,
                            gpointer  data)
{
  BtkWidget  *widget;
  BailLabel *bail_label;

  BATK_OBJECT_CLASS (bail_label_parent_class)->initialize (obj, data);
  
  bail_label = BAIL_LABEL (obj);

  bail_label->window_create_handler = 0;
  bail_label->has_top_level = FALSE;
  bail_label->cursor_position = 0;
  bail_label->selection_bound = 0;
  bail_label->textutil = NULL;
  bail_label->label_length = 0;
  
  widget = BTK_WIDGET (data);

  if (btk_widget_get_mapped (widget))
    bail_label_init_text_util (bail_label, widget);
  else
    g_signal_connect (widget,
                      "map",
                      G_CALLBACK (bail_label_map_btk),
                      bail_label);

  /* 
   * Check whether ancestor of BtkLabel is a BtkButton and if so
   * set accessible parent for BailLabel
   */
  while (widget != NULL)
    {
      widget = btk_widget_get_parent (widget);
      if (BTK_IS_BUTTON (widget))
        {
          batk_object_set_parent (obj, btk_widget_get_accessible (widget));
          break;
        }
    }

  if (BTK_IS_ACCEL_LABEL (widget))
    obj->role = BATK_ROLE_ACCEL_LABEL;
  else
    obj->role = BATK_ROLE_LABEL;
}

static void
bail_label_map_btk (BtkWidget *widget,
                    gpointer data)
{
  BailLabel *bail_label;

  bail_label = BAIL_LABEL (data);
  bail_label_init_text_util (bail_label, widget);
}

static void
bail_label_init_text_util (BailLabel *bail_label,
                           BtkWidget *widget)
{
  BtkLabel *label;
  const gchar *label_text;

  if (bail_label->textutil == NULL)
    bail_label->textutil = bail_text_util_new ();

  label = BTK_LABEL (widget);
  label_text = btk_label_get_text (label);
  bail_text_util_text_setup (bail_label->textutil, label_text);
  
  if (label_text == NULL)
    bail_label->label_length = 0;
  else
    bail_label->label_length = g_utf8_strlen (label_text, -1);
}

static void
notify_name_change (BatkObject *batk_obj)
{
  BtkLabel *label;
  BailLabel *bail_label;
  BtkWidget *widget;
  BObject *bail_obj;

  widget = BTK_ACCESSIBLE (batk_obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return;

  bail_obj = B_OBJECT (batk_obj);
  label = BTK_LABEL (widget);
  bail_label = BAIL_LABEL (batk_obj);

  if (bail_label->textutil == NULL)
    return;

  /*
   * Check whether the label has actually changed before emitting
   * notification.
   */
  if (bail_label->textutil->buffer) 
    {
      BtkTextIter start, end;
      const char *new_label;
      char *old_label;
      int same;   

      btk_text_buffer_get_start_iter (bail_label->textutil->buffer, &start);
      btk_text_buffer_get_end_iter (bail_label->textutil->buffer, &end);
      old_label = btk_text_buffer_get_text (bail_label->textutil->buffer, &start, &end, FALSE);
      new_label = btk_label_get_text (label);
      same = strcmp (new_label, old_label);
      g_free (old_label);
      if (same == 0)
        return;
    }
   
  /* Create a delete text and an insert text signal */
 
  g_signal_emit_by_name (bail_obj, "text_changed::delete", 0, 
                         bail_label->label_length);

  bail_label_init_text_util (bail_label, widget);

  g_signal_emit_by_name (bail_obj, "text_changed::insert", 0, 
                         bail_label->label_length);

  if (batk_obj->name == NULL)
    /*
     * The label has changed so notify a change in accessible-name
     */
    g_object_notify (bail_obj, "accessible-name");

  g_signal_emit_by_name (bail_obj, "visible_data_changed");
}

static void
window_created (BObject *obj,
                gpointer data)
{
  g_return_if_fail (BAIL_LABEL (data));

  notify_name_change (BATK_OBJECT (data)); 
}

static void
bail_label_real_notify_btk (BObject           *obj,
                            BParamSpec        *pspec)
{
  BtkWidget *widget = BTK_WIDGET (obj);
  BatkObject* batk_obj = btk_widget_get_accessible (widget);
  BtkLabel *label;
  BailLabel *bail_label;
  BObject *bail_obj;
  BatkObject *top_level;
  BatkObject *temp_obj;

  bail_label = BAIL_LABEL (batk_obj);

  if (strcmp (pspec->name, "label") == 0)
    {
     /* 
      * We may get a label change for a label which is not attached to an
      * application. We wait until the toplevel window is created before
      * emitting the notification.
      *
      * This happens when [Ctrl+]Alt+Tab is pressed in metacity
      */
      if (!bail_label->has_top_level)
        {
          temp_obj = batk_obj;
          top_level = NULL;
          while (temp_obj)
            {
              top_level = temp_obj;
              temp_obj = batk_object_get_parent (top_level);
            }
          if (batk_object_get_role (top_level) != BATK_ROLE_APPLICATION)
            {
              if (bail_label->window_create_handler == 0 && 
                  BAIL_IS_WINDOW (top_level))
                bail_label->window_create_handler = g_signal_connect_after (top_level, "create", G_CALLBACK (window_created), batk_obj);
            }
          else
            bail_label->has_top_level = TRUE;
        }
      if (bail_label->has_top_level)
        notify_name_change (batk_obj);
    }
  else if (strcmp (pspec->name, "cursor-position") == 0)
    {
      gint start, end, tmp;
      gboolean text_caret_moved = FALSE;
      gboolean selection_changed = FALSE;

      bail_obj = B_OBJECT (batk_obj);
      label = BTK_LABEL (widget);

      if (bail_label->selection_bound != -1 && bail_label->selection_bound < bail_label->cursor_position)
        {
          tmp = bail_label->selection_bound;
          bail_label->selection_bound = bail_label->cursor_position;
          bail_label->cursor_position = tmp;
        }

      if (btk_label_get_selection_bounds (label, &start, &end))
        {
          if (start != bail_label->cursor_position ||
              end != bail_label->selection_bound)
            {
              if (end != bail_label->selection_bound)
                {
                  bail_label->selection_bound = start;
                  bail_label->cursor_position = end;
                }
              else
                {
                  bail_label->selection_bound = end;
                  bail_label->cursor_position = start;
                }
              text_caret_moved = TRUE;
              if (start != end)
                selection_changed = TRUE;
            }
        }
      else 
        {
          if (bail_label->cursor_position != bail_label->selection_bound)
            selection_changed = TRUE;
          if (btk_label_get_selectable (label))
            {
              if (bail_label->cursor_position != -1 && start != bail_label->cursor_position)
                text_caret_moved = TRUE;
              if (bail_label->selection_bound != -1 && end != bail_label->selection_bound)
                {
                  text_caret_moved = TRUE;
                  bail_label->cursor_position = end;
                  bail_label->selection_bound = start;
                }
              else
                {
                  bail_label->cursor_position = start;
                  bail_label->selection_bound = end;
                }
            }
          else
            {
              /* BtkLabel has become non selectable */

              bail_label->cursor_position = 0;
              bail_label->selection_bound = 0;
              text_caret_moved = TRUE;
            }

        }
        if (text_caret_moved)
          g_signal_emit_by_name (bail_obj, "text_caret_moved", 
                                 bail_label->cursor_position);
        if (selection_changed)
          g_signal_emit_by_name (bail_obj, "text_selection_changed");

    }
  else
    BAIL_WIDGET_CLASS (bail_label_parent_class)->notify_btk (obj, pspec);
}

static void
bail_label_finalize (BObject            *object)
{
  BailLabel *label = BAIL_LABEL (object);

  if (label->textutil)
    g_object_unref (label->textutil);
  B_OBJECT_CLASS (bail_label_parent_class)->finalize (object);
}


/* batkobject.h */

static BatkStateSet*
bail_label_ref_state_set (BatkObject *accessible)
{
  BatkStateSet *state_set;
  BtkWidget *widget;

  state_set = BATK_OBJECT_CLASS (bail_label_parent_class)->ref_state_set (accessible);
  widget = BTK_ACCESSIBLE (accessible)->widget;

  if (widget == NULL)
    return state_set;

  batk_state_set_add_state (state_set, BATK_STATE_MULTI_LINE);

  return state_set;
}

BatkRelationSet*
bail_label_ref_relation_set (BatkObject *obj)
{
  BtkWidget *widget;
  BatkRelationSet *relation_set;

  g_return_val_if_fail (BAIL_IS_LABEL (obj), NULL);

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  relation_set = BATK_OBJECT_CLASS (bail_label_parent_class)->ref_relation_set (obj);

  if (!batk_relation_set_contains (relation_set, BATK_RELATION_LABEL_FOR))
    {
      /*
       * Get the mnemonic widget
       *
       * The relation set is not updated if the mnemonic widget is changed
       */
      BtkWidget *mnemonic_widget = BTK_LABEL (widget)->mnemonic_widget;

      if (mnemonic_widget)
        {
          BatkObject *accessible_array[1];
          BatkRelation* relation;

          if (!btk_widget_get_can_focus (mnemonic_widget))
            {
            /*
             * Handle the case where a BtkFileChooserButton is specified as the 
             * mnemonic widget. use the combobox which is a child of the
             * BtkFileChooserButton as the mnemonic widget. See bug #359843.
             */
             if (BTK_IS_BOX (mnemonic_widget))
               {
                  GList *list, *tmpl;

                  list = btk_container_get_children (BTK_CONTAINER (mnemonic_widget));
                  if (g_list_length (list) == 2)
                    {
                      tmpl = g_list_last (list);
                      if (BTK_IS_COMBO_BOX(tmpl->data))
                        {
                          mnemonic_widget = BTK_WIDGET(tmpl->data);
                        }
                    }
                  g_list_free (list);
                }
            /*
             * Handle the case where a BunnyIconEntry is specified as the 
             * mnemonic widget. use the button which is a grandchild of the
             * BunnyIconEntry as the mnemonic widget. See bug #133967.
             */
              else if (BTK_IS_BOX (mnemonic_widget))
                {
                  GList *list;

                  list = btk_container_get_children (BTK_CONTAINER (mnemonic_widget));
                  if (g_list_length (list) == 1)
                    {
                      if (BTK_IS_ALIGNMENT (list->data))
                        {
                          BtkWidget *temp_widget;

                          temp_widget = BTK_BIN (list->data)->child;
                          if (BTK_IS_BUTTON (temp_widget))
                            mnemonic_widget = temp_widget;
                        }
                      else if (BTK_IS_HBOX (list->data))
                        {
                          BtkWidget *temp_widget;

                          temp_widget = BTK_WIDGET (list->data);
                          g_list_free (list);
                          list = btk_container_get_children (BTK_CONTAINER (temp_widget));
                          if (BTK_IS_COMBO (list->data))
                            {
                              mnemonic_widget = BTK_WIDGET (list->data);
                            }
                        }
                    }
                  g_list_free (list);
                }
            }
          accessible_array[0] = btk_widget_get_accessible (mnemonic_widget);
          relation = batk_relation_new (accessible_array, 1,
                                       BATK_RELATION_LABEL_FOR);
          batk_relation_set_add (relation_set, relation);
          /*
           * Unref the relation so that it is not leaked.
           */
          g_object_unref (relation);
        }
    }
  return relation_set;
}

static const gchar*
bail_label_get_name (BatkObject *accessible)
{
  const gchar *name;

  g_return_val_if_fail (BAIL_IS_LABEL (accessible), NULL);

  name = BATK_OBJECT_CLASS (bail_label_parent_class)->get_name (accessible);
  if (name != NULL)
    return name;
  else
    {
      /*
       * Get the text on the label
       */
      BtkWidget *widget;

      widget = BTK_ACCESSIBLE (accessible)->widget;
      if (widget == NULL)
        /*
         * State is defunct
         */
        return NULL;

      g_return_val_if_fail (BTK_IS_LABEL (widget), NULL);

      return btk_label_get_text (BTK_LABEL (widget));
    }
}

/* batktext.h */

static void
batk_text_interface_init (BatkTextIface *iface)
{
  iface->get_text = bail_label_get_text;
  iface->get_character_at_offset = bail_label_get_character_at_offset;
  iface->get_text_before_offset = bail_label_get_text_before_offset;
  iface->get_text_at_offset = bail_label_get_text_at_offset;
  iface->get_text_after_offset = bail_label_get_text_after_offset;
  iface->get_character_count = bail_label_get_character_count;
  iface->get_caret_offset = bail_label_get_caret_offset;
  iface->set_caret_offset = bail_label_set_caret_offset;
  iface->get_n_selections = bail_label_get_n_selections;
  iface->get_selection = bail_label_get_selection;
  iface->add_selection = bail_label_add_selection;
  iface->remove_selection = bail_label_remove_selection;
  iface->set_selection = bail_label_set_selection;
  iface->get_character_extents = bail_label_get_character_extents;
  iface->get_offset_at_point = bail_label_get_offset_at_point;
  iface->get_run_attributes = bail_label_get_run_attributes;
  iface->get_default_attributes = bail_label_get_default_attributes;
}

static gchar*
bail_label_get_text (BatkText *text,
                     gint    start_pos,
                     gint    end_pos)
{
  BtkWidget *widget;
  BtkLabel  *label;

  const gchar *label_text;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  label = BTK_LABEL (widget);

  label_text = btk_label_get_text (label);
 
  if (label_text == NULL)
    return NULL;
  else
  {
    if (BAIL_LABEL (text)->textutil == NULL) 
      bail_label_init_text_util (BAIL_LABEL (text), widget);
    return bail_text_util_get_substring (BAIL_LABEL(text)->textutil, 
                                         start_pos, end_pos);
  }
}

static gchar*
bail_label_get_text_before_offset (BatkText         *text,
				   gint            offset,
				   BatkTextBoundary boundary_type,
				   gint            *start_offset,
				   gint            *end_offset)
{
  BtkWidget *widget;
  BtkLabel *label;
  
  widget = BTK_ACCESSIBLE (text)->widget;
  
  if (widget == NULL)
    /* State is defunct */
    return NULL;
  
  /* Get label */
  label = BTK_LABEL (widget);

  return bail_text_util_get_text (BAIL_LABEL (text)->textutil,
                           btk_label_get_layout (label), BAIL_BEFORE_OFFSET, 
                           boundary_type, offset, start_offset, end_offset); 
}

static gchar*
bail_label_get_text_at_offset (BatkText         *text,
			       gint            offset,
			       BatkTextBoundary boundary_type,
 			       gint            *start_offset,
			       gint            *end_offset)
{
  BtkWidget *widget;
  BtkLabel *label;
 
  widget = BTK_ACCESSIBLE (text)->widget;
  
  if (widget == NULL)
    /* State is defunct */
    return NULL;
  
  /* Get label */
  label = BTK_LABEL (widget);

  return bail_text_util_get_text (BAIL_LABEL (text)->textutil,
                              btk_label_get_layout (label), BAIL_AT_OFFSET, 
                              boundary_type, offset, start_offset, end_offset);
}

static gchar*
bail_label_get_text_after_offset (BatkText         *text,
				  gint            offset,
				  BatkTextBoundary boundary_type,
				  gint            *start_offset,
				  gint            *end_offset)
{
  BtkWidget *widget;
  BtkLabel *label;

  widget = BTK_ACCESSIBLE (text)->widget;
  
  if (widget == NULL)
  {
    /* State is defunct */
    return NULL;
  }
  
  /* Get label */
  label = BTK_LABEL (widget);

  return bail_text_util_get_text (BAIL_LABEL (text)->textutil,
                           btk_label_get_layout (label), BAIL_AFTER_OFFSET, 
                           boundary_type, offset, start_offset, end_offset);
}

static gint
bail_label_get_character_count (BatkText *text)
{
  BtkWidget *widget;
  BtkLabel  *label;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  label = BTK_LABEL (widget);
  return g_utf8_strlen (btk_label_get_text (label), -1);
}

static gint
bail_label_get_caret_offset (BatkText *text)
{
   return BAIL_LABEL (text)->cursor_position;
}

static gboolean
bail_label_set_caret_offset (BatkText *text, 
                             gint    offset)
{
  BtkWidget *widget;
  BtkLabel  *label;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  label = BTK_LABEL (widget);

  if (btk_label_get_selectable (label) &&
      offset >= 0 &&
      offset <= g_utf8_strlen (label->text, -1))
    {
      btk_label_select_rebunnyion (label, offset, offset);
      return TRUE;
    }
  else
    return FALSE;
}

static gint
bail_label_get_n_selections (BatkText *text)
{
  BtkWidget *widget;
  BtkLabel  *label;
  gint start, end;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  label = BTK_LABEL (widget);

  if (!btk_label_get_selectable (label))
     return 0;

  if (btk_label_get_selection_bounds (label, &start, &end))
     return 1;
  else 
     return 0;
}

static gchar*
bail_label_get_selection (BatkText *text,
			  gint    selection_num,
                          gint    *start_pos,
                          gint    *end_pos)
{
  BtkWidget *widget;
  BtkLabel  *label;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  label = BTK_LABEL (widget);

 /* Only let the user get the selection if one is set, and if the
  * selection_num is 0.
  */
  if (!btk_label_get_selectable( label) || selection_num != 0)
     return NULL;

  if (btk_label_get_selection_bounds (label, start_pos, end_pos))
    {
      const gchar* label_text = btk_label_get_text (label);
    
      if (label_text == NULL)
        return 0;
      else
        return bail_text_util_get_substring (BAIL_LABEL (text)->textutil, 
                                             *start_pos, *end_pos);
    }
  else 
    return NULL;
}

static gboolean
bail_label_add_selection (BatkText *text,
                          gint    start_pos,
                          gint    end_pos)
{
  BtkWidget *widget;
  BtkLabel  *label;
  gint start, end;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;

  label = BTK_LABEL (widget);

  if (!btk_label_get_selectable (label))
     return FALSE;

  if (! btk_label_get_selection_bounds (label, &start, &end))
    {
      btk_label_select_rebunnyion (label, start_pos, end_pos);
      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
bail_label_remove_selection (BatkText *text,
                             gint    selection_num)
{
  BtkWidget *widget;
  BtkLabel  *label;
  gint start, end;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;

  if (selection_num != 0)
     return FALSE;

  label = BTK_LABEL (widget);

  if (!btk_label_get_selectable (label))
     return FALSE;

  if (btk_label_get_selection_bounds (label, &start, &end))
    {
      btk_label_select_rebunnyion (label, 0, 0);
      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
bail_label_set_selection (BatkText *text,
			  gint	  selection_num,
                          gint    start_pos,
                          gint    end_pos)
{
  BtkWidget *widget;
  BtkLabel  *label;
  gint start, end;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;

  if (selection_num != 0)
     return FALSE;

  label = BTK_LABEL (widget);

  if (!btk_label_get_selectable (label))
     return FALSE;

  if (btk_label_get_selection_bounds (label, &start, &end))
    {
      btk_label_select_rebunnyion (label, start_pos, end_pos);
      return TRUE;
    }
  else
    return FALSE;
}

static void
bail_label_get_character_extents (BatkText      *text,
				  gint         offset,
		                  gint         *x,
                    		  gint 	       *y,
                                  gint 	       *width,
                                  gint 	       *height,
			          BatkCoordType coords)
{
  BtkWidget *widget;
  BtkLabel *label;
  BangoRectangle char_rect;
  gint index, x_layout, y_layout;
 
  widget = BTK_ACCESSIBLE (text)->widget;

  if (widget == NULL)
    /* State is defunct */
    return;

  label = BTK_LABEL (widget);
  
  btk_label_get_layout_offsets (label, &x_layout, &y_layout);
  index = g_utf8_offset_to_pointer (label->text, offset) - label->text;
  bango_layout_index_to_pos (btk_label_get_layout (label), index, &char_rect);
  
  bail_misc_get_extents_from_bango_rectangle (widget, &char_rect, 
                    x_layout, y_layout, x, y, width, height, coords);
} 

static gint 
bail_label_get_offset_at_point (BatkText      *text,
                                gint         x,
                                gint         y,
			        BatkCoordType coords)
{ 
  BtkWidget *widget;
  BtkLabel *label;
  gint index, x_layout, y_layout;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return -1;
  label = BTK_LABEL (widget);
  
  btk_label_get_layout_offsets (label, &x_layout, &y_layout);
  
  index = bail_misc_get_index_at_point_in_layout (widget, 
                                              btk_label_get_layout (label), 
                                              x_layout, y_layout, x, y, coords);
  if (index == -1)
    {
      if (coords == BATK_XY_WINDOW || coords == BATK_XY_SCREEN)
        return g_utf8_strlen (label->text, -1);

      return index;  
    }
  else
    return g_utf8_pointer_to_offset (label->text, label->text + index);  
}

static BatkAttributeSet*
bail_label_get_run_attributes (BatkText        *text,
                               gint 	      offset,
                               gint 	      *start_offset,
	                       gint	      *end_offset)
{
  BtkWidget *widget;
  BtkLabel *label;
  BatkAttributeSet *at_set = NULL;
  BtkJustification justify;
  BtkTextDirection dir;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  label = BTK_LABEL (widget);
  
  /* Get values set for entire label, if any */
  justify = btk_label_get_justify (label);
  if (justify != BTK_JUSTIFY_CENTER)
    {
      at_set = bail_misc_add_attribute (at_set, 
                                        BATK_TEXT_ATTR_JUSTIFICATION,
     g_strdup (batk_text_attribute_get_value (BATK_TEXT_ATTR_JUSTIFICATION, justify)));
    }
  dir = btk_widget_get_direction (widget);
  if (dir == BTK_TEXT_DIR_RTL)
    {
      at_set = bail_misc_add_attribute (at_set, 
                                        BATK_TEXT_ATTR_DIRECTION,
     g_strdup (batk_text_attribute_get_value (BATK_TEXT_ATTR_DIRECTION, dir)));
    }

  at_set = bail_misc_layout_get_run_attributes (at_set,
                                                btk_label_get_layout (label),
                                                label->text,
                                                offset,
                                                start_offset,
                                                end_offset);
  return at_set;
}

static BatkAttributeSet*
bail_label_get_default_attributes (BatkText        *text)
{
  BtkWidget *widget;
  BtkLabel *label;
  BatkAttributeSet *at_set = NULL;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  label = BTK_LABEL (widget);
  
  at_set = bail_misc_get_default_attributes (at_set,
                                             btk_label_get_layout (label),
                                             widget);
  return at_set;
}

static gunichar 
bail_label_get_character_at_offset (BatkText	         *text,
                                    gint	         offset)
{
  BtkWidget *widget;
  BtkLabel *label;
  const gchar *string;
  gchar *index;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return '\0';

  label = BTK_LABEL (widget);
  string = btk_label_get_text (label);
  if (offset >= g_utf8_strlen (string, -1))
    return '\0';
  index = g_utf8_offset_to_pointer (string, offset);

  return g_utf8_get_char (index);
}
