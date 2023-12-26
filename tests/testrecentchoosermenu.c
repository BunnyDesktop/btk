/* testrecentchoosermenu.c - Test BtkRecentChooserMenu
 * Copyright (C) 2007  Emmanuele Bassi  <ebassi@bunny.org>
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

#include "config.h"
#include <btk/btk.h>

static BtkRecentManager *manager = NULL;
static BtkWidget *window = NULL;
static BtkWidget *label = NULL;

static void
item_activated_cb (BtkRecentChooser *chooser,
                   gpointer          data)
{
  BtkRecentInfo *info;
  GString *text;
  gchar *label_text;

  info = btk_recent_chooser_get_current_item (chooser);
  if (!info)
    {
      g_warning ("Unable to retrieve the current item, aborting...");
      return;
    }

  text = g_string_new ("Selected recent item:\n");
  g_string_append_printf (text, "  URI: %s\n",
                          btk_recent_info_get_uri (info));
  g_string_append_printf (text, "  MIME Type: %s\n",
                          btk_recent_info_get_mime_type (info));

  label_text = g_string_free (text, FALSE);
  btk_label_set_text (BTK_LABEL (label), label_text);
  
  btk_recent_info_unref (info);
  g_free (label_text);
}

static BtkWidget *
create_recent_chooser_menu (gint limit)
{
  BtkWidget *menu;
  BtkRecentFilter *filter;
  BtkWidget *menuitem;
  
  menu = btk_recent_chooser_menu_new_for_manager (manager);

  if (limit > 0)
    btk_recent_chooser_set_limit (BTK_RECENT_CHOOSER (menu), limit);
  btk_recent_chooser_set_local_only (BTK_RECENT_CHOOSER (menu), TRUE);
  btk_recent_chooser_set_show_icons (BTK_RECENT_CHOOSER (menu), TRUE);
  btk_recent_chooser_set_show_tips (BTK_RECENT_CHOOSER (menu), TRUE);
  btk_recent_chooser_set_sort_type (BTK_RECENT_CHOOSER (menu),
                                    BTK_RECENT_SORT_MRU);
  btk_recent_chooser_menu_set_show_numbers (BTK_RECENT_CHOOSER_MENU (menu),
                                            TRUE);

  filter = btk_recent_filter_new ();
  btk_recent_filter_set_name (filter, "Gedit files");
  btk_recent_filter_add_application (filter, "gedit");
  btk_recent_chooser_add_filter (BTK_RECENT_CHOOSER (menu), filter);
  btk_recent_chooser_set_filter (BTK_RECENT_CHOOSER (menu), filter);

  g_signal_connect (menu, "item-activated",
                    G_CALLBACK (item_activated_cb),
                    NULL);

  menuitem = btk_separator_menu_item_new ();
  btk_menu_shell_prepend (BTK_MENU_SHELL (menu), menuitem);
  btk_widget_show (menuitem);

  menuitem = btk_menu_item_new_with_label ("Test prepend");
  btk_menu_shell_prepend (BTK_MENU_SHELL (menu), menuitem);
  btk_widget_show (menuitem);

  menuitem = btk_separator_menu_item_new ();
  btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
  btk_widget_show (menuitem);

  menuitem = btk_menu_item_new_with_label ("Test append");
  btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
  btk_widget_show (menuitem);

  menuitem = btk_image_menu_item_new_from_stock (BTK_STOCK_CLEAR, NULL);
  btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
  btk_widget_show (menuitem);

  btk_widget_show_all (menu);

  return menu;
}

static BtkWidget *
create_file_menu (BtkAccelGroup *accelgroup)
{
  BtkWidget *menu;
  BtkWidget *menuitem;
  BtkWidget *recentmenu;

  menu = btk_menu_new ();

  menuitem = btk_image_menu_item_new_from_stock (BTK_STOCK_NEW, accelgroup);
  btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
  btk_widget_show (menuitem);

  menuitem = btk_image_menu_item_new_from_stock (BTK_STOCK_OPEN, accelgroup);
  btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
  btk_widget_show (menuitem);

  menuitem = btk_menu_item_new_with_mnemonic ("_Open Recent");
  recentmenu = create_recent_chooser_menu (-1);
  btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), recentmenu);
  btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
  btk_widget_show (menuitem);

  menuitem = btk_separator_menu_item_new ();
  btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
  btk_widget_show (menuitem);

  menuitem = btk_image_menu_item_new_from_stock (BTK_STOCK_QUIT, accelgroup);
  btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
  btk_widget_show (menuitem);

  btk_widget_show (menu);

  return menu;
}

int
main (int argc, char *argv[])
{
  BtkWidget *box;
  BtkWidget *menubar;
  BtkWidget *menuitem;
  BtkWidget *menu;
  BtkWidget *button;
  BtkAccelGroup *accel_group;

  btk_init (&argc, &argv);

  manager = btk_recent_manager_get_default ();

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_default_size (BTK_WINDOW (window), -1, -1);
  btk_window_set_title (BTK_WINDOW (window), "Recent Chooser Menu Test");
  g_signal_connect (window, "destroy", G_CALLBACK (btk_main_quit), NULL);

  accel_group = btk_accel_group_new ();
  btk_window_add_accel_group (BTK_WINDOW (window), accel_group);
  
  box = btk_vbox_new (FALSE, 0);
  btk_container_add (BTK_CONTAINER (window), box);
  btk_widget_show (box);

  menubar = btk_menu_bar_new ();
  btk_box_pack_start (BTK_BOX (box), menubar, FALSE, TRUE, 0);
  btk_widget_show (menubar);

  menu = create_file_menu (accel_group);
  menuitem = btk_menu_item_new_with_mnemonic ("_File");
  btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), menu);
  btk_menu_shell_append (BTK_MENU_SHELL (menubar), menuitem);
  btk_widget_show (menuitem);

  menu = create_recent_chooser_menu (4);
  menuitem = btk_menu_item_new_with_mnemonic ("_Recently Used");
  btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), menu);
  btk_menu_shell_append (BTK_MENU_SHELL (menubar), menuitem);
  btk_widget_show (menuitem);

  label = btk_label_new ("No recent item selected");
  btk_box_pack_start (BTK_BOX (box), label, TRUE, TRUE, 0);
  btk_widget_show (label);

  button = btk_button_new_with_label ("Close");
  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (btk_widget_destroy),
                            window);
  btk_box_pack_end (BTK_BOX (box), button, TRUE, TRUE, 0);
  btk_widget_set_can_default (button, TRUE);
  btk_widget_grab_default (button);
  btk_widget_show (button);

  btk_widget_show (window);

  btk_main ();

  return 0;
}
