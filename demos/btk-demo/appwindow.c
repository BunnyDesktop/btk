/* Application main window
 *
 * Demonstrates a typical application window with menubar, toolbar, statusbar.
 */

#include <btk/btk.h>
#include "config.h"
#include "demo-common.h"

static BtkWidget *window = NULL;
static BtkWidget *infobar = NULL;
static BtkWidget *messagelabel = NULL;

static void
activate_action (BtkAction *action)
{
  const gchar *name = btk_action_get_name (action);
  const gchar *typename = B_OBJECT_TYPE_NAME (action);

  BtkWidget *dialog;

  dialog = btk_message_dialog_new (BTK_WINDOW (window),
                                   BTK_DIALOG_DESTROY_WITH_PARENT,
                                   BTK_MESSAGE_INFO,
                                   BTK_BUTTONS_CLOSE,
                                   "You activated action: \"%s\" of type \"%s\"",
                                    name, typename);

  /* Close dialog on user response */
  g_signal_connect (dialog,
                    "response",
                    G_CALLBACK (btk_widget_destroy),
                    NULL);

  btk_widget_show (dialog);
}

static void
activate_radio_action (BtkAction *action, BtkRadioAction *current)
{
  const gchar *name = btk_action_get_name (BTK_ACTION (current));
  const gchar *typename = B_OBJECT_TYPE_NAME (BTK_ACTION (current));
  gboolean active = btk_toggle_action_get_active (BTK_TOGGLE_ACTION (current));
  gint value = btk_radio_action_get_current_value (BTK_RADIO_ACTION (current));

  if (active)
    {
      gchar *text;

      text = g_strdup_printf ("You activated radio action: \"%s\" of type \"%s\".\n"
                              "Current value: %d",
                              name, typename, value);
      btk_label_set_text (BTK_LABEL (messagelabel), text);
      btk_info_bar_set_message_type (BTK_INFO_BAR (infobar), (BtkMessageType)value);
      btk_widget_show (infobar);
      g_free (text);
    }
}

static void
about_cb (BtkAction *action,
	  BtkWidget *window)
{
  BdkPixbuf *pixbuf, *transparent;
  gchar *filename;

  const gchar *authors[] = {
    "Peter Mattis",
    "Spencer Kimball",
    "Josh MacDonald",
    "and many more...",
    NULL
  };

  const gchar *documentors[] = {
    "Owen Taylor",
    "Tony Gale",
    "Matthias Clasen <mclasen@redhat.com>",
    "and many more...",
    NULL
  };

  const gchar *license =
    "This library is free software; you can redistribute it and/or\n"
    "modify it under the terms of the GNU Library General Public License as\n"
    "published by the Free Software Foundation; either version 2 of the\n"
    "License, or (at your option) any later version.\n"
    "\n"
    "This library is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n"
    "Library General Public License for more details.\n"
    "\n"
    "You should have received a copy of the GNU Library General Public\n"
    "License along with the Bunny Library; see the file COPYING.LIB.  If not,\n"
    "write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,\n"
    "Boston, MA 02111-1307, USA.\n";

  pixbuf = NULL;
  transparent = NULL;
  filename = demo_find_file ("btk-logo-rgb.gif", NULL);
  if (filename)
    {
      pixbuf = bdk_pixbuf_new_from_file (filename, NULL);
      g_free (filename);
      transparent = bdk_pixbuf_add_alpha (pixbuf, TRUE, 0xff, 0xff, 0xff);
      g_object_unref (pixbuf);
    }

  btk_show_about_dialog (BTK_WINDOW (window),
			 "program-name", "BTK+ Code Demos",
			 "version", PACKAGE_VERSION,
			 "copyright", "(C) 1997-2009 The BTK+ Team",
			 "license", license,
			 "website", "http://www.btk.org",
			 "comments", "Program to demonstrate BTK+ functions.",
			 "authors", authors,
			 "documenters", documentors,
			 "logo", transparent,
                         "title", "About BTK+ Code Demos",
			 NULL);

  g_object_unref (transparent);
}

typedef struct
{
  BtkAction action;
} ToolMenuAction;

typedef struct
{
  BtkActionClass parent_class;
} ToolMenuActionClass;

G_DEFINE_TYPE(ToolMenuAction, tool_menu_action, BTK_TYPE_ACTION)

static void
tool_menu_action_class_init (ToolMenuActionClass *class)
{
  BTK_ACTION_CLASS (class)->toolbar_item_type = BTK_TYPE_MENU_TOOL_BUTTON;
}

static void
tool_menu_action_init (ToolMenuAction *action)
{
}

static BtkActionEntry entries[] = {
  { "FileMenu", NULL, "_File" },               /* name, stock id, label */
  { "OpenMenu", NULL, "_Open" },               /* name, stock id, label */
  { "PreferencesMenu", NULL, "_Preferences" }, /* name, stock id, label */
  { "ColorMenu", NULL, "_Color"  },            /* name, stock id, label */
  { "ShapeMenu", NULL, "_Shape" },             /* name, stock id, label */
  { "HelpMenu", NULL, "_Help" },               /* name, stock id, label */
  { "New", BTK_STOCK_NEW,                      /* name, stock id */
    "_New", "<control>N",                      /* label, accelerator */
    "Create a new file",                       /* tooltip */
    G_CALLBACK (activate_action) },
  { "File1", NULL,                             /* name, stock id */
    "File1", NULL,                             /* label, accelerator */
    "Open first file",                         /* tooltip */
    G_CALLBACK (activate_action) },
  { "Save", BTK_STOCK_SAVE,                    /* name, stock id */
    "_Save","<control>S",                      /* label, accelerator */
    "Save current file",                       /* tooltip */
    G_CALLBACK (activate_action) },
  { "SaveAs", BTK_STOCK_SAVE,                  /* name, stock id */
    "Save _As...", NULL,                       /* label, accelerator */
    "Save to a file",                          /* tooltip */
    G_CALLBACK (activate_action) },
  { "Quit", BTK_STOCK_QUIT,                    /* name, stock id */
    "_Quit", "<control>Q",                     /* label, accelerator */
    "Quit",                                    /* tooltip */
    G_CALLBACK (activate_action) },
  { "About", NULL,                             /* name, stock id */
    "_About", "<control>A",                    /* label, accelerator */
    "About",                                   /* tooltip */
    G_CALLBACK (about_cb) },
  { "Logo", "demo-btk-logo",                   /* name, stock id */
     NULL, NULL,                               /* label, accelerator */
    "BTK+",                                    /* tooltip */
    G_CALLBACK (activate_action) },
};
static guint n_entries = G_N_ELEMENTS (entries);


static BtkToggleActionEntry toggle_entries[] = {
  { "Bold", BTK_STOCK_BOLD,                    /* name, stock id */
     "_Bold", "<control>B",                    /* label, accelerator */
    "Bold",                                    /* tooltip */
    G_CALLBACK (activate_action),
    TRUE },                                    /* is_active */
};
static guint n_toggle_entries = G_N_ELEMENTS (toggle_entries);

enum {
  COLOR_RED,
  COLOR_GREEN,
  COLOR_BLUE
};

static BtkRadioActionEntry color_entries[] = {
  { "Red", NULL,                               /* name, stock id */
    "_Red", "<control>R",                      /* label, accelerator */
    "Blood", COLOR_RED },                      /* tooltip, value */
  { "Green", NULL,                             /* name, stock id */
    "_Green", "<control>G",                    /* label, accelerator */
    "Grass", COLOR_GREEN },                    /* tooltip, value */
  { "Blue", NULL,                              /* name, stock id */
    "_Blue", "<control>B",                     /* label, accelerator */
    "Sky", COLOR_BLUE },                       /* tooltip, value */
};
static guint n_color_entries = G_N_ELEMENTS (color_entries);

enum {
  SHAPE_SQUARE,
  SHAPE_RECTANGLE,
  SHAPE_OVAL
};

static BtkRadioActionEntry shape_entries[] = {
  { "Square", NULL,                            /* name, stock id */
    "_Square", "<control>S",                   /* label, accelerator */
    "Square",  SHAPE_SQUARE },                 /* tooltip, value */
  { "Rectangle", NULL,                         /* name, stock id */
    "_Rectangle", "<control>R",                /* label, accelerator */
    "Rectangle", SHAPE_RECTANGLE },            /* tooltip, value */
  { "Oval", NULL,                              /* name, stock id */
    "_Oval", "<control>O",                     /* label, accelerator */
    "Egg", SHAPE_OVAL },                       /* tooltip, value */
};
static guint n_shape_entries = G_N_ELEMENTS (shape_entries);

static const gchar *ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu action='FileMenu'>"
"      <menuitem action='New'/>"
"      <menuitem action='Open'/>"
"      <menuitem action='Save'/>"
"      <menuitem action='SaveAs'/>"
"      <separator/>"
"      <menuitem action='Quit'/>"
"    </menu>"
"    <menu action='PreferencesMenu'>"
"      <menu action='ColorMenu'>"
"	<menuitem action='Red'/>"
"	<menuitem action='Green'/>"
"	<menuitem action='Blue'/>"
"      </menu>"
"      <menu action='ShapeMenu'>"
"        <menuitem action='Square'/>"
"        <menuitem action='Rectangle'/>"
"        <menuitem action='Oval'/>"
"      </menu>"
"      <menuitem action='Bold'/>"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <menuitem action='About'/>"
"    </menu>"
"  </menubar>"
"  <toolbar name='ToolBar'>"
"    <toolitem action='Open'>"
"      <menu action='OpenMenu'>"
"        <menuitem action='File1'/>"
"      </menu>"
"    </toolitem>"
"    <toolitem action='Quit'/>"
"    <separator action='Sep1'/>"
"    <toolitem action='Logo'/>"
"  </toolbar>"
"</ui>";



/* This function registers our custom toolbar icons, so they can be themed.
 *
 * It's totally optional to do this, you could just manually insert icons
 * and have them not be themeable, especially if you never expect people
 * to theme your app.
 */
static void
register_stock_icons (void)
{
  static gboolean registered = FALSE;

  if (!registered)
    {
      BdkPixbuf *pixbuf;
      BtkIconFactory *factory;
      char *filename;

      static BtkStockItem items[] = {
        { "demo-btk-logo",
          "_BTK!",
          0, 0, NULL }
      };

      registered = TRUE;

      /* Register our stock items */
      btk_stock_add (items, G_N_ELEMENTS (items));

      /* Add our custom icon factory to the list of defaults */
      factory = btk_icon_factory_new ();
      btk_icon_factory_add_default (factory);

      /* demo_find_file() looks in the current directory first,
       * so you can run btk-demo without installing BTK, then looks
       * in the location where the file is installed.
       */
      pixbuf = NULL;
      filename = demo_find_file ("btk-logo-rgb.gif", NULL);
      if (filename)
	{
	  pixbuf = bdk_pixbuf_new_from_file (filename, NULL);
	  g_free (filename);
	}

      /* Register icon to accompany stock item */
      if (pixbuf != NULL)
        {
          BtkIconSet *icon_set;
          BdkPixbuf *transparent;

          /* The btk-logo-rgb icon has a white background, make it transparent */
          transparent = bdk_pixbuf_add_alpha (pixbuf, TRUE, 0xff, 0xff, 0xff);

          icon_set = btk_icon_set_new_from_pixbuf (transparent);
          btk_icon_factory_add (factory, "demo-btk-logo", icon_set);
          btk_icon_set_unref (icon_set);
          g_object_unref (pixbuf);
          g_object_unref (transparent);
        }
      else
        g_warning ("failed to load BTK logo for toolbar");

      /* Drop our reference to the factory, BTK will hold a reference. */
      g_object_unref (factory);
    }
}

static void
update_statusbar (BtkTextBuffer *buffer,
                  BtkStatusbar  *statusbar)
{
  gchar *msg;
  gint row, col;
  gint count;
  BtkTextIter iter;

  btk_statusbar_pop (statusbar, 0); /* clear any previous message,
				     * underflow is allowed
				     */

  count = btk_text_buffer_get_char_count (buffer);

  btk_text_buffer_get_iter_at_mark (buffer,
                                    &iter,
                                    btk_text_buffer_get_insert (buffer));

  row = btk_text_iter_get_line (&iter);
  col = btk_text_iter_get_line_offset (&iter);

  msg = g_strdup_printf ("Cursor at row %d column %d - %d chars in document",
                         row, col, count);

  btk_statusbar_push (statusbar, 0, msg);

  g_free (msg);
}

static void
mark_set_callback (BtkTextBuffer     *buffer,
                   const BtkTextIter *new_location,
                   BtkTextMark       *mark,
                   gpointer           data)
{
  update_statusbar (buffer, BTK_STATUSBAR (data));
}

static void
update_resize_grip (BtkWidget           *widget,
		    BdkEventWindowState *event,
		    BtkStatusbar        *statusbar)
{
  if (event->changed_mask & (BDK_WINDOW_STATE_MAXIMIZED |
			     BDK_WINDOW_STATE_FULLSCREEN))
    {
      gboolean maximized;

      maximized = event->new_window_state & (BDK_WINDOW_STATE_MAXIMIZED |
					     BDK_WINDOW_STATE_FULLSCREEN);
      btk_statusbar_set_has_resize_grip (statusbar, !maximized);
    }
}


BtkWidget *
do_appwindow (BtkWidget *do_widget)
{
  if (!window)
    {
      BtkWidget *table;
      BtkWidget *statusbar;
      BtkWidget *contents;
      BtkWidget *sw;
      BtkWidget *bar;
      BtkTextBuffer *buffer;
      BtkActionGroup *action_group;
      BtkAction *open_action;
      BtkUIManager *merge;
      GError *error = NULL;

      register_stock_icons ();

      /* Create the toplevel window
       */

      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (do_widget));
      btk_window_set_title (BTK_WINDOW (window), "Application Window");
      btk_window_set_icon_name (BTK_WINDOW (window), "btk-open");

      /* NULL window variable when window is closed */
      g_signal_connect (window, "destroy",
                        G_CALLBACK (btk_widget_destroyed),
                        &window);

      table = btk_table_new (1, 5, FALSE);

      btk_container_add (BTK_CONTAINER (window), table);

      /* Create the menubar and toolbar
       */

      action_group = btk_action_group_new ("AppWindowActions");
      open_action = g_object_new (tool_menu_action_get_type (),
				  "name", "Open",
				  "label", "_Open",
				  "tooltip", "Open a file",
				  "stock-id", BTK_STOCK_OPEN,
				  NULL);
      btk_action_group_add_action (action_group, open_action);
      g_object_unref (open_action);
      btk_action_group_add_actions (action_group,
				    entries, n_entries,
				    window);
      btk_action_group_add_toggle_actions (action_group,
					   toggle_entries, n_toggle_entries,
					   NULL);
      btk_action_group_add_radio_actions (action_group,
					  color_entries, n_color_entries,
					  COLOR_RED,
					  G_CALLBACK (activate_radio_action),
					  NULL);
      btk_action_group_add_radio_actions (action_group,
					  shape_entries, n_shape_entries,
					  SHAPE_SQUARE,
					  G_CALLBACK (activate_radio_action),
					  NULL);

      merge = btk_ui_manager_new ();
      g_object_set_data_full (B_OBJECT (window), "ui-manager", merge,
			      g_object_unref);
      btk_ui_manager_insert_action_group (merge, action_group, 0);
      btk_window_add_accel_group (BTK_WINDOW (window),
				  btk_ui_manager_get_accel_group (merge));

      if (!btk_ui_manager_add_ui_from_string (merge, ui_info, -1, &error))
	{
	  g_message ("building menus failed: %s", error->message);
	  g_error_free (error);
	}

      bar = btk_ui_manager_get_widget (merge, "/MenuBar");
      btk_widget_show (bar);
      btk_table_attach (BTK_TABLE (table),
			bar,
                        /* X direction */          /* Y direction */
                        0, 1,                      0, 1,
                        BTK_EXPAND | BTK_FILL,     0,
                        0,                         0);

      bar = btk_ui_manager_get_widget (merge, "/ToolBar");
      btk_widget_show (bar);
      btk_table_attach (BTK_TABLE (table),
			bar,
                        /* X direction */       /* Y direction */
                        0, 1,                   1, 2,
                        BTK_EXPAND | BTK_FILL,  0,
                        0,                      0);

      /* Create document
       */

      infobar = btk_info_bar_new ();
      btk_widget_set_no_show_all (infobar, TRUE);
      messagelabel = btk_label_new ("");
      btk_widget_show (messagelabel);
      btk_box_pack_start (BTK_BOX (btk_info_bar_get_content_area (BTK_INFO_BAR (infobar))),
                          messagelabel,
                          TRUE, TRUE, 0);
      btk_info_bar_add_button (BTK_INFO_BAR (infobar),
                               BTK_STOCK_OK, BTK_RESPONSE_OK);
      g_signal_connect (infobar, "response",
                        G_CALLBACK (btk_widget_hide), NULL);

      btk_table_attach (BTK_TABLE (table),
                        infobar,
                        /* X direction */       /* Y direction */
                        0, 1,                   2, 3,
                        BTK_EXPAND | BTK_FILL,  0,
                        0,                      0);

      sw = btk_scrolled_window_new (NULL, NULL);

      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (sw),
                                      BTK_POLICY_AUTOMATIC,
                                      BTK_POLICY_AUTOMATIC);

      btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (sw),
                                           BTK_SHADOW_IN);

      btk_table_attach (BTK_TABLE (table),
                        sw,
                        /* X direction */       /* Y direction */
                        0, 1,                   3, 4,
                        BTK_EXPAND | BTK_FILL,  BTK_EXPAND | BTK_FILL,
                        0,                      0);

      btk_window_set_default_size (BTK_WINDOW (window),
                                   200, 200);

      contents = btk_text_view_new ();
      btk_widget_grab_focus (contents);

      btk_container_add (BTK_CONTAINER (sw),
                         contents);

      /* Create statusbar */

      statusbar = btk_statusbar_new ();
      btk_table_attach (BTK_TABLE (table),
                        statusbar,
                        /* X direction */       /* Y direction */
                        0, 1,                   4, 5,
                        BTK_EXPAND | BTK_FILL,  0,
                        0,                      0);

      /* Show text widget info in the statusbar */
      buffer = btk_text_view_get_buffer (BTK_TEXT_VIEW (contents));
      
      g_signal_connect_object (buffer,
                               "changed",
                               G_CALLBACK (update_statusbar),
                               statusbar,
                               0);

      g_signal_connect_object (buffer,
                               "mark_set", /* cursor moved */
                               G_CALLBACK (mark_set_callback),
                               statusbar,
                               0);

      g_signal_connect_object (window,
			       "window_state_event",
			       G_CALLBACK (update_resize_grip),
			       statusbar,
			       0);

      update_statusbar (buffer, BTK_STATUSBAR (statusbar));
    }

  if (!btk_widget_get_visible (window))
    {
      btk_widget_show_all (window);
    }
  else
    {
      btk_widget_destroy (window);
      window = NULL;
      infobar = NULL;
      messagelabel = NULL;
    }

  return window;
}

