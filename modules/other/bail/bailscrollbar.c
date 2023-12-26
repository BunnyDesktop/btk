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

#include <btk/btk.h>
#include "bailscrollbar.h"

static void bail_scrollbar_class_init  (BailScrollbarClass *klass);
static void bail_scrollbar_init        (BailScrollbar      *accessible);
static void bail_scrollbar_initialize  (BatkObject           *accessible,
                                        gpointer             data);

static gint bail_scrollbar_get_index_in_parent (BatkObject *accessible);

G_DEFINE_TYPE (BailScrollbar, bail_scrollbar, BAIL_TYPE_RANGE)

static void	 
bail_scrollbar_class_init (BailScrollbarClass *klass)
{
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);

  class->initialize = bail_scrollbar_initialize;
  class->get_index_in_parent = bail_scrollbar_get_index_in_parent;
}

static void
bail_scrollbar_init (BailScrollbar      *accessible)
{
}

static void
bail_scrollbar_initialize (BatkObject *accessible,
                           gpointer  data)
{
  BATK_OBJECT_CLASS (bail_scrollbar_parent_class)->initialize (accessible, data);

  accessible->role = BATK_ROLE_SCROLL_BAR;
}

static gint
bail_scrollbar_get_index_in_parent (BatkObject *accessible)
{
  BtkWidget *widget;
  BtkScrolledWindow *scrolled_window;
  gint n_children;
  GList *children;

  widget = BTK_ACCESSIBLE (accessible)->widget;

  if (widget == NULL)
  {
    /*
     * State is defunct
     */
    return -1;
  }
  g_return_val_if_fail (BTK_IS_SCROLLBAR (widget), -1);

  if (!BTK_IS_SCROLLED_WINDOW(widget->parent))
    return BATK_OBJECT_CLASS (bail_scrollbar_parent_class)->get_index_in_parent (accessible);

  scrolled_window = BTK_SCROLLED_WINDOW (widget->parent);
  children = btk_container_get_children (BTK_CONTAINER (scrolled_window));
  n_children = g_list_length (children);
  g_list_free (children);

  if (BTK_IS_HSCROLLBAR (widget))
  {
    if (!scrolled_window->hscrollbar_visible) 
    {
      n_children = -1;
    }
  }
  else if (BTK_IS_VSCROLLBAR (widget))
  {
    if (!scrolled_window->vscrollbar_visible) 
    {
      n_children = -1;
    }
    else if (scrolled_window->hscrollbar_visible) 
    {
      n_children++;
    }
  }
  else
  {
    n_children = -1;
  }
  return n_children;
} 
