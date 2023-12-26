
#include <stdlib.h>
#include <btk/btk.h>
#include <bunnylib.h>

BtkWidget *status_bar;

static void push_item( BtkWidget *widget,
                       bpointer   data )
{
  static int count = 1;
  bchar *buff;

  buff = g_strdup_printf ("Item %d", count++);
  btk_statusbar_push (BTK_STATUSBAR (status_bar), BPOINTER_TO_INT (data), buff);
  g_free (buff);
}

static void pop_item( BtkWidget *widget,
                      bpointer   data )
{
  btk_statusbar_pop (BTK_STATUSBAR (status_bar), BPOINTER_TO_INT (data));
}

int main( int   argc,
          char *argv[] )
{

    BtkWidget *window;
    BtkWidget *vbox;
    BtkWidget *button;

    bint context_id;

    btk_init (&argc, &argv);

    /* create a new window */
    window = btk_window_new (BTK_WINDOW_TOPLEVEL);
    btk_widget_set_size_request (BTK_WIDGET (window), 200, 100);
    btk_window_set_title (BTK_WINDOW (window), "BTK Statusbar Example");
    g_signal_connect (window, "delete-event",
                      G_CALLBACK (exit), NULL);

    vbox = btk_vbox_new (FALSE, 1);
    btk_container_add (BTK_CONTAINER (window), vbox);
    btk_widget_show (vbox);

    status_bar = btk_statusbar_new ();
    btk_box_pack_start (BTK_BOX (vbox), status_bar, TRUE, TRUE, 0);
    btk_widget_show (status_bar);

    context_id = btk_statusbar_get_context_id(
                          BTK_STATUSBAR (status_bar), "Statusbar example");

    button = btk_button_new_with_label ("push item");
    g_signal_connect (button, "clicked",
                      G_CALLBACK (push_item), BINT_TO_POINTER (context_id));
    btk_box_pack_start (BTK_BOX (vbox), button, TRUE, TRUE, 2);
    btk_widget_show (button);

    button = btk_button_new_with_label ("pop last item");
    g_signal_connect (button, "clicked",
                      G_CALLBACK (pop_item), BINT_TO_POINTER (context_id));
    btk_box_pack_start (BTK_BOX (vbox), button, TRUE, TRUE, 2);
    btk_widget_show (button);

    /* always display the window as the last step so it all splashes on
     * the screen at once. */
    btk_widget_show (window);

    btk_main ();

    return 0;
}
