/* Icon View/Editing and Drag-and-Drop
 *
 * The BtkIconView widget supports Editing and Drag-and-Drop.
 * This example also demonstrates using the generic BtkCellLayout
 * interface to set up cell renderers in an icon view.
 */

#include <btk/btk.h>
#include <string.h>
#include "demo-common.h"

static BtkWidget *window = NULL;

enum
{
  COL_TEXT,
  NUM_COLS
};


static void
fill_store (BtkListStore *store)
{
  BtkTreeIter iter;
  const gchar *text[] = { "Red", "Green", "Blue", "Yellow" };
  gint i;

  /* First clear the store */
  btk_list_store_clear (store);

  for (i = 0; i < 4; i++)
    {
      btk_list_store_append (store, &iter);
      btk_list_store_set (store, &iter, COL_TEXT, text[i], -1);
    }
}

static BtkListStore *
create_store (void)
{
  BtkListStore *store;

  store = btk_list_store_new (NUM_COLS, B_TYPE_STRING);

  return store;
}

static void
set_cell_color (BtkCellLayout   *cell_layout,
		BtkCellRenderer *cell,
		BtkTreeModel    *tree_model,
		BtkTreeIter     *iter,
		gpointer         data)
{
  gchar *text;
  BdkColor color;
  guint32 pixel = 0;
  BdkPixbuf *pixbuf;

  btk_tree_model_get (tree_model, iter, COL_TEXT, &text, -1);
  if (bdk_color_parse (text, &color))
    pixel =
      (color.red   >> 8) << 24 |
      (color.green >> 8) << 16 |
      (color.blue  >> 8) << 8;

  g_free (text);

  pixbuf = bdk_pixbuf_new (BDK_COLORSPACE_RGB, FALSE, 8, 24, 24);
  bdk_pixbuf_fill (pixbuf, pixel);

  g_object_set (cell, "pixbuf", pixbuf, NULL);

  g_object_unref (pixbuf);
}

static void
edited (BtkCellRendererText *cell,
	gchar               *path_string,
	gchar               *text,
	gpointer             data)
{
  BtkTreeModel *model;
  BtkTreeIter iter;
  BtkTreePath *path;

  model = btk_icon_view_get_model (BTK_ICON_VIEW (data));
  path = btk_tree_path_new_from_string (path_string);

  btk_tree_model_get_iter (model, &iter, path);
  btk_list_store_set (BTK_LIST_STORE (model), &iter,
		      COL_TEXT, text, -1);

  btk_tree_path_free (path);
}

BtkWidget *
do_iconview_edit (BtkWidget *do_widget)
{
  if (!window)
    {
      BtkWidget *icon_view;
      BtkListStore *store;
      BtkCellRenderer *renderer;

      window = btk_window_new (BTK_WINDOW_TOPLEVEL);

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (do_widget));
      btk_window_set_title (BTK_WINDOW (window), "Editing and Drag-and-Drop");

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed), &window);

      store = create_store ();
      fill_store (store);

      icon_view = btk_icon_view_new_with_model (BTK_TREE_MODEL (store));
      g_object_unref (store);

      btk_icon_view_set_selection_mode (BTK_ICON_VIEW (icon_view),
					BTK_SELECTION_SINGLE);
      btk_icon_view_set_orientation (BTK_ICON_VIEW (icon_view),
				     BTK_ORIENTATION_HORIZONTAL);
      btk_icon_view_set_columns (BTK_ICON_VIEW (icon_view), 2);
      btk_icon_view_set_reorderable (BTK_ICON_VIEW (icon_view), TRUE);

      renderer = btk_cell_renderer_pixbuf_new ();
      btk_cell_layout_pack_start (BTK_CELL_LAYOUT (icon_view),
				  renderer, TRUE);
      btk_cell_layout_set_cell_data_func (BTK_CELL_LAYOUT (icon_view),
					  renderer,
					  set_cell_color,
					  NULL, NULL);

      renderer = btk_cell_renderer_text_new ();
      btk_cell_layout_pack_start (BTK_CELL_LAYOUT (icon_view),
				  renderer, TRUE);
      g_object_set (renderer, "editable", TRUE, NULL);
      g_signal_connect (renderer, "edited", G_CALLBACK (edited), icon_view);
      btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (icon_view),
				      renderer,
				      "text", COL_TEXT,
				      NULL);

      btk_container_add (BTK_CONTAINER (window), icon_view);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    {
      btk_widget_destroy (window);
      window = NULL;
    }

  return window;
}

