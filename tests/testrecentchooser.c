/* testrecentchooser.c
 * Copyright (C) 2006  Emmanuele Bassi.
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

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <btk/btk.h>

#ifdef G_OS_WIN32
# include <io.h>
# define localtime_r(t,b) *(b) = localtime (t)
# ifndef S_ISREG
#  define S_ISREG(m) ((m) & _S_IFREG)
# endif
#endif

#include "prop-editor.h"

static void
print_current_item (BtkRecentChooser *chooser)
{
  gchar *uri;

  uri = btk_recent_chooser_get_current_uri (chooser);
  g_print ("Current item changed :\n  %s\n", uri ? uri : "null");
  g_free (uri);
}

static void
print_selected (BtkRecentChooser *chooser)
{
  gsize uris_len, i;
  gchar **uris = btk_recent_chooser_get_uris (chooser, &uris_len);

  g_print ("Selection changed :\n");
  for (i = 0; i < uris_len; i++)
    g_print ("  %s\n", uris[i]);
  g_print ("\n");

  g_strfreev (uris);
}

static void
response_cb (BtkDialog *dialog,
	     gint       response_id)
{
  if (response_id == BTK_RESPONSE_OK)
    {
    }
  else
    g_print ("Dialog was closed\n");

  btk_main_quit ();
}

static void
filter_changed (BtkRecentChooserDialog *dialog,
		gpointer                data)
{
  g_print ("recent filter changed\n");
}

static void
notify_multiple_cb (BtkWidget  *dialog,
		    GParamSpec *pspec,
		    BtkWidget  *button)
{
  gboolean multiple;

  multiple = btk_recent_chooser_get_select_multiple (BTK_RECENT_CHOOSER (dialog));

  btk_widget_set_sensitive (button, multiple);
}

static void
kill_dependent (BtkWindow *win,
		BtkObject *dep)
{
  btk_object_destroy (dep);
  g_object_unref (dep);
}

int
main (int   argc,
      char *argv[])
{
  BtkWidget *control_window;
  BtkWidget *vbbox;
  BtkWidget *button;
  BtkWidget *dialog;
  BtkWidget *prop_editor;
  BtkRecentFilter *filter;
  gint i;
  gboolean multiple = FALSE;
  
  btk_init (&argc, &argv);

  /* to test rtl layout, set RTL=1 in the environment */
  if (g_getenv ("RTL"))
    btk_widget_set_default_direction (BTK_TEXT_DIR_RTL);

  for (i = 1; i < argc; i++)
    {
      if (!strcmp ("--multiple", argv[i]))
	multiple = TRUE;
    }

  dialog = g_object_new (BTK_TYPE_RECENT_CHOOSER_DIALOG,
		         "select-multiple", multiple,
                         "show-tips", TRUE,
                         "show-icons", TRUE,
			 NULL);
  btk_window_set_title (BTK_WINDOW (dialog), "Select a file");
  btk_dialog_add_buttons (BTK_DIALOG (dialog),
		  	  BTK_STOCK_CANCEL, BTK_RESPONSE_CANCEL,
			  BTK_STOCK_OPEN, BTK_RESPONSE_OK,
			  NULL);
  
  btk_dialog_set_default_response (BTK_DIALOG (dialog), BTK_RESPONSE_OK);

  g_signal_connect (dialog, "item-activated",
		    G_CALLBACK (print_current_item), NULL);
  g_signal_connect (dialog, "selection-changed",
		    G_CALLBACK (print_selected), NULL);
  g_signal_connect (dialog, "response",
		    G_CALLBACK (response_cb), NULL);
  
  /* filters */
  filter = btk_recent_filter_new ();
  btk_recent_filter_set_name (filter, "All Files");
  btk_recent_filter_add_pattern (filter, "*");
  btk_recent_chooser_add_filter (BTK_RECENT_CHOOSER (dialog), filter);

  filter = btk_recent_filter_new ();
  btk_recent_filter_set_name (filter, "Only PDF Files");
  btk_recent_filter_add_mime_type (filter, "application/pdf");
  btk_recent_chooser_add_filter (BTK_RECENT_CHOOSER (dialog), filter);

  g_signal_connect (dialog, "notify::filter",
		    G_CALLBACK (filter_changed), NULL);

  btk_recent_chooser_set_filter (BTK_RECENT_CHOOSER (dialog), filter);

  filter = btk_recent_filter_new ();
  btk_recent_filter_set_name (filter, "PNG and JPEG");
  btk_recent_filter_add_mime_type (filter, "image/png");
  btk_recent_filter_add_mime_type (filter, "image/jpeg");
  btk_recent_chooser_add_filter (BTK_RECENT_CHOOSER (dialog), filter);

  btk_widget_show_all (dialog);

  prop_editor = create_prop_editor (G_OBJECT (dialog), BTK_TYPE_RECENT_CHOOSER);

  control_window = btk_window_new (BTK_WINDOW_TOPLEVEL);

  vbbox = btk_vbutton_box_new ();
  btk_container_add (BTK_CONTAINER (control_window), vbbox);

  button = btk_button_new_with_mnemonic ("_Select all");
  btk_widget_set_sensitive (button, multiple);
  btk_container_add (BTK_CONTAINER (vbbox), button);
  g_signal_connect_swapped (button, "clicked",
		            G_CALLBACK (btk_recent_chooser_select_all), dialog);
  g_signal_connect (dialog, "notify::select-multiple",
		    G_CALLBACK (notify_multiple_cb), button);

  button = btk_button_new_with_mnemonic ("_Unselect all");
  btk_container_add (BTK_CONTAINER (vbbox), button);
  g_signal_connect_swapped (button, "clicked",
		            G_CALLBACK (btk_recent_chooser_unselect_all), dialog);

  btk_widget_show_all (control_window);
  
  g_object_ref (control_window);
  g_signal_connect (dialog, "destroy",
		    G_CALLBACK (kill_dependent), control_window);
  
  g_object_ref (dialog);
  btk_main ();
  btk_widget_destroy (dialog);
  g_object_unref (dialog);

  return 0;
}
