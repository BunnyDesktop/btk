/* Basic BtkTreeView unit tests.
 * Copyright (C) 2009  Kristian Rietveld  <kris@btk.org>
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

#include <btk/btk.h>

static void
test_bug_546005 (void)
{
  BtkTreeIter iter;
  BtkTreePath *path;
  BtkTreePath *cursor_path;
  BtkListStore *list_store;
  BtkWidget *view;

  /* Tests provided by Bjorn Lindqvist, Paul Pogonyshev */
  view = btk_tree_view_new ();

  /* Invalid path on tree view without model */
  path = btk_tree_path_new_from_indices (1, -1);
  btk_tree_view_set_cursor (BTK_TREE_VIEW (view), path,
                            NULL, FALSE);
  btk_tree_path_free (path);

  list_store = btk_list_store_new (1, G_TYPE_STRING);
  btk_tree_view_set_model (BTK_TREE_VIEW (view),
                           BTK_TREE_MODEL (list_store));

  /* Invalid path on tree view with empty model */
  path = btk_tree_path_new_from_indices (1, -1);
  btk_tree_view_set_cursor (BTK_TREE_VIEW (view), path,
                            NULL, FALSE);
  btk_tree_path_free (path);

  /* Valid path */
  btk_list_store_insert_with_values (list_store, &iter, 0,
                                     0, "hi",
                                     -1);

  path = btk_tree_path_new_from_indices (0, -1);
  btk_tree_view_set_cursor (BTK_TREE_VIEW (view), path,
                            NULL, FALSE);

  btk_tree_view_get_cursor (BTK_TREE_VIEW (view), &cursor_path, NULL);
  //btk_assert_cmptreepath (cursor_path, ==, path);

  btk_tree_path_free (path);
  btk_tree_path_free (cursor_path);

  /* Invalid path on tree view with model */
  path = btk_tree_path_new_from_indices (1, -1);
  btk_tree_view_set_cursor (BTK_TREE_VIEW (view), path,
                            NULL, FALSE);
  btk_tree_path_free (path);
}

static void
test_bug_539377 (void)
{
  BtkWidget *view;
  BtkTreePath *path;
  BtkListStore *list_store;

  /* Test provided by Bjorn Lindqvist */

  /* Non-realized view, no model */
  view = btk_tree_view_new ();
  g_assert (btk_tree_view_get_path_at_pos (BTK_TREE_VIEW (view), 10, 10, &path,
                                           NULL, NULL, NULL) == FALSE);
  g_assert (btk_tree_view_get_dest_row_at_pos (BTK_TREE_VIEW (view), 10, 10,
                                               &path, NULL) == FALSE);

  /* Non-realized view, with model */
  list_store = btk_list_store_new (1, G_TYPE_STRING);
  btk_tree_view_set_model (BTK_TREE_VIEW (view),
                           BTK_TREE_MODEL (list_store));

  g_assert (btk_tree_view_get_path_at_pos (BTK_TREE_VIEW (view), 10, 10, &path,
                                           NULL, NULL, NULL) == FALSE);
  g_assert (btk_tree_view_get_dest_row_at_pos (BTK_TREE_VIEW (view), 10, 10,
                                               &path, NULL) == FALSE);
}

static void
test_select_collapsed_row (void)
{
  BtkTreeIter child, parent;
  BtkTreePath *path;
  BtkTreeStore *tree_store;
  BtkTreeSelection *selection;
  BtkWidget *view;

  /* Reported by Michael Natterer */
  tree_store = btk_tree_store_new (1, G_TYPE_STRING);
  view = btk_tree_view_new_with_model (BTK_TREE_MODEL (tree_store));

  btk_tree_store_insert_with_values (tree_store, &parent, NULL, 0,
                                     0, "Parent",
                                     -1);

  btk_tree_store_insert_with_values (tree_store, &child, &parent, 0,
                                     0, "Child",
                                     -1);
  btk_tree_store_insert_with_values (tree_store, &child, &parent, 0,
                                     0, "Child",
                                     -1);


  /* Try to select a child path. */
  path = btk_tree_path_new_from_indices (0, 1, -1);
  btk_tree_view_set_cursor (BTK_TREE_VIEW (view), path, NULL, FALSE);

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (view));

  /* Check that the parent is not selected. */
  btk_tree_path_up (path);
  g_return_if_fail (btk_tree_selection_path_is_selected (selection, path) == FALSE);

  /* Nothing should be selected at this point. */
  g_return_if_fail (btk_tree_selection_count_selected_rows (selection) == 0);

  /* Check that selection really still works. */
  btk_tree_view_set_cursor (BTK_TREE_VIEW (view), path, NULL, FALSE);
  g_return_if_fail (btk_tree_selection_path_is_selected (selection, path) == TRUE);
  g_return_if_fail (btk_tree_selection_count_selected_rows (selection) == 1);

  /* Expand and select child node now. */
  btk_tree_path_append_index (path, 1);
  btk_tree_view_expand_all (BTK_TREE_VIEW (view));

  btk_tree_view_set_cursor (BTK_TREE_VIEW (view), path, NULL, FALSE);
  g_return_if_fail (btk_tree_selection_path_is_selected (selection, path) == TRUE);
  g_return_if_fail (btk_tree_selection_count_selected_rows (selection) == 1);

  btk_tree_path_free (path);
}

int
main (int    argc,
      char **argv)
{
  btk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/TreeView/cursor/bug-546005", test_bug_546005);
  g_test_add_func ("/TreeView/cursor/bug-539377", test_bug_539377);
  g_test_add_func ("/TreeView/cursor/select-collapsed_row",
                   test_select_collapsed_row);

  return g_test_run ();
}
