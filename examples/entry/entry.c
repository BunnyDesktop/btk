
#include <stdio.h>
#include <stdlib.h>
#include <btk/btk.h>

static void enter_callback( BtkWidget *widget,
                            BtkWidget *entry )
{
  const gchar *entry_text;
  entry_text = btk_entry_get_text (BTK_ENTRY (entry));
  printf ("Entry contents: %s\n", entry_text);
}

static void entry_toggle_editable( BtkWidget *checkbutton,
                                   BtkWidget *entry )
{
  btk_editable_set_editable (BTK_EDITABLE (entry),
                             BTK_TOGGLE_BUTTON (checkbutton)->active);
}

static void entry_toggle_visibility( BtkWidget *checkbutton,
                                     BtkWidget *entry )
{
  btk_entry_set_visibility (BTK_ENTRY (entry),
			    BTK_TOGGLE_BUTTON (checkbutton)->active);
}

int main( int   argc,
          char *argv[] )
{

    BtkWidget *window;
    BtkWidget *vbox, *hbox;
    BtkWidget *entry;
    BtkWidget *button;
    BtkWidget *check;
    gint tmp_pos;

    btk_init (&argc, &argv);

    /* create a new window */
    window = btk_window_new (BTK_WINDOW_TOPLEVEL);
    btk_widget_set_size_request (BTK_WIDGET (window), 200, 100);
    btk_window_set_title (BTK_WINDOW (window), "BTK Entry");
    g_signal_connect (window, "destroy",
                      G_CALLBACK (btk_main_quit), NULL);
    g_signal_connect_swapped (window, "delete-event",
                              G_CALLBACK (btk_widget_destroy),
                              window);

    vbox = btk_vbox_new (FALSE, 0);
    btk_container_add (BTK_CONTAINER (window), vbox);
    btk_widget_show (vbox);

    entry = btk_entry_new ();
    btk_entry_set_max_length (BTK_ENTRY (entry), 50);
    g_signal_connect (entry, "activate",
		      G_CALLBACK (enter_callback),
		      (gpointer) entry);
    btk_entry_set_text (BTK_ENTRY (entry), "hello");
    tmp_pos = BTK_ENTRY (entry)->text_length;
    btk_editable_insert_text (BTK_EDITABLE (entry), " world", -1, &tmp_pos);
    btk_editable_select_rebunnyion (BTK_EDITABLE (entry),
			        0, BTK_ENTRY (entry)->text_length);
    btk_box_pack_start (BTK_BOX (vbox), entry, TRUE, TRUE, 0);
    btk_widget_show (entry);

    hbox = btk_hbox_new (FALSE, 0);
    btk_container_add (BTK_CONTAINER (vbox), hbox);
    btk_widget_show (hbox);

    check = btk_check_button_new_with_label ("Editable");
    btk_box_pack_start (BTK_BOX (hbox), check, TRUE, TRUE, 0);
    g_signal_connect (check, "toggled",
	              G_CALLBACK (entry_toggle_editable), (gpointer) entry);
    btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (check), TRUE);
    btk_widget_show (check);

    check = btk_check_button_new_with_label ("Visible");
    btk_box_pack_start (BTK_BOX (hbox), check, TRUE, TRUE, 0);
    g_signal_connect (check, "toggled",
	              G_CALLBACK (entry_toggle_visibility), (gpointer) entry);
    btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (check), TRUE);
    btk_widget_show (check);

    button = btk_button_new_from_stock (BTK_STOCK_CLOSE);
    g_signal_connect_swapped (button, "clicked",
			      G_CALLBACK (btk_widget_destroy),
			      window);
    btk_box_pack_start (BTK_BOX (vbox), button, TRUE, TRUE, 0);
    btk_widget_set_can_default (button, TRUE);
    btk_widget_grab_default (button);
    btk_widget_show (button);

    btk_widget_show (window);

    btk_main();

    return 0;
}
