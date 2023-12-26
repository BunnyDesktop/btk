/* Combo boxes 
 *
 * The ComboBox widget allows to select one option out of a list.
 * The ComboBoxEntry additionally allows the user to enter a value
 * that is not in the list of options. 
 *
 * How the options are displayed is controlled by cell renderers.
 */

#include <btk/btk.h>

enum 
{
  PIXBUF_COL,
  TEXT_COL
};

static bchar *
strip_underscore (const bchar *text)
{
  bchar *p, *q;
  bchar *result;
  
  result = g_strdup (text);
  p = q = result;
  while (*p) 
    {
      if (*p != '_')
	{
	  *q = *p;
	  q++;
	}
      p++;
    }
  *q = '\0';

  return result;
}

static BtkTreeModel *
create_stock_icon_store (void)
{
  bchar *stock_id[6] = {
    BTK_STOCK_DIALOG_WARNING,
    BTK_STOCK_STOP,
    BTK_STOCK_NEW,
    BTK_STOCK_CLEAR,
    NULL,
    BTK_STOCK_OPEN    
  };

  BtkStockItem item;
  BdkPixbuf *pixbuf;
  BtkWidget *cellview;
  BtkTreeIter iter;
  BtkListStore *store;
  bchar *label;
  bint i;

  cellview = btk_cell_view_new ();
  
  store = btk_list_store_new (2, BDK_TYPE_PIXBUF, B_TYPE_STRING);

  for (i = 0; i < G_N_ELEMENTS (stock_id); i++)
    {
      if (stock_id[i])
	{
	  pixbuf = btk_widget_render_icon (cellview, stock_id[i],
					   BTK_ICON_SIZE_BUTTON, NULL);
	  btk_stock_lookup (stock_id[i], &item);
	  label = strip_underscore (item.label);
	  btk_list_store_append (store, &iter);
	  btk_list_store_set (store, &iter,
			      PIXBUF_COL, pixbuf,
			      TEXT_COL, label,
			      -1);
	  g_object_unref (pixbuf);
	  g_free (label);
	}
      else
	{
	  btk_list_store_append (store, &iter);
	  btk_list_store_set (store, &iter,
			      PIXBUF_COL, NULL,
			      TEXT_COL, "separator",
			      -1);
	}
    }

  btk_widget_destroy (cellview);
  
  return BTK_TREE_MODEL (store);
}

/* A BtkCellLayoutDataFunc that demonstrates how one can control
 * sensitivity of rows. This particular function does nothing 
 * useful and just makes the second row insensitive.
 */
static void
set_sensitive (BtkCellLayout   *cell_layout,
	       BtkCellRenderer *cell,
	       BtkTreeModel    *tree_model,
	       BtkTreeIter     *iter,
	       bpointer         data)
{
  BtkTreePath *path;
  bint *indices;
  bboolean sensitive;

  path = btk_tree_model_get_path (tree_model, iter);
  indices = btk_tree_path_get_indices (path);
  sensitive = indices[0] != 1;
  btk_tree_path_free (path);

  g_object_set (cell, "sensitive", sensitive, NULL);
}

/* A BtkTreeViewRowSeparatorFunc that demonstrates how rows can be
 * rendered as separators. This particular function does nothing 
 * useful and just turns the fourth row into a separator.
 */
static bboolean
is_separator (BtkTreeModel *model,
	      BtkTreeIter  *iter,
	      bpointer      data)
{
  BtkTreePath *path;
  bboolean result;

  path = btk_tree_model_get_path (model, iter);
  result = btk_tree_path_get_indices (path)[0] == 4;
  btk_tree_path_free (path);

  return result;
}

static BtkTreeModel *
create_capital_store (void)
{
  struct {
    bchar *group;
    bchar *capital;
  } capitals[] = {
    { "A - B", NULL }, 
    { NULL, "Albany" },
    { NULL, "Annapolis" },
    { NULL, "Atlanta" },
    { NULL, "Augusta" }, 
    { NULL, "Austin" },
    { NULL, "Baton Rouge" },
    { NULL, "Bismarck" },
    { NULL, "Boise" },
    { NULL, "Boston" },
    { "C - D", NULL },
    { NULL, "Carson City" },
    { NULL, "Charleston" },
    { NULL, "Cheyenne" },
    { NULL, "Columbia" },
    { NULL, "Columbus" },
    { NULL, "Concord" },
    { NULL, "Denver" },
    { NULL, "Des Moines" },
    { NULL, "Dover" },
    { "E - J", NULL },
    { NULL, "Frankfort" },
    { NULL, "Harrisburg" },
    { NULL, "Hartford" },
    { NULL, "Helena" },
    { NULL, "Honolulu" },
    { NULL, "Indianapolis" },
    { NULL, "Jackson" },
    { NULL, "Jefferson City" },
    { NULL, "Juneau" },
    { "K - O" },
    { NULL, "Lansing" },
    { NULL, "Lincoln" },
    { NULL, "Little Rock" },
    { NULL, "Madison" },
    { NULL, "Montgomery" },
    { NULL, "Montpelier" },
    { NULL, "Nashville" },
    { NULL, "Oklahoma City" },
    { NULL, "Olympia" },
    { NULL, "P - S" },
    { NULL, "Phoenix" },
    { NULL, "Pierre" },
    { NULL, "Providence" },
    { NULL, "Raleigh" },
    { NULL, "Richmond" },
    { NULL, "Sacramento" },
    { NULL, "Salem" },
    { NULL, "Salt Lake City" },
    { NULL, "Santa Fe" },
    { NULL, "Springfield" },
    { NULL, "St. Paul" },
    { "T - Z", NULL },
    { NULL, "Tallahassee" },
    { NULL, "Topeka" },
    { NULL, "Trenton" },
    { NULL, NULL }
  };
  
  BtkTreeIter iter, iter2;
  BtkTreeStore *store;
  bint i;

  store = btk_tree_store_new (1, B_TYPE_STRING);
  
  for (i = 0; capitals[i].group || capitals[i].capital; i++)
    {
      if (capitals[i].group)
	{
	  btk_tree_store_append (store, &iter, NULL);
	  btk_tree_store_set (store, &iter, 0, capitals[i].group, -1);
	}
      else if (capitals[i].capital)
	{
	  btk_tree_store_append (store, &iter2, &iter);
	  btk_tree_store_set (store, &iter2, 0, capitals[i].capital, -1);
	}
    }
  
  return BTK_TREE_MODEL (store);
}

static void
is_capital_sensitive (BtkCellLayout   *cell_layout,
		      BtkCellRenderer *cell,
		      BtkTreeModel    *tree_model,
		      BtkTreeIter     *iter,
		      bpointer         data)
{
  bboolean sensitive;

  sensitive = !btk_tree_model_iter_has_child (tree_model, iter);

  g_object_set (cell, "sensitive", sensitive, NULL);
}

static void
fill_combo_entry (BtkWidget *combo)
{
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo), "One");
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo), "Two");
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo), "2\302\275");
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo), "Three");
}


/* A simple validating entry */

#define TYPE_MASK_ENTRY             (mask_entry_get_type ())
#define MASK_ENTRY(obj)             (B_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_MASK_ENTRY, MaskEntry))
#define MASK_ENTRY_CLASS(vtable)    (B_TYPE_CHECK_CLASS_CAST ((vtable), TYPE_MASK_ENTRY, MaskEntryClass))
#define IS_MASK_ENTRY(obj)          (B_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_MASK_ENTRY))
#define IS_MASK_ENTRY_CLASS(vtable) (B_TYPE_CHECK_CLASS_TYPE ((vtable), TYPE_MASK_ENTRY))
#define MASK_ENTRY_GET_CLASS(inst)  (B_TYPE_INSTANCE_GET_CLASS ((inst), TYPE_MASK_ENTRY, MaskEntryClass))


typedef struct _MaskEntry MaskEntry;
struct _MaskEntry
{
  BtkEntry entry;
  bchar *mask;
};

typedef struct _MaskEntryClass MaskEntryClass;
struct _MaskEntryClass
{
  BtkEntryClass parent_class;
};


static void mask_entry_editable_init (BtkEditableClass *iface);

G_DEFINE_TYPE_WITH_CODE (MaskEntry, mask_entry, BTK_TYPE_ENTRY,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_EDITABLE,
						mask_entry_editable_init));


static void
mask_entry_set_background (MaskEntry *entry)
{
  static const BdkColor error_color = { 0, 65535, 60000, 60000 };

  if (entry->mask)
    {
      if (!g_regex_match_simple (entry->mask, btk_entry_get_text (BTK_ENTRY (entry)), 0, 0))
	{
	  btk_widget_modify_base (BTK_WIDGET (entry), BTK_STATE_NORMAL, &error_color);
	  return;
	}
    }

  btk_widget_modify_base (BTK_WIDGET (entry), BTK_STATE_NORMAL, NULL);
}


static void
mask_entry_changed (BtkEditable *editable)
{
  mask_entry_set_background (MASK_ENTRY (editable));
}


static void
mask_entry_init (MaskEntry *entry)
{
  entry->mask = NULL;
}


static void
mask_entry_class_init (MaskEntryClass *klass)
{ }


static void
mask_entry_editable_init (BtkEditableClass *iface)
{
  iface->changed = mask_entry_changed;
}


BtkWidget *
do_combobox (BtkWidget *do_widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *vbox, *frame, *box, *combo, *entry;
  BtkTreeModel *model;
  BtkCellRenderer *renderer;
  BtkTreePath *path;
  BtkTreeIter iter;

  if (!window)
  {
    window = btk_window_new (BTK_WINDOW_TOPLEVEL);
    btk_window_set_screen (BTK_WINDOW (window),
                           btk_widget_get_screen (do_widget));
    btk_window_set_title (BTK_WINDOW (window), "Combo boxes");
   
    g_signal_connect (window, "destroy",
                      G_CALLBACK (btk_widget_destroyed),
                      &window);
    
    btk_container_set_border_width (BTK_CONTAINER (window), 10);

    vbox = btk_vbox_new (FALSE, 2);
    btk_container_add (BTK_CONTAINER (window), vbox);

    /* A combobox demonstrating cell renderers, separators and
     *  insensitive rows 
     */
    frame = btk_frame_new ("Some stock icons");
    btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);
    
    box = btk_vbox_new (FALSE, 0);
    btk_container_set_border_width (BTK_CONTAINER (box), 5);
    btk_container_add (BTK_CONTAINER (frame), box);
    
    model = create_stock_icon_store ();
    combo = btk_combo_box_new_with_model (model);
    g_object_unref (model);
    btk_container_add (BTK_CONTAINER (box), combo);
    
    renderer = btk_cell_renderer_pixbuf_new ();
    btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combo), renderer, FALSE);
    btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (combo), renderer,
				    "pixbuf", PIXBUF_COL, 
				    NULL);

    btk_cell_layout_set_cell_data_func (BTK_CELL_LAYOUT (combo),
					renderer,
					set_sensitive,
					NULL, NULL);
    
    renderer = btk_cell_renderer_text_new ();
    btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combo), renderer, TRUE);
    btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (combo), renderer,
				    "text", TEXT_COL,
				    NULL);

    btk_cell_layout_set_cell_data_func (BTK_CELL_LAYOUT (combo),
					renderer,
					set_sensitive,
					NULL, NULL);

    btk_combo_box_set_row_separator_func (BTK_COMBO_BOX (combo), 
					  is_separator, NULL, NULL);
    
    btk_combo_box_set_active (BTK_COMBO_BOX (combo), 0);
    
    /* A combobox demonstrating trees.
     */
    frame = btk_frame_new ("Where are we ?");
    btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);

    box = btk_vbox_new (FALSE, 0);
    btk_container_set_border_width (BTK_CONTAINER (box), 5);
    btk_container_add (BTK_CONTAINER (frame), box);
    
    model = create_capital_store ();
    combo = btk_combo_box_new_with_model (model);
    g_object_unref (model);
    btk_container_add (BTK_CONTAINER (box), combo);

    renderer = btk_cell_renderer_text_new ();
    btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combo), renderer, TRUE);
    btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (combo), renderer,
				    "text", 0,
				    NULL);
    btk_cell_layout_set_cell_data_func (BTK_CELL_LAYOUT (combo),
					renderer,
					is_capital_sensitive,
					NULL, NULL);

    path = btk_tree_path_new_from_indices (0, 8, -1);
    btk_tree_model_get_iter (model, &iter, path);
    btk_tree_path_free (path);
    btk_combo_box_set_active_iter (BTK_COMBO_BOX (combo), &iter);

    /* A BtkComboBoxEntry with validation.
     */
    frame = btk_frame_new ("Editable");
    btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);
    
    box = btk_vbox_new (FALSE, 0);
    btk_container_set_border_width (BTK_CONTAINER (box), 5);
    btk_container_add (BTK_CONTAINER (frame), box);
    
    combo = btk_combo_box_text_new_with_entry ();
    fill_combo_entry (combo);
    btk_container_add (BTK_CONTAINER (box), combo);
    
    entry = g_object_new (TYPE_MASK_ENTRY, NULL);
    MASK_ENTRY (entry)->mask = "^([0-9]*|One|Two|2\302\275|Three)$";
     
    btk_container_remove (BTK_CONTAINER (combo), btk_bin_get_child (BTK_BIN (combo)));
    btk_container_add (BTK_CONTAINER (combo), entry);
  
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
