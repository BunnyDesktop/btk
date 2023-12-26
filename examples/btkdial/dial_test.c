
#include <stdio.h>
#include <stdlib.h>
#include <btk/btk.h>
#include "btkdial.h"

void value_changed( BtkAdjustment *adjustment,
                    BtkWidget     *label )
{
  char buffer[16];

  sprintf(buffer,"%4.2f",adjustment->value);
  btk_label_set_text (BTK_LABEL (label), buffer);
}

int main( int   argc,
          char *argv[])
{
  BtkWidget *window;
  BtkAdjustment *adjustment;
  BtkWidget *dial;
  BtkWidget *frame;
  BtkWidget *vbox;
  BtkWidget *label;

  btk_init (&argc, &argv);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);

  btk_window_set_title (BTK_WINDOW (window), "Dial");

  g_signal_connect (window, "destroy",
		    G_CALLBACK (exit), NULL);

  btk_container_set_border_width (BTK_CONTAINER (window), 10);

  vbox = btk_vbox_new (FALSE, 5);
  btk_container_add (BTK_CONTAINER (window), vbox);
  btk_widget_show (vbox);

  frame = btk_frame_new (NULL);
  btk_frame_set_shadow_type (BTK_FRAME (frame), BTK_SHADOW_IN);
  btk_container_add (BTK_CONTAINER (vbox), frame);
  btk_widget_show (frame);

  adjustment = BTK_ADJUSTMENT (btk_adjustment_new (0, 0, 100, 0.01, 0.1, 0));

  dial = btk_dial_new (adjustment);
  btk_dial_set_update_policy (BTK_DIAL (dial), BTK_UPDATE_DELAYED);
  /*  btk_widget_set_size_request (dial, 100, 100); */

  btk_container_add (BTK_CONTAINER (frame), dial);
  btk_widget_show (dial);

  label = btk_label_new ("0.00");
  btk_box_pack_end (BTK_BOX (vbox), label, 0, 0, 0);
  btk_widget_show (label);

  g_signal_connect (adjustment, "value-changed",
		    G_CALLBACK (value_changed),
                    label);

  btk_widget_show (window);

  btk_main ();

  return 0;
}
