/* BtkAction tests.
 *
 * Authors: Jan Arne Petersen <jpetersen@openismus.com>
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

/* Fixture */

typedef struct
{
  BtkAction *action;
} ActionTest;

static void
action_test_setup (ActionTest    *fixture,
                   gconstpointer  test_data)
{
  fixture->action = btk_action_new ("name", "label", NULL, NULL);
}

static void
action_test_teardown (ActionTest    *fixture,
                      gconstpointer  test_data)
{
  g_object_unref (fixture->action);
}

static void
notify_count_emmisions (BObject    *object,
			BParamSpec *pspec,
			bpointer    data)
{
  unsigned int *i = data;
  (*i)++;
}

static void
menu_item_label_notify_count (ActionTest    *fixture,
                              gconstpointer  test_data)
{
  BtkWidget *item = btk_menu_item_new ();
  unsigned int emmisions = 0;

  g_signal_connect (item, "notify::label",
		    G_CALLBACK (notify_count_emmisions), &emmisions);

  btk_activatable_do_set_related_action (BTK_ACTIVATABLE (item),
					 fixture->action);

  g_assert_cmpuint (emmisions, ==, 1);

  btk_action_set_label (fixture->action, "new label");

  g_assert_cmpuint (emmisions, ==, 2);

  g_object_unref (item);
}

/* main */

int
main (int    argc,
      char **argv)
{
  btk_test_init (&argc, &argv, NULL);

  g_test_add ("/Action/MenuItem/label-notify-count",
              ActionTest, NULL,
              action_test_setup,
              menu_item_label_notify_count,
              action_test_teardown);

  return g_test_run ();
}

