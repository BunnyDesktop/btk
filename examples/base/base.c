
#include <btk/btk.h>

int main( int   argc,
          char *argv[] )
{
    BtkWidget *window;
    
    btk_init (&argc, &argv);
    
    window = btk_window_new (BTK_WINDOW_TOPLEVEL);
    btk_widget_show  (window);
    
    btk_main ();
    
    return 0;
}
