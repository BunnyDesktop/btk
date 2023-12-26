/* Tree View/Editable Cells
 *
 * This demo demonstrates the use of editable cells in a BtkTreeView. If
 * you're new to the BtkTreeView widgets and associates, look into
 * the BtkListStore example first. It also shows how to use the
 * BtkCellRenderer::editing-started signal to do custom setup of the
 * editable widget.
 *
 * The cell renderers used in this demo are BtkCellRendererText, 
 * BtkCellRendererCombo and BtkCellRendererProgress.
 */

#include <btk/btk.h>
#include <string.h>
#include <stdlib.h>

static BtkWidget *window = NULL;

typedef struct
{
  bint   number;
  bchar *product;
  bint   yummy;
}
Item;

enum
{
  COLUMN_ITEM_NUMBER,
  COLUMN_ITEM_PRODUCT,
  COLUMN_ITEM_YUMMY,
  NUM_ITEM_COLUMNS
};

enum
{
  COLUMN_NUMBER_TEXT,
  NUM_NUMBER_COLUMNS
};

static GArray *articles = NULL;

static void
add_items (void)
{
  Item foo;

  g_return_if_fail (articles != NULL);

  foo.number = 3;
  foo.product = "bottles of coke";
  foo.yummy = 20;
  g_array_append_vals (articles, &foo, 1);

  foo.number = 5;
  foo.product = "packages of noodles";
  foo.yummy = 50;
  g_array_append_vals (articles, &foo, 1);

  foo.number = 2;
  foo.product = "packages of chocolate chip cookies";
  foo.yummy = 90;
  g_array_append_vals (articles, &foo, 1);

  foo.number = 1;
  foo.product = "can vanilla ice cream";
  foo.yummy = 60;
  g_array_append_vals (articles, &foo, 1);

  foo.number = 6;
  foo.product = "eggs";
  foo.yummy = 10;
  g_array_append_vals (articles, &foo, 1);
}

static BtkTreeModel *
create_items_model (void)
{
  bint i = 0;
  BtkListStore *model;
  BtkTreeIter iter;

  /* create array */
  articles = g_array_sized_new (FALSE, FALSE, sizeof (Item), 1);

  add_items ();

  /* create list store */
  model = btk_list_store_new (NUM_ITEM_COLUMNS, B_TYPE_INT, B_TYPE_STRING,
                              B_TYPE_INT, B_TYPE_BOOLEAN);

  /* add items */
  for (i = 0; i < articles->len; i++)
    {
      btk_list_store_append (model, &iter);

      btk_list_store_set (model, &iter,
                          COLUMN_ITEM_NUMBER,
                          g_array_index (articles, Item, i).number,
                          COLUMN_ITEM_PRODUCT,
                          g_array_index (articles, Item, i).product,
                          COLUMN_ITEM_YUMMY,
                          g_array_index (articles, Item, i).yummy,
                          -1);
    }

  return BTK_TREE_MODEL (model);
}

static BtkTreeModel *
create_numbers_model (void)
{
#define N_NUMBERS 10
  bint i = 0;
  BtkListStore *model;
  BtkTreeIter iter;

  /* create list store */
  model = btk_list_store_new (NUM_NUMBER_COLUMNS, B_TYPE_STRING, B_TYPE_INT);

  /* add numbers */
  for (i = 0; i < N_NUMBERS; i++)
    {
      char str[2];

      str[0] = '0' + i;
      str[1] = '\0';

      btk_list_store_append (model, &iter);

      btk_list_store_set (model, &iter,
                          COLUMN_NUMBER_TEXT, str,
                          -1);
    }

  return BTK_TREE_MODEL (model);

#undef N_NUMBERS
}

static void
add_item (BtkWidget *button, bpointer data)
{
  Item foo;
  BtkTreeIter iter;
  BtkTreeModel *model = (BtkTreeModel *)data;

  g_return_if_fail (articles != NULL);

  foo.number = 0;
  foo.product = g_strdup ("Description here");
  foo.yummy = 50;
  g_array_append_vals (articles, &foo, 1);

  btk_list_store_append (BTK_LIST_STORE (model), &iter);
  btk_list_store_set (BTK_LIST_STORE (model), &iter,
                      COLUMN_ITEM_NUMBER, foo.number,
                      COLUMN_ITEM_PRODUCT, foo.product,
                      COLUMN_ITEM_YUMMY, foo.yummy,
                      -1);
}

static void
remove_item (BtkWidget *widget, bpointer data)
{
  BtkTreeIter iter;
  BtkTreeView *treeview = (BtkTreeView *)data;
  BtkTreeModel *model = btk_tree_view_get_model (treeview);
  BtkTreeSelection *selection = btk_tree_view_get_selection (treeview);

  if (btk_tree_selection_get_selected (selection, NULL, &iter))
    {
      bint i;
      BtkTreePath *path;

      path = btk_tree_model_get_path (model, &iter);
      i = btk_tree_path_get_indices (path)[0];
      btk_list_store_remove (BTK_LIST_STORE (model), &iter);

      g_array_remove_index (articles, i);

      btk_tree_path_free (path);
    }
}

static bboolean
separator_row (BtkTreeModel *model,
               BtkTreeIter  *iter,
               bpointer      data)
{
  BtkTreePath *path;
  bint idx;

  path = btk_tree_model_get_path (model, iter);
  idx = btk_tree_path_get_indices (path)[0];

  btk_tree_path_free (path);

  return idx == 5;
}

static void
editing_started (BtkCellRenderer *cell,
                 BtkCellEditable *editable,
                 const bchar     *path,
                 bpointer         data)
{
  btk_combo_box_set_row_separator_func (BTK_COMBO_BOX (editable), 
                                        separator_row, NULL, NULL);
}

static void
cell_edited (BtkCellRendererText *cell,
             const bchar         *path_string,
             const bchar         *new_text,
             bpointer             data)
{
  BtkTreeModel *model = (BtkTreeModel *)data;
  BtkTreePath *path = btk_tree_path_new_from_string (path_string);
  BtkTreeIter iter;

  bint column = BPOINTER_TO_INT (g_object_get_data (B_OBJECT (cell), "column"));

  btk_tree_model_get_iter (model, &iter, path);

  switch (column)
    {
    case COLUMN_ITEM_NUMBER:
      {
        bint i;

        i = btk_tree_path_get_indices (path)[0];
        g_array_index (articles, Item, i).number = atoi (new_text);

        btk_list_store_set (BTK_LIST_STORE (model), &iter, column,
                            g_array_index (articles, Item, i).number, -1);
      }
      break;

    case COLUMN_ITEM_PRODUCT:
      {
        bint i;
        bchar *old_text;

        btk_tree_model_get (model, &iter, column, &old_text, -1);
        g_free (old_text);

        i = btk_tree_path_get_indices (path)[0];
        g_free (g_array_index (articles, Item, i).product);
        g_array_index (articles, Item, i).product = g_strdup (new_text);

        btk_list_store_set (BTK_LIST_STORE (model), &iter, column,
                            g_array_index (articles, Item, i).product, -1);
      }
      break;
    }

  btk_tree_path_free (path);
}

static void
add_columns (BtkTreeView  *treeview, 
             BtkTreeModel *items_model,
             BtkTreeModel *numbers_model)
{
  BtkCellRenderer *renderer;

  /* number column */
  renderer = btk_cell_renderer_combo_new ();
  g_object_set (renderer,
                "model", numbers_model,
                "text-column", COLUMN_NUMBER_TEXT,
                "has-entry", FALSE,
                "editable", TRUE,
                NULL);
  g_signal_connect (renderer, "edited",
                    G_CALLBACK (cell_edited), items_model);
  g_signal_connect (renderer, "editing-started",
                    G_CALLBACK (editing_started), NULL);
  g_object_set_data (B_OBJECT (renderer), "column", BINT_TO_POINTER (COLUMN_ITEM_NUMBER));

  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (treeview),
                                               -1, "Number", renderer,
                                               "text", COLUMN_ITEM_NUMBER,
                                               NULL);

  /* product column */
  renderer = btk_cell_renderer_text_new ();
  g_object_set (renderer,
                "editable", TRUE,
                NULL);
  g_signal_connect (renderer, "edited",
                    G_CALLBACK (cell_edited), items_model);
  g_object_set_data (B_OBJECT (renderer), "column", BINT_TO_POINTER (COLUMN_ITEM_PRODUCT));

  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (treeview),
                                               -1, "Product", renderer,
                                               "text", COLUMN_ITEM_PRODUCT,
                                               NULL);

  /* yummy column */
  renderer = btk_cell_renderer_progress_new ();
  g_object_set_data (B_OBJECT (renderer), "column", BINT_TO_POINTER (COLUMN_ITEM_YUMMY));

  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (treeview),
                                               -1, "Yummy", renderer,
                                               "value", COLUMN_ITEM_YUMMY,
                                               NULL);
  

}

BtkWidget *
do_editable_cells (BtkWidget *do_widget)
{
  if (!window)
    {
      BtkWidget *vbox;
      BtkWidget *hbox;
      BtkWidget *sw;
      BtkWidget *treeview;
      BtkWidget *button;
      BtkTreeModel *items_model;
      BtkTreeModel *numbers_model;

      /* create window, etc */
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
                             btk_widget_get_screen (do_widget));
      btk_window_set_title (BTK_WINDOW (window), "Shopping list");
      btk_container_set_border_width (BTK_CONTAINER (window), 5);
      g_signal_connect (window, "destroy",
                        G_CALLBACK (btk_widget_destroyed), &window);

      vbox = btk_vbox_new (FALSE, 5);
      btk_container_add (BTK_CONTAINER (window), vbox);

      btk_box_pack_start (BTK_BOX (vbox),
                          btk_label_new ("Shopping list (you can edit the cells!)"),
                          FALSE, FALSE, 0);

      sw = btk_scrolled_window_new (NULL, NULL);
      btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (sw),
                                           BTK_SHADOW_ETCHED_IN);
      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (sw),
                                      BTK_POLICY_AUTOMATIC,
                                      BTK_POLICY_AUTOMATIC);
      btk_box_pack_start (BTK_BOX (vbox), sw, TRUE, TRUE, 0);

      /* create models */
      items_model = create_items_model ();
      numbers_model = create_numbers_model ();

      /* create tree view */
      treeview = btk_tree_view_new_with_model (items_model);
      btk_tree_view_set_rules_hint (BTK_TREE_VIEW (treeview), TRUE);
      btk_tree_selection_set_mode (btk_tree_view_get_selection (BTK_TREE_VIEW (treeview)),
                                   BTK_SELECTION_SINGLE);

      add_columns (BTK_TREE_VIEW (treeview), items_model, numbers_model);

      g_object_unref (numbers_model);
      g_object_unref (items_model);

      btk_container_add (BTK_CONTAINER (sw), treeview);

      /* some buttons */
      hbox = btk_hbox_new (TRUE, 4);
      btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, FALSE, 0);

      button = btk_button_new_with_label ("Add item");
      g_signal_connect (button, "clicked",
                        G_CALLBACK (add_item), items_model);
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);

      button = btk_button_new_with_label ("Remove item");
      g_signal_connect (button, "clicked",
                        G_CALLBACK (remove_item), treeview);
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);

      btk_window_set_default_size (BTK_WINDOW (window), 320, 200);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    {
      btk_widget_destroy (window);
      window = NULL;
    }

  return window;
}
