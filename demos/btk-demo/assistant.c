/* Assistant
 *
 * Demonstrates a sample multistep assistant. Assistants are used to divide
 * an operation into several simpler sequential steps, and to guide the user
 * through these steps.
 */

#include <btk/btk.h>
#include "demo-common.h"

static BtkWidget *assistant = NULL;
static BtkWidget *progress_bar = NULL;

static bboolean
apply_changes_gradually (bpointer data)
{
  bdouble fraction;

  /* Work, work, work... */
  fraction = btk_progress_bar_get_fraction (BTK_PROGRESS_BAR (progress_bar));
  fraction += 0.05;

  if (fraction < 1.0)
    {
      btk_progress_bar_set_fraction (BTK_PROGRESS_BAR (progress_bar), fraction);
      return TRUE;
    }
  else
    {
      /* Close automatically once changes are fully applied. */
      btk_widget_destroy (assistant);
      return FALSE;
    }
}

static void
on_assistant_apply (BtkWidget *widget, bpointer data)
{
  /* Start a timer to simulate changes taking a few seconds to apply. */
  g_timeout_add (100, apply_changes_gradually, NULL);
}

static void
on_assistant_close_cancel (BtkWidget *widget, bpointer data)
{
  BtkWidget **assistant = (BtkWidget **) data;

  btk_widget_destroy (*assistant);
  *assistant = NULL;
}

static void
on_assistant_prepare (BtkWidget *widget, BtkWidget *page, bpointer data)
{
  bint current_page, n_pages;
  bchar *title;

  current_page = btk_assistant_get_current_page (BTK_ASSISTANT (widget));
  n_pages = btk_assistant_get_n_pages (BTK_ASSISTANT (widget));

  title = g_strdup_printf ("Sample assistant (%d of %d)", current_page + 1, n_pages);
  btk_window_set_title (BTK_WINDOW (widget), title);
  g_free (title);

  /* The fourth page (counting from zero) is the progress page.  The
  * user clicked Apply to get here so we tell the assistant to commit,
  * which means the changes up to this point are permanent and cannot
  * be cancelled or revisited. */
  if (current_page == 3)
      btk_assistant_commit (BTK_ASSISTANT (widget));
}

static void
on_entry_changed (BtkWidget *widget, bpointer data)
{
  BtkAssistant *assistant = BTK_ASSISTANT (data);
  BtkWidget *current_page;
  bint page_number;
  const bchar *text;

  page_number = btk_assistant_get_current_page (assistant);
  current_page = btk_assistant_get_nth_page (assistant, page_number);
  text = btk_entry_get_text (BTK_ENTRY (widget));

  if (text && *text)
    btk_assistant_set_page_complete (assistant, current_page, TRUE);
  else
    btk_assistant_set_page_complete (assistant, current_page, FALSE);
}

static void
create_page1 (BtkWidget *assistant)
{
  BtkWidget *box, *label, *entry;
  BdkPixbuf *pixbuf;

  box = btk_hbox_new (FALSE, 12);
  btk_container_set_border_width (BTK_CONTAINER (box), 12);

  label = btk_label_new ("You must fill out this entry to continue:");
  btk_box_pack_start (BTK_BOX (box), label, FALSE, FALSE, 0);

  entry = btk_entry_new ();
  btk_box_pack_start (BTK_BOX (box), entry, TRUE, TRUE, 0);
  g_signal_connect (B_OBJECT (entry), "changed",
		    G_CALLBACK (on_entry_changed), assistant);

  btk_widget_show_all (box);
  btk_assistant_append_page (BTK_ASSISTANT (assistant), box);
  btk_assistant_set_page_title (BTK_ASSISTANT (assistant), box, "Page 1");
  btk_assistant_set_page_type (BTK_ASSISTANT (assistant), box, BTK_ASSISTANT_PAGE_INTRO);

  pixbuf = btk_widget_render_icon (assistant, BTK_STOCK_DIALOG_INFO, BTK_ICON_SIZE_DIALOG, NULL);
  btk_assistant_set_page_header_image (BTK_ASSISTANT (assistant), box, pixbuf);
  g_object_unref (pixbuf);
}

static void
create_page2 (BtkWidget *assistant)
{
  BtkWidget *box, *checkbutton;
  BdkPixbuf *pixbuf;

  box = btk_vbox_new (12, FALSE);
  btk_container_set_border_width (BTK_CONTAINER (box), 12);

  checkbutton = btk_check_button_new_with_label ("This is optional data, you may continue "
						 "even if you do not check this");
  btk_box_pack_start (BTK_BOX (box), checkbutton, FALSE, FALSE, 0);

  btk_widget_show_all (box);
  btk_assistant_append_page (BTK_ASSISTANT (assistant), box);
  btk_assistant_set_page_complete (BTK_ASSISTANT (assistant), box, TRUE);
  btk_assistant_set_page_title (BTK_ASSISTANT (assistant), box, "Page 2");

  pixbuf = btk_widget_render_icon (assistant, BTK_STOCK_DIALOG_INFO, BTK_ICON_SIZE_DIALOG, NULL);
  btk_assistant_set_page_header_image (BTK_ASSISTANT (assistant), box, pixbuf);
  g_object_unref (pixbuf);
}

static void
create_page3 (BtkWidget *assistant)
{
  BtkWidget *label;
  BdkPixbuf *pixbuf;

  label = btk_label_new ("This is a confirmation page, press 'Apply' to apply changes");

  btk_widget_show (label);
  btk_assistant_append_page (BTK_ASSISTANT (assistant), label);
  btk_assistant_set_page_type (BTK_ASSISTANT (assistant), label, BTK_ASSISTANT_PAGE_CONFIRM);
  btk_assistant_set_page_complete (BTK_ASSISTANT (assistant), label, TRUE);
  btk_assistant_set_page_title (BTK_ASSISTANT (assistant), label, "Confirmation");

  pixbuf = btk_widget_render_icon (assistant, BTK_STOCK_DIALOG_INFO, BTK_ICON_SIZE_DIALOG, NULL);
  btk_assistant_set_page_header_image (BTK_ASSISTANT (assistant), label, pixbuf);
  g_object_unref (pixbuf);
}

static void
create_page4 (BtkWidget *assistant)
{
  BtkWidget *page;

  page = btk_alignment_new (0.5, 0.5, 0.5, 0.0);

  progress_bar = btk_progress_bar_new ();
  btk_container_add (BTK_CONTAINER (page), progress_bar);

  btk_widget_show_all (page);
  btk_assistant_append_page (BTK_ASSISTANT (assistant), page);
  btk_assistant_set_page_type (BTK_ASSISTANT (assistant), page, BTK_ASSISTANT_PAGE_PROGRESS);
  btk_assistant_set_page_title (BTK_ASSISTANT (assistant), page, "Applying changes");

  /* This prevents the assistant window from being
   * closed while we're "busy" applying changes. */
  btk_assistant_set_page_complete (BTK_ASSISTANT (assistant), page, FALSE);
}

BtkWidget*
do_assistant (BtkWidget *do_widget)
{
  if (!assistant)
    {
      assistant = btk_assistant_new ();

	 btk_window_set_default_size (BTK_WINDOW (assistant), -1, 300);

      btk_window_set_screen (BTK_WINDOW (assistant),
			     btk_widget_get_screen (do_widget));

      create_page1 (assistant);
      create_page2 (assistant);
      create_page3 (assistant);
      create_page4 (assistant);

      g_signal_connect (B_OBJECT (assistant), "cancel",
			G_CALLBACK (on_assistant_close_cancel), &assistant);
      g_signal_connect (B_OBJECT (assistant), "close",
			G_CALLBACK (on_assistant_close_cancel), &assistant);
      g_signal_connect (B_OBJECT (assistant), "apply",
			G_CALLBACK (on_assistant_apply), NULL);
      g_signal_connect (B_OBJECT (assistant), "prepare",
			G_CALLBACK (on_assistant_prepare), NULL);
    }

  if (!btk_widget_get_visible (assistant))
    btk_widget_show (assistant);
  else
    {
      btk_widget_destroy (assistant);
      assistant = NULL;
    }

  return assistant;
}
