/* testtreeflow.c
 * Copyright (C) 2001 Red Hat, Inc
 * Author: Jonathan Blandford
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
 */

#include "config.h"
#include <btk/btk.h>

BtkTreeModel *model = NULL;
static GRand *grand = NULL;
BtkTreeSelection *selection = NULL;
enum
{
  TEXT_COLUMN,
  NUM_COLUMNS
};

static char *words[] =
{
  "Boom",
  "Borp",
  "Multiline\ntext",
  "Bingo",
  "Veni\nVedi\nVici",
  NULL
};


#define NUM_WORDS 5
#define NUM_ROWS 100


static void
initialize_model (void)
{
  gint i;
  BtkTreeIter iter;

  model = (BtkTreeModel *) btk_list_store_new (NUM_COLUMNS, B_TYPE_STRING);
  grand = g_rand_new ();
  for (i = 0; i < NUM_ROWS; i++)
    {
      btk_list_store_append (BTK_LIST_STORE (model), &iter);
      btk_list_store_set (BTK_LIST_STORE (model), &iter,
			  TEXT_COLUMN, words[g_rand_int_range (grand, 0, NUM_WORDS)],
			  -1);
    }
}

static void
futz_row (void)
{
  gint i;
  BtkTreePath *path;
  BtkTreeIter iter;
  BtkTreeIter iter2;

  i = g_rand_int_range (grand, 0,
			btk_tree_model_iter_n_children (model, NULL));
  path = btk_tree_path_new ();
  btk_tree_path_append_index (path, i);
  btk_tree_model_get_iter (model, &iter, path);
  btk_tree_path_free (path);

  if (btk_tree_selection_iter_is_selected (selection, &iter))
    return;
  switch (g_rand_int_range (grand, 0, 3))
    {
    case 0:
      /* insert */
            btk_list_store_insert_after (BTK_LIST_STORE (model),
            				   &iter2, &iter);
            btk_list_store_set (BTK_LIST_STORE (model), &iter2,
            			  TEXT_COLUMN, words[g_rand_int_range (grand, 0, NUM_WORDS)],
            			  -1);
      break;
    case 1:
      /* delete */
      if (btk_tree_model_iter_n_children (model, NULL) == 0)
	return;
      btk_list_store_remove (BTK_LIST_STORE (model), &iter);
      break;
    case 2:
      /* modify */
      return;
      if (btk_tree_model_iter_n_children (model, NULL) == 0)
	return;
      btk_list_store_set (BTK_LIST_STORE (model), &iter,
      			  TEXT_COLUMN, words[g_rand_int_range (grand, 0, NUM_WORDS)],
      			  -1);
      break;
    }
}

static gboolean
futz (void)
{
  gint i;

  for (i = 0; i < 15; i++)
    futz_row ();
  g_print ("Number of rows: %d\n", btk_tree_model_iter_n_children (model, NULL));
  return TRUE;
}

int
main (int argc, char *argv[])
{
  BtkWidget *window;
  BtkWidget *vbox;
  BtkWidget *scrolled_window;
  BtkWidget *tree_view;
  BtkWidget *hbox;
  BtkWidget *button;
  BtkTreePath *path;

  btk_init (&argc, &argv);

  path = btk_tree_path_new_from_string ("80");
  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_title (BTK_WINDOW (window), "Reflow test");
  g_signal_connect (window, "destroy", btk_main_quit, NULL);
  vbox = btk_vbox_new (FALSE, 8);
  btk_container_set_border_width (BTK_CONTAINER (vbox), 8);
  btk_box_pack_start (BTK_BOX (vbox), btk_label_new ("Incremental Reflow Test"), FALSE, FALSE, 0);
  btk_container_add (BTK_CONTAINER (window), vbox);
  scrolled_window = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window),
				  BTK_POLICY_AUTOMATIC,
				  BTK_POLICY_AUTOMATIC);
  btk_box_pack_start (BTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
  
  initialize_model ();
  tree_view = btk_tree_view_new_with_model (model);
  btk_tree_view_scroll_to_cell (BTK_TREE_VIEW (tree_view), path, NULL, TRUE, 0.5, 0.0);
  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (tree_view));
  btk_tree_selection_select_path (selection, path);
  btk_tree_view_set_rules_hint (BTK_TREE_VIEW (tree_view), TRUE);
  btk_tree_view_set_headers_visible (BTK_TREE_VIEW (tree_view), FALSE);
  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tree_view),
					       -1,
					       NULL,
					       btk_cell_renderer_text_new (),
					       "text", TEXT_COLUMN,
					       NULL);
  btk_container_add (BTK_CONTAINER (scrolled_window), tree_view);
  hbox = btk_hbox_new (FALSE, FALSE);
  btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  button = btk_button_new_with_mnemonic ("<b>_Futz!!</b>");
  btk_box_pack_start (BTK_BOX (hbox), button, FALSE, FALSE, 0);
  btk_label_set_use_markup (BTK_LABEL (BTK_BIN (button)->child), TRUE);
  g_signal_connect (button, "clicked", G_CALLBACK (futz), NULL);
  g_signal_connect (button, "realize", G_CALLBACK (btk_widget_grab_focus), NULL);
  btk_window_set_default_size (BTK_WINDOW (window), 300, 400);
  btk_widget_show_all (window);
  bdk_threads_add_timeout (1000, (GSourceFunc) futz, NULL);
  btk_main ();
  return 0;
}
