/* This file contains utility functions to create what would be a typical "main
 * window" for an application.
 *
 * TODO:
 *
 * Measurements happen from the start of the destruction of the last window.  Use
 * GTimer rather than X timestamps to fix this (it uses gettimeofday() internally!)
 *
 * Make non-interactive as well by using the above.
 *
 */

#include <string.h>
#include <btk/btk.h>

#include "widgets.h"

static void
quit_cb (BtkWidget *widget, gpointer data)
{
  btk_main_quit ();
}

static void
noop_cb (BtkWidget *widget, gpointer data)
{
  /* nothing */
}

static const BtkActionEntry menu_action_entries[] = {
  { "FileMenu", NULL, "_File" },
  { "EditMenu", NULL, "_Edit" },
  { "ViewMenu", NULL, "_View" },
  { "HelpMenu", NULL, "_Help" },

  { "New", BTK_STOCK_NEW, "_New", "<control>N", "Create a new document", G_CALLBACK (noop_cb) },
  { "Open", BTK_STOCK_OPEN, "_Open", "<control>O", "Open a file", G_CALLBACK (noop_cb) },
  { "Save", BTK_STOCK_SAVE, "_Save", "<control>S", "Save the document", G_CALLBACK (noop_cb) },
  { "SaveAs", BTK_STOCK_SAVE_AS, "Save _As...", NULL, "Save the document with a different name", NULL},
  { "PrintPreview", BTK_STOCK_PRINT_PREVIEW, "Print Previe_w", NULL, "See how the document will be printed", G_CALLBACK (noop_cb) },
  { "Print", BTK_STOCK_PRINT, "_Print", "<control>P", "Print the document", G_CALLBACK (noop_cb) },
  { "Close", BTK_STOCK_CLOSE, "_Close", "<control>W", "Close the document", G_CALLBACK (noop_cb) },
  { "Quit", BTK_STOCK_QUIT, "_Quit", "<control>Q", "Quit the program", G_CALLBACK (quit_cb) },

  { "Undo", BTK_STOCK_UNDO, "_Undo", "<control>Z", "Undo the last action", G_CALLBACK (noop_cb) },
  { "Redo", BTK_STOCK_REDO, "_Redo", "<control>Y", "Redo the last action", G_CALLBACK (noop_cb) },
  { "Cut", BTK_STOCK_CUT, "Cu_t", "<control>X", "Cut the selection to the clipboard", G_CALLBACK (noop_cb) },
  { "Copy", BTK_STOCK_COPY, "_Copy", "<control>C", "Copy the selection to the clipboard", G_CALLBACK (noop_cb) },
  { "Paste", BTK_STOCK_PASTE, "_Paste", "<control>V", "Paste the contents of the clipboard", G_CALLBACK (noop_cb) },
  { "Delete", BTK_STOCK_DELETE, "_Delete", "Delete", "Delete the selection", G_CALLBACK (noop_cb) },
  { "SelectAll", NULL, "Select _All", "<control>A", "Select the whole document", G_CALLBACK (noop_cb) },
  { "Preferences", BTK_STOCK_PREFERENCES, "Pr_eferences", NULL, "Configure the application", G_CALLBACK (noop_cb) },

  { "ZoomFit", BTK_STOCK_ZOOM_FIT, "Zoom to _Fit", NULL, "Zoom the document to fit the window", G_CALLBACK (noop_cb) },
  { "Zoom100", BTK_STOCK_ZOOM_100, "Zoom _1:1", NULL, "Zoom to 1:1 scale", G_CALLBACK (noop_cb) },
  { "ZoomIn", BTK_STOCK_ZOOM_IN, "Zoom _In", NULL, "Zoom into the document", G_CALLBACK (noop_cb) },
  { "ZoomOut", BTK_STOCK_ZOOM_OUT, "Zoom _Out", NULL, "Zoom away from the document", G_CALLBACK (noop_cb) },
  { "FullScreen", BTK_STOCK_FULLSCREEN, "Full _Screen", "F11", "Use the whole screen to view the document", G_CALLBACK (noop_cb) },

  { "HelpContents", BTK_STOCK_HELP, "_Contents", "F1", "Show the table of contents for the help system", G_CALLBACK (noop_cb) },
  { "About", BTK_STOCK_ABOUT, "_About", NULL, "About this application", G_CALLBACK (noop_cb) }
};

static const char ui_description[] =
"<ui>"
"  <menubar name=\"MainMenu\">"
"    <menu action=\"FileMenu\">"
"      <menuitem action=\"New\"/>"
"      <menuitem action=\"Open\"/>"
"      <menuitem action=\"Save\"/>"
"      <menuitem action=\"SaveAs\"/>"
"      <separator/>"
"      <menuitem action=\"PrintPreview\"/>"
"      <menuitem action=\"Print\"/>"
"      <separator/>"
"      <menuitem action=\"Close\"/>"
"      <menuitem action=\"Quit\"/>"
"    </menu>"
"    <menu action=\"EditMenu\">"
"      <menuitem action=\"Undo\"/>"
"      <menuitem action=\"Redo\"/>"
"      <separator/>"
"      <menuitem action=\"Cut\"/>"
"      <menuitem action=\"Copy\"/>"
"      <menuitem action=\"Paste\"/>"
"      <menuitem action=\"Delete\"/>"
"      <separator/>"
"      <menuitem action=\"SelectAll\"/>"
"      <separator/>"
"      <menuitem action=\"Preferences\"/>"
"    </menu>"
"    <menu action=\"ViewMenu\">"
"      <menuitem action=\"ZoomFit\"/>"
"      <menuitem action=\"Zoom100\"/>"
"      <menuitem action=\"ZoomIn\"/>"
"      <menuitem action=\"ZoomOut\"/>"
"      <separator/>"
"      <menuitem action=\"FullScreen\"/>"
"    </menu>"
"    <menu action=\"HelpMenu\">"
"      <menuitem action=\"HelpContents\"/>"
"      <menuitem action=\"About\"/>"
"    </menu>"
"  </menubar>"
"  <toolbar name=\"MainToolbar\">"
"    <toolitem action=\"New\"/>"
"    <toolitem action=\"Open\"/>"
"    <toolitem action=\"Save\"/>"
"    <separator/>"
"    <toolitem action=\"Print\"/>"
"    <separator/>"
"    <toolitem action=\"Undo\"/>"
"    <toolitem action=\"Redo\"/>"
"    <separator/>"
"    <toolitem action=\"Cut\"/>"
"    <toolitem action=\"Copy\"/>"
"    <toolitem action=\"Paste\"/>"
"  </toolbar>"
"</ui>";

static BtkUIManager *
uimanager_new (void)
{
  BtkUIManager *ui;
  BtkActionGroup *action_group;
  GError *error;

  ui = btk_ui_manager_new ();

  action_group = btk_action_group_new ("Actions");
  btk_action_group_add_actions (action_group, menu_action_entries, G_N_ELEMENTS (menu_action_entries), NULL);

  btk_ui_manager_insert_action_group (ui, action_group, 0);
  g_object_unref (action_group);

  error = NULL;
  if (!btk_ui_manager_add_ui_from_string (ui, ui_description, -1, &error))
    g_error ("Could not parse the uimanager XML: %s", error->message);

  return ui;
}

static BtkWidget *
menubar_new (BtkUIManager *ui)
{
  return btk_ui_manager_get_widget (ui, "/MainMenu");
}

static BtkWidget *
toolbar_new (BtkUIManager *ui)
{
  return btk_ui_manager_get_widget (ui, "/MainToolbar");
}

static BtkWidget *
drawing_area_new (void)
{
  BtkWidget *darea;

  darea = btk_drawing_area_new ();
  btk_widget_set_size_request (darea, 640, 480);
  return darea;
}

static BtkWidget *
content_area_new (void)
{
  BtkWidget *notebook;

  notebook = btk_notebook_new ();

  btk_notebook_append_page (BTK_NOTEBOOK (notebook),
			    drawing_area_new (),
			    btk_label_new ("First"));

  btk_notebook_append_page (BTK_NOTEBOOK (notebook),
			    drawing_area_new (),
			    btk_label_new ("Second"));

  btk_notebook_append_page (BTK_NOTEBOOK (notebook),
			    drawing_area_new (),
			    btk_label_new ("Third"));

  return notebook;
}

static BtkWidget *
status_bar_new (void)
{
  return btk_statusbar_new ();
}

static gboolean
delete_event_cb (BtkWidget *widget, BdkEvent *event, gpointer data)
{
  btk_main_quit ();
  return FALSE;
}

BtkWidget *
appwindow_new (void)
{
  BtkWidget *window;
  BtkUIManager *ui;
  BtkWidget *vbox;
  BtkWidget *widget;

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_title (BTK_WINDOW (window), "Main window");
  g_signal_connect (window, "delete-event",
		    G_CALLBACK (delete_event_cb), NULL);

  ui = uimanager_new ();
  g_signal_connect_swapped (window, "destroy",
			    G_CALLBACK (g_object_unref), ui);

  vbox = btk_vbox_new (FALSE, 0);
  btk_container_add (BTK_CONTAINER (window), vbox);

  widget = menubar_new (ui);
  btk_box_pack_start (BTK_BOX (vbox), widget, FALSE, FALSE, 0);

  widget = toolbar_new (ui);
  btk_box_pack_start (BTK_BOX (vbox), widget, FALSE, FALSE, 0);

  widget = content_area_new ();
  btk_box_pack_start (BTK_BOX (vbox), widget, TRUE, TRUE, 0);

  widget = status_bar_new ();
  btk_box_pack_end (BTK_BOX (vbox), widget, FALSE, FALSE, 0);

  return window;
}
