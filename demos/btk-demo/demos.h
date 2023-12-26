typedef	BtkWidget *(*GDoDemoFunc) (BtkWidget *do_widget);

typedef struct _Demo Demo;

struct _Demo 
{
  bchar *title;
  bchar *filename;
  GDoDemoFunc func;
  Demo *children;
};

BtkWidget *do_appwindow (BtkWidget *do_widget);
BtkWidget *do_assistant (BtkWidget *do_widget);
BtkWidget *do_builder (BtkWidget *do_widget);
BtkWidget *do_button_box (BtkWidget *do_widget);
BtkWidget *do_changedisplay (BtkWidget *do_widget);
BtkWidget *do_clipboard (BtkWidget *do_widget);
BtkWidget *do_colorsel (BtkWidget *do_widget);
BtkWidget *do_combobox (BtkWidget *do_widget);
BtkWidget *do_dialog (BtkWidget *do_widget);
BtkWidget *do_drawingarea (BtkWidget *do_widget);
BtkWidget *do_editable_cells (BtkWidget *do_widget);
BtkWidget *do_entry_buffer (BtkWidget *do_widget);
BtkWidget *do_entry_completion (BtkWidget *do_widget);
BtkWidget *do_expander (BtkWidget *do_widget);
BtkWidget *do_hypertext (BtkWidget *do_widget);
BtkWidget *do_iconview (BtkWidget *do_widget);
BtkWidget *do_iconview_edit (BtkWidget *do_widget);
BtkWidget *do_images (BtkWidget *do_widget);
BtkWidget *do_infobar (BtkWidget *do_widget);
BtkWidget *do_links (BtkWidget *do_widget);
BtkWidget *do_list_store (BtkWidget *do_widget);
BtkWidget *do_menus (BtkWidget *do_widget);
BtkWidget *do_offscreen_window (BtkWidget *do_widget);
BtkWidget *do_offscreen_window2 (BtkWidget *do_widget);
BtkWidget *do_panes (BtkWidget *do_widget);
BtkWidget *do_pickers (BtkWidget *do_widget);
BtkWidget *do_pixbufs (BtkWidget *do_widget);
BtkWidget *do_printing (BtkWidget *do_widget);
BtkWidget *do_rotated_text (BtkWidget *do_widget);
BtkWidget *do_search_entry (BtkWidget *do_widget);
BtkWidget *do_sizegroup (BtkWidget *do_widget);
BtkWidget *do_spinner (BtkWidget *do_widget);
BtkWidget *do_stock_browser (BtkWidget *do_widget);
BtkWidget *do_textview (BtkWidget *do_widget);
BtkWidget *do_textscroll (BtkWidget *do_widget);
BtkWidget *do_toolpalette (BtkWidget *do_widget);
BtkWidget *do_tree_store (BtkWidget *do_widget);
BtkWidget *do_ui_manager (BtkWidget *do_widget);

Demo child0[] = {
  { "Editable Cells", "editable_cells.c", do_editable_cells, NULL },
  { "List Store", "list_store.c", do_list_store, NULL },
  { "Tree Store", "tree_store.c", do_tree_store, NULL },
  { NULL } 
};

Demo child1[] = {
  { "Entry Buffer", "entry_buffer.c", do_entry_buffer, NULL },
  { "Entry Completion", "entry_completion.c", do_entry_completion, NULL },
  { "Search Entry", "search_entry.c", do_search_entry, NULL },
  { NULL } 
};

Demo child2[] = {
  { "Hypertext", "hypertext.c", do_hypertext, NULL },
  { "Multiple Views", "textview.c", do_textview, NULL },
  { "Automatic scrolling", "textscroll.c", do_textscroll, NULL },
  { NULL } 
};

Demo child3[] = {
  { "Icon View Basics", "iconview.c", do_iconview, NULL },
  { "Editing and Drag-and-Drop", "iconview_edit.c", do_iconview_edit, NULL },
  { NULL } 
};

Demo child4[] = {
  { "Rotated button", "offscreen_window.c", do_offscreen_window, NULL },
  { "Effects", "offscreen_window2.c", do_offscreen_window2, NULL },
  { NULL } 
};

Demo testbtk_demos[] = {
  { "Application main window", "appwindow.c", do_appwindow, NULL }, 
  { "Assistant", "assistant.c", do_assistant, NULL }, 
  { "Builder", "builder.c", do_builder, NULL }, 
  { "Button Boxes", "button_box.c", do_button_box, NULL }, 
  { "Change Display", "changedisplay.c", do_changedisplay, NULL }, 
  { "Clipboard", "clipboard.c", do_clipboard, NULL }, 
  { "Color Selector", "colorsel.c", do_colorsel, NULL }, 
  { "Combo boxes", "combobox.c", do_combobox, NULL }, 
  { "Dialog and Message Boxes", "dialog.c", do_dialog, NULL }, 
  { "Drawing Area", "drawingarea.c", do_drawingarea, NULL }, 
  { "Entry", NULL, NULL, child1 }, 
  { "Expander", "expander.c", do_expander, NULL }, 
  { "Icon View", NULL, NULL, child3 }, 
  { "Images", "images.c", do_images, NULL }, 
  { "Info bar", "infobar.c", do_infobar, NULL }, 
  { "Links", "links.c", do_links, NULL }, 
  { "Menus", "menus.c", do_menus, NULL }, 
  { "Offscreen windows", NULL, NULL, child4 }, 
  { "Paned Widgets", "panes.c", do_panes, NULL }, 
  { "Pickers", "pickers.c", do_pickers, NULL }, 
  { "Pixbufs", "pixbufs.c", do_pixbufs, NULL }, 
  { "Printing", "printing.c", do_printing, NULL }, 
  { "Rotated Text", "rotated_text.c", do_rotated_text, NULL }, 
  { "Size Groups", "sizegroup.c", do_sizegroup, NULL }, 
  { "Spinner", "spinner.c", do_spinner, NULL }, 
  { "Stock Item and Icon Browser", "stock_browser.c", do_stock_browser, NULL }, 
  { "Text Widget", NULL, NULL, child2 }, 
  { "Tool Palette", "toolpalette.c", do_toolpalette, NULL }, 
  { "Tree View", NULL, NULL, child0 }, 
  { "UI Manager", "ui_manager.c", do_ui_manager, NULL },
  { NULL } 
};
