/*
 * testoffscreen.c
 */

#undef BTK_DISABLE_DEPRECATED

#include <math.h>
#include <btk/btk.h>
#include "btkoffscreenbox.h"


static void
combo_changed_cb (BtkWidget *combo,
		  bpointer   data)
{
  BtkWidget *label = BTK_WIDGET (data);
  bint active;

  active = btk_combo_box_get_active (BTK_COMBO_BOX (combo));

  btk_label_set_ellipsize (BTK_LABEL (label), (BangoEllipsizeMode)active);
}

static bboolean
layout_expose_handler (BtkWidget      *widget,
                       BdkEventExpose *event)
{
  BtkLayout *layout = BTK_LAYOUT (widget);
  BdkWindow *bin_window = btk_layout_get_bin_window (layout);
  bairo_t *cr;

  bint i,j;
  bint imin, imax, jmin, jmax;

  if (event->window != bin_window)
    return FALSE;

  imin = (event->area.x) / 10;
  imax = (event->area.x + event->area.width + 9) / 10;

  jmin = (event->area.y) / 10;
  jmax = (event->area.y + event->area.height + 9) / 10;

  cr = bdk_bairo_create (bin_window);

  for (i = imin; i < imax; i++)
    for (j = jmin; j < jmax; j++)
      if ((i + j) % 2)
        bairo_rectangle (cr,
                         10 * i, 10 * j,
                         1 + i % 10, 1 + j % 10);

  bairo_fill (cr);

  bairo_destroy (cr);

  return FALSE;
}

static bboolean
scroll_layout (bpointer data)
{
  BtkWidget *layout = data;
  BtkAdjustment *adj;

  adj = btk_layout_get_hadjustment (BTK_LAYOUT (layout));
  btk_adjustment_set_value (adj,
			    btk_adjustment_get_value (adj) + 5.0);
  return TRUE;
}

static buint layout_timeout;

static void
create_layout (BtkWidget *vbox)
{
  BtkWidget *layout;
  BtkWidget *scrolledwindow;
  BtkWidget *button;
  bchar buf[16];
  bint i, j;

  scrolledwindow = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (scrolledwindow),
				       BTK_SHADOW_IN);
  btk_scrolled_window_set_placement (BTK_SCROLLED_WINDOW (scrolledwindow),
				     BTK_CORNER_TOP_RIGHT);

  btk_box_pack_start (BTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);

  layout = btk_layout_new (NULL, NULL);
  btk_container_add (BTK_CONTAINER (scrolledwindow), layout);

  /* We set step sizes here since BtkLayout does not set
   * them itself.
   */
  BTK_LAYOUT (layout)->hadjustment->step_increment = 10.0;
  BTK_LAYOUT (layout)->vadjustment->step_increment = 10.0;

  btk_widget_set_events (layout, BDK_EXPOSURE_MASK);
  g_signal_connect (layout, "expose_event",
		    G_CALLBACK (layout_expose_handler),
                    NULL);

  btk_layout_set_size (BTK_LAYOUT (layout), 1600, 128000);

  for (i = 0 ; i < 16 ; i++)
    for (j = 0 ; j < 16 ; j++)
      {
	g_snprintf (buf, sizeof (buf), "Button %d, %d", i, j);

	if ((i + j) % 2)
	  button = btk_button_new_with_label (buf);
	else
	  button = btk_label_new (buf);

	btk_layout_put (BTK_LAYOUT (layout), button,
			j * 100, i * 100);
      }

  for (i = 16; i < 1280; i++)
    {
      g_snprintf (buf, sizeof (buf), "Button %d, %d", i, 0);

      if (i % 2)
	button = btk_button_new_with_label (buf);
      else
	button = btk_label_new (buf);

      btk_layout_put (BTK_LAYOUT (layout), button,
		      0, i * 100);
    }

  layout_timeout = g_timeout_add (1000, scroll_layout, layout);
}

static void
create_treeview (BtkWidget *vbox)
{
  BtkWidget *scrolledwindow;
  BtkListStore *store;
  BtkWidget *tree_view;
  GSList *stock_ids;
  GSList *list;

  scrolledwindow = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (scrolledwindow),
				       BTK_SHADOW_IN);

  btk_box_pack_start (BTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);

  store = btk_list_store_new (2, B_TYPE_STRING, B_TYPE_STRING);
  tree_view = btk_tree_view_new_with_model (BTK_TREE_MODEL (store));
  g_object_unref (store);

  btk_container_add (BTK_CONTAINER (scrolledwindow), tree_view);

  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tree_view), -1,
                                               "Icon",
                                               btk_cell_renderer_pixbuf_new (),
                                               "stock-id", 0,
                                               NULL);
  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tree_view), -1,
                                               "Label",
                                               btk_cell_renderer_text_new (),
                                               "text", 1,
                                               NULL);

  stock_ids = btk_stock_list_ids ();

  for (list = stock_ids; list; list = b_slist_next (list))
    {
      const bchar *stock_id = list->data;
      BtkStockItem item;

      if (btk_stock_lookup (stock_id, &item))
        {
          btk_list_store_insert_with_values (store, NULL, -1,
                                             0, item.stock_id,
                                             1, item.label,
                                             -1);
        }
    }

  b_slist_foreach (stock_ids, (GFunc) g_free, NULL);
  b_slist_free (stock_ids);
}

static BtkWidget *
create_widgets (void)
{
  BtkWidget *main_hbox, *main_vbox;
  BtkWidget *vbox, *hbox, *label, *combo, *entry, *button, *cb;
  BtkWidget *sw, *text_view;

  main_vbox = btk_vbox_new (0, FALSE);

  main_hbox = btk_hbox_new (0, FALSE);
  btk_box_pack_start (BTK_BOX (main_vbox), main_hbox, TRUE, TRUE, 0);

  vbox = btk_vbox_new (0, FALSE);
  btk_box_pack_start (BTK_BOX (main_hbox), vbox, TRUE, TRUE, 0);

  hbox = btk_hbox_new (0, FALSE);
  btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = btk_label_new ("This label may be ellipsized\nto make it fit.");
  btk_box_pack_start (BTK_BOX (hbox), label, TRUE, TRUE, 0);

  combo = btk_combo_box_text_new ();
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo), "NONE");
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo), "START");
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo), "MIDDLE");
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo), "END");
  btk_combo_box_set_active (BTK_COMBO_BOX (combo), 0);
  btk_box_pack_start (BTK_BOX (hbox), combo, TRUE, TRUE, 0);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (combo_changed_cb),
                    label);

  entry = btk_entry_new ();
  btk_entry_set_text (BTK_ENTRY (entry), "an entry - lots of text.... lots of text.... lots of text.... lots of text.... ");
  btk_box_pack_start (BTK_BOX (vbox), entry, FALSE, FALSE, 0);

  label = btk_label_new ("Label after entry.");
  btk_label_set_selectable (BTK_LABEL (label), TRUE);
  btk_box_pack_start (BTK_BOX (vbox), label, TRUE, TRUE, 0);

  button = btk_button_new_with_label ("Button");
  btk_box_pack_start (BTK_BOX (vbox), button, TRUE, TRUE, 0);

  button = btk_check_button_new_with_mnemonic ("_Check button");
  btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  cb = btk_combo_box_text_new ();
  entry = btk_entry_new ();
  btk_widget_show (entry);
  btk_container_add (BTK_CONTAINER (cb), entry);

  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (cb), "item0");
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (cb), "item1");
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (cb), "item1");
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (cb), "item2");
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (cb), "item2");
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (cb), "item2");
  btk_entry_set_text (BTK_ENTRY (entry), "hello world ♥ foo");
  btk_editable_select_rebunnyion (BTK_EDITABLE (entry), 0, -1);
  btk_box_pack_start (BTK_BOX (vbox), cb, TRUE, TRUE, 0);

  sw = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (sw),
				  BTK_POLICY_AUTOMATIC,
				  BTK_POLICY_AUTOMATIC);
  text_view = btk_text_view_new ();
  btk_box_pack_start (BTK_BOX (vbox), sw, TRUE, TRUE, 0);
  btk_container_add (BTK_CONTAINER (sw), text_view);

  create_layout (vbox);

  create_treeview (main_hbox);

  return main_vbox;
}


static void
scale_changed (BtkRange        *range,
	       BtkOffscreenBox *offscreen_box)
{
  btk_offscreen_box_set_angle (offscreen_box, btk_range_get_value (range));
}

static BtkWidget *scale = NULL;

static void
remove_clicked (BtkButton *button,
                BtkWidget *widget)
{
  btk_widget_destroy (widget);
  g_source_remove (layout_timeout);

  btk_widget_set_sensitive (BTK_WIDGET (button), FALSE);
  btk_widget_set_sensitive (scale, FALSE);
}

int
main (int   argc,
      char *argv[])
{
  BtkWidget *window, *widget, *vbox, *button;
  BtkWidget *offscreen = NULL;
  bboolean use_offscreen;

  btk_init (&argc, &argv);

  use_offscreen = argc == 1;

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_default_size (BTK_WINDOW (window), 300,300);

  g_signal_connect (window, "destroy",
                    G_CALLBACK (btk_main_quit),
                    NULL);

  vbox = btk_vbox_new (0, FALSE);
  btk_container_add (BTK_CONTAINER (window), vbox);

  scale = btk_hscale_new_with_range (0,
				     G_PI * 2,
				     0.01);
  btk_box_pack_start (BTK_BOX (vbox), scale, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Remove child 2");
  btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  if (use_offscreen)
    {
      offscreen = btk_offscreen_box_new ();

      g_signal_connect (scale, "value_changed",
                        G_CALLBACK (scale_changed),
                        offscreen);
    }
  else
    {
      offscreen = btk_vpaned_new ();
    }

  btk_box_pack_start (BTK_BOX (vbox), offscreen, TRUE, TRUE, 0);

  widget = create_widgets ();
  if (use_offscreen)
    btk_offscreen_box_add1 (BTK_OFFSCREEN_BOX (offscreen),
			    widget);
  else
    btk_paned_add1 (BTK_PANED (offscreen), widget);

  widget = create_widgets ();
  if (1)
    {
      BtkWidget *widget2, *box2, *offscreen2;

      offscreen2 = btk_offscreen_box_new ();
      btk_box_pack_start (BTK_BOX (widget), offscreen2, FALSE, FALSE, 0);

      g_signal_connect (scale, "value_changed",
                        G_CALLBACK (scale_changed),
                        offscreen2);

      box2 = btk_vbox_new (FALSE, 0);
      btk_offscreen_box_add2 (BTK_OFFSCREEN_BOX (offscreen2), box2);

      widget2 = btk_button_new_with_label ("Offscreen in offscreen");
      btk_box_pack_start (BTK_BOX (box2), widget2, FALSE, FALSE, 0);

      widget2 = btk_entry_new ();
      btk_entry_set_text (BTK_ENTRY (widget2), "Offscreen in offscreen");
      btk_box_pack_start (BTK_BOX (box2), widget2, FALSE, FALSE, 0);
    }

  if (use_offscreen)
    btk_offscreen_box_add2 (BTK_OFFSCREEN_BOX (offscreen), widget);
  else
    btk_paned_add2 (BTK_PANED (offscreen), widget);

  btk_widget_show_all (window);

  g_signal_connect (B_OBJECT (button), "clicked",
                    G_CALLBACK (remove_clicked),
                    widget);

  /* redirect */
  if (0)
    {
      BtkWidget *redirect_win;

      redirect_win = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_default_size (BTK_WINDOW (redirect_win), 400,400);
      btk_widget_show (redirect_win);
      btk_widget_realize (redirect_win);
      btk_widget_realize (window);
      bdk_window_redirect_to_drawable (window->window,
				       BDK_DRAWABLE (redirect_win->window),
				       0, 0, 0, 0, -1, -1);
    }

  btk_main ();

  return 0;
}
