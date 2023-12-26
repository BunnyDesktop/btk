#undef BDK_DISABLE_DEPRECATED
#include <btk/btk.h>
#ifdef BDK_WINDOWING_X11
#include <X11/Xlib.h>
#endif

static BtkWidget *darea;
static BtkTreeStore *window_store = NULL;
static BtkWidget *treeview;

static void update_store (void);

static BtkWidget *main_window;

static gboolean
window_has_impl (BdkWindow *window)
{
  BdkWindowObject *w;
  w = (BdkWindowObject *)window;
  return w->parent == NULL || w->parent->impl != w->impl;
}

BdkWindow *
create_window (BdkWindow *parent,
	       int x, int y, int w, int h,
	       BdkColor *color)
{
  BdkWindowAttr attributes;
  gint attributes_mask;
  BdkWindow *window;
  BdkColor *bg;

  attributes.x = x;
  attributes.y = y;
  attributes.width = w;
  attributes.height = h;
  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.event_mask = BDK_STRUCTURE_MASK
			| BDK_BUTTON_MOTION_MASK
			| BDK_BUTTON_PRESS_MASK
			| BDK_BUTTON_RELEASE_MASK
			| BDK_EXPOSURE_MASK
			| BDK_ENTER_NOTIFY_MASK
			| BDK_LEAVE_NOTIFY_MASK;
  attributes.wclass = BDK_INPUT_OUTPUT;
      
  attributes_mask = BDK_WA_X | BDK_WA_Y;
      
  window = bdk_window_new (parent, &attributes, attributes_mask);
  bdk_window_set_user_data (window, darea);

  bg = g_new (BdkColor, 1);
  if (color)
    *bg = *color;
  else
    {
      bg->red = g_random_int_range (0, 0xffff);
      bg->blue = g_random_int_range (0, 0xffff);
      bg->green = g_random_int_range (0, 0xffff);;
    }
  
  bdk_rgb_find_color (btk_widget_get_colormap (darea), bg);
  bdk_window_set_background (window, bg);
  g_object_set_data_full (B_OBJECT (window), "color", bg, g_free);
  
  bdk_window_show (window);
  
  return window;
}

static void
add_window_cb (BtkTreeModel      *model,
	       BtkTreePath       *path,
	       BtkTreeIter       *iter,
	       gpointer           data)
{
  GList **selected = data;
  BdkWindow *window;

  btk_tree_model_get (BTK_TREE_MODEL (window_store),
		      iter,
		      0, &window,
		      -1);

  *selected = g_list_prepend (*selected, window);
}

static GList *
get_selected_windows (void)
{
  BtkTreeSelection *sel;
  GList *selected;

  sel = btk_tree_view_get_selection (BTK_TREE_VIEW (treeview));

  selected = NULL;
  btk_tree_selection_selected_foreach (sel, add_window_cb, &selected);
  
  return selected;
}

static gboolean
find_window_helper (BtkTreeModel *model,
		    BdkWindow *window,
		    BtkTreeIter *iter,
		    BtkTreeIter *selected_iter)
{
  BtkTreeIter child_iter;
  BdkWindow *w;

  do
    {
      btk_tree_model_get (model, iter,
			  0, &w,
			  -1);
      if (w == window)
	{
	  *selected_iter = *iter;
	  return TRUE;
	}
      
      if (btk_tree_model_iter_children (model,
					&child_iter,
					iter))
	{
	  if (find_window_helper (model, window, &child_iter, selected_iter))
	    return TRUE;
	}
    } while (btk_tree_model_iter_next (model, iter));

  return FALSE;
}

static gboolean
find_window (BdkWindow *window,
	     BtkTreeIter *window_iter)
{
  BtkTreeIter iter;

  if (!btk_tree_model_get_iter_first  (BTK_TREE_MODEL (window_store), &iter))
    return FALSE;

  return find_window_helper (BTK_TREE_MODEL (window_store),
			     window,
			     &iter,
			     window_iter);
}

static void
toggle_selection_window (BdkWindow *window)
{
  BtkTreeSelection *selection;
  BtkTreeIter iter;

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (treeview));

  if (window != NULL &&
      find_window (window, &iter))
    {
      if (btk_tree_selection_iter_is_selected (selection, &iter))
	btk_tree_selection_unselect_iter (selection,  &iter);
      else
	btk_tree_selection_select_iter (selection,  &iter);
    }
}

static void
unselect_windows (void)
{
  BtkTreeSelection *selection;

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (treeview));
  
  btk_tree_selection_unselect_all (selection);
}


static void
select_window (BdkWindow *window)
{
  BtkTreeSelection *selection;
  BtkTreeIter iter;

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (treeview));

  if (window != NULL &&
      find_window (window, &iter))
    btk_tree_selection_select_iter (selection,  &iter);
}

static void
select_windows (GList *windows)
{
  BtkTreeSelection *selection;
  BtkTreeIter iter;
  GList *l;

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (treeview));
  btk_tree_selection_unselect_all (selection);
  
  for (l = windows; l != NULL; l = l->next)
    {
      if (find_window (l->data, &iter))
	btk_tree_selection_select_iter (selection,  &iter);
    }
}

static void
add_window_clicked (BtkWidget *button, 
		    gpointer data)
{
  BdkWindow *parent;
  GList *l;

  l = get_selected_windows ();
  if (l != NULL)
    parent = l->data;
  else
    parent = darea->window;

  g_list_free (l);
  
  create_window (parent, 10, 10, 100, 100, NULL);
  update_store ();
}

static void
draw_drawable_clicked (BtkWidget *button, 
		       gpointer data)
{
  BdkGC *gc;
  gc = bdk_gc_new (darea->window);
  bdk_draw_drawable (darea->window,
		     gc,
		     darea->window,
		     -15, -15,
		     40, 70,
		     100, 100);
  g_object_unref (gc);
}


static void
remove_window_clicked (BtkWidget *button, 
		       gpointer data)
{
  GList *l, *selected;

  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    bdk_window_destroy (l->data);

  g_list_free (selected);

  update_store ();
}

static void save_children (GString *s, BdkWindow *window);

static void
save_window (GString *s,
	     BdkWindow *window)
{
  gint x, y, w, h;
  BdkColor *color;

  bdk_window_get_position (window, &x, &y);
  bdk_drawable_get_size (BDK_DRAWABLE (window), &w, &h);
  color = g_object_get_data (B_OBJECT (window), "color");
  
  g_string_append_printf (s, "%d,%d %dx%d (%d,%d,%d) %d %d\n",
			  x, y, w, h,
			  color->red, color->green, color->blue,
			  window_has_impl (window),
			  g_list_length (bdk_window_peek_children (window)));

  save_children (s, window);
}


static void
save_children (GString *s,
	       BdkWindow *window)
{
  GList *l;
  BdkWindow *child;

  for (l = g_list_reverse (bdk_window_peek_children (window));
       l != NULL;
       l = l->next)
    {
      child = l->data;

      save_window (s, child);
    }
}


static void
save_clicked (BtkWidget *button, 
	      gpointer data)
{
  GString *s;
  BtkWidget *dialog;
  GFile *file;

  s = g_string_new ("");

  save_children (s, darea->window);

  dialog = btk_file_chooser_dialog_new ("Filename for window data",
					NULL,
					BTK_FILE_CHOOSER_ACTION_SAVE,
					BTK_STOCK_CANCEL, BTK_RESPONSE_CANCEL,
					BTK_STOCK_SAVE, BTK_RESPONSE_ACCEPT,
					NULL);
  
  btk_file_chooser_set_do_overwrite_confirmation (BTK_FILE_CHOOSER (dialog), TRUE);
  
  if (btk_dialog_run (BTK_DIALOG (dialog)) == BTK_RESPONSE_ACCEPT)
    {
      file = btk_file_chooser_get_file (BTK_FILE_CHOOSER (dialog));

      g_file_replace_contents (file,
			       s->str, s->len,
			       NULL, FALSE,
			       0, NULL, NULL, NULL);

      g_object_unref (file);
    }

  btk_widget_destroy (dialog);
  g_string_free (s, TRUE);
}

static void
destroy_children (BdkWindow *window)
{
  GList *l;
  BdkWindow *child;

  for (l = bdk_window_peek_children (window);
       l != NULL;
       l = l->next)
    {
      child = l->data;
      
      destroy_children (child);
      bdk_window_destroy (child);
    }
}

static char **
parse_window (BdkWindow *parent, char **lines)
{
  int x, y, w, h, r, g, b, native, n_children;
  BdkWindow *window;
  BdkColor color;
  int i;

  if (*lines == NULL)
    return lines;
  
  if (sscanf(*lines, "%d,%d %dx%d (%d,%d,%d) %d %d",
	     &x, &y, &w, &h, &r, &g, &b, &native, &n_children) == 9)
    {
      lines++;
      color.red = r;
      color.green = g;
      color.blue = b;
      window = create_window (parent, x, y, w, h, &color);
      if (native)
	bdk_window_ensure_native (window);
      
      for (i = 0; i < n_children; i++)
	lines = parse_window (window, lines);
    }
  else
    lines++;
  
  return lines;
}
  
static void
load_file (GFile *file)
{
  char *data;
  char **lines, **l;
  
  if (g_file_load_contents (file, NULL, &data, NULL, NULL, NULL))
    {
      destroy_children (darea->window);

      lines = g_strsplit (data, "\n", -1);

      l = lines;
      while (*l != NULL)
	l = parse_window (darea->window, l);
    }

  update_store ();
}

static void
move_window_clicked (BtkWidget *button, 
		     gpointer data)
{
  BdkWindow *window;
  BtkDirectionType direction;
  GList *selected, *l;
  gint x, y;

  direction = GPOINTER_TO_INT (data);
    
  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    {
      window = l->data;
      
      bdk_window_get_position (window, &x, &y);
      
      switch (direction) {
      case BTK_DIR_UP:
	y -= 10;
	break;
      case BTK_DIR_DOWN:
	y += 10;
	break;
      case BTK_DIR_LEFT:
	x -= 10;
	break;
      case BTK_DIR_RIGHT:
	x += 10;
	break;
      default:
	break;
      }

      bdk_window_move (window, x, y);
    }

  g_list_free (selected);
}

static void
manual_clicked (BtkWidget *button, 
		gpointer data)
{
  BdkWindow *window;
  GList *selected, *l;
  int x, y, w, h;
  BtkWidget *dialog, *table, *label, *xspin, *yspin, *wspin, *hspin;
  

  selected = get_selected_windows ();

  if (selected == NULL)
    return;

  bdk_window_get_position (selected->data, &x, &y);
  bdk_drawable_get_size (selected->data, &w, &h);

  dialog = btk_dialog_new_with_buttons ("Select new position and size",
					BTK_WINDOW (main_window),
					BTK_DIALOG_MODAL,
					BTK_STOCK_OK, BTK_RESPONSE_OK,
					NULL);
  

  table = btk_table_new (2, 4, TRUE);
  btk_box_pack_start (BTK_BOX (btk_dialog_get_content_area (BTK_DIALOG (dialog))),
		      table,
		      FALSE, FALSE,
		      2);

  
  label = btk_label_new ("x:");
  btk_table_attach_defaults (BTK_TABLE (table),
			     label,
			     0, 1,
			     0, 1);
  label = btk_label_new ("y:");
  btk_table_attach_defaults (BTK_TABLE (table),
			     label,
			     0, 1,
			     1, 2);
  label = btk_label_new ("width:");
  btk_table_attach_defaults (BTK_TABLE (table),
			     label,
			     0, 1,
			     2, 3);
  label = btk_label_new ("height:");
  btk_table_attach_defaults (BTK_TABLE (table),
			     label,
			     0, 1,
			     3, 4);

  xspin = btk_spin_button_new_with_range (G_MININT, G_MAXINT, 1);
  btk_spin_button_set_value (BTK_SPIN_BUTTON (xspin), x);
  btk_table_attach_defaults (BTK_TABLE (table),
			     xspin,
			     1, 2,
			     0, 1);
  yspin = btk_spin_button_new_with_range (G_MININT, G_MAXINT, 1);
  btk_spin_button_set_value (BTK_SPIN_BUTTON (yspin), y);
  btk_table_attach_defaults (BTK_TABLE (table),
			     yspin,
			     1, 2,
			     1, 2);
  wspin = btk_spin_button_new_with_range (G_MININT, G_MAXINT, 1);
  btk_spin_button_set_value (BTK_SPIN_BUTTON (wspin), w);
  btk_table_attach_defaults (BTK_TABLE (table),
			     wspin,
			     1, 2,
			     2, 3);
  hspin = btk_spin_button_new_with_range (G_MININT, G_MAXINT, 1);
  btk_spin_button_set_value (BTK_SPIN_BUTTON (hspin), h);
  btk_table_attach_defaults (BTK_TABLE (table),
			     hspin,
			     1, 2,
			     3, 4);
  
  btk_widget_show_all (dialog);
  
  btk_dialog_run (BTK_DIALOG (dialog));

  x = btk_spin_button_get_value_as_int (BTK_SPIN_BUTTON (xspin));
  y = btk_spin_button_get_value_as_int (BTK_SPIN_BUTTON (yspin));
  w = btk_spin_button_get_value_as_int (BTK_SPIN_BUTTON (wspin));
  h = btk_spin_button_get_value_as_int (BTK_SPIN_BUTTON (hspin));

  btk_widget_destroy (dialog);
  
  for (l = selected; l != NULL; l = l->next)
    {
      window = l->data;
      
      bdk_window_move_resize (window, x, y, w, h);
    }

  g_list_free (selected);
}

static void
restack_clicked (BtkWidget *button,
		 gpointer data)
{
  GList *selected;

  selected = get_selected_windows ();

  if (g_list_length (selected) != 2)
    {
      g_warning ("select two windows");
    }

  bdk_window_restack (selected->data,
		      selected->next->data,
		      GPOINTER_TO_INT (data));

  g_list_free (selected);

  update_store ();
}

static void
scroll_window_clicked (BtkWidget *button, 
		       gpointer data)
{
  BdkWindow *window;
  BtkDirectionType direction;
  GList *selected, *l;
  gint dx, dy;

  direction = GPOINTER_TO_INT (data);
    
  selected = get_selected_windows ();

  dx = 0; dy = 0;
  switch (direction) {
  case BTK_DIR_UP:
    dy = 10;
    break;
  case BTK_DIR_DOWN:
    dy = -10;
    break;
  case BTK_DIR_LEFT:
    dx = 10;
    break;
  case BTK_DIR_RIGHT:
    dx = -10;
    break;
  default:
    break;
  }
  
  for (l = selected; l != NULL; l = l->next)
    {
      window = l->data;

      bdk_window_scroll (window, dx, dy);
    }

  g_list_free (selected);
}


static void
raise_window_clicked (BtkWidget *button, 
		      gpointer data)
{
  GList *selected, *l;
  BdkWindow *window;
    
  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    {
      window = l->data;
      
      bdk_window_raise (window);
    }

  g_list_free (selected);
  
  update_store ();
}

static void
lower_window_clicked (BtkWidget *button, 
		      gpointer data)
{
  GList *selected, *l;
  BdkWindow *window;
    
  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    {
      window = l->data;
      
      bdk_window_lower (window);
    }

  g_list_free (selected);
  
  update_store ();
}


static void
smaller_window_clicked (BtkWidget *button, 
			gpointer data)
{
  GList *selected, *l;
  BdkWindow *window;
  int w, h;

  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    {
      window = l->data;
      
      bdk_drawable_get_size (BDK_DRAWABLE (window), &w, &h);
      
      w -= 10;
      h -= 10;
      if (w < 1)
	w = 1;
      if (h < 1)
	h = 1;
      
      bdk_window_resize (window, w, h);
    }

  g_list_free (selected);
}

static void
larger_window_clicked (BtkWidget *button, 
			gpointer data)
{
  GList *selected, *l;
  BdkWindow *window;
  int w, h;

  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    {
      window = l->data;
      
      bdk_drawable_get_size (BDK_DRAWABLE (window), &w, &h);
      
      w += 10;
      h += 10;
      
      bdk_window_resize (window, w, h);
    }

  g_list_free (selected);
}

static void
native_window_clicked (BtkWidget *button, 
			gpointer data)
{
  GList *selected, *l;
  BdkWindow *window;

  selected = get_selected_windows ();

  for (l = selected; l != NULL; l = l->next)
    {
      window = l->data;
      
      bdk_window_ensure_native (window);
    }
  
  g_list_free (selected);
  
  update_store ();
}

static gboolean
darea_button_release_event (BtkWidget *widget,
			    BdkEventButton *event)
{
  if ((event->state & BDK_CONTROL_MASK) != 0)
    {
      toggle_selection_window (event->window);
    }
  else
    {
      unselect_windows ();
      select_window (event->window);
    }
    
  return TRUE;
}

static void
render_window_cell (BtkTreeViewColumn *tree_column,
		    BtkCellRenderer   *cell,
		    BtkTreeModel      *tree_model,
		    BtkTreeIter       *iter,
		    gpointer           data)
{
  BdkWindow *window;
  char *name;

  btk_tree_model_get (BTK_TREE_MODEL (window_store),
		      iter,
		      0, &window,
		      -1);

  if (window_has_impl (window))
      name = g_strdup_printf ("%p (native)", window);
  else
      name = g_strdup_printf ("%p", window);
  g_object_set (cell,
		"text", name,
		"background-bdk", &((BdkWindowObject *)window)->bg_color,
		NULL);  
}

static void
add_children (BtkTreeStore *store,
	      BdkWindow *window,
	      BtkTreeIter *window_iter)
{
  GList *l;
  BtkTreeIter child_iter;

  for (l = bdk_window_peek_children (window);
       l != NULL;
       l = l->next)
    {
      btk_tree_store_append (store, &child_iter, window_iter);
      btk_tree_store_set (store, &child_iter,
			  0, l->data,
			  -1);

      add_children (store, l->data, &child_iter);
    }
}

static void
update_store (void)
{
  GList *selected;

  selected = get_selected_windows ();

  btk_tree_store_clear (window_store);

  add_children (window_store, darea->window, NULL);
  btk_tree_view_expand_all (BTK_TREE_VIEW (treeview));

  select_windows (selected);
  g_list_free (selected);
}


int
main (int argc, char **argv)
{
  BtkWidget *window, *vbox, *hbox, *frame;
  BtkWidget *button, *scrolled, *table;
  BtkTreeViewColumn *column;
  BtkCellRenderer *renderer;
  BdkColor black = {0};
  GFile *file;
  
  btk_init (&argc, &argv);

  main_window = window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_container_set_border_width (BTK_CONTAINER (window), 0);

  g_signal_connect (B_OBJECT (window), "delete-event", btk_main_quit, NULL);

  hbox = btk_hbox_new (FALSE, 5);
  btk_container_add (BTK_CONTAINER (window), hbox);
  btk_widget_show (hbox);

  frame = btk_frame_new ("BdkWindows");
  btk_box_pack_start (BTK_BOX (hbox),
		      frame,
		      FALSE, FALSE,
		      5);
  btk_widget_show (frame);

  darea =  btk_drawing_area_new ();
  /*btk_widget_set_double_buffered (darea, FALSE);*/
  btk_widget_add_events (darea, BDK_BUTTON_PRESS_MASK | BDK_BUTTON_RELEASE_MASK);
  btk_widget_set_size_request (darea, 500, 500);
  g_signal_connect (darea, "button_release_event", 
		    G_CALLBACK (darea_button_release_event), 
		    NULL);

  
  btk_container_add (BTK_CONTAINER (frame), darea);
  btk_widget_realize (darea);
  btk_widget_show (darea);
  btk_widget_modify_bg (darea, BTK_STATE_NORMAL,
			&black);
			
  
  vbox = btk_vbox_new (FALSE, 5);
  btk_box_pack_start (BTK_BOX (hbox),
		      vbox,
		      FALSE, FALSE,
		      5);
  btk_widget_show (vbox);

  window_store = btk_tree_store_new (1, BDK_TYPE_WINDOW);
  
  treeview = btk_tree_view_new_with_model (BTK_TREE_MODEL (window_store));
  btk_tree_selection_set_mode (btk_tree_view_get_selection (BTK_TREE_VIEW (treeview)),
			       BTK_SELECTION_MULTIPLE);
  column = btk_tree_view_column_new ();
  btk_tree_view_column_set_title (column, "Window");
  renderer = btk_cell_renderer_text_new ();
  btk_tree_view_column_pack_start (column, renderer, TRUE);
  btk_tree_view_column_set_cell_data_func (column,
					   renderer,
					   render_window_cell,
					   NULL, NULL);

  btk_tree_view_append_column (BTK_TREE_VIEW (treeview), column);


  scrolled = btk_scrolled_window_new (NULL, NULL);
  btk_widget_set_size_request (scrolled, 200, 400);
  btk_container_add (BTK_CONTAINER (scrolled), treeview);
  btk_box_pack_start (BTK_BOX (vbox),
		      scrolled,
		      FALSE, FALSE,
		      5);
  btk_widget_show (scrolled);
  btk_widget_show (treeview);
  
  table = btk_table_new (5, 4, TRUE);
  btk_box_pack_start (BTK_BOX (vbox),
		      table,
		      FALSE, FALSE,
		      2);
  btk_widget_show (table);

  button = btk_button_new ();
  btk_button_set_image (BTK_BUTTON (button),
			btk_image_new_from_stock (BTK_STOCK_GO_BACK,
						  BTK_ICON_SIZE_BUTTON));
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (move_window_clicked), 
		    GINT_TO_POINTER (BTK_DIR_LEFT));
  btk_table_attach_defaults (BTK_TABLE (table),
			     button,
			     0, 1,
			     1, 2);
  btk_widget_show (button);

  button = btk_button_new ();
  btk_button_set_image (BTK_BUTTON (button),
			btk_image_new_from_stock (BTK_STOCK_GO_UP,
						  BTK_ICON_SIZE_BUTTON));
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (move_window_clicked), 
		    GINT_TO_POINTER (BTK_DIR_UP));
  btk_table_attach_defaults (BTK_TABLE (table),
			     button,
			     1, 2,
			     0, 1);
  btk_widget_show (button);

  button = btk_button_new ();
  btk_button_set_image (BTK_BUTTON (button),
			btk_image_new_from_stock (BTK_STOCK_GO_FORWARD,
						  BTK_ICON_SIZE_BUTTON));
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (move_window_clicked), 
		    GINT_TO_POINTER (BTK_DIR_RIGHT));
  btk_table_attach_defaults (BTK_TABLE (table),
			     button,
			     2, 3,
			     1, 2);
  btk_widget_show (button);

  button = btk_button_new ();
  btk_button_set_image (BTK_BUTTON (button),
			btk_image_new_from_stock (BTK_STOCK_GO_DOWN,
						  BTK_ICON_SIZE_BUTTON));
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (move_window_clicked), 
		    GINT_TO_POINTER (BTK_DIR_DOWN));
  btk_table_attach_defaults (BTK_TABLE (table),
			     button,
			     1, 2,
			     2, 3);
  btk_widget_show (button);


  button = btk_button_new_with_label ("Raise");
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (raise_window_clicked), 
		    NULL);
  btk_table_attach_defaults (BTK_TABLE (table),
			     button,
			     0, 1,
			     0, 1);
  btk_widget_show (button);

  button = btk_button_new_with_label ("Lower");
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (lower_window_clicked), 
		    NULL);
  btk_table_attach_defaults (BTK_TABLE (table),
			     button,
			     0, 1,
			     2, 3);
  btk_widget_show (button);


  button = btk_button_new_with_label ("Smaller");
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (smaller_window_clicked), 
		    NULL);
  btk_table_attach_defaults (BTK_TABLE (table),
			     button,
			     2, 3,
			     0, 1);
  btk_widget_show (button);

  button = btk_button_new_with_label ("Larger");
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (larger_window_clicked), 
		    NULL);
  btk_table_attach_defaults (BTK_TABLE (table),
			     button,
			     2, 3,
			     2, 3);
  btk_widget_show (button);

  button = btk_button_new_with_label ("Native");
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (native_window_clicked), 
		    NULL);
  btk_table_attach_defaults (BTK_TABLE (table),
			     button,
			     1, 2,
			     1, 2);
  btk_widget_show (button);


  button = btk_button_new_with_label ("scroll");
  btk_button_set_image (BTK_BUTTON (button),
			btk_image_new_from_stock (BTK_STOCK_GO_UP,
						  BTK_ICON_SIZE_BUTTON));
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (scroll_window_clicked), 
		    GINT_TO_POINTER (BTK_DIR_UP));
  btk_table_attach_defaults (BTK_TABLE (table),
			     button,
			     3, 4,
			     0, 1);
  btk_widget_show (button);

  button = btk_button_new_with_label ("scroll");
  btk_button_set_image (BTK_BUTTON (button),
			btk_image_new_from_stock (BTK_STOCK_GO_DOWN,
						  BTK_ICON_SIZE_BUTTON));
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (scroll_window_clicked), 
		    GINT_TO_POINTER (BTK_DIR_DOWN));
  btk_table_attach_defaults (BTK_TABLE (table),
			     button,
			     3, 4,
			     1, 2);
  btk_widget_show (button);

  button = btk_button_new_with_label ("Manual");
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (manual_clicked),
		    NULL);
  btk_table_attach_defaults (BTK_TABLE (table),
			     button,
			     3, 4,
			     2, 3);
  btk_widget_show (button);

  button = btk_button_new_with_label ("Restack above");
  g_signal_connect (button, "clicked",
		    G_CALLBACK (restack_clicked),
		    GINT_TO_POINTER (1));
  btk_table_attach_defaults (BTK_TABLE (table),
			     button,
			     2, 3,
			     3, 4);
  btk_widget_show (button);

  button = btk_button_new_with_label ("Restack below");
  g_signal_connect (button, "clicked",
		    G_CALLBACK (restack_clicked),
		    0);
  btk_table_attach_defaults (BTK_TABLE (table),
			     button,
			     3, 4,
			     3, 4);
  btk_widget_show (button);

  button = btk_button_new_with_label ("draw drawable");
  btk_box_pack_start (BTK_BOX (vbox),
		      button,
		      FALSE, FALSE,
		      2);
  btk_widget_show (button);
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (draw_drawable_clicked), 
		    NULL);

  button = btk_button_new_with_label ("Add window");
  btk_box_pack_start (BTK_BOX (vbox),
		      button,
		      FALSE, FALSE,
		      2);
  btk_widget_show (button);
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (add_window_clicked), 
		    NULL);
  
  button = btk_button_new_with_label ("Remove window");
  btk_box_pack_start (BTK_BOX (vbox),
		      button,
		      FALSE, FALSE,
		      2);
  btk_widget_show (button);
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (remove_window_clicked), 
		    NULL);

  button = btk_button_new_with_label ("Save");
  btk_box_pack_start (BTK_BOX (vbox),
		      button,
		      FALSE, FALSE,
		      2);
  btk_widget_show (button);
  g_signal_connect (button, "clicked", 
		    G_CALLBACK (save_clicked), 
		    NULL);

  btk_widget_show (window);

  if (argc == 2)
    {
      file = g_file_new_for_commandline_arg (argv[1]);
      load_file (file);
      g_object_unref (file);
    }
  
  btk_main ();

  return 0;
}
