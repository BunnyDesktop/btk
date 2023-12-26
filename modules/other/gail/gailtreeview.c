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
#ifdef BDK_WINDOWING_X11
#include <bdk/x11/bdkx.h>
#endif
#include "bailtreeview.h"
#include "bailrenderercell.h"
#include "bailbooleancell.h"
#include "bailcontainercell.h"
#include "bailtextcell.h"
#include "bailcellparent.h"
#include "bail-private-macros.h"

typedef struct _BailTreeViewRowInfo    BailTreeViewRowInfo;
typedef struct _BailTreeViewCellInfo   BailTreeViewCellInfo;

static void             bail_tree_view_class_init       (BailTreeViewClass      *klass);
static void             bail_tree_view_init             (BailTreeView           *view);
static void             bail_tree_view_real_initialize  (BatkObject              *obj,
                                                         gpointer               data);
static void             bail_tree_view_real_notify_btk  (GObject		*obj,
                                                         GParamSpec		*pspec);
static void             bail_tree_view_finalize         (GObject                *object);

static void             bail_tree_view_connect_widget_destroyed 
                                                        (BtkAccessible          *accessible);
static void             bail_tree_view_destroyed        (BtkWidget              *widget,
                                                         BtkAccessible          *accessible); 
/* batkobject.h */

static gint             bail_tree_view_get_n_children   (BatkObject              *obj);
static BatkObject*       bail_tree_view_ref_child        (BatkObject              *obj,
                                                         gint                   i);
static BatkStateSet*     bail_tree_view_ref_state_set    (BatkObject              *obj);

/* batkcomponent.h */

static void             batk_component_interface_init    (BatkComponentIface      *iface);

static BatkObject*       bail_tree_view_ref_accessible_at_point
                                                        (BatkComponent           *component,
                                                         gint                   x,
                                                         gint                   y,
                                                         BatkCoordType           coord_type);
           
/* batktable.h */

static void             batk_table_interface_init        (BatkTableIface          *iface);

static gint             bail_tree_view_get_index_at     (BatkTable               *table,
                                                         gint                   row,
                                                         gint                   column);
static gint             bail_tree_view_get_column_at_index
                                                        (BatkTable               *table,
                                                         gint                   index);
static gint             bail_tree_view_get_row_at_index (BatkTable               *table,
                                                         gint                   index);

static BatkObject*       bail_tree_view_table_ref_at     (BatkTable               *table,
                                                         gint                   row,
                                                         gint                   column);
static gint             bail_tree_view_get_n_rows       (BatkTable               *table);
static gint             bail_tree_view_get_n_columns    (BatkTable               *table);
static gint             get_n_actual_columns            (BtkTreeView            *tree_view);
static gboolean         bail_tree_view_is_row_selected  (BatkTable               *table,
                                                         gint                   row);
static gboolean         bail_tree_view_is_selected      (BatkTable               *table,
                                                         gint                   row,
                                                         gint                   column);
static gint             bail_tree_view_get_selected_rows 
                                                        (BatkTable               *table, 
                                                         gint                   **selected);
static gboolean         bail_tree_view_add_row_selection 
                                                        (BatkTable               *table, 
                                                         gint                   row);
static gboolean         bail_tree_view_remove_row_selection 
                                                        (BatkTable               *table, 
                                                         gint                   row);
static BatkObject*       bail_tree_view_get_row_header   (BatkTable               *table,
                                                         gint                   row);
static BatkObject*       bail_tree_view_get_column_header 
                                                        (BatkTable               *table,
                                                         gint                   column);
static void             bail_tree_view_set_row_header   (BatkTable               *table,
                                                         gint                   row,
                                                         BatkObject              *header);
static void             bail_tree_view_set_column_header 
                                                        (BatkTable               *table,
                                                         gint                   column,
                                                         BatkObject              *header);
static BatkObject*
                        bail_tree_view_get_caption      (BatkTable               *table);
static void             bail_tree_view_set_caption      (BatkTable               *table,
                                                         BatkObject              *caption);
static BatkObject*       bail_tree_view_get_summary      (BatkTable               *table);
static void             bail_tree_view_set_summary      (BatkTable               *table,
                                                         BatkObject              *accessible);
static const gchar*     bail_tree_view_get_row_description
                                                        (BatkTable               *table,
                                                         gint                   row);
static void             bail_tree_view_set_row_description 
                                                        (BatkTable               *table,
                                                         gint                   row,
                                                         const gchar            *description);
static const gchar*     bail_tree_view_get_column_description
                                                        (BatkTable               *table,
                                                         gint                   column);
static void             bail_tree_view_set_column_description
                                                        (BatkTable               *table,
                                                         gint                   column,
                                                         const gchar            *description);

static void             set_row_data                    (BatkTable               *table,
                                                         gint                   row,
                                                         BatkObject              *header,
                                                         const gchar            *description,
                                                         gboolean               is_header);
static BailTreeViewRowInfo* 
                        get_row_info                    (BatkTable               *table,
                                                         gint                   row);

/* batkselection.h */

static void             batk_selection_interface_init    (BatkSelectionIface      *iface);
static gboolean         bail_tree_view_add_selection    (BatkSelection           *selection,
                                                         gint                   i);
static gboolean         bail_tree_view_clear_selection  (BatkSelection           *selection);
static BatkObject*       bail_tree_view_ref_selection    (BatkSelection           *selection,
                                                         gint                   i);
static gint             bail_tree_view_get_selection_count 
                                                        (BatkSelection           *selection);
static gboolean         bail_tree_view_is_child_selected 
                                                        (BatkSelection           *selection,
                                                         gint                   i);

/* bailcellparent.h */

static void             bail_cell_parent_interface_init (BailCellParentIface    *iface);
static void             bail_tree_view_get_cell_extents (BailCellParent         *parent,
                                                         BailCell               *cell,
                                                         gint                   *x,
                                                         gint                   *y,
                                                         gint                   *width,
                                                         gint                   *height,
                                                         BatkCoordType           coord_type);
static void             bail_tree_view_get_cell_area    (BailCellParent         *parent,
                                                         BailCell               *cell,
                                                         BdkRectangle           *cell_rect);
static gboolean         bail_tree_view_grab_cell_focus  (BailCellParent         *parent,
                                                         BailCell               *cell);

/* signal handling */

static gboolean         bail_tree_view_expand_row_btk   (BtkTreeView            *tree_view,
                                                         BtkTreeIter            *iter,
                                                         BtkTreePath            *path);
static gint             idle_expand_row                 (gpointer               data);
static gboolean         bail_tree_view_collapse_row_btk (BtkTreeView            *tree_view,
                                                         BtkTreeIter            *iter,
                                                         BtkTreePath            *path);
static void             bail_tree_view_size_allocate_btk (BtkWidget             *widget,
                                                         BtkAllocation          *allocation);
static void             bail_tree_view_set_scroll_adjustments
                                                        (BtkWidget              *widget,
                                                         BtkAdjustment          *hadj,
                                                         BtkAdjustment          *vadj);
static void             bail_tree_view_changed_btk      (BtkTreeSelection       *selection,
                                                         gpointer               data);

static void             columns_changed                 (BtkTreeView            *tree_view);
static void             cursor_changed                  (BtkTreeView            *tree_view);
static gint             idle_cursor_changed             (gpointer               data);
static gboolean         focus_in                        (BtkWidget		*widget);
static gboolean         focus_out                       (BtkWidget              *widget);

static void             model_row_changed               (BtkTreeModel           *tree_model,
                                                         BtkTreePath            *path,
                                                         BtkTreeIter            *iter,
                                                         gpointer               user_data);
static void             column_visibility_changed       (GObject                *object,
                                                         GParamSpec             *param,
                                                         gpointer               user_data);
static void             column_destroy                  (BtkObject              *obj); 
static void             model_row_inserted              (BtkTreeModel           *tree_model,
                                                         BtkTreePath            *path,
                                                         BtkTreeIter            *iter,
                                                         gpointer               user_data);
static void             model_row_deleted               (BtkTreeModel           *tree_model,
                                                         BtkTreePath            *path,
                                                         gpointer               user_data);
static void             destroy_count_func              (BtkTreeView            *tree_view,
                                                         BtkTreePath            *path,
                                                         gint                   count,
                                                         gpointer               user_data);
static void             model_rows_reordered            (BtkTreeModel           *tree_model,
                                                         BtkTreePath            *path,
                                                         BtkTreeIter            *iter,
                                                         gint                   *new_order,
                                                         gpointer               user_data);
static void             adjustment_changed              (BtkAdjustment          *adjustment,
                                                         BtkTreeView            *tree_view);

/* Misc */

static gboolean         set_iter_nth_row                (BtkTreeView            *tree_view,
                                                         BtkTreeIter            *iter,
                                                         gint                   row);
static gint             get_row_from_tree_path          (BtkTreeView            *tree_view,
                                                         BtkTreePath            *path);
static BtkTreeViewColumn* get_column                    (BtkTreeView            *tree_view,
                                                         gint                   in_col);
static gint             get_actual_column_number        (BtkTreeView            *tree_view,
                                                         gint                   visible_column);
static gint             get_visible_column_number       (BtkTreeView            *tree_view,
                                                         gint                   actual_column);
static void		iterate_thru_children           (BtkTreeView            *tree_view,
                                                         BtkTreeModel           *tree_model,
                                                         BtkTreePath            *tree_path,
                                                         BtkTreePath            *orig,
                                                         gint                   *count,
                                                         gint                   depth);
static BtkTreeIter*     return_iter_nth_row             (BtkTreeView            *tree_view,
                                                         BtkTreeModel           *tree_model,
                                                         BtkTreeIter            *iter,
                                                         gint                   increment,
                                                         gint                   row);
static void             free_row_info                   (GArray                 *array,
                                                         gint                   array_idx,
                                                         gboolean               shift);
static void             clean_cell_info                 (BailTreeView           *tree_view,
                                                         GList                  *list); 
static void             clean_rows                      (BailTreeView           *tree_view);
static void             clean_cols                      (BailTreeView           *tree_view,
                                                         BtkTreeViewColumn      *tv_col);
static void             traverse_cells                  (BailTreeView           *tree_view,
                                                         BtkTreePath            *tree_path,
                                                         gboolean               set_stale,
                                                         gboolean               inc_row);
static gboolean         update_cell_value               (BailRendererCell       *renderer_cell,
                                                         BailTreeView           *bailview,
                                                         gboolean               emit_change_signal);
static void             set_cell_visibility             (BtkTreeView            *tree_view,
                                                         BailCell               *cell,
                                                         BtkTreeViewColumn      *tv_col,
                                                         BtkTreePath            *tree_path,
                                                         gboolean               emit_signal);
static gboolean         is_cell_showing                 (BtkTreeView            *tree_view,
                                                         BdkRectangle           *cell_rect);
static void             set_expand_state                (BtkTreeView            *tree_view,
                                                         BtkTreeModel           *tree_model,
                                                         BailTreeView           *bailview,
                                                         BtkTreePath            *tree_path,
                                                         gboolean               set_on_ancestor);
static void             add_cell_actions                (BailCell               *cell,
                                                         gboolean               editable);

static void             toggle_cell_expanded            (BailCell               *cell);
static void             toggle_cell_toggled             (BailCell               *cell);
static void             edit_cell                       (BailCell               *cell);
static void             activate_cell                   (BailCell               *cell);
static void             cell_destroyed                  (gpointer               data);
#if 0
static void             cell_info_remove                (BailTreeView           *tree_view, 
                                                         BailCell               *cell);
#endif
static void             cell_info_get_index             (BtkTreeView            *tree_view, 
                                                         BailTreeViewCellInfo   *info,
                                                         gint                   *index);
static void             cell_info_new                   (BailTreeView           *bailview, 
                                                         BtkTreeModel           *tree_model,
                                                         BtkTreePath            *path,
                                                         BtkTreeViewColumn      *tv_col,
                                                         BailCell               *cell);
static BailCell*        find_cell                       (BailTreeView           *bailview, 
                                                         gint                   index);
static void             refresh_cell_index              (BailCell               *cell);
static void             get_selected_rows               (BtkTreeModel           *model,
                                                         BtkTreePath            *path,
                                                         BtkTreeIter            *iter,
                                                         gpointer               data);
static void             connect_model_signals           (BtkTreeView            *view,
                                                         BailTreeView           *bailview); 
static void             disconnect_model_signals        (BailTreeView           *bailview); 
static void             clear_cached_data               (BailTreeView           *view);
static gint             get_column_number               (BtkTreeView            *tree_view,
                                                         BtkTreeViewColumn      *column,
                                                         gboolean               visible); 
static gint             get_focus_index                 (BtkTreeView            *tree_view);
static gint             get_index                       (BtkTreeView            *tree_view,
                                                         BtkTreePath            *path,
                                                         gint                   actual_column);
static void             count_rows                      (BtkTreeModel           *model,
                                                         BtkTreeIter            *iter,
                                                         BtkTreePath            *end_path,
                                                         gint                   *count,
                                                         gint                   level,
                                                         gint                   depth);

static gboolean         get_next_node_with_child_at_depth 
                                                        (BtkTreeModel           *model,
                                                         BtkTreeIter            *iter,
                                                         BtkTreePath            **path,
                                                         gint                   level,
                                                         gint                   depth);
static gboolean         get_next_node_with_child        (BtkTreeModel           *model,
                                                         BtkTreePath            *path,
                                                         BtkTreePath            **return_path);
static gboolean         get_tree_path_from_row_index    (BtkTreeModel           *model,
                                                         gint                   row_index,
                                                         BtkTreePath            **tree_path);
static gint             get_row_count                   (BtkTreeModel           *model);
static gboolean         get_path_column_from_index      (BtkTreeView            *tree_view,
                                                         gint                   index,
                                                         BtkTreePath            **path,
                                                         BtkTreeViewColumn      **column);
static void             set_cell_expandable             (BailCell               *cell);

static BailTreeViewCellInfo* find_cell_info             (BailTreeView           *view,
                                                         BailCell               *cell,
                                                          GList**                list,
							 gboolean                live_only);
static BatkObject *       get_header_from_column         (BtkTreeViewColumn      *tv_col);
static gboolean          idle_garbage_collect_cell_data (gpointer data);
static gboolean          garbage_collect_cell_data      (gpointer data);

static GQuark quark_column_desc_object = 0;
static GQuark quark_column_header_object = 0;
static gboolean editing = FALSE;
static const gchar* hadjustment = "hadjustment";
static const gchar* vadjustment = "vadjustment";

struct _BailTreeViewRowInfo
{
  BtkTreeRowReference *row_ref;
  gchar *description;
  BatkObject *header;
};

struct _BailTreeViewCellInfo
{
  BailCell *cell;
  BtkTreeRowReference *cell_row_ref;
  BtkTreeViewColumn *cell_col_ref;
  BailTreeView *view;
  gboolean in_use;
};

G_DEFINE_TYPE_WITH_CODE (BailTreeView, bail_tree_view, BAIL_TYPE_CONTAINER,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_TABLE, batk_table_interface_init)
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_SELECTION, batk_selection_interface_init)
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_COMPONENT, batk_component_interface_init)
                         G_IMPLEMENT_INTERFACE (BAIL_TYPE_CELL_PARENT, bail_cell_parent_interface_init))

static void
bail_tree_view_class_init (BailTreeViewClass *klass)
{
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);
  GObjectClass *bobject_class = G_OBJECT_CLASS (klass);
  BtkAccessibleClass *accessible_class;
  BailWidgetClass *widget_class;
  BailContainerClass *container_class;

  accessible_class = (BtkAccessibleClass*)klass;
  widget_class = (BailWidgetClass*)klass;
  container_class = (BailContainerClass*)klass;

  class->get_n_children = bail_tree_view_get_n_children;
  class->ref_child = bail_tree_view_ref_child;
  class->ref_state_set = bail_tree_view_ref_state_set;
  class->initialize = bail_tree_view_real_initialize;

  widget_class->notify_btk = bail_tree_view_real_notify_btk;

  accessible_class->connect_widget_destroyed = bail_tree_view_connect_widget_destroyed;

  /*
   * The children of a BtkTreeView are the buttons at the top of the columns
   * we do not represent these as children so we do not want to report
   * children added or deleted when these changed.
   */
  container_class->add_btk = NULL;
  container_class->remove_btk = NULL;

  bobject_class->finalize = bail_tree_view_finalize;

  quark_column_desc_object = g_quark_from_static_string ("btk-column-object");
  quark_column_header_object = g_quark_from_static_string ("btk-header-object");
}

static void
bail_tree_view_init (BailTreeView *view)
{
}

static void
bail_tree_view_real_initialize (BatkObject *obj,
                                gpointer  data)
{
  BailTreeView *view;
  BtkTreeView *tree_view;
  BtkTreeModel *tree_model; 
  BtkAdjustment *adj;
  GList *tv_cols, *tmp_list;
  BtkWidget *widget;

  BATK_OBJECT_CLASS (bail_tree_view_parent_class)->initialize (obj, data);

  view = BAIL_TREE_VIEW (obj);
  view->caption = NULL;
  view->summary = NULL;
  view->row_data = NULL;
  view->col_data = NULL;
  view->cell_data = NULL;
  view->focus_cell = NULL;
  view->old_hadj = NULL;
  view->old_vadj = NULL;
  view->idle_expand_id = 0;
  view->idle_expand_path = NULL;

  view->n_children_deleted = 0;

  widget = BTK_WIDGET (data);
  g_signal_connect_after (widget,
                          "row-collapsed",
                          G_CALLBACK (bail_tree_view_collapse_row_btk),
                          NULL);
  g_signal_connect (widget,
                    "row-expanded",
                    G_CALLBACK (bail_tree_view_expand_row_btk),
                    NULL);
  g_signal_connect (widget,
                    "size-allocate",
                    G_CALLBACK (bail_tree_view_size_allocate_btk),
                    NULL);

  tree_view = BTK_TREE_VIEW (widget);
  tree_model = btk_tree_view_get_model (tree_view);

  /* Set up signal handling */

  g_signal_connect_data (btk_tree_view_get_selection (tree_view),
                         "changed",
                         (GCallback) bail_tree_view_changed_btk,
                      	 obj, NULL, 0);

  g_signal_connect_data (tree_view, "columns-changed",
    (GCallback) columns_changed, NULL, NULL, 0);
  g_signal_connect_data (tree_view, "cursor-changed",
    (GCallback) cursor_changed, NULL, NULL, 0);
  g_signal_connect_data (BTK_WIDGET (tree_view), "focus-in-event",
    (GCallback) focus_in, NULL, NULL, 0);
  g_signal_connect_data (BTK_WIDGET (tree_view), "focus-out-event",
    (GCallback) focus_out, NULL, NULL, 0);

  view->tree_model = tree_model;
  if (tree_model)
    {
      g_object_add_weak_pointer (G_OBJECT (view->tree_model), (gpointer *)&view->tree_model);
      connect_model_signals (tree_view, view);

      if (btk_tree_model_get_flags (tree_model) & BTK_TREE_MODEL_LIST_ONLY)
        obj->role = BATK_ROLE_TABLE;
      else
        obj->role = BATK_ROLE_TREE_TABLE;
    }
  else
    {
      obj->role = BATK_ROLE_UNKNOWN;
    }

  /* adjustment callbacks */

  g_object_get (tree_view, hadjustment, &adj, NULL);
  view->old_hadj = adj;
  g_object_add_weak_pointer (G_OBJECT (view->old_hadj), (gpointer *)&view->old_hadj);
  g_signal_connect (adj, 
                    "value_changed",
                    G_CALLBACK (adjustment_changed),
                    tree_view);

  g_object_get (tree_view, vadjustment, &adj, NULL);
  view->old_vadj = adj;
  g_object_add_weak_pointer (G_OBJECT (view->old_vadj), (gpointer *)&view->old_vadj);
  g_signal_connect (adj, 
                    "value_changed",
                    G_CALLBACK (adjustment_changed),
                    tree_view);
  g_signal_connect_after (widget,
                          "set_scroll_adjustments",
                          G_CALLBACK (bail_tree_view_set_scroll_adjustments),
                          NULL);

  view->col_data = g_array_sized_new (FALSE, TRUE, 
                                      sizeof(BtkTreeViewColumn *), 0);

  tv_cols = btk_tree_view_get_columns (tree_view);

  for (tmp_list = tv_cols; tmp_list; tmp_list = tmp_list->next)
    {
      g_signal_connect_data (tmp_list->data, "notify::visible",
       (GCallback)column_visibility_changed, 
        tree_view, NULL, FALSE);
      g_signal_connect_data (tmp_list->data, "destroy",
       (GCallback)column_destroy, 
        NULL, NULL, FALSE);
      g_array_append_val (view->col_data, tmp_list->data);
    }

  btk_tree_view_set_destroy_count_func (tree_view, 
                                        destroy_count_func,
                                        NULL, NULL);
  g_list_free (tv_cols);
}

static void
bail_tree_view_real_notify_btk (GObject             *obj,
                                GParamSpec          *pspec)
{
  BtkWidget *widget;
  BatkObject* batk_obj;
  BtkTreeView *tree_view;
  BailTreeView *bailview;
  BtkAdjustment *adj;

  widget = BTK_WIDGET (obj);
  batk_obj = btk_widget_get_accessible (widget);
  tree_view = BTK_TREE_VIEW (widget);
  bailview = BAIL_TREE_VIEW (batk_obj);

  if (strcmp (pspec->name, "model") == 0)
    {
      BtkTreeModel *tree_model;
      BatkRole role;

      tree_model = btk_tree_view_get_model (tree_view);
      if (bailview->tree_model)
        {
          g_object_remove_weak_pointer (G_OBJECT (bailview->tree_model), (gpointer *)&bailview->tree_model);
          disconnect_model_signals (bailview);
        }
      clear_cached_data (bailview);
      bailview->tree_model = tree_model;
      /*
       * if there is no model the BtkTreeView is probably being destroyed
       */
      if (tree_model)
        {
          g_object_add_weak_pointer (G_OBJECT (bailview->tree_model), (gpointer *)&bailview->tree_model);
          connect_model_signals (tree_view, bailview);

          if (btk_tree_model_get_flags (tree_model) & BTK_TREE_MODEL_LIST_ONLY)
            role = BATK_ROLE_TABLE;
          else
            role = BATK_ROLE_TREE_TABLE;
        }
      else
        {
          role = BATK_ROLE_UNKNOWN;
        }
      batk_object_set_role (batk_obj, role);
      g_object_freeze_notify (G_OBJECT (batk_obj));
      g_signal_emit_by_name (batk_obj, "model_changed");
      g_signal_emit_by_name (batk_obj, "visible_data_changed");
      g_object_thaw_notify (G_OBJECT (batk_obj));
    }
  else if (strcmp (pspec->name, hadjustment) == 0)
    {
      g_object_get (tree_view, hadjustment, &adj, NULL);
      g_signal_handlers_disconnect_by_func (bailview->old_hadj, 
                                           (gpointer) adjustment_changed,
                                           widget);
      bailview->old_hadj = adj;
      g_object_add_weak_pointer (G_OBJECT (bailview->old_hadj), (gpointer *)&bailview->old_hadj);
      g_signal_connect (adj, 
                        "value_changed",
                        G_CALLBACK (adjustment_changed),
                        tree_view);
    }
  else if (strcmp (pspec->name, vadjustment) == 0)
    {
      g_object_get (tree_view, vadjustment, &adj, NULL);
      g_signal_handlers_disconnect_by_func (bailview->old_vadj, 
                                           (gpointer) adjustment_changed,
                                           widget);
      bailview->old_vadj = adj;
      g_object_add_weak_pointer (G_OBJECT (bailview->old_hadj), (gpointer *)&bailview->old_vadj);
      g_signal_connect (adj, 
                        "value_changed",
                        G_CALLBACK (adjustment_changed),
                        tree_view);
    }
  else
    BAIL_WIDGET_CLASS (bail_tree_view_parent_class)->notify_btk (obj, pspec);
}

static void
bail_tree_view_finalize (GObject	    *object)
{
  BailTreeView *view = BAIL_TREE_VIEW (object);

  clear_cached_data (view);

  /* remove any idle handlers still pending */
  if (view->idle_garbage_collect_id)
    g_source_remove (view->idle_garbage_collect_id);
  if (view->idle_cursor_changed_id)
    g_source_remove (view->idle_cursor_changed_id);
  if (view->idle_expand_id)
    g_source_remove (view->idle_expand_id);

  if (view->caption)
    g_object_unref (view->caption);
  if (view->summary)
    g_object_unref (view->summary);

  if (view->tree_model)
    {
      g_object_remove_weak_pointer (G_OBJECT (view->tree_model), (gpointer *)&view->tree_model);
      disconnect_model_signals (view);
    }

  if (view->col_data)
    {
      GArray *array = view->col_data;

     /*
      * No need to free the contents of the array since it
      * just contains pointers to the BtkTreeViewColumn
      * objects that are in the BtkTreeView.
      */
      g_array_free (array, TRUE);
    }

  G_OBJECT_CLASS (bail_tree_view_parent_class)->finalize (object);
}

static void
bail_tree_view_connect_widget_destroyed (BtkAccessible *accessible)
{
  if (accessible->widget)
    {
      g_signal_connect_after (accessible->widget,
                              "destroy",
                              G_CALLBACK (bail_tree_view_destroyed),
                              accessible);
    }
  BTK_ACCESSIBLE_CLASS (bail_tree_view_parent_class)->connect_widget_destroyed (accessible);
}

static void
bail_tree_view_destroyed (BtkWidget *widget,
                          BtkAccessible *accessible)
{
  BtkAdjustment *adj;
  BailTreeView *bailview;

  bail_return_if_fail (BTK_IS_TREE_VIEW (widget));

  bailview = BAIL_TREE_VIEW (accessible);
  adj = bailview->old_hadj;
  if (adj)
    g_signal_handlers_disconnect_by_func (adj, 
                                          (gpointer) adjustment_changed,
                                          widget);
  adj = bailview->old_vadj;
  if (adj)
    g_signal_handlers_disconnect_by_func (adj, 
                                          (gpointer) adjustment_changed,
                                          widget);
  if (bailview->tree_model)
    {
      g_object_remove_weak_pointer (G_OBJECT (bailview->tree_model), (gpointer *)&bailview->tree_model);
      disconnect_model_signals (bailview);
      bailview->tree_model = NULL;
    }
  if (bailview->focus_cell)
    {
      g_object_unref (bailview->focus_cell);
      bailview->focus_cell = NULL;
    }
  if (bailview->idle_expand_id) 
    {
      g_source_remove (bailview->idle_expand_id);
      bailview->idle_expand_id = 0;
    }
}

gint 
get_focus_index (BtkTreeView *tree_view)
{
  BtkTreePath *focus_path;
  BtkTreeViewColumn *focus_column;
  gint index;

  btk_tree_view_get_cursor (tree_view, &focus_path, &focus_column);
  if (focus_path && focus_column)
    {

      index = get_index (tree_view, focus_path,
                         get_column_number (tree_view, focus_column, FALSE));
    }
  else
    index = -1;

  if (focus_path)
    btk_tree_path_free (focus_path);

  return index;
}

BatkObject *
bail_tree_view_ref_focus_cell (BtkTreeView *tree_view)
{
  /*
   * This function returns a reference to the accessible object for the cell
   * in the treeview which has focus, if a cell has focus.
   */
  BatkObject *focus_cell = NULL;
  BatkObject *batk_obj;
  gint focus_index;

  focus_index = get_focus_index (tree_view);
  if (focus_index >= 0)
    {
      batk_obj = btk_widget_get_accessible (BTK_WIDGET (tree_view));
      focus_cell = batk_object_ref_accessible_child (batk_obj, focus_index);
    }

  return focus_cell;
}

/* batkobject.h */

static gint
bail_tree_view_get_n_children (BatkObject *obj)
{
  BtkWidget *widget;
  BtkTreeView *tree_view;
  BtkTreeModel *tree_model;
  gint n_rows, n_cols;

  bail_return_val_if_fail (BAIL_IS_TREE_VIEW (obj), 0);

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;

  tree_view = BTK_TREE_VIEW (widget);
  tree_model = btk_tree_view_get_model (tree_view);

  /*
   * We get the total number of rows including those which are collapsed
   */
  n_rows = get_row_count (tree_model);
  /*
   * We get the total number of columns including those which are not visible
   */
  n_cols = get_n_actual_columns (tree_view);
  return (n_rows * n_cols);
}

static BatkObject*
bail_tree_view_ref_child (BatkObject *obj, 
                          gint      i)
{
  BtkWidget *widget;
  BailTreeView *bailview;
  BailCell *cell;
  BtkTreeView *tree_view;
  BtkTreeModel *tree_model; 
  BtkCellRenderer *renderer;
  BtkTreeIter iter;
  BtkTreeViewColumn *tv_col;
  BtkTreeSelection *selection;
  BtkTreePath *path;
  BatkRegistry *default_registry;
  BatkObjectFactory *factory;
  BatkObject *child;
  BatkObject *parent;
  BtkTreeViewColumn *expander_tv;
  GList *renderer_list;
  GList *l;
  BailContainerCell *container = NULL;
  BailRendererCell *renderer_cell;
  gboolean is_expander, is_expanded, retval;
  gboolean editable = FALSE;
  gint focus_index;

  g_return_val_if_fail (BAIL_IS_TREE_VIEW (obj), NULL);
  g_return_val_if_fail (i >= 0, NULL);

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  if (i >= bail_tree_view_get_n_children (obj))
    return NULL;

  tree_view = BTK_TREE_VIEW (widget);
  if (i < get_n_actual_columns (tree_view))
    {
      tv_col = btk_tree_view_get_column (tree_view, i);
      child = get_header_from_column (tv_col);
      if (child)
        g_object_ref (child);
      return child;
    }

  bailview = BAIL_TREE_VIEW (obj);
  /*
   * Check whether the child is cached
   */
  cell = find_cell (bailview, i);
  if (cell)
    {
      g_object_ref (cell);
      return BATK_OBJECT (cell);
    }

  if (bailview->focus_cell == NULL)
      focus_index = get_focus_index (tree_view);
  else
      focus_index = -1;
  /*
   * Find the TreePath and BtkTreeViewColumn for the index
   */
  if (!get_path_column_from_index (tree_view, i, &path, &tv_col))
    return NULL;
 
  tree_model = btk_tree_view_get_model (tree_view);
  retval = btk_tree_model_get_iter (tree_model, &iter, path);
  bail_return_val_if_fail (retval, NULL);

  expander_tv = btk_tree_view_get_expander_column (tree_view);
  is_expander = FALSE;
  is_expanded = FALSE;
  if (btk_tree_model_iter_has_child (tree_model, &iter))
    {
      if (expander_tv == tv_col)
        {
          is_expander = TRUE;
          is_expanded = btk_tree_view_row_expanded (tree_view, path);
        }
    } 
  btk_tree_view_column_cell_set_cell_data (tv_col, tree_model, &iter, 
                                           is_expander, is_expanded);

  renderer_list = btk_cell_layout_get_cells (BTK_CELL_LAYOUT (tv_col));

  /* If there are more than one renderer in the list, make a container */

  if (renderer_list && renderer_list->next)
    {
      BailCell *container_cell;

      container = bail_container_cell_new ();
      bail_return_val_if_fail (container, NULL);

      container_cell = BAIL_CELL (container);
      bail_cell_initialise (container_cell,
                            widget, BATK_OBJECT (bailview), 
                            i);
      /*
       * The BailTreeViewCellInfo structure for the container will be before
       * the ones for the cells so that the first one we find for a position
       * will be for the container
       */
      cell_info_new (bailview, tree_model, path, tv_col, container_cell);
      container_cell->refresh_index = refresh_cell_index;
      parent = BATK_OBJECT (container);
    }
  else
    parent = BATK_OBJECT (bailview);

  child = NULL;

  /*
   * Now we make a fake cell_renderer if there is no cell in renderer_list
   */

  if (renderer_list == NULL)
  {
    BtkCellRenderer *fake_renderer;
    fake_renderer = g_object_new (BTK_TYPE_CELL_RENDERER_TEXT, NULL);
    default_registry = batk_get_default_registry ();
    factory = batk_registry_get_factory (default_registry,
                                        G_OBJECT_TYPE (fake_renderer));
    child = batk_object_factory_create_accessible (factory,
                                                  G_OBJECT (fake_renderer));
    bail_return_val_if_fail (BAIL_IS_RENDERER_CELL (child), NULL);
    cell = BAIL_CELL (child);
    renderer_cell = BAIL_RENDERER_CELL (child);
    renderer_cell->renderer = fake_renderer;

    /* Create the BailTreeViewCellInfo structure for this cell */
    cell_info_new (bailview, tree_model, path, tv_col, cell);

    bail_cell_initialise (cell,
                          widget, parent, 
                          i);

    cell->refresh_index = refresh_cell_index;

    /* set state if it is expandable */
    if (is_expander)
    {
      set_cell_expandable (cell);
      if (is_expanded)
        bail_cell_add_state (cell, 
                             BATK_STATE_EXPANDED,
                             FALSE);
    }
  } else {
    for (l = renderer_list; l; l = l->next)
      {
        renderer = BTK_CELL_RENDERER (l->data);

        if (BTK_IS_CELL_RENDERER_TEXT (renderer))
          g_object_get (G_OBJECT (renderer), "editable", &editable, NULL);

        default_registry = batk_get_default_registry ();
        factory = batk_registry_get_factory (default_registry,
                                            G_OBJECT_TYPE (renderer));
        child = batk_object_factory_create_accessible (factory,
                                                      G_OBJECT (renderer));
        bail_return_val_if_fail (BAIL_IS_RENDERER_CELL (child), NULL);
        cell = BAIL_CELL (child);
        renderer_cell = BAIL_RENDERER_CELL (child);

        /* Create the BailTreeViewCellInfo structure for this cell */
        cell_info_new (bailview, tree_model, path, tv_col, cell);

        bail_cell_initialise (cell,
                              widget, parent, 
                              i);

        if (container)
          bail_container_cell_add_child (container, cell);
        else
          cell->refresh_index = refresh_cell_index;

        update_cell_value (renderer_cell, bailview, FALSE);
        /* Add the actions appropriate for this cell */
        add_cell_actions (cell, editable);

        /* set state if it is expandable */
        if (is_expander)
          {
            set_cell_expandable (cell);
            if (is_expanded)
              bail_cell_add_state (cell, 
                                   BATK_STATE_EXPANDED,
                                   FALSE);
          }
        /*
         * If the column is visible, sets the cell's state
         */
        if (btk_tree_view_column_get_visible (tv_col))
          set_cell_visibility (tree_view, cell, tv_col, path, FALSE);
        /*
         * If the row is selected, all cells on the row are selected
         */
        selection = btk_tree_view_get_selection (tree_view);

        if (btk_tree_selection_path_is_selected (selection, path))
          bail_cell_add_state (cell, BATK_STATE_SELECTED, FALSE);

        bail_cell_add_state (cell, BATK_STATE_FOCUSABLE, FALSE);
        if (focus_index == i)
          {
            bailview->focus_cell = g_object_ref (cell);
            bail_cell_add_state (cell, BATK_STATE_FOCUSED, FALSE);
            g_signal_emit_by_name (bailview,
                                   "active-descendant-changed",
                                   cell);
          }
      }
    g_list_free (renderer_list); 
    if (container)
      child =  BATK_OBJECT (container);
  } 

  if (expander_tv == tv_col)
    {
      BatkRelationSet *relation_set;
      BatkObject *accessible_array[1];
      BatkRelation* relation;
      BatkObject *parent_node;

      relation_set = batk_object_ref_relation_set (BATK_OBJECT (child));

      btk_tree_path_up (path);
      if (btk_tree_path_get_depth (path) == 0)
        parent_node = obj;
      else
        {
          gint parent_index;
          gint n_columns;

          n_columns = get_n_actual_columns (tree_view);
          parent_index = get_index (tree_view, path, i % n_columns);
          parent_node = batk_object_ref_accessible_child (obj, parent_index);
        }
      accessible_array[0] = parent_node;
      relation = batk_relation_new (accessible_array, 1,
                                   BATK_RELATION_NODE_CHILD_OF);
      batk_relation_set_add (relation_set, relation);
      batk_object_add_relationship (parent_node, BATK_RELATION_NODE_PARENT_OF,
                                   child);
      g_object_unref (relation);
      g_object_unref (relation_set);
    }
  btk_tree_path_free (path);

  /*
   * We do not increase the reference count here; when g_object_unref() is 
   * called for the cell then cell_destroyed() is called and
   * this removes the cell from the cache.
   */
  return child;
}

static BatkStateSet*
bail_tree_view_ref_state_set (BatkObject *obj)
{
  BatkStateSet *state_set;
  BtkWidget *widget;

  state_set = BATK_OBJECT_CLASS (bail_tree_view_parent_class)->ref_state_set (obj);
  widget = BTK_ACCESSIBLE (obj)->widget;

  if (widget != NULL)
    batk_state_set_add_state (state_set, BATK_STATE_MANAGES_DESCENDANTS);

  return state_set;
}

/* batkcomponent.h */

static void
batk_component_interface_init (BatkComponentIface *iface)
{
  iface->ref_accessible_at_point = bail_tree_view_ref_accessible_at_point;
}

static BatkObject*
bail_tree_view_ref_accessible_at_point (BatkComponent           *component,
                                        gint                   x,
                                        gint                   y,
                                        BatkCoordType           coord_type)
{
  BtkWidget *widget;
  BtkTreeView *tree_view;
  BtkTreePath *path;
  BtkTreeViewColumn *tv_column;
  gint x_pos, y_pos;
  gint bx, by;
  gboolean ret_val;

  widget = BTK_ACCESSIBLE (component)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  tree_view = BTK_TREE_VIEW (widget);

  batk_component_get_extents (component, &x_pos, &y_pos, NULL, NULL, coord_type);
  btk_tree_view_convert_widget_to_bin_window_coords (tree_view, x, y, &bx, &by);
  ret_val = btk_tree_view_get_path_at_pos (tree_view, 
                                           bx - x_pos, by - y_pos, 
                                           &path, &tv_column, NULL, NULL);
  if (ret_val)
    {
      gint index, column;

      column = get_column_number (tree_view, tv_column, FALSE);
      index = get_index (tree_view, path, column);
      btk_tree_path_free (path);

      return bail_tree_view_ref_child (BATK_OBJECT (component), index);
    } 
  else
    {
      g_warning ("bail_tree_view_ref_accessible_at_point: btk_tree_view_get_path_at_pos () failed\n");
    }
  return NULL;
}
           
/* batktable.h */

static void 
batk_table_interface_init (BatkTableIface *iface)
{
  iface->ref_at = bail_tree_view_table_ref_at;
  iface->get_n_rows = bail_tree_view_get_n_rows;	
  iface->get_n_columns = bail_tree_view_get_n_columns;	
  iface->get_index_at = bail_tree_view_get_index_at;	
  iface->get_column_at_index = bail_tree_view_get_column_at_index;	
  iface->get_row_at_index = bail_tree_view_get_row_at_index;	
  iface->is_row_selected = bail_tree_view_is_row_selected;
  iface->is_selected = bail_tree_view_is_selected;
  iface->get_selected_rows = bail_tree_view_get_selected_rows;
  iface->add_row_selection = bail_tree_view_add_row_selection;
  iface->remove_row_selection = bail_tree_view_remove_row_selection;
  iface->get_column_extent_at = NULL;
  iface->get_row_extent_at = NULL;
  iface->get_row_header = bail_tree_view_get_row_header;
  iface->set_row_header = bail_tree_view_set_row_header;
  iface->get_column_header = bail_tree_view_get_column_header;
  iface->set_column_header = bail_tree_view_set_column_header;
  iface->get_caption = bail_tree_view_get_caption;
  iface->set_caption = bail_tree_view_set_caption;
  iface->get_summary = bail_tree_view_get_summary;
  iface->set_summary = bail_tree_view_set_summary;
  iface->get_row_description = bail_tree_view_get_row_description;
  iface->set_row_description = bail_tree_view_set_row_description;
  iface->get_column_description = bail_tree_view_get_column_description;
  iface->set_column_description = bail_tree_view_set_column_description;
}

static gint
bail_tree_view_get_index_at (BatkTable *table,
                             gint     row,
                             gint     column)
{
  BtkWidget *widget;
  BtkTreeView *tree_view;
  gint actual_column;
  gint n_cols, n_rows;
  BtkTreeIter iter;
  BtkTreePath *path;
  gint index;

  n_cols = batk_table_get_n_columns (table);
  n_rows = batk_table_get_n_rows (table);

  if (row >= n_rows ||
      column >= n_cols)
    return -1;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return -1;

  tree_view = BTK_TREE_VIEW (widget);
  actual_column = get_actual_column_number (tree_view, column);

  set_iter_nth_row (tree_view, &iter, row);
  path = btk_tree_model_get_path (btk_tree_view_get_model (tree_view), &iter);

  index = get_index (tree_view, path, actual_column);
  btk_tree_path_free (path);

  return index;
}

static gint
bail_tree_view_get_column_at_index (BatkTable *table,
                                    gint     index)
{
  BtkWidget *widget;
  BtkTreeView *tree_view;
  gint n_columns;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return -1;

  tree_view = BTK_TREE_VIEW (widget);
  n_columns = get_n_actual_columns (tree_view);

  if (n_columns == 0)
    return 0;
  index = index % n_columns;

  return get_visible_column_number (tree_view, index);
}

static gint
bail_tree_view_get_row_at_index (BatkTable *table,
                                 gint     index)
{
  BtkWidget *widget;
  BtkTreeView *tree_view;
  BtkTreePath *path;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return -1;

  tree_view = BTK_TREE_VIEW (widget);
  if (get_path_column_from_index (tree_view, index, &path, NULL))
    {
      gint row = get_row_from_tree_path (tree_view, path);
      btk_tree_path_free (path);
      return row;
    }
  else
    return -1;
}

static BatkObject* 
bail_tree_view_table_ref_at (BatkTable *table,
                             gint     row, 
                             gint     column)
{
  gint index;

  index = bail_tree_view_get_index_at (table, row, column);
  if (index == -1)
    return NULL;
  
  return bail_tree_view_ref_child (BATK_OBJECT (table), index);
}

static gint 
bail_tree_view_get_n_rows (BatkTable *table)
{
  BtkWidget *widget;
  BtkTreeView *tree_view;
  BtkTreeModel *tree_model;
  gint n_rows;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  tree_view = BTK_TREE_VIEW (widget);
  tree_model = btk_tree_view_get_model (tree_view);

  if (btk_tree_model_get_flags (tree_model) & BTK_TREE_MODEL_LIST_ONLY)
   /* 
    * If working with a LIST store, then this is a faster way
    * to get the number of rows.
    */
    n_rows = btk_tree_model_iter_n_children (tree_model, NULL);
  else
    {
      BtkTreePath *root_tree;

      n_rows = 0;
      root_tree = btk_tree_path_new_first ();
      iterate_thru_children (tree_view, tree_model,
                             root_tree, NULL, &n_rows, 0);
      btk_tree_path_free (root_tree);
    }

  return n_rows;
}

/*
 * The function get_n_actual_columns returns the number of columns in the 
 * BtkTreeView. i.e. it include both visible and non-visible columns.
 */
static gint 
get_n_actual_columns (BtkTreeView *tree_view)
{
  GList *columns;
  gint n_cols;

  columns = btk_tree_view_get_columns (tree_view);
  n_cols = g_list_length (columns);
  g_list_free (columns);
  return n_cols;
}

static gint 
bail_tree_view_get_n_columns (BatkTable *table)
{
  BtkWidget *widget;
  BtkTreeView *tree_view;
  BtkTreeViewColumn *tv_col;
  gint n_cols = 0;
  gint i = 0;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  tree_view = BTK_TREE_VIEW (widget);
  tv_col = btk_tree_view_get_column (tree_view, i);

  while (tv_col != NULL) 
    {
      if (btk_tree_view_column_get_visible (tv_col)) 
        n_cols++;

      i++;
      tv_col = btk_tree_view_get_column (tree_view, i);
    }

  return n_cols;
}

static gboolean 
bail_tree_view_is_row_selected (BatkTable *table,
                                gint     row)
{
  BtkWidget *widget;
  BtkTreeView *tree_view;
  BtkTreeSelection *selection;
  BtkTreeIter iter;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;

  if (row < 0)
    return FALSE;

  tree_view = BTK_TREE_VIEW (widget);

  selection = btk_tree_view_get_selection (tree_view);

  set_iter_nth_row (tree_view, &iter, row);

  return btk_tree_selection_iter_is_selected (selection, &iter);
}

static gboolean 
bail_tree_view_is_selected (BatkTable *table, 
                            gint     row, 
                            gint     column)
{
  return bail_tree_view_is_row_selected (table, row);
}

static gint 
bail_tree_view_get_selected_rows (BatkTable *table,
                                  gint     **rows_selected)
{
  BtkWidget *widget;
  BtkTreeView *tree_view;
  BtkTreeModel *tree_model;
  BtkTreeIter iter;
  BtkTreeSelection *selection;
  BtkTreePath *tree_path;
  gint ret_val = 0;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  tree_view = BTK_TREE_VIEW (widget);

  selection = btk_tree_view_get_selection (tree_view);

  switch (selection->type)
    {
    case BTK_SELECTION_SINGLE:
    case BTK_SELECTION_BROWSE:
      if (btk_tree_selection_get_selected (selection, &tree_model, &iter))
        {
          gint row;

          if (rows_selected)
            {
              *rows_selected = (gint *)g_malloc (sizeof(gint));
              tree_path = btk_tree_model_get_path (tree_model, &iter);
              row = get_row_from_tree_path (tree_view, tree_path);
              btk_tree_path_free (tree_path);

              /* shouldn't ever happen */
              g_return_val_if_fail (row != -1, 0);

              *rows_selected[0] = row;
            }
          ret_val = 1;
        }
      break;
    case BTK_SELECTION_MULTIPLE:
      {
        GPtrArray *array = g_ptr_array_new();

        btk_tree_selection_selected_foreach (selection,
                                             get_selected_rows,
                                             array);
        ret_val = array->len;

        if (rows_selected && ret_val)
          {
            gint i;
            *rows_selected = (gint *) g_malloc (ret_val * sizeof (gint));

            for (i = 0; i < ret_val; i++)
              {
                gint row;

                tree_path = (BtkTreePath *) g_ptr_array_index (array, i);
                row = get_row_from_tree_path (tree_view, tree_path);
                btk_tree_path_free (tree_path);
                (*rows_selected)[i] = row;
              }
          }
        g_ptr_array_free (array, FALSE);
      }
      break;
    case BTK_SELECTION_NONE:
      break; 
    }
  return ret_val;
}

static gboolean 
bail_tree_view_add_row_selection (BatkTable *table, 
                                  gint     row)
{
  BtkWidget *widget;
  BtkTreeView *tree_view;
  BtkTreeModel *tree_model;
  BtkTreeSelection *selection;
  BtkTreePath *tree_path;
  BtkTreeIter iter_to_row;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;
  
  if (!bail_tree_view_is_row_selected (table, row))
    {
      tree_view = BTK_TREE_VIEW (widget);
      tree_model = btk_tree_view_get_model (tree_view);
      selection = btk_tree_view_get_selection (tree_view);

      if (btk_tree_model_get_flags (tree_model) & BTK_TREE_MODEL_LIST_ONLY)
        {
          tree_path = btk_tree_path_new ();
          btk_tree_path_append_index (tree_path, row);
          btk_tree_selection_select_path (selection,tree_path);
          btk_tree_path_free (tree_path);
        }
      else
        { 
          if (set_iter_nth_row (tree_view, &iter_to_row, row))
            btk_tree_selection_select_iter (selection, &iter_to_row);
          else
            return FALSE;
        }
    }

  return bail_tree_view_is_row_selected (table, row);
}

static gboolean 
bail_tree_view_remove_row_selection (BatkTable *table, 
                                     gint     row)
{
  BtkWidget *widget;
  BtkTreeView *tree_view;
  BtkTreeSelection *selection;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;

  tree_view = BTK_TREE_VIEW (widget);

  selection = btk_tree_view_get_selection (tree_view);

  if (bail_tree_view_is_row_selected (table, row)) 
    {
      btk_tree_selection_unselect_all (selection);
      return TRUE;
    }
  else return FALSE;
}

static BatkObject* 
bail_tree_view_get_row_header (BatkTable *table, 
                               gint     row)
{
  BailTreeViewRowInfo *row_info;

  row_info = get_row_info (table, row);
  if (row_info)
    return row_info->header;
  else
    return NULL;
}

static void
bail_tree_view_set_row_header (BatkTable  *table, 
                               gint      row, 
                               BatkObject *header)
{
  set_row_data (table, row, header, NULL, TRUE);
}

static BatkObject* 
bail_tree_view_get_column_header (BatkTable *table, 
                                  gint     in_col)
{
  BtkWidget *widget;
  BtkTreeView *tree_view;
  BtkTreeViewColumn *tv_col;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  tree_view = BTK_TREE_VIEW (widget);
  tv_col = get_column (tree_view, in_col);
  return get_header_from_column (tv_col);
}

static void
bail_tree_view_set_column_header (BatkTable  *table, 
                                  gint      in_col,
                                  BatkObject *header)
{
  BtkWidget *widget;
  BtkTreeView *tree_view;
  BtkTreeViewColumn *tv_col;
  BatkObject *rc;
  BatkPropertyValues values = { NULL };

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  tree_view = BTK_TREE_VIEW (widget);
  tv_col = get_column (tree_view, in_col);
  if (tv_col == NULL)
     return;

  rc = g_object_get_qdata (G_OBJECT (tv_col),
                          quark_column_header_object);
  if (rc)
    g_object_unref (rc);

  g_object_set_qdata (G_OBJECT (tv_col),
			quark_column_header_object,
			header);
  if (header)
    g_object_ref (header);
  g_value_init (&values.new_value, G_TYPE_INT);
  g_value_set_int (&values.new_value, in_col);

  values.property_name = "accessible-table-column-header";
  g_signal_emit_by_name (table, 
                         "property_change::accessible-table-column-header",
                         &values, NULL);
}

static BatkObject*
bail_tree_view_get_caption (BatkTable	*table)
{
  BailTreeView* obj = BAIL_TREE_VIEW (table);

  return obj->caption;
}

static void
bail_tree_view_set_caption (BatkTable	*table,
                            BatkObject   *caption)
{
  BailTreeView* obj = BAIL_TREE_VIEW (table);
  BatkPropertyValues values = { NULL };
  BatkObject *old_caption;

  old_caption = obj->caption;
  obj->caption = caption;
  if (obj->caption)
    g_object_ref (obj->caption);
  g_value_init (&values.old_value, G_TYPE_POINTER);
  g_value_set_pointer (&values.old_value, old_caption);
  g_value_init (&values.new_value, G_TYPE_POINTER);
  g_value_set_pointer (&values.new_value, obj->caption);

  values.property_name = "accessible-table-caption-object";
  g_signal_emit_by_name (table, 
                         "property_change::accessible-table-caption-object", 
                         &values, NULL);
  if (old_caption)
    g_object_unref (old_caption);
}

static const gchar*
bail_tree_view_get_column_description (BatkTable	  *table,
                                       gint       in_col)
{
  BtkWidget *widget;
  BtkTreeView *tree_view;
  BtkTreeViewColumn *tv_col;
  gchar *rc;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  tree_view = BTK_TREE_VIEW (widget);
  tv_col = get_column (tree_view, in_col);
  if (tv_col == NULL)
     return NULL;

  rc = g_object_get_qdata (G_OBJECT (tv_col),
                           quark_column_desc_object);

  if (rc != NULL)
    return rc;
  else
    {
      gchar *title_text;

      g_object_get (tv_col, "title", &title_text, NULL);
      return title_text;
    }
}

static void
bail_tree_view_set_column_description (BatkTable	   *table,
                                       gint        in_col,
                                       const gchar *description)
{
  BtkWidget *widget;
  BtkTreeView *tree_view;
  BtkTreeViewColumn *tv_col;
  BatkPropertyValues values = { NULL };

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  tree_view = BTK_TREE_VIEW (widget);
  tv_col = get_column (tree_view, in_col);
  if (tv_col == NULL)
     return;

  g_object_set_qdata (G_OBJECT (tv_col),
                      quark_column_desc_object,
                      g_strdup (description));
  g_value_init (&values.new_value, G_TYPE_INT);
  g_value_set_int (&values.new_value, in_col);

  values.property_name = "accessible-table-column-description";
  g_signal_emit_by_name (table, 
                         "property_change::accessible-table-column-description",
                         &values, NULL);
}

static const gchar*
bail_tree_view_get_row_description (BatkTable    *table,
                                    gint        row)
{
  BailTreeViewRowInfo *row_info;

  row_info = get_row_info (table, row);
  if (row_info)
    return row_info->description;
  else
    return NULL;
}

static void
bail_tree_view_set_row_description (BatkTable    *table,
                                    gint        row,
                                    const gchar *description)
{
  set_row_data (table, row, NULL, description, FALSE);
}

static BatkObject*
bail_tree_view_get_summary (BatkTable	*table)
{
  BailTreeView* obj = BAIL_TREE_VIEW (table);

  return obj->summary;
}

static void
bail_tree_view_set_summary (BatkTable    *table,
                            BatkObject   *accessible)
{
  BailTreeView* obj = BAIL_TREE_VIEW (table);
  BatkPropertyValues values = { NULL };
  BatkObject *old_summary;

  old_summary = obj->summary;
  obj->summary = accessible;
  if (obj->summary)
    g_object_ref (obj->summary);
  g_value_init (&values.old_value, G_TYPE_POINTER);
  g_value_set_pointer (&values.old_value, old_summary);
  g_value_init (&values.new_value, G_TYPE_POINTER);
  g_value_set_pointer (&values.new_value, obj->summary);

  values.property_name = "accessible-table-summary";
  g_signal_emit_by_name (table, 
                         "property_change::accessible-table-ummary",
                         &values, NULL);
  if (old_summary)
    g_object_unref (old_summary);
}

static void
set_row_data (BatkTable    *table, 
              gint        row, 
              BatkObject   *header,
              const gchar *description,
              gboolean    is_header)
{
  BtkWidget *widget;
  BtkTreeView *tree_view;
  BtkTreeModel *tree_model;
  BailTreeView* obj = BAIL_TREE_VIEW (table);
  BailTreeViewRowInfo* row_info;
  BtkTreePath *path;
  BtkTreeIter iter;
  GArray *array;
  gboolean found = FALSE;
  gint i;
  BatkPropertyValues values = { NULL };
  gchar *signal_name;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  tree_view = BTK_TREE_VIEW (widget);
  tree_model = btk_tree_view_get_model (tree_view);

  set_iter_nth_row (tree_view, &iter, row);
  path = btk_tree_model_get_path (tree_model, &iter);

  if (obj->row_data == NULL)
    obj->row_data = g_array_sized_new (FALSE, TRUE,
                                       sizeof(BailTreeViewRowInfo *), 0);

  array = obj->row_data;

  for (i = 0; i < array->len; i++)
    {
      BtkTreePath *row_path;

      row_info = g_array_index (array, BailTreeViewRowInfo*, i);
      row_path = btk_tree_row_reference_get_path (row_info->row_ref);

      if (row_path != NULL)
        {
          if (path && btk_tree_path_compare (row_path, path) == 0)
            found = TRUE;

          btk_tree_path_free (row_path);

          if (found)
            {
              if (is_header)
                {
                  if (row_info->header)
                    g_object_unref (row_info->header);
                  row_info->header = header;
                  if (row_info->header)
                    g_object_ref (row_info->header);
                }
              else
                {
                  g_free (row_info->description);
                  row_info->description = g_strdup (description);
                }
              break;
            }
        }
    }

  if (!found)
    {
      /* if not found */
      row_info = g_malloc (sizeof(BailTreeViewRowInfo));
      row_info->row_ref = btk_tree_row_reference_new (tree_model, path);
      if (is_header)
        {
          row_info->header = header;
          if (row_info->header)
            g_object_ref (row_info->header);
          row_info->description = NULL;
        }
      else
        {
          row_info->header = NULL;
          row_info->description = g_strdup (description);
        }
      g_array_append_val (array, row_info);
    }
  g_value_init (&values.new_value, G_TYPE_INT);
  g_value_set_int (&values.new_value, row);

  if (is_header)
    {
      values.property_name = "accessible-table-row-header";
      signal_name = "property_change::accessible-table-row-header";
    }
  else
    {
      values.property_name = "accessible-table-row-description";
      signal_name = "property-change::accessible-table-row-description";
    }
  g_signal_emit_by_name (table, 
                         signal_name,
                         &values, NULL);

  btk_tree_path_free (path);
}


static BailTreeViewRowInfo*
get_row_info (BatkTable    *table,
              gint        row)
{
  BtkWidget *widget;
  BtkTreeView *tree_view;
  BtkTreeModel *tree_model;
  BailTreeView* obj = BAIL_TREE_VIEW (table);
  BtkTreePath *path;
  BtkTreeIter iter;
  GArray *array;
  BailTreeViewRowInfo *rc = NULL;

  widget = BTK_ACCESSIBLE (table)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  tree_view = BTK_TREE_VIEW (widget);
  tree_model = btk_tree_view_get_model (tree_view);

  set_iter_nth_row (tree_view, &iter, row);
  path = btk_tree_model_get_path (tree_model, &iter);
  array = obj->row_data;

  if (array != NULL)
    {
      BailTreeViewRowInfo *row_info;
      BtkTreePath *row_path;
      gint i;

      for (i = 0; i < array->len; i++)
        {
          row_info = g_array_index (array, BailTreeViewRowInfo*, i);
          row_path = btk_tree_row_reference_get_path (row_info->row_ref);
          if (row_path != NULL)
            {
              if (path && btk_tree_path_compare (row_path, path) == 0)
                rc = row_info;

              btk_tree_path_free (row_path);

              if (rc != NULL)
                break;
            }
        }
    }

  btk_tree_path_free (path);
  return rc;
}
/* batkselection.h */

static void batk_selection_interface_init (BatkSelectionIface *iface)
{
  iface->add_selection = bail_tree_view_add_selection;
  iface->clear_selection = bail_tree_view_clear_selection;
  iface->ref_selection = bail_tree_view_ref_selection;
  iface->get_selection_count = bail_tree_view_get_selection_count;
  iface->is_child_selected = bail_tree_view_is_child_selected;
}

static gboolean
bail_tree_view_add_selection (BatkSelection *selection,
                              gint         i)
{
  BatkTable *table;
  gint n_columns;
  gint row;

  table = BATK_TABLE (selection);
  n_columns = bail_tree_view_get_n_columns (table);
  if (n_columns != 1)
    return FALSE;

  row = bail_tree_view_get_row_at_index (table, i);
  return bail_tree_view_add_row_selection (table, row);
}

static gboolean
bail_tree_view_clear_selection (BatkSelection *selection)
{
  BtkWidget *widget;
  BtkTreeView *tree_view;
  BtkTreeSelection *tree_selection;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;
 
  tree_view = BTK_TREE_VIEW (widget);

  tree_selection = btk_tree_view_get_selection (tree_view);
  btk_tree_selection_unselect_all (tree_selection);  

  return TRUE;
}

static BatkObject*  
bail_tree_view_ref_selection (BatkSelection *selection, 
                              gint         i)
{
  BatkTable *table;
  gint row;
  gint n_selected;
  gint n_columns;
  gint *selected;

  table = BATK_TABLE (selection);
  n_columns = bail_tree_view_get_n_columns (table);
  n_selected = bail_tree_view_get_selected_rows (table, &selected);
  if (i >= n_columns * n_selected)
    return NULL;

  row = selected[i / n_columns];
  g_free (selected);

  return bail_tree_view_table_ref_at (table, row, i % n_columns);
}

static gint
bail_tree_view_get_selection_count (BatkSelection *selection)
{
  BatkTable *table;
  gint n_selected;

  table = BATK_TABLE (selection);
  n_selected = bail_tree_view_get_selected_rows (table, NULL);
  if (n_selected > 0)
    n_selected *= bail_tree_view_get_n_columns (table);
  return n_selected;
}

static gboolean
bail_tree_view_is_child_selected (BatkSelection *selection, 
                                  gint         i)
{
  BtkWidget *widget;
  gint row;

  widget = BTK_ACCESSIBLE (selection)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;

  row = batk_table_get_row_at_index (BATK_TABLE (selection), i);

  return bail_tree_view_is_row_selected (BATK_TABLE (selection), row);
}


static void bail_cell_parent_interface_init (BailCellParentIface *iface)
{
  iface->get_cell_extents = bail_tree_view_get_cell_extents;
  iface->get_cell_area = bail_tree_view_get_cell_area;
  iface->grab_focus = bail_tree_view_grab_cell_focus;
}

static void
bail_tree_view_get_cell_extents (BailCellParent *parent,
                                 BailCell       *cell,
                                 gint           *x,
                                 gint           *y,
                                 gint           *width,
                                 gint           *height,
                                 BatkCoordType   coord_type)
{
  BtkWidget *widget;
  BtkTreeView *tree_view;
  BdkWindow *bin_window;
  BdkRectangle cell_rect;
  gint w_x, w_y;

  widget = BTK_ACCESSIBLE (parent)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  tree_view = BTK_TREE_VIEW (widget);
  bail_tree_view_get_cell_area (parent, cell, &cell_rect);
  bin_window = btk_tree_view_get_bin_window (tree_view);
  bdk_window_get_origin (bin_window, &w_x, &w_y);

  if (coord_type == BATK_XY_WINDOW)
    {
      BdkWindow *window;
      gint x_toplevel, y_toplevel;

      window = bdk_window_get_toplevel (bin_window);
      bdk_window_get_origin (window, &x_toplevel, &y_toplevel);

      w_x -= x_toplevel;
      w_y -= y_toplevel;
    }

  *width = cell_rect.width;
  *height = cell_rect.height;
  if (is_cell_showing (tree_view, &cell_rect))
    {
      *x = cell_rect.x + w_x;
      *y = cell_rect.y + w_y;
    }
  else
    {
      *x = G_MININT;
      *y = G_MININT;
    }
}

#define EXTRA_EXPANDER_PADDING 4

static void
bail_tree_view_get_cell_area (BailCellParent *parent,
                              BailCell       *cell,
                              BdkRectangle   *cell_rect)
{
  BtkWidget *widget;
  BtkTreeView *tree_view;
  BtkTreeViewColumn *tv_col;
  BtkTreePath *path;
  BatkObject *parent_cell;
  BailTreeViewCellInfo *cell_info;
  BailCell *top_cell;

  widget = BTK_ACCESSIBLE (parent)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  tree_view = BTK_TREE_VIEW (widget);
  parent_cell = batk_object_get_parent (BATK_OBJECT (cell));
  if (parent_cell != BATK_OBJECT (parent))
    {
      /*
       * BailCell is in a BailContainerCell
       */
      top_cell = BAIL_CELL (parent_cell);
    }
  else
    {
      top_cell = cell;
    }
  cell_info = find_cell_info (BAIL_TREE_VIEW (parent), top_cell, NULL, TRUE);
  bail_return_if_fail (cell_info);
  bail_return_if_fail (cell_info->cell_col_ref);
  bail_return_if_fail (cell_info->cell_row_ref);
  path = btk_tree_row_reference_get_path (cell_info->cell_row_ref);
  tv_col = cell_info->cell_col_ref;
  if (path && cell_info->in_use)
    {
      BtkTreeViewColumn *expander_column;
      gint focus_line_width;

      btk_tree_view_get_cell_area (tree_view, path, tv_col, cell_rect);
      expander_column = btk_tree_view_get_expander_column (tree_view);
      if (expander_column == tv_col)
        {
          gint expander_size;

          btk_widget_style_get (widget,
                                "expander_size", &expander_size,
                                NULL);

          cell_rect->x += expander_size + EXTRA_EXPANDER_PADDING;
          cell_rect->width -= expander_size + EXTRA_EXPANDER_PADDING;
        }
      btk_widget_style_get (widget,
                            "focus-line-width", &focus_line_width,
                            NULL);

      cell_rect->x += focus_line_width;
      cell_rect->width -= 2 * focus_line_width; 

      btk_tree_path_free (path);

      /*
       * A column has more than one renderer so we find the position and width
       * of each.
       */
      if (top_cell != cell)
        {
          gint cell_index;
          gboolean found;
          gint cell_start;
          gint cell_width;
          GList *renderers;
          BtkCellRenderer *renderer;

	  cell_index = batk_object_get_index_in_parent (BATK_OBJECT (cell));
	  renderers = btk_cell_layout_get_cells (BTK_CELL_LAYOUT (tv_col));
	  renderer = g_list_nth_data (renderers, cell_index);

	  found = btk_tree_view_column_cell_get_position (tv_col, renderer, &cell_start, &cell_width);
          if (found)
            {
              cell_rect->x += cell_start;
              cell_rect->width = cell_width;
            }
	  g_list_free (renderers);
	}

    }
}

static gboolean
bail_tree_view_grab_cell_focus  (BailCellParent *parent,
                                 BailCell       *cell)
{
  BtkWidget *widget;
  BtkTreeView *tree_view;
  BtkTreeViewColumn *tv_col;
  BtkTreePath *path;
  BatkObject *parent_cell;
  BatkObject *cell_object;
  BailTreeViewCellInfo *cell_info;
  BtkCellRenderer *renderer = NULL;
  BtkWidget *toplevel;
  gint index;

  widget = BTK_ACCESSIBLE (parent)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;

  tree_view = BTK_TREE_VIEW (widget);

  cell_info = find_cell_info (BAIL_TREE_VIEW (parent), cell, NULL, TRUE);
  bail_return_val_if_fail (cell_info, FALSE);
  bail_return_val_if_fail (cell_info->cell_col_ref, FALSE);
  bail_return_val_if_fail (cell_info->cell_row_ref, FALSE);
  cell_object = BATK_OBJECT (cell);
  parent_cell = batk_object_get_parent (cell_object);
  tv_col = cell_info->cell_col_ref;
  if (parent_cell != BATK_OBJECT (parent))
    {
      /*
       * BailCell is in a BailContainerCell.
       * The BtkTreeViewColumn has multiple renderers; 
       * find the corresponding one.
       */
      GList *renderers;

      renderers = btk_cell_layout_get_cells (BTK_CELL_LAYOUT (tv_col));
      if (cell_info->in_use) {
	  index = batk_object_get_index_in_parent (cell_object);
	  renderer = g_list_nth_data (renderers, index);
      }
      g_list_free (renderers);
    }
  path = btk_tree_row_reference_get_path (cell_info->cell_row_ref);
  if (path && cell_info->in_use)
    {
      if (renderer)
        btk_tree_view_set_cursor_on_cell (tree_view, path, tv_col, renderer, FALSE);
      else
        btk_tree_view_set_cursor (tree_view, path, tv_col, FALSE);

      btk_tree_path_free (path);
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

/* signal handling */

static gboolean
bail_tree_view_expand_row_btk (BtkTreeView       *tree_view,
                               BtkTreeIter        *iter,
                               BtkTreePath        *path)
{
  BatkObject *batk_obj;
  BailTreeView *bailview;

  g_assert (BTK_IS_TREE_VIEW (tree_view));

  batk_obj = btk_widget_get_accessible (BTK_WIDGET (tree_view));

  g_assert (BAIL_IS_TREE_VIEW (batk_obj));

  bailview = BAIL_TREE_VIEW (batk_obj);

  /*
   * The visible rectangle has not been updated when this signal is emitted
   * so we process the signal when the BTK processing is completed
   */
  /* this seems wrong since it overwrites any other pending expand handlers... */
  bailview->idle_expand_path = btk_tree_path_copy (path);
  if (bailview->idle_expand_id)
    g_source_remove (bailview->idle_expand_id);
  bailview->idle_expand_id = bdk_threads_add_idle (idle_expand_row, bailview);
  return FALSE;
}

static gint
idle_expand_row (gpointer data)
{
  BailTreeView *bailview = data;
  BtkTreePath *path;
  BtkTreeView *tree_view;
  BtkTreeIter iter;
  BtkTreeModel *tree_model;
  gint n_inserted, row;

  bailview->idle_expand_id = 0;

  path = bailview->idle_expand_path;
  tree_view = BTK_TREE_VIEW (BTK_ACCESSIBLE (bailview)->widget);

  g_assert (BTK_IS_TREE_VIEW (tree_view));

  tree_model = btk_tree_view_get_model(tree_view);
  if (!tree_model)
    return FALSE;

  if (!path || !btk_tree_model_get_iter (tree_model, &iter, path))
    return FALSE;

  /*
   * Update visibility of cells below expansion row
   */
  traverse_cells (bailview, path, FALSE, FALSE);
  /*
   * Figure out number of visible children, the following test
   * should not fail
   */
  if (btk_tree_model_iter_has_child (tree_model, &iter))
    {
      BtkTreePath *path_copy;

     /*
      * By passing path into this function, we find the number of
      * visible children of path.
      */
      path_copy = btk_tree_path_copy (path);
      btk_tree_path_append_index(path_copy, 0);

      n_inserted = 0;
      iterate_thru_children (tree_view, tree_model,
                             path_copy, NULL, &n_inserted, 0);
      btk_tree_path_free (path_copy);
    }
  else
    {
      /* We can get here if the row expanded callback deleted the row */
      return FALSE;
    }

  /* Set expand state */
  set_expand_state (tree_view, tree_model, bailview, path, TRUE);

  row = get_row_from_tree_path (tree_view, path);

  /* shouldn't ever happen */
  if (row == -1)
    g_assert_not_reached ();

  /* Must add 1 because the "added rows" are below the row being expanded */
  row += 1;
  
  g_signal_emit_by_name (bailview, "row_inserted", row, n_inserted);

  bailview->idle_expand_path = NULL;

  btk_tree_path_free (path);

  return FALSE;
}

static gboolean
bail_tree_view_collapse_row_btk (BtkTreeView       *tree_view,
                                 BtkTreeIter        *iter,
                                 BtkTreePath        *path)
{
  BtkTreeModel *tree_model;
  BatkObject *batk_obj = btk_widget_get_accessible (BTK_WIDGET (tree_view));
  BailTreeView *bailview = BAIL_TREE_VIEW (batk_obj);
  gint row;

  tree_model = btk_tree_view_get_model (tree_view);

  clean_rows (bailview);

  /*
   * Update visibility of cells below collapsed row
   */
  traverse_cells (bailview, path, FALSE, FALSE);
  /* Set collapse state */
  set_expand_state (tree_view, tree_model, bailview, path, FALSE);

  bail_return_val_if_fail (bailview->n_children_deleted, FALSE);
  row = get_row_from_tree_path (tree_view, path);
  bail_return_val_if_fail (row != -1, FALSE);
  g_signal_emit_by_name (batk_obj, "row_deleted", row, 
                         bailview->n_children_deleted);
  bailview->n_children_deleted = 0;
  return FALSE;
}

static void
bail_tree_view_size_allocate_btk (BtkWidget     *widget,
                                  BtkAllocation *allocation)
{
  BatkObject *batk_obj = btk_widget_get_accessible (widget);
  BailTreeView *bailview = BAIL_TREE_VIEW (batk_obj);

  /*
   * If the size allocation changes, the visibility of cells may change so
   * update the cells visibility.
   */
  traverse_cells (bailview, NULL, FALSE, FALSE);
}

static void
bail_tree_view_set_scroll_adjustments (BtkWidget     *widget,
                                       BtkAdjustment *hadj,
                                       BtkAdjustment *vadj)
{
  BatkObject *batk_obj = btk_widget_get_accessible (widget);
  BailTreeView *bailview = BAIL_TREE_VIEW (batk_obj);
  BtkAdjustment *adj;

  g_object_get (widget, hadjustment, &adj, NULL);
  if (bailview->old_hadj != adj)
     {
        g_signal_handlers_disconnect_by_func (bailview->old_hadj, 
                                              (gpointer) adjustment_changed,
                                              widget);
        bailview->old_hadj = adj;
        g_object_add_weak_pointer (G_OBJECT (bailview->old_hadj), (gpointer *)&bailview->old_hadj);
        g_signal_connect (adj, 
                          "value_changed",
                          G_CALLBACK (adjustment_changed),
                          widget);
     } 
  g_object_get (widget, vadjustment, &adj, NULL);
  if (bailview->old_vadj != adj)
     {
        g_signal_handlers_disconnect_by_func (bailview->old_vadj, 
                                              (gpointer) adjustment_changed,
                                              widget);
        bailview->old_vadj = adj;
        g_object_add_weak_pointer (G_OBJECT (bailview->old_vadj), (gpointer *)&bailview->old_vadj);
        g_signal_connect (adj, 
                          "value_changed",
                          G_CALLBACK (adjustment_changed),
                          widget);
     } 
}

static void
bail_tree_view_changed_btk (BtkTreeSelection *selection,
                            gpointer         data)
{
  BailTreeView *bailview;
  BtkTreeView *tree_view;
  BtkWidget *widget;
  GList *cell_list;
  GList *l;
  BailTreeViewCellInfo *info;
  BtkTreeSelection *tree_selection;
  BtkTreePath *path;

  bailview = BAIL_TREE_VIEW (data);
  cell_list = bailview->cell_data;
  widget = BTK_ACCESSIBLE (bailview)->widget;
  if (widget == NULL)
    /*
     * destroy signal emitted for widget
     */
    return;
  tree_view = BTK_TREE_VIEW (widget);

  tree_selection = btk_tree_view_get_selection (tree_view);

  clean_rows (bailview);

  for (l = cell_list; l; l = l->next)
    {
      info = (BailTreeViewCellInfo *) (l->data);

      if (info->in_use)
      {
	  bail_cell_remove_state (info->cell, BATK_STATE_SELECTED, TRUE); 
	  
	  path = btk_tree_row_reference_get_path (info->cell_row_ref);
	  if (path && btk_tree_selection_path_is_selected (tree_selection, path))
	      bail_cell_add_state (info->cell, BATK_STATE_SELECTED, TRUE); 
	  btk_tree_path_free (path);
      }
    }
  if (btk_widget_get_realized (widget))
    g_signal_emit_by_name (bailview, "selection_changed");
}

static void
columns_changed (BtkTreeView *tree_view)
{
  BatkObject *batk_obj = btk_widget_get_accessible (BTK_WIDGET(tree_view));
  BailTreeView *bailview = BAIL_TREE_VIEW (batk_obj);
  GList *tv_cols, *tmp_list;
  gboolean column_found;
  gboolean move_found = FALSE;
  gboolean stale_set = FALSE;
  gint column_count = 0;
  gint i;

 /*
  * This function must determine if the change is an add, delete or
  * a move based upon its cache of TreeViewColumns in
  * bailview->col_data
  */
  tv_cols = btk_tree_view_get_columns (tree_view);

  /* check for adds or moves */
  for (tmp_list = tv_cols; tmp_list; tmp_list = tmp_list->next)
    {
      column_found = FALSE;

      for (i = 0; i < bailview->col_data->len; i++)
        {

          if ((BtkTreeViewColumn *)tmp_list->data ==
              (BtkTreeViewColumn *)g_array_index (bailview->col_data,
               BtkTreeViewColumn *, i))
            {
              column_found = TRUE;

              /* If the column isn't in the same position, a move happened */
              if (!move_found && i != column_count)
                {
                  if (!stale_set)
                    {
                      /* Set all rows to BATK_STATE_STALE */
                      traverse_cells (bailview, NULL, TRUE, FALSE);
                      stale_set = TRUE;
                    }
  
                  /* Just emit one column reordered signal when a move happens */
                  g_signal_emit_by_name (batk_obj, "column_reordered");
                  move_found = TRUE;
                }

              break;
            }
        }

     /*
      * If column_found is FALSE, then an insert happened for column
      * number column_count
      */
      if (!column_found)
        {
          gint n_cols, n_rows, row;

          if (!stale_set)
            {
              /* Set all rows to BATK_STATE_STALE */
              traverse_cells (bailview, NULL, TRUE, FALSE);
              stale_set = TRUE;
            }

          /* Generate column-inserted signal */
          g_signal_emit_by_name (batk_obj, "column_inserted", column_count, 1);

          /* Generate children-changed signals */
          n_rows = get_row_count (btk_tree_view_get_model (tree_view));
          n_cols = get_n_actual_columns (tree_view);
          for (row = 0; row < n_rows; row++)
            {
             /*
              * Pass NULL as the child object, i.e. 4th argument.
              */
              g_signal_emit_by_name (batk_obj, "children_changed::add",
                                    ((row * n_cols) + column_count), NULL, NULL);
            }
        }

      column_count++;
    }

  /* check for deletes */
  for (i = 0; i < bailview->col_data->len; i++)
    {
      column_found = FALSE;

      for (tmp_list = tv_cols; tmp_list; tmp_list = tmp_list->next)
        {
            if ((BtkTreeViewColumn *)tmp_list->data ==
                (BtkTreeViewColumn *)g_array_index (bailview->col_data,
                 BtkTreeViewColumn *, i))
              {
                column_found = TRUE;
                break;
              }
        }

       /*
        * If column_found is FALSE, then a delete happened for column
        * number i
        */
      if (!column_found)
        {
          gint n_rows, n_cols, row;

          clean_cols (bailview,
                      (BtkTreeViewColumn *)g_array_index (bailview->col_data,
	              BtkTreeViewColumn *, i));

          if (!stale_set)
            {
              /* Set all rows to BATK_STATE_STALE */
              traverse_cells (bailview, NULL, TRUE, FALSE);
              stale_set = TRUE;
            }

          /* Generate column-deleted signal */
          g_signal_emit_by_name (batk_obj, "column_deleted", i, 1);

          /* Generate children-changed signals */
          n_rows = get_row_count (btk_tree_view_get_model (tree_view));
          n_cols = get_n_actual_columns (tree_view);
          for (row = 0; row < n_rows; row++)
            {
             /*
              * Pass NULL as the child object, 4th argument.
              */
              g_signal_emit_by_name (batk_obj, "children_changed::remove",
                                    ((row * n_cols) + column_count), NULL, NULL);
            }
        }
    }
   
  /* rebuild the array */

  g_array_free (bailview->col_data, TRUE);
  bailview->col_data = g_array_sized_new (FALSE, TRUE,
    sizeof(BtkTreeViewColumn *), 0);

  for (tmp_list = tv_cols; tmp_list; tmp_list = tmp_list->next)
     g_array_append_val (bailview->col_data, tmp_list->data);
  g_list_free (tv_cols);
}

static void
cursor_changed (BtkTreeView *tree_view)
{
  BailTreeView *bailview;

  bailview = BAIL_TREE_VIEW (btk_widget_get_accessible (BTK_WIDGET (tree_view)));
  if (bailview->idle_cursor_changed_id != 0)
    return;

  /*
   * We notify the focus change in a idle handler so that the processing
   * of the cursor change is completed when the focus handler is called.
   * This will allow actions to be called in the focus handler
   */ 
  bailview->idle_cursor_changed_id = bdk_threads_add_idle (idle_cursor_changed, bailview);
}

static gint
idle_cursor_changed (gpointer data)
{
  BailTreeView *bail_tree_view = BAIL_TREE_VIEW (data);
  BtkTreeView *tree_view;
  BtkWidget *widget;
  BatkObject *cell;

  bail_tree_view->idle_cursor_changed_id = 0;

  widget = BTK_ACCESSIBLE (bail_tree_view)->widget;
  /*
   * Widget has been deleted
   */
  if (widget == NULL)
    return FALSE;

  tree_view = BTK_TREE_VIEW (widget);

  cell = bail_tree_view_ref_focus_cell (tree_view);
  if (cell)
    {
      if (cell != bail_tree_view->focus_cell)
        {
          if (bail_tree_view->focus_cell)
            {
              bail_cell_remove_state (BAIL_CELL (bail_tree_view->focus_cell), BATK_STATE_ACTIVE, FALSE); 
              bail_cell_remove_state (BAIL_CELL (bail_tree_view->focus_cell), BATK_STATE_FOCUSED, FALSE);
              g_object_unref (bail_tree_view->focus_cell);
            }
          bail_tree_view->focus_cell = cell;

          if (btk_widget_has_focus (widget))
            {
              bail_cell_add_state (BAIL_CELL (cell), BATK_STATE_ACTIVE, FALSE);
              bail_cell_add_state (BAIL_CELL (cell), BATK_STATE_FOCUSED, FALSE);
            }
          g_signal_emit_by_name (bail_tree_view,
                                 "active-descendant-changed",
                                 cell);
        }
      else
        g_object_unref (cell);
    }

  return FALSE;
}

static gboolean
focus_in (BtkWidget *widget)
{
  BtkTreeView *tree_view;
  BailTreeView *bail_tree_view;
  BatkStateSet *state_set;
  BatkObject *cell;

  tree_view = BTK_TREE_VIEW (widget);
  bail_tree_view = BAIL_TREE_VIEW (btk_widget_get_accessible (widget));

  if (bail_tree_view->focus_cell == NULL)
    {
      cell = bail_tree_view_ref_focus_cell (tree_view);
      if (cell)
        {
          state_set = batk_object_ref_state_set (cell);
          if (state_set)
            {
              if (!batk_state_set_contains_state (state_set, BATK_STATE_FOCUSED))
                {
                  bail_cell_add_state (BAIL_CELL (cell), BATK_STATE_ACTIVE, FALSE);
                  bail_tree_view->focus_cell = cell;
                  bail_cell_add_state (BAIL_CELL (cell), BATK_STATE_FOCUSED, FALSE);
                  g_signal_emit_by_name (bail_tree_view,
                                         "active-descendant-changed",
                                         cell);
                }
              g_object_unref (state_set);
            }
        }
    }
  return FALSE;
}

static gboolean
focus_out (BtkWidget *widget)
{
  BailTreeView *bail_tree_view;

  bail_tree_view = BAIL_TREE_VIEW (btk_widget_get_accessible (widget));
  if (bail_tree_view->focus_cell)
  {
    bail_cell_remove_state (BAIL_CELL (bail_tree_view->focus_cell), BATK_STATE_ACTIVE, FALSE);
    bail_cell_remove_state (BAIL_CELL (bail_tree_view->focus_cell), BATK_STATE_FOCUSED, FALSE);
    g_object_unref (bail_tree_view->focus_cell);
    bail_tree_view->focus_cell = NULL;
  }
  return FALSE;
}

static void
model_row_changed (BtkTreeModel *tree_model,
                   BtkTreePath  *path, 
                   BtkTreeIter  *iter,
                   gpointer     user_data)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW(user_data);
  BailTreeView *bailview;
  BtkTreePath *cell_path;
  GList *l;
  BailTreeViewCellInfo *cell_info;
 
  bailview = BAIL_TREE_VIEW (btk_widget_get_accessible (BTK_WIDGET (tree_view)));

  /* Loop through our cached cells */
  /* Must loop through them all */
  for (l = bailview->cell_data; l; l = l->next)
    {
      cell_info = (BailTreeViewCellInfo *) l->data;
      if (cell_info->in_use) 
      {
	  cell_path = btk_tree_row_reference_get_path (cell_info->cell_row_ref);

	  if (cell_path != NULL)
	  {
	      if (path && btk_tree_path_compare (cell_path, path) == 0)
	      {
		  if (BAIL_IS_RENDERER_CELL (cell_info->cell))
		  {
		      update_cell_value (BAIL_RENDERER_CELL (cell_info->cell),
					 bailview, TRUE);
		  }
	      }
	      btk_tree_path_free (cell_path);
	  }
      }
    }
  g_signal_emit_by_name (bailview, "visible-data-changed");
}

static void
column_visibility_changed (GObject    *object,
                           GParamSpec *pspec,
                           gpointer   user_data)
{
  if (strcmp (pspec->name, "visible") == 0)
    {
      /*
       * A column has been made visible or invisible
       *
       * We update our cache of cells and emit model_changed signal
       */ 
      BtkTreeView *tree_view = (BtkTreeView *)user_data;
      BailTreeView *bailview;
      GList *l;
      BailTreeViewCellInfo *cell_info;
      BtkTreeViewColumn *this_col = BTK_TREE_VIEW_COLUMN (object);
      BtkTreeViewColumn *tv_col;

      bailview = BAIL_TREE_VIEW (btk_widget_get_accessible (BTK_WIDGET (tree_view))
);
      g_signal_emit_by_name (bailview, "model_changed");

      for (l = bailview->cell_data; l; l = l->next)
        {
          cell_info = (BailTreeViewCellInfo *) l->data;
	  if (cell_info->in_use) 
	  {
	      tv_col = cell_info->cell_col_ref;
	      if (tv_col == this_col)
	      {
		  BtkTreePath *row_path;
		  row_path = btk_tree_row_reference_get_path (cell_info->cell_row_ref);
		  if (BAIL_IS_RENDERER_CELL (cell_info->cell))
		  {
		      if (btk_tree_view_column_get_visible (tv_col))
			  set_cell_visibility (tree_view, 
					       cell_info->cell, 
					       tv_col, row_path, FALSE);
		      else
		      {
			  bail_cell_remove_state (cell_info->cell, 
						  BATK_STATE_VISIBLE, TRUE);
			  bail_cell_remove_state (cell_info->cell, 
						  BATK_STATE_SHOWING, TRUE);
		      }
		  }
		  btk_tree_path_free (row_path);
	      }
	  }
        }
    }
}

/*
 * This is the signal handler for the "destroy" signal for a BtkTreeViewColumn
 *
 * We check whether we have stored column description or column header
 * and if so we get rid of it.
 */
static void
column_destroy (BtkObject *obj)
{
  BtkTreeViewColumn *tv_col = BTK_TREE_VIEW_COLUMN (obj);
  BatkObject *header;
  gchar *desc;

  header = g_object_get_qdata (G_OBJECT (tv_col),
                          quark_column_header_object);
  if (header)
    g_object_unref (header);
  desc = g_object_get_qdata (G_OBJECT (tv_col),
                           quark_column_desc_object);
  g_free (desc); 
}

static void
model_row_inserted (BtkTreeModel *tree_model,
                    BtkTreePath  *path, 
                    BtkTreeIter  *iter, 
                    gpointer     user_data)
{
  BtkTreeView *tree_view = (BtkTreeView *)user_data;
  BtkTreePath *path_copy;
  BatkObject *batk_obj = btk_widget_get_accessible (BTK_WIDGET (tree_view));
  BailTreeView *bailview = BAIL_TREE_VIEW (batk_obj);
  gint row, n_inserted, child_row;

  if (bailview->idle_expand_id)
    {
      g_source_remove (bailview->idle_expand_id);
      bailview->idle_expand_id = 0;

      /* don't do this if the insertion precedes the idle path, since it will now be invalid */
      if (path && bailview->idle_expand_path &&
	  (btk_tree_path_compare (path, bailview->idle_expand_path) > 0))
	  set_expand_state (tree_view, tree_model, bailview, bailview->idle_expand_path, FALSE);
      if (bailview->idle_expand_path) 
	  btk_tree_path_free (bailview->idle_expand_path);
    }
  /* Check to see if row is visible */
  row = get_row_from_tree_path (tree_view, path);

 /*
  * A row insert is not necessarily visible.  For example,
  * a row can be draged & dropped into another row, which
  * causes an insert on the model that isn't visible in the
  * view.  Only generate a signal if the inserted row is
  * visible.
  */
  if (row != -1)
    {
      BtkTreeIter iter;
      gint n_cols, col;

      btk_tree_model_get_iter (tree_model, &iter, path);

      /* Figure out number of visible children. */
      if (btk_tree_model_iter_has_child (tree_model, &iter))
        {
         /*
          * By passing path into this function, we find the number of
          * visible children of path.
          */
          n_inserted = 0;
          iterate_thru_children (tree_view, tree_model,
                                 path, NULL, &n_inserted, 0);

          /* Must add one to include the row that is being added */
          n_inserted++;
        }
      else
      n_inserted = 1;

      /* Set rows below the inserted row to BATK_STATE_STALE */
      traverse_cells (bailview, path, TRUE, TRUE);

      /* Generate row-inserted signal */
      g_signal_emit_by_name (batk_obj, "row_inserted", row, n_inserted);

      /* Generate children-changed signals */
      n_cols = bail_tree_view_get_n_columns (BATK_TABLE (batk_obj));
      for (child_row = row; child_row < (row + n_inserted); child_row++)
        {
          for (col = 0; col < n_cols; col++)
            {
             /*
              * Pass NULL as the child object, i.e. 4th argument
              */
              g_signal_emit_by_name (batk_obj, "children_changed::add",
                                    ((row * n_cols) + col), NULL, NULL);
            }
        }
    }
  else
    {
     /*
      * The row has been inserted inside another row.  This can
      * cause a row that previously couldn't be expanded to now
      * be expandable.
      */
      path_copy = btk_tree_path_copy (path);
      btk_tree_path_up (path_copy);
      set_expand_state (tree_view, tree_model, bailview, path_copy, TRUE);
      btk_tree_path_free (path_copy);
    }
}

static void
model_row_deleted (BtkTreeModel *tree_model,
                   BtkTreePath  *path, 
                   gpointer     user_data)
{
  BtkTreeView *tree_view;
  BtkTreePath *path_copy;
  BatkObject *batk_obj;
  BailTreeView *bailview;
  gint row, col, n_cols;

  tree_view = (BtkTreeView *)user_data;
  batk_obj = btk_widget_get_accessible (BTK_WIDGET (tree_view));
  bailview = BAIL_TREE_VIEW (batk_obj);

  if (bailview->idle_expand_id)
    {
      g_source_remove (bailview->idle_expand_id);
      btk_tree_path_free (bailview->idle_expand_path);
      bailview->idle_expand_id = 0;
    }
  /* Check to see if row is visible */
  clean_rows (bailview);

  /* Set rows at or below the specified row to BATK_STATE_STALE */
  traverse_cells (bailview, path, TRUE, TRUE);

  /*
   * If deleting a row with a depth > 1, then this may affect the
   * expansion/contraction of its parent(s).  Make sure this is
   * handled.
   */
  if (btk_tree_path_get_depth (path) > 1)
    {
      path_copy = btk_tree_path_copy (path);
      btk_tree_path_up (path_copy);
      set_expand_state (tree_view, tree_model, bailview, path_copy, TRUE);
      btk_tree_path_free (path_copy);
    }
  row = get_row_from_tree_path (tree_view, path);
  /*
   * If the row which is deleted is not visible because it is a child of
   * a collapsed row then row will be -1
   */
  if (row > 0)
    g_signal_emit_by_name (batk_obj, "row_deleted", row, 
                           bailview->n_children_deleted + 1);
  bailview->n_children_deleted = 0;

  /* Generate children-changed signals */
  n_cols = get_n_actual_columns (tree_view);
  for (col = 0; col < n_cols; col++)
  {
    /*
     * Pass NULL as the child object, 4th argument.
     */
    g_signal_emit_by_name (batk_obj, "children_changed::remove",
                           ((row * n_cols) + col), NULL, NULL);
  }
}

/* 
 * This function gets called when a row is deleted or when rows are
 * removed from the view due to a collapse event.  Note that the
 * count is the number of visible *children* of the deleted row,
 * so it does not include the row being deleted.
 *
 * As this function is called before the rows are removed we just note the
 * number of rows and then deal with it when we get a notification that
 * rows were deleted or collapsed.
 */
static void
destroy_count_func (BtkTreeView *tree_view, 
                    BtkTreePath *path,
                    gint        count,
                    gpointer    user_data)
{
  BatkObject *batk_obj = btk_widget_get_accessible (BTK_WIDGET (tree_view));
  BailTreeView *bailview = BAIL_TREE_VIEW (batk_obj);

  bail_return_if_fail (bailview->n_children_deleted == 0);
  bailview->n_children_deleted = count;
}

static void 
model_rows_reordered (BtkTreeModel *tree_model,
                      BtkTreePath  *path, 
                      BtkTreeIter  *iter,
                      gint         *new_order, 
                      gpointer     user_data)
{
  BtkTreeView *tree_view = (BtkTreeView *)user_data;
  BatkObject *batk_obj = btk_widget_get_accessible (BTK_WIDGET (tree_view));
  BailTreeView *bailview = BAIL_TREE_VIEW (batk_obj);

  if (bailview->idle_expand_id)
    {
      g_source_remove (bailview->idle_expand_id);
      btk_tree_path_free (bailview->idle_expand_path);
      bailview->idle_expand_id = 0;
    }
  traverse_cells (bailview, NULL, TRUE, FALSE);

  g_signal_emit_by_name (batk_obj, "row_reordered");
}

static void
adjustment_changed (BtkAdjustment *adjustment, 
                    BtkTreeView   *tree_view)
{
  BatkObject *batk_obj;
  BailTreeView* obj;

  /*
   * The scrollbars have changed
   */
  batk_obj = btk_widget_get_accessible (BTK_WIDGET (tree_view));
  obj = BAIL_TREE_VIEW (batk_obj);

  traverse_cells (obj, NULL, FALSE, FALSE);
}

static void
set_cell_visibility (BtkTreeView       *tree_view,
                     BailCell          *cell,
                     BtkTreeViewColumn *tv_col,
                     BtkTreePath       *tree_path,
                     gboolean          emit_signal)
{
  BdkRectangle cell_rect;

  /* Get these three values in tree coords */
  if (btk_widget_get_realized (BTK_WIDGET (tree_view)))
    btk_tree_view_get_cell_area (tree_view, tree_path, tv_col, &cell_rect);
  else
    cell_rect.height = 0;

  if (cell_rect.height > 0)
    {
      /*
       * The height will be zero for a cell for which an antecedent is not 
       * expanded
       */
      bail_cell_add_state (cell, BATK_STATE_VISIBLE, emit_signal);
      if (is_cell_showing (tree_view, &cell_rect))
        bail_cell_add_state (cell, BATK_STATE_SHOWING, emit_signal);
      else
        bail_cell_remove_state (cell, BATK_STATE_SHOWING, emit_signal);
    }
  else
    {
      bail_cell_remove_state (cell, BATK_STATE_VISIBLE, emit_signal);
      bail_cell_remove_state (cell, BATK_STATE_SHOWING, emit_signal);
    }
}

static gboolean 
is_cell_showing (BtkTreeView   *tree_view,
                 BdkRectangle  *cell_rect)
{
  BdkRectangle rect, *visible_rect;
  BdkRectangle rect1, *tree_cell_rect;
  gint bx, by;
  gboolean is_showing;
 /*
  * A cell is considered "SHOWING" if any part of the cell is in the visible 
  * area.  Other ways we could do this is by a cell's midpoint or if the cell 
  * is fully in the visible range.  Since we have the cell_rect x,y,width,height
  * of the cell, any of these is easy to compute.
  *
  * It is assumed that cell's rectangle is in widget coordinates so we
  * must transform to tree cordinates.
  */
  visible_rect = &rect;
  tree_cell_rect = &rect1;
  tree_cell_rect->x = cell_rect->x;
  tree_cell_rect->y = cell_rect->y;
  tree_cell_rect->width = cell_rect->width;
  tree_cell_rect->height = cell_rect->height;

  btk_tree_view_get_visible_rect (tree_view, visible_rect);
  btk_tree_view_convert_tree_to_bin_window_coords (tree_view, visible_rect->x, 
                                                   visible_rect->y, &bx, &by);

  if (((tree_cell_rect->x + tree_cell_rect->width) < bx) ||
     ((tree_cell_rect->y + tree_cell_rect->height) < by) ||
     (tree_cell_rect->x > (bx + visible_rect->width)) ||
     (tree_cell_rect->y > (by + visible_rect->height)))
    is_showing =  FALSE;
  else
    is_showing = TRUE;

  return is_showing;
}

/* Misc Public */

/*
 * This function is called when a cell's flyweight is created in
 * bail_tree_view_table_ref_at with emit_change_signal set to FALSE
 * and in model_row_changed() on receipt of "row-changed" signal when 
 * emit_change_signal is set to TRUE
 */
static gboolean
update_cell_value (BailRendererCell *renderer_cell,
                   BailTreeView     *bailview,
                   gboolean         emit_change_signal)
{
  BailTreeViewCellInfo *cell_info;
  BtkTreeView *tree_view;
  BtkTreeModel *tree_model;
  BtkTreePath *path;
  BtkTreeIter iter;
  GList *renderers, *cur_renderer;
  GParamSpec *spec;
  BailRendererCellClass *bail_renderer_cell_class;
  BtkCellRendererClass *btk_cell_renderer_class;
  BailCell *cell;
  gchar **prop_list;
  BatkObject *parent;
  gboolean is_expander, is_expanded;
  
  bail_renderer_cell_class = BAIL_RENDERER_CELL_GET_CLASS (renderer_cell);
  if (renderer_cell->renderer)
    btk_cell_renderer_class = BTK_CELL_RENDERER_GET_CLASS (renderer_cell->renderer);
  else
    btk_cell_renderer_class = NULL;

  prop_list = bail_renderer_cell_class->property_list;

  cell = BAIL_CELL (renderer_cell);
  cell_info = find_cell_info (bailview, cell, NULL, TRUE);
  bail_return_val_if_fail (cell_info, FALSE);
  bail_return_val_if_fail (cell_info->cell_col_ref, FALSE);
  bail_return_val_if_fail (cell_info->cell_row_ref, FALSE);

  if (emit_change_signal && cell_info->in_use)
    {
      tree_view = BTK_TREE_VIEW (BTK_ACCESSIBLE (bailview)->widget);
      tree_model = btk_tree_view_get_model (tree_view);
      path = btk_tree_row_reference_get_path (cell_info->cell_row_ref);
      if (path == NULL)
        return FALSE;

      btk_tree_model_get_iter (tree_model, &iter, path);
      is_expander = FALSE;
      is_expanded = FALSE;
      if (btk_tree_model_iter_has_child (tree_model, &iter))
        {
          BtkTreeViewColumn *expander_tv;

          expander_tv = btk_tree_view_get_expander_column (tree_view);
          if (expander_tv == cell_info->cell_col_ref)
            {
              is_expander = TRUE;
              is_expanded = btk_tree_view_row_expanded (tree_view, path);
            }
        } 
      btk_tree_path_free (path);
      btk_tree_view_column_cell_set_cell_data (cell_info->cell_col_ref,
                                  tree_model, &iter, is_expander, is_expanded);
    }
  renderers = btk_cell_layout_get_cells (BTK_CELL_LAYOUT (cell_info->cell_col_ref));
  bail_return_val_if_fail (renderers, FALSE);

  /*
   * If the cell is in a container, it's index is used to find the renderer 
   * in the list
   */

  /*
   * Otherwise, we assume that the cell is represented by the first renderer 
   * in the list
   */

  if (cell_info->in_use) {
      parent = batk_object_get_parent (BATK_OBJECT (cell));
      if (!BATK_IS_OBJECT (cell)) g_on_error_query (NULL);
      if (BAIL_IS_CONTAINER_CELL (parent))
	  cur_renderer = g_list_nth (renderers, cell->index);
      else
	  cur_renderer = renderers;
  }
  else {
      return FALSE;
  }
  
  bail_return_val_if_fail (cur_renderer != NULL, FALSE);

  if (btk_cell_renderer_class)
    {
      while (*prop_list)
        {
          spec = g_object_class_find_property
                           (G_OBJECT_CLASS (btk_cell_renderer_class), *prop_list);

          if (spec != NULL)
            {
              GValue value = { 0, };

              g_value_init (&value, spec->value_type);
              g_object_get_property (cur_renderer->data, *prop_list, &value);
              g_object_set_property (G_OBJECT (renderer_cell->renderer),
                                     *prop_list, &value);
              g_value_unset(&value);
            }
          else
            g_warning ("Invalid property: %s\n", *prop_list);
          prop_list++;
        }
    }
  g_list_free (renderers);
  return bail_renderer_cell_update_cache (renderer_cell, emit_change_signal);
}

static gboolean 
set_iter_nth_row (BtkTreeView *tree_view, 
                  BtkTreeIter *iter, 
                  gint        row)
{
  BtkTreeModel *tree_model;
  
  tree_model = btk_tree_view_get_model (tree_view);
  btk_tree_model_get_iter_first (tree_model, iter);
  if (return_iter_nth_row (tree_view, tree_model, iter, 0 , row) != NULL)
    return TRUE;
    
  return FALSE;
}

static gint 
get_row_from_tree_path (BtkTreeView *tree_view,
                        BtkTreePath *path)
{
  BtkTreeModel *tree_model;
  BtkTreePath *root_tree;
  gint row;

  tree_model = btk_tree_view_get_model (tree_view);

  if (btk_tree_model_get_flags (tree_model) & BTK_TREE_MODEL_LIST_ONLY)
    row = btk_tree_path_get_indices (path)[0];
  else
    {
      root_tree = btk_tree_path_new_first ();
      row = 0;
      iterate_thru_children (tree_view, tree_model, root_tree, path, &row, 0);
      btk_tree_path_free (root_tree);
    }

  return row;
}

/* Misc Private */

/*
 * Get the specified BtkTreeViewColumn in the BtkTreeView.
 * Only visible columns are considered.
 */
static BtkTreeViewColumn* 
get_column (BtkTreeView *tree_view, 
            gint        in_col)
{
  BtkTreeViewColumn *tv_col;
  gint n_cols = -1;
  gint i = 0;
 
  if (in_col < 0)
    {
       g_warning ("Request for invalid column %d\n", in_col);
       return NULL;
    }

  tv_col = btk_tree_view_get_column (tree_view, i);

  while (tv_col != NULL)
    {
      if (btk_tree_view_column_get_visible (tv_col)) 
        n_cols++;
      if (in_col == n_cols)
        break;
      tv_col = btk_tree_view_get_column (tree_view, ++i);
    }

  if (in_col != n_cols)
    {
       g_warning ("Request for invalid column %d\n", in_col);
       return NULL;
    }
  return tv_col;
}

static gint
get_actual_column_number (BtkTreeView *tree_view,
                          gint        visible_column)
{
  BtkTreeViewColumn *tv_col;
  gint actual_column = 0;
  gint visible_columns = -1;
  /*
   * This function calculates the column number which corresponds to the
   * specified visible column number
   */
  tv_col = btk_tree_view_get_column (tree_view, actual_column);

  while (tv_col != NULL)
    {
      if (btk_tree_view_column_get_visible (tv_col)) 
        visible_columns++;
      if (visible_columns == visible_column)
        return actual_column;
      tv_col = btk_tree_view_get_column (tree_view, ++actual_column);
    }
  g_warning ("get_actual_column_number failed for %d\n", visible_column);
  return -1;
}

static gint
get_visible_column_number (BtkTreeView *tree_view,
                           gint        actual_column)
{
  BtkTreeViewColumn *tv_col;
  gint column = 0;
  gint visible_columns = -1;
  /*
   * This function calculates the visible column number which corresponds to the
   * specified actual column number
   */
  tv_col = btk_tree_view_get_column (tree_view, column);

  while (tv_col != NULL)
    {
      if (btk_tree_view_column_get_visible (tv_col)) 
        {
          visible_columns++;
          if (actual_column == column)
            return visible_columns;
        }
      else
        if (actual_column == column)
          return -1;
      tv_col = btk_tree_view_get_column (tree_view, ++column);
    }
  g_warning ("get_visible_column_number failed for %d\n", actual_column);
  return -1;
}

/**
 * Helper recursive function that returns BtkTreeIter pointer to nth row.
 **/
static BtkTreeIter* 
return_iter_nth_row(BtkTreeView  *tree_view,
                    BtkTreeModel *tree_model, 
                    BtkTreeIter  *iter, 
                    gint         increment,
                    gint         row)
{
  BtkTreePath *current_path = btk_tree_model_get_path (tree_model, iter);
  BtkTreeIter new_iter;
  gboolean row_expanded;

  if (increment == row) {
    btk_tree_path_free (current_path);
    return iter;
  }

  row_expanded = btk_tree_view_row_expanded (tree_view, current_path);
  btk_tree_path_free (current_path);

  new_iter = *iter;
  if ((row_expanded && btk_tree_model_iter_children (tree_model, iter, &new_iter)) ||
      (btk_tree_model_iter_next (tree_model, iter)) ||
      (btk_tree_model_iter_parent (tree_model, iter, &new_iter) &&
          (btk_tree_model_iter_next (tree_model, iter))))
    return return_iter_nth_row (tree_view, tree_model, iter,
      ++increment, row);

  return NULL;	 	
}

/**
 * Recursively called until the row specified by orig is found.
 *
 * *count will be set to the visible row number of the child
 * relative to the row that was initially passed in as tree_path.
 *
 * *count will be -1 if orig is not found as a child (a row that is
 * not visible will not be found, e.g. if the row is inside a
 * collapsed row).  If NULL is passed in as orig, *count will
 * be a count of the visible children.
 *
 * NOTE: the value for depth must be 0 when this recursive function
 * is initially called, or it may not function as expected.
 **/
static void 
iterate_thru_children(BtkTreeView  *tree_view,
                      BtkTreeModel *tree_model,
                      BtkTreePath  *tree_path,
                      BtkTreePath  *orig,
                      gint         *count,
                      gint         depth)
{
  BtkTreeIter iter;

  if (!btk_tree_model_get_iter (tree_model, &iter, tree_path))
    return;

  if (tree_path && orig && !btk_tree_path_compare (tree_path, orig)) 
    /* Found it! */
    return;

  if (tree_path && orig && btk_tree_path_compare (tree_path, orig) > 0)
    {
      /* Past it, so return -1 */
      *count = -1;
      return;
    }
  else if (btk_tree_view_row_expanded (tree_view, tree_path) && 
    btk_tree_model_iter_has_child (tree_model, &iter)) 
    {
      (*count)++;
      btk_tree_path_append_index (tree_path, 0);
      iterate_thru_children (tree_view, tree_model, tree_path,
                             orig, count, (depth + 1));
      return;
    }
  else if (btk_tree_model_iter_next (tree_model, &iter)) 
    {
      (*count)++;
      tree_path = btk_tree_model_get_path (tree_model, &iter);
       if (tree_path)
         {
           iterate_thru_children (tree_view, tree_model, tree_path,
                                 orig, count, depth); 
           btk_tree_path_free (tree_path);
         }
      return;
  }
  else if (btk_tree_path_up (tree_path))
    {
      BtkTreeIter temp_iter;
      gboolean exit_loop = FALSE;
      gint new_depth = depth - 1;

      (*count)++;

     /*
      * Make sure that we back up until we find a row
      * where btk_tree_path_next does not return NULL.
      */
      while (!exit_loop)
        {
          if (btk_tree_path_get_depth (tree_path) == 0)
              /* depth is now zero so */
            return;
          btk_tree_path_next (tree_path);	

          /* Verify that the next row is a valid row! */
          exit_loop = btk_tree_model_get_iter (tree_model, &temp_iter, tree_path);

          if (!exit_loop)
            {
              /* Keep going up until we find a row that has a valid next */
              if (btk_tree_path_get_depth(tree_path) > 1)
                {
                  new_depth--;
                  btk_tree_path_up (tree_path);
                }
              else
                {
                 /*
                  * If depth is 1 and btk_tree_model_get_iter returns FALSE,
                  * then we are at the last row, so just return.
                  */ 
                  if (orig != NULL)
                    *count = -1;

                  return;
                }
            }
        }

     /*
      * This guarantees that we will stop when we hit the end of the
      * children.
      */
      if (new_depth < 0)
        return;

      iterate_thru_children (tree_view, tree_model, tree_path,
                            orig, count, new_depth);
      return;
    }

 /*
  * If it gets here, then the path wasn't found.  Situations
  * that would cause this would be if the path passed in is
  * invalid or contained within the last row, but not visible
  * because the last row is not expanded.  If NULL was passed
  * in then a row count is desired, so only set count to -1
  * if orig is not NULL.
  */
  if (orig != NULL)
    *count = -1;

  return;
}

static void
clean_cell_info (BailTreeView *bailview,
                 GList        *list) 
{
  BailTreeViewCellInfo *cell_info;
  GObject *obj;

  g_assert (BAIL_IS_TREE_VIEW (bailview));

  cell_info = list->data;

  if (cell_info->in_use) {
      obj = G_OBJECT (cell_info->cell);
      
      bail_cell_add_state (cell_info->cell, BATK_STATE_DEFUNCT, FALSE);
      g_object_weak_unref (obj, (GWeakNotify) cell_destroyed, cell_info);
      cell_info->in_use = FALSE; 
      if (!bailview->garbage_collection_pending) {
	  bailview->garbage_collection_pending = TRUE;
          g_assert (bailview->idle_garbage_collect_id == 0);
	  bailview->idle_garbage_collect_id = 
	    bdk_threads_add_idle (idle_garbage_collect_cell_data, bailview);
      }
  }
}

static void 
clean_rows (BailTreeView *bailview)
{
  GArray *array;

  /* Clean BailTreeViewRowInfo data */

  array = bailview->row_data;
  if (array != NULL)
    {
      BailTreeViewRowInfo *row_info;
      BtkTreePath *row_path;
      gint i;

     /*
      * Loop backwards so that calls to free_row_info
      * do not affect the index numbers 
      */
      for (i = (array->len - 1); i >= 0; i  --)
        {
          row_info = g_array_index (array, BailTreeViewRowInfo*, i);
          row_path = btk_tree_row_reference_get_path (row_info->row_ref);

          /* Remove any rows that have become invalid */
          if (row_path == NULL)
            free_row_info (array, i, TRUE);
          else
            btk_tree_path_free (row_path);
        }
    }

  /* Clean BailTreeViewCellInfo data */

  if (bailview->cell_data != NULL)
    {
      BailTreeViewCellInfo *cell_info;
      BtkTreePath *row_path;
      GList *cur_list;
      GList *temp_list;

      temp_list = bailview->cell_data;

      /* Must loop through them all */
      while (temp_list != NULL)
        {
          cur_list = temp_list;
          cell_info = temp_list->data;
          temp_list = temp_list->next;
          row_path = btk_tree_row_reference_get_path (cell_info->cell_row_ref);

         /*
          * If the cell has become invalid because the row has been removed, 
          * then set the cell's state to BATK_STATE_DEFUNCT and remove the cell
          * from bailview->cell_data.  If row_path is NULL then the row has
          * been removed.
          */
          if (row_path == NULL)
            {
              clean_cell_info (bailview, cur_list);
            }
          else
            {
              btk_tree_path_free (row_path);
            }
        }
    }
}

static void 
clean_cols (BailTreeView      *bailview,
            BtkTreeViewColumn *tv_col)
{
  /* Clean BailTreeViewCellInfo data */

  if (bailview->cell_data != NULL)
    {
      BailTreeViewCellInfo *cell_info;
      GList *cur_list, *temp_list;

      temp_list = bailview->cell_data;

      while (temp_list != NULL)
        {
          cur_list = temp_list;
          cell_info = temp_list->data;
          temp_list = temp_list->next;

         /*
          * If the cell has become invalid because the column tv_col
          * has been removed, then set the cell's state to BATK_STATE_DEFUNCT
          * and remove the cell from bailview->cell_data. 
          */
          if (cell_info->cell_col_ref == tv_col)
            {
              clean_cell_info (bailview, cur_list);
            }
        }
    }
}

static gboolean
idle_garbage_collect_cell_data (gpointer data)
{
      BailTreeView *tree_view;

      g_assert (BAIL_IS_TREE_VIEW (data));
      tree_view = (BailTreeView *)data;

      /* this is the idle handler (only one instance allowed), so
       * we can safely delete it.
       */
      tree_view->garbage_collection_pending = FALSE;
      tree_view->idle_garbage_collect_id = 0;

      tree_view->garbage_collection_pending = garbage_collect_cell_data (data);

      /* N.B.: if for some reason another handler has re-enterantly been queued
       * while this handler was being serviced, it has its own gsource, therefore this handler
       * should always return FALSE.
       */
      return FALSE; 
}

static gboolean
garbage_collect_cell_data (gpointer data)
{
      BailTreeView *tree_view;
      GList *temp_list, *list;
      BailTreeViewCellInfo *cell_info;

      g_assert (BAIL_IS_TREE_VIEW (data));
      tree_view = (BailTreeView *)data;
      list = g_list_copy (tree_view->cell_data);

      tree_view->garbage_collection_pending = FALSE;
      if (tree_view->idle_garbage_collect_id != 0) 
      {
	  g_source_remove (tree_view->idle_garbage_collect_id);
	  tree_view->idle_garbage_collect_id = 0;
      }

      /* Must loop through them all */
      temp_list = list;
      while (temp_list != NULL)
      {
          cell_info = temp_list->data;
	  if (!cell_info->in_use)
	  {
	      /* g_object_unref (cell_info->cell); */
	      tree_view->cell_data = g_list_remove (tree_view->cell_data, 
						    cell_info);
	      if (cell_info->cell_row_ref)
		  btk_tree_row_reference_free (cell_info->cell_row_ref);
	      g_free (cell_info);
	  }
          temp_list = temp_list->next;
      }
      g_list_free (list);

      return tree_view->garbage_collection_pending;
}

/**
 * If tree_path is passed in as NULL, then all cells are acted on.
 * Otherwise, just act on those cells that are on a row greater than 
 * the specified tree_path. If inc_row is passed in as TRUE, then rows 
 * greater and equal to the specified tree_path are acted on.
 *
 * if set_stale is set the BATK_STATE_STALE is set on cells which are to be
 * acted on. 
 *
 * The function set_cell_visibility() is called on all cells to be
 * acted on to update the visibility of the cell.
 **/
static void 
traverse_cells (BailTreeView *tree_view,
                BtkTreePath  *tree_path,
                gboolean     set_stale,
                gboolean     inc_row)
{
  if (tree_view->cell_data != NULL)
    {
      BailTreeViewCellInfo *cell_info;
      BtkTreeView *btk_tree_view;
      GList *temp_list;
      BtkWidget *widget;

      g_assert (BTK_IS_ACCESSIBLE (tree_view));

      widget = BTK_ACCESSIBLE (tree_view)->widget;
      if (!widget)
        /* Widget is being deleted */
        return;

      btk_tree_view = BTK_TREE_VIEW (widget);
      temp_list = tree_view->cell_data;

      /* Must loop through them all */
      while (temp_list != NULL)
        {
          BtkTreePath *row_path;
          gboolean act_on_cell;

          cell_info = temp_list->data;
          temp_list = temp_list->next;

	  if (cell_info->in_use)
	  {
	      row_path = btk_tree_row_reference_get_path (cell_info->cell_row_ref);
	      g_return_if_fail (row_path != NULL);
	      if (tree_path == NULL)
		  act_on_cell = TRUE;
	      else 
	      {
		  gint comparison;
		  
		  comparison =  btk_tree_path_compare (row_path, tree_path);
		  if ((comparison > 0) ||
		      (comparison == 0 && inc_row))
		      act_on_cell = TRUE;
		  else
		      act_on_cell = FALSE;
	      }
	      if (!cell_info->in_use) g_warning ("warning: cell info destroyed during traversal");
	      if (act_on_cell && cell_info->in_use)
	      {
		  if (set_stale)
		      bail_cell_add_state (cell_info->cell, BATK_STATE_STALE, TRUE);
		  set_cell_visibility (btk_tree_view,
				       cell_info->cell,
				       cell_info->cell_col_ref,
				       row_path, TRUE);
	      }
	      btk_tree_path_free (row_path);
	  }
	}
    }
  g_signal_emit_by_name (tree_view, "visible-data-changed");
}

static void
free_row_info (GArray   *array,
               gint     array_idx,
               gboolean shift)
{
  BailTreeViewRowInfo* obj;

  obj = g_array_index (array, BailTreeViewRowInfo*, array_idx);

  g_free (obj->description);
  if (obj->row_ref != NULL)
    btk_tree_row_reference_free (obj->row_ref);
  if (obj->header)
    g_object_unref (obj->header);
  g_free (obj);

  if (shift)
    g_array_remove_index (array, array_idx);
}

/*
 * If the tree_path passed in has children, then
 * BATK_STATE_EXPANDABLE is set.  If the row is expanded
 * BATK_STATE_EXPANDED is turned on.  If the row is 
 * collapsed, then BATK_STATE_EXPANDED is removed.
 * 
 * If the tree_path passed in has no children, then
 * BATK_STATE_EXPANDABLE and BATK_STATE_EXPANDED are removed.
 *
 * If set_on_ancestor is TRUE, then this function will also
 * update all cells that are ancestors of the tree_path.
 */
static void
set_expand_state (BtkTreeView  *tree_view,
                  BtkTreeModel *tree_model,
                  BailTreeView *bailview,
                  BtkTreePath  *tree_path,
                  gboolean     set_on_ancestor)
{
  if (bailview->cell_data != NULL)
    {
      BtkTreeViewColumn *expander_tv;
      BailTreeViewCellInfo *cell_info;
      GList *temp_list;
      BtkTreePath *cell_path;
      BtkTreeIter iter;
      gboolean found;

      temp_list = bailview->cell_data;

      while (temp_list != NULL)
        {
          cell_info = temp_list->data;
          temp_list = temp_list->next;
	  if (cell_info->in_use)
	  {
	      cell_path = btk_tree_row_reference_get_path (cell_info->cell_row_ref);
	      found = FALSE;
	      
	      if (cell_path != NULL)
	      {
		  BailCell *cell  = BAIL_CELL (cell_info->cell);
		  
		  expander_tv = btk_tree_view_get_expander_column (tree_view);
		  
		  /*
		   * Only set state for the cell that is in the column with the
		   * expander toggle
		   */
		  if (expander_tv == cell_info->cell_col_ref)
		  {
		      if (tree_path && btk_tree_path_compare (cell_path, tree_path) == 0)
			  found = TRUE;
		      else if (set_on_ancestor &&
			       btk_tree_path_get_depth (cell_path) <
			       btk_tree_path_get_depth (tree_path) && 
			       btk_tree_path_is_ancestor (cell_path, tree_path) == 1)
			  /* Only set if set_on_ancestor was passed in as TRUE */
			  found = TRUE;
		  }
		  
		  /*
		   * Set BATK_STATE_EXPANDABLE and BATK_STATE_EXPANDED
		   * for ancestors and found cells.
		   */
		  if (found)
		  {
		      /*
		       * Must check against cell_path since cell_path
		       * can be equal to or an ancestor of tree_path.
		       */
		      btk_tree_model_get_iter (tree_model, &iter, cell_path);
		      
		      /* Set or unset BATK_STATE_EXPANDABLE as appropriate */
		      if (btk_tree_model_iter_has_child (tree_model, &iter)) 
		      {
			  set_cell_expandable (cell);
			  
			  if (btk_tree_view_row_expanded (tree_view, cell_path))
			      bail_cell_add_state (cell, BATK_STATE_EXPANDED, TRUE);
			  else
			      bail_cell_remove_state (cell, 
						      BATK_STATE_EXPANDED, TRUE);
		      }
		      else
		      {
			  bail_cell_remove_state (cell, 
						  BATK_STATE_EXPANDED, TRUE);
			  if (bail_cell_remove_state (cell,
						      BATK_STATE_EXPANDABLE, TRUE))
			      /* The state may have been propagated to the container cell */
			      if (!BAIL_IS_CONTAINER_CELL (cell))
				  bail_cell_remove_action_by_name (cell,
								   "expand or contract");
		      }
		      
		      /*
		       * We assume that each cell in the cache once and
		       * a container cell is before its child cells so we are 
		       * finished if set_on_ancestor is not set to TRUE.
		       */
		      if (!set_on_ancestor)
			  break;
		  }
	      }
	      btk_tree_path_free (cell_path);
	  }
	}
    }
}


static void
add_cell_actions (BailCell *cell,
                  gboolean editable)
{
  if (BAIL_IS_BOOLEAN_CELL (cell))
    bail_cell_add_action (cell,
	"toggle",
	"toggles the cell", /* action description */
	NULL,
	toggle_cell_toggled);
  if (editable)
    bail_cell_add_action (cell,
	"edit",
	"creates a widget in which the contents of the cell can be edited", 
	NULL,
	edit_cell);
  bail_cell_add_action (cell,
	"activate",
	"activate the cell", 
	NULL,
	activate_cell);
}

static void
toggle_cell_expanded (BailCell *cell)
{
  BailTreeViewCellInfo *cell_info;
  BtkTreeView *tree_view;
  BtkTreePath *path;
  BatkObject *parent;
  BatkStateSet *stateset;
  
  parent = batk_object_get_parent (BATK_OBJECT (cell));
  if (BAIL_IS_CONTAINER_CELL (parent))
    parent = batk_object_get_parent (parent);

  cell_info = find_cell_info (BAIL_TREE_VIEW (parent), cell, NULL, TRUE);
  bail_return_if_fail (cell_info);
  bail_return_if_fail (cell_info->cell_col_ref);
  bail_return_if_fail (cell_info->cell_row_ref);

  tree_view = BTK_TREE_VIEW (BTK_ACCESSIBLE (parent)->widget);
  path = btk_tree_row_reference_get_path (cell_info->cell_row_ref);
  bail_return_if_fail (path);

  stateset = batk_object_ref_state_set (BATK_OBJECT (cell));
  if (batk_state_set_contains_state (stateset, BATK_STATE_EXPANDED))
    btk_tree_view_collapse_row (tree_view, path);
  else
    btk_tree_view_expand_row (tree_view, path, TRUE);
  g_object_unref (stateset);
  btk_tree_path_free (path);
  return;
}

static void
toggle_cell_toggled (BailCell *cell)
{
  BailTreeViewCellInfo *cell_info;
  BtkTreePath *path;
  gchar *pathstring;
  GList *renderers, *cur_renderer;
  BatkObject *parent;
  gboolean is_container_cell = FALSE;

  parent = batk_object_get_parent (BATK_OBJECT (cell));
  if (BAIL_IS_CONTAINER_CELL (parent))
    {
      is_container_cell = TRUE;
      parent = batk_object_get_parent (parent);
    }

  cell_info = find_cell_info (BAIL_TREE_VIEW (parent), cell, NULL, TRUE);
  bail_return_if_fail (cell_info);
  bail_return_if_fail (cell_info->cell_col_ref);
  bail_return_if_fail (cell_info->cell_row_ref);

  path = btk_tree_row_reference_get_path (cell_info->cell_row_ref);
  bail_return_if_fail (path);
  pathstring = btk_tree_path_to_string (path);

  renderers = btk_cell_layout_get_cells (BTK_CELL_LAYOUT (cell_info->cell_col_ref));
  bail_return_if_fail (renderers);

  /* 
   * if the cell is in a container, it's index is used to find the 
   * renderer in the list
   */

  if (is_container_cell)
    cur_renderer = g_list_nth (renderers, cell->index);
  else
  /*
   * Otherwise, we assume that the cell is represented by the first 
   * renderer in the list 
   */
    cur_renderer = renderers;

  bail_return_if_fail (cur_renderer);

  g_signal_emit_by_name (cur_renderer->data, "toggled", pathstring);
  g_list_free (renderers);
  g_free (pathstring);
  btk_tree_path_free (path);
  return;
}

static void
edit_cell (BailCell *cell)
{
  BailTreeViewCellInfo *cell_info;
  BtkTreeView *tree_view;
  BtkTreePath *path;
  BatkObject *parent;

  editing = TRUE;
  parent = batk_object_get_parent (BATK_OBJECT (cell));
  if (BAIL_IS_CONTAINER_CELL (parent))
    {
      parent = batk_object_get_parent (parent);
    }

  cell_info = find_cell_info (BAIL_TREE_VIEW (parent), cell, NULL, TRUE);
  bail_return_if_fail (cell_info);
  bail_return_if_fail (cell_info->cell_col_ref);
  bail_return_if_fail (cell_info->cell_row_ref);

  tree_view = BTK_TREE_VIEW (BTK_ACCESSIBLE (parent)->widget);
  path = btk_tree_row_reference_get_path (cell_info->cell_row_ref);
  bail_return_if_fail (path);
  btk_tree_view_set_cursor (tree_view, path, cell_info->cell_col_ref, TRUE);
  btk_tree_path_free (path);
  return;
}

static void
activate_cell (BailCell *cell)
{
  BailTreeViewCellInfo *cell_info;
  BtkTreeView *tree_view;
  BtkTreePath *path;
  BatkObject *parent;

  editing = TRUE;
  parent = batk_object_get_parent (BATK_OBJECT (cell));
  if (BAIL_IS_CONTAINER_CELL (parent))
    {
      parent = batk_object_get_parent (parent);
    }

  cell_info = find_cell_info (BAIL_TREE_VIEW (parent), cell, NULL, TRUE);
  bail_return_if_fail (cell_info);
  bail_return_if_fail (cell_info->cell_col_ref);
  bail_return_if_fail (cell_info->cell_row_ref);

  tree_view = BTK_TREE_VIEW (BTK_ACCESSIBLE (parent)->widget);
  path = btk_tree_row_reference_get_path (cell_info->cell_row_ref);
  bail_return_if_fail (path);
  btk_tree_view_row_activated (tree_view, path, cell_info->cell_col_ref);
  btk_tree_path_free (path);
  return;
}

static void
cell_destroyed (gpointer data)
{
  BailTreeViewCellInfo *cell_info = data;

  bail_return_if_fail (cell_info);
  if (cell_info->in_use) {
      cell_info->in_use = FALSE;

      g_assert (BAIL_IS_TREE_VIEW (cell_info->view));
      if (!cell_info->view->garbage_collection_pending) {
	  cell_info->view->garbage_collection_pending = TRUE;
	  cell_info->view->idle_garbage_collect_id =
	    bdk_threads_add_idle (idle_garbage_collect_cell_data, cell_info->view);
      }
  }
}

#if 0
static void
cell_info_remove (BailTreeView *tree_view, 
                  BailCell     *cell)
{
  BailTreeViewCellInfo *info;
  GList *temp_list;

  info = find_cell_info (tree_view, cell, &temp_list, FALSE);
  if (info)
    {
      info->in_use = FALSE;
      return;
    }
  g_warning ("No cell removed in cell_info_remove\n");
}
#endif

static void
cell_info_get_index (BtkTreeView            *tree_view, 
                     BailTreeViewCellInfo   *info,
                     gint                   *index)
{
  BtkTreePath *path;
  gint column_number;

  path = btk_tree_row_reference_get_path (info->cell_row_ref);
  bail_return_if_fail (path);

  column_number = get_column_number (tree_view, info->cell_col_ref, FALSE);
  *index = get_index (tree_view, path, column_number);
  btk_tree_path_free (path);
}

static void
cell_info_new (BailTreeView      *bailview, 
               BtkTreeModel      *tree_model, 
               BtkTreePath       *path,
               BtkTreeViewColumn *tv_col,
               BailCell          *cell )
{
  BailTreeViewCellInfo *cell_info;

  g_assert (BAIL_IS_TREE_VIEW (bailview));

  cell_info = g_new (BailTreeViewCellInfo, 1);
  cell_info->cell_row_ref = btk_tree_row_reference_new (tree_model, path);

  cell_info->cell_col_ref = tv_col;
  cell_info->cell = cell;
  cell_info->in_use = TRUE; /* if we've created it, assume it's in use */
  cell_info->view = bailview;
  bailview->cell_data = g_list_append (bailview->cell_data, cell_info);
      
  /* Setup weak reference notification */

  g_object_weak_ref (G_OBJECT (cell),
                     (GWeakNotify) cell_destroyed,
                     cell_info);
}

static BailCell*
find_cell (BailTreeView *bailview, 
           gint         index)
{
  BailTreeViewCellInfo *info;
  BtkTreeView *tree_view;
  GList *cell_list;
  GList *l;
  gint real_index;
  gboolean needs_cleaning = FALSE;
  BailCell *retval = NULL;

  tree_view = BTK_TREE_VIEW (BTK_ACCESSIBLE (bailview)->widget);
  cell_list = bailview->cell_data;

  for (l = cell_list; l; l = l->next)
    {
      info = (BailTreeViewCellInfo *) (l->data);
      if (info->in_use)
      {
	  cell_info_get_index (tree_view, info, &real_index);
	  if (index == real_index)
	  {
	      retval =  info->cell;
	      break;
	  }
      }
      else
      {
	  needs_cleaning = TRUE;
      }
    }
  if (needs_cleaning)
     garbage_collect_cell_data (bailview);

  return retval;
}

static void
refresh_cell_index (BailCell *cell)
{
  BailTreeViewCellInfo *info;
  BatkObject *parent;
  BtkTreeView *tree_view;
  gint index;

  parent = batk_object_get_parent (BATK_OBJECT (cell));
  bail_return_if_fail (BAIL_IS_TREE_VIEW (parent));

  tree_view = BTK_TREE_VIEW (BTK_ACCESSIBLE (parent)->widget);

  /* Find this cell in the BailTreeView's cache */

  info = find_cell_info (BAIL_TREE_VIEW (parent), cell, NULL, TRUE);
  bail_return_if_fail (info);
  
  cell_info_get_index (tree_view, info, &index); 
  cell->index = index;
}

static void
get_selected_rows (BtkTreeModel *model,
                   BtkTreePath  *path,
                   BtkTreeIter  *iter,
                   gpointer     data)
{
  GPtrArray *array = (GPtrArray *)data;

  g_ptr_array_add (array, btk_tree_path_copy (path));
}

static void
connect_model_signals (BtkTreeView  *view,
                       BailTreeView *bailview)
{
  GObject *obj;

  obj = G_OBJECT (bailview->tree_model);
  g_signal_connect_data (obj, "row-changed",
                         (GCallback) model_row_changed, view, NULL, 0);
  g_signal_connect_data (obj, "row-inserted",
                         (GCallback) model_row_inserted, view, NULL, 
                         G_CONNECT_AFTER);
  g_signal_connect_data (obj, "row-deleted",
                         (GCallback) model_row_deleted, view, NULL, 
                         G_CONNECT_AFTER);
  g_signal_connect_data (obj, "rows-reordered",
                         (GCallback) model_rows_reordered, view, NULL, 
                         G_CONNECT_AFTER);
}

static void
disconnect_model_signals (BailTreeView *view) 
{
  GObject *obj;
  BtkWidget *widget;

  obj = G_OBJECT (view->tree_model);
  widget = BTK_ACCESSIBLE (view)->widget;
  g_signal_handlers_disconnect_by_func (obj, (gpointer) model_row_changed, widget);
  g_signal_handlers_disconnect_by_func (obj, (gpointer) model_row_inserted, widget);
  g_signal_handlers_disconnect_by_func (obj, (gpointer) model_row_deleted, widget);
  g_signal_handlers_disconnect_by_func (obj, (gpointer) model_rows_reordered, widget);
}

static void
clear_cached_data (BailTreeView  *view)
{
  GList *temp_list;

  if (view->row_data)
    {
      GArray *array = view->row_data;
      gint i;

     /*
      * Since the third argument to free_row_info is FALSE, we don't remove 
      * the element.  Therefore it is safe to loop forward.
      */
      for (i = 0; i < array->len; i++)
        free_row_info (array, i, FALSE);

      g_array_free (array, TRUE);

      view->row_data = NULL;
    }

  if (view->cell_data)
    {
      /* Must loop through them all */
      for (temp_list = view->cell_data; temp_list; temp_list = temp_list->next)
        {
	    clean_cell_info (view, temp_list);
        }
    }
  garbage_collect_cell_data (view);
  if (view->cell_data)
      g_list_free (view->cell_data);
  
  view->cell_data = NULL;
}

/*
 * Returns the column number of the specified BtkTreeViewColumn
 *
 * If visible is set, the value returned will be the visible column number, 
 * i.e. suitable for use in BatkTable function. If visible is not set, the
 * value returned is the actual column number, which is suitable for use in 
 * getting an index value.
 */
static gint
get_column_number (BtkTreeView       *tree_view,
                   BtkTreeViewColumn *column,
                   gboolean          visible)
{
  GList *temp_list, *column_list;
  BtkTreeViewColumn *tv_column;
  gint ret_val;

  column_list = btk_tree_view_get_columns (tree_view);
  ret_val = 0;
  for (temp_list = column_list; temp_list; temp_list = temp_list->next)
    {
      tv_column = BTK_TREE_VIEW_COLUMN (temp_list->data);
      if (tv_column == column)
        break;
      if (!visible || btk_tree_view_column_get_visible (tv_column))
        ret_val++;
    }
  if (temp_list == NULL)
    {
      ret_val = -1;
    }
  g_list_free (column_list);
  return ret_val;
} 

static gint
get_index (BtkTreeView       *tree_view,
           BtkTreePath       *path,
           gint              actual_column)
{
  gint depth = 0;
  gint index = 1;
  gint *indices = NULL;


  if (path)
    {
      depth = btk_tree_path_get_depth (path);
      indices = btk_tree_path_get_indices (path);
    }

  if (depth > 1)
    {
      BtkTreePath *copy_path;
      BtkTreeModel *model;

      model = btk_tree_view_get_model (tree_view);
      copy_path = btk_tree_path_copy (path);
      btk_tree_path_up (copy_path);
      count_rows (model, NULL, copy_path, &index, 0, depth);
      btk_tree_path_free (copy_path);
    }

  if (path)
    index += indices[depth-1];
  index *= get_n_actual_columns (tree_view);
  index +=  actual_column;
  return index;
}

/*
 * The function count_rows counts the number of rows starting at iter and ending
 * at end_path. The value of level is the depth of iter and the value of depth
 * is the depth of end_path. Rows at depth before end_path are counted.
 * This functions counts rows which are not visible because an ancestor is 
 * collapsed.
 */
static void 
count_rows (BtkTreeModel *model,
            BtkTreeIter *iter,
            BtkTreePath *end_path,
            gint        *count,
            gint        level,
            gint        depth)
{
  BtkTreeIter child_iter;
  
  if (!model) return;

  level++;

  *count += btk_tree_model_iter_n_children (model, iter);

#if 0
  g_print ("count_rows : %d level: %d depth: %d\n", *count, level, depth);
  if (iter != NULL)
    g_print ("path: %s\n",
            btk_tree_path_to_string (btk_tree_model_get_path (model, iter)));
#endif

  if (level >= depth)
    return;

  if (btk_tree_model_iter_children (model, &child_iter, iter))
    {
      gboolean ret_val = TRUE;

      while (ret_val)
        {
          if (level == depth - 1)
            {
              BtkTreePath *iter_path; 
              gboolean finished = FALSE;

              iter_path = btk_tree_model_get_path (model, &child_iter);
              if (end_path && btk_tree_path_compare (iter_path, end_path) >= 0)
                finished = TRUE;
              btk_tree_path_free (iter_path);
              if (finished)
                break;
            }
          if (btk_tree_model_iter_has_child (model, &child_iter))
            count_rows (model, &child_iter, end_path, count, level, depth);
          ret_val = btk_tree_model_iter_next (model, &child_iter);
        }
    }
}

/*
 * Find the next node, which has children, at the specified depth below
 * the specified iter. The level is the depth of the current iter.
 * The position of the node is returned in path and the return value of TRUE 
 * means that a node was found.
 */

gboolean get_next_node_with_child_at_depth (BtkTreeModel *model,
                                            BtkTreeIter  *iter,
                                            BtkTreePath  **path,
                                            gint         level,
                                            gint         depth)
{
  BtkTreeIter child_iter;

  *path = NULL;

  if (btk_tree_model_iter_children (model, &child_iter, iter))
    {
      level++;

      while (TRUE)
        {
          while (!btk_tree_model_iter_has_child (model, &child_iter))
            {
              if (!btk_tree_model_iter_next (model, &child_iter))
                return FALSE;
            }

          if (level == depth)
          /* We have found what we were looking for */
            {
              *path = btk_tree_model_get_path (model, &child_iter);
              return TRUE;
            }

          if (get_next_node_with_child_at_depth (model, &child_iter, path,
                                                 level, depth))
            return TRUE;

          if (!btk_tree_model_iter_next (model, &child_iter))
            return FALSE;
        }
    }
  return FALSE;
}

/*
 * Find the next node, which has children, at the same depth as 
 * the specified BtkTreePath.
 */
static gboolean 
get_next_node_with_child (BtkTreeModel *model,
                          BtkTreePath  *path,
                          BtkTreePath  **return_path)
{
  BtkTreeIter iter;
  gint depth;

  btk_tree_model_get_iter (model, &iter, path);

  while (btk_tree_model_iter_next (model, &iter))
    {
      if (btk_tree_model_iter_has_child (model, &iter))
        {
          *return_path = btk_tree_model_get_path (model, &iter);
          return TRUE;
        }
    }
  depth = btk_tree_path_get_depth (path);
  while (btk_tree_path_up (path))
    {
      if (btk_tree_path_get_depth (path) == 0)
        break;

      btk_tree_model_get_iter (model, &iter, path);
      while (btk_tree_model_iter_next (model, &iter))
        if (get_next_node_with_child_at_depth (model, &iter, return_path,
                                         btk_tree_path_get_depth (path), depth))
          return TRUE;
    }
  *return_path = NULL;
  return FALSE;
}

static gboolean 
get_tree_path_from_row_index (BtkTreeModel *model,
                              gint         row_index,
                              BtkTreePath  **tree_path)
{
  BtkTreeIter iter;
  gint count;
  gint depth;

  count = btk_tree_model_iter_n_children (model, NULL);
  if (count > row_index)
    {
      if (btk_tree_model_iter_nth_child (model, &iter, NULL, row_index))
        {
          *tree_path = btk_tree_model_get_path (model, &iter);
          return TRUE;
        }
      else
        return FALSE;
    }
  else
     row_index -= count;

  depth = 0;
  while (TRUE)
    {
      depth++;

      if (get_next_node_with_child_at_depth (model, NULL, tree_path, 0, depth))
        {
          BtkTreePath *next_path;

          while (TRUE)
            {
              btk_tree_model_get_iter (model, &iter, *tree_path);
              count = btk_tree_model_iter_n_children (model, &iter);
              if (count > row_index)
                {
                  btk_tree_path_append_index (*tree_path, row_index);
                  return TRUE;
                }
              else
                row_index -= count;

              if (!get_next_node_with_child (model,  *tree_path, &next_path))
                break;
           
              btk_tree_path_free (*tree_path);
              *tree_path = next_path;
            }
        }
      else
        {
          g_warning ("Index value is too large\n");
          btk_tree_path_free (*tree_path);
           *tree_path = NULL;
          return FALSE;
        }
    }  
}

/*
 * This function returns the number of rows, including those which are collapsed
 */
static gint
get_row_count (BtkTreeModel *model)
{
  gint n_rows = 1;

  count_rows (model, NULL, NULL, &n_rows, 0, G_MAXINT);

  return n_rows;
}

static gboolean
get_path_column_from_index (BtkTreeView       *tree_view,
                            gint              index,
                            BtkTreePath       **path,
                            BtkTreeViewColumn **column)
{
  BtkTreeModel *tree_model;
  gint n_columns;

  tree_model = btk_tree_view_get_model (tree_view);
  n_columns = get_n_actual_columns (tree_view);
  if (n_columns == 0)
    return FALSE;
  /* First row is the column headers */
  index -= n_columns;
  if (index < 0)
    return FALSE;

  if (path)
    {
      gint row_index;
      gboolean retval;

      row_index = index / n_columns;
      retval = get_tree_path_from_row_index (tree_model, row_index, path);
      bail_return_val_if_fail (retval, FALSE);
      if (*path == NULL)
        return FALSE;
    }    

  if (column)
    {
      *column = btk_tree_view_get_column (tree_view, index % n_columns);
      if (*column == NULL)
        {
	  if (path)
            btk_tree_path_free (*path);
          return FALSE;
        }
  }
  return TRUE;
}

static void
set_cell_expandable (BailCell *cell)
{
  if (bail_cell_add_state (cell, 
                           BATK_STATE_EXPANDABLE,
                           FALSE))
    bail_cell_add_action (cell,
                          "expand or contract", /* action name */
                          "expands or contracts the row in the tree view "
                          "containing this cell", /* description */
                          NULL, /* Keybinding */
                          toggle_cell_expanded);
}

static BailTreeViewCellInfo*
find_cell_info (BailTreeView *view,
                BailCell     *cell,
                GList**      list,
		gboolean     live_only)
{
  GList *temp_list;
  BailTreeViewCellInfo *cell_info;

  for (temp_list = view->cell_data; temp_list; temp_list = temp_list->next)
    {
      cell_info = (BailTreeViewCellInfo *) temp_list->data;
      if (cell_info->cell == cell && (!live_only || cell_info->in_use))
        {
          if (list)
            *list = temp_list;
          return cell_info;
        }
    }
  return NULL;
}

static BatkObject *
get_header_from_column (BtkTreeViewColumn *tv_col)
{
  BatkObject *rc;
  BtkWidget *header_widget;

  if (tv_col == NULL)
    return NULL;

  /* If the user has set a header object, use that */

  rc = g_object_get_qdata (G_OBJECT (tv_col), quark_column_header_object);

  if (rc == NULL)
    {
      /* If the user has not set a header object, grab the column */
      /* header object defined by the BtkTreeView */

      header_widget = tv_col->button;

      if (header_widget)
        {
          rc = btk_widget_get_accessible (header_widget);
        }
      else
        rc = NULL;
    }
  return rc;
}
