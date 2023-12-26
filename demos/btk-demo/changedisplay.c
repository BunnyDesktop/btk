/* Change Display
 *
 * Demonstrates migrating a window between different displays and
 * screens. A display is a mouse and keyboard with some number of
 * associated monitors. A screen is a set of monitors grouped
 * into a single physical work area. The neat thing about having
 * multiple displays is that they can be on a completely separate
 * computers, as long as there is a network connection to the
 * computer where the application is running.
 *
 * Only some of the windowing systems where BTK+ runs have the
 * concept of multiple displays and screens. (The X Window System
 * is the main example.) Other windowing systems can only
 * handle one keyboard and mouse, and combine all monitors into
 * a single screen.
 *
 * This is a moderately complex example, and demonstrates:
 *
 *  - Tracking the currently open displays and screens
 *
 *  - Changing the screen for a window
 *
 *  - Letting the user choose a window by clicking on it
 *
 *  - Using BtkListStore and BtkTreeView
 *
 *  - Using BtkDialog
 */
#include <string.h>
#include <btk/btk.h>
#include "demo-common.h"

/* The ChangeDisplayInfo structure corresponds to a toplevel window and
 * holds pointers to widgets inside the toplevel window along with other
 * information about the contents of the window.
 * This is a common organizational structure in real applications.
 */
typedef struct _ChangeDisplayInfo ChangeDisplayInfo;

struct _ChangeDisplayInfo
{
  BtkWidget *window;
  BtkSizeGroup *size_group;

  BtkTreeModel *display_model;
  BtkTreeModel *screen_model;
  BtkTreeSelection *screen_selection;

  BdkDisplay *current_display;
  BdkScreen *current_screen;
};

/* These enumerations provide symbolic names for the columns
 * in the two BtkListStore models.
 */
enum
{
  DISPLAY_COLUMN_NAME,
  DISPLAY_COLUMN_DISPLAY,
  DISPLAY_NUM_COLUMNS
};

enum
{
  SCREEN_COLUMN_NUMBER,
  SCREEN_COLUMN_SCREEN,
  SCREEN_NUM_COLUMNS
};

/* Finds the toplevel window under the mouse pointer, if any.
 */
static BtkWidget *
find_toplevel_at_pointer (BdkDisplay *display)
{
  BdkWindow *pointer_window;
  BtkWidget *widget = NULL;

  pointer_window = bdk_display_get_window_at_pointer (display, NULL, NULL);

  /* The user data field of a BdkWindow is used to store a pointer
   * to the widget that created it.
   */
  if (pointer_window)
    {
      bpointer widget_ptr;
      bdk_window_get_user_data (pointer_window, &widget_ptr);
      widget = widget_ptr;
    }

  return widget ? btk_widget_get_toplevel (widget) : NULL;
}

static bboolean
button_release_event_cb (BtkWidget       *widget,
			 BdkEventButton  *event,
			 bboolean        *clicked)
{
  *clicked = TRUE;
  return TRUE;
}

/* Asks the user to click on a window, then waits for them click
 * the mouse. When the mouse is released, returns the toplevel
 * window under the pointer, or NULL, if there is none.
 */
static BtkWidget *
query_for_toplevel (BdkScreen  *screen,
		    const char *prompt)
{
  BdkDisplay *display = bdk_screen_get_display (screen);
  BtkWidget *popup, *label, *frame;
  BdkCursor *cursor;
  BtkWidget *toplevel = NULL;

  popup = btk_window_new (BTK_WINDOW_POPUP);
  btk_window_set_screen (BTK_WINDOW (popup), screen);
  btk_window_set_modal (BTK_WINDOW (popup), TRUE);
  btk_window_set_position (BTK_WINDOW (popup), BTK_WIN_POS_CENTER);

  frame = btk_frame_new (NULL);
  btk_frame_set_shadow_type (BTK_FRAME (frame), BTK_SHADOW_OUT);
  btk_container_add (BTK_CONTAINER (popup), frame);

  label = btk_label_new (prompt);
  btk_misc_set_padding (BTK_MISC (label), 10, 10);
  btk_container_add (BTK_CONTAINER (frame), label);

  btk_widget_show_all (popup);
  cursor = bdk_cursor_new_for_display (display, BDK_CROSSHAIR);

  if (bdk_pointer_grab (btk_widget_get_window (popup), FALSE,
			BDK_BUTTON_RELEASE_MASK,
			NULL,
			cursor,
			BDK_CURRENT_TIME) == BDK_GRAB_SUCCESS)
    {
      bboolean clicked = FALSE;

      g_signal_connect (popup, "button-release-event",
			G_CALLBACK (button_release_event_cb), &clicked);

      /* Process events until clicked is set by button_release_event_cb.
       * We pass in may_block=TRUE since we want to wait if there
       * are no events currently.
       */
      while (!clicked)
	g_main_context_iteration (NULL, TRUE);

      toplevel = find_toplevel_at_pointer (bdk_screen_get_display (screen));
      if (toplevel == popup)
	toplevel = NULL;
    }

  bdk_cursor_unref (cursor);
  btk_widget_destroy (popup);
  bdk_flush ();			/* Really release the grab */

  return toplevel;
}

/* Prompts the user for a toplevel window to move, and then moves
 * that window to the currently selected display
 */
static void
query_change_display (ChangeDisplayInfo *info)
{
  BdkScreen *screen = btk_widget_get_screen (info->window);
  BtkWidget *toplevel;

  toplevel = query_for_toplevel (screen,
				 "Please select the toplevel\n"
				 "to move to the new screen");

  if (toplevel)
    btk_window_set_screen (BTK_WINDOW (toplevel), info->current_screen);
  else
    bdk_display_beep (bdk_screen_get_display (screen));
}

/* Fills in the screen list based on the current display
 */
static void
fill_screens (ChangeDisplayInfo *info)
{
  btk_list_store_clear (BTK_LIST_STORE (info->screen_model));

  if (info->current_display)
    {
      bint n_screens = bdk_display_get_n_screens (info->current_display);
      bint i;

      for (i = 0; i < n_screens; i++)
	{
	  BdkScreen *screen = bdk_display_get_screen (info->current_display, i);
	  BtkTreeIter iter;

	  btk_list_store_append (BTK_LIST_STORE (info->screen_model), &iter);
	  btk_list_store_set (BTK_LIST_STORE (info->screen_model), &iter,
			      SCREEN_COLUMN_NUMBER, i,
			      SCREEN_COLUMN_SCREEN, screen,
			      -1);

	  if (i == 0)
	    btk_tree_selection_select_iter (info->screen_selection, &iter);
	}
    }
}

/* Called when the user clicks on a button in our dialog or
 * closes the dialog through the window manager. Unless the
 * "Change" button was clicked, we destroy the dialog.
 */
static void
response_cb (BtkDialog         *dialog,
	     bint               response_id,
	     ChangeDisplayInfo *info)
{
  if (response_id == BTK_RESPONSE_OK)
    query_change_display (info);
  else
    btk_widget_destroy (BTK_WIDGET (dialog));
}

/* Called when the user clicks on "Open..." in the display
 * frame. Prompts for a new display, and then opens a connection
 * to that display.
 */
static void
open_display_cb (BtkWidget         *button,
		 ChangeDisplayInfo *info)
{
  BtkWidget *dialog;
  BtkWidget *display_entry;
  BtkWidget *dialog_label;
  BtkWidget *content_area;
  bchar *new_screen_name = NULL;
  BdkDisplay *result = NULL;

  dialog = btk_dialog_new_with_buttons ("Open Display",
					BTK_WINDOW (info->window),
					BTK_DIALOG_MODAL,
					BTK_STOCK_CANCEL, BTK_RESPONSE_CANCEL,
					BTK_STOCK_OK, BTK_RESPONSE_OK,
					NULL);

  btk_dialog_set_default_response (BTK_DIALOG (dialog), BTK_RESPONSE_OK);
  display_entry = btk_entry_new ();
  btk_entry_set_activates_default (BTK_ENTRY (display_entry), TRUE);
  dialog_label =
    btk_label_new ("Please enter the name of\nthe new display\n");
  
  content_area = btk_dialog_get_content_area (BTK_DIALOG (dialog));
  
  btk_container_add (BTK_CONTAINER (content_area), dialog_label);
  btk_container_add (BTK_CONTAINER (content_area), display_entry);

  btk_widget_grab_focus (display_entry);
  btk_widget_show_all (btk_bin_get_child (BTK_BIN (dialog)));

  while (!result)
    {
      bint response_id = btk_dialog_run (BTK_DIALOG (dialog));
      if (response_id != BTK_RESPONSE_OK)
	break;

      new_screen_name = btk_editable_get_chars (BTK_EDITABLE (display_entry),
						0, -1);

      if (strcmp (new_screen_name, "") != 0)
	{
	  result = bdk_display_open (new_screen_name);
	  if (!result)
	    {
	      bchar *error_msg =
		g_strdup_printf  ("Can't open display :\n\t%s\nplease try another one\n",
				  new_screen_name);
	      btk_label_set_text (BTK_LABEL (dialog_label), error_msg);
	      g_free (error_msg);
	    }

	  g_free (new_screen_name);
	}
    }

  btk_widget_destroy (dialog);
}

/* Called when the user clicks on the "Close" button in the
 * "Display" frame. Closes the selected display.
 */
static void
close_display_cb (BtkWidget         *button,
		  ChangeDisplayInfo *info)
{
  if (info->current_display)
    bdk_display_close (info->current_display);
}

/* Called when the selected row in the display list changes.
 * Updates info->current_display, then refills the list of
 * screens.
 */
static void
display_changed_cb (BtkTreeSelection  *selection,
		    ChangeDisplayInfo *info)
{
  BtkTreeModel *model;
  BtkTreeIter iter;

  if (info->current_display)
    g_object_unref (info->current_display);
  if (btk_tree_selection_get_selected (selection, &model, &iter))
    btk_tree_model_get (model, &iter,
			DISPLAY_COLUMN_DISPLAY, &info->current_display,
			-1);
  else
    info->current_display = NULL;

  fill_screens (info);
}

/* Called when the selected row in the sceen list changes.
 * Updates info->current_screen.
 */
static void
screen_changed_cb (BtkTreeSelection  *selection,
		   ChangeDisplayInfo *info)
{
  BtkTreeModel *model;
  BtkTreeIter iter;

  if (info->current_screen)
    g_object_unref (info->current_screen);
  if (btk_tree_selection_get_selected (selection, &model, &iter))
    btk_tree_model_get (model, &iter,
			SCREEN_COLUMN_SCREEN, &info->current_screen,
			-1);
  else
    info->current_screen = NULL;
}

/* This function is used both for creating the "Display" and
 * "Screen" frames, since they have a similar structure. The
 * caller hooks up the right context for the value returned
 * in tree_view, and packs any relevant buttons into button_vbox.
 */
static void
create_frame (ChangeDisplayInfo *info,
	      const char        *title,
	      BtkWidget        **frame,
	      BtkWidget        **tree_view,
	      BtkWidget        **button_vbox)
{
  BtkTreeSelection *selection;
  BtkWidget *scrollwin;
  BtkWidget *hbox;

  *frame = btk_frame_new (title);

  hbox = btk_hbox_new (FALSE, 8);
  btk_container_set_border_width (BTK_CONTAINER (hbox), 8);
  btk_container_add (BTK_CONTAINER (*frame), hbox);

  scrollwin = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrollwin),
				  BTK_POLICY_NEVER, BTK_POLICY_AUTOMATIC);
  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (scrollwin),
				       BTK_SHADOW_IN);
  btk_box_pack_start (BTK_BOX (hbox), scrollwin, TRUE, TRUE, 0);

  *tree_view = btk_tree_view_new ();
  btk_tree_view_set_headers_visible (BTK_TREE_VIEW (*tree_view), FALSE);
  btk_container_add (BTK_CONTAINER (scrollwin), *tree_view);

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (*tree_view));
  btk_tree_selection_set_mode (selection, BTK_SELECTION_BROWSE);

  *button_vbox = btk_vbox_new (FALSE, 5);
  btk_box_pack_start (BTK_BOX (hbox), *button_vbox, FALSE, FALSE, 0);

  if (!info->size_group)
    info->size_group = btk_size_group_new (BTK_SIZE_GROUP_HORIZONTAL);

  btk_size_group_add_widget (BTK_SIZE_GROUP (info->size_group), *button_vbox);
}

/* If we have a stack of buttons, it often looks better if their contents
 * are left-aligned, rather than centered. This function creates a button
 * and left-aligns it contents.
 */
BtkWidget *
left_align_button_new (const char *label)
{
  BtkWidget *button = btk_button_new_with_mnemonic (label);
  BtkWidget *child = btk_bin_get_child (BTK_BIN (button));

  btk_misc_set_alignment (BTK_MISC (child), 0., 0.5);

  return button;
}

/* Creates the "Display" frame in the main window.
 */
BtkWidget *
create_display_frame (ChangeDisplayInfo *info)
{
  BtkWidget *frame;
  BtkWidget *tree_view;
  BtkWidget *button_vbox;
  BtkTreeViewColumn *column;
  BtkTreeSelection *selection;
  BtkWidget *button;

  create_frame (info, "Display", &frame, &tree_view, &button_vbox);

  button = left_align_button_new ("_Open...");
  g_signal_connect (button, "clicked",  G_CALLBACK (open_display_cb), info);
  btk_box_pack_start (BTK_BOX (button_vbox), button, FALSE, FALSE, 0);

  button = left_align_button_new ("_Close");
  g_signal_connect (button, "clicked",  G_CALLBACK (close_display_cb), info);
  btk_box_pack_start (BTK_BOX (button_vbox), button, FALSE, FALSE, 0);

  info->display_model = (BtkTreeModel *)btk_list_store_new (DISPLAY_NUM_COLUMNS,
							    B_TYPE_STRING,
							    BDK_TYPE_DISPLAY);

  btk_tree_view_set_model (BTK_TREE_VIEW (tree_view), info->display_model);

  column = btk_tree_view_column_new_with_attributes ("Name",
						     btk_cell_renderer_text_new (),
						     "text", DISPLAY_COLUMN_NAME,
						     NULL);
  btk_tree_view_append_column (BTK_TREE_VIEW (tree_view), column);

  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (tree_view));
  g_signal_connect (selection, "changed",
		    G_CALLBACK (display_changed_cb), info);

  return frame;
}

/* Creates the "Screen" frame in the main window.
 */
BtkWidget *
create_screen_frame (ChangeDisplayInfo *info)
{
  BtkWidget *frame;
  BtkWidget *tree_view;
  BtkWidget *button_vbox;
  BtkTreeViewColumn *column;

  create_frame (info, "Screen", &frame, &tree_view, &button_vbox);

  info->screen_model = (BtkTreeModel *)btk_list_store_new (SCREEN_NUM_COLUMNS,
							   B_TYPE_INT,
							   BDK_TYPE_SCREEN);

  btk_tree_view_set_model (BTK_TREE_VIEW (tree_view), info->screen_model);

  column = btk_tree_view_column_new_with_attributes ("Number",
						     btk_cell_renderer_text_new (),
						     "text", SCREEN_COLUMN_NUMBER,
						     NULL);
  btk_tree_view_append_column (BTK_TREE_VIEW (tree_view), column);

  info->screen_selection = btk_tree_view_get_selection (BTK_TREE_VIEW (tree_view));
  g_signal_connect (info->screen_selection, "changed",
		    G_CALLBACK (screen_changed_cb), info);

  return frame;
}

/* Called when one of the currently open displays is closed.
 * Remove it from our list of displays.
 */
static void
display_closed_cb (BdkDisplay        *display,
		   bboolean           is_error,
		   ChangeDisplayInfo *info)
{
  BtkTreeIter iter;
  bboolean valid;

  for (valid = btk_tree_model_get_iter_first (info->display_model, &iter);
       valid;
       valid = btk_tree_model_iter_next (info->display_model, &iter))
    {
      BdkDisplay *tmp_display;

      btk_tree_model_get (info->display_model, &iter,
			  DISPLAY_COLUMN_DISPLAY, &tmp_display,
			  -1);
      if (tmp_display == display)
	{
	  btk_list_store_remove (BTK_LIST_STORE (info->display_model), &iter);
	  break;
	}
    }
}

/* Adds a new display to our list of displays, and connects
 * to the "closed" signal so that we can remove it from the
 * list of displays again.
 */
static void
add_display (ChangeDisplayInfo *info,
	     BdkDisplay        *display)
{
  const bchar *name = bdk_display_get_name (display);
  BtkTreeIter iter;

  btk_list_store_append (BTK_LIST_STORE (info->display_model), &iter);
  btk_list_store_set (BTK_LIST_STORE (info->display_model), &iter,
		      DISPLAY_COLUMN_NAME, name,
		      DISPLAY_COLUMN_DISPLAY, display,
		      -1);

  g_signal_connect (display, "closed",
		    G_CALLBACK (display_closed_cb), info);
}

/* Called when a new display is opened
 */
static void
display_opened_cb (BdkDisplayManager *manager,
		   BdkDisplay        *display,
		   ChangeDisplayInfo *info)
{
  add_display (info, display);
}

/* Adds all currently open displays to our list of displays,
 * and set up a signal connection so that we'll be notified
 * when displays are opened in the future as well.
 */
static void
initialize_displays (ChangeDisplayInfo *info)
{
  BdkDisplayManager *manager = bdk_display_manager_get ();
  GSList *displays = bdk_display_manager_list_displays (manager);
  GSList *tmp_list;

  for (tmp_list = displays; tmp_list; tmp_list = tmp_list->next)
    add_display (info, tmp_list->data);

  b_slist_free (tmp_list);

  g_signal_connect (manager, "display-opened",
		    G_CALLBACK (display_opened_cb), info);
}

/* Cleans up when the toplevel is destroyed; we remove the
 * connections we use to track currently open displays, then
 * free the ChangeDisplayInfo structure.
 */
static void
destroy_info (ChangeDisplayInfo *info)
{
  BdkDisplayManager *manager = bdk_display_manager_get ();
  GSList *displays = bdk_display_manager_list_displays (manager);
  GSList *tmp_list;

  g_signal_handlers_disconnect_by_func (manager,
					display_opened_cb,
					info);

  for (tmp_list = displays; tmp_list; tmp_list = tmp_list->next)
    g_signal_handlers_disconnect_by_func (tmp_list->data,
					  display_closed_cb,
					  info);

  b_slist_free (tmp_list);

  g_object_unref (info->size_group);
  g_object_unref (info->display_model);
  g_object_unref (info->screen_model);

  if (info->current_display)
    g_object_unref (info->current_display);
  if (info->current_screen)
    g_object_unref (info->current_screen);

  g_free (info);
}

static void
destroy_cb (BtkObject          *object,
	    ChangeDisplayInfo **info)
{
  destroy_info (*info);
  *info = NULL;
}

/* Main entry point. If the dialog for this demo doesn't yet exist, creates
 * it. Otherwise, destroys it.
 */
BtkWidget *
do_changedisplay (BtkWidget *do_widget)
{
  static ChangeDisplayInfo *info = NULL;

  if (!info)
    {
      BtkWidget *vbox;
      BtkWidget *frame;

      info = g_new0 (ChangeDisplayInfo, 1);

      info->window = btk_dialog_new_with_buttons ("Change Screen or display",
					    BTK_WINDOW (do_widget),
					    BTK_DIALOG_NO_SEPARATOR,
					    BTK_STOCK_CLOSE,  BTK_RESPONSE_CLOSE,
					    "Change",         BTK_RESPONSE_OK,
					    NULL);

      btk_window_set_default_size (BTK_WINDOW (info->window), 300, 400);

      g_signal_connect (info->window, "response",
			G_CALLBACK (response_cb), info);
      g_signal_connect (info->window, "destroy",
			G_CALLBACK (destroy_cb), &info);

      vbox = btk_vbox_new (FALSE, 5);
      btk_container_set_border_width (BTK_CONTAINER (vbox), 8);

      btk_box_pack_start (BTK_BOX (btk_dialog_get_content_area (BTK_DIALOG (info->window))), vbox,
			  TRUE, TRUE, 0);

      frame = create_display_frame (info);
      btk_box_pack_start (BTK_BOX (vbox), frame, TRUE, TRUE, 0);

      frame = create_screen_frame (info);
      btk_box_pack_start (BTK_BOX (vbox), frame, TRUE, TRUE, 0);

      initialize_displays (info);

      btk_widget_show_all (info->window);
      return info->window;
    }
  else
    {
      btk_widget_destroy (info->window);
      return NULL;
    }
}
