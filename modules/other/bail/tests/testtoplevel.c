#include <batk/batk.h>
#include <btk/btk.h>
#include "testlib.h"

static void _notify_toplevel_child_added (BObject *obj,
  buint index, BatkObject *child, bpointer user_data);
static void _notify_toplevel_child_removed (BObject *obj,
  buint index, BatkObject *child, bpointer user_data);
static bboolean _button_press_event_watcher (GSignalInvocationHint *ihint,
  buint n_param_values, const BValue *param_values, bpointer data);

static buint id;
static bboolean g_register_listener = FALSE;
static buint g_signal_listener = 0;
static bint g_press_count = 0;

static void
_check_toplevel (BatkObject *obj)
{
  BatkObject *root_obj;
  const bchar *name_string, *version_string;
  bint max_depth;

  g_print ("Start of _check_toplevel\n");
  root_obj = batk_get_root();

  if (!already_accessed_batk_object(root_obj))
    {
      g_signal_connect_closure (root_obj, "children_changed::add",
		g_cclosure_new (G_CALLBACK (_notify_toplevel_child_added),
		NULL, NULL),
		FALSE);

      g_signal_connect_closure (root_obj, "children_changed::remove",
		g_cclosure_new (G_CALLBACK (_notify_toplevel_child_removed),
		NULL, NULL),
		FALSE);
    }

  name_string = batk_get_toolkit_name();
  version_string = batk_get_toolkit_version();
  g_print ("Toolkit name <%s> version <%s>\n", name_string,
    version_string);

  if (g_getenv("TEST_ACCESSIBLE_DEPTH") != NULL)
    max_depth = string_to_int(g_getenv("TEST_ACCESSIBLE_DEPTH"));
  else
    max_depth = 2;

  display_children_to_depth(root_obj, max_depth, 0, 0);
  g_print ("End of _check_toplevel\n");

  if (!g_register_listener)
    {
      g_print("Adding global event listener on buttons\n");
      g_register_listener = TRUE;
      g_signal_listener = batk_add_global_event_listener(_button_press_event_watcher,
        "Btk:BtkButton:pressed");
    }
}

static void
_create_event_watcher (void)
{
  id = batk_add_focus_tracker (_check_toplevel);
}

int
btk_module_init(bint argc, char* argv[])
{
  g_print("testtoplevel Module loaded\n");

  _create_event_watcher();

  return 0;
}

static void _notify_toplevel_child_added (BObject *obj,
  buint child_index, BatkObject *child, bpointer user_data)
{
   g_print ("SIGNAL - Child added - index %d\n", child_index);
}

static void _notify_toplevel_child_removed (BObject *obj,
  buint child_index, BatkObject *child, bpointer user_data)
{
   g_print ("SIGNAL - Child removed - index %d\n", child_index);
}

static bboolean
_button_press_event_watcher (GSignalInvocationHint *ihint,
                    buint		   n_param_values,
                    const BValue	  *param_values,
                    bpointer		   data)
{
  BObject *object;
  bchar * button_name = (bchar *) data;

  object = b_value_get_object (param_values + 0);

  if (BATK_IS_IMPLEMENTOR(object))
    {
      BatkObject * batk_obj =
        batk_implementor_ref_accessible(BATK_IMPLEMENTOR(object));

      g_print("Button <%s> pressed %d times!\n", button_name,
        (g_press_count + 1));
      g_print("Displaying children of Button pressed!\n");
      display_children(batk_obj, 0, 0);

      if (g_press_count >= 5)
        {
          g_print("Removing global event listener on buttons\n");
          batk_remove_global_event_listener(g_signal_listener);
          g_signal_listener = 0;
          g_press_count = 0;
          g_register_listener = FALSE;
        }
      else
        {
          g_press_count++;
        }
    }

  return TRUE;
}

