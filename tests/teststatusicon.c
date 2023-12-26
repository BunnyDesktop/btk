/* btkstatusicon-x11.c:
 *
 * Copyright (C) 2003 Sun Microsystems, Inc.
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
 *
 * Authors:
 *	Mark McLoughlin <mark@skynet.ie>
 */

#undef BTK_DISABLE_DEPRECATED

#include <btk/btk.h>
#include <stdlib.h>

#include "prop-editor.h"

typedef enum
{
  TEST_STATUS_INFO,
  TEST_STATUS_QUESTION
} TestStatus;

static TestStatus status = TEST_STATUS_INFO;
static gint timeout = 0;
static GSList *icons = NULL;

static void
size_changed_cb (BtkStatusIcon *icon,
		 int size)
{
  g_print ("status icon %p size-changed size = %d\n", icon, size);
}

static void
embedded_changed_cb (BtkStatusIcon *icon)
{
  g_print ("status icon %p embedded changed to %d\n", icon,
	   btk_status_icon_is_embedded (icon));
}

static void
orientation_changed_cb (BtkStatusIcon *icon)
{
  BtkOrientation orientation;

  g_object_get (icon, "orientation", &orientation, NULL);
  g_print ("status icon %p orientation changed to %d\n", icon, orientation);
}

static void
screen_changed_cb (BtkStatusIcon *icon)
{
  g_print ("status icon %p screen changed to %p\n", icon,
	   btk_status_icon_get_screen (icon));
}

static void
update_icons (void)
{
  GSList *l;
  gchar *icon_name;
  gchar *tooltip;

  if (status == TEST_STATUS_INFO)
    {
      icon_name = "dialog-information";
      tooltip = "Some Information ...";
    }
  else
    {
      icon_name = "dialog-question";
      tooltip = "Some Question ...";
    }

  for (l = icons; l; l = l->next)
    {
      BtkStatusIcon *status_icon = l->data;

      btk_status_icon_set_from_icon_name (status_icon, icon_name);
      btk_status_icon_set_tooltip_text (status_icon, tooltip);
    }
}

static gboolean
timeout_handler (gpointer data)
{
  if (status == TEST_STATUS_INFO)
    status = TEST_STATUS_QUESTION;
  else
    status = TEST_STATUS_INFO;

  update_icons ();

  return TRUE;
}

static void
blink_toggle_toggled (BtkToggleButton *toggle)
{
  GSList *l;

  for (l = icons; l; l = l->next)
    btk_status_icon_set_blinking (BTK_STATUS_ICON (l->data), 
                                  btk_toggle_button_get_active (toggle));
}

static void
visible_toggle_toggled (BtkToggleButton *toggle)
{
  GSList *l;

  for (l = icons; l; l = l->next)
    btk_status_icon_set_visible (BTK_STATUS_ICON (l->data),
                                 btk_toggle_button_get_active (toggle));
}

static void
timeout_toggle_toggled (BtkToggleButton *toggle)
{
  if (timeout)
    {
      g_source_remove (timeout);
      timeout = 0;
    }
  else
    {
      timeout = bdk_threads_add_timeout (2000, timeout_handler, NULL);
    }
}

static void
icon_activated (BtkStatusIcon *icon)
{
  BtkWidget *dialog;
  BtkWidget *toggle;

  dialog = g_object_get_data (B_OBJECT (icon), "test-status-icon-dialog");
  if (dialog == NULL)
    {
      dialog = btk_message_dialog_new (NULL, 0,
				       BTK_MESSAGE_QUESTION,
				       BTK_BUTTONS_CLOSE,
				       "You wanna test the status icon ?");

      btk_window_set_screen (BTK_WINDOW (dialog), btk_status_icon_get_screen (icon));
      btk_window_set_position (BTK_WINDOW (dialog), BTK_WIN_POS_CENTER);

      g_object_set_data_full (B_OBJECT (icon), "test-status-icon-dialog",
			      dialog, (GDestroyNotify) btk_widget_destroy);

      g_signal_connect (dialog, "response", 
			G_CALLBACK (btk_widget_hide), NULL);
      g_signal_connect (dialog, "delete_event", 
			G_CALLBACK (btk_widget_hide_on_delete), NULL);

      toggle = btk_toggle_button_new_with_mnemonic ("_Show the icon");
      btk_box_pack_end (BTK_BOX (BTK_DIALOG (dialog)->vbox), toggle, TRUE, TRUE, 6);
      btk_widget_show (toggle);

      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (toggle),
				    btk_status_icon_get_visible (icon));
      g_signal_connect (toggle, "toggled", 
			G_CALLBACK (visible_toggle_toggled), NULL);

      toggle = btk_toggle_button_new_with_mnemonic ("_Blink the icon");
      btk_box_pack_end (BTK_BOX (BTK_DIALOG (dialog)->vbox), toggle, TRUE, TRUE, 6);
      btk_widget_show (toggle);

      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (toggle),
				    btk_status_icon_get_blinking (icon));
      g_signal_connect (toggle, "toggled", 
			G_CALLBACK (blink_toggle_toggled), NULL);

      toggle = btk_toggle_button_new_with_mnemonic ("_Change images");
      btk_box_pack_end (BTK_BOX (BTK_DIALOG (dialog)->vbox), toggle, TRUE, TRUE, 6);
      btk_widget_show (toggle);

      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (toggle),
				    timeout != 0);
      g_signal_connect (toggle, "toggled", 
			G_CALLBACK (timeout_toggle_toggled), NULL);
    }

  btk_window_present (BTK_WINDOW (dialog));
}

static void
check_activated (BtkCheckMenuItem *item)
{
  GSList *l;
  BdkScreen *screen;

  screen = NULL;

  for (l = icons; l; l = l->next)
    {
      BtkStatusIcon *icon = l->data;
      BdkScreen *orig_screen;

      orig_screen = btk_status_icon_get_screen (icon);

      if (screen != NULL)
        btk_status_icon_set_screen (icon, screen);

      screen = orig_screen;

      btk_status_icon_set_blinking (icon,
                                    btk_check_menu_item_get_active (item));
    }

  g_assert (screen != NULL);

  btk_status_icon_set_screen (BTK_STATUS_ICON (icons->data), screen);
}

static void
do_properties (BtkMenuItem   *item,
	       BtkStatusIcon *icon)
{
	static BtkWidget *editor = NULL;

	if (editor == NULL) {
		editor = create_prop_editor (B_OBJECT (icon), BTK_TYPE_STATUS_ICON);
		g_signal_connect (editor, "destroy", G_CALLBACK (btk_widget_destroyed), &editor);
	}

	btk_window_present (BTK_WINDOW (editor));
}

static void
do_quit (BtkMenuItem *item)
{
  GSList *l;

  for (l = icons; l; l = l->next)
    {
      BtkStatusIcon *icon = l->data;

      btk_status_icon_set_visible (icon, FALSE);
      g_object_unref (icon);
    }

  b_slist_free (icons);
  icons = NULL;

  btk_main_quit ();
}

static void
do_exit (BtkMenuItem *item)
{
  exit (0);
}

static void 
popup_menu (BtkStatusIcon *icon,
	    guint          button,
	    guint32        activate_time)
{
  BtkWidget *menu, *menuitem;

  menu = btk_menu_new ();

  btk_menu_set_screen (BTK_MENU (menu),
                       btk_status_icon_get_screen (icon));

  menuitem = btk_check_menu_item_new_with_label ("Blink");
  btk_check_menu_item_set_active (BTK_CHECK_MENU_ITEM (menuitem), 
				  btk_status_icon_get_blinking (icon));
  g_signal_connect (menuitem, "activate", G_CALLBACK (check_activated), NULL);

  btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);

  btk_widget_show (menuitem);

  menuitem = btk_image_menu_item_new_from_stock (BTK_STOCK_PROPERTIES, NULL);
  g_signal_connect (menuitem, "activate", G_CALLBACK (do_properties), icon);

  btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);

  btk_widget_show (menuitem);

  menuitem = btk_menu_item_new_with_label ("Quit");
  g_signal_connect (menuitem, "activate", G_CALLBACK (do_quit), NULL);

  btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);

  btk_widget_show (menuitem);

  menuitem = btk_menu_item_new_with_label ("Exit abruptly");
  g_signal_connect (menuitem, "activate", G_CALLBACK (do_exit), NULL);

  btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);

  btk_widget_show (menuitem);

  btk_menu_popup (BTK_MENU (menu), 
		  NULL, NULL,
		  btk_status_icon_position_menu, icon,
		  button, activate_time);
}

int
main (int argc, char **argv)
{
  BdkDisplay *display;
  guint n_screens, i;

  btk_init (&argc, &argv);

  display = bdk_display_get_default ();

  n_screens = bdk_display_get_n_screens (display);

  for (i = 0; i < n_screens; i++)
    {
      BtkStatusIcon *icon;

      icon = btk_status_icon_new ();
      btk_status_icon_set_screen (icon, bdk_display_get_screen (display, i));
      update_icons ();

      g_signal_connect (icon, "size-changed", G_CALLBACK (size_changed_cb), NULL);
      g_signal_connect (icon, "notify::embedded", G_CALLBACK (embedded_changed_cb), NULL);
      g_signal_connect (icon, "notify::orientation", G_CALLBACK (orientation_changed_cb), NULL);
      g_signal_connect (icon, "notify::screen", G_CALLBACK (screen_changed_cb), NULL);
      g_print ("icon size %d\n", btk_status_icon_get_size (icon));
      btk_status_icon_set_blinking (BTK_STATUS_ICON (icon), FALSE);

      g_signal_connect (icon, "activate",
                        G_CALLBACK (icon_activated), NULL);

      g_signal_connect (icon, "popup-menu",
                        G_CALLBACK (popup_menu), NULL);

      icons = b_slist_append (icons, icon);
 
      update_icons ();

      timeout = bdk_threads_add_timeout (2000, timeout_handler, icon);
    }

  btk_main ();

  return 0;
}
