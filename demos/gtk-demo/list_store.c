/* Tree View/List Store
 *
 * The BtkListStore is used to store data in list form, to be used
 * later on by a BtkTreeView to display it. This demo builds a
 * simple BtkListStore and displays it. See the Stock Browser
 * demo for a more advanced example.
 *
 */

#include <btk/btk.h>

static BtkWidget *window = NULL;
static BtkTreeModel *model = NULL;
static guint timeout = 0;

typedef struct
{
  const gboolean  fixed;
  const guint     number;
  const gchar    *severity;
  const gchar    *description;
}
Bug;

enum
{
  COLUMN_FIXED,
  COLUMN_NUMBER,
  COLUMN_SEVERITY,
  COLUMN_DESCRIPTION,
  COLUMN_PULSE,
  COLUMN_ACTIVE,
  NUM_COLUMNS
};

static Bug data[] =
{
  { FALSE, 60482, "Normal",     "scrollable notebooks and hidden tabs" },
  { FALSE, 60620, "Critical",   "bdk_window_clear_area (bdkwindow-win32.c) is not thread-safe" },
  { FALSE, 50214, "Major",      "Xft support does not clean up correctly" },
  { TRUE,  52877, "Major",      "BtkFileSelection needs a refresh method. " },
  { FALSE, 56070, "Normal",     "Can't click button after setting in sensitive" },
  { TRUE,  56355, "Normal",     "BtkLabel - Not all changes propagate correctly" },
  { FALSE, 50055, "Normal",     "Rework width/height computations for TreeView" },
  { FALSE, 58278, "Normal",     "btk_dialog_set_response_sensitive () doesn't work" },
  { FALSE, 55767, "Normal",     "Getters for all setters" },
  { FALSE, 56925, "Normal",     "Btkcalender size" },
  { FALSE, 56221, "Normal",     "Selectable label needs right-click copy menu" },
  { TRUE,  50939, "Normal",     "Add shift clicking to BtkTextView" },
  { FALSE, 6112,  "Enhancement","netscape-like collapsable toolbars" },
  { FALSE, 1,     "Normal",     "First bug :=)" },
};

static gboolean
spinner_timeout (gpointer data)
{
  BtkTreeIter iter;
  guint pulse;

  if (model == NULL)
    return FALSE;

  btk_tree_model_get_iter_first (model, &iter);
  btk_tree_model_get (model, &iter,
                      COLUMN_PULSE, &pulse,
                      -1);
  if (pulse == G_MAXUINT)
    pulse = 0;
  else
    pulse++;

  btk_list_store_set (BTK_LIST_STORE (model),
                      &iter,
                      COLUMN_PULSE, pulse,
                      COLUMN_ACTIVE, TRUE,
                      -1);

  return TRUE;
}

static BtkTreeModel *
create_model (void)
{
  gint i = 0;
  BtkListStore *store;
  BtkTreeIter iter;

  /* create list store */
  store = btk_list_store_new (NUM_COLUMNS,
                              G_TYPE_BOOLEAN,
                              G_TYPE_UINT,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_UINT,
                              G_TYPE_BOOLEAN);

  /* add data to the list store */
  for (i = 0; i < G_N_ELEMENTS (data); i++)
    {
      btk_list_store_append (store, &iter);
      btk_list_store_set (store, &iter,
                          COLUMN_FIXED, data[i].fixed,
                          COLUMN_NUMBER, data[i].number,
                          COLUMN_SEVERITY, data[i].severity,
                          COLUMN_DESCRIPTION, data[i].description,
                          COLUMN_PULSE, 0,
                          COLUMN_ACTIVE, FALSE,
                          -1);
    }

  return BTK_TREE_MODEL (store);
}

static void
fixed_toggled (BtkCellRendererToggle *cell,
               gchar                 *path_str,
               gpointer               data)
{
  BtkTreeModel *model = (BtkTreeModel *)data;
  BtkTreeIter  iter;
  BtkTreePath *path = btk_tree_path_new_from_string (path_str);
  gboolean fixed;

  /* get toggled iter */
  btk_tree_model_get_iter (model, &iter, path);
  btk_tree_model_get (model, &iter, COLUMN_FIXED, &fixed, -1);

  /* do something with the value */
  fixed ^= 1;

  /* set new value */
  btk_list_store_set (BTK_LIST_STORE (model), &iter, COLUMN_FIXED, fixed, -1);

  /* clean up */
  btk_tree_path_free (path);
}

static void
add_columns (BtkTreeView *treeview)
{
  BtkCellRenderer *renderer;
  BtkTreeViewColumn *column;
  BtkTreeModel *model = btk_tree_view_get_model (treeview);

  /* column for fixed toggles */
  renderer = btk_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled",
                    G_CALLBACK (fixed_toggled), model);

  column = btk_tree_view_column_new_with_attributes ("Fixed?",
                                                     renderer,
                                                     "active", COLUMN_FIXED,
                                                     NULL);

  /* set this column to a fixed sizing (of 50 pixels) */
  btk_tree_view_column_set_sizing (BTK_TREE_VIEW_COLUMN (column),
                                   BTK_TREE_VIEW_COLUMN_FIXED);
  btk_tree_view_column_set_fixed_width (BTK_TREE_VIEW_COLUMN (column), 50);
  btk_tree_view_append_column (treeview, column);

  /* column for bug numbers */
  renderer = btk_cell_renderer_text_new ();
  column = btk_tree_view_column_new_with_attributes ("Bug number",
                                                     renderer,
                                                     "text",
                                                     COLUMN_NUMBER,
                                                     NULL);
  btk_tree_view_column_set_sort_column_id (column, COLUMN_NUMBER);
  btk_tree_view_append_column (treeview, column);

  /* column for severities */
  renderer = btk_cell_renderer_text_new ();
  column = btk_tree_view_column_new_with_attributes ("Severity",
                                                     renderer,
                                                     "text",
                                                     COLUMN_SEVERITY,
                                                     NULL);
  btk_tree_view_column_set_sort_column_id (column, COLUMN_SEVERITY);
  btk_tree_view_append_column (treeview, column);

  /* column for description */
  renderer = btk_cell_renderer_text_new ();
  column = btk_tree_view_column_new_with_attributes ("Description",
                                                     renderer,
                                                     "text",
                                                     COLUMN_DESCRIPTION,
                                                     NULL);
  btk_tree_view_column_set_sort_column_id (column, COLUMN_DESCRIPTION);
  btk_tree_view_append_column (treeview, column);

  /* column for spinner */
  renderer = btk_cell_renderer_spinner_new ();
  column = btk_tree_view_column_new_with_attributes ("Spinning",
                                                     renderer,
                                                     "pulse",
                                                     COLUMN_PULSE,
                                                     "active",
                                                     COLUMN_ACTIVE,
                                                     NULL);
  btk_tree_view_column_set_sort_column_id (column, COLUMN_PULSE);
  btk_tree_view_append_column (treeview, column);
}

static gboolean
window_closed (BtkWidget *widget,
               BdkEvent  *event,
               gpointer   user_data)
{
  model = NULL;
  window = NULL;
  if (timeout != 0)
    {
      g_source_remove (timeout);
      timeout = 0;
    }
  return FALSE;
}

BtkWidget *
do_list_store (BtkWidget *do_widget)
{
  if (!window)
    {
      BtkWidget *vbox;
      BtkWidget *label;
      BtkWidget *sw;
      BtkWidget *treeview;

      /* create window, etc */
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
                             btk_widget_get_screen (do_widget));
      btk_window_set_title (BTK_WINDOW (window), "BtkListStore demo");

      g_signal_connect (window, "destroy",
                        G_CALLBACK (btk_widget_destroyed), &window);
      btk_container_set_border_width (BTK_CONTAINER (window), 8);

      vbox = btk_vbox_new (FALSE, 8);
      btk_container_add (BTK_CONTAINER (window), vbox);

      label = btk_label_new ("This is the bug list (note: not based on real data, it would be nice to have a nice ODBC interface to bugzilla or so, though).");
      btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);

      sw = btk_scrolled_window_new (NULL, NULL);
      btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (sw),
                                           BTK_SHADOW_ETCHED_IN);
      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (sw),
                                      BTK_POLICY_NEVER,
                                      BTK_POLICY_AUTOMATIC);
      btk_box_pack_start (BTK_BOX (vbox), sw, TRUE, TRUE, 0);

      /* create tree model */
      model = create_model ();

      /* create tree view */
      treeview = btk_tree_view_new_with_model (model);
      btk_tree_view_set_rules_hint (BTK_TREE_VIEW (treeview), TRUE);
      btk_tree_view_set_search_column (BTK_TREE_VIEW (treeview),
                                       COLUMN_DESCRIPTION);

      g_object_unref (model);

      btk_container_add (BTK_CONTAINER (sw), treeview);

      /* add columns to the tree view */
      add_columns (BTK_TREE_VIEW (treeview));

      /* finish & show */
      btk_window_set_default_size (BTK_WINDOW (window), 280, 250);
      g_signal_connect (window, "delete-event",
                        G_CALLBACK (window_closed), NULL);
    }

  if (!btk_widget_get_visible (window))
    {
      btk_widget_show_all (window);
      if (timeout == 0) {
        /* FIXME this should use the animation-duration instead */
        timeout = g_timeout_add (80, spinner_timeout, NULL);
      }
    }
  else
    {
      btk_widget_destroy (window);
      window = NULL;
      if (timeout != 0)
        {
          g_source_remove (timeout);
          timeout = 0;
        }
    }

  return window;
}
