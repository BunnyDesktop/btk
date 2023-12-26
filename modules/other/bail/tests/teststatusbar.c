#include <string.h>
#include <bunnylib-object.h>
#include <batk/batk.h>

/*
 * To use this test module, run the test program testbtk and click on 
 * statusbar
 */

static void _check_statusbar (BatkObject *obj);
static BatkObject* _find_object (BatkObject* obj, BatkRole role);
static void _notify_handler (BObject *obj, BParamSpec *pspec);
static void _property_change_handler (BatkObject   *obj,
                                      BatkPropertyValues *values);

static BatkObject*
_find_object (BatkObject *obj,
              BatkRole   role)
{
  /*
   * Find the first object which is a descendant of the specified object
   * which matches the specified role.
   *
   * This function returns a reference to the BatkObject which should be
   * removed when finished with the object.
   */
  bint i;
  bint n_children;
  BatkObject *child;

  n_children = batk_object_get_n_accessible_children (obj);
  for (i = 0; i < n_children; i++)
  {
    BatkObject* found_obj;

    child = batk_object_ref_accessible_child (obj, i);
    if (batk_object_get_role (child) == role)
    {
      return child;
    }
    found_obj = _find_object (child, role);
    g_object_unref (child);
    if (found_obj)
    {
      return found_obj;
    }
  }
  return NULL;
}

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
  if (G_VALUE_HOLDS_STRING (&values->new_value))
    g_print ("_property_change_handler: PropertyValue: %s\n",
             b_value_get_string (&values->new_value));
}

static void _check_statusbar (BatkObject *obj)
{
  BatkRole role;
  BatkObject *statusbar, *label;

  role = batk_object_get_role (obj);
  if (role != BATK_ROLE_FRAME)
    return;

  statusbar = _find_object (obj, BATK_ROLE_STATUSBAR); 
  if (!statusbar)
    return;
  g_print ("_check_statusbar\n");
  label = batk_object_ref_accessible_child (statusbar, 0);
  g_return_if_fail (label == NULL);

  /*
   * We get notified of changes to the label
   */
  g_signal_connect_closure_by_id (statusbar,
                                  g_signal_lookup ("notify", 
                                                   B_OBJECT_TYPE (statusbar)),
                                  0,
                                  g_cclosure_new (G_CALLBACK (_notify_handler),
                                                 NULL, NULL),
                                  FALSE);
  batk_object_connect_property_change_handler (statusbar,
                   (BatkPropertyChangeHandler*) _property_change_handler);

}

static void 
_notify_handler (BObject *obj, BParamSpec *pspec)
{
  BatkObject *batk_obj = BATK_OBJECT (obj);
  const bchar *name;

  g_print ("_notify_handler: property: %s\n", pspec->name);
  if (strcmp (pspec->name, "accessible-name") == 0)
  {
    name = batk_object_get_name (batk_obj);
    g_print ("_notify_handler: value: |%s|\n", name ? name : "<NULL>");
  }
}

static void
_create_event_watcher (void)
{
  batk_add_focus_tracker (_check_statusbar);
}

int
btk_module_init(bint argc, char* argv[])
{
  g_print("teststatusbar Module loaded\n");

  _create_event_watcher();

  return 0;
}
