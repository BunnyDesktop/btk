
#include <stdio.h>
#include <btk/btk.h>

static BtkWidget *spinner1;

static void toggle_snap( BtkWidget     *widget,
                         BtkSpinButton *spin )
{
  btk_spin_button_set_snap_to_ticks (spin, BTK_TOGGLE_BUTTON (widget)->active);
}

static void toggle_numeric( BtkWidget *widget,
                            BtkSpinButton *spin )
{
  btk_spin_button_set_numeric (spin, BTK_TOGGLE_BUTTON (widget)->active);
}

static void change_digits( BtkWidget *widget,
                           BtkSpinButton *spin )
{
  btk_spin_button_set_digits (BTK_SPIN_BUTTON (spinner1),
                              btk_spin_button_get_value_as_int (spin));
}

static void get_value( BtkWidget *widget,
                       bpointer data )
{
  bchar *buf;
  BtkLabel *label;
  BtkSpinButton *spin;

  spin = BTK_SPIN_BUTTON (spinner1);
  label = BTK_LABEL (g_object_get_data (B_OBJECT (widget), "user_data"));
  if (BPOINTER_TO_INT (data) == 1)
    buf = g_strdup_printf ("%d", btk_spin_button_get_value_as_int (spin));
  else
    buf = g_strdup_printf ("%0.*f", spin->digits,
                           btk_spin_button_get_value (spin));
  btk_label_set_text (label, buf);
  g_free (buf);
}


int main( int   argc,
          char *argv[] )
{
  BtkWidget *window;
  BtkWidget *frame;
  BtkWidget *hbox;
  BtkWidget *main_vbox;
  BtkWidget *vbox;
  BtkWidget *vbox2;
  BtkWidget *spinner2;
  BtkWidget *spinner;
  BtkWidget *button;
  BtkWidget *label;
  BtkWidget *val_label;
  BtkAdjustment *adj;

  /* Initialise BTK */
  btk_init (&argc, &argv);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);

  g_signal_connect (window, "destroy",
		    G_CALLBACK (btk_main_quit),
		    NULL);

  btk_window_set_title (BTK_WINDOW (window), "Spin Button");

  main_vbox = btk_vbox_new (FALSE, 5);
  btk_container_set_border_width (BTK_CONTAINER (main_vbox), 10);
  btk_container_add (BTK_CONTAINER (window), main_vbox);

  frame = btk_frame_new ("Not accelerated");
  btk_box_pack_start (BTK_BOX (main_vbox), frame, TRUE, TRUE, 0);

  vbox = btk_vbox_new (FALSE, 0);
  btk_container_set_border_width (BTK_CONTAINER (vbox), 5);
  btk_container_add (BTK_CONTAINER (frame), vbox);

  /* Day, month, year spinners */

  hbox = btk_hbox_new (FALSE, 0);
  btk_box_pack_start (BTK_BOX (vbox), hbox, TRUE, TRUE, 5);

  vbox2 = btk_vbox_new (FALSE, 0);
  btk_box_pack_start (BTK_BOX (hbox), vbox2, TRUE, TRUE, 5);

  label = btk_label_new ("Day :");
  btk_misc_set_alignment (BTK_MISC (label), 0, 0.5);
  btk_box_pack_start (BTK_BOX (vbox2), label, FALSE, TRUE, 0);

  adj = (BtkAdjustment *) btk_adjustment_new (1.0, 1.0, 31.0, 1.0,
					      5.0, 0.0);
  spinner = btk_spin_button_new (adj, 0, 0);
  btk_spin_button_set_wrap (BTK_SPIN_BUTTON (spinner), TRUE);
  btk_box_pack_start (BTK_BOX (vbox2), spinner, FALSE, TRUE, 0);

  vbox2 = btk_vbox_new (FALSE, 0);
  btk_box_pack_start (BTK_BOX (hbox), vbox2, TRUE, TRUE, 5);

  label = btk_label_new ("Month :");
  btk_misc_set_alignment (BTK_MISC (label), 0, 0.5);
  btk_box_pack_start (BTK_BOX (vbox2), label, FALSE, TRUE, 0);

  adj = (BtkAdjustment *) btk_adjustment_new (1.0, 1.0, 12.0, 1.0,
					      5.0, 0.0);
  spinner = btk_spin_button_new (adj, 0, 0);
  btk_spin_button_set_wrap (BTK_SPIN_BUTTON (spinner), TRUE);
  btk_box_pack_start (BTK_BOX (vbox2), spinner, FALSE, TRUE, 0);

  vbox2 = btk_vbox_new (FALSE, 0);
  btk_box_pack_start (BTK_BOX (hbox), vbox2, TRUE, TRUE, 5);

  label = btk_label_new ("Year :");
  btk_misc_set_alignment (BTK_MISC (label), 0, 0.5);
  btk_box_pack_start (BTK_BOX (vbox2), label, FALSE, TRUE, 0);

  adj = (BtkAdjustment *) btk_adjustment_new (1998.0, 0.0, 2100.0,
					      1.0, 100.0, 0.0);
  spinner = btk_spin_button_new (adj, 0, 0);
  btk_spin_button_set_wrap (BTK_SPIN_BUTTON (spinner), FALSE);
  btk_widget_set_size_request (spinner, 55, -1);
  btk_box_pack_start (BTK_BOX (vbox2), spinner, FALSE, TRUE, 0);

  frame = btk_frame_new ("Accelerated");
  btk_box_pack_start (BTK_BOX (main_vbox), frame, TRUE, TRUE, 0);

  vbox = btk_vbox_new (FALSE, 0);
  btk_container_set_border_width (BTK_CONTAINER (vbox), 5);
  btk_container_add (BTK_CONTAINER (frame), vbox);

  hbox = btk_hbox_new (FALSE, 0);
  btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, TRUE, 5);

  vbox2 = btk_vbox_new (FALSE, 0);
  btk_box_pack_start (BTK_BOX (hbox), vbox2, TRUE, TRUE, 5);

  label = btk_label_new ("Value :");
  btk_misc_set_alignment (BTK_MISC (label), 0, 0.5);
  btk_box_pack_start (BTK_BOX (vbox2), label, FALSE, TRUE, 0);

  adj = (BtkAdjustment *) btk_adjustment_new (0.0, -10000.0, 10000.0,
					      0.5, 100.0, 0.0);
  spinner1 = btk_spin_button_new (adj, 1.0, 2);
  btk_spin_button_set_wrap (BTK_SPIN_BUTTON (spinner1), TRUE);
  btk_widget_set_size_request (spinner1, 100, -1);
  btk_box_pack_start (BTK_BOX (vbox2), spinner1, FALSE, TRUE, 0);

  vbox2 = btk_vbox_new (FALSE, 0);
  btk_box_pack_start (BTK_BOX (hbox), vbox2, TRUE, TRUE, 5);

  label = btk_label_new ("Digits :");
  btk_misc_set_alignment (BTK_MISC (label), 0, 0.5);
  btk_box_pack_start (BTK_BOX (vbox2), label, FALSE, TRUE, 0);

  adj = (BtkAdjustment *) btk_adjustment_new (2, 1, 5, 1, 1, 0);
  spinner2 = btk_spin_button_new (adj, 0.0, 0);
  btk_spin_button_set_wrap (BTK_SPIN_BUTTON (spinner2), TRUE);
  g_signal_connect (adj, "value-changed",
		    G_CALLBACK (change_digits),
		    spinner2);
  btk_box_pack_start (BTK_BOX (vbox2), spinner2, FALSE, TRUE, 0);

  hbox = btk_hbox_new (FALSE, 0);
  btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, TRUE, 5);

  button = btk_check_button_new_with_label ("Snap to 0.5-ticks");
  g_signal_connect (button, "clicked",
		    G_CALLBACK (toggle_snap),
		    spinner1);
  btk_box_pack_start (BTK_BOX (vbox), button, TRUE, TRUE, 0);
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (button), TRUE);

  button = btk_check_button_new_with_label ("Numeric only input mode");
  g_signal_connect (button, "clicked",
		    G_CALLBACK (toggle_numeric),
                    spinner1);
  btk_box_pack_start (BTK_BOX (vbox), button, TRUE, TRUE, 0);
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (button), TRUE);

  val_label = btk_label_new ("");

  hbox = btk_hbox_new (FALSE, 0);
  btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, TRUE, 5);
  button = btk_button_new_with_label ("Value as Int");
  g_object_set_data (B_OBJECT (button), "user_data", val_label);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (get_value),
		    BINT_TO_POINTER (1));
  btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 5);

  button = btk_button_new_with_label ("Value as Float");
  g_object_set_data (B_OBJECT (button), "user_data", val_label);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (get_value),
		    BINT_TO_POINTER (2));
  btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 5);

  btk_box_pack_start (BTK_BOX (vbox), val_label, TRUE, TRUE, 0);
  btk_label_set_text (BTK_LABEL (val_label), "0");

  hbox = btk_hbox_new (FALSE, 0);
  btk_box_pack_start (BTK_BOX (main_vbox), hbox, FALSE, TRUE, 0);

  button = btk_button_new_with_label ("Close");
  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (btk_widget_destroy),
			    window);
  btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 5);

  btk_widget_show_all (window);

  /* Enter the event loop */
  btk_main ();

  return 0;
}

