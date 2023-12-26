#include <batk/batk.h>

static void _check_position (BatkObject *obj);

static void _check_position (BatkObject *obj)
{
  BatkObject *parent, *ret_object;

  gint x, y, width, height;
  gint x1, y1, width1, height1;

  x = y = width = height = 0;
  x1 = y1 = width1 = height1 = 0;

  if (!BATK_IS_COMPONENT (obj))
    return;

  batk_component_get_extents (BATK_COMPONENT(obj), &x, &y, &width, &height, BATK_XY_SCREEN);
  batk_component_get_position (BATK_COMPONENT(obj), &x1, &y1, BATK_XY_SCREEN );
  batk_component_get_size (BATK_COMPONENT(obj), &width1, &height1);
  if ((x1 !=  x) || (y1 != y))
  {
    g_print ("batk_component_get_extents and batk_get_position give different"
             "  values: %d,%d %d,%d\n", x, y, x1, y1);
  }
  if ((width1 !=  width) || (height1 != height))
  {
    g_print ("batk_component_get_extents and batk_get_size give different"
             "  values: %d,%d %d,%d\n", width, height, width1, height1);
  }

  batk_component_get_position (BATK_COMPONENT(obj), &x1, &y1, BATK_XY_SCREEN);
  g_print ("Object Type: %s\n", g_type_name (G_OBJECT_TYPE (obj)));
  g_print ("Object at %d, %d on screen\n", x1, y1);
  g_print ("Object at %d, %d, size: %d, %d\n", x, y, width, height);

  parent = batk_object_get_parent (obj);

  if (BATK_IS_COMPONENT (parent))
  {
    gint px, py, pwidth, pheight;
    
    batk_component_get_extents (BATK_COMPONENT(parent),
                               &px, &py, &pwidth, &pheight, BATK_XY_SCREEN);
    g_print ("Parent Type: %s\n", g_type_name (G_OBJECT_TYPE (parent)));
    g_print ("Parent at %d, %d, size: %d, %d\n", px, py, pwidth, pheight);
    ret_object = batk_component_ref_accessible_at_point (BATK_COMPONENT (parent), 
                                                        x, y, BATK_XY_SCREEN);

    if (!ret_object)
    {
      g_print ("1:batk_component_ref_accessible_at_point returns NULL\n");
    }
    else if (ret_object != obj)
    {
      g_print ("1:batk_component_ref_accessible_at_point returns wrong value for %d %d\n",
                x, y);
      batk_component_get_extents (BATK_COMPONENT(ret_object),
                                 &px, &py, &pwidth, &pheight, BATK_XY_SCREEN);
      g_print ("ret_object at %d, %d, size: %d, %d\n", px, py, pwidth, pheight);
    }
    if (ret_object)
      g_object_unref (G_OBJECT (ret_object));
    ret_object = batk_component_ref_accessible_at_point (BATK_COMPONENT (parent), 
                                                        x+width-1, y+height-1, BATK_XY_SCREEN);
    if (!ret_object)
    {
      g_print ("2:batk_component_ref_accessible_at_point returns NULL\n");
    }
    else if (ret_object != obj)
    {
      g_print ("2:batk_component_ref_accessible_at_point returns wrong value for %d %d\n",
                x+width-1, y+height-1);
    } 
    if (ret_object)
      g_object_unref (G_OBJECT (ret_object));
    ret_object = batk_component_ref_accessible_at_point (BATK_COMPONENT (parent), 
                                                        x-1, y-1, BATK_XY_SCREEN);
    if ((ret_object) && (ret_object == obj))
    {
      g_print ("3:batk_component_ref_accessible_at_point returns wrong value for %d %d\n",
                x-1, y-1);
    } 
    if (ret_object)
      g_object_unref (G_OBJECT (ret_object));
    ret_object = batk_component_ref_accessible_at_point (BATK_COMPONENT (parent), 
                                                        x+width, y+height, BATK_XY_SCREEN);
    if ((ret_object) && (ret_object == obj))
    {
      g_print ("4:batk_component_ref_accessible_at_point returns wrong value for %d %d\n",
                x+width, y+height);
    }
    if (ret_object)
      g_object_unref (G_OBJECT (ret_object));
  }
  if (!batk_component_contains (BATK_COMPONENT(obj), x, y, BATK_XY_SCREEN))
    g_print ("Component does not contain position, %d %d\n", x, y);
  if (batk_component_contains (BATK_COMPONENT(obj), x-1, y-1, BATK_XY_SCREEN))
    g_print ("Component does contain position, %d %d\n", x-1, y-1);
  if (!batk_component_contains (BATK_COMPONENT(obj), x+width-1, y+height-1, BATK_XY_SCREEN))
    g_print ("Component does not contain position, %d %d\n", 
             x+width-1, y+height-1);
  if (batk_component_contains (BATK_COMPONENT(obj), x+width, y+height, BATK_XY_SCREEN))
    g_print ("Component does contain position, %d %d\n", x+width, y+height);
}

static void
_create_event_watcher (void)
{
  batk_add_focus_tracker (_check_position);
}

int
btk_module_init(gint argc, char* argv[])
{
  g_print("testcomponent Module loaded\n");

  _create_event_watcher();

  return 0;
}
