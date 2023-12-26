
#include <stdlib.h>
#include <btk/btk.h>

/* Create a new hbox with an image and a label packed into it
 * and return the box. */

static BtkWidget *xpm_label_box( bchar     *xpm_filename,
                                 bchar     *label_text )
{
    BtkWidget *box;
    BtkWidget *label;
    BtkWidget *image;

    /* Create box for image and label */
    box = btk_hbox_new (FALSE, 0);
    btk_container_set_border_width (BTK_CONTAINER (box), 2);

    /* Now on to the image stuff */
    image = btk_image_new_from_file (xpm_filename);

    /* Create a label for the button */
    label = btk_label_new (label_text);

    /* Pack the image and label into the box */
    btk_box_pack_start (BTK_BOX (box), image, FALSE, FALSE, 3);
    btk_box_pack_start (BTK_BOX (box), label, FALSE, FALSE, 3);

    btk_widget_show (image);
    btk_widget_show (label);

    return box;
}

/* Our usual callback function */
static void callback( BtkWidget *widget,
                      bpointer   data )
{
    g_print ("Hello again - %s was pressed\n", (char *) data);
}

int main( int   argc,
          char *argv[] )
{
    /* BtkWidget is the storage type for widgets */
    BtkWidget *window;
    BtkWidget *button;
    BtkWidget *box;

    btk_init (&argc, &argv);

    /* Create a new window */
    window = btk_window_new (BTK_WINDOW_TOPLEVEL);

    btk_window_set_title (BTK_WINDOW (window), "Pixmap'd Buttons!");

    /* It's a good idea to do this for all windows. */
    g_signal_connect (B_OBJECT (window), "destroy",
	              G_CALLBACK (btk_main_quit), NULL);

    g_signal_connect (B_OBJECT (window), "delete-event",
	 	      G_CALLBACK (btk_main_quit), NULL);

    /* Sets the border width of the window. */
    btk_container_set_border_width (BTK_CONTAINER (window), 10);

    /* Create a new button */
    button = btk_button_new ();

    /* Connect the "clicked" signal of the button to our callback */
    g_signal_connect (B_OBJECT (button), "clicked",
		      G_CALLBACK (callback), (bpointer) "cool button");

    /* This calls our box creating function */
    box = xpm_label_box ("info.xpm", "cool button");

    /* Pack and show all our widgets */
    btk_widget_show (box);

    btk_container_add (BTK_CONTAINER (button), box);

    btk_widget_show (button);

    btk_container_add (BTK_CONTAINER (window), button);

    btk_widget_show (window);

    /* Rest in btk_main and wait for the fun to begin! */
    btk_main ();

    return 0;
}
