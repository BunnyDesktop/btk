#include <stdlib.h>
#include <testlib.h>

/*
 * This module is for use with the test program testtreeview
 */
static bboolean state_change_watch  (GSignalInvocationHint *ihint,
                                     buint                  n_param_values,
                                     const BValue          *param_values,
                                     bpointer               data);

static void _check_table             (BatkObject         *in_obj);
static void _check_cell_actions      (BatkObject         *in_obj);

static bint _find_expander_column    (BatkTable          *table);
static void _check_expanders         (BatkTable          *table,
                                      bint              expander_column);
static void _runtest                 (BatkObject         *obj);
static void _create_event_watcher    (void);
static void row_inserted             (BatkObject         *obj,
                                      bint              row,
                                      bint              count);
static void row_deleted              (BatkObject         *obj,
                                      bint              row,
                                      bint              count);
static BatkObject *table_obj = NULL;
static bint expander_column = -1;
static bboolean editing_cell = FALSE;

static bboolean 
state_change_watch (GSignalInvocationHint *ihint,
                    buint                  n_param_values,
                    const BValue          *param_values,
                    bpointer               data)
{
  BObject *object;
  bboolean state_set;
  const bchar *state_name;
  BatkStateType state_type;

  object = b_value_get_object (param_values + 0);
  g_return_val_if_fail (BATK_IS_OBJECT (object), FALSE);

  state_name = b_value_get_string (param_values + 1);
  state_set = b_value_get_boolean (param_values + 2);

  g_print ("Object type: %s state: %s set: %d\n", 
           g_type_name (B_OBJECT_TYPE (object)), state_name, state_set);
  state_type = batk_state_type_for_name (state_name);
  if (state_type == BATK_STATE_EXPANDED)
    {
      BatkObject *parent = batk_object_get_parent (BATK_OBJECT (object));

      if (BATK_IS_TABLE (parent))
        _check_expanders (BATK_TABLE (parent), expander_column);
    }
  return TRUE;
}

static void 
_check_table (BatkObject *in_obj)
{
  BatkObject *obj;
  BatkRole role[2];
  BatkRole obj_role;
  static bboolean emission_hook_added = FALSE;

  if (!emission_hook_added)
    {
      emission_hook_added = TRUE;
      g_signal_add_emission_hook (
                    g_signal_lookup ("state-change", BATK_TYPE_OBJECT),
      /*
       * To specify an emission hook for a particular state, e.g. 
       * BATK_STATE_EXPANDED, instead of 0 use
       * g_quark_from_string (batk_state_type_get_name (BATK_STATE_EXPANDED))
       */  
                    0,
                    state_change_watch, NULL, (GDestroyNotify) NULL);
    }

  role[0] = BATK_ROLE_TABLE;
  role[1] = BATK_ROLE_TREE_TABLE;
  
  g_print ("focus event for: %s\n", g_type_name (B_OBJECT_TYPE (in_obj)));

  _check_cell_actions (in_obj);

  obj_role = batk_object_get_role (in_obj);
  if (obj_role == BATK_ROLE_TABLE_CELL)
    obj = batk_object_get_parent (in_obj);
  else
    obj = find_object_by_role (in_obj, role, 2);
  if (obj == NULL)
    return;

  editing_cell = FALSE;

  if (obj != table_obj)
    {
      table_obj = obj;
      g_print("Found %s table\n", g_type_name (B_OBJECT_TYPE (obj)));
      g_signal_connect (B_OBJECT (obj),
                        "row_inserted",
                        G_CALLBACK (row_inserted),
                        NULL);
      g_signal_connect (B_OBJECT (obj),
                        "row_deleted",
                        G_CALLBACK (row_deleted),
                        NULL);
      /*
       * Find expander column
       */
      if (BATK_IS_TABLE (obj))
        {
          if (batk_object_get_role (obj) == BATK_ROLE_TREE_TABLE)
            {
              expander_column = _find_expander_column (BATK_TABLE (obj));
              if (expander_column == -1)
                {
                  g_warning ("Expander column not found\n");
                  return;
                }
            }
          _runtest(obj);
        }
      else
        g_warning ("Table does not support BatkTable interface\n");
    }
  else 
    {
      /*
       * We do not call these functions at the same time as we set the 
       * signals as the BtkTreeView may not be displayed yet.
       */
      bint x, y, width, height, first_x, last_x, last_y;
      bint first_row, last_row, first_column, last_column;
      bint index;
      BatkObject *first_child, *last_child, *header;
      BatkComponent *component = BATK_COMPONENT (obj);
      BatkTable *table = BATK_TABLE (obj);

      batk_component_get_extents (component, 
                                 &x, &y, &width, &height, BATK_XY_WINDOW);
      g_print ("WINDOW: x: %d y: %d width: %d height: %d\n",
               x, y, width, height);
      batk_component_get_extents (component, 
                                 &x, &y, &width, &height, BATK_XY_SCREEN);
      g_print ("SCREEN: x: %d y: %d width: %d height: %d\n",
               x, y, width, height);
      last_x = x + width - 1;
      last_y = y + height - 1;
      first_x = x; 
      header = batk_table_get_column_header (table, 0);
      if (header)
        {
          batk_component_get_extents (BATK_COMPONENT (header), 
                                     &x, &y, &width, &height, BATK_XY_SCREEN);
          g_print ("Got column header: x: %d y: %d width: %d height: %d\n",
                   x, y, width, height);
          y += height;
        }
      first_child = batk_component_ref_accessible_at_point (component, 
                                             first_x, y, BATK_XY_SCREEN);
      batk_component_get_extents (BATK_COMPONENT (first_child), 
                                 &x, &y, &width, &height, BATK_XY_SCREEN);
      g_print ("first_child: x: %d y: %d width: %d height: %d\n",
               x, y, width, height);
      index = batk_object_get_index_in_parent (BATK_OBJECT (first_child));
      first_row = batk_table_get_row_at_index (table, index);
      first_column = batk_table_get_column_at_index (table, index);
      g_print ("first_row: %d first_column: %d index: %d\n",
               first_row, first_column, index);
      g_assert (index == batk_table_get_index_at (table, first_row, first_column));
      last_child = batk_component_ref_accessible_at_point (component, 
                                     last_x, last_y, BATK_XY_SCREEN);
      if (last_child == NULL)
        {
          /* The TreeView may be bigger than the data */
          bint n_children;

          n_children = batk_object_get_n_accessible_children (obj);
          last_child = batk_object_ref_accessible_child (obj, n_children - 1);
        }
      batk_component_get_extents (BATK_COMPONENT (last_child), 
                                 &x, &y, &width, &height, BATK_XY_SCREEN);
      g_print ("last_child: x: %d y: %d width: %d height: %d\n",
               x, y, width, height);
      index = batk_object_get_index_in_parent (BATK_OBJECT (last_child));
      last_row = batk_table_get_row_at_index (table, index);
      last_column = batk_table_get_column_at_index (table, index);
      g_print ("last_row: %d last_column: %d index: %d\n",
               last_row, last_column, index);
      g_assert (index == batk_table_get_index_at (table, last_row, last_column));
      g_object_unref (first_child);
      g_object_unref (last_child);

      if (expander_column >= 0)
        {
          bint n_rows, i;
          bint x, y, width, height;

          n_rows = batk_table_get_n_rows (table);
          for (i = 0; i < n_rows; i++)
            {
              BatkObject *child_obj;
              BatkStateSet *state_set;
              bboolean showing;

              child_obj = batk_table_ref_at (table, i, expander_column);
              state_set = batk_object_ref_state_set (child_obj);
              showing =  batk_state_set_contains_state (state_set, 
                                                       BATK_STATE_SHOWING);
              g_object_unref (state_set);
              batk_component_get_extents (BATK_COMPONENT (child_obj), 
                                         &x, &y, &width, &height, 
                                         BATK_XY_SCREEN);
              g_object_unref (child_obj);
              if (showing)
                g_print ("Row: %d Column: %d x: %d y: %d width: %d height: %d\n", 
                         i, expander_column, x, y, width, height);
            }
        }
    }
}

static void 
_check_cell_actions (BatkObject *in_obj)
{
  BatkRole role;
  BatkAction *action;
  bint n_actions, i;

  role = batk_object_get_role (in_obj);
  if (role != BATK_ROLE_TABLE_CELL)
    return;

  if (!BATK_IS_ACTION (in_obj))
    return;

  if (editing_cell)
    return;

  action = BATK_ACTION (in_obj);

  n_actions = batk_action_get_n_actions (action);

  for (i = 0; i < n_actions; i++)
    {
      const bchar* name;

      name = batk_action_get_name (action, i);
      g_print ("Action %d is %s\n", i, name);
#if 0
      if (strcmp (name, "edit") == 0)
        {
          editing_cell = TRUE;
          batk_action_do_action (action, i);
        }
#endif
    }
  return;
}

static bint 
_find_expander_column (BatkTable *table)
{
  bint n_columns, i;
  bint retval = -1;

  n_columns = batk_table_get_n_columns (table);
  for (i = 0; i < n_columns; i++)
    {
      BatkObject *cell;
      BatkRelationSet *relation_set;

      cell = batk_table_ref_at (table, 0, i);
      relation_set =  batk_object_ref_relation_set (cell);
      if (batk_relation_set_contains (relation_set, 
                                     BATK_RELATION_NODE_CHILD_OF))
        retval = i;
      g_object_unref (relation_set);
      g_object_unref (cell);
      if (retval >= 0)
        break;
    }
  return retval;
}

static void
_check_expanders (BatkTable *table,
                  bint     expander_column)
{
  bint n_rows, i;

  n_rows = batk_table_get_n_rows (table);

  for (i = 0; i < n_rows; i++)
    {
      BatkObject *cell;
      BatkRelationSet *relation_set;
      BatkRelation *relation;
      GPtrArray *target;
      bint j;

      cell = batk_table_ref_at (table, i, expander_column);

      relation_set =  batk_object_ref_relation_set (cell);
      relation = batk_relation_set_get_relation_by_type (relation_set, 
                                     BATK_RELATION_NODE_CHILD_OF);
      g_assert (relation);
      target = batk_relation_get_target (relation);
      g_assert (target->len == 1);
      for (j = 0; j < target->len; j++)
        {
          BatkObject *target_obj;
          BatkRole role;
          bint target_index, target_row;

          target_obj = g_ptr_array_index (target, j);
          role = batk_object_get_role (target_obj);

          switch (role)
            {
            case BATK_ROLE_TREE_TABLE:
              g_print ("Row %d is top level\n", i);
              break;
            case BATK_ROLE_TABLE_CELL:
              target_index = batk_object_get_index_in_parent (target_obj);
              target_row = batk_table_get_row_at_index (table, target_index);
              g_print ("Row %d has parent at %d\n", i, target_row);
              break;
            default:
              g_assert_not_reached ();
            } 
        }
      g_object_unref (relation_set);
      g_object_unref (cell);
    }
}

static void
_create_event_watcher (void)
{
  batk_add_focus_tracker (_check_table);
}

int
btk_module_init (bint argc, 
                 char *argv[])
{
  g_print ("testtreetable Module loaded\n");

  _create_event_watcher ();

  return 0;
}

static void
_runtest (BatkObject *obj)
{
  BatkObject *child_obj;
  BatkTable *table;
  BatkObject *caption;
  bint i;
  bint n_cols, n_rows, n_children; 

  table = BATK_TABLE (obj);
  n_children = batk_object_get_n_accessible_children (BATK_OBJECT (obj));
  n_cols = batk_table_get_n_columns (table);
  n_rows = batk_table_get_n_rows (table);
  g_print ("n_children: %d n_rows: %d n_cols: %d\n", 
            n_children, n_rows, n_cols);
  
  for (i = 0; i < n_rows; i++)
    {
      bint index = batk_table_get_index_at (table, i, expander_column);
      bint index_in_parent;

      child_obj = batk_table_ref_at (table, i, expander_column);
      index_in_parent = batk_object_get_index_in_parent (child_obj);
      g_print ("index: %d %d row %d column %d\n", index, index_in_parent, i, expander_column);
      
      g_object_unref (child_obj);
    }
  caption = batk_table_get_caption (table);
  if (caption)
    {
      const bchar *caption_name = batk_object_get_name (caption);

      g_print ("Caption: %s\n", caption_name ? caption_name : "<null>");
    }
  for (i = 0; i < n_cols; i++)
    {
      BatkObject *header;

      header = batk_table_get_column_header (table, i);
      g_print ("Header for column %d is %p\n", i, header);
      if (header)
        {
	  const bchar *name;
	  BatkRole role;
          BatkObject *parent;
          BatkObject *child;
          bint index;

          name = batk_object_get_name (header);
          role = batk_object_get_role (header);
          parent = batk_object_get_parent (header);

          if (parent)
            {
              index = batk_object_get_index_in_parent (header);
              g_print ("Parent: %s index: %d\n", B_OBJECT_TYPE_NAME (parent), index);
              child = batk_object_ref_accessible_child (parent, 0);
              g_print ("Child: %s %p\n", B_OBJECT_TYPE_NAME (child), child);
              if (index >= 0)
                {
                  child = batk_object_ref_accessible_child (parent, index);
                  g_print ("Index: %d child: %s\n", index, B_OBJECT_TYPE_NAME (child));
                  g_object_unref (child);
                }
            }
          else
            g_print ("Parent of header is NULL\n");
          g_print ("%s %s %s\n", B_OBJECT_TYPE_NAME (header), name ? name: "<null>", batk_role_get_name (role));
        }
    }
}

static void 
row_inserted (BatkObject *obj,
              bint      row,
              bint      count)
{
#if 0
  BtkWidget *widget;
  BtkTreeView *tree_view;
  BtkTreeModel *tree_model;
#endif
  bint index;

  g_print ("row_inserted: row: %d count: %d\n", row, count);
  index = batk_table_get_index_at (BATK_TABLE (obj), row+1, 0);
  g_print ("index for first column of next row is %d\n", index);

#if 0
  widget = BTK_ACCESSIBLE (obj)->widget;
  tree_view = BTK_TREE_VIEW (widget);
  tree_model = btk_tree_view_get_model (tree_view);
  if (BTK_IS_TREE_STORE (tree_model))
    {
      BtkTreeStore *tree_store;
      BtkTreePath *tree_path;
      BtkTreeIter tree_iter;

      tree_store = BTK_TREE_STORE (tree_model);
      tree_path = btk_tree_path_new_from_string ("3:0");
      btk_tree_model_get_iter (tree_model, &tree_iter, tree_path);
      btk_tree_path_free (tree_path);
      btk_tree_store_remove (tree_store, &tree_iter);
    }
#endif
}

static void 
row_deleted (BatkObject *obj,
             bint      row,
             bint      count)
{
#if 0
  BtkWidget *widget;
  BtkTreeView *tree_view;
  BtkTreeModel *tree_model;
#endif
  bint index;

  g_print ("row_deleted: row: %d count: %d\n", row, count);
  index = batk_table_get_index_at (BATK_TABLE (obj), row+1, 0);
  g_print ("index for first column of next row is %d\n", index);

#if 0
  widget = BTK_ACCESSIBLE (obj)->widget;
  tree_view = BTK_TREE_VIEW (widget);
  tree_model = btk_tree_view_get_model (tree_view);
  if (BTK_IS_TREE_STORE (tree_model))
    {
      BtkTreeStore *tree_store;
      BtkTreePath *tree_path;
      BtkTreeIter tree_iter, new_iter;

      tree_store = BTK_TREE_STORE (tree_model);
      tree_path = btk_tree_path_new_from_string ("2");
      btk_tree_model_get_iter (tree_model, &tree_iter, tree_path);
      btk_tree_path_free (tree_path);
      btk_tree_store_insert_before (tree_store, &new_iter, NULL, &tree_iter);
    }
#endif
}
