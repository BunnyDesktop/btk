
#include <btk/btk.h>

/* Obligatory basic callback */
static void print_hello( BtkWidget *w,
                         gpointer   data )
{
  g_message ("Hello, World!\n");
}

/* For the check button */
static void print_toggle( gpointer   callback_data,
                          guint      callback_action,
                          BtkWidget *menu_item )
{
   g_message ("Check button state - %d\n",
              BTK_CHECK_MENU_ITEM (menu_item)->active);
}

/* For the radio buttons */
static void print_selected( gpointer   callback_data,
                            guint      callback_action,
                            BtkWidget *menu_item )
{
   if(BTK_CHECK_MENU_ITEM(menu_item)->active)
     g_message ("Radio button %d selected\n", callback_action);
}

/* Our menu, an array of BtkItemFactoryEntry structures that defines each menu item */
static BtkItemFactoryEntry menu_items[] = {
  { "/_File",         NULL,         NULL,           0, "<Branch>" },
  { "/File/_New",     "<control>N", print_hello,    0, "<StockItem>", BTK_STOCK_NEW },
  { "/File/_Open",    "<control>O", print_hello,    0, "<StockItem>", BTK_STOCK_OPEN },
  { "/File/_Save",    "<control>S", print_hello,    0, "<StockItem>", BTK_STOCK_SAVE },
  { "/File/Save _As", NULL,         NULL,           0, "<Item>" },
  { "/File/sep1",     NULL,         NULL,           0, "<Separator>" },
  { "/File/_Quit",    "<CTRL>Q", btk_main_quit, 0, "<StockItem>", BTK_STOCK_QUIT },
  { "/_Options",      NULL,         NULL,           0, "<Branch>" },
  { "/Options/tear",  NULL,         NULL,           0, "<Tearoff>" },
  { "/Options/Check", NULL,         print_toggle,   1, "<CheckItem>" },
  { "/Options/sep",   NULL,         NULL,           0, "<Separator>" },
  { "/Options/Rad1",  NULL,         print_selected, 1, "<RadioItem>" },
  { "/Options/Rad2",  NULL,         print_selected, 2, "/Options/Rad1" },
  { "/Options/Rad3",  NULL,         print_selected, 3, "/Options/Rad1" },
  { "/_Help",         NULL,         NULL,           0, "<LastBranch>" },
  { "/_Help/About",   NULL,         NULL,           0, "<Item>" },
};

static gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

/* Returns a menubar widget made from the above menu */
static BtkWidget *get_menubar_menu( BtkWidget  *window )
{
  BtkItemFactory *item_factory;
  BtkAccelGroup *accel_group;

  /* Make an accelerator group (shortcut keys) */
  accel_group = btk_accel_group_new ();

  /* Make an ItemFactory (that makes a menubar) */
  item_factory = btk_item_factory_new (BTK_TYPE_MENU_BAR, "<main>",
                                       accel_group);

  /* This function generates the menu items. Pass the item factory,
     the number of items in the array, the array itself, and any
     callback data for the the menu items. */
  btk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);

  /* Attach the new accelerator group to the window. */
  btk_window_add_accel_group (BTK_WINDOW (window), accel_group);

  /* Finally, return the actual menu bar created by the item factory. */
  return btk_item_factory_get_widget (item_factory, "<main>");
}

/* Popup the menu when the popup button is pressed */
static gboolean popup_cb( BtkWidget *widget,
                          BdkEvent *event,
                          BtkWidget *menu )
{
   BdkEventButton *bevent = (BdkEventButton *)event;
  
   /* Only take button presses */
   if (event->type != BDK_BUTTON_PRESS)
     return FALSE;
  
   /* Show the menu */
   btk_menu_popup (BTK_MENU(menu), NULL, NULL,
                   NULL, NULL, bevent->button, bevent->time);
  
   return TRUE;
}

/* Same as with get_menubar_menu() but just return a button with a signal to
   call a popup menu */
BtkWidget *get_popup_menu( void )
{
   BtkItemFactory *item_factory;
   BtkWidget *button, *menu;
  
   /* Same as before but don't bother with the accelerators */
   item_factory = btk_item_factory_new (BTK_TYPE_MENU, "<main>",
                                        NULL);
   btk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);
   menu = btk_item_factory_get_widget (item_factory, "<main>");
  
   /* Make a button to activate the popup menu */
   button = btk_button_new_with_label ("Popup");
   /* Make the menu popup when clicked */
   g_signal_connect (B_OBJECT(button),
                     "event",
                     G_CALLBACK(popup_cb),
                     (gpointer) menu);

   return button;
}

/* Same again but return an option menu */
BtkWidget *get_option_menu( void )
{
   BtkItemFactory *item_factory;
   BtkWidget *option_menu;
  
   /* Same again, not bothering with the accelerators */
   item_factory = btk_item_factory_new (BTK_TYPE_OPTION_MENU, "<main>",
                                        NULL);
   btk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);
   option_menu = btk_item_factory_get_widget (item_factory, "<main>");

   return option_menu;
}

/* You have to start somewhere */
int main( int argc,
          char *argv[] )
{
  BtkWidget *window;
  BtkWidget *main_vbox;
  BtkWidget *menubar, *option_menu, *popup_button;
 
  /* Initialize BTK */
  btk_init (&argc, &argv);
 
  /* Make a window */
  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  g_signal_connect (B_OBJECT (window), "destroy",
                    G_CALLBACK (btk_main_quit),
                    NULL);
  btk_window_set_title (BTK_WINDOW(window), "Item Factory");
  btk_widget_set_size_request (BTK_WIDGET(window), 300, 200);
 
  /* Make a vbox to put the three menus in */
  main_vbox = btk_vbox_new (FALSE, 1);
  btk_container_set_border_width (BTK_CONTAINER (main_vbox), 1);
  btk_container_add (BTK_CONTAINER (window), main_vbox);
 
  /* Get the three types of menu */
  /* Note: all three menus are separately created, so they are not the
     same menu */
  menubar = get_menubar_menu (window);
  popup_button = get_popup_menu ();
  option_menu = get_option_menu ();
  
  /* Pack it all together */
  btk_box_pack_start (BTK_BOX (main_vbox), menubar, FALSE, TRUE, 0);
  btk_box_pack_end (BTK_BOX (main_vbox), popup_button, FALSE, TRUE, 0);
  btk_box_pack_end (BTK_BOX (main_vbox), option_menu, FALSE, TRUE, 0);

  /* Show the widgets */
  btk_widget_show_all (window);
  
  /* Finished! */
  btk_main ();
 
  return 0;
}
