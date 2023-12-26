
#include <btk/btk.h>

/* Create a Button Box with the specified parameters */
static BtkWidget *create_bbox( gint  horizontal,
                               char *title,
                               gint  spacing,
                               gint  child_w,
                               gint  child_h,
                               gint  layout )
{
  BtkWidget *frame;
  BtkWidget *bbox;
  BtkWidget *button;

  frame = btk_frame_new (title);

  if (horizontal)
    bbox = btk_hbutton_box_new ();
  else
    bbox = btk_vbutton_box_new ();

  btk_container_set_border_width (BTK_CONTAINER (bbox), 5);
  btk_container_add (BTK_CONTAINER (frame), bbox);

  /* Set the appearance of the Button Box */
  btk_button_box_set_layout (BTK_BUTTON_BOX (bbox), layout);
  btk_box_set_spacing (BTK_BOX (bbox), spacing);
  /*btk_button_box_set_child_size (BTK_BUTTON_BOX (bbox), child_w, child_h);*/

  button = btk_button_new_from_stock (BTK_STOCK_OK);
  btk_container_add (BTK_CONTAINER (bbox), button);

  button = btk_button_new_from_stock (BTK_STOCK_CANCEL);
  btk_container_add (BTK_CONTAINER (bbox), button);

  button = btk_button_new_from_stock (BTK_STOCK_HELP);
  btk_container_add (BTK_CONTAINER (bbox), button);

  return frame;
}

int main( int   argc,
          char *argv[] )
{
  static BtkWidget* window = NULL;
  BtkWidget *main_vbox;
  BtkWidget *vbox;
  BtkWidget *hbox;
  BtkWidget *frame_horz;
  BtkWidget *frame_vert;

  /* Initialize BTK */
  btk_init (&argc, &argv);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_title (BTK_WINDOW (window), "Button Boxes");

  g_signal_connect (B_OBJECT (window), "destroy",
		    G_CALLBACK (btk_main_quit),
		    NULL);

  btk_container_set_border_width (BTK_CONTAINER (window), 10);

  main_vbox = btk_vbox_new (FALSE, 0);
  btk_container_add (BTK_CONTAINER (window), main_vbox);

  frame_horz = btk_frame_new ("Horizontal Button Boxes");
  btk_box_pack_start (BTK_BOX (main_vbox), frame_horz, TRUE, TRUE, 10);

  vbox = btk_vbox_new (FALSE, 0);
  btk_container_set_border_width (BTK_CONTAINER (vbox), 10);
  btk_container_add (BTK_CONTAINER (frame_horz), vbox);

  btk_box_pack_start (BTK_BOX (vbox),
	   create_bbox (TRUE, "Spread (spacing 40)", 40, 85, 20, BTK_BUTTONBOX_SPREAD),
		      TRUE, TRUE, 0);

  btk_box_pack_start (BTK_BOX (vbox),
	   create_bbox (TRUE, "Edge (spacing 30)", 30, 85, 20, BTK_BUTTONBOX_EDGE),
		      TRUE, TRUE, 5);

  btk_box_pack_start (BTK_BOX (vbox),
           create_bbox (TRUE, "Start (spacing 20)", 20, 85, 20, BTK_BUTTONBOX_START),
		      TRUE, TRUE, 5);

  btk_box_pack_start (BTK_BOX (vbox),
	   create_bbox (TRUE, "End (spacing 10)", 10, 85, 20, BTK_BUTTONBOX_END),
		      TRUE, TRUE, 5);

  frame_vert = btk_frame_new ("Vertical Button Boxes");
  btk_box_pack_start (BTK_BOX (main_vbox), frame_vert, TRUE, TRUE, 10);

  hbox = btk_hbox_new (FALSE, 0);
  btk_container_set_border_width (BTK_CONTAINER (hbox), 10);
  btk_container_add (BTK_CONTAINER (frame_vert), hbox);

  btk_box_pack_start (BTK_BOX (hbox),
           create_bbox (FALSE, "Spread (spacing 5)", 5, 85, 20, BTK_BUTTONBOX_SPREAD),
		      TRUE, TRUE, 0);

  btk_box_pack_start (BTK_BOX (hbox),
           create_bbox (FALSE, "Edge (spacing 30)", 30, 85, 20, BTK_BUTTONBOX_EDGE),
		      TRUE, TRUE, 5);

  btk_box_pack_start (BTK_BOX (hbox),
           create_bbox (FALSE, "Start (spacing 20)", 20, 85, 20, BTK_BUTTONBOX_START),
		      TRUE, TRUE, 5);

  btk_box_pack_start (BTK_BOX (hbox),
           create_bbox (FALSE, "End (spacing 20)", 20, 85, 20, BTK_BUTTONBOX_END),
		      TRUE, TRUE, 5);

  btk_widget_show_all (window);

  /* Enter the event loop */
  btk_main ();
    
  return 0;
}
