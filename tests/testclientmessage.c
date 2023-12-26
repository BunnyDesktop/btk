/* testclientmessage.c
 * Copyright (C) 2008  Novell, Inc.
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

#include <btk/btk.h>

static BdkAtom my_type;
static BdkAtom random_type;

static void
send_known (void)
{
  BdkEvent *event = bdk_event_new (BDK_CLIENT_EVENT);
  static int counter = 42;
  int i;
  
  event->client.window = NULL;
  event->client.message_type = my_type;
  event->client.data_format = 32;
  event->client.data.l[0] = counter++;
  for (i = 1; i < 5; i++)
    event->client.data.l[i] = 0;

  bdk_screen_broadcast_client_message (bdk_display_get_default_screen (bdk_display_get_default ()), event);
  
  bdk_event_free (event);
}

void
send_random (void)
{
  BdkEvent *event = bdk_event_new (BDK_CLIENT_EVENT);
  static int counter = 1;
  int i;
  
  event->client.window = NULL;
  event->client.message_type = random_type;
  event->client.data_format = 32;
  event->client.data.l[0] = counter++;
  for (i = 1; i < 5; i++)
    event->client.data.l[i] = 0;

  bdk_screen_broadcast_client_message (bdk_display_get_default_screen (bdk_display_get_default ()), event);
  
  bdk_event_free (event);
}

static BdkFilterReturn
filter_func (BdkXEvent *xevent,
	     BdkEvent  *event,
	     gpointer   data)
{
  g_print ("Got matching client message!\n");
  return BDK_FILTER_REMOVE;
}

int
main (int argc, char **argv)
{
  BtkWidget *window;
  BtkWidget *vbox;
  BtkWidget *button;

  btk_init (&argc, &argv);

  my_type = bdk_atom_intern ("BtkTestClientMessage", FALSE);
  random_type = bdk_atom_intern (g_strdup_printf ("BtkTestClientMessage-%d",
						  g_rand_int_range (g_rand_new (), 1, 99)),
				 FALSE);

  g_print ("using random client message type %s\n", bdk_atom_name (random_type));

  window = g_object_connect (g_object_new (btk_window_get_type (),
					   "type", BTK_WINDOW_TOPLEVEL,
					   "title", "testclientmessage",
					   "border_width", 10,
					   NULL),
			     "signal::destroy", btk_main_quit, NULL,
			     NULL);
  vbox = g_object_new (btk_vbox_get_type (),
		       "BtkWidget::parent", window,
		       "BtkWidget::visible", TRUE,
		       NULL);
  button = g_object_connect (g_object_new (btk_button_get_type (),
					   "BtkButton::label", "send known client message",
					   "BtkWidget::parent", vbox,
					   "BtkWidget::visible", TRUE,
					   NULL),
			     "signal::clicked", send_known, NULL,
			     NULL);
  button = g_object_connect (g_object_new (btk_button_get_type (),
					   "BtkButton::label", "send random client message",
					   "BtkWidget::parent", vbox,
					   "BtkWidget::visible", TRUE,
					   NULL),
			     "signal::clicked", send_random, NULL,
			     NULL);
  bdk_display_add_client_message_filter (bdk_display_get_default (),
					 my_type,
					 filter_func,
					 NULL);
  btk_widget_show (window);

  btk_main ();

  return 0;
}
