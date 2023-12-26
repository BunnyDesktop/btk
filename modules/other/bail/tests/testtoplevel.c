#include <batk/batk.h>
#include <btk/btk.h>
#include "testlib.h"

static void _notify_toplevel_child_added (BObject *obj,
  guint index, BatkObject *child, gpointer user_data);
static void _notify_toplevel_child_removed (BObject *obj,
  guint index, BatkObject *child, gpointer user_data);
static gboolean _button_press_event_watcher (GSignalInvocationHint *ihint,
  guint n_param_values, const BValue *param_values, gpointer data);

static guint id;
static gboolean g_register_listener = FALSE;
static guint g_signal_listener = 0;
static gint g_press_count = 0;

static void
_check_toplevel (BatkObject *obj)
{
  BatkObject *root_obj;
  const gchar *name_string, *version_string;
  gint max_depth;

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
btk_module_init(gint argc, char* argv[])
{
  g_print("testtoplevel Module loaded\n");

  _create_event_watcher();

  return 0;
}

static void _notify_toplevel_child_added (BObject *obj,
  guint child_index, BatkObject *child, gpointer user_data)
{
   g_print ("SIGNAL - Child added - index %d\n", child_index);
}

static void _notify_toplevel_child_removed (BObject *obj,
  guint child_index, BatkObject *child, gpointer user_data)
{
   g_print ("SIGNAL - Child removed - index %d\n", child_index);
}

static gboolean
_button_press_event_watcher (GSignalInvocationHint *ihint,
                    guint		   n_param_values,
                    const BValue	  *param_values,
                    gpointer		   data)
{
  BObject *object;
  gchar * button_name = (gchar *) data;

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

