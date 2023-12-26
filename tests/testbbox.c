/*
 * Copyright (C) 2006 Nokia Corporation.
 * Author: Xan Lopez <xan.lopez@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <btk/btk.h>

#define N_BUTTONS 3

BtkWidget *bbox = NULL;
BtkWidget *hbbox = NULL, *vbbox = NULL;

static const char* styles[] = { "BTK_BUTTONBOX_DEFAULT_STYLE",
				"BTK_BUTTONBOX_SPREAD",
				"BTK_BUTTONBOX_EDGE",
				"BTK_BUTTONBOX_START",
				"BTK_BUTTONBOX_END",
				"BTK_BUTTONBOX_CENTER",
				NULL};

static const char* types[] = { "BtkHButtonBox",
			       "BtkVButtonBox",
			       NULL};

static void
populate_combo_with (BtkComboBoxText *combo, const char** elements)
{
  int i;
  
  for (i = 0; elements[i] != NULL; i++) {
    btk_combo_box_text_append_text (combo, elements[i]);
  }
  
  btk_combo_box_set_active (BTK_COMBO_BOX (combo), 0);
}

static void
combo_changed_cb (BtkComboBoxText *combo,
		  gpointer user_data)
{
  char *text;
  int i;
  
  text = btk_combo_box_text_get_active_text (combo);
  
  for (i = 0; styles[i]; i++) {
    if (g_str_equal (text, styles[i])) {
      btk_button_box_set_layout (BTK_BUTTON_BOX (bbox), (BtkButtonBoxStyle)i);
    }
  }
}

static void
reparent_widget (BtkWidget *widget,
		 BtkWidget *old_parent,
		 BtkWidget *new_parent)
{
  g_object_ref (widget);
  btk_container_remove (BTK_CONTAINER (old_parent), widget);
  btk_container_add (BTK_CONTAINER (new_parent), widget);
  g_object_unref (widget);
}

static void
combo_types_changed_cb (BtkComboBoxText *combo,
			BtkWidget **buttons)
{
  int i;
  char *text;
  BtkWidget *old_parent, *new_parent;
  BtkButtonBoxStyle style;
  
  text = btk_combo_box_text_get_active_text (combo);
  
  if (g_str_equal (text, "BtkHButtonBox")) {
    old_parent = vbbox;
    new_parent = hbbox;
  } else {
    old_parent = hbbox;
    new_parent = vbbox;
  }
  
  bbox = new_parent;
  
  for (i = 0; i < N_BUTTONS; i++) {
    reparent_widget (buttons[i], old_parent, new_parent);
  }
  
  btk_widget_hide (old_parent);
  style = btk_button_box_get_layout (BTK_BUTTON_BOX (old_parent));
  btk_button_box_set_layout (BTK_BUTTON_BOX (new_parent), style);
  btk_widget_show (new_parent);
}

static void
option_cb (BtkToggleButton *option,
	   BtkWidget *button)
{
  gboolean active = btk_toggle_button_get_active (option);
  
  btk_button_box_set_child_secondary (BTK_BUTTON_BOX (bbox),
				      button, active);
}

static const char* strings[] = { "Ok", "Cancel", "Help" };

int
main (int    argc,
      char **argv)
{
  BtkWidget *window, *buttons[N_BUTTONS];
  BtkWidget *vbox, *hbox, *combo_styles, *combo_types, *option;
  int i;
  
  btk_init (&argc, &argv);
  
  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (btk_main_quit), NULL);
  
  vbox = btk_vbox_new (FALSE, 0);
  btk_container_add (BTK_CONTAINER (window), vbox);
  
  /* BtkHButtonBox */
  
  hbbox = btk_hbutton_box_new ();
  btk_box_pack_start (BTK_BOX (vbox), hbbox, TRUE, TRUE, 5);
  
  for (i = 0; i < N_BUTTONS; i++) {
    buttons[i] = btk_button_new_with_label (strings[i]);
    btk_container_add (BTK_CONTAINER (hbbox), buttons[i]);
  }
  
  bbox = hbbox;
  
  /* BtkVButtonBox */
  vbbox = btk_vbutton_box_new ();
  btk_box_pack_start (BTK_BOX (vbox), vbbox, TRUE, TRUE, 5);
  
  /* Options */
  
  hbox = btk_hbox_new (FALSE, 0);
  btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  
  combo_types = btk_combo_box_text_new ();
  populate_combo_with (BTK_COMBO_BOX_TEXT (combo_types), types);
  g_signal_connect (G_OBJECT (combo_types), "changed", G_CALLBACK (combo_types_changed_cb), buttons);
  btk_box_pack_start (BTK_BOX (hbox), combo_types, TRUE, TRUE, 0);
  
  combo_styles = btk_combo_box_text_new ();
  populate_combo_with (BTK_COMBO_BOX_TEXT (combo_styles), styles);
  g_signal_connect (G_OBJECT (combo_styles), "changed", G_CALLBACK (combo_changed_cb), NULL);
  btk_box_pack_start (BTK_BOX (hbox), combo_styles, TRUE, TRUE, 0);
  
  option = btk_check_button_new_with_label ("Help is secondary");
  g_signal_connect (G_OBJECT (option), "toggled", G_CALLBACK (option_cb), buttons[N_BUTTONS - 1]);
  
  btk_box_pack_start (BTK_BOX (hbox), option, FALSE, FALSE, 0);
  
  btk_widget_show_all (window);
  btk_widget_hide (vbbox);
  
  btk_main ();
  
  return 0;
}
