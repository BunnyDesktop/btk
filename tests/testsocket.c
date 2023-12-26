/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* testsocket.c
 * Copyright (C) 2001 Red Hat, Inc
 * Author: Owen Taylor
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

#undef BTK_DISABLE_DEPRECATED

#include "config.h"
#include <btk/btk.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int n_children = 0;

GSList *sockets = NULL;

BtkWidget *window;
BtkWidget *box;

typedef struct 
{
  BtkWidget *box;
  BtkWidget *frame;
  BtkWidget *socket;
} Socket;

extern buint32 create_child_plug (buint32  xid,
				  bboolean local);

static void
quit_cb (bpointer        callback_data,
	 buint           callback_action,
	 BtkWidget      *widget)
{
  BtkWidget *message_dialog = btk_message_dialog_new (BTK_WINDOW (window), 0,
						      BTK_MESSAGE_QUESTION,
						      BTK_BUTTONS_YES_NO,
						      "Really Quit?");
  btk_dialog_set_default_response (BTK_DIALOG (message_dialog), BTK_RESPONSE_NO);

  if (btk_dialog_run (BTK_DIALOG (message_dialog)) == BTK_RESPONSE_YES)
    btk_widget_destroy (window);

  btk_widget_destroy (message_dialog);
}

static BtkItemFactoryEntry menu_items[] =
{
  { "/_File",            NULL,         NULL,                  0, "<Branch>" },
  { "/File/_Quit",       "<control>Q", quit_cb,               0 },
};

static void
socket_destroyed (BtkWidget *widget,
		  Socket    *socket)
{
  sockets = b_slist_remove (sockets, socket);
  g_free (socket);
}

static void
plug_added (BtkWidget *widget,
	    Socket    *socket)
{
  g_print ("Plug added to socket\n");
  
  btk_widget_show (socket->socket);
  btk_widget_hide (socket->frame);
}

static bboolean
plug_removed (BtkWidget *widget,
	      Socket    *socket)
{
  g_print ("Plug removed from socket\n");
  
  btk_widget_hide (socket->socket);
  btk_widget_show (socket->frame);
  
  return TRUE;
}

static Socket *
create_socket (void)
{
  BtkWidget *label;
  
  Socket *socket = g_new (Socket, 1);
  
  socket->box = btk_vbox_new (FALSE, 0);

  socket->socket = btk_socket_new ();
  
  btk_box_pack_start (BTK_BOX (socket->box), socket->socket, TRUE, TRUE, 0);
  
  socket->frame = btk_frame_new (NULL);
  btk_frame_set_shadow_type (BTK_FRAME (socket->frame), BTK_SHADOW_IN);
  btk_box_pack_start (BTK_BOX (socket->box), socket->frame, TRUE, TRUE, 0);
  btk_widget_show (socket->frame);
  
  label = btk_label_new (NULL);
  btk_label_set_markup (BTK_LABEL (label), "<span color=\"red\">Empty</span>");
  btk_container_add (BTK_CONTAINER (socket->frame), label);
  btk_widget_show (label);

  sockets = b_slist_prepend (sockets, socket);


  g_signal_connect (socket->socket, "destroy",
		    G_CALLBACK (socket_destroyed), socket);
  g_signal_connect (socket->socket, "plug_added",
		    G_CALLBACK (plug_added), socket);
  g_signal_connect (socket->socket, "plug_removed",
		    G_CALLBACK (plug_removed), socket);

  return socket;
}

void
steal (BtkWidget *window, BtkEntry *entry)
{
  buint32 xid;
  const bchar *text;
  Socket *socket;

  text = btk_entry_get_text (entry);

  xid = strtol (text, NULL, 0);
  if (xid == 0)
    {
      g_warning ("Invalid window id '%s'\n", text);
      return;
    }

  socket = create_socket ();
  btk_box_pack_start (BTK_BOX (box), socket->box, TRUE, TRUE, 0);
  btk_widget_show (socket->box);

  btk_socket_steal (BTK_SOCKET (socket->socket), xid);
}

void
remove_child (BtkWidget *window)
{
  if (sockets)
    {
      Socket *socket = sockets->data;
      btk_widget_destroy (socket->box);
    }
}

static bboolean
child_read_watch (BUNNYIOChannel *channel, BUNNYIOCondition cond, bpointer data)
{
  BUNNYIOStatus status;
  GError *error = NULL;
  char *line;
  bsize term;
  int xid;
  
  status = g_io_channel_read_line (channel, &line, NULL, &term, &error);
  switch (status)
    {
    case G_IO_STATUS_NORMAL:
      line[term] = '\0';
      xid = strtol (line, NULL, 0);
      if (xid == 0)
	{
	  fprintf (stderr, "Invalid window id '%s'\n", line);
	}
      else
	{
	  Socket *socket = create_socket ();
	  btk_box_pack_start (BTK_BOX (box), socket->box, TRUE, TRUE, 0);
	  btk_widget_show (socket->box);
	  
	  btk_socket_add_id (BTK_SOCKET (socket->socket), xid);
	}
      g_free (line);
      return TRUE;
    case G_IO_STATUS_AGAIN:
      return TRUE;
    case G_IO_STATUS_EOF:
      n_children--;
      return FALSE;
    case G_IO_STATUS_ERROR:
      fprintf (stderr, "Error reading fd from child: %s\n", error->message);
      exit (1);
      return FALSE;
    default:
      g_assert_not_reached ();
      return FALSE;
    }
  
}

void
add_child (BtkWidget *window,
	   bboolean   active)
{
  Socket *socket;
  char *argv[3] = { "./testsocket_child", NULL, NULL };
  char buffer[20];
  int out_fd;
  BUNNYIOChannel *channel;
  GError *error = NULL;

  if (active)
    {
      socket = create_socket ();
      btk_box_pack_start (BTK_BOX (box), socket->box, TRUE, TRUE, 0);
      btk_widget_show (socket->box);
      sprintf(buffer, "%#lx", (bulong) btk_socket_get_id (BTK_SOCKET (socket->socket)));
      argv[1] = buffer;
    }
  
  if (!g_spawn_async_with_pipes (NULL, argv, NULL, 0, NULL, NULL, NULL, NULL, &out_fd, NULL, &error))
    {
      fprintf (stderr, "Can't exec testsocket_child: %s\n", error->message);
      exit (1);
    }

  n_children++;
  channel = g_io_channel_unix_new (out_fd);
  g_io_channel_set_close_on_unref (channel, TRUE);
  g_io_channel_set_flags (channel, G_IO_FLAG_NONBLOCK, &error);
  if (error)
    {
      fprintf (stderr, "Error making channel non-blocking: %s\n", error->message);
      exit (1);
    }
  
  g_io_add_watch (channel, G_IO_IN | G_IO_HUP, child_read_watch, NULL);
  g_io_channel_unref (channel);
}

void
add_active_child (BtkWidget *window)
{
  add_child (window, TRUE);
}

void
add_passive_child (BtkWidget *window)
{
  add_child (window, FALSE);
}

void
add_local_active_child (BtkWidget *window)
{
  Socket *socket;

  socket = create_socket ();
  btk_box_pack_start (BTK_BOX (box), socket->box, TRUE, TRUE, 0);
  btk_widget_show (socket->box);

  create_child_plug (btk_socket_get_id (BTK_SOCKET (socket->socket)), TRUE);
}

void
add_local_passive_child (BtkWidget *window)
{
  Socket *socket;
  BdkNativeWindow xid;

  socket = create_socket ();
  btk_box_pack_start (BTK_BOX (box), socket->box, TRUE, TRUE, 0);
  btk_widget_show (socket->box);

  xid = create_child_plug (0, TRUE);
  btk_socket_add_id (BTK_SOCKET (socket->socket), xid);
}

static const char *
grab_string (int status)
{
  switch (status) {
  case BDK_GRAB_SUCCESS:          return "GrabSuccess";
  case BDK_GRAB_ALREADY_GRABBED:  return "AlreadyGrabbed";
  case BDK_GRAB_INVALID_TIME:     return "GrabInvalidTime";
  case BDK_GRAB_NOT_VIEWABLE:     return "GrabNotViewable";
  case BDK_GRAB_FROZEN:           return "GrabFrozen";
  default:
    {
      static char foo [255];
      sprintf (foo, "unknown status: %d", status);
      return foo;
    }
  }
}

static void
grab_window_toggled (BtkToggleButton *button,
		     BtkWidget       *widget)
{

  if (btk_toggle_button_get_active (button))
    {
      int status;

      status = bdk_keyboard_grab (widget->window, FALSE, BDK_CURRENT_TIME);

      if (status != BDK_GRAB_SUCCESS)
	g_warning ("Could not grab keyboard!  (%s)", grab_string (status));

    } 
  else 
    {
      bdk_keyboard_ungrab (BDK_CURRENT_TIME);
    }
}

int
main (int argc, char *argv[])
{
  BtkWidget *button;
  BtkWidget *hbox;
  BtkWidget *vbox;
  BtkWidget *entry;
  BtkWidget *checkbutton;
  BtkAccelGroup *accel_group;
  BtkItemFactory *item_factory;

  btk_init (&argc, &argv);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy",
		    G_CALLBACK (btk_main_quit), NULL);
  
  btk_window_set_title (BTK_WINDOW (window), "Socket Test");
  btk_container_set_border_width (BTK_CONTAINER (window), 0);

  vbox = btk_vbox_new (FALSE, 0);
  btk_container_add (BTK_CONTAINER (window), vbox);

  accel_group = btk_accel_group_new ();
  btk_window_add_accel_group (BTK_WINDOW (window), accel_group);
  item_factory = btk_item_factory_new (BTK_TYPE_MENU_BAR, "<main>", accel_group);

  
  btk_item_factory_create_items (item_factory,
				 G_N_ELEMENTS (menu_items), menu_items,
				 NULL);
      
  btk_box_pack_start (BTK_BOX (vbox),
		      btk_item_factory_get_widget (item_factory, "<main>"),
		      FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Add Active Child");
  btk_box_pack_start (BTK_BOX(vbox), button, FALSE, FALSE, 0);

  g_signal_connect_swapped (button, "clicked",
			    G_CALLBACK (add_active_child), vbox);

  button = btk_button_new_with_label ("Add Passive Child");
  btk_box_pack_start (BTK_BOX(vbox), button, FALSE, FALSE, 0);

  g_signal_connect_swapped (button, "clicked",
			    G_CALLBACK (add_passive_child), vbox);

  button = btk_button_new_with_label ("Add Local Active Child");
  btk_box_pack_start (BTK_BOX(vbox), button, FALSE, FALSE, 0);

  g_signal_connect_swapped (button, "clicked",
			    G_CALLBACK (add_local_active_child), vbox);

  button = btk_button_new_with_label ("Add Local Passive Child");
  btk_box_pack_start (BTK_BOX(vbox), button, FALSE, FALSE, 0);

  g_signal_connect_swapped (button, "clicked",
			    G_CALLBACK (add_local_passive_child), vbox);

  button = btk_button_new_with_label ("Remove Last Child");
  btk_box_pack_start (BTK_BOX(vbox), button, FALSE, FALSE, 0);

  g_signal_connect_swapped (button, "clicked",
			    G_CALLBACK (remove_child), vbox);

  checkbutton = btk_check_button_new_with_label ("Grab keyboard");
  btk_box_pack_start (BTK_BOX (vbox), checkbutton, FALSE, FALSE, 0);

  g_signal_connect (checkbutton, "toggled",
		    G_CALLBACK (grab_window_toggled),
		    window);

  hbox = btk_hbox_new (FALSE, 0);
  btk_box_pack_start (BTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  entry = btk_entry_new ();
  btk_box_pack_start (BTK_BOX(hbox), entry, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Steal");
  btk_box_pack_start (BTK_BOX(hbox), button, FALSE, FALSE, 0);

  g_signal_connect (button, "clicked",
		    G_CALLBACK (steal),
		    entry);

  hbox = btk_hbox_new (FALSE, 0);
  btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  box = hbox;
  
  btk_widget_show_all (window);

  btk_main ();

  if (n_children)
    {
      g_print ("Waiting for children to exit\n");

      while (n_children)
	g_main_context_iteration (NULL, TRUE);
    }

  return 0;
}
