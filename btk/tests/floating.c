/* floatingtest.c - test floating flag uses
 * Copyright (C) 2005 Tim Janik
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
#undef BTK_DISABLE_DEPRECATED
#include "../btk/btk.h"

static bboolean destroyed = FALSE;
static void
destroy (void)
{
  destroyed = TRUE;
}

static void
floating_tests (void)
{
  BtkWidget *widget = g_object_new (BTK_TYPE_LABEL, NULL);
  g_object_connect (widget, "signal::destroy", destroy, NULL, NULL);

  g_assert (BTK_OBJECT_FLOATING (widget));
  g_assert (g_object_is_floating (widget));

  BTK_OBJECT_UNSET_FLAGS (widget, BTK_FLOATING);
  g_assert (!BTK_OBJECT_FLOATING (widget));
  g_assert (!g_object_is_floating (widget));

  BTK_OBJECT_SET_FLAGS (widget, BTK_FLOATING);
  g_assert (BTK_OBJECT_FLOATING (widget));
  g_assert (g_object_is_floating (widget));

  g_object_ref_sink (widget);
  g_assert (!BTK_OBJECT_FLOATING (widget));
  g_assert (!g_object_is_floating (widget));

  g_object_force_floating (B_OBJECT (widget));
  g_assert (BTK_OBJECT_FLOATING (widget));
  g_assert (g_object_is_floating (widget));

  g_object_ref (widget);
  btk_object_sink (BTK_OBJECT (widget));
  g_assert (!BTK_OBJECT_FLOATING (widget));
  g_assert (!g_object_is_floating (widget));

  g_assert (!destroyed);
  g_object_unref (widget);
  g_assert (destroyed);
}

int
main (int   argc,
      char *argv[])
{
  btk_test_init (&argc, &argv);
  g_test_add_func ("/floatingtest", floating_tests);
  return g_test_run();
}
