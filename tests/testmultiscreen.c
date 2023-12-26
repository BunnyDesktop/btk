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

static BtkWidget **images;
static BtkWidget **vbox;

static void
hello (BtkWidget * button, char *label)
{
  g_print ("Click from %s\n", label);
}

static void
show_hide (BtkWidget * button, gpointer data)
{
  gint num_screen = GPOINTER_TO_INT (data);
    
  static gboolean visible = TRUE;
  if (visible)
    {
      btk_widget_hide (images[num_screen]);
      btk_button_set_label (BTK_BUTTON (button), "Show Icon");
      visible = FALSE;
    }
  else
    {
      btk_widget_show (images[num_screen]);
      btk_button_set_label (BTK_BUTTON (button), "Hide Icon");
      visible = TRUE;
    }
}

static void
move (BtkWidget *button, BtkVBox *vbox)
{
  BdkScreen *screen = btk_widget_get_screen (button);
  BtkWidget *toplevel = btk_widget_get_toplevel (button);
  BtkWidget *new_toplevel;  
  BdkDisplay *display = bdk_screen_get_display (screen);
  gint number_of_screens = bdk_display_get_n_screens (display);
  gint screen_num = bdk_screen_get_number (screen);
  
  g_print ("This button is on screen %d\n", bdk_screen_get_number (screen));
  
  new_toplevel = btk_window_new (BTK_WINDOW_TOPLEVEL);
  
  if ((screen_num +1) < number_of_screens)
    btk_window_set_screen (BTK_WINDOW (new_toplevel), 
			   bdk_display_get_screen (display,
						   screen_num + 1));
  else
    btk_window_set_screen (BTK_WINDOW (new_toplevel), 
			   bdk_display_get_screen (display, 0));
  
  btk_widget_reparent (BTK_WIDGET (vbox), new_toplevel);
  btk_widget_destroy (toplevel);
  btk_widget_show_all (new_toplevel);
}


int
main (int argc, char *argv[])
{
  BtkWidget **window;
  BtkWidget *moving_window, *moving_button, *moving_vbox, *moving_image;
  gint num_screen = 0;
  gchar *displayname = NULL;
  gint i;
  BdkScreen **screen_list;
  BdkDisplay *dpy;
  GSList *ids;
  
  btk_init (&argc, &argv);

  dpy = bdk_display_get_default ();
  num_screen = bdk_display_get_n_screens (dpy);
  displayname = g_strdup (bdk_display_get_name (dpy));
  g_print ("This X Server (%s) manages %d screen(s).\n",
	   displayname, num_screen);
  screen_list = g_new (BdkScreen *, num_screen);
  window = g_new (BtkWidget *, num_screen);
  images = g_new (BtkWidget *, num_screen);
  vbox = g_new (BtkWidget *, num_screen);

  ids = btk_stock_list_ids ();

  for (i = 0; i < num_screen; i++)
    {
      char *label = g_strdup_printf ("Screen %d", i);
      BtkWidget *button;
      
      screen_list[i] = bdk_display_get_screen (dpy, i);

      window[i] = g_object_new (BTK_TYPE_WINDOW,
				  "screen", screen_list[i],
				  "user_data", NULL,
				  "type", BTK_WINDOW_TOPLEVEL,
				  "title", label,
				  "allow_grow", FALSE,
				  "allow_shrink", FALSE,
				  "border_width", 10, NULL,
				  NULL);
      g_signal_connect (window[i], "destroy",
			G_CALLBACK (btk_main_quit), NULL);

      vbox[i] = btk_vbox_new (TRUE, 0);
      btk_container_add (BTK_CONTAINER (window[i]), vbox[i]);

      button = g_object_new (BTK_TYPE_BUTTON,
			       "label", label,
			       "parent", vbox[i],
			       "visible", TRUE, NULL,
			       NULL);
      g_signal_connect (button, "clicked",
			G_CALLBACK (hello), label);
  
      images[i] = btk_image_new_from_stock (g_slist_nth (ids, i+1)->data,
					     BTK_ICON_SIZE_BUTTON);
      
      btk_container_add (BTK_CONTAINER (vbox[i]), images[i]);

      button = g_object_new (BTK_TYPE_BUTTON,
			       "label", "Hide Icon",
			       "parent", vbox[i],
			       "visible", TRUE, NULL,
			       NULL);
      g_signal_connect (button, "clicked",
			G_CALLBACK (show_hide), GINT_TO_POINTER (i));
    }
  
  for (i = 0; i < num_screen; i++)
    btk_widget_show_all (window[i]);
  
  moving_window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  moving_vbox = btk_vbox_new (TRUE, 0);
  
  btk_container_add (BTK_CONTAINER (moving_window), moving_vbox);
  moving_button = g_object_new (BTK_TYPE_BUTTON,
				  "label", "Move to Next Screen",
				  "visible", TRUE,
				  NULL);
  
  g_signal_connect (moving_button, "clicked", 
		    G_CALLBACK (move), moving_vbox);
  
  btk_container_add (BTK_CONTAINER (moving_vbox), moving_button);
  
  moving_image = btk_image_new_from_stock (g_slist_nth (ids, num_screen + 2)->data,
					   BTK_ICON_SIZE_BUTTON);
  btk_container_add (BTK_CONTAINER (moving_vbox), moving_image);
  btk_widget_show_all (moving_window);

  btk_main ();

  return 0;
}
