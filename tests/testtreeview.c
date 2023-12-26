/* testtreeview.c
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

#undef BTK_DISABLE_DEPRECATED
#include <string.h>
#include "prop-editor.h"
#include <btk/btk.h>
#include <stdlib.h>

/* Don't copy this bad example; inline RGB data is always a better
 * idea than inline XPMs.
 */
static char  *book_closed_xpm[] = {
"16 16 6 1",
"       c None s None",
".      c black",
"X      c red",
"o      c yellow",
"O      c #808080",
"#      c white",
"                ",
"       ..       ",
"     ..XX.      ",
"   ..XXXXX.     ",
" ..XXXXXXXX.    ",
".ooXXXXXXXXX.   ",
"..ooXXXXXXXXX.  ",
".X.ooXXXXXXXXX. ",
".XX.ooXXXXXX..  ",
" .XX.ooXXX..#O  ",
"  .XX.oo..##OO. ",
"   .XX..##OO..  ",
"    .X.#OO..    ",
"     ..O..      ",
"      ..        ",
"                "
};

static void run_automated_tests (void);

/* This custom model is to test custom model use. */

#define BTK_TYPE_MODEL_TYPES				(btk_tree_model_types_get_type ())
#define BTK_TREE_MODEL_TYPES(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_MODEL_TYPES, BtkTreeModelTypes))
#define BTK_TREE_MODEL_TYPES_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_MODEL_TYPES, BtkTreeModelTypesClass))
#define BTK_IS_TREE_MODEL_TYPES(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_MODEL_TYPES))
#define BTK_IS_TREE_MODEL_TYPES_GET_CLASS(klass)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_MODEL_TYPES))

typedef struct _BtkTreeModelTypes       BtkTreeModelTypes;
typedef struct _BtkTreeModelTypesClass  BtkTreeModelTypesClass;

struct _BtkTreeModelTypes
{
  GObject parent;

  gint stamp;
};

struct _BtkTreeModelTypesClass
{
  GObjectClass parent_class;

  guint        (* get_flags)       (BtkTreeModel *tree_model);   
  gint         (* get_n_columns)   (BtkTreeModel *tree_model);
  GType        (* get_column_type) (BtkTreeModel *tree_model,
				    gint          index);
  gboolean     (* get_iter)        (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter,
				    BtkTreePath  *path);
  BtkTreePath *(* get_path)        (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter);
  void         (* get_value)       (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter,
				    gint          column,
				    GValue       *value);
  gboolean     (* iter_next)       (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter);
  gboolean     (* iter_children)   (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter,
				    BtkTreeIter  *parent);
  gboolean     (* iter_has_child)  (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter);
  gint         (* iter_n_children) (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter);
  gboolean     (* iter_nth_child)  (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter,
				    BtkTreeIter  *parent,
				    gint          n);
  gboolean     (* iter_parent)     (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter,
				    BtkTreeIter  *child);
  void         (* ref_iter)        (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter);
  void         (* unref_iter)      (BtkTreeModel *tree_model,
				    BtkTreeIter  *iter);

  /* These will be moved into the BtkTreeModelIface eventually */
  void         (* changed)         (BtkTreeModel *tree_model,
				    BtkTreePath  *path,
				    BtkTreeIter  *iter);
  void         (* inserted)        (BtkTreeModel *tree_model,
				    BtkTreePath  *path,
				    BtkTreeIter  *iter);
  void         (* child_toggled)   (BtkTreeModel *tree_model,
				    BtkTreePath  *path,
				    BtkTreeIter  *iter);
  void         (* deleted)         (BtkTreeModel *tree_model,
				    BtkTreePath  *path);
};

GType              btk_tree_model_types_get_type      (void) B_GNUC_CONST;
BtkTreeModelTypes *btk_tree_model_types_new           (void);

typedef enum
{
  COLUMNS_NONE,
  COLUMNS_ONE,
  COLUMNS_LOTS,
  COLUMNS_LAST
} ColumnsType;

static gchar *column_type_names[] = {
  "No columns",
  "One column",
  "Many columns"
};

#define N_COLUMNS 9

static GType*
get_model_types (void)
{
  static GType column_types[N_COLUMNS] = { 0 };
  
  if (column_types[0] == 0)
    {
      column_types[0] = G_TYPE_STRING;
      column_types[1] = G_TYPE_STRING;
      column_types[2] = BDK_TYPE_PIXBUF;
      column_types[3] = G_TYPE_FLOAT;
      column_types[4] = G_TYPE_UINT;
      column_types[5] = G_TYPE_UCHAR;
      column_types[6] = G_TYPE_CHAR;
#define BOOL_COLUMN 7
      column_types[BOOL_COLUMN] = G_TYPE_BOOLEAN;
      column_types[8] = G_TYPE_INT;
    }

  return column_types;
}

static void
col_clicked_cb (BtkTreeViewColumn *col, gpointer data)
{
  BtkWindow *win;

  win = BTK_WINDOW (create_prop_editor (G_OBJECT (col), BTK_TYPE_TREE_VIEW_COLUMN));

  btk_window_set_title (win, btk_tree_view_column_get_title (col));
}

static void
setup_column (BtkTreeViewColumn *col)
{
  btk_tree_view_column_set_clickable (col, TRUE);
  g_signal_connect (col,
		    "clicked",
		    G_CALLBACK (col_clicked_cb),
		    NULL);
}

static void
toggled_callback (BtkCellRendererToggle *celltoggle,
                  gchar                 *path_string,
                  BtkTreeView           *tree_view)
{
  BtkTreeModel *model = NULL;
  BtkTreeModelSort *sort_model = NULL;
  BtkTreePath *path;
  BtkTreeIter iter;
  gboolean active = FALSE;
  
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  model = btk_tree_view_get_model (tree_view);
  
  if (BTK_IS_TREE_MODEL_SORT (model))
    {
      sort_model = BTK_TREE_MODEL_SORT (model);
      model = btk_tree_model_sort_get_model (sort_model);
    }

  if (model == NULL)
    return;

  if (sort_model)
    {
      g_warning ("FIXME implement conversion from TreeModelSort iter to child model iter");
      return;
    }
      
  path = btk_tree_path_new_from_string (path_string);
  if (!btk_tree_model_get_iter (model,
                                &iter, path))
    {
      g_warning ("%s: bad path?", B_STRLOC);
      return;
    }
  btk_tree_path_free (path);
  
  if (BTK_IS_LIST_STORE (model))
    {
      btk_tree_model_get (BTK_TREE_MODEL (model),
                          &iter,
                          BOOL_COLUMN,
                          &active,
                          -1);
      
      btk_list_store_set (BTK_LIST_STORE (model),
                          &iter,
                          BOOL_COLUMN,
                          !active,
                          -1);
    }
  else if (BTK_IS_TREE_STORE (model))
    {
      btk_tree_model_get (BTK_TREE_MODEL (model),
                          &iter,
                          BOOL_COLUMN,
                          &active,
                          -1);
            
      btk_tree_store_set (BTK_TREE_STORE (model),
                          &iter,
                          BOOL_COLUMN,
                          !active,
                          -1);
    }
  else
    g_warning ("don't know how to actually toggle value for model type %s",
               g_type_name (G_TYPE_FROM_INSTANCE (model)));
}

static void
edited_callback (BtkCellRendererText *renderer,
		 const gchar   *path_string,
		 const gchar   *new_text,
		 BtkTreeView  *tree_view)
{
  BtkTreeModel *model = NULL;
  BtkTreeModelSort *sort_model = NULL;
  BtkTreePath *path;
  BtkTreeIter iter;
  guint value = atoi (new_text);
  
  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));

  model = btk_tree_view_get_model (tree_view);
  
  if (BTK_IS_TREE_MODEL_SORT (model))
    {
      sort_model = BTK_TREE_MODEL_SORT (model);
      model = btk_tree_model_sort_get_model (sort_model);
    }

  if (model == NULL)
    return;

  if (sort_model)
    {
      g_warning ("FIXME implement conversion from TreeModelSort iter to child model iter");
      return;
    }
      
  path = btk_tree_path_new_from_string (path_string);
  if (!btk_tree_model_get_iter (model,
                                &iter, path))
    {
      g_warning ("%s: bad path?", B_STRLOC);
      return;
    }
  btk_tree_path_free (path);

  if (BTK_IS_LIST_STORE (model))
    {
      btk_list_store_set (BTK_LIST_STORE (model),
                          &iter,
                          4,
                          value,
                          -1);
    }
  else if (BTK_IS_TREE_STORE (model))
    {
      btk_tree_store_set (BTK_TREE_STORE (model),
                          &iter,
                          4,
                          value,
                          -1);
    }
  else
    g_warning ("don't know how to actually toggle value for model type %s",
               g_type_name (G_TYPE_FROM_INSTANCE (model)));
}

static ColumnsType current_column_type = COLUMNS_LOTS;

static void
set_columns_type (BtkTreeView *tree_view, ColumnsType type)
{
  BtkTreeViewColumn *col;
  BtkCellRenderer *rend;
  BdkPixbuf *pixbuf;
  BtkWidget *image;
  BtkObject *adjustment;
    
  current_column_type = type;
  
  col = btk_tree_view_get_column (tree_view, 0);
  while (col)
    {
      btk_tree_view_remove_column (tree_view, col);

      col = btk_tree_view_get_column (tree_view, 0);
    }

  btk_tree_view_set_rules_hint (tree_view, FALSE);
  
  switch (type)
    {
    case COLUMNS_NONE:
      break;

    case COLUMNS_LOTS:
      /* with lots of columns we need to turn on rules */
      btk_tree_view_set_rules_hint (tree_view, TRUE);
      
      rend = btk_cell_renderer_text_new ();
      
      col = btk_tree_view_column_new_with_attributes ("Column 1",
                                                      rend,
                                                      "text", 1,
                                                      NULL);
      setup_column (col);
      
      btk_tree_view_append_column (BTK_TREE_VIEW (tree_view), col);
      
      col = btk_tree_view_column_new();
      btk_tree_view_column_set_title (col, "Column 2");
      
      rend = btk_cell_renderer_pixbuf_new ();
      btk_tree_view_column_pack_start (col, rend, FALSE);
      btk_tree_view_column_add_attribute (col, rend, "pixbuf", 2);
      rend = btk_cell_renderer_text_new ();
      btk_tree_view_column_pack_start (col, rend, TRUE);
      btk_tree_view_column_add_attribute (col, rend, "text", 0);

      setup_column (col);
      
      
      btk_tree_view_append_column (BTK_TREE_VIEW (tree_view), col);
      btk_tree_view_set_expander_column (tree_view, col);
      
      rend = btk_cell_renderer_toggle_new ();

      g_signal_connect (rend, "toggled",
			G_CALLBACK (toggled_callback), tree_view);
      
      col = btk_tree_view_column_new_with_attributes ("Column 3",
                                                      rend,
                                                      "active", BOOL_COLUMN,
                                                      NULL);

      setup_column (col);
      
      btk_tree_view_append_column (BTK_TREE_VIEW (tree_view), col);

      pixbuf = bdk_pixbuf_new_from_xpm_data ((const char **)book_closed_xpm);

      image = btk_image_new_from_pixbuf (pixbuf);

      g_object_unref (pixbuf);
      
      btk_widget_show (image);
      
      btk_tree_view_column_set_widget (col, image);
      
      rend = btk_cell_renderer_toggle_new ();

      /* you could also set this per-row by tying it to a column
       * in the model of course.
       */
      g_object_set (rend, "radio", TRUE, NULL);
      
      g_signal_connect (rend, "toggled",
			G_CALLBACK (toggled_callback), tree_view);
      
      col = btk_tree_view_column_new_with_attributes ("Column 4",
                                                      rend,
                                                      "active", BOOL_COLUMN,
                                                      NULL);

      setup_column (col);
      
      btk_tree_view_append_column (BTK_TREE_VIEW (tree_view), col);

      rend = btk_cell_renderer_spin_new ();

      adjustment = btk_adjustment_new (0, 0, 10000, 100, 100, 100);
      g_object_set (rend, "editable", TRUE, NULL);
      g_object_set (rend, "adjustment", adjustment, NULL);

      g_signal_connect (rend, "edited",
			G_CALLBACK (edited_callback), tree_view);

      col = btk_tree_view_column_new_with_attributes ("Column 5",
                                                      rend,
                                                      "text", 4,
                                                      NULL);

      setup_column (col);
      
      btk_tree_view_append_column (BTK_TREE_VIEW (tree_view), col);
#if 0
      
      rend = btk_cell_renderer_text_new ();
      
      col = btk_tree_view_column_new_with_attributes ("Column 6",
                                                      rend,
                                                      "text", 4,
                                                      NULL);

      setup_column (col);
      
      btk_tree_view_append_column (BTK_TREE_VIEW (tree_view), col);
      
      rend = btk_cell_renderer_text_new ();
      
      col = btk_tree_view_column_new_with_attributes ("Column 7",
                                                      rend,
                                                      "text", 5,
                                                      NULL);

      setup_column (col);
      
      btk_tree_view_append_column (BTK_TREE_VIEW (tree_view), col);
      
      rend = btk_cell_renderer_text_new ();
      
      col = btk_tree_view_column_new_with_attributes ("Column 8",
                                                      rend,
                                                      "text", 6,
                                                      NULL);

      setup_column (col);
      
      btk_tree_view_append_column (BTK_TREE_VIEW (tree_view), col);
      
      rend = btk_cell_renderer_text_new ();
      
      col = btk_tree_view_column_new_with_attributes ("Column 9",
                                                      rend,
                                                      "text", 7,
                                                      NULL);

      setup_column (col);
      
      btk_tree_view_append_column (BTK_TREE_VIEW (tree_view), col);
      
      rend = btk_cell_renderer_text_new ();
      
      col = btk_tree_view_column_new_with_attributes ("Column 10",
                                                      rend,
                                                      "text", 8,
                                                      NULL);

      setup_column (col);
      
      btk_tree_view_append_column (BTK_TREE_VIEW (tree_view), col);
      
#endif
      
      /* FALL THRU */
      
    case COLUMNS_ONE:
      rend = btk_cell_renderer_text_new ();
      
      col = btk_tree_view_column_new_with_attributes ("Column 0",
                                                      rend,
                                                      "text", 0,
                                                      NULL);

      setup_column (col);
      
      btk_tree_view_insert_column (BTK_TREE_VIEW (tree_view), col, 0);
    default:
      break;
    }
}

static ColumnsType
get_columns_type (void)
{
  return current_column_type;
}

static BdkPixbuf *our_pixbuf;
  
typedef enum
{
  /*   MODEL_TYPES, */
  MODEL_TREE,
  MODEL_LIST,
  MODEL_SORTED_TREE,
  MODEL_SORTED_LIST,
  MODEL_EMPTY_LIST,
  MODEL_EMPTY_TREE,
  MODEL_NULL,
  MODEL_LAST
} ModelType;

/* FIXME add a custom model to test */
static BtkTreeModel *models[MODEL_LAST];
static const char *model_names[MODEL_LAST] = {
  "BtkTreeStore",
  "BtkListStore",
  "BtkTreeModelSort wrapping BtkTreeStore",
  "BtkTreeModelSort wrapping BtkListStore",
  "Empty BtkListStore",
  "Empty BtkTreeStore",
  "NULL (no model)"
};

static BtkTreeModel*
create_list_model (void)
{
  BtkListStore *store;
  BtkTreeIter iter;
  gint i;
  GType *t;

  t = get_model_types ();
  
  store = btk_list_store_new (N_COLUMNS,
			      t[0], t[1], t[2],
			      t[3], t[4], t[5],
			      t[6], t[7], t[8]);

  i = 0;
  while (i < 200)
    {
      char *msg;
      
      btk_list_store_append (store, &iter);

      msg = g_strdup_printf ("%d", i);
      
      btk_list_store_set (store, &iter, 0, msg, 1, "Foo! Foo! Foo!",
                          2, our_pixbuf,
                          3, 7.0, 4, (guint) 9000,
                          5, 'f', 6, 'g',
                          7, TRUE, 8, 23245454,
                          -1);

      g_free (msg);
      
      ++i;
    }

  return BTK_TREE_MODEL (store);
}

static void
typesystem_recurse (GType        type,
                    BtkTreeIter *parent_iter,
                    BtkTreeStore *store)
{
  GType* children;
  guint n_children = 0;
  gint i;
  BtkTreeIter iter;
  gchar *str;
  
  btk_tree_store_append (store, &iter, parent_iter);

  str = g_strdup_printf ("%ld", (glong)type);
  btk_tree_store_set (store, &iter, 0, str, 1, g_type_name (type),
                      2, our_pixbuf,
                      3, 7.0, 4, (guint) 9000,
                      5, 'f', 6, 'g',
                      7, TRUE, 8, 23245454,
                      -1);
  g_free (str);
  
  children = g_type_children (type, &n_children);

  i = 0;
  while (i < n_children)
    {
      typesystem_recurse (children[i], &iter, store);

      ++i;
    }
  
  g_free (children);
}

static BtkTreeModel*
create_tree_model (void)
{
  BtkTreeStore *store;
  gint i;
  GType *t;
  volatile GType dummy; /* B_GNUC_CONST makes the optimizer remove
                         * get_type calls if you don't do something
                         * like this
                         */
  
  /* Make the tree more interesting */
  dummy = btk_scrolled_window_get_type ();
  dummy = btk_label_get_type ();
  dummy = btk_hscrollbar_get_type ();
  dummy = btk_vscrollbar_get_type ();
  dummy = bango_layout_get_type ();

  t = get_model_types ();
  
  store = btk_tree_store_new (N_COLUMNS,
			      t[0], t[1], t[2],
			      t[3], t[4], t[5],
			      t[6], t[7], t[8]);

  i = 0;
  while (i < G_TYPE_FUNDAMENTAL_MAX)
    {
      typesystem_recurse (i, NULL, store);
      
      ++i;
    }

  return BTK_TREE_MODEL (store);
}

static void
model_selected (BtkComboBox *combo_box, gpointer data)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (data);
  gint hist;

  hist = btk_combo_box_get_active (combo_box);

  if (models[hist] != btk_tree_view_get_model (tree_view))
    {
      btk_tree_view_set_model (tree_view, models[hist]);
    }
}

static void
columns_selected (BtkComboBox *combo_box, gpointer data)
{
  BtkTreeView *tree_view = BTK_TREE_VIEW (data);
  gint hist;

  hist = btk_combo_box_get_active (combo_box);

  if (hist != get_columns_type ())
    {
      set_columns_type (tree_view, hist);
    }
}


enum
{
  TARGET_BTK_TREE_MODEL_ROW
};

static BtkTargetEntry row_targets[] = {
  { "BTK_TREE_MODEL_ROW", BTK_TARGET_SAME_APP,
    TARGET_BTK_TREE_MODEL_ROW }
};

int
main (int    argc,
      char **argv)
{
  BtkWidget *window;
  BtkWidget *sw;
  BtkWidget *tv;
  BtkWidget *table;
  BtkWidget *combo_box;
  BtkTreeModel *model;
  gint i;
  
  btk_init (&argc, &argv);

  our_pixbuf = bdk_pixbuf_new_from_xpm_data ((const char **) book_closed_xpm);  
  
#if 0
  models[MODEL_TYPES] = BTK_TREE_MODEL (btk_tree_model_types_new ());
#endif
  models[MODEL_LIST] = create_list_model ();
  models[MODEL_TREE] = create_tree_model ();

  model = create_list_model ();
  models[MODEL_SORTED_LIST] = btk_tree_model_sort_new_with_model (model);
  g_object_unref (model);

  model = create_tree_model ();
  models[MODEL_SORTED_TREE] = btk_tree_model_sort_new_with_model (model);
  g_object_unref (model);

  models[MODEL_EMPTY_LIST] = BTK_TREE_MODEL (btk_list_store_new (1, G_TYPE_INT));
  models[MODEL_EMPTY_TREE] = BTK_TREE_MODEL (btk_tree_store_new (1, G_TYPE_INT));
  
  models[MODEL_NULL] = NULL;

  run_automated_tests ();
  
  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy", G_CALLBACK (btk_main_quit), NULL);
  btk_window_set_default_size (BTK_WINDOW (window), 430, 400);

  table = btk_table_new (3, 1, FALSE);

  btk_container_add (BTK_CONTAINER (window), table);

  tv = btk_tree_view_new_with_model (models[0]);
  
  btk_tree_view_enable_model_drag_source (BTK_TREE_VIEW (tv),
					  BDK_BUTTON1_MASK,
					  row_targets,
					  G_N_ELEMENTS (row_targets),
					  BDK_ACTION_MOVE | BDK_ACTION_COPY);

  btk_tree_view_enable_model_drag_dest (BTK_TREE_VIEW (tv),
					row_targets,
					G_N_ELEMENTS (row_targets),
					BDK_ACTION_MOVE | BDK_ACTION_COPY);
  
  /* Model menu */
  combo_box = btk_combo_box_text_new ();
  for (i = 0; i < MODEL_LAST; i++)
      btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo_box), model_names[i]);

  btk_table_attach (BTK_TABLE (table), combo_box,
                    0, 1, 0, 1,
                    0, 0, 
                    0, 0);

  g_signal_connect (combo_box,
                    "changed",
                    G_CALLBACK (model_selected),
		    tv);
  
  /* Columns menu */
  combo_box = btk_combo_box_text_new ();
  for (i = 0; i < COLUMNS_LAST; i++)
      btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo_box), column_type_names[i]);

  btk_table_attach (BTK_TABLE (table), combo_box,
                    0, 1, 1, 2,
                    0, 0, 
                    0, 0);

  set_columns_type (BTK_TREE_VIEW (tv), COLUMNS_LOTS);
  btk_combo_box_set_active (BTK_COMBO_BOX (combo_box), COLUMNS_LOTS);
  
  g_signal_connect (combo_box,
                    "changed",
                    G_CALLBACK (columns_selected),
                    tv);
  
  sw = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (sw),
                                  BTK_POLICY_AUTOMATIC,
                                  BTK_POLICY_AUTOMATIC);
  
  btk_table_attach (BTK_TABLE (table), sw,
                    0, 1, 2, 3,
                    BTK_EXPAND | BTK_FILL,
                    BTK_EXPAND | BTK_FILL,
                    0, 0);
  
  btk_container_add (BTK_CONTAINER (sw), tv);
  
  btk_widget_show_all (window);
  
  btk_main ();

  return 0;
}

/*
 * BtkTreeModelTypes
 */

static void         btk_tree_model_types_init                 (BtkTreeModelTypes      *model_types);
static void         btk_tree_model_types_tree_model_init      (BtkTreeModelIface   *iface);
static gint         btk_real_model_types_get_n_columns   (BtkTreeModel        *tree_model);
static GType        btk_real_model_types_get_column_type (BtkTreeModel        *tree_model,
							   gint                 index);
static BtkTreePath *btk_real_model_types_get_path        (BtkTreeModel        *tree_model,
							   BtkTreeIter         *iter);
static void         btk_real_model_types_get_value       (BtkTreeModel        *tree_model,
							   BtkTreeIter         *iter,
							   gint                 column,
							   GValue              *value);
static gboolean     btk_real_model_types_iter_next       (BtkTreeModel        *tree_model,
							   BtkTreeIter         *iter);
static gboolean     btk_real_model_types_iter_children   (BtkTreeModel        *tree_model,
							   BtkTreeIter         *iter,
							   BtkTreeIter         *parent);
static gboolean     btk_real_model_types_iter_has_child  (BtkTreeModel        *tree_model,
							   BtkTreeIter         *iter);
static gint         btk_real_model_types_iter_n_children (BtkTreeModel        *tree_model,
							   BtkTreeIter         *iter);
static gboolean     btk_real_model_types_iter_nth_child  (BtkTreeModel        *tree_model,
							   BtkTreeIter         *iter,
							   BtkTreeIter         *parent,
							   gint                 n);
static gboolean     btk_real_model_types_iter_parent     (BtkTreeModel        *tree_model,
							   BtkTreeIter         *iter,
							   BtkTreeIter         *child);


GType
btk_tree_model_types_get_type (void)
{
  static GType model_types_type = 0;

  if (!model_types_type)
    {
      const GTypeInfo model_types_info =
      {
        sizeof (BtkTreeModelTypesClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
        NULL,           /* class_init */
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (BtkTreeModelTypes),
	0,
        (GInstanceInitFunc) btk_tree_model_types_init
      };

      const GInterfaceInfo tree_model_info =
      {
	(GInterfaceInitFunc) btk_tree_model_types_tree_model_init,
	NULL,
	NULL
      };

      model_types_type = g_type_register_static (G_TYPE_OBJECT,
						 "BtkTreeModelTypes",
						 &model_types_info, 0);
      g_type_add_interface_static (model_types_type,
				   BTK_TYPE_TREE_MODEL,
				   &tree_model_info);
    }

  return model_types_type;
}

BtkTreeModelTypes *
btk_tree_model_types_new (void)
{
  BtkTreeModelTypes *retval;

  retval = g_object_new (BTK_TYPE_MODEL_TYPES, NULL);

  return retval;
}

static void
btk_tree_model_types_tree_model_init (BtkTreeModelIface *iface)
{
  iface->get_n_columns = btk_real_model_types_get_n_columns;
  iface->get_column_type = btk_real_model_types_get_column_type;
  iface->get_path = btk_real_model_types_get_path;
  iface->get_value = btk_real_model_types_get_value;
  iface->iter_next = btk_real_model_types_iter_next;
  iface->iter_children = btk_real_model_types_iter_children;
  iface->iter_has_child = btk_real_model_types_iter_has_child;
  iface->iter_n_children = btk_real_model_types_iter_n_children;
  iface->iter_nth_child = btk_real_model_types_iter_nth_child;
  iface->iter_parent = btk_real_model_types_iter_parent;
}

static void
btk_tree_model_types_init (BtkTreeModelTypes *model_types)
{
  model_types->stamp = g_random_int ();
}

static GType column_types[] = {
  G_TYPE_STRING, /* GType */
  G_TYPE_STRING  /* type name */
};
  
static gint
btk_real_model_types_get_n_columns (BtkTreeModel *tree_model)
{
  return G_N_ELEMENTS (column_types);
}

static GType
btk_real_model_types_get_column_type (BtkTreeModel *tree_model,
                                      gint          index)
{
  g_return_val_if_fail (index < G_N_ELEMENTS (column_types), G_TYPE_INVALID);
  
  return column_types[index];
}

#if 0
/* Use default implementation of this */
static gboolean
btk_real_model_types_get_iter (BtkTreeModel *tree_model,
                               BtkTreeIter  *iter,
                               BtkTreePath  *path)
{
  
}
#endif

/* The toplevel nodes of the tree are the reserved types, G_TYPE_NONE through
 * G_TYPE_RESERVED_FUNDAMENTAL.
 */

static BtkTreePath *
btk_real_model_types_get_path (BtkTreeModel *tree_model,
                               BtkTreeIter  *iter)
{
  BtkTreePath *retval;
  GType type;
  GType parent;
  
  g_return_val_if_fail (BTK_IS_TREE_MODEL_TYPES (tree_model), NULL);
  g_return_val_if_fail (iter != NULL, NULL);

  type = GPOINTER_TO_INT (iter->user_data);
  
  retval = btk_tree_path_new ();
  
  parent = g_type_parent (type);
  while (parent != G_TYPE_INVALID)
    {
      GType* children = g_type_children (parent, NULL);
      gint i = 0;

      if (!children || children[0] == G_TYPE_INVALID)
        {
          g_warning ("bad iterator?");
          return NULL;
        }
      
      while (children[i] != type)
        ++i;

      btk_tree_path_prepend_index (retval, i);

      g_free (children);
      
      type = parent;
      parent = g_type_parent (parent);
    }

  /* The fundamental type itself is the index on the toplevel */
  btk_tree_path_prepend_index (retval, type);

  return retval;
}

static void
btk_real_model_types_get_value (BtkTreeModel *tree_model,
                                BtkTreeIter  *iter,
                                gint          column,
                                GValue       *value)
{
  GType type;

  type = GPOINTER_TO_INT (iter->user_data);

  switch (column)
    {
    case 0:
      {
        gchar *str;
        
        g_value_init (value, G_TYPE_STRING);

        str = g_strdup_printf ("%ld", (long int) type);
        g_value_set_string (value, str);
        g_free (str);
      }
      break;

    case 1:
      g_value_init (value, G_TYPE_STRING);
      g_value_set_string (value, g_type_name (type));
      break;

    default:
      g_warning ("Bad column %d requested", column);
    }
}

static gboolean
btk_real_model_types_iter_next (BtkTreeModel  *tree_model,
                                BtkTreeIter   *iter)
{
  
  GType parent;
  GType type;

  type = GPOINTER_TO_INT (iter->user_data);

  parent = g_type_parent (type);
  
  if (parent == G_TYPE_INVALID)
    {
      /* find next _valid_ fundamental type */
      do
	type++;
      while (!g_type_name (type) && type <= G_TYPE_FUNDAMENTAL_MAX);
      if (type <= G_TYPE_FUNDAMENTAL_MAX)
	{
	  /* found one */
          iter->user_data = GINT_TO_POINTER (type);
          return TRUE;
        }
      else
        return FALSE;
    }
  else
    {
      GType* children = g_type_children (parent, NULL);
      gint i = 0;

      g_assert (children != NULL);
      
      while (children[i] != type)
        ++i;
  
      ++i;

      if (children[i] != G_TYPE_INVALID)
        {
          g_free (children);
          iter->user_data = GINT_TO_POINTER (children[i]);
          return TRUE;
        }
      else
        {
          g_free (children);
          return FALSE;
        }
    }
}

static gboolean
btk_real_model_types_iter_children (BtkTreeModel *tree_model,
                                    BtkTreeIter  *iter,
                                    BtkTreeIter  *parent)
{
  GType type;
  GType* children;
  
  type = GPOINTER_TO_INT (parent->user_data);

  children = g_type_children (type, NULL);

  if (!children || children[0] == G_TYPE_INVALID)
    {
      g_free (children);
      return FALSE;
    }
  else
    {
      iter->user_data = GINT_TO_POINTER (children[0]);
      g_free (children);
      return TRUE;
    }
}

static gboolean
btk_real_model_types_iter_has_child (BtkTreeModel *tree_model,
                                     BtkTreeIter  *iter)
{
  GType type;
  GType* children;
  
  type = GPOINTER_TO_INT (iter->user_data);
  
  children = g_type_children (type, NULL);

  if (!children || children[0] == G_TYPE_INVALID)
    {
      g_free (children);
      return FALSE;
    }
  else
    {
      g_free (children);
      return TRUE;
    }
}

static gint
btk_real_model_types_iter_n_children (BtkTreeModel *tree_model,
                                      BtkTreeIter  *iter)
{
  if (iter == NULL)
    {
      return G_TYPE_FUNDAMENTAL_MAX;
    }
  else
    {
      GType type;
      GType* children;
      guint n_children = 0;

      type = GPOINTER_TO_INT (iter->user_data);
      
      children = g_type_children (type, &n_children);
      
      g_free (children);
      
      return n_children;
    }
}

static gboolean
btk_real_model_types_iter_nth_child (BtkTreeModel *tree_model,
                                     BtkTreeIter  *iter,
                                     BtkTreeIter  *parent,
                                     gint          n)
{  
  if (parent == NULL)
    {
      /* fundamental type */
      if (n < G_TYPE_FUNDAMENTAL_MAX)
        {
          iter->user_data = GINT_TO_POINTER (n);
          return TRUE;
        }
      else
        return FALSE;
    }
  else
    {
      GType type = GPOINTER_TO_INT (parent->user_data);      
      guint n_children = 0;
      GType* children = g_type_children (type, &n_children);

      if (n_children == 0)
        {
          g_free (children);
          return FALSE;
        }
      else if (n >= n_children)
        {
          g_free (children);
          return FALSE;
        }
      else
        {
          iter->user_data = GINT_TO_POINTER (children[n]);
          g_free (children);

          return TRUE;
        }
    }
}

static gboolean
btk_real_model_types_iter_parent (BtkTreeModel *tree_model,
                                  BtkTreeIter  *iter,
                                  BtkTreeIter  *child)
{
  GType type;
  GType parent;
  
  type = GPOINTER_TO_INT (child->user_data);
  
  parent = g_type_parent (type);
  
  if (parent == G_TYPE_INVALID)
    {
      if (type > G_TYPE_FUNDAMENTAL_MAX)
        g_warning ("no parent for %ld %s\n",
                   (long int) type,
                   g_type_name (type));
      return FALSE;
    }
  else
    {
      iter->user_data = GINT_TO_POINTER (parent);
      
      return TRUE;
    }
}

/*
 * Automated testing
 */

#if 0

static void
treestore_torture_recurse (BtkTreeStore *store,
                           BtkTreeIter  *root,
                           gint          depth)
{
  BtkTreeModel *model;
  gint i;
  BtkTreeIter iter;  
  
  model = BTK_TREE_MODEL (store);    

  if (depth > 2)
    return;

  ++depth;

  btk_tree_store_append (store, &iter, root);
  
  btk_tree_model_iter_children (model, &iter, root);
  
  i = 0;
  while (i < 100)
    {
      btk_tree_store_append (store, &iter, root);
      ++i;
    }

  while (btk_tree_model_iter_children (model, &iter, root))
    btk_tree_store_remove (store, &iter);

  btk_tree_store_append (store, &iter, root);

  /* inserts before last node in tree */
  i = 0;
  while (i < 100)
    {
      btk_tree_store_insert_before (store, &iter, root, &iter);
      ++i;
    }

  /* inserts after the node before the last node */
  i = 0;
  while (i < 100)
    {
      btk_tree_store_insert_after (store, &iter, root, &iter);
      ++i;
    }

  /* inserts after the last node */
  btk_tree_store_append (store, &iter, root);
    
  i = 0;
  while (i < 100)
    {
      btk_tree_store_insert_after (store, &iter, root, &iter);
      ++i;
    }

  /* remove everything again */
  while (btk_tree_model_iter_children (model, &iter, root))
    btk_tree_store_remove (store, &iter);


    /* Prepends */
  btk_tree_store_prepend (store, &iter, root);
    
  i = 0;
  while (i < 100)
    {
      btk_tree_store_prepend (store, &iter, root);
      ++i;
    }

  /* remove everything again */
  while (btk_tree_model_iter_children (model, &iter, root))
    btk_tree_store_remove (store, &iter);

  btk_tree_store_append (store, &iter, root);
  btk_tree_store_append (store, &iter, root);
  btk_tree_store_append (store, &iter, root);
  btk_tree_store_append (store, &iter, root);

  while (btk_tree_model_iter_children (model, &iter, root))
    {
      treestore_torture_recurse (store, &iter, depth);
      btk_tree_store_remove (store, &iter);
    }
}

#endif

static void
run_automated_tests (void)
{
  g_print ("Running automated tests...\n");
  
  /* FIXME TreePath basic verification */

  /* FIXME generic consistency checks on the models */

  {
    /* Make sure list store mutations don't crash anything */
    BtkListStore *store;
    BtkTreeModel *model;
    gint i;
    BtkTreeIter iter;
    
    store = btk_list_store_new (1, G_TYPE_INT);

    model = BTK_TREE_MODEL (store);
    
    i = 0;
    while (i < 100)
      {
        btk_list_store_append (store, &iter);
        ++i;
      }

    while (btk_tree_model_get_iter_first (model, &iter))
      btk_list_store_remove (store, &iter);

    btk_list_store_append (store, &iter);

    /* inserts before last node in list */
    i = 0;
    while (i < 100)
      {
        btk_list_store_insert_before (store, &iter, &iter);
        ++i;
      }

    /* inserts after the node before the last node */
    i = 0;
    while (i < 100)
      {
        btk_list_store_insert_after (store, &iter, &iter);
        ++i;
      }

    /* inserts after the last node */
    btk_list_store_append (store, &iter);
    
    i = 0;
    while (i < 100)
      {
        btk_list_store_insert_after (store, &iter, &iter);
        ++i;
      }

    /* remove everything again */
    while (btk_tree_model_get_iter_first (model, &iter))
      btk_list_store_remove (store, &iter);


    /* Prepends */
    btk_list_store_prepend (store, &iter);
    
    i = 0;
    while (i < 100)
      {
        btk_list_store_prepend (store, &iter);
        ++i;
      }

    /* remove everything again */
    while (btk_tree_model_get_iter_first (model, &iter))
      btk_list_store_remove (store, &iter);
    
    g_object_unref (store);
  }

  {
    /* Make sure tree store mutations don't crash anything */
    BtkTreeStore *store;
    BtkTreeIter root;

    store = btk_tree_store_new (1, G_TYPE_INT);
    btk_tree_store_append (BTK_TREE_STORE (store), &root, NULL);
    /* Remove test until it is rewritten to work */
    /*    treestore_torture_recurse (store, &root, 0);*/
    
    g_object_unref (store);
  }

  g_print ("Passed.\n");
}
