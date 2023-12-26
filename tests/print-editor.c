#include <math.h>
#include <bango/bangobairo.h>
#include <btk/btk.h>

static BtkWidget *main_window;
static char *filename = NULL;
static BtkPageSetup *page_setup = NULL;
static BtkPrintSettings *settings = NULL;
static gboolean file_changed = FALSE;
static BtkTextBuffer *buffer;
static BtkWidget *statusbar;
static GList *active_prints = NULL;

static void
update_title (void)
{
  char *basename;
  char *title;
  
  if (filename == NULL)
    basename = g_strdup ("Untitled");
  else
    basename = g_path_get_basename (filename);

  title = g_strdup_printf ("Simple Editor with printing - %s", basename);
  g_free (basename);
  
  btk_window_set_title (BTK_WINDOW (main_window), title);
  g_free (title);
}

static void
update_statusbar (void)
{
  gchar *msg;
  gint row, col;
  BtkTextIter iter;
  const char *print_str;

  btk_statusbar_pop (BTK_STATUSBAR (statusbar), 0);
  
  btk_text_buffer_get_iter_at_mark (buffer,
                                    &iter,
                                    btk_text_buffer_get_insert (buffer));

  row = btk_text_iter_get_line (&iter);
  col = btk_text_iter_get_line_offset (&iter);

  print_str = "";
  if (active_prints)
    {
      BtkPrintOperation *op = active_prints->data;
      print_str = btk_print_operation_get_status_string (op);
    }
  
  msg = g_strdup_printf ("%d, %d%s %s",
                         row, col,
			 file_changed?" - Modified":"",
			 print_str);

  btk_statusbar_push (BTK_STATUSBAR (statusbar), 0, msg);

  g_free (msg);
}

static void
update_ui (void)
{
  update_title ();
  update_statusbar ();
}

static char *
get_text (void)
{
  BtkTextIter start, end;

  btk_text_buffer_get_start_iter (buffer, &start);
  btk_text_buffer_get_end_iter (buffer, &end);
  return btk_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static void
set_text (const char *text, gsize len)
{
  btk_text_buffer_set_text (buffer, text, len);
  file_changed = FALSE;
  update_ui ();
}

static void
do_new (BtkAction *action)
{
  g_free (filename);
  filename = NULL;
  set_text ("", 0);
}

static void
load_file (const char *open_filename)
{
  BtkWidget *error_dialog;
  char *contents;
  GError *error;
  gsize len;
  
  error_dialog = NULL;
  error = NULL;
  if (g_file_get_contents (open_filename, &contents, &len, &error))
    {
      if (g_utf8_validate (contents, len, NULL))
	{
	  filename = g_strdup (open_filename);
	  set_text (contents, len);
	  g_free (contents);
	}
      else
	{
	  error_dialog = btk_message_dialog_new (BTK_WINDOW (main_window),
						 BTK_DIALOG_DESTROY_WITH_PARENT,
						 BTK_MESSAGE_ERROR,
						 BTK_BUTTONS_CLOSE,
						 "Error loading file %s:\n%s",
						 open_filename,
						 "Not valid utf8");
	}
    }
  else
    {
      error_dialog = btk_message_dialog_new (BTK_WINDOW (main_window),
					     BTK_DIALOG_DESTROY_WITH_PARENT,
					     BTK_MESSAGE_ERROR,
					     BTK_BUTTONS_CLOSE,
					     "Error loading file %s:\n%s",
					     open_filename,
					     error->message);
      
      g_error_free (error);
    }
  if (error_dialog)
    {
      g_signal_connect (error_dialog, "response", G_CALLBACK (btk_widget_destroy), NULL);
      btk_widget_show (error_dialog);
    }
}

static void
do_open (BtkAction *action)
{
  BtkWidget *dialog;
  gint response;
  char *open_filename;
  
  dialog = btk_file_chooser_dialog_new ("Select file",
					BTK_WINDOW (main_window),
					BTK_FILE_CHOOSER_ACTION_OPEN,
					BTK_STOCK_CANCEL, BTK_RESPONSE_CANCEL,
					BTK_STOCK_OPEN, BTK_RESPONSE_OK,
					NULL);
  btk_dialog_set_default_response (BTK_DIALOG (dialog), BTK_RESPONSE_OK);
  response = btk_dialog_run (BTK_DIALOG (dialog));

  if (response == BTK_RESPONSE_OK)
    {
      open_filename = btk_file_chooser_get_filename (BTK_FILE_CHOOSER (dialog));
      load_file (open_filename);
      g_free (open_filename);
    }

  btk_widget_destroy (dialog);
}

static void
save_file (const char *save_filename)
{
  char *text = get_text ();
  BtkWidget *error_dialog;
  GError *error;

  error = NULL;
  if (g_file_set_contents (save_filename,
			   text, -1, &error))
    {
      if (save_filename != filename)
	{
	  g_free (filename);
	  filename = g_strdup (save_filename);
	}
      file_changed = FALSE;
      update_ui ();
    }
  else
    {
      error_dialog = btk_message_dialog_new (BTK_WINDOW (main_window),
					     BTK_DIALOG_DESTROY_WITH_PARENT,
					     BTK_MESSAGE_ERROR,
					     BTK_BUTTONS_CLOSE,
					     "Error saving to file %s:\n%s",
					     filename,
					     error->message);
      
      g_signal_connect (error_dialog, "response", G_CALLBACK (btk_widget_destroy), NULL);
      btk_widget_show (error_dialog);
      
      g_error_free (error);
    }
}

static void
do_save_as (BtkAction *action)
{
  BtkWidget *dialog;
  gint response;
  char *save_filename;
  
  dialog = btk_file_chooser_dialog_new ("Select file",
					BTK_WINDOW (main_window),
					BTK_FILE_CHOOSER_ACTION_SAVE,
					BTK_STOCK_CANCEL, BTK_RESPONSE_CANCEL,
					BTK_STOCK_SAVE, BTK_RESPONSE_OK,
					NULL);
  btk_dialog_set_default_response (BTK_DIALOG (dialog), BTK_RESPONSE_OK);
  response = btk_dialog_run (BTK_DIALOG (dialog));

  if (response == BTK_RESPONSE_OK)
    {
      save_filename = btk_file_chooser_get_filename (BTK_FILE_CHOOSER (dialog));
      save_file (save_filename);
      g_free (save_filename);
    }
  
  btk_widget_destroy (dialog);
}

static void
do_save (BtkAction *action)
{
  if (filename == NULL)
    do_save_as (action);
  else
    save_file (filename);
}

typedef struct {
  char *text;
  BangoLayout *layout;
  GList *page_breaks;
  BtkWidget *font_button;
  char *font;
} PrintData;

static void
begin_print (BtkPrintOperation *operation,
	     BtkPrintContext *context,
	     PrintData *print_data)
{
  BangoFontDescription *desc;
  BangoLayoutLine *layout_line;
  double width, height;
  double page_height;
  GList *page_breaks;
  int num_lines;
  int line;

  width = btk_print_context_get_width (context);
  height = btk_print_context_get_height (context);

  print_data->layout = btk_print_context_create_bango_layout (context);

  desc = bango_font_description_from_string (print_data->font);
  bango_layout_set_font_description (print_data->layout, desc);
  bango_font_description_free (desc);

  bango_layout_set_width (print_data->layout, width * BANGO_SCALE);
  
  bango_layout_set_text (print_data->layout, print_data->text, -1);

  num_lines = bango_layout_get_line_count (print_data->layout);

  page_breaks = NULL;
  page_height = 0;

  for (line = 0; line < num_lines; line++)
    {
      BangoRectangle ink_rect, logical_rect;
      double line_height;
      
      layout_line = bango_layout_get_line (print_data->layout, line);
      bango_layout_line_get_extents (layout_line, &ink_rect, &logical_rect);

      line_height = logical_rect.height / 1024.0;

      if (page_height + line_height > height)
	{
	  page_breaks = g_list_prepend (page_breaks, GINT_TO_POINTER (line));
	  page_height = 0;
	}

      page_height += line_height;
    }

  page_breaks = g_list_reverse (page_breaks);
  btk_print_operation_set_n_pages (operation, g_list_length (page_breaks) + 1);
  
  print_data->page_breaks = page_breaks;
}

static void
draw_page (BtkPrintOperation *operation,
	   BtkPrintContext *context,
	   int page_nr,
	   PrintData *print_data)
{
  bairo_t *cr;
  GList *pagebreak;
  int start, end, i;
  BangoLayoutIter *iter;
  double start_pos;

  if (page_nr == 0)
    start = 0;
  else
    {
      pagebreak = g_list_nth (print_data->page_breaks, page_nr - 1);
      start = GPOINTER_TO_INT (pagebreak->data);
    }

  pagebreak = g_list_nth (print_data->page_breaks, page_nr);
  if (pagebreak == NULL)
    end = bango_layout_get_line_count (print_data->layout);
  else
    end = GPOINTER_TO_INT (pagebreak->data);
    
  cr = btk_print_context_get_bairo_context (context);

  bairo_set_source_rgb (cr, 0, 0, 0);
  
  i = 0;
  start_pos = 0;
  iter = bango_layout_get_iter (print_data->layout);
  do
    {
      BangoRectangle   logical_rect;
      BangoLayoutLine *line;
      int              baseline;

      if (i >= start)
	{
	  line = bango_layout_iter_get_line (iter);

	  bango_layout_iter_get_line_extents (iter, NULL, &logical_rect);
	  baseline = bango_layout_iter_get_baseline (iter);
	  
	  if (i == start)
	    start_pos = logical_rect.y / 1024.0;
	  
	  bairo_move_to (cr, logical_rect.x / 1024.0, baseline / 1024.0 - start_pos);
	  
	  bango_bairo_show_layout_line  (cr, line);
	}
      i++;
    }
  while (i < end &&
	 bango_layout_iter_next_line (iter));

  bango_layout_iter_free (iter);
}

static void
do_page_setup (BtkAction *action)
{
  BtkPageSetup *new_page_setup;

  new_page_setup = btk_print_run_page_setup_dialog (BTK_WINDOW (main_window),
						    page_setup, settings);

  if (page_setup)
    g_object_unref (page_setup);
  
  page_setup = new_page_setup;
}

static void
status_changed_cb (BtkPrintOperation *op,
		   gpointer user_data)
{
  if (btk_print_operation_is_finished (op))
    {
      active_prints = g_list_remove (active_prints, op);
      g_object_unref (op);
    }
  update_statusbar ();
}

static BtkWidget *
create_custom_widget (BtkPrintOperation *operation,
		      PrintData *data)
{
  BtkWidget *vbox, *hbox, *font, *label;

  btk_print_operation_set_custom_tab_label (operation, "Other");
  vbox = btk_vbox_new (FALSE, 0);
  btk_container_set_border_width (BTK_CONTAINER (vbox), 12);

  hbox = btk_hbox_new (FALSE, 8);
  btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  btk_widget_show (hbox);

  label = btk_label_new ("Font:");
  btk_box_pack_start (BTK_BOX (hbox), label, FALSE, FALSE, 0);
  btk_widget_show (label);
  
  font = btk_font_button_new_with_font  (data->font);
  btk_box_pack_start (BTK_BOX (hbox), font, FALSE, FALSE, 0);
  btk_widget_show (font);
  data->font_button = font;

  return vbox;
}

static void
custom_widget_apply (BtkPrintOperation *operation,
		     BtkWidget *widget,
		     PrintData *data)
{
  const char *selected_font;
  selected_font = btk_font_button_get_font_name  (BTK_FONT_BUTTON (data->font_button));
  g_free (data->font);
  data->font = g_strdup (selected_font);
}

typedef struct 
{
  BtkPrintOperation *op;
  BtkPrintOperationPreview *preview;
  BtkWidget         *spin;
  BtkWidget         *area;
  gint               page;
  BtkPrintContext   *context;
  PrintData *data;
  gdouble dpi_x, dpi_y;
} PreviewOp;

static gboolean
preview_expose (BtkWidget      *widget,
		BdkEventExpose *event,
		gpointer        data)
{
  PreviewOp *pop = data;
  bairo_t *cr;

  cr = bdk_bairo_create (pop->area->window);
  btk_print_context_set_bairo_context (pop->context, cr, pop->dpi_x, pop->dpi_y);
  bairo_destroy (cr);

  bdk_window_clear (pop->area->window);
  btk_print_operation_preview_render_page (pop->preview,
					   pop->page - 1);

  return TRUE;
}

static void
preview_ready (BtkPrintOperationPreview *preview,
	       BtkPrintContext          *context,
	       gpointer                  data)
{
  PreviewOp *pop = data;
  gint n_pages;

  g_object_get (pop->op, "n-pages", &n_pages, NULL);

  btk_spin_button_set_range (BTK_SPIN_BUTTON (pop->spin), 
			     1.0, n_pages);

  g_signal_connect (pop->area, "expose_event",
		    G_CALLBACK (preview_expose),
		    pop);

  btk_widget_queue_draw (pop->area);
}

static void
preview_got_page_size (BtkPrintOperationPreview *preview, 
		       BtkPrintContext          *context,
		       BtkPageSetup             *page_setup,
		       gpointer                  data)
{
  PreviewOp *pop = data;
  BtkPaperSize *paper_size;
  double w, h;
  bairo_t *cr;
  gdouble dpi_x, dpi_y;

  paper_size = btk_page_setup_get_paper_size (page_setup);

  w = btk_paper_size_get_width (paper_size, BTK_UNIT_INCH);
  h = btk_paper_size_get_height (paper_size, BTK_UNIT_INCH);

  cr = bdk_bairo_create (pop->area->window);

  dpi_x = pop->area->allocation.width/w;
  dpi_y = pop->area->allocation.height/h;

  if (fabs (dpi_x - pop->dpi_x) > 0.001 ||
      fabs (dpi_y - pop->dpi_y) > 0.001)
    {
      btk_print_context_set_bairo_context (context, cr, dpi_x, dpi_y);
      pop->dpi_x = dpi_x;
      pop->dpi_y = dpi_y;
    }

  bango_bairo_update_layout (cr, pop->data->layout);
  bairo_destroy (cr);
}

static void
update_page (BtkSpinButton *widget,
	     gpointer       data)
{
  PreviewOp *pop = data;

  pop->page = btk_spin_button_get_value_as_int (widget);
  btk_widget_queue_draw (pop->area);
}

static void
preview_destroy (BtkWindow *window, 
		 PreviewOp *pop)
{
  btk_print_operation_preview_end_preview (pop->preview);
  g_object_unref (pop->op);

  g_free (pop);
}

static gboolean 
preview_cb (BtkPrintOperation        *op,
	    BtkPrintOperationPreview *preview,
	    BtkPrintContext          *context,
	    BtkWindow                *parent,
	    gpointer                  data)
{
  BtkPrintSettings *settings;
  BtkWidget *window, *close, *page, *hbox, *vbox, *da;
  gdouble width, height;
  bairo_t *cr;
  PreviewOp *pop;
  PrintData *print_data = data;

  pop = g_new0 (PreviewOp, 1);

  pop->data = print_data;
  settings = btk_print_operation_get_print_settings (op);

  width = 200;
  height = 300;
  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_transient_for (BTK_WINDOW (window), 
				BTK_WINDOW (main_window));
  vbox = btk_vbox_new (FALSE, 0);
  btk_container_add (BTK_CONTAINER (window), vbox);
  hbox = btk_hbox_new (FALSE, 0);
  btk_box_pack_start (BTK_BOX (vbox), hbox,
		      FALSE, FALSE, 0);
  page = btk_spin_button_new_with_range (1, 100, 1);
  btk_box_pack_start (BTK_BOX (hbox), page, FALSE, FALSE, 0);
  
  close = btk_button_new_from_stock (BTK_STOCK_CLOSE);
  btk_box_pack_start (BTK_BOX (hbox), close, FALSE, FALSE, 0);

  da = btk_drawing_area_new ();
  btk_widget_set_size_request (BTK_WIDGET (da), width, height);
  btk_box_pack_start (BTK_BOX (vbox), da, TRUE, TRUE, 0);

  btk_widget_realize (da);
  
  cr = bdk_bairo_create (da->window);

  /* TODO: What dpi to use here? This will be used for pagination.. */
  btk_print_context_set_bairo_context (context, cr, 72, 72);
  bairo_destroy (cr);
  
  pop->op = g_object_ref (op);
  pop->preview = preview;
  pop->spin = page;
  pop->area = da;
  pop->page = 1;
  pop->context = context;

  g_signal_connect (page, "value-changed", 
		    G_CALLBACK (update_page), pop);
  g_signal_connect_swapped (close, "clicked", 
			    G_CALLBACK (btk_widget_destroy), window);

  g_signal_connect (preview, "ready",
		    G_CALLBACK (preview_ready), pop);
  g_signal_connect (preview, "got-page-size",
		    G_CALLBACK (preview_got_page_size), pop);

  g_signal_connect (window, "destroy", 
                    G_CALLBACK (preview_destroy), pop);
                            
  btk_widget_show_all (window);
  
  return TRUE;
}

static void
print_done (BtkPrintOperation *op,
	    BtkPrintOperationResult res,
	    PrintData *print_data)
{
  GError *error = NULL;

  if (res == BTK_PRINT_OPERATION_RESULT_ERROR)
    {

      BtkWidget *error_dialog;
      
      btk_print_operation_get_error (op, &error);
      
      error_dialog = btk_message_dialog_new (BTK_WINDOW (main_window),
					     BTK_DIALOG_DESTROY_WITH_PARENT,
					     BTK_MESSAGE_ERROR,
					     BTK_BUTTONS_CLOSE,
					     "Error printing file:\n%s",
					     error ? error->message : "no details");
      g_signal_connect (error_dialog, "response", G_CALLBACK (btk_widget_destroy), NULL);
      btk_widget_show (error_dialog);
    }
  else if (res == BTK_PRINT_OPERATION_RESULT_APPLY)
    {
      if (settings != NULL)
	g_object_unref (settings);
      settings = g_object_ref (btk_print_operation_get_print_settings (op));
    }

  g_free (print_data->text);
  g_free (print_data->font);
  g_free (print_data);
  
  if (!btk_print_operation_is_finished (op))
    {
      g_object_ref (op);
      active_prints = g_list_append (active_prints, op);
      update_statusbar ();
      
      /* This ref is unref:ed when we get the final state change */
      g_signal_connect (op, "status_changed",
			G_CALLBACK (status_changed_cb), NULL);
    }
}

static void
end_print (BtkPrintOperation *op, BtkPrintContext *context, PrintData *print_data)
{
  g_list_free (print_data->page_breaks);
  print_data->page_breaks = NULL;
  g_object_unref (print_data->layout);
  print_data->layout = NULL;
}

static void
do_print_or_preview (BtkAction *action, BtkPrintOperationAction print_action)
{
  BtkPrintOperation *print;
  PrintData *print_data;

  print_data = g_new0 (PrintData, 1);

  print_data->text = get_text ();
  print_data->font = g_strdup ("Sans 12");

  print = btk_print_operation_new ();

  btk_print_operation_set_track_print_status (print, TRUE);
  
  if (settings != NULL)
    btk_print_operation_set_print_settings (print, settings);

  if (page_setup != NULL)
    btk_print_operation_set_default_page_setup (print, page_setup);
  
  g_signal_connect (print, "begin_print", G_CALLBACK (begin_print), print_data);
  g_signal_connect (print, "end-print", G_CALLBACK (end_print), print_data);
  g_signal_connect (print, "draw_page", G_CALLBACK (draw_page), print_data);
  g_signal_connect (print, "create_custom_widget", G_CALLBACK (create_custom_widget), print_data);
  g_signal_connect (print, "custom_widget_apply", G_CALLBACK (custom_widget_apply), print_data);
  g_signal_connect (print, "preview", G_CALLBACK (preview_cb), print_data);

  g_signal_connect (print, "done", G_CALLBACK (print_done), print_data);

  btk_print_operation_set_export_filename (print, "test.pdf");

#if 0
  btk_print_operation_set_allow_async (print, TRUE);
#endif
  btk_print_operation_run (print, print_action, BTK_WINDOW (main_window), NULL);

  g_object_unref (print);
}

static void
do_print (BtkAction *action)
{
  do_print_or_preview (action, BTK_PRINT_OPERATION_ACTION_PRINT_DIALOG);
}

static void
do_preview (BtkAction *action)
{
  do_print_or_preview (action, BTK_PRINT_OPERATION_ACTION_PREVIEW);
}

static void
do_about (BtkAction *action)
{
  const gchar *authors[] = {
    "Alexander Larsson",
    NULL
  };
  btk_show_about_dialog (BTK_WINDOW (main_window),
			 "name", "print test editor",
			 "version", "0.1",
			 "copyright", "(C) Red Hat, Inc",
			 "comments", "Program to demonstrate BTK+ printing.",
			 "authors", authors,
			 NULL);
}

static void
do_quit (BtkAction *action)
{
  btk_main_quit ();
}

static BtkActionEntry entries[] = {
  { "FileMenu", NULL, "_File" },               /* name, stock id, label */
  { "HelpMenu", NULL, "_Help" },               /* name, stock id, label */
  { "New", BTK_STOCK_NEW,                      /* name, stock id */
    "_New", "<control>N",                      /* label, accelerator */
    "Create a new file",                       /* tooltip */ 
    G_CALLBACK (do_new) },      
  { "Open", BTK_STOCK_OPEN,                    /* name, stock id */
    "_Open","<control>O",                      /* label, accelerator */     
    "Open a file",                             /* tooltip */
    G_CALLBACK (do_open) }, 
  { "Save", BTK_STOCK_SAVE,                    /* name, stock id */
    "_Save","<control>S",                      /* label, accelerator */     
    "Save current file",                       /* tooltip */
    G_CALLBACK (do_save) },
  { "SaveAs", BTK_STOCK_SAVE,                  /* name, stock id */
    "Save _As...", NULL,                       /* label, accelerator */     
    "Save to a file",                          /* tooltip */
    G_CALLBACK (do_save_as) },
  { "Quit", BTK_STOCK_QUIT,                    /* name, stock id */
    "_Quit", "<control>Q",                     /* label, accelerator */     
    "Quit",                                    /* tooltip */
    G_CALLBACK (do_quit) },
  { "About", NULL,                             /* name, stock id */
    "_About", "<control>A",                    /* label, accelerator */     
    "About",                                   /* tooltip */  
    G_CALLBACK (do_about) },
  { "PageSetup", NULL,                         /* name, stock id */
    "Page _Setup", NULL,                       /* label, accelerator */     
    "Set up the page",                         /* tooltip */
    G_CALLBACK (do_page_setup) },
  { "Preview", NULL,                           /* name, stock id */
    "Print Preview", NULL,                     /* label, accelerator */     
    "Preview the printed document",            /* tooltip */
    G_CALLBACK (do_preview) },
  { "Print", BTK_STOCK_PRINT,                  /* name, stock id */
     NULL, NULL,                               /* label, accelerator */     
    "Print the document",                      /* tooltip */
    G_CALLBACK (do_print) }
};
static guint n_entries = G_N_ELEMENTS (entries);

static const gchar *ui_info = 
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu action='FileMenu'>"
"      <menuitem action='New'/>"
"      <menuitem action='Open'/>"
"      <menuitem action='Save'/>"
"      <menuitem action='SaveAs'/>"
"      <menuitem action='PageSetup'/>"
"      <menuitem action='Preview'/>"
"      <menuitem action='Print'/>"
"      <separator/>"
"      <menuitem action='Quit'/>"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <menuitem action='About'/>"
"    </menu>"
"  </menubar>"
"</ui>";

static void
buffer_changed_callback (BtkTextBuffer *buffer)
{
  file_changed = TRUE;
  update_statusbar ();
}

static void
mark_set_callback (BtkTextBuffer     *buffer,
                   const BtkTextIter *new_location,
                   BtkTextMark       *mark,
                   gpointer           data)
{
  update_statusbar ();
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

static void
create_window (void)
{
  BtkWidget *bar;
  BtkWidget *table;
  BtkWidget *contents;
  BtkUIManager *ui;
  BtkWidget *sw;
  BtkActionGroup *actions;
  GError *error;
  BtkWindowGroup *group;
  
  main_window = btk_window_new (BTK_WINDOW_TOPLEVEL);

  group = btk_window_group_new ();
  btk_window_group_add_window (group, BTK_WINDOW (main_window));
  g_object_unref (group);

  btk_window_set_default_size (BTK_WINDOW (main_window),
			       400, 600);
  
  g_signal_connect (main_window, "delete-event",
		    G_CALLBACK (btk_main_quit), NULL);
  
  actions = btk_action_group_new ("Actions");
  btk_action_group_add_actions (actions, entries, n_entries, NULL);
  
  ui = btk_ui_manager_new ();
  btk_ui_manager_insert_action_group (ui, actions, 0);
  btk_window_add_accel_group (BTK_WINDOW (main_window), 
			      btk_ui_manager_get_accel_group (ui));
  btk_container_set_border_width (BTK_CONTAINER (main_window), 0);

  error = NULL;
  if (!btk_ui_manager_add_ui_from_string (ui, ui_info, -1, &error))
    {
      g_message ("building menus failed: %s", error->message);
      g_error_free (error);
    }

  table = btk_table_new (1, 3, FALSE);
  btk_container_add (BTK_CONTAINER (main_window), table);

  bar = btk_ui_manager_get_widget (ui, "/MenuBar");
  btk_widget_show (bar);
  btk_table_attach (BTK_TABLE (table),
		    bar, 
		    /* X direction */          /* Y direction */
		    0, 1,                      0, 1,
		    BTK_EXPAND | BTK_FILL,     0,
		    0,                         0);

  /* Create document  */
  sw = btk_scrolled_window_new (NULL, NULL);

  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (sw),
				  BTK_POLICY_AUTOMATIC,
				  BTK_POLICY_AUTOMATIC);
  
  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (sw),
				       BTK_SHADOW_IN);
  
  btk_table_attach (BTK_TABLE (table),
		    sw,
		    /* X direction */       /* Y direction */
		    0, 1,                   1, 2,
		    BTK_EXPAND | BTK_FILL,  BTK_EXPAND | BTK_FILL,
		    0,                      0);
  
  contents = btk_text_view_new ();
  btk_widget_grab_focus (contents);
      
  btk_container_add (BTK_CONTAINER (sw),
		     contents);
  
  /* Create statusbar */
  
  statusbar = btk_statusbar_new ();
  btk_table_attach (BTK_TABLE (table),
		    statusbar,
		    /* X direction */       /* Y direction */
		    0, 1,                   2, 3,
		    BTK_EXPAND | BTK_FILL,  0,
		    0,                      0);

  /* Show text widget info in the statusbar */
  buffer = btk_text_view_get_buffer (BTK_TEXT_VIEW (contents));
  
  g_signal_connect_object (buffer,
			   "changed",
			   G_CALLBACK (buffer_changed_callback),
			   NULL,
			   0);
  
  g_signal_connect_object (buffer,
			   "mark_set", /* cursor moved */
			   G_CALLBACK (mark_set_callback),
			   NULL,
			   0);
  
  g_signal_connect_object (main_window, 
			   "window_state_event", 
			   G_CALLBACK (update_resize_grip),
			   statusbar,
			   0);
  
  update_ui ();
  
  btk_widget_show_all (main_window);
}

int
main (int argc, char **argv)
{
  GError *error = NULL;

  g_set_application_name ("Print editor");
  btk_init (&argc, &argv);

  settings = btk_print_settings_new_from_file ("print-settings.ini", &error);
  if (error) {
    g_print ("Failed to load print settings: %s\n", error->message);
    g_clear_error (&error);

    settings = btk_print_settings_new ();
  }
  g_assert (settings != NULL);

  page_setup = btk_page_setup_new_from_file ("page-setup.ini", &error);
  if (error) {
    g_print ("Failed to load page setup: %s\n", error->message);
    g_clear_error (&error);
  }

  create_window ();

  if (argc == 2)
    load_file (argv[1]);
  
  btk_main ();

  if (!btk_print_settings_to_file (settings, "print-settings.ini", &error)) {
    g_print ("Failed to save print settings: %s\n", error->message);
    g_clear_error (&error);
  }
  if (page_setup &&
      !btk_page_setup_to_file (page_setup, "page-setup.ini", &error)) {
    g_print ("Failed to save page setup: %s\n", error->message);
    g_clear_error (&error);
  }

  return 0;
}
