/* simple.c
 * Copyright (C) 1997  Red Hat, Inc
 * Author: Elliot Lee
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


void
hello (void)
{
  g_print ("hello world\n");
}

int
main (int argc, char *argv[])
{
  BtkWidget *window;
  BtkWidget *button;

  /* FIXME: This is not allowable - what is this supposed to be? */
  /*  bdk_progclass = g_strdup ("XTerm"); */
  btk_init (&argc, &argv);
  
  window = g_object_connect (g_object_new (btk_window_get_type (),
					     "user_data", NULL,
					     "type", BTK_WINDOW_TOPLEVEL,
					     "title", "hello world",
					     "allow_grow", FALSE,
					     "allow_shrink", FALSE,
					     "border_width", 10,
					     NULL),
			     "signal::destroy", btk_main_quit, NULL,
			     NULL);
  button = g_object_connect (g_object_new (btk_button_get_type (),
					     "BtkButton::label", "hello world",
					     "BtkWidget::parent", window,
					     "BtkWidget::visible", TRUE,
					     NULL),
			     "signal::clicked", hello, NULL,
			     NULL);
  btk_widget_show (window);

  btk_main ();

  return 0;
}
