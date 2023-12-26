/* Btk+ testing utilities
 * Copyright (C) 2007 Imendio AB
 * Authors: Tim Janik
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
#include <bdk/bdkkeysyms.h>
#include <string.h>
#include <math.h>

/* Use a keyval that requires Shift to be active (in typical layouts)
 * like BDK_KEY_ampersand, which is '<shift>6'
 */
#define KEYVAL_THAT_REQUIRES_SHIFT BDK_KEY_ampersand


/* --- test functions --- */
static void
test_button_clicks (void)
{
  int a = 0, b = 0, c = 0;
  BtkWidget *window = btk_test_display_button_window ("Test Window",
                                                      "Test: btk_test_widget_click",
                                                      "IgnoreMe1", &a,
                                                      "ClickMe", &b,
                                                      "IgnoreMe2", &c,
                                                      NULL);
  BtkWidget *button = btk_test_find_widget (window, "*Click*", BTK_TYPE_BUTTON);
  bboolean simsuccess;
  g_assert (button != NULL);
  simsuccess = btk_test_widget_click (button, 1, 0);
  g_assert (simsuccess == TRUE);
  while (btk_events_pending ()) {
    g_print ("iterate main loop\n");
    btk_main_iteration ();
  }
  g_assert (a == 0);
  g_assert (b > 0);
  g_assert (c == 0);
}

static void
test_button_keys (void)
{
  int a = 0, b = 0, c = 0;
  BtkWidget *window = btk_test_display_button_window ("Test Window",
                                                      "Test: btk_test_widget_send_key",
                                                      "IgnoreMe1", &a,
                                                      "ClickMe", &b,
                                                      "IgnoreMe2", &c,
                                                      NULL);
  BtkWidget *button = btk_test_find_widget (window, "*Click*", BTK_TYPE_BUTTON);
  bboolean simsuccess;
  g_assert (button != NULL);
  btk_widget_grab_focus (button);
  g_assert (btk_widget_has_focus (button));
  simsuccess = btk_test_widget_send_key (button, BDK_Return, 0);
  g_assert (simsuccess == TRUE);
  while (btk_events_pending ())
    btk_main_iteration ();
  g_assert (a == 0);
  g_assert (b > 0);
  g_assert (c == 0);
}

static bboolean
store_last_key_release (BtkWidget   *widget,
                        BdkEventKey *event,
                        bpointer     user_data)
{
  *((bint *)user_data) = event->keyval;
  return FALSE;
}

static void
test_send_shift_key (void)
{
  BtkWidget *window = btk_test_display_button_window ("Test Window",
                                                      "Test: test_send_shift_key()",
                                                      "IgnoreMe1", NULL,
                                                      "SendMeKeys", NULL,
                                                      "IgnoreMe2", NULL,
                                                      NULL);
  BtkWidget *button = btk_test_find_widget (window, "SendMeKeys", BTK_TYPE_BUTTON);
  bint last_key_release = 0;
  bboolean simsuccess;
  g_assert (button != NULL);
  g_signal_connect (button, "key-release-event",
                    G_CALLBACK (store_last_key_release),
                    &last_key_release);
  btk_widget_grab_focus (button);
  g_assert (btk_widget_has_focus (button));
  simsuccess = btk_test_widget_send_key (button, KEYVAL_THAT_REQUIRES_SHIFT, 0 /*modifiers*/);
  g_assert (simsuccess == TRUE);
  while (btk_events_pending ())
    btk_main_iteration ();
  g_assert_cmpint (KEYVAL_THAT_REQUIRES_SHIFT, ==, last_key_release);
}

static void
test_slider_ranges (void)
{
  BtkWidget *window = btk_test_create_simple_window ("Test Window", "Test: btk_test_warp_slider");
  BtkWidget *hscale = btk_hscale_new_with_range (-50, +50, 5);
  btk_container_add (BTK_CONTAINER (BTK_BIN (window)->child), hscale);
  btk_widget_show (hscale);
  btk_widget_show_now (window);
  while (btk_events_pending ())
    btk_main_iteration ();
  btk_test_slider_set_perc (hscale, 0.0);
  while (btk_events_pending ())
    btk_main_iteration ();
  g_assert (btk_test_slider_get_value (hscale) == -50);
  btk_test_slider_set_perc (hscale, 50.0);
  while (btk_events_pending ())
    btk_main_iteration ();
  g_assert (fabs (btk_test_slider_get_value (hscale)) < 0.0001);
  btk_test_slider_set_perc (hscale, 100.0);
  while (btk_events_pending ())
    btk_main_iteration ();
  g_assert (btk_test_slider_get_value (hscale) == +50.0);
}

static void
test_text_access (void)
{
  const int N_WIDGETS = 4;
  BtkWidget *widgets[N_WIDGETS];
  int i = 0;
  widgets[i++] = btk_test_create_widget (BTK_TYPE_LABEL, NULL);
  widgets[i++] = btk_test_create_widget (BTK_TYPE_ENTRY, NULL);
  widgets[i++] = btk_test_create_widget (BTK_TYPE_TEXT_VIEW, NULL);
  widgets[i++] = btk_test_create_widget (g_type_from_name ("BtkText"), NULL);
  g_assert (i == N_WIDGETS);
  for (i = 0; i < N_WIDGETS; i++)
    btk_test_text_set (widgets[i], "foobar");
  for (i = 0; i < N_WIDGETS; i++)
    {
      bchar *text  = btk_test_text_get (widgets[i]);
      g_assert (strcmp (text, "foobar") == 0);
      g_free (text);
    }
  for (i = 0; i < N_WIDGETS; i++)
    btk_test_text_set (widgets[i], "");
  for (i = 0; i < N_WIDGETS; i++)
    {
      bchar *text  = btk_test_text_get (widgets[i]);
      g_assert (strcmp (text, "") == 0);
      g_free (text);
    }
}

static void
test_xserver_sync (void)
{
  BtkWidget *window = btk_test_create_simple_window ("Test Window", "Test: test_xserver_sync");
  BtkWidget *darea = btk_drawing_area_new ();
  GTimer *gtimer = g_timer_new();
  bint sync_is_slower = 0, repeat = 5;
  btk_widget_set_size_request (darea, 320, 200);
  btk_container_add (BTK_CONTAINER (BTK_BIN (window)->child), darea);
  btk_widget_show (darea);
  btk_widget_show_now (window);
  while (repeat--)
    {
      bint i, many = 200;
      double nosync_time, sync_time;
      bairo_t *cr;

      while (btk_events_pending ())
        btk_main_iteration ();
      cr = bdk_bairo_create (darea->window);
      bairo_set_source_rgba (cr, 0, 1, 0, 0.1);
      /* run a number of consecutive drawing requests, just using drawing queue */
      g_timer_start (gtimer);
      for (i = 0; i < many; i++)
        {
          bairo_paint (cr);
        }
      g_timer_stop (gtimer);
      nosync_time = g_timer_elapsed (gtimer, NULL);
      bdk_flush();
      while (btk_events_pending ())
        btk_main_iteration ();
      g_timer_start (gtimer);
      /* run a number of consecutive drawing requests with intermediate drawing syncs */
      for (i = 0; i < many; i++)
        {
          bairo_paint (cr);
          bdk_test_render_sync (darea->window);
        }
      g_timer_stop (gtimer);
      sync_time = g_timer_elapsed (gtimer, NULL);
      sync_is_slower += sync_time > nosync_time * 1.5;
    }
  g_timer_destroy (gtimer);
  g_assert (sync_is_slower > 0);
}

static void
test_spin_button_arrows (void)
{
  BtkWidget *window = btk_test_create_simple_window ("Test Window", "Test: test_spin_button_arrows");
  BtkWidget *spinner = btk_spin_button_new_with_range (0, 100, 5);
  bboolean simsuccess;
  double oldval, newval;
  btk_container_add (BTK_CONTAINER (BTK_BIN (window)->child), spinner);
  btk_widget_show (spinner);
  btk_widget_show_now (window);
  btk_test_slider_set_perc (spinner, 0);
  /* check initial spinner value */
  oldval = btk_test_slider_get_value (spinner);
  g_assert (oldval == 0);
  /* check simple increment */
  simsuccess = btk_test_spin_button_click (BTK_SPIN_BUTTON (spinner), 1, TRUE);
  g_assert (simsuccess == TRUE);
  while (btk_events_pending ()) /* let spin button timeout/idle handlers update */
    btk_main_iteration ();
  newval = btk_test_slider_get_value (spinner);
  g_assert (newval > oldval);
  /* check maximum warp */
  simsuccess = btk_test_spin_button_click (BTK_SPIN_BUTTON (spinner), 3, TRUE);
  g_assert (simsuccess == TRUE);
  while (btk_events_pending ()) /* let spin button timeout/idle handlers update */
    btk_main_iteration ();
  oldval = btk_test_slider_get_value (spinner);
  g_assert (oldval == 100);
  /* check simple decrement */
  oldval = btk_test_slider_get_value (spinner);
  simsuccess = btk_test_spin_button_click (BTK_SPIN_BUTTON (spinner), 1, FALSE);
  g_assert (simsuccess == TRUE);
  while (btk_events_pending ()) /* let spin button timeout/idle handlers update */
    btk_main_iteration ();
  newval = btk_test_slider_get_value (spinner);
  g_assert (newval < oldval);
  /* check minimum warp */
  simsuccess = btk_test_spin_button_click (BTK_SPIN_BUTTON (spinner), 3, FALSE);
  g_assert (simsuccess == TRUE);
  while (btk_events_pending ()) /* let spin button timeout/idle handlers update */
    btk_main_iteration ();
  oldval = btk_test_slider_get_value (spinner);
  g_assert (oldval == 0);
}

static void
test_statusbar_remove_all (void)
{
  BtkWidget *statusbar;

  g_test_bug ("640487");

  statusbar = btk_statusbar_new ();
  g_object_ref_sink (statusbar);

  btk_statusbar_push (BTK_STATUSBAR (statusbar), 1, "bla");
  btk_statusbar_push (BTK_STATUSBAR (statusbar), 1, "bla");
  btk_statusbar_remove_all (BTK_STATUSBAR (statusbar), 1);

  g_object_unref (statusbar);
}

int
main (int   argc,
      char *argv[])
{
  btk_test_init (&argc, &argv);
  g_test_bug_base ("http://bugzilla.bunny.org/");
  btk_test_register_all_types();

  g_test_add_func ("/tests/statusbar-remove-all", test_statusbar_remove_all);
  g_test_add_func ("/ui-tests/text-access", test_text_access);
  g_test_add_func ("/ui-tests/button-clicks", test_button_clicks);
  g_test_add_func ("/ui-tests/keys-events", test_button_keys);
  g_test_add_func ("/ui-tests/send-shift-key", test_send_shift_key);
  g_test_add_func ("/ui-tests/slider-ranges", test_slider_ranges);
  g_test_add_func ("/ui-tests/xserver-sync", test_xserver_sync);
  g_test_add_func ("/ui-tests/spin-button-arrows", test_spin_button_arrows);

  return g_test_run();
}
