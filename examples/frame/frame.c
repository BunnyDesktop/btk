
#include <btk/btk.h>

int main( int   argc,
          char *argv[] )
{
  /* BtkWidget is the storage type for widgets */
  BtkWidget *window;
  BtkWidget *frame;

  /* Initialise BTK */
  btk_init (&argc, &argv);
    
  /* Create a new window */
  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_title (BTK_WINDOW (window), "Frame Example");

  /* Here we connect the "destroy" event to a signal handler */ 
  g_signal_connect (B_OBJECT (window), "destroy",
		    G_CALLBACK (btk_main_quit), NULL);

  btk_widget_set_size_request (window, 300, 300);
  /* Sets the border width of the window. */
  btk_container_set_border_width (BTK_CONTAINER (window), 10);

  /* Create a Frame */
  frame = btk_frame_new (NULL);
  btk_container_add (BTK_CONTAINER (window), frame);

  /* Set the frame's label */
  btk_frame_set_label (BTK_FRAME (frame), "BTK Frame Widget");

  /* Align the label at the right of the frame */
  btk_frame_set_label_align (BTK_FRAME (frame), 1.0, 0.0);

  /* Set the style of the frame */
  btk_frame_set_shadow_type (BTK_FRAME (frame), BTK_SHADOW_ETCHED_OUT);

  btk_widget_show (frame);
  
  /* Display the window */
  btk_widget_show (window);
    
  /* Enter the event loop */
  btk_main ();
    
  return 0;
}
