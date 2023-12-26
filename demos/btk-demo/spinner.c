/* Spinner
 *
 * BtkSpinner allows to show that background activity is on-going.
 *
 */

#include <btk/btk.h>

static BtkWidget *window = NULL;
static BtkWidget *spinner_sensitive = NULL;
static BtkWidget *spinner_unsensitive = NULL;

static void
on_play_clicked (BtkButton *button, bpointer user_data)
{
  btk_spinner_start (BTK_SPINNER (spinner_sensitive));
  btk_spinner_start (BTK_SPINNER (spinner_unsensitive));
}

static void
on_stop_clicked (BtkButton *button, bpointer user_data)
{
  btk_spinner_stop (BTK_SPINNER (spinner_sensitive));
  btk_spinner_stop (BTK_SPINNER (spinner_unsensitive));
}

BtkWidget *
do_spinner (BtkWidget *do_widget)
{
  BtkWidget *vbox;
  BtkWidget *hbox;
  BtkWidget *button;
  BtkWidget *spinner;

  if (!window)
  {
    window = btk_dialog_new_with_buttons ("BtkSpinner",
                                          BTK_WINDOW (do_widget),
                                          0,
                                          BTK_STOCK_CLOSE,
                                          BTK_RESPONSE_NONE,
                                          NULL);
    btk_window_set_resizable (BTK_WINDOW (window), FALSE);

    g_signal_connect (window, "response",
                      G_CALLBACK (btk_widget_destroy), NULL);
    g_signal_connect (window, "destroy",
                      G_CALLBACK (btk_widget_destroyed), &window);

    vbox = btk_vbox_new (FALSE, 5);
    btk_box_pack_start (BTK_BOX (btk_dialog_get_content_area (BTK_DIALOG (window))), vbox, TRUE, TRUE, 0);
    btk_container_set_border_width (BTK_CONTAINER (vbox), 5);

    /* Sensitive */
    hbox = btk_hbox_new (FALSE, 5);
    spinner = btk_spinner_new ();
    btk_container_add (BTK_CONTAINER (hbox), spinner);
    btk_container_add (BTK_CONTAINER (hbox), btk_entry_new ());
    btk_container_add (BTK_CONTAINER (vbox), hbox);
    spinner_sensitive = spinner;

    /* Disabled */
    hbox = btk_hbox_new (FALSE, 5);
    spinner = btk_spinner_new ();
    btk_container_add (BTK_CONTAINER (hbox), spinner);
    btk_container_add (BTK_CONTAINER (hbox), btk_entry_new ());
    btk_container_add (BTK_CONTAINER (vbox), hbox);
    spinner_unsensitive = spinner;
    btk_widget_set_sensitive (hbox, FALSE);

    button = btk_button_new_from_stock (BTK_STOCK_MEDIA_PLAY);
    g_signal_connect (B_OBJECT (button), "clicked",
                      G_CALLBACK (on_play_clicked), spinner);
    btk_container_add (BTK_CONTAINER (vbox), button);

    button = btk_button_new_from_stock (BTK_STOCK_MEDIA_STOP);
    g_signal_connect (B_OBJECT (button), "clicked",
                      G_CALLBACK (on_stop_clicked), spinner);
    btk_container_add (BTK_CONTAINER (vbox), button);

    /* Start by default to test for:
     * https://bugzilla.bunny.org/show_bug.cgi?id=598496 */
    on_play_clicked (NULL, NULL);
  }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);

  return window;
}


