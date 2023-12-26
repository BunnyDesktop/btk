#include <btk/btk.h>


static BtkTreeModel *
create_model (void)
{
  BtkTreeStore *store;
  BtkTreeIter iter;
  BtkTreeIter parent;

  store = btk_tree_store_new (1, G_TYPE_STRING);

  btk_tree_store_insert_with_values (store, &parent, NULL, 0,
				     0, "Applications", -1);

  btk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "File Manager", -1);
  btk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "Gossip", -1);
  btk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "System Settings", -1);
  btk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "The GIMP", -1);
  btk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "Terminal", -1);
  btk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "Word Processor", -1);


  btk_tree_store_insert_with_values (store, &parent, NULL, 1,
				     0, "Documents", -1);

  btk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "blaat.txt", -1);
  btk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "sliff.txt", -1);
  btk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "test.txt", -1);
  btk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "blaat.txt", -1);
  btk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "brrrr.txt", -1);
  btk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "hohoho.txt", -1);


  btk_tree_store_insert_with_values (store, &parent, NULL, 2,
				     0, "Images", -1);

  btk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "image1.png", -1);
  btk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "image2.png", -1);
  btk_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "image3.jpg", -1);

  return BTK_TREE_MODEL (store);
}

static void
set_color_func (BtkTreeViewColumn *column,
		BtkCellRenderer   *cell,
		BtkTreeModel      *model,
		BtkTreeIter       *iter,
		gpointer           data)
{
  if (btk_tree_model_iter_has_child (model, iter))
    g_object_set (cell, "cell-background", "Grey", NULL);
  else
    g_object_set (cell, "cell-background", NULL, NULL);
}

static void
tree_view_row_activated (BtkTreeView       *tree_view,
			 BtkTreePath       *path,
			 BtkTreeViewColumn *column)
{
  if (btk_tree_path_get_depth (path) > 1)
    return;

  if (btk_tree_view_row_expanded (BTK_TREE_VIEW (tree_view), path))
    btk_tree_view_collapse_row (BTK_TREE_VIEW (tree_view), path);
  else
    btk_tree_view_expand_row (BTK_TREE_VIEW (tree_view), path, FALSE);
}

static gboolean
tree_view_select_func (BtkTreeSelection *selection,
		       BtkTreeModel     *model,
		       BtkTreePath      *path,
		       gboolean          path_currently_selected,
		       gpointer          data)
{
  if (btk_tree_path_get_depth (path) > 1)
    return TRUE;

  return FALSE;
}

int
main (int argc, char **argv)
{
  BtkWidget *window, *sw, *tv;
  BtkTreeModel *model;
  BtkCellRenderer *renderer;
  BtkTreeViewColumn *column;

  btk_init (&argc, &argv);

  model = create_model ();

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "delete_event",
		    G_CALLBACK (btk_main_quit), NULL);
  btk_window_set_default_size (BTK_WINDOW (window), 320, 480);

  sw = btk_scrolled_window_new (NULL, NULL);
  btk_container_add (BTK_CONTAINER (window), sw);

  tv = btk_tree_view_new_with_model (model);
  btk_container_add (BTK_CONTAINER (sw), tv);

  g_signal_connect (tv, "row-activated",
		    G_CALLBACK (tree_view_row_activated), tv);
  g_object_set (tv,
		"show-expanders", FALSE,
		"level-indentation", 10,
		NULL);

  btk_tree_view_set_headers_visible (BTK_TREE_VIEW (tv), FALSE);
  btk_tree_view_expand_all (BTK_TREE_VIEW (tv));

  btk_tree_selection_set_select_function (btk_tree_view_get_selection (BTK_TREE_VIEW (tv)),
					  tree_view_select_func,
					  NULL,
					  NULL);

  renderer = btk_cell_renderer_text_new ();
  column = btk_tree_view_column_new_with_attributes ("(none)",
						     renderer,
						     "text", 0,
						     NULL);
  btk_tree_view_column_set_cell_data_func (column,
					   renderer,
					   set_color_func,
					   NULL,
					   NULL);
  btk_tree_view_insert_column (BTK_TREE_VIEW (tv), column, 0);

  btk_widget_show_all (window);

  btk_main ();

  return 0;
}
