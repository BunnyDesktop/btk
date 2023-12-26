#include <stdio.h>

#include <bunnylib.h>
#include <batk/batk.h>
#include <btk/btk.h>
#include "testlib.h"

#define NUM_VALID_ROLES 1

static void _print_type (BatkObject *obj);
static void _do_selection (BatkObject *obj);
static gint _finish_selection (gpointer data);
static gint _remove_page (gpointer data);

static void _print_type (BatkObject *obj)
{
  const gchar *typename = NULL;
  const gchar *name = NULL;
  const gchar *description = NULL;
  BatkRole role;

  if (BTK_IS_ACCESSIBLE (obj))
  {
    BtkWidget* widget = NULL;

    widget = BTK_ACCESSIBLE (obj)->widget;
    typename = g_type_name (G_OBJECT_TYPE (widget));
    g_print ("\tWidget type name: %s\n", typename ? typename : "NULL");
  }

  typename = g_type_name (G_OBJECT_TYPE (obj));
  g_print ("\tAccessible type name: %s\n", typename ? typename : "NULL");
  
  name = batk_object_get_name (obj);
  g_print("\tAccessible Name: %s\n", (name) ? name : "NULL");
  
  role = batk_object_get_role(obj);
  g_print ("\tAccessible Role: %d\n", role);
  
  description = batk_object_get_description (obj);
  g_print ("\tAccessible Description: %s\n", (description) ? description : "NULL");
  if (role ==  BATK_ROLE_PAGE_TAB)
  {
    BatkObject *parent, *child;
    gint x, y, width, height;

    x = y = width = height = 0;
    batk_component_get_extents (BATK_COMPONENT (obj), &x, &y, &width, &height,
                               BATK_XY_SCREEN);
    g_print ("obj: x: %d y: %d width: %d height: %d\n", x, y, width, height);
    x = y = width = height = 0;
    batk_component_get_extents (BATK_COMPONENT (obj), &x, &y, &width, &height,
                               BATK_XY_WINDOW);
    g_print ("obj: x: %d y: %d width: %d height: %d\n", x, y, width, height);
    parent = batk_object_get_parent (obj);
    x = y = width = height = 0;
    batk_component_get_extents (BATK_COMPONENT (parent), &x, &y, &width, &height,
                               BATK_XY_SCREEN);
    g_print ("parent: x: %d y: %d width: %d height: %d\n", x, y, width, height);
    x = y = width = height = 0;
    batk_component_get_extents (BATK_COMPONENT (parent), &x, &y, &width, &height,
                               BATK_XY_WINDOW);
    g_print ("parent: x: %d y: %d width: %d height: %d\n", x, y, width, height);

    child = batk_object_ref_accessible_child (obj, 0);
    x = y = width = height = 0;
    batk_component_get_extents (BATK_COMPONENT (child), &x, &y, &width, &height,
                               BATK_XY_SCREEN);
    g_print ("child: x: %d y: %d width: %d height: %d\n", x, y, width, height);
    x = y = width = height = 0;
    batk_component_get_extents (BATK_COMPONENT (child), &x, &y, &width, &height,
                               BATK_XY_WINDOW);
    g_print ("child: x: %d y: %d width: %d height: %d\n", x, y, width, height);

    g_object_unref (child);
  }
}


static void
_do_selection (BatkObject *obj)
{
  gint i;
  gint n_children;
  BatkRole role;
  BatkObject *selection_obj;
  static gboolean done_selection = FALSE;
  
  if (done_selection)
    return;

  role = batk_object_get_role (obj);

  if (role == BATK_ROLE_FRAME)
  {
    BatkRole roles[NUM_VALID_ROLES];

    roles[0] = BATK_ROLE_PAGE_TAB_LIST;

    selection_obj = find_object_by_role (obj, roles, NUM_VALID_ROLES);

    if (selection_obj)
    {
      done_selection = TRUE;
    }
    else
      return;
  }
  else
  {
    return;
  }

  g_print ("*** Start do_selection ***\n");
  
  n_children = batk_object_get_n_accessible_children (selection_obj);
  g_print ("*** Number of notebook pages: %d\n", n_children); 

  for (i = 0; i < n_children; i++)
  {
    if (batk_selection_is_child_selected (BATK_SELECTION (selection_obj), i))
    {
      g_print ("%d page selected\n", i);
    }
    else
    {
      g_print ("%d page not selected\n", i);
    }
  }
  /*
   * Should not be able to select all items in a notebook.
   */
  batk_selection_select_all_selection (BATK_SELECTION (selection_obj));
  i = batk_selection_get_selection_count (BATK_SELECTION (selection_obj));
  if ( i != 1)
  {
    g_print ("Unexpected selection count: %d, expected 1\n", i);
    g_print ("\t value of i is: %d\n", i);
    return;
  }

  for (i = 0; i < n_children; i++)
  {
    batk_selection_add_selection (BATK_SELECTION (selection_obj), i);
	
    if (batk_selection_is_child_selected (BATK_SELECTION (selection_obj), i))
    {
      g_print ("Page %d: successfully selected\n", i);
      _finish_selection (selection_obj);
    }
    else
    {
      g_print ("ERROR: child %d: selection failed\n", i);
    }
  }
  g_print ("*** End _do_selection ***\n");
  g_timeout_add (5000, _remove_page, selection_obj);
} 

static gint _remove_page (gpointer data)
{
  BatkObject *obj = BATK_OBJECT (data);
  BtkWidget *widget = NULL;

  if (BTK_IS_ACCESSIBLE (obj))
    widget = BTK_ACCESSIBLE (obj)->widget;
     
  g_return_val_if_fail (BTK_IS_NOTEBOOK (widget), FALSE);
  btk_notebook_remove_page (BTK_NOTEBOOK (widget), 4);
  return FALSE;
}

static gint _finish_selection (gpointer data)
{
  BatkObject *obj = BATK_OBJECT (data);
  BatkObject *selected;
  BatkObject *parent_object;
  BtkWidget *parent_widget;
  gint      i, index;

  g_print ("\t*** Start Finish selection ***\n");
  
  i = batk_selection_get_selection_count (BATK_SELECTION (obj));
  if (i != 1)
  {
    g_print ("\tUnexpected selection count: %d, expected 1\n", i);
    return FALSE;
  }
  selected = batk_selection_ref_selection (BATK_SELECTION (obj), 0);
  g_return_val_if_fail (selected != NULL, FALSE);
  
  g_print ("\t*** Selected Item ***\n");
  index = batk_object_get_index_in_parent (selected);
  g_print ("\tIndex in parent is: %d\n", index);
  
  parent_object = batk_object_get_parent (selected);
  g_return_val_if_fail (BATK_IS_OBJECT (parent_object), FALSE);
  g_return_val_if_fail (parent_object == obj, FALSE);
  parent_widget = BTK_ACCESSIBLE (parent_object)->widget;
  g_return_val_if_fail (BTK_IS_NOTEBOOK (parent_widget), FALSE);
  
  _print_type (selected);
  i = batk_selection_get_selection_count (BATK_SELECTION (obj));
  g_return_val_if_fail (i == 1, FALSE);
  g_object_unref (selected);
  g_print ("\t*** End Finish selection ***\n");
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
  g_print("testnotebook Module loaded\n");

  _create_event_watcher();

  return 0;
}
