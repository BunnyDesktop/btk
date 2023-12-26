
#include <btk/btk.h>

/* Create an Arrow widget with the specified parameters
 * and pack it into a button */
static BtkWidget *create_arrow_button( BtkArrowType  arrow_type,
                                       BtkShadowType shadow_type )
{
  BtkWidget *button;
  BtkWidget *arrow;

  button = btk_button_new ();
  arrow = btk_arrow_new (arrow_type, shadow_type);

  btk_container_add (BTK_CONTAINER (button), arrow);
  
  btk_widget_show (button);
  btk_widget_show (arrow);

  return button;
}

int main( int   argc,
          char *argv[] )
{
  /* BtkWidget is the storage type for widgets */
  BtkWidget *window;
  BtkWidget *button;
  BtkWidget *box;

  /* Initialize the toolkit */
  btk_init (&argc, &argv);

  /* Create a new window */
  window = btk_window_new (BTK_WINDOW_TOPLEVEL);

  btk_window_set_title (BTK_WINDOW (window), "Arrow Buttons");

  /* It's a good idea to do this for all windows. */
  g_signal_connect (B_OBJECT (window), "destroy",
                    G_CALLBACK (btk_main_quit), NULL);

  /* Sets the border width of the window. */
  btk_container_set_border_width (BTK_CONTAINER (window), 10);

  /* Create a box to hold the arrows/buttons */
  box = btk_hbox_new (FALSE, 0);
  btk_container_set_border_width (BTK_CONTAINER (box), 2);
  btk_container_add (BTK_CONTAINER (window), box);

  /* Pack and show all our widgets */
  btk_widget_show (box);

  button = create_arrow_button (BTK_ARROW_UP, BTK_SHADOW_IN);
  btk_box_pack_start (BTK_BOX (box), button, FALSE, FALSE, 3);

  button = create_arrow_button (BTK_ARROW_DOWN, BTK_SHADOW_OUT);
  btk_box_pack_start (BTK_BOX (box), button, FALSE, FALSE, 3);
  
  button = create_arrow_button (BTK_ARROW_LEFT, BTK_SHADOW_ETCHED_IN);
  btk_box_pack_start (BTK_BOX (box), button, FALSE, FALSE, 3);
  
  button = create_arrow_button (BTK_ARROW_RIGHT, BTK_SHADOW_ETCHED_OUT);
  btk_box_pack_start (BTK_BOX (box), button, FALSE, FALSE, 3);
  
  btk_widget_show (window);
  
  /* Rest in btk_main and wait for the fun to begin! */
  btk_main ();
  
  return 0;
}
