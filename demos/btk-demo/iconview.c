/* Icon View/Icon View Basics
 *
 * The BtkIconView widget is used to display and manipulate icons.
 * It uses a BtkTreeModel for data storage, so the list store
 * example might be helpful.
 */

#include <btk/btk.h>
#include <string.h>
#include "demo-common.h"

static BtkWidget *window = NULL;

#define FOLDER_NAME "bunny-fs-directory.png"
#define FILE_NAME "bunny-fs-regular.png"

enum
{
  COL_PATH,
  COL_DISPLAY_NAME,
  COL_PIXBUF,
  COL_IS_DIRECTORY,
  NUM_COLS
};


static BdkPixbuf *file_pixbuf, *folder_pixbuf;
gchar *parent;
BtkToolItem *up_button;

/* Loads the images for the demo and returns whether the operation succeeded */
static gboolean
load_pixbufs (GError **error)
{
  char *filename;

  if (file_pixbuf)
    return TRUE; /* already loaded earlier */

  /* demo_find_file() looks in the current directory first,
   * so you can run btk-demo without installing BTK, then looks
   * in the location where the file is installed.
   */
  filename = demo_find_file (FILE_NAME, error);
  if (!filename)
    return FALSE; /* note that "error" was filled in and returned */

  file_pixbuf = bdk_pixbuf_new_from_file (filename, error);
  g_free (filename);

  if (!file_pixbuf)
    return FALSE; /* Note that "error" was filled with a GError */

  filename = demo_find_file (FOLDER_NAME, error);
  if (!filename)
    return FALSE; /* note that "error" was filled in and returned */

  folder_pixbuf = bdk_pixbuf_new_from_file (filename, error);
  g_free (filename);

  return TRUE;
}

static void
fill_store (BtkListStore *store)
{
  GDir *dir;
  const gchar *name;
  BtkTreeIter iter;

  /* First clear the store */
  btk_list_store_clear (store);

  /* Now go through the directory and extract all the file
   * information */
  dir = g_dir_open (parent, 0, NULL);
  if (!dir)
    return;

  name = g_dir_read_name (dir);
  while (name != NULL)
    {
      gchar *path, *display_name;
      gboolean is_dir;

      /* We ignore hidden files that start with a '.' */
      if (name[0] != '.')
	{
	  path = g_build_filename (parent, name, NULL);

	  is_dir = g_file_test (path, G_FILE_TEST_IS_DIR);

	  display_name = g_filename_to_utf8 (name, -1, NULL, NULL, NULL);

	  btk_list_store_append (store, &iter);
	  btk_list_store_set (store, &iter,
			      COL_PATH, path,
			      COL_DISPLAY_NAME, display_name,
			      COL_IS_DIRECTORY, is_dir,
			      COL_PIXBUF, is_dir ? folder_pixbuf : file_pixbuf,
			      -1);
	  g_free (path);
	  g_free (display_name);
	}

      name = g_dir_read_name (dir);
    }
  g_dir_close (dir);
}

static gint
sort_func (BtkTreeModel *model,
	   BtkTreeIter  *a,
	   BtkTreeIter  *b,
	   gpointer      user_data)
{
  gboolean is_dir_a, is_dir_b;
  gchar *name_a, *name_b;
  int ret;

  /* We need this function because we want to sort
   * folders before files.
   */


  btk_tree_model_get (model, a,
		      COL_IS_DIRECTORY, &is_dir_a,
		      COL_DISPLAY_NAME, &name_a,
		      -1);

  btk_tree_model_get (model, b,
		      COL_IS_DIRECTORY, &is_dir_b,
		      COL_DISPLAY_NAME, &name_b,
		      -1);

  if (!is_dir_a && is_dir_b)
    ret = 1;
  else if (is_dir_a && !is_dir_b)
    ret = -1;
  else
    {
      ret = g_utf8_collate (name_a, name_b);
    }

  g_free (name_a);
  g_free (name_b);

  return ret;
}

static BtkListStore *
create_store (void)
{
  BtkListStore *store;

  store = btk_list_store_new (NUM_COLS,
			      B_TYPE_STRING,
			      B_TYPE_STRING,
			      BDK_TYPE_PIXBUF,
			      B_TYPE_BOOLEAN);

  /* Set sort column and function */
  btk_tree_sortable_set_default_sort_func (BTK_TREE_SORTABLE (store),
					   sort_func,
					   NULL, NULL);
  btk_tree_sortable_set_sort_column_id (BTK_TREE_SORTABLE (store),
					BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
					BTK_SORT_ASCENDING);

  return store;
}

static void
item_activated (BtkIconView *icon_view,
		BtkTreePath *tree_path,
		gpointer     user_data)
{
  BtkListStore *store;
  gchar *path;
  BtkTreeIter iter;
  gboolean is_dir;

  store = BTK_LIST_STORE (user_data);

  btk_tree_model_get_iter (BTK_TREE_MODEL (store),
			   &iter, tree_path);
  btk_tree_model_get (BTK_TREE_MODEL (store), &iter,
		      COL_PATH, &path,
		      COL_IS_DIRECTORY, &is_dir,
		      -1);

  if (!is_dir)
    {
      g_free (path);
      return;
    }

  /* Replace parent with path and re-fill the model*/
  g_free (parent);
  parent = path;

  fill_store (store);

  /* Sensitize the up button */
  btk_widget_set_sensitive (BTK_WIDGET (up_button), TRUE);
}

static void
up_clicked (BtkToolItem *item,
	    gpointer     user_data)
{
  BtkListStore *store;
  gchar *dir_name;

  store = BTK_LIST_STORE (user_data);

  dir_name = g_path_get_dirname (parent);
  g_free (parent);

  parent = dir_name;

  fill_store (store);

  /* Maybe de-sensitize the up button */
  btk_widget_set_sensitive (BTK_WIDGET (up_button),
			    strcmp (parent, "/") != 0);
}

static void
home_clicked (BtkToolItem *item,
	      gpointer     user_data)
{
  BtkListStore *store;

  store = BTK_LIST_STORE (user_data);

  g_free (parent);
  parent = g_strdup (g_get_home_dir ());

  fill_store (store);

  /* Sensitize the up button */
  btk_widget_set_sensitive (BTK_WIDGET (up_button),
			    TRUE);
}

static void close_window(void)
{
  btk_widget_destroy (window);
  window = NULL;

  g_object_unref (file_pixbuf);
  file_pixbuf = NULL;

  g_object_unref (folder_pixbuf);
  folder_pixbuf = NULL;
}

BtkWidget *
do_iconview (BtkWidget *do_widget)
{
  if (!window)
    {
      GError *error;

      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_default_size (BTK_WINDOW (window), 650, 400);

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (do_widget));
      btk_window_set_title (BTK_WINDOW (window), "BtkIconView demo");

      g_signal_connect (window, "destroy",
			G_CALLBACK (close_window), NULL);

      error = NULL;
      if (!load_pixbufs (&error))
	{
	  BtkWidget *dialog;

	  dialog = btk_message_dialog_new (BTK_WINDOW (window),
					   BTK_DIALOG_DESTROY_WITH_PARENT,
					   BTK_MESSAGE_ERROR,
					   BTK_BUTTONS_CLOSE,
					   "Failed to load an image: %s",
					   error->message);

	  g_error_free (error);

	  g_signal_connect (dialog, "response",
			    G_CALLBACK (btk_widget_destroy), NULL);

	  btk_widget_show (dialog);
	}
      else
	{
	  BtkWidget *sw;
	  BtkWidget *icon_view;
	  BtkListStore *store;
	  BtkWidget *vbox;
	  BtkWidget *tool_bar;
	  BtkToolItem *home_button;

	  vbox = btk_vbox_new (FALSE, 0);
	  btk_container_add (BTK_CONTAINER (window), vbox);

	  tool_bar = btk_toolbar_new ();
	  btk_box_pack_start (BTK_BOX (vbox), tool_bar, FALSE, FALSE, 0);

	  up_button = btk_tool_button_new_from_stock (BTK_STOCK_GO_UP);
	  btk_tool_item_set_is_important (up_button, TRUE);
	  btk_widget_set_sensitive (BTK_WIDGET (up_button), FALSE);
	  btk_toolbar_insert (BTK_TOOLBAR (tool_bar), up_button, -1);

	  home_button = btk_tool_button_new_from_stock (BTK_STOCK_HOME);
	  btk_tool_item_set_is_important (home_button, TRUE);
	  btk_toolbar_insert (BTK_TOOLBAR (tool_bar), home_button, -1);


	  sw = btk_scrolled_window_new (NULL, NULL);
	  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (sw),
					       BTK_SHADOW_ETCHED_IN);
	  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (sw),
					  BTK_POLICY_AUTOMATIC,
					  BTK_POLICY_AUTOMATIC);

	  btk_box_pack_start (BTK_BOX (vbox), sw, TRUE, TRUE, 0);

	  /* Create the store and fill it with the contents of '/' */
	  parent = g_strdup ("/");
	  store = create_store ();
	  fill_store (store);

	  icon_view = btk_icon_view_new_with_model (BTK_TREE_MODEL (store));
	  btk_icon_view_set_selection_mode (BTK_ICON_VIEW (icon_view),
					    BTK_SELECTION_MULTIPLE);
	  g_object_unref (store);

	  /* Connect to the "clicked" signal of the "Up" tool button */
	  g_signal_connect (up_button, "clicked",
			    G_CALLBACK (up_clicked), store);

	  /* Connect to the "clicked" signal of the "Home" tool button */
	  g_signal_connect (home_button, "clicked",
			    G_CALLBACK (home_clicked), store);

	  /* We now set which model columns that correspond to the text
	   * and pixbuf of each item
	   */
	  btk_icon_view_set_text_column (BTK_ICON_VIEW (icon_view), COL_DISPLAY_NAME);
	  btk_icon_view_set_pixbuf_column (BTK_ICON_VIEW (icon_view), COL_PIXBUF);

	  /* Connect to the "item-activated" signal */
	  g_signal_connect (icon_view, "item-activated",
			    G_CALLBACK (item_activated), store);
	  btk_container_add (BTK_CONTAINER (sw), icon_view);

	  btk_widget_grab_focus (icon_view);
	}
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

