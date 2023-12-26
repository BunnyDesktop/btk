/* BTK - The GIMP Toolkit
 * testprint.c: Print example
 * Copyright (C) 2006, Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <math.h>
#include <bango/bangobairo.h>
#include <btk/btk.h>
#include "testprintfileoperation.h"

static void
request_page_setup (BtkPrintOperation *operation,
		    BtkPrintContext *context,
		    int page_nr,
		    BtkPageSetup *setup)
{
  /* Make the second page landscape mode a5 */
  if (page_nr == 1)
    {
      BtkPaperSize *a5_size = btk_paper_size_new ("iso_a5");
      
      btk_page_setup_set_orientation (setup, BTK_PAGE_ORIENTATION_LANDSCAPE);
      btk_page_setup_set_paper_size (setup, a5_size);
      btk_paper_size_free (a5_size);
    }
}

static void
draw_page (BtkPrintOperation *operation,
	   BtkPrintContext *context,
	   int page_nr)
{
  bairo_t *cr;
  BangoLayout *layout;
  BangoFontDescription *desc;
  
  cr = btk_print_context_get_bairo_context (context);

  /* Draw a red rectangle, as wide as the paper (inside the margins) */
  bairo_set_source_rgb (cr, 1.0, 0, 0);
  bairo_rectangle (cr, 0, 0, btk_print_context_get_width (context), 50);
  
  bairo_fill (cr);

  /* Draw some lines */
  bairo_move_to (cr, 20, 10);
  bairo_line_to (cr, 40, 20);
  bairo_arc (cr, 60, 60, 20, 0, G_PI);
  bairo_line_to (cr, 80, 20);
  
  bairo_set_source_rgb (cr, 0, 0, 0);
  bairo_set_line_width (cr, 5);
  bairo_set_line_cap (cr, BAIRO_LINE_CAP_ROUND);
  bairo_set_line_join (cr, BAIRO_LINE_JOIN_ROUND);
  
  bairo_stroke (cr);

  /* Draw some text */
  
  layout = btk_print_context_create_bango_layout (context);
  bango_layout_set_text (layout, "Hello World! Printing is easy", -1);
  desc = bango_font_description_from_string ("sans 28");
  bango_layout_set_font_description (layout, desc);
  bango_font_description_free (desc);

  bairo_move_to (cr, 30, 20);
  bango_bairo_layout_path (cr, layout);

  /* Font Outline */
  bairo_set_source_rgb (cr, 0.93, 1.0, 0.47);
  bairo_set_line_width (cr, 0.5);
  bairo_stroke_preserve (cr);

  /* Font Fill */
  bairo_set_source_rgb (cr, 0, 0.0, 1.0);
  bairo_fill (cr);
  
  g_object_unref (layout);
}

int
main (int argc, char **argv)
{
  BtkPrintOperation *print;
  BtkPrintOperationResult res;
  TestPrintFileOperation *print_file;

  btk_init (&argc, &argv);

  /* Test some random drawing, with per-page paper settings */
  print = btk_print_operation_new ();
  btk_print_operation_set_n_pages (print, 2);
  btk_print_operation_set_unit (print, BTK_UNIT_MM);
  btk_print_operation_set_export_filename (print, "test.pdf");
  g_signal_connect (print, "draw_page", G_CALLBACK (draw_page), NULL);
  g_signal_connect (print, "request_page_setup", G_CALLBACK (request_page_setup), NULL);
  res = btk_print_operation_run (print, BTK_PRINT_OPERATION_ACTION_EXPORT, NULL, NULL);

  /* Test subclassing of BtkPrintOperation */
  print_file = test_print_file_operation_new ("testprint.c");
  test_print_file_operation_set_font_size (print_file, 12.0);
  btk_print_operation_set_export_filename (BTK_PRINT_OPERATION (print_file), "test2.pdf");
  res = btk_print_operation_run (BTK_PRINT_OPERATION (print_file), BTK_PRINT_OPERATION_ACTION_EXPORT, NULL, NULL);
  
  return 0;
}
