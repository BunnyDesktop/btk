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
#include "bailnotebook.h"
#include "bailnotebookpage.h"
#include "bail-private-macros.h"

static void         bail_notebook_class_init          (BailNotebookClass *klass);
static void         bail_notebook_init                (BailNotebook      *notebook);
static void         bail_notebook_finalize            (BObject           *object);
static void         bail_notebook_real_initialize     (BatkObject         *obj,
                                                       gpointer          data);

static void         bail_notebook_real_notify_btk     (BObject           *obj,
                                                       BParamSpec        *pspec);

static BatkObject*   bail_notebook_ref_child           (BatkObject      *obj,
                                                       gint           i);
static gint         bail_notebook_real_remove_btk     (BtkContainer   *container,
                                                       BtkWidget      *widget,
                                                       gpointer       data);    
static void         batk_selection_interface_init      (BatkSelectionIface *iface);
static gboolean     bail_notebook_add_selection       (BatkSelection   *selection,
                                                       gint           i);
static BatkObject*   bail_notebook_ref_selection       (BatkSelection   *selection,
                                                       gint           i);
static gint         bail_notebook_get_selection_count (BatkSelection   *selection);
static gboolean     bail_notebook_is_child_selected   (BatkSelection   *selection,
                                                       gint           i);
static BatkObject*   find_child_in_list                (GList          *list,
                                                       gint           index);
static void         check_cache                       (BailNotebook   *bail_notebook,
                                                       BtkNotebook    *notebook);
static void         reset_cache                       (BailNotebook   *bail_notebook,
                                                       gint           index);
static void         create_notebook_page_accessible   (BailNotebook   *bail_notebook,
                                                       BtkNotebook    *notebook,
                                                       gint           index,
                                                       gboolean       insert_before,
                                                       GList          *list);
static void         bail_notebook_child_parent_set    (BtkWidget      *widget,
                                                       BtkWidget      *old_parent,
                                                       gpointer       data);
static gboolean     bail_notebook_focus_cb            (BtkWidget      *widget,
                                                       BtkDirectionType type);
static gboolean     bail_notebook_check_focus_tab     (gpointer       data);
static void         bail_notebook_destroyed           (gpointer       data);


G_DEFINE_TYPE_WITH_CODE (BailNotebook, bail_notebook, BAIL_TYPE_CONTAINER,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_SELECTION, batk_selection_interface_init))

static void
bail_notebook_class_init (BailNotebookClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);
  BatkObjectClass  *class = BATK_OBJECT_CLASS (klass);
  BailWidgetClass *widget_class;
  BailContainerClass *container_class;

  widget_class = (BailWidgetClass*)klass;
  container_class = (BailContainerClass*)klass;

  bobject_class->finalize = bail_notebook_finalize;

  widget_class->notify_btk = bail_notebook_real_notify_btk;

  class->ref_child = bail_notebook_ref_child;
  class->initialize = bail_notebook_real_initialize;
  /*
   * We do not provide an implementation of get_n_children
   * as the implementation in BailContainer returns the correct
   * number of children.
   */
  container_class->remove_btk = bail_notebook_real_remove_btk;
}

static void
bail_notebook_init (BailNotebook      *notebook)
{
  notebook->page_cache = NULL;
  notebook->selected_page = -1;
  notebook->focus_tab_page = -1;
  notebook->remove_index = -1;
  notebook->idle_focus_id = 0;
}

static BatkObject*
bail_notebook_ref_child (BatkObject      *obj,
                         gint           i)
{
  BatkObject *accessible = NULL;
  BailNotebook *bail_notebook;
  BtkNotebook *btk_notebook;
  BtkWidget *widget;
 
  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  bail_notebook = BAIL_NOTEBOOK (obj);
  
  btk_notebook = BTK_NOTEBOOK (widget);
  
  if (bail_notebook->page_count < g_list_length (btk_notebook->children))
    check_cache (bail_notebook, btk_notebook);

  accessible = find_child_in_list (bail_notebook->page_cache, i);

  if (accessible != NULL)
    g_object_ref (accessible);

  return accessible;
}

static void
bail_notebook_page_added (BtkNotebook *btk_notebook,
                          BtkWidget   *child,
                          guint        page_num,
                          gpointer     data)
{
  BatkObject *batk_obj;
  BailNotebook *notebook;

  batk_obj = btk_widget_get_accessible (BTK_WIDGET (btk_notebook));
  notebook = BAIL_NOTEBOOK (batk_obj);
  create_notebook_page_accessible (notebook, btk_notebook, page_num, FALSE, NULL);
  notebook->page_count++;
}

static void
bail_notebook_real_initialize (BatkObject *obj,
                               gpointer  data)
{
  BailNotebook *notebook;
  BtkNotebook *btk_notebook;
  gint i;

  BATK_OBJECT_CLASS (bail_notebook_parent_class)->initialize (obj, data);

  notebook = BAIL_NOTEBOOK (obj);
  btk_notebook = BTK_NOTEBOOK (data);
  for (i = 0; i < g_list_length (btk_notebook->children); i++)
    {
      create_notebook_page_accessible (notebook, btk_notebook, i, FALSE, NULL);
    }
  notebook->page_count = i;
  notebook->selected_page = btk_notebook_get_current_page (btk_notebook);
  if (btk_notebook->focus_tab && btk_notebook->focus_tab->data)
    {
      notebook->focus_tab_page = g_list_index (btk_notebook->children, btk_notebook->focus_tab->data);
    }
  g_signal_connect (btk_notebook,
                    "focus",
                    G_CALLBACK (bail_notebook_focus_cb),
                    NULL);
  g_signal_connect (btk_notebook,
                    "page-added",
                    G_CALLBACK (bail_notebook_page_added),
                    NULL);
  g_object_weak_ref (B_OBJECT(btk_notebook),
                     (GWeakNotify) bail_notebook_destroyed,
                     obj);                     

  obj->role = BATK_ROLE_PAGE_TAB_LIST;
}

static void
bail_notebook_real_notify_btk (BObject           *obj,
                               BParamSpec        *pspec)
{
  BtkWidget *widget;
  BatkObject* batk_obj;

  widget = BTK_WIDGET (obj);
  batk_obj = btk_widget_get_accessible (widget);

  if (strcmp (pspec->name, "page") == 0)
    {
      gint page_num, old_page_num;
      gint focus_page_num = 0;
      gint old_focus_page_num;
      BailNotebook *bail_notebook;
      BtkNotebook *btk_notebook;
     
      bail_notebook = BAIL_NOTEBOOK (batk_obj);
      btk_notebook = BTK_NOTEBOOK (widget);
     
      if (bail_notebook->page_count < g_list_length (btk_notebook->children))
       check_cache (bail_notebook, btk_notebook);
      /*
       * Notify SELECTED state change for old and new page
       */
      old_page_num = bail_notebook->selected_page;
      page_num = btk_notebook_get_current_page (btk_notebook);
      bail_notebook->selected_page = page_num;
      old_focus_page_num = bail_notebook->focus_tab_page;
      if (btk_notebook->focus_tab && btk_notebook->focus_tab->data)
        {
          focus_page_num = g_list_index (btk_notebook->children, btk_notebook->focus_tab->data);
          bail_notebook->focus_tab_page = focus_page_num;
        }
    
      if (page_num != old_page_num)
        {
          BatkObject *obj;

          if (old_page_num != -1)
            {
              obj = bail_notebook_ref_child (batk_obj, old_page_num);
              if (obj)
                {
                  batk_object_notify_state_change (obj,
                                                  BATK_STATE_SELECTED,
                                                  FALSE);
                  g_object_unref (obj);
                }
            }
          obj = bail_notebook_ref_child (batk_obj, page_num);
          if (obj)
            {
              batk_object_notify_state_change (obj,
                                              BATK_STATE_SELECTED,
                                              TRUE);
              g_object_unref (obj);
              /*
               * The page which is being displayed has changed but there is
               * no need to tell the focus tracker as the focus page will also 
               * change or a widget in the page will receive focus if the
               * Notebook does not have tabs.
               */
            }
          g_signal_emit_by_name (batk_obj, "selection_changed");
          g_signal_emit_by_name (batk_obj, "visible_data_changed");
        }
      if (btk_notebook_get_show_tabs (btk_notebook) &&
         (focus_page_num != old_focus_page_num))
        {
          if (bail_notebook->idle_focus_id)
            g_source_remove (bail_notebook->idle_focus_id);
          bail_notebook->idle_focus_id = bdk_threads_add_idle (bail_notebook_check_focus_tab, batk_obj);
        }
    }
  else
    BAIL_WIDGET_CLASS (bail_notebook_parent_class)->notify_btk (obj, pspec);
}

static void
bail_notebook_finalize (BObject            *object)
{
  BailNotebook *notebook = BAIL_NOTEBOOK (object);
  GList *list;

  /*
   * Get rid of the BailNotebookPage objects which we have cached.
   */
  list = notebook->page_cache;
  if (list != NULL)
    {
      while (list)
        {
          g_object_unref (list->data);
          list = list->next;
        }
    }

  g_list_free (notebook->page_cache);

  if (notebook->idle_focus_id)
    g_source_remove (notebook->idle_focus_id);

  B_OBJECT_CLASS (bail_notebook_parent_class)->finalize (object);
}

static void
batk_selection_interface_init (BatkSelectionIface *iface)
{
  iface->add_selection = bail_notebook_add_selection;
  iface->ref_selection = bail_notebook_ref_selection;
  iface->get_selection_count = bail_notebook_get_selection_count;
  iface->is_child_selected = bail_notebook_is_child_selected;
  /*
   * The following don't make any sense for BtkNotebook widgets.
   * Unsupported BatkSelection interfaces:
   * clear_selection();
   * remove_selection();
   * select_all_selection();
   */
}

/*
 * BtkNotebook only supports the selection of one page at a time. 
 * Selecting a page unselects any previous selection, so this 
 * changes the current selection instead of adding to it.
 */
static gboolean
bail_notebook_add_selection (BatkSelection *selection,
                             gint         i)
{
  BtkNotebook *notebook;
  BtkWidget *widget;
  
  widget =  BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;
  
  notebook = BTK_NOTEBOOK (widget);
  btk_notebook_set_current_page (notebook, i);
  return TRUE;
}

static BatkObject*
bail_notebook_ref_selection (BatkSelection *selection,
                             gint i)
{
  BatkObject *accessible;
  BtkWidget *widget;
  BtkNotebook *notebook;
  gint pagenum;
  
  /*
   * A note book can have only one selection.
   */
  bail_return_val_if_fail (i == 0, NULL);
  g_return_val_if_fail (BAIL_IS_NOTEBOOK (selection), NULL);
  
  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /* State is defunct */
	return NULL;
  
  notebook = BTK_NOTEBOOK (widget);
  pagenum = btk_notebook_get_current_page (notebook);
  bail_return_val_if_fail (pagenum != -1, NULL);
  accessible = bail_notebook_ref_child (BATK_OBJECT (selection), pagenum);

  return accessible;
}

/*
 * Always return 1 because there can only be one page
 * selected at any time
 */
static gint
bail_notebook_get_selection_count (BatkSelection *selection)
{
  BtkWidget *widget;
  BtkNotebook *notebook;
  
  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;

  notebook = BTK_NOTEBOOK (widget);
  if (notebook == NULL || btk_notebook_get_current_page (notebook) == -1)
    return 0;
  else
    return 1;
}

static gboolean
bail_notebook_is_child_selected (BatkSelection *selection,
                                 gint i)
{
  BtkWidget *widget;
  BtkNotebook *notebook;
  gint pagenumber;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /* 
     * State is defunct
     */
    return FALSE;

  
  notebook = BTK_NOTEBOOK (widget);
  pagenumber = btk_notebook_get_current_page(notebook);

  if (pagenumber == i)
    return TRUE;
  else
    return FALSE; 
}

static BatkObject*
find_child_in_list (GList *list,
                    gint  index)
{
  BatkObject *obj = NULL;

  while (list)
    {
      if (BAIL_NOTEBOOK_PAGE (list->data)->index == index)
        {
          obj = BATK_OBJECT (list->data);
          break;
        }
      list = list->next;
    }
  return obj;
}

static void
check_cache (BailNotebook *bail_notebook,
             BtkNotebook  *notebook)
{
  GList *btk_list;
  GList *bail_list;
  gint i;

  btk_list = notebook->children;
  bail_list = bail_notebook->page_cache;

  i = 0;
  while (btk_list)
    {
      if (!bail_list)
        {
          create_notebook_page_accessible (bail_notebook, notebook, i, FALSE, NULL);
        }
      else if (BAIL_NOTEBOOK_PAGE (bail_list->data)->page != btk_list->data)
        {
          create_notebook_page_accessible (bail_notebook, notebook, i, TRUE, bail_list);
        }
      else
        {
          bail_list = bail_list->next;
        }
      i++;
      btk_list = btk_list->next;
    }
  bail_notebook->page_count = i;
}

static void
reset_cache (BailNotebook *bail_notebook,
             gint         index)
{
  GList *l;

  for (l = bail_notebook->page_cache; l; l = l->next)
    {
      if (BAIL_NOTEBOOK_PAGE (l->data)->index > index)
        BAIL_NOTEBOOK_PAGE (l->data)->index -= 1;
    }
}

static void
create_notebook_page_accessible (BailNotebook *bail_notebook,
                                 BtkNotebook  *notebook,
                                 gint         index,
                                 gboolean     insert_before,
                                 GList        *list)
{
  BatkObject *obj;

  obj = bail_notebook_page_new (notebook, index);
  g_object_ref (obj);
  if (insert_before)
    bail_notebook->page_cache = g_list_insert_before (bail_notebook->page_cache, list, obj);
  else
    bail_notebook->page_cache = g_list_append (bail_notebook->page_cache, obj);
  g_signal_connect (btk_notebook_get_nth_page (notebook, index), 
                    "parent_set",
                    G_CALLBACK (bail_notebook_child_parent_set),
                    obj);
}

static void
bail_notebook_child_parent_set (BtkWidget *widget,
                                BtkWidget *old_parent,
                                gpointer   data)
{
  BailNotebook *bail_notebook;

  bail_return_if_fail (old_parent != NULL);
  bail_notebook = BAIL_NOTEBOOK (btk_widget_get_accessible (old_parent));
  bail_notebook->remove_index = BAIL_NOTEBOOK_PAGE (data)->index;
}

static gint
bail_notebook_real_remove_btk (BtkContainer *container,
                               BtkWidget    *widget,
                               gpointer      data)    
{
  BailNotebook *bail_notebook;
  BatkObject *obj;
  gint index;

  g_return_val_if_fail (container != NULL, 1);
  bail_notebook = BAIL_NOTEBOOK (btk_widget_get_accessible (BTK_WIDGET (container)));
  index = bail_notebook->remove_index;
  bail_notebook->remove_index = -1;

  obj = find_child_in_list (bail_notebook->page_cache, index);
  g_return_val_if_fail (obj, 1);
  bail_notebook->page_cache = g_list_remove (bail_notebook->page_cache, obj);
  bail_notebook->page_count -= 1;
  reset_cache (bail_notebook, index);
  g_signal_emit_by_name (bail_notebook,
                         "children_changed::remove",
                          BAIL_NOTEBOOK_PAGE (obj)->index, 
                          obj, NULL);
  g_object_unref (obj);
  return 1;
}

static gboolean
bail_notebook_focus_cb (BtkWidget      *widget,
                        BtkDirectionType type)
{
  BatkObject *batk_obj = btk_widget_get_accessible (widget);
  BailNotebook *bail_notebook = BAIL_NOTEBOOK (batk_obj);

  switch (type)
    {
    case BTK_DIR_LEFT:
    case BTK_DIR_RIGHT:
      if (bail_notebook->idle_focus_id == 0)
        bail_notebook->idle_focus_id = bdk_threads_add_idle (bail_notebook_check_focus_tab, batk_obj);
      break;
    default:
      break;
    }
  return FALSE;
}

static gboolean
bail_notebook_check_focus_tab (gpointer data)
{
  BtkWidget *widget;
  BatkObject *batk_obj;
  gint focus_page_num, old_focus_page_num;
  BailNotebook *bail_notebook;
  BtkNotebook *btk_notebook;

  batk_obj = BATK_OBJECT (data);
  bail_notebook = BAIL_NOTEBOOK (batk_obj);
  widget = BTK_ACCESSIBLE (batk_obj)->widget;

  btk_notebook = BTK_NOTEBOOK (widget);

  bail_notebook->idle_focus_id = 0;

  if (!btk_notebook->focus_tab)
    return FALSE;

  old_focus_page_num = bail_notebook->focus_tab_page;
  focus_page_num = g_list_index (btk_notebook->children, btk_notebook->focus_tab->data);
  bail_notebook->focus_tab_page = focus_page_num;
  if (old_focus_page_num != focus_page_num)
    {
      BatkObject *obj;

      obj = batk_object_ref_accessible_child (batk_obj, focus_page_num);
      batk_focus_tracker_notify (obj);
      g_object_unref (obj);
    }

  return FALSE;
}

static void
bail_notebook_destroyed (gpointer data)
{
  BailNotebook *bail_notebook = BAIL_NOTEBOOK (data);

  if (bail_notebook->idle_focus_id)
    {
      g_source_remove (bail_notebook->idle_focus_id);
      bail_notebook->idle_focus_id = 0;
    }
}
