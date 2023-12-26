/* testtext.c
 * Copyright (C) 2000 Red Hat, Inc
 * Author: Havoc Pennington
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
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#undef BDK_DISABLE_DEPRECATED
#undef BTK_DISABLE_DEPRECATED

#include <btk/btk.h>
#include <bdk/bdkkeysyms.h>
#include <bunnylib/gstdio.h>

#include "prop-editor.h"

typedef struct _Buffer Buffer;
typedef struct _View View;

static gint untitled_serial = 1;

GSList *active_window_stack = NULL;

struct _Buffer
{
  gint refcount;
  BtkTextBuffer *buffer;
  char *filename;
  gint untitled_serial;
  BtkTextTag *invisible_tag;
  BtkTextTag *not_editable_tag;
  BtkTextTag *found_text_tag;
  BtkTextTag *rise_tag;
  BtkTextTag *large_tag;
  BtkTextTag *indent_tag;
  BtkTextTag *margin_tag;
  BtkTextTag *custom_tabs_tag;
  GSList *color_tags;
  guint color_cycle_timeout;
  gdouble start_hue;
};

struct _View
{
  BtkWidget *window;
  BtkWidget *text_view;
  BtkAccelGroup *accel_group;
  BtkItemFactory *item_factory;
  Buffer *buffer;
};

static void push_active_window (BtkWindow *window);
static void pop_active_window (void);
static BtkWindow *get_active_window (void);

static Buffer * create_buffer      (void);
static gboolean check_buffer_saved (Buffer *buffer);
static gboolean save_buffer        (Buffer *buffer);
static gboolean save_as_buffer     (Buffer *buffer);
static char *   buffer_pretty_name (Buffer *buffer);
static void     buffer_filename_set (Buffer *buffer);
static void     buffer_search_forward (Buffer *buffer,
                                       const char *str,
                                       View *view);
static void     buffer_search_backward (Buffer *buffer,
                                       const char *str,
                                       View *view);
static void     buffer_set_colors      (Buffer  *buffer,
                                        gboolean enabled);
static void     buffer_cycle_colors    (Buffer  *buffer);

static View *view_from_widget (BtkWidget *widget);

static View *create_view      (Buffer *buffer);
static void  check_close_view (View   *view);
static void  close_view       (View   *view);
static void  view_set_title   (View   *view);
static void  view_init_menus  (View   *view);
static void  view_add_example_widgets (View *view);

GSList *buffers = NULL;
GSList *views = NULL;

static void
push_active_window (BtkWindow *window)
{
  g_object_ref (window);
  active_window_stack = g_slist_prepend (active_window_stack, window);
}

static void
pop_active_window (void)
{
  g_object_unref (active_window_stack->data);
  active_window_stack = g_slist_delete_link (active_window_stack, active_window_stack);
}

static BtkWindow *
get_active_window (void)
{
  if (active_window_stack)
    return active_window_stack->data;
  else
    return NULL;
}

/*
 * Filesel utility function
 */

typedef gboolean (*FileselOKFunc) (const char *filename, gpointer data);

static void
filesel_ok_cb (BtkWidget *button, BtkWidget *filesel)
{
  FileselOKFunc ok_func = (FileselOKFunc)g_object_get_data (G_OBJECT (filesel), "ok-func");
  gpointer data = g_object_get_data (G_OBJECT (filesel), "ok-data");
  gint *result = g_object_get_data (G_OBJECT (filesel), "ok-result");
  
  btk_widget_hide (filesel);
  
  if ((*ok_func) (btk_file_selection_get_filename (BTK_FILE_SELECTION (filesel)), data))
    {
      btk_widget_destroy (filesel);
      *result = TRUE;
    }
  else
    btk_widget_show (filesel);
}

gboolean
filesel_run (BtkWindow    *parent, 
	     const char   *title,
	     const char   *start_file,
	     FileselOKFunc func,
	     gpointer      data)
{
  BtkWidget *filesel = btk_file_selection_new (title);
  gboolean result = FALSE;

  if (!parent)
    parent = get_active_window ();
  
  if (parent)
    btk_window_set_transient_for (BTK_WINDOW (filesel), parent);

  if (start_file)
    btk_file_selection_set_filename (BTK_FILE_SELECTION (filesel), start_file);

  
  g_object_set_data (G_OBJECT (filesel), "ok-func", func);
  g_object_set_data (G_OBJECT (filesel), "ok-data", data);
  g_object_set_data (G_OBJECT (filesel), "ok-result", &result);

  g_signal_connect (BTK_FILE_SELECTION (filesel)->ok_button,
		    "clicked",
		    G_CALLBACK (filesel_ok_cb), filesel);
  g_signal_connect_swapped (BTK_FILE_SELECTION (filesel)->cancel_button,
			    "clicked",
			    G_CALLBACK (btk_widget_destroy), filesel);

  g_signal_connect (filesel, "destroy",
		    G_CALLBACK (btk_main_quit), NULL);
  btk_window_set_modal (BTK_WINDOW (filesel), TRUE);

  btk_widget_show (filesel);
  btk_main ();

  return result;
}

/*
 * MsgBox utility functions
 */

static void
msgbox_yes_cb (BtkWidget *widget, gboolean *result)
{
  *result = 0;
  btk_object_destroy (BTK_OBJECT (btk_widget_get_toplevel (widget)));
}

static void
msgbox_no_cb (BtkWidget *widget, gboolean *result)
{
  *result = 1;
  btk_object_destroy (BTK_OBJECT (btk_widget_get_toplevel (widget)));
}

static gboolean
msgbox_key_press_cb (BtkWidget *widget, BdkEventKey *event, gpointer data)
{
  if (event->keyval == BDK_Escape)
    {
      g_signal_stop_emission_by_name (widget, "key_press_event");
      btk_object_destroy (BTK_OBJECT (widget));
      return TRUE;
    }

  return FALSE;
}

/* Don't copy this example, it's all crack-smoking - you can just use
 * BtkMessageDialog now
 */
gint
msgbox_run (BtkWindow  *parent,
	    const char *message,
	    const char *yes_button,
	    const char *no_button,
	    const char *cancel_button,
	    gint default_index)
{
  gboolean result = -1;
  BtkWidget *dialog;
  BtkWidget *button;
  BtkWidget *label;
  BtkWidget *vbox;
  BtkWidget *button_box;
  BtkWidget *separator;

  g_return_val_if_fail (message != NULL, FALSE);
  g_return_val_if_fail (default_index >= 0 && default_index <= 1, FALSE);

  if (!parent)
    parent = get_active_window ();
  
  /* Create a dialog
   */
  dialog = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_modal (BTK_WINDOW (dialog), TRUE);
  if (parent)
    btk_window_set_transient_for (BTK_WINDOW (dialog), parent);
  btk_window_set_position (BTK_WINDOW (dialog), BTK_WIN_POS_MOUSE);

  /* Quit our recursive main loop when the dialog is destroyed.
   */
  g_signal_connect (dialog, "destroy",
		    G_CALLBACK (btk_main_quit), NULL);

  /* Catch Escape key presses and have them destroy the dialog
   */
  g_signal_connect (dialog, "key_press_event",
		    G_CALLBACK (msgbox_key_press_cb), NULL);

  /* Fill in the contents of the widget
   */
  vbox = btk_vbox_new (FALSE, 0);
  btk_container_add (BTK_CONTAINER (dialog), vbox);
  
  label = btk_label_new (message);
  btk_misc_set_padding (BTK_MISC (label), 12, 12);
  btk_label_set_line_wrap (BTK_LABEL (label), TRUE);
  btk_box_pack_start (BTK_BOX (vbox), label, TRUE, TRUE, 0);

  separator = btk_hseparator_new ();
  btk_box_pack_start (BTK_BOX (vbox), separator, FALSE, FALSE, 0);

  button_box = btk_hbutton_box_new ();
  btk_box_pack_start (BTK_BOX (vbox), button_box, FALSE, FALSE, 0);
  btk_container_set_border_width (BTK_CONTAINER (button_box), 8);
  

  /* When Yes is clicked, call the msgbox_yes_cb
   * This sets the result variable and destroys the dialog
   */
  if (yes_button)
    {
      button = btk_button_new_with_label (yes_button);
      btk_widget_set_can_default (button, TRUE);
      btk_container_add (BTK_CONTAINER (button_box), button);

      if (default_index == 0)
	btk_widget_grab_default (button);
      
      g_signal_connect (button, "clicked",
			G_CALLBACK (msgbox_yes_cb), &result);
    }

  /* When No is clicked, call the msgbox_no_cb
   * This sets the result variable and destroys the dialog
   */
  if (no_button)
    {
      button = btk_button_new_with_label (no_button);
      btk_widget_set_can_default (button, TRUE);
      btk_container_add (BTK_CONTAINER (button_box), button);

      if (default_index == 0)
	btk_widget_grab_default (button);
      
      g_signal_connect (button, "clicked",
			G_CALLBACK (msgbox_no_cb), &result);
    }

  /* When Cancel is clicked, destroy the dialog
   */
  if (cancel_button)
    {
      button = btk_button_new_with_label (cancel_button);
      btk_widget_set_can_default (button, TRUE);
      btk_container_add (BTK_CONTAINER (button_box), button);
      
      if (default_index == 1)
	btk_widget_grab_default (button);
      
      g_signal_connect_swapped (button, "clicked",
				G_CALLBACK (btk_object_destroy), dialog);
    }

  btk_widget_show_all (dialog);

  /* Run a recursive main loop until a button is clicked
   * or the user destroys the dialog through the window mananger */
  btk_main ();

  return result;
}

#ifdef DO_BLINK
/*
 * Example buffer filling code
 */
static gint
blink_timeout (gpointer data)
{
  BtkTextTag *tag;
  static gboolean flip = FALSE;
  
  tag = BTK_TEXT_TAG (data);

  g_object_set (tag,
                 "foreground", flip ? "blue" : "purple",
                 NULL);

  flip = !flip;

  return TRUE;
}
#endif

static gint
tag_event_handler (BtkTextTag *tag, BtkWidget *widget, BdkEvent *event,
                  const BtkTextIter *iter, gpointer user_data)
{
  gint char_index;

  char_index = btk_text_iter_get_offset (iter);
  
  switch (event->type)
    {
    case BDK_MOTION_NOTIFY:
      printf ("Motion event at char %d tag `%s'\n",
             char_index, tag->name);
      break;
        
    case BDK_BUTTON_PRESS:
      printf ("Button press at char %d tag `%s'\n",
             char_index, tag->name);
      break;
        
    case BDK_2BUTTON_PRESS:
      printf ("Double click at char %d tag `%s'\n",
             char_index, tag->name);
      break;
        
    case BDK_3BUTTON_PRESS:
      printf ("Triple click at char %d tag `%s'\n",
             char_index, tag->name);
      break;
        
    case BDK_BUTTON_RELEASE:
      printf ("Button release at char %d tag `%s'\n",
             char_index, tag->name);
      break;
        
    case BDK_KEY_PRESS:
    case BDK_KEY_RELEASE:
      printf ("Key event at char %d tag `%s'\n",
              char_index, tag->name);
      break;
      
    case BDK_ENTER_NOTIFY:
    case BDK_LEAVE_NOTIFY:
    case BDK_PROPERTY_NOTIFY:
    case BDK_SELECTION_CLEAR:
    case BDK_SELECTION_REQUEST:
    case BDK_SELECTION_NOTIFY:
    case BDK_PROXIMITY_IN:
    case BDK_PROXIMITY_OUT:
    case BDK_DRAG_ENTER:
    case BDK_DRAG_LEAVE:
    case BDK_DRAG_MOTION:
    case BDK_DRAG_STATUS:
    case BDK_DROP_START:
    case BDK_DROP_FINISHED:
    default:
      break;
    }

  return FALSE;
}

static void
setup_tag (BtkTextTag *tag)
{
  g_signal_connect (tag,
		    "event",
		    G_CALLBACK (tag_event_handler),
		    NULL);
}

static const char  *book_closed_xpm[] = {
"16 16 6 1",
"       c None s None",
".      c black",
"X      c red",
"o      c yellow",
"O      c #808080",
"#      c white",
"                ",
"       ..       ",
"     ..XX.      ",
"   ..XXXXX.     ",
" ..XXXXXXXX.    ",
".ooXXXXXXXXX.   ",
"..ooXXXXXXXXX.  ",
".X.ooXXXXXXXXX. ",
".XX.ooXXXXXX..  ",
" .XX.ooXXX..#O  ",
"  .XX.oo..##OO. ",
"   .XX..##OO..  ",
"    .X.#OO..    ",
"     ..O..      ",
"      ..        ",
"                "};

void
fill_example_buffer (BtkTextBuffer *buffer)
{
  BtkTextIter iter, iter2;
  BtkTextTag *tag;
  BtkTextChildAnchor *anchor;
  BdkColor color;
  BdkColor color2;
  BdkPixbuf *pixbuf;
  int i;
  char *str;
  
  /* FIXME this is broken if called twice on a buffer, since
   * we try to create tags a second time.
   */
  
  tag = btk_text_buffer_create_tag (buffer, "fg_blue", NULL);

#ifdef DO_BLINK
  btk_timeout_add (1000, blink_timeout, tag);
#endif     
 
  setup_tag (tag);
  
  color.red = color.green = 0;
  color.blue = 0xffff;
  color2.red = 0xfff;
  color2.blue = 0x0;
  color2.green = 0;
  g_object_set (tag,
                "foreground_bdk", &color,
                "background_bdk", &color2,
                "size_points", 24.0,
                NULL);

  tag = btk_text_buffer_create_tag (buffer, "fg_red", NULL);

  setup_tag (tag);
      
  color.blue = color.green = 0;
  color.red = 0xffff;
  g_object_set (tag,
                "rise", -4 * BANGO_SCALE,
                "foreground_bdk", &color,
                NULL);

  tag = btk_text_buffer_create_tag (buffer, "bg_green", NULL);

  setup_tag (tag);
      
  color.blue = color.red = 0;
  color.green = 0xffff;
  g_object_set (tag,
                "background_bdk", &color,
                "size_points", 10.0,
                NULL);

  tag = btk_text_buffer_create_tag (buffer, "strikethrough", NULL);

  setup_tag (tag);
      
  g_object_set (tag,
                "strikethrough", TRUE,
                NULL);


  tag = btk_text_buffer_create_tag (buffer, "underline", NULL);

  setup_tag (tag);
      
  g_object_set (tag,
                "underline", BANGO_UNDERLINE_SINGLE,
                NULL);

  tag = btk_text_buffer_create_tag (buffer, "underline_error", NULL);

  setup_tag (tag);
      
  g_object_set (tag,
                "underline", BANGO_UNDERLINE_ERROR,
                NULL);

  tag = btk_text_buffer_create_tag (buffer, "centered", NULL);
      
  g_object_set (tag,
                "justification", BTK_JUSTIFY_CENTER,
                NULL);

  tag = btk_text_buffer_create_tag (buffer, "rtl_quote", NULL);
      
  g_object_set (tag,
                "wrap_mode", BTK_WRAP_WORD,
                "direction", BTK_TEXT_DIR_RTL,
                "indent", 30,
                "left_margin", 20,
                "right_margin", 20,
                NULL);


  tag = btk_text_buffer_create_tag (buffer, "negative_indent", NULL);
      
  g_object_set (tag,
                "indent", -25,
                NULL);
  
  btk_text_buffer_get_iter_at_offset (buffer, &iter, 0);

  anchor = btk_text_buffer_create_child_anchor (buffer, &iter);

  g_object_ref (anchor);
  
  g_object_set_data_full (G_OBJECT (buffer), "anchor", anchor,
                          (GDestroyNotify) g_object_unref);
  
  pixbuf = bdk_pixbuf_new_from_xpm_data (book_closed_xpm);
  
  i = 0;
  while (i < 100)
    {
      BtkTextMark * temp_mark;
      
      btk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
          
      btk_text_buffer_insert_pixbuf (buffer, &iter, pixbuf);
          
      str = g_strdup_printf ("%d Hello World! blah blah blah blah blah blah blah blah blah blah blah blah\nwoo woo woo woo woo woo woo woo woo woo woo woo woo woo woo\n",
			    i);
      
      btk_text_buffer_insert (buffer, &iter, str, -1);

      g_free (str);
      
      btk_text_buffer_get_iter_at_line_offset (buffer, &iter, 0, 5);
          
      btk_text_buffer_insert (buffer, &iter,
			     "(Hello World!)\nfoo foo Hello this is some text we are using to text word wrap. It has punctuation! gee; blah - hmm, great.\nnew line with a significant quantity of text on it. This line really does contain some text. More text! More text! More text!\n"
			     /* This is UTF8 stuff, Emacs doesn't
				really know how to display it */
			     "German (Deutsch S\303\274d) Gr\303\274\303\237 Gott Greek (\316\225\316\273\316\273\316\267\316\275\316\271\316\272\316\254) \316\223\316\265\316\271\316\254 \317\203\316\261\317\202 Hebrew(\327\251\327\234\327\225\327\235) Hebrew punctuation(\xd6\xbf\327\251\xd6\xbb\xd6\xbc\xd6\xbb\xd6\xbf\327\234\xd6\xbc\327\225\xd6\xbc\xd6\xbb\xd6\xbb\xd6\xbf\327\235\xd6\xbc\xd6\xbb\xd6\xbf) Japanese (\346\227\245\346\234\254\350\252\236) Thai (\340\270\252\340\270\247\340\270\261\340\270\252\340\270\224\340\270\265\340\270\204\340\270\243\340\270\261\340\270\232) Thai wrong spelling (\340\270\204\340\270\263\340\270\225\340\271\210\340\270\255\340\271\204\340\270\233\340\270\231\340\270\267\340\271\210\340\270\252\340\270\260\340\270\201\340\270\224\340\270\234\340\270\264\340\270\224 \340\270\236\340\270\261\340\270\261\340\271\211\340\270\261\340\270\261\340\271\210\340\270\207\340\271\202\340\270\201\340\270\260)\n", -1);

      temp_mark =
        btk_text_buffer_create_mark (buffer, "tmp_mark", &iter, TRUE);

#if 1
      btk_text_buffer_get_iter_at_line_offset (buffer, &iter, 0, 6);
      btk_text_buffer_get_iter_at_line_offset (buffer, &iter2, 0, 13);

      btk_text_buffer_apply_tag_by_name (buffer, "fg_blue", &iter, &iter2);

      btk_text_buffer_get_iter_at_line_offset (buffer, &iter, 1, 10);
      btk_text_buffer_get_iter_at_line_offset (buffer, &iter2, 1, 16);

      btk_text_buffer_apply_tag_by_name (buffer, "underline", &iter, &iter2);

      btk_text_buffer_get_iter_at_line_offset (buffer, &iter, 1, 4);
      btk_text_buffer_get_iter_at_line_offset (buffer, &iter2, 1, 7);

      btk_text_buffer_apply_tag_by_name (buffer, "underline_error", &iter, &iter2);

      btk_text_buffer_get_iter_at_line_offset (buffer, &iter, 1, 14);
      btk_text_buffer_get_iter_at_line_offset (buffer, &iter2, 1, 24);

      btk_text_buffer_apply_tag_by_name (buffer, "strikethrough", &iter, &iter2);
          
      btk_text_buffer_get_iter_at_line_offset (buffer, &iter, 0, 9);
      btk_text_buffer_get_iter_at_line_offset (buffer, &iter2, 0, 16);

      btk_text_buffer_apply_tag_by_name (buffer, "bg_green", &iter, &iter2);
  
      btk_text_buffer_get_iter_at_line_offset (buffer, &iter, 4, 2);
      btk_text_buffer_get_iter_at_line_offset (buffer, &iter2, 4, 10);

      btk_text_buffer_apply_tag_by_name (buffer, "bg_green", &iter, &iter2);

      btk_text_buffer_get_iter_at_line_offset (buffer, &iter, 4, 8);
      btk_text_buffer_get_iter_at_line_offset (buffer, &iter2, 4, 15);

      btk_text_buffer_apply_tag_by_name (buffer, "fg_red", &iter, &iter2);
#endif

      btk_text_buffer_get_iter_at_mark (buffer, &iter, temp_mark);
      btk_text_buffer_insert (buffer, &iter, "Centered text!\n", -1);
	  
      btk_text_buffer_get_iter_at_mark (buffer, &iter2, temp_mark);
      btk_text_buffer_apply_tag_by_name (buffer, "centered", &iter2, &iter);

      btk_text_buffer_move_mark (buffer, temp_mark, &iter);
      btk_text_buffer_insert (buffer, &iter, "Word wrapped, Right-to-left Quote\n", -1);
      btk_text_buffer_insert (buffer, &iter, "\331\210\331\202\330\257 \330\250\330\257\330\243 \330\253\331\204\330\247\330\253 \331\205\331\206 \330\243\331\203\330\253\330\261 \330\247\331\204\331\205\330\244\330\263\330\263\330\247\330\252 \330\252\331\202\330\257\331\205\330\247 \331\201\331\212 \330\264\330\250\331\203\330\251 \330\247\331\203\330\263\331\212\331\210\331\206 \330\250\330\261\330\247\331\205\330\254\331\207\330\247 \331\203\331\205\331\206\330\270\331\205\330\247\330\252 \331\204\330\247 \330\252\330\263\330\271\331\211 \331\204\331\204\330\261\330\250\330\255\330\214 \330\253\331\205 \330\252\330\255\331\210\331\204\330\252 \331\201\331\212 \330\247\331\204\330\263\331\206\331\210\330\247\330\252 \330\247\331\204\330\256\331\205\330\263 \330\247\331\204\331\205\330\247\330\266\331\212\330\251 \330\245\331\204\331\211 \331\205\330\244\330\263\330\263\330\247\330\252 \331\205\330\247\331\204\331\212\330\251 \331\205\331\206\330\270\331\205\330\251\330\214 \331\210\330\250\330\247\330\252\330\252 \330\254\330\262\330\241\330\247 \331\205\331\206 \330\247\331\204\331\206\330\270\330\247\331\205 \330\247\331\204\331\205\330\247\331\204\331\212 \331\201\331\212 \330\250\331\204\330\257\330\247\331\206\331\207\330\247\330\214 \331\210\331\204\331\203\331\206\331\207\330\247 \330\252\330\252\330\256\330\265\330\265 \331\201\331\212 \330\256\330\257\331\205\330\251 \331\202\330\267\330\247\330\271 \330\247\331\204\331\205\330\264\330\261\331\210\330\271\330\247\330\252 \330\247\331\204\330\265\330\272\331\212\330\261\330\251. \331\210\330\243\330\255\330\257 \330\243\331\203\330\253\330\261 \331\207\330\260\331\207 \330\247\331\204\331\205\330\244\330\263\330\263\330\247\330\252 \331\206\330\254\330\247\330\255\330\247 \331\207\331\210 \302\273\330\250\330\247\331\206\331\203\331\210\330\263\331\210\331\204\302\253 \331\201\331\212 \330\250\331\210\331\204\331\212\331\201\331\212\330\247.\n", -1);
      btk_text_buffer_get_iter_at_mark (buffer, &iter2, temp_mark);
      btk_text_buffer_apply_tag_by_name (buffer, "rtl_quote", &iter2, &iter);

      btk_text_buffer_insert_with_tags (buffer, &iter,
                                        "Paragraph with negative indentation. blah blah blah blah blah. The quick brown fox jumped over the lazy dog.\n",
                                        -1,
                                        btk_text_tag_table_lookup (btk_text_buffer_get_tag_table (buffer),
                                                                   "negative_indent"),
                                        NULL);
      
      ++i;
    }

  g_object_unref (pixbuf);
  
  printf ("%d lines %d chars\n",
          btk_text_buffer_get_line_count (buffer),
          btk_text_buffer_get_char_count (buffer));

  /* Move cursor to start */
  btk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
  btk_text_buffer_place_cursor (buffer, &iter);
  
  btk_text_buffer_set_modified (buffer, FALSE);
}

gboolean
fill_file_buffer (BtkTextBuffer *buffer, const char *filename)
{
  FILE* f;
  gchar buf[2048];
  gint remaining = 0;
  BtkTextIter iter, end;

  f = fopen (filename, "r");
  
  if (f == NULL)
    {
      gchar *err = g_strdup_printf ("Cannot open file '%s': %s",
				    filename, g_strerror (errno));
      msgbox_run (NULL, err, "OK", NULL, NULL, 0);
      g_free (err);
      return FALSE;
    }
  
  btk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
  while (!feof (f))
    {
      gint count;
      const char *leftover;
      int to_read = 2047  - remaining;

      count = fread (buf + remaining, 1, to_read, f);
      buf[count + remaining] = '\0';

      g_utf8_validate (buf, count + remaining, &leftover);
      
      g_assert (g_utf8_validate (buf, leftover - buf, NULL));
      btk_text_buffer_insert (buffer, &iter, buf, leftover - buf);

      remaining = (buf + remaining + count) - leftover;
      g_memmove (buf, leftover, remaining);

      if (remaining > 6 || count < to_read)
	  break;
    }

  if (remaining)
    {
      gchar *err = g_strdup_printf ("Invalid UTF-8 data encountered reading file '%s'", filename);
      msgbox_run (NULL, err, "OK", NULL, NULL, 0);
      g_free (err);
    }
  
  /* We had a newline in the buffer to begin with. (The buffer always contains
   * a newline, so we delete to the end of the buffer to clean up.
   */
  btk_text_buffer_get_end_iter (buffer, &end);
  btk_text_buffer_delete (buffer, &iter, &end);
  
  btk_text_buffer_set_modified (buffer, FALSE);

  return TRUE;
}

static gint
delete_event_cb (BtkWidget *window, BdkEventAny *event, gpointer data)
{
  View *view = view_from_widget (window);

  push_active_window (BTK_WINDOW (window));
  check_close_view (view);
  pop_active_window ();

  return TRUE;
}

/*
 * Menu callbacks
 */

static View *
get_empty_view (View *view)
{
  if (!view->buffer->filename &&
      !btk_text_buffer_get_modified (view->buffer->buffer))
    return view;
  else
    return create_view (create_buffer ());
}

static View *
view_from_widget (BtkWidget *widget)
{
  if (BTK_IS_MENU_ITEM (widget))
    {
      BtkItemFactory *item_factory = btk_item_factory_from_widget (widget);
      return g_object_get_data (G_OBJECT (item_factory), "view");      
    }
  else
    {
      BtkWidget *app = btk_widget_get_toplevel (widget);
      return g_object_get_data (G_OBJECT (app), "view");
    }
}

static void
do_new (gpointer             callback_data,
	guint                callback_action,
	BtkWidget           *widget)
{
  create_view (create_buffer ());
}

static void
do_new_view (gpointer             callback_data,
	     guint                callback_action,
	     BtkWidget           *widget)
{
  View *view = view_from_widget (widget);
  
  create_view (view->buffer);
}

gboolean
open_ok_func (const char *filename, gpointer data)
{
  View *view = data;
  View *new_view = get_empty_view (view);

  if (!fill_file_buffer (new_view->buffer->buffer, filename))
    {
      if (new_view != view)
	close_view (new_view);
      return FALSE;
    }
  else
    {
      g_free (new_view->buffer->filename);
      new_view->buffer->filename = g_strdup (filename);
      buffer_filename_set (new_view->buffer);
      
      return TRUE;
    }
}

static void
do_open (gpointer             callback_data,
	 guint                callback_action,
	 BtkWidget           *widget)
{
  View *view = view_from_widget (widget);

  push_active_window (BTK_WINDOW (view->window));
  filesel_run (NULL, "Open File", NULL, open_ok_func, view);
  pop_active_window ();
}

static void
do_save_as (gpointer             callback_data,
	    guint                callback_action,
	    BtkWidget           *widget)
{
  View *view = view_from_widget (widget);  

  push_active_window (BTK_WINDOW (view->window));
  save_as_buffer (view->buffer);
  pop_active_window ();
}

static void
do_save (gpointer             callback_data,
	 guint                callback_action,
	 BtkWidget           *widget)
{
  View *view = view_from_widget (widget);

  push_active_window (BTK_WINDOW (view->window));
  if (!view->buffer->filename)
    do_save_as (callback_data, callback_action, widget);
  else
    save_buffer (view->buffer);
  pop_active_window ();
}

static void
do_close   (gpointer             callback_data,
	    guint                callback_action,
	    BtkWidget           *widget)
{
  View *view = view_from_widget (widget);

  push_active_window (BTK_WINDOW (view->window));
  check_close_view (view);
  pop_active_window ();
}

static void
do_exit    (gpointer             callback_data,
	    guint                callback_action,
	    BtkWidget           *widget)
{
  View *view = view_from_widget (widget);

  GSList *tmp_list = buffers;

  push_active_window (BTK_WINDOW (view->window));
  while (tmp_list)
    {
      if (!check_buffer_saved (tmp_list->data))
	return;

      tmp_list = tmp_list->next;
    }

  btk_main_quit ();
  pop_active_window ();
}

static void
do_example (gpointer             callback_data,
	    guint                callback_action,
	    BtkWidget           *widget)
{
  View *view = view_from_widget (widget);
  View *new_view;

  new_view = get_empty_view (view);
  
  fill_example_buffer (new_view->buffer->buffer);

  view_add_example_widgets (new_view);
}


static void
do_insert_and_scroll (gpointer             callback_data,
                      guint                callback_action,
                      BtkWidget           *widget)
{
  View *view = view_from_widget (widget);
  BtkTextBuffer *buffer;
  BtkTextIter start, end;
  BtkTextMark *mark;
  
  buffer = view->buffer->buffer;

  btk_text_buffer_get_bounds (buffer, &start, &end);
  mark = btk_text_buffer_create_mark (buffer, NULL, &end, /* right grav */ FALSE);

  btk_text_buffer_insert (buffer, &end,
                          "Hello this is multiple lines of text\n"
                          "Line 1\n"  "Line 2\n"
                          "Line 3\n"  "Line 4\n"
                          "Line 5\n",
                          -1);

  btk_text_view_scroll_to_mark (BTK_TEXT_VIEW (view->text_view), mark,
                                0, TRUE, 0.0, 1.0);
  btk_text_buffer_delete_mark (buffer, mark);
}

static void
do_wrap_changed (gpointer             callback_data,
		 guint                callback_action,
		 BtkWidget           *widget)
{
  View *view = view_from_widget (widget);

  btk_text_view_set_wrap_mode (BTK_TEXT_VIEW (view->text_view), callback_action);
}

static void
do_direction_changed (gpointer             callback_data,
		      guint                callback_action,
		      BtkWidget           *widget)
{
  View *view = view_from_widget (widget);
  
  btk_widget_set_direction (view->text_view, callback_action);
  btk_widget_queue_resize (view->text_view);
}


static void
do_spacing_changed (gpointer             callback_data,
                    guint                callback_action,
                    BtkWidget           *widget)
{
  View *view = view_from_widget (widget);

  if (callback_action)
    {
      btk_text_view_set_pixels_above_lines (BTK_TEXT_VIEW (view->text_view),
                                            23);
      btk_text_view_set_pixels_below_lines (BTK_TEXT_VIEW (view->text_view),
                                            21);
      btk_text_view_set_pixels_inside_wrap (BTK_TEXT_VIEW (view->text_view),
                                            9);
    }
  else
    {
      btk_text_view_set_pixels_above_lines (BTK_TEXT_VIEW (view->text_view),
                                            0);
      btk_text_view_set_pixels_below_lines (BTK_TEXT_VIEW (view->text_view),
                                            0);
      btk_text_view_set_pixels_inside_wrap (BTK_TEXT_VIEW (view->text_view),
                                            0);
    }
}

static void
do_editable_changed (gpointer callback_data,
                     guint callback_action,
                     BtkWidget *widget)
{
  View *view = view_from_widget (widget);

  btk_text_view_set_editable (BTK_TEXT_VIEW (view->text_view), callback_action);
}

static void
change_cursor_color (BtkWidget *widget,
		     gboolean   set)
{
  if (set)
    {
      BdkColor red = {0, 65535, 0, 0};
      btk_widget_modify_cursor (widget, &red, &red);
    }
  else
    btk_widget_modify_cursor (widget, NULL, NULL);
}

static void
do_cursor_visible_changed (gpointer callback_data,
                           guint callback_action,
                           BtkWidget *widget)
{
  View *view = view_from_widget (widget);

  switch (callback_action)
    {
    case 0:
      btk_text_view_set_cursor_visible (BTK_TEXT_VIEW (view->text_view), FALSE);
      break;
    case 1:
      btk_text_view_set_cursor_visible (BTK_TEXT_VIEW (view->text_view), TRUE);
      change_cursor_color (view->text_view, FALSE);
      break;
    case 2:
      btk_text_view_set_cursor_visible (BTK_TEXT_VIEW (view->text_view), TRUE);
      change_cursor_color (view->text_view, TRUE);
      break;
    }
}

static void
do_color_cycle_changed (gpointer callback_data,
                        guint callback_action,
                        BtkWidget *widget)
{
  View *view = view_from_widget (widget);

  buffer_set_colors (view->buffer, callback_action);
}

static void
do_apply_editable (gpointer callback_data,
                   guint callback_action,
                   BtkWidget *widget)
{
  View *view = view_from_widget (widget);
  BtkTextIter start;
  BtkTextIter end;
  
  if (btk_text_buffer_get_selection_bounds (view->buffer->buffer,
                                            &start, &end))
    {
      if (callback_action)
        {
          btk_text_buffer_remove_tag (view->buffer->buffer,
                                      view->buffer->not_editable_tag,
                                      &start, &end);
        }
      else
        {
          btk_text_buffer_apply_tag (view->buffer->buffer,
                                     view->buffer->not_editable_tag,
                                     &start, &end);
        }
    }
}

static void
do_apply_invisible (gpointer callback_data,
                    guint callback_action,
                    BtkWidget *widget)
{
  View *view = view_from_widget (widget);
  BtkTextIter start;
  BtkTextIter end;
  
  if (btk_text_buffer_get_selection_bounds (view->buffer->buffer,
                                            &start, &end))
    {
      if (callback_action)
        {
          btk_text_buffer_remove_tag (view->buffer->buffer,
                                      view->buffer->invisible_tag,
                                      &start, &end);
        }
      else
        {
          btk_text_buffer_apply_tag (view->buffer->buffer,
                                     view->buffer->invisible_tag,
                                     &start, &end);
        }
    }
}

static void
do_apply_rise (gpointer callback_data,
	       guint callback_action,
	       BtkWidget *widget)
{
  View *view = view_from_widget (widget);
  BtkTextIter start;
  BtkTextIter end;
  
  if (btk_text_buffer_get_selection_bounds (view->buffer->buffer,
                                            &start, &end))
    {
      if (callback_action)
        {
          btk_text_buffer_remove_tag (view->buffer->buffer,
                                      view->buffer->rise_tag,
                                      &start, &end);
        }
      else
        {
          btk_text_buffer_apply_tag (view->buffer->buffer,
                                     view->buffer->rise_tag,
                                     &start, &end);
        }
    }
}

static void
do_apply_large (gpointer callback_data,
		guint callback_action,
		BtkWidget *widget)
{
  View *view = view_from_widget (widget);
  BtkTextIter start;
  BtkTextIter end;
  
  if (btk_text_buffer_get_selection_bounds (view->buffer->buffer,
                                            &start, &end))
    {
      if (callback_action)
        {
          btk_text_buffer_remove_tag (view->buffer->buffer,
                                      view->buffer->large_tag,
                                      &start, &end);
        }
      else
        {
          btk_text_buffer_apply_tag (view->buffer->buffer,
                                     view->buffer->large_tag,
                                     &start, &end);
        }
    }
}

static void
do_apply_indent (gpointer callback_data,
		 guint callback_action,
		 BtkWidget *widget)
{
  View *view = view_from_widget (widget);
  BtkTextIter start;
  BtkTextIter end;
  
  if (btk_text_buffer_get_selection_bounds (view->buffer->buffer,
                                            &start, &end))
    {
      if (callback_action)
        {
          btk_text_buffer_remove_tag (view->buffer->buffer,
                                      view->buffer->indent_tag,
                                      &start, &end);
        }
      else
        {
          btk_text_buffer_apply_tag (view->buffer->buffer,
                                     view->buffer->indent_tag,
                                     &start, &end);
        }
    }
}

static void
do_apply_margin (gpointer callback_data,
		 guint callback_action,
		 BtkWidget *widget)
{
  View *view = view_from_widget (widget);
  BtkTextIter start;
  BtkTextIter end;
  
  if (btk_text_buffer_get_selection_bounds (view->buffer->buffer,
                                            &start, &end))
    {
      if (callback_action)
        {
          btk_text_buffer_remove_tag (view->buffer->buffer,
                                      view->buffer->margin_tag,
                                      &start, &end);
        }
      else
        {
          btk_text_buffer_apply_tag (view->buffer->buffer,
                                     view->buffer->margin_tag,
                                     &start, &end);
        }
    }
}

static void
do_apply_tabs (gpointer callback_data,
               guint callback_action,
               BtkWidget *widget)
{
  View *view = view_from_widget (widget);
  BtkTextIter start;
  BtkTextIter end;
  
  if (btk_text_buffer_get_selection_bounds (view->buffer->buffer,
                                            &start, &end))
    {
      if (callback_action)
        {
          btk_text_buffer_remove_tag (view->buffer->buffer,
                                      view->buffer->custom_tabs_tag,
                                      &start, &end);
        }
      else
        {
          btk_text_buffer_apply_tag (view->buffer->buffer,
                                     view->buffer->custom_tabs_tag,
                                     &start, &end);
        }
    }
}

static void
do_apply_colors (gpointer callback_data,
                 guint callback_action,
                 BtkWidget *widget)
{
  View *view = view_from_widget (widget);
  Buffer *buffer = view->buffer;
  BtkTextIter start;
  BtkTextIter end;
  
  if (btk_text_buffer_get_selection_bounds (view->buffer->buffer,
                                            &start, &end))
    {
      if (!callback_action)
        {
          GSList *tmp;
          
          tmp = buffer->color_tags;
          while (tmp != NULL)
            {
              btk_text_buffer_remove_tag (view->buffer->buffer,
                                          tmp->data,
                                          &start, &end);              
              tmp = g_slist_next (tmp);
            }
        }
      else
        {
          GSList *tmp;
          
          tmp = buffer->color_tags;
          while (TRUE)
            {
              BtkTextIter next;
              gboolean done = FALSE;
              
              next = start;
              btk_text_iter_forward_char (&next);
              btk_text_iter_forward_char (&next);

              if (btk_text_iter_compare (&next, &end) >= 0)
                {
                  next = end;
                  done = TRUE;
                }
              
              btk_text_buffer_apply_tag (view->buffer->buffer,
                                         tmp->data,
                                         &start, &next);

              start = next;

              if (done)
                return;
              
              tmp = g_slist_next (tmp);
              if (tmp == NULL)
                tmp = buffer->color_tags;
            } 
        }
    }
}

static void
do_remove_tags (gpointer callback_data,
                guint callback_action,
                BtkWidget *widget)
{
  View *view = view_from_widget (widget);
  BtkTextIter start;
  BtkTextIter end;
  
  if (btk_text_buffer_get_selection_bounds (view->buffer->buffer,
                                            &start, &end))
    {
      btk_text_buffer_remove_all_tags (view->buffer->buffer,
                                       &start, &end);
    }
}

static void
do_properties (gpointer callback_data,
                guint callback_action,
                BtkWidget *widget)
{
  View *view = view_from_widget (widget);

  create_prop_editor (G_OBJECT (view->text_view), 0);
}

static void
rich_text_store_populate (BtkListStore  *store,
                          BtkTextBuffer *buffer,
                          gboolean       deserialize)
{
  BdkAtom *formats;
  gint     n_formats;
  gint     i;

  btk_list_store_clear (store);

  if (deserialize)
    formats = btk_text_buffer_get_deserialize_formats (buffer, &n_formats);
  else
    formats = btk_text_buffer_get_serialize_formats (buffer, &n_formats);

  for (i = 0; i < n_formats; i++)
    {
      BtkTreeIter  iter;
      gchar       *mime_type;
      gboolean     can_create_tags = FALSE;

      mime_type = bdk_atom_name (formats[i]);

      if (deserialize)
        can_create_tags =
          btk_text_buffer_deserialize_get_can_create_tags (buffer, formats[i]);

      btk_list_store_append (store, &iter);
      btk_list_store_set (store, &iter,
                          0, formats[i],
                          1, mime_type,
                          2, can_create_tags,
                          -1);

      g_free (mime_type);
    }

  g_free (formats);
}

static void
rich_text_paste_target_list_notify (BtkTextBuffer    *buffer,
                                    const GParamSpec *pspec,
                                    BtkListStore     *store)
{
  rich_text_store_populate (store, buffer, TRUE);
}

static void
rich_text_copy_target_list_notify (BtkTextBuffer    *buffer,
                                   const GParamSpec *pspec,
                                   BtkListStore     *store)
{
  rich_text_store_populate (store, buffer, FALSE);
}

static void
rich_text_can_create_tags_toggled (BtkCellRendererToggle *toggle,
                                   const gchar           *path,
                                   BtkTreeModel          *model)
{
  BtkTreeIter iter;

  if (btk_tree_model_get_iter_from_string (model, &iter, path))
    {
      BtkTextBuffer *buffer;
      BdkAtom        format;
      gboolean       can_create_tags;

      buffer = g_object_get_data (G_OBJECT (model), "buffer");

      btk_tree_model_get (model, &iter,
                          0, &format,
                          2, &can_create_tags,
                          -1);

      btk_text_buffer_deserialize_set_can_create_tags (buffer, format,
                                                       !can_create_tags);

      btk_list_store_set (BTK_LIST_STORE (model), &iter,
                          2, !can_create_tags,
                          -1);
    }
}

static void
rich_text_unregister_clicked (BtkWidget   *button,
                              BtkTreeView *tv)
{
  BtkTreeSelection *sel = btk_tree_view_get_selection (tv);
  BtkTreeModel     *model;
  BtkTreeIter       iter;

  if (btk_tree_selection_get_selected (sel, &model, &iter))
    {
      BtkTextBuffer *buffer;
      gboolean       deserialize;
      BdkAtom        format;

      buffer = g_object_get_data (G_OBJECT (model), "buffer");
      deserialize = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (model),
                                                        "deserialize"));

      btk_tree_model_get (model, &iter,
                          0, &format,
                          -1);

      if (deserialize)
        btk_text_buffer_unregister_deserialize_format (buffer, format);
      else
        btk_text_buffer_unregister_serialize_format (buffer, format);
    }
}

static void
rich_text_register_clicked (BtkWidget   *button,
                            BtkTreeView *tv)
{
  BtkWidget *dialog;
  BtkWidget *label;
  BtkWidget *entry;

  dialog = btk_dialog_new_with_buttons ("Register new Tagset",
                                        BTK_WINDOW (btk_widget_get_toplevel (button)),
                                        BTK_DIALOG_DESTROY_WITH_PARENT,
                                        BTK_STOCK_CANCEL, BTK_RESPONSE_CANCEL,
                                        BTK_STOCK_OK,     BTK_RESPONSE_OK,
                                        NULL);
  label = btk_label_new ("Enter tagset name or leave blank for "
                         "unrestricted internal format:");
  btk_box_pack_start (BTK_BOX (BTK_DIALOG (dialog)->vbox), label,
                      FALSE, FALSE, 0);

  entry = btk_entry_new ();
  btk_box_pack_start (BTK_BOX (BTK_DIALOG (dialog)->vbox), entry,
                      FALSE, FALSE, 0);

  btk_widget_show_all (dialog);

  if (btk_dialog_run (BTK_DIALOG (dialog)) == BTK_RESPONSE_OK)
    {
      BtkTreeModel  *model  = btk_tree_view_get_model (tv);
      BtkTextBuffer *buffer = g_object_get_data (G_OBJECT (model), "buffer");
      const gchar   *tagset = btk_entry_get_text (BTK_ENTRY (entry));
      gboolean       deserialize;

      deserialize = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (model),
                                                        "deserialize"));

      if (tagset && ! strlen (tagset))
        tagset = NULL;

      if (deserialize)
        btk_text_buffer_register_deserialize_tagset (buffer, tagset);
      else
        btk_text_buffer_register_serialize_tagset (buffer, tagset);
    }

  btk_widget_destroy (dialog);
}

static void
do_rich_text (gpointer callback_data,
              guint deserialize,
              BtkWidget *widget)
{
  View *view = view_from_widget (widget);
  BtkTextBuffer *buffer;
  BtkWidget *dialog;
  BtkWidget *tv;
  BtkWidget *sw;
  BtkWidget *hbox;
  BtkWidget *button;
  BtkListStore *store;

  buffer = btk_text_view_get_buffer (BTK_TEXT_VIEW (view->text_view));

  dialog = btk_dialog_new_with_buttons (deserialize ?
                                        "Rich Text Paste & Drop" :
                                        "Rich Text Copy & Drag",
                                        BTK_WINDOW (view->window),
                                        BTK_DIALOG_DESTROY_WITH_PARENT,
                                        BTK_STOCK_CLOSE, 0,
                                        NULL);
  g_signal_connect (dialog, "response",
                    G_CALLBACK (btk_widget_destroy),
                    NULL);

  store = btk_list_store_new (3,
                              G_TYPE_POINTER,
                              G_TYPE_STRING,
                              G_TYPE_BOOLEAN);

  g_object_set_data (G_OBJECT (store), "buffer", buffer);
  g_object_set_data (G_OBJECT (store), "deserialize",
                     GUINT_TO_POINTER (deserialize));

  tv = btk_tree_view_new_with_model (BTK_TREE_MODEL (store));
  g_object_unref (store);

  btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tv),
                                               0, "Rich Text Format",
                                               btk_cell_renderer_text_new (),
                                               "text", 1,
                                               NULL);

  if (deserialize)
    {
      BtkCellRenderer *renderer = btk_cell_renderer_toggle_new ();

      btk_tree_view_insert_column_with_attributes (BTK_TREE_VIEW (tv),
                                                   1, "Can Create Tags",
                                                   renderer,
                                                   "active", 2,
                                                   NULL);

      g_signal_connect (renderer, "toggled",
                        G_CALLBACK (rich_text_can_create_tags_toggled),
                        store);
    }

  sw = btk_scrolled_window_new (NULL, NULL);
  btk_widget_set_size_request (sw, 300, 100);
  btk_container_add (BTK_CONTAINER (BTK_DIALOG (dialog)->vbox), sw);

  btk_scrolled_window_add_with_viewport (BTK_SCROLLED_WINDOW (sw), tv);

  hbox = btk_hbox_new (FALSE, 6);
  btk_box_pack_start (BTK_BOX (BTK_DIALOG (dialog)->vbox), hbox,
                      FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Unregister Selected Format");
  btk_box_pack_start (BTK_BOX (hbox), button, FALSE, FALSE, 0);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (rich_text_unregister_clicked),
                    tv);

  button = btk_button_new_with_label ("Register New Tagset\n"
                                      "for the Internal Format");
  btk_box_pack_start (BTK_BOX (hbox), button, FALSE, FALSE, 0);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (rich_text_register_clicked),
                    tv);

  if (deserialize)
    g_signal_connect_object (buffer, "notify::paste-target-list",
                             G_CALLBACK (rich_text_paste_target_list_notify),
                             G_OBJECT (store), 0);
  else
    g_signal_connect_object (buffer, "notify::copy-target-list",
                             G_CALLBACK (rich_text_copy_target_list_notify),
                             G_OBJECT (store), 0);

  rich_text_store_populate (store, buffer, deserialize);

  btk_widget_show_all (dialog);
}

enum
{
  RESPONSE_FORWARD,
  RESPONSE_BACKWARD
};

static void
dialog_response_callback (BtkWidget *dialog, gint response_id, gpointer data)
{
  BtkTextBuffer *buffer;
  View *view = data;
  BtkTextIter start, end;
  gchar *search_string;

  if (response_id != RESPONSE_FORWARD &&
      response_id != RESPONSE_BACKWARD)
    {
      btk_widget_destroy (dialog);
      return;
    }
  
  buffer = g_object_get_data (G_OBJECT (dialog), "buffer");

  btk_text_buffer_get_bounds (buffer, &start, &end);
  
  search_string = btk_text_iter_get_text (&start, &end);

  g_print ("Searching for `%s'\n", search_string);

  if (response_id == RESPONSE_FORWARD)
    buffer_search_forward (view->buffer, search_string, view);
  else if (response_id == RESPONSE_BACKWARD)
    buffer_search_backward (view->buffer, search_string, view);
    
  g_free (search_string);
  
  btk_widget_destroy (dialog);
}

static void
do_copy  (gpointer callback_data,
          guint callback_action,
          BtkWidget *widget)
{
  View *view = view_from_widget (widget);
  BtkTextBuffer *buffer;

  buffer = view->buffer->buffer;

  btk_text_buffer_copy_clipboard (buffer,
                                  btk_clipboard_get (BDK_NONE));
}

static void
do_search (gpointer callback_data,
           guint callback_action,
           BtkWidget *widget)
{
  View *view = view_from_widget (widget);
  BtkWidget *dialog;
  BtkWidget *search_text;
  BtkTextBuffer *buffer;

  dialog = btk_dialog_new_with_buttons ("Search",
                                        BTK_WINDOW (view->window),
                                        BTK_DIALOG_DESTROY_WITH_PARENT,
                                        "Forward", RESPONSE_FORWARD,
                                        "Backward", RESPONSE_BACKWARD,
                                        BTK_STOCK_CANCEL,
                                        BTK_RESPONSE_NONE, NULL);


  buffer = btk_text_buffer_new (NULL);

  search_text = btk_text_view_new_with_buffer (buffer);

  g_object_unref (buffer);
  
  btk_box_pack_end (BTK_BOX (BTK_DIALOG (dialog)->vbox),
                    search_text,
                    TRUE, TRUE, 0);

  g_object_set_data (G_OBJECT (dialog), "buffer", buffer);
  
  g_signal_connect (dialog,
                    "response",
                    G_CALLBACK (dialog_response_callback),
                    view);

  btk_widget_show (search_text);

  btk_widget_grab_focus (search_text);
  
  btk_widget_show_all (dialog);
}

static void
do_select_all (gpointer callback_data,
               guint callback_action,
               BtkWidget *widget)
{
  View *view = view_from_widget (widget);
  BtkTextBuffer *buffer;
  BtkTextIter start, end;

  buffer = view->buffer->buffer;

  btk_text_buffer_get_bounds (buffer, &start, &end);
  btk_text_buffer_select_range (buffer, &start, &end);
}

typedef struct
{
  /* position is in coordinate system of text_view_move_child */
  int click_x;
  int click_y;
  int start_x;
  int start_y;
  int button;
} ChildMoveInfo;

static gboolean
movable_child_callback (BtkWidget *child,
                        BdkEvent  *event,
                        gpointer   data)
{
  ChildMoveInfo *info;
  BtkTextView *text_view;

  text_view = BTK_TEXT_VIEW (data);
  
  g_return_val_if_fail (BTK_IS_EVENT_BOX (child), FALSE);
  g_return_val_if_fail (btk_widget_get_parent (child) == BTK_WIDGET (text_view), FALSE);  
  
  info = g_object_get_data (G_OBJECT (child),
                            "testtext-move-info");

  if (info == NULL)
    {
      info = g_new (ChildMoveInfo, 1);      
      info->start_x = -1;
      info->start_y = -1;
      info->button = -1;
      g_object_set_data_full (G_OBJECT (child),
                              "testtext-move-info",
                              info,
                              g_free);
    }
  
  switch (event->type)
    {
    case BDK_BUTTON_PRESS:
      if (info->button < 0)
        {
          if (bdk_pointer_grab (event->button.window,
                                FALSE,
                                BDK_POINTER_MOTION_MASK | BDK_POINTER_MOTION_HINT_MASK |
                                BDK_BUTTON_RELEASE_MASK,
                                NULL,
                                NULL,
                                event->button.time) != BDK_GRAB_SUCCESS)
            return FALSE;
          
          info->button = event->button.button;
          
          info->start_x = child->allocation.x;
          info->start_y = child->allocation.y;
          info->click_x = child->allocation.x + event->button.x;
          info->click_y = child->allocation.y + event->button.y;
        }
      break;

    case BDK_BUTTON_RELEASE:
      if (info->button < 0)
        return FALSE;

      if (info->button == event->button.button)
        {
          int x, y;
          
          bdk_pointer_ungrab (event->button.time);
          info->button = -1;

          /* convert to window coords from event box coords */
          x = info->start_x + (event->button.x + child->allocation.x - info->click_x);
          y = info->start_y + (event->button.y + child->allocation.y - info->click_y);

          btk_text_view_move_child (text_view,
                                    child,
                                    x, y);
        }
      break;

    case BDK_MOTION_NOTIFY:
      {
        int x, y;
        
        if (info->button < 0)
          return FALSE;
        
        bdk_window_get_pointer (child->window, &x, &y, NULL); /* ensure more events */

        /* to window coords from event box coords */
        x += child->allocation.x;
        y += child->allocation.y;
        
        x = info->start_x + (x - info->click_x);
        y = info->start_y + (y - info->click_y);
        
        btk_text_view_move_child (text_view,
                                  child,
                                  x, y);
      }
      break;

    default:
      break;
    }

  return FALSE;
}

static void
add_movable_child (BtkTextView      *text_view,
                   BtkTextWindowType window)
{
  BtkWidget *event_box;
  BtkWidget *label;
  BdkColor color;
  
  label = btk_label_new ("Drag me around");  
  
  event_box = btk_event_box_new ();
  btk_widget_add_events (event_box,
                         BDK_BUTTON_PRESS_MASK | BDK_BUTTON_RELEASE_MASK |
                         BDK_POINTER_MOTION_MASK | BDK_POINTER_MOTION_HINT_MASK);

  color.red = 0xffff;
  color.green = color.blue = 0;
  btk_widget_modify_bg (event_box, BTK_STATE_NORMAL, &color);
  
  btk_container_add (BTK_CONTAINER (event_box), label);

  btk_widget_show_all (event_box);

  g_signal_connect (event_box, "event",
                    G_CALLBACK (movable_child_callback),
                    text_view);

  btk_text_view_add_child_in_window (text_view,
                                     event_box,
                                     window,
                                     0, 0);
}

static void
do_add_children (gpointer callback_data,
                 guint callback_action,
                 BtkWidget *widget)
{
  View *view = view_from_widget (widget);

  add_movable_child (BTK_TEXT_VIEW (view->text_view),
                     BTK_TEXT_WINDOW_WIDGET);
  add_movable_child (BTK_TEXT_VIEW (view->text_view),
                     BTK_TEXT_WINDOW_LEFT);
  add_movable_child (BTK_TEXT_VIEW (view->text_view),
                     BTK_TEXT_WINDOW_RIGHT);
}

static void
do_add_focus_children (gpointer callback_data,
                       guint callback_action,
                       BtkWidget *widget)
{
  View *view = view_from_widget (widget);
  BtkWidget *child;
  BtkTextChildAnchor *anchor;
  BtkTextIter iter;
  BtkTextView *text_view;

  text_view = BTK_TEXT_VIEW (view->text_view);
  
  child = btk_button_new_with_mnemonic ("Button _A in widget->window");

  btk_text_view_add_child_in_window (text_view,
                                     child,
                                     BTK_TEXT_WINDOW_WIDGET,
                                     200, 200);

  child = btk_button_new_with_mnemonic ("Button _B in widget->window");

  btk_text_view_add_child_in_window (text_view,
                                     child,
                                     BTK_TEXT_WINDOW_WIDGET,
                                     350, 300);

  child = btk_button_new_with_mnemonic ("Button _C in left window");

  btk_text_view_add_child_in_window (text_view,
                                     child,
                                     BTK_TEXT_WINDOW_LEFT,
                                     0, 0);

  child = btk_button_new_with_mnemonic ("Button _D in right window");
  
  btk_text_view_add_child_in_window (text_view,
                                     child,
                                     BTK_TEXT_WINDOW_RIGHT,
                                     0, 0);

  btk_text_buffer_get_start_iter (view->buffer->buffer, &iter);
  
  anchor = btk_text_buffer_create_child_anchor (view->buffer->buffer, &iter);

  child = btk_button_new_with_mnemonic ("Button _E in buffer");
  
  btk_text_view_add_child_at_anchor (text_view, child, anchor);

  anchor = btk_text_buffer_create_child_anchor (view->buffer->buffer, &iter);

  child = btk_button_new_with_mnemonic ("Button _F in buffer");
  
  btk_text_view_add_child_at_anchor (text_view, child, anchor);

  anchor = btk_text_buffer_create_child_anchor (view->buffer->buffer, &iter);

  child = btk_button_new_with_mnemonic ("Button _G in buffer");
  
  btk_text_view_add_child_at_anchor (text_view, child, anchor);

  /* show all the buttons */
  btk_widget_show_all (view->text_view);
}

static void
view_init_menus (View *view)
{
  BtkTextDirection direction = btk_widget_get_direction (view->text_view);
  BtkWrapMode wrap_mode = btk_text_view_get_wrap_mode (BTK_TEXT_VIEW (view->text_view));
  BtkWidget *menu_item = NULL;

  switch (direction)
    {
    case BTK_TEXT_DIR_LTR:
      menu_item = btk_item_factory_get_widget (view->item_factory, "/Settings/Left-to-Right");
      break;
    case BTK_TEXT_DIR_RTL:
      menu_item = btk_item_factory_get_widget (view->item_factory, "/Settings/Right-to-Left");
      break;
    default:
      break;
    }

  if (menu_item)
    btk_menu_item_activate (BTK_MENU_ITEM (menu_item));

  switch (wrap_mode)
    {
    case BTK_WRAP_NONE:
      menu_item = btk_item_factory_get_widget (view->item_factory, "/Settings/Wrap Off");
      break;
    case BTK_WRAP_WORD:
      menu_item = btk_item_factory_get_widget (view->item_factory, "/Settings/Wrap Words");
      break;
    case BTK_WRAP_CHAR:
      menu_item = btk_item_factory_get_widget (view->item_factory, "/Settings/Wrap Chars");
      break;
    default:
      break;
    }
  
  if (menu_item)
    btk_menu_item_activate (BTK_MENU_ITEM (menu_item));
}

static BtkItemFactoryEntry menu_items[] =
{
  { "/_File",            NULL,         NULL,        0, "<Branch>" },
  { "/File/_New",        "<control>N", do_new,      0, NULL },
  { "/File/New _View",   NULL,         do_new_view, 0, NULL },
  { "/File/_Open",       "<control>O", do_open,     0, NULL },
  { "/File/_Save",       "<control>S", do_save,     0, NULL },
  { "/File/Save _As...", NULL,         do_save_as,  0, NULL },
  { "/File/sep1",        NULL,         NULL,        0, "<Separator>" },
  { "/File/_Close",     "<control>W" , do_close,    0, NULL },
  { "/File/E_xit",      "<control>Q" , do_exit,     0, NULL },

  { "/_Edit", NULL, 0, 0, "<Branch>" },
  { "/Edit/Copy", NULL, do_copy, 0, NULL },
  { "/Edit/sep1", NULL, NULL, 0, "<Separator>" },
  { "/Edit/Find...", NULL, do_search, 0, NULL },
  { "/Edit/Select All", "<control>A", do_select_all, 0, NULL }, 

  { "/_Settings",   	  NULL,         NULL,             0, "<Branch>" },
  { "/Settings/Wrap _Off",   NULL,      do_wrap_changed,  BTK_WRAP_NONE, "<RadioItem>" },
  { "/Settings/Wrap _Words", NULL,      do_wrap_changed,  BTK_WRAP_WORD, "/Settings/Wrap Off" },
  { "/Settings/Wrap _Chars", NULL,      do_wrap_changed,  BTK_WRAP_CHAR, "/Settings/Wrap Off" },
  { "/Settings/sep1",        NULL,      NULL,             0, "<Separator>" },
  { "/Settings/Editable", NULL,      do_editable_changed,  TRUE, "<RadioItem>" },
  { "/Settings/Not editable",    NULL,      do_editable_changed,  FALSE, "/Settings/Editable" },
  { "/Settings/sep1",        NULL,      NULL,             0, "<Separator>" },

  { "/Settings/Cursor normal",    NULL,      do_cursor_visible_changed,  1, "<RadioItem>" },
  { "/Settings/Cursor not visible", NULL,      do_cursor_visible_changed,  0, "/Settings/Cursor normal" },
  { "/Settings/Cursor colored", NULL,      do_cursor_visible_changed,  2, "/Settings/Cursor normal" },
  { "/Settings/sep1",        NULL,      NULL,          0, "<Separator>" },
  
  { "/Settings/Left-to-Right", NULL,    do_direction_changed,  BTK_TEXT_DIR_LTR, "<RadioItem>" },
  { "/Settings/Right-to-Left", NULL,    do_direction_changed,  BTK_TEXT_DIR_RTL, "/Settings/Left-to-Right" },

  { "/Settings/sep1",        NULL,      NULL,                0, "<Separator>" },
  { "/Settings/Sane spacing", NULL,    do_spacing_changed,  FALSE, "<RadioItem>" },
  { "/Settings/Funky spacing", NULL,    do_spacing_changed,  TRUE, "/Settings/Sane spacing" },
  { "/Settings/sep1",        NULL,      NULL,                0, "<Separator>" },
  { "/Settings/Don't cycle color tags", NULL,    do_color_cycle_changed,  FALSE, "<RadioItem>" },
  { "/Settings/Cycle colors", NULL,    do_color_cycle_changed,  TRUE, "/Settings/Don't cycle color tags" },
  { "/_Attributes",   	  NULL,         NULL,                0, "<Branch>" },
  { "/Attributes/Editable",   	  NULL,         do_apply_editable, TRUE, NULL },
  { "/Attributes/Not editable",   	  NULL,         do_apply_editable, FALSE, NULL },
  { "/Attributes/Invisible",   	  NULL,         do_apply_invisible, FALSE, NULL },
  { "/Attributes/Visible",   	  NULL,         do_apply_invisible, TRUE, NULL },
  { "/Attributes/Rise",   	  NULL,         do_apply_rise, FALSE, NULL },
  { "/Attributes/Large",   	  NULL,         do_apply_large, FALSE, NULL },
  { "/Attributes/Indent",   	  NULL,         do_apply_indent, FALSE, NULL },
  { "/Attributes/Margins",   	  NULL,         do_apply_margin, FALSE, NULL },
  { "/Attributes/Custom tabs",   	  NULL,         do_apply_tabs, FALSE, NULL },
  { "/Attributes/Default tabs",   	  NULL,         do_apply_tabs, TRUE, NULL },
  { "/Attributes/Color cycles",   	  NULL,         do_apply_colors, TRUE, NULL },
  { "/Attributes/No colors",   	          NULL,         do_apply_colors, FALSE, NULL },
  { "/Attributes/Remove all tags",        NULL, do_remove_tags, 0, NULL },
  { "/Attributes/Properties",             NULL, do_properties, 0, NULL },
  { "/Attributes/Rich Text copy & drag",  NULL, do_rich_text, 0, NULL },
  { "/Attributes/Rich Text paste & drop", NULL, do_rich_text, 1, NULL },
  { "/_Test",   	 NULL,         NULL,           0, "<Branch>" },
  { "/Test/_Example",  	 NULL,         do_example,  0, NULL },
  { "/Test/_Insert and scroll", NULL,         do_insert_and_scroll,  0, NULL },
  { "/Test/_Add fixed children", NULL,         do_add_children,  0, NULL },
  { "/Test/A_dd focusable children", NULL,    do_add_focus_children,  0, NULL },
};

static gboolean
save_buffer (Buffer *buffer)
{
  BtkTextIter start, end;
  gchar *chars;
  gboolean result = FALSE;
  gboolean have_backup = FALSE;
  gchar *bak_filename;
  FILE *file;

  g_return_val_if_fail (buffer->filename != NULL, FALSE);

  bak_filename = g_strconcat (buffer->filename, "~", NULL);
  
  if (rename (buffer->filename, bak_filename) != 0)
    {
      if (errno != ENOENT)
	{
	  gchar *err = g_strdup_printf ("Cannot back up '%s' to '%s': %s",
					buffer->filename, bak_filename, g_strerror (errno));
	  msgbox_run (NULL, err, "OK", NULL, NULL, 0);
	  g_free (err);
          return FALSE;
	}
    }
  else
    have_backup = TRUE;
  
  file = fopen (buffer->filename, "w");
  if (!file)
    {
      gchar *err = g_strdup_printf ("Cannot back up '%s' to '%s': %s",
				    buffer->filename, bak_filename, g_strerror (errno));
      msgbox_run (NULL, err, "OK", NULL, NULL, 0);
    }
  else
    {
      btk_text_buffer_get_iter_at_offset (buffer->buffer, &start, 0);
      btk_text_buffer_get_end_iter (buffer->buffer, &end);
  
      chars = btk_text_buffer_get_slice (buffer->buffer, &start, &end, FALSE);

      if (fputs (chars, file) == EOF ||
	  fclose (file) == EOF)
	{
	  gchar *err = g_strdup_printf ("Error writing to '%s': %s",
					buffer->filename, g_strerror (errno));
	  msgbox_run (NULL, err, "OK", NULL, NULL, 0);
	  g_free (err);
	}
      else
	{
	  /* Success
	   */
	  result = TRUE;
	  btk_text_buffer_set_modified (buffer->buffer, FALSE);	  
	}
	
      g_free (chars);
    }

  if (!result && have_backup)
    {
      if (rename (bak_filename, buffer->filename) != 0)
	{
	  gchar *err = g_strdup_printf ("Error restoring backup file '%s' to '%s': %s\nBackup left as '%s'",
					buffer->filename, bak_filename, g_strerror (errno), bak_filename);
	  msgbox_run (NULL, err, "OK", NULL, NULL, 0);
	  g_free (err);
	}
    }

  g_free (bak_filename);
  
  return result;
}

static gboolean
save_as_ok_func (const char *filename, gpointer data)
{
  Buffer *buffer = data;
  char *old_filename = buffer->filename;

  if (!buffer->filename || strcmp (filename, buffer->filename) != 0)
    {
      GStatBuf statbuf;

      if (g_stat (filename, &statbuf) == 0)
	{
	  gchar *err = g_strdup_printf ("Ovewrite existing file '%s'?", filename);
	  gint result = msgbox_run (NULL, err, "Yes", "No", NULL, 1);
	  g_free (err);

	  if (result != 0)
	    return FALSE;
	}
    }
  
  buffer->filename = g_strdup (filename);

  if (save_buffer (buffer))
    {
      g_free (old_filename);
      buffer_filename_set (buffer);
      return TRUE;
    }
  else
    {
      g_free (buffer->filename);
      buffer->filename = old_filename;
      return FALSE;
    }
}

static gboolean
save_as_buffer (Buffer *buffer)
{
  return filesel_run (NULL, "Save File", NULL, save_as_ok_func, buffer);
}

static gboolean
check_buffer_saved (Buffer *buffer)
{
  if (btk_text_buffer_get_modified (buffer->buffer))
    {
      char *pretty_name = buffer_pretty_name (buffer);
      char *msg = g_strdup_printf ("Save changes to '%s'?", pretty_name);
      gint result;
      
      g_free (pretty_name);
      
      result = msgbox_run (NULL, msg, "Yes", "No", "Cancel", 0);
      g_free (msg);
  
      if (result == 0)
	return save_as_buffer (buffer);
      else if (result == 1)
	return TRUE;
      else
	return FALSE;
    }
  else
    return TRUE;
}

#define N_COLORS 16

static Buffer *
create_buffer (void)
{
  Buffer *buffer;
  BangoTabArray *tabs;
  gint i;
  
  buffer = g_new (Buffer, 1);

  buffer->buffer = btk_text_buffer_new (NULL);
  
  buffer->refcount = 1;
  buffer->filename = NULL;
  buffer->untitled_serial = -1;

  buffer->color_tags = NULL;
  buffer->color_cycle_timeout = 0;
  buffer->start_hue = 0.0;
  
  i = 0;
  while (i < N_COLORS)
    {
      BtkTextTag *tag;

      tag = btk_text_buffer_create_tag (buffer->buffer, NULL, NULL);
      
      buffer->color_tags = g_slist_prepend (buffer->color_tags, tag);
      
      ++i;
    }

#if 1  
  buffer->invisible_tag = btk_text_buffer_create_tag (buffer->buffer, NULL,
                                                      "invisible", TRUE, NULL);
#endif  
  
  buffer->not_editable_tag =
    btk_text_buffer_create_tag (buffer->buffer, NULL,
                                "editable", FALSE,
                                "foreground", "purple", NULL);

  buffer->found_text_tag = btk_text_buffer_create_tag (buffer->buffer, NULL,
                                                       "foreground", "red", NULL);

  buffer->rise_tag = btk_text_buffer_create_tag (buffer->buffer, NULL,
						 "rise", 10 * BANGO_SCALE, NULL);

  buffer->large_tag = btk_text_buffer_create_tag (buffer->buffer, NULL,
						 "scale", BANGO_SCALE_X_LARGE, NULL);

  buffer->indent_tag = btk_text_buffer_create_tag (buffer->buffer, NULL,
						   "indent", 20, NULL);

  buffer->margin_tag = btk_text_buffer_create_tag (buffer->buffer, NULL,
						   "left_margin", 20, "right_margin", 20, NULL);

  tabs = bango_tab_array_new_with_positions (4,
                                             TRUE,
                                             BANGO_TAB_LEFT, 10,
                                             BANGO_TAB_LEFT, 30,
                                             BANGO_TAB_LEFT, 60,
                                             BANGO_TAB_LEFT, 120);
  
  buffer->custom_tabs_tag = btk_text_buffer_create_tag (buffer->buffer, NULL,
                                                        "tabs", tabs,
                                                        "foreground", "green", NULL);

  bango_tab_array_free (tabs);
  
  buffers = g_slist_prepend (buffers, buffer);
  
  return buffer;
}

static char *
buffer_pretty_name (Buffer *buffer)
{
  if (buffer->filename)
    {
      char *p;
      char *result = g_path_get_basename (buffer->filename);
      p = strchr (result, '/');
      if (p)
	*p = '\0';

      return result;
    }
  else
    {
      if (buffer->untitled_serial == -1)
	buffer->untitled_serial = untitled_serial++;

      if (buffer->untitled_serial == 1)
	return g_strdup ("Untitled");
      else
	return g_strdup_printf ("Untitled #%d", buffer->untitled_serial);
    }
}

static void
buffer_filename_set (Buffer *buffer)
{
  GSList *tmp_list = views;

  while (tmp_list)
    {
      View *view = tmp_list->data;

      if (view->buffer == buffer)
	view_set_title (view);

      tmp_list = tmp_list->next;
    }
}

static void
buffer_search (Buffer     *buffer,
               const char *str,
               View       *view,
               gboolean forward)
{
  BtkTextIter iter;
  BtkTextIter start, end;
  BtkWidget *dialog;
  int i;
  
  /* remove tag from whole buffer */
  btk_text_buffer_get_bounds (buffer->buffer, &start, &end);
  btk_text_buffer_remove_tag (buffer->buffer,  buffer->found_text_tag,
                              &start, &end );
  
  btk_text_buffer_get_iter_at_mark (buffer->buffer, &iter,
                                    btk_text_buffer_get_mark (buffer->buffer,
                                                              "insert"));

  i = 0;
  if (*str != '\0')
    {
      BtkTextIter match_start, match_end;

      if (forward)
        {
          while (btk_text_iter_forward_search (&iter, str,
                                               BTK_TEXT_SEARCH_VISIBLE_ONLY |
                                               BTK_TEXT_SEARCH_TEXT_ONLY,
                                               &match_start, &match_end,
                                               NULL))
            {
              ++i;
              btk_text_buffer_apply_tag (buffer->buffer, buffer->found_text_tag,
                                         &match_start, &match_end);
              
              iter = match_end;
            }
        }
      else
        {
          while (btk_text_iter_backward_search (&iter, str,
                                                BTK_TEXT_SEARCH_VISIBLE_ONLY |
                                                BTK_TEXT_SEARCH_TEXT_ONLY,
                                                &match_start, &match_end,
                                                NULL))
            {
              ++i;
              btk_text_buffer_apply_tag (buffer->buffer, buffer->found_text_tag,
                                         &match_start, &match_end);
              
              iter = match_start;
            }
        }
    }

  dialog = btk_message_dialog_new (BTK_WINDOW (view->window),
				   BTK_DIALOG_DESTROY_WITH_PARENT,
                                   BTK_MESSAGE_INFO,
                                   BTK_BUTTONS_OK,
                                   "%d strings found and marked in red",
                                   i);

  g_signal_connect_swapped (dialog,
                            "response",
                            G_CALLBACK (btk_widget_destroy), dialog);
  
  btk_widget_show (dialog);
}

static void
buffer_search_forward (Buffer *buffer, const char *str,
                       View *view)
{
  buffer_search (buffer, str, view, TRUE);
}

static void
buffer_search_backward (Buffer *buffer, const char *str,
                        View *view)
{
  buffer_search (buffer, str, view, FALSE);
}

static void
buffer_ref (Buffer *buffer)
{
  buffer->refcount++;
}

static void
buffer_unref (Buffer *buffer)
{
  buffer->refcount--;
  if (buffer->refcount == 0)
    {
      buffer_set_colors (buffer, FALSE);
      buffers = g_slist_remove (buffers, buffer);
      g_object_unref (buffer->buffer);
      g_free (buffer->filename);
      g_free (buffer);
    }
}

static void
hsv_to_rgb (gdouble *h,
	    gdouble *s,
	    gdouble *v)
{
  gdouble hue, saturation, value;
  gdouble f, p, q, t;

  if (*s == 0.0)
    {
      *h = *v;
      *s = *v;
      *v = *v; /* heh */
    }
  else
    {
      hue = *h * 6.0;
      saturation = *s;
      value = *v;
      
      if (hue >= 6.0)
	hue = 0.0;
      
      f = hue - (int) hue;
      p = value * (1.0 - saturation);
      q = value * (1.0 - saturation * f);
      t = value * (1.0 - saturation * (1.0 - f));
      
      switch ((int) hue)
	{
	case 0:
	  *h = value;
	  *s = t;
	  *v = p;
	  break;
	  
	case 1:
	  *h = q;
	  *s = value;
	  *v = p;
	  break;
	  
	case 2:
	  *h = p;
	  *s = value;
	  *v = t;
	  break;
	  
	case 3:
	  *h = p;
	  *s = q;
	  *v = value;
	  break;
	  
	case 4:
	  *h = t;
	  *s = p;
	  *v = value;
	  break;
	  
	case 5:
	  *h = value;
	  *s = p;
	  *v = q;
	  break;
	  
	default:
	  g_assert_not_reached ();
	}
    }
}

static void
hue_to_color (gdouble   hue,
              BdkColor *color)
{
  gdouble h, s, v;

  h = hue;
  s = 1.0;
  v = 1.0;

  g_return_if_fail (hue <= 1.0);
  
  hsv_to_rgb (&h, &s, &v);

  color->red = h * 65535;
  color->green = s * 65535;
  color->blue = v * 65535;
}


static gint
color_cycle_timeout (gpointer data)
{
  Buffer *buffer = data;

  buffer_cycle_colors (buffer);

  return TRUE;
}

static void
buffer_set_colors (Buffer  *buffer,
                   gboolean enabled)
{
  GSList *tmp;
  gdouble hue = 0.0;

  if (enabled && buffer->color_cycle_timeout == 0)
    buffer->color_cycle_timeout = bdk_threads_add_timeout (200, color_cycle_timeout, buffer);
  else if (!enabled && buffer->color_cycle_timeout != 0)
    {
      g_source_remove (buffer->color_cycle_timeout);
      buffer->color_cycle_timeout = 0;
    }
    
  tmp = buffer->color_tags;
  while (tmp != NULL)
    {
      if (enabled)
        {
          BdkColor color;
          
          hue_to_color (hue, &color);

          g_object_set (tmp->data,
                        "foreground_bdk", &color,
                        NULL);
        }
      else
        g_object_set (tmp->data,
                      "foreground_set", FALSE,
                      NULL);

      hue += 1.0 / N_COLORS;
      
      tmp = g_slist_next (tmp);
    }
}

static void
buffer_cycle_colors (Buffer *buffer)
{
  GSList *tmp;
  gdouble hue = buffer->start_hue;
  
  tmp = buffer->color_tags;
  while (tmp != NULL)
    {
      BdkColor color;
      
      hue_to_color (hue, &color);
      
      g_object_set (tmp->data,
                    "foreground_bdk", &color,
                    NULL);

      hue += 1.0 / N_COLORS;
      if (hue > 1.0)
        hue = 0.0;
      
      tmp = g_slist_next (tmp);
    }

  buffer->start_hue += 1.0 / N_COLORS;
  if (buffer->start_hue > 1.0)
    buffer->start_hue = 0.0;
}

static void
close_view (View *view)
{
  views = g_slist_remove (views, view);
  buffer_unref (view->buffer);
  btk_widget_destroy (view->window);
  g_object_unref (view->item_factory);
  
  g_free (view);
  
  if (!views)
    btk_main_quit ();
}

static void
check_close_view (View *view)
{
  if (view->buffer->refcount > 1 ||
      check_buffer_saved (view->buffer))
    close_view (view);
}

static void
view_set_title (View *view)
{
  char *pretty_name = buffer_pretty_name (view->buffer);
  char *title = g_strconcat ("testtext - ", pretty_name, NULL);

  btk_window_set_title (BTK_WINDOW (view->window), title);

  g_free (pretty_name);
  g_free (title);
}

static void
cursor_set_callback (BtkTextBuffer     *buffer,
                     const BtkTextIter *location,
                     BtkTextMark       *mark,
                     gpointer           user_data)
{
  BtkTextView *text_view;

  /* Redraw tab windows if the cursor moves
   * on the mapped widget (windows may not exist before realization...
   */
  
  text_view = BTK_TEXT_VIEW (user_data);
  
  if (btk_widget_get_mapped (BTK_WIDGET (text_view)) &&
      mark == btk_text_buffer_get_insert (buffer))
    {
      BdkWindow *tab_window;

      tab_window = btk_text_view_get_window (text_view,
                                             BTK_TEXT_WINDOW_TOP);

      bdk_window_invalidate_rect (tab_window, NULL, FALSE);
      
      tab_window = btk_text_view_get_window (text_view,
                                             BTK_TEXT_WINDOW_BOTTOM);

      bdk_window_invalidate_rect (tab_window, NULL, FALSE);
    }
}

static gint
tab_stops_expose (BtkWidget      *widget,
                  BdkEventExpose *event,
                  gpointer        user_data)
{
  gint first_x;
  gint last_x;
  gint i;
  BdkWindow *top_win;
  BdkWindow *bottom_win;
  BtkTextView *text_view;
  BtkTextWindowType type;
  BdkDrawable *target;
  gint *positions = NULL;
  gint size;
  BtkTextAttributes *attrs;
  BtkTextIter insert;
  BtkTextBuffer *buffer;
  gboolean in_pixels;
  
  text_view = BTK_TEXT_VIEW (widget);
  
  /* See if this expose is on the tab stop window */
  top_win = btk_text_view_get_window (text_view,
                                      BTK_TEXT_WINDOW_TOP);

  bottom_win = btk_text_view_get_window (text_view,
                                         BTK_TEXT_WINDOW_BOTTOM);

  if (event->window == top_win)
    {
      type = BTK_TEXT_WINDOW_TOP;
      target = top_win;
    }
  else if (event->window == bottom_win)
    {
      type = BTK_TEXT_WINDOW_BOTTOM;
      target = bottom_win;
    }
  else
    return FALSE;
  
  first_x = event->area.x;
  last_x = first_x + event->area.width;

  btk_text_view_window_to_buffer_coords (text_view,
                                         type,
                                         first_x,
                                         0,
                                         &first_x,
                                         NULL);

  btk_text_view_window_to_buffer_coords (text_view,
                                         type,
                                         last_x,
                                         0,
                                         &last_x,
                                         NULL);

  buffer = btk_text_view_get_buffer (text_view);

  btk_text_buffer_get_iter_at_mark (buffer,
                                    &insert,
                                    btk_text_buffer_get_mark (buffer,
                                                              "insert"));
  
  attrs = btk_text_attributes_new ();

  btk_text_iter_get_attributes (&insert, attrs);

  if (attrs->tabs)
    {
      size = bango_tab_array_get_size (attrs->tabs);
      
      bango_tab_array_get_tabs (attrs->tabs,
                                NULL,
                                &positions);

      in_pixels = bango_tab_array_get_positions_in_pixels (attrs->tabs);
    }
  else
    {
      size = 0;
      in_pixels = FALSE;
    }
      
  btk_text_attributes_unref (attrs);
  
  i = 0;
  while (i < size)
    {
      gint pos;

      if (!in_pixels)
        positions[i] = BANGO_PIXELS (positions[i]);
      
      btk_text_view_buffer_to_window_coords (text_view,
                                             type,
                                             positions[i],
                                             0,
                                             &pos,
                                             NULL);
      
      bdk_draw_line (target, 
                     widget->style->fg_gc [widget->state],
                     pos, 0,
                     pos, 15); 
      
      ++i;
    }

  g_free (positions);

  return TRUE;
}

static void
get_lines (BtkTextView  *text_view,
           gint          first_y,
           gint          last_y,
           GArray       *buffer_coords,
           GArray       *numbers,
           gint         *countp)
{
  BtkTextIter iter;
  gint count;
  gint size;  

  g_array_set_size (buffer_coords, 0);
  g_array_set_size (numbers, 0);
  
  /* Get iter at first y */
  btk_text_view_get_line_at_y (text_view, &iter, first_y, NULL);

  /* For each iter, get its location and add it to the arrays.
   * Stop when we pass last_y
   */
  count = 0;
  size = 0;

  while (!btk_text_iter_is_end (&iter))
    {
      gint y, height;
      gint line_num;
      
      btk_text_view_get_line_yrange (text_view, &iter, &y, &height);

      g_array_append_val (buffer_coords, y);
      line_num = btk_text_iter_get_line (&iter);
      g_array_append_val (numbers, line_num);
      
      ++count;

      if ((y + height) >= last_y)
        break;
      
      btk_text_iter_forward_line (&iter);
    }

  *countp = count;
}

static gint
line_numbers_expose (BtkWidget      *widget,
                     BdkEventExpose *event,
                     gpointer        user_data)
{
  gint count;
  GArray *numbers;
  GArray *pixels;
  gint first_y;
  gint last_y;
  gint i;
  BdkWindow *left_win;
  BdkWindow *right_win;
  BangoLayout *layout;
  BtkTextView *text_view;
  BtkTextWindowType type;
  BdkDrawable *target;
  
  text_view = BTK_TEXT_VIEW (widget);
  
  /* See if this expose is on the line numbers window */
  left_win = btk_text_view_get_window (text_view,
                                       BTK_TEXT_WINDOW_LEFT);

  right_win = btk_text_view_get_window (text_view,
                                        BTK_TEXT_WINDOW_RIGHT);

  if (event->window == left_win)
    {
      type = BTK_TEXT_WINDOW_LEFT;
      target = left_win;
    }
  else if (event->window == right_win)
    {
      type = BTK_TEXT_WINDOW_RIGHT;
      target = right_win;
    }
  else
    return FALSE;
  
  first_y = event->area.y;
  last_y = first_y + event->area.height;

  btk_text_view_window_to_buffer_coords (text_view,
                                         type,
                                         0,
                                         first_y,
                                         NULL,
                                         &first_y);

  btk_text_view_window_to_buffer_coords (text_view,
                                         type,
                                         0,
                                         last_y,
                                         NULL,
                                         &last_y);

  numbers = g_array_new (FALSE, FALSE, sizeof (gint));
  pixels = g_array_new (FALSE, FALSE, sizeof (gint));
  
  get_lines (text_view,
             first_y,
             last_y,
             pixels,
             numbers,
             &count);
  
  /* Draw fully internationalized numbers! */
  
  layout = btk_widget_create_bango_layout (widget, "");
  
  i = 0;
  while (i < count)
    {
      gint pos;
      gchar *str;
      
      btk_text_view_buffer_to_window_coords (text_view,
                                             type,
                                             0,
                                             g_array_index (pixels, gint, i),
                                             NULL,
                                             &pos);

      str = g_strdup_printf ("%d", g_array_index (numbers, gint, i));

      bango_layout_set_text (layout, str, -1);

      btk_paint_layout (widget->style,
                        target,
                        btk_widget_get_state (widget),
                        FALSE,
                        NULL,
                        widget,
                        NULL,
                        2, pos + 2,
                        layout);

      g_free (str);
      
      ++i;
    }

  g_array_free (pixels, TRUE);
  g_array_free (numbers, TRUE);
  
  g_object_unref (layout);

  /* don't stop emission, need to draw children */
  return FALSE;
}

static void
selection_changed (BtkTextBuffer *buffer,
		   GParamSpec    *pspec,
		   BtkWidget     *copy_menu)
{
  btk_widget_set_sensitive (copy_menu, btk_text_buffer_get_has_selection (buffer));
}

static View *
create_view (Buffer *buffer)
{
  View *view;
  BtkWidget *copy_menu;
  BtkWidget *sw;
  BtkWidget *vbox;
  
  view = g_new0 (View, 1);
  views = g_slist_prepend (views, view);

  view->buffer = buffer;
  buffer_ref (buffer);
  
  view->window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  g_object_set_data (G_OBJECT (view->window), "view", view);
  
  g_signal_connect (view->window, "delete_event",
		    G_CALLBACK (delete_event_cb), NULL);

  view->accel_group = btk_accel_group_new ();
  view->item_factory = btk_item_factory_new (BTK_TYPE_MENU_BAR, "<main>", view->accel_group);
  g_object_set_data (G_OBJECT (view->item_factory), "view", view);
  
  btk_item_factory_create_items (view->item_factory, G_N_ELEMENTS (menu_items), menu_items, view);

  /* make the Copy menu item sensitivity update according to the selection */
  copy_menu = btk_item_factory_get_item (view->item_factory, "<main>/Edit/Copy");
  btk_widget_set_sensitive (copy_menu, btk_text_buffer_get_has_selection (view->buffer->buffer));
  g_signal_connect (view->buffer->buffer,
		    "notify::has-selection",
		    G_CALLBACK (selection_changed),
		    copy_menu);

  btk_window_add_accel_group (BTK_WINDOW (view->window), view->accel_group);

  vbox = btk_vbox_new (FALSE, 0);
  btk_container_add (BTK_CONTAINER (view->window), vbox);

  btk_box_pack_start (BTK_BOX (vbox),
		      btk_item_factory_get_widget (view->item_factory, "<main>"),
		      FALSE, FALSE, 0);
  
  sw = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (sw),
                                 BTK_POLICY_AUTOMATIC,
                                 BTK_POLICY_AUTOMATIC);

  view->text_view = btk_text_view_new_with_buffer (buffer->buffer);
  btk_text_view_set_wrap_mode (BTK_TEXT_VIEW (view->text_view),
                               BTK_WRAP_WORD);

  /* Make sure border width works, no real reason to do this other than testing */
  btk_container_set_border_width (BTK_CONTAINER (view->text_view),
                                  10);
  
  /* Draw tab stops in the top and bottom windows. */
  
  btk_text_view_set_border_window_size (BTK_TEXT_VIEW (view->text_view),
                                        BTK_TEXT_WINDOW_TOP,
                                        15);

  btk_text_view_set_border_window_size (BTK_TEXT_VIEW (view->text_view),
                                        BTK_TEXT_WINDOW_BOTTOM,
                                        15);

  g_signal_connect (view->text_view,
                    "expose_event",
                    G_CALLBACK (tab_stops_expose),
                    NULL);  

  g_signal_connect (view->buffer->buffer,
		    "mark_set",
		    G_CALLBACK (cursor_set_callback),
		    view->text_view);
  
  /* Draw line numbers in the side windows; we should really be
   * more scientific about what width we set them to.
   */
  btk_text_view_set_border_window_size (BTK_TEXT_VIEW (view->text_view),
                                        BTK_TEXT_WINDOW_RIGHT,
                                        30);
  
  btk_text_view_set_border_window_size (BTK_TEXT_VIEW (view->text_view),
                                        BTK_TEXT_WINDOW_LEFT,
                                        30);
  
  g_signal_connect (view->text_view,
                    "expose_event",
                    G_CALLBACK (line_numbers_expose),
                    NULL);
  
  btk_box_pack_start (BTK_BOX (vbox), sw, TRUE, TRUE, 0);
  btk_container_add (BTK_CONTAINER (sw), view->text_view);

  btk_window_set_default_size (BTK_WINDOW (view->window), 500, 500);

  btk_widget_grab_focus (view->text_view);

  view_set_title (view);
  view_init_menus (view);

  view_add_example_widgets (view);
  
  btk_widget_show_all (view->window);
  return view;
}

static void
view_add_example_widgets (View *view)
{
  BtkTextChildAnchor *anchor;
  Buffer *buffer;

  buffer = view->buffer;
  
  anchor = g_object_get_data (G_OBJECT (buffer->buffer),
                              "anchor");

  if (anchor && !btk_text_child_anchor_get_deleted (anchor))
    {
      BtkWidget *widget;

      widget = btk_button_new_with_label ("Foo");
      
      btk_text_view_add_child_at_anchor (BTK_TEXT_VIEW (view->text_view),
                                         widget,
                                         anchor);

      btk_widget_show (widget);
    }
}

void
test_init (void)
{
  g_setenv ("BTK_IM_MODULE_FILE", "../modules/input/immodules.cache", TRUE);
}

int
main (int argc, char** argv)
{
  Buffer *buffer;
  View *view;
  int i;

  test_init ();
  btk_init (&argc, &argv);
  
  buffer = create_buffer ();
  view = create_view (buffer);
  buffer_unref (buffer);
  
  push_active_window (BTK_WINDOW (view->window));
  for (i=1; i < argc; i++)
    {
      char *filename;

      /* Quick and dirty canonicalization - better should be in GLib
       */

      if (!g_path_is_absolute (argv[i]))
	{
	  char *cwd = g_get_current_dir ();
	  filename = g_strconcat (cwd, "/", argv[i], NULL);
	  g_free (cwd);
	}
      else
	filename = argv[i];

      open_ok_func (filename, view);

      if (filename != argv[i])
	g_free (filename);
    }
  pop_active_window ();
  
  btk_main ();

  return 0;
}


