#include <btk/btk.h>
#include "testlib.h"

/*
 * This module is used to test the accessible implementation for buttons
 *
 * 1) It verifies that BATK_STATE_ARMED is set when a button is pressed
 * To check this click on the button whose name is specified in the
 * environment variable TEST_ACCESSIBLE_NAME or "button box" if the
 * environment variable is not set.
 *
 * 2) If the environment variable TEST_ACCESSIBLE_AUTO is set the program
 * will execute the action defined for a BailButton once.
 *
 * 3) Change an inconsistent toggle button to be consistent and vice versa.
 *
 * Note that currently this code needs to be changed manually to test
 * different actions.
 */

static void _create_event_watcher (void);
static void _check_object (BatkObject *obj);
static void button_pressed_handler (BtkButton *button);
static void _print_states (BatkObject *obj);
static void _print_button_image_info(BatkObject *obj);
static bint _do_button_action (bpointer data);
static bint _toggle_inconsistent (bpointer data);
static bint _finish_button_action (bpointer data);

#define NUM_VALID_ROLES 4

static void 
_check_object (BatkObject *obj)
{
  BatkRole role;
  static bboolean first_time = TRUE;

  role = batk_object_get_role (obj);
  if (role == BATK_ROLE_FRAME)
  /*
   * Find the specified button in the window
   */
  {
    BatkRole valid_roles[NUM_VALID_ROLES];
    const char *name;
    BatkObject *batk_button;
    BtkWidget *widget;

    valid_roles[0] = BATK_ROLE_PUSH_BUTTON;
    valid_roles[1] = BATK_ROLE_TOGGLE_BUTTON;
    valid_roles[2] = BATK_ROLE_CHECK_BOX;
    valid_roles[3] = BATK_ROLE_RADIO_BUTTON;

    name = g_getenv ("TEST_ACCESSIBLE_NAME");
    if (name == NULL)
      name = "button box";
    batk_button = find_object_by_accessible_name_and_role (obj, name,
                     valid_roles, NUM_VALID_ROLES);

    if (batk_button == NULL)
    {
      g_print ("Object not found for %s\n", name);
      return;
    }
    g_assert (BTK_IS_ACCESSIBLE (batk_button));
    widget = BTK_ACCESSIBLE (batk_button)->widget;
    g_assert (BTK_IS_BUTTON (widget));
    g_signal_connect (BTK_OBJECT (widget),
                      "pressed",
                      G_CALLBACK (button_pressed_handler),
                      NULL);
    if (BTK_IS_TOGGLE_BUTTON (widget))
    {
      _toggle_inconsistent (BTK_TOGGLE_BUTTON (widget));
    }
    if (first_time)
      first_time = FALSE;
    else
      return;

    if (g_getenv ("TEST_ACCESSIBLE_AUTO"))
      {
        g_idle_add (_do_button_action, batk_button);
      }
  }
}

static bint _toggle_inconsistent (bpointer data)
{
  BtkToggleButton *toggle_button = BTK_TOGGLE_BUTTON (data);

  if (btk_toggle_button_get_inconsistent (toggle_button))
  {
    btk_toggle_button_set_inconsistent (toggle_button, FALSE);
  }
  else
  {
    btk_toggle_button_set_inconsistent (toggle_button, TRUE);
  }
  return FALSE;
} 

static bint _do_button_action (bpointer data)
{
  BatkObject *obj = BATK_OBJECT (data);

  batk_action_do_action (BATK_ACTION (obj), 2);

  g_timeout_add (5000, _finish_button_action, obj);
  return FALSE;
}

static bint _finish_button_action (bpointer data)
{
#if 0
  BatkObject *obj = BATK_OBJECT (data);

  batk_action_do_action (BATK_ACTION (obj), 0);
#endif

  return FALSE;
}

static void
button_pressed_handler (BtkButton *button)
{
  BatkObject *obj;

  obj = btk_widget_get_accessible (BTK_WIDGET (button));
  _print_states (obj);
  _print_button_image_info (obj);

  if (BTK_IS_TOGGLE_BUTTON (button))
    {
      g_idle_add (_toggle_inconsistent, BTK_TOGGLE_BUTTON (button));
    }
}

static void 
_print_states (BatkObject *obj)
{
  BatkStateSet *state_set;
  bint i;

  state_set = batk_object_ref_state_set (obj);

  g_print ("*** Start states ***\n");
  for (i = 0; i < 64; i++)
  {
     BatkStateType one_state;
     const bchar *name;

     if (batk_state_set_contains_state (state_set, i))
     {
       one_state = i;

       name = batk_state_type_get_name (one_state);

       if (name)
         g_print("%s\n", name);
     }
  }
  g_object_unref (state_set);
  g_print ("*** End states ***\n");
}

static void 
_print_button_image_info(BatkObject *obj) {

  bint height, width;
  const bchar *desc;

  height = width = 0;

  if(!BATK_IS_IMAGE(obj)) 
	return;

  g_print("*** Start Button Image Info ***\n");
  desc = batk_image_get_image_description(BATK_IMAGE(obj));
  g_print ("batk_image_get_image_desc returns : %s\n", desc ? desc : "<NULL>");
  batk_image_get_image_size(BATK_IMAGE(obj), &height ,&width);
  g_print("batk_image_get_image_size returns: height %d width %d\n",height,width);
  if(batk_image_set_image_description(BATK_IMAGE(obj), "New image Description")){
	desc = batk_image_get_image_description(BATK_IMAGE(obj));
	g_print ("batk_image_get_image_desc now returns : %s\n",desc ?desc:"<NULL>");
  }
  g_print("*** End Button Image Info ***\n");


}

static void
_create_event_watcher (void)
{
  batk_add_focus_tracker (_check_object);
}

int
btk_module_init(bint argc, char* argv[])
{
  g_print("testbutton Module loaded\n");

  _create_event_watcher();

  return 0;
}
