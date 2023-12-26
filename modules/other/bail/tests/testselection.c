#include <string.h>
#include <batk/batk.h>
#include <btk/btk.h>

/*
 * This module tests the selection interface on menu items.
 * To use this module run the test program testbtk and use the menus 
 * option.
 */
static void _do_selection (BatkObject *obj);
static gint _finish_selection (gpointer data);
static BatkObject* _find_object (BatkObject* obj, BatkRole role);
static void _print_type (BatkObject *obj);

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
  gint i;
  gint n_children;
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

static void _print_type (BatkObject *obj)
{
  const gchar * typename = NULL;
  const gchar * name = NULL;
  BatkRole role;

  if (BTK_IS_ACCESSIBLE (obj))
  {
    BtkWidget* widget = NULL;

    widget = BTK_ACCESSIBLE (obj)->widget;
    typename = g_type_name (G_OBJECT_TYPE (widget));
    g_print ("Widget type name: %s\n", typename ? typename : "NULL");
  }
  typename = g_type_name (G_OBJECT_TYPE (obj));
  g_print ("Accessible type name: %s\n", typename ? typename : "NULL");
  name = batk_object_get_name (obj);
  g_print("Accessible Name: %s\n", (name) ? name : "NULL");
  role = batk_object_get_role(obj);
  g_print ("Accessible Role: %d\n", role);
}

static void 
_do_selection (BatkObject *obj)
{
  gint i;
  BatkObject *selected;
  BatkRole role;
  BatkObject *selection_obj;

  role = batk_object_get_role (obj);

  if ((role == BATK_ROLE_FRAME) &&
      (strcmp (batk_object_get_name (obj), "menus") == 0))
  {
    selection_obj = _find_object (obj, BATK_ROLE_MENU_BAR);
    if (selection_obj)
    {
      g_object_unref (selection_obj);
    }
  }
  else if (role == BATK_ROLE_COMBO_BOX)
  {
    selection_obj = obj;
  }
  else
    return;

  g_print ("*** Start do_selection ***\n");

  
  if (!selection_obj)
  {
    g_print ("no selection_obj\n");
    return;
  }

  i = batk_selection_get_selection_count (BATK_SELECTION (selection_obj));
  if (i != 0)
  {
    for (i = 0; i < batk_object_get_n_accessible_children (selection_obj); i++)
    {
      if (batk_selection_is_child_selected (BATK_SELECTION (selection_obj), i))
      {
        g_print ("%d child selected\n", i);
      }
      else
      {
        g_print ("%d child not selected\n", i);
      }
    } 
  } 
  /*
   * Should not be able to select all items on a menu bar
   */
  batk_selection_select_all_selection (BATK_SELECTION (selection_obj));
  i = batk_selection_get_selection_count (BATK_SELECTION (selection_obj));
  if ( i != 0)
  {
    g_print ("Unexpected selection count: %d, expected 0\n", i);
    return;
  }
  /*
   * There should not be any items selected
   */
  selected = batk_selection_ref_selection (BATK_SELECTION (selection_obj), 0);
  if ( selected != NULL)
  {
    g_print ("Unexpected selection: %d, expected 0\n", i);
  }
  batk_selection_add_selection (BATK_SELECTION (selection_obj), 1);
  g_timeout_add (2000, _finish_selection, selection_obj);
  g_print ("*** End _do_selection ***\n");
} 

static gint _finish_selection (gpointer data)
{
  BatkObject *obj = BATK_OBJECT (data);
  BatkObject *selected;
  gint      i;
  gboolean is_selected;

  g_print ("*** Start Finish selection ***\n");

  /*
   * If being run for for menus, at this point menu item foo should be 
   * selected which means that its submenu should be visible.
   */
  i = batk_selection_get_selection_count (BATK_SELECTION (obj));
  if (i != 1)
  {
    g_print ("Unexpected selection count: %d, expected 1\n", i);
    return FALSE;
  }
  selected = batk_selection_ref_selection (BATK_SELECTION (obj), 0);
  g_return_val_if_fail (selected != NULL, FALSE);
  g_print ("*** Selected Item ***\n");
  _print_type (selected);
  g_object_unref (selected);
  is_selected = batk_selection_is_child_selected (BATK_SELECTION (obj), 1);
  g_return_val_if_fail (is_selected, FALSE);
  is_selected = batk_selection_is_child_selected (BATK_SELECTION (obj), 0);
  g_return_val_if_fail (!is_selected, FALSE);
  selected = batk_selection_ref_selection (BATK_SELECTION (obj), 1);
  g_return_val_if_fail (selected == NULL, FALSE);
  batk_selection_remove_selection (BATK_SELECTION (obj), 0);
  i = batk_selection_get_selection_count (BATK_SELECTION (obj));
  g_return_val_if_fail (i == 0, FALSE);
  selected = batk_selection_ref_selection (BATK_SELECTION (obj), 0);
  g_return_val_if_fail (selected == NULL, FALSE);
  g_print ("*** End Finish selection ***\n");
  return FALSE;
}

static void
_create_event_watcher (void)
{
  batk_add_focus_tracker (_do_selection);
}

int
btk_module_init(gint argc, char* argv[])
{
  g_print("testselection Module loaded\n");

  _create_event_watcher();

  return 0;
}
