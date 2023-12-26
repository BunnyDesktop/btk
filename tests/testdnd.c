/* testdnd.c
 * Copyright (C) 1998  Red Hat, Inc.
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

#include "config.h"
#include "btk/btk.h"

/* Target side drag signals */

/* XPM */
static const char * drag_icon_xpm[] = {
"36 48 9 1",
" 	c None",
".	c #020204",
"+	c #8F8F90",
"@	c #D3D3D2",
"#	c #AEAEAC",
"$	c #ECECEC",
"%	c #A2A2A4",
"&	c #FEFEFC",
"*	c #BEBEBC",
"               .....................",
"              ..&&&&&&&&&&&&&&&&&&&.",
"             ...&&&&&&&&&&&&&&&&&&&.",
"            ..&.&&&&&&&&&&&&&&&&&&&.",
"           ..&&.&&&&&&&&&&&&&&&&&&&.",
"          ..&&&.&&&&&&&&&&&&&&&&&&&.",
"         ..&&&&.&&&&&&&&&&&&&&&&&&&.",
"        ..&&&&&.&&&@&&&&&&&&&&&&&&&.",
"       ..&&&&&&.*$%$+$&&&&&&&&&&&&&.",
"      ..&&&&&&&.%$%$+&&&&&&&&&&&&&&.",
"     ..&&&&&&&&.#&#@$&&&&&&&&&&&&&&.",
"    ..&&&&&&&&&.#$**#$&&&&&&&&&&&&&.",
"   ..&&&&&&&&&&.&@%&%$&&&&&&&&&&&&&.",
"  ..&&&&&&&&&&&.&&&&&&&&&&&&&&&&&&&.",
" ..&&&&&&&&&&&&.&&&&&&&&&&&&&&&&&&&.",
"................&$@&&&@&&&&&&&&&&&&.",
".&&&&&&&+&&#@%#+@#@*$%$+$&&&&&&&&&&.",
".&&&&&&&+&&#@#@&&@*%$%$+&&&&&&&&&&&.",
".&&&&&&&+&$%&#@&#@@#&#@$&&&&&&&&&&&.",
".&&&&&&@#@@$&*@&@#@#$**#$&&&&&&&&&&.",
".&&&&&&&&&&&&&&&&&&&@%&%$&&&&&&&&&&.",
".&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&.",
".&&&&&&&&$#@@$&&&&&&&&&&&&&&&&&&&&&.",
".&&&&&&&&&+&$+&$&@&$@&&$@&&&&&&&&&&.",
".&&&&&&&&&+&&#@%#+@#@*$%&+$&&&&&&&&.",
".&&&&&&&&&+&&#@#@&&@*%$%$+&&&&&&&&&.",
".&&&&&&&&&+&$%&#@&#@@#&#@$&&&&&&&&&.",
".&&&&&&&&@#@@$&*@&@#@#$#*#$&&&&&&&&.",
".&&&&&&&&&&&&&&&&&&&&&$%&%$&&&&&&&&.",
".&&&&&&&&&&$#@@$&&&&&&&&&&&&&&&&&&&.",
".&&&&&&&&&&&+&$%&$$@&$@&&$@&&&&&&&&.",
".&&&&&&&&&&&+&&#@%#+@#@*$%$+$&&&&&&.",
".&&&&&&&&&&&+&&#@#@&&@*#$%$+&&&&&&&.",
".&&&&&&&&&&&+&$+&*@&#@@#&#@$&&&&&&&.",
".&&&&&&&&&&$%@@&&*@&@#@#$#*#&&&&&&&.",
".&&&&&&&&&&&&&&&&&&&&&&&$%&%$&&&&&&.",
".&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&.",
".&&&&&&&&&&&&&&$#@@$&&&&&&&&&&&&&&&.",
".&&&&&&&&&&&&&&&+&$%&$$@&$@&&$@&&&&.",
".&&&&&&&&&&&&&&&+&&#@%#+@#@*$%$+$&&.",
".&&&&&&&&&&&&&&&+&&#@#@&&@*#$%$+&&&.",
".&&&&&&&&&&&&&&&+&$+&*@&#@@#&#@$&&&.",
".&&&&&&&&&&&&&&$%@@&&*@&@#@#$#*#&&&.",
".&&&&&&&&&&&&&&&&&&&&&&&&&&&$%&%$&&.",
".&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&.",
".&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&.",
".&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&.",
"...................................."};

/* XPM */
static const char * trashcan_closed_xpm[] = {
"64 80 17 1",
" 	c None",
".	c #030304",
"+	c #5A5A5C",
"@	c #323231",
"#	c #888888",
"$	c #1E1E1F",
"%	c #767677",
"&	c #494949",
"*	c #9E9E9C",
"=	c #111111",
"-	c #3C3C3D",
";	c #6B6B6B",
">	c #949494",
",	c #282828",
"'	c #808080",
")	c #545454",
"!	c #AEAEAC",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                       ==......=$$...===                        ",
"                 ..$------)+++++++++++++@$$...                  ",
"             ..=@@-------&+++++++++++++++++++-....              ",
"          =.$$@@@-&&)++++)-,$$$$=@@&+++++++++++++,..$           ",
"         .$$$$@@&+++++++&$$$@@@@-&,$,-++++++++++;;;&..          ",
"        $$$$,@--&++++++&$$)++++++++-,$&++++++;%%'%%;;$@         ",
"       .-@@-@-&++++++++-@++++++++++++,-++++++;''%;;;%*-$        ",
"       +------++++++++++++++++++++++++++++++;;%%%;;##*!.        ",
"        =+----+++++++++++++++++++++++;;;;;;;;;;;;%'>>).         ",
"         .=)&+++++++++++++++++;;;;;;;;;;;;;;%''>>#>#@.          ",
"          =..=&++++++++++++;;;;;;;;;;;;;%###>>###+%==           ",
"           .&....=-+++++%;;####''''''''''##'%%%)..#.            ",
"           .+-++@....=,+%#####'%%%%%%%%%;@$-@-@*++!.            ",
"           .+-++-+++-&-@$$=$=......$,,,@;&)+!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           =+-++-+++-+++++++++!++++!++++!+++!++!+++=            ",
"            $.++-+++-+++++++++!++++!++++!+++!++!+.$             ",
"              =.++++++++++++++!++++!++++!+++!++.=               ",
"                 $..+++++++++++++++!++++++...$                  ",
"                      $$=.............=$$                       ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                "};

/* XPM */
static const char * trashcan_open_xpm[] = {
"64 80 17 1",
" 	c None",
".	c #030304",
"+	c #5A5A5C",
"@	c #323231",
"#	c #888888",
"$	c #1E1E1F",
"%	c #767677",
"&	c #494949",
"*	c #9E9E9C",
"=	c #111111",
"-	c #3C3C3D",
";	c #6B6B6B",
">	c #949494",
",	c #282828",
"'	c #808080",
")	c #545454",
"!	c #AEAEAC",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                      .=.==.,@                  ",
"                                   ==.,@-&&&)-=                 ",
"                                 .$@,&++;;;%>*-                 ",
"                               $,-+)+++%%;;'#+.                 ",
"                            =---+++++;%%%;%##@.                 ",
"                           @)++++++++;%%%%'#%$                  ",
"                         $&++++++++++;%%;%##@=                  ",
"                       ,-++++)+++++++;;;'#%)                    ",
"                      @+++&&--&)++++;;%'#'-.                    ",
"                    ,&++-@@,,,,-)++;;;'>'+,                     ",
"                  =-++&@$@&&&&-&+;;;%##%+@                      ",
"                =,)+)-,@@&+++++;;;;%##%&@                       ",
"               @--&&,,@&)++++++;;;;'#)@                         ",
"              ---&)-,@)+++++++;;;%''+,                          ",
"            $--&)+&$-+++++++;;;%%'';-                           ",
"           .,-&+++-$&++++++;;;%''%&=                            ",
"          $,-&)++)-@++++++;;%''%),                              ",
"         =,@&)++++&&+++++;%'''+$@&++++++                        ",
"        .$@-++++++++++++;'#';,........=$@&++++                  ",
"       =$@@&)+++++++++++'##-.................=&++               ",
"      .$$@-&)+++++++++;%#+$.....................=)+             ",
"      $$,@-)+++++++++;%;@=........................,+            ",
"     .$$@@-++++++++)-)@=............................            ",
"     $,@---)++++&)@===............................,.            ",
"    $-@---&)))-$$=..............................=)!.            ",
"     --&-&&,,$=,==...........................=&+++!.            ",
"      =,=$..=$+)+++++&@$=.............=$@&+++++!++!.            ",
"           .)-++-+++++++++++++++++++++++++++!++!++!.            ",
"           .+-++-+++++++++++++++++++++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!+++!!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           .+-++-+++-+++++++++!++++!++++!+++!++!++!.            ",
"           =+-++-+++-+++++++++!++++!++++!+++!++!+++=            ",
"            $.++-+++-+++++++++!++++!++++!+++!++!+.$             ",
"              =.++++++++++++++!++++!++++!+++!++.=               ",
"                 $..+++++++++++++++!++++++...$                  ",
"                      $$==...........==$$                       ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                "};

BdkPixbuf *trashcan_open;
BdkPixbuf *trashcan_closed;

bboolean have_drag;

enum {
  TARGET_STRING,
  TARGET_ROOTWIN
};

static BtkTargetEntry target_table[] = {
  { "STRING",     0, TARGET_STRING },
  { "text/plain", 0, TARGET_STRING },
  { "application/x-rootwindow-drop", 0, TARGET_ROOTWIN }
};

static buint n_targets = sizeof(target_table) / sizeof(target_table[0]);

void  
target_drag_leave	   (BtkWidget	       *widget,
			    BdkDragContext     *context,
			    buint               time)
{
  g_print("leave\n");
  have_drag = FALSE;
  btk_image_set_from_pixbuf (BTK_IMAGE (widget), trashcan_closed);
}

bboolean
target_drag_motion	   (BtkWidget	       *widget,
			    BdkDragContext     *context,
			    bint                x,
			    bint                y,
			    buint               time)
{
  BtkWidget *source_widget;
  GList *tmp_list;

  if (!have_drag)
    {
      have_drag = TRUE;
      btk_image_set_from_pixbuf (BTK_IMAGE (widget), trashcan_open);
    }

  source_widget = btk_drag_get_source_widget (context);
  g_print ("motion, source %s\n", source_widget ?
	   B_OBJECT_TYPE_NAME (source_widget) :
	   "NULL");

  tmp_list = context->targets;
  while (tmp_list)
    {
      char *name = bdk_atom_name (BDK_POINTER_TO_ATOM (tmp_list->data));
      g_print ("%s\n", name);
      g_free (name);
      
      tmp_list = tmp_list->next;
    }

  bdk_drag_status (context, context->suggested_action, time);
  return TRUE;
}

bboolean
target_drag_drop	   (BtkWidget	       *widget,
			    BdkDragContext     *context,
			    bint                x,
			    bint                y,
			    buint               time)
{
  g_print("drop\n");
  have_drag = FALSE;

  btk_image_set_from_pixbuf (BTK_IMAGE (widget), trashcan_closed);

  if (context->targets)
    {
      btk_drag_get_data (widget, context, 
			 BDK_POINTER_TO_ATOM (context->targets->data), 
			 time);
      return TRUE;
    }
  
  return FALSE;
}

void  
target_drag_data_received  (BtkWidget          *widget,
			    BdkDragContext     *context,
			    bint                x,
			    bint                y,
			    BtkSelectionData   *data,
			    buint               info,
			    buint               time)
{
  if ((data->length >= 0) && (data->format == 8))
    {
      g_print ("Received \"%s\" in trashcan\n", (bchar *)data->data);
      btk_drag_finish (context, TRUE, FALSE, time);
      return;
    }
  
  btk_drag_finish (context, FALSE, FALSE, time);
}
  
void  
label_drag_data_received  (BtkWidget          *widget,
			    BdkDragContext     *context,
			    bint                x,
			    bint                y,
			    BtkSelectionData   *data,
			    buint               info,
			    buint               time)
{
  if ((data->length >= 0) && (data->format == 8))
    {
      g_print ("Received \"%s\" in label\n", (bchar *)data->data);
      btk_drag_finish (context, TRUE, FALSE, time);
      return;
    }
  
  btk_drag_finish (context, FALSE, FALSE, time);
}

void  
source_drag_data_get  (BtkWidget          *widget,
		       BdkDragContext     *context,
		       BtkSelectionData   *selection_data,
		       buint               info,
		       buint               time,
		       bpointer            data)
{
  if (info == TARGET_ROOTWIN)
    g_print ("I was dropped on the rootwin\n");
  else
    btk_selection_data_set (selection_data,
			    selection_data->target,
			    8, (buchar *) "I'm Data!", 9);
}
  
/* The following is a rather elaborate example demonstrating/testing
 * changing of the window hierarchy during a drag - in this case,
 * via a "spring-loaded" popup window.
 */
static BtkWidget *popup_window = NULL;

static bboolean popped_up = FALSE;
static bboolean in_popup = FALSE;
static buint popdown_timer = 0;
static buint popup_timer = 0;

bint
popdown_cb (bpointer data)
{
  popdown_timer = 0;

  btk_widget_hide (popup_window);
  popped_up = FALSE;

  return FALSE;
}

bboolean
popup_motion	   (BtkWidget	       *widget,
		    BdkDragContext     *context,
		    bint                x,
		    bint                y,
		    buint               time)
{
  if (!in_popup)
    {
      in_popup = TRUE;
      if (popdown_timer)
	{
	  g_print ("removed popdown\n");
	  g_source_remove (popdown_timer);
	  popdown_timer = 0;
	}
    }

  return TRUE;
}

void  
popup_leave	   (BtkWidget	       *widget,
		    BdkDragContext     *context,
		    buint               time)
{
  if (in_popup)
    {
      in_popup = FALSE;
      if (!popdown_timer)
	{
	  g_print ("added popdown\n");
	  popdown_timer = bdk_threads_add_timeout (500, popdown_cb, NULL);
	}
    }
}

bboolean
popup_cb (bpointer data)
{
  if (!popped_up)
    {
      if (!popup_window)
	{
	  BtkWidget *button;
	  BtkWidget *table;
	  int i, j;
	  
	  popup_window = btk_window_new (BTK_WINDOW_POPUP);
	  btk_window_set_position (BTK_WINDOW (popup_window), BTK_WIN_POS_MOUSE);

	  table = btk_table_new (3,3, FALSE);

	  for (i=0; i<3; i++)
	    for (j=0; j<3; j++)
	      {
		char buffer[128];
		g_snprintf(buffer, sizeof(buffer), "%d,%d", i, j);
		button = btk_button_new_with_label (buffer);
		btk_table_attach (BTK_TABLE (table), button, i, i+1, j, j+1,
				  BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
				  0, 0);

		btk_drag_dest_set (button,
				   BTK_DEST_DEFAULT_ALL,
				   target_table, n_targets - 1, /* no rootwin */
				   BDK_ACTION_COPY | BDK_ACTION_MOVE);
		g_signal_connect (button, "drag_motion",
				  G_CALLBACK (popup_motion), NULL);
		g_signal_connect (button, "drag_leave",
				  G_CALLBACK (popup_leave), NULL);
	      }

	  btk_widget_show_all (table);
	  btk_container_add (BTK_CONTAINER (popup_window), table);

	}
      btk_widget_show (popup_window);
      popped_up = TRUE;
    }

  popdown_timer = bdk_threads_add_timeout (500, popdown_cb, NULL);
  g_print ("added popdown\n");

  popup_timer = FALSE;

  return FALSE;
}

bboolean
popsite_motion	   (BtkWidget	       *widget,
		    BdkDragContext     *context,
		    bint                x,
		    bint                y,
		    buint               time)
{
  if (!popup_timer)
    popup_timer = bdk_threads_add_timeout (500, popup_cb, NULL);

  return TRUE;
}

void  
popsite_leave	   (BtkWidget	       *widget,
		    BdkDragContext     *context,
		    buint               time)
{
  if (popup_timer)
    {
      g_source_remove (popup_timer);
      popup_timer = 0;
    }
}

void  
source_drag_data_delete  (BtkWidget          *widget,
			  BdkDragContext     *context,
			  bpointer            data)
{
  g_print ("Delete the data!\n");
}
  
void
test_init (void)
{
  if (g_file_test ("../bdk-pixbuf/libpixbufloader-pnm.la",
		   G_FILE_TEST_EXISTS))
    {
      g_setenv ("BDK_PIXBUF_MODULE_FILE", "../bdk-pixbuf/bdk-pixbuf.loaders", TRUE);
      g_setenv ("BTK_IM_MODULE_FILE", "../modules/input/immodules.cache", TRUE);
    }
}

int 
main (int argc, char **argv)
{
  BtkWidget *window;
  BtkWidget *table;
  BtkWidget *label;
  BtkWidget *pixmap;
  BtkWidget *button;
  BdkPixbuf *drag_icon;

  test_init ();
  
  btk_init (&argc, &argv); 

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy",
		    G_CALLBACK (btk_main_quit), NULL);

  
  table = btk_table_new (2, 2, FALSE);
  btk_container_add (BTK_CONTAINER (window), table);

  drag_icon = bdk_pixbuf_new_from_xpm_data (drag_icon_xpm);
  trashcan_open = bdk_pixbuf_new_from_xpm_data (trashcan_open_xpm);
  trashcan_closed = bdk_pixbuf_new_from_xpm_data (trashcan_closed_xpm);
  
  label = btk_label_new ("Drop Here\n");

  btk_drag_dest_set (label,
		     BTK_DEST_DEFAULT_ALL,
		     target_table, n_targets - 1, /* no rootwin */
		     BDK_ACTION_COPY | BDK_ACTION_MOVE);

  g_signal_connect (label, "drag_data_received",
		    G_CALLBACK( label_drag_data_received), NULL);

  btk_table_attach (BTK_TABLE (table), label, 0, 1, 0, 1,
		    BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
		    0, 0);

  label = btk_label_new ("Popup\n");

  btk_drag_dest_set (label,
		     BTK_DEST_DEFAULT_ALL,
		     target_table, n_targets - 1, /* no rootwin */
		     BDK_ACTION_COPY | BDK_ACTION_MOVE);

  btk_table_attach (BTK_TABLE (table), label, 1, 2, 1, 2,
		    BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
		    0, 0);

  g_signal_connect (label, "drag_motion",
		    G_CALLBACK (popsite_motion), NULL);
  g_signal_connect (label, "drag_leave",
		    G_CALLBACK (popsite_leave), NULL);
  
  pixmap = btk_image_new_from_pixbuf (trashcan_closed);
  btk_drag_dest_set (pixmap, 0, NULL, 0, 0);
  btk_table_attach (BTK_TABLE (table), pixmap, 1, 2, 0, 1,
		    BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
		    0, 0);

  g_signal_connect (pixmap, "drag_leave",
		    G_CALLBACK (target_drag_leave), NULL);

  g_signal_connect (pixmap, "drag_motion",
		    G_CALLBACK (target_drag_motion), NULL);

  g_signal_connect (pixmap, "drag_drop",
		    G_CALLBACK (target_drag_drop), NULL);

  g_signal_connect (pixmap, "drag_data_received",
		    G_CALLBACK (target_drag_data_received), NULL);

  /* Drag site */

  button = btk_button_new_with_label ("Drag Here\n");

  btk_drag_source_set (button, BDK_BUTTON1_MASK | BDK_BUTTON3_MASK,
		       target_table, n_targets, 
		       BDK_ACTION_COPY | BDK_ACTION_MOVE);
  btk_drag_source_set_icon_pixbuf (button, drag_icon);

  g_object_unref (drag_icon);

  btk_table_attach (BTK_TABLE (table), button, 0, 1, 1, 2,
		    BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
		    0, 0);

  g_signal_connect (button, "drag_data_get",
		    G_CALLBACK (source_drag_data_get), NULL);
  g_signal_connect (button, "drag_data_delete",
		    G_CALLBACK (source_drag_data_delete), NULL);

  btk_widget_show_all (window);

  btk_main ();

  return 0;
}
