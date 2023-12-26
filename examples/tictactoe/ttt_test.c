
#include <stdlib.h>
#include <btk/btk.h>
#include "tictactoe.h"

void win( BtkWidget *widget,
          bpointer   data )
{
  g_print ("Yay!\n");
  tictactoe_clear (TICTACTOE (widget));
}

int main( int   argc,
          char *argv[] )
{
  BtkWidget *window;
  BtkWidget *ttt;
  
  btk_init (&argc, &argv);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  
  btk_window_set_title (BTK_WINDOW (window), "Aspect Frame");
  
  g_signal_connect (B_OBJECT (window), "destroy",
		    G_CALLBACK (exit), NULL);
  
  btk_container_set_border_width (BTK_CONTAINER (window), 10);

  ttt = tictactoe_new ();
  
  btk_container_add (BTK_CONTAINER (window), ttt);
  btk_widget_show (ttt);

  /* And attach to its "tictactoe" signal */
  g_signal_connect (B_OBJECT (ttt), "tictactoe",
		    G_CALLBACK (win), NULL);

  btk_widget_show (window);
  
  btk_main ();
  
  return 0;
}

