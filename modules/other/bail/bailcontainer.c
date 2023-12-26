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

#include <btk/btk.h>
#include "bailcontainer.h"

static void         bail_container_class_init          (BailContainerClass *klass);
static void         bail_container_init                (BailContainer      *container);

static gint         bail_container_get_n_children      (BatkObject          *obj);
static BatkObject*   bail_container_ref_child           (BatkObject          *obj,
                                                        gint               i);
static gint         bail_container_add_btk             (BtkContainer       *container,
                                                        BtkWidget          *widget,
                                                        gpointer           data);
static gint         bail_container_remove_btk          (BtkContainer       *container,
                                                        BtkWidget          *widget,
                                                        gpointer           data);
static gint         bail_container_real_add_btk        (BtkContainer       *container,
                                                        BtkWidget          *widget,
                                                        gpointer           data);
static gint         bail_container_real_remove_btk     (BtkContainer       *container,
                                                        BtkWidget          *widget,
                                                        gpointer           data);

static void          bail_container_real_initialize    (BatkObject          *obj,
                                                        gpointer           data);

static void          bail_container_finalize           (GObject            *object);

G_DEFINE_TYPE (BailContainer, bail_container, BAIL_TYPE_WIDGET)

static void
bail_container_class_init (BailContainerClass *klass)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (klass);
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);

  bobject_class->finalize = bail_container_finalize;

  class->get_n_children = bail_container_get_n_children;
  class->ref_child = bail_container_ref_child;
  class->initialize = bail_container_real_initialize;

  klass->add_btk = bail_container_real_add_btk;
  klass->remove_btk = bail_container_real_remove_btk;
}

static void
bail_container_init (BailContainer      *container)
{
  container->children = NULL;
}

static gint
bail_container_get_n_children (BatkObject* obj)
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

  return count; 
}

static BatkObject* 
bail_container_ref_child (BatkObject *obj,
                          gint       i)
{
  GList *children, *tmp_list;
  BatkObject  *accessible;
  BtkWidget *widget;

  g_return_val_if_fail (BAIL_IS_CONTAINER (obj), NULL);
  g_return_val_if_fail ((i >= 0), NULL);
  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    return NULL;

  children = btk_container_get_children (BTK_CONTAINER (widget));
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

static gint
bail_container_add_btk (BtkContainer *container,
                        BtkWidget    *widget,
                        gpointer     data)
{
  BailContainer *bail_container = BAIL_CONTAINER (data);
  BailContainerClass *klass;

  klass = BAIL_CONTAINER_GET_CLASS (bail_container);

  if (klass->add_btk)
    return klass->add_btk (container, widget, data);
  else
    return 1;
}
 
static gint
bail_container_remove_btk (BtkContainer *container,
                           BtkWidget    *widget,
                           gpointer     data)
{
  BailContainer *bail_container = BAIL_CONTAINER (data);
  BailContainerClass *klass;

  klass = BAIL_CONTAINER_GET_CLASS (bail_container);

  if (klass->remove_btk)
    return klass->remove_btk (container, widget, data);
  else
    return 1;
}
 
static gint
bail_container_real_add_btk (BtkContainer *container,
                             BtkWidget    *widget,
                             gpointer     data)
{
  BatkObject* batk_parent = BATK_OBJECT (data);
  BatkObject* batk_child = btk_widget_get_accessible (widget);
  BailContainer *bail_container = BAIL_CONTAINER (batk_parent);
  gint       index;

  g_object_notify (G_OBJECT (batk_child), "accessible_parent");

  g_list_free (bail_container->children);
  bail_container->children = btk_container_get_children (container);
  index = g_list_index (bail_container->children, widget);
  g_signal_emit_by_name (batk_parent, "children_changed::add", 
                         index, batk_child, NULL);

  return 1;
}

static gint
bail_container_real_remove_btk (BtkContainer       *container,
                                BtkWidget          *widget,
                                gpointer           data)
{
  BatkPropertyValues values = { NULL };
  BatkObject* batk_parent;
  BatkObject *batk_child;
  BailContainer *bail_container;
  gint       index;

  batk_parent = BATK_OBJECT (data);
  batk_child = btk_widget_get_accessible (widget);

  if (batk_child)
    {
      g_value_init (&values.old_value, G_TYPE_POINTER);
      g_value_set_pointer (&values.old_value, batk_parent);
    
      values.property_name = "accessible-parent";

      g_object_ref (batk_child);
      g_signal_emit_by_name (batk_child,
                             "property_change::accessible-parent", &values, NULL);
      g_object_unref (batk_child);
    }
  bail_container = BAIL_CONTAINER (batk_parent);
  index = g_list_index (bail_container->children, widget);
  g_list_free (bail_container->children);
  bail_container->children = btk_container_get_children (container);
  if (index >= 0 && index <= g_list_length (bail_container->children))
    g_signal_emit_by_name (batk_parent, "children_changed::remove", 
			   index, batk_child, NULL);

  return 1;
}

static void
bail_container_real_initialize (BatkObject *obj,
                                gpointer  data)
{
  BailContainer *container = BAIL_CONTAINER (obj);
  guint handler_id;

  BATK_OBJECT_CLASS (bail_container_parent_class)->initialize (obj, data);

  container->children = btk_container_get_children (BTK_CONTAINER (data));

  /*
   * We store the handler ids for these signals in case some objects
   * need to remove these handlers.
   */
  handler_id = g_signal_connect (data,
                                 "add",
                                 G_CALLBACK (bail_container_add_btk),
                                 obj);
  g_object_set_data (G_OBJECT (obj), "bail-add-handler-id", 
                     GUINT_TO_POINTER (handler_id));
  handler_id = g_signal_connect (data,
                                 "remove",
                                 G_CALLBACK (bail_container_remove_btk),
                                 obj);
  g_object_set_data (G_OBJECT (obj), "bail-remove-handler-id", 
                     GUINT_TO_POINTER (handler_id));

  if (BTK_IS_TOOLBAR (data))
    obj->role = BATK_ROLE_TOOL_BAR;
  else if (BTK_IS_VIEWPORT (data))
    obj->role = BATK_ROLE_VIEWPORT;
  else
    obj->role = BATK_ROLE_PANEL;
}

static void
bail_container_finalize (GObject *object)
{
  BailContainer *container = BAIL_CONTAINER (object);

  g_list_free (container->children);
  G_OBJECT_CLASS (bail_container_parent_class)->finalize (object);
}
