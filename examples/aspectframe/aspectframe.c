
#include <btk/btk.h>
   
int main( int argc,
          char *argv[] )
{
    BtkWidget *window;
    BtkWidget *aspect_frame;
    BtkWidget *drawing_area;
    btk_init (&argc, &argv);
   
    window = btk_window_new (BTK_WINDOW_TOPLEVEL);
    btk_window_set_title (BTK_WINDOW (window), "Aspect Frame");
    g_signal_connect (B_OBJECT (window), "destroy",
	              G_CALLBACK (btk_main_quit), NULL);
    btk_container_set_border_width (BTK_CONTAINER (window), 10);
   
    /* Create an aspect_frame and add it to our toplevel window */
   
    aspect_frame = btk_aspect_frame_new ("2x1", /* label */
                                         0.5, /* center x */
                                         0.5, /* center y */
                                         2, /* xsize/ysize = 2 */
                                         FALSE /* ignore child's aspect */);
   
    btk_container_add (BTK_CONTAINER (window), aspect_frame);
    btk_widget_show (aspect_frame);
   
    /* Now add a child widget to the aspect frame */
   
    drawing_area = btk_drawing_area_new ();
   
    /* Ask for a 200x200 window, but the AspectFrame will give us a 200x100
     * window since we are forcing a 2x1 aspect ratio */
    btk_widget_set_size_request (drawing_area, 200, 200);
    btk_container_add (BTK_CONTAINER (aspect_frame), drawing_area);
    btk_widget_show (drawing_area);
   
    btk_widget_show (window);
    btk_main ();
    return 0;
}
