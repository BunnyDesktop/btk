/* Menus
 *
 * There are several widgets involved in displaying menus. The
 * BtkMenuBar widget is a menu bar, which normally appears horizontally
 * at the top of an application, but can also be layed out vertically.
 * The BtkMenu widget is the actual menu that pops up. Both BtkMenuBar
 * and BtkMenu are subclasses of BtkMenuShell; a BtkMenuShell contains
 * menu items (BtkMenuItem). Each menu item contains text and/or images
 * and can be selected by the user.
 *
 * There are several kinds of menu item, including plain BtkMenuItem,
 * BtkCheckMenuItem which can be checked/unchecked, BtkRadioMenuItem
 * which is a check menu item that's in a mutually exclusive group,
 * BtkSeparatorMenuItem which is a separator bar, BtkTearoffMenuItem
 * which allows a BtkMenu to be torn off, and BtkImageMenuItem which
 * can place a BtkImage or other widget next to the menu text.
 *
 * A BtkMenuItem can have a submenu, which is simply a BtkMenu to pop
 * up when the menu item is selected. Typically, all menu items in a menu bar
 * have submenus.
 *
 * BtkUIManager provides a higher-level interface for creating menu bars
 * and menus; while you can construct menus manually, most people don't
 * do that. There's a separate demo for BtkUIManager.
 */

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

  for (i = 0, j = 1; i < 5; i++, j++)
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

static void
change_orientation (BtkWidget *button,
                    BtkWidget *menubar)
{
  BtkWidget *parent;
  BtkWidget *box = NULL;

  parent = btk_widget_get_parent (menubar);

  if (BTK_IS_VBOX (parent))
    {
      box = btk_widget_get_parent (parent);

      g_object_ref (menubar);
      btk_container_remove (BTK_CONTAINER (parent), menubar);
      btk_container_add (BTK_CONTAINER (box), menubar);
      btk_box_reorder_child (BTK_BOX (box), menubar, 0);
      g_object_unref (menubar);
      g_object_set (menubar, 
		    "pack-direction", BTK_PACK_DIRECTION_TTB,
		    NULL);
    }
  else
    {
      GList *children, *l;

      children = btk_container_get_children (BTK_CONTAINER (parent));
      for (l = children; l; l = l->next)
	{
	  if (BTK_IS_VBOX (l->data))
	    {
	      box = l->data;
	      break;
	    }
	}
      g_list_free (children);

      g_object_ref (menubar);
      btk_container_remove (BTK_CONTAINER (parent), menubar);
      btk_container_add (BTK_CONTAINER (box), menubar);
      btk_box_reorder_child (BTK_BOX (box), menubar, 0);
      g_object_unref (menubar);
      g_object_set (menubar, 
		    "pack-direction", BTK_PACK_DIRECTION_LTR,
		    NULL);
    }
}

static BtkWidget *window = NULL;

BtkWidget *
do_menus (BtkWidget *do_widget)
{
  BtkWidget *box;
  BtkWidget *box1;
  BtkWidget *box2;
  BtkWidget *button;

  if (!window)
    {
      BtkWidget *menubar;
      BtkWidget *menu;
      BtkWidget *menuitem;
      BtkAccelGroup *accel_group;

      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (do_widget));
      btk_window_set_title (BTK_WINDOW (window), "Menus");
      g_signal_connect (window, "destroy",
			G_CALLBACK(btk_widget_destroyed), &window);

      accel_group = btk_accel_group_new ();
      btk_window_add_accel_group (BTK_WINDOW (window), accel_group);

      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      box = btk_hbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), box);
      btk_widget_show (box);

      box1 = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (box), box1);
      btk_widget_show (box1);

      menubar = btk_menu_bar_new ();
      btk_box_pack_start (BTK_BOX (box1), menubar, FALSE, TRUE, 0);
      btk_widget_show (menubar);

      menu = create_menu (2, TRUE);

      menuitem = btk_menu_item_new_with_label ("test\nline2");
      btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), menu);
      btk_menu_shell_append (BTK_MENU_SHELL (menubar), menuitem);
      btk_widget_show (menuitem);

      menuitem = btk_menu_item_new_with_label ("foo");
      btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), create_menu (3, TRUE));
      btk_menu_shell_append (BTK_MENU_SHELL (menubar), menuitem);
      btk_widget_show (menuitem);

      menuitem = btk_menu_item_new_with_label ("bar");
      btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), create_menu (4, TRUE));
      btk_menu_item_set_right_justified (BTK_MENU_ITEM (menuitem), TRUE);
      btk_menu_shell_append (BTK_MENU_SHELL (menubar), menuitem);
      btk_widget_show (menuitem);

      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);
      btk_widget_show (box2);

      button = btk_button_new_with_label ("Flip");
      g_signal_connect (button, "clicked",
			G_CALLBACK (change_orientation), menubar);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
      btk_widget_show (button);

      button = btk_button_new_with_label ("Close");
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

  return window;
}
