/* testtooltips.c: Test application for BTK+ >= 2.12 tooltips code
 *
 * Copyright (C) 2006-2007  Imendio AB
 * Contact: Kristian Rietveld <kris@imendio.com>
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

#include <btk/btk.h>

static gboolean
query_tooltip_cb (BtkWidget  *widget,
		  gint        x,
		  gint        y,
		  gboolean    keyboard_tip,
		  BtkTooltip *tooltip,
		  gpointer    data)
{
  btk_tooltip_set_markup (tooltip, btk_button_get_label (BTK_BUTTON (widget)));
  btk_tooltip_set_icon_from_stock (tooltip, BTK_STOCK_DELETE,
				   BTK_ICON_SIZE_MENU);

  return TRUE;
}

static gboolean
query_tooltip_custom_cb (BtkWidget  *widget,
			 gint        x,
			 gint        y,
			 gboolean    keyboard_tip,
			 BtkTooltip *tooltip,
			 gpointer    data)
{
  BdkColor color = { 0, 0, 65535 };
  BtkWindow *window = btk_widget_get_tooltip_window (widget);

  btk_widget_modify_bg (BTK_WIDGET (window), BTK_STATE_NORMAL, &color);

  return TRUE;
}

static gboolean
query_tooltip_text_view_cb (BtkWidget  *widget,
			    gint        x,
			    gint        y,
			    gboolean    keyboard_tip,
			    BtkTooltip *tooltip,
			    gpointer    data)
{
  BtkTextTag *tag = data;
  BtkTextIter iter;
  BtkTextView *text_view = BTK_TEXT_VIEW (widget);

  if (keyboard_tip)
    {
      gint offset;

      g_object_get (text_view->buffer, "cursor-position", &offset, NULL);
      btk_text_buffer_get_iter_at_offset (text_view->buffer, &iter, offset);
    }
  else
    {
      gint bx, by, trailing;

      btk_text_view_window_to_buffer_coords (text_view, BTK_TEXT_WINDOW_TEXT,
					     x, y, &bx, &by);
      btk_text_view_get_iter_at_position (text_view, &iter, &trailing, bx, by);
    }

  if (btk_text_iter_has_tag (&iter, tag))
    btk_tooltip_set_text (tooltip, "Tooltip on text tag");
  else
   return FALSE;

  return TRUE;
}

static gboolean
query_tooltip_tree_view_cb (BtkWidget  *widget,
			    gint        x,
			    gint        y,
			    gboolean    keyboard_tip,
			    BtkTooltip *tooltip,
			    gpointer    data)
{
  BtkTreeIter iter;
  BtkTreeView *tree_view = BTK_TREE_VIEW (widget);
  BtkTreeModel *model = btk_tree_view_get_model (tree_view);
  BtkTreePath *path = NULL;
  gchar *tmp;
  gchar *pathstring;

  char buffer[512];

  if (!btk_tree_view_get_tooltip_context (tree_view, &x, &y,
					  keyboard_tip,
					  &model, &path, &iter))
    return FALSE;

  btk_tree_model_get (model, &iter, 0, &tmp, -1);
  pathstring = btk_tree_path_to_string (path);

  g_snprintf (buffer, 511, "<b>Path %s:</b> %s", pathstring, tmp);
  btk_tooltip_set_markup (tooltip, buffer);

  btk_tree_view_set_tooltip_row (tree_view, tooltip, path);

  btk_tree_path_free (path);
  g_free (pathstring);
  g_free (tmp);

  return TRUE;
}

static BtkTreeModel *
create_model (void)
{
  BtkTreeStore *store;
  BtkTreeIter iter;

  store = btk_tree_store_new (1, B_TYPE_STRING);

  /* A tree store with some random words ... */
  btk_tree_store_insert_with_values (store, &iter, NULL, 0,
				     0, "File Manager", -1);
  btk_tree_store_insert_with_values (store, &iter, NULL, 0,
				     0, "Gossip", -1);
  btk_tree_store_insert_with_values (store, &iter, NULL, 0,
				     0, "System Settings", -1);
  btk_tree_store_insert_with_values (store, &iter, NULL, 0,
				     0, "The GIMP", -1);
  btk_tree_store_insert_with_values (store, &iter, NULL, 0,
				     0, "Terminal", -1);
  btk_tree_store_insert_with_values (store, &iter, NULL, 0,
				     0, "Word Processor", -1);

  return BTK_TREE_MODEL (store);
}

static void
selection_changed_cb (BtkTreeSelection *selection,
		      BtkWidget        *tree_view)
{
  btk_widget_trigger_tooltip_query (tree_view);
}

static struct Rectangle
{
  gint x;
  gint y;
  gfloat r;
  gfloat g;
  gfloat b;
  const char *tooltip;
}
rectangles[] =
{
  { 10, 10, 0.0, 0.0, 0.9, "Blue box!" },
  { 200, 170, 1.0, 0.0, 0.0, "Red thing" },
  { 100, 50, 0.8, 0.8, 0.0, "Yellow thing" }
};

static gboolean
query_tooltip_drawing_area_cb (BtkWidget  *widget,
			       gint        x,
			       gint        y,
			       gboolean    keyboard_tip,
			       BtkTooltip *tooltip,
			       gpointer    data)
{
  gint i;

  if (keyboard_tip)
    return FALSE;

  for (i = 0; i < G_N_ELEMENTS (rectangles); i++)
    {
      struct Rectangle *r = &rectangles[i];

      if (r->x < x && x < r->x + 50
	  && r->y < y && y < r->y + 50)
        {
	  btk_tooltip_set_markup (tooltip, r->tooltip);
	  return TRUE;
	}
    }

  return FALSE;
}

static gboolean
drawing_area_expose (BtkWidget      *drawing_area,
		     BdkEventExpose *event,
		     gpointer        data)
{
  gint i;
  bairo_t *cr;

  bdk_window_get_pointer (drawing_area->window, NULL, NULL, NULL);

  cr = bdk_bairo_create (drawing_area->window);

  bairo_rectangle (cr, 0, 0,
		   drawing_area->allocation.width,
		   drawing_area->allocation.height);
  bairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  bairo_fill (cr);

  for (i = 0; i < G_N_ELEMENTS (rectangles); i++)
    {
      struct Rectangle *r = &rectangles[i];

      bairo_rectangle (cr, r->x, r->y, 50, 50);
      bairo_set_source_rgb (cr, r->r, r->g, r->b);
      bairo_stroke (cr);

      bairo_rectangle (cr, r->x, r->y, 50, 50);
      bairo_set_source_rgba (cr, r->r, r->g, r->b, 0.5);
      bairo_fill (cr);
    }

  bairo_destroy (cr);

  return FALSE;
}

static gboolean
query_tooltip_label_cb (BtkWidget  *widget,
			gint        x,
			gint        y,
			gboolean    keyboard_tip,
			BtkTooltip *tooltip,
			gpointer    data)
{
  BtkWidget *custom = data;

  btk_tooltip_set_custom (tooltip, custom);

  return TRUE;
}

int
main (int argc, char *argv[])
{
  BtkWidget *window;
  BtkWidget *box;
  BtkWidget *drawing_area;
  BtkWidget *button;
  BtkWidget *label;

  BtkWidget *tooltip_window;
  BtkWidget *tooltip_button;

  BtkWidget *tree_view;
  BtkTreeViewColumn *column;

  BtkWidget *text_view;
  BtkTextBuffer *buffer;
  BtkTextIter iter;
  BtkTextTag *tag;

  gchar *text, *markup;

  btk_init (&argc, &argv);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_title (BTK_WINDOW (window), "Tooltips test");
  btk_container_set_border_width (BTK_CONTAINER (window), 10);
  g_signal_connect (window, "delete_event",
		    G_CALLBACK (btk_main_quit), NULL);

  box = btk_vbox_new (FALSE, 3);
  btk_container_add (BTK_CONTAINER (window), box);

  /* A check button using the tooltip-markup property */
  button = btk_check_button_new_with_label ("This one uses the tooltip-markup property");
  btk_widget_set_tooltip_text (button, "Hello, I am a static tooltip.");
  btk_box_pack_start (BTK_BOX (box), button, FALSE, FALSE, 0);

  text = btk_widget_get_tooltip_text (button);
  markup = btk_widget_get_tooltip_markup (button);
  g_assert (g_str_equal ("Hello, I am a static tooltip.", text));
  g_assert (g_str_equal ("Hello, I am a static tooltip.", markup));
  g_free (text); g_free (markup);

  /* A check button using the query-tooltip signal */
  button = btk_check_button_new_with_label ("I use the query-tooltip signal");
  g_object_set (button, "has-tooltip", TRUE, NULL);
  g_signal_connect (button, "query-tooltip",
		    G_CALLBACK (query_tooltip_cb), NULL);
  btk_box_pack_start (BTK_BOX (box), button, FALSE, FALSE, 0);

  /* A label */
  button = btk_label_new ("I am just a label");
  btk_label_set_selectable (BTK_LABEL (button), FALSE);
  btk_widget_set_tooltip_text (button, "Label & and tooltip");
  btk_box_pack_start (BTK_BOX (box), button, FALSE, FALSE, 0);

  text = btk_widget_get_tooltip_text (button);
  markup = btk_widget_get_tooltip_markup (button);
  g_assert (g_str_equal ("Label & and tooltip", text));
  g_assert (g_str_equal ("Label &amp; and tooltip", markup));
  g_free (text); g_free (markup);

  /* A selectable label */
  button = btk_label_new ("I am a selectable label");
  btk_label_set_selectable (BTK_LABEL (button), TRUE);
  btk_widget_set_tooltip_markup (button, "<b>Another</b> Label tooltip");
  btk_box_pack_start (BTK_BOX (box), button, FALSE, FALSE, 0);

  text = btk_widget_get_tooltip_text (button);
  markup = btk_widget_get_tooltip_markup (button);
  g_assert (g_str_equal ("Another Label tooltip", text));
  g_assert (g_str_equal ("<b>Another</b> Label tooltip", markup));
  g_free (text); g_free (markup);

  /* Another one, with a custom tooltip window */
  button = btk_check_button_new_with_label ("This one has a custom tooltip window!");
  btk_box_pack_start (BTK_BOX (box), button, FALSE, FALSE, 0);

  tooltip_window = btk_window_new (BTK_WINDOW_POPUP);
  tooltip_button = btk_label_new ("blaat!");
  btk_container_add (BTK_CONTAINER (tooltip_window), tooltip_button);
  btk_widget_show (tooltip_button);

  btk_widget_set_tooltip_window (button, BTK_WINDOW (tooltip_window));
  g_signal_connect (button, "query-tooltip",
		    G_CALLBACK (query_tooltip_custom_cb), NULL);
  g_object_set (button, "has-tooltip", TRUE, NULL);

  /* An insensitive button */
  button = btk_button_new_with_label ("This one is insensitive");
  btk_widget_set_sensitive (button, FALSE);
  g_object_set (button, "tooltip-text", "Insensitive!", NULL);
  btk_box_pack_start (BTK_BOX (box), button, FALSE, FALSE, 0);

  /* Testcases from Kris without a tree view don't exist. */
  tree_view = btk_tree_view_new_with_model (create_model ());
  btk_widget_set_size_request (tree_view, 200, 240);

  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tree_view),
					       0, "Test",
					       btk_cell_renderer_text_new (),
					       "text", 0,
					       NULL);

  g_object_set (tree_view, "has-tooltip", TRUE, NULL);
  g_signal_connect (tree_view, "query-tooltip",
		    G_CALLBACK (query_tooltip_tree_view_cb), NULL);
  g_signal_connect (btk_tree_view_get_selection (BTK_TREE_VIEW (tree_view)),
		    "changed", G_CALLBACK (selection_changed_cb), tree_view);

  /* Set a tooltip on the column */
  column = btk_tree_view_get_column (BTK_TREE_VIEW (tree_view), 0);
  btk_tree_view_column_set_clickable (column, TRUE);
  g_object_set (column->button, "tooltip-text", "Header", NULL);

  btk_box_pack_start (BTK_BOX (box), tree_view, FALSE, FALSE, 2);

  /* And a text view for Matthias */
  buffer = btk_text_buffer_new (NULL);

  btk_text_buffer_get_end_iter (buffer, &iter);
  btk_text_buffer_insert (buffer, &iter, "Hello, the text ", -1);

  tag = btk_text_buffer_create_tag (buffer, "bold", NULL);
  g_object_set (tag, "weight", BANGO_WEIGHT_BOLD, NULL);

  btk_text_buffer_get_end_iter (buffer, &iter);
  btk_text_buffer_insert_with_tags (buffer, &iter, "in bold", -1, tag, NULL);

  btk_text_buffer_get_end_iter (buffer, &iter);
  btk_text_buffer_insert (buffer, &iter, " has a tooltip!", -1);

  text_view = btk_text_view_new_with_buffer (buffer);
  btk_widget_set_size_request (text_view, 200, 50);

  g_object_set (text_view, "has-tooltip", TRUE, NULL);
  g_signal_connect (text_view, "query-tooltip",
		    G_CALLBACK (query_tooltip_text_view_cb), tag);

  btk_box_pack_start (BTK_BOX (box), text_view, FALSE, FALSE, 2);

  /* Drawing area */
  drawing_area = btk_drawing_area_new ();
  btk_widget_set_size_request (drawing_area, 320, 240);
  g_object_set (drawing_area, "has-tooltip", TRUE, NULL);
  g_signal_connect (drawing_area, "expose_event",
		    G_CALLBACK (drawing_area_expose), NULL);
  g_signal_connect (drawing_area, "query-tooltip",
		    G_CALLBACK (query_tooltip_drawing_area_cb), NULL);
  btk_box_pack_start (BTK_BOX (box), drawing_area, FALSE, FALSE, 2);

  button = btk_label_new ("Custom tooltip I");
  label = btk_label_new ("See, custom");
  g_object_ref_sink (label);
  g_object_set (button, "has-tooltip", TRUE, NULL);
  g_signal_connect (button, "query-tooltip",
		    G_CALLBACK (query_tooltip_label_cb), label);
  btk_box_pack_start (BTK_BOX (box), button, FALSE, FALSE, 2);

  button = btk_label_new ("Custom tooltip II");
  label = btk_label_new ("See, custom, too");
  g_object_ref_sink (label);
  g_object_set (button, "has-tooltip", TRUE, NULL);
  btk_box_pack_start (BTK_BOX (box), button, FALSE, FALSE, 2);
  g_signal_connect (button, "query-tooltip",
		    G_CALLBACK (query_tooltip_label_cb), label);

  /* Done! */
  btk_widget_show_all (window);

  btk_main ();

  return 0;
}
