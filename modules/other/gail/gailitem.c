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
#include "bailitem.h"
#include <libbail-util/bailmisc.h>

static void                  bail_item_class_init      (BailItemClass *klass);
static void                  bail_item_init            (BailItem      *item);
static const gchar*          bail_item_get_name        (BatkObject     *obj);
static gint                  bail_item_get_n_children  (BatkObject     *obj);
static BatkObject*            bail_item_ref_child       (BatkObject     *obj,
                                                        gint          i);
static void                  bail_item_real_initialize (BatkObject     *obj,
                                                        gpointer      data);
static void                  bail_item_label_map_btk   (BtkWidget     *widget,
                                                        gpointer      data);
static void                  bail_item_finalize        (GObject       *object);
static void                  bail_item_init_textutil   (BailItem      *item,
                                                        BtkWidget     *label);
static void                  bail_item_notify_label_btk(GObject       *obj,
                                                        GParamSpec    *pspec,
                                                        gpointer      data);

/* batktext.h */ 
static void	  batk_text_interface_init	   (BatkTextIface	*iface);

static gchar*	  bail_item_get_text		   (BatkText	      *text,
                                                    gint	      start_pos,
						    gint	      end_pos);
static gunichar	  bail_item_get_character_at_offset(BatkText	      *text,
						    gint	      offset);
static gchar*     bail_item_get_text_before_offset (BatkText	      *text,
 						    gint	      offset,
						    BatkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gchar*     bail_item_get_text_at_offset     (BatkText	      *text,
 						    gint	      offset,
						    BatkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gchar*     bail_item_get_text_after_offset  (BatkText	      *text,
 						    gint	      offset,
						    BatkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gint	  bail_item_get_character_count    (BatkText	      *text);
static void       bail_item_get_character_extents  (BatkText	      *text,
						    gint 	      offset,
		                                    gint 	      *x,
                    		   	            gint 	      *y,
                                		    gint 	      *width,
                                     		    gint 	      *height,
			        		    BatkCoordType      coords);
static gint      bail_item_get_offset_at_point     (BatkText           *text,
                                                    gint              x,
                                                    gint              y,
			                            BatkCoordType      coords);
static BatkAttributeSet* bail_item_get_run_attributes 
                                                   (BatkText           *text,
              					    gint 	      offset,
                                                    gint 	      *start_offset,
					            gint	      *end_offset);
static BatkAttributeSet* bail_item_get_default_attributes
                                                   (BatkText           *text);
static BtkWidget*            get_label_from_container   (BtkWidget    *container);

G_DEFINE_TYPE_WITH_CODE (BailItem, bail_item, BAIL_TYPE_CONTAINER,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_TEXT, batk_text_interface_init))

static void
bail_item_class_init (BailItemClass *klass)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (klass);
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);
  BailContainerClass *container_class;

  container_class = (BailContainerClass *)klass;

  bobject_class->finalize = bail_item_finalize;

  class->get_name = bail_item_get_name;
  class->get_n_children = bail_item_get_n_children;
  class->ref_child = bail_item_ref_child;
  class->initialize = bail_item_real_initialize;
  /*
   * As we report the item as having no children we are not interested
   * in add and remove signals
   */
  container_class->add_btk = NULL;
  container_class->remove_btk = NULL;
}

static void
bail_item_init (BailItem      *item)
{
}

static void
bail_item_real_initialize (BatkObject *obj,
                           gpointer	data)
{
  BailItem *item = BAIL_ITEM (obj);
  BtkWidget  *label;

  BATK_OBJECT_CLASS (bail_item_parent_class)->initialize (obj, data);

  item->textutil = NULL;
  item->text = NULL;

  label = get_label_from_container (BTK_WIDGET (data));
  if (BTK_IS_LABEL (label))
    {
      if (btk_widget_get_mapped (label))
        bail_item_init_textutil (item, label);
      else
        g_signal_connect (label,
                          "map",
                          G_CALLBACK (bail_item_label_map_btk),
                          item);
    }

  obj->role = BATK_ROLE_LIST_ITEM;
}

static void
bail_item_label_map_btk (BtkWidget *widget,
                         gpointer data)
{
  BailItem *item;

  item = BAIL_ITEM (data);
  bail_item_init_textutil (item, widget);
}

static void
bail_item_init_textutil (BailItem  *item,
                         BtkWidget *label)
{
  const gchar *label_text;

  if (item->textutil == NULL)
    {
      item->textutil = bail_text_util_new ();
      g_signal_connect (label,
                        "notify",
                        (GCallback) bail_item_notify_label_btk,
                        item);     
    }
  label_text = btk_label_get_text (BTK_LABEL (label));
  bail_text_util_text_setup (item->textutil, label_text);
}

static void
bail_item_finalize (GObject *object)
{
  BailItem *item = BAIL_ITEM (object);

  if (item->textutil)
    {
      g_object_unref (item->textutil);
    }
  if (item->text)
    {
      g_free (item->text);
      item->text = NULL;
    }
  G_OBJECT_CLASS (bail_item_parent_class)->finalize (object);
}

static const gchar*
bail_item_get_name (BatkObject *obj)
{
  const gchar* name;

  g_return_val_if_fail (BAIL_IS_ITEM (obj), NULL);

  name = BATK_OBJECT_CLASS (bail_item_parent_class)->get_name (obj);
  if (name == NULL)
    {
      /*
       * Get the label child
       */
      BtkWidget *widget;
      BtkWidget *label;

      widget = BTK_ACCESSIBLE (obj)->widget;
      if (widget == NULL)
        /*
         * State is defunct
         */
        return NULL;

      label = get_label_from_container (widget);
      if (BTK_IS_LABEL (label))
	return btk_label_get_text (BTK_LABEL(label));
      /*
       * If we have a menu item in a menu attached to a BtkOptionMenu
       * the label of the selected item is detached from the menu item
       */
      else if (BTK_IS_MENU_ITEM (widget))
        {
          BtkWidget *parent;
          BtkWidget *attach;
          GList *list;
          BatkObject *parent_obj;
          gint index;

          parent = btk_widget_get_parent (widget);
          if (BTK_IS_MENU (parent))
            {
              attach = btk_menu_get_attach_widget (BTK_MENU (parent)); 

              if (BTK_IS_OPTION_MENU (attach))
                {
                  label = get_label_from_container (attach);
                  if (BTK_IS_LABEL (label))
	            return btk_label_get_text (BTK_LABEL(label));
                }
              list = btk_container_get_children (BTK_CONTAINER (parent));
              index = g_list_index (list, widget);

              if (index < 0 || index > g_list_length (list))
                {
                  g_list_free (list);
                  return NULL;
                }
              g_list_free (list);

              parent_obj = batk_object_get_parent (btk_widget_get_accessible (parent));
              if (BTK_IS_ACCESSIBLE (parent_obj))
                {
                  parent = BTK_ACCESSIBLE (parent_obj)->widget;
                  if (BTK_IS_COMBO_BOX (parent))
                    {
                      BtkTreeModel *model;
                      BtkTreeIter iter;
                      BailItem *item;
                      gint n_columns, i;

                      model = btk_combo_box_get_model (BTK_COMBO_BOX (parent));                       
                      item = BAIL_ITEM (obj);
                      if (btk_tree_model_iter_nth_child (model, &iter, NULL, index))
                        {
                          n_columns = btk_tree_model_get_n_columns (model);
                          for (i = 0; i < n_columns; i++)
                            {
                              GValue value = { 0, };

                               btk_tree_model_get_value (model, &iter, i, &value);
                               if (G_VALUE_HOLDS_STRING (&value))
                                 {
				   g_free (item->text);
                                   item->text =  (gchar *) g_value_dup_string (&value);
                                   g_value_unset (&value);
                                   break;
                                 }

                               g_value_unset (&value);
                            }
                        }
                      name = item->text;
                    }
                }
            }
        }
    }
  return name;
}

/*
 * We report that this object has no children
 */

static gint
bail_item_get_n_children (BatkObject* obj)
{
  return 0;
}

static BatkObject*
bail_item_ref_child (BatkObject *obj,
                     gint      i)
{
  return NULL;
}

static void
bail_item_notify_label_btk (GObject           *obj,
                            GParamSpec        *pspec,
                            gpointer           data)
{
  BatkObject* batk_obj = BATK_OBJECT (data);
  BtkLabel *label;
  BailItem *bail_item;

  if (strcmp (pspec->name, "label") == 0)
    {
      const gchar* label_text;

      label = BTK_LABEL (obj);

      label_text = btk_label_get_text (label);

      bail_item = BAIL_ITEM (batk_obj);
      bail_text_util_text_setup (bail_item->textutil, label_text);

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
}

/* batktext.h */

static void
batk_text_interface_init (BatkTextIface *iface)
{
  iface->get_text = bail_item_get_text;
  iface->get_character_at_offset = bail_item_get_character_at_offset;
  iface->get_text_before_offset = bail_item_get_text_before_offset;
  iface->get_text_at_offset = bail_item_get_text_at_offset;
  iface->get_text_after_offset = bail_item_get_text_after_offset;
  iface->get_character_count = bail_item_get_character_count;
  iface->get_character_extents = bail_item_get_character_extents;
  iface->get_offset_at_point = bail_item_get_offset_at_point;
  iface->get_run_attributes = bail_item_get_run_attributes;
  iface->get_default_attributes = bail_item_get_default_attributes;
}

static gchar*
bail_item_get_text (BatkText *text,
                    gint    start_pos,
                    gint    end_pos)
{
  BtkWidget *widget;
  BtkWidget *label;
  BailItem *item;
  const gchar *label_text;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  label = get_label_from_container (widget);

  if (!BTK_IS_LABEL (label))
    return NULL;

  item = BAIL_ITEM (text);
  if (!item->textutil) 
    bail_item_init_textutil (item, label);

  label_text = btk_label_get_text (BTK_LABEL (label));

  if (label_text == NULL)
    return NULL;
  else
  {
    return bail_text_util_get_substring (item->textutil, 
                                         start_pos, end_pos);
  }
}

static gchar*
bail_item_get_text_before_offset (BatkText         *text,
				  gint            offset,
				  BatkTextBoundary boundary_type,
				  gint            *start_offset,
				  gint            *end_offset)
{
  BtkWidget *widget;
  BtkWidget *label;
  BailItem *item;
  
  widget = BTK_ACCESSIBLE (text)->widget;
  
  if (widget == NULL)
    /* State is defunct */
    return NULL;
  
  /* Get label */
  label = get_label_from_container (widget);

  if (!BTK_IS_LABEL(label))
    return NULL;

  item = BAIL_ITEM (text);
  if (!item->textutil)
    bail_item_init_textutil (item, label);

  return bail_text_util_get_text (item->textutil,
                           btk_label_get_layout (BTK_LABEL (label)), BAIL_BEFORE_OFFSET, 
                           boundary_type, offset, start_offset, end_offset); 
}

static gchar*
bail_item_get_text_at_offset (BatkText         *text,
			      gint            offset,
			      BatkTextBoundary boundary_type,
 			      gint            *start_offset,
			      gint            *end_offset)
{
  BtkWidget *widget;
  BtkWidget *label;
  BailItem *item;
 
  widget = BTK_ACCESSIBLE (text)->widget;
  
  if (widget == NULL)
    /* State is defunct */
    return NULL;
  
  /* Get label */
  label = get_label_from_container (widget);

  if (!BTK_IS_LABEL(label))
    return NULL;

  item = BAIL_ITEM (text);
  if (!item->textutil)
    bail_item_init_textutil (item, label);

  return bail_text_util_get_text (item->textutil,
                              btk_label_get_layout (BTK_LABEL (label)), BAIL_AT_OFFSET, 
                              boundary_type, offset, start_offset, end_offset);
}

static gchar*
bail_item_get_text_after_offset (BatkText         *text,
				 gint            offset,
				 BatkTextBoundary boundary_type,
				 gint            *start_offset,
				 gint            *end_offset)
{
  BtkWidget *widget;
  BtkWidget *label;
  BailItem *item;

  widget = BTK_ACCESSIBLE (text)->widget;
  
  if (widget == NULL)
  {
    /* State is defunct */
    return NULL;
  }
  
  /* Get label */
  label = get_label_from_container (widget);

  if (!BTK_IS_LABEL(label))
    return NULL;

  item = BAIL_ITEM (text);
  if (!item->textutil)
    bail_item_init_textutil (item, label);

  return bail_text_util_get_text (item->textutil,
                           btk_label_get_layout (BTK_LABEL (label)), BAIL_AFTER_OFFSET, 
                           boundary_type, offset, start_offset, end_offset);
}

static gint
bail_item_get_character_count (BatkText *text)
{
  BtkWidget *widget;
  BtkWidget *label;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  label = get_label_from_container (widget);

  if (!BTK_IS_LABEL(label))
    return 0;

  return g_utf8_strlen (btk_label_get_text (BTK_LABEL (label)), -1);
}

static void
bail_item_get_character_extents (BatkText      *text,
				 gint         offset,
		                 gint         *x,
                    		 gint 	      *y,
                                 gint 	      *width,
                                 gint 	      *height,
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

  label = get_label_from_container (widget);

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
bail_item_get_offset_at_point (BatkText      *text,
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

  label = get_label_from_container (widget);

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
bail_item_get_run_attributes (BatkText *text,
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

  label = get_label_from_container (widget);

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
bail_item_get_default_attributes (BatkText *text)
{
  BtkWidget *widget;
  BtkWidget *label;
  BatkAttributeSet *at_set = NULL;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  label = get_label_from_container (widget);

  if (!BTK_IS_LABEL(label))
    return NULL;

  at_set = bail_misc_get_default_attributes (at_set,
                                             btk_label_get_layout (BTK_LABEL (label)),
                                             widget);
  return at_set;
}

static gunichar 
bail_item_get_character_at_offset (BatkText *text,
                                   gint	   offset)
{
  BtkWidget *widget;
  BtkWidget *label;
  const gchar *string;
  gchar *index;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return '\0';

  label = get_label_from_container (widget);

  if (!BTK_IS_LABEL(label))
    return '\0';
  string = btk_label_get_text (BTK_LABEL (label));
  if (offset >= g_utf8_strlen (string, -1))
    return '\0';
  index = g_utf8_offset_to_pointer (string, offset);

  return g_utf8_get_char (index);
}

static BtkWidget*
get_label_from_container (BtkWidget *container)
{
  BtkWidget *label;
  GList *children, *tmp_list;

  if (!BTK_IS_CONTAINER (container))
    return NULL;
 
  children = btk_container_get_children (BTK_CONTAINER (container));
  label = NULL;

  for (tmp_list = children; tmp_list != NULL; tmp_list = tmp_list->next) 
    {
      if (BTK_IS_LABEL (tmp_list->data))
	{
           label = tmp_list->data;
           break;
	}
      /*
       * Get label from menu item in desktop background preferences
       * option menu. See bug #144084.
       */
      else if (BTK_IS_BOX (tmp_list->data))
        {
           label = get_label_from_container (BTK_WIDGET (tmp_list->data));
           if (label)
             break;
        }
    }
  g_list_free (children);

  return label;
}
