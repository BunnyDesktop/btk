/* Stock Item and Icon Browser
 *
 * This source code for this demo doesn't demonstrate anything
 * particularly useful in applications. The purpose of the "demo" is
 * just to provide a handy place to browse the available stock icons
 * and stock items.
 */

#include <string.h>

#include <btk/btk.h>

static BtkWidget *window = NULL;

typedef struct _StockItemInfo StockItemInfo;
struct _StockItemInfo
{
  gchar *id;
  BtkStockItem item;
  BdkPixbuf *small_icon;
  gchar *macro;
  gchar *accel_str;
};

/* Make StockItemInfo a boxed type so we can automatically
 * manage memory
 */
#define STOCK_ITEM_INFO_TYPE stock_item_info_get_type ()

static void
stock_item_info_free (StockItemInfo *info)
{
  g_free (info->id);
  g_free (info->macro);
  g_free (info->accel_str);
  if (info->small_icon)
    g_object_unref (info->small_icon);
  
  g_free (info);
}

static StockItemInfo*
stock_item_info_copy (StockItemInfo *src)
{
  StockItemInfo *info;

  info = g_new (StockItemInfo, 1);
  info->id = g_strdup (src->id);
  info->macro = g_strdup (src->macro);
  info->accel_str = g_strdup (src->accel_str);
  
  info->item = src->item;

  info->small_icon = src->small_icon;
  if (info->small_icon)
    g_object_ref (info->small_icon);

  return info;
}

static GType
stock_item_info_get_type (void)
{
  static GType our_type = 0;
  
  if (our_type == 0)
    our_type = g_boxed_type_register_static ("StockItemInfo",
                                             (GBoxedCopyFunc) stock_item_info_copy,
                                             (GBoxedFreeFunc) stock_item_info_free);

  return our_type;
}

typedef struct _StockItemDisplay StockItemDisplay;
struct _StockItemDisplay
{
  BtkWidget *type_label;
  BtkWidget *macro_label;
  BtkWidget *id_label;
  BtkWidget *label_accel_label;
  BtkWidget *icon_image;
};

static gchar*
id_to_macro (const gchar *id)
{
  GString *macro = NULL;
  const gchar *cp;

  /* btk-foo-bar -> BTK_STOCK_FOO_BAR */

  macro = g_string_new (NULL);
  
  cp = id;
  
  if (strncmp (cp, "btk-", 4) == 0)
    {
      g_string_append (macro, "BTK_STOCK_");
      cp += 4;
    }

  while (*cp)
    {
      if (*cp == '-')
	g_string_append_c (macro, '_');
      else if (g_ascii_islower (*cp))
	g_string_append_c (macro, g_ascii_toupper (*cp));
      else
	g_string_append_c (macro, *cp);

      cp++;
    }

  return g_string_free (macro, FALSE);
}

static BtkTreeModel*
create_model (void)
{
  BtkListStore *store;
  GSList *ids;
  GSList *tmp_list;
  
  store = btk_list_store_new (2, STOCK_ITEM_INFO_TYPE, B_TYPE_STRING);

  ids = btk_stock_list_ids ();
  ids = b_slist_sort (ids, (GCompareFunc) strcmp);
  tmp_list = ids;
  while (tmp_list != NULL)
    {
      StockItemInfo info;
      BtkStockItem item;
      BtkTreeIter iter;
      BtkIconSet *icon_set;
      
      info.id = tmp_list->data;
      
      if (btk_stock_lookup (info.id, &item))
        {
          info.item = item;
        }
      else
        {
          info.item.label = NULL;
          info.item.stock_id = NULL;
          info.item.modifier = 0;
          info.item.keyval = 0;
          info.item.translation_domain = NULL;
        }

      /* only show icons for stock IDs that have default icons */
      icon_set = btk_icon_factory_lookup_default (info.id);
      if (icon_set)
        {
          BtkIconSize *sizes = NULL;
          gint n_sizes = 0;
          gint i;
          BtkIconSize size;

          /* See what sizes this stock icon really exists at */
          btk_icon_set_get_sizes (icon_set, &sizes, &n_sizes);

          /* Use menu size if it exists, otherwise first size found */
          size = sizes[0];
          i = 0;
          while (i < n_sizes)
            {
              if (sizes[i] == BTK_ICON_SIZE_MENU)
                {
                  size = BTK_ICON_SIZE_MENU;
                  break;
                }
              ++i;
            }
          g_free (sizes);
          
          info.small_icon = btk_widget_render_icon (window, info.id,
                                                    size,
                                                    NULL);
          
          if (size != BTK_ICON_SIZE_MENU)
            {
              /* Make the result the proper size for our thumbnail */
              gint w, h;
              BdkPixbuf *scaled;
              
              btk_icon_size_lookup (BTK_ICON_SIZE_MENU, &w, &h);
              
              scaled = bdk_pixbuf_scale_simple (info.small_icon,
                                                w, h,
                                                BDK_INTERP_BILINEAR);

              g_object_unref (info.small_icon);
              info.small_icon = scaled;
            }
        }
      else
        info.small_icon = NULL;

      if (info.item.keyval != 0)
        {
          info.accel_str = btk_accelerator_name (info.item.keyval,
                                                 info.item.modifier);
        }
      else
        {
          info.accel_str = g_strdup ("");
        }

      info.macro = id_to_macro (info.id);
      
      btk_list_store_append (store, &iter);
      btk_list_store_set (store, &iter, 0, &info, 1, info.id, -1);

      g_free (info.macro);
      g_free (info.accel_str);
      if (info.small_icon)
        g_object_unref (info.small_icon);
      
      tmp_list = b_slist_next (tmp_list);
    }
  
  b_slist_foreach (ids, (GFunc)g_free, NULL);
  b_slist_free (ids);

  return BTK_TREE_MODEL (store);
}

/* Finds the largest size at which the given image stock id is
 * available. This would not be useful for a normal application
 */
static BtkIconSize
get_largest_size (const char *id)
{
  BtkIconSet *set = btk_icon_factory_lookup_default (id);
  BtkIconSize *sizes;
  gint n_sizes, i;
  BtkIconSize best_size = BTK_ICON_SIZE_INVALID;
  gint best_pixels = 0;

  btk_icon_set_get_sizes (set, &sizes, &n_sizes);

  for (i = 0; i < n_sizes; i++)
    {
      gint width, height;
      
      btk_icon_size_lookup (sizes[i], &width, &height);

      if (width * height > best_pixels)
	{
	  best_size = sizes[i];
	  best_pixels = width * height;
	}
    }
  
  g_free (sizes);

  return best_size;
}

static void
selection_changed (BtkTreeSelection *selection)
{
  BtkTreeView *treeview;
  StockItemDisplay *display;
  BtkTreeModel *model;
  BtkTreeIter iter;
  
  treeview = btk_tree_selection_get_tree_view (selection);
  display = g_object_get_data (B_OBJECT (treeview), "stock-display");

  if (btk_tree_selection_get_selected (selection, &model, &iter))
    {
      StockItemInfo *info;
      gchar *str;
      
      btk_tree_model_get (model, &iter,
                          0, &info,
                          -1);

      if (info->small_icon && info->item.label)
        btk_label_set_text (BTK_LABEL (display->type_label), "Icon and Item");
      else if (info->small_icon)
        btk_label_set_text (BTK_LABEL (display->type_label), "Icon Only");
      else if (info->item.label)
        btk_label_set_text (BTK_LABEL (display->type_label), "Item Only");
      else
        btk_label_set_text (BTK_LABEL (display->type_label), "???????");

      btk_label_set_text (BTK_LABEL (display->macro_label), info->macro);
      btk_label_set_text (BTK_LABEL (display->id_label), info->id);

      if (info->item.label)
        {
          str = g_strdup_printf ("%s %s", info->item.label, info->accel_str);
          btk_label_set_text_with_mnemonic (BTK_LABEL (display->label_accel_label), str);
          g_free (str);
        }
      else
        {
          btk_label_set_text (BTK_LABEL (display->label_accel_label), "");
        }

      if (info->small_icon)
        btk_image_set_from_stock (BTK_IMAGE (display->icon_image), info->id,
                                  get_largest_size (info->id));
      else
        btk_image_set_from_pixbuf (BTK_IMAGE (display->icon_image), NULL);

      stock_item_info_free (info);
    }
  else
    {
      btk_label_set_text (BTK_LABEL (display->type_label), "No selected item");
      btk_label_set_text (BTK_LABEL (display->macro_label), "");
      btk_label_set_text (BTK_LABEL (display->id_label), "");
      btk_label_set_text (BTK_LABEL (display->label_accel_label), "");
      btk_image_set_from_pixbuf (BTK_IMAGE (display->icon_image), NULL);
    }
}

static void
macro_set_func_text (BtkTreeViewColumn *tree_column,
		     BtkCellRenderer   *cell,
		     BtkTreeModel      *model,
		     BtkTreeIter       *iter,
		     gpointer           data)
{
  StockItemInfo *info;
  
  btk_tree_model_get (model, iter,
                      0, &info,
                      -1);
  
  g_object_set (BTK_CELL_RENDERER (cell),
                "text", info->macro,
                NULL);
  
  stock_item_info_free (info);
}

static void
id_set_func (BtkTreeViewColumn *tree_column,
             BtkCellRenderer   *cell,
             BtkTreeModel      *model,
             BtkTreeIter       *iter,
             gpointer           data)
{
  StockItemInfo *info;
  
  btk_tree_model_get (model, iter,
                      0, &info,
                      -1);
  
  g_object_set (BTK_CELL_RENDERER (cell),
                "text", info->id,
                NULL);
  
  stock_item_info_free (info);
}

static void
accel_set_func (BtkTreeViewColumn *tree_column,
                BtkCellRenderer   *cell,
                BtkTreeModel      *model,
                BtkTreeIter       *iter,
                gpointer           data)
{
  StockItemInfo *info;
  
  btk_tree_model_get (model, iter,
                      0, &info,
                      -1);
  
  g_object_set (BTK_CELL_RENDERER (cell),
                "text", info->accel_str,
                NULL);
  
  stock_item_info_free (info);
}

static void
label_set_func (BtkTreeViewColumn *tree_column,
                BtkCellRenderer   *cell,
                BtkTreeModel      *model,
                BtkTreeIter       *iter,
                gpointer           data)
{
  StockItemInfo *info;
  
  btk_tree_model_get (model, iter,
                      0, &info,
                      -1);
  
  g_object_set (BTK_CELL_RENDERER (cell),
                "text", info->item.label,
                NULL);
  
  stock_item_info_free (info);
}

BtkWidget *
do_stock_browser (BtkWidget *do_widget)
{  
  if (!window)
    {
      BtkWidget *frame;
      BtkWidget *vbox;
      BtkWidget *hbox;
      BtkWidget *sw;
      BtkWidget *treeview;
      BtkWidget *align;
      BtkTreeModel *model;
      BtkCellRenderer *cell_renderer;
      StockItemDisplay *display;
      BtkTreeSelection *selection;
      BtkTreeViewColumn *column;

      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (do_widget));
      btk_window_set_title (BTK_WINDOW (window), "Stock Icons and Items");
      btk_window_set_default_size (BTK_WINDOW (window), -1, 500);

      g_signal_connect (window, "destroy", G_CALLBACK (btk_widget_destroyed), &window);
      btk_container_set_border_width (BTK_CONTAINER (window), 8);

      hbox = btk_hbox_new (FALSE, 8);
      btk_container_add (BTK_CONTAINER (window), hbox);

      sw = btk_scrolled_window_new (NULL, NULL);
      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (sw),
                                      BTK_POLICY_NEVER,
                                      BTK_POLICY_AUTOMATIC);
      btk_box_pack_start (BTK_BOX (hbox), sw, FALSE, FALSE, 0);

      model = create_model ();
      
      treeview = btk_tree_view_new_with_model (model);

      g_object_unref (model);

      btk_container_add (BTK_CONTAINER (sw), treeview);
      
      column = btk_tree_view_column_new ();
      btk_tree_view_column_set_title (column, "Macro");

      cell_renderer = btk_cell_renderer_pixbuf_new ();
      btk_tree_view_column_pack_start (column,
				       cell_renderer,
				       FALSE);
      btk_tree_view_column_set_attributes (column, cell_renderer,
					   "stock_id", 1, NULL);
      cell_renderer = btk_cell_renderer_text_new ();
      btk_tree_view_column_pack_start (column,
				       cell_renderer,
				       TRUE);
      btk_tree_view_column_set_cell_data_func (column, cell_renderer,
					       macro_set_func_text, NULL, NULL);

      btk_tree_view_append_column (BTK_TREE_VIEW (treeview),
				   column);

      cell_renderer = btk_cell_renderer_text_new ();
      btk_tree_view_insert_column_with_data_func (BTK_TREE_VIEW (treeview),
                                                  -1,
                                                  "Label",
                                                  cell_renderer,
                                                  label_set_func,
                                                  NULL,
                                                  NULL);

      cell_renderer = btk_cell_renderer_text_new ();
      btk_tree_view_insert_column_with_data_func (BTK_TREE_VIEW (treeview),
                                                  -1,
                                                  "Accel",
                                                  cell_renderer,
                                                  accel_set_func,
                                                  NULL,
                                                  NULL);

      cell_renderer = btk_cell_renderer_text_new ();
      btk_tree_view_insert_column_with_data_func (BTK_TREE_VIEW (treeview),
                                                  -1,
                                                  "ID",
                                                  cell_renderer,
                                                  id_set_func,
                                                  NULL,
                                                  NULL);
      
      align = btk_alignment_new (0.5, 0.0, 0.0, 0.0);
      btk_box_pack_end (BTK_BOX (hbox), align, FALSE, FALSE, 0);
      
      frame = btk_frame_new ("Selected Item");
      btk_container_add (BTK_CONTAINER (align), frame);

      vbox = btk_vbox_new (FALSE, 8);
      btk_container_set_border_width (BTK_CONTAINER (vbox), 4);
      btk_container_add (BTK_CONTAINER (frame), vbox);

      display = g_new (StockItemDisplay, 1);
      g_object_set_data_full (B_OBJECT (treeview),
                              "stock-display",
                              display,
                              g_free); /* free display with treeview */
      
      display->type_label = btk_label_new (NULL);
      display->macro_label = btk_label_new (NULL);
      display->id_label = btk_label_new (NULL);
      display->label_accel_label = btk_label_new (NULL);
      display->icon_image = btk_image_new_from_pixbuf (NULL); /* empty image */

      btk_box_pack_start (BTK_BOX (vbox), display->type_label,
                          FALSE, FALSE, 0);

      btk_box_pack_start (BTK_BOX (vbox), display->icon_image,
                          FALSE, FALSE, 0);
      
      btk_box_pack_start (BTK_BOX (vbox), display->label_accel_label,
                          FALSE, FALSE, 0);
      btk_box_pack_start (BTK_BOX (vbox), display->macro_label,
                          FALSE, FALSE, 0);
      btk_box_pack_start (BTK_BOX (vbox), display->id_label,
                          FALSE, FALSE, 0);

      selection = btk_tree_view_get_selection (BTK_TREE_VIEW (treeview));
      btk_tree_selection_set_mode (selection, BTK_SELECTION_SINGLE);
      
      g_signal_connect (selection,
			"changed",
			G_CALLBACK (selection_changed),
			NULL);
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
