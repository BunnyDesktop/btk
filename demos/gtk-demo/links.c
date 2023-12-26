/* Links
 *
 * BtkLabel can show hyperlinks. The default action is to call
 * btk_show_uri() on their URI, but it is possible to override
 * this with a custom handler.
 */

#include <btk/btk.h>

static void
response_cb (BtkWidget *dialog,
             gint       response_id,
             gpointer   data)
{
  btk_widget_destroy (dialog);
}

static gboolean
activate_link (BtkWidget   *label,
               const gchar *uri,
               gpointer     data)
{
  if (g_strcmp0 (uri, "keynav") == 0)
    {
      BtkWidget *dialog;
      BtkWidget *parent;

      parent = btk_widget_get_toplevel (label);
      dialog = btk_message_dialog_new_with_markup (BTK_WINDOW (parent),
                 BTK_DIALOG_DESTROY_WITH_PARENT,
                 BTK_MESSAGE_INFO,
                 BTK_BUTTONS_OK,
                 "The term <i>keynav</i> is a shorthand for "
                 "keyboard navigation and refers to the process of using "
                 "a program (exclusively) via keyboard input.");

      btk_window_present (BTK_WINDOW (dialog));
      g_signal_connect (dialog, "response", G_CALLBACK (response_cb), NULL);

      return TRUE;
    }

  return FALSE;
}

static BtkWidget *window = NULL;

BtkWidget *
do_links (BtkWidget *do_widget)
{
  BtkWidget *label;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (do_widget));
      btk_window_set_title (BTK_WINDOW (window), "Links");
      btk_container_set_border_width (BTK_CONTAINER (window), 12);
      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed), &window);

      label = btk_label_new ("Some <a href=\"http://en.wikipedia.org/wiki/Text\""
                             "title=\"plain text\">text</a> may be marked up\n"
                             "as hyperlinks, which can be clicked\n"
                             "or activated via <a href=\"keynav\">keynav</a>");
      btk_label_set_use_markup (BTK_LABEL (label), TRUE);
      g_signal_connect (label, "activate-link", G_CALLBACK (activate_link), NULL);
      btk_container_add (BTK_CONTAINER (window), label);
      btk_widget_show (label);
    }

  if (!btk_widget_get_visible (window))
    {
      btk_widget_show (window);
    }
  else
    {
      btk_widget_destroy (window);
      window = NULL;
    }

  return window;
}
