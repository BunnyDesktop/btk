/* Expander
 *
 * BtkExpander allows to provide additional content that is initially hidden.
 * This is also known as "disclosure triangle".
 *
 */

#include <btk/btk.h>

static BtkWidget *window = NULL;


BtkWidget *
do_expander (BtkWidget *do_widget)
{
  BtkWidget *vbox;
  BtkWidget *label;
  BtkWidget *expander;
  
  if (!window)
  {
    window = btk_dialog_new_with_buttons ("BtkExpander",
					  BTK_WINDOW (do_widget),
					  0,
					  BTK_STOCK_CLOSE,
					  BTK_RESPONSE_NONE,
					  NULL);
    btk_window_set_resizable (BTK_WINDOW (window), FALSE);

    g_signal_connect (window, "response",
		      G_CALLBACK (btk_widget_destroy), NULL);
    g_signal_connect (window, "destroy",
		      G_CALLBACK (btk_widget_destroyed), &window);

    vbox = btk_vbox_new (FALSE, 5);
    btk_box_pack_start (BTK_BOX (btk_dialog_get_content_area (BTK_DIALOG (window))), vbox, TRUE, TRUE, 0);
    btk_container_set_border_width (BTK_CONTAINER (vbox), 5);

    label = btk_label_new ("Expander demo. Click on the triangle for details.");
    btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);

    /* Create the expander */
    expander = btk_expander_new ("Details");
    btk_box_pack_start (BTK_BOX (vbox), expander, FALSE, FALSE, 0);

    label = btk_label_new ("Details can be shown or hidden.");
    btk_container_add (BTK_CONTAINER (expander), label);
  }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);

  return window;
}


