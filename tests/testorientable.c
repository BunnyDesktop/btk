/* testorientable.c
 * Copyright (C) 2004  Red Hat, Inc.
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

static void
orient_toggled (BtkToggleButton *button, bpointer user_data)
{
  GList *orientables = (GList *) user_data, *ptr;
  bboolean state = btk_toggle_button_get_active (button);
  BtkOrientation orientation;

  if (state)
    {
      orientation = BTK_ORIENTATION_VERTICAL;
      btk_button_set_label (BTK_BUTTON (button), "Vertical");
    }
  else
    {
      orientation = BTK_ORIENTATION_HORIZONTAL;
      btk_button_set_label (BTK_BUTTON (button), "Horizontal");
    }

  for (ptr = orientables; ptr; ptr = ptr->next)
    {
      BtkOrientable *orientable = BTK_ORIENTABLE (ptr->data);

      btk_orientable_set_orientation (orientable, orientation);
    }
}

int
main (int argc, char **argv)
{
  BtkWidget *window;
  BtkWidget *table;
  BtkWidget *box, *button;
  GList *orientables = NULL;

  btk_init (&argc, &argv);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  table = btk_table_new (2, 3, FALSE);
  btk_table_set_row_spacings (BTK_TABLE (table), 12);
  btk_table_set_col_spacings (BTK_TABLE (table), 12);

  /* BtkBox */
  box = btk_hbox_new (6, FALSE);
  orientables = g_list_prepend (orientables, box);
  btk_table_attach_defaults (BTK_TABLE (table), box, 0, 1, 1, 2);
  btk_box_pack_start (BTK_BOX (box),
                  btk_button_new_with_label ("BtkBox 1"),
                  TRUE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (box),
                  btk_button_new_with_label ("BtkBox 2"),
                  TRUE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (box),
                  btk_button_new_with_label ("BtkBox 3"),
                  TRUE, TRUE, 0);

  /* BtkButtonBox */
  box = btk_hbutton_box_new ();
  orientables = g_list_prepend (orientables, box);
  btk_table_attach_defaults (BTK_TABLE (table), box, 1, 2, 1, 2);
  btk_box_pack_start (BTK_BOX (box),
                  btk_button_new_with_label ("BtkButtonBox 1"),
                  TRUE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (box),
                  btk_button_new_with_label ("BtkButtonBox 2"),
                  TRUE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (box),
                  btk_button_new_with_label ("BtkButtonBox 3"),
                  TRUE, TRUE, 0);

  /* BtkSeparator */
  box = btk_hseparator_new ();
  orientables = g_list_prepend (orientables, box);
  btk_table_attach_defaults (BTK_TABLE (table), box, 2, 3, 1, 2);

  button = btk_toggle_button_new_with_label ("Horizontal");
  btk_table_attach (BTK_TABLE (table), button, 0, 1, 0, 1,
                  BTK_FILL, BTK_FILL, 0, 0);
  g_signal_connect (button, "toggled",
                  G_CALLBACK (orient_toggled), orientables);

  btk_container_add (BTK_CONTAINER (window), table);
  btk_widget_show_all (window);

  g_signal_connect (window, "destroy",
                  G_CALLBACK (btk_main_quit), NULL);

  btk_main ();

  return 0;
}
