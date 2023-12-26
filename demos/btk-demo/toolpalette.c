/* Tool Palette
 *
 * A tool palette widget shows groups of toolbar items as a grid of icons
 * or a list of names.
 */

#include <string.h>
#include <btk/btk.h>
#include "config.h"
#include "demo-common.h"

static BtkWidget *window = NULL;

static void load_stock_items (BtkToolPalette *palette);
static void load_toggle_items (BtkToolPalette *palette);
static void load_special_items (BtkToolPalette *palette);

typedef struct _CanvasItem CanvasItem;

struct _CanvasItem
{
  BdkPixbuf *pixbuf;
  gdouble x, y;
};

static CanvasItem *drop_item = NULL;
static GList *canvas_items = NULL;

/********************************/
/* ====== Canvas drawing ====== */
/********************************/

static CanvasItem*
canvas_item_new (BtkWidget     *widget,
                 BtkToolButton *button,
                 gdouble        x,
                 gdouble        y)
{
  CanvasItem *item = NULL;
  const gchar *stock_id;
  BdkPixbuf *pixbuf;

  stock_id = btk_tool_button_get_stock_id (button);
  pixbuf = btk_widget_render_icon (widget, stock_id, BTK_ICON_SIZE_DIALOG, NULL);

  if (pixbuf)
    {
      item = g_slice_new0 (CanvasItem);
      item->pixbuf = pixbuf;
      item->x = x;
      item->y = y;
    }

  return item;
}

static void
canvas_item_free (CanvasItem *item)
{
  g_object_unref (item->pixbuf);
  g_slice_free (CanvasItem, item);
}

static void
canvas_item_draw (const CanvasItem *item,
                  bairo_t          *cr,
                  gboolean          preview)
{
  gdouble cx = bdk_pixbuf_get_width (item->pixbuf);
  gdouble cy = bdk_pixbuf_get_height (item->pixbuf);

  bdk_bairo_set_source_pixbuf (cr,
                               item->pixbuf,
                               item->x - cx * 0.5,
                               item->y - cy * 0.5);

  if (preview)
    bairo_paint_with_alpha (cr, 0.6);
  else
    bairo_paint (cr);
}

static gboolean
canvas_expose_event (BtkWidget      *widget,
                     BdkEventExpose *event)
{
  bairo_t *cr;
  GList *iter;
  BtkAllocation allocation;

  cr = bdk_bairo_create (btk_widget_get_window (widget));
  bdk_bairo_rebunnyion (cr, event->rebunnyion);
  bairo_clip (cr);

  btk_widget_get_allocation (widget, &allocation);
  bairo_set_source_rgb (cr, 1, 1, 1);
  bairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
  bairo_fill (cr);

  for (iter = canvas_items; iter; iter = iter->next)
    canvas_item_draw (iter->data, cr, FALSE);

  if (drop_item)
    canvas_item_draw (drop_item, cr, TRUE);

  bairo_destroy (cr);

  return TRUE;
}

/*****************************/
/* ====== Palette DnD ====== */
/*****************************/

static void
palette_drop_item (BtkToolItem      *drag_item,
                   BtkToolItemGroup *drop_group,
                   gint              x,
                   gint              y)
{
  BtkWidget *drag_group = btk_widget_get_parent (BTK_WIDGET (drag_item));
  BtkToolItem *drop_item = btk_tool_item_group_get_drop_item (drop_group, x, y);
  gint drop_position = -1;

  if (drop_item)
    drop_position = btk_tool_item_group_get_item_position (BTK_TOOL_ITEM_GROUP (drop_group), drop_item);

  if (BTK_TOOL_ITEM_GROUP (drag_group) != drop_group)
    {
      gboolean homogeneous, expand, fill, new_row;

      g_object_ref (drag_item);
      btk_container_child_get (BTK_CONTAINER (drag_group), BTK_WIDGET (drag_item),
                               "homogeneous", &homogeneous,
                               "expand", &expand,
                               "fill", &fill,
                               "new-row", &new_row,
                               NULL);
      btk_container_remove (BTK_CONTAINER (drag_group), BTK_WIDGET (drag_item));
      btk_tool_item_group_insert (BTK_TOOL_ITEM_GROUP (drop_group),
                                  drag_item, drop_position);
      btk_container_child_set (BTK_CONTAINER (drop_group), BTK_WIDGET (drag_item),
                               "homogeneous", homogeneous,
                               "expand", expand,
                               "fill", fill,
                               "new-row", new_row,
                               NULL);
      g_object_unref (drag_item);
    }
  else
    btk_tool_item_group_set_item_position (BTK_TOOL_ITEM_GROUP (drop_group),
                                           drag_item, drop_position);
}

static void
palette_drop_group (BtkToolPalette   *palette,
                    BtkToolItemGroup *drag_group,
                    BtkToolItemGroup *drop_group)
{
  gint drop_position = -1;

  if (drop_group)
    drop_position = btk_tool_palette_get_group_position (palette, drop_group);

  btk_tool_palette_set_group_position (palette, drag_group, drop_position);
}

static void
palette_drag_data_received (BtkWidget        *widget,
                            BdkDragContext   *context,
                            gint              x,
                            gint              y,
                            BtkSelectionData *selection,
                            guint             info,
                            guint             time,
                            gpointer          data)
{
  BtkToolItemGroup *drop_group = NULL;
  BtkWidget        *drag_palette = btk_drag_get_source_widget (context);
  BtkWidget        *drag_item = NULL;
  BtkAllocation	    allocation;

  while (drag_palette && !BTK_IS_TOOL_PALETTE (drag_palette))
    drag_palette = btk_widget_get_parent (drag_palette);

  if (drag_palette)
    {
      drag_item = btk_tool_palette_get_drag_item (BTK_TOOL_PALETTE (drag_palette),
                                                  selection);
      drop_group = btk_tool_palette_get_drop_group (BTK_TOOL_PALETTE (widget),
                                                    x, y);
      btk_widget_get_allocation (BTK_WIDGET (drop_group), &allocation);
    }

  if (BTK_IS_TOOL_ITEM_GROUP (drag_item))
    palette_drop_group (BTK_TOOL_PALETTE (drag_palette),
                        BTK_TOOL_ITEM_GROUP (drag_item),
                        drop_group);
  else if (BTK_IS_TOOL_ITEM (drag_item) && drop_group)
    palette_drop_item (BTK_TOOL_ITEM (drag_item),
                       drop_group,
                       x - allocation.x,
                       y - allocation.y);
}

/********************************/
/* ====== Passive Canvas ====== */
/********************************/

static void
passive_canvas_drag_data_received (BtkWidget        *widget,
                                   BdkDragContext   *context,
                                   gint              x,
                                   gint              y,
                                   BtkSelectionData *selection,
                                   guint             info,
                                   guint             time,
                                   gpointer          data)
{
  /* find the tool button, which is the source of this DnD operation */

  BtkWidget *palette = btk_drag_get_source_widget (context);
  CanvasItem *canvas_item = NULL;
  BtkWidget *tool_item = NULL;

  while (palette && !BTK_IS_TOOL_PALETTE (palette))
    palette = btk_widget_get_parent (palette);

  if (palette)
    tool_item = btk_tool_palette_get_drag_item (BTK_TOOL_PALETTE (palette),
                                                selection);

  g_assert (NULL == drop_item);

  /* append a new canvas item when a tool button was found */

  if (BTK_IS_TOOL_ITEM (tool_item))
    canvas_item = canvas_item_new (widget, BTK_TOOL_BUTTON (tool_item), x, y);

  if (canvas_item)
    {
      canvas_items = g_list_append (canvas_items, canvas_item);
      btk_widget_queue_draw (widget);
    }
}

/************************************/
/* ====== Interactive Canvas ====== */
/************************************/

static gboolean
interactive_canvas_drag_motion (BtkWidget      *widget,
                                BdkDragContext *context,
                                gint            x,
                                gint            y,
                                guint           time,
                                gpointer        data)
{
  if (drop_item)
    {
      /* already have a drop indicator - just update position */

      drop_item->x = x;
      drop_item->y = y;

      btk_widget_queue_draw (widget);
      bdk_drag_status (context, BDK_ACTION_COPY, time);
    }
  else
    {
      /* request DnD data for creating a drop indicator */

      BdkAtom target = btk_drag_dest_find_target (widget, context, NULL);

      if (!target)
        return FALSE;

      btk_drag_get_data (widget, context, target, time);
    }

  return TRUE;
}

static void
interactive_canvas_drag_data_received (BtkWidget        *widget,
                                       BdkDragContext   *context,
                                       gint              x,
                                       gint              y,
                                       BtkSelectionData *selection,
                                       guint             info,
                                       guint             time,
                                       gpointer          data)

{
  /* find the tool button which is the source of this DnD operation */

  BtkWidget *palette = btk_drag_get_source_widget (context);
  BtkWidget *tool_item = NULL;

  while (palette && !BTK_IS_TOOL_PALETTE (palette))
    palette = btk_widget_get_parent (palette);

  if (palette)
    tool_item = btk_tool_palette_get_drag_item (BTK_TOOL_PALETTE (palette),
                                                selection);

  /* create a drop indicator when a tool button was found */

  g_assert (NULL == drop_item);

  if (BTK_IS_TOOL_ITEM (tool_item))
    {
      drop_item = canvas_item_new (widget, BTK_TOOL_BUTTON (tool_item), x, y);
      bdk_drag_status (context, BDK_ACTION_COPY, time);
      btk_widget_queue_draw (widget);
    }
}

static gboolean
interactive_canvas_drag_drop (BtkWidget      *widget,
                              BdkDragContext *context,
                              gint            x,
                              gint            y,
                              guint           time,
                              gpointer        data)
{
  if (drop_item)
    {
      /* turn the drop indicator into a real canvas item */

      drop_item->x = x;
      drop_item->y = y;

      canvas_items = g_list_append (canvas_items, drop_item);
      drop_item = NULL;

      /* signal the item was accepted and redraw */

      btk_drag_finish (context, TRUE, FALSE, time);
      btk_widget_queue_draw (widget);

      return TRUE;
    }

  return FALSE;
}

static gboolean
interactive_canvas_real_drag_leave (gpointer data)
{
  if (drop_item)
    {
      BtkWidget *widget = BTK_WIDGET (data);

      canvas_item_free (drop_item);
      drop_item = NULL;

      btk_widget_queue_draw (widget);
    }

  return FALSE;
}

static void
interactive_canvas_drag_leave (BtkWidget      *widget,
                               BdkDragContext *context,
                               guint           time,
                               gpointer        data)
{
  /* defer cleanup until a potential "drag-drop" signal was received */
  g_idle_add (interactive_canvas_real_drag_leave, widget);
}

static void
on_combo_orientation_changed (BtkComboBox *combo_box,
                              gpointer     user_data)
{
  BtkToolPalette *palette = BTK_TOOL_PALETTE (user_data);
  BtkScrolledWindow *sw = BTK_SCROLLED_WINDOW (btk_widget_get_parent (BTK_WIDGET (palette)));
  BtkTreeModel *model = btk_combo_box_get_model (combo_box);
  BtkTreeIter iter;
  gint val = 0;

  if (!btk_combo_box_get_active_iter (combo_box, &iter))
    return;

  btk_tree_model_get (model, &iter, 1, &val, -1);

  btk_orientable_set_orientation (BTK_ORIENTABLE (palette), val);

  if (val == BTK_ORIENTATION_HORIZONTAL)
    btk_scrolled_window_set_policy (sw, BTK_POLICY_AUTOMATIC, BTK_POLICY_NEVER);
  else
    btk_scrolled_window_set_policy (sw, BTK_POLICY_NEVER, BTK_POLICY_AUTOMATIC);
}

static void
on_combo_style_changed (BtkComboBox *combo_box,
                        gpointer     user_data)
{
  BtkToolPalette *palette = BTK_TOOL_PALETTE (user_data);
  BtkTreeModel *model = btk_combo_box_get_model (combo_box);
  BtkTreeIter iter;
  gint val = 0;

  if (!btk_combo_box_get_active_iter (combo_box, &iter))
    return;

  btk_tree_model_get (model, &iter, 1, &val, -1);

  if (val == -1)
    btk_tool_palette_unset_style (palette);
  else
    btk_tool_palette_set_style (palette, val);
}

BtkWidget *
do_toolpalette (BtkWidget *do_widget)
{
  BtkWidget *box = NULL;
  BtkWidget *hbox = NULL;
  BtkWidget *combo_orientation = NULL;
  BtkListStore *orientation_model = NULL;
  BtkWidget *combo_style = NULL;
  BtkListStore *style_model = NULL;
  BtkCellRenderer *cell_renderer = NULL;
  BtkTreeIter iter;
  BtkWidget *palette = NULL;
  BtkWidget *palette_scroller = NULL;
  BtkWidget *notebook = NULL;
  BtkWidget *contents = NULL;
  BtkWidget *contents_scroller = NULL;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
                             btk_widget_get_screen (do_widget));
      btk_window_set_title (BTK_WINDOW (window), "Tool Palette");
      btk_window_set_default_size (BTK_WINDOW (window), 200, 600);

      g_signal_connect (window, "destroy",
                        G_CALLBACK (btk_widget_destroyed), &window);
      btk_container_set_border_width (BTK_CONTAINER (window), 8);

      /* Add widgets to control the ToolPalette appearance: */
      box = btk_vbox_new (FALSE, 6);
      btk_container_add (BTK_CONTAINER (window), box);

      /* Orientation combo box: */
      orientation_model = btk_list_store_new (2, B_TYPE_STRING, B_TYPE_INT);
      btk_list_store_append (orientation_model, &iter);
      btk_list_store_set (orientation_model, &iter,
                          0, "Horizontal",
                          1, BTK_ORIENTATION_HORIZONTAL,
                          -1);
      btk_list_store_append (orientation_model, &iter);
      btk_list_store_set (orientation_model, &iter,
                          0, "Vertical",
                          1, BTK_ORIENTATION_VERTICAL,
                          -1);

      combo_orientation =
        btk_combo_box_new_with_model (BTK_TREE_MODEL (orientation_model));
      cell_renderer = btk_cell_renderer_text_new ();
      btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combo_orientation),
                                  cell_renderer,
                                  TRUE);
      btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (combo_orientation),
                                      cell_renderer,
                                      "text", 0,
                                      NULL);
      btk_combo_box_set_active_iter (BTK_COMBO_BOX (combo_orientation), &iter);
      btk_box_pack_start (BTK_BOX (box), combo_orientation, FALSE, FALSE, 0);

      /* Style combo box: */
      style_model = btk_list_store_new (2, B_TYPE_STRING, B_TYPE_INT);
      btk_list_store_append (style_model, &iter);
      btk_list_store_set (style_model, &iter,
                          0, "Text",
                          1, BTK_TOOLBAR_TEXT,
                          -1);
      btk_list_store_append (style_model, &iter);
      btk_list_store_set (style_model, &iter,
                          0, "Both",
                          1, BTK_TOOLBAR_BOTH,
                          -1);
      btk_list_store_append (style_model, &iter);
      btk_list_store_set (style_model, &iter,
                          0, "Both: Horizontal",
                          1, BTK_TOOLBAR_BOTH_HORIZ,
                          -1);
      btk_list_store_append (style_model, &iter);
      btk_list_store_set (style_model, &iter,
                          0, "Icons",
                          1, BTK_TOOLBAR_ICONS,
                          -1);
      btk_list_store_append (style_model, &iter);
      btk_list_store_set (style_model, &iter,
                          0, "Default",
                          1, -1,  /* A custom meaning for this demo. */
                          -1);
      combo_style = btk_combo_box_new_with_model (BTK_TREE_MODEL (style_model));
      cell_renderer = btk_cell_renderer_text_new ();
      btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combo_style),
                                  cell_renderer,
                                  TRUE);
      btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (combo_style),
                                      cell_renderer,
                                      "text", 0,
                                      NULL);
      btk_combo_box_set_active_iter (BTK_COMBO_BOX (combo_style), &iter);
      btk_box_pack_start (BTK_BOX (box), combo_style, FALSE, FALSE, 0);

      /* Add hbox */
      hbox = btk_hbox_new (FALSE, 5);
      btk_box_pack_start (BTK_BOX (box), hbox, TRUE, TRUE, 0);

      /* Add and fill the ToolPalette: */
      palette = btk_tool_palette_new ();

      load_stock_items (BTK_TOOL_PALETTE (palette));
      load_toggle_items (BTK_TOOL_PALETTE (palette));
      load_special_items (BTK_TOOL_PALETTE (palette));

      palette_scroller = btk_scrolled_window_new (NULL, NULL);
      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (palette_scroller),
                                      BTK_POLICY_NEVER,
                                      BTK_POLICY_AUTOMATIC);
      btk_container_set_border_width (BTK_CONTAINER (palette_scroller), 6);

      btk_container_add (BTK_CONTAINER (palette_scroller), palette);
      btk_container_add (BTK_CONTAINER (hbox), palette_scroller);

      btk_widget_show_all (box);

      /* Connect signals: */
      g_signal_connect (combo_orientation, "changed",
                        G_CALLBACK (on_combo_orientation_changed), palette);
      g_signal_connect (combo_style, "changed",
                        G_CALLBACK (on_combo_style_changed), palette);

      /* Keep the widgets in sync: */
      on_combo_orientation_changed (BTK_COMBO_BOX (combo_orientation), palette);

      /* ===== notebook ===== */

      notebook = btk_notebook_new ();
      btk_container_set_border_width (BTK_CONTAINER (notebook), 6);
      btk_box_pack_end (BTK_BOX(hbox), notebook, FALSE, FALSE, 0);

      /* ===== DnD for tool items ===== */

      g_signal_connect (palette, "drag-data-received",
                        G_CALLBACK (palette_drag_data_received), NULL);

      btk_tool_palette_add_drag_dest (BTK_TOOL_PALETTE (palette),
                                      palette,
                                      BTK_DEST_DEFAULT_ALL,
                                      BTK_TOOL_PALETTE_DRAG_ITEMS |
                                      BTK_TOOL_PALETTE_DRAG_GROUPS,
                                      BDK_ACTION_MOVE);

      /* ===== passive DnD dest ===== */

      contents = btk_drawing_area_new ();
      btk_widget_set_app_paintable (contents, TRUE);

      g_object_connect (contents,
                        "signal::expose-event", canvas_expose_event, NULL,
                        "signal::drag-data-received", passive_canvas_drag_data_received, NULL,
                        NULL);

      btk_tool_palette_add_drag_dest (BTK_TOOL_PALETTE (palette),
                                      contents,
                                      BTK_DEST_DEFAULT_ALL,
                                      BTK_TOOL_PALETTE_DRAG_ITEMS,
                                      BDK_ACTION_COPY);

      contents_scroller = btk_scrolled_window_new (NULL, NULL);
      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (contents_scroller),
                                      BTK_POLICY_AUTOMATIC,
                                      BTK_POLICY_ALWAYS);
      btk_scrolled_window_add_with_viewport (BTK_SCROLLED_WINDOW (contents_scroller),
                                             contents);
      btk_container_set_border_width (BTK_CONTAINER (contents_scroller), 6);

      btk_notebook_append_page (BTK_NOTEBOOK (notebook),
                                contents_scroller,
                                btk_label_new ("Passive DnD Mode"));

      /* ===== interactive DnD dest ===== */

      contents = btk_drawing_area_new ();
      btk_widget_set_app_paintable (contents, TRUE);

      g_object_connect (contents,
                        "signal::expose-event", canvas_expose_event, NULL,
                        "signal::drag-motion", interactive_canvas_drag_motion, NULL,
                        "signal::drag-data-received", interactive_canvas_drag_data_received, NULL,
                        "signal::drag-leave", interactive_canvas_drag_leave, NULL,
                        "signal::drag-drop", interactive_canvas_drag_drop, NULL,
                        NULL);

      btk_tool_palette_add_drag_dest (BTK_TOOL_PALETTE (palette),
                                      contents,
                                      BTK_DEST_DEFAULT_HIGHLIGHT,
                                      BTK_TOOL_PALETTE_DRAG_ITEMS,
                                      BDK_ACTION_COPY);

      contents_scroller = btk_scrolled_window_new (NULL, NULL);
      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (contents_scroller),
                                      BTK_POLICY_AUTOMATIC,
                                      BTK_POLICY_ALWAYS);
      btk_scrolled_window_add_with_viewport (BTK_SCROLLED_WINDOW (contents_scroller),
                                             contents);
      btk_container_set_border_width (BTK_CONTAINER (contents_scroller), 6);

      btk_notebook_append_page (BTK_NOTEBOOK (notebook), contents_scroller,
                                btk_label_new ("Interactive DnD Mode"));
    }

  if (!btk_widget_get_visible (window))
    {
      btk_widget_show_all (window);
    }
  else
    {
      btk_widget_destroy (window);
      window = NULL;
    }

  return window;
}


static void
load_stock_items (BtkToolPalette *palette)
{
  BtkWidget *group_af = btk_tool_item_group_new ("Stock Icons (A-F)");
  BtkWidget *group_gn = btk_tool_item_group_new ("Stock Icons (G-N)");
  BtkWidget *group_or = btk_tool_item_group_new ("Stock Icons (O-R)");
  BtkWidget *group_sz = btk_tool_item_group_new ("Stock Icons (S-Z)");
  BtkWidget *group = NULL;

  BtkToolItem *item;
  GSList *stock_ids;
  GSList *iter;

  stock_ids = btk_stock_list_ids ();
  stock_ids = b_slist_sort (stock_ids, (GCompareFunc) strcmp);

  btk_container_add (BTK_CONTAINER (palette), group_af);
  btk_container_add (BTK_CONTAINER (palette), group_gn);
  btk_container_add (BTK_CONTAINER (palette), group_or);
  btk_container_add (BTK_CONTAINER (palette), group_sz);

  for (iter = stock_ids; iter; iter = b_slist_next (iter))
    {
      BtkStockItem stock_item;
      gchar *id = iter->data;

      switch (id[4])
        {
          case 'a':
            group = group_af;
            break;

          case 'g':
            group = group_gn;
            break;

          case 'o':
            group = group_or;
            break;

          case 's':
            group = group_sz;
            break;
        }

      item = btk_tool_button_new_from_stock (id);
      btk_tool_item_set_tooltip_text (BTK_TOOL_ITEM (item), id);
      btk_tool_item_set_is_important (BTK_TOOL_ITEM (item), TRUE);
      btk_tool_item_group_insert (BTK_TOOL_ITEM_GROUP (group), item, -1);

      if (!btk_stock_lookup (id, &stock_item) || !stock_item.label)
        btk_tool_button_set_label (BTK_TOOL_BUTTON (item), id);

      g_free (id);
    }

  b_slist_free (stock_ids);
}

static void
load_toggle_items (BtkToolPalette *palette)
{
  GSList *toggle_group = NULL;
  BtkToolItem *item;
  BtkWidget *group;
  char *label;
  int i;

  group = btk_tool_item_group_new ("Radio Item");
  btk_container_add (BTK_CONTAINER (palette), group);

  for (i = 1; i <= 10; ++i)
    {
      label = g_strdup_printf ("#%d", i);
      item = btk_radio_tool_button_new (toggle_group);
      btk_tool_button_set_label (BTK_TOOL_BUTTON (item), label);
      g_free (label);

      btk_tool_item_group_insert (BTK_TOOL_ITEM_GROUP (group), item, -1);
      toggle_group = btk_radio_tool_button_get_group (BTK_RADIO_TOOL_BUTTON (item));
    }
}

static BtkToolItem *
create_entry_item (const char *text)
{
  BtkToolItem *item;
  BtkWidget *entry;

  entry = btk_entry_new ();
  btk_entry_set_text (BTK_ENTRY (entry), text);
  btk_entry_set_width_chars (BTK_ENTRY (entry), 5);

  item = btk_tool_item_new ();
  btk_container_add (BTK_CONTAINER (item), entry);

  return item;
}

static void
load_special_items (BtkToolPalette *palette)
{
  BtkToolItem *item;
  BtkWidget *group;
  BtkWidget *label_button;

  group = btk_tool_item_group_new (NULL);
  label_button = btk_button_new_with_label ("Advanced Features");
  btk_widget_show (label_button);
  btk_tool_item_group_set_label_widget (BTK_TOOL_ITEM_GROUP (group),
                                        label_button);
  btk_container_add (BTK_CONTAINER (palette), group);

  item = create_entry_item ("homogeneous=FALSE");
  btk_tool_item_group_insert (BTK_TOOL_ITEM_GROUP (group), item, -1);
  btk_container_child_set (BTK_CONTAINER (group), BTK_WIDGET (item),
                           "homogeneous", FALSE, NULL);

  item = create_entry_item ("homogeneous=FALSE, expand=TRUE");
  btk_tool_item_group_insert (BTK_TOOL_ITEM_GROUP (group), item, -1);
  btk_container_child_set (BTK_CONTAINER (group), BTK_WIDGET (item),
                           "homogeneous", FALSE, "expand", TRUE,
                           NULL);

  item = create_entry_item ("homogeneous=FALSE, expand=TRUE, fill=FALSE");
  btk_tool_item_group_insert (BTK_TOOL_ITEM_GROUP (group), item, -1);
  btk_container_child_set (BTK_CONTAINER (group), BTK_WIDGET (item),
                           "homogeneous", FALSE, "expand", TRUE,
                           "fill", FALSE, NULL);

  item = create_entry_item ("homogeneous=FALSE, expand=TRUE, new-row=TRUE");
  btk_tool_item_group_insert (BTK_TOOL_ITEM_GROUP (group), item, -1);
  btk_container_child_set (BTK_CONTAINER (group), BTK_WIDGET (item),
                           "homogeneous", FALSE, "expand", TRUE,
                           "new-row", TRUE, NULL);

  item = btk_tool_button_new_from_stock (BTK_STOCK_GO_UP);
  btk_tool_item_set_tooltip_text (item, "Show on vertical palettes only");
  btk_tool_item_group_insert (BTK_TOOL_ITEM_GROUP (group), item, -1);
  btk_tool_item_set_visible_horizontal (item, FALSE);

  item = btk_tool_button_new_from_stock (BTK_STOCK_GO_FORWARD);
  btk_tool_item_set_tooltip_text (item, "Show on horizontal palettes only");
  btk_tool_item_group_insert (BTK_TOOL_ITEM_GROUP (group), item, -1);
  btk_tool_item_set_visible_vertical (item, FALSE);

  item = btk_tool_button_new_from_stock (BTK_STOCK_DELETE);
  btk_tool_item_set_tooltip_text (item, "Do not show at all");
  btk_tool_item_group_insert (BTK_TOOL_ITEM_GROUP (group), item, -1);
  btk_widget_set_no_show_all (BTK_WIDGET (item), TRUE);

  item = btk_tool_button_new_from_stock (BTK_STOCK_FULLSCREEN);
  btk_tool_item_set_tooltip_text (item, "Expanded this item");
  btk_tool_item_group_insert (BTK_TOOL_ITEM_GROUP (group), item, -1);
  btk_container_child_set (BTK_CONTAINER (group), BTK_WIDGET (item),
                           "homogeneous", FALSE,
                           "expand", TRUE,
                           NULL);

  item = btk_tool_button_new_from_stock (BTK_STOCK_HELP);
  btk_tool_item_set_tooltip_text (item, "A regular item");
  btk_tool_item_group_insert (BTK_TOOL_ITEM_GROUP (group), item, -1);
}
