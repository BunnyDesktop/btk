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
#include "bailstatusbar.h"
#include <libbail-util/bailmisc.h>

static void         bail_statusbar_class_init          (BailStatusbarClass *klass);
static void         bail_statusbar_init                (BailStatusbar      *bar);
static const gchar* bail_statusbar_get_name            (BatkObject          *obj);
static gint         bail_statusbar_get_n_children      (BatkObject          *obj);
static BatkObject*   bail_statusbar_ref_child           (BatkObject          *obj,
                                                        gint               i);
static void         bail_statusbar_real_initialize     (BatkObject          *obj,
                                                        gpointer           data);

static gint         bail_statusbar_notify              (GObject            *obj,
                                                        GParamSpec         *pspec,
                                                        gpointer           user_data);
static void         bail_statusbar_finalize            (GObject            *object);
static void         bail_statusbar_init_textutil       (BailStatusbar      *statusbar,
                                                        BtkWidget          *label);

/* batktext.h */ 
static void	  batk_text_interface_init	   (BatkTextIface	*iface);

static gchar*	  bail_statusbar_get_text	   (BatkText	      *text,
                                                    gint	      start_pos,
						    gint	      end_pos);
static gunichar	  bail_statusbar_get_character_at_offset
                                                   (BatkText	      *text,
						    gint	      offset);
static gchar*     bail_statusbar_get_text_before_offset
                                                   (BatkText	      *text,
 						    gint	      offset,
						    BatkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gchar*     bail_statusbar_get_text_at_offset(BatkText	      *text,
 						    gint	      offset,
						    BatkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gchar*     bail_statusbar_get_text_after_offset
                                                   (BatkText	      *text,
 						    gint	      offset,
						    BatkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gint	  bail_statusbar_get_character_count (BatkText	      *text);
static void       bail_statusbar_get_character_extents
                                                   (BatkText	      *text,
						    gint 	      offset,
		                                    gint 	      *x,
                    		   	            gint 	      *y,
                                		    gint 	      *width,
                                     		    gint 	      *height,
			        		    BatkCoordType      coords);
static gint      bail_statusbar_get_offset_at_point(BatkText           *text,
                                                    gint              x,
                                                    gint              y,
			                            BatkCoordType      coords);
static BatkAttributeSet* bail_statusbar_get_run_attributes 
                                                   (BatkText           *text,
              					    gint 	      offset,
                                                    gint 	      *start_offset,
					            gint	      *end_offset);
static BatkAttributeSet* bail_statusbar_get_default_attributes
                                                   (BatkText           *text);
static BtkWidget* get_label_from_statusbar         (BtkWidget         *statusbar);

G_DEFINE_TYPE_WITH_CODE (BailStatusbar, bail_statusbar, BAIL_TYPE_CONTAINER,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_TEXT, batk_text_interface_init))

static void
bail_statusbar_class_init (BailStatusbarClass *klass)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (klass);
  BatkObjectClass  *class = BATK_OBJECT_CLASS (klass);
  BailContainerClass *container_class;

  container_class = (BailContainerClass*)klass;

  bobject_class->finalize = bail_statusbar_finalize;

  class->get_name = bail_statusbar_get_name;
  class->get_n_children = bail_statusbar_get_n_children;
  class->ref_child = bail_statusbar_ref_child;
  class->initialize = bail_statusbar_real_initialize;
  /*
   * As we report the statusbar as having no children we are not interested
   * in add and remove signals
   */
  container_class->add_btk = NULL;
  container_class->remove_btk = NULL;
}

static void
bail_statusbar_init (BailStatusbar *bar)
{
}

static const gchar*
bail_statusbar_get_name (BatkObject *obj)
{
  const gchar* name;

  g_return_val_if_fail (BAIL_IS_STATUSBAR (obj), NULL);

  name = BATK_OBJECT_CLASS (bail_statusbar_parent_class)->get_name (obj);
  if (name != NULL)
    return name;
  else
    {
      /*
       * Get the text on the label
       */
      BtkWidget *widget;
      BtkWidget *label;

      widget = BTK_ACCESSIBLE (obj)->widget;
      if (widget == NULL)
        /*
         * State is defunct
         */
        return NULL;

     g_return_val_if_fail (BTK_IS_STATUSBAR (widget), NULL);
     label = get_label_from_statusbar (widget);
     if (BTK_IS_LABEL (label))
       return btk_label_get_label (BTK_LABEL (label));
     else 
       return NULL;
   }
}

static gint
bail_statusbar_get_n_children (BatkObject *obj)
{
  BtkWidget *widget;
  GList *children;
  gint count = 0;

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    return 0;

  children = btk_container_get_children (BTK_CONTAINER (widget));
  if (children != NULL)
    {
      count = g_list_length (children);
      g_list_free (children);
    }

  return count;
}

static BatkObject*
bail_statusbar_ref_child (BatkObject *obj,
                          gint      i)
{
  GList *children, *tmp_list;
  BatkObject  *accessible;
  BtkWidget *widget;

  g_return_val_if_fail ((i >= 0), NULL);
  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    return NULL;

  children = btk_container_get_children (BTK_CONTAINER (widget));
  if (children == NULL)
    return NULL;

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
bail_statusbar_real_initialize (BatkObject *obj,
                                gpointer  data)
{
  BailStatusbar *statusbar = BAIL_STATUSBAR (obj);
  BtkWidget *label;

  BATK_OBJECT_CLASS (bail_statusbar_parent_class)->initialize (obj, data);

  /*
   * We get notified of changes to the label
   */
  label = get_label_from_statusbar (BTK_WIDGET (data));
  if (BTK_IS_LABEL (label))
    {
      bail_statusbar_init_textutil (statusbar, label);
    }

  obj->role = BATK_ROLE_STATUSBAR;

}

static gint
bail_statusbar_notify (GObject    *obj, 
                       GParamSpec *pspec,
                       gpointer   user_data)
{
  BatkObject *batk_obj = BATK_OBJECT (user_data);
  BtkLabel *label;
  BailStatusbar *statusbar;

  if (strcmp (pspec->name, "label") == 0)
    {
      const gchar* label_text;

      label = BTK_LABEL (obj);

      label_text = btk_label_get_text (label);

      statusbar = BAIL_STATUSBAR (batk_obj);
      bail_text_util_text_setup (statusbar->textutil, label_text);

      if (batk_obj->name == NULL)
      {
        /*
         * The label has changed so notify a change in accessible-name
         */
        g_object_notify (G_OBJECT (batk_obj), "accessible-name");
      }
      /*
       * The label is the only property which can be changed
       */
      g_signal_emit_by_name (batk_obj, "visible_data_changed");
    }
  return 1;
}

static void
bail_statusbar_init_textutil (BailStatusbar *statusbar,
                              BtkWidget     *label)
{
  const gchar *label_text;

  statusbar->textutil = bail_text_util_new ();
  label_text = btk_label_get_text (BTK_LABEL (label));
  bail_text_util_text_setup (statusbar->textutil, label_text);
  g_signal_connect (label,
                    "notify",
                    (GCallback) bail_statusbar_notify,
                    statusbar);     
}

static void
bail_statusbar_finalize (GObject *object)
{
  BailStatusbar *statusbar = BAIL_STATUSBAR (object);

  if (statusbar->textutil)
    {
      g_object_unref (statusbar->textutil);
    }
  G_OBJECT_CLASS (bail_statusbar_parent_class)->finalize (object);
}

/* batktext.h */

static void
batk_text_interface_init (BatkTextIface *iface)
{
  iface->get_text = bail_statusbar_get_text;
  iface->get_character_at_offset = bail_statusbar_get_character_at_offset;
  iface->get_text_before_offset = bail_statusbar_get_text_before_offset;
  iface->get_text_at_offset = bail_statusbar_get_text_at_offset;
  iface->get_text_after_offset = bail_statusbar_get_text_after_offset;
  iface->get_character_count = bail_statusbar_get_character_count;
  iface->get_character_extents = bail_statusbar_get_character_extents;
  iface->get_offset_at_point = bail_statusbar_get_offset_at_point;
  iface->get_run_attributes = bail_statusbar_get_run_attributes;
  iface->get_default_attributes = bail_statusbar_get_default_attributes;
}

static gchar*
bail_statusbar_get_text (BatkText *text,
                         gint    start_pos,
                         gint    end_pos)
{
  BtkWidget *widget;
  BtkWidget *label;
  BailStatusbar *statusbar;
  const gchar *label_text;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  label = get_label_from_statusbar (widget);

  if (!BTK_IS_LABEL (label))
    return NULL;

  statusbar = BAIL_STATUSBAR (text);
  if (!statusbar->textutil) 
    bail_statusbar_init_textutil (statusbar, label);

  label_text = btk_label_get_text (BTK_LABEL (label));

  if (label_text == NULL)
    return NULL;
  else
  {
    return bail_text_util_get_substring (statusbar->textutil, 
                                         start_pos, end_pos);
  }
}

static gchar*
bail_statusbar_get_text_before_offset (BatkText         *text,
     				       gint            offset,
				       BatkTextBoundary boundary_type,
				       gint            *start_offset,
				       gint            *end_offset)
{
  BtkWidget *widget;
  BtkWidget *label;
  BailStatusbar *statusbar;
  
  widget = BTK_ACCESSIBLE (text)->widget;
  
  if (widget == NULL)
    /* State is defunct */
    return NULL;
  
  /* Get label */
  label = get_label_from_statusbar (widget);

  if (!BTK_IS_LABEL(label))
    return NULL;

  statusbar = BAIL_STATUSBAR (text);
  if (!statusbar->textutil)
    bail_statusbar_init_textutil (statusbar, label);

  return bail_text_util_get_text (statusbar->textutil,
                           btk_label_get_layout (BTK_LABEL (label)), BAIL_BEFORE_OFFSET, 
                           boundary_type, offset, start_offset, end_offset); 
}

static gchar*
bail_statusbar_get_text_at_offset (BatkText         *text,
			           gint            offset,
			           BatkTextBoundary boundary_type,
 			           gint            *start_offset,
			           gint            *end_offset)
{
  BtkWidget *widget;
  BtkWidget *label;
  BailStatusbar *statusbar;
 
  widget = BTK_ACCESSIBLE (text)->widget;
  
  if (widget == NULL)
    /* State is defunct */
    return NULL;
  
  /* Get label */
  label = get_label_from_statusbar (widget);

  if (!BTK_IS_LABEL(label))
    return NULL;

  statusbar = BAIL_STATUSBAR (text);
  if (!statusbar->textutil)
    bail_statusbar_init_textutil (statusbar, label);

  return bail_text_util_get_text (statusbar->textutil,
                              btk_label_get_layout (BTK_LABEL (label)), BAIL_AT_OFFSET, 
                              boundary_type, offset, start_offset, end_offset);
}

static gchar*
bail_statusbar_get_text_after_offset (BatkText         *text,
				      gint            offset,
				      BatkTextBoundary boundary_type,
				      gint            *start_offset,
				      gint            *end_offset)
{
  BtkWidget *widget;
  BtkWidget *label;
  BailStatusbar *statusbar;

  widget = BTK_ACCESSIBLE (text)->widget;
  
  if (widget == NULL)
  {
    /* State is defunct */
    return NULL;
  }
  
  /* Get label */
  label = get_label_from_statusbar (widget);

  if (!BTK_IS_LABEL(label))
    return NULL;

  statusbar = BAIL_STATUSBAR (text);
  if (!statusbar->textutil)
    bail_statusbar_init_textutil (statusbar, label);

  return bail_text_util_get_text (statusbar->textutil,
                           btk_label_get_layout (BTK_LABEL (label)), BAIL_AFTER_OFFSET, 
                           boundary_type, offset, start_offset, end_offset);
}

static gint
bail_statusbar_get_character_count (BatkText *text)
{
  BtkWidget *widget;
  BtkWidget *label;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  label = get_label_from_statusbar (widget);

  if (!BTK_IS_LABEL(label))
    return 0;

  return g_utf8_strlen (btk_label_get_text (BTK_LABEL (label)), -1);
}

static void
bail_statusbar_get_character_extents (BatkText      *text,
				      gint         offset,
		                      gint         *x,
                    		      gint 	   *y,
                                      gint 	   *width,
                                      gint 	   *height,
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

  label = get_label_from_statusbar (widget);

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
bail_statusbar_get_offset_at_point (BatkText      *text,
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

  label = get_label_from_statusbar (widget);

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
bail_statusbar_get_run_attributes (BatkText *text,
                                   gint    offset,
                                   gint    *start_offset,
	                           gint    *end_offset)
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

  label = get_label_from_statusbar (widget);

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
bail_statusbar_get_default_attributes (BatkText *text)
{
  BtkWidget *widget;
  BtkWidget *label;
  BatkAttributeSet *at_set = NULL;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  label = get_label_from_statusbar (widget);

  if (!BTK_IS_LABEL(label))
    return NULL;

  at_set = bail_misc_get_default_attributes (at_set,
                                             btk_label_get_layout (BTK_LABEL (label)),
                                             widget);
  return at_set;
}

static gunichar 
bail_statusbar_get_character_at_offset (BatkText *text,
                                        gint	offset)
{
  BtkWidget *widget;
  BtkWidget *label;
  const gchar *string;
  gchar *index;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return '\0';

  label = get_label_from_statusbar (widget);

  if (!BTK_IS_LABEL(label))
    return '\0';
  string = btk_label_get_text (BTK_LABEL (label));
  if (offset >= g_utf8_strlen (string, -1))
    return '\0';
  index = g_utf8_offset_to_pointer (string, offset);

  return g_utf8_get_char (index);
}

static BtkWidget*
get_label_from_statusbar (BtkWidget *statusbar)
{
  return BTK_STATUSBAR (statusbar)->label;
}
