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

#undef BTK_DISABLE_DEPRECATED

#include <btk/btk.h>
#ifdef BDK_WINDOWING_X11
#include <bdk/x11/bdkx.h>
#endif
#include "bailwidget.h"
#include "bailnotebookpage.h"
#include "bail-private-macros.h"

extern BtkWidget *focus_widget;

static void bail_widget_class_init (BailWidgetClass *klass);
static void bail_widget_init                     (BailWidget       *accessible);
static void bail_widget_connect_widget_destroyed (BtkAccessible    *accessible);
static void bail_widget_destroyed                (BtkWidget        *widget,
                                                  BtkAccessible    *accessible);

static const bchar* bail_widget_get_description (BatkObject *accessible);
static BatkObject* bail_widget_get_parent (BatkObject *accessible);
static BatkStateSet* bail_widget_ref_state_set (BatkObject *accessible);
static BatkRelationSet* bail_widget_ref_relation_set (BatkObject *accessible);
static bint bail_widget_get_index_in_parent (BatkObject *accessible);

static void batk_component_interface_init (BatkComponentIface *iface);

static buint    bail_widget_add_focus_handler
                                           (BatkComponent    *component,
                                            BatkFocusHandler handler);

static void     bail_widget_get_extents    (BatkComponent    *component,
                                            bint            *x,
                                            bint            *y,
                                            bint            *width,
                                            bint            *height,
                                            BatkCoordType    coord_type);

static void     bail_widget_get_size       (BatkComponent    *component,
                                            bint            *width,
                                            bint            *height);

static BatkLayer bail_widget_get_layer      (BatkComponent *component);

static bboolean bail_widget_grab_focus     (BatkComponent    *component);


static void     bail_widget_remove_focus_handler 
                                           (BatkComponent    *component,
                                            buint           handler_id);

static bboolean bail_widget_set_extents    (BatkComponent    *component,
                                            bint            x,
                                            bint            y,
                                            bint            width,
                                            bint            height,
                                            BatkCoordType    coord_type);

static bboolean bail_widget_set_position   (BatkComponent    *component,
                                            bint            x,
                                            bint            y,
                                            BatkCoordType    coord_type);

static bboolean bail_widget_set_size       (BatkComponent    *component,
                                            bint            width,
                                            bint            height);

static bint       bail_widget_map_btk            (BtkWidget     *widget);
static void       bail_widget_real_notify_btk    (BObject       *obj,
                                                  BParamSpec    *pspec);
static void       bail_widget_notify_btk         (BObject       *obj,
                                                  BParamSpec    *pspec);
static bboolean   bail_widget_focus_btk          (BtkWidget     *widget,
                                                  BdkEventFocus *event);
static bboolean   bail_widget_real_focus_btk     (BtkWidget     *widget,
                                                  BdkEventFocus *event);
static void       bail_widget_size_allocate_btk  (BtkWidget     *widget,
                                                  BtkAllocation *allocation);

static void       bail_widget_focus_event        (BatkObject     *obj,
                                                  bboolean      focus_in);

static void       bail_widget_real_initialize    (BatkObject     *obj,
                                                  bpointer      data);
static BtkWidget* bail_widget_find_viewport      (BtkWidget     *widget);
static bboolean   bail_widget_on_screen          (BtkWidget     *widget);
static bboolean   bail_widget_all_parents_visible(BtkWidget     *widget);

G_DEFINE_TYPE_WITH_CODE (BailWidget, bail_widget, BTK_TYPE_ACCESSIBLE,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_COMPONENT, batk_component_interface_init))

static void
bail_widget_class_init (BailWidgetClass *klass)
{
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);
  BtkAccessibleClass *accessible_class = BTK_ACCESSIBLE_CLASS (klass);

  klass->notify_btk = bail_widget_real_notify_btk;
  klass->focus_btk = bail_widget_real_focus_btk;

  accessible_class->connect_widget_destroyed = bail_widget_connect_widget_destroyed;

  class->get_description = bail_widget_get_description;
  class->get_parent = bail_widget_get_parent;
  class->ref_relation_set = bail_widget_ref_relation_set;
  class->ref_state_set = bail_widget_ref_state_set;
  class->get_index_in_parent = bail_widget_get_index_in_parent;
  class->initialize = bail_widget_real_initialize;
}

static void
bail_widget_init (BailWidget *accessible)
{
}

/**
 * This function  specifies the BtkWidget for which the BailWidget was created 
 * and specifies a handler to be called when the BtkWidget is destroyed.
 **/
static void 
bail_widget_real_initialize (BatkObject *obj,
                             bpointer  data)
{
  BtkAccessible *accessible;
  BtkWidget *widget;

  g_return_if_fail (BTK_IS_WIDGET (data));

  widget = BTK_WIDGET (data);

  accessible = BTK_ACCESSIBLE (obj);
  accessible->widget = widget;
  btk_accessible_connect_widget_destroyed (accessible);
  g_signal_connect_after (widget,
                          "focus-in-event",
                          G_CALLBACK (bail_widget_focus_btk),
                          NULL);
  g_signal_connect_after (widget,
                          "focus-out-event",
                          G_CALLBACK (bail_widget_focus_btk),
                          NULL);
  g_signal_connect (widget,
                    "notify",
                    G_CALLBACK (bail_widget_notify_btk),
                    NULL);
  g_signal_connect (widget,
                    "size_allocate",
                    G_CALLBACK (bail_widget_size_allocate_btk),
                    NULL);
  batk_component_add_focus_handler (BATK_COMPONENT (accessible),
                                   bail_widget_focus_event);
  /*
   * Add signal handlers for BTK signals required to support property changes
   */
  g_signal_connect (widget,
                    "map",
                    G_CALLBACK (bail_widget_map_btk),
                    NULL);
  g_signal_connect (widget,
                    "unmap",
                    G_CALLBACK (bail_widget_map_btk),
                    NULL);
  g_object_set_data (B_OBJECT (obj), "batk-component-layer",
		     BINT_TO_POINTER (BATK_LAYER_WIDGET));

  obj->role = BATK_ROLE_UNKNOWN;
}

BatkObject* 
bail_widget_new (BtkWidget *widget)
{
  BObject *object;
  BatkObject *accessible;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  object = g_object_new (BAIL_TYPE_WIDGET, NULL);

  accessible = BATK_OBJECT (object);
  batk_object_initialize (accessible, widget);

  return accessible;
}

/*
 * This function specifies the function to be called when the widget
 * is destroyed
 */
static void
bail_widget_connect_widget_destroyed (BtkAccessible *accessible)
{
  if (accessible->widget)
    {
      g_signal_connect_after (accessible->widget,
                              "destroy",
                              G_CALLBACK (bail_widget_destroyed),
                              accessible);
    }
}

/*
 * This function is called when the widget is destroyed.
 * It sets the widget field in the BtkAccessible structure to NULL
 * and emits a state-change signal for the state BATK_STATE_DEFUNCT
 */
static void 
bail_widget_destroyed (BtkWidget     *widget,
                       BtkAccessible *accessible)
{
  accessible->widget = NULL;
  batk_object_notify_state_change (BATK_OBJECT (accessible), BATK_STATE_DEFUNCT,
                                  TRUE);
}

static const bchar*
bail_widget_get_description (BatkObject *accessible)
{
  if (accessible->description)
    return accessible->description;
  else
    {
      /* Get the tooltip from the widget */
      BtkAccessible *obj = BTK_ACCESSIBLE (accessible);

      bail_return_val_if_fail (obj, NULL);

      if (obj->widget == NULL)
        /*
         * Object is defunct
         */
        return NULL;
 
      bail_return_val_if_fail (BTK_WIDGET (obj->widget), NULL);

      return btk_widget_get_tooltip_text (obj->widget);
    }
}

static BatkObject* 
bail_widget_get_parent (BatkObject *accessible)
{
  BatkObject *parent;

  parent = accessible->accessible_parent;

  if (parent != NULL)
    g_return_val_if_fail (BATK_IS_OBJECT (parent), NULL);
  else
    {
      BtkWidget *widget, *parent_widget;

      widget = BTK_ACCESSIBLE (accessible)->widget;
      if (widget == NULL)
        /*
         * State is defunct
         */
        return NULL;
      bail_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

      parent_widget = widget->parent;
      if (parent_widget == NULL)
        return NULL;

      /*
       * For a widget whose parent is a BtkNoteBook, we return the
       * accessible object corresponding the BtkNotebookPage containing
       * the widget as the accessible parent.
       */
      if (BTK_IS_NOTEBOOK (parent_widget))
        {
          bint page_num;
          BtkWidget *child;
          BtkNotebook *notebook;

          page_num = 0;
          notebook = BTK_NOTEBOOK (parent_widget);
          while (TRUE)
            {
              child = btk_notebook_get_nth_page (notebook, page_num);
              if (!child)
                break;
              if (child == widget)
                {
                  parent = btk_widget_get_accessible (parent_widget);
                  parent = batk_object_ref_accessible_child (parent, page_num);
                  g_object_unref (parent);
                  return parent;
                }
              page_num++;
            }
        }

      parent = btk_widget_get_accessible (parent_widget);
    }
  return parent;
}

static BtkWidget*
find_label (BtkWidget *widget)
{
  GList *labels;
  BtkWidget *label;
  BtkWidget *temp_widget;

  labels = btk_widget_list_mnemonic_labels (widget);
  label = NULL;
  if (labels)
    {
      if (labels->data)
        {
          if (labels->next)
            {
              g_warning ("Widget (%s) has more than one label", B_OBJECT_TYPE_NAME (widget));
              
            }
          else
            {
              label = labels->data;
            }
        }
      g_list_free (labels);
    }

  /*
   * Ignore a label within a button; bug #136602
   */
  if (label && BTK_IS_BUTTON (widget))
    {
      temp_widget = label;
      while (temp_widget)
        {
          if (temp_widget == widget)
            {
              label = NULL;
              break;
            }
          temp_widget = btk_widget_get_parent (temp_widget);
        }
    } 
  return label;
}

static BatkRelationSet*
bail_widget_ref_relation_set (BatkObject *obj)
{
  BtkWidget *widget;
  BatkRelationSet *relation_set;
  BtkWidget *label;
  BatkObject *array[1];
  BatkRelation* relation;

  bail_return_val_if_fail (BAIL_IS_WIDGET (obj), NULL);

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  relation_set = BATK_OBJECT_CLASS (bail_widget_parent_class)->ref_relation_set (obj);

  if (BTK_IS_BOX (widget) && !BTK_IS_COMBO (widget))
      /*
       * Do not report labelled-by for a BtkBox which could be a 
       * BunnyFileEntry.
       */
    return relation_set;

  if (!batk_relation_set_contains (relation_set, BATK_RELATION_LABELLED_BY))
    {
      label = find_label (widget);
      if (label == NULL)
        {
          if (BTK_IS_BUTTON (widget))
            /*
             * Handle the case where BunnyIconEntry is the mnemonic widget.
             * The BtkButton which is a grandchild of the BunnyIconEntry
             * should really be the mnemonic widget. See bug #133967.
             */
            {
              BtkWidget *temp_widget;

              temp_widget = btk_widget_get_parent (widget);

              if (BTK_IS_ALIGNMENT (temp_widget))
                {
                  temp_widget = btk_widget_get_parent (temp_widget);
                  if (BTK_IS_BOX (temp_widget))
                    {
                      label = find_label (temp_widget);
                 
                      if (!label)
                        label = find_label (btk_widget_get_parent (temp_widget));
                    }
                }
            }
          else if (BTK_IS_COMBO (widget))
            /*
             * Handle the case when BunnyFileEntry is the mnemonic widget.
             * The BunnyEntry which is a grandchild of the BunnyFileEntry
             * should be the mnemonic widget. See bug #137584.
             */
            {
              BtkWidget *temp_widget;

              temp_widget = btk_widget_get_parent (widget);

              if (BTK_IS_HBOX (temp_widget))
                {
                  temp_widget = btk_widget_get_parent (temp_widget);
                  if (BTK_IS_BOX (temp_widget))
                    {
                      label = find_label (temp_widget);
                    }
                }
            }
          else if (BTK_IS_COMBO_BOX (widget))
            /*
             * Handle the case when BtkFileChooserButton is the mnemonic
             * widget.  The BtkComboBox which is a child of the
             * BtkFileChooserButton should be the mnemonic widget.
             * See bug #359843.
             */
            {
              BtkWidget *temp_widget;

              temp_widget = btk_widget_get_parent (widget);
              if (BTK_IS_HBOX (temp_widget))
                {
                  label = find_label (temp_widget);
                }
            }
        }

      if (label)
        {
	  array [0] = btk_widget_get_accessible (label);

	  relation = batk_relation_new (array, 1, BATK_RELATION_LABELLED_BY);
	  batk_relation_set_add (relation_set, relation);
	  g_object_unref (relation);
        }
    }

  return relation_set;
}

static BatkStateSet*
bail_widget_ref_state_set (BatkObject *accessible)
{
  BtkWidget *widget = BTK_ACCESSIBLE (accessible)->widget;
  BatkStateSet *state_set;

  state_set = BATK_OBJECT_CLASS (bail_widget_parent_class)->ref_state_set (accessible);

  if (widget == NULL)
    {
      batk_state_set_add_state (state_set, BATK_STATE_DEFUNCT);
    }
  else
    {
      if (btk_widget_is_sensitive (widget))
        {
          batk_state_set_add_state (state_set, BATK_STATE_SENSITIVE);
          batk_state_set_add_state (state_set, BATK_STATE_ENABLED);
        }
  
      if (btk_widget_get_can_focus (widget))
        {
          batk_state_set_add_state (state_set, BATK_STATE_FOCUSABLE);
        }
      /*
       * We do not currently generate notifications when an BATK object 
       * corresponding to a BtkWidget changes visibility by being scrolled 
       * on or off the screen.  The testcase for this is the main window 
       * of the testbtk application in which a set of buttons in a BtkVBox 
       * is in a scrooled window with a viewport.
       *
       * To generate the notifications we would need to do the following: 
       * 1) Find the BtkViewPort among the antecendents of the objects
       * 2) Create an accesible for the BtkViewPort
       * 3) Connect to the value-changed signal on the viewport
       * 4) When the signal is received we need to traverse the children 
       * of the viewport and check whether the children are visible or not 
       * visible; we may want to restrict this to the widgets for which 
       * accessible objects have been created.
       * 5) We probably need to store a variable on_screen in the 
       * BailWidget data structure so we can determine whether the value has 
       * changed.
       */
      if (btk_widget_get_visible (widget))
        {
          batk_state_set_add_state (state_set, BATK_STATE_VISIBLE);
          if (bail_widget_on_screen (widget) && btk_widget_get_mapped (widget) &&
              bail_widget_all_parents_visible (widget))
            {
              batk_state_set_add_state (state_set, BATK_STATE_SHOWING);
            }
        }
  
      if (btk_widget_has_focus (widget) && (widget == focus_widget))
        {
          BatkObject *focus_obj;

          focus_obj = g_object_get_data (B_OBJECT (accessible), "bail-focus-object");
          if (focus_obj == NULL)
            batk_state_set_add_state (state_set, BATK_STATE_FOCUSED);
        }
      if (btk_widget_has_default (widget))
        {
          batk_state_set_add_state (state_set, BATK_STATE_DEFAULT);
        }
    }
  return state_set;
}

static bint
bail_widget_get_index_in_parent (BatkObject *accessible)
{
  BtkWidget *widget;
  BtkWidget *parent_widget;
  bint index;
  GList *children;
  GType type;

  type = g_type_from_name ("BailCanvasWidget");
  widget = BTK_ACCESSIBLE (accessible)->widget;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return -1;

  if (accessible->accessible_parent)
    {
      BatkObject *parent;

      parent = accessible->accessible_parent;

      if (BAIL_IS_NOTEBOOK_PAGE (parent) ||
          B_TYPE_CHECK_INSTANCE_TYPE ((parent), type))
        return 0;
      else
        {
          bint n_children, i;
          bboolean found = FALSE;

          n_children = batk_object_get_n_accessible_children (parent);
          for (i = 0; i < n_children; i++)
            {
              BatkObject *child;

              child = batk_object_ref_accessible_child (parent, i);
              if (child == accessible)
                found = TRUE;

              g_object_unref (child); 
              if (found)
                return i;
            }
        }
    }

  bail_return_val_if_fail (BTK_IS_WIDGET (widget), -1);
  parent_widget = widget->parent;
  if (parent_widget == NULL)
    return -1;
  bail_return_val_if_fail (BTK_IS_CONTAINER (parent_widget), -1);

  children = btk_container_get_children (BTK_CONTAINER (parent_widget));

  index = g_list_index (children, widget);
  g_list_free (children);
  return index;  
}

static void 
batk_component_interface_init (BatkComponentIface *iface)
{
  /*
   * Use default implementation for contains and get_position
   */
  iface->add_focus_handler = bail_widget_add_focus_handler;
  iface->get_extents = bail_widget_get_extents;
  iface->get_size = bail_widget_get_size;
  iface->get_layer = bail_widget_get_layer;
  iface->grab_focus = bail_widget_grab_focus;
  iface->remove_focus_handler = bail_widget_remove_focus_handler;
  iface->set_extents = bail_widget_set_extents;
  iface->set_position = bail_widget_set_position;
  iface->set_size = bail_widget_set_size;
}

static buint 
bail_widget_add_focus_handler (BatkComponent    *component,
                               BatkFocusHandler handler)
{
  GSignalMatchType match_type;
  bulong ret;
  buint signal_id;

  match_type = G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC;
  signal_id = g_signal_lookup ("focus-event", BATK_TYPE_OBJECT);

  ret = g_signal_handler_find (component, match_type, signal_id, 0, NULL,
                               (bpointer) handler, NULL);
  if (!ret)
    {
      return g_signal_connect_closure_by_id (component, 
                                             signal_id, 0,
                                             g_cclosure_new (
                                             G_CALLBACK (handler), NULL,
                                             (GClosureNotify) NULL),
                                             FALSE);
    }
  else
    {
      return 0;
    }
}

static void 
bail_widget_get_extents (BatkComponent   *component,
                         bint           *x,
                         bint           *y,
                         bint           *width,
                         bint           *height,
                         BatkCoordType   coord_type)
{
  BdkWindow *window;
  bint x_window, y_window;
  bint x_toplevel, y_toplevel;
  BtkWidget *widget = BTK_ACCESSIBLE (component)->widget;

  if (widget == NULL)
    /*
     * Object is defunct
     */
    return;

  bail_return_if_fail (BTK_IS_WIDGET (widget));

  *width = widget->allocation.width;
  *height = widget->allocation.height;
  if (!bail_widget_on_screen (widget) || (!btk_widget_is_drawable (widget)))
    {
      *x = B_MININT;
      *y = B_MININT;
      return;
    }

  if (widget->parent)
    {
      *x = widget->allocation.x;
      *y = widget->allocation.y;
      window = btk_widget_get_parent_window (widget);
    }
  else
    {
      *x = 0;
      *y = 0;
      window = widget->window;
    }
  bdk_window_get_origin (window, &x_window, &y_window);
  *x += x_window;
  *y += y_window;

 
 if (coord_type == BATK_XY_WINDOW) 
    { 
      window = bdk_window_get_toplevel (widget->window);
      bdk_window_get_origin (window, &x_toplevel, &y_toplevel);

      *x -= x_toplevel;
      *y -= y_toplevel;
    }
}

static void 
bail_widget_get_size (BatkComponent   *component,
                      bint           *width,
                      bint           *height)
{
  BtkWidget *widget = BTK_ACCESSIBLE (component)->widget;

  if (widget == NULL)
    /*
     * Object is defunct
     */
    return;

  bail_return_if_fail (BTK_IS_WIDGET (widget));

  *width = widget->allocation.width;
  *height = widget->allocation.height;
}

static BatkLayer
bail_widget_get_layer (BatkComponent *component)
{
  bint layer;
  layer = BPOINTER_TO_INT (g_object_get_data (B_OBJECT (component), "batk-component-layer"));

  return (BatkLayer) layer;
}

static bboolean 
bail_widget_grab_focus (BatkComponent   *component)
{
  BtkWidget *widget = BTK_ACCESSIBLE (component)->widget;
  BtkWidget *toplevel;

  bail_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);
  if (btk_widget_get_can_focus (widget))
    {
      btk_widget_grab_focus (widget);
      toplevel = btk_widget_get_toplevel (widget);
      if (btk_widget_is_toplevel (toplevel))
	{
#ifdef BDK_WINDOWING_X11
	  btk_window_present_with_time (BTK_WINDOW (toplevel), bdk_x11_get_server_time (widget->window));
#else
	  btk_window_present (BTK_WINDOW (toplevel));
#endif
	}
      return TRUE;
    }
  else
    return FALSE;
}

static void 
bail_widget_remove_focus_handler (BatkComponent   *component,
                                  buint          handler_id)
{
  g_signal_handler_disconnect (component, handler_id);
}

static bboolean 
bail_widget_set_extents (BatkComponent   *component,
                         bint           x,
                         bint           y,
                         bint           width,
                         bint           height,
                         BatkCoordType   coord_type)
{
  BtkWidget *widget = BTK_ACCESSIBLE (component)->widget;

  if (widget == NULL)
    /*
     * Object is defunct
     */
    return FALSE;
  bail_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  if (btk_widget_is_toplevel (widget))
    {
      if (coord_type == BATK_XY_WINDOW)
        {
          bint x_current, y_current;
          BdkWindow *window = widget->window;

          bdk_window_get_origin (window, &x_current, &y_current);
          x_current += x;
          y_current += y;
          if (x_current < 0 || y_current < 0)
            return FALSE;
          else
            {
              btk_widget_set_uposition (widget, x_current, y_current);
              btk_widget_set_size_request (widget, width, height);
              return TRUE;
            }
        }
      else if (coord_type == BATK_XY_SCREEN)
        {  
          btk_widget_set_uposition (widget, x, y);
          btk_widget_set_size_request (widget, width, height);
          return TRUE;
        }
    }
  return FALSE;
}

static bboolean
bail_widget_set_position (BatkComponent   *component,
                          bint           x,
                          bint           y,
                          BatkCoordType   coord_type)
{
  BtkWidget *widget = BTK_ACCESSIBLE (component)->widget;

  if (widget == NULL)
    /*
     * Object is defunct
     */
    return FALSE;
  bail_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  if (btk_widget_is_toplevel (widget))
    {
      if (coord_type == BATK_XY_WINDOW)
        {
          bint x_current, y_current;
          BdkWindow *window = widget->window;

          bdk_window_get_origin (window, &x_current, &y_current);
          x_current += x;
          y_current += y;
          if (x_current < 0 || y_current < 0)
            return FALSE;
          else
            {
              btk_widget_set_uposition (widget, x_current, y_current);
              return TRUE;
            }
        }
      else if (coord_type == BATK_XY_SCREEN)
        {  
          btk_widget_set_uposition (widget, x, y);
          return TRUE;
        }
    }
  return FALSE;
}

static bboolean 
bail_widget_set_size (BatkComponent   *component,
                      bint           width,
                      bint           height)
{
  BtkWidget *widget = BTK_ACCESSIBLE (component)->widget;

  if (widget == NULL)
    /*
     * Object is defunct
     */
    return FALSE;
  bail_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  if (btk_widget_is_toplevel (widget))
    {
      btk_widget_set_size_request (widget, width, height);
      return TRUE;
    }
  else
   return FALSE;
}

/*
 * This function is a signal handler for notify_in_event and focus_out_event
 * signal which gets emitted on a BtkWidget.
 */
static bboolean
bail_widget_focus_btk (BtkWidget     *widget,
                       BdkEventFocus *event)
{
  BailWidget *bail_widget;
  BailWidgetClass *klass;

  bail_widget = BAIL_WIDGET (btk_widget_get_accessible (widget));
  klass = BAIL_WIDGET_GET_CLASS (bail_widget);
  if (klass->focus_btk)
    return klass->focus_btk (widget, event);
  else
    return FALSE;
}

/*
 * This function is the signal handler defined for focus_in_event and
 * focus_out_event got BailWidget.
 *
 * It emits a focus-event signal on the BailWidget.
 */
static bboolean
bail_widget_real_focus_btk (BtkWidget     *widget,
                            BdkEventFocus *event)
{
  BatkObject* accessible;
  bboolean return_val;
  return_val = FALSE;

  accessible = btk_widget_get_accessible (widget);
  g_signal_emit_by_name (accessible, "focus_event", event->in, &return_val);
  return FALSE;
}

static void
bail_widget_size_allocate_btk (BtkWidget     *widget,
                               BtkAllocation *allocation)
{
  BatkObject* accessible;
  BatkRectangle rect;

  accessible = btk_widget_get_accessible (widget);
  if (BATK_IS_COMPONENT (accessible))
    {
      rect.x = allocation->x;
      rect.y = allocation->y;
      rect.width = allocation->width;
      rect.height = allocation->height;
      g_signal_emit_by_name (accessible, "bounds_changed", &rect);
    }
}

/*
 * This function is the signal handler defined for map and unmap signals.
 */
static bint
bail_widget_map_btk (BtkWidget     *widget)
{
  BatkObject* accessible;

  accessible = btk_widget_get_accessible (widget);
  batk_object_notify_state_change (accessible, BATK_STATE_SHOWING,
                                  btk_widget_get_mapped (widget));
  return 1;
}

/*
 * This function is a signal handler for notify signal which gets emitted 
 * when a property changes value on the BtkWidget associated with the object.
 *
 * It calls a function for the BailWidget type
 */
static void 
bail_widget_notify_btk (BObject     *obj,
                        BParamSpec  *pspec)
{
  BailWidget *widget;
  BailWidgetClass *klass;

  widget = BAIL_WIDGET (btk_widget_get_accessible (BTK_WIDGET (obj)));
  klass = BAIL_WIDGET_GET_CLASS (widget);
  if (klass->notify_btk)
    klass->notify_btk (obj, pspec);
}

/*
 * This function is a signal handler for notify signal which gets emitted 
 * when a property changes value on the BtkWidget associated with a BailWidget.
 *
 * It constructs an BatkPropertyValues structure and emits a "property_changed"
 * signal which causes the user specified BatkPropertyChangeHandler
 * to be called.
 */
static void 
bail_widget_real_notify_btk (BObject     *obj,
                             BParamSpec  *pspec)
{
  BtkWidget* widget = BTK_WIDGET (obj);
  BatkObject* batk_obj = btk_widget_get_accessible (widget);
  BatkState state;
  bboolean value;

  if (strcmp (pspec->name, "has-focus") == 0)
    /*
     * We use focus-in-event and focus-out-event signals to catch
     * focus changes so we ignore this.
     */
    return;
  else if (batk_obj->description == NULL &&
           strcmp (pspec->name, "tooltip-text") == 0)
    {
      g_object_notify (B_OBJECT (batk_obj), "accessible-description");
      return;
    }
  else if (strcmp (pspec->name, "visible") == 0)
    {
      state = BATK_STATE_VISIBLE;
      value = btk_widget_get_visible (widget);
    }
  else if (strcmp (pspec->name, "sensitive") == 0)
    {
      state = BATK_STATE_SENSITIVE;
      value = btk_widget_get_sensitive (widget);
    }
  else
    return;

  batk_object_notify_state_change (batk_obj, state, value);
  if (state == BATK_STATE_SENSITIVE)
    batk_object_notify_state_change (batk_obj, BATK_STATE_ENABLED, value);

}

static void 
bail_widget_focus_event (BatkObject   *obj,
                         bboolean    focus_in)
{
  BatkObject *focus_obj;

  focus_obj = g_object_get_data (B_OBJECT (obj), "bail-focus-object");
  if (focus_obj == NULL)
    focus_obj = obj;
  batk_object_notify_state_change (focus_obj, BATK_STATE_FOCUSED, focus_in);
}

static BtkWidget*
bail_widget_find_viewport (BtkWidget *widget)
{
  /*
   * Find an antecedent which is a BtkViewPort
   */
  BtkWidget *parent;

  parent = widget->parent;
  while (parent != NULL)
    {
      if (BTK_IS_VIEWPORT (parent))
        break;
      parent = parent->parent;
    }
  return parent;
}

/*
 * This function checks whether the widget has an antecedent which is 
 * a BtkViewport and, if so, whether any part of the widget intersects
 * the visible rectangle of the BtkViewport.
 */ 
static bboolean bail_widget_on_screen (BtkWidget *widget)
{
  BtkWidget *viewport;
  bboolean return_value;

  viewport = bail_widget_find_viewport (widget);
  if (viewport)
    {
      BtkAdjustment *adjustment;
      BdkRectangle visible_rect;

      adjustment = btk_viewport_get_vadjustment (BTK_VIEWPORT (viewport));
      visible_rect.y = adjustment->value;
      adjustment = btk_viewport_get_hadjustment (BTK_VIEWPORT (viewport));
      visible_rect.x = adjustment->value;
      visible_rect.width = viewport->allocation.width;
      visible_rect.height = viewport->allocation.height;
             
      if (((widget->allocation.x + widget->allocation.width) < visible_rect.x) ||
         ((widget->allocation.y + widget->allocation.height) < visible_rect.y) ||
         (widget->allocation.x > (visible_rect.x + visible_rect.width)) ||
         (widget->allocation.y > (visible_rect.y + visible_rect.height)))
        return_value = FALSE;
      else
        return_value = TRUE;
    }
  else
    {
      /*
       * Check whether the widget has been placed of the screen. The
       * widget may be MAPPED as when toolbar items do not fit on the toolbar.
       */
      if (widget->allocation.x + widget->allocation.width <= 0 &&
          widget->allocation.y + widget->allocation.height <= 0)
        return_value = FALSE;
      else 
        return_value = TRUE;
    }

  return return_value;
}

/**
 * bail_widget_all_parents_visible:
 * @widget: a #BtkWidget
 *
 * Checks if all the predecesors (the parent widget, his parent, etc) are visible
 * Used to check properly the SHOWING state.
 *
 * Return value: TRUE if all the parent hierarchy is visible, FALSE otherwise
 **/
static bboolean bail_widget_all_parents_visible (BtkWidget *widget)
{
  BtkWidget *iter_parent = NULL;
  bboolean result = TRUE;

  for (iter_parent = btk_widget_get_parent (widget); iter_parent;
       iter_parent = btk_widget_get_parent (iter_parent))
    {
      if (!btk_widget_get_visible (iter_parent))
        {
          result = FALSE;
          break;
        }
    }

  return result;
}
