/* testsocket_common.c
 * Copyright (C) 2001 Red Hat, Inc
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
#if defined (BDK_WINDOWING_X11)
#include "x11/bdkx.h"
#elif defined (BDK_WINDOWING_WIN32)
#include "win32/bdkwin32.h"
#endif

enum
{
  ACTION_FILE_NEW,
  ACTION_FILE_OPEN,
  ACTION_OK,
  ACTION_HELP_ABOUT
};

static void
print_hello (BtkWidget *w,
	     guint      action)
{
  switch (action)
    {
    case ACTION_FILE_NEW:
      g_message ("File New activated");
      break;
    case ACTION_FILE_OPEN:
      g_message ("File Open activated");
      break;
    case ACTION_OK:
      g_message ("OK activated");
      break;
    case ACTION_HELP_ABOUT:
      g_message ("Help About activated ");
      break;
    default:
      g_assert_not_reached ();
      break;
    }
}

static BtkItemFactoryEntry menu_items[] = {
  { "/_File",         NULL,         NULL,           0, "<Branch>" },
  { "/File/_New",     "<control>N", print_hello,    ACTION_FILE_NEW, "<Item>" },
  { "/File/_Open",    "<control>O", print_hello,    ACTION_FILE_OPEN, "<Item>" },
  { "/File/sep1",     NULL,         NULL,           0, "<Separator>" },
  { "/File/Quit",     "<control>Q", btk_main_quit,  0, "<Item>" },
  { "/O_K",            "<control>K",print_hello,    ACTION_OK, "<Item>" },
  { "/_Help",         NULL,         NULL,           0, "<LastBranch>" },
  { "/_Help/About",   NULL,         print_hello,    ACTION_HELP_ABOUT, "<Item>" },
};

static void
remove_buttons (BtkWidget *widget, BtkWidget *other_button)
{
  btk_widget_destroy (other_button);
  btk_widget_destroy (widget);
}

static gboolean
blink_cb (gpointer data)
{
  BtkWidget *widget = data;

  btk_widget_show (widget);
  g_object_set_data (G_OBJECT (widget), "blink", NULL);

  return FALSE;
}

static void
blink (BtkWidget *widget,
       BtkWidget *window)
{
  guint blink_timeout = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (window), "blink"));
  
  if (!blink_timeout)
    {
      blink_timeout = bdk_threads_add_timeout (1000, blink_cb, window);
      btk_widget_hide (window);

      g_object_set_data (G_OBJECT (window), "blink", GUINT_TO_POINTER (blink_timeout));
    }
}

static void
local_destroy (BtkWidget *window)
{
  guint blink_timeout = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (window), "blink"));
  if (blink_timeout)
    g_source_remove (blink_timeout);
}

static void
remote_destroy (BtkWidget *window)
{
  local_destroy (window);
  btk_main_quit ();
}

static void
add_buttons (BtkWidget *widget, BtkWidget *box)
{
  BtkWidget *add_button;
  BtkWidget *remove_button;

  add_button = btk_button_new_with_mnemonic ("_Add");
  btk_box_pack_start (BTK_BOX (box), add_button, TRUE, TRUE, 0);
  btk_widget_show (add_button);

  g_signal_connect (add_button, "clicked",
		    G_CALLBACK (add_buttons),
		    box);

  remove_button = btk_button_new_with_mnemonic ("_Remove");
  btk_box_pack_start (BTK_BOX (box), remove_button, TRUE, TRUE, 0);
  btk_widget_show (remove_button);

  g_signal_connect (remove_button, "clicked",
		    G_CALLBACK (remove_buttons),
		    add_button);
}

static BtkWidget *
create_combo (void)
{
  BtkComboBoxText *combo;
  BtkWidget *entry;

  combo = BTK_COMBO_BOX_TEXT (btk_combo_box_text_new_with_entry ());

  btk_combo_box_text_append_text (combo, "item0");
  btk_combo_box_text_append_text (combo, "item1 item1");
  btk_combo_box_text_append_text (combo, "item2 item2 item2");
  btk_combo_box_text_append_text (combo, "item3 item3 item3 item3");
  btk_combo_box_text_append_text (combo, "item4 item4 item4 item4 item4");
  btk_combo_box_text_append_text (combo, "item5 item5 item5 item5 item5 item5");
  btk_combo_box_text_append_text (combo, "item6 item6 item6 item6 item6");
  btk_combo_box_text_append_text (combo, "item7 item7 item7 item7");
  btk_combo_box_text_append_text (combo, "item8 item8 item8");
  btk_combo_box_text_append_text (combo, "item9 item9");

  entry = btk_bin_get_child (BTK_BIN (combo));
  btk_entry_set_text (BTK_ENTRY (entry), "hello world");
  btk_editable_select_rebunnyion (BTK_EDITABLE (entry), 0, -1);

  return BTK_WIDGET (combo);
}

static BtkWidget *
create_menubar (BtkWindow *window)
{
  BtkItemFactory *item_factory;
  BtkAccelGroup *accel_group=NULL;
  BtkWidget *menubar;
  
  accel_group = btk_accel_group_new ();
  item_factory = btk_item_factory_new (BTK_TYPE_MENU_BAR, "<main>",
                                       accel_group);
  btk_item_factory_create_items (item_factory,
				 G_N_ELEMENTS (menu_items),
				 menu_items, NULL);
  
  btk_window_add_accel_group (window, accel_group);
  menubar = btk_item_factory_get_widget (item_factory, "<main>");

  return menubar;
}

static BtkWidget *
create_combo_box (void)
{
  BtkComboBoxText *combo_box = BTK_COMBO_BOX_TEXT (btk_combo_box_text_new ());

  btk_combo_box_text_append_text (combo_box, "This");
  btk_combo_box_text_append_text (combo_box, "Is");
  btk_combo_box_text_append_text (combo_box, "A");
  btk_combo_box_text_append_text (combo_box, "ComboBox");
  
  return BTK_WIDGET (combo_box);
}

static BtkWidget *
create_content (BtkWindow *window, gboolean local)
{
  BtkWidget *vbox;
  BtkWidget *button;
  BtkWidget *frame;

  frame = btk_frame_new (local? "Local" : "Remote");
  btk_container_set_border_width (BTK_CONTAINER (frame), 3);
  vbox = btk_vbox_new (TRUE, 0);
  btk_container_set_border_width (BTK_CONTAINER (vbox), 3);

  btk_container_add (BTK_CONTAINER (frame), vbox);
  
  /* Combo */
  btk_box_pack_start (BTK_BOX (vbox), create_combo(), TRUE, TRUE, 0);

  /* Entry */
  btk_box_pack_start (BTK_BOX (vbox), btk_entry_new(), TRUE, TRUE, 0);

  /* Close Button */
  button = btk_button_new_with_mnemonic ("_Close");
  btk_box_pack_start (BTK_BOX (vbox), button, TRUE, TRUE, 0);
  g_signal_connect_swapped (button, "clicked",
			    G_CALLBACK (btk_widget_destroy), window);

  /* Blink Button */
  button = btk_button_new_with_mnemonic ("_Blink");
  btk_box_pack_start (BTK_BOX (vbox), button, TRUE, TRUE, 0);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (blink),
		    window);

  /* Menubar */
  btk_box_pack_start (BTK_BOX (vbox), create_menubar (BTK_WINDOW (window)),
		      TRUE, TRUE, 0);

  /* Combo Box */
  btk_box_pack_start (BTK_BOX (vbox), create_combo_box (), TRUE, TRUE, 0);
  
  add_buttons (NULL, vbox);

  return frame;
}

guint32
create_child_plug (guint32  xid,
		   gboolean local)
{
  BtkWidget *window;
  BtkWidget *content;

  window = btk_plug_new (xid);

  g_signal_connect (window, "destroy",
		    local ? G_CALLBACK (local_destroy)
			  : G_CALLBACK (remote_destroy),
		    NULL);
  btk_container_set_border_width (BTK_CONTAINER (window), 0);

  content = create_content (BTK_WINDOW (window), local);
  
  btk_container_add (BTK_CONTAINER (window), content);

  btk_widget_show_all (window);

  if (btk_widget_get_realized (window))
#if defined (BDK_WINDOWING_X11)
    return BDK_WINDOW_XID (window->window);
#elif defined (BDK_WINDOWING_WIN32)
    return (guint32) BDK_WINDOW_HWND (window->window);
#endif
  else
    return 0;
}
