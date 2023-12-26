/* BTK - The GIMP Toolkit
 * btksizegroup.c: 
 * Copyright (C) 2001 Red Hat Software
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
#include "btkcontainer.h"
#include "btkintl.h"
#include "btkprivate.h"
#include "btksizegroup.h"
#include "btkbuildable.h"
#include "btkalias.h"

enum {
  PROP_0,
  PROP_MODE,
  PROP_IGNORE_HIDDEN
};

static void btk_size_group_set_property (BObject      *object,
					 guint         prop_id,
					 const BValue *value,
					 BParamSpec   *pspec);
static void btk_size_group_get_property (BObject      *object,
					 guint         prop_id,
					 BValue       *value,
					 BParamSpec   *pspec);

static void add_group_to_closure  (BtkSizeGroup      *group,
				   BtkSizeGroupMode   mode,
				   GSList           **groups,
				   GSList           **widgets);
static void add_widget_to_closure (BtkWidget         *widget,
				   BtkSizeGroupMode   mode,
				   GSList           **groups,
				   GSList           **widgets);

/* BtkBuildable */
static void btk_size_group_buildable_init (BtkBuildableIface *iface);
static gboolean btk_size_group_buildable_custom_tag_start (BtkBuildable  *buildable,
							   BtkBuilder    *builder,
							   BObject       *child,
							   const gchar   *tagname,
							   GMarkupParser *parser,
							   gpointer      *data);
static void btk_size_group_buildable_custom_finished (BtkBuildable  *buildable,
						      BtkBuilder    *builder,
						      BObject       *child,
						      const gchar   *tagname,
						      gpointer       user_data);

static GQuark size_groups_quark;
static const gchar size_groups_tag[] = "btk-size-groups";

static GQuark visited_quark;
static const gchar visited_tag[] = "btk-size-group-visited";

static GSList *
get_size_groups (BtkWidget *widget)
{
  return g_object_get_qdata (B_OBJECT (widget), size_groups_quark);
}

static void
set_size_groups (BtkWidget *widget,
		 GSList    *groups)
{
  g_object_set_qdata (B_OBJECT (widget), size_groups_quark, groups);
}

static void
mark_visited (gpointer object)
{
  g_object_set_qdata (object, visited_quark, "visited");
}

static void
mark_unvisited (gpointer object)
{
  g_object_set_qdata (object, visited_quark, NULL);
}

static gboolean
is_visited (gpointer object)
{
  return g_object_get_qdata (object, visited_quark) != NULL;
}

static void
add_group_to_closure (BtkSizeGroup    *group,
		      BtkSizeGroupMode mode,
		      GSList         **groups,
		      GSList         **widgets)
{
  GSList *tmp_widgets;
  
  *groups = b_slist_prepend (*groups, group);
  mark_visited (group);

  tmp_widgets = group->widgets;
  while (tmp_widgets)
    {
      BtkWidget *tmp_widget = tmp_widgets->data;
      
      if (!is_visited (tmp_widget))
	add_widget_to_closure (tmp_widget, mode, groups, widgets);
      
      tmp_widgets = tmp_widgets->next;
    }
}

static void
add_widget_to_closure (BtkWidget       *widget,
		       BtkSizeGroupMode mode,
		       GSList         **groups,
		       GSList         **widgets)
{
  GSList *tmp_groups;

  *widgets = b_slist_prepend (*widgets, widget);
  mark_visited (widget);

  tmp_groups = get_size_groups (widget);
  while (tmp_groups)
    {
      BtkSizeGroup *tmp_group = tmp_groups->data;
      
      if ((tmp_group->mode == BTK_SIZE_GROUP_BOTH || tmp_group->mode == mode) &&
	  !is_visited (tmp_group))
	add_group_to_closure (tmp_group, mode, groups, widgets);

      tmp_groups = tmp_groups->next;
    }
}

static void
real_queue_resize (BtkWidget *widget)
{
  BTK_PRIVATE_SET_FLAG (widget, BTK_ALLOC_NEEDED);
  BTK_PRIVATE_SET_FLAG (widget, BTK_REQUEST_NEEDED);
  
  if (widget->parent)
    _btk_container_queue_resize (BTK_CONTAINER (widget->parent));
  else if (btk_widget_is_toplevel (widget) && BTK_IS_CONTAINER (widget))
    _btk_container_queue_resize (BTK_CONTAINER (widget));
}

static void
reset_group_sizes (GSList *groups)
{
  GSList *tmp_list = groups;
  while (tmp_list)
    {
      BtkSizeGroup *tmp_group = tmp_list->data;

      tmp_group->have_width = FALSE;
      tmp_group->have_height = FALSE;
      
      tmp_list = tmp_list->next;
    }
}

static void
queue_resize_on_widget (BtkWidget *widget,
			gboolean   check_siblings)
{
  BtkWidget *parent = widget;
  GSList *tmp_list;

  while (parent)
    {
      GSList *widget_groups;
      GSList *groups;
      GSList *widgets;
      
      if (widget == parent && !check_siblings)
	{
	  real_queue_resize (widget);
	  parent = parent->parent;
	  continue;
	}
      
      widget_groups = get_size_groups (parent);
      if (!widget_groups)
	{
	  if (widget == parent)
	    real_queue_resize (widget);

	  parent = parent->parent;
	  continue;
	}

      groups = NULL;
      widgets = NULL;
	  
      add_widget_to_closure (parent, BTK_SIZE_GROUP_HORIZONTAL, &groups, &widgets);
      b_slist_foreach (widgets, (GFunc)mark_unvisited, NULL);
      b_slist_foreach (groups, (GFunc)mark_unvisited, NULL);

      reset_group_sizes (groups);
	      
      tmp_list = widgets;
      while (tmp_list)
	{
	  if (tmp_list->data == parent)
	    {
	      if (widget == parent)
		real_queue_resize (parent);
	    }
	  else if (tmp_list->data == widget)
            {
              g_warning ("A container and its child are part of this SizeGroup");
            }
	  else
	    queue_resize_on_widget (tmp_list->data, FALSE);

	  tmp_list = tmp_list->next;
	}
      
      b_slist_free (widgets);
      b_slist_free (groups);
	      
      groups = NULL;
      widgets = NULL;
	      
      add_widget_to_closure (parent, BTK_SIZE_GROUP_VERTICAL, &groups, &widgets);
      b_slist_foreach (widgets, (GFunc)mark_unvisited, NULL);
      b_slist_foreach (groups, (GFunc)mark_unvisited, NULL);

      reset_group_sizes (groups);
	      
      tmp_list = widgets;
      while (tmp_list)
	{
	  if (tmp_list->data == parent)
	    {
	      if (widget == parent)
		real_queue_resize (parent);
	    }
	  else if (tmp_list->data == widget)
            {
              g_warning ("A container and its child are part of this SizeGroup");
            }
	  else
	    queue_resize_on_widget (tmp_list->data, FALSE);

	  tmp_list = tmp_list->next;
	}
      
      b_slist_free (widgets);
      b_slist_free (groups);
      
      parent = parent->parent;
    }
}

static void
queue_resize_on_group (BtkSizeGroup *size_group)
{
  if (size_group->widgets)
    queue_resize_on_widget (size_group->widgets->data, TRUE);
}

static void
initialize_size_group_quarks (void)
{
  if (!size_groups_quark)
    {
      size_groups_quark = g_quark_from_static_string (size_groups_tag);
      visited_quark = g_quark_from_static_string (visited_tag);
    }
}

static void
btk_size_group_class_init (BtkSizeGroupClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);

  bobject_class->set_property = btk_size_group_set_property;
  bobject_class->get_property = btk_size_group_get_property;
  
  g_object_class_install_property (bobject_class,
				   PROP_MODE,
				   g_param_spec_enum ("mode",
						      P_("Mode"),
						      P_("The directions in which the size group affects the requested sizes"
							" of its component widgets"),
						      BTK_TYPE_SIZE_GROUP_MODE,
						      BTK_SIZE_GROUP_HORIZONTAL,						      BTK_PARAM_READWRITE));

  /**
   * BtkSizeGroup:ignore-hidden:
   *
   * If %TRUE, unmapped widgets are ignored when determining 
   * the size of the group.
   *
   * Since: 2.8
   */
  g_object_class_install_property (bobject_class,
				   PROP_IGNORE_HIDDEN,
				   g_param_spec_boolean ("ignore-hidden",
							 P_("Ignore hidden"),
							 P_("If TRUE, unmapped widgets are ignored "
							    "when determining the size of the group"),
							 FALSE,
							 BTK_PARAM_READWRITE));
  
  initialize_size_group_quarks ();
}

static void
btk_size_group_init (BtkSizeGroup *size_group)
{
  size_group->widgets = NULL;
  size_group->mode = BTK_SIZE_GROUP_HORIZONTAL;
  size_group->have_width = 0;
  size_group->have_height = 0;
  size_group->ignore_hidden = 0;
}

static void
btk_size_group_buildable_init (BtkBuildableIface *iface)
{
  iface->custom_tag_start = btk_size_group_buildable_custom_tag_start;
  iface->custom_finished = btk_size_group_buildable_custom_finished;
}

G_DEFINE_TYPE_WITH_CODE (BtkSizeGroup, btk_size_group, B_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
						btk_size_group_buildable_init))

static void
btk_size_group_set_property (BObject      *object,
			     guint         prop_id,
			     const BValue *value,
			     BParamSpec   *pspec)
{
  BtkSizeGroup *size_group = BTK_SIZE_GROUP (object);

  switch (prop_id)
    {
    case PROP_MODE:
      btk_size_group_set_mode (size_group, b_value_get_enum (value));
      break;
    case PROP_IGNORE_HIDDEN:
      btk_size_group_set_ignore_hidden (size_group, b_value_get_boolean (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_size_group_get_property (BObject      *object,
			     guint         prop_id,
			     BValue       *value,
			     BParamSpec   *pspec)
{
  BtkSizeGroup *size_group = BTK_SIZE_GROUP (object);

  switch (prop_id)
    {
    case PROP_MODE:
      b_value_set_enum (value, size_group->mode);
      break;
    case PROP_IGNORE_HIDDEN:
      b_value_set_boolean (value, size_group->ignore_hidden);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * btk_size_group_new:
 * @mode: the mode for the new size group.
 * 
 * Create a new #BtkSizeGroup.
 
 * Return value: a newly created #BtkSizeGroup
 **/
BtkSizeGroup *
btk_size_group_new (BtkSizeGroupMode mode)
{
  BtkSizeGroup *size_group = g_object_new (BTK_TYPE_SIZE_GROUP, NULL);

  size_group->mode = mode;

  return size_group;
}

/**
 * btk_size_group_set_mode:
 * @size_group: a #BtkSizeGroup
 * @mode: the mode to set for the size group.
 * 
 * Sets the #BtkSizeGroupMode of the size group. The mode of the size
 * group determines whether the widgets in the size group should
 * all have the same horizontal requisition (%BTK_SIZE_GROUP_MODE_HORIZONTAL)
 * all have the same vertical requisition (%BTK_SIZE_GROUP_MODE_VERTICAL),
 * or should all have the same requisition in both directions
 * (%BTK_SIZE_GROUP_MODE_BOTH).
 **/
void
btk_size_group_set_mode (BtkSizeGroup     *size_group,
			 BtkSizeGroupMode  mode)
{
  g_return_if_fail (BTK_IS_SIZE_GROUP (size_group));

  if (size_group->mode != mode)
    {
      if (size_group->mode != BTK_SIZE_GROUP_NONE)
	queue_resize_on_group (size_group);
      size_group->mode = mode;
      if (size_group->mode != BTK_SIZE_GROUP_NONE)
	queue_resize_on_group (size_group);

      g_object_notify (B_OBJECT (size_group), "mode");
    }
}

/**
 * btk_size_group_get_mode:
 * @size_group: a #BtkSizeGroup
 * 
 * Gets the current mode of the size group. See btk_size_group_set_mode().
 * 
 * Return value: the current mode of the size group.
 **/
BtkSizeGroupMode
btk_size_group_get_mode (BtkSizeGroup *size_group)
{
  g_return_val_if_fail (BTK_IS_SIZE_GROUP (size_group), BTK_SIZE_GROUP_BOTH);

  return size_group->mode;
}

/**
 * btk_size_group_set_ignore_hidden:
 * @size_group: a #BtkSizeGroup
 * @ignore_hidden: whether unmapped widgets should be ignored
 *   when calculating the size
 * 
 * Sets whether unmapped widgets should be ignored when
 * calculating the size.
 *
 * Since: 2.8 
 */
void
btk_size_group_set_ignore_hidden (BtkSizeGroup *size_group,
				  gboolean      ignore_hidden)
{
  g_return_if_fail (BTK_IS_SIZE_GROUP (size_group));
  
  ignore_hidden = ignore_hidden != FALSE;

  if (size_group->ignore_hidden != ignore_hidden)
    {
      size_group->ignore_hidden = ignore_hidden;

      g_object_notify (B_OBJECT (size_group), "ignore-hidden");
    }
}

/**
 * btk_size_group_get_ignore_hidden:
 * @size_group: a #BtkSizeGroup
 *
 * Returns if invisible widgets are ignored when calculating the size.
 *
 * Returns: %TRUE if invisible widgets are ignored.
 *
 * Since: 2.8
 */
gboolean
btk_size_group_get_ignore_hidden (BtkSizeGroup *size_group)
{
  g_return_val_if_fail (BTK_IS_SIZE_GROUP (size_group), FALSE);

  return size_group->ignore_hidden;
}

static void
btk_size_group_widget_destroyed (BtkWidget    *widget,
				 BtkSizeGroup *size_group)
{
  btk_size_group_remove_widget (size_group, widget);
}

/**
 * btk_size_group_add_widget:
 * @size_group: a #BtkSizeGroup
 * @widget: the #BtkWidget to add
 * 
 * Adds a widget to a #BtkSizeGroup. In the future, the requisition
 * of the widget will be determined as the maximum of its requisition
 * and the requisition of the other widgets in the size group.
 * Whether this applies horizontally, vertically, or in both directions
 * depends on the mode of the size group. See btk_size_group_set_mode().
 *
 * When the widget is destroyed or no longer referenced elsewhere, it will 
 * be removed from the size group.
 */
void
btk_size_group_add_widget (BtkSizeGroup     *size_group,
			   BtkWidget        *widget)
{
  GSList *groups;
  
  g_return_if_fail (BTK_IS_SIZE_GROUP (size_group));
  g_return_if_fail (BTK_IS_WIDGET (widget));
  
  groups = get_size_groups (widget);

  if (!b_slist_find (groups, size_group))
    {
      groups = b_slist_prepend (groups, size_group);
      set_size_groups (widget, groups);

      size_group->widgets = b_slist_prepend (size_group->widgets, widget);

      g_signal_connect (widget, "destroy",
			G_CALLBACK (btk_size_group_widget_destroyed),
			size_group);

      g_object_ref (size_group);
    }
  
  queue_resize_on_group (size_group);
}

/**
 * btk_size_group_remove_widget:
 * @size_group: a #BtkSizeGrup
 * @widget: the #BtkWidget to remove
 * 
 * Removes a widget from a #BtkSizeGroup.
 **/
void
btk_size_group_remove_widget (BtkSizeGroup *size_group,
			      BtkWidget    *widget)
{
  GSList *groups;
  
  g_return_if_fail (BTK_IS_SIZE_GROUP (size_group));
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (b_slist_find (size_group->widgets, widget));

  g_signal_handlers_disconnect_by_func (widget,
					btk_size_group_widget_destroyed,
					size_group);
  
  groups = get_size_groups (widget);
  groups = b_slist_remove (groups, size_group);
  set_size_groups (widget, groups);

  size_group->widgets = b_slist_remove (size_group->widgets, widget);
  queue_resize_on_group (size_group);
  btk_widget_queue_resize (widget);

  g_object_unref (size_group);
}

/**
 * btk_size_group_get_widgets:
 * @size_group: a #BtkSizeGrup
 * 
 * Returns the list of widgets associated with @size_group.
 *
 * Return value:  (element-type BtkWidget) (transfer none): a #GSList of
 *   widgets. The list is owned by BTK+ and should not be modified.
 *
 * Since: 2.10
 **/
GSList *
btk_size_group_get_widgets (BtkSizeGroup *size_group)
{
  return size_group->widgets;
}

static gint
get_base_dimension (BtkWidget        *widget,
		    BtkSizeGroupMode  mode)
{
  BtkWidgetAuxInfo *aux_info = _btk_widget_get_aux_info (widget, FALSE);

  if (mode == BTK_SIZE_GROUP_HORIZONTAL)
    {
      if (aux_info && aux_info->width > 0)
	return aux_info->width;
      else
	return widget->requisition.width;
    }
  else
    {
      if (aux_info && aux_info->height > 0)
	return aux_info->height;
      else
	return widget->requisition.height;
    }
}

static void
do_size_request (BtkWidget *widget)
{
  if (BTK_WIDGET_REQUEST_NEEDED (widget))
    {
      btk_widget_ensure_style (widget);      
      BTK_PRIVATE_UNSET_FLAG (widget, BTK_REQUEST_NEEDED);
      g_signal_emit_by_name (widget,
			     "size-request",
			     &widget->requisition);
    }
}

static gint
compute_base_dimension (BtkWidget        *widget,
			BtkSizeGroupMode  mode)
{
  do_size_request (widget);

  return get_base_dimension (widget, mode);
}

static gint
compute_dimension (BtkWidget        *widget,
		   BtkSizeGroupMode  mode)
{
  GSList *widgets = NULL;
  GSList *groups = NULL;
  GSList *tmp_list;
  gint result = 0;

  add_widget_to_closure (widget, mode, &groups, &widgets);

  b_slist_foreach (widgets, (GFunc)mark_unvisited, NULL);
  b_slist_foreach (groups, (GFunc)mark_unvisited, NULL);
  
  b_slist_foreach (widgets, (GFunc)g_object_ref, NULL);
  
  if (!groups)
    {
      result = compute_base_dimension (widget, mode);
    }
  else
    {
      BtkSizeGroup *group = groups->data;

      if (mode == BTK_SIZE_GROUP_HORIZONTAL && group->have_width)
	result = group->requisition.width;
      else if (mode == BTK_SIZE_GROUP_VERTICAL && group->have_height)
	result = group->requisition.height;
      else
	{
	  tmp_list = widgets;
	  while (tmp_list)
	    {
	      BtkWidget *tmp_widget = tmp_list->data;

	      gint dimension = compute_base_dimension (tmp_widget, mode);

	      if (btk_widget_get_mapped (tmp_widget) || !group->ignore_hidden)
		{
		  if (dimension > result)
		    result = dimension;
		}

	      tmp_list = tmp_list->next;
	    }

	  tmp_list = groups;
	  while (tmp_list)
	    {
	      BtkSizeGroup *tmp_group = tmp_list->data;

	      if (mode == BTK_SIZE_GROUP_HORIZONTAL)
		{
		  tmp_group->have_width = TRUE;
		  tmp_group->requisition.width = result;
		}
	      else
		{
		  tmp_group->have_height = TRUE;
		  tmp_group->requisition.height = result;
		}
	      
	      tmp_list = tmp_list->next;
	    }
	}
    }

  b_slist_foreach (widgets, (GFunc)g_object_unref, NULL);

  b_slist_free (widgets);
  b_slist_free (groups);

  return result;
}

static gint
get_dimension (BtkWidget        *widget,
	       BtkSizeGroupMode  mode)
{
  GSList *widgets = NULL;
  GSList *groups = NULL;
  gint result = 0;

  add_widget_to_closure (widget, mode, &groups, &widgets);

  b_slist_foreach (widgets, (GFunc)mark_unvisited, NULL);
  b_slist_foreach (groups, (GFunc)mark_unvisited, NULL);  

  if (!groups)
    {
      result = get_base_dimension (widget, mode);
    }
  else
    {
      BtkSizeGroup *group = groups->data;

      if (mode == BTK_SIZE_GROUP_HORIZONTAL && group->have_width)
	result = group->requisition.width;
      else if (mode == BTK_SIZE_GROUP_VERTICAL && group->have_height)
	result = group->requisition.height;
    }

  b_slist_free (widgets);
  b_slist_free (groups);

  return result;
}

static void
get_fast_child_requisition (BtkWidget      *widget,
			    BtkRequisition *requisition)
{
  BtkWidgetAuxInfo *aux_info = _btk_widget_get_aux_info (widget, FALSE);
  
  *requisition = widget->requisition;
  
  if (aux_info)
    {
      if (aux_info->width > 0)
	requisition->width = aux_info->width;
      if (aux_info && aux_info->height > 0)
	requisition->height = aux_info->height;
    }
}

/**
 * _btk_size_group_get_child_requisition:
 * @widget: a #BtkWidget
 * @requisition: location to store computed requisition.
 * 
 * Retrieve the "child requisition" of the widget, taking account grouping
 * of the widget's requisition with other widgets.
 **/
void
_btk_size_group_get_child_requisition (BtkWidget      *widget,
				       BtkRequisition *requisition)
{
  initialize_size_group_quarks ();

  if (requisition)
    {
      if (get_size_groups (widget))
	{
	  requisition->width = get_dimension (widget, BTK_SIZE_GROUP_HORIZONTAL);
	  requisition->height = get_dimension (widget, BTK_SIZE_GROUP_VERTICAL);

	  /* Only do the full computation if we actually have size groups */
	}
      else
	get_fast_child_requisition (widget, requisition);
    }
}

/**
 * _btk_size_group_compute_requisition:
 * @widget: a #BtkWidget
 * @requisition: location to store computed requisition.
 * 
 * Compute the requisition of a widget taking into account grouping of
 * the widget's requisition with other widgets.
 **/
void
_btk_size_group_compute_requisition (BtkWidget      *widget,
				     BtkRequisition *requisition)
{
  gint width;
  gint height;

  initialize_size_group_quarks ();

  if (get_size_groups (widget))
    {
      /* Only do the full computation if we actually have size groups */
      
      width = compute_dimension (widget, BTK_SIZE_GROUP_HORIZONTAL);
      height = compute_dimension (widget, BTK_SIZE_GROUP_VERTICAL);

      if (requisition)
	{
	  requisition->width = width;
	  requisition->height = height;
	}
    }
  else
    {
      do_size_request (widget);
      
      if (requisition)
	get_fast_child_requisition (widget, requisition);
    }
}

/**
 * _btk_size_group_queue_resize:
 * @widget: a #BtkWidget
 * 
 * Queue a resize on a widget, and on all other widgets grouped with this widget.
 **/
void
_btk_size_group_queue_resize (BtkWidget *widget)
{
  initialize_size_group_quarks ();

  queue_resize_on_widget (widget, TRUE);
}

typedef struct {
  BObject *object;
  GSList *items;
} GSListSubParserData;

static void
size_group_start_element (GMarkupParseContext *context,
			  const gchar         *element_name,
			  const gchar        **names,
			  const gchar        **values,
			  gpointer            user_data,
			  GError            **error)
{
  guint i;
  GSListSubParserData *data = (GSListSubParserData*)user_data;

  if (strcmp (element_name, "widget") == 0)
    for (i = 0; names[i]; i++)
      if (strcmp (names[i], "name") == 0)
	data->items = b_slist_prepend (data->items, g_strdup (values[i]));
  else if (strcmp (element_name, "widgets") == 0)
    return;
  else
    g_warning ("Unsupported type tag for BtkSizeGroup: %s\n",
	       element_name);

}

static const GMarkupParser size_group_parser =
  {
    size_group_start_element
  };

static gboolean
btk_size_group_buildable_custom_tag_start (BtkBuildable  *buildable,
					   BtkBuilder    *builder,
					   BObject       *child,
					   const gchar   *tagname,
					   GMarkupParser *parser,
					   gpointer      *data)
{
  GSListSubParserData *parser_data;

  if (child)
    return FALSE;

  if (strcmp (tagname, "widgets") == 0)
    {
      parser_data = g_slice_new0 (GSListSubParserData);
      parser_data->items = NULL;
      parser_data->object = B_OBJECT (buildable);

      *parser = size_group_parser;
      *data = parser_data;
      return TRUE;
    }

  return FALSE;
}

static void
btk_size_group_buildable_custom_finished (BtkBuildable  *buildable,
					  BtkBuilder    *builder,
					  BObject       *child,
					  const gchar   *tagname,
					  gpointer       user_data)
{
  GSList *l;
  GSListSubParserData *data;
  BObject *object;

  if (strcmp (tagname, "widgets"))
    return;
  
  data = (GSListSubParserData*)user_data;
  data->items = b_slist_reverse (data->items);

  for (l = data->items; l; l = l->next)
    {
      object = btk_builder_get_object (builder, l->data);
      if (!object)
	{
	  g_warning ("Unknown object %s specified in sizegroup %s",
		     (const gchar*)l->data,
		     btk_buildable_get_name (BTK_BUILDABLE (data->object)));
	  continue;
	}
      btk_size_group_add_widget (BTK_SIZE_GROUP (data->object),
				 BTK_WIDGET (object));
      g_free (l->data);
    }
  b_slist_free (data->items);
  g_slice_free (GSListSubParserData, data);
}


#define __BTK_SIZE_GROUP_C__
#include "btkaliasdef.c"
