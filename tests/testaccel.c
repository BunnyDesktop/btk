/* btkcellrendereraccel.h
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
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
#include <bdk/bdkkeysyms.h>

static void
accel_edited_callback (BtkCellRendererText *cell,
                       const char          *path_string,
                       buint                keyval,
                       BdkModifierType      mask,
                       buint                hardware_keycode,
                       bpointer             data)
{
  BtkTreeModel *model = (BtkTreeModel *)data;
  BtkTreePath *path = btk_tree_path_new_from_string (path_string);
  BtkTreeIter iter;

  btk_tree_model_get_iter (model, &iter, path);

  g_print ("%u %d %u\n", keyval, mask, hardware_keycode);
  
  btk_list_store_set (BTK_LIST_STORE (model), &iter,
		      0, (bint)mask,
		      1, keyval,
		      -1);
  btk_tree_path_free (path);
}

static BtkWidget *
key_test (void)
{
	BtkWidget *window, *sw, *tv;
	BtkListStore *store;
	BtkTreeViewColumn *column;
	BtkCellRenderer *rend;
	bint i;

	/* create window */
	window = btk_window_new (BTK_WINDOW_TOPLEVEL);


	sw = btk_scrolled_window_new (NULL, NULL);
	btk_container_add (BTK_CONTAINER (window), sw);

	store = btk_list_store_new (2, B_TYPE_INT, B_TYPE_UINT);
	tv = btk_tree_view_new_with_model (BTK_TREE_MODEL (store));
	btk_container_add (BTK_CONTAINER (sw), tv);
	column = btk_tree_view_column_new ();
	rend = btk_cell_renderer_accel_new ();
	g_object_set (B_OBJECT (rend), 
		      "accel-mode", BTK_CELL_RENDERER_ACCEL_MODE_BTK, 
                      "editable", TRUE, 
		      NULL);
	g_signal_connect (B_OBJECT (rend),
			  "accel-edited",
			  G_CALLBACK (accel_edited_callback),
			  store);

	btk_tree_view_column_pack_start (column, rend,
					 TRUE);
	btk_tree_view_column_set_attributes (column, rend,
					     "accel-mods", 0,
					     "accel-key", 1,
					     NULL);
	btk_tree_view_append_column (BTK_TREE_VIEW (tv), column);

	for (i = 0; i < 10; i++) {
		BtkTreeIter iter;

		btk_list_store_append (store, &iter);
	}

	/* done */

	return window;
}

bint
main (bint argc, bchar **argv)
{
  BtkWidget *dialog;
  
  btk_init (&argc, &argv);

  dialog = key_test ();

  btk_widget_show_all (dialog);

  btk_main ();

  return 0;
}
