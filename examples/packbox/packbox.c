
#include <stdio.h>
#include <stdlib.h>
#include "btk/btk.h"

static gboolean delete_event( BtkWidget *widget,
                              BdkEvent  *event,
                              gpointer   data )
{
    btk_main_quit ();
    return FALSE;
}

/* Make a new hbox filled with button-labels. Arguments for the
 * variables we're interested are passed in to this function.
 * We do not show the box, but do show everything inside. */
static BtkWidget *make_box( gboolean homogeneous,
                            gint     spacing,
                            gboolean expand,
                            gboolean fill,
                            guint    padding )
{
    BtkWidget *box;
    BtkWidget *button;
    char padstr[80];

    /* Create a new hbox with the appropriate homogeneous
     * and spacing settings */
    box = btk_hbox_new (homogeneous, spacing);

    /* Create a series of buttons with the appropriate settings */
    button = btk_button_new_with_label ("btk_box_pack");
    btk_box_pack_start (BTK_BOX (box), button, expand, fill, padding);
    btk_widget_show (button);

    button = btk_button_new_with_label ("(box,");
    btk_box_pack_start (BTK_BOX (box), button, expand, fill, padding);
    btk_widget_show (button);

    button = btk_button_new_with_label ("button,");
    btk_box_pack_start (BTK_BOX (box), button, expand, fill, padding);
    btk_widget_show (button);

    /* Create a button with the label depending on the value of
     * expand. */
    if (expand == TRUE)
	    button = btk_button_new_with_label ("TRUE,");
    else
	    button = btk_button_new_with_label ("FALSE,");

    btk_box_pack_start (BTK_BOX (box), button, expand, fill, padding);
    btk_widget_show (button);

    /* This is the same as the button creation for "expand"
     * above, but uses the shorthand form. */
    button = btk_button_new_with_label (fill ? "TRUE," : "FALSE,");
    btk_box_pack_start (BTK_BOX (box), button, expand, fill, padding);
    btk_widget_show (button);

    sprintf (padstr, "%d);", padding);

    button = btk_button_new_with_label (padstr);
    btk_box_pack_start (BTK_BOX (box), button, expand, fill, padding);
    btk_widget_show (button);

    return box;
}

int main( int   argc,
          char *argv[])
{
    BtkWidget *window;
    BtkWidget *button;
    BtkWidget *box1;
    BtkWidget *box2;
    BtkWidget *separator;
    BtkWidget *label;
    BtkWidget *quitbox;
    int which;

    /* Our init, don't forget this! :) */
    btk_init (&argc, &argv);

    if (argc != 2) {
	fprintf (stderr, "usage: packbox num, where num is 1, 2, or 3.\n");
	/* This just does cleanup in BTK and exits with an exit status of 1. */
	exit (1);
    }

    which = atoi (argv[1]);

    /* Create our window */
    window = btk_window_new (BTK_WINDOW_TOPLEVEL);

    /* You should always remember to connect the "delete-event" signal
     * to the main window. This is very important for proper intuitive
     * behavior */
    g_signal_connect (window, "delete-event",
		      G_CALLBACK (delete_event), NULL);
    btk_container_set_border_width (BTK_CONTAINER (window), 10);

    /* We create a vertical box (vbox) to pack the horizontal boxes into.
     * This allows us to stack the horizontal boxes filled with buttons one
     * on top of the other in this vbox. */
    box1 = btk_vbox_new (FALSE, 0);

    /* which example to show. These correspond to the pictures above. */
    switch (which) {
    case 1:
	/* create a new label. */
	label = btk_label_new ("btk_hbox_new (FALSE, 0);");

	/* Align the label to the left side.  We'll discuss this function and
	 * others in the section on Widget Attributes. */
	btk_misc_set_alignment (BTK_MISC (label), 0, 0);

	/* Pack the label into the vertical box (vbox box1).  Remember that
	 * widgets added to a vbox will be packed one on top of the other in
	 * order. */
	btk_box_pack_start (BTK_BOX (box1), label, FALSE, FALSE, 0);

	/* Show the label */
	btk_widget_show (label);

	/* Call our make box function - homogeneous = FALSE, spacing = 0,
	 * expand = FALSE, fill = FALSE, padding = 0 */
	box2 = make_box (FALSE, 0, FALSE, FALSE, 0);
	btk_box_pack_start (BTK_BOX (box1), box2, FALSE, FALSE, 0);
	btk_widget_show (box2);

	/* Call our make box function - homogeneous = FALSE, spacing = 0,
	 * expand = TRUE, fill = FALSE, padding = 0 */
	box2 = make_box (FALSE, 0, TRUE, FALSE, 0);
	btk_box_pack_start (BTK_BOX (box1), box2, FALSE, FALSE, 0);
	btk_widget_show (box2);

	/* Args are: homogeneous, spacing, expand, fill, padding */
	box2 = make_box (FALSE, 0, TRUE, TRUE, 0);
	btk_box_pack_start (BTK_BOX (box1), box2, FALSE, FALSE, 0);
	btk_widget_show (box2);

	/* Creates a separator, we'll learn more about these later,
	 * but they are quite simple. */
	separator = btk_hseparator_new ();

        /* Pack the separator into the vbox. Remember each of these
         * widgets is being packed into a vbox, so they'll be stacked
	 * vertically. */
	btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 5);
	btk_widget_show (separator);

	/* Create another new label, and show it. */
	label = btk_label_new ("btk_hbox_new (TRUE, 0);");
	btk_misc_set_alignment (BTK_MISC (label), 0, 0);
	btk_box_pack_start (BTK_BOX (box1), label, FALSE, FALSE, 0);
	btk_widget_show (label);

	/* Args are: homogeneous, spacing, expand, fill, padding */
	box2 = make_box (TRUE, 0, TRUE, FALSE, 0);
	btk_box_pack_start (BTK_BOX (box1), box2, FALSE, FALSE, 0);
	btk_widget_show (box2);

	/* Args are: homogeneous, spacing, expand, fill, padding */
	box2 = make_box (TRUE, 0, TRUE, TRUE, 0);
	btk_box_pack_start (BTK_BOX (box1), box2, FALSE, FALSE, 0);
	btk_widget_show (box2);

	/* Another new separator. */
	separator = btk_hseparator_new ();
	/* The last 3 arguments to btk_box_pack_start are:
	 * expand, fill, padding. */
	btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 5);
	btk_widget_show (separator);

	break;

    case 2:

	/* Create a new label, remember box1 is a vbox as created
	 * near the beginning of main() */
	label = btk_label_new ("btk_hbox_new (FALSE, 10);");
	btk_misc_set_alignment (BTK_MISC (label), 0, 0);
	btk_box_pack_start (BTK_BOX (box1), label, FALSE, FALSE, 0);
	btk_widget_show (label);

	/* Args are: homogeneous, spacing, expand, fill, padding */
	box2 = make_box (FALSE, 10, TRUE, FALSE, 0);
	btk_box_pack_start (BTK_BOX (box1), box2, FALSE, FALSE, 0);
	btk_widget_show (box2);

	/* Args are: homogeneous, spacing, expand, fill, padding */
	box2 = make_box (FALSE, 10, TRUE, TRUE, 0);
	btk_box_pack_start (BTK_BOX (box1), box2, FALSE, FALSE, 0);
	btk_widget_show (box2);

	separator = btk_hseparator_new ();
	/* The last 3 arguments to btk_box_pack_start are:
	 * expand, fill, padding. */
	btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 5);
	btk_widget_show (separator);

	label = btk_label_new ("btk_hbox_new (FALSE, 0);");
	btk_misc_set_alignment (BTK_MISC (label), 0, 0);
	btk_box_pack_start (BTK_BOX (box1), label, FALSE, FALSE, 0);
	btk_widget_show (label);

	/* Args are: homogeneous, spacing, expand, fill, padding */
	box2 = make_box (FALSE, 0, TRUE, FALSE, 10);
	btk_box_pack_start (BTK_BOX (box1), box2, FALSE, FALSE, 0);
	btk_widget_show (box2);

	/* Args are: homogeneous, spacing, expand, fill, padding */
	box2 = make_box (FALSE, 0, TRUE, TRUE, 10);
	btk_box_pack_start (BTK_BOX (box1), box2, FALSE, FALSE, 0);
	btk_widget_show (box2);

	separator = btk_hseparator_new ();
	/* The last 3 arguments to btk_box_pack_start are: expand, fill, padding. */
	btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 5);
	btk_widget_show (separator);
	break;

    case 3:

        /* This demonstrates the ability to use btk_box_pack_end() to
	 * right justify widgets. First, we create a new box as before. */
	box2 = make_box (FALSE, 0, FALSE, FALSE, 0);

	/* Create the label that will be put at the end. */
	label = btk_label_new ("end");
	/* Pack it using btk_box_pack_end(), so it is put on the right
	 * side of the hbox created in the make_box() call. */
	btk_box_pack_end (BTK_BOX (box2), label, FALSE, FALSE, 0);
	/* Show the label. */
	btk_widget_show (label);

	/* Pack box2 into box1 (the vbox remember ? :) */
	btk_box_pack_start (BTK_BOX (box1), box2, FALSE, FALSE, 0);
	btk_widget_show (box2);

	/* A separator for the bottom. */
	separator = btk_hseparator_new ();
	/* This explicitly sets the separator to 400 pixels wide by 5 pixels
	 * high. This is so the hbox we created will also be 400 pixels wide,
	 * and the "end" label will be separated from the other labels in the
	 * hbox. Otherwise, all the widgets in the hbox would be packed as
	 * close together as possible. */
	btk_widget_set_size_request (separator, 400, 5);
	/* pack the separator into the vbox (box1) created near the start
	 * of main() */
	btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 5);
	btk_widget_show (separator);
    }

    /* Create another new hbox.. remember we can use as many as we need! */
    quitbox = btk_hbox_new (FALSE, 0);

    /* Our quit button. */
    button = btk_button_new_with_label ("Quit");

    /* Setup the signal to terminate the program when the button is clicked */
    g_signal_connect_swapped (button, "clicked",
			      G_CALLBACK (btk_main_quit),
			      window);
    /* Pack the button into the quitbox.
     * The last 3 arguments to btk_box_pack_start are:
     * expand, fill, padding. */
    btk_box_pack_start (BTK_BOX (quitbox), button, TRUE, FALSE, 0);
    /* pack the quitbox into the vbox (box1) */
    btk_box_pack_start (BTK_BOX (box1), quitbox, FALSE, FALSE, 0);

    /* Pack the vbox (box1) which now contains all our widgets, into the
     * main window. */
    btk_container_add (BTK_CONTAINER (window), box1);

    /* And show everything left */
    btk_widget_show (button);
    btk_widget_show (quitbox);

    btk_widget_show (box1);
    /* Showing the window last so everything pops up at once. */
    btk_widget_show (window);

    /* And of course, our main function. */
    btk_main ();

    /* Control returns here when btk_main_quit() is called, but not when
     * exit() is used. */

    return 0;
}
