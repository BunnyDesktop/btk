#include <string.h>
#include <stdlib.h>
#include <batk/batk.h>
#include <btk/btk.h>
#include <testlib.h>

static void _traverse_children (BatkObject *obj);
static void _add_handler (BatkObject *obj);
static void _check_properties (BatkObject *obj);
static void _property_change_handler (BatkObject   *obj,
                                      BatkPropertyValues *values);
static void _state_changed (BatkObject   *obj,
                            const gchar *name,
                            gboolean    set);
static void _selection_changed (BatkObject   *obj);
static void _visible_data_changed (BatkObject   *obj);
static void _model_changed (BatkObject   *obj);
static void _create_event_watcher (void);

static guint id;

static void 
_state_changed (BatkObject   *obj,
                const gchar *name,
                gboolean    set)
{
  g_print ("_state_changed: %s: state %s %s\n", 
           g_type_name (G_OBJECT_TYPE (obj)),
           set ? "is" : "was", name);
}

static void 
_selection_changed (BatkObject   *obj)
{
  gchar *type;

  if (BATK_IS_TEXT (obj))
    type = "text";
  else if (BATK_IS_SELECTION (obj))
    type = "child selection";
  else
    {
      g_assert_not_reached();
      return;
    }

  g_print ("In selection_changed signal handler for %s, object type: %s\n",
           type, g_type_name (G_OBJECT_TYPE (obj)));
}

static void 
_visible_data_changed (BatkObject   *obj)
{
  g_print ("In visible_data_changed signal handler, object type: %s\n",
           g_type_name (G_OBJECT_TYPE (obj)));
}

static void 
_model_changed (BatkObject   *obj)
{
  g_print ("In model_changed signal handler, object type: %s\n",
           g_type_name (G_OBJECT_TYPE (obj)));
}

static void 
_property_change_handler (BatkObject   *obj,
                          BatkPropertyValues   *values)
{
  const gchar *type_name = g_type_name (G_TYPE_FROM_INSTANCE (obj));
  const gchar *name = batk_object_get_name (obj);

  g_print ("_property_change_handler: Accessible Type: %s\n",
           type_name ? type_name : "NULL");
  g_print ("_property_change_handler: Accessible name: %s\n",
           name ? name : "NULL");
  g_print ("_property_change_handler: PropertyName: %s\n",
           values->property_name ? values->property_name: "NULL");
  if (G_VALUE_HOLDS_STRING (&values->new_value))
    g_print ("_property_change_handler: PropertyValue: %s\n",
             g_value_get_string (&values->new_value));
  else if (strcmp (values->property_name, "accessible-child") == 0)
    {
      if (G_IS_VALUE (&values->old_value))
        {
          g_print ("Child is removed: %s\n", 
               g_type_name (G_TYPE_FROM_INSTANCE (g_value_get_pointer (&values->old_value))));
        }
      if (G_IS_VALUE (&values->new_value))
        {
          g_print ("Child is added: %s\n", 
               g_type_name (G_TYPE_FROM_INSTANCE (g_value_get_pointer (&values->new_value))));
        }
    }
  else if (strcmp (values->property_name, "accessible-parent") == 0)
    {
      if (G_IS_VALUE (&values->old_value))
        {
          g_print ("Parent is removed: %s\n", 
               g_type_name (G_TYPE_FROM_INSTANCE (g_value_get_pointer (&values->old_value))));
        }
      if (G_IS_VALUE (&values->new_value))
        {
          g_print ("Parent is added: %s\n", 
               g_type_name (G_TYPE_FROM_INSTANCE (g_value_get_pointer (&values->new_value))));
        }
    }
  else if (strcmp (values->property_name, "accessible-value") == 0)
    {
      if (G_VALUE_HOLDS_DOUBLE (&values->new_value))
        {
          g_print ("Value now is (double) %f\n", 
                   g_value_get_double (&values->new_value));
        }
    }
}

static void 
_traverse_children (BatkObject *obj)
{
  gint n_children, i;
  BatkRole role;
 
  role = batk_object_get_role (obj);

  if ((role == BATK_ROLE_TABLE) ||
      (role == BATK_ROLE_TREE_TABLE))
    return;

  n_children = batk_object_get_n_accessible_children (obj);
  for (i = 0; i < n_children; i++)
    {
      BatkObject *child;

      child = batk_object_ref_accessible_child (obj, i);
      _add_handler (child);
      _traverse_children (child);
      g_object_unref (G_OBJECT (child));
    }
}

static void 
_add_handler (BatkObject *obj)
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
    {
      batk_object_connect_property_change_handler (obj,
                   (BatkPropertyChangeHandler*) _property_change_handler);
      g_signal_connect (obj, "state-change", 
                        (GCallback) _state_changed, NULL);
      if (BATK_IS_SELECTION (obj))
        g_signal_connect (obj, "selection_changed", 
                          (GCallback) _selection_changed, NULL);
      g_signal_connect (obj, "visible_data_changed", 
                        (GCallback) _visible_data_changed, NULL);
      if (BATK_IS_TABLE (obj))
        g_signal_connect (obj, "model_changed", 
                        (GCallback) _model_changed, NULL);
      g_ptr_array_add (obj_array, obj);
    }
}
 
static void 
_check_properties (BatkObject *obj)
{
  BatkRole role;

  g_print ("Start of _check_properties: %s\n", 
           g_type_name (G_OBJECT_TYPE (obj)));

  _add_handler (obj);

  role = batk_object_get_role (obj);
  if (role == BATK_ROLE_FRAME ||
      role == BATK_ROLE_DIALOG)
    {
      /*
       * Add handlers to all children.
       */
      _traverse_children (obj);
    }
  g_print ("End of _check_properties\n");
}

static void
_create_event_watcher (void)
{
  id = batk_add_focus_tracker (_check_properties);
}

int
btk_module_init(gint argc, char* argv[])
{
  g_print("testprops Module loaded\n");

  _create_event_watcher();

  return 0;
}
