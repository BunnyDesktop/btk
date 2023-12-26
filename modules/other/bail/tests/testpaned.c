#include <string.h>
#include <stdlib.h>
#include <btk/btk.h>
#include <testlib.h>

static bint _test_paned (bpointer data);
static void _check_paned (BatkObject *obj);

static void _property_change_handler (BatkObject   *obj,
                                      BatkPropertyValues *values);

#define NUM_VALID_ROLES 1
static bint last_position;

static void _property_change_handler (BatkObject   *obj,
                                      BatkPropertyValues   *values)
{
  const bchar *type_name = g_type_name (B_TYPE_FROM_INSTANCE (obj));
  const bchar *name = batk_object_get_name (obj);

  g_print ("_property_change_handler: Accessible Type: %s\n",
           type_name ? type_name : "NULL");
  g_print ("_property_change_handler: Accessible name: %s\n",
           name ? name : "NULL");
  g_print ("_property_change_handler: PropertyName: %s\n",
           values->property_name ? values->property_name: "NULL");
  if (strcmp (values->property_name, "accessible-value") == 0)
  {
    BValue *value, val;
    int position;
    value = &val;

    memset (value, 0, sizeof (BValue));
    batk_value_get_current_value (BATK_VALUE (obj), value);
    g_return_if_fail (G_VALUE_HOLDS_INT (value));
    position = b_value_get_int (value); 
    g_print ("Position is  %d previous position was %d\n", 
             position, last_position);
    last_position = position;
    batk_value_get_minimum_value (BATK_VALUE (obj), value);
    g_return_if_fail (G_VALUE_HOLDS_INT (value));
    position = b_value_get_int (value); 
    g_print ("Minimum Value is  %d\n", position); 
    batk_value_get_maximum_value (BATK_VALUE (obj), value);
    g_return_if_fail (G_VALUE_HOLDS_INT (value));
    position = b_value_get_int (value); 
    g_print ("Maximum Value is  %d\n", position); 
  }
}
 
static bint _test_paned (bpointer data)
{
  BatkObject *obj = BATK_OBJECT (data);
  BatkRole role = batk_object_get_role (obj);
  BtkWidget *widget;
  static bint times = 0;

  widget = BTK_ACCESSIBLE (obj)->widget;

  if (role == BATK_ROLE_SPLIT_PANE)
  {
    BValue *value, val;
    int position;
    value = &val;

    memset (value, 0, sizeof (BValue));
    batk_value_get_current_value (BATK_VALUE (obj), value);
    g_return_val_if_fail (G_VALUE_HOLDS_INT (value), FALSE);
    position = b_value_get_int (value); 
    g_print ("Position is : %d\n", position);
    last_position = position;
    position *= 2;
    b_value_set_int (value, position);
    batk_value_set_current_value (BATK_VALUE (obj), value);
    times++;
  }
  if (times < 4)
    return TRUE;
  else
    return FALSE;
}

static void _check_paned (BatkObject *obj)
{
  static bboolean done_paned = FALSE;
  BatkRole role;

  role = batk_object_get_role (obj);

  if (role == BATK_ROLE_FRAME)
  {
    BatkRole roles[NUM_VALID_ROLES];
    BatkObject *paned_obj;

    if (done_paned)
      return;

    roles[0] = BATK_ROLE_SPLIT_PANE;

    paned_obj = find_object_by_role (obj, roles, NUM_VALID_ROLES);

    if (paned_obj)
    {
      if (!done_paned)
      {
        done_paned = TRUE;
      }
      batk_object_connect_property_change_handler (paned_obj,
                   (BatkPropertyChangeHandler*) _property_change_handler);
      g_timeout_add (2000, _test_paned, paned_obj);
    }

    return;
  }
  if (role != BATK_ROLE_COMBO_BOX)
    return;
}

static void
_create_event_watcher (void)
{
  batk_add_focus_tracker (_check_paned);
}

int
btk_module_init(bint argc, char* argv[])
{
  g_print("testpaned Module loaded\n");

  _create_event_watcher();

  return 0;
}
