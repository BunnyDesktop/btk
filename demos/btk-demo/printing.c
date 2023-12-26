/* Printing
 *
 * BtkPrintOperation offers a simple API to support printing
 * in a cross-platform way.
 *
 */

#include <math.h>
#include <btk/btk.h>
#include "demo-common.h"

/* In points */
#define HEADER_HEIGHT (10*72/25.4)
#define HEADER_GAP (3*72/25.4)

typedef struct
{
  gchar *filename;
  gdouble font_size;

  gint lines_per_page;
  gchar **lines;
  gint num_lines;
  gint num_pages;
} PrintData;

static void
begin_print (BtkPrintOperation *operation,
	     BtkPrintContext   *context,
	     gpointer           user_data)
{
  PrintData *data = (PrintData *)user_data;
  char *contents;
  int i;
  double height;

  height = btk_print_context_get_height (context) - HEADER_HEIGHT - HEADER_GAP;

  data->lines_per_page = floor (height / data->font_size);

  g_file_get_contents (data->filename, &contents, NULL, NULL);

  data->lines = g_strsplit (contents, "\n", 0);
  g_free (contents);

  i = 0;
  while (data->lines[i] != NULL)
    i++;

  data->num_lines = i;
  data->num_pages = (data->num_lines - 1) / data->lines_per_page + 1;

  btk_print_operation_set_n_pages (operation, data->num_pages);
}

static void
draw_page (BtkPrintOperation *operation,
	   BtkPrintContext   *context,
	   gint               page_nr,
	   gpointer           user_data)
{
  PrintData *data = (PrintData *)user_data;
  bairo_t *cr;
  BangoLayout *layout;
  gint text_width, text_height;
  gdouble width;
  gint line, i;
  BangoFontDescription *desc;
  gchar *page_str;

  cr = btk_print_context_get_bairo_context (context);
  width = btk_print_context_get_width (context);

  bairo_rectangle (cr, 0, 0, width, HEADER_HEIGHT);

  bairo_set_source_rgb (cr, 0.8, 0.8, 0.8);
  bairo_fill_preserve (cr);

  bairo_set_source_rgb (cr, 0, 0, 0);
  bairo_set_line_width (cr, 1);
  bairo_stroke (cr);

  layout = btk_print_context_create_bango_layout (context);

  desc = bango_font_description_from_string ("sans 14");
  bango_layout_set_font_description (layout, desc);
  bango_font_description_free (desc);

  bango_layout_set_text (layout, data->filename, -1);
  bango_layout_get_pixel_size (layout, &text_width, &text_height);

  if (text_width > width)
    {
      bango_layout_set_width (layout, width);
      bango_layout_set_ellipsize (layout, BANGO_ELLIPSIZE_START);
      bango_layout_get_pixel_size (layout, &text_width, &text_height);
    }

  bairo_move_to (cr, (width - text_width) / 2,  (HEADER_HEIGHT - text_height) / 2);
  bango_bairo_show_layout (cr, layout);

  page_str = g_strdup_printf ("%d/%d", page_nr + 1, data->num_pages);
  bango_layout_set_text (layout, page_str, -1);
  g_free (page_str);

  bango_layout_set_width (layout, -1);
  bango_layout_get_pixel_size (layout, &text_width, &text_height);
  bairo_move_to (cr, width - text_width - 4, (HEADER_HEIGHT - text_height) / 2);
  bango_bairo_show_layout (cr, layout);

  g_object_unref (layout);

  layout = btk_print_context_create_bango_layout (context);

  desc = bango_font_description_from_string ("monospace");
  bango_font_description_set_size (desc, data->font_size * BANGO_SCALE);
  bango_layout_set_font_description (layout, desc);
  bango_font_description_free (desc);

  bairo_move_to (cr, 0, HEADER_HEIGHT + HEADER_GAP);
  line = page_nr * data->lines_per_page;
  for (i = 0; i < data->lines_per_page && line < data->num_lines; i++)
    {
      bango_layout_set_text (layout, data->lines[line], -1);
      bango_bairo_show_layout (cr, layout);
      bairo_rel_move_to (cr, 0, data->font_size);
      line++;
    }

  g_object_unref (layout);
}

static void
end_print (BtkPrintOperation *operation,
	   BtkPrintContext   *context,
	   gpointer           user_data)
{
  PrintData *data = (PrintData *)user_data;

  g_free (data->filename);
  g_strfreev (data->lines);
  g_free (data);
}


BtkWidget *
do_printing (BtkWidget *do_widget)
{
  BtkPrintOperation *operation;
  BtkPrintSettings *settings;
  PrintData *data;
  gchar *uri, *ext;
  const gchar *dir;
  GError *error = NULL;

  operation = btk_print_operation_new ();
  data = g_new0 (PrintData, 1);
  data->filename = demo_find_file ("printing.c", NULL);
  data->font_size = 12.0;

  g_signal_connect (G_OBJECT (operation), "begin-print",
		    G_CALLBACK (begin_print), data);
  g_signal_connect (G_OBJECT (operation), "draw-page",
		    G_CALLBACK (draw_page), data);
  g_signal_connect (G_OBJECT (operation), "end-print",
		    G_CALLBACK (end_print), data);

  btk_print_operation_set_use_full_page (operation, FALSE);
  btk_print_operation_set_unit (operation, BTK_UNIT_POINTS);
  btk_print_operation_set_embed_page_setup (operation, TRUE);

  settings = btk_print_settings_new ();
  dir = g_get_user_special_dir (G_USER_DIRECTORY_DOCUMENTS);
  if (dir == NULL)
    dir = g_get_home_dir ();
  if (g_strcmp0 (btk_print_settings_get (settings, BTK_PRINT_SETTINGS_OUTPUT_FILE_FORMAT), "ps") == 0)
    ext = ".ps";
  else if (g_strcmp0 (btk_print_settings_get (settings, BTK_PRINT_SETTINGS_OUTPUT_FILE_FORMAT), "svg") == 0)
    ext = ".svg";
  else
    ext = ".pdf";

  uri = g_strconcat ("file://", dir, "/", "btk-demo", ext, NULL);
  btk_print_settings_set (settings, BTK_PRINT_SETTINGS_OUTPUT_URI, uri);
  btk_print_operation_set_print_settings (operation, settings);

  btk_print_operation_run (operation, BTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, BTK_WINDOW (do_widget), &error);

  g_object_unref (operation);
  g_object_unref (settings);
  g_free (uri);

  if (error)
    {
      BtkWidget *dialog;

      dialog = btk_message_dialog_new (BTK_WINDOW (do_widget),
				       BTK_DIALOG_DESTROY_WITH_PARENT,
				       BTK_MESSAGE_ERROR,
				       BTK_BUTTONS_CLOSE,
				       "%s", error->message);
      g_error_free (error);

      g_signal_connect (dialog, "response",
			G_CALLBACK (btk_widget_destroy), NULL);

      btk_widget_show (dialog);
    }


  return NULL;
}
