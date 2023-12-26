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
#include <btk/btk.h>
#include "bailnotebookpage.h"
#include <libbail-util/bailmisc.h>
#include "bail-private-macros.h"

static void      bail_notebook_page_class_init      (BailNotebookPageClass     *klass);
static void                  bail_notebook_page_init           (BailNotebookPage *page);
static void                  bail_notebook_page_finalize       (GObject   *object);
static void                  bail_notebook_page_label_map_btk  (BtkWidget *widget,
                                                                gpointer  data);

static const gchar*          bail_notebook_page_get_name       (BatkObject *accessible);
static BatkObject*            bail_notebook_page_get_parent     (BatkObject *accessible);
static gint                  bail_notebook_page_get_n_children (BatkObject *accessible);
static BatkObject*            bail_notebook_page_ref_child      (BatkObject *accessible,
                                                                gint      i); 
static gint                  bail_notebook_page_get_index_in_parent
                                                               (BatkObject *accessible);
static BatkStateSet*          bail_notebook_page_ref_state_set  (BatkObject *accessible);

static gint                  bail_notebook_page_notify          (GObject   *obj,
                                                                 GParamSpec *pspec,
                                                                 gpointer   user_data);
static void                  bail_notebook_page_init_textutil   (BailNotebookPage      *notebook_page,
                                                                 BtkWidget             *label);

static void                  batk_component_interface_init      (BatkComponentIface *iface);

static BatkObject*            bail_notebook_page_ref_accessible_at_point 
                                                               (BatkComponent *component,
                                                                gint         x,
                                                                gint         y,
                                                                BatkCoordType coord_type);

static void                  bail_notebook_page_get_extents    (BatkComponent *component,
                                                                gint         *x,
                                                                gint         *y,
                                                                gint         *width,
                                                                gint         *height,
                                                                BatkCoordType coord_type);

static BatkObject*            _bail_notebook_page_get_tab_label (BailNotebookPage *page);

/* batktext.h */ 
static void	  batk_text_interface_init	   (BatkTextIface	*iface);

static gchar*	  bail_notebook_page_get_text	   (BatkText	      *text,
                                                    gint	      start_pos,
						    gint	      end_pos);
static gunichar	  bail_notebook_page_get_character_at_offset
                                                   (BatkText	      *text,
						    gint	      offset);
static gchar*     bail_notebook_page_get_text_before_offset
                                                   (BatkText	      *text,
 						    gint	      offset,
						    BatkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gchar*     bail_notebook_page_get_text_at_offset
                                                   (BatkText	      *text,
 						    gint	      offset,
						    BatkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gchar*     bail_notebook_page_get_text_after_offset
                                                   (BatkText	      *text,
 						    gint	      offset,
						    BatkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gint	  bail_notebook_page_get_character_count (BatkText	      *text);
static void       bail_notebook_page_get_character_extents
                                                   (BatkText	      *text,
						    gint 	      offset,
		                                    gint 	      *x,
                    		   	            gint 	      *y,
                                		    gint 	      *width,
                                     		    gint 	      *height,
			        		    BatkCoordType      coords);
static gint      bail_notebook_page_get_offset_at_point
                                                   (BatkText           *text,
                                                    gint              x,
                                                    gint              y,
			                            BatkCoordType      coords);
static BatkAttributeSet* bail_notebook_page_get_run_attributes 
                                                   (BatkText           *text,
              					    gint 	      offset,
                                                    gint 	      *start_offset,
					            gint	      *end_offset);
static BatkAttributeSet* bail_notebook_page_get_default_attributes
                                                   (BatkText           *text);
static BtkWidget* get_label_from_notebook_page     (BailNotebookPage  *page);
static BtkWidget* find_label_child (BtkContainer *container);

/* FIXME: not BAIL_TYPE_OBJECT? */
G_DEFINE_TYPE_WITH_CODE (BailNotebookPage, bail_notebook_page, BATK_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_COMPONENT, batk_component_interface_init)
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_TEXT, batk_text_interface_init))

static void
bail_notebook_page_class_init (BailNotebookPageClass *klass)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (klass);
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);
  
  class->get_name = bail_notebook_page_get_name;
  class->get_parent = bail_notebook_page_get_parent;
  class->get_n_children = bail_notebook_page_get_n_children;
  class->ref_child = bail_notebook_page_ref_child;
  class->ref_state_set = bail_notebook_page_ref_state_set;
  class->get_index_in_parent = bail_notebook_page_get_index_in_parent;

  bobject_class->finalize = bail_notebook_page_finalize;
}

static void
bail_notebook_page_init (BailNotebookPage *page)
{
}

static gint
notify_child_added (gpointer data)
{
  BailNotebookPage *page;
  BatkObject *batk_object, *batk_parent;

  g_return_val_if_fail (BAIL_IS_NOTEBOOK_PAGE (data), FALSE);
  page = BAIL_NOTEBOOK_PAGE (data);
  batk_object = BATK_OBJECT (data);

  page->notify_child_added_id = 0;

  /* The widget page->notebook may be deleted before this handler is called */
  if (page->notebook != NULL)
    {
      batk_parent = btk_widget_get_accessible (BTK_WIDGET (page->notebook));
      batk_object_set_parent (batk_object, batk_parent);
      g_signal_emit_by_name (batk_parent, "children_changed::add", page->index, batk_object, NULL);
    }
  
  return FALSE;
}

BatkObject*
bail_notebook_page_new (BtkNotebook *notebook, 
                        gint        pagenum)
{
  GObject *object;
  BatkObject *batk_object;
  BailNotebookPage *page;
  BtkWidget *child;
  BtkWidget *label;
  GList *list;
  
  g_return_val_if_fail (BTK_IS_NOTEBOOK (notebook), NULL);

  child = btk_notebook_get_nth_page (notebook, pagenum);

  if (!child)
    return NULL;

  object = g_object_new (BAIL_TYPE_NOTEBOOK_PAGE, NULL);
  g_return_val_if_fail (object != NULL, NULL);

  page = BAIL_NOTEBOOK_PAGE (object);
  page->notebook = notebook;
  g_object_add_weak_pointer (G_OBJECT (page->notebook), (gpointer *)&page->notebook);
  page->index = pagenum;
  list = g_list_nth (notebook->children, pagenum);
  page->page = list->data;
  page->textutil = NULL;
  
  batk_object = BATK_OBJECT (page);
  batk_object->role = BATK_ROLE_PAGE_TAB;
  batk_object->layer = BATK_LAYER_WIDGET;

  page->notify_child_added_id = bdk_threads_add_idle (notify_child_added, batk_object);
  /*
   * We get notified of changes to the label
   */
  label = get_label_from_notebook_page (page);
  if (BTK_IS_LABEL (label))
    {
      if (btk_widget_get_mapped (label))
        bail_notebook_page_init_textutil (page, label);
      else
        g_signal_connect (label,
                          "map",
                          G_CALLBACK (bail_notebook_page_label_map_btk),
                          page);
    }

  return batk_object;
}

static void
bail_notebook_page_label_map_btk (BtkWidget *widget,
                                  gpointer data)
{
  BailNotebookPage *page;

  page = BAIL_NOTEBOOK_PAGE (data);
  bail_notebook_page_init_textutil (page, widget);
}

static void
bail_notebook_page_init_textutil (BailNotebookPage *page,
                                  BtkWidget        *label)
{
  const gchar *label_text;

  if (page->textutil == NULL)
    {
      page->textutil = bail_text_util_new ();
      g_signal_connect (label,
                        "notify",
                        (GCallback) bail_notebook_page_notify,
                        page);     
    }
  label_text = btk_label_get_text (BTK_LABEL (label));
  bail_text_util_text_setup (page->textutil, label_text);
}

static gint
bail_notebook_page_notify (GObject    *obj, 
                           GParamSpec *pspec,
                           gpointer   user_data)
{
  BatkObject *batk_obj = BATK_OBJECT (user_data);
  BtkLabel *label;
  BailNotebookPage *page;

  if (strcmp (pspec->name, "label") == 0)
    {
      const gchar* label_text;

      label = BTK_LABEL (obj);

      label_text = btk_label_get_text (label);

      page = BAIL_NOTEBOOK_PAGE (batk_obj);
      bail_text_util_text_setup (page->textutil, label_text);

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
bail_notebook_page_finalize (GObject *object)
{
  BailNotebookPage *page = BAIL_NOTEBOOK_PAGE (object);

  if (page->notebook)
    g_object_remove_weak_pointer (G_OBJECT (page->notebook), (gpointer *)&page->notebook);

  if (page->textutil)
    g_object_unref (page->textutil);

  if (page->notify_child_added_id)
    g_source_remove (page->notify_child_added_id);

  G_OBJECT_CLASS (bail_notebook_page_parent_class)->finalize (object);
}

static const gchar*
bail_notebook_page_get_name (BatkObject *accessible)
{
  g_return_val_if_fail (BAIL_IS_NOTEBOOK_PAGE (accessible), NULL);
  
  if (accessible->name != NULL)
    return accessible->name;
  else
    {
      BtkWidget *label;

      label = get_label_from_notebook_page (BAIL_NOTEBOOK_PAGE (accessible));
      if (BTK_IS_LABEL (label))
        return btk_label_get_text (BTK_LABEL (label));
      else
        return NULL;
    }
}

static BatkObject*
bail_notebook_page_get_parent (BatkObject *accessible)
{
  BailNotebookPage *page;
  
  g_return_val_if_fail (BAIL_IS_NOTEBOOK_PAGE (accessible), NULL);
  
  page = BAIL_NOTEBOOK_PAGE (accessible);

  if (!page->notebook)
    return NULL;

  return btk_widget_get_accessible (BTK_WIDGET (page->notebook));
}

static gint
bail_notebook_page_get_n_children (BatkObject *accessible)
{
  /* Notebook page has only one child */
  g_return_val_if_fail (BAIL_IS_NOTEBOOK_PAGE (accessible), 0);

  return 1;
}

static BatkObject*
bail_notebook_page_ref_child (BatkObject *accessible,
                              gint i)
{
  BtkWidget *child;
  BatkObject *child_obj;
  BailNotebookPage *page = NULL;
   
  g_return_val_if_fail (BAIL_IS_NOTEBOOK_PAGE (accessible), NULL);
  if (i != 0)
    return NULL;
   
  page = BAIL_NOTEBOOK_PAGE (accessible);
  if (!page->notebook)
    return NULL;

  child = btk_notebook_get_nth_page (page->notebook, page->index);
  bail_return_val_if_fail (BTK_IS_WIDGET (child), NULL);
   
  child_obj = btk_widget_get_accessible (child);
  g_object_ref (child_obj);
  return child_obj;
}

static gint
bail_notebook_page_get_index_in_parent (BatkObject *accessible)
{
  BailNotebookPage *page;

  g_return_val_if_fail (BAIL_IS_NOTEBOOK_PAGE (accessible), -1);
  page = BAIL_NOTEBOOK_PAGE (accessible);

  return page->index;
}

static BatkStateSet*
bail_notebook_page_ref_state_set (BatkObject *accessible)
{
  BatkStateSet *state_set, *label_state_set, *merged_state_set;
  BatkObject *batk_label;

  g_return_val_if_fail (BAIL_NOTEBOOK_PAGE (accessible), NULL);

  state_set = BATK_OBJECT_CLASS (bail_notebook_page_parent_class)->ref_state_set (accessible);

  batk_label = _bail_notebook_page_get_tab_label (BAIL_NOTEBOOK_PAGE (accessible));
  if (batk_label)
    {
      label_state_set = batk_object_ref_state_set (batk_label);
      merged_state_set = batk_state_set_or_sets (state_set, label_state_set);
      g_object_unref (label_state_set);
      g_object_unref (state_set);
    }
  else
    {
      BatkObject *child;

      child = batk_object_ref_accessible_child (accessible, 0);
      bail_return_val_if_fail (child, state_set);

      merged_state_set = state_set;
      state_set = batk_object_ref_state_set (child);
      if (batk_state_set_contains_state (state_set, BATK_STATE_VISIBLE))
        {
          batk_state_set_add_state (merged_state_set, BATK_STATE_VISIBLE);
          if (batk_state_set_contains_state (state_set, BATK_STATE_ENABLED))
              batk_state_set_add_state (merged_state_set, BATK_STATE_ENABLED);
          if (batk_state_set_contains_state (state_set, BATK_STATE_SHOWING))
              batk_state_set_add_state (merged_state_set, BATK_STATE_SHOWING);

        } 
      g_object_unref (state_set);
      g_object_unref (child);
    }
  return merged_state_set;
}


static void
batk_component_interface_init (BatkComponentIface *iface)
{
  /*
   * We use the default implementations for contains, get_position, get_size
   */
  iface->ref_accessible_at_point = bail_notebook_page_ref_accessible_at_point;
  iface->get_extents = bail_notebook_page_get_extents;
}

static BatkObject*
bail_notebook_page_ref_accessible_at_point (BatkComponent *component,
                                            gint         x,
                                            gint         y,
                                            BatkCoordType coord_type)
{
  /*
   * There is only one child so we return it.
   */
  BatkObject* child;

  g_return_val_if_fail (BATK_IS_OBJECT (component), NULL);

  child = batk_object_ref_accessible_child (BATK_OBJECT (component), 0);
  return child;
}

static void
bail_notebook_page_get_extents (BatkComponent *component,
                                gint         *x,
                                gint         *y,
                                gint         *width,
                                gint         *height,
                                BatkCoordType coord_type)
{
  BatkObject *batk_label;

  g_return_if_fail (BAIL_IS_NOTEBOOK_PAGE (component));

  batk_label = _bail_notebook_page_get_tab_label (BAIL_NOTEBOOK_PAGE (component));

  if (!batk_label)
    {
      BatkObject *child;

      *width = 0;
      *height = 0;

      child = batk_object_ref_accessible_child (BATK_OBJECT (component), 0);
      bail_return_if_fail (child);

      batk_component_get_position (BATK_COMPONENT (child), x, y, coord_type);
      g_object_unref (child);
    }
  else
    {
      batk_component_get_extents (BATK_COMPONENT (batk_label), 
                                 x, y, width, height, coord_type);
    }
  return; 
}

static BatkObject*
_bail_notebook_page_get_tab_label (BailNotebookPage *page)
{
  BtkWidget *label;

  label = get_label_from_notebook_page (page);
  if (label)
    return btk_widget_get_accessible (label);
  else
    return NULL;
}

/* batktext.h */

static void
batk_text_interface_init (BatkTextIface *iface)
{
  iface->get_text = bail_notebook_page_get_text;
  iface->get_character_at_offset = bail_notebook_page_get_character_at_offset;
  iface->get_text_before_offset = bail_notebook_page_get_text_before_offset;
  iface->get_text_at_offset = bail_notebook_page_get_text_at_offset;
  iface->get_text_after_offset = bail_notebook_page_get_text_after_offset;
  iface->get_character_count = bail_notebook_page_get_character_count;
  iface->get_character_extents = bail_notebook_page_get_character_extents;
  iface->get_offset_at_point = bail_notebook_page_get_offset_at_point;
  iface->get_run_attributes = bail_notebook_page_get_run_attributes;
  iface->get_default_attributes = bail_notebook_page_get_default_attributes;
}

static gchar*
bail_notebook_page_get_text (BatkText *text,
                             gint    start_pos,
                             gint    end_pos)
{
  BtkWidget *label;
  BailNotebookPage *notebook_page;
  const gchar *label_text;

  notebook_page = BAIL_NOTEBOOK_PAGE (text);
  label = get_label_from_notebook_page (notebook_page);

  if (!BTK_IS_LABEL (label))
    return NULL;

  if (!notebook_page->textutil) 
    bail_notebook_page_init_textutil (notebook_page, label);

  label_text = btk_label_get_text (BTK_LABEL (label));

  if (label_text == NULL)
    return NULL;
  else
  {
    return bail_text_util_get_substring (notebook_page->textutil, 
                                         start_pos, end_pos);
  }
}

static gchar*
bail_notebook_page_get_text_before_offset (BatkText         *text,
     				           gint            offset,
				           BatkTextBoundary boundary_type,
				           gint            *start_offset,
				           gint            *end_offset)
{
  BtkWidget *label;
  BailNotebookPage *notebook_page;
  
  notebook_page = BAIL_NOTEBOOK_PAGE (text);
  label = get_label_from_notebook_page (notebook_page);

  if (!BTK_IS_LABEL(label))
    return NULL;

  if (!notebook_page->textutil)
    bail_notebook_page_init_textutil (notebook_page, label);

  return bail_text_util_get_text (notebook_page->textutil,
                           btk_label_get_layout (BTK_LABEL (label)), BAIL_BEFORE_OFFSET, 
                           boundary_type, offset, start_offset, end_offset); 
}

static gchar*
bail_notebook_page_get_text_at_offset (BatkText         *text,
			               gint            offset,
			               BatkTextBoundary boundary_type,
 			               gint            *start_offset,
			               gint            *end_offset)
{
  BtkWidget *label;
  BailNotebookPage *notebook_page;
 
  notebook_page = BAIL_NOTEBOOK_PAGE (text);
  label = get_label_from_notebook_page (notebook_page);

  if (!BTK_IS_LABEL(label))
    return NULL;

  if (!notebook_page->textutil)
    bail_notebook_page_init_textutil (notebook_page, label);

  return bail_text_util_get_text (notebook_page->textutil,
                              btk_label_get_layout (BTK_LABEL (label)), BAIL_AT_OFFSET, 
                              boundary_type, offset, start_offset, end_offset);
}

static gchar*
bail_notebook_page_get_text_after_offset (BatkText         *text,
				          gint            offset,
				          BatkTextBoundary boundary_type,
				          gint            *start_offset,
				          gint            *end_offset)
{
  BtkWidget *label;
  BailNotebookPage *notebook_page;

  notebook_page = BAIL_NOTEBOOK_PAGE (text);
  label = get_label_from_notebook_page (notebook_page);

  if (!BTK_IS_LABEL(label))
    return NULL;

  if (!notebook_page->textutil)
    bail_notebook_page_init_textutil (notebook_page, label);

  return bail_text_util_get_text (notebook_page->textutil,
                           btk_label_get_layout (BTK_LABEL (label)), BAIL_AFTER_OFFSET, 
                           boundary_type, offset, start_offset, end_offset);
}

static gint
bail_notebook_page_get_character_count (BatkText *text)
{
  BtkWidget *label;
  BailNotebookPage *notebook_page;

  notebook_page = BAIL_NOTEBOOK_PAGE (text);
  label = get_label_from_notebook_page (notebook_page);

  if (!BTK_IS_LABEL(label))
    return 0;

  return g_utf8_strlen (btk_label_get_text (BTK_LABEL (label)), -1);
}

static void
bail_notebook_page_get_character_extents (BatkText      *text,
				          gint         offset,
		                          gint         *x,
                    		          gint         *y,
                                          gint 	       *width,
                                          gint 	       *height,
			                  BatkCoordType coords)
{
  BtkWidget *label;
  BailNotebookPage *notebook_page;
  BangoRectangle char_rect;
  gint index, x_layout, y_layout;
  const gchar *label_text;
 
  notebook_page = BAIL_NOTEBOOK_PAGE (text);
  label = get_label_from_notebook_page (notebook_page);

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
bail_notebook_page_get_offset_at_point (BatkText      *text,
                                        gint         x,
                                        gint         y,
	          		        BatkCoordType coords)
{ 
  BtkWidget *label;
  BailNotebookPage *notebook_page;
  gint index, x_layout, y_layout;
  const gchar *label_text;

  notebook_page = BAIL_NOTEBOOK_PAGE (text);
  label = get_label_from_notebook_page (notebook_page);

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
bail_notebook_page_get_run_attributes (BatkText *text,
                                       gint    offset,
                                       gint    *start_offset,
	                               gint    *end_offset)
{
  BtkWidget *label;
  BailNotebookPage *notebook_page;
  BatkAttributeSet *at_set = NULL;
  BtkJustification justify;
  BtkTextDirection dir;

  notebook_page = BAIL_NOTEBOOK_PAGE (text);
  label = get_label_from_notebook_page (notebook_page);

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
bail_notebook_page_get_default_attributes (BatkText *text)
{
  BtkWidget *label;
  BailNotebookPage *notebook_page;
  BatkAttributeSet *at_set = NULL;

  notebook_page = BAIL_NOTEBOOK_PAGE (text);
  label = get_label_from_notebook_page (notebook_page);

  if (!BTK_IS_LABEL(label))
    return NULL;

  at_set = bail_misc_get_default_attributes (at_set,
                                             btk_label_get_layout (BTK_LABEL (label)),
                                             label);
  return at_set;
}

static gunichar 
bail_notebook_page_get_character_at_offset (BatkText *text,
                                            gint    offset)
{
  BtkWidget *label;
  BailNotebookPage *notebook_page;
  const gchar *string;
  gchar *index;

  notebook_page = BAIL_NOTEBOOK_PAGE (text);
  label = get_label_from_notebook_page (notebook_page);

  if (!BTK_IS_LABEL(label))
    return '\0';
  string = btk_label_get_text (BTK_LABEL (label));
  if (offset >= g_utf8_strlen (string, -1))
    return '\0';
  index = g_utf8_offset_to_pointer (string, offset);

  return g_utf8_get_char (index);
}

static BtkWidget*
get_label_from_notebook_page (BailNotebookPage *page)
{
  BtkWidget *child;
  BtkNotebook *notebook;

  notebook = page->notebook;
  if (!notebook)
    return NULL;

  if (!btk_notebook_get_show_tabs (notebook))
    return NULL;

  child = btk_notebook_get_nth_page (notebook, page->index);
  if (child == NULL) return NULL;
  g_return_val_if_fail (BTK_IS_WIDGET (child), NULL);

  child = btk_notebook_get_tab_label (notebook, child);

  if (BTK_IS_LABEL (child))
    return child;

  if (BTK_IS_CONTAINER (child))
    child = find_label_child (BTK_CONTAINER (child));

  return child;
}

static BtkWidget*
find_label_child (BtkContainer *container)
{
  GList *children, *tmp_list;
  BtkWidget *child;

  children = btk_container_get_children (container);

  child = NULL;
  for (tmp_list = children; tmp_list != NULL; tmp_list = tmp_list->next)
    {
      if (BTK_IS_LABEL (tmp_list->data))
        {
          child = BTK_WIDGET (tmp_list->data);
          break;
        }
      else if (BTK_IS_CONTAINER (tmp_list->data))
        {
          child = find_label_child (BTK_CONTAINER (tmp_list->data));
          if (child)
            break;
        }
    }
  g_list_free (children);
  return child;
}
