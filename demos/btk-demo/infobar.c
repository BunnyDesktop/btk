/* Info bar
 *
 * Info bar widgets are used to report important messages to the user.
 */

#include <btk/btk.h>

static BtkWidget *window = NULL;

static void
on_bar_response (BtkInfoBar *info_bar,
                 gint        response_id,
                 gpointer    user_data)
{
  BtkWidget *dialog;

  dialog = btk_message_dialog_new (BTK_WINDOW (window),
				   BTK_DIALOG_MODAL | BTK_DIALOG_DESTROY_WITH_PARENT,
				   BTK_MESSAGE_INFO,
				   BTK_BUTTONS_OK,
				   "You clicked a button on an info bar");
  btk_message_dialog_format_secondary_text (BTK_MESSAGE_DIALOG (dialog),
                                            "Your response has id %d", response_id);
  btk_dialog_run (BTK_DIALOG (dialog));
  btk_widget_destroy (dialog);
}

BtkWidget *
do_infobar (BtkWidget *do_widget)
{
  BtkWidget *frame;
  BtkWidget *bar;
  BtkWidget *vbox;
  BtkWidget *vbox2;
  BtkWidget *label;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
                             btk_widget_get_screen (do_widget));
      btk_window_set_title (BTK_WINDOW (window), "Info Bars");

      g_signal_connect (window, "destroy", G_CALLBACK (btk_widget_destroyed), &window);
      btk_container_set_border_width (BTK_CONTAINER (window), 8);

      vbox = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), vbox);

      bar = btk_info_bar_new ();
      btk_box_pack_start (BTK_BOX (vbox), bar, FALSE, FALSE, 0);
      btk_info_bar_set_message_type (BTK_INFO_BAR (bar), BTK_MESSAGE_INFO);
      label = btk_label_new ("This is an info bar with message type BTK_MESSAGE_INFO");
      btk_box_pack_start (BTK_BOX (btk_info_bar_get_content_area (BTK_INFO_BAR (bar))), label, FALSE, FALSE, 0);

      bar = btk_info_bar_new ();
      btk_box_pack_start (BTK_BOX (vbox), bar, FALSE, FALSE, 0);
      btk_info_bar_set_message_type (BTK_INFO_BAR (bar), BTK_MESSAGE_WARNING);
      label = btk_label_new ("This is an info bar with message type BTK_MESSAGE_WARNING");
      btk_box_pack_start (BTK_BOX (btk_info_bar_get_content_area (BTK_INFO_BAR (bar))), label, FALSE, FALSE, 0);

      bar = btk_info_bar_new_with_buttons (BTK_STOCK_OK, BTK_RESPONSE_OK, NULL);
      g_signal_connect (bar, "response", G_CALLBACK (on_bar_response), window);
      btk_box_pack_start (BTK_BOX (vbox), bar, FALSE, FALSE, 0);
      btk_info_bar_set_message_type (BTK_INFO_BAR (bar), BTK_MESSAGE_QUESTION);
      label = btk_label_new ("This is an info bar with message type BTK_MESSAGE_QUESTION");
      btk_box_pack_start (BTK_BOX (btk_info_bar_get_content_area (BTK_INFO_BAR (bar))), label, FALSE, FALSE, 0);

      bar = btk_info_bar_new ();
      btk_box_pack_start (BTK_BOX (vbox), bar, FALSE, FALSE, 0);
      btk_info_bar_set_message_type (BTK_INFO_BAR (bar), BTK_MESSAGE_ERROR);
      label = btk_label_new ("This is an info bar with message type BTK_MESSAGE_ERROR");
      btk_box_pack_start (BTK_BOX (btk_info_bar_get_content_area (BTK_INFO_BAR (bar))), label, FALSE, FALSE, 0);

      bar = btk_info_bar_new ();
      btk_box_pack_start (BTK_BOX (vbox), bar, FALSE, FALSE, 0);
      btk_info_bar_set_message_type (BTK_INFO_BAR (bar), BTK_MESSAGE_OTHER);
      label = btk_label_new ("This is an info bar with message type BTK_MESSAGE_OTHER");
      btk_box_pack_start (BTK_BOX (btk_info_bar_get_content_area (BTK_INFO_BAR (bar))), label, FALSE, FALSE, 0);

      frame = btk_frame_new ("Info bars");
      btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 8);

      vbox2 = btk_vbox_new (FALSE, 8);
      btk_container_set_border_width (BTK_CONTAINER (vbox2), 8);
      btk_container_add (BTK_CONTAINER (frame), vbox2);

      /* Standard message dialog */
      label = btk_label_new ("An example of different info bars");
      btk_box_pack_start (BTK_BOX (vbox2), label, FALSE, FALSE, 0);
    }

  if (!btk_widget_get_visible (window))
    {
      btk_widget_show_all (window);
    }
  else
    {
      btk_widget_destroy (window);
      window = NULL;
    }

  return window;
}
