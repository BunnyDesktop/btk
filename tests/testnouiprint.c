/* -*- Mode: C; c-basic-offset: 2; -*- */
/* Btk+ - non-ui printing
 *
 * Copyright (C) 2006 Alexander Larsson <alexl@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <math.h>
#include "btk/btk.h"

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
  GMainLoop *loop;
  BtkPrintOperation *print;
  BtkPrintOperationResult res;
  BtkPrintSettings *settings;

  g_type_init (); 
 
  loop = g_main_loop_new (NULL, TRUE);

  settings = btk_print_settings_new ();
  /* btk_print_settings_set_printer (settings, "printer"); */
  
  print = btk_print_operation_new ();
  btk_print_operation_set_print_settings (print, settings);
  btk_print_operation_set_n_pages (print, 1);
  btk_print_operation_set_unit (print, BTK_UNIT_MM);
  g_signal_connect (print, "draw_page", G_CALLBACK (draw_page), NULL);
  res = btk_print_operation_run (print, BTK_PRINT_OPERATION_ACTION_PRINT, NULL, NULL);

  return 0;
}
