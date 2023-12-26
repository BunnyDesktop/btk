/* testcombochange.c
 * Copyright (C) 2004  Red Hat, Inc.
 * Author: Owen Taylor
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <btk/btk.h>
#include <stdarg.h>

BtkWidget *text_view;
BtkListStore *model;
GArray *contents;

static char next_value = 'A';

static void
test_init (void)
{
  if (g_file_test ("../bdk-pixbuf/libpixbufloader-pnm.la",
		   G_FILE_TEST_EXISTS))
    {
      g_setenv ("BDK_PIXBUF_MODULE_FILE", "../bdk-pixbuf/bdk-pixbuf.loaders", TRUE);
      g_setenv ("BTK_IM_MODULE_FILE", "../modules/input/immodules.cache", TRUE);
    }
}

static void
combochange_log (const char *fmt,
     ...)
{
  BtkTextBuffer *buffer = btk_text_view_get_buffer (BTK_TEXT_VIEW (text_view));
  BtkTextIter iter;
  va_list vap;
  char *msg;
  GString *order_string;
  BtkTextMark *tmp_mark;
  int i;

  va_start (vap, fmt);
  
  msg = g_strdup_vprintf (fmt, vap);

  btk_text_buffer_get_end_iter (buffer, &iter);
  btk_text_buffer_insert (buffer, &iter, msg, -1);

  order_string = g_string_new ("\n  ");
  for (i = 0; i < contents->len; i++)
    {
      if (i)
	g_string_append_c (order_string, ' ');
      g_string_append_c (order_string, g_array_index (contents, char, i));
    }
  g_string_append_c (order_string, '\n');
  btk_text_buffer_insert (buffer, &iter, order_string->str, -1);
  g_string_free (order_string, TRUE);

  tmp_mark = btk_text_buffer_create_mark (buffer, NULL, &iter, FALSE);
  btk_text_view_scroll_mark_onscreen (BTK_TEXT_VIEW (text_view), tmp_mark);
  btk_text_buffer_delete_mark (buffer, tmp_mark);

  g_free (msg);
}

static BtkWidget *
align_button_new (const char *text)
{
  BtkWidget *button = btk_button_new ();
  BtkWidget *label = btk_label_new (text);

  btk_container_add (BTK_CONTAINER (button), label);
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);

  return button;
}

static BtkWidget *
create_combo (const char *name,
	      gboolean is_list)
{
  BtkCellRenderer *cell_renderer;
  BtkWidget *combo;
  char *rc_string;
  
  rc_string = g_strdup_printf ("style \"%s-style\" {\n"
			       "  BtkComboBox::appears-as-list = %d\n"
			       "}\n"
			       "\n"
			       "widget \"*.%s\" style \"%s-style\"",
			       name, is_list, name, name);
  btk_rc_parse_string (rc_string);
  g_free (rc_string);

  combo = btk_combo_box_new_with_model (BTK_TREE_MODEL (model));
  cell_renderer = btk_cell_renderer_text_new ();
  btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combo), cell_renderer, TRUE);
  btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (combo), cell_renderer,
				  "text", 0, NULL);

  btk_widget_set_name (combo, name);
  
  return combo;
}

static void
on_insert (void)
{
  BtkTreeIter iter;
  
  int insert_pos;
  char new_value[2];

  new_value[0] = next_value++;
  new_value[1] = '\0';

  if (next_value > 'Z')
    next_value = 'A';
  
  if (contents->len)
    insert_pos = g_random_int_range (0, contents->len + 1);
  else
    insert_pos = 0;
  
  btk_list_store_insert (model, &iter, insert_pos);
  btk_list_store_set (model, &iter, 0, new_value, -1);

  g_array_insert_val (contents, insert_pos, new_value);

  combochange_log ("Inserted '%c' at position %d", new_value[0], insert_pos);
}

static void
on_delete (void)
{
  BtkTreeIter iter;
  
  int delete_pos;
  char old_val;

  if (!contents->len)
    return;
  
  delete_pos = g_random_int_range (0, contents->len);
  btk_tree_model_iter_nth_child (BTK_TREE_MODEL (model), &iter, NULL, delete_pos);
  
  btk_list_store_remove (model, &iter);

  old_val = g_array_index (contents, char, delete_pos);
  g_array_remove_index (contents, delete_pos);
  combochange_log ("Deleted '%c' from position %d", old_val, delete_pos);
}

static void
on_reorder (void)
{
  GArray *new_contents;
  gint *shuffle_array;
  gint i;

  shuffle_array = g_new (int, contents->len);
  
  for (i = 0; i < contents->len; i++)
    shuffle_array[i] = i;

  for (i = 0; i + 1 < contents->len; i++)
    {
      gint pos = g_random_int_range (i, contents->len);
      gint tmp;

      tmp = shuffle_array[i];
      shuffle_array[i] = shuffle_array[pos];
      shuffle_array[pos] = tmp;
    }

  btk_list_store_reorder (model, shuffle_array);

  new_contents = g_array_new (FALSE, FALSE, sizeof (char));
  for (i = 0; i < contents->len; i++)
    g_array_append_val (new_contents,
			g_array_index (contents, char, shuffle_array[i]));
  g_array_free (contents, TRUE);
  contents = new_contents;

  combochange_log ("Reordered array");
    
  g_free (shuffle_array);
}

static int n_animations = 0;
static int timer = 0;

static gint
animation_timer (gpointer data)
{
  switch (g_random_int_range (0, 3)) 
    {
    case 0: 
      on_insert ();
      break;
    case 1:
      on_delete ();
      break;
    case 2:
      on_reorder ();
      break;
    }

  n_animations--;
  return n_animations > 0;
}

static void
on_animate (void)
{
  n_animations += 20;
 
  timer = bdk_threads_add_timeout (1000, (GSourceFunc) animation_timer, NULL);
}

int
main (int argc, char **argv)
{
  BtkWidget *dialog;
  BtkWidget *scrolled_window;
  BtkWidget *hbox;
  BtkWidget *button_vbox;
  BtkWidget *combo_vbox;
  BtkWidget *button;
  BtkWidget *menu_combo;
  BtkWidget *alignment;
  BtkWidget *label;
  BtkWidget *list_combo;
  
  test_init ();

  btk_init (&argc, &argv);

  model = btk_list_store_new (1, G_TYPE_STRING);
  contents = g_array_new (FALSE, FALSE, sizeof (char));
  
  dialog = btk_dialog_new_with_buttons ("BtkComboBox model changes",
					NULL, BTK_DIALOG_NO_SEPARATOR,
					BTK_STOCK_CLOSE, BTK_RESPONSE_CLOSE,
					NULL);
  
  hbox = btk_hbox_new (FALSE, 12);
  btk_container_set_border_width (BTK_CONTAINER (hbox), 12);
  btk_box_pack_start (BTK_BOX (BTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);

  combo_vbox = btk_vbox_new (FALSE, 8);
  btk_box_pack_start (BTK_BOX (hbox), combo_vbox, FALSE, FALSE, 0);

  label = btk_label_new (NULL);
  btk_label_set_markup (BTK_LABEL (label), "<b>Menu mode</b>");
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_box_pack_start (BTK_BOX (combo_vbox), label, FALSE, FALSE, 0);

  alignment = g_object_new (BTK_TYPE_ALIGNMENT, "left-padding", 12, NULL);
  btk_box_pack_start (BTK_BOX (combo_vbox), alignment, FALSE, FALSE, 0);

  menu_combo = create_combo ("menu-combo", FALSE);
  btk_container_add (BTK_CONTAINER (alignment), menu_combo);

  label = btk_label_new (NULL);
  btk_label_set_markup (BTK_LABEL (label), "<b>List mode</b>");
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_box_pack_start (BTK_BOX (combo_vbox), label, FALSE, FALSE, 0);

  alignment = g_object_new (BTK_TYPE_ALIGNMENT, "left-padding", 12, NULL);
  btk_box_pack_start (BTK_BOX (combo_vbox), alignment, FALSE, FALSE, 0);

  list_combo = create_combo ("list-combo", TRUE);
  btk_container_add (BTK_CONTAINER (alignment), list_combo);

  scrolled_window = btk_scrolled_window_new (NULL, NULL);
  btk_box_pack_start (BTK_BOX (hbox), scrolled_window, TRUE, TRUE, 0);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window),
				  BTK_POLICY_AUTOMATIC, BTK_POLICY_AUTOMATIC);

  text_view = btk_text_view_new ();
  btk_text_view_set_editable (BTK_TEXT_VIEW (text_view), FALSE);
  btk_text_view_set_cursor_visible (BTK_TEXT_VIEW (text_view), FALSE);

  btk_container_add (BTK_CONTAINER (scrolled_window), text_view);

  button_vbox = btk_vbox_new (FALSE, 8);
  btk_box_pack_start (BTK_BOX (hbox), button_vbox, FALSE, FALSE, 0);
  
  btk_window_set_default_size (BTK_WINDOW (dialog), 500, 300);

  button = align_button_new ("Insert");
  btk_box_pack_start (BTK_BOX (button_vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", G_CALLBACK (on_insert), NULL);
  
  button = align_button_new ("Delete");
  btk_box_pack_start (BTK_BOX (button_vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", G_CALLBACK (on_delete), NULL);

  button = align_button_new ("Reorder");
  btk_box_pack_start (BTK_BOX (button_vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", G_CALLBACK (on_reorder), NULL);

  button = align_button_new ("Animate");
  btk_box_pack_start (BTK_BOX (button_vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", G_CALLBACK (on_animate), NULL);

  btk_widget_show_all (dialog);
  btk_dialog_run (BTK_DIALOG (dialog));

  return 0;
}
