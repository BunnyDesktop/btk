
#include "config.h"
#include <btk/btk.h>


/* XPM data of Open-File icon */
static const char * xpm_data[] = {
"16 16 3 1",
"       c None",
".      c #000000000000",
"X      c #FFFFFFFFFFFF",
"                ",
"   ......       ",
"   .XXX.X.      ",
"   .XXX.XX.     ",
"   .XXX.XXX.    ",
"   .XXX.....    ",
"   .XXXXXXX.    ",
"   .XXXXXXX.    ",
"   .XXXXXXX.    ",
"   .XXXXXXX.    ",
"   .XXXXXXX.    ",
"   .XXXXXXX.    ",
"   .XXXXXXX.    ",
"   .........    ",
"                ",
"                "};


/* when invoked (via signal delete_event), terminates the application.
 */
gint close_application( BtkWidget *widget,
                        BdkEvent  *event,
                        gpointer   data )
{
    btk_main_quit ();
    return FALSE;
}


/* is invoked when the button is clicked.  It just prints a message.
 */
void button_clicked( BtkWidget *widget,
                     gpointer   data ) {
    g_print ("button clicked\n");
}

int main( int   argc,
          char *argv[] )
{
    /* BtkWidget is the storage type for widgets */
    BtkWidget *window, *pixmapwid, *button;
    BdkPixmap *pixmap;
    BdkBitmap *mask;
    BtkStyle *style;

    /* create the main window, and attach delete_event signal to terminating
       the application */
    btk_init (&argc, &argv);
    window = btk_window_new (BTK_WINDOW_TOPLEVEL);
    g_signal_connect (window, "delete-event",
                      G_CALLBACK (close_application), NULL);
    btk_container_set_border_width (BTK_CONTAINER (window), 10);
    btk_widget_show (window);

    /* now for the pixmap from bdk */
    style = btk_widget_get_style (window);
    pixmap = bdk_pixmap_create_from_xpm_d (window->window,  &mask,
                                           &style->bg[BTK_STATE_NORMAL],
                                           (gchar **)xpm_data);

    /* a pixmap widget to contain the pixmap */
    pixmapwid = btk_image_new_from_pixmap (pixmap, mask);
    btk_widget_show (pixmapwid);

    /* a button to contain the pixmap widget */
    button = btk_button_new ();
    btk_container_add (BTK_CONTAINER (button), pixmapwid);
    btk_container_add (BTK_CONTAINER (window), button);
    btk_widget_show (button);

    g_signal_connect (button, "clicked",
                      G_CALLBACK (button_clicked), NULL);

    /* show the window */
    btk_main ();

    return 0;
}
