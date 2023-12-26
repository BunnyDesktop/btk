/* Text Widget/Hypertext
 *
 * Usually, tags modify the appearance of text in the view, e.g. making it 
 * bold or colored or underlined. But tags are not restricted to appearance. 
 * They can also affect the behavior of mouse and key presses, as this demo 
 * shows.
 */

#include <btk/btk.h>
#include <bdk/bdkkeysyms.h>

/* Inserts a piece of text into the buffer, giving it the usual
 * appearance of a hyperlink in a web browser: blue and underlined.
 * Additionally, attaches some data on the tag, to make it recognizable
 * as a link. 
 */
static void 
insert_link (BtkTextBuffer *buffer, 
             BtkTextIter   *iter, 
             gchar         *text, 
             gint           page)
{
  BtkTextTag *tag;
  
  tag = btk_text_buffer_create_tag (buffer, NULL, 
                                    "foreground", "blue", 
                                    "underline", BANGO_UNDERLINE_SINGLE, 
                                    NULL);
  g_object_set_data (B_OBJECT (tag), "page", GINT_TO_POINTER (page));
  btk_text_buffer_insert_with_tags (buffer, iter, text, -1, tag, NULL);
}

/* Fills the buffer with text and interspersed links. In any real
 * hypertext app, this method would parse a file to identify the links.
 */
static void
show_page (BtkTextBuffer *buffer, 
           gint           page)
{
  BtkTextIter iter;

  btk_text_buffer_set_text (buffer, "", 0);
  btk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
  if (page == 1)
    {
      btk_text_buffer_insert (buffer, &iter, "Some text to show that simple ", -1);
      insert_link (buffer, &iter, "hypertext", 3);
      btk_text_buffer_insert (buffer, &iter, " can easily be realized with ", -1);
      insert_link (buffer, &iter, "tags", 2);
      btk_text_buffer_insert (buffer, &iter, ".", -1);
    }
  else if (page == 2)
    {
      btk_text_buffer_insert (buffer, &iter, 
                              "A tag is an attribute that can be applied to some range of text. "
                              "For example, a tag might be called \"bold\" and make the text inside "
                              "the tag bold. However, the tag concept is more general than that; "
                              "tags don't have to affect appearance. They can instead affect the "
                              "behavior of mouse and key presses, \"lock\" a range of text so the "
                              "user can't edit it, or countless other things.\n", -1);
      insert_link (buffer, &iter, "Go back", 1);
    }
  else if (page == 3) 
    {
      BtkTextTag *tag;
  
      tag = btk_text_buffer_create_tag (buffer, NULL, 
                                        "weight", BANGO_WEIGHT_BOLD, 
                                        NULL);
      btk_text_buffer_insert_with_tags (buffer, &iter, "hypertext:\n", -1, tag, NULL);
      btk_text_buffer_insert (buffer, &iter, 
                              "machine-readable text that is not sequential but is organized "
                              "so that related items of information are connected.\n", -1);
      insert_link (buffer, &iter, "Go back", 1);
    }
}

/* Looks at all tags covering the position of iter in the text view, 
 * and if one of them is a link, follow it by showing the page identified
 * by the data attached to it.
 */
static void
follow_if_link (BtkWidget   *text_view, 
                BtkTextIter *iter)
{
  GSList *tags = NULL, *tagp = NULL;

  tags = btk_text_iter_get_tags (iter);
  for (tagp = tags;  tagp != NULL;  tagp = tagp->next)
    {
      BtkTextTag *tag = tagp->data;
      gint page = GPOINTER_TO_INT (g_object_get_data (B_OBJECT (tag), "page"));

      if (page != 0)
        {
          show_page (btk_text_view_get_buffer (BTK_TEXT_VIEW (text_view)), page);
          break;
        }
    }

  if (tags) 
    b_slist_free (tags);
}

/* Links can be activated by pressing Enter.
 */
static gboolean
key_press_event (BtkWidget *text_view,
                 BdkEventKey *event)
{
  BtkTextIter iter;
  BtkTextBuffer *buffer;

  switch (event->keyval)
    {
      case BDK_Return: 
      case BDK_KP_Enter:
        buffer = btk_text_view_get_buffer (BTK_TEXT_VIEW (text_view));
        btk_text_buffer_get_iter_at_mark (buffer, &iter, 
                                          btk_text_buffer_get_insert (buffer));
        follow_if_link (text_view, &iter);
        break;

      default:
        break;
    }

  return FALSE;
}

/* Links can also be activated by clicking.
 */
static gboolean
event_after (BtkWidget *text_view,
             BdkEvent  *ev)
{
  BtkTextIter start, end, iter;
  BtkTextBuffer *buffer;
  BdkEventButton *event;
  gint x, y;

  if (ev->type != BDK_BUTTON_RELEASE)
    return FALSE;

  event = (BdkEventButton *)ev;

  if (event->button != 1)
    return FALSE;

  buffer = btk_text_view_get_buffer (BTK_TEXT_VIEW (text_view));

  /* we shouldn't follow a link if the user has selected something */
  btk_text_buffer_get_selection_bounds (buffer, &start, &end);
  if (btk_text_iter_get_offset (&start) != btk_text_iter_get_offset (&end))
    return FALSE;

  btk_text_view_window_to_buffer_coords (BTK_TEXT_VIEW (text_view), 
                                         BTK_TEXT_WINDOW_WIDGET,
                                         event->x, event->y, &x, &y);

  btk_text_view_get_iter_at_location (BTK_TEXT_VIEW (text_view), &iter, x, y);

  follow_if_link (text_view, &iter);

  return FALSE;
}

static gboolean hovering_over_link = FALSE;
static BdkCursor *hand_cursor = NULL;
static BdkCursor *regular_cursor = NULL;

/* Looks at all tags covering the position (x, y) in the text view, 
 * and if one of them is a link, change the cursor to the "hands" cursor
 * typically used by web browsers.
 */
static void
set_cursor_if_appropriate (BtkTextView    *text_view,
                           gint            x,
                           gint            y)
{
  GSList *tags = NULL, *tagp = NULL;
  BtkTextIter iter;
  gboolean hovering = FALSE;

  btk_text_view_get_iter_at_location (text_view, &iter, x, y);
  
  tags = btk_text_iter_get_tags (&iter);
  for (tagp = tags;  tagp != NULL;  tagp = tagp->next)
    {
      BtkTextTag *tag = tagp->data;
      gint page = GPOINTER_TO_INT (g_object_get_data (B_OBJECT (tag), "page"));

      if (page != 0) 
        {
          hovering = TRUE;
          break;
        }
    }

  if (hovering != hovering_over_link)
    {
      hovering_over_link = hovering;

      if (hovering_over_link)
        bdk_window_set_cursor (btk_text_view_get_window (text_view, BTK_TEXT_WINDOW_TEXT), hand_cursor);
      else
        bdk_window_set_cursor (btk_text_view_get_window (text_view, BTK_TEXT_WINDOW_TEXT), regular_cursor);
    }

  if (tags) 
    b_slist_free (tags);
}

/* Update the cursor image if the pointer moved. 
 */
static gboolean
motion_notify_event (BtkWidget      *text_view,
                     BdkEventMotion *event)
{
  gint x, y;

  btk_text_view_window_to_buffer_coords (BTK_TEXT_VIEW (text_view), 
                                         BTK_TEXT_WINDOW_WIDGET,
                                         event->x, event->y, &x, &y);

  set_cursor_if_appropriate (BTK_TEXT_VIEW (text_view), x, y);

  bdk_window_get_pointer (btk_widget_get_window (text_view), NULL, NULL, NULL);
  return FALSE;
}

/* Also update the cursor image if the window becomes visible
 * (e.g. when a window covering it got iconified).
 */
static gboolean
visibility_notify_event (BtkWidget          *text_view,
                         BdkEventVisibility *event)
{
  gint wx, wy, bx, by;
  
  bdk_window_get_pointer (btk_widget_get_window (text_view), &wx, &wy, NULL);
  
  btk_text_view_window_to_buffer_coords (BTK_TEXT_VIEW (text_view), 
                                         BTK_TEXT_WINDOW_WIDGET,
                                         wx, wy, &bx, &by);

  set_cursor_if_appropriate (BTK_TEXT_VIEW (text_view), bx, by);

  return FALSE;
}

BtkWidget *
do_hypertext (BtkWidget *do_widget)
{
  static BtkWidget *window = NULL;

  if (!window)
    {
      BtkWidget *view;
      BtkWidget *sw;
      BtkTextBuffer *buffer;

      hand_cursor = bdk_cursor_new (BDK_HAND2);
      regular_cursor = bdk_cursor_new (BDK_XTERM);
      
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
                             btk_widget_get_screen (do_widget));
      btk_window_set_default_size (BTK_WINDOW (window),
                                   450, 450);
      
      g_signal_connect (window, "destroy",
                        G_CALLBACK (btk_widget_destroyed), &window);

      btk_window_set_title (BTK_WINDOW (window), "Hypertext");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      view = btk_text_view_new ();
      btk_text_view_set_wrap_mode (BTK_TEXT_VIEW (view), BTK_WRAP_WORD);
      g_signal_connect (view, "key-press-event", 
                        G_CALLBACK (key_press_event), NULL);
      g_signal_connect (view, "event-after", 
                        G_CALLBACK (event_after), NULL);
      g_signal_connect (view, "motion-notify-event", 
                        G_CALLBACK (motion_notify_event), NULL);
      g_signal_connect (view, "visibility-notify-event", 
                        G_CALLBACK (visibility_notify_event), NULL);

      buffer = btk_text_view_get_buffer (BTK_TEXT_VIEW (view));
      
      sw = btk_scrolled_window_new (NULL, NULL);
      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (sw),
                                      BTK_POLICY_AUTOMATIC,
                                      BTK_POLICY_AUTOMATIC);
      btk_container_add (BTK_CONTAINER (window), sw);
      btk_container_add (BTK_CONTAINER (sw), view);

      show_page (buffer, 1);

      btk_widget_show_all (sw);
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

