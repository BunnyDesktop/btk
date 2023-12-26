/* treestoretest.c
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
#include <stdlib.h>
#include <string.h>

BtkTreeStore *base_model;
static bint node_count = 0;

static void
selection_changed (BtkTreeSelection *selection,
		   BtkWidget        *button)
{
  if (btk_tree_selection_get_selected (selection, NULL, NULL))
    btk_widget_set_sensitive (button, TRUE);
  else
    btk_widget_set_sensitive (button, FALSE);
}

static void
node_set (BtkTreeIter *iter)
{
  bint n;
  bchar *str;

  str = g_strdup_printf ("Row (<span color=\"red\">%d</span>)", node_count++);
  btk_tree_store_set (base_model, iter, 0, str, -1);
  g_free (str);

  n = g_random_int_range (10000,99999);
  if (n < 0)
    n *= -1;
  str = g_strdup_printf ("%d", n);

  btk_tree_store_set (base_model, iter, 1, str, -1);
  g_free (str);
}

static void
iter_remove (BtkWidget *button, BtkTreeView *tree_view)
{
  BtkTreeIter selected;
  BtkTreeModel *model;

  model = btk_tree_view_get_model (tree_view);

  if (btk_tree_selection_get_selected (btk_tree_view_get_selection (tree_view),
				       NULL,
				       &selected))
    {
      if (BTK_IS_TREE_STORE (model))
	{
	  btk_tree_store_remove (BTK_TREE_STORE (model), &selected);
	}
    }
}

static void
iter_insert (BtkWidget *button, BtkTreeView *tree_view)
{
  BtkWidget *entry;
  BtkTreeIter iter;
  BtkTreeIter selected;
  BtkTreeModel *model = btk_tree_view_get_model (tree_view);

  entry = g_object_get_data (B_OBJECT (button), "user_data");
  if (btk_tree_selection_get_selected (btk_tree_view_get_selection (BTK_TREE_VIEW (tree_view)),
				       NULL,
				       &selected))
    {
      btk_tree_store_insert (BTK_TREE_STORE (model),
			     &iter,
			     &selected,
			     atoi (btk_entry_get_text (BTK_ENTRY (entry))));
    }
  else
    {
      btk_tree_store_insert (BTK_TREE_STORE (model),
			     &iter,
			     NULL,
			     atoi (btk_entry_get_text (BTK_ENTRY (entry))));
    }

  node_set (&iter);
}

static void
iter_change (BtkWidget *button, BtkTreeView *tree_view)
{
  BtkWidget *entry;
  BtkTreeIter selected;
  BtkTreeModel *model = btk_tree_view_get_model (tree_view);

  entry = g_object_get_data (B_OBJECT (button), "user_data");
  if (btk_tree_selection_get_selected (btk_tree_view_get_selection (BTK_TREE_VIEW (tree_view)),
				       NULL, &selected))
    {
      btk_tree_store_set (BTK_TREE_STORE (model),
			  &selected,
			  1,
			  btk_entry_get_text (BTK_ENTRY (entry)),
			  -1);
    }
}

static void
iter_insert_with_values (BtkWidget *button, BtkTreeView *tree_view)
{
  BtkWidget *entry;
  BtkTreeIter iter;
  BtkTreeIter selected;
  BtkTreeModel *model = btk_tree_view_get_model (tree_view);
  bchar *str1, *str2;

  entry = g_object_get_data (B_OBJECT (button), "user_data");
  str1 = g_strdup_printf ("Row (<span color=\"red\">%d</span>)", node_count++);
  str2 = g_strdup_printf ("%d", atoi (btk_entry_get_text (BTK_ENTRY (entry))));

  if (btk_tree_selection_get_selected (btk_tree_view_get_selection (BTK_TREE_VIEW (tree_view)),
				       NULL,
				       &selected))
    {
      btk_tree_store_insert_with_values (BTK_TREE_STORE (model),
					 &iter,
					 &selected,
					 -1,
					 0, str1,
					 1, str2,
					 -1);
    }
  else
    {
      btk_tree_store_insert_with_values (BTK_TREE_STORE (model),
					 &iter,
					 NULL,
					 -1,
					 0, str1,
					 1, str2,
					 -1);
    }

  g_free (str1);
  g_free (str2);
}

static void
iter_insert_before  (BtkWidget *button, BtkTreeView *tree_view)
{
  BtkTreeIter iter;
  BtkTreeIter selected;
  BtkTreeModel *model = btk_tree_view_get_model (tree_view);

  if (btk_tree_selection_get_selected (btk_tree_view_get_selection (BTK_TREE_VIEW (tree_view)),
				       NULL,
				       &selected))
    {
      btk_tree_store_insert_before (BTK_TREE_STORE (model),
				    &iter,
				    NULL,
				    &selected);
    }
  else
    {
      btk_tree_store_insert_before (BTK_TREE_STORE (model),
				    &iter,
				    NULL,
				    NULL);
    }

  node_set (&iter);
}

static void
iter_insert_after (BtkWidget *button, BtkTreeView *tree_view)
{
  BtkTreeIter iter;
  BtkTreeIter selected;
  BtkTreeModel *model = btk_tree_view_get_model (tree_view);

  if (btk_tree_selection_get_selected (btk_tree_view_get_selection (BTK_TREE_VIEW (tree_view)),
				       NULL,
				       &selected))
    {
      if (BTK_IS_TREE_STORE (model))
	{
	  btk_tree_store_insert_after (BTK_TREE_STORE (model),
				       &iter,
				       NULL,
				       &selected);
	  node_set (&iter);
	}
    }
  else
    {
      if (BTK_IS_TREE_STORE (model))
	{
	  btk_tree_store_insert_after (BTK_TREE_STORE (model),
				       &iter,
				       NULL,
				       NULL);
	  node_set (&iter);
	}
    }
}

static void
iter_prepend (BtkWidget *button, BtkTreeView *tree_view)
{
  BtkTreeIter iter;
  BtkTreeIter selected;
  BtkTreeModel *model = btk_tree_view_get_model (tree_view);
  BtkTreeSelection *selection = btk_tree_view_get_selection (tree_view);

  if (btk_tree_selection_get_selected (selection, NULL, &selected))
    {
      if (BTK_IS_TREE_STORE (model))
	{
	  btk_tree_store_prepend (BTK_TREE_STORE (model),
				  &iter,
				  &selected);
	  node_set (&iter);
	}
    }
  else
    {
      if (BTK_IS_TREE_STORE (model))
	{
	  btk_tree_store_prepend (BTK_TREE_STORE (model),
				  &iter,
				  NULL);
	  node_set (&iter);
	}
    }
}

static void
iter_append (BtkWidget *button, BtkTreeView *tree_view)
{
  BtkTreeIter iter;
  BtkTreeIter selected;
  BtkTreeModel *model = btk_tree_view_get_model (tree_view);

  if (btk_tree_selection_get_selected (btk_tree_view_get_selection (BTK_TREE_VIEW (tree_view)),
				       NULL,
				       &selected))
    {
      if (BTK_IS_TREE_STORE (model))
	{
	  btk_tree_store_append (BTK_TREE_STORE (model), &iter, &selected);
	  node_set (&iter);
	}
    }
  else
    {
      if (BTK_IS_TREE_STORE (model))
	{
	  btk_tree_store_append (BTK_TREE_STORE (model), &iter, NULL);
	  node_set (&iter);
	}
    }
}

static void
make_window (bint view_type)
{
  BtkWidget *window;
  BtkWidget *vbox;
  BtkWidget *hbox, *entry;
  BtkWidget *button;
  BtkWidget *scrolled_window;
  BtkWidget *tree_view;
  BtkTreeViewColumn *column;
  BtkCellRenderer *cell;
  BObject *selection;

  /* Make the Widgets/Objects */
  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  switch (view_type)
    {
    case 0:
      btk_window_set_title (BTK_WINDOW (window), "Unsorted list");
      break;
    case 1:
      btk_window_set_title (BTK_WINDOW (window), "Sorted list");
      break;
    }

  vbox = btk_vbox_new (FALSE, 8);
  btk_container_set_border_width (BTK_CONTAINER (vbox), 8);
  btk_window_set_default_size (BTK_WINDOW (window), 300, 350);
  scrolled_window = btk_scrolled_window_new (NULL, NULL);
  switch (view_type)
    {
    case 0:
      tree_view = btk_tree_view_new_with_model (BTK_TREE_MODEL (base_model));
      break;
    case 1:
      {
	BtkTreeModel *sort_model;
	
	sort_model = btk_tree_model_sort_new_with_model (BTK_TREE_MODEL (base_model));
	tree_view = btk_tree_view_new_with_model (BTK_TREE_MODEL (sort_model));
      }
      break;
    default:
      g_assert_not_reached ();
      tree_view = NULL; /* Quiet compiler */
      break;
    }

  btk_tree_view_set_rules_hint (BTK_TREE_VIEW (tree_view), TRUE);
  selection = B_OBJECT (btk_tree_view_get_selection (BTK_TREE_VIEW (tree_view)));
  btk_tree_selection_set_mode (BTK_TREE_SELECTION (selection), BTK_SELECTION_SINGLE);

  /* Put them together */
  btk_container_add (BTK_CONTAINER (scrolled_window), tree_view);
  btk_box_pack_start (BTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
  btk_container_add (BTK_CONTAINER (window), vbox);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window),
				  BTK_POLICY_AUTOMATIC,
				  BTK_POLICY_AUTOMATIC);
  g_signal_connect (window, "destroy", btk_main_quit, NULL);

  /* buttons */
  button = btk_button_new_with_label ("btk_tree_store_remove");
  btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (selection, "changed",
                    G_CALLBACK (selection_changed),
                    button);
  g_signal_connect (button, "clicked", 
                    G_CALLBACK (iter_remove), 
                    tree_view);
  btk_widget_set_sensitive (button, FALSE);

  button = btk_button_new_with_label ("btk_tree_store_insert");
  hbox = btk_hbox_new (FALSE, 8);
  entry = btk_entry_new ();
  btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (hbox), entry, FALSE, FALSE, 0);
  g_object_set_data (B_OBJECT (button), "user_data", entry);
  g_signal_connect (button, "clicked", 
                    G_CALLBACK (iter_insert), 
                    tree_view);
  
  button = btk_button_new_with_label ("btk_tree_store_set");
  hbox = btk_hbox_new (FALSE, 8);
  entry = btk_entry_new ();
  btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (hbox), entry, FALSE, FALSE, 0);
  g_object_set_data (B_OBJECT (button), "user_data", entry);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (iter_change),
		    tree_view);

  button = btk_button_new_with_label ("btk_tree_store_insert_with_values");
  hbox = btk_hbox_new (FALSE, 8);
  entry = btk_entry_new ();
  btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (hbox), entry, FALSE, FALSE, 0);
  g_object_set_data (B_OBJECT (button), "user_data", entry);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (iter_insert_with_values),
		    tree_view);
  
  button = btk_button_new_with_label ("btk_tree_store_insert_before");
  btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", 
                    G_CALLBACK (iter_insert_before), 
                    tree_view);
  g_signal_connect (selection, "changed",
                    G_CALLBACK (selection_changed),
                    button);
  btk_widget_set_sensitive (button, FALSE);

  button = btk_button_new_with_label ("btk_tree_store_insert_after");
  btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", 
                    G_CALLBACK (iter_insert_after), 
                    tree_view);
  g_signal_connect (selection, "changed",
                    G_CALLBACK (selection_changed),
                    button);
  btk_widget_set_sensitive (button, FALSE);

  button = btk_button_new_with_label ("btk_tree_store_prepend");
  btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", 
                    G_CALLBACK (iter_prepend), 
                    tree_view);

  button = btk_button_new_with_label ("btk_tree_store_append");
  btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", 
                    G_CALLBACK (iter_append), 
                    tree_view);

  /* The selected column */
  cell = btk_cell_renderer_text_new ();
  column = btk_tree_view_column_new_with_attributes ("Node ID", cell, "markup", 0, NULL);
  btk_tree_view_column_set_sort_column_id (column, 0);
  btk_tree_view_append_column (BTK_TREE_VIEW (tree_view), column);

  cell = btk_cell_renderer_text_new ();
  column = btk_tree_view_column_new_with_attributes ("Random Number", cell, "text", 1, NULL);
  btk_tree_view_column_set_sort_column_id (column, 1);
  btk_tree_view_append_column (BTK_TREE_VIEW (tree_view), column);

  /* A few to start */
  if (view_type == 0)
    {
      iter_append (NULL, BTK_TREE_VIEW (tree_view));
      iter_append (NULL, BTK_TREE_VIEW (tree_view));
      iter_append (NULL, BTK_TREE_VIEW (tree_view));
      iter_append (NULL, BTK_TREE_VIEW (tree_view));
      iter_append (NULL, BTK_TREE_VIEW (tree_view));
      iter_append (NULL, BTK_TREE_VIEW (tree_view));
    }
  /* Show it all */
  btk_widget_show_all (window);
}

int
main (int argc, char *argv[])
{
  btk_init (&argc, &argv);

  base_model = btk_tree_store_new (2, B_TYPE_STRING, B_TYPE_STRING);

  /* FIXME: reverse this */
  make_window (0);
  make_window (1);

  btk_main ();

  return 0;
}

