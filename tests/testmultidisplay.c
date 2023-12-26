/* testmultidisplay.c
 * Copyright (C) 2001 Sun Microsystems Inc.
 * Author: Erwann Chenede <erwann.chenede@sun.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <btk/btk.h>

gchar *screen2_name = NULL;

typedef struct
{
  BtkEntry *e1;
  BtkEntry *e2;
}
MyDoubleBtkEntry;

void
get_screen_response (BtkDialog *dialog,
		     gint       response_id,
		     BtkEntry  *entry)
{
  if (response_id == BTK_RESPONSE_DELETE_EVENT)
    return;
  g_free (screen2_name);
  screen2_name = g_strdup (btk_entry_get_text (entry));
}

void
entry_dialog_response (BtkDialog        *dialog,
		       gint              response_id,
		       MyDoubleBtkEntry *de)
{
  if (response_id == BTK_RESPONSE_APPLY)
    btk_entry_set_text (de->e2, btk_entry_get_text (de->e1));
  else
    btk_main_quit ();
}

void
make_selection_dialog (BdkScreen * screen,
		       BtkWidget * entry,
		       BtkWidget * other_entry)
{
  BtkWidget *window, *vbox;
  MyDoubleBtkEntry *double_entry = g_new (MyDoubleBtkEntry, 1);
  double_entry->e1 = BTK_ENTRY (entry);
  double_entry->e2 = BTK_ENTRY (other_entry);

  if (!screen)
    screen = bdk_screen_get_default ();

  window = g_object_new (BTK_TYPE_DIALOG,
			   "screen", screen,
			   "user_data", NULL,
			   "type", BTK_WINDOW_TOPLEVEL,
			   "title", "MultiDisplay Cut & Paste",
			   "border_width", 10, NULL);
  g_signal_connect (window, "destroy",
		    G_CALLBACK (btk_main_quit), NULL);


  vbox = g_object_new (BTK_TYPE_VBOX,
			 "border_width", 5,
			 NULL);
  btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), vbox, FALSE, FALSE, 0);

  btk_box_pack_start (BTK_BOX (vbox), entry, FALSE, FALSE, 0);
  btk_widget_grab_focus (entry);

  btk_dialog_add_buttons (BTK_DIALOG (window),
			  BTK_STOCK_APPLY, BTK_RESPONSE_APPLY,
			  BTK_STOCK_QUIT, BTK_RESPONSE_DELETE_EVENT,
			  NULL);
  btk_dialog_set_default_response (BTK_DIALOG (window), BTK_RESPONSE_APPLY);

  g_signal_connect (window, "response",
		    G_CALLBACK (entry_dialog_response), double_entry);

  btk_widget_show_all (window);
}

int
main (int argc, char *argv[])
{
  BtkWidget *dialog, *display_entry, *dialog_label;
  BtkWidget *entry, *entry2;
  BdkDisplay *dpy2;
  BdkScreen *scr2 = NULL;	/* Quiet GCC */
  gboolean correct_second_display = FALSE;

  btk_init (&argc, &argv);

  if (argc == 2)
    screen2_name = g_strdup (argv[1]);
  
  /* Get the second display information */

  dialog = btk_dialog_new_with_buttons ("Second Display Selection",
					NULL,
					BTK_DIALOG_MODAL,
					BTK_STOCK_OK,
					BTK_RESPONSE_OK, NULL);

  btk_dialog_set_default_response (BTK_DIALOG (dialog), BTK_RESPONSE_OK);
  display_entry = btk_entry_new ();
  btk_entry_set_activates_default (BTK_ENTRY (display_entry), TRUE);
  dialog_label =
    btk_label_new ("Please enter the name of\nthe second display\n");

  btk_container_add (BTK_CONTAINER (BTK_DIALOG (dialog)->vbox), dialog_label);
  btk_container_add (BTK_CONTAINER (BTK_DIALOG (dialog)->vbox),
		     display_entry);
  g_signal_connect (dialog, "response",
		    G_CALLBACK (get_screen_response), display_entry);

  btk_widget_grab_focus (display_entry);
  btk_widget_show_all (BTK_BIN (dialog)->child);
  
  while (!correct_second_display)
    {
      if (screen2_name)
	{
	  if (!g_ascii_strcasecmp (screen2_name, ""))
	    g_printerr ("No display name, reverting to default display\n");
	  
	  dpy2 = bdk_display_open (screen2_name);
	  if (dpy2)
	    {
	      scr2 = bdk_display_get_default_screen (dpy2);
	      correct_second_display = TRUE;
	    }
	  else
	    {
	      char *error_msg =
		g_strdup_printf  ("Can't open display :\n\t%s\nplease try another one\n",
				  screen2_name);
	      btk_label_set_text (BTK_LABEL (dialog_label), error_msg);
	      g_free (error_msg);
	    }
       }

      if (!correct_second_display)
	btk_dialog_run (BTK_DIALOG (dialog));
    }
  
  btk_widget_destroy (dialog);

  entry = g_object_new (BTK_TYPE_ENTRY,
			  "activates_default", TRUE,
			  "visible", TRUE,
			  NULL);
  entry2 = g_object_new (BTK_TYPE_ENTRY,
			   "activates_default", TRUE,
			   "visible", TRUE,
			   NULL);

  /* for default display */
  make_selection_dialog (NULL, entry2, entry);
  /* for selected display */
  make_selection_dialog (scr2, entry, entry2);
  btk_main ();

  return 0;
}
