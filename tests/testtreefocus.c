/* testtreefocus.c
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

typedef struct _TreeStruct TreeStruct;
struct _TreeStruct
{
  const bchar *label;
  bboolean alex;
  bboolean havoc;
  bboolean tim;
  bboolean owen;
  bboolean dave;
  bboolean world_holiday; /* shared by the european hackers */
  TreeStruct *children;
};


static TreeStruct january[] =
{
  {"New Years Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  {"Presidential Inauguration", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  {"Martin Luther King Jr. day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeStruct february[] =
{
  { "Presidents' Day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { "Groundhog Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Valentine's Day", FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, NULL },
  { NULL }
};

static TreeStruct march[] =
{
  { "National Tree Planting Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "St Patrick's Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { NULL }
};

static TreeStruct april[] =
{
  { "April Fools' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Army Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Earth Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Administrative Professionals' Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeStruct may[] =
{
  { "Nurses' Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "National Day of Prayer", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Mothers' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Armed Forces Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Memorial Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { NULL }
};

static TreeStruct june[] =
{
  { "June Fathers' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Juneteenth (Liberation of Slaves)", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Flag Day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeStruct july[] =
{
  { "Parents' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Independence Day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeStruct august[] =
{
  { "Air Force Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Coast Guard Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Friendship Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeStruct september[] =
{
  { "Grandparents' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Citizenship Day or Constitution Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Labor Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { NULL }
};

static TreeStruct october[] =
{
  { "National Children's Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Bosses' Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Sweetest Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Mother-in-Law's Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Navy Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Columbus Day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { "Halloween", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { NULL }
};

static TreeStruct november[] =
{
  { "Marine Corps Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Veterans' Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { "Thanksgiving", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeStruct december[] =
{
  { "Pearl Harbor Remembrance Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Christmas", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { "Kwanzaa", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { NULL }
};


static TreeStruct toplevel[] =
{
  {"January", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, january},
  {"February", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, february},
  {"March", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, march},
  {"April", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, april},
  {"May", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, may},
  {"June", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, june},
  {"July", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, july},
  {"August", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, august},
  {"September", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, september},
  {"October", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, october},
  {"November", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, november},
  {"December", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, december},
  {NULL}
};


enum
{
  HOLIDAY_COLUMN = 0,
  ALEX_COLUMN,
  HAVOC_COLUMN,
  TIM_COLUMN,
  OWEN_COLUMN,
  DAVE_COLUMN,
  VISIBLE_COLUMN,
  WORLD_COLUMN,
  NUM_COLUMNS
};

static BtkTreeModel *
make_model (void)
{
  BtkTreeStore *model;
  TreeStruct *month = toplevel;
  BtkTreeIter iter;

  model = btk_tree_store_new (NUM_COLUMNS,
			      B_TYPE_STRING,
			      B_TYPE_BOOLEAN,
			      B_TYPE_BOOLEAN,
			      B_TYPE_BOOLEAN,
			      B_TYPE_BOOLEAN,
			      B_TYPE_BOOLEAN,
			      B_TYPE_BOOLEAN,
			      B_TYPE_BOOLEAN);

  while (month->label)
    {
      TreeStruct *holiday = month->children;

      btk_tree_store_append (model, &iter, NULL);
      btk_tree_store_set (model, &iter,
			  HOLIDAY_COLUMN, month->label,
			  ALEX_COLUMN, FALSE,
			  HAVOC_COLUMN, FALSE,
			  TIM_COLUMN, FALSE,
			  OWEN_COLUMN, FALSE,
			  DAVE_COLUMN, FALSE,
			  VISIBLE_COLUMN, FALSE,
			  WORLD_COLUMN, FALSE,
			  -1);
      while (holiday->label)
	{
	  BtkTreeIter child_iter;

	  btk_tree_store_append (model, &child_iter, &iter);
	  btk_tree_store_set (model, &child_iter,
			      HOLIDAY_COLUMN, holiday->label,
			      ALEX_COLUMN, holiday->alex,
			      HAVOC_COLUMN, holiday->havoc,
			      TIM_COLUMN, holiday->tim,
			      OWEN_COLUMN, holiday->owen,
			      DAVE_COLUMN, holiday->dave,
			      VISIBLE_COLUMN, TRUE,
			      WORLD_COLUMN, holiday->world_holiday,
			      -1);

	  holiday ++;
	}
      month ++;
    }

  return BTK_TREE_MODEL (model);
}

static void
alex_toggled (BtkCellRendererToggle *cell,
	      bchar                 *path_str,
	      bpointer               data)
{
  BtkTreeModel *model = (BtkTreeModel *) data;
  BtkTreeIter iter;
  BtkTreePath *path = btk_tree_path_new_from_string (path_str);
  bboolean alex;

  btk_tree_model_get_iter (model, &iter, path);
  btk_tree_model_get (model, &iter, ALEX_COLUMN, &alex, -1);

  alex = !alex;
  btk_tree_store_set (BTK_TREE_STORE (model), &iter, ALEX_COLUMN, alex, -1);

  btk_tree_path_free (path);
}

static void
havoc_toggled (BtkCellRendererToggle *cell,
	       bchar                 *path_str,
	       bpointer               data)
{
  BtkTreeModel *model = (BtkTreeModel *) data;
  BtkTreeIter iter;
  BtkTreePath *path = btk_tree_path_new_from_string (path_str);
  bboolean havoc;

  btk_tree_model_get_iter (model, &iter, path);
  btk_tree_model_get (model, &iter, HAVOC_COLUMN, &havoc, -1);

  havoc = !havoc;
  btk_tree_store_set (BTK_TREE_STORE (model), &iter, HAVOC_COLUMN, havoc, -1);

  btk_tree_path_free (path);
}

static void
owen_toggled (BtkCellRendererToggle *cell,
	      bchar                 *path_str,
	      bpointer               data)
{
  BtkTreeModel *model = (BtkTreeModel *) data;
  BtkTreeIter iter;
  BtkTreePath *path = btk_tree_path_new_from_string (path_str);
  bboolean owen;

  btk_tree_model_get_iter (model, &iter, path);
  btk_tree_model_get (model, &iter, OWEN_COLUMN, &owen, -1);

  owen = !owen;
  btk_tree_store_set (BTK_TREE_STORE (model), &iter, OWEN_COLUMN, owen, -1);

  btk_tree_path_free (path);
}

static void
tim_toggled (BtkCellRendererToggle *cell,
	     bchar                 *path_str,
	     bpointer               data)
{
  BtkTreeModel *model = (BtkTreeModel *) data;
  BtkTreeIter iter;
  BtkTreePath *path = btk_tree_path_new_from_string (path_str);
  bboolean tim;

  btk_tree_model_get_iter (model, &iter, path);
  btk_tree_model_get (model, &iter, TIM_COLUMN, &tim, -1);

  tim = !tim;
  btk_tree_store_set (BTK_TREE_STORE (model), &iter, TIM_COLUMN, tim, -1);

  btk_tree_path_free (path);
}

static void
dave_toggled (BtkCellRendererToggle *cell,
	      bchar                 *path_str,
	      bpointer               data)
{
  BtkTreeModel *model = (BtkTreeModel *) data;
  BtkTreeIter iter;
  BtkTreePath *path = btk_tree_path_new_from_string (path_str);
  bboolean dave;

  btk_tree_model_get_iter (model, &iter, path);
  btk_tree_model_get (model, &iter, DAVE_COLUMN, &dave, -1);

  dave = !dave;
  btk_tree_store_set (BTK_TREE_STORE (model), &iter, DAVE_COLUMN, dave, -1);

  btk_tree_path_free (path);
}

static void
set_indicator_size (BtkTreeViewColumn *column,
		    BtkCellRenderer *cell,
		    BtkTreeModel *model,
		    BtkTreeIter *iter,
		    bpointer data)
{
  bint size;
  BtkTreePath *path;

  path = btk_tree_model_get_path (model, iter);
  size = btk_tree_path_get_indices (path)[0]  * 2 + 10;
  btk_tree_path_free (path);

  g_object_set (cell, "indicator_size", size, NULL);
}

int
main (int argc, char *argv[])
{
  BtkWidget *window;
  BtkWidget *vbox;
  BtkWidget *scrolled_window;
  BtkWidget *tree_view;
  BtkTreeModel *model;
  BtkCellRenderer *renderer;
  bint col_offset;
  BtkTreeViewColumn *column;

  btk_init (&argc, &argv);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_title (BTK_WINDOW (window), "Card planning sheet");
  g_signal_connect (window, "destroy", btk_main_quit, NULL);
  vbox = btk_vbox_new (FALSE, 8);
  btk_container_set_border_width (BTK_CONTAINER (vbox), 8);
  btk_box_pack_start (BTK_BOX (vbox), btk_label_new ("Jonathan's Holiday Card Planning Sheet"), FALSE, FALSE, 0);
  btk_container_add (BTK_CONTAINER (window), vbox);

  scrolled_window = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (scrolled_window), BTK_SHADOW_ETCHED_IN);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window), BTK_POLICY_AUTOMATIC, BTK_POLICY_AUTOMATIC);
  btk_box_pack_start (BTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);

  model = make_model ();
  tree_view = btk_tree_view_new_with_model (model);
  btk_tree_view_set_rules_hint (BTK_TREE_VIEW (tree_view), TRUE);
  btk_tree_selection_set_mode (btk_tree_view_get_selection (BTK_TREE_VIEW (tree_view)),
			       BTK_SELECTION_MULTIPLE);
  renderer = btk_cell_renderer_text_new ();
  col_offset = btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tree_view),
							    -1, "Holiday",
							    renderer,
							    "text", HOLIDAY_COLUMN, NULL);
  column = btk_tree_view_get_column (BTK_TREE_VIEW (tree_view), col_offset - 1);
  btk_tree_view_column_set_clickable (BTK_TREE_VIEW_COLUMN (column), TRUE);

  /* Alex Column */
  renderer = btk_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled", G_CALLBACK (alex_toggled), model);

  g_object_set (renderer, "xalign", 0.0, NULL);
  col_offset = btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tree_view),
							    -1, "Alex",
							    renderer,
							    "active", ALEX_COLUMN,
							    "visible", VISIBLE_COLUMN,
							    "activatable", WORLD_COLUMN,
							    NULL);
  column = btk_tree_view_get_column (BTK_TREE_VIEW (tree_view), col_offset - 1);
  btk_tree_view_column_set_sizing (BTK_TREE_VIEW_COLUMN (column), BTK_TREE_VIEW_COLUMN_FIXED);
  btk_tree_view_column_set_fixed_width (BTK_TREE_VIEW_COLUMN (column), 50);
  btk_tree_view_column_set_clickable (BTK_TREE_VIEW_COLUMN (column), TRUE);

  /* Havoc Column */
  renderer = btk_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled", G_CALLBACK (havoc_toggled), model);

  g_object_set (renderer, "xalign", 0.0, NULL);
  col_offset = btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tree_view),
							    -1, "Havoc",
							    renderer,
							    "active", HAVOC_COLUMN,
							    "visible", VISIBLE_COLUMN,
							    NULL);
  column = btk_tree_view_get_column (BTK_TREE_VIEW (tree_view), col_offset - 1);
  btk_tree_view_column_set_sizing (BTK_TREE_VIEW_COLUMN (column), BTK_TREE_VIEW_COLUMN_FIXED);
  btk_tree_view_column_set_fixed_width (BTK_TREE_VIEW_COLUMN (column), 50);
  btk_tree_view_column_set_clickable (BTK_TREE_VIEW_COLUMN (column), TRUE);

  /* Tim Column */
  renderer = btk_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled", G_CALLBACK (tim_toggled), model);

  g_object_set (renderer, "xalign", 0.0, NULL);
  col_offset = btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tree_view),
					       -1, "Tim",
					       renderer,
					       "active", TIM_COLUMN,
					       "visible", VISIBLE_COLUMN,
					       "activatable", WORLD_COLUMN,
					       NULL);
  column = btk_tree_view_get_column (BTK_TREE_VIEW (tree_view), col_offset - 1);
  btk_tree_view_column_set_sizing (BTK_TREE_VIEW_COLUMN (column), BTK_TREE_VIEW_COLUMN_FIXED);
  btk_tree_view_column_set_clickable (BTK_TREE_VIEW_COLUMN (column), TRUE);
  btk_tree_view_column_set_fixed_width (BTK_TREE_VIEW_COLUMN (column), 50);

  /* Owen Column */
  renderer = btk_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled", G_CALLBACK (owen_toggled), model);
  g_object_set (renderer, "xalign", 0.0, NULL);
  col_offset = btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tree_view),
					       -1, "Owen",
					       renderer,
					       "active", OWEN_COLUMN,
					       "visible", VISIBLE_COLUMN,
					       NULL);
  column = btk_tree_view_get_column (BTK_TREE_VIEW (tree_view), col_offset - 1);
  btk_tree_view_column_set_sizing (BTK_TREE_VIEW_COLUMN (column), BTK_TREE_VIEW_COLUMN_FIXED);
  btk_tree_view_column_set_clickable (BTK_TREE_VIEW_COLUMN (column), TRUE);
  btk_tree_view_column_set_fixed_width (BTK_TREE_VIEW_COLUMN (column), 50);

  /* Owen Column */
  renderer = btk_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled", G_CALLBACK (dave_toggled), model);
  g_object_set (renderer, "xalign", 0.0, NULL);
  col_offset = btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tree_view),
					       -1, "Dave",
					       renderer,
					       "active", DAVE_COLUMN,
					       "visible", VISIBLE_COLUMN,
					       NULL);
  column = btk_tree_view_get_column (BTK_TREE_VIEW (tree_view), col_offset - 1);
  btk_tree_view_column_set_cell_data_func (column, renderer, set_indicator_size, NULL, NULL);
  btk_tree_view_column_set_sizing (BTK_TREE_VIEW_COLUMN (column), BTK_TREE_VIEW_COLUMN_FIXED);
  btk_tree_view_column_set_fixed_width (BTK_TREE_VIEW_COLUMN (column), 50);
  btk_tree_view_column_set_clickable (BTK_TREE_VIEW_COLUMN (column), TRUE);

  btk_container_add (BTK_CONTAINER (scrolled_window), tree_view);

  g_signal_connect (tree_view, "realize",
		    G_CALLBACK (btk_tree_view_expand_all),
		    NULL);
  btk_window_set_default_size (BTK_WINDOW (window),
			       650, 400);
  btk_widget_show_all (window);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_title (BTK_WINDOW (window), "Model");
  g_signal_connect (window, "destroy", btk_main_quit, NULL);
  vbox = btk_vbox_new (FALSE, 8);
  btk_container_set_border_width (BTK_CONTAINER (vbox), 8);
  btk_box_pack_start (BTK_BOX (vbox), btk_label_new ("The model revealed"), FALSE, FALSE, 0);
  btk_container_add (BTK_CONTAINER (window), vbox);

  scrolled_window = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (scrolled_window), BTK_SHADOW_ETCHED_IN);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window), BTK_POLICY_AUTOMATIC, BTK_POLICY_AUTOMATIC);
  btk_box_pack_start (BTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);


  tree_view = btk_tree_view_new_with_model (model);
  g_object_unref (model);
  btk_tree_view_set_rules_hint (BTK_TREE_VIEW (tree_view), TRUE);

  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tree_view),
					       -1, "Holiday Column",
					       btk_cell_renderer_text_new (),
					       "text", 0, NULL);

  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tree_view),
					       -1, "Alex Column",
					       btk_cell_renderer_text_new (),
					       "text", 1, NULL);

  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tree_view),
					       -1, "Havoc Column",
					       btk_cell_renderer_text_new (),
					       "text", 2, NULL);

  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tree_view),
					       -1, "Tim Column",
					       btk_cell_renderer_text_new (),
					       "text", 3, NULL);

  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tree_view),
					       -1, "Owen Column",
					       btk_cell_renderer_text_new (),
					       "text", 4, NULL);

  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tree_view),
					       -1, "Dave Column",
					       btk_cell_renderer_text_new (),
					       "text", 5, NULL);

  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tree_view),
					       -1, "Visible Column",
					       btk_cell_renderer_text_new (),
					       "text", 6, NULL);

  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tree_view),
					       -1, "World Holiday",
					       btk_cell_renderer_text_new (),
					       "text", 7, NULL);

  g_signal_connect (tree_view, "realize",
		    G_CALLBACK (btk_tree_view_expand_all),
		    NULL);
			   
  btk_container_add (BTK_CONTAINER (scrolled_window), tree_view);


  btk_window_set_default_size (BTK_WINDOW (window),
			       650, 400);

  btk_widget_show_all (window);
  btk_main ();

  return 0;
}

