#include <string.h>
#include "testtextlib.h"

#define NUM_ROWS_TO_LOOP 30

typedef struct
{
  BtkWidget *tb_others;
  BtkWidget *tb_ref_selection;
  BtkWidget *tb_ref_at;
  BtkWidget *tb_ref_accessible_child;
  BtkWidget *child_entry;
  BtkWidget *row_entry;
  BtkWidget *col_entry;
  BtkWidget *index_entry;
}TestChoice;

static void _check_table (BatkObject *in_obj);
void table_runtest(BatkObject *);
static void other_runtest(BatkObject *obj);
static void ref_at_runtest(BatkObject *obj, bint row, bint col);
static void ref_accessible_child_runtest(BatkObject *obj, bint childno);
static void ref_selection_runtest (BatkObject *obj, bint index);
static void _selection_tests(BatkObject *obj);
static void _display_header_info(bchar *type,
  BatkObject *header_obj, bint header_num);
static void _process_child(BatkObject *child_obj);

static void _notify_table_row_inserted (BObject *obj,
  bint start_offset, bint length);
static void _notify_table_column_inserted (BObject *obj,
  bint start_offset, bint length);
static void _notify_table_row_deleted (BObject *obj,
  bint start_offset, bint length);
static void _notify_table_column_deleted (BObject *obj,
  bint start_offset, bint length);
static void _notify_table_row_reordered (BObject *obj);
static void _notify_table_column_reordered (BObject *obj);
static void _notify_table_child_added (BObject *obj,
  bint index, BatkObject *child);
static void _notify_table_child_removed (BObject *obj,
  bint index, BatkObject *child);
static void _property_signal_connect (BatkObject	*obj);
static void _property_change_handler (BatkObject	*obj,
  BatkPropertyValues *values);

static bboolean tested_set_headers = FALSE;
static void test_choice_gui (BatkObject **obj);
static void nogui_runtest (BatkObject *obj);
static void nogui_ref_at_runtest (BatkObject *obj);
static void nogui_process_child (BatkObject *obj);

static TestChoice *tc;
static bint g_visibleGUI = 0;
static BatkTable *g_table = NULL;
static BatkObject *current_obj = NULL;
bboolean g_done = FALSE;
bboolean g_properties = TRUE;

/* 
 * destroy
 *
 * Destroy Callback 
 *
 */
static void destroy (BtkWidget *widget, bpointer data)
{
  btk_main_quit();
}

static void choicecb (BtkWidget *widget, bpointer data)
{
  BatkObject **ptr_to_obj = (BatkObject **)data;
  BatkObject *obj = *ptr_to_obj;

  if (BTK_TOGGLE_BUTTON(tc->tb_others)->active)
  {
    other_runtest(obj);
  }
  else if (BTK_TOGGLE_BUTTON(tc->tb_ref_selection)->active)
  {
    const bchar *indexstr;
    bint index;

    indexstr = btk_entry_get_text(BTK_ENTRY(tc->index_entry));
    index = string_to_int((bchar *)indexstr);

    ref_selection_runtest(obj, index); 
  }
  else if (BTK_TOGGLE_BUTTON(tc->tb_ref_at)->active)
  {
    const bchar *rowstr, *colstr;
    bint row, col;
 
    rowstr = btk_entry_get_text(BTK_ENTRY(tc->row_entry));
    colstr = btk_entry_get_text(BTK_ENTRY(tc->col_entry));
    row = string_to_int((bchar *)rowstr);
    col = string_to_int((bchar *)colstr);
 
    ref_at_runtest(obj, row, col);
  }
  else if (BTK_TOGGLE_BUTTON(tc->tb_ref_accessible_child)->active)
  {
    const bchar *childstr;
    bint childno;
    childstr = btk_entry_get_text(BTK_ENTRY(tc->child_entry));
    childno = string_to_int((bchar *)childstr);

    ref_accessible_child_runtest(obj, childno);
  }
}

static void _check_table (BatkObject *in_obj)
{
  BatkObject *obj = NULL;
  const char *no_properties;
  const char *no_gui;

  no_properties = g_getenv ("TEST_ACCESSIBLE_NO_PROPERTIES");
  no_gui = g_getenv ("TEST_ACCESSIBLE_NO_GUI");

  if (no_properties != NULL)
    g_properties = FALSE;
  if (no_gui != NULL)
    g_visibleGUI = 1;
  
  obj = find_object_by_type(in_obj, "BailTreeView");
  if (obj != NULL)
    g_print("Found BailTreeView table in child!\n");
  else
  {
    obj = find_object_by_type(in_obj, "BailCList");
    if (obj != NULL)
      g_print("Found BailCList in child!\n");
    else
    {
      g_print("No object found %s\n", g_type_name (B_OBJECT_TYPE (in_obj)));
      return;
    }
  }

  g_print ("In _check_table\n");

  if (!already_accessed_batk_object(obj))
  {
    /* Set up signal handlers */

    g_print ("Adding signal handler\n");
    g_signal_connect_closure_by_id (obj,
		g_signal_lookup ("column_inserted", B_OBJECT_TYPE (obj)),
		0,
		g_cclosure_new (G_CALLBACK (_notify_table_column_inserted),
		NULL, NULL),
		FALSE);
    g_signal_connect_closure_by_id (obj,
		g_signal_lookup ("row_inserted", B_OBJECT_TYPE (obj)),
		0,
                g_cclosure_new (G_CALLBACK (_notify_table_row_inserted),
			NULL, NULL),
		FALSE);
    g_signal_connect_closure_by_id (obj,
		g_signal_lookup ("column_deleted", B_OBJECT_TYPE (obj)),
		0,
                g_cclosure_new (G_CALLBACK (_notify_table_column_deleted),
			NULL, NULL),
		FALSE);
    g_signal_connect_closure_by_id (obj,
		g_signal_lookup ("row_deleted", B_OBJECT_TYPE (obj)),
		0,
                g_cclosure_new (G_CALLBACK (_notify_table_row_deleted),
			NULL, NULL),
		FALSE);
    g_signal_connect_closure_by_id (obj,
		g_signal_lookup ("column_reordered", B_OBJECT_TYPE (obj)),
		0,
                g_cclosure_new (G_CALLBACK (_notify_table_column_reordered),
			NULL, NULL),
		FALSE);
    g_signal_connect_closure_by_id (obj,
		g_signal_lookup ("row_reordered", B_OBJECT_TYPE (obj)),
		0,
                g_cclosure_new (G_CALLBACK (_notify_table_row_reordered),
			NULL, NULL),
	        FALSE);
    g_signal_connect_closure (obj, "children_changed::add",
		g_cclosure_new (G_CALLBACK (_notify_table_child_added),
                        NULL, NULL),
		FALSE);

    g_signal_connect_closure (obj, "children_changed::remove",
		g_cclosure_new (G_CALLBACK (_notify_table_child_removed),
                        NULL, NULL),
		FALSE);

  }
  g_table = BATK_TABLE(obj);

  batk_object_connect_property_change_handler (obj,
                   (BatkPropertyChangeHandler*) _property_change_handler);

  current_obj = obj;
  /*
   * The use of &current_obj allows us to change the object being processed
   * in the GUI.
   */
  if (g_visibleGUI != 1)
    test_choice_gui(&current_obj);
  else if (no_gui != NULL)
    nogui_runtest(obj);
}

static 
void other_runtest(BatkObject *obj)
{
  BatkObject *header_obj;
  BatkObject *out_obj;
  const bchar *out_string;
  GString *out_desc;
  bint n_cols, n_rows;
  bint rows_to_loop = NUM_ROWS_TO_LOOP;
  bint i, j;
  out_desc = g_string_sized_new(256);

  n_cols = batk_table_get_n_columns(BATK_TABLE(obj));
  n_rows = batk_table_get_n_rows(BATK_TABLE(obj));

  g_print ("Number of columns is %d\n", n_cols);
  g_print ("Number of rows is %d\n", n_rows);

  /* Loop NUM_ROWS_TO_LOOP rows if possible */
  if (n_rows < NUM_ROWS_TO_LOOP)
     rows_to_loop = n_rows;

  g_print ("\n");

  /* Caption */

  out_obj = batk_table_get_caption(BATK_TABLE(obj));
  if (out_obj != NULL)
  {
    out_string = batk_object_get_name (out_obj);
    if (out_string)
      g_print("Caption Name is <%s>\n", out_string);
    else
      g_print("Caption has no name\n");
  }
  else
    g_print("No caption\n");

  /* Column descriptions and headers */

  g_print ("\n");
  for (i=0; i < n_cols; i++)
  {
    /* check default */
    out_string = batk_table_get_column_description(BATK_TABLE(obj), i);
    if (out_string != NULL)
      g_print("%d: Column description is <%s>\n", i, out_string);
    else
      g_print("%d: Column description is <NULL>\n", i);

    /* check setting a new value */
    
    g_string_printf(out_desc, "new column description %d", i);

    if (out_string == NULL || strcmp (out_string, out_desc->str) != 0)
    {
      g_print("%d, Setting the column description to <%s>\n",
        i, out_desc->str);
      batk_table_set_column_description(BATK_TABLE(obj), i, out_desc->str);
      out_string = batk_table_get_column_description(BATK_TABLE(obj), i);
      if (out_string != NULL)
        g_print("%d: Column description is <%s>\n", i, out_string);
      else
        g_print("%d: Column description is <NULL>\n", i);
    }

    /* Column header */
    header_obj = batk_table_get_column_header(BATK_TABLE(obj), i);
    _display_header_info("Column", header_obj, i);
  }

  /* Row description */

  g_print ("\n");

  for (i=0; i < rows_to_loop; i++)
  {
    out_string = batk_table_get_row_description(BATK_TABLE(obj), i);
    if (out_string != NULL)
      g_print("%d: Row description is <%s>\n", i, out_string);
    else
      g_print("%d: Row description is <NULL>\n", i);

    g_string_printf(out_desc, "new row description %d", i);

    if (out_string == NULL || strcmp (out_string, out_desc->str) != 0)
    {
      g_print("%d: Setting the row description to <%s>\n",
        i, out_desc->str);
      batk_table_set_row_description(BATK_TABLE(obj), i, out_desc->str);

      out_string = batk_table_get_row_description(BATK_TABLE(obj), i);
      if (out_string != NULL)
        g_print("%d: Row description is <%s>\n", i, out_string);
      else
        g_print("%d: Row description is <NULL>\n", i);
    }

    header_obj = batk_table_get_row_header(BATK_TABLE(obj), i);
    _display_header_info("Row", header_obj, i);
    
    for (j=0; j <n_cols; j++)
    {
      bint index = batk_table_get_index_at(BATK_TABLE(obj), i, j);
      bint row, column;

      column = batk_table_get_column_at_index (BATK_TABLE (obj), index);
      g_return_if_fail (column == j);

      row = batk_table_get_row_at_index (BATK_TABLE (obj), index);
      g_return_if_fail (row == i);

      if(batk_selection_is_child_selected(BATK_SELECTION(obj), index))
        g_print("batk_selection_is_child_selected,index = %d returns TRUE\n", index);
      /* Generic cell tests */
      /* Just test setting column headers once. */

      if (!tested_set_headers)
      {
        tested_set_headers = TRUE;
   
        /* Hardcode to 1,1 for now */
        g_print(
          "Testing set_column_header for column %d, to table\n",
          (n_cols - 1));
        batk_table_set_column_header(BATK_TABLE(obj), (n_cols - 1), obj);

        g_print("Testing set_row_header for row %d, to table\n", n_rows);
        batk_table_set_row_header(BATK_TABLE(obj), n_rows, obj);
      }
    }
  }

  /* row/column extents */

  g_print("\n");
  g_print("Row extents at 1,1 is %d\n",
    batk_table_get_row_extent_at(BATK_TABLE(obj), 1, 1));
  g_print("Column extents at 1,1 is %d\n",
    batk_table_get_column_extent_at(BATK_TABLE(obj), 1, 1));
}

static
void ref_accessible_child_runtest(BatkObject *obj, bint child)
{
  BatkObject *child_obj;
  /* ref_child */
  g_print ("Accessing child %d\n", child);
  child_obj = batk_object_ref_accessible_child (obj, child);
  _property_signal_connect(child_obj);
  if (child_obj != NULL)
     _process_child(child_obj);
}

static 
void ref_selection_runtest (BatkObject *obj, bint index)
{
  BatkObject *child_obj;

  /* use batk_selection_ref_selection just once to check it works */
  child_obj = batk_selection_ref_selection(BATK_SELECTION(obj), index);
  if (child_obj)
  {
    g_print("child_obj gotten from batk_selection_ref_selection\n");
    g_object_unref (child_obj);
  }
  else 
    g_print("NULL returned by batk_selection_ref_selection\n");

  _selection_tests(obj);
}

static
void ref_at_runtest(BatkObject *obj, bint row, bint col)
{
  BatkObject *child_obj;
  /* ref_at */

  g_print("Testing ref_at row %d column %d\n", row, col);

  child_obj = batk_table_ref_at(BATK_TABLE(obj), row, col);
  _property_signal_connect(child_obj);

  g_print("Row is %d, col is %d\n", row, col);

  _process_child(child_obj);
  if (child_obj)
    g_object_unref (child_obj);
}

/**
 * process_child
 **/
static void
_process_child(BatkObject *child_obj)
{
  if (child_obj != NULL)
  {
    if (BATK_IS_TEXT(child_obj))
    {
      add_handlers(child_obj);
      setup_gui(child_obj, runtest);
    }
    else
    {
      g_print("Interface is not text!\n");
    }
/*
    if (BATK_IS_ACTION(child_obj))
      {
	bint i, j;
	bchar *action_name;
	bchar *action_description;
	bchar *action_keybinding;
	BatkAction *action = BATK_ACTION(child_obj);

	i = batk_action_get_n_actions (action);
	g_print ("Supports BatkAction with %d actions.\n", i);
	for (j = 0; j < i; j++)
	  {
	    g_print ("Action %d:\n", j);
	    action_name = batk_action_get_name (action, j);
	    if (action_name)
	      g_print (" Name = %s\n", action_name);
	    action_description = batk_action_get_description (action, j);
	    if (action_description)
	      g_print (" Description = %s\n", action_description);
	    action_keybinding = batk_action_get_keybinding (action, j);
	    if (action_keybinding)
	      g_print (" Keybinding = %s\n", action_keybinding);
	    action_description = "new description";
	    g_print (" Setting description to %s\n", action_description);
	    batk_action_set_description (action, j, action_description);
	    action_description = batk_action_get_description (action, j);
	    if (action_description)
	      g_print (" New description is now %s\n", action_description);
	  }
      }
*/
  }
  else
  {
    g_print("Child is NULL!\n");
  }
}
       
/**
 * Combined tests on BatkTable and BatkSelection on individual rows rather than 
 * all of them 
 **/
static void
_selection_tests(BatkObject *obj)
{
  bint n_rows = 0;
  bint n_cols = 0;
  bint selection_count = 0;
  bint i = 0;
  bint *selected = NULL;
  BatkTable *table;

  table = BATK_TABLE (obj);

  n_rows = batk_table_get_selected_rows(table, &selected);
  for (i = 0; i < n_rows; i++)
  {
    g_print("batk_table_get_selected_row returns : %d\n", 
              selected[i]);
    if (!batk_table_is_row_selected (table, selected[i]))
      g_print("batk_table_is_row_selected returns false for selected row %d\n", 
              selected[i]);
  }
  g_free (selected);

  selected = NULL;
  n_cols = batk_table_get_selected_columns(table, &selected);
  for (i = 0; i < n_cols; i++)
    g_print("batk_table_get_selected_columns returns : %d\n", selected[i]);
  g_free (selected);
	
  selection_count = batk_selection_get_selection_count(BATK_SELECTION(obj));
  g_print("batk_selection_get_selection_count returns %d\n", selection_count);

  if (batk_table_is_row_selected(table, 2))
  {
    g_print("batk_table_is_row_selected (table, 2) returns TRUE\n");
    batk_selection_clear_selection (BATK_SELECTION (obj));
    if (batk_table_add_row_selection(table, 4))
      g_print("batk_table_add_row_selection: selected row 4\n");
    if (!batk_table_is_row_selected (table, 4))
      g_print("batk_table_is_row_selected returns false for row 2\n");
    if (batk_table_is_row_selected (table, 2))
      g_print("batk_table_is_row_selected gives false positive for row 2\n");
  }

  if (batk_table_is_row_selected(table, 3))
  {
    if (batk_table_remove_row_selection(table, 3))
      g_print("batk_table_remove_row_selection unselected row 3\n");
  }

  if (batk_table_is_selected(table, 5, 4))
  {
    batk_selection_clear_selection(BATK_SELECTION(obj));
    g_print("batk_selection_clear_selection: just cleared all selected\n");
  }

  if (batk_table_is_column_selected(table, 2))
  {
    g_print("batk_table_is_column_selected(obj, 2) returns TRUE\n");
    if (batk_table_add_column_selection(table, 4))
      g_print("batk_table_add_column_selection: selected column 4\n");
    g_print("batk_table_is_column_selected(obj, 2) returns TRUE\n");
  }

  if (batk_table_is_column_selected(table, 3))
  {
    if (batk_table_remove_column_selection(table, 3))
      g_print("batk_table_remove_column_selection: unselected column 3\n");
  }
}

static void
_create_event_watcher (void)
{
  batk_add_focus_tracker (_check_table);
}

int
btk_module_init(bint argc, char* argv[])
{
    g_print("TestTable Module loaded\n");

    _create_event_watcher();

    return 0;
}

static void
_notify_table_row_inserted (BObject *obj, bint start_offset, bint length)
{
  g_print ("SIGNAL - Row inserted at position %d, num of rows inserted %d!\n",
    start_offset, length);
}

static void
_notify_table_column_inserted (BObject *obj, bint start_offset, bint length)
{
  g_print ("SIGNAL - Column inserted at position %d, num of columns inserted %d!\n",
    start_offset, length);
}

static void
_notify_table_row_deleted (BObject *obj, bint start_offset, bint length)
{
  g_print ("SIGNAL - Row deleted at position %d, num of rows deleted %d!\n",
    start_offset, length);
}

static void
_notify_table_column_deleted (BObject *obj, bint start_offset, bint length)
{
  g_print ("SIGNAL - Column deleted at position %d, num of columns deleted %d!\n",
    start_offset, length);
}

static void
_notify_table_row_reordered (BObject *obj)
{
  g_print ("SIGNAL - Row reordered!\n");
}

static void
_notify_table_column_reordered (BObject *obj)
{
  g_print ("SIGNAL - Column reordered!\n");
}

static void _notify_table_child_added (BObject *obj,
  bint index, BatkObject *child)
{
   g_print ("SIGNAL - Child added - index %d\n", index);
}

static void _notify_table_child_removed (BObject *obj,
  bint index, BatkObject *child)
{
   g_print ("SIGNAL - Child removed - index %d\n", index);
}

static void
_display_header_info(bchar *type, BatkObject *header_obj, bint header_num)
{
  if (header_obj != NULL)
  {
    BatkRole role;
    role = batk_object_get_role(header_obj);

    if (role == BATK_ROLE_PUSH_BUTTON)
    {
      g_print ("%d: %s header is a push button!\n", header_num, type);
    }
    else if (role == BATK_ROLE_LABEL)
    {
      g_print ("%d: %s header is a label!\n", header_num, type);
    }
    else if (BATK_IS_TEXT(header_obj))
    {
      bchar *header_text;

      header_text = batk_text_get_text (BATK_TEXT (header_obj), 0, 3);
      if (header_text != NULL)
      {
        g_print("%d: %s header is a text value <%s>\n", header_num,
          type, header_text);
      }
      else
      {
        g_print("%d: %s header is a text value <NULL>\n", header_num,
          type);
      }
    }
    else 
    {
      g_print ("%d: %s header is of type %s!\n", header_num,
        type, batk_role_get_name (role));
    }
  }
  else
  {
    g_print ("%d: %s header object is NULL!\n", header_num, type);
  }
}

static void _property_signal_connect (BatkObject *obj)
{
  if (g_properties && obj != NULL)
  {
    g_signal_connect_closure_by_id (obj,
    g_signal_lookup ("property_change", B_OBJECT_TYPE (obj)),
      0,
      g_cclosure_new (G_CALLBACK (_property_change_handler),
      NULL, NULL),
      FALSE);
  }
}

static void 
_property_change_handler (BatkObject         *obj,
                          BatkPropertyValues *values)
{
  bchar *obj_text;
  const bchar *name;

  if (g_table != NULL)
  {
    bint index = batk_object_get_index_in_parent(obj);
    
    if (index >= 0)
      g_print("Index is %d, row is %d, col is %d\n", index,
              batk_table_get_row_at_index(g_table, index),
              batk_table_get_column_at_index(g_table, index));
    else
      g_print ("index: %d for %s\n", index, g_type_name (B_OBJECT_TYPE (obj)));
  }

  if (BATK_IS_TEXT(obj))
  {
     obj_text = batk_text_get_text (BATK_TEXT (obj), 0, 15);
     if (obj_text == NULL)
       g_print("  Cell text is <NULL>\n");
     else
       g_print("  Cell text is <%s>\n", obj_text);
  }

  g_print("  PropertyName <%s>\n",
	   values->property_name ? values->property_name: "NULL");
  g_print("    - ");

  if (&values->old_value != NULL && G_IS_VALUE (&values->old_value))
  {
    GType old_type = G_VALUE_TYPE (&values->old_value);

    switch (old_type)
    {
    case B_TYPE_INT:
      g_print("value was <%d>\n", b_value_get_int (&values->old_value));
      break;
    case B_TYPE_STRING:
      name = b_value_get_string (&values->old_value);
      if (name != NULL)
        g_print ("value was <%s>\n", name);
      else
        g_print ("value was <NULL>\n");
      break;
    default: 
      g_print("value was <unknown type>\n");
      break;
    }
  }
  else
  {
    g_print("value was <not a value>\n");
  }
  g_print("    - ");
  if (&values->new_value != NULL && G_IS_VALUE (&values->new_value))
  {
    GType new_type = G_VALUE_TYPE (&values->new_value);

    switch (new_type)
    {
    case B_TYPE_INT:
      g_print("value is <%d>\n", b_value_get_int (&values->new_value));
      break;
    case B_TYPE_STRING:
      name = b_value_get_string (&values->new_value);
      if (name != NULL)
        g_print ("value is <%s>\n", name);
      else
        g_print ("value is <NULL>\n");
      break;
    default: 
      g_print("value is <unknown type>\n");
      break;
    }
  }
  else
  {
    g_print("value is <not a value>\n");
  }
}

static
void test_choice_gui(BatkObject **obj)
{
  BtkWidget *window;
  BtkWidget *vbox;
  BtkWidget *hbox;
  BtkWidget *child_label;
  BtkWidget *row_label;
  BtkWidget *col_label;
  BtkWidget *index_label;
  BtkWidget *hseparator;
  BtkWidget *hbuttonbox;
  BtkWidget *button;


  tc = (TestChoice *) g_malloc (sizeof(TestChoice));
  
  window = btk_window_new(BTK_WINDOW_TOPLEVEL);
  btk_window_set_title(BTK_WINDOW(window), "Test to run");

  g_signal_connect(BTK_OBJECT (window), "destroy",
                   G_CALLBACK (destroy), &window);

  vbox = btk_vbox_new(TRUE, 0);
  btk_box_set_spacing(BTK_BOX(vbox), 10);


  hbox = btk_hbox_new(FALSE, 0);
  btk_box_set_spacing(BTK_BOX(hbox), 10);
  tc->tb_ref_selection = btk_toggle_button_new_with_label("ref_selection");
  btk_box_pack_start (BTK_BOX (hbox), tc->tb_ref_selection, TRUE, TRUE, 0);
  index_label = btk_label_new("index: ");
  btk_box_pack_start (BTK_BOX (hbox), index_label, TRUE, TRUE, 0);
  tc->index_entry = btk_entry_new();
  btk_entry_set_text(BTK_ENTRY(tc->index_entry), "1");
  btk_box_pack_start (BTK_BOX (hbox), tc->index_entry, TRUE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (vbox), hbox, TRUE, TRUE, 0); 

  hbox = btk_hbox_new(FALSE, 0);
  btk_box_set_spacing(BTK_BOX(hbox), 10);
  tc->tb_ref_at = btk_toggle_button_new_with_label("ref_at");
  btk_box_pack_start (BTK_BOX (hbox), tc->tb_ref_at, TRUE, TRUE, 0);
  row_label = btk_label_new("row:");
  btk_box_pack_start (BTK_BOX (hbox), row_label, TRUE, TRUE, 0);
  tc->row_entry = btk_entry_new();
  btk_entry_set_text(BTK_ENTRY(tc->row_entry), "1");
  btk_box_pack_start (BTK_BOX (hbox), tc->row_entry, TRUE, TRUE, 0);
  col_label = btk_label_new("column:");
  btk_box_pack_start (BTK_BOX (hbox), col_label, TRUE, TRUE, 0);
  tc->col_entry = btk_entry_new();
  btk_entry_set_text(BTK_ENTRY(tc->col_entry), "1");
  btk_box_pack_start (BTK_BOX (hbox), tc->col_entry, TRUE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (vbox), hbox, TRUE, TRUE, 0); 

  hbox = btk_hbox_new(FALSE, 0);
  btk_box_set_spacing(BTK_BOX(hbox), 10);
  tc->tb_ref_accessible_child = btk_toggle_button_new_with_label("ref_accessible_child");
  btk_box_pack_start (BTK_BOX (hbox), tc->tb_ref_accessible_child, TRUE, TRUE, 0);
  child_label = btk_label_new("Child no:");
  btk_box_pack_start (BTK_BOX (hbox), child_label, TRUE, TRUE, 0); 
  tc->child_entry = btk_entry_new();
  btk_entry_set_text(BTK_ENTRY(tc->child_entry), "1");
  btk_box_pack_start (BTK_BOX (hbox), tc->child_entry, TRUE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  tc->tb_others = btk_toggle_button_new_with_label("others");
  btk_box_pack_start (BTK_BOX (vbox), tc->tb_others, TRUE, TRUE, 0);
  
  hseparator = btk_hseparator_new();
  btk_box_pack_start (BTK_BOX (vbox), hseparator, TRUE, TRUE, 0);

  button = btk_button_new_with_mnemonic("_Run Test");

  hbuttonbox = btk_hbutton_box_new();
  btk_button_box_set_layout(BTK_BUTTON_BOX(hbuttonbox),
    BTK_BUTTONBOX_SPREAD);
  btk_box_pack_end (BTK_BOX (hbuttonbox), BTK_WIDGET (button), TRUE, TRUE, 0);
  btk_box_pack_end (BTK_BOX (vbox), hbuttonbox, TRUE, TRUE, 0);
  g_signal_connect(BTK_OBJECT(button), "clicked", G_CALLBACK (choicecb), obj);

  btk_container_add(BTK_CONTAINER(window), vbox);
  btk_widget_show(vbox);
  btk_widget_show(window);
  btk_widget_show_all(BTK_WIDGET(window));

  g_visibleGUI = 1;
}

void static
nogui_runtest (BatkObject *obj)
{
  g_print ("Running non-GUI tests...\n");
  other_runtest (obj);
  nogui_ref_at_runtest (obj);
}

static void
nogui_ref_at_runtest (BatkObject *obj)
{
  BatkObject *child_obj;
  bint i, j;
  bint n_cols;
  bint rows_to_loop = 5;

  n_cols = batk_table_get_n_columns (BATK_TABLE(obj));
  
  if (batk_table_get_n_rows(BATK_TABLE(obj)) < rows_to_loop)
    rows_to_loop = batk_table_get_n_rows (BATK_TABLE(obj));

  for (i=0; i < rows_to_loop; i++)
  {
    /* Just the first rows_to_loop rows */
    for (j=0; j < n_cols; j++)
    {
      bint index = batk_table_get_index_at(BATK_TABLE(obj), i, j);
	  if(batk_selection_is_child_selected(BATK_SELECTION(obj), index))
		 g_print("batk_selection_is_child_selected,index = %d returns TRUE\n", index);

      g_print("Testing ref_at row %d column %d\n", i, j);

      if (i == 3 && j == 0)
      {
        g_print("child_obj gotten from batk_selection_ref_selection\n");

        /* use batk_selection_ref_selection just once to check it works */
        child_obj = batk_selection_ref_selection(BATK_SELECTION(obj), index );
      }
      else	
      {
        child_obj = batk_table_ref_at(BATK_TABLE(obj), i, j);
      }

      _property_signal_connect(child_obj);

      g_print("Index is %d, row is %d, col is %d\n", index,
        batk_table_get_row_at_index(BATK_TABLE(obj), index),
        batk_table_get_column_at_index(BATK_TABLE(obj), index));

      nogui_process_child (child_obj);

      /* Generic cell tests */
      /* Just test setting column headers once. */

      if (!tested_set_headers)
      {
        tested_set_headers = TRUE;

        g_print("Testing set_column_header for column %d, to cell value %d,%d\n",
          j, i, j);
        batk_table_set_column_header(BATK_TABLE(obj), j, child_obj);

        g_print("Testing set_row_header for row %d, to cell value %d,%d\n",
          i, i, j);
        batk_table_set_row_header(BATK_TABLE(obj), i, child_obj);
      }
      if (child_obj)
        g_object_unref (child_obj);
    }
  }
}

static void
nogui_process_child (BatkObject *obj)
{
  bchar default_val[5] = "NULL";

  if (BATK_IS_TEXT(obj))
    {
      bchar *current_text;
      current_text = batk_text_get_text (BATK_TEXT(obj), 0, -1);
      g_print ("Child supports text interface.\nCurrent text is %s\n", current_text);
    }

  if (BATK_IS_ACTION(obj))
    {
      BatkAction *action = BATK_ACTION(obj);
      bint n_actions, i;
      const bchar *name, *description;
      
      n_actions = batk_action_get_n_actions (action);
      g_print ("Child supports %d actions.\n", n_actions);
      for (i = 0; i < n_actions; i++)
	{
	  name = batk_action_get_name (action, i);
	  description = batk_action_get_description (action, i);

          if (name == NULL)
             name = default_val;
          if (description == NULL)
             description = default_val;
          
	  g_print (" %d: name = <%s>\n", i, name);
          g_print ("    description = <%s>\n", description);
	}
    }
}

