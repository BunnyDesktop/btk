/* BTK - The GIMP Toolkit
 * btkrecentmanager.c: a manager for the recently used resources
 *
 * Copyright (C) 2006 Emmanuele Bassi
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

#include <bunnylib/gstdio.h>
#include <btk/btk.h>

const bchar *uri = "file:///tmp/testrecentchooser.txt";
const bchar *uri2 = "file:///tmp/testrecentchooser2.txt";

static void
recent_manager_get_default (void)
{
  BtkRecentManager *manager;
  BtkRecentManager *manager2;

  manager = btk_recent_manager_get_default ();
  g_assert (manager != NULL);

  manager2 = btk_recent_manager_get_default ();
  g_assert (manager == manager2);
}

static void
recent_manager_add (void)
{
  BtkRecentManager *manager;
  BtkRecentData *recent_data;
  bboolean res;

  manager = btk_recent_manager_get_default ();

  recent_data = g_slice_new0 (BtkRecentData);

  /* mime type is mandatory */
  recent_data->mime_type = NULL;
  recent_data->app_name = "testrecentchooser";
  recent_data->app_exec = "testrecentchooser %u";
  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR))
    {
      res = btk_recent_manager_add_full (manager,
                                         uri,
                                         recent_data);
    }
  g_test_trap_assert_failed ();

  /* app name is mandatory */
  recent_data->mime_type = "text/plain";
  recent_data->app_name = NULL;
  recent_data->app_exec = "testrecentchooser %u";
  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR))
    {
      res = btk_recent_manager_add_full (manager,
                                         uri,
                                         recent_data);
    }
  g_test_trap_assert_failed ();

  /* app exec is mandatory */
  recent_data->mime_type = "text/plain";
  recent_data->app_name = "testrecentchooser";
  recent_data->app_exec = NULL;
  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR))
    {
      res = btk_recent_manager_add_full (manager,
                                         uri,
                                         recent_data);
    }
  g_test_trap_assert_failed ();

  recent_data->mime_type = "text/plain";
  recent_data->app_name = "testrecentchooser";
  recent_data->app_exec = "testrecentchooser %u";
  res = btk_recent_manager_add_full (manager,
                                     uri,
                                     recent_data);
  g_assert (res == TRUE);

  g_slice_free (BtkRecentData, recent_data);
}

typedef struct {
  GMainLoop *main_loop;
  bint counter;
} AddManyClosure;

static void
check_bulk (BtkRecentManager *manager,
            bpointer          data)
{
  AddManyClosure *closure = data;

  if (g_test_verbose ())
    g_print (B_STRLOC ": counter = %d\n", closure->counter);

  g_assert_cmpint (closure->counter, ==, 100);

  if (g_main_loop_is_running (closure->main_loop))
    g_main_loop_quit (closure->main_loop);
}

static void
recent_manager_add_many (void)
{
  BtkRecentManager *manager = g_object_new (BTK_TYPE_RECENT_MANAGER,
                                            "filename", "recently-used.xbel",
                                            NULL);
  AddManyClosure *closure = g_new (AddManyClosure, 1);
  BtkRecentData *data = g_slice_new0 (BtkRecentData);
  bint i;

  closure->main_loop = g_main_loop_new (NULL, FALSE);
  closure->counter = 0;

  g_signal_connect (manager, "changed", G_CALLBACK (check_bulk), closure);

  for (i = 0; i < 100; i++)
    {
      bchar *new_uri;

      data->mime_type = "text/plain";
      data->app_name = "testrecentchooser";
      data->app_exec = "testrecentchooser %u";

      if (g_test_verbose ())
        g_print (B_STRLOC ": adding item %d\n", i);

      new_uri = g_strdup_printf ("file:///doesnotexist-%d.txt", i);
      btk_recent_manager_add_full (manager, new_uri, data);
      g_free (new_uri);

      closure->counter += 1;
    }

  g_main_loop_run (closure->main_loop);

  g_main_loop_unref (closure->main_loop);
  g_slice_free (BtkRecentData, data);
  g_free (closure);
  g_object_unref (manager);

  g_assert_cmpint (g_unlink ("recently-used.xbel"), ==, 0);
}

static void
recent_manager_has_item (void)
{
  BtkRecentManager *manager;
  bboolean res;

  manager = btk_recent_manager_get_default ();

  res = btk_recent_manager_has_item (manager, "file:///tmp/testrecentdoesnotexist.txt");
  g_assert (res == FALSE);

  res = btk_recent_manager_has_item (manager, uri);
  g_assert (res == TRUE);
}

static void
recent_manager_move_item (void)
{
  BtkRecentManager *manager;
  bboolean res;
  GError *error;

  manager = btk_recent_manager_get_default ();

  error = NULL;
  res = btk_recent_manager_move_item (manager,
                                      "file:///tmp/testrecentdoesnotexist.txt",
                                      uri2,
                                      &error);
  g_assert (res == FALSE);
  g_assert (error != NULL);
  g_assert (error->domain == BTK_RECENT_MANAGER_ERROR);
  g_assert (error->code == BTK_RECENT_MANAGER_ERROR_NOT_FOUND);
  g_error_free (error);

  error = NULL;
  res = btk_recent_manager_move_item (manager, uri, uri2, &error);
  g_assert (res == TRUE);
  g_assert (error == NULL);

  res = btk_recent_manager_has_item (manager, uri);
  g_assert (res == FALSE);

  res = btk_recent_manager_has_item (manager, uri2);
  g_assert (res == TRUE);
}

static void
recent_manager_lookup_item (void)
{
  BtkRecentManager *manager;
  BtkRecentInfo *info;
  GError *error;

  manager = btk_recent_manager_get_default ();

  error = NULL;
  info = btk_recent_manager_lookup_item (manager,
                                         "file:///tmp/testrecentdoesnotexist.txt",
                                         &error);
  g_assert (info == NULL);
  g_assert (error != NULL);
  g_assert (error->domain == BTK_RECENT_MANAGER_ERROR);
  g_assert (error->code == BTK_RECENT_MANAGER_ERROR_NOT_FOUND);
  g_error_free (error);

  error = NULL;
  info = btk_recent_manager_lookup_item (manager, uri2, &error);
  g_assert (info != NULL);
  g_assert (error == NULL);

  g_assert (btk_recent_info_has_application (info, "testrecentchooser"));

  btk_recent_info_unref (info);
}

static void
recent_manager_remove_item (void)
{
  BtkRecentManager *manager;
  bboolean res;
  GError *error;

  manager = btk_recent_manager_get_default ();

  error = NULL;
  res = btk_recent_manager_remove_item (manager,
                                        "file:///tmp/testrecentdoesnotexist.txt",
                                        &error);
  g_assert (res == FALSE);
  g_assert (error != NULL);
  g_assert (error->domain == BTK_RECENT_MANAGER_ERROR);
  g_assert (error->code == BTK_RECENT_MANAGER_ERROR_NOT_FOUND);
  g_error_free (error);

  /* remove an item that's actually there */
  error = NULL;
  res = btk_recent_manager_remove_item (manager, uri2, &error);
  g_assert (res == TRUE);
  g_assert (error == NULL);

  res = btk_recent_manager_has_item (manager, uri2);
  g_assert (res == FALSE);
}

static void
recent_manager_purge (void)
{
  BtkRecentManager *manager;
  BtkRecentData *recent_data;
  bint n;
  GError *error;

  manager = btk_recent_manager_get_default ();

  /* purge, add 1, purge again and check that 1 item has been purged */
  error = NULL;
  n = btk_recent_manager_purge_items (manager, &error);
  g_assert (error == NULL);

  recent_data = g_slice_new0 (BtkRecentData);
  recent_data->mime_type = "text/plain";
  recent_data->app_name = "testrecentchooser";
  recent_data->app_exec = "testrecentchooser %u";
  btk_recent_manager_add_full (manager, uri, recent_data);
  g_slice_free (BtkRecentData, recent_data);

  error = NULL;
  n = btk_recent_manager_purge_items (manager, &error);
  g_assert (error == NULL);
  g_assert (n == 1);
}

int
main (int    argc,
      char **argv)
{
  btk_test_init (&argc, &argv, NULL);

  g_test_add_func ("/recent-manager/get-default", recent_manager_get_default);
  g_test_add_func ("/recent-manager/add", recent_manager_add);
  g_test_add_func ("/recent-manager/add-many", recent_manager_add_many);
  g_test_add_func ("/recent-manager/has-item", recent_manager_has_item);
  g_test_add_func ("/recent-manager/move-item", recent_manager_move_item);
  g_test_add_func ("/recent-manager/lookup-item", recent_manager_lookup_item);
  g_test_add_func ("/recent-manager/remove-item", recent_manager_remove_item);
  g_test_add_func ("/recent-manager/purge", recent_manager_purge);

  return g_test_run ();
}
