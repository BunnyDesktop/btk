/* Text Widget/Automatic scrolling
 *
 * This example demonstrates how to use the gravity of 
 * BtkTextMarks to keep a text view scrolled to the bottom
 * while appending text.
 */

#include <btk/btk.h>
#include "demo-common.h"

/* Scroll to the end of the buffer.
 */
static gboolean
scroll_to_end (BtkTextView *textview)
{
  BtkTextBuffer *buffer;
  BtkTextIter iter;
  BtkTextMark *mark;
  char *spaces;
  static int count;

  buffer = btk_text_view_get_buffer (textview);
  
  /* Get "end" mark. It's located at the end of buffer because 
   * of right gravity
   */
  mark = btk_text_buffer_get_mark (buffer, "end");
  btk_text_buffer_get_iter_at_mark (buffer, &iter, mark);

  /* and insert some text at its position, the iter will be 
   * revalidated after insertion to point to the end of inserted text
   */
  spaces = g_strnfill (count++, ' ');
  btk_text_buffer_insert (buffer, &iter, "\n", -1);
  btk_text_buffer_insert (buffer, &iter, spaces, -1);
  btk_text_buffer_insert (buffer, &iter,
                          "Scroll to end scroll to end scroll "
                          "to end scroll to end ",
                          -1);
  g_free (spaces);

  /* Now scroll the end mark onscreen.
   */
  btk_text_view_scroll_mark_onscreen (textview, mark);

  /* Emulate typewriter behavior, shift to the left if we 
   * are far enough to the right.
   */
  if (count > 150)
    count = 0;

  return TRUE;
}

/* Scroll to the bottom of the buffer.
 */
static gboolean
scroll_to_bottom (BtkTextView *textview)
{
  BtkTextBuffer *buffer;
  BtkTextIter iter;
  BtkTextMark *mark;
  char *spaces;
  static int count;

  buffer = btk_text_view_get_buffer (textview);
  
  /* Get end iterator */
  btk_text_buffer_get_end_iter (buffer, &iter);

  /* and insert some text at it, the iter will be revalidated
   * after insertion to point to the end of inserted text
   */
  spaces = g_strnfill (count++, ' ');
  btk_text_buffer_insert (buffer, &iter, "\n", -1);
  btk_text_buffer_insert (buffer, &iter, spaces, -1);
  btk_text_buffer_insert (buffer, &iter,
                          "Scroll to bottom scroll to bottom scroll "
                          "to bottom scroll to bottom",
                          -1);
  g_free (spaces);

  /* Move the iterator to the beginning of line, so we don't scroll 
   * in horizontal direction 
   */
  btk_text_iter_set_line_offset (&iter, 0);
  
  /* and place the mark at iter. the mark will stay there after we
   * insert some text at the end because it has right gravity.
   */
  mark = btk_text_buffer_get_mark (buffer, "scroll");
  btk_text_buffer_move_mark (buffer, mark, &iter);
  
  /* Scroll the mark onscreen.
   */
  btk_text_view_scroll_mark_onscreen (textview, mark);

  /* Shift text back if we got enough to the right.
   */
  if (count > 40)
    count = 0;

  return TRUE;
}

static guint
setup_scroll (BtkTextView *textview,
              gboolean     to_end)
{
  BtkTextBuffer *buffer;
  BtkTextIter iter;

  buffer = btk_text_view_get_buffer (textview);
  btk_text_buffer_get_end_iter (buffer, &iter);

  if (to_end)
  {
    /* If we want to scroll to the end, including horizontal scrolling,
     * then we just create a mark with right gravity at the end of the 
     * buffer. It will stay at the end unless explicitely moved with 
     * btk_text_buffer_move_mark.
     */
    btk_text_buffer_create_mark (buffer, "end", &iter, FALSE);
    
    /* Add scrolling timeout. */
    return g_timeout_add (50, (GSourceFunc) scroll_to_end, textview);
  }
  else
  {
    /* If we want to scroll to the bottom, but not scroll horizontally, 
     * then an end mark won't do the job. Just create a mark so we can 
     * use it with btk_text_view_scroll_mark_onscreen, we'll position it
     * explicitely when needed. Use left gravity so the mark stays where 
     * we put it after inserting new text.
     */
    btk_text_buffer_create_mark (buffer, "scroll", &iter, TRUE);
    
    /* Add scrolling timeout. */
    return g_timeout_add (100, (GSourceFunc) scroll_to_bottom, textview);
  }
}

static void
remove_timeout (BtkWidget *window,
                gpointer   timeout)
{
  g_source_remove (GPOINTER_TO_UINT (timeout));
}

static void
create_text_view (BtkWidget *hbox,
                  gboolean   to_end)
{
  BtkWidget *swindow;
  BtkWidget *textview;
  guint timeout;

  swindow = btk_scrolled_window_new (NULL, NULL);
  btk_box_pack_start (BTK_BOX (hbox), swindow, TRUE, TRUE, 0);
  textview = btk_text_view_new ();
  btk_container_add (BTK_CONTAINER (swindow), textview);

  timeout = setup_scroll (BTK_TEXT_VIEW (textview), to_end);

  /* Remove the timeout in destroy handler, so we don't try to
   * scroll destroyed widget. 
   */
  g_signal_connect (textview, "destroy",
                    G_CALLBACK (remove_timeout),
                    GUINT_TO_POINTER (timeout));
}

BtkWidget *
do_textscroll (BtkWidget *do_widget)
{
  static BtkWidget *window = NULL;

  if (!window)
    {
      BtkWidget *hbox;

      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed), &window);
      btk_window_set_default_size (BTK_WINDOW (window), 600, 400);
      
      hbox = btk_hbox_new (TRUE, 6);
      btk_container_add (BTK_CONTAINER (window), hbox);

      create_text_view (hbox, TRUE);
      create_text_view (hbox, FALSE);
    }

  if (!btk_widget_get_visible (window))
      btk_widget_show_all (window);
  else
      btk_widget_destroy (window);

  return window;
}
