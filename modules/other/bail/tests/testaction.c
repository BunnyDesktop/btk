#include <string.h>
#include <stdlib.h>
#include <btk/btk.h>
#include "testlib.h"

/*
 * This module is used to test the implementation of BatkAction,
 * i.e. the getting of the name and the getting and setting of description
 */

static void _create_event_watcher (void);
static void _check_object (BatkObject *obj);

static void 
_check_object (BatkObject *obj)
{
  const char *accessible_name;
  const bchar * typename = NULL;

  if (BTK_IS_ACCESSIBLE (obj))
  {
    BtkWidget* widget = NULL;

    widget = BTK_ACCESSIBLE (obj)->widget;
    typename = g_type_name (B_OBJECT_TYPE (widget));
    g_print ("Widget type name: %s\n", typename ? typename : "NULL");
  }
  typename = g_type_name (B_OBJECT_TYPE (obj));
  g_print ("Accessible type name: %s\n", typename ? typename : "NULL");
  accessible_name = batk_object_get_name (obj);
  if (accessible_name)
    g_print ("Name: %s\n", accessible_name);

  if (BATK_IS_ACTION (obj))
  {
    BatkAction *action = BATK_ACTION (obj);
    bint n_actions, i;
    const bchar *action_name;
    const bchar *action_desc;
    const bchar *action_binding;
    const bchar *desc = "Test description";
 
    n_actions = batk_action_get_n_actions (action);
    g_print ("BatkAction supported number of actions: %d\n", n_actions);
    for (i = 0; i < n_actions; i++)
    {
      action_name = batk_action_get_name (action, i);
      g_print ("Name of Action %d: %s\n", i, action_name);
      action_binding = batk_action_get_keybinding (action, i);
      if (action_binding)
        g_print ("Name of Action Keybinding %d: %s\n", i, action_binding);
     
      if (!batk_action_set_description (action, i, desc))
      {
        g_print ("batk_action_set_description failed\n");
      }
      else
      {
        action_desc = batk_action_get_description (action, i);
        if (strcmp (desc, action_desc) != 0)
        {
          g_print ("Problem with setting and getting action description\n");
        }
      }
    } 
    if (batk_action_set_description (action, n_actions, desc))
    {
      g_print ("batk_action_set_description succeeded but should not have\n");
    }
  }
}

static void
_create_event_watcher (void)
{
  batk_add_focus_tracker (_check_object);
}

int
btk_module_init(bint argc, char* argv[])
{
  g_print("testaction Module loaded\n");

  _create_event_watcher();

  return 0;
}
