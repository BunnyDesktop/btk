#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "testlib.h" 

static gint	_get_position_in_array		(gint		window,
						gchar		*the_test_name);
static gint	_get_position_in_parameters	(gint		window,
						gchar		*label,
						gint		position);
static void	_create_output_window		(OutputWindow	**outwin);
static gboolean	_create_select_tests_window	(BatkObject	*obj,
                                                TLruntest	runtest,
                                                OutputWindow	**outwin);
static void	_toggle_selectedcb		(BtkWidget	*widget,
						gpointer	test);
static void	_testselectioncb		(BtkWidget	*widget,
						gpointer	data);
static void	_destroy			(BtkWidget	*widget,
						gpointer	data);

/* General functions */

/**
 * find_object_by_role:
 * @obj: An #BatkObject
 * @roles: An array of roles to search for
 * @num_roles: The number of entries in @roles
 *
 * Find the #BatkObject which is a decendant of the specified @obj
 * which is of an #BatkRole type specified in the @roles array.
 *
 * Returns: the #BatkObject that meets the specified criteria or NULL
 * if no object is found. 
 **/
BatkObject*
find_object_by_role (BatkObject *obj,
                     BatkRole   *roles,
                     gint      num_roles)
{
  /*
   * Find the first object which is a descendant of the specified object
   * which matches the specified role.
   *
   * This function returns a reference to the BatkObject which should be
   * removed when finished with the object.
   */
  gint i, j;
  gint n_children;
  BatkObject *child;

  if (obj == NULL)
    return NULL;

  for (j=0; j < num_roles; j++)
    {
      if (batk_object_get_role (obj) == roles[j])
        return obj;
    }

  n_children = batk_object_get_n_accessible_children (obj);
  for (i = 0; i < n_children; i++)
    {
      BatkObject* found_obj;

      child = batk_object_ref_accessible_child (obj, i);

      if (child == NULL)
        continue;

      for (j=0; j < num_roles; j++)
        {
          if (batk_object_get_role (child) == roles[j])
            return child;
        }

      found_obj = find_object_by_role (child, roles, num_roles);
      g_object_unref (child);
      if (found_obj)
        return found_obj;
    }
  return NULL;
}

/**
 * find_object_by_name_and_role:
 * @obj: An #BatkObject
 * @name: The BTK widget name
 * @roles: An array of roles to search for
 * @num_roles: The number of entries in @roles
 *
 * Find the #BatkObject which is a decendant of the specified @obj
 * which is of an #BatkRole type specified in the @roles array which
 * also has the BTK widget name specified in @name.
 *
 * Returns: the #BatkObject that meets the specified criteria or NULL
 * if no object is found. 
 **/
BatkObject*
find_object_by_name_and_role(BatkObject   *obj,
                             const gchar *name,
                             BatkRole	 *roles,
                             gint        num_roles)
{
  BatkObject *child;
  BtkWidget* widget;
  gint i, j;
  gint n_children;

  if (obj == NULL)
    return NULL;

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (BTK_IS_WIDGET (widget))
    {
      if (strcmp (name, btk_widget_get_name(BTK_WIDGET (widget))) == 0)
        {
          for (j=0; j < num_roles; j++)
            {
              if (batk_object_get_role (obj) == roles[j])
                return obj;
            }
        }
    }

  n_children = batk_object_get_n_accessible_children (obj);
  for (i = 0; i < n_children; i++)
    {
      BatkObject* found_obj;
 
      child = batk_object_ref_accessible_child (obj, i);

      if (child == NULL)
        continue;

      widget = BTK_ACCESSIBLE (child)->widget;
      if (BTK_IS_WIDGET (widget))
        {
          if (strcmp(name, btk_widget_get_name(BTK_WIDGET (widget))) == 0)
            {
              for (j=0; j < num_roles; j++)
                {
                  if (batk_object_get_role (child) == roles[j])
                    return child;
                }
            }
        }
      found_obj = find_object_by_name_and_role (child, name, roles, num_roles);
      g_object_unref (child);
      if (found_obj)
        return found_obj;
    }
  return NULL;
}

/**
 * find_object_by_accessible_name_and_role:
 * @obj: An #BatkObject
 * @name: The accessible name
 * @roles: An array of roles to search for
 * @num_roles: The number of entries in @roles
 *
 * Find the #BatkObject which is a decendant of the specified @obj
 * which has the specified @name and matches one of the 
 * specified @roles.
 * 
 * Returns: the #BatkObject that meets the specified criteria or NULL
 * if no object is found. 
 */
BatkObject*
find_object_by_accessible_name_and_role (BatkObject   *obj,
                                         const gchar *name,
                                         BatkRole     *roles,
	                                 gint        num_roles)
{
  BatkObject *child;
  gint i, j;
  gint n_children;
  const gchar *accessible_name;

  if (obj == NULL)
    return NULL;

  accessible_name = batk_object_get_name (obj);
  if (accessible_name && (strcmp(name, accessible_name) == 0))
    {
      for (j=0; j < num_roles; j++)
        {
          if (batk_object_get_role (obj) == roles[j])
            return obj;
        }
    }

  n_children = batk_object_get_n_accessible_children (obj);
  for (i = 0; i < n_children; i++)
    {
      BatkObject* found_obj;

      child = batk_object_ref_accessible_child (obj, i);

      if (child == NULL)
        continue;

      accessible_name = batk_object_get_name (child);
      if (accessible_name && (strcmp(name, accessible_name) == 0))
        {
          for (j=0; j < num_roles; j++)
            {
              if (batk_object_get_role (child) == roles[j])
                return child;
            }
        }
      found_obj = find_object_by_accessible_name_and_role (child, name, 
                                                           roles, num_roles);
      g_object_unref (child);
      if (found_obj)
        return found_obj;
    }
  return NULL;
}

/**
 * find_object_by_name_and_role:
 * @obj: An #BatkObject
 * @type: The type 
 *
 * Find the #BatkObject which is a decendant of the specified @obj
 * which has the specified @type.
 * 
 * Returns: the #BatkObject that meets the specified criteria or NULL
 * if no object is found. 
 */
BatkObject*
find_object_by_type (BatkObject *obj, 
                     gchar     *type)
{
  /*
   * Find the first object which is a descendant of the specified object
   * which matches the specified type.
   *
   * This function returns a reference to the BatkObject which should be
   * removed when finished with the object.
   */
  gint i;
  gint n_children;
  BatkObject *child;
  const gchar * typename = NULL;

  if (obj == NULL)
    return NULL;

  typename = g_type_name (B_OBJECT_TYPE (obj));
  if (strcmp (typename, type) == 0)
     return obj;

  n_children = batk_object_get_n_accessible_children (obj);
  for (i = 0; i < n_children; i++)
    {
      BatkObject* found_obj;

      child = batk_object_ref_accessible_child (obj, i);

      if (child == NULL)
        continue;

      typename = g_type_name (B_OBJECT_TYPE (child));

      if (strcmp (typename, type) == 0)
        return child;

      found_obj = find_object_by_type (child, type);
      g_object_unref (child);
      if (found_obj)
        return found_obj;
    }
  return NULL;
}

/**
 * already_accessed_batk_object
 * @obj: An #BatkObject
 *
 * Keeps a static GPtrArray of objects that have been passed into this
 * function. 
 *
 * Returns: TRUE if @obj has been passed into this function before
 * and FALSE otherwise.
 */
gboolean
already_accessed_batk_object (BatkObject *obj)
{
  static GPtrArray *obj_array = NULL;
  gboolean found = FALSE;
  gint i;

  /*
   * We create a property handler for each object if one was not associated
   * with it already.
   *
   * We add it to our array of objects which have property handlers; if an
   * object is destroyed it remains in the array.
   */
  if (obj_array == NULL)
    obj_array = g_ptr_array_new ();

  for (i = 0; i < obj_array->len; i++)
    {
      if (obj == g_ptr_array_index (obj_array, i))
        {
          found = TRUE;
          break;
        }
    }
  if (!found)
    g_ptr_array_add (obj_array, obj);

  return found;
}

/**
 * display_children
 * @obj: An #BatkObject
 * @depth: Number of spaces to indent output.
 * @child_number: The child number of this object.
 *
 * Displays the hierarchy of widgets starting from @obj.
 **/
void
display_children (BatkObject *obj, 
                  gint      depth, 
                  gint      child_number)
{
  display_children_to_depth(obj, -1, depth, child_number);
}

/**
 * display_children_to_depth
 * @obj: An #BatkObject
 * @to_depth: Display to this depth.
 * @depth: Number of spaces to indent output.
 * @child_number: The child number of this object.
 *
 * Displays the hierarchy of widgets starting from @obj only
 * to the specified depth.
 **/
void
display_children_to_depth (BatkObject *obj,
                           gint      to_depth,
                           gint      depth,
                           gint      child_number)
{
  BatkRole role;
  const gchar *rolename;
  const gchar *typename;
  gint n_children, parent_index, i;

  if (to_depth >= 0 && depth > to_depth)
     return;

  if (obj == NULL)
     return;

  for (i=0; i < depth; i++)
    g_print(" ");

  role = batk_object_get_role (obj);
  rolename = batk_role_get_name (role);

 /*
  * Note that child_number and parent_index should be the same
  * unless there is an error.
  */
  parent_index = batk_object_get_index_in_parent(obj);
  g_print("child <%d == %d> ", child_number, parent_index);

  n_children = batk_object_get_n_accessible_children (obj);
  g_print ("children <%d> ", n_children);

  if (rolename)
    g_print("role <%s>, ", rolename);
  else
    g_print("role <error>");

  if (BTK_IS_ACCESSIBLE(obj))
    {
      BtkWidget *widget;

      widget = BTK_ACCESSIBLE (obj)->widget;
      g_print("name <%s>, ", btk_widget_get_name(BTK_WIDGET (widget)));
    }
  else
    g_print("name <NULL>, ");

  typename = g_type_name (B_OBJECT_TYPE (obj));
  g_print ("typename <%s>\n", typename);

  for (i = 0; i < n_children; i++)
    {
      BatkObject *child;

      child = batk_object_ref_accessible_child (obj, i);
      if (child != NULL)
        {
          display_children_to_depth (child, to_depth, depth + 1, i);
          g_object_unref (B_OBJECT (child));
        }
    }
}

/* Test GUI logic */

/* GUI Information for the Select Tests Window */
typedef struct
{
  BtkWidget     *selecttestsWindow;
  BtkWidget     *hbox;
  BtkWidget     *vbox;
  BtkWidget     *label;
  BtkWidget     *textInsert;
  BtkWidget     *button;
  gchar         *selecttestsTitle;
}MainDialog;

/* Functionality information about each added test */
typedef struct
{
  BtkWidget     *toggleButton;
  BtkWidget     *hbox;
  BtkWidget     *parameterLabel[MAX_PARAMS];
  BtkWidget     *parameterInput[MAX_PARAMS];
  gchar         *testName;
  gint          numParameters;
}TestList;

typedef struct
{
   TLruntest   runtest;
   BatkObject*  obj;
   gint	       win_num;
}TestCB;

static MainDialog      *md[MAX_WINDOWS];
static OutputWindow    *ow;

/* An array containing function information on all of the tests */
static TestList        listoftests[MAX_WINDOWS][MAX_TESTS];

/* A counter for the actual number of added tests */
gint                   counter;

/* A global for keeping track of the window numbers */
static gint 	       window_no = 0;
/* An array containing the names of the tests that are "on" */
static gchar           *onTests[MAX_WINDOWS][MAX_TESTS]; 
static gint            g_visibleDialog = 0;
static gint	       testcount[MAX_WINDOWS];
static TestCB          testcb[MAX_WINDOWS];

/**
 * create_windows:
 * @obj: An #BatkObject
 * @runtest: The callback function to run when the "Run Tests" button
 *   is clicked.
 * @outwin: The output window to use.  If NULL is passed in, then 
 *   create a new one.
 *
 * Creates the test window and the output window (if @outwin is NULL)
 * Runs _create_output_window() and _create_select_tests_window() 
 * and sets g_visibleDialog to 1
 *
 * Returns: The window number of the created window if successful, -1 otherwise.
 **/
gint
create_windows (BatkObject    *obj,
                TLruntest    runtest,
                OutputWindow **outwin)
{
  gboolean valid;  
  gint tmp;

  g_visibleDialog = 1;
  _create_output_window(outwin); 
  valid = _create_select_tests_window(obj, runtest, outwin);
  if (valid)
    {
      tmp = window_no;
      window_no++;
      return tmp;
    }
  else
    return -1;
}

/** 
 * _create_output_window
 * @outwin: If outwin is passed in as NULL, a new output window is created
 *   otherwise, the outwin passed in is shared.
 *
 * Creates the Test Result Output Window .
 **/
static void
_create_output_window (OutputWindow **outwin)
{
  BtkWidget *view;
  BtkWidget *scrolled_window;
  OutputWindow *localow;

  if (*outwin == NULL)
    {
      localow = (OutputWindow *) malloc (sizeof(OutputWindow));
   
      localow->outputBuffer = btk_text_buffer_new(NULL);
      view = btk_text_view_new_with_buffer(BTK_TEXT_BUFFER(localow->outputBuffer));
      btk_widget_set_size_request (view, 700, 500);
      btk_text_view_set_wrap_mode(BTK_TEXT_VIEW(view), BTK_WRAP_WORD);
      btk_text_view_set_editable(BTK_TEXT_VIEW(view), FALSE);	

      localow->outputWindow = btk_window_new(BTK_WINDOW_TOPLEVEL);
      btk_window_set_title(BTK_WINDOW(localow->outputWindow), "Test Output");
      scrolled_window = btk_scrolled_window_new(NULL, NULL);

      btk_scrolled_window_set_policy(BTK_SCROLLED_WINDOW(scrolled_window),
                                     BTK_POLICY_NEVER, BTK_POLICY_AUTOMATIC);
      btk_container_add(BTK_CONTAINER(localow->outputWindow), scrolled_window);
      btk_container_add(BTK_CONTAINER(scrolled_window), view);
      btk_text_buffer_get_iter_at_offset(localow->outputBuffer, &localow->outputIter, 0);
      btk_widget_show(view);
      btk_widget_show(scrolled_window);
      btk_widget_show(localow->outputWindow);

      btk_text_buffer_set_text(BTK_TEXT_BUFFER(localow->outputBuffer),
        "\n\nWelcome to the test GUI:\nTest results are printed here\n\n", 58);
      btk_text_buffer_get_iter_at_offset(BTK_TEXT_BUFFER(localow->outputBuffer),
                                          &localow->outputIter, 0);
      *outwin = localow;
      ow = *outwin;
    }
}

/** 
 * _create_select_tests_window:
 * @obj: An #BatkObject
 * @runtest: The callback function that is run when the "Run Tests"
 *   button is clicked.
 * @outwin: The output window to use.
 *
 * Creates the Test Select Window 
 *
 * Returns: TRUE if successful, FALSE otherwise
 **/
static gboolean
_create_select_tests_window (BatkObject    *obj,
                             TLruntest    runtest,
                             OutputWindow **outwin)
{
  BatkText   *textwidget;
  BtkWidget *hbuttonbox;
  BtkWidget *scrolledWindow;

  if (window_no >= 0 && window_no < MAX_WINDOWS)
    {
      md[window_no] = (MainDialog *) malloc (sizeof(MainDialog));
     
      textwidget = BATK_TEXT (obj);
     
      /* Setup Window */
      md[window_no]->selecttestsTitle = "Test Setting";
      md[window_no]->selecttestsWindow = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_title (BTK_WINDOW( ow->outputWindow),
                            md[window_no]->selecttestsTitle);
      btk_window_set_resizable (BTK_WINDOW(md[window_no]->selecttestsWindow),
                                FALSE);
      btk_window_set_position (BTK_WINDOW(md[window_no]->selecttestsWindow),
                               BTK_WIN_POS_CENTER); 
      g_signal_connect (BTK_OBJECT(md[window_no]->selecttestsWindow), 
                        "destroy",
                        G_CALLBACK (_destroy),
                        &md[window_no]->selecttestsWindow);
     
      /* Setup Scrolling */
      scrolledWindow = btk_scrolled_window_new(NULL, NULL);
      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolledWindow),
                                      BTK_POLICY_NEVER, BTK_POLICY_AUTOMATIC); 
      btk_widget_set_size_request (scrolledWindow, 500, 600);
      btk_container_add (BTK_CONTAINER (md[window_no]->selecttestsWindow), 
                         scrolledWindow);
      
      /* Setup Layout */
      md[window_no]->vbox = btk_vbox_new (TRUE, 0);
      md[window_no]->button = btk_button_new_with_mnemonic ("_Run Tests");
      hbuttonbox = btk_hbutton_box_new ();
      btk_button_box_set_layout (BTK_BUTTON_BOX (hbuttonbox),
                                 BTK_BUTTONBOX_SPREAD);
      btk_box_pack_end (BTK_BOX (hbuttonbox),
                        BTK_WIDGET (md[window_no]->button), TRUE, TRUE, 0);
      btk_box_pack_end (BTK_BOX (md[window_no]->vbox), hbuttonbox,
                        TRUE, TRUE, 0);
      btk_scrolled_window_add_with_viewport (BTK_SCROLLED_WINDOW (scrolledWindow),
                                             md[window_no]->vbox);

      testcb[window_no].runtest = runtest;
      testcb[window_no].obj = obj;
      testcb[window_no].win_num = window_no; 
      g_signal_connect (BTK_OBJECT (md[window_no]->button), 
                        "clicked",
                        G_CALLBACK (_testselectioncb),
                        (gpointer)&testcb[window_no]);
     
      /* Show all */
      btk_widget_grab_focus (md[window_no]->button);
      btk_widget_show (md[window_no]->button);
      btk_widget_show (hbuttonbox); 
      btk_widget_show (scrolledWindow); 
      btk_widget_show_all (BTK_WIDGET (md[window_no]->selecttestsWindow));
      return TRUE;
    }
  else
    return FALSE;
}

/** 
 * add_test
 * @window: The window number
 * @name: The test name
 * @num_params: The number of arguments the test uses.
 * @parameter_names: The names of each argument.
 * @default_names: The default values of each argument.
 *
 * Adds a Test with the passed-in details to the Tests Select Window.  
 *
 * Returns: FALSE if the num_params passed in is greater than
 * MAX_PARAMS, otherwise returns TRUE 
 *
 **/
gboolean
add_test (gint   window, 
          gchar  *name,
          gint   num_params,
          gchar* parameter_names[],
          gchar* default_names[])
{
  gint i;

  if (num_params > MAX_PARAMS)
    return FALSE;
  else
    {
      md[window]->hbox = btk_hbox_new (FALSE, 0);
      btk_box_set_spacing (BTK_BOX (md[window]->hbox), 10);
      btk_container_set_border_width (BTK_CONTAINER (md[window]->hbox), 10);
      btk_container_add (BTK_CONTAINER (md[window]->vbox), md[window]->hbox);
      listoftests[window][testcount[window]].toggleButton =
         btk_toggle_button_new_with_label (name);
      btk_box_pack_start (BTK_BOX (md[window]->hbox),
          listoftests[window][testcount[window]].toggleButton, FALSE, FALSE, 0);
      listoftests[window][testcount[window]].testName = name;
      listoftests[window][testcount[window]].numParameters = num_params;
      for (i=0; i<num_params; i++) 
        {
     	  listoftests[window][testcount[window]].parameterLabel[i] =
            btk_label_new (parameter_names[i]);
          btk_box_pack_start (BTK_BOX (md[window]->hbox),
          listoftests[window][testcount[window]].parameterLabel[i], FALSE, FALSE, 0);
	  listoftests[window][testcount[window]].parameterInput[i] = btk_entry_new();
          btk_entry_set_text (BTK_ENTRY (listoftests[window][testcount[window]].parameterInput[i]),
            default_names[i]);
          btk_widget_set_size_request (listoftests[window][testcount[window]].parameterInput[i], 50, 22);
	  btk_box_pack_start (BTK_BOX (md[window]->hbox),
            listoftests[window][testcount[window]].parameterInput[i], FALSE, FALSE, 0);
          btk_widget_set_sensitive (
            BTK_WIDGET (listoftests[window][testcount[window]].parameterLabel[i]), FALSE);
          btk_widget_set_sensitive (
            BTK_WIDGET (listoftests[window][testcount[window]].parameterInput[i]), FALSE);
	  btk_widget_show (listoftests[window][testcount[window]].parameterLabel[i]);
	  btk_widget_show (listoftests[window][testcount[window]].parameterInput[i]);
        }
      g_signal_connect (BTK_OBJECT (listoftests[window][testcount[window]].toggleButton),
                        "toggled",
                        G_CALLBACK (_toggle_selectedcb),
                        (gpointer)&(listoftests[window][testcount[window]]));
      btk_widget_show (listoftests[window][testcount[window]].toggleButton);
      btk_widget_show (md[window]->hbox);
      btk_widget_show (md[window]->vbox);

      testcount[window]++;
      counter++;
      return TRUE;
    }  
}

/** 
 * tests_set:
 * @window: The window number
 * @count: Passes back the number of tests on.
 *
 * Gets an array of strings corresponding to the tests that are "on".
 * A test is assumed on if the toggle button is on and if all its
 * parameters have values.
 *
 * Returns: an array of strings corresponding to the tests that
 * are "on".
 **/
gchar **tests_set(gint window, int *count)
{
  gint        i =0, j = 0, num;
  gboolean    nullparam;
  gchar*      input;

  *count = 0;
  for (i = 0; i < MAX_TESTS; i++)
      onTests[window][i] = NULL;

  for (i = 0; i < testcount[window]; i++)
    {
      nullparam = FALSE;
      if (BTK_TOGGLE_BUTTON(listoftests[window][i].toggleButton)->active)
        {
          num = listoftests[window][i].numParameters;
          for (j = 0; j < num; j++)
            {
              input = btk_editable_get_chars (
                    BTK_EDITABLE (listoftests[window][i].parameterInput[j]), 0, -1);

              if (input != NULL && (! strcmp(input, "")))
                nullparam = TRUE;
            } 
          if (!nullparam)
            {
              onTests[window][*count] = listoftests[window][i].testName;
              *count = *count + 1; 
            }
        }
    } 
  return onTests[window];
}

/**
 * _get_position_in_array:
 * @window: The window number
 * @the_test_name: The name of the test
 *
 * Gets the index of the passed-in @the_test_name.
 *
 * Returns: the position in listoftests[] of @the_test_name
 **/
static gint
_get_position_in_array(gint  window,
                       gchar *the_test_name)
{
  gint        i;
  
  for (i = 0; i < testcount[window]; i++)
    {
      if (strcmp(listoftests[window][i].testName, the_test_name) == 0)
        return i;
    }
  return -1;
}

/**
 * _get_position_in_parameters:
 * @window: The window number
 * @label: The label name
 * @position: The parameter position
 *
 * Gets the index of the passed-in parameter @label.
 *
 * Returns: the position in parameterLabel[] (a member of
 * listoftests[]) of @label 
 **/
static gint
_get_position_in_parameters(gint  window,
                            gchar *label,
                            gint  position)
{
  gint                    i;
  const gchar    *label_string;
  
  for (i = 0; i < MAX_PARAMS; i++)
    {
      label_string = btk_label_get_text( 
               BTK_LABEL (listoftests[window][position].parameterLabel[i]));

      if (strcmp(label_string, label) == 0)
        return i;
    }
  return -1;
}

/** 
 * set_output_buffer:
 * @output: The string to add to the output buffer
 * 
 * Tidies up the output Window 
 **/
void
set_output_buffer(gchar *output)
{
  btk_text_buffer_insert (BTK_TEXT_BUFFER (ow->outputBuffer),
                          &ow->outputIter, output, strlen(output));
  btk_text_buffer_get_iter_at_offset (BTK_TEXT_BUFFER (ow->outputBuffer),
                                      &ow->outputIter, 0);
}

/**
 * isVisibleDialog:
 *
 * Informs user if a visible test window running.
 *
 * Returns: TRUE if g_visibleDialog is set to 1, otherwise FALSE
 **/
gboolean
isVisibleDialog(void)
{
 if (g_visibleDialog >= 1)
   return TRUE;
 else
   return FALSE;
}

/**
 * get_arg_of_func:
 * @window: The window number
 * @function_name: The name of the function
 * @arg_label: The label of the argument.
 *
 * Gets the user input associated with the @function_name and @arg_label.
 *
 * Returns: the user input associated with the @function_name and @arg_label.
 **/
gchar*
get_arg_of_func (gint  window,
                 gchar *function_name,
                 gchar *arg_label)
{
  const gchar       *argString;
  gchar                      *retString;
  gint                       position, paramPosition;

  position =  _get_position_in_array(window, function_name);

  if (position == -1)
    {
      g_print("No such function\n");
      return NULL;
    }

  paramPosition = _get_position_in_parameters(window, arg_label, position);

  if (paramPosition == -1)
    {
      g_print("No such parameter Label\n");
      return NULL;
    }

  if (position != -1 && paramPosition != -1)
    {
      argString = btk_editable_get_chars (
        BTK_EDITABLE (listoftests[window][position].parameterInput[paramPosition]),
      0, -1);
      retString = g_strdup(argString);
    }
  else
    retString = NULL;

  return retString;
}

/**
 * string_to_int:
 * @the_string: The string to convert
 *
 * Converts the passed-in string to an integer 
 *
 * Returns: An integer corresponding to @the_string.
 **/
int
string_to_int (const char *the_string)
{
  char *end_ptr;
  double ret_val;
  int int_ret_val; 

  while (1)
    {
      ret_val = strtod( the_string, &end_ptr);
      if (*end_ptr == '\0')
        break;
      else
        printf("\nError: input must be a number\n");
    }
   
  int_ret_val = (int) ret_val;
  return (int_ret_val);
}

/** 
 * _toggle_selectedcb:
 * @widget: The ToggleButton widget
 * @test: user data containing the TestList structure.
 *
 * Toggle Button Callback, activating the text entry fields 
 **/
static void
_toggle_selectedcb (BtkWidget *widget,
                    gpointer  test)
{
  int i;
  TestList *testlist = (TestList *) test;
  gboolean toggled;
  gboolean sensitive;
  toggled = btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (widget));
  if (toggled)
    sensitive = TRUE;
  else
    sensitive = FALSE;

  for (i=0; i < testlist->numParameters; i++)
    {
      btk_widget_set_sensitive (BTK_WIDGET (testlist->parameterLabel[i]),
                                sensitive);
      btk_widget_set_sensitive (BTK_WIDGET (testlist->parameterInput[i]),
                                sensitive);
    }
}

/* 
 * _testselectioncb:
 * widget: The Button widget
 * data: The user data containing a TestCB structure
 *
 * Callback for when the "Run Tests" button is pressed 
 **/
static void
_testselectioncb (BtkWidget *widget,
                  gpointer data)
{
  TestCB* local_testcb = (TestCB *)data;
  local_testcb->runtest(local_testcb->obj, local_testcb->win_num);
}

/**
 * _destroy:
 * @widget: The GUI widget
 * @data: User data, not used.
 *
 * Destroy Callback.
 **/
static void
_destroy (BtkWidget *widget,
          gpointer  data)
{
  btk_main_quit();
}

