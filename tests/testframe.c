/* testframe.c
 * Copyright (C) 2007  Xan López <xan@bunny.org>
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
#include <math.h>

static void
spin_ythickness_cb (BtkSpinButton *spin, bpointer user_data)
{
  BtkWidget *frame = user_data;
  BtkRcStyle *rcstyle;

  rcstyle = btk_rc_style_new ();
  rcstyle->xthickness = BTK_WIDGET (frame)->style->xthickness;
  rcstyle->ythickness = btk_spin_button_get_value (spin);
  btk_widget_modify_style (frame, rcstyle);

  g_object_unref (rcstyle);
}

static void
spin_xthickness_cb (BtkSpinButton *spin, bpointer user_data)
{
  BtkWidget *frame = user_data;
  BtkRcStyle *rcstyle;

  rcstyle = btk_rc_style_new ();
  rcstyle->xthickness = btk_spin_button_get_value (spin);
  rcstyle->ythickness = BTK_WIDGET (frame)->style->ythickness;
  btk_widget_modify_style (frame, rcstyle);

  g_object_unref (rcstyle);
}

/* Function to normalize rounding errors in FP arithmetic to
   our desired limits */

#define EPSILON 1e-10

static bdouble
double_normalize (bdouble n)
{
  if (fabs (1.0 - n) < EPSILON)
    n = 1.0;
  else if (n < EPSILON)
    n = 0.0;

  return n;
}

static void
spin_xalign_cb (BtkSpinButton *spin, BtkFrame *frame)
{
  bdouble xalign = double_normalize (btk_spin_button_get_value (spin));
  btk_frame_set_label_align (frame, xalign, frame->label_yalign);
}

static void
spin_yalign_cb (BtkSpinButton *spin, BtkFrame *frame)
{
  bdouble yalign = double_normalize (btk_spin_button_get_value (spin));
  btk_frame_set_label_align (frame, frame->label_xalign, yalign);
}

int main (int argc, char **argv)
{
  BtkWidget *window, *frame, *xthickness_spin, *ythickness_spin, *vbox;
  BtkWidget *xalign_spin, *yalign_spin, *button, *table, *label;

  btk_init (&argc, &argv);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_container_set_border_width (BTK_CONTAINER (window), 5);
  btk_window_set_default_size (BTK_WINDOW (window), 300, 300);

  g_signal_connect (B_OBJECT (window), "delete-event", btk_main_quit, NULL);

  vbox = btk_vbox_new (FALSE, 5);
  btk_container_add (BTK_CONTAINER (window), vbox);

  frame = btk_frame_new ("Testing");
  btk_box_pack_start (BTK_BOX (vbox), frame, TRUE, TRUE, 0);

  button = btk_button_new_with_label ("Hello!");
  btk_container_add (BTK_CONTAINER (frame), button);

  table = btk_table_new (4, 2, FALSE);
  btk_box_pack_start (BTK_BOX (vbox), table, FALSE, FALSE, 0);

  /* Spin to control xthickness */
  label = btk_label_new ("xthickness: ");
  btk_table_attach_defaults (BTK_TABLE (table), label, 0, 1, 0, 1);

  xthickness_spin = btk_spin_button_new_with_range (0, 250, 1);
  g_signal_connect (B_OBJECT (xthickness_spin), "value-changed", G_CALLBACK (spin_xthickness_cb), frame);
  btk_spin_button_set_value (BTK_SPIN_BUTTON (xthickness_spin), frame->style->xthickness);
  btk_table_attach_defaults (BTK_TABLE (table), xthickness_spin, 1, 2, 0, 1);

  /* Spin to control ythickness */
  label = btk_label_new ("ythickness: ");
  btk_table_attach_defaults (BTK_TABLE (table), label, 0, 1, 1, 2);

  ythickness_spin = btk_spin_button_new_with_range (0, 250, 1);
  g_signal_connect (B_OBJECT (ythickness_spin), "value-changed", G_CALLBACK (spin_ythickness_cb), frame);
  btk_spin_button_set_value (BTK_SPIN_BUTTON (ythickness_spin), frame->style->ythickness);
  btk_table_attach_defaults (BTK_TABLE (table), ythickness_spin, 1, 2, 1, 2);

  /* Spin to control label xalign */
  label = btk_label_new ("xalign: ");
  btk_table_attach_defaults (BTK_TABLE (table), label, 0, 1, 2, 3);

  xalign_spin = btk_spin_button_new_with_range (0.0, 1.0, 0.1);
  g_signal_connect (B_OBJECT (xalign_spin), "value-changed", G_CALLBACK (spin_xalign_cb), frame);
  btk_spin_button_set_value (BTK_SPIN_BUTTON (xalign_spin), BTK_FRAME (frame)->label_xalign);
  btk_table_attach_defaults (BTK_TABLE (table), xalign_spin, 1, 2, 2, 3);

  /* Spin to control label yalign */
  label = btk_label_new ("yalign: ");
  btk_table_attach_defaults (BTK_TABLE (table), label, 0, 1, 3, 4);

  yalign_spin = btk_spin_button_new_with_range (0.0, 1.0, 0.1);
  g_signal_connect (B_OBJECT (yalign_spin), "value-changed", G_CALLBACK (spin_yalign_cb), frame);
  btk_spin_button_set_value (BTK_SPIN_BUTTON (yalign_spin), BTK_FRAME (frame)->label_yalign);
  btk_table_attach_defaults (BTK_TABLE (table), yalign_spin, 1, 2, 3, 4);

  btk_widget_show_all (window);

  btk_main ();

  return 0;
}
