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
#include <btk/btk.h>
#include "bailscrolledwindow.h"


static void         bail_scrolled_window_class_init     (BailScrolledWindowClass  *klass); 
static void         bail_scrolled_window_init           (BailScrolledWindow       *window);
static void         bail_scrolled_window_real_initialize
                                                        (BatkObject     *obj,
                                                         gpointer      data);

static gint         bail_scrolled_window_get_n_children (BatkObject     *object);
static BatkObject*   bail_scrolled_window_ref_child      (BatkObject     *obj,
                                                         gint          child);
static void         bail_scrolled_window_scrollbar_visibility_changed 
                                                        (BObject       *object,
                                                         BParamSpec    *pspec,
                                                         gpointer      user_data);

G_DEFINE_TYPE (BailScrolledWindow, bail_scrolled_window, BAIL_TYPE_CONTAINER)

static void
bail_scrolled_window_class_init (BailScrolledWindowClass *klass)
{
  BatkObjectClass  *class = BATK_OBJECT_CLASS (klass);

  class->get_n_children = bail_scrolled_window_get_n_children;
  class->ref_child = bail_scrolled_window_ref_child;
  class->initialize = bail_scrolled_window_real_initialize;
}

static void
bail_scrolled_window_init (BailScrolledWindow *window)
{
}

static void
bail_scrolled_window_real_initialize (BatkObject *obj,
                                      gpointer  data)
{
  BtkScrolledWindow *window;

  BATK_OBJECT_CLASS (bail_scrolled_window_parent_class)->initialize (obj, data);

  window = BTK_SCROLLED_WINDOW (data);
  g_signal_connect_data (window->hscrollbar, "notify::visible",
    (GCallback)bail_scrolled_window_scrollbar_visibility_changed, 
    obj, NULL, FALSE);
  g_signal_connect_data (window->vscrollbar, "notify::visible",
    (GCallback)bail_scrolled_window_scrollbar_visibility_changed, 
    obj, NULL, FALSE);

  obj->role = BATK_ROLE_SCROLL_PANE;
}

static gint
bail_scrolled_window_get_n_children (BatkObject *object)
{
  BtkWidget *widget;
  BtkScrolledWindow *btk_window;
  GList *children;
  gint n_children;
 
  widget = BTK_ACCESSIBLE (object)->widget;
  if (widget == NULL)
    /* Object is defunct */
    return 0;

  btk_window = BTK_SCROLLED_WINDOW (widget);
   
  /* Get the number of children returned by the backing BtkScrolledWindow */

  children = btk_container_get_children (BTK_CONTAINER(btk_window));
  n_children = g_list_length (children);
  g_list_free (children);
  
  /* Add one to the count for each visible scrollbar */

  if (btk_window->hscrollbar_visible)
    n_children++;
  if (btk_window->vscrollbar_visible)
    n_children++;
  return n_children;
}

static BatkObject *
bail_scrolled_window_ref_child (BatkObject *obj, 
                                gint      child)
{
  BtkWidget *widget;
  BtkScrolledWindow *btk_window;
  GList *children, *tmp_list;
  gint n_children;
  BatkObject  *accessible = NULL;

  g_return_val_if_fail (child >= 0, NULL);

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /* Object is defunct */
    return NULL;

  btk_window = BTK_SCROLLED_WINDOW (widget);

  children = btk_container_get_children (BTK_CONTAINER (btk_window));
  n_children = g_list_length (children);

  if (child == n_children)
    {
      if (btk_window->hscrollbar_visible)
        accessible = btk_widget_get_accessible (btk_window->hscrollbar);
      else if (btk_window->vscrollbar_visible)
        accessible = btk_widget_get_accessible (btk_window->vscrollbar);
    }
  else if (child == n_children+1 && 
           btk_window->hscrollbar_visible &&
           btk_window->vscrollbar_visible)
    accessible = btk_widget_get_accessible (btk_window->vscrollbar);
  else if (child < n_children)
    {
      tmp_list = g_list_nth (children, child);
      if (tmp_list)
	accessible = btk_widget_get_accessible (
		BTK_WIDGET (tmp_list->data));
    }

  g_list_free (children);
  if (accessible)
    g_object_ref (accessible);
  return accessible; 
}

static void
bail_scrolled_window_scrollbar_visibility_changed (BObject    *object,
                                                   BParamSpec *pspec,
                                                   gpointer   user_data)
{
  if (!strcmp (pspec->name, "visible"))
    {
      gint index;
      gint n_children;
      gboolean child_added = FALSE;
      GList *children;
      BatkObject *child;
      BtkScrolledWindow *btk_window;
      BailScrolledWindow *bail_window = BAIL_SCROLLED_WINDOW (user_data);
      gchar *signal_name;

      btk_window = BTK_SCROLLED_WINDOW (BTK_ACCESSIBLE (user_data)->widget);
      if (btk_window == NULL)
        return;
      children = btk_container_get_children (BTK_CONTAINER (btk_window));
      index = n_children = g_list_length (children);
      g_list_free (children);

      if ((gpointer) object == (gpointer) (btk_window->hscrollbar))
        {
          if (btk_window->hscrollbar_visible)
            child_added = TRUE;

          child = btk_widget_get_accessible (btk_window->hscrollbar);
        }
      else if ((gpointer) object == (gpointer) (btk_window->vscrollbar))
        {
          if (btk_window->vscrollbar_visible)
            child_added = TRUE;

          child = btk_widget_get_accessible (btk_window->vscrollbar);
          if (btk_window->hscrollbar_visible)
            index = n_children+1;
        }
      else 
        {
          g_assert_not_reached ();
          return;
        }

      if (child_added)
        signal_name = "children_changed::add";
      else
        signal_name = "children_changed::delete";

      g_signal_emit_by_name (bail_window, signal_name, index, child, NULL);
    }
}
