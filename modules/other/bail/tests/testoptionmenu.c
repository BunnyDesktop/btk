#undef BTK_DISABLE_DEPRECATED

#include <btk/btk.h>

#include "testlib.h"

/*
 * This module is used to test the accessible implementation for BtkOptionMenu
 *
 * When the BtkOption menu in the FileSelectionDialog is tabbed to, the menu
 * is opened and the second item in the menu is selected which causes the 
 * menu to be closed and the item in the BtkOptionMenu to be updated.
 */
#define NUM_VALID_ROLES 1

static void _create_event_watcher (void);
static void _check_object (BatkObject *obj);
static bint _do_menu_item_action (bpointer data);
static bboolean doing_action = FALSE;

static void 
_check_object (BatkObject *obj)
{
  BatkRole role;
  static const char *name = NULL;
  static bboolean first_time = TRUE;

  role = batk_object_get_role (obj);
  if (role == BATK_ROLE_PUSH_BUTTON)
  /*
   * Find the specified optionmenu item
   */
    {
      BatkRole valid_roles[NUM_VALID_ROLES];
      BatkObject *batk_option_menu;
      BtkWidget *widget;

      if (name == NULL)
      {
        name = g_getenv ("TEST_ACCESSIBLE_NAME");
        if (name == NULL)
          name = "foo";
      }
      valid_roles[0] = BATK_ROLE_PUSH_BUTTON;
      batk_option_menu = find_object_by_accessible_name_and_role (obj, name,
                               valid_roles, NUM_VALID_ROLES);

      if (batk_option_menu == NULL)
        {
          g_print ("Object not found for %s\n", name);
          return;
        }
      else
        {
          g_print ("Object found for %s\n", name);
        }


      g_assert (BTK_IS_ACCESSIBLE (batk_option_menu));
      widget = BTK_ACCESSIBLE (batk_option_menu)->widget;
      g_assert (BTK_IS_OPTION_MENU (widget));

      if (first_time)
        first_time = FALSE;
      else
        return;

      /*
       * This action opens the BtkOptionMenu whose name is "foo" or whatever
       * was specified in the environment variable TEST_ACCESSIBLE_NAME
       */
      batk_action_do_action (BATK_ACTION (batk_option_menu), 0);
    }
  else if ((role == BATK_ROLE_MENU_ITEM) ||
           (role == BATK_ROLE_CHECK_MENU_ITEM) ||
           (role == BATK_ROLE_RADIO_MENU_ITEM) ||
           (role == BATK_ROLE_TEAR_OFF_MENU_ITEM))
    {
      BatkObject *parent, *child;
      BatkRole parent_role;

      /*
       * If we receive focus while waiting for the menu to be closed
       * we return immediately
       */
      if (doing_action)
        return;

      parent = batk_object_get_parent (obj);
      parent_role = batk_object_get_role (parent);
      g_assert (parent_role == BATK_ROLE_MENU);
    
      child = batk_object_ref_accessible_child (parent, 1);
      doing_action = TRUE;
      g_timeout_add (5000, _do_menu_item_action, child);
    }
  else
    {
      const char *accessible_name;

      accessible_name = batk_object_get_name (obj);
      if (accessible_name)
        {
          g_print ("Name: %s\n", accessible_name);
        } 
      else if (BTK_IS_ACCESSIBLE (obj))
        {
          BtkWidget *widget = BTK_ACCESSIBLE (obj)->widget;
          g_print ("Type: %s\n", g_type_name (B_OBJECT_TYPE (widget)));
        } 
      if (role == BATK_ROLE_TABLE)
        {
          bint n_cols, i;

          n_cols = batk_table_get_n_columns (BATK_TABLE (obj));
          g_print ("Number of Columns: %d\n", n_cols);

          for (i  = 0; i < n_cols; i++)
            {
              BatkObject *header;

              header = batk_table_get_column_header (BATK_TABLE (obj), i);
              g_print ("header: %s %s\n", 
                           g_type_name (B_OBJECT_TYPE (header)),
                           batk_object_get_name (header));
            }
        }
    }
}

static bint _do_menu_item_action (bpointer data)
{
  BatkObject *obj = BATK_OBJECT (data);

  batk_action_do_action (BATK_ACTION (obj), 0);
  doing_action = FALSE;

  g_object_unref (obj);

  return FALSE;
}

static void
_create_event_watcher (void)
{
  batk_add_focus_tracker (_check_object);
}

int
btk_module_init(bint argc, char* argv[])
{
  g_print("testoptionmenu Module loaded\n");

  _create_event_watcher();

  return 0;
}
