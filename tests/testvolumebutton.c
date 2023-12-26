/* testvolumebutton.c
 * Copyright (C) 2007  Red Hat, Inc.
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
value_changed (BtkWidget *button,
               gdouble volume,
               gpointer user_data)
{
  g_message ("volume changed to %f", volume);
}

static void
toggle_orientation (BtkWidget *button,
                    BtkWidget *scalebutton)
{
  if (btk_orientable_get_orientation (BTK_ORIENTABLE (scalebutton)) ==
      BTK_ORIENTATION_HORIZONTAL)
    {
      btk_orientable_set_orientation (BTK_ORIENTABLE (scalebutton),
                                        BTK_ORIENTATION_VERTICAL);
    }
  else
    {
      btk_orientable_set_orientation (BTK_ORIENTABLE (scalebutton),
                                        BTK_ORIENTATION_HORIZONTAL);
    }
}

static void
response_cb (BtkDialog *dialog,
             gint       arg1,
             gpointer   user_data)
{
  btk_widget_destroy (BTK_WIDGET (dialog));
}

static gboolean
show_error (gpointer data)
{
  BtkWindow *window = (BtkWindow *) data;
  BtkWidget *dialog;

  g_message ("showing error");

  dialog = btk_message_dialog_new (window,
                                   BTK_DIALOG_MODAL,
                                   BTK_MESSAGE_INFO,
                                   BTK_BUTTONS_CLOSE,
                                   "This should have unbroken the grab");
  g_signal_connect (G_OBJECT (dialog),
                    "response",
                    G_CALLBACK (response_cb), NULL);
  btk_widget_show (dialog);

  return FALSE;
}

int
main (int    argc,
      char **argv)
{
  BtkWidget *window;
  BtkWidget *button;
  BtkWidget *button2;
  BtkWidget *button3;
  BtkWidget *box;

  btk_init (&argc, &argv);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  button = btk_volume_button_new ();
  button2 = btk_volume_button_new ();
  box = btk_hbox_new (FALSE, 0);

  g_signal_connect (G_OBJECT (button), "value-changed",
                    G_CALLBACK (value_changed),
                    NULL);

  btk_container_add (BTK_CONTAINER (window), box);
  btk_container_add (BTK_CONTAINER (box), button);
  btk_container_add (BTK_CONTAINER (box), button2);

  button3 = btk_button_new_with_label ("Toggle orientation");
  btk_container_add (BTK_CONTAINER (box), button3);

  g_signal_connect (G_OBJECT (button3), "clicked",
                    G_CALLBACK (toggle_orientation),
                    button);
  g_signal_connect (G_OBJECT (button3), "clicked",
                    G_CALLBACK (toggle_orientation),
                    button2);

  btk_widget_show_all (window);
  btk_button_clicked (BTK_BUTTON (button));
  g_timeout_add (4000, (GSourceFunc) show_error, window);

  btk_main ();

  return 0;
}
