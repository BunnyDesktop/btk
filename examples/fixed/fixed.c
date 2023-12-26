
#include <btk/btk.h>

/* I'm going to be lazy and use some global variables to
 * store the position of the widget within the fixed
 * container */
gint x = 50;
gint y = 50;

/* This callback function moves the button to a new position
 * in the Fixed container. */
static void move_button( BtkWidget *widget,
                         BtkWidget *fixed )
{
  x = (x + 30) % 300;
  y = (y + 50) % 300;
  btk_fixed_move (BTK_FIXED (fixed), widget, x, y); 
}

int main( int   argc,
          char *argv[] )
{
  /* BtkWidget is the storage type for widgets */
  BtkWidget *window;
  BtkWidget *fixed;
  BtkWidget *button;
  gint i;

  /* Initialise BTK */
  btk_init (&argc, &argv);
    
  /* Create a new window */
  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_title (BTK_WINDOW (window), "Fixed Container");

  /* Here we connect the "destroy" event to a signal handler */ 
  g_signal_connect (B_OBJECT (window), "destroy",
		    G_CALLBACK (btk_main_quit), NULL);
 
  /* Sets the border width of the window. */
  btk_container_set_border_width (BTK_CONTAINER (window), 10);

  /* Create a Fixed Container */
  fixed = btk_fixed_new ();
  btk_container_add (BTK_CONTAINER (window), fixed);
  btk_widget_show (fixed);
  
  for (i = 1 ; i <= 3 ; i++) {
    /* Creates a new button with the label "Press me" */
    button = btk_button_new_with_label ("Press me");
  
    /* When the button receives the "clicked" signal, it will call the
     * function move_button() passing it the Fixed Container as its
     * argument. */
    g_signal_connect (B_OBJECT (button), "clicked",
		      G_CALLBACK (move_button), (gpointer) fixed);
  
    /* This packs the button into the fixed containers window. */
    btk_fixed_put (BTK_FIXED (fixed), button, i*50, i*50);
  
    /* The final step is to display this newly created widget. */
    btk_widget_show (button);
  }

  /* Display the window */
  btk_widget_show (window);
    
  /* Enter the event loop */
  btk_main ();
    
  return 0;
}
