/* stresstest-toolbar.c
 *
 * Copyright (C) 2003 Soeren Sandmann <sandmann@daimi.au.dk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#undef BTK_DISABLE_DEPRECATED
#include "config.h"
#include <btk/btk.h>

typedef struct _Info Info;
struct _Info
{
  BtkWindow  *window;
  BtkToolbar *toolbar;
  bint	      counter;
};

static void
add_random (BtkToolbar *toolbar, bint n)
{
  bint position;
  bchar *label = g_strdup_printf ("Button %d", n);

  BtkWidget *widget = btk_button_new_with_label (label);

  g_free (label);
  btk_widget_show_all (widget);

  if (g_list_length (toolbar->children) == 0)
    position = 0;
  else
    position = g_random_int_range (0, g_list_length (toolbar->children));

  btk_toolbar_insert_widget (toolbar, widget, "Bar", "Baz", position);
}

static void
remove_random (BtkToolbar *toolbar)
{
  BtkToolbarChild *child;
  bint position;

  if (!toolbar->children)
    return;

  position = g_random_int_range (0, g_list_length (toolbar->children));

  child = g_list_nth_data (toolbar->children, position);
  
  btk_container_remove (BTK_CONTAINER (toolbar), child->widget);
}

static bboolean
stress_test_old_api (bpointer data)
{
  typedef enum {
    ADD_RANDOM,
    REMOVE_RANDOM,
    LAST_ACTION
  } Action;
      
  Info *info = data;
  Action action;
  
  if (info->counter++ == 200)
    {
      btk_main_quit ();
      return FALSE;
    }

  if (!info->toolbar)
    {
      info->toolbar = BTK_TOOLBAR (btk_toolbar_new ());
      btk_container_add (BTK_CONTAINER (info->window),
			 BTK_WIDGET (info->toolbar));
      btk_widget_show (BTK_WIDGET (info->toolbar));
    }

  if (!info->toolbar->children)
    {
      add_random (info->toolbar, info->counter);
      return TRUE;
    }
  else if (g_list_length (info->toolbar->children) > 50)
    {
      int i;
      for (i = 0; i < 25; i++)
	remove_random (info->toolbar);
      return TRUE;
    }
  
  action = g_random_int_range (0, LAST_ACTION);

  switch (action)
    {
    case ADD_RANDOM:
      add_random (info->toolbar, info->counter);
      break;

    case REMOVE_RANDOM:
      remove_random (info->toolbar);
      break;
      
    default:
      g_assert_not_reached();
      break;
    }
  
  return TRUE;
}


bint
main (bint argc, bchar **argv)
{
  Info info;
  
  btk_init (&argc, &argv);

  info.toolbar = NULL;
  info.counter = 0;
  info.window = BTK_WINDOW (btk_window_new (BTK_WINDOW_TOPLEVEL));

  btk_widget_show (BTK_WIDGET (info.window));
  
  bdk_threads_add_idle (stress_test_old_api, &info);

  btk_widget_show_all (BTK_WIDGET (info.window));
  
  btk_main ();

  btk_widget_destroy (BTK_WIDGET (info.window));

  info.toolbar = NULL;
  info.window = NULL;
  
  return 0;
}
