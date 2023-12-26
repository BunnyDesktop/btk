#include <string.h>
#include <batk/batk.h>
#include <btk/btk.h>

static void _traverse_children (BatkObject *obj);
static void _add_handler (BatkObject *obj);
static void _check_values (BatkObject *obj);
static void _value_change_handler (BatkObject   *obj,
                                      BatkPropertyValues *values);

static buint id;

static void _value_change_handler (BatkObject   *obj,
                                   BatkPropertyValues   *values)
{
  const bchar *type_name = g_type_name (B_TYPE_FROM_INSTANCE (obj));
   BValue *value_back, val;

  value_back = &val;
    
  if (!BATK_IS_VALUE (obj)) { 
   	return;
  }

  if (strcmp (values->property_name, "accessible-value") == 0) {
	g_print ("_value_change_handler: Accessible Type: %s\n",
           type_name ? type_name : "NULL");
	if(G_VALUE_HOLDS_DOUBLE (&values->new_value))
    {
		g_print( "adjustment value changed : new value: %f\n", 
		b_value_get_double (&values->new_value));
 	}

	g_print("Now calling the BatkValue interface functions\n");

  	batk_value_get_current_value (BATK_VALUE(obj), value_back);
  	g_return_if_fail (G_VALUE_HOLDS_DOUBLE (value_back));
  	g_print ("batk_value_get_current_value returns %f\n",
			b_value_get_double (value_back)	);

  	batk_value_get_maximum_value (BATK_VALUE (obj), value_back);
  	g_return_if_fail (G_VALUE_HOLDS_DOUBLE (value_back));
  	g_print ("batk_value_get_maximum returns %f\n",
			b_value_get_double (value_back));

  	batk_value_get_minimum_value (BATK_VALUE (obj), value_back);
  	g_return_if_fail (G_VALUE_HOLDS_DOUBLE (value_back));
  	g_print ("batk_value_get_minimum returns %f\n", 
			b_value_get_double (value_back));
	
 
    }
  
}

static void _traverse_children (BatkObject *obj)
{
  bint n_children, i;

  n_children = batk_object_get_n_accessible_children (obj);
  for (i = 0; i < n_children; i++)
  {
    BatkObject *child;

    child = batk_object_ref_accessible_child (obj, i);
    _add_handler (child);
    _traverse_children (child);
    g_object_unref (B_OBJECT (child));
  }
}

static void _add_handler (BatkObject *obj)
{
  static GPtrArray *obj_array = NULL;
  bboolean found = FALSE;
  bint i;

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
  {
    batk_object_connect_property_change_handler (obj,
                   (BatkPropertyChangeHandler*) _value_change_handler);
    g_ptr_array_add (obj_array, obj);
  }
}

static void _set_values (BatkObject *obj) {

  BValue *value_back, val;
  static bint count = 0;
  bdouble double_value;

  value_back = &val;

  if(BATK_IS_VALUE(obj)) {
	/* Spin button also inherits the text interfaces from BailEntry. 
	 * Check when spin button recieves focus.
     */

	if(BATK_IS_TEXT(obj) && BATK_IS_EDITABLE_TEXT(obj)) {
		if(count == 0) {	
			bint x;
			bchar* text;
			count++;
			x = batk_text_get_character_count (BATK_TEXT (obj));
  			text = batk_text_get_text (BATK_TEXT (obj), 0, x);
			g_print("Text : %s\n", text);
			text = "5.7";
			batk_editable_text_set_text_contents(BATK_EDITABLE_TEXT(obj),text);
			g_print("Set text to %s\n",text);
			batk_value_get_current_value(BATK_VALUE(obj), value_back);
			g_return_if_fail (G_VALUE_HOLDS_DOUBLE (value_back));
			g_print("batk_value_get_current_value returns %f\n", 
				b_value_get_double( value_back));
			} 
	} else {
		memset (value_back, 0, sizeof (BValue));
		b_value_init (value_back, B_TYPE_DOUBLE);
		b_value_set_double (value_back, 10.0);	
		if (batk_value_set_current_value (BATK_VALUE (obj), value_back))
		{
 			double_value = b_value_get_double (value_back);
  			g_print("batk_value_set_current_value returns %f\n", 
			double_value);
		}
	}
  }
}

static void _check_values (BatkObject *obj)
{
  static bint calls = 0;
  BatkRole role;

  g_print ("Start of _check_values\n");

  _set_values(obj);

  _add_handler (obj);

  if (++calls < 2)
  { 
    /*
     * Just do this on this on the first 2 objects visited
     */
    batk_object_set_name (obj, "test123");
    batk_object_set_description (obj, "test123");
  }

  role = batk_object_get_role (obj);

  if (role == BATK_ROLE_FRAME || role == BATK_ROLE_DIALOG)
  {
    /*
     * Add handlers to all children.
     */
    _traverse_children (obj);
  }
  g_print ("End of _check_values\n");
}

static void
_create_event_watcher (void)
{
  id = batk_add_focus_tracker (_check_values);
}

int
btk_module_init(bint argc, char* argv[])
{
  g_print("testvalues Module loaded\n");

  _create_event_watcher();

  return 0;
}
