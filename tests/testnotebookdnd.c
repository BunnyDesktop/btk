/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* 
 * BTK - The GIMP Toolkit
 * Copyright (C) 2006  Carlos Garnacho Parro <carlosg@bunny.org>
 *
 * All rights reserved.
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

enum {
  PACK_START,
  PACK_END,
  PACK_ALTERNATE
};

static bpointer GROUP_A = "GROUP_A";
static bpointer GROUP_B = "GROUP_B";

bchar *tabs1 [] = {
  "aaaaaaaaaa",
  "bbbbbbbbbb",
  "cccccccccc",
  "dddddddddd",
  NULL
};

bchar *tabs2 [] = {
  "1",
  "2",
  "3",
  "4",
  "55555",
  NULL
};

bchar *tabs3 [] = {
  "foo",
  "bar",
  NULL
};

bchar *tabs4 [] = {
  "beer",
  "water",
  "lemonade",
  "coffee",
  "tea",
  NULL
};

static const BtkTargetEntry button_targets[] = {
  { "BTK_NOTEBOOK_TAB", BTK_TARGET_SAME_APP, 0 },
};

static BtkNotebook*
window_creation_function (BtkNotebook *source_notebook,
			  BtkWidget   *child,
			  bint         x,
			  bint         y,
			  bpointer     data)
{
  BtkWidget *window, *notebook;

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  notebook = btk_notebook_new ();
  g_signal_connect (notebook, "create-window",
                    G_CALLBACK (window_creation_function), NULL);

  btk_notebook_set_group_name (BTK_NOTEBOOK (notebook),
			  btk_notebook_get_group_name (source_notebook));

  btk_container_add (BTK_CONTAINER (window), notebook);

  btk_window_set_default_size (BTK_WINDOW (window), 300, 300);
  btk_window_move (BTK_WINDOW (window), x, y);
  btk_widget_show_all (window);

  return BTK_NOTEBOOK (notebook);
}

static void
on_page_reordered (BtkNotebook *notebook, BtkWidget *child, buint page_num, bpointer data)
{
  g_print ("page %d reordered\n", page_num);
}

static void
on_notebook_drag_begin (BtkWidget      *widget,
			BdkDragContext *context,
			bpointer        data)
{
  BdkPixbuf *pixbuf;
  buint page_num;

  page_num = btk_notebook_get_current_page (BTK_NOTEBOOK (widget));

  if (page_num > 2)
    {
      pixbuf = btk_widget_render_icon (widget,
  				   (page_num % 2) ? BTK_STOCK_HELP : BTK_STOCK_STOP,
				   BTK_ICON_SIZE_DND, NULL);

      btk_drag_set_icon_pixbuf (context, pixbuf, 0, 0);
      g_object_unref (pixbuf);
    }
}

static void
on_button_drag_data_received (BtkWidget        *widget,
			      BdkDragContext   *context,
			      bint              x,
			      bint              y,
			      BtkSelectionData *data,
			      buint             info,
			      buint             time,
			      bpointer          user_data)
{
  BtkWidget *source, *tab_label;
  BtkWidget **child;

  source = btk_drag_get_source_widget (context);
  child = (void*) data->data;

  tab_label = btk_notebook_get_tab_label (BTK_NOTEBOOK (source), *child);
  g_print ("Removing tab: %s\n", btk_label_get_text (BTK_LABEL (tab_label)));

  btk_container_remove (BTK_CONTAINER (source), *child);
}

static BtkWidget*
create_notebook (bchar           **labels,
		 bpointer          group,
		 bint              packing,
		 BtkPositionType   pos)
{
  BtkWidget *notebook, *title, *page;
  bint count = 0;

  notebook = btk_notebook_new ();
  g_signal_connect (notebook, "create-window",
                    G_CALLBACK (window_creation_function), NULL);

  btk_notebook_set_tab_pos (BTK_NOTEBOOK (notebook), pos);
  btk_notebook_set_scrollable (BTK_NOTEBOOK (notebook), TRUE);
  btk_container_set_border_width (BTK_CONTAINER (notebook), 6);
  btk_notebook_set_group_name (BTK_NOTEBOOK (notebook), group);

  while (*labels)
    {
      page = btk_entry_new ();
      btk_entry_set_text (BTK_ENTRY (page), *labels);

      title = btk_label_new (*labels);

      btk_notebook_append_page (BTK_NOTEBOOK (notebook), page, title);
      btk_notebook_set_tab_reorderable (BTK_NOTEBOOK (notebook), page, TRUE);
      btk_notebook_set_tab_detachable (BTK_NOTEBOOK (notebook), page, TRUE);

      if (packing == PACK_END ||
	  (packing == PACK_ALTERNATE && count % 2 == 1))
	btk_container_child_set (BTK_CONTAINER (notebook), page, "tab-pack", BTK_PACK_END, NULL);

      count++;
      labels++;
    }

  g_signal_connect (BTK_NOTEBOOK (notebook), "page-reordered",
		    G_CALLBACK (on_page_reordered), NULL);
  g_signal_connect_after (B_OBJECT (notebook), "drag-begin",
			  G_CALLBACK (on_notebook_drag_begin), NULL);
  return notebook;
}

static BtkWidget*
create_notebook_with_notebooks (bchar           **labels,
			        bpointer          group,
			        bint              packing,
			        BtkPositionType   pos)
{
  BtkWidget *notebook, *title, *page;
  bint count = 0;

  notebook = btk_notebook_new ();
  g_signal_connect (notebook, "create-window",
                    G_CALLBACK (window_creation_function), NULL);

  btk_notebook_set_tab_pos (BTK_NOTEBOOK (notebook), pos);
  btk_notebook_set_scrollable (BTK_NOTEBOOK (notebook), TRUE);
  btk_container_set_border_width (BTK_CONTAINER (notebook), 6);
  btk_notebook_set_group_name (BTK_NOTEBOOK (notebook), group);

  while (*labels)
    {
      page = create_notebook (labels, group, packing, pos);
      btk_notebook_popup_enable (BTK_NOTEBOOK (page));
      
      title = btk_label_new (*labels);

      btk_notebook_append_page (BTK_NOTEBOOK (notebook), page, title);
      btk_notebook_set_tab_reorderable (BTK_NOTEBOOK (notebook), page, TRUE);
      btk_notebook_set_tab_detachable (BTK_NOTEBOOK (notebook), page, TRUE);
      
      if (packing == PACK_END ||
	  (packing == PACK_ALTERNATE && count % 2 == 1))
	btk_container_child_set (BTK_CONTAINER (notebook), page, "tab-pack", BTK_PACK_END, NULL);

      count++;
      labels++;
    }

  g_signal_connect (BTK_NOTEBOOK (notebook), "page-reordered",
		    G_CALLBACK (on_page_reordered), NULL);
  g_signal_connect_after (B_OBJECT (notebook), "drag-begin",
			  G_CALLBACK (on_notebook_drag_begin), NULL);
  return notebook;
}

static BtkWidget*
create_trash_button (void)
{
  BtkWidget *button;

  button = btk_button_new_from_stock (BTK_STOCK_DELETE);

  btk_drag_dest_set (button,
		     BTK_DEST_DEFAULT_MOTION | BTK_DEST_DEFAULT_DROP,
		     button_targets,
		     G_N_ELEMENTS (button_targets),
		     BDK_ACTION_MOVE);

  g_signal_connect_after (B_OBJECT (button), "drag-data-received",
			  G_CALLBACK (on_button_drag_data_received), NULL);
  return button;
}

bint
main (bint argc, bchar *argv[])
{
  BtkWidget *window, *table;

  btk_init (&argc, &argv);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  table = btk_table_new (3, 2, FALSE);

  btk_table_attach_defaults (BTK_TABLE (table),
			     create_notebook (tabs1, GROUP_A, PACK_ALTERNATE, BTK_POS_TOP),
			     0, 1, 0, 1);

  btk_table_attach_defaults (BTK_TABLE (table),
			     create_notebook (tabs2, GROUP_B, PACK_ALTERNATE, BTK_POS_BOTTOM),
			     0, 1, 1, 2);

  btk_table_attach_defaults (BTK_TABLE (table),
			     create_notebook (tabs3, GROUP_B, PACK_END, BTK_POS_LEFT),
			     1, 2, 0, 1);

  btk_table_attach_defaults (BTK_TABLE (table),
			     create_notebook_with_notebooks (tabs4, GROUP_A, PACK_ALTERNATE, BTK_POS_RIGHT),
			     1, 2, 1, 2);

  btk_table_attach (BTK_TABLE (table),
		    create_trash_button (), 1, 2, 2, 3,
		    BTK_EXPAND | BTK_FILL, BTK_SHRINK, 0, 0);

  btk_container_add (BTK_CONTAINER (window), table);
  btk_window_set_default_size (BTK_WINDOW (window), 400, 400);
  btk_widget_show_all (window);

  btk_main ();

  return 0;
}
