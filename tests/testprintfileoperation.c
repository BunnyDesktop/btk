#include <bango/bangobairo.h>
#include <btk/btk.h>
#include <math.h>
#include "testprintfileoperation.h"

/* In points */
#define HEADER_HEIGHT (10*72/25.4)
#define HEADER_GAP (3*72/25.4)

G_DEFINE_TYPE (TestPrintFileOperation, test_print_file_operation, BTK_TYPE_PRINT_OPERATION)

static void
test_print_file_operation_finalize (BObject *object)
{
  TestPrintFileOperation *op = TEST_PRINT_FILE_OPERATION (object);
  
  g_free (op->filename);
  
  B_OBJECT_CLASS (test_print_file_operation_parent_class)->finalize (object);
}

static void
test_print_file_operation_init (TestPrintFileOperation *operation)
{
  btk_print_operation_set_unit (BTK_PRINT_OPERATION (operation), BTK_UNIT_POINTS);
  operation->font_size = 14.0;
}

TestPrintFileOperation *
test_print_file_operation_new (const char *filename)
{
  TestPrintFileOperation *op;

  op = g_object_new (TEST_TYPE_PRINT_FILE_OPERATION, NULL);

  op->filename = g_strdup (filename);
  
  return op;
}  
  
void
test_print_file_operation_set_font_size (TestPrintFileOperation *op,
					 double points)
{
  op->font_size = points;
}

static void
test_print_file_operation_begin_print (BtkPrintOperation *operation, BtkPrintContext *context)
{
  TestPrintFileOperation *op = TEST_PRINT_FILE_OPERATION (operation);
  char *contents;
  int i;
  double height;

  height = btk_print_context_get_height (context) - HEADER_HEIGHT - HEADER_GAP;
  
  op->lines_per_page = floor (height / op->font_size);
  
  g_file_get_contents (op->filename,
		       &contents,
		       NULL, NULL);

  op->lines = g_strsplit (contents, "\n", 0);
  g_free (contents);

  i = 0;
  while (op->lines[i] != NULL)
    i++;
  
  op->num_lines = i;
  op->num_pages = (op->num_lines - 1) / op->lines_per_page + 1;
  btk_print_operation_set_n_pages (operation, op->num_pages);
}

static void
test_print_file_operation_draw_page (BtkPrintOperation *operation,
				     BtkPrintContext *context,
				     int page_nr)
{
  bairo_t *cr;
  BangoLayout *layout;
  TestPrintFileOperation *op = TEST_PRINT_FILE_OPERATION (operation);
  double width, text_height;
  int line, i, layout_height;
  BangoFontDescription *desc;
  char *page_str;

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

  bango_layout_set_text (layout, op->filename, -1);
  bango_layout_set_width (layout, width);
  bango_layout_set_alignment (layout, BANGO_ALIGN_CENTER);
			      
  bango_layout_get_size (layout, NULL, &layout_height);
  text_height = (double)layout_height / BANGO_SCALE;

  bairo_move_to (cr, width / 2,  (HEADER_HEIGHT - text_height) / 2);
  bango_bairo_show_layout (cr, layout);

  page_str = g_strdup_printf ("%d/%d", page_nr + 1, op->num_pages);
  bango_layout_set_text (layout, page_str, -1);
  g_free (page_str);
  bango_layout_set_alignment (layout, BANGO_ALIGN_RIGHT);
			      
  bairo_move_to (cr, width - 2, (HEADER_HEIGHT - text_height) / 2);
  bango_bairo_show_layout (cr, layout);
  
  g_object_unref (layout);
  
  layout = btk_print_context_create_bango_layout (context);
  
  desc = bango_font_description_from_string ("mono");
  bango_font_description_set_size (desc, op->font_size * BANGO_SCALE);
  bango_layout_set_font_description (layout, desc);
  bango_font_description_free (desc);
  
  bairo_move_to (cr, 0, HEADER_HEIGHT + HEADER_GAP);
  line = page_nr * op->lines_per_page;
  for (i = 0; i < op->lines_per_page && line < op->num_lines; i++, line++) {
    bango_layout_set_text (layout, op->lines[line], -1);
    bango_bairo_show_layout (cr, layout);
    bairo_rel_move_to (cr, 0, op->font_size);
  }

  g_object_unref (layout);
}

static void
test_print_file_operation_end_print (BtkPrintOperation *operation, BtkPrintContext *context)
{
  TestPrintFileOperation *op = TEST_PRINT_FILE_OPERATION (operation);
  g_strfreev (op->lines);
}

static void
test_print_file_operation_class_init (TestPrintFileOperationClass *class)
{
  BObjectClass *bobject_class = (BObjectClass *)class;
  BtkPrintOperationClass *print_class = (BtkPrintOperationClass *)class;

  bobject_class->finalize = test_print_file_operation_finalize;
  print_class->begin_print = test_print_file_operation_begin_print;
  print_class->draw_page = test_print_file_operation_draw_page;
  print_class->end_print = test_print_file_operation_end_print;
  
}
