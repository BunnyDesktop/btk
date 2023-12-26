/* Button Boxes
 *
 * The Button Box widgets are used to arrange buttons with padding.
 */

#include <btk/btk.h>

static BtkWidget *
create_bbox (bint  horizontal,
	     char *title, 
	     bint  spacing,
	     bint  layout)
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

  btk_button_box_set_layout (BTK_BUTTON_BOX (bbox), layout);
  btk_box_set_spacing (BTK_BOX (bbox), spacing);
  
  button = btk_button_new_from_stock (BTK_STOCK_OK);
  btk_container_add (BTK_CONTAINER (bbox), button);
  
  button = btk_button_new_from_stock (BTK_STOCK_CANCEL);
  btk_container_add (BTK_CONTAINER (bbox), button);
  
  button = btk_button_new_from_stock (BTK_STOCK_HELP);
  btk_container_add (BTK_CONTAINER (bbox), button);

  return frame;
}

BtkWidget *
do_button_box (BtkWidget *do_widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *main_vbox;
  BtkWidget *vbox;
  BtkWidget *hbox;
  BtkWidget *frame_horz;
  BtkWidget *frame_vert;
	
  if (!window)
  {
    window = btk_window_new (BTK_WINDOW_TOPLEVEL);
    btk_window_set_screen (BTK_WINDOW (window),
			   btk_widget_get_screen (do_widget));
    btk_window_set_title (BTK_WINDOW (window), "Button Boxes");
    
    g_signal_connect (window, "destroy",
		      G_CALLBACK (btk_widget_destroyed),
		      &window);
    
    btk_container_set_border_width (BTK_CONTAINER (window), 10);

    main_vbox = btk_vbox_new (FALSE, 0);
    btk_container_add (BTK_CONTAINER (window), main_vbox);
    
    frame_horz = btk_frame_new ("Horizontal Button Boxes");
    btk_box_pack_start (BTK_BOX (main_vbox), frame_horz, TRUE, TRUE, 10);
    
    vbox = btk_vbox_new (FALSE, 0);
    btk_container_set_border_width (BTK_CONTAINER (vbox), 10);
    btk_container_add (BTK_CONTAINER (frame_horz), vbox);

    btk_box_pack_start (BTK_BOX (vbox), 
			create_bbox (TRUE, "Spread", 40, BTK_BUTTONBOX_SPREAD),
			TRUE, TRUE, 0);

    btk_box_pack_start (BTK_BOX (vbox), 
			create_bbox (TRUE, "Edge", 40, BTK_BUTTONBOX_EDGE),
			TRUE, TRUE, 5);
    
    btk_box_pack_start (BTK_BOX (vbox), 
			create_bbox (TRUE, "Start", 40, BTK_BUTTONBOX_START),
			TRUE, TRUE, 5);
    
    btk_box_pack_start (BTK_BOX (vbox), 
			create_bbox (TRUE, "End", 40, BTK_BUTTONBOX_END),
			TRUE, TRUE, 5);

    frame_vert = btk_frame_new ("Vertical Button Boxes");
    btk_box_pack_start (BTK_BOX (main_vbox), frame_vert, TRUE, TRUE, 10);
    
    hbox = btk_hbox_new (FALSE, 0);
    btk_container_set_border_width (BTK_CONTAINER (hbox), 10);
    btk_container_add (BTK_CONTAINER (frame_vert), hbox);

    btk_box_pack_start (BTK_BOX (hbox), 
			create_bbox (FALSE, "Spread", 30, BTK_BUTTONBOX_SPREAD),
			TRUE, TRUE, 0);

    btk_box_pack_start (BTK_BOX (hbox), 
			create_bbox (FALSE, "Edge", 30, BTK_BUTTONBOX_EDGE),
			TRUE, TRUE, 5);

    btk_box_pack_start (BTK_BOX (hbox), 
			create_bbox (FALSE, "Start", 30, BTK_BUTTONBOX_START),
			TRUE, TRUE, 5);

    btk_box_pack_start (BTK_BOX (hbox), 
			create_bbox (FALSE, "End", 30, BTK_BUTTONBOX_END),
			TRUE, TRUE, 5);
  }

  if (!btk_widget_get_visible (window))
    {
      btk_widget_show_all (window);
    }
  else
    {	 
      btk_widget_destroy (window);
      window = NULL;
    }

  return window;
}
