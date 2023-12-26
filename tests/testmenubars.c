/* testmenubars.c -- test different packing directions
 * Copyright (C) 2005  Red Hat, Inc.
 * Author: Matthias Clasen
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

#include <btk/btk.h>

static BtkWidget *
create_menu (depth)
{
    BtkWidget *menu;
    BtkWidget *menuitem;

    if (depth < 1)
        return NULL;

    menu = btk_menu_new ();

    menuitem = btk_menu_item_new_with_label ("One");
    btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
    btk_widget_show (menuitem);
    btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem),
			       create_menu (depth - 1));

    menuitem = btk_menu_item_new_with_mnemonic ("Two");
    btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
    btk_widget_show (menuitem);
    btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem),
			       create_menu (depth - 1));

    menuitem = btk_menu_item_new_with_mnemonic ("Three");
    btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
    btk_widget_show (menuitem);
    btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem),
			       create_menu (depth - 1));

    return menu;
}

static BtkWidget*
create_menubar (BtkPackDirection pack_dir,
		BtkPackDirection child_pack_dir,
		bdouble          angle)
{
  BtkWidget *menubar;
  BtkWidget *menuitem;
  BtkWidget *menu;

  menubar = btk_menu_bar_new ();
  btk_menu_bar_set_pack_direction (BTK_MENU_BAR (menubar), 
				   pack_dir);
  btk_menu_bar_set_child_pack_direction (BTK_MENU_BAR (menubar),
					 child_pack_dir);
  
  menuitem = btk_image_menu_item_new_from_stock (BTK_STOCK_HOME, NULL);
  btk_menu_shell_append (BTK_MENU_SHELL (menubar), menuitem);
  btk_label_set_angle (BTK_LABEL (BTK_BIN (menuitem)->child), angle);
  menu = create_menu (2, TRUE);
  btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), menu);

  menuitem = btk_menu_item_new_with_label ("foo");
  btk_menu_shell_append (BTK_MENU_SHELL (menubar), menuitem);
  btk_label_set_angle (BTK_LABEL (BTK_BIN (menuitem)->child), angle);
  menu = create_menu (2, TRUE);
  btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), menu);

  menuitem = btk_menu_item_new_with_label ("bar");
  btk_menu_shell_append (BTK_MENU_SHELL (menubar), menuitem);
  btk_label_set_angle (BTK_LABEL (BTK_BIN (menuitem)->child), angle);
  menu = create_menu (2, TRUE);
  btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), menu);

  return menubar;
}

int 
main (int argc, char **argv)
{
  static BtkWidget *window = NULL;
  BtkWidget *box1;
  BtkWidget *box2;
  BtkWidget *box3;
  BtkWidget *button;
  BtkWidget *separator;

  btk_init (&argc, &argv);
  
  if (!window)
    {
      BtkWidget *menubar1;
      BtkWidget *menubar2;
      BtkWidget *menubar3;
      BtkWidget *menubar4;
      BtkWidget *menubar5;
      BtkWidget *menubar6;
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
      box2 = btk_hbox_new (FALSE, 0);
      box3 = btk_vbox_new (FALSE, 0);
      
      /* Rotation by 0 and 180 degrees is broken in Bango, #166832 */
      menubar1 = create_menubar (BTK_PACK_DIRECTION_LTR, BTK_PACK_DIRECTION_LTR, 0.01);
      menubar2 = create_menubar (BTK_PACK_DIRECTION_BTT, BTK_PACK_DIRECTION_BTT, 90);
      menubar3 = create_menubar (BTK_PACK_DIRECTION_TTB, BTK_PACK_DIRECTION_TTB, 270);
      menubar4 = create_menubar (BTK_PACK_DIRECTION_RTL, BTK_PACK_DIRECTION_RTL, 180.01);
      menubar5 = create_menubar (BTK_PACK_DIRECTION_LTR, BTK_PACK_DIRECTION_BTT, 90);
      menubar6 = create_menubar (BTK_PACK_DIRECTION_BTT, BTK_PACK_DIRECTION_LTR, 0.01);

      btk_container_add (BTK_CONTAINER (window), box1);
      btk_box_pack_start (BTK_BOX (box1), menubar1, FALSE, TRUE, 0);
      btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);
      btk_box_pack_start (BTK_BOX (box2), menubar2, FALSE, TRUE, 0);
      btk_box_pack_start (BTK_BOX (box2), box3, TRUE, TRUE, 0);
      btk_box_pack_start (BTK_BOX (box2), menubar3, FALSE, TRUE, 0);
      btk_box_pack_start (BTK_BOX (box1), menubar4, FALSE, TRUE, 0);
      btk_box_pack_start (BTK_BOX (box3), menubar5, TRUE, TRUE, 0);
      btk_box_pack_start (BTK_BOX (box3), menubar6, TRUE, TRUE, 0);

      btk_widget_show_all (box1);
            
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

