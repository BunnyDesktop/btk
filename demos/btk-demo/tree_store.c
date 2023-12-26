/* Tree View/Tree Store
 *
 * The BtkTreeStore is used to store data in tree form, to be
 * used later on by a BtkTreeView to display it. This demo builds
 * a simple BtkTreeStore and displays it. If you're new to the
 * BtkTreeView widgets and associates, look into the BtkListStore
 * example first.
 *
 */

#include <btk/btk.h>

static BtkWidget *window = NULL;

/* TreeItem structure */
typedef struct _TreeItem TreeItem;
struct _TreeItem
{
  const gchar    *label;
  gboolean        alex;
  gboolean        havoc;
  gboolean        tim;
  gboolean        owen;
  gboolean        dave;
  gboolean        world_holiday; /* shared by the European hackers */
  TreeItem       *children;
};

/* columns */
enum
{
  HOLIDAY_NAME_COLUMN = 0,
  ALEX_COLUMN,
  HAVOC_COLUMN,
  TIM_COLUMN,
  OWEN_COLUMN,
  DAVE_COLUMN,

  VISIBLE_COLUMN,
  WORLD_COLUMN,
  NUM_COLUMNS
};

/* tree data */
static TreeItem january[] =
{
  {"New Years Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  {"Presidential Inauguration", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  {"Martin Luther King Jr. day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeItem february[] =
{
  { "Presidents' Day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { "Groundhog Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Valentine's Day", FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, NULL },
  { NULL }
};

static TreeItem march[] =
{
  { "National Tree Planting Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "St Patrick's Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { NULL }
};
static TreeItem april[] =
{
  { "April Fools' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Army Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Earth Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Administrative Professionals' Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeItem may[] =
{
  { "Nurses' Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "National Day of Prayer", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Mothers' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Armed Forces Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Memorial Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { NULL }
};

static TreeItem june[] =
{
  { "June Fathers' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Juneteenth (Liberation of Slaves)", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Flag Day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeItem july[] =
{
  { "Parents' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Independence Day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeItem august[] =
{
  { "Air Force Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Coast Guard Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Friendship Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeItem september[] =
{
  { "Grandparents' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Citizenship Day or Constitution Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Labor Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { NULL }
};

static TreeItem october[] =
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

static TreeItem november[] =
{
  { "Marine Corps Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Veterans' Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { "Thanksgiving", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeItem december[] =
{
  { "Pearl Harbor Remembrance Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Christmas", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { "Kwanzaa", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { NULL }
};


static TreeItem toplevel[] =
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


static BtkTreeModel *
create_model (void)
{
  BtkTreeStore *model;
  BtkTreeIter iter;
  TreeItem *month = toplevel;

  /* create tree store */
  model = btk_tree_store_new (NUM_COLUMNS,
			      B_TYPE_STRING,
			      B_TYPE_BOOLEAN,
			      B_TYPE_BOOLEAN,
			      B_TYPE_BOOLEAN,
			      B_TYPE_BOOLEAN,
			      B_TYPE_BOOLEAN,
			      B_TYPE_BOOLEAN,
			      B_TYPE_BOOLEAN);

  /* add data to the tree store */
  while (month->label)
    {
      TreeItem *holiday = month->children;

      btk_tree_store_append (model, &iter, NULL);
      btk_tree_store_set (model, &iter,
			  HOLIDAY_NAME_COLUMN, month->label,
			  ALEX_COLUMN, FALSE,
			  HAVOC_COLUMN, FALSE,
			  TIM_COLUMN, FALSE,
			  OWEN_COLUMN, FALSE,
			  DAVE_COLUMN, FALSE,
			  VISIBLE_COLUMN, FALSE,
			  WORLD_COLUMN, FALSE,
			  -1);

      /* add children */
      while (holiday->label)
	{
	  BtkTreeIter child_iter;

	  btk_tree_store_append (model, &child_iter, &iter);
	  btk_tree_store_set (model, &child_iter,
			      HOLIDAY_NAME_COLUMN, holiday->label,
			      ALEX_COLUMN, holiday->alex,
			      HAVOC_COLUMN, holiday->havoc,
			      TIM_COLUMN, holiday->tim,
			      OWEN_COLUMN, holiday->owen,
			      DAVE_COLUMN, holiday->dave,
			      VISIBLE_COLUMN, TRUE,
			      WORLD_COLUMN, holiday->world_holiday,
			      -1);

	  holiday++;
	}

      month++;
    }

  return BTK_TREE_MODEL (model);
}

static void
item_toggled (BtkCellRendererToggle *cell,
	      gchar                 *path_str,
	      gpointer               data)
{
  BtkTreeModel *model = (BtkTreeModel *)data;
  BtkTreePath *path = btk_tree_path_new_from_string (path_str);
  BtkTreeIter iter;
  gboolean toggle_item;

  gint *column;

  column = g_object_get_data (B_OBJECT (cell), "column");

  /* get toggled iter */
  btk_tree_model_get_iter (model, &iter, path);
  btk_tree_model_get (model, &iter, column, &toggle_item, -1);

  /* do something with the value */
  toggle_item ^= 1;

  /* set new value */
  btk_tree_store_set (BTK_TREE_STORE (model), &iter, column,
		      toggle_item, -1);

  /* clean up */
  btk_tree_path_free (path);
}

static void
add_columns (BtkTreeView *treeview)
{
  gint col_offset;
  BtkCellRenderer *renderer;
  BtkTreeViewColumn *column;
  BtkTreeModel *model = btk_tree_view_get_model (treeview);

  /* column for holiday names */
  renderer = btk_cell_renderer_text_new ();
  g_object_set (renderer, "xalign", 0.0, NULL);

  col_offset = btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (treeview),
							    -1, "Holiday",
							    renderer, "text",
							    HOLIDAY_NAME_COLUMN,
							    NULL);
  column = btk_tree_view_get_column (BTK_TREE_VIEW (treeview), col_offset - 1);
  btk_tree_view_column_set_clickable (BTK_TREE_VIEW_COLUMN (column), TRUE);

  /* alex column */
  renderer = btk_cell_renderer_toggle_new ();
  g_object_set (renderer, "xalign", 0.0, NULL);
  g_object_set_data (B_OBJECT (renderer), "column", (gint *)ALEX_COLUMN);

  g_signal_connect (renderer, "toggled", G_CALLBACK (item_toggled), model);

  col_offset = btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (treeview),
							    -1, "Alex",
							    renderer,
							    "active",
							    ALEX_COLUMN,
							    "visible",
							    VISIBLE_COLUMN,
							    "activatable",
							    WORLD_COLUMN, NULL);

  column = btk_tree_view_get_column (BTK_TREE_VIEW (treeview), col_offset - 1);
  btk_tree_view_column_set_sizing (BTK_TREE_VIEW_COLUMN (column),
				   BTK_TREE_VIEW_COLUMN_FIXED);
  btk_tree_view_column_set_fixed_width (BTK_TREE_VIEW_COLUMN (column), 50);
  btk_tree_view_column_set_clickable (BTK_TREE_VIEW_COLUMN (column), TRUE);

  /* havoc column */
  renderer = btk_cell_renderer_toggle_new ();
  g_object_set (renderer, "xalign", 0.0, NULL);
  g_object_set_data (B_OBJECT (renderer), "column", (gint *)HAVOC_COLUMN);

  g_signal_connect (renderer, "toggled", G_CALLBACK (item_toggled), model);

  col_offset = btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (treeview),
							    -1, "Havoc",
							    renderer,
							    "active",
							    HAVOC_COLUMN,
							    "visible",
							    VISIBLE_COLUMN,
							    NULL);

  column = btk_tree_view_get_column (BTK_TREE_VIEW (treeview), col_offset - 1);
  btk_tree_view_column_set_sizing (BTK_TREE_VIEW_COLUMN (column),
				   BTK_TREE_VIEW_COLUMN_FIXED);
  btk_tree_view_column_set_fixed_width (BTK_TREE_VIEW_COLUMN (column), 50);
  btk_tree_view_column_set_clickable (BTK_TREE_VIEW_COLUMN (column), TRUE);

  /* tim column */
  renderer = btk_cell_renderer_toggle_new ();
  g_object_set (renderer, "xalign", 0.0, NULL);
  g_object_set_data (B_OBJECT (renderer), "column", (gint *)TIM_COLUMN);

  g_signal_connect (renderer, "toggled", G_CALLBACK (item_toggled), model);

  col_offset = btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (treeview),
							    -1, "Tim",
							    renderer,
							    "active",
							    TIM_COLUMN,
							    "visible",
							    VISIBLE_COLUMN,
							    "activatable",
							    WORLD_COLUMN, NULL);

  column = btk_tree_view_get_column (BTK_TREE_VIEW (treeview), col_offset - 1);
  btk_tree_view_column_set_sizing (BTK_TREE_VIEW_COLUMN (column),
				   BTK_TREE_VIEW_COLUMN_FIXED);
  btk_tree_view_column_set_fixed_width (BTK_TREE_VIEW_COLUMN (column), 50);
  btk_tree_view_column_set_clickable (BTK_TREE_VIEW_COLUMN (column), TRUE);

  /* owen column */
  renderer = btk_cell_renderer_toggle_new ();
  g_object_set (renderer, "xalign", 0.0, NULL);
  g_object_set_data (B_OBJECT (renderer), "column", (gint *)OWEN_COLUMN);

  g_signal_connect (renderer, "toggled", G_CALLBACK (item_toggled), model);

  col_offset = btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (treeview),
							    -1, "Owen",
							    renderer,
							    "active",
							    OWEN_COLUMN,
							    "visible",
							    VISIBLE_COLUMN,
							    NULL);

  column = btk_tree_view_get_column (BTK_TREE_VIEW (treeview), col_offset - 1);
  btk_tree_view_column_set_sizing (BTK_TREE_VIEW_COLUMN (column),
				   BTK_TREE_VIEW_COLUMN_FIXED);
  btk_tree_view_column_set_fixed_width (BTK_TREE_VIEW_COLUMN (column), 50);
  btk_tree_view_column_set_clickable (BTK_TREE_VIEW_COLUMN (column), TRUE);

  /* dave column */
  renderer = btk_cell_renderer_toggle_new ();
  g_object_set (renderer, "xalign", 0.0, NULL);
  g_object_set_data (B_OBJECT (renderer), "column", (gint *)DAVE_COLUMN);

  g_signal_connect (renderer, "toggled", G_CALLBACK (item_toggled), model);

  col_offset = btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (treeview),
							    -1, "Dave",
							    renderer,
							    "active",
							    DAVE_COLUMN,
							    "visible",
							    VISIBLE_COLUMN,
							    NULL);

  column = btk_tree_view_get_column (BTK_TREE_VIEW (treeview), col_offset - 1);
  btk_tree_view_column_set_sizing (BTK_TREE_VIEW_COLUMN (column),
				   BTK_TREE_VIEW_COLUMN_FIXED);
  btk_tree_view_column_set_fixed_width (BTK_TREE_VIEW_COLUMN (column), 50);
  btk_tree_view_column_set_clickable (BTK_TREE_VIEW_COLUMN (column), TRUE);
}

BtkWidget *
do_tree_store (BtkWidget *do_widget)
{
  if (!window)
    {
      BtkWidget *vbox;
      BtkWidget *sw;
      BtkWidget *treeview;
      BtkTreeModel *model;

      /* create window, etc */
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (do_widget));
      btk_window_set_title (BTK_WINDOW (window), "Card planning sheet");
      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed), &window);

      vbox = btk_vbox_new (FALSE, 8);
      btk_container_set_border_width (BTK_CONTAINER (vbox), 8);
      btk_container_add (BTK_CONTAINER (window), vbox);

      btk_box_pack_start (BTK_BOX (vbox),
			  btk_label_new ("Jonathan's Holiday Card Planning Sheet"),
			  FALSE, FALSE, 0);

      sw = btk_scrolled_window_new (NULL, NULL);
      btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (sw),
					   BTK_SHADOW_ETCHED_IN);
      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (sw),
				      BTK_POLICY_AUTOMATIC,
				      BTK_POLICY_AUTOMATIC);
      btk_box_pack_start (BTK_BOX (vbox), sw, TRUE, TRUE, 0);

      /* create model */
      model = create_model ();

      /* create tree view */
      treeview = btk_tree_view_new_with_model (model);
      g_object_unref (model);
      btk_tree_view_set_rules_hint (BTK_TREE_VIEW (treeview), TRUE);
      btk_tree_selection_set_mode (btk_tree_view_get_selection (BTK_TREE_VIEW (treeview)),
				   BTK_SELECTION_MULTIPLE);

      add_columns (BTK_TREE_VIEW (treeview));

      btk_container_add (BTK_CONTAINER (sw), treeview);

      /* expand all rows after the treeview widget has been realized */
      g_signal_connect (treeview, "realize",
			G_CALLBACK (btk_tree_view_expand_all), NULL);
      btk_window_set_default_size (BTK_WINDOW (window), 650, 400);
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
