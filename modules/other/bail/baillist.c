/* BAIL - The BUNNY Accessibility Enabling Library
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

#undef BTK_DISABLE_DEPRECATED

#include <btk/btk.h>
#include "baillist.h"
#include "bailcombo.h"

static void         bail_list_class_init            (BailListClass  *klass); 

static void         bail_list_init                  (BailList       *list);

static void         bail_list_initialize            (BatkObject      *accessible,
                                                     bpointer        data);

static bint         bail_list_get_index_in_parent   (BatkObject      *accessible);

static void         batk_selection_interface_init    (BatkSelectionIface *iface);
static bboolean     bail_list_add_selection         (BatkSelection   *selection,
                                                     bint           i);
static bboolean     bail_list_clear_selection       (BatkSelection   *selection);
static BatkObject*   bail_list_ref_selection         (BatkSelection   *selection,
                                                     bint           i);
static bint         bail_list_get_selection_count   (BatkSelection   *selection);
static bboolean     bail_list_is_child_selected     (BatkSelection   *selection,
                                                     bint           i);
static bboolean     bail_list_remove_selection      (BatkSelection   *selection,
                                                     bint           i);


G_DEFINE_TYPE_WITH_CODE (BailList, bail_list, BAIL_TYPE_CONTAINER,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_SELECTION, batk_selection_interface_init))

static void
bail_list_class_init (BailListClass *klass)
{
  BatkObjectClass  *class = BATK_OBJECT_CLASS (klass);

  class->initialize = bail_list_initialize;
  class->get_index_in_parent = bail_list_get_index_in_parent;
}

static void
bail_list_init (BailList *list)
{
}

static void
bail_list_initialize (BatkObject *accessible,
                      bpointer  data)
{
  BATK_OBJECT_CLASS (bail_list_parent_class)->initialize (accessible, data);

  accessible->role = BATK_ROLE_LIST;
}

static bint
bail_list_get_index_in_parent (BatkObject *accessible)
{
  /*
   * If the parent widget is a combo box then the index is 0
   * otherwise do the normal thing.
   */
  if (accessible->accessible_parent)
  {
    if (BAIL_IS_COMBO (accessible->accessible_parent))
      return 0;
  }
  return BATK_OBJECT_CLASS (bail_list_parent_class)->get_index_in_parent (accessible);
}

static void
batk_selection_interface_init (BatkSelectionIface *iface)
{
  iface->add_selection = bail_list_add_selection;
  iface->clear_selection = bail_list_clear_selection;
  iface->ref_selection = bail_list_ref_selection;
  iface->get_selection_count = bail_list_get_selection_count;
  iface->is_child_selected = bail_list_is_child_selected;
  iface->remove_selection = bail_list_remove_selection;
  /*
   * select_all_selection does not make sense for a combo box
   * so no implementation is provided.
   */
}

static bboolean
bail_list_add_selection (BatkSelection   *selection,
                         bint           i)
{
  BtkList *list;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  list = BTK_LIST (widget);

  btk_list_select_item (list, i);
  return TRUE;
}

static bboolean 
bail_list_clear_selection (BatkSelection *selection)
{
  BtkList *list;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  list = BTK_LIST (widget);

  btk_list_unselect_all (list);
  return TRUE;
}

static BatkObject*
bail_list_ref_selection (BatkSelection *selection,
                         bint         i)
{
  BtkList *list;
  GList *g_list;
  BtkWidget *item;
  BatkObject *obj;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  list = BTK_LIST (widget);

  /*
   * A combo box can have only one selection.
   */
  if (i != 0)
    return NULL;

  g_list = list->selection;

  if (g_list == NULL)
    return NULL;

  item = BTK_WIDGET (g_list->data);

  obj = btk_widget_get_accessible (item);
  g_object_ref (obj);
  return obj;
}

static bint
bail_list_get_selection_count (BatkSelection *selection)
{
  BtkList *list;
  GList *g_list;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;

  list = BTK_LIST (widget);

  g_list = list->selection;

  return g_list_length (g_list);;
}

static bboolean
bail_list_is_child_selected (BatkSelection *selection,
                             bint         i)
{
  BtkList *list;
  GList *g_list;
  BtkWidget *item;
  bint j;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  list = BTK_LIST (widget);

  g_list = list->selection;

  if (g_list == NULL)
    return FALSE;

  item = BTK_WIDGET (g_list->data);

  j = g_list_index (list->children, item);

  return (j == i);
}

static bboolean
bail_list_remove_selection (BatkSelection   *selection,
                             bint           i)
{
  if (batk_selection_is_child_selected (selection, i))
    batk_selection_clear_selection (selection);

  return TRUE;
}
