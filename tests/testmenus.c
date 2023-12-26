/* testmenus.c -- dynamically add and remove items to a menu 
 * Copyright (C) 2002  Red Hat, Inc.
 * Author: Owen Taylor
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
#include <bdk/bdkkeysyms.h>

#include <stdio.h>

static BtkWidget *
create_menu (gint     depth,
	     gboolean tearoff)
{
  BtkWidget *menu;
  BtkWidget *menuitem;
  GSList *group;
  char buf[32];
  int i, j;

  if (depth < 1)
    return NULL;

  menu = btk_menu_new ();
  group = NULL;

  if (tearoff)
    {
      menuitem = btk_tearoff_menu_item_new ();
      btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
      btk_widget_show (menuitem);
    }

  for (i = 0, j = 1; i < depth / 4 * 100 + 5; i++, j++)
    {
      sprintf (buf, "item %2d - %d", depth, j);
      menuitem = btk_radio_menu_item_new_with_label (group, buf);
      group = btk_radio_menu_item_get_group (BTK_RADIO_MENU_ITEM (menuitem));

      btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
      btk_widget_show (menuitem);
      if (i == 3)
	btk_widget_set_sensitive (menuitem, FALSE);

      btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), create_menu (depth - 1, TRUE));
    }

  return menu;
}

static gboolean
change_item (gpointer user_data)
{
  BtkWidget *widget;
  BtkMenuShell *shell = BTK_MENU_SHELL (user_data);
  static gint step = 0;

  if (((step++ / 40) % 2) == 0)
    {
      g_message ("Idle add");

      widget = btk_menu_item_new_with_label ("Foo");
      btk_widget_show (widget);
  
      btk_menu_shell_append (shell, widget);
    }
  else
    {
      GList *children = btk_container_get_children (BTK_CONTAINER (shell));

      g_message ("Idle remove");
      
      btk_widget_destroy (g_list_last (children)->data);

      g_list_free (children);
    }
  
  return TRUE;
}
 
int 
main (int argc, char **argv)
{
  static BtkWidget *window = NULL;
  BtkWidget *box1;
  BtkWidget *box2;
  BtkWidget *button;
  BtkWidget *optionmenu;
  BtkWidget *separator;

  btk_init (&argc, &argv);
  
  if (!window)
    {
      BtkWidget *menubar;
      BtkWidget *menu;
      BtkWidget *submenu;
      BtkWidget *menuitem;
      BtkAccelGroup *accel_group;
      
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      
      g_signal_connect (window, "destroy",
			G_CALLBACK(btk_main_quit), NULL);
      g_signal_connect (window, "delete-event",
			G_CALLBACK (btk_true), NULL);
      
      accel_group = btk_accel_group_new ();
      btk_window_add_accel_group (BTK_WINDOW (window), accel_group);

      btk_window_set_title (BTK_WINDOW (window), "menus");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);
      
      
      box1 = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), box1);
      btk_widget_show (box1);
      
      menubar = btk_menu_bar_new ();
      btk_box_pack_start (BTK_BOX (box1), menubar, FALSE, TRUE, 0);
      btk_widget_show (menubar);
      
      menu = create_menu (2, TRUE);
      
      menuitem = btk_menu_item_new_with_label ("test\nline2");
      btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), menu);
      btk_menu_shell_append (BTK_MENU_SHELL (menubar), menuitem);
      btk_widget_show (menuitem);

      
      menuitem = btk_menu_item_new_with_label ("dynamic");
      submenu = create_menu (3, TRUE);
      btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), submenu);
      btk_menu_shell_append (BTK_MENU_SHELL (menubar), menuitem);
      btk_widget_show (menuitem);
      
      bdk_threads_add_timeout (250, change_item, submenu);

      menuitem = btk_menu_item_new_with_label ("bar");
      btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), create_menu (4, TRUE));
      btk_menu_item_set_right_justified (BTK_MENU_ITEM (menuitem), TRUE);
      btk_menu_shell_append (BTK_MENU_SHELL (menubar), menuitem);
      btk_widget_show (menuitem);
      
      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, TRUE, TRUE, 0);
      btk_widget_show (box2);
      
      menu = create_menu (1, FALSE);
      btk_menu_set_accel_group (BTK_MENU (menu), accel_group);
      
      menuitem = btk_separator_menu_item_new ();
      btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
      btk_widget_show (menuitem);
      
      menuitem = btk_check_menu_item_new_with_label ("Accelerate Me");
      btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
      btk_widget_show (menuitem);
      btk_widget_add_accelerator (menuitem,
				  "activate",
				  accel_group,
				  BDK_F1,
				  0,
				  BTK_ACCEL_VISIBLE);
      menuitem = btk_check_menu_item_new_with_label ("Accelerator Locked");
      btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
      btk_widget_show (menuitem);
      btk_widget_add_accelerator (menuitem,
				  "activate",
				  accel_group,
				  BDK_F2,
				  0,
				  BTK_ACCEL_VISIBLE | BTK_ACCEL_LOCKED);
      menuitem = btk_check_menu_item_new_with_label ("Accelerators Frozen");
      btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
      btk_widget_show (menuitem);
      btk_widget_add_accelerator (menuitem,
				  "activate",
				  accel_group,
				  BDK_F2,
				  0,
				  BTK_ACCEL_VISIBLE);
      btk_widget_add_accelerator (menuitem,
				  "activate",
				  accel_group,
				  BDK_F3,
				  0,
				  BTK_ACCEL_VISIBLE);
      
      optionmenu = btk_option_menu_new ();
      btk_option_menu_set_menu (BTK_OPTION_MENU (optionmenu), menu);
      btk_option_menu_set_history (BTK_OPTION_MENU (optionmenu), 3);
      btk_box_pack_start (BTK_BOX (box2), optionmenu, TRUE, TRUE, 0);
      btk_widget_show (optionmenu);

      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);
      btk_widget_show (separator);

      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);
      btk_widget_show (box2);

      button = btk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
				G_CALLBACK(btk_widget_destroy), window);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
      btk_widget_set_can_default (button, TRUE);
      btk_widget_grab_default (button);
      btk_widget_show (button);
    }

  if (!btk_widget_get_visible (window))
    {
      btk_widget_show (window);
    }
  else
    {
      btk_widget_destroy (window);
      window = NULL;
    }

  btk_main ();

  return 0;
}

