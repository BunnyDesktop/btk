#include <batk/batk.h>
#include "testtextlib.h"

#define NUM_VALID_ROLES 6  

static void _create_event_watcher (void);
static void _check_text (BatkObject *obj);
void runtest(BatkObject *, bint);

static buint id1 = 0;
static buint win_count = 0;

static void _check_text (BatkObject *in_obj)
{
  BatkObject *obj = NULL;
  BatkRole role;
  bchar* title;
  BatkRole valid_roles[NUM_VALID_ROLES];

  if (g_getenv("TEST_ACCESSIBLE_DELAY") != NULL)
  {
    int max_cnt = string_to_int(g_getenv("TEST_ACCESSIBLE_DELAY"));
    win_count++;
    if (win_count <= max_cnt)
      return;
  }

  /* Set Up */

  valid_roles[0] = BATK_ROLE_TEXT;
  valid_roles[1] = BATK_ROLE_LABEL;
  valid_roles[2] = BATK_ROLE_ACCEL_LABEL;
  valid_roles[3] = BATK_ROLE_PASSWORD_TEXT;
  valid_roles[4] = BATK_ROLE_TABLE_CELL;
  valid_roles[5] = BATK_ROLE_PANEL;
  
  /* The following if/else grabs the windows name, or sets title to NULL if none. */
  if (in_obj->name)
  {
     title = in_obj->name;
  }
  else
  {
    BtkWidget *toplevel;
    BtkWidget* widget = BTK_ACCESSIBLE (in_obj)->widget;

    if (widget == NULL)
    {
      title = NULL;
    }

    toplevel = btk_widget_get_toplevel (widget);
    if (BTK_IS_WINDOW (toplevel) && BTK_WINDOW (toplevel)->title)
    {
      title = BTK_WINDOW (toplevel)->title;
    }
    else
      title = NULL;
  }
  /* If no window name, do nothing */
  if (title == NULL) 
    return;
  /* 
   * If testtext test program, find obj just by role since only one child 
   * with no name
   */
  else if (g_ascii_strncasecmp(title, "testtext", 7) == 0) 
  {
    obj = find_object_by_role(in_obj, valid_roles, NUM_VALID_ROLES);
  }
  /*
   * Otherwise, get obj by name and role so you can specify exactly which 
   * obj to run tests on 
   */
  else 
  {
    const bchar *test_accessible_name = g_getenv ("TEST_ACCESSIBLE_NAME");

    if (test_accessible_name != NULL)
    {
      obj = find_object_by_accessible_name_and_role(in_obj,
        test_accessible_name, valid_roles, NUM_VALID_ROLES);
    }
    if (obj != NULL)
    {
      if (batk_object_get_role (obj) == BATK_ROLE_PANEL)
      {
        /* Get the child and check whether it is a label */

        obj = batk_object_ref_accessible_child (obj, 0);
        g_assert (batk_object_get_role (obj) == BATK_ROLE_LABEL);
        g_object_unref (obj);
      }
       g_print("Found valid name and role in child!\n");
    }
    else
    {
       obj = find_object_by_role(in_obj, valid_roles, NUM_VALID_ROLES - 1);
       if (obj != NULL)
          g_print("Found valid role in child\n");
    }   
  } 
  if (obj == NULL)
  {
     g_print("Object not found\n");
     return;
  }
  role = batk_object_get_role(obj);

  g_print("_check_text - Found role type %s!\n\n", batk_role_get_name (role));

  add_handlers(obj);

  if (!(isVisibleDialog()))
    setup_gui(obj, runtest);
  batk_remove_focus_tracker (id1);
}

static void
_create_event_watcher (void)
{
  id1 = batk_add_focus_tracker (_check_text);
}

int
btk_module_init(bint argc, char* argv[])
{
  g_print("testtext Module loaded.\n");
  _create_event_watcher();

  return 0;
}


