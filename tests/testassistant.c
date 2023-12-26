/* 
 * BTK - The GIMP Toolkit
 * Copyright (C) 1999  Red Hat, Inc.
 * Copyright (C) 2002  Anders Carlsson <andersca@gnu.org>
 * Copyright (C) 2003  Matthias Clasen <mclasen@redhat.com>
 * Copyright (C) 2005  Carlos Garnacho Parro <carlosg@gnome.org>
 *
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <btk/btk.h>

static BtkWidget*
get_test_page (const gchar *text)
{
  return btk_label_new (text);
}

typedef struct {
  BtkAssistant *assistant;
  BtkWidget    *page;
} PageData;

static void
complete_cb (BtkWidget *check, 
	     gpointer   data)
{
  PageData *pdata = data;
  gboolean complete;

  complete = btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (check));

  btk_assistant_set_page_complete (pdata->assistant,
				   pdata->page,
				   complete);
}
	     
static BtkWidget *
add_completion_test_page (BtkWidget   *assistant,
			  const gchar *text, 
			  gboolean     visible,
			  gboolean     complete)
{
  BtkWidget *page;
  BtkWidget *check;
  PageData *pdata;

  page = btk_vbox_new (0, FALSE);
  check = btk_check_button_new_with_label ("Complete");

  btk_container_add (BTK_CONTAINER (page), btk_label_new (text));
  btk_container_add (BTK_CONTAINER (page), check);
  
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (check), complete);

  pdata = g_new (PageData, 1);
  pdata->assistant = BTK_ASSISTANT (assistant);
  pdata->page = page;
  g_signal_connect (G_OBJECT (check), "toggled", 
		    G_CALLBACK (complete_cb), pdata);


  if (visible)
    btk_widget_show_all (page);

  btk_assistant_append_page (BTK_ASSISTANT (assistant), page);
  btk_assistant_set_page_title (BTK_ASSISTANT (assistant), page, text);
  btk_assistant_set_page_complete (BTK_ASSISTANT (assistant), page, complete);

  return page;
}

static void
cancel_callback (BtkWidget *widget)
{
  g_print ("cancel\n");

  btk_widget_hide (widget);
}

static void
close_callback (BtkWidget *widget)
{
  g_print ("close\n");

  btk_widget_hide (widget);
}

static void
apply_callback (BtkWidget *widget)
{
  g_print ("apply\n");
}

static gboolean
progress_timeout (BtkWidget *assistant)
{
  BtkWidget *page, *progress;
  gint current_page;
  gdouble value;

  current_page = btk_assistant_get_current_page (BTK_ASSISTANT (assistant));
  page = btk_assistant_get_nth_page (BTK_ASSISTANT (assistant), current_page);
  progress = BTK_BIN (page)->child;

  value  = btk_progress_bar_get_fraction (BTK_PROGRESS_BAR (progress));
  value += 0.1;
  btk_progress_bar_set_fraction (BTK_PROGRESS_BAR (progress), value);

  if (value >= 1.0)
    {
      btk_assistant_set_page_complete (BTK_ASSISTANT (assistant), page, TRUE);
      return FALSE;
    }

  return TRUE;
}

static void
prepare_callback (BtkWidget *widget, BtkWidget *page)
{
  if (BTK_IS_LABEL (page))
    g_print ("prepare: %s\n", btk_label_get_text (BTK_LABEL (page)));
  else if (btk_assistant_get_page_type (BTK_ASSISTANT (widget), page) == BTK_ASSISTANT_PAGE_PROGRESS)
    {
      BtkWidget *progress;

      progress = BTK_BIN (page)->child;
      btk_assistant_set_page_complete (BTK_ASSISTANT (widget), page, FALSE);
      btk_progress_bar_set_fraction (BTK_PROGRESS_BAR (progress), 0.0);
      bdk_threads_add_timeout (300, (GSourceFunc) progress_timeout, widget);
    }
  else
    g_print ("prepare: %d\n", btk_assistant_get_current_page (BTK_ASSISTANT (widget)));
}

static void
create_simple_assistant (BtkWidget *widget)
{
  static BtkWidget *assistant = NULL;

  if (!assistant)
    {
      BtkWidget *page;

      assistant = btk_assistant_new ();
      btk_window_set_default_size (BTK_WINDOW (assistant), 400, 300);

      g_signal_connect (G_OBJECT (assistant), "cancel",
			G_CALLBACK (cancel_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "close",
			G_CALLBACK (close_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "apply",
			G_CALLBACK (apply_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "prepare",
			G_CALLBACK (prepare_callback), NULL);

      page = get_test_page ("Page 1");
      btk_widget_show (page);
      btk_assistant_append_page (BTK_ASSISTANT (assistant), page);
      btk_assistant_set_page_title (BTK_ASSISTANT (assistant), page, "Page 1");
      btk_assistant_set_page_complete (BTK_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("Page 2");
      btk_widget_show (page);
      btk_assistant_append_page (BTK_ASSISTANT (assistant), page);
      btk_assistant_set_page_title (BTK_ASSISTANT (assistant), page, "Page 2");
      btk_assistant_set_page_type  (BTK_ASSISTANT (assistant), page, BTK_ASSISTANT_PAGE_CONFIRM);
      btk_assistant_set_page_complete (BTK_ASSISTANT (assistant), page, TRUE);
    }

  if (!btk_widget_get_visible (assistant))
    btk_widget_show (assistant);
  else
    {
      btk_widget_destroy (assistant);
      assistant = NULL;
    }
}

static void
visible_cb (BtkWidget *check, 
	    gpointer   data)
{
  BtkWidget *page = data;
  gboolean visible;

  visible = btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (check));

  g_object_set (G_OBJECT (page), "visible", visible, NULL);
}

static void
create_generous_assistant (BtkWidget *widget)
{
  static BtkWidget *assistant = NULL;

  if (!assistant)
    {
      BtkWidget *page, *next, *check;
      PageData  *pdata;

      assistant = btk_assistant_new ();
      btk_window_set_default_size (BTK_WINDOW (assistant), 400, 300);

      g_signal_connect (G_OBJECT (assistant), "cancel",
			G_CALLBACK (cancel_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "close",
			G_CALLBACK (close_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "apply",
			G_CALLBACK (apply_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "prepare",
			G_CALLBACK (prepare_callback), NULL);

      page = get_test_page ("Introduction");
      btk_widget_show (page);
      btk_assistant_append_page (BTK_ASSISTANT (assistant), page);
      btk_assistant_set_page_title (BTK_ASSISTANT (assistant), page, "Introduction");
      btk_assistant_set_page_type  (BTK_ASSISTANT (assistant), page, BTK_ASSISTANT_PAGE_INTRO);
      btk_assistant_set_page_complete (BTK_ASSISTANT (assistant), page, TRUE);

      page = add_completion_test_page (assistant, "Content", TRUE, FALSE);
      next = add_completion_test_page (assistant, "More Content", TRUE, TRUE);

      check = btk_check_button_new_with_label ("Next page visible");
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (check), TRUE);
      g_signal_connect (G_OBJECT (check), "toggled", 
			G_CALLBACK (visible_cb), next);
      btk_widget_show (check);
      btk_container_add (BTK_CONTAINER (page), check);
      
      add_completion_test_page (assistant, "Even More Content", TRUE, TRUE);

      page = get_test_page ("Confirmation");
      btk_widget_show (page);
      btk_assistant_append_page (BTK_ASSISTANT (assistant), page);
      btk_assistant_set_page_title (BTK_ASSISTANT (assistant), page, "Confirmation");
      btk_assistant_set_page_type  (BTK_ASSISTANT (assistant), page, BTK_ASSISTANT_PAGE_CONFIRM);
      btk_assistant_set_page_complete (BTK_ASSISTANT (assistant), page, TRUE);

      page = btk_alignment_new (0.5, 0.5, 0.9, 0.0);
      btk_container_add (BTK_CONTAINER (page), btk_progress_bar_new ());
      btk_widget_show_all (page);
      btk_assistant_append_page (BTK_ASSISTANT (assistant), page);
      btk_assistant_set_page_title (BTK_ASSISTANT (assistant), page, "Progress");
      btk_assistant_set_page_type  (BTK_ASSISTANT (assistant), page, BTK_ASSISTANT_PAGE_PROGRESS);

      page = btk_check_button_new_with_label ("Summary complete");
      btk_widget_show (page);
      btk_assistant_append_page (BTK_ASSISTANT (assistant), page);
      btk_assistant_set_page_title (BTK_ASSISTANT (assistant), page, "Summary");
      btk_assistant_set_page_type  (BTK_ASSISTANT (assistant), page, BTK_ASSISTANT_PAGE_SUMMARY);

      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (page),
                                    btk_assistant_get_page_complete (BTK_ASSISTANT (assistant),
                                                                     page));

      pdata = g_new (PageData, 1);
      pdata->assistant = BTK_ASSISTANT (assistant);
      pdata->page = page;
      g_signal_connect (page, "toggled",
                      G_CALLBACK (complete_cb), pdata);
    }

  if (!btk_widget_get_visible (assistant))
    btk_widget_show (assistant);
  else
    {
      btk_widget_destroy (assistant);
      assistant = NULL;
    }
}

static gchar selected_branch = 'A';

static void
select_branch (BtkWidget *widget, gchar branch)
{
  selected_branch = branch;
}

static gint
nonlinear_assistant_forward_page (gint current_page, gpointer data)
{
  switch (current_page)
    {
    case 0:
      if (selected_branch == 'A')
	return 1;
      else
	return 2;
    case 1:
    case 2:
      return 3;
    default:
      return -1;
    }
}

static void
create_nonlinear_assistant (BtkWidget *widget)
{
  static BtkWidget *assistant = NULL;

  if (!assistant)
    {
      BtkWidget *page, *button;

      assistant = btk_assistant_new ();
      btk_window_set_default_size (BTK_WINDOW (assistant), 400, 300);

      g_signal_connect (G_OBJECT (assistant), "cancel",
			G_CALLBACK (cancel_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "close",
			G_CALLBACK (close_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "apply",
			G_CALLBACK (apply_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "prepare",
			G_CALLBACK (prepare_callback), NULL);

      btk_assistant_set_forward_page_func (BTK_ASSISTANT (assistant),
					   nonlinear_assistant_forward_page,
					   NULL, NULL);

      page = btk_vbox_new (FALSE, 6);

      button = btk_radio_button_new_with_label (NULL, "branch A");
      btk_box_pack_start (BTK_BOX (page), button, FALSE, FALSE, 0);
      g_signal_connect (G_OBJECT (button), "toggled", G_CALLBACK (select_branch), GINT_TO_POINTER ('A'));
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (button), TRUE);
      
      button = btk_radio_button_new_with_label (btk_radio_button_get_group (BTK_RADIO_BUTTON (button)),
						"branch B");
      btk_box_pack_start (BTK_BOX (page), button, FALSE, FALSE, 0);
      g_signal_connect (G_OBJECT (button), "toggled", G_CALLBACK (select_branch), GINT_TO_POINTER ('B'));

      btk_widget_show_all (page);
      btk_assistant_append_page (BTK_ASSISTANT (assistant), page);
      btk_assistant_set_page_title (BTK_ASSISTANT (assistant), page, "Page 1");
      btk_assistant_set_page_complete (BTK_ASSISTANT (assistant), page, TRUE);
      
      page = get_test_page ("Page 2A");
      btk_widget_show (page);
      btk_assistant_append_page (BTK_ASSISTANT (assistant), page);
      btk_assistant_set_page_title (BTK_ASSISTANT (assistant), page, "Page 2A");
      btk_assistant_set_page_complete (BTK_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("Page 2B");
      btk_widget_show (page);
      btk_assistant_append_page (BTK_ASSISTANT (assistant), page);
      btk_assistant_set_page_title (BTK_ASSISTANT (assistant), page, "Page 2B");
      btk_assistant_set_page_complete (BTK_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("Confirmation");
      btk_widget_show (page);
      btk_assistant_append_page (BTK_ASSISTANT (assistant), page);
      btk_assistant_set_page_title (BTK_ASSISTANT (assistant), page, "Confirmation");
      btk_assistant_set_page_type  (BTK_ASSISTANT (assistant), page, BTK_ASSISTANT_PAGE_CONFIRM);
      btk_assistant_set_page_complete (BTK_ASSISTANT (assistant), page, TRUE);
    }

  if (!btk_widget_get_visible (assistant))
    btk_widget_show (assistant);
  else
    {
      btk_widget_destroy (assistant);
      assistant = NULL;
    }
}

static gint
looping_assistant_forward_page (gint current_page, gpointer data)
{
  switch (current_page)
    {
    case 0:
      return 1;
    case 1:
      return 2;
    case 2:
      return 3;
    case 3:
      {
	BtkAssistant *assistant;
	BtkWidget *page;

	assistant = (BtkAssistant*) data;
	page = btk_assistant_get_nth_page (assistant, current_page);

	if (btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (page)))
	  return 0;
	else
	  return 4;
      }
    case 4:
    default:
      return -1;
    }
}

static void
create_looping_assistant (BtkWidget *widget)
{
  static BtkWidget *assistant = NULL;

  if (!assistant)
    {
      BtkWidget *page;

      assistant = btk_assistant_new ();
      btk_window_set_default_size (BTK_WINDOW (assistant), 400, 300);

      g_signal_connect (G_OBJECT (assistant), "cancel",
			G_CALLBACK (cancel_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "close",
			G_CALLBACK (close_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "apply",
			G_CALLBACK (apply_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "prepare",
			G_CALLBACK (prepare_callback), NULL);

      btk_assistant_set_forward_page_func (BTK_ASSISTANT (assistant),
					   looping_assistant_forward_page,
					   assistant, NULL);

      page = get_test_page ("Introduction");
      btk_widget_show (page);
      btk_assistant_append_page (BTK_ASSISTANT (assistant), page);
      btk_assistant_set_page_title (BTK_ASSISTANT (assistant), page, "Introduction");
      btk_assistant_set_page_type  (BTK_ASSISTANT (assistant), page, BTK_ASSISTANT_PAGE_INTRO);
      btk_assistant_set_page_complete (BTK_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("Content");
      btk_widget_show (page);
      btk_assistant_append_page (BTK_ASSISTANT (assistant), page);
      btk_assistant_set_page_title (BTK_ASSISTANT (assistant), page, "Content");
      btk_assistant_set_page_complete (BTK_ASSISTANT (assistant), page, TRUE);

      page = get_test_page ("More content");
      btk_widget_show (page);
      btk_assistant_append_page (BTK_ASSISTANT (assistant), page);
      btk_assistant_set_page_title (BTK_ASSISTANT (assistant), page, "More content");
      btk_assistant_set_page_complete (BTK_ASSISTANT (assistant), page, TRUE);

      page = btk_check_button_new_with_label ("Loop?");
      btk_widget_show (page);
      btk_assistant_append_page (BTK_ASSISTANT (assistant), page);
      btk_assistant_set_page_title (BTK_ASSISTANT (assistant), page, "Loop?");
      btk_assistant_set_page_complete (BTK_ASSISTANT (assistant), page, TRUE);
      
      page = get_test_page ("Confirmation");
      btk_widget_show (page);
      btk_assistant_append_page (BTK_ASSISTANT (assistant), page);
      btk_assistant_set_page_title (BTK_ASSISTANT (assistant), page, "Confirmation");
      btk_assistant_set_page_type  (BTK_ASSISTANT (assistant), page, BTK_ASSISTANT_PAGE_CONFIRM);
      btk_assistant_set_page_complete (BTK_ASSISTANT (assistant), page, TRUE);
    }

  if (!btk_widget_get_visible (assistant))
    btk_widget_show (assistant);
  else
    {
      btk_widget_destroy (assistant);
      assistant = NULL;
    }
}

static void
create_full_featured_assistant (BtkWidget *widget)
{
  static BtkWidget *assistant = NULL;

  if (!assistant)
    {
      BtkWidget *page, *button;
	 BdkPixbuf *pixbuf;

      assistant = btk_assistant_new ();
      btk_window_set_default_size (BTK_WINDOW (assistant), 400, 300);

	 button = btk_button_new_from_stock (BTK_STOCK_STOP);
	 btk_widget_show (button);
	 btk_assistant_add_action_widget (BTK_ASSISTANT (assistant), button);

      g_signal_connect (G_OBJECT (assistant), "cancel",
			G_CALLBACK (cancel_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "close",
			G_CALLBACK (close_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "apply",
			G_CALLBACK (apply_callback), NULL);
      g_signal_connect (G_OBJECT (assistant), "prepare",
			G_CALLBACK (prepare_callback), NULL);

      page = get_test_page ("Page 1");
      btk_widget_show (page);
      btk_assistant_append_page (BTK_ASSISTANT (assistant), page);
      btk_assistant_set_page_title (BTK_ASSISTANT (assistant), page, "Page 1");
      btk_assistant_set_page_complete (BTK_ASSISTANT (assistant), page, TRUE);

	 /* set a side image */
	 pixbuf = btk_widget_render_icon (page, BTK_STOCK_DIALOG_WARNING, BTK_ICON_SIZE_DIALOG, NULL);
	 btk_assistant_set_page_side_image (BTK_ASSISTANT (assistant), page, pixbuf);

	 /* set a header image */
	 pixbuf = btk_widget_render_icon (page, BTK_STOCK_DIALOG_INFO, BTK_ICON_SIZE_DIALOG, NULL);
	 btk_assistant_set_page_header_image (BTK_ASSISTANT (assistant), page, pixbuf);

      page = get_test_page ("Invisible page");
      btk_assistant_append_page (BTK_ASSISTANT (assistant), page);

      page = get_test_page ("Page 3");
      btk_widget_show (page);
      btk_assistant_append_page (BTK_ASSISTANT (assistant), page);
      btk_assistant_set_page_title (BTK_ASSISTANT (assistant), page, "Page 3");
      btk_assistant_set_page_type  (BTK_ASSISTANT (assistant), page, BTK_ASSISTANT_PAGE_CONFIRM);
      btk_assistant_set_page_complete (BTK_ASSISTANT (assistant), page, TRUE);

	 /* set a header image */
	 pixbuf = btk_widget_render_icon (page, BTK_STOCK_DIALOG_INFO, BTK_ICON_SIZE_DIALOG, NULL);
	 btk_assistant_set_page_header_image (BTK_ASSISTANT (assistant), page, pixbuf);
    }

  if (!btk_widget_get_visible (assistant))
    btk_widget_show (assistant);
  else
    {
      btk_widget_destroy (assistant);
      assistant = NULL;
    }
}

struct {
  gchar *text;
  void  (*func) (BtkWidget *widget);
} buttons[] =
  {
    { "simple assistant",        create_simple_assistant },
    { "generous assistant",      create_generous_assistant },
    { "nonlinear assistant",     create_nonlinear_assistant },
    { "looping assistant",       create_looping_assistant },
    { "full featured assistant", create_full_featured_assistant },
  };

int
main (int argc, gchar *argv[])
{
  BtkWidget *window, *box, *button;
  gint i;

  btk_init (&argc, &argv);

  if (g_getenv ("RTL"))
    btk_widget_set_default_direction (BTK_TEXT_DIR_RTL);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);

  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (btk_main_quit), NULL);
  g_signal_connect (G_OBJECT (window), "delete-event",
		    G_CALLBACK (btk_false), NULL);

  box = btk_vbox_new (FALSE, 6);
  btk_container_add (BTK_CONTAINER (window), box);

  for (i = 0; i < G_N_ELEMENTS (buttons); i++)
    {
      button = btk_button_new_with_label (buttons[i].text);

      if (buttons[i].func)
	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (buttons[i].func), NULL);

      btk_box_pack_start (BTK_BOX (box), button, TRUE, TRUE, 0);
    }

  btk_widget_show_all (window);
  btk_main ();

  return 0;
}
