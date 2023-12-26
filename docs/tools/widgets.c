#include "config.h"

#include <btk/btkunixprint.h>
#include <bdk/bdkkeysyms.h>
#include <X11/Xatom.h>
#include <bdkx.h>
#include "widgets.h"

#define SMALL_WIDTH  240
#define SMALL_HEIGHT 75
#define MEDIUM_WIDTH 240
#define MEDIUM_HEIGHT 165
#define LARGE_WIDTH 240
#define LARGE_HEIGHT 240

static Window
find_toplevel_window (Window xid)
{
  Window root, parent, *children;
  guint nchildren;

  do
    {
      if (XQueryTree (BDK_DISPLAY_XDISPLAY (bdk_display_get_default ()), xid, &root,
		      &parent, &children, &nchildren) == 0)
	{
	  g_warning ("Couldn't find window manager window");
	  return None;
	}

      if (root == parent)
	return xid;

      xid = parent;
    }
  while (TRUE);
}


static gboolean
adjust_size_callback (WidgetInfo *info)
{
  Window toplevel;
  Window root;
  gint tx;
  gint ty;
  guint twidth;
  guint theight;
  guint tborder_width;
  guint tdepth;
  gint target_width = 0;
  gint target_height = 0;

  toplevel = find_toplevel_window (BDK_WINDOW_XID (info->window->window));
  XGetGeometry (BDK_WINDOW_XDISPLAY (info->window->window),
		toplevel,
		&root, &tx, &ty, &twidth, &theight, &tborder_width, &tdepth);

  switch (info->size)
    {
    case SMALL:
      target_width = SMALL_WIDTH;
      target_height = SMALL_HEIGHT;
      break;
    case MEDIUM:
      target_width = MEDIUM_WIDTH;
      target_height = MEDIUM_HEIGHT;
      break;
    case LARGE:
      target_width = LARGE_WIDTH;
      target_height = LARGE_HEIGHT;
      break;
    case ASIS:
      target_width = twidth;
      target_height = theight;
      break;
    }

  if (twidth > target_width ||
      theight > target_height)
    {
      btk_widget_set_size_request (info->window,
				   2 + target_width - (twidth - target_width), /* Dunno why I need the +2 fudge factor; */
				   2 + target_height - (theight - target_height));
    }
  return FALSE;
}

static void
realize_callback (BtkWidget  *widget,
		  WidgetInfo *info)
{
  bdk_threads_add_timeout (500, (GSourceFunc)adjust_size_callback, info);
}

static WidgetInfo *
new_widget_info (const char *name,
		 BtkWidget  *widget,
		 WidgetSize  size)
{
  WidgetInfo *info;

  info = g_new0 (WidgetInfo, 1);
  info->name = g_strdup (name);
  info->size = size;
  if (BTK_IS_WINDOW (widget))
    {
      info->window = widget;
      btk_window_set_resizable (BTK_WINDOW (info->window), FALSE);
      info->include_decorations = TRUE;
      g_signal_connect (info->window, "realize", G_CALLBACK (realize_callback), info);
    }
  else
    {
      info->window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      info->include_decorations = FALSE;
      btk_widget_show_all (widget);
      btk_container_add (BTK_CONTAINER (info->window), widget);
    }
  info->no_focus = TRUE;

  btk_widget_set_app_paintable (info->window, TRUE);
  g_signal_connect (info->window, "focus", G_CALLBACK (btk_true), NULL);
  btk_container_set_border_width (BTK_CONTAINER (info->window), 12);

  switch (size)
    {
    case SMALL:
      btk_widget_set_size_request (info->window,
				   240, 75);
      break;
    case MEDIUM:
      btk_widget_set_size_request (info->window,
				   240, 165);
      break;
    case LARGE:
      btk_widget_set_size_request (info->window,
				   240, 240);
      break;
    default:
	break;
    }

  return info;
}

static WidgetInfo *
create_button (void)
{
  BtkWidget *widget;
  BtkWidget *align;

  widget = btk_button_new_with_mnemonic ("_Button");
  align = btk_alignment_new (0.5, 0.5, 0.0, 0.0);
  btk_container_add (BTK_CONTAINER (align), widget);

  return new_widget_info ("button", align, SMALL);
}

static WidgetInfo *
create_toggle_button (void)
{
  BtkWidget *widget;
  BtkWidget *align;

  widget = btk_toggle_button_new_with_mnemonic ("_Toggle Button");
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (widget), FALSE);
  align = btk_alignment_new (0.5, 0.5, 0.0, 0.0);
  btk_container_add (BTK_CONTAINER (align), widget);

  return new_widget_info ("toggle-button", align, SMALL);
}

static WidgetInfo *
create_check_button (void)
{
  BtkWidget *widget;
  BtkWidget *align;

  widget = btk_check_button_new_with_mnemonic ("_Check Button");
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (widget), TRUE);
  align = btk_alignment_new (0.5, 0.5, 0.0, 0.0);
  btk_container_add (BTK_CONTAINER (align), widget);

  return new_widget_info ("check-button", align, SMALL);
}

static WidgetInfo *
create_link_button (void)
{
  BtkWidget *widget;
  BtkWidget *align;

  widget = btk_link_button_new_with_label ("http://www.btk.org", "Link Button");
  align = btk_alignment_new (0.5, 0.5, 0.0, 0.0);
  btk_container_add (BTK_CONTAINER (align), widget);

  return new_widget_info ("link-button", align, SMALL);
}

static WidgetInfo *
create_entry (void)
{
  BtkWidget *widget;
  BtkWidget *align;

  widget = btk_entry_new ();
  btk_entry_set_text (BTK_ENTRY (widget), "Entry");
  btk_editable_set_position (BTK_EDITABLE (widget), -1);
  align = btk_alignment_new (0.5, 0.5, 0.0, 0.0);
  btk_container_add (BTK_CONTAINER (align), widget);

  return  new_widget_info ("entry", align, SMALL);
}

static WidgetInfo *
create_radio (void)
{
  BtkWidget *widget;
  BtkWidget *radio;
  BtkWidget *align;

  widget = btk_vbox_new (FALSE, 3);
  radio = btk_radio_button_new_with_mnemonic (NULL, "Radio Button _One");
  btk_box_pack_start (BTK_BOX (widget), radio, FALSE, FALSE, 0);
  radio = btk_radio_button_new_with_mnemonic_from_widget (BTK_RADIO_BUTTON (radio), "Radio Button _Two");
  btk_box_pack_start (BTK_BOX (widget), radio, FALSE, FALSE, 0);
  radio = btk_radio_button_new_with_mnemonic_from_widget (BTK_RADIO_BUTTON (radio), "Radio Button T_hree");
  btk_box_pack_start (BTK_BOX (widget), radio, FALSE, FALSE, 0);
  align = btk_alignment_new (0.5, 0.5, 0.0, 0.0);
  btk_container_add (BTK_CONTAINER (align), widget);

  return new_widget_info ("radio-group", align, MEDIUM);
}

static WidgetInfo *
create_label (void)
{
  BtkWidget *widget;
  BtkWidget *align;

  widget = btk_label_new ("Label");
  align = btk_alignment_new (0.5, 0.5, 0.0, 0.0);
  btk_container_add (BTK_CONTAINER (align), widget);

  return new_widget_info ("label", align, SMALL);
}

static WidgetInfo *
create_accel_label (void)
{
  WidgetInfo *info;
  BtkWidget *widget, *button, *box;
  BtkAccelGroup *accel_group;

  widget = btk_accel_label_new ("Accel Label");

  button = btk_button_new_with_label ("Quit");
  btk_accel_label_set_accel_widget (BTK_ACCEL_LABEL (widget), button);
  btk_widget_set_no_show_all (button, TRUE);

  box = btk_vbox_new (FALSE, 0);
  btk_container_add (BTK_CONTAINER (box), widget);
  btk_container_add (BTK_CONTAINER (box), button);

  btk_accel_label_set_accel_widget (BTK_ACCEL_LABEL (widget), button);
  accel_group = btk_accel_group_new();

  info = new_widget_info ("accel-label", box, SMALL);

  btk_widget_add_accelerator (button, "activate", accel_group, BDK_Q, BDK_CONTROL_MASK,
			      BTK_ACCEL_VISIBLE | BTK_ACCEL_LOCKED);

  return info;
}

static WidgetInfo *
create_combo_box_entry (void)
{
  BtkWidget *widget;
  BtkWidget *align;
  BtkWidget *child;
  BtkTreeModel *model;
  
  btk_rc_parse_string ("style \"combo-box-entry-style\" {\n"
		       "  BtkComboBox::appears-as-list = 1\n"
		       "}\n"
		       "widget_class \"BtkComboBoxEntry\" style \"combo-box-entry-style\"\n" );

  model = (BtkTreeModel *)btk_list_store_new (1, G_TYPE_STRING);
  widget = g_object_new (BTK_TYPE_COMBO_BOX,
			 "has-entry", TRUE,
			 "model", model,
			 "entry-text-column", 0,
			 NULL);
  g_object_unref (model);

  child = btk_bin_get_child (BTK_BIN (widget));
  btk_entry_set_text (BTK_ENTRY (child), "Combo Box Entry");
  align = btk_alignment_new (0.5, 0.5, 0.0, 0.0);
  btk_container_add (BTK_CONTAINER (align), widget);

  return new_widget_info ("combo-box-entry", align, SMALL);
}

static WidgetInfo *
create_combo_box (void)
{
  BtkWidget *widget;
  BtkWidget *align;
  
  btk_rc_parse_string ("style \"combo-box-style\" {\n"
		       "  BtkComboBox::appears-as-list = 0\n"
		       "}\n"
		       "widget_class \"BtkComboBox\" style \"combo-box-style\"\n" );

  widget = btk_combo_box_text_new ();
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (widget), "Combo Box");
  btk_combo_box_set_active (BTK_COMBO_BOX (widget), 0);
  align = btk_alignment_new (0.5, 0.5, 0.0, 0.0);
  btk_container_add (BTK_CONTAINER (align), widget);

  return new_widget_info ("combo-box", align, SMALL);
}

static WidgetInfo *
create_recent_chooser_dialog (void)
{
  WidgetInfo *info;
  BtkWidget *widget;

  widget = btk_recent_chooser_dialog_new ("Recent Chooser Dialog",
					  NULL,
					  BTK_STOCK_CANCEL, BTK_RESPONSE_CANCEL,
					  BTK_STOCK_OPEN, BTK_RESPONSE_ACCEPT,
					  NULL); 
  btk_window_set_default_size (BTK_WINDOW (widget), 505, 305);
  
  info = new_widget_info ("recentchooserdialog", widget, ASIS);
  info->include_decorations = TRUE;

  return info;
}

static WidgetInfo *
create_text_view (void)
{
  BtkWidget *widget;
  BtkWidget *text_view;

  widget = btk_frame_new (NULL);
  btk_frame_set_shadow_type (BTK_FRAME (widget), BTK_SHADOW_IN);
  text_view = btk_text_view_new ();
  btk_container_add (BTK_CONTAINER (widget), text_view);
  /* Bad hack to add some size to the widget */
  btk_text_buffer_set_text (btk_text_view_get_buffer (BTK_TEXT_VIEW (text_view)),
			    "Multiline\nText\n\n", -1);
  btk_text_view_set_cursor_visible (BTK_TEXT_VIEW (text_view), FALSE);

  return new_widget_info ("multiline-text", widget, MEDIUM);
}

static WidgetInfo *
create_tree_view (void)
{
  BtkWidget *widget;
  BtkWidget *tree_view;
  BtkListStore *list_store;
  BtkTreeIter iter;
  WidgetInfo *info;

  widget = btk_frame_new (NULL);
  btk_frame_set_shadow_type (BTK_FRAME (widget), BTK_SHADOW_IN);
  list_store = btk_list_store_new (1, G_TYPE_STRING);
  btk_list_store_append (list_store, &iter);
  btk_list_store_set (list_store, &iter, 0, "Line One", -1);
  btk_list_store_append (list_store, &iter);
  btk_list_store_set (list_store, &iter, 0, "Line Two", -1);
  btk_list_store_append (list_store, &iter);
  btk_list_store_set (list_store, &iter, 0, "Line Three", -1);

  tree_view = btk_tree_view_new_with_model (BTK_TREE_MODEL (list_store));
  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tree_view),
					       0, "List and Tree",
					       btk_cell_renderer_text_new (),
					       "text", 0, NULL);
  btk_container_add (BTK_CONTAINER (widget), tree_view);

  info = new_widget_info ("list-and-tree", widget, MEDIUM);
  info->no_focus = FALSE;

  return info;
}

static WidgetInfo *
create_icon_view (void)
{
  BtkWidget *widget;
  BtkWidget *vbox;
  BtkWidget *align;
  BtkWidget *icon_view;
  BtkListStore *list_store;
  BtkTreeIter iter;
  BdkPixbuf *pixbuf;
  WidgetInfo *info;

  widget = btk_frame_new (NULL);
  btk_frame_set_shadow_type (BTK_FRAME (widget), BTK_SHADOW_IN);
  list_store = btk_list_store_new (2, G_TYPE_STRING, BDK_TYPE_PIXBUF);
  btk_list_store_append (list_store, &iter);
  pixbuf = bdk_pixbuf_new_from_file ("folder.png", NULL);
  btk_list_store_set (list_store, &iter, 0, "One", 1, pixbuf, -1);
  btk_list_store_append (list_store, &iter);
  pixbuf = bdk_pixbuf_new_from_file ("bunny.png", NULL);
  btk_list_store_set (list_store, &iter, 0, "Two", 1, pixbuf, -1);

  icon_view = btk_icon_view_new();

  btk_icon_view_set_model (BTK_ICON_VIEW (icon_view), BTK_TREE_MODEL (list_store));
  btk_icon_view_set_text_column (BTK_ICON_VIEW (icon_view), 0);
  btk_icon_view_set_pixbuf_column (BTK_ICON_VIEW (icon_view), 1);

  btk_container_add (BTK_CONTAINER (widget), icon_view);

  vbox = btk_vbox_new (FALSE, 3);
  align = btk_alignment_new (0.5, 0.5, 1.0, 1.0);
  btk_container_add (BTK_CONTAINER (align), widget);
  btk_box_pack_start (BTK_BOX (vbox), align, TRUE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (vbox),
		      btk_label_new ("Icon View"),
		      FALSE, FALSE, 0);

  info = new_widget_info ("icon-view", vbox, MEDIUM);
  info->no_focus = FALSE;

  return info;
}

static WidgetInfo *
create_color_button (void)
{
  BtkWidget *vbox;
  BtkWidget *picker;
  BtkWidget *align;
  BdkColor color;

  vbox = btk_vbox_new (FALSE, 3);
  align = btk_alignment_new (0.5, 0.5, 0.0, 0.0);
  color.red = 0x1e<<8;  /* Go Gagne! */
  color.green = 0x90<<8;
  color.blue = 0xff<<8;
  picker = btk_color_button_new_with_color (&color);
  btk_container_add (BTK_CONTAINER (align), picker);
  btk_box_pack_start (BTK_BOX (vbox), align, FALSE, FALSE, 0);
  btk_box_pack_start (BTK_BOX (vbox),
		      btk_label_new ("Color Button"),
		      FALSE, FALSE, 0);

  return new_widget_info ("color-button", vbox, SMALL);
}

static WidgetInfo *
create_font_button (void)
{
  BtkWidget *vbox;
  BtkWidget *picker;
  BtkWidget *align;

  vbox = btk_vbox_new (FALSE, 3);
  align = btk_alignment_new (0.5, 0.5, 0.0, 0.0);
  picker = btk_font_button_new_with_font ("Sans Serif 10");
  btk_container_add (BTK_CONTAINER (align), picker);
  btk_box_pack_start (BTK_BOX (vbox), align, FALSE, FALSE, 0);
  btk_box_pack_start (BTK_BOX (vbox),
		      btk_label_new ("Font Button"),
		      FALSE, FALSE, 0);

  return new_widget_info ("font-button", vbox, SMALL);
}

static WidgetInfo *
create_file_button (void)
{
  BtkWidget *vbox;
  BtkWidget *vbox2;
  BtkWidget *picker;
  BtkWidget *align;

  vbox = btk_vbox_new (FALSE, 12);
  vbox2 = btk_vbox_new (FALSE, 3);
  align = btk_alignment_new (0.5, 0.5, 0.0, 0.0);
  picker = btk_file_chooser_button_new ("File Chooser Button",
		  			BTK_FILE_CHOOSER_ACTION_OPEN);
  btk_widget_set_size_request (picker, 150, -1);
  btk_file_chooser_set_filename (BTK_FILE_CHOOSER (picker), "/etc/yum.conf");
  btk_container_add (BTK_CONTAINER (align), picker);
  btk_box_pack_start (BTK_BOX (vbox2), align, FALSE, FALSE, 0);
  btk_box_pack_start (BTK_BOX (vbox2),
		      btk_label_new ("File Button (Files)"),
		      FALSE, FALSE, 0);

  btk_box_pack_start (BTK_BOX (vbox),
		      vbox2, TRUE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (vbox),
		      btk_hseparator_new (),
		      FALSE, FALSE, 0);

  vbox2 = btk_vbox_new (FALSE, 3);
  align = btk_alignment_new (0.5, 0.5, 0.0, 0.0);
  picker = btk_file_chooser_button_new ("File Chooser Button",
		  			BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  btk_widget_set_size_request (picker, 150, -1);
  btk_file_chooser_set_filename (BTK_FILE_CHOOSER (picker), "/");
  btk_container_add (BTK_CONTAINER (align), picker);
  btk_box_pack_start (BTK_BOX (vbox2), align, FALSE, FALSE, 0);
  btk_box_pack_start (BTK_BOX (vbox2),
		      btk_label_new ("File Button (Select Folder)"),
		      FALSE, FALSE, 0);
  btk_box_pack_start (BTK_BOX (vbox),
		      vbox2, TRUE, TRUE, 0);

  return new_widget_info ("file-button", vbox, MEDIUM);
}

static WidgetInfo *
create_separator (void)
{
  BtkWidget *hbox;
  BtkWidget *vbox;

  vbox = btk_vbox_new (FALSE, 3);
  hbox = btk_hbox_new (TRUE, 0);
  btk_box_pack_start (BTK_BOX (hbox),
		      btk_hseparator_new (),
		      TRUE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (hbox),
		      btk_vseparator_new (),
		      TRUE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (vbox),
		      g_object_new (BTK_TYPE_LABEL,
				    "label", "Horizontal and Vertical\nSeparators",
				    "justify", BTK_JUSTIFY_CENTER,
				    NULL),
		      FALSE, FALSE, 0);
  return new_widget_info ("separator", vbox, MEDIUM);
}

static WidgetInfo *
create_panes (void)
{
  BtkWidget *hbox;
  BtkWidget *vbox;
  BtkWidget *pane;

  vbox = btk_vbox_new (FALSE, 3);
  hbox = btk_hbox_new (TRUE, 12);
  pane = btk_hpaned_new ();
  btk_paned_pack1 (BTK_PANED (pane),
		   g_object_new (BTK_TYPE_FRAME,
				 "shadow", BTK_SHADOW_IN,
				 NULL),
		   FALSE, FALSE);
  btk_paned_pack2 (BTK_PANED (pane),
		   g_object_new (BTK_TYPE_FRAME,
				 "shadow", BTK_SHADOW_IN,
				 NULL),
		   FALSE, FALSE);
  btk_box_pack_start (BTK_BOX (hbox),
		      pane,
		      TRUE, TRUE, 0);
  pane = btk_vpaned_new ();
  btk_paned_pack1 (BTK_PANED (pane),
		   g_object_new (BTK_TYPE_FRAME,
				 "shadow", BTK_SHADOW_IN,
				 NULL),
		   FALSE, FALSE);
  btk_paned_pack2 (BTK_PANED (pane),
		   g_object_new (BTK_TYPE_FRAME,
				 "shadow", BTK_SHADOW_IN,
				 NULL),
		   FALSE, FALSE);
  btk_box_pack_start (BTK_BOX (hbox),
		      pane,
		      TRUE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (vbox),
		      g_object_new (BTK_TYPE_LABEL,
				    "label", "Horizontal and Vertical\nPanes",
				    "justify", BTK_JUSTIFY_CENTER,
				    NULL),
		      FALSE, FALSE, 0);
  return new_widget_info ("panes", vbox, MEDIUM);
}

static WidgetInfo *
create_frame (void)
{
  BtkWidget *widget;

  widget = btk_frame_new ("Frame");

  return new_widget_info ("frame", widget, MEDIUM);
}

static WidgetInfo *
create_window (void)
{
  WidgetInfo *info;
  BtkWidget *widget;

  widget = btk_window_new (BTK_WINDOW_TOPLEVEL);
  info = new_widget_info ("window", widget, MEDIUM);
  info->include_decorations = TRUE;
  btk_window_set_title (BTK_WINDOW (info->window), "Window");

  return info;
}

static WidgetInfo *
create_colorsel (void)
{
  WidgetInfo *info;
  BtkWidget *widget;
  BtkColorSelection *colorsel;
  BdkColor color;

  widget = btk_color_selection_dialog_new ("Color Selection Dialog");
  colorsel = BTK_COLOR_SELECTION (BTK_COLOR_SELECTION_DIALOG (widget)->colorsel);

  color.red   = 0x7979;
  color.green = 0xdbdb;
  color.blue  = 0x9595;

  btk_color_selection_set_previous_color (colorsel, &color);
  
  color.red   = 0x7d7d;
  color.green = 0x9393;
  color.blue  = 0xc3c3;
  
  btk_color_selection_set_current_color (colorsel, &color);

  info = new_widget_info ("colorsel", widget, ASIS);
  info->include_decorations = TRUE;

  return info;
}

static WidgetInfo *
create_fontsel (void)
{
  WidgetInfo *info;
  BtkWidget *widget;

  widget = btk_font_selection_dialog_new ("Font Selection Dialog");
  info = new_widget_info ("fontsel", widget, ASIS);
  info->include_decorations = TRUE;

  return info;
}

static WidgetInfo *
create_filesel (void)
{
  WidgetInfo *info;
  BtkWidget *widget;

  widget = btk_file_chooser_dialog_new ("File Chooser Dialog",
					NULL,
					BTK_FILE_CHOOSER_ACTION_OPEN,
					BTK_STOCK_CANCEL, BTK_RESPONSE_CANCEL,
					BTK_STOCK_OPEN, BTK_RESPONSE_ACCEPT,
					NULL); 
  btk_window_set_default_size (BTK_WINDOW (widget), 505, 305);
  
  info = new_widget_info ("filechooser", widget, ASIS);
  info->include_decorations = TRUE;

  return info;
}

static WidgetInfo *
create_print_dialog (void)
{
  WidgetInfo *info;
  BtkWidget *widget;

  widget = btk_print_unix_dialog_new ("Print Dialog", NULL);   
  btk_widget_set_size_request (widget, 505, 350);
  info = new_widget_info ("printdialog", widget, ASIS);
  info->include_decorations = TRUE;

  return info;
}

static WidgetInfo *
create_page_setup_dialog (void)
{
  WidgetInfo *info;
  BtkWidget *widget;
  BtkPageSetup *page_setup;
  BtkPrintSettings *settings;

  page_setup = btk_page_setup_new ();
  settings = btk_print_settings_new ();
  widget = btk_page_setup_unix_dialog_new ("Page Setup Dialog", NULL);   
  btk_page_setup_unix_dialog_set_page_setup (BTK_PAGE_SETUP_UNIX_DIALOG (widget),
					     page_setup);
  btk_page_setup_unix_dialog_set_print_settings (BTK_PAGE_SETUP_UNIX_DIALOG (widget),
						 settings);

  info = new_widget_info ("pagesetupdialog", widget, ASIS);
  btk_widget_set_app_paintable (info->window, FALSE);
  info->include_decorations = TRUE;

  return info;
}

static WidgetInfo *
create_toolbar (void)
{
  BtkWidget *widget, *menu;
  BtkToolItem *item;

  widget = btk_toolbar_new ();

  item = btk_tool_button_new_from_stock (BTK_STOCK_NEW);
  btk_toolbar_insert (BTK_TOOLBAR (widget), item, -1);

  item = btk_menu_tool_button_new_from_stock (BTK_STOCK_OPEN);
  menu = btk_menu_new ();
  btk_menu_tool_button_set_menu (BTK_MENU_TOOL_BUTTON (item), menu);
  btk_toolbar_insert (BTK_TOOLBAR (widget), item, -1);

  item = btk_tool_button_new_from_stock (BTK_STOCK_REFRESH);
  btk_toolbar_insert (BTK_TOOLBAR (widget), item, -1);

  btk_toolbar_set_show_arrow (BTK_TOOLBAR (widget), FALSE);

  return new_widget_info ("toolbar", widget, SMALL);
}

static WidgetInfo *
create_toolpalette (void)
{
  BtkWidget *widget, *group;
  BtkToolItem *item;

  widget = btk_tool_palette_new ();
  group = btk_tool_item_group_new ("Tools");
  btk_container_add (BTK_CONTAINER (widget), group);
  item = btk_tool_button_new_from_stock (BTK_STOCK_ABOUT);
  btk_tool_item_group_insert (BTK_TOOL_ITEM_GROUP (group), item, -1);
  item = btk_tool_button_new_from_stock (BTK_STOCK_FILE);
  btk_tool_item_group_insert (BTK_TOOL_ITEM_GROUP (group), item, -1);
  item = btk_tool_button_new_from_stock (BTK_STOCK_CONNECT);
  btk_tool_item_group_insert (BTK_TOOL_ITEM_GROUP (group), item, -1);

  group = btk_tool_item_group_new ("More tools");
  btk_container_add (BTK_CONTAINER (widget), group);
  item = btk_tool_button_new_from_stock (BTK_STOCK_CUT);
  btk_tool_item_group_insert (BTK_TOOL_ITEM_GROUP (group), item, -1);
  item = btk_tool_button_new_from_stock (BTK_STOCK_EXECUTE);
  btk_tool_item_group_insert (BTK_TOOL_ITEM_GROUP (group), item, -1);
  item = btk_tool_button_new_from_stock (BTK_STOCK_CANCEL);
  btk_tool_item_group_insert (BTK_TOOL_ITEM_GROUP (group), item, -1);

  return new_widget_info ("toolpalette", widget, MEDIUM);
}

static WidgetInfo *
create_menubar (void)
{
  BtkWidget *widget, *vbox, *align, *item;

  widget = btk_menu_bar_new ();

  item = btk_menu_item_new_with_mnemonic ("_File");
  btk_menu_shell_append (BTK_MENU_SHELL (widget), item);

  item = btk_menu_item_new_with_mnemonic ("_Edit");
  btk_menu_shell_append (BTK_MENU_SHELL (widget), item);

  item = btk_menu_item_new_with_mnemonic ("_Help");
  btk_menu_shell_append (BTK_MENU_SHELL (widget), item);

  vbox = btk_vbox_new (FALSE, 3);
  align = btk_alignment_new (0.5, 0.5, 0.0, 0.0);
  btk_container_add (BTK_CONTAINER (align), widget);
  btk_box_pack_start (BTK_BOX (vbox), align, FALSE, FALSE, 0);
  btk_box_pack_start (BTK_BOX (vbox),
		      btk_label_new ("Menu Bar"),
		      FALSE, FALSE, 0);

  return new_widget_info ("menubar", vbox, SMALL);
}

static WidgetInfo *
create_message_dialog (void)
{
  BtkWidget *widget;

  widget = btk_message_dialog_new (NULL,
				   0,
				   BTK_MESSAGE_INFO,
				   BTK_BUTTONS_OK,
				   NULL);
  btk_window_set_icon_name (BTK_WINDOW (widget), "btk-copy");
  btk_message_dialog_set_markup (BTK_MESSAGE_DIALOG (widget),
				 "<b>Message Dialog</b>\n\nWith secondary text");
  return new_widget_info ("messagedialog", widget, ASIS);
}

static WidgetInfo *
create_about_dialog (void)
{
  BtkWidget *widget;
  const gchar *authors[] = {
    "Peter Mattis",
    "Spencer Kimball",
    "Josh MacDonald",
    "and many more...",
    NULL
  };

  widget = btk_about_dialog_new ();
  g_object_set (widget,
                "program-name", "BTK+ Code Demos",
                "version", PACKAGE_VERSION,
                "copyright", "(C) 1997-2009 The BTK+ Team",
                "website", "http://www.btk.org",
                "comments", "Program to demonstrate BTK+ functions.",
                "logo-icon-name", "btk-about",
                "title", "About BTK+ Code Demos",
                "authors", authors,
		NULL);
  btk_window_set_icon_name (BTK_WINDOW (widget), "btk-about");
  return new_widget_info ("aboutdialog", widget, ASIS);
}

static WidgetInfo *
create_notebook (void)
{
  BtkWidget *widget;

  widget = btk_notebook_new ();

  btk_notebook_append_page (BTK_NOTEBOOK (widget), 
			    btk_label_new ("Notebook"),
			    NULL);
  btk_notebook_append_page (BTK_NOTEBOOK (widget), btk_event_box_new (), NULL);
  btk_notebook_append_page (BTK_NOTEBOOK (widget), btk_event_box_new (), NULL);

  return new_widget_info ("notebook", widget, MEDIUM);
}

static WidgetInfo *
create_progressbar (void)
{
  BtkWidget *vbox;
  BtkWidget *widget;
  BtkWidget *align;

  widget = btk_progress_bar_new ();
  btk_progress_bar_set_fraction (BTK_PROGRESS_BAR (widget), 0.5);

  vbox = btk_vbox_new (FALSE, 3);
  align = btk_alignment_new (0.5, 0.5, 0.0, 0.0);
  btk_container_add (BTK_CONTAINER (align), widget);
  btk_box_pack_start (BTK_BOX (vbox), align, FALSE, FALSE, 0);
  btk_box_pack_start (BTK_BOX (vbox),
		      btk_label_new ("Progress Bar"),
		      FALSE, FALSE, 0);

  return new_widget_info ("progressbar", vbox, SMALL);
}

static WidgetInfo *
create_scrolledwindow (void)
{
  BtkWidget *scrolledwin, *label;

  scrolledwin = btk_scrolled_window_new (NULL, NULL);
  label = btk_label_new ("Scrolled Window");

  btk_scrolled_window_add_with_viewport (BTK_SCROLLED_WINDOW (scrolledwin), 
					 label);

  return new_widget_info ("scrolledwindow", scrolledwin, MEDIUM);
}

static WidgetInfo *
create_spinbutton (void)
{
  BtkWidget *widget;
  BtkWidget *vbox, *align;

  widget = btk_spin_button_new_with_range (0.0, 100.0, 1.0);

  vbox = btk_vbox_new (FALSE, 3);
  align = btk_alignment_new (0.5, 0.5, 0.0, 0.0);
  btk_container_add (BTK_CONTAINER (align), widget);
  btk_box_pack_start (BTK_BOX (vbox), align, FALSE, FALSE, 0);
  btk_box_pack_start (BTK_BOX (vbox),
		      btk_label_new ("Spin Button"),
		      FALSE, FALSE, 0);

  return new_widget_info ("spinbutton", vbox, SMALL);
}

static WidgetInfo *
create_statusbar (void)
{
  WidgetInfo *info;
  BtkWidget *widget;
  BtkWidget *vbox, *align;

  vbox = btk_vbox_new (FALSE, 0);
  align = btk_alignment_new (0.5, 0.5, 0.0, 0.0);
  btk_container_add (BTK_CONTAINER (align), btk_label_new ("Status Bar"));
  btk_box_pack_start (BTK_BOX (vbox),
		      align,
		      TRUE, TRUE, 0);
  widget = btk_statusbar_new ();
  align = btk_alignment_new (0.5, 1.0, 1.0, 0.0);
  btk_container_add (BTK_CONTAINER (align), widget);
  btk_statusbar_set_has_resize_grip (BTK_STATUSBAR (widget), TRUE);
  btk_statusbar_push (BTK_STATUSBAR (widget), 0, "Hold on...");

  btk_box_pack_end (BTK_BOX (vbox), align, FALSE, FALSE, 0);

  info = new_widget_info ("statusbar", vbox, SMALL);
  btk_container_set_border_width (BTK_CONTAINER (info->window), 0);

  return info;
}

static WidgetInfo *
create_scales (void)
{
  BtkWidget *hbox;
  BtkWidget *vbox;

  vbox = btk_vbox_new (FALSE, 3);
  hbox = btk_hbox_new (TRUE, 0);
  btk_box_pack_start (BTK_BOX (hbox),
		      btk_hscale_new_with_range (0.0, 100.0, 1.0),
		      TRUE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (hbox),
		      btk_vscale_new_with_range (0.0, 100.0, 1.0),
		      TRUE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (vbox),
		      g_object_new (BTK_TYPE_LABEL,
				    "label", "Horizontal and Vertical\nScales",
				    "justify", BTK_JUSTIFY_CENTER,
				    NULL),
		      FALSE, FALSE, 0);
  return new_widget_info ("scales", vbox, MEDIUM);}

static WidgetInfo *
create_image (void)
{
  BtkWidget *widget;
  BtkWidget *align, *vbox;

  widget = btk_image_new_from_stock (BTK_STOCK_DIALOG_WARNING, 
				     BTK_ICON_SIZE_DND);

  vbox = btk_vbox_new (FALSE, 3);
  align = btk_alignment_new (0.5, 0.5, 0.0, 0.0);
  btk_container_add (BTK_CONTAINER (align), widget);
  btk_box_pack_start (BTK_BOX (vbox), align, FALSE, FALSE, 0);
  btk_box_pack_start (BTK_BOX (vbox),
		      btk_label_new ("Image"),
		      FALSE, FALSE, 0);

  return new_widget_info ("image", vbox, SMALL);
}

static WidgetInfo *
create_spinner (void)
{
  BtkWidget *widget;
  BtkWidget *align, *vbox;

  widget = btk_spinner_new ();
  btk_widget_set_size_request (widget, 24, 24);

  vbox = btk_vbox_new (FALSE, 3);
  align = btk_alignment_new (0.5, 0.5, 0.0, 0.0);
  btk_container_add (BTK_CONTAINER (align), widget);
  btk_box_pack_start (BTK_BOX (vbox), align, FALSE, FALSE, 0);
  btk_box_pack_start (BTK_BOX (vbox),
		      btk_label_new ("Spinner"),
		      FALSE, FALSE, 0);

  return new_widget_info ("spinner", vbox, SMALL);
}

static WidgetInfo *
create_volume_button (void)
{
  BtkWidget *button, *widget;

  button = btk_volume_button_new ();
  btk_scale_button_set_value (BTK_SCALE_BUTTON (button), 33);
  /* Hack: get the private dock */
  widget = BTK_SCALE_BUTTON (button)->plus_button->parent->parent->parent;
  btk_widget_show_all (widget);
  return new_widget_info ("volumebutton", widget, ASIS);
}

static WidgetInfo *
create_assistant (void)
{
  BtkWidget *widget;
  BtkWidget *page1, *page2;
  WidgetInfo *info;

  widget = btk_assistant_new ();
  btk_window_set_title (BTK_WINDOW (widget), "Assistant");

  page1 = btk_label_new ("Assistant");
  btk_widget_show (page1);
  btk_widget_set_size_request (page1, 300, 140);
  btk_assistant_prepend_page (BTK_ASSISTANT (widget), page1);
  btk_assistant_set_page_title (BTK_ASSISTANT (widget), page1, "Assistant page");
  btk_assistant_set_page_complete (BTK_ASSISTANT (widget), page1, TRUE);

  page2 = btk_label_new (NULL);
  btk_widget_show (page2);
  btk_assistant_append_page (BTK_ASSISTANT (widget), page2);
  btk_assistant_set_page_type (BTK_ASSISTANT (widget), page2, BTK_ASSISTANT_PAGE_CONFIRM);

  info = new_widget_info ("assistant", widget, ASIS);
  info->include_decorations = TRUE;

  return info;
}

GList *
get_all_widgets (void)
{
  GList *retval = NULL;

  retval = g_list_prepend (retval, create_toolpalette ());
  retval = g_list_prepend (retval, create_spinner ());
  retval = g_list_prepend (retval, create_about_dialog ());
  retval = g_list_prepend (retval, create_accel_label ());
  retval = g_list_prepend (retval, create_button ());
  retval = g_list_prepend (retval, create_check_button ());
  retval = g_list_prepend (retval, create_color_button ());
  retval = g_list_prepend (retval, create_combo_box ());
  retval = g_list_prepend (retval, create_combo_box_entry ());
  retval = g_list_prepend (retval, create_entry ());
  retval = g_list_prepend (retval, create_file_button ());
  retval = g_list_prepend (retval, create_font_button ());
  retval = g_list_prepend (retval, create_frame ());
  retval = g_list_prepend (retval, create_icon_view ());
  retval = g_list_prepend (retval, create_image ());
  retval = g_list_prepend (retval, create_label ());
  retval = g_list_prepend (retval, create_link_button ());
  retval = g_list_prepend (retval, create_menubar ());
  retval = g_list_prepend (retval, create_message_dialog ());
  retval = g_list_prepend (retval, create_notebook ());
  retval = g_list_prepend (retval, create_panes ());
  retval = g_list_prepend (retval, create_progressbar ());
  retval = g_list_prepend (retval, create_radio ());
  retval = g_list_prepend (retval, create_scales ());
  retval = g_list_prepend (retval, create_scrolledwindow ());
  retval = g_list_prepend (retval, create_separator ());
  retval = g_list_prepend (retval, create_spinbutton ());
  retval = g_list_prepend (retval, create_statusbar ());
  retval = g_list_prepend (retval, create_text_view ());
  retval = g_list_prepend (retval, create_toggle_button ());
  retval = g_list_prepend (retval, create_toolbar ());
  retval = g_list_prepend (retval, create_tree_view ());
  retval = g_list_prepend (retval, create_window ());
  retval = g_list_prepend (retval, create_colorsel ());
  retval = g_list_prepend (retval, create_filesel ());
  retval = g_list_prepend (retval, create_fontsel ());
  retval = g_list_prepend (retval, create_assistant ());
  retval = g_list_prepend (retval, create_recent_chooser_dialog ());
  retval = g_list_prepend (retval, create_page_setup_dialog ());
  retval = g_list_prepend (retval, create_print_dialog ());
  retval = g_list_prepend (retval, create_volume_button ());

  return retval;
}
