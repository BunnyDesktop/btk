/* Paned Widgets
 *
 * The BtkHPaned and BtkVPaned Widgets divide their content
 * area into two panes with a divider in between that the
 * user can adjust. A separate child is placed into each
 * pane.
 *
 * There are a number of options that can be set for each pane.
 * This test contains both a horizontal (HPaned) and a vertical
 * (VPaned) widget, and allows you to adjust the options for
 * each side of each widget.
 */

#include <btk/btk.h>

void
toggle_resize (BtkWidget *widget,
	       BtkWidget *child)
{
  BtkPaned *paned = BTK_PANED (btk_widget_get_parent (child));
  bboolean is_child1 = (child == btk_paned_get_child1 (paned));
  bboolean resize, shrink;

  btk_container_child_get (BTK_CONTAINER (paned), child, "resize", &resize, "shrink", &shrink, NULL);

  g_object_ref (child);
  btk_container_remove (BTK_CONTAINER (btk_widget_get_parent (child)), child);
  if (is_child1)
    btk_paned_pack1 (paned, child, !resize, shrink);
  else
    btk_paned_pack2 (paned, child, !resize, shrink);
  g_object_unref (child);
}

void
toggle_shrink (BtkWidget *widget,
	       BtkWidget *child)
{
  BtkPaned *paned = BTK_PANED (btk_widget_get_parent (child));
  bboolean is_child1 = (child == btk_paned_get_child1 (paned));
  bboolean resize, shrink;

  btk_container_child_get (BTK_CONTAINER (paned), child, "resize", &resize, "shrink", &shrink, NULL);

  g_object_ref (child);
  btk_container_remove (BTK_CONTAINER (btk_widget_get_parent (child)), child);
  if (is_child1)
    btk_paned_pack1 (paned, child, resize, !shrink);
  else
    btk_paned_pack2 (paned, child, resize, !shrink);
  g_object_unref (child);
}

BtkWidget *
create_pane_options (BtkPaned	 *paned,
		     const bchar *frame_label,
		     const bchar *label1,
		     const bchar *label2)
{
  BtkWidget *frame;
  BtkWidget *table;
  BtkWidget *label;
  BtkWidget *check_button;
  BtkWidget *child1;
  BtkWidget *child2;
  
  child1 = btk_paned_get_child1 (paned);
  child2 = btk_paned_get_child2 (paned);
  
  frame = btk_frame_new (frame_label);
  btk_container_set_border_width (BTK_CONTAINER (frame), 4);
  
  table = btk_table_new (3, 2, TRUE);
  btk_container_add (BTK_CONTAINER (frame), table);
  
  label = btk_label_new (label1);
  btk_table_attach_defaults (BTK_TABLE (table), label,
			     0, 1, 0, 1);
  
  check_button = btk_check_button_new_with_mnemonic ("_Resize");
  btk_table_attach_defaults (BTK_TABLE (table), check_button,
			     0, 1, 1, 2);
  g_signal_connect (check_button, "toggled",
		    G_CALLBACK (toggle_resize), child1);
  
  check_button = btk_check_button_new_with_mnemonic ("_Shrink");
  btk_table_attach_defaults (BTK_TABLE (table), check_button,
			     0, 1, 2, 3);
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (check_button),
			       TRUE);
  g_signal_connect (check_button, "toggled",
		    G_CALLBACK (toggle_shrink), child1);
  
  label = btk_label_new (label2);
  btk_table_attach_defaults (BTK_TABLE (table), label,
			     1, 2, 0, 1);
  
  check_button = btk_check_button_new_with_mnemonic ("_Resize");
  btk_table_attach_defaults (BTK_TABLE (table), check_button,
			     1, 2, 1, 2);
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (check_button),
			       TRUE);
  g_signal_connect (check_button, "toggled",
		    G_CALLBACK (toggle_resize), child2);
  
  check_button = btk_check_button_new_with_mnemonic ("_Shrink");
  btk_table_attach_defaults (BTK_TABLE (table), check_button,
			     1, 2, 2, 3);
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (check_button),
			       TRUE);
  g_signal_connect (check_button, "toggled",
		    G_CALLBACK (toggle_shrink), child2);

  return frame;
}

BtkWidget *
do_panes (BtkWidget *do_widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *frame;
  BtkWidget *hpaned;
  BtkWidget *vpaned;
  BtkWidget *button;
  BtkWidget *vbox;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (do_widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed), &window);

      btk_window_set_title (BTK_WINDOW (window), "Panes");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      vbox = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), vbox);
      
      vpaned = btk_vpaned_new ();
      btk_box_pack_start (BTK_BOX (vbox), vpaned, TRUE, TRUE, 0);
      btk_container_set_border_width (BTK_CONTAINER(vpaned), 5);

      hpaned = btk_hpaned_new ();
      btk_paned_add1 (BTK_PANED (vpaned), hpaned);

      frame = btk_frame_new (NULL);
      btk_frame_set_shadow_type (BTK_FRAME(frame), BTK_SHADOW_IN);
      btk_widget_set_size_request (frame, 60, 60);
      btk_paned_add1 (BTK_PANED (hpaned), frame);
      
      button = btk_button_new_with_mnemonic ("_Hi there");
      btk_container_add (BTK_CONTAINER(frame), button);

      frame = btk_frame_new (NULL);
      btk_frame_set_shadow_type (BTK_FRAME(frame), BTK_SHADOW_IN);
      btk_widget_set_size_request (frame, 80, 60);
      btk_paned_add2 (BTK_PANED (hpaned), frame);

      frame = btk_frame_new (NULL);
      btk_frame_set_shadow_type (BTK_FRAME(frame), BTK_SHADOW_IN);
      btk_widget_set_size_request (frame, 60, 80);
      btk_paned_add2 (BTK_PANED (vpaned), frame);

      /* Now create toggle buttons to control sizing */

      btk_box_pack_start (BTK_BOX (vbox),
			  create_pane_options (BTK_PANED (hpaned),
					       "Horizontal",
					       "Left",
					       "Right"),
			  FALSE, FALSE, 0);

      btk_box_pack_start (BTK_BOX (vbox),
			  create_pane_options (BTK_PANED (vpaned),
					       "Vertical",
					       "Top",
					       "Bottom"),
			  FALSE, FALSE, 0);

      btk_widget_show_all (vbox);
    }

  if (!btk_widget_get_visible (window))
    {
      btk_widget_show (window);
    }
  else
    {
      btk_widget_destroy (window);
      window = NULL;
    }

  return window;
}
