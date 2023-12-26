/* testtreesort.c
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

#include "btk/btktreedatalist.h"


typedef struct _ListSort ListSort;
struct _ListSort
{
  const gchar *word_1;
  const gchar *word_2;
  const gchar *word_3;
  const gchar *word_4;
  gint number_1;
};

static ListSort data[] =
{
  { "Apples", "Transmogrify long word to demonstrate weirdness", "Exculpatory", "Gesundheit", 30 },
  { "Oranges", "Wicker", "Adamantine", "Convivial", 10 },
  { "Bovine Spongiform Encephilopathy", "Sleazebucket", "Mountaineer", "Pander", 40 },
  { "Foot and Mouth", "Lampshade", "Skim Milk\nFull Milk", "Viewless", 20 },
  { "Blood,\nsweat,\ntears", "The Man", "Horses", "Muckety-Muck", 435 },
  { "Rare Steak", "Siam", "Watchdog", "Xantippe" , 99999 },
  { "SIGINT", "Rabbit Breath", "Alligator", "Bloodstained", 4123 },
  { "Google", "Chrysanthemums", "Hobnob", "Leapfrog", 1 },
  { "Technology fibre optic", "Turtle", "Academe", "Lonely", 3 },
  { "Freon", "Harpes", "Quidditch", "Reagan", 6},
  { "Transposition", "Fruit Basket", "Monkey Wort", "Glogg", 54 },
  { "Fern", "Glasnost and Perestroika", "Latitude", "Bomberman!!!", 2 },
  {NULL, }
};

static ListSort childdata[] =
{
  { "Heineken", "Nederland", "Wanda de vis", "Electronische post", 2},
  { "Hottentottententententoonstelling", "Rotterdam", "Ionentransport", "Palm", 45},
  { "Fruitvlieg", "Eigenfrequentie", "Supernoodles", "Ramen", 2002},
  { "Gereedschapskist", "Stelsel van lineaire vergelijkingen", "Tulpen", "Badlaken", 1311},
  { "Stereoinstallatie", "Rood tapijt", "Het periodieke systeem der elementen", "Laaste woord", 200},
  {NULL, }
};
  

enum
{
  WORD_COLUMN_1 = 0,
  WORD_COLUMN_2,
  WORD_COLUMN_3,
  WORD_COLUMN_4,
  NUMBER_COLUMN_1,
  NUM_COLUMNS
};

gboolean
select_func (BtkTreeSelection  *selection,
	     BtkTreeModel      *model,
	     BtkTreePath       *path,
	     gboolean           path_currently_selected,
	     gpointer           data)
{
  if (btk_tree_path_get_depth (path) > 1)
    return TRUE;
  return FALSE;
}

static void
switch_search_method (BtkWidget *button,
		      gpointer   tree_view)
{
  if (!btk_tree_view_get_search_entry (BTK_TREE_VIEW (tree_view)))
    {
      gpointer data = g_object_get_data (tree_view, "my-search-entry");
      btk_tree_view_set_search_entry (BTK_TREE_VIEW (tree_view), BTK_ENTRY (data));
    }
  else
    btk_tree_view_set_search_entry (BTK_TREE_VIEW (tree_view), NULL);
}

int
main (int argc, char *argv[])
{
  BtkWidget *window;
  BtkWidget *vbox;
  BtkWidget *scrolled_window;
  BtkWidget *tree_view;
  BtkTreeStore *model;
  BtkTreeModel *smodel = NULL;
  BtkTreeModel *ssmodel = NULL;
  BtkCellRenderer *renderer;
  BtkTreeViewColumn *column;
  BtkTreeIter iter;
  gint i;

  BtkWidget *entry, *button;
  BtkWidget *window2, *vbox2, *scrolled_window2, *tree_view2;
  BtkWidget *window3, *vbox3, *scrolled_window3, *tree_view3;

  btk_init (&argc, &argv);

  /**
   * First window - Just a BtkTreeStore
   */

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_title (BTK_WINDOW (window), "Words, words, words - Window 1");
  g_signal_connect (window, "destroy", btk_main_quit, NULL);
  vbox = btk_vbox_new (FALSE, 8);
  btk_container_set_border_width (BTK_CONTAINER (vbox), 8);
  btk_box_pack_start (BTK_BOX (vbox), btk_label_new ("Jonathan and Kristian's list of cool words. (And Anders' cool list of numbers) \n\nThis is just a BtkTreeStore"), FALSE, FALSE, 0);
  btk_container_add (BTK_CONTAINER (window), vbox);

  entry = btk_entry_new ();
  btk_box_pack_start (BTK_BOX (vbox), entry, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Switch search method");
  btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  scrolled_window = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (scrolled_window), BTK_SHADOW_ETCHED_IN);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window), BTK_POLICY_AUTOMATIC, BTK_POLICY_AUTOMATIC);
  btk_box_pack_start (BTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);

  model = btk_tree_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

/*
  smodel = btk_tree_model_sort_new_with_model (BTK_TREE_MODEL (model));
  ssmodel = btk_tree_model_sort_new_with_model (BTK_TREE_MODEL (smodel));
*/
  tree_view = btk_tree_view_new_with_model (BTK_TREE_MODEL (model));

  btk_tree_view_set_search_entry (BTK_TREE_VIEW (tree_view), BTK_ENTRY (entry));
  g_object_set_data (G_OBJECT (tree_view), "my-search-entry", entry);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (switch_search_method), tree_view);

 /* btk_tree_selection_set_select_function (btk_tree_view_get_selection (BTK_TREE_VIEW (tree_view)), select_func, NULL, NULL);*/

  /* 12 iters now, 12 later... */
  for (i = 0; data[i].word_1 != NULL; i++)
    {
      gint k;
      BtkTreeIter child_iter;


      btk_tree_store_prepend (BTK_TREE_STORE (model), &iter, NULL);
      btk_tree_store_set (BTK_TREE_STORE (model), &iter,
			  WORD_COLUMN_1, data[i].word_1,
			  WORD_COLUMN_2, data[i].word_2,
			  WORD_COLUMN_3, data[i].word_3,
			  WORD_COLUMN_4, data[i].word_4,
			  NUMBER_COLUMN_1, data[i].number_1,
			  -1);

      btk_tree_store_append (BTK_TREE_STORE (model), &child_iter, &iter);
      btk_tree_store_set (BTK_TREE_STORE (model), &child_iter,
			  WORD_COLUMN_1, data[i].word_1,
			  WORD_COLUMN_2, data[i].word_2,
			  WORD_COLUMN_3, data[i].word_3,
			  WORD_COLUMN_4, data[i].word_4,
			  NUMBER_COLUMN_1, data[i].number_1,
			  -1);

      for (k = 0; childdata[k].word_1 != NULL; k++)
	{
	  btk_tree_store_append (BTK_TREE_STORE (model), &child_iter, &iter);
	  btk_tree_store_set (BTK_TREE_STORE (model), &child_iter,
			      WORD_COLUMN_1, childdata[k].word_1,
			      WORD_COLUMN_2, childdata[k].word_2,
			      WORD_COLUMN_3, childdata[k].word_3,
			      WORD_COLUMN_4, childdata[k].word_4,
			      NUMBER_COLUMN_1, childdata[k].number_1,
			      -1);

	}

    }
  
  smodel = btk_tree_model_sort_new_with_model (BTK_TREE_MODEL (model));
  ssmodel = btk_tree_model_sort_new_with_model (BTK_TREE_MODEL (smodel));
  g_object_unref (model);

  btk_tree_view_set_rules_hint (BTK_TREE_VIEW (tree_view), TRUE);

  renderer = btk_cell_renderer_text_new ();
  column = btk_tree_view_column_new_with_attributes ("First Word", renderer,
						     "text", WORD_COLUMN_1,
						     NULL);
  btk_tree_view_append_column (BTK_TREE_VIEW (tree_view), column);
  btk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_1);

  renderer = btk_cell_renderer_text_new ();
  column = btk_tree_view_column_new_with_attributes ("Second Word", renderer,
						     "text", WORD_COLUMN_2,
						     NULL);
  btk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_2);
  btk_tree_view_append_column (BTK_TREE_VIEW (tree_view), column);

  renderer = btk_cell_renderer_text_new ();
  column = btk_tree_view_column_new_with_attributes ("Third Word", renderer,
						     "text", WORD_COLUMN_3,
						     NULL);
  btk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_3);
  btk_tree_view_append_column (BTK_TREE_VIEW (tree_view), column);

  renderer = btk_cell_renderer_text_new ();
  column = btk_tree_view_column_new_with_attributes ("Fourth Word", renderer,
						     "text", WORD_COLUMN_4,
						     NULL);
  btk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_4);
  btk_tree_view_append_column (BTK_TREE_VIEW (tree_view), column);
  
  renderer = btk_cell_renderer_text_new ();
  column = btk_tree_view_column_new_with_attributes ("First Number", renderer,
						     "text", NUMBER_COLUMN_1,
						     NULL);
  btk_tree_view_column_set_sort_column_id (column, NUMBER_COLUMN_1);
  btk_tree_view_append_column (BTK_TREE_VIEW (tree_view), column);

  /*  btk_tree_sortable_set_sort_column_id (BTK_TREE_SORTABLE (smodel),
					WORD_COLUMN_1,
					BTK_SORT_ASCENDING);*/

  btk_container_add (BTK_CONTAINER (scrolled_window), tree_view);
  btk_window_set_default_size (BTK_WINDOW (window), 400, 400);
  btk_widget_show_all (window);

  /**
   * Second window - BtkTreeModelSort wrapping the BtkTreeStore
   */

  if (smodel)
    {
      window2 = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_title (BTK_WINDOW (window2), 
			    "Words, words, words - window 2");
      g_signal_connect (window2, "destroy", btk_main_quit, NULL);
      vbox2 = btk_vbox_new (FALSE, 8);
      btk_container_set_border_width (BTK_CONTAINER (vbox2), 8);
      btk_box_pack_start (BTK_BOX (vbox2), 
			  btk_label_new ("Jonathan and Kristian's list of words.\n\nA BtkTreeModelSort wrapping the BtkTreeStore of window 1"),
			  FALSE, FALSE, 0);
      btk_container_add (BTK_CONTAINER (window2), vbox2);
      
      scrolled_window2 = btk_scrolled_window_new (NULL, NULL);
      btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (scrolled_window2),
					   BTK_SHADOW_ETCHED_IN);
      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window2),
				      BTK_POLICY_AUTOMATIC,
				      BTK_POLICY_AUTOMATIC);
      btk_box_pack_start (BTK_BOX (vbox2), scrolled_window2, TRUE, TRUE, 0);
      
      
      tree_view2 = btk_tree_view_new_with_model (smodel);
      btk_tree_view_set_rules_hint (BTK_TREE_VIEW (tree_view2), TRUE);
      
      
      renderer = btk_cell_renderer_text_new ();
      column = btk_tree_view_column_new_with_attributes ("First Word", renderer,
							 "text", WORD_COLUMN_1,
							 NULL);
      btk_tree_view_append_column (BTK_TREE_VIEW (tree_view2), column);
      btk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_1);
      
      renderer = btk_cell_renderer_text_new ();
      column = btk_tree_view_column_new_with_attributes ("Second Word", renderer,
							 "text", WORD_COLUMN_2,
							 NULL);
      btk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_2);
      btk_tree_view_append_column (BTK_TREE_VIEW (tree_view2), column);
      
      renderer = btk_cell_renderer_text_new ();
      column = btk_tree_view_column_new_with_attributes ("Third Word", renderer,
							 "text", WORD_COLUMN_3,
							 NULL);
      btk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_3);
      btk_tree_view_append_column (BTK_TREE_VIEW (tree_view2), column);
      
      renderer = btk_cell_renderer_text_new ();
      column = btk_tree_view_column_new_with_attributes ("Fourth Word", renderer,
							 "text", WORD_COLUMN_4,
							 NULL);
      btk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_4);
      btk_tree_view_append_column (BTK_TREE_VIEW (tree_view2), column);
      
      /*      btk_tree_sortable_set_default_sort_func (BTK_TREE_SORTABLE (smodel),
					       (BtkTreeIterCompareFunc)btk_tree_data_list_compare_func,
					       NULL, NULL);
      btk_tree_sortable_set_sort_column_id (BTK_TREE_SORTABLE (smodel),
					    WORD_COLUMN_1,
					    BTK_SORT_DESCENDING);*/
      
      
      btk_container_add (BTK_CONTAINER (scrolled_window2), tree_view2);
      btk_window_set_default_size (BTK_WINDOW (window2), 400, 400);
      btk_widget_show_all (window2);
    }
  
  /**
   * Third window - BtkTreeModelSort wrapping the BtkTreeModelSort which
   * is wrapping the BtkTreeStore.
   */
  
  if (ssmodel)
    {
      window3 = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_title (BTK_WINDOW (window3), 
			    "Words, words, words - Window 3");
      g_signal_connect (window3, "destroy", btk_main_quit, NULL);
      vbox3 = btk_vbox_new (FALSE, 8);
      btk_container_set_border_width (BTK_CONTAINER (vbox3), 8);
      btk_box_pack_start (BTK_BOX (vbox3), 
			  btk_label_new ("Jonathan and Kristian's list of words.\n\nA BtkTreeModelSort wrapping the BtkTreeModelSort of window 2"),
			  FALSE, FALSE, 0);
      btk_container_add (BTK_CONTAINER (window3), vbox3);
      
      scrolled_window3 = btk_scrolled_window_new (NULL, NULL);
      btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (scrolled_window3),
					   BTK_SHADOW_ETCHED_IN);
      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window3),
				      BTK_POLICY_AUTOMATIC,
				      BTK_POLICY_AUTOMATIC);
      btk_box_pack_start (BTK_BOX (vbox3), scrolled_window3, TRUE, TRUE, 0);
      
      
      tree_view3 = btk_tree_view_new_with_model (ssmodel);
      btk_tree_view_set_rules_hint (BTK_TREE_VIEW (tree_view3), TRUE);
      
      renderer = btk_cell_renderer_text_new ();
      column = btk_tree_view_column_new_with_attributes ("First Word", renderer,
							 "text", WORD_COLUMN_1,
							 NULL);
      btk_tree_view_append_column (BTK_TREE_VIEW (tree_view3), column);
      btk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_1);
      
      renderer = btk_cell_renderer_text_new ();
      column = btk_tree_view_column_new_with_attributes ("Second Word", renderer,
							 "text", WORD_COLUMN_2,
							 NULL);
      btk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_2);
      btk_tree_view_append_column (BTK_TREE_VIEW (tree_view3), column);
      
      renderer = btk_cell_renderer_text_new ();
      column = btk_tree_view_column_new_with_attributes ("Third Word", renderer,
							 "text", WORD_COLUMN_3,
							 NULL);
      btk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_3);
      btk_tree_view_append_column (BTK_TREE_VIEW (tree_view3), column);
      
      renderer = btk_cell_renderer_text_new ();
      column = btk_tree_view_column_new_with_attributes ("Fourth Word", renderer,
							 "text", WORD_COLUMN_4,
							 NULL);
      btk_tree_view_column_set_sort_column_id (column, WORD_COLUMN_4);
      btk_tree_view_append_column (BTK_TREE_VIEW (tree_view3), column);
      
      /*      btk_tree_sortable_set_default_sort_func (BTK_TREE_SORTABLE (ssmodel),
					       (BtkTreeIterCompareFunc)btk_tree_data_list_compare_func,
					       NULL, NULL);
      btk_tree_sortable_set_sort_column_id (BTK_TREE_SORTABLE (ssmodel),
					    WORD_COLUMN_1,
					    BTK_SORT_ASCENDING);*/
      
      btk_container_add (BTK_CONTAINER (scrolled_window3), tree_view3);
      btk_window_set_default_size (BTK_WINDOW (window3), 400, 400);
      btk_widget_show_all (window3);
    }

  for (i = 0; data[i].word_1 != NULL; i++)
    {
      gint k;
      
      btk_tree_store_prepend (BTK_TREE_STORE (model), &iter, NULL);
      btk_tree_store_set (BTK_TREE_STORE (model), &iter,
			  WORD_COLUMN_1, data[i].word_1,
			  WORD_COLUMN_2, data[i].word_2,
			  WORD_COLUMN_3, data[i].word_3,
			  WORD_COLUMN_4, data[i].word_4,
			  -1);
      for (k = 0; childdata[k].word_1 != NULL; k++)
	{
	  BtkTreeIter child_iter;
	  
	  btk_tree_store_append (BTK_TREE_STORE (model), &child_iter, &iter);
	  btk_tree_store_set (BTK_TREE_STORE (model), &child_iter,
			      WORD_COLUMN_1, childdata[k].word_1,
			      WORD_COLUMN_2, childdata[k].word_2,
			      WORD_COLUMN_3, childdata[k].word_3,
			      WORD_COLUMN_4, childdata[k].word_4,
			      -1);
	}
    }

  btk_main ();
  
  return 0;
}
