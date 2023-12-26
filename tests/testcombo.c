/* testcombo.c
 * Copyright (C) 2003  Kristian Rietveld
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

#include <string.h>
#include <stdio.h>

/**
 * oh yes, this test app surely has a lot of ugly code
 */

/* grid combo demo */
static BdkPixbuf *
create_color_pixbuf (const char *color)
{
        BdkPixbuf *pixbuf;
        BdkColor col;

        int x;
        int num;
        guchar *pixels, *p;

        if (!bdk_color_parse (color, &col))
                return NULL;

        pixbuf = bdk_pixbuf_new (BDK_COLORSPACE_RGB,
                                 FALSE, 8,
                                 16, 16);

        p = pixels = bdk_pixbuf_get_pixels (pixbuf);

        num = bdk_pixbuf_get_width (pixbuf) *
                bdk_pixbuf_get_height (pixbuf);

        for (x = 0; x < num; x++) {
                p[0] = col.red / 65535.0 * 255;
                p[1] = col.green / 65535.0 * 255;
                p[2] = col.blue / 65535.0 * 255;
                p += 3;
        }

        return pixbuf;
}

static BtkWidget *
create_combo_box_grid_demo (void)
{
        BtkWidget *combo;
        BtkTreeIter iter;
        BdkPixbuf *pixbuf;
        BtkCellRenderer *cell = btk_cell_renderer_pixbuf_new ();
        BtkListStore *store;

        store = btk_list_store_new (3, BDK_TYPE_PIXBUF, B_TYPE_INT, B_TYPE_INT);

        /* first row */
        pixbuf = create_color_pixbuf ("red");
        btk_list_store_append (store, &iter);
        btk_list_store_set (store, &iter,
                            0, pixbuf,
                            1, 1, /* row span */
                            2, 1, /* column span */
                            -1);
        g_object_unref (pixbuf);

        pixbuf = create_color_pixbuf ("green");
        btk_list_store_append (store, &iter);
        btk_list_store_set (store, &iter,
                            0, pixbuf,
                            1, 1,
                            2, 1,
                            -1);
        g_object_unref (pixbuf);

        pixbuf = create_color_pixbuf ("blue");
        btk_list_store_append (store, &iter);
        btk_list_store_set (store, &iter,
                            0, pixbuf,
                            1, 1,
                            2, 1,
                            -1);
        g_object_unref (pixbuf);

        /* second row */
        pixbuf = create_color_pixbuf ("yellow");
        btk_list_store_append (store, &iter);
        btk_list_store_set (store, &iter,
                            0, pixbuf,
                            1, 1,
                            2, 2, /* Span 2 columns */
                            -1);
        g_object_unref (pixbuf);

        pixbuf = create_color_pixbuf ("black");
        btk_list_store_append (store, &iter);
        btk_list_store_set (store, &iter,
                            0, pixbuf,
                            1, 2, /* Span 2 rows */
                            2, 1,
                            -1);
        g_object_unref (pixbuf);

        /* third row */
        pixbuf = create_color_pixbuf ("gray");
        btk_list_store_append (store, &iter);
        btk_list_store_set (store, &iter,
                            0, pixbuf,
                            1, 1,
                            2, 1,
                            -1);
        g_object_unref (pixbuf);

        pixbuf = create_color_pixbuf ("magenta");
        btk_list_store_append (store, &iter);
        btk_list_store_set (store, &iter,
                            0, pixbuf,
                            1, 1,
                            2, 1,
                            -1);
        g_object_unref (pixbuf);

        /* Create ComboBox after model to avoid btk_menu_attach() warnings(?) */
        combo = btk_combo_box_new_with_model (BTK_TREE_MODEL (store));
        g_object_unref (store);

        btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combo),
                                    cell, TRUE);
        btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (combo),
                                        cell, "pixbuf", 0, NULL);

        /* Set wrap-width != 0 to enforce grid mode */
        btk_combo_box_set_wrap_width (BTK_COMBO_BOX (combo), 3);
        btk_combo_box_set_row_span_column (BTK_COMBO_BOX (combo), 1);
        btk_combo_box_set_column_span_column (BTK_COMBO_BOX (combo), 2);

        btk_combo_box_set_active (BTK_COMBO_BOX (combo), 0);

        return combo;
}

/* blaat */
static BtkTreeModel *
create_tree_blaat (void)
{
        BdkPixbuf *pixbuf;
        BtkWidget *cellview;
        BtkTreeIter iter, iter2;
        BtkTreeStore *store;

        cellview = btk_cell_view_new ();

	store = btk_tree_store_new (3, BDK_TYPE_PIXBUF, B_TYPE_STRING, B_TYPE_BOOLEAN);

        pixbuf = btk_widget_render_icon (cellview, BTK_STOCK_DIALOG_WARNING,
                                         BTK_ICON_SIZE_BUTTON, NULL);
        btk_tree_store_append (store, &iter, NULL);
        btk_tree_store_set (store, &iter,
                            0, pixbuf,
                            1, "btk-stock-dialog-warning",
			    2, FALSE,
                            -1);

        pixbuf = btk_widget_render_icon (cellview, BTK_STOCK_STOP,
                                         BTK_ICON_SIZE_BUTTON, NULL);
        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, pixbuf,
                            1, "btk-stock-stop",
			    2, FALSE,
                            -1);

        pixbuf = btk_widget_render_icon (cellview, BTK_STOCK_NEW,
                                         BTK_ICON_SIZE_BUTTON, NULL);
        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, pixbuf,
                            1, "btk-stock-new",
			    2, FALSE,
                            -1);

        pixbuf = btk_widget_render_icon (cellview, BTK_STOCK_CLEAR,
                                         BTK_ICON_SIZE_BUTTON, NULL);
        btk_tree_store_append (store, &iter, NULL);
        btk_tree_store_set (store, &iter,
                            0, pixbuf,
                            1, "btk-stock-clear",
			    2, FALSE,
                            -1);

#if 0
        btk_tree_store_append (store, &iter, NULL);
        btk_tree_store_set (store, &iter,
                            0, NULL,
                            1, "separator",
			    2, TRUE,
                            -1);
#endif

        pixbuf = btk_widget_render_icon (cellview, BTK_STOCK_OPEN,
                                         BTK_ICON_SIZE_BUTTON, NULL);
        btk_tree_store_append (store, &iter, NULL);
        btk_tree_store_set (store, &iter,
                            0, pixbuf,
                            1, "btk-stock-open",
			    2, FALSE,
                            -1);

        btk_widget_destroy (cellview);

        return BTK_TREE_MODEL (store);
}

static BtkTreeModel *
create_empty_list_blaat (void)
{
        BdkPixbuf *pixbuf;
        BtkWidget *cellview;
        BtkTreeIter iter;
        BtkListStore *store;

        cellview = btk_cell_view_new ();

        store = btk_list_store_new (2, BDK_TYPE_PIXBUF, B_TYPE_STRING);

        pixbuf = btk_widget_render_icon (cellview, BTK_STOCK_DIALOG_WARNING,
                                         BTK_ICON_SIZE_BUTTON, NULL);
        btk_list_store_append (store, &iter);
        btk_list_store_set (store, &iter,
                            0, pixbuf,
                            1, "btk-stock-dialog-warning",
                            -1);

        btk_widget_destroy (cellview);

        return BTK_TREE_MODEL (store);
}

static void
populate_list_blaat (gpointer data)
{
  BtkComboBox *combo_box = BTK_COMBO_BOX (data);
  BtkListStore *store;
  BdkPixbuf *pixbuf;
  BtkWidget *cellview;
  BtkTreeIter iter;
  
  store = BTK_LIST_STORE (btk_combo_box_get_model (combo_box));

  btk_tree_model_get_iter_first (BTK_TREE_MODEL (store), &iter);

  if (btk_tree_model_iter_next (BTK_TREE_MODEL (store), &iter))
    return;

  cellview = btk_cell_view_new ();
  
  pixbuf = btk_widget_render_icon (cellview, BTK_STOCK_STOP,
				   BTK_ICON_SIZE_BUTTON, NULL);
  btk_list_store_append (store, &iter);			       
  btk_list_store_set (store, &iter,
		      0, pixbuf,
		      1, "btk-stock-stop",
		      -1);
  
  pixbuf = btk_widget_render_icon (cellview, BTK_STOCK_NEW,
				   BTK_ICON_SIZE_BUTTON, NULL);
  btk_list_store_append (store, &iter);			       
  btk_list_store_set (store, &iter,
		      0, pixbuf,
		      1, "btk-stock-new",
		      -1);
  
  pixbuf = btk_widget_render_icon (cellview, BTK_STOCK_CLEAR,
				   BTK_ICON_SIZE_BUTTON, NULL);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter,
		      0, pixbuf,
		      1, "btk-stock-clear",
		      -1);
  
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter,
		      0, NULL,
		      1, "separator",
		      -1);
  
  pixbuf = btk_widget_render_icon (cellview, BTK_STOCK_OPEN,
				   BTK_ICON_SIZE_BUTTON, NULL);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter,
		      0, pixbuf,
		      1, "btk-stock-open",
		      -1);
  
  btk_widget_destroy (cellview);  
}

static BtkTreeModel *
create_list_blaat (void)
{
        BdkPixbuf *pixbuf;
        BtkWidget *cellview;
        BtkTreeIter iter;
        BtkListStore *store;

        cellview = btk_cell_view_new ();

        store = btk_list_store_new (2, BDK_TYPE_PIXBUF, B_TYPE_STRING);

        pixbuf = btk_widget_render_icon (cellview, BTK_STOCK_DIALOG_WARNING,
                                         BTK_ICON_SIZE_BUTTON, NULL);
        btk_list_store_append (store, &iter);
        btk_list_store_set (store, &iter,
                            0, pixbuf,
                            1, "btk-stock-dialog-warning",
                            -1);

        pixbuf = btk_widget_render_icon (cellview, BTK_STOCK_STOP,
                                         BTK_ICON_SIZE_BUTTON, NULL);
        btk_list_store_append (store, &iter);			       
        btk_list_store_set (store, &iter,
                            0, pixbuf,
                            1, "btk-stock-stop",
                            -1);

        pixbuf = btk_widget_render_icon (cellview, BTK_STOCK_NEW,
                                         BTK_ICON_SIZE_BUTTON, NULL);
        btk_list_store_append (store, &iter);			       
        btk_list_store_set (store, &iter,
                            0, pixbuf,
                            1, "btk-stock-new",
                            -1);

        pixbuf = btk_widget_render_icon (cellview, BTK_STOCK_CLEAR,
                                         BTK_ICON_SIZE_BUTTON, NULL);
        btk_list_store_append (store, &iter);
        btk_list_store_set (store, &iter,
                            0, pixbuf,
                            1, "btk-stock-clear",
                            -1);

        btk_list_store_append (store, &iter);
        btk_list_store_set (store, &iter,
                            0, NULL,
                            1, "separator",
                            -1);

        pixbuf = btk_widget_render_icon (cellview, BTK_STOCK_OPEN,
                                         BTK_ICON_SIZE_BUTTON, NULL);
        btk_list_store_append (store, &iter);
        btk_list_store_set (store, &iter,
                            0, pixbuf,
                            1, "btk-stock-open",
                            -1);

        btk_widget_destroy (cellview);

        return BTK_TREE_MODEL (store);
}

/* blaat */
static BtkTreeModel *
create_phylogenetic_tree (void)
{
        BtkTreeIter iter, iter2, iter3;
        BtkTreeStore *store;

	store = btk_tree_store_new (1,B_TYPE_STRING);

        btk_tree_store_append (store, &iter, NULL);
        btk_tree_store_set (store, &iter,
                            0, "Eubacteria",
                            -1);

        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Aquifecales",
                            -1);

        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Thermotogales",
                            -1);

        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Thermodesulfobacterium",
                            -1);

        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Thermus-Deinococcus group",
                            -1);

        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Chloroflecales",
                            -1);

        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Cyanobacteria",
                            -1);

        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Firmicutes",
                            -1);

        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Leptospirillium Group",
                            -1);

        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Synergistes",
                            -1);
        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Chlorobium-Flavobacteria group",
                            -1);
        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Chlamydia-Verrucomicrobia group",
                            -1);

        btk_tree_store_append (store, &iter3, &iter2);			       
        btk_tree_store_set (store, &iter3,
                            0, "Verrucomicrobia",
                            -1);
        btk_tree_store_append (store, &iter3, &iter2);			       
        btk_tree_store_set (store, &iter3,
                            0, "Chlamydia",
                            -1);

        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Flexistipes",
                            -1);


        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Fibrobacter group",
                            -1);


        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "spirocheteus",
                            -1);


        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Proteobacteria",
                            -1);


        btk_tree_store_append (store, &iter3, &iter2);			       
        btk_tree_store_set (store, &iter3,
                            0, "alpha",
                            -1);


        btk_tree_store_append (store, &iter3, &iter2);			       
        btk_tree_store_set (store, &iter3,
                            0, "beta",
                            -1);


        btk_tree_store_append (store, &iter3, &iter2);			       
        btk_tree_store_set (store, &iter3,
                            0, "delta ",
                            -1);


        btk_tree_store_append (store, &iter3, &iter2);			       
        btk_tree_store_set (store, &iter3,
                            0, "epsilon",
                            -1);


        btk_tree_store_append (store, &iter3, &iter2);  
        btk_tree_store_set (store, &iter3,
                            0, "gamma ",
                            -1);


        btk_tree_store_append (store, &iter, NULL);			       
        btk_tree_store_set (store, &iter,
                            0, "Eukaryotes",
                            -1);


        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Metazoa",
                            -1);


        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Bilateria",
                            -1);


        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Myxozoa",
                            -1);


        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Cnidaria",
                            -1);


        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Ctenophora",
                            -1);


        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Placozoa",
                            -1);


        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Porifera",
                            -1);


        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "choanoflagellates",
                            -1);


        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Fungi",
                            -1);


        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Microsporidia",
                            -1);


        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Aleveolates",
                            -1);


        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Stramenopiles",
                            -1);


        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Rhodophyta",
                            -1);


        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Viridaeplantae",
                            -1);


        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "crytomonads et al",
                            -1);


        btk_tree_store_append (store, &iter, NULL);			       
        btk_tree_store_set (store, &iter,
                            0, "Archaea ",
                            -1);


        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Korarchaeota",
                            -1);


        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Crenarchaeota",
                            -1);


        btk_tree_store_append (store, &iter2, &iter);			       
        btk_tree_store_set (store, &iter2,
                            0, "Buryarchaeota",
                            -1);

        return BTK_TREE_MODEL (store);
}


/* blaat */
static BtkTreeModel *
create_capital_tree (void)
{
        BtkTreeIter iter, iter2;
        BtkTreeStore *store;

	store = btk_tree_store_new (1, B_TYPE_STRING);

        btk_tree_store_append (store, &iter, NULL);
        btk_tree_store_set (store, &iter, 0, "A - B", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Albany", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Annapolis", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Atlanta", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Augusta", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Austin", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Baton Rouge", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Bismarck", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Boise", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Boston", -1);

        btk_tree_store_append (store, &iter, NULL);
        btk_tree_store_set (store, &iter, 0, "C - D", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Carson City", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Charleston", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Cheyenne", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Columbia", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Columbus", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Concord", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Denver", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Des Moines", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Dover", -1);


        btk_tree_store_append (store, &iter, NULL);
        btk_tree_store_set (store, &iter, 0, "E - J", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Frankfort", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Harrisburg", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Hartford", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Helena", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Honolulu", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Indianapolis", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Jackson", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Jefferson City", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Juneau", -1);


        btk_tree_store_append (store, &iter, NULL);
        btk_tree_store_set (store, &iter, 0, "K - O", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Lansing", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Lincoln", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Little Rock", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Madison", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Montgomery", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Montpelier", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Nashville", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Oklahoma City", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Olympia", -1);


        btk_tree_store_append (store, &iter, NULL);
        btk_tree_store_set (store, &iter, 0, "P - S", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Phoenix", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Pierre", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Providence", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Raleigh", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Richmond", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Sacramento", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Salem", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Salt Lake City", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Santa Fe", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Springfield", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "St. Paul", -1);


        btk_tree_store_append (store, &iter, NULL);
        btk_tree_store_set (store, &iter, 0, "T - Z", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Tallahassee", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Topeka", -1);

        btk_tree_store_append (store, &iter2, &iter);
        btk_tree_store_set (store, &iter2, 0, "Trenton", -1);

        return BTK_TREE_MODEL (store);
}

static void
capital_sensitive (BtkCellLayout   *cell_layout,
		   BtkCellRenderer *cell,
		   BtkTreeModel    *tree_model,
		   BtkTreeIter     *iter,
		   gpointer         data)
{
  gboolean sensitive;

  sensitive = !btk_tree_model_iter_has_child (tree_model, iter);

  g_object_set (cell, "sensitive", sensitive, NULL);
}

static gboolean
capital_animation (gpointer data)
{
  static gint insert_count = 0;
  BtkTreeModel *model = BTK_TREE_MODEL (data);
  BtkTreePath *path;
  BtkTreeIter iter, parent;

  switch (insert_count % 8)
    {
    case 0:
      btk_tree_store_insert (BTK_TREE_STORE (model), &iter, NULL, 0);
      btk_tree_store_set (BTK_TREE_STORE (model), 
			  &iter,
			  0, "Europe", -1);
      break;

    case 1:
      path = btk_tree_path_new_from_indices (0, -1);
      btk_tree_model_get_iter (model, &parent, path);
      btk_tree_path_free (path);
      btk_tree_store_insert (BTK_TREE_STORE (model), &iter, &parent, 0);
      btk_tree_store_set (BTK_TREE_STORE (model), 
			  &iter,
			  0, "Berlin", -1);
      break;

    case 2:
      path = btk_tree_path_new_from_indices (0, -1);
      btk_tree_model_get_iter (model, &parent, path);
      btk_tree_path_free (path);
      btk_tree_store_insert (BTK_TREE_STORE (model), &iter, &parent, 1);
      btk_tree_store_set (BTK_TREE_STORE (model), 
			  &iter,
			  0, "London", -1);
      break;

    case 3:
      path = btk_tree_path_new_from_indices (0, -1);
      btk_tree_model_get_iter (model, &parent, path);
      btk_tree_path_free (path);
      btk_tree_store_insert (BTK_TREE_STORE (model), &iter, &parent, 2);
      btk_tree_store_set (BTK_TREE_STORE (model), 
			  &iter,
			  0, "Paris", -1);
      break;

    case 4:
      path = btk_tree_path_new_from_indices (0, 2, -1);
      btk_tree_model_get_iter (model, &iter, path);
      btk_tree_path_free (path);
      btk_tree_store_remove (BTK_TREE_STORE (model), &iter);
      break;

    case 5:
      path = btk_tree_path_new_from_indices (0, 1, -1);
      btk_tree_model_get_iter (model, &iter, path);
      btk_tree_path_free (path);
      btk_tree_store_remove (BTK_TREE_STORE (model), &iter);
      break;

    case 6:
      path = btk_tree_path_new_from_indices (0, 0, -1);
      btk_tree_model_get_iter (model, &iter, path);
      btk_tree_path_free (path);
      btk_tree_store_remove (BTK_TREE_STORE (model), &iter);
      break;

    case 7:
      path = btk_tree_path_new_from_indices (0, -1);
      btk_tree_model_get_iter (model, &iter, path);
      btk_tree_path_free (path);
      btk_tree_store_remove (BTK_TREE_STORE (model), &iter);
      break;

    default: ;

    }
  insert_count++;

  return TRUE;
}

static void
setup_combo_entry (BtkComboBoxText *combo)
{
  btk_combo_box_text_append_text (combo,
				   "dum de dum");
  btk_combo_box_text_append_text (combo,
				   "la la la");
  btk_combo_box_text_append_text (combo,
				   "la la la dum de dum la la la la la la boom de da la la");
  btk_combo_box_text_append_text (combo,
				   "bloop");
  btk_combo_box_text_append_text (combo,
				   "bleep");
  btk_combo_box_text_append_text (combo,
				   "klaas");
  btk_combo_box_text_append_text (combo,
				   "klaas0");
  btk_combo_box_text_append_text (combo,
				   "klaas1");
  btk_combo_box_text_append_text (combo,
				   "klaas2");
  btk_combo_box_text_append_text (combo,
				   "klaas3");
  btk_combo_box_text_append_text (combo,
				   "klaas4");
  btk_combo_box_text_append_text (combo,
				   "klaas5");
  btk_combo_box_text_append_text (combo,
				   "klaas6");
  btk_combo_box_text_append_text (combo,
				   "klaas7");
  btk_combo_box_text_append_text (combo,
				   "klaas8");
  btk_combo_box_text_append_text (combo,
				   "klaas9");
  btk_combo_box_text_append_text (combo,
				   "klaasa");
  btk_combo_box_text_append_text (combo,
				   "klaasb");
  btk_combo_box_text_append_text (combo,
				   "klaasc");
  btk_combo_box_text_append_text (combo,
				   "klaasd");
  btk_combo_box_text_append_text (combo,
				   "klaase");
  btk_combo_box_text_append_text (combo,
				   "klaasf");
  btk_combo_box_text_append_text (combo,
				   "klaas10");
  btk_combo_box_text_append_text (combo,
				   "klaas11");
  btk_combo_box_text_append_text (combo,
				   "klaas12");
}

static void
set_sensitive (BtkCellLayout   *cell_layout,
	       BtkCellRenderer *cell,
	       BtkTreeModel    *tree_model,
	       BtkTreeIter     *iter,
	       gpointer         data)
{
  BtkTreePath *path;
  gint *indices;
  gboolean sensitive;

  path = btk_tree_model_get_path (tree_model, iter);
  indices = btk_tree_path_get_indices (path);
  sensitive = indices[0] != 1;
  btk_tree_path_free (path);

  g_object_set (cell, "sensitive", sensitive, NULL);
}

static gboolean
is_separator (BtkTreeModel *model,
	      BtkTreeIter  *iter,
	      gpointer      data)
{
  BtkTreePath *path;
  gboolean result;

  path = btk_tree_model_get_path (model, iter);
  result = btk_tree_path_get_indices (path)[0] == 4;
  btk_tree_path_free (path);

  return result;
  
}

static void
displayed_row_changed (BtkComboBox *combo,
                       BtkCellView *cell)
{
  gint row;
  BtkTreePath *path;

  row = btk_combo_box_get_active (combo);
  path = btk_tree_path_new_from_indices (row, -1);
  btk_cell_view_set_displayed_row (cell, path);
  btk_tree_path_free (path);
}

int
main (int argc, char **argv)
{
        BtkWidget *window, *cellview, *mainbox;
        BtkWidget *combobox, *comboboxtext, *comboboxgrid;
        BtkWidget *tmp, *boom;
        BtkCellRenderer *renderer;
        BdkPixbuf *pixbuf;
        BtkTreeModel *model;
	BtkTreePath *path;
	BtkTreeIter iter;
        BdkColor color;
        gint i;

        btk_init (&argc, &argv);

	if (g_getenv ("RTL"))
	  btk_widget_set_default_direction (BTK_TEXT_DIR_RTL);

        window = btk_window_new (BTK_WINDOW_TOPLEVEL);
        btk_container_set_border_width (BTK_CONTAINER (window), 5);
        g_signal_connect (window, "destroy", btk_main_quit, NULL);

        mainbox = btk_vbox_new (FALSE, 2);
        btk_container_add (BTK_CONTAINER (window), mainbox);

        /* BtkCellView */
        tmp = btk_frame_new ("BtkCellView");
        btk_box_pack_start (BTK_BOX (mainbox), tmp, FALSE, FALSE, 0);

        boom = btk_vbox_new (FALSE, 0);
        btk_container_set_border_width (BTK_CONTAINER (boom), 5);
        btk_container_add (BTK_CONTAINER (tmp), boom);

        cellview = btk_cell_view_new ();
        renderer = btk_cell_renderer_pixbuf_new ();
        pixbuf = btk_widget_render_icon (cellview, BTK_STOCK_DIALOG_WARNING,
                                         BTK_ICON_SIZE_BUTTON, NULL);

        btk_cell_layout_pack_start (BTK_CELL_LAYOUT (cellview),
                                    renderer,
                                    FALSE);
        g_object_set (renderer, "pixbuf", pixbuf, NULL);

        renderer = btk_cell_renderer_text_new ();
        btk_cell_layout_pack_start (BTK_CELL_LAYOUT (cellview),
                                    renderer,
                                    TRUE);
        g_object_set (renderer, "text", "la la la", NULL);
        btk_container_add (BTK_CONTAINER (boom), cellview);

        /* BtkComboBox list */
        tmp = btk_frame_new ("BtkComboBox (list)");
        btk_box_pack_start (BTK_BOX (mainbox), tmp, FALSE, FALSE, 0);

        boom = btk_vbox_new (FALSE, 0);
        btk_container_set_border_width (BTK_CONTAINER (boom), 5);
        btk_container_add (BTK_CONTAINER (tmp), boom);

        model = create_list_blaat ();
        combobox = btk_combo_box_new_with_model (model);
	btk_combo_box_set_add_tearoffs (BTK_COMBO_BOX (combobox), TRUE);
        g_object_unref (model);
        btk_container_add (BTK_CONTAINER (boom), combobox);

        renderer = btk_cell_renderer_pixbuf_new ();
        btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combobox),
                                    renderer,
                                    FALSE);
        btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (combobox), renderer,
                                        "pixbuf", 0,
                                        NULL);
	btk_cell_layout_set_cell_data_func (BTK_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);

        renderer = btk_cell_renderer_text_new ();
        btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combobox),
                                    renderer,
                                    TRUE);
        btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (combobox), renderer,
                                        "text", 1,
                                        NULL);
	btk_cell_layout_set_cell_data_func (BTK_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);
	btk_combo_box_set_row_separator_func (BTK_COMBO_BOX (combobox), 
					      is_separator, NULL, NULL);
						
        btk_combo_box_set_active (BTK_COMBO_BOX (combobox), 0);

        /* BtkComboBox dynamic list */
        tmp = btk_frame_new ("BtkComboBox (dynamic list)");
        btk_box_pack_start (BTK_BOX (mainbox), tmp, FALSE, FALSE, 0);

        boom = btk_vbox_new (FALSE, 0);
        btk_container_set_border_width (BTK_CONTAINER (boom), 5);
        btk_container_add (BTK_CONTAINER (tmp), boom);

        model = create_empty_list_blaat ();
        combobox = btk_combo_box_new_with_model (model);
	g_signal_connect (combobox, "notify::popup-shown", 
			  G_CALLBACK (populate_list_blaat), combobox);

	btk_combo_box_set_add_tearoffs (BTK_COMBO_BOX (combobox), TRUE);
        g_object_unref (model);
        btk_container_add (BTK_CONTAINER (boom), combobox);

        renderer = btk_cell_renderer_pixbuf_new ();
        btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combobox),
                                    renderer,
                                    FALSE);
        btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (combobox), renderer,
                                        "pixbuf", 0,
                                        NULL);
	btk_cell_layout_set_cell_data_func (BTK_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);

        renderer = btk_cell_renderer_text_new ();
        btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combobox),
                                    renderer,
                                    TRUE);
        btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (combobox), renderer,
                                        "text", 1,
                                        NULL);
	btk_cell_layout_set_cell_data_func (BTK_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);
	btk_combo_box_set_row_separator_func (BTK_COMBO_BOX (combobox), 
					      is_separator, NULL, NULL);
						
        btk_combo_box_set_active (BTK_COMBO_BOX (combobox), 0);
	btk_combo_box_set_title (BTK_COMBO_BOX (combobox), "Dynamic list");

        /* BtkComboBox custom entry */
        tmp = btk_frame_new ("BtkComboBox (custom)");
        btk_box_pack_start (BTK_BOX (mainbox), tmp, FALSE, FALSE, 0);

        boom = btk_vbox_new (FALSE, 0);
        btk_container_set_border_width (BTK_CONTAINER (boom), 5);
        btk_container_add (BTK_CONTAINER (tmp), boom);

        model = create_list_blaat ();
        combobox = btk_combo_box_new_with_model (model);
	btk_combo_box_set_add_tearoffs (BTK_COMBO_BOX (combobox), TRUE);
        g_object_unref (model);
        btk_container_add (BTK_CONTAINER (boom), combobox);

        renderer = btk_cell_renderer_pixbuf_new ();
        btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combobox),
                                    renderer,
                                    FALSE);
        btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (combobox), renderer,
                                        "pixbuf", 0,
                                        NULL);
	btk_cell_layout_set_cell_data_func (BTK_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);

        renderer = btk_cell_renderer_text_new ();
        btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combobox),
                                    renderer,
                                    TRUE);
        btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (combobox), renderer,
                                        "text", 1,
                                        NULL);
	btk_cell_layout_set_cell_data_func (BTK_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);
	btk_combo_box_set_row_separator_func (BTK_COMBO_BOX (combobox), 
					      is_separator, NULL, NULL);
						
        btk_combo_box_set_active (BTK_COMBO_BOX (combobox), 0);

        tmp = btk_cell_view_new ();
        btk_widget_show (tmp);
        btk_cell_view_set_model (BTK_CELL_VIEW (tmp), model);

        renderer = btk_cell_renderer_text_new ();
        btk_cell_layout_pack_start (BTK_CELL_LAYOUT (tmp), renderer, TRUE);
        btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (tmp), renderer,
                                        "text", 1,
                                        NULL);
        color.red = 0xffff;
        color.blue = 0xffff;
        color.green = 0;
        btk_cell_view_set_background_color (BTK_CELL_VIEW (tmp), &color);
        displayed_row_changed (BTK_COMBO_BOX (combobox), BTK_CELL_VIEW (tmp));
        g_signal_connect (combobox, "changed", G_CALLBACK (displayed_row_changed), tmp); 
           
        btk_container_add (BTK_CONTAINER (combobox), tmp);

        /* BtkComboBox tree */
        tmp = btk_frame_new ("BtkComboBox (tree)");
        btk_box_pack_start (BTK_BOX (mainbox), tmp, FALSE, FALSE, 0);

        boom = btk_vbox_new (FALSE, 0);
        btk_container_set_border_width (BTK_CONTAINER (boom), 5);
        btk_container_add (BTK_CONTAINER (tmp), boom);

        model = create_tree_blaat ();
        combobox = btk_combo_box_new_with_model (model);
	btk_combo_box_set_add_tearoffs (BTK_COMBO_BOX (combobox), TRUE);
        g_object_unref (model);
        btk_container_add (BTK_CONTAINER (boom), combobox);

        renderer = btk_cell_renderer_pixbuf_new ();
        btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combobox),
                                    renderer,
                                    FALSE);
        btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (combobox), renderer,
                                        "pixbuf", 0,
                                        NULL);
	btk_cell_layout_set_cell_data_func (BTK_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);

        renderer = btk_cell_renderer_text_new ();
        btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combobox),
                                    renderer,
                                    TRUE);
        btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (combobox), renderer,
                                        "text", 1,
                                        NULL);
	btk_cell_layout_set_cell_data_func (BTK_CELL_LAYOUT (combobox),
					    renderer,
					    set_sensitive,
					    NULL, NULL);
	btk_combo_box_set_row_separator_func (BTK_COMBO_BOX (combobox), 
					      is_separator, NULL, NULL);
						
        btk_combo_box_set_active (BTK_COMBO_BOX (combobox), 0);
#if 0
	g_timeout_add (1000, (GSourceFunc) animation_timer, model);
#endif

        /* BtkComboBox (grid mode) */
        tmp = btk_frame_new ("BtkComboBox (grid mode)");
        btk_box_pack_start (BTK_BOX (mainbox), tmp, FALSE, FALSE, 0);

        boom = btk_vbox_new (FALSE, 0);
        btk_container_set_border_width (BTK_CONTAINER (boom), 5);
        btk_container_add (BTK_CONTAINER (tmp), boom);

        comboboxgrid = create_combo_box_grid_demo ();
        btk_box_pack_start (BTK_BOX (boom), comboboxgrid, FALSE, FALSE, 0);


        /* BtkComboBoxEntry */
        tmp = btk_frame_new ("BtkComboBox with entry");
        btk_box_pack_start (BTK_BOX (mainbox), tmp, FALSE, FALSE, 0);

        boom = btk_vbox_new (FALSE, 0);
        btk_container_set_border_width (BTK_CONTAINER (boom), 5);
        btk_container_add (BTK_CONTAINER (tmp), boom);

        comboboxtext = btk_combo_box_text_new_with_entry ();
        setup_combo_entry (BTK_COMBO_BOX_TEXT (comboboxtext));
        btk_container_add (BTK_CONTAINER (boom), comboboxtext);


        /* Phylogenetic tree */
        tmp = btk_frame_new ("What are you ?");
        btk_box_pack_start (BTK_BOX (mainbox), tmp, FALSE, FALSE, 0);

        boom = btk_vbox_new (FALSE, 0);
        btk_container_set_border_width (BTK_CONTAINER (boom), 5);
        btk_container_add (BTK_CONTAINER (tmp), boom);

        model = create_phylogenetic_tree ();
        combobox = btk_combo_box_new_with_model (model);
	btk_combo_box_set_add_tearoffs (BTK_COMBO_BOX (combobox), TRUE);
        g_object_unref (model);
        btk_container_add (BTK_CONTAINER (boom), combobox);

        renderer = btk_cell_renderer_text_new ();
        btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combobox),
                                    renderer,
                                    TRUE);
        btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (combobox), renderer,
                                        "text", 0,
                                        NULL);
	
        btk_combo_box_set_active (BTK_COMBO_BOX (combobox), 0);


        /* Capitals */
        tmp = btk_frame_new ("Where are you ?");
        btk_box_pack_start (BTK_BOX (mainbox), tmp, FALSE, FALSE, 0);

        boom = btk_vbox_new (FALSE, 0);
        btk_container_set_border_width (BTK_CONTAINER (boom), 5);
        btk_container_add (BTK_CONTAINER (tmp), boom);

        model = create_capital_tree ();
	combobox = btk_combo_box_new_with_model (model);
	btk_combo_box_set_add_tearoffs (BTK_COMBO_BOX (combobox), TRUE);
        g_object_unref (model);
        btk_container_add (BTK_CONTAINER (boom), combobox);
        renderer = btk_cell_renderer_text_new ();
        btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combobox),
                                    renderer,
                                    TRUE);
        btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (combobox), renderer,
                                        "text", 0,
                                        NULL);
	btk_cell_layout_set_cell_data_func (BTK_CELL_LAYOUT (combobox),
					    renderer,
					    capital_sensitive,
					    NULL, NULL);
	path = btk_tree_path_new_from_indices (0, 8, -1);
	btk_tree_model_get_iter (model, &iter, path);
	btk_tree_path_free (path);
        btk_combo_box_set_active_iter (BTK_COMBO_BOX (combobox), &iter);

        tmp = btk_frame_new ("Looong");
        btk_box_pack_start (BTK_BOX (mainbox), tmp, FALSE, FALSE, 0);
        combobox = btk_combo_box_text_new ();
        for (i = 0; i < 200; i++)
          {
            gchar *text = g_strdup_printf ("Item %d", i);
            btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combobox), text);
            g_free (text);
          }
        btk_combo_box_set_active (BTK_COMBO_BOX (combobox), 53);
        btk_container_add (BTK_CONTAINER (tmp), combobox);

#if 1
	bdk_threads_add_timeout (1000, (GSourceFunc) capital_animation, model);
#endif

        btk_widget_show_all (window);

        btk_main ();

        return 0;
}

/* vim:expandtab
 */
