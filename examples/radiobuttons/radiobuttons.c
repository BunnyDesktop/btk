
#include <bunnylib.h>
#include <btk/btk.h>

static bboolean close_application( BtkWidget *widget,
                                   BdkEvent  *event,
                                   bpointer   data )
{
  btk_main_quit ();
  return FALSE;
}

int main( int   argc,
          char *argv[] )
{
    BtkWidget *window = NULL;
    BtkWidget *box1;
    BtkWidget *box2;
    BtkWidget *button;
    BtkWidget *separator;
    GSList *group;

    btk_init (&argc, &argv);

    window = btk_window_new (BTK_WINDOW_TOPLEVEL);

    g_signal_connect (window, "delete-event",
		      G_CALLBACK (close_application),
                      NULL);

    btk_window_set_title (BTK_WINDOW (window), "radio buttons");
    btk_container_set_border_width (BTK_CONTAINER (window), 0);

    box1 = btk_vbox_new (FALSE, 0);
    btk_container_add (BTK_CONTAINER (window), box1);
    btk_widget_show (box1);

    box2 = btk_vbox_new (FALSE, 10);
    btk_container_set_border_width (BTK_CONTAINER (box2), 10);
    btk_box_pack_start (BTK_BOX (box1), box2, TRUE, TRUE, 0);
    btk_widget_show (box2);

    button = btk_radio_button_new_with_label (NULL, "button1");
    btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
    btk_widget_show (button);

    group = btk_radio_button_get_group (BTK_RADIO_BUTTON (button));
    button = btk_radio_button_new_with_label (group, "button2");
    btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (button), TRUE);
    btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
    btk_widget_show (button);

    button = btk_radio_button_new_with_label_from_widget (BTK_RADIO_BUTTON (button),
	                                                  "button3");
    btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
    btk_widget_show (button);

    separator = btk_hseparator_new ();
    btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);
    btk_widget_show (separator);

    box2 = btk_vbox_new (FALSE, 10);
    btk_container_set_border_width (BTK_CONTAINER (box2), 10);
    btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);
    btk_widget_show (box2);

    button = btk_button_new_with_label ("close");
    g_signal_connect_swapped (button, "clicked",
                              G_CALLBACK (close_application),
                              window);
    btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
    btk_widget_set_can_default (button, TRUE);
    btk_widget_grab_default (button);
    btk_widget_show (button);
    btk_widget_show (window);

    btk_main ();

    return 0;
}
