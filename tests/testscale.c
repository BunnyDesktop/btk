/* testscale.c - scale mark demo
 * Copyright (C) 2009 Red Hat, Inc.
 * Author: Matthias Clasen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <btk/btk.h>

int main (int argc, char *argv[])
{
  BtkWidget *window;
  BtkWidget *box;
  BtkWidget *box2;
  BtkWidget *frame;
  BtkWidget *scale;
  bdouble marks[3] = { 0.0, 50.0, 100.0 };
  const bchar *labels[3] = { 
    "<small>Left</small>", 
    "<small>Middle</small>", 
    "<small>Right</small>" 
  };

  bdouble bath_marks[4] = { 0.0, 33.3, 66.6, 100.0 };
  const bchar *bath_labels[4] = { 
    "<span color='blue' size='small'>Cold</span>", 
    "<span size='small'>Baby bath</span>", 
    "<span size='small'>Hot tub</span>", 
    "<span color='Red' size='small'>Hot</span>" 
  };

  btk_init (&argc, &argv);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_title (BTK_WINDOW (window), "Ranges with marks");
  box = btk_vbox_new (FALSE, 5);

  frame = btk_frame_new ("No marks");
  scale = btk_hscale_new_with_range (0, 100, 1);
  btk_scale_set_draw_value (BTK_SCALE (scale), FALSE);
  btk_container_add (BTK_CONTAINER (frame), scale);
  btk_box_pack_start (BTK_BOX (box), frame, FALSE, FALSE, 0);

  frame = btk_frame_new ("Simple marks");
  scale = btk_hscale_new_with_range (0, 100, 1);
  btk_scale_set_draw_value (BTK_SCALE (scale), FALSE);
  btk_scale_add_mark (BTK_SCALE (scale), marks[0], BTK_POS_BOTTOM, NULL);
  btk_scale_add_mark (BTK_SCALE (scale), marks[1], BTK_POS_BOTTOM, NULL);
  btk_scale_add_mark (BTK_SCALE (scale), marks[2], BTK_POS_BOTTOM, NULL);
  btk_container_add (BTK_CONTAINER (frame), scale);
  btk_box_pack_start (BTK_BOX (box), frame, FALSE, FALSE, 0);
 
  frame = btk_frame_new ("Labeled marks");
  scale = btk_hscale_new_with_range (0, 100, 1);
  btk_scale_set_draw_value (BTK_SCALE (scale), FALSE);
  btk_scale_add_mark (BTK_SCALE (scale), marks[0], BTK_POS_BOTTOM, labels[0]);
  btk_scale_add_mark (BTK_SCALE (scale), marks[1], BTK_POS_BOTTOM, labels[1]);
  btk_scale_add_mark (BTK_SCALE (scale), marks[2], BTK_POS_BOTTOM, labels[2]);
  btk_container_add (BTK_CONTAINER (frame), scale);
  btk_box_pack_start (BTK_BOX (box), frame, FALSE, FALSE, 0);
  
  frame = btk_frame_new ("Some labels");
  scale = btk_hscale_new_with_range (0, 100, 1);
  btk_scale_set_draw_value (BTK_SCALE (scale), FALSE);
  btk_scale_add_mark (BTK_SCALE (scale), marks[0], BTK_POS_BOTTOM, labels[0]);
  btk_scale_add_mark (BTK_SCALE (scale), marks[1], BTK_POS_BOTTOM, NULL);
  btk_scale_add_mark (BTK_SCALE (scale), marks[2], BTK_POS_BOTTOM, labels[2]);
  btk_container_add (BTK_CONTAINER (frame), scale);
  btk_box_pack_start (BTK_BOX (box), frame, FALSE, FALSE, 0);
  
  frame = btk_frame_new ("Above and below");
  scale = btk_hscale_new_with_range (0, 100, 1);
  btk_scale_set_draw_value (BTK_SCALE (scale), FALSE);
  btk_scale_add_mark (BTK_SCALE (scale), bath_marks[0], BTK_POS_TOP, bath_labels[0]);
  btk_scale_add_mark (BTK_SCALE (scale), bath_marks[1], BTK_POS_BOTTOM, bath_labels[1]);
  btk_scale_add_mark (BTK_SCALE (scale), bath_marks[2], BTK_POS_BOTTOM, bath_labels[2]);
  btk_scale_add_mark (BTK_SCALE (scale), bath_marks[3], BTK_POS_TOP, bath_labels[3]);
  btk_container_add (BTK_CONTAINER (frame), scale);
  btk_box_pack_start (BTK_BOX (box), frame, FALSE, FALSE, 0);

  box2 = btk_hbox_new (FALSE, 5);
  btk_box_pack_start (BTK_BOX (box), box2, TRUE, TRUE, 0);

  frame = btk_frame_new ("No marks");
  scale = btk_vscale_new_with_range (0, 100, 1);
  btk_scale_set_draw_value (BTK_SCALE (scale), FALSE);
  btk_container_add (BTK_CONTAINER (frame), scale);
  btk_box_pack_start (BTK_BOX (box2), frame, FALSE, FALSE, 0);

  frame = btk_frame_new ("Simple marks");
  scale = btk_vscale_new_with_range (0, 100, 1);
  btk_scale_set_draw_value (BTK_SCALE (scale), FALSE);
  btk_scale_add_mark (BTK_SCALE (scale), marks[0], BTK_POS_LEFT, NULL);
  btk_scale_add_mark (BTK_SCALE (scale), marks[1], BTK_POS_LEFT, NULL);
  btk_scale_add_mark (BTK_SCALE (scale), marks[2], BTK_POS_LEFT, NULL);
  btk_container_add (BTK_CONTAINER (frame), scale);
  btk_box_pack_start (BTK_BOX (box2), frame, FALSE, FALSE, 0);
 
  frame = btk_frame_new ("Labeled marks");
  scale = btk_vscale_new_with_range (0, 100, 1);
  btk_scale_set_draw_value (BTK_SCALE (scale), FALSE);
  btk_scale_add_mark (BTK_SCALE (scale), marks[0], BTK_POS_LEFT, labels[0]);
  btk_scale_add_mark (BTK_SCALE (scale), marks[1], BTK_POS_LEFT, labels[1]);
  btk_scale_add_mark (BTK_SCALE (scale), marks[2], BTK_POS_LEFT, labels[2]);
  btk_container_add (BTK_CONTAINER (frame), scale);
  btk_box_pack_start (BTK_BOX (box2), frame, FALSE, FALSE, 0);
  
  frame = btk_frame_new ("Some labels");
  scale = btk_vscale_new_with_range (0, 100, 1);
  btk_scale_set_draw_value (BTK_SCALE (scale), FALSE);
  btk_scale_add_mark (BTK_SCALE (scale), marks[0], BTK_POS_LEFT, labels[0]);
  btk_scale_add_mark (BTK_SCALE (scale), marks[1], BTK_POS_LEFT, NULL);
  btk_scale_add_mark (BTK_SCALE (scale), marks[2], BTK_POS_LEFT, labels[2]);
  btk_container_add (BTK_CONTAINER (frame), scale);
  btk_box_pack_start (BTK_BOX (box2), frame, FALSE, FALSE, 0);
  
  frame = btk_frame_new ("Right and left");
  scale = btk_vscale_new_with_range (0, 100, 1);
  btk_scale_set_draw_value (BTK_SCALE (scale), FALSE);
  btk_scale_add_mark (BTK_SCALE (scale), bath_marks[0], BTK_POS_RIGHT, bath_labels[0]);
  btk_scale_add_mark (BTK_SCALE (scale), bath_marks[1], BTK_POS_LEFT, bath_labels[1]);
  btk_scale_add_mark (BTK_SCALE (scale), bath_marks[2], BTK_POS_LEFT, bath_labels[2]);
  btk_scale_add_mark (BTK_SCALE (scale), bath_marks[3], BTK_POS_RIGHT, bath_labels[3]);
  btk_container_add (BTK_CONTAINER (frame), scale);
  btk_box_pack_start (BTK_BOX (box2), frame, FALSE, FALSE, 0);

  btk_container_add (BTK_CONTAINER (window), box);
  btk_widget_show_all (window);

  btk_main ();

  return 0;
}


