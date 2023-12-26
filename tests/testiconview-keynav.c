/* testiconview-keynav.c
 * Copyright (C) 2010  Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Matthias Clasen
 */

/*
 * This example demonstrates how to use the keynav-failed signal to
 * extend arrow keynav over adjacent icon views. This can be used when
 * grouping items.
 */

#include <btk/btk.h>

static BtkTreeModel *
get_model (void)
{
  static BtkListStore *store;
  BtkTreeIter iter;

  if (store)
    return (BtkTreeModel *) g_object_ref (store);

  store = btk_list_store_new (1, G_TYPE_STRING);

  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "One", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "Two", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "Three", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "Four", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "Five", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "Six", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "Seven", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "Eight", -1);

  return (BtkTreeModel *) store;
}

static gboolean
visible_func (BtkTreeModel *model,
              BtkTreeIter  *iter,
              gpointer      data)
{
  gboolean first = GPOINTER_TO_INT (data);
  gboolean visible;
  BtkTreePath *path;

  path = btk_tree_model_get_path (model, iter);

  if (btk_tree_path_get_indices (path)[0] < 4)
    visible = first;
  else
    visible = !first;

  btk_tree_path_free (path);

  return visible;
}

BtkTreeModel *
get_filter_model (gboolean first)
{
  BtkTreeModelFilter *model;

  model = (BtkTreeModelFilter *)btk_tree_model_filter_new (get_model (), NULL);

  btk_tree_model_filter_set_visible_func (model, visible_func, GINT_TO_POINTER (first), NULL);

  return (BtkTreeModel *) model;
}

static BtkWidget *
get_view (gboolean first)
{
  BtkWidget *view;

  view = btk_icon_view_new_with_model (get_filter_model (first));
  btk_icon_view_set_text_column (BTK_ICON_VIEW (view), 0);
  btk_widget_set_size_request (view, 0, -1);

  return view;
}

typedef struct
{
  BtkWidget *header1;
  BtkWidget *view1;
  BtkWidget *header2;
  BtkWidget *view2;
} Views;

static gboolean
keynav_failed (BtkWidget        *view,
               BtkDirectionType  direction,
               Views            *views)
{
  BtkTreePath *path;
  BtkTreeModel *model;
  BtkTreeIter iter;
  gint col;
  BtkTreePath *sel;

  if (view == views->view1 && direction == BTK_DIR_DOWN)
    {
      if (btk_icon_view_get_cursor (BTK_ICON_VIEW (views->view1), &path, NULL))
        {
          col = btk_icon_view_get_item_column (BTK_ICON_VIEW (views->view1), path);
          btk_tree_path_free (path);

          sel = NULL;
          model = btk_icon_view_get_model (BTK_ICON_VIEW (views->view2));
          btk_tree_model_get_iter_first (model, &iter);
          do {
            path = btk_tree_model_get_path (model, &iter);
            if (btk_icon_view_get_item_column (BTK_ICON_VIEW (views->view2), path) == col)
              {
                sel = path;
                break;
              }
          } while (btk_tree_model_iter_next (model, &iter));

          btk_icon_view_set_cursor (BTK_ICON_VIEW (views->view2), sel, NULL, FALSE);
          btk_tree_path_free (sel);
        }
      btk_widget_grab_focus (views->view2);
      return TRUE;
    }

  if (view == views->view2 && direction == BTK_DIR_UP)
    {
      if (btk_icon_view_get_cursor (BTK_ICON_VIEW (views->view2), &path, NULL))
        {
          col = btk_icon_view_get_item_column (BTK_ICON_VIEW (views->view2), path);
          btk_tree_path_free (path);

          sel = NULL;
          model = btk_icon_view_get_model (BTK_ICON_VIEW (views->view1));
          btk_tree_model_get_iter_first (model, &iter);
          do {
            path = btk_tree_model_get_path (model, &iter);
            if (btk_icon_view_get_item_column (BTK_ICON_VIEW (views->view1), path) == col)
              {
                if (sel)
                  btk_tree_path_free (sel);
                sel = path;
              }
            else
              btk_tree_path_free (path);
          } while (btk_tree_model_iter_next (model, &iter));

          btk_icon_view_set_cursor (BTK_ICON_VIEW (views->view1), sel, NULL, FALSE);
          btk_tree_path_free (sel);
        }
      btk_widget_grab_focus (views->view1);
      return TRUE;
    }

  return FALSE;
}

static gboolean
focus_out (BtkWidget     *view,
           BdkEventFocus *event,
           gpointer       data)
{
  btk_icon_view_unselect_all (BTK_ICON_VIEW (view));

  return FALSE;
}

static gboolean
focus_in (BtkWidget     *view,
          BdkEventFocus *event,
          gpointer       data)
{
  BtkTreePath *path;

  if (!btk_icon_view_get_cursor (BTK_ICON_VIEW (view), &path, NULL))
    {
      path = btk_tree_path_new_from_indices (0, -1);
      btk_icon_view_set_cursor (BTK_ICON_VIEW (view), path, NULL, FALSE);
    }

  btk_icon_view_select_path (BTK_ICON_VIEW (view), path);
  btk_tree_path_free (path);

  return FALSE;
}

static void
header_style_set (BtkWidget *widget,
                  BtkStyle  *old_style)
{
  g_signal_handlers_block_by_func (widget, header_style_set, NULL);
  btk_widget_modify_bg (widget, BTK_STATE_NORMAL,
                        &widget->style->base[BTK_STATE_NORMAL]);
  btk_widget_modify_fg (widget, BTK_STATE_NORMAL,
                        &widget->style->text[BTK_STATE_NORMAL]);
  g_signal_handlers_unblock_by_func (widget, header_style_set, NULL);
}

int
main (int argc, char *argv[])
{
  BtkWidget *window;
  BtkWidget *vbox;
  Views views;

  btk_init (&argc, &argv);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  vbox = btk_vbox_new (FALSE, 0);
  btk_container_add (BTK_CONTAINER (window), vbox);

  views.header1 = g_object_new (BTK_TYPE_LABEL,
                                "label", "<b>Group 1</b>",
                                "use-markup", TRUE,
                                "xalign", 0.0,
                                NULL);
  views.view1 = get_view (TRUE);
  views.header2 = g_object_new (BTK_TYPE_LABEL,
                                "label", "<b>Group 2</b>",
                                "use-markup", TRUE,
                                "xalign", 0.0,
                                NULL);
  views.view2 = get_view (FALSE);

  g_signal_connect (views.view1, "keynav-failed",
                    G_CALLBACK (keynav_failed), &views);
  g_signal_connect (views.view2, "keynav-failed",
                    G_CALLBACK (keynav_failed), &views);
  g_signal_connect (views.view1, "focus-in-event",
                    G_CALLBACK (focus_in), NULL);
  g_signal_connect (views.view1, "focus-out-event",
                    G_CALLBACK (focus_out), NULL);
  g_signal_connect (views.view2, "focus-in-event",
                    G_CALLBACK (focus_in), NULL);
  g_signal_connect (views.view2, "focus-out-event",
                    G_CALLBACK (focus_out), NULL);
  g_signal_connect (views.header1, "style-set",
                    G_CALLBACK (header_style_set), NULL);
  g_signal_connect (views.header2, "style-set",
                    G_CALLBACK (header_style_set), NULL);
  g_signal_connect (window, "style-set",
                    G_CALLBACK (header_style_set), NULL);

  btk_container_add (BTK_CONTAINER (vbox), views.header1);
  btk_container_add (BTK_CONTAINER (vbox), views.view1);
  btk_container_add (BTK_CONTAINER (vbox), views.header2);
  btk_container_add (BTK_CONTAINER (vbox), views.view2);

  btk_widget_show_all (window);

  btk_main ();

  return 0;
}

