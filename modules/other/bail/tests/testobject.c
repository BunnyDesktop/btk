#include <btk/btk.h>
#include "testlib.h"

static void _print_accessible (BatkObject *obj);
static void _print_type (BatkObject *obj);
static void _print_states (BatkObject *obj);
static void _check_children (BatkObject *obj);
static void _check_relation (BatkObject *obj);
static void _create_event_watcher (void);
static void _focus_handler (BatkObject *obj, gboolean focus_in);
static gboolean _children_changed (GSignalInvocationHint *ihint,
                                   guint                  n_param_values,
                                   const BValue          *param_values,
                                   gpointer               data);

static guint id;

static void _print_states (BatkObject *obj)
{
  BatkStateSet *state_set;
  gint i;

  state_set = batk_object_ref_state_set (obj);

  g_print ("*** Start states ***\n");
  for (i = 0; i < 64; i++)
    {
       BatkStateType one_state;
       const gchar *name;

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

static void _print_type (BatkObject *obj)
{
  const gchar * typename = NULL;
  const gchar * name = NULL;
  BatkRole role;
  static gboolean in_print_type = FALSE;
   
  if (BTK_IS_ACCESSIBLE (obj))
    {
      BtkWidget* widget = NULL;

      widget = BTK_ACCESSIBLE (obj)->widget;
      typename = g_type_name (B_OBJECT_TYPE (widget));
      g_print ("Widget type name: %s\n", typename ? typename : "NULL");
    }
  typename = g_type_name (B_OBJECT_TYPE (obj));
  g_print ("Accessible type name: %s\n", typename ? typename : "NULL");
  name = batk_object_get_name (obj);
  g_print("Accessible Name: %s\n", (name) ? name : "NULL");
  role = batk_object_get_role (obj);
  g_print ("Accessible Role: %s\n", batk_role_get_name (role));

  if (BATK_IS_COMPONENT (obj))
    {
      gint x, y, width, height;
      BatkStateSet *states;

      _print_states (obj);
      states = batk_object_ref_state_set (obj);
      if (batk_state_set_contains_state (states, BATK_STATE_VISIBLE))
        {
          BatkObject *parent;

          batk_component_get_extents (BATK_COMPONENT (obj), 
                                     &x, &y, &width, &height, 
                                     BATK_XY_SCREEN);
          g_print ("BATK_XY_SCREEN: x: %d y: %d width: %d height: %d\n",
                   x, y, width, height);

          batk_component_get_extents (BATK_COMPONENT (obj), 
                                     &x, &y, &width, &height, 
                                     BATK_XY_WINDOW);
          g_print ("BATK_XY_WINDOW: x: %d y: %d width: %d height: %d\n", 
                   x, y, width, height);
          if (batk_state_set_contains_state (states, BATK_STATE_SHOWING) &&
              BATK_IS_TEXT (obj))
            {
              gint offset;

              batk_text_get_character_extents (BATK_TEXT (obj), 1, 
                                              &x, &y, &width, &height, 
                                              BATK_XY_WINDOW);
              g_print ("Character extents : %d %d %d %d\n", 
                       x, y, width, height);
              if (width != 0 && height != 0)
                {
                  offset = batk_text_get_offset_at_point (BATK_TEXT (obj), 
                                                         x, y, 
                                                         BATK_XY_WINDOW);
                  if (offset != 1)
                    {
                      g_print ("Wrong offset returned (%d) %d\n", 1, offset);
                    }
                }
            }
          if (in_print_type)
            return;

          parent = batk_object_get_parent (obj);
          if (!BATK_IS_COMPONENT (parent))
            {
              /* Assume toplevel */
              g_object_unref (B_OBJECT (states));
              return;
            }
#if 0
          obj1 = batk_component_ref_accessible_at_point (BATK_COMPONENT (parent),
                                                        x, y, BATK_XY_WINDOW);
          if (obj != obj1)
            {
              g_print ("Inconsistency between object and ref_accessible_at_point\n");
              in_print_type = TRUE;
              _print_type (obj1);
              in_print_type = FALSE;
            }
#endif
        }
      g_object_unref (B_OBJECT (states));
    }
}

static void _print_accessible (BatkObject *obj)
{
  BtkWidget* widget = NULL;
  BatkObject* parent_batk;
  BatkObject* ref_obj;
  BatkRole    role;
  static gboolean first_time = TRUE;

  if (first_time)
    {
      first_time = FALSE;
      batk_add_global_event_listener (_children_changed, 
                                     "Batk:BatkObject:children_changed");
    }

  /*
   * Check that the object returned by the batk_implementor_ref_accessible()
   * for a widget is the same as the accessible object
   */
  if (BTK_IS_ACCESSIBLE (obj))
    {
      widget = BTK_ACCESSIBLE (obj)->widget;
      ref_obj = batk_implementor_ref_accessible (BATK_IMPLEMENTOR (widget));
      g_assert (ref_obj == obj);
      g_object_unref (B_OBJECT (ref_obj));
    }
  /*
   * Add a focus handler so we see focus out events as well
   */
  if (BATK_IS_COMPONENT (obj))
    batk_component_add_focus_handler (BATK_COMPONENT (obj), _focus_handler);
  g_print ("Object:\n");
  _print_type (obj);
  _print_states (obj);

  /*
   * Get the parent object
   */
  parent_batk = batk_object_get_parent (obj);
  if (parent_batk)
    {
      g_print ("Parent Object:\n");
      _print_type (parent_batk);
      parent_batk = batk_object_get_parent (parent_batk);
      if (parent_batk)
        {
          g_print ("Grandparent Object:\n");
          _print_type (parent_batk);
        }
    } 
  else 
    {
      g_print ("No parent\n");
    }

  role = batk_object_get_role (obj);

  if ((role == BATK_ROLE_FRAME) || (role == BATK_ROLE_DIALOG))
    {
      _check_children (obj);
    }
}

static void _check_relation (BatkObject *obj)
{
  BatkRelationSet* relation_set = batk_object_ref_relation_set (obj);
  gint n_relations, i;

  g_return_if_fail (relation_set);

  n_relations = batk_relation_set_get_n_relations (relation_set);
  for (i = 0; i < n_relations; i++)
    {
      BatkRelation* relation = batk_relation_set_get_relation (relation_set, i);

      g_print ("Index: %d Relation type: %d Number: %d\n", i,
                batk_relation_get_relation_type (relation),
                batk_relation_get_target (relation)->len);
    }
  g_object_unref (relation_set);
}

static void _check_children (BatkObject *obj)
{
  gint n_children, i;
  BatkLayer layer;
  BatkRole role;

  g_print ("Start Check Children\n");
  n_children = batk_object_get_n_accessible_children (obj);
  g_print ("Number of children: %d\n", n_children);

  role = batk_object_get_role (obj);

  if (BATK_IS_COMPONENT (obj))
    {
      batk_component_add_focus_handler (BATK_COMPONENT (obj), _focus_handler);
      layer = batk_component_get_layer (BATK_COMPONENT (obj));
      if (role == BATK_ROLE_MENU)
	      g_assert (layer == BATK_LAYER_POPUP);
      else
	      g_print ("Layer: %d\n", layer);
    }

  for (i = 0; i < n_children; i++)
    {
      BatkObject *child;
      BatkObject *parent;
      int j;

      child = batk_object_ref_accessible_child (obj, i);
      parent = batk_object_get_parent (child);
      j = batk_object_get_index_in_parent (child);
      _print_type (child);
      _check_relation (child);
      _check_children (child);
      if (obj != parent)
        {
          g_print ("*** Inconsistency between batk_object_get_parent() and "
                   "batk_object_ref_accessible_child() ***\n");
          _print_type (child);
          _print_type (obj);
          if (parent)
            _print_type (parent);
        }
      g_object_unref (B_OBJECT (child));
                 
      if (j != i)
        g_print ("*** Inconsistency between parent and children %d %d ***\n",
                 i, j);
    }
  g_print ("End Check Children\n");
}

static gboolean
_children_changed (GSignalInvocationHint *ihint,
                   guint                  n_param_values,
                   const BValue          *param_values,
                   gpointer               data)
{
  BObject *object;
  guint index;
  gpointer target;
  const gchar *target_name = "NotBatkObject";

  object = b_value_get_object (param_values + 0);
  index = b_value_get_uint (param_values + 1);
  target = b_value_get_pointer (param_values + 2);
  if (G_IS_OBJECT (target))
    {
      if (BATK_IS_OBJECT (target))
        {
          target_name = batk_object_get_name (target);
        }
      if (!target_name) 
        target_name = g_type_name (B_OBJECT_TYPE (B_OBJECT (target)));
    }
  else
    {
      if (!target)
        {
          BatkObject *child;

          child = batk_object_ref_accessible_child (BATK_OBJECT (object), index);
          if (child)
            {
              target_name = g_type_name (B_OBJECT_TYPE (B_OBJECT (child)));
              g_object_unref (child);
            }
        }
    }
  g_print ("_children_watched: %s %s %s index: %d\n", 
           g_type_name (B_OBJECT_TYPE (object)),
           g_quark_to_string (ihint->detail),
           target_name, index);
  return TRUE;
}

static void
_create_event_watcher (void)
{
  /*
   * _print_accessible() will be called for an accessible object when its
   * widget receives focus.
   */
  id = batk_add_focus_tracker (_print_accessible);
}

static void 
_focus_handler (BatkObject *obj, gboolean focus_in)
{
  g_print ("In _focus_handler focus_in: %s\n", focus_in ? "true" : "false"); 
  _print_type (obj);
}

int
btk_module_init(gint argc, char* argv[])
{
  g_print("testobject Module loaded\n");

  _create_event_watcher();

  return 0;
}
