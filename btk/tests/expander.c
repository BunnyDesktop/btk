/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */
#include <btk/btk.h>

static void
test_click_expander (void)
{
  BtkWidget *window = btk_test_create_simple_window ("Test Window", "Test click on expander");
  BtkWidget *expander = btk_expander_new ("Test Expander");
  BtkWidget *label = btk_label_new ("Test Label");
  gboolean expanded;
  gboolean simsuccess;
  btk_container_add (BTK_CONTAINER (expander), label);
  btk_container_add (BTK_CONTAINER (BTK_BIN (window)->child), expander);
  btk_widget_show (expander);
  btk_widget_show (label);
  btk_widget_show_now (window);
  /* check initial expander state */
  expanded = btk_expander_get_expanded (BTK_EXPANDER (expander));
  g_assert (!expanded);
  /* check expanding */
  simsuccess = btk_test_widget_click (expander, 1, 0);
  g_assert (simsuccess == TRUE);
  while (btk_events_pending ()) /* let expander timeout/idle handlers update */
    btk_main_iteration ();
  expanded = btk_expander_get_expanded (BTK_EXPANDER (expander));
  g_assert (expanded);
  /* check collapsing */
  simsuccess = btk_test_widget_click (expander, 1, 0);
  g_assert (simsuccess == TRUE);
  while (btk_events_pending ()) /* let expander timeout/idle handlers update */
    btk_main_iteration ();
  expanded = btk_expander_get_expanded (BTK_EXPANDER (expander));
  g_assert (!expanded);
}

static void
test_click_content_widget (void)
{
  BtkWidget *window = btk_test_create_simple_window ("Test Window", "Test click on content widget");
  BtkWidget *expander = btk_expander_new ("Test Expander");
  BtkWidget *entry = btk_entry_new ();
  gboolean expanded;
  gboolean simsuccess;
  btk_container_add (BTK_CONTAINER (expander), entry);
  btk_container_add (BTK_CONTAINER (BTK_BIN (window)->child), expander);
  btk_expander_set_expanded (BTK_EXPANDER (expander), TRUE);
  btk_widget_show (expander);
  btk_widget_show (entry);
  btk_widget_show_now (window);

  /* check click on content with expander open */
  expanded = btk_expander_get_expanded (BTK_EXPANDER (expander));
  g_assert (expanded);
  simsuccess = btk_test_widget_click (entry, 1, 0);
  g_assert (simsuccess == TRUE);
  while (btk_events_pending ()) /* let expander timeout/idle handlers update */
    btk_main_iteration ();
  expanded = btk_expander_get_expanded (BTK_EXPANDER (expander));
  g_assert (expanded);
}

int
main (int   argc,
      char *argv[])
{
  btk_test_init (&argc, &argv);
  g_test_add_func ("/expander/click-expander", test_click_expander);
  g_test_add_func ("/expander/click-content-widget", test_click_content_widget);
  return g_test_run();
}
