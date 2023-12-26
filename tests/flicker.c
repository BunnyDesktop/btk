#include <btk/btk.h>

BtkWidget*
create_flicker (void)
{
  BtkWidget *window1;
  BtkWidget *hpaned1;
  BtkWidget *vpaned2;
  BtkWidget *hbox2;
  BtkObject *spinbutton7_adj;
  BtkWidget *spinbutton7;
  BtkObject *spinbutton8_adj;
  BtkWidget *spinbutton8;
  BtkWidget *vbox1;
  BtkObject *spinbutton9_adj;
  BtkWidget *spinbutton9;
  BtkObject *spinbutton10_adj;
  BtkWidget *spinbutton10;
  BtkObject *spinbutton11_adj;
  BtkWidget *spinbutton11;
  BtkObject *spinbutton12_adj;
  BtkWidget *spinbutton12;
  BtkObject *spinbutton13_adj;
  BtkWidget *spinbutton13;
  BtkObject *spinbutton14_adj;
  BtkWidget *spinbutton14;
  BtkObject *spinbutton15_adj;
  BtkWidget *spinbutton15;
  BtkObject *spinbutton16_adj;
  BtkWidget *spinbutton16;
  BtkWidget *vpaned1;
  BtkWidget *hbox1;
  BtkObject *spinbutton17_adj;
  BtkWidget *spinbutton17;
  BtkObject *spinbutton18_adj;
  BtkWidget *spinbutton18;
  BtkObject *spinbutton19_adj;
  BtkWidget *spinbutton19;
  BtkWidget *vbox2;
  BtkObject *spinbutton20_adj;
  BtkWidget *spinbutton20;
  BtkObject *spinbutton21_adj;
  BtkWidget *spinbutton21;
  BtkObject *spinbutton22_adj;
  BtkWidget *spinbutton22;
  BtkObject *spinbutton23_adj;
  BtkWidget *spinbutton23;
  BtkObject *spinbutton24_adj;
  BtkWidget *spinbutton24;
  BtkObject *spinbutton25_adj;
  BtkWidget *spinbutton25;
  BtkObject *spinbutton26_adj;
  BtkWidget *spinbutton26;
  BtkObject *spinbutton27_adj;
  BtkWidget *spinbutton27;

  window1 = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_default_size (BTK_WINDOW (window1), 500, 400);
  btk_window_set_title (BTK_WINDOW (window1), "window1");

  hpaned1 = btk_hpaned_new ();
  btk_widget_show (hpaned1);
  btk_container_add (BTK_CONTAINER (window1), hpaned1);
  btk_paned_set_position (BTK_PANED (hpaned1), 100);

  vpaned2 = btk_vpaned_new ();
  btk_widget_show (vpaned2);
  btk_paned_pack1 (BTK_PANED (hpaned1), vpaned2, FALSE, TRUE);
  btk_paned_set_position (BTK_PANED (vpaned2), 100);

  hbox2 = btk_hbox_new (FALSE, 0);
  btk_widget_show (hbox2);
  btk_paned_pack1 (BTK_PANED (vpaned2), hbox2, FALSE, TRUE);

  spinbutton7_adj = btk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton7 = btk_spin_button_new (BTK_ADJUSTMENT (spinbutton7_adj), 1, 0);
  btk_widget_show (spinbutton7);
  btk_box_pack_start (BTK_BOX (hbox2), spinbutton7, TRUE, TRUE, 0);

  spinbutton8_adj = btk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton8 = btk_spin_button_new (BTK_ADJUSTMENT (spinbutton8_adj), 1, 0);
  btk_widget_show (spinbutton8);
  btk_box_pack_start (BTK_BOX (hbox2), spinbutton8, TRUE, TRUE, 0);

  vbox1 = btk_vbox_new (FALSE, 0);
  btk_widget_show (vbox1);
  btk_paned_pack2 (BTK_PANED (vpaned2), vbox1, TRUE, TRUE);

  spinbutton9_adj = btk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton9 = btk_spin_button_new (BTK_ADJUSTMENT (spinbutton9_adj), 1, 0);
  btk_widget_show (spinbutton9);
  btk_box_pack_start (BTK_BOX (vbox1), spinbutton9, FALSE, FALSE, 0);

  spinbutton10_adj = btk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton10 = btk_spin_button_new (BTK_ADJUSTMENT (spinbutton10_adj), 1, 0);
  btk_widget_show (spinbutton10);
  btk_box_pack_start (BTK_BOX (vbox1), spinbutton10, FALSE, FALSE, 0);

  spinbutton11_adj = btk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton11 = btk_spin_button_new (BTK_ADJUSTMENT (spinbutton11_adj), 1, 0);
  btk_widget_show (spinbutton11);
  btk_box_pack_start (BTK_BOX (vbox1), spinbutton11, FALSE, FALSE, 0);

  spinbutton12_adj = btk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton12 = btk_spin_button_new (BTK_ADJUSTMENT (spinbutton12_adj), 1, 0);
  btk_widget_show (spinbutton12);
  btk_box_pack_start (BTK_BOX (vbox1), spinbutton12, FALSE, FALSE, 0);

  spinbutton13_adj = btk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton13 = btk_spin_button_new (BTK_ADJUSTMENT (spinbutton13_adj), 1, 0);
  btk_widget_show (spinbutton13);
  btk_box_pack_start (BTK_BOX (vbox1), spinbutton13, FALSE, FALSE, 0);

  spinbutton14_adj = btk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton14 = btk_spin_button_new (BTK_ADJUSTMENT (spinbutton14_adj), 1, 0);
  btk_widget_show (spinbutton14);
  btk_box_pack_start (BTK_BOX (vbox1), spinbutton14, FALSE, FALSE, 0);

  spinbutton15_adj = btk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton15 = btk_spin_button_new (BTK_ADJUSTMENT (spinbutton15_adj), 1, 0);
  btk_widget_show (spinbutton15);
  btk_box_pack_start (BTK_BOX (vbox1), spinbutton15, FALSE, FALSE, 0);

  spinbutton16_adj = btk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton16 = btk_spin_button_new (BTK_ADJUSTMENT (spinbutton16_adj), 1, 0);
  btk_widget_show (spinbutton16);
  btk_box_pack_start (BTK_BOX (vbox1), spinbutton16, FALSE, FALSE, 0);

  vpaned1 = btk_vpaned_new ();
  btk_widget_show (vpaned1);
  btk_paned_pack2 (BTK_PANED (hpaned1), vpaned1, TRUE, TRUE);
  btk_paned_set_position (BTK_PANED (vpaned1), 0);

  hbox1 = btk_hbox_new (FALSE, 0);
  btk_widget_show (hbox1);
  btk_paned_pack1 (BTK_PANED (vpaned1), hbox1, FALSE, TRUE);

  spinbutton17_adj = btk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton17 = btk_spin_button_new (BTK_ADJUSTMENT (spinbutton17_adj), 1, 0);
  btk_widget_show (spinbutton17);
  btk_box_pack_start (BTK_BOX (hbox1), spinbutton17, TRUE, TRUE, 0);

  spinbutton18_adj = btk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton18 = btk_spin_button_new (BTK_ADJUSTMENT (spinbutton18_adj), 1, 0);
  btk_widget_show (spinbutton18);
  btk_box_pack_start (BTK_BOX (hbox1), spinbutton18, TRUE, TRUE, 0);

  spinbutton19_adj = btk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton19 = btk_spin_button_new (BTK_ADJUSTMENT (spinbutton19_adj), 1, 0);
  btk_widget_show (spinbutton19);
  btk_box_pack_start (BTK_BOX (hbox1), spinbutton19, TRUE, TRUE, 0);

  vbox2 = btk_vbox_new (FALSE, 0);
  btk_widget_show (vbox2);
  btk_paned_pack2 (BTK_PANED (vpaned1), vbox2, FALSE, FALSE);

  spinbutton20_adj = btk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton20 = btk_spin_button_new (BTK_ADJUSTMENT (spinbutton20_adj), 1, 0);
  btk_widget_show (spinbutton20);
  btk_box_pack_start (BTK_BOX (vbox2), spinbutton20, FALSE, FALSE, 0);

  spinbutton21_adj = btk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton21 = btk_spin_button_new (BTK_ADJUSTMENT (spinbutton21_adj), 1, 0);
  btk_widget_show (spinbutton21);
  btk_box_pack_start (BTK_BOX (vbox2), spinbutton21, FALSE, FALSE, 0);

  spinbutton22_adj = btk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton22 = btk_spin_button_new (BTK_ADJUSTMENT (spinbutton22_adj), 1, 0);
  btk_widget_show (spinbutton22);
  btk_box_pack_start (BTK_BOX (vbox2), spinbutton22, FALSE, FALSE, 0);

  spinbutton23_adj = btk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton23 = btk_spin_button_new (BTK_ADJUSTMENT (spinbutton23_adj), 1, 0);
  btk_widget_show (spinbutton23);
  btk_box_pack_start (BTK_BOX (vbox2), spinbutton23, FALSE, FALSE, 0);

  spinbutton24_adj = btk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton24 = btk_spin_button_new (BTK_ADJUSTMENT (spinbutton24_adj), 1, 0);
  btk_widget_show (spinbutton24);
  btk_box_pack_start (BTK_BOX (vbox2), spinbutton24, FALSE, FALSE, 0);

  spinbutton25_adj = btk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton25 = btk_spin_button_new (BTK_ADJUSTMENT (spinbutton25_adj), 1, 0);
  btk_widget_show (spinbutton25);
  btk_box_pack_start (BTK_BOX (vbox2), spinbutton25, FALSE, FALSE, 0);

  spinbutton26_adj = btk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton26 = btk_spin_button_new (BTK_ADJUSTMENT (spinbutton26_adj), 1, 0);
  btk_widget_show (spinbutton26);
  btk_box_pack_start (BTK_BOX (vbox2), spinbutton26, TRUE, FALSE, 0);

  spinbutton27_adj = btk_adjustment_new (1, 0, 100, 1, 10, 10);
  spinbutton27 = btk_spin_button_new (BTK_ADJUSTMENT (spinbutton27_adj), 1, 0);
  btk_widget_show (spinbutton27);
  btk_box_pack_end (BTK_BOX (vbox2), spinbutton27, FALSE, FALSE, 0);


  return window1;
}


int
main (int argc, char *argv[])
{
  BtkWidget *window1;

  btk_init (&argc, &argv);

  window1 = create_flicker ();
  btk_widget_show (window1);

  btk_main ();
  return 0;
}

