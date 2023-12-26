/* Text Widget/Multiple Views
 *
 * The BtkTextView widget displays a BtkTextBuffer. One BtkTextBuffer
 * can be displayed by multiple BtkTextViews. This demo has two views
 * displaying a single buffer, and shows off the widget's text
 * formatting features.
 *
 */

#include <btk/btk.h>
#include <stdlib.h> /* for exit() */

#include "demo-common.h"

static void easter_egg_callback (BtkWidget *button, gpointer data);

static void
create_tags (BtkTextBuffer *buffer)
{
  /* Create a bunch of tags. Note that it's also possible to
   * create tags with btk_text_tag_new() then add them to the
   * tag table for the buffer, btk_text_buffer_create_tag() is
   * just a convenience function. Also note that you don't have
   * to give tags a name; pass NULL for the name to create an
   * anonymous tag.
   *
   * In any real app, another useful optimization would be to create
   * a BtkTextTagTable in advance, and reuse the same tag table for
   * all the buffers with the same tag set, instead of creating
   * new copies of the same tags for every buffer.
   *
   * Tags are assigned default priorities in order of addition to the
   * tag table.	 That is, tags created later that affect the same text
   * property affected by an earlier tag will override the earlier
   * tag.  You can modify tag priorities with
   * btk_text_tag_set_priority().
   */

  btk_text_buffer_create_tag (buffer, "heading",
			      "weight", BANGO_WEIGHT_BOLD,
			      "size", 15 * BANGO_SCALE,
			      NULL);
  
  btk_text_buffer_create_tag (buffer, "italic",
			      "style", BANGO_STYLE_ITALIC, NULL);

  btk_text_buffer_create_tag (buffer, "bold",
			      "weight", BANGO_WEIGHT_BOLD, NULL);  
  
  btk_text_buffer_create_tag (buffer, "big",
			      /* points times the BANGO_SCALE factor */
			      "size", 20 * BANGO_SCALE, NULL);

  btk_text_buffer_create_tag (buffer, "xx-small",
			      "scale", BANGO_SCALE_XX_SMALL, NULL);

  btk_text_buffer_create_tag (buffer, "x-large",
			      "scale", BANGO_SCALE_X_LARGE, NULL);
  
  btk_text_buffer_create_tag (buffer, "monospace",
			      "family", "monospace", NULL);
  
  btk_text_buffer_create_tag (buffer, "blue_foreground",
			      "foreground", "blue", NULL);  

  btk_text_buffer_create_tag (buffer, "red_background",
			      "background", "red", NULL);

  btk_text_buffer_create_tag (buffer, "big_gap_before_line",
			      "pixels_above_lines", 30, NULL);

  btk_text_buffer_create_tag (buffer, "big_gap_after_line",
			      "pixels_below_lines", 30, NULL);

  btk_text_buffer_create_tag (buffer, "double_spaced_line",
			      "pixels_inside_wrap", 10, NULL);

  btk_text_buffer_create_tag (buffer, "not_editable",
			      "editable", FALSE, NULL);
  
  btk_text_buffer_create_tag (buffer, "word_wrap",
			      "wrap_mode", BTK_WRAP_WORD, NULL);

  btk_text_buffer_create_tag (buffer, "char_wrap",
			      "wrap_mode", BTK_WRAP_CHAR, NULL);

  btk_text_buffer_create_tag (buffer, "no_wrap",
			      "wrap_mode", BTK_WRAP_NONE, NULL);
  
  btk_text_buffer_create_tag (buffer, "center",
			      "justification", BTK_JUSTIFY_CENTER, NULL);

  btk_text_buffer_create_tag (buffer, "right_justify",
			      "justification", BTK_JUSTIFY_RIGHT, NULL);

  btk_text_buffer_create_tag (buffer, "wide_margins",
			      "left_margin", 50, "right_margin", 50,
			      NULL);
  
  btk_text_buffer_create_tag (buffer, "strikethrough",
			      "strikethrough", TRUE, NULL);
  
  btk_text_buffer_create_tag (buffer, "underline",
			      "underline", BANGO_UNDERLINE_SINGLE, NULL);

  btk_text_buffer_create_tag (buffer, "double_underline",
			      "underline", BANGO_UNDERLINE_DOUBLE, NULL);

  btk_text_buffer_create_tag (buffer, "superscript",
			      "rise", 10 * BANGO_SCALE,	  /* 10 pixels */
			      "size", 8 * BANGO_SCALE,	  /* 8 points */
			      NULL);
  
  btk_text_buffer_create_tag (buffer, "subscript",
			      "rise", -10 * BANGO_SCALE,   /* 10 pixels */
			      "size", 8 * BANGO_SCALE,	   /* 8 points */
			      NULL);

  btk_text_buffer_create_tag (buffer, "rtl_quote",
			      "wrap_mode", BTK_WRAP_WORD,
			      "direction", BTK_TEXT_DIR_RTL,
			      "indent", 30,
			      "left_margin", 20,
			      "right_margin", 20,
			      NULL);
}

static void
insert_text (BtkTextBuffer *buffer)
{
  BtkTextIter iter;
  BtkTextIter start, end;
  BdkPixbuf *pixbuf;
  BdkPixbuf *scaled;
  BtkTextChildAnchor *anchor;
  char *filename;

  /* demo_find_file() looks in the current directory first,
   * so you can run btk-demo without installing BTK, then looks
   * in the location where the file is installed.
   */
  pixbuf = NULL;
  filename = demo_find_file ("btk-logo-rgb.gif", NULL);
  if (filename)
    {
      pixbuf = bdk_pixbuf_new_from_file (filename, NULL);
      g_free (filename);
    }

  if (pixbuf == NULL)
    {
      g_printerr ("Failed to load image file btk-logo-rgb.gif\n");
      exit (1);
    }

  scaled = bdk_pixbuf_scale_simple (pixbuf, 32, 32, BDK_INTERP_BILINEAR);
  g_object_unref (pixbuf);
  pixbuf = scaled;
  
  /* get start of buffer; each insertion will revalidate the
   * iterator to point to just after the inserted text.
   */
  btk_text_buffer_get_iter_at_offset (buffer, &iter, 0);

  btk_text_buffer_insert (buffer, &iter, "The text widget can display text with all kinds of nifty attributes. It also supports multiple views of the same buffer; this demo is showing the same buffer in two places.\n\n", -1);

  btk_text_buffer_insert_with_tags_by_name (buffer, &iter, "Font styles. ", -1,
					    "heading", NULL);
  
  btk_text_buffer_insert (buffer, &iter, "For example, you can have ", -1);
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "italic", -1,
					    "italic", NULL);
  btk_text_buffer_insert (buffer, &iter, ", ", -1);  
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "bold", -1,
					    "bold", NULL);
  btk_text_buffer_insert (buffer, &iter, ", or ", -1);
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "monospace (typewriter)", -1,
					    "monospace", NULL);
  btk_text_buffer_insert (buffer, &iter, ", or ", -1);
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "big", -1,
					    "big", NULL);
  btk_text_buffer_insert (buffer, &iter, " text. ", -1);
  btk_text_buffer_insert (buffer, &iter, "It's best not to hardcode specific text sizes; you can use relative sizes as with CSS, such as ", -1);
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "xx-small", -1,
					    "xx-small", NULL);
  btk_text_buffer_insert (buffer, &iter, " or ", -1);
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "x-large", -1,
					    "x-large", NULL);
  btk_text_buffer_insert (buffer, &iter, " to ensure that your program properly adapts if the user changes the default font size.\n\n", -1);
  
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter, "Colors. ", -1,
					    "heading", NULL);
  
  btk_text_buffer_insert (buffer, &iter, "Colors such as ", -1);  
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "a blue foreground", -1,
					    "blue_foreground", NULL);
  btk_text_buffer_insert (buffer, &iter, " or ", -1);  
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "a red background", -1,
					    "red_background", NULL);
  btk_text_buffer_insert (buffer, &iter, " or even ", -1);  
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "a blue foreground on red background", -1,
					    "blue_foreground",
					    "red_background",
					    NULL);
  btk_text_buffer_insert (buffer, &iter, " (select that to read it) can be used.\n\n", -1);  

  btk_text_buffer_insert_with_tags_by_name (buffer, &iter, "Underline, strikethrough, and rise. ", -1,
					    "heading", NULL);
  
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "Strikethrough", -1,
					    "strikethrough", NULL);
  btk_text_buffer_insert (buffer, &iter, ", ", -1);
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "underline", -1,
					    "underline", NULL);
  btk_text_buffer_insert (buffer, &iter, ", ", -1);
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "double underline", -1, 
					    "double_underline", NULL);
  btk_text_buffer_insert (buffer, &iter, ", ", -1);
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "superscript", -1,
					    "superscript", NULL);
  btk_text_buffer_insert (buffer, &iter, ", and ", -1);
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "subscript", -1,
					    "subscript", NULL);
  btk_text_buffer_insert (buffer, &iter, " are all supported.\n\n", -1);

  btk_text_buffer_insert_with_tags_by_name (buffer, &iter, "Images. ", -1,
					    "heading", NULL);
  
  btk_text_buffer_insert (buffer, &iter, "The buffer can have images in it: ", -1);
  btk_text_buffer_insert_pixbuf (buffer, &iter, pixbuf);
  btk_text_buffer_insert_pixbuf (buffer, &iter, pixbuf);
  btk_text_buffer_insert_pixbuf (buffer, &iter, pixbuf);
  btk_text_buffer_insert (buffer, &iter, " for example.\n\n", -1);

  btk_text_buffer_insert_with_tags_by_name (buffer, &iter, "Spacing. ", -1,
					    "heading", NULL);

  btk_text_buffer_insert (buffer, &iter, "You can adjust the amount of space before each line.\n", -1);
  
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "This line has a whole lot of space before it.\n", -1,
					    "big_gap_before_line", "wide_margins", NULL);
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "You can also adjust the amount of space after each line; this line has a whole lot of space after it.\n", -1,
					    "big_gap_after_line", "wide_margins", NULL);
  
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "You can also adjust the amount of space between wrapped lines; this line has extra space between each wrapped line in the same paragraph. To show off wrapping, some filler text: the quick brown fox jumped over the lazy dog. Blah blah blah blah blah blah blah blah blah.\n", -1,
					    "double_spaced_line", "wide_margins", NULL);

  btk_text_buffer_insert (buffer, &iter, "Also note that those lines have extra-wide margins.\n\n", -1);

  btk_text_buffer_insert_with_tags_by_name (buffer, &iter, "Editability. ", -1,
					    "heading", NULL);
  
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "This line is 'locked down' and can't be edited by the user - just try it! You can't delete this line.\n\n", -1,
					    "not_editable", NULL);

  btk_text_buffer_insert_with_tags_by_name (buffer, &iter, "Wrapping. ", -1,
					    "heading", NULL);

  btk_text_buffer_insert (buffer, &iter,
			  "This line (and most of the others in this buffer) is word-wrapped, using the proper Unicode algorithm. Word wrap should work in all scripts and languages that BTK+ supports. Let's make this a long paragraph to demonstrate: blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah\n\n", -1);  
  
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "This line has character-based wrapping, and can wrap between any two character glyphs. Let's make this a long paragraph to demonstrate: blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah\n\n", -1,
					    "char_wrap", NULL);
  
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "This line has all wrapping turned off, so it makes the horizontal scrollbar appear.\n\n\n", -1,
					    "no_wrap", NULL);

  btk_text_buffer_insert_with_tags_by_name (buffer, &iter, "Justification. ", -1,
					    "heading", NULL);  
  
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "\nThis line has center justification.\n", -1,
					    "center", NULL);

  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "This line has right justification.\n", -1,
					    "right_justify", NULL);

  btk_text_buffer_insert_with_tags_by_name (buffer, &iter,
					    "\nThis line has big wide margins. Text text text text text text text text text text text text text text text text text text text text text text text text text text text text text text text text text text text text.\n", -1,
					    "wide_margins", NULL);  

  btk_text_buffer_insert_with_tags_by_name (buffer, &iter, "Internationalization. ", -1,
					    "heading", NULL);
	  
  btk_text_buffer_insert (buffer, &iter,
			  "You can put all sorts of Unicode text in the buffer.\n\nGerman (Deutsch S\303\274d) Gr\303\274\303\237 Gott\nGreek (\316\225\316\273\316\273\316\267\316\275\316\271\316\272\316\254) \316\223\316\265\316\271\316\254 \317\203\316\261\317\202\nHebrew	\327\251\327\234\327\225\327\235\nJapanese (\346\227\245\346\234\254\350\252\236)\n\nThe widget properly handles bidirectional text, word wrapping, DOS/UNIX/Unicode paragraph separators, grapheme boundaries, and so on using the Bango internationalization framework.\n", -1);  

  btk_text_buffer_insert (buffer, &iter, "Here's a word-wrapped quote in a right-to-left language:\n", -1);
  btk_text_buffer_insert_with_tags_by_name (buffer, &iter, "\331\210\331\202\330\257 \330\250\330\257\330\243 \330\253\331\204\330\247\330\253 \331\205\331\206 \330\243\331\203\330\253\330\261 \330\247\331\204\331\205\330\244\330\263\330\263\330\247\330\252 \330\252\331\202\330\257\331\205\330\247 \331\201\331\212 \330\264\330\250\331\203\330\251 \330\247\331\203\330\263\331\212\331\210\331\206 \330\250\330\261\330\247\331\205\330\254\331\207\330\247 \331\203\331\205\331\206\330\270\331\205\330\247\330\252 \331\204\330\247 \330\252\330\263\330\271\331\211 \331\204\331\204\330\261\330\250\330\255\330\214 \330\253\331\205 \330\252\330\255\331\210\331\204\330\252 \331\201\331\212 \330\247\331\204\330\263\331\206\331\210\330\247\330\252 \330\247\331\204\330\256\331\205\330\263 \330\247\331\204\331\205\330\247\330\266\331\212\330\251 \330\245\331\204\331\211 \331\205\330\244\330\263\330\263\330\247\330\252 \331\205\330\247\331\204\331\212\330\251 \331\205\331\206\330\270\331\205\330\251\330\214 \331\210\330\250\330\247\330\252\330\252 \330\254\330\262\330\241\330\247 \331\205\331\206 \330\247\331\204\331\206\330\270\330\247\331\205 \330\247\331\204\331\205\330\247\331\204\331\212 \331\201\331\212 \330\250\331\204\330\257\330\247\331\206\331\207\330\247\330\214 \331\210\331\204\331\203\331\206\331\207\330\247 \330\252\330\252\330\256\330\265\330\265 \331\201\331\212 \330\256\330\257\331\205\330\251 \331\202\330\267\330\247\330\271 \330\247\331\204\331\205\330\264\330\261\331\210\330\271\330\247\330\252 \330\247\331\204\330\265\330\272\331\212\330\261\330\251. \331\210\330\243\330\255\330\257 \330\243\331\203\330\253\330\261 \331\207\330\260\331\207 \330\247\331\204\331\205\330\244\330\263\330\263\330\247\330\252 \331\206\330\254\330\247\330\255\330\247 \331\207\331\210 \302\273\330\250\330\247\331\206\331\203\331\210\330\263\331\210\331\204\302\253 \331\201\331\212 \330\250\331\210\331\204\331\212\331\201\331\212\330\247.\n\n", -1,
						"rtl_quote", NULL);
      
  btk_text_buffer_insert (buffer, &iter, "You can put widgets in the buffer: Here's a button: ", -1);
  anchor = btk_text_buffer_create_child_anchor (buffer, &iter);
  btk_text_buffer_insert (buffer, &iter, " and a menu: ", -1);
  anchor = btk_text_buffer_create_child_anchor (buffer, &iter);
  btk_text_buffer_insert (buffer, &iter, " and a scale: ", -1);
  anchor = btk_text_buffer_create_child_anchor (buffer, &iter);
  btk_text_buffer_insert (buffer, &iter, " and an animation: ", -1);
  anchor = btk_text_buffer_create_child_anchor (buffer, &iter);
  btk_text_buffer_insert (buffer, &iter, " finally a text entry: ", -1);
  anchor = btk_text_buffer_create_child_anchor (buffer, &iter);
  btk_text_buffer_insert (buffer, &iter, ".\n", -1);
  
  btk_text_buffer_insert (buffer, &iter, "\n\nThis demo doesn't demonstrate all the BtkTextBuffer features; it leaves out, for example: invisible/hidden text, tab stops, application-drawn areas on the sides of the widget for displaying breakpoints and such...", -1);

  /* Apply word_wrap tag to whole buffer */
  btk_text_buffer_get_bounds (buffer, &start, &end);
  btk_text_buffer_apply_tag_by_name (buffer, "word_wrap", &start, &end);

  g_object_unref (pixbuf);
}

static gboolean
find_anchor (BtkTextIter *iter)
{
  while (btk_text_iter_forward_char (iter))
    {
      if (btk_text_iter_get_child_anchor (iter))
        return TRUE;
    }
  return FALSE;
}

static void
attach_widgets (BtkTextView *text_view)
{
  BtkTextIter iter;
  BtkTextBuffer *buffer;
  int i;
  
  buffer = btk_text_view_get_buffer (text_view);

  btk_text_buffer_get_start_iter (buffer, &iter);

  i = 0;
  while (find_anchor (&iter))
    {
      BtkTextChildAnchor *anchor;
      BtkWidget *widget;
      
      anchor = btk_text_iter_get_child_anchor (&iter);

      if (i == 0)
        {
          widget = btk_button_new_with_label ("Click Me");

          g_signal_connect (widget, "clicked",
                            G_CALLBACK (easter_egg_callback),
                            NULL);
        }
      else if (i == 1)
        {
          widget = btk_combo_box_text_new ();

          btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (widget), "Option 1");
          btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (widget), "Option 2");
          btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (widget), "Option 3");
        }
      else if (i == 2)
        {
          widget = btk_hscale_new (NULL);
          btk_range_set_range (BTK_RANGE (widget), 0, 100);
          btk_widget_set_size_request (widget, 70, -1);
        }
      else if (i == 3)
        {
	  gchar *filename = demo_find_file ("floppybuddy.gif", NULL);
	  widget = btk_image_new_from_file (filename);
	  g_free (filename);
        }
      else if (i == 4)
        {
          widget = btk_entry_new ();
        }
      else
        {
          widget = NULL; /* avoids a compiler warning */
          g_assert_not_reached ();
        }

      btk_text_view_add_child_at_anchor (text_view,
                                         widget,
                                         anchor);

      btk_widget_show_all (widget);

      ++i;
    }
}

BtkWidget *
do_textview (BtkWidget *do_widget)
{
  static BtkWidget *window = NULL;

  if (!window)
    {
      BtkWidget *vpaned;
      BtkWidget *view1;
      BtkWidget *view2;
      BtkWidget *sw;
      BtkTextBuffer *buffer;
      
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (do_widget));
      btk_window_set_default_size (BTK_WINDOW (window),
				   450, 450);
      
      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed), &window);

      btk_window_set_title (BTK_WINDOW (window), "TextView");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      vpaned = btk_vpaned_new ();
      btk_container_set_border_width (BTK_CONTAINER(vpaned), 5);
      btk_container_add (BTK_CONTAINER (window), vpaned);

      /* For convenience, we just use the autocreated buffer from
       * the first text view; you could also create the buffer
       * by itself with btk_text_buffer_new(), then later create
       * a view widget.
       */
      view1 = btk_text_view_new ();
      buffer = btk_text_view_get_buffer (BTK_TEXT_VIEW (view1));
      view2 = btk_text_view_new_with_buffer (buffer);
      
      sw = btk_scrolled_window_new (NULL, NULL);
      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (sw),
				      BTK_POLICY_AUTOMATIC,
				      BTK_POLICY_AUTOMATIC);
      btk_paned_add1 (BTK_PANED (vpaned), sw);

      btk_container_add (BTK_CONTAINER (sw), view1);

      sw = btk_scrolled_window_new (NULL, NULL);
      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (sw),
				      BTK_POLICY_AUTOMATIC,
				      BTK_POLICY_AUTOMATIC);
      btk_paned_add2 (BTK_PANED (vpaned), sw);

      btk_container_add (BTK_CONTAINER (sw), view2);

      create_tags (buffer);
      insert_text (buffer);

      attach_widgets (BTK_TEXT_VIEW (view1));
      attach_widgets (BTK_TEXT_VIEW (view2));
      
      btk_widget_show_all (vpaned);
    }

  if (!btk_widget_get_visible (window))
    {
      btk_widget_show (window);
    }
  else
    {
      btk_widget_destroy (window);
      window = NULL;
    }

  return window;
}

static void
recursive_attach_view (int                 depth,
                       BtkTextView        *view,
                       BtkTextChildAnchor *anchor)
{
  BtkWidget *child_view;
  BtkWidget *event_box;
  BdkColor color;
  BtkWidget *align;
  
  if (depth > 4)
    return;
  
  child_view = btk_text_view_new_with_buffer (btk_text_view_get_buffer (view));

  /* Event box is to add a black border around each child view */
  event_box = btk_event_box_new ();
  bdk_color_parse ("black", &color);
  btk_widget_modify_bg (event_box, BTK_STATE_NORMAL, &color);

  align = btk_alignment_new (0.5, 0.5, 1.0, 1.0);
  btk_container_set_border_width (BTK_CONTAINER (align), 1);
  
  btk_container_add (BTK_CONTAINER (event_box), align);
  btk_container_add (BTK_CONTAINER (align), child_view);
  
  btk_text_view_add_child_at_anchor (view, event_box, anchor);

  recursive_attach_view (depth + 1, BTK_TEXT_VIEW (child_view), anchor);
}

static void
easter_egg_callback (BtkWidget *button,
                     gpointer   data)
{
  static BtkWidget *window = NULL;
  gpointer window_ptr;
  BtkTextBuffer *buffer;
  BtkWidget     *view;
  BtkTextIter    iter;
  BtkTextChildAnchor *anchor;
  BtkWidget *sw;

  if (window)
    {
      btk_window_present (BTK_WINDOW (window));
      return;
    }
  
  buffer = btk_text_buffer_new (NULL);

  btk_text_buffer_get_start_iter (buffer, &iter);

  btk_text_buffer_insert (buffer, &iter,
                          "This buffer is shared by a set of nested text views.\n Nested view:\n", -1);
  anchor = btk_text_buffer_create_child_anchor (buffer, &iter);
  btk_text_buffer_insert (buffer, &iter,
                          "\nDon't do this in real applications, please.\n", -1);

  view = btk_text_view_new_with_buffer (buffer);
  
  recursive_attach_view (0, BTK_TEXT_VIEW (view), anchor);
  
  g_object_unref (buffer);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  sw = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (sw),
                                  BTK_POLICY_AUTOMATIC,
                                  BTK_POLICY_AUTOMATIC);

  btk_container_add (BTK_CONTAINER (window), sw);
  btk_container_add (BTK_CONTAINER (sw), view);

  window_ptr = &window;
  g_object_add_weak_pointer (G_OBJECT (window), window_ptr);

  btk_window_set_default_size (BTK_WINDOW (window), 300, 400);
  
  btk_widget_show_all (window);
}

