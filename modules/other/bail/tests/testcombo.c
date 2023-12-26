#undef BTK_DISABLE_DEPRECATED

#include <btk/btk.h>
#include "testlib.h"

static void _test_selection (BatkObject *obj);
static void _check_combo_box (BatkObject *obj);
static void _check_children (BatkObject *obj);
static bint _open_combo_list (bpointer data);
static bint _close_combo_list (bpointer data);

#define NUM_VALID_ROLES 1

static void _check_children (BatkObject *obj)
{
  bint n_children, i, j;
  BatkObject *child;
  BatkObject *grand_child;
  BatkObject *parent;

  n_children = batk_object_get_n_accessible_children (obj);

  if (n_children > 1)
  {
    g_print ("*** Unexpected number of children for combo box: %d\n", 
             n_children);
    return;
  }
  if (n_children == 2)
  {
    child = batk_object_ref_accessible_child (obj, 1);
    g_return_if_fail (batk_object_get_role (child) == BATK_ROLE_TEXT);
    parent = batk_object_get_parent (child);
    j = batk_object_get_index_in_parent (child);
    if (j != 1)
     g_print ("*** inconsistency between parent and children %d %d ***\n",
              1, j);       
    g_object_unref (B_OBJECT (child));
  }

  child = batk_object_ref_accessible_child (obj, 0);
  g_return_if_fail (batk_object_get_role (child) == BATK_ROLE_LIST);
  parent = batk_object_get_parent (child);
  j = batk_object_get_index_in_parent (child);
  if (j != 0)
     g_print ("*** inconsistency between parent and children %d %d ***\n",
              0, j);       

  n_children = batk_object_get_n_accessible_children (child);
  for (i = 0; i < n_children; i++)
  {
    const bchar *name;

    grand_child = batk_object_ref_accessible_child (child, i);
    name = batk_object_get_name (grand_child);
    g_print ("Index: %d Name: %s\n", i, name ? name : "<NULL>");
    g_object_unref (B_OBJECT (grand_child));
  }
  g_object_unref (B_OBJECT (child));
}
  
static void _test_selection (BatkObject *obj)
{
  bint count;
  bint n_children;
  BatkObject *list;

  count = batk_selection_get_selection_count (BATK_SELECTION (obj));
  g_return_if_fail (count == 0);

  list = batk_object_ref_accessible_child (obj, 0);
  n_children = batk_object_get_n_accessible_children (list); 
  g_object_unref (B_OBJECT (list));

  batk_selection_add_selection (BATK_SELECTION (obj), n_children - 1);
  count = batk_selection_get_selection_count (BATK_SELECTION (obj));
  g_return_if_fail (count == 1);
  g_return_if_fail (batk_selection_is_child_selected (BATK_SELECTION (obj),
                     n_children - 1));	
  batk_selection_add_selection (BATK_SELECTION (obj), 0);
  count = batk_selection_get_selection_count (BATK_SELECTION (obj));
  g_return_if_fail (count == 1);
  g_return_if_fail (batk_selection_is_child_selected (BATK_SELECTION (obj), 0));
  batk_selection_clear_selection (BATK_SELECTION (obj));
  count = batk_selection_get_selection_count (BATK_SELECTION (obj));
  g_return_if_fail (count == 0);
}

static void _check_combo_box (BatkObject *obj)
{
  static bboolean done = FALSE;
  static bboolean done_selection = FALSE;
  BatkRole role;

  role = batk_object_get_role (obj);

  if (role == BATK_ROLE_FRAME)
  {
    BatkRole roles[NUM_VALID_ROLES];
    BatkObject *combo_obj;

    if (done_selection)
      return;

    roles[0] = BATK_ROLE_COMBO_BOX;

    combo_obj = find_object_by_role (obj, roles, NUM_VALID_ROLES);

    if (combo_obj)
    {
      if (!done_selection)
      {
        done_selection = TRUE;
      }
      if (g_getenv ("TEST_ACCESSIBLE_COMBO_NOEDIT") != NULL)
      {
        BtkEntry *entry;

        entry = BTK_ENTRY (BTK_COMBO (BTK_ACCESSIBLE (combo_obj)->widget)->entry);
        btk_entry_set_editable (entry, FALSE);
      }
      _check_children (combo_obj);
      _test_selection (combo_obj);
    }

    return;
  }
  if (role != BATK_ROLE_COMBO_BOX)
    return;

  g_print ("*** Start ComboBox ***\n");
  _check_children (obj);
 
  if (!done)
  {
    btk_idle_add (_open_combo_list, obj);
    done = TRUE;
  }
  else
      return;
  g_print ("*** End ComboBox ***\n");
}

static bint _open_combo_list (bpointer data)
{
  BatkObject *obj = BATK_OBJECT (data);

  g_print ("_open_combo_list\n");
  batk_action_do_action (BATK_ACTION (obj), 0);

  btk_timeout_add (5000, _close_combo_list, obj);
  return FALSE;
}

static bint _close_combo_list (bpointer data)
{
  BatkObject *obj = BATK_OBJECT (data);

  bint count;
  bint n_children;
  BatkObject *list;

  count = batk_selection_get_selection_count (BATK_SELECTION (obj));
  g_return_val_if_fail (count == 0, FALSE);

  list = batk_object_ref_accessible_child (obj, 0);
  n_children = batk_object_get_n_accessible_children (list); 
  g_object_unref (B_OBJECT (list));

  batk_selection_add_selection (BATK_SELECTION (obj), n_children - 1);

  batk_action_do_action (BATK_ACTION (obj), 0);

  return FALSE;
}

static void
_create_event_watcher (void)
{
  batk_add_focus_tracker (_check_combo_box);
}

int
btk_module_init(bint argc, char* argv[])
{
  g_print("testcombo Module loaded\n");

  _create_event_watcher();

  return 0;
}
