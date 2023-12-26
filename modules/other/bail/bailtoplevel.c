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

#include <stdlib.h>
#include <string.h>

#undef BTK_DISABLE_DEPRECATED

#include <btk/btk.h>

#include "bailtoplevel.h"

static void             bail_toplevel_class_init        (BailToplevelClass      *klass);
static void             bail_toplevel_init              (BailToplevel           *toplevel);
static void             bail_toplevel_initialize        (BatkObject              *accessible,
                                                         gpointer                data);
static void             bail_toplevel_object_finalize   (BObject                *obj);

/* batkobject.h */

static gint             bail_toplevel_get_n_children    (BatkObject              *obj);
static BatkObject*       bail_toplevel_ref_child         (BatkObject              *obj,
                                                        gint                    i);
static BatkObject*       bail_toplevel_get_parent        (BatkObject              *obj);

/* Callbacks */


static void             bail_toplevel_window_destroyed  (BtkWindow              *window,
                                                        BailToplevel            *text);
static gboolean         bail_toplevel_hide_event_watcher (GSignalInvocationHint *ihint,
                                                        guint                   n_param_values,
                                                        const BValue            *param_values,
                                                        gpointer                data);
static gboolean         bail_toplevel_show_event_watcher (GSignalInvocationHint *ihint,
                                                        guint                   n_param_values,
                                                        const BValue            *param_values,
                                                        gpointer                data);

/* Misc */

static void      _bail_toplevel_remove_child            (BailToplevel           *toplevel,
                                                        BtkWindow               *window);
static gboolean  is_attached_menu_window                (BtkWidget              *widget);
static gboolean  is_combo_window                        (BtkWidget              *widget);


G_DEFINE_TYPE (BailToplevel, bail_toplevel, BATK_TYPE_OBJECT)

static void
bail_toplevel_class_init (BailToplevelClass *klass)
{
  BatkObjectClass *class = BATK_OBJECT_CLASS(klass);
  BObjectClass *g_object_class = B_OBJECT_CLASS(klass);

  class->initialize = bail_toplevel_initialize;
  class->get_n_children = bail_toplevel_get_n_children;
  class->ref_child = bail_toplevel_ref_child;
  class->get_parent = bail_toplevel_get_parent;

  g_object_class->finalize = bail_toplevel_object_finalize;
}

static void
bail_toplevel_init (BailToplevel *toplevel)
{
  BtkWindow *window;
  BtkWidget *widget;
  GList *l;
  guint signal_id;
  
  l = toplevel->window_list = btk_window_list_toplevels ();

  while (l)
    {
      window = BTK_WINDOW (l->data);
      widget = BTK_WIDGET (window);
      if (!window || 
          !btk_widget_get_visible (widget) ||
          is_attached_menu_window (widget) ||
          BTK_WIDGET (window)->parent ||
          BTK_IS_PLUG (window))
        {
          GList *temp_l  = l->next;

          toplevel->window_list = g_list_delete_link (toplevel->window_list, l);
          l = temp_l;
        }
      else
        {
          g_signal_connect (B_OBJECT (window), 
                            "destroy",
                            G_CALLBACK (bail_toplevel_window_destroyed),
                            toplevel);
          l = l->next;
        }
    }

  g_type_class_ref (BTK_TYPE_WINDOW);

  signal_id  = g_signal_lookup ("show", BTK_TYPE_WINDOW);
  g_signal_add_emission_hook (signal_id, 0,
    bail_toplevel_show_event_watcher, toplevel, (GDestroyNotify) NULL);

  signal_id  = g_signal_lookup ("hide", BTK_TYPE_WINDOW);
  g_signal_add_emission_hook (signal_id, 0,
    bail_toplevel_hide_event_watcher, toplevel, (GDestroyNotify) NULL);
}

static void
bail_toplevel_initialize (BatkObject *accessible,
                          gpointer  data)
{
  BATK_OBJECT_CLASS (bail_toplevel_parent_class)->initialize (accessible, data);

  accessible->role = BATK_ROLE_APPLICATION;
  accessible->name = g_get_prgname();
  accessible->accessible_parent = NULL;
}

static void
bail_toplevel_object_finalize (BObject *obj)
{
  BailToplevel *toplevel = BAIL_TOPLEVEL (obj);

  if (toplevel->window_list)
    g_list_free (toplevel->window_list);

  B_OBJECT_CLASS (bail_toplevel_parent_class)->finalize (obj);
}

static BatkObject*
bail_toplevel_get_parent (BatkObject *obj)
{
    return NULL;
}

static gint
bail_toplevel_get_n_children (BatkObject *obj)
{
  BailToplevel *toplevel = BAIL_TOPLEVEL (obj);

  gint rc = g_list_length (toplevel->window_list);
  return rc;
}

static BatkObject*
bail_toplevel_ref_child (BatkObject *obj,
                         gint      i)
{
  BailToplevel *toplevel;
  gpointer ptr;
  BtkWidget *widget;
  BatkObject *batk_obj;

  toplevel = BAIL_TOPLEVEL (obj);
  ptr = g_list_nth_data (toplevel->window_list, i);
  if (!ptr)
    return NULL;
  widget = BTK_WIDGET (ptr);
  batk_obj = btk_widget_get_accessible (widget);

  g_object_ref (batk_obj);
  return batk_obj;
}

/*
 * Window destroy events on BtkWindow cause a child to be removed
 * from the toplevel
 */
static void
bail_toplevel_window_destroyed (BtkWindow    *window,
                                BailToplevel *toplevel)
{
  _bail_toplevel_remove_child (toplevel, window);
}

/*
 * Show events cause a child to be added to the toplevel
 */
static gboolean
bail_toplevel_show_event_watcher (GSignalInvocationHint *ihint,
                                  guint                  n_param_values,
                                  const BValue          *param_values,
                                  gpointer               data)
{
  BailToplevel *toplevel = BAIL_TOPLEVEL (data);
  BatkObject *batk_obj = BATK_OBJECT (toplevel);
  BObject *object;
  BtkWidget *widget;
  gint n_children;
  BatkObject *child;

  object = b_value_get_object (param_values + 0);

  if (!BTK_IS_WINDOW (object))
    return TRUE;

  widget = BTK_WIDGET (object);
  if (widget->parent || 
      is_attached_menu_window (widget) ||
      is_combo_window (widget) ||
      BTK_IS_PLUG (widget))
    return TRUE;

  child = btk_widget_get_accessible (widget);
  if (batk_object_get_role (child) == BATK_ROLE_REDUNDANT_OBJECT)
    {
      return TRUE;
    }

  /* 
   * Add the window to the list & emit the signal.
   * Don't do this for tooltips (Bug #150649).
   */
  if (batk_object_get_role (child) == BATK_ROLE_TOOL_TIP)
    {
      return TRUE;
    }

  toplevel->window_list = g_list_append (toplevel->window_list, widget);

  n_children = g_list_length (toplevel->window_list);

  /*
   * Must subtract 1 from the n_children since the index is 0-based
   * but g_list_length is 1-based.
   */
  batk_object_set_parent (child, batk_obj);
  g_signal_emit_by_name (batk_obj, "children-changed::add",
                         n_children - 1, 
                         child, NULL);

  /* Connect destroy signal callback */
  g_signal_connect (B_OBJECT(object), 
                    "destroy",
                    G_CALLBACK (bail_toplevel_window_destroyed),
                    toplevel);

  return TRUE;
}

/*
 * Hide events on BtkWindow cause a child to be removed from the toplevel
 */
static gboolean
bail_toplevel_hide_event_watcher (GSignalInvocationHint *ihint,
                                  guint                  n_param_values,
                                  const BValue          *param_values,
                                  gpointer               data)
{
  BailToplevel *toplevel = BAIL_TOPLEVEL (data);
  BObject *object;

  object = b_value_get_object (param_values + 0);

  if (!BTK_IS_WINDOW (object))
    return TRUE;

  _bail_toplevel_remove_child (toplevel, BTK_WINDOW (object));
  return TRUE;
}

/*
 * Common code used by destroy and hide events on BtkWindow
 */
static void
_bail_toplevel_remove_child (BailToplevel *toplevel, 
                             BtkWindow    *window)
{
  BatkObject *batk_obj = BATK_OBJECT (toplevel);
  GList *l;
  guint window_count = 0;
  BatkObject *child;

  if (toplevel->window_list)
    {
        BtkWindow *tmp_window;

        /* Must loop through them all */
        for (l = toplevel->window_list; l; l = l->next)
        {
          tmp_window = BTK_WINDOW (l->data);

          if (window == tmp_window)
            {
              /* Remove the window from the window_list & emit the signal */
              toplevel->window_list = g_list_remove (toplevel->window_list,
                                                     l->data);
              child = btk_widget_get_accessible (BTK_WIDGET (window));
              g_signal_emit_by_name (batk_obj, "children-changed::remove",
                                     window_count, 
                                     child, NULL);
              batk_object_set_parent (child, NULL);
              break;
            }

          window_count++;
        }
    }
}

static gboolean
is_attached_menu_window (BtkWidget *widget)
{
  BtkWidget *child = BTK_BIN (widget)->child;
  gboolean ret = FALSE;

  if (BTK_IS_MENU (child))
    {
      BtkWidget *attach;

      attach = btk_menu_get_attach_widget (BTK_MENU (child));
      /* Allow for menu belonging to the Panel Menu, which is a BtkButton */
      if (BTK_IS_MENU_ITEM (attach) ||
          BTK_IS_OPTION_MENU (attach) ||
          BTK_IS_BUTTON (attach))
        ret = TRUE;
    }
  return ret;
}

static gboolean
is_combo_window (BtkWidget *widget)
{
  BtkWidget *child = BTK_BIN (widget)->child;
  BatkObject *obj;
  BtkAccessible *accessible;

  if (!BTK_IS_EVENT_BOX (child))
    return FALSE;

  child = BTK_BIN (child)->child;

  if (!BTK_IS_FRAME (child))
    return FALSE;

  child = BTK_BIN (child)->child;

  if (!BTK_IS_SCROLLED_WINDOW (child))
    return FALSE;

  obj = btk_widget_get_accessible (child);
  obj = batk_object_get_parent (obj);
  accessible = BTK_ACCESSIBLE (obj);
  if (BTK_IS_COMBO (accessible->widget))
    return TRUE;

  return  FALSE;
}
