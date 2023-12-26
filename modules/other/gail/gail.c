/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2001 Sun Microsystems Inc.
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

#include <stdio.h>
#include <stdlib.h>

#undef BTK_DISABLE_DEPRECATED

#include <btk/btk.h>
#include "bail.h"
#include "bailfactory.h"

#define BUNNY_ACCESSIBILITY_ENV "BUNNY_ACCESSIBILITY"
#define NO_BAIL_ENV "NO_BAIL"

static gboolean bail_focus_watcher      (GSignalInvocationHint *ihint,
                                         guint                  n_param_values,
                                         const GValue          *param_values,
                                         gpointer               data);
static gboolean bail_select_watcher     (GSignalInvocationHint *ihint,
                                         guint                  n_param_values,
                                         const GValue          *param_values,
                                         gpointer               data);
static gboolean bail_deselect_watcher   (GSignalInvocationHint *ihint,
                                         guint                  n_param_values,
                                         const GValue          *param_values,
                                         gpointer               data);
static gboolean bail_switch_page_watcher(GSignalInvocationHint *ihint,
                                         guint                  n_param_values,
                                         const GValue          *param_values,
                                         gpointer               data);
static BatkObject* bail_get_accessible_for_widget (BtkWidget    *widget,
                                                  gboolean     *transient);
static void     bail_finish_select       (BtkWidget            *widget);
static void     bail_map_cb              (BtkWidget            *widget);
static void     bail_map_submenu_cb      (BtkWidget            *widget);
static gint     bail_focus_idle_handler  (gpointer             data);
static void     bail_focus_notify        (BtkWidget            *widget);
static void     bail_focus_notify_when_idle (BtkWidget            *widget);

static void     bail_focus_tracker_init (void);
static void     bail_focus_object_destroyed (gpointer data);
static void     bail_focus_tracker (BatkObject *object);
static void     bail_set_focus_widget (BtkWidget *focus_widget,
                                       BtkWidget *widget); 
static void     bail_set_focus_object (BatkObject *focus_obj,
                                       BatkObject *obj);

BtkWidget* focus_widget = NULL;
static BtkWidget* next_focus_widget = NULL;
static gboolean was_deselect = FALSE;
static BtkWidget* subsequent_focus_widget = NULL;
static BtkWidget* focus_before_menu = NULL;
static guint focus_notify_handler = 0;    
static guint focus_tracker_id = 0;
static GQuark quark_focus_object = 0;

BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_OBJECT, BailObject, bail_object, BTK_TYPE_OBJECT)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_WIDGET, BailWidget, bail_widget, BTK_TYPE_WIDGET)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_CONTAINER, BailContainer, bail_container, BTK_TYPE_CONTAINER)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_BUTTON, BailButton, bail_button, BTK_TYPE_BUTTON)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_ITEM, BailItem, bail_item, BTK_TYPE_ITEM)
BAIL_IMPLEMENT_FACTORY_WITH_FUNC (BAIL_TYPE_MENU_ITEM, BailMenuItem, bail_menu_item, bail_menu_item_new)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_TOGGLE_BUTTON, BailToggleButton, bail_toggle_button, BTK_TYPE_TOGGLE_BUTTON)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_IMAGE, BailImage, bail_image, BTK_TYPE_IMAGE)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_TEXT_VIEW, BailTextView, bail_text_view, BTK_TYPE_TEXT_VIEW)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_COMBO, BailCombo, bail_combo, BTK_TYPE_COMBO)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_COMBO_BOX, BailComboBox, bail_combo_box, BTK_TYPE_COMBO_BOX)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_ENTRY, BailEntry, bail_entry, BTK_TYPE_ENTRY)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_MENU_SHELL, BailMenuShell, bail_menu_shell, BTK_TYPE_MENU_SHELL)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_MENU, BailMenu, bail_menu, BTK_TYPE_MENU)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_WINDOW, BailWindow, bail_window, BTK_TYPE_BIN)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_RANGE, BailRange, bail_range, BTK_TYPE_RANGE)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_SCALE, BailScale, bail_scale, BTK_TYPE_SCALE)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_SCALE_BUTTON, BailScaleButton, bail_scale_button, BTK_TYPE_SCALE_BUTTON)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_CLIST, BailCList, bail_clist, BTK_TYPE_CLIST)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_LABEL, BailLabel, bail_label, BTK_TYPE_LABEL)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_STATUSBAR, BailStatusbar, bail_statusbar, BTK_TYPE_STATUSBAR)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_NOTEBOOK, BailNotebook, bail_notebook, BTK_TYPE_NOTEBOOK)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_CALENDAR, BailCalendar, bail_calendar, BTK_TYPE_CALENDAR)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_PROGRESS_BAR, BailProgressBar, bail_progress_bar, BTK_TYPE_PROGRESS_BAR)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_SPIN_BUTTON, BailSpinButton, bail_spin_button, BTK_TYPE_SPIN_BUTTON)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_TREE_VIEW, BailTreeView, bail_tree_view, BTK_TYPE_TREE_VIEW)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_FRAME, BailFrame, bail_frame, BTK_TYPE_FRAME)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_RADIO_BUTTON, BailRadioButton, bail_radio_button, BTK_TYPE_RADIO_BUTTON)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_ARROW, BailArrow, bail_arrow, BTK_TYPE_ARROW)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_PIXMAP, BailPixmap, bail_pixmap, BTK_TYPE_PIXMAP)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_SEPARATOR, BailSeparator, bail_separator, BTK_TYPE_SEPARATOR)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_BOX, BailBox, bail_box, BTK_TYPE_BOX)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_SCROLLED_WINDOW, BailScrolledWindow, bail_scrolled_window, BTK_TYPE_SCROLLED_WINDOW)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_LIST, BailList, bail_list, BTK_TYPE_LIST)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_PANED, BailPaned, bail_paned, BTK_TYPE_PANED)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_SCROLLBAR, BailScrollbar, bail_scrollbar, BTK_TYPE_SCROLLBAR)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_OPTION_MENU, BailOptionMenu, bail_option_menu, BTK_TYPE_OPTION_MENU)
BAIL_IMPLEMENT_FACTORY_WITH_FUNC (BAIL_TYPE_CHECK_MENU_ITEM, BailCheckMenuItem, bail_check_menu_item, bail_check_menu_item_new)
BAIL_IMPLEMENT_FACTORY_WITH_FUNC (BAIL_TYPE_RADIO_MENU_ITEM, BailRadioMenuItem, bail_radio_menu_item, bail_radio_menu_item_new)
BAIL_IMPLEMENT_FACTORY (BAIL_TYPE_EXPANDER, BailExpander, bail_expander, BTK_TYPE_EXPANDER)
BAIL_IMPLEMENT_FACTORY_WITH_FUNC_DUMMY (BAIL_TYPE_RENDERER_CELL, BailRendererCell, bail_renderer_cell, BTK_TYPE_CELL_RENDERER, bail_renderer_cell_new)
BAIL_IMPLEMENT_FACTORY_WITH_FUNC_DUMMY (BAIL_TYPE_BOOLEAN_CELL, BailBooleanCell, bail_boolean_cell, BTK_TYPE_CELL_RENDERER_TOGGLE, bail_boolean_cell_new)
BAIL_IMPLEMENT_FACTORY_WITH_FUNC_DUMMY (BAIL_TYPE_IMAGE_CELL, BailImageCell, bail_image_cell, BTK_TYPE_CELL_RENDERER_PIXBUF, bail_image_cell_new)
BAIL_IMPLEMENT_FACTORY_WITH_FUNC_DUMMY (BAIL_TYPE_TEXT_CELL, BailTextCell, bail_text_cell, BTK_TYPE_CELL_RENDERER_TEXT, bail_text_cell_new)

static BatkObject*
bail_get_accessible_for_widget (BtkWidget *widget,
                                gboolean  *transient)
{
  BatkObject *obj = NULL;
  GType bunny_canvas;

  bunny_canvas = g_type_from_name ("BunnyCanvas");

  *transient = FALSE;
  if (!widget)
    return NULL;

  if (BTK_IS_ENTRY (widget))
    {
      BtkWidget *other_widget = widget->parent;
      if (BTK_IS_COMBO (other_widget))
        {
          bail_set_focus_widget (other_widget, widget);
          widget = other_widget;
        }
    } 
  else if (BTK_IS_NOTEBOOK (widget)) 
    {
      BtkNotebook *notebook;
      gint page_num = -1;

      notebook = BTK_NOTEBOOK (widget);
      /*
       * Report the currently focused tab rather than the currently selected tab
       */
      if (notebook->focus_tab)
        {
          page_num = g_list_index (notebook->children, notebook->focus_tab->data);
        }
      if (page_num != -1)
        {
          obj = btk_widget_get_accessible (widget);
          obj = batk_object_ref_accessible_child (obj, page_num);
          g_object_unref (obj);
        }
    }
  else if (BTK_CHECK_TYPE ((widget), bunny_canvas))
    {
      GObject *focused_item;
      GValue value = {0, };

      g_value_init (&value, G_TYPE_OBJECT);
      g_object_get_property (G_OBJECT (widget), "focused_item", &value);
      focused_item = g_value_get_object (&value);

      if (focused_item)
        {
          BatkObject *tmp;

          obj = batk_bobject_accessible_for_object (G_OBJECT (focused_item));
          tmp = g_object_get_qdata (G_OBJECT (obj), quark_focus_object);
          if (tmp != NULL)
            obj = tmp;
        }
    }
  else if (BTK_IS_TOGGLE_BUTTON (widget))
    {
      BtkWidget *other_widget = widget->parent;
      if (BTK_IS_COMBO_BOX (other_widget))
        {
          bail_set_focus_widget (other_widget, widget);
          widget = other_widget;
        }
    }
  if (obj == NULL)
    {
      BatkObject *focus_object;

      obj = btk_widget_get_accessible (widget);
      focus_object = g_object_get_qdata (G_OBJECT (obj), quark_focus_object);
      /*
       * We check whether the object for this focus_object has been deleted.
       * This can happen when navigating to an empty directory in nautilus. 
       * See bug #141907.
       */
      if (BATK_IS_BOBJECT_ACCESSIBLE (focus_object))
        {
          if (!batk_bobject_accessible_get_object (BATK_BOBJECT_ACCESSIBLE (focus_object)))
            focus_object = NULL;
        }
      if (focus_object)
        obj = focus_object;
    }

  return obj;
}

static gboolean
bail_focus_watcher (GSignalInvocationHint *ihint,
                    guint                  n_param_values,
                    const GValue          *param_values,
                    gpointer               data)
{
  GObject *object;
  BtkWidget *widget;
  BdkEvent *event;

  object = g_value_get_object (param_values + 0);
  g_return_val_if_fail (BTK_IS_WIDGET(object), FALSE);

  event = g_value_get_boxed (param_values + 1);
  widget = BTK_WIDGET (object);

  if (event->type == BDK_FOCUS_CHANGE) 
    {
      if (event->focus_change.in)
        {
          if (BTK_IS_WINDOW (widget))
            {
              BtkWindow *window;

              window = BTK_WINDOW (widget);
              if (window->focus_widget)
                {
                  /*
                   * If we already have a potential focus widget set this
                   * windows's focus widget to focus_before_menu so that 
                   * it will be reported when menu item is unset.
                   */
                  if (next_focus_widget)
                    {
                      if (BTK_IS_MENU_ITEM (next_focus_widget) &&
                          !focus_before_menu)
                        {
                          void *vp_focus_before_menu = &focus_before_menu;
                          focus_before_menu = window->focus_widget;
                          g_object_add_weak_pointer (G_OBJECT (focus_before_menu), vp_focus_before_menu);
                        }

                      return TRUE;
                    }
                  widget = window->focus_widget;
                }
              else if (window->type == BTK_WINDOW_POPUP) 
                {
	          if (BTK_IS_BIN (widget))
		    {
		      BtkWidget *child = btk_bin_get_child (BTK_BIN (widget));

		      if (BTK_IS_WIDGET (child) && btk_widget_has_grab (child))
			{
			  if (BTK_IS_MENU_SHELL (child))
			    {
			      if (BTK_MENU_SHELL (child)->active_menu_item)
				{
				  /*
				   * We have a menu which has a menu item selected
				   * so we do not report focus on the menu.
				   */ 
				  return TRUE; 
				}
			    }
			  widget = child;
			} 
		    }
		  else /* popup window has no children; this edge case occurs in some custom code (OOo for instance) */
		    {
		      return TRUE;
		    }
                }
	      else /* Widget is a non-popup toplevel with no focus children; 
		      don't emit for this case either, as it's useless */
		{
		  return TRUE;
		}
            }
        }
      else
        {
          if (next_focus_widget)
            {
               BtkWidget *toplevel;

               toplevel = btk_widget_get_toplevel (next_focus_widget);
               if (toplevel == widget)
                 next_focus_widget = NULL; 
            }
          /* focus out */
          widget = NULL;
        }
    }
  else
    {
      if (event->type == BDK_MOTION_NOTIFY && btk_widget_has_focus (widget))
        {
          if (widget == focus_widget)
            {
              return TRUE;
            }
        }
      else
        {
          return TRUE;
        }
    }
  /*
   * If the focus widget is a BtkSocket without a plug
   * then ignore the focus notification as the embedded
   * plug will report a focus notification.
   */
  if (BTK_IS_SOCKET (widget) &&
      BTK_SOCKET (widget)->plug_widget == NULL)
    return TRUE;
  /*
   * The widget may not yet be visible on the screen so we wait until it is.
   */
  bail_focus_notify_when_idle (widget);
  return TRUE; 
}

static gboolean
bail_select_watcher (GSignalInvocationHint *ihint,
                     guint                  n_param_values,
                     const GValue          *param_values,
                     gpointer               data)
{
  GObject *object;
  BtkWidget *widget;

  object = g_value_get_object (param_values + 0);
  g_return_val_if_fail (BTK_IS_WIDGET(object), FALSE);

  widget = BTK_WIDGET (object);

  if (!btk_widget_get_mapped (widget))
    {
      g_signal_connect (widget, "map",
                        G_CALLBACK (bail_map_cb),
                        NULL);
    }
  else
    bail_finish_select (widget);

  return TRUE;
}

static void
bail_finish_select (BtkWidget *widget)
{
  if (BTK_IS_MENU_ITEM (widget))
    {
      BtkMenuItem* menu_item;

      menu_item = BTK_MENU_ITEM (widget);
      if (menu_item->submenu &&
          !btk_widget_get_mapped (menu_item->submenu))
        {
          /*
           * If the submenu is not visble, wait until it is before
           * reporting focus on the menu item.
           */
          gulong handler_id;

          handler_id = g_signal_handler_find (menu_item->submenu,
                                              G_SIGNAL_MATCH_FUNC,
                                              g_signal_lookup ("map",
                                                               BTK_TYPE_WINDOW),
                                              0,
                                              NULL,
                                              (gpointer) bail_map_submenu_cb,
                                              NULL); 
          if (!handler_id)
            g_signal_connect (menu_item->submenu, "map",
                              G_CALLBACK (bail_map_submenu_cb),
                              NULL);
            return;

        }
      /*
       * If we are waiting to report focus on a menubar or a menu item
       * because of a previous deselect, cancel it.
       */
      if (was_deselect &&
          focus_notify_handler &&
          next_focus_widget &&
          (BTK_IS_MENU_BAR (next_focus_widget) ||
           BTK_IS_MENU_ITEM (next_focus_widget)))
        {
          void *vp_next_focus_widget = &next_focus_widget;
          g_source_remove (focus_notify_handler);
          g_object_remove_weak_pointer (G_OBJECT (next_focus_widget), vp_next_focus_widget);
	  next_focus_widget = NULL;
          focus_notify_handler = 0;
          was_deselect = FALSE;
        }
    } 
  /*
   * If previously focused widget is not a BtkMenuItem or a BtkMenu,
   * keep track of it so we can return to it after menubar is deactivated
   */
  if (focus_widget && 
      !BTK_IS_MENU_ITEM (focus_widget) && 
      !BTK_IS_MENU (focus_widget))
    {
      void *vp_focus_before_menu = &focus_before_menu;
      focus_before_menu = focus_widget;
      g_object_add_weak_pointer (G_OBJECT (focus_before_menu), vp_focus_before_menu);

    } 
  bail_focus_notify_when_idle (widget);

  return; 
}

static void
bail_map_cb (BtkWidget *widget)
{
  bail_finish_select (widget);
}

static void
bail_map_submenu_cb (BtkWidget *widget)
{
  if (BTK_IS_MENU (widget))
    {
      if (BTK_MENU (widget)->parent_menu_item)
        bail_finish_select (BTK_MENU (widget)->parent_menu_item);
    }
}


static gboolean
bail_deselect_watcher (GSignalInvocationHint *ihint,
                       guint                  n_param_values,
                       const GValue          *param_values,
                       gpointer               data)
{
  GObject *object;
  BtkWidget *widget;
  BtkWidget *menu_shell;

  object = g_value_get_object (param_values + 0);
  g_return_val_if_fail (BTK_IS_WIDGET(object), FALSE);

  widget = BTK_WIDGET (object);

  if (!BTK_IS_MENU_ITEM (widget))
    return TRUE;

  if (subsequent_focus_widget == widget)
    subsequent_focus_widget = NULL;

  menu_shell = btk_widget_get_parent (widget);
  if (BTK_IS_MENU_SHELL (menu_shell))
    {
      BtkWidget *parent_menu_shell;

      parent_menu_shell = BTK_MENU_SHELL (menu_shell)->parent_menu_shell;
      if (parent_menu_shell)
        {
          BtkWidget *active_menu_item;

          active_menu_item = BTK_MENU_SHELL (parent_menu_shell)->active_menu_item;
          if (active_menu_item)
            {
              bail_focus_notify_when_idle (active_menu_item);
            }
        }
      else
        {
          if (!BTK_IS_MENU_BAR (menu_shell))
            {
              bail_focus_notify_when_idle (menu_shell);
            }
        }
    }
  was_deselect = TRUE;
  return TRUE; 
}

static gboolean 
bail_switch_page_watcher (GSignalInvocationHint *ihint,
                          guint                  n_param_values,
                          const GValue          *param_values,
                          gpointer               data)
{
  GObject *object;
  BtkWidget *widget;
  BtkNotebook *notebook;

  object = g_value_get_object (param_values + 0);
  g_return_val_if_fail (BTK_IS_WIDGET(object), FALSE);

  widget = BTK_WIDGET (object);

  if (!BTK_IS_NOTEBOOK (widget))
    return TRUE;

  notebook = BTK_NOTEBOOK (widget);
  if (!notebook->focus_tab)
    return TRUE;

  bail_focus_notify_when_idle (widget);
  return TRUE;
}

static gboolean
bail_focus_idle_handler (gpointer data)
{
  focus_notify_handler = 0;
  /*
   * The widget which was to receive focus may have been removed
   */
  if (!next_focus_widget)
    {
      if (next_focus_widget != data)
        return FALSE;
    }
  else
    {
      void *vp_next_focus_widget = &next_focus_widget;
      g_object_remove_weak_pointer (G_OBJECT (next_focus_widget), vp_next_focus_widget);
      next_focus_widget = NULL;
    }
    
  bail_focus_notify (data);

  return FALSE;
}

static void
bail_focus_notify (BtkWidget *widget)
{
  BatkObject *batk_obj;
  gboolean transient;

  if (widget != focus_widget)
    {
      if (focus_widget)
        {
          void *vp_focus_widget = &focus_widget;
          g_object_remove_weak_pointer (G_OBJECT (focus_widget), vp_focus_widget);
        }
      focus_widget = widget;
      if (focus_widget)
        {
          void *vp_focus_widget = &focus_widget;
          g_object_add_weak_pointer (G_OBJECT (focus_widget), vp_focus_widget);
          /*
           * The UI may not have been updated yet; e.g. in btkhtml2
           * html_view_layout() is called in a idle handler
           */
          if (focus_widget == focus_before_menu)
            {
              void *vp_focus_before_menu = &focus_before_menu;
              g_object_remove_weak_pointer (G_OBJECT (focus_before_menu), vp_focus_before_menu);
              focus_before_menu = NULL;
            }
        }
      bail_focus_notify_when_idle (focus_widget);
    }
  else
    {
      if (focus_widget)
        batk_obj  = bail_get_accessible_for_widget (focus_widget, &transient);
      else
        batk_obj = NULL;
      /*
       * Do not report focus on redundant object
       */
      if (batk_obj && 
	  (batk_object_get_role(batk_obj) != BATK_ROLE_REDUNDANT_OBJECT))
	  batk_focus_tracker_notify (batk_obj);
      if (batk_obj && transient)
        g_object_unref (batk_obj);
      if (subsequent_focus_widget)
        {
          BtkWidget *tmp_widget = subsequent_focus_widget;
          subsequent_focus_widget = NULL;
          bail_focus_notify_when_idle (tmp_widget);
        }
    }
}

static void
bail_focus_notify_when_idle (BtkWidget *widget)
{
  if (focus_notify_handler)
    {
      if (widget)
        {
          /*
           * Ignore focus request when menu item is going to be focused.
           * See bug #124232.
           */
          if (BTK_IS_MENU_ITEM (next_focus_widget) && !BTK_IS_MENU_ITEM (widget))
            return;

          if (next_focus_widget)
            {
              if (BTK_IS_MENU_ITEM (next_focus_widget) && BTK_IS_MENU_ITEM (widget))
                {
                  if (btk_menu_item_get_submenu (BTK_MENU_ITEM (next_focus_widget)) == btk_widget_get_parent (widget))
                    {
                      if (subsequent_focus_widget)
                        g_assert_not_reached ();
                      subsequent_focus_widget = widget;
                      return;
                    } 
                }
            }
          g_source_remove (focus_notify_handler);
          if (next_focus_widget)
	    {
	      void *vp_next_focus_widget = &next_focus_widget;
	      g_object_remove_weak_pointer (G_OBJECT (next_focus_widget), vp_next_focus_widget);
	      next_focus_widget = NULL;
	    }
        }
      else
        /*
         * Ignore if focus is being set to NULL and we are waiting to set focus
         */
        return;
    }

  if (widget)
    {
      void *vp_next_focus_widget = &next_focus_widget;
      next_focus_widget = widget;
      g_object_add_weak_pointer (G_OBJECT (next_focus_widget), vp_next_focus_widget);
    }
  else
    {
      /*
       * We are about to report focus as NULL so remove the weak pointer
       * for the widget we were waiting to report focus on.
       */ 
      if (next_focus_widget)
        {
          void *vp_next_focus_widget = &next_focus_widget;
          g_object_remove_weak_pointer (G_OBJECT (next_focus_widget), vp_next_focus_widget);
          next_focus_widget = NULL;
        }
    }

  focus_notify_handler = bdk_threads_add_idle (bail_focus_idle_handler, widget);
}

static gboolean
bail_deactivate_watcher (GSignalInvocationHint *ihint,
                         guint                  n_param_values,
                         const GValue          *param_values,
                         gpointer               data)
{
  GObject *object;
  BtkWidget *widget;
  BtkMenuShell *shell;
  BtkWidget *focus = NULL;

  object = g_value_get_object (param_values + 0);
  g_return_val_if_fail (BTK_IS_WIDGET(object), FALSE);
  widget = BTK_WIDGET (object);

  g_return_val_if_fail (BTK_IS_MENU_SHELL(widget), TRUE);
  shell = BTK_MENU_SHELL(widget);
  if (!shell->parent_menu_shell)
    focus = focus_before_menu;
      
  /*
   * If we are waiting to report focus on a menubar or a menu item
   * because of a previous deselect, cancel it.
   */
  if (was_deselect &&
      focus_notify_handler &&
      next_focus_widget &&
      (BTK_IS_MENU_BAR (next_focus_widget) ||
       BTK_IS_MENU_ITEM (next_focus_widget)))
    {
      void *vp_next_focus_widget = &next_focus_widget;
      g_source_remove (focus_notify_handler);
      g_object_remove_weak_pointer (G_OBJECT (next_focus_widget), vp_next_focus_widget);
      next_focus_widget = NULL;
      focus_notify_handler = 0;
      was_deselect = FALSE;
    }
  bail_focus_notify_when_idle (focus);

  return TRUE; 
}

static void
bail_focus_tracker_init (void)
{
  static gboolean  emission_hooks_added = FALSE;

  if (!emission_hooks_added)
    {
      /*
       * We cannot be sure that the classes exist so we make sure that they do.
       */
      g_type_class_ref (BTK_TYPE_WIDGET);
      g_type_class_ref (BTK_TYPE_ITEM);
      g_type_class_ref (BTK_TYPE_MENU_SHELL);
      g_type_class_ref (BTK_TYPE_NOTEBOOK);

      /*
       * We listen for event_after signal and then check that the
       * event was a focus in event so we get called after the event.
       */
      g_signal_add_emission_hook (
             g_signal_lookup ("event-after", BTK_TYPE_WIDGET), 0,
             bail_focus_watcher, NULL, (GDestroyNotify) NULL);
      /*
       * A "select" signal is emitted when arrow key is used to
       * move to a list item in the popup window of a BtkCombo or
       * a menu item in a menu.
       */
      g_signal_add_emission_hook (
             g_signal_lookup ("select", BTK_TYPE_ITEM), 0,
             bail_select_watcher, NULL, (GDestroyNotify) NULL);

      /*
       * A "deselect" signal is emitted when arrow key is used to
       * move from a menu item in a menu to the parent menu.
       */
      g_signal_add_emission_hook (
             g_signal_lookup ("deselect", BTK_TYPE_ITEM), 0,
             bail_deselect_watcher, NULL, (GDestroyNotify) NULL);

      /*
       * We listen for deactivate signals on menushells to determine
       * when the "focus" has left the menus.
       */
      g_signal_add_emission_hook (
             g_signal_lookup ("deactivate", BTK_TYPE_MENU_SHELL), 0,
             bail_deactivate_watcher, NULL, (GDestroyNotify) NULL);

      /*
       * We listen for "switch-page" signal on a BtkNotebook to notify
       * when page has changed because of clicking on a notebook tab.
       */
      g_signal_add_emission_hook (
             g_signal_lookup ("switch-page", BTK_TYPE_NOTEBOOK), 0,
             bail_switch_page_watcher, NULL, (GDestroyNotify) NULL);
      emission_hooks_added = TRUE;
    }
}

static void
bail_focus_object_destroyed (gpointer data)
{
  GObject *obj;

  obj = G_OBJECT (data);
  g_object_set_qdata (obj, quark_focus_object, NULL);
  g_object_unref (obj); 
}

static void
bail_focus_tracker (BatkObject *focus_object)
{
  /*
   * Do not report focus on redundant object
   */
  if (focus_object && 
      (batk_object_get_role(focus_object) != BATK_ROLE_REDUNDANT_OBJECT))
    {
      BatkObject *old_focus_object;

      if (!BTK_IS_ACCESSIBLE (focus_object))
        {
          BatkObject *parent;

          parent = focus_object;
          while (1)
            {
              parent = batk_object_get_parent (parent);
              if (parent == NULL)
                break;
              if (BTK_IS_ACCESSIBLE (parent))
                break;
            }

          if (parent)
            {
              bail_set_focus_object (focus_object, parent);
            }
        }
      else
        {
          old_focus_object = g_object_get_qdata (G_OBJECT (focus_object), quark_focus_object);
          if (old_focus_object)
            {
              g_object_weak_unref (G_OBJECT (old_focus_object),
                                   (GWeakNotify) bail_focus_object_destroyed,
                                   focus_object);
              g_object_set_qdata (G_OBJECT (focus_object), quark_focus_object, NULL);
              g_object_unref (G_OBJECT (focus_object));
            }
        }
    }
}

static void 
bail_set_focus_widget (BtkWidget *focus_widget,
                       BtkWidget *widget)
{
  BatkObject *focus_obj;
  BatkObject *obj;

  focus_obj = btk_widget_get_accessible (focus_widget);
  obj = btk_widget_get_accessible (widget);
  bail_set_focus_object (focus_obj, obj);
}

static void 
bail_set_focus_object (BatkObject *focus_obj,
                       BatkObject *obj)
{
  BatkObject *old_focus_obj;

  old_focus_obj = g_object_get_qdata (G_OBJECT (obj), quark_focus_object);
  if (old_focus_obj != obj)
    {
      if (old_focus_obj)
        g_object_weak_unref (G_OBJECT (old_focus_obj),
                             (GWeakNotify) bail_focus_object_destroyed,
                             obj);
      else
        /*
         * We call g_object_ref as if obj is destroyed 
         * while the weak reference exists then destroying the 
         * focus_obj would cause bail_focus_object_destroyed to be 
         * called when obj is not a valid GObject.
         */
        g_object_ref (obj);

      g_object_weak_ref (G_OBJECT (focus_obj),
                         (GWeakNotify) bail_focus_object_destroyed,
                         obj);
      g_object_set_qdata (G_OBJECT (obj), quark_focus_object, focus_obj);
    }
}

/*
 *   These exported symbols are hooked by bunny-program
 * to provide automatic module initialization and shutdown.
 */
extern void bunny_accessibility_module_init     (void);
extern void bunny_accessibility_module_shutdown (void);

static int bail_initialized = FALSE;

static void
bail_accessibility_module_init (void)
{
  const char *env_a_t_support;
  gboolean a_t_support = FALSE;

  if (bail_initialized)
    {
      return;
    }
  bail_initialized = TRUE;
  quark_focus_object = g_quark_from_static_string ("bail-focus-object");
  
  env_a_t_support = g_getenv (BUNNY_ACCESSIBILITY_ENV);

  if (env_a_t_support)
    a_t_support = atoi (env_a_t_support);
  if (a_t_support)
    fprintf (stderr, "BTK Accessibility Module initialized\n");

  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_WIDGET, bail_widget);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_CONTAINER, bail_container);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_BUTTON, bail_button);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_ITEM, bail_item);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_MENU_ITEM, bail_menu_item);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_TOGGLE_BUTTON, bail_toggle_button);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_IMAGE, bail_image);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_TEXT_VIEW, bail_text_view);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_COMBO, bail_combo);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_COMBO_BOX, bail_combo_box);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_ENTRY, bail_entry);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_MENU_BAR, bail_menu_shell);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_MENU, bail_menu);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_WINDOW, bail_window);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_RANGE, bail_range);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_SCALE, bail_scale);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_SCALE_BUTTON, bail_scale_button);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_CLIST, bail_clist);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_LABEL, bail_label);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_STATUSBAR, bail_statusbar);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_NOTEBOOK, bail_notebook);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_CALENDAR, bail_calendar);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_PROGRESS_BAR, bail_progress_bar);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_SPIN_BUTTON, bail_spin_button);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_TREE_VIEW, bail_tree_view);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_FRAME, bail_frame);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_CELL_RENDERER_TEXT, bail_text_cell);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_CELL_RENDERER_TOGGLE, bail_boolean_cell);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_CELL_RENDERER_PIXBUF, bail_image_cell);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_CELL_RENDERER, bail_renderer_cell);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_RADIO_BUTTON, bail_radio_button);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_ARROW, bail_arrow);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_PIXMAP, bail_pixmap);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_SEPARATOR, bail_separator);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_BOX, bail_box);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_SCROLLED_WINDOW, bail_scrolled_window);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_LIST, bail_list);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_PANED, bail_paned);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_SCROLLBAR, bail_scrollbar);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_OPTION_MENU, bail_option_menu);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_CHECK_MENU_ITEM, bail_check_menu_item);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_RADIO_MENU_ITEM, bail_radio_menu_item);
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_EXPANDER, bail_expander);

  /* LIBBUNNYCANVAS SUPPORT */
  BAIL_WIDGET_SET_FACTORY (BTK_TYPE_OBJECT, bail_object);

  batk_focus_tracker_init (bail_focus_tracker_init);
  focus_tracker_id = batk_add_focus_tracker (bail_focus_tracker);

  /* Initialize the BailUtility class */
  g_type_class_unref (g_type_class_ref (BAIL_TYPE_UTIL));
  g_type_class_unref (g_type_class_ref (BAIL_TYPE_MISC));
}

/**
 * bunny_accessibility_module_init:
 * @void: 
 * 
 *   This method is invoked by name from libbunny's
 * bunny-program.c to activate accessibility support.
 **/
void
bunny_accessibility_module_init (void)
{
  bail_accessibility_module_init ();
}

/**
 * bunny_accessibility_module_shutdown:
 * @void: 
 * 
 *   This method is invoked by name from libbunny's
 * bunny-program.c to de-activate accessibility support.
 **/
void
bunny_accessibility_module_shutdown (void)
{
  if (!bail_initialized)
    {
      return;
    }
  bail_initialized = FALSE;
  batk_remove_focus_tracker (focus_tracker_id);

  fprintf (stderr, "BTK Accessibility Module shutdown\n");

  /* FIXME: de-register the factory types so we can unload ? */
}

int
btk_module_init (gint *argc, char** argv[])
{
  const char* env_no_bail;
  gboolean no_bail = FALSE;

  env_no_bail = g_getenv (NO_BAIL_ENV);
  if (env_no_bail)
      no_bail = atoi (env_no_bail);

  if (no_bail)
      return 0;

  bail_accessibility_module_init ();

  return 0;
}

const char *
g_module_check_init (GModule *module)
{
  g_module_make_resident (module);

  return NULL;
}
 
