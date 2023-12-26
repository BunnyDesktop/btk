/* UI Manager
 *
 * The BtkUIManager object allows the easy creation of menus
 * from an array of actions and a description of the menu hierarchy.
 */

#include <btk/btk.h>

static void
activate_action (BtkAction *action)
{
  g_message ("Action \"%s\" activated", btk_action_get_name (action));
}

static void
activate_radio_action (BtkAction *action, BtkRadioAction *current)
{
  g_message ("Radio action \"%s\" selected", 
	     btk_action_get_name (BTK_ACTION (current)));
}

static BtkActionEntry entries[] = {
  { "FileMenu", NULL, "_File" },               /* name, stock id, label */
  { "PreferencesMenu", NULL, "_Preferences" }, /* name, stock id, label */
  { "ColorMenu", NULL, "_Color"  },            /* name, stock id, label */
  { "ShapeMenu", NULL, "_Shape" },             /* name, stock id, label */
  { "HelpMenu", NULL, "_Help" },               /* name, stock id, label */
  { "New", BTK_STOCK_NEW,                      /* name, stock id */
    "_New", "<control>N",                      /* label, accelerator */
    "Create a new file",                       /* tooltip */ 
    G_CALLBACK (activate_action) },      
  { "Open", BTK_STOCK_OPEN,                    /* name, stock id */
    "_Open","<control>O",                      /* label, accelerator */     
    "Open a file",                             /* tooltip */
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
    G_CALLBACK (activate_action) },
  { "Logo", "demo-btk-logo",                   /* name, stock id */
     NULL, NULL,                               /* label, accelerator */     
    "BTK+",                                    /* tooltip */
    G_CALLBACK (activate_action) },
};
static buint n_entries = G_N_ELEMENTS (entries);


static BtkToggleActionEntry toggle_entries[] = {
  { "Bold", BTK_STOCK_BOLD,                    /* name, stock id */
     "_Bold", "<control>B",                    /* label, accelerator */     
    "Bold",                                    /* tooltip */
    G_CALLBACK (activate_action), 
    TRUE },                                    /* is_active */
};
static buint n_toggle_entries = G_N_ELEMENTS (toggle_entries);

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
static buint n_color_entries = G_N_ELEMENTS (color_entries);

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
static buint n_shape_entries = G_N_ELEMENTS (shape_entries);

static const bchar *ui_info = 
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
"  <toolbar  name='ToolBar'>"
"    <toolitem action='Open'/>"
"    <toolitem action='Quit'/>"
"    <separator action='Sep1'/>"
"    <toolitem action='Logo'/>"
"  </toolbar>"
"</ui>";

BtkWidget *
do_ui_manager (BtkWidget *do_widget)
{
  static BtkWidget *window = NULL;
  
  if (!window)
    {
      BtkWidget *box1;
      BtkWidget *box2;
      BtkWidget *separator;
      BtkWidget *label;
      BtkWidget *button;
      BtkUIManager *ui;
      BtkActionGroup *actions;
      GError *error = NULL;

      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (do_widget));
      
      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed), &window);
      g_signal_connect (window, "delete-event",
			G_CALLBACK (btk_true), NULL);

      actions = btk_action_group_new ("Actions");
      btk_action_group_add_actions (actions, entries, n_entries, NULL);
      btk_action_group_add_toggle_actions (actions, 
					   toggle_entries, n_toggle_entries, 
					   NULL);
      btk_action_group_add_radio_actions (actions, 
					  color_entries, n_color_entries, 
					  COLOR_RED,
					  G_CALLBACK (activate_radio_action), 
					  NULL);
      btk_action_group_add_radio_actions (actions, 
					  shape_entries, n_shape_entries, 
					  SHAPE_OVAL,
					  G_CALLBACK (activate_radio_action), 
					  NULL);

      ui = btk_ui_manager_new ();
      btk_ui_manager_insert_action_group (ui, actions, 0);
      g_object_unref (actions);
      btk_window_add_accel_group (BTK_WINDOW (window), 
				  btk_ui_manager_get_accel_group (ui));
      btk_window_set_title (BTK_WINDOW (window), "UI Manager");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      
      if (!btk_ui_manager_add_ui_from_string (ui, ui_info, -1, &error))
	{
	  g_message ("building menus failed: %s", error->message);
	  g_error_free (error);
	}

      box1 = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), box1);
      
      btk_box_pack_start (BTK_BOX (box1),
			  btk_ui_manager_get_widget (ui, "/MenuBar"),
			  FALSE, FALSE, 0);

      label = btk_label_new ("Type\n<alt>\nto start");
      btk_widget_set_size_request (label, 200, 200);
      btk_misc_set_alignment (BTK_MISC (label), 0.5, 0.5);
      btk_box_pack_start (BTK_BOX (box1), label, TRUE, TRUE, 0);


      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);


      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);

      button = btk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
				G_CALLBACK (btk_widget_destroy), window);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
      btk_widget_set_can_default (button, TRUE);
      btk_widget_grab_default (button);

      btk_widget_show_all (window);
      g_object_unref (ui);
    }
  else
    {
      btk_widget_destroy (window);
      window = NULL;
    }

  return window;
}
