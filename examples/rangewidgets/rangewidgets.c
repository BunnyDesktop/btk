
#include <btk/btk.h>

BtkWidget *hscale, *vscale;

static void cb_pos_menu_select( BtkWidget       *item,
                                BtkPositionType  pos )
{
    /* Set the value position on both scale widgets */
    btk_scale_set_value_pos (BTK_SCALE (hscale), pos);
    btk_scale_set_value_pos (BTK_SCALE (vscale), pos);
}

static void cb_update_menu_select( BtkWidget     *item,
                                   BtkUpdateType  policy )
{
    /* Set the update policy for both scale widgets */
    btk_range_set_update_policy (BTK_RANGE (hscale), policy);
    btk_range_set_update_policy (BTK_RANGE (vscale), policy);
}

static void cb_digits_scale( BtkAdjustment *adj )
{
    /* Set the number of decimal places to which adj->value is rounded */
    btk_scale_set_digits (BTK_SCALE (hscale), (gint) adj->value);
    btk_scale_set_digits (BTK_SCALE (vscale), (gint) adj->value);
}

static void cb_page_size( BtkAdjustment *get,
                          BtkAdjustment *set )
{
    /* Set the page size and page increment size of the sample
     * adjustment to the value specified by the "Page Size" scale */
    set->page_size = get->value;
    set->page_increment = get->value;

    /* This sets the adjustment and makes it emit the "changed" signal to
       reconfigure all the widgets that are attached to this signal.  */
    btk_adjustment_set_value (set, CLAMP (set->value,
					  set->lower,
					  (set->upper - set->page_size)));
    g_signal_emit_by_name(G_OBJECT(set), "changed");
}

static void cb_draw_value( BtkToggleButton *button )
{
    /* Turn the value display on the scale widgets off or on depending
     *  on the state of the checkbutton */
    btk_scale_set_draw_value (BTK_SCALE (hscale), button->active);
    btk_scale_set_draw_value (BTK_SCALE (vscale), button->active);
}

/* Convenience functions */

static BtkWidget *make_menu_item ( gchar     *name,
                                   GCallback  callback,
                                   gpointer   data )
{
    BtkWidget *item;

    item = btk_menu_item_new_with_label (name);
    g_signal_connect (item, "activate",
	              callback, (gpointer) data);
    btk_widget_show (item);

    return item;
}

static void scale_set_default_values( BtkScale *scale )
{
    btk_range_set_update_policy (BTK_RANGE (scale),
                                 BTK_UPDATE_CONTINUOUS);
    btk_scale_set_digits (scale, 1);
    btk_scale_set_value_pos (scale, BTK_POS_TOP);
    btk_scale_set_draw_value (scale, TRUE);
}

/* makes the sample window */

static void create_range_controls( void )
{
    BtkWidget *window;
    BtkWidget *box1, *box2, *box3;
    BtkWidget *button;
    BtkWidget *scrollbar;
    BtkWidget *separator;
    BtkWidget *opt, *menu, *item;
    BtkWidget *label;
    BtkWidget *scale;
    BtkObject *adj1, *adj2;

    /* Standard window-creating stuff */
    window = btk_window_new (BTK_WINDOW_TOPLEVEL);
    g_signal_connect (window, "destroy",
                      G_CALLBACK (btk_main_quit),
                      NULL);
    btk_window_set_title (BTK_WINDOW (window), "range controls");

    box1 = btk_vbox_new (FALSE, 0);
    btk_container_add (BTK_CONTAINER (window), box1);
    btk_widget_show (box1);

    box2 = btk_hbox_new (FALSE, 10);
    btk_container_set_border_width (BTK_CONTAINER (box2), 10);
    btk_box_pack_start (BTK_BOX (box1), box2, TRUE, TRUE, 0);
    btk_widget_show (box2);

    /* value, lower, upper, step_increment, page_increment, page_size */
    /* Note that the page_size value only makes a difference for
     * scrollbar widgets, and the highest value you'll get is actually
     * (upper - page_size). */
    adj1 = btk_adjustment_new (0.0, 0.0, 101.0, 0.1, 1.0, 1.0);

    vscale = btk_vscale_new (BTK_ADJUSTMENT (adj1));
    scale_set_default_values (BTK_SCALE (vscale));
    btk_box_pack_start (BTK_BOX (box2), vscale, TRUE, TRUE, 0);
    btk_widget_show (vscale);

    box3 = btk_vbox_new (FALSE, 10);
    btk_box_pack_start (BTK_BOX (box2), box3, TRUE, TRUE, 0);
    btk_widget_show (box3);

    /* Reuse the same adjustment */
    hscale = btk_hscale_new (BTK_ADJUSTMENT (adj1));
    btk_widget_set_size_request (BTK_WIDGET (hscale), 200, -1);
    scale_set_default_values (BTK_SCALE (hscale));
    btk_box_pack_start (BTK_BOX (box3), hscale, TRUE, TRUE, 0);
    btk_widget_show (hscale);

    /* Reuse the same adjustment again */
    scrollbar = btk_hscrollbar_new (BTK_ADJUSTMENT (adj1));
    /* Notice how this causes the scales to always be updated
     * continuously when the scrollbar is moved */
    btk_range_set_update_policy (BTK_RANGE (scrollbar),
                                 BTK_UPDATE_CONTINUOUS);
    btk_box_pack_start (BTK_BOX (box3), scrollbar, TRUE, TRUE, 0);
    btk_widget_show (scrollbar);

    box2 = btk_hbox_new (FALSE, 10);
    btk_container_set_border_width (BTK_CONTAINER (box2), 10);
    btk_box_pack_start (BTK_BOX (box1), box2, TRUE, TRUE, 0);
    btk_widget_show (box2);

    /* A checkbutton to control whether the value is displayed or not */
    button = btk_check_button_new_with_label("Display value on scale widgets");
    btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (button), TRUE);
    g_signal_connect (button, "toggled",
                      G_CALLBACK (cb_draw_value), NULL);
    btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
    btk_widget_show (button);

    box2 = btk_hbox_new (FALSE, 10);
    btk_container_set_border_width (BTK_CONTAINER (box2), 10);

    /* An option menu to change the position of the value */
    label = btk_label_new ("Scale Value Position:");
    btk_box_pack_start (BTK_BOX (box2), label, FALSE, FALSE, 0);
    btk_widget_show (label);

    opt = btk_option_menu_new ();
    menu = btk_menu_new ();

    item = make_menu_item ("Top",
                           G_CALLBACK (cb_pos_menu_select),
                           GINT_TO_POINTER (BTK_POS_TOP));
    btk_menu_shell_append (BTK_MENU_SHELL (menu), item);

    item = make_menu_item ("Bottom", G_CALLBACK (cb_pos_menu_select),
                           GINT_TO_POINTER (BTK_POS_BOTTOM));
    btk_menu_shell_append (BTK_MENU_SHELL (menu), item);

    item = make_menu_item ("Left", G_CALLBACK (cb_pos_menu_select),
                           GINT_TO_POINTER (BTK_POS_LEFT));
    btk_menu_shell_append (BTK_MENU_SHELL (menu), item);

    item = make_menu_item ("Right", G_CALLBACK (cb_pos_menu_select),
                           GINT_TO_POINTER (BTK_POS_RIGHT));
    btk_menu_shell_append (BTK_MENU_SHELL (menu), item);

    btk_option_menu_set_menu (BTK_OPTION_MENU (opt), menu);
    btk_box_pack_start (BTK_BOX (box2), opt, TRUE, TRUE, 0);
    btk_widget_show (opt);

    btk_box_pack_start (BTK_BOX (box1), box2, TRUE, TRUE, 0);
    btk_widget_show (box2);

    box2 = btk_hbox_new (FALSE, 10);
    btk_container_set_border_width (BTK_CONTAINER (box2), 10);

    /* Yet another option menu, this time for the update policy of the
     * scale widgets */
    label = btk_label_new ("Scale Update Policy:");
    btk_box_pack_start (BTK_BOX (box2), label, FALSE, FALSE, 0);
    btk_widget_show (label);

    opt = btk_option_menu_new ();
    menu = btk_menu_new ();

    item = make_menu_item ("Continuous",
                           G_CALLBACK (cb_update_menu_select),
                           GINT_TO_POINTER (BTK_UPDATE_CONTINUOUS));
    btk_menu_shell_append (BTK_MENU_SHELL (menu), item);

    item = make_menu_item ("Discontinuous",
                           G_CALLBACK (cb_update_menu_select),
                           GINT_TO_POINTER (BTK_UPDATE_DISCONTINUOUS));
    btk_menu_shell_append (BTK_MENU_SHELL (menu), item);

    item = make_menu_item ("Delayed",
                           G_CALLBACK (cb_update_menu_select),
                           GINT_TO_POINTER (BTK_UPDATE_DELAYED));
    btk_menu_shell_append (BTK_MENU_SHELL (menu), item);

    btk_option_menu_set_menu (BTK_OPTION_MENU (opt), menu);
    btk_box_pack_start (BTK_BOX (box2), opt, TRUE, TRUE, 0);
    btk_widget_show (opt);

    btk_box_pack_start (BTK_BOX (box1), box2, TRUE, TRUE, 0);
    btk_widget_show (box2);

    box2 = btk_hbox_new (FALSE, 10);
    btk_container_set_border_width (BTK_CONTAINER (box2), 10);

    /* An HScale widget for adjusting the number of digits on the
     * sample scales. */
    label = btk_label_new ("Scale Digits:");
    btk_box_pack_start (BTK_BOX (box2), label, FALSE, FALSE, 0);
    btk_widget_show (label);

    adj2 = btk_adjustment_new (1.0, 0.0, 5.0, 1.0, 1.0, 0.0);
    g_signal_connect (adj2, "value_changed",
                      G_CALLBACK (cb_digits_scale), NULL);
    scale = btk_hscale_new (BTK_ADJUSTMENT (adj2));
    btk_scale_set_digits (BTK_SCALE (scale), 0);
    btk_box_pack_start (BTK_BOX (box2), scale, TRUE, TRUE, 0);
    btk_widget_show (scale);

    btk_box_pack_start (BTK_BOX (box1), box2, TRUE, TRUE, 0);
    btk_widget_show (box2);

    box2 = btk_hbox_new (FALSE, 10);
    btk_container_set_border_width (BTK_CONTAINER (box2), 10);

    /* And, one last HScale widget for adjusting the page size of the
     * scrollbar. */
    label = btk_label_new ("Scrollbar Page Size:");
    btk_box_pack_start (BTK_BOX (box2), label, FALSE, FALSE, 0);
    btk_widget_show (label);

    adj2 = btk_adjustment_new (1.0, 1.0, 101.0, 1.0, 1.0, 0.0);
    g_signal_connect (adj2, "value-changed",
                      G_CALLBACK (cb_page_size), (gpointer) adj1);
    scale = btk_hscale_new (BTK_ADJUSTMENT (adj2));
    btk_scale_set_digits (BTK_SCALE (scale), 0);
    btk_box_pack_start (BTK_BOX (box2), scale, TRUE, TRUE, 0);
    btk_widget_show (scale);

    btk_box_pack_start (BTK_BOX (box1), box2, TRUE, TRUE, 0);
    btk_widget_show (box2);

    separator = btk_hseparator_new ();
    btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);
    btk_widget_show (separator);

    box2 = btk_vbox_new (FALSE, 10);
    btk_container_set_border_width (BTK_CONTAINER (box2), 10);
    btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);
    btk_widget_show (box2);

    button = btk_button_new_with_label ("Quit");
    g_signal_connect_swapped (button, "clicked",
                              G_CALLBACK (btk_main_quit),
                              NULL);
    btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
    btk_widget_set_can_default (button, TRUE);
    btk_widget_grab_default (button);
    btk_widget_show (button);

    btk_widget_show (window);
}

int main( int   argc,
          char *argv[] )
{
    btk_init (&argc, &argv);

    create_range_controls ();

    btk_main ();

    return 0;
}

